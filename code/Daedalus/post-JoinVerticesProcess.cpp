#include "Daedalus.(c).h"  
using namespace Daedalus;
class JoinVerticesProcess //Assimp/JoinVerticesProcess.cpp
{	
	Daedalus::post &post; 
	 	
	struct Vertex //Assimp/Vertex.h
	{
		struct Param3D
		{double _[3]; //avoid construction
		inline operator const pre3D&()const{ return (pre3D&)*this; }
		}position, normal, tangent, bitangent,
		texcoords[PreMesh::texturecoordslistsN];
		struct Param4D
		{double _[4]; //avoid construction
		inline operator const pre4D&()const{ return (pre4D&)*this; }
		}colors[PreMesh::colorslistsN];
		struct Mesh
		{
			const preMesh *mesh;
			inline const preMesh *operator->()const{ return mesh; }
			size_t normal, tangent, texcoords, colors;			
			Mesh(const preMesh *p):mesh(p)
			{
				normal = p->HasNormals(); 
				tangent = p->HasTangentsAndBitangents();
				texcoords = p->CountTextureCoordsLists();
				colors = p->CountVertexColorsLists();
			}
		};
		inline Vertex(const Mesh &mesh, size_t i) 
		{
			PreSuppose(sizeof(position)==sizeof(pre3D));
			PreSuppose(sizeof(colors[0])==sizeof(pre4D));
			position = (Param3D&)mesh->positionslist[i];
			if(mesh.normal) normal = (Param3D&)mesh->normalslist[i];
			if(mesh.tangent) 
			{ tangent = (Param3D&)mesh->tangentslist[i]; 
			bitangent = (Param3D&)mesh->bitangentslist[i]; }
			for(size_t ii=0;ii<mesh.texcoords;ii++)
			texcoords[ii] = (Param3D&)mesh->texturecoordslists[ii][i];			
			for(size_t ii=0;ii<mesh.colors;ii++) 
			colors[ii] = (Param4D&)mesh->colorslists[ii][i];			
		}
	};	
	std::vector<signed> verticesFound;
	std::vector<Vertex> uniqueVertices;	
	std::vector<PreBone::Weight> newWeights;
	std::vector<std::pair<signed,bool>> replaceIndex;
	void JoinVerticesProcess::Process(preMesh *mesh, preID meshID)
	{
		if(!mesh->HasPositions()||!mesh->HasFaces()) return;

		//tricky: squared to avoid sqrt in CompareArrays		
		const double fixedEpsilon2 = post.fixedEpsilon2();				

		static const signed Unmatched = INT_MAX; 
		//NOTE: Assimp code packed second into the MSB of first
		//claiming it avoided another vector and yielded better branch prediction
		//(NATURALLY LET'S AVOID CRYPTIC CODE AND LET Intel WORRY ABOUT BRANCH PREDICTION)
		//per face index with a parallel track marking if it's an original or otherwise a duplicate
		replaceIndex.assign(mesh->Positions(),std::make_pair(Unmatched,true));
				
		//TODO? compute scene reserve size in advance
		uniqueVertices.clear();	uniqueVertices.reserve(mesh->Positions());

		const Vertex::Mesh vMesh(mesh); //NEW: Assimp doesn't include this

		//AI: Use optimized code path without multiple UVs or vertex colors.
		const bool complex = vMesh.texcoords>1||vMesh.colors;

		//AI: Now check each vertex if it brings something new to the table		
		auto pl = mesh->PositionsCoList(); for(size_t i=pl;i<pl.Size();i++)  
		{				
			const Vertex v(vMesh,i);

			#ifdef NDEBUG
			#error can't work. not including morphs in sortIndex (meshID)
			#endif
			//AI: Get all vertices that share this one
			post.SpatialSort_Compute.FindCommonPositions(meshID,v.position,verticesFound);
			
			signed matchIndex = Unmatched;

			//AI: check all unique vertices close to the position if this vertex is already present among them
			for(size_t j=0;j<verticesFound.size();j++) 
			{
				const size_t vidx = verticesFound[j];
				const size_t uidx = replaceIndex[vidx].first;
				if(replaceIndex[uidx].second) continue;

				const Vertex &uv = uniqueVertices[uidx];

				//AI: Position mismatch is impossible - the vertex finder already discarded all non-matching positions
				// We just test the other attributes even if they're not present in the mesh.
				// In this case they're initialized to 0 so the comparision succeeds.
				// By this method the non-present attributes are effectively ignored in the comparision.
				if(vMesh.normal&&(uv.normal-v.normal).SquareLength()>fixedEpsilon2)
				continue;
				if(vMesh.texcoords&&(uv.texcoords[0]-v.texcoords[0]).SquareLength()>fixedEpsilon2)
				continue;
				if(vMesh.tangent)
				if((uv.tangent-v.tangent).SquareLength()>fixedEpsilon2)
				continue;
				else if((uv.bitangent-v.bitangent).SquareLength()>fixedEpsilon2)
				continue;
								
				PreSuppose(8==mesh->colorslistsN&&8==mesh->texturecoordslistsN);
				//AI: Usually we won't have vertex colors or multiple UVs, so we can skip from here
				//This increases performance slightly, at least if branch prediction is on our side
				if(complex)
				{
					//AI: manually unrolled because continue wouldn't work as desired in an inner loop,
					//also because some compilers seem to fail the task (PRE: IN WHAT RESPECT???) 
					//Colors and UV coords are interleaved since the higher entries are most likely to
					//not be used. By interleaving the arrays, vertices are, on average, rejected earlier.
					#define _(i) \
					if(vMesh.texcoords>i+1&&(uv.texcoords[i+1]-v.texcoords[i+1]).SquareLength()>fixedEpsilon2)\
					continue;\
					if(vMesh.colors>i&&(uv.colors[i]-v.colors[i]).SquareLength()>fixedEpsilon2)\
					continue;
					_(0)_(1)_(2)_(3)_(4)_(5)_(6)_(7)
					#undef _
				}
				#ifdef NDEBUG
				#error must search over bone weights and morph targets
				#endif
				matchIndex = uidx; break;
			}
			if(matchIndex!=Unmatched)
			{
				//AI: store where to find the matching unique vertex
				replaceIndex[i] = std::make_pair(matchIndex,true);
			}
			else //AI: no unique vertex matches it upto now. So add it
			{				
				replaceIndex[i] = std::make_pair(uniqueVertices.size(),false);
				uniqueVertices.push_back(v);
			}
		}

		if(mesh->name.HasString()) post.Verbose(mesh->name.String());
		post.Verbose("Mesh ")<<meshID<<" | Verts in: "<<pl.Size()<<" out: "<<uniqueVertices.size()<<" | ~"<<(pl.Size()-uniqueVertices.size())/(double)pl.Size()*100<<"%";
		
		//AI: replace vertex data with the unique data sets
		pl.Reserve(uniqueVertices.size()); pl^=[&](size_t i){ pl[i] = uniqueVertices[i].position; };
		if(vMesh.normal)
		{
			auto l = mesh->NormalsCoList(); 
			l.Reserve(uniqueVertices.size()); l^=[&](size_t i){ l[i] = uniqueVertices[i].normal; };
		}		
		if(vMesh.tangent)
		{
			auto l = mesh->TangentsCoList(); l.Reserve(uniqueVertices.size());
			auto l2 = mesh->BitangentsCoList(); l2.Reserve(uniqueVertices.size());
			l^=[&](size_t i){ l[i] = uniqueVertices[i].tangent; l2[i] = uniqueVertices[i].bitangent; };
		}
		for(size_t i=0;i<vMesh.colors;i++)
		{
			auto l = mesh->VertexColorsCoList(i); 
			l.Reserve(uniqueVertices.size()); l^=[&](size_t j){ l[j] = uniqueVertices[j].colors[i]; };
		}
		for(size_t i=0;i<vMesh.texcoords;i++)
		{
			auto l = mesh->TextureCoordsCoList(i); 
			l.Reserve(uniqueVertices.size()); l^=[&](size_t j){ l[j] = uniqueVertices[j].texcoords[i]; };
		}
		mesh->IndicesList()^[&](signed &ea){ ea = replaceIndex[ea].first; };

		auto il = mesh->BonesList(); for(size_t i=il;i<il.Size();i++) deboned:
		{
			preBone *bone = il[i];
			newWeights.clear();	newWeights.reserve(bone->Weights());
			PreConst(bone)->WeightsList()^[&](const PreBone::Weight &ea)
			{
				if(!replaceIndex[ea].second) //AI: if the key is unique, translate it
				newWeights.push_back(PreBone::Weight(replaceIndex[ea].first,ea.value));
			}; 
			if(newWeights.empty()) //TODO: WOULD LIKE TO DO WITHOUT THIS
			{	
				//NOTE: assuming unanimated mesh separates vertices that touch when animated
				//(if needed bones can be factored into the battery of tests for uniqueness)

				 //AI: NOTE: 
				/*  In the algorithm above we're assuming that there are no vertices
				*  with a different bone weight setup at the same position. That wouldn't
				*  make sense, but it is not absolutely impossible. SkeletonMeshBuilder
				*  for example generates such input data if two skeleton points
				*  share the same position. Again this doesn't make sense but is
				*  reality for some model formats (MD5 for example uses these special
				*  nodes as attachment tags for its weapons).
				*
				*  Then it is possible that a bone has no weights anymore .... as a quick
				*  workaround, we're just removing these bones. If they're animated,
				*  model geometry might be modified but at least there's no risk of a crash.
				*/				
				il.ForgoMemory(il.Size()-1);
				for(size_t j=i;j<il.Size();j++)	il[j] = il[j+1];
				post.CriticalIssue("Removed bone with ")<<bone->Weights()<<" weights, as its vertices where all removed.";
				delete bone; if(i<il.Size()) goto deboned; 
				else if(0==i) il.Clear(); 
			}
			else bone->WeightsList().Assign(newWeights.size(),newWeights.data());
		}
	}

public: //JoinVerticesProcess

