												 				
#include "Ex.h"
EX_TRANSLATION_UNIT //(C)
#pragma hdrstop

#include <deque>
#include <vector>

#include "Ex.ini.h" 
#include "Ex.output.h"
#include "Ex.langs.h" 

#include "../Sompaste/Sompaste.h"

static SOMPASTE Sompaste = 0;

extern DWORD EX::tls(short mode, DWORD get)
{	
	static std::vector<DWORD> tls_index;

	if(mode<=0)
	{
		EX_CRITICAL_SECTION
				
		DWORD out = tls_index.size();

		for(int count=abs(mode);count;count--)
		{
			DWORD paranoia = TlsAlloc(); 
			
			if(paranoia==TLS_OUT_OF_INDEXES)
			{
				EX::ok_generic_failure
				(L"Woops!! We've run out of thread local storage indices."); 
				EX::is_needed_to_shutdown_immediately(1);
			}
			else tls_index.push_back(paranoia);
		}

		return out;
	}
	
	const size_t size = mode; //bytes

	DWORD *swap = (DWORD*)TlsGetValue(tls_index[get]);

	if(swap&&swap[0]>=size) return DWORD(swap+1);

	DWORD *p = (DWORD*)new char[sizeof(DWORD)+size];

	if(!TlsSetValue(tls_index[get],p))
	{
		EX::ok_generic_failure
		(L"Woops!! Was unable to set thread local storage index.");
		EX::is_needed_to_shutdown_immediately(1);
	}
	else if(swap)
	{
		char *set = (char*)
		memcpy(p+1,swap+1,swap[0]);
		memset(set+swap[0],0x00,size-swap[0]);
		delete[] swap;
	}
	else memset(p+1,0x00,size);

	p[0] = size;

	return DWORD(p+1);
}

namespace Ex_cpp
{
	typedef wchar_t place[MAX_PATH];

	static const place noplace = L"";

	struct deplace //C2075
	{
		place myplace; 

		operator place&(){ return myplace; }

		deplace(const place &cp){ wcscpy_s(myplace,cp); } 
	};		
	static std::deque<deplace> langs, fonts, texts, inies;	

static int cd_placed = false, user_placed = false; //HACK
}			   
extern void Ex_2018_reset_SOM_EX_or_SOM_EDIT_tool() //HACK
{
	using namespace Ex_cpp;
	cd_placed = user_placed = false;
	langs.clear(); fonts.clear(); texts.clear(); inies.clear();
}

