
#include "Ex.h" 
EX_TRANSLATION_UNIT

#include "dx8.1/dshow.h"

#define DIRECT3D_VERSION 0x0900

#include <d3d9.h> //vmr9.h does not include this itself
#include <d3dx9.h> //for vmr9 sample code

#include "dx9extras/vmr9.h"
#include "dx9extras/uuids.h"

#pragma comment(lib,"dx9extras/strmiids.lib")

#include "dx.ddraw.h"
#include "dx.dinput.h" //todo: graceful movie cancel
#include "Ex.window.h"
#include "Ex.movie.h"

static bool Ex_movie_playing = false;
static bool Ex_movie_stop_playing = false;
static bool Ex_movie_have_picture = false;

static void Ex_movie_plaster_screen(::IDirect3DTexture9*,float);

static BOOL VerifyVMR9(void) //note: from samples
{
    //Verify that the VMR exists on this system

    IBaseFilter* pBF = NULL;
	HRESULT hr = ::CoCreateInstance
	(CLSID_VideoMixingRenderer9,0,CLSCTX_INPROC,IID_IBaseFilter,(LPVOID*)&pBF);
    if(SUCCEEDED(hr))
    {
        pBF->Release(); return TRUE;
    }
    else
    {
        /*MessageBox(NULL,
        TEXT("This application requires the VMR-9.\r\n\r\n")
        TEXT("The VMR-9 is not enabled when viewing through a Remote\r\n")
        TEXT(" Desktop session. You can run VMR-enabled applications only\r\n") 
        TEXT("on your local computer.\r\n\r\n")
        TEXT("\r\nThis sample will now exit."),
        TEXT("Video Mixing Renderer (VMR9) capabilities are required"), MB_OK);
		*/return FALSE;
    }
}

//REMOVE ME?
#define FAIL_RET(x) do{ if(FAILED(hr=(x))) return hr; }while(0)

namespace{ //2018: DirectShow(strmbase) definies CCritSec/QzAtlComPtrAssign

	//wxutil: same as AtlComPtrAssign (scheduled obsolete)
	IUnknown* QzAtlComPtrAssign(IUnknown** pp, IUnknown* lp) 
	{
		if(pp&&*pp) (*pp)->Release(); if(!pp) return 0; //docs are unclear
	
		if(lp) lp->AddRef(); return *pp = lp;
	}

	class CCritSec //wxutil.h (scheduled obsolete)
	{
		// make copy constructor and assignment operator inaccessible

		CCritSec(const CCritSec &refCritSec);
		CCritSec &operator=(const CCritSec &refCritSec);

		CRITICAL_SECTION m_CritSec;
	/*
	#ifdef DEBUG
	public:
		DWORD   m_currentOwner;
		DWORD   m_lockCount;
		BOOL    m_fTrace;        // Trace this one
	public:
		CCritSec();
		~CCritSec();
		void Lock();
		void Unlock();
	#else
	*/
	public:
		CCritSec() {
			InitializeCriticalSection(&m_CritSec);
		};

		~CCritSec() {
			DeleteCriticalSection(&m_CritSec);
		};

		void Lock() {
			EnterCriticalSection(&m_CritSec);
		};

		void Unlock() {
			LeaveCriticalSection(&m_CritSec);
		};
	//#endif
	};
}

class CAutoLock //wxutil.h (scheduled obsolete)
{
    // make copy constructor and assignment operator inaccessible

    CAutoLock(const CAutoLock &refAutoLock);
    CAutoLock &operator=(const CAutoLock &refAutoLock);

protected:
    CCritSec * m_pLock;

public:
    CAutoLock(CCritSec * plock)
    {
        m_pLock = plock;
        m_pLock->Lock();
    };

    ~CAutoLock() {
        m_pLock->Unlock();
    };
};

