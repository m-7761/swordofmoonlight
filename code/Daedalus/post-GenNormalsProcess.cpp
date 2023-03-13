#include "Daedalus.(c).h"
using namespace Daedalus;
//Assimp/GenFaceNormalsProcess.cpp
//Assimp/GenVertexNormalsProcess.cpp
class GenNormalsProcess 
{
	Daedalus::post &post;			 									 
	
	bool ShouldProceed(const preMesh *mesh)
	{			
		//AI: If the mesh consists of lines and/or points but not of
		//triangles or higher-order polygons the normal vectors are undefined
		if(!mesh->_trianglesflag&&!mesh->_polygonsflag)
		{	//SEEMS LIKE JUST SPAM THEN?
			//post("Normal vectors are undefined for line and point meshes");
			return false;
		}
		else return !mesh->HasNormals();
	}
	std::vector<bool> haveNormal;
	std::vector<signed> verticesFound; 
	void Smooth(preMorph *morph, size_t sortIndex)
	{			
		auto nl = morph->NormalsCoList();
		auto pl = PreConst(morph)->PositionsCoList();
		const PreNew<const pre3D[]> faceNormals(std::move(nl));
		nl.Reserve(pl.Size());
		if(maxAngle>=capAngle) //AI: There is no angle limit
		{
			//AI: Thus overlapping vertices share a common normal
			
			haveNormal.assign(nl.Size(),false);
			for(size_t i=nl;i<nl.Size();i++) if(!haveNormal[i]) 
			{
				//SCHEDULED OBSOLTE
				//AI: Get all vertices that share this one
				post.SpatialSort_Compute.FindCommonPositions(sortIndex,pl[i],verticesFound);

				pre3D vNor;
				for(size_t i=0;i<verticesFound.size();i++) 
				{
					const pre3D &v = faceNormals[verticesFound[i]];
					if(!_isnan(v.x)) vNor+=v;
				}
				vNor.Normalize();

				//AI: Write the smoothed normal back to the affected normals
				for(size_t i=0;i<verticesFound.size();i++)
				{
					nl[verticesFound[i]] = vNor;
					haveNormal[verticesFound[i]] = true;
				}
			}
		}		
		else //AI: Slower code path if a smooth angle is set
		{
			//AI: There are many ways to achieve the 
			//effect, this is the most straightforward
			const double limit = std::cos(maxAngle);
			for(size_t i=nl;i<nl.Size();i++)   
			{
				//SCHEDULED OBSOLTE
				//AI: Get all vertices that share this one
				post.SpatialSort_Compute.FindCommonPositions(sortIndex,pl[i],verticesFound);

				pre3D vNor,vr = faceNormals[i]; 				
				double limit_vrlen = vr.Length();
				for(size_t a=0;a<verticesFound.size();a++) 
				{
					pre3D v = morph->normalslist[verticesFound[a]];					
					//AI: check whether the angle between the two normals is not too large
					//HACK: if v.x is qnan the dot product will become qnan, too
					//therefore comparison against limit should be false
					if(v.DotProduct(vr)>=limit_vrlen*v.Length()) vNor+=v;
				}
				nl[i] = vNor.Normalize();
			}
		}				
	}
	PreSet<size_t,-1> sortIndex;
	void Process(const preMesh *mesh, preMorph *morph)
	{
		sortIndex++; //scheduled obsolete
		if(!morph->HasPositions()) return; //trivial

		auto pl = morph->PositionsCoList();
		const pre3D qnan(std::numeric_limits<double>::quiet_NaN());				
		auto nl = morph->NormalsCoList(); nl.Assign(pl.Size(),qnan);
		mesh->FacesList()^[&](const preFace &ea)
		{	
			if(ea.polytypeN<3) return void(ea); //point or line?	 
			auto il = mesh->IndicesSubList(ea);
			auto &pV1 = pl[il[0]], &pV2 = pl[il[1]], &pV3 = pl[il.Back()];
			const pre3D eaNormal = (pV2-pV1).CrossProduct(pV3-pV1).Normalize();
			il^[&](signed i){ nl[i] = eaNormal; };
		};
		if(maxAngle>0) Smooth(morph,sortIndex);
		log = true;
	}
	bool log;

public: //GenNormalsProcess

	GenNormalsProcess(Daedalus::post *p)
	:maxAngle(ChooseNormalsTypeByAngle())
	,post(*p),log(){}operator bool()
	{
		PreSuppose(!PreMesh2::IsSupported);

		post.progLocalFileName(maxAngle==0
		?"Generating Hard Lighting Normals"
		:"Generating Soft Lighting Smoothing Groups");

		post.Verbose("GenNormalsProcess begin");

		preScene *scene = post.Scene();

		//(raw face normals are "verbose")
		if(scene->_non_verbose_formatflag) //noting requirement
		{
			post.CriticalIssue("Post-processing order mismatch: expecting pseudo-indexed (\"verbose\") vertices here");
			return false;
		}

		scene->MeshesList()^[=](preMesh *ea)
		{		   
			if(!ShouldProceed(ea)) return void(ea);
			Process(ea,ea); ea->MorphsList()^[=](preMorph *ea2){ Process(ea,ea2); };
		};

		if(log) post("GenNormalsProcess finished. ")
		<<(maxAngle==0?"Face":"Vertex")<<" normals have been calculated";		
		else post.Verbose("GenNormalsProcess finished. Normals are already there");		
		return true;
	}	
	const double maxAngle; 
	inline double ChooseNormalsTypeByAngle()
	{
		if(post.steps&post.GenSmoothNormals::step)
		return std::max<double>(0,post.GenNormalsProcess_Vertex.configMaxAngle);
		return 0; //face normals
	}
	static const double capAngle; //175deg
};
const double GenNormalsProcess::capAngle(prePi-prePi/36); 
static bool GenNormalsProcess(Daedalus::post *post)
{
	return class GenNormalsProcess(post);
}	
Daedalus::post::GenNormals::GenNormals
(post *p):Base(p,p->steps&step?::GenNormalsProcess:0)
{
	if(p->steps&GenSmoothNormals::step) procedure = 0;
}	
Daedalus::post::GenSmoothNormals::GenSmoothNormals
(post *p):Base(p,p->steps&step?::GenNormalsProcess:0)
{
	configMaxAngle = GenNormalsProcess::capAngle;
}