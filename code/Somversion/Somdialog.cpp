
#include "Somversion.pch.h"

extern std::deque<Somdialog_csv_vt> //deque?
Somdialog_csv(0,Somdialog_csv_vt());

//2021: try to lower splash screen so component update 
//isn't stuck behind it
extern bool Somdialog_unsplash(HWND closer, LPARAM sw)
{
	HWND win = 0;

	//extern HWND SOM::splash() //som.exe.cpp
	//{
		wchar_t splash[9] = L"0";
		GetEnvironmentVariableW(L"SomEx.dll Splash Screen",splash,9);
		win = (HWND)wcstoul(splash,0,16);
	//}

	//extern bool SOM::splashed(HWND closer, LPARAM sw) //som.exe.cpp
	{
		//says invisible when starting up with a MessageBox displayed
		//(this is probably related to Windows Vista phantom windows)
		if(IsWindow(win)) //if(IsWindowVisible(win))
		{
			//NOTE: this should prevent entering here more than once
			SetEnvironmentVariableW(L"SomEx.dll Splash Screen",L"0");

			//2018: set to 0 to disable (or minimize?) splash screen
		//	DWORD to = min(3000,EX::INI::Window()->splash_screen_ms_timeout);
		//	if('mpx'==SOM::image()) //MapComp.exe
		//	to = 0;
			DWORD to = 3000;

			//speeding up because Windows 10 Creators Update introduced
			//a race condition, mainly because it seems to take a while
			//to get the splash screen u
		//	DWORD spell = EX::tick()-GetWindowLong(win,DWL_USER);
			DWORD spell = GetTickCount()-GetWindowLong(win,DWL_USER);
			//2017: Windows 10 goes to sleep and never wakes up???
			//if(spell<2500) Sleep(2500-spell); //3000
			if(spell<to) Sleep(to-spell); //3000
			//NOT SUBCLASSED
			//if(sw&&closer) DefSubclassProc(closer,WM_SHOWWINDOW,1,sw);
			if(sw&&closer) SendMessage(closer,WM_SHOWWINDOW,1,sw);
			//hack: hWnd will get reactivated in case
			//users try to click on the splash screen
			SendMessage(win,WM_CLOSE,(WPARAM)closer,0);	
			return true;
		}
	}
	return false;
}

static const char *Somdialog_print(time_t t)
{
	char *out = ctime(&t); return out?out:"<unable to print time>";
}
static const char *Somdialog_print(const FILETIME &ftWrite)
{								 
	static char lpszString[64];	 
	strcpy(lpszString,"<unable to print time>");	

    SYSTEMTIME stUTC, stLocal;
    FileTimeToSystemTime(&ftWrite,&stUTC);
    SystemTimeToTzSpecificLocalTime(0,&stUTC,&stLocal);

    sprintf_s(lpszString,  
    "%02d/%02d/%d  %02d:%02d",
    stLocal.wMonth,stLocal.wDay,stLocal.wYear,
    stLocal.wHour,stLocal.wMinute);

    return lpszString;
}
static HANDLE Somdialog_lock(const wchar_t pe[MAX_PATH])
{		
	SetLastError(0); 

	HANDLE out = CreateFileW(pe,GENERIC_WRITE|GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);

	if(GetLastError()&&out==INVALID_HANDLE_VALUE) //paranoia
	{
		Sleep(333); //Have observed Windows releasing a write lock only after second attempt

		out = CreateFileW(pe,GENERIC_WRITE|GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);

		if(GetLastError()) return 0; //TODO: Warning message
	}
	return out;
}
//scheduled obsolete
//(where did this come from?)
static LONG DeleteTemporaryAndAllSubfolders(LPCWSTR wzDirectory)
{
	//+1 for 00 terminator
    WCHAR szDir[MAX_PATH+1];
	SHFILEOPSTRUCTW fos; memset(&fos,0x00,sizeof(fos));
	wcscpy_s(szDir,MAX_PATH,wzDirectory);
	//double null terminate for SHFileOperation
    int len = lstrlenW(szDir); szDir[len+1] = 0; 
    //delete the folder and everything inside
    fos.wFunc = FO_DELETE; fos.pFrom = szDir; 	
	fos.fFlags = FOF_SILENT|FOF_NOCONFIRMATION;
    return SHFileOperationW(&fos);
}

typedef struct //careful: POD for now
{
	int ms, ref; 

	wchar_t *pe; const wchar_t *csv;	

	SOMVERSION v;

	HANDLE lock;

	FILE *_; //csv file (rt mode)

	wchar_t base[MAX_PATH]; //of remote urls
	wchar_t work[MAX_PATH]; //copy of archive
	wchar_t home[MAX_PATH]; //of local files
	wchar_t save[MAX_PATH]; //backup/assist

	const wchar_t *ext;
	wchar_t shortname[32];
	
	HWND hwnd;

	int step, item;		
	int downloading, caching, installing; 

	size_t counter, charge, commit; //int

	bool restore; //backup
	bool assist;
	bool out;

	//NEW: ID_FILE_OPEN and WM_DROPFILES
	bool backup(const wchar_t file[MAX_PATH])
	{
		if(assist) return wcscpy(save,file); 

		wcscpy(save,file); wcscat_s(save,L"~");
		
		if(CopyFileW(file,save,0)) return true;

		return GetLastError()==ERROR_FILE_NOT_FOUND;
	}
	void cleanup()
	{
		if(assist)
		{
			if(out)
			{
				assert(save);
				wcscpy(pe+MAX_PATH,save);
				*save = 0;
			}
			else pe[MAX_PATH] = '\0';
		}
		else if(restore&&!out)
		{
			wchar_t repair[MAX_PATH] = L"";
			int len = wcslen(wcscpy(repair,save));
			repair[len-1] = '\0'; assert(save[len-1]=='~');
			CopyFileW(save,repair,0);
		}
		if(*work) DeleteFileW(work); 
		if(*home) DeleteTemporaryAndAllSubfolders(home);
		if(*save) DeleteFileW(save);		
		*work = *home = *save = '\0';		
		if(_) fclose(_); 		
		if(lock) CloseHandle(lock);
		lock = 0; _ = 0;
		restore = false;
	}
	//HACK: reset for Open->File
	wchar_t file_open_pe[MAX_PATH];
	wchar_t file_open_csv[MAX_PATH];
	bool finish()
	{
		int option = IDCONTINUE;

		if(!out&&*pe)
		{
			option = MessageBoxW(hwnd,
			L"There are one or more jobs in the update queue.\n"
			L"\n"
			L"CANCEL to empty the queue. TRY AGAIN to start over.", 
			L"Please decide",
			MB_CANCELTRYCONTINUE);		
			if(option==IDCANCEL) 
			Somdialog_csv.clear(); //cancel
		}

		step = 1; cleanup(); //!

		if(option!=IDTRYAGAIN) 
		if(!Somdialog_csv.empty())
		{
			const wchar_t *_csv = 
			Somdialog_csv.front().path;	Somdialog_csv.pop_front();		
			csv = wmemcpy(file_open_csv,_csv,MAX_PATH);
			pe = wmemcpy(pe,csv,MAX_PATH);
			v = Sword_of_Moonlight_Subversion_Library_Version(0,pe,0);			
			//manually updating the updater?
			if(v==SOMVERSION(SOMVERSION_VERSION)		
			&&Somversion_dll()==GetModuleHandleW(pe))
			{
				Somversion_update(csv,pe,hwnd); 
				*file_open_csv = '\0'; //skip
				out = Somdialog_csv.empty();
				if(!out) finish();				
			}
		}
		else return out = true; //empty

		//retry: reacquire lock
		lock = Somdialog_lock(pe); 

		return out = false;
	}				 	

}Somdialog_instance;

