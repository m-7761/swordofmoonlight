	   
#include "Somplayer.pch.h"

#include "Somregis.h"
#include "Somthread.h" 
#include "Somcontrol.h"

namespace Somcontrol_cpp
{
	struct postman
	{
		HWND window; UINT message;
		
		WPARAM wparam; LPARAM lparam;		
	};

	struct internals
	{				
		//2 pointers 1 allocation
		float *buttons, *actions; 

		~internals(){ delete [] buttons; }

		internals(){ buttons = actions = 0; }		
				
		std::vector<postman> postmen;
		Somthread_h::account poboxes;
	};

	//scheduled obsolete
	struct DIDEVICEOBJECTINSTANCE_etc 
	:public DIDEVICEOBJECTINSTANCEW
	{
		DIDEVICEOBJECTINSTANCE_etc
		(const DIDEVICEOBJECTINSTANCEW &cp)
		{
			//stack overflow???
			memcpy(this,&cp,sizeof(cp)); //*this = cp;
			
			lMid = lMin = 0; lMax = 0x80; //pro forma
		} 

		void diproprange(IDirectInputDevice8W *p)
		{				
			static DIPROPRANGE dipr = 
			{
				{sizeof(DIPROPRANGE),sizeof(DIPROPHEADER),0,DIPH_BYID},0,0
			};

			if(DIDFT_GETTYPE(dwType)!=DIDFT_RELAXIS) 
			{
				dipr.diph.dwObj = dwType; 

				HRESULT hr = p->GetProperty(DIPROP_RANGE,&dipr.diph);

				lMin = hr||dipr.lMin==DIPROPRANGE_NOMIN?0:dipr.lMin;
				lMax = hr||dipr.lMin==DIPROPRANGE_NOMAX?65535:dipr.lMax;				
			}
			else //assuming mouse or mouse like
			{
				lMin = -540; lMax = 540; //1080 (may not hold universally)
			}

			lMid = lMin+(lMax-lMin)/2;
		}
				
		LONG lMid, lMin, lMax; //DIPROPRANGE

		float operator*(LONG bt)
		{
			if(bt==lMid) return 0; 
			if(bt>=lMax) return 1; if(bt<=lMin) return -1.0;
			
			return float(bt-lMin)/(lMax-lMin)*2-1;
		}
	};

	struct di8device : public internals
	{	
		DIDEVICEINSTANCEW dideviceinstance;

		DIDEVCAPS devcaps; //IDirectInputDevice8W *device; 
				
		//dataformat.rgodf: 3 pointers 1 allocation
		DIDATAFORMAT dataformat; BYTE *databytes; DWORD *datawords; 

		//WARNING: these are shuffled around a little
		std::vector<DIDEVICEOBJECTINSTANCE_etc> buttons; 
		std::vector<DIDEVICEOBJECTINSTANCE_etc> actions;

		di8device()
		{
			dataformat.rgodf = 0; *guid = 0; //device = 0;
		}
				
		~di8device()
		{
			//if(device&&Somplayer_dll()) device->Release();		

			delete dataformat.rgodf;
		}

		static const size_t guid_s = 40;

		wchar_t guid[guid_s];
	};

	//not being used; probably obsolete
	struct wmmdevice : public internals
	{	
		JOYCAPSW joycaps; //! mNum/Max are repurposed
	};	

	//friend
	static struct multitap
	{	
		size_t size; multitap(){ size = 0; }

		Somcontrol *devices[SOMCONTROL_DEVICES];
		
		Somthread_h::account refs[SOMCONTROL_DEVICES];

		inline Somcontrol* &operator[](int i){ return devices[i]; }
		
		~multitap(){ for(size_t i=0;i<size;i++) delete devices[i]; }
		
		Somcontrol *operator++() //TODO: triage??
		{			
			if(size==SOMCONTROL_DEVICES) return 0; 
						
			refs[size] = 0; Somcontrol *out = new Somcontrol(size); 
			
			return devices[size++] = out; 
		}		

	}multitap;

	#undef thread //???

	//friend
	static struct thread
	{
		HANDLE handle; DWORD id; 

		static DWORD WINAPI main(LPVOID); 

		operator HANDLE(){ return handle; }

		void operator=(HANDLE h)
		{
			HANDLE quit = handle; handle = 0;
			while(GetExitCodeThread(quit,&id)==STILL_ACTIVE) Sleep(1);
			CloseHandle(quit); handle = h;
		}		

		thread(){ memset(this,0x00,sizeof(thread)); }
		~thread(){ handle = 0; }

		void freeze_and_thaw_mouse(int ms);

		private: IDirectInputDevice8W *di8(size_t);

		IDirectInput8W *directinput8;

		void post(size_t);
		void freeze(size_t);
		void thaw(size_t,IDirectInputDevice8W*);		

		unsigned char *scancodemap(); 

		int pause, mice; //mouse+ice

	}thread;
}

//placeholders
Somcontrol_cpp::wmmdevice *Somcontrol::wmm = 0;
Somcontrol_cpp::ds8device *Somcontrol::ds8 = 0;

Somcontrol::Somcontrol(int i) : tap(i)
{
	port = time = post = -1; //user variables 

	portholder = 0; api = _; di8 = 0; wmm = 0; 
										  	
	memset(contexts,0x00,sizeof(contexts)); 

	some_internal_states = 0;
}

const Somcontrol Somcontrol::unplugged(SOMCONTROL_DEVICES);

Somcontrol::~Somcontrol()
{
	if(api) api = _; else return;

	if(tap<0||tap>=Somcontrol_cpp::multitap.size) return;
	
	Somthread_h::section cs;

	if(Somcontrol_cpp::multitap[tap]!=this)
	{
		if(--Somcontrol_cpp::multitap.refs[tap]<0) assert(0);
	}
	else Somcontrol_cpp::multitap[tap] = 0;			 

	if(!Somcontrol_cpp::multitap[tap])
	if(!Somcontrol_cpp::multitap.refs[tap])
	{
		delete di8; delete wmm;			
	}
}

void Somcontrol::keepalive()const
{
	if(!this||!api) return;
	
	if(tap<0||tap>=Somcontrol_cpp::multitap.size) return;

	if(++Somcontrol_cpp::multitap.refs[tap]!=1) return;
	
	if(!Somcontrol_cpp::thread)
	{
		Somthread_h::section cs;

		if(!Somcontrol_cpp::thread)
		{
			Somcontrol_cpp::thread = 
			CreateThread(0,0,Somcontrol_cpp::thread::main,0,0,0);
		}
	}
}

static BOOL CALLBACK Somcontrol_di8_buttons_cb(LPCDIDEVICEOBJECTINSTANCEW lpddoi, LPVOID di8)
{
	((Somcontrol_cpp::di8device*)di8)->buttons.push_back(*lpddoi); return TRUE;
}

