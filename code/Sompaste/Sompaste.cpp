		
#include "Sompaste.pch.h"

#include <vector>

//#define SOMPASTE_MMTIMER

#ifdef SOMPASTE_MMTIMER

#include <mmsystem.h>

#pragma comment(lib,"Winmm.lib")

#endif

#define SOMPASTE_CPP_STACKMEM 8

HWND Sompaste_window = 0;

SOMPASTE_API HWND SOMPASTE_LIB(Window)(HWND set)
{
	if(!IsWindow(Sompaste_window)) Sompaste_window = set; 

	return Sompaste_window;
}

SOMPASTE_API RECT SOMPASTE_LIB(Pane)(HWND window, HWND client, bool frame)
{
	RECT out = {0,0,0,0}; 
	
	if(!frame) //Seems like a lot of work...
	{
		WINDOWINFO info = {sizeof(info)};
		if(GetWindowInfo(window,&info)) out = info.rcClient; 
		else return out;
	}
	else if(!GetWindowRect(window,&out)) return out;
	
	if(!client) client = GetParent(window);
	ScreenToClient(client,(POINT*)&out.left);
	ScreenToClient(client,(POINT*)&out.right);	

	return out;
}

SOMPASTE_API wchar_t *SOMPASTE_LIB(Path)(wchar_t inout[MAX_PATH])
{
	if(!inout||!*inout) return inout;

	/*reminder: we don't know the current directory*/

	bool canonicalize = false;

	if(inout[1]==':'&&(inout[2]=='\\'||inout[2]=='/'))
	{	
		*inout = toupper(*inout); //drive letter
	}
	for(int i=0;i<MAX_PATH-1&&inout[i];i++) switch(inout[i])
	{
	case '.': 
		
		switch(inout[i+1]) // ./ and ../ patterns
		{
		case '.': case '/': case '\\': canonicalize = true; 
		}		
		break;

	case '/': inout[i] = '\\'; case '\\':
		
		//i-1: assume \\UNC\ path
		if(i&&inout[i-1]=='\\'&&i-1) //duplicate slashing		
		for(int j=i--;inout[j];j++) inout[j] = inout[j+1];
		break;	
	}

	inout[MAX_PATH-1] = '\0'; //paranoia

	if(canonicalize) 
	{
		wchar_t swap[MAX_PATH];		
		int compile[2*sizeof(WCHAR)==sizeof(DWORD)];
		bool unc = *(DWORD*)inout==*(DWORD*)L"\\\\";
		//using the pattern \\.\ for SOM_EDIT default SOM file
		//This is an optimization to prevent network searching
		//(techincally it's wrong to strip .\ here regardless)
		if(unc&&inout[2]=='.'&&inout[3]=='\\') 
		{
			PathCanonicalizeW(swap,inout+4); 
			wcscpy(inout+4,swap);
		}
		else
		{
			PathCanonicalizeW(swap,inout); 
			wcscpy(inout,swap);
		}
	}

	return inout;
}

static WNDCLASSEXW Sompaste_create = 
{
	sizeof(Sompaste_create),
	CS_PARENTDC /*CS_DBLCLKS*/, //want to discourage double clicks
	DefWindowProcW,
	0,0,0,0, //window icon
	LoadCursor(0,IDC_ARROW),
	(HBRUSH)GetStockObject(NULL_BRUSH), //background 
	0,L"Sompaste.dll",
	0 //class icon
};

SOMPASTE_API HWND SOMPASTE_LIB(Create)(HWND parent, int ID)
{
	static ATOM init = 0; if(!init) init = RegisterClassExW(&Sompaste_create);

	return CreateWindowExW(WS_EX_CONTROLPARENT,L"Sompaste.dll",L"",parent?WS_CHILD:0,0,0,0,0,parent,(HMENU)ID,0,0);
}							  

SOMPASTE_API HWND *SOMPASTE_LIB(Propsheet)(HWND owner, int tabs, const wchar_t *title, HMENU menu)
{
	return Somprops(owner,tabs,title,menu);
}
	 
