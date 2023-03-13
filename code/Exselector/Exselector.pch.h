
#ifndef EXSELECTOR_PCH_INCLUDED
#define EXSELECTOR_PCH_INCLUDED

#include <stdio.h> 
#include <iostream>
#include <iomanip>
#include <fstream>
#include <assert.h>

#include <vector>

#define NOMINMAX
#define _WIN32_WINNT 0x0500 //AttachConsole
#define WIN32_LEAN_AND_MEAN	

#include <windows.h> 
#include <windowsx.h> //GET_X_LPARAM
#include <shlwapi.h>
#include <shlobj.h> //SHCreateDirectoryExW
#include <shellapi.h> //SHFileOperationW
#include <Commdlg.h> //GetOpenFileNameW
#include "afxres.h" //ID_FILE_OPEN

#pragma comment(lib,"shlwapi.lib")	
#pragma comment(lib,"version.lib")
#pragma comment(lib,"Comdlg32.lib")

#include <Wininet.h>
#pragma comment(lib,"delayimp")
//#pragma comment(lib,"Wininet.lib")
#include <Winsock2.h>
#pragma comment(lib,"Ws2_32.lib")

//HACK: Using SetWindowTheme on some scrollbars :(
#include <uxtheme.h>
#pragma comment(lib,"uxtheme.lib")

#define SOMVERSION_API extern "C" __declspec(dllexport)  

#include "src/nanovg/nanovg.h"
#include "Exselector.h"
#include "Exselector.res.h"

//Exselector.cpp
extern HMODULE Exselector_dll();
extern BOOL APIENTRY DllMain(HMODULE,DWORD,LPVOID);
extern class Exselector*const Exselector;

#endif //EXSELECTOR_PCH_INCLUDED