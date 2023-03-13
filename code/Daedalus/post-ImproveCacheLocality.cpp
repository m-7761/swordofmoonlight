#include "Daedalus.(c).h"  
using namespace Daedalus;
class ImproveCacheLocality //Assimp/ImproveCacheLocality.cpp
{	
	Daedalus::post &post; //TODO: allow for non-triangle meshes

	struct VertexTriangleAdjacency //Assimp/VertexTriangleAdjacency.h
	{
		/*size_t &GetNumTrianglesPtr(size_t vertIndex)
		{
			assert(vertIndex<safeVertices&&liveTriangles);
			return liveTriangles[vertIndex];
		}*/		
		inline size_t *GetAdjacentTriangles(signed vertIndex)
		{
			assert(vertIndex<safeVertices);
			return &adjacencyTable[offsetTable[vertIndex]];
		}
		std::vector<size_t> offsetTable; 
		std::vector<size_t> adjacencyTable;
		//AI: Table containing the number of referenced triangles per vertex
		std::vector<size_t> liveTriangles;	
		signed safeVertices; //AI: Debug: Number of referenced vertices
		
		//Assimp/VertexTriangleAdjacency.cpp
		//NOTE: The header says that polygons ARE supported
		//HOWEVER THE CODE SUGGESTS OTHERWISE. HERE WE CAN ASSUME TRIANGLES
		//NOTE: Only ImproveCacheLocality utilizes Assimp's VertexTriangleAdjacency 		
		VertexTriangleAdjacency(){} //REPLACING CTOR WITH Set//		
		void SetTriangles(const preMesh *mesh)
		{
			safeVertices = mesh->Positions();

			//TODO: TRY TO WRITE THIS MORE CLEARLY
			liveTriangles.assign(safeVertices+1,0);
			offsetTable.resize(std::max(safeVertices+2u,offsetTable.size()));
			size_t *pi = liveTriangles.data(), *piEnd = pi+safeVertices; *piEnd++ = 0;
			//AI: first pass: compute the number of faces referencing each vertex
			//for(preFace *pcTriangle=pcTriangles;pcTriangle!=pcTriangleEnd;pcTriangle++)			
			mesh->FacesList()^[&](const preFace &ea)
			{
				auto il = mesh->IndicesSubList(ea);
				pi[il[0]]++; pi[il[1]]++; pi[il[2]]++; assert(ea.triangle==ea.polytype);
			};
			//AI: second pass: compute the final offset table
			size_t iSum = 0, *piCurOut = offsetTable.data()+1;
			for(size_t *piCur=pi;piCur!=piEnd;piCur++,piCurOut++)
			{
				size_t iLastSum = iSum; iSum+=*piCur; *piCurOut = iLastSum;
			}

			pi = offsetTable.data()+1;
			//AI: third pass: compute the final table
			adjacencyTable.resize(std::max(iSum,adjacencyTable.size()));
			iSum = 0; mesh->FacesList()^[&](const preFace &ea)
			{
				auto il = mesh->IndicesSubList(ea);
				adjacencyTable[pi[il[0]]++] = iSum;
				adjacencyTable[pi[il[1]]++] = iSum;
				adjacencyTable[pi[il[2]]++] = iSum++;
			};
			//+1? 
			//AI: fourth pass: undo the offset computations made during the third pass
			//We could do this in a separate buffer, but this would be TIMES slower.			
			offsetTable[0] = 0;
		}
	}adj; //lots and lots of storage		
	std::vector<size_t> piCachingStamps;
	std::vector<bool> abEmitted;	
	const std::vector<size_t> piNumTriPtrNoModify;
	std::vector<signed> piCandidates, piFIFOStack, piIBOutput, sDeadEndVStack;		
	double Process(preMesh *mesh, preID meshID)
	{					  
		//AI: See if the input data is valid
		//1.) there must be vertices and faces		
		if(!mesh->HasFaces()||!mesh->HasPositions()) return 0;
		//TODO: DEFEAT THIS REQUIREMENT
		//2.) all faces must be triangulated or we won't operate on them
		if(!mesh->_trianglesflag||mesh->_polygonsflag||mesh->_pointsflag||mesh->_linesflag)
		{
			//DO NOT FORGET TO GENERALIZE VertexTriangleAdjacency BEFORE REMOVING THIS TEST
			post.CriticalIssue("Sorry, this algorithm supports triangle meshes only. (It requires some work)");
			return 0;
		}

		//TODO: TRY TO WRITE THIS MORE CLEARLY

		const size_t cacheDepth = post.ImproveCacheLocality.configCacheDepth;
		if(mesh->positions<=cacheDepth)	return 0;

		double fACMR = 3;
		//AI: Input ACMR is for logging purposes only
		//(ACTUALLY IT RETURNS, SO THEORETICALLY NOT)
		if(post.ImproveCacheLocality.configLogACMR)
		{
			//AI: count the number of cache misses
			piFIFOStack.assign(cacheDepth,-1);
			signed iCacheMisses = 0, *piCur = piFIFOStack.data();			
			const signed *const piCurEnd = piFIFOStack.data()+cacheDepth;
			PreConst(mesh)->FacesList()^[&](const preFace &ea)
			{
				mesh->IndicesSubList(ea)^[&](signed ea2)
				{
					bool bInCache = false;
					for(signed *pp=piFIFOStack.data();pp<piCurEnd;pp++) 
					if(*pp==ea2)    
					{
						bInCache = true; break; //AI: the vertex is in cache
					}
					if(!bInCache)  
					{
						iCacheMisses++;
						if(piCurEnd==piCur) piCur = piFIFOStack.data();
						*piCur++ = ea2;
					}
				};
			};
			fACMR = (double)iCacheMisses/mesh->Faces();
			if(fACMR>=3) //AI: should be sufficiently large in every case
			{
				if(mesh->name.HasString()) post.Verbose(mesh->name.cstring);
				//AI: the JoinIdenticalVertices process has not been executed on this
				//mesh, otherwise ACMR would normally be at least minimally smaller than 3
				post.Verbose("Mesh ")<<meshID<<": Unsuitable for vcache optimization";

				return 0; //TODO? AND WHAT IF NOT LOGGING?! WHAT THEN
			}
		}

		//AI: first build a vertex-triangle adjacency list
		adj.SetTriangles(mesh);

		//AI: a list to store per-vertex caching time stamps
		piCachingStamps.assign(mesh->Positions(),0);

		//A: allocate an empty output index buffer. We store the output indices in one large array.
		//Since the number of triangles won't change the input faces can be reused. This is how
		//we save thousands of redundant mini allocations for preFace::indiceslist		
		const size_t iIdxCnt = mesh->Faces()*3;
		piIBOutput.resize(std::max(iIdxCnt,piIBOutput.size()));
		signed *piCSIter = piIBOutput.data();

		//AI: allocate the flag array to hold the information
		//whether a face has already been emitted or not
		abEmitted.assign(mesh->Faces(),false);

		//AI: dead-end vertex index stack
		assert(sDeadEndVStack.empty()); //paranoia
		while(!sDeadEndVStack.empty()) sDeadEndVStack.pop_back();		

		//AI: create a const copy of the piNumTriPtr buffer
		size_t *const piNumTriPtr = adj.liveTriangles.data();
		size_t *const piNumTriEnd = piNumTriPtr+mesh->Positions();
		const_cast<std::vector<size_t>&>(piNumTriPtrNoModify).assign(piNumTriPtr,piNumTriEnd);

		size_t iMaxRefTris = 0; //TODO? why not just do a push_back here?
		//AI: get the largest number of referenced triangles and allocate the "candidate buffer"
		for(size_t *piCur=piNumTriPtr;piCur!=piNumTriEnd;piCur++) iMaxRefTris = std::max(iMaxRefTris,*piCur);
		piCandidates.resize(std::max(iMaxRefTris*3,piCandidates.size()));

		size_t iCacheMisses = 0;
		/** AI: PSEUDOCODE for the algorithm 
			A = Build-Adjacency(I) Vertex-triangle adjacency
			L = Get-Triangle-Counts(A) Per-vertex live triangle counts
			C = Zero(Vertex-Count(I)) Per-vertex caching time stamps
			D = Empty-Stack() Dead-end vertex stack
			E = False(Triangle-Count(I)) Per triangle emitted flag
			O = Empty-Index-Buffer() Empty output buffer
			f = 0 Arbitrary starting vertex
			s = k+1, i = 1 Time stamp and cursor
			while f >= 0 For all valid fanning vertices
				N = Empty-Set() 1-ring of next candidates
				for each Triangle t in Neighbors(A, f)
					if !Emitted(E,t)
						for each Vertex v in t
							Append(O,v) Output vertex
							Push(D,v) Add to dead-end stack
							Insert(N,v) Register as candidate
							L[v] = L[v]-1 Decrease live triangle count
							if s-C[v] > k If not in cache
								C[v] = s Set time stamp
								s = s+1 Increment time stamp
						E[t] = true Flag triangle as emitted
				Select next fanning vertex
				f = Get-Next-Vertex(I,i,k,N,C,s,L,D)
			return O
		*/
		signed ivdx = 0, ics = 1;
		size_t iStampCnt = cacheDepth+1; while(ivdx>=0)   
		{
			size_t icnt = piNumTriPtrNoModify[ivdx];
			size_t *piList = adj.GetAdjacentTriangles(ivdx);
			signed *piCurCandidate = piCandidates.data();

			//AI: get all triangles in the neighborhood
			for(size_t tri=0;tri<icnt;tri++)    
			{
				//AI: if they have not yet been emitted, add them to the output IB
				const size_t fidx = *piList++; if(!abEmitted[fidx])   
				{
					//AI: so iterate through all vertices of the current triangle
					mesh->IndicesSubList(mesh->faceslist[fidx])^[&](signed ea)
					{
						//AI: the current vertex won't have any free triangles after this step
						if(ivdx!=ea) 
						{
							//AI: append the vertex to the dead-end stack
							sDeadEndVStack.push_back(ea);
							//AI: register as candidate for the next step
							*piCurCandidate++ = ea;	
							//AI: decrease the per-vertex triangle counts
							piNumTriPtr[ea]--;
						}
						//AI: append the vertex to the output index buffer
						*piCSIter++ = ea;
						//AI: if the vertex is not yet in cache, set its cache count
						if(iStampCnt-piCachingStamps[ea]>cacheDepth) 
						{
							piCachingStamps[ea] = iStampCnt++;
							iCacheMisses++;
						}
					};
					abEmitted[fidx] = true; //AI: flag triangle as emitted
				}
			}
			//AI: the vertex has now no living adjacent triangles anymore
			piNumTriPtr[ivdx] = 0;

			//AI: get next fanning vertex
			ivdx = -1; int max_priority = -1;
			for(signed *piCur=piCandidates.data();piCur!=piCurCandidate;piCur++)
			{
				//AI: must have live triangles
				const size_t dp = *piCur; if(piNumTriPtr[dp]>0)
				{
					size_t tmp; int priority = 0;
					//AI: will the vertex be in cache, even after fanning occurs?					
					if((tmp=iStampCnt-piCachingStamps[dp])+2*piNumTriPtr[dp]<=cacheDepth)
					priority = tmp;
					//AI: keep best candidate
					if(priority>max_priority){ ivdx = dp; max_priority = priority; }
				}
			}			
			if(-1==ivdx) //AI: did we reach a dead end?
			{
				//AI: need to get a non-local vertex for which 
				//there is a good chance that it is still in the cache 
				while(!sDeadEndVStack.empty()) 
				{
					signed iCachedIdx = sDeadEndVStack.back(); sDeadEndVStack.pop_back();

					if(piNumTriPtr[iCachedIdx]>0){ ivdx = iCachedIdx; break; }
				}
				//AI: if there isn't such a vertex,
				//get the next vertex in input order and hope it is not too bad								
				if(-1==ivdx) while(ics<adj.safeVertices)
				{
					if(piNumTriPtr[++ics]>0){ ivdx = ics; break; }
				}
			}
		}

		double fACMR2 = 0;		
		if(post.ImproveCacheLocality.configLogACMR)
		{
			fACMR2 = (double)iCacheMisses/mesh->faces;		
			if(mesh->name.HasString()) post.Verbose(mesh->name.String());
			//AI: very intense verbose logging; prepare for much text if there are many meshes
			post.Verbose("Mesh ")<<meshID<<" | ACMR in: "<<fACMR<<" out: "<<fACMR2<<" | ~"<<(fACMR-fACMR2)/fACMR*100<<"%";
			fACMR2*=mesh->Faces(); //!!
		}
		
		//AI: sort the output index buffer back to the input array		
		piCSIter = piIBOutput.data();		
		PreConst(mesh)->FacesList()^[&](const preFace &ea)
		{
			auto il = mesh->IndicesSubList(ea);
			il[0] = *piCSIter++; il[1] = *piCSIter++; il[2] = *piCSIter++;
		};
		return fACMR2;
	}

public: //ImproveCacheLocality

