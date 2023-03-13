
#include "Daedalus.(c).h"
using namespace Daedalus;	
namespace dae = DAEP::COLLADA_1_5_0;

//EXPERIMENTAL
//This was the first experiment with the old
//COLLADA-DOM library. It's since been used to
//test the 2016 rewrite. 
class CollaDAPP	//todo? post-DataEntry
{	
	static daeDOM &DOM;
	dae::COLLADA doc; const preScene *const art;		
	std::vector<const char*> images; size_t _images()
	{
		images.reserve(8); //todo: include _texturename
		art->MaterialsList()^[&](const preMaterial *ea)
		{
			ea->PropertiesList()^[&](const PreMaterial::Property *ea2)
			{ 
				if(ea2->key==ea->_texture) images.push_back(ea2->CharPointer()); 
			};
		};return images.size();
	}
	std::unordered_map<const char*,int> nodes; //optimizing
	typedef std::pair<preID,preID> skins_range;
	std::vector<skins_range> skins; size_t _skins()
	{
		auto ml = art->MeshesCoList();
		skins.reserve(ml.RealSize()); //todo? use PreNew
		for(preID m=ml,n=m,nN=ml.Size()-1;m<ml.Size();m=n=n+1)
		{
			while(n<nN&&ml[n+1]->name.cstring==ml[n]->name.cstring) n++;
			skins.push_back(skins_range(m,n));			
		}return skins.size();
	}

	char _URInt[35], *_Int;
	inline const char *Int(int id){ return itoa(id,_Int,10); }
	inline const char *Id(int id){ return &((char&)*Int(id)+='A'-'0'); }
	inline const char *URId(int id){ return Id(id)-1; }		

	//these are in the order they're to be called
	dae::material Create(const preMaterial *mat)
	{
		dae::material o(DOM); o->id = Id(mat0+matID);
		o->instance_effect->url = URId(fx0+matID); matID++; return o;	
	}
	struct common_shading_models
	{	
		template<class T> void setConstant(T &el)
		{
			if(e) emission = +el.emission; 
			if(r) reflective = +el.reflective;
			if(r) reflectivity = +el.reflectivity;
			if(t) transparent = +el.transparent;
			if(t) transparency = +el.transparency;
			if(t) index_of_refraction = +el.index_of_refraction;
		}
		template<class T> void setLambert(T &el)
		{
			setConstant(el);
			if(a) ambient = +el.ambient; if(d) diffuse = +el.diffuse;
		}
		template<class T> void setPhong(T &el)
		{
			 setLambert(el);
			if(s) specular = +el.specular; if(s) shininess = +el.shininess;
		}
		template<class T> void setBlinn(T &el){ setPhong(el); }		
		
		dae::emission emission,ambient,diffuse,specular,reflective;
		dae::transparency transparency,reflectivity,index_of_refraction,shininess;
		dae::transparent transparent;

