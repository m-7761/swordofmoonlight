
#ifndef SWORDOFMOONLIGHT_INCLUDED
#define SWORDOFMOONLIGHT_INCLUDED

#include <string.h> //memset?

#ifndef _WIN32
#include <stdint.h>
#else
#undef small /*rpcndr.h*/
#if !defined(_STDINT)/*MSVC2015*/\
 && !defined(SWORDOFMOONLIGHT_NSTDINT)
typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#endif
#endif	

/*HACK: int8_t uses signed keyword
//NOTE: char8_t is part of C++20
//#define cint8_t char*/
#define cint8_t char

/*Big Endian will be C++ only*/
#ifndef SWORDOFMOONLIGHT_BIGEND
#ifndef lefp_t
#define lefp_t float
#define le16_t int16_t
#define ule16_t uint16_t
#define le32_t int32_t
#define ule32_t uint32_t
#endif
#else
#error //todo: templates
#endif

/*/notes on integers (in this library)
//int is often used instead of size_t or unsigned just because
//it is shorter. Don't assume it is tested for negativity unless
//that would make sense. In a few instances negative values can
//wrap backwards when accessing array-like items. Where this is
//possible an explicit 'signed' modifier preceeds 'int' types*/

#ifdef _WIN32
typedef wchar_t swordofmoonlight_lib_char_t;
typedef const wchar_t* swordofmoonlight_lib_text_t;
typedef const wchar_t* swordofmoonlight_lib_file_t;
/*we want to encourage adoption of UTF-8 over ANSI file formats*/
#define SWORDOFMOONLIGHT_FOPEN(fname,mode) _wfopen(fname,L##mode L", ccs=UTF-8"); 
#define SWORDOFMOONLIGHT_FGETS fgetws
/*note: this only works cleanly because Windows sees %s as wchar_t*/
#define SWORDOFMOONLIGHT_SNPRINTF(dst,n,fmt,...) swprintf(dst,n,L##fmt,__VA_ARGS__)
#else
typedef char swordofmoonlight_lib_char_t;
typedef const char* swordofmoonlight_lib_text_t;
typedef const char* swordofmoonlight_lib_file_t;
#define SWORDOFMOONLIGHT_FOPEN(fname,mode) fopen(fname,mode); 
#define SWORDOFMOONLIGHT_FGETS fgets
#define SWORDOFMOONLIGHT_SNPRINTF(dst,n,...) snprintf(dst,n,__VA_ARGS__)
#endif
#define SWORDOFMOONLIGHT_SPRINTF(dst,...) \
SWORDOFMOONLIGHT_SNPRINTF(dst,sizeof(dst)/sizeof(dst[0]),__VA_ARGS__)

#define SWORDOFMOONLIGHT_READONLY 'r'  
#define SWORDOFMOONLIGHT_READWRITE 'w' 
#define SWORDOFMOONLIGHT_WRITECOPY 'v' /*copy-on-write (half-write)*/

#ifdef _DEBUG
#define SWORDOFMOONLIGHT_N(x) x /*for visual debugger*/
#else
#define SWORDOFMOONLIGHT_N(x) 1
#endif

