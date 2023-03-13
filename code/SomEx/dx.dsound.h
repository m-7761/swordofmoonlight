
#ifndef DX_DSOUND_INCLUDED
#define DX_DSOUND_INCLUDED

#include <mmsystem.h> //WAVEFORMATEX

class IDirectSound8;
class IDirectSoundBuffer8;

namespace DSOUND
{
	extern HINSTANCE dll;

	static const char *library = "DirectX";

	extern int target; //'dx7', 'dx8'

	extern bool doPiano;
	extern bool doDelay;
	extern bool doForward3D;
	extern bool doReverb; //2020
	extern int doReverb_i3dl2[2];
	extern int doReverb_mask;

	extern DWORD listenedFor;

	extern void Piano(int key);
}

#ifdef DIRECTX_INCLUDED

namespace DSOUND
{
	//NTOE: DX8 solves the freeze at start up
	//problem when the BGM is silent or MIDI
	static bool doIDirectSound8 = 1;
	static bool doIDirectSound = 1;
	static bool doIDirectSoundBuffer = 1;
	static bool doIDirectSound3DListener = 1;
	static bool doIDirectSound3DBuffer = 1;
}

namespace DSOUNDLOG
{
	static int debug = 0; //debugging
	static int error = 7; //serious errors
	static int panic = 7; //undefined
	static int alert = 7; //warning
	static int hello = 7; //fun stuff
	static int sorry = 7; //woops

	static int *master = DX_LOG(DSound);
}

#define DSOUND_LEVEL(lv) if(lv<=DSOUNDLOG::debug&&lv<=*DSOUNDLOG::master) dx.log
#define DSOUND_ERROR(lv) if(lv<=DSOUNDLOG::error&&lv<=*DSOUNDLOG::master) dx.log
#define DSOUND_PANIC(lv) if(lv<=DSOUNDLOG::panic&&lv<=*DSOUNDLOG::master) dx.log
#define DSOUND_ALERT(lv) if(lv<=DSOUNDLOG::alert&&lv<=*DSOUNDLOG::master) dx.log
#define DSOUND_HELLO(lv) if(lv<=DSOUNDLOG::hello&&lv<=*DSOUNDLOG::master) dx.log
#define DSOUND_SORRY(lv) if(lv<=DSOUNDLOG::sorry&&lv<=*DSOUNDLOG::master) dx.log

namespace DSOUND
{
	static char *error(HRESULT);
}

static const HRESULT DSOUND_UNIMPLEMENTED = 2;

#ifdef _DEBUG

#define DSOUND_RETURN(statement){ HRESULT __ = (statement);\
	if(__) DSOUND_ALERT(2) << "DirectX interface returned error " << dx%DSOUND::error(__) << '(' << std::hex << __ << ')' << " in " << dx%__FILE__ << " at " << std::dec << __LINE__ << " (" << dx%__FUNCTION__ ")\n";\
		assert(1||!__||DDRAW::fullscreen); if(__==DSOUND_UNIMPLEMENTED) __ = S_OK; return __; }
#else

#define DSOUND_RETURN(statement)\
{ HRESULT __ = (statement); if(__==DSOUND_UNIMPLEMENTED) __ = S_OK; return __; }

#endif

#define DSOUND_FINISH_(out,statement)\
{ out = (statement); if(out==DSOUND_UNIMPLEMENTED) out = S_OK; goto pophack; }

#define DSOUND_FINISH(statement) DSOUND_FINISH_(out,statement)

#endif //DIRECTX_INCLUDED

namespace DX
{
	#undef __DSOUND_INCLUDED__
		
	//see dx.ddraw.h etc.
	#ifndef DX__DX_DEFINED
	#undef D3DCOLOR_DEFINED
	#undef D3DVALUE_DEFINED
	#undef D3DVECTOR_DEFINED
	#undef D3DRECT_DEFINED
	#undef DX_SHARED_DEFINES
	#define DX__DX_DEFINED
	#endif	

	#define DIRECTSOUND_VERSION 0x0700

