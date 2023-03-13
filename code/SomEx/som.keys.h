	 
#ifndef SOM_KEYS_INCLUDED
#define SOM_KEYS_INCLUDED

//TODO: DISABLE SOMEHOW???

		/*2021

		I've been meaning to remove this for a very long time
		since at this stage it should by file names

		Moving dx_d3d9c_mipmap/colorkey into updating_texture
		makes it no longer work since it can't rely on the 
		mipmap being accurate... I had a bad afternoon because
		som_hacks_shadowfog was being applied to multiple
		textures after resetting the device and I started
		to set the texture colors to get to the bottom of it
		and this made it worse since many textures were being
		matched to the shadow's "key" and get the "shadow" 
		effect, which turned white textures invisible/black, I
		was really confused since I didn't remember the effect

		Anyway, I'm starting the process of pulling this out
		but I'm not ready to delete it since I may bring it 
		back in some form

		REMINDER: som_main_CopyFileA had copied IMAGES.INI

		*/
		#error REMOVED FROM BUILD

struct IDirect3DTexture9;

namespace SOM
{		
	namespace KEY 
	{		
		class Image;
		class Model;
		class Index; //opaque

		struct FogFVF; //D3DFVF_XYZ
		struct UnlitFVF; //D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1 
		struct BlendedFVF; //D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1

		//image classification routines
		extern const SOM::KEY::Image *image(::IDirect3DTexture9*,SOM::KEY::Index**_=0);

		//model classification routines
		extern const SOM::KEY::Model *model(const SOM::KEY::UnlitFVF*,DWORD,const WORD*,DWORD,SOM::KEY::Index**_=0);
		extern const SOM::KEY::Model *model(const SOM::KEY::BlendedFVF*,DWORD,const WORD*,DWORD,SOM::KEY::Index**_=0);

		//calculate rotation independent center for a given map tile data set
		extern const SOM::KEY::FogFVF &center(const SOM::KEY::Model*,const SOM::KEY::UnlitFVF*,const WORD*);
	
		//compare a model on hand to a given data set (may be faster than classification)
		extern bool compare(const SOM::KEY::Model*,const SOM::KEY::UnlitFVF*,DWORD,const WORD*,DWORD);
		extern bool compare(const SOM::KEY::Model*,const SOM::KEY::BlendedFVF*,DWORD,const WORD*,DWORD);

		//append begin to end to the keygen audit folder if within MAX_PATH
		//genpath2 returns secondary location of the file whenever applicable		
		extern const wchar_t *genpath(const wchar_t *begin, const wchar_t *end);
		extern const wchar_t *genpath2(const wchar_t *begin, const wchar_t *end);		
	};

	static const SOM::KEY::Image &classify(::IDirect3DTexture9*);  
	static const SOM::KEY::Model &classify(DWORD,const void*,DWORD,const WORD*,DWORD,size_t);

	extern void initialize_som_keys_cpp();
}

namespace SOM
{ 
	namespace KEY
	{
		struct Struct //GetPrivateProfileStruct()
		{			
			static const int struct_compiler_assert = -1;

			DWORD structure; //sizeof struct
			DWORD bytecheck; //header offset
		};

		extern const wchar_t *newvalue;
		extern const wchar_t *regvalue;

		typedef struct 
		{
			const wchar_t *section; 
			const wchar_t *hint; 
			const wchar_t *comments;			
			const wchar_t *audit; 
			const wchar_t *parity; 
			const wchar_t *checksum; 			
			
			inline operator const wchar_t**()const
			{
				return (const wchar_t**)this;
			}

		private: 
		    
			const wchar_t *_garbage; 

		}class_t; //SOM::KEY::

		typedef struct 
		: 
		public SOM::KEY::class_t
		{
			const wchar_t *dxformat;
			const wchar_t *texelsuv;
			const wchar_t *addressu;
			const wchar_t *addressv;
			const wchar_t *lodbias;
			const wchar_t *samples;	  
			const wchar_t *correct;

		}image_t; //SOM::KEY::
				
		typedef struct 
		: 
		public SOM::KEY::class_t
		{
			const wchar_t *fvformat;
			const wchar_t *vertices;
			const wchar_t *polygons;  
			const wchar_t *center;
			const wchar_t *radius;
			const wchar_t *height;
			const wchar_t *margin;
			const wchar_t *instance;
			const wchar_t *composite;

		}model_t; //SOM::KEY::	
	}
}

