#ifndef DX_DINPUT_INCLUDED
#define DX_DINPUT_INCLUDED
	  
namespace DINPUT
{
	extern HINSTANCE dll;

	extern unsigned noPolls; //keyboard

	extern bool doData; //use GetDeviceData in place of State

	//Querty: returns 1 for q, 2 for w, and so on thru the alphabet.
	//27 (1+26) for Q. 0 otherwise. Follows the Qwerty keyboard layout.
	extern int Qwerty(unsigned char dik, bool shift=0, bool capslock=0);

	static bool doIDirectInputA = 1;
	static bool doIDirectInputDeviceA = 1;
	static bool doIDirectInputDevice2A = 1;

	static bool isAvailable = true; //TODO: extern

	static bool can_support_window()
	{
		return DINPUT::isAvailable&&doIDirectInputA&&doIDirectInputDeviceA;
	}

	//2017: When breaking into the debugger DirectInput stops updating
	//the state, but behaves normal otherwise.
	extern bool repair_broken_Keyboard();
}

#ifdef DIRECTX_INCLUDED

namespace DINPUTLOG
{
	static int debug = 0; //debugging
	static int error = 7; //serious errors
	static int panic = 7; //undefined
	static int alert = 7; //warning
	static int hello = 7; //fun stuff
	static int sorry = 7; //woops
	
	static int *master = DX_LOG(DInput);	
}

#define DINPUT_LEVEL(lv) if(lv<=DINPUTLOG::debug&&lv<=*DINPUTLOG::master) dx.log
#define DINPUT_ERROR(lv) if(lv<=DINPUTLOG::error&&lv<=*DINPUTLOG::master) dx.log
#define DINPUT_PANIC(lv) if(lv<=DINPUTLOG::panic&&lv<=*DINPUTLOG::master) dx.log
#define DINPUT_ALERT(lv) if(lv<=DINPUTLOG::alert&&lv<=*DINPUTLOG::master) dx.log
#define DINPUT_HELLO(lv) if(lv<=DINPUTLOG::hello&&lv<=*DINPUTLOG::master) dx.log
#define DINPUT_SORRY(lv) if(lv<=DINPUTLOG::sorry&&lv<=*DINPUTLOG::master) dx.log

namespace DINPUT
{
	static char *error(HRESULT);
}

static const HRESULT DINPUT_UNIMPLEMENTED = 2;

#ifndef NDEBUG

#define DINPUT_RETURN(statement){ HRESULT __ = (statement);\
	if(__) DINPUT_ALERT(2) << "DirectX interface returned error " << dx%DINPUT::error(__) << '(' << std::hex << __ << ')' << " in " << dx%__FILE__ << " at " << std::dec << __LINE__ << " (" << dx%__FUNCTION__ ")\n";\
		assert(!__||DDRAW::fullscreen); if(__==DINPUT_UNIMPLEMENTED) __ = S_OK; return __; }

#define DINPUT_REPORT(statement){ HRESULT __ = (statement);\
	if(__) DINPUT_ALERT(6) << "DirectX interface returned error " << dx%DINPUT::error(__) << '(' << std::hex << __ << ')' << " in " << dx%__FILE__ << " at " << std::dec << __LINE__ << " (" << dx%__FUNCTION__ ")\n";\
		if(__==DINPUT_UNIMPLEMENTED) __ = S_OK; return __; }
#else

#define DINPUT_RETURN(statement)\
{ HRESULT __ = (statement); if(__==DINPUT_UNIMPLEMENTED) __ = S_OK; return __; }

#define DINPUT_REPORT(statement)\
{ HRESULT __ = (statement); if(__==DINPUT_UNIMPLEMENTED) __ = S_OK; return __; }

#endif

#define DINPUT_FINISH_(out,statement)\
{ out = (statement); if(out==DINPUT_UNIMPLEMENTED) out = S_OK; goto pophack; }

#define DINPUT_FINISH(statement) DINPUT_FINISH_(out,statement)

#endif //DIRECTX_INCLUDED

namespace DX
{
	#undef __DINPUT_INCLUDED__
/*		
	//see dx.dsound.h etc.
	#ifndef DX__DX_DEFINED
	#undef D3DCOLOR_DEFINED
	#undef D3DVALUE_DEFINED
	#undef D3DVECTOR_DEFINED
	#undef D3DRECT_DEFINED
	#undef DX_SHARED_DEFINES
	#define DX__DX_DEFINED
	#endif	
 */
	#define DIRECTINPUT_VERSION 0x0700
		
	struct IDirectInputA;
	struct IDirectInput2A;
	struct IDirectInputDeviceA;
	struct IDirectInputDevice2A;

