#include "Daedalus.(c).h"
using namespace Daedalus;
class ConvertToLHProcess //Assimp/ConvertToLHProcess.cpp
{
	Daedalus::post &post; 
	
	static void Process(preMesh *mesh)
	{
		PreSuppose(!PreMesh2::IsSupported);
		auto &f = [](pre3D &ea){ ea.z*=-1; };
		auto &g = [&f](preMorph *ea)
		{
			ea->PositionsCoList()^f; ea->NormalsCoList()^f; 
			ea->TangentsCoList()^f; ea->BitangentsCoList()^f;
		};
		g(mesh); mesh->MorphsCoList()^g;

		/////NEW/////NEW/////NEW/////NEW/////NEW/////NEW/////NEW/////NEW/////
		//According to this comment this goes in ConvertToLHProcess_FlipUVs right?
		//AI: mirror bitangents as well as they're derived from the texture coords
		//mesh->BitangentsCoList()^(pre3D &ea){ ea*=-1; };
		mesh->BonesList()^[](preBone *ea)
		{
			ea->matrix.a3*=-1; ea->matrix.b3*=-1; ea->matrix.d3*=-1;
			ea->matrix.c1*=-1; ea->matrix.c2*=-1; ea->matrix.c4*=-1;
		};
	} 
	static void Process(preMaterial *mat)
	{
		mat->PropertiesList()^[mat](PreMaterial::Property *ea)
		{			
			if(mat->_mapping_axis==ea->key)	ea->Datum(mat->mapping_axis)*=-1;
		};
	}	
	static void Process(PreAnimation::Skin *skin)
	{
		skin->RotationKeysList()^[](PreQuaternion::Time &ea)
		{
			//NOT SURE WHAT THIS IS TRYING TO SAY
			/*AI: That's the safe version, but the double errors add up. So we try the short version instead
			PreQuaternion::Matrix rotmat = ea.value.GetMatrix();
			rotmat.a3 = -rotmat.a3; rotmat.b3 = -rotmat.b3;
			rotmat.c1 = -rotmat.c1; rotmat.c2 = -rotmat.c2;
			skin->rotationkeyslist[a].value = preQuaternion(rotmat);
			*/ea.value.x*=-1; ea.value.y*=-1;
		};
		skin->PositionKeysList()^[](Pre3D::Time &ea){ ea.value.z*=-1; };
	}
	
public: //ConvertToLHProcess
	 
	ConvertToLHProcess(Daedalus::post *p)
	:post(*p){}operator bool()
	{	
		PreSuppose(!PreMesh2::IsSupported);

		post.progLocalFileName("Converting to Left-handed Coordinate System");

		post.Verbose("MakeLeftHandedProcess begin");

		preScene *scene = post.Scene();	
		scene->rootnode->ForEach([](preNode *ea) 
		{
			//AI: mirror all base vectors at the local Z axis
			ea->matrix.c1*=-1; ea->matrix.c2*=-1; /*ea->matrix.c3*=-1;*/ ea->matrix.c4*=-1;
			//AI: now invert the Z axis again to keep the matrix determinant positive.
			//The local meshes will be inverted accordingly so that the result should be just fine
			ea->matrix.a3*=-1; ea->matrix.b3*=-1; /*ea->matrix.c3*=-1;*/ ea->matrix.d3*=-1;
			//REMINDER: Assimp passes in an identity matrix and then procedes to do:
			//for(i...) ProcessNode(ea->nodes[i],pParentGlobalRotation*ea->matrix);
			//(but this cannot possibly have any effect, can it?)
		});
		scene->MeshesCoList()^[=](preMesh *ea){ Process(ea); };
		scene->MaterialsList()^[=](preMaterial *ea){ Process(ea); };
		scene->AnimationsList()^[=](preAnimation *ea)
		{ ea->SkinChannelsList()^[=](preAnimation::Skin *ea2){ Process(ea2); }; };
		post.Verbose("MakeLeftHandedProcess finished");
		return true;
	}
};
static bool ConvertToLHProcess(Daedalus::post *post)
{
	return class ConvertToLHProcess(post);
}							
Daedalus::post::MakeLeftHanded::MakeLeftHanded
(post *p):Base(p,p->steps&step?::ConvertToLHProcess:0)
{}

