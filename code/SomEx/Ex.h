
#ifndef EX_INCLUDED
#define EX_INCLUDED SomEx

//REMOVE ME?
#define _USE_MATH_DEFINES

#ifndef UNICODE
#define UNICODE //windowsx.h
#endif

//stdlib
#include <time.h>
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <assert.h>
//fstat
#include <io.h> 
#include <sys/stat.h>
//std::
#include <ios>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <cwchar>
#include <limits>
//build times
#ifdef _DEBUG
#include <set>
#include <map>
#include <array>
#include <deque>
#include <vector>
#include <string>
#include <algorithm>
#include <functional> //function
#include <hash_map> //unordered_map for Windows XP
#include <bitset>
#endif

//#define NOMINMAX
#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_DEPRECATE //for faster builds 								 
#include <windows.h>
#include "windowsex.h"
#include <objbase.h>

#define GetThreadId Vista Only

//Release build with assert
#if defined(NDEBUG) && defined(RASSERT)
#include <crtdefs.h>
#undef NDEBUG
#include <assert.h>
#define NDEBUG
#endif

#include <shlobj.h>	//SHCreateDirectoryEx()
#include <shlwapi.h> //PathIsRelativeW/PathCombineW()
#include <shellapi.h> //CommandLineToArgvW
#include <commdlg.h> //GetOpenFileNameW
					
#include <richedit.h>
#include <richole.h> //4.1
#include <tom.h> //ITextDocument
//MSDN is wrong--affects dragging not dblclk
#undef ECO_AUTOWORDSELECTION 

#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"comdlg32.lib")

#include <psapi.h>
#pragma comment(lib,"psapi.lib")
namespace EX{ static HMODULE comctl32()
{
	static HMODULE out = 0; if(out) return out;
	//per "Enumerating All Modules For a Process example
	HMODULE hMods[1024]; HANDLE hProcess; DWORD cbNeeded;	
	hProcess = GetCurrentProcess(); WCHAR szModName[MAX_PATH];
	if(EnumProcessModules(hProcess,hMods,sizeof(hMods),&cbNeeded))
    for(DWORD i=0,n=cbNeeded/sizeof(HMODULE);i<n;i++)
    if(GetModuleBaseNameW(hProcess,hMods[i],szModName,MAX_PATH))
    if(!wcsicmp(L"comctl32.dll",szModName)) return out = hMods[i]; assert(0); 
	return out = (HMODULE)~UINT_PTR(0);
}}
#include <commctrl.h> //SetWindowSubclass
//5.8 or better required for SetWindowSubclass and friends
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib,"comctl32.lib")	
#include <uxtheme.h>
#pragma comment(lib,"uxtheme.lib")
#include <Vssym32.h>

//Microsoft specific
#pragma warning(disable:3) //macro parameters
#pragma warning(disable:5) //macro redefinition
#pragma warning(disable:506) //no inline definition 
#pragma warning(disable:819) //character codepage mismatch
#pragma warning(disable:996) //declared deprecated
//Microsoft specific
#pragma warning(error:18) //signed/unsigned mismatch

#define MS_VC_EXCEPTION 0x406D1388
static void SetThreadName(DWORD ID, LPCSTR Name)
{	
	struct{ DWORD x1000; LPCSTR Name; DWORD ID, reserved; }info={0x1000,Name,ID,0};
	__try{ RaiseException(MS_VC_EXCEPTION,0,sizeof(info)/sizeof(DWORD),(DWORD*)&info); }
	__except(EXCEPTION_CONTINUE_EXECUTION){}
}

/////////////////////////////////////////////////////////////////////////
//NEW: SLOWLY MIGRATING STANDARD BITS & PIECES OF THIS FILE INTO Ex.hpp//
/////////////////////////////////////////////////////////////////////////

#include "Ex.hpp"
#include "Ex.log.h"
static EX::Prefix Ex;
#define __WFILE__ EX_WIDEN(__FILE__)
#define EX_LOG(NS)\
EX::including_log(__WOBJECT__,L#NS,\
&debug,&error,&panic,&alert,&hello,&sorry)
//this is added after the precompiled header #include of every single file
#define EX_TRANSLATION_UNIT static const wchar_t *__WOBJECT__ = __WFILE__;

