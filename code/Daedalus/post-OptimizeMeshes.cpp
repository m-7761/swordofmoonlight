#include "Daedalus.(c).h"  
using namespace Daedalus;
class OptimizeMeshes //Assimp/OptimizeMeshes.cpp
{	
	Daedalus::post &post; 
	
	preScene *scene;
	enum{ Unset=INT_MAX };
	const size_t maxVerts, maxFaces; //treacherous?	
	
	MergeMeshesHelper MergeMeshes; //Assimp/SceneCombiner.h/cpp

	struct MeshInfo //Assimp/OptimizeMeshes.h
	{
		size_t ref_count, vertex_format, output_id;
		MeshInfo():ref_count(),vertex_format(),output_id(Unset){}		
	};
	std::vector<preMesh*> output;
	std::vector<MeshInfo> perMeshInformation;	
	template<class L> inline bool ShouldJoin
	(L &ml, size_t a, size_t b, size_t verts, size_t faces)const
	{
		if(perMeshInformation[a].vertex_format!=perMeshInformation[b].vertex_format)
		return false;		
		const preMesh *ma = ml[a], *mb = ml[b];
		if(Unset!=maxVerts&&maxVerts<verts+mb->Positions()||Unset!=maxFaces&&maxFaces<faces+mb->Faces()) 
		return false;
		//AI: Never merge unskinned meshes with skinned meshes
		if(ma->material!=mb->material||ma->HasBones()!=mb->HasBones())
		return false;
		//AI: Never merge meshes with different kinds of primitives 
		//if SortByPType did already do its work, it would override it
		if(post.steps&post.SortByPTypeProcess.step&&ma->_polytypesflags!=mb->_polytypesflags) 
		return false;
		//UNSURE WHAT IS BEING SAID HERE
		//AI: If both meshes are skinned, check whether we have many bones defined in both meshes.
		//If yes, can join them.
		if(ma->HasBones()) 
		{  	//Reminder: preventing MergeBones subroutine in post-MergeMeshesHelper.hpp
			//AI: TODO
			#ifdef NDEBUG
			#error TODO?
			#endif
			return false;
		}
		return true;
	}	
    std::vector<const preMesh*> mergeList;
	void RecursivelyProcess(preNode *node)
	{
		auto il = node->OwnMeshesList(); 
		auto ml = PreConst(scene)->MeshesCoList();
		for(size_t i=il;i<il.Size();i++) 
		{
			size_t m = il[i]; 
			if(perMeshInformation[m].ref_count<=1) 
			{	//AI: Find meshes to join with
				mergeList.clear(); 
				for(size_t j=i+1,verts=0,faces=0;j<il.Size();j++)
				{
					size_t m2 = il[j]; 
					if(1==perMeshInformation[m2].ref_count&&ShouldJoin(ml,m,m2,verts,faces))
					{	 
						mergeList.push_back(ml[m2]);
						verts+=mergeList.back()->Positions(); faces+=mergeList.back()->Faces();
						il.ForgoMemory(il.Size()-1);
						for(size_t k=j--;k<il.Size();k++) il[k] = il[k+1];
					}
				}//AI: merge all meshes which were found, replacing the old ones
				if(!mergeList.empty()) 
				{
					mergeList.push_back(ml[m]);
					output.push_back(MergeMeshes(mergeList.begin(),mergeList.end()));
				}
				else output.push_back(const_cast<preMesh*>(ml[m]));
				il[i] = output.size()-1;
			}
			else il[i] = perMeshInformation[m].output_id;
		}
		node->NodesList()^[=](preNode *ea){ RecursivelyProcess(ea); };
	}	

public: //OptimizeMeshes

	OptimizeMeshes(Daedalus::post *p)	
	//UNSURE ABOUT THE RATIONALE HERE
	:maxVerts(p->steps&p->SplitLargeMeshes_Vertex.step
	?p->SplitLargeMeshes_Vertex.configVertexLimit:Unset)
	,maxFaces(p->steps&p->SplitLargeMeshes_Triangle.step
	?p->SplitLargeMeshes_Triangle.configTriangleLimit:Unset)
	,post(*p),scene(p->Scene()){}operator bool()
	{	
		post.progLocalFileName("Optimizing Meshes");

		const size_t meshesPrior = scene->Meshes(); if(meshesPrior<=1) 
		{
			post.Verbose("Skipping OptimizeMeshesProcess. There are less than two meshes.");
			return true;
		}
		post.Verbose("OptimizeMeshesProcess begin");						   

		output.reserve(meshesPrior);
		mergeList.reserve(meshesPrior); 
		perMeshInformation.resize(meshesPrior);
		scene->rootnode->ForEach([&](const preNode *ea)
		{ ea->OwnMeshesList()^[&](preID ea){ perMeshInformation[ea].ref_count++; }; });
		//CAN THIS NOT BE IMPROVED?
		//AI: instanced meshes are immediately processed and added to the output list
		auto il = scene->MeshesCoList(); il^=[&](size_t i)
		{
			perMeshInformation[i].vertex_format = il[i]->GetMeshVFormatUnique();
			if(il[i]->Morphs() //NEW: not wanting to dig into morph meshes for time being
 			 ||1<perMeshInformation[i].ref_count&&Unset==perMeshInformation[i].output_id) 
			{ perMeshInformation[i].output_id = output.size(); output.push_back(il[i]);	}
		};
		RecursivelyProcess(scene->rootnode); 
		if(!il.ForgoMemory(output.size())) //paranoia?
		{
			il.Clear();	post.CriticalIssue("No meshes remaining; there is definitely something wrong");
			//return false; //IS THIS PROGRAMMER ERROR?
		}
		else il^=[&](size_t i){ il[i] = output[i]; };		
		if(il.RealSize()<meshesPrior) 
		post("OptimizeMeshesProcess finished. Input meshes: ")<<meshesPrior<<", Output meshes: "<<il.RealSize();
		else post.Verbose("OptimizeMeshesProcess finished");
		return true;
	}
};
static bool OptimizeMeshes(Daedalus::post *post)
{
	return class OptimizeMeshes(post);
}							
Daedalus::post::OptimizeMeshes::OptimizeMeshes
(post *p):Base(p,p->steps&step?::OptimizeMeshes:0)
{}