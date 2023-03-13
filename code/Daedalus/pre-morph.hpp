
//struct PreMorph //PreServe.h
//{
	//TODO! implement PreAnimation::Morph
	static const bool IsSupported = false;
	
	inline const PreMesh2 *Is2()const{ return _complex; }
	inline PreMesh2 *Mesh2(){ return (PreMesh2*)_complex; }
	inline const PreMesh2 *Mesh2()const{ return _complex; }		

	PRESERVE_LIST(PreMorph*)
	PRESERVE_LISTS(preID,MetaData,metadata)
	#define _(x,y) /*& is for the co-lists*/\
	template<int> inline preSize&_##x(){ return Is2()?Mesh2()->y:positions; }\
	template<int> inline preSize _##x()const{ return Is2()?Mesh2()->y:positions; }
	_(Positions,positions)_(Normals,normals)
	_(VertexColors,colors)_(TextureCoords,texturecoords)
	#undef _ //co-lists are coordinated buffer-sizes-wise
	PRESERVE_COLISTS(PreMorph,pre3D,Positions,_Positions<2>(),positionslist)
	PRESERVE_COLISTS(PreMorph,pre3D,Normals,_Normals<2>(),normalslist)
	PRESERVE_COLISTS(PreMorph,pre3D,Tangents,_Normals<2>(),tangentslist)
	PRESERVE_COLISTS(PreMorph,pre3D,Bitangents,_Normals<2>(),bitangentslist)
	PRESERVE_COLISTS2(PreMorph,pre4D,VertexColorsCoList(preN i=0),
	_VertexColors<2>(),i<colorslistsN?colorslists[i]:(pre4D*&)preZeroMemory)	
	PRESERVE_COLISTS2(PreMorph,pre3D,TextureCoordsCoList(preN i=0),
	_TextureCoords<2>(),i<texturecoordslistsN?texturecoordslists[i]:(pre3D*&)preZeroMemory)	
	template<class T, class I> static inline T &_CoGet(T *p, const I &i){ return p[i]; }	
	template<class List> static inline void _CoClear(List &list){ list._CoNew(0,list._list); }
	template<class List> static inline void _CoMoved(List &list){ /*list._size = 0;*/ }
	template<class List> inline void _CoReserve(size_t size, List &list)
	{	
		//TODO: RAISE A RELEASE MODE FLAG (put in pre-lists.hpp?)
		if((void**)&list._list==&preZeroMemory){ assert(preZeroMemory&&0); return; }
		//CAUTION: it isn't strictly correct to not //optimizing
		//propogate if _size is 0 & size is nonzero //doing _list unconditionally
		bool propagate = list._size!=size||size==0;	list._CoNew(size,list._list); 
		if(propagate) _CoReserve_noinline(size,list);
	}		
	template<class List> DAEDALUS_CALLMETHOD void _CoReserve_noinline(size_t size, List &list)
	{
		if(!Is2()) //simple, just do everything
		{
		list._CoNewIf(positionslist,size); list._CoNewIf(normalslist,size); 
		list._CoNewIf(tangentslist,size); list._CoNewIf(bitangentslist,size);
		for(preN i=0;i<colorslistsN;i++) list._CoNewIf(colorslists[i],size);
		for(preN i=0;i<texturecoordslistsN;i++) list._CoNewIf(texturecoordslists[i],size);		
		}
		else if(Mesh2()==this) //propagated up to mesh and back down to morphs
		{
			_CoReserve_noinline2(size,list);
			Mesh2()->MorphsList()^[&](PreMorph *ea){ ea->_CoReserve_noinline2(size,list); };
		}
		else if(Mesh2()->_complex==_complex) //propagate up to this morph's mesh
		{ Mesh2()->_CoReserve_noinline(size,list); }else assert(0);
	}
	template<class List> DAEDALUS_CALLMETHOD void _CoReserve_noinline2(size_t size, List &list)
	{
		if(&list._size==&_complex->positions) 
		list._CoNewIf(positionslist,size);
		else if(&list._size==&_complex->normals)
		{ list._CoNewIf(normalslist,size); 
		list._CoNewIf(tangentslist,size); list._CoNewIf(bitangentslist,size); 
		}else if(&list._size==&_complex->colors)
		for(preN i=0;i<colorslistsN;i++) list._CoNewIf(colorslists[i],size);
		else if(&list._size==&_complex->texturecoords)
		for(preN i=0;i<texturecoordslistsN;i++) list._CoNewIf(texturecoordslists[i],size);		
		else assert(0);
	}
	//Hmm: Assimp rules assume Bitangents here by convention, but it may not be so!
	inline bool HasTangentsAndBitangents()const{ return TangentsCoList().HasList(); }
	inline preN VertexColors(preN n=0)const{ return VertexColorsCoList(n).RealSize(); }	
	inline bool HasVertexColors(preN n=0)const{ return VertexColorsCoList(n).HasList(); }
	inline preN TextureCoords(preN n=0)const{ return TextureCoordsCoList(n).RealSize(); }
	inline bool HasTextureCoords(preN n=0)const{ return TextureCoordsCoList(n).HasList(); }	
	DAEDALUS_DEPRECATED("not meaningful. Use PreMesh's instead")
	inline preN CountVertexColorsLists()const;
	DAEDALUS_DEPRECATED("not meaningful. Use PreMesh's instead")
	inline preN CountTextureCoordsLists()const;

	DAEDALUS_DEPRECATED("porting Assimp? Try adding ,0")
	PreMorph(const PreMorph &cp); //not implementing
	PreMorph(const PreMorph &cp, const PreMesh2 *_mesh):PRESERVE_VERSION
	{	
		PRESERVE_COPYTHISAFTER(_size) 
		PRESERVE_COPYLIST(metadata)	_complex = _mesh; 	
		//pN is hard to explain, but if a PreMesh2::Morph it should 
		//be 0 and _mesh will be a local copy of PreMesh2. Otherwise
		//_mesh is the PreMesh2 being copied adjusted for differences
		//in _base, and so positions belongs to PreMorph--not PreMesh2
		preN pN = positions!=0?positions:Positions();
		PRESERVE_COPYLIST2(pN,positionslist) 
		PRESERVE_COPYLIST2(Normals(),normalslist)
		PRESERVE_COPYLIST2(Normals(),tangentslist)
		PRESERVE_COPYLIST2(Normals(),bitangentslist)
		for(preN i=0;i<colorslistsN;i++)
		PRESERVE_COPYLIST2(VertexColors(i),colorslists[i])		
		for(preN i=0;i<texturecoordslistsN;i++)
		PRESERVE_COPYLIST2(TextureCoords(i),texturecoordslists[i]) 		
	}	
	PreMorph():PRESERVE_VERSION{ PRESERVE_ZEROTHISAFTER(_size) }	
	~PreMorph()
	{
		MetaDataList().Clear();
		PositionsCoList().CoClearAll(); NormalsCoList().CoClearAll();		
		VertexColorsCoList().CoClearAll(); TextureCoordsCoList().CoClearAll();
	}				

	//avoid Assimp's specialization of std::min and std::max
	Pre3D::Container GetContainer(Pre3D::Container=Pre3D::MinMax())const;	
	  
	//moved from pre-mesh2.hpp to support morphs
	#define _(s) i<0?sub->s##list[-i]:s##list[i]	
	inline pre3D &Position2(PreMorph *sub, signed i){ return _(positions); }
	inline pre3D &Normal2(PreMorph *sub, signed i){ return _(normals); }
	inline pre3D &Tangent2(PreMorph *sub, signed i){ return tangentslist?_(tangents):_(normals); }
	inline pre3D &Bitangent2(PreMorph *sub, signed i){ return bitangentslist?_(bitangents):_(normals); }	
	inline const pre3D &Position2(const PreMorph *sub, signed i)const{ return _(positions); }
	inline const pre3D &Normal2(const PreMorph *sub, signed i)const{ return _(normals); }
	inline const pre3D &Tangent2(const PreMorph *sub, signed i)const{ return tangentslist?_(tangents):_(normals); }
	inline const pre3D &Bitangent2(const PreMorph *sub, signed i)const{ return bitangentslist?_(bitangents):_(normals); }	
	#undef _
	#define _(s) i<0?sub->s##lists[sub->s##lists[n]?n:0][-i]:s##lists[s##lists[n]?n:0][i]
	inline pre4D &VertexColor2(PreMorph *sub, preN n, signed i){ return _(colors); }
	inline const pre4D &VertexColor2(const PreMorph *sub, preN n, signed i)const{ return _(colors); }
	inline pre3D &TextureCoord2(PreMorph *sub, preN n, signed i){ return _(texturecoords); }
	inline const pre3D &TextureCoord2(const PreMorph *sub, preN n, signed i)const{ return _(texturecoords); }
	#undef _
	inline bool HasPositions2(const PreMorph *sub)const
	{ return HasPositions()||sub&&sub->HasPositions(); }	
	inline bool HasNormals2(const PreMorph *sub)const
	{ return HasNormals()||sub&&sub->HasNormals(); }	
	inline bool HasTangents2(const PreMorph *sub)const
	{ return HasTangents()||sub&&sub->HasTangents(); }	
	inline bool HasBitangents2(const PreMorph *sub)const
	{ return HasBitangents()||sub&&sub->HasBitangents(); }	
	inline bool HasVertexColors2(const PreMorph *sub, preN n=0)const
	{ return HasVertexColors(n)||sub&&sub->HasVertexColors(n); }	
	inline bool HasTextureCoords2(const PreMorph *sub, preN n=0)const
	{ return HasTextureCoords(n)||sub&&sub->HasTextureCoords(n); }	
//};