
//struct PreMaterial //PreServe.h
//{	
	PRESERVE_LIST(PreMaterial*)
	//the list itself is const, properties are not
	const PreList<Property*const*> PropertiesList()
	{ return PreList<Property*const*>(_properties,_propertieslist); }
	const PreList<const Property*const*> PropertiesList()const
	{ return PreList<const Property*const*>(_properties,_propertieslist); }
	inline preN PropertiesRange(preN from)const
	{
		preN to = from; auto p = _propertieslist;
		if(p) while(to<_properties)	if(p[to]->texturepair==p[from]->texturepair)
		to++; else break; return to;
	}
	const PreList<Property*const*> PropertiesSubList(preN from)
	{ return PreList<Property*const*>(PropertiesRange(from)-from,_propertieslist+from); }
	const PreList<const Property*const*> PropertiesSubList(preN from)const
	{ return PreList<const Property*const*>(PropertiesRange(from)-from,_propertieslist+from); }
	static bool Less(const Property *a, const Property *b)
	{
		int cmp = b->texturepair-a->texturepair; return !cmp?a->key<b->key:cmp<0;
	}
	inline void Sort()
	{ PreList<Property**>(_properties,_propertieslist).Sort(Less); }
	inline bool IsSorted()const{ return PropertiesList().IsSorted(Less); }
		
	typedef Property::Key(*_Key)();
	template<class T, class Tex> static inline
	_Key _Underscore(Property::Key(*k(const T&,Tex))()){ return k((T&)k,(Tex)0); } 	
	struct _KeyID //BEGIN CROSS-COPY SEQUENCE
	{
		_KeyID(const char *id=0):permaID(id){} 
		_Key key; const char *permaID; size_t sizeof_T; 
		template<class T, class Tex> _KeyID(Property::Key(*k(const T&,Tex))())
		:key(_Underscore(k)),permaID(key().permanentID),sizeof_T(sizeof(T)){}				
		friend inline bool operator<(const _KeyID &a, const _KeyID &b){ return strcmp(a.permaID,b.permaID)<0; }						
		//not sure what is appropriate. Just guarantee data won't crash
		bool Crosswise(const Property &cp, const Property::Key255 &cmp)
		{	Property::Key k = key(); if(k.storagetype!=cmp->storagetype) return true;
			return cp.SizeOfMemory()>=sizeof_T?false:cmp->storagetype!=cp.stringtype;
		}inline bool Lost(const Property &cp, const Property::Key255 &cmp)
		{ return key==cp.key?false:Crosswise(cp,cmp); }
	};
	static const PreNew<_KeyID[]>::Vector &_GetTranslationUnitKeysList()
	{
		static PreNew<_KeyID[]>::Vector x; if(!x.HasList())	
		{
		x.Reserve(27);						x.PushBack(name);
		x.PushBack(_default);				x.PushBack(white);
		x.PushBack(twosided);				x.PushBack(shadingmodel);
		x.PushBack(enablewireframe);		x.PushBack(blendingmodel);
		x.PushBack(transparency);			x.PushBack(bumpscaling);
		x.PushBack(shininess);				x.PushBack(shininess_strength);
		x.PushBack(reflectivity);			x.PushBack(index_of_refraction);
		x.PushBack(diffuse);				x.PushBack(ambient);
		x.PushBack(specular);				x.PushBack(emission);
		x.PushBack(transparent);			x.PushBack(reflective);
		x.PushBack(diffuselist);			x.PushBack(ambientlist);
		x.PushBack(specularlist);			x.PushBack(emissionlist);
		x.PushBack(transparentlist);		x.PushBack(reflectivelist);
		x.PushBack(background);				x.PushBack(texture);
		x.PushBack(sourceslist);			x.PushBack(transfer);
		x.PushBack(blendfactor);			x.PushBack(wrapping);
		x.PushBack(mapping_axis);			x.PushBack(matrix);
		x.PushBack(tidbits);				x.Sort();
		}
		return x;
	}	
	DAEDALUS_DEPRECATED("porting Assimp? Try adding ,0")
	PreMaterial(const PreMaterial &cp); //not implementing
	PreMaterial(const PreMaterial &cp, PreX::Pool *p):PRESERVE_VERSION
	{
		PRESERVE_COPYTHISAFTER(_size)						
		auto il = PreConst(this)->PropertiesList();
		_propertieslist = new Property*[_allocated=_properties]; 
		int losses = 0, holes = 0;		
		auto &x = _GetTranslationUnitKeysList();
		Property::Key255 k; auto &_watch = (Property::Key&)k;		
		for(preN i=il,j=0;i<il.Size();i++)
		{
			if(!il[i]){ holes++; continue; }
			k = il[i]->_key255(); 
			_KeyID *xKey = x.PairKey(k->permanentID);
			if(!xKey||xKey->Lost(*il[i],k)) losses+='_'!=*k->permanentID; 
			else _propertieslist[j++] = new Property(*il[i],p,xKey->key);
		};_properties = il.RealSize()-losses; assert(!losses);
		if(!IsSorted()) Sort();	assert(!losses&&!holes);
		if(losses) InsertProperty(losses,_lost_in_copy); 
		if(holes) InsertProperty(holes,_hole_in_copy); 		
	}
	//Assimp/MaterialSystem.cpp
	PreMaterial(preN _=0):PRESERVE_VERSION
	{			
		PRESERVE_ZEROTHISAFTER(_size)
		_propertieslist = new Property*[_allocated=_?_:5];
	}
	static const int _default_version = 0;
	PreMaterial(PreDefault):PRESERVE_VERSION
	{
		PRESERVE_ZEROTHISAFTER(_size)
		_propertieslist = new Property*[_allocated=4];		
		//INCREMENT _default_version IF CHANGED
		AddProperty(_default_version,_default);
		{
			AddProperty(pre3D(0.6),diffuse);
			//AI: add a small ambient amount
			AddProperty(pre3D(0.05),ambient);
			AddProperty("System Default Material",name);
		}
		Sort();
	}
	~PreMaterial()
	{
		Clear(); delete[] _propertieslist;
	}	
	
	//unsorted lookup
	inline preN _FindN(_Key key, Texture tex=_nontexture, preN texN=0)const
	{ 	auto nl = PropertiesList();
		auto tp = Property::MakeTexturePair(tex,texN);
		for(preN n=nl;n<nl.Size();n++) 
		if(tp==nl[n]->texturepair&&key==nl[n]->key) return n; return preMost;
	}template<class Key>
	inline Property *Find(Key k, Texture t=_nontexture, preN c=0)
	{ preN n = _FindN(_Underscore(k),t,c); return n==preMost?0:_propertieslist[n]; }
	template<class Key>
	inline const Property *Find(Key k, Texture t=_nontexture, preN c=0)const
	{ preN n = _FindN(_Underscore(k),t,c); return n==preMost?0:_propertieslist[n]; }
	//sorted lookup
	template<class Key> 
	inline Property *PairKey(Key key, Texture tex=_nontexture, preN texN=0)
	{ 	Property lesskey(tex,texN,_Underscore(key));
		auto pp = PreList<Property**>(_properties,_propertieslist).PairKey(&lesskey,Less);
		return pp?*pp:0;
	}template<class Key>
	inline const Property *PairKey(Key k, Texture t=_nontexture, preN c=0)const
	{ return const_cast<PreMaterial*>(this)->PairKey(k,t,c); }

	Property *_AddProperty(const char *cp, preN unitsize, preN count, _Key key, Texture tex, preN texN)
	{			
		Property::Key k = key(); 
		preN realsize = 0; switch(k.storagetype)
		{
		case Property::blobtype: //Assimp?
		{
			realsize = unitsize; assert(0); break;
		}
		case Property::stringtype: realsize = 1; break;
		case Property::inttype: realsize = sizeof(int); break;
		case Property::floattype: realsize = sizeof(float); break;		
		case Property::doubletype: realsize = sizeof(double); break;		
		}if(unitsize>realsize){ assert(0); return 0; } //paranoia

		//NOTE: data is not reassigned
		//TODO: RECONSIDER IF delete IS NECESSARY
		preN n = _FindN(key,tex,texN); 
		if(n!=preMost) delete _propertieslist[n];
		Property *p = new Property(tex,texN,key); 
		p->_sizeofdatalist = realsize*count;
		if(p->_sizeofdatalist>sizeof(p->_datalist)) //NEW
		{
			p->_datalist = new char[p->_sizeofdatalist];
		
			if(realsize!=unitsize) //too much? mainly for bool
			{
				assert(k.storagetype==Property::inttype&&unitsize<realsize);
				char *pc = (char*)p->_datalist;
				for(preN j,i=0;i<count;i++)
				{				
					#ifndef WIN32
					#error Little Endian?
					#endif //sign extending
					char fill = ((int&)*cp)&(1<<(unitsize*CHAR_BIT-1))?0xFF:0;
					for(j=0;i<unitsize;j++) *pc++ = *cp++;
					while(j<realsize) *pc++ = fill;
				}
			}
			else memcpy(p->_datalist,cp,p->_sizeofdatalist);
		}		
		else if(count||k.storagetype!=Property::stringtype)
		{
			if(count) memcpy(&p->_datalist,cp,p->_sizeofdatalist); 
			else p->_datalist = 0; //PreDoNothing?
		}
		else (const char*&)p->_datalist = cp; //pooled string?

		if(n>=_properties&&_properties>=_allocated)
		{
			assert(_properties==_allocated);
			preN allocating = _allocated*2;
			if(allocating<5) allocating = 5; //paranoia
			Property **swap = _propertieslist;
			_propertieslist = new Property*[allocating];
			memcpy(_propertieslist,swap,_allocated*sizeof(*swap));
			_allocated = allocating; delete[] swap;
		}
		if(n==preMost) n = _properties++;
		return _propertieslist[n] = p; 
	}
	template<class T> //MaterialSystem.cpp
	inline Property *_AddBinaryProperty(const T *cp, preN count, _Key key, Texture texture, preN texN)
	{ return _AddProperty((const char*)cp,sizeof(T),count,key,texture,texN); }	
	template<class T, class Tex>
	inline Property *AddProperty(const T &cp, Property::Key(*key(const T&,Tex))(), Tex texture=(Tex)0, preN texN=0)
	{ return _AddBinaryProperty(&cp,1,key(cp,texture),(Texture)(int)texture,texN); }	
	template<class T, int N, class Tex> 
	inline Property *AddProperty(const T (&cp)[N],Property::Key(*key(const T(&)[N],Tex))(), Tex texture=(Tex)0, preN texN=0)
	{ return _AddBinaryProperty(cp,N,key(cp,texture),(Texture)(int)texture,texN); }
	template<class Tex> 
	inline Property *AddProperty(const preX &cp, Property::Key(*key(const preX&,Tex))(), Tex texture=(Tex)0, preN texN=0)
	{ return _AddBinaryProperty(cp.String(),cp.length+1,key(cp,texture),(Texture)(int)texture,texN); }	
	template<class Tex> 
	inline Property *AddProperty(const char *cp, Property::Key(*key(const preX&,Tex))(), Tex texture=(Tex)0, preN texN=0)
	{ return _AddBinaryProperty(cp,strlen(cp)+1,key(*(preX*)0,texture),(Texture)(int)texture,texN); }
	template<class Tex> //USE THIS FOR DOING AddProperty()->PoolString() ONLY
	inline Property *AddProperty(PreNoTouching, Property::Key(*key(const preX&,Tex))(), Tex texture=(Tex)0, preN texN=0)
	{ return _AddProperty(0,0,0,_Underscore(key),(Texture)(int)texture,texN); }
	template<class Tex> 
	inline Property *AddProperty(const pre3D &cp, Property::Key(*key(const pre4D&,Tex))(), Tex texture=(Tex)0, preN texN=0)
	{
		pre4D alpha(cp.r,cp.g,cp.b,1);
		return _AddBinaryProperty(&alpha,1,key(alpha,texture),(Texture)(int)texture,texN);
	}		
	template<class S, class T, class Tex> //sorted-add, for use by post-process procedures
	inline Property *InsertProperty(const S &cp, Property::Key(*key(const T&,Tex))(), Tex texture=(Tex)0, preN texN=0)
	{	
		preN n = _properties;
		auto val = AddProperty(cp,key,texture,texN); 
		if(n<_properties) //added to end?
		{	auto end = _propertieslist+n;
			auto ub = std::upper_bound(_propertieslist,end,val,Less); //scheduled obsolete
			memmove(ub+1,ub,end-ub); *ub = val;
		}return val;
	}

	void Clear() //MaterialSystem.cpp
	{
		RemoveEachPropertyIf([](const Property*){ return true; });
	}
	template<class F> inline preN RemoveEachPropertyIf(const F &predicate)
	{
		PreList<Property**&> il(_properties,_propertieslist);
		preN i,j; for(i=il,j=i;i<il.Size();i++)
		{ if(predicate(PreConst(il[i]))) delete il[i]; else il[j++] = il[i]; }
		il.ForgoMemory(j); return i-j;
	}
	bool _RemoveProperty(_Key key, Texture tex, preN texN)
	{
		auto tp = Property::MakeTexturePair(tex,texN);
		return 0!=RemoveEachPropertyIf([=](const Property *ea)
		{ return ea->key==key&&tp==ea->texturepair; });
	}
	template<class T, class Tex> 
	inline bool RemoveProperty(Property::Key(*key(const T&,Tex))(), Tex texture=(Tex)0, preN texN=0)
	{ return _RemoveProperty(_Underscore(key),(Texture)(int)texture,texN); }	
	
	DAEDALUS_DEPRECATED("please use copy-constructor instead")
	static void CopyPropertyList(PreMaterial *dst, const PreMaterial *src);	

	//TODO: test if p's content is legit/return false if not
	template<class T> static inline bool _Get(T &out, const Property *p)
	{
		assert(sizeof(T)==p->_sizeofdatalist); out = p->_unsafe_cast<T>(); 
		return true;
	}
	template<> static inline bool _Get(PreX &out, const Property *p)
	{
		if(0==p->_sizeofdatalist)	
		out.PoolString(p->_datalist); //pooledstring
		else out.SetString(p->_datalist,p->_sizeofdatalist-1);
		return true;
	}	   
	template<class T> //unsorted
	inline bool Get(Property::Key(*key(const T&,void*))(), T &out)const
	{ auto p = Find(key); return p?_Get(out,p):false; }
	template<class T>
	inline bool Get(Property::Key(*key(const T&,Texture))(), Texture tex, preN texN, T &out)const
	{ auto p = Find(key,tex,texN); return p?_Get(out,p):false; }
	template<class T>
	inline bool Get(Property::Key(*key(const T&,Texture))(), Texture tex, T &out)const
	{ return Get(key,tex,0,out); }

	template<class T> //sorted Get
	inline const Property *PairKey_Get(Property::Key(*key(const T&,void*))(), T &out)const
	{ auto kv = PairKey(key); if(kv) _Get(out,kv); return kv; }
	template<class T>
	inline const Property *PairKey_Get(Property::Key(*key(const T&,Texture))(), Texture tex, preN texN, T &out)const
	{ auto kv = PairKey(key,tex,texN); if(kv) _Get(out,kv); return kv; }
	template<class T>
	inline const Property *PairKey_Get(Property::Key(*key(const T&,Texture))(), Texture tex, T &out)const
	{ auto kv = PairKey(key,tex,0); if(kv) _Get(out,kv); return kv;	}

	//SCHEDULED OBSOLETE
	//NOTE: Daedalus does not require client APIs
	inline preN GetTextureCount(Texture tex)const //MaterialSystem.cpp
	{
		preN out = 0; PropertiesList()^[&](const Property *ea)
		{ if(tex==ea->TextureCategory()) out = std::max(out,ea->TextureNumber()+1); };
		return out;
	}//reminder: it isn't clear that blend is ever not 1 or what it's for
	inline bool GetTexture(Texture texture, preN texN, preX &uri, Mapping *p=0, preN *texcoordslist=0,/* double *blend=0,*/ Transfer *q=0, Wrapping *r=0)const;		
	//RIP PreMesh::texturedimensions	
	inline void GetTextureDimensions
	(preN (&tc)[PreMorph::texturecoordslistsN], preN (&vc)[PreMorph::colorslistsN]
	,const Property *(PreMaterial::*FindOrPairKey)(decltype(mapping),Texture,preN)const=&PreMaterial::Find)const;	
//};