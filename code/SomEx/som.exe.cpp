
#include "Ex.h" 
EX_TRANSLATION_UNIT

#include <propkey.h>
#include <propvarutil.h>

#include "Ex.ini.h"
#include "Ex.output.h" //logging

#include "som.state.h" 
#include "som.extra.h"

static bool som_exe_mismatch = false;
static DWORD som_exe_entrypoint(HANDLE proc) //experimental
{	
	struct 
	{
		DWORD pe; 
		IMAGE_FILE_HEADER file; 
		IMAGE_OPTIONAL_HEADER32 optional;

	}peh; IMAGE_DOS_HEADER idh;
	if(ReadProcessMemory(proc,(void*)0x400000,&idh,sizeof(idh),0))
	{
		if(idh.e_magic==*(WORD*)"MZ"
		  &&ReadProcessMemory(proc,(char*)0x400000+idh.e_lfanew,&peh,sizeof(peh),0))
		{
			if(peh.pe==*(DWORD*)"PE\0\0"
			  &&peh.file.Machine==0x14c //i386
			   &&peh.optional.Magic==0x10B) //?
			{
				return 0x400000+
				peh.optional.AddressOfEntryPoint;
			}
		}
	}
	assert(0);
	return 0;
}
static DWORD som_exe_breakpoint(HANDLE proc, DWORD bp, DWORD *rp=0)
{
	DWORD out = 0, op;
	if(VirtualProtectEx(proc,(void*)bp,4,PAGE_EXECUTE_READWRITE,&op))
	{
		const DWORD breakpoint = 0x909090CC;
		if(!ReadProcessMemory(proc,(void*)bp,&out,4,0) 
		  ||!WriteProcessMemory(proc,(void*)bp,rp?rp:&breakpoint,4,0))					
		som_exe_mismatch = true; if(rp) assert(out==breakpoint);
		VirtualProtectEx(proc,(void*)bp,4,op,&op);
		FlushInstructionCache(proc,(void*)bp,4);
	}
	else som_exe_mismatch = true;
	return out;
}
/*todo: rewrite/consider not using strlen*/
static bool som_exe_overwrite(HANDLE proc, void *raddress, char *safeword, char *with)
{	
	if(som_exe_mismatch) return false;

	EXLOG_LEVEL(7) << "som_exe_overwrite()\n";

	char safety_buffer[64]; //todo: null char boundary check
	
	int safety = strlen(safeword); assert(safety<sizeof(safety_buffer));

	bool success = //debugging
	ReadProcessMemory(proc,raddress,safety_buffer,safety,0);

	if(som_exe_mismatch=strnicmp(safety_buffer,safeword,safety)) return false;

	DWORD goddmanit = PAGE_EXECUTE_WRITECOPY; //4hrs lost forever

	safety = strlen(with)+1; assert(safety<sizeof(safety_buffer));
	success = VirtualProtectEx(proc,raddress,safety,PAGE_EXECUTE_READWRITE,&goddmanit);

	success = WriteProcessMemory(proc,raddress,with,safety,0);	
	success = VirtualProtectEx(proc,raddress,safety,goddmanit,&goddmanit);
	success = ReadProcessMemory(proc,raddress,safety_buffer,safety,0);

	if(som_exe_mismatch=memcmp(safety_buffer,with,safety)) return false;

	return true;
}
//unused: tried preloading before DllMain to run vsjitdebugger
void som_exe_Ole32_dll(HANDLE proc, LPTHREAD_START_ROUTINE rLoadLibrary, char *SomEx_dll)
{	
	//copying som_exe_overwrite 
	const char with[] = "Ole32.dll";
	const size_t safety = sizeof(with);
	char save[safety] = "", test[safety] = "";
	void *raddress = SomEx_dll-sizeof(with)+sizeof("SomEx.dll");
	ReadProcessMemory(proc,raddress,save,safety,0);
	DWORD old = PAGE_EXECUTE_WRITECOPY;
	VirtualProtectEx(proc,raddress,safety,PAGE_EXECUTE_READWRITE,&old);
	WriteProcessMemory(proc,raddress,with,safety,0);		
	ReadProcessMemory(proc,raddress,test,safety,0);
	if(!strcmp(with,test)) //paranoia
	{	
		HANDLE dllMain = 
		CreateRemoteThread(proc,0,0,rLoadLibrary,raddress,0,0);		
		if(dllMain) WaitForSingleObject(dllMain,INFINITE); 
		CloseHandle(dllMain);
	}
	else assert(0); 
	WriteProcessMemory(proc,raddress,save,safety,0);		
	VirtualProtectEx(proc,raddress,safety,old,&old);
}
 