static INT_PTR CALLBACK Somdialog_SOM(HWND,UINT,WPARAM,LPARAM);

extern bool Somdialog(int &ms, const wchar_t *csv, wchar_t pe[MAX_PATH], SOMVERSION in, HWND parent, bool assist)
{
	//NEW: just adding to queue
	if(ms==SOMVERSION::queueup)
	{
		if(csv&&*csv)
		{
			Somdialog_csv_vt v;
			wcscpy_s(v.path,csv);
			Somdialog_csv.push_back(v);
		}
		return ms = 0; //hack
	}

	Somfont();

	HANDLE lock = 0;

	if(csv&&*pe&&in&&!assist) //try to lock. requiring for sessions that timeout
	{
		lock = Somdialog_lock(pe); if(!lock&&ms>=SOMVERSION::notimeout) return in;
	}

	//what a minefield 
	//Somdialog_instance p = {ms,ms,pe,csv,in,lock,0,L"",L"",L"",L"",L".zip",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	Somdialog_instance p; memset(&p,0x00,sizeof(p));
	p.ms = p.ref = ms; p.pe = pe; p.csv = csv; p.v = in; p.lock = lock; p.ext = L".zip"; p.assist = assist;

	//OBSOLETE?
	if(!csv||!*pe) //using in standalone mode or Somtext(0)
	{
		//THIS IS PROBABLY FROM MSVC2005 DEVELOPMENT DAYS, SINCE
		//IT REQUIRED RUNNING IN THE ELEVATED ADMINSTRATION MODE.

		//Required for WM_DROPFILES (above) to work in 7 with UAC
		BOOL (WINAPI *ChangeWindowMessageFilter)(UINT,DWORD);	
		if(*(void**)&ChangeWindowMessageFilter=
		GetProcAddress(GetModuleHandle("user32.dll"),"ChangeWindowMessageFilter"))
		{
		ChangeWindowMessageFilter(WM_DROPFILES,1); //MSGFLT_ADD
		ChangeWindowMessageFilter(WM_COPYDATA,1);
		ChangeWindowMessageFilter(0x0049,1); 
		}
	}

	//2022: I think this has always been 0 but Windows 10 is now
	//hiding the window where it can't readily be found it seems
	if(!parent)
	if(parent=GetActiveWindow())
	if(!IsWindowVisible(parent))
	parent = 0;
	/*this seems to return a crapshoot since it can be an invisible
	//window (somehow) even though logically a active or foreground
	//window logically should not be invisible
//	if(!parent) parent = GetForegroundWindow();
	if(0&&!parent&&csv)
	{
		parent = GetForegroundWindow(); //maybe not a SOM window

		//TODO: I want to detect a SOM window. I thought maybe if
		//the som.exe.cpp code for initialize_taskbar could match
		//that would work but it leaves out standalone games. the
		//caller could provide an HWND to somversion.hpp but it's
		//too complicated in many cases

		if(!IsWindowVisible(parent)) parent = 0;
	}*/

	DialogBoxParamW(Somversion_dll(),MAKEINTRESOURCEW(IDD_SOM),parent,Somdialog_SOM,(LPARAM)&p);
	
	if(csv) p.cleanup(); //just to be safe
	
	//hack: suppressing follow-up dialog
	if(ms<SOMVERSION::notimeout) ms = 0; 

	return p.out;
}

#define SOMDIALOG_SAYS_WHAT(vanity) \
	const size_t said_s = 2056;\
	wchar_t said[said_s+16] = L"";\
	wchar_t *a = said+16, *p = a, *z = a+said_s;\
	wchar_t *ui = *in->pe&&in->v?L"update":L"install";

#define SOMDIALOG_UI(u,i) (*ui=='u'?u:i)

static void Somdialog_csvstep1(Somdialog_instance *in)
{
	if(!*in->csv) //hack
	{
		//after cancelled Somversion.dll update
		*in->pe = '\0';
		SendMessage(in->hwnd,WM_INITDIALOG,0,(LPARAM)in);
		return;
	}
	
	SOMDIALOG_SAYS_WHAT(?)

	EnableWindow(GetDlgItem(in->hwnd,IDC_SEEN),0);
	EnableWindow(GetDlgItem(in->hwnd,IDC_BACK),in->step>1);

	p+=swprintf_s(p,z-p,L"STEP %d:\r\n",in->step);

	p+=swprintf_s(p,z-p,
	L"\r\n"
	L"Automatic %c%s Procedure\r\n"
	L"==========%S================\r\n"
	L"\r\n"
	L"The goal at hand is to %s one or more files which you yourself have optioned to %s or an application has decided it might be time to do so.\r\n",
	SOMDIALOG_UI('U','I'),ui+1,SOMDIALOG_UI("","="),ui,ui);

	if(in->pe)
	p+=swprintf_s(p,z-p,
	L"\r\n"
	L"An application may detect that one of your files is out of date if its modification timestamp is older than that of a corresponding CSV file.\r\n");
	
	if(!in->pe)
	p+=swprintf_s(p,z-p,
	L"\r\n"
	L"Some applications will attempt to install a file whenever it encounters CSV file. Others will install a file they require if a corresponding CSV file is recognized\r\n");

	p+=swprintf_s(p,z-p,
	L"\r\n"
	L"Below are the files involved in this operation and their respective modification dates.\r\n");

	p+=swprintf_s(p,z-p,
	L"\r\n"
	L"Local Files Involved\r\n"
	L"====================\r\n"
	L"\r\n");

	if(in->csv&&*in->csv)
	{		
		struct _stat fst; if(_wstat(in->csv,&fst)) fst.st_mtime = 0;
		p+=swprintf_s(p,z-p,L"%s\r\n%S\r\n",in->csv,Somdialog_print(fst.st_mtime));
	}
	if(in->pe&&*in->pe)
	{		
		struct _stat fst; if(_wstat(in->pe,&fst)) fst.st_mtime = 0;
		p+=swprintf_s(p,z-p,L"%s\r\n%S\r\n",in->pe,Somdialog_print(fst.st_mtime));
	}

	if(!in->_) in->_ = _wfopen(in->csv,L"rt");	
	if(in->_) fseek(in->_,0,SEEK_SET);
	if(in->_)
	p+=swprintf_s(p,z-p,
	L"\r\n"
	L"CSV File Preview\r\n"
	L"================\r\n"
	L"\r\n"
	L"The line by line contents of %s are shown below. Please proceed to the next step if everything appears correct.\r\n",
	in->csv);

	int i;
	for(i=0;in->_&&z-p>1&&i<8;i++)
	{
		if(feof(in->_)) 
		{
			p+=swprintf_s(p,z-p,L"\r\n"); break;
		}

		p+=swprintf_s(p,z-p,L"\r\n%d:\r\n",i+1);

		if(!fgetws(p,z-p,in->_)) break; 
		
		p+=wcslen(p);
	}

	if(i>=8)
	p+=swprintf_s(p,z-p,
	L"\r\n"
	L"WARNING: %s exceeds 8 lines. Odds are it's not a valid CSV file or even a text file. Exercise caution proceeding.\r\n",
	in->csv);	

	p+=swprintf_s(p,z-p,
	L"\r\n"
	L"Should You Continue\r\n"
	L"===================\r\n"
	L"\r\n"
	L"The next step will attempt to convert the CSV file into a menu of %s options.",
	ui);

	EnableWindow(GetDlgItem(in->hwnd,IDC_NEXT),1);

	SetDlgItemTextW(in->hwnd,IDC_SAID,a); 
}

