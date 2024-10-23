
#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

#include <Winsock2.h> //PSVR
#pragma comment(lib,"Ws2_32.lib")

#include "dx.ddraw.h"
#include "dx.dinput.h" 
#include "dx.dsound.h" 

#include "Ex.ini.h" 
#include "Ex.input.h" 
#include "Ex.output.h" 
#include "Ex.window.h" 
#include "Ex.cursor.h" 
#include "Ex.movie.h"

#include "som.932.h"
#include "som.932w.h"
#include "som.title.h"
#include "som.state.h"
#include "som.status.h"
#include "som.extra.h"

extern int SomEx_pc,SomEx_npc; //SOM::Red

static bool som_status_ddraw_is_pausing();
static bool som_status_ddraw_is_waiting();

static void som_status_onreset();
static void (*som_status_onreset_passthru)() = 0;
static void som_status_oneffects();
static void (*som_status_oneffects_passthru)() = 0;

extern void SOM::initialize_som_status_cpp() 
{
	if(som_status_onreset_passthru) return;

	som_status_onreset_passthru = DDRAW::onReset; 
	som_status_oneffects_passthru = DDRAW::onEffects; 

	DDRAW::onReset = som_status_onreset;
	DDRAW::onEffects = som_status_oneffects;

	EX::INI::Option op;

	if(op->do_pause)
	{
		DDRAW::doPause = true;	 
		DDRAW::onPause = som_status_ddraw_is_pausing;
	}
}

extern void SOM::Clear(DWORD color, float z)
{
	if(!DDRAW::Direct3DDevice7) return;

	DDRAW::IDirect3DDevice7 *dev = DDRAW::Direct3DDevice7;

	/*SOM should be emulated precisely here*/

	DWORD fvf = D3DFVF_TLVERTEX, c = color; 

	//float w = SOM::fov[0], h = SOM::fov[1];
	float w = SOM::width, h = SOM::height;

	float fan[4*8] =
	{
		0,0,z,1,*(float*)&c,0,0,0,
		w,0,z,1,*(float*)&c,0,0,0,
		w,h,z,1,*(float*)&c,0,0,0,
		0,h,z,1,*(float*)&c,0,0,0,
	};

	DWORD state = 0xFFFFFFFF;

	if(dev->CreateStateBlock(DX::D3DSBT_ALL,&state)!=D3D_OK) return;
		
	assert(z<1); //obsolete? (reminder: do_fix_zbuffer_abuse)

	dev->SetRenderState(DX::D3DRENDERSTATE_ZENABLE,z>0&&z<1);
	dev->SetRenderState(DX::D3DRENDERSTATE_ZFUNC,DX::D3DCMP_GREATEREQUAL);
	dev->SetRenderState(DX::D3DRENDERSTATE_ALPHATESTENABLE,0);
	dev->SetRenderState(DX::D3DRENDERSTATE_LIGHTING,0); 
	dev->SetRenderState(DX::D3DRENDERSTATE_FOGENABLE,0);
	dev->SetRenderState(DX::D3DRENDERSTATE_ZWRITEENABLE,z==1);	
	dev->SetRenderState(DX::D3DRENDERSTATE_CULLMODE,DX::D3DCULL_NONE); 
																	   
	if((color&0xFF000000)!=0xFF000000)
	{
		dev->SetRenderState(DX::D3DRENDERSTATE_ALPHABLENDENABLE,1);	
		dev->SetRenderState(DX::D3DRENDERSTATE_SRCBLEND,DX::D3DBLEND_SRCALPHA);	
		dev->SetRenderState(DX::D3DRENDERSTATE_DESTBLEND,DX::D3DBLEND_INVSRCALPHA);
	}
	else dev->SetRenderState(DX::D3DRENDERSTATE_ALPHABLENDENABLE,0);	

	dev->SetTexture(0,0); //binds a white texture if DDRAW::doWhite

	//mimic Sword of Moonlight for shader constants consistency
	dev->SetTextureStageState(0,DX::D3DTSS_COLOROP,DX::D3DTOP_SELECTARG2);
	dev->SetTextureStageState(0,DX::D3DTSS_COLORARG2,D3DTA_DIFFUSE);
	dev->SetTextureStageState(0,DX::D3DTSS_ALPHAOP,DX::D3DTOP_SELECTARG2);
	dev->SetTextureStageState(0,DX::D3DTSS_ALPHAARG2,D3DTA_DIFFUSE);	
	
	DDRAW::PushScene();		
	dev->DrawPrimitive(DX::D3DPT_TRIANGLEFAN,fvf,fan,4,0);	
	DDRAW::PopScene();

	dev->ApplyStateBlock(state);
	dev->DeleteStateBlock(state);
}

static int som_status_paused = 0;
static unsigned som_status_pausetick = 0;

extern void SOM::Pause(int ch)
{ 
	if(DDRAW::xr&&ch!='user') return;

	if(EX::window&&!EX::is_visible()) return;

	/*seeing if this is unnecessary //TESTING
	if(DDRAW::isPaused)
	{
		//assert(som_status_paused);

		//2021: something is constantly spamming WM_ACTIVATE messages
		//when minimized
		if(ch=='mini'&&som_status_paused)
		{
			som_status_paused = 'mini'; //giving priority :(
		}
	}*/

	if(!DDRAW::doPause||DDRAW::isPaused/*||!DINPUT::Keyboard*/) //???
	{	
		return; //hack??
	}
	
	//disabled during movie
	if(!EX::is_playing_movie())
	{
		som_status_paused = ch; //Note: one pauser per pause permitted

		/*2021; minimizing while 'auto' paused was phantom unpausing leaving
		//the app runningi in the taskbar... I've also changed Ex.window.cpp
		//to use WM_SYSCOMMAND instead of WM_SIZE
		//(probably a Windows 10 app "breaking change")
		if(DDRAW::onPause==som_status_ddraw_is_waiting)
		return;*/
		if(DDRAW::onPause!=som_status_ddraw_is_waiting)
		{
			DDRAW::onPause = som_status_ddraw_is_pausing; //should be unnecessary

			som_status_pausetick = EX::tick();
		}

		DDRAW::isPaused = true;
	}
}
extern void som_mocap_Tobii_unpause();
extern void SOM::Unpause(int ch)
{
	if(DDRAW::xr&&ch!='user') return;

	//disabled during movie
	if(!EX::is_playing_movie())
	{
		if(!DDRAW::doPause) return;
		if(!DDRAW::isPaused) return;
	//	if(!DINPUT::Keyboard) return; //hack?? //???
		
		//user channel may unpause whenever they like
		if(ch!='user'&&som_status_paused!=ch) return;
	}

	DDRAW::isPaused = false;

	som_status_paused = 0;

	unsigned t = EX::tick()-som_status_pausetick;
	if(SOM::eventick!=0) //cancelled?
	SOM::eventick+=t;

	som_mocap_Tobii_unpause();
}

//static DWORD som_status_interlace; //REMOVE ME
static bool som_status_ddraw_is_pausing()
{
	//if(!DINPUT::Keyboard) return true; //paranoia	 //???

	/*REMOVE ME (NO LONGER SUPPORTING)
	//hack? maybe could do in dx.ddraw.cpp	
	DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_STENCILENABLE,0);
	DDRAW::Direct3DDevice7->GetRenderState(DX::D3DRENDERSTATE_STENCILENABLE,&som_status_interlace);*/
	//
	//	FIX ME: som_status_ddraw_is_pausing() is
	//	entered differently for OpenGL so that the
	//	fx2ndSceneBuffer=0 comes too late
	//
	DDRAW::fx2ndSceneBuffer = 0;

	//TODO: 43fd00/43fe70 has code for displaying
	//Truth Glass text but it will have to be reworked
	//to be of any use
	wchar_t paused[32];	
	SOM::translate(paused,som_932_PAUSED,"PAUSED");	 
	SOM::Black();
	SOM::Print(paused);

	DDRAW::onPause = som_status_ddraw_is_waiting;

	return true; //hack
}

