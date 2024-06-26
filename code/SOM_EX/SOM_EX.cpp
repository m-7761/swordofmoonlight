
#include <stdlib.h>
#include <assert.h>
#include <iostream>

#define WIN32_LEAN_AND_MEAN	
#define _WIN32_WINNT 0x0500 //AttachConsole
#define _WIN32_WINNT 0x0601 //ICustomDestinationList
#define _CRT_SECURE_NO_DEPRECATE //output

#include <windows.h>		
#include <windowsx.h>
#include <shellapi.h>
#include <shlwapi.h>
#pragma comment(lib,"shlwapi.lib")
#include <shobjidl.h> //ICustomDestinationList
#include <shlobj.h> //CSIDL_PROGRAMS
#include <propkey.h>
#include <propvarutil.h>
//http://blogs.msdn.com/b/oldnewthing/archive/2012/08/17/10340743.aspx
void IPropertyStore_SetValue(IPropertyStore *pps, REFPROPERTYKEY pkey, PCWSTR pszValue)
{
	PROPVARIANT var; if(!InitPropVariantFromString(pszValue,&var))
	{ pps->SetValue(pkey,var); PropVariantClear(&var); }
}

#include "../SomEx/SomEx.h"
#include "../Sompaste/Sompaste.h"
#include "../Somversion/Somversion.h"
#include "SOM_EX.h"

SOMPASTE Sompaste = 0;
HINSTANCE Instance = GetModuleHandle(0);

//in lieu of PostQuitMessage
const DWORD ThreadId = GetCurrentThreadId();
void Quit(WPARAM code){ PostThreadMessage(ThreadId,WM_QUIT,code,0); }

