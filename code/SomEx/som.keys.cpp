
#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

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

//stdext::hash_map
#include <hash_map>

#include <d3d9.h>

#include "d3d10/DDS.H"

#include "Ex.ini.h"
#include "Ex.output.h"
#include "Ex.dataset.h"

//REMOVE ME?
#include "som.game.h"
#include "som.state.h"
#include "som.keys.h"

//this is functionally obsolete
static bool som_keys_debugging()
{
	static bool out = !wcsicmp(EX::exe(),L"SOM_DB");
	return out;
}

EX_DATA_DEFINE(const SOM::KEY::Image*)
EX_DATA_DEFINE(const SOM::KEY::Index*)

#define SOM_KEYS_MAX_NAME 255 
#define SOM_KEYS_MAX_HASH (1+SOM_KEYS_MAX_NAME/8) //8 hexadecimal chars per 32bits 
#define SOM_KEYS_INI_FLAG 1
#define SOM_KEYS_REG_FLAG 2
#define SOM_KEYS_FVF_FOG (D3DFVF_XYZ)
#define SOM_KEYS_FVF_UNLIT (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)
#define SOM_KEYS_FVF_BLENDED (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1)

struct SOM::KEY::FogFVF
{
	static const DWORD fvf = SOM_KEYS_FVF_FOG;

	float x, y, z;
};
struct SOM::KEY::UnlitFVF
{
	static const DWORD fvf = SOM_KEYS_FVF_UNLIT;

	float x, y, z; DWORD diffuse; float u, v;
};
struct SOM::KEY::BlendedFVF
{
	static const DWORD fvf = SOM_KEYS_FVF_BLENDED;

	float x, y, z, l, i, t, u, v;
};

static SOM::KEY::image_t som_keys_default_image_t;
static SOM::KEY::image_t som_keys_initial_image_t;
static SOM::KEY::model_t som_keys_default_model_t;
static SOM::KEY::model_t som_keys_initial_model_t;

const SOM::KEY::image_t *SOM::KEY::Image::_default = &som_keys_default_image_t;
const SOM::KEY::image_t *SOM::KEY::Image::_initial = &som_keys_initial_image_t;
const SOM::KEY::model_t *SOM::KEY::Model::_default = &som_keys_default_model_t;
const SOM::KEY::model_t *SOM::KEY::Model::_initial = &som_keys_initial_model_t;

HKEY SOM::KEY::Image::key = 0;
LONG SOM::KEY::Image::err = 0; //ERROR_SUCCESS
HKEY SOM::KEY::Model::key = 0;
LONG SOM::KEY::Model::err = 0; //ERROR_SUCCESS

const wchar_t *SOM::KEY::Image::ini = 0;
const wchar_t *SOM::KEY::Image::eof = 0;		
const wchar_t *SOM::KEY::Image::bad = 0;
const wchar_t *SOM::KEY::Model::ini = 0;
const wchar_t *SOM::KEY::Model::eof = 0;		
const wchar_t *SOM::KEY::Model::bad = 0;

void (*SOM::KEY::Image::clientdtor)(SOM::KEY::Image*) = 0;

static const wchar_t *som_keys_bad = L""; 
static const wchar_t *som_keys_project = 0; 
extern const wchar_t *SOM::KEY::newvalue = L"<initial value>";
extern const wchar_t *SOM::KEY::regvalue = L"<registry value>";

typedef struct //hash_compare (reproduces GHashTable)
{
	static const size_t bucket_size = 4, min_buckets = 8;

	size_t operator()(const unsigned *in)const //djb2
	{
		//som_keys_hash_f		
		unsigned out = 5381; assert(in&&*in&&*in<64);
		for(unsigned i=*in;i>0;i--) out = ((out<<5)+out)+*in++;
		int compile[sizeof(unsigned)==sizeof(size_t)];
		return out;    
	}				   
	bool operator()(const unsigned *in1, const unsigned *in2)const
	{
	  //som_keys_equal_f 
	  assert(in1&&*in1&&*in1<64&&in2&&*in2&&*in2<64);	
	  //for(unsigned i=*in1;i>0;i--) if(*in1++!=*in2++) return false;
      for(unsigned i=*in1;i>0;i--) if(*in1++<*in2++) return true;
	  return false;
	}

}dbj2;
template<class T> struct som_keys : stdext::hash_map<const unsigned*,T,dbj2>
{
	//reproducing patterns of GHashTable
	inline T *findptr(const unsigned *k)
	{
		iterator it = find(k); return it==end()?0:&it->second; 
	}
	inline T *insertnew(const unsigned *k) 
	{
		//hash is a chicken and egg problem
		int compile[sizeof(value_type)==sizeof(k)+sizeof(T)];

		T &paranoia = operator[](k); paranoia.~T(); //busy		
		new(&paranoia) T(k);		
		((const unsigned**)&paranoia)[-1] = paranoia.hash;		
		return &paranoia;
	}
};
static som_keys<SOM::KEY::Image> *som_keys_images = 0;
static som_keys<SOM::KEY::Model> *som_keys_models = 0;

static const int som_keys_parities[256] = 
{
#define P2(n) n, n^1, n^1, n
#define P4(n) P2(n), P2(n^1), P2(n^1), P2(n)
#define P6(n) P4(n), P4(n^1), P4(n^1), P4(n)
    P6(0), P6(1), P6(1), P6(0)
#undef P2
#undef P4
#undef P6
};

namespace som_keys_squasher
{
	static unsigned __int16 compress(float);
}

static inline unsigned __int16 som_keys_squash(float in)
{
	return som_keys_squasher::compress(in);
}

template<typename T>
static const T *som_keys_hex(const unsigned *hash, bool dot=false)
{
	assert(0); return 0; //see specializations...
}
template<>
static const char *som_keys_hex(const unsigned *hash, bool dot)
{
	static char out[SOM_KEYS_MAX_NAME+1]; 

	if(!hash||!*hash||*hash>SOM_KEYS_MAX_HASH) goto err;

	assert(!dot); //sanity check (use wchar_t specialization instead)

	for(size_t i=0;i<*hash-1;i++)
	{	
		int _8 = sprintf_s(out+8*i,SOM_KEYS_MAX_NAME+1-8*i,"%08x",hash[1+i]); 
		
		assert(_8==8); if(_8!=8) goto err;
	}

	return out;

err: *out = '\0'; return out;
}
template<>
static const wchar_t *som_keys_hex(const unsigned *hash, bool ambiguous)
{
	static wchar_t out[1+SOM_KEYS_MAX_NAME+1]; 

	if(!hash||!*hash||*hash>SOM_KEYS_MAX_HASH) goto err;

	int dot = ambiguous?1:0;

	for(size_t i=0;i<*hash-1;i++)
	{	
		int _8 = swprintf_s(out+dot+8*i,SOM_KEYS_MAX_NAME+1-8*i,L"%08x",hash[1+i]); 
		
		assert(_8==8); if(_8!=8) goto err;
	}

	return out;

err: *out = '\0'; return out;
}

template<typename T>
static bool som_keys_test(const T *hex)
{
	int i;
	for(i=0;hex[i]&&i<SOM_KEYS_MAX_NAME;i++)
	{
		if(hex[i]>='0'&&hex[i]<='9'
		 ||hex[i]>='a'&&hex[i]<='z'
		 ||hex[i]>='A'&&hex[i]<='Z') continue;
		break;
	}
	return i&&!hex[i];
}
static const unsigned *som_keys_hash(const char *hex, bool test = false)
{
	static unsigned out[SOM_KEYS_MAX_HASH];

	if(test&&!som_keys_test(hex)) return 0;

	const char *p = hex; char x[9]; *out = 1;

	for(int i=0;*p&&i<SOM_KEYS_MAX_HASH;i++)
	{
		int j = 0; while(*p&&j<8) j++; x[j] = *p++; x[j] = '\0';

		if(*x) out[(*out)++] = strtoul(x,0,16);
	}

	return out;
}

static const unsigned *som_keys_hash(const wchar_t *hex, bool test=false)
{
	static unsigned out[SOM_KEYS_MAX_HASH];

	if(test&&!som_keys_test(hex)) return 0;

	const wchar_t *p = hex; wchar_t x[9]; *out = 1;

	for(int i=0;*p&&i<SOM_KEYS_MAX_HASH;i++)
	{
		int j = 0; while(*p&&j<8) x[j++] = *p++; x[j] = '\0';

		if(*x) out[(*out)++] = wcstoul(x,0,16);
	}

	return out;
}

static bool som_keys_hash_overflow = false; //hack

static unsigned *som_keys_hash(IDirect3DTexture9 *in, int lv=-1, SOM::KEY::Image *UP=0)
{	
	som_keys_hash_overflow = false;

	if(lv>1) 
	{
		som_keys_hash_overflow = true;

		return 0; //hack: limit to 2x2 mipmaps for now
	}

	static unsigned out[SOM_KEYS_MAX_HASH];

	if(lv==-1) return out; //retrieving hash 

	int lvs = in->GetLevelCount(); if(!lvs) return 0;

	D3DSURFACE_DESC desc; D3DLOCKED_RECT lock;

	if(lv==0)
	{
		in->GetLevelDesc(0,&desc);

		unsigned __int16 squashed;

		if(desc.Width<desc.Height)
		{
			squashed = som_keys_squash(-fabs(float(desc.Width)/desc.Height+desc.Height));			
		}
		else squashed = som_keys_squash(float(desc.Height)/desc.Width+desc.Height);
				
		switch(desc.Format)
		{
		case D3DFMT_X8R8G8B8: desc.Format = D3DFMT_A8R8G8B8; break;
		case D3DFMT_X1R5G5B5: desc.Format = D3DFMT_A1R5G5B5; break;
		}

		out[1] = squashed|desc.Format<<16;

		*out = 2; 
	}

	if(in->GetLevelDesc(lvs-lv-1,&desc)!=D3D_OK) return 0;	
	
	if(lv==0) 
	{
		//NEW: ignore if mipmap_samples_limit limited
		//assert(desc.Height==1&&desc.Width==1);
		if(desc.Height!=1||desc.Width!=1) return 0;	
	}

	int bpp = 0;

	switch(desc.Format)
	{
	case D3DFMT_A8R8G8B8: 
	case D3DFMT_X8R8G8B8: bpp = 4; break;
	case D3DFMT_A1R5G5B5:
	case D3DFMT_X1R5G5B5: bpp = 2; break;

	default: assert(0); return 0;
	}
	
	if(in->LockRect(lvs-lv-1,&lock,0,D3DLOCK_READONLY)!=D3D_OK) return 0;

	size_t m = desc.Width*bpp%4, n = desc.Width*bpp/4;

	if(*out+n*desc.Height+m>SOM_KEYS_MAX_HASH)
	{
		in->UnlockRect(lvs-lv-1);

		som_keys_hash_overflow = true;

		return 0; 
	}

	for(size_t i=0;i<desc.Height;i++)
	{
		DWORD *p = (DWORD*)((BYTE*)lock.pBits+lock.Pitch*i);

		for(size_t j=0;j<n;j++) out[(*out)++] = p[j];

		if(m) out[(*out)++] = *(WORD*)&p[n];
	}

	in->UnlockRect(lvs-lv-1);

	return out;
}

static int som_keys_quadrant(float x, float z)
{
	switch((z<0)<<1|x<0)
	{				  //Cartesian
	case 0: return 1; //I   // II|I
	case 1: return 2; //II  //---+---
	case 2: return 4; //IV	//III|IV
	case 3: return 3; //III //quadrants
	}

	return 0; //compiler
}

template<typename T>
static int som_keys_quadrant(const T *verts, const WORD *prims, int n)
{		
	int nz;
	for(nz=1;nz<n;nz++) //nonzero
	if(verts[prims[nz]].x!=verts[*prims].x
	 ||verts[prims[nz]].z!=verts[*prims].z) break;

	float ax = verts[prims[nz]].x-verts[*prims].x; 
	float az = verts[prims[nz]].z-verts[*prims].z; 
		
	return som_keys_quadrant(ax,az); 
}

template<typename T>
static int som_keys_quadrant(const T *verts, const WORD *a, const WORD *b, int n)
{	
	int nz; //nonzero
	for(nz=1;nz<n;nz++) 
	{
		//assuming a & b are symmetric
		if(verts[a[nz]].x!=verts[*a].x
		 ||verts[a[nz]].z!=verts[*a].z) break;
	}

	float ax = verts[a[nz]].x-verts[*a].x; 
	float az = verts[a[nz]].z-verts[*a].z; 
	float bx = verts[b[nz]].x-verts[*b].x; 
	float bz = verts[b[nz]].z-verts[*b].z; 
	
	//quadrant of b relative to a's quadrant
	return som_keys_quadrant(ax*bx,az*bz); 
}

template<typename T>
static void som_keys_rotate(int quadrant, T &x, T &z)
{
#define SWAP(X,Z) {T tmp=X;x=Z;z=tmp;}

	switch(quadrant) 
	{				  
	case 2: case -4: SWAP(-x,z); break;	
	case 3: case -3: x = -x; z = -z; break;
	case 4: case -2: SWAP(x,-z); break;
	}

#undef SWAP
}

template<typename T1, typename T2>
static void som_keys_rotate(const T1 *verts, const WORD *prims, int n, T2 &x, T2 &z)
{
	som_keys_rotate(som_keys_quadrant(verts,prims,n),x,z); 
}

