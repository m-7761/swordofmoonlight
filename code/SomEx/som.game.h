	
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
static const auto som_game_fgetc = *(int(__cdecl*)(FILE*))0x451822;
static const auto som_game_fgets = *(char*(__cdecl*)(char*,int,FILE*))0x44fabc;
static const auto som_game_fread = *(int(__cdecl*)(void*,int,int,FILE*))0x44fb13;
static const auto som_game_fputc = *(int(__cdecl*)(int,FILE*))0x451485;
static const auto som_game_fwrite = *(int(__cdecl*)(void*,int,int,FILE*))0x44ff9f;
static const auto som_game_fseek = *(int(__cdecl*)(FILE*,int,int))0x44fd53;
static const auto som_game_ftell = *(int(__cdecl*)(FILE*))0x44fbfb;

//basic math support?
static const auto som_game_dist2 = *(FLOAT(__cdecl*)(FLOAT,FLOAT,FLOAT,FLOAT))0x44cde0;

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

		//TESTING/OBSERVING
		HBITMAP (WINAPI *CreateDIBSection)(HDC hdc, const BITMAPINFO *pbmi, UINT iUsage, VOID **ppvBits, HANDLE hSection, DWORD dwOffset);

		//http://www.swordofmoonlight.net/bbs2/index.php?topic=320.0
		VOID (WINAPI *OutputDebugStringA)(LPCSTR lpOutputString);

		//2022: seeing if switching to newer new/delete speeds up MPX loading
		void*(__cdecl*_new_401500)(size_t),(__cdecl*_delete_401580)(void*); //TESTING

		//I'm just quickly implementing these with Detours. these basic utility
		//routines new allocate 2 string buffers for absolute no reason and it's
		//a problem for som_MPX_new other than just being absurdly wasteful
		BYTE(__cdecl*_ext_401300)(char*,char*),(__cdecl*_ext_401410)(char*,char*,char*);

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

#endif //SOM_GAME_INCLUDED