
#ifndef EX_HPP_INCLUDED
#define EX_HPP_INCLUDED

#define EX_WIDEN2(x) L ## x
#define EX_WIDEN(x) EX_WIDEN2(x)
#define EX_CSTRING2(x) #x
#define EX_CSTRING(x) EX_CSTRING2(x)
#define EX_WCSTRING2(x) L#x
#define EX_WCSTRING(x) EX_WCSTRING2(x)

#define EX_ARRAYSIZEOF(array) (sizeof(array)/sizeof(array[0]))
#define EX_COUNTER(x) static const int ex_counter_##x = 1+__COUNTER__;
#define EX_COUNTOF(x) __COUNTER__ - ex_counter_##x

#include <DbgHelp.h> //EX::crash_reporter
#ifdef _DEBUG	  
#include <crtdbg.h>
#define EX_BREAKPOINT(x) {static int breakpoint_##x=0; assert(breakpoint_##x++);}
#define EX_CHECKPOINT(x) {static bool checkpoint_##x=0; assert(checkpoint_##x++||_CrtCheckMemory()); }
#define EX_UNFINISHED
#else 
#define EX_UNFINISHED    unfini$hed //cause syntax error
#define EX_CHECKPOINT(x) {}
#define EX_BREAKPOINT(x) {}
#endif 

#include "../Exselector/JoyShockLibrary/JoyShockLibrary/JoyShockLibrary.h"

//allow for overlapping enum types 
//originally of the EXML namespace
#define EX_ENUMSPACE(ctor,Type) e;\
inline operator Type(){ return e; }\
inline explicit ctor(int i){ e = (Type)i; }\
inline ctor(){} typedef Type T;\
inline ctor(Type ofe){ e = ofe; }\
inline Type operator=(Type ofe){ return e = ofe; }
						 
namespace EX
{
#ifdef _DEBUG
enum{ debug=1 };
#else
enum{ debug=0 };
#endif

#ifdef __APPLE__
static const wchar_t crlf[] = L"\r";
#elif defined(_WIN32)
static const wchar_t crlf[] = L"\r\n";
#else
static const wchar_t crlf[] = L"\n";
#endif
  
template<class S, class T=S> class preset //RAII
{
	S *_p; T _reset; public:
	preset(S &s, T reset){ _p = &s; _reset = reset; }
	~preset(){ *_p = _reset; }
};

class temporary
{	
	HANDLE file; WCHAR *path; 

public:

	temporary()
	{
		WCHAR temp[MAX_PATH];
		GetTempPathW(MAX_PATH,temp);
		GetTempFileNameW(temp,EX_WCSTRING(EX_INCLUDED),0,temp);
		size_t len = wcslen(temp);
		wmemcpy(path=new WCHAR[len+1],temp,len+1);
		//REMINDER: GetTempFileName CREATES A FILE
		//trying FILE_SHARE_DELETE+FILE_FLAG_DELETE_ON_CLOSE
		//(seems to work? maybe it's not getting deleted???)
		SetFileAttributesW(path,
		//https://blogs.msdn.microsoft.com/oldnewthing/20160108-00/?p=92821
		FILE_ATTRIBUTE_TEMPORARY/*|FILE_FLAG_DELETE_ON_CLOSE*/);
		file = CreateFileW(path,
		GENERIC_READ|GENERIC_WRITE,
		FILE_SHARE_READ|FILE_SHARE_WRITE, //|FILE_SHARE_DELETE,
		0,OPEN_EXISTING,0,0); 
		assert(file&&file!=INVALID_HANDLE_VALUE);		
	}
	temporary(bool needed) //RAII model support (USED?)
	{
		file = 0; path = 0; if(needed) new(this)temporary;

		extern void Ex_log_created(temporary*); Ex_log_created(this);
	}	
	//https://blogs.msdn.microsoft.com/oldnewthing/20160108-00/?p=92821
	~temporary() //FILE_FLAG_DELETE_ON_CLOSE precludes sharing 
	{
		if(file) CloseHandle(file); if(path){ DeleteFileW(path); delete[] path; }
		file = 0; path = 0;
		extern void Ex_log_deleted(temporary*); Ex_log_deleted(this);
	}		
	inline operator const WCHAR*(){ return path; }
	inline operator HANDLE(){ return file; }