static bool Somdialog_csvstep2(Somdialog_instance *in)
{		
	in->downloading = 0;	

	assert(in->csv&&*in->csv);
							
	*in->base = '\0';
	in->downloading = 0; //assuming...
	SendDlgItemMessage(in->hwnd,IDC_DONE,PBM_SETPOS,0,0);

	SOMDIALOG_SAYS_WHAT(?)

	EnableWindow(GetDlgItem(in->hwnd,IDC_BACK),in->step>1);

	p+=swprintf_s(p,z-p,L"STEP %d:\r\n",in->step);

	p+=swprintf_s(p,z-p,
	L"\r\n"
	L"CSV Menu Options\r\n"
	L"================\r\n"
	L"\r\n"
	L"Below are the contents of %s scanned and converted into a list of file URLs. "
	L"Each file corresponds to one of the %s options in the menu to the right.\r\n",
	in->csv,ui);
	
	wchar_t *seen = 0; size_t seen_s = 0;

	if(!in->_) in->_ = _wfopen(in->csv,L"rt");	

	if(!in->_)
	{
		assert(0); //TODO: error condition
	
		return false;
	}
	else fseek(in->_,0,SEEK_SET);

	wchar_t val[128] = L"", sep = 0, eof = 0;

	int ln = 0; //first pass
	while(!eof) switch(fwscanf(in->_,L"%128[^, \r\n]%c",val,&sep))
	{
	case EOF: eof = 1; break;

	case 0: fgetwc(in->_); break; //stuck (double delimiter)

	default: //1 or 2
	
		if(*val)
		{
			if(ln==0)
			{
				if(!*in->base)
				{
					wcsncpy(in->base,val,MAX_PATH);

					for(int i=0;i<4;i++) 
					in->base[i] = towlower(in->base[i]); //paranoia

					in->base[MAX_PATH-1] = '\0';
				}
			}
			else if(ln==1) //assuming
			{
				seen_s++; 
			}
		}
		
		if(sep=='\n')
		{
			if(ln==1) break; //good enough for now

			ln++; 
		}

		sep = '\0';								  
	}
		
	if(!seen_s||wcsncmp(in->base,L"http://",7)) 
	{
		assert(0); //TODO: error condition

		return false;
	}

	eof = 0; rewind(in->_); //second pass...
	while(ln--&&fwscanf(in->_,L"%*[^\n]%*c")!=EOF);

	seen = new wchar_t[seen_s*4]; 

	seen[0] = seen[1] = seen[2] = seen[3] = 0;

	int errors = 0;

	int i = 0; 
	while(!eof&&errors<8)
	switch(fwscanf(in->_,L"%128[^, \r\n]%c",val,&sep))
	{
	case EOF: eof = 1; break;

	case 0: fgetwc(in->_); break; //stuck (double delimiter)
 
	default: //1 or 2

		if(i>=seen_s*4) //exceeded the 'seen' buffer
		{
			assert(0); //the loop above and this one are out of whack

			errors = 8; continue; //break out of the while loop
		}

		if(*val) //parse version string
		{
			//each val is thought to be a dotted 
			//four component version string with
			//an optional dotted file format dot
			//or two (eg. tar.gz) or more though
			//only .zip is implemented right now

			//the current version is seeded with
			//the previous' values. Only the last
			//component is required, and so on...
			//up to the four in all if need be. 

			if(i)
			{
				seen[i+0] = seen[i-4];
				seen[i+1] = seen[i-3];
				seen[i+2] = seen[i-2];
				seen[i+3] = seen[i-1];
			}

			wchar_t *dot = wcsrchr(val,'.'), *end = 0;
			
			int n = wcstoul(dot?dot+1:val,&end,10);

			while(dot&&end&&*end) //not a number
			{
				if(wcsicmp(dot+1,L"zip"))
				{
					std::wcout << "CSV Update: " << dot << "format is not recognized.\n";

					errors++;
				}	
				else in->ext = L".zip";

				*dot = '\0'; dot = wcsrchr(val,'.');

				n = wcstoul(dot?dot+1:val,&end,10);
			}

			for(int j=i+3;j>=i;j--)
			{
				seen[j] = n; 

				if(!dot) break;

				if(end&&*end||n>65535)
				{
					std::wcout << "CSV Update: non-numeric version string, or number out of range.\n";

					errors++;
				}
												
				*dot = '\0'; dot = wcsrchr(val,'.');

				n = wcstoul(dot?dot+1:val,&end,10);
			}

			if(dot)
			{
				std::wcout << "CSV Update: too many dots in this version string.\n";

				errors++;
			}

			if(!seen[3]||i&&wmemcmp(seen,seen-4,4)>=0)
			{
				std::wcout << "CSV Update: version timeline inconsistent.\n";

				errors++;
			}

			i+=4;
		}

		if(sep=='\n') 
		{
			eof = 1; break; //hack
		}

		sep = '\0';
	}

	if(i/4!=seen_s)
	{
		assert(0); //should not be

		//TODO: error condition

		errors++;
	}

	if(errors)
	{
		p+=swprintf_s(p,z-p,
		L"\r\n"
		L"Errors Encountered!\r\n"
		L"===================\r\n"
		L"\r\n"
		L"%s encountered while processing the CSV file. "
		L"It's advisable to not continue. If that is even an option at this point. \r\n",
		errors>1?L"Multiple format errors were":L"A format error was");
	}

	int top = seen_s*4-4;

	HWND cbox = GetDlgItem(in->hwnd,IDC_SEEN); 

	SendMessage(cbox,CB_RESETCONTENT,0,0); 		

	char vstring[32] = "";
	for(int i=seen_s-1;i>=0;i--)
	{
		p+=swprintf_s(p,z-p,L"\r\n%s%d.%d.%d.%d%s",
		in->base,seen[i*4],seen[i*4+1],seen[i*4+2],seen[i*4+3],in->ext);

		char *current = !wmemcmp(in->v.v,seen+i*4,4)?" (=) ":"";
		sprintf(vstring,"%d.%d.%d.%d%s",seen[i*4],seen[i*4+1],seen[i*4+2],seen[i*4+3],current);

		SendMessage(cbox,CB_ADDSTRING,0,(LPARAM)vstring); 

		if(i==seen_s-1)
		SendMessage(cbox,CB_SELECTSTRING,1,(LPARAM)vstring); 		
	}

	if(seen) delete [] seen;

	if(seen_s) p+=swprintf_s(p,z-p,L"\r\n");

	if(seen_s)
	p+=swprintf_s(p,z-p,
	L"\r\n"
	L"Manual %c%s Alternative\r\n"
	L"==========%S================\r\n"
	L"\r\n"
	L"If an automatic update is impossible or not preferable the %s can be performed by hand.\r\n"
	L"\r\n"
	L"1.\r\nCopy/paste one of the URLs above into a file browser, or obtain the archive by some other means.\r\n"
	L"\r\n"
	L"2.\r\nOpen the archive and move the files inside to the folder containing the CSV file. It may be necessary to overwrite some files. "
	L"Make backups in case.\r\n",
	SOMDIALOG_UI('U','I'),ui+1,SOMDIALOG_UI("","="),ui);


	p+=swprintf_s(p,z-p,
	L"\r\n"
	L"Should You Continue\r\n"
	L"===================\r\n"
	L"\r\n"
	L"The next step will attempt to acquire a working copy of the file indicated by your selection. "
	L"%c%s is still a couple steps away.",
	SOMDIALOG_UI('U','I'),ui+1);

	EnableWindow(GetDlgItem(in->hwnd,IDC_NEXT),1);
	EnableWindow(GetDlgItem(in->hwnd,IDC_SEEN),1);

	SetDlgItemTextW(in->hwnd,IDC_SAID,a); 

	return true;
}

