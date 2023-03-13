#include "Daedalus.(c).h"
using namespace Daedalus;
class CalcTangentsProcess //Assimp/CalcTangentsProcess.cpp
{	
	Daedalus::post &post; 

	const double maxAngle;
	std::vector<bool> normalDone;	  
	std::vector<signed> verticesFound, closeVertices;
	void Process(const preMesh *mesh, preMorph *morph, size_t sortIndex)
	{
		PreSuppose(!PreMesh2::IsSupported);

		if(morph->HasTangentsAndBitangents()) return; //trivial

		//AI: we assume that the morph is still in the verbose vertex format 
		//where each face has its own set of vertices and no vertices are shared		

		if(!mesh->_trianglesflag&&!mesh->_polygonsflag)
		{
			if(mesh==morph)
			post.Verbose("Tangents are undefined for line and point morphes");
			return;
		}
		if(!morph->HasNormals())
		{
			if(mesh==morph&&!mesh->Normals())
			post.CriticalIssue("Failed to compute tangents; need normals");
			return;
		}
		if(!morph->HasTextureCoords(post.CalcTangentsProcess.configSourceUV))
		{
			if(mesh==morph&&!mesh->TextureCoords())
			post.CriticalIssue("Failed to compute tangents; need UV data in channel #")
			<<post.CalcTangentsProcess.configSourceUV;
			return;
		}

		const double angleEpsilon = 0.9999;		
		const pre3D qnan(std::numeric_limits<double>::quiet_NaN());
		normalDone.clear(); normalDone.assign(morph->Normals(),false);

		auto morphTang = morph->TangentsCoList(), morphBitang = morph->BitangentsCoList();
		auto morphTex = PreConst(morph)->TextureCoordsCoList(post.CalcTangentsProcess.configSourceUV);
		auto morphPos = PreConst(morph)->PositionsCoList(), morphNorm = PreConst(morph)->NormalsCoList();
		morphTang.Reserve(morphNorm.Size()); //co-list		
		auto meshInd = PreConst(mesh)->IndicesList();
		auto l = PreConst(mesh)->FacesList(); for(preN i=l;i<l.Size();i++)
		{
			const preFace &face = mesh->faceslist[i];
			const signed *faceInd = &meshInd[face.startindex];

			if(face.polytypeN<3) //line or point?
			{
				for(size_t i=0;i<face.polytypeN;i++)
				{
					signed ind = faceInd[i]; 
					morphTang[ind] = morphBitang[ind] = qnan; normalDone[ind] = true;
				}
				continue;
			}
			
			//AI: USING ONLY THE FIRST THREE INDICES. SO ASSUME PLANAR POLYGON
			const signed uv0 = faceInd[0], uv1 = faceInd[1], uv2 = faceInd[2];

			pre3D v = morphPos[uv1]-morphPos[uv0], w = morphPos[uv2]-morphPos[uv0];
			double sx = morphTex[uv1].x-morphTex[uv0].x, sy = morphTex[uv1].y-morphTex[uv0].y;
			double tx = morphTex[uv2].x-morphTex[uv0].x, ty = morphTex[uv2].y-morphTex[uv0].y;
			double dirCorrection = (tx*sy-ty*sx)<0?-1:1;
			//AI: using the default UV direction if there is no separation in UV space
			if(!sx&&!sy&&!tx&&!ty ){ sx = 0; sy = 1; tx = 1; ty = 0; }

			//AI: tangent points along the positive X axis of the UV coords
			//in model space. so bitangents point along the positive Y axis
			pre3D uvTang = (w*sy-v*ty)*dirCorrection, uvBitang = (w*sx-v*tx)*dirCorrection;

			for(size_t i=0;i<face.polytypeN;i++)
			{
				signed normID = faceInd[i];
				pre3D norm3D = morphNorm[normID];
				//AI: project tangent/bitangent onto the vertex normal plane 
				pre3D localTang = uvTang-norm3D*uvTang.DotProduct(norm3D);
				pre3D localBitang = uvBitang-norm3D*uvBitang.DotProduct(norm3D);
				localTang.Normalize(); localBitang.Normalize();	 
				//AI: reconstruct tangent/bitangent according to normal and bitangent/tangent when infinite or NaN.
				bool invalidTang = !_finite(localTang.x)||!_finite(localTang.y)||!_finite(localTang.z);
				bool invalidBitang = !_finite(localBitang.x)||!_finite(localBitang.y)||!_finite(localBitang.z);
				if(invalidTang!=invalidBitang) 
				{
					if(invalidTang)
					{
						localTang = norm3D.CrossProduct(localBitang);
						localTang.Normalize();
					} 
					else 
					{
						localBitang = localTang.CrossProduct(norm3D);
						localBitang.Normalize();
					}
				}
				morphTang[normID] = localTang; morphBitang[normID] = localBitang;
			}
		}

		//smooth tangents/bitantents?	
		const double fLimit = cosf(maxAngle);		
		assert(morphNorm.Size()==morphPos.Size());
		for(size_t i=morphNorm;i<morphNorm.Size();i++) 
		{
			if(normalDone[i]) continue; //line or point?

			//SCHEDULED OBSOLTE
			post.SpatialSort_Compute.FindCommonPositions(sortIndex,morphPos[i],verticesFound);

			closeVertices.clear();
			closeVertices.reserve(verticesFound.size()+5);
			closeVertices.push_back(i);
			//AI: look among them for other vertices sharing
			//the same normal and a close-enough tangent/bitangent
			for(size_t j=0;j<verticesFound.size();j++)
			{
				size_t ind = verticesFound[j];
				if(!normalDone[ind]) 
				if(morphNorm[ind].DotProduct(morphNorm[i])>=angleEpsilon) //~1
				if(morphTang[ind].DotProduct(morphTang[i])>=fLimit)
				if(morphBitang[ind].DotProduct(morphBitang[i])>=fLimit)
				{
					//AI: add to smoothing group
					closeVertices.push_back(ind); normalDone[ind] = true;
				}
			}

			pre3D smoothTang, smoothBitang;
			for(size_t i=0;i<closeVertices.size();i++)
			{
				smoothTang+=morphTang[closeVertices[i]];
				smoothBitang+=morphBitang[closeVertices[i]];
			}
			smoothTang.Normalize(); smoothBitang.Normalize();
			for(size_t i=0;i<closeVertices.size();i++)
			{
				morphTang[closeVertices[i]] = smoothTang;
				morphBitang[closeVertices[i]] = smoothBitang;
			}
		}
		log = true;
	}
	bool log;

public: //CalcTangentsProcess