template<typename T1, typename T2>
static void som_keys_revert(const T1 *verts, const WORD *prims, int n, T2 &x, T2 &z)
{
	som_keys_rotate(-som_keys_quadrant(verts,prims,n),x,z);
}

template<typename T>
static unsigned *som_keys_hash(const T *verts, DWORD m=0, const WORD *prims=0, DWORD n=0, int lv=-1, SOM::KEY::Model *UP=0)
{	
	som_keys_hash_overflow = false;

	if(lv>2) 
	{
		som_keys_hash_overflow = true;
  
		return 0; //hack: limit to 3 levels for now
	}

	static unsigned out[SOM_KEYS_MAX_HASH];

	if(lv==-1) return out; //retrieving hash 

	if(lv==0)
	{
		unsigned __int16 squashed;
		squashed = som_keys_squash(float(m)/n);			
		out[1] = squashed|T::fvf<<16;

		*out = 2; 
	}

	size_t next = lv*3;

	if(UP&&UP->hint&&*UP->hint>2)
	{
		switch(UP->hint[1]) 
		{
		case 'hop':	next = UP->hint[2]; break; 
		}
	}

	if(n>=3) //paranoia
	if(next>n-3||*out+3>SOM_KEYS_MAX_HASH)
	{
		som_keys_hash_overflow = true; return 0; 
	}

	const T *p = verts+prims[next%n];
	const T *q = verts+prims[(next+1)%n];

	short *out16 = (short*)(out+*out);

	//Note: the addressing here is flipped per 32bits

	out16[1] = q->u*255-p->u*255;		
	out16[0] = q->v*255-p->v*255; 

	switch(T::fvf)
	{
	case SOM_KEYS_FVF_UNLIT:

		out16[3] = q->x*1000-p->x*1000;
		out16[2] = q->y*1000-p->y*1000; 
		out16[5] = q->z*1000-p->z*1000; 

		som_keys_rotate(verts,prims,n,out16[3],out16[5]);
		break;

	case SOM_KEYS_FVF_BLENDED:

		out16[3] = ((SOM::KEY::BlendedFVF*)p)->l*4096;
		out16[2] = ((SOM::KEY::BlendedFVF*)p)->i*4096;
		out16[5] = ((SOM::KEY::BlendedFVF*)p)->t*4096;	 
		break;

	default: assert(0); return 0;
	}

	out16[4] = next;

	*out+=3; //12bytes

	return out;
}

template<typename T>
size_t som_keys_buffersz(int profile, void *buffer)
{
	size_t out = ((size_t*)buffer)[-1]; return out/sizeof(T);
}

template<typename T>
T *som_keys_buffer(int profile, float multiply = 1.0f)
{
	assert(multiply>=1.0f);

	static size_t *buffer = 0; //TODO: one buffer per profile

	static const size_t inimax = 65535; //so says docs
	
	if(!buffer)
	{
		assert(multiply==1.0f);

		buffer = new size_t[4096+1]; *buffer++ = 4096*sizeof(size_t);
	}

	if(multiply>1.0f)
	{
		size_t sz = som_keys_buffersz<T>(profile,buffer);
		size_t newsz = multiply*sz;

		switch(profile)
		{
		case '.ini': newsz = min(newsz,inimax); break;
		}

		if(sz!=newsz)
		{
			size_t *swap = buffer;
			buffer = new size_t[newsz*sizeof(T)/sizeof(size_t)+2];			
			*buffer++ = newsz*sizeof(T);
			memcpy(buffer,swap,swap[-1]);
			delete[] --swap;
		}
	}

	return (T*)buffer;
}

static unsigned som_keys_hash_f(const unsigned *in) //djb2
{
    unsigned out = 5381; assert(in&&*in&&*in<64);
	for(unsigned i=*in;i>0;i--) out = ((out<<5)+out)+*in++;
    return out;    
}				   
static inline bool som_keys_equal_f(const unsigned *in1, const unsigned *in2)
{
	assert(in1&&*in1&&*in1<64&&in2&&*in2&&*in2<64);	
	for(unsigned i=*in1;i>0;i--) if(*in1++!=*in2++) return false;
	return true;
}

static HKEY som_keys_create(const wchar_t *subkey, HKEY top = 0)
{
	//TODO: migrate to generic registry routines header/object

	static wchar_t *tmp = 0; static size_t tmplen = 0;

	if(!top)
	{
		tmp = wcsdup(subkey); tmplen = wcslen(tmp);

		for(size_t i=0;i<tmplen;i++)
		{
			if(tmp[i]=='\\'||tmp[i]=='/') tmp[i] = '\0';
		}

		subkey = tmp; //...
	}
		
	HKEY out = 0; 

	if(tmp&&subkey>=tmp&&subkey<tmp+tmplen&&!tmp[tmplen]) //paranoia
	{
		LONG err = RegCreateKeyExW(HKEY_CURRENT_USER,subkey,0,0,0,KEY_ALL_ACCESS,0,&out,0);

		if(err==ERROR_SUCCESS)
		{
			while(*subkey) subkey++; 

			if(subkey<tmp+tmplen) 
			{
				out = som_keys_create(subkey+1,out);

				RegCloseKey(out);
			}			
		}
	}	

	if(!top) free(tmp);

	return out;
}

template<class T>
static som_keys<T> *som_keys_open(const wchar_t *sub, const wchar_t *ini)
{
	if(sub&&!T::err&&!T::key)
	{
		T::err = RegCreateKeyExW(HKEY_CURRENT_USER,sub,0,0,0,KEY_ALL_ACCESS,0,&T::key,0);

		if(T::err!=ERROR_SUCCESS) if(T::key=som_keys_create(sub)) T::err = ERROR_SUCCESS;
	}

	som_keys<T> *out = new som_keys<T>; 	

	if(T::key&&T::err==ERROR_SUCCESS)
	{
		DWORD n = 0; FILETIME tm = {0,0}; 

		T::err = RegQueryInfoKey(T::key,0,0,0,&n,0,0,0,0,0,0,&tm);

		char keyname[256]; //docs: maximum name length

		if(T::err==ERROR_SUCCESS)
		for(unsigned int i=0;i<n;i++)
		{
			DWORD _256 = 256;

			if(RegEnumKeyExA(T::key,i,keyname,&_256,0,0,0,0)==ERROR_SUCCESS)
			{			
				//T *p = new T(keyname);
				const unsigned *p_hash = som_keys_hash(keyname,true);
				
				if(p_hash) //if(p->hash) 
				{						
					T *p = out->insertnew(p_hash);
					p->flags()|=SOM_KEYS_REG_FLAG;
				}
				//else delete p;
			}						  
		}
	}
	
	if(ini) //allocating ini buffer
	{
		assert(!T::ini);

		FILE *p = _wfopen(ini,L"rb");

		if(!p) T::ini = T::eof = T::bad = som_keys_bad;

		if(!T::bad)
		{
			fseek(p,0,SEEK_END);

			long fsize = ftell(p);
			
			if(fsize>0)
			{								
				T::ini = new wchar_t[MAX_PATH+1+fsize]; //assuming UTF16
								
				T::eof = T::ini+MAX_PATH+1+fsize;

				T::inicpy(ini,MAX_PATH); //store filename at top of buffer

				T::inicpy(L"",1); //double null terminate
			}
			else if(fsize<0)
			{
				T::ini = T::eof = T::bad = som_keys_bad;				
			}
			else T::ini = T::eof = T::bad; //hack 		

			fclose(p);
		}
	}

	if(ini&&T::ini&&!T::bad&&T::eof-T::ini) 
	{
		int paranoia = 0x0CD;
		wchar_t *p = som_keys_buffer<wchar_t>('.ini');
		size_t sz = som_keys_buffersz<wchar_t>('.ini',p); 
		while(paranoia=GetPrivateProfileSectionNamesW(p,sz,ini))
		{
			if(paranoia==65535) //docs: maximum permitted
			{
				assert(0); //unimplemented: will require a proper ini library

			  //// TODO: major warning ////////////////////////////////////////

				paranoia = 0; break; //short circuit the ini file
			}
			else if(paranoia==sz-2) //buffer too small
			{
				p = som_keys_buffer<wchar_t>('.ini',2.0f); //double
				sz = som_keys_buffersz<wchar_t>('.ini',p);
			}
			else break;
		}

		for(wchar_t *q=p;q-p<paranoia&&*q;q++)
		{
			bool ambiguous = *q=='.'; if(ambiguous) q++; 

			const unsigned *hash = som_keys_hash(q,true);

			if(hash) //valid class name/hash
			{
				//T *p = (T*)g_hash_table_lookup(out,hash);
				T *p = out->findptr(hash);

				if(!p)
				{
					p = out->insertnew(hash);
					p->ambiguous = ambiguous; //overwriting
					//g_hash_table_insert(out,(gpointer)p->hash,p);
				}

				p->flags()|=SOM_KEYS_INI_FLAG;				
			}
			else //treat as template
			{
				assert(0); //unimplemented
			}

			while(q-p<paranoia&&*q) q++;
		}
	}

	return out;
}

//REMOVE ME?
//static void som_keys_open(GHashTable* &in)
template<class T> static void som_keys_open(T* &in)
{
	bool images = (void*)&in==(void*)&som_keys_images;
	bool models = (void*)&in==(void*)&som_keys_models;

	assert(images||models); if(!images&&!models) return;

	EX::INI::Keygen kg;

	wchar_t ini[MAX_PATH+7] = {'\0'}, key[MAX_PATH] = {'\0'};

	if(images)
	if(*kg->keygen_image_file) wcscpy(ini,kg->keygen_image_file);
	if(models)
	if(*kg->keygen_model_file) wcscpy(ini,kg->keygen_model_file);

	wchar_t *X = images?L"IMAGE":L"MODEL";

	if(*kg->keygen_toplevel_subkey_in_registry)
	{
		wcscpy(key,kg->keygen_toplevel_subkey_in_registry);

		bool lowercase = false; int keylen = wcslen(key); //pretty keyname

		for(int i=0;i<keylen;i++) if(key[i]>='a'&&key[i]<='z')
		{
			lowercase = true; break; 
		}

		if(key[keylen-1]!='\\'&&key[keylen-1]!='/')	key[keylen++-1] = '\\';

		wcscpy(key+keylen,X); //IMAGE/MODEL 

		if(lowercase) //image/model
		for(int i=keylen;key[i];i++) key[i]+=32;		
	}

	if(som_keys_debugging()||kg->do_somdb_keygen_defaults)
	{	
		if(som_keys_debugging())
		if(!*ini) swprintf(ini,MAX_PATH,L"%s\\%sS.INI",EX::cd(),X);
		if(!*key) swprintf(key,MAX_PATH,L"Software\\FROMSOFTWARE\\SOM\\EX\\KEYS\\%s",X);
	}
	else //TODO: check for files / timestamps first
	{
		if(!*key)
		{
			//TODO: when is it appropriate to access the registry?
		}

		if(!*ini) swprintf(ini,MAX_PATH,L"%ls\\EX\\KEYS\\%sS.KEY",EX::cd(),X);
	}

	if(*ini&&PathIsRelativeW(ini)) 
	{
		wchar_t swap[MAX_PATH]; 
		swprintf_s(swap,L"%ls\\%ls",EX::cd(),ini);
		wcscpy(ini,swap);	
	}

	in = som_keys_open<T::mapped_type>(*key?key:0,*ini?ini:0);
}

static void som_keys_enum(HKEY key, SOM::KEY::class_t &in, size_t n, void *f)
{
	typedef const char*(*enumerate_t)(int); //f
		
	const size_t m = sizeof(SOM::KEY::class_t)/sizeof(void*);

	static char *class_e[m] = {0,0,"comments","audit","parity","checksum",0};

	for(int i=3;i<m;i++) 
	{
		const char *val = class_e[i]; 		
		if(RegQueryValueExA(key,val,0,0,0,0)==ERROR_SUCCESS)
		in[i] = SOM::KEY::regvalue;		
	}

	const char *optimization = 0; //avoid unnecessary queries

	for(size_t i=m;i<n;i++)
	{
		const char *val = enumerate_t(f)(i-m); 
		
		if(!val) continue;
		
		if(val==optimization) 
		{
			in[i] = SOM::KEY::regvalue; continue;
		}
		else optimization = val;

		if(!RegQueryValueExA(key,val,0,0,0,0))
		{
			in[i] = SOM::KEY::regvalue;
		}
		else optimization = 0;
	}
}

template<typename t> 
static inline void som_keys_enum(HKEY key, t &in, void *f)
{
	som_keys_enum(key,in,sizeof(t)/sizeof(void*),f);	
}	   