static int Somdialog_csvstep3_pt2(Somdialog_instance*);

static bool Somdialog_csvstep3(Somdialog_instance *in)
{	 	
	in->caching = 0;
	SendDlgItemMessage(in->hwnd,IDC_DONE,PBM_SETPOS,0,0);

	SOMDIALOG_SAYS_WHAT(?)
			
	EnableWindow(GetDlgItem(in->hwnd,IDC_BACK),in->step>1);
	EnableWindow(GetDlgItem(in->hwnd,IDC_NEXT),0);
	EnableWindow(GetDlgItem(in->hwnd,IDC_SEEN),0);

	p+=swprintf_s(p,z-p,L"STEP %d:\r\n",in->step);

	p+=swprintf_s(p,z-p,
	L"\r\n"
	L"HTTP Download\r\n"
	L"=============\r\n"
	L"\r\n");

	HWND cbox = GetDlgItem(in->hwnd,IDC_SEEN);
		
	size_t wstringlen =
	(size_t)SendMessage(cbox,CB_GETLBTEXTLEN,in->item,0);

	if(!wstringlen||wstringlen+wcslen(in->ext)>31)
	{
		assert(0); return false;
	}

	wchar_t wstring[32+4] = L""; 

	if(wstringlen!=SendMessageW(cbox,CB_GETLBTEXT,in->item,LPARAM(wstring)))
	{
		assert(0); return false;
	}

	//for future steps...
	//reset menu to selection only
	SendMessage(cbox,CB_RESETCONTENT,0,0);
	SendMessageW(cbox,CB_ADDSTRING,0,LPARAM(wstring)); 
	SendMessageW(cbox,CB_SELECTSTRING,1,LPARAM(wstring)); 		

	wchar_t *trim = wcschr(wstring,' ');

	if(trim) *trim = '\0'; //remove decoration
		
	wcscat_s(wstring,in->ext); wcscpy_s(in->shortname,wstring);
	wcscpy(in->work,in->base); wcscat_s(in->work,wstring); 
		
	wchar_t *uri = wcschr(in->work+7,'/'); //assuming http://

	assert(uri); if(!uri) return false; //not good enough

	*uri = '\0'; //hack
	p+=swprintf_s(p,z-p,
	L"Connecting to\r\n%s\r\n...\r\n",in->work+7);
	*uri = '/'; //hack

	SetDlgItemTextW(in->hwnd,IDC_SAID,a); 

	in->downloading = wcslen(a);

	//WinInet can take a long time to load up...

	if(!CreateThread(0,0,
	(LPTHREAD_START_ROUTINE)Somdialog_csvstep3_pt2,
	in,0,0)) Somdialog_csvstep3_pt2(in);

	return true;
}

static bool Somdialog_downloading(const int[4],Somdialog_instance*);

static int Somdialog_csvstep3_pt2(Somdialog_instance *in)
{	 	
	SOMDIALOG_SAYS_WHAT(?)

	if(in->downloading>
	  GetDlgItemTextW(in->hwnd,IDC_SAID,a,said_s))
	{
		assert(0); //this would be problematic
	}

	p+=in->downloading;	

	wchar_t url[MAX_PATH] = L""; wcscpy(url,in->work);

	Somdownload_progress progress = 
	(Somdownload_progress)Somdialog_downloading;
	if(Somdownload(in->work,progress,in))
	{
		p+=swprintf_s(p,z-p,
		L"\r\n"
		L"Connection Success\r\n"
		L"==================\r\n"
		L"\r\n"
		L"Downloading\r\n%s\r\n...\r\n"
		L"\r\n",
		url);

		in->downloading = wcslen(a);
	}
	else
	{			
		//assuming looks like http://
		wchar_t *uri = wcschr(url+7,'/');  				
		if(uri) *uri = '\0'; assert(uri); //...

		p+=swprintf_s(p,z-p,
		L"\r\n"
		L"Connection Failure\r\n"
		L"==================\r\n"
		L"\r\n"
		L"Failed to connect to host. Please check your internet connection to %s and try again.",
		url+7); //protocol+host

		in->downloading = 0;
	}
			
	SetDlgItemTextW(in->hwnd,IDC_SAID,a); 

	return S_OK;
}