static bool som_status_ddraw_is_waiting()
{	
	DSOUND::Stop(); //pause all playback
		
	HANDLE th = GetCurrentThread();
	DWORD priority = GetThreadPriority(th);
	if(!SetThreadPriority(th,THREAD_PRIORITY_IDLE)) assert(0);

	DWORD ticks = EX::tick();

	const int sleep = 100; //100: polled
	for(int sleeping=0;DDRAW::isPaused;)
	{	
		MSG msg; //VK_PAUSE/courtesy...
		while(PeekMessage(&msg,0,0,0,PM_REMOVE))
		{
			if(msg.message==WM_KEYDOWN) switch(msg.wParam)
			{
			case 'S': case VK_SCROLL: //adjacent to Pause/Break

				SetThreadPriority(GetCurrentThread(),priority);
				if(!SOM::record())
				SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_IDLE);
				else SOM::Unpause();
				continue;

				/*REMINDER: this repauses (preventing unpause)
				 
			case VK_PAUSE: //2022: why not?

				SOM::Unpause(); assert(!DDRAW::isPaused);

				continue;*/

			case VK_F2: //2022

				//headsets can go to sleep it can be confusing
				//if the game isn't unpausing so users may try
				//F2 and this assumes they want to leave VR as
				//there's no function overlay (alt+f2) display
				//plus DirectInput still fails when debugging!
				if(DDRAW::xr)
				{
					//altf2 does this
					//SOM::Unpause(); assert(!DDRAW::isPaused);

					SOM::altf2(); continue;
				}
				break;
			}		
			else switch(msg.message) //2018: having issues with workstation
			{
			case WM_NCACTIVATE:
			case WM_ACTIVATE:
			case WM_LBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_LBUTTONDBLCLK:
			case WM_RBUTTONDBLCLK:

			case WM_SYSCOMMAND: //helping? taskbar is getting stuck :(

				SOM::Unpause('auto'); //testing
			}
			//if(!TranslateAccelerator(EX::window,haccel,&msg))
			{
				TranslateMessage(&msg); DispatchMessage(&msg); 
			}
		}
		if(!DDRAW::isPaused) continue; //2020
				
		DWORD delta = EX::tick();

		sleeping+=delta-ticks; ticks = delta;

		if(sleeping>sleep) 
		{	
			sleeping = 0; 

			//NEW: prevent unpausing with controller
			if(GetForegroundWindow()==EX::display())
			{
				if(DINPUT::Keyboard) //mimic SOM 
				{
					char dinput[256];
					if(1!=DINPUT::Keyboard->Acquire()) //S_FALSE always?
						assert(0); //2021
					DINPUT::Keyboard->GetDeviceState(256,dinput); 
				} 
				if(DINPUT::Joystick) //mimic SOM
				{
					DX::DIJOYSTATE st; DINPUT::Joystick->Acquire(); 
					if(DINPUT::Joystick->as2A) DINPUT::Joystick->as2A->Poll();
					DINPUT::Joystick->GetDeviceState(sizeof(st),&st);
				}
			}
			else EX::sleep(500); //nice
		}		
		else EX::sleep(33);
	}	

	if(!SetThreadPriority(th,priority)) assert(0);

	DDRAW::onPause = som_status_ddraw_is_pausing;

	DSOUND::Play(); //resume all playback

	DDRAW::fx2ndSceneBuffer = 0;
	//DDRAW::Direct3DDevice7->SetRenderState
	//(DX::D3DRENDERSTATE_STENCILENABLE,som_status_interlace); //REMOVE ME

	return false; //hack
}

static void som_status_shadow
(DWORD c, int off, HDC hdc, const wchar_t *txt, int len, LPRECT box, UINT how)
{	
	if(!(c&0xff000000)) return; //short circuit
		
	c = c&0xFF00FF00|c<<16&0xFF0000|c>>16&0xFF;

	box->left+=off; box->right+=off; box->bottom+=off; box->top+=off;
	
	SetTextColor(hdc,c&0x00FFFFFF); DrawTextW(hdc,txt,len,box,how);
	
	box->left-=off; box->right-=off; box->bottom-=off; box->top-=off;
}

static UINT som_status_start = 0;

extern int SOM::Start(const wchar_t *txt, DWORD tint, LPRECT box, UINT how)   
{
	return SOM::Print(txt,box,how,&(som_status_start=tint));
}

extern int SOM::Print(const wchar_t *txt, LPRECT pbox, UINT how, UINT *how2)   
{
	auto pri = DDRAW::BackBufferSurface7; assert(pri); //2021

	if(!txt||!*txt||!pri) return 0;

	UINT Start = how2==&som_status_start?*how2:0;

	if(how2&&!Start) how|=*how2; //sequence points workaround //???

	RECT box; if(!pbox)
	{
		if(!GetClientRect(SOM::window,&box)) return 0;

		//FIX ME?
		//any margins need to be subtracted
		/*fails do_scale_640x480_modes_to_setting
		if(DDRAW::gl)
		{			
			//here D3DX is strange for working in window coordinates
			box.right*=DDRAW::xyScaling[0];
			box.bottom*=DDRAW::xyScaling[1];
		}*/

		DDRAW::doSuperSamplingMul(box);
	}
	else 
	{
		box = *pbox;
		/*2020: note, Ex_detours_DrawTextA does this destructively
		box.top = DDRAW::xyScaling[1]*box.top+DDRAW::xyMapping[1]+0.5f;
		box.left = DDRAW::xyScaling[0]*box.left+DDRAW::xyMapping[0]+0.5f;		
		box.right = DDRAW::xyScaling[0]*box.right+DDRAW::xyMapping[0]+0.5f;
		box.bottom = DDRAW::xyScaling[1]*box.bottom+DDRAW::xyMapping[1]+0.5f;
		DDRAW::doSuperSamplingMul(box);*/
		DDRAW::xyRemap2(box);
	}	

	HDC hdc = 0; pri->GetDC(&hdc);

	HGDIOBJ pusha = 0; int pushb = 0; COLORREF pushc; 
	
	static HFONT font = 0, som_font = 0; //hack

	static bool enlarge = false; //2020

	if(!font||som_font!=SOM::font||!GetObject(font,0,0)) 
	{
		DeleteObject(font); //2020: resource leak?

		//Reminder: status and system fonts may be the same...
		font = SOM::makefont(SOM::fov[0]/DDRAW::xyScaling[0],0,0); //YUCK 		
		
		//hack: make a menu font for those routines that expect it (eg. SOM::st)
		if(!SOM::font) SOM::font = SOM::makefont(SOM::fov[0]/DDRAW::xyScaling[0]); //YUCK
		
		som_font = SOM::font; //!

		LOGFONTW log; GetObjectW(font,sizeof(log),&log);
		LOGFONTW cmp; GetObjectW(som_font,sizeof(cmp),&cmp);

		enlarge = abs(cmp.lfHeight)>abs(log.lfHeight);
	}
	
	//HACK: the kf2 gauge is using Start to draw its menu style
	//text using status font (it needs more work)
	//if the status font is smaller than the main font centered
	//text is displayed with the main font. status_fonts_to_use
	//was intended for larger fonts but is repurposed for KF2's
	//gauges display
	HFONT f = how&DT_CENTER&&(Start||enlarge)?som_font:font;

	pusha = SelectObject(hdc,f); //SOM::font

	EX::INI::Script lc;

	DWORD c = !Start?lc->status_fonts_tint:Start;

	c = c&0xFF00FF00|c<<16&0xFF0000|c>>16&0xFF;

	pushb = SetBkMode(hdc,TRANSPARENT);
	
	//This works but fails hard on closeout
	//if(Start) assert(GetTextColor(hdc)==c);

	int len = wcslen(txt);

	if(txt&&!Start) //NEW: shadows
	{
		if(~how&DT_CALCRECT) //2021
		{
			//if(SOM::fontshadow&0xFF000000)		
			{
				DWORD c2 = lc->status_fonts_contrast;

				//som_status_shadow(c2,-1,hdc,txt,len,&box,how);
				som_status_shadow(c2,+1,hdc,txt,len,&box,how);
			}
		}
	}

	pushc = SetTextColor(hdc,Start?c:c&0x00FFFFFF);
	//calling Ex_detours_DrawTextW?
	int out = DrawTextW(hdc,txt,len,&box,how);

	SetTextColor(hdc,pushc);
	SetBkMode(hdc,pushb);

	if(pusha) SelectObject(hdc,pusha);

	pri->ReleaseDC(hdc);

	if(how&DT_CALCRECT&&pbox) 
	{
		/*2020: note, Ex_detours_DrawTextA isn't reversing afterward
		box.top = (box.top-DDRAW::xyMapping[1])/DDRAW::xyScaling[1]+0.5f;
		box.left = (box.left-DDRAW::xyMapping[0])/DDRAW::xyScaling[0]+0.5f;
		box.right = (box.right-DDRAW::xyMapping[0])/DDRAW::xyScaling[0]+0.5f;
		box.bottom = (box.bottom-DDRAW::xyMapping[1])/DDRAW::xyScaling[1]+0.5f;
		DDRAW::doSuperSamplingDiv(box);*/
		DDRAW::xyUnmap2(box);

		*pbox = box;
	}

	return out;
}

