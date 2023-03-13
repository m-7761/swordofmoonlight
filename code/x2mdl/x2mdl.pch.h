
#ifndef X2MDL_PCH_INCLUDED
#define X2MDL_PCH_INCLUDED

#include <assert.h>
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <fstream>

#include <vector> //flush_diffs
#include <unordered_map> //x2mdo

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#define _WIN32_WINNT 0x0500 //GetConsoleWindow

#include <windows.h> //globbing
#include <Wincon.h> //GetConsoleWindow
#include <Shellapi.h> //CommandLineToArgvW
#include <shlwapi.h> //PathIsRelative
#include <Shlobj.h> //SHCreateDirectoryW
#include <Shobjidl.h> //IShellLinkW

#include <d3d9.h>
#include <d3dx9.h>

#ifdef _DEBUG
#define D3D_DEBUG_INFO
#endif 

#include "assimp.h"
#include "aiPostProcess.h"
#include "aiScene.h"
#include "Euler.hpp" //Assimp include

#pragma comment(lib,"assimp.lib")

extern const aiScene *X;

#ifndef _CONSOLE
struct x2mdl_dll
{
	HWND progress;
	int exit_code;
	const wchar_t *exit_reason;	
	const wchar_t *input;
	const wchar_t *cache_dir,*image_suf;
	const wchar_t **data_begin,**data_end;
	IShellLinkW *link; IPersistFile *link2;
	IDirect3DDevice9 *d3d9d;
	wchar_t *output(wchar_t(&buf)[264],const wchar_t*);
	void copy(int,wchar_t[],const wchar_t*);
	void makelink(const wchar_t*);
	bool havelink(const wchar_t*);
};
#define X2MDL_EXE L"x2mdl.dll"
#define X2MDL_YESNO MB_OK|MB_ICONERROR
#else
#define X2MDL_EXE L"x2mdl.exe" 
#define X2MDL_YESNO MB_YESNO|MB_ICONERROR
#endif
extern HWND X2MDL_MODAL;

extern bool x2mdo_makedir(wchar_t*);

namespace MDL
{
#ifdef NDEBUG
	static const int N = 0;
#else
	static const int N = 64;
#endif

	struct Head
	{
		int flags;
		int anims;
		int diffs;
		int skins;
		int parts;
	};

	struct Part
	{
		int verts;
		int extra;
		int norms;	
		int faces;

		//see: consolidate()
		unsigned short *cverts, *cnorms;

		#ifdef _CONSOLE
		int mm3d_cvertsz;
		#endif

		//Mm3dLoader.cpp CP consolidation
		//for MM3D->MDL conversion
		int cextra, cstart, cnode;
		Part *cpart; 
		short *cextrav, *cextran;

		int diff_v0; //REMOVE ME

		Part()
		{
			memset(this,0x00,sizeof(*this));
		}
		~Part()
		{
			delete[] cverts; delete[] cnorms;
		}		
	};

	struct Tile
	{
		int type;
		int size;

		struct X00 //quadrangle
		{

		};

		union //variable arrays
		{
		X00 x00[N];
		};

		template<int T>
		static inline Tile *New(int n)
		{
			switch(T)
			{			
			case 0x00: return (Tile*) new char[sizeof(Tile)+sizeof(X00)*max(n-N,0)];

			default: assert(0); return NULL;
			}
		}
	};

	struct Pack
	{
		int part;
		int type;
		int size;

		struct X00 //flat colored triangles
		{
			unsigned short verts[3];
			unsigned short norms[1];

			unsigned char r,g,b,c; //mode constant (0x20)

			//#ifdef _CONSOLE
			//aiVector3D *mm3d_norm;
			//#endif
		};
		struct X01 //flat shaded triangles
		{
			
		};
		struct X03 //smooth colored triangles
		{
			unsigned short verts[3];
			unsigned short norms[3];

			unsigned char r,g,b,c; //mode constant (0x30)

			#ifdef _CONSOLE
			aiVector3D *mm3d_norms[3];
			#endif
		};
		struct X04 //smooth shaded triangles
		{
			unsigned short comps[3];
			unsigned short verts[3];
			unsigned short norms[3];
			unsigned short flags[3];

			#ifdef _CONSOLE
			aiVector3D *mm3d_norms[3];
			aiVector3D *mm3d_uvs[3];
			#endif
		};
		struct X06 //flat colored quadrangles
		{
			
		};
		struct X07 //flat shaded quadrangles
		{
			
		};
		struct X0A //smooth shaded quadrangles
		{
			
		};
		struct X0D //unlit triangles
		{
			
		};
		struct X11 //unlit quadrangles
		{
			
		};

