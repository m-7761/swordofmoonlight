		
#include "Somversion.pch.h"

#include "../COLLADA-SOM/COLLADA-SOM.h"

static SOMVERSION Somversion_error(const wchar_t *msg=0)
{	
	return SOMVERSION(0,0,0,0); //TODO: error message
}

static SOMVERSION Somversion_quartet(const wchar_t *pe, bool *debug=0)
{	
	if(!pe||!*pe) return Somversion_error();

	DWORD nul = 0; //docs say not used

	UINT sz = GetFileVersionInfoSizeW(pe,&nul);
	
	if(!sz||sz>65535) return Somversion_error();

	BYTE *v = new BYTE[sz];
	
	VS_FIXEDFILEINFO *f = 0;
		
	if(!GetFileVersionInfoW(pe,0,sz,v)
	 ||!VerQueryValueW(v,L"\\",(void**)&f,&sz)) 
	{
		delete [] v; return Somversion_error();
	}
	//else delete [] v; //yikes: f points into v

	if(debug) *debug = f->dwFileFlags&VS_FF_DEBUG;

	SOMVERSION out 
	(
	HIWORD(f->dwFileVersionMS),
	LOWORD(f->dwFileVersionMS),
	HIWORD(f->dwFileVersionLS),
	LOWORD(f->dwFileVersionLS)
	);

	delete [] v; //!
	return out;
}

static bool Somversion_locked(const wchar_t *in)
{
	if(!in||!*in) return false;

	SetLastError(0); 

	HANDLE close = 
	CreateFileW(in,GENERIC_WRITE,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);

	if(GetLastError()) return true;

	CloseHandle(close);

	return false;
}

static time_t Somversion_compare(const wchar_t *f, const wchar_t *g)
{
	//NOTE: It seems Windows' stat implementation is a magical beast.
	//So maybe instead this should use CompareFileTime. But is should
	//not matter since they are not stored.
	struct _stat fst, gst; 
	
	if(_wstat(f,&fst)) fst.st_mtime = 0;
	if(_wstat(g,&gst)) gst.st_mtime = 0; 

	return fst.st_mtime-gst.st_mtime;	
}

void Somversion_stamp(const wchar_t *f, const wchar_t *g = 0)
{
	if(f&&*f&&!PathIsRelativeW(f))
	{
		//https://support.microsoft.com/en-us/help/190315/last-accessed-time-and-last-modified-time-reported-by-c-run-time-crt-f
		//struct _stat fst; 
		//if(_wstat(f,&fst))
		FILETIME mt;
		HANDLE crud = CreateFileW(f,GENERIC_READ,7,0,OPEN_EXISTING,0,0);
		if(!GetFileTime(crud,0,0,&mt))
		{			
			CloseHandle(crud);
			wchar_t errmsg[MAX_PATH*2];		  
			swprintf_s(errmsg,L"Failed to stat %s",f);
			MessageBoxW(0,errmsg,L"SOM Registry Error",MB_OK); 
			return;
		}	
		CloseHandle(crud);

		HKEY stamp = 0; //must open in order to write to value (not subkey)
		RegOpenKeyExW(HKEY_CURRENT_USER,L"SOFTWARE\\FROMSOFTWARE\\SOM",0,KEY_SET_VALUE,&stamp);

		//Maybe shouldn't save this as binary? It says stat uses FileTimeToLocalFileTime???
		//I don't understand why time_t is different for timezones???
		//Article name:
		//Last accessed time and last modified time reported by C Run-Time (CRT) functions can be adjusted by using the "Automatically Adjust for Daylight Saving Time" option
		//https://support.microsoft.com/en-us/help/190315/last-accessed-time-and-last-modified-time-reported-by-c-run-time-crt-f
		/*https://metacpan.org/pod/Win32::UTCFileTime
		The problem with Microsoft's stat(2) and utime(2), and hence Perl's built-in stat(), lstat() and utime() when built with the Microsoft C library, is basically this: 
		file times reported by stat(2) or stored by utime(2) may change by an hour as we move into or out of daylight saving time (DST) if the computer is set to "Automatically
		adjust clock for daylight saving changes" (which is the default setting) and the file is stored on an NTFS volume. */
		//LONG err = RegSetValueExW(stamp,f,0,REG_BINARY,(BYTE*)&fst.st_mtime,sizeof(time_t));	
		LONG err = RegSetValueExW(stamp,f,0,REG_BINARY,(BYTE*)&mt,sizeof(mt));	

		if(err!=ERROR_SUCCESS)
		{
			wchar_t errmsg[MAX_PATH*2];		  
			swprintf_s(errmsg,L"Failed to write stat for %s",f);
			MessageBoxW(0,errmsg,L"SOM Registry Error",MB_OK); 
			return;
		}
	}

	if(g) Somversion_stamp(g);
}

