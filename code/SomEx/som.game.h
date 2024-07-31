	
#ifndef SOM_GAME_INCLUDED
#define SOM_GAME_INCLUDED

#include <mmsystem.h> //mmio

#define SOM_STEREO_SUBMENU 0.3f

EX_COUNTER(games)
extern struct som_db
{
	static const int exe = 1+EX_COUNTOF(games);

}som_db;

extern struct som_rt //deprecated
{
	static const int exe = 1+EX_COUNTOF(games);

}som_rt;

#define SOM_GAME_READ \
GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0

//som.MDL.cpp
static const auto som_game_fopen = *(FILE*(__cdecl*)(const char*,const char*))0x44f59e;
static const auto som_game_fclose = *(int(__cdecl*)(FILE*))0x44f5b1;
static const auto som_game_fgetc = *(int(__cdecl*)(FILE*))0x451822;
static const auto som_game_fgets = *(char*(__cdecl*)(char*,int,FILE*))0x44fabc;
static const auto som_game_fread = *(int(__cdecl*)(void*,int,int,FILE*))0x44fb13;
static const auto som_game_fputc = *(int(__cdecl*)(int,FILE*))0x451485;
static const auto som_game_fwrite = *(int(__cdecl*)(void*,int,int,FILE*))0x44ff9f;
static const auto som_game_fseek = *(int(__cdecl*)(FILE*,int,int))0x44fd53;
static const auto som_game_ftell = *(int(__cdecl*)(FILE*))0x44fbfb;

//basic math support?
static const auto som_game_dist2 = *(FLOAT(__cdecl*)(FLOAT,FLOAT,FLOAT,FLOAT))0x44cde0;

extern int som_MPX_operator_new_lock;