template <class T> class CComPtr //from ATL (scheduled obsolete)
{
public:

	//from CComPtrBase 
	HRESULT CopyTo(T** ppT) //throw();
	{			
		if(ppT) *ppT = p; else return !S_OK;

		p->AddRef(); return S_OK;
	}

	//from CComPtrBase 
	HRESULT CoCreateInstance
		(REFCLSID rclsid, LPUNKNOWN pUnkOuter=0, DWORD dwClsContext=CLSCTX_ALL) //throw();
	{
		if(p) return !S_OK;

		//warning: assuming 0 safe here
		return ::CoCreateInstance(rclsid,pUnkOuter,dwClsContext,__uuidof(T),(void**)&p);
	}

	//from CComPtrBase 
	void Attach(T* p2) //throw();
	{
		if(p) p->Release(); p = p2;
	}


	typedef T _PtrClass;
	CComPtr() {p=0;}
	CComPtr(T* lp)
	{
		if ((p = lp) != 0)
			p->AddRef();
	}
	CComPtr(const CComPtr<T>& lp)
	{
		if ((p = lp.p) != 0)
			p->AddRef();
	}
	~CComPtr() {if (p) p->Release();}
	void Release() {if (p) p->Release(); p=0;}
	operator T*() {return (T*)p;}
	T& operator*() {assert(p!=0); return *p; }
	//The assert on operator& usually indicates a bug.  If this is really
	//what is needed, however, take the address of the p member explicitly.
	T** operator&() { assert(p==0); return &p; }
	T* operator->() { assert(p!=0); return p; }
	T* operator=(T* lp){return (T*)QzAtlComPtrAssign((IUnknown**)&p, lp);}
	T* operator=(const CComPtr<T>& lp)
	{
		return (T*)QzAtlComPtrAssign((IUnknown**)&p, lp.p);
	}
#if _MSC_VER>1020
	bool operator!(){return (p == 0);}
#else
	BOOL operator!(){return (p == 0) ? TRUE : FALSE;}
#endif
	T* p;
};

namespace EX{
namespace AVI{ 
class Vmr9  : public IVMRSurfaceAllocator9, IVMRImagePresenter9
{
public:
    Vmr9(HWND h); virtual ~Vmr9();

    // IVMRSurfaceAllocator9
    virtual HRESULT STDMETHODCALLTYPE InitializeDevice( 
            /* [in] */ DWORD_PTR dwUserID,
            /* [in] */ VMR9AllocationInfo *lpAllocInfo,
            /* [out][in] */ DWORD *lpNumBuffers);
            
    virtual HRESULT STDMETHODCALLTYPE TerminateDevice( 
        /* [in] */ DWORD_PTR dwID);
    
    virtual HRESULT STDMETHODCALLTYPE GetSurface( 
        /* [in] */ DWORD_PTR dwUserID,
        /* [in] */ DWORD SurfaceIndex,
        /* [in] */ DWORD SurfaceFlags,
        /* [out] */ IDirect3DSurface9 **lplpSurface);
    
    virtual HRESULT STDMETHODCALLTYPE AdviseNotify( 
        /* [in] */ IVMRSurfaceAllocatorNotify9 *lpIVMRSurfAllocNotify);

    // IVMRImagePresenter9
    virtual HRESULT STDMETHODCALLTYPE StartPresenting( 
        /* [in] */ DWORD_PTR dwUserID);
    
    virtual HRESULT STDMETHODCALLTYPE StopPresenting( 
        /* [in] */ DWORD_PTR dwUserID);
    
    virtual HRESULT STDMETHODCALLTYPE PresentImage( 
        /* [in] */ DWORD_PTR dwUserID,
        /* [in] */ VMR9PresentationInfo *lpPresInfo);
    
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface( 
        REFIID riid,
        void** ppvObject);

    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

protected:

    bool NeedToHandleDisplayChange();

	void DeleteSurface();

    // This function is here so we can catch the loss of surfaces.
    // All the functions are using the FAIL_RET macro so that they exit
    // with the last error code.  When this returns with the surface lost
    // error code we can restore the surfaces.
    HRESULT PresentHelper(VMR9PresentationInfo *lpPresInfo);

private:
    // needed to make this a thread safe object
    CCritSec    m_ObjectLock;
    HWND        m_window;
    long        m_refCount;

    CComPtr<IVMRSurfaceAllocatorNotify9>    m_lpIVMRSurfAllocNotify;
	CComPtr<IDirect3DSurface9>			    m_surface;
    CComPtr<IDirect3DTexture9>              m_privateTexture;
};}}


EX::AVI::Vmr9::Vmr9(HWND h) : m_window(h), m_refCount(1)
{
    CAutoLock Lock(&m_ObjectLock); 
}

EX::AVI::Vmr9::~Vmr9()
{
	DeleteSurface();
}

void EX::AVI::Vmr9::DeleteSurface()
{
    CAutoLock Lock(&m_ObjectLock);

    m_privateTexture = 0;

	m_surface = 0;
}
 