char Moonlight_Sword[] =							
//"\n"	   
"\n"	   
"             0\n"
"             E\n"
"            }-{<><><><><><><><><><><><><><><><><><><><><><><><>\n"
"(+}ZZZZZZZZ{<X>}SOM_EX.exe <><><><><><><><><><><><><><><><><><><>\n"
"            }-{<><><><><><><><><><><><><><><><><><><><><><><><>\n"
"             E\n"
"             0\n"
"\n";
char *Greets(SOMVERSION dll)
{
	SOMVERSION exe(SOM_EX_VERSION); 
	char *runes = strstr(Moonlight_Sword,"SOM_EX.exe ")+11;
	int diamond = sprintf(runes,"%d.%d.%d.%d ",exe[0],exe[1],exe[2],exe[3]);
	if(dll) diamond+=
	sprintf(runes+diamond,"SomEx.dll %d.%d.%d.%d",dll[0],dll[1],dll[2],dll[3]);
	runes+=diamond; *runes = runes[+1]=='>'?'<':'>'; 
	return Moonlight_Sword;
}
struct MailSlotGram
{WCHAR cwd[MAX_PATH], commandline[1];
}*GotMailSlotGram = 0;
extern "C" __declspec(dllexport) LPWSTR __stdcall CommandLine() //SomEx.cpp
{
	return GotMailSlotGram?GotMailSlotGram->commandline:GetCommandLineW();
}
HWND Splash = 0; HHOOK SplashHooked = 0;
LRESULT CALLBACK SplashHook(int code, WPARAM wParam, LPARAM lParam)
{
	assert(Splash); //theoretically there can be a race??
	if(code==HCBT_CREATEWND) SendMessage(Splash,WM_CLOSE,wParam,0);
	return CallNextHookEx(SplashHooked,code,wParam,lParam);	
}
INT_PTR CALLBACK SplashProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{	
	//Do first to speed up the race condition introduced by Windows 10 Creators Update.
	if(uMsg==WM_INITDIALOG)
	{	
		wchar_t str[9] = L"0";_ultow((DWORD)hwndDlg,str,16); 
		SetEnvironmentVariableW(L"SomEx.dll Splash Screen",str);	
		SetWindowLong(hwndDlg,DWL_USER,GetTickCount());
	}

	static LPCTSTR cursor = IDC_WAIT;
	static HDC compatdc = CreateCompatibleDC(0);
	static HBITMAP picture = LoadBitmap(GetModuleHandle(0),MAKEINTRESOURCE(IDB_SPLASH));

	switch(uMsg)
	{																
	case WM_INITDIALOG: Splash = hwndDlg;
	{	
		if(!picture) return DestroyWindow(hwndDlg); //paranoia
																	   		
		RECT wr; GetWindowRect(hwndDlg,&wr);
		BITMAP sz; GetObject(picture,sizeof(BITMAP),&sz);
		wr.right = wr.left+sz.bmWidth; wr.bottom = wr.top+sz.bmHeight;
		AdjustWindowRectEx(&wr,GetWindowStyle(hwndDlg),0,GetWindowExStyle(hwndDlg));
		SetWindowPos(hwndDlg,0,wr.left,wr.top,wr.right-wr.left,wr.bottom-wr.top,SWP_HIDEWINDOW);		
		//hack: this will update Sompaste.dll before the splash screen appears
		Sompaste->center(hwndDlg,hwndDlg,false);		

		cursor = IDC_WAIT; //ShowSplashWindow();
		//Moved above the bitmap initialization.
		//wchar_t str[9] = L"0";_ultow((DWORD)hwndDlg,str,16); 
		//SetEnvironmentVariableW(L"SomEx.dll Splash Screen",str);	

		int sw = IsDebuggerPresent()?SW_MINIMIZE:1; //2020

//		#ifndef _DEBUG
		OpenIcon(GetParent(hwndDlg));		
		ShowWindow(Splash,sw); if(!SplashHooked) //resplashing?
		//NEW: if failed to launch pull down the splash screen
		SplashHooked = SetWindowsHookExW(WH_CBT,SplashHook,0,ThreadId);
//		#endif

		return 1;
	}
	case WM_ERASEBKGND: 
	{
		RECT cr; GetClientRect(hwndDlg,&cr);
		SelectObject(compatdc,(HGDIOBJ)picture); 		
		BitBlt((HDC)wParam,0,0,cr.right,cr.bottom,compatdc,0,0,SRCCOPY);								
		SetWindowLong(hwndDlg,DWL_MSGRESULT,1);
		return 1;
	}
	case WM_SHOWWINDOW:
		
		if(wParam&&!lParam) 
		//Moved above the bitmap initialization.
		//if(cursor==IDC_WAIT)
		//SetWindowLong(hwndDlg,DWL_USER,GetTickCount());
		break;

	case WM_ACTIVATE:   

		if(cursor!=IDC_WAIT)
		cursor = LOWORD(wParam)?IDC_NO:IDC_ARROW;
		break;

	case WM_SETCURSOR:

		SetCursor(LoadCursor(0,cursor));
		SetWindowLong(hwndDlg,DWL_MSGRESULT,1);
		return 1;

	case WM_NCRBUTTONDOWN:
	case WM_NCLBUTTONDOWN: 

		if(cursor==IDC_ARROW) //prevent dragging
		{
			POINT pt; POINTSTOPOINT(pt,lParam);
			DragDetect(hwndDlg,pt); SetActiveWindow(hwndDlg);
		}
		SetWindowLong(hwndDlg,DWL_MSGRESULT,0);
		return 1;

	case WM_NCACTIVATE: 
		
		if(!wParam||cursor!=IDC_WAIT) break;		
		SetWindowLong(hwndDlg,DWL_MSGRESULT,0);
		return 1;
	
	case WM_CLOSE:
	{
		HWND gp = GetParent(hwndDlg);
		if(WS_EX_APPWINDOW&GetWindowLong(gp,GWL_EXSTYLE))
		{					
			//in case failure to launch
			DWORD now = GetTickCount();
			DWORD was = GetWindowLong(hwndDlg,DWL_USER);
			if(now-was<1000) Sleep(1000-(now-was));

			//in case splash was clicked
			if(wParam&&IsWindow((HWND)wParam))			
			SetForegroundWindow((HWND)wParam);

			cursor = IDC_ARROW; //7 thumbnail
			UnhookWindowsHookEx(SplashHooked); SplashHooked = 0;

			CloseWindow(gp); return 1; 
		}
		else DestroyWindow(hwndDlg);		
		break;
	}
	case WM_DESTROY:

		UnhookWindowsHookEx(SplashHooked); SplashHooked = 0;
		DeleteObject((HGDIOBJ)picture);		
		Splash = 0; picture = 0;
		break;
	}

	return 0;
}