static BOOL CALLBACK Somcontrol_di8_actions_cb(LPCDIDEVICEOBJECTINSTANCEW lpddoi, LPVOID di8)
{
	using namespace Somcontrol_cpp;

	std::vector<DIDEVICEOBJECTINSTANCE_etc> &v = ((di8device*)di8)->actions;

	v.push_back(*lpddoi); if(lpddoi->guidType==GUID_POV) v.push_back(*lpddoi); else return TRUE;

	//We are going to treat a pov-hat as a normalized pair of axes
	//To see if an action is in fact a pov hat do dwType&DIDFT_POV

	DIDEVICEOBJECTINSTANCEW &x = v[v.size()-2], &y = v[v.size()-1];

	wcscat_s(x.tszName,L" (X-axis)"); wcscat_s(y.tszName,L" (Y-axis)");

	x.guidType = GUID_XAxis; y.guidType = GUID_YAxis; return TRUE;
}	  

static size_t Somcontrol_sysmouse = -2; 
static size_t Somcontrol_syskeyboard = -2; //hack

//static
BOOL CALLBACK Somcontrol::di8_callback(LPCDIDEVICEINSTANCEW lpddi, LPVOID di)
{	
	IDirectInput8W *direct = (IDirectInput8W*)di;

	for(size_t i=0;i<Somcontrol_cpp::multitap.size;i++)
	{
		Somcontrol *p = Somcontrol_cpp::multitap[i];

		if(p&&p->di8&&p->di8->dideviceinstance.guidInstance==lpddi->guidInstance)
		{
			return TRUE; //passing: assuming previously discovered
		}
	}

	IDirectInputDevice8W *q = 0; DIDEVCAPS caps = {sizeof(DIDEVCAPS)};

	if(direct->CreateDevice(lpddi->guidInstance,&q,0)==DI_OK)
	if(q->GetCapabilities(&caps)!=DI_OK||!caps.dwButtons&&!caps.dwAxes&&!caps.dwPOVs)
	{
		q->Release(); return TRUE; //passing: something other than a controller??
	}
	//else q->Release(); //...

	Somcontrol *p = q?Somcontrol_cpp::multitap++:0; if(!p) return TRUE;

	p->api = Somcontrol::DI8; p->cpp = p->di8 = new Somcontrol_cpp::di8device;

	p->di8->dideviceinstance = *lpddi; p->di8->devcaps = caps;
	   
	//// It might be desirable to delay this step until it is required ////

	p->di8->buttons.reserve(p->di8->devcaps.dwButtons+1);
	p->di8->actions.reserve(p->di8->devcaps.dwAxes+p->di8->devcaps.dwPOVs*2);

	q->EnumObjects(Somcontrol_di8_buttons_cb,p->di8,DIDFT_BUTTON); 
	q->EnumObjects(Somcontrol_di8_actions_cb,p->di8,DIDFT_AXIS|DIDFT_POV);	 		
		 		
	if(p->di8->buttons.size()%2) //button_count says must be even
	{
		DIDEVICEOBJECTINSTANCEW even; even.dwType = 0; *even.tszName = '\0';

		p->di8->buttons.push_back(even);
	}
				
	for(size_t i=0,n=p->di8->actions.size();i<n;i++)
	{
		p->di8->actions[i].diproprange(q);
	}

	q->Release();

	///////////////////////////////////////////////////////////////////////
	
	size_t bts = p->di8->buttons.size(), ats = p->di8->actions.size();

	p->cpp->actions = (p->cpp->buttons=new float[bts+ats])+bts;

	memset(p->cpp->buttons,0x00,sizeof(float)*(bts+ats));

	if(lpddi->guidInstance==GUID_SysMouse)
	{
		Somcontrol_sysmouse = Somcontrol_cpp::multitap.size-1; //hack

		wcscpy_s(p->di8->dideviceinstance.tszInstanceName,L"System Mouse");
		p->di8->dideviceinstance.tszProductName[0] = '\0';
	}
	else if(lpddi->guidInstance==GUID_SysKeyboard)
	{
		Somcontrol_syskeyboard = Somcontrol_cpp::multitap.size-1; //hack

		wcscpy_s(p->di8->dideviceinstance.tszInstanceName,L"System Keyboard");
		p->di8->dideviceinstance.tszProductName[0] = '\0';
	}

	return TRUE;
}

const Somcontrol *Somcontrol::multitap(int out)
{		
	if(out==DISCOVER)
	{
		Somthread_h::section cs;

		out = Somcontrol_cpp::multitap.size;

		if(out==0) //testing...
		{						 
			IDirectInput8W *di8 = 0;

			DirectInput8Create(Somplayer_dll(),
			DIRECTINPUT_VERSION,IID_IDirectInput8W,(void**)&di8,0);								  
			if(di8)	di8->EnumDevices(0,Somcontrol::di8_callback,di8,0); 
			if(di8)	di8->Release();
						
			JOYCAPSW caps; 
			
			if(&Somcontrol::wmm!=&wmm)
			for(size_t i=0,n=joyGetNumDevs();i<n;i++)
			{													
				if(joyGetDevCapsW(i,&caps,sizeof(JOYCAPSW))==S_OK)
				{	
					Somcontrol *p = Somcontrol_cpp::multitap++; if(!p) break;

					p->api = WMM; p->cpp = p->wmm = new Somcontrol_cpp::wmmdevice;

					//we don't do mode shifts???					
					caps.wNumButtons = caps.wMaxButtons; caps.wNumAxes = caps.wMaxAxes;
		
					if(caps.wCaps&JOYCAPS_HASPOV) caps.wMaxAxes+=2; //pov axes

					//spec say must be even
					if(caps.wNumButtons%2) caps.wMaxButtons++;

					p->wmm->joycaps = caps;

					size_t bts = caps.wMaxButtons, ats = caps.wMaxAxes;

					p->cpp->actions = (p->cpp->buttons=new float[bts+ats])+bts;

					memset(p->cpp->buttons,0x00,sizeof(float)*(bts+ats));
				}	   				
			}
		}
	}

	if(out<0||out>=Somcontrol_cpp::multitap.size) return 0;

	return Somcontrol_cpp::multitap[out];
}

const wchar_t *Somcontrol::persistent_identifier()const
{
	if(this&&api) if(di8)
	{
		if(!*di8->guid)
		StringFromGUID2(di8->dideviceinstance.guidInstance,di8->guid,di8->guid_s);		
		return di8->guid; 
	}
	return L"";	
}

const wchar_t *Somcontrol::device()const
{
	if(this&&api) if(di8)
	{
		return di8->dideviceinstance.tszInstanceName;
	}
	else if(wmm) return wmm->joycaps.szPname;
	
	return L""; 
}

const wchar_t *Somcontrol::device2()const
{
	const wchar_t * out = L"";

	if(this&&api) if(di8)
	{
		out = di8->dideviceinstance.tszProductName;
	}
	else if(wmm) out = wmm->joycaps.szOEMVxD;

	return wcscmp(out,device())?out:L"";
}

