#include "Daedalus.(c).h"  
using namespace Daedalus;
class SortByPTypeProcess //Assimp/SortByPTypeProcess.cpp
{	
	Daedalus::post &post;

	MakeSubmeshHelper MakeSubmesh; //Assimp/ProcessHelper.h/cpp

	preScene *const scene;
	enum{ Missing=INT_MAX };
	std::vector<preID> newMeshIDs; preID *const tmpMeshIDs;
	void RecursivelyRebuildMeshIDs(preNode *node)
	{
		auto il = node->OwnMeshesList(); if(il.HasList())
		{	//ugh: makeshift swapbuffer
			size_t tmpSize = il.Size();
			assert(&newMeshIDs.back()+1>=tmpMeshIDs+tmpSize);
			for(size_t i=0;i<tmpSize;i++) tmpMeshIDs[i] = il[i];			
			auto iv = il.AsVector(); iv.Clear();
			for(size_t i=0;i<tmpSize;i++)
			for(preID add=tmpMeshIDs[i]*4,j=0;j<4;j++)			
			if(Missing!=newMeshIDs[add+j]) 
			iv.PushBack(newMeshIDs[add+j]); if(!il.Size()) il.Clear();
		}
		node->NodesList()^[=](preNode *ea){ RecursivelyRebuildMeshIDs(ea); };
	}		   	
	const bool (&removeIf)[3]; //VS2010

public: //SortByPTypeProcess
		
	SortByPTypeProcess(Daedalus::post *p)
	//5-4=1: using back of newMeshIDs for temporary storage
	:scene(p->Scene()),newMeshIDs(scene->Meshes()*5,Missing)
	,tmpMeshIDs(newMeshIDs.data()+scene->Meshes()*4)
	,removeIf(p->SortByPTypeProcess.configRemoveIfPolytope)
	,post(*p){}operator bool()
	{
		post.progLocalFileName("Splitting Meshes into Points, Lines, Triangles & Polygons");
		
		if(!scene->Meshes())
		{
			post.Verbose("SortByPTypeProcess skipped, there are no meshes");
			return true;
		}
		else post.Verbose("SortByPTypeProcess begin");
		
		size_t log[4] = {0,0,0,0};
		std::vector<preN> subFaces;
		auto it = newMeshIDs.begin();
		auto il = scene->MeshesCoList();		
		std::vector<preMesh*> outMeshes;		
		outMeshes.reserve(4*il.RealSize());												
		for(size_t i=il;i<il.Size();i++) 
		{
			preMesh *mesh = il[i]; //Process?
			
			preSize counter = 0; 
			bool x[4] = {false,false,false,false};
			auto &f = [&](int i, int j)
			{ counter++; x[i] = removeIf[j]; log[i]++; };
			if(mesh->_pointsflag) f(0,0);
			if(mesh->_linesflag) f(1,1);
			if(mesh->_trianglesflag) f(2,2);
			if(mesh->_polygonsflag) f(3,2); 
			if(counter<=1) //trivial case
			{
				if(!x[0]&&!x[1]&&!x[2]&&!x[3])
				{ 
					il[i] = 0; //preventing deletion
					*it = outMeshes.size(); outMeshes.push_back(mesh); 
				}it+=4;
			}
			else for(size_t j=0;j<4;j++,it++) if(!x[j])
			{
				if(!subFaces.capacity()) //optimizing?
				{					
					counter = 0; il^[&](preMesh *ea)
					{ counter = std::max(counter,ea->Faces()); };
					subFaces.reserve(counter);
				}
				subFaces.clear();
				auto l = mesh->FacesList();				
				auto p = PreFace::Polytype(j+1);
				signed k = -1, n = l.RealSize(); switch(j)
				{
				case 0:	case 1:	case 2:	
				while(++k<n) if(!l[k]||l[k].polytype==p) subFaces.push_back(k);
				break; default:
				while(++k<n) if(!l[k]||l[k].polytypeN>3) subFaces.push_back(k);
				}
				preMesh *made = MakeSubmesh(mesh,subFaces,0);
				if(made) *it = outMeshes.size();
				if(made) outMeshes.push_back(made);
			}
		}//AI: repair mesh instances if necessary
		for(size_t i=il;i<il.Size();i++) if(il[i])
		{ RecursivelyRebuildMeshIDs(scene->rootnode); break; }
		il.Assign(outMeshes.size(),outMeshes.data());

		post("Points: ")  <<log[0]<<"X"+!removeIf[0]
		<< ", Lines: "    <<log[1]<<"X"+!removeIf[1]
		<< ", Triangles: "<<log[2]<<"X"+!removeIf[2]
		<< ", Polygons: " <<log[3]<<"X"+!removeIf[2]<<" (Meshes, X = removed)"; 		
		post.Verbose("SortByPTypeProcess finished");
		return true;
	}
};
static bool SortByPTypeProcess(Daedalus::post *post)
{
	return class SortByPTypeProcess(post);
}							
Daedalus::post::SortByPType::SortByPType
(post *p):Base(p,p->steps&step?::SortByPTypeProcess:0)
{
	for(int i=0;i<3;i++) configRemoveIfPolytope[i] = false;
}