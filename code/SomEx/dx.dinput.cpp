
#include "directx.h" 
DX_TRANSLATION_UNIT

#define DIRECTINPUT_VERSION 0x0700

#include "dx8.1/dinput.h"

#pragma comment(lib,"dinput.lib")

//LEGACY HACKS:
//do not belong (at all)
#include "Ex.h"
#include "Ex.ini.h" 
#include "Ex.output.h"
#include "Ex.input.h" 
#include "Ex.window.h"

#include "dx.dinput.h"
 
namespace DDRAW
{
	extern HWND window;
	extern bool fullscreen; //DINPUT_RETURN
}

static char *DINPUT::error(HRESULT err)
{
#define CASE_(E) case E: return #E;

	switch(err)
	{
	CASE_(DINPUT_UNIMPLEMENTED)

	CASE_(S_FALSE) CASE_(E_FAIL)
	}

	return "";

#undef CASE_
}

#define DX_INPUT_FORMAT(c)\
extern const DX::DIDATAFORMAT &DX::c = *(DX::DIDATAFORMAT*)&::c;

DX_INPUT_FORMAT(c_dfDIMouse)
DX_INPUT_FORMAT(c_dfDIMouse2)
DX_INPUT_FORMAT(c_dfDIKeyboard)
DX_INPUT_FORMAT(c_dfDIJoystick)

extern DINPUT::IDirectInputA *DINPUT::DirectInputA = 0;

extern DINPUT::IDirectInputDeviceA *DINPUT::Keyboard = 0;
extern DINPUT::IDirectInputDeviceA *DINPUT::Joystick = 0;
extern DINPUT::IDirectInputDeviceA *DINPUT::Joystick2 = 0;

extern unsigned DINPUT::noPolls = 0;

extern bool DINPUT::doData = false;

static bool dx_dinput_widen(DINPUT::IDirectInputDeviceA *a)
{
	if(!a||!a->proxy) return false; if(a->proxyW) return true;

	return a->proxy->QueryInterface(IID_IDirectInputDeviceW,(LPVOID*)&a->proxyW)==S_OK;
}
 
extern void DINPUT::Yelp(const char *Interface)
{
	DINPUT_LEVEL(7) << '~' << Interface << "()\n";
}

extern int DINPUT::Qwerty(unsigned char dik, bool shift, bool capslock)
{
	int add = shift?(capslock?0:26):(capslock?26:0);

#define _(A,N) case DIK_##A: return N+add;

	switch(dik) //alphabetical keys only
	{
	_(Q, 1)_(W, 2)_(E, 3)_(R, 4)_(T, 5)_(Y, 6)_(U, 7)_(I, 8)_(O, 9)_(P,10) 
	   _(A,11)_(S,12)_(D,13)_(F,14)_(G,15)_(H,16)_(J,17)_(K,18)_(L,19) 
	      _(Z,20)_(X,21)_(C,22)_(V,23)_(B,24)_(N,25)_(M,26)
	}

#undef _

	return 0;
}

////////////////////////////////////////////////////////
//              DIRECTX7 INTERFACES                   //
////////////////////////////////////////////////////////


static void **dx_dinput_hacks = 0; 