namespace Somcontrol_cpp
{
	static struct showjoycpl
	{
		HMODULE lib; LPFNSHOWJOYCPL proc;

		showjoycpl(){ lib = 0; proc = 0; }

		bool operator()(HWND owner)
		{
			if(!lib) lib = LoadLibraryA("joy.cpl"); 

			if(!proc) proc = (LPFNSHOWJOYCPL)GetProcAddress(lib,"ShowJoyCPL");

			if(proc) proc(owner); return proc;
		}

		~showjoycpl(){ FreeLibrary(lib); }

	}showjoycpl;
}

bool Somcontrol::system()const
{
	if(this&&api&&di8)
	{
		//if(di8->dideviceinstance.guidInstance==GUID_SysMouse) return true;
		//if(di8->dideviceinstance.guidInstance==GUID_SysKeyboard) return true;

		return tap==Somcontrol_syskeyboard||tap==Somcontrol_sysmouse;
	}			 
	return false;
}

bool Somcontrol::popup_control_panel(HWND owner)const
{
	bool out = false; 

	if(this&&api) if(di8)
	{	
		IDirectInput8W *direct = 0;

		if(DirectInput8Create(Somplayer_dll(),
		DIRECTINPUT_VERSION,IID_IDirectInput8W,(void**)&direct,0)==DI_OK)
		{
			IDirectInputDevice8W *device = 0;				

			if(direct->CreateDevice(di8->dideviceinstance.guidInstance,&device,0)==DI_OK)
			{
				out = device->RunControlPanel(owner,0)==DI_OK; device->Release();
			}			
			direct->Release();
		}
	}
	else if(wmm) return Somcontrol_cpp::showjoycpl(owner);

	return out;
}

Somspeaker *Somcontrol::speakers()
{
	assert(0); return 0; //unimplemented
}

const wchar_t *Somcontrol::button_label(int n)const
{
	if(this&&api&&n>=0) if(di8)
	{
		if(n>=di8->buttons.size())
		{
			return action_label((n-di8->buttons.size())/2);
		}
		else return di8->buttons[n].tszName;
	}
	else if(wmm)
	{
		if(n>=wmm->joycaps.wMaxButtons)
		{
			return action_label((n-wmm->joycaps.wMaxButtons)/2);
		}
		else switch(n) 
		{
		default: assert(0); //unimplmented
		} 
	}
	return L""; 
}

const wchar_t *Somcontrol::action_label(int n)const
{
	if(this&&api) if(di8)
	{
		if(n<di8->actions.size()) return di8->actions[n].tszName;
	}
	else if(wmm) switch(n) 
	{
	default: assert(0); //unimplmented
	}  
	return L""; 
}

size_t Somcontrol::button_count()const
{
	if(this&&api) if(di8)
	{
		return di8->buttons.size();
	}
	else if(wmm) 
	{
		return wmm->joycaps.wMaxButtons;
	} 
	return 0; 
}

size_t Somcontrol::action_count()const
{
	if(this&&api) if(di8)
	{
		return di8->actions.size();
	}
	else if(wmm) 
	{
		return wmm->joycaps.wMaxAxes;
	}		 
	return 0; 
}

float Somcontrol::button_state(int n)const
{	   
	if(n<0) return 0.0f;

	size_t bts = button_count();

	if(n<bts) return cpp->buttons[n]; 

	n-=bts; float out = n%2?1.0f:-1.0f; 
	
	n/=2; if(n>=action_count()) return 0.0f;

	out*=cpp->actions[n]; if(out<0) return 0.0f;

	return out;
}

float Somcontrol::action_state(int n)const
{
	return n>=0&&n<action_count()?cpp->actions[n]:0.0f;
}

const float *Somcontrol::buttons()const
{
	return this&&api?cpp->buttons:0;
}

const float *Somcontrol::actions()const
{
	return this&&api?cpp->actions:0;
}

int Somcontrol::post_messages_to(HWND hwnd, int msg, void *lparam)const
{		
	Somthread_h::section cs; if(!this||!api||!hwnd) return -1; 

	Somcontrol_cpp::postman nt = { hwnd, msg, (WPARAM)this, (LPARAM)lparam };

	if(cpp->poboxes!=cpp->postmen.size())
	for(size_t i=0,n=cpp->postmen.size();i<n;i++) if(!cpp->postmen[i].window)
	{
		cpp->postmen[i] = nt; cpp->poboxes++; return i;
	}
	
	cpp->postmen.push_back(nt); cpp->poboxes++; 
	
	return cpp->postmen.size()-1;
}

void Somcontrol::stop_messages_to(HWND hwnd, int i)const
{
	Somthread_h::section cs; if(!this||!api||!hwnd) return; 

	if(i<0||i>=cpp->postmen.size()||cpp->postmen[i].window!=hwnd) return;

	cpp->postmen[i].window = 0; cpp->poboxes--; 
}

bool Somcontrol::press(int pb, int status)
{	
	const int P = 1; //power 

	const int switches = POWER;
	const int paranoia = POWER|PAUSE|BELL;	
			
	int &st = some_internal_states;

	switch(status)
	{
	case 0: case 1:
	
		status*=pb;

		//assuming singular
		if((paranoia&pb)==pb)
		{
			if((st&pb)!=status) //different
			{
				st = st&~pb|status; 
				
				if(status) return false; //down
					
				if((pb&switches)==POWER) goto flip; //power 

				return true; //up (released)
			}
		}
		else assert(0);	return false;

	case -1: //predicate mode
		
		switch(pb)
		{
		case POWER:

		flip: st = st&P?st&~P:st|P; return true;

		case POWER|IF_ON: 			
			
			if(st&P) goto flip; else return false;

		case POWER|IF_OFF:
			
			if((st&P)==0) goto flip; else return false;

		default: assert(0);
		}

	default: assert(0);
	}

	return false;
}

static HWND Somcontrol_focus = 0;
static HWND Somcontrol_mouse = 0; 
static HWND Somcontrol_board = 0; 

bool Somcontrol::power()const
{
	if(this&&api
	  &&some_internal_states&1)
	{
		return system()?Somcontrol_focus:true;
	}
	else return false;
}

static RECT Somcontrol_mousetrap = {0,0,0,0};

