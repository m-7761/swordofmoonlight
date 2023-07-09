
#include "directx.h" 
DX_TRANSLATION_UNIT //(C)

#include <vector>

#define DIRECT3D_VERSION 0x0700

#include "dx8.1/ddraw.h"
#include "dx8.1/d3d.h" 
#include "dx8.1/errors.h" //AMGetErrorText()
#include "dx8.1/uuids.h" //CLSID_FilterGraph
#include "dx8.1/control.h" //IMediaControl

//dx_dshow_DMO_wrapper
#include "Mediaobj.h"
#include "dmodshow.h" //CLSID_DMOWrapperFilter
#include "Wmcodecdsp.h" //CLSID_CMP3DecMediaObject
#include "Dmoreg.h" //DMOCATEGORY_AUDIO_DECODER //includes Mediaobj.h
#include "MMReg.h" //WAVEFORMATEX
#pragma comment(lib,"Dmoguids.lib") 
#pragma comment(lib,"wmcodecdspuuid.lib") //CLSID_CMP3DecMediaObject

#include "dx.ddraw.h" //DSHOW::IAMMediaStream::Initialize() 
#include "dx.dshow.h" 

extern bool (*DSHOW::opening)(LPCWSTR) = 0;

extern void DSHOW::Yelp(const char *Interface)
{
	DSHOW_LEVEL(7) << '~' << Interface << "()\n";
}

HRESULT DSHOW::IMultiMediaStream::QueryInterface(REFIID riid, LPVOID FAR *ppvObj)
{			
	DSHOW_LEVEL(7) << "IMultiMediaStream::QueryInterface()\n";

	LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';

	return proxy->QueryInterface(riid,ppvObj);
}
ULONG DSHOW::IMultiMediaStream::AddRef()
{			
	DSHOW_LEVEL(7) << "IMultiMediaStream::AddRef()\n";

	return proxy->AddRef();
}
ULONG DSHOW::IMultiMediaStream::Release()
{			
	DSHOW_LEVEL(7) << "IMultiMediaStream::Release()\n";

	ULONG out = proxy?proxy->Release():0; 

	if(out==0) delete this; 

	return out;
}
HRESULT DSHOW::IMultiMediaStream::GetInformation(DWORD *x, STREAM_TYPE *y)    
{			
	DSHOW_LEVEL(7) << "IMultiMediaStream::GetInformation()\n";

	return proxy->GetInformation(x,y);
}
HRESULT DSHOW::IMultiMediaStream::GetMediaStream(REFMSPID x, ::IMediaStream **y)    
{			
	DSHOW_LEVEL(7) << "IMultiMediaStream::GetMediaStream()\n";

	return proxy->GetMediaStream(x,y);
}
HRESULT DSHOW::IMultiMediaStream::EnumMediaStreams(long x, ::IMediaStream **y)     
{			
	DSHOW_LEVEL(7) << "IMultiMediaStream::EnumMediaStreams()\n";

	return proxy->EnumMediaStreams(x,y);
}
HRESULT DSHOW::IMultiMediaStream::GetState(STREAM_STATE *x)    
{			
	DSHOW_LEVEL(7) << "IMultiMediaStream::GetState()\n";

	return proxy->GetState(x);
}
HRESULT DSHOW::IMultiMediaStream::SetState(STREAM_STATE x)     
{			
	DSHOW_LEVEL(7) << "IMultiMediaStream::SetState()\n";

	return proxy->SetState(x);
}
HRESULT DSHOW::IMultiMediaStream::GetTime(STREAM_TIME *x)    
{			
	DSHOW_LEVEL(7) << "IMultiMediaStream::GetTime()\n";

	return proxy->GetTime(x);
}
HRESULT DSHOW::IMultiMediaStream::GetDuration(STREAM_TIME *x)     
{			
	DSHOW_LEVEL(7) << "IMultiMediaStream::GetDuration()\n";

	return proxy->GetDuration(x);
}
HRESULT DSHOW::IMultiMediaStream::Seek(STREAM_TIME x)     
{			
	DSHOW_LEVEL(7) << "IMultiMediaStream::Seek()\n";

	return proxy->Seek(x);
}
HRESULT DSHOW::IMultiMediaStream::GetEndOfStreamEventHandle(HANDLE *x)    
{			
	DSHOW_LEVEL(7) << "IMultiMediaStream::GetEndOfStreamEventHandle()\n";

	return proxy->GetEndOfStreamEventHandle(x);
}






