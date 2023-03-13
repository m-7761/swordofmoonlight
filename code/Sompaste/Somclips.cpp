
#include "Sompaste.pch.h"

//#include <ole2.h>
#include <richedit.h>
#include <richole.h> //IRichEditOle

namespace Somclips_cpp
{	
	struct fe
	{
		enum
		{			
		bitmap = 0,	
		unicodetext,	
		hdrop,			
		intotal, invalid = -1 
		};
	};

	//nonvirtual inheritance
	#define SOMCLIPS_UNKNOWN \
		ULONG refs;\
		void unknown(int r){refs=r;}\
		STDMETHOD_(ULONG,AddRef)(void)\
		{return InterlockedIncrement(&refs);}\
		STDMETHOD_(ULONG,Release)(void)\
		{LONG out = InterlockedDecrement(&refs);\
		if(out==0) delete this;	return out;}   	

	//CTinyDataObject	
	//http://blogs.msdn.com/b/oldnewthing/archive/2008/03/11/8080077.aspx	
	//"Our mission for today is to create the tiniest data object possible."
	class dataobject : public IDataObject
	{
	public: SOMCLIPS_UNKNOWN
				
		FORMATETC fes[fe::intotal]; 
			
		int festack[fe::intotal], datadir;
	
		dataobject() : datadir(DATADIR_GET)
		{				
			memset(fes,0x00,sizeof(fes)); unknown(1); //refs
			
			memset(festack,0xFF,sizeof(festack)); //FF: fe::invalid
		}
		//IUnknown		
		STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj)
		{
			IUnknown *punk = 0;
			if(riid==IID_IUnknown)
			{
				punk = static_cast<IUnknown*>(this);
			}
			else if(riid==IID_IDataObject) 
			{
				punk = static_cast<IDataObject*>(this);
			}
			if(*ppvObj=punk)
			{
				punk->AddRef();	return S_OK;
			} 
			return E_NOINTERFACE;
		}
		//IDataObject
		STDMETHOD(GetData)(FORMATETC *pfe, STGMEDIUM *pmed)
		{
			memset(pmed,0x00,sizeof(*pmed));

			switch(GetDataIndex(pfe)) 
			{
			case fe::bitmap:		
			return bitmapGetData(pfe,pmed);
			case fe::unicodetext:	
			return unicodetextGetData(pfe,pmed);
			case fe::hdrop:			
			return hdropGetData(pfe,pmed);
			}
			return DV_E_FORMATETC;
		}
		STDMETHOD(GetDataHere)(FORMATETC *pfe, STGMEDIUM *pmed)
		{
			return E_NOTIMPL;
		}
		STDMETHOD(QueryGetData)(FORMATETC *pfe)
		{
			return GetDataIndex(pfe)==fe::invalid?S_FALSE:S_OK;
		}
		STDMETHOD(GetCanonicalFormatEtc)(FORMATETC *pfeIn, FORMATETC *pfeOut)
		{
			*pfeOut = *pfeIn; pfeOut->ptd = 0; return DATA_S_SAMEFORMATETC;
		}
		STDMETHOD(SetData)(FORMATETC *pfe, STGMEDIUM *pmed, BOOL fRelease)
		{
			HRESULT out = DV_E_FORMATETC;
			switch(GetDataIndex(pfe)) 
			{
			case fe::bitmap:		
			out = bitmapSetData(pfe,pmed); break;
			case fe::unicodetext:	
			out = unicodetextSetData(pfe,pmed); break;
			case fe::hdrop:			
			out = hdropSetData(pfe,pmed); break;
			}
			if(fRelease) ReleaseStgMedium(pmed); assert(!fRelease); //!!
			return out;
		}
		STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC *ppefe)
		{
			if(dwDirection&datadir) //bidirectional
			{
				//unimplemented: will fes be one or both?
				assert(datadir!=DATADIR_GET|DATADIR_SET); 

				UINT packed = 0;
				FORMATETC fespacked[fe::intotal];
				for(int i=0;i<fe::intotal;i++) if(festack[i]!=fe::invalid)
				{
					fespacked[packed++] = fes[festack[i]];
				}
				return SHCreateStdEnumFmtEtc(packed,fespacked,ppefe);
			}
			*ppefe = 0;	return E_NOTIMPL;
		}
		STDMETHOD(DAdvise)(FORMATETC *pfe, DWORD grfAdv, IAdviseSink *pAdvSink, DWORD *pdwConnection)
		{
			return OLE_E_ADVISENOTSUPPORTED;
		}
		STDMETHOD(DUnadvise)(DWORD dwConnection)
		{
			return OLE_E_ADVISENOTSUPPORTED;
		}
		STDMETHOD(EnumDAdvise)(LPENUMSTATDATA *ppefe)
		{
			return OLE_E_ADVISENOTSUPPORTED;
		}

		void addformat(int i, UINT cf,
		//http://msdn.microsoft.com/en-us/library/windows/desktop/ms691227%28v=vs.85%29.aspx
		DWORD/*TYMED*/ tymed=TYMED_HGLOBAL, LONG lindex=-1,
		DWORD dwAspect=DVASPECT_CONTENT,
		DVTARGETDEVICE *ptd=0)
		{
			assert(cf);
			if(!fes[i].cfFormat)		
			for(int j=0;j<fe::intotal;j++)
			{
				if(festack[j]==fe::invalid) 
				{				
					festack[j] = i; break;
				}
			}
			fes[i].cfFormat = (CLIPFORMAT)cf;
			fes[i].tymed    = tymed;
			fes[i].lindex   = lindex;
			fes[i].dwAspect = dwAspect;
			fes[i].ptd      = ptd;
		}		
		bool forward(IDataObject *in)
		{
			bool out = false;
			if(datadir&DATADIR_SET)
			{
				STGMEDIUM sm;
				for(int i=0;festack[i]!=fe::invalid;i++)				
				{
					FORMATETC paranoia = fes[festack[i]];
					if(S_OK==in->GetData(&paranoia,&sm))
					{
						//assert(paranoia==fes[festack[i]]);
						assert(!memcmp(&paranoia,fes+festack[i],sizeof(paranoia)));
						out = S_OK==SetData(&paranoia,&sm,0);
						ReleaseStgMedium(&sm);
						if(out)	break;				
					}
				}
			}
			else assert(0);
			return out;
		}

		//protected
		int GetDataIndex(const FORMATETC *pfe)
		{
			for(int i=0;i<fe::intotal;i++) 			
			if(pfe->cfFormat==fes[i].cfFormat
			  &&(pfe->tymed&fes[i].tymed)
			  &&pfe->dwAspect==fes[i].dwAspect 
			  &&pfe->lindex==fes[i].lindex)
			return i;			
			return fe::invalid;
		}
		//scheduled obsolete
		static void *CreateHGlobalFromBlob
		(SIZE_T cbData, HGLOBAL *phglob, const void *pvData=0, SIZE_T cbCopy=0)
		{
			void *out = 0;

			HGLOBAL hglob = GlobalAlloc(GMEM_MOVEABLE,cbData);

			if(hglob) 
			{
				out = GlobalLock(hglob);

				if(out) 
				{
					if(pvData)
					{
						CopyMemory(out,pvData,cbCopy?cbCopy:cbData);

						if(!cbCopy) GlobalUnlock(hglob);
					}
				} 
				else 
				{
					GlobalFree(hglob); hglob = 0;
				}
			}

			*phglob = hglob;

			return out;
		}		
		virtual HRESULT bitmapGetData(FORMATETC*,STGMEDIUM*)
		{
			return DV_E_FORMATETC;
		}
		virtual HRESULT unicodetextGetData(FORMATETC*,STGMEDIUM*)
		{
			return DV_E_FORMATETC;
		}
		virtual HRESULT hdropGetData(FORMATETC*,STGMEDIUM*)
		{
			return DV_E_FORMATETC;
		}
		virtual HRESULT bitmapSetData(FORMATETC*,STGMEDIUM*)
		{
			return DV_E_FORMATETC;
		}
		virtual HRESULT unicodetextSetData(FORMATETC*,STGMEDIUM*)
		{
			return DV_E_FORMATETC;
		}
		virtual HRESULT hdropSetData(FORMATETC*,STGMEDIUM*)
		{
			return DV_E_FORMATETC;
		}
	};
	//pack data into IDataObject argument
	//
	//REMINDER: OleCreateStaticFromData calls ReleaseStageMedium
	//on pUnkForRelease.
	class dataadapter : public dataobject
	{
	public: STGMEDIUM medium;

		dataadapter()
		{
			memset(&medium,0x00,sizeof(medium));

			//REMINDER: OleCreateStaticFromData calls ReleaseStageMedium
			//on pUnkForRelease.

			medium.pUnkForRelease = this;		
		}
		dataadapter(int f, int cf, const STGMEDIUM &cp)
		{
			addformat(f,cf,cp.tymed);
			medium = cp; medium.pUnkForRelease = this;			
		}
		//IDataObject
		STDMETHOD(GetData)(FORMATETC *pformatetc, STGMEDIUM *pmedium) 
		{
			if(pformatetc->cfFormat!=fes[festack[0]].cfFormat)
			return DV_E_FORMATETC;
			*pmedium = medium;
			pmedium->pUnkForRelease = this;
			return S_OK;
		}		
		STDMETHOD(SetData)(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL release) 
		{	
			return DV_E_FORMATETC;
		}
	};
	class bitmapadapter : public dataadapter
	{
	public: 

		bitmapadapter(HBITMAP hBitmap) 
		{
			addformat(fe::bitmap,CF_BITMAP,TYMED_GDI);
			medium.tymed = TYMED_GDI;
			medium.hBitmap = hBitmap;
		}
	};
	class dropsource : public IDropSource 
	{
	public: SOMCLIPS_UNKNOWN

		HWND recipient;

		dropsource():recipient(0){ unknown(1); } //refs	  
		//IUnknown		
		STDMETHOD(QueryInterface)(REFIID riid, void **ppv)
		{
			IUnknown *punk = 0;
			if(riid==IID_IUnknown) 
			{
				punk = static_cast<IUnknown*>(this);
			} 
			else if (riid==IID_IDropSource) 
			{
				punk = static_cast<IDropSource*>(this);
			}
			if(*ppv=punk)
			{
				punk->AddRef();	return S_OK;
			}
			return E_NOINTERFACE;
		}		  
		//IDropSource
		STDMETHOD(QueryContinueDrag)(BOOL fEscapePressed, DWORD grfKeyState)
		{
			if(fEscapePressed
			  ||grfKeyState&MK_RBUTTON) return DRAGDROP_S_CANCEL;
			if(!(grfKeyState&MK_LBUTTON))
			{
				POINT pt; if(GetCursorPos(&pt))
				recipient = WindowFromPoint(pt); return DRAGDROP_S_DROP;		
			}
			return S_OK;
		}
		STDMETHOD(GiveFeedback)(DWORD dwEffect)
		{
			return DRAGDROP_S_USEDEFAULTCURSORS;
		}		
	};

	//Somtransfer
	//(here for MSVC2005 visual debugger)
	class xferobject : public dataobject
	{
	public: void *data; size_t data_s; 
			
		const HWND *wins; size_t wins_s;

		int parts[4]; bool noconv; HWND recipients;

		inline int operator[](int i){ return parts[i]; }

		enum{ cut=1, copy, paste, text, list, clip, file, Windows };

		xferobject(const char *op, const HWND *w, int w_s, void *d, size_t d_s)
		:
		wins(w), wins_s(w_s), data(d), data_s(d_s), recipients(0)
		{
			const char *imp, *app, *obj; //*op; 
			
			for(imp=op;*imp&&*imp!='.'&&*imp!='!';imp++);

			app = imp; while(--app>op&&*app!=':'&&*app!=' ');

			obj = app; if(*obj==':') while(--obj>op&&*obj!=' ');

			/*this is terribly unsophisticated*/ if(obj<op) return;

			parts[3] = -1;
			noconv = *imp=='!';
			if(*imp=='.'||noconv) imp++; 
			switch(imp[0])
			{
			case 'W': parts[3] = Windows;
				
				if(!strcmp(imp,"Windows")) break;

			default: return;
			}						

			parts[2] = -1;
			if(*app==':') app++;
			switch(app[0])
			{
			case 't': parts[2] = text;

				if(imp-app==5&&!strncmp(app,"text.",5)) break;

			case 'f': parts[2] = file;
				
				if(imp-app==5&&!strncmp(app,"file.",5)) break;

			case 'c': parts[2] = clip;
				
				if(imp-app==5&&!strncmp(app,"clip.",5)) break;
	
			default: return;
			}			

			parts[1] = -1;
			if(*obj==' ') obj++;			
			if(obj!=app) switch(obj[0])
			{
			case 't': parts[1] = text;
				
				if(app-obj==5&&!strncmp(obj,"text:",5)) break;

			case 'l': parts[1] = list;
				
				if(app-obj==5&&!strncmp(obj,"list:",5)) break;

			case 'c': parts[1] = clip;
				
				if(app-obj==5&&!strncmp(obj,"clip:",5)) break;

			default: return;
			}
			else parts[1] = parts[2];

			parts[0] = -1;
			if(op!=obj)	switch(toupper(op[0]))
			{				
			case 'X': parts[0] = cut;   if(op[1]==' ') break;
			case 'V': parts[0] = paste; if(op[1]==' ') break;		
			case 'C': parts[0] = copy;  if(op[1]==' ') break;				

				if(obj-op==5&&!strnicmp(op,"copy ",5)) break;

				parts[0] = cut;

				if(obj-op==4&&!strnicmp(op,"cut ",4)) break;

			case 'P': parts[0] = paste; 
					
				if(obj-op==6&&!strnicmp(op,"paste ",6)) break;

			default: return;
			}
			else parts[0] = 0; //drag & drop

			for(int i=0;i<4;i++) assert(parts[i]!=-1);

			if(parts[0]==paste) datadir = DATADIR_SET;

			if(parts[3]==Windows) switch(parts[2])
			{				
			case clip:
				if(parts[1]==clip)				
				addformat(fe::bitmap,CF_BITMAP); 
				else assert(0);
				break;			
			case text:
				if(parts[1]==text)				
				addformat(fe::unicodetext,CF_UNICODETEXT); 
				else assert(0);
				break;
			case file:
				if(parts[1]==text||parts[1]==list)	
				addformat(fe::hdrop,CF_HDROP);
				else assert(0);
				break;
			default: assert(0); 
				return;
			}
		}

		//dataobject (protected)
		virtual HRESULT bitmapGetData(FORMATETC *pfe,STGMEDIUM *pmed)
		{
			return DV_E_FORMATETC; //unimplemented
		}
		virtual HRESULT unicodetextGetData(FORMATETC *pfe,STGMEDIUM *pmed)
		{
			if(!(pfe->tymed&TYMED_HGLOBAL)) return DV_E_FORMATETC; 
				
			pmed->tymed = TYMED_HGLOBAL;

			if(parts[1]==text)
			{
				WCHAR *lock = (WCHAR*)
				CreateHGlobalFromBlob(data_s+sizeof(WCHAR),&pmed->hGlobal,data,data_s);
				if(!lock) return DV_E_FORMATETC;
				lock[data_s/sizeof(WCHAR)] = '\0';
				GlobalUnlock(pmed->hGlobal);					
				return S_OK;
			}
			else assert(0);

			return DV_E_FORMATETC;
		}
		virtual HRESULT hdropGetData(FORMATETC *pfe,STGMEDIUM *pmed)
		{
			//todo: try TYMED_FILE?
			if(!(pfe->tymed&TYMED_HGLOBAL)) return DV_E_FORMATETC; 

			pmed->tymed = TYMED_HGLOBAL;			
			//There is an MS bug in here
			//http://stackoverflow.com/questions/23757424/windows-disregards-dropfiles-fwide-member-any-ideas
			//NOTE: sizeof(DROPFILES) is aligned too (but if you had to think about that its already to late)
			//size_t files = sizeof(WCHAR)*(sizeof(DROPFILES)/sizeof(WCHAR)+1);						
			size_t files = sizeof(DROPFILES);
			//assert(files>=sizeof(DROPFILES));
			size_t size = files+data_s+2*sizeof(WCHAR);
			void *lock = CreateHGlobalFromBlob(size,&pmed->hGlobal);
			if(!lock) return DV_E_FORMATETC; 
			/*////POINT OF NO RETURN////*/
			DROPFILES *df = (DROPFILES*)lock;
			df->fWide = 1;
			df->pFiles = files;
			df->fNC = 1; GetCursorPos(&df->pt);
			WCHAR *p = (WCHAR*)(((char*)df)+files);			
			size_t zt = data_s/sizeof(WCHAR);
			if(parts[1]==list)
			{
				size_t seps_s = 0;
				const WCHAR *seps = (WCHAR*)data;				
				while(seps_s<zt&&seps[seps_s++]); 
				const WCHAR *q = seps+seps_s, *d = seps+zt;
				for(WCHAR *pp=p;q<d;*p++=*q++)				
				for(size_t i=0;i<seps_s;i++) if(*q==seps[i])
				{
					if(p!=pp) *p++ = '\0'; pp = p;					

					if(++q<d) i = 0; else goto breakout; //!

				}breakout:;
			}
			else if(parts[1]==text)
			{
				memcpy(p,data,data_s); p+=zt; //!
			}
			else assert(0);
			*p++ = '\0'; *p++ = '\0';	 			
			GlobalUnlock(pmed->hGlobal);
			return S_OK;
		}		
		virtual HRESULT hdropSetData(FORMATETC *pfe,STGMEDIUM *pmed)
		{				
			bool implemented = !data&&wins_s==2;
			assert(implemented);
			if(!implemented) return DV_E_FORMATETC;

			recipients = wins[1];
			if(WS_EX_ACCEPTFILES&GetWindowLong(recipients,GWL_EXSTYLE))
			{
				//can't rely on DLGPROCs to return 1 via DWL_MSGRESULT
				SendMessage(recipients,WM_DROPFILES,(WPARAM)pmed->hGlobal,0);
				return S_OK;
			}			
			
			IUnknown *punk = //LOCAL TO dest'S ADDRESS SPACE!!
			(IUnknown*)GetPropW(recipients,L"OleDropTargetInterface"); 
			//OleDropTargetInterface is 0 for WS_EX_ACCEPTFILES
			if(!punk) return DV_E_FORMATETC;			 

			assert(!"untested"); 
			
			IDropTarget *pdt = 0; 
			if(S_OK!=punk->QueryInterface(IID_IDropTarget,(void**)&pdt))
			return DV_E_FORMATETC;

			dataadapter da(fe::hdrop,CF_HDROP,*pmed);
			  
			RECT wr = {0,0}; GetWindowRect(recipients,&wr);
			DWORD eff;			 
			HRESULT out = pdt->Drop(&da,0,*(POINTL*)&wr,&eff);

			pdt->Release();

			return out;			
		}
	};
}