	struct IDirectSound;
	struct IDirectSoundBuffer;
	struct IDirectSound3DListener;
	struct IDirectSound3DBuffer;

	struct _DSBUFFERDESC;

	#include "dx8.1/dsound.h"

	#undef DIRECTSOUND_VERSION
}

struct IDirectSoundBuffer;

namespace DSOUND
{
	//REMOVE ME?
	extern int is_needed_to_initialize();
		
	extern unsigned noStops; //times paused
	extern unsigned noLoops; //times paused with looping

	//pause outstanding sound buffers
	extern void Stop(bool looping=true);
	extern void Play(bool looping=true); 

	//adjust volume of paused buffers
	extern void Sync(LONG mBs, LONG mBs3D);

	extern void playing_delays();

	//there can be only one knockout timeout
	//so 0 effectively cancels the knockout request
	//note this only effects delays (eg. sound effects)
	extern void knocking_out_delay(DWORD timeout_tick=-1);

	//manual release: use this only if you must
	extern void releasing_buffer(::IDirectSoundBuffer*);

	extern void multicasting_dinput_key_engaged(unsigned char keycode);

	struct IDirectSound;
	struct IDirectSoundBuffer;
	struct IDirectSound3DBuffer;
	struct IDirectSound3DListener;

	extern IDirectSound *DirectSound;
	extern IDirectSound8 *DirectSound8;

	extern IDirectSoundBuffer *PrimaryBuffer;

	extern IDirectSound3DListener *Listener;

	//DSOUND_LEVEL(7) << "~I()\n";
	extern void Yelp(const char *I);

	enum Hack
	{			
	DIRECTSOUND_QUERYINTERFACE_HACK = 0,
	DIRECTSOUND_SETCOOPERATIVELEVEL_HACK,
	DIRECTSOUNDBUFFER_SETVOLUME_HACK,	
	DIRECTSOUNDBUFFER_SETFREQUENCY_HACK,
	DIRECTSOUND3DLISTENER_SETORIENTATION_HACK,	
	DIRECTSOUND3DLISTENER_SETPOSITION_HACK,	
	DIRECTSOUND3DLISTENER_SETVELOCITY_HACK,	
	DIRECTSOUND3DBUFFER_SETPOSITION_HACK,	
	TOTAL_HACKS
	};

	//hack_interface: 
	//'f' is a pointer to a function which receives the interface
	//and all of the arguments for 'hack' by address like so...
	//
	// void *CreateSoundBuffer(HRESULT*,DSOUND::IDirectSound*,
	//			LPCDSBUFFERDESC&,LPDIRECTSOUNDBUFFER*&,LPUNKNOWN&);
	//
	//if another function pointer is returned it will be called after the interface
	bool hack_interface(DSOUND::Hack hack, void *f);

	static const int MAX_TABLES = (1+2); //DX7 and 8 for now

	static inline int v(int t) //vtable index
	{
		switch(t)
		{
		case 'dx7': return 1; //DirectX 7
		case 'dx8': return 2; //DirectX 8

		default: //reserved
			
			assert(0); return 0; 
		}
	}

	template<class T> T *is_proxy(void *p)
	{
		if(!p) return 0;

		void *v = *(void**)p; 

		for(int t=0;t<DSOUND::MAX_TABLES;t++)
		{
			if(T::vtables[t]==v&&v) return (T*)p;
		}
		return 0; 		
	}

	template<class X, class Y> void u_dlists(X *x, Y *y)
	{
		if(!x||!x->dlist||!y||!y->dlist) return;

		if(y->dlist==(void**)y) //y is singleton
		{
			void **swap = x->dlist; 
			
			x->dlist = (void**)y; y->dlist = swap;
		}
		else assert(0); //unimplemented
	}

	template<class T> void x_dlist(T *p)
	{
		if(!p||!p->dlist) return;  

		typedef void**(*dtor)(void*);

		void **d = p->dlist; 
		while(d!=(void**)p) d = ((dtor)d[1])(d);
	}