HRESULT DSHOW::IAMMultiMediaStream::QueryInterface(REFIID riid, LPVOID FAR *ppvObj)
{			
	DSHOW_LEVEL(7) << "IAMMultiMediaStream::QueryInterface()\n";

	LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';

	return proxy->QueryInterface(riid,ppvObj);
}
ULONG DSHOW::IAMMultiMediaStream::AddRef()
{			
	DSHOW_LEVEL(7) << "IAMMultiMediaStream::AddRef()\n";

	return proxy->AddRef();
}
ULONG DSHOW::IAMMultiMediaStream::Release()
{			
	DSHOW_LEVEL(7) << "IAMMultiMediaStream::Release()\n";

	ULONG out = proxy?proxy->Release():0; 

	if(out==0) delete this; 

	return out;
}
HRESULT DSHOW::IAMMultiMediaStream::GetInformation(DWORD *x, STREAM_TYPE *y)    
{			
	DSHOW_LEVEL(7) << "IAMMultiMediaStream::GetInformation()\n";

	return proxy->GetInformation(x,y);
}
HRESULT DSHOW::IAMMultiMediaStream::GetMediaStream(REFMSPID x, ::IMediaStream **y)    
{			
	DSHOW_LEVEL(7) << "IAMMultiMediaStream::GetMediaStream()\n"; //
			
	static OLECHAR g[64]; StringFromGUID2(x,g,64); DSHOW_LEVEL(7) << ' ' << g << '\n';

	if(!directshow7compatible)
	{
		if(y) *y = new DSHOW::IMediaStream;	return !S_OK;
	}

	HRESULT out = proxy->GetMediaStream(x,y); if(out!=S_OK) return out;

	if(x==MSPID_PrimaryVideo) //assuming IDirectDraw
	{
		DSHOW_LEVEL(7) << " (PrimaryVideo)\n";

		DSHOW::IMediaStream *p = DSHOW::IMediaStream::get_head->get(*y);

		if(!p) p = (DSHOW::IMediaStream*)DSHOW::IAMMediaStream::get_head->get(*y);

		if(!p) p = new DSHOW::IMediaStream; p->proxy = *y;

		*y = (::IMediaStream*)p;
	}

	return out;
}
HRESULT DSHOW::IAMMultiMediaStream::EnumMediaStreams(long x, ::IMediaStream **y)     
{			
	DSHOW_LEVEL(7) << "IAMMultiMediaStream::EnumMediaStreams()\n";

	return proxy->EnumMediaStreams(x,y);
}
HRESULT DSHOW::IAMMultiMediaStream::GetState(STREAM_STATE *x)    
{			
	DSHOW_LEVEL(7) << "IAMMultiMediaStream::GetState()\n";

	return proxy->GetState(x);
}
HRESULT DSHOW::IAMMultiMediaStream::SetState(STREAM_STATE x)     
{			
	DSHOW_LEVEL(7) << "IAMMultiMediaStream::SetState()\n";

	if(!directshow7compatible) return S_OK;

	return proxy->SetState(x);
}
HRESULT DSHOW::IAMMultiMediaStream::GetTime(STREAM_TIME *x)    
{			
	DSHOW_LEVEL(7) << "IAMMultiMediaStream::GetTime()\n";

	if(!directshow7compatible)
	{
		if(x) *x = 0; return S_OK;
	}

	return proxy->GetTime(x);
}
HRESULT DSHOW::IAMMultiMediaStream::GetDuration(STREAM_TIME *x)     
{			
	DSHOW_LEVEL(7) << "IAMMultiMediaStream::GetDuration()\n";

	if(!directshow7compatible)
	{
		if(x) *x = 0; return S_OK;
	}

	return proxy->GetDuration(x);
}
HRESULT DSHOW::IAMMultiMediaStream::Seek(STREAM_TIME x)     
{			
	DSHOW_LEVEL(7) << "IAMMultiMediaStream::Seek()\n";

	if(!directshow7compatible) return S_OK;

	return proxy->Seek(x);
}
HRESULT DSHOW::IAMMultiMediaStream::GetEndOfStreamEventHandle(HANDLE *x)  
{			
	DSHOW_LEVEL(7) << "IAMMultiMediaStream::GetEndOfStreamEventHandle()\n";

	return proxy->GetEndOfStreamEventHandle(x);
}
HRESULT DSHOW::IAMMultiMediaStream::Initialize(STREAM_TYPE x, DWORD y, IGraphBuilder *z)	        
{			
	DSHOW_LEVEL(7) << "IAMMultiMediaStream::Initialize()\n";

	DSHOW_LEVEL(7) << ' ' << y << ':' << z << '\n';

	return proxy->Initialize(x,y,z);
}
HRESULT DSHOW::IAMMultiMediaStream::GetFilterGraph(IGraphBuilder **x)	        
{			
	DSHOW_LEVEL(7) << "IAMMultiMediaStream::GetFilterGraph()\n";

	return proxy->GetFilterGraph(x);
}
HRESULT DSHOW::IAMMultiMediaStream::GetFilter(IMediaStreamFilter **x)         
{			
	DSHOW_LEVEL(7) << "IAMMultiMediaStream::GetFilter()\n";

	return proxy->GetFilter(x);
}
HRESULT DSHOW::IAMMultiMediaStream::AddMediaStream(IUnknown *x, const MSPID *y, DWORD z, ::IMediaStream **w)	         
{			
	DSHOW_LEVEL(7) << "IAMMultiMediaStream::AddMediaStream()\n";

#define OUT(x) if(z&x) DDRAW_LEVEL(2) << ' ' << #x << '\n';

	OUT(AMMSF_ADDDEFAULTRENDERER)
	OUT(AMMSF_CREATEPEER)
	OUT(AMMSF_NOSTALL)
	OUT(AMMSF_STOPIFNOSAMPLES)

#undef OUT

	if(y&&*y==MSPID_PrimaryVideo) //NOT SAFE!!
	{	
		DDRAW::IDirectDraw *dd = DDRAW::is_proxy<DDRAW::IDirectDraw>(x);
				
		if(dd) 
		{
			x = (IUnknown*)dd->proxy; 

			DSHOW_LEVEL(7) << " IUnknown: IDirectDraw\n";			
		}

		if(dd&&dd->target!='dx7a') //create dummy proxy
		{
			if(w) *w = new DSHOW::IMediaStream;	
			
			directshow7compatible = false;  

			return S_OK;
		}
		else if(w)
		{
			static ::IMediaStream *q = 0; q = 0; //paranoia

			HRESULT out = proxy->AddMediaStream(x,y,z,&q); 
		
			if(out!=S_OK){ DSHOW_LEVEL(7) << "AddMediaStream() Failed\n"; return out; }

			if(DSHOW::doIMediaStream)
			{	
				*w = q; ::IMediaStream *ms = *(::IMediaStream**)w;
				
				DSHOW::IMediaStream *p = new DSHOW::IMediaStream;

				p->proxy = ms; *w = p;
			}
			else *w = q;

			return out;		
		}
	}

	return proxy->AddMediaStream(x,y,z,w);
}
HRESULT DSHOW::IAMMultiMediaStream::OpenFile(LPCWSTR x, DWORD y)        
{			
	DSHOW_LEVEL(7) << "IAMMultiMediaStream::OpenFile()\n";

#define OUT(Y) if(y&Y) DDRAW_LEVEL(2) << ' ' << #Y << '\n';

	OUT(AMMSF_NOCLOCK)
	OUT(AMMSF_NORENDER)
	OUT(AMMSF_RENDERALLSTREAMS)
	OUT(AMMSF_RENDERTOEXISTING)
	OUT(AMMSF_RUN)

#undef OUT

	DSHOW_LEVEL(7) << ' ' << x << '\n';

	if(opening&&!DSHOW::opening(x)||!directshow7compatible)
	return E_INVALIDARG;

	HRESULT out = proxy->OpenFile(x,y&AMMSF_RENDERALLSTREAMS);

	if(out!=S_OK)
	{
		DSHOW_ERROR(2) << " IAMMultiMediaStream::OpenFile() Failed...\n";
											
		char error[100] = ""; AMGetErrorTextA(out,error,sizeof(error));

		DSHOW_ERROR(2) << ' ' << error << '\n'; 
	}

	return out;
}
HRESULT DSHOW::IAMMultiMediaStream::OpenMoniker(IBindCtx *x, IMoniker *y, DWORD z)         
{			
	DSHOW_LEVEL(7) << "IAMMultiMediaStream::OpenMoniker()\n";

	return proxy->OpenMoniker(x,y,z);
}
HRESULT DSHOW::IAMMultiMediaStream::Render(DWORD x)
{			
	DSHOW_LEVEL(7) << "IAMMultiMediaStream::Render()\n";

	return proxy->Render(x);
}




