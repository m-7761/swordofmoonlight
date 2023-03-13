
#define _commingle _coclass(_colistee)

#ifndef PRE_LIST_VECTOR_ONLY //HACK: MSVC2015 workaround

//template<class T, class Co> class PreList //PreServe.h
//{			
	//TODO: x64 size_t may benefit from optimizing for preSize
	//TODO: add non-const accessors to work with PRESERVE_LIST
	//(in which case S/T will have to be const if the size is)
	//TODO: is_trivially_destructible v has_trivial_destructor

	//////////////////Messy Internals/////////////////////////

	static inline void *_coclass(void*){ return 0; }
	static inline void *_coclass(PreDefault*){ return 0; }
	template<class Co> static inline Co *_coclass(Co*){ return 0; }	
	////inner-core feature-set//////////
	typedef typename std::remove_pointer 
	<typename std::remove_reference<T>::type>::type _T;	
	typedef typename std::remove_reference<S>::type _S;
	typedef typename std::remove_const<_T>::type _nc_T;		
	template<class I, class Co> inline _T &_get(const I &i, Co*)const
	{ return _cc->_CoGet(_list,i); } //note: _contiguous needs _CoGet
	template<class I> inline _T &_get(const I &i, void*)const{ return _list[i]; }
	//_nc_T: ctors are non-const. Still const PreNew<const T[]> compiles Resize??
	template<class I, class CP> inline _T &_copy_construct(const I &i, const CP &cp)
	{ return *new((_nc_T*)&_get(i,_commingle))_T(cp); }	
	//TODO? sometimes delete[] is desired. It's hard to know when though
	inline void _delete_or_destruct(_S i, _S beyond, std::true_type ptr)
	{ while(i<beyond) delete (_get(i++,_commingle)); }
	inline void _delete_or_destruct(_S i, _S beyond, std::false_type ptr)
	{ while(i<beyond) _get(i++,_commingle).~_T(); }
	inline void _delete_or_destruct(_S i, _S beyond)
	{ if(_list) _delete_or_destruct(i,beyond,std::is_pointer<_T>::type()); }
	inline bool _renew(_S _new_size)const
	{
		assert(_new_size<=preMost);
		return _size==_new_size&&!_list==!_new_size; 
	}
	template<class Co> inline void _new(_S size, Co*)
	{ 
		if(_renew(size)) return; 
		_colistee->_CoReserve(size,*this); _size = size;
	}friend typename Co; //_CoNew provides a default _new to Co
	template<int sizeof_T, class X> inline void _CoNew(_S size, X &_list) 
	{ 
		_delete_or_destruct(size,_size);
		if(!size){ delete[] (void*)_list; _list = 0; return; } 
		const void *swap = _list; 
		(const void*&)_list = operator new[](sizeof_T*size);
		if(swap) memcpy((void*)_list,swap,sizeof_T*std::min(size,_size));		
		delete[] swap;
	}
	template<class X> inline void _CoNew(_S size, X &_list) 
	{ _CoNew<sizeof(*_list)>(size,_list); }
	template<class X> inline void _CoNewIf(X &_list, _S size) 
	{ if(_list) _CoNew(size,_list); }
	template<> inline void _new<void>(_S size, void*) 
	{
		if(_renew(size)) return; 
		_CoNew(size,_list); _size = size; //body is _CoNew
	}
	template<class Co> //zero memory for nontrivial destructor
	inline void _destructor_proof_new(_S size, Co *commingled)
	{ _S i = _size; _new(size,commingled); _destructor_proof(i,size); }	
	inline void _destructor_proof(_S i, _S beyond)
	{		
		if(std::has_trivial_destructor<_T>::value||i>=beyond) 
		return; _T *p = &_get(i,_commingle); 
		memset(p,0x00,(ptrdiff_t)&_get(beyond,_commingle)-(ptrdiff_t)p);
	}	
	template<class Co> inline void _clear(Co*)
	{ _colistee->_CoClear(*this); } //singles out list
	template<> inline void _clear(void*){ Resize(0); }		
	template<class Co> inline void _moved(Co*)
	{ _colistee->_CoMoved(*this); } //singles out list
	template<> inline void _moved(void*){ _size = 0; }
	////construction/assignment///////////////////////
	template<class Co> inline void _default(Co*){} //constructor
	template<> inline void _default(PreDefault*){ _size = 0; _list = 0; }
	template<class Co> inline void __default(Co*){} //destructor
	template<> inline void __default(PreDefault*){ Clear(); }	
	template<class Co> inline void _reseat_friendly(Co*){}	
	template<> inline void _reseat_friendly(PreDefault*)
	{ static_assert(0,"PreDefault/PreNew cannot be reseated"); }
	template<class Co> inline void _no_default(){}
	template<> inline void _no_default<PreDefault>()
	{ static_assert(0,"PreDefault must be default-constructed"); }
	typedef const PreList<_T*const, //sublist constructors
	typename PreContiguity<Co>::Voided::type> _View;	
	template<class Co> inline _View _view(_S size, _T *list, Co *cc)const
	{ assert(_list); return _View(size,list,cc); }
	template<> _View inline _view<void>(_S size, _T *list, void *cc)const
	{ assert(_list); return _View(size,list); }