namespace EX
{ 
extern HMODULE module;
extern bool attached,detached;
extern HDC hdc(HDC *release=0);
extern int central_processing_units;
extern DWORD threadmain,tls(short, DWORD=0);

extern void vblank(); //uses DDRAW::vblank
extern unsigned tick(); //uses multimedia timer

static const int log_ceiling = 8;
extern const wchar_t *log(), *exe();
extern int messagebox(int,const char*,...);
extern void numbers(),numbers_mismatch(bool,const wchar_t*);
									 
//file system places
extern const wchar_t *cd(),*user(int);
extern const wchar_t *ini(int),*text(int);
extern const wchar_t *lang(int),*font(int),*data(int); 
template<typename T>
inline int data(const T *in, wchar_t out[MAX_PATH])
{
	wchar_t f[] = L"%ls\\%hs";
	if(sizeof(T)!=1) f[5] = 'l';
	for(int i=0;*EX::data(i);i++)
	{
		int outlen = swprintf(out,f,EX::data(i),in);
		if(outlen>0&&PathFileExists(out)) 
		return outlen;
	}
	*out = '\0'; return 0;
}

enum{ contexts=4 };
extern int context(),directx(); 
extern void pause(int),unpause(int);
extern bool alt(),numlock(),arrow();
extern int tilt(),is_needed_to_initialize();
extern bool ok_generic_failure(const wchar_t *one_liner,...);
extern void is_needed_to_shutdown_immediately(int exitcode=-1, const char *caller=0);

//caution: an internal buffer is returned upon error
extern const char *need_ansi_equivalent(int cp, const wchar_t *in, char *out=0, int out_s=0);
extern const wchar_t *need_unicode_equivalent(int cp, const char *in, wchar_t *out=0, int out_s=0);

/*2018 (source of significant typographic errors)
//caution: an internal buffer is returned upon error
inline const char *need_ansi_equivalent(const wchar_t *in, char *out=0, int out_s=0)
{
	//use need_ansi_path_to_file (below) for file names
	return EX::need_ansi_equivalent(CP_THREAD_ACP,in,out,out_s);
}
inline const wchar_t *need_unicode_equivalent(const char *in, wchar_t *out=0, int out_s=0)
{
	//use need_unicode_path_to_file (below) for file names
	return need_unicode_equivalent(CP_THREAD_ACP,in,out,out_s); 
}
*/

static int ansi_codepage = AreFileApisANSI()?CP_ACP:CP_OEMCP;
inline const wchar_t *need_unicode_path_to_file(const char *in, wchar_t *out=0, int out_s=0)
{
	assert(EX::ansi_codepage==AreFileApisANSI()?CP_ACP:CP_OEMCP);
	return need_unicode_equivalent(AreFileApisANSI()?CP_ACP:CP_OEMCP,in,out,out_s); 
}

extern bool validating_direct_input_key(unsigned char unx, const char[256]=0);
extern unsigned char translating_direct_input_key(unsigned char x, const char[256]=0);
extern void broadcasting_direct_input_key(unsigned char x, unsigned char unx);
	
////input: ascii_input modes ARE tab num hex int pos AND new 
//
// lead: if lead is 0 then mode is lead and the mode is 'tab'
// note: it used to be of char type, and that's how it's used
extern bool requesting_ascii_input(int mode='tab', int lead=0);
extern bool requesting_unicode_input(int lead=0); //unused 

//displaying: length in returned in [-1] cursor in [-2] 
//
extern const char *returning_ascii; 
extern const char *displaying_ascii_input(const char *cursor="_", size_t=0);
extern const wchar_t *returning_unicode; 
extern const wchar_t *displaying_unicode_input(const wchar_t *cursor=L"_", size_t=0);
extern int inquiring_regarding_input(bool complete=true);

extern const char *inputting_ascii; 
extern const char *retrieving_ascii_input(const char *esc=0);
extern const wchar_t *inputting_unicode; //unused
extern const wchar_t *retrieving_unicode_input(const wchar_t *esc=0);
extern double retrieving_numerical_input(double esc=-0.0);	
extern long long retrieving_hexadecimal_input(long long esc=-1); //UNUSED

extern void setting_default_input(double,long long); 
inline void setting_default_input(double d){ setting_default_input(d,d); }
extern void suspending_requested_input(int term=0);
extern void cancelling_requested_input(int term=0);
//
/////////////////////////////////////////////input////

///*scheduled obsolete*/
//Workpath: returns internal current working directory for Ex.
//			a nonzero input will change the directory provided
//			it is fully qualified and not longer than MAX_PATH.
//			otherwise where input is nonzero, NULL is returned.
//NEW!		As a courtesy relative paths will now be appended.
extern const wchar_t *Workpath(const wchar_t *cd=0);
//
///*of dubious utility*/
static bool fully_qualify_path(wchar_t inout[MAX_PATH], const wchar_t *in=0)
{	
	if(!inout) return false;

	if(!PathIsRelativeW(in?in:inout))
	{
		return in?!wcscpy_s(inout,MAX_PATH,in):true;
	}

	if(!in)
	{
		static wchar_t tmp[MAX_PATH]; wcscpy_s(tmp,MAX_PATH,inout); in = tmp;
	}

	bool out = PathCombineW(inout,EX::Workpath(),in); assert(out);
	
	return out;
}

static bool clearing_a_path_to_file(const wchar_t *not_a_folder)
{
	wchar_t full[MAX_PATH];

	if(!not_a_folder) return false;

	if(EX::fully_qualify_path(full,not_a_folder)) 
	{
		if(!PathFileExistsW(full))
		{
			wchar_t *trim = wcsrchr(full,'\\');

			if(trim) *trim = '\0'; else return false;			
				
			//2021: found out today SHCreateDirectory
			//refuses to parse '/' (returns hresult 3)
			for(int i=0;full[i];i++)
			{
				 if(full[i]=='/') full[i] = '\\'; //FIX ME
			}

			switch(SHCreateDirectoryExW(0,full,0))
			{
			case ERROR_SUCCESS:
			case ERROR_FILE_EXISTS: 
			case ERROR_ALREADY_EXISTS: return true;

			default: return false;
			}
		}
		else return true;
	}
	else return false;
}

#define EX_MAX_CONVERT 2056 //applies for Decode as well

//NEW: These return ASCII only strings in the off chance of an error

//Convert: inout will point to a utf8 internal buffer or to the input
//         if cp is utf8 (65001) and wout if present will point to a
//         utf16 internal buffer. Without wout the output codepage is 
//         returned. If no changes are made/error the return is equal
//         to cp / with wout the length of wout is returned or zero for
//         no changes/error (the inout codepage is understood to be 
//         utf8.) inout_s is set to the length of inout.
//
extern int Convert(int cp, const char **inout, int *inout_s, const wchar_t **wout=0);	

//Encode: unused/unimplemented (should mirror Decode)
//Decode: inout/out will point to an internal buffer where the internal
//		  version of the input will be stored. The output codepage is
//		  returned, or zero for UTF16-LE. To discover the internal code-
//        page, almost assuredly utf8 (65001), call Decode with no argu-
//        ments. To see if a codepage is supported, call Decode with just
//        the codepage argument. If supported the internal codepage is 
//        returned. inout_s is set to the length of inout/out.
//
//NOTICE: w/ inout Decode/Convert behave alike/don't share buffers
extern int Decode(int cp=0, const char **inout=0, int *inout_s=0);
extern int Decode(const wchar_t*,const char **out,int *inout_s=0);

//Translate: used by Ex_detours_TextOutA/DrawTextA respectively
extern int Translate(void*TextOutA,HDC,int*,int*,LPCSTR*,int*,int*);
extern int Translate(void*DrawTextA,HDC,LPCSTR*,int*,LPRECT*,UINT*,int*); 
} //EX

//2021: precompiling some heavies
#ifdef _DEBUG
#include "../Sompaste/Sompaste.h"
#define SOMVECTOR_MATH
#include "../Somplayer/Somvector.h"
#include "../lib/swordofmoonlight.h"
#endif

#endif //EX_INCLUDED