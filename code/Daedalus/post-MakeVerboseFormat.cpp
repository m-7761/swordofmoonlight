#include "Daedalus.(c).h"  
using namespace Daedalus;
class MakeVerboseFormat //Assimp/MakeVerboseFormat.cpp
{	
	/////////////////////////////////////////
	//Everything here is SCHEDULED OBSOLETE//
	/////////////////////////////////////////

	Daedalus::post &post; 

	std::vector<signed> iMap;
	PreNew<double[]>::Vector swapMemory;	
	template<class PreList> void Swap(PreList &l)
	{
		if(!l.HasList()) return; PreSuppose(PreList::Item::Type::N<=4);
		auto swap = reinterpret_cast<PreList::Item::Type*>(swapMemory.Pointer());
		for(size_t i=0;i<iMap.size();i++) swap[i] = l[iMap[i]]; l.Assign(iMap.size(),swap);		
	}
	void Map(preMorph *morph) 
	{
		swapMemory.Resize(Pre4D::N*iMap.size(),preNoTouching);
		Swap(morph->PositionsCoList()); Swap(morph->NormalsCoList()); 
		Swap(morph->TangentsCoList()); Swap(morph->BitangentsCoList());
		for(size_t i=0;i<morph->colorslistsN;i++) Swap(morph->VertexColorsCoList(i));		
		for(size_t i=0;i<morph->texturecoordslistsN;i++) Swap(morph->TextureCoordsCoList(i));
	}
	std::vector<std::vector<PreBone::Weight>> newWeights;
	void Process(preMesh *mesh)
	{
		const size_t vertsAfter = 3*mesh->Faces();		
		const size_t vertsPrior = mesh->Positions();				
		//NEW: passing indices map to Map above
		iMap.clear(); iMap.reserve(vertsAfter);
		//AI: allocate enough memory to hold output bones & weights 		
		newWeights.resize(std::max<size_t>(mesh->Bones(),newWeights.size()));
		auto it = newWeights.begin();
		PreConst(mesh)->BonesList()^[&](const preBone *ea)
		{ it->clear(); it->reserve(3*ea->Weights()); it++; };
		//AI: build vertex map to originals and remap face indices 
		mesh->FacesList()^[&](preFace &ea)
		{
			PreConst(mesh)->IndicesSubList(ea)^[&](signed i)
			{	//////////////////////////////////////////////////
				//AI: need to build a clean map of bones as well//
				//COULD BUILD A LOOKUP TABLE OUT OF iMap HOWEVER//
				//THIS FILE IS SCHEDULED OBSOLETE SO WHY BOTHER?//
				PreConst(mesh)->BonesList()^[&](const preBone *ea)
				{		
					auto wEnd = ea->weightslist+ea->weights;
					auto w = std::lower_bound(ea->weightslist,wEnd,i);
					if(w<wEnd&&i==w->key)
					newWeights[i].push_back(PreBone::Weight(iMap.size(),w->value));
				};
				iMap.push_back(i); 
			};
			if(ea.polytype) ea.startindex = iMap.size()-ea.polytypeN;
		};
		mesh->IndicesList().Reserve(vertsAfter);
		for(size_t i=0;i<vertsAfter;i++) mesh->indiceslist[i] = i;		
		//NEW: fill in morphs' attributes
		Map(mesh); mesh->MorphsList()^[=](preMorph *ea){ Map(ea); };
		//AI: update output vertex weights
		it = newWeights.begin(); 
		mesh->BonesList()^[&](preBone *ea)
		{ ea->WeightsList().Assign(it->size(),it->data()); it++; };
		if(vertsAfter!=vertsPrior) 
		log = true;
	}
	bool log;

public: //MakeVerboseFormat

	MakeVerboseFormat(Daedalus::post *p)
	:post(*p),log(){}operator bool()
	{	
		post.progLocalFileName("Making Verbose Format");

		preScene *scene = post.Scene();
		if(!scene->_non_verbose_formatflag)
		{
			assert(0); return true;
		}
		else post.Verbose("MakeVerboseFormatProcess begin");
		
		post.Scene()->MeshesList()^[=](preMesh *ea){ Process(ea); };
		if(log) post("MakeVerboseFormatProcess finished.");
		else post.Verbose("MakeVerboseFormatProcess. There was nothing to do.");
		scene->_non_verbose_formatflag = false;
		return true;
	}
};
static bool MakeVerboseFormat(Daedalus::post *post)
{
	return class MakeVerboseFormat(post);
}							
Daedalus::post::_MakeVerboseFormat::_MakeVerboseFormat
(post *p):Base(p,p->steps&steps&&p->Scene()->_non_verbose_formatflag?::MakeVerboseFormat:0)
{}