	ImproveCacheLocality(Daedalus::post *p)
	:post(*p){}operator bool()
	{		   
		PreSuppose(!PreMesh2::IsSupported);

		post.progLocalFileName("Improving Caching Locality (hopefully)");

		preScene *scene = post.Scene();

		if(!scene->Meshes()) 
		{
			post.Verbose("ImproveCacheLocalityProcess skipped; there are no meshes");
			return true;
		}
		else post.Verbose("ImproveCacheLocalityProcess begin");

		double acmr = 0;
		size_t numf = 0, numm = 0;
		preID meshID = 0; scene->MeshesList()^[&](preMesh *ea)
		{
			const double res = Process(ea,meshID++); if(!res) return void(ea);			
			numf+=ea->Faces(); numm++; acmr+=res;
		};
		if(acmr) //ACMR: Average Cache Miss Ratio (per face or per mesh?) 
		post("Cache relevant are ")<<numm<<" meshes ("<<numf<<" faces). Average output ACMR is "<<acmr/numf;
		post.Verbose("ImproveCacheLocalityProcess finished. ");		
		return true;
	}
};
static bool ImproveCacheLocality(Daedalus::post *post)
{
	return class ImproveCacheLocality(post);
}							
Daedalus::post::ImproveCacheLocality::ImproveCacheLocality
(post *p):Base(p,p->steps&step?::ImproveCacheLocality:0)
{
	//assuming this is hardwired into post's ctor
	assert(p->steps&JoinIdenticalVertices::step||!p->steps);

	configLogACMR = false; configCacheDepth = 32; //12;
}