#define DINPUT_PUSH_HACK(h,...) \
\
	void *hP,*hK;\
	for(hP=0,hK=dx_dinput_hacks?dx_dinput_hacks[DINPUT::h##_HACK]:0;hK;hK=0)hP=((void*(*)(HRESULT*,DINPUT::__VA_ARGS__))hK)

#define DINPUT_POP_HACK(h,...) \
\
	pophack: if(hP) ((void*(*)(HRESULT*,DINPUT::__VA_ARGS__))hP)

#define DINPUT_PUSH_HACK_IF(cond,h,...) \
\
	void *hP=0,*hK=dx_dinput_hacks?dx_dinput_hacks[DINPUT::h##_HACK]:0;\
	if(cond) for(;hK;hK=0)hP=((void*(*)(HRESULT*,DINPUT::__VA_ARGS__))hK)

bool DINPUT::hack_interface(DINPUT::Hack hack, void *f)
{	
	if(hack<0||hack>=DINPUT::TOTAL_HACKS) return false;

	if(!dx_dinput_hacks)
	{
		if(!f) return true;
		
		dx_dinput_hacks = new void*[DINPUT::TOTAL_HACKS];

		memset(dx_dinput_hacks,0x00,sizeof(void*)*DINPUT::TOTAL_HACKS);
	}

	dx_dinput_hacks[hack] = f; return true;
}


HRESULT DINPUT::IDirectInputA::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	DINPUT_LEVEL(7) << "IDirectInputA::QueryInterface()\n";

	LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DINPUT_LEVEL(7) << ' ' << p << '\n';

	DINPUT_RETURN(proxy->QueryInterface(riid,ppvObj))
}
ULONG DINPUT::IDirectInputA::AddRef()
{
	DINPUT_LEVEL(7) << "IDirectInputA::AddRef()\n";

	return proxy->AddRef();
}
ULONG DINPUT::IDirectInputA::Release()
{
	DINPUT_LEVEL(7) << "IDirectInputA::Release()\n";

	ULONG out = proxy->Release(); 

	if(out==0)
	{
		if(DINPUT::DirectInputA==this) DINPUT::DirectInputA = 0;

		delete this; 
	}

	return out;
}
HRESULT DINPUT::IDirectInputA::CreateDevice(REFGUID x, DX::LPDIRECTINPUTDEVICEA *y, LPUNKNOWN z)
{
	DINPUT_LEVEL(7) << "IDirectInputA::CreateDevice()\n";
	
	static OLECHAR g[64]; StringFromGUID2(x,g,64); DINPUT_LEVEL(7) << ' ' << g << '\n';

	DINPUT_PUSH_HACK(DIRECTINPUTA_CREATEDEVICE,IDirectInputA*,
		REFGUID,DX::LPDIRECTINPUTDEVICEA*&,LPUNKNOWN&)(0,this,x,y,z);
	
	if(x==GUID_SysKeyboard) //{6F1D2B61-D5A0-11CF-BFC7-444553540000}
	{
		DINPUT_LEVEL(7) << "(SysKeyboard)\n";
	}
	else; //maybe try to query some information about it??
		
	//return proxy->CreateDevice(x,y,z);
										  
	DX::LPDIRECTINPUTDEVICEA q = 0; 

	HRESULT out = proxy->CreateDevice(x,(DX::LPDIRECTINPUTDEVICEA*)&q,z);

	if(out!=DI_OK){ DINPUT_LEVEL(7) << "IDirectDraw7::CreateDevice() Failed\n"; DINPUT_FINISH(out) }

	DX::DIDEVICEINSTANCEA info; info.dwSize = sizeof(info);

	q->GetDeviceInfo(&info); DINPUT_LEVEL(7) << "Product Info: " << info.tszProductName << '\n';
	
	if(DINPUT::doIDirectInputDeviceA)
	{	
		DX::LPDIRECTINPUTDEVICEA did = q;
		
		DINPUT::IDirectInputDeviceA *p = new DINPUT::IDirectInputDeviceA('dx7');

		strcpy(p->product,info.tszProductName);	p->instance = x;

		p->productID = info.guidProduct;

		p->proxy = did; *y = (DX::LPDIRECTINPUTDEVICEA)p;
	}
	else *y = q; 

	DINPUT_POP_HACK(DIRECTINPUTA_CREATEDEVICE,IDirectInputA*,
		REFGUID,DX::LPDIRECTINPUTDEVICEA*&,LPUNKNOWN&)(&out,this,x,y,z);	   

	DINPUT_RETURN(out);
}

HRESULT DINPUT::IDirectInputA::EnumDevices(DWORD x, DX::LPDIENUMDEVICESCALLBACKA y, LPVOID z, DWORD w)
{
	DINPUT_LEVEL(7) << "IDirectInputA::EnumDevices()\n";

	DINPUT_LEVEL(4) << "Enumerating...\n";

	DINPUT_PUSH_HACK(DIRECTINPUTA_ENUMDEVICES,IDirectInputA*,
	DWORD&,DX::LPDIENUMDEVICESCALLBACKA&,LPVOID&,DWORD&)(0,this,x,y,z,w);

	HRESULT out = DI_OK;

	switch(x)
	{
	case DIDEVTYPE_DEVICE:	 DINPUT_LEVEL(4) << " DIDEVTYPE_DEVICE -- "; break;
	case DIDEVTYPE_MOUSE:	 DINPUT_LEVEL(4) << " DIDEVTYPE_MOUSE -- "; break;
	case DIDEVTYPE_KEYBOARD: DINPUT_LEVEL(4) << " DIDEVTYPE_KEYBOARD -- "; break;
	case DIDEVTYPE_JOYSTICK: DINPUT_LEVEL(4) << " DIDEVTYPE_JOYSTICK -- "; break;
	case DIDEVTYPE_HID:	     DINPUT_LEVEL(4) << " DIDEVTYPE_HID\n"; break;

	default: DINPUT_LEVEL(4) << " Unrecognized device type?? -- "; break;
	}

	if(z) DINPUT_LEVEL(4) << z << " -- ";

	switch(w&DIEDFL_ATTACHEDONLY)
	{
	case DIEDFL_ALLDEVICES: DINPUT_LEVEL(4) << "DIEDFL_ALLDEVICES"; break;

	case DIEDFL_ATTACHEDONLY: DINPUT_LEVEL(4) << "DIEDFL_ATTACHEDONLY"; break;
	}

	if(w&DIEDFL_FORCEFEEDBACK)	 DINPUT_LEVEL(4) << "/DIEDFL_FORCEFEEDBACK";
//	if(w&DIEDFL_INCLUDEALIASES)  DINPUT_LEVEL(4) << "/DIEDFL_INCLUDEALIASES";
//	if(w&DIEDFL_INCLUDEPHANTOMS) DINPUT_LEVEL(4) << "/DIEDFL_INCLUDEPHANTOMS";

	DINPUT_LEVEL(4) << '\n';
				
	if(y) out = proxy->EnumDevices(x,y,z,w);
	
	DINPUT_POP_HACK(DIRECTINPUTA_ENUMDEVICES,IDirectInputA*,
	DWORD&,DX::LPDIENUMDEVICESCALLBACKA&,LPVOID&,DWORD&)(&out,this,x,y,z,w);	   

	DINPUT_RETURN(out)
}
HRESULT DINPUT::IDirectInputA::GetDeviceStatus(REFGUID x)
{
	DINPUT_LEVEL(7) << "IDirectInputA::GetDeviceStatus()\n";

	static OLECHAR g[64]; StringFromGUID2(x,g,64); DINPUT_LEVEL(7) << ' ' << g << '\n';

	DINPUT_RETURN(proxy->GetDeviceStatus(x))
}
HRESULT DINPUT::IDirectInputA::RunControlPanel(HWND x, DWORD y)
{
	DINPUT_LEVEL(7) << "IDirectInputA::RunControlPanel()\n";

	DINPUT_RETURN(proxy->RunControlPanel(x,y))
}
HRESULT DINPUT::IDirectInputA::Initialize(HINSTANCE x, DWORD y)
{
	DINPUT_LEVEL(7) << "IDirectInputA::Initialize()\n";

	DINPUT_RETURN(proxy->Initialize(x,y))
}
/*
HRESULT DINPUT::IDirectInputA::FindDevice(REFGUID x, LPCSTR y, LPGUID z)
{
	DINPUT_LEVEL(7) << "IDirectInputA::FindDevice()\n";

	static OLECHAR g[64]; StringFromGUID2(x,g,64); DINPUT_LEVEL(7) << ' ' << g << '\n';

	DINPUT_RETURN(proxy->FindDevice(x,y,z))
}
*/

HRESULT DINPUT::IDirectInputDeviceA::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	DINPUT_LEVEL(7) << "IDirectInputDeviceA::QueryInterface()\n";

	DINPUT_LEVEL(7) << product << '\n';

	LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DINPUT_LEVEL(7) << ' ' << p << '\n';

	if(riid==IID_IDirectInputDevice2A)
	{
		DINPUT_LEVEL(7) << "(IDirectInputDevice2A)" << '\n';
				
		::IDirectInputDevice2A *q = 0; 

		HRESULT out = proxy->QueryInterface(riid,(LPVOID FAR*)&q); 
		
		if(out!=S_OK)
		{
			DINPUT_LEVEL(7) << "IDirectInputDeviceA::QueryInterface() Failed\n"; 
			
			DINPUT_RETURN(out)
		}

		if(DINPUT::doIDirectInputDevice2A)
		{	
			::IDirectInputDevice2A *did2 = q;
			
			DINPUT::IDirectInputDevice2A *p = new DINPUT::IDirectInputDevice2A('dx7');
							
			p->proxy7 = did2; p->asA = this; as2A = p; p->proxy7 = did2; 

			DINPUT::u_dlists(this,p); *ppvObj = p;
		} 
		else *ppvObj = q;		

		DINPUT_RETURN(out)
	}
	else assert(0);

	DINPUT_RETURN(proxy->QueryInterface(riid,ppvObj))
}
ULONG DINPUT::IDirectInputDeviceA::AddRef()
{
	DINPUT_LEVEL(7) << "IDirectInputDeviceA::AddRef()\n";

	return proxy->AddRef();
}
ULONG DINPUT::IDirectInputDeviceA::Release()
{
	DINPUT_LEVEL(7) << "IDirectInputDeviceA::Release()\n";

	DINPUT_LEVEL(7) << product << '\n';

	ULONG out = proxy->Release(); 

	if(out==0)
	{
		if(DINPUT::Keyboard==this) DINPUT::Keyboard = 0;
		if(DINPUT::Joystick==this) DINPUT::Joystick = 0;
		if(DINPUT::Joystick2==this) DINPUT::Joystick2 = 0;

		proxy = 0; if(!as2A||!as2A->proxy) delete this; 
	}

	return out;
}
HRESULT DINPUT::IDirectInputDeviceA::GetCapabilities(DX::LPDIDEVCAPS x)
{
	DINPUT_LEVEL(7) << "IDirectInputDeviceA::GetCapabilities()\n";

	DINPUT_LEVEL(7) << product << '\n';

	DINPUT_RETURN(proxy->GetCapabilities(x))
}
HRESULT DINPUT::IDirectInputDeviceA::EnumObjects(DX::LPDIENUMDEVICEOBJECTSCALLBACKA x, LPVOID y, DWORD z)
{
	DINPUT_LEVEL(7) << "IDirectInputDeviceA::EnumObjects()\n";

	DINPUT_RETURN(proxy->EnumObjects(x,y,z))
}
HRESULT DINPUT::IDirectInputDeviceA::GetProperty(REFGUID x, DX::LPDIPROPHEADER y)
{
	DINPUT_LEVEL(7) << "IDirectInputDeviceA::GetProperty()\n";

	if((size_t)&x>64) //see DIPROP_BUFFERSIZE etc.
	{
		static OLECHAR g[64]; StringFromGUID2(x,g,64); DINPUT_LEVEL(7) << ' ' << g << '\n';
	}

	DINPUT_RETURN(proxy->GetProperty(x,y))
}
HRESULT DINPUT::IDirectInputDeviceA::SetProperty(REFGUID x, DX::LPCDIPROPHEADER y)
{
	DINPUT_LEVEL(7) << "IDirectInputDeviceA::SetProperty()\n";
									  
	DINPUT_LEVEL(7) << product << '\n';

	if((size_t)&x>64) //see DIPROP_BUFFERSIZE etc.
	{
		static OLECHAR g[64]; StringFromGUID2(x,g,64); DINPUT_LEVEL(7) << ' ' << g << '\n';
	}
	else switch((size_t)&x)
	{
	case 1: DINPUT_LEVEL(7) << "DIPROP_BUFFERSIZE\n"; break;
	case 2: DINPUT_LEVEL(7) << "DIPROP_AXISMODE\n"; break;
	case 3: DINPUT_LEVEL(7) << "DIPROP_GRANULARITY\n"; break;
	case 4: DINPUT_LEVEL(7) << "DIPROP_RANGE\n"; break;	//REMINDER: som_db likes -5120,5120
	case 5: DINPUT_LEVEL(7) << "DIPROP_DEADZONE\n"; break;
	case 6: DINPUT_LEVEL(7) << "DIPROP_SATURATION\n"; break;
	case 7: DINPUT_LEVEL(7) << "DIPROP_FFGAIN\n"; break;
	case 8: DINPUT_LEVEL(7) << "DIPROP_FFLOAD\n"; break;
	case 9: DINPUT_LEVEL(7) << "DIPROP_AUTOCENTER\n"; break;
	case 10: DINPUT_LEVEL(7) << "DIPROP_CALIBRATIONMODE\n"; break;
	
	default: assert(!"unrecognized property");
	}

	//DIPROP_AXISMODE may not be supported
	DINPUT_REPORT(proxy->SetProperty(x,y))
}
HRESULT DINPUT::IDirectInputDeviceA::Acquire()
{
	DINPUT_LEVEL(5) << "IDirectInputDeviceA::Acquire()\n";

	DINPUT_LEVEL(5) << product << '\n';

	if(instance==GUID_SysKeyboard) 
	{
		DINPUT::Keyboard = this;
	}
	else if(instance!=GUID_SysMouse) 
	{
		DINPUT::Joystick = this;
	}

	DINPUT_REPORT(proxy->Acquire())
}
HRESULT DINPUT::IDirectInputDeviceA::Unacquire()
{
	DINPUT_LEVEL(7) << "IDirectInputDeviceA::Unacquire()\n";

	DINPUT_RETURN(proxy->Unacquire())
}

DX::DIDEVICEOBJECTDATA *DINPUT::IDirectInputDeviceA::data(size_t perunitsz)
{
	//2012: why GetDeviceData?? Performance?

	if(buffersize==0) return 0;

	static DWORD databufsz = 0;

	static DX::DIDEVICEOBJECTDATA *data = 0;

	if(buffersize*perunitsz>databufsz)
	{
		delete[] data; 
		
		databufsz = buffersize*perunitsz;

		data = (DX::DIDEVICEOBJECTDATA*) new BYTE[databufsz];
	}				

	bufferdata = databufsz/perunitsz;
	
	HRESULT ok = proxy->GetDeviceData(perunitsz,data,&bufferdata,0);

	if(ok==DI_BUFFEROVERFLOW)
	{					
		::DIPROPDWORD dpwbs = 
		{
			{sizeof(::DIPROPDWORD),sizeof(::DIPROPHEADER),0,DIPH_DEVICE},buffersize*2
		};

		if((ok=proxy->Unacquire())==DI_OK)
		if(proxy7->SetProperty(DIPROP_BUFFERSIZE,&dpwbs.diph)==DI_OK)
		if(proxy7->GetProperty(DIPROP_BUFFERSIZE,&dpwbs.diph)==DI_OK)
		{
			buffersize = dpwbs.dwData;
		}

		proxy->Acquire();
	}

	if(ok!=DI_OK&&ok!=DI_BUFFEROVERFLOW) return 0; 
			
	if(!rawdata) return 0;

	if(0&&format)
	//Note: keyboard ghosting etc can cause the 
	//buffer data to become out of sync with the 
	//actual state. You could in theory try to 
	//correct for ghosting by comparing with the 
	//actual state--however that's probably no
	//better than grabbing the state at present
	//2012: SetEventNotification might help here
	//And whenever overflow favor GetDeviceState 
	for(size_t i=0;i<bufferdata;i++)  
	{
		//stupid: data does not specify size
		for(size_t j=0;j<format->dwNumObjs;j++) 
		if(format->rgodf[j].dwOfs==data[i].dwOfs)
		{
			switch(format->rgodf[j].dwType)
			{
			case DIDFT_BUTTON:
			case DIDFT_PSHBUTTON:
			case DIDFT_TGLBUTTON:
				
				rawdata[data[i].dwOfs] = data[i].dwData; break;

			default: //assuming sizeof(DWORD)

				*(DWORD*)(rawdata+data[i].dwOfs) = data[i].dwData;
			}
		}
	}
	else if(proxy->GetDeviceState(rawdatasize,rawdata)!=DI_OK) 
	{
		return 0;
	}

	return data;
}

HRESULT DINPUT::IDirectInputDeviceA::GetDeviceState(DWORD x, LPVOID y)
{
	DINPUT_LEVEL(5) << "IDirectInputDeviceA::GetDeviceState()\n";

	DINPUT_LEVEL(5) << product << '\n';
		
	DINPUT_PUSH_HACK(DIRECTINPUTDEVICEA_GETDEVICESTATE,IDirectInputDeviceA*,
	DWORD&,LPVOID&)(0,this,x,y);

	HRESULT out = !DI_OK; 

	if(!y||!x) DINPUT_FINISH(!DI_OK); //short circuited?

	//keyboard only for now
	if(instance==GUID_SysKeyboard) 
	if(DINPUT::doData&&buffersize==0&&rawdata)
	{
		assert(0); //UNUSED

		DIPROPDWORD dpwbs = 
		{{sizeof(DIPROPDWORD),sizeof(DIPROPHEADER),0,DIPH_DEVICE},16};

		HRESULT ok;

		if((ok=proxy->Unacquire())==DI_OK)
		if((ok=proxy7->SetProperty(DIPROP_BUFFERSIZE,&dpwbs.diph))==DI_OK)
		if((ok=proxy7->GetProperty(DIPROP_BUFFERSIZE,&dpwbs.diph))==DI_OK)
		{
			buffersize = dpwbs.dwData;
		}

		proxy->Acquire();
	}

	//REMINDER: GUID_Joystick (and GUID_SysMouse) don't have
	//context switching logic... ideally they should be routed
	//through the keyboard output and ignored, but that requires
	//multi-context configuration
	if(instance==GUID_SysKeyboard) 
	{					
		if(!y||x>256) //paranoia
		{
			DINPUT_FINISH(!DI_OK); //TODO: a serious warning
		}

		static unsigned char state[256]; //untranslated

		unsigned char *ystate = (unsigned char*)y; //casting

		//initial simulation state
		memset(y,0x00,x*sizeof(char));
		
		if(!EX::Keypad.simulate(ystate,x,state))
		{	
			if(rawdata)	memcpy(state,rawdata,256*sizeof(char)); 			

			DX::DIDEVICEOBJECTDATA *buffer = data();

			if(buffer)
			{
				for(size_t i=0;i<bufferdata;i++)
				if(buffer[i].dwData&0x80&&buffer[i].dwOfs<x)
				{
					state[buffer[i].dwOfs] = 0x80; //conservative sampling
				}

				out = DI_OK;
			}
			else if(1) //todo: timeout
			{
				while(proxy->GetDeviceState(x,state)!=DI_OK)
				{
					HRESULT ok = proxy->Acquire();

					if(ok!=DI_OK&&ok!=DIERR_INPUTLOST&&ok!=E_ACCESSDENIED) 
					break;					
				}
			}
			else out = proxy->GetDeviceState(x,state);			

			if(out!=DI_OK) DINPUT_FINISH(out)	

			if(EX::vk_pause_was_pressed) state[DIK_PAUSE]|=0x80;

			EX::vk_pause_was_pressed = false;

			memcpy(y,state,x*sizeof(char));

			for(size_t i=0;i<x;i++) if(state[i]&0x80)
			{
				bool ok = EX::validating_direct_input_key(i,(char*)state);

				ystate[i] = ok?0x80:0;
			}
			else ystate[i] = 0;			
		}		
		 		
		for(size_t i=0;i<EX_JOYPADS+EX_MICE;i++)
		{
			EX::Joypads[i].simulate(ystate,x);
		}		

		//TODO: this should be in Ex.input.cpp 
		EX::INI::Keymap km;	EX::INI::Keypad kp; 
			
		//should be handled by keypad	  
		static unsigned char status[256];
		static unsigned char repeat[256];
		static unsigned char xstate[256];

		km->translate(ystate,xstate,x);				

		memset(y,0x00,x*sizeof(char)); //ystate

		EX::Syspad.simulate(xstate,x);

		//destructive translation
		{
			//2020: peek at xstate
			EX::translating_direct_input_key(0,(char*)xstate); 
		}
		for(size_t i=0;i<x;i++) if(xstate[i]) //&0x80
		{		
			int xlat = EX::translating_direct_input_key(i,(char*)xstate); 

			ystate[xlat] = xstate[i];			
		}

		static int known = 0; 
		if(known) //synchronization signal
		EX::broadcasting_direct_input_key(0,0);			
		if(known++) for(size_t i=0;i<x;i++)		
		{		
			unsigned char unx = state[i]?i:0;

			if(ystate[i]&0x80&&!(status[i]&0x80)) 
			{			
				EX::broadcasting_direct_input_key(i,unx);
				
				repeat[i] = 0; //turbo repeat 
			}
			else if(ystate[i]&0x80&&kp->turbo_keys[i])
			{	 			
				if(++repeat[i]==kp->turbo_keys[i])
				{
					EX::broadcasting_direct_input_key(i,unx);

					repeat[i] = 0; //turbo repeat
				}
				else EX::broadcasting_direct_input_key(0,unx);
			}
			else if(state[i]) //untranslated
			{
				EX::broadcasting_direct_input_key(0,unx);
			}			
		}

		ystate[0] = 0; //2021: garbage is getting written into 0

		memcpy(status,ystate,x);

		DINPUT_FINISH(DI_OK)
	}

	assert(proxy);

	if(1) //todo: timeout
	{
		while((out=proxy->GetDeviceState(x,y))!=DI_OK)
		{
			HRESULT ok = proxy->Acquire();

			if(ok!=DI_OK&&ok!=DIERR_INPUTLOST&&ok!=E_ACCESSDENIED) break;					
		}
	}
	else out = proxy->GetDeviceState(x,y); 			

	if(y&&x==sizeof(DX::DIJOYSTATE)) 
	{			
		static DX::DIJOYSTATE cmp; //keep display alive

		if(memcmp(y,&cmp,sizeof(cmp)))
		{
			SetThreadExecutionState(ES_DISPLAY_REQUIRED);		

			memcpy(&cmp,y,sizeof(cmp));
		}
	}
		
	DINPUT_POP_HACK(DIRECTINPUTDEVICEA_GETDEVICESTATE,IDirectInputDeviceA*,
	DWORD&,LPVOID&)(&out,this,x,y);

	//Reminder: comes after DINPUT_POP_HACK closure
	if(instance==GUID_SysKeyboard) DINPUT::noPolls++; 

	//disrupts hot plugging (unplugging)
	//HRESULT_FROM_WIN32(ERROR_READ_FAULT) : The system cannot read from the specified device. 
	if(out==0x8007001e) return out;
	//2020: seeing this (HRESULT_FROM_WIN32(INVALID_ACCESS)) today while unplugging ... maybe
	//there should be a system in place to react to the controller being unplugged?
	if(out==0x8007000c) return out;

	DINPUT_RETURN(out)
}
HRESULT DINPUT::IDirectInputDeviceA::GetDeviceData(DWORD x, DX::LPDIDEVICEOBJECTDATA y, LPDWORD z, DWORD w)
{
	DINPUT_LEVEL(5) << "IDirectInputDeviceA::GetDeviceData()\n";

	DINPUT_RETURN(proxy->GetDeviceData(x,y,z,w))
}
HRESULT DINPUT::IDirectInputDeviceA::SetDataFormat(DX::LPCDIDATAFORMAT x)
{
	DINPUT_LEVEL(5) << "IDirectInputDeviceA::SetDataFormat()\n";
												  
	DINPUT_LEVEL(5) << product << '\n';
	
	format = x; 
	
	if(DINPUT::doData&&x&&x->dwDataSize)
	{
		rawdata = new unsigned char[x->dwDataSize]; 

		memset(rawdata,0x00,sizeof(char)*x->dwDataSize);

		rawdatasize = x->dwDataSize;
	}

	DINPUT_RETURN(proxy->SetDataFormat(x))
}
HRESULT DINPUT::IDirectInputDeviceA::SetEventNotification(HANDLE x)
{
	DINPUT_LEVEL(7) << "IDirectInputDeviceA::SetEventNotification()\n";

	DINPUT_RETURN(proxy->SetEventNotification(x))
}
HRESULT DINPUT::IDirectInputDeviceA::SetCooperativeLevel(HWND x, DWORD y)
{
	DINPUT_LEVEL(7) << "IDirectInputDeviceA::SetCooperativeLevel()\n";

	DINPUT_LEVEL(7) << product << '\n';
	
	y = DISCL_NONEXCLUSIVE|DISCL_BACKGROUND; //FOREGROUND; 

	/*2021: no :(
	//trying to avoid drop out when attaching debugger
	//if(EX::window) x = EX::window;
	//if(!x) x = DDRAW::window; assert(x); //helpful?
	if(!x||x==DDRAW::window)
	x = GetAncestor(DDRAW::window,GA_ROOT); assert(x); //helpful?*/
	//2021: getting INVALID_HANLDE with child window??? maybe invisible?
	x = GetAncestor(DDRAW::window,GA_ROOT); 

	HRESULT out = proxy->SetCooperativeLevel(x,y); 

	DINPUT_RETURN(out)
}
HRESULT DINPUT::IDirectInputDeviceA::GetObjectInfo(DX::LPDIDEVICEOBJECTINSTANCEA x, DWORD y, DWORD z)
{
	DINPUT_LEVEL(7) << "IDirectInputDeviceA::GetObjectInfo()\n";

	DINPUT_RETURN(proxy->GetObjectInfo(x,y,z))
}
HRESULT DINPUT::IDirectInputDeviceA::GetDeviceInfo(DX::LPDIDEVICEINSTANCEA x)
{
	DINPUT_LEVEL(7) << "IDirectInputDeviceA::GetDeviceInfo()\n";

	DINPUT_RETURN(proxy->GetDeviceInfo(x))
}
HRESULT DINPUT::IDirectInputDeviceA::RunControlPanel(HWND x, DWORD y)
{
	DINPUT_LEVEL(7) << "IDirectInputDeviceA::RunControlPanel()\n";

	DINPUT_RETURN(proxy->RunControlPanel(x,y))
}
HRESULT DINPUT::IDirectInputDeviceA::Initialize(HINSTANCE x, DWORD y, REFGUID z)
{
	DINPUT_LEVEL(7) << "IDirectInputDeviceA::Initialize()\n";

	static OLECHAR g[64]; StringFromGUID2(z,g,64); DINPUT_LEVEL(7) << ' ' << g << '\n';

	DINPUT_RETURN(proxy->Initialize(x,y,z))
}
HRESULT DINPUT::IDirectInputDeviceA::GetObjectInfo(DX::LPDIDEVICEOBJECTINSTANCEW x, DWORD y, DWORD z)
{
	DINPUT_LEVEL(7) << "IDirectInputDeviceA::GetObjectInfo(W)\n";

	if(dx_dinput_widen(this)) DINPUT_RETURN(proxyW->GetObjectInfo((LPDIDEVICEOBJECTINSTANCEW)x,y,z))

	DINPUT_RETURN(!DI_OK)
}







//2020: all? or most of the interfaces are using DINPUT_RETURN so doing so here
//prevents them from returning legit error/success codes through to the callers
#define DX_DINPUT_RETURN_THRU_A(f)\
	DINPUT_LEVEL(8) << "IDirectInputDevice2A::"#f"() ---> ";\
	/*DINPUT_RETURN(asA->proxy==proxy?asA->f:proxy->f)*/\
	if(asA->proxy==proxy) return asA->f; DINPUT_RETURN(proxy->f)
#define DX_DINPUT_REPORT_THRU_A(f)\
	DINPUT_LEVEL(8) << "IDirectInputDevice2A::"#f"() ---> ";\
	DINPUT_REPORT(asA->proxy==proxy?asA->f:proxy->f)

HRESULT DINPUT::IDirectInputDevice2A::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	DX_DINPUT_RETURN_THRU_A(QueryInterface(riid,ppvObj))
}
ULONG DINPUT::IDirectInputDevice2A::AddRef()
{
	DINPUT_LEVEL(7) << "IDirectInputDevice2A::AddRef()\n";

	return proxy->AddRef();
}
ULONG DINPUT::IDirectInputDevice2A::Release()
{
	DINPUT_LEVEL(7) << "IDirectInputDevice2A::Release()\n";

	DINPUT_LEVEL(7) << asA->product << '\n';

	ULONG out = proxy->Release(); 

	if(out==0)
	{
		proxy = 0; if(!asA->proxy) delete asA; 
	}

	return out;
}
HRESULT DINPUT::IDirectInputDevice2A::GetCapabilities(DX::LPDIDEVCAPS x)
{
	DX_DINPUT_RETURN_THRU_A(GetCapabilities(x))
}
HRESULT DINPUT::IDirectInputDevice2A::EnumObjects(DX::LPDIENUMDEVICEOBJECTSCALLBACKA x, LPVOID y, DWORD z)
{
	DX_DINPUT_RETURN_THRU_A(EnumObjects(x,y,z))
}
HRESULT DINPUT::IDirectInputDevice2A::GetProperty(REFGUID x, DX::LPDIPROPHEADER y)
{
	DX_DINPUT_RETURN_THRU_A(GetProperty(x,y))
}
HRESULT DINPUT::IDirectInputDevice2A::SetProperty(REFGUID x, DX::LPCDIPROPHEADER y)
{
	DX_DINPUT_REPORT_THRU_A(SetProperty(x,y))
}
HRESULT DINPUT::IDirectInputDevice2A::Acquire()
{
	DX_DINPUT_REPORT_THRU_A(Acquire())
}
HRESULT DINPUT::IDirectInputDevice2A::Unacquire()
{
	DX_DINPUT_REPORT_THRU_A(Unacquire())
}
HRESULT DINPUT::IDirectInputDevice2A::GetDeviceState(DWORD x, LPVOID y)
{
	DX_DINPUT_RETURN_THRU_A(GetDeviceState(x,y))
}
HRESULT DINPUT::IDirectInputDevice2A::GetDeviceData(DWORD x, DX::LPDIDEVICEOBJECTDATA y, LPDWORD z, DWORD w)
{
	DX_DINPUT_RETURN_THRU_A(GetDeviceData(x,y,z,w))
}
HRESULT DINPUT::IDirectInputDevice2A::SetDataFormat(DX::LPCDIDATAFORMAT x)
{
	DX_DINPUT_RETURN_THRU_A(SetDataFormat(x))
}
HRESULT DINPUT::IDirectInputDevice2A::SetEventNotification(HANDLE x)
{
	DX_DINPUT_RETURN_THRU_A(SetEventNotification(x))
}
HRESULT DINPUT::IDirectInputDevice2A::SetCooperativeLevel(HWND x, DWORD y)
{
	DX_DINPUT_RETURN_THRU_A(SetCooperativeLevel(x,y))
}
HRESULT DINPUT::IDirectInputDevice2A::GetObjectInfo(DX::LPDIDEVICEOBJECTINSTANCEA x, DWORD y, DWORD z)
{
	DX_DINPUT_RETURN_THRU_A(GetObjectInfo(x,y,z))
}
HRESULT DINPUT::IDirectInputDevice2A::GetDeviceInfo(DX::LPDIDEVICEINSTANCEA x)
{
	DX_DINPUT_RETURN_THRU_A(GetDeviceInfo(x))
}
HRESULT DINPUT::IDirectInputDevice2A::RunControlPanel(HWND x, DWORD y)
{
	DX_DINPUT_RETURN_THRU_A(RunControlPanel(x,y))
}
HRESULT DINPUT::IDirectInputDevice2A::Initialize(HINSTANCE x, DWORD y, REFGUID z)
{
	DX_DINPUT_RETURN_THRU_A(Initialize(x,y,z))
}
HRESULT DINPUT::IDirectInputDevice2A::CreateEffect(REFGUID x, DX::LPCDIEFFECT y, DX::LPDIRECTINPUTEFFECT *z, LPUNKNOWN w)
{
	DINPUT_LEVEL(7) << "IDirectInputDevice2A::CreateEffect()\n";

	static OLECHAR g[64]; StringFromGUID2(x,g,64); DINPUT_LEVEL(7) << ' ' << g << '\n';

	DINPUT_RETURN(proxy->CreateEffect(x,y,z,w))
}
HRESULT DINPUT::IDirectInputDevice2A::EnumEffects(DX::LPDIENUMEFFECTSCALLBACKA x, LPVOID y, DWORD z)
{
	DINPUT_LEVEL(7) << "IDirectInputDevice2A::EnumEffects()\n";

	DINPUT_RETURN(proxy->EnumEffects(x,y,z))
}
HRESULT DINPUT::IDirectInputDevice2A::GetEffectInfo(DX::LPDIEFFECTINFOA x, REFGUID y)
{
	DINPUT_LEVEL(7) << "IDirectInputDevice2A::GetEffectInfo()\n";

	static OLECHAR g[64]; StringFromGUID2(y,g,64); DINPUT_LEVEL(7) << ' ' << g << '\n';

	DINPUT_RETURN(proxy->GetEffectInfo(x,y))
}
HRESULT DINPUT::IDirectInputDevice2A::GetForceFeedbackState(LPDWORD x)
{
	DINPUT_LEVEL(7) << "IDirectInputDevice2A::GetForceFeedbackState()\n";

	DINPUT_RETURN(proxy->GetForceFeedbackState(x))
}
HRESULT DINPUT::IDirectInputDevice2A::SendForceFeedbackCommand(DWORD x)
{
	DINPUT_LEVEL(7) << "IDirectInputDevice2A::SendForceFeedbackCommand()\n";

	DINPUT_RETURN(proxy->SendForceFeedbackCommand(x))
}
HRESULT DINPUT::IDirectInputDevice2A::EnumCreatedEffectObjects(DX::LPDIENUMCREATEDEFFECTOBJECTSCALLBACK x, LPVOID y, DWORD z)
{
	DINPUT_LEVEL(7) << "IDirectInputDevice2A::EnumCreatedEffectObjects()\n";

	DINPUT_RETURN(proxy->EnumCreatedEffectObjects(x,y,z))
}
HRESULT DINPUT::IDirectInputDevice2A::Escape(DX::LPDIEFFESCAPE x)
{
	DINPUT_LEVEL(7) << "IDirectInputDevice2A::Escape()\n";

	DINPUT_RETURN(proxy->Escape(x))
}
HRESULT DINPUT::IDirectInputDevice2A::Poll()
{
	DINPUT_LEVEL(7) << "IDirectInputDevice2A::Poll()\n";

	DINPUT_REPORT(proxy->Poll())
}
HRESULT DINPUT::IDirectInputDevice2A::SendDeviceData(DWORD x, DX::LPCDIDEVICEOBJECTDATA y, LPDWORD z, DWORD w)
{
	DINPUT_LEVEL(7) << "IDirectInputDevice2A::SendDeviceData()\n";

	DINPUT_RETURN(proxy->SendDeviceData(x,y,z,w))
}