public: ////Core Operators & Methods//////////////////////////
	
	template<class _S> //NEW: ensure correct binding
	//just an economical way to write if(l.HasList()) 
	//For example: for(size_t i=l;i<l.Size();i++){...}
	inline operator _S()const{ return !_list?_size:0; }	

	//^ and ^= do for-each style iteration over the list
	//To avoid confusion write ea/each in ^ and i/j in ^=	
	//VS2010: without const a nested lambda expression has
	//"int" type. const is removed to allow mutable lambdas
	template<class Lambda> inline _S operator^=(const Lambda &f)const
	{ _S i = 0; if(_list) while(i<_size) ((Lambda&)f)(i++); return i; }
	template<class Lambda> inline _S operator^(const Lambda &f)const
	{ _S i = 0; if(_list) while(i<_size) ((Lambda&)f)(_get(i++,_commingle)); return i; }
	
	//(especially for co-lists) & of [] may not be an array
	template<class I> inline _T &operator[](const I &i)const
	{ assert(i>=0&&(_S)i<_size); return _get(i,_commingle); } 
	//access to [] is bound checked. &= or "tie-off" is useful
	//to diagnose cases where iteration falls short of its goal
	//&= looks like shoelaces; & evokes A for assert, even if Et
	template<class I> void operator&=(const I &i)const{ assert(i==RealSize()); }
	template<class P> void operator&=(P *p)const{ assert(p==&_get(RealSize(),_colistee)); }
	
	//NEW: support for PreDefault/PreNew
	template<class> friend struct PreNew; //#include "pre-pre.hpp"	
	PreList(){ _default(_colistee); } ~PreList(){ __default(_colistee); }	
	PreList(const PreList &cp):_size(cp._size),_list(cp._list),_cc(cp._cc)
	{ _reseat_friendly(_colistee); } //PreDefault cannot be reseated
	PreList(PreList &&mv):_size(mv._size),_list(mv._list),_cc(mv._cc)
	{ mv.MovePointer(); } //workaround PreDefault cannot be reseated
	inline PreList &operator=(const PreList &cp){ return *new(this)PreList(cp); }
	
	//first-class objects used classical constructors
	PreList(S size, T list):_size(size),_list(list){ _no_default<Co>(); } 
	PreList(S size, T list, Co *co):_size(size),_list(list),_colistee(co){ _no_default<Co>(); }		
	PreList(S size, T list, const Co *cc):_size(size),_list(list),_cc(cc){ _no_default<Co>(); }
	
	//THIS IS THE INNER-CORE FUNCTIONS SET
	inline _S Size()const{ return _size; }		
	inline _T *Pointer()const{ return _list; }		
	inline _S RealSize()const{ return _list?_size:0; }
	inline bool HasList()const{ return _size&&_list; }
	inline void Reserve(_S size) //do not construct elements
	{ 	
		//new allocate unless size-IS-RealSize!
		//0-fill (not construct) unless has_trivial_destructor
		_destructor_proof_new(size,_commingle); 
	}//PLEASE prefer Assign when setting ALL values
	inline void Resize(_S size, const _T &set=_T()) //constructs
	{
		_S i = _list?_size:0; _new(size,_commingle);
		while(i<_size) _copy_construct(i++,set);
	}	
	inline _T *MovePointer() //todo? ensure it's moved (somehow??)
	{
		_T *out = _list; _list = 0; _moved(_commingle); return out; 
	}
	inline bool ForgoMemory(_S i) //Reserve/Resize if you're new to C++
	{  	
		if(i<=_size&&i>=0) _size = i; else assert(0); return i>0;
	}
	inline void Reserve_Forgo_Clear(_S i) //Assimp does this at least once
	{
		if(i>RealSize()) Reserve(i); else if(!ForgoMemory(i)) CoClearAll();
	}
	inline void Clear(){ _clear(_commingle); }
	inline void CoClearAll(){ _new(0,_commingle); _size = 0; }
			  	
	////LIST//////////////////////////////////////////////////

	//these "range" methods are not bound-checked
	typedef _View List; inline List AsList()const
	{ return _view(_size,_list,(List::Co*)_cc); }
	template<class S> List AsList(const S &s)const
	{ assert(s<=_size); return _view(s,_list,(List::Co*)_cc); }
	template<class S, class I> List AsList(const S &s, const I &i)const
	{ assert(i+s<=_size); return _view(s,&_get(i,_commingle),(List::Co*)_cc); }
	inline operator List()const{ return AsList(); }
	//some more unorthodox operators to go with ^
	template<class S> inline List operator<(const S &s)const
	{ assert((_S)s<=_size); return _view(s,_list,(List::Co*)_cc); }
	template<class S> inline List operator<=(const S &s)const
	{ assert((_S)s<_size); return _view(s+1,_list,(List::Co*)_cc); }	
	template<class I> inline List operator>=(const I &i)const
	{ assert((_S)i<=_size); return _view(_size-i,&_get(i,_commingle),(List::Co*)_cc); }
	template<class I> inline List operator>(const I &i)const
	{ assert(_S(i+1)<=_size); return _view(_size-(i+1),&_get(i+1,_commingle),(List::Co*)_cc); }		

	////VECTOR////////////////////////////////////////////////

	//No checks are done as Begin & End must do. Equivalent to []
	inline _T &Front(){ assert(_size>0); return _get(0,_commingle); }
	inline _T &Back(){ assert(_size>0); return _get(_size-1,_commingle); }

	//HACK: MSVC2015's compiler crashes on the following inline definition.
	class Vector;

