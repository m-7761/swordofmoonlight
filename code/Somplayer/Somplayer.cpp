		
#include "Somplayer.pch.h"
 
#include "Somthread.h"
#include "Somconsole.h"

static Somplayer *therecanbeonlyonefornow = 0;

SOMPLAYER_API SOMPLAYER SOMPLAYER_LIB(Connect)(const wchar_t v[4], const wchar_t *party)
{	
	if(!party&&!therecanbeonlyonefornow) 
	{
		return therecanbeonlyonefornow = Somconnect();		
	}
	else assert(0); return 0; //unimplemented
}

namespace Somplayer_cpp
{
	extern "C"{
	typedef struct 
	{
		void *vtable; int status;

	}c; //Somplayer.h
	}
}

SOMPLAYER_API SOMPLAYER SOMPLAYER_LIB(Status)(SOMPLAYER pt) 
{
	if(!pt) return 0;

	Somplayer_cpp::c *c = 0;
	SOMPLAYER cplusplus = 0;	
	
	const long long a = (long long)&c->status;
	const long long b = (long long)&cplusplus->status;

	return (SOMPLAYER)((char*)pt+b-a);
}

SOMPLAYER_API void SOMPLAYER_LIB(Disconnect)(SOMPLAYER pt, bool host)
{
	if(pt==therecanbeonlyonefornow) 
	{
		delete therecanbeonlyonefornow; therecanbeonlyonefornow = 0;
	}
	else assert(0);
}

SOMPLAYER_API size_t SOMPLAYER_LIB(Open)(SOMPLAYER pt, const wchar_t path[MAX_PATH], bool play)
{
	return pt->open(path,play);
}

SOMPLAYER_API size_t SOMPLAYER_LIB(Listing)(SOMPLAYER pt, const wchar_t **inout, size_t inout_s, size_t skip)
{
	return pt->listing(inout,inout_s,skip);
}

SOMPLAYER_API const wchar_t *SOMPLAYER_LIB(Change)(SOMPLAYER pt, const wchar_t *play, double skip)
{
	return pt->change(play,skip);
}

SOMPLAYER_API bool SOMPLAYER_LIB(Current)(SOMPLAYER pt, const wchar_t *item, void *reserved)
{
	return pt->current(item,reserved);
}

SOMPLAYER_API bool SOMPLAYER_LIB(Capture)(SOMPLAYER pt, const wchar_t *surrounding)
{
	return pt->capture(surrounding);
}

SOMPLAYER_API const wchar_t *SOMPLAYER_LIB(Captive)(SOMPLAYER pt)
{
	return pt->captive();
}

SOMPLAYER_API const wchar_t *SOMPLAYER_LIB(Release)(SOMPLAYER pt)
{
	return pt->release();
}

SOMPLAYER_API bool SOMPLAYER_LIB(Priority)(SOMPLAYER pt, const wchar_t **inout, size_t inout_s)
{
	return pt->priority(inout,inout_s);
}

SOMPLAYER_API double SOMPLAYER_LIB(Vicinity)(SOMPLAYER pt, double meters, bool visibility)
{
	return pt->vicinity(meters,visibility);
}

SOMPLAYER_API size_t SOMPLAYER_LIB(Surrounding)(SOMPLAYER pt, const wchar_t **inout, size_t inout_s)
{
	return pt->surrounding(inout,inout_s);
}

SOMPLAYER_API size_t SOMPLAYER_LIB(Perspective)(SOMPLAYER pt, const wchar_t**inout, size_t inout_s)
{
	return pt->perspective(inout,inout_s);
}

SOMPLAYER_API size_t SOMPLAYER_LIB(Control)(SOMPLAYER pt, HWND window, size_t N)
{
	return pt->control(window,N);
}

SOMPLAYER_API size_t SOMPLAYER_LIB(Picture)(SOMPLAYER pt, HWND window, size_t N)
{
	return pt->picture(window,N);
}

SOMPLAYER_API size_t SOMPLAYER_LIB(Texture)(SOMPLAYER pt, HWND window, size_t N)
{
	return pt->texture(window,N);
}

SOMPLAYER_API size_t SOMPLAYER_LIB(Palette)(SOMPLAYER pt, HWND window, size_t N)
{
	return pt->palette(window,N);
}

HMENU SOMPLAYER_LIB(Context)(SOMPLAYER pt, const wchar_t *item, HWND window, const char *menutext, size_t *ID)
{
	return pt->context(item,window,menutext,ID);
}

static HMODULE Somplayer_hmodule = 0;

static bool Somplayer_detached = false;

extern HMODULE Somplayer_dll()
{
	//has DllMain been entered?
	assert(Somplayer_hmodule||Somplayer_detached); 

	return Somplayer_hmodule;
}

static std::vector<void*> Somplayer_delete;

extern void Somdelete(void *del)
{
	Somthread_h::section cs;
	Somplayer_delete.push_back(del);	
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	extern void Somthumb_clsid(bool); //Somthumb.cpp
														   
	if(ul_reason_for_call==DLL_PROCESS_ATTACH)
	{	
		Somplayer_hmodule = hModule; 

		Somthumb_clsid(0); //hack: update thumb server
	}	
	else if(ul_reason_for_call==DLL_THREAD_DETACH)
	{

	}
	else if(ul_reason_for_call==DLL_PROCESS_DETACH)
	{
		Somplayer_detached = true; //Somplayer_dll

		size_t i = Somplayer_delete.size();
		while(i>0) delete Somplayer_delete[--i];

		if(lpReserved) Somplayer_hmodule = 0; //...

		//interlocking DLLs 
		//semi-documented behavior
		//http://blogs.msdn.com/b/larryosterman/archive/2004/06/10/152794.aspx
		if(lpReserved) return TRUE; //terminating

		/////WARNING/////////////////////////
		//
		// It is more crucial than usual that 
		// we see that no memory is leaked so
		// that the Shell Extension component
		// does not leave memory lying around
		// when unloading from an instance of
		// Explorer (and other shell clients)

		//SompalD3D9.cpp
		//extern volatile bool SompalD3D9_released; 

		//assert(SompalD3D9_released); //SompalD3D9_release(0);

		//TODO: make sure consoles are closed
	}

    return TRUE;
} 