bool Somversion_clear(const wchar_t *f, const wchar_t *g = 0)
{
	bool out = false;

	if(f&&*f&&!PathIsRelativeW(f))
	{
		//See Somversion_stamp comments.
		//time_t mt = 0;
		FILETIME mt = {};
		DWORD sizeof_mt = sizeof(mt);

		SHGetValueW(HKEY_CURRENT_USER,
		L"SOFTWARE\\FROMSOFTWARE\\SOM",f,0,&mt,&sizeof_mt);
		//if(mt)
		if(mt.dwLowDateTime||mt.dwHighDateTime)
		{
			//struct _stat fst; 
			//if(_wstat(f,&fst)) fst.st_mtime = 0;
			FILETIME cmp;
			HANDLE crud = CreateFileW(f,GENERIC_READ,7,0,OPEN_EXISTING,0,0);
			if(GetFileTime(crud,0,0,&cmp))
			{			
				//out = fst.st_mtime==mt;
				out = 0==CompareFileTime(&mt,&cmp);
			}
			else out = false; CloseHandle(crud);
		}
		else out = false;
	}

	if(g&&out) out = Somversion_clear(g);

	return out;
}

void Somversion_touch(const wchar_t *f, const wchar_t *min = 0)
{
	HANDLE file =
	CreateFileW(f,FILE_WRITE_ATTRIBUTES,0,0,OPEN_EXISTING,0,0);
	if(!file||file==INVALID_HANDLE_VALUE) return;
					  
	FILETIME ft; SYSTEMTIME st;
    
	GetSystemTime(&st);           
	SystemTimeToFileTime(&st,&ft);

	if(min&&*min)
	{
		HANDLE fmin =
		CreateFileW(min,FILE_WRITE_ATTRIBUTES,0,0,OPEN_EXISTING,0,0);

		if(fmin&&fmin!=INVALID_HANDLE_VALUE)
		{
			FILETIME mint;
			GetFileTime(fmin,0,0,&mint);
			SetFileTime(file,0,0,CompareFileTime(&mint,&ft)>0?&mint:&ft);
		}
		else SetFileTime(file,0,0,&ft);

		CloseHandle(fmin);
	}
	else SetFileTime(file,0,0,&ft);

	CloseHandle(file);
}

extern void Somversion_update(const wchar_t *csv, const wchar_t *dll, HWND hwnd)
{		
	wchar_t som[MAX_PATH] = L"";
	GetModuleFileNameW(0,som,MAX_PATH);

	if(!*som||!*csv||!*dll) return; //unimplemented
	
	wchar_t procedure[MAX_PATH*2];		
	swprintf_s(procedure,L"%ls\n"
	L"\n"
	L"Because this file houses this updater, the host program must\n"
	L"be start over so it can assist in this one-of-a-kind procedure.\n"
	L"\n"		
	L"(OK means anything remaining in the queue is discarded!)\n",
	dll);
	if(IDOK!=MessageBoxW(hwnd,procedure,L"Software reset",MB_OKCANCEL))
	return;				  
											  
	Somversion_touch(csv,dll);	
	ShellExecuteW(hwnd,L"open",som,0,0,SW_SHOWNORMAL);
	Sleep(33); //can't hurt
	ExitProcess(0);
}

