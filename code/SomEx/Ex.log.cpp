
#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

#include "Ex.ini.h"

static size_t Ex_log_top = 1;

static const size_t Ex_log_max = 64;

static struct
{
	const wchar_t *of, *ns;

	int *d, *e, *p, *a, *h, *s; 

}Ex_log[Ex_log_max] = {{L"",L"",0,0,0,0,0,0}};

static int Ex_log_master_switch = 0; //EX::log_ceiling; 

extern int *EX::including_log(const wchar_t *object, const wchar_t *ns, 
int debug[1], int error[1], int panic[1], int alert[1], int hello[1], int sorry[1])
{
#ifdef NDEBUG //Release: initial defaults
	
	if(debug) *debug = -1; 
	if(error) *error = 0;
	if(panic) *panic = 0;
	if(alert) *alert = 0; 
	if(hello) *hello = 0;
	if(sorry) *sorry = 0;

#else //_DEBUG

	if(debug) *debug = 2; 
	if(error) *error = 7;
	if(panic) *panic = 5;
	if(alert) *alert = 3; 
	if(hello) *hello = 2;
	if(sorry) *sorry = 2;

#endif
		
	assert(Ex_log_top!=Ex_log_max);

	if(Ex_log_top>=Ex_log_max)
	{
		Ex_log_top++; return &Ex_log_master_switch;
	}

	//strip out any leading ./ like notation
	for(int paranoia=0;paranoia<MAX_PATH;paranoia++,object++)
	if(*object>='a'&&*object<='z'||*object>='A'&&*object<='Z') break; 	

	bool ok = wcsnlen(object,MAX_PATH)<MAX_PATH; assert(ok);

	Ex_log[Ex_log_top].ns = ok?ns:L"";
	Ex_log[Ex_log_top].of = ok?object:L"";
	Ex_log[Ex_log_top].d = ok?debug:0;
	Ex_log[Ex_log_top].e = ok?error:0;
	Ex_log[Ex_log_top].p = ok?panic:0;
	Ex_log[Ex_log_top].a = ok?alert:0;
	Ex_log[Ex_log_top].h = ok?hello:0;
	Ex_log[Ex_log_top].s = ok?sorry:0;
	
    Ex_log_top++;
	
	return &Ex_log_master_switch;
}

extern int EX::logging(int bar, int logs, const wchar_t *ns, const wchar_t *of)
{
	int out = 0; if(!logs) return 0;

	if(!ns) ns = L""; if(!of) of = L""; //paranoia

	const wchar_t *best = 0; size_t best_s = 0;
	for(int i=min(Ex_log_top,Ex_log_max)-1;i>=0;i--) 
	{
		if(!Ex_log[i].ns||*ns&&*ns!=*Ex_log[i].ns) continue;
		if(!Ex_log[i].of||*of&&*of!=*Ex_log[i].of) continue;

		if(*ns&&wcsncmp(ns,Ex_log[i].ns,8)) continue;
				
		if(*of)
		{
			//must be as good as best candidate to continue
			if(best&&wcsncmp(Ex_log[i].of,best,best_s)) continue;

			size_t j = best_s;
			for(j;Ex_log[i].of[j]&&of[j];j++)
			{
				if(of[j]!=Ex_log[i].of[j])
				{
					wchar_t _ = Ex_log[i].of[j];

					//any non-alphanumeric characters match
					if(of[j]>='a'&&of[j]<='z'||_>='a'&&_<='z'
					 ||of[j]>='A'&&of[j]<='Z'||_>='A'&&_<='Z'
					 ||of[j]>='0'&&of[j]<='9'||_>='0'&&_<='9') 
					{					   
						break; //one or the other was alphanumeric
					}
				}
			}  
			if(j)
			{				
				best = Ex_log[i].of;

				best_s = j-1;
			} 
			if(of[j]) continue;
		}

		if(logs&EX::debug_log&&Ex_log[i].d) *Ex_log[i].d = bar;
		if(logs&EX::error_log&&Ex_log[i].e) *Ex_log[i].e = bar;
		if(logs&EX::panic_log&&Ex_log[i].p) *Ex_log[i].p = bar;
		if(logs&EX::alert_log&&Ex_log[i].a) *Ex_log[i].a = bar;
		if(logs&EX::hello_log&&Ex_log[i].h) *Ex_log[i].h = bar;
		if(logs&EX::sorry_log&&Ex_log[i].h) *Ex_log[i].h = bar;

		out++;
	}

	return out;	
}

