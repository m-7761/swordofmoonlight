	   
#ifndef DX_DSHOW_INCLUDED
#define DX_DSHOW_INCLUDED

#include "dx8.1/mmstream.h"	//IMultiMediaStream
#include "dx8.1/amstream.h"	//IAMMultiMediaStream
#include "dx8.1/ddstream.h" //IDirectDrawMediaStream

#ifdef DIRECTX_INCLUDED

namespace DSHOW
{
	static bool doIMultiMediaStream = 1;
	static bool doIAMMultiMediaStream = 1; //ms.win32.cpp
	static bool doIMediaStream = 1;
	static bool doIAMMediaStream = 1;
	static bool doIDirectDrawMediaStream = 1;
	static bool doIDirectDrawStreamSample = 1;

	//user handler for IAMMultiMediaStream::OpenFile
	//return false to prevent the media from playing
	extern bool (*opening)(LPCWSTR);
}

namespace DSHOWLOG
{
	static int debug = 0; //debugging
	static int error = 7; //serious errors
	static int panic = 7; //undefined
	static int alert = 7; //warning
	static int hello = 7; //fun stuff
	static int sorry = 7; //woops

	static int *master = DX_LOG(DShow);
}
			  
#define DSHOW_LEVEL(lv) if(lv<=DSHOWLOG::debug&&lv<=*DSHOWLOG::master) dx.log
#define DSHOW_ERROR(lv) if(lv<=DSHOWLOG::error&&lv<=*DSHOWLOG::master) dx.log
#define DSHOW_PANIC(lv) if(lv<=DSHOWLOG::panic&&lv<=*DSHOWLOG::master) dx.log
#define DSHOW_ALERT(lv) if(lv<=DSHOWLOG::alert&&lv<=*DSHOWLOG::master) dx.log
#define DSHOW_HELLO(lv) if(lv<=DSHOWLOG::hello&&lv<=*DSHOWLOG::master) dx.log
#define DSHOW_SORRY(lv) if(lv<=DSHOWLOG::sorry&&lv<=*DSHOWLOG::master) dx.log

#endif //DIRECTX_INCLUDED

namespace DDRAW //dx.ddraw.h
{
	class IDirectDrawSurface;
}

namespace DSHOW
{
	//DSHOW_LEVEL(7) << "~I()\n";
	extern void Yelp(const char *I);
}

namespace DSHOW{
class IMultiMediaStream : public ::IMultiMediaStream
{
public: ::IMultiMediaStream *proxy;

	IMultiMediaStream(){ proxy = 0; }

	~IMultiMediaStream(){ DSHOW::Yelp("IMultiMediaStream"); } //hack

	/*** IUnknown methods ***/
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID FAR *ppvObj);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
	/*** IMultiMediaStream methods ***/
    HRESULT STDMETHODCALLTYPE GetInformation(DWORD*,STREAM_TYPE*);    
	HRESULT STDMETHODCALLTYPE GetMediaStream(REFMSPID,::IMediaStream**);    
    HRESULT STDMETHODCALLTYPE EnumMediaStreams(long,::IMediaStream**);     
    HRESULT STDMETHODCALLTYPE GetState(STREAM_STATE*);    
    HRESULT STDMETHODCALLTYPE SetState(STREAM_STATE);     
    HRESULT STDMETHODCALLTYPE GetTime(STREAM_TIME*);    
    HRESULT STDMETHODCALLTYPE GetDuration(STREAM_TIME*);     
    HRESULT STDMETHODCALLTYPE Seek(STREAM_TIME);     
    HRESULT STDMETHODCALLTYPE GetEndOfStreamEventHandle(HANDLE*);    
};}

namespace DSHOW{
class IAMMultiMediaStream : public ::IAMMultiMediaStream
{
public: ::IAMMultiMediaStream *proxy;

	bool directshow7compatible;

	IAMMultiMediaStream(){ proxy = 0; directshow7compatible = true; }

	~IAMMultiMediaStream(){ DSHOW::Yelp("IAMMultiMediaStream"); } //hack 