	CalcTangentsProcess(Daedalus::post *p)
	:log(),maxAngle(std::max<double>(0,p->CalcTangentsProcess.configMaxAngle))
	,post(*p){}operator bool()
	{			
		PreSuppose(!PreMesh2::IsSupported);

		post.progLocalFileName("Calculating Tangents");

		post.Verbose("CalcTangentsProcess begin");

		preScene *scene = post.Scene();
		
		if(scene->_non_verbose_formatflag)
		{
			PreSuppose(post.MakeVerboseFormat.steps);
			post.CriticalIssue("Post-processing order mismatch: expecting pseudo-indexed (\"verbose\") vertices here");
			return false;
		}

		size_t sortIndex = 0;
		scene->MeshesCoList()^[&](preMesh *ea)
		{
			Process(ea,ea,sortIndex++);
			ea->MorphsCoList()^[&](preMorph *ea2){ Process(ea,ea2,sortIndex++); };
		};
		if(log)	post("CalcTangentsProcess finished. Tangents have been calculated");
		else post.Verbose("CalcTangentsProcess finished");
		return true;
	}
};
static bool CalcTangentsProcess(Daedalus::post *post)
{
	return class CalcTangentsProcess(post);
}							
Daedalus::post::CalcTangentSpace::CalcTangentSpace
(post *p):Base(p,p->steps&step?::CalcTangentsProcess:0)
{
	configMaxAngle = prePi/4; configSourceUV = 0;
}
