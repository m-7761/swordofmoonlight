
#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

//XInput detection	
//#include <wbemidl.h>
//#include <oleauto.h>
//#include <wmsstd.h> //SAFE_RELEASE
//#define SAFE_RELEASE(p) {if((p)){(p)->Release();(p)=0;}}
#include <vector>

#pragma comment(lib,"Winmm.lib") 

//want to compile on (private) XP builds
//#include "XInput.h" 
//#pragma comment(lib,"xinput.lib")
typedef struct _XINPUT_GAMEPAD
{
    WORD                                wButtons;
    BYTE                                bLeftTrigger;
    BYTE                                bRightTrigger;
    SHORT                               sThumbLX;
    SHORT                               sThumbLY;
    SHORT                               sThumbRX;
    SHORT                               sThumbRY;
} XINPUT_GAMEPAD, *PXINPUT_GAMEPAD;
typedef struct _XINPUT_STATE
{
    DWORD                               dwPacketNumber;
    XINPUT_GAMEPAD                      Gamepad;
} XINPUT_STATE, *PXINPUT_STATE;
static DWORD(WINAPI*Ex_input_XInputGetState)(DWORD,XINPUT_STATE*) = 0;

#include "dx.dinput.h"

#include "Ex.ini.h"
#include "Ex.input.h"
#include "Ex.window.h"
#include "Ex.output.h" //EX::dbgmsg

#include "som.state.h" //hack

#include "../Somplayer/Somvector.h" //hack

namespace EX{ extern float mouse[3]; }
namespace DDRAW{ extern bool isPaused; }

extern bool EX::vk_pause_was_pressed = false;

float EX::Joypad::analog_scale_3 = 0.5f;
float EX::Joypad::analog_scale_4 = 0.666666f;
bool (*EX::Joypad::thumbs)(int,float[8]) = 0;

float EX::Joypad::analog_dilation = 1;

//REMOVE ME?
unsigned &EX::Joypad::analog_clock = DINPUT::noPolls; 	
unsigned &EX::Pedals::analog_clock = DINPUT::noPolls; 	

class EX::Keypad EX::Keypad; class EX::Syspad EX::Syspad;

EX::Pedals EX::Affects[EX_AFFECTS];        
EX::Joypad EX::Joypads[EX_JOYPADS+EX_MICE];
EX::Joypad &EX::Mouse = EX::Joypads[EX_JOYPADS];

EX::Pedals::Configuration EX::Pedals::pedscfg;
EX::Joypad::Configuration EX::Joypad::padscfg;
EX::Joypad::Configuration EX::Joypad::micecfg;

static EX::INI::Joypad::Section Ex_input_mice[EX_MICE];
static EX::INI::Joypad::Section Ex_input_default_joypad[1];

void EX::Joypad::assign_gaits_if_default_joypad(const float gaits[7])
{
	if(ini!=EX::INI::Joypad(Ex_input_default_joypad)) return;

	if(!&Ex_input_default_joypad->axis_analog_gaits)
	Ex_input_default_joypad->axis_analog_gaits = L"0";
	if(!&Ex_input_default_joypad->axis2_analog_gaits)
	Ex_input_default_joypad->axis2_analog_gaits = L"0";
	if(!&Ex_input_default_joypad->slider_analog_gaits)
	Ex_input_default_joypad->slider_analog_gaits = L"0";
	if(!&Ex_input_default_joypad->slider2_analog_gaits)
	Ex_input_default_joypad->slider2_analog_gaits = L"0";

	for(int i=0;i<7;i++) //should be safe
	{	
		(float&)ini->axis_analog_gaits[i] = gaits[i];
		(float&)ini->axis2_analog_gaits[i] = gaits[i];
		(float&)ini->slider_analog_gaits[i] = gaits[i];
		(float&)ini->slider2_analog_gaits[i] = gaits[i];
	}
}

extern EX::INI::Joypad EX::universal_mouse(Ex_input_mice);

int EX::Joypad::wm_input[EX_MICE*2] = {INT_MAX}; 
float EX::Joypad::absolute[EX_MICE*3]; 
float EX::Joypad::smoothing[EX_MICE*2][4];

static DX::IDirectInput7W *Ex_input_directinput = 0;

static BOOL CALLBACK Ex_input_enumdevicescb(DX::LPCDIDEVICEINSTANCEW x, LPVOID y)
{		
	for(int i=0;i<EX_JOYPADS;i++) 		
	if(EX::Joypads[i].instance==x->guidInstance) return DIENUM_CONTINUE; 
	for(int i=0;i<EX_JOYPADS;i++) if(GUID_NULL==EX::Joypads[i].instance)
	{				
		EX::INI::Joypad p = 0, q = p;

		for(i=0;i<EX_JOYPADS;i++) if(q=EX::INI::Joypad(1+i))
		if(PathMatchSpecW(x->tszInstanceName,q->joypad_to_use_for_play))
		{
			p = q; break; //this one's a keeper
		}
		if(!p) if(p=EX::INI::Joypad(1)) //anonymous?
		{
			if(*p->joypad_to_use_for_play) break; //unplugged?
		}	 
		if(p) EX::Joypads[i].activate(p,x->guidInstance,x->guidProduct); break;
	}		 
	return DIENUM_CONTINUE;
}

static bool Ex_input_initialize()
{  
	HRESULT ok = DI_OK;

	static bool initializing = false;

	if(initializing) return true; //assuming recursion

	if(!Ex_input_directinput) //IID_IDirectInput8W
	if(DINPUT::DirectInputA)
	{
		ok = DINPUT::DirectInputA->proxy->
		QueryInterface(DX::IID_IDirectInput7W,(LPVOID*)&Ex_input_directinput);

		if(ok!=DI_OK){ assert(0); return false; } //unimplemented

		initializing = true;

		if(EX::INI::Joypad())
		Ex_input_directinput->EnumDevices(DIDEVTYPE_JOYSTICK,Ex_input_enumdevicescb,0,DIEDFL_ATTACHEDONLY);
		
		
		//CAN CAUSE LONG DELAY 
		//doing in som_hacks_thread_main
		//EX::Joypad::detectXInputDevices(); //NEW

		if(EX_MICE) EX::Mouse.activate(EX::universal_mouse,DX::GUID_SysMouse);

		initializing = false;
	}
	else
	{
		assert(0); return false;
	}

	return true;
}