DSHOW::IMediaStream *DSHOW::IMediaStream::get_head = 0; //static

HRESULT DSHOW::IMediaStream::QueryInterface(REFIID riid, LPVOID FAR *ppvObj)
{			
	DSHOW_LEVEL(7) << "IMediaStream::QueryInterface()\n";

	LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';

	if(riid==IID_IDirectDrawMediaStream) //{F4104FCE-9A70-11D0-8FDE-00C04FD9189D}
	{	
		DSHOW_LEVEL(7) << "(IDirectDrawMediaStream)\n";		

		if(!proxy) //warning: memory leakish (need to implement DSHOW::u_dlists)
		{ 
			if(ppvObj) *ppvObj = new DSHOW::IDirectDrawMediaStream; return S_OK; 
		} 

		::IDirectDrawMediaStream *q = 0;

		HRESULT out = proxy->QueryInterface(riid,(LPVOID FAR*)&q); 
		
		if(out!=S_OK){ DSHOW_LEVEL(7) << "IMediaStream::QueryInterface() Failed\n"; return out; }

		if(DSHOW::doIDirectDrawMediaStream)
		{	
			*ppvObj = q; ::IDirectDrawMediaStream *dd = *(::IDirectDrawMediaStream**)ppvObj;
			
			DSHOW::IDirectDrawMediaStream *p = new DSHOW::IDirectDrawMediaStream;

			p->proxy = dd; *ppvObj = p;
		}
		else *ppvObj = q;

		return out;	
	}

	return proxy->QueryInterface(riid,ppvObj);
}
ULONG DSHOW::IMediaStream::AddRef()
{
	DSHOW_LEVEL(7) << "IMediaStream::AddRef()\n";

	return proxy->AddRef();
}
ULONG DSHOW::IMediaStream::Release()
{
	DSHOW_LEVEL(7) << "IMediaStream::Release()\n";

	ULONG out = proxy?proxy->Release():0; 

	if(out==0) delete this; 

	return out;
}
HRESULT DSHOW::IMediaStream::GetMultiMediaStream(::IMultiMediaStream **x)        
{
	DSHOW_LEVEL(7) << "IMediaStream::GetMultiMediaStream()\n";

	return proxy->GetMultiMediaStream(x);
}
HRESULT DSHOW::IMediaStream::GetInformation(MSPID *x, STREAM_TYPE *y)        
{
	DSHOW_LEVEL(7) << "IMediaStream::GetInformation()\n";

	return proxy->GetInformation(x,y);
}
HRESULT DSHOW::IMediaStream::SetSameFormat(::IMediaStream *x, DWORD y)        
{
	DSHOW_LEVEL(7) << "IMediaStream::SetSameFormat()\n";

	return proxy->SetSameFormat(x,y);
}
HRESULT DSHOW::IMediaStream::AllocateSample(DWORD x, IStreamSample **y)        
{
	DSHOW_LEVEL(7) << "IMediaStream::AllocateSample()\n";

	return proxy->AllocateSample(x,y);
}
HRESULT DSHOW::IMediaStream::CreateSharedSample(IStreamSample *x, DWORD y, IStreamSample **z)        
{
	DSHOW_LEVEL(7) << "IMediaStream::CreateSharedSample()\n";

	return proxy->CreateSharedSample(x,y,z);
}
HRESULT DSHOW::IMediaStream::SendEndOfStream(DWORD x)
{
	DSHOW_LEVEL(7) << "IMediaStream::SendEndOfStream()\n";

	return proxy->SendEndOfStream(x);
}




DSHOW::IAMMediaStream *DSHOW::IAMMediaStream::get_head = 0; //static