//IVMRSurfaceAllocator9
HRESULT EX::AVI::Vmr9::InitializeDevice( 
            /* [in] */ DWORD_PTR dwUserID,
            /* [in] */ VMR9AllocationInfo *lpAllocInfo,
            /* [out][in] */ DWORD *lpNumBuffers)
{
    D3DCAPS9 d3dcaps;

    DWORD dwWidth = 1;
    DWORD dwHeight = 1;
    float fTU = 1.f;
    float fTV = 1.f;

    if(!lpNumBuffers) return E_POINTER;

    if(!m_lpIVMRSurfAllocNotify) return E_FAIL;
    
    HRESULT hr = S_OK;

    DDRAW::Direct3DDevice7->proxy9->GetDeviceCaps(&d3dcaps);

    if(d3dcaps.TextureCaps&D3DPTEXTURECAPS_POW2)
    {
        while(dwWidth<lpAllocInfo->dwWidth) dwWidth = dwWidth << 1;

        while(dwHeight<lpAllocInfo->dwHeight ) dwHeight = dwHeight << 1;

        fTU = (float)(lpAllocInfo->dwWidth)/(float)(dwWidth);
        fTV = (float)(lpAllocInfo->dwHeight)/(float)(dwHeight);

        lpAllocInfo->dwWidth = dwWidth;
        lpAllocInfo->dwHeight = dwHeight;
    }

    // NOTE:
    // we need to make sure that we create textures because
    // surfaces can not be textured onto a primitive.
    lpAllocInfo->dwFlags|=VMR9AllocFlag_TextureSurface;

    DeleteSurface();

    hr = m_lpIVMRSurfAllocNotify->AllocateSurfaceHelper(lpAllocInfo,lpNumBuffers,&m_surface);
    
    // If we couldn't create a texture surface and 
    // the format is not an alpha format,
    // then we probably cannot create a texture.
    // So what we need to do is create a private texture
    // and copy the decoded images onto it.
    if(FAILED(hr)&&!(lpAllocInfo->dwFlags&VMR9AllocFlag_3DRenderTarget))
    {
        DeleteSurface();            

        // is surface YUV ?
        if(lpAllocInfo->Format>'0000') 
        {           
            D3DDISPLAYMODE dm; 

            FAIL_RET(DDRAW::Direct3DDevice7->proxy9->GetDisplayMode(0,&dm));

            // create the private texture
            FAIL_RET(DDRAW::Direct3DDevice7->proxy9->CreateTexture
								(lpAllocInfo->dwWidth,lpAllocInfo->dwHeight,
                                    1, 
                                    D3DUSAGE_RENDERTARGET, 
                                    dm.Format, 
                                    D3DPOOL_DEFAULT /* default pool - usually video memory */, 
                                    &m_privateTexture.p,0));
        }

        
        lpAllocInfo->dwFlags&=~VMR9AllocFlag_TextureSurface;
        lpAllocInfo->dwFlags|=VMR9AllocFlag_OffscreenSurface;

        FAIL_RET(m_lpIVMRSurfAllocNotify->AllocateSurfaceHelper(lpAllocInfo,lpNumBuffers,&m_surface));
    }

    return S_OK;
}
            
HRESULT EX::AVI::Vmr9::TerminateDevice(DWORD_PTR dwID)
{
    DeleteSurface(); return S_OK;
}
    
HRESULT EX::AVI::Vmr9::GetSurface( 
        /* [in] */ DWORD_PTR dwUserID,
        /* [in] */ DWORD SurfaceIndex,
        /* [in] */ DWORD SurfaceFlags,
        /* [out] */ IDirect3DSurface9 **lplpSurface)
{
    if(!lplpSurface) return E_POINTER;
    
    if(SurfaceIndex>0) return E_FAIL;

    CAutoLock Lock(&m_ObjectLock);

    return m_surface.CopyTo(lplpSurface);
}
    
HRESULT EX::AVI::Vmr9::AdviseNotify( 
        /* [in] */ IVMRSurfaceAllocatorNotify9 *lpIVMRSurfAllocNotify)
{
    CAutoLock Lock(&m_ObjectLock);

    HRESULT hr;

    m_lpIVMRSurfAllocNotify = lpIVMRSurfAllocNotify;

	IDirect3D9 *d3d9 = 0;
	DDRAW::Direct3DDevice7->proxy9->GetDirect3D(&d3d9);

	HMONITOR hMonitor = d3d9?d3d9->GetAdapterMonitor(D3DADAPTER_DEFAULT):0;

	d3d9->Release();

    FAIL_RET(m_lpIVMRSurfAllocNotify->SetD3DDevice(DDRAW::Direct3DDevice7->proxy9,hMonitor));

    return hr;
}

HRESULT EX::AVI::Vmr9::StartPresenting(DWORD_PTR dwUserID)
{
    CAutoLock Lock(&m_ObjectLock);

    assert(DDRAW::Direct3DDevice7->proxy9);

    if(!DDRAW::Direct3DDevice7->proxy9) return E_FAIL;

	Ex_movie_have_picture = true; //hack??

		//REMINDER: in 2022 SomEx_movies has to call
		//EX::showing_window or the video doesn't start

	EX::showing_window(); //NEW: may show before first frame?
    
    return S_OK;
}