static LRESULT CALLBACK SomcontrolFocusProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scid, DWORD_PTR) 
{
	RECT &trap = Somcontrol_mousetrap;

	if(hwnd!=Somcontrol_focus) 
	{
		if(Somcontrol_focus) goto remove;
	}	
	
	switch(uMsg)
	{	
	case WM_KILLFOCUS:

		ClipCursor(0); 		
		SetCursor(LoadCursor(0,IDC_ARROW)); 
		
		Somcontrol_focus = 0; break;

	case WM_MOVE:
	case WM_SETFOCUS:
				
		//so the focus click is not translated into a move
		Somcontrol_cpp::thread.freeze_and_thaw_mouse(300);

		Somcontrol_focus = hwnd;

		if(Somcontrol_mouse==hwnd)
		if(GetWindowRect(Somcontrol_focus,&trap)) 
		{
			POINT size = //shrink wrap to about 1 pixel
			{trap.right-trap.left,trap.bottom-trap.top};

			trap.left+=size.x/2-1; trap.right-=size.x/2-1;
			trap.top+=size.y/2-1; trap.bottom-=size.y/2-1;

			SetCursorPos(trap.left,trap.top);

			if(ClipCursor(&trap)) SetCursor(0); 

		}break; 

	case WM_GETDLGCODE:

		if(Somcontrol_board!=hwnd) break;

		return DLGC_WANTALLKEYS|DefSubclassProc(hwnd,uMsg,wParam,lParam);

	case WM_SYSKEYDOWN: case WM_SYSKEYUP: case WM_SYSDEADCHAR:

	case WM_KEYDOWN: case WM_KEYUP: case WM_CHAR: case WM_DEADCHAR: case WM_UNICHAR:

		if(Somcontrol_board==hwnd) return 1; else break;

	case WM_SETCURSOR:
	case WM_LBUTTONUP: case WM_LBUTTONDOWN:
	case WM_RBUTTONUP: case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK: 
				
		if(Somcontrol_mouse==hwnd)  
		{
			if(uMsg==WM_SETCURSOR) SetCursor(0); return 1;									
		}
		else if(uMsg==WM_SETCURSOR) 
		{
			ClipCursor(0); 
			
			DefSubclassProc(hwnd,uMsg,wParam,lParam);

			if(!GetCursor()) SetCursor(LoadCursor(0,IDC_ARROW));

			return 1;
		}
		break;

	case WM_NCDESTROY: 
	
	remove:	RemoveWindowSubclass(hwnd,SomcontrolFocusProc,scid);

		if(Somcontrol_focus==hwnd) Somcontrol_focus = 0; break;
	}

	return DefSubclassProc(hwnd,uMsg,wParam,lParam);
}

void Somcontrol::focus(HWND hwnd)
{
	if(Somcontrol_focus!=hwnd) if(Somcontrol_focus=hwnd)
	{
		SetWindowSubclass(hwnd,SomcontrolFocusProc,0,0);

		PostMessage(hwnd,WM_SETFOCUS,0,0);
	}
}

static int Somcontrol_mouse_capture = 0;
static int Somcontrol_board_capture = 0;

void Somcontrol::capture(int ms)const
{
	if(!Somcontrol_focus) return;

	//todo: make mouse/board OOP
	if(tap==Somcontrol_sysmouse)
	{
		if(!ms) ms = 100;

		if(Somcontrol_mouse_capture<=0)
		{
			Somcontrol_mouse = Somcontrol_focus; 
			PostMessage(Somcontrol_focus,WM_SETFOCUS,0,0);
		}

		if(ms>Somcontrol_mouse_capture)
		{
			Somcontrol_mouse_capture = ms; //timeout
		}
	}
	else if(tap==Somcontrol_syskeyboard)
	{
		if(!ms) ms = 100;

		if(Somcontrol_board_capture<=0)
		{
			Somcontrol_board = Somcontrol_focus;
			PostMessage(Somcontrol_focus,WM_SETFOCUS,0,0);
		}

		if(ms>Somcontrol_board_capture)
		{
			Somcontrol_board_capture = ms; //timeout
		}
	}
}

static HWND Somcontrol_hidden = 0;

static INT_PTR CALLBACK 
SomcontrolHiddenProc(HWND,UINT,WPARAM,LPARAM){ return 0; }

