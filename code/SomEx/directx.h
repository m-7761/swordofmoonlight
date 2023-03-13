
#ifndef DIRECTX_INCLUDED
#define DIRECTX_INCLUDED

//This is the precompiled header for dx. objects

#include <math.h>
#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <fstream>

#include <map> //dx_d3d9c_stereoVD_t
#include <vector> //dx_d3d9x_flushaders

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_DEPRECATE //for faster builds 

//#include "stdafx.h" 
#include <windows.h>

//Release build with assert
#if defined(NDEBUG)&&defined(RASSERT)
#include <crtdefs.h>
#undef NDEBUG
#include <assert.h>
#define NDEBUG
#else
#include <assert.h>
#endif

//Microsoft specific
#pragma warning(disable:3) //macro parameters
#pragma warning(disable:5) //macro redefinition
#pragma warning(disable:506) //no inline definition 
#pragma warning(disable:819) //character codepage mismatch
#pragma warning(disable:996) //declared deprecated
#pragma warning(disable:288) //for-loop scoping

#pragma warning(error:18) //signed/unsigned mismatch

#ifdef NDEBUG
#define DX_UNFINISHED unfini$hed
#else
#define DX_UNFINISHED 
#endif

#include "Ex.log.h"	  
static EX::Prefix dx; 
#define DX_WIDEN2(x) L ## x
#define DX_WIDEN(x) DX_WIDEN2(x)
#define __WFILE__ DX_WIDEN(__FILE__)
#define DX_LOG(NS,_)\
EX::including_log(__WOBJECT__,L#NS,\
&debug,&error,&panic,&alert,&hello,&sorry)
//define this after the precompiled header inclusion (in every file)
#define DX_TRANSLATION_UNIT static const wchar_t *__WOBJECT__ = __WFILE__;

#define DX_ARRAYSIZEOF(array) (sizeof(array)/sizeof(array[0])) //2021

namespace EX //REMOVE ME?
{
	extern int is_needed_to_initialize(); //SomEx.cpp	
	extern int detouring(void(*)(LONG(WINAPI*)(PVOID*,PVOID))); //Ex.detours.cpp
	extern unsigned tick(); //ms timer
	extern int central_processing_units;
	#ifdef _DEBUG 
	extern void dbgmsg(const char*,...); 
	#endif
}
namespace DX
{
	using EX::is_needed_to_initialize;
	using EX::detouring;
	using EX::tick;
	using EX::central_processing_units;
	#ifdef _DEBUG 
	using EX::dbgmsg;
	#else
	inline void dbgmsg(...){} //NEW
	#endif

	#ifdef _DEBUG
	enum{ debug=1 };
	#else
	enum{ debug=0 };
	#endif
	
	//http://joeduffyblog.com/2006/08/22/priorityinduced-starvation-why-sleep1-is-better-than-sleep0-and-the-windows-balance-set-manager/
	inline void sleep(int ms=1){ Sleep(ms); }

	#define DX_CRITICAL_SECTION \
	static DX::critical c__; DX::section s__(c__);
	struct critical
	{	
		CRITICAL_SECTION _CS_; //DUPLICATE (Ex.hpp)
		critical(DWORD sc=~0){ InitializeCriticalSectionAndSpinCount(&_CS_,sc==~0?4000:sc); }
		~critical(){ DeleteCriticalSection(&_CS_); }	
	};
	struct section
	{	
		CRITICAL_SECTION *_CS_; //private
		section(critical &cs):_CS_(&cs._CS_){ if(_CS_) EnterCriticalSection(_CS_); }
		section(CRITICAL_SECTION*cs):_CS_(cs){ if(_CS_) EnterCriticalSection(_CS_); }
		~section(){ if(_CS_) LeaveCriticalSection(_CS_); }
	};
}

#endif //DIRECTX_INCLUDED