	//Note: these are extern "C" {
	#define c_dfDIMouse DX__c_dfDIMouse
	#define c_dfDIMouse2 DX__c_dfDIMouse2
	#define c_dfDIKeyboard DX__c_dfDIKeyboard
	#define c_dfDIJoystick DX__c_dfDIJoystick

	#include "dx8.1/dinput.h"

	#undef c_dfDIJoystick
	#undef c_dfDIKeyboard	
	#undef c_dfDIMouse2
	#undef c_dfDIMouse

	extern const DIDATAFORMAT &c_dfDIMouse;
	extern const DIDATAFORMAT &c_dfDIMouse2;
	extern const DIDATAFORMAT &c_dfDIKeyboard;
	extern const DIDATAFORMAT &c_dfDIJoystick;

	#undef DIRECTINPUT_VERSION
}

namespace DINPUT
{								  
	//REMOVE ME?
	extern int is_needed_to_initialize();

	typedef HRESULT (WINAPI *CreateA)(HINSTANCE,DWORD,DX::LPDIRECTINPUTA*,LPUNKNOWN);
	typedef HRESULT (WINAPI *CreateW)(HINSTANCE,DWORD,DX::LPDIRECTINPUTW*,LPUNKNOWN);
	
	namespace DLL
	{		
		extern DINPUT::CreateA DirectInputCreateA;	 
		extern DINPUT::CreateW DirectInputCreateW;	 
	}

	class IDirectInputA; //DINPUT::

	extern DINPUT::IDirectInputA *DirectInputA;

	class IDirectInputDeviceA; //DINPUT::
	class IDirectInputDevice2A; //DINPUT::

	extern DINPUT::IDirectInputDeviceA *Keyboard;
	extern DINPUT::IDirectInputDeviceA *Joystick;
	extern DINPUT::IDirectInputDeviceA *Joystick2;

 	//DINPUT_LEVEL(7) << "~I()\n";
	extern void Yelp(const char *I);

	enum Hack
	{			
	DIRECTINPUTA_QUERYINTERFACE_HACK = 0,
	DIRECTINPUTA_ENUMDEVICES_HACK,
	DIRECTINPUTA_CREATEDEVICE_HACK,
	DIRECTINPUTDEVICEA_GETDEVICESTATE_HACK,
	TOTAL_HACKS
	};

	//hack_interface: 
	//'f' is a pointer to a function which receives the interface
	//and all of the arguments for 'hack' by address like so...
	//
	// void *DrawPrimitive(HRESULT*,DINPUT::IDirect3DDevice7*, 
	//			D3DPRIMITIVETYPE&,DWORD&,LPVOID&,DWORD&,DWORD&);
	//
	//if another function pointer is returned it will be called after the interface
	bool hack_interface(DINPUT::Hack hack, void *f);

	static const int MAX_TABLES = (1+2); //DX7 and 8 for now

	static inline int v(int t) //vtable index
	{
		switch(t)
		{
		case 'dx7': return 1; //DirectX 7
		case 'dx8': return 2; //DirectX 8

		default: return 0; 
		}
	}