static void som_keys_init
(const wchar_t *sectionname, size_t sz, SOM::KEY::class_t &pub, 
							 size_t n, SOM::KEY::class_t &prv, void *f, void *g)
{	if(!sz) return;	
	typedef int (*initialize_t)(const wchar_t*); //f
	typedef const wchar_t*(*inicpy_t)(const wchar_t*,size_t); //g
	const wchar_t *name = inicpy_t(g)(sectionname,wcslen(sectionname)+1);

	int i = 1; //public/private (private is default)

	wchar_t *p = som_keys_buffer<wchar_t>('.ini');

	for(wchar_t *q,*d,*b=p+sz;p<b&&*p;p++)
	{
		for(q=p;q<b&&*q&&*q!='=';q++); 
		
		if(p==q||q>=b) goto err; 
		
		d = q; if(*q&&q<b) q++; *d = '\0';

		switch(d-p)
		{
		case 6: //maybe public
			
			if(*p=='p'&&!memcmp(p,L"public",12))
			{
				i = 0; //public
				if(*q) //templates
				{
					assert(0); //unimplemented

					pub.section = name;
				}
				goto fin;
			}
			else break;

		case 7: //maybe private
		
			if(*p=='p'&&!memcmp(p,L"private",14))
			{
				i = 1; //private  
				if(*q) //templates
				{
					assert(0); //unimplemented

					prv.section = name;
				}
				goto fin;
			}
			else break;
		}

		size_t j = 0; 
		
		const size_t garbage = 
		sizeof(SOM::KEY::class_t)/sizeof(void*)-1;

		switch(d-p)
		{		
		case 4: //maybe hint

			if(*p=='h'&&!memcmp(p,L"hint",8)) j = 1; break;

		case 5: //maybe audit

			if(*p=='a'&&!memcmp(p,L"audit",10)) j = 3; break;

		case 6: //maybe parity

			if(*p=='p'&&!memcmp(p,L"parity",12)) j = 4; break;			

		case 8: //maybe checksum or comments

			if(*p=='c'&&!memcmp(p,L"comments",16)) j = 2; else
			if(*p=='c'&&!memcmp(p,L"checksum",16)) j = 5; break;
		}

		if(!j) j = garbage+1+initialize_t(f)(p);

		if(j==garbage||j>=n) goto err; 
		for(d=q;d<b&&*d;d++); if(d>=b) goto err;

		SOM::KEY::class_t &now = i?prv:pub;			
		now.section = name;	now[j] = inicpy_t(g)(q,d-q+1);

fin:	for(p=d;p<b&&*p;p++); continue;

err:	goto fin; //TODO: something
	}	
}

template<typename t> 
static inline void som_keys_init
(const wchar_t *sectionname, size_t sz, t &pub, t &prv, void *f, void *g)
{
	som_keys_init(sectionname,sz,pub,sizeof(t)/sizeof(void*),prv,f,g);	
}	   

template<class T>
static void som_keys_read(T *in)
{
	typedef T::t t;

	if(in->complete())
	{
		assert(0); return; //paranoia
	}

	t t3[3]; memset(&t3,0x00,sizeof(t)*3);
	t &pub = t3[0], &prv = t3[1], &prj = t3[2];

	if(T::key&&!T::err)
	{
		HKEY top; LONG err = 
		RegOpenKeyExA(T::key,som_keys_hex<char>(in->hash),0,KEY_ALL_ACCESS,&top); 		
		err = RegQueryValueExA(top,"ambiguous",0,0,0,0);

		const wchar_t *ambiguous = err==ERROR_SUCCESS?SOM::KEY::regvalue:0;

		if(ambiguous) err = RegQueryValueExA(top,"hint",0,0,0,0);

		const wchar_t *hint = err==ERROR_SUCCESS?SOM::KEY::regvalue:0;

		for(int i=0;i<3;i++)
		{
			const wchar_t *which = 0;

			switch(i)
			{
			case 0: which = L"public"; break;
			case 1: which = L"private"; break;
			case 2: which = som_keys_project;
			}

			if(!which) continue;

			HKEY key = 0; LONG err = 				
			RegOpenKeyExW(top,which,0,KEY_ALL_ACCESS,&key);			
			if(err==ERROR_SUCCESS) 
			{
				if(ambiguous) //special cases
				{
					t3[i].section = ambiguous; 
					t3[i].hint = hint;
				}

				som_keys_enum(key,t3[i],T::enumerate);

				RegCloseKey(key);
			}
		}
		RegCloseKey(top);
	}

	if(in->flags()&SOM_KEYS_INI_FLAG&&T::ini&&!T::bad)
	{
		DWORD paranoia = 0x0CD;
		wchar_t *p = som_keys_buffer<wchar_t>('.ini');
		size_t sz = som_keys_buffersz<wchar_t>('.ini',p); 
		const wchar_t *name = som_keys_hex<wchar_t>(in->hash,in->ambiguous);
		while(paranoia=GetPrivateProfileSectionW(name,p,sz,T::ini))
		{
			if(paranoia==65535) //docs: maximum permitted
			{
				assert(0); //unimplemented: will require a proper ini library

			  //// TODO: major warning ////////////////////////////////////////

				paranoia = 0; break; //short circuit the ini file
			}
			else if(paranoia==sz-2) //buffer too small
			{
				p = som_keys_buffer<wchar_t>('.ini',2.0f); //double			
				sz = som_keys_buffersz<wchar_t>('.ini',p);
			}
			else break;
		}
		
		som_keys_init(name,paranoia,pub,prv,T::initialize,T::inicpy);
	}

	new (in) T(prj,prv,pub); //placement syntax
}

static SOM::KEY::Image *som_keys_image(const unsigned *hash)						
{
	//Note: identical to som_keys_model()

	if(!hash||!*hash) return 0;
	if(!som_keys_images) som_keys_open(som_keys_images);
	SOM::KEY::Image *out = som_keys_images->findptr(hash);
	//(SOM::KEY::Image*)g_hash_table_lookup(som_keys_images,hash);
		
	//catch bugged glib dlls
	if(0&&out&&!som_keys_equal_f(out->hash,hash))
	{
		assert(!out||som_keys_equal_f(out->hash,hash)); 
		unsigned debug[SOM_KEYS_MAX_HASH]; memcpy(debug,hash,sizeof(debug));
		return 0;
	} 
	//glib's not handling collisions correctly
	if(!out||!som_keys_equal_f(out->hash,hash)) 
	if(SOM::KEY::Image::key&&!SOM::KEY::Image::err)
	{
		//requires auto-initialization
		return som_keys_images->insertnew(hash);
	}
	else return 0;

	if(out->erroneous) return out;

	if(!out->complete())
	if(!SOM::KEY::Image::key||out->flags()&SOM_KEYS_REG_FLAG)
	{
		som_keys_read(out);	//registry and or ini initialization
	}
	else return out; //requires auto (plus ini) initialization

	return out; 
}

static SOM::KEY::Model *som_keys_model(const unsigned *hash)
{
	//Note: identical to som_keys_image()

	if(!hash||!*hash) return 0;
	if(!som_keys_models) som_keys_open(som_keys_models);
	SOM::KEY::Model *out = som_keys_models->findptr(hash);
		
	//catch bugged glib dlls
	if(0&&out&&!som_keys_equal_f(out->hash,hash))
	{
		assert(!out||som_keys_equal_f(out->hash,hash)); 
		unsigned debug[SOM_KEYS_MAX_HASH]; memcpy(debug,hash,sizeof(debug));
		return 0;
	}

	//glib's not handling collisions correctly
	if(!out||!som_keys_equal_f(out->hash,hash)) 
	//if(SOM::KEY::Model::key&&!SOM::KEY::Model::err)
	{
		//requires auto-initialization
		return som_keys_models->insertnew(hash);
	}
	//else return 0;

	if(out->erroneous) return out;

	if(!out->complete())
	if(!SOM::KEY::Model::key||out->flags()&SOM_KEYS_REG_FLAG)
	{
		som_keys_read(out);	//registry and or ini initialization
	}
	else return out; //requires auto (plus ini) initialization

	return out; 
}

template<class T>
static bool som_keys_readonly(T *in)
{
	if(!in) return true;
	static bool cache, cached = false;
	if(cached) return cache; //assuming all or none readonly
	if(!T::key||T::err) return cached = cache = true; 

	EX::INI::Keygen kg;
	if(som_keys_debugging()||kg->do_somdb_keygen_defaults) 
	{
		cached = true; return cache = false; //assuming
	}
	return cached = cache = true;
}

template <class T>
static bool som_keys_ambiguate(T *in)
{
	if(!in) return false; assert(!in->ambiguous);

	if(in->ambiguous) return true;
	
	if(T::key&&!T::err&&!som_keys_readonly(in))
	{
		EX::INI::Keygen kg;
		if(kg->do_enable_keygen_automation)
		{
			HKEY top; LONG err =
			RegOpenKeyExA(T::key,som_keys_hex<char>(in->hash),0,KEY_ALL_ACCESS,&top);

			//assert(err==ERROR_SUCCESS);

			if(err==ERROR_SUCCESS)
			{
				BYTE bin = 1;
				err = RegSetValueExA(top,"ambiguous",0,REG_BINARY,&bin,1);
				assert(err==ERROR_SUCCESS);
				RegCloseKey(top);
			}
		}
	}

	in->ambiguous = true; 

	return true;
}

template<typename T>
static void som_keys_update(T *in, const wchar_t *value, const BYTE *overwritewith, size_t overwritesize)
{
	if(som_keys_readonly(in)) return;

	if(!value||!overwritewith||!overwritesize) return;

	int _value_ = in->initialize(value);

	const wchar_t *current = in->setting(_value_);

	if(current!=SOM::KEY::regvalue)
	{
		if(current&&current!=SOM::KEY::newvalue) return;
	
		EX::INI::Keygen kg;
		if(!kg||!kg->do_enable_keygen_automation) return;

		if(!current)
		{
			const wchar_t *f = kg->keygen_automatic_filter;
			if(!in->filter(*f?f:L"public",_value_,SOM::KEY::newvalue)) return;			
		}
	}

	HKEY top, key; LONG err = 
	RegOpenKeyExA(T::key,som_keys_hex<char>(in->hash),0,KEY_ALL_ACCESS,&top);

	if(err==ERROR_SUCCESS) 
	{
		const wchar_t *sub[3] = {L"public",L"private",som_keys_project};
		err = RegOpenKeyExW(top,sub[in->select(_value_)],0,KEY_ALL_ACCESS,&key);
		if(err==ERROR_SUCCESS)
		{
			err = RegSetValueExW(key,value,0,REG_BINARY,overwritewith,overwritesize);
			assert(err==ERROR_SUCCESS);
			err = RegCloseKey(key);
		}
		err = RegCloseKey(top);
	}	
}
 
template<typename T>
static short *som_keys_prepare(const T *verts, const WORD *prims, DWORD n)
{
	//TODO: make use of som_keys_buffer()

	static size_t maxout = 4096*5; 
	static short *out = new short[maxout];

	if(n*5>maxout)
	{
		delete[] out; out = new short[maxout=n*5];
	}

	short *p = out+1; *out = 1+n*5-5; //size in shorts

	if(T::fvf==SOM_KEYS_FVF_BLENDED) *out+=3; //final normal

	//displacement should be rotation/animation independent

	const T *t = verts+prims[0]; 
		
	*p++ = t->u*255.0f; 
	*p++ = t->v*255.0f; 
	
	for(size_t i=1;i<n;i++)
	{
		t = verts+prims[i];

		*p++ = t->u*255.0f; p[-3]-=p[-1];
		*p++ = t->v*255.0f; p[-3]-=p[-1];
	}

	p-=2; t = verts+prims[0]; 
	float x = t->x, y = t->y, z = t->z; int q;

	switch(T::fvf)
	{
	case SOM_KEYS_FVF_UNLIT:
	
		q = som_keys_quadrant(verts,prims,n);
		x*=1000; y*=1000; z*=1000;

		for(size_t i=1;i<n;i++)
		{
			t = verts+prims[i];

			*p++ = t->x*1024-x; x = t->x*1024;
			*p++ = t->y*1024-y; y = t->y*1024;
			*p++ = t->z*1024-z; z = t->z*1024;

			if(q>1) som_keys_rotate(q,p[-3],p[-1]);
		}
		break;
	
	case SOM_KEYS_FVF_BLENDED:  

		//you don't want to displace face normals

		for(size_t i=0;i<n;i++) 
		{
			SOM::KEY::BlendedFVF *t = 
			(SOM::KEY::BlendedFVF*)verts+prims[i];

			*p++ = t->l*4096;
			*p++ = t->i*4096;
			*p++ = t->t*4096;
		}
		break;
	}

	assert(*out==p-out);		
	*out = p-out;
	return out;
}

static unsigned som_keys_parity(size_t n, const void *p)
{
	unsigned out = 0; 	
	const BYTE *q = (const BYTE*)p;	//docs say 8bits		
	for(size_t i=0;i<n;i++) out+=som_keys_parities[*q++];
	return out;
}

static unsigned som_keys_parity(short *prepared)
{
	return prepared?som_keys_parity(sizeof(short)*prepared[0],prepared):0;
}

static unsigned som_keys_parity(D3DSURFACE_DESC &desc, D3DLOCKED_RECT lock)
{
	unsigned out = 0;

	int bpp = 0;

	switch(desc.Format)
	{
	case D3DFMT_A8R8G8B8: 
	case D3DFMT_X8R8G8B8: bpp = 4; break;
	case D3DFMT_A1R5G5B5:
	case D3DFMT_X1R5G5B5: bpp = 2; break;

	default: assert(0); return 0;
	}
	
	int n = desc.Width*bpp;
	for(size_t i=0;i<desc.Height;i++)
	{
		BYTE *p = (BYTE*)lock.pBits+lock.Pitch*i;
		out+=som_keys_parity(n,p);
	}

	return out;
}

static unsigned som_keys_checksum(size_t n, const void *p)
{
	//note: assuming xor commutes

	unsigned out = 0; 
	
	const DWORD *q = (const DWORD*)p; //docs say 32bits
	
	int r = n%4; n/=4; //remainder

	size_t i = 0; while(i<n) out^=q[i++];

	switch(r)
	{
	case 3: out^=q[i]&0xFFFFFF; break;
	case 2:	out^=q[i]&0x00FFFF; break;
	case 1: out^=q[i]&0x0000FF; break;
	}

	return out;
}

