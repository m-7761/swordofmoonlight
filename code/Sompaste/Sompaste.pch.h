
#ifndef SOMPASTE_PCH_INCLUDED
#define SOMPASTE_PCH_INCLUDED

#include <stdio.h> 
#include <iostream>
#include <iomanip>
#include <fstream>
#include <assert.h>

#ifdef _DEBUG
//precompiling
#include <map>
#include <vector>
#include <string>	   
#include <deque>
#endif

namespace Sompaste_pch //annoying 
{
	template<typename T, size_t N> struct array
	{
		T _s[N]; operator T*(){ return _s; }
	};
}

#define NOMINMAX
#define _WIN32_WINNT 0x0501 //commctrl.h
#define WIN32_LEAN_AND_MEAN	

#include <windows.h> 
#include <windowsx.h> //GET_X_LPARAM
#include "afxres.h" //ID_EDIT_UNDO

#include <commctrl.h> //SetWindowSubclass
//5.8 or better required for SetWindowSubclass and friends
//2019: I don't think a manifest is needed for SetWindowSubclass? For XP?
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib,"comctl32.lib")

#include <uxtheme.h>
#pragma comment(lib,"uxtheme.lib")

#include <shlwapi.h>
#include <shlobj.h> //SHCreateDirectoryExW
#include <shellapi.h> //SHFileOperationW
#include <Commdlg.h> //GetOpenFileNameW

#pragma comment(lib,"shlwapi.lib")	

#define SOMPASTE_API extern "C" __declspec(dllexport) 

#include "Sompaste.h"
#include "Sompaste.res.h"

//Sompaste.cpp
extern HMODULE Sompaste_dll();
extern BOOL APIENTRY DllMain(HMODULE,DWORD,LPVOID);
extern void Somdelete(void*); //on process detach

//Sompaste.pch.cpp
extern HICON Somicon();

//Somprops.cpp
extern HWND *Somprops(HWND owner, int tabs, const wchar_t *title, HMENU menu);

//todo: would like to expose somehow???
extern int Somprops_confirm(HWND owner, const wchar_t *title); //returns IDYES/NO/CANCEL

//Somclips.cpp
extern HWND Somclips(SOMPASTE, HWND owner, void *note);
extern HWND Somtransfer(const char *op, const HWND *wins, int count, void *data, size_t data_s);

//Somcolor.cpp
extern COLORREF Somcolor(wchar_t *args, HWND window, SOMPASTE_LIB(cccb) proc);

//Somdirect.cpp
extern HWND Somdirect(SOMPASTE, HWND owner, wchar_t inout[MAX_PATH], const wchar_t *filter, const wchar_t *title, void *modeless);

//Somproject.cpp
extern const wchar_t *Somenviron(SOMPASTE p, const wchar_t *var, const wchar_t *set);
extern HWND Somplace(SOMPASTE, HWND owner, wchar_t out[MAX_PATH], const wchar_t *in, const wchar_t *title, void *modeless);
extern HWND Somproject(SOMPASTE, HWND owner, wchar_t inout[MAX_PATH], const wchar_t *filter, const wchar_t *title, void *modeless);
extern const wchar_t *Somproject_longname(long);
extern wchar_t Somproject_name(const wchar_t *longname); //EXPERIMENTAL
extern bool Somproject_inject(SOMPASTE,HWND,wchar_t[MAX_PATH],size_t); //EXPERIMENTAL

#endif //SOMPASTE_PCH_INCLUDED