extern const wchar_t *SOM::Joypad(int i, const wchar_t *prefix)
{		
	if(!DINPUT::Joystick2) //hack
	{
		DINPUT::Joystick2 = DINPUT::Joystick; 
		
		assert(0); //wanna know if this is happening
	}

	static wchar_t out[MAX_PATH];

	if(prefix) wcscpy_s(out,MAX_PATH,prefix); else *out = '\0';

	DX::DIDEVICEOBJECTINSTANCEW doi = {sizeof(doi)};

	typedef DX::DIJOYSTATE DIJOYSTATE; //DIJOFS_BUTTON 

	assert(DINPUT::Joystick2->target=='dx7');
	if(DINPUT::Joystick2->GetObjectInfo(&doi,DIJOFS_BUTTON(i),DIPH_BYOFFSET)==DI_OK)
	{			
		assert(DINPUT::Joystick2->instance==EX::Joypads[0].instance);

		if(i>=0&&i<=7)
		if(EX::Joypads[0].isXInputDevice) //Xbox like?
		{
			const wchar_t bts[8][3] = 
			{L"A",L"B",L"X",L"Y",L"LB",L"RB",L"LT",L"RT"};
			wcscpy(doi.tszName,bts[i]);

			//NEW: these are virtual buttons, not in the control panel
			if(i>=6&&'('==*out) out[1] = '0';
		}
		else if(EX::Joypads[0].isDualShock //Sony DS4 (DS3 uses Xbox)
		||!strcmp("SmartJoy PLUS Adapter",DINPUT::Joystick2->product)) //Lik-Sang DS2
		{
			if(EX::Joypads[0].isDualShock&&*out) switch(i)
			{
			//remap control panel buttons / match Press_any_button
			case 0: case 1: out[1]++; break; case 2: out[1] = '1';			
			}

			if(i<=3) //custom face buttons
			{
				doi.tszName[1] = L'\0';
				int j = i;
				int ds4[4] = {2,1,3,0};
				if(EX::Joypads[0].isDualShock) 
				j = ds4[i];
				doi.tszName[0] = som_932w_SmartJoy[j];
			}
			else //names don't show with a generic driver
			{	
				int _1_ = '1', _2_ = '2';
				if(EX::Joypads[0].isDualShock)
				std::swap(_1_,_2_);

				doi.tszName[0] = i&1?'R':'L';
				doi.tszName[1] = i>=6?_1_:_2_;
				doi.tszName[2] = L'\0';				
			}
		}
		
		wcscat_s(out,MAX_PATH,doi.tszName);		
	}

	return *doi.tszName?out:0;	
}

extern bool *SOM::Buttons(bool *down, size_t downsz)
{
	if(!DINPUT::Joystick2)
	DINPUT::Joystick2 = DINPUT::Joystick; //hack

	if(!DINPUT::Joystick2) return 0;

	assert('dx7'==DINPUT::Joystick2->target);

	//REMOVE ME?
	static bool out[32]; //__declspec(thread)

	if(!down)
	{ 
		if(downsz>32) return 0;

		down = out; downsz = 32; 
	}
	else if(downsz>32) //paranoia 
	{
		memset(down+32,0x00,sizeof(bool)*(downsz-32));
		downsz = 32;
	}

	if(!downsz) return 0;

	DINPUT::Joystick2->proxy->Acquire(); //Note: bypassing proxy...

	DX::DIJOYSTATE dijs;
	if(DINPUT::Joystick2->proxy->GetDeviceState(sizeof(dijs),&dijs)==DI_OK)
	{
		for(size_t i=0;i<downsz;i++) down[i] = dijs.rgbButtons[i];
	}
	else return 0; return down;
}

static struct som_status_PSVRToolbox
{		 
	//HDC gammaDC; WORD *gamma;

	SOCKADDR_IN recv,send; SOCKET sensor;

	som_status_PSVRToolbox()
	{
		memset(this,0x00,sizeof(*this));
		sensor = INVALID_SOCKET;
		WSADATA w; 
		if(0!=WSAStartup(0x202,&w))
		return;
		sensor = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP); 
		u_long mode = 1; // 1 to enable non-blocking socket
		ioctlsocket(sensor,FIONBIO,&mode);
		hostent *lh = gethostbyname("");
		char *ip = inet_ntoa(*(struct in_addr*)*lh->h_addr_list);
		EX::INI::Stereo vr;
		unsigned short recv_port = vr->PSVRToolbox_status_port;
		unsigned short send_port = vr->PSVRToolbox_command_port;
		recv.sin_family = AF_INET;
		recv.sin_port = htons(recv_port);
		//I think this needs setsockopt to work?
		//recv.sin_addr.s_addr = INADDR_BROADCAST;
		recv.sin_addr.s_addr = inet_addr(ip);
		//Picky: Only recv socket bind.
		int err = bind(sensor,(SOCKADDR*)&recv,sizeof(recv));
		send.sin_family = AF_INET;		
		send.sin_addr.s_addr = inet_addr(ip);
		send.sin_port = htons(send_port);
	}
	//~som_status_PSVRToolbox()
	//{
	//	if(gammaDC) SetDeviceGammaRamp(gammaDC,gamma);
	//}

	void _send_command(SOCKET s, char *json, const char *command)
	{
		int len = sprintf(json,"{\"Command\":\"%s\"}",command);
		if(-1==sendto(s,json,len+1,0,(SOCKADDR*)&send,sizeof(send)))
		{
			len = WSAGetLastError();
			EX::dbgmsg("sendto: %d (PSVR)",len);
		}	
	}
	void operator()(const char *command=0)
	{	
		char json[4096];

		extern void som_mocap_PSVR(SOM::Sixaxis[2],int);

		if(command)
		{	
			if('R'==*command) //Recenter?
			{
				assert(!strcmp("Recenter",command));
				som_mocap_PSVR(0,1);
			}
			else //Not sending "Recenter"
			{			
				SOCKET s = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
				int err = bind(s,(SOCKADDR*)&send,sizeof(send)); 
				if(-1==err) err = WSAGetLastError();
				_send_command(s,json,command);
				//PSVRToolbox reverts settings after a mode change
				//and so uses this awkwardly worded API to get the
				//setting back????
				_send_command(s,json,"DiscardSettings");
				closesocket(s);
			}
		}

		if(sensor!=INVALID_SOCKET) flush:
		{
			int len = recvfrom(sensor,json,sizeof(json)-1,0,0,0);
			if(len==-1)
			{	
				len = WSAGetLastError(); 
				if(len!=WSAEWOULDBLOCK)
				EX::dbgmsg("recvfrom: %d (PSVR)",len);
				goto def;
			}
			else if(len>0)
			{
				//assuming layout won't change... skip past uninteresting
				//tidbits
				json[len] = '\0';
				/*#ifdef _DEBUG
				//Visual Studio can't Copy a slightly long watch variable
				static bool one_off = (OutputDebugStringA(json),true);
				#endif*/
				char *p = json; 
				while(*p++) if(*p==',') *p = '\n';				
				p = strstr(json,"GroupSequence1"); //OBSOLETE
				bool getAccelShort;
				if(getAccelShort=!p)
				{
					//this is unreleased version??
					p = strstr(json,"Timestamp1");
				}

			if(0) //REMINDER: might disappear if resolution can't fit it
			{
				static unsigned cmp = 0; if(cmp!=SOM::frame)
				{
					EX::dbgmsg(0&&p?p:json); cmp = SOM::frame;
				}
			}
				  
				//note: there are 2 sets of sensors (actually no, 2 sets of samples)
				SOM::Sixaxis sa[2]; if(p) 
				{
					for(int i=0;i<2;i++)
					{
						sa[i].time = atoi(p=strchr(p,':')+1);
						short *q = sa[i].gyro,*b = sa[i].motion+3;
						while(q<b) *q++ = atoi(p=strchr(p,':')+1);					
						//NOTE: getAccelShort does so, contradicting the wiki saying:
						if(!getAccelShort) //OBSOLETE
						{
							//"Accelerometer data is raw data from the sensor, 
							//it must be displaced 4 bits to the right preserving sign"
							b[-3]>>=4; b[-2]>>=4; b[-1]>>=4;
						}
						/*This seems to mess with MadgwickAHRS (it ties the linear and
						//angular measurements together)
						//WARNING: swapping this so the headset appears to be upright
						//otherwise quaternion code, etc. is unorthodox
						std::swap(sa[i].gyro[0],sa[i].gyro[1]);
						*/
					}

					assert(0==strncmp(json,"{\"Buttons\":",11));
					switch(sa[0].button=json[11]-'0')
					{
					default: assert(0);
					case 0: case 1: case 2: case 4: case 8: break;
					}

					som_mocap_PSVR(sa,0);
				}
				else assert(p);

				goto flush; //don't want back up on the socket
			}
		}
		else def: //HACK: Ex_output_f6_head[5] = -SOM::uvw[1];
		{
			som_mocap_PSVR(0,0); 
		}
	}

}*som_status_PSVR = 0;
extern void SOM::PSVRToolbox(const char *Command)
{
	if(DDRAW::gl) return; //OpenXR?

	//2020: this is instantiating som_status_PSVR
	//without ever using stereo!
	/*if(!DDRAW::inStereo) return;
	//the "EnableVRMode" command at startup can't
	//get through here*/
	if(!SOM::stereo&&!som_status_PSVR) return;

	if(!som_status_PSVR)
	som_status_PSVR = new som_status_PSVRToolbox;
	som_status_PSVR->operator()(Command);
}

