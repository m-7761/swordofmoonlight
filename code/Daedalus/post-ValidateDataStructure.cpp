#include "Daedalus.(c).h"  
using namespace Daedalus;
class ValidateDataStructure //Assimp/ValidateDataStructure.cpp
{	
	Daedalus::post &post; 

	preScene *const scene;

	//////////////////////////////////////////////////////////////////////
	//FOR THE MOST PART, this file works with the low-level data members//
	//////////////////////////////////////////////////////////////////////
														
	struct Counter : PreSet<unsigned long long,0> //syntactic sugar
	{
		inline void operator+=(PreServer::LogStream&){ set++; } 

	}warnings, errors; 
	
	PreNew<char[]>::Vector aliasedbuffer;
	template<class T, int i> T *AliasedBuffer(preN n)
	{
		PreSuppose(i==0); aliasedbuffer.Reserve(n*sizeof(T));	
		return (T*)memset(aliasedbuffer.Pointer(),0,sizeof(T)*n);
	}
	std::string identifier;
	const char *IdString(const char *id, preN i=preMost, const char *m="")
	{
		if(i==preMost) return !*m?id:((identifier=id)+m).c_str();
		return (((((identifier=id)+="list[")+=i)+="]")+m).c_str();
	}
	inline bool ValidateN(preN n, preN most, const char *id)
	{
		if(n<most) return true;
		errors+=post.CriticalIssue(id)<<" is invalid (value: "<<n<<", out of: "<<most<<")";
		return false;
	}	
	inline bool ValidateN(preN n, preN most, const char *id, preN i, const char *dot="")
	{
		return ValidateN(n,most,IdString(id,i,dot));
	}				
	void ValidateString(const preX &string, const char *id, preN i=preMost)
	{
		if(string.length>PreX::lengthN) //ValidateN
		if(post.ValidateDataStructure.configWarnOfAssimpErrors)
		warnings+=post("Warning: ")<<IdString(id,i)<<".length exceeds Assimp maximum string limit (value: "<<string.length<<", out of: "<<PreX::lengthN<<")";
		if(string.HasString())
		for(const char *p=string.cstring,*d=p+PreX::lengthN;;p++) if('\0'==*p)
		{
			if(string.length!=p-string.cstring)
			errors+=post.CriticalIssue(IdString(id,i))<<".cstring is invalid: the 0 terminator precedes the expected length (PreX::length)";
			break;
		}
		else if(p>=d)
		{
			errors+=post.CriticalIssue(IdString(id,i))<<".cstring is invalid: there is no 0 terminator";
			break;
		}
	}		
	inline void ValidateIDs(preN size, const preID *list, preN most, const char *id)
	{
		if(ValidateList(size,list,id)) for(preN i=0;i<size;i++) ValidateN(list[i],most,id,i);
	}
	inline void ValidateUniqueIDs(preN size, const preID *list, preN most, const char *id)
	{			
		if(!ValidateList(size,list,id)) return;
		auto boolvec = AliasedBuffer<bool,false>(most);
		for(preN i=0;i<size;i++) if(ValidateN(list[i],most,id,i))
		{
			if(boolvec[list[i]]) 
			errors+=post.CriticalIssue(id)<<"list["<<i<<"] is duplicates this ID (value: "<<list[i]<<")";
			boolvec[list[i]] = true;
		}
	}
	template<class T> inline void ValidateMetaData(const T *t, const char *id)
	{
		if(scene->metadata)	ValidateIDs(t->metadata,t->metadatalist,scene->metadata,id);
	}
	template<class T> static inline const char *listN(){ return "list"; }
	template<> static inline const char *listN<PreMesh2*>(){ return "list2"; }
	template<class T> inline const char *CheckListN(T *ptr){ return listN<T>(); }
	template<class T> inline bool ValidateList(preN size, T *ptr, const char *id)
	{
		if(size>0&&ValidateN(size,preMost,id)&&ptr) return true;
		if(ptr&&!size) warnings+=post("Warning: ")<<listN<T>()<<" is not 0, but "<<id<<" is 0";
		if(!ptr&&size) errors+=post.CriticalIssue(id)<<listN<T>()<<" is 0, but "<<id<<" is "<<size; 		
		return false;
	}
	template<class T> inline bool DoValidation(preN size, T *list, const char *id)
	{
		if(!ValidateList(size,list,id)) return false;
		for(preN i=0;i<size;i++) if(!list[i]) //AI: validate all entries
		errors+=post.CriticalIssue(id)<<listN<T>()<<"["<<i<<"] is 0 ("<<id<<" is "<<size<<")";
		else{ post.Verbose("Validating ")<<id<<listN<T>()<<"["<<i<<"]"; Validate(list[i]); } return true;
	}
	void CheckForNode(const preX &node, const char *id, preN i=preMost, const char *m="")
	{
		int counter = 0; scene->rootnode->ForEach([&](const preNode *ea){ if(ea->name==node) counter++; });
		if(!counter) errors+=post.CriticalIssue(IdString(id,i,m))<<" has no corresponding node in the scene graph ("<<node.String()<<")";			
		if(1<counter) errors+=post.CriticalIssue(IdString(id,i,m))<<": there are more than one nodes with "<<node.String()<<" as name";
	}
	void CheckForMesh(const preX &mesh, const char *id)
	{
		int counter = 0; scene->MeshesList()^[&](const preMesh *ea){ if(ea->name==mesh) counter++; }; 
		if(!counter) scene->MeshesList2()^[&](const preMesh *ea){ if(ea->name==mesh) counter++; }; 
		if(!counter) errors+=post.CriticalIssue(id)<<" has no corresponding mesh in the scene graph ("<<mesh.String()<<")";			
	}	
	template<class T> inline bool DoValidationWithNodeCheck(preN size, T **list, const char *id)
	{
		if(!DoValidation(size,list,id)) return false;
		//NOTE: this Warning applies to bones, cameras & lights 
		//Assimp may remove this constraint as it's unnecessary
		if(post.ValidateDataStructure.configWarnOfAssimpErrors)
		for(preN i=0;i<size;i++) if(list[i]) //AI: look for duplicate "names"
		for(preN j=0;j<size;j++) if(list[j]) if(list[i]->node==list[j]->node)
		warnings+=post("Warning: ")<<id<<"list["<<i<<"] has the same node as "<<id<<"list["<<j<<"]";		
		for(preN i=0;i<size;i++) CheckForNode(list[i]->node,id,i,"->node"); return true;
	}
	void Validate(const preNode *node)
	{
		ValidateMetaData(node,"PreNode::metadata");
		ValidateString(node->name,"PreNode::name");

		ValidateUniqueIDs(node->meshes,node->mesheslist,scene->meshes,"PreNode::meshes");
		
		if(!node->node&&node!=scene->rootnode)
		errors+=post.CriticalIssue("A node has no valid parent (PreNode::node is 0)");
		DoValidation(node->nodes,node->nodeslist,"PreNode::nodes");
	}
	void Validate(const preMesh2 *mesh)
	{
		#ifdef NDEBUG
		//assert(!"Validate(preMesh2*) is not implemented");
		#error Validate(preMesh2*) is not implemented
		#endif
	}
	void Validate(const preMesh *mesh)
	{
		ValidateMetaData(mesh,"PreMesh::metadata");
		ValidateString(mesh->name,"PreMesh::name");		
		ValidateN(mesh->material,scene->materials,"PreMesh::material");				
		if(ValidateList(mesh->faces,mesh->faceslist,"PreMesh::faces"))		
		for(preN i=0;i<mesh->faces;i++)
		{
			preFace &face = mesh->faceslist[i]; if(!face) continue;
			if(ValidateN(face.polytypeN,mesh->indices,"PreMesh::polytypeN"))
			if(post.ValidateDataStructure.configWarnOfAssimpErrors&&face.polytypeN>0x7FFF)
			warnings+=post("Warning: PreMesh::polytypeN exceeds Assimp limit (value: ")<<face.polytypeN<<", out of: 32767)";
			ValidateN(face.startindex,mesh->indices-face.polytypeN,"PreMesh::faces",i,".startindex");
			//debugging: self-testing after post-processing?
			if(mesh->_polytypesflags) switch(face.polytypeN) 
			{
			case 0:	assert(0); //PreFace::start?					
			errors+=post.CriticalIssue("PreMesh::faceslist[")<<i<<"].polytype is PreFace::start. This is not supported by this client."; 
			break; case 1: if(!mesh->_pointsflag)
			errors+=post.CriticalIssue("PreMesh::faceslist[")<<i<<"].polytype is PreFace::point, yet PreMesh::_pointsflag is unset.";
			break; case 2: if(!mesh->_linesflag)
			errors+=post.CriticalIssue("PreMesh::faceslist[")<<i<<"].polytype is PreFace::line, yet PreMesh::_linesflag is unset.";
			break; case 3: if(!mesh->_trianglesflag)
			errors+=post.CriticalIssue("PreMesh::faceslist[")<<i<<"].polytype is PreFace::triangle, yet PreMesh::_trianglesflag is unset.";
			break; default: if(!mesh->_polygonsflag)
			errors+=post.CriticalIssue("PreMesh::faceslist[")<<i<<"].polytypeN is greater than 3, yet PreMesh::_polygonsflag is unset.";	
			};			
		}
		else if(post.ValidateDataStructure.configWarnOfAssimpErrors) 
		{
			warnings+=post("Warning: Mesh contains no point, line, or face primitives");
		}		
		if(ValidateList(mesh->indices,mesh->indiceslist,"PreMesh::indices"))
		{
			preN most = mesh->positions; 
			auto boolvec = AliasedBuffer<bool,false>(most);
			if(most) for(preN i=0;i<mesh->indices;i++) 
			if(ValidateN(mesh->indiceslist[i],most,"PreMesh::indices",i)) boolvec[most] = true;
			for(preN i=0;i<most;i++) if(!boolvec[i])
			{ warnings+=post("Warning: There are unreferenced vertex attributes"); break; }
		}
		if(ValidateList(mesh->positions,mesh->positionslist,"PreMesh::positions"))
		{
			//SEEMS TO CONTRADICT ScenePreprocessor.cpp
			//AI: if tangents are there there must also be bitangent vectors
			//if((!mesh->tangentslist)!=(!mesh->bitangentslist))
			//errors+=post.CriticalIssue("If there are tangents, bitangent vectors must be present as well");		
			//INSTEAD, MAY AS WELL:
			if(mesh->bitangentslist&&!mesh->tangentslist)
			errors+=post.CriticalIssue("PreMesh::bitangentslist is nonzero, while PreMesh::tangentslist is 0");		
			for(preN i=0;i<PreMesh::texturecoordslistsN;i++)
			if(!mesh->texturecoordslists[i])
			for(i++;i<PreMesh::texturecoordslistsN;i++)	
			if(mesh->texturecoordslists[i])
			errors+=post.CriticalIssue("PreMesh::texturecoordslists[")<<i<<"] is not 0, although a prior element was";
			for(preN i=0;i<PreMesh::colorslistsN;i++)
			if(!mesh->colorslists[i])
			for(i++;i<PreMesh::colorslistsN;i++)	
			if(mesh->colorslists[i])
			errors+=post.CriticalIssue("PreMesh::colorslists[")<<i<<"] is not 0, although a prior element was";
		}
		else errors+=post.CriticalIssue("The mesh contains no position attributes"); //AI: positions must exist

		if(DoValidationWithNodeCheck(mesh->bones,mesh->boneslist,"PreMesh::bones"))
		{	
			preN most = mesh->positions;
			auto floatvec = AliasedBuffer<float,0>(most);
			for(preN i=0;i<mesh->bones;i++) 
			{
				const preBone *bone = mesh->boneslist[i]; if(!bone) continue;
				signed prev = -1; bool disordered = false;
				auto il = bone->WeightsList(); for(preN i=il;i<il.Size();prev=il[i++])
				{
					if(il[i]<=prev) if(!disordered++)
					errors+=post.CriticalIssue("PreBone::weighlist ")
					<<(prev==il[i]?"contains duplicate entries":"is not sorted in ascending order. Consider WeightsList().IsSorted()");					
					if(ValidateN(il[i],most,"PreBone::weights",i,".key"))
					{
						float w = il[i].value; floatvec[il[i]]+=w; if(w<=0||w>1)
						warnings+=post("Warning: PreBone::weightslist[")<<i<<"].value is outside (0,1]";
					}
				}
			}//AI: see if weight sets sum to 1
			for(preN i=0;i<mesh->positions;i++)
			if(floatvec[i]&&(floatvec[i]<=0.94f||floatvec[i]>=1.05f))				
			{
				warnings+=post("Warning: PreMesh::positionslist[")<<i<<"]: 1 does not approximate weighted sum of bones ("<<floatvec[i]<<")";
				if(errors) post("Prior warning can be the result of complications due to previously reported errors");
			}
		}		
		if(DoValidation(mesh->morphs,mesh->morphslist,"PreMesh::morphs"))
		{
			for(preN i=0;i<mesh->morphs;i++) 
			{
				const preMorph *morph = mesh->morphslist[i];
				
				//Reminder: SimplifyProcess can result in empty morphs
				//(since it does not attempt to remove morph animation keys)
				if(!morph||!morph->positions) continue;

				if(morph->positions!=mesh->positions)
				errors+=post.CriticalIssue("PreMesh::morphslist[")<<i<<"]->positions does not equal PreMesh::positions";
				#define _(x,...) if(morph->x&&!mesh->x)\
				errors+=post.CriticalIssue("PreMesh::morphslist[")<<i<<"]->"<<#x<<#__VA_ARGS__<<" is not 0, but PreMesh::"<<#x<<#__VA_ARGS__<<" is 0";
				_(positions,)_(normalslist,)_(tangentslist,)_(bitangentslist,)
				PreSuppose(8==PreMesh::colorslistsN&&8==PreMesh::texturecoordslistsN);
				_(colorslists,[0])_(colorslists,[1])_(colorslists,[2])_(colorslists,[3])
				_(colorslists,[4])_(colorslists,[5])_(colorslists,[6])_(colorslists,[7])
				_(texturecoordslists,[0])_(texturecoordslists,[1])_(texturecoordslists,[2])_(texturecoordslists,[3])
				_(texturecoordslists,[4])_(texturecoordslists,[5])_(texturecoordslists,[6])_(texturecoordslists,[7])
				#undef _			
			}
		}
		if(mesh->_complex)
		errors+=post.CriticalIssue("PreMesh::_complex of level 1 mesh is not 0");
	}
	void Validate(const preBone *bone)
	{
		ValidateMetaData(bone,"PreBone::metadata");
		ValidateString(bone->node,"PreBone::node");
		if(!ValidateList(bone->weights,bone->weightslist,"PreBone::weights")) 
		errors+=post.CriticalIssue("The bone contains no vertex weights");
	}
	void Validate(const preMorph *morph)
	{
		ValidateMetaData(morph,"PreMorph::metadata");
		if(morph->_complex)
		errors+=post.CriticalIssue("PreMorph::_complex of level 1 morph is not 0");
	}
	void Validate(const preMaterial *mat)
	{
		//AI: check whether there are material keys that are obviously not legal
		if(!DoValidation(mat->_properties,mat->_propertieslist,"PreMaterial::_properties"))
		{
			errors+=post.CriticalIssue("Validation cannot continue, as PreMaterial::_propertieslist is empty or invalid");
			assert(0); return;
		}
		if(!mat->IsSorted()) //NOTE: the copy-constructor sorts itself afterward
		{
			errors+=post.CriticalIssue("Programmer error: validation cannot continue, as PreMaterial::IsSorted() is negative");
			assert(0); return;
		}
		
		int i; float f; preN n; pre4D color;
		if(mat->PairKey_Get(mat->_lost_in_copy,i))
		warnings+=post("Warning: ")<<i<<" material properties were lost in translation:\n"
		<<"This can suggest that this client is older than the preservation server--or, PreServer"; 
		if(mat->PairKey_Get(mat->_hole_in_copy,i))
		errors+=post.CriticalIssue(i)<<" elements of the PreMaterial::_propertieslist array were 0:\n"
		<<"It's too late to say which ones; most likely they had to be discarded by the copy-constructor"; 

		//AI: make some more specific tests		
		auto kv = mat->PairKey(mat->shadingmodel);
		if(kv) switch(kv->Datum(mat->shadingmodel))
		{
		case mat->Blinn: case mat->CookTorrance: case mat->Phong: 
		if(!mat->PairKey(mat->shininess))
		warnings+=post("Warning: a specular shading model is specified but $ai:mat.shininess$ is not");
		if(mat->PairKey_Get(mat->shininess_strength,f)&&!f)
		warnings+=post("Warning: a specular shading model is specified but $ai:mat.shinpercent$ is 0");			
		}
		if(mat->PairKey_Get(mat->transparency,f)&&(f<=0||f>1))
		warnings+=post("Warning: $ai:mat.opacity$ is outside (0,1]");	

		//Reminder: loaders should not pre-modulate; still this is more error than warning
		#define _(x) if(mat->PairKey_Get(mat->x##list,n)&&mat->PairKey_Get(mat->x,color))\
		warnings+=post("SOFT-ERROR: $pre:clr."#x"list$ and $ai:clr."#x"$ aren't supposed to coexist (or should they?!)");
		_(diffuse)_(ambient)_(specular)_(emission)_(transparent)_(reflective)
		#undef _

		//TODO? HERE ASSIMP WANTS EACH TEXTURE TO HAVE AN IMAGE FILE NAME. SEEMS LIKE A WARNING AT MOST

		auto l = mat->PropertiesSubList(0);	
		PreMaterial::Texture t,tex = mat->_nontexture;
		for(preN texN=0,i=0;l.Size();l=mat->PropertiesSubList(i+=l.Size()))
		{
			t = l.Front()->TextureCategory(); n = l.Front()->TextureNumber();			
			if(tex!=t){ tex = t; texN = 0; } if(texN++==n) continue; else texN = n+1; 
			errors+=post.CriticalIssue(post.LogString(tex))<<"textures "<<texN-1<<" through "<<n-1<<" are missing";
		}
	}
	void Validate(const PreMaterial::Property *prop)
	{
		ValidateMetaData(prop,"PreMaterial::Property::metadata");
		//NOTE: WON'T GET PAST PreMaterial COPY-CONSTRUCTOR
		if(!prop->HasData()) //paranoia? Assimp checks this		
		errors+=post.CriticalIssue("PreMaterial::Property::_datalist should not be 0 under any circumstances")
		<<" (permanent ID is $"<<prop->key().permanentID<<"$)";		
		else if(!prop->Data()) //Assimp minimum buffer check
		errors+=post.CriticalIssue("PreMaterial::Property::_sizeofdatalist is too small to fill builtin type")
		<<" (permanent ID is $"<<prop->key().permanentID<<"$)";
	}		
	void Validate(const preAnimation *anim)
	{
		ValidateMetaData(anim,"PreAnimation::metadata");
		ValidateString(anim->name,"PreAnimation::name");				
		//AI: ScenePreprocessor will compute the duration if still the default value
		//(Aramis) Add small epsilon, comparison tended to fail if max_time == duration,
		//seems to be due the compilers register usage/width.
		ValidateTimeLimit = anim->duration>0?anim->duration+0.001:DBL_MAX; 
		int animated = 0; preN size; const char *id;
		//Reminder: DoValidationWithNodeCheck yields warnings, these yield errors
		if(DoValidation(size=anim->skinchannels,anim->skinchannelslist,id="PreAnimation::skinchannels"))
		{
			animated++; 
			auto list = anim->skinchannelslist; //AI: check for duplicate nodes
			for(preN i=0;i<size;i++) if(list[i]) for(preN j=0;j<size;j++) if(list[j]) 
			{
				if(list[i]->node==list[j]->node)
				errors+=post.CriticalIssue(id)<<"list["<<i<<"] animates the same node as "<<id<<"list["<<j<<"]";		
				if(list[i]->appendednodes&&list[j]->appendednodes) 
				CheckAppendedNodes(list[i]->AppendedNodesList(),list[j]->AppendedNodesList(),id,i,j,"appendednodes");
			}
		}
		if(DoValidation(size=anim->morphchannels,anim->morphchannelslist,id="PreAnimation::morphchannels"))
		{
			animated++;
			auto list = anim->morphchannelslist; //AI: check for duplicate meshes
			for(preN i=0;i<size;i++) if(list[i]) //(and then those meshes' nodes)
			for(preN j=0;j<size;j++) if(list[j]) if(list[i]->mesh==list[j]->mesh) 
			{
				if(!list[i]->HasAffectedNodes()||!list[j]->HasAffectedNodes())
				errors+=post.CriticalIssue(id)<<"list["<<i<<"] animates the same mesh as "<<id<<"list["<<j<<"]";		
				else //same mesh, so if affected nodes overlap, generate error
				CheckAppendedNodes(list[i]->AffectedNodesList(),list[j]->AffectedNodesList(),id,i,j,"affectednodes");				
			}
		}
		if(animated==0&&post.ValidateDataStructure.configWarnOfAssimpErrors)
		if(post.interestedIn.skinChannels||post.interestedIn.morphChannels) //suppressing
		warnings+=post("Warning: there is no animation data");
	}
	void Validate(const PreAnimation::Skin *skin)
	{
		ValidateMetaData(skin,"PreAnimation::Skin::metadata");

		const char *id = "PreAnimation::Skin::node";
		ValidateString(skin->node,id); CheckForNode(skin->node,id); 
		id = "PreAnimation::Skin::appendednodes";
		if(ValidateList(skin->appendednodes,skin->appendednodeslist,id))
		for(preN i=0;i<skin->appendednodes;i++) 
		{ ValidateString(skin->appendednodeslist[i],id,i); CheckForNode(skin->appendednodeslist[i],id,i); }

		int srp = 0;		
		srp+=!ValidateTime(skin->scalekeys,skin->scalekeyslist,"PreAnimation::Skin::scalekeys");
		srp+=!ValidateTime(skin->rotationkeys,skin->rotationkeyslist,"PreAnimation::Skin::rotationkeys");
		srp+=!ValidateTime(skin->positionkeys,skin->positionkeyslist,"PreAnimation::Skin::positionkeys");		
		if(srp==3) errors+=post.CriticalIssue("Empty skin animation channel");
	}		 		
	void Validate(const PreAnimation::Morph *morph)
	{	
		ValidateMetaData(morph,"PreAnimation::Morph::metadata");

		const char *id = "PreAnimation::Morph::mesh";
		ValidateString(morph->mesh,id); CheckForMesh(morph->mesh,id); 
		id = "PreAnimation::Morph::affectednodes";
		if(ValidateList(morph->affectednodes,morph->affectednodeslist,id))
		for(preN i=0;i<morph->affectednodes;i++) 
		{ ValidateString(morph->affectednodeslist[i],id,i); CheckForNode(morph->affectednodeslist[i],id,i); }

		if(!ValidateTime(morph->morphkeys,morph->morphkeyslist,"PreAnimation::Morph::morphkeys"))		
		errors+=post.CriticalIssue("Empty morph animation channel");
	}
	double ValidateTimeLimit; template<class T>
	inline bool ValidateTime(preN size, const PreTime<T> *ptr, const char *id)
	{
		double prev = -10e10;
		if(ValidateList(size,ptr,id)) for(preN i=0;i<size;i++)
		{
			if(ptr[i]>ValidateTimeLimit)				
			errors+=post.CriticalIssue(id)<<"list["<<i<<"].key ("<<+ptr[i]<<") is larger than PreAnimation::duration (which is "<<ValidateTimeLimit<<")";
			//NOTE: Assimp reports Warning this time. Daedalus cannot
			if(i!=0&&ptr[i]<=prev) 
			errors+=post.CriticalIssue(id)<<"list["<<i<<"].key ("<<+ptr[i]<<") is smaller than "<<id<<"list["<<i-1<<"] (which is "<<prev<<")";				
			prev = ptr[i];
		}
		else return false; return true;
	}
	void CheckAppendedNodes(const PreX::List il, const PreX::List jl, const char *id, preN ii, preN jj, const char *m)
	{
		for(preN i=il;i<il.Size();i++) for(preN j=jl;j<jl.Size();j++) if(il[i]==jl[j])
		errors+=post.CriticalIssue(id)<<"list["<<ii<<"]->"<<m<<"list["<<i<<"] animates the same node as "<<id<<"list["<<jj<<"]->"<<m<<"list["<<j<<"]";		
	}
	void Validate(const preLight *light)
	{
		ValidateMetaData(light,"PreLight::metadata");
		ValidateString(light->node,"PreLight::node");

		if(!light->shape)
		warnings+=post("Warning: PreLight::shape is 0");
		if(!light->constattenuate&&!light->linearattenuate&&!light->squareattenuate) 
		warnings+=post("Warning: PreLight::constattenuate, etc. are all 0");
		if(light->innercone>light->outercone)
		errors+=post.CriticalIssue("PreLight::innercone is larger than PreLight::outercone");
		if(light->diffusecolor.IsBlack()&&light->ambientcolor.IsBlack()&&light->specularcolor.IsBlack())
		warnings+=post("Warning: PreLight::diffusecolor, etc. are all 0 in the 8-bit sRGB color space");		
	}
	void Validate(const preCamera *camera)
	{
		ValidateMetaData(camera,"PreCamera::metadata");
		ValidateString(camera->node,"PreCamera::node");
		
		if(camera->farclipdistance<camera->clipdistance)
		errors+=post.CriticalIssue("PreCamera::farclipdistance cannot be less than PreCamera::clipdistance");

		//AI: there are many 3DS files with invalid FOVs. No reason to reject them all
		if(camera->halfofwidth_angleofview<=0||camera->halfofwidth_angleofview>=prePi)
		warnings+=post("Warning: PreCamera::halfofwidth_angleofview is outside (0,")<<prePi<<")";
	}
	void Validate(const preTexture *texture)
	{
		ValidateMetaData(texture,"PreTexture::metadata");
				
		int data = 0; //AI: the data section may NEVER be 0
		if(ValidateList(texture->colors,texture->colorslist,"PreTexture::colors"))
		{
			data++;			
			if(texture->colors!=texture->width*texture->height)
			errors+=post.CriticalIssue("PreTexture::colors should be equal to the area of PreTexture::width by PreTexture::height");			
			if(*(char32_t*)texture->assimp3charcode_if0height)
			errors+=post.CriticalIssue("PreTexture::assimp3charcode_if0height should be 0,0,0,0 when accompanying decoded image data");			
		}
		if(ValidateList(texture->colordata,texture->colordatalist,"PreTexture::colordata"))
		{
			data++;
			if(texture->height)
			errors+=post.CriticalIssue("Assimp expects PreTexture::height to be 0 for encoded image data");
			if(texture->width!=texture->colordata)
			errors+=post.CriticalIssue("Assimp expects PreTexture::width to match PreTexture::colordata for encoded image data");
			if('\0'!=texture->assimp3charcode_if0height[3]) //NOTE: Assimp reports Warning
			errors+=post.CriticalIssue("PreTexture::assimp3charcode_if0height must be zero-terminated");			
			if('.'==texture->assimp3charcode_if0height[0]) //NOTE: Assimp reports Warning
			errors+=post.CriticalIssue("PreTexture::assimp3charcode_if0height should contain a file extension without a leading \"dot\" character");			
			const char *sz = texture->assimp3charcode_if0height;
			if((sz[0]>='A'&&sz[0]<='Z')||(sz[1]>='A'&&sz[1]<='Z')||(sz[2]>='A'&&sz[2]<='Z'))
			errors+=post.CriticalIssue("PreTexture::assimp3charcode_if0height contains uppercase letters");
		}
		if(0==data) errors+=post.CriticalIssue("There is no image data");
		if(2==data) errors+=post.CriticalIssue("There is BOTH encoded and decoded image data. There can only be one or the other");		
	}
	void DoRecursiveValidation(PreNode *rootnode, const char *id)
	{
		if(!rootnode) return; //NEW: will be added by ScenePreprocessor.cpp
	
		post.Verbose("Validating ")<<id; Validate(rootnode); //DoValidation
	}
	void Validate(const char *pooledstring){} //satisfy DoValidation

public: //ValidateDataStructure

	ValidateDataStructure(Daedalus::post *p)
	:scene(p->Scene())
	,post(*p){}operator bool()
	{	
		post.progLocalFileName("Validating Data Structure");

		post.Verbose("ValidateDataStructureProcess begin");

		ValidateString(scene->name,"PreScene::name");
		DoRecursiveValidation(scene->rootnode,"PreScene::rootnode");
		DoValidation(scene->meshes,scene->mesheslist,"PreScene::meshes");
		assert('2'==CheckListN(scene->mesheslist2)[4]); //sanity check
		DoValidation(scene->meshes,scene->mesheslist2,"PreScene::meshes");
		DoValidation(scene->materials,scene->materialslist,"PreScene::materials");		
		DoValidation(scene->animations,scene->animationslist,"PreScene::animations");				
		DoValidation(scene->embeddedtextures,scene->embeddedtextureslist,"PreScene::embeddedtextures");		
		DoValidationWithNodeCheck(scene->lights,scene->lightslist,"PreScene::lights");		
		DoValidationWithNodeCheck(scene->cameras,scene->cameraslist,"PreScene::cameras");
		PreSuppose(!PreMeta::IsSupported);
		//DoValidation(scene->metadata,scene->metadatalist,"PreScene::metadata");
		DoValidation(scene->pooledstrings,scene->pooledstringslist,"PreScene::pooledstrings");
		
		//NEW: check for inconsistent vertices
		auto jl = PreConst(scene)->MeshesList();
		for(preN i=0,n=scene->Materials();i<n;i++)
		for(preN vf=0,vj,j=jl;j<jl.Size();j++) if(jl[j]&&i==jl[j]->material) if(vf)
		{
			if(vf!=jl[j]->GetMeshVFormatUnique()) 
			errors+=post.CriticalIssue("PreScene::materialslist[")<<i<<"] vertices are inconsistent:\n"
			"PreScene::mesheslist["<<j<<"] differs from the first mesh using this material, thats index is "<<vj;
		}
		else vf = jl[vj=j]->GetMeshVFormatUnique();			
		
		if(errors||warnings&&post.ValidateDataStructure.configWarningsAreErrors)
		{
			if(post.ValidateDataStructure.configWarningsAreErrors)
			{
				post("Treating Warnings as Error, as configured");
				post.Verbose("ValidateDataStructureProcess reported ")<<+errors+warnings<<" combined errors, "<<+warnings<<" of which were warnings.";
			}
			else post.Verbose("ValidateDataStructureProcess reported ")<<+errors<<" errors and "<<+warnings<<" warnings.";
			if(!post.ValidateDataStructure.configProceedWithErrors)
			{
				post("As there are errors, processing is unable to complete. Considering disabling validation, or ignoring errors.");
				return false;
			}			
			//IT'S TOO LATE TO WARN ABOUT THE LIKELIHOOD OF CRASHES AT THIS POINT
			post("Proceeding as if there is no error, as configured. Aborting.");			
			post("ValidateDataStructureProcess aborted");
			return true;
		}
		else if(warnings)
		post("ValidateDataStructureProcess finished with ")<<+warnings<<" warnings, without error.";
		else post.Verbose("ValidateDataStructureProcess finished without incident.");
		return true;
	}
};
static bool ValidateDataStructure(Daedalus::post *post)
{
	return class ValidateDataStructure(post);
}							
Daedalus::post::ValidateDataStructure::ValidateDataStructure
(post *p):Base(p,p->steps&step?::ValidateDataStructure:0)
{
	#ifdef _DEBUG
	configWarnOfAssimpErrors = true;
	#else
	configWarnOfAssimpErrors = false;
	#endif
	configWarningsAreErrors = false;
	configProceedWithErrors = false;
}