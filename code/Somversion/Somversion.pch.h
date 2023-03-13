
#ifndef SOMVERSION_PCH_INCLUDED
#define SOMVERSION_PCH_INCLUDED

#include <stdio.h> 
#include <iostream>
#include <iomanip>
#include <fstream>
#include <assert.h>

#include <time.h>
#include <deque>

#define NOMINMAX
#define _WIN32_WINNT 0x0500 //AttachConsole
#define WIN32_LEAN_AND_MEAN	

#include <windows.h> 
#include <windowsx.h> //GET_X_LPARAM
#include <shlwapi.h>
#include <shlobj.h> //SHCreateDirectoryExW
#include <shellapi.h> //SHFileOperationW
#include <Commdlg.h> //GetOpenFileNameW
#include <afxres.h> //ID_FILE_OPEN

#pragma comment(lib,"shlwapi.lib")	
#pragma comment(lib,"version.lib")
#pragma comment(lib,"Comdlg32.lib")

#include <Wininet.h>
#pragma comment(lib,"delayimp")
#pragma comment(lib,"Wininet.lib")
#include <Winsock2.h>
#pragma comment(lib,"Ws2_32.lib")

//HACK: Using SetWindowTheme on some scrollbars :(
#include <uxtheme.h>
#pragma comment(lib,"uxtheme.lib")

#define SOMVERSION_API extern "C" __declspec(dllexport)  

#include "Somversion.h"
#include "Somversion.res.h"

#define C_SOMVERSION SOMVERSION_LIB(quartet_pod)

//Somversion.cpp
extern HMODULE Somversion_dll();
extern BOOL APIENTRY DllMain(HMODULE,DWORD,LPVOID);

typedef struct
{
	wchar_t path[MAX_PATH]; //dropped/opened files

}Somdialog_csv_vt; 

extern std::deque<Somdialog_csv_vt> Somdialog_csv;

//Somdialog.cpp		  //ms //csv		  //pe 	            //in			  
extern bool Somdialog(int&,const wchar_t*,wchar_t[MAX_PATH],SOMVERSION,HWND=0,bool=0);

//Somversion.cpp
extern void Somversion_update(const wchar_t *csv, const wchar_t *dll, HWND);

//Somdownload_progress
//stats[0] total bytes downloaded
//stats[1] bytes to be downloaded in total...
//when total is 0, the download has failed...
//when bytes is total the download has finished.
//stats[2]: bytes downloaded per second
//stats[3]: time elapsed in seconds
//return false to cancel/conclude download
typedef bool (*Somdownload_progress)(const int stats[4], void*);

//Somdownload.cpp		//remote->local	   
extern bool Somdownload(wchar_t[MAX_PATH], Somdownload_progress, void*);
//hack/scheduled obsolete?
//(seem to be stuck with this as long as we use the WinInet APIs)
extern DWORD Somdownload_sleep; 

//Somcache_progress
//x: item currently being inflated
//stats[0~3] same as Somdownload (inflate vs. download)
//stats[4] starts at 0, increases with each item inflated
//stats[5] 0 if x is a file, 1 if x is a folder
//return false to cancel/conclude 
typedef bool (*Somcache_progress)(const wchar_t *x, const int stats[6], void*);

//Somcache.cpp		 //local->cache	 
extern bool Somcache(wchar_t[MAX_PATH], Somcache_progress, void*);

//Somversion.pch.cpp 
extern void Somfont();

//Somversion.pch.cpp 	//illegal NTFS filename
extern wchar_t *Somlegalize(wchar_t inout[MAX_PATH]);
 
//Somversion.pch.cpp 	  //in->out+in
extern wchar_t *Somfolder(wchar_t inout[MAX_PATH], bool dir=1);
inline wchar_t *Somfile(wchar_t inout[MAX_PATH])
{
	//XP: Because PathFindFileName is different on
	//inout CANNOT end with a trailing slash
	return Somfolder(inout,false);
}

//Somtext.cpp
extern void Somtext(HWND parent);

#endif //SOMVERSION_PCH_INCLUDED