bool EX::Keypad::simulate(unsigned char *in, size_t sz, unsigned char *unx)
{		
	size_t xsz = min(sz,keystates_s);

	static unsigned char xin[keystates_s];
	
	if(!unx) unx = xin; //untranslated
											
	if(!DINPUT::Keyboard) return false;

	HRESULT ok = !DI_OK;
	
	if(DINPUT::Keyboard->rawdata) //mostly paranoia
	{	
		assert(0); //are we using this?
		assert(DINPUT::Keyboard->format);
				
		unsigned char *rin = DINPUT::Keyboard->rawdata;

		size_t rsz = 0;		
		if(DINPUT::Keyboard->format)
		rsz = DINPUT::Keyboard->format->dwDataSize;

		if(xsz>rsz) //zero excess memory
		memset(unx+rsz,0x00,(xsz-rsz));

		xsz = min(xsz,rsz);		
				
		memcpy(unx,rin,xsz); //the point of all this
	}

	DX::DIDEVICEOBJECTDATA *data = DINPUT::Keyboard->data();

	if(data) //UNUSEDS
	{
		assert(0); //UNUSED?
		//sampling conservatively from buff
		for(size_t i=0;i<DINPUT::Keyboard->bufferdata;i++) 
		{
			if(data[i].dwData&0x80&&data[i].dwOfs<xsz)			
			{				
				unx[data[i].dwOfs] = 0x80;
			}
		}

		ok = DI_OK;
	}
	else if(1) //todo: timeout
	{	 
		//TESTING: the keyboard always dies if I use a debugger for
		//more than a few seconds. GetDeviceState returning DI_OK
		//som.status.cpp calls Acquire so I don't think this helps
		if(EX::debug&&IsDebuggerPresent())
		{
			/*som.status.cpp calls Acquire so I don't think this helps
			HRESULT a = DINPUT::Keyboard->proxy->Acquire();
			if(!DINPUT::Keyboard->as2A)
			{
				GUID IID_IDirectInputDevice2A={0x5944E682,0xC92E,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00};
				DINPUT::Keyboard->QueryInterface(IID_IDirectInputDevice2A,(void**)&DINPUT::Keyboard->as2A);
			}
			HRESULT b = DINPUT::Keyboard->as2A->Poll(); //TESTING
			//assert(!a&&!b||GetForegroundWindow()!=EX::display());
			//EX::dbgmsg("kb: %d %d",a,b);
			assert(1==a&&1==b); //S_FALSE always? (while working) */

			//NOTE: I think it's MessageBox style interactions that
			//kill keyboard input. mouse drops out just from changing
			//app windows :(
			
			DINPUT::Keyboard->proxy->SetCooperativeLevel(EX::display(),DISCL_NONEXCLUSIVE|DISCL_FOREGROUND);
			DINPUT::Keyboard->proxy->Acquire();
		}

		while((ok=DINPUT::Keyboard->proxy->GetDeviceState(xsz,unx))!=DI_OK)
		{					
			ok = DINPUT::Keyboard->proxy->Acquire(); //S_FALSE always?

			if(ok!=DI_OK&&ok!=DIERR_INPUTLOST&&ok!=E_ACCESSDENIED) break;					
		}
	}
	else ok = DINPUT::Keyboard->proxy->GetDeviceState(xsz,unx);			

	if(ok!=DI_OK) return false;		
		
	if(EX::vk_pause_was_pressed) unx[DIK_PAUSE]|=0x80;

	int c = EX::context();

	for(size_t i=0;i<xsz;i++)
	if(unx[i]&0x80&&EX::validating_direct_input_key(i,(char*)unx))
	{
		if(keystates[i]) //2020: prevent cycles like controller
		{
			if(contexts[i]!=c) contexts[i] = -1;
		}
		else{ keystates[i] = true; contexts[i] = c; }
	}
	else{ keystates[i] = false; contexts[i] = c; }	
				
	EX::vk_pause_was_pressed = false;			

	EX::INI::Keypad kp;

	if(!kp) //todo: repetition/etc
	{
		if(in) for(size_t i=0;i<xsz;i++) 
		{
			if(keystates[i]&&contexts[i]==c) in[i]|=0x80;
		}				 
		return true;
	}
	
	int i = 0, x_completed = 0;

	for(i;i<MAX_MACROS&&macros[i];i++)
	{		
		macros[i-x_completed] = macros[i];

		macro_runaways[i-x_completed] = macro_runaways[i];

		const unsigned char* &p = macros[i-x_completed];

		int &r = macro_runaways[i-x_completed];
		int &c = macro_counters[i-x_completed];

		const unsigned char *q = p; //rewind

		if(!c) c = *p; assert(c);

		if(*p) for(p++,r++,c--;*p;p++)
		{			
			if(*p<sz) in[*p]|=0x80;
			
			if(r++>EX_MACRO_RUNAWAY_ALERT_ON)
			if(!EX::issue_macro_runaway_warning()) 
			goto err1; //player opted to kill macro
		}		

		if(*p) //macro appears to be bad 
		{
err1:		assert(0); //TODO: handle gracefully??
				
			x_completed++; //just toss it 

			p = 0; c = 0; continue;
		}
		else p = c?q:p+1;
		
		if(!*p) 
		{		
			r = 0; p = 0; c = 0;

			x_completed++;
		}
	}

	int x_remaining = i-x_completed, x_appended = 0;

	const unsigned char **x_new = &macros[x_remaining];

	int *x_new_runaways = &macro_runaways[x_remaining];
	int *x_new_counters = &macro_counters[x_remaining];

	for(size_t i=1;i<=xsz;i++) 
	if(keystates[i]&&contexts[i]==c)
	{
		unsigned short x = EX::INI::Keypad()->translate(i);

		if(x!=EX_INI_BAD_MACRO) if(x>255)
		{
			if(x_remaining+x_appended<MAX_MACROS-1)

			x_new[x_appended++] = EX::INI::Action()->xtranslate(x);
		}
		else if(in) in[x]|=0x80;		
	}

	x_completed = 0;

	for(int i=0;i<x_appended&&x_new[i];i++)
	{		
		x_new[i-x_completed] = x_new[i];

		x_new_runaways[i-x_completed] = x_new_runaways[i];

		const unsigned char* &p = x_new[i-x_completed];

		int &r = x_new_runaways[i-x_completed];
		int &c = x_new_counters[i-x_completed];

		const unsigned char *q = p; //rewind

		if(!c) c = *p; assert(c);

		if(*p) for(p++,r++,c--;*p;p++)
		{			
			if(*p<sz) in[*p]|=0x80;
			
			if(r++>EX_MACRO_RUNAWAY_ALERT_ON) 
			if(!EX::issue_macro_runaway_warning()) 
			goto err2; //player opted to kill macro
		}		

		if(*p) //macro appears to be bad 
		{
err2:		assert(0); //TODO: handle gracefully??
				
			x_completed++; //just toss it 

			p = 0; c = 0; continue;
		}
		else p = c?q:p+1;
		
		if(!*p) 
		{		
			r = 0; p = 0; c = 0;

			x_completed++;
		}
	}

	return true;
}

void EX::Syspad::_repeat(int behavior, unsigned char x, int n)
{
	if(n<1) return;

	if(n>EX::Syspad::MAX_REPEAT) n = EX::Syspad::MAX_REPEAT;

	if(_fifo%3==0&&_fifo+3<EX::Syspad::MAX_BUFFER*3)
	{
		_buff[_fifo+0] = behavior; 
		_buff[_fifo+1] = x; 
		_buff[_fifo+2] = n;

		_fifo+=3;
	}
	else //overflow: just dropping buff contents
	{
		if(behavior==10) //mission critical
		{
			EX::is_needed_to_shutdown_immediately(-1,"EX::Syspad::repeat");
		}

		assert(0); _fifo = 0;
	}
}

void EX::Syspad::_tie(int behavior, unsigned char x)
{
	if(!_fifo) return;

	int i;
	for(i=_fifo-3;i>0&&_buff[i]==0;i-=3); 

	if(_fifo%3==0&&_fifo+3<EX::Syspad::MAX_BUFFER*3)
	{
		_buff[_fifo+0] = 1; //tied

		if(behavior==1)
		{
			int swap = _buff[i+1]; //move x to front
			
			_buff[i+1] = x; _buff[_fifo+1] = swap;
		}
		else //unimplemented
		{
			assert(0); return;
		}

		_buff[_fifo+2] = behavior; 		

		_fifo+=3;
	}
	else //overflow: just dropping buff contents
	{
		assert(0); _fifo = 0;
	}
}

void EX::Syspad::exit(int code)
{
	_repeat(10,code,1);
}

bool EX::Syspad::simulate(unsigned char *x, size_t sz)
{	
	if(!_fifo) 
	{
		memcpy(memory,x,max(sz,EX::Syspad::MAX_MEMORY));

		if(sz<EX::Syspad::MAX_MEMORY)
		memset(memory,0x00,(EX::Syspad::MAX_MEMORY-sz));

		return true; //capturing in memory only
	}

	unsigned char y[EX::Syspad::MAX_MEMORY]; 
	memcpy(y,memory,EX::Syspad::MAX_MEMORY);
	memcpy(memory,x,max(sz,EX::Syspad::MAX_MEMORY));
	if(sz<EX::Syspad::MAX_MEMORY)
	memset(memory,0x00,(EX::Syspad::MAX_MEMORY-sz));	
		
	int in = _fifo, out = 0;

	unsigned char *p = _buff;
	unsigned char *q = _swap[0];

	if(q==p) q = _swap[1]; _buff = q;

	auto *qq = q; //2022

	bool carry = false;

	for(int i=0;i<in;i+=3,p+=3)
	{		
		if(p[1]>=sz){ assert(0); continue; }

		//2022: don't process same key twice?
		//(I think start_mode=1 is jamming by
		//the release event never processing)
		for(auto*x=qq;x<q;x+=3) if(p[1]==x[1])
		{
			goto carry; //breakpoint
		}
			
		switch(*p)
		{
		case 10: //exit

			if(_fifo==3)
			{
				EX::is_needed_to_shutdown_immediately(p[2],"EX::Syspad::simulate");
			}
			else carry = true; 
			
			break;

		case 2: case 3: case 4: //pressing

			if(!x[p[1]]&&!y[p[1]]) 
			{	
				if(*p==3) //3:echoing
				{
					EX::broadcasting_direct_input_key(p[1],0);
				}				
				else x[p[1]] = 0x80;

				if(--p[2]==0||*p==4) *p|=0x80; //4:spamming
			}

			carry = true; break;

		case 2|0x80: case 3|0x80: case 4|0x80: //releasing

			//TODO: consider forcing release

			if(carry=p[2]) *p&=0x7F; break;

		case 1: case 1|0x80: //tying

			if(carry) 			
			if(p[-3]&0x80) //was pressed
			{
				x[p[1]] = 0x80; *p|=0x80; //notify upstream ties
			}
			else *p&=0x7F; //reset

			break;
		}

		if(carry) carry:
		{
			//memcpy(q,p,sizeof(int)*3); //??? //2022
			memcpy(q,p,3); 
			
			q+=3; out+=3;
		}
	}

	_fifo = out;

	return true;
}

