														  
#include "Sompaint_D3D9.h" //pch
				
#include "../Sompaint/Sompaint.hpp" //parsing

//D3DCREATE_PUREDEVICE
#define D3D9_PUREDEVICE	0
#define D3D9_RESOLUTION(x) (x/256*256+256)

//scheduled obsolete
static DWORD D3D9_thread = 0;
static HWND D3D9_hidden = 0; //XP/Reset

#ifdef _DEBUG
static bool D3D9_d = (GetVersion()&0xFF)<6;
//required for (Explorer) Shell Extension compatibility
static DWORD D3D9_multithread = D3DCREATE_MULTITHREADED;
//static DWORD D3D9_multithread = 0;
#else
static bool D3D9_d = false;
static DWORD D3D9_multithread = D3DCREATE_MULTITHREADED;
#endif

//scheduled obsolete
static IDirect3D9       *D3D9_direct = 0;
static IDirect3DDevice9 *D3D9_device = 0;
static IDirect3DSurface9 *D3D9_surface = 0;
static IDirect3DSurface9 *D3D9_texture = 0;
static IDirect3DSurface9 *D3D9_fbuffer = 0;

//scheduled obsolete: racy thread management 
static volatile bool D3D9_working   = false;
static volatile bool D3D9_resetting = false;
static volatile bool D3D9_releasing = false;
static volatile bool D3D9_released  = true; 

namespace D3D9_cpp 
{		 
	static struct _sections 
	{
		CRITICAL_SECTION cs; //Winbase.h

		_sections(){ InitializeCriticalSection(&cs); }
		~_sections(){ DeleteCriticalSection(&cs); }

	}_sections; struct section
	{
		section(){ EnterCriticalSection(&_sections.cs); }
		~section(){ LeaveCriticalSection(&_sections.cs); }
	};				  

	#undef DELETE //winnt.h

	enum{ DELETE, RELEASE, REBUILD }; //pool calling codes

	typedef std::map<void*,void(*)(void*,int)> pool;

	typedef std::map<void*,void(*)(void*,int)>::iterator pool_it;	

	static pool reset; //hack: See D3D9_reset
}

//scheduled obsolete 
static void D3D9_release(bool reset = false) 
{	
	if(!D3D9_direct||!D3D9_dll()) return;

	HRESULT hr = S_OK; //debugging

	if(D3D9_fbuffer) D3D9_fbuffer->Release();
	if(D3D9_texture) D3D9_texture->Release();
	if(D3D9_surface) D3D9_surface->Release();	
	
	D3D9_fbuffer = D3D9_surface = 0;
	D3D9_texture = 0;	

	if(reset) return; //// full release /////////////////
		
	//Notice: assuming the dll is being unloading for now

	if(!D3D9_working
	 ||D3D9_thread==GetCurrentThreadId())
	{			
		//D3D9_cpp::section cs;
		D3D9_cpp::pool_it it = D3D9_cpp::reset.begin();

		while(it!=D3D9_cpp::reset.end()) //hack
		{
			it->second(it->first,D3D9_cpp::RELEASE); it++; 
		}

		ULONG debug = 0; //number any outstanding interfaces

		if(D3D9_device) debug = D3D9_device->Release();
		if(D3D9_direct) debug = D3D9_direct->Release();	

		D3D9_direct = 0; D3D9_device = 0;

		DestroyWindow(D3D9_hidden); D3D9_hidden = 0;
		
		//Assuming desirable for now...
		PostThreadMessage(D3D9_thread,WM_QUIT,0,0);

		D3D9_releasing = false;
	}
	else //wait for release	(and exit)
	{
		D3D9_releasing = true;			

		while(D3D9_releasing); //wait for release

		PostThreadMessage(D3D9_thread,WM_QUIT,0,0);

		DWORD x; HANDLE D3D9_worker = 
		OpenThread(THREAD_QUERY_INFORMATION,0,D3D9_thread);
		
		//let D3D9_worker return before Somplayer.dll is unloaded
		while(GetExitCodeThread(D3D9_worker,&x)&&x==STILL_ACTIVE);

		CloseHandle(D3D9_worker);
	}	

	D3D9_released = !D3D9_device;
}

//scheduled obsolete
static bool D3D9_recoup()
{
	if(!D3D9_device) return false;

	assert(!D3D9_fbuffer&&!D3D9_texture&&!D3D9_surface);

	HRESULT hr = D3D9_device->GetBackBuffer
		(0,0,D3DBACKBUFFER_TYPE_MONO,&D3D9_fbuffer);

	PDIRECT3DSURFACE9 &rt = D3D9_texture, &rs = D3D9_surface; 

	return hr==D3D_OK; //don't need the rest for now

	if(hr==D3D_OK) hr = D3D9_device->CreateRenderTarget 
		(512,512,D3DFMT_X8R8G8B8,D3DMULTISAMPLE_NONE,0,!TRUE,&rt,0);
	
	if(hr==D3D_OK) hr = D3D9_device->CreateOffscreenPlainSurface
		(512,512,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&rs,0);

	//D3D9_cpp::section cs;
	D3D9_cpp::pool_it it = D3D9_cpp::reset.begin();

	while(it!=D3D9_cpp::reset.end()) //hack
	{
		it->second(it->first,D3D9_cpp::REBUILD); it++; 
	}

	return hr==D3D_OK;
}

//scheduled obsolete
static D3DPRESENT_PARAMETERS D3D9_present = 
{
	1,1,D3DFMT_UNKNOWN,1, 
	D3DMULTISAMPLE_NONE,0,
	//Required for src/dst Present???
	D3DSWAPEFFECT_COPY,0,1, //DISCARD
	0,D3DFMT_UNKNOWN,0,0,
	//want asynchronous present//
	D3DPRESENT_INTERVAL_IMMEDIATE 
};

static INT_PTR CALLBACK D3D9_hiddenproc(HWND hwnd, UINT Msg, WPARAM, LPARAM)
{	
	//A window seems to be required by XP
	//My theory is it's more about having a
	//message loop than a device context. May
	//just be a D3DCREATE_MULTITHREADED thing??
		
	if(0) switch(Msg) //testing
	{

#ifdef _DEBUG

	default: //assert(0);
	{
		wchar_t debug[32]; swprintf_s(debug,L"Msg was %x",Msg);

		//unable to break into code when coming out of standby/hibernation
		MessageBoxW(hwnd,debug,__WFILE__ L", ln:"__WLINE__,MB_RETRYCANCEL);
	}
	case WM_SETFOCUS:
	case WM_KILLFOCUS: //message boxes?

	case WM_ACTIVATE:
	case WM_NCACTIVATE:
	case WM_IME_SETCONTEXT:
	case WM_IME_NOTIFY:
	case WM_CANCELMODE:
	case WM_TIMECHANGE:

	case 0x090: //unknown
	case 0x31e: //unknown
	case 0x31f: //unknown

	case 0xc499: //something to do with joysticks
	case 0xc2a6: //ditto???
	case 0xc054: //ditto???
	case 0xc116: //ditto???
	case 0xc2e6: //ditto???

	case WM_WININICHANGE:
	case WM_DEVICECHANGE:	
	case WM_POWERBROADCAST:
	case WM_WINDOWPOSCHANGING:
	case WM_WINDOWPOSCHANGED:
	case WM_NCCALCSIZE:

	case WM_SETFONT:
	case WM_INITDIALOG:
	case WM_CHANGEUISTATE:
	case WM_UPDATEUISTATE:	
	case WM_ACTIVATEAPP: 
		
	case WM_DESTROY: break;

#endif

	case WM_NCDESTROY:

		D3D9_hidden = 0; D3D9_release(); 

		break;

	case WM_CLOSE:
		
		DestroyWindow(hwnd); 		

		break;
	}

	return 0;
}

//scheduled obsolete
static bool D3D9_create()
{	
	D3D9_released = false;

	if(D3D9_direct) return true;

	IDirect3D9* &pd3D9 = D3D9_direct;
	const wchar_t *dll = D3D9_d?L"d3d9d.dll":L"d3d9.dll";	

	IDirect3D9 *(__stdcall *proc)(UINT) = 0;
	*(void**)&proc = GetProcAddress(LoadLibraryW(dll),"Direct3DCreate9");

	if(!proc)
	{
		assert(!"d3d9d.dll is unavailable");
		return false; //pd3D9 = Direct3DCreate9(D3D_SDK_VERSION);		 
	}
	else pd3D9 = proc(D3D_SDK_VERSION);

	if(!pd3D9) goto d3d_failure;

	D3DFORMAT d3d9f = D3DFMT_X8R8G8B8; //D3DFMT_UNKNOWN; 
	
	int modes = pd3D9->GetAdapterModeCount(D3DADAPTER_DEFAULT,d3d9f);

	while(modes==0) //TODO: be rid of this copy/paste job
	{
		switch(d3d9f) //doesn't matter, just find one that works
		{
		case D3DFMT_UNKNOWN:  d3d9f = D3DFMT_R5G6B5; break;
		case D3DFMT_R5G6B5:   d3d9f = D3DFMT_X1R5G5B5; break;
		case D3DFMT_X1R5G5B5: d3d9f = D3DFMT_A1R5G5B5; break;
		case D3DFMT_A1R5G5B5: d3d9f = D3DFMT_X8R8G8B8; break;
		case D3DFMT_X8R8G8B8: d3d9f = D3DFMT_A8R8G8B8; break;
		case D3DFMT_A8R8G8B8: d3d9f = D3DFMT_A2R10G10B10; break;

		default: goto d3d_failure;
		}

		modes = pd3D9->GetAdapterModeCount(D3DADAPTER_DEFAULT,d3d9f);
	}

	int tiny = 0x7FFFFFFF; D3DDISPLAYMODE mode, best;

	for(int i=0;i<modes;i++)
	{
		pd3D9->EnumAdapterModes(D3DADAPTER_DEFAULT,d3d9f,i,&mode);

		if(mode.Width>=512&&mode.Height>=512)
		if(mode.Height<tiny){ tiny = mode.Height; best = mode; }
	}

	IDirect3DDevice9* &pd3Dd9 = D3D9_device;

	D3D9_present.BackBufferWidth  = 512; //best.Width;
	D3D9_present.BackBufferHeight = 512; //best.Height;
	D3D9_present.BackBufferFormat = best.Format;
	D3D9_present.hDeviceWindow    = 0;
		
	HRESULT hr = !D3D_OK; //pd3D9->CreateDevice
		//(D3DADAPTER_DEFAULT,D3DDEVTYPE_REF,client, //D3DDEVTYPE_NULLREF
		 //D3DCREATE_SOFTWARE_VERTEXPROCESSING,&null,&pd3Dd9);

	//might be a d3d9d.dll thing??
	//Direct3D9: (ERROR) :Neither hDeviceWindow nor Focus window specified. ResetEx fails
	//Oh and BTW; Create fails on XP. 
	//And FYI; we don't have a window (and can't assume a Sompaste dependency)
	if(!D3D9_hidden)
	D3D9_hidden = CreateDialog(D3D9_dll(),MAKEINTRESOURCE(IDD_HIDDEN),0,D3D9_hiddenproc);

	if(hr!=D3D_OK)
	{
		hr = pd3D9->CreateDevice
			(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,D3D9_hidden,D3D9_multithread
			 |D3DCREATE_HARDWARE_VERTEXPROCESSING,&D3D9_present,&pd3Dd9);
	}

	if(hr!=D3D_OK||!D3D9_recoup()) 
	{
	d3d_failure: D3D9_release(); return false;
	}

	if(pd3Dd9)
	{
		pd3Dd9->SetRenderState(D3DRS_ZENABLE,0);
		pd3Dd9->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);		
		pd3Dd9->SetRenderState(D3DRS_ALPHATESTENABLE,0);	
	}	

	return true;
}

//scheduled obsolete
static bool D3D9_reset()
{
	if(D3D9_working
	 &&D3D9_thread!=GetCurrentThreadId())
	{
		D3D9_resetting = true; //Assuming reset may never happen...
		for(int timeout=0;D3D9_resetting&&timeout<10000;timeout++) Sleep(1);
	}
	else 
	{	
		HRESULT test = 
		D3D9_device->TestCooperativeLevel();

		if(test!=D3D_OK) 
		{
			D3D9_release("reset");

			DWORD time = GetTickCount(); //paranoia

			while(test!=D3D_OK&&GetTickCount()-time<3000)
			{
				switch(test=D3D9_device->TestCooperativeLevel())
				{
				case D3DERR_DEVICELOST:	test=test; break;
				case D3DERR_DRIVERINTERNALERROR: test=test; break;
				case D3DERR_DEVICENOTRESET: test=test; break;
				}

				if(test==D3DERR_DEVICENOTRESET)
				{
					//2005 docs say parameters are spoiled on Reset???
					D3DPRESENT_PARAMETERS present = D3D9_present; 

					switch(test=D3D9_device->Reset(&present))
					{
					case D3DERR_DEVICELOST:	test=test; break;
					case D3DERR_DRIVERINTERNALERROR: test=test; break;
					case D3DERR_OUTOFVIDEOMEMORY: test=test; break;
					case D3DERR_INVALIDCALL: test=test; break;

					case D3D_OK: D3D9_recoup(); break;
					}
				}

				if(test!=D3D_OK) Sleep(3);
			}
		}

		D3D9_resetting = false; 
	}
	return !D3D9_resetting; 
}

//scheduled obsolete
static DWORD WINAPI D3D9_worker(LPVOID)
{		
	const bool messageloop = true;

	if(D3D9_create())
	{
		D3D9_working = true;

		//messageloop: waiting for WM_QUIT
		while(messageloop||D3D9_working) 
		{  		
			if(D3D9_resetting) D3D9_reset();
			if(D3D9_releasing) D3D9_release();

			if(messageloop)
			{
				MSG msg;
				if(PeekMessageW(&msg,0,0,0,PM_REMOVE))
				{	
					DispatchMessageW(&msg); 

					if(msg.message==WM_QUIT) 
					{
						D3D9_release(); //paranoia						
						break;
					}
				}  
				else Sleep(14);
			} 
			else Sleep(50);	//nice?
		}				
		D3D9_working = false;
	}			  
	D3D9_thread = 0; //ok?

	return 0; //I guess
}