#ifdef __cplusplus
#define SWORDOFMOONLIGHT_PREVENT_COPY(ctor) \
	private: _swordofmoonlight_##ctor(const _swordofmoonlight_##ctor&);\
	void operator=(const _swordofmoonlight_##ctor&);\
	public: _swordofmoonlight_##ctor() 
#ifndef _WIN32
#error Must defeat this==0 code elision below...
#endif
#define SWORDOFMOONLIGHT_RETURN_BY_REFERENCE2(ctor) \
	inline explicit operator bool()const{ return this?true:false; }
#define SWORDOFMOONLIGHT_RETURN_BY_REFERENCE(ctor) \
	SWORDOFMOONLIGHT_RETURN_BY_REFERENCE2(ctor) \
	SWORDOFMOONLIGHT_PREVENT_COPY(ctor);
/*RFC: NOTICE FUNKY REVERSE LOGIC (use list to avoid it I guess)*/
#define SWORDOFMOONLIGHT_ITEMIZE_LIST _item list[SWORDOFMOONLIGHT_N(32)];\
	inline const _item &operator[](signed int i)\
	const{ assert(i>=0); return list[i>=0?i:count+i]; }\
	inline _item &operator[](signed int i)\
	{ assert(i>=0); return list[i>=0?i:count+i]; } //TODO: remove count+i
#else
#define SWORDOFMOONLIGHT_PREVENT_COPY
#define SWORDOFMOONLIGHT_RETURN_BY_REFERENCE
#define SWORDOFMOONLIGHT_ITEMIZE_LIST _item list[SWORDOFMOONLIGHT_N(32)];
#endif

#ifdef _WIN32
#define SWORDOFMOONLIGHT_PACK
#define SWORDOFMOONLIGHT_USER
#elif defined(__GNUC__)
#define SWORDOFMOONLIGHT_PACK __attribute__((packed))
#define SWORDOFMOONLIGHT_USER __attribute__((__used__))
#else
#error Compiler not supported
#endif

typedef struct _swordofmoonlight_lib_image
{	
	int mode, mask; le32_t *set, *end; 
	
	struct{ int head:16, real:4; }; /*bits*/

	void *file; /*internal mmap file handle*/

	size_t size; /*added for unaligned files*/
		
#ifdef __cplusplus	
	
	static const size_t headmax = 0x7fff; /*32767*/

	/*NOTE: Where the head starts and ends or what is considered
	//the head (for the purposes of this library) is different for 
	//each file format. Don't assume a raw offset in a file begins
	//before or after the head. You should never need to use such 
	//offsets with this library. So it's a non-issue really*/	

	/*binary int operators:
	//these are templates only so not to be ambiguous with (bool)*/
	template<class T> inline T operator&(T i)const{ return mask&i; }

	/*CAUTION: these assume mode is nonzero and one of 'r', 'w', or 'v'*/
	inline bool readonly()const{ return mode==SWORDOFMOONLIGHT_READONLY; }
	inline bool writable()const{ return mode!=SWORDOFMOONLIGHT_READONLY; }
		
	template<class T> inline le32_t *operator+(T i){ return set+i; }

	template<class T> inline T *header(){ return (T*)(bad?0:(int8_t*)set-4*head+real); }

	template<class T> inline const le32_t *operator+(T i)const{ return set+i; }

	template<class T> inline const T *header()const{ return (T*)(bad?0:(int8_t*)set-4*head+real); }

	template<class T> inline static T &badref()
	{
		//MSVC2013??? "error C2101: '&' on constant" (with prf::name_t like T)
		//return *(T*)0; 
		T *C2101 = (T*)0; return *C2101;
	}

#ifdef SWORDOFMOONLIGHT_INTERNAL

#elif !defined(SWORDOFMOONLIGHT_INTERNAL)

	/*///////////////////////////////////////////////////////////////////////////////
	////TODO: SOMETHING CONCLUSIVE ABOUT INTEGER OVERFLOW////////////////////////////
	///////////////////////////////////////////////////////////////////////////////*/
	
	/*const versions are for user convenience. internally all images are non-const*/
	template<class T> inline bool operator<(T n)const{ return !bad?(set+n>=end):true; }
	template<class T> inline bool operator>(T n)const{ return !bad?!(set+n>=end):false; }
	template<class T> inline bool operator<(T *n)const{ return !bad?(n>=(T*)end):true; }
	template<class T> inline bool operator>(T *n)const{ return !bad?!(n>=(T*)end):false; }
	template<class T> inline bool operator<=(T n)const{ return !bad?(set+n>end):true; }
	template<class T> inline bool operator>=(T n)const{ return !bad?!(set+n>end):false; }
	template<class T> inline bool operator<=(T *n)const{ return !bad?(n>(T*)end):true; }
	template<class T> inline bool operator>=(T *n)const{ return !bad?!(n>(T*)end):false; }
	
	template<class T> /*const test (explicit cast to bool is for GCC: probably a bug)*/
	inline bool test(T cond, bool pass=true)const{ return !bad?bool(cond)==pass:false; }
	
#define const /*ILLEGAL C++ removing const keyword from play below*/
#endif

	/*fail if some address or offset is at or past the end of the file image*/
	template<class T> inline bool operator<(T n)const{ return !bad?(bad=set+n>=end):true; }
	template<class T> inline bool operator>(T n)const{ return !bad?!(bad=set+n>=end):false; }
	template<class T> inline bool operator<(T *n)const{ return !bad?(bad=n>=(T*)end):true; }
	template<class T> inline bool operator>(T *n)const{ return !bad?!(bad=n>=(T*)end):false; }

	/*fail if some address or offset is past (but not at) the end of the file image*/
	template<class T> inline bool operator<=(T n)const{ return !bad?(bad=set+n>end):true; }
	template<class T> inline bool operator>=(T n)const{ return !bad?!(bad=set+n>end):false; }
	template<class T> inline bool operator<=(T *n)const{ return !bad?(bad=n>(T*)end):true; }
	template<class T> inline bool operator>=(T *n)const{ return !bad?!(bad=n>(T*)end):false; }
	
	template<class T> /*general use test (explicit cast to bool is for GCC: probably a bug)*/
	inline bool test(T cond, bool pass=true)const{ return !bad?!(bad=bool(cond)!=pass?1:0):false; }
	
#undef const /*ILLEGAL C++ quietly put the const keyword back*/
	
	/*this test succeeds regardless of the bad flag state and does not affect bad*/
	template<class T> inline bool claim(T *p)const{ return p>(T*)set&&p<(T*)end;	}
	template<class T> inline bool claim(T &p)const{ return p>(T*)set&&&p<(T*)end; }

	inline bool operator!()const{ return !this||bad?true:false; }
	inline operator bool()const{ return !this||bad?false:true; }

	mutable /*badness..*/

#endif /*__cplusplus*/

	/*bad: if bad the image is ignored by the library. 
	//bad can be zeroed if you want (but that is obviously not recommended)*/
		
	int bad; /*non-zero if an image was flagged as corrupt*/

}swordofmoonlight_lib_image_t;

typedef const cint8_t* swordofmoonlight_lib_reference_t; /*0 terminated string*/
typedef const cint8_t swordofmoonlight_lib_references_t[SWORDOFMOONLIGHT_N(256)];

#include "pack.inl" /*push*/

typedef struct _swordofmoonlight_mhm_header
{
	SWORDOFMOONLIGHT_RETURN_BY_REFERENCE2(mhm_header)

	ule32_t vertcount; 
	ule32_t normcount; 
	ule32_t facecount; /*sum of the following counts*/
	ule32_t	sidecount; /*1*/
	ule32_t flatcount; /*2*/
	ule32_t cantcount; /*4*/	
}SWORDOFMOONLIGHT_PACK
swordofmoonlight_mhm_header_t;

typedef lefp_t swordofmoonlight_mhm_vector_t[3];	

typedef ule32_t swordofmoonlight_mhm_index_t;	

typedef struct _swordofmoonlight_mhm_face
{
	ule32_t clipmode; /*historically this is 1 2 or 4*/

	swordofmoonlight_mhm_vector_t box[2]; /*{min,max}*/
	
	ule32_t normal;
	ule32_t ndexcount; /*each face is read in sequence*/
}SWORDOFMOONLIGHT_PACK
swordofmoonlight_mhm_face_t;

typedef struct _swordofmoonlight_tnl_vertex
{
	lefp_t pos[3], lit[3], uvs[2];

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_tnl_vertex_t;	

typedef ule16_t swordofmoonlight_tnl_triple_t[3];

typedef struct _swordofmoonlight_msm_textures
{
	SWORDOFMOONLIGHT_RETURN_BY_REFERENCE(msm_textures)	

	ule16_t count;

	swordofmoonlight_lib_references_t refs;

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_msm_textures_t;

typedef struct _swordofmoonlight_msm_vertices
{
	SWORDOFMOONLIGHT_RETURN_BY_REFERENCE(msm_vertices)
	
	ule16_t count; typedef swordofmoonlight_tnl_vertex_t _item;

	SWORDOFMOONLIGHT_ITEMIZE_LIST /*_item list[];*/

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_msm_vertices_t;	

typedef struct _swordofmoonlight_msm_polygon
{
	/*2202: note, what I think is happening here is
	the first one of these is really the total count
	of top-level triangles, and the rest are actually
	fields that come after the indices... or at least
	conceptually*/
	ule16_t subdivs; 
	ule16_t texture;
	/*SOM_MAP 4421cc accesses this as "char"
	but promotes (sign extended) to unsigned*/
	ule16_t corners; 
	ule16_t indices[SWORDOFMOONLIGHT_N(3)];

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_msm_polygon_t;

typedef struct _swordofmoonlight_mdo_textures
{
	SWORDOFMOONLIGHT_RETURN_BY_REFERENCE(mdo_textures)

	ule32_t count;

	swordofmoonlight_lib_references_t refs;

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_mdo_textures_t,
swordofmoonlight_mpx_textures_t;

typedef struct _swordofmoonlight_mdo_materials
{
	SWORDOFMOONLIGHT_RETURN_BY_REFERENCE(mdo_materials)
	
	ule32_t count;

	union _item
	{		
		struct{	lefp_t r,g,b,a, r2,g2,b2; };
		struct{	lefp_t rgba[4], rgb2[3],_; };
	};

	SWORDOFMOONLIGHT_ITEMIZE_LIST /*_item list[];*/

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_mdo_materials_t;	

typedef struct _swordofmoonlight_mdo_controlpoint
{
	lefp_t pos[3];

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_mdo_controlpoints_t[4];

typedef struct _swordofmoonlight_mdo_extra /*EXTENSION*/
{		
	struct uv_extra
	{
		ule16_t size,procedure;
		
		lefp_t tu,tv;
	};

	/*this is used to combine a MDO
	//and MDL file*/
	uint8_t part, _pad;
	ule16_t part_verts;
	ule32_t part_index; 
	/*VERSION 2 (8B)*/
	uv_extra uv_fx;

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_mdo_extra_t;

typedef struct _swordofmoonlight_mdo_channel
{		
	#ifdef __cplusplus
	swordofmoonlight_mdo_extra_t &extra()
	{
		assert(extrasize);
		return *(swordofmoonlight_mdo_extra_t*)
		((ule32_t*)this+extradata);
	}
	#endif

	uint8_t blendmode; /*1 is dst, 0 is (1-alpha)*dst*/
	/*EXTENSION 
	 
		dword size of extension data descriptor
		probably this is the version as well and
		is likely to be the same for all chunks
	 */
	uint8_t extrasize;
	/*EXTENSION 
		
		dword offset to extension data starting
		from top of this object. there should be
		room for about 13107 chunks in that case
	*/
	ule16_t extradata;
	ule16_t texnumber; /*texture id*/
	ule16_t matnumber; /*material id*/
	ule16_t ndexcount; /*3 per triangle (index count)*/
	ule16_t vertcount; 		
	ule32_t ndexblock; /*byte offset to indice block*/
	ule32_t vertblock; /*byte offset to vertex block*/
}SWORDOFMOONLIGHT_PACK
swordofmoonlight_mdo_channel_t;
typedef struct _swordofmoonlight_mdo_channels
{			
	SWORDOFMOONLIGHT_RETURN_BY_REFERENCE(mdo_channels)

	ule32_t count;

	//NEW: moved struct definition outside/above
	typedef _swordofmoonlight_mdo_channel _item; 

	SWORDOFMOONLIGHT_ITEMIZE_LIST /*_item list[];*/

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_mdo_channels_t;

typedef union _swordofmoonlight_psx_uword_t
{
	struct{ ule16_t lo, hi; };
	struct{ uint8_t u, v, s, t; };		   
	struct{ uint8_t r, g, b, c; };

#ifdef __cplusplus

	inline operator ule32_t&(){ return *(ule32_t*)this; }
	inline operator uint32_t()const{ return *(ule32_t*)this; }

#endif

}SWORDOFMOONLIGHT_PACK 
swordofmoonlight_psx_uword_t;

typedef union _swordofmoonlight_psx_sword_t
{
	struct{ le16_t lo, hi; };

#ifdef __cplusplus

	inline operator le32_t&(){ return *(le32_t*)this;	}	
	inline operator int32_t()const{ return *(le32_t*)this;	}

#endif

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_psx_sword_t;

typedef struct _swordofmoonlight_mdl_header /*16B*/
{
	SWORDOFMOONLIGHT_RETURN_BY_REFERENCE2(mdl_header)

	/*NOTE
	//historically these "flags" are 1 or 4 depending on
	//the kind of animation. som_db.exe uses &3 instead of
	//==1 but if bit 2 is set the geometry is all jumbled up.
	//
	//I think bit 4 is being set for "modern" hard animation
	//format that is backward compatible but indicates how the
	//its skeleton is represented
	*/
	uint8_t animflags; /*allocates memory if nonzero*/
	uint8_t hardanims; /*Joint MIMe animation (PSOne)*/
	uint8_t softanims; /*Vertex/normal MIMe animation*/
	uint8_t timblocks; /*Textures (PSOne TIM image format)*/
	uint8_t primchans; /*one or more required (meshes)*/

		//RENAME ME
		//THIS IS REALLY LOD OR SOMETHING???
		/*2021: this seems to be a multi-texture system 
		or at least primchans is looped over "uvsblocks"
		times. multi-resolution?*/
	uint8_t uvsblocks; /*EneEdit (403A66) loops over these*/

	/*2018:
	//making primchanwords 16-bit because 32-bit is not an aligned read
	*/
	ule16_t primchanwords; /*dword anims block offset*/
	ule16_t _primchanwords2; /*2021: uvsblocks>=2*/
	ule16_t _primchanwords3; /*2021: uvsblocks==3*/
	ule16_t hardanimwords; /*dword sizeof anims block*/
	ule16_t softanimwords; 

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_mdl_header_t;

typedef struct _swordofmoonlight_mdl_primch
{
	SWORDOFMOONLIGHT_RETURN_BY_REFERENCE2(mdl_primch)

	ule32_t vertsbase; /*dword offset to vertex block*/
	ule32_t vertcount; 
	ule32_t normsbase; /*dword offset to normal block*/
	ule32_t normcount; 		
	ule32_t primsbase; /*dword offset to face block*/
	ule32_t primcount; 
	ule32_t _unknown0; /*32-bits wouldn't be padding*/

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_mdl_primch_t;

typedef struct _swordofmoonlight_mdl_vertex
{
	union{ struct{ le16_t x, y, z, _; }; le16_t xyz[3]; };

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_mdl_vertex_t,
swordofmoonlight_mdl_normal_t;

#define SWORDOFMOONLIGHT_PRIMITIVE_LIST(n) \
	SWORDOFMOONLIGHT_RETURN_BY_REFERENCE(mdl_prims##n)\
	SWORDOFMOONLIGHT_ITEMIZE_LIST\
	enum{ N=0x##n };
typedef struct _swordofmoonlight_mdl_uvparts
{	SWORDOFMOONLIGHT_RETURN_BY_REFERENCE(mdl_uvparts)
	/*EneEdit (403AA6) takes this as a DWORD*/
	ule32_t count;
	struct _item /*3 words*/
	{
	uint8_t u0,v0;
	ule16_t ___; /*pad? 0092.MDL (SFX) has 0x7ec0*/
	uint8_t u1,v1;
	ule16_t tsb; 
	uint8_t u2,v2;
	uint8_t u3,v3;
	}; /*_item list[];*/
	SWORDOFMOONLIGHT_ITEMIZE_LIST
}SWORDOFMOONLIGHT_PACK
swordofmoonlight_mdl_uvparts_t;
typedef struct _swordofmoonlight_mdl_prims00
{	ule16_t x00_flatlit_RGB_triangle;
	ule16_t count;
	struct _item /*3 words*/
	{
	uint8_t r,g,b,c; /*cp*/
	ule16_t lit; /*normal*/
	ule16_t pos0;
	ule16_t pos1;
	ule16_t pos2; 
	}; /*_item list[];*/
	SWORDOFMOONLIGHT_PRIMITIVE_LIST(00)
}SWORDOFMOONLIGHT_PACK
swordofmoonlight_mdl_prims00_t;
typedef struct _swordofmoonlight_mdl_prims01
{	ule16_t x01_flatlit_UVs_triangle;
	ule16_t count;	
	struct _item /*5 words*/
	{
	uint8_t u0,v0;
	ule16_t ___; /*pad?*/
	uint8_t u1,v1;	
	ule16_t tsb; 
	uint8_t u2,v2;
	ule16_t modeflags; 
	ule16_t lit; /*normal*/
	ule16_t pos0;
	ule16_t pos1;
	ule16_t pos2;
	}; /*_item list[];*/
	SWORDOFMOONLIGHT_PRIMITIVE_LIST(01)
}SWORDOFMOONLIGHT_PACK
swordofmoonlight_mdl_prims01_t;
typedef struct _swordofmoonlight_mdl_prims03
{	ule16_t x03_softlit_RGB_triangle;
	ule16_t count;
	struct _item /*4 words*/
	{
	uint8_t r,g,b,c; /*cp*/
	ule16_t lit0,pos0;
	ule16_t lit1,pos1;
	ule16_t lit2,pos2; 
	}; /*_item list[];*/
	SWORDOFMOONLIGHT_PRIMITIVE_LIST(03)
}SWORDOFMOONLIGHT_PACK
swordofmoonlight_mdl_prims03_t;
typedef struct _swordofmoonlight_mdl_prims04
{	ule16_t x04_softlit_UVs_triangle;
	ule16_t count;
	struct _item /*6 words*/
	{
	uint8_t u0,v0;
	ule16_t ___; /*pad? 0001.MDL (SFX) has 0x7da0*/
	uint8_t u1,v1;	
	ule16_t tsb; 
	uint8_t u2,v2;
	ule16_t modeflags;
	ule16_t lit0,pos0;
	ule16_t lit1,pos1;
	ule16_t lit2,pos2; 
	}; /*_item list[];*/
	SWORDOFMOONLIGHT_PRIMITIVE_LIST(04)
}SWORDOFMOONLIGHT_PACK
swordofmoonlight_mdl_prims04_t;
typedef struct _swordofmoonlight_mdl_prims06
{	ule16_t x06_flatlit_RGB_quadrangle;
	ule16_t count;
	struct _item /*4 words*/
	{
	uint8_t r,g,b,c; /*cp?*/
	ule16_t lit; /*normal*/
	ule16_t pos0,pos1;
	ule16_t pos2,pos3; 	
	ule16_t pad_; 
	}; /*_item list[];*/
	SWORDOFMOONLIGHT_PRIMITIVE_LIST(06)
}SWORDOFMOONLIGHT_PACK
swordofmoonlight_mdl_prims06_t;
typedef struct _swordofmoonlight_mdl_prims07
{	ule16_t x07_flatlit_UVs_quadrangle;
	ule16_t count;
	struct _item /*4 words*/
	{
	ule16_t uvs; /*uvpart*/
	ule16_t modeflags; 
	ule16_t lit; /*normal*/
	ule16_t pos0,pos1;
	ule16_t pos2,pos3; 
	ule16_t pad_;
	}; /*_item list[];*/
	SWORDOFMOONLIGHT_PRIMITIVE_LIST(07)
}SWORDOFMOONLIGHT_PACK
swordofmoonlight_mdl_prims07_t;
	/*
	NOTE: (08) is gutted 442d50 in som_db.exe
	NOTE: (09) exists at 442d70 in som_db.exe
	NOTE: (0B) is gutted 443200 in som_db.exe
	*/
typedef struct _swordofmoonlight_mdl_prims0A
{	ule16_t x0a_softlit_UVs_quadrangle;
	ule16_t count;
	struct _item /*5 words*/
	{
	ule16_t uvs; /*uvpart*/
	ule16_t modeflags;
	ule16_t lit0,pos0;
	ule16_t lit1,pos1;
	ule16_t lit2,pos2; 
	ule16_t lit3,pos3; 
	}; /*_item list[];*/
	SWORDOFMOONLIGHT_PRIMITIVE_LIST(0A)
}SWORDOFMOONLIGHT_PACK
swordofmoonlight_mdl_prims0A_t;
typedef struct _swordofmoonlight_mdl_prims0D
{	ule16_t x0d_unlit_UVs_triangle;
	ule16_t count;
	struct _item /*6 words*/
	{
	uint8_t u0,v0;
	ule16_t ___; /*pad?*/
	uint8_t u1,v1;	
	ule16_t tsb; 
	uint8_t u2,v2;	
	/*RGB is decoded but 4435a0 ignores them*/
	uint8_t r,g,b,c; /*meaningful to SOM?*/ 
	ule16_t modeflags; 
	ule16_t pos0;
	ule16_t pos1;
	ule16_t pos2; 
	ule16_t pad_; 
	}; /*_item list[];*/
	SWORDOFMOONLIGHT_PRIMITIVE_LIST(0D)
}SWORDOFMOONLIGHT_PACK
swordofmoonlight_mdl_prims0D_t;
	/*
	NOTE: (0F) exists at 4436e0 in som_db.exe
	NOTE: (13) exists at 443d10 in som_db.exe
	*/
typedef struct _swordofmoonlight_mdl_prims11
{	ule16_t x11_unlit_UVs_quadrangle;
	ule16_t count;
	struct _item /*4 words*/
	{
	ule16_t uvs; /*uvpart*/	
	/*RGB is decoded but 4435a0 ignores them*/
	uint8_t r,g,b,c; /*meaningful to SOM?*/
	ule16_t modeflags;
	ule16_t pos0,pos1; 
	ule16_t pos2,pos3; 
	}; /*_item list[];*/
	SWORDOFMOONLIGHT_PRIMITIVE_LIST(11)
}SWORDOFMOONLIGHT_PACK
swordofmoonlight_mdl_prims11_t;

typedef union _swordofmoonlight_softanimframe
{
	struct{ int8_t lo; uint8_t time; }; ule16_t bits;

#ifdef __cplusplus

	inline operator bool()const{ return bits?true:false; }

#endif

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_softanimframe_t;

typedef union _swordofmoonlight_softanimdatum
{
#ifndef __cplusplus

	/*[0] IS NONSTANDARD. THE union MAY BE THE WRONG SIZE!*/	
	int8_t small,size:1; le16_t large[0];

#else

	/*2018: can't be 0 with C++ and Cxx?
	//2018: can't be 1 either, because that's two bytes*/
	int8_t _small,_size:1; /*//le16_t large[!1];*/
	
	/*inline signed magnitude()const
	{
		return size?small:*large; 
	}*/
	inline int16_t magnitude()const
	{
		return _size?_small:*(le16_t*)this; 
	}	
	inline unsigned footprint()const
	{
		return _size?1:2; 
	} 

#endif
}SWORDOFMOONLIGHT_PACK
swordofmoonlight_softanimdatum_t;

typedef union _swordofmoonlight_softanimcodec
{
	/*diffmask is a bit vector per each xyz component
	//of every vertex in the file. So in theory it should
	//be the same size file wide and less than or equal to
	//diffdataoffset below ... which you would also expect
	//to be the same file wide (per animation channel)
	//
	//UPDATE: the bit vector is actually all x components
	//first, and then all y, and then z. See the implementation
	//of copyvertebuffer in Swordofmoonlight.cpp for details*/

	/*these start at diffdata+diffdataoffset*/
	swordofmoonlight_softanimdatum_t diffdata[1];
	
	struct
	{			
		ule16_t diffdataoffset; /*in bytes*/

		uint8_t primchan; /*unimplemented*/
		uint8_t diffmask[SWORDOFMOONLIGHT_N(16)];
	};

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_softanimcodec_t;

typedef struct _swordofmoonlight_mdl_animation
{
	SWORDOFMOONLIGHT_RETURN_BY_REFERENCE(mdl_animation)

	ule16_t id; /*2021: maybe 32-bit for soft animations (44438c) */
	
	union
	{
		struct /*hard variety (anonymous struct is NON-PORTABLE)*/
		{
			ule16_t htime; //UNFRIENDLY!! (doesn't apply to soft-anims) 

			swordofmoonlight_psx_uword_t hwords[SWORDOFMOONLIGHT_N(32)];
		};

		/*2021: is the first 2 bytes always 0? so id is 32-bit??? */
		swordofmoonlight_softanimframe_t frames[SWORDOFMOONLIGHT_N(32)];
	};

}SWORDOFMOONLIGHT_PACK
*swordofmoonlight_mdl_animation_t;
typedef const _swordofmoonlight_mdl_animation* 
swordofmoonlight_mdl_const_animation_t;

#include "pack.inl" /*pop*/

typedef struct _swordofmoonlight_msm_polyinfo
{
#define SWORDOFMOONLIGHT_POLYZERO 0,0,0,0,

	int polygons, corners, triangles, partials;

#ifdef __cplusplus

	inline void operator+=(int n)
	{
		if(n>2) 
		{				
			corners+=n; triangles+=n-2; polygons++; 
		}		
		else partials++;
	}

#endif

}swordofmoonlight_msm_polyinfo_t;

typedef struct _swordofmoonlight_msm_polystack
{
#ifndef SWORDOFMOONLIGHT_POLYSTACK
#define SWORDOFMOONLIGHT_POLYSTACK 2
#endif
	/*max: 0 for no tessellation, -1UL for full*/
	unsigned int max:SWORDOFMOONLIGHT_POLYSTACK; 
	unsigned int top:SWORDOFMOONLIGHT_POLYSTACK; 
		
	int n[1<<SWORDOFMOONLIGHT_POLYSTACK];

	/*these can now be used in a streaming setup*/
	const swordofmoonlight_msm_polygon_t *restart;
	bool tessellate;

#ifdef __cplusplus

	/*TODO: COULD USE RETHINKING/MORE ANALYSIS*/
	/*traverse polygon subdivision: true if p should be tessellated*/
	bool operator<<(const swordofmoonlight_msm_polygon_t *p)
	{	
		if(top==0&&n[0]) return !"overflow";

		if(restart!=p&&(restart=p)) /*allow restart*/
		{	
		uint16_t next_polygons_subdivs = p->indices[p->corners];

		if(p->subdivs>1) /*ascending subD tree*/
		{				
			n[++top] = p->subdivs; /*can result in overflow*/			

			if(!next_polygons_subdivs) n[top]--; /*leaf-closure*/

			tessellate = next_polygons_subdivs?top-1==max:top-1<=max;
		}
		else if(!next_polygons_subdivs) /*descending subD tree*/
		{
			tessellate = top-1<=max;

			if(top==0||n[top]<=0) /*<=: should never be less than*/
			{
				n[top=0] = 1; /*signal "underflow" overflow*/
			}
			else if(--n[top]==0) /*gather up leaves until..*/
			{
				while(--top&&--n[top]==0); /*closure*/
			}
		}
		else tessellate = top-1==max; 

		}return tessellate; /*assuming nonzero p*/
	}

	inline operator bool() /*return final status and reset*/
	{ 	
		bool ok = top==0&&n[0]==0; top = n[0] = 0; restart = 0; return ok;
	}

#endif

}swordofmoonlight_msm_polystack_t;

typedef struct _swordofmoonlight_mdl_priminfo
{
#define SWORDOFMOONLIGHT_PRIMZERO 0,0,0,0,

	/*Note: total primitives is singles+doubles*/

	int triangles, singles, doubles, troubles; /*,primitives; //mask*/

#ifdef __cplusplus

	inline void operator+=(int n) /*could use primcode instead*/
	{
		switch(n)
		{
		case 3: triangles+=1; singles++; break;
		case 4: triangles+=2; doubles++; break;

		default: troubles++; break;
		}		
	}

#endif

}swordofmoonlight_mdl_priminfo_t;

typedef struct _swordofmoonlight_tmd_fields
{
	struct{ uint16_t tsb, modeflags; }; uint32_t bits;

	/*PlayStation TMD encoding: could use more work/documentation*/
	struct{ uint32_t tpn:5,abr:2,tpf:2,:1,_ef:6, mode:8,tge:1,abe:1,tme:1,:5; }; /*lgt*/

#ifdef __cplusplus

	/*EXPERIMENTAL
	This is 6 flags for each of 3 UV pairs. If set the UV is extended to 
	edge of the map that MDL cannot otherwise support... in that case it
	is 1.0 or add 1 to the UV's value. For codes that have 4 UVs the 4th
	can't use this, but a quad should have one UV pair that doesn't need
	to, so it can be rotated until that it's last.
	*/
	inline int mdl_edge_flags_ext(){ return _ef; }

#endif

}swordofmoonlight_tmd_fields_t;

typedef struct _swordofmoonlight_mdl_triangle 
{	 
/*WARNING: if edges is 0 the rest is junk*/

	short edges; /*3 for tris 2 for quads*/
				
#ifdef __cplusplus

	inline operator short(){ return edges; }

#endif

	short set; /*Sword of Moonlight primcode*/

	/*primcodes 00, 03, & 06* use rgb 
	//primcodes 0D and 11 store rgb values in lit*
	//*Probably these codes are unusable in a game??
	//06 is a CP-like quad. Unsure where it's from*/

	unsigned short pos[3], lit[3];
		
	union{ unsigned char uvs[3][2], rgb[3];	};

	/*
	NOTE: mdl_edge_flags_ext EXTENDS uvs TO THE EDGE
	OF THE UV MAP, UP TO 256
	*/
	swordofmoonlight_tmd_fields_t tmd; 	
	
}swordofmoonlight_mdl_triangle_t;

struct _swordofmoonlight_mdl_v;
typedef _swordofmoonlight_mdl_v swordofmoonlight_mdl_v_t;

typedef struct _swordofmoonlight_mdl_v/*buffer*/
{
	#include "pack.inl" /*push*/

	struct /*__cplusplus*/
	{
		int16_t pos[3], lit[3], uvs[2];	

	}SWORDOFMOONLIGHT_PACK;

	#include "pack.inl" /*pop*/

#ifdef __cplusplus
		
	/*this is used to build the vertex buffer out of unique vertices*/
	inline bool operator!=(const _swordofmoonlight_mdl_v/*buffer*/ &cmp)
	{
		return ((int64_t*)this)[0]!=((int64_t*)&cmp)[0]||((int64_t*)this)[1]!=((int64_t*)&cmp)[1];
	}

	/*ideal for considering two soft animation vertices for tweening*/
	inline const int64_t &operator~()const{ return *((const int64_t*)this); }

	inline int64_t &operator~(){ return *((int64_t*)this); }

#endif
 
	int32_t hash; /*int2*/

}*swordofmoonlight_mdl_vbuffer_t;
typedef const swordofmoonlight_mdl_v_t
*swordofmoonlight_mdl_const_vbuffer_t;

typedef struct _swordofmoonlight_mdl_hardanim
{	
	int map; /*parent in hierarchy*/

	struct /*__cplusplus*/
	{
		int16_t cv[3]; /*current rotation*/
		int16_t ct[3]; /*current position*/
		uint8_t cs[3],x80; /*2020: scale*/
	};

	int dvt; /*difference mask (now with scale)*/

#ifdef __cplusplus

	inline bool operator!() /*returns true if identity*/
	{
		if(*(int64_t*)cv==0&&*(int32_t*)(ct+1)==0)
		return *((uint32_t*)cs)==0x80808080;
		return false;
	}

	inline void set()
	{
		memset(cv,0x00,12); unscale();
	}
	inline void unscale()
	{
		*((uint32_t*)cs) = 0x80808080; /*big-endian safe*/
	}
	template<class T> inline void set(const T (&p)[3+3])
	{
		cv[0] = p[0]; cv[1] = p[1]; cv[2] = p[2];
		ct[0] = p[3]; ct[1] = p[4]; ct[2] = p[5]; unscale();
	}
	inline void set(const _swordofmoonlight_mdl_hardanim &cp)
	{
		memcpy(cv,cp.cv,16);
	}
	template<class T> inline void add(const T (&p)[3+3])
	{
		cv[0]+=p[0]; cv[1]+=p[1]; cv[2]+=p[2];
		ct[0]+=p[3]; ct[1]+=p[4]; ct[2]+=p[5];
	}
	template<class T> inline void sub(const T (&p)[3+3])
	{
		cv[0]-=p[0]; cv[1]-=p[1]; cv[2]-=p[2];
		ct[0]-=p[3]; ct[1]-=p[4]; ct[2]-=p[5];
	}
	template<class T> inline void scale(const T (&p)[3])
	{
		for(int i=3;i-->0;) cs[i] = (uint8_t)p[i]; 
	}
	template<class T> inline void get(T (&p)[3+3])const
	{
		p[0] = cv[0]; p[1] = cv[1]; p[2] = cv[2];
		p[3] = ct[0]; p[4] = ct[1]; p[5] = ct[2];
	}	

#endif

	static const bool SOFT = 0;

}swordofmoonlight_mdl_hardanim_t;

typedef struct _swordofmoonlight_mdl_softanim
{
	static const bool SOFT = 1;

	uint32_t dec, vec; /*int2; _internal; subject to change_*/
	const swordofmoonlight_softanimcodec_t *firststep,*overstep;

#ifdef __cplusplus

	inline operator bool()const{ return firststep?true:false; }

#endif
	
	/*for your convenience...

	//delta: difference data multiplier
	//Typically the full delta of an animation frame.
	//
	//UPDATE: delta is not used by copyvertexbuffer()
	//It may yet be used by translate/other interfaces.
	//
	//changed: gets turned on if/when a change occurs.
	//You must turn it off manually to detect anything.
	//
	//nowrite: decoding happens but nothing is written.
	//You can use this along with vwindow/vrestart to
	//reposition the decoder.
	//
	//nodup: do not update fillvertexbuffer duplicates.
	//
	//vrestart: you can manually set this to reposition
	//the decoding. Basically if dec/3 does not equal
	//vrestart the decoder will seek to vrestart.
	//
	//vwindow: set it to the vertex you want decoding
	//to stop at. Normally vrestart will be equal to 
	//vwindow after decoding. Set to 0 to ignore*/

	int32_t vrestart, vwindow; signed delta; /*int2*/
	
	union
	{			
		struct{ unsigned changed:1, nowrite:1, nodup:1; }; 
		
		int status;
	};

}swordofmoonlight_mdl_softanim_t;

#include "pack.inl" /*push*/

typedef uint8_t swordofmoonlight_tim_index_t;

typedef union _swordofmoonlight_tim_codex
{
	#ifndef SWORDOFMOONLIGHT_BIGEND
	struct{ uint8_t lo:4,hi:4; };
	#else
	struct{ uint8_t hi:4,lo:4; };
	#endif
	uint8_t bits;

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_tim_codex_t;	

typedef union _swordofmoonlight_tim_highcolor
{
	#ifndef SWORDOFMOONLIGHT_BIGEND
	/*MDL files don't use _ however the
	PlayStation games set bits to 0 for
	transparent and set _ to 1 if black*/
	struct{ uint16_t r:5,g:5,b:5,_:1; };
	#else
	struct{ uint16_t _:1,b:5,g:5,r:5; };
	#endif
	uint16_t bits;	
	
#ifdef __cplusplus

	inline _swordofmoonlight_tim_highcolor operator!()const
	{
		/*_swordofmoonlight_tim_highcolor out = {{r,g,b,_}}; //GCC*/
		_swordofmoonlight_tim_highcolor out = *this; 		
		out.r = b; out.b = r; return out;
	}

#endif

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_tim_highcolor_t;	

typedef struct _swordofmoonlight_tim_truecolor
{
	/*PlayStation only (SOM runtimes do not do 8bpp)*/

	uint8_t r, g, b; 

#ifdef __cplusplus

	operator int()const{ return (int)b<<16|(int)g<<8|r; } 
	
	inline _swordofmoonlight_tim_truecolor operator!()const
	{
		_swordofmoonlight_tim_truecolor out = {b,g,r}; return out;
	}
	inline bool operator=(_swordofmoonlight_tim_highcolor x)
	{
		/*2021: SOM_MAP uses the following formula, which is
		favoring black over white
		r = x.r<<3; g = x.g>>2&0xf8; b = x.b>>7&0xf8;*/

		/*THIS FAVORS WHITE OVER BLACK. PROBABLY TOO LATE TO 
		//CHANGE IT, BUT FAVORING BLACK SEEMS TO WORK BETTER
		//(subtract 7 to do that; consider the consequences)*/
		r = (int)x.r*8+7; g = (int)x.g*8+7; b = (int)x.b*8+7;
		return x.bits!=0;
	}
	inline void operator=(uint8_t greyscale)
	{
		r = g = b = greyscale;
	}

#endif

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_tim_truecolor_t;	

typedef struct _swordofmoonlight_tim_header
{
	SWORDOFMOONLIGHT_RETURN_BY_REFERENCE2(tim_header)

	/*NOTE: despite	the names of this structure
	//it works as a PlayStation TIM header also*/

	uint8_t sixteen, version; /*0x10, 0x00*/
	le16_t reservedbySony00;
	#ifndef SWORDOFMOONLIGHT_BIGEND
	uint8_t	pmode:3, cf:1,:4; /*Sony's names*/
	#else
	uint8_t	:4,cf:1, pmode:3;
	#endif
	uint8_t reservedbySony01;
	le16_t reservedbySony02;

#ifdef __cplusplus

	inline bool operator!()const /*validate header*/
	{ 	
		//pmode==4: disregarding "mixed" mode for now
		return !this||sixteen!=16||version||pmode==4; 
	}

#endif

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_tim_header_t;	

typedef union _swordofmoonlight_tim_pixrow
{
	/*it's too bad [0] arrays were not thought thru*/

	swordofmoonlight_tim_codex_t codices[1]; /*4bit*/
	swordofmoonlight_tim_index_t indices[1]; /*8bit*/

	swordofmoonlight_tim_truecolor_t truecolor[1]; /*48bit*/
	swordofmoonlight_tim_highcolor_t highcolor[1]; /*16bit*/

#ifdef __cplusplus

	inline operator bool(){ return this?true:false; }

#endif

}swordofmoonlight_tim_pixrow_t;

typedef struct _swordofmoonlight_tim_pixmap
{
	SWORDOFMOONLIGHT_RETURN_BY_REFERENCE2(tim_pixmap)

	/*Sony's names*/
	ule32_t bnum; /*sizeof block in bytes*/
	ule16_t dx;   /*x position in framebuffer*/
	ule16_t dy;   /*y position in framebuffer*/
	ule16_t w;    /*width in 16bit units*/
	ule16_t h;    /*height*/
	
	/*Data/CLUT entries
	//Depends on the following settings for pmode
	//0x00: 4bits per texel lookup into CLUT (up to 16)
	//0x01: 8bits per texel lookup into CLUT (up to 256)
	//0x02: 5bits per rgb plus highest order transparency mode bit
	//0x03: 2/3rds of a 24-bit rgb triplet (3 entries per two texels)
	//0x04: mixed mode (interpretation requires external info)
	//IN THEORY: it's very possible that what 0x04 really means is
	//that there is only a CLUT or only an image (per the cf flag)
	//FYI: Sword of Moonlight doesn't implement 3 & 4 is moot*/

#ifdef __cplusplus

	typedef _swordofmoonlight_tim_pixrow _pixels;

	/*make sure your pixmap_t is legit before going here!*/
	inline _pixels &operator[](int i)
	{
		return *(_pixels*)((le16_t*)(this+1)+i*w);
	}
	inline const _pixels &operator[](int i)const
	{
		return *(_pixels*)((le16_t*)(this+1)+i*w);
	}

	/*calculated width of the row (the true width in pixels)
	//Note: swordofmoonlight_tim_codex_t are two pixels apiece*/
	inline int operator()(int pmode)const
	{ 
		switch(this?pmode:-1)
		{
		case 0: case 1: case 2: return w*(pmode?3-pmode:4);

		case 3: return w/3*2; //24bpp mode

		default: return 0;
		}
	}

	inline bool operator!()const /*validate pixmap*/
	{
		return !this||!h||!w||bnum<w*2*h+sizeof(_swordofmoonlight_tim_pixmap);
	}

#endif

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_tim_pixmap_t;	

typedef uint8_t swordofmoonlight_txr_index_t;

typedef union _swordofmoonlight_txr_truecolor
{
	struct{ uint8_t b, g, r; }; uint8_t bgr[3];

#ifdef __cplusplus

	operator int()const{ return (int)r<<16|(int)g<<8|b; } 

	inline _swordofmoonlight_txr_truecolor operator!()const
	{
		/*_swordofmoonlight_txr_truecolor out = {{r,g,b}}; //GCC*/
		_swordofmoonlight_txr_truecolor out; out.g = g; 

		out.b = r; out.r = b; return out;
	}

#endif

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_txr_truecolor_t;	

typedef union _swordofmoonlight_txr_palette
{
	struct{	uint8_t b, g, r, _; }; uint8_t bgr[4];

	_swordofmoonlight_txr_truecolor truecolor;

#ifdef __cplusplus

	operator int()const{ return (int)r<<16|(int)g<<8|b; } 

	inline _swordofmoonlight_txr_palette operator!()const
	{
		_swordofmoonlight_txr_palette out = {r,g,b}; return out;
	}

#endif

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_txr_palette_t;	

typedef struct _swordofmoonlight_txr_header
{
	SWORDOFMOONLIGHT_RETURN_BY_REFERENCE2(txr_header)

	/*32bit-ness: assuming SOM reads these as 32bit values*/

	/*WARNING: width may be height / vice versa (need examples)*/
	
	le32_t width, height, depth, mipmaps; /*depth: 8 or 24*/

#ifdef __cplusplus

	/*Note: mind ! operator and RETURN_BY_REFERENCE (bool) operator*/

	inline bool operator!()const /*semi-validate header (could do more)*/
	{ 	
		return this&&width>0&&height>0&&(depth==8||depth==24)&&mipmaps>0?false:true;
	}

#endif

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_txr_header_t;	

#include "pack.inl" /*pop*/

typedef union _swordofmoonlight_txr_pixrow
{
	/*it's too bad [0] arrays were not thought thru*/

	swordofmoonlight_txr_truecolor_t truecolor[1];
	swordofmoonlight_txr_index_t indices[1];

#ifdef __cplusplus

	inline operator bool()const{ return this?true:false; }

#endif

}swordofmoonlight_txr_pixrow_t;

typedef struct _swordofmoonlight_txr_mipmap
{	
	/*depth can be one of 8 or 24*/
	int height, width, depth, rowlength; 

	swordofmoonlight_txr_pixrow_t *firstrow;
	const swordofmoonlight_txr_pixrow_t *readonly;

#ifdef __cplusplus

	typedef swordofmoonlight_txr_pixrow_t _pixels;

	/*notice: no validation is done*/
	inline _pixels &operator[](int i)
	{
		return *(_pixels*)(firstrow->indices+rowlength*i); 
	}
	inline const _pixels &operator[](int i)const 
	{
		return *(_pixels*)(readonly->indices+rowlength*i); 
	}
	inline operator bool()const{ return readonly!=0; } 

	inline operator bool(){ return firstrow!=0; } 

#endif

}swordofmoonlight_txr_mipmap_t;

#include "pack.inl" /*push*/

typedef struct _swordofmoonlight_prm_item /*336B*/
{
	ule16_t profile;
	cint8_t name[31];
	cint8_t text[241];
	uint8_t unknown1[22]; /*274*/
	lefp_t weight; /*296*/
	uint8_t unknown2[36];

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_prm_item_t;
typedef struct _swordofmoonlight_prm_magic /*320B*/
{
	ule16_t profile;
	cint8_t name[31];
	uint8_t unknown1[15];
	uint8_t damage[8];
	uint8_t unknown2[4];
	cint8_t text[241];
	uint8_t unknown3[27];

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_prm_magic_t;
typedef struct _swordofmoonlight_prm_object /*56B*/
{
	cint8_t name[31];
	uint8_t unknown1[5];
	ule16_t profile;
	/*
	//38+4: extension? str/mag/?
	*/
	uint8_t unknown2[18];

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_prm_object_t;
typedef struct _swordofmoonlight_prm_enemy /*488B*/
{
	ule16_t profile;
	/*
	//282+4: extension? str/mag/?
	//296+2: max HP word
	//298+2: max MP word? (probably)
	//310+2: view cone wedge word
	//312+4: view cone radius float
	//316+1: view odds
	//317+1: hunter (0) sniper (1) flag
	//318+1: attack recovery
	//324:1: evade (1) defend (2) mode
	//325:1: evade/defend/counter odds
	//328+1: reset recovers HP/MP flag
	//335+1: item
	//336+4: activation radius float
	//340+1: immobilization flag
	//344: begin of attacks?
	//345+24*6: indirect/direct attacks
	//489: end of indirect attacks (overboard?)
	*/	
	ule16_t uknown0;
	lefp_t scale;
	cint8_t name[31];
	uint8_t unknown1[295];
	uint8_t item;
	uint8_t unknown2[153];

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_prm_enemy_t;
typedef struct _swordofmoonlight_prm_npc /*320B*/
{
	ule16_t profile;	
	/*
	//2+2: max HP word
	//4+2: max MP word? (maybe)
	//44+1: item
	//59+1: invincibility (1) flag
	//302+4: extension? str/mag/?
	*/
	ule16_t uknown0;
	lefp_t scale;
	cint8_t name[31];
	uint8_t unknown1[5];
	uint8_t item;
	uint8_t unknown2; 
	/*name+38*/
	uint8_t unknown3[12];
	uint8_t posture;
	uint8_t unknown4[274-13];

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_prm_npc_t;

typedef struct _swordofmoonlight_prt_part
{	
	cint8_t msm[31],_unused1;
	cint8_t mhm[31],_unused2;
	
	/*bits north,west,south,east?*/
	uint8_t blinders; 
	uint8_t iconbyte;
	/*bits damage,poison*/
	uint8_t features;
	
	cint8_t _unused3,bmp[31];
	
	/*EXTENSION DOCUMENTATION
	//aim is to be a SOM_MAP feature that suggests a 
	//default number of turns to standardize layouts
	//iconquad extends map_icon.bmp to no longer use
	//rotations, but to continue to use their spaces
	//where 1 means 400, 3 at 800, 4 at 1200. Future
	//work may open up 256~399, 1056~1199, 1456~1599
	//*/
	#ifndef SWORDOFMOONLIGHT_BIGEND
	uint8_t iconquad:2, aim:3, _reserved_iconshift_etc:3;
	#else
	uint8_t :3, aim:3, iconquad:2;
	#endif
	
	cint8_t single_line_text[31];
	cint8_t text_and_history[97];

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_prt_part_t;	
typedef struct _swordofmoonlight_prf_magic 
{
	enum{ mask=1 };

	SWORDOFMOONLIGHT_RETURN_BY_REFERENCE2(prf_magic)
	
	uint8_t onscreen; /*0~1*/

	ule16_t friendlySFX;

	/*INVESTIGATE ME*/
	ule16_t _unknown1; /*0?*/
	ule32_t _unknown2; /*0?*/

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_prf_magic_t;			
typedef struct _swordofmoonlight_prf_item
{
	enum{ mask=10 };

	SWORDOFMOONLIGHT_RETURN_BY_REFERENCE2(prf_item)

	/*EXTENSION DOCUMENTATION
	//0: "item"
	//1: "melee" weapon, a classical weapon if my==0
	//2: non-weapon equipment
	//3: is a bow like weapon that is like a moveset
	//it reserves moveset_t::position like equipment
	//in case it diversifies into arrows or anything
	//else, but right now the bow itself is a shield
	//and mainly tells SOM_PRM to use arrows instead
	//of sword magics*/
	uint8_t equip; 

	/*EXTENSION DOCUMENTATION
	//my is valid if equip==1. position2 is valid if
	//equip==2 or 3==equip*/
	union
	{
		/*EXTENSION DOCUMENTATION
		//this byte is used by the new MY/ARM etc. body
		//PRF files to designate that they aren't items
		//nonzero codes are:
		//0 for classic weapon profile (i.e. item/prof)
		//1 for head (0 is off the table)
		//2 for arms (weapons, shields, etc.)
		//3 for legs
		//4 for body (ItemEdit uses this as a catch-all
		//it's closer to a full body than upper body)*/
		int8_t my;

		/*EXTENSION DOCUMENTATION
		//if not -1 this is an alternative animation ID
		//to use instead of moveset_t::position
		//REMINDER: nonweapon moves start at move[2] so
		//there are only 2 if position2 is -1*/
		uint8_t position2;
	};

	lefp_t center;
	/*2021: move_t requires a move chain system*/
	union
	{
		/*le16_t up_inhand, up_atrest*/
		le16_t up_angles[2]; 

		/*EXTENSION
		//move_t needs to be able to store at
		//least three more chained move types
		//moveset_t is limited to 255 or less
		//but these are 10 bit units with the
		//high 2 added to move_t::movement so
		//moveset_t's address space is spared
		//uint8_t movement2[4];*/
		uint32_t movement2;
	};

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_prf_item_t;	 
typedef struct _swordofmoonlight_prf_item2
{
	enum{ mask=10 }; /*byte 72*/

	/*ASSUMING UNUSED*/
	uint8_t _unused1[4];

	uint8_t receptacle;

	/*ASSUMING UNUSED*/
	uint8_t _unused2[11];

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_prf_item2_t;	
typedef struct _swordofmoonlight_prf_move
{	
	enum{ mask=11 }; /*byte 72*/

	/*EXTENSION DOCUMENTATION
	//this is the old weapon format and the new arm
	//format. new weapons get this information from
	//the arm. there is one PRF for each animation*/

	uint8_t movement; /*animation number*/
	uint8_t SND_delay;
	le16_t SND; /*-1=none*/
	uint8_t hit_window[2];
	le16_t hit_pie;  /*0~360*/
	lefp_t hit_radius;

	/*EXTENSION DOCUMENTATION
	//zero is 0 for move_t, nonzero for moveset_t 
	//NOTE: was SND_pitch an extension? (yes)*/
	int8_t zero_if_move_t,SND_pitch;

	uint8_t swordmagic_window[2]; /*0~255*/

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_prf_move_t;	
typedef struct _swordofmoonlight_prf_moveset
{
	enum{ mask=12 }; /*byte 72*/

	/*EXTENSION DOCUMENTATION
	//a new weapon format w/ 4 attack move-set that
	//gets some of its information from an MDL file
	//and the rest from MY/PROF/ARM PRF files which
	//use the original weapon PRF file formatting*/
	union
	{
		/*0:head
		//1:body
		//2:arms
		//3:legs
		//4:suit
		//5:shield
		//6:accessory*/
		int8_t position; /*i.e. "equip" slots*/
		uint8_t move[4]; /*animation number*/
	};
	le16_t SND[4]; /*-1=none*/	
	/*
	//-24~-1=lower pitch
	//0=no attack, or this is an old/arm weapon PRF 
	//1~20=raise pitch
	//127=default pitch (0 is used to detect equip1)
	//-128=normal pitch (2021)
	//
	//NOTE: with 2 or 1 attacks a weapon can assign
	//those hit-points to passive-defense as if the
	//weapon is like a shield or piece of equipment
	//with non-negligible coverage
	*/
	union
	{
		/*EXTENSION DOCUMENTATION
		//nonzero is 0 for move_t, nonzero for moveset_t */
		int8_t nonzero_if_moveset_t,SND_pitch[4]; 
	};

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_prf_moveset_t;	
typedef struct _swordofmoonlight_prf_object
{
	enum{ mask=20 };

	SWORDOFMOONLIGHT_RETURN_BY_REFERENCE2(prf_object)

	int8_t billboard; /*0~1*/

	/*CONSEQUENTIAL?
	//chests,switches,doors,traps... seems not
	//to do anything*/
	uint8_t openable; /*0~1*/  

		/*64B marker*/

	lefp_t height; /*traps looks like CP size*/
	lefp_t width; /*radius, not diameter*/
	lefp_t depth; 
	/*CONSEQUENTIAL?
	//1.0 for 0000.prf, 0001.prf, and 2 barrels
	//trying 5 didn't seem to affect 0000.prf
	//trap clip tests (not hit tests) may use 
	//this per CP. Or at least they use memory
	//at this location but it's different from
	//the file's value, at least when zeroed*/
	lefp_t _unused1;

		/*80B marker*/

	int8_t clipmodel; /*0~2*/
	int8_t animateUV; /*0~2*/

	/*
	case "\x14": //box
	case "\x15": //chest
	case "\x16": //corpse
	case "\x0B": //door (secret door perhaps)
	case "\x0D": //door (wood swinging door)
	case "\x0E": //door
	case "\x0A": //lamp
	case "\x29": //receptacle (switch)
	case "\x28": //switch
	case "\x1E": //trap (spear variety perhaps)
	case "\x1F": //trap (arrow variety perhaps)
	case "\x00": //inert (may be ornamental)	
	*/
	ule16_t operation;
	
		/*84B marker*/

	le16_t flameSFX; /*-1=none*/

	/*SOMETHING... crashes if 0x80,40,20 not 0x10,f,1,1f*/
	uint8_t _unknown3; /*0? 0~31?*/

	uint8_t flameSFX_periodicity; /*0~255 frames*/

	le16_t loopingSND; /*-1=none*/
	le16_t openingSND; /*-1=none*/
	le16_t closingSND; /*-1=none*/
	uint8_t loopingSND_delay; /*0=mute?*/
	uint8_t openingSND_delay; /*0=mute?*/
	uint8_t closingSND_delay; /*0=mute?*/
	int8_t loopingSND_pitch; /*-24~20*/
	int8_t openingSND_pitch; /*-24~20*/
	int8_t closingSND_pitch; /*-24~20*/

		/*100B marker*/

	le16_t trapSFX; /*-1=none*/
	int8_t trapSFX_orientate; /*0~1*/
	int8_t trapSFX_visible; /*0~1*/

	int8_t loopable; /*0~1*/
	int8_t invisible; /*0~1*/
	uint8_t receptacle; 
	int8_t sixDOF; /*0~1*/

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_prf_object_t;		 
typedef struct _swordofmoonlight_prf_data
{
	uint8_t time; 
	union{ uint8_t CP; int8_t pitch; };
	le16_t effect;
}swordofmoonlight_prf_data_t;
typedef struct _swordofmoonlight_prf_enemy
{
	enum{ mask=30 };

	SWORDOFMOONLIGHT_RETURN_BY_REFERENCE2(prf_enemy)
			
		/*2022: may be using this to control
		//UV animations same as with objects
		//under the rubric of "skin"
		*/
	le16_t _unknown1; /*0*/

	uint8_t skin; /*0~1*/
	cint8_t skinTXR[31];
	  
	/*this appears to somewhat correspond to 
	//SOM_PRM's monster screen #1. the first 
	//4 bits disable countermeasures whereas
	//the last 4 are concerned with movement
	*/
	#ifndef SWORDOFMOONLIGHT_BIGEND
	/*bit 2 prevents shadows*/
	uint8_t countermeasures:4;
	/*0=walk,0x2=fly,0x1=turn,0x5=none?*/
	uint8_t locomotion:4;
	#else
	uint8_t locomotion:4, countermeasures:4;
	#endif

	uint8_t activation; /*0~1*/

	le16_t _unknown2; /*0 byte 36/x24*/

	/*note: historically shadow is used for
	a few things other than shadow diameter 
	but that shouldn't be the case */
	lefp_t height,shadow,diameter;

	/*unused?
	0030/65.prf ("gremlin") 
	0056.prf ("kald")
	these are fliers, but others have 0,0,0
	1,
	1.5,
	2 (1.5)
	byte 50/x32
	lefp_t flying_related[3];*/
	/*EXTENSION
	//turning_table should be ignored if 
	//turning_ratio is 0.0
	lefp_t flying_related[3];
	REPURPOSING*/
	le32_t _unknown3[3-2]; 
	/*EXTENSIONS
	these 2 depend on turning_ratio being
	nonzero
	*/
	uint8_t turning_cycle[2][2]; //#23 and #24 
	le32_t turning_table;
	/*EXTENSION
	//if 0 the PRF file had not previously
	//used this value. not sure how to 
	//represent 0 (binary inf: 0x7f800000)
	le32_t _unknown3; //0*/
	lefp_t turning_ratio;

	uint8_t hit_delay[3][3];

	uint8_t defend_window[2]; /*byte 75/x4B*/

	uint8_t _unknown4; /*0*/

	le16_t flameSFX; /*-1=none*/
	uint8_t flameSFX_greenofRGB; /*CP selection*/
	uint8_t flameSFX_periodicity; /*0~255 frames*/

	uint8_t direct,indirect;
	cint8_t descriptions[6][21]; /*byte 84*/

	/*I've not found "larger_radii" used anywhere
	but it's been extended to be used to trigger
	the attack, which is especially useful when it
	moves the base CP*/
	lefp_t larger_radii[3],radii[3];
	ule16_t pies[3]; /*0~360*/

	/*EXTENSION
	le16_t _unknown5; //0 byte 240/xf0
	le32_t _unknown6; //0*/
	uint8_t inclination[2];
	lefp_t flight_envelope; /*EXTENSION*/

	/**count/offset pairs that locate packed
	//words having an 8-bit time, 8-bit pitch,
	//16-bit SFX.dat number... or time, green
	//value of CPs and SND number in dataSND's
	//case. subscripts relate to animation IDs*/
	ule16_t dataSFX[32][2], dataSND[32][2];

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_prf_enemy_t;
typedef struct _swordofmoonlight_prf_npc
{
	enum{ mask=31 };

	SWORDOFMOONLIGHT_RETURN_BY_REFERENCE2(prf_npc)

	le16_t _unknown1; /*0x100?*/

	lefp_t height,shadow,diameter;

	le16_t _unknown2; /*0?*/

	le16_t flameSFX; /*-1=none*/	
	uint8_t flameSFX_greenofRGB; /*CP selection*/
	uint8_t flameSFX_periodicity; /*0~255 frames*/

	le16_t _unknown3; /*0?*/

	/*doesn't affect walking speed??? oh, it's
	  turning speed. enemies use the PRM file
	  2 seems standard speed

	  speed is 180/ratio (bigger is slower)
	  note: 0 is infinity I guess, SOM_PRM
	  uses 0 too. 0/infinity doesn't turn
	*/
	lefp_t turning_ratio; /*0,1,1.5,2,2.5,3*/
	
	/*these are for the talk animation stops
	//[0] is forward facing
	//[1] is turning right and left
	//[2] is sitting
	//[3] is sitting right and left*/
	uint8_t title_frames[4];

	/*92B mark (from top of record)*/

	uint8_t skin; /*0~1*/
	cint8_t skinTXR[31];

	le32_t _unknown4; /*0?*/

	ule16_t dataSFX[32][2], dataSND[32][2];

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_prf_npc_t;	

/*2021: WORK IN PROGRESS*/
typedef struct _swordofmoonlight_mpx_base /*256B*/
{
	SWORDOFMOONLIGHT_RETURN_BY_REFERENCE2(mpx_base)

	ule32_t flags; /*1 is BSP, 2 is lit?*/

	/*NOTE: these are really 31 characters*/
	cint8_t title[32], bgm[32], maps[3][32];

	/*FYI som_db ignores fov. MPX files
	have 49.75, whereas 411bba sets fov
	to 50 degrees in radians (0.872664)*/
	lefp_t fov, near_z, far_z;
	lefp_t fog; uint8_t bg[4]; /*color*/

	uint8_t ambient[4];

	struct light
	{
		uint8_t color[4]; lefp_t rays[3];

	}lights[3]; /*48B*/

	uint8_t unknown,_pad[3]; /*start layer?*/
	
	lefp_t pc_xyz[3]; ule32_t pc_v; /*start*/

	/*variable-length blocks follow
	objects, 
	enemies, 
	npcs,
	items,
	sky+tiles, (sky is 32-bit number only)
	bsp (first bit of MPX file is nonzero)
	textures,
	vertices,
	msm, (counter is how many tiles exist)
	mhm...*/
}SWORDOFMOONLIGHT_PACK
swordofmoonlight_mpx_base_t;
typedef struct _swordofmoonlight_mpx_objects
{
	ule32_t count;

	struct _item /*68B*/
	{
		/*NOTE: should match first 68B of som_db.exe*/

		ule16_t kind; /*SOM_PRM index*/
		uint8_t _zindex; /*0 (should stay removed)*/
		uint8_t initially_animated;
		uint8_t initially_shown;
		/*EXTENSION
		//need to set these to zindex and tile x/y*/
		uint8_t zindex,setting[2]; 
		lefp_t xyzuvw[6];
		lefp_t scale;

		/*NOTE: the rest depends on the profile used*/

		union /*32B from 36B*/
		{
			struct /*anonymous struct?*/
			{
				uint8_t kept_item;
				uint8_t lock_item;
			};
			struct /*lamp*/
			{
				uint8_t unknown[8];
				uint8_t light_up_objects;
			};

			/*a lot more should go here if mapped out*/

			uint8_t _union[32];
		};
	};

	SWORDOFMOONLIGHT_ITEMIZE_LIST /*_item list[];*/

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_mpx_objects_t;
typedef struct _swordofmoonlight_mpx_npcs
{
	ule32_t count;

	struct _item /*52B*/
	{
		/*NOTE: should match first 52B of som_db.exe
		//I'm going to assume they're the same for 
		//enemies and NPCs until I see otherwise*/


		uint8_t _zindex; /*0 (should stay removed)*/
		/*EXTENSION
		//need to set these to zindex and tile x/y*/
		uint8_t zindex,setting[2]; 
		lefp_t xyzuvw[6], scale;
		ule16_t kind; /*SOM_PRM index*/
		uint8_t frequency,entry,instances;
		uint8_t shadow;
		uint8_t item;
		uint8_t unknown2[13]; /*not read from MAP*/
	};

	SWORDOFMOONLIGHT_ITEMIZE_LIST /*_item list[];*/

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_mpx_npcs_t,
swordofmoonlight_mpx_enemies_t;
typedef struct _swordofmoonlight_mpx_items
{
	ule32_t count;

	struct _item /*40B*/
	{
		uint8_t _zindex; /*0 (should stay removed)*/
		/*EXTENSION
		//need to set these to zindex and tile x/y*/
		uint8_t zindex,setting[2];
		uint8_t unknown1[24];
		ule16_t kind; /*SOM_PRM index*/
		uint8_t unknown2[10];
	};

	SWORDOFMOONLIGHT_ITEMIZE_LIST /*_item list[];*/

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_mpx_items_t;
typedef struct _swordofmoonlight_mpx_tiles
{
	ule32_t sky; /*just sticking this here*/

	/*just ignore this, it would indicate more
	  layers as if the remaining data repeated
	  but From Software didn't include this in
	  the version of SOM that was finally sold

	  TODO: MPY files can store their group ID
	  here
	*/
	ule32_t _unpublished_layer_system_counter;

	ule32_t width, height;

	struct _item /*12B*/
	{
		ule16_t mhm,msm; lefp_t z;

		union
		{
			ule32_t flags; struct
			{
				#ifndef SWORDOFMOONLIGHT_BIGEND
			/*	uint32_t rotation:2,:4,e:1,:9,icon:8,msb:8;*/
				uint32_t rotation:2,:4,e:8,:2,icon:8,msb:8;
				#else
				uint32_t msb:8,icon:8,:2,e:8,:4,rotation:2;
				#endif
			};
		};
	};

	/*SWORDOFMOONLIGHT_ITEMIZE_LIST has a reverse index system
	//I must have thought important for some reason but didn't
	//document it then*/
	enum{ count=100*100 };
	SWORDOFMOONLIGHT_ITEMIZE_LIST /*_item list[];*/

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_mpx_tiles_t;

typedef struct _swordofmoonlight_mpx_bspdata
{
	SWORDOFMOONLIGHT_RETURN_BY_REFERENCE(mpx_bspdata)

	/*PLACEHOLDER
	//
	// This is the data responsible for detecting
	// if cells are occluded in order to not draw
	// them (when not hiding cells it shouldn't.)
	//
	// It's an unexplored detail since SOM_MAP is
	// able to disable it, but it still has to be
	// crossed when not disabled by the first bit.
	//
	// These structures are unpacked in the order
	// they're defined below...
	//
	///////////*/

	struct struct1
	{
		ule32_t count;

		struct var_item /*variable-length*/
		{
			ule32_t unknown1;

			lefp_t xy1[2],xy2[2];

			ule32_t count; /*unknown2_count*/

			struct _item
			{
				ule32_t unknown2[2];
			};

			SWORDOFMOONLIGHT_ITEMIZE_LIST /*_item list[];*/
		};

		var_item var_item_1[1]; /*variable-length*/
	};

	struct struct2
	{
		ule32_t count;

		struct _item
		{
			ule32_t unknown1[4];

			lefp_t xy1[2],xy2[2];

			ule32_t unknown2[2];
				
			ule32_t unknown3;
		};

		SWORDOFMOONLIGHT_ITEMIZE_LIST /*_item list[];*/
	};

	struct struct3
	{
		ule32_t count;

		struct _item
		{		
			lefp_t xy1[2],xy2[2];

			ule32_t unknown1[2];
		};

		SWORDOFMOONLIGHT_ITEMIZE_LIST /*_item list[];*/
	};

	struct struct4 /*ILLUSTRATING*/
	{
		/*this has to be computed from struct1::count
		//by (count+7)/8*count
		ule32_t count;*/
		enum{ count=0 };

		typedef uint8_t _item;

		SWORDOFMOONLIGHT_ITEMIZE_LIST /*_item list[];*/
	};

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_mpx_bspdata_t;
typedef struct _swordofmoonlight_mpx_vertices
{
	SWORDOFMOONLIGHT_RETURN_BY_REFERENCE(mpx_vertices)

	/*WARNING: this memory is UNALIGNED so it shouldn't 
	//be accessed directly*/

	ule32_t count;

	struct _item /*20B*/
	{
		lefp_t pos[3], uvs[2];
	};

	SWORDOFMOONLIGHT_ITEMIZE_LIST /*_item list[];*/

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_mpx_vertices_t;

typedef struct _swordofmoonlight_evt_header
{
	SWORDOFMOONLIGHT_RETURN_BY_REFERENCE(evt_header)

	ule32_t count;
	 
	struct _item /*252B*/
	{
		cint8_t name[31];

		uint8_t type;
		ule16_t subject;
		uint8_t protocol;
		uint8_t item;
		union
		{
			struct
			{			
			ule16_t cone; /*wedge in degrees*/
			ule16_t _pad;
			lefp_t square_x;
			lefp_t square_y;
			lefp_t circle_r;
			};			

			/*2022
			* ext_zr_flags holds 2 bits:
			* bit 1 limits the trigger with
			* the z1,z2 pairs corresponding
			* to the event type
			* bit 2 rotates the trip region
			* by its bound object/character
			* how this is implemented isn't
			* a strict rotation for lateral
			* axes (read source code)
			*/
			struct ext_t /*EXTENSION*/
			{
				lefp_t square_z1;
				lefp_t radius_z1;
				lefp_t radius_z2;
				lefp_t square_z2;

			}ext;
		};
		
		struct pred
		{
			ule16_t test;
			ule16_t index;
			ule16_t value;
			uint8_t logic;
		/*	uint8_t _pad; //ext_zr_flags*/

		}predicate;

		uint8_t ext_zr_flags; /*EXTENSION*/		

		struct loop /*12B*/
		{
			ule32_t offset;
			pred predicate; uint8_t _pad;

		}loops[16];
	};

	SWORDOFMOONLIGHT_ITEMIZE_LIST /*_item list[];*/

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_evt_header_t;

/*INCOMPLETE list of EVT opcodes*/
typedef struct _swordofmoonlight_evt_code
{			 
	ule16_t function, nextcode;

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_evt_code_t;
typedef struct _swordofmoonlight_evt_code00
{
	ule16_t x00, next; /*text*/

	/*SOM trims this to 4 byte multiples*/
	cint8_t text[SWORDOFMOONLIGHT_N(40*7+1)];

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_evt_code00_t;
typedef struct _swordofmoonlight_evt_code01
{
	ule16_t x01, next; /*text plus logfont*/

	ule32_t textcolorref; le32_t logfontweight; 
	cint8_t text_0_logfontface[SWORDOFMOONLIGHT_N(40*7+1+32)];

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_evt_code01_t;
typedef struct _swordofmoonlight_evt_code17
{
	ule16_t x17, x08; /*shop*/

	uint8_t shop; /*0-127*/

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_evt_code17_t;
typedef struct _swordofmoonlight_evt_code19
{
	union{ ule16_t x19,x20; }; /*place npc/enemy*/
	ule16_t x18;

	ule16_t npc;
	uint8_t setting[2];
	ule16_t heading;
	uint8_t zindex,_unused; /*EXTENSION*/
	lefp_t offsetting[3];

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_evt_code19_t,
swordofmoonlight_evt_code1a_t;
typedef struct _swordofmoonlight_evt_code3b
{
	ule16_t x3b, x0c; /*standby map*/
	
	/*EXTENSION
	 * 
	 * these mimic code3c but are just hints
	 * to prioritize which models, etc. load
	 * based on what can be visible on entry
	 * 
	 * nosettingmask (bits)
	 * 1: use the map's built-in start point
	 * 2: ignore z (3D) for priority sorting
	 */
	uint8_t map;
	uint8_t nosettingmask;
	uint8_t setting[2];
	lefp_t zsetting;
}SWORDOFMOONLIGHT_PACK
swordofmoonlight_evt_code3b_t;
typedef struct _swordofmoonlight_evt_code3c
{
	ule16_t x3c, x1c; /*load map*/

	uint8_t map; 
	uint8_t nosetting;
	uint8_t fade[2];
	uint8_t setting[2];
	le16_t heading;
	lefp_t offsetting[3];
	/*bit 1 is heading
	//bits 2,3,4 are offsetting[0], etc.
	//EXTENSION
	//bit 5 computes the difference from
	//the PC and event subject positions
	*/
	uint8_t settingmask;
	uint8_t zindex; /*EXTENSION*/
	uint8_t _unused[2];
}SWORDOFMOONLIGHT_PACK
swordofmoonlight_evt_code3c_t;
typedef struct _swordofmoonlight_evt_code3d
{
	ule16_t x3d, x18; /*warp*/

	uint8_t setting[2];
	le16_t heading; 
	lefp_t offsetting[3];
	/*bit 1 is heading
	//bits 2,3,4 are offsetting[0], etc.
	//EXTENSION
	//bit 5 computes the difference from
	//the PC and event subject positions
	*/
	uint8_t settingmask;
	uint8_t zindex; /*EXTENSION*/
	uint8_t _unused[2];
}SWORDOFMOONLIGHT_PACK
swordofmoonlight_evt_code3d_t;
typedef struct _swordofmoonlight_evt_code50
{
	ule16_t x50, x0c; /*status update*/

	/*status2 could be 16-bit but then it
	//would not be symmetric with code 54*/
	uint8_t status, operation, status2, unused;	
	/*operation 3's operand is a counter*/
	ule16_t unused2, operand;

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_evt_code50_t;
typedef struct _swordofmoonlight_evt_code54
{
	ule16_t x54, x08; /*load counter w/ status*/

	/*the effective range of status2 is 0-249*/
	uint8_t status, status2; ule16_t counter;

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_evt_code54_t;
typedef struct _swordofmoonlight_evt_code64
{
	ule16_t x64, x08; /*engage object*/

	ule16_t object; uint8_t engage; 	

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_evt_code64_t;
typedef struct _swordofmoonlight_evt_code66
{
	ule16_t x66, x1c; /*move object*/

	ule16_t object; 	
	uint8_t setting[2];
	/*FIX ME
	//40a518 treats these as unsigned, but
	//reports in the past are that this is
	//preventing scenarios where the object
	//needs to rotate the other direction*/
	le16_t heading[3];
	ule16_t deciseconds; /*running time*/
	lefp_t offsetting[3];

	uint8_t zindex; /*EXTENSION (adds 4B)*/
	uint8_t _unused[3];

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_evt_code66_t;
typedef struct _swordofmoonlight_evt_code78
{
	ule16_t x78, x08; /*system mode flags*/

	uint8_t sysmode, newflag, unused[2];

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_evt_code78_t;
typedef struct _swordofmoonlight_evt_code8D
{
	ule16_t x8d, next; /*text plus branch*/

	cint8_t text_0_yes_0_no[SWORDOFMOONLIGHT_N(40*7+1+40+1+40+1)];

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_evt_code8D_t;
typedef struct _swordofmoonlight_evt_code90
{
	ule16_t x90, x0c; /*counter update*/

	ule16_t counter, operand; 
	/*source 1's operand is a counter*/
	uint8_t source, operation;

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_evt_code90_t;
typedef struct _swordofmoonlight_evt_codeFF
{
	ule16_t xff, x04; /*end of sequence*/

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_evt_codeFF_t;

typedef const struct _swordofmoonlight_zip_header
{	
	SWORDOFMOONLIGHT_RETURN_BY_REFERENCE(zip_header)

	//ZIP is little-endian according to Wikipedia.
	ule32_t x4034b50;
	ule16_t pkminver;
	ule16_t modemask, approach;
	ule32_t datetime, crc32sum;
	ule32_t bodysize, filesize;
	ule16_t namesize, datasize;

	uint8_t name[SWORDOFMOONLIGHT_N(260)]; /*MAX_PATH*/

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_zip_header_t;

typedef const struct _swordofmoonlight_zip_entry
{
	SWORDOFMOONLIGHT_RETURN_BY_REFERENCE(zip_entry)

	//ZIP is little-endian according to Wikipedia.
	ule32_t x2014b50, versions;
	ule16_t modemask, approach;
	ule32_t datetime, crc32sum;
	ule32_t bodysize, filesize;
	ule16_t namesize, datasize;
	ule16_t comments, diskette, pkattrib;
	ule32_t fsattrib, locentry; 

	uint8_t name[SWORDOFMOONLIGHT_N(260)];

}SWORDOFMOONLIGHT_PACK 
swordofmoonlight_zip_entry_t,
*const*swordofmoonlight_zip_entries_t;

#include "pack.inl" /*pop*/

typedef struct _swordofmoonlight_zip_inflate
{
	/*don't forget to initialize restart to 0!*/
	uint8_t dictionary[32*1024]; size_t restart;
	typedef char private_implementation[4096*4]; 
	private_implementation Swordofmoonlight_cpp;
						   
#ifdef __cplusplus

	/*want this class to be POD so it can be a union
	//_swordofmoonlight_zip_inflate():restart(0){}*/

	inline operator uint8_t*(){ return dictionary; } 	

#endif

}swordofmoonlight_zip_inflate_t;

typedef struct _swordofmoonlight_res_resource
{		
	union 
	{
	const ule32_t *size; /*(-2,-1)*/
	const ule16_t *type; 
	};
	const ule16_t *name;
	const ule16_t *lcid; /*cont...*/

}swordofmoonlight_res_resource_t;
typedef const 
swordofmoonlight_res_resource_t
*swordofmoonlight_res_resources_t; 

#include "pack.inl" /*push*/

typedef const struct _swordofmoonlight_res_fullspec
{
	ule32_t dataversion;
	ule16_t memoryflags;
	ule16_t languageid; /*lcid*/
	ule32_t version;
	ule32_t characteristics; 

}SWORDOFMOONLIGHT_PACK 
/*full specification*/
swordofmoonlight_res_fullspec_t; 

typedef struct _swordofmoonlight_mo_header
{	
	SWORDOFMOONLIGHT_RETURN_BY_REFERENCE2(mo_header)

	ule32_t x950412de; /*can be de120495*/
	ule32_t revisions; /*major and minor*/
	
	/*major revisions 0~1 (is 1 documented?)*/
	ule32_t nmessages, msgidndex, msgstrndex;	
	ule32_t _nhashes, _hashndex; /*unused*/
	
	/*minor revision 1 to do with c-format*/
	ule32_t _nsysdepsegments,_sysdepsegndex;
	ule32_t _nsysdepmessages,_sysdepmsgidndex,_sysdepmsgstrndex;	

}SWORDOFMOONLIGHT_PACK
swordofmoonlight_mo_header_t;

#include "pack.inl" /*pop*/

typedef struct _swordofmoonlight_mo_range_t
{	
	size_t lowerbound, n;
	/*if n is _not_ zero:
	//ctxtlog+idof[0].coff is the lower bound's msgid
	//catalog+idof[0].coff if you need msgctxt\4msgid
	//catalog+strof[0].coff corresponds to the msgstr
	//clen does not include the _final_ 0 terminator*/
	const struct{ ule32_t clen, coff; }*idof, *strof;							  
	const cint8_t *catalog, *ctxtlog;

#ifdef __cplusplus

	inline _swordofmoonlight_mo_range_t operator[](size_t i)
	{
		_swordofmoonlight_mo_range_t out = 
		{lowerbound+i,1,idof+i,strof+i,catalog,ctxtlog};
		return out;
	}

#endif

}swordofmoonlight_mo_range_t;

//previously memory_t but the desired
//form of const doesn't work on typedef
typedef void* swordofmoonlight_lib_ram_t;
typedef void* swordofmoonlight_lib_opaque_t;
typedef const void* swordofmoonlight_lib_rom_t;

#ifndef SWORDOFMOONLIGHT_API
#define SWORDOFMOONLIGHT_API
#endif											 

SWORDOFMOONLIGHT_API
int swordofmoonlight_lib_unmap(swordofmoonlight_lib_image_t*);

/*Ideally: all not-inline routines (interfaces) should go here.
//However: most stuff is declared with C++ style linkage below*/

#ifdef __cplusplus

namespace SWORDOFMOONLIGHT
{ 
namespace _memory_ /*NEW*/
{
	typedef swordofmoonlight_lib_file_t file_t;
	typedef swordofmoonlight_lib_image_t image_t;	

	//THESE DON'T SET THE IMAGE HEADER AT ALL
	typedef swordofmoonlight_lib_ram_t ram_t;
	inline void maptoram(image_t &img, ram_t ram, size_t bytes, int mask=~0)
	{
		img.mode = SWORDOFMOONLIGHT_READWRITE;
		img.file = 0; img.bad = !ram||!bytes; img.size = bytes; img.mask = mask;
		img.set = (le32_t*)ram; img.end = (le32_t*)((int8_t*)ram+bytes); 
	}
	typedef swordofmoonlight_lib_rom_t rom_t;
	inline void maptorom(image_t &img, rom_t rom, size_t bytes, int mask=~0)
	{
		maptoram(img,(ram_t)rom,bytes,mask);
		img.mode = SWORDOFMOONLIGHT_READONLY;
	}
}
namespace mhm /*SWORDOFMOONLIGHT::*/
{
	using namespace _memory_;		
	/*maptofile: modes; 'r' for read, 'w' for read+write.
	//If file_t is 0 mode can alternatively be an open file descriptor.
	//If file_t is 0 and mode is negative, "-mode" will be mapped write-copy*/
	void maptofile(mhm::image_t&, mhm::file_t, int mode='r');		
	/*always unmap after map (even if image is bad)*/
	inline int unmap(mhm::image_t &img){ return swordofmoonlight_lib_unmap(&img); } 
	/*these position the header same as maptofile*/
	void maptoram(mhm::image_t&,mhm::ram_t,size_t);
	void maptorom(mhm::image_t&,mhm::rom_t,size_t);
	
	typedef swordofmoonlight_mhm_header_t header_t;

	inline mhm::header_t &imageheader(mhm::image_t &img)
	{
		return *img.header<mhm::header_t>();
	}
	inline const mhm::header_t &imageheader(const mhm::image_t &img)
	{
		return *img.header<const mhm::header_t>();
	}

	typedef swordofmoonlight_mhm_face_t face_t;	
	typedef swordofmoonlight_mhm_index_t index_t;
	typedef swordofmoonlight_mhm_vector_t vector_t;
	
	int _imagememory(const mhm::image_t&,const void*(&)[4]);

	/*pointers: returns the size of the index pointer or 0
	//if the image is discovered to be "bad." 
	//n is the normals. i is vertex indices. Any can be 0. */
	inline int imagememory(mhm::image_t &img, mhm::vector_t**v=0, mhm::vector_t**n=0, mhm::face_t**f=0, mhm::index_t**i=0)
	{
		const void *ptrs[4] = {v,n,f,i}; return _imagememory(img,ptrs);
	}	
	/*pointers: returns the size of the index pointer or 0
	//if the image is discovered to be "bad." 
	//n is the normals. i is vertex indices. Any can be 0. */
	inline int imagememory(const mhm::image_t &img, const mhm::vector_t**v=0, const mhm::vector_t**n=0, const mhm::face_t**f=0, const mhm::index_t**i=0)
	{
		const void *ptrs[4] = {v,n,f,i}; return _imagememory(img,ptrs);
	}		
}
namespace msm /*SWORDOFMOONLIGHT::*/
{
	using namespace _memory_;
	/*maptofile: modes; 'r' for read, 'w' for read+write.
	//If file_t is 0 mode can alternatively be an open file descriptor.
	//If file_t is 0 and mode is negative, "-mode" will be mapped write-copy.
	//mask: bit 1 is the header including all of the texture references
	//bit 2 includes everything else that makes up a classical MSM file. */
	void maptofile(msm::image_t&, msm::file_t, int mode='r', int mask=0x3,...);		
	/*always unmap after map (even if image is bad)*/
	inline int unmap(msm::image_t &img){ return swordofmoonlight_lib_unmap(&img); } 
	/*these position the header same as maptofile*/
	void maptoram(msm::image_t&,msm::ram_t,size_t);
	void maptorom(msm::image_t&,msm::rom_t,size_t);
	
	typedef swordofmoonlight_msm_textures_t textures_t;

	msm::textures_t &textures(msm::image_t&);

	inline const msm::textures_t &textures(const msm::image_t &img)
	{
		return textures(const_cast<msm::image_t&>(img));
	}

	typedef swordofmoonlight_lib_reference_t reference_t;

	/*texturereferences: get pointers to individual reference strings.
	//out: The address of an array of pointers which will point to each string.
	//fill: The number of pointers pointed to by 'out'. fill should not be larger than out!
    //skip: number of strings to skip before filling out. Returns the number of pointers actually filled*/
	int texturereferences(const msm::image_t&, msm::reference_t *out, int fill, int skip=0);

	typedef swordofmoonlight_msm_vertices_t vertices_t;

	msm::vertices_t &vertices(msm::image_t&);

	inline const msm::vertices_t &vertices(const msm::image_t &img)
	{
		return vertices(const_cast<msm::image_t&>(img));
	}

	typedef ule16_t softvertex_t;
	/*Extension: get the first soft-edge vertex if this MSM file is modified
	//to partition these vertices on the back and store the number after the
	//regular 0-terminator. This is used to blend the seams in the MPX model*/
	const softvertex_t *softvertex(const msm::image_t &img);

	typedef swordofmoonlight_tnl_vertex_t vertex_t;
	
	inline msm::vertex_t *tnlvertexpackets(msm::image_t &img)
	{
		msm::vertices_t &out = msm::vertices(img); return out&&out.count?out.list:0;
	}	
	inline const msm::vertex_t *tnlvertexpackets(const msm::image_t &img)
	{
		const msm::vertices_t &out = msm::vertices(img); return out&&out.count?out.list:0;
	}

	typedef swordofmoonlight_msm_polygon_t polygon_t;
	
	msm::polygon_t *firstpolygon(msm::image_t&);

	inline const msm::polygon_t *firstpolygon(const msm::image_t &img)
	{
		return firstpolygon(const_cast<msm::image_t&>(img));
	}

	/*shiftpolygon: you're responsible for not going over*/
	inline msm::polygon_t *shiftpolygon(msm::polygon_t *p)
	{
		return (msm::polygon_t*)(p->indices+p->corners);
	}
	inline const msm::polygon_t *shiftpolygon(const msm::polygon_t *p)
	{
		return (msm::polygon_t*)(p->indices+p->corners);
	}
		
	/*2022: this walks the polygons in casee the file is longer
	//than the polygons. the returned value is the 0-terminator*/
	const msm::polygon_t *terminator(const msm::image_t &img);
	
	typedef swordofmoonlight_msm_polyinfo_t polyinfo_t;

	/*polyinfo: MSM polygons end roughly at the EOF*/
	static msm::polyinfo_t polyinfo(const msm::image_t &img, int texmask=0xFFFF)
	{	
		msm::polyinfo_t out = {SWORDOFMOONLIGHT_POLYZERO};

		const msm::polygon_t *q,*p = msm::firstpolygon(img); 

		if(p) for(q=p;img>=(p=msm::shiftpolygon(p));q=p) 
		{
			if(1<<q->texture&texmask) out+=q->corners; 
		}		
		return out; 
	}				

	typedef swordofmoonlight_tnl_triple_t triple_t;

	static int triangulatepolygon(const msm::polygon_t *p, msm::triple_t *t, int n, int first=0)
	{
		int i; if(!p||n<p->corners-2) return 0; /*do not allow partials*/

		if(t) for(t+=first,i=n-3;i>=0;i--) /*count triangles if t is missing*/
		{
			t[i][0] = p->indices[0]; t[i][1] = p->indices[i+1]; t[i][2] = p->indices[i+2];
		}
		return p->corners-2;
	}

	typedef swordofmoonlight_msm_polystack_t polystack_t;

	static msm::polystack_t polystack = {-1u,0,0}; /*shared stack (per translation unit)*/

	/*triangulate_t: custom callback signature for triangulatepolygons (below)
	//Note: the way this is setup you can safely cast anything to msm::triple_t* for arg t*/
	typedef int (*triangulate_t)(const msm::polygon_t *p, msm::triple_t *t, int n, int first);

	static msm::polyinfo_t /*consider: lim is the number of polygons to consider; or in other words: not the number you expect to get out*/
	triangulatepolygons(const msm::polygon_t *p, int lim, msm::triple_t *inout, int len, int texmask=0xFFFF, msm::polystack_t &st=msm::polystack, msm::triangulate_t f=0)
	{
		msm::polyinfo_t out = {SWORDOFMOONLIGHT_POLYZERO};
		if(p) for(int i=0;i<lim&&out.triangles<len&&out.partials==0;i++) 
		{				
			if(st<<p&&1<<p->texture&texmask) 
			out+=2+((f?f:msm::triangulatepolygon)(p,inout,len-out.triangles,out.triangles)); 			
			p = msm::shiftpolygon(p);
		}
		return out;
	}

	SWORDOFMOONLIGHT_USER 
	/*flagpolygonedges: eg. a square polygon has a 0 flag along its winged edge.
	//you can pass this as a triangulator_t to generate edgeflags for a triangulation*/
	static int flagpolygonedges(const msm::polygon_t *p, msm::triple_t *t, int n, int first=0)
	{
		int i; if(!p||n<p->corners-2) return 0; /*do not allow partials*/

		if(t) for(t+=first,i=n-3;i>=0;i--){	t[i][0] = 0; t[i][1] = 1; t[i][2] = 0; }

		if(t) t[0][0] = t[p->corners-3][2] = 1; return p->corners-2;
	}
}
namespace mdo /*SWORDOFMOONLIGHT::*/
{
	using namespace _memory_;
	/*maptofile: modes; 'r' for read, 'w' for read+write.
	//If file_t is 0 mode can alternatively be an open file descriptor.
	//If file_t is 0 and mode is negative, "-mode" will be mapped write-copy.
	//mask: bit 1 is header, bit 2 is materials/channels/control points
	//bit 3 includes everything else that makes up a classical MDO file*/
	void maptofile(mdo::image_t&, mdo::file_t, int mode='r',int mask=0x7,...);	
	/*always unmap after map (even if image is bad)*/
	inline int unmap(mdo::image_t &img){ return swordofmoonlight_lib_unmap(&img); } 
	/*these position the header same as maptofile*/
	void maptoram(mdo::image_t&,mdo::ram_t,size_t);
	void maptorom(mdo::image_t&,mdo::rom_t,size_t);
	
	typedef swordofmoonlight_mdo_textures_t textures_t;

	mdo::textures_t &textures(mdo::image_t&);

	inline const mdo::textures_t &textures(const mdo::image_t &img)
	{
		return textures(const_cast<mdo::image_t&>(img));
	}

	typedef swordofmoonlight_lib_reference_t reference_t;

	/*texturereferences: get pointers to individual reference strings.
	//out: The address of an array of pointers which will point to each string.
	//fill: The number of pointers pointed to by 'out'. fill should not be larger than out!
    //skip: number of strings to skip before filling out. Returns the number of pointers actually filled*/
	int texturereferences(const mdo::image_t&, mdo::reference_t *out, int fill, int skip=0);

	typedef swordofmoonlight_mdo_materials_t materials_t;

	mdo::materials_t &materials(mdo::image_t&);

	inline const mdo::materials_t &materials(const mdo::image_t &img)
	{
		return materials(const_cast<mdo::image_t&>(img));
	}

	typedef swordofmoonlight_mdo_controlpoints_t controlpoints_t;

	/*4 control points (careful: return may be 0)*/
	mdo::controlpoints_t &controlpoints(mdo::image_t&);

	inline const mdo::controlpoints_t &controlpoints(const mdo::image_t &img)
	{
		return controlpoints(const_cast<mdo::image_t&>(img));
	}

	typedef swordofmoonlight_mdo_extra_t extra_t;

	typedef swordofmoonlight_mdo_channel_t channel_t;
	typedef swordofmoonlight_mdo_channels_t channels_t;

	mdo::channels_t &channels(mdo::image_t&);

	inline const mdo::channels_t &channels(const mdo::image_t &img)
	{
		return channels(const_cast<mdo::image_t&>(img));
	}

	typedef swordofmoonlight_tnl_vertex_t vertex_t;
	typedef swordofmoonlight_tnl_triple_t triple_t;

	mdo::vertex_t *tnlvertexpackets(mdo::image_t&, signed int ch);
	mdo::triple_t *tnlvertextriples(mdo::image_t&, signed int ch);

	inline const mdo::vertex_t *tnlvertexpackets(const mdo::image_t &img, signed int ch)
	{
		return tnlvertexpackets(const_cast<mdo::image_t&>(img),ch);
	}
	inline const mdo::triple_t *tnlvertextriples(const mdo::image_t &img, signed int ch)
	{
		return tnlvertextriples(const_cast<mdo::image_t&>(img),ch);
	}				 
}
namespace psx /*SWORDOFMOONLIGHT::*/
{
	typedef swordofmoonlight_psx_uword_t uword_t;
	typedef swordofmoonlight_psx_sword_t sword_t;

	template<class T> inline size_t word_size()
	{
		return sizeof(T)/sizeof(le32_t); 
	}
	template<class T> inline size_t word_size(const T &t)
	{
		return sizeof(T)/sizeof(le32_t);
	}

	template<class T> inline T &word_cast(le32_t *out)
	{
		return *reinterpret_cast<T*>(out);
	}		  
	template<class T> inline const T &word_cast(const le32_t *out)
	{
		return *reinterpret_cast<const T*>(out);
	}
}
namespace tim /*SWORDOFMOONLIGHT::*/
{
	typedef swordofmoonlight_lib_image_t image_t;

	/*The rest of namespace tim is defined below mdl (below)*/
}
namespace mdl /*SWORDOFMOONLIGHT::*/
{
	static short round(float f)
	{
		return (short)(f+(f<0?-0.5f:0.5f)); //copysign? std::round?
	}

	using namespace _memory_;
	/*maptofile: modes; 'r' for read, 'w' for read+write.
	//If file_t is 0 mode can alternatively be an open file descriptor.
	//If file_t is 0 and mode is negative, "-mode" will be mapped write-copy.
	//mask: bit 1 includes the header, 2 the primitive headers.
	//bits 3~5 are primitives, hard and soft animations respectively.
	//bits 6~8 are reserved. Bits 9~16 are the embedded TIM images 1~8*/
	void maptofile(mdl::image_t&, mdl::file_t, int mode='r', int mask=0xff1f,...);	
	/*always unmap after map (even if image is bad)*/
	inline int unmap(mdl::image_t &img){ return swordofmoonlight_lib_unmap(&img); }; 
	/*these position the header same as maptofile*/
	void maptoram(mdl::image_t&,mdl::ram_t,size_t);
	void maptorom(mdl::image_t&,mdl::rom_t,size_t);
	
	typedef swordofmoonlight_mdl_header_t header_t;

	mdl::header_t &imageheader(mdl::image_t&);

	inline const mdl::header_t &imageheader(const mdl::image_t &img)
	{
		return imageheader(const_cast<mdl::image_t&>(img));
	}

	/*//Primitives Block /////////////////////////////*/

	typedef swordofmoonlight_mdl_primch_t primch_t;

	mdl::primch_t &primitivechannel(mdl::image_t&, int ch);

	inline const mdl::primch_t &primitivechannel(const mdl::image_t &img, int ch)
	{
		return primitivechannel(const_cast<mdl::image_t&>(img),ch);
	}

	typedef swordofmoonlight_mdl_vertex_t vertex_t;
	typedef swordofmoonlight_mdl_normal_t normal_t;

	mdl::vertex_t *pervertexlocation(mdl::image_t&, int ch);
	mdl::normal_t *pervertexlighting(mdl::image_t&, int ch);

	inline const mdl::vertex_t *pervertexlocation(const mdl::image_t &img, int ch)
	{
		return pervertexlocation(const_cast<mdl::image_t&>(img),ch);
	}
	inline const mdl::normal_t *pervertexlighting(const mdl::image_t &img, int ch)
	{
		return pervertexlighting(const_cast<mdl::image_t&>(img),ch);
	}

	/*primsets: 
	//If a set is positive, it is taken as a single set indicated
	//by the primitive code. If it is negative then the set is a 
	//combination of sets, where each bit is shifted up by the code.
	//The nonprimset constant below "or'd" with any value produces 
	//a negative int -- so the set will be interpreted as a setmask*/

	static const int allprimset = -1; /*Any primitives*/
	static const int nonprimset = 0x80<<sizeof(int)*8-8; /*MSB*/

	/*control points (CPs) / not-CPs / not-square / square */
	static const int controlprimset = nonprimset|1<<0x00|1<<0x03;
	static const int displayprimset = nonprimset|~controlprimset;
	static const int triangleprimset = controlprimset|1<<0x04|1<<0x0D;
	static const int quadrangleprimset = nonprimset|~triangleprimset;

	/*primset: build your own primitive mask like so...
	//int mycodes = [0x0A,0x0D], set = primset(mycodes);*/
	template<int N> inline int primset(const int (&primcodes)[N])
	{			
		int out = nonprimset; for(int i=0;i<N;i++) out|=1<<primcodes[i]; return out;
	}

	/*primsum:
	//in is an array containing primitive counts from
	//the likes of countprimitives below. n is the last
	//primcode and the size of of in where i equals zero. 
	//n and i are primcodes. So adjust in if you must set i*/
	inline int primsum(int set, int *in, int n, int i=0)
	{
		int out = 0; for(/*i=0*/;i<n;i++) if(in[i]) 
		if(set<0?set&1<<i:set==i) out+=in[i]; return out;
	}

	typedef swordofmoonlight_mdl_priminfo_t priminfo_t;

	/*priminfo: works like primsum, just messier to look at*/
	static mdl::priminfo_t priminfo(int set, int *in, int n, int i=0)
	{
		mdl::priminfo_t out = {SWORDOFMOONLIGHT_PRIMZERO};

		for(/*i=0*/;i<n;i++) if(in[i])
		if(set<0?set&1<<i:set==i) switch(i)
		{
		case 0x00: case 0x01: case 0x03: case 0x04: case 0x0D: 
			
			out.triangles+=in[i]; out.singles+=in[i]; break; /*tris*/

		case 0x06: case 0x07: case 0x0A: case 0x11: /*quadrangles*/

			out.triangles+=in[i]*2; out.doubles+=in[i];  break;

		default: out.troubles++; /*up to you to catch these*/
		}		
		return out; 
	}

	/*countprimitives: SOM assigns each primitive a code...
	//Supported codes fall between 0x00 and 0x11 (there may be more)
	//So out needs to be 0x11+1 counters if they are all to be counted.
	//Out is allowed to be zero. The return will be the same regardless.
	//The size of out filled (may be zero or less than len) is returned.
	//Primsum (above) can be used to get the actual number of primitives.
	//Firstprimcode+len works to narrow the counting to a range of codes.
	//Which is only a convenience (everything must be counted regardless)*/
	int countprimitives(const mdl::image_t&, int ch, int *out, int len, int firstprimcode=0, int accum=0);

	/*accumulateprimitives:
	//Pass the return of countprimitives (or accumulateprimitives) to accum to add your results together*/
	inline int accumulateprimitives(const mdl::image_t &in, int accum, int ch, int *out, int len, int fpc=0)
	{
		return countprimitives(in,ch,out,len,fpc,accum);
	}

	/*countprimitivesinset:
	//Chances are this equals primsum(countprimitives())*/
	int countprimitivesinset(const mdl::image_t&, int ch, int set);
			
	typedef swordofmoonlight_mdl_uvparts_t uvparts_t; typedef uvparts_t::_item uvpart_t;
	typedef swordofmoonlight_mdl_prims00_t prims00_t; typedef prims00_t::_item prim00_t;
	typedef swordofmoonlight_mdl_prims01_t prims01_t; typedef prims01_t::_item prim01_t;
	typedef swordofmoonlight_mdl_prims03_t prims03_t; typedef prims03_t::_item prim03_t;
	typedef swordofmoonlight_mdl_prims04_t prims04_t; typedef prims04_t::_item prim04_t;
	typedef swordofmoonlight_mdl_prims06_t prims06_t; typedef prims06_t::_item prim06_t;
	typedef swordofmoonlight_mdl_prims07_t prims07_t; typedef prims07_t::_item prim07_t;
	typedef swordofmoonlight_mdl_prims0A_t prims0A_t; typedef prims0A_t::_item prim0A_t;
	typedef swordofmoonlight_mdl_prims0D_t prims0D_t; typedef prims0D_t::_item prim0D_t;
	typedef swordofmoonlight_mdl_prims11_t prims11_t; typedef prims11_t::_item prim11_t;


  /*FIX ME***FIX ME***FIX ME***FIX ME***FIX ME***FIX ME***FIX ME
  //
  // While all of SOMs MDL files appear to follow this pattern, at
  // least EneEdit has code for when these are not present, or for
  // when there is multiple such UV maps (apparently limited to 4
  // sided polygons.) The header reports the number present, but it
  // is still unclear how to individual groups are targeted by the
  // 4-sided primitives!
  //
  //////////////////////////////////////////////////////////////
	*/
	/*OLD: MAY WANT TO DEPRECATE*/
	/*partialprimitives: //RETURNS 0 IF EMPTY
	//MDL format begins with some primitive-like things that are 
	//actually shared parameters indexed by "impartial" primitives.
	//There is only one kind. They come first and are codeless, so..*/
	mdl::uvparts_t &partialprimitives(mdl::image_t&);
	/*returns 0 if partials are not required for set argument*/
	inline const mdl::uvparts_t &partialprimitives(const mdl::image_t &img, int set=mdl::allprimset)
	{
		return set<0&&set&(1<<0x7|1<<0xA|1<<0x11)||set>=0&&(set==0x7||set==0xA||set==0x11)
		?partialprimitives(const_cast<mdl::image_t&>(img)):*(mdl::uvparts_t*)0;
	}
	inline mdl::uvparts_t &partialprimitives(mdl::image_t &img, int set)
	{
		return set<0&&set&(1<<0x7|1<<0xA|1<<0x11)||set>=0&&(set==0x7||set==0xA||set==0x11)
		?partialprimitives(img):*(mdl::uvparts_t*)0;
	}
	/*NEW: always returns nonzero if not a bad file*/
	mdl::uvparts_t &quadprimscomplement(mdl::image_t&);

	struct primtable_t //swordofmoonlight_mdl_primtable_t
	{
	const prims00_t *prims00; //00: flat-lit RGB triangles
	const prims01_t *prims01; //01: flat-lit UVs triangles
	const void *_prims02;
	const prims03_t *prims03; //03: soft-lit RGB triangles
	const prims04_t *prims04; //04: soft-lit UVs triangles
	const void *_prims05;
	const prims06_t *prims06; //06: flat-lit RGB quadrangles
	const prims07_t *prims07; //07: flat-lit UVs quadrangles
	const void *_prims08, *_prims09;
	const prims0A_t *prims0A; //0A: soft-lit UVs quadrangles
	const void *_prims0B, *_prims0C;
	const prims0D_t *prims0D; //0D: unlit UVs/RGB triangles
	const void *_prims0E, *_prims0F, *_prims10;
	const prims11_t *prims11; //11: unlit UVs/RGB quadrangles
	const void *operator[](int i)const{ return ((void**)this)[i]; }
	};
	template<class prims> inline int primwords(const prims *p)
	{
		return p?sizeof(prims::_item)/4*p->count:0; 
	}
	primtable_t primitives(const mdl::image_t &img, int ch);

	typedef swordofmoonlight_mdl_triangle_t triangle_t;

	/*triangulateprimitives:
	//Streaming is accomplished by feeding the return back into the restart argument*/
	int triangulateprimitives(const mdl::image_t&, int ch, int set, mdl::triangle_t*, int len, int restart=0);

	inline mdl::triangle_t triangulateprimitive(const mdl::image_t &img, int ch, int set, int restart=0)
	{
		mdl::triangle_t out = {0}; triangulateprimitives(img,ch,set,&out,1,restart); return out;
	}

	/*2018
	//int2 was "long" but long is 64-bit on some systems. 64 seems like overkill. I can't
	//remember the original intent, other than to be "cute" because it was mainly used to
	//notate so-called "long" indices; longer than MDL's native 16-bit ones.
	//Long term MDL will be used to clip models regardless, so 16-bits is probably plenty
	//and aggregation is unlikely to be of need*/
	typedef int32_t int2;

	/*trianglemats:
	//Each int2 is a 32bit mask per the 32 texture pages. If a texture page is encountered
	//it is recorded in the mask. Additionally 4 transparency modes can be captured by abr.
	//
	//PlayStation ABR blend rops
	//00:  50%back +  50%polygon
	//01: 100%back + 100%polygon
	//10: 100%back - 100%polygon 
	//11: 100%back +  25%polygon
	//
	//Background:
	//By convention SOM likes 12,28,8,24 for its TIM images. 0,4,16,20 might be worth trying also.
	//These pages correspond to the PlayStation frame-buffer which the TIM images map to. If there
	//is no image in a bank, SOM defaults to the first image of the MDL file (unless this is wrong)
	//
	//The 8 pages mentioned above can all hold 256x256 textures at once. For the PlayStation the 
	//final rendering of the screen would need to go somewhere, but SOM has no parallel to deal with.
	//In theory you can go upto 32 64x256 textures. But it's unlikely SOM's simulation stretches so far.
	//
	//If abr is 0 everything will go into tpn (assuming that you are concerned with textures only)*/
	static int2 trianglemats(const mdl::triangle_t *p, int len, int2 abr[4]=0, int2 tpn=0, bool clear=true)
	{
		if(clear) if(abr) memset(abr,tpn=0,sizeof(*abr)*4); else tpn = 0; 
		int2 out = tpn;	
		for(int i=0;i<len;i++,p++) if(p->edges)		
		if(abr&&p->tmd.abe) abr[p->tmd.abr]|=1<<p->tmd.tpn; else out|=1<<p->tmd.tpn;
		return out;
	}	
	
	static const int tpnbits = 0x1F, abrbits = 0x60, abebits = 0x2000000;
	static const int tpnshift = 0, abrshift = 2; //right shift

	static int trianglematsort(mdl::triangle_t *dst, const mdl::triangle_t *src, int len, int2 abr[4]=0)
	{
		int2 tpn = mdl::trianglemats(src,len,abr); unsigned innerloops = abr?5:1; int at = 0;

		/*unsigned: avoiding signed/unsigned mismatch compiler warnings*/
		for(unsigned i=0;i<32;i++) for(unsigned j=0;j<innerloops;j++) if(!j&&tpn&1<<i||j&&abr[j-1]&1<<i)
		for(int k=0;k<len;k++) if(src[k].tmd.tpn==i) if(src[k].tmd.abe?!abr||j&&src[k].tmd.abr==j-1:!j)
		{
			if(src[k].edges) dst[at++] = src[k]; /*sort order is by texture and then transparency mode*/
		}		

		if(len>at) while(at!=len) dst[at++].edges = 0; return tpn;
	}

	/*///VERTEX BUFFERS ////////////////////////////////////////////////
	//
	//The MDL format (unlike MDO and MSM) stores its position and lighting and texture coordinates
	//all in different places and different ways. This kind of setup won't fly with real-time hardware.
	//So you need to create a buffer where each set of values is its own multi-attribute vertex packet.
	//The interfaces below can help with this. We don't sort the vertices and the algorithm is not based
	//on the one Sword of Moonlight must use at run-time (it's possible they are identical but unlikely)*/

	typedef swordofmoonlight_mdl_v_t v_t;
	typedef swordofmoonlight_mdl_vbuffer_t vbuffer_t;
	typedef swordofmoonlight_mdl_const_vbuffer_t const_vbuffer_t;
		
	/*newvertexbuffer: _implementation subject to change_
	//You will need to create your vertex buffer to get started.
	//n is the number of vertices you need to fill the buffer up with*/
	inline int2 newvertexbuffer(mdl::vbuffer_t &v, int2 n, bool fill=0, float room=1.5f)
	{	
		int2 m = n>2?int((room>1?room:1.0f)*n):2; v = new swordofmoonlight_mdl_v_t[m];

		memset(v+n*fill,0x00,sizeof(*v)*(m-n*fill)); v[m-1].hash = n>0?n:0; return m;
	} 

	/*fillvertexbuffer: (see newvertexbuffer above)
	//
	//Rationale
	//If you need to draw the MDL in real-time you will need to use this (or roll your own solution)
	//All you really need to know is that the size of the buffer array is managed by fillvertexbuffer...
	//And that it returns the new capacity of the buffer array. Which you MUST keep track of; so don't loose it.
	//
	//Arguments
	//m: the capacity of the buffer. You must keep track of this yourself. 
	//n: the number of triangles to process: all triangles _will_ be processed no matter what.
	//lit/uvs: Leaving one out will yield a smaller vertex buffer. Anything missing will be 0.
	//o: the offset into the vertex buffer from where the triangles indices will be taken to start.
	//out: You can provide 3 ints per triangle (3*n) to receive the indices into the vertex buffer...
	//Alternatively the indices will overwrite what is in the pos members of the triangle_t argument. 
	//Note that mdl::triangle_t address space is 0~65535. Out is 0~virtual infinity in comparison.
	//
	//Caveats
 	//Degenerate triangles will have indices (0,0,0) and CP triangles will add vertices with (0,0) UVs.
	//Primcodes 0D and 11 will have (0,0,0) lighting normals: it's unclear if these are usable in a game.
	//
	//Ex. int m = mdl::newvertexbuffer(v,ch.vertcount); m = fillvertebuffer(v,m,...); //where ch is your mdl::primch_t*/
	int2 fillvertexbuffer(mdl::vbuffer_t&,int2 m,mdl::triangle_t*,int n,int2 o,const mdl::vertex_t*pos,const mdl::normal_t*lit=0,bool uvs=true,int2*out=0);
	
	/*copyvertexbuffer: see soft animation routines below*/

	/*sizeofvertexbuffer: _implementation subject to change_
	//The size of the buffer is not the same as the capacity (at most m-1)*/
	inline int2 sizeofvertexbuffer(mdl::const_vbuffer_t v, int2 m)
	{ 
		int2 out = v&&m>1?v[m-1].hash:0; return out<m&&out>0?out:0;
	}

	/*restartvertexbuffer: _implementation subject to change_
	//If you don't need what's in a buffer anymore but need to make more 
	//buffers, you can use this to reuse the memory instead of deleting it*/
	inline void restartvertexbuffer(mdl::vbuffer_t v, int2 m, int2 n)
	{
		if(v&&m>1&&n>=0&&n<m){ memset(v,0x00,sizeof(*v)*v[m-1].hash); v[m-1].hash = n; }
	}

	/*deletevertexbuffer: _implementation subject to change_*/
	inline void deletevertexbuffer(mdl::vbuffer_t v){ delete[] v; } 

	/*///Animation Blocks ////////////////////////////*/

	typedef swordofmoonlight_mdl_animation_t animation_t;

	typedef swordofmoonlight_mdl_const_animation_t const_animation_t;

	/*/animations: fill a list of animation pointers with what is in the file
	//
	//IDs: IDs to include or 0 for all where IDs[0] is the number and IDs[1] is the 1st ID.
	// IMPORTANT: Any omitted IDs due to IDs being nonzero will have their pointer set to 0. 
	//skip: number of animations to skip before filling. Hard come first then soft (in file order)
	//
	//FYI: The animations are not random access. You should get as many at once as you can*/
	int animations(mdl::image_t&, mdl::animation_t *out, int len, int *IDs=0, int skip=0);

	inline int animations(const mdl::image_t &img, mdl::const_animation_t *out, int len, int *IDs=0, int skip=0)
	{
		return mdl::animations(const_cast<mdl::image_t&>(img),const_cast<mdl::animation_t*>(out),len,IDs,skip);
	}

	typedef swordofmoonlight_mdl_hardanim_t hardanim_t;
	typedef swordofmoonlight_mdl_softanim_t softanim_t;
	
	int animationchannels(const mdl::image_t&, bool soft=false);
		
	template<class anim_t> inline int animationchannels(const mdl::image_t &img)
	{ 
		return mdl::animationchannels(img,anim_t::SOFT); /*mdl::hard/softanim_t;*/
	}

	/*/mapanimationchannels: set map members of out... 
	//
	//len: must equal animationchannels<hardanim_t>() until further notice.
	//
	//Prepares out for use with animate/transform (technically just transform)
	//Alternatively: you can use separate below. In fact it may be what you want.
	//
	//FYI: the map member of hardanim_t forms the basis of the MDL's "skeleton".
	//Each map is the channel number of its upstream channel (or "parent") and 
	//-1 is the root. More than one root can exist but such an arrangement may 
	//well not be compatible with Sword of Moonlight for one reason or another*/
	int mapanimationchannels(const mdl::image_t&, mdl::hardanim_t*out, int len);
												
	/*/separate:
	//A hardanim_t must be initialized before you can transform it.
	//This is the simplest way (see mapanimationchannels above) and is
	//advantageous if what you want is the local transformation information*/
	inline void separate(mdl::hardanim_t *out, int len)
	{
		for(int i=0;i<len;i++) out[i].map = -1; /*so all nodes look like a root*/
	}

 /*///WARNING: last I looked validate was not implemented -Old Hand ///*/

/*#define SWORDOFMOONLIGHT_CAN_VALIDATE*/

	/*validate: validate the integrity of an animation (todo)*/
	bool validate(const mdl::image_t&, const mdl::animation_t);

	/*/animate: _pay attention to first until further notice_
	//Clr is how many of out to reset to zero when now equals 0.
	//First must be set to true when animating the first animation.
	//Now is 0 to begin or the return value of last call to animate.
	//
	// (Note: "first" is because the parent index is embedded there.)
	//
	//Steps allows you to advance thru more than 1 unit of time. And also
	//the ability to rewind by supplying a negative step value. However it
	//makes no sense to step backwards through the animation before stepping
	//forward. If you wanted to play the animation backwards for instance. You
	//must first play it forward to the end and then start playing backwards.
	//
	//The new now is returned. However, if now is less than zero or greater 
	//than or equal to the animation running time or steps is 0, then 0 is returned.
	//0 is also returned if any trouble occurs. Just test for 0 for any abnormality*/
	int animate(mdl::const_animation_t, mdl::hardanim_t *out, int clr, int now, bool first, int steps=1);
		
	/*The following "constants" are shared (per translation unit)*/

	/*Turns out 1024 is it. But I swear the model-view scale is 1/1000.
	//I wonder if the player is scaling down to 1000... or is it 10000? */
	#if 0
//	static float m = 1/1000.0f, m3[4] = {m,m,m,1}; /*meter scale factor*/
//	static float mm = 1.0f, mm3[4] = {mm,mm,mm,1}; /*millimeter scale factor*/
	#else
	static float m = 1/1024.0f, m3[4] = {m,m,m,1}; /*meter scale factor*/
	static float mm = 1.024f, mm3[4] = {mm,mm,mm,1}; /*millimeter scale factor*/
	#endif
	static float n = 1.0f/4096, n3[4] = {n,n,n,0}; /*unit normal scale factor*/

	/*/transform: _before using: you must separate or mapanimationchannels n_
	//
	//	WARNING: the new mdl::hardanim_t::cs scale factor isn't implemented
	//	and you can't factor it into the scale parameter. SFX files use the
	//  scale factors. It just needs to be tested, and demonstrated correct.
	//
	//Matrices are row major order with 0~3 as the X vector of the orthonormal 
	//matrix, 4~7 as the Y vector, 8~11 as the Z vector, and 12~15 as the origin.
	//This is classical OpenGL representation. Direct3D may have order agnostic APIs
	//for loading matrices up, but either way you can use transpose (below) if necessary.
	//
	//vinout is a 3 float array corresponding to a 3d vector to be animated.
	//nmats is an optional array of 16 float matrices, one matrix per hardanim_t.
	// Note: omitting nmats is not thread safe. If nknown is 0 all mats are unknown.
	//nknown is a boolean array corresponding to nmats, one per matrix (not per float)
	//if nknown[i]==true then the matrix is not computed, else nknown[i] is set to true.
	//scale: if 0 1:1 (mdl::mm) scale is used (millimeter scale is the native scale factor)
	// Note: the default below is not 0. It is 1 meter scale (or mdl::m)
	//
	//Alternatively:
	//If vinout is omitted nmats and nknown will still be populated.
	//You can calculate all of the matrices by running transform over every channel.
	//The matrices will be either global or local depending on whether n was initialized
	//with mapanimationchannels (global) or separate (local)
	//
	//Important: what scale is and what scale is not...
	//The translation components of the animations are scaled by scale when building the
	//nmats transformation matrices. If scale[3] is 0 3x3 rotation only matrices will be
	//
	// 2021: thinking "scale[3]" is too baroque, or what what I thinking when I added it.
	// 
	//built if nknown[i] is false and vinout will be treated as a normal (not translated) 
	//Note: if the matrices are all known it is safe to multiply a normal with 4x4 nmats.
	//Also: if scale[3] is 0 scale[0~2] just affect the sign (in case you were wondering)
	//
	//FYI: The maximum number of channels is 254 if nmats is nonzero, otherwise 127*/
	bool transform(const mdl::hardanim_t *n, int ch, float vinout[3]=0, const float scale[4]=0, float *nmats=0, bool *nknown=0);
		
	/*multiply: better to use this than transform for multiple vertices*/
	static void multiply(const float *mat, const float *vin, float *vout, bool homogeneous=true, int m=3, int n=1)
	{
		float vtmp[3], *vptr=vtmp, *&vsrc = vin==vout?vptr:(float*&)vin;

		for(int i=0;i<n;i++,vin+=m,vout+=m)
		{
			if(vin==vout) vtmp[0]=vin[0],vtmp[1]=vin[1],vtmp[2]=vin[2]; 

			vout[0] = mat[0]*vsrc[0] + mat[4]*vsrc[1] + mat[ 8]*vsrc[2];
			vout[1] = mat[1]*vsrc[0] + mat[5]*vsrc[1] + mat[ 9]*vsrc[2];
			vout[2] = mat[2]*vsrc[0] + mat[6]*vsrc[1] + mat[10]*vsrc[2];
				
			if(homogeneous) /*0 or 1 (0 for normals, 1 for vertices)*/
			{
				vout[0]+=mat[12]; vout[1]+=mat[13]; vout[2]+=mat[14];
			}
		}
	}

	/*/transpose: generic matrix transposition over m matrices
	//
	//WARNING: multiply (above) is incompatible after trasponse*/
	static void transpose(float *dst, const float *src, int m=1)
	{
		float *p = dst; const float *q = src;	   

		for(int i=0;i<m;i++,p+=16,q+=16) 
		{
			if(src==dst) 
			{	
				float q1 = q[1], q2 = q[2], q3 = q[3];	
				p[4] = q1; p[8] = q2; p[12] = q3; 

				float q6 = q[6], q7 = q[7], q11 = q[11];				
				p[9] = q6; p[13] = q7; p[14] = q11;				
			}
			else
			{					
				p[0] = q[0]; p[4] = q[1]; p[8] = q[2];				
				p[5] = q[5]; p[9] = q[6]; p[10] = q[10]; 

				p[12] = q[3]; p[13] = q[7];	p[14] = q[11]; p[15] = q[15];				
			}

			p[1] = q[4]; p[2] = q[8]; p[3] = q[12];	 
			p[6] = q[9]; p[7] = q[13]; p[11] = q[14];
		}
	}
	
	/*inverse: generic matrix inversion over m matrices*/
	void inverse(float *dst, const float *src, int m=1);

	/*product: generic matrix multiplication over m matrices*/
	void product(float *result, const float *left, const float *right, int m=1);

	static void identity(float *dst, int m=1)
	{
		static const float id[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};

		for(int i=0;i<m;i++) memcpy(dst+i*16,id,sizeof(float)*16);
	}
	
	typedef swordofmoonlight_softanimframe_t softanimframe_t;
	typedef swordofmoonlight_softanimcodec_t softanimcodec_t;
	typedef swordofmoonlight_softanimdatum_t softanimdatum_t;

#ifndef SWORDOFMOONLIGHT_FRAMEMAX
#define SWORDOFMOONLIGHT_FRAMEMAX 32
#endif	
		static const int framemax = SWORDOFMOONLIGHT_FRAMEMAX;

	inline int softanimframestrlen(const mdl::softanimframe_t *p,  void *safeword=0)
	{
		const mdl::softanimframe_t *q = safeword?(mdl::softanimframe_t*)safeword:p+mdl::framemax; 

		int out = 0; if(p) while(*p&&p<q) out++,p++; return p&&!*p&&p<=q?out:0;
	}
	inline int softanimframestrtime(const mdl::softanimframe_t *p, void *safeword=0)
	{
		const mdl::softanimframe_t *q = safeword?(mdl::softanimframe_t*)safeword:p+mdl::framemax; 

		int out = 0; if(p) while(*p&&p<q) out+=p->time, p++; return p&&!*p&&p<=q?out:0;
	}	
	inline int softanimframechannel(const mdl::softanimframe_t *p)
	{
		return p&&p->lo?(p->lo<0?-p->lo:p->lo)-1:0; /*warning: defaults to channel 0*/
	}
	inline signed int softanimframedelta(const mdl::softanimframe_t *p)
	{
		return p?(p->lo<0?-int(p->time):p->time):0;
	}

	/*2024: get animation time (resolves ambiguity with union) */
	inline int softanimtime(mdl::const_animation_t a)
	{
		return softanimframestrtime(a->frames);
	}
	/*2024: get animation time (resolves ambiguity with union) */
	inline int hardanimtime(mdl::const_animation_t a)
	{
		return a->htime;
	}
	/*2024: get animation time (resolves ambiguity with union) */
	int animtime(const mdl::image_t &, mdl::const_animation_t a);

	/*/softanimframes: figure the total number or the number of unique frames.
	//IDs: IDs to include or 0 for all where IDs[0] is the number and IDs[1] is the 1st ID.
	//
	//Getting the total number of frames is fast. To get unique frames, unique must be at
	//least as many as the number of total frames where len is the size of unique. If len is 
	//not the exact size of the total frames then 0 is returned. Otherwise an index of unique 
	//frames per total frames is stored inside unique. 
	//
	//Note: uniqueness is determined by tracking the steps per channel per animation.
	//The channel data is not considered. So this is a relatively non-invasive procedure...
	//While on the otherhand, it doesn't handle cases where the datapoints might happen to 
	//to arrive at the same configuration by taking a different direction. You might want to 
	//compare the frames that you end up with against one another if you can afford to do so.
	//
	//Important: when figuring the number of unique frames the initial frame is never counted.
	//Its index is equal to the total number of unique frames (which is also the return value)
	//In addition the index is arranged in lowest to highest order with respect to uniqueness*/
	int softanimframes(const mdl::image_t&, const int *IDs=0, int *unique=0, int len=0);

	static mdl::softanim_t softanim = {0,0,0,0}; /*shared softanim (per translation unit)*/

	/*/A serial version of softanimchannel (below)
	//Note: not to be confused with animationchannels (which should be used to get the total number of channels)*/
	mdl::softanim_t *softanimchannels(const mdl::image_t&,const mdl::softanimframe_t*deltas,mdl::softanim_t*out,int n,const mdl::softanim_t *defaults=0);

	/*/softanimchannel: initialize a softanim_t (it may then be configured to your needs)
	//defaults: if non zero all channels will be copy-initialized with the values of *defaults.
	//Tip: mdl::softanimframe_t tmp = {ch+1,0}; works if you just need to get at the channel by way of a softanim_t*/
	inline mdl::softanim_t &softanimchannel(const mdl::image_t&img,const mdl::softanimframe_t*delta,mdl::softanim_t&out=mdl::softanim,const mdl::softanim_t *defaults=0)
	{
		return *mdl::softanimchannels(img,delta,&out,1,defaults);
	}

	/*/softanimvertices: convert indices into soft animation space.
	//
	//primch: this is the primitive channel the input indices are from.
	//in: if non-zero the pos indices are converted and stored in inout.
	// If inout is 0 the 16bit pos indices of in are converted in place.
	// If 0 the indices are taken from inout and converted in place.
	// Note: translate (below) can only work with "int2" type indices.
	//len: the number of triangles. Or if in is 0, the length of inout/n. 
	//n: if in is 0 then use n if you need a multiple other than 3. 
	//
	//Returns true if there are no problems with the input parameters*/
	bool softanimvertices(const mdl::image_t&, int primch, mdl::triangle_t *in, int len=1, int2 *inout=0, int n=3);
	
	/*2018: THIS ALGORITHM IS BRANCH HEAVY. BUT IT'S NOT A BAD REFERENCE.
	//(If this library is to play games, there should be an alternative.)
	*/
	/*/copyvertexbuffer: soft animate single or partial soft animation frame.
	//
	//dst=src+ch*steps: dst is required if src is 0 except in Nowrite Mode (described below)
	//
	//m: the capacity of src or if src is 0 the capacity of dst. 
	// Note: If dst is 0 it will be allocated with a capacity of m if necessary. 
	//ch: this is a pointer to one or more softanim_t (eg. initialized by softanimchannels)
	// If ch is 0 then a straight copy will occur. If src is 0 also then nothing will take place.
	//n: number of softanim_t pointed to by ch. For use with softanimchannels (with an s) above.
	//sign: step multiplier; +1 or -1. If 0 the sign is taken from ch->delta, otherwise ignored...
	//
	//UPDATE: Sword of Moonlight works a little differently than expected. ch->delta is ignored.
	//To animate a soft animation this way you need to have two buffers and manually interpolate 
	//between them. This is actually ideal for modern rendering hardware, and even that of SOM's 
	//day since you can just have the two frames interpolated (tweened) by the graphics hardware.
	//But that's not how it was implemented by Sword of Moonlight even though that is really how
	//The file format is best setup for. 
	//
	//IMPORTANT: ch is suited to animating a block of vertices that is the size of the sum of 
	//every vertex in the MDL file. It has a bitvector for each vertex in the file and per each
	//bit that is the vertex of the destination buffer that it will operate on if not out of range.
	//If everything is in order, duplicatations made by fillvertexbuffer will automatically reflect 
	//changes made to the originating vertices (you can use ch->nodup if this is undesirable)
	//
	//Nowrite mode: if dst/src/m are all 0 and ch is non zero ch will behave as if ch->nowrite was set.
	//
	//The capacity of dst is returned. It will be either m or 0 (if something goes wrong)*/
	int2 copyvertexbuffer(mdl::vbuffer_t &dst, mdl::const_vbuffer_t src, int2 m, mdl::softanim_t *ch=0, int n=1, int sign=0);
	
	/*/translate: soft animate 1 or more vertices (ideal for control points)
	//
	//n: serial mode. For use with softanimchannels (with an s) above.
	//ch: the soft animation channel.
	//m: when mverts is 0 this is the index of the vertex to animate. The index should be of softanimvertices above.
	// When mverts is nonzero, m is the number of indices pointed to by mverts.
	//vinout: when mverts is 0 this is 3 floats (xyz) to be translated. When mverts is nonzero it is 3*m floats per each vert.
	//mverts. This is a pointer to multiple vertex indices retrieved from softanimvertices above. See m: for more details.
	//
	// CAUTION: IF mverts IS NOT SORTED IT IS CONVERTED TO A SORTED MAP OF TWO 16BIT TUPLES.
	// TO SORT mverts IN ADVANCE SET ch TO 0. NEGATE m TO UNMAP/UNSORT mverts. THE MAP PUTS
	// THE INDEX INTO vinout IN THE HIGH 16BITS. IF THESE BITS AREN'T 0 IT'S ASSUMED MAPPED.
	//
	//scale: if 0 1:1 (mdl::mm) scale is used (millimeter scale is the native scale factor)
	// Note the default below is not 0. It is 1 meter scale (or mdl::m)
	//sign: step multiplier; +1 or -1. If 0 the sign is taken from ch->delta, otherwise ignored.
	//
	//Returns true if there are no problems with the input parameters*/
	int translate(mdl::softanim_t *ch, int2 m, float vinout[3], const float scale[4]=0, int2 *mverts=0, int n=1, int sign=0);

	/*/accumulate: soft animate vertices //2024
	//
	//stride: number of bytes to skip between vinout accesses
	//vinout_s: vinout must be less than vinout_s*stride int16_ts
	*/
	int accumulate(mdl::softanim_t *ch, int16_t *vinout, int vinout_s, int stride, int n=1, int sign=0);
	/*/accumulate: soft animate vertices //2024
	//
	//stride: number of bytes to skip between vinout accesses
	//vinout_s: vinout must be less than vinout_s*stride floats
	*/
	int accumulate(mdl::softanim_t *ch, float *vinout, int vinout_s, int stride, float step=1, int n=1, int sign=0);

	/*///PlayStation TIM Blocks //////////////////////*/

	/*maptotimblock: tim is the embedded TIM graphic*/	
	void maptotimblock(tim::image_t&, mdl::image_t&, int tim); 
			  
	inline const tim::image_t& /*const copy of out*/
	maptotimblock(tim::image_t &out, const mdl::image_t &img, int tim)
	{
		maptotimblock(out,const_cast<mdl::image_t&>(img),tim);
		out.mode = SWORDOFMOONLIGHT_READONLY; return out;
	}
}
namespace tim /*SWORDOFMOONLIGHT::*/
{
	/*A TIM image is a Sony PlayStation TIM file usually embedded
	//inside of the MDL format. Use mdl::timsubimageblock to map an image*/

	using namespace _memory_;
	/*maptofile: modes; 'r' for read, 'w' for read+write.
	//If file_t is 0 mode can alternatively be an open file descriptor.
	//If file_t is 0 and mode is negative, "-mode" will be mapped write-copy.
	//mask: bit 1 is clue, bit 2 is the pixel data*/
	void maptofile(tim::image_t&, tim::file_t, int mode='r', int mask=0x3,...);	
	/*always unmap after map (even if image is bad)*/
	inline int unmap(tim::image_t &img){ return swordofmoonlight_lib_unmap(&img); }; 
	/*these position the header same as maptofile*/
	void maptoram(tim::image_t&,tim::ram_t,size_t);
	void maptorom(tim::image_t&,tim::rom_t,size_t);

	typedef swordofmoonlight_tim_header_t header_t;
	 	
	tim::header_t &imageheader(tim::image_t&);

	inline const tim::header_t &imageheader(const tim::image_t &img)
	{
		return imageheader(const_cast<tim::image_t&>(img));
	}

	typedef swordofmoonlight_tim_pixmap_t pixmap_t;
	typedef swordofmoonlight_tim_pixrow_t pixrow_t;

	tim::pixmap_t &clut(tim::image_t&);
	tim::pixmap_t &data(tim::image_t&);

	inline const tim::pixmap_t &clut(const tim::image_t &img)
	{
		return clut(const_cast<tim::image_t&>(img));
	}
	inline const tim::pixmap_t &data(const tim::image_t &img)
	{
		return data(const_cast<tim::image_t&>(img));
	}
		
	typedef swordofmoonlight_tim_index_t index_t;
	typedef swordofmoonlight_tim_codex_t codex_t;

	typedef swordofmoonlight_tim_highcolor_t highcolor_t;
	typedef swordofmoonlight_tim_truecolor_t truecolor_t;

	enum{codexmode=0,indexmode,highcolormode,truecolormode,mixedmode};

	inline tim::pixrow_t &pixrow(tim::pixmap_t &pix, int row)
	{
		return &pix&&row>=0&&row<pix.h?pix[row]:*(tim::pixrow_t*)0;
	}
	inline const tim::pixrow_t &pixrow(const tim::pixmap_t &pix, int row)
	{
		return &pix&&row>=0&&row<pix.h?pix[row]:*(tim::pixrow_t*)0;
	}

	/*tpn: texture page number (0~31)
	//FYI: there is no reason to use this on a clut pixmap_t*/
	inline int tpn(const tim::pixmap_t &p)
	{
		return p?((p.dy/256+2)%2*16+(p.dx/64+16)%16):0;
	}
}
namespace txr /*SWORDOFMOONLIGHT::*/
{
	using namespace _memory_;
	/*maptofile: modes; 'r' for read, 'w' for read+write.
	//If file_t is 0 mode can alternatively be an open file descriptor.
	//If file_t is 0 and mode is negative, "-mode" will be mapped write-copy*/
	void maptofile(txr::image_t&, txr::file_t, int mode='r', int mipmask=0xFF,...);	
	/*always unmap after map (even if image is bad)*/
	inline int unmap(txr::image_t &img){ return swordofmoonlight_lib_unmap(&img); } 
	
	typedef swordofmoonlight_txr_header_t header_t;
	 	
	txr::header_t &imageheader(txr::image_t&);

	inline const txr::header_t &imageheader(const txr::image_t &img)
	{
		return imageheader(const_cast<txr::image_t&>(img));
	}

	typedef swordofmoonlight_txr_palette_t palette_t;

	/*256 of these (if return is nonzero)*/
	txr::palette_t *palette(txr::image_t&);

	inline const txr::palette_t *palette(const txr::image_t &img)
	{
		return palette(const_cast<txr::image_t&>(img));
	}

	typedef swordofmoonlight_txr_mipmap_t mipmap_t;
	typedef swordofmoonlight_txr_pixrow_t pixrow_t;

	typedef swordofmoonlight_txr_index_t index_t;
	typedef swordofmoonlight_txr_truecolor_t truecolor_t;

	/*level: 0>1 and so on*/	
	const txr::mipmap_t mipmap(const txr::image_t&, int level);

	inline txr::mipmap_t mipmap(txr::image_t &img, int level) 
	{
		txr::mipmap_t out = mipmap(const_cast<const txr::image_t&>(img),level);

		if(!img.readonly()) out.firstrow = 
		const_cast<txr::pixrow_t*>(out.readonly); return out;
	}							

	inline txr::pixrow_t &pixrow(txr::mipmap_t &mip, int row)
	{
		return &mip&&row>=0&&row<mip.height?mip[row]:*(txr::pixrow_t*)0;
	}
	inline const txr::pixrow_t &pixrow(const txr::mipmap_t &mip, int row)
	{
		return &mip&&row>=0&&row<mip.height?mip[row]:*(txr::pixrow_t*)0;
	}
}
namespace prm /*SWORDOFMOONLIGHT::*/
{
	typedef swordofmoonlight_prm_item_t item_t;
	typedef swordofmoonlight_prm_magic_t magic_t;
	typedef swordofmoonlight_prm_object_t object_t;
	typedef swordofmoonlight_prm_enemy_t enemy_t;
	typedef swordofmoonlight_prm_npc_t npc_t;
}
namespace prt
{
	typedef swordofmoonlight_prt_part_t part_t;
}
namespace prf /*SWORDOFMOONLIGHT::*/
{		
	using namespace _memory_;
	typedef void file_t,maptorom;		
	typedef swordofmoonlight_prf_magic_t magic_t;
	typedef swordofmoonlight_prf_item_t item_t;		
	typedef swordofmoonlight_prf_item2_t item2_t;
	typedef swordofmoonlight_prf_move_t move_t;
	typedef swordofmoonlight_prf_moveset_t moveset_t;
	typedef swordofmoonlight_prf_object_t object_t;
	typedef swordofmoonlight_prf_data_t data_t;
	typedef swordofmoonlight_prf_enemy_t enemy_t;
	typedef swordofmoonlight_prf_npc_t npc_t;
	/*UNIMPLEMENTED
	//this sets prf::image_t::mask equal to the PRF file
	//format, or 0 if it doesn't appear to be one of the
	//following formats:
	//prf::magic_t::mask (old fashioned magic spell)
	//prf::item_t::mask (equal to prf::item2_t::mask)
	//prf::item2_t::mask (non equipment/movement item)
	//prf::move_t::mask (old fashioned weapon/movement)
	//prf::moveset_t::mask (new weapon or equipped item)
	//prf::object_t::mask (standard DATA/OBJ/PROF format)
	//prf::enemy_t::mask (standard DATA/ENEMY/PROF format)
	//prf::npc_t::mask (standard DATA/NPC/PROF format)*/
	void maptoram(prf::image_t&,prf::ram_t,size_t bytes);	

	typedef cint8_t name_t[30+1];	
	typedef cint8_t history_t[96+1];

	inline prf::name_t &name(prf::image_t &img)
	{
		if(0!=img.mask)
		return img.header<prf::name_t>()[0];
		return img.badref<prf::name_t>();
	}

	inline prf::name_t &model(prf::image_t &img)
	{
		if(img.mask>prf::magic_t::mask)
		return img.header<prf::name_t>()[1];
		return img.badref<prf::name_t>();
	}

	/*EXTENSION DOCUMENTATION
	//this is an optional 96B note card on the end.
	//it's the same size as PRT files have, but do
	//not appear to use. the author/source are the
	//most important bit of information. the final
	//byte should be 0, but is reserved for future
	//standards when it's nonzero--1 for example*/
	inline prf::history_t &history(prf::image_t &img)
	{
		return *(history_t*)(img.set==img.end?0:img.set);
	}

	inline prf::magic_t &magic(prf::image_t &img)
	{
		if(img.mask==prf::magic_t::mask)
		return *(prf::magic_t*)(img.set-40/4);
		return img.badref<prf::magic_t>();
	}

	inline prf::item_t &item(prf::image_t &img)
	{
		if(img.mask>=prf::item_t::mask
		&&img.mask<=prf::moveset_t::mask)
		return *(prf::item_t*)(img.set-88/4);
		return img.badref<prf::item_t>();
	}
	inline prf::item2_t &item2(prf::item_t &i)
	{
		assert(i&&i.equip==0);
		return *(prf::item2_t*)(((cint8_t*)&i)+10);
	}
	inline prf::move_t &move(prf::item_t &i)
	{
		assert(i&&i.equip==1);
		prf::moveset_t *p = (prf::moveset_t*)(((cint8_t*)&i)+10);
		assert(0==p->SND_pitch[0]&&0==p->SND_pitch[1]);
		return *(prf::move_t*)p;
	}
	inline prf::moveset_t &moveset(prf::item_t &i)	
	{
		assert(i&&i.equip>0);
		prf::moveset_t *p = (prf::moveset_t*)(((cint8_t*)&i)+10);
		assert(i.equip>1||0!=p->SND_pitch[0]);
		return *p;
	}

	inline prf::object_t &object(prf::image_t &img)
	{
		if(img.mask==prf::object_t::mask)
		return *(prf::object_t*)(img.set-108/4);
		return img.badref<prf::object_t>();
	}	

	inline prf::enemy_t &enemy(prf::image_t &img)
	{
		if(img.mask==prf::enemy_t::mask)
		return *(prf::enemy_t*)&img.header<prf::name_t>()[2];
		return img.badref<prf::enemy_t>();
	}	
	inline prf::npc_t &npc(prf::image_t &img)
	{
		if(img.mask==prf::npc_t::mask)
		return *(prf::npc_t*)&img.header<prf::name_t>()[2];
		return img.badref<prf::npc_t>();
	}	
}
namespace mpx /*SWORDOFMOONLIGHT::*/
{
	using namespace _memory_;
	/*WARNING: there's no validation so maptofile is symmetric 
	//with maptoram/maptorom. use "base" to validate*/
	void maptofile(mpx::image_t&, mpx::file_t, int mode='r');	
	/*always unmap after map (even if image is bad)*/
	inline int unmap(mpx::image_t &img){ return swordofmoonlight_lib_unmap(&img); } 	

	typedef swordofmoonlight_mpx_base_t base_t; 
	typedef swordofmoonlight_mpx_objects_t objects_t;
	typedef swordofmoonlight_mpx_enemies_t enemies_t;
	typedef swordofmoonlight_mpx_npcs_t npcs_t;
	typedef swordofmoonlight_mpx_items_t items_t;
	typedef swordofmoonlight_mpx_tiles_t tiles_t;
	typedef swordofmoonlight_mpx_bspdata_t bspdata_t;
	typedef swordofmoonlight_mpx_textures_t textures_t;
	typedef swordofmoonlight_mpx_vertices_t vertices_t;

	typedef objects_t::_item object_t;
	typedef enemies_t::_item enemy_t;
	typedef npcs_t::_item npc_t;
	typedef items_t::_item item_t;
	typedef tiles_t::_item tile_t;

	/*This breaks down the simple part of the file
	//the more complex sections aren't implemented*/
	struct body_t
	{
		SWORDOFMOONLIGHT_RETURN_BY_REFERENCE2(body_t)

		objects_t &objects;
		npcs_t &enemies,&npcs;
		items_t &items;
		tiles_t &tiles;		

		/*WARNING: b must be valid to form references
		//base_cast does not validate, base validates.
		*/
		body_t(mpx::base_t &b);
	};

	/*This base_t can be used with body_t if nonzero*/
	mpx::base_t &base(mpx::image_t &img);

	/*WARNING: This doesn't ensure anything is valid*/
	inline mpx::base_t &base_cast(mpx::image_t &img)
	{
		return *(mpx::base_t*)img.set;
	}

	/*Unlike body_t each member of this object must
	//be tested via the bool like operator, however
	//all but bspdata will be nonzero unless masked*/
	struct data_t
	{
		/*WARNING: this memory is UNALIGNED so it shouldn't 
		//be accessed directly (especially the vertices.)*/

		mpx::bspdata_t &bspdata; /*OPTIONAL*/
				
		mpx::textures_t &textures;
		mpx::vertices_t &vertices; /*UNALIGNED (WARNING)*/

		/*MSM and MHM follow (unimplemented)*/
	};

	/*Mask bits apply to each member in the data_t object.
	//
	// WARNING: like the other functions this does bounds
	// tests on img so is not recommended for performance.
	*/
	mpx::data_t data(mpx::image_t &img, mpx::body_t&, int mask=0x1f);
}
namespace evt /*SWORDOFMOONLIGHT::*/
{
	typedef swordofmoonlight_evt_header_t header_t;

	typedef evt::header_t::_item event_t;

	/*THIS LIST IS INCOMPLETE*/
	typedef swordofmoonlight_evt_code_t code_t;
	typedef swordofmoonlight_evt_code00_t code00_t; /*text*/
	typedef swordofmoonlight_evt_code01_t code01_t; /*text+font*/
	typedef swordofmoonlight_evt_code17_t code17_t; /*shop*/
	typedef swordofmoonlight_evt_code19_t code19_t; /*warp npc*/
	typedef swordofmoonlight_evt_code1a_t code1a_t; /*warp enemy*/
	typedef swordofmoonlight_evt_code3b_t code3b_t;	/*standby map EXTENSION*/
	typedef swordofmoonlight_evt_code3c_t code3c_t;	/*map*/
	typedef swordofmoonlight_evt_code3d_t code3d_t;	/*warp*/
	typedef swordofmoonlight_evt_code50_t code50_t; /*status*/
	typedef swordofmoonlight_evt_code54_t code54_t; /*status->counter*/
	typedef swordofmoonlight_evt_code64_t code64_t; /*enage object*/
	typedef swordofmoonlight_evt_code66_t code66_t; /*move object*/
	typedef swordofmoonlight_evt_code78_t code78_t; /*system*/
	typedef swordofmoonlight_evt_code8D_t code8D_t; /*text->branch*/
	typedef swordofmoonlight_evt_code90_t code90_t; /*counter*/
	typedef swordofmoonlight_evt_codeFF_t codeFF_t;

	using namespace _memory_;
	/*maptofile: modes; 'r' for read, 'w' for read+write.
	//If file_t is 0 mode can alternatively be an open file descriptor.
	//If file_t is 0 and mode is negative, "-mode" will be mapped write-copy.
	//mask: bit 1 is header-only, bit 2 is the rest of the file*/
	void maptofile(evt::image_t&, evt::file_t, int mode='r', int mask=0x3);	
	/*always unmap after map (even if image is bad)*/
	inline int unmap(evt::image_t &img){ return swordofmoonlight_lib_unmap(&img); } 	
	/*these position the header same as maptofile*/
	void maptoram(evt::image_t&,evt::ram_t,size_t);
	void maptorom(evt::image_t&,evt::rom_t,size_t);

	inline evt::header_t &imageheader(evt::image_t &img)
	{ 
		return *img.header<evt::header_t>(); 
	}
	inline const evt::header_t &imageheader(const evt::image_t &img)
	{ 
		return *img.header<const evt::header_t>(); 
	}
}
namespace som /*SWORDOFMOONLIGHT::*/
{		
	typedef swordofmoonlight_lib_file_t file_t;  
	typedef swordofmoonlight_lib_char_t char_t;  
	typedef swordofmoonlight_lib_text_t text_t;  
		
	inline void _setenv(som::text_t key, som::text_t val)
	{
		#ifdef _WIN32 
		_wputenv_s(key,val);
		#else
		::setenv(key,val,1);
		#endif
	}
	inline som::text_t _getenv(som::text_t key)
	{
		#ifdef _WIN32 
		#pragma warning(suppress:4996)
		return _wgetenv(key);
		#else
		return ::getenv(key);
		#endif
	}
	SWORDOFMOONLIGHT_USER
	/*_INCOMPATIBLE WITH SetEnvironmentVariableW_*/
	static void environ_f(som::text_t kv[2], void*)
	{
		if(kv[1]) som::_setenv(kv[0],kv[1]); else kv[1] = som::_getenv(kv[0]);
	}		   
			
	#pragma push_macro("environ") //stdlib
	#undef environ

	typedef void (*environ_t)(som::text_t[2],void*);
	/*readfile: open (then close) text file for reading
	//false is returned if a randomly chosen SOM file in CD
	//does not match GAME and DISC where one or both are defined
	//Reminder: SOM files are INI files. Consider ini::readfile/open*/
	bool readfile(som::file_t, som::environ_t environ=environ_f, void *env1=0);	

	/*////courtesy///////////////////////////////////////////////////////////////
	//																		   
	// http://en.swordofmoonlight.org/wiki/SOM_file/list_of_environment_variables
	*/	
	#define SETENV(x) { som::text_t kv[2] = {var,var+x}; environ(kv,env); }	
	#define GETENV(x) { som::text_t kv[2] = {var,0}; environ(kv,env); x = kv[1]; }	
	
	inline som::text_t readerror(som::environ_t environ=environ_f, void *env=0)
	{
		som::text_t val; som::char_t var[32+260]; /*260: MAX_PATH*/

		SWORDOFMOONLIGHT_SPRINTF(var,"ERROR"); GETENV(val) return val;
	}	
	static void clean(som::file_t instdir, som::environ_t environ=environ_f, void *env=0)
	{
		som::char_t var[32+260]; /*260: MAX_PATH*/
					
		#ifdef _WIN32
		char sep = '\\';
		#else
		char sep = '/';
		#endif
		SWORDOFMOONLIGHT_SPRINTF(var,"INSTALL%c%s",0,instdir); /*2024*/
		SETENV(8)
		SETENV(SWORDOFMOONLIGHT_SPRINTF(var,"GAME"))
		SETENV(SWORDOFMOONLIGHT_SPRINTF(var,"DISC"))
		SETENV(SWORDOFMOONLIGHT_SPRINTF(var,"SOM"))
		SETENV(SWORDOFMOONLIGHT_SPRINTF(var,"LOAD")) /*2020*/
		SETENV(SWORDOFMOONLIGHT_SPRINTF(var,"TITLE"))		
		SETENV(SWORDOFMOONLIGHT_SPRINTF(var,"PLAYER"))
		SETENV(SWORDOFMOONLIGHT_SPRINTF(var,"USER"))
		/*tool: intended to get SOM_MAIN into the tool folder*/
		SWORDOFMOONLIGHT_SPRINTF(var,"CD%c%s%ctool",0,instdir,sep);
		SETENV(3)
		SWORDOFMOONLIGHT_SPRINTF(var,"DATA%c%s%cdata",0,instdir,sep);
		SETENV(5)
		SETENV(SWORDOFMOONLIGHT_SPRINTF(var,"DATASET"))
		SETENV(SWORDOFMOONLIGHT_SPRINTF(var,"SCRIPT"))
		SWORDOFMOONLIGHT_SPRINTF(var,"FONT%c%s%cfont",0,instdir,sep);
		SETENV(5)		
		SETENV(SWORDOFMOONLIGHT_SPRINTF(var,"INI"))
		SETENV(SWORDOFMOONLIGHT_SPRINTF(var,"TRIAL"))
		SWORDOFMOONLIGHT_SPRINTF(var,"EX%c%s%ctool%cSOM_EX.ini",0,instdir,sep,sep);
		SETENV(3)
		SETENV(SWORDOFMOONLIGHT_SPRINTF(var,"SETUP"))
		SETENV(SWORDOFMOONLIGHT_SPRINTF(var,"ICON"))		
		SWORDOFMOONLIGHT_SPRINTF(var,"TEXT%c%s%ctext",0,instdir,sep);
		SETENV(5)
		SWORDOFMOONLIGHT_SPRINTF(var,"ART%c%s%cart",0,instdir,sep);
		SETENV(4)
	}
	static void cd(som::file_t som, som::environ_t environ=environ_f, void *env=0)
	{
		int cat; som::char_t var[32+260]; /*260: MAX_PATH*/
		cat = SWORDOFMOONLIGHT_SPRINTF(var,"SOM%c%s",0,som);  		
		SETENV(4)
		if(*som) /*hack: trying to keep CD in the tool folder*/
		{
			cat = SWORDOFMOONLIGHT_SPRINTF(var,"CD%c%s",0,som);  
			while(cat!=3&&var[cat]!='/'&&var[cat]!='\\') cat--;
			var[cat] = '\0'; SETENV(3)
		}			
		som::text_t ini, game, title;
		SWORDOFMOONLIGHT_SPRINTF(var,"INI"); GETENV(ini)
		SWORDOFMOONLIGHT_SPRINTF(var,"GAME"); GETENV(game)
		SWORDOFMOONLIGHT_SPRINTF(var,"TITLE"); GETENV(title)
		if(!title||!*title) /*strip path and dotted extension*/
		{	
			for(cat=0;som[cat];cat++); 			
			while(cat--&&som[cat]!='/'&&som[cat]!='\\'); 
			cat = SWORDOFMOONLIGHT_SPRINTF(var,"TITLE%c%s",0,som+cat+1); 
			while(cat>6&&var[cat]!='.') cat--; if(cat>6) var[cat] = '\0';
			SETENV(6) GETENV(title)
		}
		if(!ini||!*ini) /*default to %TITLE%.ini*/
		{
			SWORDOFMOONLIGHT_SPRINTF(var,"INI%c%s.ini",0,title);  
			SETENV(4)
		}
		if(!game||!*game) /*default to %TITLE%*/
		{
			SWORDOFMOONLIGHT_SPRINTF(var,"GAME%c%s",0,title);  
			SETENV(5)
		}
	}	
	#undef SETENV
	#undef GETENV

	#pragma pop_macro("environ") //stdlib
}
namespace ini /*SWORDOFMOONLIGHT::*/
{		
	typedef swordofmoonlight_lib_file_t file_t; 
	typedef swordofmoonlight_lib_char_t char_t;
	typedef swordofmoonlight_lib_text_t text_t;  
	
	/*http://en.swordofmoonlight.org/wiki/INI_file*/
	typedef void (*environ_t)(ini::text_t[2],void*);
	/*readfile is a more complicated version of som::readfile
	//With it you can read an INI file without ever storing anything 
	//Its environ_t pattern is more complex, and must be caller implemented 
	//mask: corresponds to som::open below (readfile is completely self contained)
	//environ_t: _see below WARNING about embedded margins_
	//0 is set, 1 is 0: set 1 to the environment variable value of 0
	//0 is set, 1 is set: this is a property. 0 is its name, 1 is its value
	//0 is 0, 1 is set: margin corresponding to the following property/section head
	//0 is [x], 1 is 0: section head; if 1 is not set to 0 the entire section gets skipped!
	//0 is 0, 1 is 0: end-of-file mark (it is preceded by its margin)
	//Nice: the line ending variable is named \n. Its value is \r\n*/
	void readfile(ini::file_t, int mask, ini::environ_t, void *env=0);	
		
	/*///// HOW THIS WORKS //////////////////////////////////////////
	//
	// There is no support for named lookup because there may be more
	// than one instance of the section/property per section. Neither
	// are sections concatenated, so not to alter the author's intent
	// Spaces around = and blank lines are discarded. Margins include
	// anything and everything that is not a property or section head
	//
	// WARNING: lines beginning with semicolons are always understood
	// to be margins, even when embedded within properties which span 
	// multiple lines of the text file. If reading margins such lines 
	// are retained, within the property itself! And if not, then the 
	// lines will be stricken from the property! Proceed with caution
	*/
	typedef swordofmoonlight_lib_opaque_t sections_t; 
	/*open/count: convert INI file into an enumerable object
	//mask: bit 1 is properties, bit 2 is margins, bit 3 filters out
	//whole sections by passing bracketed names to environ_t (see readfile)
	//environ_t: expands %VAR% where encountered. void* is passed back to environ_t
	//size_t: returns the number of sections per mask(3)/environ_t; same goes for count*/
	size_t open(ini::sections_t&, ini::file_t, int mask=0x1, ini::environ_t=som::environ_f, void *env=0);		
	size_t count(const ini::sections_t&); 	
	/*error: there's no such thing as an invalid INI file
	//however implementation constraints might result in truncation
	//bool: true if count is 0 or aborted before reaching the end of the file*/
	bool error(const ini::sections_t&);
	/*ATTENTION! _subject to change_
	//the current implementation does not keep the file open*/	
	void close(ini::sections_t&); /*always close after open*/
	
	/*section: access an individual section
	//text_t+1: returns POSIX exec e suffix envp
	//text_t[0]: returns the section's name bracketed
	//A property's value begins after the first = separator*/
	const ini::text_t *section(const ini::sections_t&, size_t);	  
	/*text_t works the same here as above. The margins are unaltered*/
	const ini::text_t *sectionmargins(const ini::sections_t&, size_t); 
}
namespace zip /*SWORDOFMOONLIGHT::*/
{
	/*For DEFLATEd theme/language packs*/

	typedef swordofmoonlight_lib_file_t file_t;
	typedef swordofmoonlight_lib_image_t image_t;
	typedef swordofmoonlight_lib_opaque_t mapper_t;

	/*INTERNAL FILE SYSTEM PATHS ARE CASE-SENSITIVE!*/	
	
	/*open/count: returns mask'ed central directory entries count
	//mask: can be an internal directory, yielding a partial index
	//or an internal file system path, yielding a single file index*/
	size_t open(zip::mapper_t&, zip::file_t, zip::file_t mask=0);	
	size_t count(const zip::mapper_t&); 
	/*always close after open*/
	void close(zip::mapper_t&);	

	/*directory: does directory file_t exist?*/
	bool directory(const zip::mapper_t&, zip::file_t);

	typedef swordofmoonlight_zip_entry_t entry_t;
	typedef swordofmoonlight_zip_entries_t entries_t;

	//*entries: there are count in total
	//they are sorted in 8-bit binary order
	//all IBM code page 437 first, then UTF-8*/
	zip::entries_t entries(const zip::mapper_t&);
	/*this will be equal to count if there are none*/
	size_t firstunicodeentry(const zip::mapper_t&);

	/*find: returns 0 if exact match is not found*/
	zip::entry_t &find(const zip::mapper_t&, zip::file_t);
		 
	typedef swordofmoonlight_zip_header_t header_t;
	typedef swordofmoonlight_zip_inflate_t inflate_t;

	/*maptolocalentry: file_t is a file system path to the image	
	//The image will be marked bad if it doesn't match its entry
	//NOTE: image_t member end is misaligned WRT its member set!*/	
	void maptolocalentry(zip::image_t&, const zip::mapper_t&, zip::entry_t&);	
	/*always unmap after maptolocalentry (even if image is bad)*/
	inline int unmap(zip::image_t &img){ return swordofmoonlight_lib_unmap(&img); } 	
	/*this is the original implementation of maptolocalentry*/
	inline void maptolocalentry(zip::image_t &img, const zip::mapper_t &map, zip::file_t mask)
	{
		zip::maptolocalentry(img,map,zip::find(map,mask));
	}
		
	inline zip::header_t &imageheader(const zip::image_t &img)
	{ 
		return *img.header<zip::header_t>(); 
	}

	/*interchangeable for an inflate_t*/
	template<class T> struct inflatable : public T
	{
		inflatable<T>():inflate_restart(0)
		{
			int compile[sizeof(*this)==sizeof(inflate_t)];
		}
		uint8_t inflate_dictionary[32*1024-sizeof(T)];
		size_t inflate_restart;			

		inflate_t::private_implementation inflate_private;

		inline operator inflate_t&(){ return *(inflate_t*)this; }
	};			
	/*inflate: returns decompressed bytes up to size_t bytes
	//0 is returned if an error occurs. Does algorithms 0 and 8
	//if size_t is 0 the sizeof the inflate dictionary is assumed*/		
	size_t inflate(const zip::image_t&, zip::inflate_t&, size_t=0);
}
namespace res /*SWORDOFMOONLIGHT::*/
{	
	typedef const ule16_t *unicode_t;

	typedef swordofmoonlight_lib_file_t file_t;
	typedef swordofmoonlight_lib_image_t image_t;
	typedef swordofmoonlight_lib_opaque_t mapper_t;

	enum /*winuser.h*/ 
	{	
		cursor=1<<1, 
		bitmap=1<<2, icon=1<<3,
		dialog=1<<5, menu=1<<4,
		string=1<<6, font=1<<8, 
		hotkey=1<<9, blob=1<<10,
		fontdir=1<<7, html=1<<23,
		message=1<<11, icons=1<<13,
		cursors=1<<12, aniicon=1<<22, 
		include=1<<17, anicursor=1<<21,
		version=1<<16, vxd=1<<20,	 
		plugplay=1<<19,
		manifest=1<<24,	others=1	
	};
	using namespace _memory_;
	/*open/count: returns the number of resources
	//mask: each bit corresponds to the enum above*/
	size_t open(res::mapper_t&, res::file_t, int mask=~0);	
	size_t count(const res::mapper_t&); 
	/*always close after open*/
	void close(res::mapper_t&);	

	typedef swordofmoonlight_res_resource_t resource_t;
	typedef swordofmoonlight_res_resources_t resources_t;
	
	/*resources: there are count in total*/
	res::resources_t resources(const res::mapper_t&);	

	/*find/end: returns 0 if nothing is found*/
	res::resources_t find(const res::mapper_t&, const resource_t&);
	/*use end when multiple matches can exist*/
	inline res::resources_t end(const res::mapper_t &map)
	{
		return res::resources(map)+res::count(map);
	}	
	
	/*RESOURCEHEADER*/
	inline size_t data(const res::resource_t &r)
	{
		return r.size[-2];
	}	
	inline const void *database(const res::resource_t &r)
	{
		/*ensures header is aligned on 32-bit boundary*/
		return r.size-2+r.size[-1]/4+(r.size[-1]%4?1:0);
	}
	typedef swordofmoonlight_res_fullspec_t fullspec_t;
	inline res::fullspec_t &fullspec(const res::resource_t &r)
	{
		return *(res::fullspec_t*)(r.lcid-3);
	}	

	/*type: convert to res::dialog etc*/
	inline int type(const res::resource_t &r)
	{
		if(r.type[0]==0xFFFF) 
		{
			if(r.type[1]<32) return 1<<r.type[1];
		}
		return res::others;
	}
}
namespace mo /*SWORDOFMOONLIGHT::*/
{
	typedef const cint8_t *unicode_t;
	
	/*FOR purposes of string comparison:
	//THIS implementation treats \r, \n, and \r\n equivalently.
	//THIS is not intended to be a solution to exporting/saving line-endings accordingly*/

	using namespace _memory_;
	/*For gettext-based script files*/	
	/*http://punt.sourceforge.net/new_svn/gettext/gettext_8.html#SEC155*/
	/*http://www.gnu.org/savannah-checkouts/gnu/gettext/manual/html_node/Plural-forms.html*/
	void maptofile(mo::image_t&, mo::file_t mo);
	/*always unmap after map (even if image is bad)*/
	inline int unmap(mo::image_t &img){ return swordofmoonlight_lib_unmap(&img); } 
		
	typedef swordofmoonlight_mo_header_t header_t;

	inline mo::header_t &imageheader(mo::image_t &img)
	{ 
		return *img.header<mo::header_t>(); 
	}
	inline const mo::header_t &imageheader(const mo::image_t &img)
	{
		return *img.header<const mo::header_t>();
	}
	inline mo::unicode_t catalog(const mo::image_t &img)
	{
		return img.header<cint8_t>(); 
	}

	/*get the "" gettext header-entry message*/
	int metadata(const mo::image_t&, mo::unicode_t &msgstr);
		
	/*nplurals: we'll parse the header for you
	//plural: you'll have to compute plural yourself*/
	/*reminder: add Unicode logic symbols to Ex.number.cpp*/
	int nplurals(const mo::image_t&); const char *plural(const mo::image_t&);

	/*_this is not set in stone_*/
	/*gettext: int returns the length of msgstr
	//int includes 0 terminated multi-string plural forms
	//int also includes the final 0 terminator itself if nonzero*/
	/*notice: if msgid is "" then 0 is returned. Use metadata to access ""*/
	int gettext(const mo::image_t&, mo::unicode_t msgid, mo::unicode_t &msgstr);

	inline mo::unicode_t metadata(const mo::image_t &img)
	{
		mo::unicode_t out = ""; mo::metadata(img,out); return out;
	}
	inline mo::unicode_t gettext(const mo::image_t &img, mo::unicode_t msgid)
	{
		mo::unicode_t out = msgid; mo::gettext(img,msgid,out); return out;
	}	
	
	/*is the header entry string required or not?*/
	inline size_t nmetadata(const mo::image_t &img)
	{
		mo::unicode_t _ = 0; metadata(img,_); return _!=0;
	}
		
	/*lowerbound: ctxt can be any prefix clen characters long
	//note: msgctxt like prefixes are separated/end with a \4
	//size_t is -1 or 0 respectively if image_t is misordered*/
	size_t lowerbound(const mo::image_t&, mo::unicode_t ctxt, int clen);
	size_t upperbound(const mo::image_t&, mo::unicode_t ctxt, int clen);

	typedef swordofmoonlight_mo_range_t range_t;

	/*range: initialize range_t to the interval [lb,ub)
	//the returned size_t is equal to range_t's size/n member
	//as a convenience the range will exclude the metadata msgid
	//if 0 is returned range_t's members other than n are undefined*/
	size_t range(const mo::image_t&, range_t&, size_t lb=0, size_t ub=-1);
	/*nrange: range_t here includes the entire range (including metadata)*/
	size_t nrange(const mo::image_t&, range_t&, size_t lb=0, size_t ub=-1);
	inline size_t range(const mo::image_t &img, range_t &r, mo::unicode_t ctxt, int clen)
	{
		size_t out = mo::range(img,r,mo::lowerbound(img,ctxt,clen),mo::upperbound(img,ctxt,clen));
		if(out) r.ctxtlog+=clen; return out;
	}
	template<class T> /*case where lb is the 0 literal*/
	inline size_t range(const mo::image_t &img, range_t &r, int lb, T ub=-1)
	{
		return range(img,r,(size_t)lb,(size_t)ub);
	}
	inline size_t entry(const mo::image_t &img, range_t &r, size_t lb)
	{
		return nrange(img,r,lb,lb+1);
	}
	inline size_t move(mo::image_t &img, size_t lb, mo::unicode_t id, mo::unicode_t str)
	{
		mo::range_t e; if(mo::entry(img,e,lb))
		{const_cast<ule32_t&>(e.idof[0].coff) = id-e.catalog; 
		const_cast<ule32_t&>(e.strof[0].coff) = str-e.catalog; 
		}return e.n;		
	}
	/*find: returns r.n if id is not found within r*/
	size_t find(const mo::range_t &r, mo::unicode_t id);

	/*validate: test a range_t against its image_t
	//tests that the strings are contained with the image
	//and that the msgids are 0 terminated where they should be	
	//note: see validate00 if 0 terminated msgstrs is what you desire*/
	inline bool validate(const mo::image_t &img, const range_t &r)
	{
		if(!r.n||!img) return !img.bad;
		size_t n = r.n, i = n-1, j = 0, k; 
		do if(j<(k=r.idof[i].coff+r.idof[i].clen)) j = k;
		while(i--); if(img<r.catalog+j) return false;
		for(i=0;i<n;i++) if(r.catalog[r.idof[i].coff+r.idof[i].clen]) 
		return img.test(false); 
		i = n-1; /*iterating over sequential memory*/
		do if(j<(k=r.strof[i].coff+r.strof[i].clen)) j = k;
		while(i--); if(img<r.catalog+j) return false;		
		return !img.bad;
	}
	/*not good: will cause the whole file to be pulled into memory*/
	inline bool validate00(const mo::image_t &img, const range_t &r)
	{
		if(mo::validate(img,r))
		for(size_t i=0,n=r.n;i<n;i++) 
		if(r.catalog[r.strof[i].coff+r.strof[i].clen]) /*not good*/
		return img.test(false);
		return !img.bad;
	}
	inline bool validate(const mo::image_t &img)
	{
		mo::range_t r; mo::nrange(img,r); return validate(img,r);
	}
	inline bool validate00(const mo::image_t &img)
	{
		mo::range_t r; mo::nrange(img,r); return validate00(img,r);
	}

	/*careful: these return raw untested pointers*/
	inline mo::unicode_t msgid(const mo::range_t &r, size_t i=0)
	{
		return r.ctxtlog+r.idof[i].coff;
	}
	inline mo::unicode_t msgctxt(const mo::range_t &r, size_t i=0)
	{
		return r.catalog+r.idof[i].coff;
	}
	inline mo::unicode_t msgstr(const mo::range_t &r, size_t i=0)
	{
		return r.catalog+r.strof[i].coff;
	}	
}
}/*namespace SWORDOFMOONLIGHT*/

#endif /*__cplusplus*/

#endif /*SWORDOFMOONLIGHT_INCLUDED*/