	JoinVerticesProcess(Daedalus::post *p)
	:post(*p){}operator bool()
	{	
		#ifdef NDEBUG
		#error unfinished
		#endif
		assert(!"JoinVerticesProcess requires more work. Disable it. Aborting.");
		post.CriticalIssue("JoinVerticesProcess requires more work. Disable it. Aborting.");
		return true;

		post.progLocalFileName("Joining Identical Vertices");

		PreSuppose(!PreMesh2::IsSupported&&!PreMorph::IsSupported);

		preScene *scene = post.Scene();
		post.Verbose("JoinVerticesProcess begin");		
		size_t oldVertices = 0, newVertices = 0; preID meshID = 0;
		#ifdef NDEBUG
		#error won't do. basically this step requires a complete overhaul like FindInstances, abandoning FindCommonPositions
		#endif
		scene->MeshesList()^[&](preMesh *ea){ oldVertices+=ea->Positions(); Process(ea,meshID++); newVertices+=ea->Positions(); };
		if(oldVertices!=newVertices)		
		post("JoinVerticesProcess finished | Verts in: ")<<oldVertices<<" out: "<<newVertices<<" | ~"<<(oldVertices-newVertices)/(double)oldVertices*100<<"%";			
		else post.Verbose("JoinVerticesProcess finished ");	
		scene->_non_verbose_formatflag = true;
		return true;
	}
};
static bool JoinVerticesProcess(Daedalus::post *post)
{
	return class JoinVerticesProcess(post);
}							
Daedalus::post::JoinIdenticalVertices::JoinIdenticalVertices
(post *p):Base(p,p->steps&step?::JoinVerticesProcess:0)
{}