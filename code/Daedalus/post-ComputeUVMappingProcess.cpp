#include "Daedalus.(c).h"
using namespace Daedalus;
class ComputeUVMappingProcess //Assimp/ComputeUVMappingProcess.cpp
{
	Daedalus::post &post;

	preScene *const scene;

	struct MappingInfo
	{
		PreMaterial::Mapping type; pre3D axis; preN UVs;
		MappingInfo(PreMaterial::Mapping _type):type(_type),axis(0,1,0),UVs(){}
		inline bool operator==(const MappingInfo &cmp)const{ return type==cmp.type&&axis==cmp.axis;	}
	};	
	//AI: Try to remove UV seams 
	//From ComputeSphereMapping:
	//"Now find and remove UV seams. A seam occurs if a face has a texcoord
	//close to zero on the one side, and a texcoord close to one on the other side"
	static void RemoveUVSeams(const preMesh *mesh, pre3D *UVs)
	{
		//AI: TODO: just a very rough algorithm. I think it could be done
		//much easier, but I don't know how and am currently too tired to
		//to think about a better solution. (PRE: GET SOME SLEEP)
		const static double LOWER_LIMIT = 0.1;
		const static double UPPER_LIMIT = 0.9;
		const static double LOWER_EPSILON = 10e-3;
		const static double UPPER_EPSILON = 1.0-10e-3;					
		auto il = mesh->FacesList(); assert(mesh->HasIndices());
		for(size_t i=il;i<il.Size();i++) if(il[i].polytype>=PreFace::triangle)
		{
			auto l = mesh->IndicesSubList(il[i]);

			size_t small = l.Size(), large = small;
			bool zero = false, one = false, round_to_zero = false;
			//AI: Check whether this face lies on a UV seam. We can just guess,
			//but the assumption that a face with at least one very small
			//on the one side and one very large U coord on the other side
			//lies on a UV seam should work for most cases.
			for(size_t i=0;i<l.Size();i++)
			{
				if(UVs[l[i]].x<LOWER_LIMIT)
				{
					small = i;
					//AI: If we have a U value very close to 0 we can't round the others to 0, too.
					if(UVs[l[i]].x<=LOWER_EPSILON) zero = true;
					else round_to_zero = true;
				}
				if(UVs[l[i]].x>UPPER_LIMIT)
				{
					large = i;
					//AI: If we have a U value very close to 1 we can't round the others to 1, too.
					if(UVs[l[i]].x>=UPPER_EPSILON) one = true;
				}
			}
			if(small!=l.Size()&&large!=l.Size())
			{
				for(size_t i=0;i<l.Size();i++)
				{
					//AI: If the u value is over the upper limit and no other u value of that face is 0, round it to 0
					if(UVs[l[i]].x>UPPER_LIMIT&&!zero) UVs[l[i]].x = 0;
					//AI: If the u value is below the lower limit and no other u value of that face is 1, round it to 1
					else if(UVs[l[i]].x<LOWER_LIMIT&&!one) UVs[l[i]].x = 1;
					//AI: The face contains both 0 and 1 as UV coords. This can occur
					//for faces which have an edge that lies directly on the seam.
					//Due to numerical inaccuracies one U coord becomes 0, the
					//other 1. But we do still have a third UV coord to determine
					//to which side we must round to.
					else if(one&&zero)					
					if(round_to_zero&&UVs[l[i]].x>=UPPER_EPSILON) UVs[l[i]].x = 0;
					else if(!round_to_zero&&UVs[l[i]].x<=LOWER_EPSILON) UVs[l[i]].x = 1;
				}
			}
		}
	}	
	static bool IsBasicAxis(const pre3D &axis, double pre3D::*(&x), double pre3D::*(&y), double pre3D::*(&z))
	{
		if(axis.x>0.99){ x = &pre3D::x; y = &pre3D::y; z = &pre3D::z; return true; }
		if(axis.y>0.99){ x = &pre3D::y; y = &pre3D::z; z = &pre3D::x; return true; }
		if(axis.z>0.99){ x = &pre3D::z; y = &pre3D::x; z = &pre3D::z; return true; } return false;
	}
	void ComputeSphereMapping(const preMesh *mesh, const preMorph *morph, const pre3D &axis, pre3D *out)
	{
		pre3D center(0,0,0); //THE FOLLOWING SEEMS LIKE A BAD IDEA???
		//pre3D center, min, max; FindMeshCenter(morph,center,min,max);

		//REMINDER: only useful if un-rotated
		double pre3D::*x, pre3D::*y, pre3D::*z; //simplifying	
		//AI: If the axis is one of x,y,z run a faster code path. 
		//currently the mapping axis will always be one of x,y,z, except if the
		//PretransformVertices step is used (as it transforms the meshes into worldspace)
		if(IsBasicAxis(axis,x,y,z))
		{ 
			//AI: For each point get a normalized projection vector in the sphere,
			//get its longitude and latitude and map them to their respective
			//UV axes. Problems occur around the poles ... unsolvable.
			//
			//The spherical coordinate system looks like this:
			//x = cos(lon)*cos(lat)
			//y = sin(lon)*cos(lat)
			//z = sin(lat)
			//
			//Thus it can be derived:
			//lat = arcsin (z)
			//lon = arctan (y/x)
			auto uv = out; morph->PositionsCoList()^[&](const pre3D &ea)
			{
				const pre3D diff = (ea-center).Normalize();
				*uv++ = pre3D((atan2(diff.*z,diff.*y)+prePi)/(2*prePi),
				(std::asin(diff.*x)+prePi/2)/prePi,0);
			};
		}
		else //loader included mapping_axis??
		{
			//AI: again the same, except we're applying a transformation now
			auto trafo = PreQuaternion::Matrix::FromToMatrix(axis,pre3D(0,1,0));			
			auto uv = out; morph->PositionsCoList()^[&](const pre3D &ea)
			{
				const pre3D diff = ((trafo*ea)-center).Normalize();
				*uv++ = pre3D((atan2(diff.y,diff.x)+prePi)/(2*prePi),
				(std::asin(diff.z)+prePi/2)/prePi,0);
			};
		}		
		RemoveUVSeams(mesh,out);
	}
	void ComputeCylinderMapping(const preMesh *mesh, const preMorph *morph, const pre3D &axis, pre3D *out)
	{
		pre3D min(-0.5),max(0.5); //TODO: apply PreMaterial::Matrix
		pre3D center(0,0,0); //THE FOLLOWING SEEMS LIKE A BAD IDEA???
		//pre3D center, min, max; FindMeshCenter(morph,center,min,max);		

		//REMINDER: only useful if un-rotated
		double pre3D::*x, pre3D::*y, pre3D::*z; //simplifying	  
		//AI: If the axis is one of x,y,z run a faster code path. 
		//currently the mapping axis will always be one of x,y,z, except if the
		//PretransformVertices step is used (as it transforms the meshes into worldspace)
		if(IsBasicAxis(axis,x,y,z))   
		{	
			//AI: If the main axis is 'z', the z coordinate of a point 'p' is mapped
			//directly to the texture V axis. The other axis is derived from
			//the angle between ( p.x - c.x, p.y - c.y ) and (1,0), where
			//'c' is the center point of the morph.
			const double l_diff = 1/(max.*x-min.*x);
			auto uv = out; morph->PositionsCoList()^[&](const pre3D &ea)
			{
				uv->y = (ea.*x-min.*x)*l_diff;
				uv++->x = (atan2(ea.*z-center.*z,ea.*y-center.*y)+prePi)/(2*prePi);
			};
		}
		else //loader included mapping_axis??
		{
			//AI: again the same, except we're applying a transformation now
			auto trafo = PreQuaternion::Matrix::FromToMatrix(axis,pre3D(0,1,0));
			//THE FOLLOWING SEEMS LIKE A BAD IDEA???
			//FindMeshCenterTransformed(morph,center,min,max,mTrafo);
			const double l_diff = 1/(max.y-min.y);
			auto uv = out; morph->PositionsCoList()^[&](const pre3D &ea)
			{
				const pre3D pos = trafo*ea;	uv->y = (pos.y-min.y)*l_diff;
				uv++->x = (atan2(pos.x-center.x,pos.z-center.z)+prePi)/(2*prePi);
			};
		}		
		RemoveUVSeams(mesh,out);
	}
	void ComputePlaneMapping(const preMesh *mesh, const preMorph *morph, const pre3D& axis, pre3D *out)
	{
		pre3D center(0),min(-0.5),max(0.5);	
		//REMINDER: only useful if un-rotated
		double pre3D::*x, pre3D::*y, pre3D::*z; //simplifying	
		//AI: If the axis is one of x,y,z run a faster code path. It's worth the extra effort ...
		//currently the mapping axis will always be one of x,y,z, except if the
		//PretransformVertices step is used (it transforms the meshes into worldspace,
		//thus changing the mapping axis)
		if(IsBasicAxis(axis,x,y,z)) 
		{
			//THE FOLLOWING SEEMS LIKE A BAD IDEA???
			//FindMeshCenter(morph,center,min,max);
			double l_diffu = 1/(max.*z-min.*z), l_diffv = 1/(max.*y-min.*y);
			morph->PositionsCoList()^[&](const pre3D &ea)
			{
				out++->Set((ea.*z-min.*z)*l_diffu,(ea.*y-min.*y)*l_diffv,0);
			};
		}		
		else //loader included mapping_axis??
		{
			//AI: again the same, except we're applying a transformation now
			auto trafo = PreQuaternion::Matrix::FromToMatrix(axis,pre3D(0,1,0));
			//THE FOLLOWING SEEMS LIKE A BAD IDEA???
			//FindMeshCenterTransformed(morph,center,min,max,trafo);
			double l_diffu = 1/(max.x-min.x), l_diffv = 1/(max.z-min.z);
			morph->PositionsCoList()^[&](const pre3D &ea)
			{
				out++->Set((ea.x-min.x)*l_diffu,(ea.z-min.z)*l_diffv,0);
			};
		}		
	}
	void ComputeBoxMapping(const preMesh *mesh, const preMorph*, const pre3D &axis, pre3D *out)
	{
		post.CriticalIssue("Mapping type currently not implemented. UVs will be (0,0)");
		memset(out,0x00,sizeof(*out)*mesh->Positions());
	}
	void UnrecognizedMapping(const preMesh *mesh, const preMorph*, const pre3D &axis, pre3D *out)
	{
		post.CriticalIssue("This mapping ID is not recognized??? Its UVs will be (0,0)");
		memset(out,0x00,sizeof(*out)*mesh->Positions());
	}	
	template<typename F> //VS2010's decltype fails here, still template is probably better
	void Compute(F f, const preMesh *mesh, preMorph *morph, const pre3D &axis, preN UVs)	
	{
		if(!morph->HasPositions()) return;
		PreSuppose(!PreMesh2::IsSupported);
		morph->TextureCoordsCoList(UVs).Reserve(morph->Positions());
		return (this->*f)(mesh,morph,axis,mesh->texturecoordslists[UVs]); 
	}	
	std::vector<MappingInfo> mappingStack; PreSet<preID,-1> matPrev;
	void Process(preMaterial *mat, PreMaterial::Property *prop, preID matID)
	{
		if(mat->_mapping!=prop->key) return;		
		auto &mapping = prop->Datum(mat->mapping);
		if(mat->texturecoords2D==mapping) return;
				
		post("Found non-UV mapped texture (")<<post.LogString(prop->TextureCategory())
		<<" #"<<prop->TextureNumber()<<"). Mapping type: "<<post.LogString(mapping);
														
		MappingInfo info(mapping);
		mat->Get(mat->mapping_axis,prop->TextureCategory(),prop->TextureNumber(),info.axis);

		//AI: share UVs aamong similarly mapped textures
		if(matID!=matPrev){ matPrev = matID; mappingStack.clear(); }
		size_t UVs = 0;	auto shared = std::find(mappingStack.begin(),mappingStack.end(),info);
		if(shared!=mappingStack.end())
		{
			UVs = shared->UVs; //can be shared
		}
		else //AI: found a non-UV mapped texture
		{
			auto l = scene->MeshesCoList();	
			for(size_t i=l;i<l.Size();i++) if(matID==l[i]->material)
			{
				preMesh *mesh = l[i]; if(!mesh->Positions()) continue;

				size_t outUVs = 0; //AI: Find an empty UV channel within the mesh
				while(outUVs<PreMesh::texturecoordslistsN&&mesh->texturecoordslists[outUVs]) outUVs++;
				if(outUVs>=PreMesh::texturecoordslistsN) 
				{
					post.CriticalIssue("Cannot generate UVs because the internal limit (")<<outUVs<<") was met";
					continue; 
				}								
				auto f = &ComputeUVMappingProcess::UnrecognizedMapping;
				switch(mapping) 
				{
				case PreMaterial::orthomapped: 
				f = &ComputeUVMappingProcess::ComputePlaneMapping; break;
				case PreMaterial::spheremapped:
				f = &ComputeUVMappingProcess::ComputeSphereMapping; break;
				case PreMaterial::tubemapped:
				f = &ComputeUVMappingProcess::ComputeCylinderMapping; break;								
				case PreMaterial::cubemapped: 
				f = &ComputeUVMappingProcess::ComputeBoxMapping; break;								
				default: assert(0);
				}
				Compute(f,mesh,mesh,info.axis,outUVs);
				mesh->MorphsCoList()^[=](preMorph *ea){ Compute(f,mesh,ea,info.axis,outUVs); };
				if(UVs!=outUVs&&mesh!=l.Front()) post <<
				"UV index mismatch. Not all meshes assigned to "
				"this material have equal numbers of UV channels. The UV index stored in  "
				"the material structure does therefore not apply for all meshes. ";								
				UVs = outUVs;
			}
			info.UVs = UVs; mappingStack.push_back(info);
		}
		//AI: update the mapping mode
		mapping = mat->texturecoords2D;							
		mat->InsertProperty(info.UVs,mat->sourceslist,prop->TextureCategory(),prop->TextureNumber());
	}
	
public: //ComputeUVMappingProcess