static bool Somdialog_downloading(const int progress[4], Somdialog_instance *in)
{
	static int debug = 0;

	if(!in||!in->downloading) return false;

	SOMDIALOG_SAYS_WHAT(?)
		
	if(in->downloading>
	  GetDlgItemTextW(in->hwnd,IDC_SAID,a,said_s))
	{
		assert(0); //this would be problematic
	}

	p+=in->downloading;	

	int percent = 0;
	
	if(progress[1])
	{
		percent = float(progress[0])/progress[1]*100;

		SendDlgItemMessage(in->hwnd,IDC_DONE,PBM_SETPOS,percent,0);
	}
	else percent = SendDlgItemMessage(in->hwnd,IDC_DONE,PBM_GETPOS,0,0);
	
	if(progress[1]==0)
	{
		p+=swprintf_s(p,z-p,
		L"%d%% Connection was terminated...\r\n",
		percent);

		p+=swprintf_s(p,z-p,
		L"\r\n"
		L"Please Try Again\r\n"
		L"================\r\n"
		L"\r\n"
		L"The download failed to complete. Please try again.");
		
		//SEE ALSO Somtext.cpp
		//hack: more sleep may help WinInet cope better :/
		if(Somdownload_sleep<1000) //Somdownload_sleep+=100; 		
		if(IDYES==MessageBoxW(in->hwnd,
		L"Would you like to try a slower download speed. This usually works, but is slower.\r\n"
		L"This will slow the downloader down by half, and will be in affect until this window is closed.\r\n"
		L"\r\n"
		L"Alternatively! Choose NO, and select the other Download option after pressing Alt+D.\r\n",
		L"Somversion.dll - Download Failure)",
		MB_YESNO)) Somdownload_sleep+=Somdownload_sleep;

		in->downloading = 0;
	}
	else if(progress[0]==progress[1])
	{			
		wchar_t *units = L"B";

		float total = progress[1];

		if(progress[1]>1024*1024) 
		{
			total/=1024; units = L"KB";
		}

		p+=swprintf_s(p,z-p,
		L"100%% %d%s completed in %d seconds...\r\n", 
		int(total),units,progress[3]);
		
		//paranoia
		SendDlgItemMessage(in->hwnd,IDC_DONE,PBM_SETPOS,100,0);

		p+=swprintf_s(p,z-p,
		L"\r\n"
		L"Should You Continue\r\n"
		L"===================\r\n"
		L"\r\n"
		L"The next step will check the integrity of the acquired file before %s.",
		ui);

		in->downloading = 0;
		EnableWindow(GetDlgItem(in->hwnd,IDC_NEXT),1);
	}
	else p+=swprintf_s(p,z-p,
	L"%d%% %dB of %dB at %dBps",
	percent,progress[0],progress[1],progress[2]);

	SetDlgItemTextW(in->hwnd,IDC_SAID,a); 

	if(progress[1])
	{
		return progress[0]!=progress[1];
	}
	else return false;
}

static bool Somdialog_caching(const wchar_t*,const int[6],Somdialog_instance*);

static bool Somdialog_csvstep4(Somdialog_instance *in)
{	 	
	in->out = false; //hack??

	in->installing = in->counter = 0;
	SendDlgItemMessage(in->hwnd,IDC_DONE,PBM_SETPOS,0,0);
	SetDlgItemTextW(in->hwnd,IDC_NEXT,L"Next Step"); 

	SOMDIALOG_SAYS_WHAT(?)

	EnableWindow(GetDlgItem(in->hwnd,IDC_BACK),in->step>1);
	EnableWindow(GetDlgItem(in->hwnd,IDC_NEXT),0);
	EnableWindow(GetDlgItem(in->hwnd,IDC_SEEN),0);

	p+=swprintf_s(p,z-p,L"STEP %d:\r\n",in->step);

	//assuming .zip
	p+=swprintf_s(p,z-p,
	L"\r\n"
	L"Zip Decompression\r\n"
	L"=================\r\n"
	L"\r\n"
	L"The file downloaded appears to be a compressed archive. "
	L"If uncorrupted the contents will be shown below.\r\n");

	Somcache_progress progress = 
	(Somcache_progress)Somdialog_caching;
	if(Somcache(wcscpy(in->home,in->work),progress,in))
	{
		p+=swprintf_s(p,z-p,
		L"\r\n"
		L"Working Copy Found Satisfactory\r\n"
		L"===============================\r\n"
		L"\r\n"
		L"Decompressing\r\n%s\r\n...\r\n"
		L"\r\n",
		in->shortname);

		in->caching = wcslen(a);
	}
	else
	{			
		p+=swprintf_s(p,z-p,
		L"\r\n"
		L"Working Copy Corrupt\r\n"
		L"====================\r\n"
		L"\r\n"
		L"This copy appears to be corrupt. Please try to download the file again. "
		L"If that fails. It's possible the remote files involved are out of order. "
		L"Please alert the network administrator or someone who will do so.\r\n");
	}
			
	SetDlgItemTextW(in->hwnd,IDC_SAID,a); 

	return true;
}

static bool Somdialog_caching(const wchar_t *x, const int progress[6], Somdialog_instance *in)
{
	if(!in||!in->caching) return false;

	SOMDIALOG_SAYS_WHAT(?)
		
	if(in->downloading>GetDlgItemTextW(in->hwnd,IDC_SAID,a,said_s))
	{
		assert(0); //this would be problematic
	}

	p+=in->caching;	

	if(in->counter==progress[4])
	{
		SendDlgItemMessageW(in->hwnd,IDC_SEEN,CB_ADDSTRING,0,LPARAM(x)); 

		in->counter++;

		//2021: trying to allow for multiple files
		p+=swprintf_s(p,z-p,L"%s\r\n",x);

		in->caching = wcslen(a);
	}
	else assert(in->counter>progress[4]);

	int percent = 0;
	
	if(progress[1])
	{
		percent = float(progress[0])/progress[1]*100;

		SendDlgItemMessage(in->hwnd,IDC_DONE,PBM_SETPOS,percent,0);
	}
	else percent = 
	SendDlgItemMessage(in->hwnd,IDC_DONE,PBM_GETPOS,0,0);
	
	if(progress[1]==0)
	{
		p+=swprintf_s(p,z-p,
		L"%s\r\n%d%% Decompression terminated...\r\n",
		//x,percent);
		in->shortname,percent);

		p+=swprintf_s(p,z-p,
		L"\r\n"
		L"Please Try Again\r\n"
		L"================\r\n"
		L"\r\n"
		L"Decompression interrupted or the compressed archive might be partially corrupt. "
		L"Please try again. If repeated attempts are unsuccessful. Try stepping back to the download step and proceeding from there. "
		L"If still unsuccessful, please alert the network administrator or someone who will do so.\r\n"); 
		
		in->caching = 0;
	}
	if(progress[0]==progress[1])
	{			
		wchar_t *units = L"B";

		float total = progress[1];

		if(progress[1]>1024*1024) 
		{
			total/=1024; units = L"KB";
		}

		//2021: trying to allow for multiple files
		p+=swprintf_s(p,z-p,
		//L"%s\r\n100%% %d%s completed in %d seconds...\r\n", 
		//x,int(total),units,progress[3]);
		L"100%% %d%s completed in %d seconds...\r\n", 
		int(total),units,progress[3]);
		
		//paranoia
		SendDlgItemMessage(in->hwnd,IDC_DONE,PBM_SETPOS,100,0);

		wchar_t folder[MAX_PATH] = L"";

		wcscpy_s(folder,in->csv); *wcsrchr(folder,'\\') = '\0';

		p+=swprintf_s(p,z-p,
		L"\r\n"
		L"Files To Be %S Next Step\r\n"
		L"============%S==================\r\n"
		L"\r\n"
		L"In\r\n"
		L"%s\r\n"
		L"...\r\n"
		L"\r\n",
		SOMDIALOG_UI("Updated","Installed"),SOMDIALOG_UI("","="),folder);
		
		HWND cbox = GetDlgItem(in->hwnd,IDC_SEEN);

		wchar_t cget[MAX_PATH] = L"";
		for(size_t i=1;i<=in->counter;i++)
		if(SendMessageW(cbox,CB_GETLBTEXT,i,LPARAM(cget))!=CB_ERR) 
		{
			p+=swprintf_s(p,z-p,L"%s\r\n",cget); *cget = '\0';
		}

		p+=swprintf_s(p,z-p,
		L"\r\n"
		L"Should You Continue\r\n"
		L"===================\r\n"
		L"\r\n"
		L"The next step will attempt to %s the files shown above.",
		ui);

		in->caching = 0;
		EnableWindow(GetDlgItem(in->hwnd,IDC_NEXT),1);
	} 
	else //control wants to flicker :/
	{
		p+=swprintf_s(p,z-p,
		//2021: trying to allow for multiple files
		//L"%s\r\n%d%% %dB of %dB at %dBps",
		//x,percent,progress[0],progress[1],progress[2]);
		L"%d%% %dB of %dB at %dBps",
		percent,progress[0],progress[1],progress[2]);

		Sleep(60); //TODO: without
	}

	SetDlgItemTextW(in->hwnd,IDC_SAID,a); 

	if(progress[1])
	{
		return progress[0]!=progress[1];
	}
	else return false;
}

