
#ifndef DAEDALUS_H_INCLUDED
//FYI: you can wrap namespace around
//PreServe.h. eg. nameaspace Daedalus
//or not. May help with DAPP namespace
//NOTE: if PreServe.h includes Daedalus
//the errors are confused, so it doesn't
#error PreServe.h depends on Daedalus.h
#endif

//Migrating from Visual Studio 2010?
#if defined(_MSC_VER) && _MSC_VER>=1900 //MSVC2015
#define has_trivial_destructor is_trivially_destructible 
#endif

//pre-declaring
template<class> struct PreNew;
struct PreNode; struct PreFace; 
struct PreFace2; struct PreMesh2; union PreMatrix;

//THE STATIC CROSS-COPYING SCHEME
#define PRESERVE_FIRSTCLASS const short _base, _size;\
int:32; //reserved/padding if x64
#define PRESERVE_SUPPRESSING_C4355(x) \
offsetof(std::remove_reference<decltype(*this)>::type,x)
#define PRESERVE_VERSION \
_base(PRESERVE_SUPPRESSING_C4355(_base)),_size(sizeof(*this))
//EXTENSION REQUIRES ZERO PADDING
#define PRESERVE_ZEROTHISAFTER(x) \
memset((void*)(&x+1),0x00,(ptrdiff_t)this+sizeof(*this)-intptr_t(&x+1));
//PARANOIA (BASE CLASS ALIGNMENT)
#define PRESERVE_EXTENDBASE(base) \
int padding = (char*)&_base-(char*)this-sizeof(base);\
if(padding) memset((char*)this+sizeof(base),0x00,padding);\
if(padding) const_cast<short&>(base::_size)+=padding;

//MESSY BOUNDARY CROSSING COPYING
#define PRESERVE_MOVECP(base,cp) \
((decltype(cp)&)*((char*)&cp+(sizeof(base)-cp.base::_size)));
#define PRESERVE_COPYTHISAFTER(x) {\
int movablesize = cp._size-cp._base+_base;\
int sameversion = ptrdiff_t(this+1)-intptr_t(&x+1);\
if(sizeof(*this)==movablesize) \
memcpy((void*)(&x+1),&cp.x+1,sameversion);\
else{\
int remainder = sizeof(*this)-movablesize;\
int copyable = ptrdiff_t(&cp)+movablesize-intptr_t(&cp.x+1);\
if(remainder<0) copyable+=remainder;\
if(copyable>0&&copyable+remainder<=sameversion)\
memcpy((void*)(&x+1),(void*)(&cp.x+1),copyable);\
else remainder = sameversion;\
if(remainder>0) memset((char*)(this+1)-remainder,0x00,remainder); }}\
//NEW COPY-BYPASSING CONSTRUCTORS /*eg.meshes/mesheslist2*/
#define PRESERVE_COPYLIST2(x,y) { /*assert(((x)==0)==!y);*/\
/*despite assert above, new[](0) is desired for the validation step*/\
if(y) (void*&)y = memcpy(operator new[](x*sizeof(*y)),y,x*sizeof(*y)); }
#define PRESERVE_COPYLIST(x) PRESERVE_COPYLIST2(x,x##list)
//NEW COPY-INCLUDING CONSTRUCTORS
#define PRESERVE_DEEPCOPY(x,...) { if(x) \
x = new std::remove_reference<decltype(*x)>::type(*x,__VA_ARGS__); }
//lazy: could hold onto a copy of y...
#define PRESERVE_DEEPCOPYLIST2(x,y,...) { PRESERVE_COPYLIST2(x,y)\
if(y) for(size_t _i=0;_i<x;_i++) PRESERVE_DEEPCOPY(y[_i],__VA_ARGS__); }
#define PRESERVE_DEEPCOPYLIST(x,...) PRESERVE_DEEPCOPYLIST2(x,x##list,__VA_ARGS__)