DWORD WINAPI Somcontrol_cpp::thread::main(LPVOID)
{	
  /*WARNING: Sleep causes Acquire to deadlock!!!*/
  //Thankfully MsgWaitForMultipleObjects timeout//
  //parameter is sufficient (the docs suggest 0)//

	using namespace Somcontrol_cpp;
		
	thread &thread = Somcontrol_cpp::thread; //names
		
	thread.id = GetCurrentThreadId();
	SetThreadPriority(thread,THREAD_PRIORITY_TIME_CRITICAL);
		
	HANDLE events[SOMCONTROL_DEVICES];
	
	IDirectInputDevice8W *devices[SOMCONTROL_DEVICES];

	memset(events,0x00,sizeof(events));
	memset(devices,0x00,sizeof(devices));
	
	//MsgWaitForMultipleObjects requires
	//each event to be a non-zero handle
	//There is also MAXIMUM_WAIT_OBJECTS
	HANDLE events_objects[SOMCONTROL_DEVICES];
	size_t events_devices[SOMCONTROL_DEVICES];
	
	DestroyWindow(Somcontrol_hidden); //paranoia

//	Somcontrol_hidden = CreateDialog //testing
//	(Somplayer_dll(),MAKEINTRESOURCE(IDD_HIDDEN),0,SomcontrolHiddenProc);
	
	int pausing = 0; //give the pause key some umph

	DWORD wait = 0, poll_wait = 3; //remember not to Sleep

	bool frozen = false, hard_freeze = true; //in background 

	unsigned char *scancodemap = thread.scancodemap(); //hacks

	DIPROPDWORD dpwbs = 
	{
		{sizeof(DIPROPDWORD),sizeof(DIPROPHEADER),0,DIPH_DEVICE},0
	};

	while(1)
	{ 
		if(!thread)	PostQuitMessage(0);

		//todo: Somcontrol::frozen() or something
		static DWORD pid = GetCurrentProcessId(); 
		
		for(DWORD qid=0;qid!=pid;) //Sleep(10)) 
		{
			HWND fg = GetForegroundWindow();

			if(fg) GetWindowThreadProcessId(fg,&qid);
			
				else continue; //changing focus

			if(qid==pid&&frozen)
			{
				frozen = false;

				for(size_t i=0,n=multitap.size;i<n;i++)
				{
					if(multitap.refs[i]) 
					{
						if(!devices[i]) //hard_freeze 
						{
							devices[i] = thread.di8(i);

							CloseHandle(events[i]); events[i] = 0;
						}

						thread.thaw(i,devices[i]);
					}
				}
			}
			else if(qid!=pid&&!frozen)
			{
				frozen = true; //zero out the buttons

				for(size_t i=0,n=multitap.size;i<n;i++)
				{
					if(multitap.refs[i]) 
					{
						thread.freeze(i);	

						if(hard_freeze)
						{
							if(devices[i]) 
							devices[i]->Release(); devices[i] = 0;
							CloseHandle(events[i]); events[i] = 0;
						}
					}
				}				
			}
			else break;
		}

		DWORD events_s = 0; if(!frozen) //...

		for(size_t i=0;i<multitap.size;i++) if(multitap.refs[i]) 
		{	
			if(!events[i]) 
			{
				events[i] = INVALID_HANDLE_VALUE;				

				Somthread_h::section cs;

				Somcontrol *p = multitap[i]; if(!p) continue;

				if(!devices[i]) devices[i] = thread.di8(i);

				if(devices[i])
				{
					devices[i]->Unacquire(); 

					if(~p->di8->devcaps.dwFlags&DIDC_POLLEDDATAFORMAT)
					{
						events[i] = CreateEvent(0,0,0,0);
						
						devices[i]->SetEventNotification(events[i]);	

						dpwbs.dwData = 16;
						devices[i]->SetProperty(DIPROP_BUFFERSIZE,&dpwbs.diph);
					}
					else assert(0);
					
					devices[i]->SetCooperativeLevel
					(Somcontrol_hidden,DISCL_BACKGROUND|DISCL_NONEXCLUSIVE); 
					
					HRESULT hr = devices[i]->Acquire(); 

					assert(hr==DI_OK);
				}
			}
			
			if(events[i]!=INVALID_HANDLE_VALUE)
			{
				events_objects[events_s] = events[i]; 
				events_devices[events_s++] = i; 
			}
		}
		else if(events[i]||devices[i]) //paranoia
		{	
			if(devices[i]) devices[i]->Release(); devices[i] = 0;
			
			CloseHandle(events[i]); events[i] = 0;		
						
			thread.freeze(i); thread.pause = 0;
		}
				
		if(Somcontrol_mouse_capture>0)
		{
			Somcontrol_mouse_capture-=wait; 

			if(Somcontrol_mouse_capture<=0)
			{
				Somcontrol_mouse_capture = 0;
				Somcontrol_mouse = 0;

				//reveal: otherwise the mouse will have to move
				PostMessage(Somcontrol_focus,WM_SETCURSOR,0,0); 
			}
		}
		if(Somcontrol_board_capture>0)
		{
			Somcontrol_board_capture-=wait; 

			if(Somcontrol_board_capture<=0)
			{
				Somcontrol_board_capture = 0;
				Somcontrol_board = 0;
			}
		}

		if(pausing)			
		{
			pausing-=wait;

			if(pausing<=0)
			{
				pausing = 0; Somthread_h::section cs; 

				Somcontrol *p = multitap[Somcontrol_syskeyboard];

				if(p) p->cpp->buttons[thread.pause] = 0; thread.pause = 0;

				if(p) thread.post(Somcontrol_syskeyboard);				
			}
		}

		DWORD evt = 
		MsgWaitForMultipleObjects(events_s,events_objects,0,wait,QS_ALLINPUT); 

		wait = evt>events_s?(frozen?100:poll_wait):0; //remember not to Sleep!

		if(evt==events_s)
		{
			MSG msg;
			while(PeekMessageW(&msg,0,0,0,PM_REMOVE))
			{ 
				if(msg.message==WM_QUIT) 
				{ 
					HMODULE dll = Somplayer_dll();

					if(thread.directinput8)
					{
						if(dll) thread.directinput8->Release(); 
						
						thread.directinput8 = 0;
					}

					for(size_t i=0;i<multitap.size;i++) 
					{
						thread.freeze(i);

						if(devices[i]&&dll) devices[i]->Release();

						CloseHandle(events[i]);
					}

					thread.pause = 0;

					return msg.wParam;
				} 
				//Does not affect SysKeyboard
				//TranslateMessage(&msg); 
				DispatchMessage(&msg); 
			} 
		}
		else if(evt<events_s)
		{
			size_t tap = events_devices[evt];
						
			IDirectInputDevice8W *q = devices[tap]; 

			SetThreadExecutionState(ES_DISPLAY_REQUIRED);

			bool post = false;
								  
			//assuming appropriate		 
			if(q&&q->Acquire()>=0) 
			{
				enum{ sz = sizeof(DIDEVICEOBJECTDATA) };

				DWORD buffered = INFINITE; HRESULT hr = 
				q->GetDeviceData(sz,0,&buffered,DIGDD_PEEK);

				//TODO: make this a subroutine
				if(buffered!=INFINITE&&buffered) 
				{	
					HRESULT test = 0;
					if(hr==DI_BUFFEROVERFLOW) 
					{							
						thread.thaw(tap,q); //hack: synchronize state

						//TODO: determine if buffer expansion is appropriate

						q->Unacquire();
						q->GetProperty(DIPROP_BUFFERSIZE,&dpwbs.diph);

						dpwbs.dwData*=2; //expand the buffer size

						q->SetProperty(DIPROP_BUFFERSIZE,&dpwbs.diph);
						q->Acquire();
					}

					static const size_t data_s = 512;

					static DIDEVICEOBJECTDATA data[data_s];

					while(buffered)
					{
						DWORD inout = data_s;
						
						if(inout>buffered) inout = buffered;

						if(hr=q->GetDeviceData(sz,data,&inout,0)) //DI_OK
						{
							assert(hr==DI_BUFFEROVERFLOW); 

							if(!inout)
							{
								inout = INFINITE;
								q->GetDeviceData(sz,0,&inout,0);
								break;
							}
						}

						Somthread_h::section cs;

						//freeze_and_thaw_mouse
						if(thread.mice>0&&tap==Somcontrol_sysmouse) 
						{
							buffered-=inout; continue;
						}

						Somcontrol *p = multitap[tap]; if(!p) break;
												
						size_t bts = p->di8->buttons.size();

						for(size_t i=0;i<inout;i++) if(data[i].dwOfs<bts)
						{
							size_t bt = data[i].dwOfs; 
							
							if(tap==Somcontrol_syskeyboard) 
							{	
								if(p->di8->buttons[bt].dwType>>8!=DIK_PAUSE) 
								{
									if(bt=scancodemap[bt])
									{
										p->cpp->buttons[bt-1] = data[i].dwData&0x80?1:0;
									}
								}
								else //the pause scancode is an event
								{
									p->cpp->buttons[thread.pause=bt] = 1;

									pausing = 1000;									
								}
							}
							else p->cpp->buttons[bt] = data[i].dwData&0x80?1:0;
						}
						else
						{								
							size_t j = (data[i].dwOfs-bts)/4; 

							float &action = p->cpp->actions[j];

							if(p->di8->actions[j].dwType&DIDFT_POV)
							{
								if((data[i].dwData&0xFFFF)!=0xFFFF)
								{
									float rads = 1.0f/36000*3.141592*2*data[i].dwData;

									action = std::sin(rads); p->cpp->actions[j+1] = -std::cos(rads);
								}
								else action = p->cpp->actions[j+1] = 0;
							}
							else if(p->di8->dataformat.dwFlags==DIDF_RELAXIS)
							{
								action+=p->di8->actions[j]*data[i].dwData;

								if(action>1) action = 1; else if(action<-1) action = -1; //saturate
							}
							else action = p->di8->actions[j]*data[i].dwData;

							assert(j<p->di8->actions.size()); 
						}
						
						if(p->cpp->poboxes) post = true;

						buffered-=inout;
					}
				}

				//Reminder: causes keys to repeat/not come up
				//q->Unacquire();
			}			
						
			if(post) thread.post(tap);
		}
		else if(!frozen) //polling
		{
			//freeze_and_thaw_mouse
			if(thread.mice>0) thread.mice-=wait; 
		
			for(size_t i=0,n=multitap.size;i<n;i++)
			{
				if(!devices[i]) continue;

				devices[i]->Acquire();
				devices[i]->Poll();
			}
		}
    } 
}