#else //PRE_LIST_VECTOR_ONLY

	/*class*/ Vector : public PreList<T,Co> //std::vector-like
	{	
		S _capacity; inline void _destructor_proof_grow(_S to)
		{ _S i = _capacity; _grow(to); _destructor_proof(i,_capacity); }
		DAEDALUS_CALLMETHOD void _grow(_S to) //can use more thought
		{
			_S back = _size;
			_new(std::max<preN>(to,_capacity<=8?16:_capacity*2),_commingle);			
			_capacity = _size; _size = back; 
		}				

	public: inline _S Capacity()const{ return _capacity; }

		Vector():PreList(){ _capacity = 0; } //PreDefault form
		Vector(const PreList &cp):PreList(cp),_capacity(_size) 
		{	
			//guarantee Size() returns the real size. RealSize is unmeaningful
			if(!_list&&_size){ _destructor_proof_grow(_capacity); _size = 0; } 
		}						
		inline Vector &operator=(const Vector &cp){ return *new(this)Vector(cp); }

		inline _T &PushBack(const _T &set=_T())
		{
			Reserve(_size+1); return _copy_construct(_size++,set);
		}
		inline void PopBack() 
		{	
			_delete_or_destruct(_size-1,_size); assert(_size);			
			_destructor_proof(_size-1,_size--); 
		}
		inline void Reserve(_S size) 
		{
			if(_capacity<size) _destructor_proof_grow(size); 			
		}
		//PLEASE prefer Assign to set ALL values
		inline void Resize(_S size, _T set=_T())
		{
			if(_capacity<size) _grow(size);
			for(_S i=_size;i<size;i++) _copy_construct(i,set); _size = size;
		}						
		inline void Resize(_S size, PreNoTouching) //Vector exclusive
		{
			Reserve(size); _size = size;			
		}					
		inline _T *MovePointer()
		{
			_capacity = 0; return PreList::MovePointer(); 
		}
		inline void Clear() 
		{
			_delete_or_destruct(0,_size); _size = 0;
		}		
		inline void CoClearAll() //for completeness sake only
		{
			PreList::CoClearAll(); _capacity = _size; //0
		}
		template<class CP> inline void Assign(_S size, const CP &data) //non-core
		{
			if(_capacity<size) _grow(size);
			_delete_or_destruct(0,_size); _copy_or_fill(0,size,data); _size = size;
		}
		template<class MV> inline void Have(MV &&mv) //non-core
		{ 
			MV::_swapcapacity(mv,*this); PreList::Have(std::forward(mv)); 
		}	
		template<class MV> inline void Swap(MV &&mv) //likewise
		{ 
			MV::_swapcapacity(mv,*this); std::swap(mv._size,_size); std::swap(mv._list,_list); 
		}
		template<class,class> friend class PreList;
		DAEDALUS_DEPRECATED("Vector's Size is true. Is Capacity desired?")
		void RealSize()const; //not implementing
		DAEDALUS_DEPRECATED("Difficult to implement/not recommended at all")
		void ShrinkToFit()const; //not implementing
		DAEDALUS_DEPRECATED("Not meaninful in this context. Hiding")
		void Reserve_Forgo_Clear(_S)const; //not implementing
	};	

#endif //PRE_LIST_VECTOR_ONLY
#ifndef PRE_LIST_VECTOR_ONLY

	DAEDALUS_DEPRECATED("please make do with PreList")
	inline const Vector AsVector()const; //not implementing	

	//////////////////Non-Core Methods////////////////////////
	//(DON'T FORGET TO IMPLEMENT Vector'S VERSION IF NEEDED)//
	//////////////////////////////////////////////////////////