extern const wchar_t *EX::is_log_object(const wchar_t *in, bool partial)
{
	if(!in) return 0;

	const wchar_t *out = in, *best = 0;		
	for(int i=min(Ex_log_top,Ex_log_max)-1;i>=0;i--) 	
	{
		//same object as previous
		if(i&&Ex_log[i-1].of==Ex_log[i].of) continue; 

		//simple first character test
		if(!Ex_log[i].of||*in!=*Ex_log[i].of) continue;

		//must be as good as best candidate to continue
		if(best&&wcsncmp(Ex_log[i].of,best,out-in)) continue;

		for(int j=out-in;Ex_log[i].of[j]&&in[j];j++)
		{
			if(in[j]!=Ex_log[i].of[j])
			{
				wchar_t _ = Ex_log[i].of[j];

				//any non-alphanumeric characters match
				if(in[j]>='a'&&in[j]<='z'||_>='a'&&_<='z'
				  ||in[j]>='A'&&in[j]<='Z'||_>='A'&&_<='Z'
				   ||in[j]>='0'&&in[j]<='9'||_>='0'&&_<='9') 
				{
				   break; //one or the other was alphanumeric
				}
				else if(in[j+1]!=Ex_log[i].of[j+1])  
				{
				//assuming non-alphanumerics single periods

				   break; //stop before consuming period				   
				}

			}

			out++; //match
		}

		if(!*out) return out;

		best = Ex_log[i].of;
	}

	if(partial) return out;

	while(out!=in
	 &&(*out>='a'&&*out<='z'
	  ||*out>='A'&&*out<='Z'
	  ||*out>='0'&&*out<='9')) out--;

	if(out==in) return 0;
	
	return out;
}

extern const wchar_t *EX::is_log_namespace(const wchar_t *in)
{
	if(!in) return 0;
		
	const wchar_t *out = in, *best = 0;		
	for(int i=min(Ex_log_top,Ex_log_max)-1;i>=0;i--) 	
	{
		//same object as previous
		if(i&&Ex_log[i-1].ns==Ex_log[i].ns) continue; 

		//simple first character test
		if(!Ex_log[i].ns||*in!=*Ex_log[i].ns) continue;

		//must be as good as best candidate to continue
		if(best&&wcsncmp(Ex_log[i].ns,best,out-in)) continue;
			
		int j;
		for(j=out-in;Ex_log[i].ns[j]&&in[j];j++)
		{
			if(in[j]==Ex_log[i].ns[j]) out++; else break;
		}

		if(!*out) return out;

		best = Ex_log[i].ns;

	//assuming no namespace is a substring of another!

		if(!best[j]) break;
	}

//	if(partial) return out;
	if(out==in) return 0;

	if(*out>='a'&&*out<='z') return 0;
	if(*out>='A'&&*out<='Z') return 0;
	if(*out>='0'&&*out<='9') return 0;
	
	return out;
}


#ifdef _WIN32
HANDLE EX::monolog::handle = 0;
#endif
std::wofstream EX::monolog::log;		   			 
//Unicode codecvt for use by EX::monolog::log only
struct Ex_log_codecvt:std::codecvt<wchar_t,char,mbstate_t>
{
	/*Note: that monolog is flushed by \n output*/
	/*In fact I am unsure it offers any benefits*/
	/*otherwise compared to a wide-binary stream*/

	Ex_log_codecvt(size_t r):std::codecvt<wchar_t,char,mbstate_t>(r){}

	protected: //false: do_out is never called whenever true
	virtual bool do_always_noconv()const throw(){ return false; }
	//virtual bool do_always_noconv()const throw(){ return true; }	
	virtual int do_encoding()const throw(){ return sizeof(wchar_t); }
	virtual int do_max_length()const throw(){ return sizeof(wchar_t); }