extern HWND Somtransfer(const char *op, const HWND *wins, int count, void *data, size_t data_s)
{				
	HWND out = 0;

	Somclips_cpp::xferobject xo(op,wins,count,data,data_s);
	
	switch(xo[0])
	{
	case 0:
	{
		#ifdef _DEBUG/*
		//MSVC2005: impossible to debug inside DoDragDrop
		FORMATETC test = xo.fes[Somclips_cpp::fe::hdrop];
		STGMEDIUM test2;
		xo.hdropGetData(&test,&test2);*/
		#endif

		Somclips_cpp::dropsource ds;	
		DWORD des = DROPEFFECT_COPY, did = 0;
		bool done = DoDragDrop(&xo,&ds,des,&did)==DRAGDROP_S_DROP;		
		assert(ds.refs==1);
		if(out=(HWND)done)
		if(ds.recipient) out = ds.recipient;		
		break;
	}
	case xo.copy: 
	
		if(OleSetClipboard(&xo)==S_OK)
		{
			out = (HWND)1; OleFlushClipboard(); //!
		}			
		break;

	case xo.paste: 
	{
		IDataObject *got = 0;
		if(OleGetClipboard(&got)!=S_OK) break;
		bool done = xo.forward(got);
		got->Release();
		if(out=(HWND)done)
		if(xo.recipients) out = xo.recipients;
		break;
	}
	case xo.cut: //unimplemented

	default: assert(0);
	}
	assert(xo.refs==1);
	return out;
}