void Somcontrol_cpp::thread::post(size_t tap)
{
	Somthread_h::section cs;  
				
	if(tap>=Somcontrol_cpp::multitap.size) return;

	Somcontrol *p = Somcontrol_cpp::multitap[tap]; if(!p) return; 

	for(size_t i=0,n=p->cpp->postmen.size();i<n;i++)
	{
		if(IsWindow(p->cpp->postmen[i].window))
		{
			postman pm = p->cpp->postmen[i];

			PostMessage(pm.window,pm.message,pm.wparam,pm.lparam);
		}
		else p->cpp->postmen[i].window = 0;
	}			
}

void Somcontrol_cpp::thread::freeze(size_t tap)
{	
	Somthread_h::section cs; 

	if(tap>=Somcontrol_cpp::multitap.size) return;

	Somcontrol *p = Somcontrol_cpp::multitap[tap]; if(!p) return; 

	size_t sz = p->button_count()+p->action_count(); if(!sz) return;
	
	memset(p->cpp->buttons,0x00,sizeof(float)*sz);	
}

void Somcontrol_cpp::thread::thaw(size_t tap, IDirectInputDevice8W *out)
{
	//not much point / translation is hard
	if(tap==Somcontrol_syskeyboard) return; 

	if(!out||tap>=Somcontrol_cpp::multitap.size) return; //paranoia

	Somthread_h::section cs;	

	Somcontrol *p = Somcontrol_cpp::multitap[tap]; if(!p||!p->di8) return; 

	if(out->Acquire()<0) return; //TODO: wait for acquisition?

	BYTE *bytes = p->di8->databytes; DWORD *words = p->di8->datawords;

	if(out->GetDeviceState(p->di8->dataformat.dwDataSize,bytes)<0) return;

	size_t bts = p->di8->buttons.size(), ats = p->di8->actions.size();

	float *buttons = p->cpp->buttons, *actions = p->cpp->actions;

	for(size_t i=0;i<bts;i++) 
	{
		buttons[i] = bytes[i]&0x80?1:0;
	}
	for(size_t i=0;i<ats;i++) 
	{
		float &action = actions[i];

		if(p->di8->actions[i].dwType&DIDFT_POV)
		{
			if((words[i]&0xFFFF)!=0xFFFF)
			{
				float rads = 1.0f/36000*3.141592*2*words[i];

				action = std::sin(rads); actions[i+1] = -std::cos(rads);
			}
			else action = p->cpp->actions[i+1] = 0;
		}
		else if(p->di8->dataformat.dwFlags==DIDF_RELAXIS)
		{
			action+=p->di8->actions[i]*words[i];

			if(action>1) action = 1; else if(action<-1) action = -1; 
		}
		else action = p->di8->actions[i]*words[i];
	}
}

void Somcontrol_cpp::thread::freeze_and_thaw_mouse(int ms)
{
	assert(GetCurrentThreadId()!=id); //public

	if(Somcontrol_sysmouse<Somcontrol_cpp::multitap.size)
	{
		if(Somcontrol_cpp::multitap.refs[Somcontrol_sysmouse])
		{
			mice = ms; Sleep(10); freeze(Somcontrol_sysmouse);
		}
	}
}

IDirectInputDevice8W *Somcontrol_cpp::thread::di8(size_t tap)
{
	Somthread_h::section cs;

	if(tap>=Somcontrol_cpp::multitap.size) return 0; //paranoia

	Somcontrol_cpp::di8device *di8 = 
	Somcontrol_cpp::multitap[tap]->di8; if(!di8) return 0; //paranoia

	if(!directinput8) 
	DirectInput8Create(Somplayer_dll(),
	DIRECTINPUT_VERSION,IID_IDirectInput8W,(void**)&directinput8,0);	
	if(!directinput8) return 0; //paranoia	

	REFGUID guid = di8->dideviceinstance.guidInstance;

	IDirectInputDevice8W *out = 0;
	directinput8->CreateDevice(guid,&out,0); if(!out) return 0; //paranoia

	if(!di8->dataformat.rgodf)
	{	
		size_t ats = di8->actions.size();
		size_t bts = di8->buttons.size(); if(bts%4) bts+=2; 
		
		size_t sizeof_rgodf = sizeof(DIOBJECTDATAFORMAT)*(bts+ats);

		BYTE *p = new BYTE[sizeof_rgodf+bts+ats*4];
		
		di8->databytes = p+sizeof_rgodf;
		di8->datawords = (DWORD*)(p+sizeof_rgodf+bts);
			
		di8->dataformat.dwSize = sizeof(DIDATAFORMAT);
		di8->dataformat.dwObjSize = sizeof(DIOBJECTDATAFORMAT);

		int axis = 0; //using whatever is advertised
		
		//apparently these are mutually exclusive???
		for(size_t i=0,n=di8->actions.size();i<n;i++) 
		{
			switch(DIDFT_GETTYPE(di8->actions[i].dwType))
			{
			case DIDFT_RELAXIS: axis = DIDF_RELAXIS; i = n; break;
			case DIDFT_ABSAXIS: axis = DIDF_ABSAXIS; i = n; break;
			}
		}

		di8->dataformat.dwFlags = axis; //0
		di8->dataformat.dwDataSize = bts+ats*4;
		di8->dataformat.dwNumObjs = 0; //homework
		di8->dataformat.rgodf = (DIOBJECTDATAFORMAT*)p;
		
		for(size_t i=0,n=di8->buttons.size();i<n;i++)
		{	
			DIOBJECTDATAFORMAT &odf =
			di8->dataformat.rgodf[di8->dataformat.dwNumObjs++];

			DIDEVICEOBJECTINSTANCEW &doi = di8->buttons[i];
			
			odf.pguid = &doi.guidType; odf.dwOfs = i;
			odf.dwType = doi.dwType; odf.dwFlags = 0;

			//button_count wants the buttons to be even
			if(!odf.dwType) di8->dataformat.dwNumObjs--;

			doi.dwOfs = odf.dwOfs; //may as well
		}

		for(size_t i=0,j=bts,n=di8->actions.size();i<n;i++,j+=4)
		{
			DIOBJECTDATAFORMAT &odf =
			di8->dataformat.rgodf[di8->dataformat.dwNumObjs++];

			DIDEVICEOBJECTINSTANCEW &doi = di8->actions[i];

			odf.dwOfs = j; odf.dwType = doi.dwType; odf.dwFlags = 0;

			if(doi.dwType&DIDFT_POV) //treating as twin axes
			{
				odf.pguid = &GUID_POV; i++; j+=4; //skip the Y axis
			}
			else odf.pguid = &doi.guidType;

			doi.dwOfs = odf.dwOfs; 					
		}
	}
								
	out->SetDataFormat(&di8->dataformat);

	return out;
}	  