HRESULT EX::AVI::Vmr9::StopPresenting(DWORD_PTR dwUserID)
{
    Ex_movie_have_picture = false; //hack //assert(0); 
	
	return S_OK; //note: unimplented by sample
}

HRESULT EX::AVI::Vmr9::PresentImage( 
    /* [in] */ DWORD_PTR dwUserID,
    /* [in] */ VMR9PresentationInfo *lpPresInfo)
{
    HRESULT hr;

    CAutoLock Lock(&m_ObjectLock);

    // if we are in the middle of the display change
    if(NeedToHandleDisplayChange())
    {
        // NOTE: this piece of code is left as a user exercise.  
        // The D3DDevice here needs to be switched
        // to the device that is using another adapter
    }

    hr = PresentHelper(lpPresInfo);

    // IMPORTANT: device can be lost when user changes the resolution
    // or when (s)he presses Ctrl + Alt + Delete.
    // We need to restore our video memory after that			 
    if(hr==D3DERR_DEVICELOST)
    {
        if(DDRAW::Direct3DDevice7->proxy9->TestCooperativeLevel()==D3DERR_DEVICENOTRESET) 
        {
            DeleteSurface(); 
			
			assert(0); //FAIL_RET(CreateDevice());

			IDirect3D9 *d3d9;

			DDRAW::Direct3DDevice7->proxy9->GetDirect3D(&d3d9);

            HMONITOR hMonitor = d3d9->GetAdapterMonitor(D3DADAPTER_DEFAULT);

			d3d9->Release();

            FAIL_RET(m_lpIVMRSurfAllocNotify->ChangeD3DDevice(DDRAW::Direct3DDevice7->proxy9,hMonitor));        
		}

        hr = S_OK;
    }

		//HACK: Nvidia isn't redrawing itself promptly... it gets lost
		//too, and turns off prematurely
		if(m_window) InvalidateRect(m_window,0,0);

    return hr;
}

HRESULT EX::AVI::Vmr9::PresentHelper(VMR9PresentationInfo *lpPresInfo)
{
    if(!lpPresInfo||!lpPresInfo->lpSurf) return E_POINTER;

	HRESULT hr; CAutoLock Lock(&m_ObjectLock);

	float aspect =
	float(lpPresInfo->szAspectRatio.cx)/lpPresInfo->szAspectRatio.cy;

    // if we created a  private texture
    // blt the decoded image onto the texture.
    if(m_privateTexture)
    {   
        CComPtr<IDirect3DSurface9> surface;

        FAIL_RET(m_privateTexture->GetSurfaceLevel(0,&surface.p));

        // copy the full surface onto the texture's surface
        FAIL_RET(DDRAW::Direct3DDevice7->proxy9->StretchRect
			(lpPresInfo->lpSurf,0,surface,0,D3DTEXF_NONE));

		Ex_movie_plaster_screen(m_privateTexture,aspect);
    }
    else // this is the case where we have got the textures allocated by VMR
         // all we need to do is to get them from the surface
    {
        CComPtr<IDirect3DTexture9> texture;

        FAIL_RET(lpPresInfo->lpSurf->GetContainer(__uuidof(IDirect3DTexture9),(LPVOID*)&texture.p));    

		Ex_movie_plaster_screen(texture,aspect);
    }

	//REMINDER: I think this is failing on my new system because
	//I get output: "NVD3DREL: GR-805 : DX9 Overlay is DISABLED"
    HRESULT ok; if(DDRAW::doFlipEx)
	{		
		auto *ex = (IDirect3DDevice9Ex*)DDRAW::Direct3DDevice7->proxy9;

		//2022: Present isn't working now... so maybe PresentEx?
		ok = ex->PresentEx(0,0,0,0,0);
	}
	else ok = DDRAW::Direct3DDevice7->proxy9->Present(0,0,0,0);

	DDRAW::noFlips++; //hack

    return hr;
}

bool EX::AVI::Vmr9::NeedToHandleDisplayChange()
{
    if( m_lpIVMRSurfAllocNotify) return false;

    D3DDEVICE_CREATION_PARAMETERS Parameters;

    if(FAILED(DDRAW::Direct3DDevice7->proxy9->GetCreationParameters(&Parameters)))
    {
        assert(0); return false;
    }

	IDirect3D9 *d3d9;

	DDRAW::Direct3DDevice7->proxy9->GetDirect3D(&d3d9);

    HMONITOR currentMonitor = d3d9->GetAdapterMonitor(Parameters.AdapterOrdinal);

    HMONITOR hMonitor = d3d9->GetAdapterMonitor(D3DADAPTER_DEFAULT);

	d3d9->Release();

    return hMonitor!=currentMonitor;
}


