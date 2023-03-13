
//NEW: includes PreBone::Weight
//template<class T> struct Time //PreServe.h
//{			
	PRESERVE_LIST(typeof_this)

	typeofkey key; typeofvalue value; 	
	typeof_this(const typeofkey &k):key(k){}
	typeof_this(const typeofkey &k, const typeofvalue &v):key(k),value(v){}
	
	template<class T> DAEDALUS_DEPRECATED
	/*Assimp IS FAST & WILD WITH ITS KEYS!
	//Assimp/anim.h says used by std::find	
	inline bool operator==(const PreTime<T> &cmp)const{ return cmp.value==value; }
	inline bool operator!=(const PreTime<T> &cmp)const{ return cmp.value!=value; }
	//Assimp/anim.h says used by std::sort
	inline bool operator<(const PreTime<T> &cmp)const{ return key<cmp.key; }
	inline bool operator>(const PreTime<T> &cmp)const{ return key>cmp.key; }*/
	("Assimp use of == is deprecated. Consider adding == to local :: namespace")
	void operator==(T)const; //not implementing
	//this implements the < operator for optimized lookups
	inline operator const typeofkey&()const{ return key; }	

	//NEW: somewhat confusing, but why not?
	inline operator typeofkey&(){ return key; }
	template<typename T> //detangle ambiguity
	inline typeofkey &operator=(const T &cp){ return key = cp; }
	inline typeof_this &operator=(const typeof_this &cp)
	{ key = cp.key; value = cp.value; return *this; }

#undef typeof_this
//};