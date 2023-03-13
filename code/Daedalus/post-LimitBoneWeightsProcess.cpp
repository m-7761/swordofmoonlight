#include "Daedalus.(c).h"  
using namespace Daedalus;
class LimitBoneWeightsProcess //Assimp/LimitBoneWeightsProcess.cpp
{	
	Daedalus::post &post; 	 	
		  
	const size_t maximum;	  	
	//optimizing (concerned about heap allocations)
	template<typename T, int N> struct FixedVector : std::array<T,N>
	{
		typedef void back, pop_back;
		size_t _size; FixedVector():_size(){} 		
		inline size_t size()const{ return _size; }
		inline iterator end(){ return begin()+size(); }		
		inline const_iterator cend()const{ return cbegin()+size(); }
		template<class CP> inline bool push_back(const CP &cp)
		{
			if(_size>=N) //paranoia: worst case scenario
			{
				assert(_size==N); //should be pre-sorted
				for(size_t i=0;i<N;i++) if(cp<(*this)[i])
				{
					end()->~T();
					for(size_t ii=i+1;ii<N;i++) (*this)[ii] = (*this)[ii-1];
					new(&at(i))T(cp); break;					
				}				
				return false; //signal data loss incurred
			}
			new(&at(_size++))T(cp); if(_size==N) std::sort(begin(),end());
			return true;
		}		
		inline iterator erase(iterator it, iterator itt)
		{ _size = it-begin(); while(it<itt) it++->~T(); return it; }
	};
	struct Weight //Assimp/LimitBoneWeightsProcess.h
	{
		size_t bone; double weight; Weight(){} Weight(size_t b):bone(b){}		
		inline bool operator<(const Weight &cmp)const{ return weight>cmp.weight; }
	};		
	std::vector<FixedVector<Weight,16>> vertexWeights;
	std::vector<FixedVector<PreBone::Weight,16>> boneWeights;
	void Process(preMesh *mesh)
	{
		size_t bonesPrior = mesh->Bones(); if(!bonesPrior) return;

		//AI: collect all weights per vertex
		vertexWeights.clear();
		vertexWeights.resize(mesh->Positions());	
		auto il = mesh->BonesList(); il^=[&](Weight i)
		{ il[i.bone]->WeightsList()^[&](const PreBone::Weight &ea)
		{ i.weight = ea.value; vertexWeights[ea].push_back(i);
		};};		
		//AI: remove after maximum
		size_t weightsRemoved = 0;
		for(auto vit=vertexWeights.begin();vit!=vertexWeights.end();vit++)
		if(vit->size()>maximum)
		{						  
			std::sort(vit->begin(),vit->end());		
			weightsRemoved+=vit->size()-maximum;
			vit->erase(vit->begin()+maximum,vit->end());			
			//AI: re-normalize the weights
			double sum = 0;
			for(auto it=vit->cbegin();it!=vit->cend();it++) sum+=it->weight;			
			if(sum) continue; //dummy weights?
			const double invSum = 1/sum;
			for(auto it=vit->begin();it!=vit->end();it++) it->weight*=invSum;
		}
		if(0==weightsRemoved) return; //TRIVIAL/NOT LOGGING					
		//AI: rebuild the vertex weight array for all bones
		boneWeights.resize(il.Size());
		for(size_t i=0;i<vertexWeights.size();i++)
		{
			const auto &vw = vertexWeights[i];
			for(auto it=vw.cbegin();it!=vw.cend();it++)
			boneWeights[it->bone].push_back(PreBone::Weight(i,it->weight));
		}				
		auto it = boneWeights.begin(); il^[&](preBone* &ea)
		{ 
			if(it->empty()){ delete ea; ea = 0; }else
			ea->WeightsList().Assign(it->size(),it->data()); it++;
		};
		size_t bonesAfter = 0; il^=[&](size_t i)
		{ if(il[i]) il[bonesAfter++] = il[i]; }; il.ForgoMemory(bonesAfter);

		post("Removed ")<<weightsRemoved<<" weights. Input bones: "<<bonesPrior<<". Output bones: "<<bonesAfter;			
	}

public: //LimitBoneWeightsProcess

	LimitBoneWeightsProcess(Daedalus::post *p)
	:post(*p),maximum(post.LimitBoneWeightsProcess.configMaxWeights){}operator bool()
	{	
		post.progLocalFileName("Limiting Skinimation Bone Weights");

		post.Verbose("LimitBoneWeightsProcess begin");
		if(maximum>vertexWeights.data()->max_size())		
		post.CriticalIssue("Limiting configured limit ")<<maximum<<" to internal buffer size of "<<vertexWeights.data()->max_size();
		post.Scene()->MeshesCoList()^[=](preMesh *ea){ Process(ea); };
		post.Verbose("LimitBoneWeightsProcess end");
		return true;
	}
};
static bool LimitBoneWeightsProcess(Daedalus::post *post)
{
	return class LimitBoneWeightsProcess(post);
}							
Daedalus::post::LimitBoneWeights::LimitBoneWeights
(post *p):Base(p,p->steps&step?::LimitBoneWeightsProcess:0)
{
	configMaxWeights = 4;
}