		union //variable arrays
		{
		X00 x00[N];
		X01 x01[N];
		X03 x03[N];
		X04 x04[N];
		X06 x06[N];
		X07 x07[N];
		X0A x0A[N];
		X0D x0D[N];
		X11 x11[N];
		};

		template<int T>
		static inline Pack *New(int n)
		{
			switch(T)
			{			
			case 0x00: return (Pack*) new char[sizeof(Pack)+sizeof(X00)*std::max(n-N,0)];
			case 0x01: return (Pack*) new char[sizeof(Pack)+sizeof(X01)*std::max(n-N,0)];
			case 0x03: return (Pack*) new char[sizeof(Pack)+sizeof(X03)*std::max(n-N,0)];
			case 0x04: return (Pack*) new char[sizeof(Pack)+sizeof(X04)*std::max(n-N,0)];
			case 0x06: return (Pack*) new char[sizeof(Pack)+sizeof(X06)*std::max(n-N,0)];
			case 0x07: return (Pack*) new char[sizeof(Pack)+sizeof(X07)*std::max(n-N,0)];
			case 0x0A: return (Pack*) new char[sizeof(Pack)+sizeof(X0A)*std::max(n-N,0)];
			case 0x0D: return (Pack*) new char[sizeof(Pack)+sizeof(X0D)*std::max(n-N,0)];
			case 0x11: return (Pack*) new char[sizeof(Pack)+sizeof(X11)*std::max(n-N,0)];

			default: assert(0); return NULL;
			}
		}
	};

	struct Anim
	{
		int type;
		int size;
		int steps;

		unsigned char(*scale)[3];

		/*retiring (REMOVE ME)
		int chans[4]; //128bit inclusion mask		
		inline void setchans(int eg0){ memset(chans,eg0,sizeof(int)*4); }
		bool getmask(int get) //x2mm3d.cpp
		{
			if(get>CHAR_BIT*sizeof(chans)) return 0;
			int i = get/32; get-=i*32;
			return chans[i]&1<<get?true:false;
		}*/

		short info[N][6];

		static inline Anim *New(int n) 
		{
			auto a = (Anim*)new char[sizeof(Anim)+sizeof(short)*6*std::max(n-N,0)];

			a->scale = 0; return a;
		}
		void Delete() //~Anim() wants to destruct and array
		{
			delete[] (char*)scale; delete[] (char*)this;
		}
	};

	struct Diff
	{
		int type;				
		int width;
		int steps;

		__int16 verts[1]; //width*steps

		__int16 *times(){ return verts+width*steps; }

		__int16 *flush(){ return times()+steps; }

		static inline Diff *New(int width, int steps) 
		{
			//2020: zero-initializing just to cover any missing cyan-cp frames
			Diff *o = (Diff*) new char[sizeof(Diff)+2*width*steps+2*2*steps]();			
			o->width = width; assert(width%3==0&&2==sizeof(short));
			o->steps = steps; return o;
		}
	};

	struct Skin
	{
		int index;
		int width; 
		int height;
		
		void *image; //pointed at info???

		char info[N];

		static inline Skin *New(int n) 
		{
			return (Skin*) new char[sizeof(Skin)+sizeof(char)*std::max(n-N,0)];
		}

		struct S1B5G5R5
		{
			unsigned __int16 r:5,g:5,b:5,s:1;
		};

		void X1R5G5B5toS1B5G5R5()
		{
			for(int i=width*height-1;i>=0;i--)
			{
				S1B5G5R5 &texel = ((S1B5G5R5*)image)[i];

				int swap = texel.r; texel.r = texel.b;

				texel.b = swap;	texel.s = 0;
			}
		}
	};

	struct File
	{
		struct x2mdl_dll *dll;

		bool write,error,mm3d;

		wchar_t name[MAX_PATH];

		Head head;
		Part *parts;
		Tile **tiles;
		Pack **packs;
		short *verts;
		short *norms;
		short *chans; 
		Anim **anims;
	//	short *anim4; //???
		Diff **diffs;
		Skin **skins;

		void consolidate(); //verts/norms

		void flush(); //writes file to name

		bool flush_diffs(FILE*,std::vector<short>&); //subroutine

		File(const wchar_t *fname=0)
		{
			memset(this,0x00,sizeof(File));

			if(!fname) return; //goto dll_continue?
			
			if(fname!=name) wcscpy_s(name,MAX_PATH,fname);

			write = true;
		}

		~File()
		{
			if(write) flush();   

			if(tiles) for(Tile**p=tiles;*p;p++) delete[] *p;
			if(packs) for(Pack**p=packs;*p;p++) delete[] *p;
			if(anims) for(Anim**p=anims;*p;p++) (*p)->Delete();
			if(diffs) for(Diff**p=diffs;*p;p++) delete[] *p;
			if(skins) for(Skin**p=skins;*p;p++) delete[] *p;

			delete[] parts;
			delete[] tiles;
			delete[] packs;
			delete[] verts;
			delete[] norms;
			delete[] chans;
			delete[] anims;
			delete[] diffs;
			delete[] skins;
		}