extern const wchar_t *EX::cd()
{
	static Ex_cpp::place out = L""; 
	
	//SOM_MAIN and SOM_RUN should have empty strings
	//static bool one_off = false; if(one_off++) return out; //???
	if(!Ex_cpp::cd_placed) Ex_cpp::cd_placed = true;
	else return out;
	
	const wchar_t *cd = Sompaste->get(L"CD");
	
	if(*cd) Sompaste->place(0,out,cd,'?'); 
	
	//TODO: SOM::Game::project expects this is nonempty
	return out;
}
extern const wchar_t *EX::user(int i)
{
	if(i>1) return Ex_cpp::noplace;

	static Ex_cpp::place out = L""; 

	//SOM_MAIN and SOM_RUN should have empty strings
	//static bool one_off = false; if(one_off++) return out; //???
	if(!Ex_cpp::user_placed) Ex_cpp::user_placed = 1;
	else return !i&&Ex_cpp::user_placed==1?L"":out;

	const wchar_t *user = Sompaste->get(L"USER");
	
	if(*user) Sompaste->place(0,out,user,'?'); 
	
	if(!*out) wcscpy(out,EX::cd());		

	DWORD cdrom = GetFileAttributesW(out);

	//NOTE: SHCreateDirectory refuses / but this case should
	//be clean (Sompaste->place() "canonocalizes" its paths)
	if(cdrom&FILE_ATTRIBUTE_READONLY
	||~cdrom&FILE_ATTRIBUTE_DIRECTORY)
	if(cdrom!=INVALID_FILE_ATTRIBUTES
	||SHCreateDirectory(0,out)!=ERROR_SUCCESS) //NEW
	{
		const wchar_t *game = Sompaste->get(L"GAME");

		if(!*game) game = Sompaste->get(L"TITLE"); //fallback

		swprintf_s(out,L"%ls\\%ls",Sompaste->get(L"PLAYER"),game);

		Sompaste->place(0,out,out,'?');
	}

	if(wcsicmp(out,EX::cd())) Ex_cpp::user_placed = 2; //2023

	return out;
}
extern const wchar_t *EX::lang(int i)
{	
	if(i<(int)Ex_cpp::langs.size()) return Ex_cpp::langs[i];

	if(Ex_cpp::langs.size()) return Ex_cpp::noplace;
	
	Ex_cpp::place place = L"";		

	if(*EX::user(0))
	{
		swprintf_s(place,L"%ls\\lang",EX::user(0));
		if(PathIsDirectoryW(place)) Ex_cpp::langs.push_back(place);
	}
	swprintf_s(place,L"%ls\\lang",EX::cd());
	if(PathIsDirectoryW(place)) Ex_cpp::langs.push_back(place);

	const wchar_t *script = Sompaste->get(L"SCRIPT");

	if(*script) 
	for(int j=0;j=Sompaste->place(0,place,script,'?');script+=j)
	{
		if(*place) if(PathIsDirectoryW(place))
		{
			wcscat_s(place,L"\\lang"); 
			if(PathIsDirectoryW(place)) Ex_cpp::langs.push_back(place);
		}
		else Ex_cpp::langs.push_back(place);
	}

	if(Ex_cpp::langs.empty()) 
	Ex_cpp::langs.push_back(Ex_cpp::noplace); //token place
	return EX::lang(i);
}									 
extern const wchar_t *EX::font(int i)
{	
	if(i<(int)Ex_cpp::fonts.size()) return Ex_cpp::fonts[i];

	if(Ex_cpp::fonts.size()) return Ex_cpp::noplace;

	Ex_cpp::place place = L"";		
			 	
	if(*EX::user(0))
	{
		swprintf_s(place,L"%ls\\font",EX::user(0));
		if(PathIsDirectoryW(place)) Ex_cpp::fonts.push_back(place);
	}
	swprintf_s(place,L"%ls\\font",EX::cd());
	if(PathIsDirectoryW(place)) Ex_cpp::fonts.push_back(place);
	
	const wchar_t *font = Sompaste->get(L"FONT");

	if(*font)	
	for(int j=0;j=Sompaste->place(0,place,font,'?');font+=j)
	{
		if(*place) Ex_cpp::fonts.push_back(place);
	}
	
	const wchar_t *script = Sompaste->get(L"SCRIPT");

	if(*script) 
	for(int j=0;j=Sompaste->place(0,place,script,'?');script+=j)
	{
		if(*place) if(PathIsDirectoryW(place))
		{
			wcscat_s(place,L"\\font"); 
			if(PathIsDirectoryW(place)) Ex_cpp::fonts.push_back(place);
		}
		else Ex_cpp::fonts.push_back(place);
	}

	if(Ex_cpp::fonts.empty()) 
	Ex_cpp::fonts.push_back(Ex_cpp::noplace); //token place
	return EX::font(i);
}
extern const wchar_t *EX::text(int i)
{	
	if(i<(int)Ex_cpp::texts.size()) return Ex_cpp::texts[i];

	if(Ex_cpp::texts.size()) return Ex_cpp::noplace;
	
	Ex_cpp::place place = L"";	
		
	const wchar_t *text = Sompaste->get(L"TEXT"); 

	if(*text)
	for(int j=0;j=Sompaste->place(0,place,text,'?');text+=j)
	{
		if(*place) Ex_cpp::texts.push_back(place);
	}

	if(Ex_cpp::texts.empty()) 
	Ex_cpp::texts.push_back(Ex_cpp::noplace); //token place
	return EX::text(i);		
}
extern const wchar_t *EX::ini(int i)
{	
	if(i<(int)Ex_cpp::inies.size()) return Ex_cpp::inies[i];

	if(Ex_cpp::inies.size()) return Ex_cpp::noplace;
	
	Ex_cpp::place place = L"";	
		
	const wchar_t *ex = Sompaste->get(L"EX"); 

	if(*ex)
	for(int j=0;j=Sompaste->place(0,place,ex,'?');ex+=j)
	{
		if(*place) Ex_cpp::inies.push_back(place);

		//Reminder: PathIsPrefix seems to work for this
		if(PathIsPrefixW(EX::cd(),place)&&*EX::user(0)) 
		{
			swprintf_s(place,L"%ls\\%ls",EX::user(0),Ex_cpp::inies.back()+wcslen(EX::cd())+1);

			if(PathFileExistsW(place)) Ex_cpp::inies.back() = place;
		}
	}

	if(Ex_cpp::inies.empty()) 
	Ex_cpp::inies.push_back(Ex_cpp::noplace); //token place
	return EX::ini(i);		
} 
extern const wchar_t *EX::data(int i)
{
	//data is not actually part of Ex
	//(see som.tool.h and som.game.h)

	static std::deque<Ex_cpp::deplace> out;

	if(i<(int)out.size()) return out[i];

	if(!out.empty()) return Ex_cpp::noplace;
	
	Ex_cpp::place place;		

	if(*EX::user(0))
	{
		swprintf_s(place,L"%ls\\data",EX::user(0));
		if(PathIsDirectoryW(place)) out.push_back(place);
	}
	swprintf_s(place,L"%ls\\data",EX::cd());
	if(PathIsDirectoryW(place)) out.push_back(place);

	const wchar_t *data = Sompaste->get(L"DATA");

	if(*data) 
	for(int j=0;j=Sompaste->place(0,place,data,'?');data+=j)
	{
		if(!*place) continue;

		int k; for(k=out.size();k-->0;) //2022
		{
			if(out[k]==place) break;
		}
		if(k==-1) out.push_back(place);
	}

	if(out.empty()) 
	out.push_back(Ex_cpp::noplace); //token place
	return EX::data(i);
}