static int Somdialog_csvstep5_pt2(Somdialog_instance*);

static bool Somdialog_csvstep5(Somdialog_instance *in)
{	
	SendDlgItemMessage(in->hwnd,IDC_DONE,PBM_SETPOS,0,0);

	SOMDIALOG_SAYS_WHAT(?)

	EnableWindow(GetDlgItem(in->hwnd,IDC_BACK),in->step>1);
	EnableWindow(GetDlgItem(in->hwnd,IDC_NEXT),0);
	SetDlgItemTextW(in->hwnd,IDC_NEXT,L"Finish"); 
	EnableWindow(GetDlgItem(in->hwnd,IDC_SEEN),0);

	p+=swprintf_s(p,z-p,L"STEP %d:\r\n",in->step);
		
	if(in->assist)
	{
		p+=swprintf_s(p,z-p,
		L"\r\n"
		L"Assisted Update\r\n"
		L"===============\r\n"
		L"\r\n"
		L"Another application (%s) is assisting in this udpate. "
		L"This is usually done when a file in use is able to be updated. "
		L"It is up to the assisting application to overwrite the files only after exit.\r\n"
		L"\r\n"
		L"Copying files to the staging folder...\r\n"
		L"\r\n",
		in->pe+MAX_PATH);
	}
	else p+=swprintf_s(p,z-p,
	L"\r\n"
	L"%S Files\r\n"
	L"%S==============\r\n"
	L"\r\n"
	L"Please do not exit until all files are %s...\r\n"
	L"\r\n",
	SOMDIALOG_UI("Updating","Installing"),SOMDIALOG_UI("","=="),
	SOMDIALOG_UI(L"updated",L"installed"));

	in->installing = wcslen(a);

	SetDlgItemTextW(in->hwnd,IDC_SAID,a); 
		
	if(!CreateThread(0,0,
	(LPTHREAD_START_ROUTINE)Somdialog_csvstep5_pt2,
	in,0,0)) Somdialog_csvstep5_pt2(in);
	
	return true;
}

DWORD CALLBACK Somdialog_installing(
  LARGE_INTEGER TotalFileSize,
  LARGE_INTEGER TotalBytesTransferred,
  LARGE_INTEGER StreamSize,
  LARGE_INTEGER StreamBytesTransferred,
  DWORD dwStreamNumber,
  DWORD dwCallbackReason,
  HANDLE hSourceFile,
  HANDLE hDestinationFile,
  LPVOID lpData
)
{	Somdialog_instance *in = (Somdialog_instance*)lpData;

	int percent = float(in->commit+TotalBytesTransferred.LowPart)/in->charge*100;

	SendDlgItemMessage(in->hwnd,IDC_DONE,PBM_SETPOS,percent,0);

	//assuming always be signaled this way
	if(TotalFileSize.LowPart==TotalBytesTransferred.LowPart)
	{
		in->commit+=TotalFileSize.LowPart;
	}

	return PROGRESS_CONTINUE;
}

static int Somdialog_csvstep5_pt2(Somdialog_instance *in)
{		
	SOMDIALOG_SAYS_WHAT(?)

	if(in->installing>
	  GetDlgItemTextW(in->hwnd,IDC_SAID,a,said_s))
	{
		assert(0); //this would be problematic
	}

	p+=in->installing;

	HWND cbox = GetDlgItem(in->hwnd,IDC_SEEN);
		
	int unlock = 0; //pe

	if(in->pe) 
	{
		for(int i=0;in->csv[i]==in->pe[i]&&in->csv[i];i++);

		wchar_t *p = wcsrchr(in->pe,'\\');

		if(p++) unlock = 
		SendMessageW(cbox,CB_FINDSTRINGEXACT,-1,LPARAM(p));
	}

	wchar_t cached[MAX_PATH] = L"";

	wcscat_s(wcscpy(cached,in->home),MAX_PATH,L"\\");

	size_t cached_s = wcslen(cached);

	wchar_t install[MAX_PATH] = L"";

	if(!in->assist)
	{			
		wchar_t *trim = wcsrchr(wcscpy(install,in->csv),'\\');

		if(trim) trim[1] = '\0'; assert(trim);
	}
	else Somfile(wcscpy(install,L"\\staging\\~"));

	size_t install_s = wcslen(install);

	in->charge = in->commit = 0;
	
	size_t i, lockcheck = 0;

	//calculate number of bytes
	//that will be copied. Assuming
	//a figure that is non astronomical
	for(i=1;i<=in->counter;i++)
	{			
		if(in->assist&&i!=unlock) continue;

		wchar_t *cp = cached+cached_s;

		size_t cblen = 
		SendMessageW(cbox,CB_GETLBTEXTLEN,i,0);

		//extreme paranoia
		if(install_s+cblen>MAX_PATH
		  ||cached_s+cblen>MAX_PATH)
		{
			in->charge = 0; break;
		}
		
		SendMessageW(cbox,CB_GETLBTEXT,i,LPARAM(cp));

		HANDLE size = 
		CreateFileW(cached,GENERIC_READ,0,0,OPEN_EXISTING,0,0);
				
		if(size&&size!=INVALID_HANDLE_VALUE)
		{	
			if(i==unlock)
			{
				lockcheck = GetFileSize(size,0);

				in->charge+=lockcheck;
			}
			else in->charge+=GetFileSize(size,0);

			CloseHandle(size);
		}
		else //TODO: better than this
		{
			in->charge = 0; break;
		}
	}

	if(in->charge)
	for(i=1;i<=in->counter;i++)
	{			
		wchar_t *cp = cached+cached_s;
		wchar_t *up = install+install_s;

		SendMessageW(cbox,CB_GETLBTEXT,i,LPARAM(up));
		wcscpy(cp,up);

		bool locked = in->lock; //NEW

		if(unlock==i)
		{
			if(!in->backup(install)) break;

			if(in->lock) //assuming this won't collide...
			{			
				CloseHandle(in->lock); in->lock = 0;

				if(!DeleteFileW(install)) break;
			}
		}
		else if(in->assist) continue;

		p = a+in->installing;
		p+=swprintf_s(p,z-p,L"%s ...",up);
				
		SetDlgItemTextW(in->hwnd,IDC_SAID,a); 

		in->installing+=wcslen(up);

		int committed = in->commit;

		BOOL x = false, ok = 
		CopyFileExW(cached,install,Somdialog_installing,in,&x,COPY_FILE_RESTARTABLE);

		if(!ok&&!in->assist&&committed!=in->commit) 
		{
			assert(0); //TODO: something
		}

		if(unlock==i&&locked) //in->lock) 
		{
			in->lock = Somdialog_lock(install); 

			if(in->lock&&in->lock!=INVALID_HANDLE_VALUE) 
			{
				if(lockcheck!=GetFileSize(in->lock,0))
				{
					assert(0); in->restore = true;
				}
			}
			else //unable to verify installation
			{
				in->lock = 0; break;
			}
		}

		p = a+in->installing;		
		p+=swprintf_s(p,z-p,L" %S\r\n",
		in->assist?"staged":SOMDIALOG_UI("up-to-date","installed"));	

		SetDlgItemTextW(in->hwnd,IDC_SAID,a); 

		in->installing = p-a;
		
		Sleep(i!=in->counter?200:0);

		if(!ok) break;
	}

	if(in->installing>GetDlgItemTextW(in->hwnd,IDC_SAID,a,said_s))
	{
		assert(0); //this would be problematic
	}

	p = a+in->installing;	

	if(i>in->counter)
	{
		//paranoia
		SendDlgItemMessage(in->hwnd,IDC_DONE,PBM_SETPOS,100,0);

		p+=swprintf_s(p,z-p,
		L"\r\n"
		L"Congratulations!\r\n"
		L"================\r\n"
		L"\r\n"
		L"You've reached the end of this ordeal. \r\n"
		L"\r\n"
		L"Please excuse the interruption. You can get back to creating worlds whenever you feel like it...\r\n"
		L"\r\n"
		L"REMINDER:\r\n"
		L"The Sword of Moonlight is not a toy. "
		L"Wield it with care.");

		EnableWindow(GetDlgItem(in->hwnd,IDC_NEXT),1);

		in->out = true;
	}
	else
	{
		p+=swprintf_s(p,z-p,
		L"\r\n"
		L"%S Failed To Finish\r\n"
		L"%S=======================\r\n"
		L"\r\n"
		L"Something prevented the %s from finishing. Probably one of the files involved was not able to be written to. "
		L"If the install was only partial or a file was partially copied, the repercussions could be serious. "
		L"A not distant release will add a recovery mechanism. In the meantime, please hang in there. \r\n",
		SOMDIALOG_UI("Update","Install"),SOMDIALOG_UI("","="),ui);			
	}
		
	SetDlgItemTextW(in->hwnd,IDC_SAID,a); 

	in->installing = 0; return S_OK;
}