private: //messy non-core internals
	
	//contiguous if _CoGet is static
	static inline bool _contiguous() //pre-pre.hpp
	{ return !PreContiguity<Co>::_noncontiguous<Co>::yes::value; }	
	static inline bool _rawcopyable() //heuristical
	{ return _contiguous()&&std::has_trivial_destructor<_T>::value; }				
	template<class CP> inline void _copy(_S i, _S beyond, const CP *data)
	{
		if(_rawcopyable()&&(std::is_same<_T*,std::decay<CP>::type>::value
		||std::is_integral<CP>::value&&sizeof(CP)==sizeof(_T)&&std::is_integral<_T>::value))
		{ if(i<beyond) memcpy(_list+i,data,sizeof(_T)*(beyond-i)); return; }
		for(;i<beyond;i++) _copy_construct(i,data[i]); 
	}		
	template<class CP> inline void _copy_or_fill(_S i, _S beyond, const CP &cp, std::true_type copy) 
	{ _copy(i,beyond,cp); }
	template<class CP> inline void _copy_or_fill(_S i, _S beyond, const CP &cp, std::false_type copy) 
	{ for(;i<beyond;i++) _copy_construct(i,cp); }
	template<class R> struct _points{ enum{ value=0 }; }; //recursive template
	template<class R> struct _points<R*>{ enum{ value=1+_points<R>::value }; };
	template<class R> struct _points<R*&>{ enum{ value=1+_points<R>::value }; };			
	template<class CP> inline void _copy_or_fill(_S i, _S beyond, const CP &cp)
	{ _copy_or_fill(i,beyond,cp,std::integral_constant<bool,_points<CP>::value==_points<T>::value>::type()); }	
	//List,List Swap & List,Vector Swap
	template<class MV> static inline void _swapcapacity(MV&){} 	
	template<> static inline void _swapcapacity(Vector &v){ v._capacity = v._size; } 			
	//Vector implementation of Have & Swap
	template<class V, class MV> static inline void _swapcapacity(MV &l, V &v){ v._capacity = l._size; }
	template<class V> static inline void _swapcapacity(Vector &v, V &v2)
	{ std::swap(v._capacity,v2._capacity); } 	
	//std::less requires #include <functional>
	static inline bool _less(const _T &a, const _T &b){ return a<b; }

public: typedef T T; typedef Co Co; //cannot hide S
	
	//FYI: Item::Type is _T
	typedef PreItem<T,Co> Item; //STL iterator
	Item Begin()const{ return Item(Pointer(),_cc); }
	Item End()const{ return Item(Pointer()+RealSize(),_cc); }
	typedef PreItem<T,Co,-1> _Item; //reverse_iterator
	_Item EndBegin()const{ return _Item(Pointer()+RealSize()-1,_cc); }
	_Item EndEnd()const{ return _Item(Pointer()-1,_cc); }						
	//ASSIGNMENT FROM std::vector::size & data
	template<class CP> inline void Assign(_S size, const CP &data)
	{ _delete_or_destruct(0,_size); Reserve(size); _copy_or_fill(0,_size,data); }	
	template<class CP> inline void Copy(const CP &cp)
	{ 
		preN s = cp.RealSize();
		_delete_or_destruct(0,_size); Reserve(s); 
		if(cp._rawcopyable()) return _copy(0,s,cp.Pointer());
		else while(s-->0) _copy_construct(s,cp[s]); 
	}
	//MOVE-SEMANTICS: you must write std::move
	template<class MV> inline void Have(MV &mv); //VS2010???
	template<class MV> inline void Have(MV &&mv)
	{ assert(!HasList()); _size = mv._size; _list = mv.MovePointer(); }		
	template<class MV> inline void Swap(MV &&mv)
	{ std::swap(mv._size,_size); std::swap(mv._list,_list); MV::_swapcapacity(mv); }	
	template<class,class> friend class PreList;
	//KEYS-BINSEARCH:
	template<class F> inline void Sort(const F &predicate)
	{ std::sort(Begin(),End(),predicate); }
	template<class F> inline bool IsSorted(const F &predicate)const
	{ return std::is_sorted(Begin(),End(),predicate); }
	inline void Sort(){ Sort(_less); }
	inline bool IsSorted()const{ return IsSorted(_less); }
	inline _T *PairKey(const _T &key)const{ return PairKey(key,_less); }	
	template<class F> inline _T *PairKey(const _T &key, const F &predicate)const
	{ 	//STOP! the returned pointer may be/become noncontiguous
		auto lb = std::lower_bound(Begin(),End(),key,predicate); 
		if(lb==End()||predicate(key,*lb)) return 0; return &*lb;
	}
//};

#endif //PRE_LIST_VECTOR_ONLY

#undef _commingle //_coclass(_colistee)