	/*** IUnknown methods ***/
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID FAR *ppvObj);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
	/*** IMultiMediaStream methods ***/
    HRESULT STDMETHODCALLTYPE GetInformation(DWORD*,STREAM_TYPE*);    
	HRESULT STDMETHODCALLTYPE GetMediaStream(REFMSPID,::IMediaStream**);    
    HRESULT STDMETHODCALLTYPE EnumMediaStreams(long,::IMediaStream**);     
    HRESULT STDMETHODCALLTYPE GetState(STREAM_STATE*);    
    HRESULT STDMETHODCALLTYPE SetState(STREAM_STATE);     
    HRESULT STDMETHODCALLTYPE GetTime(STREAM_TIME*);    
    HRESULT STDMETHODCALLTYPE GetDuration(STREAM_TIME*);     
    HRESULT STDMETHODCALLTYPE Seek(STREAM_TIME);     
    HRESULT STDMETHODCALLTYPE GetEndOfStreamEventHandle(HANDLE*);  
	/*** IAMMultiMediaStream methods ***/
	HRESULT STDMETHODCALLTYPE Initialize(STREAM_TYPE,DWORD,IGraphBuilder*);	        
    HRESULT STDMETHODCALLTYPE GetFilterGraph(IGraphBuilder**);	        
    HRESULT STDMETHODCALLTYPE GetFilter(IMediaStreamFilter**);         
	HRESULT STDMETHODCALLTYPE AddMediaStream(IUnknown*,const MSPID*,DWORD,::IMediaStream**);	         
    HRESULT STDMETHODCALLTYPE OpenFile(LPCWSTR,DWORD);        
    HRESULT STDMETHODCALLTYPE OpenMoniker(IBindCtx*,IMoniker*,DWORD);         
    HRESULT STDMETHODCALLTYPE Render(DWORD);
};}

namespace DSHOW{
class IMediaStream : public ::IMediaStream
{
public: ::IMediaStream *proxy;

	IMediaStream *get_next, *get_prev;

	static IMediaStream *get_head; 

	IMediaStream():proxy(0)
	{	
		get_next = get_head?get_head:this; 		

		get_prev = get_head?get_head->get_prev:this; 

		get_next->get_prev = this;
		get_prev->get_next = this;

		get_head = this;
	}

	~IMediaStream()
	{
		get_prev->get_next = get_next; get_next->get_prev = get_prev;

		if(get_head==this) get_head = get_next==this?0:get_next;		

		DSHOW::Yelp("IMediaStream"); //hack 
	}

	IMediaStream *get(::IMediaStream *in)
	{
		if(!this||proxy==in) return this; //TODO: paranoia

		for(IMediaStream *out=get_next;out&&out!=this;out=out->get_next)
		
			if(out->proxy==in) return out; return 0; return 0; //TODO: assert(out)
	}

	/*** IUnknown methods ***/
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID FAR *ppvObj);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
	/*** IMediaStream methods ***/
	HRESULT STDMETHODCALLTYPE GetMultiMediaStream(::IMultiMediaStream**);        
    HRESULT STDMETHODCALLTYPE GetInformation(MSPID*,STREAM_TYPE*);
	HRESULT STDMETHODCALLTYPE SetSameFormat(::IMediaStream*,DWORD);
    HRESULT STDMETHODCALLTYPE AllocateSample(DWORD,IStreamSample**);
    HRESULT STDMETHODCALLTYPE CreateSharedSample(IStreamSample*,DWORD,IStreamSample**);        
    HRESULT STDMETHODCALLTYPE SendEndOfStream(DWORD);
};}

namespace DSHOW{
class IAMMediaStream : public ::IAMMediaStream
{
public: ::IAMMediaStream *proxy;

	IAMMediaStream *get_next, *get_prev;

	static IAMMediaStream *get_head; 

	IAMMediaStream():proxy(0)
	{	
		get_next = get_head?get_head:this; 		

		get_prev = get_head?get_head->get_prev:this; 

		get_next->get_prev = this;
		get_prev->get_next = this;

		get_head = this;
	}

	~IAMMediaStream()
	{
		get_prev->get_next = get_next; get_next->get_prev = get_prev;

		if(get_head==this) get_head = get_next==this?0:get_next;		
		
		DSHOW::Yelp("IAMMediaStream"); //hack 
	}

	IAMMediaStream *get(::IMediaStream *in)
	{
		if(!this||proxy==in) return this; //TODO: paranoia

		for(IAMMediaStream *out=get_next;out&&out!=this;out=out->get_next)
		
			if(out->proxy==in) return out; return 0;return 0; //TODO: assert(out)
	}

	/*** IUnknown methods ***/
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID FAR *ppvObj);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    /*** IMediaStream methods ***/
	HRESULT STDMETHODCALLTYPE GetMultiMediaStream(::IMultiMediaStream**);        
    HRESULT STDMETHODCALLTYPE GetInformation(MSPID*,STREAM_TYPE*);        
    HRESULT STDMETHODCALLTYPE SetSameFormat(::IMediaStream*,DWORD);        
    HRESULT STDMETHODCALLTYPE AllocateSample(DWORD,IStreamSample**);        
    HRESULT STDMETHODCALLTYPE CreateSharedSample(IStreamSample*,DWORD,IStreamSample**);        
    HRESULT STDMETHODCALLTYPE SendEndOfStream(DWORD dwFlags);
	/*** IAMMediaStream methods ***/
    HRESULT STDMETHODCALLTYPE Initialize(IUnknown*,DWORD,REFMSPID,const STREAM_TYPE);         
    HRESULT STDMETHODCALLTYPE SetState(FILTER_STATE);			         
	HRESULT STDMETHODCALLTYPE JoinAMMultiMediaStream(::IAMMultiMediaStream*);	        
    HRESULT STDMETHODCALLTYPE JoinFilter(IMediaStreamFilter*);         
    HRESULT STDMETHODCALLTYPE JoinFilterGraph(IFilterGraph*);
};}