static int Somcontrol_sc2dik(size_t sc)
{
	switch(sc) //US centric
	{
	default: return sc<=DIK_F15?sc:0;

	case 0XE010: return DIK_PREVTRACK;
	case 0XE019: return DIK_NEXTTRACK;
    case 0XE01C: return DIK_NUMPADENTER;
    case 0XE01D: return DIK_RCONTROL;

	case 0XE020: return DIK_MUTE;
	case 0XE021: return DIK_CALCULATOR;
	case 0XE022: return DIK_PLAYPAUSE;
	case 0XE024: return DIK_MEDIASTOP;
	case 0XE02E: return DIK_VOLUMEDOWN;
    case 0XE030: return DIK_VOLUMEUP;
    case 0XE01E: return DIK_WEBHOME; //?? 

    case 0XE035: return DIK_DIVIDE;
    case 0XE037: return DIK_SYSRQ;
    case 0XE038: return DIK_RMENU;
	//case 'N/A': return DIK_PAUSE;
    case 0XE047: return DIK_HOME;
    case 0XE048: return DIK_UP;
    case 0XE049: return DIK_PRIOR;
    case 0XE04B: return DIK_LEFT;
    case 0XE04D: return DIK_RIGHT;
    case 0XE04F: return DIK_END;
    case 0XE050: return DIK_DOWN;
    case 0XE051: return DIK_NEXT;
    case 0XE052: return DIK_INSERT;
    case 0XE053: return DIK_DELETE;
    case 0XE05B: return DIK_LWIN;
    case 0XE05c: return DIK_RWIN;
    case 0XE05D: return DIK_APPS;
    case 0xE05E: return DIK_POWER;
	case 0xE05F: return DIK_SLEEP;
	case 0xE063: return DIK_WAKE;

	//WebTV?
    //case '???': return DIK_WEBSEARCH;
    //case '???': return DIK_WEBFAVORITES;
    //case '???': return DIK_WEBREFRESH;
    //case '???': return DIK_WEBSTOP;
    //case '???': return DIK_WEBFORWARD;
    //case '???': return DIK_WEBBACK;
	
	case 0XE06B: return DIK_MYCOMPUTER;
    case 0XE06C: return DIK_MAIL;    
	case 0XE06D: return DIK_MEDIASELECT;
	}
}