//REMOVE ME???
extern HANDLE SOM::exe_process = 0, SOM::exe_thread = 0;
extern DWORD SOM::exe_threadId = 0; //XP

static DWORD som_exe(wchar_t **argv, size_t argc) 
{	
	//*PathFindExtensionW(argv[0]) = '\0';
	wchar_t *_exe = PathFindExtensionW(argv[0]);
	if(!wcsicmp(_exe,L".exe")) *_exe = '\0'; 	
	SetCurrentDirectoryW(EX::cd()); assert(!SOM::game);
							
	bool show = som_exe_mismatch = false;

	EX::INI::Launch go;

	bool this_is_a_control_launch =	go->do_without_the_extension_library;	
	
	wchar_t bin[MAX_PATH] = L"";

	if(!wcsnicmp(argv[0],L"SOM_",4)
	||!wcsnicmp(argv[0],L"MapComp",7)
	||!wcscmp(L"ws",EX::log()) //HACK (workshop.cpp)
	||SOM::tool&&4==wcslen(wcsstr(argv[0],L"Edit"))) //workshop_exe
	{
		SOM::xtool(bin,argv[0]); 
		if(!*bin) return 0; //HACK: PrtsEdit and friends?

		if(wcslen(argv[0]+4)>2) //2: DB/RT
		{
			show = 'M'!=*argv[0]||!SOM::image(); //MapComp

			this_is_a_control_launch = false; 					
		}		
	}
	else if(*go->launch_image_to_use)
	{
		wcscpy_s(bin,go->launch_image_to_use);
	}
	else //Reminder: results are unsorted
	{
		WIN32_FIND_DATAW data; 		
		HANDLE glob = FindFirstFileW(L"EX\\*.bin",&data); 
		FindClose(glob); 

		if(glob!=INVALID_HANDLE_VALUE)
		{
			wcscpy(wcscpy(bin,L"EX\\")+3,data.cFileName);			
		}
		else return 0; //not good
	}			
										
	//hack: only collecting for top level applications
	if(!GetEnvironmentVariableW(L"SomEx.dll Binary",0,0))
	{
		SetEnvironmentVariableW(L"SomEx.dll Binary",bin);
	}							
		
	EXLOG_LEVEL(7) << "Creating subprocess threadmain\n"; 

	const wchar_t *args[5] = {bin};
	assert(argc<=5); argc = min(5,argc);
	for(size_t i=1;i<argc;i++) args[i] = argv[i]; 								   	
	wchar_t cmd[MAX_PATH*6] = L""; assert(argc<=5);
	//reminder: mapcomp.exe fails if there is anything after 5
	WCHAR f[] = L"\"%ls\" \"%ls\" \"%ls\" \"%ls\" \"%ls\"";
	f[argc*6-1] = '\0'; //NpcEdit tries to open ""
	swprintf_s(cmd,f,args[0],args[1],args[2],args[3],args[4]);	  	

    STARTUPINFOW si; memset(&si,0x00,sizeof(si)); si.cb=sizeof(si);			
	si.wShowWindow = show?SW_SHOW:SW_HIDE; //clean in-window startup								  
	if(!this_is_a_control_launch) si.dwFlags = STARTF_USESHOWWINDOW; 	

	PROCESS_INFORMATION pi; 
	//NEW: twart phantom jobs courtesy Microsoft / meddlers
	//thankfully these job mongers usually allow breakaways
	DWORD cflags = CREATE_BREAKAWAY_FROM_JOB|CREATE_NO_WINDOW;
	DWORD suspend = this_is_a_control_launch?0:CREATE_SUSPENDED;					
	//NEW: seems DebugActiveProcess isn't cutting further down
	if(suspend) cflags|=DEBUG_ONLY_THIS_PROCESS|DEBUG_PROCESS;
	//1: inherit handles for "SomEx.dll Binary Job" as seen below
	if(!CreateProcessW(bin,cmd,0,0,1,suspend|cflags,0,0,&si,&pi)) return 0;
	//messy: but unavoidable (perhaps there's no use for _thread)
	SOM::exe_process = pi.hProcess; SOM::exe_thread = pi.hThread;
	SOM::exe_threadId = pi.dwThreadId; //XP

	//Important (brings order to SOM)
	AllowSetForegroundWindow(pi.dwProcessId);  

	wchar_t str[9] = L"0";
	GetEnvironmentVariableW(L"SomEx.dll Binary Job",str,9);
	HANDLE job = (HANDLE)wcstoul(str,0,16); 
	if(job&&!AssignProcessToJobObject(job,pi.hProcess)) assert(0);
			
	EXLOG_LEVEL(7) << "Subprocess threadmain created\n";

	if(!suspend) return pi.dwProcessId; //!!

    ////TODO: verify with respect to SOM::image (som.state.h/cpp)
	//
	DWORD rdata = 0, data = 0; const wchar_t *tool = 0; 
	//	
	LPCVOID guess = (LPCVOID*)0x400000; MEMORY_BASIC_INFORMATION info; 
	//
	//obsolete: relying on Microsoft Detours 2.1 (Ex.detours.h/cpp)
	bool som_run = 0; int comdlg32 = 0, ddraw[2] = {0,0}, winmm = 0; //tools
	//
	while(VirtualQueryEx(pi.hProcess,guess,&info,sizeof(info)))
	{
		DWORD i = (DWORD)info.BaseAddress;
		if(info.AllocationProtect&PAGE_EXECUTE_WRITECOPY)
		{				
			const int 
			MAIN = 0x9F9E, 
			RUN  = 0x93E6, //w/ cracked image
			EDIT = 0x9B5C, 
			PRM  = 0x10E98,
			MAP  = 0x155EE,
			SYS  = 0xE952, 
			MPX  = 0x7F6; //MapComp.exe
			switch(i)  
			{
			case 0x00414000: //MPX rdata //EneEdit/NpcEdit rdata

				assert(!winmm); winmm = MPX; //ambiguous/obsolete

				rdata = i; break;
				
			case 0x00415000: //MPX data
				
				data = i; assert(winmm==MPX); 
				
				tool = L"MapComp"; break;

			case 0x00416000: //EneEdit/NpcEdit data

				winmm = 0; //winmm = MPX; //ambiguous/obsolote

				switch(**argv) //ambiguous
				{				
				case 'E': tool = L"EneEdit"; break;
				case 'N': tool = L"NpcEdit"; break;
				case 'S': tool = L"SfxEdit"; break;
				}

				data = i; break;

			case 0x00420000: //ItemEdit rdata							
			case 0x00425000: //ObjEdit rdata

				rdata = i; break;

			case 0x00426000: //ItemEdit data //PrtsEdit rdata

				if(rdata) //ItemEdit
				{
					data = i; tool = L"ItemEdit";
				}
				else rdata = i; break;

			case 0x0042B000: //MAIN rdata //ObjEdit data 
				
				if(rdata) //ObjEdit
				{
					data = i; tool = L"ObjEdit";
					break;
				}

				rdata = i; assert(!comdlg32);
				
				comdlg32 = MAIN; break;

			case 0x00430000: //PrtsEdit data

				data = i; tool = L"PrtsEdit";
				break;

			case 0x00436000: //MAIN data

				data = i; assert(comdlg32==MAIN); 
				
				tool = L"SOM_MAIN"; break;

			case 0x0042D000: //RUN rdata (JDO unlocked)

				assert(!som_run&&!comdlg32);

				comdlg32 = RUN;	//FALL THRU

				rdata = i; //break;

			case 0x00428000: //RUN rdata
			
				if(!rdata) rdata = i;
			
				assert(!comdlg32||comdlg32==RUN);

				som_run = true;	tool = L"SOM_RUN"; break;

			case 0x00429000: //RUN data / EDIT rdata

				if(!som_run) //then assume EDIT
				{
					rdata = i; assert(!comdlg32);

					comdlg32 = EDIT; break;
				}
				//break; //FALL THRU

			case 0x00437000: //RUN data (JDO crack)

				assert(!comdlg32||comdlg32==RUN);

				data = i; assert(som_run); tool = L"SOM_RUN"; break;

			case 0x00433000: //EDIT data

				data = i; assert(comdlg32==EDIT); 
				
				tool = L"SOM_EDIT"; break;

			case 0x00464000: //PRM rdata

				rdata = i; assert(!comdlg32);

				comdlg32 = PRM; 				
				ddraw[0] = PRM-4868; 
				ddraw[1] = PRM+7796; break;

			case 0x00476000: //PRM data	(-483FDC)

				data = i; assert(comdlg32==PRM); 
				
				tool = L"SOM_PRM"; break;

			case 0x00478000: //MAP rdata

				rdata = i; assert(!comdlg32);
						   
				comdlg32 = MAP; 
				ddraw[0] = MAP-4878; 
				ddraw[1] = MAP+2858; break;

			case 0x0048E000: //MAP data

				assert(comdlg32==MAP); 

				data = i; tool = L"SOM_MAP"; break;

			case 0x0043C000: //SYS rdata

				rdata = i; assert(!comdlg32);

				comdlg32 = SYS; break;

			case 0x0044B000: //SYS data
				
				data = i; assert(comdlg32==SYS);
				
				tool = L"SOM_SYS"; break;
			//}

			//if(!som_ex)
			//switch((int)info.BaseAddress) //safe? hardcoding
			//{
				//DOS/PE headers and section table

			//case 0x00400000: break;

				//.text section is same for all som binaries

			//case 0x00401000: break;

				//original som_db.exe/patched som_rt.exe

			case 0x00457000: rdata = i; break; 
			case 0x00459000: data = i; break;

				//original som_rt.exe...

			case 0x00456000: rdata = i; break; 

			case 0x00458000: //ambiguous: can be patched som_db.exe
				
				if(rdata) data = i; //then original som_rt.exe				
				//patched som_db.exe	  
				else rdata = i; break; 				

		  //case 0x00458000: rdata = i; break; 

			case 0x0045A000: data = i; break;
			}
		}

		guess = (LPCVOID)(i+info.RegionSize);

		if(data&&rdata) break; //early out
	}

	if(!data||!rdata) //unrecognized PE layout
	{
		TerminateProcess(pi.hProcess,0); return 0;
	}	  
	else if(tool) //ensure argument matches PE layout
	{
		wchar_t *dot = argv[0]; while(*dot&&*dot!='.') dot++;

		if(wcsnicmp(tool,argv[0],dot-argv[0])) //mismatch
		{
			TerminateProcess(pi.hProcess,0); return 0;
		}
	}
		
	void *SomEx_dll = 0; //void: remote string pointer

	//Mark DOS header or Ex will refuse to work with it
	{
		//TODO: store post CreateProcess info in here

		char o[] = "This program cannot be run in DOS mode.";
		char w[] = "00000000 is WinMain thread ID SomEx.dll";

		//dirty: allow thread ID to be retrieved in host process
		sprintf(w,"%08x",pi.dwThreadId); 
		//NEW: repair deletion by sprintf (bug affected 1.1.2.2/4)
		w[+8] = ' ';  
		//Reminder: overwrite utilizes strlen (see bug above)
		if(som_exe_overwrite(pi.hProcess,(void*)0x40004E,o,w))
		{
			SomEx_dll = (char*)0x40004E+sizeof(w)-10; 
		}
		if(tool&&!wcscmp(tool,L"SfxEdit")&&SomEx_dll)
		som_exe_overwrite(pi.hProcess,(void*)0x414204,"EneEdit","SfxEdit");		
	}	 	
		
	/*obsolete: relying on Microsoft Detours 2.1 (Ex.detours.h/cpp)
	//
	if(tool) //newer
	{			
		if(comdlg32)
		{
			void *comdlg32_dll  = (void*)(rdata+comdlg32);

			som_exe_overwrite(pi.hProcess,comdlg32_dll,"comdlg32.dll","SomEx.dll");	
		}
		else if(winmm)
		{
			void *winmm_dll  = (void*)(rdata+winmm);

			som_exe_overwrite(pi.hProcess,winmm_dll,"winmm.dll","SomEx.dll");	
		}
		if(ddraw[0])
		{
			//assert(0);
			som_exe_overwrite(pi.hProcess,(void*)(rdata+ddraw[0]),"DDRAW.dll","SomEx.dll");	
		
			if(ddraw[1])
			som_exe_overwrite(pi.hProcess,(void*)(rdata+ddraw[1]),"DDRAW.dll","SomEx.dll");			
		}
	}
	else //game: older
	{
		int loadlibrarytextoffset = 0; //hack: technically offset to DDRAW.dll
						 
		switch(rdata) //since 1.1.2.2 support for som_rt.exe is transitional only
		{		
		case 0x00457000: loadlibrarytextoffset = 0x1520; break; //patched som_rt.exe
		case 0x00458000: loadlibrarytextoffset = 0x15D2; break; //patched som_db.exe

		default: som_exe_mismatch = true; //return 0;
		}

		void *OLE32_dll  = (void*)(rdata+loadlibrarytextoffset-0xB0);
		void *WINMM_dll  = (void*)(0);
		void *DDRAW_dll  = (void*)(rdata+loadlibrarytextoffset);
		void *DINPUT_dll = (void*)(rdata+loadlibrarytextoffset+0x20);
		void *DSOUND_dll = (void*)(rdata+loadlibrarytextoffset+0x2C);

		if(OLE32_dll) som_exe_overwrite(pi.hProcess,OLE32_dll,"ole32.dll","SomEx.dll");	
		if(WINMM_dll) som_exe_overwrite(pi.hProcess,WINMM_dll,"WINMM.dll","SomEx.dll");	
		if(DDRAW_dll) som_exe_overwrite(pi.hProcess,DDRAW_dll,"DDRAW.dll","SomEx.dll");	

		if(DINPUT_dll) som_exe_overwrite(pi.hProcess,DINPUT_dll,"DINPUT.dll","SomEx.dll");
		if(DSOUND_dll) som_exe_overwrite(pi.hProcess,DSOUND_dll,"DSOUND.dll","SomEx.dll");	
	}*/				
	
	//XP: queue SomEx.dll up prior to WinMain
	//(So that GetOpenFileName et al don't CDERR_DIALOGFAILURE on XP)
	//Reminder: DebugActiveProcess is not working (probably access rights)
	if(cflags&DEBUG_ONLY_THIS_PROCESS||DebugActiveProcess(pi.dwProcessId))
	{			
		/*it's a wonder this was ever gotten to work*/
		DWORD ep = som_exe_entrypoint(pi.hProcess), bp;
		if(ep) if(bp=som_exe_breakpoint(pi.hProcess,ep,0))
		{	
			if(ResumeThread(pi.hThread)&&!som_exe_mismatch)
			for(DEBUG_EVENT de;WaitForDebugEvent(&de,INFINITE);)
			{
				assert(de.dwProcessId==pi.dwProcessId);
				if(EXCEPTION_DEBUG_EVENT==de.dwDebugEventCode)
				{			
					//Reminder: there is an automatic breakpoint event for whatever reason
					if(EXCEPTION_BREAKPOINT==de.u.Exception.ExceptionRecord.ExceptionCode)
					{		
						if((void*)ep==de.u.Exception.ExceptionRecord.ExceptionAddress)
						{
							//reset instruction pointer
							CONTEXT c = {CONTEXT_CONTROL};
							if(GetThreadContext(pi.hThread,&c)) 
							{									
								c.Eip--; SetThreadContext(pi.hThread,&c); assert(ep==c.Eip);
							}	  					
							//repair entry/break point
							SuspendThread(pi.hThread); 
							som_exe_breakpoint(pi.hProcess,ep,&bp);
							//Reminder: may want DBG_EXCEPTION_HANDLED (undocumented) here
							ContinueDebugEvent(de.dwProcessId,de.dwThreadId,DBG_CONTINUE); 
							break;
						}
						//DBG_CONTINUE: the automatic breakpoint is a one off
						else if(!ContinueDebugEvent(de.dwProcessId,de.dwThreadId,DBG_CONTINUE))
						break;
					}
					else if(!ContinueDebugEvent(de.dwProcessId,de.dwThreadId,DBG_EXCEPTION_NOT_HANDLED))
					break;
				}
				else if(!ContinueDebugEvent(de.dwProcessId,de.dwThreadId,DBG_CONTINUE))
				break;
			}			
		}
		//Reminder: works with CreateProcess ok
		DebugActiveProcessStop(pi.dwProcessId);
	}		
	if(SomEx_dll&&!som_exe_mismatch) 
	{
		EXLOG_LEVEL(7) << 
		"Injecting SomEx.dll to perform first-chance hooking\n"; 

		LPTHREAD_START_ROUTINE LoadLibraryProc=(LPTHREAD_START_ROUTINE)
		GetProcAddress(GetModuleHandleA("kernel32.dll"),"LoadLibraryA");		
		/*whatever works: switched to using CreateProcess in DllMain
		EXPERIMENTAL: tried loading for SomEx_assert_IsDebuggerPresent
		som_exe_Ole32_dll(pi.hProcess,LoadLibraryProc,(char*)SomEx_dll);*/
		HANDLE dllMain = 
		CreateRemoteThread(pi.hProcess,0,0,LoadLibraryProc,SomEx_dll,0,0);		
		assert(dllMain);
		//This would be best practice except because the thread
		//is executed exclusively within DllMain is unnecessary
		//But what else is for a reason that remains mysterious 
		//the following code causes GetOpenFileName and friends 
		//to fail, CommDlgExtendedError returning DIALOGFAILURE
		//UPDATE: THE DEBUGGER-LIKE CODE UP ABOVE REMEDIES THIS
		DWORD debug = WaitForSingleObject(dllMain,INFINITE); 
		assert(!debug);	   
		HMODULE hmodule = 0; 
		if(!GetExitCodeThread(dllMain,(LPDWORD)&hmodule)) assert(0);
		if(!hmodule)
		{
			#ifdef NDEBUG
			EX::messagebox(MB_OK,"SomEx.dll failed to initialize");
			TerminateProcess(pi.hProcess,0);
			#endif
		}
		CloseHandle(dllMain);
	}
	
	EXLOG_LEVEL(7) << "Resuming subprocess thread main\n";

	if(som_exe_mismatch||ResumeThread(pi.hThread)>1) //paranoia
	{
		assert(0);
		TerminateProcess(pi.hProcess,0); return 0;
	}

	return pi.dwProcessId;
}