// IUnknown
HRESULT EX::AVI::Vmr9::QueryInterface(REFIID riid, void **ppvObject)
{
    HRESULT hr = E_NOINTERFACE;

    if(!ppvObject) 
	{
        hr = E_POINTER;
    } 
    else if(riid==IID_IVMRSurfaceAllocator9) 
	{
        *ppvObject = static_cast<IVMRSurfaceAllocator9*>(this);

        AddRef(); hr = S_OK;
    } 
    else if(riid==IID_IVMRImagePresenter9) 
	{
        *ppvObject = static_cast<IVMRImagePresenter9*>(this);

        AddRef(); hr = S_OK;
    } 
    else if(riid==IID_IUnknown) 
	{
        *ppvObject = 
            static_cast<IUnknown*>( 
            static_cast<IVMRSurfaceAllocator9*>(this));

        AddRef(); hr = S_OK;    
    }

    return hr;
}

ULONG EX::AVI::Vmr9::AddRef()
{
    return InterlockedIncrement(&m_refCount);
}

ULONG EX::AVI::Vmr9::Release()
{
    ULONG ret = InterlockedDecrement(&m_refCount);

    if(ret==0) delete this;

    return ret;
}

extern bool EX::is_playing_movie()
{
	return Ex_movie_playing;
}

extern void EX::stop_playing_movie()
{
	if(Ex_movie_playing)
	{
		Ex_movie_stop_playing = true;
	}
}

//hack: movie renderer changes the viewport behind you back
static D3DVIEWPORT9 Ex_movie_viewport; 