namespace SOM
{		
	extern const struct Game	
	{
		Game();	//for SOM::Game only
		
		static void splashed();

		union //EXPERIMENTAL //2022
		{
			//HACK: this came about organically... it may benefit
			//from a refactor, if it's even worth thinking about?

			mutable unsigned item_lock; struct //UNION
			{
				//2022: these replace som_game_browsing_menu, etc.
				//NOTE: item_lock is no longer set by these three
				//bits since the first 8 is a nesting counter now
				mutable unsigned:8,browsing_menu:1,browsing_shop:1,taking_item:1;
			};			
		};
		struct item_lock_section : EX::section //RAII
		{
			item_lock_section(int=0); ~item_lock_section();
		};

		struct operator_new_lock_raii
		{
			operator_new_lock_raii()
			{
				som_MPX_operator_new_lock++;
			}
			~operator_new_lock_raii()
			{
				som_MPX_operator_new_lock--;
			}
		};

		//ext must be one of 0 or '.ini'
		static const char *image(int ext=0);
		static const wchar_t *title(int ext=0);
				
		//Reminder: som_db looks in data before project/DATA
		//See som.tools.h for some inline documentation for these
		static const wchar_t *data(const char *in=0, bool debug=1);	  
		static const wchar_t *project(const char *in=0, bool debug=1);
		static const wchar_t *file(const char *in);
			
		//[Bugfix] do_fix_slowdown_on_save_data_select = yes
		//required for 0 return. Will simply translate otherwise
		//retrieve Unicode path to save file or 0 if reading/absent
		static const wchar_t *save(const char *in=0, bool reading=0);

		//return true if this matches/belongs in the configuration file
		static bool ini(const char *inifile, const char *section="config");

		//config file access (value,write/default,section)
		static BOOL write(const char*,const char*,const char *section="config");
		//WARNING: FLUSHES EXTENDED VALUES ONCE IF SOM::field IS SET
		static UINT get(const char*,int,const char *section="config");
		
		//MALLOC/FREE LIKE
		//reminder: 401500 allocates memory by way
		//of kernel32.dll saving the result in what
		//looks like a hash table of fixed dimension
		//with 3 times the low 15 bits of the address
		//collisions go into a linked-list		
		//401580 deallocates
		//it doesn't look like it recycles
		//4015E0 dumps the memory at the program's end
		//00401BB5 E8 26 FA FF FF       call        004015E0
		template<class T>
		static void malloc_401500(T* &p, DWORD n=1)
		{
			p = ((T*(__cdecl*)(DWORD))0x401500)(n*sizeof(T)); 
		}
		static void free_401580(const void *p)
		{
			assert(p); //40169c prints 0 as a "delete failure" case
			((void(__cdecl*)(DWORD))0x401580)((DWORD)p); 
		}

		inline const SOM::Game *operator->()const{ return detours; }

		LRESULT (CALLBACK *WindowProc)(HWND,UINT,WPARAM,LPARAM);					

		void *_detour_begin;
		inline void detour(LONG (WINAPI *f)(PVOID*,PVOID))const
		{
			if(detours) for(void
			**p = (void**)&_detour_begin,
			**q = (void**)&detours->_detour_begin;
			p!=&_detour_end;p++,q++) if(*q&&*p) f(p,*q);
		}
										
		//NEW: using timeGetTime
		//2020: it turns out som_db uses timeGetTime but Direct X uses GetTickCount
		//(I'm not sure what for)
		DWORD (WINAPI *GetTickCount)(); 

		BOOL (WINAPI *SetCurrentDirectoryA)(LPCSTR); 

		BOOL (WINAPI *WritePrivateProfileStringA)(LPCSTR,LPCSTR,LPCSTR,LPCSTR);
		UINT (WINAPI *GetPrivateProfileIntA)(LPCSTR,LPCSTR,INT,LPCSTR); 

		HANDLE (WINAPI *FindFirstFileA)(LPCSTR,LPWIN32_FIND_DATAA);

		BOOL (WINAPI *FindClose)(HANDLE hObject); //paranoia

		HANDLE (WINAPI *CreateFileA)(LPCSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,DWORD,DWORD,HANDLE);
		HANDLE (WINAPI *LoadImageA)(HINSTANCE,LPCSTR,UINT,int,int,UINT);		

		BOOL (WINAPI *ReadFile)(HANDLE,LPVOID,DWORD,LPDWORD,LPOVERLAPPED);							 
		BOOL (WINAPI *WriteFile)(HANDLE,LPCVOID,DWORD,LPDWORD,LPOVERLAPPED); 
		BOOL (WINAPI *CloseHandle)(HANDLE hObject);	 

		//// BGM //////////
		HMMIO (WINAPI *mmioOpenA)(LPSTR,LPMMIOINFO,DWORD);
		LONG (WINAPI *mmioRead)(HMMIO,HPSTR,LONG);
		//LONG (WINAPI *mmioSeek)(HMMIO,LONG,int);
		MMRESULT (WINAPI *mmioDescend)(HMMIO,LPMMCKINFO,const MMCKINFO*,UINT);
		MMRESULT (WINAPI *mmioAscend)(HMMIO,LPMMCKINFO,UINT);
		MMRESULT (WINAPI *mmioClose)(HMMIO,UINT);
		//the only MIDI interface
		//MCI_OPEN happens at the following address/call
		//(it might make more sense to just commandeer that call, but it's not 
		//a normal call)
		//0042362E FF D7                call        edi
		MCIERROR (WINAPI *mciSendCommandA)(MCIDEVICEID,UINT,DWORD_PTR,DWORD_PTR);

		//maybe put in Ex.detours.h
		//// DirectSound insanity ////
		LSTATUS (APIENTRY *RegOpenKeyExA)(HKEY,LPCSTR,DWORD,REGSAM,PHKEY);
		LSTATUS (APIENTRY *RegOpenKeyExW)(HKEY,LPCWSTR,DWORD,REGSAM,PHKEY);

		//// Is there no way to remove WS_EX_TOPMOST??? /////////////

		HWND (WINAPI *CreateWindowExA)(DWORD,LPCSTR,LPCSTR,DWORD,int,int,int,int,HWND,HMENU,HINSTANCE,LPVOID);

		//http://www.swordofmoonlight.net/bbs2/index.php?topic=320.0
		VOID (WINAPI *OutputDebugStringA)(LPCSTR lpOutputString);

		//2022: seeing if switching to newer new/delete speeds up MPX loading
		void*(__cdecl*_new_401500)(size_t),(__cdecl*_delete_401580)(void*); //TESTING

		//I'm just quickly implementing these with Detours. these basic utility
		//routines new allocate 2 string buffers for absolute no reason and it's
		//a problem for som_MPX_new other than just being absurdly wasteful
		BYTE(__cdecl*_ext_401300)(char*,char*),(__cdecl*_ext_401410)(char*,char*,char*);

		//2023: ext.speed support
		BYTE(__cdecl*_ext_441510_advance_MDL)(void*),(__cdecl*_ext_4414c0_set_MDL)(void*,int);

		void *_detour_end;

	private: const SOM::Game *detours;
		
	}Game;
}

