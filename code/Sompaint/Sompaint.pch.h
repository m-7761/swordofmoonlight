									 
#ifndef SOMPAINT_PCH_INCLUDED
#define SOMPAINT_PCH_INCLUDED

#include <stdio.h> 
#include <iostream>
#include <iomanip>
#include <fstream>
#include <assert.h>

//build times
#ifdef _DEBUG
#include <vector>
#endif

#define NOMINMAX 
#define WIN32_LEAN_AND_MEAN	
#define _WIN32_WINNT 0x0501
#include <windows.h> 
				  
#define WIDEN2(x) L ## x
#define WIDEN3(x) WIDEN2(#x)
#define WIDEN(x) WIDEN2(x)
#define WSTRING(x) WIDEN3(x)
#define __WFILE__ WIDEN(__FILE__)
#define __WLINE__ WSTRING(__LINE__)

#define SOMPAINT_DIRECT

#define SOMPAINT_API extern "C" __declspec(dllexport) 

#include "Sompaint.h"
#include "Sompaint.res.h"

//Sompaint.cpp
extern HMODULE Sompaint_dll();
extern BOOL APIENTRY DllMain(HMODULE,DWORD,LPVOID);
extern void Somdelete(void*); //on process detach

//Sompaint.cpp
extern SOMPAINT Somconnect(const wchar_t v[4]=0, const wchar_t *party=0);   

#endif //SOMPAINT_PCH_INCLUDED