static unsigned som_keys_checksum(short *prepared)
{
	return prepared?som_keys_checksum(sizeof(short)*prepared[0],prepared):0;
}

static unsigned som_keys_checksum(D3DSURFACE_DESC &desc, D3DLOCKED_RECT lock)
{
	unsigned out = 0;

	int bpp = 0;

	switch(desc.Format)
	{
	case D3DFMT_A8R8G8B8: 
	case D3DFMT_X8R8G8B8: bpp = 4; break;
	case D3DFMT_A1R5G5B5:
	case D3DFMT_X1R5G5B5: bpp = 2; break;

	default: assert(0); return 0;
	}

	size_t m = desc.Width*bpp%4, n = desc.Width*bpp/4;

	for(size_t i=0;i<desc.Height;i++)
	{
		DWORD *p = (DWORD*)((BYTE*)lock.pBits+lock.Pitch*i);

		for(size_t j=0;j<n;j++) out^=p[j];

		if(m) out^=*(WORD*)&p[n];
	}

	return out;
}

static bool som_keys_compare(IDirect3DTexture9 *in, unsigned parity, unsigned checksum)
{	
	D3DSURFACE_DESC desc; D3DLOCKED_RECT lock;

	if(in->GetLevelDesc(0,&desc)!=D3D_OK
	  ||in->LockRect(0,&lock,0,D3DLOCK_READONLY)!=D3D_OK) return true;

	bool out = som_keys_parity(desc,lock)==parity&&som_keys_checksum(desc,lock)==checksum;
	in->UnlockRect(0);
	return out;
}

template<typename T>
static bool som_keys_compare(const T *verts, const WORD *prims, DWORD n, unsigned parity, unsigned checksum)
{
	bool out = true;  
	short *p = som_keys_prepare(verts,prims,n);
	out = som_keys_parity(p)==parity&&som_keys_checksum(p)==checksum;
	return out;
}

static int som_keys_timeout = 0;

static unsigned som_keys_timeref = 0;

template<typename T>
static int &som_keys_metrics(T *in)
{
	static int out = 1;	
	if(out<60) out = 60; //a bogus frame
	return out;
}

template<typename T>
static int som_keys_time(T *in, int add = 0)
{
	som_keys_timeout+=add; 

	if(SOM::frame!=som_keys_timeref)
	{
		som_keys_timeout = som_keys_timeout<0?0:10;
		som_keys_timeref = SOM::frame;

		som_keys_metrics(in) = som_keys_hit(in,true);
	}

	if(som_keys_timeout<0) return 0;

	return som_keys_timeout; 
}

template<typename T>
static void som_keys_miss(T *in)
{
	som_keys_time(in,-1000);
}

template<typename T>
static int som_keys_hit(T *in, bool reset = false)
{
	static int hits = 0; int out = in?++hits:hits; 	
	if(reset) hits = 0; return out;
}

static bool som_keys_srand = false;

template<typename T>
static int som_keys_rand(T *in, int passthru=1)
{
	if(!som_keys_srand)
	{
		srand(time(0));	som_keys_srand = true;
	}

	if(som_keys_time(in)) return rand();

	return passthru;
}

static bool som_keys_audit(const SOM::KEY::Image *in, D3DLOCKED_RECT lock, int partial=0)
{
	if(!in) return true;

	EX::INI::Keygen kg;
	if(som_keys_debugging()||kg->do_somdb_keygen_defaults)
	{
		if(kg->do_disable_keygen_auditing) return true; //defaults to enabled
	}
	else if(!kg||!kg->do_enable_keygen_auditing) return true;		

	int bpp = 0;

	switch(in->dxformat)
	{
	case D3DFMT_A8R8G8B8: 
	case D3DFMT_X8R8G8B8: bpp = 4; break;
	case D3DFMT_A1R5G5B5:
	case D3DFMT_X1R5G5B5: bpp = 2; break;

	default: assert(0); return true;
	}
	
	static int file = 0;
	static wchar_t path[MAX_PATH+9] = {'\0'}; 

	if(!*path)
	{
		if(*kg->keygen_audit_folder)
		{
			wcscpy(path,kg->keygen_audit_folder);

			int pathlen = wcslen(path);
			if(path[pathlen-1]!='\\'&&path[pathlen-1]!='/')
			{
				path[pathlen-1] = '\\';
			}
			wcscat(path,L"texture");
		}
		else
		{
			GetEnvironmentVariableW(L".NET",path,MAX_PATH);
			wcscat_s(path,MAX_PATH,L"\\data\\key\\texture");
		}

		//NOTE: this API refuses / but this case should be clean
		SHCreateDirectoryExW(0,path,0);		
		file = min(MAX_PATH,wcslen(path)); 
		path[file++] = '\\';
	}

	//Note: return includes null terminator
	int guidlen = StringFromGUID2(in->audit,path+file,MAX_PATH-file-4);

	if(!guidlen||file+guidlen+4>MAX_PATH)
	{
		//TODO: issue one time warning

		return true;
	}

	wcscpy(path+file+guidlen-1,L".dds");

	FILE *dds = _wfopen(path,L"rb");

	if(dds) //TODO: actual auditing
	{
		fclose(dds);
		assert(0); return true; //unimplemented;
	} 

	//writing .dds file for first time

	DDS_HEADER header; 	
	assert(sizeof(header)==124);
	memset(&header,0x00,sizeof(DDS_HEADER));	
	header.dwSize = 124;
	header.dwHeaderFlags = DDS_HEADER_FLAGS_TEXTURE;
	header.dwHeight = in->texelsuv[1];
	header.dwWidth = in->texelsuv[0];
	header.dwSurfaceFlags = DDS_SURFACE_FLAGS_TEXTURE;
	assert(sizeof(header.ddspf)==32); 
	header.ddspf = DDSPF_A8R8G8B8;
	switch(in->dxformat)
	{
	case D3DFMT_A8R8G8B8: 
	case D3DFMT_X8R8G8B8: header.ddspf = DDSPF_A8R8G8B8; break;
	case D3DFMT_A1R5G5B5: 
	case D3DFMT_X1R5G5B5: header.ddspf = DDSPF_A1R5G5B5; break;
	}	  
/*	switch(in->dxformat)
	{
	case D3DFMT_X8R8G8B8: case D3DFMT_X1R5G5B5:  
		
		header.ddspf.dwFlags&=~1; //DDPF_ALPHAPIXELS

		header.ddspf.dwABitMask = 0; 
	}
*/
	DWORD ddsmagic = DDS_MAGIC; //'DDS'

	dds = _wfopen(path,L"wb");

	if(dds)
	if(fwrite("BAD",4,1,dds)) 
	if(fwrite(&header,124,1,dds))		
	for(size_t i=0;i<in->texelsuv[1];i++)	
	if(!fwrite((BYTE*)lock.pBits+lock.Pitch*i,in->texelsuv[0]*bpp,1,dds)) 
	break;

	if(dds&&!ferror(dds)) 
	{
		fseek(dds,0,SEEK_SET);		
		fwrite(&ddsmagic,4,1,dds);
	}

	if(dds) fclose(dds);

	return true;
}

template<typename T>
static bool som_keys_audit(const SOM::KEY::Model *in, const T *verts, const WORD *prims)
{
	if(!in) return true;

	EX::INI::Keygen kg;
	if(som_keys_debugging()||kg->do_somdb_keygen_defaults)
	{
		if(kg->do_disable_keygen_auditing) return true; //defaults to enabled
	}
	else if(!kg||!kg->do_enable_keygen_auditing) return true;		

	static wchar_t path[MAX_PATH+9] = {'\0'}; static int file = 0;

	if(!*path)
	{
		if(*kg->keygen_audit_folder)
		{
			wcscpy(path,kg->keygen_audit_folder);

			int pathlen = wcslen(path);
			if(path[pathlen-1]!='\\'&&path[pathlen-1]!='/')
			{
				path[pathlen-1] = '\\';
			}
			wcscat(path,L"model");
		}
		else
		{
			GetEnvironmentVariableW(L".NET",path,MAX_PATH);
			wcscat_s(path,MAX_PATH,L"\\data\\key\\model");
		}

		//NOTE: this API refuses / but this case should be clean
		SHCreateDirectoryExW(0,path,0);		
		file = min(MAX_PATH,wcslen(path)); 
		path[file++] = '\\';
	}

	//Note: return includes null terminator
	int guidlen = StringFromGUID2(in->audit,path+file,MAX_PATH-file-4);

	if(!guidlen||file+guidlen+4>MAX_PATH)
	{
		//TODO: issue one time warning

		return true;
	}

	wcscpy(path+file+guidlen-1,L".dds");

	FILE *dds = _wfopen(path,L"rb");

	if(dds) //TODO: actual auditing
	{
		fclose(dds);

		assert(0); return true; //unimplemented;
	}

	//writing .dds file for first time

	DDS_HEADER header; 	
	assert(sizeof(header)==124);
	memset(&header,0x00,sizeof(DDS_HEADER));	
	header.dwSize = 124;	
	header.dwHeight = 1; //1D
	header.dwHeaderFlags = DDS_HEADER_FLAGS_TEXTURE;
	header.dwSurfaceFlags = DDS_SURFACE_FLAGS_TEXTURE;
	assert(sizeof(header.ddspf)==32);
	header.ddspf = DDSPF_DX10;

	DDS_HEADER_DXT10 dxt10;
	memset(&dxt10,0x00,sizeof(dxt10));
	dxt10.resourceDimension = D3D10_RESOURCE_DIMENSION_BUFFER;			
	dxt10.arraySize = 1; //TODO: composite/key frames

	DWORD ddsmagic = DDS_MAGIC; //'DDS'

	dds = _wfopen(path,L"wb");

	if(!dds) return true;

	fwrite("BAD",4,1,dds);
	
	// 16bit indices ///////////////////
	
	header.dwWidth = in->polygons*3;
	fwrite(&header,124,1,dds);		
	dxt10.dxgiFormat = DXGI_FORMAT_R16_UINT;
	fwrite(&dxt10,sizeof(dxt10),1,dds);
	fwrite(prims,sizeof(WORD)*in->polygons*3,1,dds);
	
  //// departing from .dds format ////////////////////
 //just an embedded dds file except for magic word //

	fwrite(&T::fvf,4,1,dds); //FVF data (mono block for now)		
	assert(sizeof(T)%4==0); //members should all be FLOAT or DWORD
	header.dwWidth = in->vertices*sizeof(T)/4; 
	fwrite(&header,124,1,dds);
	dxt10.dxgiFormat = DXGI_FORMAT_R32_TYPELESS;
	fwrite(&dxt10,sizeof(dxt10),1,dds);
	fwrite(verts,sizeof(T)*in->vertices,1,dds);

	if(dds&&!ferror(dds))
	{
		fseek(dds,0,SEEK_SET);
		
		fwrite(&ddsmagic,4,1,dds);
	}

	if(dds) fclose(dds);

	return true;
}

template<typename T>
static int som_keys_xyzcmp(const T *verts, const WORD *a, const WORD *b, int n=1)
{	
#define Q(A,SIGN,B) (fabsf(\
	(verts[a[1]].A-verts[a[0]].A)SIGN\
	(verts[b[1]].B-verts[b[0]].B))>0.00003f)

	switch(som_keys_quadrant(verts,a,b,n))
	{
	case 1:	for(int i=1;i<n;i++,a++,b++)
			if(Q(y,-,y)||Q(x,-,x)||Q(z,-,z)) return n-i;
			break;
	case 2:	for(int i=1;i<n;i++,a++,b++)
			if(Q(y,-,y)||Q(x,-,z)||Q(z,+,x)) return n-i;
			break;
	case 3:	for(int i=1;i<n;i++,a++,b++)
			if(Q(y,-,y)||Q(x,+,x)||Q(z,+,z)) return n-i;
			break;
	case 4:	for(int i=1;i<n;i++,a++,b++)
			if(Q(y,-,y)||Q(x,+,z)||Q(z,-,x)) return n-i;			
			break;
	}
	return 0; //zero for equivalence

#undef Q
#undef Y
}

template<typename T>
static int som_keys_uvscmp(const T *verts, const WORD *a, const WORD *b, int n=1)
{	
#ifdef _DEBUG

	static bool paranoia = false;

	if(!paranoia)
	{
		assert(&verts[0].v-&verts[0].u==1&&sizeof(float)==sizeof(double)/2);
		paranoia = true;
	}

#endif

	int i;
	for(i=0;i<n;i++)	
	if(*(double*)&verts[*a++].u!=*(double*)&verts[*b++].u) 
	break;

	return n-i; //zero for equivalence
}

