#include "Daedalus.(c).h"
using namespace Daedalus;
class FindInstancesProcess //Assimp/FindInstancesProcess.cpp
{
	////////////////////////////////////////////////////////
	//Everything here is SCHEDULED OBSOLETE. What's called//
	//for is a global per mesh data structure representing//
	//uniqueness, comparable as a whole to prove instances//
	////////////////////////////////////////////////////////

	Daedalus::post &post;
			
	//these two routines MUST be in agreement
	static inline unsigned long long GetMeshHash(const preMesh* in)
	{				   
		//AI: get an unique value representing the vertex format of the mesh
		const auto fhash = (unsigned long long)in->GetMeshVFormatUnique()<<32ULL;
		//AI: and combine it with the vertices/faces/bones/material/primitives
		unsigned hash = in->bones<<16^in->positions^in->faces<<4^in->material<<15^in->_polytypesflags<<28;
		return fhash|(hash&0xFFFFFFFF);
	}  
	static bool GetMeshHash_Collision(const preMesh *a, const preMesh *b)
	{
		//NOTE: Assimp skips the VF check because of the hash
		//construction, however it's not so costly to do is it?
		if(a->GetMeshVFormatUnique()!=b->GetMeshVFormatUnique()
		||a->bones!= b->bones||a->faces!=b->faces||a->positions!=b->positions   
		||a->material!=b->material||a->_polytypesflags!=b->_polytypesflags)
		return true; return false;
	}

	template<class preND>
	static inline bool CompareArrays(const preND *first, const preND *second, size_t size, double const epsilon2)
	{
		for(const preND *end=first+size;first!=end;first++,second++) 
		if((*first-*second).SquareLength()>=epsilon2) return false; return true;
	}
	static bool CompareBones(const preMesh *orig, const preMesh *inst, double const epsilonIn)
	{
		PreBone::Weight::typeofvalue const epsilon(epsilonIn);
		for(size_t i=0;i<orig->bones;i++)
		{
			preBone *aha = orig->boneslist[i], *oha = inst->boneslist[i];
			if(aha->weights!=oha->weights||aha->matrix!=oha->matrix) 
			return false;			
			for(size_t n=0;n<aha->weights;n++) 			
			if(aha->weightslist[n].key!=oha->weightslist[n].key 
			||(aha->weightslist[n].value-oha->weightslist[n].value)<epsilon)
			return false;
		}
		return true;
	}
	static void UpdateMeshIndices(preNode *node, size_t *lookup)
	{
		for(size_t n=0;n<node->meshes;n++)
		node->mesheslist[n] = lookup[node->mesheslist[n]];
		//recursively update child nodes
		for(size_t n=0;n<node->nodes;n++)
		UpdateMeshIndices(node->nodeslist[n],lookup);
	}

public: //FindInstancesProcess

