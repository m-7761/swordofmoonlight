#include "Daedalus.(c).h"  
using namespace Daedalus; 
class ScenePreprocessor //Assimp/ScenePreprocessor.cpp
{	
	Daedalus::post &post; 

	preScene *const scene;
	
	void Process(preMesh *mesh)
	{
		//NOTE: here Assimp fills out mNumUVComponents. The PreMesh
		//analog was to be texturedimensions, but it's thought better
		//to not repeat PreMaterial::mapping, and force meshes to agree
		//the decision to do so was prompted by the TextureTransform step

		//AI: if tangents/normals exist without bitangents, compute them
		if(mesh->tangentslist&&!mesh->bitangentslist&&mesh->HasNormals())    
		{
			auto tl = mesh->TangentsCoList(), nl = mesh->NormalsCoList();
			auto bl = mesh->BitangentsCoList(); bl.Reserve(tl.RealSize());
			for(size_t i=tl;i<tl.Size();i++) bl[i] = nl[i].CrossProduct(tl[i]);			
			mesh->MorphsList()^[&](preMorph *ea) //NEW
			{
				tl = ea->TangentsCoList(); if(tl.HasList())
				{
					auto nl2 = !ea->normalslist?nl:ea->NormalsCoList(); 
					bl = ea->BitangentsCoList(); bl.Reserve(tl.RealSize());
					for(size_t i=tl;i<tl.Size();i++) bl[i] = nl[i].CrossProduct(tl[i]);
				}
			};
		}

		//AI: -if unspecified-
		mesh->_polytypesflags = 0; //if(!mesh->_polytypesflags) 			
		PreConst(mesh)->FacesCoList()^[mesh](const preFace &ea)
		{ switch(ea.polytype)
		{ default: mesh->_polygonsflag = true; break;
		case PreFace::line: mesh->_linesflag = true; break;
		case PreFace::point: mesh->_pointsflag = true; break;		
		case PreFace::triangle: mesh->_trianglesflag = true; break;		
		case PreFace::start: assert(ea.startindex==PreFace::start_polygons);
		}};
	}
	void Process(preAnimation *anim)
	{  	//AI: See if the animation channel has no rotation
		//or position tracks. In this case generate a dummy
		//track from the transformation of the node's matrix
		auto l = anim->SkinChannelsList(); for(size_t i=l;i<l.Size();i++)
		{
			const PreAnimation::Skin *li = l[i]; 						
			bool srp[3] = { li->HasScaleKeys(),li->HasRotationKeys(),li->HasPositionKeys() };			
			if(!srp[0]||!srp[1]||!srp[2])
			{  	//AI: find the node that belongs to this animation
				preNode *node = scene->rootnode->FindNode(li->node);
				if(node) //AI: Validator will complain if node is 0
				{
				//AI: Decompose the node's transformation matrix
				pre3D scaling, position; preQuaternion rotation;
				node->matrix.Decompose(scaling,rotation,position);
				PreAnimation::Skin *li = l[i]; //const
				if(!srp[0]) li->ScaleKeysList().Assign(1,Pre3D::Time(0,scaling));
				if(!srp[0]) post.Verbose("Dummy scaling track has been generated");		
				if(!srp[1]) li->RotationKeysList().Assign(1,PreQuaternion::Time(0,rotation));
				if(!srp[1]) post.Verbose("Dummy rotation track has been generated");
				if(!srp[2]) li->PositionKeysList().Assign(1,Pre3D::Time(0,position));
				if(!srp[2]) post.Verbose("Dummy position track has been generated");									
				}
			}	
		}			
		anim->MorphChannelsList()^[](PreAnimation::Morph *ea)
		{ if(!ea->HasMorphKeys()) ea->MorphKeysList().Assign(1,PreMorph::Time(0,-1)); };
		if(anim->duration<0) //AI: compute if unspecified
		{				
			#define BOOKEND(x) assert(ea->x);\
			min = std::min(min,ea->x##list[0].key);\
			max = std::max(max,ea->x##list[ea->x-1].key);
			//Assimp does minmax over every key. Seems quite unnecessary
			//(though admittedly this code was time consuming to setup!)
			double min = std::numeric_limits<double>::max(), max = -min;
			PreConst(anim)->SkinChannelsList()^[&min,&max](const PreAnimation::Skin *ea)
			{ BOOKEND(scalekeys) BOOKEND(positionkeys) BOOKEND(rotationkeys)
			};PreConst(anim)->MorphChannelsList()^[&min,&max](const PreAnimation::Morph *ea)
			{ BOOKEND(morphkeys) };
			#undef BOOKEND
			//Assimp uses std::min here. Could be a bug??
			anim->duration = max-std::max<double>(0,min);
			post.Verbose("Animation duration has been generated");
		}
	}	
	void GroupMeshesByName() //todo? alphabetize
	{
		auto il = scene->MeshesCoList();		
		for(preN i=il;i<il.Size();i++) il[i]->name._target = i;
		il.Sort([](preMesh *a, preMesh *b){	return a->name.cstring<b->name.cstring;	});
		scene->rootnode->ForEach([&](preNode *ea)
		{ ea->OwnMeshesList()^[&](preID &ea2){ ea2 = il[ea2]->name._target; }; });
	}
	void ParkNodelessMeshesInRootNode()
	{
		std::vector<size_t> meshRefs(scene->Meshes(),0);		
		scene->rootnode->ForEach([&meshRefs](const preNode *ea)
		{ ea->OwnMeshesList()^[&](size_t ea2){ meshRefs[ea2]++; };});				
		auto v = scene->rootnode->OwnMeshesList().AsVector(); 
		size_t meshesPrior = v.Size();
		for(size_t i=0;i<meshRefs.size();i++) if(!meshRefs[i]) v.PushBack(i); 
		if(meshesPrior<v.Size())
		post.Verbose("Moved ")<<v.Size()-meshesPrior<<"/"<<meshRefs.size()<<" nodeless meshes into root node";
	}	
 
public: //ScenePreprocessor

	ScenePreprocessor(Daedalus::post *p)	
	:post(*p),scene(p->Scene()){}operator bool()
	{
		post.progLocalFileName("Preparing Scene for Postprocessing");

		//NEW: Assimp wasn't doing these two
		if(!scene->rootnode) scene->rootnode = new preNode; 
		ParkNodelessMeshesInRootNode();
		GroupMeshesByName(); //NEW: simplifying Collada
		scene->MeshesCoList()^[=](preMesh *ea){ Process(ea); };
		//hack: anticipate removal by Pretransform step
		post.PretransformVertices._animationsToBeRemoved = 
		scene->AnimationsList()^[=](preAnimation *ea){ Process(ea); };
		//AI: supply material if none was specified
		//NEW: Assimp 0's the mesh material indices. Assuming 0'ed
		if(!scene->HasMaterials()) 
		scene->MaterialsList().Assign(1,new preMaterial(preDefault));	
		return true;
	}
};
static bool ScenePreprocessor(Daedalus::post *post)
{
	return class ScenePreprocessor(post);
}							
Daedalus::post::_ScenePreprocessor::_ScenePreprocessor
(post *p):Base(p,::ScenePreprocessor)
{}