#include "Daedalus.(c).h"
using namespace Daedalus;
class FixNormalsStep //Assimp/FixNormalsStep.cpp
{
	Daedalus::post &post;

	void Process(preMesh *mesh, preID meshID)
	{
		if(!mesh->HasNormals()) return;

		PreSuppose(!PreMesh2::IsSupported);

		//AI: Compute the bounding box of both the model vertices+normals 
		//and the umodified model vertices. Then check whether the first BB
		//is smaller than the second. In this case we can assume that the
		//normals need to be flipped, although there are a few special cases:
		//convex, concave, planar models
		auto nl = PreConst(mesh)->NormalsCoList();
		auto pl = PreConst(mesh)->PositionsCoList();
		Pre3D::Container without = mesh->GetContainer();
		Pre3D::Container with = Pre3D::MinMax();		
		pl^=[&](size_t i){ with.AddToContainer(pl[i]+nl[i]); };
		pre3D withDelta = with.maxima-with.minima;
		pre3D withoutDelta = without.maxima-without.minima;
		//AI: Check whether the boxes are overlapping
		if((withDelta.x>0)!=(withoutDelta.x>0)) return;
		if((withDelta.y>0)!=(withoutDelta.y>0)) return;
		if((withDelta.z>0)!=(withoutDelta.z>0)) return;
		//AI: Check whether this is a planar surface
		const double withoutDelta_yz = withoutDelta.y*withoutDelta.z;
		if(withoutDelta.x<0.05*std::sqrt(withoutDelta_yz)) return;
		if(withoutDelta.y<0.05*std::sqrt(withoutDelta.z*withoutDelta.x)) return;
		if(withoutDelta.z<0.05*std::sqrt(withoutDelta.y*withoutDelta.x)) return;
		//AI: compare the volumes of the bounding boxes
		if(std::abs(withDelta.x*withDelta.y*withDelta.z)<std::fabs(withoutDelta.x*withoutDelta_yz))
		{
			if(mesh->name.HasString()) post(mesh->name.String());
			post("Mesh ")<<meshID<<": Normals face inward (or so it seems)";
			//AI: Invert normals and flip faces
			auto &f = [](pre3D &ea){ ea*=-1.0f; }; mesh->NormalsCoList()^f; 
			mesh->MorphsList()^[&](preMorph *ea){ ea->NormalsCoList()^f; };
			PreConst(mesh)->FacesList()^[mesh](const preFace &ea)
			{
				auto il = mesh->IndicesSubList(ea);
				for(size_t i=il;i<il.Size()/2;i++) std::swap(il[i],il[il.Size()-1-i]);
			};
			log = true;
		}
	}
	bool log;

public: //FixNormalsStep

	FixNormalsStep(Daedalus::post *p)
	:post(*p),log(){}operator bool()
	{	
		//todo? provide a manual option
		post.progLocalFileName("Automatically Flipping Normals");

		post.Verbose("FixInfacingNormalsProcess begin");	
		preID meshID = 0; PreSuppose(!PreMesh2::IsSupported);
		post.Scene()->MeshesList()^[&](preMesh *ea){ Process(ea,meshID++); };
		if(log) post("FixInfacingNormalsProcess finished. Found issues.");
		else post.Verbose("FixInfacingNormalsProcess finished. No changes to the scene.");
		return true;
	}
};
static bool FixNormalsStep(Daedalus::post *post)
{
	return class FixNormalsStep(post);
}							
Daedalus::post::FixInfacingNormals::FixInfacingNormals
(post *p):Base(p,p->steps&step?::FixNormalsStep:0)
{}