unsigned char* //static 
Somcontrol_cpp::thread::scancodemap()
{		
	unsigned char dik2dik[256]; 

	for(int i=0;i<256;i++) dik2dik[i] = i;

	struct scancodemap
	{
		__int8 header[8]; __int16 entries_s, _;

		struct{ unsigned __int16 mapping, scancode; }entries[256];

	}map;		

	DWORD sz = sizeof(map), err = SHGetValueW(HKEY_LOCAL_MACHINE, 
	L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layout",L"Scancode Map",0,&map,&sz);

	if(!err&&sz) for(int i=0,n=std::min<int>(map.entries_s,256);i<n;i++) 
	{
		dik2dik[Somcontrol_sc2dik(map.entries[i].scancode)]	= Somcontrol_sc2dik(map.entries[i].mapping);
	}

	//hack: the returned table really goes bt2bt+1

	assert(Somcontrol_syskeyboard<SOMCONTROL_DEVICES);

	Somcontrol *kb = Somcontrol_cpp::multitap[Somcontrol_syskeyboard];
		
	unsigned char dik2bts[256]; memset(dik2bts,0x00,256); 

	for(size_t i=0,n=kb->di8->buttons.size();i<n;i++)
	{
		dik2bts[kb->di8->buttons[i].dwType>>8&0xFF] = i+1;
	}

	static unsigned char out[256]; memset(out,0x00,256);

	for(size_t i=dik2bts[0]=0,n=kb->di8->buttons.size();i<n;i++)
	{
		out[i] = dik2bts[dik2dik[kb->di8->buttons[i].dwType>>8&0xFF]];
	}

	return out;
}

#define Somcontrol_SOM L"Software\\FROMSOFTWARE\\SOM"

namespace Somcontrol_cpp
{			  
	typedef Somregis_h::multi_sz<Somcontrol*,64> multi_sz;
}

inline int Somcontrol_cpp::multi_sz::f(Somcontrol *in, wchar_t (&ln)[64], size_t &i)
{	
	//Control Name
	//v00, 0100000C
	//000, Move Left 
	//012, Turn Right

	using namespace Somcontrol_h;

	enum{ h=2, x=h+BUTTONS*in->CONTEXTS, v=0 }; //version
		
	switch(i)
	{
	case 0: return _snwprintf(ln,N-1,L"%s",in->device());
	case 1: return _snwprintf(ln,N-1,L"v%02x, %04x%04x",v,BUTTONS,ACTIONS);

	default: int compile[h==2]; //header
		
		while(i<x&&!in->contexts[0][i-h]) i++; if(i>=x) return 0;
	}

	int bt = i-h, compile[sizeof(in->contexts)==sizeof(button)*(x-h)];

	return swprintf_s(ln,L"%03x, %s",bt,in->contexts[0][bt].text());
}

inline int Somcontrol_cpp::multi_sz::g(Somcontrol *out, wchar_t (&ln)[64], size_t &i)
{
	using namespace Somcontrol_h;
		
	switch(i)
	{
	case 0: //Name 

		//TODO: set instance name

		memset(out->contexts,0x00,sizeof(out->contexts));

		return 1; //keep going 

	case 1: //Version
	{	
		int v, b, a, s = swscanf_s(ln,L"v%02x, %04x%04x",&v,&b,&a);

		//FYI: too support multiple versions/layouts 
		//T will have to wrap around the Somcontrol*
		return s!=3||v||b!=BUTTONS||a!=ACTIONS?-1:1;
	}
	default: //Button
		
		if(!ln[0]) return 0; //last line
	}

	wchar_t text[16] = L"";

	int bt, s = swscanf_s(ln,L"%03x, %[^,]",&bt,text,16);

	if(s!=2||bt>=BUTTONS||bt<0) return -1;

	out->contexts[0][bt] = text;

	return 1; //keep going
}

bool Somcontrol::export(const wchar_t *player, HWND owner)
{
	if(!this||!api||!di8) return false;
		
	if(!player||!*player||*player=='.') //.database
	{
		return false; //TODO: if(owner) MessageBox(yada yada yada);		
	}
	
	GUID *guid = &di8->dideviceinstance.guidInstance;

	if(owner) //TODO: Export dialog
	{
		assert(0); return false; //unimplemented
	}

	const size_t subkey_s = 128; 	
	wchar_t subkey[subkey_s] = L"", *config = L"PLAYER";

	if(owner) config = L"CONFIG"; //TODO: allow alternative key

	int cat = swprintf_s(subkey,L"%s\\%s\\%s\\",Somcontrol_SOM,config,player);
	
	if(cat<=0||!StringFromGUID2(*guid,subkey+cat,subkey_s-cat)) return false;

	Somcontrol_cpp::multi_sz wr = this; if(wr.cb<sizeof(wchar_t)*2) return false;
	
	const wchar_t *home = L""; //TODO: get the current project/game directory

	LONG err = SHSetValueW(HKEY_CURRENT_USER,subkey,home,REG_MULTI_SZ,wr.sz,wr.cb);

	return err==ERROR_SUCCESS;
}

bool Somcontrol::import(const wchar_t *player, HWND owner)
{
	if(!this||!api||!di8) return false;
		
	if(!player||!*player||*player=='.') //.database
	{
		return false; //TODO: if(owner) MessageBox(yada yada yada);		
	}
	
	GUID *guid = &di8->dideviceinstance.guidInstance;

	if(owner) //TODO: Export dialog
	{
		assert(0); return false; //unimplemented
	}

	const size_t subkey_s = 128; 	
	wchar_t subkey[subkey_s] = L"", *config = L"PLAYER";

	if(owner) config = L"CONFIG"; //TODO: allow alternative key

	int cat = swprintf_s(subkey,L"%s\\%s\\%s\\",Somcontrol_SOM,config,player);
	
	if(cat<=0||!StringFromGUID2(*guid,subkey+cat,subkey_s-cat)) return false;
	
	const wchar_t *home = L""; //TODO: get the current project/game directory

	DWORD cb = 0; LONG err = 
	SHGetValueW(HKEY_CURRENT_USER,subkey,home,0,0,&cb);

	if(!cb||err) return false;

	Somcontrol_cpp::multi_sz rd(this,cb); err = 
	SHGetValueW(HKEY_CURRENT_USER,subkey,home,0,rd.sz,&rd.cb);	

	return err==ERROR_SUCCESS;
}

const wchar_t *Somcontrol_h::button::text()const
{						
	switch(id)
	{
	case 0: return L"";

	case ID_MOVE_LEFT: return L"Move Left";
	case ID_MOVE_RIGHT: return L"Move Right";
	case ID_MOVE_FORWARD: return L"Move Forward";
	case ID_MOVE_BACKWARD: return L"Move Backward";

	case ID_MOVE_UP: return L"Move Up";
	case ID_MOVE_DOWN: return L"Move Down";

	case ID_MOVE_SPEED: return L"Move Speed";

	case ID_TURN_LEFT: return L"Turn Left";
	case ID_TURN_RIGHT: return L"Turn Right";

	case ID_ROLL_LEFT: return L"Roll Left";
	case ID_ROLL_RIGHT: return L"Roll Right";
	case ID_ROLL_FORWARD: return L"Roll Forward";
	case ID_ROLL_BACKWARD: return L"Roll Backward";

	case ID_LOOK_UP: return L"Look Up";
	case ID_LOOK_DOWN: return L"Look Down";
	case ID_LOOK_LEFT: return L"Look Left";
	case ID_LOOK_RIGHT: return L"Look Right";
	case ID_LOOK_FORWARD: return L"Look Forward";

	case ID_PULL_UP: return L"Pull Up";
	case ID_PULL_BACK: return L"Pull Back";

	//game / client specific business

	case ID_TURN_ON: return L"Turn On";
	case ID_MENU_ON: return L"Main Menu";

	case ID_LEFT_TRIGGER: return L"Left Trigger";
	case ID_RIGHT_TRIGGER: return L"Right Trigger";

	//system functions

	case ID_POWER_ON: return L"Power";
	case ID_POWER_RTC: return L"Pause";			
	case ID_POWER_MICE: return L"Mice On";
	case ID_POWER_KEYS: return L"Keys On";

	case ID_POWER_BELL: return L"Bell";

	default: assert(0); 
	}
	
	return L"?";
};

Somcontrol_h::button& 
Somcontrol_h::button::operator=(const wchar_t *text)
{
	id = 0; 
	
	if(!text||!*text) return *this;

	typedef std::map<std::wstring,int> key;

	static key set; static const key &out = set;

	if(set.empty())
	{
		Somthread_h::section cs;

		if(!set.empty()) return operator=(text);

		set[L"Move Left"] = ID_MOVE_LEFT;
		set[L"Move Right"] = ID_MOVE_RIGHT;
		set[L"Move Forward"] = ID_MOVE_FORWARD;
		set[L"Move Backward"] = ID_MOVE_BACKWARD;

		set[L"Move Up"] = ID_MOVE_UP;
		set[L"Move Down"] = ID_MOVE_DOWN;

		set[L"Move Speed"] = ID_MOVE_SPEED;

		set[L"Turn Left"] = ID_TURN_LEFT;
		set[L"Turn Right"] = ID_TURN_RIGHT;

		set[L"Roll Left"] = ID_ROLL_LEFT;
		set[L"Roll Right"] = ID_ROLL_RIGHT;
		set[L"Roll Forward"] = ID_ROLL_FORWARD;
		set[L"Roll Backward"] = ID_ROLL_BACKWARD;

		set[L"Look Up"] = ID_LOOK_UP;
		set[L"Look Down"] = ID_LOOK_DOWN;
		set[L"Look Left"] = ID_LOOK_LEFT;
		set[L"Look Right"] = ID_LOOK_RIGHT;
		set[L"Look Forward"] = ID_LOOK_FORWARD;

		set[L"Pull Up"] = ID_PULL_UP;
		set[L"Pull Back"] = ID_PULL_BACK;

		//game / client specific business

		set[L"Turn On"] = ID_TURN_ON;
		set[L"Main Menu"] = ID_MENU_ON;

		set[L"Left Trigger"] = ID_LEFT_TRIGGER;
		set[L"Right Trigger"] = ID_RIGHT_TRIGGER;

		//system functions

		set[L"Power"] = ID_POWER_ON;
		set[L"Pause"] = ID_POWER_RTC;
		set[L"Mice On"] = ID_POWER_MICE;
		set[L"Keys On"] = ID_POWER_KEYS;

		set[L"Bell"] = ID_POWER_BELL;
	}

	key::const_iterator it = out.find(text);

	if(it==out.end()) 
	{	
		wchar_t i, temp[32+1];
		for(i=0;text[i]&&i<32;i++)
		{				
			temp[i] = text[i];

			if(i==0||temp[i-1]==' ')
			{
				temp[i] = toupper(text[i]);
			}
			else temp[i] = tolower(text[i]);
		}	
		temp[i] = '\0'; 
		
		it = out.find(temp);

		if(it!=out.end()) id = it->second;		
	}
	else id = it->second; 

	return *this;
}
