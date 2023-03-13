												   									 
#ifndef SOMPAINT_D3D9_INCLUDED
#define SOMPAINT_D3D9_INCLUDED

//#include <stdio.h> 
//#include <iostream>
//#include <iomanip>
//#include <fstream>
#include <assert.h>
#include <typeinfo.h> 

//precompiling
#include <map>
#include <vector>
#include <string>

#define NOMINMAX 
#define WIN32_LEAN_AND_MEAN	

#include <windows.h> 
#include <windowsx.h> //DeleteBitmap

#include <d3d9.h>
#include <d3dx9.h>

#ifdef _DEBUG
#pragma comment(lib,"d3dx9d.lib")
#else
#pragma comment(lib,"d3dx9.lib")
#endif

#define WIDEN2(x) L ## x
#define WIDEN3(x) WIDEN2(#x)
#define WIDEN(x) WIDEN2(x)
#define WSTRING(x) WIDEN3(x)
#define __WFILE__ WIDEN(__FILE__)
#define __WLINE__ WSTRING(__LINE__)

//Sompaint_D3D9.cpp
extern HMODULE D3D9_dll();
extern BOOL APIENTRY DllMain(HMODULE,DWORD,LPVOID);

//Exporting Connect and Disconnect
#define SOMPAINT_MODULE_API extern "C" __declspec(dllexport) 

#include "../Sompaint/Sompaint.h" 

#include "Sompaint_D3D9.res.h"

//Microsoft IntelliSense Bug
#define typeid(x) (typeid(x))

#endif //SOMPAINT_D3D9