class ConvertToLHProcess_FlipUVs
{
	Daedalus::post &post; 

	//AI: mirror V texcoords and rotation
	static void Process(preMaterial *mat)
	{
		mat->PropertiesList()^[mat](PreMaterial::Property *ea)
		{
			if(mat->_matrix!=ea->key) return;
			auto &uv = ea->Datum(mat->matrix);
			#ifdef NDEBUG
			#error UNTESTED: changing to using preMatrix
			#endif
			//uv.translation.y*=-1; uv.rotation*=-1;
			pre3D scaling, position; preQuaternion rotation;
			uv.Decompose(scaling,rotation,position);
			position.y*=-1; rotation.z*=-1; rotation.w*=-1;
			new(&uv)preMatrix(scaling,rotation,position);			
		};
	}
	static void Process(preMesh *mesh)
	{	
		PreSuppose(!PreMesh2::IsSupported);
		for(size_t i=0;mesh->HasTextureCoords(i);i++) 
		mesh->TextureCoordsCoList(i)^[](pre3D &ea){ ea.y = 1-ea.y; }; 		
		/////NEW/////NEW/////NEW/////NEW/////NEW/////NEW/////NEW/////NEW/////
		//MOVING THIS HERE FROM ConvertToLHProcess//////////////////////////
		//AS THE COMMENT BELOW SUGGESTS IT'S RELATED TO TEXTURE COORDINATES
		//AI: mirror bitangents as well as they're derived from the texture coords
		auto &f = [](preMorph *ea){ ea->BitangentsCoList()^[](pre3D &ea2){ ea2 = -ea2; }; };
		f(mesh); mesh->MorphsCoList()^f;
	}	
	
public: //ConvertToLHProcess_FlipUVs
	 
	ConvertToLHProcess_FlipUVs(Daedalus::post *p)
	:post(*p){}operator bool()
	{
		PreSuppose(!PreMesh2::IsSupported);

		post.progLocalFileName("Flipping UVs");

		post.Verbose("FlipUVsProcess begin");
		post.Scene()->MeshesCoList()^[=](preMesh *ea){ Process(ea); };
		post.Scene()->MaterialsList()^[=](preMaterial *ea){ Process(ea); };
		post.Verbose("FlipUVsProcess finished");
		return true;
	}
};
static bool ConvertToLHProcess_FlipUVs(Daedalus::post *post)
{
	return class ConvertToLHProcess_FlipUVs(post);
}							
Daedalus::post::FlipUVs::FlipUVs
(post *p):Base(p,p->steps&step?::ConvertToLHProcess_FlipUVs:0)
{}

class ConvertToLHProcess_FlipWindingOrder
{
	Daedalus::post &post; 

	static void Process(preMesh *mesh)
	{
		if(mesh->HasIndices())
		PreConst(mesh)->FacesList()^[mesh](const preFace &ea)
		{
			auto il = mesh->IndicesSubList(ea);
			for(size_t i=0;i<il.Size()/2;i++) std::swap(il[i],il[il.Size()-1-i]);
		};
	}
	static void Process(preMesh2 *mesh2)
	{
		PreSuppose(!PreMesh2::IsSupported); //unimplemented
	}

public: //ConvertToLHProcess_FlipWindingOrder
	 
	ConvertToLHProcess_FlipWindingOrder(Daedalus::post *p)
	:post(*p){}operator bool()
	{
		PreSuppose(!PreMesh2::IsSupported);

		post.progLocalFileName("Flipping Winding Order");

		post.Verbose("FlipWindingOrderProcess begin");
		if(post.simpleSceneOut)
		post.simpleSceneOut->MeshesList()^[=](preMesh *ea){ Process(ea); };
		if(post.complexSceneOut)
		post.complexSceneOut->MeshesList2()^[=](preMesh2 *ea){ Process(ea); };
		post.Verbose("FlipWindingOrderProcess finished");
		return true;
	}
};
static bool ConvertToLHProcess_FlipWindingOrder(Daedalus::post *post)
{
	return class ConvertToLHProcess_FlipWindingOrder(post);
}							
Daedalus::post::FlipWindingOrder::FlipWindingOrder
(post *p):Base(p,p->steps&step?::ConvertToLHProcess_FlipWindingOrder:0)
{}