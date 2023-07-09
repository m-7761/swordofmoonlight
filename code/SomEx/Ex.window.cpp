
#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

#include "math.h" //fabsf

#include "Ex.ini.h"
#include "Ex.movie.h"
#include "Ex.input.h"
#include "Ex.output.h"	
#include "Ex.cursor.h"
#include "Ex.window.h"

extern HWND EX::window = 0;
extern HWND EX::client = 0;
extern RECT EX::adjust = {0,0,0,0};
extern RECT EX::coords = {0,0,0,0};
extern RECT EX::screen = {0,0,0,0};
extern DWORD EX::style[2] = {WS_OVERLAPPEDWINDOW,0};
extern DWORD EX::multihead = 0; 
extern bool EX::fullscreen = false;
extern const wchar_t *EX::caption = 0;

static int Ex_window_visible = 0;
static RECT Ex_window_adjusted = {0,0,0,0};
static ATOM Ex_window_class_registered = 0;
static LRESULT CALLBACK Ex_window_proc(HWND,UINT,WPARAM,LPARAM);
static WNDCLASSW Ex_window_class = 
{0,Ex_window_proc,0,0,0,0,0,(HBRUSH)GetStockObject(BLACK_BRUSH),0,L"Ex window class"};

static DWORD Ex_window_display(HMONITOR hmonitor)
{	
	//Note: this code is also maintained in dx_draw_display(HMONITOR)

	if(!hmonitor) return 0;
	static DWORD cacheval = 0;
	static HMONITOR cachehit = 0; 
	if(hmonitor==cachehit) return cacheval;

	MONITORINFOEXA mon; 
	mon.cbSize = sizeof(mon); //non-POD	
	DEVMODEA dm = { "",0,0,sizeof(dm),0 };	  
	DISPLAY_DEVICEA dev = { sizeof(dev) };
	
	if(GetMonitorInfoA(hmonitor,&mon))
	{
		if(0&&mon.dwFlags&MONITORINFOF_PRIMARY)
		{
			cacheval = 0; cachehit = hmonitor;
		}
		else for(DWORD i=0;EnumDisplayDevicesA(0,i,&dev,0);i++)
		{
			//Note: ChangeDisplaySettings (sans Ex) does not expose DM_POSITION 
			if(EnumDisplaySettingsExA(dev.DeviceName,ENUM_CURRENT_SETTINGS,&dm,0))
			{
				if(dm.dmPosition.x==mon.rcMonitor.left&&
					dm.dmPosition.y==mon.rcMonitor.top) //should be enough
				{
					//Note: if holds true, can simplify all of this
					assert(!strcmp(mon.szDevice,dev.DeviceName)); 

					assert(dm.dmPosition.x+dm.dmPelsWidth==mon.rcMonitor.right);
					assert(dm.dmPosition.y+dm.dmPelsHeight==mon.rcMonitor.bottom);

					cacheval = i+1; cachehit = hmonitor;
					
					return cacheval;
				}
			}
		}
	}

	return 0;
}

static bool Ex_window_correct_placement(int x=CW_USEDEFAULT, int y=CW_USEDEFAULT)
{
	//REMINDER: I THINK MAYBE THIS IS THE SOURCE OF SOME
	//RECENT PROBLEMS WITH RESTYLING A FULLSCREEN WINDOW

//2020: Windows 10 has problems with fullscreen so it's necessary
//to make a window with a frame and position it at negative origin
//so the 32,32 stuff has to go, I guess... should test the center
//maybe

	HMONITOR hm = MonitorFromWindow
	(EX::window,MONITOR_DEFAULTTONULL);
	MONITORINFO mon = {sizeof(mon)};
	if(!hm||!GetMonitorInfo(hm,&mon)) return false;
			
	if(x==CW_USEDEFAULT)
	{
		RECT temp; 
		if(GetWindowRect(EX::window,&temp))
		{
			x = temp.left-mon.rcMonitor.left;
			y = temp.top-mon.rcMonitor.top;
		}
//		else x = y = 32; //apply arbitrary offset			
	}		

	//TODO: ensure entire window is inside display bounds
//	if(x<0||x>EX::screen.right-EX::screen.left) x = 32; //paranioa
//	if(y<0||y>EX::screen.bottom-EX::screen.top) y = 32; //paranioa

	x+=EX::screen.left; y+=EX::screen.top;

	int w = EX::coords.right-EX::coords.left;
	int h = EX::coords.bottom-EX::coords.top;

	if(!MoveWindow(EX::window,x,y,w,h,false))
	return false;
	GetWindowRect(EX::window,&EX::coords); 
	return true;
}