#ifdef SOMPASTE_MMTIMER
void CALLBACK Sompaste_paper2(UINT uID,UINT, DWORD_PTR dwUser, DWORD,DWORD)
{
	HWND window = (HWND)dwUser;

	if(window!=SOMPASTE_LIB(Paper)()) 
	{
		timeKillEvent(uID); return;
	}

	HWND fore = GetForegroundWindow();

	//A Sompaste_window is required for paper to work 
	if(GetAncestor(fore,GA_ROOTOWNER)==Sompaste_window)
	{
		if(Sompaste_window&&fore!=window)
		{					
			int style = GetWindowStyle(fore);
			int exstyle = GetWindowExStyle(fore);

			//beware modal dialog boxes
			if(style&DS_MODALFRAME||exstyle&WS_EX_DLGMODALFRAME) return;

			SetForegroundWindow(window);

			MessageBeep(-1); //Attempt to match modal dialog behavior

			FLASHWINFO f = {sizeof(f),window,FLASHW_ALL,5,75};

			FlashWindowEx(&f);
		}
	}
}
#endif

VOID CALLBACK Sompaste_paper(HWND window,UINT id,UINT_PTR,DWORD)
{	
	if(window!=SOMPASTE_LIB(Wallpaper)()) 
	{
		KillTimer(window,id); return;
	}

	HWND fore = GetForegroundWindow();

	//A Sompaste_window is required for paper to work 
	if(GetAncestor(fore,GA_ROOTOWNER)==Sompaste_window)
	{
		if(Sompaste_window&&fore!=window)
		{					
			int style = GetWindowStyle(fore);
			int exstyle = GetWindowExStyle(fore);

			//beware modal dialog boxes
			if(style&DS_MODALFRAME||exstyle&WS_EX_DLGMODALFRAME) return;

			SetForegroundWindow(window);

			MessageBeep(-1); //Attempt to match modal dialog behavior

			FLASHWINFO f = {sizeof(f),window,FLASHW_ALL,5,75};

			FlashWindowEx(&f);
		}
	}
}

SOMPASTE_API HWND SOMPASTE_LIB(Wallpaper)(HWND paper, MSG *msg)
{
	const int to = 50;

	static HWND out = 0; //thread-safe??
	
	if(!out&&!paper) return 0; 
	
	if(paper)
	{		
		//TODO: papers should be stacked up

		if(paper==out //2017: Don't destroy itself.
		||IsWindowVisible(out)&&!DestroyWindow(out)) return out;

		ShowWindow(paper,SW_SHOW);	
		if(!IsWindowVisible(paper)&&!IsIconic(Sompaste_window)) 
		{
			DestroyWindow(paper); return out = 0;
		}
		else DestroyWindow(out);
		
		out = paper;

		//Reminder: SetTimer never fires when the message queue is 
		//being flooded. Which happend one night while short-circuiting
		//a WM_PAINT message (guess you can't do that??)

#ifdef SOMPASTE_MMTIMER

		timeSetEvent(50,50,Sompaste_paper2,(DWORD_PTR)out,TIME_PERIODIC);
#else
		SetTimer(out,0,to,Sompaste_paper); 		
#endif	
		return out;
	}
	else if(!out) return 0; //optimized route
	
	if(!IsWindowVisible(out)&&!IsIconic(Sompaste_window)) 
	{
		DestroyWindow(out); out = 0; 
	}

	if(!msg) return out;
	
	switch(msg->message)
	{		
	case WM_NCLBUTTONDOWN: 
	case WM_NCRBUTTONDOWN:

#ifndef SOMPASTE_MMTIMER

		//Not sure there is a clean way to sync a multimedia timer
		//otherwise we might be using that API instead of SetTimer...

		//pretty (synchronize timer)
		SetTimer(out,0,to,Sompaste_paper); 
#endif

		if(msg->wParam!=HTMENU) break;

	case WM_LBUTTONDOWN: case WM_LBUTTONUP:
	case WM_RBUTTONDOWN: case WM_RBUTTONUP:	//filter out...

		if(out&&GetAncestor(msg->hwnd,GA_ROOT)!=out) return msg->hwnd;
	}		  

	return 0;
}

SOMPASTE_API HWND SOMPASTE_LIB(Folder)(SOMPASTE p, HWND owner, wchar_t inout[MAX_PATH], const wchar_t *filter, const wchar_t *title, void *modeless)
{
	return Somdirect(p,owner,inout,filter,title,modeless);
}
 
SOMPASTE_API HWND SOMPASTE_LIB(Database)(SOMPASTE p, HWND owner, wchar_t inout[MAX_PATH], const wchar_t *filter, const wchar_t *title, void *modeless)
{
	return Somproject(p,owner,inout,filter,title,modeless);
}