namespace DSHOW{
class IDirectDrawMediaStream : public ::IDirectDrawMediaStream
{
public: ::IDirectDrawMediaStream *proxy;

	IDirectDrawMediaStream *get_next, *get_prev;

	static IDirectDrawMediaStream *get_head; 

	IDirectDrawMediaStream():proxy(0)
	{	
		get_next = get_head?get_head:this; 		

		get_prev = get_head?get_head->get_prev:this; 

		get_next->get_prev = this;
		get_prev->get_next = this;

		get_head = this;
	}

	~IDirectDrawMediaStream()
	{
		get_prev->get_next = get_next; get_next->get_prev = get_prev;

		if(get_head==this) get_head = get_next==this?0:get_next;		

		DSHOW::Yelp("IDirectDrawMediaStream"); //hack 
	}

	IDirectDrawMediaStream *get(::IMediaStream *in)
	{
		if(!this||proxy==in) return this; //TODO: paranoia

		for(IDirectDrawMediaStream *out=get_next;out&&out!=this;out=out->get_next)
		
			if(out->proxy==in) return out; return 0; //TODO: assert(out)
	}

	/*** IUnknown methods ***/
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID FAR *ppvObj);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    /*** IMediaStream methods ***/
	HRESULT STDMETHODCALLTYPE GetMultiMediaStream(::IMultiMediaStream**);        
    HRESULT STDMETHODCALLTYPE GetInformation(MSPID*,STREAM_TYPE*);        
    HRESULT STDMETHODCALLTYPE SetSameFormat(::IMediaStream*,DWORD);        
    HRESULT STDMETHODCALLTYPE AllocateSample(DWORD,::IStreamSample**);        
    HRESULT STDMETHODCALLTYPE CreateSharedSample(::IStreamSample*,DWORD,::IStreamSample**);        
    HRESULT STDMETHODCALLTYPE SendEndOfStream(DWORD dwFlags);
	/*** IDirectDrawMediaStream methods ***/
    HRESULT STDMETHODCALLTYPE GetFormat(DDSURFACEDESC*,IDirectDrawPalette**,DDSURFACEDESC*,DWORD*);    
    HRESULT STDMETHODCALLTYPE SetFormat(const DDSURFACEDESC*,IDirectDrawPalette*);    
    HRESULT STDMETHODCALLTYPE GetDirectDraw(IDirectDraw**);
    HRESULT STDMETHODCALLTYPE SetDirectDraw(IDirectDraw*);    
    HRESULT STDMETHODCALLTYPE CreateSample(IDirectDrawSurface*,const RECT*,DWORD,::IDirectDrawStreamSample**);
    HRESULT STDMETHODCALLTYPE GetTimePerFrame(STREAM_TIME*);    
};}

namespace DSHOW{
class IDirectDrawStreamSample : public ::IDirectDrawStreamSample
{
public: ::IDirectDrawStreamSample *proxy;

	DDRAW::IDirectDrawSurface *surface;

	IDirectDrawStreamSample(){ proxy = 0; surface = 0; }

	~IDirectDrawStreamSample(){ DSHOW::Yelp("IDirectDrawStreamSample"); } //hack 

	/*** IUnknown methods ***/
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID FAR *ppvObj);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
	/*** IStreamSample methods ***/
	HRESULT STDMETHODCALLTYPE GetMediaStream(::IMediaStream**);
	HRESULT STDMETHODCALLTYPE GetSampleTimes(STREAM_TIME*,STREAM_TIME*,STREAM_TIME*);
	HRESULT STDMETHODCALLTYPE SetSampleTimes(const STREAM_TIME*,const STREAM_TIME*);
	HRESULT STDMETHODCALLTYPE Update(DWORD,HANDLE,PAPCFUNC,DWORD_PTR);
	HRESULT STDMETHODCALLTYPE CompletionStatus(DWORD,DWORD);
	/*** IDirectDrawStreamSample methods ***/
	HRESULT STDMETHODCALLTYPE GetSurface(IDirectDrawSurface**,RECT*);
	HRESULT STDMETHODCALLTYPE SetRect(const RECT*);
};}

#endif //DX_DSHOW_INCLUDED 