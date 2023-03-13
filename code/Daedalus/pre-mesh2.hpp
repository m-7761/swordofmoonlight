
//struct PreMesh2 //PreServe.h
//{	
	typedef void Is2, This2, Mesh2;

	//means fully supported by post-steps
	static const bool IsSupported = false;
	
	PRESERVE_LIST(PreMesh2*)
	//3: HasFaces is specialized below
	PRESERVE_LISTS3(preFace2,FacesList(),faces,faceslist)
	const PreList<signed*> IndicesSubList(const PreFace2::Range &r)
	{ 
		assert(indiceslist!=0); 
		return PreList<signed*>(r.size,r.find<0?submesh->indiceslist-r.find:indiceslist+r.find); 
	}
	const PreList<const signed*> IndicesSubList(const PreFace2::Range &r)const
	{ 
		assert(indiceslist!=0);
		return PreList<const signed*>(r.size,r.find<0?submesh->indiceslist-r.find:indiceslist+r.find); 
	}
	PRESERVE_LISTS(Morph*,Morphs,morphs)

	inline const PreMesh2&
	_MoveCP(const PreMesh2&__cp)
	{
		const PreMesh2 &_cp = 
		PRESERVE_MOVECP(PreMorph,__cp)
		return PRESERVE_MOVECP(PreMesh,_cp);
	}
	PreMesh2(const PreMesh2 &__cp, PreX::Pool *p)
	:PRESERVE_VERSION,PreMesh(__cp,p,&_MoveCP(__cp))
	{
		//cp = _MoveCP(__cp); //reusing result from above
		auto &cp = *PreMorph::_complex; PreMorph::_complex = this;
		PRESERVE_EXTENDBASE(PreMesh) PRESERVE_COPYTHISAFTER(_size)		
		if(faceslist) if(faces) //cross copy
		{
			const PreFace2 *copy = faceslist; 
			(void*&)faceslist = operator new[](faces*sizeof(PreFace2)); 
			for(preN i=0;i<faces;copy=copy->NextInFacesList2(++i==faces))
			new(faceslist+i)PreFace2(*copy,0);
		}
		else faceslist = 0;
		PRESERVE_DEEPCOPYLIST(morphs,this)
		if(cp.IsSubmesh()) submesh = this;
	}
	PreMesh2():PRESERVE_VERSION
	{
		_complex = this;
		PRESERVE_EXTENDBASE(PreMesh) 
		PRESERVE_ZEROTHISAFTER(_size) 
	}
	~PreMesh2()
	{ 
		FacesList().Clear(); MorphsList().Clear(); 
	}	

	inline bool HasPositions2()const{ return PreMorph::HasPositions2(submesh); }	
	inline bool HasNormals2()const{ return PreMorph::HasNormals2(submesh); }	
	inline bool HasTangents2()const{ return PreMorph::HasTangents2(submesh); }	
	inline bool HasBitangents2()const{ return PreMorph::HasBitangents2(submesh); }	
	inline bool HasVertexColors2(preN n=0)const{ return PreMorph::HasVertexColors2(submesh,n); }	
	inline bool HasTextureCoords2(preN n=0)const{ return PreMorph::HasTextureCoords2(submesh,n); }		
	inline preN CountVertexColorsLists2()const
	{
		preN n = PreMesh::CountVertexColorsLists();
		return submesh==0?n:std::max(n,submesh->PreMesh::CountVertexColorsLists());
	}
	inline preN CountTextureCoordsLists2()const
	{
		preN n = PreMesh::CountTextureCoordsLists();
		return submesh==0?n:std::max(n,submesh->PreMesh::CountTextureCoordsLists());
	}
//};