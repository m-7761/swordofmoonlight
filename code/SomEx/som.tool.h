										   
#ifndef SOM_TOOL_INCLUDED
#define SOM_TOOL_INCLUDED

#include <mmsystem.h> //mmio

EX_COUNTER(tools)
#ifndef TOOL
#define TOOL(X) static struct X\
{\
	enum{ exe=1+EX_COUNTOF(tools) };\
}X;
#elif !defined(SOM_TOOL_HPP)
#error TOOL not #defined by SOM_TOOL_HPP
#endif
//Sword of Moonlight tools
TOOL(SOM_MAIN) TOOL(SOM_RUN) TOOL(SOM_PRM)
TOOL(SOM_EDIT) TOOL(SOM_SYS) TOOL(SOM_MAP)
TOOL(MapComp) //MPX compiler 
//Reminder: SOM::tool tests expect PrtsEdit 
//to be first (and these anything after it)
TOOL(PrtsEdit) TOOL(ItemEdit) TOOL(ObjEdit) 
TOOL(EneEdit) TOOL(NpcEdit) TOOL(SfxEdit)
#undef TOOL

#define SOM_TOOL_READ \
GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0

typedef struct tagOFNA* LPOPENFILENAMEA; //Commdlg.h
typedef struct _browseinfoA* LPBROWSEINFOA; //shlobj.h

namespace SOM
{		
	extern int tool,game;

