
//struct PreFace //PreServe.h
//{	
	PRESERVE_LIST(PreFace)
	bool Is2()const{ return _size2>_size1; }
	inline PreFace2 *This2(){ return (PreFace2*)this; }
	inline const PreFace2 *This2()const{ return (PreFace2*)this; }		
	struct Range //PreMesh::IndicesSubList
	{ 
		preN size; signed find;
		Range(preN s, signed f):size(s),find(f){} 
		inline operator signed&(){ return find; }
		inline operator signed()const{ return find; }
	};		
	inline const Range &PositionsRange()const{ return (Range&)polytypeN; }
	
	private: friend struct PreFace2; 
	PreFace(char size, char size2, PreNoTouching)
	:_size1(size),_size2(size2),_size(size2){}
	private: friend struct PreMesh; 
	inline const PreFace *NextInFacesList(bool end)const
	{ auto out = &_size1+_size; assert(end||out[0]==_size); return (const PreFace*)out; }
	private: friend struct PreMesh2; 
	inline const PreFace2 *NextInFacesList2(bool end)const
	{ auto out = &_size1+_size; assert(end||out[1]==_size); return (const PreFace2*)out; }
	static const int _base = 0;
	PreFace(const PreFace &cp, int)
	:_size1(sizeof(PreFace)),_size2(_size1),_size(_size1)
	{ PRESERVE_COPYTHISAFTER(_size) } 
	public: PreFace &operator=(const PreFace &cp)
	{ memcpy(this,&cp,sizeof(PreFace)); return *this; }
    PreFace():_size1(sizeof(PreFace)),_size2(_size1),_size(_size1)
	{ PRESERVE_ZEROTHISAFTER(_size) }	
//};