		common_shading_models(bool e, bool a, bool d, bool s, bool r, bool t)
		:e(e),a(a),d(d),s(s),r(r),t(t){}bool e,a,d,s,r,t;
	};	
	dae::effect Create2(const preMaterial *mat)
	{
		dae::effect o(DOM);
		o->id = Id(fx0+fxID++);
		//TODO: extract these more elegantly
		float f = 0; mat->PairKey_Get(mat->white,f);
		pre4D ec(0,1), ac(f,1), dc=ac, sc=ac, rc=ec, tc=ec;
		//COLLADA 1.5 can't do texture*color; Cg can later on
		mat->PairKey_Get(mat->emission,ec);
		mat->PairKey_Get(mat->ambient,ac);
		mat->PairKey_Get(mat->diffuse,dc);
		mat->PairKey_Get(mat->specular,sc);
		mat->PairKey_Get(mat->reflective,rc);
		mat->PairKey_Get(mat->transparent,tc);
		union{ const PreMaterial::Property *samplers[6];
		struct{ const PreMaterial::Property *et,*at,*dt,*st,*rt,*tt; }; };
		et = mat->PairKey(mat->texture,mat->emissiontexture);
		at = mat->PairKey(mat->texture,mat->ambienttexture);
		dt = mat->PairKey(mat->texture,mat->diffusetexture);
		st = mat->PairKey(mat->texture,mat->speculartexture);
		rt = mat->PairKey(mat->texture,mat->reflectivetexture);
		tt = mat->PairKey(mat->texture,mat->transparenttexture);
		dae::profile_COMMON pc = +o->profile_COMMON;				
		int counter = 0; 
		for(int i=0;i<6;i++) if(samplers[i])		
		pc->newparam.push_back(Create(counter++,samplers[i]->CharPointer()));
		dae::profile_COMMON::technique pct = +pc->technique;
		pct->sid = "common";
		PreMaterial::Shading sh = mat->notshaded;
		mat->Get(mat->shadingmodel,sh); const pre4D Bl(0,0,0,1); //black
		common_shading_models csm(et||ec!=Bl,at||ac!=Bl,dt||dc!=Bl,st||sc!=Bl,rt||rc!=Bl,tt||tc!=Bl);
		switch(sh) //approximating
		{
		case mat->notshaded: default:			
		if(!csm.a&&!csm.d&&!csm.s)
		{ csm.setConstant(*pct->constant); break; }
		case mat->flatshaded: case mat->Gouraud: 
		case mat->cellshaded: case mat->OrenNayar: 		
		case mat->Minnaert: 			
		if(!csm.s){ csm.setLambert(*pct->lambert); break; }
		case mat->CookTorrance: case mat->Fresnel:		
		case mat->Phong: csm.setPhong(*pct->phong); break;				
		case mat->Blinn: csm.setBlinn(*pct->blinn); break;
		}
		counter = 0; 
		if(csm.emission) Choose(ec,et?counter++:-1,*csm.emission);
		if(csm.ambient) Choose(ac,at?counter++:-1,*csm.ambient); 
		if(csm.diffuse) Choose(dc,dt?counter++:-1,*csm.diffuse); 
		if(csm.specular) //mat->shininess_strength?
		{
			Choose(sc,st?counter++:-1,*csm.specular);
			if(mat->PairKey_Get(mat->shininess,f))			
			csm.shininess->float__alias->value = f;
		}
		if(csm.reflective) 
		{
			Choose(rc,rt?counter++:-1,*csm.reflective);
			if(mat->PairKey_Get(mat->reflectivity,f))			
			csm.reflectivity->float__alias->value = f;
		}
		if(csm.transparent)
		{
			Choose(tc,tt?counter++:-1,*csm.transparent);
			if(mat->PairKey_Get(mat->transparency,f))
			csm.transparency->float__alias->value = f;
			if(mat->PairKey_Get(mat->index_of_refraction,f))
			csm.index_of_refraction->float__alias->value = f;
		}return o;
	}	
	dae::profile_COMMON::newparam Create(int texture, const char *image) 
	{
		dae::profile_COMMON::newparam o(DOM); o->sid = Id(texture);
		int i = std::lower_bound(images.begin(),images.end(),image)-images.begin();		
		o->sampler2D->instance_image->url = URId(image0+i); return o;		
	}
	template<class T> //domFx_common_color_or_texture
	void Choose(const pre4D &color, int texture, T &e) 
	{
		if(texture>=0) //this is an awful choice (texture or color??)
		{
			dae::texture t = +e.texture;
			t->texture = Id(texture);
			t->texcoord = "UVSET0";
			int i; for(i=0;i<4&&color.n[i]==(int)color.n[i];i++); 
			if(i==4) return; //include color in the XML text for review only?
			t->extra->technique->profile = "color";
		}
		e.color->value->assign(color.n,4); 
	}
	dae::image Create(const char *texture) //material key
	{	
		dae::image o(DOM);
		o->id = Id(image0+imageID++);
		dae::image::init_from iif = +o->init_from;
		if(*texture!='*') //* is Assimp notation
		{
			//TODO: warn if absolute/ensure file:// used
			iif->ref->value = texture;
		}
		else //then assuming embedded image data
		{				
			dae::image::init_from::hex hex = +iif->hex; 
			auto &hex_value = hex->value;
			//COLLADA says this is a list. it shouldn't be
			hex_value->setCountMore(1); 
			xs::hexBinary &hexbin = hex_value->front();	
			const preTexture *tex = 
			art->EmbeddedTexturesList()[strtol(texture+1,0,10)];
			if(!tex->HasColors()) //come what may
			{
				hex->format = tex->assimp3charcode_if0height;
				auto cdl = tex->ColorDataList();
				hexbin.assign((char*)cdl.Pointer(),cdl.Size());
			}
			else //DDS is the simplest format in this case
			{
				hex->format = "dds"; //Assimp is lowercase
				struct //Assimp format
				{
				PreSet<preN,124> dwSize;
				PreSet<preN,0x100F> dwFlags;
				preN dwHeight,dwWidth,dwPitchOrLinearSize;				
				PreSet<preN,0> dwDepth,dwMipMapCount,dwReserved1[11];
				struct //ddspf
				{
				PreSet<preN,32> dwSize;
				//1: might want to consult PreMaterial::tidbits
				PreSet<preN,0x41> dwFlags; 
				PreSet<preN,0> dwFourCC;
				PreSet<preN,32> dwRGBBitCount;
				PreSet<preN,0x00FF0000> dwRBitMask;
				PreSet<preN,0x0000FF00> dwGBitMask;
				PreSet<preN,0x000000FF> dwBBitMask;
				PreSet<preN,0xFF000000> dwABitMask;
				}ddspf;
				PreSet<preN,0x1000> dwCaps;
				PreSet<preN,0> dwCaps2,dwCaps3,dwCaps4,dwReserved2;
				}dds; PreSuppose(sizeof(dds)==124); 				
				dds.dwWidth = tex->width;				
				dds.dwHeight = tex->height;
				dds.dwPitchOrLinearSize = 8*tex->width;
				//DISABLING AS IT MAKES THE TEXT FILE BIG
				//2016: REQUIRES PATCH TO daeLIBXMLPlugin.cpp (or daeAtomicType.h/cpp)
				//hexbin.setCountMore(sizeof(dds)+8*tex->colors);
				//memcpy(&hexbin[0],&dds,sizeof(dds));
				//memcpy(&hexbin[sizeof(dds)],tex->colorslist,8*tex->colors);
			}			
		}return o;
	}
	/////GEOMETRY/////////////////////////
	dae::node Create(const preNode *node)
	{
		preN id = node0+nodeID++;
		nodes[node->name.cstring] = id; //optimizing
		dae::node o(DOM);
		o->id = Id(id);
		o->name = node->name.String();
		o->matrix->value->assign(node->matrix.n,16); 			
		node->NodesList()^[&](const preNode *ea)
		{ o->node.push_back(Create(ea)); }; return o;
	}	
	std::vector<dae::source> sources; 
	std::vector<dae::polylist::input> inputs;
	static double *Floats(dae::source &s, preN count)
	{ 
		auto &v = s->float_array->value; 
		v->setCountMore(count); return v->data(); 
	}
	dae::geometry Create(const skins_range r)
	{
		preN pos = 0; //positions must be combined		
		auto il = art->MeshesCoList()<=r.second>=r.first;		
		il^[&](const preMesh *ea){ pos+=ea->Positions(); };

	  /////////////////////////////////////////////////////////////////////
	  //for now meshes sharing the same name are merged into one-big-skin//
	  /////////////////////////////////////////////////////////////////////

		dae::geometry o(DOM);
		o->id = Id(geom0+geomID++);
		o->name = il.Front()->name.String();
		dae::mesh mesh = +o->mesh;
		sources.push_back(Create("float_array",'xyzw',pos,3));
		double *d = Floats(sources.back(),pos*3); //not automatic???
		auto d3 = (pre3D*)d; il^[&](const preMesh *ea)
		{ ea->PositionsCoList()^[&](const pre3D &ea2){ *d3++ = ea2; };	};		
		dae::vertices verts = +mesh->vertices;
		verts->id = Id(freeID++);
		verts->input = CreateUnshared("POSITION",sources.back()->id);	
	
	  /////////////////////////////////////////////////////////////////////
	  //TODO: Collada 1.5 wants morph-targets to be limited to <vertices>//
	  /////////////////////////////////////////////////////////////////////

		auto ml = art->MaterialsList();
		const preMaterial *mat,*met = 0; pos = 0;
		for(preN i=il,tcoords[8],vcoords[8];i<il.Size();pos+=il[i++]->Positions())
		{
			//This is a MSVC2010 workaround, so this-> can be implicit.
			auto &inputs = this->inputs; auto &sources = this->sources;

			const preMesh *ea = il[i];
			//warning: not outputting 3D tcoords
			mat = ml[ea->material]; if(mat!=met)
			(met=mat)->GetTextureDimensions(tcoords,vcoords,&PreMaterial::PairKey);

			dae::polylist pl = ++mesh->polylist;			
			pl->material = Id(ea->material); pl->count = ea->Faces();
			inputs.push_back(CreateShared("VERTEX",verts->id,0));			
			auto &f = [&](const char *semantic, decltype(ea->NormalsCoList()) l)
			{
				if(l.HasList())
				{
					sources.push_back(Create("float_array",'xyzw',l.Size(),3));
					inputs.push_back(CreateShared(semantic,sources.back()->id,1));
					d = Floats(sources.back(),l.Size()*3);
					d3 = (pre3D*)d; l^[&](const pre3D &ea2){ *d3++ = ea2; };
				}	
			};f("NORMAL",ea->NormalsCoList());
			f("TEXTANGENT",ea->TangentsCoList());
			f("TEXBINORMAL",ea->BitangentsCoList());
			int set = 0, usage = 'stpq'; auto semantic = "TEXCOORD";
			auto &g = [&](int dN, decltype(ea->TextureCoordsCoList()) l)->bool
			{
				if(l.HasList())
				{
					if(!dN) dN = 2;
					sources.push_back(Create("float_array",usage,l.Size(),dN));
					inputs.push_back(CreateShared(semantic,sources.back()->id,1));
					inputs.back()->set = set++;
					d = Floats(sources.back(),l.Size()*dN); //not automatic???
					if(dN==1) l^[&](const pre3D &ea2){ *d++ = ea2.s; };
					if(dN==2) l^[&](const pre3D &ea2){ *d++ = ea2.s; *d++ = ea2.t; };
					if(dN==3) l^[&](const pre3D &ea2){ *d++ = ea2.s; *d++ = ea2.t; *d++ = ea2.p; };
					return true;
				}return false;
			};for(preN i=0;i<ea->texturecoordslistsN&&g(tcoords[i],ea->TextureCoordsCoList(i));i++);
			auto &h = [&](int dN, decltype(ea->VertexColorsCoList()) l)->bool
			{
				if(l.HasList())
				{
					if(!dN) return true;
					sources.push_back(Create("float_array",usage,l.Size(),dN));
					inputs.push_back(CreateShared(semantic,sources.back()->id,1));
					inputs.back()->set = set++;
					d = Floats(sources.back(),l.Size()*dN);
					if(dN==1) l^[&](const pre4D &ea2){ *d++ = ea2.r; };
					if(dN==2) l^[&](const pre4D &ea2){ *d++ = ea2.r; *d++ = ea2.g; };
					if(dN==3) l^[&](const pre4D &ea2){ *d++ = ea2.r; *d++ = ea2.g; *d++ = ea2.b; };
					if(dN==4) l^[&](const pre4D &ea2){ *d++ = ea2.r; *d++ = ea2.g; *d++ = ea2.b; *d++ = ea2.a; };
					return true;
				}return false;
			};for(preN i=0;i<ea->colorslistsN&&h(vcoords[i],ea->VertexColorsCoList(i));i++);
			set = 0; usage = 'rgba'; semantic = "COLOR"; //4: note 1.5 says COLOR is float3!?
			for(preN i=0;i<ea->colorslistsN&&h(vcoords[i]?0:4,ea->VertexColorsCoList(i));i++);
			dae::vcount::value_list_of_uints_type 
			&vcount = pl->vcount->value; vcount.grow(ea->faces);
			preN vsumtotal = 0;
			ea->FacesList()^[&](const preFace::Range &ea2)
			{ 
				vsumtotal+=ea2.size; vcount.push_back(ea2.size); 
			};	
			//sneaky: supplying pos as prototype and assuming cleared
			//here offset="0" is <vertices> and 1 is shared by inputs
			dae::p::value_list_of_uints_type 
			&pa = pl->p->value; pa.setCountMore(2*vsumtotal,pos);
			auto *p = &pa[0]; ea->FacesList()^[&](const preFace &ea2)
			{
				ea->IndicesSubList(ea2)^[&](signed ea3){ *p++ += ea3; *p++ = ea3; }; 
			};
			pl->input = inputs; inputs.clear();
		}mesh->source = sources; sources.clear(); return o;
	}
	dae::source Create(const char *dt, int convention, preN count, preN stride=0)
	{
		dae::source o(DOM);
		const char *names, *type = "float"; 
		assert(stride<=4); 
		switch(convention) 
		{ 
		default: assert(0); return o; //paranoia
		case 'xyzw': names = "X\0Y\0Z\0W\0"; break;
		case 'stpq': names = "S\0T\0P\0Q\0"; break;
		case 'rgba': names = "R\0G\0B\0A\0"; break;
		case 'bone': names = "JOINT"; type = "IDREF"; stride = 1; break;
		case 'pose': names = 0; type = "float4x4"; stride = 16; break;
		case 'skin': names = "WEIGHT"; stride = 1; break;
		}
		o->id = Id(freeID++);
		int gratuitous_id = freeID++; 
		auto edt = COLLADA::dae(o)->add(dt); 
		edt->setAttribute("id",Id(gratuitous_id));
		edt->setAttribute("count",Int(count*stride));
		dae::accessor a = +o->technique_common->accessor;
		a->source = URId(gratuitous_id);
		a->count = count; if(stride>1) a->stride = stride;		
		preN i=0,iN='pose'==convention?1:stride; //odd exception to the rule		
		for(auto &pa=a->param;i<iN;i++)
		{
			if(names) pa[i]->name = names+i*2; if(type) pa[i]->type = type;
		}return o;
	}
	dae::vertices::input CreateUnshared(const char *sem, const daeStringRef &id)
	{
		dae::vertices::input o(DOM);
		o->semantic = sem; o->source = id.getID_fragment();
		return o; //o->offset = off;
	}
	dae::polylist::input CreateShared(const char *sem, const daeStringRef &id, int off)
	{
		dae::polylist::input o(DOM); 
		o->semantic = sem; o->source = id.getID_fragment();
		o->offset = off; return o;
	}
	dae::node root; //testing
	std::unordered_set<preID> materials;
	dae::controller Create2(const skins_range r)
	{
		preN pos = 0; //positions must be combined
		auto il = art->MeshesCoList()<=r.second>=r.first;		
		il^[&](const preMesh *ea){ pos+=ea->Positions(); };
		dae::controller o(DOM);
		o->name = il.Front()->name.String();
		dae::skin skin = +o->skin;
		o->id = Id(skin0+skinID);
		skin->source__ATTRIBUTE = URId(geom0+skinID++);
		//UNIMPLEMENTED/PRELIMINARY		
		skin->bind_shape_matrix->value->assign(preMatrix().n,16);
		sources.push_back(Create("IDREF_array",'bone',1));
		sources.back()->IDREF_array->value->push_back(Id(node0));
		sources.push_back(Create("float_array",'pose',1));
		sources.back()->float_array->value->assign(preMatrix().n,16);
		sources.push_back(Create("float_array",'skin',1));
		sources.back()->float_array->value->push_back(1);
		skin->source__ELEMENT = sources;
		dae::joints joints = +skin->joints;
		joints->input.push_back(CreateUnshared("JOINT",sources[0]->id));
		joints->input.push_back(CreateUnshared("INV_BIND_MATRIX",sources[1]->id));
		dae::vertex_weights vweights = +skin->vertex_weights;
		vweights->count = pos;
		vweights->input.push_back(CreateShared("JOINT",sources[0]->id,0));
		vweights->input.push_back(CreateShared("WEIGHT",sources[2]->id,1));
		vweights->vcount->value->setCountMore(pos,1);				
		vweights->v->value->setCountMore(2*pos,0);				
		//UNIMPLEMENTED/PRELIMINARY
		dae::instance_controller ic = ++root->instance_controller;
		ic->url = URId(skin0);
		dae::bind_material
		::technique_common tc = +ic->bind_material->technique_common;		
		materials.clear(); 
		il^[&](const preMesh *ea) //THIS IS RENDUNDANT AS IS
		{
			materials.insert(ea->material); //???
		};
		for(auto it=materials.begin();it!=materials.end();it++)
		{
			dae::bind_material
			::technique_common
			::instance_material im = ++tc->instance_material;
			im->symbol = Id(*it); im->target = URId(mat0+*it);
		}
		sources.clear(); return o;
	}	
	/////ANIMATIONS///////////////////////
	PreSet<preN,0> matID,fxID,imageID,nodeID,geomID,skinID,freeID;	
	
public:	//CollaDAPP