template<typename T>
static void som_keys_extents(SOM::KEY::Model *in, const T *verts, const WORD *prims)
{
	static int maxoverlap = 4096; 
	static char *overlap = new char[maxoverlap];

	int m = in->vertices, n = in->polygons*3;

	if(m>maxoverlap)
	{
		delete[] overlap; overlap = new char[maxoverlap=m];
	}

	memset(overlap,0x00,sizeof(char)*m);

	float minmax[6] = 
	{
		+FLT_MAX,+FLT_MAX,+FLT_MAX,
		-FLT_MAX,-FLT_MAX,-FLT_MAX
	};

	for(int i=0;i<n;i++) 
	{
//		assert(prims[i]<maxoverlap);

		if(prims[i]<maxoverlap&&!overlap[prims[i]])
		{
			minmax[0] = min(minmax[0],verts[prims[i]].x);
			minmax[1] = min(minmax[1],verts[prims[i]].y);
			minmax[2] = min(minmax[2],verts[prims[i]].z);
			minmax[3] = max(minmax[3],verts[prims[i]].x);
			minmax[4] = max(minmax[4],verts[prims[i]].y);
			minmax[5] = max(minmax[5],verts[prims[i]].z);

			overlap[prims[i]] = true; 
		}
	}

	float center[3] =
	{
		(minmax[0]+minmax[3])/2.0f,
		(minmax[1]+minmax[4])/2.0f,
		(minmax[2]+minmax[5])/2.0f
	};

	for(int i=0;i<6;i++) minmax[i]-=center[i%3];

	if(T::fvf==SOM_KEYS_FVF_UNLIT)
	{
		center[0]-=verts[*prims].x;
		center[1]-=verts[*prims].y;
		center[2]-=verts[*prims].z;

		som_keys_rotate(verts,prims,n,center[0],center[2]);
	}
		
	in->center[0] = center[0];
	in->center[1] = center[1];
	in->center[2] = center[2];
	in->radius    = max(minmax[3],minmax[5]);
	in->height    = minmax[4];
	in->margin[0] = 0.0f;
	in->margin[1] = 0.0f;
	in->margin[2] = 0.0f;
	 	
	static int debug = 0;

	if(0) if(T::fvf==SOM_KEYS_FVF_UNLIT)
	{
		if(debug)
		{
			in->radius = in->height = 0;
		}
		else debug = 1;
	}
}

static int som_keys_composite(SOM::KEY::Model *in, SHORT instance[4])
{
	if(!in||!in->composite) return 0;

	if(1) //linear search
	{
		for(size_t i=0;i<in->composite->instances;i++)
		if(!memcmp(instance,in->composite->at[i].instance,6)) return i;
		return 0;
	}

	//binary search needs a bit more work

	int sz = in->composite->instances;
	int cmp, first = 0, last = sz-1, stop = -1;

	int x;
	for(x=(last+first)/2;x!=stop;x=(last+first)/2)
	{
		int xx = in->composite->at[x].instance[3];
		cmp = memcmp(in->composite->at[xx].instance,instance,6); 
		
		if(!cmp) return xx;
		if(cmp>0) stop = last = x; else stop = first = x;
	}	   
	if(last==sz-1&&x==last-1) //round-off error
	{
		int xx = in->composite->at[last].instance[3];
		if(!memcmp(in->composite->at[xx].instance,instance,6))
		return xx;
	}	   
	return in->composite->at[x].instance[3];
}

static const SOM::KEY::Model *som_keys_instance(SOM::KEY::Model *inout, const SOM::KEY::BlendedFVF *verts, const WORD *prims)
{		
	if(!inout||!verts||!prims) return 0;

	inout->newinstance = false; //debugging

	//note: soft animations are not in real world coordinates
	//lowering resolution of soft animations for better convergence
	float scale = max(inout->radius,inout->height)>20?0.1f:1024;

	const SOM::KEY::BlendedFVF &instance = verts[*prims];
	SHORT in[4] = {instance.x*scale,instance.y*scale,instance.z*scale,0};
	if(*(DWORD*)in==*(DWORD*)inout->instance&&in[2]==inout->instance[2]) 
	return inout;

	int i = som_keys_composite(inout,in);

	if(inout->composite)
	if(*(DWORD*)in==*(DWORD*)inout->composite->at[i].instance)
	{
		if(in[2]==inout->composite->at[i].instance[2])
		{	
			memcpy(inout->instance,inout->composite->at[i].instance,sizeof(SHORT)*4);
			memcpy(inout->center,inout->composite->at[i].center,sizeof(FLOAT)*8); 

			return inout;
		}
	}

	if(!inout->composite)
	{
		size_t bytes = inout->composite->footprint(4);

		(void*&)inout->composite = new BYTE[bytes];

		inout->composite->bytecheck = inout->composite->footprint(0);
		inout->composite->instances = 1; inout->composite->allocated = 4;

		memcpy(inout->composite->at[0].instance,inout->instance,sizeof(SHORT)*4);
		memcpy(inout->composite->at[0].center,inout->center,sizeof(FLOAT)*8); 

		inout->composite->at[0].instance[3] = 0; //binary indexing
	}
	else if(inout->composite->instances==inout->composite->allocated)
	{
		void *swap = inout->composite;
		int i = inout->composite->instances;

		size_t oldsize = inout->composite->footprint(i);
		size_t newsize = inout->composite->footprint(i*2);

		(void*&)inout->composite = new BYTE[newsize];
		memcpy(inout->composite,swap,oldsize);
		inout->composite->allocated = i*2;
		delete[] swap;
	}
	
	som_keys_extents(inout,verts,prims); 

	memcpy(inout->instance,in,sizeof(SHORT)*3);	

	size_t k, j=inout->composite->instances++;		
	for(k=0;k<inout->composite->instances;k++)
	if(inout->composite->at[k].instance[3]==i) 
	break;

	if(memcmp(inout->composite->at[i].instance,in,6)>0) k++;

	//TODO: shift in whichever direction is less work

	for(size_t l=j;l>k;l--) 
	inout->composite->at[l].instance[3] = inout->composite->at[l-1].instance[3];
	inout->composite->at[k].instance[3] = j;

	//sorted
	inout->instance[3] = inout->composite->at[j].instance[3]; 
		
	inout->composite->structure = inout->composite->footprint(j+1);

	memcpy(inout->composite->at[j].instance,inout->instance,sizeof(SHORT)*4);
	memcpy(inout->composite->at[j].center,inout->center,sizeof(FLOAT)*8); 
		
	if(!som_keys_readonly(inout)) //update registry
	{
		EX::INI::Keygen kg;
		if(kg->keygen_model_mode=='on'
		 &&kg->do_enable_keygen_automation
		 &&kg->do_enable_keygen_instantiation)
		{
			size_t bytesize = inout->composite->structure;		
			som_keys_update(inout,L"composite",(BYTE*)inout->composite,bytesize);
		}
	}

	inout->newinstance = true; //debugging/visualization

	return inout;
}

static void som_keys_auto(SOM::KEY::Image *in, IDirect3DTexture9 *p)
{
	if(!in||!in->hash||in->complete()
	 ||!SOM::KEY::Image::key||SOM::KEY::Image::err) return;

	unsigned in__flags__ = in->flags(); //unioned with parity!!

	bool auditing = false;

	CoCreateGuid(&in->audit);

	D3DSURFACE_DESC desc; D3DLOCKED_RECT lock;

	if(p->GetLevelDesc(0,&desc)!=D3D_OK) return;
	
	switch(desc.Format)
	{
	case D3DFMT_X8R8G8B8: desc.Format = D3DFMT_A8R8G8B8; break;
	case D3DFMT_X1R5G5B5: desc.Format = D3DFMT_A1R5G5B5; break;
	}

	if(p->LockRect(0,&lock,0,D3DLOCK_READONLY)!=D3D_OK) return;

	in->parity = som_keys_parity(desc,lock);
	in->checksum = som_keys_checksum(desc,lock);
	in->dxformat = desc.Format;
	in->texelsuv[0] = desc.Width;
	in->texelsuv[1] = desc.Height;
	
	EX::INI::Keygen kg;
	const wchar_t *filter = L"public";	
	if(*kg->keygen_automatic_filter)
	filter = kg->keygen_automatic_filter;

	if(kg->do_enable_keygen_automation)
	if(SOM::KEY::Image::key&&!SOM::KEY::Image::err)
	{
		const char *name = som_keys_hex<char>(in->hash);

		HKEY top; LONG err = 
		RegCreateKeyExA(SOM::KEY::Image::key,name,0,0,0,KEY_ALL_ACCESS,0,&top,0);
		if(err==ERROR_SUCCESS)
		{				
			HKEY sub = 0; err = 
			RegCreateKeyExW(top,filter,0,0,0,KEY_ALL_ACCESS,0,&sub,0);
			
			if(err==ERROR_SUCCESS) 
			{
				BYTE set[256] = {0}; DWORD szset = 0;

				szset = StringFromGUID2(in->audit,(wchar_t*)set,128);

				RegSetValueExW(sub,L"audit",0,REG_SZ,set,szset*2+2);
				RegSetValueExA(sub,"parity",0,REG_DWORD,(BYTE*)&in->parity,4);
				RegSetValueExA(sub,"checksum",0,REG_DWORD,(BYTE*)&in->checksum,4);
				assert(&in->texelsuv[2]-&in->dxformat==3&&sizeof(DWORD)==4);
				RegSetValueExA(sub,"vitals",0,REG_BINARY,(BYTE*)&in->dxformat,12);

				RegCloseKey(sub);
				auditing = true;
			}			
			RegCloseKey(top);
		}
	}
		
	if(!som_keys_initial_image_t.section) //assuming uninitialized
	{	
		som_keys_initial_image_t.section  = SOM::KEY::newvalue;
		som_keys_initial_image_t.audit    = SOM::KEY::newvalue;
		som_keys_initial_image_t.parity   = SOM::KEY::newvalue;
		som_keys_initial_image_t.checksum = SOM::KEY::newvalue;		
		som_keys_initial_image_t.dxformat = SOM::KEY::newvalue;
		som_keys_initial_image_t.texelsuv = SOM::KEY::newvalue;
	};

	const SOM::KEY::image_t *pub = 0, *prv = 0, *prj = 0;
	if(!wcscmp(filter,L"public")) pub = &som_keys_initial_image_t;
	else if(!wcscmp(filter,L"private")) prv = &som_keys_initial_image_t;
	else prj = &som_keys_initial_image_t;
	
	if(in__flags__&SOM_KEYS_INI_FLAG)
	if(SOM::KEY::Image::ini&&!SOM::KEY::Image::bad)
	{
		DWORD paranoia = 0x0CD;
		wchar_t *p = som_keys_buffer<wchar_t>('.ini');
		size_t sz = som_keys_buffersz<wchar_t>('.ini',p); 
		const wchar_t *name = som_keys_hex<wchar_t>(in->hash,in->ambiguous);
		while(paranoia=GetPrivateProfileSectionW(name,p,sz,SOM::KEY::Image::ini))
		{
			if(paranoia==65535) //docs: maximum permitted
			{
				assert(0); //unimplemented: will require a proper ini library

			  //// TODO: major warning ////////////////////////////////////////

				paranoia = 0; break; //short circuit the ini file
			}
			else if(paranoia==sz-2) //buffer too small
			{
				p = som_keys_buffer<wchar_t>('.ini',2.0f); //double

				sz = som_keys_buffersz<wchar_t>('.ini',p);
			}
			else break;
		}

		if(paranoia!=0)
		{	
			SOM::KEY::image_t tmp1, tmp2; 

			if(pub) memcpy(&tmp1,pub,sizeof(tmp1));
			if(prv) memcpy(&tmp2,prv,sizeof(tmp2));

			if(!pub) memset(&tmp1,0x00,sizeof(tmp1));
			if(!prv) memset(&tmp2,0x00,sizeof(tmp2));

			void *f = SOM::KEY::Image::initialize, *g = SOM::KEY::Image::inicpy;

			som_keys_init(name,paranoia,tmp1,tmp2,f,g);		

			pub = SOM::KEY::Image::copy(tmp1);
			prv = SOM::KEY::Image::copy(tmp2);
		}
	}

	new (in) SOM::KEY::Image(prj,prv,pub);

	if(auditing) 
	som_keys_audit(in,lock);

	p->UnlockRect(0);	
}

