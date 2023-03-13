#include "Daedalus.(c).h"
using namespace Daedalus;
class DeboneProcess //Assimp/DeboneProcess.cpp
{
	Daedalus::post &post;

	MakeSubmeshHelper MakeSubmesh; //Assimp/ProcessHelper.h/cpp

	size_t numBones, numBonesCanDoWithout;		
    std::vector<std::vector<std::pair<preID,preNode*>>> newMeshIDs;
	
	//Subroutine used by both passes
	std::vector<size_t> vertexBones;
	std::vector<bool> isBoneNecessary;	
	enum{ Coowned = preMost, Unowned=-1 }; //C4340
	bool InterstitialRequired(int pass, const preMesh *mesh)
	{
		bool out = false;
		auto il = mesh->BonesList();
		isBoneNecessary.assign(il.RealSize(),false);
		vertexBones.assign(mesh->Positions(),Unowned);										  
		for(size_t i=il;i<il.Size();i++) 
		{
			PreBone::Weight::typeofvalue w;
			auto jl = il[i]->WeightsList();
			for(size_t j=jl;j<jl.Size();j++)
			{
				w = jl[j].value; if(!w) continue;
				if(w>=post.DeboneProcess.configThreshold)   
				{
					signed vid = jl[j]; 
					if(vertexBones[vid]!=Unowned)  
					{
						if(vertexBones[vid]==i)
						if(pass!=1){/*don't double log*/}else
						post("Encountered double entry in bone weights");
						//AI: TODO: track attraction in order to break tie
						else vertexBones[vid] = Coowned;
					}
					else vertexBones[vid] = i;
				}
				if(!isBoneNecessary[i]) 
				isBoneNecessary[i] = w<post.DeboneProcess.configThreshold;
			}
			if(!isBoneNecessary[i]) out = true;
		}
		return out;
	}
	//First pass: gather information
	bool Consider(const preMesh *mesh)
	{
		if(!mesh->HasBones()) return false;

		//NEW: perform common subroutine
		//AI: interstitial faces not allowed
		bool isInterstitialRequired = InterstitialRequired(1,mesh);
		
		if(isInterstitialRequired) 		
		{
			auto il = mesh->FacesList(); 
			for(size_t i=il;i<il.Size();i++)  
			{
				auto jl = mesh->IndicesSubList(il[i]);
				if(jl.HasList()) //IMPORTANT
				for(preID v=vertexBones[jl[0]],j=1;j<jl.Size();j++) 
				{
					preID w = vertexBones[jl[j]];	if(v!=w)    
					{
						if(v<preMost) isBoneNecessary[v] = true;
						if(w<preMost) isBoneNecessary[w] = true;
					}
				}
			}		
		}	
		bool split = false;
		for(size_t i=0;i<mesh->bones;i++,numBones++)   
		if(!isBoneNecessary[i]){ numBonesCanDoWithout++; split = true; }
		return split;
	}			   
	//Second pass: perform operation	
	//AI: same deal here as ConsiderMesh more or less
	std::vector<preID> subFaces;	
	std::vector<size_t> facesPerBone, faceBones; 
	std::vector<std::pair<preMesh*,const preBone*>> splitMeshes;
	bool Split(const preMesh *mesh)
	{
		splitMeshes.clear(); //previously was second argument		

		//NEW: perform common subroutine
		//(fill vertexBones & isBoneNecessary)
		InterstitialRequired(2,mesh); 		

		facesPerBone.assign(mesh->Bones(),0);	
		size_t numUnowned = 0;		
		faceBones.assign(mesh->Faces(),Unowned);		
		
		auto il = mesh->FacesList(); 
		for(size_t i=il;i<il.Size();i++) if(il[i])
		{
			size_t v,j,nInterstitial = 1;
			auto jl = mesh->IndicesSubList(il[i]);
			if(jl.HasList()) //IMPORTANT
			for(v=vertexBones[jl[0]],j=1;j<jl.Size();j++) 
			{
				size_t w = vertexBones[jl[j]];	if(v!=w)    
				{
					if(v<mesh->bones) isBoneNecessary[v] = true;
					if(w<mesh->bones) isBoneNecessary[w] = true;
				}
				else nInterstitial++;
			}			
			//is this second condition really necessary?
			if(nInterstitial==jl.Size()&&v<facesPerBone.size())   
			{
				//AI: primitive belongs to bone #v
				faceBones[i] = v; facesPerBone[v]++;
			}
			else //numUnowned++;
			{
				numUnowned++; assert(nInterstitial!=jl.Size());
			}
		}
		//AI: invalidate conjoined faces
		for(size_t i=il;i<il.Size();i++)		
		if(faceBones[i]<facesPerBone.size()&&isBoneNecessary[faceBones[i]])
		{
			assert(facesPerBone[faceBones[i]]>0);				
			numUnowned++; faceBones[i] = Unowned; facesPerBone[faceBones[i]]--;			
		}
		//MakeSubmesh
		if(numUnowned)
		{
			subFaces.clear(); il^=[&](preN i)
			{ if(faceBones[i]==Unowned||!il[i]) subFaces.push_back(i); };
			preMesh *subMesh = MakeSubmesh(mesh,subFaces,0);
			splitMeshes.push_back(std::make_pair(subMesh,nullptr));
		}	 
		for(size_t i=0;i<facesPerBone.size();i++) 
		if(!isBoneNecessary[i]&&facesPerBone[i]>0)  
		{
			subFaces.clear(); il^=[&](preN ii)
			{ if(i==faceBones[ii]||!il[i]) subFaces.push_back(ii); };
			preMesh *subMesh = MakeSubmesh(mesh,subFaces,MakeSubmesh_SansBones);
			subMesh->ApplyTransform(mesh->boneslist[i]->matrix);
			splitMeshes.push_back(std::make_pair(subMesh,mesh->boneslist[i]));
		}
		assert(!splitMeshes.empty()); //checking
		return !splitMeshes.empty(); //courtesy
	}	
	std::vector<preID> newMeshList;
	void RecursivelyRebuildMeshIDs(preNode *node)
	{			
		newMeshList.clear();
		//AI: this will require two passes
		//AI: first pass, look for meshes which have not moved
		node->OwnMeshesList()^[&](size_t srcIndex)
		{
			const auto &subMeshes = newMeshIDs[srcIndex];
			for(size_t i=0;i<subMeshes.size();i++) 
			if(!subMeshes[i].second) newMeshList.push_back(subMeshes[i].first);
		};
		//AI: second pass, collect deboned meshes
		for(size_t i=0;i<newMeshIDs.size();i++)
		{
			const auto &subMeshes = newMeshIDs[i];
			for(size_t i=0;i<subMeshes.size();i++) 
			if(node==subMeshes[i].second) newMeshList.push_back(subMeshes[i].first);
		}
		node->OwnMeshesList().Assign(newMeshList.size(),newMeshList.data());
		node->NodesList()^[=](preNode *ea){ RecursivelyRebuildMeshIDs(ea); };
	}

public: //DeboneProcess