	template<class T> T *is_proxy(void *p)
	{
		if(!p) return 0;

		void *v = *(void**)p; 

		for(int t=0;t<DINPUT::MAX_TABLES;t++)

			if(T::vtables[t]==v&&v) return (T*)p;
		
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

			while(d!=(void**)p)			

				d = ((dtor)d[1])(d);
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
#define STDMETHOD(method)  HRESULT STDMETHODCALLTYPE method //virtual HRESULT STDMETHODCALLTYPE method
#define	STDMETHOD_(type,method) type STDMETHODCALLTYPE method //virtual type STDMETHODCALLTYPE method

#define DINPUT_INTRAFACE(Interface)\
public:\
\
	void register_tables(int t)\
	{\
		vtables[DINPUT::v(t)] = *(void**)this;\
		dtables[DINPUT::v(t)] = Interface::destruct;\
	}\
\
	static void **destruct(Interface *d)\
	{\
		void **out = d->dlist; d->dlist = 0; delete d; return out;\
	}\

#define DINPUT_INTERFACE(Interface)\
\
	DINPUT_INTRAFACE(Interface)\
\
	static void *vtables[DINPUT::MAX_TABLES];\
	static void *dtables[DINPUT::MAX_TABLES];\
\
	void **(*dtor)(void*); /*important! expected to be first member*/\
\
	void **dlist;\
\
	const int target; /*const for now*/\
\
	Interface():target(0),dtor(0),dlist(0){}\

#define DINPUT_CONSTRUCT(Interface)\
\
		*(void**)this = vtables[DINPUT::v(t)];\
\
		dtor = (void**(*)(void*))dtables[DINPUT::v(t)];\
\
		dlist = (void**)this;

#define DINPUT_DESTRUCT(Interface)\
\
		DINPUT::Yelp(#Interface);\
\
		DINPUT::x_dlist(this);

struct IDirectInputA;

namespace DINPUT{
class IDirectInputA : public DX::IDirectInputA
{
DINPUT_INTERFACE(IDirectInputA) //public
	
	union
	{
	DX::IDirectInputA *proxy;
	::IDirectInputA *proxy7;
	};

	IDirectInputA(int t):proxy(0),target(t)
	{
		DINPUT_CONSTRUCT(IDirectInputA)
	}

	~IDirectInputA()
	{
		if(!target) return;

		DINPUT_DESTRUCT(IDirectInputA)
	}

     /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    /*** IDirectInputA methods ***/
    STDMETHOD(CreateDevice)(THIS_ REFGUID,DX::LPDIRECTINPUTDEVICEA *,LPUNKNOWN) PURE;
    STDMETHOD(EnumDevices)(THIS_ DWORD,DX::LPDIENUMDEVICESCALLBACKA,LPVOID,DWORD) PURE;
    STDMETHOD(GetDeviceStatus)(THIS_ REFGUID) PURE;
    STDMETHOD(RunControlPanel)(THIS_ HWND,DWORD) PURE;
    STDMETHOD(Initialize)(THIS_ HINSTANCE,DWORD) PURE;

    /*** IDirectInput2A methods ***/
    //STDMETHOD(FindDevice)(THIS_ REFGUID,LPCSTR,LPGUID) PURE;
};}

struct IDirectInputDeviceA;
struct IDirectInputDeviceW;
struct IDirectInputDevice2A;
struct IDirectInputDevice2W;

namespace DINPUT{ //Notice: may drop in favor of 2A only
class IDirectInputDeviceA : public DX::IDirectInputDeviceA
{
DINPUT_INTERFACE(IDirectInputDeviceA) //public

	union
	{
	DX::IDirectInputDeviceA *proxy; 
	::IDirectInputDeviceA *proxy7; 
	};

	CHAR product[MAX_PATH]; GUID instance;

	GUID productID; //NEW: EX::IsXInputDevice (Ex.input.h)

	DX::LPCDIDATAFORMAT format; //hack: assuming persistent object, eg. c_dfDIKeyboard

	unsigned char *rawdata; DWORD rawdatasize;

	DWORD buffersize, bufferdata;

	::IDirectInputDeviceW *proxyW; 		
				
	DINPUT::IDirectInputDevice2A *as2A;

	IDirectInputDeviceA(int t):proxy(0),target(t)
	{ 
		DINPUT_CONSTRUCT(IDirectInputDeviceA) 
			
		*product = '\0'; format = 0;
			
		rawdata = 0; rawdatasize = 0; 
			
		buffersize = bufferdata = 0;
 
		proxyW = 0; as2A = 0;
	}

	//data: sets bufferdata to per unit size of returned buffer
	DX::DIDEVICEOBJECTDATA *data(size_t perunitsz=sizeof(DX::DIDEVICEOBJECTDATA)); 

	~IDirectInputDeviceA()
	{
		if(!target) return;
				
		DINPUT_DESTRUCT(IDirectInputDeviceA)
	}

    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    /*** IDirectInputDeviceA methods ***/
    STDMETHOD(GetCapabilities)(THIS_ DX::LPDIDEVCAPS) PURE;
    STDMETHOD(EnumObjects)(THIS_ DX::LPDIENUMDEVICEOBJECTSCALLBACKA,LPVOID,DWORD) PURE;
    STDMETHOD(GetProperty)(THIS_ REFGUID,DX::LPDIPROPHEADER) PURE;
    STDMETHOD(SetProperty)(THIS_ REFGUID,DX::LPCDIPROPHEADER) PURE;
    STDMETHOD(Acquire)(THIS) PURE;
    STDMETHOD(Unacquire)(THIS) PURE;
    STDMETHOD(GetDeviceState)(THIS_ DWORD,LPVOID) PURE;
    STDMETHOD(GetDeviceData)(THIS_ DWORD,DX::LPDIDEVICEOBJECTDATA,LPDWORD,DWORD) PURE;
    STDMETHOD(SetDataFormat)(THIS_ DX::LPCDIDATAFORMAT) PURE;
    STDMETHOD(SetEventNotification)(THIS_ HANDLE) PURE;
    STDMETHOD(SetCooperativeLevel)(THIS_ HWND,DWORD) PURE;
    STDMETHOD(GetObjectInfo)(THIS_ DX::LPDIDEVICEOBJECTINSTANCEA,DWORD,DWORD) PURE;
    STDMETHOD(GetDeviceInfo)(THIS_ DX::LPDIDEVICEINSTANCEA) PURE;
    STDMETHOD(RunControlPanel)(THIS_ HWND,DWORD) PURE;
    STDMETHOD(Initialize)(THIS_ HINSTANCE,DWORD,REFGUID) PURE;

	HRESULT GetObjectInfo(DX::LPDIDEVICEOBJECTINSTANCEW,DWORD,DWORD);
};}

namespace DINPUT{
class IDirectInputDevice2A : public DX::IDirectInputDevice2A
{
DINPUT_INTERFACE(IDirectInputDevice2A) //public

	union
	{
	DX::IDirectInputDevice2A *proxy; 
	::IDirectInputDevice2A *proxy7;	
	};
	
	::IDirectInputDevice2W *proxyW; 
 
	DINPUT::IDirectInputDeviceA *asA;

	IDirectInputDevice2A(int t):proxy(0),target(t)
	{ 
		DINPUT_CONSTRUCT(IDirectInputDevice2A) 
			
		proxyW = 0;	asA = 0;
	}

	~IDirectInputDevice2A()
	{
		if(!target) return;

		DINPUT_DESTRUCT(IDirectInputDevice2A)
	}

	    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    /*** IDirectInputDeviceA methods ***/
    STDMETHOD(GetCapabilities)(THIS_ DX::LPDIDEVCAPS) PURE;
    STDMETHOD(EnumObjects)(THIS_ DX::LPDIENUMDEVICEOBJECTSCALLBACKA,LPVOID,DWORD) PURE;
    STDMETHOD(GetProperty)(THIS_ REFGUID,DX::LPDIPROPHEADER) PURE;
    STDMETHOD(SetProperty)(THIS_ REFGUID,DX::LPCDIPROPHEADER) PURE;
    STDMETHOD(Acquire)(THIS) PURE;
    STDMETHOD(Unacquire)(THIS) PURE;
    STDMETHOD(GetDeviceState)(THIS_ DWORD,LPVOID) PURE;
    STDMETHOD(GetDeviceData)(THIS_ DWORD,DX::LPDIDEVICEOBJECTDATA,LPDWORD,DWORD) PURE;
    STDMETHOD(SetDataFormat)(THIS_ DX::LPCDIDATAFORMAT) PURE;
    STDMETHOD(SetEventNotification)(THIS_ HANDLE) PURE;
    STDMETHOD(SetCooperativeLevel)(THIS_ HWND,DWORD) PURE;
    STDMETHOD(GetObjectInfo)(THIS_ DX::LPDIDEVICEOBJECTINSTANCEA,DWORD,DWORD) PURE;
    STDMETHOD(GetDeviceInfo)(THIS_ DX::LPDIDEVICEINSTANCEA) PURE;
    STDMETHOD(RunControlPanel)(THIS_ HWND,DWORD) PURE;
    STDMETHOD(Initialize)(THIS_ HINSTANCE,DWORD,REFGUID) PURE;

    /*** IDirectInputDevice2A methods ***/
    STDMETHOD(CreateEffect)(THIS_ REFGUID,DX::LPCDIEFFECT,DX::LPDIRECTINPUTEFFECT *,LPUNKNOWN) PURE;
    STDMETHOD(EnumEffects)(THIS_ DX::LPDIENUMEFFECTSCALLBACKA,LPVOID,DWORD) PURE;
    STDMETHOD(GetEffectInfo)(THIS_ DX::LPDIEFFECTINFOA,REFGUID) PURE;
    STDMETHOD(GetForceFeedbackState)(THIS_ LPDWORD) PURE;
    STDMETHOD(SendForceFeedbackCommand)(THIS_ DWORD) PURE;
    STDMETHOD(EnumCreatedEffectObjects)(THIS_ DX::LPDIENUMCREATEDEFFECTOBJECTSCALLBACK,LPVOID,DWORD) PURE;
    STDMETHOD(Escape)(THIS_ DX::LPDIEFFESCAPE) PURE;
    STDMETHOD(Poll)(THIS) PURE;
    STDMETHOD(SendDeviceData)(THIS_ DWORD,DX::LPCDIDEVICEOBJECTDATA,LPDWORD,DWORD) PURE;
};}
#undef STDMETHOD_
#undef STDMETHOD
#undef PURE

//see: push_macro("PURE")
#pragma pop_macro("PURE") 
#pragma pop_macro("STDMETHOD")
#pragma pop_macro("STDMETHOD_")

#endif //DX_DINPUT_INCLUDED