HRESULT DSHOW::IAMMediaStream::QueryInterface(REFIID riid, LPVOID FAR *ppvObj)
{			
	DSHOW_LEVEL(7) << "IAMMediaStream::QueryInterface()\n";

	LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';

	return proxy->QueryInterface(riid,ppvObj);
}
ULONG DSHOW::IAMMediaStream::AddRef()
{
	DSHOW_LEVEL(7) << "IAMMediaStream::AddRef()\n";

	return proxy->AddRef();
}
ULONG DSHOW::IAMMediaStream::Release()
{
	DSHOW_LEVEL(7) << "IAMMediaStream::Release()\n";

	ULONG out = proxy?proxy->Release():0; 

	if(out==0) delete this; 

	return out;
}
HRESULT DSHOW::IAMMediaStream::GetMultiMediaStream(::IMultiMediaStream **x)        
{
	DSHOW_LEVEL(7) << "IAMMediaStream::GetMultiMediaStream()\n";

	return proxy->GetMultiMediaStream(x);
}
HRESULT DSHOW::IAMMediaStream::GetInformation(MSPID *x, STREAM_TYPE *y)        
{
	DSHOW_LEVEL(7) << "IAMMediaStream::GetInformation()\n";

	return proxy->GetInformation(x,y);
}
HRESULT DSHOW::IAMMediaStream::SetSameFormat(::IMediaStream *x, DWORD y)        
{
	DSHOW_LEVEL(7) << "IAMMediaStream::SetSameFormat()\n";

	return proxy->SetSameFormat(x,y);
}
HRESULT DSHOW::IAMMediaStream::AllocateSample(DWORD x, IStreamSample **y)        
{
	DSHOW_LEVEL(7) << "IAMMediaStream::AllocateSample()\n";

	return proxy->AllocateSample(x,y);
}
HRESULT DSHOW::IAMMediaStream::CreateSharedSample(IStreamSample *x, DWORD y, IStreamSample **z)        
{
	DSHOW_LEVEL(7) << "IAMMediaStream::CreateSharedSample()\n";

	return proxy->CreateSharedSample(x,y,z);
}
HRESULT DSHOW::IAMMediaStream::SendEndOfStream(DWORD x)
{
	DSHOW_LEVEL(7) << "IAMMediaStream::SendEndOfStream()\n";

	return proxy->SendEndOfStream(x);
}
HRESULT DSHOW::IAMMediaStream::Initialize(IUnknown *x, DWORD y, REFMSPID z, const STREAM_TYPE w)         
{
	DSHOW_LEVEL(7) << "IAMMediaStream::Initialize()\n";
	 	
	DDRAW::IDirectDraw *dd = DDRAW::is_proxy<DDRAW::IDirectDraw>(x);

	if(dd) 
	{
		DSHOW_LEVEL(7) << " IUnkown: IDirectDraw\n";

		x = (IUnknown*)dd->proxy;
	}

	return proxy->Initialize(x,y,z,w);
}
HRESULT DSHOW::IAMMediaStream::SetState(FILTER_STATE x)			         
{
	DSHOW_LEVEL(7) << "IAMMediaStream::SetState()\n";

	return proxy->SetState(x);
}
HRESULT DSHOW::IAMMediaStream::JoinAMMultiMediaStream(::IAMMultiMediaStream *x)	        
{
	DSHOW_LEVEL(7) << "IAMMediaStream::JoinAMMultiMediaStream()\n";

	return proxy->JoinAMMultiMediaStream(x);
}
HRESULT DSHOW::IAMMediaStream::JoinFilter(IMediaStreamFilter *x)         
{
	DSHOW_LEVEL(7) << "IAMMediaStream::JoinFilter()\n";

	return proxy->JoinFilter(x);
}
HRESULT DSHOW::IAMMediaStream::JoinFilterGraph(IFilterGraph *x)
{
	DSHOW_LEVEL(7) << "IAMMediaStream::JoinFilterGraph()\n";

	return proxy->JoinFilterGraph(x);
}




DSHOW::IDirectDrawMediaStream *DSHOW::IDirectDrawMediaStream::get_head = 0; //static

HRESULT DSHOW::IDirectDrawMediaStream::QueryInterface(REFIID riid, LPVOID FAR *ppvObj)
{			
	DSHOW_LEVEL(7) << "IDirectDrawMediaStream::QueryInterface()\n";

	LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';

	return proxy->QueryInterface(riid,ppvObj);
}
ULONG DSHOW::IDirectDrawMediaStream::AddRef()
{
	DSHOW_LEVEL(7) << "IDirectDrawMediaStream::AddRef()\n";

	return proxy->AddRef();
}
ULONG DSHOW::IDirectDrawMediaStream::Release()
{
	DSHOW_LEVEL(7) << "IDirectDrawMediaStream::Release()\n";

	ULONG out = proxy?proxy->Release():0; 

	if(out==0) delete this; 

	return out;
}
HRESULT DSHOW::IDirectDrawMediaStream::GetMultiMediaStream(::IMultiMediaStream **x)        
{
	DSHOW_LEVEL(7) << "IDirectDrawMediaStream::GetMultiMediaStream()\n";

	return proxy->GetMultiMediaStream(x);
}
HRESULT DSHOW::IDirectDrawMediaStream::GetInformation(MSPID *x, STREAM_TYPE *y)        
{
	DSHOW_LEVEL(7) << "IDirectDrawMediaStream::GetInformation()\n";

	return proxy->GetInformation(x,y);
}
HRESULT DSHOW::IDirectDrawMediaStream::SetSameFormat(::IMediaStream *x, DWORD y)        
{
	DSHOW_LEVEL(7) << "IDirectDrawMediaStream::SetSameFormat()\n";

	return proxy->SetSameFormat(x,y);
}
HRESULT DSHOW::IDirectDrawMediaStream::AllocateSample(DWORD x, IStreamSample **y)        
{
	DSHOW_LEVEL(7) << "IDirectDrawMediaStream::AllocateSample()\n";

	return proxy->AllocateSample(x,y);
}
HRESULT DSHOW::IDirectDrawMediaStream::CreateSharedSample(IStreamSample *x, DWORD y, IStreamSample **z)        
{
	DSHOW_LEVEL(7) << "IDirectDrawMediaStream::CreateSharedSample()\n";

	return proxy->CreateSharedSample(x,y,z);
}
HRESULT DSHOW::IDirectDrawMediaStream::SendEndOfStream(DWORD x)
{
	DSHOW_LEVEL(7) << "IDirectDrawMediaStream::SendEndOfStream()\n";

	return proxy->SendEndOfStream(x);
}
HRESULT DSHOW::IDirectDrawMediaStream::GetFormat(DDSURFACEDESC *x, IDirectDrawPalette **y, DDSURFACEDESC *z, DWORD *w)  
{
	DSHOW_LEVEL(7) << "IDirectDrawMediaStream::GetFormat()\n";

	return proxy->GetFormat(x,y,z,w);
}
HRESULT DSHOW::IDirectDrawMediaStream::SetFormat(const DDSURFACEDESC *x, IDirectDrawPalette *y)  
{
	DSHOW_LEVEL(7) << "IDirectDrawMediaStream::SetFormat()\n";

	return proxy->SetFormat(x,y);
}
HRESULT DSHOW::IDirectDrawMediaStream::GetDirectDraw(IDirectDraw **x)
{
	DSHOW_LEVEL(7) << "IDirectDrawMediaStream::GetDirectDraw()\n";

	DSHOW_LEVEL(1) << " PANIC!: GetDirectDraw() was called...\n";

	return proxy->GetDirectDraw(x);
}
HRESULT DSHOW::IDirectDrawMediaStream::SetDirectDraw(IDirectDraw *x)
{
	DSHOW_LEVEL(7) << "IDirectDrawMediaStream::SetDirectDraw()\n";

	DDRAW::IDirectDraw *p = DDRAW::is_proxy<DDRAW::IDirectDraw>(x);

	if(p) x = p->proxy7; return proxy->SetDirectDraw(x);
}
HRESULT DSHOW::IDirectDrawMediaStream::CreateSample(IDirectDrawSurface *x, const RECT *y, DWORD z, ::IDirectDrawStreamSample **w)
{
	DSHOW_LEVEL(7) << "IDirectDrawMediaStream::CreateSample()\n";

	DDRAW::IDirectDrawSurface *p = DDRAW::is_proxy<DDRAW::IDirectDrawSurface>(x);

	if(p) x = p->proxy7; if(!w) return !S_OK; //paranoia

	if(!proxy)
	{
		DSHOW::IDirectDrawStreamSample *q = new DSHOW::IDirectDrawStreamSample;

		q->surface = p; *w = q; return S_OK;		
	}	

	::IDirectDrawStreamSample *q = 0;

	HRESULT out = proxy->CreateSample(x,y,z,w?&q:0);

	if(out!=S_OK){ DSHOW_LEVEL(7) << "IDirectDrawMediaStream::CreateSample() Failed\n"; return out; }

	if(DSHOW::doIDirectDrawStreamSample)
	{	
		*w = q; ::IDirectDrawStreamSample *ddss = *w;
		
		DSHOW::IDirectDrawStreamSample *p = new DSHOW::IDirectDrawStreamSample;
				
		p->proxy = ddss; *w = p;
	}
	else *w = q; 

	return out;
}
HRESULT DSHOW::IDirectDrawMediaStream::GetTimePerFrame(STREAM_TIME *x)
{
	DSHOW_LEVEL(7) << "IDirectDrawMediaStream::GetTimePerFrame()\n";

	return proxy->GetTimePerFrame(x);
}