void EX::playing_movie(const wchar_t *filename)
{
	Ex_movie_have_picture = false; 

	if(!DDRAW::Direct3DDevice7) return;

	auto d3dd9 = DDRAW::Direct3DDevice7->proxy9; if(!d3dd9) return;

	::CoInitialize(0);

	if(!VerifyVMR9()){ ::CoUninitialize(); return; }

	////////////////////////WARNING///////////////////////////
	// 
	// Nvidia just doesn't work execpt for VMR9Mode_Windowless
	// mode, and then only one time at the start of the program
	// I've made a not to try to figure something out. AMD works
	// Intel always worked (I miss my old Intel computer/chipset)
	// 
	// I get output: "NVD3DREL: GR-805 : DX9 Overlay is DISABLED"
	// But I don't know if that's really the problem or not???
	// 
	//////////////////////////////////////////////////////////
	 
	//2021: this is going to have to work with OpenGL
	//
	// I couldn't modify InitializeDevice to use a D3DPOOL_SYSTEMMEM
	// surface, it just went into a loop, kept reentering InitializeDevice
	// reallocating m_surface??? I thought maybe a system surface could
	// be uploaded to OpenGL as a regular texture. if not I'm thinking
	// of keeping a device for SYSTEMEM that can just be used for movies
	// if it plays nice with OpenGL
	//
	::IDirect3DSurface9 *backBuffer = 0, *rt[4] = {}; 
	::IDirect3DVertexShader9 *vs = 0; //are shaders not in state blocks?
	::IDirect3DPixelShader9 *ps = 0;
	::IDirect3DStateBlock9 *d3dstate = 0;
	if(DDRAW::target_backbuffer=='dx9c') //OpenGL? D3D12 back buffer?
	{
		if(d3dd9->CreateStateBlock(D3DSBT_ALL,&d3dstate)!=D3D_OK) return;

		if(d3dd9->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&backBuffer)!=D3D_OK) return;

	//	D3DSURFACE_DESC backBufferDesc; backBuffer->GetDesc(&backBufferDesc);
							   	
	//	DWORD w = backBufferDesc.Width, h = backBufferDesc.Height;
   
		d3dd9->GetViewport(&Ex_movie_viewport); 	

		//Nvidia: disabling MRT gets the movie to play the first 
		//time, but it's cut short on subsequent plays
		for(int i=4;i-->0;)
		{
			d3dd9->GetRenderTarget(i,rt+i); 

			if(rt[i]) d3dd9->SetRenderTarget(i,0);
		}
		d3dd9->SetRenderTarget(0,backBuffer); backBuffer->Release();

		d3dd9->SetViewport(&Ex_movie_viewport);

		d3dd9->GetVertexShader(&vs); d3dd9->SetVertexShader(0); 
		d3dd9->GetPixelShader(&ps);	d3dd9->SetPixelShader(0); 

	//	D3DXMATRIX matProj; // Set the projection matrix

	//	FLOAT fAspect = w/(float)h;

		d3dd9->SetRenderState(D3DRS_ZENABLE,0);
		d3dd9->SetRenderState(D3DRS_LIGHTING,0);
		d3dd9->SetRenderState(D3DRS_FOGENABLE,0);
		d3dd9->SetRenderState(D3DRS_ALPHABLENDENABLE,0);
		d3dd9->SetRenderState(D3DRS_ALPHATESTENABLE,0);
		d3dd9->SetRenderState(D3DRS_ZWRITEENABLE,0);
		d3dd9->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);	

		d3dd9->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);
		d3dd9->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);
		d3dd9->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
		d3dd9->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);
		d3dd9->SetSamplerState(0,D3DSAMP_MIPFILTER,D3DTEXF_NONE); //D3DTEXF_LINEAR

		d3dd9->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
		d3dd9->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
		d3dd9->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTA_TEXTURE);
	}

	DWORD_PTR g_userId = 0xACDCACDC; //from sample

	CComPtr<IGraphBuilder>          g_graph;
	CComPtr<IBaseFilter>            g_filter;
	CComPtr<IMediaControl>          g_mediaControl;
	CComPtr<IMediaSeeking>			g_mediaSeeking;
	CComPtr<IVMRSurfaceAllocator9>  g_allocator;
    
	HRESULT debug = g_graph.CoCreateInstance(CLSID_FilterGraph);

    debug = g_filter.CoCreateInstance(CLSID_VideoMixingRenderer9,0,CLSCTX_INPROC_SERVER);
	
    CComPtr<IVMRFilterConfig9> filterConfig;
	CComPtr<IVMRWindowlessControl9> lpIVMRWindowlessControl9;
	CComPtr<IVMRSurfaceAllocatorNotify9> lpIVMRSurfAllocNotify;

    debug = g_filter->QueryInterface(IID_IVMRFilterConfig9,(void**)&filterConfig);

	//EXPERIMENTAL (OpenGL?)
	//I'm trying to find an alternative for OpenGL to use
	//this works without D3DSWAPEFFECT_FLIPEX but is kind
	//of choppy and so is the regular frame rate 
	//https://www.khronos.org/registry/OpenGL/extensions/NV/WGL_NV_DX_interop.txt
	//it a way to use DXGI flip (FLIPEX) with OpenGL
	//DDRAW::IDirect3DDevice->proxy could be temporarily 
	//used for FLIPEX playback. IDirect3DDevice9::Reset
	//would have to used to switch back-and-forth from
	//0,0,DISCARD to w,h,FLIPEX
	if(!backBuffer||0&&EX::debug&&!DDRAW::doFlipEx) //TEMPORARY
	{
		//this will paint the window by itself, the only
		//trouble is it can't work on DXGI flip (FLIPEX)

		debug = filterConfig->SetRenderingMode(VMR9Mode_Windowless);

		debug = g_filter->QueryInterface(IID_IVMRWindowlessControl9,(void**)&lpIVMRWindowlessControl9);

		debug = lpIVMRWindowlessControl9->SetVideoClippingWindow(DDRAW::window);

		debug = lpIVMRWindowlessControl9->SetAspectRatioMode(VMR9ARMode_LetterBox); //default?

		//WINSANITY: WHY ISN'T THIS THE DEFAULT BEHAVIOR?!
		long lWidth, lHeight; 
		if(!lpIVMRWindowlessControl9->GetNativeVideoSize(&lWidth,&lHeight,0,0))
		{
			RECT rcSrc; SetRect(&rcSrc,0,0,lWidth,lHeight); 
			RECT rcDst; GetClientRect(DDRAW::window,&rcDst);
			debug = lpIVMRWindowlessControl9->SetVideoPosition(&rcSrc,&rcDst); 
		}
	}
	else //D3DSWAPEFFECT_FLIPEX compatible way
	{
		debug = filterConfig->SetRenderingMode(VMR9Mode_Renderless);

		debug = g_filter->QueryInterface(IID_IVMRSurfaceAllocatorNotify9,(void**)&lpIVMRSurfAllocNotify);

		g_allocator.Attach(new EX::AVI::Vmr9(DDRAW::window));

		// let the allocator and the notify know about each other
		debug = lpIVMRSurfAllocNotify->AdviseSurfaceAllocator(g_userId,g_allocator);
		debug = g_allocator->AdviseNotify(lpIVMRSurfAllocNotify);
	}
	
    debug = filterConfig->SetNumberOfStreams(2);
    
    debug = g_graph->AddFilter(g_filter,L"Video Mixing Renderer 9");
    
    debug = g_graph->QueryInterface(IID_IMediaControl,(void**)&g_mediaControl);
	debug = g_graph->QueryInterface(IID_IMediaSeeking,(void**)&g_mediaSeeking);

    debug = g_graph->RenderFile(filename,0);

	REFERENCE_TIME duration = 0, position = 0;
	
	debug = g_mediaSeeking->GetDuration(&duration);	position = duration;

	Ex_movie_playing = true; //hmmm: best timing??

	if(lpIVMRWindowlessControl9)
	{
		Ex_movie_have_picture = !debug; //REQUIRED/TESTING
	}

	//REMINDER: S_FALSE is normal
    debug = g_mediaControl->Run();
	
	OAFilterState state;
		
	DWORD ticks = EX::tick();

	int sleeping = 0, sleep = 200; DWORD test = 0;

			HDC test2 = GetWindowDC(DDRAW::window);

	char dinput[256];
	while(g_mediaControl->GetState(0,&state),state==State_Running)
	{
		if(Ex_movie_stop_playing) break;

		g_mediaSeeking->GetCurrentPosition(&position);	

		if(position&&!Ex_movie_have_picture) break;

		if(position>=duration) break;

		//REMINDER: Nvidia requires manually prompting repaint
		//after the first play (subsequent plays) so I've stuck
		//InvalidateRect inside PresentImage

	  ////From som.status.cpp////////////

		MSG msg; //VK_PAUSE/courtesy
		while(PeekMessage(&msg,0,0,0,PM_REMOVE))
		{ 
			if(msg.hwnd==DDRAW::window)
			if(lpIVMRWindowlessControl9)
			switch(msg.message)
			{
			case WM_PAINT:
			{
				PAINTSTRUCT ps; 
				HDC	hdc; 
			//	RECT rcClient; 
			//	GetClientRect(msg.hwnd,&rcClient); 
				hdc = BeginPaint(msg.hwnd,&ps); 
				if(lpIVMRWindowlessControl9)
				{
					debug = lpIVMRWindowlessControl9->RepaintVideo(msg.hwnd,hdc); 
					if(debug)
					{
						assert(!debug);
						Ex_movie_have_picture = false;
					}
				}
				EndPaint(msg.hwnd,&ps); 

				continue;
			}
			case WM_DISPLAYCHANGE: //???

				if(lpIVMRWindowlessControl9)
				lpIVMRWindowlessControl9->DisplayModeChanged();
				break;
			//case WM_SIZE: case WM_WINDOWPOSCHANGED:
				//lpIVMRWindowlessControl9->SetVideoPosition();
			}

			//if(!TranslateAccelerator(EX::window,haccel,&msg))
			{
				TranslateMessage(&msg);	DispatchMessage(&msg); 
			}
		}		

		DWORD delta = EX::tick();

		sleeping+=delta-ticks; ticks = delta;

		if(sleeping>sleep) 
		{
			sleeping = 0;

			if(DINPUT::Keyboard) //mimic SOM
			{
				char dinput[256];   
				DINPUT::Keyboard->Acquire(); 
				DINPUT::Keyboard->GetDeviceState(256,dinput); 
			}
			if(DINPUT::Joystick) //mimic SOM
			{
				DX::DIJOYSTATE st; DINPUT::Joystick->Acquire(); 
				if(DINPUT::Joystick->as2A) DINPUT::Joystick->as2A->Poll();	 
				DINPUT::Joystick->GetDeviceState(sizeof(st),&st);
			}
		}		
	}

	g_mediaControl->Stop();

	if(0) do
	{	
		g_mediaControl->GetState(0,&state); 	
		g_mediaSeeking->GetCurrentPosition(&position); 
		if(position>=duration) break;

	}while(state!=State_Stopped);

	Ex_movie_playing = false; //hmmm: best timing??

	Ex_movie_stop_playing = false; //reset

	//note: these assignments all Release (CComPtr)
	g_allocator    = 0; 
	g_mediaControl = 0;        
	g_mediaSeeking = 0; 
	g_filter       = 0;        
	g_graph        = 0;
		
	::CoUninitialize();

	//if(DDRAW::target=='dx9c')
	//if(DDRAW::target_backbuffer=='dx9c')
	if(d3dstate)
	{
		d3dstate->Apply(); d3dstate->Release();

		DDRAW::ApplyStateBlock(); //hack

		for(int i=4;i-->0;) if(rt[i])
		{
			d3dd9->SetRenderTarget(0,rt[i]); rt[i]->Release();
		}

		d3dd9->SetVertexShader(vs); 
		d3dd9->SetPixelShader(ps); 

		d3dd9->SetViewport(&Ex_movie_viewport); 
	}
	else assert(!rt[0]&&!ps);
}

