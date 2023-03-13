#include "Daedalus.(c).h"  
using namespace Daedalus;
//Assimp/SplitLargeMeshes.cpp
 //Assimp/SplitByBoneCountProcess.cpp
class SplitLargeMeshes 
{
	Daedalus::post &post;	
	
	const unsigned long long maximum;
	
	std::vector<preID> outMeshIDs;
	PreNew<preMesh*[]>::Vector outMeshes;

	MakeSubmeshHelper MakeSubmesh; //Assimp/ProcessHelper.h/cpp	

	bool trying; //hack
	std::vector<bool> subVerts;
	std::vector<preN> subFaces;
	bool SplitByVertex(const preMesh *mesh)
	{
		if(mesh->Positions()<=maximum) return false;
		if(trying) return true;
		if(CannotProceed(mesh)) return false;

		bool out = false;
		auto il = mesh->FacesList();
		subFaces.reserve(il.RealSize());	
		preN startFace = 0, numFacesHandled = 0;		
		for(preN counter=0;numFacesHandled<il.Size();counter=0)
		{	
			subFaces.clear();		
			if(startFace>0) //NEW
			subFaces.push_back(startFace-1);
			subVerts.assign(mesh->Positions(),false);
			for(size_t i=numFacesHandled;i<il.Size();i++) if(!il[i]) 
			{	
				subFaces.push_back(i); 
				//HACK: pull start faces out of the face-loop condition
				if(startFace<=i){ startFace = i+1; numFacesHandled++; }
			}
			else //AI: try to add to MakeSubmesh
			{	
				mesh->IndicesSubList(il[i])^[&](signed ea)
				{ 	
					if(!subVerts[ea]){ subVerts[ea] = true; counter++; }
				};
				if(counter<=maximum)
				{
					subFaces.push_back(i); numFacesHandled++;
				}
				else if(counter==il[i].polytypeN)
				{
					post.CriticalIssue("Overflow: the maximum is less than the vertices of this polygon: ")<<counter<<"!";					
					subFaces.push_back(i); numFacesHandled++; break;
				}
				else break;
			}
			preMesh *made = MakeSubmesh(mesh,subFaces,0);
			if(made){ out = true; outMeshes.PushBack(made); }
		}return out;
	}		 
	bool SplitByIndex(const preMesh *mesh)
	{
		auto il = mesh->FacesList();
		unsigned long long counter = 0;
		il^[&](const PreFace::Range &ea){ counter+=ea.size; };
		if(counter<=maximum) return false;
		if(trying) return true;
		if(CannotProceed(mesh)) return false;
		
		//this is done differently from Assimp's original way
		//which really worked on faces, and not triangles at all

		bool out = false;
		subFaces.reserve(il.RealSize());	
		preN startFace = 0, numFacesHandled = 0;		
		for(counter=0;numFacesHandled<il.Size();counter=0)
		{	
			subFaces.clear();		
			if(startFace>0) //NEW
			subFaces.push_back(startFace-1);
			for(preN i=numFacesHandled;i<il.Size();i++) if(!il[i]) 
			{	
				subFaces.push_back(i); 
				//HACK: pull start faces out of the face-loop condition
				if(startFace<=i){ startFace = i+1; numFacesHandled++; }
			}
			else //AI: try to add to MakeSubmesh
			{	
				counter+=il[i].polytypeN; if(counter<=maximum)
				{
					subFaces.push_back(i); numFacesHandled++;
				}
				else if(counter==il[i].polytypeN)
				{
					post.CriticalIssue("Overflow: the maximum is less than the indices of this polygon: ")<<counter<<"!";					
					subFaces.push_back(i); numFacesHandled++; break;
				}
				else break;
			}			
			preMesh *made = MakeSubmesh(mesh,subFaces,0);
			if(made){ out = true; outMeshes.PushBack(made); }
		}return out;
	}
	std::vector<preBone*> superMeshBones;
	std::vector<std::vector<bool>> boneVerts;
	std::unordered_set<size_t> candidateBones;
	std::vector<bool> isFaceHandled, isBoneUsed;
	bool SplitByBone(const preMesh *mesh, PreList<preBone**&>::Vector &bonesMask)
	{
		if(bonesMask.Size()<=maximum) return false; 				
		if(trying) return true;
		if(CannotProceed(mesh)) return false;
		
		//NEW: first build a per bone vertex inclusion map
		boneVerts.resize(std::max<size_t>(boneVerts.size(),bonesMask.Size()));
		for(size_t i=0;i<bonesMask.Size();i++) 
		{
			auto &boolvec = boneVerts[i];
			boolvec.assign(mesh->Positions(),false);
			bonesMask[i]->WeightsList()^[&](signed ea){ boolvec[ea] = true; };
		}		

		bool out = false; 
		preN startFace = 0; //NEW
		preN numFacesHandled = 0;
		auto il = mesh->FacesList();
		subFaces.reserve(il.Size());
		isFaceHandled.assign(il.Size(),false);		
		superMeshBones.assign(bonesMask.Begin(),bonesMask.End()); 		
		while(bonesMask.ForgoMemory(0),subFaces.clear(),numFacesHandled<il.Size());		
		{	//REMINDER: boneMask cannot have double entries
			isBoneUsed.assign(superMeshBones.size(),false);							
			//AI: a small local array of new bones for the current face. State of all used bones for that face
			//can only be updated AFTER the face is completely analysed. Thanks to imre for the fix.
			candidateBones.clear();
			//AI: add faces to the new submesh as long as all bones affecting the faces' vertices fit in the limit
			for(preN i=0;i<il.Size();i++) if(!il[i]) //NEW
			{	
				subFaces.push_back(i); 
				//HACK: pull start faces out of the face-loop condition
				if(startFace<=i){ startFace = i+1; numFacesHandled++; }
			}
			else if(!isFaceHandled[i]) //AI: try to add to MakeSubmesh
			{	
				auto l = mesh->IndicesSubList(il[i]);				
				for(size_t j=0;j<superMeshBones.size();j++) if(!isBoneUsed[j])
				{	
					auto &boolvec = boneVerts[j]; 					
					l^[&](signed ea){ if(boolvec[ea]) candidateBones.insert(j); };
				}
				if(bonesMask.Size()+candidateBones.size()>maximum)
				{
					if(bonesMask.Size()) continue; //NEW: infinite loop
					post.CriticalIssue("Data Loss: A face belongs to more than the specified number of bones!");					
				}//AI: mark all new bones as necessary
				for(auto it=candidateBones.begin();it!=candidateBones.end();it++)
				{
					isBoneUsed[*it] = true;	bonesMask.PushBack(superMeshBones[*it]);					
				}
				subFaces.push_back(i); 			
				isFaceHandled[i] = true; numFacesHandled++;
			}//NOTICE: bonesMask IS the current boneslist
			preMesh *made = MakeSubmesh(mesh,subFaces,0);
			if(made){ out = true; outMeshes.PushBack(made); }
		};//hack: restoring the bones is nicer than deleting them here
		bonesMask.Assign(superMeshBones.size(),superMeshBones.data());
		return out;
	}	
	bool CannotProceed(const preMesh *mesh)
	{
		if(mesh->HasFaces()&&mesh->HasIndices()) return false;
		else post.CriticalIssue("Splitting meshes without faces/indices has not been contemplated");
		return true;
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
	bool SplitByBone(preMesh *m){ return SplitByBone(m,m->BonesList().AsVector()); }
	bool SplitByVertex(preMesh *mesh){ return SplitByVertex(PreConst(mesh)); }
	bool SplitByIndex(preMesh *mesh){ return SplitByIndex(PreConst(mesh)); }
	const struct Mode
	{ unsigned long long max; bool (SplitLargeMeshes::*split)(preMesh*);
	}mode;

public: //SplitLargeMeshes
	
	SplitLargeMeshes(Daedalus::post *p, const Mode &mode)	
	:mode(mode),maximum(mode.max),trying(true)
	,post(*p){}operator bool()
	{ 	
		const char *big = "Bone", *small = " bones";
		if(mode.split!=&SplitLargeMeshes::SplitByBone)
		{ big = mode.split==&SplitLargeMeshes::SplitByVertex?"Vertex":"Triangle";
		small = mode.split==&SplitLargeMeshes::SplitByVertex?" vertices":" triangles"; }

		post.Verbose("SplitMeshesBy")<<big<<"Process begin";

		preScene *scene = post.Scene();
		auto il = scene->MeshesCoList();		
		size_t i; for(i=il;i<il.Size()&&!(this->*mode.split)(il[i]);i++);
		if(i==il.Size())
		{
			post.Verbose("All-clear: no meshes with more than ")<<maximum<<small;	
			return true;
		}		
		else trying = false; //hack
		outMeshIDs.assign(il.Size()+1,0);			
		for(i=0;i<il.Size();outMeshIDs[++i]=outMeshes.Size()) if(!(this->*mode.split)(il[i]))
		{				
			outMeshes.PushBack(il[i]); il[i] = 0; //prevent deletion
		}
		il.Swap(std::move(outMeshes)); RecursivelyRebuildMeshIDs(scene->rootnode);
				
		post("SplitMeshesBy")<<big<<"Process finished. Meshes have been split";
		return true;
	}
	static bool Bone(Daedalus::post *p)
	{
		p->progLocalFileName("Splitting Meshes to Required Number of \"Bones\"");
		Mode mode = { p->SplitLargeMeshes_Bone.configMaxBoneCount,&SplitByBone };
		return class SplitLargeMeshes(p,mode);
	}
	static bool Vertex(Daedalus::post *p)
	{
		p->progLocalFileName("Splitting Large Meshes By Vertex (or Vertex Buffer)");
		Mode mode = { p->SplitLargeMeshes_Vertex.configVertexLimit,&SplitByVertex };
		return class SplitLargeMeshes(p,mode);
	}
	static bool Triangle(Daedalus::post *p)
	{
		p->progLocalFileName("Splitting Large Meshes By Triangle (or Index Trios)");
		Mode mode = { 3ULL*p->SplitLargeMeshes_Triangle.configTriangleLimit,&SplitByIndex };
		return class SplitLargeMeshes(p,mode);
	}
};
static bool SplitLargeMeshes_Bone(Daedalus::post *post)
{
	return SplitLargeMeshes::Bone(post);
}							
Daedalus::post::SplitByBoneCount::SplitByBoneCount
(post *p):Base(p,p->steps&step?::SplitLargeMeshes_Bone:0)
{
	configMaxBoneCount = 60; //todo? seems high
}
static bool SplitLargeMeshes_Vertex(Daedalus::post *post)
{
	return SplitLargeMeshes::Vertex(post);
}				
Daedalus::post::SplitLargeMeshes_Vertex::SplitLargeMeshes_Vertex(post *p)
:Daedalus::post::Base(p,p->steps&step?::SplitLargeMeshes_Vertex:0)
{
	configVertexLimit = 1000000;
}
static bool SplitLargeMeshes_Triangle(Daedalus::post *post)
{
	return SplitLargeMeshes::Triangle(post);
}							
Daedalus::post::SplitLargeMeshes_Triangle::SplitLargeMeshes_Triangle
(post *p):Base(p,p->steps&step?::SplitLargeMeshes_Triangle:0)
{
	configTriangleLimit = 1000000;
}