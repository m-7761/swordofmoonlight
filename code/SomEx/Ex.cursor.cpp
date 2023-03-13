
#include "Ex.h" 
EX_TRANSLATION_UNIT

//#include <d3dx9.h> //direct3d cursor interfaces

#include "Ex.ini.h"

#include "dx.ddraw.h"

#include "Ex.window.h"
#include "Ex.cursor.h"

extern int EX::x = 0;
extern int EX::y = 0;

extern bool EX_cursor = false;
static bool EX_active = false;
static bool EX_crosshair = false;

extern bool EX::cursor = false;
extern bool EX::inside = false;
extern bool EX::active = false;
extern bool EX::onhold = false;
extern bool EX::crosshair = false; //2020

extern int EX::pointer = 0;
//extern int EX::waiting = 0;
static int Ex_cursor_wait[2] = {};

extern void (*EX::timeout)() = 0;

static int Ex_cursor_position[2] = {0,0};
static int Ex_cursor_movement[2] = {0,0};

static int Ex_cursor_trigger = 3;
static int Ex_cursor_timeout = 3000;
static int Ex_cursor_restref = 0;
static int Ex_cursor_resting = 0;
static int Ex_cursor_holdref = 0;
static int Ex_cursor_holding = 0; 

static bool Ex_cursor_hiding = false;
static bool Ex_cursor_showing = false;
static bool Ex_cursor_waiting = false;

static bool Ex_mouse_installed = false;

static bool Ex_cursor_nonclient = false;

static HCURSOR Ex_cursor_z = 0;

//allow a frame to not appear busy
static int Ex_cursor_activate = 0; 

static bool Ex_cursor_initialized = false;

static bool Ex_cursor_captured = false;

static RECT *Ex_cursor_clipped = 0;

static POINT Ex_cursor_clipping={0,0};

static DWORD Ex_cursor_clipticks = 0;

static void Ex_cursor_timer_rollback()
{
	Ex_cursor_trigger = 5;
	Ex_cursor_holding = 0;
	Ex_cursor_resting = 0;	
	Ex_cursor_restref = EX::tick();
}

static void Ex_cursor_timer_wrapitup()
{
	Ex_cursor_trigger = 15;
	Ex_cursor_resting = Ex_cursor_timeout;
	Ex_cursor_holding = 300;
	Ex_cursor_holdref = EX::tick();
}

static void Ex_cursor_ShowCursor(int sh) //2020
{
	if(!Ex_mouse_installed) return; //I guess?

	if(EX_crosshair&&Ex_cursor_clipped) //return;
	{
		if(EX_active&&!sh) return;
	}

	//the cursor is sometimes becoming hidden for
	//the remainder of the program. this is a fix
	int cmp = ShowCursor(sh!=0);
	while(cmp<=0&&++cmp!=sh) ShowCursor(true);
	while(cmp>=0&&cmp--!=sh) ShowCursor(false);
}

static void Ex_cursor_clip()
{
	if(!Ex_cursor_clipped) return;

	RECT client, &clip = *Ex_cursor_clipped;

	if(!GetWindowRect(EX::client,&client))  
	{
		ClipCursor(0); Ex_cursor_clipped = 0;
				
		return; //good enough?
	}

	clip.left+=client.left; clip.right+=client.left;
	clip.top+=client.top; clip.bottom+=client.top;

	ClipCursor(Ex_cursor_clipped); //ie. clip

	clip.left-=client.left; clip.right-=client.left;
	clip.top-=client.top; clip.bottom-=client.top;
}