//scheduled obsolete
static bool D3D9_get_to_work()
{
	if(D3D9_multithread)
	{
		if(!D3D9_thread) 
		{
			CreateThread(0,0,D3D9_worker,0,0,&D3D9_thread);
		}
		else if(!D3D9_fbuffer) 
		{
			D3D9_recoup(); //thread safe?
		}

		while(!D3D9_working&&D3D9_thread); //wait for it
	}
	else return D3D9_create();

	return D3D9_device; 
}

static const int D3D9_lost_status = -2;
static const wchar_t *D3D9_lost_message =
L"Direct3D9 device was lost. Failed to self-reset.";

class D3D9_Buffer;
 
namespace D3D9_cpp 
{		
	//forward declarations
	struct buffer; struct shader; 
	
	template<int N> struct block
	{			
		block(){ _active = false; } static const int N = N;
				
		inline operator bool()const{ return this?_active:false; }

		inline bool operator!()const{ return this?!_active:true; }

		inline bool operator=(bool b){ return _active = b; }

	private: bool _active; 
	};

	struct state
	{
		enum
		{				
			Z=1, BLEND, FRONT, TOTAL
		};

		int index, set[TOTAL]; //-1

		state(){ index = set[0] = 0; }					  

		~state(){ /*must do nothing*/ } //~D3D9_Buffer
		
		template<int N>	inline void enable(block<N> &rs)
		{
			if(rs) return; 			
			for(int i=0;i<TOTAL;i++) 
			{
				if(!set[i]){ set[i] = N; set[i+1] = 0; break; }
				
				if(set[i]==N){ assert(0); break; }
			}
			rs = true;
		}

		template<int N>	inline void disable(block<N> &rs)
		{
			if(!rs) return;			
			for(int i=0;i<TOTAL;i++) 
			{
				if(set[i]==N){ while(i++<TOTAL) set[i-1] = set[i]; break; }

				if(!set[i]){ assert(0); break; }
			}
			rs = false;
		}  		

		struct z : public block<Z>
		{	
			//D3DCMP_NEVER means D3DZB_FALSE
			D3DCMPFUNC function; BOOL write; float range[2];

			inline void operator()() //defaults
			{
				function=D3DCMP_NEVER; write=0; range[0]=0; range[1]=1;	
			}
			inline bool operator==(const z &cmp)const
			{
				if(!write||range[0]==cmp.range[0]&&range[1]==cmp.range[1])
				{
					return function==cmp.function&&write==cmp.write; 
				}
				else return false;
			}
			inline void apply(IDirect3DDevice9 *dev, z *cmp=0)const
			{				
				if(!*this||*cmp&&*cmp==*this) return; 
				
				if(function!=D3DCMP_NEVER)
				{	
					dev->SetRenderState(D3DRS_ZFUNC,function);
					dev->SetRenderState(D3DRS_ZWRITEENABLE,write);
										
					if(write) //hmm: docs are unclear but this looks like glDepthRange
					{
						D3DVIEWPORT9 vp; dev->GetViewport(&vp);	
						vp.MinZ = range[0]; vp.MaxZ = range[1]; dev->SetViewport(&vp);
					}

					dev->SetRenderState(D3DRS_ZENABLE,D3DZB_TRUE);
				}
				else dev->SetRenderState(D3DRS_ZENABLE,D3DZB_FALSE);

				if(cmp) *cmp = *this; 
			}

			z(){ operator()(); }
		}z;

		struct blend : public block<BLEND>
		{
			D3DBLEND src, dest;	
			
			D3DCOLOR factors; D3DBLENDOP op; //D3DRS_BLENDOPALPHA

			inline void operator()() //defaults
			{
				src=D3DBLEND_ONE; dest=D3DBLEND_ZERO; factors=~0; op=D3DBLENDOP_ADD; 
			}
			inline bool operator==(const blend &cmp)const
			{
				return src==cmp.src&&dest==cmp.dest&&factors==cmp.factors&&op==cmp.op; 
			}
			inline void apply(IDirect3DDevice9 *dev, blend *cmp=0)const
			{				
				if(!*this||*cmp&&*cmp==*this) return; 
				
				if(src!=D3DBLEND_ONE||dest!=D3DBLEND_ZERO)
				{	
					dev->SetRenderState(D3DRS_SRCBLEND,src);
					dev->SetRenderState(D3DRS_DESTBLEND,dest);
					//todo: query driver about these two
					dev->SetRenderState(D3DRS_BLENDOP,op);
					dev->SetRenderState(D3DRS_BLENDFACTOR,factors);
					dev->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);					
				}
				else dev->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);

				if(cmp) *cmp = *this; 
			}

			blend(){ operator()(); }

		}blend;		
		
		struct front : public block<FRONT>
		{	
			D3DCULL cull; //not much to this one

			inline void operator()() //defaults
			{
				cull=D3DCULL_NONE;
			}
			inline bool operator==(const front &cmp)const
			{
				return cull==cmp.cull;
			}
			inline void apply(IDirect3DDevice9 *dev, front *cmp=0)const
			{				
				if(!*this||*cmp&&*cmp==*this) return; 
				
				dev->SetRenderState(D3DRS_CULLMODE,cull);

				if(cmp) *cmp = *this; 
			}

			front(){ operator()(); }

		}front;

		void apply(IDirect3DDevice9 *dev, state *cmp=0)const
		{
			if(cmp&&cmp==this||!this) return;

			for(int i=0;i<TOTAL;i++) switch(set[i])
			{
			case 0: i = TOTAL; break; //end of the line

			case Z: z.apply(dev,cmp?&cmp->z:0); break;

			case BLEND: blend.apply(dev,cmp?&cmp->blend:0); break;
			case FRONT: front.apply(dev,cmp?&cmp->front:0); break;
			
			default: assert(0);
			}	  
		}
				
		template<class D3D9_Server>
		inline void apply(D3D9_Server &server)const
		{
			return apply(server.device,&server.state);
		}

		static const state reset;
		
		state(bool e) //hack: reset's ctor
		{
			index = set[0] = 0; if(!e) return;

			enable(z); enable(blend); enable(front);
		}
	};

	const state state::reset(true);
}

static void D3D9_server_reset(void*,int);
static void D3D9_simple_delete(void *key, int msg)
{
	//turns out it's infeasible to actually call this one??? 
	if(0&&msg==D3D9_cpp::DELETE) delete key; else assert(0);
}
static void D3D9_shader_delete(void*,int);
static void D3D9_buffer_delete(void*,int);

class D3D9_Server
:
public D3D9_cpp::state
,
public SOMPAINT_LIB(server)
{
/*hack: sharing device for now*/

static LONG active; //scheduled obsolete

public:	size_t vstride[2], vertex;
	
	static const size_t vdecl_s = 4;

	IDirect3DVertexDeclaration9 *vdecl[vdecl_s];	

	IDirect3DVertexBuffer9 *up; size_t up_s;

	IDirect3DDevice9 *device; //forward thinking

	IDirect3DDevice9 *operator->(){ return device; }

	inline operator IDirect3DDevice9*(){ return device; }
		
	bool scene; int foreground; //profile

	//should probably throw up a critical section around these
	void begin(){ if(!scene) device->BeginScene(); scene = true; }
	void end(){ if(scene) device->EndScene(); scene = false; }

	//// LEGEND //////////////////////////
	//	
	// up: upload buffer b_: dummy buffer
	// b0: the buffer(0) d_: depth buffer
	// rt: render target ds: depth-stencil 
	// vb: vertex buffer ib: index buffer
	// mt: multi-texture rs: render states 
	// vs: vertex shader ps: pixel shader
				
	IDirect3DTexture9 *b0_swapbuffer;
	IDirect3DSurface9 *b0_backbuffer, *b0_mirror;

	D3D9_cpp::shader *b0_swapbuffer_ps;
	D3D9_cpp::shader *b0_swapbuffer_ps_aaa;
	D3D9_cpp::shader *b0_swapbuffer_ps_bgr;
	D3D9_cpp::shader *b0_swapbuffer_vs;

	static const size_t mt_s = 1; //textures

	D3D9_Buffer *b0, *d_, *b_; 
	D3D9_Buffer *rt, *ds, *vb, *ib, *mt[mt_s]; 	

	D3D9_cpp::shader *vs, *ps; //per foreground

	D3D9_cpp::state &state, **rs; size_t rs_s; 

	D3D9_cpp::pool pool; //DELETEd on disconnect

	D3D9_Server() : state(*this)
	{			
		b0 = b_ = d_ = 0; device = 0;

		b0_backbuffer = 0; 
		b0_swapbuffer = 0; b0_mirror = 0;		
		b0_swapbuffer_ps = b0_swapbuffer_ps_aaa = 0;
		b0_swapbuffer_vs = b0_swapbuffer_ps_bgr = 0;

		scene = false; foreground = 0; 
				
		up = 0; up_s = vertex = 0; //vstride

		for(size_t i=0;i<vdecl_s;i++) vdecl[i] = 0;
	
		vs = ps = 0; rt = ds = vb = ib = mt[0] = 0; 		

		pool[&(*(rs=new D3D9_cpp::state*[rs_s=1])=0)] = D3D9_simple_delete;

		InterlockedIncrement(&active); //scheduled obsolete//
		
		D3D9_get_to_work();	device = D3D9_device; 

		device->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&b0_backbuffer);

		D3D9_cpp::section cs;
		D3D9_cpp::reset[this] = D3D9_server_reset; //??
	}				
	~D3D9_Server()
	{	
		D3D9_cpp::section cs; 
		D3D9_cpp::pool_it it; D3D9_cpp::reset.erase(this); 
		
		//each DELETE diminishes the pool
		while((it=pool.begin())!=pool.end()) 
		{				
			//hack: this is really undesirable
			if(it->second==D3D9_simple_delete) 
			{
				delete it->first; pool.erase(it->first); //needs more thought
			}
			else it->second(it->first,D3D9_cpp::DELETE);
		}
				
		D3D9_server_reset(this,D3D9_cpp::RELEASE); //hack

		//device->Release(); //scheduled obsolete//

		if(InterlockedDecrement(&active)==0) D3D9_release(); 
	}
	
	//// virtual methods ////

	SOMPAINT_PAL buffer(void**);

	bool share(void**, void**);	
	bool format2(void**, const char*,va_list);
	bool source(void**, const wchar_t[MAX_PATH]);
	bool expose2(void**, const char*,va_list);
	void discard(void**);

	void *lock(void**,const char*,size_t[4],int);
	void unlock(void**); 

	const wchar_t *assemble(const char*,size_t);

	bool run(const wchar_t*);	
	int load2(const char*,va_list);
	int load3(const char*,void**,size_t);

	void reclaim(const wchar_t*);

	void **define(const char*,const char*,void**);	

	const char *ifdef(const char*,void**);

	bool reset(const char*);
	bool frame(size_t[4]);
	bool clip(size_t[4]);

	/*portable operating systems APIs*/
	SOMPAINT_PIX raster(void**,void*,const char*);	
	SOMPAINT_BOX window(void**,void*,const char*);

	//protected
	void *disconnect(bool host); 
};

LONG D3D9_Server::active = 0;

class D3D9_Buffer
:
public SOMPAINT_LIB(buffer)
{
public: 
	
	union
	{
		IUnknown *unknown;

		template<int N, class I> struct rtti
		{
			IUnknown *i; int n;
						
			inline I *operator&()
			{
				if(i) i->Release(); assert(i==0);

				n = N; return (I*)&i;
			}

			inline I operator=(I j)
			{
				n = N; if(i==j) return j;

				if(i) i->Release(); assert(i==0);

				return (I&)i = j;
			}

			inline I operator->(){ return (I)i; }

			inline operator I()
			{
				return n==N?(I)i:0;
			}
		};
		
		rtti<0,IDirect3DSurface9*> surface;
		rtti<1,IDirect3DTexture9*> texture;
		rtti<2,IDirect3DIndexBuffer9*> ibuffer;
		rtti<3,IDirect3DVertexBuffer9*> vbuffer;
		rtti<4,IDirect3DSwapChain9*> xbuffer;
	};	 

	IDirect3DSurface9 *mirror; //present
		
	union //cannot have a dtor
	{			
		D3D9_cpp::state *state; //state buffers	 
		D3DSURFACE_DESC *reset; //render targets

		D3DPRESENT_PARAMETERS *xreset; //xbuffer		
	};

	D3D9_Buffer *planes; //circular

	//D3D9_Server::format
	Sompaint_hpp::keyword format; 
	
	union //format details
	{	
		int format_flags:32; 
													   
		struct 
		{	
			//canvas: basic buffer is a canvas
			//mipmap: basic buffer mipmaps were auto-generated
			//memory: basic memory specification
			//colour: basic buffer swizzle

			unsigned canvas:1, mipmap:1, memory:1, colour:2; 
				
			//x:
			//D3DPRESENT_BACK_BUFFERS_MAX    3
			//D3DPRESENT_BACK_BUFFERS_MAX_EX 30 
			
			unsigned x:5, z:1; //z: clearZ
		};
	};		
	int swizzle(int set=-1)
	{
		if(set!=-1)	switch(set)
		{
		default: colour = 0; assert(set==0); break;

		case 'aaa': colour = 1; break; case 'bgr': colour = 2; break;
		}
		switch(colour)
		{	
		case 1: return 'aaa'; case 2: return 'bgr';

		default: return 0; assert(colour==0); 
		}
	}

	union //format specific
	{	
		//depth-stencil
		struct{	float clearZ; int clear32:32; }; 

		//render-target
		struct{ float clearR, clearG, clearB, clearA; };

		//point buffer           //index buffer
		size_t bytes_per_vertex, bytes_per_index;
	};
		
	D3D9_Server &server; 
		
	D3D9_Buffer(D3D9_Server &s) : server(s) 
	{			
		unknown = 0; mirror = 0; planes = this; state = 0;

		format = Sompaint_hpp::UNRECOGNIZED; format_flags = 0;

		clearR = clearG = clearB = clearA = 0; //cosmetic
	}
	~D3D9_Buffer()
	{
		D3D9_cpp::section cs;
		D3D9_cpp::reset.erase(this); 
						
		for(size_t i=0;i<server.rs_s;i++)
		{
			if(server.rs[i]==state) server.rs[i] = 0;
		}
		delete state; //cannot have a dtor

		if(server.rt==this) server.rt = 0;
		if(server.ds==this) server.ds = 0;
		if(server.vb==this) server.vb = 0;
		if(server.ib==this) server.ib = 0;

		for(size_t i=0;i<server.mt_s;i++)
		{
			if(server.mt[i]==this) server.mt[i] = 0;
		}
		
		if(unknown) unknown->Release();

		if(mirror&&mirror->Release())
		{
			if(server.b0_mirror==mirror) 
			{
				server.b0_mirror = 0; mirror->Release(); 
			}
			else assert(0); 
		}

		D3D9_Buffer *swap = planes; planes = 0;

		if(swap->planes) delete swap;
	}

	//// virtual methods ////

	bool apply(int);
	bool setup(int);
	bool clear(int);
	bool sample(int);
	bool layout(const wchar_t*);
	int stream(int,const void*,size_t); 
	bool draw(int,int,int,int); 

	bool present(int,SOMPAINT_BOX,SOMPAINT_BOX,SOMPAINT_PIX);
	bool fill(int,int,int,int,SOMPAINT_BOX,SOMPAINT_BOX,SOMPAINT_PIX);
	bool print(int,int,SOMPAINT_PIX);
};

