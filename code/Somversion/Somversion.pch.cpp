													   
#include "Somversion.pch.h" 

extern void Somfont()
{	
	static bool once = false; if(once++) return;

	wchar_t font[MAX_PATH] = L"";
	DWORD max_path = sizeof(font);		
	SHGetValueW(HKEY_CURRENT_USER,
	L"SOFTWARE\\FROMSOFTWARE\\SOM\\INSTALL",L"InstDir",0,font,&max_path);
	size_t cat = wcslen(font); if(!cat) return;
	wcscat_s(font+cat,MAX_PATH-cat,L"\\font"); cat+=5;

	WIN32_FIND_DATAW found; 

	wchar_t *file = font+cat+1;				
	wcscpy_s(file-1,MAX_PATH-cat,L"\\*");

	HANDLE glob = FindFirstFileW(font,&found);

	if(glob==INVALID_HANDLE_VALUE) return;
	
	for(int safety=0;1;safety++)
	{
		wcscpy(file,found.cFileName);	
		AddFontResourceExW(font,FR_PRIVATE|FR_NOT_ENUM,0);
		
		//TODO: safety dialog
		if(!FindNextFileW(glob,&found)||safety>32)
		{				
			assert(GetLastError()==ERROR_NO_MORE_FILES);
					
			FindClose(glob); break;
		}		
	}
}

extern wchar_t *Somlegalize(wchar_t inout[MAX_PATH])
{
	for(int i=0;inout[i]&&i<MAX_PATH;i++) switch(inout[i])
	{
	//Not allowed by file systems (so says docs)
	case '<': case '>': case ':': case '"': case '|': case '/': case '\\':

	//Not allowed by Explorer.exe (so says Explorer->rename)
	case '*': case '?':					 
					
		inout[i] = '-';	break;

	case ' ': //preference

		inout[i] = '_';	break;
	}
	return inout;
}

extern wchar_t *Somfolder(wchar_t inout[MAX_PATH], bool dir)
{	
	static size_t temp_s = 0;
	static wchar_t temp[MAX_PATH] = L""; 
	
	if(!*temp) //hack: initialized by DllMain
	{								   
		wchar_t L[MAX_PATH] = 
		L"%TEMP%\\Swordofmoonlight.net";		
		ExpandEnvironmentStringsW(L,temp,MAX_PATH);	
		//NEW: GetLongPathNameW
		if(!PathFileExistsW(temp))
		SHCreateDirectoryExW(0,temp,0); 
		if(!*temp||!GetLongPathNameW(temp,L,MAX_PATH))
		{						 			
			if(GetLastError()==5) //junction?
			wcscpy(L,temp); else return 0; //yikes
		}
		temp_s = wcslen(wcscpy(temp,L));
	}

	if(!inout) return 0; //NEW: initializing by DllMain

	if(*inout)
	{
		wchar_t cat[MAX_PATH]; 
		
		wcscpy_s(cat,inout);	
		wmemcpy(inout,temp,temp_s+1);
		wcscpy_s(inout+temp_s,MAX_PATH-temp_s,cat); 
	}
	else wmemcpy(inout,temp,temp_s+1);

	//WARNING! On XP (differs from Vista/7)
	//PathFindFileNameW returns trailing slash
	//when the given file name is an empty string
	wchar_t *sep = dir?0:PathFindFileNameW(inout);
	wchar_t slash = sep?*sep:0;

	if(slash) *sep = '\0';

	if(!PathFileExistsW(inout)) 
	switch(SHCreateDirectoryExW(0,inout,0))
	{
	case ERROR_SUCCESS:
	case ERROR_FILE_EXISTS: 
	case ERROR_ALREADY_EXISTS: break;

	default: return 0;
	}

	if(slash) *sep = slash;

	return inout;
}