template<typename T>
static void som_keys_auto(SOM::KEY::Model *in, const T *verts, DWORD m, const WORD *prims, DWORD n)
{
	if(!in||!in->hash||in->complete()) return;

	//if(!SOM::KEY::Model::key||SOM::KEY::Model::err) return;

	unsigned in__flags__ = in->flags(); //unioned with parity!!

	if(T::fvf==SOM_KEYS_FVF_UNLIT) //assuming map tiles
	{	
/*		for(int i=0;i<n;i++)
		{
			if(i%54==0)
			{
				EXLOG_LEVEL(-1) << "===================================\n";
			}
			else if(i%3==0) EXLOG_LEVEL(-1) << '\n';

			EXLOG_LEVEL(-1) << verts[prims[i]].u << "u " << verts[prims[i]].v << "v\n";
		}

		EXLOG_LEVEL(-1) << "-----------------------------------\n";
*/
		size_t i;
		for(i=3;i<n;i+=3) //per triangle
		if(n%i==0&&!som_keys_uvscmp(verts,prims+i-3,prims+n-3,3))
		{
			if(!som_keys_uvscmp(verts,prims,prims+n-i,i) 
			  &&!som_keys_xyzcmp(verts,prims,prims+n-i,i)) break;
		}		

		m/=n/i;	n = i; //discard after first tile
	}

	bool auditing = false;

	CoCreateGuid(&in->audit);

	short *p = som_keys_prepare(verts,prims,n);

	in->parity = som_keys_parity(p);
	in->checksum = som_keys_checksum(p);
	in->fvformat = T::fvf;
	in->vertices = m;
	in->polygons = n/3; assert(n%3==0);

	//careful: check requirements
	som_keys_extents(in,verts,prims); 

	//note: soft animations are not in real world coordinates
	//lowering resolution of soft animations for better convergence
	float scale = max(in->radius,in->height)>20?0.1f:1024;

	in->instance[0] = verts[*prims].x*scale;
	in->instance[1] = verts[*prims].y*scale;
	in->instance[2] = verts[*prims].z*scale;
	in->instance[3] = 0;

	if(T::fvf==SOM_KEYS_FVF_UNLIT) //fyi: unused
	som_keys_rotate(verts,prims,n,in->instance[0],in->instance[2]);

	in->composite = 0;

	EX::INI::Keygen kg;
	const wchar_t *filter = L"public";			
	if(*kg->keygen_automatic_filter)
	filter = kg->keygen_automatic_filter;

	if(kg->keygen_model_mode=='on'
	  &&kg->do_enable_keygen_automation
	  &&SOM::KEY::Model::key&&!SOM::KEY::Model::err)
	{
		const char *name = som_keys_hex<char>(in->hash);

		HKEY top; LONG err = 
		RegCreateKeyExA(SOM::KEY::Model::key,name,0,0,0,KEY_ALL_ACCESS,0,&top,0);

		if(err==ERROR_SUCCESS)
		{				
			HKEY sub = 0; err = //TODO: initialize to private/project option				
			RegCreateKeyExW(top,filter,0,0,0,KEY_ALL_ACCESS,0,&sub,0);
			
			if(err==ERROR_SUCCESS) 
			{
				BYTE set[256] = {0}; DWORD szset = 0;

				szset = StringFromGUID2(in->audit,(wchar_t*)set,128);

				RegSetValueExW(sub,L"audit",0,REG_SZ,set,szset*2+2);
				RegSetValueExA(sub,"parity",0,REG_DWORD,(BYTE*)&in->parity,4);
				RegSetValueExA(sub,"checksum",0,REG_DWORD,(BYTE*)&in->checksum,4);
				assert(&in->polygons+1-&in->fvformat==3);
				RegSetValueExA(sub,"vitals",0,REG_BINARY,(BYTE*)&in->fvformat,12);
				assert(&in->margin[3]-in->center==8&&sizeof(FLOAT)==4);
				RegSetValueExA(sub,"extent",0,REG_BINARY,(BYTE*)in->center,32);	 
				RegSetValueExA(sub,"instance",0,REG_BINARY,(BYTE*)in->instance,8);
								
				RegCloseKey(sub);
				auditing = true;
			}			
			RegCloseKey(top);
		}
	}
		
	if(!som_keys_initial_model_t.section) //assuming uninitialized
	{	
		som_keys_initial_model_t.section  = SOM::KEY::newvalue;
		som_keys_initial_model_t.audit    = SOM::KEY::newvalue;
		som_keys_initial_model_t.parity   = SOM::KEY::newvalue;
		som_keys_initial_model_t.checksum = SOM::KEY::newvalue;		
		som_keys_initial_model_t.fvformat = SOM::KEY::newvalue;
		som_keys_initial_model_t.vertices = SOM::KEY::newvalue;
		som_keys_initial_model_t.polygons = SOM::KEY::newvalue;
		som_keys_initial_model_t.center   = SOM::KEY::newvalue;
		som_keys_initial_model_t.radius   = SOM::KEY::newvalue;
		som_keys_initial_model_t.height   = SOM::KEY::newvalue;
		som_keys_initial_model_t.margin   = SOM::KEY::newvalue;
		som_keys_initial_model_t.instance = SOM::KEY::newvalue;
	};

	const SOM::KEY::model_t *pub = 0, *prv = 0, *prj = 0;

	if(!wcscmp(filter,L"public")) pub = &som_keys_initial_model_t;
	else if(!wcscmp(filter,L"private")) prv = &som_keys_initial_model_t;
	else prj = &som_keys_initial_model_t;
	
	if(in__flags__&SOM_KEYS_INI_FLAG)
	if(SOM::KEY::Model::ini&&!SOM::KEY::Model::bad)
	{
		DWORD paranoia = 0x0CD;
		wchar_t *p = som_keys_buffer<wchar_t>('.ini');
		size_t sz = som_keys_buffersz<wchar_t>('.ini',p); 
		const wchar_t *name = som_keys_hex<wchar_t>(in->hash,in->ambiguous);
		while(paranoia=GetPrivateProfileSectionW(name,p,sz,SOM::KEY::Model::ini))
		{
			if(paranoia==65535) //docs: maximum permitted
			{
				assert(0); //unimplemented: will require a proper ini library

			  //// TODO: major warning ////////////////////////////////////////

				paranoia = 0; break; //short circuit the ini file
			}
			else if(paranoia==sz-2) //buffer too small
			{
				p = som_keys_buffer<wchar_t>('.ini',2.0f); //double
				sz = som_keys_buffersz<wchar_t>('.ini',p);
			}
			else break;
		}

		if(paranoia!=0)
		{	
			SOM::KEY::model_t tmp1, tmp2; 

			if(pub) memcpy(&tmp1,pub,sizeof(tmp1));
			if(prv) memcpy(&tmp2,prv,sizeof(tmp2));
			if(!pub) memset(&tmp1,0x00,sizeof(tmp1));
			if(!prv) memset(&tmp2,0x00,sizeof(tmp2));

			void *f = SOM::KEY::Model::initialize, *g = SOM::KEY::Model::inicpy;

			som_keys_init(name,paranoia,tmp1,tmp2,f,g);		

			pub = SOM::KEY::Model::copy(tmp1);
			prv = SOM::KEY::Model::copy(tmp2);
		}
	}

	new (in) SOM::KEY::Model(prj,prv,pub);

	if(auditing) 
	som_keys_audit(in,verts,prims);
}

static bool som_keys_next(const wchar_t *in, int &out, const char *seps=",xX")
{
	for(int out=0;in[out];out++)	
	for(int j=0;seps[j];j++) if(in[out]==seps[j]) 
	{
		for(out++;in[out]&&(in[out]==' '||in[out]=='\t');out++); 

		return true; //TODO: more space characters
	}
	return false;
}

static const wchar_t *som_keys_genpath(const wchar_t *p, const wchar_t *q, int priority=1)
{
	wchar_t *out = 0;

	static wchar_t buf1[MAX_PATH] = {0}, *out1 = 0; static int file1 = 0; 
	static wchar_t buf2[MAX_PATH] = {0}, *out2 = 0;	static int file2 = 0;

	switch(priority)
	{
	case 1: out = out1; break;
	case 2:	out = out2; break;

	default: return 0;
	}

	if(!out) //initialize buffers
	{
		EX::INI::Keygen kg;

		switch(priority)
		{
		case 1:
												
			if(som_keys_debugging())
			{
				wcscpy(buf1,EX::cd());	
				//wcscpy(buf2,som_db::project);
				wcscat_s(buf1,MAX_PATH,L"\\DATA\\KEY");

				if(!PathFileExistsW(buf1))
				{							
					//wcscpy(buf2,som_db::install);
					GetEnvironmentVariableW(L".NET",buf1,MAX_PATH);
					wcscat_s(buf1,MAX_PATH,L"\\data\\key");
					if(PathFileExistsW(buf1))
					file1 = wcslen(buf1); 					
					else *buf1 = '\0';
				}
				else file1 = wcslen(buf1); 				
			}
			else if(kg->do_somdb_keygen_defaults)
			{
				GetEnvironmentVariableW(L".NET",buf1,MAX_PATH);
				wcscat_s(buf1,MAX_PATH,L"\\data\\key");
				if(PathFileExistsW(buf1))
				file1 = wcslen(buf1);				
				else *buf1 = 0;
			}
			else
			{
				if(*kg->keygen_audit_folder) 				
				wcscpy(buf1,kg->keygen_audit_folder);
				else wcscpy(buf1,L"DATA\\KEY");
				if(PathFileExistsW(buf1))
				file1 = wcslen(buf1); 
				else *buf1 = 0;
			}		 	 	
			out1 = buf1;
			out = out1;
			break;

		case 2:

			if(som_keys_debugging())
			{
				wcscpy(buf2,EX::cd()); 	
				//wcscpy(buf2,som_db::project);
				wcscat_s(buf2,MAX_PATH,L"\\DATA\\KEY");	 
				if(PathFileExistsW(buf2))
				{
					//wcscpy(buf2,som_db::install);
					GetEnvironmentVariableW(L".NET",buf1,MAX_PATH);					
					wcscat_s(buf2,MAX_PATH,L"\\data\\key");
					if(PathFileExistsW(buf2))
					file2 = wcslen(buf2); 					
					else *buf2 = 0;
				}
				else *buf2 = 0;				
			}
			else if(kg->do_somdb_keygen_defaults||*kg->keygen_audit_folder)
			{
				wcscpy(buf2,L"DATA\\KEY");				
				if(PathFileExistsW(buf2))
				file2 = wcslen(buf2); 
				else *buf2 = 0;
			}				
			out2 = buf2;
			out = out2;
			break;
		}
	}

	//assuming not cwd
	if(!out||!*out) return 0; 

	int file = out==out1?file1:file2;

	if(file+1+(q-p)+1>MAX_PATH) //won't fit
	{
		return 0; //TODO: issue warning
	}

	out[file] = '\\';  
	memcpy(out+file+1,p,(q-p)*sizeof(wchar_t));
	out[file+1+(q-p)] = '\0';

	return out;
}

static bool som_keys_dot(const wchar_t *in, bool &out, size_t _=0)
{									 
	if(!in||!*in) return false; out = *in=='.'; return true;
}

static bool som_keys_hint(const BYTE *in, const unsigned *&out, size_t outsz)
{									 
	assert(0); return true; //unimplemented
}

static bool som_keys_hint(const wchar_t *in, const unsigned *&out, size_t _=0)
{									 
	assert(0); return true; //unimplemented
}

template<typename T>
static bool som_keys_int(const wchar_t *in, T &out, size_t outsz=sizeof(T))
{
	if(outsz!=sizeof(unsigned)) return false;

	int sign = *in=='+'||*in=='-';

	if(!in||!*in||in[sign]<'0'||in[sign]>'9') return false;

	out = wcstoul(in,0,10); return true;
}

template<typename T>
static bool som_keys_int(const wchar_t *in, T *out, size_t outsz)
{
	assert(outsz%sizeof(unsigned)==0);

	int n = outsz/sizeof(unsigned); assert(n<=3);

	if(n>3) return false; //paranoia
	
	if(!in||!*in) return false;

	for(int i=0,_=0;i<n;i++) //cursor
	{
		if(in[_]<'0'||in[_]>'9') return false;
		out[n] = wcstod(in+_,0); 		
		if(!som_keys_next(in,_)) return false;
	}	
	return true;
}

static bool som_keys_float(const wchar_t *in, float &out, size_t _=0)
{
	if(!in||!*in) return false;
	
	int sign = *in=='+'||*in=='-';
	if(in[sign]<'0'||in[sign]>'9') return false;

	out = wcstod(in,0); return true;
}

static bool som_keys_float(const wchar_t *in, float *out, size_t outsz)
{
	assert(outsz%sizeof(float)==0);

	int n = outsz/sizeof(float); assert(n<=3);

	if(n>3) return false; //paranoia

	if(!in||!*in) return false;
	
	for(int i=0,_=0;i<n;i++) //cursor
	{
		int sign = in[_]=='+'||in[_]=='-';		
		if(in[_+sign]<'0'||in[_+sign]>'9') return false;
		out[n] = wcstod(in+_,0); 		
		if(!som_keys_next(in,_)) return false;
	}	
	return true;
}

static inline bool som_keys_guid(const wchar_t *in, GUID &out, size_t _=0)
{
	if(!in||*in!='{') return false;
	
	//docs: StringFromCLSID calls StringFromGUID2  
	return CLSIDFromString((wchar_t*)in,&out)==NOERROR;	
}

static inline bool som_keys_value(const wchar_t *in, const wchar_t* &out, size_t _=0)
{
	if(!in||!*in) return false;

	out = wcsdup(in); //warning: slightly promiscuous

	return true;
}

template<typename T>
static bool som_keys_struct(const BYTE *in, T* &out, size_t insize=-1)
{
	if(!in||!T::struct_compiler_assert) return false;

	if(insize>4) //assuming sizeof out was not passed / testing sizeof in
	{
		if(((SOM::KEY::Struct*)in)->structure!=insize) return false;
	}

	out = (T*)new BYTE[((SOM::KEY::Struct*)in)->structure];

	memcpy(out,in,((SOM::KEY::Struct*)in)->structure);

	return true;
}

template<typename T>
static bool som_keys_struct(const wchar_t *in, T* &out, size_t _=0)
{
	if(!in||!T::struct_compiler_assert) return false;

	//TODO: simulate Write/ReadPrivateProfileStruct()

	assert(0); return false; //unimplemented
}

static bool som_keys_d3dtaddress(const wchar_t *in, DWORD &out, size_t _=0)
{
	if(!in||!*in) return false;

	if(*in>='0'&&*in<='9') //optimal case
	{
		out = wcstoul(in,0,10); return true;
	}

	out = 0;

	switch(*in)
	{
	case 'W': if(!wcscmp(in,L"WRAP"))   out = 1; break;
	case 'M': if(!wcscmp(in,L"MIRROR")) out = 2; break;
	case 'C': if(!wcscmp(in,L"CLAMP"))  out = 3; break;
	case 'B': if(!wcscmp(in,L"BORDER")) out = 4; break;	
	}

	if(out) return true;

	switch(*in) //second tier
	{
	case 'M': if(!wcscmp(in,L"MIRRORONCE")) out = 5; break;
	}

	if(!out) return false;

	return true;
}