SOMPASTE_API const wchar_t *SOMPASTE_LIB(Longname)(wchar_t token)
{
	return token>=256?Somproject_longname(token-256):0;
}
SOMPASTE_API wchar_t SOMPASTE_LIB(Name)(const wchar_t *longname)
{
	return (wchar_t)(256+Somproject_name(longname));
}

SOMPASTE_API bool SOMPASTE_LIB(Inject)(SOMPASTE p, HWND owner, wchar_t path[MAX_PATH], size_t kind_or_filesize)
{
	return Somproject_inject(p,owner,path,kind_or_filesize);
}

SOMPASTE_API HWND SOMPASTE_LIB(Place)(SOMPASTE p, HWND owner, wchar_t out[MAX_PATH], const wchar_t *in, const wchar_t *title, void *modeless)
{
	return Somplace(p,owner,out,in,title,modeless);
}

SOMPASTE_API const wchar_t *SOMPASTE_LIB(Environ)(SOMPASTE p, const wchar_t *var, const wchar_t *set)
{	
	return Somenviron(p,var,set);
}

SOMPASTE_API HWND SOMPASTE_LIB(Clipboard)(SOMPASTE p, HWND owner, void *note)
{
	return Somclips(p,owner,note);
}

SOMPASTE_API void *SOMPASTE_LIB(Choose)(wchar_t *args, HWND window, SOMPASTE_LIB(cccb) modeless)
{
	return (void*)Somcolor(args,window,modeless);
}

SOMPASTE_API bool SOMPASTE_LIB(Center)(HWND win, HWND ref, bool activate)
{		
	POINT pt;
	if(!ref&&!GetCursorPos(&pt)) return false;

	//Assuming parent is desktop for now
	MONITORINFO info = {sizeof(info)};

	HMONITOR hm = ref
	?MonitorFromWindow(ref,MONITOR_DEFAULTTONEAREST)
	:MonitorFromPoint(pt,MONITOR_DEFAULTTONEAREST);

	if(!ref //paranoia: proove mouse/keyboard are on the same page
	 &&hm!=MonitorFromWindow(GetFocus(),MONITOR_DEFAULTTONEAREST))
	{
		hm = MonitorFromWindow(GetFocus(),MONITOR_DEFAULTTONEAREST);
	}
	
	if(!GetMonitorInfo(hm,&info)) return false;

	RECT rect, work = info.rcWork;
	if(!GetWindowRect(win,&rect)) return false;
	
	if(ref||!PtInRect(&work,pt))
	{
		pt.x = (work.right-work.left)/2+work.left;
		pt.y = (work.bottom-work.top)/2+work.top;
	}

	SIZE size = { rect.right-rect.left, rect.bottom-rect.top };

	SetRect(&rect,0,0,size.cx,size.cy);	

	OffsetRect(&rect,pt.x,pt.y);
	OffsetRect(&rect,-size.cx/2,-size.cy/2);

	//now solve for top left corner...
	pt.x = rect.left; pt.y = rect.top;

	if(rect.left+size.cx>work.right) pt.x = work.right-size.cx;

	if(rect.left<work.left) pt.x = work.left;

	if(rect.top+size.cy>work.bottom) pt.y = work.bottom-size.cy;

	if(rect.top<work.top) pt.y = work.top;

	UINT flags = SWP_NOSIZE|SWP_NOZORDER; 

	if(!activate) flags|=SWP_NOACTIVATE; else flags|=SWP_SHOWWINDOW;

	return SetWindowPos(win,0,pt.x,pt.y,0,0,flags);
}

SOMPASTE_API bool SOMPASTE_LIB(Cascade)(const HWND *wins, int n)
{			
	for(int i=0;i<n;i++)
	{
		if(*wins) break; wins++; n--;
	}
		
	RECT casc, temp; if(n<2) return true; 	
	
	if(!GetWindowRect(*wins,&casc)) return false;

	HDWP def = BeginDeferWindowPos(n); 

	//Would not mind to activate last window however...
	//DeferWindowPos gets Z order wrong without SWP_NOACTIVATE 
	const UINT flags = SWP_NOSIZE|SWP_NOACTIVATE; 

	for(int i=0,j=0;i<n-1;i++) 
	{		
		if(wins[i]&&IsWindowVisible(wins[i]))
		{				
			LONG style = GetWindowStyle(wins[i]);

			if(style&&GetClientRect(wins[i],&temp))
			{
				AdjustWindowRect(&temp,style,GetMenu(wins[i])?1:0);

				j = i; casc.left-=temp.left; casc.top-=temp.top; 
			}
		}

		if(!wins[i+1]) continue; //blows up def

		def = DeferWindowPos(def,wins[i+1],wins[j],casc.left,casc.top,0,0,flags);
		def = DeferWindowPos(def,wins[j],wins[i+1],0,0,0,0,flags|SWP_NOMOVE);
	}

	EndDeferWindowPos(def); 

	return true;
}