	DeboneProcess(Daedalus::post *p)
	:post(*p),numBones(),numBonesCanDoWithout()
	{}operator bool()
	{
		post.progLocalFileName("Deboning");

		preScene *scene = post.Scene();

		if(!scene->Meshes()) //good enough?
		{
			post.Verbose("DeboneProcess skipped; there are no meshes, and so no bones");
			return true;
		} 
		else post.Verbose("DeboneProcess begin");

		auto il = scene->MeshesCoList();
		std::vector<bool> splitList(scene->meshes);
		il^=[&](size_t i){ splitList[i] = Consider(il[i]); };

		size_t numSplits = 0;	
		if(numBonesCanDoWithout)
		if(!post.DeboneProcess.configAllOrNone||numBones==numBonesCanDoWithout)
		il^=[&](size_t i){ if(splitList[i]) numSplits++; };
		if(numSplits)
		{				
			std::vector<preMesh*> assign;
			newMeshIDs.resize(il.Size());
			for(size_t i=il;i<il.Size();i++)
			{
				preMesh *srcMesh = il[i];
				if(!splitList[i]||!Split(srcMesh))
				{
					il[i] = 0; assign.push_back(srcMesh);
					newMeshIDs[i].push_back(std::make_pair(assign.size()-1,nullptr));					
				}
				else //splitMeshes HOLDS RESULT OF Split
				{
					size_t out = 0, in = srcMesh->bones;
					//AI: store new meshes and indices of the new meshes
					for(size_t j=0;j<splitMeshes.size();j++)    
					{
						const preX *find = 
						splitMeshes[j].second?&splitMeshes[j].second->node:0;
						preNode *theNode = find?scene->rootnode->FindNode(*find):0;
						newMeshIDs[i].push_back(std::make_pair(assign.size(),theNode));
						assign.push_back(splitMeshes[j].first);
						out+=splitMeshes[j].first->bones;
					}				
					post("Removed ")<<in-out<<"bones. Input bones: "<<in<<". Output bones: "<<out;										
				}
			}
			//AI: reassign, destroying the source meshes
			//that should be contained inside the new submeshes
			scene->MeshesList().Assign(assign.size(),assign.data());			
			RecursivelyRebuildMeshIDs(scene->rootnode);
		}

		post.Verbose("DeboneProcess end");
		return true;
	}
};		
static bool DeboneProcess(Daedalus::post *post)
{
	return class DeboneProcess(post);
}							
Daedalus::post::Debone::Debone
(post *p):Base(p,p->steps&step?::DeboneProcess:0)
{
	configAllOrNone = false; configThreshold = 1; 
}