namespace D3D9_cpp
{
	struct share //abstract
	{			
		void **lock; //todo: sharing

		Sompaint_hpp::keyword spec; //buffer, window, raster

	protected: share(){}
	};

	struct buffer : public share 
	{
		D3D9_Buffer pal;
				
		buffer(D3D9_Server &server, void **io) : pal(server)
		{
			if(io) *io = this; lock = io; spec = Sompaint_hpp::buffer; 

			D3D9_cpp::section cs; pal.server.pool[this] = D3D9_buffer_delete;
		}
		~buffer()
		{
			D3D9_cpp::section cs; pal.server.pool.erase(this);
		}

		inline operator D3D9_Buffer*()
		{
			assert(this&&spec==Sompaint_hpp::buffer);

			return &pal; 
		}
		inline D3D9_Buffer *operator->()
		{
			return &pal; 
		}
	};
}

namespace D3D9_cpp 
{		
	typedef SOMPAINT_LIB(raster) raster;
	typedef SOMPAINT_LIB(window) window;
}

//opaque
struct SOMPAINT_LIB(raster) : public D3D9_cpp::share
{
	int type; union{ void *_; HWND *_1; HBITMAP *_2; };

	inline void operator=(void *p){ if(this){ _ = p; } }

	inline HWND *hwnd(){ return this&&type==1?_1:0; }

	inline void operator=(HWND *p){ if(this){ type = 1; _1 = p; } }	  	

	inline bool operator==(HWND *p){ return this&&type==1&&p==_1||!this&&!p; }

	inline HBITMAP *hbitmap(){ return this&&type==2?_2:0; }	

	inline void operator=(HBITMAP *p){ if(this){ type = 2; _2 = p; } }	

	inline bool operator==(HBITMAP *p){ return this&&type==2&&p==_2||!this&&!p; }
	
	inline operator SOMPAINT_PIX(){ return this&&type&&_?this:0; }

	D3D9_Server &server;

	SOMPAINT_LIB(raster)(D3D9_Server &s, void **io) : server(s)
	{
		type = 0; _1 = 0; 
		
		if(io) *io = this; lock = io; spec = Sompaint_hpp::raster;

		D3D9_cpp::section cs; server.pool[this] = D3D9_simple_delete;
	}	  
	~SOMPAINT_LIB(raster)()
	{
		D3D9_cpp::section cs; server.pool.erase(this);
	}
};

//opaque
struct SOMPAINT_LIB(window) : public D3D9_cpp::share
{
	int type; union{ void *_; RECT *_1; };

	inline void operator=(void *p){ if(this){ _ = p; } }

	inline void operator=(RECT *p){ if(this){ type = 1; _1 = p; } }

	inline bool operator==(RECT *p){ return this&&type==1&&p==_1||!this&&!p; }

	inline RECT *rect(){ return this&&type==1?_1:0; }

	inline operator SOMPAINT_BOX(){ return this&&type&&_?this:0; }

	D3D9_Server &server;

	SOMPAINT_LIB(window)(D3D9_Server &s, void **io) : server(s)
	{
		type = 0; _1 = 0; 
		
		if(io) *io = this; lock = io; spec = Sompaint_hpp::window;

		D3D9_cpp::section cs; server.pool[this] = D3D9_simple_delete;
	}	
	~SOMPAINT_LIB(window)()
	{
		D3D9_cpp::section cs; server.pool.erase(this);
	}
};

namespace D3D9_cpp
{
	struct handle
	{
		const wchar_t errors;

		int profile; //0, 'ps', 'vs'

		handle(int p):errors(0){ profile = p; }
	};
		 
	struct shader : public handle
	{		
		D3D9_Server &server;

		ID3DXBuffer *bytecode;

		union
		{
			IUnknown *unknown;			
			IDirect3DVertexShader9 *vshader;
			IDirect3DPixelShader9 *pshader;		
		};		

		shader(int p, D3D9_Server *s) :	handle(p), server(*s)
		{
			bytecode = 0; unknown = 0; 

			D3D9_cpp::section cs; server.pool[this] = D3D9_shader_delete; 
		}  
		~shader()
		{	
			D3D9_cpp::section cs; server.pool.erase(this);

			if(bytecode) bytecode->Release(); 
			if(unknown) unknown->Release(); 
		}	  
	};
}

template<int N>
static D3D9_cpp::shader *D3D9_shader(D3D9_Server &server, const char (&code)[N])
{
	const wchar_t *out = server.assemble(code,N); 
	
	if(out&&!*out) return (D3D9_cpp::shader*)out; assert(0);

	server.reclaim(out); return 0; 
}

//callbacks
static void D3D9_buffer_delete(void *key, int msg)
{
	if(msg==D3D9_cpp::DELETE) delete (D3D9_cpp::buffer*)key; else assert(0);
}
static void D3D9_shader_delete(void *key, int msg)
{ 
	if(msg==D3D9_cpp::DELETE) delete (D3D9_cpp::shader*)key; else assert(0);
}

//callback
static void D3D9_server_reset(void *key, int msg)
{
	D3D9_Server &server = *(D3D9_Server*)key; 
	
	if(msg==D3D9_cpp::RELEASE)
	{
		server.b0_backbuffer->Release(); server.b0_backbuffer = 0;
		
		if(server.b0_mirror) server.b0_mirror->Release(); server.b0_mirror =  0;

		if(server.b0_swapbuffer) server.b0_swapbuffer->Release(); server.b0_swapbuffer = 0;				 
				
		server.up_s = 0; if(server.up) server.up->Release(); server.up = 0; //upload buffer 		

	////not good enough: assuming client will just happen to reset these for now////

		server.vb = server.ib = server.mt[0] = 0; 
	}
	else if(msg==D3D9_cpp::REBUILD)
	{
		server.device->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&server.b0_backbuffer);

		for(size_t i=0;i<server.rs_s;i++) if(server.rs[i]) server.rs[i]->apply(server,0);
	}
	else assert(0);
}

//callback
static void D3D9_shader_reset(void *key, int msg)
{
	D3D9_cpp::shader *sh = (D3D9_cpp::shader*)key; 
	
	if(msg==D3D9_cpp::RELEASE)
	{
		if(sh->unknown) sh->unknown->Release(); sh->unknown = 0;
	}
	else if(msg==D3D9_cpp::REBUILD)
	{
		DWORD *bc = (DWORD*)sh->bytecode->GetBufferPointer();

		switch(sh->profile) 
		{
		case 'vs': sh->server->CreateVertexShader(bc,&sh->vshader); break;
		case 'ps': sh->server->CreatePixelShader(bc,&sh->pshader); break;
		}

		if(sh==sh->server.vs) sh->server->SetVertexShader(sh->vshader); 
		if(sh==sh->server.ps) sh->server->SetPixelShader(sh->pshader); 
	}
	else assert(0);
}

//callback
static void D3D9_buffer_reset(void *key, int msg)
{
	D3D9_Buffer *b = (D3D9_Buffer*)key; 
	
	if(msg==D3D9_cpp::RELEASE) 
	{
		if(b->unknown) b->unknown->Release(); b->unknown = 0;
	}
	else if(msg==D3D9_cpp::REBUILD)
	{	
		HRESULT hr; D3DFORMAT f = b->reset->Format;

		size_t w = b->reset->Width, h = b->reset->Height;
		
		if(b->surface) 
		{
			if(b->reset->Usage==D3DUSAGE_DEPTHSTENCIL)			
			{
				hr = b->server->
				CreateDepthStencilSurface(w,h,f,D3DMULTISAMPLE_NONE,0,true,&b->surface,0);
						
				if(b->server.ds==b) //repair depth buffer
				{
					if(b->server->SetDepthStencilSurface(b->surface)!=D3D_OK) assert(0);
				}
			}
			else if(b->reset->Usage==D3DUSAGE_RENDERTARGET)
			{
				hr = b->server->
				CreateRenderTarget(w,h,f,D3DMULTISAMPLE_NONE,0,false,&b->surface,0);
						
				if(b->server.rt==b) //repair render target
				{
					if(b->server->SetRenderTarget(0,b->surface)!=D3D_OK) assert(0);
				}
			}
			else assert(0);
		}
		else if(b->texture) //canvas
		{
			hr = b->server->
			CreateTexture(w,h,1,D3DUSAGE_RENDERTARGET,f,b->reset->Pool,&b->texture,0);

			if(b->server.rt==b) //repair render target
			{
				IDirect3DSurface9 *top = 0;

				if(b->texture->GetSurfaceLevel(0,&top)==D3D_OK)
				{
					if(b->server->SetRenderTarget(0,top)!=D3D_OK) assert(0);
					
					top->Release();
				}
				else assert(0);
			}
		}
		else assert(0);
	}
	else assert(0);
}