	//numerical IDs table
	struct : PreSet<preN,1>
	{ inline long long operator+=(preN n)
	{ auto out = set; set+=n; return out; }}_0thID;
	const preN mat0,fx0,image0,cam0,light0,node0,geom0,skin0,anim0;
	CollaDAPP(dae::COLLADA root, const preScene *scene)
	:doc(root),art(scene)
	,cam0(_0thID+=art->Cameras())
	,light0(_0thID+=art->Lights())
	,mat0(_0thID+=art->Materials())
	,fx0(_0thID+=art->Materials())
	,image0(_0thID+=_images())
	,node0(_0thID+=art->CountNodes())
	,geom0(_0thID+=_skins())
	,skin0(_0thID+=skins.size())
	,anim0(_0thID+=art->Animations())
	{
		freeID = +_0thID; //sensitive to ctor order

		_Int = _URInt+1; _Int[-1] = '#';
	}
	operator bool() //one and done
	{	
		assert(!doc->asset);

		//the standard requires these		
		dae::asset asset = +doc->asset; 
		+asset->contributor; //anon?
		const char *dtUnknown = "2000-01-01Z00:00:00";
		asset->created->value = dtUnknown;
		asset->modified->value = dtUnknown;

		PreSuppose(!PreMesh2::IsSupported);		
		
		//now, fill in each library		
		dae::library_materials libm = +doc->library_materials;		
		art->MaterialsList()^[&](const preMaterial *ea)
		{
			libm->material.push_back(Create(ea)); 
		};
		std::sort(images.begin(),images.end());
		images.erase(std::unique(images.begin(),images.end()),images.end());
		dae::library_effects libe = +doc->library_effects;
		art->MaterialsList()^[&](const preMaterial *ea)
		{
			libe->effect.push_back(Create2(ea)); 
		};
		dae::library_images libi = +doc->library_images;
		std::for_each(images.begin(),images.end(),[&](const char *img)
		{
			libi->image.push_back(Create(img)); 
		});
		//nodes must be done before skins
		//skins are generated to make them easier to external reference
		//(as Collada 1.5 lacks a concept akin to a ref-friendly-object)
		//mainly so <instance_controller><bind_material> can override the
		//FX part of the specification, as applications tend to neglect it
		dae::visual_scene vs(DOM);
		root = Create(art->rootnode); vs->node = root;		
		dae::library_geometries libg = +doc->library_geometries;
		std::for_each(skins.begin(),skins.end(),[&](skins_range r)
		{ 
			libg->geometry.push_back(Create(r)); 
		});
		dae::library_controllers libc = +doc->library_controllers;
		std::for_each(skins.begin(),skins.end(),[&](skins_range r)
		{ 
			libc->controller.push_back(Create2(r)); 
		});
		//and, last but not least...
		doc->library_visual_scenes->visual_scene = vs;
		vs->id = Id(0); vs->name = art->name.String();
		doc->scene->instance_visual_scene->url = URId(0); 

		return true; //pro forma for now
	}
};
struct CollaDAPP_IO : daeIO
{	
	virtual daeOK getError()
	{
		return OK;  
	}
	virtual size_t getLock()
	{
		if(OK==DAE_OK&&lock==size_t(-1))
		{	
			lock = 0;
			wchar_t maxpath[MAX_PATH];
			if(!I.getRequest().isEmptyRequest())
			{
				int i = 0; 
				if(_file_protocol(maxpath,I))
				r = CRT.r->fopen(maxpath,L"rb",_SH_DENYWR);
				if(r!=nullptr)
				{
					i = CRT.r->stat(r).size;
				}
				if(i>0) lock = i; else OK = DAE_ERROR;
			}
			if(!O.getRequest().isEmptyRequest())
			{
				if(_file_protocol(maxpath,O))
				w = CRT.w->fopen(maxpath,L"wb",_SH_DENYRW);
				if(w==nullptr) OK = DAE_ERROR; 
			}
		}
		return lock;
	}
	virtual daeOK readIn(void *in, size_t chars)
	{
		if(1!=CRT.r->fread(in,chars,1,r)&&chars!=0) OK = DAE_ERROR; return OK;
	}
	virtual daeOK writeOut(const void *out, size_t chars)
	{
		if(1!=CRT.w->fwrite(out,chars,1,w)&&chars!=0) OK = DAE_ERROR; return OK;
	}
	virtual FILE *getReadFILE(int=0)
	{
		if(r==nullptr) getLock(); return r; 
	}
	virtual FILE *getWriteFILE(int=0)
	{
		if(w==nullptr) getLock(); return w; 
	}	