extern const wchar_t *SOM::Status(RECT *box, UINT *how, int elem, size_t n)
{
	static wchar_t pts[2][66] = {L"",L""};
	static wchar_t ptf[2][32] = {L"",L""};
	static wchar_t bad[5][32] = {L"",L"",L"",L"",L""};
	static wchar_t hit[1][32] = {L""};

	static int locale = 0;

	if(!locale++) //todo: watch locale
	{
		//TODO: allow HP/MP to exceed 999
		SOM::translate(ptf[0],som_932_Shadow_Tower_HP,"HP %3d");
		SOM::translate(ptf[1],som_932_Shadow_Tower_MP,"MP %3d");		
		//reminder: should match som.terms.inl
		assert(som_932_States[0][0]==' '); //+1
		SOM::translate(bad[0],som_932_States[0]+1,"Poison");
		SOM::translate(bad[1],som_932_States[1]+1,"Palsy");
		SOM::translate(bad[2],som_932_States[2]+1,"Blind");
		SOM::translate(bad[3],som_932_States[3]+1,"Curse");
		SOM::translate(bad[4],som_932_States[4]+1,"Slow");
	}

	static float lineheight = 0, ratio;

	static int kf2_w; //2020: King's Field II mode?

	static HFONT hack = 0; 
	if(!hack||hack!=SOM::font
	||EX::stereo_font&&SOM::altf&1<<2)
	{
		//0: reserved for a decimal point presentation
		swprintf(pts[0],ptf[0],999,0); swprintf(pts[1],ptf[1],999,0);

		int how2 = DT_CALCRECT|DT_NOPREFIX|DT_SINGLELINE;

		RECT hp_s = {}; SOM::Print(pts[0],&hp_s,how2);
		RECT mp_s = {}; SOM::Print(pts[1],&mp_s,how2);

		kf2_w = max(hp_s.right-hp_s.left,mp_s.right-mp_s.left);

		ratio = float(max(hp_s.right,mp_s.right))/(hp_s.bottom-hp_s.top);

		//TODO: provide an extension to provide "X"
		RECT calc = {}; int test = SOM::Print(L"X",&calc,how2);

		//REMINDER: DDRAW::xyMapping applies a fixed Y offset
		lineheight = calc.bottom-calc.top; 
		
		hack = SOM::font;
	}
	
	if(box) //a general starting point
	{
		float margin = lineheight/2;

		margin*=EX::INI::Detail()->st_margin_multiplier;		
							
		box->left = margin; box->right = SOM::width-margin;
		box->top = margin; box->bottom = SOM::height-margin;

		//2020: trying to do this in SOM::Print
		//if(elem!='bar')
		//OffsetRect(box,DDRAW::xyMapping[0],DDRAW::xyMapping[1]);
	}

	if(how) *how =	DT_NOPREFIX|DT_NOCLIP|DT_SINGLELINE; //ditto

	const float x = 1.1; //Japanese is cramped

	switch(elem)
	{
	default: return 0; 

	case 'pts': if(n>1) return 0;

		if(how) *how|=DT_TOP; if(how&&n==1) *how|=DT_RIGHT; //MP

		if(EX::INI::Detail()->st_status_mode==2) n = n?0:1; //hack: flip sides

		swprintf_s(pts[n],ptf[n],SOM::L.pcstatus[n?6:4],0);

		if(box) //2020: King's Field II mode?
		{
			box->bottom = box->top+lineheight;
			if(~*how&DT_RIGHT) 
			box->right = box->left+kf2_w+box->left;
		}

		return pts[n];

	case 'bar': if(n>1) return 0;

		if(box)
		{
			box->top+=lineheight*x; //x: just looks nicer
			
			box->bottom = box->top+16; //height in texture

			if(EX::INI::Detail()->st_status_mode==2) n = n?0:1; //hack: flip sides

			if(n==0) box->right = box->left+ratio*lineheight;
			if(n==1) box->left = box->right-ratio*lineheight;
		}
		return L""; //non-textual

	case 'bad': if(n>4) return 0;

		if(how) *how|=DT_BOTTOM|DT_RIGHT; 

		if(SOM::L.pcstatus[SOM::PC::xxx]&1<<n)
		{
			for(size_t i=0;i<n;i++) 
			if(SOM::L.pcstatus[SOM::PC::xxx]&1<<i)
			{
				if(*bad[i]) box->bottom-=lineheight*x;
			}
			return bad[n]; 
		}		
		return 0;		

	case 'hit': if(n>0) return 0;

		if(how) *how|=DT_BOTTOM;

		SOM::Versus(); //int x = 

		if(SOM::Versus.all.timer>0)
		{	
			if(SOM::Versus.all.combo>0)
			_itow_s(SOM::Versus.all.combo,hit[0],10); 

			return hit[0];
		}
		return 0;
	}
}

//extern float SOM::Red(WORD hit, WORD max)
extern float SOM::Red(WORD hit, int src, int dst, WORD max)
{	
	if(dst==12288) max = SOM::L.pcstatus[SOM::PC::_hp];

	assert(hit&&max);

	EX::INI::Damage hp; //2020

	//note: designers may want this to be a downward
	//spiral by basing it on current HP instead of max
	int crit; if(&hp->critical_hit_point_quantifier)
	{
		int swap[2] = {SomEx_pc,SomEx_npc};
		{
			//DUPLICATES som_logic_406ab0 (arguments?)
			crit = hp->critical_hit_point_quantifier();
		}
		SomEx_pc = *swap; SomEx_npc = swap[1];
	}
	else //HACK: o is 1/3 so other uses are consistent
	{	
		crit = max*hp->critical_hit_point_quantifier.o();
	}

	float x = (float)hit/crit;
	
	//2021: note, som_state_404470 is calling this
	//without consulting do_red... it's too much
	//trouble to set up critical_hit_point_quantifier 
	if(!EX::INI::Option()->do_red) 
	{
		return x>=1; //critical hit?
	}

	/*REFERENCE: this is the old formula prior
	//to Decemeber 2020. since discovering SOM
	//has a critical hit system (animation #22) 
	//I feel like these should be retired so it
	//can be synchronized with the critical hit
	//(only red_multiplier was ever documented)
	//EX::INI::Detail dt; 
	float x = hit/(0.5f/dt->red_multiplier*max);
	if(&dt->red_adjustment) 
	x = dt->red_adjustment(x); return min(x,1);*/
	return min(1,x); 
}
extern bool SOM::Stun(float red) //REMOVE ME
{									 
	if(red>=1) return true;
				 
	//EXTENSION
	//2020: I feel like there should be more hit animations
	//some way that doesn't involve the existing extensions
	//the player should be stunned more often too, and some
	//way to stun large HP monsters would be a good idea to
	//incorporate
	//red = powf(red,0.75f); //sqrtf

	//2020: I think this is equivalent to the old code (and
	//much simpler)
	//return int(red*100-50)>SOM::rng()%50;
	return int(red*100-20)>SOM::rng()%80;
}

struct SOM::Versus SOM::Versus;