SOMPASTE_API bool SOMPASTE_LIB(Desktop)(const HWND*, int count, bool flash)
{
	return true; //unimplemented
}

SOMPASTE_API bool SOMPASTE_LIB(Sticky)(const HWND *wins, int n, int *sticky, RECT *memory, bool zorder)
{	
	if(!wins||!n||IsIconic(*wins)) return false;

	for(int i=0;i<n;i++)
	{
		if(*wins) break; wins++; n--;

		if(memory&&memory++) SetRect(memory-1,0,0,0,0);

		if(sticky) *sticky++ = -1; 
	}

	//TODO: heap allocate if n>
	if(n<2||!memory&&n>SOMPASTE_CPP_STACKMEM) return false; 	
	
	RECT _memory[SOMPASTE_CPP_STACKMEM];
	
	if(!memory) memory = _memory;

	for(int i=0;i<n;i++)
	{
		if(!GetWindowRect(wins[i],memory+i)) 
		{
			SetRect(memory+i,0,0,0,0); if(i==0) return false;
		}

		if(sticky) sticky[i] = -1;
	}

	if(!sticky) return true; //finished

	HDWP def = zorder?BeginDeferWindowPos(0):0; 

	//TODO: better algorithm/logic

	bool againandagain = true; RECT temp;

	while(againandagain)
	{
		againandagain = false;
		for(int i=1;i<n;i++) if(sticky[i]<0)
		{
			RECT &rect = memory[i];
			for(int j=0;j<n;j++) if(!j||sticky[j]>=0)	
			if(IntersectRect(&temp,&rect,&memory[j]))
			{
				sticky[i] = j; againandagain = true; 

				const int flags = SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE;

				//// Recall: z-order is reversed/dumb ////

				if(zorder) 
				{
					def = DeferWindowPos(def,wins[j],wins[i],0,0,0,0,flags); 
				}

				break;
			}					
		}
	}	
	
	if(zorder) EndDeferWindowPos(def); 

	return true;
}