namespace Somclips_cpp //RTF insertion routines
{			
	static bool insert(IRichEditOle *pRichEditOle, 
	IOleClientSite *pOleClientSite, IStorage *pStorage, IOleObject *pOleObject, HRESULT *phr=0)
	{
		HRESULT hr; 
		
		REOBJECT reobject;
		memset(&reobject,0x00,sizeof(reobject));
		reobject.cbStruct = sizeof(reobject);
		
		CLSID clsid;
		hr = pOleObject->GetUserClassID(&clsid);

		if(hr!=S_OK) return false; //AfxThrowOleException(hr);

		reobject.clsid = clsid;		
		reobject.cp = REO_CP_SELECTION;
		reobject.dvaspect = DVASPECT_CONTENT;
		reobject.poleobj = pOleObject;
		reobject.polesite = pOleClientSite;
		reobject.pstg = pStorage;

		//Allow images to be resized
		//Scratch that: resize algorithm is fugly as is
		//REO_BELOWBASELINE fits images tighter to EN_REQUESTRESIZE
		reobject.dwFlags = REO_BELOWBASELINE; //REO_RESIZABLE

		hr = pRichEditOle->InsertObject(&reobject);

		if(phr) *phr = hr;

		return hr==S_OK;
	}

	static bool insert(IRichEditOle *pRichEditOle, HBITMAP hBitmap, HRESULT *phr=0)
	{		
		HRESULT hr;

		bitmapadapter ba(hBitmap); 
		//2017: This wasn't set before. OleCreateStaticFromData calls ReleaseStageMedium
		//on ba.medium.pUnkForRelease; which is ba itself.
		ba.AddRef();

		IOleClientSite *pOleClientSite;	
		pRichEditOle->GetClientSite(&pOleClientSite);

		LPLOCKBYTES lpLockBytes = 0;
		hr = CreateILockBytesOnHGlobal(0,TRUE,&lpLockBytes);
		if(hr!=S_OK) return false; //AfxThrowOleException(hr);		
		assert(lpLockBytes);
		
		IStorage *pStorage;	hr = StgCreateDocfileOnILockBytes
		(lpLockBytes,STGM_SHARE_EXCLUSIVE|STGM_CREATE|STGM_READWRITE,0,&pStorage);
		if(hr!=S_OK)
		{				
			if(lpLockBytes->Release()) assert(0); //VERIFY(lpLockBytes->Release());

			lpLockBytes = 0; return false; //AfxThrowOleException(hr);
		}
		assert(pStorage);

		IOleObject *pOleObject; 
		//pOleObject = ba.GetOleObject(pOleClientSite, pStorage);
		{
			//IOleObject *pOleObject;
 			SCODE hr = OleCreateStaticFromData
			(&ba,IID_IOleObject,OLERENDER_FORMAT, 
			&ba.fes[fe::bitmap],pOleClientSite,pStorage,(void**)&pOleObject);

 			if(hr!=S_OK) return false; //AfxThrowOleException(hr);
 			
			//return pOleObject;
		}
		OleSetContainedObject(pOleObject,TRUE);
		
		if(hr==S_OK) insert(pRichEditOle,pOleClientSite,pStorage,pOleObject,&hr);

		pOleObject->Release();
		pOleClientSite->Release();
		pStorage->Release();

		if(phr) *phr = hr;

		assert(ba.refs==1);
		assert(hr==S_OK);
		return hr==S_OK;
	} 
	static bool insert(IRichEditOle *pRichEditOle, const wchar_t lpszFileName[MAX_PATH], HRESULT *phr=0)
	{
		HRESULT hr; 

		if(PathIsDirectoryW(lpszFileName)) return false; 
		
		wchar_t temp[MAX_PATH] = L"", *title = 
		PathFindFileNameW(wcscpy(temp,lpszFileName));

		if(title>temp) temp[title-temp-1] = '\0'; else return false;

		IShellFolder *pDesktop = 0, *pFolder = 0;		 

		SHGetDesktopFolder(&pDesktop); if(!pDesktop) return false;		
		
		LPITEMIDLIST pIidlist = 0; ULONG attribs = 0;
		pDesktop->ParseDisplayName(0,0,temp,0,&pIidlist,&attribs);	
		pDesktop->BindToObject(pIidlist,0,IID_IShellFolder,(void**)&pFolder);
		CoTaskMemFree(pIidlist);

		IExtractImage *pExtract = 0;

		if(pFolder)
		{
			pIidlist = 0; hr = 
			pFolder->ParseDisplayName(0,0,title,0,&pIidlist,0);			
			pFolder->GetUIObjectOf(0,1,(LPCITEMIDLIST*)&pIidlist,IID_IExtractImage,0,(void**)&pExtract); 
			CoTaskMemFree(pIidlist);

			if(pExtract)
			{	
				SIZE sz = {512,512}; 
				wchar_t blah[MAX_PATH]; DWORD meh; 
				DWORD fio = IEIFLAG_SCREEN|IEIFLAG_QUALITY;			
				hr = pExtract->GetLocation(blah,MAX_PATH,&meh,&sz,32,&fio);
							
				if(hr==S_OK)
				{
					HBITMAP bitmap = 0;
					hr = pExtract->Extract(&bitmap);
					if(hr==S_OK) insert(pRichEditOle,bitmap,&hr);
					DeleteObject(bitmap);
				}

				pExtract->Release();
			}
			pFolder->Release();
		}
		pDesktop->Release();

		if(phr) *phr = hr;

		return hr==S_OK;
	}
}

