#include "Daedalus.(c).h"  
using namespace Daedalus;
class RemoveRedundantMaterials //Assimp/RemoveRedundantMaterials.cpp
{	
	Daedalus::post &post; 

	//Assuming properties have been presorted by ScenePreprocessor.cpp
	static int CompareProperties(const PreMaterial *a, const PreMaterial *b)
	{
		auto al = a->PropertiesList(), bl = b->PropertiesList();
		auto it = al.Begin(), itt = al.End(), jt = bl.Begin(), jtt = bl.End();
		for(int cmp;it<itt&&jt<jtt;it++,jt++)
		{
			auto p = *it, q = *jt;					
			if(cmp=q->texturepair-p->texturepair) goto meta;
			if(cmp=ptrdiff_t(q->key)-ptrdiff_t(p->key)) goto meta;			
			if(cmp=p->MemoryCompare(q)) goto meta;			
			continue; meta: //try to recover if either or meta
			bool pMeta = p->key().meta, qMeta = q->key().meta;
			if(pMeta||qMeta){ if(!pMeta) it--; if(!qMeta) jt--; continue; }	
			return cmp;	//unequal
		}
		while(jt<jtt) if(!(*jt++)->key().meta) return -1; return 0;
	} 
	struct Less : std::pair<const PreMaterial*,size_t>
	{
		Less(const preMaterial *p, size_t i):pair(p,i){}
		inline bool operator<(const Less &cmp)const{ return CompareProperties(first,cmp.first)<0; }		
	};

public: //RemoveRedundantMaterials
		
	RemoveRedundantMaterials(Daedalus::post *p)	
	:post(*p){}operator bool()
	{
		post.progLocalFileName("Removing Identical Materials");

		preScene *scene = post.Scene();
		auto il = scene->MaterialsList(); if(il.RealSize()<=1)
		{
			post.Verbose("Skipping RemoveRedundantMaterialsProcess. Single material");
			return true;
		}
		else post.Verbose("RemoveRedundantMaterialsProcess begin");
		assert(il[0]->IsSorted()); //ScenePreprocessor.cpp		
		
		struct PerMaterial
		{
			size_t mapping, references; bool remove;
			PerMaterial():mapping(),references(),remove(){} 
		};				
		size_t redundantRemoved = 0, unusedRemoved = 0;						
		std::vector<PerMaterial> materials(il.RealSize());
		if(!post.RemoveRedundantMaterials.configKeepUnused)
		{	//AI: Find out which materials are used by meshes
			PreSuppose(!PreMesh2::IsSupported);			
			PreConst(scene)->MeshesList()^[&](const preMesh *ea)
			{ materials[ea->material].references++; };			
			//AI: If a list of materials to be excluded was given, match it
			if(post.RemoveRedundantMaterials.configOffLimits.HasList()) 		
			for(size_t i=0;i<materials.size();i++) 
			{
				preX name; il[i]->Get(PreMaterial::name,name);
				if(name.HasString()) 					
				if(post.RemoveRedundantMaterials.configOffLimits.PairKey(name.cstring))
				{
					#ifdef NDEBUG //InsertProperty(&dummy,1,"~RRM.UniqueMaterial",0,0);
					#error seems Assimp wants to keep the name itself, but there's a bug here
					#endif
					materials[i].references++; //AI: Keep even if no mesh references it
					post.Verbose("Found positive match in exclusion list: \'")<<name.cstring<<"\'";
				}
			}//AI: If no mesh is referencing this material, remove it.
			for(size_t i=0;i<materials.size();i++) if(!materials[i].references)
			{ unusedRemoved++; materials[i].remove = true; }		
		}
		std::vector<Less> less; //comparing
		less.reserve(materials.size()-unusedRemoved);
		for(size_t i=0;i<materials.size();i++) 
		if(!materials[i].remove) less.push_back(Less(il[i],i));			
		//sort & remove duplicate materials
		std::sort(less.begin(),less.end());		
		if(!less.empty()) materials[less[0].second].mapping = 0;
		for(size_t i=1;i<less.size();i++) if(!(less[i-1].first<less[i].first)) 
		{
			materials[less[i].second].remove = true; redundantRemoved++;
			materials[less[i].second].mapping = materials[less[i-1].second].mapping;
		}
		else materials[less[i].second].mapping = i;						
		//AI: If the new material count differs from the original,
		//rebuild the material list and remap the mesh material indexes.
		size_t newNum = less.size()-redundantRemoved;
		if(newNum!=materials.size()) 
		{
			const PreNew<preMaterial*[]> oldMaterials(std::move(il));
			il.Assign(newNum,nullptr); //paranoia
			//NOTE: Assimp was renaming the modified materials to JoinedMaterial_#X
			//HOWEVER THIS SEEMS LIKE A LOT OF WORK JUST TO ERASE A MEANINGFUL NAME
			for(size_t i=0;i<materials.size();i++) if(!materials[i].remove) 
			{ il[materials[i].mapping] = oldMaterials[i]; oldMaterials[i] = 0; };
			for(size_t i=0;i<materials.size();i++) assert(il[i]); //paranoia
			//AI: update all material indices
			scene->MeshesList()^[&](preMesh *ea)
			{ ea->material = materials[ea->material].mapping; };
			post("RemoveRedundantMaterialsProcess finished. Removed ")<<redundantRemoved<<" redundant and "<<unusedRemoved<<" unused materials.";
		}				
		else post.Verbose("RemoveRedundantMaterialsProcess finished.");		
		return true;
	}
};
static bool RemoveRedundantMaterials(Daedalus::post *post)
{
	return class RemoveRedundantMaterials(post);
}							
Daedalus::post::RemoveRedundantMaterials::RemoveRedundantMaterials
(post *p):Base(p,p->steps&step?::RemoveRedundantMaterials:0)
{
	configKeepUnused = false;
}