HRESULT DSHOW::IDirectDrawStreamSample::QueryInterface(REFIID riid, LPVOID FAR *ppvObj)
{			
	DSHOW_LEVEL(7) << "IDirectDrawStreamSample::QueryInterface()\n";

	LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';

	return proxy->QueryInterface(riid,ppvObj);
}
ULONG DSHOW::IDirectDrawStreamSample::AddRef()
{
	DSHOW_LEVEL(7) << "IDirectDrawStreamSample::AddRef()\n";

	return proxy->AddRef();
}
ULONG DSHOW::IDirectDrawStreamSample::Release()
{
	DSHOW_LEVEL(7) << "IDirectDrawStreamSample::Release()\n";

	ULONG out = proxy?proxy->Release():0; 

	if(out==0) delete this; 

	return out;
}
HRESULT DSHOW::IDirectDrawStreamSample::GetMediaStream(::IMediaStream **x)
{
	DSHOW_LEVEL(7) << "IDirectDrawStreamSample::GetMediaStream()\n";

	return proxy->GetMediaStream(x);
}
HRESULT DSHOW::IDirectDrawStreamSample::GetSampleTimes(STREAM_TIME *x, STREAM_TIME *y, STREAM_TIME *z)
{
	DSHOW_LEVEL(7) << "IDirectDrawStreamSample::GetSampleTimes()\n";

	return proxy->GetSampleTimes(x,y,z);
}
HRESULT DSHOW::IDirectDrawStreamSample::SetSampleTimes(const STREAM_TIME *x, const STREAM_TIME *y)
{
	DSHOW_LEVEL(7) << "IDirectDrawStreamSample::SetSampleTimes()\n";

	return proxy->SetSampleTimes(x,y);
}
HRESULT DSHOW::IDirectDrawStreamSample::Update(DWORD x, HANDLE y, PAPCFUNC z, DWORD_PTR w)
{
	DSHOW_LEVEL(7) << "IDirectDrawStreamSample::Update()\n";

	if(!proxy) return !S_OK;

	return proxy->Update(x,y,z,w);
}
HRESULT DSHOW::IDirectDrawStreamSample::CompletionStatus(DWORD x, DWORD y)
{
	DSHOW_LEVEL(7) << "IDirectDrawStreamSample::CompletionStatus()\n";

	return proxy->CompletionStatus(x,y);
}
HRESULT DSHOW::IDirectDrawStreamSample::GetSurface(IDirectDrawSurface **x, RECT *y)
{
	DSHOW_LEVEL(7) << "IDirectDrawStreamSample::GetSurface()\n";

	if(!x) return !S_OK; //paranoia

	if(!proxy)
	{
		*x = (IDirectDrawSurface*)surface; return S_OK; //note: ignoring y
	}

	HRESULT out = proxy->GetSurface(x,y); if(out!=S_OK) return out;

	if(!surface||surface->proxy!=(DX::IDirectDrawSurface*)*x) //hard lookup
	{	
		DDRAW::IDirectDrawSurface *p = DDRAW::IDirectDrawSurface::get_head->get(*x);

		if(!p&&DDRAW::doIDirectDrawSurface)
		{
			p = new DDRAW::IDirectDrawSurface('dx7a'); p->proxy7 = *x;
		}

		if(p) *x = (::IDirectDrawSurface*)p; 
	}
	else *x = (IDirectDrawSurface*)surface;

	return out;
}
HRESULT DSHOW::IDirectDrawStreamSample::SetRect(const RECT *x)
{
	DSHOW_LEVEL(7) << "IDirectDrawStreamSample::SetRect()\n";

	return proxy->SetRect(x);
}
		