extern void EX::letterbox(HMONITOR hmonitor, DWORD &w, DWORD &h)
{
	MONITORINFO info = {sizeof(info)};
	if(!GetMonitorInfo(hmonitor,&info)) 
	return;

	float aspect = info.rcMonitor.right-info.rcMonitor.left;
	aspect/=info.rcMonitor.bottom-info.rcMonitor.top;
	if(fabsf(aspect-float(w)/h)<0.00003) 
	return; //1:1	

	DWORD display = Ex_window_display(hmonitor);

	DISPLAY_DEVICEA dev = { sizeof(dev) };
		
	const char *devname = 0; 

	if(display)
	{	
		dev.DeviceName[0] = '\0'; 
		EnumDisplayDevicesA(0,display-1,&dev,0);
		devname = *dev.DeviceName?dev.DeviceName:0;
	}

	DEVMODEA dm = { "",0,0,sizeof(dm),0 };

	//warning: may not handle rotated displays correctly
	for(DWORD i=0;EnumDisplaySettingsA(devname,i,&dm);i++)
	{	
		if(dm.dmPelsWidth<w||dm.dmPelsHeight<h) 
		continue;	
		float cmp = fabs(aspect-float(dm.dmPelsWidth)/dm.dmPelsHeight);
		if(cmp<0.00003) //best fit with square pixels
		{
			w = dm.dmPelsWidth; h = dm.dmPelsHeight; 
			return;
		}		
	}
}