		void mo(__int32*); //King's Field II

		void x2mm3d(FILE*); //2020
		
		bool x2mdo(); //2021
		bool x2msm(bool x2mhm); //2022
	};
}

#ifndef _DEBUG
#define _DEBUGPOINT(x) { static int note = 0 if(!note&&++note)\
	MessageBoxA(X2MDL_MODAL,"Developer Reminder: " #x,"x2mdl.exe",MB_OK|MB_ICONERROR);}
#else
#define _DEBUGPOINT(x) 
#endif

union _IEEESingle //from Assimp qnan.h
{
	float Float;
	struct
	{
		unsigned __int32 Frac : 23;
		unsigned __int32 Exp  : 8;
		unsigned __int32 Sign : 1;
	} IEEE;
};

inline bool is_qnan(float in) //from Assimp qnan.h
{
	// the straightforward solution does not work:
	//   return (in != in);
	// compiler generates code like this
	//   load <in> to <register-with-different-width>
	//   compare <register-with-different-width> against <in>

	// FIXME: Use <float> stuff instead? I think fpclassify needs C99
	return (reinterpret_cast<_IEEESingle*>(&in)->IEEE.Exp == (1u << 8)-1 &&
		reinterpret_cast<_IEEESingle*>(&in)->IEEE.Frac);
}

struct Assimp__MaterialHelper : aiMaterial
{
	aiReturn AddProperty (const aiString* pInput,
		const char* pKey,
		unsigned int type,
		unsigned int index)
	{
		// We don't want to add the whole buffer .. write a 32 bit length
		// prefix followed by the zero-terminated UTF8 string.
		// (HACK) I don't want to break the ABI now, but we definitely
		// ought to change aiString::mLength to uint32_t one day.
		if (sizeof(size_t) == 8) {
			aiString copy = *pInput;
			uint32_t* s = reinterpret_cast<uint32_t*>(&copy.length);
			s[1] = static_cast<uint32_t>(pInput->length);

			return AddBinaryProperty(s+1,
				pInput->length+1+4,
				pKey,
				type,
				index, 
				aiPTI_String);
		}
		ai_assert(sizeof(size_t)==4);
		return AddBinaryProperty(pInput,
			pInput->length+1+4,
			pKey,
			type,
			index, 
			aiPTI_String);
	}

	aiReturn AddBinaryProperty (const void* pInput,
		unsigned int pSizeInBytes,
		const char* pKey,
		unsigned int type,
		unsigned int index,
		aiPropertyTypeInfo pType
		)
	{
		ai_assert (pInput != NULL);
		ai_assert (pKey != NULL);
		ai_assert (0 != pSizeInBytes);

		// first search the list whether there is already an entry with this key
		unsigned int iOutIndex = UINT_MAX;
		for (unsigned int i = 0; i < mNumProperties;++i)	{
			aiMaterialProperty* prop = mProperties[i];

			if (prop /* just for safety */ && !strcmp( prop->mKey.data, pKey ) &&
				prop->mSemantic == type && prop->mIndex == index){

				delete mProperties[i];
				iOutIndex = i;
			}
		}

		// Allocate a new material property
		aiMaterialProperty* pcNew = new aiMaterialProperty();

		// .. and fill it
		pcNew->mType = pType;
		pcNew->mSemantic = type;
		pcNew->mIndex = index;

		pcNew->mDataLength = pSizeInBytes;
		pcNew->mData = new char[pSizeInBytes];
		memcpy (pcNew->mData,pInput,pSizeInBytes);

		pcNew->mKey.length = ::strlen(pKey);
		ai_assert ( MAXLEN > pcNew->mKey.length);
		strcpy( pcNew->mKey.data, pKey );

		if (UINT_MAX != iOutIndex)	{
			mProperties[iOutIndex] = pcNew;
			return AI_SUCCESS;
		}

		// resize the array ... double the storage allocated
		if (mNumProperties == mNumAllocated)	{
			const unsigned int iOld = mNumAllocated;
			mNumAllocated *= 2;

			aiMaterialProperty** ppTemp;
			try {
			ppTemp = new aiMaterialProperty*[mNumAllocated];
			} catch (std::bad_alloc&) {
				return AI_OUTOFMEMORY;
			}

			// just copy all items over; then replace the old array
			memcpy (ppTemp,mProperties,iOld * sizeof(void*));

			delete[] mProperties;
			mProperties = ppTemp;
		}
		// push back ...
		mProperties[mNumProperties++] = pcNew;
		return AI_SUCCESS;
	}
};

#endif //X2MDL_PCH_INCLUDED