	//copy by HANDLE returning the number of bytes successfully copied
	template<size_t buffer_size> DWORD copy(HANDLE cp, size_t cp_s=-1)
	{
		DWORD out = 0; BYTE swap[buffer_size]; 
		for(DWORD rd=1,wr=1;cp_s&&rd&&wr&&wr==rd;cp_s-=rd,out+=wr)
		(ReadFile(cp,swap,min(cp_s,sizeof(swap)),&rd,0)
		,WriteFile(file,swap,rd,&wr,0)); return out;
	}
	HANDLE dup() //use dup to return a HANDLE to be an unwitting party
	{
		HANDLE out,gcp = GetCurrentProcess(); 
		DuplicateHandle(gcp,file,gcp,&out,0,0,DUPLICATE_SAME_ACCESS);
		return out;
	}
};
static bool is_temporary(HANDLE test)
{
	BY_HANDLE_FILE_INFORMATION bhfi;
	if(GetFileInformationByHandle(test,&bhfi))
	return bhfi.dwFileAttributes&FILE_ATTRIBUTE_TEMPORARY;
	return false;
}

template<class T, size_t N=1> class thread
{	
	DWORD out; public: /*Usage:
	*
	//create two TLS indices
	static const DWORD tls = EX::tls(-2); 
	*
	//memory is _always_ expanded as necessary
	wchar_t *p = EX::thread<wchar_t,MAX_PATH>(tls);
	wchar_t *q = EX::thread<wchar_t,MAX_PATH>(tls+1);
	*
	*/
	inline thread(DWORD get, size_t n=1)
	{
		assert(short(sizeof(T)*N*n)>0);
		out = EX::tls(sizeof(T)*N*n,get);
	} 
	inline T &operator=(const T &cp)
	{
		return *(T*)out = cp;
	}
	inline operator T&(){ return *(T*)out; }
	inline operator T*(){ return (T*)out; }	
};

//http://joeduffyblog.com/2006/08/22/priorityinduced-starvation-why-sleep1-is-better-than-sleep0-and-the-windows-balance-set-manager/
inline void sleep(int ms=1){ Sleep(ms); }

#define EX_CRITICAL_SECTION \
static EX::critical c__; EX::section s__(c__);
struct critical
{	
	//TODO: need XP bypass for InitializeCriticalSectionEx(CRITICAL_SECTION_NO_DEBUG_INFO)
	//(DX_CRITICAL_SECTION too)
	CRITICAL_SECTION _CS_; //DUPLICATE (directx.h)
	critical(DWORD sc=~0){ InitializeCriticalSectionAndSpinCount(&_CS_,sc==~0?4000:sc); }
	~critical(){ DeleteCriticalSection(&_CS_); }	
};
struct section
{	
	CRITICAL_SECTION *_CS_; //private
	section(critical &cs):_CS_(&cs._CS_){ EnterCriticalSection(_CS_); }
	section(critical *cs):_CS_(cs?&cs->_CS_:0){ if(_CS_) EnterCriticalSection(_CS_); }
	section(CRITICAL_SECTION*cs):_CS_(cs){ if(_CS_) EnterCriticalSection(_CS_); }
	~section(){ if(_CS_) LeaveCriticalSection(_CS_); }
};

//https://docs.microsoft.com/en-us/windows/win32/dxtecharts/crash-dump-analysis#analyzing-a-minidump
//https://stackoverflow.com/questions/20237201/best-way-to-have-crash-dumps-generated-when-processes-crash
class crash_reporter
{
public: //TODO: extract file name and version with Win32 API!
		static const wchar_t *app1,*app2;

    inline crash_reporter(const wchar_t *d, const wchar_t *a){ Register(); app1 = d; app2 = a; }
    inline ~crash_reporter(){ Unregister(); }

    inline static void Register() 
	{
        if(m_lastExceptionFilter)
		{
            fprintf(stdout,"EX::crash_reporter: is already registered\n");
            fflush(stdout);
        }
        SetErrorMode(SEM_FAILCRITICALERRORS);
        //ensures UnHandledExceptionFilter is called before App dies.
        m_lastExceptionFilter = SetUnhandledExceptionFilter(UnHandledExceptionFilter);
    }
    inline static void Unregister() 
	{
        SetUnhandledExceptionFilter(m_lastExceptionFilter);
    }

private:
			//these are currently defined in Ex.log.cpp
			//for lack of a better home

    static LPTOP_LEVEL_EXCEPTION_FILTER m_lastExceptionFilter;
    static LONG WINAPI UnHandledExceptionFilter(_EXCEPTION_POINTERS *);
};

//DEBUGGING
// 
// WARNING: THESE ARE LIMITED TO 4 PER THREAD, SO MAKE SURE TO CLEAN
// UP DEBUGGING CODE!
// 
//these set hardware breakpoints. I don't know if they will conflict
//with the VS debugger. especially if you use the "code" breakpoints
//https://www.codeproject.com/Articles/28071/Toggle-hardware-data-read-execute-breakpoints-prog
class data_breakpoint
{
	HANDLE _h;

public: //Ex.debug.cpp (2021)

	//sz: if 0 a bonus code breakpoint (not data) will be set
	//in addition, sz must be 1, 2, 4, or 8
	//rw: normally break is triggered on write
	data_breakpoint(HANDLE thread, void *addr, int sz=sizeof(void*), bool rw=false);
	~data_breakpoint();
};

} //EX

#endif //EX_HPP_INCLUDED