void EX::Joypad::deactivate()
{
	activate(0,GUID_NULL);
}	   
static const GUID Ex_input_DualShock4 = 
{0x09CC054C,0,0,0,0,0x50,0x49,0x44,0x56,0x49,0x44};
static const GUID Ex_input_DualShock5 = 
{0x0CE6054C,0,0,0,0,0x50,0x49,0x44,0x56,0x49,0x44};
bool EX::Joypad::activate(EX::INI::Joypad in, const GUID &guid, const GUID &XIid)
{		
	ini = in; instance = guid; product = XIid; 
	
	isDualShock = false; //HACK?
		
	if(dx) dx->Release(); if(dx7) dx7->Release();

	dx = 0; dx7 = 0; if(guid==GUID_NULL) return active = false;
	
	if(!Ex_input_initialize()) return active = false;

	HRESULT ok = DI_OK; 

	/*2021: not helping :( 
	//EXPERIMENTING WITH DROP OUT ISSUE WHEN ATTACHING THE DEBUGGER
	HWND coop = EX::display(); assert(EX::client&&coop!=EX::client);*/

	if(in==EX::universal_mouse)
	{	
		assert(EX_MICE==1);
		memset(absolute,0x00,sizeof(absolute));
		memset(smoothing,0x00,sizeof(smoothing));

		EX::INI::Option op;

		if(!op||!op->do_mouse) return active = false;

		if(Ex_input_directinput->CreateDevice(DX::GUID_SysMouse,&dx,0)!=DI_OK)
		{
			assert(ok==DI_OK); return active = false; //unimplemented
		}	   		
		else if(dx->QueryInterface(DX::IID_IDirectInputDevice7W,(LPVOID*)&dx7)!=DI_OK)
		{
			assert(ok==DI_OK); return active = false; //unimplemented
		}

	//	ok = dx7->SetCooperativeLevel(coop,DISCL_NONEXCLUSIVE|DISCL_BACKGROUND);
		ok = dx7->SetDataFormat(&DX::c_dfDIMouse2); 
	}
	else
	{
		if(product==Ex_input_DualShock4) isDualShock = 4;
		if(product==Ex_input_DualShock5) isDualShock = 5;

		if(Ex_input_directinput->CreateDevice(guid,&dx,0)!=DI_OK)
		{
			assert(ok==DI_OK); return active = false; //unimplemented
		}
		else if(dx->QueryInterface(DX::IID_IDirectInputDevice7W,(LPVOID*)&dx7)!=DI_OK)
		{
			assert(ok==DI_OK); return active = false; //unimplemented
		}

	//	dx->SetCooperativeLevel(coop,DISCL_NONEXCLUSIVE|DISCL_BACKGROUND); 
			
		dx->SetDataFormat(&DX::c_dfDIJoystick);	

		/*Not having any effect with PS3 controller with non-circular inputs.
		DX::DIPROPDWORD setpropw = 
		{
			{sizeof(setpropw),sizeof(setpropw.diph),0,DIPH_DEVICE}
		};
		setpropw.dwData = DIPROPCALIBRATIONMODE_RAW;
		if(dx->SetProperty(DIPROP_CALIBRATIONMODE,&setpropw.diph)) assert(0);
		setpropw.dwData = 10000;
		if(dx->SetProperty(DIPROP_SATURATION,&setpropw.diph)) assert(0);		
		*/

		if(isDualShock) //EXPERIMENTAL
		{
			JslDisconnectAndDisposeAll();
			int n = JslConnectDevices(); //2024: soft load Exselector.dll?
			JslDeviceID = ~0;
			JslGetConnectedDeviceHandles(&JslDeviceID,1);
			JslDeviceID = ~JslDeviceID;
			n = n;
		}
	}

	if(dx&&ini!=EX::universal_mouse)
	{
		typedef DX::DIJOYSTATE DIJOYSTATE; //DIJOFS_X

		DX::DIPROPRANGE getpropr = 
		{
			{sizeof(getpropr),sizeof(getpropr.diph),DIJOFS_X,DIPH_BYOFFSET}
		};
		
		int compile[DIJOFS_Y-DIJOFS_X==sizeof(LONG)]; //sanity check

		for(int i=0;i<8;i++)
		{
			if(dx->GetProperty(DIPROP_RANGE,&getpropr.diph)==DI_OK)
			{
				assert(getpropr.lMax>1);
				assert(getpropr.lMin==0);
				range[i][0] = EX::debug?getpropr.lMin:0;
				range[i][1] = getpropr.lMax;
			}
			else range[i][0] = range[i][1] = 0;

			getpropr.diph.dwObj+=sizeof(LONG);
		}

		DX::DIDEVCAPS caps = {sizeof(caps)};
	}
		
	EX::INI::Option op;	EX::INI::Detail dt;

	if(in==EX::universal_mouse)
	{
		cfg = 0; //zero out
			
		for(int c=0;c<EX::contexts;c++)
		if(dt->mouse_tilt_left_action[c])
		{				
			cfg.neg_rot[0][c] = dt->mouse_tilt_left_action[c];
		}

		for(int c=0;c<EX::contexts;c++)
		if(dt->mouse_tilt_right_action[c])
		{				
			cfg.pos_rot[0][c] = dt->mouse_tilt_right_action[c];
		}

		if(dt->mouse_button_actions)
		for(int i=0;i<8;i++) for(int c=0;c<EX::contexts;c++)
		{
			cfg.buttons[i][c] = dt->mouse_button_actions[i][c];
		}

		cfg+=EX::Joypad::micecfg;

		if(dt->mouse_menu_button_action)
		{
			for(int c=0;c<EX::contexts;c++)
			{
				cfg.menu[c] = dt->mouse_menu_button_action[c];
			}
		}
		else if(dt->mouse_left_button_action
			 ||dt->mouse_right_button_action)
		{
			for(int c=0;c<EX::contexts;c++)
			{
				cfg.menu[c] = 0; //disable menu
			}
		}
	}
	else if(in)
	{	
		cfg = 0; //zero out
				
		for(int c=0;c<EX::contexts-1;c++) 
		{
			const int *p = in->pseudo_x_axis; //xyz
			const int *r = in->pseudo_x_axis2; //xyz
			
			for(int i=0;i<3;i++) //xyz
			{	
				cfg.pos_pos[i][c] = +EX_INI_BUTTON2MACRO(in->buttons,p[i],c);
				cfg.neg_pos[i][c] = -EX_INI_BUTTON2MACRO(in->buttons,p[i],c);
				cfg.pos_rot[i][c] = +EX_INI_BUTTON2MACRO(in->buttons,r[i],c);
				cfg.neg_rot[i][c] = -EX_INI_BUTTON2MACRO(in->buttons,r[i],c);			
			}

			p = in->pseudo_slider; //1&2

			for(int i=0;i<2;i++) //sliders
			{	
				cfg.pos_aux[i][c] = +EX_INI_BUTTON2MACRO(in->buttons,p[i],c);
				cfg.neg_aux[i][c] = -EX_INI_BUTTON2MACRO(in->buttons,p[i],c);
			}	

			p = in->pseudo_pov_hat_0;

			for(int i=0;i<8;i++) //pov hat primary directions			
			{	
				cfg.pov_hat[i][c] = +EX_INI_BUTTON2MACRO(in->buttons,p[i],c);
			}

			for(int i=0;i<32;i++) //buttons
			{
				cfg.buttons[i][c] = EX_INI_BUTTON2MACRO(in->buttons,i+1,c);
			}
		}

		cfg+=EX::Joypad::padscfg;
	}
	else //manual config
	{
		ini = Ex_input_default_joypad; 		
	}		

	return active = true;
}									 

	//2021: whether this is good or not som_mocap
	//is now rotating the pedals when turning so
	//it feels better when drifting after circling
	//Ex_input_ratchet SEEMS TO CAUSE THIS TO BE
	//BUMPY. IT MIGHT MIGHT NOT IF THE springstep
	//SUBROUTINE ALGORITHM IS IMPROVED
	enum{ Ex_input_ratchet_pedals=0 };
