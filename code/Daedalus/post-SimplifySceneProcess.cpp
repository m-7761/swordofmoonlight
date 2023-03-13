#include "Daedalus.(c).h"
using namespace Daedalus;
class SimplifySceneProcess
{
	Daedalus::post &post;

	preScene *const scene;

	std::vector<preID> outMeshIDs;
	PreNew<preMesh*[]>::Vector outMeshes;

	enum{ NoEmit=preMost };
	
	struct Vertex 
	{
		Vertex() 
		{ 
			PreSuppose(N==sizeof(*this)/sizeof(signed));
			for(size_t i=0;i<N;i++) ((signed*)this)[i] = NoEmit; 
		}
		enum //shorthand
		{ 
		colorsN=PreMesh::colorslistsN, 
		coordsN=PreMesh::texturecoordslistsN, N=5+colorsN+coordsN
		}; 		
		signed material, position, normal;
		signed tangent, bitangent, colors[colorsN], coords[coordsN]; 				
	};
	struct PolyVertexInfo : Vertex 
	{			
		inline int VertexCompare(const Vertex *o)
		{ return memcmp(static_cast<Vertex*>(this),o,sizeof(Vertex)); } 		
		//used to fill in the 3D attribues
		signed emittedVertex, filledMorph;
		//subPositionIndex supporting members
		PolyVertexInfo *nextInPosition; size_t subPosition;
		PolyVertexInfo(size_t sub):Vertex(),
		emittedVertex(NoEmit),filledMorph(-2),subPosition(sub){}
	};
	std::vector<PolyVertexInfo> polyVertexInformation;	
	std::vector<PolyVertexInfo*> subPositionIndex;
	struct PerMaterialInfo //one mesh per material
	{
		//output mesh if any
		preMesh *simpleMesh; 
		//pre-calculated size of buffers
		preN simpleFaces, simpleIndices;
		//incremented as data is filled in
		preN emittedFaces, emittedIndices, emittedVertices;					
		//bones--can't forget about the bones
		PreList<preBone**>::Vector bonesList;
		PreList<PreBone::Weight*>::Vector boneWeights;
		PerMaterialInfo(){ memset(this,0x00,sizeof(*this)); }
	};
	std::vector<PerMaterialInfo> perMaterialInformation;  			
	void Process(/*const*/preMesh2 *mesh)
	{			
		if(mesh->IsSubmesh()) return;
		const bool doNormals = mesh->HasNormals2();
		const bool doTangents = mesh->HasTangents2();
		const bool doBitangents = mesh->HasBitangents2();
		const bool doColors = mesh->HasVertexColors2();
		const bool doCoords = mesh->HasTextureCoords2();
		const preN colorsN = mesh->CountVertexColorsLists2();
		const preN coordsN = mesh->CountTextureCoordsLists2();
		auto il = PreConst(mesh)->FacesList(); preN counter = 0;
		il^[&counter](const preFace2 &ea){ counter+=ea.polytypeN; };
		const signed sub = mesh->submesh?mesh->submesh->Positions():0;
		polyVertexInformation.assign(counter,PolyVertexInfo(sub));		
		subPositionIndex.assign(sub+mesh->Positions(),nullptr);
		perMaterialInformation.assign(scene->Materials(),PerMaterialInfo());		
		auto pvip = polyVertexInformation.data();
		auto pvid = pvip+polyVertexInformation.size();
		for(preN i=il,j,jN;i<il.Size();i++,pvip+=jN) 
		{	
			auto &face = il[i]; jN = face.polytypeN;
			auto &pmi =	perMaterialInformation[face.material];
			pmi.simpleFaces++; 
			if(face.start==face.polytype) continue;			
			pmi.simpleIndices+=jN; 			
			auto jl = mesh->IndicesSubList(face.PositionsRange());
			for(j=0;j<jl.Size();j++)
			{
				pvip[j].material = face.material;
				pvip[j].subPosition += pvip[j].position = jl[j]; 
			}if(doNormals)
			{
				jl = mesh->IndicesSubList(face.NormalsRange());
				if(1==jl.Size()) //face.normalflag?
				for(j=0;j<jN;j++) pvip[j].normal = jl[0];
				else for(j=0;j<jl.Size();j++) pvip[j].normal = jl[j];
			}if(doTangents)
			{
				jl = mesh->IndicesSubList(face.TangentsRange());
				if(1==jl.Size()) //face.normalflag? 
				for(j=0;j<jN;j++) pvip[j].tangent = jl[0];
				else for(j=0;j<jl.Size();j++) pvip[j].tangent = jl[j];
			}if(doBitangents)
			{
				jl = mesh->IndicesSubList(face.BitangentsRange());
				if(1==jl.Size()) //face.normalflag?
				for(j=0;j<jN;j++) pvip[j].bitangent = jl[0];
				else for(j=0;j<jl.Size();j++) pvip[j].bitangent = jl[j];
			}if(doColors) for(preN n=0;n<colorsN;n++)
			{
				jl = mesh->IndicesSubList(face.VertexColorsRange(n));
				if(1==jl.Size()) //face.colorflag?
				for(j=0;j<jN;j++) pvip[j].colors[n] = jl[0];
				else for(j=0;j<jl.Size();j++) pvip[j].colors[n] = jl[j];
			}if(doCoords) for(preN n=0;n<coordsN;n++)
			{
				jl = mesh->IndicesSubList(face.TextureCoordsRange(n));
				for(j=0;j<jl.Size();j++) pvip[j].coords[n] = jl[j];
			}						
			for(j=0;j<jN;j++) //fill in positional index 
			{
				auto pj = pvip+j;
				auto p = subPositionIndex[pj->subPosition]; if(p)
				{
					while(p&&pj->VertexCompare(p)) p = p->nextInPosition;
					if(p) continue; //duplication
				}pj->nextInPosition = subPositionIndex[pj->subPosition];
				subPositionIndex[pj->subPosition] = pj;
			}
		}pvip = polyVertexInformation.data();
		//in 3 stages allocate and fill the meshes & morphs
		for(size_t i=0;i<perMaterialInformation.size();i++)
		{	//note: could test 0==simpleIndices but that
			//would mean testing again in the faces loops
			auto &pmii = perMaterialInformation[i];			
			if(0==pmii.simpleFaces) continue;
			auto newMesh = pmii.simpleMesh = new preMesh;				
			newMesh->FacesList().Reserve(pmii.simpleFaces);
			newMesh->IndicesList().Reserve(pmii.simpleIndices);
			newMesh->name = mesh->name; newMesh->material = i; 			
			newMesh->MetaDataList().Copy(mesh->MetaDataList());
		}for(preN i=il,v=0;i<il.Size();i++) //fill in faces
		{			
			auto &pmi = perMaterialInformation[il[i].material];			
			preFace &newFace = 
			pmi.simpleMesh->FacesList()[pmi.emittedFaces] = il[i];						
			pmi.emittedFaces++;
			if(newFace.start==newFace.polytype) continue;
			else newFace.startindex = pmi.emittedIndices;			
			pmi.emittedIndices+=newFace.polytypeN;			
			pmi.simpleMesh->IndicesSubList(newFace)^[&](signed &ea)
			{	//ignore the data members if duplication
				PolyVertexInfo *ppi = subPositionIndex[pvip->subPosition];
				if(ppi!=pvip) //optimizing??
				while(ppi->VertexCompare(pvip)) ppi = ppi->nextInPosition; 											
				if(NoEmit==ppi->emittedVertex)
				ppi->emittedVertex = pmi.emittedVertices++;				
				//correcting: must broadcast duplicates if optimizing is to work!
				else polyVertexInformation[v].emittedVertex = ppi->emittedVertex;
				//optimizing: v++ affords direct access
				pvip++; ea = v++; //ppi->emittedVertex;	 				
			};
		}//lastly allocate & fill in the vertex/morphs data
		for(size_t i=0;i<perMaterialInformation.size();i++)
		{
			auto &pmii = perMaterialInformation[i];	
			preMesh *newMesh = pmii.simpleMesh;	if(!newMesh) continue;			
			const preMorph *morph = mesh;									
			const preMorph *submorph = mesh->submesh;			
			const signed mN = mesh->Morphs();
			const signed submorphN = submorph?mesh->submesh->Morphs():0;
			preMorph *newMorph = newMesh;
			auto ml = newMesh->MorphsList(); ml.Reserve(mN);			
			auto jl = newMesh->IndicesList();
			auto vf = polyVertexInformation[jl[0]]; //optimizing: v++ trick
			for(signed m=-1;m<mN;m++)
			{
				if(m!=-1) 
				{
					morph = mesh->MorphsList()[m]; 
					newMorph = ml[m] = new preMorph; 					
					newMorph->MetaDataList().Copy(morph->MetaDataList());					
					submorph = m<submorphN?mesh->submesh->MorphsList()[m]:0;
				}
				PreNew<pre3D[]> pl,nl,tl,bl;
				PreNew<pre4D[]> vcl[mesh->colorslistsN];
				PreNew<pre3D[]> tcl[mesh->texturecoordslistsN];
				if(morph->HasPositions2(submorph)) 
				pl.Reserve(pmii.emittedVertices);				
				if(NoEmit!=vf.normal&&morph->HasNormals2(submorph))
				nl.Reserve(pmii.emittedVertices);	
				if(NoEmit!=vf.tangent&&morph->HasTangents2(submorph))
				tl.Reserve(pmii.emittedVertices);
				if(NoEmit!=vf.bitangent&&morph->HasBitangents2(submorph))
				bl.Reserve(pmii.emittedVertices);				
				for(preN n=0;n<colorsN;n++)			
				if(NoEmit!=vf.colors[n]&&morph->HasVertexColors2(submorph,n))
				vcl[n].Reserve(pmii.emittedVertices);
				for(preN n=0;n<coordsN;n++)
				if(NoEmit!=vf.coords[n]&&morph->HasTextureCoords2(submorph,n))
				tcl[n].Reserve(pmii.emittedVertices);
				//optimizing: the indices do not yet contain the final values
				for(preN j=0;j<jl.Size();j++)
				{
					auto &pvi = polyVertexInformation[jl[j]];
					if(pvi.filledMorph==m) continue; 
					pvi.filledMorph = m; const signed v = pvi.emittedVertex;
					if(pl.Pointer()!=0) pl[v] = morph->Position2(submorph,pvi.position);
					if(nl.Pointer()!=0) nl[v] = morph->Normal2(submorph,pvi.normal);
					if(tl.Pointer()!=0) tl[v] = morph->Tangent2(submorph,pvi.tangent);
					if(bl.Pointer()!=0) bl[v] = morph->Bitangent2(submorph,pvi.bitangent);
					for(preN n=0;n<colorsN;n++)			
					if(vcl[n].Pointer()!=0) vcl[n][v] = morph->VertexColor2(submorph,n,pvi.colors[n]);
					for(preN n=0;n<coordsN;n++)			
					if(tcl[n].Pointer()!=0) tcl[n][v] = morph->TextureCoord2(submorph,n,pvi.coords[n]);					
				}
				newMorph->PositionsCoList().Have(std::move(pl));
				newMorph->NormalsCoList().Have(std::move(nl)); 				
				newMorph->TangentsCoList().Have(std::move(tl));
				newMorph->BitangentsCoList().Have(std::move(bl)); 
				for(preN n=0;n<colorsN;n++) newMorph->VertexColorsCoList(n).Have(std::move(vcl[n]));
				for(preN n=0;n<coordsN;n++) newMorph->TextureCoordsCoList(n).Have(std::move(tcl[n]));
			}//optimizing: this sleight-of-hand is explained in the block above
			jl^[&](signed &ea){ ea = polyVertexInformation[ea].emittedVertex; assert(ea!=preMost); };
			outMeshes.PushBack(newMesh); //done!			
		}for(int pass=1;pass<=2;pass++) //what! what about the bones?!
		{
			auto il = mesh->BonesList();
			if(pass==1) if(mesh->submesh) 
			il = mesh->submesh->BonesList();	
			else pass = 2;			
			for(preN i=il;i<il.Size();i++)
			{
				auto jl = il[i]->WeightsList();
				auto it = perMaterialInformation.begin();
				for(preN j=jl;j<jl.Size();j++)
				for(auto p=subPositionIndex[sub+jl[j]];p;p=p->nextInPosition)
				it[p->material].boneWeights.PushBack(jl[j]=p->emittedVertex);
				preBone *bone = il[i]; bone->WeightsList().Clear();
				for(;it<perMaterialInformation.end();it++) if(it->boneWeights.HasList())
				{
					preBone *newBone = new preBone(*bone,0);
					it->boneWeights.Sort();
					newBone->WeightsList().Have(std::move(it->boneWeights));
					it->bonesList.PushBack(newBone);
				}
			}//again! with the bones?!
		}auto it = perMaterialInformation.begin();
		for(;it<perMaterialInformation.end();it++) 
		{
			if(it->simpleMesh)		
			it->simpleMesh->BonesList().Have(std::move(it->bonesList)); 	
		}//outMeshes.PushBack(newMesh); //done!!			
	}	
	void RecursivelyRebuildMeshIDs(preNode *node)
	{
		auto il = node->OwnMeshesList(); if(il.HasList())
		{
			for(size_t i=il;i<il.Size();i++) 
			if(1<outMeshes[il[i]+1]-outMeshes[il[i]]) //optimizing
			{
				PreNew<size_t[]>::Vector v;	il^[&](preID ea)
				{ for(size_t i=outMeshIDs[ea];i<outMeshIDs[ea+1];v.PushBack(i++)); };
				il.Swap(std::move(v)); break; 
			}

		}node->NodesList()^[=](preNode *ea){ RecursivelyRebuildMeshIDs(ea); };
	}
	void MakeCopiesOfCommonMaterials() //preprocessing step
	{
		switch(scene->Materials()) //trivial?
		{
		case 0: assert(0); return; case 1: //default material?		
		if(scene->materialslist[0]->PairKey(PreMaterial::_default))	return;
		}//assign duplicate materials to vformats		
		struct Hash : std::pair<preID,int>
		{ 
			Hash(preID f, int s):pair(f,s){}
			Hash(const std::pair<preID,int> &cp):pair(cp){}
			inline operator const long long&()const{ return (long long&)*this; }	
		};PreSuppose(sizeof(Hash)==sizeof(long long));
		std::unordered_set<Hash,std::hash<long long>> hashes;		
		//SCHEDULED OBSOLETE: can remove if PreFace2 is out in the wild
		assert(offsetof(PreFace2,normals)==offsetof(PreFace2,colors)-1);
		assert(offsetof(PreFace2,colors)==offsetof(PreFace2,texturecoords)-1);		
		auto &vformat = [](const preFace2 &f)->int{ return (int&)f.normals; };		
		Hash cache(preMost,0);
		PreConst(scene)->MeshesList2()^[&](const preMesh2 *ea)
		{
			ea->FacesList()^[&](const preFace2 &ea2)
			{ 
				auto made = std::make_pair(ea2.material,vformat(ea2));
				if(cache!=made) hashes.insert(cache=made); 
			};		
		};//sort: un unordered_set is unordered
		std::vector<Hash> groups(hashes.begin(),hashes.end());
		std::sort(groups.begin(),groups.end());
		size_t trivial = true;
		for(size_t i=1;i<groups.size();i++)
		if(groups[i].first==groups[i-1].first){ trivial = false; break; }			
		if(trivial) return;
		auto it = groups.begin();
		cache.first = preMost; preID hit = 0;
		scene->MeshesList2()^[&](preMesh2 *ea)
		{
			ea->FacesList()^[&](preFace2 &ea2)
			{
				auto made = std::make_pair(ea2.material,vformat(ea2));
				if(cache!=made) hit = std::lower_bound(it,groups.end(),made)-it;
				ea2.material = hit;
			}; 
		};
		//fill in the new materials list
		auto il = scene->MaterialsList();
		//add any unused materials to the back of the list
		for(preN i=il,j=0;i<il.Size()&&j<groups.size();j++) 		
		while(i++<groups[j].first) groups.push_back(Hash(i-1,0));
		PreNew<preMaterial*[]> jl;
		jl.Reserve(groups.size());
		jl[0] = il[groups[0].first];		
		for(preN j=1;j<jl.Size();j++)
		{
			jl[j] = il[groups[j].first]; 
			if(jl[j]!=jl[j-1]) continue;
			jl[j] = new preMaterial(*jl[j],0);			
			post("Copying material so that each vertex-format has a copy: originally #")<<groups[j].first;
		}for(preN j=0;j<jl.Size();j++)			
		jl[j]->InsertProperty(groups[j].second,PreMaterial::_vformat2);		
		//0 to prevent deletion of the originals
		for(preN i=0;i<il.Size();i++) il[i] = 0; il.Swap(std::move(jl));							   		
	}

public: //SimplifySceneProcess