static const int SOMCLIPS_RTF = 
RegisterClipboardFormat(CF_RTF);
static const int SOMCLIPS_RETEXTOBJ = 
RegisterClipboardFormat(CF_RETEXTOBJ);
static const int SOMCLIPS_HTML = 
RegisterClipboardFormat("HTML Format");
static int IDC_RICHEDIT = IDC_RICHEDIT20W;

//scheduled obsolete: copy/paste job
static const UINT Somclips_formats_s = 5;
static UINT Somclips_formats[Somclips_formats_s] = 
{ 
//Not really practical for pasting anyway
//http://support.microsoft.com/kb/109552/f
//	CF_OWNERDISPLAY, 
	SOMCLIPS_RETEXTOBJ,
	SOMCLIPS_RTF,	
//	SOMCLIPS_HTML,
	CF_UNICODETEXT, 
//	CF_ENHMETAFILE, 
	CF_BITMAP,
	CF_HDROP
};

SIZE Somclips_reference = {0,0};
SIZE Somclips_calibrate = {0,0};

static void Somclips_drawclipboard(HWND hwnd) 
{		
    UINT cf = GetPriorityClipboardFormat(Somclips_formats,Somclips_formats_s); 

	int i; for(i=0;i<Somclips_formats_s;i++) if(cf==Somclips_formats[i]) break;

	if(i>=Somclips_formats_s) return;

	HBITMAP clip = 0;
	HWND re = GetDlgItem(hwnd,IDC_RICHEDIT);

	if(GetFocus()==re&&cf!=CF_BITMAP) return;

	SETTEXTEX st = {ST_DEFAULT,CP_ACP}; //clean slate
	SendMessage(re,EM_SETTEXTEX,(WPARAM)&st,(LPARAM)L"");
	SendMessage(re,EM_SETZOOM,1,1);

	CHARFORMAT ch; //minimize spacing/reset font
	memset(&ch,0x00,sizeof(ch)); ch.cbSize = sizeof(ch);
	SendMessage(re,EM_SETCHARFORMAT,SCF_ALL,(LPARAM)&ch);			

	Somclips_calibrate.cx = -Somclips_reference.cx; //cursor width

	if(!SendMessage(re,EM_CANPASTE,cf,0))
	{
		assert(cf==CF_BITMAP||cf==CF_HDROP);

		if(cf==CF_BITMAP||cf==CF_HDROP)
			
		if(OpenClipboard(hwnd))
		{	
			IRichEditOle *p = 0;
			SendMessage(re,EM_GETOLEINTERFACE,0,(LPARAM)&p);

			if(cf==CF_BITMAP&&p)
			{
				HANDLE copy = GetClipboardData(CF_BITMAP); 
				//clip = (HBITMAP)OleDuplicateData(copy,CF_BITMAP,0);

				if(copy&&p) Somclips_cpp::insert(p,(HBITMAP)copy);		
			}
			if(cf==CF_HDROP&&p)
			{
				wchar_t file[MAX_PATH];

				HDROP drop = (HDROP)GetClipboardData(CF_HDROP); 

				UINT n = DragQueryFileW(drop,-1,0,0);

				for(int i=0;i<n;i++)
				{	
					UINT sz = DragQueryFileW(drop,i,file,MAX_PATH);

					if(sz&&sz<MAX_PATH)	Somclips_cpp::insert(p,file);		
				}

				//XP:causes RtlFreeHeap complaint
				//Probably the drop handle is supposed to be undisturbed
				//DragFinish(drop);
			}

			CloseClipboard(); if(p) p->Release();			
		}
	}
	else SendMessage(re,EM_PASTESPECIAL,cf,0);

	if(!clip)
	{
		//TODO: SendMessage(re,WM_PRINTCLIENT,...);
	}

	DeleteObject(clip); //will want to keep around in the future

	RedrawWindow(re,0,0,RDW_INVALIDATE|RDW_FRAME|RDW_ERASE);
	//InvalidateRect(re,0,TRUE); //hwnd
	//UpdateWindow(re); //hwnd
} 