static float Ex_input_ratchet(int pos, float out, const float cal[7], float half=0)
{
	//30: increasing for do_u2
	const float c = 0.30f; //30% //20%

	//REMINDER: -1 is because the 0 gait is
	//not in cal... not because it's 1 lower
	int g = EX::Joypad::gaitcode(pos)-1;

	if(Ex_input_ratchet_pedals) //DISABLING
	{
		assert(0);

		if(g==0&&!half) g = -6; //2021: pedals gap?
	}

	if(g>=0&&out<cal[g]) //gaits 1~7
	{
		float h = g<6?cal[g+1]:1; //after g
		
		if(cal[g]-out<c*(h-cal[g])) out = cal[g]; 		
	}
	else if(g<=-6&&out>=0.0625f) //lower ranges
	{
		if(half) //2018: c*(hi-lo) is too small
		{
			if(out<half&&half-out<c*half) 
			return half;
		}
		else if(Ex_input_ratchet_pedals) //pedals
		{
			//2021: I think this must be backward
			//(I'm watching it fighting)

			/*2021: this looks like the code above?
			//REMINDER: g IS NEGATIVE (i.e -2=-3+1)
			float lo = -1.0f/++g, hi = -1.0f/++g;*/
			float lo = -1.0f/g, hi = -1.0f/(g+1);
			assert(hi>lo);
			if(out<lo&&lo-out<c*(hi-lo)) out = lo;
		}
	}

	return out;
}

static unsigned Ex_input_triggers2[2];
static EX::Joypad *Ex_input_triggers[2];
int EX::Joypad::trigger_full_release(int t)
{
	if(t<0||t>1) return 100000;
	unsigned &t2 = Ex_input_triggers2[t];
	return t2?EX::tick()-t2:0;
}