static EX::Movie Ex_movie_black_mattes(float w, float h, float aspect);

static void Ex_movie_plaster_screen(::IDirect3DTexture9 *with, float aspect)
{
	if(!DDRAW::Direct3DDevice7||DDRAW::Direct3DDevice7->target!='dx9c') return;

	IDirect3DDevice9 *d3dd9 = DDRAW::Direct3DDevice7->proxy9; if(!d3dd9) return;
						
	IDirect3DSurface9 *rt = 0; //really the backbuffer (having troubles

    if(d3dd9->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&rt)!=D3D_OK) return;

	D3DSURFACE_DESC desc; rt->GetDesc(&desc);

	float w = desc.Width, h = desc.Height;

	//note: movie render changes the render target behind your back
	d3dd9->SetRenderTarget(0,rt); 

	D3DVIEWPORT9 fullscreen = { 0, 0, w, h, 0.0f,1.0f }; 

	d3dd9->SetViewport(&fullscreen); //SetRenderTarget resets viewport
			
	rt->Release();

	DWORD fvf = D3DFVF_XYZRHW|D3DFVF_TEX1;

	if(!DDRAW::inScene) d3dd9->BeginScene();

	EX::Movie movie = Ex_movie_black_mattes(w,h,aspect);

	FLOAT blt[] =
	{
		movie.x1, movie.y1, 0.0f,1.0f, 0,0, //0, 0
		movie.x2, movie.y1, 0.0f,1.0f, 1,0, //w, 0
		movie.x2, movie.y2, 0.0f,1.0f, 1,1, //w, h
		movie.x1, movie.y2, 0.0f,1.0f, 0,1, //0, h
	};

	d3dd9->SetTexture(0,with);

	d3dd9->SetFVF(fvf); 
	d3dd9->DrawPrimitiveUP(D3DPT_TRIANGLEFAN,2,blt,6*sizeof(float)); 

		//if(EX::debug) //nothing (Nvidia)
		//d3dd9->Clear(0,0,D3DCLEAR_TARGET,0xffff0000,0,0);

	if(!DDRAW::inScene) d3dd9->EndScene();

	d3dd9->SetViewport(&Ex_movie_viewport); 
}

