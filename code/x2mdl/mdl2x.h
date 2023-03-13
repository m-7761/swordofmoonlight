
#ifndef MDL2X_INCLUDED
#define MDL2X_INCLUDED

#include <assert.h>
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath> //msm_clamp

#define WIN32_LEAN_AND_MEAN		

#define _WIN32_WINNT 0x0500 //GetConsoleWindow

#include <windows.h> //globbing
#include <Wincon.h> //GetConsoleWindow
#include <Shlwapi.h> //PathFindFileName
#pragma comment(lib,"Shlwapi.lib")

#include <d3dx9xof.h> //ID3DXFileSaveObject 

//TODO: move to SDK
#include "rmxfguid.h"
#include "rmxftmpl.h"

#pragma comment(lib,"dxguid.lib")

#include "assimp.h"
#include "export.h" //Assimp
#include "aiPostProcess.h"
#include "aiScene.h"
#include "Euler.hpp" //Assimp include

#pragma comment(lib,"assimp.lib")
						
static const aiScene *mdl;

//Reminder: Assimp's DAE is a joke
//And it's 3DS exporter is an empty {}
static const char *x = "x"; //"x"; "dae";

namespace X
{		
	struct File;
	struct Data;
		
	size_t fill(BYTE**,BYTE*,size_t);

	size_t reset(BYTE**);

	void free(BYTE**);

	struct File
	{
		HRESULT ok; 
				
		ID3DXFile *created;

		ID3DXFileSaveObject *saving;

		Data *serial; BYTE *buffer;
		
		inline operator bool(){ return ok==S_OK; }
			
		File(const char *fname=0, const char *mode=0)
		{
			bool w = mode&&strchr(mode,'w')?true:false;
			bool t = mode&&strchr(mode,'b')?false:true; 
			bool c = mode&&strchr(mode,'c')?true:false;

			open(fname,D3DXF_FILESAVE_TOFILE,w,t,c);
		}

		File(const wchar_t *fname, const wchar_t *mode=0)
		{
			bool w = mode&&wcschr(mode,'w')?true:false;
			bool t = mode&&wcschr(mode,'b')?false:true; 
			bool c = mode&&wcschr(mode,'c')?true:false;

			open(fname,D3DXF_FILESAVE_TOWFILE,w,t,c);
		}

		~File()
		{
			flush(); //flush serial

			if(saving) saving->Release();
			if(created) created->Release();

			X::free(&buffer);
		}	

		void flush();  //doesn't save until released anyway!!

	private: //paranoia: see that file is opened only once on construction
		
		void open(const void *fname, DWORD saveops, bool write, bool text, bool compress);
	};

	struct Data
	{			
		X::File *root;
		X::Data *parent;
				
		ID3DXFileSaveData *saved;
		
		const GUID *templateid;

		char named[64];

		template<typename T>
		inline void write(const T &t)
		{
			assert(root->ok==S_OK); //debugging

			assert(!saved); if(saved) return; 

			X::fill(&root->buffer,(BYTE*)&t,sizeof(T));
		}

		template<typename T>
		inline void write(const T *t, int n=1)
		{
			assert(root->ok==S_OK); //debugging

			assert(!saved); if(saved) return;

			X::fill(&root->buffer,(BYTE*)t,sizeof(T)*n);
		}

		void flush()
		{
			if(!this) return; assert(root);

			if(saved||!root||!root->saving||root->ok!=S_OK) return;

			size_t size = X::reset(&root->buffer);

			if(parent)
			{
				root->ok = 
				parent->saved->AddDataObject(*templateid,*named?named:0,0,size,size?root->buffer:0,&saved);

				//root->buffer = 0; //debugging
			}
			else if(root->saving)
			{
				root->ok = 
				root->saving->AddDataObject(*templateid,*named?named:0,0,size,size?root->buffer:0,&saved);

				//root->buffer = 0; //debugging
			}
			else root->ok = !S_OK;

			if(root->ok!=S_OK) saved = 0; //paranoia
		}
			
		//passing by reference to va_start is no longer okay
		#define _va_name(name,named) \
		va_list va; va_start(va,name);\
		_va_name2(name,named,va); va_end(va);            

		void refer(char *name,...)
		{
			flush(); //assert(name&&*name&&saved);

			if(name&&*name&&saved)
			{
				char rname[sizeof(named)] = "";
			
				_va_name(name,rname);

				root->ok = saved->AddDataReference(rname,0);
			}
		}