bool EX::Joypad::simulate(unsigned char *in, size_t sz) 
{	
	if(!Ex_input_initialize()||!active||!dx) 
	{
		for(int i=2;i-->0;)
		if(this==Ex_input_triggers[i])
		{
			Ex_input_triggers[i] = 0;
			Ex_input_triggers2[i] = 0;
		}

		return false;
	}
		
	int c = EX::context(); bool captured = true; 
		
	//hack: som_state_thumbs_boost increases
	//the position, so in order to compensate
	//it can remove the outer deadzone like so
	bool boost = false;

	if(ini==EX::universal_mouse)
	{
		const float *calibration = ini->axis_analog_gaits;
				
		HWND cap = GetCapture(); captured = true;
				
		captured = cap&&cap==EX::client||cap&&cap==EX::window;

		RECT client; if(!GetClientRect(EX::client,&client)) return false;

		if(GetActiveWindow()!=cap) captured = false; //NEW (debug startup)
				
		static bool wascaptured = false; //hack

		dx->Acquire(); dx7->Poll(); DX::DIMOUSESTATE2 st;

		//NOTE: lX/lY REPORT NO MOVEMENT UNTIL THE WINDOW
		//IS CLICKED ON... BUT IT WORKS FOR A WHILE AT 
		//START UP, BUT I CAN'T FIGURE OUT WHY IT'S REFUSING
		//TO COOPERATE... USUAL Microsoft FIDDLING NO DOUBT
		//(access is DISCL_NONEXCLUSIVE|DISCL_BACKGROUND)
		HRESULT ok = dx->GetDeviceState(sizeof(st),&st);
				
		//hack: quick fix
		if(DDRAW::isPaused) wascaptured = false; 

		DWORD now = EX::tick();

		static DWORD timeout = now; 

		if(captured&&!wascaptured) //hack
		{
			timeout = now;

			memset(absolute,0x00,sizeof(absolute));

			//the relative mouse data for this round 
			//maybe be skewed by the cursor capture??
			wascaptured = captured; captured = false;

			wm_input[0] = INT_MAX;
		}
		else if(wascaptured=captured)
		{
			if(ok!=DI_OK) return false;

			//WM_INPUT?
			//Note: DirectInput can't improve on this
			//https://docs.microsoft.com/en-us/windows/win32/dxtecharts/taking-advantage-of-high-dpi-mouse-movement
			range[0][1] = client.right;
			range[1][1] = client.bottom;
	//		range[2][1] = 10000; //wheel

			EX::INI::Detail dt;
						
			float sa = dt->mouse_saturate_multiplier;
			float dz = dt->mouse_deadzone_multiplier*0.5f; 

			bool raw,dead = true; //NEW: timeout

			if(raw=*wm_input!=INT_MAX)
			{
				st.lX = wm_input[0]; wm_input[0] = 0;
				st.lY = wm_input[1]; wm_input[1] = 0;
			}

			LONG *p = &st.lX; float *q = absolute;
						
			for(int i=0;i<2;i++) 
			{		
				float rel = p[i];

				rel/=sa*range[i][1]*0.5f;

				bool am = cfg.analog_mode[i][c]>0;

				//2020: preventing use in menus?!
				//(because not set in context #1)
				//if(am)
				{
					if(fabsf(q[i]+=rel)>1.0f)
					{
						q[i] = q[i]<0?-1.0f:1.0f;
					}
				}
				/*else if(rel) //2020 (not working???)
				{
					q[i] = rel<0?-1.0f:1.0f; 
				}
				else q[i] = 0; //2020 (ditto)*/
				
				float spin = max(0,fabsf(q[i])-dz);

				//if(am) 
				{
					if(spin)
					{
						float v = dz?spin/dz:spin*spin; //zero divide //2024

						if(q[i]<0)
						{
							position[i] = -v-calibration[0]; 
						}
						else position[i] = v+calibration[0]; 
					}
					else position[i] = 0.0f;					
				}
				//else position[i] = q[i]; //2020 (ditto)
								
				//if(am) //2020
				{
					//free look
					if(fabsf(p[i])>cfg.deadzone[i][c]) 
					{					
						if(dz<1) //zero divide
						{
							float t = 1.0f-spin/(1.0f-dz); //lerp

							//black magic: just seems to work
							position[i]+=p[i]*calibration[0]*t; 
						}
						else //2024
						{
							float v = p[i]*calibration[0];

							//position[i]+=_copysign(powf(v,2),v); 
							position[i]+=v; 
						}
					}
				}

				if(p[0]||p[1]) //function overlay?
				{
					EX::mouse[i] = q[i]*range[i][1]/2;
				}
				
				if(spin&&am) dead = false;
			}

			//n: mouse_sample_limit
			int n = raw?2:3; if(n>1)
			{
				assert(n<=Joypad::mice_smoothing_max);

				//Reminder: this is here because
				//a query will return no movement
				//if the device(s) refresh rate is
				//slower than the calls to simulate

				assert(EX_MICE==1); //unimplemented

				//todo: factor in refresh rates
				int s = DINPUT::noPolls%n; 

				smoothing[0][s] = position[0]; 
				smoothing[1][s] = position[1]; 
				
				//over representing current sample?
				//position[0] = 0; position[1] = 0;
				int m = n+1;

				for(int i=0;i<n;i++)
				{
					position[0]+=smoothing[0][i];
					position[1]+=smoothing[1][i];
				}

				position[0]/=m; position[1]/=m;
			}

			if(!dead||p[0]||p[1]) timeout = now;

			DWORD ms = dt->mouse_deadzone_ms_timeout; 

			if(dead&&now-timeout>(ms?ms:250)) 
			{
				absolute[0] = absolute[1] = 0;
			}

			position[0]*=dt->mouse_horizontal_multiplier;
			position[1]*=dt->mouse_vertical_multiplier;

			for(int i=0;i<8;i++) if(st.rgbButtons[i]&0x80)
			{
				if(!buttons[i]){ buttons[i] = 0x80; contexts[i] = c; }
			}
			else{ buttons[i] = 0; contexts[i] = c; } 

			//TODO: SOM may poll many times per frame
			if(buttons[0]&&buttons[1]) menu[c]++; else menu[c] = 0; 

			position[3] = EX::tilt(); //hack??
		}
	}
	else 
	{
		HRESULT ok1 = dx->Acquire(); 
		
		HRESULT ok2 = dx7->Poll(); DX::DIJOYSTATE st;

		HRESULT ok3 = dx->GetDeviceState(sizeof(st),&st);

		if(ok3!=DI_OK) return false;	
			
		bool trigger_device = false;

		if(trigger_device=isXInputDevice||isDualShock)
		{
			//2018: too much for DualShock4
			//const int extra_2017 = 7500; //extension?
			const int extra_2017 = 4000; //extension?
			//2020: DS4 is extremely noisy
			//100 only worked because bugs
			const int extra_2020 = 1500; //100 //extension?

			LONG lz = st.lZ; int t2[2];

			if(isXInputDevice) //NEW: not loading XInput APIs for now
			{	
				st.lZ = st.lRx; st.lRx = st.lRy; 

				t2[0] = max(0,lz-32767);
				t2[1] = max(0,32767-lz);
				
				//TODO: USE IN PLACE OF dx->GetDeviceState 
				//2020: DirectInput has a known problem of making 
				//the triggers mutually exclusive for some reason
				XINPUT_STATE xs;
				if(Ex_input_XInputGetState)
				if(!Ex_input_XInputGetState(~isXInputDevice,&xs))
				{
					//TODO? try to determine device state matches
					XINPUT_GAMEPAD &gp = xs.Gamepad;

					//range is 0 to 255 (256*128==32767)
					t2[0] = gp.bLeftTrigger*128; 
					t2[1] = gp.bRightTrigger*128;

					//2021: some reports suggest the DirectInput buttons
					//may not be consistent across devices, but it's not
					//clear if those are DI drivers/devices or XInput...
					//maybe (surely) the XInput ones are consistent?
					st.rgbButtons[0] = gp.wButtons&0x1000?0x80:0; //cross
					st.rgbButtons[1] = gp.wButtons&0x2000?0x80:0; //circle
					st.rgbButtons[2] = gp.wButtons&0x4000?0x80:0; //square
					st.rgbButtons[3] = gp.wButtons&0x8000?0x80:0; //triangle
					st.rgbButtons[8] = gp.wButtons&16?0x80:0; //start //0x10 //4
					st.rgbButtons[9] = gp.wButtons&32?0x80:0; //select //0x20 //5
					st.rgbButtons[10] = gp.wButtons&64?0x80:0; //L3 //0x40 //6
					st.rgbButtons[11] = gp.wButtons&128?0x80:0; //R3 //0x80 //7
					st.rgbButtons[4] = gp.wButtons&256?0x80:0; //L1 //0x100 //8
					st.rgbButtons[5] = gp.wButtons&512?0x80:0; //R1 //0x200 //9

					switch(gp.wButtons&0xf)
					{
					default: *st.rgdwPOV = 0; break;
					case 0: *st.rgdwPOV = 0xffff; break; //not sure?
					case 1: *st.rgdwPOV = 0; break; //up dpad
					case 2: *st.rgdwPOV = 18000; break; //down dpad
					case 6: *st.rgdwPOV = 22500; break; //down+left
					case 4: *st.rgdwPOV = 27000; break; //left dpad
					case 5: *st.rgdwPOV = 31500; break; //up+left
					case 8: *st.rgdwPOV = 9000; break; //right dpad
					case 9: *st.rgdwPOV = 4500; break; //up+right
					case 10: *st.rgdwPOV = 13500; break;  //down+right
					}
				}
				else //old way?
				{
					assert(0);

					//2018: allow [Button] assignments??
					st.rgbButtons[10] = st.rgbButtons[8];
					st.rgbButtons[11] = st.rgbButtons[9];
					st.rgbButtons[8] = st.rgbButtons[7];
					st.rgbButtons[9] = st.rgbButtons[6];
				}
			}
			else if(isDualShock)
			{	
				//make X and O first two buttons
				std::swap(st.rgbButtons[0],st.rgbButtons[2]);
				//swap X and O buttons??
				std::swap(st.rgbButtons[0],st.rgbButtons[1]);

				//Start/Select are reversed??
				std::swap(st.rgbButtons[8],st.rgbButtons[9]);
				//Reminder: 6/7 report (arbitrary) button equivalents
				//
				// 2022: I'm seeing these spaz out for a split-second
				// when the app starts up. triggers are canceling the
				// opening movie
				// 
				// all axes are set to 0x7fff, where triggers should
				// report 0 (DualSense)
				//
				int i; for(i=6;i-->0;) if(0x7fff!=(&st.lX)[i])
				{
					//I don't know what else to do... maybe ignore a
					//few milliseconds?

					t2[0] = st.lRx/2;
					t2[1] = st.lRy/2; break;
				}
				if(i==0) //DEBUGGING
				{
					//I think maybe this is returned when
					//the window doesn't have focus or if
					//movie is playing??
					static int once = 0; assert(once++==0);

					i = i; //breakpoint
				}
				
				st.lRx = st.lRz; st.lRz = 32767;

				//EX::dbgmsg("DS4 %d %d",st.lX,st.lY);
			}
			st.lRy = 32767;

			for(int i=0;i<2;i++) //NEW: trigger release logic
			{
				//trying full_release is all the way up
				if(Ex_input_triggers[i]==this)				
				if(!t2[i]&&!Ex_input_triggers2[i])
				{
					Ex_input_triggers2[i] = EX::tick();
				}

				st.rgbButtons[6+i] = 0; 
				//if(t2[i]<extra_2017)
				if(t2[i]<extra_2017-(triggers[i]>0?extra_2020:0))
				{
					triggers[i] = 0;
				}
				else if(triggers[i]>=0) 
				{
					st.rgbButtons[6+i] = 0x80;					
					triggers[i] = max(t2[i]-extra_2020,triggers[i]);
					if(t2[i]<triggers[i])
					{
						st.rgbButtons[6+i] = 0;

						Ex_input_triggers[i] = this;
						Ex_input_triggers2[i] = 0;

						triggers[i] = min(t2[i]+extra_2020,32767-extra_2020);

						triggers[i] = -triggers[i]; //HACK
					}					
				}
				else if(1) if(t2[i]>-triggers[i]) //2020
				{
					st.rgbButtons[6+i] = 0x80;

					triggers[i] = t2[i]-extra_2020;
				}
			}
		}

		static DX::DIJOYSTATE cmp; //keep alive

		if(memcmp(&st,&cmp,sizeof(cmp)))
		{
			SetThreadExecutionState(ES_DISPLAY_REQUIRED);		

			memcpy(&cmp,&st,sizeof(cmp));
		}

		LONG *p = &st.lX; //lX lY lZ lRx lRy lRz rglSlider[2]

		float *q = position;
	 
		int i; for(i=0;i<8;i++)
		{	
			float a = q[i]; //TESTING
			
			if(!range[i][1]) 
			{
				q[i] = 0; continue;
			}																		  
			else q[i] = p[i]; 

			if(range[i][0]!=0)
			{
				//not entirely positive how to be interperet this
			
				assert(0); //unimplemented
			}
			else q[i]-=range[i][1]*0.5f;

			q[i]/=(range[i][1]-range[i][0])*0.5f;

			if(0)
			{
				//2020: should there be a deadzone extension like 
				//with do_mouse? what about asymmetry? calibration
				//via Control Panel doesn't seem to help with that
				q[i]*=1.1f; 
				if(q[i]>0) q[i] = max(0,q[i]-0.1f);
				if(q[i]<0) q[i] = min(0,q[i]+0.1f);
			}
		}
		
		//SomEx.dll uses this to rotate the thumbpads		
		if(thumbs) boost = thumbs(this-EX::Joypads,q);

		int hat = int(ini->pov_hat_to_use_for_play)-1; 
		
		for(i=0;i<4;i++) //digital only for now
		{
			povhat = st.rgdwPOV[min(max(hat,0),3)];

			if(povhat&0xFFFF<65535&&povhat%4500) 
			{						
				povhat+=2250; povhat/=4500; povhat*=4500;
			}
		}

		for(i=0;i<32;i++) if(st.rgbButtons[i]&0x80)
		{
			if(!buttons[i])
			{
				buttons[i] = 0x80; contexts[i] = c; 
			}
			else if(contexts[i]!=c) //NEW
			{
				//2020: SEEMS TO NOT BE WORKING?

				//2017: prevent cycling contexts when buttons
				//are held down. E.g. 0->1->0->1
				contexts[i] = -1;
			}
		}
		else{ buttons[i] = 0; contexts[i] = c; }

		//hack: try to make it possible to detect
		//if a button relates to a trigger device
		if(trigger_device)
		{
			//these are special trigger codes that
			//would map to 1/61 & 1/63 lower gaits
			if(0x80==buttons[6]) buttons[6] = 0xFC;
			if(0x80==buttons[7]) buttons[7] = 0xFE;
		}
	}

	int i, x_completed = 0;
	for(i=0;i<MAX_MACROS&&macros[i];i++)
	{		
		macros[i-x_completed] = macros[i];

		macro_runaways[i-x_completed] = macro_runaways[i];

		const unsigned char* &p = macros[i-x_completed];

		int &r = macro_runaways[i-x_completed];
		int &c = macro_counters[i-x_completed];

		const unsigned char *q = p; //rewind

		if(!c) c = *p; assert(c);

		if(*p) for(p++,r++,c--;*p;p++)
		{			
			if(*p<sz) in[*p]|=0x80;
			
			if(r++>EX_MACRO_RUNAWAY_ALERT_ON)
			if(!EX::issue_macro_runaway_warning()) 
			goto err1; //player opted to kill macro
		}		

		if(*p) //macro appears to be bad 
		{
err1:		assert(0); //TODO: handle gracefully??
				
			x_completed++; //just toss it 

			p = 0; c = 0; continue;
		}
		else p = c?q:p+1;
		
		if(!*p) 
		{		
			r = 0; p = 0; c = 0;

			x_completed++;
		}
	}

	int x_remaining = i-x_completed, x_appended = 0;

	const unsigned char **x_new = &macros[x_remaining];

	int *x_new_runaways = &macro_runaways[x_remaining];
	int *x_new_counters = &macro_counters[x_remaining];

	int _x80[8] = {0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80}; 

	bool lo = false; //NEW
	for(int i=0,n;i<8;i++)
	{		
		const float *cal = ini->analog_gaits(i);
		float dil[7];
		if(1!=analog_dilation) //TESTING
		{
			for(int i=7;i-->0;)
			dil[i] = pow(cal[i],analog_dilation);
			cal = dil;
		}

		if(!range[i][1])
		{
			analog[i] = 0;
		}
		else //if(cfg.analog_mode[i][c])
		{	
			float f = fabsf(position[i]);		 
								  		
			//NEW: anti-twitch logic
			f = Ex_input_ratchet(analog[i],f,cal,cal[0]/2);

			analog[i] = 0;

			if(f>=(boost?1:cal[6])) 
			{
				analog[i] = 0x7F; //1111111
			}
			else if(f>=cal[3]) //0.66666
			{
				if(f>=cal[5]) n = 7; //7/8 (+.125)			
				else if(f>=cal[4]) n = 4; //3/4 (+.09)
				else n = 3; //2/3 (+.16)

				if(analog_clock%n==0) _x80[i] = 0; 

				switch(n)
				{
				case 7: analog[i] = 0x3F; break; //111111
				case 4: analog[i] = 0x1F; break; //11111
				case 3: analog[i] = 0x0F; break; //1111

				default: assert(0);
				}
			}
			else if(f>=cal[0]) //0.2
			{						
				if(f>=cal[2]) n = 2; //1/2 (+.17)
				else if(f>=cal[1]) n = 3; //1/3 (+.13)
				else n = 5; //1/5  

				if(analog_clock%n) _x80[i] = 0; 

				switch(n)
				{
				case 2: analog[i] = 0x7; break; //111
				case 3: analog[i] = 0x3; break; //11
				case 5: analog[i] = 0x1; break; //1

				default: assert(0);
				}
			}
			else if(f>=cal[0]/2) //NEW: 0.1
			{	
				//THIS IS HARD TO DESCRIBE, BUT
				//the first gait can only go in
				//8 directions. tentatively let 
				//it go in 16 directions. later
				//on down this may be retracted
				lo = true; n = 10;
				if(analog_clock%n) _x80[i] = 0; 
				analog[i] = n<<1; //low range
			}
		}/*
		else //analog[i] = 0; //2020  
		{
			analog[i] = position[i]?0x80:0; //2020
		}*/
		
		if(!analog[i]) _x80[i] = 0; //dilation?
	}
	if(lo) //carry on?
	{
		for(int i=0;i<8;i++)
		if(analog[i]>1&&analog[i]&1) 
		{
			lo = false; break;
		}if(lo)
		{	lo = false;
			for(int i=0;i<8;i++)
			if(analog[i]&1)
			{
				lo = true; break;
			}
		}if(!lo) for(int i=0;i<8;i++)		
		if(~analog[i]&1) analog[i] = _x80[i] = 0;
	}

	if(captured)
	{
		#define	DO(test,f,_x80,...) if(test)\
		{\
			__VA_ARGS__ unsigned x = cfg.f[c];\
		\
			if(x!=EX_INI_BAD_MACRO) if(x>255)\
			{\
				if(x_remaining+x_appended<MAX_MACROS-1)\
				x_new[x_appended++] = EX::INI::Action()->xtranslate(x);\
			}\
			else if(x) if((_x80)==0x80||in[x]==0x80)\
			{\
				in[x] = 0x80; /*NEW: digital overpowers*/\
			}\
			else /*in[x]|=_x80;*/\
			{\
				in[x] = EX::Joypad::analog_select(in[x],_x80);\
			}\
		}
				
		for(int i=0;i<3;i++) //NEW: dividing by 2 to account for "lo"
		{	
			DO(position[i+0]>=+ini->axis_analog_gaits[0]/2,pos_pos[i],_x80[i+0]|analog[i+0])	
			DO(position[i+0]<=-ini->axis_analog_gaits[0]/2,neg_pos[i],_x80[i+0]|analog[i+0])
			DO(position[i+3]>=+ini->axis2_analog_gaits[0]/2,pos_rot[i],_x80[i+3]|analog[i+3])	
			DO(position[i+3]<=-ini->axis2_analog_gaits[0]/2,neg_rot[i],_x80[i+3]|analog[i+3])			
		}
		for(int i=0;i<2;i++) 
		{
			DO(position[i+6]>=+ini->slider_analog_gaits[0]/2,pos_aux[i],_x80[i+6]|analog[i+6])	
			DO(position[i+6]<=-ini->slider2_analog_gaits[0]/2,neg_aux[i],_x80[i+6]|analog[i+6])
		}	

		DO(povhat==0,pov_hat[0],0x80)
		DO(povhat==9000,pov_hat[1],0x80)
		DO(povhat==18000,pov_hat[2],0x80)
		DO(povhat==27000,pov_hat[3],0x80)

		//should be mapped out during configuration!
		if(!ini->do_not_associate_pov_hat_diagonals)
		{
			DO(povhat==4500,pov_hat[0],0x80)
			DO(povhat==4500,pov_hat[1],0x80)
			DO(povhat==13500,pov_hat[1],0x80)
			DO(povhat==13500,pov_hat[2],0x80)
			DO(povhat==22500,pov_hat[2],0x80)
			DO(povhat==22500,pov_hat[3],0x80)
			DO(povhat==31500,pov_hat[3],0x80)
			DO(povhat==31500,pov_hat[0],0x80)
		}
		else
		{					
			DO(povhat==4500,pov_hat[4],0x80)
			DO(povhat==13500,pov_hat[5],0x80)
			DO(povhat==22500,pov_hat[6],0x80)
			DO(povhat==31500,pov_hat[7],0x80)
		}			

		for(int i=0;i<32;i++)
		if(buttons[i]<0xFC)
		{
			DO(buttons[i]&&contexts[i]==c,buttons[i],0x80,trigger_macro:)
		}
		else if(contexts[i]==c) //triggers?
		{
			int x = cfg.buttons[i][c]; if(x>=256) goto trigger_macro;
			if(x) in[x] = buttons[i];
		}

		DO(menu[c]>8,menu,0x80) 
		
#undef	DO //DO(test,f,a)
		
	}

	x_completed = 0;

	for(i=0;i<x_appended&&x_new[i];i++)
	{		
		x_new[i-x_completed] = x_new[i];

		x_new_runaways[i-x_completed] = x_new_runaways[i];

		const unsigned char* &p = x_new[i-x_completed];

		int &r = x_new_runaways[i-x_completed];
		int &c = x_new_counters[i-x_completed];

		const unsigned char *q = p; //rewind

		if(!c) c = *p; assert(c);

		if(*p) for(p++,r++,c--;*p;p++)
		{			
			if(*p<sz) in[*p]|=0x80;
			
			if(r++>EX_MACRO_RUNAWAY_ALERT_ON) 
			if(!EX::issue_macro_runaway_warning()) 
			goto err2; //player opted to kill macro
		}		

		if(*p) //macro appears to be bad 
		{
err2:		assert(0); //TODO: handle gracefully??
				
			x_completed++; //just toss it 

			p = 0; c = 0; continue;
		}
		else p = c?q:p+1;
		
		if(!*p) 
		{		
			r = 0; p = 0; c = 0;

			x_completed++;
		}
	}

	return true;
}