namespace DSHOW
{
	static HRESULT (STDAPICALLTYPE*CoCreateInstance)(REFCLSID,LPUNKNOWN,DWORD,REFIID,LPVOID*) = 0; 
}
HRESULT STDAPICALLTYPE 						
dx_dshow_CoCreateInstance(REFCLSID rclsid,LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID* ppv)
{	
	DSHOW_LEVEL(7) << "dx_dshow_CoCreateInstance()\n";

	DX::is_needed_to_initialize();

	LPOLESTR p; if(StringFromCLSID(rclsid,&p)==S_OK) DSHOW_LEVEL(7) << "CLSID: " << dx%p << '\n';

	//{49C47CE5-9BA4-11D0-8212-00C04FC32C45} CLSID_AMMultiMediaStream 

	DSHOW_LEVEL(7) << "pUnkOuter: " << pUnkOuter << '\n';

	if(StringFromIID(riid,&p)==S_OK) DSHOW_LEVEL(7) << "IID: " << p << '\n';

	//BEBE595C-9A6F-11d0-8FDE-00C04FD9189D") IID_IAMMultiMediaStream 
						 
	if(rclsid==CLSID_AMMultiMediaStream&&riid==IID_IAMMultiMediaStream)
	{	
		if(0) return !S_OK; //disable movies

		DSHOW_LEVEL(7) << " (AMMultiMediaStream/IAMMultiMediaStream)\n";

		IAMMultiMediaStream *q = 0;

		HRESULT out = DSHOW::CoCreateInstance(rclsid,pUnkOuter,dwClsContext,riid,(LPVOID*)&q); 
		
		if(out!=S_OK){ DSHOW_LEVEL(7) << "CoCreateInstance() Failed\n"; return out; }

		if(DSHOW::doIAMMultiMediaStream) //TODO: DSHOW::is_proxy
		{	
			*ppv = q; IAMMultiMediaStream *ammms = *(IAMMultiMediaStream**)ppv;
			
			DSHOW::IAMMultiMediaStream *p = new DSHOW::IAMMultiMediaStream;

			p->proxy = ammms; *ppv = p;
		}
		else *ppv = q;

		return out;		
	}
	return DSHOW::CoCreateInstance(rclsid,pUnkOuter,dwClsContext,riid,ppv);
}
static void dx_dshow_detours(LONG (WINAPI *f)(PVOID*,PVOID))
{
	if(!DSHOW::CoCreateInstance)
	DSHOW::CoCreateInstance = CoCreateInstance;	
	assert(CoCreateInstance);
	f(&(PVOID&)DSHOW::CoCreateInstance,dx_dshow_CoCreateInstance);	

}//register dx_dshow_detours
static int dx_dshow_detouring = DX::detouring(dx_dshow_detours);




	//2018 //2018 //2018 //2018 //2018 //2018 //2018 //2018