	//This is not the normal way to go about this.
	//It's just easy to set up for basic file I/O.
	struct{ const struct daeCRT::FILE *r,*w; }CRT;
	daeIOPlugin &I,&O; daeOK OK; FILE *r,*w; size_t lock;
	CollaDAPP_IO(std::pair<daeIOPlugin*,daeIOPlugin*> IO)
	:I(*IO.first),O(*IO.second),r(),w(),lock(-1)
	{
		CRT.r = &I.getCRT_default().FILE; 
		CRT.w = &O.getCRT_default().FILE;
	}
	~CollaDAPP_IO()
	{
		if(r!=nullptr) CRT.r->fclose(r); 
		if(w!=nullptr) CRT.w->fclose(w); 
	}

	static bool _file_protocol(wchar_t (&maxpath)[MAX_PATH], daeIOPlugin &source)
	{
		const daeURI *URI = source.getRequest().remoteURI; 
		if(URI!=nullptr&&"file"==URI->getURI_protocol())
		{
			daeRefView v = URI->getURI_upto<'?'>(); int UNC = v[7]=='/'?8:5;
			maxpath[MultiByteToWideChar(CP_UTF8,0,v+UNC,v.extent-UNC,maxpath,MAX_PATH-1)] = 0;
			return true;
		}assert(0); return false; 
	}
};
static struct CollaDAPP_Platform : daePlatform
{
	std::vector<CollaDAPP_IO> IO_stack;