template<typename Tin, typename Tout>
static inline void som_keys_default(Tin in, Tout &out, size_t _=0)
{
	out = in;
}

template<typename Tin, typename Tout>
static inline void som_keys_default(Tin in, Tout *out, size_t outsz)
{
	assert(out&&outsz%sizeof(unsigned)==0);
	unsigned n = outsz/sizeof(unsigned); assert(n<=3);
	for(size_t i=0;i<n;i++) out[i] = in;
}

template<typename T>
static inline void som_keys_default(int in, T* &out, size_t outsz)
{
	if(in==0) out = 0; assert(in==0&&outsz==sizeof(void*));
}

template<typename T>
static inline void som_keys_default(T *in, T* &out, size_t outsz)
{
	out = in; assert(outsz==sizeof(void*));
}

static inline void som_keys_default(int in, GUID &out, size_t _=0)
{
	if(in==0) memset(&out,0x00,sizeof(out)); assert(in==0);
}

const SOM::KEY::Image *SOM::KEY::image(IDirect3DTexture9 *in, SOM::KEY::Index **indexing)
{
	SOM::KEY::Image *out = 0; int lv = 0; lvUP:

	while(out=som_keys_image(som_keys_hash(in,lv++,out)))		
	if(!out->ambiguous) break; 
		
	if(!out) //hashing error
	{
		if(som_keys_hash_overflow) //using best match
		{
			out = som_keys_image(som_keys_hash(in));

			if(out&&!out->erroneous)
			{
				return out->complete()?out:0; 
			}
			else return 0;
		}
	}
	else if(out->erroneous) return 0;

	if(out&&out->complete()) 
	{					 
		som_keys_hit(out); 
		if(som_keys_readonly(out)) return out;
		if(som_keys_compare(in,out->parity,out->checksum)) 
		{
			return out;
		}		
		if(!som_keys_ambiguate(out)) return 0; goto lvUP;
	}
	else if(out) 
	{
		som_keys_auto(out,in); 
		som_keys_miss(out); 
	}	

	return out;
}

const SOM::KEY::Model *SOM::KEY::model(const SOM::KEY::UnlitFVF *verts, DWORD m, const WORD *prims, DWORD n, SOM::KEY::Index **indexing)
{
	if(!verts||!m||!prims||n<=3||n%3) return 0; //avoiding control points??

	SOM::KEY::Model *out = 0; int lv = 0; lvUP:
	while(out=som_keys_model(som_keys_hash(verts,m,prims,n,lv++,out)))		
	if(!out->ambiguous) break; 
		
	if(!out) //hashing error
	{
		if(som_keys_hash_overflow) //using best match
		{
			out = som_keys_model(som_keys_hash(verts,m,prims,n));

			if(out&&!out->erroneous)
			{
				return out->complete()?out:0; 
			}
			else return 0;
		}
	}
	else if(out->erroneous) return 0;

	if(out&&out->complete()) 
	{	
		som_keys_hit(out); 
		
//		if(som_keys_readonly(out)) return out;
		
		if(m%out->vertices==0&&n%(out->polygons*3)==0)	 
		{
			int N = 1; //test N models per frame
			int metric = som_keys_metrics(out)/N+1;

			if(som_keys_rand(out)%metric) return out; 
						
			int r = som_keys_rand(out), x = out->polygons*3;

			const WORD *p = prims+r%(n/x)*x; //pick a random tile

			//TODO: implement random/partial audit regime
			if(som_keys_compare(verts,p,x,out->parity,out->checksum)) 
			{
				 return out;				
			}
		}
		
		if(!som_keys_ambiguate(out)) return 0;

		goto lvUP;
	}
	else if(out)
	{
		som_keys_auto(out,verts,m,prims,n); 
		som_keys_miss(out);
	}

	return out;
}

const SOM::KEY::Model *SOM::KEY::model(const SOM::KEY::BlendedFVF *verts, DWORD m, const WORD *prims, DWORD n, SOM::KEY::Index **indexing)
{
	if(!verts||!m||!prims||n<3||n%3) return 0; //paranoia

	SOM::KEY::Model *out = 0; int lv = 0; lvUP:

	while(out=som_keys_model(som_keys_hash(verts,m,prims,n,lv++)))		
	if(!out->ambiguous) break; 
		
	if(!out) //hashing error
	{
		if(som_keys_hash_overflow) //using best match
		{
			//assert(!"hash_overflow"); //debugging

			out = som_keys_model(som_keys_hash(verts,m,prims,n));

			if(out->vertices!=m||out->polygons!=n/3) out = 0;

			if(out&&!out->erroneous)
			{
				return out->complete()?som_keys_instance(out,verts,prims):0; 
			}
			else return 0;
		}
	}
	else if(out->erroneous) return 0;

	if(out&&out->complete()) 
	{			
		som_keys_hit(out); 
		
		if(som_keys_readonly(out)) 		
		return som_keys_instance(out,verts,prims);
				
		if(m==out->vertices&&n==out->polygons*3)
		{
			int N = 1; //test N models per frame
			int metric = som_keys_metrics(out)/N+1;

			if(som_keys_rand(out)%metric
			  ||som_keys_compare(verts,prims,n,out->parity,out->checksum)) 
			{	
				return som_keys_instance(out,verts,prims);
			}	
		}

		if(!som_keys_ambiguate(out)) return 0;
		
		goto lvUP;
	}
	else if(out)
	{
		som_keys_auto(out,verts,m,prims,n); 
		som_keys_miss(out);
	}

	return som_keys_instance(out,verts,prims);
}

extern const SOM::KEY::FogFVF &SOM::KEY::center(const SOM::KEY::Model *in, const SOM::KEY::UnlitFVF *verts, const WORD *prims)
{
	static SOM::KEY::FogFVF out; 
	
	if(in&&verts&&prims&&in->fvformat==SOM_KEYS_FVF_UNLIT) //paranoia
	{
		out.x = in->center[0];
		out.y = in->center[1];
		out.z = in->center[2];

		som_keys_revert(verts,prims,in->polygons*3,out.x,out.z);

		out.x+=verts[*prims].x;
		out.y+=verts[*prims].y;
		out.z+=verts[*prims].z;
	}
	else 
	{
		assert(0); out.x = out.y = out.z = 0.0f;
	}

	return out;
}

extern const wchar_t *SOM::KEY::genpath(const wchar_t *begin, const wchar_t *end)
{
	return som_keys_genpath(begin,end);
}
extern const wchar_t *SOM::KEY::genpath2(const wchar_t *begin, const wchar_t *end)
{
	return som_keys_genpath(begin,end,2);
}

template<typename t>
inline SOM::KEY::Class<t>::Class(const char *hex) //explicit
:
erroneous(0), ambiguous(0), hint(0), _project(0), _private(0), _public(0)
{
	const unsigned *p = som_keys_hash(hex,true);
	
#ifdef NDEBUG

		hash = !p?0:(unsigned*)memcpy(new int[*p],p,*p*sizeof(int));
#else
		*hash = 0; if(p) memcpy(hash,p,*p*sizeof(int));
#endif

	flags() = 0x00000000;
}

template<typename t>
inline SOM::KEY::Class<t>::Class(const wchar_t *hex) //explicit 
:
erroneous(0), ambiguous(0), hint(0), _project(0), _private(0), _public(0)
{
	const unsigned *p = som_keys_hash(hex,true);

#ifdef NDEBUG

		hash = !p?0:(unsigned*)memcpy(new int[*p],p,*p*sizeof(int));
#else 	
		*hash = 0; if(p) memcpy(hash,p,*p*sizeof(int));
#endif

	flags() = 0x00000000;
}

template<typename t>
inline SOM::KEY::Class<t>::Class(const unsigned *copy) //explicit
:
erroneous(0), ambiguous(0), hint(0), _project(0), _private(0), _public(0)
{
	if(copy&&*copy&&*copy<=SOM_KEYS_MAX_HASH)
	
#ifdef NDEBUG

		hash = (unsigned*)
		memcpy(new int[*copy],copy,*copy*sizeof(int));	
		else hash = 0;
#else
		memcpy(hash,copy,*copy*sizeof(int));
		else *hash = 0;
#endif

	flags() = 0x00000000;
}

template<typename t>
SOM::KEY::Class<t>::Class(const t *prj, const t *prv, const t *pub, size_t sz)
:
_project(prj), _private(prv), _public(pub), _union(0)
{
	if(!complete()||!hash) memset(this,0x00,sz); //prepare for dtor
}

template<typename t>
SOM::KEY::Class<t>::~Class()
{
#ifdef NDEBUG

	delete[] hash;  

#endif

	if(safe_to_delete(_union)) delete _union;
	if(safe_to_delete(_project)) delete _project;
	if(safe_to_delete(_private)) delete _private;
	if(safe_to_delete(_public)) delete _public;
}

template<typename t>
inline bool SOM::KEY::Class<t>::complete()
{
	return _project||_private||_public;
}

template<typename t>
inline const wchar_t *SOM::KEY::Class<t>::setting(int i)
{
	if(_project&&(*_project)[i]) return (*_project)[i]; 
	if(_private&&(*_private)[i]) return (*_private)[i]; 

	return _public?(*_public)[i]:0;
}				  

template<typename t>
inline bool SOM::KEY::Class<t>::filter(const wchar_t *f, int i, const wchar_t *in)
{
	if(!wcscmp(f,L"public"))
	{
		if(!_public) return false;
		(*_public)[i] = in;
	}
	else if(!wcscmp(f,L"private"))
	{
		if(!_private) return false;
		(*_private)[i] = in;
	}
	else //assuming project
	{
		if(!_project) return false;
		(*_project)[i] = in;
	}
	return true;
}

template<typename t>
inline int SOM::KEY::Class<t>::select(int i)
{
	if(_project&&(*_project)[i]) return 2; 
	
	return _private&&(*_private)[i]?1:0;
}

template<typename t>
inline unsigned &SOM::KEY::Class<t>::flags()
{
	return parity; //economics
}

template<typename t>
bool SOM::KEY::Class<t>::keys(HKEY &top, HKEY keys[3])
{
	typedef SOM::KEY::Class<t> T;

	const int n = sizeof(t)/sizeof(void*);

	top = 0; keys[0] = keys[1] = keys[2] = 0; 

	if(!T::key||T::err) return false;

	bool out = false; LONG err; 

	for(int i=0;i<3;i++)
	{
		const t *p = 0; const wchar_t *which = 0;

		switch(i)
		{
		case 0: p = _public; which = L"public"; break;
		case 1: p = _private; which = L"private"; break;
		case 2: p = _project; which = som_keys_project;
		}

		if(!p) continue;

		bool bother = false;

		const wchar_t **q = *p;

		for(int j=0;j<n;j++) 
		if(q[j]==SOM::KEY::regvalue)
		{
			bother = true; break;
		}

		if(!bother) continue;

		if(!top)
		{
			err = RegOpenKeyExA(T::key,som_keys_hex<char>(hash),0,KEY_ALL_ACCESS,&top); 
			if(err!=ERROR_SUCCESS) goto err;
		}
					
		err = RegOpenKeyExW(top,which,0,KEY_ALL_ACCESS,&keys[i]); 
		if(err!=ERROR_SUCCESS) goto err;

		out = true;
	}

	return out;

err: if(top) RegCloseKey(top); top = 0;

	for(int i=0;i<3;i++) if(keys[i]) 
	{
		RegCloseKey(keys[i]); keys[i] = 0;
	}

	erroneous = true;

	return false;
}

template<typename t>
inline const t *SOM::KEY::Class<t>::zero()
{
	static t out; 	
	static void *set = memset(&out,0x00,sizeof(t));
	return &out;
}

template<typename t>
inline const t *SOM::KEY::Class<t>::copy(const t &cp)
{
	if(!memcmp(zero(),&cp,sizeof(t))) return 0;

	if(!_default||memcmp(_default,&cp,sizeof(t)))
	{
		return (t*)memcpy(new t,&cp,sizeof(t));
	}
	else return _default;
}

template<typename t>
inline const wchar_t *SOM::KEY::Class<t>::inicpy(const wchar_t *src, size_t chars)
{
	static size_t dst = 0; //just easier to manage this way

	if(bad||!ini||ini+dst+chars>eof) return 0;

	wchar_t *out = (wchar_t*)memcpy((wchar_t*)ini+dst,src,chars*2);

	dst+=chars; return out;
}

template<typename t>
inline bool SOM::KEY::Class<t>::safe_to_delete(const t *p)
{
	if(p==_union)
	{
		return p&&p!=_public&&p!=_private&&p!=_project;
	}
	else return p&&p!=_default&&p!=_initial;
}