	extern const struct Tool 
	{	
		Tool(); //for SOM::Tool only

		//data: If able (0 return otherwise)
		//convert ANSI data path into Unicode path*
		//*data path provided by EX::INI::Folder::data
		static const wchar_t *data(const char *in=0, bool debug=1);		

		//project: If able (0 return otherwise)
		//convert ANSI project path into Unicode path
		static const wchar_t *project(const char *in=0, bool debug=1);		
		static const wchar_t *install(const char *in=0); 
		//legacy support for SOM_RUN and SOM_EDIT stuff
		static const wchar_t *runtime(const char *in=0);
		static const wchar_t *project2(const char *in=0);

		//file: return is from data or project
		static const wchar_t *file(const char *in)
		{
			const wchar_t *out=SOM::Tool::data(in,0);
			return !out?SOM::Tool::project(in,0):out; 
		}

		//TODO? relocate to Sompaste.dll
		//emulates DragDetect with timeout and right-click	
		//FYI: DragDetect's timeout is at least 500ms and fixed
		//HACK: wm can be a DBLCLK code to detect double-clicks, in
		//which case 2 is returned		
		static int dragdetect(HWND, unsigned wm, unsigned ms=GetDoubleClickTime());		

		//HACK: these call WriteFile a lot		
		HANDLE Mapcomp_MSM(const wchar_t*)const; //do_aa
		HANDLE Mapcomp_MHM(const wchar_t*)const; //SOM_MAP_165
		EX::temporary &Mapcomp_MHM_temporary()const; //MapComp_main

		inline const SOM::Tool *operator->()const{ return detours; }

		void *_detour_begin;
		inline void detour(LONG (WINAPI *f)(PVOID*,PVOID))const
		{
			if(detours) for(void
			**p = (void**)&_detour_begin,
			**q = (void**)&detours->_detour_begin;
			p!=&_detour_end;p++,q++) if(*q&&*p) f(p,*q);
		}

		LRESULT (WINAPI *SendMessageA)(HWND,UINT,WPARAM,LPARAM);
		LRESULT (WINAPI *SendDlgItemMessageA)(HWND,int,UINT,WPARAM,LPARAM);
		BOOL (WINAPI *PostMessageA)(HWND,UINT,WPARAM,LPARAM);
		BOOL (WINAPI *PostThreadMessageA)(DWORD,UINT,WPARAM,LPARAM);
		LRESULT (WINAPI *CallWindowProcA)(WNDPROC,HWND,UINT,WPARAM,LPARAM);

		//notice: these bypass SendMessageA		
		//SOM may not use Set/GetDlgItemTextA 
		BOOL (WINAPI *SetWindowTextA)(HWND,LPCSTR);
		BOOL (WINAPI *SetDlgItemTextA)(HWND,int,LPCSTR);
		int (WINAPI *GetWindowTextA)(HWND,LPSTR,int); 
		int (WINAPI *GetWindowTextLengthA)(HWND); 
		UINT (WINAPI *GetDlgItemTextA)(HWND,int,LPSTR,int);
		//UINT (WINAPI *GetDlgItemTextLengthA)(HWND,int);
				
		INT_PTR (WINAPI *DialogBoxParamA)(HINSTANCE,LPCSTR,HWND,DLGPROC,LPARAM);
		HWND (WINAPI *CreateDialogIndirectParamA)(HINSTANCE,LPCDLGTEMPLATEA,HWND,DLGPROC,LPARAM);				
		HWND (WINAPI *CreateWindowExA)(DWORD,LPCSTR,LPCSTR,DWORD,int,int,int,int,HWND,HMENU,HINSTANCE,LPVOID);
				
		//used by SOM_SYS's slideshow preview (2018: or is it?)
		BOOL (WINAPI *SetCurrentDirectoryA)(LPCSTR); 

		//monitoring
		BOOL (WINAPI *SetEnvironmentVariableA)(LPCSTR,LPCSTR);
		HANDLE (WINAPI *CreateMutexA)(LPSECURITY_ATTRIBUTES,BOOL,LPCSTR);
		HANDLE (WINAPI *OpenMutexA)(DWORD,BOOL,LPCSTR);		
		//SOM_MAP
		BOOL (WINAPI *DeleteFileA)(LPCSTR);
		DWORD (WINAPI *GetFullPathNameA)(LPCSTR,DWORD,LPSTR,LPSTR*);
		DWORD (WINAPI *GetFileAttributesA)(LPCSTR);
		BOOL (WINAPI *GetDiskFreeSpaceA)(LPCSTR,LPDWORD,LPDWORD,LPDWORD,LPDWORD);
		BOOL (WINAPI *GetVolumeInformationA)(LPCSTR,LPSTR,DWORD,LPDWORD,LPDWORD,LPDWORD,LPSTR,DWORD);

		LONG (APIENTRY *RegQueryValueExA)(HKEY,LPCSTR,LPDWORD,LPDWORD,LPBYTE,LPDWORD);

		HINSTANCE (STDAPICALLTYPE *ShellExecuteA)(HWND,LPCSTR,LPCSTR,LPCSTR,LPCSTR,INT); 
		
		//2020: W catches WinHlp32.exe (Windows 10 opens browser on F1. EneEdit uses F1)
		BOOL (WINAPI *CreateProcessW)(LPCWSTR,LPWSTR,LPSECURITY_ATTRIBUTES,LPSECURITY_ATTRIBUTES,BOOL,DWORD,LPVOID,LPCWSTR,LPSTARTUPINFOW,LPPROCESS_INFORMATION);
		BOOL (WINAPI *CreateProcessA)(LPCSTR,LPSTR,LPSECURITY_ATTRIBUTES,LPSECURITY_ATTRIBUTES,BOOL,DWORD,LPVOID,LPCSTR,LPSTARTUPINFOA,LPPROCESS_INFORMATION);
																																							  				
		LPITEMIDLIST (STDAPICALLTYPE *SHBrowseForFolderA)(LPBROWSEINFOA);

		BOOL (APIENTRY *GetOpenFileNameA)(LPOPENFILENAMEA);
		//PrtsEdit & co
		BOOL (APIENTRY *GetSaveFileNameA)(LPOPENFILENAMEA);
		HMENU (WINAPI *GetMenu)(HWND);
		BOOL (WINAPI *SetMenu)(HWND,HMENU);

		BOOL (WINAPI *CopyFileA)(LPCSTR,LPCSTR,BOOL);
				
		HANDLE (WINAPI *FindFirstFileA)(LPCSTR,LPWIN32_FIND_DATAA);

		BOOL (WINAPI *FindNextFileA)(HANDLE,LPWIN32_FIND_DATAA); 
		BOOL (WINAPI *FindClose)(HANDLE);
				
		HANDLE (WINAPI *CreateFileA)(LPCSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,DWORD,DWORD,HANDLE);

		BOOL (WINAPI *ReadFile)(HANDLE,LPVOID,DWORD,LPDWORD,LPOVERLAPPED);
		BOOL (WINAPI *WriteFile)(HANDLE,LPCVOID,DWORD,LPDWORD,LPOVERLAPPED);
		
		BOOL (WINAPI *CloseHandle)(HANDLE);

		BOOL (WINAPI *CreateDirectoryA)(LPCSTR,LPSECURITY_ATTRIBUTES);

		//HBITMAP (WINAPI *LoadBitmapA)(HINSTANCE,LPCSTR); 
		HANDLE (WINAPI *LoadImageA)(HINSTANCE,LPCSTR,UINT,int,int,UINT);		
		
		//SOM_MAP's treeview's icons
		//HIMAGELIST (WINAPI *ImageList_LoadBitmapA)(HINSTANCE,LPCSTR,int,int,COLORREF);
		HIMAGELIST (WINAPI *ImageList_LoadImageA)(HINSTANCE,LPCSTR,int,int,COLORREF,UINT,UINT);		

		//button discoloration if 2000!=time_machine_mode
		HIMAGELIST (WINAPI *ImageList_Create)(int cx, int cy, UINT flags, int cInitial, int cGrow);
		
		//BitBlt is just trying to find out where
		//SOM_MAPs icon palette width is in memory
		//but it might be required for themes later
		BOOL (WINAPI *BitBlt)(HDC,int,int,int,int,HDC,int,int,DWORD);
		BOOL (WINAPI *StretchBlt)(HDC,int,int,int,int,HDC,int,int,int,int,DWORD);

		int (WINAPI *DrawTextA)(HDC,LPCSTR,int,LPRECT,UINT);
		BOOL (WINAPI *TextOutA)(HDC,int,int,LPCSTR,int);
		BOOL (WINAPI *FrameRgn)(HDC,HRGN,HBRUSH,int,int);
		
		//Unicode (A->W)
		//http://stackoverflow.com/questions/19737412/
		BOOL (WINAPI *GetMessageA)(LPMSG,HWND,UINT,UINT);
		BOOL (WINAPI *PeekMessageA)(LPMSG,HWND,UINT,UINT,UINT);
		BOOL (WINAPI *IsDialogMessageA)(HWND,LPMSG);
		LRESULT (WINAPI *DispatchMessageA)(CONST MSG*);		
		//in practice this is the only one that matters
		LONG (WINAPI *SetWindowLongA)(HWND,int,LONG);
				
		//SOM uses some kind of a "skinning" library???
		//Mainly this needs to filter out non-SOM windows
		HHOOK (WINAPI *SetWindowsHookExA)(int,HOOKPROC,HINSTANCE,DWORD);		
		//there's no other way to work around this one		
		BOOL (WINAPI *DrawFocusRect)(HDC,CONST RECT*);
		//needed to hide the control keys
		SHORT (WINAPI *GetKeyState)(int); 

		int (WINAPI *MessageBoxA)(HWND,LPCSTR,LPCSTR,UINT);

		//// Winmm ADPCM business //////////

		//same arrangement as som.game.h/cpp
		HMMIO (WINAPI *mmioOpenA)(LPSTR,LPMMIOINFO,DWORD);
		
		//// IP Address Control bug ////////

		//will never be fixed for XP if ever
		//http://connect.microsoft.com/VisualStudio/feedback/details/1029535
		BOOL (WINAPI *DeleteObject)(HGDIOBJ);

		//// workshop.cpp exclusives ///////

		int (WINAPI *TranslateAcceleratorA)(HWND,HACCEL,LPMSG);
		HACCEL (WINAPI *LoadAcceleratorsA)(HINSTANCE,LPCSTR);
		BOOL (WINAPI *MessageBeep)(UINT); //debugging

		//// MapComp MAP diagnostic ////////

		char * (__cdecl *MapComp_fgets)(char*,int,char**); //40c415

		void *_detour_end;

	private: const SOM::Tool *detours;

	}Tool;
}

#define SOM_TOOL_DETOUR_THREADMAIN(f,...) \
if(GetCurrentThreadId()!=EX::threadmain) return SOM::Tool.f(__VA_ARGS__);

#endif //SOM_TOOL_INCLUDED