static void Ex_cursor_set(bool following=false)
{
	//2020: "following" is needed because is_cursor_hidden
	//is unreliable before SetCursor is called

	if(!Ex_cursor_nonclient)
	if(EX::pointer=='wait'&&DDRAW::isPaused)
	{
		EX::pointer = Ex_cursor_wait[0]; //hack??
	}

	if(EX::pointer!='wait') Ex_cursor_wait[0] = EX::pointer;

	if(!Ex_cursor_nonclient)
	if((EX::pointer=='wait')!=Ex_cursor_waiting)
	{
		Ex_cursor_waiting = !Ex_cursor_waiting;
		if(!Ex_cursor_wait[1]) //2020
		Ex_cursor_ShowCursor(Ex_cursor_waiting); 
	}

	if(!EX::inside) return;
	 
	LPWSTR lc = 0;

	if(!EX_active||Ex_cursor_activate)
	{
		if(!Ex_cursor_activate) Ex_cursor_activate = DDRAW::noFlips+1;

		if(Ex_cursor_activate==DDRAW::noFlips) Ex_cursor_activate = 0;

		lc = IDC_ARROW; 
			
		if(!Ex_cursor_nonclient&&EX::pointer=='wait')
		{
			lc = IDC_APPSTARTING; //hourglass+arrow
		}
	}	
	else if(!Ex_cursor_nonclient) 
	{
		LPWSTR arrow = EX::arrow()?IDC_ARROW:IDC_NO;

		switch(EX::pointer)
		{
		case 'z': 
			
			if(!Ex_cursor_z) //TODO: XP
			{
				/*2020: discontinuing for time being
				//Ex_cursor_z = LoadCursorFromFile("EX\\VISTA\\z.cur"); 
				Ex_cursor_z = (HCURSOR)LoadImage(0,L"EX\\VISTA\\z.cur",IMAGE_CURSOR,0,0,LR_DEFAULTSIZE|LR_LOADFROMFILE); 
				*/
				if(!Ex_cursor_z) Ex_cursor_z = LoadCursor(0,IDC_UPARROW); 
			}
			SetCursor(Ex_cursor_z); return; //break; 
		
		case 'wait': lc = IDC_WAIT; break;

		case 'hand': lc = IDC_HAND; break;

		default: if(EX_cursor) lc = arrow;
		}
	}
	else if(EX::window) //basic cursor support
	{
		if(EX_cursor) lc = IDC_ARROW;
	}

	if(EX_crosshair&&Ex_cursor_clipped&&EX_active) 
	{
		if(!lc||lc==IDC_ARROW||lc==IDC_NO) lc = IDC_CROSS;
	}

	if(!following||lc!=IDC_NO||!EX::is_cursor_hidden()) //2020
	{
		SetCursor(lc?LoadCursor(0,lc):0);
	}
}

static void Ex_cursor_show(bool show)
{		
	if(!Ex_cursor_initialized)
	{
		int ic = ShowCursor(false);

		if(ic>=-1)
		{
			Ex_mouse_installed = true; assert(ic==-1);
		}
		else assert(ic==-2);

		EX::INI::Option op;
		
		if(!op||!op->do_cursor)		
		{
			Ex_cursor_nonclient = true;
		}

		Ex_cursor_initialized = true;

		//GOOD IDEA?
		//2020: prevent bogus data at startup?
		//EX::following_cursor()
		{
			POINT pos; RECT client;
			if(GetCursorPos(&pos))
			if(GetWindowRect(EX::client,&client))
			{
				Ex_cursor_position[0] = pos.x; 
				Ex_cursor_position[1] = pos.y; 

				EX::x = pos.x-client.left;
				EX::y = pos.y-client.top;
			}
		}
	}

	if(show)
	{	 	
		Ex_cursor_timer_rollback();

//		if(!EX_cursor)
		{
			if(0)
			{
				int ic = ShowCursor(EX::cursor=EX_cursor=true);

				assert(ic==Ex_cursor_waiting?1:0);
			}
			else Ex_cursor_ShowCursor(EX::cursor=EX_cursor=true);

//			if(0&&DDRAW::Direct3DDevice7&&DDRAW::Direct3DDevice7->target=='dx9c')
			{
//				DDRAW::Direct3DDevice7->proxyX->ShowCursor(true);
			}
		}
	}
	else
	{
		Ex_cursor_timer_wrapitup();

//		if(EX_cursor)
		{
			if(0)
			{
				int ic = ShowCursor(EX::cursor=EX_cursor=false);

				assert(ic==Ex_cursor_waiting?0:-1);
			}
			else Ex_cursor_ShowCursor(EX::cursor=EX_cursor=false);	

//			if(0&&DDRAW::Direct3DDevice7&&DDRAW::Direct3DDevice7->target=='dx9c')
			{
//				DDRAW::Direct3DDevice7->proxyX->ShowCursor(false);
			}
		}
	}
}

