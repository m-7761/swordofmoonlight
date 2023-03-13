													   
#include "Somplayer.pch.h" 

extern void Somfonts()
{
	static int once = 0;

	if(once) return; once = 1;

	HMODULE SomEx = LoadLibraryA("SomEx.dll");

	if(!SomEx) return;

	const wchar_t* (*Fonts)(bool add) = 0;

	*(void**)&Fonts = 
	GetProcAddress(SomEx,"Sword_of_Moonlight_Extension_Library_Fonts");

	if(Fonts) Fonts(true);

	FreeLibrary(SomEx);	
}

extern wchar_t *Somfolder(wchar_t inout[MAX_PATH], bool dir)
{	
	static size_t out_s = 0;
	static wchar_t out[MAX_PATH] = L""; 
	
	static bool paranoia = false; while(paranoia);

	if(paranoia=!*out)
	{
		wchar_t L[MAX_PATH] = L"%TEMP%\\Swordofmoonlight.net";
				  
		ExpandEnvironmentStringsW(L,out,MAX_PATH);

		GetLongPathNameW(out,out,MAX_PATH);

		out_s = wcslen(out);

		paranoia = false;
	}

	if(*inout)
	{
		wchar_t cat[MAX_PATH]; 
		
		wcscpy_s(cat,inout);	
		wmemcpy(inout,out,out_s+1);
		wcscpy_s(inout+out_s,MAX_PATH-out_s,cat); 
	}
	else wmemcpy(inout,out,out_s+1);

	wchar_t *slash = !dir?wcsrchr(inout,'\\'):0;

	if(!dir&&slash) *slash = '\0';

	if(!PathFileExistsW(inout)) 
	switch(SHCreateDirectoryExW(0,inout,0))
	{
	case ERROR_SUCCESS:
	case ERROR_FILE_EXISTS: 
	case ERROR_ALREADY_EXISTS: break;

	default: return 0;
	}

	if(!dir&&slash) *slash = '\\';

	return inout;
}

extern wchar_t *Somlibrary(wchar_t inout[MAX_PATH])
{	 	
	static size_t out_s = 0;
	static wchar_t out[MAX_PATH] = L""; 
	
	static bool paranoia = false; while(paranoia);

	if(paranoia=!*out)
	{
		//GetModuleBaseNameW
		if(GetModuleFileNameW(Somplayer_dll(),out,MAX_PATH))
		{
			//The online docs (no point in linking to a MS website) say to do this instead
			wchar_t *base = wcsrchr(out,'\\'); if(base) *base = '\0';
			
			out_s = base?base-out+1:0;
		}
		else assert(0);

		paranoia = false;
	}

	if(!*out) return inout;

	if(*inout)
	{
		wchar_t cat[MAX_PATH]; 
		
		wcscpy_s(cat,inout);	
		wmemcpy(inout,out,out_s);
		wcscpy_s(inout+out_s,MAX_PATH-out_s,cat); 
		
		if(out_s) inout[out_s-1] = '\\';
	}
	else wmemcpy(inout,out,out_s+1);

	return inout;
}
 