	ComputeUVMappingProcess(Daedalus::post *p)
	:scene(post.Scene())
	,post(*p){}operator bool()
	{
		PreSuppose(!PreMesh2::IsSupported);

		post.progLocalFileName("Generating UVs");

		post.Verbose("GenUVCoordsProcess begin");

		if(scene->_non_verbose_formatflag) //noting requirement
		{
			PreSuppose(post.MakeVerboseFormat.steps);
			post.CriticalIssue("Post-processing order mismatch: expecting pseudo-indexed (\"verbose\") vertices here");
			return false;
		}

		size_t matID = 0; scene->MaterialsList()^[&](preMaterial *ea)
		{ ea->PropertiesList()^[&](preMaterial::Property *ea2){ Process(ea,ea2,matID); }; matID++; };		
		
		post.Verbose("GenUVCoordsProcess finished");		
		return true;
	}	 
};
static bool ComputeUVMappingProcess(Daedalus::post *post)
{
	return class ComputeUVMappingProcess(post);
}							
Daedalus::post::GenUVCoords::GenUVCoords
(post *p):Base(p,p->steps&step?::ComputeUVMappingProcess:0)
{
	//Assimp does not do this, but the implication is that all UVs are required
	if(p->steps&TransformUVCoords::step) procedure = ::ComputeUVMappingProcess;
}