extern void EX::following_cursor(int x, int y)
{	
	/*trying to prevent 1px jiggle
	if(EX::crosshair&&EX_active&&Ex_cursor_clipped)
	{
		SetCursorPos(x=EX::x,y=EX::y);
	}*/
	
	if(x!=-1&&Ex_cursor_position[0]!=x
	 ||y!=-1&&Ex_cursor_position[1]!=y)
	{
		POINT pos; RECT client;

		if(GetCursorPos(&pos))
		if(GetWindowRect(EX::client,&client))
		{
			EX::x = pos.x-client.left;
			EX::y = pos.y-client.top;
		}		
	}

	if(EX::window&&EX_active) 
	{			
		if(EX::coords.left!=Ex_cursor_clipping.x
		  ||EX::coords.top!=Ex_cursor_clipping.y)
		{				
			if(!Ex_cursor_clipticks //cursor API needs time
			||EX::tick()-Ex_cursor_clipticks>200)
			{
				if(Ex_cursor_clipped) Ex_cursor_clip();			

				Ex_cursor_clipping.x = EX::coords.left;
				Ex_cursor_clipping.y = EX::coords.top;

				Ex_cursor_clipticks = 0;
			}
		}
	}

	if(!EX::inside) return;

	static bool following = false;

	if(following) return; following = true;

	if(x==-1) x = Ex_cursor_position[0];
	if(y==-1) y = Ex_cursor_position[1];

	if(!EX::onhold)
	if(Ex_cursor_holding&&!Ex_cursor_showing)
	{	
		Ex_cursor_movement[0] = Ex_cursor_movement[1] = 0;

		Ex_cursor_holding-=EX::tick()-Ex_cursor_holdref;

		Ex_cursor_holdref = Ex_cursor_restref = EX::tick();

		if(Ex_cursor_holding<0) Ex_cursor_holding = 0;
	}
	else if(!Ex_cursor_showing&&!Ex_cursor_hiding)
	{
		Ex_cursor_movement[0]+=Ex_cursor_position[0]-x;
		Ex_cursor_movement[1]+=Ex_cursor_position[1]-y;

		if(abs(Ex_cursor_movement[0])>Ex_cursor_trigger
		 ||abs(Ex_cursor_movement[1])>Ex_cursor_trigger) 
		{
			Ex_cursor_resting = Ex_cursor_movement[0] = Ex_cursor_movement[1] = 0;		
		}
		else Ex_cursor_resting+=EX::tick()-Ex_cursor_restref;

		Ex_cursor_restref = EX::tick();

		if(Ex_cursor_resting<Ex_cursor_timeout)
		{
			if(!Ex_cursor_resting&&!EX_cursor) EX::showing_cursor();
		}
		else if(EX_cursor)
		{
			if(EX::timeout) EX::timeout(); //timeout handler

			EX::hiding_cursor();
		}
	}

	Ex_cursor_position[0] = x;
	Ex_cursor_position[1] = y;

	if(1&&Ex_cursor_showing)
	{	
		Ex_cursor_show(true);

		Ex_cursor_showing = false;
	}
	if(1&&Ex_cursor_hiding)
	{
		Ex_cursor_show(false);

		Ex_cursor_hiding = false;
	}

	Ex_cursor_set(true);

	following = false;
}

extern void EX::showing_cursor(int in)
{
	if(EX::inside) EX::onhold = false;

	if(in!='same')
	{
		if(in=='wait')
		{				
			if(EX::pointer!='wait')
			{
				Ex_cursor_wait[0] = EX::pointer;
				Ex_cursor_wait[1] = EX_cursor; //2020
			}

			EX::pointer = 'wait'; 
			
			return; //special
		}
		else if(EX::pointer=='wait')
		{
			Ex_cursor_wait[0] = in; //put on hold
		}
		else EX::pointer = in;
	}
	
	/*if(Ex_cursor_hiding)*/ Ex_cursor_hiding = false;
	
	Ex_cursor_showing = true; //return; //???

	Ex_cursor_show(true);

	Ex_cursor_set();	
}

extern void EX::hiding_cursor(int in)
{
	if(EX::inside) EX::onhold = false;

	if(in=='wait')
	{
		if(EX::pointer=='wait') 
		{
			EX::pointer = Ex_cursor_wait[0]; 
			
			Ex_cursor_set(); //2020

			if(!Ex_cursor_wait[1]) goto wait; //2020
		}

		return; //special
	}
	else if(EX::pointer!='wait')
	{
		EX::pointer = 0;
	}
	else Ex_cursor_wait[0] = 0; wait:

	/*if(Ex_cursor_showing)*/ Ex_cursor_showing = false;
	
	Ex_cursor_hiding = true; 

	Ex_cursor_show(false);

	Ex_cursor_set();
}

static void Ex_cursor_capture(HWND w)
{
	//2022: should 0 be passed to this? (deactivating_cursor)
	
	//NOTE: Ex_window_proc implements WM_INPUT
	//via Ex.input.cpp

	if(EX::INI::Option()->do_mouse2) //doesn't feel so good
	{
		RAWINPUTDEVICE Rid[1];
		Rid[0].usUsagePage = 1; //HID_USAGE_PAGE_GENERIC; 
		Rid[0].usUsage = 2; //HID_USAGE_GENERIC_MOUSE; 
		Rid[0].dwFlags = w?0:RIDEV_REMOVE;
		if(!EX::INI::Window()->do_auto_pause_window_while_inactive)
		Rid[0].dwFlags|= RIDEV_INPUTSINK;   
		Rid[0].hwndTarget = w;
		int bp = RegisterRawInputDevices(Rid,1,sizeof(Rid[0]));
		assert(bp);
	}

	//January 2022 Windows 10 seems to have new bugs/behavior?!
	//I can't figure out why but SetCapture is failing on start

	if(w) //2022: was passing 0 to SetCapture?
	{
		SetFocus(w); //maybe set focus first???

		SetCapture(w); //almost forgot
	}
	else ReleaseCapture(); //2022 
}

