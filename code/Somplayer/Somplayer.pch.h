
#ifndef SOMPLAYER_PCH_INCLUDED
#define SOMPLAYER_PCH_INCLUDED

#include <cmath> 
#include <ctime>
#include <cstdlib> 
#include <cassert>

//precompiling
#include <map>
#include <list>
#include <vector>
#include <string>

namespace Somplayer_pch //annoying STL nonsense
{
	template<typename T, size_t N> struct array
	{			
		T _s[N]; inline operator T*(){ return _s; }		

		//the use of size is strictly optional
		size_t size; array<T,N>(){ size = 0; }
	};
}

#ifdef _DEBUG
//#define _WIN32_IE 0x0600 //IID_IBrowserFrameOptions
#define _WIN32_IE _WIN32_IE_IE70 //IObjectProvider
#define NTDDI_VERSION NTDDI_WIN7 //IObjectWithFolderEnumMode
#else
#define _WIN32_IE 0x0500 
#endif

//ListView trial/error
#define _WIN32_WINNT 0x0501

#define NOMINMAX 
#define WIN32_LEAN_AND_MEAN	

#include <windows.h> 
#include <windowsx.h> //GET_X_LPARAM
#include <afxres.h> //ID_EDIT_UNDO

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib,"comctl32.lib")

//REMINDER: GetThemeMetric vs GetSystemMetrics
#include <uxtheme.h>
#pragma comment(lib,"uxtheme.lib")

#include <shlwapi.h>
#include <shlobj.h> //SHCreateDirectoryExW
#include <shellapi.h> //SHFileOperationW
#include <shobjidl.h> //IShellView/IBrowserFrameOptions 

#pragma comment(lib,"shlwapi.lib")	
#pragma comment(lib,"version.lib")

#include <mmsystem.h> //Somwindows.h

#pragma comment(lib,"Winmm.lib")

//scheduled obsolete
#define DIRECTINPUT_VERSION 0x800

#include <dinput.h>

#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")

#define WIDEN2(x) L ## x
#define WIDEN3(x) WIDEN2(#x)
#define WIDEN(x) WIDEN2(x)
#define WSTRING(x) WIDEN3(x)
#define __WFILE__ WIDEN(__FILE__)
#define __WLINE__ WSTRING(__LINE__)

#define SOMPLAYER_API extern "C" __declspec(dllexport) 

#include "Somplayer.h"
#include "Somplayer.res.h"

//Somplayer.cpp
extern HMODULE Somplayer_dll();
extern BOOL APIENTRY DllMain(HMODULE,DWORD,LPVOID);
extern void Somdelete(void*); //on process detach

//passthru
template<typename T> T *Somdelete(T *t)
{
	Somdelete((void*)t); return t;
}

//Somplayer.pch.cpp
extern void Somfonts();

//Somplayer.pch.cpp		  //in->out+in
extern wchar_t *Somfolder(wchar_t inout[MAX_PATH], bool dir=1);
extern wchar_t *Somlibrary(wchar_t inout[MAX_PATH]);

class Somplayer; //Somconsole.cpp
extern Somplayer *Somconnect(const wchar_t v[4]=0, const wchar_t *party=0);   

#endif //SOMPLAYER_PCH_INCLUDED