extern void SOM::launch_legacy_exe()
{	
	DWORD pid = 0;

	wchar_t *exe = (wchar_t*)EX::exe();

	//2: rule out SOM_DB and SOM_00 pattern
	if(!wcsncmp(exe,L"SOM_",4)&&wcslen(exe+4)>2)
	{
		wchar_t *argv[] =
		{
			exe, SOMEX_L(A) L"/" SOMEX_L(B)
		};

		if(!wcscmp(exe+4,L"RUN")) argv[1] = SOMEX_L(A);

		pid = som_exe(argv,2);
	}
	else if(!wcscmp(exe,L"MapComp"))
	{
		wchar_t *argv[] =
		{
			exe, SOMEX_L(B)L"\\data\\map\\.MAP", SOMEX_L(B)L"\\data\\map\\.MPX", SOMEX_L(A), SOMEX_L(B)
		};

		pid = som_exe(argv,5);
	}
	else if(!wcscmp(L"ws",EX::log())) //HACK 
	{
		int argc = GetEnvironmentVariableW(L".WS",0,0)>1?2:1;
		wchar_t *argv[] = { exe, argc==2?SOMEX_L(A)L"\\.ws":0 };
		pid = som_exe(argv,argc); 
	}
	else //SOM_DB or a game
	{
		wchar_t map[32] = L"";

		GetEnvironmentVariableW(L".MAP",map,32);

		if(!*map) GetEnvironmentVariableW(L"TRIAL",map,32);

		//courtesy. I don't know if SOM_DB cares
		if(map[0]!='\0'&&map[1]=='\0')
		{
			map[2] = '\0'; map[1] = map[0]; map[0] = '0';
		}

		wchar_t *argv[] =
		{
			exe, SOMEX_L(B), SOMEX_L(A), map
		};

		pid = som_exe(argv,4);
	}

	wchar_t hex[9] = L"0"; _ultow(pid,hex,16); 
	SetEnvironmentVariableW(L"SomEx.dll Binary PID",hex);

	CloseHandle(SOM::exe_process);
	CloseHandle(SOM::exe_thread);
}