int SOM::Versus::operator()()
{
	static unsigned sync = 0; if(sync!=SOM::frame) 
	{
		sync = SOM::frame;

		int delta = DDRAW::noTicks; 
	
		for(int i=0;i<x_s;i++) 
		{
			//Going negative because F7 no longer shows 
			//dead NPCs for long before they may change.
			if((x[i].timer-=delta)<=/*0*/-timeout) 
			{
				x[i].timer = x[i].multi = x[i].combo = 0;
			}
		}

		//Going negative because F7 no longer shows 
		//dead NPCs for long before they may change.
		if((all.timer-=delta)<=/*0*/-timeout) 
		{	
			all.timer = all.multi = all.combo = 0;
		}
	}

	return f7;	
}
bool SOM::Versus::hit2(int npc, int dmg, int _hp, float red)
{
	//2020: KF2 needs to increase this limit

	return hit(512+npc,dmg,_hp,red); //128
}
bool SOM::Versus::hit(int ai, int dmg, int _hp, float red)
{	
	enum{ sep=512 }; //128 //see hit2 above

	int hp; if(ai>=sep) //NPCs
	{
		hp = SOM::L.ai2[ai-sep][SOM::AI::hp2];
	}
	else hp = SOM::L.ai[ai][SOM::AI::hp];
		
	all.multi++; //new //UNUSED?
	all.combo+=dmg;
	all.timer = timeout;

	int EDI; if(ai<sep) //som_db.exe
	{
		//EDI = 0x4C77C8+ai*149*4+72; 
		EDI = (DWORD)SOM::L.ai+ai*149*4+72;
	}
	else EDI = 0X1A12DF0+(ai-sep)*43*4+64; 

	int i = -1; 
	for(int j=0;j<x_s;j++)		
	//F7 lasts a second; do_red about a third.
	if(x[j].EDI==EDI&&x[j].timer>timeout-333
	//default to the first unused enemy slot
	||i==-1&&x[j].timer<=0/*-timeout*/)
	i = j;
	if(i==-1) i = 0;
	
	f7 = i; //2017

	if(x[i].EDI==EDI&&x[i].timer>timeout-333)
	{
		//som_db.exe calls som_state_404470 multiple times
		//in a single frame. assuming multi-hit attacks do
		//not hit in a frame
		//don't know how to reason about multi-hit attacks
		//if(x[i].timer==timeout)
		return false;
	}
	else x[i].multi = x[i].combo = 0;
		
	x[i].EDI = EDI;
	x[i].enemy = ai<sep?ai:-1;
	x[i].npc = ai>=sep?ai-sep:-1;		
	//1000: message timeout
	//delta: gets subtracted
	x[i].timer = timeout; 
	x[i].drain = dmg>hp?hp:dmg;
	//2020: critical_hit_point_quantifier?
	//TODO: how to add attacks?
	x[i].red = red;
	//the combo just builds up so that
	//the player doesn't miss anything
	x[i].multi++; x[i].combo+=dmg; 
	//2017
	if(x[i].enemy!=-1)
	{
		WORD prm = SOM::L.ai[x[i].enemy][SOM::AI::enemy];
		x[i].HP = SOM::L.enemy_prm_file[prm].s[148];
	}
	else
	{
		WORD prm = SOM::L.ai2[x[i].npc][SOM::AI::npc];
		x[i].HP = SOM::L.NPC_prm_file[prm].s[1];
	}
	return true;
}

static DX::IDirectDrawSurface7 *som_status_autolock = 0;
static DX::IDirectDrawSurface7 *som_status_autolock2 = 0;

//extern: som.state.cpp
extern SOM::Texture *som_status_mapmap = 0;
extern void som_status_automap(int i, int j, int x, int y, int sh)
{	
	static DX::DDSURFACEDESC2 image, icons = {sizeof(icons)};	

	if(!som_status_autolock)
	{
		/*2021: not needed with colorkey_f
		//NOTE: som_game_LoadImageA sets this up
		//HACK! reenabling MLAA
		if(!DDRAW::psColorkey)
		DDRAW::colorkey = SOM::colorkey;*/

		//2021: now have need to fill in alpha masked regions
		//manually since it's deferred until SetTexture or Blt
		//2022: 4 is an optimization since this textures doesn't
		//need to be uploaded
		som_status_mapmap->texture->updating_texture(4);

		som_status_autolock = som_status_mapmap->texture;
		som_status_autolock->Lock(0,&icons,DDLOCK_WAIT|DDLOCK_READONLY,0);
		assert(icons.lpSurface);

		//Reminder: Moratheia has a 256x256 BMP but it's unclear
		//if it is used. It's been upsized and is fuzzy
		assert(128==icons.dwWidth&&128==icons.dwHeight);

		//TODO: USE SOM'S TEXTURE FRAMEWORK
		if(!som_status_autolock2)
		{	
			image = icons;

			//2021: for some reason Ex_mipmap is now failing on
			//odd sized mipmaps... I think SetColorKey was never
			//called and therefor mipmaps were never generated???
			//it seems that it's impossible to scale the map below
			//300x300 so it doesn't require mipmaps
			auto swap = DDRAW::mipmap; DDRAW::mipmap = 0;

			image.dwHeight = image.dwWidth = 300;
			image.dwFlags|=DDSD_CAPS; //2021: Lock SHOULD include this
			image.ddsCaps.dwCaps|=DDSCAPS_TEXTURE; 
			image.ddsCaps.dwCaps&=~(DDSCAPS_OFFSCREENPLAIN|DDSCAPS_SYSTEMMEMORY); 
			DDRAW::DirectDraw7->CreateSurface(&image,&som_status_autolock2,0);			
			assert(som_status_autolock2);

			DDRAW::mipmap = swap;
		}

		DX::DDBLTFX fx = {sizeof(fx)}; fx.dwFillColor = 0;
		som_status_autolock2->Blt(0,0,0,DDBLT_COLORFILL,&fx);
		som_status_autolock2->Lock(0,&image,0,0); 
		assert(image.lpSurface);

		//HACK: need to suppress TEXTURE0_NOCLIP shaders but
		//calling updating_texture(2) is overkill and it's not
		//obvious where is a good place to call it
		if(DDRAW::compat) //queryX?
		((DDRAW::IDirectDrawSurface7*)som_status_autolock2)->queryX->knockouts = 1;
	}

	const int bytes = 4;
	assert(bytes==image.ddpfPixelFormat.dwRGBBitCount/8);
	BYTE *src = (BYTE*)icons.lpSurface+i*bytes+j*icons.lPitch;	
	BYTE *dst = (BYTE*)image.lpSurface+x*bytes+y*image.lPitch; 	
	for(int k=3;k-->0;dst+=image.lPitch,src+=icons.lPitch)
	{
		//memcpy(dst,src,3*bytes); //3x3
	//	for(int i=3*bytes;i-->0;)
	//	dst[i] = max(dst[i],src[i]); //2024: trying to show layers?
		for(int i=3,j=0;i-->0;j+=4)
		{
			dst[j+0] = max(dst[j+0],src[j+0]>>sh);
			dst[j+1] = max(dst[j+1],src[j+1]>>sh);
			dst[j+2] = max(dst[j+2],src[j+2]>>sh);
			dst[j+3] = max(dst[j+3],src[j+3]);
		}

	}
}

static unsigned char som_status_map_edge[4];

static int som_status_map_x(RECT &map)
{
	if(0!=EX::context())
	{
		int x = 3*SOM::height*0.9f/300;
		SetRect(&map,-50*x,-50*x,50*x,50*x); 
		OffsetRect(&map,SOM::width/2,SOM::height/2);
		return x;
	}
	else
	{	
		//int x = 3+SOM::mapZ;
		int x = sqrtf(9+abs(SOM::mapZ));
		SetRect(&map,-50*x,-50*x,50*x,50*x);

		int l = 0; int r = 0; 
		l-=som_status_map_edge[0];
		l+=som_status_map_edge[2];
		r-=som_status_map_edge[1];
		r+=som_status_map_edge[3];		
		OffsetRect(&map,l/2*x+SOM::mapX,r/2*x+SOM::mapY);
		return x;
	}  	
}

static void som_status_map(bool automap)
{
	som_MPX &mpx = *SOM::L.mpx->pointer;

	int curl = SOM::mapL; 
	auto &ll = ((SOM::MPX::Layer*)&mpx[SOM::MPX::layer_0])[curl];
	if(!ll.tiles) 
	curl = SOM::mapL = 0; 

	RECT edge = {3*99,3*99,0,0};

	int &ls = mpx[SOM::MPX::layer_selector];
	for(;ls>=-6;ls--)
	{
		auto &l = ((SOM::MPX::Layer*)&mpx[SOM::MPX::layer_0])[ls];
		auto *p = l.tiles;
		if(!p) continue;

		//Reminder: 419020 interleaves these 100 at
		//a time (assuming 100x100 in doing so)
		for(int y=297;y>=0;y-=3)
		for(int x=0;x<300;x+=3,p++) if(0xFFFF!=p->msm)
		{	
			if(automap)
			{
				int rot = p->rotation;
				int ico = p->icon; 
				int i = ico%20*3;
				int j = ico/20*3;
				if((rot&1)==1) i+=64;
				if((rot&2)==2) j+=64;		
				som_status_automap(i,j,x,y,ls!=curl);
			}

			if(x<edge.left) edge.left = x;
			if(y<edge.top) edge.top = y;
			if(x>edge.right) edge.right = x;
			if(y>edge.bottom) edge.bottom = y;
		}
	}
	ls = 0;

	som_status_map_edge[0] = 0xFF&edge.left/3;
	som_status_map_edge[1] = 0xFF&edge.top/3;
	som_status_map_edge[2] = 0xFF&(100-1-edge.right/3);
	som_status_map_edge[3] = 0xFF&(100-1-edge.bottom/3);
}
   