const float *EX::Pedals::calibration()
{
	static const float cal[7] = 
	{
	EX::Joypad::analog_scale(0x01),	
	EX::Joypad::analog_scale(0x03),
	EX::Joypad::analog_scale(0x07),	
	EX::Joypad::analog_scale(0x0f),
	EX::Joypad::analog_scale(0x1f),	
	EX::Joypad::analog_scale(0x3f), 0.95f //1.0f
	};
	return cal;
}
const float EX::Pedals::analog_least = 1.0f/12; //24
int EX::Pedals::byteencode_scale(float f, int *_x80)
{
	const float *cal = calibration();

	int n,o = 0;

	if(f>=cal[6]) 
	{
		o = 0x7F; //1111111
	}
	else if(f>=cal[3]) //0.66666
	{
		if(f>=cal[5]) n = 7; //7/8 (+.125)
		else if(f>=cal[4]) n = 4; //3/4 (+.09)
		else n = 3; //2/3 (+.16)

		if(_x80)
		if(analog_clock%n==0) *_x80 = 0; 

		switch(n)
		{
		case 7: o = 0x3F; break; //111111
		case 4: o = 0x1F; break; //11111
		case 3: o = 0x0F; break; //1111

		default: assert(0);
		}
	}
	else if(f>=cal[0]) //0.2
	{						
		if(f>=cal[2]) n = 2; //1/2 (+.17)
		else if(f>=cal[1]) n = 3; //1/3 (+.13)
		else n = 5; //1/5  

		if(_x80)
		if(analog_clock%n) *_x80 = 0; 

		switch(n)
		{
		case 2: o = 0x7; break; //111
		case 3: o = 0x3; break; //11
		case 5: o = 0x1; break; //1

		default: assert(0);
		}
	}
	else if(f>analog_least) //low ranges
	{	
		if(f<1.0f/8) 
		{
			if(f<1.0f/12) 
			{							
				if(f<1.0f/16) 
				{
					if(f<1.0f/20) 
					{
						if(f<1.0f/24) 
						{
							n = f<1.0f/25?25:24;
						}
						else if(f<1.0f/22) 
						{
							n = f<1.0f/23?23:22;
						}
						else n = f<1.0f/21?21:20;
					}
					else if(f<1.0f/18) 
					{
						n = f<1.0f/19?19:20;
					}
					else n = f<1.0f/17?17:16;
				}
				else if(f<1.0f/14) 
				{
					n = f<1.0f/15?15:14;
				}
				else n = f<1.0f/13?13:12;
			}
			else if(f<1.0f/10) 
			{
				n = f<1.0f/11?11:10;
			}
			else n = f<1.0f/9?9:8;
		}
		else n = f<1.0f/7?7:6;

		if(_x80)
		if(analog_clock%n) *_x80 = 0; 

		o = n<<1;
	}
	else if(_x80) *_x80 = 0; return o;
}
bool EX::Pedals::simulate(unsigned char *in, size_t sz, float step, float mod)
{	
	int c = EX::context();
	
	if(c!=0||!active) return false; 
	
	step = max(1-step,0); //brain dead relaxation

	const float *cal = calibration();

	int _x80[8] = {0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80}; 
		
	static const int x = 5;	

	float reposition[x]; for(int i=0;i<x;i++) 
	{
		//if(fabsf(position[i])>analog_lowest) 
		{
			reposition[i] = position[i]*=step;
			//reposition[i] = position[i]; position[i]*=step;
		}
		//else reposition[i] = position[i] = 0;

		if(mod==0) return true;
		if(mod!=1) reposition[i]*=mod;
	}

	Somvector::map(reposition).rotate<3>(correction);
	
	for(int i=0;i<x;i++)
	if(cfg.analog_mode[i][c]) 
	{	
		float f = fabsf(reposition[i]);

		//NEW: anti-twitch logic
		if(Ex_input_ratchet_pedals)
		f = Ex_input_ratchet(analog[i],f,cal);

		analog[i] = byteencode_scale(f,_x80+i);
	}	
	else analog[i] = 0;
	
	#define	DO(test,f,_x80) if(test)\
	{\
		unsigned x = cfg.f[c];\
		/*hack: account for lower ranges*/\
		if(x<sz&&EX::Joypad::analog_scale(in[x])\
		<EX::Joypad::analog_scale(_x80)) in[x] = _x80;\
	}
		
	for(int i=0;i<3;i++) 
	{	
		DO(reposition[i+0]>analog_least,pos_pos[i],_x80[i+0]|analog[i+0])	
		DO(reposition[i+0]<-analog_least,neg_pos[i],_x80[i+0]|analog[i+0])
	}
	for(int i=0;i<2;i++) 
	{	
		DO(reposition[i+3]>analog_least,pos_rot[i],_x80[i+3]|analog[i+3])	
		DO(reposition[i+3]<-analog_least,neg_rot[i],_x80[i+3]|analog[i+3])
	}
	#undef DO //DO(test,f,a)
	#undef LO

	return true;
}

