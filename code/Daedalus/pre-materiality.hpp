
//struct Prematerial //PreServe.h
//{			
	//struct Property
	//{
		PRESERVE_LIST(Property*)
		PRESERVE_LISTS(preID,MetaData,metadata)
	
		//AddProperty(preNoTouching)
		inline void PoolString(const char *ps) //assuming stringtype
		{ if(!_sizeofdatalist) _datalist = (char*)ps; else assert(0); }		
		inline bool IsPooledString()const{ return !_sizeofdatalist; } 						
		
private: //hidden unsavory un-type-safe methods

		template<class S> inline S &_unsafe_cast()const 
		{ return sizeof(S)<=sizeof(_datalist)?(S&)_datalist:*(S*)_datalist; }	
		template<> inline preX &_unsafe_cast()const; //not implementing
		inline const char *_unpool_cast()const //spawn of MemoryCompare
		{ return _sizeofdatalist>sizeof(_datalist)?_datalist:(char*)&_datalist; }

public: //these are their type-safe corollaries

		//TODO? can check key in _DEBUG mode
		template<class S, class T> inline S&
		Datum(Key(*(const S&,T))()){ return _unsafe_cast<S>(); }
		template<class S, class T> inline const S& 
		Datum(Key(*(const S&,T))())const{ return _unsafe_cast<S>(); }						
		template<class T> inline const char* //non-const only for strings
		Datum(Key(*(const preX&,T))())const{ return CharPointer(); }				
		inline bool HasData()const{ return _datalist!=0; }
		inline preN Data()const
		{
			switch(key().storagetype)
			{
			case inttype: return _sizeofdatalist/sizeof(int);
			case floattype: return _sizeofdatalist/sizeof(float);
			case doubletype: return _sizeofdatalist/sizeof(double);
			case stringtype: //falling through
			if(IsPooledString()&&_datalist) return strlen(_datalist)+1;
			}return _sizeofdatalist;
		}
		template<class T> PreList<T> DataList(T); //not implementing anytime soon
		
		//these work equally well for all types
				
		inline const char *CharPointer()const
		{ return IsPooledString()?_datalist:_unpool_cast(); }		
		inline preSize SizeOfMemory()const 
		{ return IsPooledString()?strlen(_datalist)+1:_sizeofdatalist; }		
		inline int MemoryCompare(const Property *right)const 
		{	
			if(IsPooledString()) //avoid calling strlen
			return !right->IsPooledString()?-1:right->_datalist-_datalist; 
			else if(right->IsPooledString()) return +1;
			int c = right->_sizeofdatalist-_sizeofdatalist; if(c) return c;
			return memcmp(_unpool_cast(),right->_unpool_cast(),_sizeofdatalist);
		}

		//miscellanea						
		inline void EmbedTexture(preN i) //Assimp * notation 
		{ 
			if(!_sizeofdatalist&&key==_texture&&i<=99) //"*99"
			{
				_sizeofdatalist = 2+(i>9)+1; //long long is VS2010
				DAEDALUS_SUPPRESS_C(4996) //_SCL_SECURE_NO_WARNINGS
				strcpy(_c+1,std::to_string((long long)i).c_str())[-1] = '*'; 				
			}
			else assert(0); //AddProperty(preNoTouching)->EmbedTexture
		}
		inline bool IsEmbeddedTexture()const //assuming a number if so
		{ return key==_texture&&*CharPointer()=='*'&&SizeOfMemory()<=4; }		
		inline const preID EmbeddedTexture()const
		{ assert(IsEmbeddedTexture()); return strtol(CharPointer()+1,0,10);	}				

		private: friend struct PreMaterial; 
		Property(const Property&cp, PreX::Pool*p, Key(*k)()):PRESERVE_VERSION
		,_sizeofdatalist(cp._sizeofdatalist),texturepair(cp.texturepair),key(k)
		,_datalist(cp._datalist)
		{ 
			PRESERVE_COPYTHISAFTER(key) 
			PRESERVE_COPYLIST(metadata) //NEW	 
			switch(key().storagetype) //TODO? pass in parameter
			{		
			case stringtype: //pooling strings?
			{				
				const char *string = CharPointer();
				if(PreX::GetPool(p)->PoolMaterialString(string))
				{
					_datalist = (char*)string; _sizeofdatalist = 0; break;
				}
			}
			default: if(_sizeofdatalist<=sizeof(_datalist)) break; 
				
				PRESERVE_COPYLIST2(_sizeofdatalist,_datalist); 
			} 
		}
		Property(Texture t, preN n, Key(*k)())
		:PRESERVE_VERSION,_sizeofdatalist(),texturepair(MakeTexturePair(t,n)),key(k)
		,metadata(0),metadatalist(){} public: ~Property()
		{
			if(_sizeofdatalist>sizeof(_datalist)) delete[] _datalist; //NEW
			MetaDataList().Clear();	
		}		
		static int MakeTexturePair(Texture t, preN n){ return t<<16|n; }
	//};
//};