extern void EX::capturing_cursor()
{
	if(EX::inside) EX::onhold = false;

	//Note: could enforce EX_inside 
	if(EX_active) Ex_cursor_capture(EX::display()); 

	Ex_cursor_captured = true;
}

extern void EX::releasing_cursor()
{
	if(EX::inside) EX::onhold = false;

	if(EX_active) ReleaseCapture(); 

	Ex_cursor_captured = false;
}

extern void EX::clipping_cursor(int x, int y, int w, int h)
{
	if(EX::inside) EX::onhold = false;

	RECT client;

	if(!GetWindowRect(EX::client,&client))  
	{
		ClipCursor(0); Ex_cursor_clipped = 0;
		
		return; //good enough?
	}

	if(EX_crosshair=EX::crosshair&&x==-1&&y==-1) //2020
	{
		//EX::x = 
		x = (client.right-client.left)/2;
		//EX::y = 
		y = (client.bottom-client.top)/2;

		SetCursor(LoadCursor(0,IDC_CROSS));
		SetCursorPos(client.left+x,client.top+y);
		Ex_cursor_ShowCursor(true);
	}
	else if(x==-1||y==-1) //defaults
	{
		POINT pos; 
		
		if(GetCursorPos(&pos))
		{
			if(x==-1) x = pos.x-client.left;
			if(y==-1) y = pos.y-client.top;
		}
	}
											  
	x+=client.left; y+=client.top;

	static RECT clip; RECT trap = {x,y,x+w,y+h};

	if(w>0&&h>0) //Note: treating "lines" as points
	{
		if(!IntersectRect(&clip,&client,&trap)) clip = client;
	}
	else clip = PtInRect(&client,*(POINT*)&trap)?trap:client;

	//defer clipping until Ex::following_cursor()
	Ex_cursor_clipping.x = EX::coords.left-12345; //fill with
	Ex_cursor_clipping.y = EX::coords.top-678910; //garbage!!

	clip.left-=client.left; clip.right-=client.left;
	clip.top-=client.top; clip.bottom-=client.top;

	Ex_cursor_clipticks = EX::tick();

	Ex_cursor_clipped = &clip;

	//2022: I think I must have accidentally deleted this line
	//for some time (It wasn't clipping at start up) or was it
	//that following_cursor calls Ex_cursor_clip? now it isn't
	//being released on exit by abandoing_cursor?
	Ex_cursor_clip();
}

extern void EX::unclipping_cursor()
{
	if(EX::inside) EX::onhold = false;

	//2022: abandoning_cursor (SOM::altf4) isn't unclipping
	//and Windows isn't unclipping with process termination
	//if(EX_active) 
	ClipCursor(0);
	
	Ex_cursor_clipped = 0;
}

extern void EX::activating_cursor()
{
	if(EX::inside) EX::onhold = false;

	if(EX::active&&EX_active) return;

	if(EX_active)
	{
		assert(0); //should not happen

		EX::active = true; return;
	}
		
	if(Ex_cursor_initialized)
	{
		//SURE ! ISN'T A MISTAKE???
		if(!EX_cursor) Ex_cursor_ShowCursor(false);
	}

	if(Ex_cursor_clipped) EX::clipping_cursor(); //hacK: clipticks

	if(Ex_cursor_captured) Ex_cursor_capture(EX::display());

	EX::active = EX_active = true; 

	Ex_cursor_activate = 0;

	//hack: more responsive
	if(Ex_cursor_captured||!EX::pointer
	||EX::pointer=='wait'&&!Ex_cursor_wait[0])
	{
		Ex_cursor_show(false); //hack
	}

	EX::following_cursor(); 
}

extern void EX::deactivating_cursor(bool hold)
{
	EX::onhold = hold;

	if(!EX::active&&!EX_active) return;

	if(!EX_active)
	{
		assert(0); //should not happen

		EX::active = false; return;
	}

	if(Ex_cursor_initialized)
	{
		if(!EX_cursor) Ex_cursor_ShowCursor(true);
	}

	if(Ex_cursor_clipped) ClipCursor(0);

	//2022: passing 0 seems wrong here???
	if(Ex_cursor_captured) Ex_cursor_capture(0); 

	EX::active = EX_active = false; 

	Ex_cursor_activate = 0;

	Ex_cursor_set();
}

extern bool EX::is_cursor_hidden()
{
	//REMINDER: this doesn't return the ShowCursor
	//state... it depends on SetCursor being called
	//after ShowCursor to be false

	CURSORINFO ci = {sizeof(ci)};
	if(GetCursorInfo(&ci))
	{
		//assert(ci.flags&CURSOR_SHOWING);
		return ~ci.flags&CURSOR_SHOWING;
	}
	else assert(0); return false;
}