//REMOVE ME?
static int som_status_map_icon_bmp = -1;
extern void som_status_mapmap2__automap()
{	
	//Now a som_status_mapmap2 subroutine...
	assert(!som_status_mapmap);
	//assert(0!=SOM::newmap&&0==EX::context());
	//SOM::automap = SOM::mapmap = SOM::frame;
	//static unsigned build = -1;
	//unsigned hash = DDRAW::noResets<<6|SOM::mpx;
	//if(build!=hash) build = hash;
	//else return;
		
	/*This is done when Yes is chosen. (The item is used.) 
	0041981E 68 10 20 00 00       push        2010h  
	00419823 6A 00                push        0  
	00419825 6A 00                push        0  
	00419827 6A 00                push        0  
	00419829 8D 4C 24 70          lea         ecx,[esp+70h]  
	0041982D 51                   push        ecx  
	0041982E 6A 00                push        0  
	00419830 FF 15 D8 81 45 00    call        dword ptr ds:[4581D8h]  
	00419836 8B F8                mov         edi,eax  
	00419838 85 FF                test        edi,edi  
	0041983A 74 38                je          00419874  
	0041983C 57                   push        edi  
	0041983D 6A 00                push        0  
	0041983F 6A 00                push        0  
	00419841 8D 54 24 6C          lea         edx,[esp+6Ch]  
	00419845 6A 02                push        2  
	00419847 52                   push        edx  
	00419848 E8 13 EE 02 00       call        00448660  
	0041984D 83 C4 14             add         esp,14h  
	00419850 57                   push        edi  
	00419851 89 86 04 01 00 00    mov         dword ptr [esi+104h],eax  
	00419857 FF 15 38 80 45 00    call        dword ptr ds:[458038h]  
	0041985D C6 05 D5 B1 9A 01 01 mov         byte ptr ds:[19AB1D5h],1  
	00419864 B8 01 00 00 00       mov         eax,1 
	*/
	HANDLE map_icon = 0;
	const char *db_string = SOMEX_(A)"\\Data\\Menu\\map_icon.bmp";		
	if(-1==som_status_map_icon_bmp)
	{
		map_icon = LoadImageA(0,db_string,0,0,0,0x2010);	

		reset:

		//2021: som_game_LoadImageA was disabling
		//colorkey and som_status_automap restored
		//it. with colorkey_f this no longer works
		//but I'm unsure what's best
		auto swap = DDRAW::colorkey; //REMOVE ME?
		DDRAW::colorkey = 0;
		{		
			som_status_map_icon_bmp =
			((DWORD(*)(const char*,DWORD,DWORD,DWORD,HANDLE))0x448660)(db_string,2,0,0,map_icon);		
		}
		DDRAW::colorkey = swap;

		//DeleteObject(map_icon);	  
		if(!~som_status_map_icon_bmp) return;
	}
	som_status_mapmap =	SOM::L.textures+som_status_map_icon_bmp;
	if(!som_status_mapmap->texture) goto reset;

	som_status_map(true);
	som_status_autolock->Unlock(0);
	som_status_autolock2->Unlock(0);	   
	assert(som_status_autolock==som_status_mapmap->texture);

	som_status_mapmap = 0; som_status_autolock = 0; //!

	/*NEW: Leaving HANDLE/texture in database
	//00419625 8B 96 04 01 00 00    mov         edx,dword ptr [esi+104h]  
	//0041962B 52                   push        edx  
	//0041962C E8 4F F1 02 00       call        00448780  
	//00419631 C7 86 04 01 00 00 FF FF FF FF mov         dword ptr [esi+104h],0FFFFFFFFh 
	if(((BYTE(*)(DWORD))0x448780)(h))
	{
		//feels wrong
		//*(DWORD*)db_texture = 0xFFFFFFFF; 
		*(DWORD*)&SOM::L.textures[h].texture = 0xFFFFFFFF; 
	}
	else assert(0);*/

	//What does 4487F0 (som_state_automap4487F0) do???
	/*NON-AUTO-MAPS
	0041956F E8 C4 63 03 00       call        0044F938  
	00419574 50                   push        eax  
	00419575 E8 BE 63 03 00       call        0044F938  
	0041957A 50                   push        eax  
	0041957B 8B 86 04 01 00 00    mov         eax,dword ptr [esi+104h]  
	00419581 55                   push        ebp  
	00419582 57                   push        edi  
	00419583 6A 01                push        1  
	00419585 50                   push        eax  
	00419586 E8 15 80 24 52       call        som_state_automap4487F0 (526615A0h)  
	0041958B 83 C4 18             add         esp,18h  
	0041958E 33 FF                xor         edi,edi  
	*/
}
static unsigned som_status_mapmap__build = 0;
extern void som_status_mapmap__free()
{
	if(!som_status_mapmap) return;

	if(!som_status_mapmap__build) //HACK!!
	{
		//REMOVE ME?
		//REMOVE ME?
		//REMOVE ME?
		//som_state_automap4487F0
		som_status_mapmap = 0; return;
	}

	int h = som_status_mapmap-SOM::L.textures; 

	//this is from the automap part of the item menu subroutine
	//but the logic should be similar and eventually formalized
	//00419625 8B 96 04 01 00 00    mov         edx,dword ptr [esi+104h]  
	//0041962B 52                   push        edx  
	//0041962C E8 4F F1 02 00       call        00448780  
	//00419631 C7 86 04 01 00 00 FF FF FF FF mov         dword ptr [esi+104h],0FFFFFFFFh 
	if(((BYTE(*)(DWORD))0x448780)(h))
	{
		//feels wrong
		//*(DWORD*)db_texture = 0xFFFFFFFF; 
		*(DWORD*)&SOM::L.textures[h].texture = 0xFFFFFFFF; 
	}
	else assert(0); //2022: level change with map off?

	som_status_mapmap = 0;
	som_status_mapmap__build = 0;
}
static int som_status_mapmap2(int map, bool rescue=SOM::mapmap<SOM::frame-1)
{		
	som_MPX &mpx = *SOM::L.mpx->pointer;
	
	const char *maps = &mpx[SOM::MPX::maps];

	int mapsN = 1;
	for(int i=0;i<32*3;i+=32) if('\0'!=maps[i])
	mapsN++;

	assert(0!=SOM::newmap&&0==EX::context());
	
	if(map>1&&0==(map-1)%mapsN) return 0; //2020: alt+control to hide map?	

	unsigned &build = som_status_mapmap__build; 
	unsigned hash = DDRAW::noResets<<12|SOM::mpx<<6;

	if(map>0) map-=1;
	if(map==0){ goto automap; } //SKIPPING INITILIAZATION?!
	
	while(map<0){ map+=mapsN; assert(map>=0); }
	map%=mapsN;	
	
	automap: if(build!=(hash|map))
	{
		som_status_mapmap__free();
	}

	if(map==0)
	{
		if(build!=hash||!som_status_autolock2)
		{
			build = hash;
			som_status_mapmap2__automap();
		}
		SOM::automap = SOM::frame; goto built;
	}

	hash|=map; 
	
	if(build==hash&&som_status_mapmap) goto built;

	maps = &mpx[SOM::MPX::maps];	
	for(int i=0,j=1;i<mapsN;i++,maps+=32) if('\0'!=maps[0])
	{
		if(j==map) break; j++;
	}

	char db_string[MAX_PATH] = SOMEX_(A)"\\Data\\Picture\\";
	strcat(db_string,maps);
	HANDLE map_image = 0;	
	if(!som_status_mapmap) //indicates device reset
	map_image = LoadImageA(0,db_string,0,0,0,0x2010);	
	//2 is saved in 4th byte. maybe bytes 5,6,7 too
	//changing to 0, 1, etc. doesn't seem to matter
	DWORD h = ((DWORD(*)(const char*,DWORD,DWORD,DWORD,HANDLE))0x448660)(db_string,2,0,0,map_image);	
	//DeleteObject(map_image); //retained by texture struct
	SOM::Texture &t = SOM::L.textures[h];
	assert(!map_image||map_image==t.mipmaps[0]);
	if(som_status_mapmap)
	{
		//hack: keep from increasing on device reset
	//	t.ref_counter = 1;
		t.ref_counter--; 

		assert(&t==som_status_mapmap);
	}
	else som_status_mapmap = &t; //t.texture;	

	//2021: 0x448660 calls SetColorKey... I'm not sure this was ever required?
	//som_status_mapmap->texture->SetColorKey(DDCKEY_SRCBLT|DDCKEY_DESTBLT,0);	

	build = hash; som_status_map(false);

	built: SOM::mapmap = SOM::frame; 
	
	//HACK? The rest is in case the map is changed in such 
	//a way that no tiles remain available with which to
	//interact. This way Alt+Alt can reset the map...
	if(rescue)
	{
		//Assuming SOM::map is in play.
		RECT map; int x = som_status_map_x(map);
		map.left+=x*som_status_map_edge[0];
		map.top+=x*som_status_map_edge[1];
		map.right-=x*som_status_map_edge[2];
		map.bottom-=x*som_status_map_edge[3];
		//TODO? Investigate individual tiles?
		if(map.left<0)
		{
			SOM::mapX-=map.left;
		}
		else if(map.right>SOM::width)
		{
			SOM::mapX-=map.right-SOM::width;
		}
		if(map.top<0)
		{
			SOM::mapY-=map.top;
		}
		else if(map.bottom>SOM::height)
		{
			SOM::mapY-=map.bottom-SOM::height;
		}
	}
	
	return map+1;	
}