		template<int N>
		static void _va_name2(char *name, char (&named)[N], va_list va)
		{
			*named = '\0'; if(!name) return;
			
			//passing by reference to va_start is no longer okay
			//va_list va; va_start(va,name);            
			int len = vsnprintf(named,sizeof(named)-1,name,va); 
			//va_end(va);

			//Blender's COLLADA exporter doesn't escape XML.
			for(int i=0;i<len;i++) 
			if(!(named[i]>='a'&&named[i]<='z'
			   ||named[i]>='A'&&named[i]<='Z'||isdigit(named[i])))
			{
				//Note: _ is the only thing that works with x2msm.exe.
				named[i] = '_';
			}
		}

		//top-level constructor
		Data(X::File &x, const GUID &rguid, char *name=0,...)
		{				
			if(x.saving) x.serial->flush(); x.serial = this;

			root = &x; parent = 0; saved = 0;
		
			templateid = &rguid; _va_name(name,named); 
		}

		//sub-level constructor
		Data(X::Data &x, const GUID &rguid, char *name=0,...)
		{
			if(x.root)
			{
				if(x.root->saving) x.root->serial->flush(); 
				
				x.root->serial = this;
			}

			root = x.root; parent = &x; saved = 0;
						
			templateid = &rguid; _va_name(name,named); 
		}

		~Data() 
		{
			flush(); if(saved) saved->Release();

			if(root&&root->serial==this) root->serial = 0;
		}
	};
}

const char x_header[] = 
"xof 0303txt 0032 "
"template Header {\
 <3D82AB43-62DA-11cf-AB39-0020AF71E433>\
 WORD major;\
 WORD minor;\
 DWORD flags;\
}";
				 
void X::File::open(const void *fname, DWORD saveops, bool write, bool text, bool compress)
{
	created = 0; saving = 0; serial = 0; buffer = 0;

	//TODO: dynamically load interface
	ok = D3DXFileCreate(&created);
	
	if(created&&ok==S_OK)
	{	
		ok = 
		created->RegisterTemplates(x_header,sizeof(x_header)-1);
		assert(ok==S_OK);

		ok = 
		created->RegisterTemplates(D3DRM_XTEMPLATES,D3DRM_XTEMPLATE_BYTES);
		assert(ok==S_OK);

		ok = 
		created->RegisterTemplates(XSKINEXP_TEMPLATES,sizeof(XSKINEXP_TEMPLATES)-1);
		assert(ok==S_OK);

//		created->RegisterTemplates(XEXTENSIONS_TEMPLATES,sizeof(XEXTENSIONS_TEMPLATES)-1);

		if(fname&&write)
		{
			DWORD f = text?D3DXF_FILEFORMAT_TEXT:D3DXF_FILEFORMAT_BINARY;

			if(compress) f|=D3DXF_FILEFORMAT_COMPRESSED;

			ok = created->CreateSaveObject(fname,saveops,f,&saving);

			if(ok!=S_OK) saving = 0;
		}
	}
	else created = 0;
}

void X::File::flush()
{
	if(serial) serial->flush();

	if(saving&&ok==S_OK) ok = saving->Save();

	assert(ok==S_OK);
}

size_t X::fill(BYTE**p, BYTE*q, size_t n)
{
	size_t *cp = *p?(size_t*)*p:0;

	while(!cp||cp[-1]+n>cp[-2])
	{
		size_t dbl = cp?cp[-2]*2:256;

		*(size_t**)p = new size_t[dbl+2];

		*(size_t*)*p = dbl*sizeof(size_t); *p+=sizeof(size_t);

		*(size_t*)*p = cp?cp[-1]:0; *p+=sizeof(size_t);
		
		if(cp) memcpy(*p,cp,cp[-1]);

		if(cp) delete ----cp;

		cp = (size_t*)*p;
	}

	memcpy(*p+cp[-1],q,n);

	return cp[-1]+=n; 
}

size_t X::reset(BYTE**p)
{
	size_t *cp = *p?(size_t*)*p:0; if(!cp) return 0;
	
	size_t out = cp[-1]; cp[-1] = 0; return out;
}

void X::free(BYTE**p)
{
	size_t *cp = *p?(size_t*)*p:0;

	if(cp) delete ----cp;

	if(p) *p = 0;
}

#ifndef _DEBUG
#define _DEBUGPOINT(x) { static int note = 0 if(!note&&++note)\
	MessageBoxA(0,"Developer Reminder: " #x,"mdl2x.exe",MB_OK|MB_ICONERROR);}
#else
#define _DEBUGPOINT(x) 
#endif

#endif //MDL2X_INCLUDED