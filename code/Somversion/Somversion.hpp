
#ifndef SOMVERSION_HPP_INCLUDED
#define SOMVERSION_HPP_INCLUDED

/*this file provides portable delay load macros*/
/*that assume the libaries will not be unloaded*/

#ifndef _WIN32
#error unimplemented
#else

#ifdef SOMVERSION_PCH_INCLUDED
/*SOMVERSION_PCH_INCLUDED implicates Somversion.dll*/
/*Somversion cannot hold onto libraries permanently*/
#define SOMVERSION_HPP_INTERFACE_(lib,out,imp,prot,args) \
{return((out(*)prot)Somversion_hpp(L#lib L".dll",#imp))args;}
#else /*!SOMVERSION_PCH_INCLUDED*/
#define SOMVERSION_HPP_INTERFACE_(lib,out,imp,prot,args) \
{static void*jmp=Somversion_hpp(L#lib L".dll",#imp);return((out(*)prot)jmp)args;}
#endif /*SOMVERSION_PCH_INCLUDED*/
#define SOMVERSION_HPP_INTERFACE(lib,out,imp,prot,args) \
		SOMVERSION_HPP_INTERFACE_(lib,out,imp,prot,args) 

/*this can trigger updates whenever it's convenient*/
#define SOMVERSION_HPP_REPLACE_(lib) Somversion_hpp_dl(L#lib L".dll",0);
#define SOMVERSION_HPP_REPLACE(lib) SOMVERSION_HPP_REPLACE_(lib)

static void *Somversion_hpp_dl(const wchar_t *dll, const char *proc)
{		
	HMODULE lib = GetModuleHandleW(dll);
	if(lib) return GetProcAddress(lib,proc);

	/*lib must be preloaded*/
	#ifdef SOMVERSION_PCH_INCLUDED /*Somversion.dll*/
	assert(0); return 0; 
	#endif

	HANDLE mutex = CreateMutexA(0,0,"Somversion_hpp");

	//ensure that updates do not collide
	WaitForSingleObject(mutex,INFINITE);
				  
	wchar_t pe[2][MAX_PATH] = {L"Somversion.dll",L""};

	//long long int: don't want to depend on Somversion.h
	typedef long long int (*Version_t)(HWND,wchar_t[],int);

	HMODULE Somversion = GetModuleHandleW(L"Somversion.dll");

	if(!Somversion) //update Somversion
	if(Somversion=LoadLibraryW(L"Somversion.dll"))	
	{	
		Version_t Assist = (Version_t)
		GetProcAddress(Somversion,"Sword_of_Moonlight_Subversion_Library_Assist");

		//note: could probably do without v
		long long int v = Assist(0,pe[0],-1); 

		if((v&0xFF)&&*pe[1]) //todo: SHFileOperation?
		{
			FreeLibrary(Somversion);

			CopyFileW(pe[1],pe[0],0); DeleteFileW(pe[1]);

			Somversion = LoadLibraryW(L"Somversion.dll");
		}
	}
	if(Somversion)
	if(Somversion!=GetModuleHandleW(dll))
	{
		Version_t Version = (Version_t)
		GetProcAddress(Somversion,"Sword_of_Moonlight_Subversion_Library_Version");

		wcscpy_s(pe[0],dll); Version(0,pe[0],-1); 		
	}

	ReleaseMutex(mutex); CloseHandle(mutex);

	lib = LoadLibraryW(dll);
	if(proc)
	return GetProcAddress(lib,proc);
	return 0;
}

inline void *Somversion_hpp(const wchar_t *dll, const char *proc)
{
	void *out = Somversion_hpp_dl(dll,proc); if(out) return out;
	MessageBoxW(0,L"This Sword of Moonlight environment component is missing or out-of-date.",dll,MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
	ExitProcess(0); //should allow user to save their work
}

#endif
#endif //SOMVERSION_HPP_INCLUDED