extern void som_status_map_xy(int dx, int dy)
{
	if(!SOM::map||EX::is_captive()) return;

	assert(EX::context()==0);
	
	EX::INI::Player pc;

	RECT map; int x = som_status_map_x(map);
	map.left+=x*som_status_map_edge[0];
	map.top+=x*som_status_map_edge[1];
	map.right-=x*som_status_map_edge[2];
	map.bottom-=x*som_status_map_edge[3];

	bool rb = GetKeyState(VK_RBUTTON)>>15;
	bool drag = rb&&'hand'==EX::pointer;

	POINT xy = {EX::x,EX::y};	
	//TODO? support stereo mode
	bool hover = !DDRAW::inStereo&&PtInRect(&map,xy); 	
	if(hover)
	{	
		//assuming 3 is the size of the automap
		xy.x-=map.left; xy.y-=map.top;
		xy.x/=x; xy.y/=x;
		xy.x+=som_status_map_edge[0];
		xy.y+=som_status_map_edge[1]; 		
		xy.y = 100-1-xy.y;

		//EX::dbgmsg("xy %d %d",xy.x,xy.y);

		bool hover2 = false;

		const float r = pc->player_character_shape;
		const float h = pc->player_character_height;

		bool warp = !rb&&GetKeyState(VK_LBUTTON)>>15&&SOM::warped!=SOM::frame;		
		
		float elevation = -10000;

		som_MPX &mpx = *SOM::L.mpx->pointer;
		int &ls = mpx[SOM::MPX::layer_selector];
		for(int pass=1;pass<=2;pass++)
		for(ls=0;ls>=-6;ls--)
		{
			if(pass==1&&ls!=SOM::mapL) continue;

			auto &l = ((SOM::MPX::Layer*)&mpx[SOM::MPX::layer_0])[ls];
			auto *p = l.tiles;
			if(!p) continue;

			p+=xy.x+xy.y*100; //assuming 100x100!

			if(p<p+10000)
			{
				if(0xffff==p->msm) continue; 
			}
			else assert(0);

			hover2 = true;

			//find heighest MHM polygon?
			SOM::L.shape = *SOM::stool = r;
			float xyz[3] = { 2*xy.x-1,p->elevation-h*2,2*xy.y-1 };						
			xyz[0]+=(EX::x-map.left)%x/float(x)*2;
			xyz[2]-=(EX::y-map.top)%x/float(x)*2;
			//HACK? I DON'T UNDERSTAND THIS. Can be Z is just not centered
			xyz[2]+=2; 

			int swap = ls; ls = 0; //YUCK

			//NOTE: the range is -2*h,h so to drop down into moats but not 
			//to go so high to jump onto the roofs of man scale structures
			if(SOM::clipper.clip(xyz,h*3,r,14,0.000001f))						
			xyz[1] = max(SOM::clipper.floor,SOM::clipper.slopefloor)+0.001f;
			else xyz[1] = p->elevation;

			elevation = max(elevation,xyz[1]);

			ls = swap;

			if(pass==1) break; //mapL?
		}
		ls = 0;

		if(!hover2) hover = false;

		//find heighest object top?
		if(hover&&warp)
		{			
			SOM::L.shape = *SOM::stool = r;
			float xyz[3] = { 2*xy.x-1,elevation-h*2,2*xy.y-1 };						
			xyz[0]+=(EX::x-map.left)%x/float(x)*2;
			xyz[2]-=(EX::y-map.top)%x/float(x)*2;
			//HACK? I DON'T UNDERSTAND THIS. Can be Z is just not centered
			xyz[2]+=2; 
			xyz[1] = elevation;
			
			float _[3];
			for(int i=SOM::L.ai3_size;i-->0;)
			if(((BYTE(__cdecl*)(FLOAT*,FLOAT,FLOAT,DWORD,DWORD,FLOAT*,FLOAT*))0x40DFF0)
			(xyz,h,r,i,0,_,_))
			{
				auto &ai = SOM::L.ai3[i];
				float y = ai[SOM::AI::y3];
				float h = ai[SOM::AI::height3];				
				xyz[1] = max(xyz[1],y+h+0.001f);
				//EX::dbgmsg("obj %f--%f--%f %f--%f--%f",pos[0],pos[1],pos[2],box[0],box[1],box[2]);
			}	
			memcpy(SOM::L.pcstate,xyz,sizeof(xyz));
			//if(dx|dy) //follow mouse?
			{
				//xyz[0] = dx; xyz[1] = 0; xyz[2] = dy;
				MOUSEMOVEPOINT mmp[8], wtf = //WEIRD???
				{GET_X_LPARAM(GetMessagePos()),GET_Y_LPARAM(GetMessagePos()),GetMessageTime()};
				int n = GetMouseMovePointsEx(sizeof(wtf),&wtf,mmp,8,GMMP_USE_DISPLAY_POINTS);				
				xyz[0] = 0; xyz[1] = 0; xyz[2] = 0;
				float t = mmp[n-1].time-mmp[0].time; 				
				for(int i=1;i<n;i++)
				{
					//can't tell if w helps or not??
					float w = (mmp[i].time-mmp[i-1].time)/t;
					xyz[0]+=w*(mmp[i].x-mmp[i-1].x);
					xyz[2]+=w*(mmp[i].y-mmp[i-1].y);
				}
				//HACK: normalize...
				((void(*)(float*,float*,float*))0x4466c0)(xyz+0,xyz+1,xyz+2);
				SOM::L.pcstate[4] = atan2(xyz[0],xyz[2]);
			}
			float hack = xyz[3];
			SOM::warp(SOM::L.pcstate,SOM::L.pcstate2);
			xyz[3] = hack;
		}
	}
								   
	if(!hover)
	{
		if('hand'==EX::pointer)
		{
			if(!rb) EX::pointer = 0;
		}
	}
	else if(0==EX::pointer||'z'==EX::pointer)
	{
		//EX::pointer = 'hand';

		if(!EX::active) //2020
		{
			EX::activating_cursor();
			EX::abandoning_cursor();
		}
		EX::showing_cursor('hand');
		EX::holding_cursor();
	}	

	bool lock = false;
	if(drag&&0!=EX::pointer&&dx|dy)
	{
		if(GetKeyState(VK_LBUTTON)>>15)
		{
			lock = true;			
		}
		else recenter:
		{
			SOM::mapX = SOM::mapX-dx;		
			SOM::mapY = SOM::mapY-dy;
		}
	}
	
	//FINAL
	//HACK: using tools style zoom direction...
	dy = -dy;

	static DWORD mp = -1; if(lock)
	{
		if(mp==DWORD(-1)) mp = GetMessagePos();
		xy.x = GET_X_LPARAM(mp);
		xy.y = GET_Y_LPARAM(mp);		
		//SetCursorPos is completely unreliable
		//SetCursorPos(xy.x,xy.y);
		ScreenToClient(SOM::window,&xy);

		//just use mp as the point of reference
		dy+=abs(SOM::mapZ);
		dy = max(1,min(99,dy));
		SOM::mapZ = SOM::mapZ<0?-dy:dy;		
		int x2 = sqrtf(9+dy); if(x!=x2);
		{
			float t = float(x2)/x;
			dx = xy.x-SOM::mapX; dx = t*dx-dx+0.5f;
			dy = xy.y-SOM::mapY; dy = t*dy-dy+0.5f;
					  
			lock = false; goto recenter;
		}
	}
	else mp = -1; //FINAL
}