	//destructible_t:
	//useful for notification of release of an interface
	template<typename T> struct destructible_t
	{			
		T *tptr;		
		void **(*dtor)(destructible_t<T>*);
		void **dlist;

		inline operator void**(){ return (void**)this; }

		inline destructible_t(void **(*d)(destructible_t<T>*), T *t=0)
		{
			dtor = d; tptr = t; dlist = *this;
		}		

		static void **destruct(destructible_t<T> *d)
		{
			void **out = d->dlist; d->dlist = 0; delete d; return out;
		}
	};
}

#pragma push_macro("PURE") 
#pragma push_macro("STDMETHOD")
#pragma push_macro("STDMETHOD_") //for DirectX headers compatability

#define PURE
#define STDMETHOD(method) HRESULT STDMETHODCALLTYPE method //virtual HRESULT STDMETHODCALLTYPE method
#define	STDMETHOD_(type,method) type STDMETHODCALLTYPE method //virtual type STDMETHODCALLTYPE method

#define DSOUND_INTRAFACE(Intraface)\
public:\
\
	void register_tables(int t)\
	{\
		vtables[DSOUND::v(t)] = *(void**)this;\
		dtables[DSOUND::v(t)] = Intraface::destruct;\
	}\
\
	static void **destruct(Intraface *d)\
	{\
		void **out = d->dlist; d->dlist = 0; delete d; return out;\
	}\

#define DSOUND_INTERFACE(Interface)\
public:\
\
	DSOUND_INTRAFACE(Interface)\
\
	static void *vtables[DSOUND::MAX_TABLES];\
	static void *dtables[DSOUND::MAX_TABLES];\
\
	void **(*dtor)(void*); /*important! expected to be first member*/\
\
	void **dlist;\
\
	const int target; /*const for now*/\
\
	Interface():target(0),dtor(0),dlist(0){}\

#define DSOUND_CONSTRUCT(Interface)\
\
		*(void**)this = vtables[DSOUND::v(t)];\
\
		dtor = (void**(*)(void*))dtables[DSOUND::v(t)];\
\
		dlist = (void**)this;