/*SAMPLE CODE trying to allow MP3 files, etc. for use as BGM
https://groups.google.com/forum/#!topic/microsoft.public.win32.programmer.directx.audio/Cq2f1soJJ4c
https://www.codeproject.com/articles/2662/converting-wav-file-to-mp-or-other-format-using-d
https://stackoverflow.com/questions/20115186/what-sdk-version-to-download (Windows 7 SDK has DirectShow)
/*
HRESULT CDSEncoder::AddFilterByClsid(IGraphBuilder *pGraph, LPCWSTR wszName, const GUID& clsid, IBaseFilter **ppF)
{
    *ppF = NULL;
    HRESULT hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER,
       IID_IBaseFilter, (void**)ppF);
    if (SUCCEEDED(hr))
    {
        hr = pGraph->AddFilter((*ppF), wszName);
    }
    return hr;
}
BOOL CDSEncoder::SetFilterFormat(AM_MEDIA_TYPE* pStreamFormat, IBaseFilter* pBaseFilter)
{
	HRESULT hr;
	BOOL retVal = FALSE;

	// Pin enumeration
	IEnumPins* pEnumPins = NULL;
	hr = pBaseFilter->EnumPins(&pEnumPins);
	if (FAILED(hr)) {
		// ERROR HERE
		return FALSE;
	}

	IPin* pPin = NULL;
	while (pEnumPins->Next(1, &pPin, 0) == S_OK) {
		PIN_DIRECTION sDirection;
		pPin->QueryDirection(&sDirection);
		// Output Pin ?
		if (sDirection == PINDIR_OUTPUT) {
			IAMStreamConfig* pStreamConfig = NULL;
			hr = pPin->QueryInterface(IID_IAMStreamConfig, (void**) &pStreamConfig);
			if (SUCCEEDED(hr)) {
				hr = pStreamConfig->SetFormat(pStreamFormat);
				if (SUCCEEDED(hr)) {
					retVal = TRUE;
				}
				pStreamConfig->Release();
			}
		}
		pPin->Release();
	}

	// Free memory
	pEnumPins->Release();

	return retVal;
}
void CDSEncoder::BuildGraph(CString szSrcFileName, CString szDestFileName, int nCodec, int nFormat)
{
	HRESULT hr;
	IBaseFilter *pParser = NULL, *pCodec = NULL, *pMux = NULL, *pDest = NULL;
	IFileSinkFilter* pSink = NULL;
	IFileSourceFilter* pSourceFilter = NULL;

	GUID CLSID_WavParser;
	UuidFromString((unsigned char*)"3C78B8E2-6C4D-11D1-ADE2-0000F8754B99", &CLSID_WavParser);
	GUID CLSID_WavDest;
	UuidFromString((unsigned char*)"D51BD5A1-7548-11CF-A520-0080C77EF58A", &CLSID_WavDest);

	// GraphBuilder construction
	hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**) &m_pGraphBuilder);
	if (SUCCEEDED(hr)) {
		// Parse filter
		hr = AddFilterByClsid(m_pGraphBuilder, L"Parser", CLSID_WavParser, &pParser);

		// ACM codec filter
		IMoniker* pMoniker = GetAt(nCodec)->m_pMoniker;;
		pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter, (void**) &pCodec);
		hr = m_pGraphBuilder->AddFilter(pCodec, L"ACM Codec");

		// Mux filter
		hr = AddFilterByClsid(m_pGraphBuilder, L"WavDest", CLSID_WavDest, &pMux);

		// Output file filter
		hr = AddFilterByClsid(m_pGraphBuilder, L"File Writer", CLSID_FileWriter, &pDest);
		pDest->QueryInterface(IID_IFileSinkFilter, (void**) &pSink);
		pSink->SetFileName(szDestFileName.AllocSysString(), NULL);
		pSink->Release();

		// Calculate output file size
		CFileStatus fileStatus;
		CFile::GetStatus(szSrcFileName, fileStatus);
		int nInputFileSize = fileStatus.m_size;
		// Assuming 44kHz, 16bits, stereo
		int nMediaTime = (int) (nInputFileSize / (44000 * 2 * 2));
		WAVEFORMATEX *pWav = (WAVEFORMATEX *) GetAt(nCodec)->GetAt(nFormat)->m_pMediaType->pbFormat;
		int nOutputSize = nMediaTime * (pWav->nAvgBytesPerSec);

		// Check for output File
		try {
			CFile::Remove(szDestFileName);
		} catch(...) {
			// nothing to do
		}

		// Render Graph
		hr = m_pGraphBuilder->RenderFile(szSrcFileName.AllocSysString(), NULL);
		if (SUCCEEDED(hr)) {
			// Set Codec property
			hr = SetFilterFormat(GetAt(nCodec)->GetAt(nFormat)->m_pMediaType, pCodec);
			// Retrieve control interfaces
			IMediaControl* pMediaControl = NULL;
			hr = m_pGraphBuilder->QueryInterface(IID_IMediaControl, (void**) &pMediaControl);
			if (SUCCEEDED(hr)) {
				hr = pMediaControl->Run();
				// start encoding
				if (SUCCEEDED(hr)) {
					long nCode = 0;
					IMediaEvent *pMediaEvent = NULL;
					m_pGraphBuilder->QueryInterface(IID_IMediaEvent, (void**) &pMediaEvent);
					int nPercentComplete = 0;

					// Wait until job complete
					while (nCode != EC_COMPLETE) {
						pMediaEvent->WaitForCompletion(1000, &nCode);
						// Report Progress
						CFile::GetStatus(szDestFileName, fileStatus);
						CString szPercent;
						szPercent.Format("%d %%", (fileStatus.m_size*100)/nOutputSize);
						AfxGetMainWnd()->SetWindowText(szPercent);
					}

					pMediaControl->Stop();
					AfxGetMainWnd()->SetWindowText("Complete");
					pMediaEvent->Release();
				} else {
					char szError[1024];
					AMGetErrorText(hr, szError, 1024);
					CString szDesc(szError);
					AfxMessageBox(szDesc);
				}

				pMediaControl->Release();
			}
		}

		// Free interfaces
		pCodec->Release();
		pParser->Release();
		pMux->Release();
		pDest->Release();

		m_pGraphBuilder->Release();
		m_pGraphBuilder = NULL;
	}
}*/
/*REFERENCE 
static void dx_dshow_pins(IBaseFilter *pBaseFilter)
{
	IEnumPins *pEnumPins;
	pBaseFilter->EnumPins(&pEnumPins);

	IPin* pPin; PIN_INFO pi;
	while(!pEnumPins->Next(1,&pPin,0)) 
	{
		//NOTE! the name may be "XForm In" but in that case
		//FindPin uses "In" ... maybe spaces are separators
		pPin->QueryPinInfo(&pi);
		pPin->Release();
	}

	pEnumPins->Release();
}
static IBaseFilter *dx_dshow_DMO_wrapper(const GUID &id, const GUID &cat) 
{
		//"Using DMOs in DirectShow"

	// Create the DMO Wrapper filter.
	IBaseFilter *o; IDMOWrapperFilter *w;
	if(CoCreateInstance(CLSID_DMOWrapperFilter,NULL,CLSCTX_INPROC_SERVER,IID_IBaseFilter,(void**)&o))
	return 0;

	// Query for IDMOWrapperFilter.
	o->QueryInterface(IID_IDMOWrapperFilter,(void**)&w);

	// Initialize the filter.
	HRESULT hr = w->Init(id,cat); w->Release();

	if(!hr) return o; assert(0); o->Release(); return 0;
}*/
static std::vector<IGraphBuilder*> dx_dshow_media;
extern bool dx_dshow_media_release(bool only_if_finished=true)
{
	if(dx_dshow_media.empty()) return false;
	size_t i,j; 
	for(i=0,j=0;i<dx_dshow_media.size();i++) if(only_if_finished)
	{
		IGraphBuilder *ig = dx_dshow_media[i];
		long ok = 0;
		IMediaEvent *ime = 0; IMediaControl *imc = 0;
		if(!ig->QueryInterface(IID_IMediaEvent,(void**)&ime))
		{
			if(VFW_E_WRONG_STATE==ime->WaitForCompletion(0,&ok))
			ok = 1;
			if(ok==1)
			if(!ig->QueryInterface(IID_IMediaControl,(void**)&imc))
			{
				imc->Stop(); imc->Release(); 
			}
			ime->Release();
		}
		else ok = 1;
		if(ok!=1) dx_dshow_media[j++] = dx_dshow_media[i];
		else ig->Release();
	}
	else dx_dshow_media[i]->Release(); dx_dshow_media.resize(j); return true;
}
extern bool dx_dshow_begin_conversion(const wchar_t *in[2], const wchar_t *out[2], HANDLE out2=0, bool finish=false)
{
	//THOUGHTS: DirectX Media Objects (DMO) might be better
	//suited to SOM's needs, except that it appears to lack
	//an API for doing file extension based pairing to DMOs

		//EXPERIMENTAL
		//if(0||!DX::debug) return false; //disabling

	dx_dshow_media_release();

	// set of in, out pins from filters
	IPin *ip[4] = {};

	/*
	graphBuilder = (IGraphBuilder)new FilterGraph();
	mediaControl = (IMediaControl)graphBuilder;

	// creating File Source filter
	IBaseFilter sourceFilter = null;
	graphBuilder.AddSourceFilter(@"C:\Temp\a.mp3", "source",
	out sourceFilter);
	*/
	IGraphBuilder *ig;
	IBaseFilter *isf = 0;
	if(!CoCreateInstance(CLSID_FilterGraph,0,CLSCTX_INPROC_SERVER,IID_IGraphBuilder,(void**)&ig))
	{	
		//if(1) //seems like should be just as good
		{
			ig->AddSourceFilter(*in,L"source",&isf);
			isf->FindPin(L"Output",ip+0); //sourceOut
		}
		/*else if(!CoCreateInstance(CLSID_WMAsfReader,0,CLSCTX_INPROC_SERVER,IID_IBaseFilter,(void**)&isf))
		{
			IFileSourceFilter *ifs;
			if(!isf->QueryInterface(IID_IFileSourceFilter,(void**)&ifs))
			{
				ifs->Load(*in,0); 
			}
			else assert(0);

			ig->AddFilter(isf,L"source");	

			//"Raw Audio 0" is the only pin
			//isf->FindPin(L"Output",ip+0); //sourceOut
			isf->FindPin(L"Raw Audio 0",ip+0);						
		}*/			
	}
	else return false;
	
	/*unnecessary if WavDest is modified to negotiate
	//for WAVE_FORMAT_PCM
	//NOTE: I don't think DirectShow has an MP3 codec
	//yet AddSourceFilter finds one nonetheless 
	if(0) //if(mp3)
	if(IBaseFilter *mp3=dx_dshow_DMO_wrapper(CLSID_CMP3DecMediaObject,DMOCATEGORY_AUDIO_DECODER))
	{
		ig->AddFilter(mp3,L"decoder");
		//dx_dshow_pins(mp3);
		if(!mp3->FindPin(L"in0",ip+1)) 
		{
			if(!mp3->FindPin(L"out0",ip+2))
			{
				ig->Connect(ip[0],ip[1]);			
				ip[0] = ip[2];
			}			
			ip[1]->Release();
		}
		ip[1] = ip[2] = 0; mp3->Release(); 
	}*/
	 
	/*
	// creating WavDest filter
	Type type = Type.GetTypeFromCLSID(new Guid("E882F102-
	F626-49E9-BD68-CE2BE7E59EA0"));
	IBaseFilter wavedest =
	(DirectShowLib.IBaseFilter)Activator.CreateInstance(type);
	*/
	//GUID CLSID_WavDest; //where is this defined?
	//CLSIDFromString(L"{E882F102-F626-49E9-BD68-CE2BE7E59EA0}",&CLSID_WavDest);
	IBaseFilter *idf = 0;
	//if(!CoCreateInstance(CLSID_WavDest,0,CLSCTX_INPROC_SERVER,IID_IBaseFilter,(void**)&idf))
	extern IBaseFilter *SOM_SDK_CreateInstance_pcmdest(HRESULT*);
	HRESULT hr = 0;
	if(idf=SOM_SDK_CreateInstance_pcmdest(&hr))
	{
		//want to be sure WavDest static library is binary compatible
		hr = ig->AddFilter(idf,L"destination");
		assert(!hr);
		idf->FindPin(L"In",ip+1); //waveDestIn
		idf->FindPin(L"Out",ip+2); //waveDestOut
	}
		//TODO: would like to do without the file-writer interface to
		//instead use an on-demand pull-model to fill the DirectSound
		//buffer in real-time (on the behalf of som_db.exe)

	//HRESULT hr = 0;
	/*
	// creating File Writer filter
	FileWriter writer = new FileWriter();
	IFileSinkFilter fs = (IFileSinkFilter)writer;
	fs.SetFileName(@"C:\Temp\b.wav", null);
	*/
	IBaseFilter *iwf = 0;
	if(!CoCreateInstance(CLSID_FileWriter,0,CLSCTX_INPROC_SERVER,IID_IBaseFilter,(void**)&iwf))
	{
		ig->AddFilter(iwf,L"writer");		
		iwf->FindPin(L"in",ip+3); //writerIn //"In"
		IFileSinkFilter *i;
		if(!iwf->QueryInterface(IID_IFileSinkFilter,(void**)&i))
		{
			hr|=i->SetFileName(*out,0); i->Release();
		}
		else hr = E_FAIL;
	}

	// connecting filters
	if(ip[0]&&ip[1]) hr|=ig->Connect(ip[0],ip[1]); //sourceOut, waveDestIn
	else hr = E_FAIL;
	if(ip[2]&&ip[3]) hr|=ig->Connect(ip[2],ip[3]); //waveDestOut, writerIn
	//else hr = E_FAIL;		
	{			
		/*NOTHING SEEMS TO WORK... modifying WavDest?
		//HACK: ConnectionMediaType doesn't work prior
		//to "intelligent connect"
		AM_MEDIA_TYPE mt; mt.pbFormat = 0;
		ip[1]->ConnectionMediaType(&mt);
		if(mt.formattype==FORMAT_WaveFormatEx)
		{
			WAVEFORMATEX &f = *(WAVEFORMATEX*)mt.pbFormat;
			f.wFormatTag = WAVE_FORMAT_PCM;				
			if(2==f.nChannels)
			{
				f.nChannels/=2;				
				f.nBlockAlign/=2; //wBitsPerSample*nChannels/8
				f.nAvgBytesPerSec/=2; //nSamplesPerSec*nBlockAlign
			}
			mt.cbFormat = 16;
			mt.bTemporalCompression = 0;
			mt.bFixedSizeSamples = 1;
			mt.lSampleSize = 2;
		
			//ig->Disconnect(ip[0]);
			//ig->Disconnect(ip[1]);
			//hr|=ip[0]->ReceiveConnection(ip[1],&mt);
		}
		else assert(0);	

		//FreeMediaType
		if(mt.pbFormat) CoTaskMemFree((void*)mt.pbFormat);
		if(mt.pUnk) mt.pUnk->Release();
		*/
	}
	for(int i=0;i<4;i++) if(ip[i]) ip[i]->Release();
	 
	IMediaControl *imc = 0;
	if(!hr&&!ig->QueryInterface(IID_IMediaControl,(void**)&imc))
	{
		imc->Run(); 
		
		//REMINDER: caller can do this if out2 is 0
		//write header
		//hack: this is supposed to have a side effect		
		//DX::sleep(5);
		while(out2&&(LONG)GetFileSize(out2,0)<=44)
		{
			DX::sleep();
			DWORD err = GetLastError();			
			if(err) break;
		}

		imc->Release();
	}
	else hr = E_FAIL;

	if(isf) isf->Release();
	if(idf) idf->Release();
	if(iwf) iwf->Release(); 
	
	if(finish&&!hr) //need to set data size field?
	{
		IMediaEvent *ime = 0;  IMediaControl *imc = 0;
		if(!ig->QueryInterface(IID_IMediaEvent,(void**)&ime))
		{
			long ok = 0;
			auto test = ime->WaitForCompletion(-1,&ok); 
			if(ok==1)
			if(!ig->QueryInterface(IID_IMediaControl,(void**)&imc))
			{
				imc->Stop(); imc->Release(); 
			}
			ime->Release();
		}
		ig->Release();
	}
	else if(ig&&!hr)
	{
		dx_dshow_media.push_back(ig);

		dx_dshow_media_release();
	}
	else ig->Release(); return !hr;
}