extern DWORD SOM::exe(const wchar_t *args)
{
	int argc = 0;
	wchar_t **argv = CommandLineToArgvW(args,&argc);	
	DWORD out = som_exe(argv,argc); LocalFree(argv); 
	//keep Windows 7 taskbar group icons in position
	EX::sleep(500);
	return out;
}

extern HWND SOM::splash()
{
	wchar_t splash[9] = L"0";
	GetEnvironmentVariableW(L"SomEx.dll Splash Screen",splash,9);
	return (HWND)wcstoul(splash,0,16);
}
extern bool SOM::splashed(HWND closer, LPARAM sw)
{
	//NOTE: this static variable is a very slight optimization since
	//SOM::splash should return 0 on second try
	static bool splashed = false; if(!splashed)
	{
		splashed = true;

		HWND win = SOM::splash();
		//says invisible when starting up with a MessageBox displayed
		//(this is probably related to Windows Vista phantom windows)
		if(IsWindow(win)) //if(IsWindowVisible(win))
		{	
			//NOTE: this should prevent entering here more than once
			SetEnvironmentVariableW(L"SomEx.dll Splash Screen",L"0");

			//2018: set to 0 to disable (or minimize?) splash screen
			DWORD to = min(3000,EX::INI::Window()->splash_screen_ms_timeout);
			if('mpx'==SOM::image()) //MapComp.exe
			to = 0;

			//speeding up because Windows 10 Creators Update introduced
			//a race condition, mainly because it seems to take a while
			//to get the splash screen u
			DWORD spell = EX::tick()-GetWindowLong(win,DWL_USER);
			//2017: Windows 10 goes to sleep and never wakes up???
			//if(spell<2500) EX::sleep(2500-spell); //3000
			if(spell<to) EX::sleep(to-spell); //3000
			if(sw&&closer) DefSubclassProc(closer,WM_SHOWWINDOW,1,sw);
			//hack: hWnd will get reactivated in case
			//users try to click on the splash screen
			SendMessage(win,WM_CLOSE,(WPARAM)closer,0);	
			return true;
		}
	}
	return false;	
}