	virtual result do_out
	(mbstate_t&, const wchar_t* from, const wchar_t* from_end, 
	 const wchar_t* &from_next, char* to, char* to_end, char*& to_next)const
	{
		result out = ok;
		bool flush = false;
		while(from!=from_end)
		{
			if(*from=='\r')
			{
				from++; //skip
			}
			else if(*from=='\n')
			{
				from++;	flush = true;

				if(EX::crlf[1]&&to_end-to<sizeof(wchar_t)*2) 
				{
					out = partial; break; //can't fit \r\n
				}
				else assert(EX::crlf[0]);

				if(EX::crlf[0]) *((wchar_t*&)to)++ = EX::crlf[0];
				if(EX::crlf[1]) *((wchar_t*&)to)++ = EX::crlf[1];
			}
			else if(to_end-to<sizeof(wchar_t))
			{
				out = partial; break;
			}
			else *((wchar_t*&)to)++ = *from++;
		}

		from_next = from; to_next = to;

		if(flush)
		{
			EX::monolog::log.setf(std::ios_base::unitbuf);
		}
		else EX::monolog::log.unsetf(std::ios_base::unitbuf);

		return out;
	}
	virtual result do_in
	(mbstate_t&, const char* from, const char* from_end, 
	 const char* &from_next, wchar_t* to, wchar_t* to_end, wchar_t*& to_next)const
	{
		assert(0); //unexpected
		size_t len = (from_end-from); memcpy(to,from,len);
		from_next = from_end; to_next = to+(len/sizeof(wchar_t));
		return ok;
	}
	virtual result do_unshift(mbstate_t&, char* to, char*, char* &to_next)const
	{
		to_next = to; return noconv;
	}
	virtual int do_length(mbstate_t&, const char* from, const char* end, size_t max)const
	{
		return (int)((max<(size_t)(end-from))?max:(end-from));
	}
};

extern void EX::closing_log()
{
	EX::logging_off();
	if(!EX::monolog::log.is_open())
	return;
	EX::monolog::log.close();
	EX::monolog::log.~basic_ofstream();
	//CloseHandle(EX::monolog::handle);
	new(&EX::monolog::log) std::wofstream;
}
extern int &EX::logging_onoff(bool flip)
{
	if(!flip) return Ex_log_master_switch;
	
	Ex_log_master_switch = Ex_log_master_switch?0:EX::log_ceiling;
	
	static bool opened = 0; if(opened) 
	{	
		//2018: SOM_EX is always on/so has to change files
		if(Ex_log_master_switch
		&&!EX::monolog::log.is_open())
		goto reopen;

		return Ex_log_master_switch;
	}
	else opened = 1; reopen:

	EX::INI::Output(); //REMOVE ME? (log extensions)

	wchar_t log[MAX_PATH] = L""; 
	swprintf_s(log,L"%ls\\%ls.log",EX::user(1),EX::log());

	//binary: required by Ex_log_codecvt (Unicode)
	std::ios_base::openmode mode = std::ios_base::binary; 

	if(!GetEnvironmentVariableW(log,0,0)
	   //unforgivable XP bug:
	   //http://stackoverflow.com/questions/20436735/
	   &&GetLastError()!=ERROR_MORE_DATA) 
	{
		SetEnvironmentVariableW(log,L"");  
	}
	else mode|=std::ios_base::app; //hack

	#ifdef _WIN32
	{
		wchar_t wb[] = L"wb";
		if(mode&std::ios_base::app) wb[0] = 'a';

		FILE *f = _wfopen(log,wb); 
		EX::monolog::handle = (HANDLE)_get_osfhandle(fileno(f));

		//(nonstandard Microsoft extension)
		EX::monolog::log.~basic_ofstream();		
		new (&EX::monolog::log) std::wofstream(f); 
	}
	#else
	{
		EX::monolog::log.open(log,mode);
	}
	#endif									  	
	
	//won't work at global scope???
	static Ex_log_codecvt facet(1); 
	//won't work as a temporary object
	static std::locale locale(std::locale::classic(),&facet);
	//static: cross our fingers dtor wise
	EX::monolog::log.imbue(locale);

	if(~mode&std::ios_base::app)
	{
		const wchar_t feff = 0xfeff; //BOM
		EX::monolog::log << feff << "Routing logs to\n"; //L"‚l‚r –¾’©\n" 
	}
	else EX::monolog::log << "Joining logs to\n";

	EX::monolog::log << log << '\n';		
	EX::monolog::log << _wstrdate(log) << ' ';
	EX::monolog::log << _wstrtime(log) << '\n'
	<< GetCommandLineW() << '\n';

	return Ex_log_master_switch;
}