NOTIFYICONDATAW Icon = 
{NOTIFYICONDATAW_V3_SIZE,0,0,NIF_ICON|NIF_TIP|NIF_MESSAGE,WM_APP+0,0,0}; //NIF_GUID
WCHAR RelaunchCommand[MAX_PATH] = L"";
LRESULT CALLBACK IconProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{		
	switch(uMsg)
	{					
	case WM_INITDIALOG:
	{
		CreateDialogW(Instance,MAKEINTRESOURCEW(IDD_SPLASH),hwndDlg,SplashProc);		

		void *win7 = GetProcAddress
		(GetModuleHandleA("Shell32.dll"),"SHGetPropertyStoreForWindow"); if(win7)
		{																			
			IPropertyStore *pps; assert(*RelaunchCommand);
			if(!((HRESULT(__stdcall*)(HWND,REFIID,void**))win7)(hwndDlg,IID_PPV_ARGS(&pps)))
			{	
				IPropertyStore_SetValue(pps,PKEY_AppUserModel_RelaunchCommand,RelaunchCommand);
				IPropertyStore_SetValue(pps,PKEY_AppUserModel_RelaunchDisplayNameResource,L"Moonlight Sword");												
				IPropertyStore_SetValue(pps,PKEY_AppUserModel_ID,L"Swordofmoonlight.net.Moonlight Sword.rev.1");
				pps->Release();
			}														   
		}

		//systray icon
		Icon.hWnd = hwndDlg;
		Icon.uID = 1;
		//can't associate a Window with NIF_GUID approach???? 
		//CoCreateGuid(&Icon.guidItem);
		//IIDFromString(L"{793FB9A5-D687-4470-8229-6D9EF3C8286E}",&Icon.guidItem);
		Icon.hIcon = (HICON)LoadImage
		(Instance,MAKEINTRESOURCE(IDI_ICON),IMAGE_ICON,16,16,0);		
		ExpandEnvironmentStringsW(L"%USERNAME%",Icon.szTip,64);
		BOOL test = Shell_NotifyIconW(NIM_ADD,&Icon);
		test = test;
		Icon.uVersion = NOTIFYICON_VERSION;	
		Shell_NotifyIconW(NIM_SETVERSION,&Icon);	
				
		//7 taskbar icon
		DWORD version = GetVersion();
		if((version&0xFF)<6||(version&0xFFFF)==0x0006)		
		{
			//XP won't hide this window w/ splash 
			//SetWindowLong(hwndDlg,GWL_STYLE,0);
			//SetWindowLong(hwndDlg,GWL_EXSTYLE,0);			
			//return !SetWindowPos(hwndDlg,0,0,0,0,0,SWP_HIDEWINDOW); 
		}
		SetWindowPos(hwndDlg,0,0,0,0,0,SWP_NOMOVE|SWP_SHOWWINDOW);
		SendMessage(hwndDlg,WM_SETICON,ICON_BIG,(LPARAM)
		LoadIcon(GetModuleHandle(0),MAKEINTRESOURCE(IDI_ICON)));
		SendMessage(hwndDlg,WM_SETICON,ICON_SMALL,(LPARAM)Icon.hIcon);		
		SetWindowLong(hwndDlg,GWL_EXSTYLE,WS_EX_APPWINDOW);
		//this window is seen in the taskbar
		SetWindowTextW(hwndDlg,Icon.szTip);
		return 0;
	}
	case WM_WINDOWPOSCHANGING: 
		
		//paranoia 		
		((WINDOWPOS*)lParam)->cx = 0;
		((WINDOWPOS*)lParam)->cy = 0; 
		break;

	case WM_SIZE: 

		if(wParam==SIZE_RESTORED) goto fore; 
		break;

	case WM_NCACTIVATE: 

		if(wParam) fore:
		{
			SetActiveWindow(Splash);
			return 1;		
		}
		break;

	//would work if not triggered by the task bar
	//case WM_ACTIVATEAPP:

	//	if(!wParam) CloseWindow(hwndDlg);
	//	break;
	
	case WM_SYSCOMMAND: 

		switch(wParam) 
		{
		//permits nested close out
		case SC_CLOSE: goto close;
		}
		break;

	case WM_APP+0: //Shell_NotifyIcon

		if(lParam==WM_CONTEXTMENU)
		{			
			//seems like a Windows bug
			//http://stackoverflow.com/questions/5597525/
			//POINTS pt = MAKEPOINTS(GetMessagePos());
			POINT pt; GetCursorPos(&pt);  			

			static HMENU main = 0, menu = 
			LoadMenu(GetModuleHandle(0),MAKEINTRESOURCE(IDR_ICON)); 

			if(!main) //initialize
			{			
				main = GetSubMenu(menu,0); assert(main);

				MENUITEMINFO info = {sizeof(info),MIIM_BITMAP};

				info.hbmpItem = HBMMENU_CALLBACK;			
				if(!SetMenuItemInfo(main,ID_MAIN_EXIT,0,&info)) assert(0);
			}	  			

			//looks like more Win bugs 
			//http://www.codeproject.com/Articles/4768/
			SetForegroundWindow(hwndDlg);
			switch(TrackPopupMenu(main,TPM_RETURNCMD,pt.x,pt.y,0,hwndDlg,0))
			{
			case ID_MAIN_EXIT: goto close;
			}			
		}
		break;		

	case WM_MEASUREITEM: //32x32 icon
	{
		MEASUREITEMSTRUCT *meter = (MEASUREITEMSTRUCT*)lParam;

		if(meter->CtlType!=ODT_MENU) break;

		meter->itemWidth = meter->itemHeight = 32;

		SetWindowLong(hwndDlg,DWL_MSGRESULT,1);
		return 1;
	}
	case WM_DRAWITEM:
	{													   
		DRAWITEMSTRUCT *frame = (DRAWITEMSTRUCT*)lParam;

		if(frame->CtlType!=ODT_MENU||frame->itemID!=ID_MAIN_EXIT) break;
		
		static HICON exit = (HICON)
		LoadImage(GetModuleHandle(0),MAKEINTRESOURCE(IDI_EXIT),IMAGE_ICON,32,32,0);

		int bug = 32-GetSystemMetrics(SM_CXMENUCHECK);
		DrawIcon(frame->hDC,frame->rcItem.left-bug,frame->rcItem.top,exit);

		SetWindowLong(hwndDlg,DWL_MSGRESULT,1);
		return 1;
	}
	case WM_CLOSE: close: 

		//PostQuitMessage(0);
		//PostMessage(0,WM_QUIT,0,0);
		//AttachThreadInput doesn't register PostQuitMessage???		
		Quit(0); 		
		return 1; //keeps splash down

	case WM_DESTROY:
		
		Shell_NotifyIconW(NIM_DELETE,&Icon);
		break;
	}

	//return 0;
	return DefWindowProcW(hwndDlg,uMsg,wParam,lParam);
}
/*struct Win7_tasks : IObjectArray
{
	ULONG refs;
	Win7_tasks():refs(1){}
	ULONG __stdcall AddRef(void)
	{return InterlockedIncrement(&refs);}
	ULONG __stdcall Release(void)
	{return InterlockedDecrement(&refs);}
	HRESULT __stdcall QueryInterface(REFIID riid, void **ppvObj)
	{
		IUnknown *punk = 0;
		if(riid==IID_IUnknown)
		punk = static_cast<IUnknown*>(this);
		else if(riid==IID_IObjectArray)
		punk = static_cast<IObjectArray*>(this);
		if(*ppvObj=punk) punk->AddRef();
		else return E_NOINTERFACE;
		return S_OK;
	}			
	HRESULT __stdcall GetCount(UINT*pcObjects)
	{
		*pcObjects = 1; return S_OK;
	}        
	HRESULT __stdcall GetAt(UINT i,REFIID riid,void**ppv)
	{										   
		wchar_t out[MAX_PATH], *link = 0, *title; switch(i)
		{
		case 0: title = L"Setup"; //PER GetCount ABOVE
			
			link = L"\\Swordofmoonlight.net\\Dragon Sword.lnk"; break;
		}
		IShellLinkW* psl; IPersistFile *ppf; IPropertyStore *pps;
		if(link&&!SHGetFolderPathW(0,CSIDL_PROGRAMS,0,SHGFP_TYPE_CURRENT,out)
		 &&!CoCreateInstance(CLSID_ShellLink,0,CLSCTX_INPROC_SERVER,IID_IShellLinkW,(void**)&psl))
		{	
			if(!psl->QueryInterface(IID_IPersistFile,(void**)&ppf)) //how the hell do you know this?
			{
				wcscat_s(out,link);
				if(!ppf->Load(out,STGM_READWRITE)) //abort?
				if(!psl->QueryInterface(IID_IPropertyStore,(void**)&pps)) //totally undocument/required??
				{
					//note: without titles the list is aborted
					PROPVARIANT pv = {VT_LPWSTR}; pv.pwszVal = title;
					PROPERTYKEY pk = 
					{{0xf29f85e0,0x4ff9,0x1068,0xab,0x91,0x08,0x00,0x2b,0x27,0xb3,0xd9},2};
					pps->SetValue(pk,pv); pps->Commit(); pps->Release(); 
				}
				ppf->Release();
				psl->QueryInterface(riid,ppv); psl->Release();				
				return S_OK;
			}
			else psl->Release();
		}
		return E_FAIL;
	}			   
};*/
DWORD WINAPI IconThreadProc(LPVOID tid) 
{	
	//may be deadlocking Somversion_hpp mutex
	//PostQuitMessage (still doesn't work???)	
	//PostMessage(0,0,0,0); //required for Attach to work
	//AttachThreadInput(GetCurrentThreadId(),(DWORD)tid,1);
		
	/*10: CoCreateInstance fails
	ICustomDestinationList *pcdl = 0;
	//just do this everytime in case the shortcuts change		
	if(!CoCreateInstance(CLSID_DestinationList,0,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&pcdl)))
	{
		UINT room; IObjectArray *removed; 
		if(!pcdl->BeginList(&room,IID_PPV_ARGS(&removed)))
		{	
			Win7_tasks tasks; 							
			if(pcdl->AddUserTasks(&tasks)) pcdl->AbortList();
			pcdl->CommitList();					
			removed->Release();
		}		 
		pcdl->Release();
	}*/
	//2024: need a class to use FindWindow and try to interact
	//return DialogBoxW(Instance,MAKEINTRESOURCEW(IDD_ICON),0,IconProc);			
	WNDCLASSW systray_class = 
	{0,IconProc,0,0,0,0,0,(HBRUSH)GetStockObject(BLACK_BRUSH),0,L"Swordofmoonlight.net Systray WNDCLASS"};
	RegisterClassW(&systray_class);
	HWND hw = CreateWindowExW(WS_EX_APPWINDOW,L"Swordofmoonlight.net Systray WNDCLASS",L"SOM_EX",WS_MINIMIZEBOX|WS_POPUP|WS_CAPTION|WS_SYSMENU,0,0,0,0,0,0,0,0);
	SendMessage(hw,WM_INITDIALOG,0,0);
	MSG msg;
	while(GetMessageW(&msg,0,0,0))
	{
		if(!IsWindow(hw)) break; //HACK

		DispatchMessageW(&msg);
	}

	return 1;
}

