
//struct PreMesh //PreServe.h
//{	
	typedef void Mesh2, IsSupported;

	inline PreMesh2 *This2(){ return (PreMesh2*)this; }
	inline const PreMesh2 *This2()const{ return (PreMesh2*)this; }	

	//copy FacesCoList items
	template<class T> inline void CoCopy(T &co, const T &cp)
	{ memcpy(&co,&cp,Is2()?sizeof(*co.This2()):sizeof(T)); }

	PRESERVE_LIST(PreMesh*)
	//3: HasFaces is FacesCoList
	//TODO: slip in assert(!_complex);
	PRESERVE_LISTS3(preFace,FacesList(),faces,faceslist)			
	PRESERVE_CONSTLISTS(PreMesh,preFace,Faces,faces,_FacesCoList<2>())
	template<class Face, class I> inline Face &_CoGet(Face *p, const I &i)const
	{ return *(Face*)((char*)p+i*(Is2()?sizeof(*p->This2()):sizeof(*p))); }	
	template<class Face> inline ptrdiff_t _CoDiff(Face *p, Face *q)const
	{ return Is2()?(PreFace2*)p-(PreFace2*)q:p-q; }	//std::iterator support
	template<class List> inline void _CoClear(List &list){ _CoReserve(0,list); }
	template<class List> inline void _CoMoved(List &list){ list._size = 0; }
	template<class List> void _CoReserve(size_t size, List &list)
	{	//TODO: RAISE A RELEASE MODE FLAG (put in pre-lists.hpp?)
		if((void**)&list._list==&preZeroMemory) assert(preZeroMemory&&0); 
		else if(Is2()) list._CoNew<sizeof(PreFace2)>(size,_FacesCoList<2>());
		else list._CoNew(size,faceslist);
	}				
	PRESERVE_LISTS3(PreFace2,FacesList2(),faces,_FacesList2<2>())
	inline bool HasFaces2(){ return FacesList2().HasList(); }
	PRESERVE_LISTS(signed,Indices,indices)		
	//AND? WHAT IF r.find IS NEGATIVE? IndicesSubList2 OR?
	const PreList<signed*> IndicesSubList(const PreFace::Range &r)
	{ 
		assert(indiceslist!=0&&r.find>=0); 
		return PreList<signed*>(r.size,indiceslist+r.find); 
	}
	const PreList<const signed*> IndicesSubList(const PreFace::Range &r)const
	{ 
		assert(indiceslist!=0&&r.find>=0); 
		return PreList<const signed*>(r.size,indiceslist+r.find); 
	}
	PRESERVE_LISTS(preBone*,Bones,bones)
	//3: HasMorphs is MorphsCoList/not using PRESERVE_COLISTS
	//TODO: slip in assert(!_complex);
	PRESERVE_LISTS3(preMorph*,MorphsList(),morphs,morphslist)		
	PRESERVE_LISTS3(preMorph*,MorphsCoList(),morphs,_MorphsCoList<2>())
	inline bool HasMorphs()const{ return MorphsCoList().HasList(); }
	inline preSize Morphs()const{ return MorphsCoList().RealSize(); }
	
	//ugly/lazy evaluation PreMesh2 internals	
	template<int i> inline preFace* &_FacesCoList()const
	{ return (preFace*&)(Is2()?(void*&)This2()->faceslist:(void*&)faceslist); }
	template<int i> inline PreFace2* &_FacesList2()
	{ return Is2()?This2()->faceslist:(PreFace2*&)preZeroMemory;
	}template<int i> inline const PreFace2 *_FacesList2()const
	{ return Is2()?This2()->faceslist:0; }	
	template<int i> preMorph** &_MorphsCoList()const
	{ return (preMorph**&)(Is2()?(void*&)This2()->morphslist:(void*&)morphslist); }										  	

	PreMesh(const PreMesh &_cp, PreX::Pool *p, const PreMesh2 *_this=0)
	:PRESERVE_VERSION,PreMorph(_cp,_this)
	{
		PRESERVE_EXTENDBASE(PreMorph)
		const PreMesh &cp = 
		PRESERVE_MOVECP(PreMorph,_cp)
		PRESERVE_COPYTHISAFTER(_size)
		PreX::GetPool(p)->PoolString(name);								
		PRESERVE_COPYLIST(indices)
		if(faceslist) if(faces) //cross copy
		{
			assert(!faceslist->Is2());
			const PreFace *copy = faceslist; 			
			(void*&)faceslist = operator new[](faces*sizeof(PreFace)); 
			for(preN i=0;i<faces;copy=copy->NextInFacesList(++i==faces))
			new(faceslist+i)PreFace(*copy,0);
		}
		else faceslist = 0;
		PRESERVE_DEEPCOPYLIST(bones,p)
		PRESERVE_DEEPCOPYLIST(morphs,0)
	}
	PreMesh():PRESERVE_VERSION
	{ PRESERVE_EXTENDBASE(PreMorph) PRESERVE_ZEROTHISAFTER(_size) }
	~PreMesh()
	{	
		FacesList().Clear(); IndicesList().Clear();
		BonesList().Clear(); MorphsList().Clear();
	}

	DAEDALUS_DEPRECATED("please use CountVertexColorsLists instead")
	void GetNumColorChannels()const; //not implementing
	inline preN CountVertexColorsLists()const
	{ preN n; for(n=0;n<colorslistsN&&colorslists[n];n++); return n; }	
	DAEDALUS_DEPRECATED("please use CountTextureCoordsLists instead")
	void GetNumUVChannels()const; //not implementing
	inline preN CountTextureCoordsLists()const
	{ preN n; for(n=0;n<texturecoordslistsN&&texturecoordslists[n];n++); return n; }	

	//BEWARE! not applied to PreBone::matrix
	//DeboneProcess, PretransformVertices & OptimizeGraph
	void _ApplyTransform(const preMatrix&);
	inline void ApplyTransform(const preMatrix&, int identity=-1);
	//PROBABLY DUE TO BE OBSOLETE
	//FindInstancesProcess, OptimizeMeshes & PretransformVertices 
	unsigned GetMeshVFormatUnique()const;
	
	//avoid Assimp's specialization of std::min and std::max
	Pre3D::Container GetMorphsContainer(Pre3D::Container=Pre3D::MinMax())const;

	//VS2010: without const a nested lambda expression has
	//"int" type. const is removed to allow mutable lambdas
	template<class Lambda> inline void ForEach(const Lambda &f)
	{ ((Lambda&)f)(this); MorphsCoList()^f; }
	template<class Lambda> inline void ForEach(const Lambda &f)const
	{ ((Lambda&)f)(this); MorphsCoList()^f; }
//};