static std::vector<EX::temporary*> Ex_log_temp;
extern void Ex_log_terminating()
{
	while(!Ex_log_temp.empty())
	Ex_log_temp.back()->~temporary();
}
extern void Ex_log_created(EX::temporary *p)
{
	Ex_log_temp.push_back(p);
}
extern void Ex_log_deleted(EX::temporary *p)
{
	for(size_t i=Ex_log_temp.size();i-->0;)
	if(p==Ex_log_temp[i])
	Ex_log_temp.erase(Ex_log_temp.begin()+i);
}

const wchar_t *EX::crash_reporter::app1 = L"";
const wchar_t *EX::crash_reporter::app2 = L"";

//https://stackoverflow.com/questions/20237201/best-way-to-have-crash-dumps-generated-when-processes-crash
LPTOP_LEVEL_EXCEPTION_FILTER EX::crash_reporter::m_lastExceptionFilter = NULL;
LONG WINAPI EX::crash_reporter::UnHandledExceptionFilter(struct _EXCEPTION_POINTERS *exceptionPtr)
{
			Ex_log_terminating(); //2021: remove temporary files?

			//SetUnhandledExceptionFilter docs say it's not called if
			//a debugger is attached
			//if(IsDebuggerPresent()) return EXCEPTION_CONTINUE_SEARCH;
			assert(!IsDebuggerPresent());

			wchar_t dumpFilePath[MAX_PATH]; ExpandEnvironmentStringsW(app1,dumpFilePath,MAX_PATH);

						//not my code//

	typedef BOOL (WINAPI *MiniDumpWriteDumpFunc)(HANDLE hProcess, DWORD ProcessId
        , HANDLE hFile
        , MINIDUMP_TYPE DumpType
        , const MINIDUMP_EXCEPTION_INFORMATION *ExceptionInfo
        , const MINIDUMP_USER_STREAM_INFORMATION *UserStreamInfo
        , const MINIDUMP_CALLBACK_INFORMATION *Callback
    );

    //we load DbgHelp.dll dynamically, to support Windows 2000
    if(HMODULE hModule=::LoadLibraryA("DbgHelp.dll"))
	if(MiniDumpWriteDumpFunc dumpFunc = reinterpret_cast
	  <MiniDumpWriteDumpFunc>(::GetProcAddress(hModule, "MiniDumpWriteDump")))
	{
        //fetch system time for dump-file name
        SYSTEMTIME  SystemTime; ::GetLocalTime(&SystemTime);

        //choose proper path for dump-file
        //wchar_t dumpFilePath[MAX_PATH] = {};
		//_snwprintf_s(dumpFilePath,MAX_PATH, 
			size_t cat = wcslen(dumpFilePath);        
			swprintf_s(dumpFilePath+cat,MAX_PATH-cat, 
				L"%s_crash_%04d-%d-%02d_%d-%02d.dmp",app2 //-%02d
				, SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay
                , SystemTime.wHour, SystemTime.wMinute//, SystemTime.wSecond
            );

        //create and open the dump-file
        HANDLE hFile = ::CreateFileW( dumpFilePath, GENERIC_WRITE
                , FILE_SHARE_WRITE
                , NULL
                , CREATE_ALWAYS
                , FILE_ATTRIBUTE_HIDDEN
                , NULL
            );
        if(hFile!=INVALID_HANDLE_VALUE) 
		{
            _MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
            exceptionInfo.ThreadId          = GetCurrentThreadId();
            exceptionInfo.ExceptionPointers = exceptionPtr;
            exceptionInfo.ClientPointers    = NULL;
                
			//at last write crash-dump to file
            bool ok = dumpFunc(::GetCurrentProcess(), ::GetCurrentProcessId()
                    , hFile, MiniDumpNormal
                    , &exceptionInfo, NULL, NULL
                );
            //dump-data is written, and we can close the file
            CloseHandle(hFile);
                
            //Return from UnhandledExceptionFilter and execute the associated exception handler.
            //  This usually results in process termination.
            if(ok) return EXCEPTION_EXECUTE_HANDLER;
        }
    }
    //Proceed with normal execution of UnhandledExceptionFilter.
    //  That means obeying the SetErrorMode flags,
    //  or invoking the Application Error pop-up message box.
    return EXCEPTION_CONTINUE_SEARCH;
}