static INT_PTR CALLBACK Somdialog_About_SOM(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg)
	{
	case WM_INITDIALOG:

		//Version information
		{
			wchar_t v[512], tmp[MAX_PATH] = L"";

			swprintf_s(v,
			L"Somversion.dll Ver. %d.%d.%d.%d\r\n"
			L"Sword of Moonlight Subversion Client",
			SOMVERSION_VERSION);

			SetDlgItemTextW(hWndDlg,IDC_SOMVERSION,v); 

			swprintf_s(v,
			L"Temporary Folder\r\n"
			L"%s",
			Somfolder(tmp));
				
			SetDlgItemTextW(hWndDlg,IDC_TEMP,v); 

			ShowWindow(hWndDlg,SW_SHOW);
		}
		break;
		
	case WM_CLOSE:
								
		EndDialog(hWndDlg,0); return 0;
	}
	return 0;
}

static UINT_PTR CALLBACK Somdialog_OpenHook(HWND hdlg,UINT uiMsg,WPARAM,LPARAM)
{
	return 0; //reverts to pre-Vista Open dialogs
}

static INT_PTR CALLBACK Somdialog_SOM(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{	
	Somdialog_instance *in = 
	(Somdialog_instance*)GetWindowLongPtr(hWndDlg,DWL_USER);

	switch(Msg)
	{
	case WM_SHOWWINDOW:
	
		if(wParam) //2021: lower splash screen?
		{
			if(Somdialog_unsplash(hWndDlg,lParam)) return 0; 
		}
		break;

	case WM_INITDIALOG:
	{	
		in = (Somdialog_instance*)lParam; 
		SetWindowLongPtr(hWndDlg,DWL_USER,lParam); assert(in);		 
		in->hwnd = hWndDlg;

		static HICON icon = 
		LoadIcon(Somversion_dll(),MAKEINTRESOURCE(IDI_ICON));
		SendMessage(hWndDlg,WM_SETICON,ICON_BIG,(LPARAM)icon);

		DWORD val = 0, got = 4;
		SHGetValueW(HKEY_CURRENT_USER,L"SOFTWARE\\FROMSOFTWARE\\SOM",L"?DownloadMethod",0,&val,&got);		
		CheckMenuRadioItem(GetMenu(hWndDlg),ID_DOWNLOAD_WINSOCK,ID_DOWNLOAD_WININET,ID_DOWNLOAD_WINSOCK+(val&1),0);

		//hack: hosting service
		if(!in->csv&&in->ref>0)					   
		{
			switch(in->ref) //subordinate dialogs
			{
			default: assert(0); break;
			case ID_TOOLS_SEKAI: Somtext(hWndDlg); break;			
			}
			EndDialog(hWndDlg,0); return 0; //!!
		}		

		HWND said = GetDlgItem(hWndDlg,IDC_SAID);
		SetWindowTheme(said,L"",L""); //scrollbar
		SetFocus(said);	//HideCaret(said);

		if(*in->pe)
		{
			in->step = 1;
			Somdialog_csvstep1(in);		
		}
		else //File->Open mode
		{
			DragAcceptFiles(hWndDlg,TRUE);

			SetDlgItemTextW(hWndDlg,IDC_TEXT,
			L"Make sure all Sword of Moonlight applications have been exited.");
			//Reminder: even if UAC is not a problem, we have the 
			//drag&drop message which seems dumb if it doesn't work!
			
			HMENU menu = GetMenu(hWndDlg);
			EnableMenuItem(menu,ID_FILE_OPEN,MF_BYCOMMAND|MF_ENABLED);
			EnableMenuItem(menu,ID_TOOLS_SEKAI,MF_BYCOMMAND|MF_ENABLED);			
			if(!in->finish())
			{
				Somdialog_csvstep1(in);	//have files queued up in advance
				return TRUE;
			}
			SetWindowTextW(said,
			L"Drag & Drop or File->Open to begin...\r\n"
			L"\r\n"
			L"Process Explanation\r\n"
			L"===================\r\n"
			L"\r\n"
			L"This wizard updates Sword of Moonlight environment components.\r\n"
			L"\r\n"
			L"It understands CSV files. CSV files contain a list of files to be updated and where to find the updated files.\r\n"
			L"\r\n"
			L"Many such CSV files can be found in the current installation's TOOL folder:\r\n"
			L"\r\n");
			int KB109550 = GetWindowTextLength(said);
			SendMessage(said,EM_SETSEL,KB109550,KB109550);
			wchar_t install[MAX_PATH+2] = L""; got = sizeof(install);
			SHGetValueW(HKEY_CURRENT_USER,
			L"SOFTWARE\\FROMSOFTWARE\\SOM\\INSTALL",L"InstDir",0,install,&got);
			if(*install) 
			{			
				wcscat_s(install,L"\\tool\r\n");
				SendMessageW(said,EM_REPLACESEL,0,(LPARAM)install);			
			}
			else SendMessageW(said,EM_REPLACESEL,0,(LPARAM)
			L"Is Sword of Moonlight Installed?\r\n"
			L"================================\r\n"
			L"\r\n"
			L"Sorry! The path to the TOOL folder cannot be displayed because "
			L"there is no default Sword of Moonlight installation registered "
			L"within the Window's Registry according to:\r\n" 
			L"\r\n"
			L"Software\\FROMSOFTWARE\\SOM\\INSTALL\\InstDir\r\n");
		}		

		//hack: prevent loud hiliting of read-only textbox
		//return TRUE;
		ShowWindow(hWndDlg,SW_SHOW);
		SetForegroundWindow(hWndDlg);
		return FALSE;		
	}
	case WM_DROPFILES:
	{
		bool finished = in->finish();
		HDROP &drop = (HDROP&)wParam;
		UINT dropped = DragQueryFileW(drop,-1,0,0);		
		for(UINT i=0;i<dropped;i++)
		{
			Somdialog_csv.push_back(Somdialog_csv_vt());
			DragQueryFileW(drop,i,Somdialog_csv.back().path,MAX_PATH);
		}
		DragFinish(drop);
		//hack: stopping and starting? must be restarting
		if(finished&&!in->finish()) Somdialog_csvstep1(in);
		break;
	}
	case WM_COMMAND:

		if(!in) break;

		switch(HIWORD(wParam))
		{
		case EN_SETFOCUS: 			

			if(ES_READONLY&GetWindowStyle((HWND)lParam))
			DestroyCaret(); 
			break;

		case EN_KILLFOCUS:
				
			SendMessage((HWND)lParam,EM_SETSEL,-1,0);	
			break;

		case CBN_SELENDOK:
		
			in->item = SendMessage((HWND)LOWORD(lParam),CB_GETCURSEL,0,0); 
			break;
		}

		switch(LOWORD(wParam))
		{
		case IDC_NEXT:
			
			switch(in->step)
			{			
			case 0: assert(0);
			
			default: goto close;
					 
			case 1: in->step++;
			Somdialog_csvstep2(in); break;
			case 2: in->step++;
			Somdialog_csvstep3(in); break;
			case 3: in->step++;
			Somdialog_csvstep4(in); break;
			case 4: in->step++;
			Somdialog_csvstep5(in); break;
			}			
			break;			

		case IDC_BACK:
			
			switch(in?in->step:0)
			{			
			case 0: assert(0);
			
			default: goto close;

			case 2: in->step--;
			Somdialog_csvstep1(in); break;
			case 3: in->step--;
			Somdialog_csvstep2(in); break;
			case 4: in->step--;
			Somdialog_csvstep3(in); break;
			case 5: in->step--;
			Somdialog_csvstep4(in); break;
			}			
			break;			

		case ID_FILE_OPEN:
		{			
			const size_t csv_s = MAX_PATH*10; //OFN_ALLOWMULTISELECT

			wchar_t csv[csv_s] = L"";
			wchar_t tools[MAX_PATH+2] = L""; DWORD max_path = sizeof(tools);
			SHGetValueW(HKEY_CURRENT_USER,
			L"SOFTWARE\\FROMSOFTWARE\\SOM\\INSTALL",L"InstDir",0,tools,&max_path);
			if(*tools) wcscat_s(tools,L"\\tool");

			OPENFILENAMEW open = 
			{
				sizeof(open),hWndDlg,0,
				L"Comma-Separated Value (*.csv)\0*.csv\0",	
				0, 0, 0, csv, csv_s, 0, 0, tools,
				L"Open special CSV file containing update values", 	
				//hook is just for the better/consistent UI (on Windows 7)
				OFN_ENABLEHOOK|OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_FILEMUSTEXIST|OFN_ALLOWMULTISELECT,
				0, 0, 0, 0, Somdialog_OpenHook, 0, 0, 0 
			};
			if(GetOpenFileNameW(&open)) 
			{
				bool finished = in->finish();
				if(open.nFileOffset&&!open.nFileExtension) //multiple
				{
					Somdialog_csv_vt v; wcscpy(v.path,csv);

					wchar_t *f = v.path+open.nFileOffset; f[-1] = '\\';
																 					
					for(wchar_t *p=csv+open.nFileOffset,*q;*p;p++)
					{
						for(q=f;*p;) *q++ = *p++; *q++ = '\0';

						Somdialog_csv.push_back(v);
					}
				}
				else //single
				{
					Somdialog_csv.push_back(Somdialog_csv_vt());
					wcscpy_s(Somdialog_csv.back().path,csv);
				}
				//hack: stopping and starting? must be restarting
				if(finished&&!in->finish()) Somdialog_csvstep1(in);
			}
			break;
		}
		case ID_FILE_EXIT: goto close;		  			

		case ID_TOOLS_SEKAI: Somtext(hWndDlg); break;

		case ID_DOWNLOAD_WINSOCK: case ID_DOWNLOAD_WININET:
		{
			DWORD val = wParam&1;
			if(!SHSetValueW(HKEY_CURRENT_USER,
			L"SOFTWARE\\FROMSOFTWARE\\SOM",L"?DownloadMethod",REG_DWORD,&val,4))
			CheckMenuRadioItem(GetMenu(hWndDlg),ID_DOWNLOAD_WINSOCK,ID_DOWNLOAD_WININET,ID_DOWNLOAD_WINSOCK+val,0);			
			else MessageBeep(-1); break;
		}
		case ID_HELP_ABOUTSOM:
		 
			DialogBoxW(Somversion_dll(),MAKEINTRESOURCEW(IDD_ABOUT),hWndDlg,Somdialog_About_SOM);
			break;
		}		
		break;

	case WM_CLOSE: close:

		if(in->installing)
		{
			wchar_t seriously[MAX_PATH*2];

			swprintf_s(seriously,
			L"It appears that a file is in the process of being installed.\r\n"
			L"\r\n"
			L"Closing now could leave your Sword of Moonlight install in an unusable state.\r\n"
			L"\r\n"
			L"Are you positive you wish to abandone this procedure at this time?",
			L"\r\n"
			L"Choosing yes will exit mid-install.");

			int no = MessageBoxW(0,seriously,L"Exiting SOM",MB_YESNO|MB_DEFBUTTON2);

			if(no==IDNO) return 0;
		}
		
		if(Somdialog_csv.empty()) //what is this all about?
		{
			//hack: create the illusion of responsiveness...
			if(in->downloading||in->caching||in->installing) 
			ShowWindow(hWndDlg,SW_HIDE); 
		}
		//hack: allow some time for threads to wrap up...
		in->downloading = in->caching = in->installing = 0;
		Sleep(std::max<int>(200,Somdownload_sleep));

		if(!Somdialog_csv.empty())
		{
			if(!in->finish()) //next in line
			{
				Somdialog_csvstep1(in); break;
			}
		}	
		EndDialog(hWndDlg,0); break;
	} 
	return FALSE;
}