//Reminder: som_db looks in data before project/DATA
inline const wchar_t *SOM::Game::file(const char *in)
{
	const wchar_t *out = SOM::Game::data(in,0);

	return !out?SOM::Game::project(in,0):out; 
}

namespace DX
{
	struct IDirect3DVertexBuffer7;
}
namespace som_scene
{
	const int vbuffersN = 3;
	const int vbuffer_size = 4096; //EXTENSION?
	extern DX::IDirect3DVertexBuffer7 *ubuffers[vbuffersN];
	extern DX::IDirect3DVertexBuffer7 *vbuffers[vbuffersN];
}
namespace som_scene_state //2022: formerly som.scene.cpp
{
	static DWORD
	//44D0B0 increments this as it pushes a render state block onto a stack
	//and then copies the current block into the following global state mem
	//44d200 pops the state
	//&?? = *(DWORD*)0x1D69DA0,
	//&vb = *(DWORD*)0x1D69DA4, //SOM::L.vbuffer
	&zwriteenable = *(DWORD*)0x1D6A248,
	&alphaenable = *(DWORD*)0x1D6A24C,
	&colorkeyenable = *(DWORD*)0x1D6A250,
	&fogenable = *(DWORD*)0x1D6A254,
	&srcblend = *(DWORD*)0x1D6A258,
	&destblend = *(DWORD*)0x1D6A25C,
	&tex0colorop = *(DWORD*)0x1D6A260,
	&tex0colorarg1 = *(DWORD*)0x1D6A264,
	&tex0colorarg2 = *(DWORD*)0x1D6A268,
	&tex0alphaop = *(DWORD*)0x1D6A26C,
	&tex0alphaarg1 = *(DWORD*)0x1D6A270,
	&tex0alphaarg2 = *(DWORD*)0x1D6A274,
	//gs11: unidentified
	//&gs11 = *(DWORD*)0x1D6A278, //lighting? 44d8a5 (deferred?)
	//I think this is 0 for tile drawing phases and 1 for 
	//MDL/MDO drawing however Ghidra shows no write references
	//push/pop_render_state changes it (meaningfully?)
	//only 44d810 references it, but doesn't assign it???
	&lighting_desired = *(DWORD*)0x1D6A278,
	&lighting_current = *(DWORD*)0x1D6A27C,
	&texture = *(DWORD*)0x1D6A280, 
	&material = *(DWORD*)0x1D6A284;
	static BYTE //1 if not identity matrix
	&worldtransformed = *(BYTE*)0x1D6A288;

	inline void push(){ ((BYTE(__cdecl*)())0x44D0B0)(); }
	inline void pop(){ ((BYTE(__cdecl*)())0x44d200)(); }

