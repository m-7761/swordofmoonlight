						
#ifndef SOM_INCLUDED
#define SOM_INCLUDED

#include "../Sompaste/Sompaste.h"
#include "../lib/swordofmoonlight.h"

extern wchar_t SomEx_exe[16];

static SOMPASTE Sompaste = 0; //portable

namespace SOM{ extern int image(); } //2018

static void Som_h_softreset() //NEW: reset environment
{
	//2018: don't pollute the launcher... I'm not sure
	//if this was ever possible or contemplated, but it
	//seems to be making ".WS" revert back to an earlier
	//instance, and would have the same effect for others
	if(!SOM::image()) return;

	wchar_t *sr = (wchar_t*)
	Sompaste->get(L"Som_h_softreset",0); 
	if(sr) for(wchar_t *p=sr,*key,*val,len=*p;len;) //reset
	{							
		key = p+1; val = wcschr(key,'=')+1; p+=len; len = *p;
		val[-1] = '\0'; *p = '\0'; SetEnvironmentVariableW(key,val);
		val[-1] = '='; *p = len;
	}
	else if(sr=GetEnvironmentStringsW()) //backup initial environment
	{
		enum{pack_s=32*1024};
		wchar_t pack[pack_s+1] = L"";
		for(wchar_t *p=sr,*q=pack,len;*p;p+=len,q+=len)
		if(*p!='S'||wcsncmp(p,L"Som_h",5)||p[5]!='='&&p[5]!='_') 
		{
			len = 1+wcslen(p); if(q-pack+len>pack_s)
			EX::ok_generic_failure(L"Process environment exceeds %d code units",pack_s);
			*q = len; wmemcpy(q+1,p,len); 
		}
		else q-=len=1+wcslen(p); //NEW: skip Som_h
		SetEnvironmentVariableW(L"Som_h_softreset",pack);
		FreeEnvironmentStringsW(sr);
	}
}

static void Som_h_environ(const wchar_t *kv[2], void *redirect)
{
	if(kv[1]&&*kv[1]=='\r') kv[1]+=2; //new behavior of block text

	if(*kv[0]=='.'||*kv[0]=='#') return; //reserved/disabled variable

	if(redirect&&*(int*)redirect<2) //2020
	{
		//a redirected SOM file can only contain one SOM= directive
		//(not others) and comments. new save files begin with this

		//HACK: screen out "ERROR" and "\n"?
		if(*kv&&**kv&&kv[1]&&*kv[1])
		{
			if(!*(int*)redirect) 
			{
				*(int*)redirect = !wmemcmp(kv[0],L"SOM",4)?1:2;
			}
			else *(int*)redirect = 2;
		}
	}

	//REMINDER: I think setting kv[1] is for %env-variable% expansion
	kv[1] = Sword_of_Moonlight_Client_Library_Environ(0,kv[0],kv[1]);
}