//NEW: Add DAE to file types SOM.exe knows about.
static bool Somversion_try_COLLADA_SOM()
{
	if(!Somdialog_csv.empty())
	return false;

	//HACK: Process the command-line at most once.
	static bool once = false;
	if(once) return false; once = true;

	int argc = 0;
	LPWSTR cli = GetCommandLineW();
	LPWSTR *argv = CommandLineToArgvW(cli,&argc);

	for(int i=1;i<argc;i++)
	if(0==wcsicmp(L".dae",PathFindExtensionW(argv[i])))
	{
		//HACK: Somversion.hpp isn't working. It's
		//possible this scenario wasn't considered.
		wchar_t pe[MAX_PATH] = L"COLLADA-SOM.dll";
		SOMVERSION_LIB(Version)(0,pe,-1); 
		LoadLibraryW(pe);

		argv[0] = nullptr; //Signal noninteractive.

		if(0!=COLLADA_SOM(argc,argv)) MessageBeep(-1);

		return true; //Suppress application dialog.
	}

	return false;
}

static SOMVERSION Somversion_internal(HWND parent, wchar_t *pe, int timeout, bool assist)
{	
	assert(pe);

	if(!pe) return Somversion_error();

	if(timeout!=SOMVERSION::noupdate) //courtesy
	{
		static int once = (Somtext(0),1); //setup language?
	}

	//hacK: File->Open & forced update mode
	if(!*pe&&timeout<SOMVERSION::notimeout) //SOM.exe?
	{
		SOMVERSION bogus;
		//HACK: Shoehorn a COLLADA CLI?
		if(!Somversion_try_COLLADA_SOM())		
		Somdialog(timeout,0,pe,bogus,parent);
		return bogus;
	}

	wchar_t *dot = 0, swap[MAX_PATH]; 

	//scheduled obsolete
	//TODO: JUST REPLACE pe WITH csv
	if(PathIsRelativeW(pe))
	{		
		//TODO: issue warning
		wcscpy_s(swap,MAX_PATH,pe);
			   
		//hack: SearchPath is safer than it seems
		//as the .csv file must match and be valid 
		if(!SearchPathW(0,swap,0,MAX_PATH,pe,&dot)) 
		{
			if(dot=wcsrchr(swap,'.'))
			{
				wcscpy_s(dot,MAX_PATH-(dot-swap),L".csv");

				if(!SearchPathW(0,swap,0,MAX_PATH,pe,&dot)) 
				return Somversion_error();
			}
			else return Somversion_error();
		}
	}
	else if(!PathFileExistsW(pe))
	{
		if(dot=wcsrchr(pe,'.'))
		{
			wcscpy_s(dot,MAX_PATH-(dot-pe),L".csv");
		}
		else return Somversion_error();
	}

	dot = wcsrchr(dot?dot:pe,'.');

	DWORD nul = 0, dot_s = dot?dot-pe:wcslen(pe);

	wchar_t csv[MAX_PATH] = L""; 
	
	if(wcsicmp(dot,L".csv")) 
	{
		wcscpy(csv,pe); 
		wcscpy_s(csv+dot_s,MAX_PATH-dot_s,L".csv"); 
		if(!PathFileExistsW(csv)) *csv = 0;
	}
	else //PE is CSV file
	{
		wcscpy_s(csv,pe);
		//HACK: it's not super clear just yet how to 
		//extract the PE file name from the csv file
		//So we just assume PE is a DLL when missing
		wcscpy_s(pe+dot_s,MAX_PATH-dot_s,L".dll");		
	}

	bool debug = false;
	SOMVERSION out = Somversion_quartet(pe,&debug);	
			 
	//2: skips lock
	if(timeout>=-1)
	//0: force skip menu
	if(0==timeout||!*csv 
	||out&&!assist&&Somversion_locked(pe))
	{
		if(out&&assist) pe[MAX_PATH] = '\0';

		return out; //version only
	}

	bool finish = false; //changed
	bool clear = debug||Somversion_clear(csv); //pe,

	//NEW: force open version menu
	if(timeout<=-2) clear = false; //hack 

	//facilitate automatic updates
	if(!out||!clear||Somversion_compare(csv,pe)>0)
	if(finish=Somdialog(timeout,csv,pe,out,parent,assist))
	{
		if(assist)
		{
			out = Somversion_quartet(pe+MAX_PATH);
		}
		else out = Somversion_quartet(pe);

		Somversion_stamp(csv); //pe,
	}
	else if(out&&timeout!=0)
	{
		wchar_t suppress[MAX_PATH*2];		  

		swprintf_s(suppress,
		L"The automatic update procedure was unable to finish.\n"
		L"\n"
		L"Would you like to skip this component update from now on?\n"
		L"\n"
		L"Yes will set the component file time to the current time.",
		pe);

		int yes = 
		MessageBoxW(0,suppress,L"SOM Follow-up",MB_YESNO|MB_DEFBUTTON2);

		if(yes==IDYES) 
		{
			Somversion_touch(pe,csv);
			Somversion_stamp(csv); //pe,
		}
	}
	
	if(out&&assist&&!finish) pe[MAX_PATH] = '\0';

	return out;
}

