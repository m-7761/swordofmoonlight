
//struct PreX //PreServe.h
//{	
	PRESERVE_LIST(PreX) 

	enum{ lengthN=1024-1 }; //aiString

	//pointer-based comparators	
	PreX():cstring(),length(),_allocated(){}
	~PreX(){ if(!IsPooled()) delete[] cstring; }
	inline bool IsPooled()const{ return !_allocated; }
	inline void PoolString(const char *p, preN len=0)
	{
		this->~PreX(); cstring = p; _allocated = 0;
		length = len?len:strlen(p); assert(!cstring[length]);
	}	
	//NOTE: *this activates the pooling behavior
	inline bool operator==(const PreX &cmp)const
	{
		if(IsPooled()) return cmp.cstring==cstring;
		else return !cstring?!cmp.cstring||!*cmp.cstring:
		length==cmp.length&&!memcmp(cstring,cmp.String(),length);
    }
    inline bool operator!=(const PreX &cmp)const
	{ return !(*this==cmp); }

	//string data accessors
	inline preN Size()const{ return length; }
	inline const char *Pointer()const{ return cstring; }
	inline bool HasString()const{ return cstring&&*cstring; }		
	inline const char *String(const char *def="")const
	{ return cstring?cstring:def; }
	const char *DottedPathExtension()const
	{
		for(int i=length;i-->0;) switch(cstring[i])
		{ case '.': return cstring+i+1; case '/': case '\\': break; }
		return "";
	}

	//non-pooled constructors
    PreX(const PreX &cp):length(cp.length),_allocated()
    {
		if(!cp.IsPooled())
		{
			cstring = length?new char[_allocated=length+1]:0;
			if(cstring) memcpy((char*)cstring,cp.cstring,length+1);
			assert(!cstring[length]);
		}
		else cstring = cp.cstring;
    }	
	PreX(const char *cp):length(strlen(cp)),_allocated()
    {
		cstring = length?new char[_allocated=length+1]:0;
		if(cstring) memcpy((char*)cstring,cp,length+1);
    }	
	PreX(const std::string &cp):length(cp.size()),_allocated()
    {
		cstring = length?new char[_allocated=length+1]:0;
		if(cstring) memcpy((char*)cstring,cp.c_str(),length+1);
    }		  			

	template<class T> 
	DAEDALUS_DEPRECATED("please use SetString instead")
	inline PreX &operator=(T); //not implementing
	template<class T> inline PreX &SetString(const T &string)
	{
		this->~PreX(); new(this)PreX(string); return *this; 
	}	
	inline PreX &SetString(const char *cp, preN len)
	{
		this->~PreX(); length = len;
		cstring = length?new char[_allocated=length+1]:0;
		if(cstring) memcpy((char*)cstring,cp,length+1);
		return *this;
	}	
	
	//non-pooled buffering
   	PreX &Append(const char *app)   
	{
		if(!app) return *this;
		assert(_allocated>=length+1);
        const preN len = strlen(app);
		if(len>=_allocated-length)
		{
			const preN minalign = 32;
			_allocated = std::max(_allocated*2,length+len+1);
			_allocated+=minalign-_allocated%minalign;
			const char *swap = cstring;
			cstring = (char*)memcpy(new char[_allocated],swap,length);
			delete[] swap;
		}
		memcpy((char*)cstring+length,app,len+1); 
		length+=len; return *this;
    }    
    inline void Clear()
	{
		length = 0; if(_allocated) *(char*)cstring = '\0';
	}
	inline PreX &Assign(const char *sz)   
	{
		Clear(); Append(sz); return *this;
	}		

	//// RESERVED FOR INTERNAL USE ////

	inline PreNode *operator->()const{ return (PreNode*)_target; } 
	inline void operator=(PreNode *node)const{ (PreNode*&)_target = node; }
	inline uintptr_t &operator*()const{ return const_cast<uintptr_t&>(_target); }

	//// AUXILIARY STRUCTURES /////////

	struct Pool //for copy constructors
	{
		//default implementation
		virtual bool PoolString(PreX &flatcopied)
		{
			if(flatcopied.IsPooled()) return true;
			flatcopied._allocated = 0; 
			flatcopied.SetString(flatcopied.cstring);
			return false;
		}
		virtual bool PoolMaterialString(const char* &inout)
		{
			return false; //should do nothing if not pooling
		}
		virtual void GetDeleteBuffers(preN &x, char** &xlist)
		{
			x = 0; xlist = 0; //see PreScene copy constructor
		}
	};
	static Pool *GetPool(Pool *havepool)
	{
		if(havepool) return havepool; 
		static Pool nonpooling; return &nonpooling;
	}
//};