#define DSOUND_DESTRUCT(Interface)\
\
		DSOUND::Yelp(#Interface);\
\
		DSOUND::x_dlist(this);

struct IDirectSound;

namespace DSOUND{
class IDirectSound : public DX::IDirectSound
{
DSOUND_INTERFACE(IDirectSound) //public

	union
	{
	DX::IDirectSound *proxy;
	::IDirectSound *proxy7;
	};
	
	IDirectSound(int t):proxy(0),target(t)
	{
		DSOUND_CONSTRUCT(IDirectSound)	
	}
		
	~IDirectSound()
	{ 
		if(!target) return;

		DSOUND_DESTRUCT(IDirectSound)
	}

	/*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IDirectSound methods ***/
	STDMETHOD(CreateSoundBuffer)(THIS_ DX::LPCDSBUFFERDESC, DX::LPDIRECTSOUNDBUFFER *, LPUNKNOWN) PURE;
	STDMETHOD(GetCaps)(THIS_ DX::LPDSCAPS) PURE;
    STDMETHOD(DuplicateSoundBuffer)(THIS_ DX::LPDIRECTSOUNDBUFFER, DX::LPDIRECTSOUNDBUFFER *) PURE;
    STDMETHOD(SetCooperativeLevel)(THIS_ HWND, DWORD) PURE;
    STDMETHOD(Compact)(THIS) PURE;
    STDMETHOD(GetSpeakerConfig)(THIS_ LPDWORD) PURE;
    STDMETHOD(SetSpeakerConfig)(THIS_ DWORD) PURE;
    STDMETHOD(Initialize)(THIS_ LPCGUID) PURE;
};}

struct IDirectSoundBuffer;
struct IDirectSound3DBuffer;

namespace DSOUND
{
	class IDirectSoundMaster;
}

namespace DSOUND{
class IDirectSoundBuffer : public DX::IDirectSoundBuffer
{
DSOUND_INTERFACE(IDirectSoundBuffer) //public

	union
	{
	DX::IDirectSoundBuffer *proxy;	
	::IDirectSoundBuffer *proxy7;
	};

	IDirectSoundMaster *master;
	IDirectSound3DBuffer *in3D;

	bool isPrimary;
	bool isLocked;
	
	DWORD isPaused; //bool
	DWORD isLooping; //if(isPaused)

	int pauserefs;

	bool isDuplicate;
	bool isForwarding;

	IDirectSoundBuffer *get_next, *get_prev;

	static IDirectSoundBuffer *get_head; 

	IDirectSoundBuffer(int t):proxy(0),target(t)
	{
		DSOUND_CONSTRUCT(IDirectSoundBuffer)

		master = 0; in3D = 0;

		isLocked = false; 

		isPrimary = isDuplicate = false;

		isPaused = isLooping = false; pauserefs = 0;

		isForwarding = false;

		get_next = get_head?get_head:this; 		

		get_prev = get_head?get_head->get_prev:this; 

		get_next->get_prev = this;
		get_prev->get_next = this;

		get_head = this;
	}

	~IDirectSoundBuffer()
	{
		if(!target) return;		

		get_prev->get_next = get_next; get_next->get_prev = get_prev;

		if(get_head==this) get_head = get_next==this?0:get_next;		

		get_prev = get_next = 0; 		

		DSOUND_DESTRUCT(IDirectSoundBuffer)
	}

	IDirectSoundBuffer *get(void *in)
	{
		if(!this||proxy==in) return this; //TODO: paranoia

		for(IDirectSoundBuffer *out=get_next;out&&out!=this;out=out->get_next)
		{		
			if((void*)out->proxy==in) return out; //TODO: assert(out)		
		}
		return 0; 
	}
					 
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;  
    /*** IDirectSound methods ***/
    STDMETHOD(GetCaps)(THIS_ DX::LPDSBCAPS) PURE;
    STDMETHOD(GetCurrentPosition)(THIS_ LPDWORD, LPDWORD) PURE;
    STDMETHOD(GetFormat)(THIS_ LPWAVEFORMATEX, DWORD, LPDWORD) PURE;
    STDMETHOD(GetVolume)(THIS_ LPLONG) PURE;
    STDMETHOD(GetPan)(THIS_ LPLONG) PURE;
    STDMETHOD(GetFrequency)(THIS_ LPDWORD) PURE;
    STDMETHOD(GetStatus)(THIS_ LPDWORD) PURE;
    STDMETHOD(Initialize)(THIS_ DX::LPDIRECTSOUND, DX::LPCDSBUFFERDESC) PURE;
    STDMETHOD(Lock)(THIS_ DWORD, DWORD, LPVOID *, LPDWORD, LPVOID *, LPDWORD, DWORD) PURE;
    STDMETHOD(Play)(THIS_ DWORD, DWORD, DWORD) PURE;
    STDMETHOD(SetCurrentPosition)(THIS_ DWORD) PURE;
    STDMETHOD(SetFormat)(THIS_ LPCWAVEFORMATEX) PURE;
    STDMETHOD(SetVolume)(THIS_ LONG) PURE;
    STDMETHOD(SetPan)(THIS_ LONG) PURE;
    STDMETHOD(SetFrequency)(THIS_ DWORD) PURE;
    STDMETHOD(Stop)(THIS) PURE;
    STDMETHOD(Unlock)(THIS_ LPVOID, DWORD, LPVOID, DWORD) PURE;
    STDMETHOD(Restore)(THIS) PURE;
};}
   
namespace DSOUND{
class IDirectSoundMaster : public DSOUND::IDirectSoundBuffer
{
public:	
	
	//Note: IDirectSoundMaster is not a DirectSound interface
	//This class represents the IDirectSoundBuffer from which 
	//all "duplicates" of the IDirectSoundBuffer were created

	DX::IDirectSoundBuffer *capture;	
	DX::IDirectSound3DBuffer *capture3D;		

	bool wasPlayed;

	float frequency; //average //DSOUND::Piano only???

	int duplicates;
	int forwarding;

	::IDirectSoundBuffer **forward7;
	::IDirectSound3DBuffer **forward3D7;	

	char *pausefwds; //pauserefs per forward 

	void playfwd7(IDirectSoundBuffer*dup,DWORD,DWORD,DWORD);
	void stopfwd7(IDirectSoundBuffer*dup);

	void movefwd7(float delta[3]); //2022

	UINT piano;

	inline void killpiano()
	{
		if(!piano) return;

		timeKillEvent(piano); piano = 0;

		DX::IDirectSoundBuffer *p = proxy;

		if(capture) p = capture;

		if(p)
		switch(target)
		{
		case 'dx7':	p->Stop(); break;

		default: assert(0);
		}
	}

	IDirectSoundMaster(int t):IDirectSoundBuffer(t)
	{
		//overwrite vtable
		*(void**)this=IDirectSoundBuffer::vtables[DSOUND::v(t)];

		master = this;

		capture = 0;
		capture3D = 0;
		
		wasPlayed = false;

		frequency = 0.0f;
	
		duplicates = 1;

		forwarding = 0;
		forward3D7 = 0;
		forward7 = 0;

		pausefwds = 0;

		piano = 0; 
	}

	~IDirectSoundMaster()
	{
		if(!target) return;

		//forward3D7 shares forward7 ptr
		delete[] forward7;
		delete[] pausefwds;
	}
};}


struct IDirectSound3DListener;

namespace DSOUND{
class IDirectSound3DListener : public DX::IDirectSound3DListener
{
DSOUND_INTERFACE(IDirectSound3DListener) //public

	int deaf; //REMOVE ME?

	enum{ deaf_s=60 }; //may need to relax for long load times

	union
	{
	DX::IDirectSound3DListener *proxy;
	::IDirectSound3DListener *proxy7;
	};
		
	IDirectSound3DListener(int t):proxy(0),target(t)
	{
		DSOUND_CONSTRUCT(IDirectSound3DListener)		

		deaf = 0;
	}

	~IDirectSound3DListener()
	{
		if(!target) return;

		DSOUND_DESTRUCT(IDirectSound3DListener)
	}			  
					 
    //IUnknown methods
    STDMETHOD(QueryInterface)(THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    //IDirectSound3DListener methods
    STDMETHOD(GetAllParameters)(THIS_ DX::LPDS3DLISTENER pListener) PURE;
    STDMETHOD(GetDistanceFactor)(THIS_ DX::D3DVALUE* pflDistanceFactor) PURE;
    STDMETHOD(GetDopplerFactor)(THIS_ DX::D3DVALUE* pflDopplerFactor) PURE;
    STDMETHOD(GetOrientation)(THIS_ DX::D3DVECTOR* pvOrientFront, DX::D3DVECTOR* pvOrientTop) PURE;
    STDMETHOD(GetPosition)(THIS_ DX::D3DVECTOR* pvPosition) PURE;
    STDMETHOD(GetRolloffFactor)(THIS_ DX::D3DVALUE* pflRolloffFactor) PURE;
    STDMETHOD(GetVelocity)(THIS_ DX::D3DVECTOR* pvVelocity) PURE;
	STDMETHOD(SetAllParameters)(THIS_ DX::LPCDS3DLISTENER pcListener, DWORD dwApply) PURE;
    STDMETHOD(SetDistanceFactor)(THIS_ DX::D3DVALUE flDistanceFactor, DWORD dwApply) PURE;
    STDMETHOD(SetDopplerFactor)(THIS_ DX::D3DVALUE flDopplerFactor, DWORD dwApply) PURE;
    STDMETHOD(SetOrientation)(THIS_ DX::D3DVALUE xFront, DX::D3DVALUE yFront, DX::D3DVALUE zFront, DX::D3DVALUE xTop, DX::D3DVALUE yTop, DX::D3DVALUE zTop, DWORD dwApply) PURE;
	STDMETHOD(SetPosition)(THIS_ DX::D3DVALUE x, DX::D3DVALUE y, DX::D3DVALUE z, DWORD dwApply) PURE;
    STDMETHOD(SetRolloffFactor)(THIS_ DX::D3DVALUE flRolloffFactor, DWORD dwApply) PURE;
    STDMETHOD(SetVelocity)(THIS_ DX::D3DVALUE x, DX::D3DVALUE y, DX::D3DVALUE z, DWORD dwApply) PURE;
    STDMETHOD(CommitDeferredSettings)(THIS) PURE;
};}

namespace DSOUND{
class IDirectSound3DBuffer : public DX::IDirectSound3DBuffer
{
DSOUND_INTERFACE(IDirectSound3DBuffer) //public

	union
	{
	DX::IDirectSound3DBuffer *proxy;
	::IDirectSound3DBuffer *proxy7;
	};

	IDirectSoundBuffer *source;

	IDirectSound3DBuffer(int t):proxy(0),target(t)
	{	
		DSOUND_CONSTRUCT(IDirectSound3DBuffer)		

		source = 0;
	}

	~IDirectSound3DBuffer()
	{
		if(!target) return;

		DSOUND_DESTRUCT(IDirectSound3DBuffer)
	}
									 
	//IUnknown methods
    STDMETHOD(QueryInterface)(THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    //IDirectSound3DBuffer methods
    STDMETHOD(GetAllParameters)(THIS_ DX::LPDS3DBUFFER pDs3dBuffer) PURE;
    STDMETHOD(GetConeAngles)(THIS_ LPDWORD pdwInsideConeAngle, LPDWORD pdwOutsideConeAngle) PURE;
    STDMETHOD(GetConeOrientation)(THIS_ DX::D3DVECTOR* pvOrientation) PURE;
    STDMETHOD(GetConeOutsideVolume)(THIS_ LPLONG plConeOutsideVolume) PURE;
    STDMETHOD(GetMaxDistance)(THIS_ DX::D3DVALUE* pflMaxDistance) PURE;
    STDMETHOD(GetMinDistance)(THIS_ DX::D3DVALUE* pflMinDistance) PURE;
    STDMETHOD(GetMode)(THIS_ LPDWORD pdwMode) PURE;
    STDMETHOD(GetPosition)(THIS_ DX::D3DVECTOR* pvPosition) PURE;
    STDMETHOD(GetVelocity)(THIS_ DX::D3DVECTOR* pvVelocity) PURE;
    STDMETHOD(SetAllParameters)(THIS_ DX::LPCDS3DBUFFER pcDs3dBuffer, DWORD dwApply) PURE;
    STDMETHOD(SetConeAngles)(THIS_ DWORD dwInsideConeAngle, DWORD dwOutsideConeAngle, DWORD dwApply) PURE;
    STDMETHOD(SetConeOrientation)(THIS_ DX::D3DVALUE x, DX::D3DVALUE y, DX::D3DVALUE z, DWORD dwApply) PURE;
    STDMETHOD(SetConeOutsideVolume)(THIS_ LONG lConeOutsideVolume, DWORD dwApply) PURE;
    STDMETHOD(SetMaxDistance)(THIS_ DX::D3DVALUE flMaxDistance, DWORD dwApply) PURE;
    STDMETHOD(SetMinDistance)(THIS_ DX::D3DVALUE flMinDistance, DWORD dwApply) PURE;
    STDMETHOD(SetMode)(THIS_ DWORD dwMode, DWORD dwApply) PURE;
    STDMETHOD(SetPosition)(THIS_ DX::D3DVALUE x, DX::D3DVALUE y, DX::D3DVALUE z, DWORD dwApply) PURE;
    STDMETHOD(SetVelocity)(THIS_ DX::D3DVALUE x, DX::D3DVALUE y, DX::D3DVALUE z, DWORD dwApply) PURE;
};}

//see: push_macro("PURE")
#pragma pop_macro("PURE") 
#pragma pop_macro("STDMETHOD")
#pragma pop_macro("STDMETHOD_")

#endif //DX_DSOUND_INCLUDED