			
#include <stdio.h> 
#include <iostream>
#include <iomanip>
#include <fstream>
#include <assert.h>

#define _WIN32_WINNT 0x0500 //AttachConsole
#define WIN32_LEAN_AND_MEAN																					

#include <windows.h> 
#include <shellapi.h> //CommandLineToArgvW
#include <shlwapi.h> //PathFindExtensionW

#pragma comment(lib,"shlwapi.lib")

#include "../Somversion/Somversion.h"

#include "SOM.h"

int Exiting(int out)
{														   
	//CoUninitialize();
	OleUninitialize(); return out;
}
int APIENTRY wWinMain(HINSTANCE,HINSTANCE,LPWSTR,int)
{			
	#define return return Exiting	
	//CoInitializeEx(0,COINIT_MULTITHREADED);
	OleInitialize(0);

	//Windows 7 (consider IDeskBand for XP/Vista) 		
	//This is mainly to enable the Close button for Tasks
	void *merge_taskbar = 
	GetProcAddress(GetModuleHandleA("Shell32.dll"),"SetCurrentProcessExplicitAppUserModelID");
	if(merge_taskbar) 
	((HRESULT(__stdcall*)(PCWSTR))merge_taskbar)(L"Sword of Moonlight Dragon Sword");

	int argc = 0;
	LPWSTR cli = GetCommandLineW();
	LPWSTR *argv = CommandLineToArgvW(cli,&argc);
	
	BOOL con = AttachConsole(ATTACH_PARENT_PROCESS);			
	HWND gcw = GetConsoleWindow();

	SOMVERSION v; //checking for support
	v.version(gcw,L"Somversion.dll",v.noupdate);
	if(v<SOMVERSION(1,0,1,6)) //1.0.1.6
	{
		MessageBoxW(gcw,
		L"Please update to version 1.0.1.6 or later...",
		L"Checking Somversion.dll",MB_OK);
		v.version(gcw,L"Somversion.dll",v.doupdate);
	}
	if(v>=SOMVERSION(1,0,1,6)) //1.0.1.6
	{
		//1.0.4.2 picks up on DAE files if passed "" and
		//not handed CSV files.
		for(int i=1;i<argc;i++)
		if(!wcsicmp(L".csv",PathFindExtensionW(argv[i])))
		v.version(gcw,argv[i],v.queueup); 
		v.version(gcw,L"",v.doupdate); 
	}

	FreeConsole(); return(0);
}