static void Ex_window_move_to_screen()
{
	if(EX::window)
	{
		RECT test; 
		if(!IntersectRect(&test,&EX::coords,&EX::screen)
		||test.top!=EX::coords.top||test.bottom!=EX::coords.bottom
		||test.left!=EX::coords.left||test.right!=EX::coords.right)
		{
			if(!Ex_window_correct_placement()) 			
			SetWindowPos(EX::window,0,EX::screen.left,EX::screen.top,
			0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
		}
	}
}
extern void EX::capturing_screen(HMONITOR hmonitor)
{	
	if(hmonitor)
	EX::multihead = Ex_window_display(hmonitor); 

	DEVMODEA dm = { "",0,0,sizeof(dm),0 };	  
	DISPLAY_DEVICEA dev = { sizeof(dev) };

	const char *devname = 0; if(EX::multihead)
	{	
		dev.DeviceName[0] = '\0'; 		  
		EnumDisplayDevicesA(0,EX::multihead-1,&dev,0);	  
		devname = *dev.DeviceName?dev.DeviceName:0;
	}

	//////////////////////////////////////////////////////////////////////

	if(EnumDisplaySettingsExA(devname,ENUM_CURRENT_SETTINGS,&dm,0)) 
	{
		EX::screen.top = dm.dmFields&DM_POSITION?dm.dmPosition.y:0; 
		EX::screen.left = dm.dmFields&DM_POSITION?dm.dmPosition.x:0; 		
		EX::screen.right = dm.dmFields&DM_PELSWIDTH?dm.dmPelsWidth:0;
		EX::screen.bottom = dm.dmFields&DM_PELSHEIGHT?dm.dmPelsHeight:0;

		EX::screen.right+=EX::screen.left;
		EX::screen.bottom+=EX::screen.top;
	}

	//TODO: MAYBE THIS SHOULDN'T BE AUTOMATIC?
	//
	// REMINDER: I THINK MAYBE THIS IS THE SOURCE OF SOME
	// RECENT PROBLEMS WITH RESTYLING A FULLSCREEN WINDOW
	//
	Ex_window_move_to_screen();
}

//REMOVE ME (somehow)
namespace DDRAW{ extern bool (*onFlip)(); }
static bool (*Ex_window_onflip_passthru)() = 0;
static bool Ex_window_onflip()
{
	bool out = Ex_window_onflip_passthru();

	static int now = 0;
	if(now++) EX::showing_window();
	if(!now) now++; //paranoia

	return out;
}

static void Ex_window_center_client()
{
	if(EX::window&&EX::client)
	{	
		RECT c,w; assert(EX::client!=EX::window);
		if(GetClientRect(EX::client,&c)&&GetClientRect(EX::window,&w))
		{
			int x = (w.right-c.right)/2;
			int y = (w.bottom-c.bottom)/2;
			SetWindowPos(EX::client,0,x,y,0,0,SWP_NOSIZE);
		}
	}
}

extern void EX::creating_window(HMONITOR monitor, int x, int y)
{				
	//hack: letting be recreated for Microsoft bug workaround
	//https://social.msdn.microsoft.com/Forums/en-US/7743d566-7360-4394-ac72-02f748479c4e/
	//if(EX::window) //return;
	HWND old = EX::window; 
	if(old) SetWindowLong(old,GWLP_WNDPROC,(LONG)DefWindowProcW);	
	if(old) SetParent(EX::client,0);
	
	static LONG icon = GetClassLong(EX::client,GCL_HICON);
	static LONG iconsm = GetClassLong(EX::client,GCL_HICONSM);
									  
	//EXLOG_LEVEL(7) << "Creating Ex Window\n";

	if(!Ex_window_class_registered)
	Ex_window_class_registered = RegisterClassW(&Ex_window_class);

	wchar_t title[128] = L"";
	GetWindowTextW(EX::client,title,128);
	//2019: Windows 10 no longer supports icons on windows created
	//initially without a title bar
	DWORD style = EX::style[0/*EX::fullscreen*/];
	if(EX::fullscreen) style|=WS_THICKFRAME; //Windows 10

	EX::capturing_screen(monitor);

//	if(EX::fullscreen)
//	EX::adjust = EX::screen; //2019
//	else
	if(EX::client)
	GetClientRect(EX::client,&EX::adjust);
	AdjustWindowRect(&EX::adjust,style,0);	
	Ex_window_adjusted = EX::adjust;

	int w = EX::adjust.right-EX::adjust.left;
	int h = EX::adjust.bottom-EX::adjust.top;
	
	EX::window = CreateWindowEx
	(WS_EX_APPWINDOW,Ex_window_class.lpszClassName,title,style,x,y,w,h,0,0,0,0);
	if(EX::window) GetWindowRect(EX::window,&EX::coords);
	else goto old; //return; //paranoia
		
	//2019: chicken and egg refactor
	//EX::capturing_screen(monitor);
	Ex_window_move_to_screen();

	if(!Ex_window_onflip_passthru)
	{
		Ex_window_onflip_passthru = DDRAW::onFlip;
		DDRAW::onFlip = Ex_window_onflip;
	}

	SetClassLong(EX::window,GCL_HICON,icon);
	SetClassLong(EX::window,GCL_HICONSM,iconsm);

	DWORD cs = GetWindowStyle(EX::client);
	SetWindowExStyle(EX::client,WS_EX_APPWINDOW|WS_EX_DLGMODALFRAME,0);
	//note: assuming undecorated parent window
	cs|=WS_CHILD; cs&=~(WS_CLIPCHILDREN|WS_POPUP);
	SetParent(EX::client,EX::window); SetWindowStyle(EX::client,-1,cs); 

	assert(EqualRect(&Ex_window_adjusted,&EX::adjust));
	//assert(EX::style[EX::fullscreen]==GetWindowStyle(EX::window));	
	if(!EX::fullscreen)
	EX::style[0/*EX::fullscreen*/] = GetWindowStyle(EX::window);	
	
	//2020: window isn't brought to front ... icon is missing from
	//taskbar. probably a temporary (typical) Windows Update issue
	if(old) EX::showing_window(); 
old:if(old) DestroyWindow(old);
}

void EX::showing_window()
{
	if(Ex_window_visible) return;
	ShowWindow(EX::client,SW_SHOWNORMAL);
	ShowWindow(EX::window,SW_SHOWNORMAL); 
	ShowWindow(EX::client,SW_SHOW);
	ShowWindow(EX::window,SW_SHOW);
	Ex_window_visible = WS_VISIBLE;
	
		//2020: took me forever to figure
		//out what was showing the cursor
		//for some reason it doesn't hide
		//EX::showing_cursor();

	SetForegroundWindow(EX::window);
}

extern void EX::destroying_window()
{
	if(!EX::window) return;
	DestroyWindow(EX::window);
	EX::window = 0;	Ex_window_visible = 0;
}

extern void EX::adjusting_window(int w, int h)
{
	if(!EX::window) return;

	//2019
	if(EX::fullscreen) w = EX::screen.right-EX::screen.left; 
	if(EX::fullscreen) h = EX::screen.bottom-EX::screen.top; 

	DWORD style = EX::style[EX::fullscreen]|Ex_window_visible;

	RECT in = {0,0,w,h}; if(!AdjustWindowRect(&in,style,0))
	{				 
		assert(0); return; //unimplemetned
	}								   

	//Note: Ex_window_adjusted is used (instead of EX::adjust) because
	//EX::adjust would be undefined after EX::styling_window() but the
	//state gets updated anyway by EX::styling_window for clarity sake

	if(in.left==Ex_window_adjusted.left&&in.top==Ex_window_adjusted.top
	&&in.right==Ex_window_adjusted.right&&in.bottom==Ex_window_adjusted.bottom)
	return; //redundant adjustment

	w = in.right-in.left; h = in.bottom-in.top; //changing to window coords!
		
	if(EX::fullscreen)
	{	
		EX::coords.top = EX::screen.top+in.top;
		EX::coords.left = EX::screen.left+in.left;	
		EX::coords.right = EX::screen.left+in.right;
		EX::coords.bottom = EX::screen.top+in.bottom;		
		
		int x = EX::coords.left, y = EX::coords.top;
		SetWindowPos(EX::window,0,x,y,w,h,SWP_NOZORDER);
	}
	else
	{
		int x = EX::coords.left, y = EX::coords.top;		
											   
		if(1) //TODO: default_window_position ini setting
		{
			//Note: there is no API for default positioning an existing window?!

			//if(Ex_window_visible)
			//ShowWindow(EX::window,SW_HIDE); //make room on desktop for new shape

			DWORD hidden = EX::style[EX::fullscreen]&~WS_VISIBLE;

			HWND usedefault = CreateWindowW
			(Ex_window_class.lpszClassName,L"CW_USEDEFAULT",hidden,CW_USEDEFAULT,CW_USEDEFAULT,w,h,0,0,0,0);

			RECT temp; if(GetWindowRect(usedefault,&temp))
			{
				x = temp.left; y = temp.top;
			}

			////NOTICE: this needs the same multihead support seen in creating_window ////

			DestroyWindow(usedefault);
		}

		SetWindowPos(EX::window,0,x,y,w,h,SWP_NOZORDER);		
		GetWindowRect(EX::window,&EX::coords);

		if(EX::multihead)
		{
			RECT test; 
			if(!IntersectRect(&test,&EX::coords,&EX::screen)
			||test.top!=EX::coords.top||test.bottom!=EX::coords.bottom
			||test.left!=EX::coords.left||test.right!=EX::coords.right)
			{	 
				if(!Ex_window_correct_placement()) assert(0);
			}
		}
	}

	EX::adjust = Ex_window_adjusted = in;
}

extern void EX::styling_window(bool fullscreen, int x, int y)
{
	if(!EX::window) return;

	DWORD style = EX::style[fullscreen]|Ex_window_visible;
	
	//Windows 10 dead client area bug after exiting fullscreen
	//NOTE: my goal here was to not appear to be fullscreen to
	//the window manager so the bug would not apply
	//this may make the application not appear to be fullscreen
	//but oddly the frame is not rendered, so it looks as if so
	//(I think the frame may be outside of the screen)
	if(fullscreen) style|=WS_THICKFRAME;

	if(GetWindowStyle(EX::window)==style)
	{
		EX::fullscreen = fullscreen; 
				
		goto center; //return; //2019
	}
	
	if(fullscreen) EX::coords = EX::screen;		
	if(fullscreen||GetWindowRect(EX::client,&EX::coords))
	if(AdjustWindowRect(&EX::coords,style,0))
	{
		//REMINDER: important to set before generating WM_MOVE below
		//so message handling code can check for fullscreen status
		EX::fullscreen = fullscreen;

		SetWindowLong(EX::window,GWL_STYLE,style);
				  
		EX::style[fullscreen] =	GetWindowStyle(EX::window);

		//2019: looks like an error???
		//GetClientRect(EX::client,&EX::adjust);
		GetClientRect(EX::window,&EX::adjust);		
		AdjustWindowRect(&EX::adjust,style,0);

		//2018: restore to a client saved position?
		//I dunno, seems better to keep it centered
		//but it's just too complicated to not save
		//NOTE: a way to center is to maximize then
		//drag the window down by its titlebar area
		if(!fullscreen)
		OffsetRect(&EX::coords,x-coords.left,y-coords.top);

		int w = EX::coords.right-EX::coords.left;
		int h = EX::coords.bottom-EX::coords.top;		
		MoveWindow(EX::window,EX::coords.left,EX::coords.top,w,h,true);		

		//NEW: support F11 style fullscreen toggle
		if(fullscreen) ShowWindow(EX::window,SW_MAXIMIZE);					
	}
	assert(EX::fullscreen==fullscreen);

	center: Ex_window_center_client();
}

static LRESULT CALLBACK Ex_client_proc(HWND hwnd, UINT msg, WPARAM w, LPARAM l)
{
	assert(hwnd&&hwnd==EX::client);

	if(!hwnd||hwnd!=EX::client) return 0; //I guess
	
	WNDPROC wp = (WNDPROC)GetWindowLongPtrA(hwnd,GWLP_WNDPROC);
	return wp?CallWindowProcA(wp,hwnd,msg,w,l):0;
}

static LRESULT CALLBACK Ex_window_proc(HWND hwnd, UINT msg, WPARAM w, LPARAM l)
{
	if(hwnd!=EX::window) //WM_GETMINMAXINFO WM_NCCREATE WM_NCCALCSIZE WM_CREATE
	{
		return DefWindowProcW(hwnd,msg,w,l); 
	}

	switch(msg) 
	{
	case WM_INPUT: 
	{
		//NOTE: Ex_cursor_capture does RegisterRawInputDevices

		UINT dwSize = sizeof(RAWINPUT);
		static BYTE lpb[sizeof(RAWINPUT)];

		GetRawInputData((HRAWINPUT)l,RID_INPUT,lpb,&dwSize,sizeof(RAWINPUTHEADER));
		RAWINPUT *raw = (RAWINPUT*)lpb;

		if(raw->header.dwType==RIM_TYPEMOUSE) 
		{
			EX::Joypad::wm_input[0]+=raw->data.mouse.lLastX;
			EX::Joypad::wm_input[1]+=raw->data.mouse.lLastY;
		} 
		break;
	}
	case WM_SYSCHAR:
	case WM_SYSKEYUP: 
	case WM_SYSKEYDOWN: 
					
		if(GetMenu(hwnd)) break;	

		//NOTE: som_db actively suppresses this
		//passing it to the client window would
		//require it to implement WM_SYSCOMMAND
		//WM_SYSCOMMAND?
		if(w==VK_SPACE) break; //Alt+Space menu

		//doesn't send WM_SYSKEYUP?!
		//if(w==VK_F10) goto client; //SOM::f10

		return 0; //snuff out alt combo beeps
		
	case WM_SIZE:
	//case WM_SHOWWINDOW: never fires on min/maximize??
		
		if(Ex_window_visible) switch(w)
		{
		/*2021: moving to WM_SYSCOMMAND, phantom unpausing issues
		case SIZE_MINIMIZED: 
			
			if(EX::INI::Window()->do_auto_pause_window_while_inactive)
			EX::pause('mini'); 
			return 0;*/

		//NEW: used by F11
		case SIZE_RESTORED: ////EX::unpause('mini'); return 0;
		case SIZE_MAXIMIZED: //EX::unpause('mini'); //NEW
						
			Ex_window_center_client(); break;
		}		   

		if(EX::is_playing_movie())		
		EX::is_playing_movie_WM_SIZE(LOWORD(l),HIWORD(l));

		break;
	case WM_SYSCOMMAND: //2021

		if(Ex_window_visible) switch(w)
		{
		case SC_MINIMIZE: 
			
			if(EX::INI::Window()->do_auto_pause_window_while_inactive)
			EX::pause('mini'); 
			break;

		//NEW: used by F11
		case SC_RESTORE: //EX::unpause('mini'); return 0;
		case SC_MAXIMIZE: EX::unpause('mini'); //NEW
						
			//Ex_window_center_client();
			break;
		}	
		break;

	case WM_ACTIVATE: 

		if(0==HIWORD(w)) //2021: indicates minimized
		{		
			LOWORD(w)==WA_INACTIVE?
			EX::deactivating_cursor():EX::activating_cursor();
		}

		if(Ex_window_visible)
		if(EX::INI::Window()->do_auto_pause_window_while_inactive)
		{
			(LOWORD(w)==WA_INACTIVE?EX::pause:EX::unpause)('auto');		
		}

		break;
	 	 
	case WM_SETCURSOR: EX::inside = false; 
		
		EX::showing_cursor(); EX::following_cursor(); //hack??	  
		break; 

	case WM_SETFOCUS: break; //???

	case WM_DISPLAYCHANGE: EX::capturing_screen(); return 0;

	case WM_MOVE: case WM_MOVING: GetWindowRect(EX::window,&EX::coords); 
		
		goto client;

	case WM_SETTEXT:
	case WM_GETTEXT: //TODO: EX::caption

		//if(SOM::caption&&*SOM::caption) 
		//	SetWindowTextW(EX::window,SOM::caption);	   		
		break;

	case 0x020E: //WM_MOUSEHWHEEL:

	case WM_MOUSEWHEEL: //requires focus??

	case WM_LBUTTONDOWN: case WM_LBUTTONUP:

		if(!EX::inside) return 0; //FALL THRU (hack)

	case WM_DESTROY: case WM_CLOSE: case WM_QUIT: //exit(0); break;

	case WM_KEYDOWN: case WM_KEYUP: //defer to client window

	case WM_DEVICECHANGE: //hot plug controllers

client:	return Ex_client_proc(EX::client,msg,w,l);

	//NEW: keep background off client window
	case WM_ERASEBKGND:
	{
		RECT rc; assert(EX::client);
		if(GetWindowRect(EX::client,&rc)&&MapWindowRect(0,hwnd,&rc))
		ExcludeClipRect((HDC)w,rc.left,rc.top,rc.right,rc.bottom);
		LRESULT out = DefWindowProcW(hwnd,msg,w,l);		
		ExtSelectClipRgn((HDC)w,0,RGN_COPY);
		return out;
	}}			
	//TODO: defer to client by default
	return DefWindowProcW(hwnd,msg,w,l);
}