	extern void setss(DWORD); //som.bsp.cpp
}
typedef struct som_scene_element //104B
{			
	//2023: these should match som_MDL::vbuf
	//modes
	//0: never seen in wild, close to 1	
	//1: DrawPrimitive (D3DFVF_LVERTEX?)
	//2: DrawPrimitive (D3DFVF_TLVERTEX?)
	//3: DrawIndexedPrimitiveVB (lock/unlock pattern)
	//4: DrawIndexedPrimitiveVB (D3DFVF_LVERTEX?)
	//lit enables light selection 
	//EXTENSIONS
	//npc is new/used for shadows
	//vs is for do_red on (sorted) transparent elements
	//2021: batch marks transparent and batched elements
	//(ai is for debugging troubles)
	//2021: tnl must be pretransformed prior to drawing
	//this enables skinning and consolidating MDO chunks
	//see som_MDL_447fc0_cpp	
	//2022: sort is to not overload transparency sorting
	DWORD fmode:4,unused1:2,sort:1,tnl:1,vs:8,npc:2,kage:1,ai:11,batch:1,lit:1;
	union //a mess of flags
	{
		DWORD flags; struct
		{
		unsigned primtype:4, 
		//unpacked by 44D853~44D8A2
		//cop selects the colorop/etc model
		//0044D958 FF 24 85 08 E1 44 00 jmp dword ptr [eax*4+44E108h]
		//2021: 42e57f sets u31 to 0 (it's usually 1)
		//MDL sets u12 to 1 or 2 (what sets fe? fog?)
		//
		// MDO sblend/dblend (2021)
		// MDO has sblend equal to alpha*src and dblend
		// may be 1*dst or (1-alpha)*dst
		//
		sblend:4,dblend:4,cop:4,u12:4,u16:7,fe:1,zwe:1,abe:1,cke:1,
		u31:1; //is u16 unused? (44d521 sets u12, 44d510 sets u31)
		};
	};
	
	//the old approach did sorting at the chunk
	//level. I don't know if the original x2mdo
	//"intelligently" chunked into convex parts
	//or not, but I doubt it
	//the new approach stores a split-structure
	//for generating additional triangles given
	//a world transformed set of base triangles
	union
	{
		//depth-sorting key (old)
		//0044D68B D9 58 08 fstp dword ptr [eax+8]
		FLOAT depth_44D5A0; 

		struct som_BSP *bsp; //UNUSED
	};

	//material ISN'T CONSULTED BEFORE SetMaterial
	//material ISN'T CONSULTED BEFORE SetMaterial
	//material ISN'T CONSULTED BEFORE SetMaterial
	//IDs: textures 0~1023, materials 0~65535???
	//probably everything gets a unique material
	WORD texture, material; //D3DMATERIAL7

	//union
	//MODES 1 &2 VERTEX DATA BEGINS HERE	

	//0x10: 		
	FLOAT worldxform[4][4];		
	//0x50:	
	//0044DED6 E83533FCFF call 00411210
	//0044DC85 E88635FCFF call 00411210 (uses first vertex)
	FLOAT lightselector[3]; 
	//vertex/index
	WORD vcount, icount;  
	//0x60	
	//VOID *vdata; WORD *idata;
	struct v //guessing (32 bytes)
	{
		//NOTE: some MDLs are sprites but
		//they happen to also be 32 bytes
		//they have hpos[4],color,_,uv[2]
		float pos[3],lit[3],uv[2];
	};
	v *vdata; WORD *idata;

}*som_scene_elements[1024]; //256+512+128+128?

struct som_scene_picture //144B //sfx_element?
{			
	//som_scene_element (draw subroutine compatible)
	DWORD fmode:4,unused1:2,sort:1,tnl:1,vs:8,npc:2,kage:1,ai:11,batch:1,lit:1;
	union 
	{
		DWORD flags; struct
		{
		unsigned primtype:4, 
		sblend:4,dblend:4,cop:4,u12:4,u16:7,fe:1,zwe:1,abe:1,cke:1,
		u31:1; 
		};
	};
	FLOAT depth_44D5A0; //???
	WORD texture, material; //material?

	struct vertex //D3DFVF_LVERTEX or D3DFVF_TLVERTEX
	{
		float x,y,z,w; //w is not used by LVERTEX but is reserved space

		DWORD diffuse,specular;

		float s,t;

	}vdata[4];
};

#endif //SOM_GAME_INCLUDED