	FindInstancesProcess(Daedalus::post *p)
	:post(*p){}operator bool()
	{
		PreSuppose(!PreMorph::IsSupported);
		#ifdef NDEBUG
		#error unfinished
		#endif
		assert(!"FindInstancesProcess requires more work. Disable it. Aborting.");
		post.CriticalIssue("FindInstancesProcess requires more work. Disable it. Aborting.");
		return true;

		post.progLocalFileName("Instancing Identical Meshes");

		post.Verbose("FindInstancesProcess begin");

		preScene *scene = post.Scene();

		//(so says faces comparison step)
		if(scene->_non_verbose_formatflag) //noting requirement
		{
			post.CriticalIssue("Post-processing order mismatch: expecting pseudo-indexed (\"verbose\") vertices here");
			return false;
		}

		//NOTE: These replace boost::scoped_array
		//AI: index lookup table of UpdateMeshIndices
		std::vector<size_t> remapping(scene->meshes);
		//AI: use a pseudo hash to eliminate candidates early
		std::vector<unsigned long long> hashes(scene->meshes);	
		std::vector<size_t> ftbl_orig, ftbl_inst; preSize ftbl_s = 0; 

		size_t numMeshesOut = 0;		
		for(size_t i=0;i<scene->meshes;i++) 
		{
			preMesh *inst = scene->mesheslist[i];
			hashes[i] = GetMeshHash(inst);
			for(int a=(int)i-1;a>=0;a--) if(hashes[i]==hashes[a])
			{
				preMesh *orig = scene->mesheslist[a];
				if(!orig||GetMeshHash_Collision(orig,inst))	
				continue;

				const double epsilon2 = std::pow
				//tricky: squared to avoid sqrt in CompareArrays
				(post.SpatialSort_Compute.configSeparationEpsilon,2);				
				const double fixedEpsilon2 = post.fixedEpsilon2();
				
				if(orig->HasPositions()) 				
				if(!CompareArrays(orig->positionslist,inst->positionslist,orig->positions,epsilon2))
				continue;
				if(orig->HasNormals()) 
				if(!CompareArrays(orig->normalslist,inst->normalslist,orig->positions,fixedEpsilon2))
				continue;
				if(orig->HasTangentsAndBitangents()) 
				if(!CompareArrays(orig->tangentslist,inst->tangentslist,orig->positions,fixedEpsilon2) 
				||!CompareArrays(orig->bitangentslist,inst->bitangentslist,orig->positions,fixedEpsilon2))
				continue;
								
				size_t i, end = orig->CountTextureCoordsLists();
				for(i=0;i<end;i++) if(orig->texturecoordslists[i]) //paranoia?
				if(!CompareArrays(orig->texturecoordslists[i],inst->texturecoordslists[i],orig->positions,fixedEpsilon2))
				break;
				if(i!=end) continue;
				end = orig->CountVertexColorsLists();
				for(i=0;i<end;i++) if(orig->colorslists[i]) //paranoia?
				if(!CompareArrays(orig->colorslists[i],inst->colorslists[i],orig->positions,fixedEpsilon2)) 
				break;
				if(i!=end) continue;

				//NOTE: Daedalus isn't a speed oriented project
				//However others may want to repurpose its code
				//AI: These two checks are actually quite expensive and almost *never* required.
				//Almost. That's why they're still here. But there's no reason to do them in speed-targeted imports.
				if(!post.FindInstancesProcess.configSpeedFlag) 
				{
					//A: It seems to be strange, but we really need to see if the
					//bones are identical too. Although it's extremely unprobable
					//that they're not if control reaches here, we need to deal
					//with unprobable cases, too. It could still be that there are
					//equal shapes which are deformed differently.
					if(!CompareBones(orig,inst,post.fixedEpsilon())) continue;

					PreSuppose(post.MakeVerboseFormat.steps);
					//NOTE: Mentions "VERBOSE-FORMAT"
					//AI: For completeness ... compare even the index buffers for equality
					//face order & winding order doesn't care. Input data is in verbose format.					
					if(!ftbl_s) //(positions is equal to the number of indices in verbose format)
					{
						for(size_t i=0;i<scene->meshes;i++)
						ftbl_s = std::max(ftbl_s,scene->mesheslist[i]->positions);
						ftbl_orig.resize(ftbl_s); ftbl_inst.resize(ftbl_s);
					}
					const preFace *o = orig->faceslist;
					const preFace *p = inst->faceslist, *d = p+orig->faces;
					size_t faceID; for(faceID=0;p<d&&o->polytypeN==p->polytypeN;faceID++)
					{
						auto ol = orig->IndicesSubList(*o++), pl = inst->IndicesSubList(*p++);
						ol^=[&](size_t i){ ftbl_orig[ol[i]] = ftbl_inst[pl[i]] = faceID; };
					}
					if(faceID<orig->faces
					||memcmp(ftbl_inst.data(),ftbl_orig.data(),orig->positions*sizeof(size_t)))
					continue;
				}
				//AI: instanced
				remapping[i] = remapping[a];				
				delete inst; scene->mesheslist[i] = 0;
				break;
			}
			//AI: If not instanced, keep it
			if(scene->mesheslist[i]) remapping[i] = numMeshesOut++;
		}
		if(numMeshesOut!=scene->meshes) 
		{
			//AI: Collapse the meshes removing 0 entries
			for(size_t real=0,i=0;real<numMeshesOut;i++) 
			if(scene->mesheslist[i]) scene->mesheslist[real++] = scene->mesheslist[i];
			UpdateMeshIndices(scene->rootnode,remapping.data());			
			post("FindInstancesProcess finished. Found ")<<scene->meshes-numMeshesOut<<" instances";			
			scene->meshes = numMeshesOut;
		}
		else post.Verbose("FindInstancesProcess finished. No instanced meshes found");
   		return true;
	}
};		
static bool FindInstancesProcess(Daedalus::post *post)
{
	return class FindInstancesProcess(post);
}							
Daedalus::post::FindInstances::FindInstances
(post *p):Base(p,p->steps&step?::FindInstancesProcess:0)
{
	if(p->steps&p->PretransformVertices.step) procedure = 0;

	configSpeedFlag = false;
}