static bool D3D9_vdecl(D3D9_Server &server, const wchar_t *string)
{		
	size_t vertex = server.vdecl_s;
	size_t compile[sizeof(long long)==8&&sizeof(wchar_t)==2];

	//assuming Little Endian is ok (Direct3D is Windows afterall)
	const long long MDO = long long('M'|'D'<<16)|long long('O')<<32;
	const long long MDL = long long('M'|'D'<<16)|long long('L')<<32;
	const long long MPX = long long('M'|'P'<<16)|long long('X')<<32;

	switch(*(long long*)string)
	{
	case MDO: vertex = 1; break;
	case MDL: vertex = 2; break;
	case MPX: vertex = 3; break;

	default: assert(0); return false;
	}

	if(vertex<=0||vertex>=server.vdecl_s) return 0;

	if(server.vdecl[vertex]) 
	{		
		HRESULT hr = D3D_OK;

		if(server.vertex!=vertex)
		{				
			switch(vertex)
			{
			case 1: server.vstride[0] = 32; break; //MDO
			case 2: server.vstride[0] = 20; break; //MDL
			case 3: server.vstride[0] = 26; break; //MPX

			default: assert(0); 
			}

			server.vstride[1] = 80; //upload stream

			hr = server->SetVertexDeclaration(server.vdecl[server.vertex=vertex]);
		}

		return hr==D3D_OK;
	}

	if(*string!='M') return 0; //expecting MDO, MDL, or MPX

#define SOMPALD3D9_INSTANCE_DECL \
		/*world matrix*/\
		{1,  0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},\
		{1, 16, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2},\
		{1, 32, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 3},\
		{1, 48, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 4},\
		/*opacity, tween, reserved*/\
		{1, 64, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 5},\
		/*tween index (3 reserved)*/\
		{1, 76, D3DDECLTYPE_UBYTE4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0},

	if(!wcscmp(string,L"MDO")) //documented in Somgraphic.h
	{
		D3DVERTEXELEMENT9 mdo[] = {
		{0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		{0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0},
		{0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
		SOMPALD3D9_INSTANCE_DECL
		D3DDECL_END()};

		server->CreateVertexDeclaration(mdo,&server.vdecl[vertex]);
	}
	else if(!wcscmp(string,L"MDL")) //documented in Somgraphic.h
	{
		D3DVERTEXELEMENT9 mdl[] = {
		{0,  0, D3DDECLTYPE_SHORT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		{0,  8, D3DDECLTYPE_SHORT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0},
		{0, 16, D3DDECLTYPE_UBYTE4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
		SOMPALD3D9_INSTANCE_DECL
		D3DDECL_END()};

		server->CreateVertexDeclaration(mdl,&server.vdecl[vertex]);
	}
	else if(!wcscmp(string,L"MPX"))
	{
		assert(0); //unimplemented
	}
	else assert(0);

	if(!server.vdecl[vertex]) return false;

	return D3D9_vdecl(server,string);
}


static bool D3D9_upload(D3D9_Server &server, const void *up, int up_s, int n, int stride)
{
	void *out = 0; if(!n) return true;

	int s =  n*stride, up_stride = up_s/n;

	if(!up||!up_s||up_s%n||up_stride>stride||!stride)
	{
		assert(0); return false;
	}

	if(server.up_s<s||!server.up)
	{
		if(server.up) server.up->Release(); server.up = 0;

		server.up_s = std::max<size_t>(server.up_s*2,s>4096?s:4096);

		server->CreateVertexBuffer(server.up_s,D3DUSAGE_DYNAMIC,0,D3DPOOL_DEFAULT,&server.up,0);
	}

	if(server.up)
	if(server.up->Lock(0,s,&out,D3DLOCK_DISCARD)==D3D_OK)
	{
		if(up_s!=n*stride)
		{
			for(int i=0;i<n;i++)
			{
				memcpy((char*)out+stride*i,(char*)up+up_stride*i,up_stride); 
			}
		}
		else memcpy(out,up,up_s); 
		
		server.up->Unlock();
	}
	else assert(0); 
	
	return out;
}

#undef D3D9_BADSTATUS
#define D3D9_BADSTATUS (status<0) 

SOMPAINT_PAL D3D9_Server::buffer(void **io)
{
	/*Reminder: buffer cannot return 0*/

	if(!io) //buffer(0)
	{
		//WARNING:
		//As long as we are sharing a single 
		//device, these are linked together!
		//assert(active==1);

		if(!b0)
		{						 
			b0 = *new D3D9_cpp::buffer(*this,0); 
			
			device->GetSwapChain(0,&b0->xbuffer);

			b0->xreset = new D3DPRESENT_PARAMETERS;

			HRESULT paranoia = 
			b0->xbuffer->GetPresentParameters(b0->xreset);	 
			assert(paranoia==D3D_OK);

			b0->format = Sompaint_hpp::frame;
		}

		return b0;
	}		
	
	D3D9_cpp::buffer *p = (D3D9_cpp::buffer*)*io;

	if(p) return *p; //looking for a fast out

	if(!b_) b_ = *new D3D9_cpp::buffer(*this,0); //dummy 

	return b_;
}

bool D3D9_Server::share(void**, void**)
{
	assert(0); return false; //unimplemented
}

//subroutines
static bool D3D9_basic_buffer(D3D9_Buffer*,Sompaint_hpp::format&,bool);
static bool D3D9_frame_buffer(D3D9_Buffer*,Sompaint_hpp::format&,bool);
static bool D3D9_point_buffer(D3D9_Buffer*,Sompaint_hpp::format&,bool);
static bool D3D9_index_buffer(D3D9_Buffer*,Sompaint_hpp::format&,bool);
static bool D3D9_state_buffer(D3D9_Buffer*,Sompaint_hpp::format&,bool);

bool D3D9_Server::format2(void **io, const char *f, va_list v)
{	
	if(D3D9_BADSTATUS) return false;

	Sompaint_hpp::format pp(f,v); //preprocessor

	static bool once = Sompaint_hpp::coin(pp.key); 
	
	bool closure = pp++; 
		
	D3D9_cpp::share *p = io?(D3D9_cpp::share*)*io:0; 

	if(io) switch(pp.spec) 
	{
	case Sompaint_hpp::buffer:
	case Sompaint_hpp::memory: discard(io); 
		
		p = new D3D9_cpp::buffer(*this,io); break;

	case Sompaint_hpp::UNSPECIFIED: assert(p);
		
		if(!p) return false; break;

	default: assert(0); return false; 
	}
	else if(pp.spec!=Sompaint_hpp::UNSPECIFIED) 
	{
		assert(0); return false; 
	}
	else if(!b0) buffer(0);
	
	switch(io?p->spec:Sompaint_hpp::buffer)
	{
	case Sompaint_hpp::buffer: case Sompaint_hpp::memory:
	{	
		D3D9_Buffer *q = !io?b0:*(D3D9_cpp::buffer*)p;

		switch(pp.spec==Sompaint_hpp::UNSPECIFIED?q->format:pp.left)
		{
		case Sompaint_hpp::basic: return D3D9_basic_buffer(q,pp,closure);
		case Sompaint_hpp::frame: return D3D9_frame_buffer(q,pp,closure);
		case Sompaint_hpp::point: return D3D9_point_buffer(q,pp,closure);
		case Sompaint_hpp::index: return D3D9_index_buffer(q,pp,closure);
		case Sompaint_hpp::state: return D3D9_state_buffer(q,pp,closure);
		}
		assert(0); break;
	}
	default: assert(0); 
	}
	
	return false;
}

#define BEGIN_FORMAT int out = 0; \
\
	keep_going: bool error = false;\
\
	using namespace Sompaint_hpp;\
\
	switch(pp.lvalue)\
	{
#define FORMAT_E() { error = true; assert(0); }
#define FORMAT_EIF(test) if(test) FORMAT_E()
#define BEGIN_FORMAT_F(max) \
	{\
		int compile[max];\
		for(int arg=0;1;arg++,closure=pp++)\
		{\
			/*the first argument should always go thru*/\
			if(arg>=max) assert(0); else if(pp!=""||!arg)\
			{
#define END_FORMAT_F \
			}\
			if(closure||!*pp) break; }\
		}
#define END_FORMAT default: error = *pp;\
	}\
\
	if(!error) out++;\
\
	assert(closure||!*pp);\
\
	while(!closure&&*pp) closure = pp++;\
\
	if(*pp){ closure = pp++; goto keep_going; }\
\
	if(!out) return false; //assuming desirable

//no return
static bool D3D9_basic_buffer(D3D9_Buffer *b, Sompaint_hpp::format &pp, bool closure)
{		
	D3DSURFACE_DESC create; 
	
	int mipmaps = 1, lod = 0, bpp = 0;
	
	if(b->unknown)
	{			
		if(b->texture)
		{
			lod = b->texture->GetLOD();

			mipmaps = b->mipmap?0:b->texture->GetLevelCount();

			if(lod>mipmaps) lod = 0; //error code

			if(b->texture->GetLevelDesc(0,&create)!=D3D_OK) return false;		
		}
		else //unimplemented??
		{
			assert(0); return false;
		}
	}
	else memset(&create,0x00,sizeof(create)); //good enough??
	
	bool recreate = !b->texture;

	if(pp.spec!=Sompaint_hpp::UNSPECIFIED) //hack
	{
		if(b->memory!=(pp.spec==Sompaint_hpp::memory)) recreate = true;

		b->memory = pp.spec==Sompaint_hpp::memory;
	}

	BEGIN_FORMAT
//	{
	case _1d:
	BEGIN_FORMAT_F(2)

		int i = pp; 
		
		if(pp.rvalue==_1d) switch(arg)
		{
		case 0: 

			if(create.Height!=1)
			{
				create.Height = 1; recreate = true;
			}
			if(create.Width!=i)
			{
				create.Width = i; recreate = true;
			}
			break;

		case 1:	bpp = i; break;
		}
		else FORMAT_E()			

	END_FORMAT_F
	break;
	case _2d: 
	BEGIN_FORMAT_F(3)
		
		int i = pp; 
		
		if(pp.rvalue==_2d) switch(arg)
		{
		case 0: 

			if(create.Width!=i)
			{
				create.Width = i; recreate = true;

				if(!create.Height) create.Height = i;
			}
			break;

		case 1:

			if(create.Height!=i)
			{
				create.Height = i; recreate = true;
			}
			break;

		case 3:	bpp = i; break;
		}
		else FORMAT_E()
	
	END_FORMAT_F
	break;
	case mipmap:
	BEGIN_FORMAT_F(2)
				
		if(pp.rvalue==_0)
		{
			b->mipmap = false;

			lod = 0; mipmaps = 1; 
		}
		else if(pp.rvalue==mipmap) switch(arg)
		{
		case 0: lod = mipmaps = 0; b->mipmap = true;

			if(pp==UNRECOGNIZED) lod = pp; else FORMAT_E() break;		

		case 1:	if(pp==UNRECOGNIZED) mipmaps = pp; FORMAT_EIF(mipmaps<1);
			
			b->mipmap = false; break;
		}
		else FORMAT_E()

	END_FORMAT_F
	break;
	case canvas:
	BEGIN_FORMAT_F(1)

		b->canvas = false;

		create.Usage&=~D3DUSAGE_RENDERTARGET;

		if(pp.rvalue==canvas)
		{
			b->canvas = true;

			create.Usage|=D3DUSAGE_RENDERTARGET; 
		}	 
		else FORMAT_EIF(pp.rvalue!=_0)			
	
	END_FORMAT_F
	break;
	case rgb: 
	case rgba:
	BEGIN_FORMAT_F(1)

		D3DFORMAT f = D3DFMT_UNKNOWN;

		switch(pp.rvalue)
		{
		default: b->swizzle(0); break;

		case aaa: b->swizzle('aaa'); break;
		case bgr: b->swizzle('bgr'); break;
		}  
		switch(b->swizzle()?rgba:pp.lvalue)
		{		
		case rgb: f = D3DFMT_X8R8G8B8; break; 			
		case rgba: f = D3DFMT_A8R8G8B8; break;
		}  
		if(b->memory&&pp.lvalue==rgb) //hack
		{
			f = D3DFMT_A8R8G8B8; //b0_swapbuffer
		}
		if(f==D3DFMT_X8R8G8B8||f==D3DFMT_A8R8G8B8)
		{
			int i = pp; //colour plane bits
			
			FORMAT_EIF(i&&i!=32||bpp&&bpp!=32)

			if((!i||i==32)&&(!bpp||bpp==32)) 
			{
				if(create.Format!=f) recreate = true;

				create.Format = f;
			}	
			else FORMAT_E()
		}
		else FORMAT_E()

	END_FORMAT_F
	break;
//	}
	END_FORMAT

	if(recreate)
	{
		int w = create.Width?create.Width:1;
		int h = create.Height?create.Height:1;

		if(bpp&&bpp!=32){ assert(0); return false; }

		D3DFORMAT f = create.Format?create.Format:D3DFMT_A8R8G8B8;

		D3DPOOL pool = b->canvas?D3DPOOL_DEFAULT:D3DPOOL_MANAGED;
			
		if(b->memory){ pool = D3DPOOL_SYSTEMMEM; assert(!b->canvas); } //software canvas???
			
		if(b->mirror) b->mirror->Release(); b->mirror = 0; 
		if(b->unknown) b->unknown->Release(); b->unknown = 0; 
		
		assert(pp.left!=UNSPECIFIED); //TODO: preserve contents

		if(b->memory){ b->mipmap = false; assert(mipmaps==1); }
 
		if(b->mipmap){ create.Usage|=D3DUSAGE_AUTOGENMIPMAP; assert(mipmaps==0); }

		out = !b->server->CreateTexture(w,h,mipmaps,create.Usage,f,pool,&b->texture,0);		

		assert(out);
	}
	else assert(b->unknown);

	if(b->texture) b->texture->SetLOD(lod); //assuming trivial

	b->format = basic;

	return out;
}

//no return
static bool D3D9_frame_buffer(D3D9_Buffer *b, Sompaint_hpp::format &pp, bool closure)
{				
	D3DPRESENT_PARAMETERS create;
	
	D3DSURFACE_DESC create2 = { D3DFMT_UNKNOWN };

	D3D9_Buffer *z = 0;	int bpp = 0;

	if(b->unknown)
	{
		if(b->surface)
		{
			if(b->surface->GetDesc(&create2)!=D3D_OK) return false;
			
			create.BackBufferWidth = create2.Width; 
			create.BackBufferHeight = create2.Height;
			create.BackBufferFormat = create2.Format;
			create.BackBufferCount = 0;
			create.MultiSampleType = create2.MultiSampleType;
			create.MultiSampleQuality = create2.MultiSampleQuality;			

			if(create2.Usage&D3DUSAGE_DEPTHSTENCIL) z = b;
		}
		else if(b->xbuffer)
		{
			if(b->xbuffer->GetPresentParameters(&create)!=D3D_OK) return false;		

			if(b->planes!=b) z = b->planes; //hack: assuming depth-stencil surface

			if(z&&z->surface->GetDesc(&create2)!=D3D_OK) return false;
		}
		else //unimplemented??
		{
			assert(0); return false;
		}
	}	
	else memset(&create,0x00,sizeof(create)); 

	bool recreate = !b->unknown;
		
	if(pp.spec!=Sompaint_hpp::UNSPECIFIED) //hack
	{
		if(pp.spec!=Sompaint_hpp::buffer){ assert(0); return false; }
	}

	BEGIN_FORMAT
//	{
	case _2d: 
	BEGIN_FORMAT_F(3)
		
		int i = pp; 
		
		if(pp.rvalue==_2d) switch(arg)
		{
		case 0: 

			if(create.BackBufferWidth!=i)
			{
				create.BackBufferWidth = i; recreate = true;

				if(!create.BackBufferHeight) create.BackBufferHeight = i;
			}
			break;

		case 1:

			if(create.BackBufferHeight!=i)
			{
				create.BackBufferHeight = i; recreate = true;
			}
			break;

		case 3:	bpp = i; break;
		}
		else FORMAT_E()
	
	END_FORMAT_F
	break;
	case clear: 
	BEGIN_FORMAT_F(4) //todo: 5+
				
		if(pp.rvalue==clear) 
		{				
			if(arg==0) //defaults
			{
				b->clearR=b->clearG=b->clearB=b->clearA=0;
			}

			if(pp==_ellipsis) //...
			{
				if(arg>=1)
				{
					float *rgba = &b->clearR;

					for(int i=arg;i<4;i++) rgba[i] = rgba[i-1];
				}
				else FORMAT_E()
			}
			else //assuming a number
			{
				float f = pp; 

				switch(arg)
				{
				case 0: b->clearR = f; break;
				case 1: b->clearG = f; break;
				case 2: b->clearB = f; break;
				case 3: b->clearA = f; break;

				default: assert(0); 
				}
			}
		}
		else FORMAT_E()

	END_FORMAT_F
	break;
	case rgb: 
	case rgba:
	BEGIN_FORMAT_F(1)

		D3DFORMAT f = D3DFMT_UNKNOWN;
	
		switch(pp.lvalue)
		{		
		case rgb: f = D3DFMT_X8R8G8B8; break; 			
		case rgba: f = D3DFMT_A8R8G8B8; break;
		}  		
		if(f==D3DFMT_X8R8G8B8||f==D3DFMT_A8R8G8B8)
		{
			int i = pp; //colour plane bits
			
			FORMAT_EIF(i&&i!=32||bpp&&bpp!=32)

			if((!i||i==32)&&(!bpp||bpp==32)) 
			{
				if(create.BackBufferFormat!=f) recreate = true;

				create.BackBufferFormat = f;
			}	
			else FORMAT_E()
		}
		else FORMAT_E()

	END_FORMAT_F
	break;
	case depth:
	{
		assert(0); return false; //unimplemented				

	}break;
	case stencil:
	{
		assert(0); return false; //unimplemented				

	}break;
	case x:
	BEGIN_FORMAT_F(1)
			
		int i = pp;

		if(!i||pp.rvalue==_0)
		{
			if(create.BackBufferCount) recreate = true;

			create.BackBufferCount = 0;
		}
		else if(pp.rvalue==x)
		{			
			if(i!=create.BackBufferCount)
			{
				assert(0); //was b->x set?

				if(x!=b->x) recreate = true;
			}

			create.BackBufferCount = i;
		}

	END_FORMAT_F
	break;
//	}	
	END_FORMAT

	if(recreate)
	{			
		int w = create.BackBufferWidth?create.BackBufferWidth:1;
		int h = create.BackBufferHeight?create.BackBufferHeight:1;

		if(bpp&&bpp!=32){ assert(0); return false; } //32bit colour

		D3DFORMAT f = create.BackBufferFormat?create.BackBufferFormat:D3DFMT_X8R8G8B8;

		D3DMULTISAMPLE_TYPE s = create.MultiSampleType;	DWORD q = create.MultiSampleQuality;

		if(b->mirror) b->mirror->Release(); b->mirror = 0; 
		if(b->unknown) b->unknown->Release(); b->unknown = 0; 
		
		assert(pp.left!=UNSPECIFIED); //TODO: preserve contents
				
		if(create.BackBufferCount==0) //render target
		{
			out = !b->server->CreateRenderTarget(w,h,f,s,q,0,&b->surface,0);		
		}
		else //swap chain (or depth stencil maybe)
		{
			assert(0); return false; //unimplemented
		}		

		if(out)
		{
			if(!b->reset) b->xreset = new D3DPRESENT_PARAMETERS; //union

			if(create.BackBufferCount) //render target
			{
				if(b->xbuffer->GetPresentParameters(b->xreset)!=D3D_OK) assert(0);				
			}
			else if(b->surface->GetDesc(b->reset)!=D3D_OK) assert(0);
		}
		else assert(0);
	}
	else assert(b->unknown);

	b->format = frame; assert(!z);

	return out;
}

//no return
static bool D3D9_point_buffer(D3D9_Buffer *b, Sompaint_hpp::format &pp, bool closure)
{
	D3DVERTEXBUFFER_DESC create;

	int bpp = 0, n = 0; //lookout below 

	if(b->vbuffer)
	{
		if(b->vbuffer->GetDesc(&create)==D3D_OK) 
		{
			if(create.Size%b->bytes_per_vertex==0)
			{
				n = create.Size/b->bytes_per_vertex;
			}
			else assert(0);
		}
		else return false;
	}	
	else memset(&create,0x00,sizeof(create)); 

	bool recreate = !b->unknown;
	
	if(pp.spec!=Sompaint_hpp::UNSPECIFIED) //hack
	{
		if(pp.spec!=Sompaint_hpp::buffer){ assert(0); return false; }
	}
	else bpp = b->bytes_per_vertex*8; //!
	
	BEGIN_FORMAT
//	{
	case _1d: 
	BEGIN_FORMAT_F(2)

		int i = pp; 
		
		if(pp.rvalue==_1d) switch(arg)
		{
		case 0: n = i;

			if(i*bpp!=8*create.Size) recreate = true;

		break;
		case 1:	bpp = i;

			if(n*bpp!=8*create.Size) recreate = true;

		break;
		}
		else FORMAT_E()			

	END_FORMAT_F
	break;	
//	}	
	END_FORMAT
	
	if(recreate)
	{		
		if(!bpp) bpp = 8; if(bpp%8){ assert(0); return false; }

		b->bytes_per_vertex = bpp/8; create.Size = n*b->bytes_per_vertex;

		out = !b->server->CreateVertexBuffer(create.Size,0,0,D3DPOOL_DEFAULT,&b->vbuffer,0);

		assert(out);
	}
	else assert(b->unknown);

	b->format = point;

	return out;
}

//no return
static bool D3D9_index_buffer(D3D9_Buffer *b, Sompaint_hpp::format &pp, bool closure)
{
	D3DINDEXBUFFER_DESC create;

	int bpp = 0, n = 0; //lookout below

	if(b->ibuffer)
	{
		if(b->ibuffer->GetDesc(&create)==D3D_OK) 
		{
			if(create.Size%b->bytes_per_index==0)
			{
				n = create.Size/b->bytes_per_index;
			}
			else assert(0);
		}
		else return false;
	}	
	else memset(&create,0x00,sizeof(create)); 

	bool recreate = !b->unknown;
	
	if(pp.spec!=Sompaint_hpp::UNSPECIFIED) //hack
	{
		if(pp.spec!=Sompaint_hpp::buffer){ assert(0); return false; }
	}
	else bpp = b->bytes_per_vertex*8; //!
	
	BEGIN_FORMAT
//	{
	case _1d: 
	BEGIN_FORMAT_F(2)

		int i = pp; 
		
		if(pp.rvalue==_1d) switch(arg)
		{
		case 0: n = i;

			if(i*bpp!=8*create.Size) recreate = true;

		break;
		case 1:	bpp = i;

			if(n*bpp!=8*create.Size) recreate = true;

		break;
		}
		else FORMAT_E()			

	END_FORMAT_F
	break;	
//	}	
	END_FORMAT
	
	if(recreate)
	{			
		D3DFORMAT f = bpp&&bpp<=16?D3DFMT_INDEX16:D3DFMT_INDEX32;

		if(!bpp) bpp = 32; if(bpp%8||bpp>32){ assert(0); return false; }

		b->bytes_per_index = f==D3DFMT_INDEX16?2:4; create.Size = n*b->bytes_per_index;		

		out = !b->server->CreateIndexBuffer(create.Size,0,f,D3DPOOL_DEFAULT,&b->ibuffer,0);

		assert(out);
	}
	else assert(b->unknown);

	b->format = index;

	return out;
}

//no return
static bool D3D9_state_buffer(D3D9_Buffer *b, Sompaint_hpp::format &pp, bool closure)
{
	using namespace Sompaint_hpp; 

	if(!b->state) b->state = new D3D9_cpp::state;

	bool apply = b->server.rs[b->state->index]==b->state;

	#define BEGIN(rs)\
	case rs:{ struct D3D9_cpp::state::rs &st = b->state->rs; st();

	#define END(rs)\
	if(pp.rvalue!=_0)\
	{\
		b->state->enable(st); if(apply) st.apply(b->server,&b->server.rs);\
	}\
	else b->state->disable(st); }break;

	BEGIN_FORMAT
//	{
	BEGIN(z)
	BEGIN_FORMAT_F(2)
	
		switch(pp.rvalue)
		{
		case _0:
		case reset: break;

		default: 

			switch(arg)
			{
			case 0: 

				st.range[0] = pp;

				switch(pp.rvalue)
				{	
				case z: st.function = D3DCMP_LESS; break;
								
				case range: st.function = D3DCMP_ALWAYS; break; 

				case decal: st.function = D3DCMP_LESSEQUAL;	break;
		
				default: FORMAT_E() 
				}

			break;
			case 1: 

				st.range[1] = pp;
				
				st.write = TRUE;

			break;
			}
		}

	END_FORMAT_F
	END(z)
	BEGIN(blend)
	BEGIN_FORMAT_F(2)
	
		int i = pp; 

		switch(pp.rvalue)
		{
		case _0:
		case reset: break;

		case blend:	FORMAT_EIF(i&~1)

			if(pp!="") //default args are 1,1
			{
				(arg==0?st.src:st.dest) = (i?D3DBLEND_ONE:D3DBLEND_ZERO);
			}
			else (arg==0?st.src:st.dest) =  D3DBLEND_ONE; 

		break;
		case mdo: FORMAT_EIF(arg!=0)

			switch(i)
			{
			case 0: //this one blends whenever the material is transparent 
												
				st.src = D3DBLEND_SRCALPHA; st.dest = D3DBLEND_INVSRCALPHA; break;

			case 1: //this one blends whether material is transparent or not
				
				st.src = st.dest = D3DBLEND_ONE; break; //guessing: see i018.mdl

				//TODO: some one needs to try other number with Sword of Moonlight

			default: FORMAT_E() 
			}
			
		break;
		case mdl: FORMAT_EIF(arg!=0)
					  
			//WARNING: assuming factors and ops onboard!!
			
			switch(i) //PlayStation
			{
			case 0:	//00  50%back + 50%polygon
				
				st.src = st.dest = D3DBLEND_BLENDFACTOR; st.factors = 0xFF808080; break;

			case 1: //01 100%back + 100%polygon
				
				st.src = st.dest = D3DBLEND_ONE; break;

			case 2: //10 100%back - 100%polygon  
				
				st.src = st.dest = D3DBLEND_ONE; st.op = D3DBLENDOP_REVSUBTRACT; break;

			case 3: //11 100%back + 25%polygon 
				
				st.src = D3DBLEND_BLENDFACTOR; st.dest = D3DBLEND_ONE; st.factors = 0xFF404040; break;

			default: //(there are only 2 bits)
				
				FORMAT_E() 
			}

		break;
		default: FORMAT_E()
		}

	END_FORMAT_F
	END(blend)
	BEGIN(front)
	BEGIN_FORMAT_F(1) //0
	
		switch(pp.rvalue)
		{
		case _0:
		case both:
		case reset: break;

		case front: st.cull = D3DCULL_CW; break;
		case back: st.cull = D3DCULL_CCW; break;
			
		default: FORMAT_E()
		}

	END_FORMAT_F
	END(front)
//	}	
	END_FORMAT
	
	//that's it for state buffers
	if(b->format==UNRECOGNIZED) b->format = pp.left; 	

	assert(b->state&&b->format==state);

	return out;
}

bool D3D9_Server::source(void**, const wchar_t[MAX_PATH])
{
	assert(0); return false; //unimplemented
}

bool D3D9_Server::expose2(void **io, const char *f, va_list va)
{
	if(!f) return false;	

	void **p = va_arg(va,void**); if(!p) return false;

	D3D9_Buffer *q = (D3D9_Buffer*)buffer(io); 

	IDirect3DSurface9 *s = q->surface;		
	IDirect3DTexture9 *t = q->texture;

	if(!strcmp(f,"IDirect3DDevice9*"))
	{
		device->AddRef(); *p = device; return true;
	}
	else if(!strcmp(f,"IDirect3DSurface9*"))
	{
		if(s) s->AddRef(); *p = s; if(s||!t) return true;

		if(t->GetSurfaceLevel(0,&s)==D3D_OK) *p = s; return true;
	}
	else if(!strcmp(f,"IDirect3DTexture9*"))
	{
		if(t) t->AddRef(); *p = t; return true;
	}

	return false;
}

void D3D9_Server::discard(void **io)
{
	if(!io||!*io) return;

	D3D9_cpp::share *p = (D3D9_cpp::share*)*io;

	//TODO: implement sharing 
	if(p->lock!=io){ assert(0); return; }

	switch(p->spec)
	{
	case Sompaint_hpp::buffer:

		delete (D3D9_cpp::buffer*)p; break;

	case Sompaint_hpp::raster:

		delete (SOMPAINT_PIX)p; break;

	case Sompaint_hpp::window:

		delete (SOMPAINT_BOX)p; break;
	}

	*io = 0;
}

void *D3D9_Server::lock(void **io, const char *mode, size_t inout[4], int plane)
{
	if(D3D9_BADSTATUS) return 0;

	//Reminder: would pass plane for mipmaps but 
	//unlock needs to know which level was locked

	if(!io||!*io||!inout) return 0; 
	
	D3D9_Buffer *b = *(D3D9_cpp::buffer*)*io;
		
	int flags = -1; if(!b->unknown) return 0; 

	if(mode)
	{
		if(*mode=='w')
		{
			bool plus = mode[1]=='+';

			if(!mode[plus?2:1]) flags = 0; //TODO: D3DLOCK_DISCARD/NOOVERWRITE
		}
		else if(*mode=='r')
		{
			if(mode[1]=='\0') flags = D3DLOCK_READONLY; //TODO: non-lockable surfaces 
		}

		if(flags==-1){ assert(0); return 0; }
	}
		
	void *out = 0;
	
	if(b->vbuffer)
	{
		D3DVERTEXBUFFER_DESC desc; 

		if(inout[1]==0&&inout[3]==1
		 &&b->vbuffer->GetDesc(&desc)==D3D_OK)
		{	
			size_t offset = inout[0]*b->bytes_per_vertex;	
			size_t bytes = inout[2]*b->bytes_per_vertex-offset;
			
			inout[0] = 0; inout[1] = 8*b->bytes_per_vertex; 

			inout[2] = b->bytes_per_vertex; inout[3] = desc.Size;

			if(inout[3]%inout[2]==0) //paranoia
			{
				if(mode)
				{
					if(b->vbuffer->Lock(offset,bytes,&out,flags)!=D3D_OK) assert(0);
				}
				else out = (void*)1;
			}
			else assert(0);
		}
		else assert(0);
	}	
	else if(b->ibuffer)
	{
		D3DINDEXBUFFER_DESC desc; 

		if(inout[1]==0&&inout[3]==1
		 &&b->ibuffer->GetDesc(&desc)==D3D_OK)
		{	
			size_t offset = inout[0]*b->bytes_per_index;
			size_t bytes = inout[2]*b->bytes_per_index-offset;

			inout[0] = 0; inout[1] = 8*b->bytes_per_index; 
						
			inout[2] = b->bytes_per_index; inout[3] = desc.Size;

			if(inout[3]%inout[2]==0
			&&(inout[2]==2&&desc.Format==D3DFMT_INDEX16
			 ||inout[2]==4&&desc.Format==D3DFMT_INDEX32)) //paranoia
			{
				if(mode)
				{
					if(b->ibuffer->Lock(offset,bytes,&out,flags)!=D3D_OK) assert(0);
				}
				else out = (void*)1;
			}
			else assert(0);
		}
		else assert(0);
	}
	else 
	{
		D3DSURFACE_DESC desc;

		if(b->texture)
		{
			//very similar to IDirect3DSurface9*
			if(b->texture->GetLevelDesc(0,&desc)==D3D_OK)
			{				
				D3DLOCKED_RECT lock;
				
				RECT rect = {inout[0],inout[1],inout[2],inout[3]};

				switch(desc.Format)
				{
				case D3DFMT_A8R8G8B8: case D3DFMT_X8R8G8B8: 

					inout[0] = 0; inout[1] = 32; inout[2] = 4; break;

				default: assert(0); return 0;
				}

				inout[3] = desc.Width*inout[2]; 

				if(mode)
				{
					if(b->texture->LockRect(0,&lock,&rect,flags)==D3D_OK)
					{
						out = lock.pBits; inout[3] = lock.Pitch;
					}				
					else assert(0);
				}
				else out = (void*)1;
			}
			else assert(0);
		}							
		else if(b->surface)
		{
			if(b->surface->GetDesc(&desc)==D3D_OK)
			{				
				D3DLOCKED_RECT lock;
				
				RECT rect = {inout[0],inout[1],inout[2],inout[3]};

				inout[0] = inout[3] = 0;

				switch(desc.Format) //TODO: stencil bits
				{	
				case D3DFMT_D16: inout[1] = 16; inout[2] = 2;  break;

				case D3DFMT_D24X8: inout[1] = 24; inout[2] = 4;  break;

				case D3DFMT_D32: case D3DFMT_A8R8G8B8: case D3DFMT_X8R8G8B8: 

					inout[1] = 32; inout[2] = 4; break;

				default: assert(0); return 0;
				}

				inout[3] = desc.Width*inout[2]; 

				if(mode)
				{
					if(b->surface->LockRect(&lock,&rect,flags)==D3D_OK)
					{
						out = lock.pBits; inout[3] = lock.Pitch;
					}
					else assert(0);
				}
				else out = (void*)1;
			}
			else assert(0);
		}	
		else if(b->xbuffer)
		{
			assert(0); //unimplemented
		}
		else assert(0);
	}

	assert(out);
	return out;
}

void D3D9_Server::unlock(void **io)
{		
	if(!io||!*io) return; 
	
	D3D9_Buffer *b = *(D3D9_cpp::buffer*)*io; 
				   	
	if(b->texture)
	{
		if(b->texture->UnlockRect(0)!=D3D_OK) assert(0);
	}
	else if(b->surface)
	{
		if(b->surface->UnlockRect()!=D3D_OK) assert(0);
	}
	else if(b->vbuffer)
	{
		if(b->vbuffer->Unlock()!=D3D_OK) assert(0);
	}
	else if(b->ibuffer)
	{
		if(b->ibuffer->Unlock()!=D3D_OK) assert(0);
	}
	else if(b->xbuffer)
	{
		assert(0); //unimplemented
	}
	else assert(0);
}

const wchar_t *D3D9_Server::assemble(const char *code, size_t codelen)
{	
	if(!code) return false;

	if(codelen==size_t(-1)) codelen = strlen(code);

	LPD3DXBUFFER bytecode = 0, errors;

	D3DXAssembleShader(code,codelen,0,0,0,&bytecode,&errors);

	wchar_t *out = 0; 

	if(bytecode)
	{			
		D3D9_cpp::shader *sh = 0;

		DWORD *bc = (DWORD*)bytecode->GetBufferPointer();

		IDirect3DVertexShader9 *vsout; IDirect3DPixelShader9 *psout;

		if(device->CreateVertexShader(bc,&vsout)==D3D_OK)
		{
			sh = new D3D9_cpp::shader('vs',this); sh->vshader = vsout;
		}
		else if(device->CreatePixelShader(bc,&psout)==D3D_OK)
		{
			sh = new D3D9_cpp::shader('ps',this); sh->pshader = psout;
		}
				
		if(sh){ out = (wchar_t*)&sh->errors; sh->bytecode = bytecode; }

		if(!sh){ bytecode->Release(); assert(0); }
	}
	else if(errors)
	{
		size_t len = errors->GetBufferSize();
				
		char *c_str = (char*)errors->GetBufferPointer();

		wchar_t *w_str = new wchar_t[len];
		
		for(size_t i=0;i<len;i++) w_str[i] = c_str[i]; assert(w_str[len-1]==0); 

		out = w_str; errors->Release();
	}

	D3D9_cpp::section cs; 

	if(out) pool[out] = !errors?D3D9_shader_delete:D3D9_simple_delete;		

	return out;
}

bool D3D9_Server::run(const wchar_t *program)
{	
	if(D3D9_BADSTATUS) return false;
			
	bool out = true;

	D3D9_cpp::shader *sh = (D3D9_cpp::shader*)program;

	switch(foreground=sh&&!sh->errors?sh->profile:0)
	{
	case 'vs': if(sh!=vs) vs = sh; else break;

		out = device->SetVertexShader(sh->vshader)==D3D_OK; break;
	
	case 'ps': if(sh!=ps) ps = sh; else break;

		out = device->SetPixelShader(sh->pshader)==D3D_OK; break;

	default: out = false;
	}

	assert(out);
	return out;
}

//subroutine
static int D3D9_load(D3D9_Server &server, Sompaint_hpp::load &pp);

int D3D9_Server::load2(const char *f, va_list v)
{
	if(D3D9_BADSTATUS) return 0;
		
	Sompaint_hpp::load pp(f,v); //preprocessor

	return D3D9_load(*this,pp);
}

int D3D9_Server::load3(const char *f, void **v, size_t s)
{
	if(D3D9_BADSTATUS) return 0;

	Sompaint_hpp::load pp(f,(const void**)v,s); //preprocessor

	return D3D9_load(*this,pp);
}

static int D3D9_load(D3D9_Server &server, Sompaint_hpp::load &pp)
{
	int out = 0;

	while(*pp)
	{
		int mod = 1; pp++;

		bool L = pp.type[0]=='L';
	
		char type = pp.type[L?1:0];

		switch(type)
		{
		case 'f': case 'i': mod = 4; case 'b': break;

		default: pp++; continue;
		}

		//Hmmm: should unaligned be supported?
		//TODO: simulation (for counting registers)
		if(pp.width%mod||pp.precision%mod||!pp.data)
		{
			assert(0); return out; //unimplemented
		}
		
		HRESULT hr = !D3D_OK;

		if(type=='f')
		{
			if(L) //doubles (todo: conversion)
			{
				assert(0); continue; //unimplemented
			}
			else if(server.foreground=='vs')
			{
				hr = server->SetVertexShaderConstantF(pp.precision/4,pp,pp.width/4);
			}
			else if(server.foreground=='ps') 
			{
				hr = server->SetPixelShaderConstantF(pp.precision/4,pp,pp.width/4);
			}
			else assert(0); 
		}
		else if(type=='i')
		{
			if(L) //typo??
			{
				assert(0); continue; 
			}
			else if(server.foreground=='vs')
			{
				hr = server->SetVertexShaderConstantI(pp.precision/4,pp,pp.width/4);
			}
			else if(server.foreground=='ps') 
			{
				hr = server->SetPixelShaderConstantI(pp.precision/4,pp,pp.width/4);
			}
			else assert(0); 
		}
		else if(type=='b')
		{
			if(L) //32bit
			{				
				if(server.foreground=='vs')
				{
					hr = server->SetVertexShaderConstantB(pp.precision,(int*)pp,pp.width);
				}
				else if(server.foreground=='ps') 
				{
					hr = server->SetPixelShaderConstantB(pp.precision,(int*)pp,pp.width);
				}
				else assert(0); 				
			}
			else //bool (todo: conversion)
			{
				assert(0); continue; //unimplemented
			}
		}
		else assert(0);

		if(hr==D3D_OK) out+=pp.width;		
	}

	return out;
}

void D3D9_Server::reclaim(const wchar_t *hin)
{		
	if(!hin) return;

	D3D9_cpp::handle *sh = (D3D9_cpp::handle*)hin;

	if(!sh->errors)
	{
		switch(sh?sh->profile:0)
		{
		case 'vs': case 'ps': 

			delete (D3D9_cpp::shader*)sh; 

		default: assert(0);
		}
	}
	else //error string
	{
		D3D9_cpp::section cs; 
		pool.erase(sh); //does not have a destructor

		delete [] sh;
	}
}

void **D3D9_Server::define(const char *d, const char *def, void **io)
{
	//TODO: implement general solution in Sompaint.hpp

	assert(0); return 0; //unimplemented
}

const char *D3D9_Server::ifdef(const char *d, void **io)
{
	return 0; //unimplemented
}

bool D3D9_Server::reset(const char *reserved)
{				
	end(); assert(reserved==0);
		
	if(status<=0) //D3D9_BADSTATUS
	{
		switch(device->TestCooperativeLevel())
		{			
		case D3DERR_DEVICELOST: assert(status==0);
		{			
			D3D9_cpp::section cs; //hack

			if(D3D9_reset()) 
			{
				//assuming won't stack overflow
				status = 0; return reset(reserved); 
			}
			message = D3D9_lost_message;
			status = D3D9_lost_status;  //FALL THRU
		}
		case D3DERR_DEVICENOTRESET: assert(lost()); return false;
		
		case D3DERR_DRIVERINTERNALERROR: assert(0); return false;

		case D3D_OK: message = L""; status = 0; break;

		default: assert(0); return false;
		}		
	}
	else assert(0);

	vertex = 0; vs = ps = 0;

	//Reminder: Present can be called inside of Begin/EndScene 
	//as long as the backbuffer is not the bound render target

	if(rt) device->SetRenderTarget(0,b0_backbuffer);	
	if(ds) device->SetDepthStencilSurface(0); 
	if(vb) device->SetStreamSource(0,0,0,0);
	if(ib) device->SetIndices(0);		

	rt = ds = vb = ib = 0;  
	
	for(size_t i=0;i<mt_s;i++)
	{
		if(mt[i]) device->SetTexture(i,0); mt[i] = 0;
	}

	for(size_t i=0;i<rs_s;i++) rs[i] = 0;

	state.reset.apply(*this);

	return true;
}

bool D3D9_Server::frame(size_t vp[4])
{
	//TODO: clipping

	if(vp) //Reminder: d3d9.dll will spam on error
	{	
		D3DVIEWPORT9 vp9 = {vp[0],vp[1],vp[2]-vp[0],vp[3]-vp[1],z.range[0],z.range[1]};

		if(device->SetViewport(&vp9)==D3D_OK) return true;
	}

	//TODO: rt->GetDesc

	assert(0); return false; 
}
	
bool D3D9_Server::clip(size_t[4])
{
	//TODO: SetScissorRect/D3DRS_SCISSORTESTENABLE

	assert(0); return false; //unimplemented
}

SOMPAINT_PIX D3D9_Server::raster(void **io, void *local, const char *raw_name)
{
	if(!io) return 0;
		
	if(!*io) *io = new D3D9_cpp::raster(*this,io);
	
	D3D9_cpp::raster *out = (D3D9_cpp::raster*)*io;
	
	if(out->lock!=io)
	{
		if(out->lock)
		{
			assert(0); return 0; //TODO: implement sharing?
		}
		else out->lock = io;
	}

	if(raw_name) 
	{
		if(!strcmp(raw_name,typeid(HWND).raw_name()))
		{
			*out = (HWND*)local;
		}
		else if(!strcmp(raw_name,typeid(HBITMAP).raw_name()))
		{
			*out = (HBITMAP*)local;
		}
		else *out = (void*)0;
	}
	else *out = local;

	return *out;
}

SOMPAINT_BOX D3D9_Server::window(void **io, void *local, const char *raw_name)
{
	if(!io) return 0;
		
	if(!*io) *io = new D3D9_cpp::window(*this,io);
	
	D3D9_cpp::window *out = (D3D9_cpp::window*)*io;
	
	if(out->lock!=io)
	{
		if(out->lock)
		{
			assert(0); return 0; //TODO: implement sharing?
		}
		else out->lock = io;
	}
	
	if(raw_name) 
	{
		if(!strcmp(raw_name,typeid(RECT).raw_name()))
		{
			*out = (RECT*)local;
		}
		else *out = (void*)0;
	}
	else *out = local;

	return *out;
}

#undef D3D9_BADSTATUS
#define D3D9_BADSTATUS (server.status<0) 

bool D3D9_Buffer::apply(int i)
{
	if(D3D9_BADSTATUS||i<0) return false;

	if(i>=server.rs_s)
	{
		if(this==server.b_) return true;

		D3D9_cpp::section cs;
				
		size_t new_rs_s = server.rs_s*2;
		
		if(i>new_rs_s) new_rs_s = i; assert(i<16); 
		
		D3D9_cpp::state **new_rs = new D3D9_cpp::state*;

		memcpy(new_rs,server.rs,server.rs_s*sizeof(void*));

		server.pool[new_rs] = D3D9_simple_delete;

		server.rs_s = new_rs_s;
		server.rs = new_rs; 
	}

	if(this!=server.b_)
	{	
		if(format==Sompaint_hpp::state)
		{
			if(server.rs[i]!=state) state->apply(server);

			server.rs[state->index=i] = state;
		}
		else assert(0);
	}
	else server.rs[i] = 0;

	return true;
}

bool D3D9_Buffer::setup(int i)
{
	if(D3D9_BADSTATUS) return false;

	bool out = false;

	if(texture)
	{
		assert(canvas);

		IDirect3DSurface9 *rt = 0;

		if(texture->GetSurfaceLevel(0,&rt)==D3D_OK)
		{
			out = !server->SetRenderTarget(i,rt); rt->Release();
		}
		else assert(0);
	}
	else if(xbuffer)
	{
		IDirect3DSurface9 *rt = 0;

		if(xbuffer->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,&rt)==D3D_OK)
		{
			out = !server->SetRenderTarget(i,rt); rt->Release();
		}
		else assert(0);
	}		
	else if(surface)
	{				
		if(!z)
		{	
			out = !server->SetRenderTarget(i,surface);			
		}	
		else out = !server->SetDepthStencilSurface(surface);
	}
	else assert(0);

	if(out) (z?server.ds:server.rt) = this;

	//could dirty the mirror instead
	if(mirror&&server.b0_mirror==mirror)
	{
		server.b0_mirror = 0; mirror->Release();
	}
	
	if(planes!=this)
	{
		assert(0); //unimplemented
	}

	return out;
}

bool D3D9_Buffer::clear(int mask)
{
	if(D3D9_BADSTATUS) return false;

	D3DVIEWPORT9 vp; server->GetViewport(&vp);

	RECT scissor; server->GetScissorRect(&scissor);

	RECT viewport = { vp.X, vp.Y, vp.X+vp.Width, vp.Y+vp.Height };

	RECT i; if(!IntersectRect(&i,&scissor,&viewport)) return true;

	bool out = false;

	if(mask&1)
	{
		if(z) //todo: D3DCLEAR_STENCIL
		{
			out = !server->Clear(1,(D3DRECT*)&i,D3DCLEAR_ZBUFFER,0,clearZ,0);
		}
		else //assuming 8bit colour...
		{
			D3DCOLOR rgba = D3DCOLOR_COLORVALUE(clearR,clearB,clearG,clearA);

			out = !server->Clear(1,(D3DRECT*)&i,D3DCLEAR_TARGET,rgba,0,0);
		}
	}
	else assert(0);

	if(planes!=this)
	{
		assert(0); //unimplemented

		if(mask!=1) return false;
	}

	return out;
}

bool D3D9_Buffer::sample(int i)
{
	if(D3D9_BADSTATUS) return false;

	if(i>=server.mt_s) return false;	

	if(server.mt[i]==this) return true;

	if(server->SetTexture(i,texture)==D3D_OK)
	{
		int compile[!D3D9_PUREDEVICE]; 

		D3DTEXTUREFILTERTYPE f = D3DTEXF_NONE, g = f; //POINT?

		if(mipmap) //||texture&&texture->GetLevelCount()>1)
		{
			f = D3DTEXF_ANISOTROPIC; g = D3DTEXF_LINEAR; 

			if(!server->SetSamplerState(i,D3DSAMP_MAXANISOTROPY,8))
			{
				if(!server->SetSamplerState(i,D3DSAMP_MAXANISOTROPY,4))
				{
					f = D3DTEXF_LINEAR;
				}
			}			
		}	   
		
		server->SetSamplerState(i,D3DSAMP_MAGFILTER,f);
		server->SetSamplerState(i,D3DSAMP_MINFILTER,f);
		server->SetSamplerState(i,D3DSAMP_MIPFILTER,g);

		server.mt[i] = this; return true;
	}
	else return false;
}

bool D3D9_Buffer::layout(const wchar_t *model)
{
	if(D3D9_BADSTATUS) return false;

	if(!model||*model!='M'){ assert(0); return false; }
	
	if(model[1]&&model[2]) return D3D9_vdecl(server,model);

	return false;
}

int D3D9_Buffer::stream(int n, const void *up, size_t up_s)
{
	if(D3D9_BADSTATUS||n<=0) return 0;
	
	HRESULT hr = D3D_OK;

	if(server.vb!=this) 
	{		
		hr = server->SetStreamSource(0,vbuffer,0,server.vstride[0]);

		if(hr==D3D_OK) server.vb = this; else return 0;
	}

	if(up) 
	{
		if(!D3D9_upload(server,up,up_s,n,server.vstride[1])) return 0;
		
		hr = server->SetStreamSource(1,server.up,0,server.vstride[1]);
	}
	else if(!server.vstride[1])
	{
		hr = server->SetStreamSource(1,0,0,0); assert(hr==D3D_OK);
	}
		
	if(hr==D3D_OK) hr = server->SetStreamSourceFreq(0,n|D3DSTREAMSOURCE_INDEXEDDATA);	
	if(hr==D3D_OK) hr = server->SetStreamSourceFreq(1,1|D3DSTREAMSOURCE_INSTANCEDATA);

	if(!server.vstride[1]) assert(hr==D3D_OK);

	return hr==D3D_OK?n:0;
}

static IDirect3DSurface9 *D3D9_z(D3D9_Buffer *b)
{	
	if(!b||!b->reset) return 0;

	//Reminder:
	//the common zbuffer cannot be multi-sampled 

	size_t w = 0, h = 0; 
	
	if(b->surface||b->texture)
	{
		if(b->reset->MultiSampleType) return 0;

		w = b->reset->Width; h = b->reset->Height;
	}
	else if(b->xbuffer) 
	{
		if(b->xreset->MultiSampleType) return 0;

		w = b->xreset->BackBufferWidth; h = b->xreset->BackBufferHeight;
	}
	else return 0;

	D3DSURFACE_DESC desc; 

	IDirect3DSurface9 *out = 0;
	
	D3D9_Buffer *d = b->server.d_;

	if(d) out = d->surface;

	if(out&&out->GetDesc(&desc)!=D3D_OK) return 0;
			   
	if(!out||desc.Width<w||desc.Height<h)	
	{	
		if(out){ out->Release(); d->unknown = 0; }

		if(out){ w = D3D9_RESOLUTION(w); h = D3D9_RESOLUTION(h); }

		b->server->CreateDepthStencilSurface(w,h,D3DFMT_D32,D3DMULTISAMPLE_NONE,0,TRUE,&out,0);

		if(!out) b->server->CreateDepthStencilSurface(w,h,D3DFMT_D24X8,D3DMULTISAMPLE_NONE,0,TRUE,&out,0);
		if(!out) b->server->CreateDepthStencilSurface(w,h,D3DFMT_D16,D3DMULTISAMPLE_NONE,0,TRUE,&out,0);

		if(d&&out&&out->GetDesc(d->reset)!=D3D_OK) assert(0);

		if(!out) return 0;		
	}

	if(!d)
	{
		d = b->server.d_ = *new D3D9_cpp::buffer(b->server,0);

		if(out->GetDesc(d->reset=new D3DSURFACE_DESC)!=D3D_OK) assert(0);

		d->format = Sompaint_hpp::frame; d->z = 1;
		d->clearZ = 1.0;
	}

	if(out) d->surface = out;	  

	return out;
}

bool D3D9_Buffer::draw(int start, int count, int vstart, int vcount)
{
	if(D3D9_BADSTATUS) return false;

	if(server.ib!=this) 
	{
		if(server->SetIndices(ibuffer)==D3D_OK) server.ib = this; else return false;
	}

	//communal depth buffer
	if(!server.ds&&server.z.function!=D3DCMP_NEVER) 
	{	
		if(server->SetDepthStencilSurface(D3D9_z(server.rt))!=D3D_OK) assert(0);

		server.ds = server.d_; assert(server.ds);

		server.ds->clear(1);
	}

	server.begin(); assert(count%3==0);

	HRESULT hr = server->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,vstart,vcount,start,count/3);

	return hr==D3D_OK;
}

static const char D3D9_swap_vs[] = 
"   vs_2_0\n"
"   dcl_position v0\n"
"   dcl_texcoord v1\n"
"   mov oPos, v0\n"
"   mov oT0.xy, v1\n";

static const char D3D9_swap_ps[] = 
"   ps_2_0\n"
"   def c0, 1, 0, 0, 0\n"
"   dcl t0.xy\n"
"   dcl_2d s0\n"
"   texld r0, t0, s0\n"
"	mov r0.w, c0.x\n"
"   mov oC0, r0\n";

static const char D3D9_swap_ps_aaa[] = 
"   ps_2_0\n"
"   def c0, 1, 0, 0, 0\n"
"   dcl t0.xy\n"
"   dcl_2d s0\n"
"   texld r0, t0, s0\n"
"	mov r1.x, r0.w\n"
"	mov r1.y, r0.w\n"
"	mov r1.z, r0.w\n"
"	mov r1.w, c0.x\n"
"   mov oC0, r1\n";

static const char D3D9_swap_ps_bgr[] = 
"   ps_2_0\n"
"   def c0, 1, 0, 0, 0\n"
"   dcl t0.xy\n"
"   dcl_2d s0\n"
"   texld r0, t0, s0\n"
"	mov r1.x, r0.z\n"
"	mov r1.y, r0.y\n"
"	mov r1.z, r0.x\n"
"	mov r1.w, c0.x\n"
"   mov oC0, r1\n";

static IDirect3DTexture9 *D3D9_swap(D3D9_Buffer *b)
{		
	if(!b->server.b0_swapbuffer_ps)
	{
		assert(!b->server.b0_swapbuffer_vs);

		b->server.b0_swapbuffer_ps = D3D9_shader(b->server,D3D9_swap_ps);
		b->server.b0_swapbuffer_vs = D3D9_shader(b->server,D3D9_swap_vs);
				
		//might want to delay assembling these until they're needed (no big deal)
		b->server.b0_swapbuffer_ps_aaa = D3D9_shader(b->server,D3D9_swap_ps_aaa);
		b->server.b0_swapbuffer_ps_bgr = D3D9_shader(b->server,D3D9_swap_ps_bgr);

		assert(b->server.b0_swapbuffer_ps&&b->server.b0_swapbuffer_vs);
	}

	if(b->texture&&!b->memory) return b->texture; 

	if(!b->mirror) if(b->texture)
	{
		assert(b->memory);

		b->texture->GetSurfaceLevel(0,&b->mirror); //trivial		
	}
	else if(b->surface)
	{
		b->surface->AddRef(); b->mirror = b->surface;
	}
	else if(0) //this logic is likely completely obsolete
	{	 
		/*still this code will almost definitely come in handy somewhere

		D3DLOCKED_RECT src, dst;

		if(b->texture->LockRect(0,&src,0,D3DLOCK_READONLY)!=D3D_OK) return 0;

		//WARNING: assuming client is responisble for rgb vs rgba 

		b->server->CreateOffscreenPlainSurface
		(desc.Width,desc.Height,desc.Format,D3DPOOL_SYSTEMMEM,&b->mirror,0);			
		
		//will need a generalized update routine
		if(b->mirror->LockRect(&dst,0,0)==D3D_OK) 
		{
			if(src.Pitch==4*desc.Width&&dst.Pitch==4*desc.Width)
			{
				memcpy(dst.pBits,src.pBits,4*desc.Width*desc.Height);
			}
			else for(int i=0;i<desc.Height;i++)
			{
				void *d = (char*)dst.pBits+dst.Pitch*i;
				void *s = (char*)src.pBits+src.Pitch*i;

				memcpy(d,s,4*desc.Width);
			}

			b->mirror->UnlockRect();
		}
		else assert(0);

		b->texture->UnlockRect(0);
		*/
	}
	else assert(0); //unimplemented

	if(!b->mirror) return 0;
	
	IDirect3DTexture9 *out = b->server.b0_swapbuffer;

	if(b->server.b0_mirror==b->mirror) return out;
						  	
	D3DSURFACE_DESC desc, desc2;

	if(b->mirror->GetDesc(&desc)!=D3D_OK
	||out&&out->GetLevelDesc(0,&desc2)!=D3D_OK)
	{
		assert(0); return 0; //paranoia
	}
	if(!out||desc2.Width<desc.Width||desc2.Height<desc.Height)
	{			
		desc2.Width = !out?desc.Width:D3D9_RESOLUTION(desc.Width);
		desc2.Height = !out?desc.Height:D3D9_RESOLUTION(desc.Height);

		//assert(desc.Format==D3DFMT_A8R8G8B8); //hack

		b->server->CreateTexture //Reminder: basic memory is forced to A8R8G8B8
		(desc2.Width,desc2.Height,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&out,0);	

		if(b->server.b0_swapbuffer)	b->server.b0_swapbuffer->Release();

		b->server.b0_swapbuffer = out; assert(out);
	}
	
	IDirect3DSurface9 *top = 0;

	if(out&&out->GetSurfaceLevel(0,&top)!=D3D_OK) assert(0);

	RECT rect = {0,0,desc.Width,desc.Height}; POINT pt = {0,0};

	if(desc.Pool==D3DPOOL_SYSTEMMEM)
	{
		if(top&&b->server->UpdateSurface(b->mirror,&rect,top,&pt)!=D3D_OK) assert(0);
	}
	else if(desc.Pool==D3DPOOL_DEFAULT)
	{
		if(top&&b->server->StretchRect(b->mirror,&rect,top,&rect,D3DTEXF_NONE)!=D3D_OK) assert(0);
	}
	else assert(0); 

	if(top) top->Release();

	if(out)
	{
		if(b->server.b0_mirror) b->server.b0_mirror->Release();
		b->server.b0_mirror = b->mirror; b->mirror->AddRef();
	}
	else assert(0);	

	return out;
}

bool D3D9_Buffer::present(int zoom, SOMPAINT_BOX box1, SOMPAINT_BOX box2, SOMPAINT_PIX pix)
{		
	if(D3D9_BADSTATUS||zoom<0) return false;

	if(this==server.b_)
	{
		assert(0); return false; 
	}
		
	HWND *hwnd = pix->hwnd(); if(!hwnd) return false;	

	RECT *src = box1->rect(), *dst = box2->rect(); assert(*box1==src&&*box2==dst);
		
	HRESULT dl = D3D_OK; 

	IDirect3DSwapChain9 *x = 0;

	if(x=xbuffer) //early out for genuine frame buffers
	{
		if(server.rt==this) server.end(); 

		//Reminder: if this fails d3d9d.dll will spam a lot
		if(!zoom) dl = xbuffer->Present(src,dst,*hwnd,0,0);
				
		if(dl==D3D_OK) return true; 
	}	
	
	//TODO: b0_critical_section
	
	IDirect3DSurface9 *d = server.b0_backbuffer;
	IDirect3DTexture9 *s = surface?0:D3D9_swap(this);	
	
	RECT dstrect, srcrect; D3DSURFACE_DESC ddesc, sdesc;

	if(d&&d->GetDesc(&ddesc)!=D3D_OK) d = 0;

	if(s&&s->GetLevelDesc(0,&sdesc)!=D3D_OK) d = 0;

	if(surface&&surface->GetDesc(&sdesc)!=D3D_OK) d = 0;
	
	if(!dst) if(!GetClientRect(*hwnd,&dstrect)) d = 0; 

	if(!src) SetRect(&srcrect,0,0,sdesc.Width,sdesc.Height);	
	
	if(dst) dstrect = *dst; dst = &dstrect;
	if(src) srcrect = *src; src = &srcrect;	
		
	if(zoom>0) //clipping (maybe overkill)
	{			  		
		int x = std::min(src->right-src->left,dst->right-dst->left);
		int y = std::min(src->bottom-src->top,dst->bottom-dst->top);

		src->right = src->left+x; src->bottom = src->top+y;
		dst->right = dst->left+x; dst->bottom = dst->top+y;
	}

	if(surface&&d)				
	{				
		if(zoom<2&&!swizzle()) 
		{				
			RECT swap = //render target optimization
			{0,0,dst->right-dst->left,dst->bottom-dst->top};
			
			//TODO: we can do this piece wise-whenever zoom is 1
			if(swap.right<=ddesc.Width&&swap.bottom<=ddesc.Height)
			{
				if(server.rt==this) server.end(); HRESULT hr; //debugging 

				D3DTEXTUREFILTERTYPE f = zoom?D3DTEXF_POINT:D3DTEXF_LINEAR;

				if(hr=server->StretchRect(surface,src,d,&swap,f))
				{	
					//WM_PAINT can have 0 sized area
					if(swap.right<=0||swap.bottom<=0)
					{
						return true; //technically correct
					}
					else assert(0); //troubling
				}
				else  
				{
					dl = server->Present(&swap,dst,*hwnd,0);
					
					if(dl==D3D_OK) return true; 
				}
			}
		}

		if(!dl) if(s=D3D9_swap(this)) //b0_swapbuffer
		{
			if(s->GetLevelDesc(0,&sdesc)!=D3D_OK) d = 0;
		}
	}	

	D3D9_cpp::shader *vs = server.b0_swapbuffer_vs;
	D3D9_cpp::shader *ps = server.b0_swapbuffer_ps;	
				
	switch(swizzle())
	{
	case 'aaa': ps = server.b0_swapbuffer_ps_aaa; break;
	case 'bgr': ps = server.b0_swapbuffer_ps_bgr; break;
	}

	bool out = false;

	if(d&&s&&vs&&ps&&!dl)
	{	
		assert(zoom); //unimplemented

		int w = ddesc.Width, h = ddesc.Height;		

		int m = (dst->right-dst->left)/w+1, n = (dst->bottom-dst->top)/h+1;

		for(int i=0;i<m;i++) for(int j=0;j<n;j++)
		{		
			RECT si,st = {0,0,w,h}, di,dt = {0,0,w,h};

			OffsetRect(&st,src->left+i*w,src->top+j*h);
			OffsetRect(&dt,dst->left+i*w,dst->top+j*h);

			if(!IntersectRect(&si,&st,src)||!IntersectRect(&di,&dt,dst)) continue;

		    //Reminder: StretchRect cannot work for abitrary strecthing//
		    //across multiple composited present calls in all scenarios//
			
			//todo: server critical section
			if(server.scene) server.reset(0);
			
			//server.reset() should amount to this//
			//server->SetRenderState(D3DRS_ZENABLE,0);
			//server->SetRenderState(D3DRS_ALPHABLENDENABLE,0);
			//server->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);		
			//server->SetRenderTarget(0,d);
			//server->SetDepthStencilSurface(0);
			
			server.begin(); 

			server->SetPixelShader(ps->pshader);
			server->SetVertexShader(vs->vshader);			
			server->SetTexture(0,s);
						
			server->SetStreamSourceFreq(0,1);
			server->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);			
													
			float l = float(si.left)/zoom/sdesc.Width;
			float t = float(si.top)/zoom/sdesc.Height;
			float r = float(si.right)/zoom/sdesc.Width;
			float b = float(si.bottom)/zoom/sdesc.Height;
			
			//Note: Seems to be more than round off error...
			//Don't really understand subpixel raster coords
			//myself, but these constants just seem to work!

			float x = si.right-si.left+0.5, y = si.bottom-si.top+0.5;

			float vb[24] = 
			{
				0,0,0,1,l,t,  x,0,0,1,r,t,  x,y,0,1,r,b,  0,y,0,1,l,b
			};
			
			D3DTEXTUREFILTERTYPE f = zoom?D3DTEXF_POINT:D3DTEXF_LINEAR;

			server->SetSamplerState(i,D3DSAMP_MAGFILTER,f);
			server->SetSamplerState(i,D3DSAMP_MINFILTER,f);
			server->SetSamplerState(i,D3DSAMP_MIPFILTER,D3DTEXF_NONE);
				 
			server->DrawPrimitiveUP(D3DPT_TRIANGLEFAN,2,vb,24);
			
			server.end();

			RECT xy = {0,0,x,y}; 

			if(dl=server->Present(&xy,&di,*hwnd,0)) 
			{	
				out = false; i = m; j = n; break; //hack: break out
			}
			else out = true;
		}
	}
	else assert(dl);
							
	if(dl==D3DERR_DEVICELOST) 
	{
		D3D9_cpp::section cs; //hack

		//goto retry; //assuming won't stack overflow
		if(D3D9_reset()) return present(zoom,box1,box2,pix);
		
		server.message = D3D9_lost_message;
		server.status = D3D9_lost_status; 
	}

	return out;
}

bool D3D9_Buffer::fill(int x, int y, int wrap, int zoom, SOMPAINT_BOX box1, SOMPAINT_BOX box2, SOMPAINT_PIX pix)
{
	assert(zoom); //unimplemented

	if(D3D9_BADSTATUS) return false;

	if(server.rt==this) server.end(); 

	HWND *hwnd = pix->hwnd(); if(!hwnd) return false;	

	RECT *src = box1->rect(), *dst = box2->rect(); assert(*box1==src&&*box2==dst);
				
	D3DSURFACE_DESC sdesc, ddesc; 

	D3DLOCKED_RECT lock; RECT rect = {0,0,x,1}; //1D

	if(surface)
	{
		if(surface->GetDesc(&sdesc)!=D3D_OK) return false;

		if(surface->LockRect(&lock,&rect,D3DLOCK_READONLY)!=D3D_OK) return false;
	}
	else if(texture)
	{
		if(texture->GetLevelDesc(0,&sdesc)!=D3D_OK) return false;

		if(texture->LockRect(0,&lock,&rect,D3DLOCK_READONLY)!=D3D_OK) return false;
	}
	else return false;

	/*///// point of no return /////*/	

	RECT srcrect, dstrect; 

	if(!src)
	{	
		SetRect(&srcrect,0,0,sdesc.Width,sdesc.Height);
	}
	else srcrect = *src; src = &srcrect;

	if(!dst)
	{
		if(!GetClientRect(*hwnd,&dstrect)) assert(0);
	}
	else dstrect = *dst; dst = &dstrect;
	
	HRESULT dl = !D3D_OK; 

	PALETTEENTRY *p = (PALETTEENTRY*)lock.pBits;

	if(lock.Pitch>=sizeof(*p)*x 
	 //TODO: critical section around buffer(0)
	 &&server.b0_backbuffer->GetDesc(&ddesc)==D3D_OK)
	{	
		assert(dl!=D3DERR_DEVICELOST);

		int w = ddesc.Width, h = ddesc.Height;		

		int m = (dst->right-dst->left)/w+1, n = (dst->bottom-dst->top)/h+1;

		for(int i=0;i<m;i++) for(int j=0;j<n;j++)
		{		
			RECT si,st = {0,0,w,h}, di,dt = {0,0,w,h};

			OffsetRect(&st,src->left+i*w,src->top+j*h);
			OffsetRect(&dt,dst->left+i*w,dst->top+j*h);

			if(!IntersectRect(&si,&st,src)||!IntersectRect(&di,&dt,dst)) continue;

			assert(!server.scene); //todo: server critical section

			/////// different from present ///////////////////////
							
			for(int k=0;k<y;k++)
			{
				RECT f = {k%wrap*zoom,k/wrap*zoom};
				f.right = f.left+zoom; f.bottom = f.top+zoom;

				if(!IntersectRect(&st,&si,&f)) continue;

				f.left = st.left-si.left; f.right = st.right-si.left;
				f.top = st.top-si.top; f.bottom = st.bottom-si.top;

				PALETTEENTRY pe = p[k%x]; D3DCOLOR g;
				
				switch(swizzle())
				{				
				case 'bgr':	g = D3DCOLOR_XRGB(pe.peRed,pe.peGreen,pe.peBlue); break;
				case 'aaa':	g = D3DCOLOR_XRGB(pe.peFlags,pe.peFlags,pe.peFlags); break;

				default: g = reinterpret_cast<D3DCOLOR&>(pe); break;
				}

				if(server->ColorFill(server.b0_backbuffer,&f,g)!=D3D_OK)
				{
					k = y; j = n; i = m; assert(0); break;
				}
			}

			//////////////////////////////////////////////////////

			RECT swap = {0,0,si.right-si.left,si.bottom-si.top};

			if(dl=server->Present(&swap,&di,*hwnd,0)) //!D3D_OK
			{	
				i = m; j = n; break; //hack: early out				
			}
		}
	}
	else assert(0);
								
	if(surface) surface->UnlockRect(); 
	if(texture) texture->UnlockRect(0);

	if(dl==D3DERR_DEVICELOST) 
	{
		D3D9_cpp::section cs; //hack

		//goto retry; //assuming won't stack overflow
		if(D3D9_reset()) return fill(x,y,wrap,zoom,box1,box2,pix);
		
		server.message = D3D9_lost_message;
		server.status = D3D9_lost_status; 
	}

	return dl==D3D_OK;
}

bool D3D9_Buffer::print(int width, int height, SOMPAINT_PIX pix)
{			  
	if(D3D9_BADSTATUS) return false;

	HBITMAP *hbitmap = pix->hbitmap(); 
	
	if(!hbitmap){ assert(0); return false; }

	if(!texture||memory) //memory forces A8R8G8B8
	{
	//Reminder: this needs to work for all colour buffers

		assert(0); return false; //unimplemented
	}	

    //// _unimplemented: passing thru for Somthumb.cpp_ ////
   //// Anything else might cross GetDC's restrictions ////

  /*THE FOLLOWING IS GUARANTEED FOR XRGB TEXTURES ONLY*/
	
	D3DSURFACE_DESC desc; IDirect3DSurface9 *p = 0; 

	if(texture->GetLevelDesc(0,&desc)!=D3D_OK) return 0;
	if(texture->GetSurfaceLevel(0,&p)!=D3D_OK) return 0; 
	
	HBITMAP out = 0; HDC src = 0, dst = 0; 

	if(p->GetDC(&src)==D3D_OK)
	{	
		if(dst=CreateCompatibleDC(src))
		{
			if(out=CreateCompatibleBitmap(src,width,height))
			{
				HGDIOBJ restore = SelectObject(dst,(HGDIOBJ)out);

				SetStretchBltMode(dst,STRETCH_HALFTONE);
				if(!StretchBlt(dst,0,0,width,height,src,0,0,desc.Width,desc.Height,SRCCOPY))
				{
					DeleteBitmap(out); out = 0;
				}
				SelectObject(dst,restore);
			}			
			DeleteDC(dst);			
		}	
		p->ReleaseDC(src); 
	}			 
	p->Release();
	
#ifdef _DEBUG

	if(!out) assert(0); //TODO: ensure the buffer is ineligible

#endif

	return *hbitmap = out;
}

SOMPAINT_MODULE_API SOMPAINT SOMPAINT_LIB(Connect)(const wchar_t v[4], const wchar_t *party)
{
	return new D3D9_Server; 
}

SOMPAINT_MODULE_API void SOMPAINT_LIB(Disconnect)(SOMPAINT p, bool host)
{
	delete (D3D9_Server*)p; assert(!host);
}

static DWORD WINAPI D3D9_unload(LPVOID)
{
	Sleep(1000); FreeLibraryAndExitThread(D3D9_dll(),0);
}

void *D3D9_Server::disconnect(bool host)
{
	delete this; CreateThread(0,0,D3D9_unload,0,0,0); assert(!host);

	return 0; //Microsoft Visual Studio
}

static HMODULE D3D9_hmodule = 0;

static bool D3D9_detached = false;

extern HMODULE D3D9_dll()
{
	//has DllMain been entered?
	assert(D3D9_hmodule||D3D9_detached); 

	return D3D9_hmodule;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{							   
	if(ul_reason_for_call==DLL_PROCESS_ATTACH)
	{	
		D3D9_hmodule = hModule; 
	}	
	else if(ul_reason_for_call==DLL_THREAD_DETACH)
	{

	}
	else if(ul_reason_for_call==DLL_PROCESS_DETACH)
	{
		D3D9_detached = true; //D3D9_dll

		if(lpReserved) D3D9_hmodule = 0; //...

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

		assert(D3D9_released); D3D9_release(0);
	}

    return TRUE;
} 