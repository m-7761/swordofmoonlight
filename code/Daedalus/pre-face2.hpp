
//struct PreFace2 : PreFace //PreServe.h
//{	
	typedef void Is2, This2;

	PRESERVE_LIST(PreFace2)

	struct Range //identical to PreMesh::Range
	{ 
		preN size; signed find;
		Range(preN s, signed f):size(s),find(f){} 
		inline operator signed&(){ return find; }
		inline operator signed()const{ return find; }
	};
	inline const Range &PositionsRange()const{ return (Range&)polytypeN; }	
	inline Range NormalsRange()const
	{ 
		return Range(normals?normalflag?1:polytypeN:0,startindex+normalsheadstart); 
	}
	inline Range TangentsRange()const 
	{
		switch(normals){ case 1: return NormalsRange(); case 0: return Range(0,0); }
		preN n = normalflag?1:polytypeN; return Range(n,startindex+normalsheadstart+n); 
	}
	inline Range BitangentsRange()const 
	{
		switch(normals){ case 1: return NormalsRange(); case 0: case 2: return Range(0,0); }
		preN n = normalflag?1:polytypeN; return Range(n,startindex+normalsheadstart+n+n); 
	}
	inline Range VertexColorsRange(preN m=0)const 
	{
		if(!colors||m>=PreMorph::colorslistsN) return Range(0,0);
		preN n = colorflag?1:polytypeN;	return Range(n,startindex+colorsheadstart+(colors==1?0:m*n)); 
	}
	inline Range TextureCoordsRange(preN m=0)const
	{
		if(!texturecoords||m>=PreMorph::texturecoordslistsN) return Range(0,0);
		return Range(polytypeN,startindex+texturecoordsheadstart+(texturecoords==1?0:m*polytypeN)); 
	}

	private: friend struct PreMesh2; 
	PreFace2(const PreFace2 &cp, int)
	:PreFace(offsetof(PreFace2,material),sizeof(*this),preNoTouching)
	{ PRESERVE_COPYTHISAFTER(_size) }
	public: PreFace2 &operator=(const PreFace2 &cp)
	{ memcpy(this,&cp,sizeof(PreFace2)); return *this; }
	PreFace2():PreFace(offsetof(PreFace2,material),sizeof(*this),preNoTouching)
	{ PRESERVE_ZEROTHISAFTER(_size) }	
//};