extern void SOM::Map(void *d3dtlvertices)
{	
	if(SOM::mapmap!=SOM::frame) return;	

	DX::D3DTLVERTEX *p = (DX::D3DTLVERTEX*)d3dtlvertices;

	if(SOM::map&&0==EX::context()
	||EX::INI::Bugfix()->do_fix_automap_graphic) 
	{
		if(som_status_autolock)
		{
			//acknowledge first frame
			SOM::automap = SOM::frame;
			som_status_autolock->Unlock(0);
			som_status_autolock2->Unlock(0);	   
			som_status_autolock = 0;			
		}
		
		RECT map; int x = som_status_map_x(map);
	
		DX::IDirectDrawSurface7 *getexture = //hack 
		DDRAW::TextureSurface7[0], *texture = som_status_autolock2;
		if(SOM::automap!=SOM::frame) texture = som_status_mapmap->texture;				
		if(0) //TODO? texture->Blt is more like som_db.exe!
		{
			//SetColorKey?
			DDRAW::BackBufferSurface7->Blt(&map,texture,0,0,0);
		}
		else
		{
			DWORD fvf = D3DFVF_TLVERTEX, c = 0xFFFFFFFF;

			//50% transparency is sometimes difficult to see
			if(SOM::mapZ<0&&0==EX::context()) c = 0xA0FFFFFF;

			/*2020
			//I can't make this pixel perfect for a repeating
			//pattern
			//maybe the fullscreen effects (dx_d3d9c_backblt)  
			//blit is wrong
			float h = 0.5f;
			DX::DDSURFACEDESC2 yuck;
			yuck.dwSize = sizeof(DX::DDSURFACEDESC2);
			texture->GetSurfaceDesc(&yuck);
			float s = 0.5f/yuck.dwWidth;
			float t = 0.5f/yuck.dwHeight;
			float fan[4*8] =
			{
				h+map.left,h+map.top,0,1,*(float*)&c,0,s,t,
				h+map.right,h+map.top,0,1,*(float*)&c,0,1+s,t,
				h+map.right,h+map.bottom,0,1,*(float*)&c,0,1+s,1+t,
				h+map.left,h+map.bottom,0,1,*(float*)&c,0,s,1+t,
			};*/
			float fan[4*8] =
			{
				map.left,map.top,0,1,*(float*)&c,0,0,0,
				map.right,map.top,0,1,*(float*)&c,0,1,0,
				map.right,map.bottom,0,1,*(float*)&c,0,1,1,
				map.left,map.bottom,0,1,*(float*)&c,0,0,1,
			};
			if(0==EX::context()) for(int i=0;i<4*8;i+=8)
			{
				fan[i+0] = fan[i+0]/DDRAW::xyScaling[0]-DDRAW::xyMapping[0];
				fan[i+1] = fan[i+1]/DDRAW::xyScaling[1]-DDRAW::xyMapping[1];				
			}

			DDRAW::Direct3DDevice7->SetTexture(0,texture);		
 			DDRAW::Direct3DDevice7->DrawPrimitive(DX::D3DPT_TRIANGLEFAN,fvf,fan,4,0);				
			DDRAW::Direct3DDevice7->SetTexture(0,getexture);
		}
									   
		//0.5f is because the first tile goes from -1 to 1. Not 0 to 2.
		float px = 0.5f+SOM::xyz[0]/2;
		float pz = 0.5f+SOM::xyz[2]/2;
		float pr = x/2.0f; 

		px = map.left+px*x; 
		pz = map.bottom-pz*x; //Y/Z is reversed

		p[0].sx = px-pr; p[0].sy = pz-pr;
		p[1].sx = px+pr; p[1].sy = pz-pr;
		p[2].sx = px+pr; p[2].sy = pz+pr;
		p[3].sx = px-pr; p[3].sy = pz+pr;		
	}
	else assert(!som_status_autolock);

	//todo: convert square marker into 2x arrow

	float w = p[1].sx-p[0].sx, cx = p[0].sx+w/2;
	float h = p[2].sy-p[0].sy, cy = p[0].sy+h/2;

	if(w<6) w = h = 6; //3x3 is too small
	
	//h*=1.5f;
	//h*=1.333333f;	
	h*=1.414213f; //sqrt(2)
	//h*=1.618033f; //golden ratio
	p[0].sx = cx; p[0].sy = cy-h;
	p[1].sx = cx+w; p[1].sy = cy+h;
	p[2].sx = cx; p[2].sy = cy+h; 
	p[3].sx = cx-w; p[3].sy = cy+h;

	((BYTE*)&p[2].color)[3]/=3; //cleft

	float c = cosf(SOM::uvw[1]), s = sinf(SOM::uvw[1]);

	for(int i=0;i<4;i++) //rotate the arrow
	{
		float xx = p[i].sx-cx, yy = p[i].sy-cy;

		p[i].sx = cx+xx*c+yy*s; p[i].sy = cy+xx*-s+yy*c;

		if(0==EX::context()) 
		{
			p[i].sx = p[i].sx/DDRAW::xyScaling[0]-DDRAW::xyMapping[0];
			p[i].sy = p[i].sy/DDRAW::xyScaling[1]-DDRAW::xyMapping[1];				
		}
	}	
}
extern void SOM::Map2(void *save)
{
	DX::D3DTLVERTEX p[4],swap[4];
	
	if(SOM::map&&0==EX::context()) 
	{	
		if(SOM::mapmap==SOM::frame) return;

		if(save) memcpy(swap,save,sizeof(swap));

		//this is selecting/wrapping around map
		SOM::map = som_status_mapmap2(SOM::map);

		if(!SOM::map) return; //2020: alt+ctrl hid map?

		DDRAW::IDirect3DDevice7 *d = DDRAW::Direct3DDevice7;
		d->SetRenderState(DX::D3DRENDERSTATE_ALPHABLENDENABLE,1);	
		d->SetRenderState(DX::D3DRENDERSTATE_SRCBLEND,DX::D3DBLEND_SRCALPHA);		
		d->SetRenderState(DX::D3DRENDERSTATE_DESTBLEND,DX::D3DBLEND_INVSRCALPHA);
		d->SetTextureStageState(0,DX::D3DTSS_ALPHAOP,DX::D3DTOP_SELECTARG2);
		{
			//this code copies the alpha value, but esp+100h is hard to 
			//trace back to the source. It's easier to fudge it for now
			//00448B22 D9 84 24 00 01 00 00 fld         dword ptr [esp+100h]
			memset(&p,0x00,sizeof(p));
//REMINDER: 1D11DC0 seems to hold the pulse for shops
//			menus must have one too
			//don't know if the blink function is a sine wave. 400 is a
			//little slow or fast, but close enough. the wave may be off
			//float s = sinf(EX::tick()%400/400.0f*M_PI);			
			float a = cosf(EX::tick()%400/400.0f*M_PI*2)/2+0.5f;
			DX::D3DCOLOR c = a*0x7E; c = c+0x7F<<24|0xFF;
			for(int i=0;i<4;i++){ p[i].color = c; p[i].rhw = 1; }

			SOM::Map(p); 			
								
			DX::LPDIRECTDRAWSURFACE7 gt; //for magical partical effects?
			if(save) d->GetTexture(0,&gt); assert(!save||!gt);
			d->SetTexture(0,0); 			
			d->DrawPrimitive(DX::D3DPT_TRIANGLEFAN,D3DFVF_TLVERTEX,p,4,0);			
			if(save) d->SetTexture(0,gt);
		}
		//*(DWORD*)0x1D6A24C //som_scene_state		
		d->SetRenderState(DX::D3DRENDERSTATE_ALPHABLENDENABLE,*(DWORD*)0x1D6A24C);
		d->SetRenderState(DX::D3DRENDERSTATE_SRCBLEND,*(DWORD*)0x1D6A258);
		d->SetRenderState(DX::D3DRENDERSTATE_DESTBLEND,*(DWORD*)0x1D6A25C);
		d->SetTextureStageState(0,DX::D3DTSS_ALPHAOP,*(DWORD*)0x1D6A26C);

		if(save) memcpy(save,swap,sizeof(swap));
	}
	else if('hand'==EX::pointer) EX::pointer = 0; //hack
		
	//REMOVE ME?
	if(som_status_autolock){ assert(0); SOM::Map(p); } //not good	
}

static void som_status_oneffects() //ensure icons are unlocked
{
	som_status_oneffects_passthru();

	SOM::Map2(0); //draw SOM::map if a fade/tint isn't present
}

static void som_status_onreset()
{
	som_status_onreset_passthru();	

	if(som_status_autolock2)
	{
		som_status_autolock2->Release();
		som_status_autolock2 = 0;
	}

	//MEMORY LEAK?
	//#error need to learn more about SOM::menupcs
	int todolist[SOMEX_VNUMBER<=0x10203024UL];

	//release textures for King's Field II 
	//style status bar?
	auto men = (DWORD*)0x1a5b400;
	if(men[42]&&men[42]!=0xFFFFFFFF)
	{
		//this tries to access memory? function pointers
		//actually, that are 0
		//((void(__cdecl*)(void*))0x4214c0)(men);
		((void(__cdecl*)(void*))0x421910)(&men[0x2a]);
	}
}