	SimplifySceneProcess(Daedalus::post *p)
	:scene(post.complexSceneOut)
	,post(*p){}operator bool()
	{			
		if(!scene) return true;

		post.progLocalFileName("Simplifying Scene");

		post.Verbose("SimplifyProcess begin");
		
		MakeCopiesOfCommonMaterials(); //preprocessing step

		const preN meshesPrior = scene->Meshes();

		PreNew<preMesh2*[]> il(std::move(scene->MeshesList2()));
		outMeshIDs.assign(il.RealSize()+1,0);					
		for(preN i=il;i<il.Size();outMeshIDs[++i]=outMeshes.Size()) Process(il[i]);		
		if(scene->rootnode)	RecursivelyRebuildMeshIDs(scene->rootnode);
		scene->MeshesList().Have(std::move(outMeshes));
		
		post.complexSceneOut = 0; post.simpleSceneOut = scene;		
		post("SimplifyProcess finished. Total meshes in was ")<<meshesPrior<<". Total out is "<<scene->Meshes();
		return true;
	}	
};
static bool SimplifySceneProcess(Daedalus::post *post)
{
	return class SimplifySceneProcess(post);  
}							
Daedalus::post::SimplifyScene::SimplifyScene
(post *p):Base(p,::SimplifySceneProcess)
{}
Daedalus::post::ComplicateScene::ComplicateScene
(post *p):Base(p,0) //reserved for future use
{}