//http://blogs.msdn.com/b/oldnewthing/archive/2012/08/17/10340743.aspx
static void IPropertyStore_SetValue(IPropertyStore *pps, REFPROPERTYKEY pkey, PCWSTR pszValue)
{
	PROPVARIANT var; if(!InitPropVariantFromString(pszValue,&var))
	{
		pps->SetValue(pkey,var); PropVariantClear(&var);
	}
}
extern void SOM::initialize_taskbar(HWND win)
{
	void *win7 = GetProcAddress
	(GetModuleHandleA("Shell32.dll"),"SHGetPropertyStoreForWindow"); 
	if(!win7) return;
	IPropertyStore *pps; 
	if(!((HRESULT(__stdcall*)(HWND,REFIID,void**))win7)(win,IID_PPV_ARGS(&pps)))
	{
		const wchar_t *group = L"SomEx Tools";
		if(SOM::game) group = !SOM::retail?L"SomEx Games":EX::exe();
		wchar_t value[MAX_PATH] = L""; swprintf_s(value,L"Swordofmoonlight.net.%ls.rev.2",group);
		IPropertyStore_SetValue(pps,PKEY_AppUserModel_ID,value);
		if(SOM::retail&&1<GetEnvironmentVariable(L"SomEx.dll Launch EXE",value,MAX_PATH))
		{	
			IPropertyStore_SetValue(pps,PKEY_AppUserModel_RelaunchCommand,value);
			IPropertyStore_SetValue(pps,PKEY_AppUserModel_RelaunchDisplayNameResource,group);			
			if(1<GetEnvironmentVariableW(L"ICON",value,MAX_PATH))
			{
				const wchar_t *ext = PathFindExtension(value);
				if(*ext&&!wcschr(ext,',')) wcscat_s(value,!wcsicmp(L".ico",ext)?L",0":L",-128");
				IPropertyStore_SetValue(pps,PKEY_AppUserModel_RelaunchIconResource,value);
			}			
		}
		else
		{	
			PROPVARIANT var; var.vt = VT_BOOL; var.boolVal = VARIANT_TRUE;
			pps->SetValue(PKEY_AppUserModel_PreventPinning,var);

			//2018/EXPERIMENTAL: (Windows 10/NEEDS TESTING)
			//Group is showing first opened's FileDescription, e.g. "Sword of Moonlight MAPEDITOR"
			//NOTE: Games and tools are grouped separately, and will have different icons, but are
			//both being titled "Sword of Moonlight" since I cannot think of something else to say
			//(2 games must be opened before grouping occurs, and maybe space must be limited too)
			IPropertyStore_SetValue(pps,PKEY_AppUserModel_RelaunchDisplayNameResource,L"Sword of Moonlight");
		}
		pps->Release();
	}														   
	else assert(0);
}