/*GOT TO HAND IT TO MICROSOFT, THIS CODE/API IS HOT GARBAGE
//Based on:
//http://msdn.microsoft.com/en-us/library/windows/desktop/ee417014%28v=vs.85%29.aspx
//-----------------------------------------------------------------------------
// Enum each PNP device using WMI and check each device ID to see if it contains 
// "IG_" (ex. "VID_045E&PID_028E&IG_00").  If it does, then it's an XInput device
// Unfortunately this information can not be found by just using DirectInput 
//-----------------------------------------------------------------------------
//BOOL IsXInputDevice( const GUID* pGuidProductFromDirectInput )
void EX::Joypad::detectXInputDevices()// const GUID* pGuidProductFromDirectInput )
{
	for(int i=0;i<EX_JOYPADS;i++) EX::Joypads[i].isXInputDevice = false;

    IWbemLocator*           pIWbemLocator  = NULL;
    IEnumWbemClassObject*   pEnumDevices   = NULL;
    IWbemClassObject*       pDevices[20]   = {0};
    IWbemServices*          pIWbemServices = NULL;
    BSTR                    bstrNamespace  = NULL;
    BSTR                    bstrDeviceID   = NULL;
    BSTR                    bstrClassName  = NULL;
    DWORD                   uReturned      = 0;
//    bool                    bIsXinputDevice= false;
    UINT                    iDevice        = 0;
    VARIANT                 var;
    HRESULT                 hr;

    // CoInit if needed
    hr = CoInitialize(NULL);
    bool bCleanupCOM = SUCCEEDED(hr);

    // Create WMI
    hr = CoCreateInstance( __uuidof(WbemLocator),
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           __uuidof(IWbemLocator),
                           (LPVOID*) &pIWbemLocator);
    if( FAILED(hr) || pIWbemLocator == NULL )
        goto LCleanup;

    bstrNamespace = SysAllocString( L"\\\\.\\root\\cimv2" );
    bstrClassName = SysAllocString( L"Win32_PNPEntity" );  
    bstrDeviceID  = SysAllocString( L"DeviceID" );          
    
    // Connect to WMI 
    hr = pIWbemLocator->ConnectServer( bstrNamespace, NULL, NULL, 0L, 
                                       0L, NULL, NULL, &pIWbemServices );
    if( FAILED(hr) || pIWbemServices == NULL )
        goto LCleanup;
		
		//WHY IS THIS NECESSARY?
	// Switch security level to IMPERSONATE. 
	CoSetProxyBlanket( pIWbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, 
                     RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE );                    

		//TODO: need to alert users when this is taking too long
		//this routine is now called by som_hacks_thread_main to
		//not block at the top
		int time = EX::tick();
		int help = WBEM_FLAG_FORWARD_ONLY|WBEM_FLAG_RETURN_IMMEDIATELY;
		help|=WBEM_FLAG_SHALLOW;
    hr = pIWbemServices->CreateInstanceEnum( bstrClassName,help, NULL, &pEnumDevices ); 
		assert(!hr);
		time = EX::tick()-time;
		assert(time<500); //250 //50
    if( FAILED(hr) || pEnumDevices == NULL )
        goto LCleanup;

    // Loop over all devices
    for( ;; )
    {
			//TODO: I think maybe 10000 is stalling on my system :(
			time = EX::tick();
        // Get 20 at a time				
        hr = pEnumDevices->Next(10000, 20, pDevices, &uReturned );
			assert(!FAILED(hr)); //assert(!hr);
			time = EX::tick()-time;
			assert(time<500); //250 //50
        if( FAILED(hr) )
		{
			assert(0);
            goto LCleanup;
		}
        if( uReturned == 0 )
            break;

        for( iDevice=0; iDevice<uReturned; iDevice++ )
        {
            // For each device, get its device ID
            hr = pDevices[iDevice]->Get( bstrDeviceID, 0L, &var, NULL, NULL );
            if( SUCCEEDED( hr ) && var.vt == VT_BSTR && var.bstrVal != NULL )
            {
                // Check if the device ID contains "IG_".  If it does, then it's an XInput device
				    // This information can not be found from DirectInput 
                if( wcsstr( var.bstrVal, L"IG_" ) )
                {
                    // If it does, then get the VID/PID from var.bstrVal
                    DWORD dwPid = 0, dwVid = 0;
                    WCHAR* strVid = wcsstr( var.bstrVal, L"VID_" );
                    if( strVid && swscanf( strVid, L"VID_%4X", &dwVid ) != 1 )
                        dwVid = 0;
                    WCHAR* strPid = wcsstr( var.bstrVal, L"PID_" );
                    if( strPid && swscanf( strPid, L"PID_%4X", &dwPid ) != 1 )
                        dwPid = 0;

                    // Compare the VID/PID to the DInput device
                    DWORD dwVidPid = MAKELONG( dwVid, dwPid );
                    //if(dwVidPid == pGuidProductFromDirectInput->Data1 )
                    //{
                    //    bIsXinputDevice = true;
					//	goto LCleanup;
                    //}
					for(int i=0;i<EX_JOYPADS;i++) 
					if(dwVidPid==EX::Joypads[i].product.Data1)
					EX::Joypads[i].isXInputDevice = true;
                }
            }   
            SAFE_RELEASE( pDevices[iDevice] );
        }
    }

LCleanup:
        SysFreeString(bstrNamespace);
        SysFreeString(bstrDeviceID);
        SysFreeString(bstrClassName);
    for( iDevice=0; iDevice<20; iDevice++ )
        SAFE_RELEASE( pDevices[iDevice] );
    SAFE_RELEASE( pEnumDevices );
    SAFE_RELEASE( pIWbemLocator );
    SAFE_RELEASE( pIWbemServices );

   if( bCleanupCOM )
        CoUninitialize();

    //return bIsXinputDevice;
}*/
//code adapted from SDL_dxjoystick.c
//http://lists.libsdl.org/pipermail/commits-libsdl.org/2013-August/007312.html
//http://hg.libsdl.org/SDL/file/8cc29a668223/src/joystick/windows/SDL_dxjoystick.c
//DEFINE_GUID(IID_ValveStreamingGamepad,  MAKELONG( 0x28DE, 0x11FF ),0x0000,0x0000,0x00,0x00,0x50,0x49,0x44,0x56,0x49,0x44);
//DEFINE_GUID(IID_X360WiredGamepad,  MAKELONG( 0x045E, 0x02A1 ),0x0000,0x0000,0x00,0x00,0x50,0x49,0x44,0x56,0x49,0x44);
//DEFINE_GUID(IID_X360WirelessGamepad,  MAKELONG( 0x045E, 0x028E ),0x0000,0x0000,0x00,0x00,0x50,0x49,0x44,0x56,0x49,0x44);
//static SDL_bool SDL_IsXInputDevice(const GUID* pGuidProductFromDirectInput)
void EX::Joypad::detectXInputDevices()
{
	UINT i,j;
	for(j=0;j<EX_JOYPADS;j++) EX::Joypads[j].isXInputDevice = false;
		
    const DWORD s_XInputProductGUID[] = //static
	{
		MAKELONG( 0x28DE, 0x11FF ), //IID_ValveStreamingGamepad
		MAKELONG( 0x045E, 0x02A1 ), //IID_X360WiredGamepad /* Microsoft's wired X360 controller for Windows. */
		//matches ScpBus
		MAKELONG( 0x045E, 0x028E ), //IID_X360WirelessGamepad /* Microsoft's wireless X360 controller for Windows. */
	};
    /* Check for well known XInput device GUIDs */
    /* This lets us skip RAWINPUT for popular devices. Also, we need to do this for the Valve Streaming Gamepad because it's virtualized and doesn't show up in the device list. */
    for(i=0;i<EX_ARRAYSIZEOF(s_XInputProductGUID);i++)
	{
		//if(!memcmp(pGuidProductFromDirectInput,s_XInputProductGUID[i],sizeof(GUID))) 
		for(j=0;j<EX_JOYPADS;j++) if(EX::Joypads[j].product.Data1==s_XInputProductGUID[i])
		EX::Joypads[j].isXInputDevice = true;
	}

	std::vector<RAWINPUTDEVICELIST> SDL_RawDevList;

    /* Go through RAWINPUT (WinXP and later) to find HID devices. */
    /* Cache this if we end up using it. */
    for(UINT n=i=0;i<2;i++)
	{
		GetRawInputDeviceList(n?&SDL_RawDevList[0]:0,&n,sizeof(RAWINPUTDEVICELIST));
		SDL_RawDevList.resize(n);
    }

	RID_DEVICE_INFO rdi = {sizeof(rdi)};
    char devName[128];
    UINT rdiSize = sizeof(rdi),nameSize = sizeof(devName);
    for(i=0;i<SDL_RawDevList.size();i++)       
	{	
		if(SDL_RawDevList[i].dwType==RIM_TYPEHID
		&&0<(INT)GetRawInputDeviceInfoA(SDL_RawDevList[i].hDevice,RIDI_DEVICENAME,devName,&nameSize)
		&&strstr(devName,"IG_")
		&&0<(INT)GetRawInputDeviceInfoA(SDL_RawDevList[i].hDevice,RIDI_DEVICEINFO,&rdi,&rdiSize))
		{		
			DWORD dwVidPid = MAKELONG(rdi.hid.dwVendorId,rdi.hid.dwProductId);			
			for(j=0;j<EX_JOYPADS;j++) if(dwVidPid==EX::Joypads[j].product.Data1)
			{
				EX::Joypads[j].isXInputDevice = true;
			}
		}
		assert(rdi.cbSize==rdiSize&&rdiSize==sizeof(rdi)&&nameSize==sizeof(devName));
	}

	//2020: assuming this corresponds to the XInput user id?
	int xi = 0;
	for(j=0;j<EX_JOYPADS;j++) if(EX::Joypads[j].isXInputDevice)
	{
		EX::Joypads[j].isXInputDevice = ~xi++;
	}

	if(xi) 
	{
		//I think this is in order of oldest. not sure
		const char *libs[] = {"xinput1_4.dll","xinput1_3.dll","xinput9_1_0.dll"};
		for(int i=0;i<3;i++) if(HMODULE xinput=LoadLibraryA(libs[i]))
		{
			(void*&)Ex_input_XInputGetState = GetProcAddress(xinput,"XInputGetState");
			break;
		}
	}
}