extern bool EX::ok_generic_failure(const wchar_t *fstring,...)
{	
	va_list va; va_start(va,fstring); 
	
	wchar_t cap[64] = L""; _vsnwprintf_s(cap,_TRUNCATE,fstring,va); va_end(va);

	wchar_t *msg = 
	L"We've passed the point of no return. This was not supposed to happen. \n"
	L"If you are reading this. Please tell someone so that in the future it "
	L"may be possible to save your work.";

	//2021: http://www.swordofmoonlight.net/bbs2/index.php?topic=320.0
	EXLOG_ERROR(0) << "EX::ok_generic_failure: \n" << msg << "\n";

#ifdef _DEBUG

	return IDOK==MessageBoxW(0,msg,cap,MB_OKCANCEL|MB_ICONERROR);

#else

	MessageBoxW(0,msg,cap,MB_OK|MB_ICONERROR); return true;

#endif
}

extern void EX::numbers_mismatch(bool many, const wchar_t *expression)
{
	EX::ok_generic_failure
	(L"Too %s numbers in series (%s)",many?L"many":L"few",expression);
}

extern void Ex_log_terminating();
extern void EX::is_needed_to_shutdown_immediately(int code, const char *caller)
{
	//2021: http://www.swordofmoonlight.net/bbs2/index.php?topic=320.0
	EXLOG_ERROR(0) << "EX::is_needed_to_shutdown_immediately (" << code << ")\n";
	if(caller) EXLOG_ERROR(-1) << "\n(caller was " << caller << "\n";

	//2021: ensure temporary files are deleted?
	//NOTE: I don't think ExitProcess requires this, but
	//I'm not 100% certain. I don't know if it processes
	//atexit or destructors but it likely closes handles
	Ex_log_terminating();

	ExitProcess(0); //see if this won't do it

	/*
	EX::abandoning_cursor();
		
	EX::cleaning_up_Detours();

	EXLOG_HELLO(0) << "SomEx.dll: Shutting down immediately\n";	
	EXLOG_HELLO(0) << "Have a nice day...\n";
	
	//todo: see if ExitProcess(code) won't do the trick

	EX::detached = true; //signal to static destructors

	exit(code); //does not call DllMain
	*/
}
