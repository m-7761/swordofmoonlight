	
#ifndef SOM_TITLE_INCLUDED
#define SOM_TITLE_INCLUDED

#include "EXML.h"

namespace SOM
{	
	extern bool japanese(); 

	namespace MO //SOM_MAIN
	{
		struct memoryfilebase 
		{
			const DWORD mobase;						
			HWND clientext, server;			
			FILETIME writetofiletime;
			WCHAR writetofilename[MAX_PATH];
			HWND pastespecial_2ED_clipboard;
			DWORD megabytes; BOOL closedbyserver;
			DWORD numberofalterationsinceopening;
			memoryfilebase():mobase(sizeof(*this))
			{memset((void*)(&mobase+1),0,mobase-sizeof(mobase));}
		};
		struct memoryfilename //Open/CreateFileMapping
		{
			wchar_t name[7+MAX_PATH];			
			operator const wchar_t*(){ return *name?name:0; }
			memoryfilename(const wchar_t *mo)
			{
				//todo: figure out if Global\\ is even possible
				//(using OpenThreadToken/AdjustTokenPrivileges)

				*name = '\0';
				if(!mo||!*mo) return; //wcscpy(name,L"Global\\");
				int i = wcslen(name);
				for(;*mo;i++,mo++) name[i] = *mo=='\\'?'/':*mo; 
				name[i] = '\0';
			}			
		};
		extern memoryfilebase *view;
	}

	//NEW: for games gettext calls upon exml_text_component
	extern const char *gettext(const char *id, int ctxt=0); 
	extern const char *transl8(const char *ja, const char *en);
	inline const wchar_t *translate(const char *ja, const char *en, size_t *sz=0)
	{		
		const char *l10n = SOM::transl8(ja,en); 		
		const wchar_t *out = L""; int l10n_s = strlen(l10n);
		size_t out_s = EX::Convert(65001,&l10n,&l10n_s,&out);
		if(sz) *sz = out_s; return out;
	}		
	template<int N>
	inline size_t translate(wchar_t (&out)[N], const char *ja, const char *en)
	{
		size_t sz = 0; const wchar_t *x = SOM::translate(ja,en,&sz); 
		wmemcpy_s(out,N,x,sz); out[sz] = '\0'; return sz;
	}

	//REMOVE ME?
	//just doing <fps/> in the title bar for testing DirectX 7 mode
	extern const wchar_t *lex(const wchar_t *in, wchar_t *out=0, size_t out_s=0);

	template<int N>
	const char *exml_attribute(EXML::Attribs at, EXML::Attribuf<N> &ab)
	{
		while(EXML::tag(ab)) if(ab[1].tag==EXML::Tags::exml)
		for(size_t i=1;ab[i];i++) if(ab[i].key==at) return ab[i].val; 
		return 0;
	}		
	template<int N> const char *exml_text_component(EXML::Attribuf<N> &ab)
	{
		for(EXML::Tags tag;tag=EXML::tag(ab);) if(tag==EXML::Tags::exml)
		{
			while(ab->tag&&(tag=EXML::tag(ab)));
			if(tag==EXML::Tags::exml) EXML::tag(ab); else assert(0);
			if(tag==EXML::Tags::exml) return ab->val;
		}
		return 0;
	}	
	
	extern int title_pixels;
	extern int title(void *procA, const char* &txt, int &len); 
	extern const char *title(int msgctxt=0, const char *id=0); 
	
	extern const wchar_t *subtitle(); //get current subtitle	
}

#endif //SOM_TITLE_INCLUDED