//NEW: temp is for the SOM_EDIT.cpp settings tool
static bool Som_h(wchar_t SOM_file[MAX_PATH], const wchar_t *InstDir=L"", const wchar_t *temp=0)
{	
	namespace som = SWORDOFMOONLIGHT::som;
	
	int redirect = 0;

	if(!temp) //NEW: bypass if editing SOM file(s)
	{
		//skip if current
		if(SOM_file&&*SOM_file)
		if(!wcscmp(SOM_file,Sompaste->get(L"Som_h"))) 
		return true; //skip

		//reload current file?
		//2020: this is a hack for creating a new project
		if(!SOM_file) redirect = 2;
		if(!SOM_file) SOM_file = (wchar_t*)Sompaste->get(L"Som_h",0); 
		//remember current file
		else Sompaste->set(L"Som_h",SOM_file); assert(SOM_file);		
	}
	else Sompaste->set(L"Som_h",L""); //SOM_EDIT.cpp

	if(*SOM_file)
cd:	Som_h_softreset(); 	
	som::clean(InstDir,Som_h_environ);	
	//PLAYER
	wchar_t val[MAX_PATH*16] = L""; if(*SOM_file) 
	{
		wchar_t *Saved_Games = 0;
		HRESULT (WINAPI *Vista)(CLSID&,DWORD,HANDLE,PWSTR*) = 0;

		if(*(void**)(&Vista) = //localized Saved Games folder
		GetProcAddress(GetModuleHandleA("Shell32.dll"),"SHGetKnownFolderPath"))
		{			
			GUID FOLDERID_SavedGames = 
			{0x4C5C32FF,0xBB9D,0x43b0,0xB5,0xB4,0x2D,0x72,0xE5,0x4E,0xAA,0xA4};		
			(*Vista)(FOLDERID_SavedGames,0,0,&Saved_Games);		
			SetEnvironmentVariableW(L"PLAYER",Saved_Games); 
			CoTaskMemFree(Saved_Games);
		}
		else assert((GetVersion()&0xFF)<6);

		if(!Saved_Games) //XP
		{
			SHGetFolderPathW(0,CSIDL_APPDATA,0,SHGFP_TYPE_CURRENT,val);	
			SetEnvironmentVariableW(L"PLAYER",val);
		}
	}	
	//TEXT 
	{
		DWORD sizeofval = sizeof(val);
		LONG err = SHGetValueW(HKEY_CURRENT_USER,
		L"SOFTWARE\\FROMSOFTWARE\\SOM\\TEXT",InstDir,0,val,&sizeofval);
		if(err) err = SHGetValueW(HKEY_CURRENT_USER, //try (Default)
		L"SOFTWARE\\FROMSOFTWARE\\SOM\\TEXT",L"",0,val,&(sizeofval=sizeof(val)));
		if(!err&&*val&&wcsstr(val,L"%TEXT%"))
		{
			wchar_t swap[sizeof(val)]; 
			ExpandEnvironmentStringsW(wcscpy(swap,val),val,sizeof(val));
		}
		if(!err) SetEnvironmentVariableW(L"TEXT",val);	
	}

	som::cd(SOM_file,Som_h_environ);

	if(temp) //SOM_EDIT.cpp: temp as SOM_file 
	{
		return som::readfile(temp,Som_h_environ); 
	}
	else if(!som::readfile(SOM_file,Som_h_environ,&redirect)) 
	{
		switch(EX::messagebox(MB_CANCELTRYCONTINUE,
		"Please insert the disc marked '%ls'-'%ls' into the drive at:\n"
		"\n"
		"%ls",
		Sompaste->get(L"GAME"),Sompaste->get(L"DISC"),Sompaste->get(L"CD")))
		{
		case IDRETRY: goto cd;
		case IDCANCEL: EX::is_needed_to_shutdown_immediately(0,"Som_h"); //!
		}
		return false; 
	}
	
	if(1==redirect++) //save game SOM file?
	{
		//HACK: .LOAD is used to indicate that
		//a binary SOM file is involved, so it
		//can't be loading a tool
		const wchar_t *load[2] = {L"LOAD",0}; //"LOAD"

		//if redirecting save the old file name to SAVE in case
		//it's a safe file. if it's a binary file it's taken to
		//be a save file
		if(FILE*f=_wfopen(SOM_file,L"rb"))
		{
			char buf[MAX_PATH*8]; //UTF8?
			if(fread(buf,sizeof(buf),1,f)&&!memcmp("SOM=",buf,4))
			for(size_t i=4;i<sizeof(buf);i++) switch(buf[i])
			{
			//assuming there is just one SOM= directive, but if 
			//that ever changes \n will be valid but it must end
			//with \0 if so
			case '\r': case '\n': break; 
			case '\0': 

				//HACK: forcibly override SOM_EDIT since
				//this is known to be a binary save file
				if(*SomEx_exe) wcscpy(SomEx_exe,L"SOM_DB");
				
				load[1] = wcscpy(val,SOM_file); break;
			}				
			fclose(f);
		}
		//switch to the new SOM file
		wcscpy_s(SOM_file,MAX_PATH,Sompaste->get(L"SOM"));
		Sompaste->path(SOM_file);

		//RECURSIVE: Som_h will unset LOAD so it has to be
		//set after it returns
		bool ret = Som_h(SOM_file,InstDir);
		{
			if(load[1]) Som_h_environ(load,0); //ret?
		}
		return ret;
	}
	return true;
}

#endif //SOM_INCLUDED