namespace SOM{ 
namespace KEY{ 
template<typename t> class Class //SOM::KEY:: 
{
public:

	GUID audit;

	bool erroneous;
	bool ambiguous;

	const unsigned *hint;

#ifdef NDEBUG

	const unsigned *hash;

#else

	unsigned hash[1+255/8]; //SOM_KEYS_MAX_HASH

#endif

	unsigned parity, checksum;

	inline const t *settings()const
	{
		return _union?_union:unite();
	}
	inline operator const t*()const
	{
		return _union?_union:unite();
	}
	inline const t *operator->()const
	{
		return _union?_union:unite();
	}	
			
  //// internal use ///////////

	static HKEY key;
	static LONG err; //registry error

	static const wchar_t *ini; //ini file
	static const wchar_t *eof;
	static const wchar_t *bad; //bad file

	inline bool complete();

	inline const wchar_t *setting(int);  

	inline int select(int);

	inline bool filter(const wchar_t*,int,const wchar_t*);

	inline unsigned &flags();

	bool keys(HKEY&,HKEY[3]); //must be closed

	static inline const t *zero();
	static inline const t *copy(const t&);

	static inline const wchar_t *inicpy(const wchar_t*,size_t);

	typedef t t;

protected:

	const t *_public;
	const t *_private;
	const t *_project;
	const t *_union;

	static const t *_default;
	static const t *_initial;

	inline bool safe_to_delete(const t *p);

	inline explicit Class(const char *hex);
	inline explicit Class(const wchar_t *hex);
	inline explicit Class(const unsigned *hash);

	Class(const t *prj, const t *prv, const t *pub, size_t size);

	inline Class(){ /*do nothing*/ }

	~Class();

private:

	const t *unite()const
	{
		if(!_public&&!_private&&!_project)
		{
			static t _incomplete = {(const wchar_t*)1}; 
			
			if(_incomplete[0]) memset(&_incomplete,0x00,sizeof(t));

			return &_incomplete;
		}		

		if(!_private&&!_public)  return _union = _project;
		if(!_public&&!_project)  return _union = _private;
		if(!_private&&!_project) return _union = _public;		

		t *out = new t; 

		const wchar_t **p = *out;

		for(int i=0;i<sizeof(t)/sizeof(void*);i++)
		{
			if(_project&&(*_project)[i]) p[i] = (*_project)[i];
			else if(_public&&(*_public)[i]) p[i] = (*_public)[i];
			else if(_private&&(*_private)[i]) p[i] = (*_private)[i];			
			else p[i] = 0;
		}

		return _union = out;
	}
};}}

namespace SOM{ 
namespace KEY{ class Image //SOM::KEY::
: 
public SOM::KEY::Class<SOM::KEY::image_t>
{
public:

	DWORD dxformat;
	DWORD texelsuv[2];
	DWORD addressu;
	DWORD addressv;
	DWORD lodbias;
	DWORD samples;

	const wchar_t *correct;

  //// client's use ///////////

	mutable void *corrected;

	static void (*clientdtor)(Image*);

  //// internal use ///////////

	inline explicit Image(const char *hex):Class(hex){}	
	inline explicit Image(const wchar_t *hex):Class(hex){}
	inline explicit Image(const unsigned *hash):Class(hash){}

	//default/placement new ctor
	Image(const t *prj=0, const t *prv=0, const t *pub=0); //SOM::KEY::image_t

	inline Image(const t &prj, const t &prv, const t &pub)
	{
		new (this) Image(copy(prj),copy(prv),copy(pub));
	}

	~Image()
	{ 
		if(clientdtor) clientdtor(this); 
	
		delete[] correct; 
	}

	static int initialize(const wchar_t*); //.ini reverse enumeration

	static const char *enumerate(int); //registry value enumeration
};}}

namespace SOM{
namespace KEY{ class Model //SOM::KEY:: 
: 
public SOM::KEY::Class<SOM::KEY::model_t>
{
public:
	
	DWORD fvformat;
	DWORD vertices;
	DWORD polygons;	
		
	FLOAT center[3];
	FLOAT radius;   
	FLOAT height;   
	FLOAT margin[3];

	SHORT instance[4];

	bool newinstance; //debugging

	struct
	:
	public SOM::KEY::Struct
	{
		DWORD instances;
		DWORD allocated;		

		inline size_t footprint(int n)
		{
			return (BYTE*)(at+n)-(BYTE*)this;
		}

		struct
		{
			SHORT instance[4];

			FLOAT center[3];
			FLOAT radius;   
			FLOAT height;   
			FLOAT margin[3];

		}at[];

	}*composite;

  //// internal use ///////////

	inline explicit Model(const char *hex):Class(hex){} 
	inline explicit Model(const wchar_t *hex):Class(hex){}
	inline explicit Model(const unsigned *hash):Class(hash){}

	//default/placement new ctor
	Model(const t *prj=0, const t *prv=0, const t *pub=0); //SOM::KEY::model_t

	inline Model(const t &prj, const t &prv, const t &pub)
	{
		new (this) Model(copy(prj),copy(prv),copy(pub));
	}

	static int initialize(const wchar_t*); //.ini reverse enumeration

	static const char *enumerate(int); //registry value enumeration
};}}

namespace SOM{
static const SOM::KEY::Image &classify(::IDirect3DTexture9 *in)
{
	static SOM::KEY::Image classless;

	const SOM::KEY::Image *out = SOM::KEY::image(in);		

	return !out?classless:*out;
}}

namespace SOM{
static const SOM::KEY::Model &classify(DWORD fvf, const void *verts, DWORD m,
												  const WORD *prims, DWORD n, size_t sz)
{	static SOM::KEY::Model classless;

	if(!m||!n||!verts||!prims) return classless;

	const SOM::KEY::Model *out = 0;

	switch(fvf) 
	{
	case 0x2|0x40|0x100: if(sz) assert(sz==24); //D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1

		out = SOM::KEY::model((SOM::KEY::UnlitFVF*)verts,m,(WORD*)prims,n); break;

	case 0x2|0x10|0x100: if(sz) assert(sz==32); //D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1

		out = SOM::KEY::model((SOM::KEY::BlendedFVF*)verts,m,(WORD*)prims,n); break;

	default: assert(0); 
	}

	return !out?classless:*out;		
}
template<typename T> 
static const SOM::KEY::Model &classify(DWORD a, const T *b, DWORD c, const WORD *d, DWORD e)
{
	return SOM::classify(a,b,c,d,e,sizeof(T));
}
template<> //specialization
static const SOM::KEY::Model &classify(DWORD a, const void *b, DWORD c,const WORD *d, DWORD e)
{
	return SOM::classify(a,b,c,d,e,0); //use at own risk
}} 

#endif SOM_KEYS_INCLUDED