static LRESULT CALLBACK SomclipsRichEditProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scid, DWORD_PTR) 
{
	HWND &hWndDlg = hwnd; 
	
	static bool arrow = false; //hack??

	switch(uMsg) 
    { 
	case WM_SETCURSOR: 
		
		if(arrow) return 1; break; //hack

	case WM_CONTEXTMENU: 
	{
		arrow = true;
		HCURSOR pop = SetCursor(LoadCursor(0,IDC_ARROW));

		static HMENU menu = 
		LoadMenu(Sompaste_dll(),MAKEINTRESOURCE(IDR_CLIPBOARD_MENU));

		int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

		BOOL cmd = TrackPopupMenu(GetSubMenu(menu,0),TPM_RETURNCMD,x,y,0,hWndDlg,0); 

		SendMessage(hwnd,WM_COMMAND,cmd,0);

		arrow = false; SetCursor(pop);

		return 1;

	}break;
	case WM_COMMAND: 
	{
		switch(LOWORD(wParam)) 
        { 
		case ID_ZOOM_1X: case ID_ZOOM_10X: 
		case ID_ZOOM_2X: case ID_ZOOM_11X: 
		case ID_ZOOM_3X: case ID_ZOOM_12X: 
		case ID_ZOOM_4X: case ID_ZOOM_13X:
		case ID_ZOOM_5X: case ID_ZOOM_14X:
		case ID_ZOOM_6X: case ID_ZOOM_15X:
		case ID_ZOOM_7X: case ID_ZOOM_16X:
		case ID_ZOOM_8X:
		case ID_ZOOM_9X: SendMessage(hwnd,EM_SETZOOM,LOWORD(wParam)-ID_ZOOM_1X+1,1); 
						break;		

		case ID_EPR_0: SendMessage(hwnd,EM_SETPAGEROTATE,EPR_0,0); break;
		case ID_EPR_90: SendMessage(hwnd,EM_SETPAGEROTATE,EPR_90,0); break;
		case ID_EPR_180: SendMessage(hwnd,EM_SETPAGEROTATE,EPR_180,0); break;
		case ID_EPR_270: SendMessage(hwnd,EM_SETPAGEROTATE,EPR_270,0); break;
		case ID_EPR_SE: SendMessage(hwnd,EM_SETPAGEROTATE,5,0); break; //EPR_SE

		case ID_TEXT_ROTATE:

			switch(SendMessage(hwnd,EM_GETPAGEROTATE,0,0))
			{
			default:	  SendMessage(hwnd,EM_SETPAGEROTATE,EPR_0,0);	  break;
			case EPR_0:   SendMessage(hwnd,EM_SETPAGEROTATE,EPR_90,0);  break;
			case EPR_90:  SendMessage(hwnd,EM_SETPAGEROTATE,EPR_180,0); break;
			case EPR_180: SendMessage(hwnd,EM_SETPAGEROTATE,EPR_270,0); break;
			}
			break;

		case ID_CHOOSE_BACKGROUND:

			COLORREF hack = (COLORREF)
			SOMPASTE_LIB(Choose)(L"Background",hwnd);			
			SendMessage(hwnd,EM_SETBKGNDCOLOR,0,hack);
			break;
        } 

	}break;
	case WM_NCDESTROY:
	
		RemoveWindowSubclass(hwnd,SomclipsRichEditProc,scid);
		break;
	}

	return DefSubclassProc(hwnd,uMsg,wParam,lParam);
}