void Wrapup(HANDLE descendant)
{
	if(1) //experimental: call ExitProcess from within program
	{
		//TODO: this just ensures the process is ended normally
		//It takes more to hassle the user (if their preference)
		LPTHREAD_START_ROUTINE ExitProcessProc=(LPTHREAD_START_ROUTINE)
		GetProcAddress(GetModuleHandle("kernel32.dll"),"ExitProcess");
		HANDLE save_your_work = 
		CreateRemoteThread(descendant,0,0,ExitProcessProc,0,0,0);		
		assert(save_your_work);
		//todo: if taking too long, offer to TerminateProcess
		//DWORD debug = WaitForSingleObject(save_your_work,INFINITE); 
		CloseHandle(save_your_work);
		DWORD debug = WaitForSingleObject(descendant,INFINITE); 
		assert(!debug);	   
		DWORD exit_code = 1;
		if(!GetExitCodeProcess(descendant,(LPDWORD)&exit_code)) assert(0);
		assert(!exit_code);
	}
	TerminateProcess(descendant,0); 	
}
int Exiting(int out)
{	 
	//hack: doesn't actually destroy
	//Shell_NotifyIconW(NIM_DELETE,&Icon))
	SendMessage(Icon.hWnd,WM_DESTROY,0,0); 
	//CoUninitialize();
	OleUninitialize(); return out;
}
int APIENTRY wWinMain(HINSTANCE hinstance, HINSTANCE, LPWSTR, int)
{						  
	#define return return Exiting	
	//CoInitializeEx(0,COINIT_MULTITHREADED);
	OleInitialize(0);

	wchar_t cwd[MAX_PATH] = L"";  
	size_t cwdlen = GetCurrentDirectoryW(MAX_PATH,cwd);

	//////// execution path ////////////////
	
	wchar_t path[MAX_PATH] = L"";  
	wmemcpy(RelaunchCommand,path,1+GetModuleFileNameW(0,path,MAX_PATH));	
	wchar_t *path_s = PathFindFileNameW(path); assert(path_s>path);
	if(path_s-path+sizeof("EX\\SYSTEM")<MAX_PATH-1)
	{
		wcscpy(path_s,L"EX\\SYSTEM");		
		if(!PathIsDirectoryW(path)) path_s[-1] = '\0';
	}
	else path_s[-1] = '\0'; //strip slash

	SetEnvironmentVariableW(L"PATH",path);
	SetEnvironmentVariableW(L"SomEx.dll Launch EXE",RelaunchCommand);

	///// THERE CAN BE ONLY ONE ////////////

	const wchar_t *slot = L"\\\\.\\mailslot\\Swordofmoonlight.net.Moonlight Sword";
	//Woops: A mailslot is not an event??
	HANDLE waitforit[2] = {0,CreateEventW(0,0,0,wcsrchr(slot,'\\')+1)};
	HANDLE mailslot = CreateMailslotW(slot,1024,0,0);
	if(!path_s[-1]) //exclude EX\\SYSTEM 
	if(mailslot==INVALID_HANDLE_VALUE)
	{
		HANDLE h = 
		CreateFileW(slot,GENERIC_WRITE,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
		if(h!=INVALID_HANDLE_VALUE)
		{
			LPWSTR gcl = GetCommandLineW();
			DWORD wr = sizeof(MailSlotGram)+wcslen(gcl)*sizeof(WCHAR);
			MailSlotGram *p = (MailSlotGram*)new BYTE[wr];
			wcscpy(p->cwd,cwd); wcscpy(p->commandline,gcl);
			WriteFile(h,p,wr,&wr,0); CloseHandle(h); delete[] p; 

			if(wr&&SetEvent(waitforit[1])) 
			{
				return(0); //handed off to singleton instance
			}
		}
		slot = 0; //TODO: report mailslot disconnect??
	}		

	///////// update and launch //////////////
																	 
	SOMVERSION csv; csv.version(0,L"SomEx.dll");
	
	//////// commandline console /////////////
	
	if(AttachConsole(ATTACH_PARENT_PROCESS))
	{	
		freopen("CON","w",stdout);

		HANDLE conout = 
		GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO info;
		GetConsoleScreenBufferInfo(conout,&info);

		if(info.dwCursorPosition.Y)
		{										
			COORD _ = {0,max(info.dwCursorPosition.Y-1,0)};

			if(SetConsoleCursorPosition(conout,_))
			{
				DWORD filled = 0;
				//Hmmm?? Does not work when FillConsoleOutputAttribute is used
				//FillConsoleOutputAttribute(c,0x0000,2*info.dwSize.X,_,&filled);
				FillConsoleOutputCharacter(conout,' ',2*info.dwSize.X,_,&filled);
			}
		}
		
		std::cout << Greets(csv); //Moonlight_Sword
			   		
		//freezes if cwd can't be represented by C locale
		//std::wcout << '\n' << cwd << '>' << std::flush;

		DWORD required;		
		WriteConsoleW(conout,L"\n",1,&required,0); 
		WriteConsoleW(conout,cwd,cwdlen,&required,0);
		WriteConsoleW(conout,L">",1,&required,0); 

		FreeConsole(); //cmd.exe prompt
	}	
	
	///////// icon w/ splash screen //////////

	if(!path_s[-1]) //hack: exclude EX\\SYSTEM 
	{
		//may be deadlocking Somversion_hpp mutex
		//PostMessage(0,0,0,0); //AttachThreadInput
		CreateThread(0,0,IconThreadProc,(LPVOID)ThreadId,0,0);

		//Hack: Windows 10 Creators Update has a race condition :(
		wchar_t splash[9] = L"0";
		while(0==GetEnvironmentVariableW(L"SomEx.dll Splash Screen",splash,9))
		Sleep(25);
	}
		  
	//////// Windows job security ////////////

	SECURITY_ATTRIBUTES inherit = {sizeof(inherit),0,1};

	//could use a named job
	HANDLE job = CreateJobObject(&inherit,0);	
	//close launched process if terminated		 
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION tie; memset(&tie,0x00,sizeof(tie)); 
	//Doesn't work with inherited jobs (grand children)
	//tie.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
	//This is required for SOM_EDIT and onward only because
	//SomEx.dll must use CREATE_BREAKAWAY_FROM_JOB to workaround 
	//outside meddling courtesy of Windows and or third party products
	tie.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
	SetInformationJobObject(job,JobObjectExtendedLimitInformation,&tie,sizeof(tie));

	//////// self launch sequence ////////////
	
	char str[9] = "0"; _ultoa((DWORD)job,str,16); 
	SetEnvironmentVariableA("SomEx.dll Binary Job",str); //som.exe.cpp
	//WOW64 or Windows 10 one isn't unloading
	//(still Somversion.hpp cannot handle unloads either)
	//if(!FreeLibrary(Sword_of_Moonlight(cwd))) return(0); //SomEx.h	
	if(!Sword_of_Moonlight(cwd)) return(0); //SomEx.h	
	GetEnvironmentVariableA("SomEx.dll Binary PID",str,9); //som.exe.cpp
	if(path_s[-1]) return(0); //NEW: Jettisoning launcher

	//////// wait for termination ////////////
		
	DWORD access_rights = //ALL_ACCESS stopped working today
	PROCESS_QUERY_INFORMATION|PROCESS_TERMINATE|SYNCHRONIZE;
	DWORD wrapup_rights = //Wrapup (see its defintion above)
	PROCESS_CREATE_THREAD|PROCESS_VM_OPERATION|PROCESS_VM_READ|PROCESS_VM_WRITE; 

	DWORD pid = strtoul(str,0,16); assert(pid);
	HANDLE &exe = waitforit[0] = OpenProcess(access_rights|wrapup_rights,0,pid); 
	if(!exe) return(0);								
								   
	MSG msg; bool quit = false; assert(slot);
	while(1) switch(MsgWaitForMultipleObjects(2,waitforit,0,INFINITE,QS_ALLINPUT))
	{	
	default: assert(0); continue;

	case 1: //another SOM_EX.exe?

		for(DWORD next,rd;GetMailslotInfo(mailslot,0,&next,0,0)&&next!=MAILSLOT_NO_MESSAGE;)
		{
			GotMailSlotGram = (MailSlotGram*)new BYTE[next];
			if(ReadFile(mailslot,GotMailSlotGram,next,&rd,0)&&rd>=sizeof(GotMailSlotGram))
			{
				if(!IsDebuggerPresent())
				SendMessage(Splash,WM_INITDIALOG,0,0);
				//NOTE: not updating SomEx.dll via Somversion.dll
				Sword_of_Moonlight(GotMailSlotGram->cwd);
			}
			delete[] GotMailSlotGram; GotMailSlotGram = 0;
		}		
		continue;

	case 2: //wait for WM_QUIT to end job
			
		while(PeekMessageW(&msg,0,0,0,PM_REMOVE))
		{ 
			if(quit=msg.message==WM_QUIT) goto quit; DispatchMessage(&msg); 
		}
		continue; quit: Wrapup(exe);

	case 0: CloseHandle(exe); exe = 0; //next?		

	//NEW: dunno if this is the best idea???
	case WAIT_ABANDONED+0: case WAIT_FAILED:
										  
		DWORD bug_to_do_with_WOW64[2+64] = {0,0,0};
		//returns false flag error ERROR_MORE_DATA 
		JOBOBJECT_BASIC_PROCESS_ID_LIST //pids = {0,0};		
		&pids = *(JOBOBJECT_BASIC_PROCESS_ID_LIST*)bug_to_do_with_WOW64;
		//QueryInformationJobObject(job,JobObjectBasicProcessIdList,&pids,sizeof(pids),0);
		QueryInformationJobObject(job,JobObjectBasicProcessIdList,&pids,sizeof(bug_to_do_with_WOW64),0);		
		
		if(pids.NumberOfProcessIdsInList)
		{
			exe = OpenProcess(PROCESS_ALL_ACCESS,0,pids.ProcessIdList[0]);	
				
			if(!quit) continue; goto quit;
		}
		//if(quit) DispatchMessage(&msg); 
		return(0); //while(1)
	}
	return(0);
}