SOMPASTE_API bool SOMPASTE_LIB(Arrange)(const HWND *wins, int n, const int *sticky, const RECT *memory)
{
	if(!wins||!n||IsIconic(*wins)) return false;

	const int sep = 10; 
	
	for(int i=0;i<n;i++)
	{
		if(*wins) break; wins++; n--;

		if(sticky) sticky++; if(memory) memory++;
	}
		
	//TODO: heap allocate if n>
	if(n<2||n>SOMPASTE_CPP_STACKMEM) return false; 	

	RECT rects[SOMPASTE_CPP_STACKMEM]; 

	for(int i=0;i<n;i++)
	{
		if(!GetWindowRect(wins[i],&rects[i])) 
		{
			SetRect(&rects[i],0,0,0,0); if(i==0) return false;
		}
	}
	
	RECT &main = rects[0], temp;
	
	if(memory)
	{
		const RECT &s0 = memory[0];

		POINT center =
		{
			s0.left+(s0.right-s0.left)/2,
			s0.top+(s0.bottom-s0.top)/2
		};
		
		for(int i=1;i<n;i++)
		{
			RECT &rect = rects[i]; 
	
			if(!rect.left&&!rect.top) continue;

			int x = 0;
			int y = 0;
			int w = rect.right-rect.left;
			int h = rect.bottom-rect.top;

			const RECT &si = memory[i];

			if(si.left>center.x)
			{
				x = main.right+(si.left-s0.right);
			}
			else x = main.left+(si.left-s0.left);

			if(si.top>center.y)
			{
				y = main.bottom+(si.top-s0.bottom);
			}
			else y = main.top+(si.top-s0.top);

			RECT swap = {x,y,x+w,y+h};

			rect = swap;
		}			
	}

	//Assuming parent is desktop for now
	MONITORINFO info = {sizeof(info)};
	HMONITOR hm = MonitorFromWindow(*wins,MONITOR_DEFAULTTONEAREST);

	if(!GetMonitorInfo(hm,&info)) return false;

	RECT work = info.rcWork;

	work.left+=sep; work.right-=sep; 
	work.top+=sep; work.bottom-=sep;

	for(int i=1;i<n;i++) 
	{		
		RECT &rect = rects[i]; 
		
		if(sticky&&sticky[i]>=0) continue;

		if(!rect.left&&!rect.top) continue;

		IntersectRect(&temp,&rect,&work);
				
		bool outofbounds = !EqualRect(&temp,&rect);

		bool justfineasis = !outofbounds;

		for(int j=0;j<n&&justfineasis;j++) if(i!=j) 
		justfineasis = !IntersectRect(&temp,&rect,&rects[j]);

		if(justfineasis) continue;
						
		int w = rect.right-rect.left, cx = (main.right-main.left-w)/2; 
		int h = rect.bottom-rect.top, cy = (main.bottom-main.top-h)/2; 

		RECT best = rect; int area = 0;

		for(int j=0,x,y;j<3;j++) //3 pass algorithm		
		{
			switch(j)
			{
			case 0: x = main.right+sep; y = main.top+cy; break;

			case 1: x = main.left+cx; y = main.bottom+sep; break;

			case 2:	x = main.left+cx; y = main.top-sep-h; break;
			}

			RECT move = {x,y,x+w,y+h}, copy = move; 

			bool rtl = false;
rtl: 		for(int k=!rtl;k<n;k++) if(i!=k)
			{
				RECT &test = rects[k];

				if(IntersectRect(&temp,&move,&test))
				{
					if(rtl)
					{
						 x = test.left-sep-move.right;
					}
					else x = test.right+sep-move.left;

					OffsetRect(&move,x,0);									
				}				
			}
						
			IntersectRect(&temp,&move,&work);
				
			if(EqualRect(&temp,&move)) 
			{
				best = move; break;
			}
			//condition: want to avoid cutting off title bars
			else if(temp.top!=work.top&&temp.left!=work.left)
			{	
				x = temp.right-temp.left;
				y = temp.bottom-temp.top;

				if(x*y>area)
				{
					best = move; area = x*y;
				}
			}

			if(rtl) continue;
			
			OffsetRect(&(move=copy),-sep*2,0);					

			rtl = true; 
			
			goto rtl;
		}

		rect = best;
	}

	HDWP def = BeginDeferWindowPos(n); 

	for(int i=1;i<n;i++) 
	{
		if(!wins[i]) continue; //blows up def

		const UINT flags = SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER;

		RECT &rect = rects[i]; if(!rect.left&&!rect.top) continue;

		def = DeferWindowPos(def,wins[i],0,rect.left,rect.top,0,0,flags);
	}

	EndDeferWindowPos(def); 

	return true;
}

SOMPASTE_API void *SOMPASTE_LIB(New)(size_t sizeinbytes)
{
	return new char[sizeinbytes];
}

SOMPASTE_API void SOMPASTE_LIB(Depart)(SOMPASTE p)
{
	return; //unimplemented
}

SOMPASTE_API bool SOMPASTE_LIB(Undo)(SOMPASTE p, HWND hist, void *add)
{
	return false; //unimplemented
}

SOMPASTE_API bool SOMPASTE_LIB(Redo)(SOMPASTE p, HWND hist)
{
	return false; //unimplemented
}

SOMPASTE_API void SOMPASTE_LIB(Reset)(HWND hist)
{
	return; //unimplemented
}

SOMPASTE_API HWND SOMPASTE_LIB(Xfer)(const char *op, const HWND *wins, int count, void *data, size_t data_s)
{		
	return Somtransfer(op,wins,count,data,data_s);
}

static HMODULE Sompaste_hmodule = 0;

extern HMODULE Sompaste_dll()
{
	return Sompaste_hmodule;
}
namespace Sompaste_cpp
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
}

static std::vector<void*> Sompaste_delete;

extern void Somdelete(void *del)
{
	Sompaste_cpp::section cs;
	Sompaste_delete.push_back(del);	
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{	
	if(ul_reason_for_call==DLL_PROCESS_ATTACH)
	{	
		Sompaste_hmodule = hModule;
	}
	else if(ul_reason_for_call==DLL_THREAD_ATTACH)
	{

	}
	else if(ul_reason_for_call==DLL_THREAD_DETACH)
	{

	}
	else if(ul_reason_for_call==DLL_PROCESS_DETACH)
	{
		size_t i = Sompaste_delete.size();
		while(i>0) delete Sompaste_delete[--i];
	}

    return TRUE;
} 