static INT_PTR CALLBACK SomclipsProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{ 
	HWND &hWndDlg = hwnd;

    static HWND hwndNextViewer = 0; 
  
    switch(uMsg) 
    { 
	case WM_INITDIALOG:
	{	
		CoInitialize(0);

		DefWindowProc(hwnd,WM_SETICON,ICON_BIG,LPARAM(Somicon()));
		DefWindowProc(hwnd,WM_SETICON,ICON_SMALL,LPARAM(Somicon()));

		static HMODULE Msftedit = LoadLibrary("Msftedit.dll"); //hack

		if(!Msftedit)
		{
			static HMODULE Riched20 = LoadLibrary("Riched20.dll"); //hack

			if(!Riched20) goto close; 
		}
		else IDC_RICHEDIT = IDC_RICHEDIT50W; //hack

		const char *ctrl = IDC_RICHEDIT==IDC_RICHEDIT50W?"RICHEDIT50W":"RichEdit20W";
		HWND re = CreateWindowEx(0,ctrl,0,WS_CHILD|WS_VISIBLE|WS_VSCROLL|WS_HSCROLL|ES_MULTILINE|ES_WANTRETURN,0,0,0,0,hwnd,(HMENU)IDC_RICHEDIT,0,0);
		
		SetWindowSubclass(re,SomclipsRichEditProc,0,0);

		//testing...
		//SetClassLong(re,GCL_STYLE,CS_OWNDC|GetClassLong(re,GCL_STYLE));
		
		//REO_RESIZABLE: No good :/
		//HDC dc = GetDC(re); SetStretchBltMode(dc,HALFTONE); ReleaseDC(re,dc); 

		SendMessage(re,EM_SETEVENTMASK,0,ENM_REQUESTRESIZE);
		SendMessage(re,EM_SETMARGINS,EC_LEFTMARGIN|EC_RIGHTMARGIN,0); //paranoia

		SETTEXTEX st = {ST_DEFAULT,CP_ACP}; //calibration
		SendMessage(re,EM_SETTEXTEX,(WPARAM)&st,(LPARAM)"1");

		HWND tabs = GetDlgItem(hwnd,IDC_TABS);	  
		TCITEMW tab = { TCIF_TEXT|TCIF_STATE,0,0,L"Clip 1",1,0}; //bogus
		SendMessage(tabs,TCM_INSERTITEMW,0,(LPARAM)&tab); 

		SetWindowPos(tabs,re,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE);

		RECT client; GetClientRect(hWndDlg,&client);
		SendMessage(hWndDlg,WM_SIZE,SIZE_RESTORED,MAKELPARAM(client.right,client.bottom));

		return FALSE; //Assuming client does not want focus
	}
    case WM_SIZE: 
	{
		int w = LOWORD(lParam), h = HIWORD(lParam); 

		HWND tabs = GetDlgItem(hwnd,IDC_TABS); 
		MoveWindow(tabs,-2,-2,w+4+3,h+4,0); //3: Explorer theme

		int off = 0; //texture border/offset (left/top only)

		RECT tbot; TabCtrl_GetItemRect(tabs,0,&tbot);

		HWND re = GetDlgItem(hwnd,IDC_RICHEDIT); 
		MoveWindow(re,off,tbot.bottom+off,w-off,h-tbot.bottom-off,1);		
		break;
	}
    case WM_CHANGECBCHAIN: 

        if((HWND)wParam!=hwndNextViewer) 
		SendMessage(hwndNextViewer,uMsg,wParam,lParam); 		
		else hwndNextViewer = (HWND)lParam; 
        break; 

    case WM_DESTROY: 		
        ChangeClipboardChain(hwnd,hwndNextViewer); 
		hwndNextViewer = 0;
		CoUninitialize();
		break; 
	
    case WM_DRAWCLIPBOARD:  

        Somclips_drawclipboard(hwnd); 
        SendMessage(hwndNextViewer,uMsg,wParam,lParam); 
        break; 

	case WM_NOTIFY:
	{
		REQRESIZE *rr = (REQRESIZE*)lParam;
		if(wParam==IDC_RICHEDIT) switch(rr->nmhdr.code)
		{
		case EN_REQUESTRESIZE:
						
			HWND re = rr->nmhdr.hwndFrom; 
						
			WPARAM num; LPARAM den; 
			SendMessage(re,EM_GETZOOM,(WPARAM)&num,(LPARAM)&den);

			if(num==0) num = 1; if(den==0) den = 1; //best practice?

			if(num<den) //counter balancing...
			{
				SendMessage(re,EM_SETZOOM,1,1); break;
			}
			else if(num%den) //Ctrl+Mouse Wheel (zoom)
			{
				//Reminder: wheel seems to stop around 6x
				//Maybe a built in limit? Below code looks alright??

				float f = float(num)/den; 
				int i = f+0.5f; num = i+(f<i?-1:1);
				SendMessage(re,EM_SETZOOM,num<1?1:num>64?64:num,1); break;
			}
			
			float scale = (float)den/num;

			float x = scale*(rr->rc.right-rr->rc.left);
			float y = scale*(rr->rc.bottom-rr->rc.top);

			SIZE dims = {x+0.5,y+0.5}; 
			Somclips_reference = dims; 

			dims.cx = x+Somclips_calibrate.cx*scale+0.5;
			dims.cy = y+Somclips_calibrate.cy*scale+0.5;			

			wchar_t tmp[64]; swprintf_s(tmp,
			L"Sompaste.dll - Clipboard %d x %d",dims.cx,dims.cy);
			SetWindowTextW(hwnd,tmp);
			break;			
		}			
	        
	}break; 		
	case WM_CLOSE:
		
close: DestroyWindow(hwnd); break;

    } 

    return 0;
} 

extern HWND Somclips(SOMPASTE p, HWND owner, void *note)
{
#ifdef NDEBUG
	//return 0; //not ready for release
#endif

	static HWND out = 0; if(IsWindow(out)) return out; //testing

	out = CreateDialogW(Sompaste_dll(),MAKEINTRESOURCEW(IDD_CLIPBOARD),owner,SomclipsProc);
						
#ifdef _DEBUG

	//todo: ensure WM_CHANGECBCHAIN is setting up the clipboard 
	//chain so it is the same as returned by SetClipboardViewer below	
	assert("double check me"); 

#endif

	SetClipboardViewer(out); return out;
}