	virtual const daeURI &getDefaultBaseURI(daeURI &URI)
	{
		wchar_t maxpath[MAX_PATH];
		GetCurrentDirectoryW(MAX_PATH,maxpath);
		char UTF[4*MAX_PATH+8] = "file:///"; int UNC = maxpath[0]=='//'?5:8;
		int len = UNC+WideCharToMultiByte(CP_UTF8,0,maxpath,-1,UTF+UNC,sizeof(UTF)-8,0,0);
		//If the directory URI doesn't end in a slash it looks like a filename?
		switch(UTF[len-2]) 
		{
		case '\\': case '/': break; default: UTF[len-1] = '/'; UTF[len] = '\0';
		}
		URI.setURI(UTF); return URI;
	}
	virtual daeOK resolveURI(daeURI &URI, const daeDOM &DOM)
	{
		daeOK OK = URI.resolve_RFC3986(DOM);
		assert(URI.getURI_protocol().size()>=4); return OK;
	}
	virtual daeOK openURI(const daeIORequest &req, daeIOPlugin *I, daeURIRef &URI)
	{
		daeDocRoot<> doc; 
		if(req.localURI->getURI_extensionIs("dae"))
		req.fulfillRequestI<dae::COLLADA>(I,doc); else req.unfulfillRequest(doc); 		
		if(doc==DAE_OK) URI = &doc->getDocURI(); else assert(0); return doc; 
	}
	virtual daeIO *openIO(daeIOPlugin &I, daeIOPlugin &O)
	{
		IO_stack.emplace_back(std::make_pair(&I,&O)); return &IO_stack.back();
	}
	virtual void closeIO(daeIO *IO)
	{
		assert(IO==&IO_stack.back()); IO_stack.pop_back();
	}		
	virtual int getLegacyProfile()
	{
		return LEGACY_LIBXML; 
	}
}CollaDAPP_Platform;
daeDOM &CollaDAPP::DOM = Daedalus::CollaDB;
extern daeDOM Daedalus::CollaDB(nullptr,&CollaDAPP_Platform);
extern bool Daedalus::CollaDAPP(daeElementRef root, const preScene *scene)
{
	return class CollaDAPP(root->a<COLLADA_1_5_0::COLLADA>(),scene);
}