SOM::KEY::Image::Image(const t *prj, const t *prv, const t *pub) //SOM::KEY::image_t
:
SOM::KEY::Class<SOM::KEY::image_t>(prj,prv,pub,sizeof(SOM::KEY::Image))
{	
	if(erroneous||!complete()||!hash) return;

	HKEY top, subs[3]; keys(top,subs);	 
	
	LONG err; if(erroneous) return; //keys failed
	
	static size_t getmost = 256;
	static BYTE *getmore = new BYTE[getmost];

	BYTE* &get = getmore; DWORD getsz;
	
	const wchar_t *set; int i = -1;

//Note: the cpp stuff here is only meant to get the job done with minimal code

#define SOM_KEYS_VARSET(f,val,def,get,sz,...) \
\
	set = setting(++i);\
\
	if(set!=SOM::KEY::newvalue)\
	if(set==SOM::KEY::regvalue)\
	{\
		if(sz>getmost){ delete[] getmore; getmore = new BYTE[getmost=sz]; }\
\
		if(sz) err = RegQueryValueExW(SOM_KEYS_VARVAL(val),0,0,(BYTE*)get,&(getsz=(sz)));\
\
		if(err==ERROR_SUCCESS){ __VA_ARGS__; }else erroneous = true;\
	}\
	else if(set)\
	{\
		erroneous = !som_keys_##f(set,val,sizeof(val));\
	}\
	else som_keys_default(def,val,sizeof(val));\
\
	if(erroneous) return;

#define SOM_KEYS_VALSET(f,val,def) SOM_KEYS_VARSET(f,val,def,&val,sizeof(val),0)

#define SOM_KEYS_NOTSET(val) i++;

#define SOM_KEYS_VARVAL(val) top,L#val

	SOM_KEYS_VARSET(dot,ambiguous,0,get,1,ambiguous=get[0])
	SOM_KEYS_VARSET(hint,hint,0,get,256,erroneous=!som_keys_hint(get,hint,getsz))

#define SOM_KEYS_VARVAL(val) subs[select(i)],L#val

	SOM_KEYS_NOTSET(comments)
	SOM_KEYS_VARSET(guid,audit,0,get,256,erroneous=!som_keys_guid((wchar_t*)get,audit))
	SOM_KEYS_VALSET(int,parity,0)
	SOM_KEYS_VALSET(int,checksum,0)
	SOM_KEYS_NOTSET(garbage)

//Note: in the registry dxformat/texelsuv/future stuff is packed into 'vitals'

#define SOM_KEYS_VARVAL(val) subs[select(i)],L"vitals"

	memset(get,0x00,12); getsz = 0; //tricky: pull vitals only once

	SOM_KEYS_VARSET(int,dxformat,0,get,getsz?0:12,dxformat=*(unsigned*)get)
	SOM_KEYS_VARSET(int,texelsuv,0,get,getsz?0:12,memcpy(texelsuv,get+4,8))

#define SOM_KEYS_VARVAL(val) subs[select(i)],L#val

	SOM_KEYS_VALSET(d3dtaddress,addressu,0)
	SOM_KEYS_VALSET(d3dtaddress,addressv,0)
	SOM_KEYS_VALSET(int,lodbias,0)
	SOM_KEYS_VALSET(int,samples,0)
	SOM_KEYS_VARSET(value,correct,0,get,256,erroneous=!som_keys_value((wchar_t*)get,correct))

	for(int i=0;i<3;i++) if(subs[i]) RegCloseKey(subs[i]);

	if(top) RegCloseKey(top);

	corrected = 0;
}

int SOM::KEY::Image::initialize(const wchar_t *in)
{
	switch(*in)
	{
	case 'd': return !wcscmp(in,L"dxformat")?0:-1;
	case 't': return !wcscmp(in,L"texelsuv")?1:-1;
	case 'a': 
				
		if(wcslen(in)!=8) return -1;

		if(in[7]=='u') return !wcscmp(in,L"addressu")?2:-1;
		if(in[7]=='v') return !wcscmp(in,L"addressv")?3:-1;

		return -1;

	case 'l': return !wcscmp(in,L"lodbias")?4:-1;
	case 's': return !wcscmp(in,L"samples")?5:-1;
	case 'c': return !wcscmp(in,L"correct")?6:-1;

	default: return -1; //eg. class_t::garbage
	}
}

const char *SOM::KEY::Image::enumerate(int in)
{
	switch(in)								 
	{
	case 0: //dxformat
	case 1: //texelsuv
			return "vitals";

	case 2: return "addressu";
	case 3: return "addressv";
	case 4: return "lodbias";
	case 5: return "samples"; 
	case 6: return "correct";

	default: return 0;
	}
}

SOM::KEY::Model::Model(const t *prj, const t *prv, const t *pub) //SOM::KEY::model_t
:
SOM::KEY::Class<SOM::KEY::model_t>(prj,prv,pub,sizeof(SOM::KEY::Model))
{ 
	if(erroneous||!complete()||!hash) return;

	HKEY top, subs[3]; keys(top,subs);
	
	LONG err; if(erroneous) return; //keys failed
				
	static size_t getmost = 256;
	static BYTE *getmore = new BYTE[getmost];

	BYTE* &get = getmore; DWORD getsz;
	
	const wchar_t *set; int i = -1;	

#define SOM_KEYS_VARVAL(val) top,L#val

	SOM_KEYS_VARSET(dot,ambiguous,0,get,1,ambiguous=get[0])
	SOM_KEYS_VARSET(hint,hint,0,get,256,erroneous=!som_keys_hint(get,hint,getsz))

#define SOM_KEYS_VARVAL(val) subs[select(i)],L#val

	SOM_KEYS_NOTSET(comments)
	SOM_KEYS_VARSET(guid,audit,0,get,256,erroneous=!som_keys_guid((wchar_t*)get,audit))
	SOM_KEYS_VALSET(int,parity,0)
	SOM_KEYS_VALSET(int,checksum,0)
	SOM_KEYS_NOTSET(garbage)

//Note: in the registry fvformat/vertices/polygons/etc is packed into 'vitals'

#define SOM_KEYS_VARVAL(val) subs[select(i)],L"vitals"

	memset(get,0x00,12); getsz = 0; //tricky: pull vitals only once

	SOM_KEYS_VARSET(int,fvformat,0,get,getsz?0:12,fvformat=*(unsigned*)(get+0))
	SOM_KEYS_VARSET(int,vertices,0,get,getsz?0:12,vertices=*(unsigned*)(get+4))
	SOM_KEYS_VARSET(int,polygons,0,get,getsz?0:12,polygons=*(unsigned*)(get+8))

//Note: in the registry center/radius/height is packed into 'extent'

#define SOM_KEYS_VARVAL(val) subs[select(i)],L"extent"

	memset(get,0x00,12); getsz = 0; //tricky: pull extent only once
	
	*(float*)(get+12) = *(float*)(get+16) = -1; //paranoia: radius/height defaults

	memset(get+20,0x00,12); //margin

	SOM_KEYS_VARSET(float,center, 0,get,getsz?0:32,memcpy(center,get,12))
	SOM_KEYS_VARSET(float,radius,-1,get,getsz?0:32,radius=*(float*)(get+12))
	SOM_KEYS_VARSET(float,height,-1,get,getsz?0:32,height=*(float*)(get+16))
	SOM_KEYS_VARSET(float,margin, 0,get,getsz?0:32,memcpy(margin,get+20,12))

#define SOM_KEYS_VARVAL(val) subs[select(i)],L#val

	SOM_KEYS_VALSET(int,instance,0)
	SOM_KEYS_VARSET(struct,composite,0,get,-1,som_keys_struct(get,composite,getsz))

	if(composite)
	if(composite->bytecheck!=composite->footprint(0)
	 ||composite->structure!=composite->footprint(composite->instances))		
	{
		delete[] composite; composite = 0; //unable to use composite data

		assert(0); //TODO: issue warning
	}

	if(composite) composite->allocated = composite->instances;

	for(int i=0;i<3;i++) if(subs[i]) RegCloseKey(subs[i]);

	if(top) RegCloseKey(top);

	newinstance = false;
}

int SOM::KEY::Model::initialize(const wchar_t *in)
{
	switch(*in)
	{
	case 'f': return !wcscmp(in,L"fvformat")?0:-1;
	case 'v': return !wcscmp(in,L"vertices")?1:-1;
	case 'p': return !wcscmp(in,L"polygons")?2:-1;

	case 'c': if(!wcscmp(in,L"center")) return 3; break; 

	case 'r': return !wcscmp(in,L"radius")?4:-1;
	case 'h': return !wcscmp(in,L"height")?5:-1;
	case 'm': return !wcscmp(in,L"margin")?6:-1;
	case 'i': return !wcscmp(in,L"instance")?7:-1; 
	}	

	switch(*in) //second tier
	{
	case 'c': return !wcscmp(in,L"composite")?8:-1; break;
	}

	return -1; //eg. class_t::garbage
}

const char *SOM::KEY::Model::enumerate(int in)
{
	switch(in)								 
	{
	case 0:	//fvformat
	case 1:	//vertices
	case 2:	//polygons
		    return "vitals";

	case 3: //center
	case 4: //radius
	case 5: //height
	case 6: //margin
		    return "extent";

	case 7: return "instance";
	case 8: return "composite";

	default: return 0;
	}
}

extern void SOM::initialize_som_keys_cpp()
{
	assert(sizeof(SOM::KEY::UnlitFVF)==6*4);
	assert(sizeof(SOM::KEY::BlendedFVF)==8*4);

	memset(&som_keys_initial_image_t,0x00,sizeof(SOM::KEY::image_t)); 
	memset(&som_keys_initial_model_t,0x00,sizeof(SOM::KEY::model_t));
	memset(&som_keys_default_image_t,0x00,sizeof(SOM::KEY::image_t));
	memset(&som_keys_default_model_t,0x00,sizeof(SOM::KEY::model_t));

	if(!som_keys_default_image_t.section) 
	{	
		som_keys_default_image_t.audit    = SOM::KEY::regvalue;
		som_keys_default_image_t.parity   = SOM::KEY::regvalue;
		som_keys_default_image_t.checksum = SOM::KEY::regvalue;		
		som_keys_default_image_t.dxformat = SOM::KEY::regvalue;
		som_keys_default_image_t.texelsuv = SOM::KEY::regvalue;		
	};

	if(!som_keys_default_model_t.section) 
	{	
		som_keys_default_model_t.audit    = SOM::KEY::regvalue;
		som_keys_default_model_t.parity   = SOM::KEY::regvalue;
		som_keys_default_model_t.checksum = SOM::KEY::regvalue;		
		som_keys_default_model_t.fvformat = SOM::KEY::regvalue;
		som_keys_default_model_t.vertices = SOM::KEY::regvalue;
		som_keys_default_model_t.polygons = SOM::KEY::regvalue;
		som_keys_default_model_t.center   = SOM::KEY::regvalue;
		som_keys_default_model_t.radius   = SOM::KEY::regvalue;
		som_keys_default_model_t.height   = SOM::KEY::regvalue;
		som_keys_default_model_t.margin   = SOM::KEY::regvalue;
		som_keys_default_model_t.instance = SOM::KEY::regvalue;
	};

	EX::INI::Keygen kg;
	if(*kg->keygen_automatic_filter) 
	{
		som_keys_project = kg->keygen_automatic_filter;
	}
}

namespace som_keys_squasher
{
    union Bits
    {
        float f;
        __int32 si;
        unsigned __int32 ui;
    };

    static int const shift = 13;
    static int const shiftSign = 16;

    static __int32 const infN = 0x7F800000; // flt32 infinity
    static __int32 const maxN = 0x477FE000; // max flt16 normal as a flt32
    static __int32 const minN = 0x38800000; // min flt16 normal as a flt32
    static __int32 const signN = 0x80000000; // flt32 sign bit

    static __int32 const infC = infN >> shift;
    static __int32 const nanN = (infC + 1) << shift; // minimum flt16 nan as a flt32
    static __int32 const maxC = maxN >> shift;
    static __int32 const minC = minN >> shift;
    static __int32 const signC = signN >> shiftSign; // flt16 sign bit

    static __int32 const mulN = 0x52000000; // (1 << 23) / minN
    static __int32 const mulC = 0x33800000; // minN / (1 << (23 - shift))

    static __int32 const subC = 0x003FF; // max flt32 subnormal down shifted
    static __int32 const norC = 0x00400; // min flt32 normal down shifted

    static __int32 const maxD = infC - maxC - 1;
    static __int32 const minD = minC - subC - 1;

    static unsigned __int16 compress(float value)
    {
		//NOTE: I didn't write this code... I think
		//it's testing undefined behavior (C4804)
		assert(0xFFFFFFFF&-true==~0);
		int compile[0xFFFFFFFF&-true==~0]; //DUPLICATE

        Bits v, s;
        v.f = value;
        unsigned __int32 sign = v.si & signN;
        v.si ^= sign;
        sign >>= shiftSign; // logical shift
        s.si = mulN;
        s.si = s.f * v.f; // correct subnormals
        v.si ^= (s.si ^ v.si) & -(minN > v.si);
        v.si ^= (infN ^ v.si) & -((infN > v.si) & (v.si > maxN));
        v.si ^= (nanN ^ v.si) & -((nanN > v.si) & (v.si > infN));
        v.ui >>= shift; // logical shift
        v.si ^= ((v.si - maxD) ^ v.si) & -(v.si > maxC);
        v.si ^= ((v.si - minD) ^ v.si) & -(v.si > subC);
        return v.ui | sign;
    }

    static float decompress(unsigned __int16 value)
    {
		//assert(0xFFFFFFFF&-true==~0);
		//int compile[0xFFFFFFFF&-true==~0]; //DUPLICATE

        Bits v;
        v.ui = value;
        __int32 sign = v.si & signC;
        v.si ^= sign;
        sign <<= shiftSign;
        v.si ^= ((v.si + minD) ^ v.si) & -(v.si > subC);
        v.si ^= ((v.si + maxD) ^ v.si) & -(v.si > maxC);
        Bits s;
        s.si = mulC;
        s.f *= v.si;
        __int32 mask = -(norC > v.si);
        v.si <<= shift;
        v.si ^= (s.si ^ v.si) & mask;
        v.si |= sign;
        return v.f;
    }
}