static EX::Movie Ex_movie_black_mattes(float w, float h, float aspect)
{
	EX::Movie out = { 0, 0, w, h };

	if(fabs(w/h-aspect)<0.0003) return out;
		
	//TODO: non stretching behavior

	if(h*aspect>w) //book shelf mattes
	{
		out.y1 = (h-w/aspect)*0.5f;	out.y2 = h-out.y1;
	}
	else  //book ended mattes
	{
		out.x1 = (w-h*aspect)*0.5f;	out.x2 = w-out.x1;
	}

	if(!DDRAW::Direct3DDevice7
	 ||!DDRAW::Direct3DDevice7->proxy9
	 ||DDRAW::Direct3DDevice7->target!='dx9c') return out;	 

	IDirect3DDevice9 *d3dd9 = DDRAW::Direct3DDevice7->proxy9; 
	
	D3DVIEWPORT9 vp; d3dd9->GetViewport(&vp);

	D3DRECT buf[4]; DWORD i = 0;

	if(out.x1!=0.0f)
	{																	  
		D3DRECT clear;

		clear.x1 = 0; clear.y1 = 0;		
		clear.y2 = h; clear.x2 = out.x1+1; 

		//d3dd9->Clear(1,&clear,D3DCLEAR_TARGET,0x00000000,0.0,0);	
		buf[i++] = clear;

		clear.x1 = out.x2-1; clear.x2 = w;

		//d3dd9->Clear(1,&clear,D3DCLEAR_TARGET,0x00000000,0.0,0);	
		buf[i++] = clear;

		/*buf won't work
		if(out.y1!=0.0f) //optimization
		{
			D3DVIEWPORT9 half = vp; //avoid double clearing corners
			
			half.X = out.x1; half.Width-=out.x1*2;

			d3dd9->SetViewport(&half);
		}*/
	}
	if(out.y1!=0.0f)
	{
		D3DRECT clear;

		clear.x1 = 0; clear.y1 = 0;		
		clear.x2 = w; clear.y2 = out.y1+1;

		//d3dd9->Clear(1,&clear,D3DCLEAR_TARGET,0x00000000,0.0,0);	
		buf[i++] = clear;

		clear.y1 = out.y2-1; clear.y2 = h;

		//d3dd9->Clear(1,&clear,D3DCLEAR_TARGET,0x00000000,0.0,0);	
		buf[i++] = clear;
	}
	if(i) d3dd9->Clear(i,buf,D3DCLEAR_TARGET,0x00000000,0.0,0);	

	d3dd9->SetViewport(&vp);

	return out;
}