SOMVERSION_API C_SOMVERSION SOMVERSION_LIB(Version)(HWND parent, wchar_t pe[MAX_PATH], int timeout)
{
	Somfont(); //running as a courtesy for now (so other apps don't need to)

	pe[MAX_PATH-1] = '\0'; //paranoia

	return Somversion_internal(parent,pe,timeout,false);
}

SOMVERSION_API C_SOMVERSION SOMVERSION_LIB(Assist)(HWND parent, wchar_t pe[2][MAX_PATH], int timeout)
{
	if(!*pe[1])
	{
		DWORD len = GetModuleFileNameW(0,pe[1],MAX_PATH);

		DWORD mov = len; while(mov&&pe[1][mov]!='\\') mov--; 
		
		if(len-mov) wmemmove(pe[1],pe[1]+mov+1,len-mov); 
	}

	pe[0][MAX_PATH-1] = '\0'; pe[1][MAX_PATH-1] = '\0'; //paranoia

	SOMVERSION out = Somversion_internal(parent,pe[0],timeout,true);

	if(!out) *pe[1] = '\0'; return out;
}

SOMVERSION_API wchar_t *SOMVERSION_LIB(Temporary)(wchar_t inout[MAX_PATH], int slash)
{	
	//Somfolder expects a slash.
	bool slashed = inout[0]=='/'||inout[0]=='\\';
	
	//Save separator to restore it.
	wchar_t sep = inout[slash];

	//This API can make a folder, or a folder+file.
	Somlegalize(slashed?inout+1:inout); 

	//Repair folder/file separator.
	if(sep=='/'||sep=='\\') inout[slash] = '\\'; 
	
	//Prepend TEMP folder, and make the directory.
	if(slashed||inout[0]=='\0') Somfolder(inout,sep=='\0'); return inout;
}
 
static HMODULE Somversion_hmodule = 0;

extern HMODULE Somversion_dll()
{
	return Somversion_hmodule;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	if(ul_reason_for_call==DLL_PROCESS_ATTACH)
	{	
		Somversion_hmodule = hModule;

		Somfolder(0); //hack: initialize temporary folder
	}
	else if(ul_reason_for_call==DLL_PROCESS_DETACH)
	{

	}

    return TRUE;
} 