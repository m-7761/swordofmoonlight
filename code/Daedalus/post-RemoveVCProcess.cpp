#include "Daedalus.(c).h"  
using namespace Daedalus;
class RemoveVCProcess //Assimp/RemoveVCProcess.cpp
{	
	Daedalus::post &post; 

	void Process(preMesh *mesh)
	{
		//AI: if all materials have been deleted let the material
		//index of the mesh point to the created default material
		if(!post.interestedIn.materials) 
		{ 
			mesh->material = 0;
			mesh->FacesList2()^[](preFace2 &ea){ ea.material = 0; };
		}
		if(!post.interestedIn.lightingNormals&&mesh->HasNormals())
		{
			log = true; mesh->NormalsCoList().Clear();
			mesh->TangentsCoList().Clear();	mesh->BitangentsCoList().Clear();
		}
		if(!post.interestedIn.tangentBivectors&&mesh->HasTangentsAndBitangents())
		{
			log = true; mesh->TangentsCoList().Clear();	mesh->BitangentsCoList().Clear();
		}
		//not implementing the per-list mask
		if(!post.interestedIn.textureCoords)
		for(size_t i=0;mesh->HasTextureCoords(i);i++)
		{
			log = true; mesh->TextureCoordsCoList(i).Clear();
		}
		//not implementing the per-list mask
		if(!post.interestedIn.perVertexColor)
		for(size_t i=0;mesh->HasVertexColors(i);i++)
		{
			log = true; mesh->VertexColorsCoList(i).Clear();
		}
		if(!post.interestedIn.weights&&mesh->HasBones())
		{
			log = true;	mesh->BonesList().Clear();
		}		
		if(!post.interestedIn.morphs&&mesh->HasMorphs())
		{
			log = true;	mesh->MorphsList().Clear();
		}		
	}
	bool log;

public: //RemoveVCProcess

	RemoveVCProcess(Daedalus::post *p)
	:post(*p),log(){}operator bool()
	{	
		post.progLocalFileName("Removing Unrequired Scene Elements");

		post.Verbose("RemoveVCProcess begin");		

		preScene *scene = post.Scene();
				
		if(scene->HasAnimations())
		if(!post.interestedIn.animations)
		{
			log = true;	scene->AnimationsList().Clear();
		}
		else //interested in animation data?
		{
			auto il = scene->AnimationsList();			
			if(!post.interestedIn.skinChannels)
			for(size_t i=il;i<il.Size();i++) if(il[i]->HasSkinChannels())
			{
				log = true;	il[i]->SkinChannelsList().Clear();
			}
			if(!post.interestedIn.morphChannels)
			for(size_t i=il;i<il.Size();i++) if(il[i]->HasMorphChannels())
			{
				log = true;	il[i]->MorphChannelsList().Clear();
			}
		}
		//not really necessary; but only because Assimp does this
		//NOTE: elsewhere references to textures should be avoided
		//(post.interestedIn.embeddedImages says make 1x1 textures)
		if(!post.interestedIn.textures&&scene->HasEmbeddedTextures())
		{
			log = true;	scene->EmbeddedTexturesList().Clear();
		}
		if(!post.interestedIn.materials&&scene->HasMaterials())
		{
			log = true;	scene->MaterialsList().Assign(1,new preMaterial(preDefault));
		}		 
		if(!post.interestedIn.lighting&&scene->HasLights())
		{
			log =  true; scene->LightsList().Clear();
		}
		if(!post.interestedIn.photography&&scene->HasCameras())
		{
			log =  true; scene->CamerasList().Clear();
		}
		if(!post.interestedIn.meshes)
		{
			log =  true; scene->MeshesCoList().Clear();
			scene->rootnode->ForEach([](preNode *ea){ ea->OwnMeshesList().Clear(); });
		}
		else scene->MeshesCoList()^[=](preMesh *ea){ Process(ea); };
		//AI: see if it is still a full scene
		if(!scene->meshes||!scene->materials)
		{
			if(!scene->incompleteflag)
			{
				scene->incompleteflag = true;
				post.Verbose("Flagging scene as \"incomplete\"");
			}
			//AI: if there are no more meshes, clear another flag
			if(!scene->meshes) scene->_non_verbose_formatflag = false;
		}
		if(!log) post.Verbose("RemoveVCProcess finished. Nothing was done.");		
		else post("RemoveVCProcess finished. Data structure cleanup was done.");
		return true;
	}
};
static bool RemoveVCProcess(Daedalus::post *post)
{
	return class RemoveVCProcess(post);
}							
Daedalus::post::RemoveComponent::RemoveComponent
(post *p):Base(p,p->steps&step?::RemoveVCProcess:0)
{
	//TODO: leave animation for pretransform?
	if(p->steps&p->PretransformVertices.step)
	{
		procedure = ::RemoveVCProcess;
		auto ii = const_cast<preInterestedIn&>(p->interestedIn);
		ii.weights = ii.morphs = ii.animations = false;			
	}
}