#define DINPUT_TABLES(Interface)\
void *DINPUT::Interface::vtables[DINPUT::MAX_TABLES] = {0,0,0};\
void *DINPUT::Interface::dtables[DINPUT::MAX_TABLES] = {0,0,0};
	
DINPUT_TABLES(IDirectInputA)
DINPUT_TABLES(IDirectInputDeviceA)
DINPUT_TABLES(IDirectInputDevice2A)

#undef DINPUT_TABLES

//REMOVE ME?
extern int DINPUT::is_needed_to_initialize()
{
	static int one_off = 0; if(one_off++) return one_off; //???

	DX::is_needed_to_initialize(); 
	
#define DINPUT_TABLES(Interface) DINPUT::Interface().register_tables('dx7');

	DINPUT_TABLES(IDirectInputA)
	DINPUT_TABLES(IDirectInputDeviceA)
	DINPUT_TABLES(IDirectInputDevice2A)

#undef DINPUT_TABLES

	DINPUT_HELLO(0) << "DirectInput initialized\n";

	return 1; //for static thunks //???
}						 

namespace DINPUT
{		
	static HRESULT (WINAPI *DirectInputCreateA)(HINSTANCE,DWORD,LPDIRECTINPUTA*,LPUNKNOWN) = 0;
}
static HRESULT WINAPI dx_dinput_DirectInputCreateA(HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUTA *ppDI, LPUNKNOWN punkOuter)
{	
	//NEW: require EXE is source
	if(hinst&&hinst!=(HINSTANCE)0x00400000)
	return DINPUT::DirectInputCreateA(hinst,dwVersion,ppDI,punkOuter); 

	DINPUT::is_needed_to_initialize();

	DINPUT_LEVEL(7) << "dx_dinput_DirectInputCreateA()\n";

	DINPUT_LEVEL(7) << "Version = " << std::hex << dwVersion << '\n';

	dwVersion = 0x700; //upgrading version as far as possible

	LPDIRECTINPUTA q = 0; 
	
	if(DINPUT::DirectInputA) //NEW
	{
		DINPUT_LEVEL(7) << " Proxy exists. Adding reference...\n"; 	
		DINPUT::DirectInputA->AddRef(); *ppDI = (LPDIRECTINPUTA)DINPUT::DirectInputA; 
		return DI_OK;
	}

	HRESULT out = DINPUT::DirectInputCreateA(hinst,dwVersion,&q,punkOuter); 

	if(out!=DI_OK)
	{ 
		DINPUT_LEVEL(7) << "DirectInputCreateA() Failed\n"; 		
		DINPUT_RETURN(out)
	}

	if(DINPUT::doIDirectInputA)
	{
		DINPUT::IDirectInputA *p = new DINPUT::IDirectInputA('dx7');

		p->proxy7 = q; *ppDI = (LPDIRECTINPUTA)p;

		DINPUT_LEVEL(7) << ' ' << p->proxy << '\n';

		if(out==DI_OK) DINPUT::DirectInputA = p;
	}
	else *ppDI = (LPDIRECTINPUTA)q;

	DINPUT_RETURN(out)
}	 
static void dx_dinput_detours(LONG (WINAPI *f)(PVOID*,PVOID))
{
		//REQUIRED
		//this was not necessary prior to adding the DirectShow (wavdest)
		//static libraries to the mix. it seemed to fail even then, until
		//I noticed the wavdest project was using __stdcall by way of its
		//MSVC project file. could be just a coincidence
		HMODULE dll = GetModuleHandleA("dinput.dll");
		if(!dll) return; //not a game
		//#define _(x) x
		#define _(x) GetProcAddress(dll,#x)
	if(!DINPUT::DirectInputCreateA)
	(void*&)DINPUT::DirectInputCreateA = _(DirectInputCreateA);	
		#undef _
	f(&(PVOID&)DINPUT::DirectInputCreateA,dx_dinput_DirectInputCreateA);	

}//register dx_dinput_detours
static int dx_dinput_detouring = DX::detouring(dx_dinput_detours);