//LIGHTWEIGHT-HIGHLEVEL INTERFACE
#define PRESERVE_LISTS3(x,y,z,w) \
inline PreList<x*&> y { return PreList<x*&>(z,w); }\
inline const PreList<const x const*> y const{ return PreList<const x const*>(z,w); }
#define PRESERVE_LISTS2(x,y,z,w) PRESERVE_LISTS3(x,y##List(),z,w)\
inline bool Has##y()const{ return y##List().HasList(); }\
inline preSize y()const{ return y##List().RealSize(); }
#define PRESERVE_LISTS(x,y,z) PRESERVE_LISTS2(x,y,z,z##list)

#define PRESERVE_COLISTS2(co,x,y,z,w) \
inline PreList<x*&,co> y { return PreList<x*&,co>(z,w,this); }\
inline const PreList<const x const*> y const{ return PreList<const x const*>(z,w); }
#define PRESERVE_COLISTS(co,x,y,z,w) PRESERVE_COLISTS2(co,x,y##CoList(preN=0),z,w)\
inline bool Has##y()const{ return y##CoList().HasList(); }\
inline preSize y()const{ return y##CoList().RealSize(); }
#define PRESERVE_CONSTLISTS(co,x,y,z,w) \
inline PreList<x*&,co> y##CoList(){ return PreList<x*&,co>(z,w,this); }\
inline const PreList<const x const*,co> y##CoList()const{ return PreList<const x const*,co>(z,w,this); }\
inline bool Has##y()const{ return y##CoList().HasList(); }\
inline preSize y()const{ return y##CoList().RealSize(); }

//delcare same type as PreList::List
template<class,class> class PreList;
//WORK-IN-PROGRESS: TODO: remove const in favor of const accessors
#define PRESERVE_LIST(x) typedef const PreList<x*const,void> List;

//NEW: std::iterator business 
template<class,class,ptrdiff_t> class PreItem;
template<class T, class Co, ptrdiff_t Sense>
struct std::iterator_traits<class PreItem<T,Co,Sense>>
{ typedef typename std::remove_pointer
<typename std::remove_reference<T>::type>::type value_type;
typedef std::random_access_iterator_tag iterator_category;
typedef ptrdiff_t difference_type, distance_type;
typedef value_type *pointer, &reference;
};enum PreDefault; //pre-declaring
//TODO: take a typed _CoGet parameter
template<class Co> struct PreContiguity
{	//moved from PreList so PreItem
	//parameters are symmetric/clean
	template<class T> T static _msvc201X(T);
	template<class Co> struct _noncontiguous
	{ typedef typename std::is_member_function_pointer
	<decltype(_msvc201X(&Co::_CoGet<int*,int>))>::type yes; };
	template<> struct _noncontiguous<void>{ typedef std::false_type yes; };
	template<> struct _noncontiguous<PreDefault>{ typedef std::false_type yes; };		
	template<class> struct Noncontiguous //const Co* Wrapper
	{
		const Co *_cc; Noncontiguous(const Co *_cc=0):_cc(_cc){}
		template<class _T>
		inline _T &_get(_T *p, ptrdiff_t i)const{ return _cc->_CoGet(p,i); }
		template<class _T>
		inline ptrdiff_t _diff(_T *p, _T *q)const{ return _cc->_CoDiff(p,q); }
		const Co *_C()const{ return _cc; } 
		typedef typename Co type; //Voided
	};
	template<> struct Noncontiguous<std::false_type> //Empty Base Optimization
	{
		Noncontiguous(const Co *_cc=0){}
		template<class _T>
		static inline _T &_get(_T *p, ptrdiff_t i){ return p[i]; }
		template<class _T>
		static inline ptrdiff_t _diff(_T *p, _T *q){ return p-q; }
		static const Co *_C(){ return 0; }
		typedef void type; //Voided
	};
	typedef Noncontiguous<typename _noncontiguous<Co>::yes> ProbablyEBO,Voided; 	
};
template<class T, class Co, ptrdiff_t Sense=1> 
class PreItem : PreContiguity<Co>::ProbablyEBO
{ 
public: typedef typename //PreList::Item::Type
std::iterator_traits<PreItem>::value_type Type;
private: Type *_got; 
typedef typename PreContiguity<Co>::ProbablyEBO _Base;
inline Type *_get(ptrdiff_t off)const{ return &_Base::_get(_got,off); }
inline ptrdiff_t _diff(Type *ptr)const{ return _Base::_diff(_got,ptr); }
public: PreItem():_got(){}
PreItem(Type *got, const Co *cc):_Base(cc),_got(got){}
inline Type &operator*()const{ return *_got; }
inline Type *operator->()const{ return _got; }
inline Type &operator[](ptrdiff_t off)const{ return *_get(off); }
inline PreItem operator+(ptrdiff_t off)const{ return PreItem(_get(Sense*off),_C()); }
inline PreItem operator-(ptrdiff_t off)const{ return PreItem(_get(-Sense*off),_C()); }
inline ptrdiff_t operator-(const PreItem &right)const{ return Sense*_diff(right._got); }
inline bool operator==(const PreItem &right)const{ return _got==right._got; }
inline bool operator!=(const PreItem &right)const{ return _got!=right._got; }
inline bool operator<(const PreItem &right)const{ return _got<right._got; }
inline bool operator<=(const PreItem &right)const{ return _got<=right._got; }
inline bool operator>(const PreItem &right)const{ return _got>right._got; }
inline bool operator>=(const PreItem &right)const{ return _got>=right._got; }
inline PreItem &operator++(){ _got = _get(Sense); return *this; }
inline PreItem operator++(int){ Type *had = _got; ++*this; return PreItem(had,_C()); }  
inline PreItem &operator--(){ _got = _get(-Sense); return *this; }
inline PreItem operator--(int){ Type *had = _got; --*this; return PreItem(had,_C()); }
inline PreItem &operator+=(ptrdiff_t off){ _got = _get(Sense*off); return *this; }
friend inline PreItem operator+(ptrdiff_t off, const PreItem &right)
{ return PreItem(right._get(Sense*off),_C()); }
inline PreItem &operator-=(ptrdiff_t off){ _got = _get(-Sense*off); return *this; }
};//not doing a const variant of PreItem so:
template<class T, class Co, ptrdiff_t Sense> 
static inline PreItem<const T,Co,Sense> &PreConst(PreItem<T,Co,Sense> &t)
{ return (PreItem<const T,Co,Sense>&)t; }
//simplifying PreList template parameters
template<class S, class T> struct PreCopyRef
{
	template<class> struct RefIf{ typedef S type; };
	template<> struct RefIf<std::true_type>{ typedef S &type; };
	typedef typename RefIf<typename std::is_reference<T>::type>::type type; 
};