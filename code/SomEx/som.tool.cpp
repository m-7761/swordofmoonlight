
#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

#pragma comment(lib,"Imm32.lib") //IME?

#include <set>
#include <map>
#include <vector>
#include <algorithm>
#include <string>

#include "Ex.ini.h"
#include "Ex.output.h"

#include "SomEx.h" 
#include "SomEx.res.h" //IDD_DUMMY

#include "som.932.h"
#include "som.state.h"
#include "som.files.h"
#include "som.title.h" //SOM::MO::
#include "som.tool.hpp"
extern SOMPASTE Sompaste = 0;

#include "../lib/swordofmoonlight.h" //zip

namespace EX{ extern int x,y; }

namespace DDRAW{ extern bool fx2ndSceneBuffer; }

//classic: means emulating Som2k
static HWND som_tool_pushed = 0;	 
static bool som_tool_classic = true;
static const bool som_tool_f1 = false; //WinHelp

//SCHEDULED FOR REVAMPING
//(som_tool_stack/dialog)
//2018: what was it that went beyond 8?
enum{ som_tool_stack_s = 16 };
extern HWND som_tool_stack[16+1] = {};
//HACK: ultra frugally repurposed unused memory
extern DWORD(&MapComp_memory)[6] = (DWORD(&)[6])som_tool_stack[2];
extern HWND &workshop_tool = som_tool_stack[5];
extern HWND &workshop_host = som_tool_stack[6];
extern HMENU &workshop_menu = (HMENU&)som_tool_stack[7];
extern HACCEL &workshop_accel = (HACCEL&)som_tool_stack[8];
static HWND &som_prm_model = som_tool_stack[8];
extern void *SOM_PRM_strength = som_tool_stack+9;
extern HMENU &workshop_mode2_15_submenu = (HMENU&)som_tool_stack[9];
extern BYTE workshop_mode;

extern HWND som_tool_workshop = 0;
extern bool workshop_exe(int);

extern HWND som_tool_dialog(HWND=0);
static RECT som_tool_dialogunits = {};
inline bool som_tool_dialog_untouched()
{
	return som_tool_dialogunits.right==0;
}
//NEW: !GetParent replacement for taskbar
extern HWND &som_tool = som_tool_stack[0];
extern int som_tool_initializing = 0; 

static bool som_prm_graphing = false;
extern int som_prm_tab = 0, som_sys_tab = 0, som_map_tab = 0;
extern void SOM_PRM_extend(int=0);
extern std::vector<BYTE> SOM_PRM_items;

//som.state.cpp and som.hacks.cpp
extern HWND som_map_tileview = 0;
extern HWND som_map_tileviewport = 0;
extern HWND som_map_tileviewinsert = 0;
extern UINT som_map_tileviewmask = 0x303f;
static void som_map_tileviewpaint() 
{
	if(0x80000000&~som_map_tileviewmask
	//RDW_UPDATENOW keeps things from going haywire???
	&&som_map_tileviewport&&IsWindowVisible(som_map_tileviewinsert)) 
	RedrawWindow(som_map_tileviewport,0,0,RDW_UPDATENOW|RDW_INVALIDATE|RDW_NOERASE);
}
//todo: pare these variable nests down
static char *som_map_event_memory = 0; //hack
static bool som_map_event_is_system_only = false;
static HWND som_map_event = 0, som_map_event_class = 0; 
static long som_map_event_slot = -1;

//do compile/test in one step?
//2018: SOM_MAP_165 uses this; and SOM_SYS will as well
extern char som_tool_play = 0; 
extern char som_tool_space = 0;

/*extern*/ std::vector<WCHAR> som_tool_wector; 
extern HDC som_tool_dc = CreateCompatibleDC(0);
extern HFONT som_tool_charset = 0, som_tool_typeset = 0;
extern int som_tool_charset_height = 0;

//REMOVE ME?
//IP Address Control deletes
//our fonts when it is destroyed?!?
static BOOL WINAPI som_tool_DeleteObject(HGDIOBJ obj)
{
	if(obj==(HGDIOBJ)som_tool_typeset) return TRUE;	  
	return SOM::Tool.DeleteObject(obj);
}

static LPARAM som_tool_SendMessageA_a = 0;
static LRESULT WINAPI som_tool_SendMessageA(HWND,UINT,WPARAM,LPARAM);
static LRESULT WINAPI som_tool_SendDlgItemMessageA(HWND A,int B,UINT C,WPARAM D,LPARAM E)
{
	return som_tool_SendMessageA(GetDlgItem(A,B),C,D,E);
}
static BOOL WINAPI som_tool_PostMessageA(HWND A,UINT B,WPARAM C,LPARAM D)
{
	assert(B!=WM_PAINT);
	return PostMessageW(A,B,C,D);
}
static BOOL WINAPI som_tool_PostThreadMessageA(DWORD A,UINT B,WPARAM C,LPARAM D)
{	
	assert(B!=WM_PAINT);
	return PostThreadMessageW(A,B,C,D);
}
static LRESULT WINAPI som_tool_CallWindowProcA(WNDPROC A,HWND B,UINT C,WPARAM D,LPARAM E)
{
	if(A<(WNDPROC)0x401000) 
	{
		return 0; //SOM_MAP.exe!004691f5() passed 6 one day (callstack diagnosis could be bogus)
	}

	return CallWindowProcW(A,B,C,D,E);
}

//notice: these bypass SendMessageA
extern BOOL WINAPI workshop_SetWindowTextA(HWND,LPCSTR); 
static BOOL WINAPI som_tool_SetWindowTextA(HWND,LPCSTR); 
static BOOL WINAPI som_tool_SetDlgItemTextA(HWND A,int B,LPCSTR C)
{
	return som_tool_SetWindowTextA(GetDlgItem(A,B),C);
}
static int WINAPI som_tool_GetWindowTextA(HWND,LPSTR,int); 
static UINT WINAPI som_tool_GetDlgItemTextA(HWND A,int B,LPSTR C,int D)
{
	return som_tool_GetWindowTextA(GetDlgItem(A,B),C,D);
}
static int WINAPI som_tool_GetWindowTextLengthA(HWND); 

static INT_PTR WINAPI workshop_DialogBoxParamA(HINSTANCE,LPCSTR,HWND,DLGPROC,LPARAM);
static HWND WINAPI som_tool_CreateDialogIndirectParamA(HINSTANCE,LPCDLGTEMPLATE,HWND,DLGPROC,LPARAM);
static HWND WINAPI som_tool_CreateWindowExA(DWORD,LPCSTR,LPCSTR,DWORD,int,int,int,int,HWND,HMENU,HINSTANCE,LPVOID);
static int WINAPI som_tool_DrawTextA(HDC,LPCSTR,int,LPRECT,UINT);
static BOOL WINAPI som_tool_TextOutA(HDC,int,int,LPCSTR,int);
static BOOL WINAPI som_map_FrameRgn(HDC,HRGN,HBRUSH,int,int);

//monitoring
static BOOL WINAPI som_tool_SetCurrentDirectoryA(LPCSTR); 
static BOOL WINAPI som_tool_SetEnvironmentVariableA(LPCSTR,LPCSTR);
static HANDLE WINAPI som_tool_CreateMutexA(LPSECURITY_ATTRIBUTES,BOOL,LPCSTR);
static HANDLE WINAPI som_tool_OpenMutexA(DWORD,BOOL,LPCSTR);
//SOM_MAP
static BOOL WINAPI som_tool_DeleteFileA(LPCSTR);
static DWORD WINAPI som_tool_GetFullPathNameA(LPCSTR,DWORD,LPSTR,LPSTR*);
static DWORD WINAPI som_tool_GetFileAttributesA(LPCSTR);
static BOOL WINAPI som_tool_GetDiskFreeSpaceA(LPCSTR,LPDWORD,LPDWORD,LPDWORD,LPDWORD);
static BOOL WINAPI som_tool_GetVolumeInformationA(LPCSTR,LPSTR,DWORD,LPDWORD,LPDWORD,LPDWORD,LPSTR,DWORD);
		
static HINSTANCE STDAPICALLTYPE som_tool_ShellExecuteA(HWND,LPCSTR,LPCSTR,LPCSTR,LPCSTR,INT); 

static BOOL WINAPI som_tool_CreateProcessA(LPCSTR,LPSTR,LPSECURITY_ATTRIBUTES,LPSECURITY_ATTRIBUTES,BOOL,DWORD,LPVOID,LPCSTR,LPSTARTUPINFOA,LPPROCESS_INFORMATION);
static BOOL WINAPI som_tool_CreateProcessW(LPCWSTR,LPWSTR,LPSECURITY_ATTRIBUTES,LPSECURITY_ATTRIBUTES,BOOL,DWORD,LPVOID,LPCWSTR,LPSTARTUPINFOW,LPPROCESS_INFORMATION);

//2018: PrtsEdit & co use this to spoof InstDir just like SOM_MAIN
static LONG APIENTRY som_main_etc_RegQueryValueExA(HKEY,LPCSTR,LPDWORD,LPDWORD,LPBYTE,LPDWORD);

extern LPITEMIDLIST STDAPICALLTYPE som_tool_SHBrowseForFolderA(LPBROWSEINFOA);
	
static BOOL APIENTRY som_tool_GetOpenFileNameA(LPOPENFILENAMEA);
extern BOOL APIENTRY workshop_GetSaveFileNameA(LPOPENFILENAMEA);

static BOOL WINAPI som_main_CopyFileA(LPCSTR,LPCSTR,BOOL);
static BOOL WINAPI som_run_CopyFileA(LPCSTR,LPCSTR,BOOL);

static HANDLE WINAPI som_tool_FindFirstFileA(LPCSTR,LPWIN32_FIND_DATAA);

static BOOL WINAPI som_tool_FindNextFileA(HANDLE,LPWIN32_FIND_DATAA);
static BOOL WINAPI som_tool_FindClose(HANDLE);

static HANDLE WINAPI som_tool_CreateFileA(LPCSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,DWORD,DWORD,HANDLE);
extern HANDLE WINAPI workshop_CreateFileA(LPCSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,DWORD,DWORD,HANDLE);

static BOOL WINAPI som_tool_ReadFile(HANDLE,LPVOID,DWORD,LPDWORD,LPOVERLAPPED);
extern BOOL WINAPI workshop_ReadFile(HANDLE,LPVOID,DWORD,LPDWORD,LPOVERLAPPED);
extern BOOL WINAPI SOM_MAP_WriteFile(HANDLE,LPCVOID,DWORD,LPDWORD,LPOVERLAPPED);
extern BOOL WINAPI SOM_SYS_WriteFile(HANDLE,LPCVOID,DWORD,LPDWORD,LPOVERLAPPED);

static BOOL WINAPI som_tool_CloseHandle(HANDLE);
static BOOL WINAPI som_tool_CreateDirectoryA(LPCSTR,LPSECURITY_ATTRIBUTES);

//static HBITMAP WINAPI som_main_LoadBitmapA(HINSTANCE,LPCSTR);
extern HANDLE WINAPI som_map_LoadImageA(HINSTANCE,LPCSTR,UINT,int,int,UINT);	
static HIMAGELIST WINAPI som_map_ImageList_LoadImageA(HINSTANCE,LPCSTR,int,int,COLORREF,UINT,UINT);
static HIMAGELIST WINAPI som_tool_ImageList_Create(int A, int B, UINT C, int D, int E)
{
	//2000!=time_machine_mode
	//ILC_COLOR8 is used. but passing a full color image to AddMasked
	//produces junk. something must prepare the images for AddMasked
	if(1&&!som_tool_classic)
	C = ILC_COLORDDB; return SOM::Tool.ImageList_Create(A,B,C,D,E);
}

static BOOL WINAPI som_prm_BitBlt(HDC,int,int,int,int,HDC,int,int,DWORD);
static BOOL WINAPI som_tool_StretchBlt(HDC,int,int,int,int,HDC,int,int,int,int,DWORD);

static BOOL WINAPI som_tool_DrawFocusRect(HDC,const RECT*);

static SHORT som_tool_VK_MENU = 0;
extern SHORT som_tool_VK_CONTROL = 0;
static SHORT WINAPI som_tool_GetKeyState(int vk)
{
	if(vk==VK_MENU&&som_tool_VK_MENU) return som_tool_VK_MENU;
	if(vk==VK_CONTROL&&som_tool_VK_CONTROL) return som_tool_VK_CONTROL;
	
	//assert(vk!=VK_LCONTROL); //Windows 10

	return SOM::Tool.GetKeyState(vk);
}

static BOOL WINAPI som_tool_GetMessageA(LPMSG A,HWND B,UINT C,UINT D);
static BOOL WINAPI som_tool_PeekMessageA(LPMSG A,HWND B,UINT C,UINT D,UINT E);
static BOOL WINAPI som_tool_IsDialogMessageA(HWND A,LPMSG B)
{
	if(SOM::tool==PrtsEdit.exe&&workshop_accel)
	if(TranslateAccelerator(A,workshop_accel,B)) 
	return 1;
	return IsDialogMessageW(A,B); 
}
static LRESULT WINAPI som_tool_DispatchMessageA(CONST MSG*A)
{
	return DispatchMessageW(A);
}
static LONG WINAPI som_tool_SetWindowLongA(HWND A,int B,LONG C)
{
	return SetWindowLongW(A,B,C);
}

static HACCEL WINAPI workshop_LoadAcceleratorsA(HINSTANCE A, LPCSTR B)
{
	extern HACCEL workshop_accelerator(HMENU);
	return workshop_accelerator(workshop_menu);
}
static int WINAPI workshop_TranslateAcceleratorA(HWND A, HACCEL B, LPMSG C)
{
	if(SOM::tool>=EneEdit.exe) return 0; 
	return TranslateAcceleratorW(A,B,C);
}
/*debugging
static BOOL WINAPI workshop_MessageBeep(UINT A)
{
	return SOM::Tool.MessageBeep(A);
}*/

static BOOL WINAPI workshop_SetMenu(HWND A, HMENU B)
{
	if(A==workshop_host)
	{
		//EneEdit/NpcEdit are shown/positioned before
		//their menu is set
		if(!workshop_menu)
		{
			if(SOM::tool==SfxEdit.exe)
			{				
				//00402B30 A3 B0 70 41 00       mov         dword ptr ds:[004170B0h],eax
				DestroyMenu(B);
				B = LoadMenu(0,MAKEINTRESOURCE(129));
				*(HMENU*)0x4170B0 = B;
			}
			workshop_menu = B; assert(B);
			//HACK: timing issues... doing the job of
			//workshop_SetWindowTextA
			ShowWindow(som_tool,1);
			extern wchar_t som_tool_text[MAX_PATH];
			if(!*som_tool_text)
			EnableMenuItem(B,32773,MF_GRAYED);
		}
		A = som_tool;
	}
	return SOM::Tool.SetMenu(A,B);
}
static HMENU WINAPI workshop_GetMenu(HWND A)
{
	//these tools exit (crash?) without their menu
	if(A==workshop_host&&workshop_menu)
	{
		//ObjEdit wants this.
		/*assert(0);*/ return workshop_menu; 
	}
	return SOM::Tool.GetMenu(A);
}

static char *__cdecl som_tool_MapComp_fgets(char *a, int b, char **c)
{
	SOM_MAP.map.error_line++;

	return SOM::Tool.MapComp_fgets(a,b,c);
}
						
extern HOOKPROC som_tool_cbt = 0;
static HHOOK som_tool_cbthook = 0;
static void som_tool_editcell(HWND,HWND);
extern ATOM som_tool_listview(HWND lv=0); 
static ATOM som_tool_listbox(HWND lb=0),som_tool_edittext(HWND ed=0);
static LRESULT CALLBACK som_tool_cbtproc(int code, WPARAM wParam, LPARAM lParam)
{
	//hmm: this is either MFC or a second party layer

	if(code==HCBT_DESTROYWND) //yikes!
	{
		HWND wp = (HWND)wParam;

		//TODO: try implementing som_tool_DestroyWindow
		//wouldn't work in som_tool_subclassproc without 
		//sometimes raising the windows of other programs
		//it would be better to do in som_tool_GetMessageA
		//in theory but there are ways for that to go wrong

		//NOTE: includes workshop_tool/its som_tool_taskbar!
		extern HWND som_tool_taskbar;
		if(GetAncestor(wp,GA_ROOTOWNER)
		==GetAncestor(som_tool,GA_ROOTOWNER))
		if(wp!=som_tool&&wp!=som_tool_taskbar
		&&wp==GetAncestor(som_tool_dialog(),GA_ROOT))
		{	
			HWND gp = GetParent(wp);

			//what's happening here is all of SOM's screens are
			//modeless dialogs which must "wakeup" their parent
			//(that is so you have a single level message loop)
						
			if(gp!=som_tool_taskbar) //wrinkle: SOM_MAIN_151
			{
				EnableWindow(gp,1); SetForegroundWindow(gp); 
			}
		}
	}

	if(code!=HCBT_CREATEWND)
	//return som_tool_cbt(code,wParam,lParam);
	return CallNextHookEx(som_tool_cbthook,code,wParam,lParam);	
					
	HWND w = (HWND)wParam;
	CBT_CREATEWNDW *cw = code==HCBT_CREATEWND?(CBT_CREATEWNDW*)lParam:0;
	const WCHAR *wc = cw->lpcs->lpszClass;
	assert(cw);

	if(WS_CHILD&~cw->lpcs->style)
	{
		//hack: MessageBoxW?
		if(som_tool_cbt&&w!=som_tool&&!IsWindowVisible(som_tool))	
		SOM::splashed(w); 

		/*EXPERIMENTAL
		extern bool som_tool_MessageBoxed;
		if(som_tool_MessageBoxed)
		{
			POINT pt; if(GetCursorPos(&pt)) 
			{
				cw->lpcs->x+=(pt.x-cw->lpcs->x)/8*9-200;
				cw->lpcs->y+=(pt.y-cw->lpcs->y)/8*9-100;
			}
		}*/
	}
	else if((UINT_PTR)wc>=65536) switch(wc[0])
	{
	case 'S': case 'm':
	
		//these must be themed afterward
		if(!wcscmp(wc,L"SysIPAddress32")  //crashes
		||!wcscmp(wc,L"msctls_progress32")) //XP: crashes
		break;

	default: goto theme; //breakpoint (might want to catch things here later)
	}
	else theme:
	{
		//reminder: apparently this is not the best place 
		//to do this. For IP control it crashes and for DataTime
		//controls it can't set the theme of the child updown control
		//also in all of the GetOpenFileName dialogs the scrollbars must be
		//handled individually even though the headers are just fine (go figure)
		SetWindowTheme(w,L"",L""); 
	}
	
	if(cw->lpcs->hInstance!=(HINSTANCE)0x00400000) hide:
	{
		return CallNextHookEx(som_tool_cbthook,code,wParam,lParam);
	}
	else if(!som_tool_cbt) //hack: hide dialog from SOM's hook?
	{
		goto hide; //UNUSED (som_tool_cbt is required below.)
	}

	LONG proc = GetWindowLongW(w,GWL_WNDPROC);
	LRESULT out = som_tool_cbt(code,wParam,lParam);
	LONG proc2 = GetWindowLongW(w,GWL_WNDPROC);	   
	if(cw&&proc!=proc2)
	SetPropW(w,L"A",(HANDLE)proc2);
	return out; 
}
//static bool som_map_rightclick(HWND);
static HOOKPROC som_tool_msgfilter = 0;
static HHOOK som_tool_msgfilterhook = 0;
extern int som_tool_plusminus(int minus);
static bool som_tool_neighbor(HWND,bool,LPARAM,HWND);
static LRESULT CALLBACK som_tool_msgfilterproc(int code, WPARAM wParam, LPARAM lParam)
{
	/*0 is dialog boxes, can ignore rest
	//menu and scrollbar messages are sent to buttons
	//switch(code){ case 2: case 5: goto next; }
	//EneEdit/NpcEdit don't install this hook
	if(code<0) next:*/
	if(code<0) next:
	return CallNextHookEx(som_tool_msgfilterhook,code,wParam,lParam);

	int out = 0; MSG &msg = *(MSG*)lParam;

	switch(msg.message)
	{
	case WM_CHAR:

		//Reminder: must reject to prevent beeps
				
		switch(msg.wParam)
		{
		case '+': case '-': case '=': case '_':
			
			//NOTE: it might be nice to ignore Shift and use Backspace
			//to input equal but it's a little weird and international
			//keyboards may have different conventions

			out = som_tool_plusminus(msg.wParam); break;

		case '>': goto play;
		}		
		break;

	case WM_KEYDOWN: 
		
		switch(msg.wParam)
		{
		case VK_TAB:
		case VK_ESCAPE: 
		case VK_RETURN: //prevent repeat!!

			if(msg.lParam&1<<30) out = 1; 	

			//WTH Windows???!!!! seems VK_TAB just doesn't work anymore
			extern windowsex_TIMERPROC som_tool_repair_tab_navigation_hack;
			if(VK_TAB==msg.wParam)
			SetTimer(0,0,0,som_tool_repair_tab_navigation_hack);

			break;

		case 'A': //select all? 

			if(GetKeyState(VK_CONTROL)>>15)
			if(DLGC_HASSETSEL&SendMessage(msg.hwnd,WM_GETDLGCODE,0,0))
			{
				SendMessage(msg.hwnd,EM_SETSEL,0,-1);
				out = 1;
			}
			else
			{
				ATOM a = GetClassAtom(msg.hwnd);
				if(a==som_tool_listview())
				{
					ListView_SetItemState(msg.hwnd,-1,~0,LVIS_SELECTED);
					out = 1;
				}
				else if(a==som_tool_listbox())
				{
					ListBox_SetSel(msg.hwnd,1,-1);
					out = 1;
				}
			}
			break;

		case VK_F2: //editing labels 

			if(som_tool_edittext()==GetClassAtom(msg.hwnd))
			{
				//doesn't work for SOM_SYS???
				//SetFocus(GetParent(msg.hwnd));
				SendMessage(msg.hwnd,WM_KEYDOWN,VK_RETURN,msg.lParam);
				SendMessage(msg.hwnd,WM_CHAR,'\r',msg.lParam);
				out = 1;
			}
			break;

		case VK_F5: //force refresh (provisional)

			//note: just in case automatic write detection isn't working
			if(SOM::PARAM::onWrite) SOM::PARAM::trigger_write_monitor();

			//REMOVE ME?
			if(SOM::tool==SOM_MAIN.exe) //sync rich edit boxes
			{
				UINT rdw = RDW_ERASE|RDW_INVALIDATE|RDW_ALLCHILDREN;
				RedrawWindow(GetDlgItem(som_tool,1022),0,0,rdw);
				RedrawWindow(GetDlgItem(som_tool,1023),0,0,rdw);
			}
			break;

		//case VK_F6: //Play shortcut? I'm hesitant to use F6
		//because it's too close to F5 and it can accidentally
		//save the map.
		play: //>
		
		case VK_PLAY: case VK_MEDIA_PLAY_PAUSE: //APPCOMMAND_MEDIA_PLAY??

			if(SOM::tool==SOM_MAP.exe
			&&som_tool==GetAncestor(msg.hwnd,GA_ROOT)
			||som_sys_tab==1020)
			{
				SendMessage(som_tool,WM_COMMAND,1080,0);
			}
			break;

	  //WINSANITY //WINSANITY //WINSANITY //WINSANITY //WINSANITY //WINSANITY

		case VK_LEFT: case VK_RIGHT: case VK_DOWN: case VK_UP:

			//HACK: Seems Microsoft has gone and made basic buttons
			//almost entirely unavailable to SetWindowSubclass jobs
			//NOTE: DLGC_BUTTON is excluding radio/checkbox buttons
			if(DLGC_BUTTON&SendMessage(msg.hwnd,WM_GETDLGCODE,0,0))
			{
				//2018: BS_PUSHBUTTON controls NEVER receive WM_KEYDOWN
				//(PrtsEdit & co.)
				//if(EX::debug) MessageBeep(-1); 
				//SendMessage(msg.hwnd,WM_KEYDOWN,msg.wParam,msg.lParam);
				if(~GetKeyState(VK_CONTROL)>>15) 
				{
					som_tool_neighbor(msg.hwnd,msg.wParam==VK_UP||msg.wParam==VK_LEFT,msg.lParam,0);
					out = 1;
				}
				else if(som_map_tileview)
				switch(out=GetDlgCtrlID(msg.hwnd)) //HACK: drive trackbars?
				{
				case 1010: case 1037: case 1038: 
					out = out==1010?1005:1036;
					//msg.hwnd = GetDlgItem(som_map_tileview,1005);
					PostMessage(GetDlgItem(GetParent(msg.hwnd),out),WM_KEYDOWN,msg.wParam,msg.lParam);
					out = 1; break;
				default: out = 0;				
				}
			}
			break;
		}
		break;

		/*2018: can't tell this is helping, but it disables
		//context menus in GetOpenFileName dialogs and fails
		//to disable (likely looked) scroll bar context menus
		//should use subclass procedues to remove context menu
	case WM_RBUTTONUP:
	case WM_RBUTTONDOWN: //reserving for Exselector
	case WM_RBUTTONDBLCLK:	

		out = !som_tool_classic;

		switch(SOM::tool)
		{
		case SOM_MAP.exe: 

			if(som_map_rightclick(msg.hwnd)) out = 0; 
			break;

		case SOM_PRM.exe: 
			
			if(som_prm_model) out = 0;
			break;
		}
		break;*/
	}

	if(!out) 
	{
		if(som_tool_msgfilter)
		return som_tool_msgfilter(code,wParam,lParam);	
		else goto next;
	}
	return 1;
}
static HHOOK WINAPI som_tool_SetWindowsHookExA(int A,HOOKPROC B,HINSTANCE C,DWORD D)
{	
	MEMORY_BASIC_INFORMATION mbi;
	if(!VirtualQuery((void*)0x401000,&mbi,sizeof(mbi))) assert(0);
	if(mbi.AllocationBase!=(void*)0x400000
	||(void*)B<mbi.BaseAddress||(char*)B>=(char*)mbi.BaseAddress+mbi.RegionSize)
	return SOM::Tool.SetWindowsHookExA(A,B,C,D);

	assert(D==GetCurrentThreadId()&&D==EX::threadmain);

	switch(A) //these procs come from deep within the EXE text segment
	{
	default: assert(0);

		return SetWindowsHookExW(A,B,C,D);

	case WH_CBT: assert(!som_tool_cbt&&B); if(!B) return 0; //???

		som_tool_cbt = B; 
		return som_tool_cbthook = SetWindowsHookExW(A,som_tool_cbtproc,C,D);

	case WH_MSGFILTER: assert(!som_tool_msgfilter); //return SetWindowsHookExW(A,B,C,D);

		som_tool_msgfilter = B; 
		return som_tool_msgfilterhook = SetWindowsHookExW(A,som_tool_msgfilterproc,C,D);	
	}	
}

static HMMIO WINAPI som_tool_mmioOpenA(LPSTR,LPMMIOINFO,DWORD);
static int WINAPI som_tool_MessageBoxA(HWND,LPCSTR,LPCSTR,UINT);

const struct SOM::Tool SOM::Tool; //extern
//SOM::Tool initialized
static int &som_tool_tab = SOM::tool==SOM_PRM.exe?som_prm_tab:som_sys_tab;

SOM::Tool::Tool() 
{		
	static int twice = 1; //paranoia	

	if(this==&SOM::Tool&&twice++==1)
	{	
		assert(!SOM::tool);

		switch(SOM::image()) //hack??
		{
		case 'main': SOM::tool = SOM_MAIN.exe; break;
		case 'run':  SOM::tool = SOM_RUN.exe;  break;
		case 'edit': SOM::tool = SOM_EDIT.exe; break;
		case 'prm':  SOM::tool = SOM_PRM.exe;  break;
		case 'map':  SOM::tool = SOM_MAP.exe;  break;
		case 'sys':  SOM::tool = SOM_SYS.exe;  break;
		case 'mpx':  SOM::tool = MapComp.exe;  
			
			//HACK: there's not really anywhere else 
			//to do this
			if(wcschr(Sompaste->get(L".mapcomp.165"),'h'))
			{
				som_map_tileviewmask|=0x100;
			}
			break;

		case 'prts': SOM::tool = PrtsEdit.exe; break;
		case 'item': SOM::tool = ItemEdit.exe; break;
		case 'obj':  SOM::tool = ObjEdit.exe; break;
		case 'ene': SOM::tool = EneEdit.exe; //break;
					if('S'==*(char*)0x414204)
					SOM::tool = SfxEdit.exe; break;
		case 'npc': SOM::tool = NpcEdit.exe; break;		
		}
		if(SOM::tool>=PrtsEdit.exe)
		{			
			wchar_t str[9] = L"0";
			GetEnvironmentVariableW(L"SomEx.dll Workshop Tool",str,9);
			workshop_tool = (HWND)wcstoul(str,0,16);
		}

		memset(this,0x00,sizeof(SOM::Tool));
		
		SendMessageA = ::SendMessageA;	
		PostMessageA = ::PostMessageA;		
		SendDlgItemMessageA = ::SendDlgItemMessageA;
		PostMessageA = ::PostMessageA;
		PostThreadMessageA = ::PostThreadMessageA;
		CallWindowProcA = ::CallWindowProcA;
		ShellExecuteA = ::ShellExecuteA;
		SetWindowTextA = ::SetWindowTextA;
		SetDlgItemTextA = ::SetDlgItemTextA;
		GetWindowTextA = ::GetWindowTextA;
		GetDlgItemTextA = ::GetDlgItemTextA;
		GetWindowTextLengthA = ::GetWindowTextLengthA;
		DialogBoxParamA = ::DialogBoxParamA;
		CreateDialogIndirectParamA = ::CreateDialogIndirectParamA;
		CreateWindowExA = ::CreateWindowExA; 
		DrawTextA = ::DrawTextA;
		TextOutA = ::TextOutA;
		FrameRgn = ::FrameRgn;
		CreateProcessA = ::CreateProcessA;
		CreateProcessW = ::CreateProcessW; //WinHlp32.exe
		RegQueryValueExA = ::RegQueryValueExA;
		GetOpenFileNameA = ::GetOpenFileNameA;
		GetSaveFileNameA = ::GetSaveFileNameA;
		GetMenu = ::GetMenu;
		SetMenu = ::SetMenu;
		SHBrowseForFolderA = ::SHBrowseForFolderA;
		CopyFileA = ::CopyFileA;
		FindFirstFileA = ::FindFirstFileA;
		FindNextFileA = ::FindNextFileA;
		FindClose = ::FindClose;
		CreateFileA = ::CreateFileA;
		ReadFile = ::ReadFile;
		WriteFile = ::WriteFile;
		CloseHandle = ::CloseHandle;	
		CreateDirectoryA = ::CreateDirectoryA;	
		//LoadBitmapA = ::LoadBitmapA;
		LoadImageA = ::LoadImageA;
		if(SOM::tool==SOM_MAP.exe)
		(void*&)ImageList_LoadImageA = //::ImageList_LoadImageA;
		GetProcAddress(EX::comctl32(),"ImageList_LoadImageA");
		(void*&)ImageList_Create = //2018
		GetProcAddress(EX::comctl32(),"ImageList_Create");

		BitBlt = ::BitBlt;
		StretchBlt = ::StretchBlt;
		//monitoring
		SetCurrentDirectoryA = ::SetCurrentDirectoryA;
		SetEnvironmentVariableA = ::SetEnvironmentVariableA;
		CreateMutexA = ::CreateMutexA;
		OpenMutexA = ::OpenMutexA;
		//SOM_MAP
		DeleteFileA = ::DeleteFileA;
		GetFullPathNameA = ::GetFullPathNameA;
		GetFileAttributesA = ::GetFileAttributesA;
		GetDiskFreeSpaceA = ::GetDiskFreeSpaceA;
		GetVolumeInformationA = ::GetVolumeInformationA;
		
		DrawFocusRect = ::DrawFocusRect;
		GetKeyState = ::GetKeyState;

		GetMessageA = ::GetMessageA;
		PeekMessageA = ::PeekMessageA;
		IsDialogMessageA = ::IsDialogMessageA;
		DispatchMessageA = ::DispatchMessageA;
		SetWindowLongA = ::SetWindowLongA;
		//experimental (fyi: used by the tools)
		SetWindowsHookExA = ::SetWindowsHookExA;

		MessageBoxA = ::MessageBoxA;

		mmioOpenA = ::mmioOpenA;

		DeleteObject = ::DeleteObject;

		TranslateAcceleratorA = ::TranslateAcceleratorA;
		LoadAcceleratorsA = ::LoadAcceleratorsA;
		MessageBeep = ::MessageBeep;

		(DWORD&)MapComp_fgets = 0x40c415; //DEBUGGING

		detours = new struct SOM::Tool;
	}
	else if(!SOM::Tool.detours&&twice++==2) 
	{
		//assuming SOM::Tool.detours

		memset(this,0x00,sizeof(SOM::Tool));
		
		if(!SOM::tool) return;
		
		if(1) //0: debugging without Unicode
		{					  
		SendMessageA = som_tool_SendMessageA;
		SetWindowTextA = som_tool_SetWindowTextA;
		SetDlgItemTextA = som_tool_SetDlgItemTextA;
		GetWindowTextA = som_tool_GetWindowTextA;
		GetDlgItemTextA = som_tool_GetDlgItemTextA;
		GetWindowTextLengthA = som_tool_GetWindowTextLengthA;
		SendDlgItemMessageA = som_tool_SendDlgItemMessageA;
		PostMessageA = som_tool_PostMessageA;
		PostThreadMessageA = som_tool_PostThreadMessageA;
		CallWindowProcA = som_tool_CallWindowProcA;		
		SetWindowLongA = som_tool_SetWindowLongA;
		SetWindowsHookExA = som_tool_SetWindowsHookExA;		
		CreateDialogIndirectParamA = som_tool_CreateDialogIndirectParamA;		
		CreateWindowExA = som_tool_CreateWindowExA;		
		//paranoia (A->W)
		GetMessageA = som_tool_GetMessageA;
		PeekMessageA = som_tool_PeekMessageA;
		IsDialogMessageA = som_tool_IsDialogMessageA;
		DispatchMessageA = som_tool_DispatchMessageA;		
		}
		//2018 NOTE: The rectangle isn't shown unless Alt is pressed.
		//Don't know if this is because of SomEx or Windows' changes.
		if(SOM::tool<PrtsEdit.exe)
		DrawFocusRect = som_tool_DrawFocusRect;
		GetKeyState = som_tool_GetKeyState;
		DrawTextA = som_tool_DrawTextA;
		TextOutA = som_tool_TextOutA;	

		 //2018: These seem to be in play?
		//if(EX::debug) //monitoring
		{
		SetCurrentDirectoryA = som_tool_SetCurrentDirectoryA;
		SetEnvironmentVariableA = som_tool_SetEnvironmentVariableA;		
		}
		DeleteFileA = som_tool_DeleteFileA;
		GetFullPathNameA = som_tool_GetFullPathNameA;
		GetDiskFreeSpaceA = som_tool_GetDiskFreeSpaceA;
		GetFileAttributesA = som_tool_GetFileAttributesA;
		GetVolumeInformationA = som_tool_GetVolumeInformationA;

		if(SOM::tool==SOM_MAIN.exe)
		RegQueryValueExA = som_main_etc_RegQueryValueExA;
		if(SOM::tool==SOM_MAIN.exe
		 ||SOM::tool==SOM_RUN.exe
		 ||SOM::tool==SOM_EDIT.exe) //old map import tool
		{		
		SetWindowTextA = som_tool_SetWindowTextA;
		GetWindowTextA = som_tool_GetWindowTextA;
		GetWindowTextLengthA = som_tool_GetWindowTextLengthA;
		GetOpenFileNameA = som_tool_GetOpenFileNameA;	  
		SHBrowseForFolderA = som_tool_SHBrowseForFolderA;
		CreateFileA = som_tool_CreateFileA;
		CreateDirectoryA = som_tool_CreateDirectoryA;
		}				  		
		if(SOM::tool==SOM_RUN.exe
		 ||SOM::tool==SOM_MAIN.exe)
		{
			//SOM_EDIT doesn't use CopyFile
			CopyFileA = som_main_CopyFileA;				

			//addressing with SOM_MAIN_reprogram
		//	if(EX::debug)
		//	LoadBitmapA = som_main_LoadBitmapA;
		}		

		if(SOM::tool==SOM_MAIN.exe
		  ||SOM::tool==SOM_RUN.exe
		  ||SOM::tool==SOM_EDIT.exe				  
		  ||SOM::tool==SOM_SYS.exe)
		ShellExecuteA = som_tool_ShellExecuteA;
		if(SOM::tool==SOM_MAP.exe) 
		CreateProcessA = som_tool_CreateProcessA;		
		CreateProcessW = som_tool_CreateProcessW; //WinHlp32.exe

		if(SOM::tool==SOM_RUN.exe
		 ||SOM::tool==SOM_PRM.exe
		 ||SOM::tool==SOM_MAP.exe
		 ||SOM::tool==SOM_SYS.exe
		 ||SOM::tool==MapComp.exe
		 ||SOM::tool==SOM_EDIT.exe) //old map import tool
		{		
		FindFirstFileA = som_tool_FindFirstFileA;
		FindNextFileA = som_tool_FindNextFileA;
		FindClose = som_tool_FindClose;
		CreateFileA = som_tool_CreateFileA;
		ReadFile = som_tool_ReadFile;
		CloseHandle = som_tool_CloseHandle; 		
		}
		if(SOM::tool==SOM_MAP.exe)
		{
		WriteFile = SOM_MAP_WriteFile;		
		FrameRgn = som_map_FrameRgn;
		LoadImageA = som_map_LoadImageA;
		ImageList_LoadImageA = som_map_ImageList_LoadImageA;
		}
	//	else if(SOM::tool==SOM_SYS.exe)
	//	WriteFile = SOM_SYS_WriteFile;

		if(SOM::tool>=EneEdit.exe) //icon/skin with BMP
		{
			LoadImageA = som_map_LoadImageA;
		}

		//Too soon to initialize EX::INI::Editor //2018
		//if(2000!=EX::INI::Editor()->time_machine_mode)	
		ImageList_Create = som_tool_ImageList_Create;

		if(SOM::tool==SOM_MAP.exe
		 ||SOM::tool==SOM_PRM.exe)
		StretchBlt = som_tool_StretchBlt;
		//NEW: makes things generally look better
		if(SOM::tool<PrtsEdit.exe) //2018
		StretchBlt = som_tool_StretchBlt;
//		if(EX::debug)
//		if(SOM::tool==SOM_PRM.exe)
//		BitBlt = som_prm_BitBlt; //testing

		if(SOM::tool==SOM_MAP.exe
		 ||SOM::tool==SOM_SYS.exe)
		mmioOpenA = som_tool_mmioOpenA;

		MessageBoxA = som_tool_MessageBoxA;

		//NEW: disable locks
		CreateMutexA = som_tool_CreateMutexA;
		OpenMutexA = som_tool_OpenMutexA;

		//IP Address Control deletes
		//our fonts when it is destroyed?!?
		if(SOM::tool==SOM_MAIN.exe)
		DeleteObject = som_tool_DeleteObject;

		if(SOM::tool>=PrtsEdit.exe) //workshop.cpp
		{
			RegQueryValueExA = som_main_etc_RegQueryValueExA;

			GetOpenFileNameA = som_tool_GetOpenFileNameA;	
			GetSaveFileNameA = workshop_GetSaveFileNameA;
			GetMenu = workshop_GetMenu;
			SetMenu = workshop_SetMenu;
			CreateFileA = workshop_CreateFileA;
			ReadFile = workshop_ReadFile;
			SetWindowTextA = workshop_SetWindowTextA;
			if(SOM::tool>=EneEdit.exe)
			{
				//non MFC app
				CreateWindowExA = som_tool_CreateWindowExA;		
				DialogBoxParamA = workshop_DialogBoxParamA;
			}
				
			//absolute paranoia, so SOMEX_(A) isn't
			//passed into FindFirstFile's API
			FindFirstFileA = som_tool_FindFirstFileA;
			//FindNextFileA = som_tool_FindNextFileA;
			//FindClose = som_tool_FindClose;

			LoadAcceleratorsA = workshop_LoadAcceleratorsA;
			TranslateAcceleratorA = workshop_TranslateAcceleratorA;
		}

		if(SOM::tool==MapComp.exe)
		{
			MapComp_fgets = som_tool_MapComp_fgets; //DEBUGGING
		}
	}
	else assert(0); //this can't be happening
}

static DWORD som_tool_help = 0;
extern DWORD som_map_instruct = 0;
static const char *som_tool_find = 0;
extern wchar_t som_tool_text[MAX_PATH] = L"";

//2018: Was going to use this for extensions. \ doesn't work with #???
#define som_tool_4_letter_word_(word,p) (*(DWORD*)(p)==*(DWORD*)/*#*/word)
static bool som_tool_data(const char *p)
{
	return som_tool_4_letter_word_("DATA",p)||som_tool_4_letter_word_("data",p);
}
const wchar_t *SOM::Tool::data(const char *in, bool ok)
{
	//static size_t a_s = 0;
	enum{ a_s=sizeof(SOMEX_(A)"\\data")-1 }; //2021
	static size_t w_s = 0; 	

	//static char a[MAX_PATH] = "";
	static char a[a_s+1] = "";
	static wchar_t w[MAX_PATH] = L"";	

	if(!*w)  
	{	
		w_s = wcslen(wcscpy(w,L"DATA"));
		//a_s = strlen(strcpy(a,SOMEX_(A)"\\data"));
		memcpy(a,SOMEX_(A)"\\data",a_s+1);
		assert(som_tool_data(a+sizeof(SOMEX_(A))));
	}

	w[w_s] = '\0'; if(!in) return w;
		
	if(!som_tool_data(in)||in[4]!='\\') 
	if(strnicmp(in,a,a_s)||in[a_s]!='\\')
	{
		return 0; 
	}
	else in+=a_s; else in+=4;

	int i,j;
	for(i=0,j=w_s;in[i];i++,j++) if(in[i]>127)
	{
		return 0; //todo: warn about non-ASCII file names
	}
	else w[j] = in[i]; w[j] = '\0'; 
	
		/*2022
		//map_442440_407c60 does this (better) now
		//MapComp should have it's own way as well
	//SOM_MAP_165
	//technically .MHM shouldn't have to be the extension
	//mapcomp.exe SHOULD SET UP som_map_tileviewmask ALSO
	if(0x100&som_map_tileviewmask
	&&som_tool_4_letter_word_("\\map",in)
	&&som_tool_4_letter_word_("\\msm",in+4))
	w[w_s+6] = 'h'; //"\map\msm"->"\map\mhm"
		*/

	return w;
}

const wchar_t *SOM::Tool::install(const char *in) 
{
	enum{ a_s=sizeof(SOMEX_(A))-1 }; //2021
	//static size_t a_s = 0;
	static size_t w_s = 0;

	//static char a[MAX_PATH] = "";
	static char a[a_s+1] = "";
	static wchar_t w[1+MAX_PATH] = L"";

	if(!w_s) //A: InstDir
	{
		//a_s = strlen(strcpy(a,SOMEX_(A)));
		memcpy(a,SOMEX_(A),a_s+1);
		w_s = GetEnvironmentVariableW(L".NET",w,MAX_PATH); 		
	}
	if(!w_s) return 0;

	w[w_s] = '\0'; if(!in) return w;

	if(strnicmp(in,a,a_s)) return 0;
	
	int i,j;
	for(i=a_s,j=w_s;in[i];i++,j++) if(in[i]>127)
	{
		return 0; //todo: warn about non-ASCII file names
	}
	else w[j] = in[i]; w[j] = '\0'; return w;
}

extern int som_LYT_base(int);
static bool som_tool_project = false;
const wchar_t *SOM::Tool::project(const char *in, bool ok) 
{	
	//static size_t a_s = 0
	enum{ a_s=sizeof(SOMEX_(B))-1 }; //2021
	static size_t w_s = 0, x_s = 0;

	//static char a[MAX_PATH] = "";
	static char a[a_s+1] = ""; //2021
	static wchar_t w[MAX_PATH] = L"", x[MAX_PATH] = L"";

	if(!som_tool_project)
	if(SOM::tool==SOM_MAIN.exe
	 ||SOM::tool==SOM_RUN.exe)
	{	
		//a_s = strlen(strcpy(a,SOMEX_(B)));
		memcpy(a,SOMEX_(B),a_s+1);
		//Reminder: prior to establishing the .SOM file
		if(GetEnvironmentVariableW(L".SOM",w,MAX_PATH)) //if: SOM_MAIN
		w[w_s=wcsrchr(w,'\\')-w] = '\0';	   
		som_tool_project = true;		
	}
	else 
	{			
		w_s = wcslen(wcscpy(w,EX::cd()));		
		//a_s = strlen(strcpy(a,SOMEX_(B))); //B: Project
		memcpy(a,SOMEX_(B),a_s+1);

		if(wcscmp(w,EX::user(0))) //NEW
		{
			x_s = wcscpy(x,EX::user(0))-x;
		}

		som_tool_project = true; 
	}
	if(!w_s) return 0; //???

	w[w_s] = '\0'; if(!in) return w;

	if(strnicmp(in,a,a_s)) return 0;
	
	if(in[a_s]=='\0') return w;
	if(in[a_s]!='\\') return 0;

	w[w_s] = '\\';

	bool map_based = false;
	int i,j, allcaps = w_s; 	
	for(i=a_s,j=w_s;in[i];i++,j++) if(in[i]>127)
	{
		return 0; //todo: warn about non-ASCII file names
	}
	else if((w[j]=in[i])=='.') //REMOVE ME?
	{
		//YUCK: SomEx CODE MIGHT PREFER SOM::Tool.data TO
		//AVOID THESE OLD TRICKS

		wchar_t ext[4] = {};
		for(int k=0;k<4;k++) 
		if(L'\0'==(ext[k]=tolower(in[i+1+k]))) 
		break;
		
		if(0==wmemcmp(ext,L"dat",4))
		{
			allcaps = j+4;
		}
		else if(0==wmemcmp(ext,L"som",4))
		{
			return Sompaste->get(L".SOM",0);
		}		
		else if('m'==ext[0]||'e'==ext[0]) //mpx,map,evt?
		{	
			//TODO: THIS IS A PROBLEM FOR som.art.cpp AND
			//SOM_MAP.cpp WHEN WORKING WITH MULTIPLE MAPS

			if(SOM::tool==SOM_MAP.exe)
			if(!wcsnicmp(ext,L"evt",4)
			 ||!wcsnicmp(ext,L"mpx",4)) map_based = true;

			enum{ times=2+2 }; //FindFirstFileA+CreatFileA

			//2017: trying to load SOM_MAP from the command-line
			//or eventually the registry
			static int one_off = SOM::tool==SOM_MAP.exe?0:times;			
			if(one_off<times&&ext[1]!='z') //avoid sys.ezt
			{
				one_off++;
				//HACK: it's not ideal for this routine to change
				//the file name itself, but the alternative is to
				//scan every file at the top of CreateFileA
				i-=2; j-=2; assert(in[i-1]=='\\');

				//better safe than sorry
				//note: hit this with MPX when it wasn't prepared
				//for testing... unsure if MPX should be added or
				//if it's a potential problem to enter this block
				assert(!wcscmp(ext,L"map")||!wcscmp(ext,L"evt"));
			}
			if(in[i-1]=='\\'||in[i-1]=='/') //Of the form "/.ext"?
			{
				//replace the map related file with the .MAP evironment
				//variable. if not defined use 00, or registry, or if it
				//is ever possible to use non two-digit names, a strategy
				DWORD len = GetEnvironmentVariableW(L".MAP",w+j,MAX_PATH-j);
				if(len!=2) //First?
				{
					//note, anything more than 2 digits will make problems
					if(len==0) 
					len = GetEnvironmentVariableW(L"TRIAL",w+j,MAX_PATH-j);

					if(len==1&&isdigit(w[j])) //courtesy
					{
						w[j+1] = w[j]; w[j] = '0'; w[j+2] = '\0'; //0 prefix
					}
					else if(!isdigit(w[j])||!isdigit(w[j+1]))
					{
						w[j] = w[j+1] = '0'; w[j+2] = '\0'; //default to 00
					}

					//2018: just setting .MAP in case there is no MAP file
					//and so able to utilize SOM_MAP_201_open()					
					SetEnvironmentVariableW(L".MAP",w+j);
				}
				j+=2;
				w[j] = '.'; wmemcpy(w+j+1,ext,4);
				j+=4;
				break;
			}
			else if(!wmemcmp(ext,L"mpx",4))
			{
				//2018: checking for existence of MPX prior to playtesting?
				const wchar_t *map = Sompaste->get(L".MAP");
				if(map[0]!='\0')
				{
					w[j-2] = map[0]; w[j-1] = map[1];
					wmemcpy(w+j+1,L"mpx",4); 
					j+=4; break;
				}
			}
		}
		else if('p'==ext[0])
		{				
			if(!wmemcmp(ext,L"pr2",4))
			{
				wmemcpy(w+j+1,L"PRO",4); 
				j+=4; break;
			}
			if(!wmemcmp(ext,L"prm",4))
			{
				allcaps = j+4;
			}
		}
		else if(i>0&&in[i-1]=='*') 
		{
			switch(ext[0]) //FindFirstFile
			{		
			default: continue;			
			case 'b': if(0!=wmemcmp(ext,L"bmp",4)) continue; break;
			case 'a': if(0!=wmemcmp(ext,L"avi",4)) continue; break;
			case 'm': if(0!=wmemcmp(ext,L"mid",4)) continue; break;
			case 'r': if(0!=wmemcmp(ext,L"rmi",4)) continue; break;
			case 's': if(0!=wmemcmp(ext,L"snd",4)) continue; break;
			case 'w': if(0!=wmemcmp(ext,L"wav",4)) continue; break;
			}
			//FindFirstFile is expected to hide * returns			
			return wcscpy(w+j,L"*");
		}
	}  
	else if(w[j]=='\\') allcaps = j;

	w[j] = '\0'; //!

	if(map_based) //basic layers system
	{
		int base = som_LYT_base(_wtoi(w+j-6));		
		if(base!=-1) w[j-6] = '0'+base/10;
		if(base!=-1) w[j-5] = '0'+base%10;
	}
		
	//2021: this is old code but I think it's just
	//standardizing to all-caps to look consistent
	for(j=w_s;j<allcaps;j++) w[j] = toupper(w[j]);

	//NEW: prefer copy of file in user's folder
	return x_s&&PathFileExistsW(wcscpy(x+x_s,w+w_s))?x:w;
}
//todo: merge runtime/project2?
static bool som_tool_project2 = false;
static wchar_t som_edit_project2[MAX_PATH] = L"";
//HACK: These MAX_PATH globals probably shouldn't be
//but might as well repurpose them wherever possible
extern wchar_t (&workshop_savename)[MAX_PATH] = som_edit_project2;
const wchar_t *SOM::Tool::project2(const char *in) 
{		
	//static size_t a_s = 0
	enum{ a_s=sizeof(SOMEX_(D))-1 }; //2021
	static size_t w_s = 0;

	//static char a[MAX_PATH] = "";
	static char a[a_s+1] = ""; //2021
	static wchar_t w[MAX_PATH] = L"";

	if(!som_tool_project2)
	if(SOM::tool==SOM_EDIT.exe)
	{	
		//a_s = strlen(strcpy(a,SOMEX_(D)));
		memcpy(a,SOMEX_(D),a_s);
		w_s = wcslen(wcscpy(w,som_edit_project2));
		som_tool_project2 = true;		
	}
	else assert(0);

	if(!w_s) return 0; 

	w[w_s] = '\0'; if(!in) return w;

	if(strnicmp(in,a,a_s)) return 0;

	if(in[a_s]=='\0') return w;
	if(in[a_s]!='\\') return 0;

	w[w_s] = '\\';

	if(in[a_s+1]=='.')
	if(!strcmp(in+a_s+1,".som"))
	{
		assert(!wcsncmp(som_tool_text,w,w_s));
		wcscpy(w+w_s,som_tool_text+w_s);
		return w;
	}

	int i,j;
	for(i=a_s,j=w_s;in[i];i++,j++) if(in[i]>127)
	{
		return 0; //todo: warn about non-ASCII file names
	}
	else if((w[j]=in[i])=='.')
	{
		if(!strnicmp(in+i,".pr2",5))
		{
			wcscpy_s(w+j,MAX_PATH-j,L".pro"); j+=4; break;
		}
	}

	w[w_s+i-a_s] = '\0'; return w;
}
//todo: merge runtime/project2?
static bool som_tool_runtime = false;
const wchar_t *SOM::Tool::runtime(const char *in) 
{		
	//static size_t a_s = 0
	enum{ a_s=sizeof(SOMEX_(C))-1 }; //2021
	static size_t w_s = 0;

	//static char a[MAX_PATH] = "";
	static char a[a_s+1] = ""; //2021
	static wchar_t w[MAX_PATH] = L"";

	if(!som_tool_runtime)
	if(SOM::tool==SOM_RUN.exe)
	{	
		//a_s = strlen(strcpy(a,SOMEX_(C)));
		memcpy(a,SOMEX_(C),a_s);
		w_s = GetEnvironmentVariableW(L".RUN",w,MAX_PATH); //hack
		som_tool_runtime = true;
	}
	else assert(0);

	if(!w_s) return 0; 

	w[w_s] = '\0'; if(!in) return w;

	if(strnicmp(in,a,a_s)) return 0;

	if(in[a_s]=='\0') return w;
	if(in[a_s]!='\\') return 0;

	w[w_s] = '\\';

	int i,j;
	for(i=a_s,j=w_s;in[i];i++,j++) if(in[i]>127)
	{
		return 0; //todo: warn about non-ASCII file names
	}
	else if((w[j]=in[i])=='.')
	{
		if(!strnicmp(in+i,".pr2",5))
		{
			wcscpy_s(w+j,MAX_PATH-j,L".pro"); j+=4; break;
		}
	}

	w[w_s+i-a_s] = '\0'; return w;
}

typedef SWORDOFMOONLIGHT::zip::inflate_t som_tool_xdata_t;

extern size_t som_tool_xdata(const wchar_t *x, som_tool_xdata_t &inf)
{		
	namespace zip = SWORDOFMOONLIGHT::zip;

	struct datapack
	{
		size_t unzipped_s;
		wchar_t unzipped[MAX_PATH]; zip::mapper_t zipped;
		datapack(){ zipped = 0; unzipped_s = 0; *unzipped = 0; }
		void inflate(const wchar_t *x, som_tool_xdata_t &inf)
		{
			if(zipped)
			{
				zip::image_t unzip;
				zip::maptolocalentry(unzip,zipped,x);				
				//2018: inf.restart is compressed size
				inf.restart = zip::inflate(unzip,inf);
				zip::unmap(unzip); 
			}
			else
			{
				wcscpy_s(unzipped+unzipped_s,MAX_PATH-unzipped_s,x);
				HANDLE f = CreateFileW(unzipped,SOM_TOOL_READ);
				int compile[sizeof(inf.restart)==sizeof(DWORD)];
				if(!ReadFile(f,inf,sizeof(inf.dictionary),(DWORD*)&inf.restart,0)) inf.restart = 0;
				CloseHandle(f);
			}
		}
	};
	static std::vector<datapack> out;
	static bool one_off = false; if(!one_off++) //???
	{
		for(int i=0;*EX::text(i);i++)
		{
			datapack dp; 

			if(!PathIsDirectoryW(EX::text(i)))
			{
				if(!zip::open(dp.zipped,EX::text(i),L"data")) 
				{
					zip::close(dp.zipped); dp.zipped = 0;
				}
			}
			else if(0<swprintf_s(dp.unzipped,L"%ls\\data",EX::text(i)))
			{
				if(PathIsDirectoryW(dp.unzipped))
				dp.unzipped_s = wcslen(dp.unzipped)-4; //-data
			}
			if(dp.zipped||dp.unzipped_s) out.push_back(dp);
		}
	}
	//HACK: inf.restart returns the UNCOMPRESSED size
	//NOTE: prior to 2017 it was return the COMPRESSED size (erroneously)
	inf.restart = 0;
	for(size_t i=0;i<out.size()&&!inf.restart;i++) out[i].inflate(x,inf);
	return inf.restart; 
}

static bool som_tool_zcopy(const wchar_t *dst, const wchar_t *text, const wchar_t *src)
{
	bool out = false;

	namespace zip = SWORDOFMOONLIGHT::zip;

	//Reminder: it would be nice to test the
	//timestamps before opening up text, but
	//we don't know if text contains the src
	zip::mapper_t z; if(zip::open(z,text,src))
	{	
		out = true;

		struct ft : FILETIME
		{
			ft(const wchar_t *fp)
			{
				dwLowDateTime = dwHighDateTime = 0;
				HANDLE f = CreateFileW(fp,SOM_TOOL_READ);
				GetFileTime(f,0,0,this);
				CloseHandle(f);
			}

		}t1(text), t2(dst);

		if(CompareFileTime(&t1,&t2))
		{
			//assert(!"performing xcopy");

			zip::image_t unzip; zip::maptolocalentry(unzip,z,src);

			EXLOG_HELLO(0) << "Extracting " << Ex%src << " from " << Ex%text << " to " << Ex%dst << "...\n";

			EX::clearing_a_path_to_file(dst);

			HANDLE f = CreateFileW(dst,GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0);
			
			//stack overflow in SOM_MAIN?
			//zip::inflate_t swap = {0,0};
			zip::inflate_t *swap = new zip::inflate_t();
			//for(DWORD wr;(wr=zip::inflate(unzip,swap))&&WriteFile(f,swap,wr,&wr,0););
			for(DWORD wr;(wr=zip::inflate(unzip,*swap))&&WriteFile(f,*swap,wr,&wr,0););
			CloseHandle(f);
			delete swap; //2023

			DWORD fs = 
			//must reopen the timestamp is not overwritten by CloseHandle
			GetFileSize(f=CreateFileW(dst,FILE_WRITE_ATTRIBUTES,0,0,OPEN_EXISTING,0,0),0);
			if(!SetFileTime(f,0,0,&t1)) assert(0);			
			CloseHandle(f);
			
			zip::header_t &hd = zip::imageheader(unzip);

			if(!hd||hd.filesize!=fs) //paranoia
			{		
				EXLOG_ALERT(0) 	<< "Failed to extract file from archive\n";

				DeleteFileW(dst); assert(0); //bad TEMP directory???

				out = false;
			}				
			zip::unmap(unzip); 
		}										
	}
	zip::close(z); return out;
}

extern void SOM::xtool(wchar_t dst[MAX_PATH], const wchar_t *src)
{
	bool PrtsEdit_etc = false;
	const wchar_t *sep = PathFindExtensionW(src);	
	const wchar_t *ext = *sep?L"":L".exe",*x = 0;
	while(sep!=src&&*sep!='\\'&&*sep!='/') sep--; if(sep!=src) sep++;
	wchar_t tool[32] = L""; swprintf_s(tool,L"tool\\%ls%ls",sep,ext);	
	if(!wcsnicmp(tool+5,L"SOM_",4)&&wcsnicmp(tool+5+4,L"DB",2)
	||(PrtsEdit_etc=wcsstr(tool+5,L"Edit")))
	{
		if(PrtsEdit_etc)
		if(!wcsnicmp(tool+5,L"SfxEdit",7))
		{
			//TODO? NpcEdit can be merged into EneEdit with some work
			//(SfxEdit is a complete fabrication)
			wmemcpy(tool+5,L"Ene",3);
		}

		const wchar_t *text; 
		for(int i=0;*(text=EX::text(i));i++)
		{
			if(!PathIsDirectoryW(text)) //zipped
			{					
				swprintf_s(dst,MAX_PATH,
				L"%ls\\Swordofmoonlight.net\\xtool\\%ls",Sompaste->get(L"TEMP"),tool+5);
				if(!som_tool_zcopy(dst,text,tool)) continue;				
			}
			else swprintf_s(dst,MAX_PATH,L"%ls\\%ls",text,tool); //unzipped

			if(*dst&&PathFileExistsW(dst)) return;
		}
	}		
//	if(!PrtsEdit_etc) //2023
	swprintf_s(dst,MAX_PATH,L"%ls\\%ls",Sompaste->get(L".NET"),tool);		
}

extern bool SOM::xcopy(const wchar_t *dst, const wchar_t *src)
{		
	wchar_t wdst[MAX_PATH] = L"";
	//TODO: check if path is relative?
	swprintf_s(wdst,L"%ls\\%ls",SOM::Tool::project(),dst);
	const wchar_t *text;
	for(int i=0;*(text=EX::text(i));i++)
	{
		if(PathIsDirectoryW(text)) //unzipped
		{
			wchar_t wsrc[MAX_PATH] = L"";
			swprintf_s(wsrc,L"%ls\\%ls",text,src);
			EX::clearing_a_path_to_file(wdst);
			if(CopyFileW(wsrc,wdst,0)) return true;
		}
		else if(som_tool_zcopy(wdst,text,src)) return true;
	}
	wchar_t wsrc[MAX_PATH] = L"";
	swprintf_s(wsrc,L"%ls\\%ls",SOM::Tool::install(),src);
	EX::clearing_a_path_to_file(wdst);
	return CopyFileW(wsrc,wdst,0);	
}

static HWND som_sys_openscript = 0;
extern ATOM som_tool_richtext(HWND=0);
static HWND som_tool_groupitembyid(HWND dlg, HWND start, int id)
{
	HWND ctl = start; 
	do{ ctl = GetNextDlgGroupItem(dlg,ctl,0); }
	while(ctl!=start&&id!=GetDlgCtrlID(ctl));
	return ctl==start?0:ctl;
}
static void som_tool_openscript_enable(HWND on, int how=-1)
{
	HWND bt = GetDlgItem(on,2000); if(bt) if(how>=0)
	EnableWindow(bt,how<=1?how:IsWindowEnabled(GetDlgItem(on,how)));
	else InvalidateRect(bt,0,0);
}
static void som_tool_openscript(HWND on, HWND of)
{
	HWND ed = 0;
	if(som_tool_richtext()==GetClassAtom(of))
	{
		ed = of; assert(GetKeyState(VK_F2)>>15);
	}
	else switch(GetWindowContextHelpId(on))
	{
	case 30000: //name (1004 is shops)

		ed = GetDlgItem(som_tool_dialog(),1029+(1004==som_prm_tab)); 
		break;

	//Truth Glass lore (1005 is enemies)
	case 31100: case 33100: case 35300: case 36100:
	
		ed = som_tool_groupitembyid(on,of,1005==som_prm_tab?1103:1032); 
		break;	
		
	case 44000: //Map

		ed = som_tool_groupitembyid(on,of,1132); 
		break;

	case 45100: //Event

		ed = som_tool_groupitembyid(on,of,1024); 
		break;

	case 46100: //Text

		ed = som_tool_groupitembyid(on,of,1173); 
		break;

	case 47700: //Menu

		ed = som_tool_groupitembyid(on,of,1173); 
		if(!ed) ed = som_tool_groupitembyid(on,of,1197);
		if(!ed) ed = som_tool_groupitembyid(on,of,1198);
		break;
	
	case 45200:	//Counters

		ed = som_tool_groupitembyid(on,of,1254); 
		break;	

	case 50000: //SOM_SYS

		switch(som_sys_tab)
		{
		case 1017: case 1019: //classes & messages

			ed = som_sys_openscript;
		}
		break;

	case 56200: 

		ed = som_tool_groupitembyid(on,of,1082);
		break;

	default: assert(0);
	}
	if(!ed) return; 

	using SOM::MO::view; if(!view) 
	{		
		//todo: Editor extension
		const size_t megabytes = 1; 
		size_t bytes = megabytes*1024*1024;
		const wchar_t *mo = Sompaste->get(L".MO"); 
		assert(*mo); if(!*mo) return;
		SOM::MO::memoryfilename name(mo);
		const DWORD access = FILE_MAP_WRITE|FILE_MAP_READ;
		HANDLE preexists = OpenFileMapping(access,0,name); 
		HANDLE fm = preexists; if(!fm) fm = CreateFileMapping
		(INVALID_HANDLE_VALUE,0,PAGE_READWRITE,0,sizeof(*view)+bytes,name);			
		(void*&)view = MapViewOfFile(fm,access,0,0,0);
		assert(view); if(!view) return; if(!preexists)
		{
			new(view)SOM::MO::memoryfilebase;
			wcscpy_s(view->writetofilename,mo);
			view->megabytes = megabytes;
		}
	}
	view->clientext = ed; 
	//IsWindow: in case terminated by a debugger
	if(IsWindow(view->server))
	{
		HWND ga = GetAncestor(view->server,GA_ROOTOWNER);
		SendMessage(ga,WM_SYSCOMMAND,SC_RESTORE,0);
		SetForegroundWindow(view->server);
	}
	else SOM::exe(L"SOM_MAIN");
}
static void som_tool_openfolder(HWND on, WPARAM id) //library
{		
	id-=2001; if(id>2) return; /*if(id==2) //som_rt.exe?
	{
		//old SOM_RT based projects have sound/bgm folders
		//todo: may want to ask to move the old files over
		const wchar_t *bgm = SOM::Tool.project(SOMEX_(B)"\\DATA\\SOUND\\BGM");	
		if(PathFileExists(bgm)) ShellExecuteW(on,L"explore",bgm,0,0,1);
	}*/
	const char *a[4] = //todo? check for/open user folders
//	{SOMEX_(B)"\\DATA\\PICTURE",SOMEX_(B)"\\DATA\\MOVIE",SOMEX_(B)"\\DATA\\BGM"};
//	ShellExecuteW(on,L"explore",SOM::Tool.project(a[id]),0,0,1);
	{"PICTURE","MOVIE","BGM","SOUND\\BGM"};
	const wchar_t *data; wchar_t w[MAX_PATH]; 
	sound:
	for(int i=0;*(data=EX::data(i));i++)
	{
		swprintf_s(w,L"%ls\\%hs",data,a[id]);	
		ShellExecuteW(on,L"explore",w,0,0,1);
	}
	if(id==2){ id = 3; goto sound; }
}

static HINSTANCE STDAPICALLTYPE som_tool_ShellExecuteA(HWND hwnd, LPCSTR verb, LPCSTR file, LPCSTR args, LPCSTR dir, INT show)
{	
	SOM_TOOL_DETOUR_THREADMAIN(ShellExecuteA,hwnd,verb,file,args,dir,show)

	EXLOG_LEVEL(7) << "som_tool_ShellExecuteA()\n";
	
	DWORD out = 0;

	if(*(DWORD*)file==*(DWORD*)"SOM_")	
	{
		wchar_t ex_args[MAX_PATH*2+15] = L"";

		if(!stricmp(file+4,"DB.exe"))
		{
			if(som_tool_play==IDNO)
			{
				som_tool_play = 0; return (HINSTANCE)33;
			}

			//wchar_t f[] = L"%S \"%s\" \"%s\"";				
			wchar_t f[] = L"%hs \"%ls\" \"%ls\" %hs";
								
			const char *slideshow = 
			SOM::tool==SOM_SYS.exe?strchr(args,'/'):0;
			if(!slideshow) slideshow = "";

			swprintf_s(ex_args,f,file,SOMEX_L(B),SOMEX_L(A),slideshow);			
						
			return (HINSTANCE)(SOM::exe(ex_args)?33:SE_ERR_FNF);
		}			 
		if(!stricmp(file+4,"MAIN.exe")
		 ||!stricmp(file+4,"EDIT.exe")
		 ||!stricmp(file+4,"PRM.exe")
		 ||!stricmp(file+4,"MAP.exe")		 
		 ||!stricmp(file+4,"SYS.exe")
		 ||!stricmp(file+4,"RUN.exe"))
		{
			if(SOM::tool==SOM_RUN.exe) //SOM_MAIN.exe			
			if(Sompaste->get(L".MO",0)) return (HINSTANCE)33;

			if(args&&*args)
			{
				//assuming "<instdir>"/"<project>"
				wchar_t f[] = L"%hs \"%ls\"/\"%ls\"";
				
				if(!stricmp(file+4,"RUN.exe")) wcscpy(f,L"%hs \"%ls\""); 

				swprintf_s(ex_args,f,file,SOMEX_L(A),SOMEX_L(B));
			}
			else swprintf_s(ex_args,L"%hs",file);

			if(SOM::tool==SOM_EDIT.exe)
			if(!stricmp(file+4,"MAIN.exe"))	
			SetEnvironmentVariableW(L".MO",0); //will be closing
			
			return (HINSTANCE)(SOM::exe(ex_args)?33:SE_ERR_FNF);
		}
	}

	if(!out) return SOM::Tool.ShellExecuteA(hwnd,verb,file,args,dir,show);

	CloseHandle(SOM::exe_process); 
	CloseHandle(SOM::exe_thread);

	return (HINSTANCE)out;
} 

static BOOL WINAPI som_tool_CreateProcessW(LPCWSTR W, LPWSTR B, LPSECURITY_ATTRIBUTES C, LPSECURITY_ATTRIBUTES D, BOOL E, DWORD F, LPVOID G, LPCWSTR H, LPSTARTUPINFOW I, LPPROCESS_INFORMATION J)
{
	SOM_TOOL_DETOUR_THREADMAIN(CreateProcessW,W,B,C,D,E,F,G,H,I,J)

	//2020: Windows 10 opens its browser on F1
	//(HelpPane.exe via C:\Windows\winhlp32.exe)
	if(!wcsnicmp(PathFindFileName(W),L"winhlp32",8)) //"winhlp32.exe -x"
	{
		//may hook this if I can ever find time to work on a help system
		//but not on F1
		SetLastError(ERROR_NOT_SUPPORTED); return 0; 
	}
	return SOM::Tool.CreateProcessW(W,B,C,D,E,F,G,H,I,J);
}
extern char SOM_MAP_layer(WCHAR[]=0,DWORD=0);
static BOOL WINAPI som_tool_CreateProcessA(LPCSTR A, LPSTR B, LPSECURITY_ATTRIBUTES C, LPSECURITY_ATTRIBUTES D, BOOL E, DWORD F, LPVOID G, LPCSTR H, LPSTARTUPINFOA I, LPPROCESS_INFORMATION J)
{				 	
	SOM_TOOL_DETOUR_THREADMAIN(CreateProcessA,A,B,C,D,E,F,G,H,I,J)

	EXLOG_LEVEL(7) << "som_tool_CreateProcessA()\n";

	char *str; 
	if(!(str=strstr(B,"som_db"))&&!(str=strstr(B,"mapcomp"))) 
	{
		return SOM::Tool.CreateProcessA(A,B,C,D,E,F,G,H,I,J);	
	}
		
	//HACK: basic layer system must switch to base map
	WCHAR map[6] = {};
	int base = SOM_MAP_layer(map,3);
	if(SOM::tool==SOM_MAP.exe)
	{
		base = som_LYT_base(base);
		if(-1!=base) map[2] = '0'+base/10;
		if(-1!=base) map[3] = '0'+base%10;			
	}

	wchar_t cli[MAX_PATH*2]; if(*str=='m') //mapcomp?
	{	
		//if(EX::debug) return 0; //testing
		wchar_t ex[4] = L"H";
		if(0!=som_tool_play) //old language pack?
		if(1!=som_tool_play) //SOM_MAP_165 
		{				
			assert(0!=som_tool_play);
			ex[0] = som_tool_play&1?'H':'h'; //MHM
			ex[1] = som_tool_play&4?'m':'M'; //mask
			ex[2] = som_tool_play&8?'l':'L'; //light
			som_tool_play = 0;
		}
		else ex[0] = 0x100&som_map_tileviewmask?'h':'H'; //MHM
		//HACK: mapcomp.exe will not accept additional 
		//commandline arguments without some tinkering
		SetEnvironmentVariable(L".mapcomp.165",ex);

		//Reminder: the map is passed here via environment variable
		swprintf(cli,L"mapcomp.exe"
		L" "SOMEX_L(B)L"\\data\\map\\.MAP "SOMEX_L(B)L"\\data\\map\\.MPX "SOMEX_L(A)L" "SOMEX_L(B));

		if(map[2]) SetEnvironmentVariable(L".MAP",map+2);
	}
	else //som_db? passing MAP directly on command-line
	{
		swprintf_s(cli,L"som_db.exe"
		L" "SOMEX_L(B)L" "SOMEX_L(A)L" \"%ls\"",map[2]?map+2:map);
		map[2] = '\0';
	}	
	DWORD pid = SOM::exe(cli);
	
	if(map[2]) //HACK: restore MAP variable to the current layer's
	{
		map[2] = '\0';
		SetEnvironmentVariable(L".MAP",map); assert(*map);
	}
	
	if(!pid) return 0;

	J->dwProcessId = pid; J->hProcess = SOM::exe_process;
	//unavailable to XP: SOM is probably single threaded
	//J->dwThreadId = GetThreadId(SOM::exe_thread);
	//CloseHandle(SOM::exe_thread);
	//2021: hThread was uninitialized
	//(SOM_MAP.cpp/som.art.cpp call CreateProcess)
	J->dwThreadId = SOM::exe_threadId; 
	J->hThread = SOM::exe_thread;	

	return 1;
}

//REMOVE ME?
static bool som_tool_DrawTextA_item_text = false;
static DRAWITEMSTRUCT *som_tool_DrawTextA_item = 0;
typedef union{ COLORREF bgr; BYTE rgb[3]; }som_tool_RGB;
static som_tool_RGB som_tool_DrawTextA_black_and_white[2] = {-1,-1};
static int WINAPI som_tool_DrawTextA(HDC A, LPCSTR B, int C, LPRECT inD, UINT E)
{
	SOM_TOOL_DETOUR_THREADMAIN(DrawTextA,A,B,C,inD,E)

	EXLOG_LEVEL(7) << "som_tool_DrawTextA()\n";

	if(!som_tool_DrawTextA_item)
	{
		const wchar_t *w = L"";
		int wlen = EX::Convert(932,&B,&C,&w);
		return DrawTextW(A,w,wlen,inD,E);
	}
		
	HWND gp, hw = som_tool_DrawTextA_item->hwndItem;
	DWORD hlp = GetWindowContextHelpId(gp=GetParent(hw));

	wchar_t text[64+1] = L"";
	size_t text_s = GetWindowTextW(hw,text,64);		 
	if(!text_s) return 0;

	//2018: SOM doesn't handle these styles
	switch(BS_VCENTER&GetWindowStyle(hw))
	{
	case BS_BOTTOM: E&=~DT_VCENTER; 			
		E|=DT_BOTTOM|DT_SINGLELINE; break;
	case BS_TOP: E&=~(DT_VCENTER|DT_SINGLELINE);
	}

	//2018: seeing weirdness with multi-line code
	RECT outD = *inD, *D = &outD;
	
	E|=DT_EXPANDTABS; //2018: done most eleshwere

	som_tool_DrawTextA_item_text = true; //hack
		
	int quis = SendMessage(hw,WM_QUERYUISTATE,0,0);

	if(UISF_HIDEACCEL&quis)	E|=DT_HIDEPREFIX;	

	COLORREF c = GetTextColor(A);

	if(c) switch(SOM::tool)
	{
	case SOM_MAIN.exe: case SOM_EDIT.exe: 
	
		if(som_tool==gp //hack
		&&hlp!=10200&&hlp!=10300) //hack: stages 2 & 3
		{
			//recenter big buttons
			GetClientRect(hw,D); D->left+=40; //fudge!
			if(BST_PUSHED&SendMessage(hw,BM_GETSTATE,0,0))
			OffsetRect(D,1,1); assert(E&DT_CENTER);
		}
		break;

	case SOM_PRM.exe: if(c!=0xFFFFFF) break;
				
		if(som_tool!=gp)
		if(ODS_DISABLED&~som_tool_DrawTextA_item->itemState) 
		{
			//SOM_PRM doesn't always disable its buttons
			//(mainly happens in empty selection situations)
			som_tool_DrawTextA_item->itemState|=ODS_DISABLED;
			EnableWindow(hw,0);
		}
		break;
	}
	else if(UISF_HIDEFOCUS&~quis) //black
	{
		//NEW: som_tool_highlight painting SOM's buttons
		if(ODS_FOCUS&som_tool_DrawTextA_item->itemState)
		{
			c = 0X00FFFF; //yellow
		}
	}

	if(ODS_DISABLED&som_tool_DrawTextA_item->itemState)
	{		
		if(c==0xFFFFFF) switch(hlp)
		{		
		case 10000: case 20000: case 30000: case 50000:

			c = 0xCCCCCC; break; //big buttons and tabs

		default: c = 0x363636;
		}
		else c = 0x363636;
	}

	//2018: Multi-line?
	for(size_t i=0;i<text_s;i++) if(text[i]<='\n')
	{
		int div = E&DT_BOTTOM?1:2;
		E&=~(DT_SINGLELINE|DT_VCENTER|DT_BOTTOM);
		RECT calc = {};
		int h = DrawTextW(A,text,text_s,&calc,E|DT_CALCRECT);
		OffsetRect(D,0,(D->bottom-D->top-h)/div);		
		break;
	}
		
	//note: currently only used for 
	//flipping a single triangle around 
	//since Shift_JIS only has up & down ones
	int sideways = text[0]=='@'; if(sideways) 	
	{		
		XFORM xf =  {0,1,-1,0,0,0};
		//assuming there are no transformations already in place
		SetGraphicsMode(A,GM_ADVANCED);	SetWorldTransform(A,&xf);				
		
		//DrawText transforms the box, so inverse transform it
		static RECT ic; SetRect(&ic,D->left,-D->right,D->bottom,-D->top);
		D = &ic; 		
	}

	if(-1==som_tool_DrawTextA_black_and_white[0].bgr)
	{
		EX::INI::Editor ed;
		//Very simple for now. Not NaN (0) means buttons.
		som_tool_RGB b = {ed->black_saturation(0.0f)};
		som_tool_RGB w = {ed->white_saturation(0.0f)};
		std::swap(b.rgb[0],b.rgb[2]); std::swap(w.rgb[0],w.rgb[2]);
		som_tool_DrawTextA_black_and_white[0] = b;
		som_tool_DrawTextA_black_and_white[1] = w;
	}

	//contrast (assuming button)
	{	 
		int x = 1; 
		int y = sideways?-1:1; 

		if(c==0x00FFFF) //ODS_FOCUS
		{
			SetTextColor(A,0); //yellow
		}
		else if(c==0||c==0xFFFFFF)
		{
			SetTextColor(A,som_tool_DrawTextA_black_and_white[c==0].bgr);
		}
		else SetTextColor(A,~c&0xFFFFFF); //disabled? 

		OffsetRect(D,x,y);
		DrawTextW(A,text+sideways,text_s-sideways,D,E);
		OffsetRect(D,-x,-y);
				
		//2018: this calls for extensions
		//SetTextColor(A,c/*?c:0x070707*/);
		if(c==0||c==0xFFFFFF)
		SetTextColor(A,som_tool_DrawTextA_black_and_white[c!=0].bgr);
		else SetTextColor(A,c); //disabled?
	}		

	int out = DrawTextW(A,text+sideways,text_s-sideways,D,E);
	if(sideways) 
	{
		assert(out);
		ModifyWorldTransform(A,0,MWT_IDENTITY);
		SetGraphicsMode(A,GM_COMPATIBLE);
	}
	return out;
}
extern float som_tool_enlarge = 1;
extern float som_tool_enlarge2 = 1;
static BOOL WINAPI som_tool_TextOutA(HDC A, int B, int C, LPCSTR D, int E)
{
	SOM_TOOL_DETOUR_THREADMAIN(TextOutA,A,B,C,D,E)

	EXLOG_LEVEL(7) << "som_tool_TextOutA()\n";
			
	const wchar_t *text = 0, text_s = EX::Convert(932,&D,&E,&text);

	if(!text_s) return SOM::Tool.TextOutA(A,B,C,D,E);
		
	if(SOM::tool==SOM_PRM.exe) //graphs
	{
		switch(text_s==1?*text:0)
		{
		default: assert(0);			
		return TextOutW(A,B,C,text,text_s);
		case 26028: break; //Edge
		case 27572: break; //Area
		case 21050: break; //Point
		case 28779: break; //Fire
		case 22303: break; //Earth
		case 39080: break; //Wind
		case 27700: break; //Water
		case 32854: break; //Holy
		}
		
		if(som_tool_stack[0])
		for(int i=1;som_tool_stack[i];i++)
		{	
			assert(i<som_tool_stack_s);
			HWND label = GetDlgItem(som_tool_stack[i],*text);
			if(!label) continue;

			wchar_t text[24+1] = L"", 
			text_s = GetWindowTextW(label,text,24); 
			int ss = GetWindowStyle(label);
			RECT lc = Sompaste->frame(label);

			//Hmmm: unsure what SOM is doing here?? 
			//checked everthing but GetWorldTransform 
			//(guide was only meant to be a visual aid)
			POINT pt = {(lc.left+lc.right)/2,lc.top-1};
			HWND guide = ChildWindowFromPoint(GetParent(label),pt);
			lc = Sompaste->frame(label,guide);			

			
			//REMOVE ME?... really the bitmap
			//needs to be created with a larger size
			//(FOR NOW IT JUST HAS PERSONALITY)
			lc.left/=som_tool_enlarge2;
			lc.right/=som_tool_enlarge2;
			lc.top/=som_tool_enlarge;
			lc.bottom/=som_tool_enlarge;


			if(ss&SS_RIGHT)
			{
				SetTextAlign(A,TA_RIGHT);
				TextOutW(A,lc.right,lc.top,text,text_s);				
			}
			else if(ss&SS_CENTER)
			{
				SetTextAlign(A,TA_CENTER);
				TextOutW(A,(lc.left+lc.right)/2,lc.top,text,text_s);
			}
			else 
			{
				SetTextAlign(A,TA_LEFT);
				TextOutW(A,lc.left,lc.top,text,text_s);
			}
			return TRUE;
		}				
	}
	else //EneEdit/NpcEdit large font profile name text?
	{
		//TODO: RATHER PRINT ZOOM/OTHER INFORMATION HERE
		//TODO: RATHER PRINT ZOOM/OTHER INFORMATION HERE
		//TODO: RATHER PRINT ZOOM/OTHER INFORMATION HERE

		assert(SOM::tool>=PrtsEdit.exe);
		if(1&&som_tool!=workshop_host) //legacy support?
		{
			////DISABLING GOING FORWARD? 
			//
			// This shows an enemy/NPC's name in a large
			// font (MS Gothic) however 1) items/objects
			// don't have this built in (it will have to
			// be added) and 2) it's not obvious what is
			// the right way for language/theme packages
			// to choose a font for this purpose. And it
			// seems superfluous with workshop_102's box
		}
		else if(workshop_mode==0&&!workshop_tool)
		{
			assert(SOM::tool>=EneEdit.exe);			
			//don't select som_tool_charset
			goto use_selected;
		}
		return TRUE;
	}

	//unclear why this is required here
	SelectObject(A,(HGDIOBJ)som_tool_charset);
	use_selected: 
	return TextOutW(A,B,C,text,text_s);
}

extern ATOM som_tool_updown(HWND=0);
static ATOM som_tool_trackbar(HWND=0);
extern HWND som_tool_neighbor_focusing = 0;
static bool som_tool_neighbor(HWND hWnd, bool prev, LPARAM lparam=0, HWND loop=0)
{
	if(lparam&1<<30) return true;
	if(GetKeyState(VK_CONTROL)>>15) return true;

	if(!loop) 
	{
		loop = hWnd; 
	
		if(GetKeyState(VK_SHIFT)>>15) prev = !prev;
	}
	else if(hWnd==loop) return false;	

	HWND neighbor = som_tool_neighbor_focusing = 
	GetNextDlgGroupItem(GetParent(hWnd),hWnd,prev);

	if(neighbor&&neighbor!=hWnd
	&&SetFocus(neighbor)&&!GetLastError())
	{
		som_tool_neighbor_focusing = 0;

		int dc = SendMessage(neighbor,WM_GETDLGCODE,0,0);
		
		if(dc&DLGC_HASSETSEL) //Edit control
		{
			SendMessage(neighbor,EM_SETSEL,0,-1);
		}
		else if(dc&DLGC_STATIC) //Static control
		{
			return som_tool_neighbor(neighbor,prev,lparam,loop);
		}
		else //hack: weed out Trackbar and Updown controls
		{
			//Reminder: we assume every trackbar is paired
			//with an edit box. Trackbars are for mice only
			//(Besides. There is no way to focus a trackbar)

			DWORD atom = GetClassAtom(neighbor);
			if(som_tool_trackbar()==atom||som_tool_updown()==atom)
			return som_tool_neighbor(neighbor,prev,lparam,loop);
		}

		//LISTVIEWS ARE INCONSISTENT
		//2018: BS_PUSHBUTTON doesn't show its focus rectangle
		//if(dc&DLGC_BUTTON)
		{
			SendMessage(neighbor,WM_UPDATEUISTATE,MAKEWPARAM(UIS_CLEAR,UISF_HIDEFOCUS),0);		
			//LISTVIEWS NEED THIS ALSO
			//works if the state changes, but not otherwise, so 
			//the button must be redrawn in case			
			InvalidateRect(neighbor,0,0);
		}

		return true;
	}
	som_tool_neighbor_focusing = 0; return false;
}

struct
{
	//2017: trying to make the 1024 object list more 
	//friendly to keyboard input
	typedef std::pair<HWND,std::vector<wchar_t>> box;

	//SOM_MAP can have many boxes onscreen at a time
	enum{ boxenN=8 }; box boxen[boxenN]; 
	
	void reset_content(HWND hw)
	{
		for(int i=0;i<boxenN;i++)
		if(hw==boxen[i].first) 
		return boxen[i].second.clear(); 
	}

	void clear(HWND hw) //reset_content came later :(
	{
		for(int i=0;i<boxenN;i++)
		if(!IsWindow(boxen[i].first))
		{
			boxen[i].first = hw; boxen[i].second.clear(); 
			return;
		}
		else assert(hw!=boxen[i].first); //reset_content?
	}

	void add(HWND hw, const wchar_t *in)
	{
		if(*in=='[') while(*in!='\0'&&*in++!=' ');
		
		add(hw,*in);
	}
	void add(HWND hw, wchar_t in, size_t fill=0)
	{
		for(int i=0;i<boxenN;i++) if(hw==boxen[i].first)
		{
			//event battery 
			while(boxen[i].second.size()<fill) boxen[i].second.push_back('\0');

			boxen[i].second.push_back(tolower(in)); break;
		}
	}

	void update(HWND hw, int item, wchar_t *in)
	{
		while(*in!='\0'&&*in++!=' '); update(hw,item,*in);
	}
	void update(HWND hw, int item, wchar_t in)
	{
		for(int i=0;i<boxenN;i++) if(hw==boxen[i].first) 
		{
			//event battery
			boxen[i].second.resize(item+1);
			boxen[i].second[item] = tolower(in); 
			return;
		}
		assert(0);
	}
	 	
	bool select(int cc, HWND hw, wchar_t in)
	{
		switch(in)
		{
		case 3: //^C I think... which is Ctrl+C or copy
		case ' ': case '\r': case '\x1b': return false;
		}

		int i;
		for(i=0;i<boxenN;i++)
		if(hw==boxen[i].first) break; if(i==boxenN) return false;
		
		struct box &box = boxen[i];

		int iN,iM = (int)box.second.size();
		int gcs = cc=='lb'?LB_GETCURSEL:cc=='cb'?CB_GETCURSEL:0;
		if(gcs!=0) iN = SendMessage(hw,gcs,0,0);
		else iN = ListView_GetNextItem(hw,-1,LVNI_FOCUSED);
		if(iN>=iM) iN = iM-1; //HACK (SOM_PRM)

		if(!isdigit(in))
		{			
			//TODO: VK_SHIFT to go in reverse.
			//Maybe not fair to non-ASCII scripts.
			//TODO? WM_IME_SETCONTENT
			in = tolower(in);

			/*Windows' boxes don't go in reverse. 
			//This will mean much higher boxenN and
			//som_tool_box must do a search for non [
			//leading items.
			if(GetKeyState(VK_SHIFT)>>15)
			{
				for(i=iN-1;i>=0;i--)		
				if(in==box.second[i]) goto select;
				for(i=iM-1;i>iN;i--)
				if(in==box.second[i]) goto select;
			}
			else*/
			{
				for(i=iN+1;i<iM;i++)		
				if(in==box.second[i]) goto select;
				for(i=0;i<iN;i++)
				if(in==box.second[i]) goto select;
			}
		}
		else
		{
			i = (in-'0');

			//HACK: this is for selecting SOM_MAP's contents
			//these differ because the length is not known and there are
			//holes in the list
			if(cc=='cb'&&SOM::tool==SOM_MAP.exe) switch(GetDlgCtrlID(hw))
			{
			case 1170: case 1213: //Event instruction & Event subject

				wchar_t peek[8] = L""; 
				GetWindowText(hw,peek,8);
				if('['==peek[0]&&']'==peek[4]) //000-999
				{
					iN = _wtoi(peek+1); peek[1] = in; peek[2] = '\0';
					i = SendMessageW(hw,CB_FINDSTRING,i==iN/100?iN:-1,(LPARAM)peek);
					if(i==-1) return false;
				
					goto select;
				}				
			}

			if(iM>100)
			{
				//HACK: SOM_MAP has boxes with odd options
				//before the standard list of 250 items.
				int jmp = iM>250&&iM<255?iM-250:0;

				//HACK: Keep the old repeating behavior.
				i = i==(iN-jmp)/100?iN+1:jmp+i*100; 
			}
			else i = i==in/10?iN+1:i*10;

			goto select;
		}

		return false; select:
					  
		if(0==gcs)
		{
			//Control combos don't really work
			//if(~GetKeyState(VK_CONTROL)>>15)
			ListView_SetItemState(hw,-1,0,LVNI_SELECTED); //multisel?
			ListView_SetItemState(hw,i,~0,LVNI_FOCUSED|LVNI_SELECTED);
			ListView_EnsureVisible(hw,i,0);
		}
		else SendMessage(hw,cc=='cb'?CB_SETCURSEL:LB_SETCURSEL,i,0);				

		return true;
	}

}som_tool_boxen; //SINGLETON

inline bool som_tool_v(HWND v)
{
	return v&&WS_VISIBLE&GetWindowStyle(v);
}
static HWND som_tool_x(HWND x) //enable translation
{
	if(som_tool_classic) return 0; //NEW

	x = GetNextWindow(x,GW_HWNDNEXT);
	if(x&&!som_tool_v(x)&&DLGC_STATIC&SendMessage(x,WM_GETDLGCODE,0,0)) 
	return x; //returns invisible window with translation text
	return 0;
}
extern int som_tool_et(wchar_t *p) //strip &
{
	if(*p) for(int i=0,j=0;1;i++) switch(p[i])
	{
	case '&': if(p[i+1]=='&') i++; else continue;

	default: if(p[j++]=p[i]) continue; return i-1; 
	}
	return 0;
}
//destructively extract Nth line of text
static wchar_t *som_tool_nth(wchar_t *p, size_t len, size_t n, wchar_t *def)
{		
	if(som_tool_classic) return def; //NEW

	if(len) if(!EX::debug||*p!='X') //X: filling out
	{
		wchar_t *q = p, *d = p+len;
		for(size_t i=0;p<d&&i<=n;p=++q,i++)
		{
			while(q<d&&*q&&*q!='\n') q++; if(i<n) continue;
													  
			*q = '\0'; return p; //!! (destructive)
		}									
	}
	return def;
}
inline wchar_t *som_tool_nth(wchar_t *p, size_t len, size_t n)
{
	return som_tool_nth(p,len,n,p);
}
inline wchar_t *som_tool_bar(wchar_t *p, size_t len, size_t n, wchar_t *def)
{
	//0xff5c: fullwidth vertical bar / Chouonpu
	for(size_t i=0;i<len;i++) if(p[i]=='|'||p[i]==0xff5c) p[i] = '\0';
	return som_tool_nth(p,len,n,def);			   
}
extern wchar_t *som_tool_bar(wchar_t *p, size_t len, size_t n)
{
	return som_tool_bar(p,len,n,p);
}
static void som_tool_box(UINT_PTR id, HWND hWnd, UINT &uMsg, WPARAM &wParam, LPARAM &lParam)
{	
	if(som_tool_classic) return; //NEW

	switch(uMsg)
	{
	default: assert(0); return; 

	case CB_ADDSTRING: 
	
		wParam = -1; uMsg = CB_INSERTSTRING;		

	case CB_INSERTSTRING: 

		if(wParam==WPARAM(-1))
		wParam = SendMessage(hWnd,CB_GETCOUNT,0,0); break;

	case LB_ADDSTRING: 

		wParam = -1; uMsg = LB_INSERTSTRING; 

	case LB_INSERTSTRING:

		if(wParam==WPARAM(-1))
		wParam = ListBox_GetCount(hWnd); break;
	}

	HWND next =	som_tool_x(hWnd);

	bool boxen = '['==((WCHAR*)lParam)[0];
	
	if(SOM::tool==SOM_MAP.exe) //if(uMsg==CB_INSERTSTRING)
	{
		switch(id)
		{
		case 1020: case 1022: 

			assert(uMsg==CB_INSERTSTRING);

			//COURTESY: Event Loop Predicate
			//Don't make language packages define this twice
			if(!next) 
			{
				if(45300==GetWindowContextHelpId(next=GetParent(hWnd)))
				som_tool_box(id,GetDlgItem(GetParent(next),id),uMsg,wParam,lParam);
				return; //fyi: repurposing next
			}
			break;

		//item lists with one or two extra options
		case 1044: case 1046: boxen = true; assert(uMsg==CB_INSERTSTRING); break;

		//complicate case. event menu
		case 1210: boxen = false; assert(uMsg==LB_INSERTSTRING); break;
		}		
	}
			
	if(!next) goto boxen; //UNTRANSLATED?
	
	enum{ x_s=1024 };

	//5: guards against "[00] "	
	static wchar_t x[5+x_s] = L""; //static for som_tool_nth

	size_t xlen = GetWindowTextW(next,x,x_s);

	if(!xlen||EX::debug&&*x=='X') goto boxen; //X: filling out

	wchar_t *p = (WCHAR*)lParam;	

	if(!IsWindowEnabled(next))
	{
		switch(som_tool_help) //[%02d] mi-tou-roku
		{
		case 41000: case 46900: case 48100: //maps
		case 47100: //magic
		if(char*a=(char*)som_tool_SendMessageA_a)
		if(*a++=='['&&*a++&&*a++&&!strcmp(a,(char*)0x49003d))
		{
			int i; for(i=0;x[i]&&x[i]!='\n';i++)
			x[i+5] = x[i];
			x[i+5] = '\0';
			for(i=0;i<5;i++) x[i] = p[i]; p = x; //!
		}}
	}
	else //standard line by line insertion
	{
		size_t n = wParam;

		//nonstandard exceptions
		if(SOM::tool==SOM_MAP.exe)		
		if(som_map_event&&GetParent(hWnd)==som_map_event)
		{
			switch(id)
			{
			case 1199: //skip to System/SystemWide

				if(som_map_event_is_system_only) n+=3;
				break; 

			case 1014: //Character/System Protocols

				if(!som_map_event_is_system_only)
				switch(SendMessage(som_map_event_class,CB_GETCURSEL,0,0))
				{
				case 2: if(n>=2) n++; break; //skip Defeat for Objects
				case 3: case 4: n+=5; break; //System/Systemwide
				}
				else n+=5; //System/Systemwide
				break;
			}
		}

		auto cmp = p;

		p = som_tool_nth(x,xlen,n,p);

		//2022: this is guarding against mistakes where
		//a - control was overwriting the [00] item in
		//SOM_MAP's magic event menu
		/*NOTE: 1044 and 1046 are exceptions above that 
		//make this dicey
		if(p!=cmp&&boxen&&*p!='[') p = (WCHAR*)lParam;*/
		if(p!=cmp&&boxen&&*p!='['&&'['==*(WCHAR*)lParam) p = (WCHAR*)lParam;
	}

	lParam = (LPARAM)p; //!!

	boxen: if(boxen) //EXPERIMENTAL/TESTING
	{
		if(uMsg==CB_INSERTSTRING||som_tool_initializing)
		som_tool_boxen.add(hWnd,(wchar_t*)lParam);
	}
}

static void som_tool_media(HWND hwnd, UINT msg, const wchar_t *dir) //library
{	
	const wchar_t *data;
	for(int i=0;*(data=EX::data(i));i++)
	{
		//could use some more thought
		static wchar_t w[MAX_PATH] = L"", *root = w;
		bool deep = dir>=w&&dir<w+MAX_PATH;
		int w_s = deep?dir-w:
		swprintf_s(w,L"%ls\\%ls\\",data,dir);	
		if(w_s<3||w_s>MAX_PATH-8) return; //paranoia
		if(!deep) root = w+w_s;

		wcscpy(w+w_s,L"*"); 
		WIN32_FIND_DATAW data; 
		HANDLE find = FindFirstFileW(w,&data);
		if(find!=INVALID_HANDLE_VALUE) do		
		if(data.cFileName[0]!='.' //., .., and .hidden
		&&~data.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN)
		if(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
		{
			int dir_s =
			swprintf_s(w+w_s,MAX_PATH-w_s,L"%ls\\",data.cFileName);
			if(dir_s>1)
			som_tool_media(hwnd,msg,w+w_s+dir_s); 
		}
		else //adding any kind of media file available
		{
			wcscpy_s(w+w_s,MAX_PATH-w_s,data.cFileName);

			UINT test = 0; switch(msg)
			{			
			case CB_ADDSTRING: test = CB_FINDSTRINGEXACT; break;
			case LB_ADDSTRING: test = LB_FINDSTRINGEXACT; break;
			default: assert(0);
			}
			if(!test||CB_ERR==SendMessageW(hwnd,test,-1,(LPARAM)root))
			SendMessageW(hwnd,msg,0,(LPARAM)root);

		}while(FindNextFileW(find,&data));
	
		FindClose(find);
	}
}
/*thinking click-anywhere beats this
//(plus, the list-boxes lack inputs)
//EXPERIMENTAL
static void som_tool_media1001(HWND et, int hlp)
{
	if(!Edit_GetModify(et)) return; 
	
	switch(hlp)
	{
	default: return;

	case 44000: //map BGM, etc.

		Edit_SetModify(et,0);
		break;
	}	

	WCHAR t[64]; HWND cb = GetParent(et);
	if(!GetWindowText(et,t,64)) return;
	int i = ComboBox_FindStringExact(cb,1,t);
	if(i==-1) i = ComboBox_AddString(cb,t);
	ComboBox_SetCurSel(cb,i);
}*/

//better than nothing? Microsoft's behavior is horrible!
//(maybe because software like Logitech's interfere with software)
extern int som_tool_mousewheel(HWND hWnd, WPARAM wParam, int sb=1)
{	
	int spi = SPI_GETWHEELSCROLLLINES;
	if(sb==0&&GetKeyState(VK_SHIFT)>>15==0) //HACK
	spi = 0x6c; //SPI_GETWHEELSCROLLCHARS;
	UINT m; if(!SystemParametersInfo(spi,0,&m,0)) m = 2;

	int i = GET_WHEEL_DELTA_WPARAM(wParam); 
	//assert(i%WHEEL_DELTA==0);
	i/=WHEEL_DELTA;	
	if(spi==0x6c) 
	i = -i; //WM_HMOUSEWHEEL is reversed. Or maybe MOUSEWHEEL is.

	if(0) //SOM_MAP's palette redraws the 3D viewport every time.
	{			
		for(;i;i-=i>0?1:-1) for(UINT j=0;j<m;j++)
		SendMessage(hWnd,sb+WM_HSCROLL,i>0?SB_LINEUP:SB_LINEDOWN,0); 
	}
	else //for SOM_MAP's palette...
	{	
		SCROLLINFO si = {sizeof(si),SIF_TRACKPOS|SIF_RANGE|SIF_PAGE};
		GetScrollInfo(hWnd,sb,&si); 
		i = si.nTrackPos-i*m; 
		i = max(si.nMin,min(i,si.nMax-(int)si.nPage+1)); //YIKES!
		SendMessage(hWnd,sb+WM_HSCROLL,MAKEWPARAM(SB_THUMBTRACK,i),0); 
	}

	return 1; //courtesy //return 0; //1 suppresses Logitech hooks :(
}

//REMOVE ME?
static int som_map_zentai_new();
static void som_map_zentai_name(size_t,wchar_t*,size_t);
static void som_map_zentai_rename(size_t,wchar_t*,size_t);
static int som_map_codesel = 0;
static LRESULT CALLBACK som_tool_comboboxproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR hlp)
{
	WCHAR* &w = (WCHAR*&)lParam; //alias

	switch(uMsg)
	{
	case CB_RESETCONTENT:

		som_tool_boxen.reset_content(hWnd); break;

	case CB_SETITEMDATA:

		//REMOVE ME?
		//2018 RUSH-JOB: copy sword magics to new boxes		
		if(1041==id&&hlp==31100) 
		{
			extern std::vector<BYTE> SOM_PRM_magics;
			SOM_PRM_magics.push_back(lParam);
		}
		break;

	case CB_ADDSTRING:   
	case CB_INSERTSTRING: 
	{	
		//REMOVE ME?
		//2018: copy sword magics to new boxes	
		if(1041==id&&hlp==31100) for(int i=1231;i<=1232;i++)		
		SendDlgItemMessage(som_tool_stack[1],i,uMsg,wParam,lParam);

		LPARAM &a = som_tool_SendMessageA_a;
		
		if(SOM::tool==SOM_MAP.exe) //System/Systemwide
		{
			//hack: not the best approach
			if(!som_map_event_class) switch(a) 
			{
			case 0x490130: case 0x48FE40: //System/NPC

				som_map_event_is_system_only = a==0x490130;				
				som_map_event_class = hWnd; 
			}
			else if(som_map_event_is_system_only) switch(id)
			{
			case 1213: case 1014: return CB_ERR;
			}

			if(id==1170) switch(hlp) //47100 is magic list
			{
			case 45300:

				//this box is misformatted. enable som_tool_boxen
				if('['==*w&&' '!=*wcschr(w,']')) //Item list?			
				wmemmove(w+6,w+5,31)[-1] = ' ';
				break;

			case 46700: //2020: Sound event menu
				
				//SOM_SYS is 1 based, SOM_MAP (here) is 0 based
				if(w[2]=='9')
				{
					w[1]++; w[2] = '0';
				}
				else w[2]++; break;		
			}
		}

		//subroutine shared by combo/listbox
		som_tool_box(id,hWnd,uMsg,wParam,lParam); 						

		if(som_map_event&&1206==id&&47900==hlp)
		{
			//event entrypoint control instruction
			wchar_t *at = wcschr(w,'@');
			if(at) som_map_zentai_name(wParam,at,31);
		}

		LRESULT out = DefSubclassProc(hWnd,uMsg,wParam,lParam);

		if(wParam==0) //media detection
		{
			int multimedia = 0;

			if(SOM::tool==SOM_MAP.exe) 
			{
				//1069: Map Settings
				if(1069==GetDlgCtrlID(som_tool_pushed)
				&&som_tool==GetParent(som_tool_pushed))
				{				
					switch(id)	
					{
					case 1213: multimedia = 'bgm'; break;

					case 1214: case 1215: case 1216: multimedia = 'pic'; break;
					}
				}
			}
			else if(SOM::tool==SOM_SYS.exe) 
			{
				if(som_sys_tab==1020) switch(id) //MOVIES
				{
				case 1093: multimedia = 'bgm'; break;
				case 1012:
				case 1094: case 1004: case 1023: multimedia = 'pic'; break;
				case 1003: case 1009: case 1022: 
				case 1013: case 1016: case 1019: multimedia = 'vid'; break;
				}
				else if(som_sys_tab==1021) switch(id) //etc.
				{
				case 1012: multimedia = 'pic'; break;
				}				
			}
			switch(multimedia)
			{
			case 'bgm': som_tool_media(hWnd,CB_ADDSTRING,L"BGM"); 
						som_tool_media(hWnd,CB_ADDSTRING,L"SOUND\\BGM"); break;
			case 'pic': som_tool_media(hWnd,CB_ADDSTRING,L"PICTURE"); break;
			case 'vid': som_tool_media(hWnd,CB_ADDSTRING,L"MOVIE"); break;
			}
		}
		if(SOM::tool==SOM_MAP.exe) switch(a)
		{
		case 0x48FE30: //Object event class (last in dropdown menu)

			SendMessageA(hWnd,CB_ADDSTRING,-1,0x490130); //add System class
			break; 

		case 0x490130: //System event class (special use/reserved)		
			
			SendMessageA(hWnd,CB_ADDSTRING,-1,(LPARAM)som_932_Systemwide);
			EnableWindow(hWnd,1);
			break; 
		}
		return out;	
	}
	case CB_SETCURSEL: //System/Systemwide
	{
		if(som_map_event&&GetParent(hWnd)==som_map_event)
		{														 
			HWND protocol = GetDlgItem(som_map_event,1014);

			//3/4: System/Systemwide
			if(hWnd==som_map_event_class) if(3==wParam||4==wParam) 
			{
				//out: SendMessageA queries the selection
				LRESULT out = DefSubclassProc(hWnd,uMsg,wParam,lParam);				
				SendMessage(protocol,CB_RESETCONTENT,0,0);
				SendMessageA(protocol,CB_INSERTSTRING,0,0x4901A0);
				SendMessageA(protocol,CB_INSERTSTRING,1,0x49018C);
				SendMessage(protocol,CB_SETCURSEL,0,0);
				int ch[] = {1213,1164,1044,1016,1017,1018};
				windowsex_enable(som_map_event,ch,0);
				return out;
			}
			if(hWnd==protocol)
			switch(SendMessage(som_map_event_class,CB_GETCURSEL,0,0))
			{
			case 3: case 4: //enable/disable item based system event

				windowsex_enable<1044>(som_map_event,wParam==1);
			}
		}

		int out = DefSubclassProc(hWnd,uMsg,wParam,lParam);

		if(out==CB_ERR) som_map_codesel = wParam; 
		
		if(wParam!=0xffffffff) //-1
		if(1002==id&&SOM::tool==SOM_PRM.exe) //2023		
		if(IsWindowVisible(som_tool_workshop))
		workshop_exe(30000);

		return out;
	}
	case WM_PAINT:

		//trying this for SOM_MAP's layer menu, but seems like not such 
		//a bad feature to have
		if(ComboBox_GetDroppedState(hWnd))
		return 0;
		break;

	case WM_ENABLE: //System/Systemwide

		if(!wParam)
		if(som_map_event&&GetParent(hWnd)==som_map_event)
		{
			switch(id) //falling thru
			{			
			case 1044: //items: SOM re-disables it if enabled???
				
				//2: need to enable for new system event classes only
				if(3>ComboBox_GetCurSel(som_map_event_class)) break;
				//1: item triggered coworkshop_menuntinuous evaluation event protocol
				if(1!=ComboBox_GetCurSel(GetDlgItem(som_map_event,1014))) break;
				//break;
			case 1014: if(som_map_event_is_system_only) break;
				return EnableWindow(hWnd,1); 
			case 1199: if(!som_map_event_is_system_only) break;				
				return EnableWindow(hWnd,1); 
			}		
		}
		break;										   
			  				  
	case WM_CTLCOLORSTATIC:	
		
		//in case edit control is readonly
		//XP: pitch black
		//SetBkMode((HDC)wParam,TRANSPARENT);
		//return 0; 
		SetBkMode((HDC)wParam,OPAQUE);
		extern COLORREF som_tool_gray();
		extern HBRUSH som_tool_graybrush();
		extern HBRUSH som_tool_whitebrush();
		if(hlp==40000)	
		{						
			SetBkColor((HDC)wParam,som_tool_gray());			
			return (LRESULT)som_tool_graybrush();
		}
		//SOM_MAIN_179_textproc		
		SetBkColor((HDC)wParam,GetSysColor(COLOR_WINDOW));		
		return (LRESULT)som_tool_whitebrush(); 
		
	case WM_KEYDOWN: 
	case WM_UNICHAR: case WM_CHAR: //from TranslateMessage 	
	{
		bool st = ComboBox_GetDroppedState(hWnd);

		if(uMsg!=WM_KEYDOWN) //WM_CHAR (popup)
		{
			switch(wParam)
			{
			case ' ': case '\r': case '\x1b': //problem inputs
			
				break;

			default:

				if(!st) SendMessage(hWnd,CB_SHOWDROPDOWN,1,0); 			
				
				if(som_tool_boxen.select('cb',hWnd,(wchar_t)wParam))
				return 0;
			}
		}
		else switch(wParam) //WM_KEYDOWN (keyboard navigation)
		{			
		case VK_SPACE: 
		//case VK_RETURN: //belongs to default button

			if(st){ wParam = VK_RETURN; break; } //CBN_SELENDOK

			SendMessage(hWnd,CB_SHOWDROPDOWN,!st,0); return 1;

		case VK_NEXT: case VK_PRIOR: if(st) break;

			som_tool_neighbor(hWnd,wParam==VK_PRIOR,lParam); return 1;

		case VK_UP: case VK_DOWN: if(st) break;

		case VK_LEFT: case VK_RIGHT:			

			som_tool_neighbor(hWnd,wParam==VK_UP||wParam==VK_LEFT,lParam);
			return 1;
		}
		break;
	}
	case WM_MOUSEWHEEL:  
		
		//Reminder: Probably don't want this anyway. Generally the menu
		//must be expanded.

		return 0; //just disabling for parity

	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,som_tool_comboboxproc,id);
		break;
	}
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
static LRESULT CALLBACK som_tool_combobox1001(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR hlp)
{
	switch(uMsg) //1001 is the combobox's edit control
	{		
	case WM_CONTEXTMENU: //Exselector?
		
		//2018: disabling these menus one at a time (editbox)
		return 0; 

	case EM_SHOWBALLOONTIP: //comctl32.dll 6.0
		
		return 0; //disables chunky balloon tips
		
	case WM_GETDLGCODE: 

		//CBN_SELENOK on Return
		return DLGC_WANTALLKEYS| 
		DefSubclassProc(hWnd,uMsg,wParam,lParam);

		/*thinking click-anywhere beats this
		//(plus, the list-boxes lack inputs)
	case WM_KILLFOCUS:

		som_tool_media1001(hWnd,hlp);
		break;*/

	case WM_SETFOCUS:

		//prevent odd select all on click
		//behavior of combobox edit boxes
		if(IsLButtonDown())
		{
			POINT pt; GetCursorPos(&pt); 
			if(hWnd==WindowFromPoint(pt)) //parent window activating?
			{
				ScreenToClient(hWnd,&pt);
				lParam = DefSubclassProc(hWnd,uMsg,wParam,lParam);
				SendMessage(hWnd,WM_LBUTTONDOWN,MK_LBUTTON,POINTTOPOINTS(pt));
				return lParam;
			}
		}	
		break;

	case WM_PAINT: //doesn't work

		//trying this for SOM_MAP's layer menu, but seems like not such 
		//a bad feature to have
		if(ComboBox_GetDroppedState(GetParent(hWnd)))
		return 0;
		break;

	case WM_CHAR: case WM_UNICHAR:
	{
		HWND cb = GetParent(hWnd);
		if(ComboBox_GetDroppedState(cb)) 
		{
			COMBOBOXINFO cbi = {sizeof(cbi)};
			if(GetComboBoxInfo(cb,&cbi)&&cbi.hwndList)		
			return SendMessageW(cbi.hwndList,uMsg,wParam,lParam);
		}
		else switch(wParam)
		{
		case '\r': case '\t': case VK_ESCAPE: return 0;	
		}
		break;
	}
	case WM_KEYUP: 

		if(wParam==VK_ESCAPE)  return 0;
		break;

	case WM_KEYDOWN: 
	{
		HWND cb = GetParent(hWnd);

		switch(wParam) //dropdown
		{
		case VK_UP: case VK_DOWN: 
		case VK_PRIOR: case VK_NEXT: 
		case VK_ESCAPE: case VK_RETURN: case VK_SPACE:

			if(SendMessage(cb,CB_GETDROPPEDSTATE,0,0)) 
			{
				//VK_RETURN: doesn't confirm selection
				//(possibly because returning DLGC_WANTRETURN)
				if(wParam==VK_SPACE||wParam==VK_RETURN)
				{
					//posts so ' ' is handled by WM_CHAR
					PostMessage(cb,CB_SHOWDROPDOWN,0,0); 
					if(CB_ERR!=ComboBox_GetCurSel(cb))
					PostMessage(GetParent(cb),WM_COMMAND,
					MAKEWPARAM(GetDlgCtrlID(cb),CBN_SELENDOK),(LPARAM)cb);
					return 0;
				}
				else if(wParam==VK_ESCAPE) //not handled???
				{
					SendMessage(cb,CB_SHOWDROPDOWN,0,0); 
					SendMessage(GetParent(cb),WM_COMMAND,
					MAKEWPARAM(GetDlgCtrlID(cb),CBN_SELENDCANCEL),(LPARAM)cb);
					return 0;
				}
				return DefSubclassProc(hWnd,uMsg,wParam,lParam);
			}
		}

		switch(wParam) //same as singlelineproc
		{				
		case VK_SPACE:
		{
			int es = GetWindowStyle(hWnd);
			if(es&ES_READONLY)
			{
				if(uMsg==WM_KEYDOWN)
				SendMessage(cb,uMsg,wParam,lParam);
				return 0;
			}
			DWORD len =	GetWindowTextLengthW(hWnd);
			DWORD sel =	SendMessage(hWnd,EM_GETSEL,0,0);
			if(!LOWORD(sel)&&HIWORD(sel)==len)
			{
				if(uMsg==WM_KEYDOWN)
				SendMessage(cb,uMsg,wParam,lParam);
				return 0;
			}
			break;
		}		
		case VK_UP: case VK_DOWN:

			//disable blind menu browsing behavior
			//(reminder: mouse wheel is generating these???)
			//(reminder: WM_MOUSEWHEEL is not being sent???)
			wParam = wParam==VK_UP?VK_LEFT:VK_RIGHT;

		case VK_LEFT: case VK_RIGHT:			
		{				
			bool prev = wParam==VK_UP||wParam==VK_LEFT; 

			DWORD a = -1, z = -1;
			SendMessage(hWnd,EM_GETSEL,(WPARAM)&a,(WPARAM)&z);

			if(z-a)
			{
				if(GetKeyState(VK_SHIFT)>>15) break;
				if(prev) SendMessage(hWnd,EM_SETSEL,a,a);
				if(!prev) SendMessage(hWnd,EM_SETSEL,z,z);
				return 0;
			}
			
			if(a!=0&&prev||!prev&&a!=GetWindowTextLengthW(hWnd)) break;

			som_tool_neighbor(cb,prev,lParam); return 0; 
		}
		case VK_PRIOR: case VK_NEXT:

			som_tool_neighbor(cb,wParam==VK_PRIOR,lParam); return 0;

		case VK_TAB: //there's no DLGC_WANTRETURN???

			SetFocus(GetNextDlgTabItem
			(GetParent(cb),cb,GetKeyState(VK_SHIFT)>>15));
			return 0;

		case VK_ESCAPE: //there's no DLGC_WANTRETURN???

			PostMessage(GetAncestor(cb,GA_ROOT),WM_COMMAND,IDCANCEL,0);
			return 0;

		case VK_RETURN: //notify parent of selection affirmed

			//this behavior is first and foremost
			//for when the selection loads content
			//into a static area of the same screen
 			if(SendMessage(hWnd,EM_GETMODIFY,0,0))
			{	
				assert(-1==ComboBox_GetCurSel(cb));
				SendMessage(hWnd,EM_SETMODIFY,0,0);
				SendMessage(GetParent(cb),WM_COMMAND,
				MAKEWPARAM(GetDlgCtrlID(cb),CBN_SELENDOK),(LPARAM)cb);
			}
			else //default push button 
			{
				HWND dlg = GetAncestor(cb,GA_ROOT);
				int defid = SendMessage(dlg,DM_GETDEFID,0,0);
				if(HIWORD(defid)==DC_HASDEFID) 
				//SendDlgItemMessage(dlg,LOWORD(defid),BM_CLICK,0,0);
				SendMessage(dlg,WM_COMMAND,LOWORD(defid),0);
			}
			return 1;

		case WM_KEYDOWN:

			if(wParam=='Z') //undo
			if(GetKeyState(VK_CONTROL)>>15)
			if(GetKeyState(VK_SHIFT)>>15) wParam = 'Y'; //redo
			break;
		}
		break;
	}		  
	case WM_LBUTTONDBLCLK:
		
		SendMessage(GetFocus(),EM_SETSEL,0,-1);
		return 0;
		
	case WM_SETCURSOR:
	case WM_LBUTTONDOWN:
		if(hlp==40000) //SOM_MAP layers menu?
		{
			//it's really a read-only text box, but it should
			//be able to click on it like a regular drop menu
			if(uMsg==WM_LBUTTONDOWN)
			if(!SOM::Tool::dragdetect(hWnd,uMsg))			
			SendMessage(GetParent(hWnd),CB_SHOWDROPDOWN,1,0);
			else break;
			return 0;
		}
		break;

	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,som_tool_combobox1001,id);
		break;
	}
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
/*EXPERIMENTAL
static LRESULT CALLBACK som_tool_mirrorboxproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scID, DWORD_PTR hlp)
{
	HWND &mirror = *(HWND*)scID;
	switch(uMsg) 
	{		
	case CB_INSERTSTRING:
	case CB_RESETCONTENT: assert(0); return CB_ERR;

	case CB_SETITEMDATA: return 1;
	case CB_ADDSTRING: 
		
		return SendMessage(mirror,uMsg,wParam,lParam);
		return ComboBox_SetCurSel(hWnd,1+ComboBox_GetCurSel(hWnd));

	case CB_GETLBTEXT: 
	case CB_GETLBTEXTLEN: 
	case CB_GETITEMDATA:
	case CB_FINDSTRING: 
	case CB_FINDSTRINGEXACT:

		return SendMessage(mirror,uMsg,wParam,lParam);

	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,som_tool_mirrorboxproc,scID);
		break;
	}
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}*/

//SOM_MAP instruction set 
static std::map<std::wstring,std::wstring> som_map_instructions;
static LRESULT CALLBACK som_tool_listboxproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR hlp)
{	
	switch(uMsg) 
	{			
	case WM_CONTEXTMENU: //Exselector?
		
		//2018: disabling these menus one at a time (scrollbar)
		return 0; 

	case LB_RESETCONTENT:

		som_tool_boxen.reset_content(hWnd); break;

	case LB_INSERTSTRING: 

		if(SOM::tool==SOM_PRM.exe||41000==hlp)
		som_tool_boxen.update(hWnd,wParam,(WCHAR*)lParam);
		//break;

	case LB_ADDSTRING: 
	{	
		wchar_t *q = (WCHAR*)lParam;
		//subroutine shared by combo/listbox
		som_tool_box(id,hWnd,uMsg,wParam,lParam); 	
		wchar_t *p = (WCHAR*)lParam;
		
		switch(hlp) //SOM_MAP
		{		
		case 45000: //Event Battery
		{	
			int event_slot = -1, copied_slot=-1, c=0;
			if(som_tool_SendMessageA_a)
			if(swscanf(q,L"[%d] %*s @%d-%x",&event_slot,&copied_slot,&c)) 
			{	
				wchar_t x[64] = L""; wcscpy(x,p); //!
				
				if(wParam<10) //pasted blank into system event?
				{
					if(p==q) //no translation was applied
					{
						som_map_zentai_name(event_slot,x+13,31);
					}
					else if(event_slot!=wParam) //repairs
					{
						WPARAM repair_translation = event_slot; 
						som_tool_box(id,hWnd,uMsg,repair_translation,lParam=(LPARAM)q);
						wcscpy(x,p=(wchar_t*)lParam);
					}
				}							

				if(q[8]!='-') 
				{
					if(c==0xFE) wcsncpy(x+8,L"SYS",3);
					if(c==0xFD) wcsncpy(x+8,L"EZT",3); 
					if(event_slot>=10)
					{
						som_map_zentai_name(copied_slot,x+13,31);
						if(copied_slot!=event_slot)
						som_map_zentai_rename(event_slot,x+13,31);					
					}
				}				
				else //pass 2: deletions (obsolete)
				{
					if(!som_tool_SendMessageA_a)
					//note: this is safe to do only because
					//deletions are filled in behind copies
					som_map_zentai_rename(event_slot,L"",2);	
					else assert(0);
				}
				
				//hack: using LB_SETITEMDATA to fill menu with placeholders
				//(doesn't seem to interfere, however SOM sometimes uses ITEMDATA)			
				LRESULT out = DefSubclassProc(hWnd,LB_INSERTSTRING,wParam,(LPARAM)x);
				if(out!=LB_ERR) ListBox_SetItemData(hWnd,out,event_slot);	 

				if(1024!=ListBox_GetCount(hWnd))
				{
					som_tool_boxen.add(hWnd,x[13],event_slot); 
				}
				else som_tool_boxen.update(hWnd,event_slot,x[13]); 
				
				return out;			
			}			
			else assert(0); break;
		}
		case 45100: //Event
		{
			if(p[0]=='[' //"[%02d]   %2d   "
			&&isdigit(p[1])&&isdigit(p[2])&&p[3]==']')
			{
				wchar_t *q = p+4, *d; while (*q==' ') q++; 
				long proglen = wcstol(q,&d,10); if(q==d) break;
				q = d+1; while(*q==' ') q++;
				
				static std::wstring tests[7] = 
				{
				EX::need_unicode_equivalent(932,(char*)(0x4900A0+12)),
				//items/gold
				EX::need_unicode_equivalent(932,(char*)(0x4900F0+0)),
				EX::need_unicode_equivalent(932,(char*)(0x4900E0+0)),
				//strength/magic/level
				EX::need_unicode_equivalent(932,(char*)(0x4900D0+8)),
				EX::need_unicode_equivalent(932,(char*)(0x4900D0+0)),
				EX::need_unicode_equivalent(932,(char*)(0x4900C0+8)),
				//counter
				EX::need_unicode_equivalent(932,(char*)(0x4900B0+8)),
				};
				for(size_t i=0,j=-1;i<7;i++)
				{
					size_t n = tests[i].size();
					if(!tests[i].compare(0,n,q,n))
					{	
						for(q+=n;*q==' ';q++);
						
						switch(*q) //full-width inequalities
						{	
						case 0xFF1D: j = 0; q++; break; //=
						case 0x2260: j = 1; q++; break; //!=
						case 0xFF1E: j = 2; q++; break; //>
						case 0xFF1C: j = 3; q++; break; //<

						default: assert(!*q); break;
						}
						while(*q==' ') q++;

						HWND a = GetDlgItem(GetParent(hWnd),1022);
						HWND b = GetDlgItem(GetParent(hWnd),1020);

						const size_t x_s=32; WCHAR x[x_s] = L"", y[x_s] = L"";

						//2018: i==0 is to not show the "None" lines, since
						//it's hard to translate it for both contexts and to
						//reduce noise.
						if(i==0||SendMessageW(a,CB_GETLBTEXTLEN,i,0)<x_s
						&&SendMessageW(a,CB_GETLBTEXT,i,(LPARAM)x)!=CB_ERR
						&&!*q||SendMessageW(b,CB_GETLBTEXTLEN,j,0)<x_s
						&&SendMessageW(b,CB_GETLBTEXT,j,(LPARAM)y)!=CB_ERR)
						{
							wchar_t z[4+3+33+3+x_s*2+6] = L"";
							swprintf_s(z,L"[%02d] %4d   %ls%ls%s",wParam,proglen,x,y,q);
							//2018: Hiding 0s so to further reduce noise.
							if(z[7]==' '&&z[8]=='0') wmemset(z+5,'-',4); //z[8] = '+';
							return DefSubclassProc(hWnd,LB_INSERTSTRING,wParam,(LPARAM)z);
						}							
					}
				}
			}			
			break;
		}
		case 45300: //Event Loop
		{
			//REMINDER: I think these are reset 
			//every single time there's a change

			if(id!=1026&&id!=1029) break;

			//2021: need to reset indent on reopen
			//(it was saved in som_tool_wector but
			//other code is sharing it these days)
			static int max_indent;
			int gc = ListBox_GetCount(hWnd);
			if(!gc) max_indent = 0;
			if(wParam==WPARAM(LB_ERR)) 
			{
			//	wParam = ListBox_GetCount(hWnd);
				wParam = gc;
			}

			int i = 0; while(q[i]==' ') i++; //indent?

			std::wstring &w = som_map_instructions[q+i]; 
			if(w.empty())
			w.assign(1,som_map_instructions.size()).append(p); 
			const wchar_t *c = w.c_str()+1;
			
			auto &indent = som_tool_wector; if(i) //indent?
			{
				indent.assign(i,' ');
				indent.insert(indent.end(),c,c+w.size());
				//2018: correct the horizontal scroll bar
				if(i>max_indent)
				{
					max_indent = i; RECT cr; GetClientRect(hWnd,&cr);
					ListBox_SetHorizontalExtent(hWnd,cr.right+i*som_tool_space);
				}
				c = &indent[0];
			}									
			if(id==1026&&1==w[0]) //inspect event data?
			{
				if(auto*ed=SOM_MAP.evt_data(GetParent(hWnd),wParam))
				{
					assert(0==ed->code); 

					if('\1'==*ed->data[0]) //comment?
					{
						c = EX::need_unicode_equivalent(932,ed->data[0]+1);
						if(i)
						{
							indent.resize(i);
							indent.insert(indent.end(),c,c+wcslen(c));
							c = &indent[0];
						}
					}
				}
			}

			uMsg = id==1029?LB_ADDSTRING:LB_INSERTSTRING; //2018
			wParam = DefSubclassProc(hWnd,uMsg,wParam,(LPARAM)c);
			if(wParam!=WPARAM(LB_ERR))
			ListBox_SetItemData(hWnd,wParam,w[0]); else assert(0);
			return wParam;			
		}}	
		//2018 //return DefSubclassProc(hWnd,uMsg,wParam,(LPARAM)p);
		lParam = (LPARAM)p; break; 
	}	
	case WM_SETFOCUS: //highlight

		//2018: Enable som_tool_listbox_focusrect?
		//DefSubclassProc isn't even managing UISF_HIDEFOCUS???
		SendMessage(hWnd,WM_UPDATEUISTATE,MAKEWPARAM(UIS_INITIALIZE/*CLEAR*/,UISF_HIDEFOCUS),0);		

		if(IsLButtonDown()) break;

		if(ListBox_GetCurSel(hWnd)==LB_ERR)
		if(ListBox_SetCurSel(hWnd,0)!=LB_ERR)
		{			
			SendMessage(GetParent(hWnd),WM_COMMAND,
			MAKEWPARAM(id,LBN_SELCHANGE),(LPARAM)hWnd);
		}
		else if(GetKeyState(VK_TAB)>>15)
		{
			HWND next =	GetNextDlgTabItem(GetParent(hWnd),hWnd,GetKeyState(VK_SHIFT)>>15); 
			if(next) return (LRESULT)SetFocus(next);
		}

		break;
		
	case WM_CHAR: case WM_UNICHAR:

		if(som_tool_boxen.select('lb',hWnd,(wchar_t)wParam))
		{
			//2018: SOM_PRM isn't loading the screen with the selection.
			//Might rather press space to load? But keyboard loads them.
			SendMessage(GetParent(hWnd),WM_COMMAND,MAKEWPARAM(id,LBN_SELCHANGE),(LPARAM)hWnd);
			return 0;
		}
		break;

	case WM_KEYDOWN: 

		switch(wParam)
		{
		case VK_SPACE: //courtesy
		
			SendMessage(GetParent(hWnd),WM_COMMAND,
			MAKEWPARAM(id,LBN_DBLCLK),(LPARAM)hWnd);			
			return 0; //break; //2018 (better?)

		//todo: extend to cut/copy/paste
		case VK_DELETE: //map to buttons
		{	
			int bt = 0; switch(hlp)
			{
			case 31100: if(id==1030) bt = 1014; break; //items
			case 32100: if(id==1029) bt = 1014; break; //shops
			case 33100: if(id==1034) bt = 1014; break; //magic
			case 34100: if(id==1219) bt = 1014; break; //objects
			case 35100: if(id==1220) bt = 1014; break; //enemies
			case 36100: if(id==1023) bt = 1014; break; //NPCs
			case 41000: if(id==1239) bt = 1046; break; //maps
			case 45000: if(id==1210) bt = 1046; break; //events
			case 45300: if(id==1026) bt = 1010; break; //event loop
			}
			if(bt)
			{
				HWND dlg = GetParent(hWnd);
				if(SOM::tool==SOM_PRM.exe) dlg = GetParent(dlg);
				SendMessage(dlg,WM_COMMAND,bt,0/*(LPARAM)GetDlgItem(dlg,bt))*/);
			}
			break;
		}					
		case 'X': case 'C': case 'V': 
		{
			if(GetKeyState(VK_CONTROL)>>15) switch(hlp)
			{
			case 41000: case 45000: //maps/events

				if('X'==wParam)
				return MessageBeep(-1); //TODO? Copy+Delete? Or add Cut button?				
				SendMessage(som_tool_dialog(),WM_COMMAND,wParam=='C'?1213:1047,0);
				return 0;				

			default: //SOM_PRM?

				if(SOM::tool==SOM_PRM.exe) //Cut, Copy, Paste
				return SendMessage(som_tool,WM_COMMAND,wParam=='X'?1015:wParam=='C'?1012:1011,0);
			}
			break;
		}
		case VK_UP: case VK_DOWN: case VK_PRIOR: case VK_NEXT:
		{
			int i = ListBox_GetCurSel(hWnd);
			if(wParam==VK_UP&&!i||wParam==VK_DOWN&&i==ListBox_GetCount(hWnd)-1)
			som_tool_neighbor(hWnd,wParam==VK_UP||wParam==VK_PRIOR,lParam);
			break;
		}
		case VK_LEFT: case VK_RIGHT:
		{		
			//can a listboxes have two scrollbars?
			if(LBS_MULTICOLUMN&GetWindowStyle(hWnd))
			{
				int pos = GetScrollPos(hWnd,SB_HORZ);
				LRESULT out = DefSubclassProc(hWnd,uMsg,wParam,lParam);
				if(pos==GetScrollPos(hWnd,SB_HORZ))
				som_tool_neighbor(hWnd,wParam==VK_LEFT,lParam);
				return out;
			}
			else som_tool_neighbor(hWnd,wParam==VK_LEFT,lParam);
			return 0;			
		}}		
		break;

	case WM_MOUSEWHEEL: 
	{
		//Reminder: There are multiple wheel issues, including SOM_MAP
		//which never had a wheel. And the listbox wheel performance is
		//unbearable even on Windows 10. Listview however is not bad...
		//Maybe the listboxes can be quietely converted to listviews if
		//the day comes when it seems worthwhile to make a major project
		//of this.

		//return 0; //just disabling for parity 

		//On Windows 10 the first scroll-down event is very sluggish.
		//For whatever reason InvalidateRect kickstarts it.
		if(0==ListBox_GetTopIndex(hWnd)) 
		{
			//Just give it a bogus rect so not to muss the paint job.
			RECT r = {0,0,1,1}; InvalidateRect(hWnd,&r,0);
		}
		return som_tool_mousewheel(hWnd,wParam);
	}	
	//WARNING: som_tool_LBS_EXTENDEDSEL handles these
	//WARNING: som_tool_LBS_EXTENDEDSEL handles these
	//WARNING: som_tool_LBS_EXTENDEDSEL handles these
	{
		case LB_SETCURSEL:
	
			//MOVING TO som_tool_LBS_EXTENDEDSEL TO BE SAFE
			//if(hlp==41000&&som_tool_initializing)
			//wParam = wcstol(Sompaste->get(L".MAP"),0,10);
			break;
			
		case LB_GETCURSEL: //2018
		
			if(hlp==45300) //event loop
			{
				//NOTE: this doesn't want to set the selection
				//so this is pretty much the simplest approach

				if(som_tool_play) //Programmer's Message? Or?
				{				
					if(id==1029) //potential conflicts
					{
						return som_tool_play-1;
					}
				}
			}
			break;
	}
	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,som_tool_listboxproc,id);
		break;
	}

	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
static LRESULT CALLBACK som_tool_LBS_EXTENDEDSEL(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR hlp)
{
	switch(uMsg)
	{
	case LB_GETCURSEL:
	
		//NOTE: 0 is returned even for an empty list
		//but for compatiblity sake, don't return the
		//caret-index unless it's selected
		lParam = DefSubclassProc(hWnd,uMsg,wParam,lParam);
		//LB_ERR???
		if(1!=ListBox_GetSel(hWnd,lParam)) 
		{
			lParam = 0; //32bit?
			//in case the caret is on an unselected
			//item, yet there is a selection
			if(1!=ListBox_GetSelItems(hWnd,1,&lParam))
			lParam = LB_ERR;
		}
		return lParam;
	
	case LB_SETCURSEL:
	
		//MOVED FROM som_tool_listboxproc TO BE SAFE
		//historically SOM_MAP opens to the first map
		/*might want to adjust layers or something.*/
		if(/*wParam==0&&*/hlp==41000&&som_tool_initializing)
		{
			wParam = wcstol(Sompaste->get(L".MAP"),0,10);
		}

		ListBox_SetSel(hWnd,0,-1); 
		if(wParam!=-1)
		if(LB_ERR==ListBox_SetSel(hWnd,1,wParam))
		wParam = LB_ERR;
		return wParam;
	}		
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
static void som_tool_dialogpixels(int x, int *cx, wchar_t **pe)
{
	wchar_t *p = *pe, *e;
	//Example: "100-v+1Caption text follows"
	//So 100 dialog units - the menu button + a pixel
	int out = 0, units = wcstol(p,&e,10); 
	if(p==e||!*e||iswspace(*e)||units<0) return;
	//i<2: this enforces at most one metric and one offset
	//and lets any kind of text (+-0123456789iv) be useful
	if(p!=e) for(int i=0,metric;*e=='+'||*e=='-'&&i<2;i++)
	{
		switch(e[1])
		{
		default: metric = 0; break; //will try per pixel adjust
		case 'i': metric = x=='x'?SM_CXVSCROLL:SM_CYHSCROLL; break;	
		case 'v': metric = x=='x'?SM_CXMENUSIZE:SM_CYMENUSIZE; break;			
		}
		if(metric&&(e+=2))
		out+=GetSystemMetrics(metric)*(e[-2]=='-'?-1:1); 
		else if(e[1]=='0') e+=2;
		else if(isdigit(e[1])) out+=wcstol(e,&e,10);
		else break; 
	}
	if(x=='x') out+=units*som_tool_dialogunits.right/4; 
	if(x=='y') out+=units*som_tool_dialogunits.bottom/8;
	if(!*e||!out) return;
	*pe = e; *cx = out;
}
static LRESULT CALLBACK som_tool_listviewproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR hlp)
{		
	switch(uMsg) 
	{						
	case WM_CONTEXTMENU: //Exselector?
		
		//2018: disabling these menus one at a time (scrollbar)
		return 0; 

	case WM_RBUTTONDBLCLK:
	case WM_RBUTTONDOWN: case WM_RBUTTONUP: //Exselector?

		//2018: disabling these menus one at a time (selects??)
		return 0; 

	case LVM_INSERTCOLUMNW: 
	{	
		HWND next =	//enabling translation
		som_tool_x(hWnd); if(!next) break;				
		const size_t x_s = 96; wchar_t x[x_s] = L"";
		size_t xlen = GetWindowTextW(next,x,x_s);
		if(!xlen||EX::debug&&*x=='X') break; //X: filling out
		wchar_t *p = som_tool_bar(x,xlen,wParam,0); if(!p) break;		
		//isdigit: strip language pack sizing prefix
		LVCOLUMNW *lvc = (LVCOLUMNW*)lParam; lvc->pszText = p;		
		if(isdigit(*p)) som_tool_dialogpixels('x',&lvc->cx,&lvc->pszText);
		return DefSubclassProc(hWnd,LVM_INSERTCOLUMNW,wParam,lParam);
	}
	case LVM_SETITEMW:
	case LVM_SETITEMTEXTW:
	case LVM_INSERTITEMW: //special cases
	{
		LVITEMW *lvi = (LVITEMW*)lParam;	
		if(uMsg!=LVM_SETITEMTEXTW&&~lvi->mask&LVIF_TEXT) break;

		switch(hlp)
		{
		case 32100: //SOM_PRM shops

			if(0==lvi->iSubItem&&uMsg==LVM_SETITEMTEXTW)			
			{
				som_tool_boxen.add(hWnd,lvi->pszText);
				//hide unused items from multi-assignments
				lvi->mask = LVIF_PARAM|LVIF_TEXT;
				uMsg = LVM_SETITEMW;
				lvi->iItem = wParam;
				lvi->lParam = lvi->pszText[6]?wParam:0xff;
			}
			break;

		case 45200: //SOM_MAP counters

			if(1==lvi->iSubItem)
			if(1024>ListView_GetItemCount(hWnd))
			som_tool_boxen.add(hWnd,lvi->pszText[0]);
			else som_tool_boxen.update(hWnd,wParam,lvi->pszText[0]);			
			break;			

		case 51100: //SOM_SYS magic_table
		{	
			enum{x_s=32}; static wchar_t
			mitouroku[x_s]=L"",shinai[x_s]=L"",suru[x_s]=L"",x
			=GetWindowTextW(som_tool_x(GetDlgItem(GetParent(hWnd),1020)),mitouroku,x_s)
			|GetWindowTextW(GetDlgItem(GetParent(hWnd),1043),shinai,x_s)&&som_tool_et(shinai)
			|GetWindowTextW(GetDlgItem(GetParent(hWnd),1044),suru,x_s)&&som_tool_et(suru);
			switch(som_tool_SendMessageA_a)
			{default: if(lvi->iSubItem==1&&*mitouroku) 
			if(!strcmp((char*)0x44B1BC,(char*)som_tool_SendMessageA_a))
			   /*0x44B1BC*/ lvi->pszText = mitouroku; case 0: break; 
			case 0x44B174: if(*shinai) lvi->pszText = shinai; break;
			case 0x44b17C: if(*suru) lvi->pszText = suru; break;
			}
		}}
		break;
	}
	case WM_SETFOCUS: //highlight
	
		if(!IsLButtonDown())
		if(!ListView_GetSelectedCount(hWnd))
		ListView_SetItemState(hWnd,0,~0,LVIS_SELECTED|LVIS_FOCUSED);
		break;
	
	case WM_CHAR: case WM_UNICHAR: 
		
		if(som_tool_boxen.select('lv',hWnd,(wchar_t)wParam))
		return 0;

		//suppress beeps and search function
		/*if(wParam==' ')*/ return 0; break;
		switch(wParam) 
		{
		case ' ': return 0; //this actually selects items???
		}

		break;

	case WM_KEYDOWN: //note: these interfere with LVN_KEYDOWN
	{
		switch(wParam)
		{
		case VK_SPACE:
		{
			int i = ListView_GetNextItem(hWnd,-1,LVNI_FOCUSED);

			if(GetKeyState(VK_CONTROL)>>15) //invert selection
			{
				ListView_SetItemState(hWnd,i,~
				ListView_GetItemState(hWnd,i,~0),LVIS_SELECTED);
			}
			else //activate (through double-click just in case)
			{
				if(~ListView_GetItemState(hWnd,i,~0)&LVIS_SELECTED)
				{
					ListView_SetItemState(hWnd,-1,0,LVIS_SELECTED);
					ListView_SetItemState(hWnd,i,~0,LVIS_SELECTED);
				}
				RECT ir; if(ListView_GetItemRect(hWnd,i,&ir,0))			
				SendMessage(hWnd,WM_LBUTTONDBLCLK,0,POINTTOPOINTS((POINT&)ir));
			}			
			return 0;
		}
		case VK_F2: 
			
			if(LVS_EDITLABELS&GetWindowStyle(hWnd)) 
			{				
				int i = ListView_GetNextItem(hWnd,-1,LVNI_FOCUSED);
				return SendMessageW(hWnd,LVM_EDITLABELW,i,0);			
			}
			break;

		case VK_SHIFT:
						
			return ListView_SetSelectionMark
			(hWnd,ListView_GetNextItem(hWnd,-1,LVNI_FOCUSED));		

		case VK_UP: case VK_DOWN: case VK_PRIOR: case VK_NEXT:
		{
			int f = ListView_GetNextItem(hWnd,-1,LVNI_FOCUSED);
			if(wParam==VK_UP&&!f||wParam==VK_DOWN&&f==ListView_GetItemCount(hWnd)-1)
			som_tool_neighbor(hWnd,wParam==VK_UP||wParam==VK_PRIOR,lParam);
			break;
		}
		case VK_LEFT: case VK_RIGHT:			
		{				
			int pos = GetScrollPos(hWnd,SB_HORZ);
			LRESULT out = DefSubclassProc(hWnd,uMsg,wParam,lParam);
			if(pos==GetScrollPos(hWnd,SB_HORZ))
			som_tool_neighbor(hWnd,wParam==VK_LEFT,lParam);
			return out;
		}}		
		break;
	}		
	case WM_LBUTTONDOWN: 
	{
		LVHITTESTINFO hti; POINTSTOPOINT(hti.pt,lParam);
		int ht = ListView_SubItemHitTest(hWnd,&hti);

		//full-row marquee-less multi-select? (full-row is required.)
		if(LVS_SINGLESEL&~GetWindowStyle(hWnd)
		&&LVS_EX_FULLROWSELECT&ListView_GetExtendedListViewStyle(hWnd))
		{
			SetFocus(hWnd);			
			//marquee is hidden because of som_tool_DrawFocusRect
			POINT pt; GetCursorPos(&pt); if(DragDetect(hWnd,pt)) 
			{
				//Shift for union and Ctrl for invert on drag is free
				*(WORD*)&lParam = -27000;
			}
			else
			{
				//DragDetect swallows everything up???
				//MUST EMULATE SINGULAR CLICK BEHAVIOR
				//DefSubclassProc(hWnd,uMsg,wParam,lParam);
				//DefSubclassProc(hWnd,WM_LBUTTONUP,wParam,lParam);				
				NMITEMACTIVATE clk = //NM_CLICK just for completeness
				{{hWnd,GetDlgCtrlID(hWnd),NM_CLICK},-1,-1,0,0,0,hti.pt,0,0};
				if(GetKeyState(VK_MENU)>>15) clk.uKeyFlags|=LVKF_ALT;				
				if(GetKeyState(VK_SHIFT)>>15) clk.uKeyFlags|=LVKF_SHIFT;
				if(GetKeyState(VK_CONTROL)>>15) clk.uKeyFlags|=LVKF_CONTROL;
				if(-1!=ht)
				{				
					//observed NM_CLICK behavior 
					clk.iItem = hti.iItem; clk.iSubItem = hti.iSubItem; 
					LVITEMW lvi = {LVIF_PARAM|LVIF_STATE,hti.iItem,0,0,~0};
					ListView_GetItem(hWnd,&lvi);					
					clk.uOldState = lvi.state; clk.lParam = lvi.lParam;
					int sel = LVIS_SELECTED;
					int selected = LVIS_SELECTED&clk.uOldState;
					if(~clk.uKeyFlags&LVKF_CONTROL)
					{	
						if(selected&&hti.iSubItem==0) 
						ListView_EditLabel(hWnd,hti.iItem);			   
						ListView_SetItemState(hWnd,-1,0,LVIS_SELECTED);
					}
					else if(selected) sel = 0; //toggling
					ListView_SetItemState(hWnd,hti.iItem,sel|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);					
				}
				else if(~clk.uKeyFlags&LVKF_CONTROL)
				ListView_SetItemState(hWnd,-1,0,LVIS_SELECTED|LVIS_FOCUSED);
				//Note: behaves like regular behavior: doesn't wait for an up event
				//(curiously NM_CLICK gets sent when dragging only after released!)
				SendMessage(GetParent(hWnd),WM_NOTIFY,clk.hdr.idFrom,(LPARAM)&clk);
				return 0;
			}
		}
		
		//2018: SOM_SYS & SOM_PRM seem to have a bug, where clicking in the item
		//doesn't acknowledge that the row is moved, as keyboard navigation does
		if(id==1015&&som_sys_tab==1018||id==1031&&SOM::tool==SOM_PRM.exe)
		{		
			if(hti.iSubItem!=0&&-1!=ht)	  
			//if(!ListView_GetItemState(hWnd,ht,LVIS_FOCUSED))
			{
				POINTS pts = (POINTS&)lParam;
				pts.x = -GetScrollPos(hWnd,SB_HORZ);	
			
				//XP: there is a larger bug in these listviews, in that when an
				//item is clicked, the first row gets selected momentarily. for
				//Windows 7+ the problem goes away by clicking the first column
				//(which this does) but on XP what works is to send only the UP
				//message. XP still invalidates multi-selection, but it does so
				//anyway, without this fix. It likely doesn't envision a column
				//based multi-selection model (can backup/restore; but for XP?)
				//DefSubclassProc(hWnd,WM_LBUTTONDOWN,0,(LPARAM&)pts);
				//DefSubclassProc(hWnd,WM_LBUTTONUP,0,(LPARAM&)pts);	
				if((GetVersion()&0xFF)>=6) //Vista?
				{
					//must do this way to implement Ctrl+click to not open edit
					//box behavior
					DefSubclassProc(hWnd,WM_LBUTTONDOWN,0,(LPARAM&)pts);
					DefSubclassProc(hWnd,WM_LBUTTONUP,0,(LPARAM&)pts);	
				}
				else //only send WM_LBUTTONUP as lengthy commentary prescribes
				{
					//TODO? XP cannot use the mouse to enter multi-selection...
					//it can kind of do it by holding control and using the keyboard
					//to blind navigate, before pressing Enter :(
					SetFocus(hWnd);
					//DefSubclassProc(hWnd,WM_LBUTTONDOWN,0,(LPARAM&)pts);
					DefSubclassProc(hWnd,WM_LBUTTONUP,0,(LPARAM&)pts);	
				}				

				while(hti.iSubItem-->0) //geez				
				DefSubclassProc(hWnd,WM_KEYDOWN,VK_RIGHT,0);
				//double-click?
				//WEIRD: makes no difference??? there must be a
				//hook involved!!
				//DefSubclassProc(hWnd,WM_KEYDOWN,VK_RETURN,0);		
				return 0;
			}
		}

		break;
	}
	case LVM_SETEXTENDEDLISTVIEWSTYLE: //standardizing
	{
		if(SOM::tool==SOM_SYS.exe)
		{	  			
			//SOM_SYS doesn't seem to require LVN_ITEMACTIVATE
			lParam&=~LVS_EX_ONECLICKACTIVATE; 
			//Magic table looks like Counters
			lParam|=LVS_EX_GRIDLINES;
		}

		lParam|=LVS_EX_LABELTIP; //LVS_EX_INFOTIP; //2018

		//REMINDER: 0 is equivalent to ~0
		assert(0==wParam);
		wParam = lParam; //making one-way, but additive
						   		
		break;
	}
	case LVM_SETCOLUMNWIDTH:
		
		//2018: why is SOM_PRM.exe doing this???
		//it expands the column when clicking between columns and maybe
		//if a scroll is triggered, but mainly if clicking
		if(!som_tool_initializing) //IsWindowVisible(hWnd)
		return 0;
		break;

	case WM_CAPTURECHANGED:

		//WORKAROUND FOR THIS BUG
		//https://stackoverflow.com/questions/47605376/bug-with-listview-scrolling-during-mouse-lasso-selection-windows-10-update-1629
		if(1)
		{
			SetFocus(GetParent(hWnd)); SetFocus(hWnd);
			return 0;
		}
		break;

	case WM_MOUSEWHEEL: case 0x020E: //WM_HMOUSEWHEEL:
		
		//2018: Destroy subitem edit box, if any.
		DestroyWindow(FindWindowEx(hWnd,0,L"EDIT",0));

		//Reminder: There are multiple wheel issues elswhere. Listview
		//wheel functionality is not bad. But listbox is miserable. Maybe
		//if the listboxes were replaced with simple listviews.

		break; //return 0; //just disabling for parity
			  	
	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,som_tool_listviewproc,id);
		break;
	}

	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}

static bool som_map_treeview_event(TVITEMW *tvi, wchar_t *x, size_t x_s)
{
	int event_slot = -1, copied_slot=-1, c, p;
	
	if(tvi->mask&TVIF_TEXT
	&&4==swscanf(tvi->pszText,L"[%d] @%d-%x-%x",&event_slot,&copied_slot,&c,&p)) 
	{
		wcscpy(x,tvi->pszText);
		som_map_zentai_name(copied_slot,x+7,x_s-7);
		if(copied_slot!=event_slot)
		som_map_zentai_rename(event_slot,x+7,x_s-7);	
		tvi->pszText = x;
		return true;
	}
	else return false;
}
extern HTREEITEM som_map_treeview[4] = {0,0,0,0};
static LRESULT CALLBACK som_tool_treeviewproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR hlp)
{		
	//REMOVE ME?
	static int SOM_MAP_treeview = 0; 

	switch(uMsg) 
	{	
	case WM_CONTEXTMENU: //Exselector?
		
		//2018: disabling these menus one at a time (scrollbar)
		return 0;

	case TVM_INSERTITEMW: //SOM only has one of these
	{	
		//Reminder: SOM_MAP sets TVIF_PARAM with TVM_SETITEM 

		if(SOM::tool!=SOM_MAP.exe) break;				

		TVINSERTSTRUCTW *is = (TVINSERTSTRUCTW*)lParam; 
		
		if(TVI_ROOT!=is->hParent&&is->hParent) 
		{							
			if(TreeView_GetParent(hWnd,is->hParent))
			{
				const size_t x_s = 64; wchar_t x[x_s] = L"";
				if(!som_map_treeview_event(&is->item,x,x_s)) assert(0);
			}
		}
		else if(is->item.mask&TVIF_TEXT)
		{
			HWND next = som_tool_x(hWnd); if(next)
			{			
				const size_t x_s = 128;
				WCHAR x[x_s] = L"", xlen = GetWindowTextW(next,x,x_s);		
				is->item.pszText = 
				som_tool_nth(x,xlen,SOM_MAP_treeview,is->item.pszText);
			}							
			switch(SOM_MAP_treeview) //assuming SOM_PRM order
			{
			case 2: case 3: is->hInsertAfter = TVI_FIRST; break;
			}
			LRESULT out = DefSubclassProc(hWnd,uMsg,wParam,lParam);							
			if(SOM_MAP_treeview<4)
			som_map_treeview[SOM_MAP_treeview++] = (HTREEITEM)out;
			else assert(0);
			return out;
		}
		break;
	}
	case TVM_SETITEMW:
	{	
		if(SOM::tool==SOM_MAP.exe)
		if(((TVITEMW*)lParam)->mask&TVIF_TEXT)
		{	
			const size_t x_s = 64; wchar_t x[x_s] = L"";
			som_map_treeview_event((TVITEMW*)lParam,x,x_s);
		}
		break;
	}	 
	case WM_CHAR: case WM_UNICHAR: 

		//doesn't generate NM_RETURN
		//possibly at odds with IDOK buttons
		//if(wParam=='\r') break;
		//suppress beeps and "incremental search" 
		/*if(wParam==' ')*/ return 0; break;
	
	case WM_KEYDOWN:

		if(wParam==VK_SPACE||wParam==VK_DELETE)
		{
			case WM_LBUTTONDBLCLK: //double click activation

			if(SOM::tool==SOM_MAP.exe)
			if(GetParent(hWnd)==som_map_tileview) //future proofing
			{
				HTREEITEM sel = TreeView_GetSelection(hWnd);

				if(TreeView_GetParent(hWnd,sel)) //1161 is the event button							
				{
					bool del = wParam==VK_DELETE&&uMsg==WM_KEYDOWN;
					SendMessage(som_map_tileview,WM_COMMAND,del?1046:1161,0);
					return 0;
				}
			}
		}
		switch(wParam)
		{	  			
		case VK_UP: case VK_DOWN:
		case VK_LEFT: case VK_RIGHT:
		{
			HTREEITEM i = TreeView_GetSelection(hWnd);
			int st = TreeView_GetItemState(hWnd,i,~0);
			LRESULT out = DefSubclassProc(hWnd,uMsg,wParam,lParam);
			if(i==TreeView_GetSelection(hWnd)
			&&st==TreeView_GetItemState(hWnd,i,~0)) 		
			som_tool_neighbor(hWnd,wParam==VK_UP||wParam==VK_LEFT,lParam);
			return out;
		}
		case VK_PRIOR: case VK_NEXT:
		{
			HTREEITEM i = TreeView_GetSelection(hWnd);
			int pos = GetScrollPos(hWnd,SB_VERT);
			LRESULT out = DefSubclassProc(hWnd,uMsg,wParam,lParam);
			if(i==TreeView_GetSelection(hWnd)
			&&pos==GetScrollPos(hWnd,SB_VERT)) 		
			som_tool_neighbor(hWnd,wParam==VK_PRIOR,lParam);
			return out;
		}}		
		break; 
		
	case WM_VSCROLL: case WM_MOUSEWHEEL:
	
		//MS's INTEGRAL IMPLEMENTATION IS COMPLETELY BROKEN//

		if(uMsg==WM_MOUSEWHEEL)
		{
			//Reminder: SOM_MANU can't wheel scroll either.
			//Neither can SOM_MAP. And listbox performance
			//is awful.

			//Reminder: includes SOM_MANU rich-edit
			//container.
			return som_tool_mousewheel(hWnd,wParam); //2017
			return 0; /*just disabling
			int z = GET_WHEEL_DELTA_WPARAM(wParam)/WHEEL_DELTA;
			if(z) if(z>0) while(z--)
			DefSubclassProc(hWnd,WM_VSCROLL,SB_LINEUP,0);
			else while(z++);
			DefSubclassProc(hWnd,WM_VSCROLL,SB_LINEDOWN,0);*/
		}
		else switch(LOWORD(wParam)) 
		{
		case SB_THUMBPOSITION: case SB_THUMBTRACK:		

			HTREEITEM top = TreeView_GetChild(hWnd,TVI_ROOT);			
			if(!top) break;
			
			int ih = -TreeView_GetItemHeight(hWnd);
			RECT ir; TreeView_GetItemRect(hWnd,top,&ir,0);
			int pos = ih*HIWORD(wParam), paranoia = ir.top;
			
			if(pos>=ir.top) 
			for(;ir.top<pos;paranoia=ir.top)
			{
				DefSubclassProc(hWnd,WM_VSCROLL,SB_LINEUP,0);
				TreeView_GetItemRect(hWnd,top,&ir,0);
				if(ir.top==paranoia) break;
			}
			else for(;ir.top>=pos;paranoia=ir.top)
			{
				//overcome anti-fighting bias
				//note: this has to be done in this direction
				//because there is no reverse iteration support
				HTREEITEM vis = TreeView_GetFirstVisible(hWnd);				
				TVITEMEXW tvi = {TVIF_INTEGRAL,vis};
				if(TreeView_GetItem(hWnd,&tvi)&&pos-ir.top<=ih*tvi.iIntegral)
				{
					DefSubclassProc(hWnd,WM_VSCROLL,SB_LINEDOWN,0);
					TreeView_GetItemRect(hWnd,top,&ir,0);
					if(ir.top==paranoia) break;
				}
				else break;
			}			
			return 0;
		}
		break;

	case WM_NCDESTROY: SOM_MAP_treeview = 0; //!!

		RemoveWindowSubclass(hWnd,som_tool_treeviewproc,id);
		break;
	}

	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
static int som_tool_background(DWORD);
extern HBRUSH som_tool_erasebrush(HWND,HDC);
static LRESULT CALLBACK som_tool_trackbarproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR hlp)
{
	switch(uMsg)
	{
	case WM_CONTEXTMENU: //Exselector?
		
		return 0; //2022: annoying //som_map_AfxWnd42sproc needs to do this 

	case WM_LBUTTONDBLCLK:
	{
		if(hlp==57100) switch(id)
		{
		case 1076: lParam = 14; break; //walk //1.5
		case 1077: lParam = 44; break; //dash //4.5
		case 1078: lParam =  3; break; //turn //90
		}
		else lParam = 0;
		//Reminder: SOM must not understand TBM_SETPOSNOTIFY's notification???
		SendMessage(hWnd,TBM_SETPOS,1/*!*/,lParam); //NOTIFY
		SendMessage(GetParent(hWnd),WM_HSCROLL,SB_THUMBPOSITION,(LPARAM)hWnd);
		return 0;
	}
	case TBM_GETPOS: //up is up
	case TBM_SETPOS: case TBM_SETPOSNOTIFY: 
	{
		//HACK: too complicated reversed
		if(id==1005) switch(hlp) 
		{		
		case 42000: case 44000: //assert?
		case 46400: case 46900: case 47300: //2022
				
			goto def; //break; //HACK: double break out :(

		default: assert(0);
		}

		bool reversed = false; 
		HWND edit = GetWindow(hWnd,GW_HWNDNEXT);
		if(DLGC_HASSETSEL&SendMessage(edit,WM_GETDLGCODE,0,0))
		reversed = ES_RIGHT&GetWindowStyle(edit);
		if(reversed!=bool(TBS_VERT&GetWindowStyle(hWnd))) 
		{
			if(uMsg==TBM_GETPOS)
			lParam = DefSubclassProc(hWnd,uMsg,wParam,lParam);
			int n = SendMessage(hWnd,TBM_GETRANGEMIN,0,0);
			int x = SendMessage(hWnd,TBM_GETRANGEMAX,0,0);
			lParam = x-(lParam-n);
			if(uMsg==TBM_GETPOS) 
			return lParam;
		}
		break;
	}
	case WM_KEYDOWN: case WM_KEYUP:
		
		if(wParam==VK_UP||wParam==VK_DOWN)
		if(TBS_VERT&~GetWindowStyle(hWnd)) //TBS_HORZ
		{
			wParam = wParam==VK_UP?VK_DOWN:VK_UP; //reverse these
		}
		break;

	case WM_ERASEBKGND: 
		
		if(WS_EX_TRANSPARENT&GetWindowExStyle(hWnd)) 		
		{
			/*assert(0);*/ ValidateRect(hWnd,0); return 1;
		}
		break;

	case WM_PAINT:
		
		assert(!som_prm_graphing);
		if(!GetUpdateRect(hWnd,0,0)) return 0;
		if(WS_EX_TRANSPARENT&GetWindowExStyle(hWnd)) 
		{	
			RECT bt = {-1}, ch = {-1};

			SendMessage(hWnd,TBM_GETTHUMBRECT,0,(LPARAM)&bt);
			SendMessage(hWnd,TBM_GETCHANNELRECT,0,(LPARAM)&ch);

			//2: assuming standard 1px edge
			int tbs = GetWindowStyle(hWnd), range = ch.right-ch.left-2; 

			if(TBS_VERT&tbs) //funny business
			{
				std::swap(ch.left,ch.top); std::swap(ch.right,ch.bottom);
			}

			if(bt.left!=-1&&ch.left!=-1)
			{	
				HBRUSH br; HDC dc = GetDC(hWnd);
				if(br=som_tool_erasebrush(hWnd,dc)) //NEWER
				{
					RECT cr; //GetClientRect(hWnd,&cr);
					ExcludeClipRect(dc,ch.left,ch.top,ch.right,ch.bottom);
					ExcludeClipRect(dc,bt.left,bt.top,bt.right,bt.bottom);

					//2018: trimming fat to line up with buttons & labels
					//ASSUMING HORIZONTAL KNOBS WON'T EXTEND BEYOND TRACK
					//cr.left = min(ch.left,bt.left);
					//cr.right = max(ch.right,bt.right);
					UnionRect(&cr,&ch,&bt); //assuming won't bother ticks

					FillRect(dc,&cr,br);
					SelectClipRgn(dc,0);
				}
				else //OLDER
				{
					//NEW: prefer to erase/on the appropriate background
					HWND bg = GetParent(hWnd), ga = GetAncestor(hWnd,GA_ROOT);
					int erase = SOM::tool!=SOM_MAIN.exe?RDW_ERASE|RDW_ERASENOW:RDW_NOERASE;
					while(bg!=ga&&WS_EX_TRANSPARENT&GetWindowExStyle(bg))
					bg = GetParent(bg);
					if(som_tool_background(GetWindowContextHelpId(bg)))	
					erase = RDW_ERASE|RDW_ERASENOW;
					//this bit eliminates flicker
					RECT fr = Sompaste->frame(hWnd,bg);
					HRGN rgn = CreateRectRgnIndirect(&fr);
					OffsetRect(&ch,fr.left,fr.top);
					HRGN rgn2 = CreateRectRgnIndirect(&ch);
					OffsetRect(&ch,-fr.left,-fr.top);
					CombineRgn(rgn,rgn,rgn2,RGN_XOR); 
					RedrawWindow(bg,0,rgn,erase|RDW_INVALIDATE|RDW_UPDATENOW|RDW_NOCHILDREN);
					DeleteObject((HGDIOBJ)rgn2);
					DeleteObject((HGDIOBJ)rgn);
				}
				 				
				//todo: support more than SOM_MAIN_154 
				//(someway intelligently divide ticks)
				if(tbs&TBS_AUTOTICKS&&tbs&TBS_NOTICKS)
				{						
					//note: putting ticks on the bottom/right 
					//because of the standard arrow cursor shape
					//and because there is more room on those sides
					int ticks = SendMessage(hWnd,TBM_GETRANGEMAX,0,0);
					if((ticks-=SendMessage(hWnd,TBM_GETRANGEMIN,0,0)))
					{
						if(range<ticks*10) switch(ticks) 
						{
						case 999: ticks = 20; break; //Strength & Magic?

						case 255: ticks = 8; break; //8-bit?

						default: ticks = range/10; 
						}

						float sep = float(range-4)/ticks;
						//note: past 4 clicks don't connect
						//(the margins could also use work)
						const int outside = 4, inside = -4;

						RECT rc = ch; if(TBS_VERT&tbs)
						{
							rc.left=(rc.right+=outside)+inside;
							for(int i=0,start=rc.top+2;i<=ticks;i++)
							{
								rc.bottom=(rc.top=start+int(sep*i+0.5f))+2; 
								DrawEdge(dc,&rc,EDGE_ETCHED,BF_BOTTOMRIGHT); 
							}
						}
						else
						{
							rc.top=(rc.bottom+=outside)+inside;
							for(int i=0,start=rc.left+2;i<=ticks;i++)
							{
								rc.right=(rc.left=start+int(sep*i+0.5f))+2; 
								DrawEdge(dc,&rc,EDGE_ETCHED,BF_BOTTOMRIGHT); 
							}
						}						
					}					
				}

				ReleaseDC(hWnd,dc);

				//draw just ch and bt
				ValidateRect(hWnd,0);
				InvalidateRect(hWnd,&bt,0);
				InvalidateRect(hWnd,&ch,0);								
			}
			else assert(0);
		}
		break;	
	
	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,som_tool_trackbarproc,id);
		break;
	}		  
	
def:return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
inline void som_tool_paint(int l, HWND f, WORD lw=0, WORD hw=0)
{
	//UPDATEUISTATE draws never sending WM_PAINT
	SetWindowRedraw(f,0); SendMessage(f,WM_UPDATEUISTATE,MAKEWPARAM(lw,hw),0);			
	SetWindowRedraw(f,l); InvalidateRect(f,0,0);		
}
static som_tool_RGB som_tool_drawstatictext_black_and_white[2] = {-1,-1};
static void som_tool_drawstatictext(HDC dc, RECT *rc, const wchar_t *text, size_t text_s, int dt, COLORREF c=0)
{	
	dt|=DT_EXPANDTABS;

	if(dt&(DT_VCENTER|DT_BOTTOM)) //2018
	for(size_t i=0;i<text_s;i++) if(text[i]<='\n')
	{
		int div = dt&DT_BOTTOM?1:2;
		dt&=~(DT_SINGLELINE|DT_VCENTER|DT_BOTTOM);
		RECT calc = *rc;
		int h = DrawTextW(dc,text,text_s,&calc,dt|DT_CALCRECT);
		OffsetRect(&calc,0,(rc->bottom-rc->top-h)/div);
		return som_tool_drawstatictext(dc,&calc,text,text_s,dt,c);
	}
		
	if(-1==som_tool_drawstatictext_black_and_white[0].bgr)
	{
		EX::INI::Editor ed;
		//Very simple for now. Not NaN (0) means buttons. NaN here means static text
		som_tool_RGB b = {ed->black_saturation(/*0.0f*/)};
		som_tool_RGB w = {ed->white_saturation(/*0.0f*/)};
		std::swap(b.rgb[0],b.rgb[2]); std::swap(w.rgb[0],w.rgb[2]);
		som_tool_drawstatictext_black_and_white[0] = b;
		som_tool_drawstatictext_black_and_white[1] = w;
	}
		
	//Reminder: disabled text here appears the same
	COLORREF contrast = 0;
	if(c!=0xFFFF) //focus?
	contrast = som_tool_drawstatictext_black_and_white[1].bgr;
	SetTextColor(dc,contrast); 
	OffsetRect(rc,1,1);
	DrawTextW(dc,text,text_s,rc,dt);
	OffsetRect(rc,-1,-1);
	if(c!=0xFFFF)
	c = som_tool_drawstatictext_black_and_white[c!=0].bgr;
	SetTextColor(dc,c);
	DrawTextW(dc,text,text_s,rc,dt);	
}
static LRESULT CALLBACK som_tool_groupboxproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR hlp)
{
	switch(uMsg)
	{
	case WM_ERASEBKGND: 

		if(WS_EX_TRANSPARENT&GetWindowExStyle(hWnd)) 
		return 1;

	case WM_PAINT: 
	{
		if(!GetUpdateRect(hWnd,0,0)) return 0;

		//2018 
		//break;
		//Microsoft! When SOM_PRM_extend_items changes text there
		//are overdraw glitches that shouldn't be. But there is a 
		//problem too in needing to erase the old text background.
	case WM_SETTEXT: 
	
		//The goal here is a transparent background without
		//having the text strucken out by the groupbox border
		if(WS_EX_TRANSPARENT&~GetWindowExStyle(hWnd)) 
		break;

		WCHAR text[96] = L"";
		int text_s = GetWindowTextW(hWnd,text,96); 
		if(!text_s) break;

		//NEW
		if(uMsg==WM_SETTEXT&&!wcscmp(text,(WCHAR*)lParam))
		return 1;

		HDC dc; HGDIOBJ so = 
		SelectObject(dc=GetDC(hWnd),(HGDIOBJ)GetWindowFont(hWnd));
		RECT cr,calc = {}; GetClientRect(hWnd,&cr);							 
		LONG rightmost = -1; settext_pass2:		
		
		//pixel perfect for MS Gothic
		//NOTE: An & in the title throws the border off. But
		//that is inside the default procedure. Maybe it's a
		//good idea to just draw the border. That's all that
		//is left to do afterall.
		DrawTextW(dc,L"X",1,&calc,DT_CALCRECT);
		calc.left = calc.right+GetSystemMetrics(SM_CXEDGE)+1;
		DrawTextW(dc,text,text_s,&calc,DT_CALCRECT);
		if(BS_RIGHT&GetWindowStyle(hWnd)||WS_EX_RIGHT&GetWindowExStyle(hWnd))
		OffsetRect(&calc,cr.right-calc.right-calc.left,0);

		if(uMsg==WM_SETTEXT)
		{
			if(-1==rightmost)
			{
				rightmost = calc.right;

				//Microsoft! when SOM_PRM_extend_items changes text there
				//are overdraw glitches that shouldn't be
				SetWindowRedraw(hWnd,0);
				DefSubclassProc(hWnd,uMsg,wParam,lParam);
				SetWindowRedraw(hWnd,1);

				wcscpy(text,(WCHAR*)lParam);
				goto settext_pass2;				
			}
			else calc.right = max(rightmost,calc.right);
			HWND gp = GetParent(hWnd);
			MapWindowRect(hWnd,gp,&calc);
			RedrawWindow(gp,&calc,0,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ERASE);
		}
		else if(uMsg==WM_PAINT)
		{	
			//2018: this helps a lot with rogue paint jobs where
			//a dropdown or ethched bar is positioned in the top
			//part of the group box
			HWND next = GetWindow(hWnd,GW_HWNDNEXT);
			if(WS_EX_TRANSPARENT&~GetWindowExStyle(next)
			||!IsWindowVisible(next))
			next = 0;
						
			if(next) //detour?
			{
				GetWindowRect(next,&cr);
				MapWindowRect(0,hWnd,&cr);
				//ValidateRect(hWnd,&cr);
				ExcludeClipRect(dc,cr.left,cr.top,cr.right,cr.bottom); 
			}			
						
			SetBkMode(dc,TRANSPARENT);			
			SelectObject(dc,(HGDIOBJ)GetWindowFont(hWnd));
										
			HBRUSH br = som_tool_erasebrush(hWnd,dc); 
			if(br) FillRect(dc,&calc,br); 

			int uis = SendMessage(hWnd,WM_QUERYUISTATE,0,0);
			int dt = UISF_HIDEACCEL&uis?DT_HIDEPREFIX:0;
			//hmm: text sits on som_tool_dialogunits.bottom?
			som_tool_drawstatictext(dc,&calc,text,text_s,dt);

			InflateRect(&calc,2,2);	
			//REDUDANT?
			//InvalidateRect(hWnd,&cr,0); //cr IS next NOW
			ValidateRect(hWnd,&calc); //!	

			if(next) InvalidateRect(next,0,0); //Helps???
		}

		SelectObject(dc,so); ReleaseDC(hWnd,dc);
		if(uMsg==WM_SETTEXT) return 1;
		else break;	
	}}		  
	
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
extern void som_tool_highlight(HWND f, int bt=0)
{		
	if(bt) f = GetDlgItem(f,bt); SetFocus(f);
	som_tool_paint(1,f,UIS_CLEAR,UISF_HIDEFOCUS);
}
static LRESULT CALLBACK som_tool_buttonproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR hlp)
{
	//*SendMessage BYPASSES MANY MESSAGES FOR MANY BUTTONS TYPES :(
	//(Maybe som_tool_msgfilterproc can fix this?)
	//(WM_GETDLGCODE seemed unable to)

	switch(uMsg)
	{	
	case BM_CLICK:
						  	
		//&mnemonics: catch/prevent auto-press
		if(GetKeyState(VK_MENU)>>15) return 0;

		if(~GetKeyState(VK_LBUTTON)>>15) //paranoia
		som_tool_highlight(hWnd); break;

	case WM_SETFOCUS: 
	{
		//for keyboard nav the &based shortcuts get shown
		//that looks bad on individual buttons and anyway
		//Alt is always available (just for that purpose)
		int before = SendMessage(hWnd,WM_QUERYUISTATE,0,0);
		som_tool_paint(0,hWnd,UIS_INITIALIZE,0);
		int op = before&UISF_HIDEACCEL?UIS_SET:UIS_CLEAR;
		som_tool_paint(1,hWnd,op,UISF_HIDEACCEL);
		break;
	}			 
	case WM_GETDLGCODE: //keyboard navigation
	
		return DLGC_WANTARROWS|
		DefSubclassProc(hWnd,uMsg,wParam,lParam);
		
	case WM_KEYUP: case WM_KEYDOWN:

		if(wParam==VK_SPACE) //control?
		{	
			//control controls satellite controls
			//VK_LBUTTON: hack to let SPACE thru to SOM_MAIN.cpp
			if(GetKeyState(VK_CONTROL)>>15) wParam = VK_LBUTTON; 
			//return 0;
			break;
		}
		else if(uMsg!=WM_KEYDOWN) 
		break;

	//case WM_KEYDOWN:

		switch(wParam)
		{		
		case VK_UP: case VK_DOWN:
		case VK_LEFT: case VK_RIGHT:			
		{	
			//2018: NOW IMPLEMENTED BY som_tool_msgfilterproc
			//IN ORDER TO INCLUDE BS_PUSHBUTTON BUTTONS IN IT
			////Apparently radios/checkboxes don't return the
			////DLGC_BUTTON code/and so still reach this code
			////assert(0);
			som_tool_neighbor(hWnd,wParam==VK_UP||wParam==VK_LEFT,lParam);
			return 0;
		}
		case VK_PRIOR: case VK_NEXT:

			som_tool_neighbor(hWnd,wParam==VK_PRIOR,lParam); 
			return 0;
		}
		break;
	
	case WM_LBUTTONDOWN: 

		if(GetKeyState(VK_CONTROL)>>15) 
		{
			som_tool_highlight(hWnd); return 0;
		}
		else if(GetFocus()==hWnd) //remove highlight
		som_tool_paint(1,hWnd,UIS_SET,UISF_HIDEFOCUS);			
		break;

	case WM_ERASEBKGND: //not part of ownerdraw???
		
		//especially noticeable using the classic themes
		if((BS_TYPEMASK&GetWindowStyle(hWnd))==BS_OWNERDRAW)
		return 1;

	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,som_tool_buttonproc,id);
		break;
	}

	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
static LRESULT CALLBACK som_tool_checkboxproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR hlp)
{
	//*SendMessage BYPASSES MANY MESSAGES FOR MANY BUTTONS TYPES :(
	//(Maybe som_tool_msgfilterproc can fix this?)
	//(WM_GETDLGCODE seemed unable to)

	switch(uMsg)
	{	
	case BM_SETCHECK: //*
	{		
		//WINSANITY
		if(SOM::tool==ObjEdit.exe)
		{
			extern WORD workshop_category;			
			if(id>2000&&id<2050)
			{
				//safer than following every radio
				if(id==2031) windowsex_enable(som_tool,1030,1032,wParam);
				if(id==2041) windowsex_enable<1019>(som_tool,wParam);

				if(wParam==1) workshop_category = id-2000;
			}
			else if(wParam==1) switch(id)
			{
			case 1007: workshop_category = 0; break;
			case 1008: workshop_category = 0x14; break;
			case 1009: workshop_category = 0x16; break;
			}
		}		
		break;
	}
	case WM_ERASEBKGND: 
		
		if(WS_EX_TRANSPARENT&GetWindowExStyle(hWnd)) 
		return 1;

	case WM_ENABLE:

		////not working
		//2017: Windows 10 is drawing the text over in gray
		//DefSubclassProc(hWnd,uMsg,wParam,lParam);
		//if(!wParam) InvalidateRect(hWnd,0,0);
		//break;
		return 0&InvalidateRect(hWnd,0,0);		
	
	case WM_PAINT: paint:
		
		if(!GetUpdateRect(hWnd,0,0)) return 0;
		if(WS_EX_TRANSPARENT&GetWindowExStyle(hWnd)) 
		{	
			//Windows does not abide by its own clipping rules here
			//UPDATE: switching over to just use DrawFrameControl to
			//support &mnemonics (Visual Styles are not "transparent")
			LRESULT out = 0;//DefSubclassProc(hWnd,uMsg,wParam,lParam);			
			PAINTSTRUCT ps;	
			BeginPaint(hWnd,&ps); 						
			SetBkMode(ps.hdc,TRANSPARENT);			
			SelectObject(ps.hdc,(HGDIOBJ)SendMessage(hWnd,WM_GETFONT,0,0));
			//vertical centering
			int dt = DT_VCENTER|DT_SINGLELINE|DT_NOCLIP;
			int uis = SendMessage(hWnd,WM_QUERYUISTATE,0,0);
			if(UISF_HIDEACCEL&uis) dt|=DT_HIDEPREFIX;
			RECT calc; GetClientRect(hWnd,&calc);						
			//http://stackoverflow.com/questions/1164868/how-to-get-size-of-check-and-gap-in-check-box/1165052
			int x = MulDiv(GetDeviceCaps(ps.hdc,LOGPIXELSX),13,96); 
			int y = MulDiv(GetDeviceCaps(ps.hdc,LOGPIXELSY),13,96);
			int v = (calc.bottom-y)/2;
			//hack: assuming text is on the right for now (fudging)			
			calc.left = calc.right = x; 				
			static wchar_t text[100] = L""; //chkstk
			int text_s = GetWindowTextW(hWnd,text,EX_ARRAYSIZEOF(text)); 
			OffsetRect(&calc,GetSystemMetrics(SM_CXEDGE)+3,GetSystemMetrics(SM_CYEDGE)); 									
			DrawTextW(ps.hdc,text,text_s,&calc,dt|DT_CALCRECT);			
			DWORD fc = DFCS_TRANSPARENT;
			DWORD bs = GetWindowStyle(hWnd);
			DWORD st = SendMessage(hWnd,BM_GETSTATE,0,0);	
			if(st&1) fc|=DFCS_CHECKED;
			if(bs&BS_FLAT) fc|=DFCS_FLAT;
			if(bs&WS_DISABLED) fc|=DFCS_INACTIVE;
			switch(bs&BS_TYPEMASK)
			{				
			case BS_RADIOBUTTON: case BS_AUTORADIOBUTTON: fc|=DFCS_BUTTONRADIO; break;
			default: fc|=st&BST_INDETERMINATE?DFCS_BUTTON3STATE:DFCS_BUTTONCHECK; break;
			}
			RECT box = {0,v,x,v+y};
			//2018: Multi-line support
			if(y<calc.bottom-calc.top) switch(bs&BS_VCENTER)
			{
			case BS_TOP:					
				OffsetRect(&box,0,calc.top-v-1); //1: fudgetastic
				break;
			case BS_VCENTER: OffsetRect(&box,0,(box.top-calc.top)/2-v);
				break;
			}
			DrawFrameControl(ps.hdc,&box,DFC_BUTTON,fc);			
			if(~st&BST_PUSHED&&UISF_HIDEFOCUS&uis) st = 0;
			som_tool_drawstatictext(ps.hdc,&calc,text,text_s,dt,st&BST_FOCUS?0xFFFF:0);			
			EndPaint(hWnd,&ps);
			return out;
		}					
		break;	

	case WM_KEYDOWN: case WM_LBUTTONDOWN: 
	case BM_CLICK: case WM_SETFOCUS: case WM_GETDLGCODE:

		return som_tool_buttonproc(hWnd,uMsg,wParam,lParam,id,0);
		
	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,som_tool_checkboxproc,id);
		break;
	}		  
	
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
const int som_tool_pushlikepseudoid = -2;
static LRESULT CALLBACK som_tool_pushlikeproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR hlp)
{
	//what's this do? 
	//it converts legacy checkboxes to non-seesaw toggles

	static LPARAM capture = 0;

	//*SendMessage BYPASSES MANY MESSAGES FOR MANY BUTTONS TYPES :(
	//(Maybe som_tool_msgfilterproc can fix this?)
	//(WM_GETDLGCODE seemed unable to)

	switch(uMsg)
	{	
	case WM_ERASEBKGND: return 1;
						
	case WM_PAINT: 
		
		if(GetUpdateRect(hWnd,0,0))
		{
			PAINTSTRUCT ps;	
			BeginPaint(hWnd,&ps);
			int plpid = som_tool_pushlikepseudoid;
			int ods = IsWindowEnabled(hWnd)?0:ODS_DISABLED;
			DRAWITEMSTRUCT dis = {ODT_BUTTON,plpid,0,0,ods,hWnd,ps.hdc,{0},0};
			GetClientRect(hWnd,&dis.rcItem); 			
			SelectObject(ps.hdc,(HGDIOBJ)GetWindowFont(hWnd));
			SendMessage(GetParent(hWnd),WM_DRAWITEM,plpid,(LPARAM)&dis);		
			EndPaint(hWnd,&ps);
		}
		return 0;

	/////begin: WM_PAINT is bypassed
	/////when interacting with the button/checkbox
	case WM_KEYDOWN: 
		
		if(lParam&1<<30) break; //repeating
		case WM_KEYUP:
		if(wParam!=VK_SPACE&&wParam!=VK_RETURN) 
		break;			

	case WM_ENABLE: 

		//if(!wParam) //2018 
		{
			//Microsoft! When SOM_PRM_extend_items changes text there
			//are overdraw glitches that shouldn't be.
			SetWindowRedraw(hWnd,0);
			LRESULT out = DefSubclassProc(hWnd,uMsg,wParam,lParam);
			SetWindowRedraw(hWnd,1);
			InvalidateRect(hWnd,0,0);
			return out;
		}
		break;

	case WM_SETFOCUS:

		/*Doesn't help :(
		//radios BN_CLICKED on focus... but don't check
		//themselves???
		if(BS_RADIOBUTTON&GetWindowStyle(hWnd))
		return 0;*/ //break;

	case BM_SETCHECK: //case WM_ENABLE:
	case WM_KILLFOCUS: //case WM_SETFOCUS:
	case WM_LBUTTONDOWN: //case WM_LBUTTONUP:

		RedrawWindow(hWnd,0,0,RDW_INVALIDATE|RDW_NOERASE);
		break;
		
	case WM_LBUTTONDBLCLK: return 0; //flickers
	case WM_MOUSEMOVE: //winsanity
	{
		//ignores SetWindowRedraw/bypasses WM_PAINT
		//flickers in classic mode if button was up

		if(hWnd==GetCapture()) //DefSubclassProc
		{
			POINT a = {GET_X_LPARAM(capture),GET_Y_LPARAM(capture)};
			POINT b = {GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
			RECT cr; GetClientRect(hWnd,&cr);
			if(PtInRect(&cr,a)!=PtInRect(&cr,b)) 
			RedrawWindow(hWnd,0,0,RDW_INVALIDATE|RDW_NOERASE);
			capture = lParam; return 0;
		}
		else capture = lParam; break;
	}
	case BM_GETSTATE: //continued from above
	{
		if(hWnd!=GetCapture()) break; 		
		int out = DefSubclassProc(hWnd,uMsg,wParam,lParam);
		out&=~BST_PUSHED;
		POINT a = {GET_X_LPARAM(capture),GET_Y_LPARAM(capture)};
		RECT cr; GetClientRect(hWnd,&cr);
		if(PtInRect(&cr,a)) out|=BST_PUSHED;
		return out;
	}
	case WM_LBUTTONUP: //continued from above
	{
		if(hWnd!=GetCapture()) break; 		
		if(BST_PUSHED&~Button_GetState(hWnd)) 
		return 0|ReleaseCapture();
		RedrawWindow(hWnd,0,0,RDW_INVALIDATE|RDW_NOERASE);
		break;
	}
	/////end: WM_PAINT is bypassed
	/////when interacting with the button/checkbox
	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,som_tool_pushlikeproc,id);
		break;
	}
	
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
static LRESULT CALLBACK som_tool_staticproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR hlp)
{
	switch(uMsg)
	{
	case WM_SETTEXT:

		if(IsWindowVisible(hWnd)) //Strength & Magic
		{
			DefSubclassProc(hWnd,uMsg,wParam,lParam);
			RedrawWindow(hWnd,0,0,RDW_INVALIDATE|RDW_ERASE|RDW_INTERNALPAINT|RDW_UPDATENOW|RDW_ERASENOW);
			return 1;
		}
		break;
	
	case WM_PAINT:
	{
		if(!GetUpdateRect(hWnd,0,0)) return 0;
		//modified because WM_CTLCOLORSTATIC can't change disabled text
		bool trans = WS_EX_TRANSPARENT&GetWindowExStyle(hWnd);
		if(!trans&&IsWindowEnabled(hWnd)) break;

		static wchar_t text[1024] = L""; //chkstk
		int text_s = GetWindowTextW(hWnd,text,EX_ARRAYSIZEOF(text)); 

		PAINTSTRUCT ps;			
		BeginPaint(hWnd,&ps);
		HDC dc = ps.hdc; //GetDC(hWnd); 
		RECT rc; GetClientRect(hWnd,&rc); 
		
		SetBkMode(dc,TRANSPARENT);			
		SelectObject(dc,(HGDIOBJ)SendMessage(hWnd,WM_GETFONT,0,0));

		int ss = GetWindowStyle(hWnd);
		//deprecated (taking to mean bottom alignment style)
		int dt = text[text_s-1]=='&'?DT_BOTTOM|DT_SINGLELINE:0;
		if(UISF_HIDEACCEL&SendMessage(hWnd,WM_QUERYUISTATE,0,0))
		dt|=DT_HIDEPREFIX;
		if(ss&SS_RIGHT) dt|=DT_RIGHT;
		if(ss&SS_CENTER) dt|=DT_CENTER; dt|=DT_NOCLIP; //!
		if(ss&SS_NOPREFIX) dt|=DT_NOPREFIX;		
		if(ss&SS_CENTERIMAGE) dt|=DT_VCENTER|DT_SINGLELINE;		
		
		HBRUSH bg = (HBRUSH)SendMessage 
		(GetParent(hWnd),WM_CTLCOLORSTATIC,(WPARAM)dc,(LPARAM)hWnd);			
		FillRect(dc,&rc,bg?bg:GetSysColorBrush(COLOR_3DFACE));

		if(!trans) //this is towards &mnemonics for labels
		{
			SetTextColor(dc,GetSysColor(COLOR_WINDOWTEXT));			
			if(bg&&!(dt&3)) rc.left+=1; //hack? have a 1px margin
		}

		//observed behavior
		if(dt&SS_RIGHT) rc.right-=1;
		if(~ss&SS_CENTERIMAGE) rc.bottom-=1;		
		//FOR SOME REASON WRAPPING JUST DOESN'T WORK???
		if(trans) som_tool_drawstatictext(dc,&rc,text,text_s,dt);
		if(!trans) DrawTextW(dc,text,text_s,&rc,dt|DT_EXPANDTABS);			

		//ReleaseDC(hWnd,dc);
		EndPaint(hWnd,&ps);
		return 0;
	}
	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,som_tool_staticproc,id);
		break;
	}

	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
inline COLORREF som_tool_white()
{
	return GetSysColor(COLOR_WINDOW);
}
extern HBRUSH som_tool_whitebrush()
{		
	//winsanity: GetSysColor(COLOR_WINDOW)
	//returns white whereas GetSysColorBrush
	//returns grey. This manifests itself in the
	//form of many disabled controls having a white
	//inner border surrounding a grey inner background
	static HBRUSH out = CreateSolidBrush(som_tool_white());
	return out;
}
extern HBRUSH som_tool_graybrush()
{
	//reminder: there is no GetSysColor equivalent
	//assuming GetWindowTheme is 0 (the classic theme)
	//(this is the background of readonly edit controls)
	return (HBRUSH)GetStockObject(LTGRAY_BRUSH);
}
extern COLORREF som_tool_gray() //see notes above
{	
	static LOGBRUSH lb; static int one_off =
	GetObject(som_tool_graybrush(),sizeof(lb),&lb);
	return lb.lbColor;
}
static COLORREF som_tool_readonly(WPARAM ro=1)
{
	return ro?som_tool_gray():som_tool_white();
}
static void CALLBACK som_tool_shopcell(HWND,UINT,UINT,DWORD);
static LRESULT CALLBACK som_tool_editcellproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scID, DWORD_PTR listview)
{
	HWND &lv = (HWND&)listview;

	switch(uMsg)
	{			
	case EM_SHOWBALLOONTIP: //comctl32.dll 6.0
		
		return 0; //disables chunky balloon tips
			  
	case WM_WINDOWPOSCHANGING:
	
		/*if(!GetParent(hWnd)) //ListView_GetEditControl?
		{
			WINDOWPOS *p = (WINDOWPOS*)lParam;
			RECT wr; GetWindowRect(lv,&wr);
			p->x+=wr.left; p->y+=wr.top;
		}*/
		break;

	case WM_GETDLGCODE:

		return DLGC_WANTALLKEYS|DefSubclassProc(hWnd,uMsg,wParam,lParam);
	
	case WM_KEYDOWN: //subset of som_tool_textareaproc 

		switch(wParam)
		{		
		case VK_UP: case VK_DOWN:
		case VK_LEFT: case VK_RIGHT:			
		{		
			int wp = wParam;
			bool prev = wp==VK_UP||wp==VK_LEFT;
			wParam = prev?VK_LEFT:VK_RIGHT; 

			DWORD a = -1, z = -1;
			SendMessage(hWnd,EM_GETSEL,(WPARAM)&a,(LPARAM)&z);

			if(z-a) //selection
			{
				if(GetKeyState(VK_SHIFT)>>15) break;
			
				if(prev) SendMessage(hWnd,EM_SETSEL,a,a);
				if(!prev) SendMessage(hWnd,EM_SETSEL,z,z);
				return 0;
			}			
			
			int len = GetWindowTextLengthW(hWnd);
			
			if(a!=0&&prev||!prev&&a!=len) break;				

			//som_tool_neighbor(hWnd,prev,lParam);
			{
				DestroyWindow(hWnd);
				PostMessage(lv,WM_KEYDOWN,wp,0);
				PostMessage(lv,WM_KEYDOWN,VK_RETURN,0);
				if(wp!=wParam) //up/down?
				if(1031==GetDlgCtrlID(lv)) //SOM_PRM?
				SetTimer(lv,wp,50,som_tool_shopcell);
				return 0;
			}
		}
		case VK_ESCAPE: SetFocus(lv); goto cancel;
		}
		break;

	case WM_NCDESTROY:

		//If multiple rows are selected and the cell belongs to
		//one of them, then change them all WRT the same column.
		if(Edit_GetModify(hWnd)&&1<ListView_GetSelectedCount(lv))
		{
			RECT rc; GetWindowRect(hWnd,&rc);
			MapWindowRect(0,lv,&rc);
			LVHITTESTINFO hti = {{rc.left+5,rc.top+5}};			 
			ListView_SubItemHitTest(lv,&hti); WCHAR a[32];
			LVITEMW lvi = {LVIF_PARAM,0,hti.iSubItem,0,0,a};										
			if(ListView_GetItemState(lv,hti.iItem,LVIS_SELECTED))
			if(GetWindowText(hWnd,a,32))
			{
				int lim,&i = lvi.iItem;
				i = _wtoi(a); lim = 1031==GetDlgCtrlID(lv)?65535:250;
				if(i<0) i = 0; if(i>lim) i = lim;
				_itow(i,a,10);							
				for(i=-1;-1!=(i=ListView_GetNextItem(lv,i,LVNI_SELECTED));)
				{
					SendMessageW(lv,LVM_GETITEMW,0,(LPARAM)&lvi);
					if(0xFF!=lvi.lParam)
					SendMessageW(lv,LVM_SETITEMTEXTW,i,(LPARAM)&lvi);
				}
			}
		}

		cancel:
		RemoveWindowSubclass(hWnd,som_tool_editcellproc,scID);
		break;
	}

	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
static LRESULT CALLBACK som_tool_singlelineproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR hlp)
{
	switch(uMsg)
	{			
	case WM_ENABLE:

		//if(wParam) //redraw buddy. Go Microsoft
		{
			//yikes... updown is 0 when there are not updowns
			//and HWNDNEXT is 0 if the last child in a dialog
			if(ATOM a=som_tool_updown())
			if(a==GetClassAtom(GetWindow(hWnd,GW_HWNDNEXT)))
			InvalidateRect(GetWindow(hWnd,GW_HWNDNEXT),0,0);
		}
		break;	 
	
	case WM_MOUSEWHEEL:

		wParam&=~0xFFFF; //Remove modifiers. Otherwise the message is discarded.
		break;

	case WM_LBUTTONDBLCLK:

		SendMessage(hWnd,EM_SETSEL,0,-1);
		return 0;

	case WM_KEYDOWN: //trackbars and tickers

		switch(wParam)
		{		
		case VK_UP: case VK_DOWN:
		case VK_LEFT: case VK_RIGHT:							
		if(GetKeyState(VK_CONTROL)>>15)
		{
			//Shift gets finer increments.
			bool shift = GetKeyState(VK_SHIFT)>>15;
			HWND manip = GetWindow(hWnd,shift?GW_HWNDNEXT:GW_HWNDPREV);					 
			DWORD atom = GetClassAtom(manip);				
			if(som_tool_trackbar()==atom)
			{
				//hack: first update it by simulating mousing 
				SendMessage(GetParent(hWnd),WM_COMMAND,MAKEWPARAM(id,EN_KILLFOCUS),(LPARAM)hWnd);
				return SendMessage(manip,uMsg,wParam,lParam);
			}
			updown: if(som_tool_updown()==atom)
			{
				//todo: WM_LBUTTONDOWN
				int delta = WHEEL_DELTA; //requires UDS_ARROWKEYS
				if(wParam==VK_DOWN||wParam==VK_LEFT) 
				delta = -delta;
				return SendMessage(hWnd,WM_MOUSEWHEEL,delta<<16,0);
			}
			if(!shift)
			{
				//NEW: 2017: Try to add tickers without trackbars?
				//Reminder: whether integer tickers work without the
				//Ctrl modifier, SomEx uses up/down to navigate forward
				//and backward through the controls.				
				atom = GetClassAtom(manip=GetWindow(hWnd,GW_HWNDNEXT));
				shift = true; goto updown;
			}
			return MessageBeep(-1);
		}}	
		break;
		
	case WM_SETFOCUS:

		//treat updowns like sliders
		//(don't auto focus the buddy)
		if(IsLButtonDown())
		{
			POINT pt; GetCursorPos(&pt);
			HWND updown = WindowFromPoint(pt); 
			if(updown!=hWnd)
			if(updown==GetWindow(hWnd,GW_HWNDNEXT))
			{
				SetFocus(updown); return 0;								
			}
		}
		if(som_sys_openscript)		
		if(som_tool_richtext()==GetClassAtom(hWnd))
		som_sys_openscript = hWnd;
		break;

	case WM_KILLFOCUS:
		
		switch(id)
		{
		case 1036: //scale

			if(SOM::tool==SOM_PRM.exe) 
			{
				//this is for end users only
				//(see SetWindowTextA for Delete)
				wchar_t dec[10] = L"";					
				if(GetWindowTextW(hWnd,dec,10)<10)
				{							
					double d = wcstod(dec,0);
					//SOM resets to 0.5 anyway, but that could change
					if(d<=0) SetWindowTextW(hWnd,L"1.0");
				}
			}
		}
		break;

	case WM_PAINT:

		if(hWnd==GetFocus()) break;
		if(!GetUpdateRect(hWnd,0,0)) return 0;			
		if(WS_EX_TRANSPARENT&GetWindowExStyle(hWnd))
		{			 			
			static wchar_t text[250] = L""; //chkstk
			int text_s = GetWindowTextW(hWnd,text,EX_ARRAYSIZEOF(text)); 
			if(!text_s) return 0;

			PAINTSTRUCT ps;			
			BeginPaint(hWnd,&ps);
			HDC dc = ps.hdc; //GetDC(hWnd); 
			RECT rc; GetClientRect(hWnd,&rc); 
			RECT fr; Edit_GetRect(hWnd,&fr); rc.left = fr.left; rc.top = fr.top;
			
			SetBkMode(dc,TRANSPARENT);			
			SelectObject(dc,(HGDIOBJ)SendMessage(hWnd,WM_GETFONT,0,0));

			int es = GetWindowStyle(hWnd);
			int dt = DT_SINGLELINE|DT_NOPREFIX;
						
			if(es&ES_RIGHT) dt|=DT_RIGHT;
			if(es&ES_CENTER) dt|=DT_CENTER; dt|=DT_NOCLIP; //!

			if(dt&ES_RIGHT) rc.right-=1; //ditto

			som_tool_drawstatictext(dc,&rc,text,text_s,dt);

			//ReleaseDC(hWnd,dc);
			EndPaint(hWnd,&ps);

			return 0;
		}
		break;

	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,som_tool_singlelineproc,id);
		break;
	}
	
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
static LRESULT CALLBACK som_tool_realnumberproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR hlp)
{
	switch(uMsg)
	{
	case WM_CHAR: case WM_UNICHAR:

		if(wParam>=' ') //Ctrl+C is 3??? (^C code?) Ctrl+V is 22 
		if(wParam<'0'||wParam>'9')
		{
			switch(wParam)
			{
			case '\b': break; //for some reason this is a character

			case '.': case '-': break;
			
			default: return MessageBeep(-1);
			}
		}
	}
	return som_tool_singlelineproc(hWnd,uMsg,wParam,lParam,id,hlp);
}
static LRESULT CALLBACK som_tool_negintegerproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR hlp)
{
	switch(uMsg)
	{
	case WM_CHAR: case WM_UNICHAR:

		if(wParam>=' ') //Ctrl+C is 3??? (^C code?) Ctrl+V is 22 
		if(wParam<'0'||wParam>'9')
		{
			switch(wParam)
			{
			case '\b': break; //for some reason this is a character

			case '-': break; //case '.': break;
			
			default: return MessageBeep(-1);
			}
		}
	}
	return som_tool_singlelineproc(hWnd,uMsg,wParam,lParam,id,hlp);
}
static bool som_tool_extendedtext(HWND);
static LRESULT CALLBACK som_tool_textareaproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR hlp)
{
	switch(uMsg)
	{			
	case EM_SHOWBALLOONTIP: //comctl32.dll 6.0
		
		return 0; //disables chunky balloon tips

	case EM_SETREADONLY:
		
		if(GetClassAtom(hWnd)==som_tool_richtext()) 
		SendMessage(hWnd,EM_SETBKGNDCOLOR,0,som_tool_readonly());
		break;

	case WM_ENABLE: //make disabled richtext white
		
		if(!som_tool_extendedtext(hWnd)) 
		if(GetClassAtom(hWnd)==som_tool_richtext()) 				
		{
			CHARFORMATW cf = 
			{sizeof(cf),CFM_COLOR,wParam?CFE_AUTOCOLOR:0};			
			cf.crTextColor = GetSysColor(COLOR_GRAYTEXT); 
			SendMessage(hWnd,EM_SETCHARFORMAT,0,(LPARAM)&cf);										   
			return 0;
		}
		break;
	
	case WM_GETDLGCODE: //may not be for 4.1? 

		//Winsanity: Rich Edit controls don't return DLGC_HASSETSEL?!?
		return DLGC_HASSETSEL|DefSubclassProc(hWnd,uMsg,wParam,lParam);
	
	case WM_CONTEXTMENU:

		if(!som_tool_classic) return 0; break;
	
	case WM_MOUSEWHEEL: //disable Rich Text zoom
		
		if(MK_CONTROL&wParam) 
		{
			//classical text, no zoom
			if(!som_tool_extendedtext(hWnd)) return 0; 
			//extended text, up zoom only
			int z = GET_WHEEL_DELTA_WPARAM(wParam); if(z<0)
			{
				DWORD zn=0,zd=0;
				SendMessage(hWnd,EM_GETZOOM,(WPARAM)&zn,(LPARAM)&zd);
				if(zn==1&&zd==1) return 0;
				DefSubclassProc(hWnd,uMsg,wParam,lParam);
				SendMessage(hWnd,EM_GETZOOM,(WPARAM)&zn,(LPARAM)&zd);
				if(zn<zd)
				SendMessage(hWnd,EM_SETZOOM,1,1);
				return 0;
			}
		}
		break;

	case EM_PASTESPECIAL: 

		//som_tool_openscript
		//CF_PRIVATEFIRST+0xED
		if(wParam==0x2ED&&SOM::MO::view&&!lParam)		
		{
			HWND cb = //SOM_MAIN_170
			SOM::MO::view->pastespecial_2ED_clipboard;
			//48*16: SOM_SYS slideshow text
			//2*: in case filled with CRLFs
			static wchar_t text[2*48*16+1] = L""; //chkstk
			SendMessageW(cb,WM_GETTEXT,EX_ARRAYSIZEOF(text),(LPARAM)text);
			Edit_SetSel(hWnd,0,-1);
			SendMessageW(hWnd,EM_REPLACESEL,1,(LPARAM)text);
			return 0;
		}
		break;

	case WM_PASTE: goto paste;

	case WM_KEYDOWN: 

		switch(wParam)
		{		
		case VK_APPS:
			
			return SendMessage(hWnd,EM_RECONVERSION,0,0);

		case VK_UP: case VK_DOWN:
		case VK_LEFT: case VK_RIGHT:			
		{		
			bool prev = wParam==VK_UP||wParam==VK_LEFT;

			int es = GetWindowStyle(hWnd); 

			if(~es&ES_MULTILINE)
			wParam = prev?VK_LEFT:VK_RIGHT; //richedit beeps

			DWORD a = -1, z = -1;
			SendMessage(hWnd,EM_GETSEL,(WPARAM)&a,(WPARAM)&z);

			if(z-a) //selection
			{
				if(GetKeyState(VK_SHIFT)>>15) break;
				if(wParam==VK_UP||wParam==VK_DOWN) break;
				if(prev) SendMessage(hWnd,EM_SETSEL,a,a);
				if(!prev) SendMessage(hWnd,EM_SETSEL,z,z);
				return 0;
			}
			
			if(wParam==VK_UP||wParam==VK_DOWN) //multiline
			{
				int ln = SendMessage(hWnd,EM_LINEFROMCHAR,a,0);
				if(wParam==VK_UP&&ln!=0
				 ||wParam==VK_DOWN&&ln!=Edit_GetLineCount(hWnd)-1)
				break;			
			}
			else 
			{
				int len = GetWindowTextLengthW(hWnd);
				//len--: observed behavior based hack
				if(a==len-1&&som_tool_richtext()==GetClassAtom(hWnd))
				if(Edit_GetLineCount(hWnd)>1) len--;
				if(a!=0&&prev||!prev&&a!=len) break;				
			}			

			som_tool_neighbor(hWnd,prev,lParam); return 0;
		}
		case VK_PRIOR: case VK_NEXT:

			if(GetWindowStyle(hWnd)&ES_MULTILINE)
			{	
				DWORD a = -1, z = -1;
				SendMessage(hWnd,EM_GETSEL,(WPARAM)&a,(WPARAM)&z);
				bool prev = wParam==VK_PRIOR;
				if(z-a||a&&prev||!prev&&a!=GetWindowTextLengthW(hWnd))
				break;				
			}
			som_tool_neighbor(hWnd,wParam==VK_PRIOR,lParam); return 0;	  
		
		case VK_F2:

			switch(SOM::image())
			{case 'prm': case 'map': case 'sys':
			som_tool_openscript(GetParent(hWnd),hWnd); //rich text?
			}break;

		case 'V': 

			if(GetKeyState(VK_CONTROL)>>15)	paste:
			if(!som_tool_extendedtext(hWnd))
			if(GetClassAtom(hWnd)==som_tool_richtext()) 			
			{
				SendMessageW(hWnd,EM_PASTESPECIAL,CF_UNICODETEXT,0);
				uMsg = WM_NULL;
			}
			break;

		case 'Z': //undo quirks

			if(GetKeyState(VK_SHIFT)>>15)
			if(GetKeyState(VK_CONTROL)>>15)
			if(GetClassAtom(hWnd)==som_tool_richtext()) 
			{				
				wParam = 'Y'; //redo
			}
			break;
		}
		//goto readonly_transparent;
		break;

	//readonly_transparent:
	//abandoning: see WM_CTLCOLORSTATIC comctl32 6.0 note
	//REPURPOSING: the entire control is repainted whenever 
	//the mouse cursor is dragged around with a selection so
	//the relationship is reversed in order to counteract that
	case WM_MOUSEMOVE: if(!IsLButtonDown()) break;	
	//case WM_MOUSELEAVE: case WM_LBUTTONDOWN: case WM_LBUTTONUP:
	//case WM_SETTEXT: case EM_SETSEL: 
	case WM_SETFOCUS: case WM_KILLFOCUS: 	

//		READONLY_TRANSPARENT: //HACK: TRANSPARENT BACKGROUND
		
		//hack: SOM_EDIT variable special chars box is blue??
		if(WS_EX_TRANSPARENT&GetWindowExStyle(hWnd))
		{
			DWORD sel = Edit_GetSel(hWnd);		

			static struct{ HWND edit; DWORD sel; }wothere = {0,0};

			if(wothere.edit!=hWnd||wothere.sel!=sel
			  ||uMsg==WM_SETFOCUS||uMsg==WM_KILLFOCUS)
			{
				wothere.edit = hWnd; wothere.sel = sel;

				//now just return static look
				if(uMsg!=WM_KILLFOCUS) break; //NEW
				
				HWND bg = GetParent(hWnd); 
				RECT box = Sompaste->frame(hWnd,bg);
				RedrawWindow(bg,&box,0,RDW_ERASE|RDW_INVALIDATE);				
			}
			else //NEW: STEMMING NEEDLESS REDRAW DURING MOUSE DRAGGING
			{
				//doesn't work/seem necessary if multi-lined
				if(ES_MULTILINE&GetWindowStyle(hWnd)) break;

				SetWindowRedraw(hWnd,0);
				LRESULT out = DefSubclassProc(hWnd,uMsg,wParam,lParam);  
				SetWindowRedraw(hWnd,1);
				RedrawWindow(hWnd,0,0,RDW_NOERASE|RDW_NOINTERNALPAINT|RDW_VALIDATE);				
				return out;
			}
		}
		break;	

	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,som_tool_textareaproc,id);
		break;
	}

	return DefSubclassProc(hWnd,uMsg,wParam,lParam);  
}
static int CALLBACK som_tool_EditWordBreakProc(LPWSTR/*LPSTR*/ lpch, int i, int cch, int code)
{
	//return i; //just works?

	//2017: Some history
	//The Windows 10 Anniversary Edition Update would freeze on input or wrap one if "return i;"
	//is used. So I programmed the following code, that for then worked. The freezing was entering
	//an infinite loop in the RichEdit DLL. I made a comment at a blog that got the friendly-hyperlink
	//underlines problem fixed in the past: 
	//https://blogs.msdn.microsoft.com/murrays/2017/02/27/microsoft-office-math-speech
	//http://blogs.msdn.com/b/murrays/archive/2009/09/24/richedit-friendly-name-hyperlinks.aspx
	//
	//The Windows 10 Creators Edition Update (still 2017) disabled word-wrap completely. Note this is
	//word-break; not word-wrap! With some monkeying around, what worked is to counterintuitively, not
	//indicate that any characters are delimiters.
	
	if(1) switch(code) 
	{
	//case WB_LEFT: return i;

	//Want basic Japanese style wrapping (WYSIWYG in other words.)

	//Windows 10 Creators Update disables word-wrap altogether if all are delimiters.
	//case WB_ISDELIMITER: //lpch[i]!='\r'; //paranoia
	case WB_ISDELIMITER: return 0;

	//MSDN: How to Use Word and Line Break Information
	
	//Just combining all modes for good luck :(
	//case 3/*WB_CLASSIFY*/: return 0x40; //WBF_BREAKAFTER
	case 3/*WB_CLASSIFY*/: return 0x70; //WBF_BREAKAFTER|WBF_BREAKLINE|WBF_ISWHITE
													  		
	//Windows 10 Anniversary Edition Update is going "non responsive" without this.	
	case 4: //WB_MOVEWORDLEFT
	case 6: //WB_LEFTBREAK
	case WB_LEFT: return i+1; //Left-to-Right
	
	case 5: //WB_MOVEWORDRIGHT
	case 7: //WB_RIGHTBREAK
	case WB_RIGHT: return i-1; //Right-to-Left
	}	

	//This is the old way. Note, it freezes when word-wrap is triggered since the Anniversary update.
	//I kind of remember it freezing on input. If so it's upon wrap in the Creators Update. I don't
	//know what's going on at Microsoft these days. If it is at the end of the line, I'm confident it
	//is two distinct bugs, introduced in two consecutive updates. And if not, well the bug's behavior 
	//changed.
	assert(0);
	return i; //just works? (This will freeze in Windows 10. But the compiler wants something here.)
}		  
static LRESULT som_tool_wordbreak(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//easy-selection piggybacks on wordbreak
	if((LRESULT)som_tool_EditWordBreakProc==SendMessageW(hWnd,EM_GETWORDBREAKPROC,0,0))
	{
		SendMessageW(hWnd,EM_SETWORDBREAKPROC,0,0);
		LRESULT out = DefSubclassProc(hWnd,uMsg,wParam,lParam);
		SendMessageW(hWnd,EM_SETWORDBREAKPROC,0,(LPARAM)som_tool_EditWordBreakProc);
		return out;
	}
	else return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
//WTH! WM_GETTFONT return 0 (WINSANITY)
static WPARAM som_map_46100_etc_SETFONT = 0; 
static std::vector<WCHAR> som_tool_multilineproc_text(1,0); //chkstk
static LRESULT CALLBACK som_tool_multilineproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR hlp)
{
	std::vector<WCHAR> &text = som_tool_multilineproc_text; //chkstk

	switch(uMsg)
	{		
	case WM_SETFONT: 

		//this shouldn't be necessary, however WM_GETFONT is a lie??
		som_map_46100_etc_SETFONT = wParam;
		break;

	case WM_SETTEXT: //winsanity: add f'ing \r chars
	{
		text[0] = '\0'; 
		wchar_t *pp=0,*p=(WCHAR*)lParam; do
		{
			p = wcschr(p,'\n');
			if(p&&p[-1]!='\r')
			{
				if(pp) text.insert(text.end(),pp,p);
				else text.assign((wchar_t*)lParam,p);
				text.push_back('\r');
				pp = p; 
			}
		}while(p++); if(pp)
		{
			text.insert(text.end(),pp,pp+wcslen(pp)+1); 
			lParam = (LPARAM)&text[0];
		}
		break;
	}
	case WM_PAINT:

		if(hWnd==GetFocus()) break;
		if(!GetUpdateRect(hWnd,0,0)) return 0;		
		if(WS_EX_TRANSPARENT&GetWindowExStyle(hWnd))
		{			 			
			text.reserve(1024); //assuming safe
			int text_s = GetWindowTextW(hWnd,&text[0],1024);
			if(!text_s) return 0;

			PAINTSTRUCT ps;			
			BeginPaint(hWnd,&ps);
			HDC dc = ps.hdc; //GetDC(hWnd); 
			RECT rc; GetClientRect(hWnd,&rc); 
			RECT fr; Edit_GetRect(hWnd,&fr); rc.left = fr.left; rc.top = fr.top;
			
			SetBkMode(dc,TRANSPARENT);			
			SelectObject(dc,(HGDIOBJ)SendMessage(hWnd,WM_GETFONT,0,0));

			int es = GetWindowStyle(hWnd);			
			int dt = DT_WORD_ELLIPSIS|DT_NOPREFIX;
			
			//DrawText word break is not like SOM's/som_tool_EditWordBreakProc
			//if(~es&ES_AUTOHSCROLL) dt|=DT_WORDBREAK;
						   
			if(es&ES_RIGHT) dt|=DT_RIGHT;
			if(es&ES_CENTER) dt|=DT_CENTER; //dt|=DT_NOCLIP; //!

			if(dt&ES_RIGHT) rc.right-=1; //ditto

			//rc.left+=1; rc.right+=1; //line up focus

			som_tool_drawstatictext(dc,&rc,&text[0],text_s,dt);

			//ReleaseDC(hWnd,dc);
			EndPaint(hWnd,&ps);

			return 0;
		}
		break;	

	case WM_KEYDOWN:

		switch(wParam)
		{
		case VK_LEFT: case VK_RIGHT:

			if(GetKeyState(VK_CONTROL)>>15)
			return som_tool_wordbreak(hWnd,uMsg,wParam,lParam);
			
			//looks dangerous to SOM_MAIN_richedproc
		//case VK_ESCAPE: //WINSANITY?		
			//return SendMessage(GetParent(hWnd),WM_COMMAND,IDCANCEL,0);
		}
		break;

	case WM_LBUTTONDBLCLK: 

		return som_tool_wordbreak(hWnd,uMsg,wParam,lParam);

	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,som_tool_multilineproc,id);
		break;
	}

	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
static LRESULT CALLBACK som_tool_updownproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR hlp)
{
	switch(uMsg) //not doing anything?
	{	
	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,som_tool_updownproc,id);
		break;
	}		  
	
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
static LRESULT CALLBACK som_tool_progressproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR hlp)
{
	switch(uMsg)
	{		
	/*EXPERIMENTAL 
	//Trying to hide slit-second windows :(
	case WM_WINDOWPOSCHANGING: 
		
		((WINDOWPOS*)lParam)->flags|=SWP_HIDEWINDOW;
		break;*/

	case PBM_STEPIT: //SOM_PRM uses STEPIT
	case PBM_SETPOS: //used by som_tool_prt
	{		
		MSG keepalive; 
		while(PeekMessageW(&keepalive,0,0,0,PM_REMOVE))
		if(!IsDialogMessageW(GetActiveWindow(),&keepalive))		
		{
			TranslateMessage(&keepalive); DispatchMessageW(&keepalive);
		}	
		/*if(0) //EXPERIMENTAL
		{
			//2018: pause at the end so it doesn't look like a
			//glitch and so the completion status can be shown
			lParam = DefSubclassProc(hWnd,uMsg,wParam,lParam);
			wParam = SendMessage(hWnd,PBM_GETPOS,0,0);
			if(wParam!=lParam)
			if(wParam==SendMessage(hWnd,PBM_GETRANGE,0,0))
			for(int i=0;i<5;i++)
			{
				EX::sleep(50);
				if(IsWindow(hWnd))
				SendMessage(hWnd,PBM_SETPOS,wParam,0);
				else break;
			} 
			return lParam;
		}
		else*/ break;
	}
	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,som_tool_progressproc,id);
		break;
	}		  
	
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
static HWND som_tool_timedate(HWND dt)
{
	return GetDlgItem(GetParent(dt),GetDlgCtrlID(dt)^1);	
}
static LRESULT CALLBACK som_tool_datetimeproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR hlp)
{
	switch(uMsg)
	{	
	case WM_SETTEXT: 
	{
		//extension for SOM_MAIN.cpp
		NMDATETIMESTRINGW dts = {{hWnd,GetDlgCtrlID(hWnd),DTN_USERSTRING}};
		dts.pszUserString = (WCHAR*)lParam;
		dts.st.wDay = 0xABBA; dts.dwFlags = 'tool';
		SendMessage(GetParent(hWnd),WM_NOTIFY,dts.nmhdr.idFrom,(LPARAM)&dts);
		if(dts.st.wDay==0xABBA) return 0;
		DateTime_SetSystemtime(hWnd,0,&dts.st);
		DateTime_SetSystemtime(som_tool_timedate(hWnd),0,&dts.st);
		return 1;
	}
	case WM_KEYDOWN: 

		switch(wParam)
		{		
		case VK_UP: if(IsLButtonDown()) break;
		
		case VK_LEFT: case VK_PRIOR:
						
			if(GetKeyState(VK_CONTROL)>>15) break;
			som_tool_neighbor(hWnd,true,lParam); return 0;	  		
		
		case VK_DOWN: if(IsLButtonDown()) break;

		case VK_RIGHT: case VK_NEXT:

			if(GetKeyState(VK_CONTROL)>>15) break;
			som_tool_neighbor(hWnd,false,lParam); return 0;	  		

		case VK_F4: case VK_SPACE: //open calendar

			wParam = VK_F4;
			if(8!=(0xC&GetWindowStyle(hWnd))) break;
			SetFocus(hWnd=som_tool_timedate(hWnd)); //!
			return SendMessage(hWnd,uMsg,wParam,lParam);

		case 'A': //select all (for copy)

			if(GetKeyState(VK_CONTROL)>>15)
			return SendMessage(hWnd,WM_LBUTTONDOWN,0,0);

		case VK_DELETE: //set to the current time

			if(som_tool_gray() //SOM_MAIN.cpp
			!=DateTime_GetMonthCalColor(hWnd,MCSC_TITLEBK))
			{
				SYSTEMTIME st; GetLocalTime(&st);
				DateTime_SetSystemtime(hWnd,0,&st);
				DateTime_SetSystemtime(som_tool_timedate(hWnd),0,&st);				
			}
			else MessageBeep(-1);
			return 0;
		}
		break;	

	case WM_LBUTTONDBLCLK: //select all

		//wouldn't be necessary except that clicking
		//on a different field exits field edit mode
		return SendMessage(hWnd,WM_LBUTTONDOWN,0,0);

	case WM_LBUTTONDOWN: //see above commends

		if(lParam) SetFocus(0); break; //select field

	case WM_CHAR: //beeps

		switch(wParam){	case ' ': return 0;	}break;

	//sharing ticker
	case WM_VSCROLL: return 0; //sends SB_THUMBPOSITION
	case WM_NOTIFY:
	{
		NMUPDOWN *p = (NMUPDOWN*)lParam;

		if(p->hdr.code==UDN_DELTAPOS)
		{
			HWND f = GetFocus();
			if(f!=hWnd&&f!=som_tool_timedate(hWnd)) 
			SetFocus(hWnd);
			SendMessage(GetFocus(),WM_KEYDOWN,p->iDelta>0?VK_UP:VK_DOWN,0); 
			return 1;
		}
		break;
	}
	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,som_tool_datetimeproc,id);
		break;
	}

	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
static LRESULT CALLBACK som_tool_ipaddressproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR hlp)
{
	switch(uMsg)
	{	
	case WM_ERASEBKGND: return 1;

	case WM_PAINT: //make white even when disabled
	{	
		if(!GetUpdateRect(hWnd,0,0)) return 0;

		PAINTSTRUCT ps;	
		BeginPaint(hWnd,&ps);

		if(!IsWindowEnabled(hWnd))
		lParam = SendMessage(hWnd,WM_CTLCOLORSTATIC,(WPARAM)ps.hdc,lParam);
		else lParam = (LPARAM)som_tool_whitebrush();		
		FillRect(ps.hdc,&ps.rcPaint,(HBRUSH)lParam);				
		//if(!IsWindowEnabled(hWnd)&&SendMessage(hWnd,IPM_ISBLANK,0,0))
		//return 0;
		RECT rects[4]; //must draw the dots
		HWND ch = GetWindow(hWnd,GW_CHILD); 
		SelectObject(ps.hdc,(HGDIOBJ)GetWindowFont(ch));
		for(int i=0;i<4;i++,ch=GetWindow(ch,GW_HWNDNEXT))
		{
			GetWindowRect(ch,rects+i); 
			MapWindowRect(0,hWnd,rects+i);
		}
		UnionRect(rects+0,rects+1,rects+0);
		UnionRect(rects+1,rects+2,rects+3);
		UnionRect(rects+2,rects+0,rects+1);
				
		for(int i=0;i<3;i++)
		{
			rects[i].left+=2; //fudge
			DrawText(ps.hdc,L".",1,rects+i,DT_CENTER|DT_SINGLELINE);
		}
		EndPaint(hWnd,&ps);
		return 0;
	}	
	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLOREDIT:
		
		//HACK: the tools prefer white backgrounds, whereas workshop
		//is using a gray
		if(!IsWindowEnabled(hWnd))
		{
			//don't understand why, but letting it override the text
			//color produces black!
			//SetTextColor((HDC)wParam,GetSysColor(COLOR_GRAYTEXT));
			lParam = SendMessage(GetParent(hWnd),WM_CTLCOLORSTATIC,wParam,lParam);
			SetTextColor((HDC)wParam,GetSysColor(COLOR_GRAYTEXT));
			return lParam;
		}

		if(!IsWindowEnabled((HWND)lParam))		
		SetTextColor((HDC)wParam,GetSysColor(COLOR_GRAYTEXT));	
		return (LRESULT)som_tool_whitebrush();

	case WM_ENABLE: //WINSANITY?!

		//if(wParam!=IsWindowEnabled(hWnd))
		RedrawWindow(hWnd,0,0,RDW_ALLCHILDREN|RDW_INVALIDATE|RDW_UPDATENOW);
		break; //return 0;

	case WM_SETFOCUS:

		//workshop_ipchangeling?
		hWnd = GetWindow(GetWindow(hWnd,GW_CHILD),GW_HWNDLAST);
		SetFocus(hWnd); 
		SendMessage(hWnd,EM_SETSEL,0,-1); //NEW
		return 0;

	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,som_tool_ipaddressproc,id);
		break;
	}

	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
static LRESULT CALLBACK som_tool_iprangeproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR hlp)
{		
	switch(uMsg)
	{			
	case EM_SHOWBALLOONTIP: //comctl32.dll 6.0
		
		return 0; //disables chunky balloon tips
		
	case WM_CONTEXTMENU: return 0;
		
	case WM_ERASEBKGND: return 1;

	case WM_CHAR: case WM_UNICHAR: //workshop_ipchangeling

		if(!isdigit(wParam)) switch(wParam)
		{
		case '-': case '\x08': break; //backspace
		
		default: 
			
			if(~GetKeyState(VK_CONTROL)>>15) //Ctrl+Z???
			return 0;
		}
		break;

	case WM_KEYDOWN: 

		switch(wParam)
		{		
		case VK_UP: case VK_DOWN: 
			
			if(GetKeyState(VK_CONTROL)>>15)
			{
				wchar_t w[8]; GetWindowTextW(hWnd,w,8);
				int i = _wtoi(w)+(wParam==VK_UP?1:-1);
				if(i>=0) SetWindowTextW(hWnd,_itow(i,w,10)); 
				else MessageBeep(-1);
				Edit_SetSel(hWnd,0,-1);
				return 0;				
			}

			//doesn't increment? seems to behave the
			//same as down below but without looping
			//if(GetKeyState(VK_CONTROL)>>15) break;			
			wParam = wParam==VK_UP?VK_LEFT:VK_RIGHT;
			//break;

		case VK_LEFT: case VK_RIGHT: 
		{
			if(GetKeyState(VK_SHIFT)>>15) break;

			HWND neighbor = hWnd; //RTL Z-order			
			neighbor: 
			neighbor = GetWindow(neighbor,wParam==VK_LEFT?GW_HWNDNEXT:GW_HWNDPREV);
			if(GetKeyState(VK_CONTROL)>>15) 
			{
				if(!neighbor) neighbor = 
				GetWindow(hWnd,wParam==VK_LEFT?GW_HWNDFIRST:GW_HWNDLAST);
				Edit_SetSel(neighbor,0,-1);
				SetFocus(neighbor); return 0;
			}
			else if(neighbor) //workshop_ipchangeling?
			{			
				if(!IsWindowEnabled(neighbor))
				goto neighbor;
				//REMINDER: unlike the time/date controls, these are 
				//made to act like 4 consecutive text input controls
				//--but also like one-long one insofar as Control is
				//used to do select-all while navigating over ranges
				//break;
			}

		  //// edit control logic ////
			
			DWORD a = -1, z = -1;
			SendMessage(hWnd,EM_GETSEL,(WPARAM)&a,(WPARAM)&z);
			bool prev = wParam==VK_LEFT; if(z-a)
			{					
				if(prev) SendMessage(hWnd,EM_SETSEL,a,a);
				if(!prev) SendMessage(hWnd,EM_SETSEL,z,z);
				return 0;
			}			
			if(a!=0&&prev||!prev&&a!=GetWindowTextLengthW(hWnd)) 
			break;			
			
			if(neighbor) //workshop_ipchangeling?
			{
				//REMINDER: unlike the time/date controls, these are 
				//made to act like 4 consecutive text input controls
				//--but also like one-long one insofar as Control is
				//used to do select-all while navigating over ranges
				SetFocus(neighbor); return 0;
			}

			wParam = prev?VK_PRIOR:VK_NEXT; //break;
		}
		case VK_PRIOR: case VK_NEXT:
		
			som_tool_neighbor(GetParent(hWnd),wParam==VK_PRIOR,lParam); 			
			return 0;	  		
		}
		break;	

	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,som_tool_iprangeproc,id);
		break;
	}

	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}

extern void som_tool_icon(HWND hw)
{
	HICON icon = //128: the ID used by SOM's tools	
	LoadIcon(GetModuleHandle(0),MAKEINTRESOURCE(128));
	SendMessage(hw,WM_SETICON,ICON_SMALL,(LPARAM)icon);
	SendMessage(hw,WM_SETICON,ICON_BIG,(LPARAM)icon);
}

//REMIND ME? What precisely is the benefits of som_tool_taskbar?
//Does it help ItemEdit.exe, etc?
//(A: clicking buttons on the taskbar won't minimize without it)
extern HWND som_tool_taskbar = 0;
static INT_PTR CALLBACK som_tool_taskbarproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{		
	case WM_INITDIALOG:
	{				
		som_tool_icon(hwndDlg);
		SetWindowTextW(hwndDlg,EX::exe());	
		SetWindowPos(hwndDlg,0,0,0,0,0,SWP_NOMOVE);		
		SOM::initialize_taskbar(hwndDlg);
		return 1;
	}	
	case WM_WINDOWPOSCHANGING: 
		
		//paranoia 		
		((WINDOWPOS*)lParam)->cx = ((WINDOWPOS*)lParam)->cy = 0; 
		break;

	case WM_SHOWWINDOW:	assert(wParam);	break;

	//ShowOwnedPopups?
	case WM_SYSCOMMAND: 

		switch(wParam) //nothing?
		{
		//permits nested close out
		case SC_CLOSE: goto close;

		/*what about nested Minimize?
		case SC_RESTORE:; //assert(0);
		case SC_MAXIMIZE: //assert(0); break;
		
			ShowWindow(som_tool,1); break;
		
		case SC_MINIMIZE: //assert(0); //break;		
		
			ShowWindow(som_tool,0); break;*/
		}
		break;

	case WM_SIZE: //nothing?

		switch(wParam)
		{		
		case SIZE_MINIMIZED: break;				
		case SIZE_MAXIMIZED: assert(0); break;
		case SIZE_RESTORED: goto fore;
		}
		break;
		
	case WM_NCACTIVATE:  
		
		if(wParam) fore:
		{	
			HWND top = som_tool_dialog();
			if(top) SetActiveWindow(GetAncestor(top,GA_ROOT));
			return 1;
		}
		break;
		
	case WM_CLOSE: close: //taskbar   			
	
		if(som_tool!=GetAncestor(som_tool_dialog(),GA_ROOT))
		{
			//NEW: just close even if there are nesting dialogs		
			//Destory/Sleep: hide windows before removing hooks
			DestroyWindow(som_tool);EX::sleep(250);ExitProcess(0);
		}
		else 
		{
			SendMessage(som_tool,WM_CLOSE,0,0);				
			//SOM_PRM prompts to save work twice
			return 1;
		}
		break;
	}	

	return 0;
}

extern int som_tool_subclass(HWND ctrl, SUBCLASSPROC proc)
{
	int id = GetDlgCtrlID(ctrl);
	HWND gp = GetParent(ctrl);
	int hlp = GetWindowContextHelpId(gp);
	if(!hlp) //HACK: combobox1001
	hlp = GetWindowContextHelpId(som_tool_dialog());
	SetWindowSubclass(ctrl,proc,id,hlp); return id;
}

#define return_atom_if_not(hw) static ATOM atom = 0;\
if(!hw) return atom; if(!atom) atom = GetClassAtom(hw);
static ATOM som_tool_static(HWND st=0)
{
	return_atom_if_not(st)
	SetWindowFont(st,som_tool_charset,0);

	int ss = GetWindowStyle(st);
	switch(ss&SS_TYPEMASK)
	{
	case SS_CENTER: 	
	case SS_LEFT: case SS_RIGHT: 
	case SS_SIMPLE: case SS_LEFTNOWORDWRAP: //sketchy???
	
		if(ss&SS_SUNKEN) break; //don't know how to support yet

		if(GetWindowExStyle(st)
		&(WS_EX_CLIENTEDGE|WS_EX_STATICEDGE
		|WS_EX_DLGMODALFRAME|WS_EX_WINDOWEDGE)) break; //ditto

		som_tool_subclass(st,som_tool_staticproc);
				
		//HACK: this is a workaround due to ResEdit resetting
		//the width of controls to a minimum value guessed to
		//be 8 units since the base units are 4 per the width
		//12 is to allow for subunit (or per pixel) centering
		//(as of 2015 ResEdit's maintainer cannot be reached)
		if(~ss&SS_CENTER) break; //concerned with wedged text
		RECT rc,calc = {0,0,0,0}; 
		if(GetClientRect(st,&rc))
		if(rc.right<som_tool_dialogunits.right*12)
		{	
			int dt = ss&SS_NOPREFIX?DT_NOPREFIX:0;
			HDC dc = GetDC(st); wchar_t text[1024] = L""; 
			GetWindowTextW(st,text,EX_ARRAYSIZEOF(text));
			DrawTextW(dc,text,-1,&calc,dt|DT_CALCRECT);		
			LONG trim = rc.right-calc.right; if(trim>1)
			{
				GetWindowRect(st,&rc);
				rc.left+= trim-trim/2; rc.right-=trim/2;
				MapWindowRect(0,GetParent(st),&rc); 
				SetWindowPos(st,0,rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,SWP_NOZORDER);		
			}
			ReleaseDC(st,dc); //!!
		}
	}
	return atom;
}
static ATOM som_tool_button(HWND bt=0)
{	
	return_atom_if_not(bt)
	SetWindowFont(bt,som_tool_charset,0);

	int bs = GetWindowStyle(bt);
	int ex = GetWindowExStyle(bt);
	
	switch(bs&BS_TYPEMASK)
	{
	case BS_GROUPBOX:
	
		if(WS_EX_TRANSPARENT&ex)
		som_tool_subclass(bt,som_tool_groupboxproc);
		break;

	//FRUSTRATING
	//2018/PrtsEdit, etc??
	case BS_DEFPUSHBUTTON: //Note: ResEdit doesn't have BS_PUSHBOX
	case BS_PUSHBUTTON: 

		//dialogs can't do mixed control type groups...
		
		//SUBCLASSING doesn't work for these buttons!!!
		//they don't receive any messages more or less!
		goto crud; ////break;

	case BS_OWNERDRAW: //standard SOM button

		som_tool_subclass(bt,som_tool_buttonproc);
		//break;

	default: crud:

		//Windows 10 is double-clicking buttons???
		//remove double-click so there are two clicks.
		if(CS_DBLCLKS&GetClassLong(bt,GCL_STYLE))
		SetClassLong(bt,GCL_STYLE,~CS_DBLCLKS&GetClassLong(bt,GCL_STYLE));
		break;
	
	case BS_3STATE: case BS_AUTO3STATE:
	case BS_CHECKBOX: case BS_AUTOCHECKBOX:
	case BS_RADIOBUTTON: case BS_AUTORADIOBUTTON:
		
		if(BS_PUSHLIKE&bs) //restyle legacy checkbox?
		{					
			som_tool_subclass(bt,som_tool_buttonproc);
			if(WS_EX_TRANSPARENT&ex)
			som_tool_subclass(bt,som_tool_pushlikeproc);
			break;
		}
		//reminder: calls through to som_tool_buttonproc 
		som_tool_subclass(bt,som_tool_checkboxproc);
		break;
	}
	return atom;
}
extern ATOM som_tool_combobox(HWND cb=0)
{	
	return_atom_if_not(cb)
	SetWindowFont(cb,som_tool_typeset,0);

	int cbs = GetWindowStyle(cb);
	//hack: using CBS_AUTOHSCROLL as 
	//a marker for resize on CBN_DROPDOWN
	//Note: seems to be harmless for boxes with 
	//edit controls since not consulted one_off created
	//(still this should be done a more professional way)
	if(cbs&CBS_AUTOHSCROLL)
	SetWindowLong(cb,GWL_STYLE,cbs&~CBS_AUTOHSCROLL);
	int id = som_tool_subclass(cb,som_tool_comboboxproc);
	
	HWND em = GetDlgItem(cb,1001); if(em) //SOM_EDIT_151
	{
		som_tool_subclass(em,som_tool_combobox1001);
		SetWindowTheme(em,L"",L""); 
	}
		
	COMBOBOXINFO cbi = {sizeof(cbi)};
	if(GetComboBoxInfo(cb,&cbi))
	SetWindowTheme(cbi.hwndList,L"",L""); else assert(0);

	if(SOM::tool==SOM_PRM.exe&&id==1075)
	{			 
		som_tool_media(cb,CB_ADDSTRING,L"PICTURE"); //item
		ComboBox_SetCurSel(cb,-1); 
	}
	
	if(SOM::tool==SOM_PRM.exe) 
	{
		if(id==1002) //profiles
		som_tool_boxen.clear(cb); 
	}
	if(SOM::tool==SOM_MAP.exe) switch(id) 
	{
		//whitelist. including some false positives
		//just try to keep the number onscreen down

	case 1021: //placement type
	case 1044: case 1046: //items
	case 1170: //event, various.
	case 1182: //warp maps
	case 1188: case 1191: case 1192: //counters	
	case 1206: //terminate event list?
	case 1213: //event subject

		som_tool_boxen.clear(cb); break;
	}

	return atom;
}
static ATOM som_tool_listbox(HWND lb)
{
	return_atom_if_not(lb)
	SetWindowFont(lb,som_tool_typeset,0);

	//ORDER-IS-IMPORTANT
	//this works, but is it guaranteed to?
	//https://stackoverflow.com/a/7136491
	if(LBS_EXTENDEDSEL&GetWindowStyle(lb))
	som_tool_subclass(lb,som_tool_LBS_EXTENDEDSEL);
	int id = som_tool_subclass(lb,som_tool_listboxproc);

	if(SOM::tool==SOM_PRM.exe) 
	{
		//1220/1023/1029/1030/1034/1219/1220
		som_tool_boxen.clear(lb);
	}
	if(SOM::tool==SOM_MAP.exe) switch(id)
	{	
	case 1210: case 1239: //events/Maps lists (assuming unique)
		
		som_tool_boxen.clear(lb);
		break;
		
	case 1047: //media selection menu
	{
		//todo: track buttons used to open windows
		const int paranoia = GetWindowContextHelpId(GetParent(lb));

		if(paranoia==46700||paranoia==46800) switch(som_map_instruct)
		{
		case 13: som_tool_media(lb,LB_ADDSTRING,L"PICTURE"); break;	
		case 14: som_tool_media(lb,LB_ADDSTRING,L"MOVIE"); break;	
		case 16: som_tool_media(lb,LB_ADDSTRING,L"BGM"); 
				 som_tool_media(lb,LB_ADDSTRING,L"SOUND\\BGM"); break;	
		}
		else assert(0); break;
	}}

	return atom;
}
static int som_tool_textarea(HWND ed)
{	
	//return_atom_if_not(ed)	
	int id = som_tool_subclass(ed,som_tool_textareaproc);

	SetWindowFont(ed,id==-1?som_tool_charset:som_tool_typeset,0);	

	LONG es = GetWindowStyle(ed);

	if(es&ES_MULTILINE)
	{	
		som_tool_subclass(ed,som_tool_multilineproc);
		
		//this ensures SOM's edit boxes are WYSIWYG
		SendMessageW(ed,EM_SETWORDBREAKPROC,0,(LPARAM)som_tool_EditWordBreakProc);

		//assuming SOM styles
		if(es&ES_AUTOHSCROLL) //disable
		SetWindowLong(ed,GWL_STYLE,es&~ES_AUTOHSCROLL);
	}
	else 
	{
		char test[8]; 
		if(1==GetWindowTextA(ed,test,8)) //2020
		{
			//this is for centered workshop templates
			//there's some default text that shouldn't
			//be cleared in other templates
			SetWindowTextA(ed,""); 
		}

		/*DICEY
		//2022: I'm not 100% all boxes with . in them are 
		//float fields! I'm expanding it to cover the two
		//boxes on SOM_MAP's main screen
		//if(*test=='.')*/
		if(strchr(test,'.'))
		som_tool_subclass(ed,som_tool_realnumberproc);
		else if(*test=='-')
		som_tool_subclass(ed,som_tool_negintegerproc); //2022
		else
		som_tool_subclass(ed,som_tool_singlelineproc);

		//assuming SOM styles
		if(~es&ES_AUTOHSCROLL) //enable
		SetWindowLong(ed,GWL_STYLE,es|ES_AUTOHSCROLL); 				
	}
	return id;
}
static ATOM som_tool_edittext(HWND ed)
{	
	return_atom_if_not(ed) 
	
	int id = som_tool_textarea(ed);

	//SOM_MAP_colorfulproc
	if(SOM::tool==SOM_MAP.exe) switch(id)
	{
	case 1080: case 1081: case 1082: case 1083: case 1084:
		
		HWND gp = GetParent(ed);
		int hlp = GetWindowContextHelpId(gp); switch(hlp+id)
		{
		case 42400+1080: case 44000+1084: case 46000+100+1080:
		case 43000+1080: case 43000+1081: case 43000+1082: case 43000+1083:

			//HACK: must enable before subclassing
			EnableWindow(ed,1);	Edit_LimitText(ed,3);
			extern windowsex_SUBCLASSPROC SOM_MAP_colorproc,SOM_MAP_colorfulproc;
			som_tool_subclass(ed,SOM_MAP_colorproc);
			SetWindowSubclass(gp,SOM_MAP_colorfulproc,0,hlp);	
			SendMessage(GetWindow(ed,GW_HWNDNEXT),UDM_SETRANGE32,0,240);			
			break;
		}
		break;
	}

	return atom;
}
static void som_tool_editcell(HWND ed, HWND lv)
{
	//TODO? Might want to set up limits/invisible updown buddy

	//Reminder: here the box is 1x2 at 0,0
	SetWindowSubclass(ed,som_tool_editcellproc,0,(DWORD_PTR)lv);			
}
//#define IMF_SPELLCHECKING 0x00000800
static LPARAM som_tool_richspell = 0x0800; 
extern char som_main_richinit_pt[32] = "";
extern HWND som_tool_richinit(HWND ed, int em=0)
{	
	//prevent causing an EN_CHANGE event
	SendMessage(ed,EM_SETEVENTMASK,0,0); 

	if(som_tool_richspell)
	if((GetVersion()&0x206)>=0x200 //8/10?
	&&2000!=EX::INI::Editor()->spellchecker_mode)
	{	
		//http://blogs.msdn.com/b/murrays/archive/2012/08/31/
		//richedit-spell-checking-autocorrection-and-prediction.aspx
		SendMessage(ed,EM_SETEDITSTYLE,~0, //base spellchecking requirement
		SES_USECTF|SES_CTFALLOWEMBED|SES_CTFALLOWSMARTTAG|SES_CTFALLOWPROOFING);
	}
	else som_tool_richspell = 0;

	//NEW: remove blue/underline styling
	//(a fix for underlines if forthcoming)
	//EM_SETEDITSTYLEEX,0,SES_EX_HANDLEFRIENDLYURL
	//http://blogs.msdn.com/b/murrays/archive/2009/09/24/richedit-friendly-name-hyperlinks.aspx?CommentPosted=true#10634641
	SendMessage(ed,WM_USER+275,0,0x00000100);
	//2018 (workshop) makes selection match normal
	//edit controls
	//https://blogs.msdn.microsoft.com/murrays/2015/03/27/richedit-colors/
	SendMessage(ed,WM_USER+275,0x00100000,0x00100000); //SES_EX_NOACETATESELECTION
	
	//disabling IMF_AUTOFONTSIZEADJUST	
	SendMessage(ed,EM_SETLANGOPTIONS,0,som_tool_richspell);	
	SendMessage(ed,EM_AUTOURLDETECT,1,0);
	//ensure off because the documentation is wrong
	//on: it annoying expands selection while being dragged
	//off: you can still double-click to select words (what it says it does)
	SendMessage(ed,EM_SETOPTIONS,ECOOP_AND,~1); //~ECO_AUTOWORDSELECTION);
	
	static CHARFORMAT2W cf;
	static bool one_off = false; if(!one_off++) //non-aggregate //???
	{
		cf.cbSize = sizeof(cf); 
		cf.dwReserved = 0; //so says the docs???
		cf.dwMask = CFM_LCID|CFM_CHARSET|CFM_FACE|CFM_SIZE|CFM_COLOR|CFM_ITALIC|CFM_WEIGHT;

		LOGFONTW lf; assert(som_tool_typeset);
		int lf_s = GetObjectW(som_tool_typeset,sizeof(lf),&lf); assert(lf_s);

		cf.dwEffects = CFE_AUTOCOLOR; if(lf.lfItalic) cf.dwEffects|=CFE_ITALIC;
		cf.wWeight = lf.lfWeight; 

		//96: GetDeviceCaps(LOGPIXELSY)				
		//twips: convert to points times 20
		//abs: sometimes positive sometimes not
		cf.yHeight = abs(lf.lfHeight*72*20/96);
		cf.bPitchAndFamily = lf.lfPitchAndFamily;	
		cf.bCharSet = lf.lfCharSet;

		wmemcpy(cf.szFaceName,lf.lfFaceName,LF_FACESIZE);

		//REMOVE ME?
		if(SOM::tool==SOM_MAIN.exe) //for toolbar
		{
			sprintf(som_main_richinit_pt,"%.2f",float(cf.yHeight)/20);
			char *_ = strchr(som_main_richinit_pt,'.');
			if(_&&!strcmp(_,".00")) *_ = '\0';
			if(_&&_[2]=='0') _[2] = '\0';
		}
	}	
	//does appear to register multiple defaults per charset
	int swap = cf.bCharSet;
	cf.lcid = ConvertDefaultLocale(LANG_JAPANESE);
	//covers Japanese style brackets and maybe kanji/kana too
	SendMessage(ed,EM_SETCHARFORMAT,SCF_ASSOCIATEFONT,(LPARAM)&cf);
	cf.bCharSet = ANSI_CHARSET;		
	//covers basic Latin range/space even all fonts have them
	cf.lcid = ConvertDefaultLocale(LANG_ENGLISH);
	SendMessage(ed,EM_SETCHARFORMAT,SCF_ASSOCIATEFONT,(LPARAM)&cf);
	cf.bCharSet = swap;		
	//not sure why this is necessary. It establishes the font
	//that is used when text is programmatically added to the
	//control. Whereas ASSOCIATEFONT seems to select the font
	//for typed in text. And what about synthesized charsets?
	SendMessage(ed,EM_SETCHARFORMAT,SCF_DEFAULT,(LPARAM)&cf);		

	Edit_SetModify(ed,0);
	SendMessage(ed,EM_SETEVENTMASK,0,em|ENM_LINK);
	return ed;
}
extern HWND som_tool_richcode(HWND ed)
{
	som_tool_richinit(ed,0);
	SendMessage(ed,EM_AUTOURLDETECT,0,0);
	SendMessage(ed,EM_SETLANGOPTIONS,0,IMF_UIFONTS|som_tool_richspell);
	assert(!Edit_GetModify(ed));
	return ed;
}
enum{ som_tool_richtext_margin=2 };
extern ATOM som_tool_richtext(HWND ed) 
{
	return_atom_if_not(ed)	
	//0: disable EN_CHANGE inside som_tool_richtext
	som_tool_richinit(ed,0); som_tool_textarea(ed); 

	//2: MAY BE DEPEND ON THE FONT USED
	//use poor edit control left margin
	//(note: this lines up better with the
	//older listbox/combobox controls however
	//later controls seem to prefer 1px margins)
	//2018: WHY NOT SET BOTH?
	//SendMessage(ed,EM_SETMARGINS,EC_LEFTMARGIN,2);	
	SendMessage(ed,EM_SETMARGINS,EC_LEFTMARGIN|EC_RIGHTMARGIN
	,MAKELPARAM(som_tool_richtext_margin,som_tool_richtext_margin));	
	//disabling IMF_AUTOFONTSIZEADJUST	
	//IMF_UIFONTS is needed to fit into SOM's boxes
	SendMessage(ed,EM_SETLANGOPTIONS,0,IMF_UIFONTS|som_tool_richspell);	
	//see som_tool_richedit code
	ShowScrollBar(ed,SB_BOTH,0);	
	SendMessage(ed,EM_SETTARGETDEVICE,0,0); //wrap

	LONG es = GetWindowStyle(ed);	
	
	if(es&ES_READONLY)
	SendMessage(ed,EM_SETBKGNDCOLOR,0,som_tool_readonly());
	Edit_SetModify(ed,0);
	//rr: handle classical text overflows
	int rr = es&ES_AUTOVSCROLL?0:ENM_REQUESTRESIZE;
	//SOM relies on EN_CHANGE notifications
	SendMessage(ed,EM_SETEVENTMASK,0,ENM_CHANGE|rr|ENM_LINK);
	return atom;
}
//used by SOM_MAIN.cpp to enable Rich Text boxes
extern void som_tool_extendtext(HWND ed, int em)
{
	//SEE som_tool_extendedtext BELOW
	SendMessage(ed,EM_SETLANGOPTIONS,0,
	SendMessage(ed,EM_GETLANGOPTIONS,0,0)&~IMF_UIFONTS);
	SendMessage(ed,EM_SETEVENTMASK,0,em|ENM_LINK);
	SendMessageW(ed,EM_SETWORDBREAKPROC,0,0);
}
static bool som_tool_extendedtext(HWND ed)
{
	return ~SendMessage(ed,EM_GETLANGOPTIONS,0,0)&IMF_UIFONTS;
}
extern ATOM som_tool_listview(HWND lv)
{
	return_atom_if_not(lv)
	SetWindowFont(lv,som_tool_typeset,0);
	SetWindowTheme(ListView_GetToolTips(lv),L"",L"");  
	int id = som_tool_subclass(lv,som_tool_listviewproc);

	if(id==1031&&SOM::tool==SOM_PRM.exe  //shop
	 ||id==1230&&SOM::tool==SOM_MAP.exe) //assuming counters
	{
		som_tool_boxen.clear(lv);
		if(id==1031) goto shop;
	}

	if(id==1015&&som_sys_tab==1018) shop: //inventory?
	{
		int ws = GetWindowStyle(lv);
		ws&=~LVS_SINGLESEL; ws|=LVS_SHOWSELALWAYS; 
		SetWindowLong(lv,GWL_STYLE,ws);	
	}

	return atom;
}
static ATOM som_tool_treeview(HWND tv=0)
{
	return_atom_if_not(tv)
	SetWindowFont(tv,som_tool_typeset,0);
	SetWindowTheme(TreeView_GetToolTips(tv),L"",L"");

	//REMOVE ME? 
	//(once ResEdit supports TVS_NOHSCROLL)
	int tvs = ~TVS_NOSCROLL&GetWindowStyle(tv);
	SetWindowLong(tv,GWL_STYLE,tvs|TVS_NOHSCROLL);	
	som_tool_subclass(tv,som_tool_treeviewproc);	


	//2018: Trying this Vista forward option
	if(0) TreeView_SetExtendedStyle(tv,4,4); //TVS_EX_DOUBLEBUFFER

	return atom;
}
static ATOM som_tool_trackbar(HWND tb)
{
	return_atom_if_not(tb)
							
	if(SOM::tool<PrtsEdit.exe) //HACK
	{
		long tbs = GetWindowStyle(tb);
		tbs&=~WS_TABSTOP; //assuming cannot be visualized
		SetWindowLong(tb,GWL_STYLE,TBS_FIXEDLENGTH|tbs);
		//15:12: yields buttons that are both 7 pixels across
		//but the vertical is 15x7 instead of 7x12 (which is nuts)
		//in fact it suggests this is a bug, setting the wrong dimension
		//in both cases, or it could just be a cosmically unlikely coincidence
		SendMessage(tb,TBM_SETTHUMBLENGTH,tbs&TBS_VERT?15:12,0);

		//NEW: let language pack center off dialog units
		RECT cr,ch; 
		SendMessage(tb,TBM_GETCHANNELRECT,0,(LPARAM)&ch);
		GetClientRect(tb,&cr);
		if(TBS_VERT&tbs) //TBM_GETCHANNELRECT is sideways
		cr.left+=cr.right/2-(ch.top+2);
		else //TBS_HORZ
		cr.top+=cr.bottom/2-(ch.top+2);
		MapWindowRect(tb,GetParent(tb),&cr);
		SetWindowPos(tb,0,cr.left,cr.top,0,0,SWP_NOSIZE|SWP_NOZORDER);
	}

	//add double-click to reset/recenter
	if(CS_DBLCLKS&~GetClassLong(tb,GCL_STYLE))
	SetClassLong(tb,GCL_STYLE,CS_DBLCLKS|GetClassLong(tb,GCL_STYLE));
	som_tool_subclass(tb,som_tool_trackbarproc);

	//probably shouldn't encourage this?
	HWND tt = (HWND)SendMessage(tb,TBM_GETTOOLTIPS,0,0);
	if(tt) SetWindowTheme(tt,L"",L"");
	//if(tt) SetWindowStyle(tt,TTS_ALWAYSTIP); //not working
	return atom;
}
extern ATOM som_tool_updown(HWND ud)
{	   
	return_atom_if_not(ud)
	//assuming cannot be visualized
	LONG uds = GetWindowStyle(ud);	
	//uds&=~UDS_ARROWKEYS; //ignored either way
	SetWindowLong(ud,GWL_STYLE,uds&~WS_TABSTOP);
	som_tool_subclass(ud,som_tool_updownproc);

	/*EXPERIMENTAL
	//HACK: putting border around up/down squashes the tickers
	//and makes the arrows offcenter
	if(int es=GetWindowExStyle(ud)&(WS_EX_CLIENTEDGE|WS_EX_STATICEDGE))
	{
		RECT rc; GetWindowRect(ud,&rc);
		rc.right-=rc.left; rc.bottom-=rc.top;		
		switch(es) //assuming classic window style
		{
		case WS_EX_CLIENTEDGE: es = 4; break;
		case WS_EX_STATICEDGE: es = 2; break; default: es = 6; //both
		}						
		(uds&UDS_HORZ?rc.bottom:rc.right)+=es;
		assert(~uds&UDS_ALIGNLEFT);
		SetWindowPos(ud,0,0,0,rc.right,rc.bottom,SWP_NOMOVE|SWP_NOZORDER);
	}*/

	return atom;
}
static ATOM som_tool_progress(HWND pb)
{
	return_atom_if_not(pb)
	SetWindowTheme(pb,L"",L"");
	SetWindowPos(pb,0,0,0,0,0,SWP_FRAMECHANGED|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER);	
	//ensure loading windows are not "unresponsive"
	som_tool_subclass(pb,som_tool_progressproc);
	return atom;
}
static DWORD som_tool_datetime(HWND dt=0)
{
	return_atom_if_not(dt)
	LONG dts = GetWindowStyle(dt);
	SetWindowLong(dt,GWL_STYLE,dts&~DTS_SHOWNONE|DTS_APPCANPARSE);
	//DateTime_SetMonthCalFont(dt,0,0);
	SetWindowFont(dt,som_tool_typeset,0);	
	SetWindowTheme(GetWindow(dt,GW_CHILD),L"",L""); 	
	//note, COLOR_HOTLIGHT is dark blue but it drowns out the selected text
	DateTime_SetMonthCalColor(dt,MCSC_TITLEBK,GetSysColor(COLOR_HIGHLIGHT));		
	//add double-click to select all so that
	//single clicking can select fields always
	if(CS_DBLCLKS&~GetClassLong(dt,GCL_STYLE))
	SetClassLong(dt,GCL_STYLE,CS_DBLCLKS|GetClassLong(dt,GCL_STYLE));
	som_tool_subclass(dt,som_tool_datetimeproc);
	return atom;
}
static ATOM som_tool_ipaddress(HWND ip=0)
{
	return_atom_if_not(ip)
	
	HWND ch = GetWindow(ip,GW_CHILD); 
	int id = som_tool_subclass(ip,som_tool_ipaddressproc);	
	for(int i=0;ch;/*ch=GetWindow(ch,GW_HWNDNEXT)*/) 	
	{
		HWND ch2 = GetWindow(ch,GW_HWNDNEXT);
		if(SOM::tool==ItemEdit.exe)
		{
			extern HWND workshop_ipchangeling(int,int,HWND,HWND);
			ch = workshop_ipchangeling(id,i++,ch,ip);
		}
		som_tool_subclass(ch,som_tool_iprangeproc); ch = ch2;
	}

	//winsanity: see DeleteObject comment
	SetWindowFont(ip,som_tool_typeset,0);	 
	SetWindowTheme(ip,L"",L"");

	if(0) //COMES OUT SAME SIZE
	///*works to optimize space, but limiting to 99 for now
	//(since the control isn't being used for IP addresses)
	if(SOM::tool==ItemEdit.exe)
	{
		RECT cr; GetClientRect(ip,&cr); int cx = cr.right/4-3;
		HWND ch = GetWindow(ip,GW_CHILD); GetClientRect(ch,&cr); 	 
		for(;ch;ch=GetWindow(ch,GW_HWNDNEXT)) 	
		SetWindowPos(ch,0,0,0,cx,cr.bottom,SWP_NOMOVE|SWP_NOZORDER);
	}//*/
	
	return atom;
}
#undef return_atom_if_not

//when in doubt, use a timer!
#define SOM_TOOL_TIMER(x,...) \
extern void CALLBACK x(HWND win,UINT,UINT kill,DWORD)\
{if(!KillTimer(win,kill))assert(0);else{__VA_ARGS__}}
SOM_TOOL_TIMER(som_map_showtileview,
{
	HWND tv = GetDlgItem(win,1052);
	SendMessage(tv,TVM_SELECTITEM,TVGN_CARET,0);
	SendMessage(tv,TVM_SELECTITEM,TVGN_CARET,(LPARAM)TreeView_GetChild(tv,TVI_ROOT));			
})
SOM_TOOL_TIMER(som_prm_enemywizard,
{
	HWND f = GetFocus(); SOM::xxiix ex;
	ex = GetWindowLong(GetDlgItem(som_tool,som_prm_tab),GWL_USERDATA);		
	SendMessage(win,WM_COMMAND,ex.ii==0?1101:1103+ex.ii,0);
	SetFocus(f); 
})
SOM_TOOL_TIMER(som_map_mousewheel,
{
	SendMessage(win,WM_VSCROLL,MAKEWPARAM(SB_THUMBPOSITION,GetScrollPos(win,1)),0); 
})
SOM_TOOL_TIMER(som_tool_shopcell, //EXPERIMENTAL
{
	if(GetFocus()!=win||GetKeyState(VK_CONTROL)>>15)
	return;
	int i = ListView_GetNextItem(win,-1,LVNI_FOCUSED);
	if(i<=0||i>=249)
	return;
	PostMessage(win,WM_KEYDOWN,kill,0);
	PostMessage(win,WM_KEYDOWN,VK_RETURN,0); SetTimer(win,kill,50,som_tool_shopcell);
})
SOM_TOOL_TIMER(som_tool_repair_tab_navigation_hack, //DESPERATION
{
	HWND gf = GetFocus();
	if(gf) InvalidateRect(gf,0,0);
	if(gf) SendMessage(gf,WM_UPDATEUISTATE,MAKEWPARAM(UIS_CLEAR,UISF_HIDEFOCUS),0);	
})

static int som_sys_slideshow = 0; 

static void som_map_Q() //unlock the axes
{
	int xy[] = {1028,1030,1031,1033};	
	windowsex_enable(som_map_tileviewinsert,xy);
}

/*2018: can't remember if this is helpful
//EXPERIMENTAL
//hack: patching system wide repaint bug
static bool som_tool_gettingmessage = false;
static HWND som_tool_gettingmessage_lock = 0;
static DWORD som_tool_gettingmessage_wait = 0;
*/

//SCHEDULED FOR REVAMPING
//2018: code is accessing som_tool_stack assuming that
//its order is reliable. this is convenient, but it was
//not designed with this in mind
//som_tool_NCDESTROY is for slower computers or maybe
//even older versions of Windows? (At least 7 if so)
//where IsWindow is unreliable within WM_INITDIALOG
extern void som_tool_stack_post_NCDESTROY(HWND hw) 
{
	int i,j;
	for(i=j=0;som_tool_stack[i];i++)	
	if(hw!=(som_tool_stack[j]=som_tool_stack[i])) 
	j++;
	som_tool_stack[j] = 0;
}
extern HWND som_tool_dialog(HWND set)
{	 
	int i,j;
	for(i=j=0;som_tool_stack[i];i++)
	{
		if(IsWindow(som_tool_stack[j]=som_tool_stack[i])) 
		j++;
		else som_tool_stack[i] = 0;
	}
	assert(j<8); //which tool went beyond 8? 16?!
	som_tool_stack[j] = 0; assert(j<som_tool_stack_s);
	if(!set)
	return j==0?0:som_tool_stack[j-1];
	
	if(!IsWindow(set)){ set = 0; assert(0); }

	if(j==0||set!=som_tool_stack[j-1])
	som_tool_stack[j] = set; return set;	
}  

extern BYTE SOM_MAP_z_index;
static bool som_tool_ok(HWND);
static HWND som_main_menu = 0;
static HWND som_tool_refocus= 0;
static long som_tool_slideZ = 0;
static bool som_tool_slide(HWND);
static void som_tool_increment(float,HWND);
static void som_tool_togglemnemonics(HWND);
extern int som_tool_erasebkgnd(HWND,HDC);
extern int som_tool_drawitem(DRAWITEMSTRUCT*,int,int=-1);
extern windowsex_DIALOGPROC SOM_MAIN_151,SOM_EDIT_151;
extern void som_tool_toggle(HWND,HTREEITEM,POINT,int=TVE_TOGGLE);
extern INT_PTR CALLBACK som_tool_PseudoModalDialog(HWND,UINT,WPARAM,LPARAM){ return 0; }
static LRESULT CALLBACK som_tool_subclassproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scID, DWORD_PTR hlp)
{
	//THIS SUBROUTINE IS TOO LARGE. NO TELLING HOW LARGE ITS STACK IS. WIN32 CALLS 
	//WINDOW PROCEDURES IN A HIGHLY RECUSRIVE WAY. 
	//THIS SUBROUTINE IS TOO LARGE. NO TELLING HOW LARGE ITS STACK IS. WIN32 CALLS 
	//WINDOW PROCEDURES IN A HIGHLY RECUSRIVE WAY. 
	//THIS SUBROUTINE IS TOO LARGE. NO TELLING HOW LARGE ITS STACK IS. WIN32 CALLS 
	//WINDOW PROCEDURES IN A HIGHLY RECUSRIVE WAY. 
	//THIS SUBROUTINE IS TOO LARGE. NO TELLING HOW LARGE ITS STACK IS. WIN32 CALLS 
	//WINDOW PROCEDURES IN A HIGHLY RECUSRIVE WAY. 
	//THIS SUBROUTINE IS TOO LARGE. NO TELLING HOW LARGE ITS STACK IS. WIN32 CALLS 
	//WINDOW PROCEDURES IN A HIGHLY RECUSRIVE WAY. 
	//THIS SUBROUTINE IS TOO LARGE. NO TELLING HOW LARGE ITS STACK IS. WIN32 CALLS 
	//WINDOW PROCEDURES IN A HIGHLY RECUSRIVE WAY. 

	switch(uMsg)
	{	
	case WM_APP+'ws':

		//NOTE: workshop_moves_32767 spawns more tool windows
		if(!som_tool_workshop)
		{
			som_tool_workshop = (HWND)wParam;
		}
		//2023: make modeless? see also workshop_exe
		EnableWindow(som_tool,1);	
	//	SetParent((HWND)wParam,som_tool);
		SetWindowLong((HWND)wParam,GWL_HWNDPARENT,(LONG)som_tool);
	
		break;

	case WM_ENABLE:

		if(wParam&&som_tool_workshop)
		if(GetWindowLong(som_tool_workshop,GWL_USERDATA))
		{
			SetWindowLong(som_tool_workshop,GWL_USERDATA,0);
			assert(hWnd==som_tool);
			if(som_tool_stack[1]) //HACK: update PR2 file?
			windowsex_notify(som_tool_stack[1],1002,CBN_SELENDOK);
		}
		break;

	case WM_PAINT: //SOM_PRM 
		 
		/*see som_tool_gettingmessage declaration 
		//EXPERIMENTAL
		//hack: patching system wide repaint bug
		if(som_tool_gettingmessage&&GetActiveWindow()==hWnd)
		{
			//LockWindowUpdate(som_tool_gettingmessage_lock=hWnd);
			SetWindowRedraw(som_tool_gettingmessage_lock=hWnd,0);
			som_tool_gettingmessage_wait = EX::tick();
		}*/
		if(!som_tool_classic) //see WM_ERASEBKGND fix
		{
			if(som_tool_background(hlp)) switch(hlp) 
			{
			//NOTICE! NOT DefSubclassProc
			default: return DefWindowProc(hWnd,uMsg,wParam,lParam);

			case 35400: case 36100: if(!som_prm_graphing) //crazy
			{
				//2018: leaving to SOM_PRM_reprogram
				//const RECT npc = {640,137,640+270,137+250};
				//const RECT mon = {180,30,180+270,30+250}; //from 309,165
				//const RECT &gr = hlp==35400?mon:npc; ValidateRect(hWnd,&gr);
				PAINTSTRUCT ps;	BeginPaint(hWnd,&ps); EndPaint(hWnd,&ps); 
				som_prm_graphing = true; RECT rc = ps.rcPaint;				
				ExcludeClipRect(ps.hdc,rc.left,rc.top,rc.right,rc.bottom); 
				//IntersectClipRect(ps.hdc,gr.left,gr.top,gr.right,gr.bottom);
				RedrawWindow(hWnd,0,0,RDW_NOERASE|RDW_INTERNALPAINT|RDW_UPDATENOW);
				SelectClipRgn(ps.hdc,0);
				som_prm_graphing = false; return 0; //!!
			}}			
		}
		break;

	case WM_ERASEBKGND:
	{
		if(som_prm_graphing) return 1;
		if(som_tool_erasebkgnd(hWnd,(HDC)wParam)) return 1;
		break;	
	}
	case WM_DRAWITEM: //buttons
	{
		if(som_tool_classic) break; //NEW

		DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT*)lParam;

		if(!hlp) break;		
		
		switch(dis->CtlType)
		{
		case ODT_STATIC: 
			
			if((LPARAM&)wParam<=0) goto def; //decor?

			if(wParam==1232) //Q
			if(hWnd==som_map_tileviewinsert) return 1; 
			if(wParam==1235)
			if(SOM::tool==SOM_PRM.exe) return 1; //3D viewer

			if(hlp==12500) goto def; //SOM_MAIN hue wells/slider

			//break;

		default: assert(0); goto def;			
		
		case ODT_BUTTON:; //...
		}
						
		if(som_tool_DrawTextA_item) return 1; //!!

		som_tool_DrawTextA_item = dis;

		HWND bt = dis->hwndItem;
		//experimental: trouble with XP manifest
		DWORD st = SendMessage(bt,BM_GETSTATE,0,0);	
		if(st&BST_PUSHED) //make comctl32.dll 6.0 conform to...
		{
			dis->itemAction = ODA_SELECT;
			dis->itemState|=ODS_SELECTED;
			dis->itemState|=ODS_FOCUS;
		}
		else if(st&BST_FOCUS) //Sword of Moonlight's expectations
		{		
			dis->itemAction = ODA_FOCUS;
			dis->itemState|=ODS_FOCUS;
		}

		LRESULT out = 0;

		switch(hlp) //extensions
		{
		case 10000: //HALFTONE issues

			if(wParam>=1000&&wParam<=1003)
			{
				int map[6] = {3,1,2,0};
				out = som_tool_drawitem(dis,152+map[wParam-1000]); 
			}
			break;

		case 20000: //HALFTONE issues

			if(wParam>=1000&&wParam<=1005)
			{
				int map[6] = {3,1,2,0,4,15};				
				out = som_tool_drawitem(dis,132+map[wParam-1000]); 
			}
			break;

		case 30000: //SOM_PRM

			switch(wParam)
			{
			case 1001: case 1004: case 1003: case 1007: case 1005: case 1006: 

				out = som_tool_drawitem(dis,132,wParam==som_prm_tab); 
				break;
			
			case 2000: //script button

				if(1005==som_prm_tab)
				if(35200!=GetWindowContextHelpId(som_tool_dialog()))
				out = som_tool_drawitem(dis,132,1); 
				if(out) break;

			default: out = som_tool_drawitem(dis,132);
			}
			break;

		case 35100: //enemy wizard

			switch(wParam) //KF2 multi body monster
			{
			case 1003: case 1004: case 1106: 
				
				//PRELIMINARY
				out = som_tool_drawitem(dis,135,wParam==1003); break;
			}
			break;
			
		case 40000: //SOM_MAP

			if(wParam==1080 //Alt Play button?
			||wParam==3000) //Make overlay button push down?
			out = som_tool_drawitem(dis,175);
			if(wParam==1263 //checkpoint overlay
			||wParam==1264) //arrows overlay
			{
				st = dis->hwndItem==GetFocus();
				int mask = WS_EX_CLIENTEDGE|WS_EX_DLGMODALFRAME;
				SetWindowExStyle(dis->hwndItem,mask,st?WS_EX_CLIENTEDGE:WS_EX_DLGMODALFRAME);
			//	RedrawWindow(dis->hwndItem,0,0,RDW_FRAME|RDW_INVALIDATE);
				out = som_tool_drawitem(dis,175,st);
			}
			break;	 

		case 41000:

			if(wParam>=1071&&wParam<=1074)
			out = som_tool_drawitem(dis,175); 
			break;

		case 42000: //tile editor

			if(wParam>1257&&wParam<=1264)
			{
				int st = som_map_tileviewmask&1<<wParam-1257;
				out = som_tool_drawitem(dis,175,st); 
			}
			break;

		case 42100:
		case 42200: case 42300: //deprecated

			if(wParam==1037||wParam==1038) 
			out = som_tool_drawitem(dis,175); //+/-
			break;

		case 45100: //event

			if(wParam==1211) //open
			out = som_tool_drawitem(dis,175);
			break;

		case 45300:

			if(wParam==1012) //comment
			out = som_tool_drawitem(dis,175);
			break;

		case 48100: //generic? (Standby Map)

			out = som_tool_drawitem(dis,175);
			break;

		case 49100: //prepare step

			if(wParam!=som_tool_pushlikepseudoid) //HACK: All?
			out = som_tool_drawitem(dis,175);
			break;

		case 50000: //these are identical, so draw them too

			switch(wParam)
			{
			case 1022: case 1018: case 1017: case 1023: case 1019: case 1020: case 1021:

				out = som_tool_drawitem(dis,130,wParam==som_sys_tab); break; 

			case 2000: //script button

				switch(som_sys_tab)
				{
				case 1017: case 1019: break; //classes/messages

				default: out = som_tool_drawitem(dis,130,1);
				}
				if(out) break; 

			default: out = som_tool_drawitem(dis,130);
			}
			break;

		case 56200: case 57100: //play/stop buttons

			switch(wParam)
			{
			case 1043: case 1044:

				//5: CRLF/surrogate pairs
				wchar_t test[5]; if(GetWindowTextW(bt,test,5))
				{
					out = som_tool_drawitem(dis,131);
				}
				break;

			case 1080: //Alt Play button?
			
				out = som_tool_drawitem(dis,132);
				break;
			}
			break;
		}
		if(!out)
		{
			if(som_tool_pushlikepseudoid==wParam) 
			{							
				int bm = 0; switch(SOM::tool)
				{
				case SOM_PRM.exe: bm = 135; break;
				case SOM_MAP.exe: bm = 175; break;
				case SOM_MAIN.exe: bm = 156; break; default: assert(0);
				}
				out = som_tool_drawitem(dis,bm,st&BST_CHECKED); 
			}
			else if(wParam>=2000&&wParam<=2003)
			{
				switch(SOM::tool) //som_tool_openscript/folder
				{
				case SOM_PRM.exe: out = som_tool_drawitem(dis,135); break;
				case SOM_MAP.exe: out = som_tool_drawitem(dis,175); break;
				case SOM_SYS.exe: out = som_tool_drawitem(dis,131); break;
				}
			}
		}
		if(!out)
		{			
			//hack: play/stop buttons
			som_tool_DrawTextA_item_text = false;

			out = DefSubclassProc(hWnd,uMsg,wParam,lParam);
		
			if(!som_tool_DrawTextA_item_text)
			if(~dis->itemState&ODS_NOFOCUSRECT)
			if(~dis->itemAction&ODA_SELECT&&dis->itemState&ODS_FOCUS)
			{
				InflateRect(&dis->rcItem,-2,-2);
				FrameRect(dis->hDC,&dis->rcItem,(HBRUSH)GetStockObject(BLACK_BRUSH));
				InflateRect(&dis->rcItem,-1,-1);
				FrameRect(dis->hDC,&dis->rcItem,(HBRUSH)GetStockObject(BLACK_BRUSH));
			}
		}
		som_tool_DrawTextA_item = 0;
		 		 
		return out;
	} 
	case WM_CHANGEUISTATE: //workaround WM_PAINT bypassing
	{
		//this is constantly firing...
		//and not refreshing with keyboard input...
		//just going to use som_tool_GetMessageA_mnemonics for Alt
		return 1; MessageBeep(-1);
		/*extern int som_tool_initializing;		
		if(som_tool_classic||som_tool_initializing) break;		
		int changed = SendMessage(hWnd,WM_QUERYUISTATE,0,0);
		SetWindowRedraw(hWnd,0); DefSubclassProc(hWnd,uMsg,wParam,lParam);		
		SetWindowRedraw(hWnd,1);		
		changed^=SendMessage(hWnd,WM_QUERYUISTATE,0,0);
		if(changed&UISF_HIDEACCEL)
		som_tool_togglemnemonics(hWnd); //recursive
		if(changed&UISF_HIDEFOCUS)
		if(GetFocus()) InvalidateRect(GetFocus(),0,0); //paranoia?
		return 1;*/
	}					
	case WM_CTLCOLORSTATIC: //added by language pack
	{
		//note: this is the text background
		SetBkMode((HDC)wParam,TRANSPARENT);

		int dlgc = SendMessage((HWND)lParam,WM_GETDLGCODE,0,0);

		//hack: checkboxes and editboxes tagged with WS_EX_TRANSPARENT
		if(WS_EX_TRANSPARENT&GetWindowExStyle((HWND)lParam)) 
		{	
			//hack: the WS_EX_TRANSPARENT documentation is a lie
			//In truth Transparent controls are painted as a last step
			//By marking tab stops they can be put back in the true Z-order
			//REMINDER: THINKING THIS WAS FOR SOM_PRM'S ENEMY ATTACKS MENUS
			if((DLGC_BUTTON|DLGC_HASSETSEL|DLGC_STATIC)&dlgc)
			{
				//this is to support transparent edit
				//boxes with comctl32.dll 6.0 which paints
				//the background inside WM_PAINT with junk that 
				//looks like other edit controls, whereas before 6.0
				//just the text/selection highlighting would get painted
				//NEW: using pattern brush technique to fix classic/basic modes				
				if(DLGC_HASSETSEL&dlgc) //readonly edit control
				if(GetFocus()==(HWND)lParam) SetTextColor((HDC)wParam,0x333333);								
				LRESULT bg = (LRESULT)som_tool_erasebrush((HWND)lParam,(HDC)wParam);
				return bg?bg:(LRESULT)GetStockObject(HOLLOW_BRUSH);
			}
		}	

		//make comctl32.dll 6.0 conform
		if(DLGC_HASSETSEL&dlgc&&ES_READONLY&GetWindowStyle((HWND)lParam))		
		{
			combobox1001:
			return (LRESULT)som_tool_graybrush(); 		
		}

		if(~dlgc&DLGC_STATIC) 
		{
			if(hlp==40000) goto combobox1001;

			//winsanity: looks much better this way
			//See som_tool_whitebrush for bug details
			//XP: return always for readonly drop boxes
			//if(WS_DISABLED&GetWindowStyle((HWND)lParam)) 		
			return (LRESULT)som_tool_whitebrush(); 		
		}
		break;	
	}			
	case WM_NCHITTEST: //zaniness (ancient)
						 
		if(!som_tool_classic)
		//fix issues around clicking sub-dialogs
		//Note: SOM is changing this behavior because 
		//it used hovering POPUP windows instead of CHILD
		if(WS_CHILD&GetWindowStyle(hWnd))
		return HTCLIENT;
		break;
	
	case WM_NCLBUTTONDOWN: //iconify
	{
		POINT pt; RECT cr; 
		POINTSTOPOINT(pt,(POINTS&)lParam);			
		if(!som_tool_classic)
		if(hWnd==som_tool||som_sys_slideshow)		
		if(GetClientRect(hWnd,&cr)&&MapWindowRect(hWnd,0,&cr)
		  &&PtInRect(&cr,pt)&&WS_CAPTION&GetWindowStyle(hWnd))
		{
			//twart this trick that old SOM_PRM/SYS did in lieu of frames
			SetActiveWindow(hWnd); return 0; 
		}

		if(som_tool_taskbar&&wParam==HTCAPTION)
		{	
			//2018: seems to work to minimize from a popup window
			//if(hWnd!=som_tool) break;
				
			//TODO: SOM::Tool::dragdetect SEEMS TO DO AN ADEQUATE
			//JOB OF THIS IN workshop.cpp
			if(DragDetect(hWnd,pt)) break;

			//Reminder: this used to be GetForeground before DragDetect,
			//but the AfxWnd42s windows make SOM_MAP unreliable in that
			//regard
			//Note, in addition som_map_AfxWnd42sproc must watch for a
			//WM_KILLFOCUS message and hide it if SOM_MAP is not active
			if(hWnd!=GetAncestor(GetFocus(),GA_ROOT))
			{
				//NCLBUTTONDOWN is being echoed back??
				SetForegroundWindow(som_tool); 
			}
			else 
			{
				//isn't replacing foreground???
				//CloseWindow(som_tool_taskbar);			
				SendMessage(som_tool_taskbar,WM_SYSCOMMAND,SC_MINIMIZE,0);
			}
			return 0; 
		}
		break;
	}
	case WM_SETFOCUS:

		switch(hlp) //SOM_MAIN
		{	
		case 10000: case 10200: //compact?
		
			//sent by comctl32.dll???
			if(hWnd==som_tool) break;
			SetFocus(GetNextDlgTabItem(som_tool,0,0));	
			return 0;			
		}
		break;

	case WM_WINDOWPOSCHANGING: //added by language pack
	{	
		WINDOWPOS *p = (WINDOWPOS*)lParam;

		//if(!EX::debug)
		//hack: Image/Video/Music instructions
		if(som_tool_find) p->flags|=SWP_NOSIZE;

		//TODO: force these to be children
		switch(hlp)
		{	
		case 10100: //new project

			if(!som_tool_classic)
			p->flags|=SWP_NOSIZE; //use resource
			break;

		case 10000: //do_enlarge

			p->flags|=SWP_NOSIZE; //WTH?
			//break;		
		case 10200: //compact?
		{
			if(hWnd!=som_tool)
			if(p->flags&SWP_SHOWWINDOW)			
			if(GetParent(hWnd)==som_tool_taskbar)
			p->flags&=~SWP_SHOWWINDOW;
			break;
		}		
		case 31100: case 32100: case 33100:	
		case 34100:	case 35100:	case 36100:

			//must be setup by language pack
			if(WS_CHILD&GetWindowStyle(hWnd))
			{				
				HWND tab =  //NPC
				GetDlgItem(GetParent(hWnd),1006);
				
				RECT cr; GetClientRect(tab,&cr);
			
				//+1: necessary to keep from drawing over the tabs
				//it isn't clear why that happens, by all rights it
				//shouldn't. Som2k does this too. It's possible that
				//it copies the top row of pixels from the background
				p->x = 0; p->y = cr.bottom+1;		

				p->hwndInsertAfter = tab;
			}
			else //not setup by language pack
			{							
				HWND tab =  //ITEMS
				//GetDlgItem(GetParent(hWnd),1001);
				GetWindow(GetParent(hWnd),GW_CHILD);
				
				RECT wr; GetWindowRect(tab,&wr);

				p->x = wr.left; p->y = wr.bottom;		
			}

			//unsure what SOM is smoking here
			//p->flags|=SWP_NOSIZE;
			p->cx = 990*som_tool_enlarge2+0.5f; //BMP dimens
			p->cy = 667*som_tool_enlarge+0.5f; 
			p->flags&=~(SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER);
			break;
		
		case 35200: case 35300: case 35400:

			//must be setup by language pack
			if(WS_CHILD&GetWindowStyle(hWnd))
			{
				//last wizard button
				p->hwndInsertAfter = 
				GetDlgItem(GetParent(hWnd),1105);
				
				p->x = 309*som_tool_enlarge2+0.5f; 
				p->y = 165*som_tool_enlarge+0.5f;
				p->cx = 631*som_tool_enlarge2+0.5f; 
				p->cy = 443*som_tool_enlarge+0.5f;
				p->flags&=~(SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER);
			}
			break;

		//these should be children already
		case 42100: 
		case 42200: case 42300: //deprecated

			if(WS_CHILD&GetWindowStyle(hWnd))
			{
				//tree view control
				p->hwndInsertAfter = 
				GetDlgItem(GetParent(hWnd),/*1052*/1147);
				p->flags&=~SWP_NOZORDER;
			}
			else assert(0);
			break;

		case 51100: case 52100: case 53100:	
		case 54100:	case 55100:	case 56100: case 57100:

			//must be setup by language pack
			if(WS_CHILD&GetWindowStyle(hWnd))
			{				
				HWND tab =  //etc. (last)
				GetDlgItem(GetParent(hWnd),1021);
				
				RECT cr; GetClientRect(tab,&cr);
			
				//+1: same as SOM_PRM, see its notes above
				p->x = 0; p->y = cr.bottom+1;		

				p->hwndInsertAfter = tab;
			}
			else //not setup by language pack
			{							
				HWND tab =  //first
				GetWindow(GetParent(hWnd),GW_CHILD);
				
				RECT wr; GetWindowRect(tab,&wr);

				p->x = wr.left; p->y = wr.bottom;		
			}

			//unsure what SOM is smoking here
			//p->flags|=SWP_NOSIZE;
			p->cx = 768*som_tool_enlarge2+0.5f; //BMP dimens 
			p->cy = 532*som_tool_enlarge+0.5f;
			p->flags&=~(SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER);
			break;

		case 20000: case 56200: //do_enlarge
			
			p->flags|=SWP_NOSIZE; //WTH?
			break;
		}
		break;		
	}
	case WM_SYSCOMMAND:
	
		//prevent Alt key from locking navigation
		//except for the system menu (space) which
		//unfortunately escapes to the non-existent
		//menu bar. But why open just then to escape?
		if(wParam==SC_KEYMENU&&lParam!=' ') return 0;

		switch(wParam)
		{	
		//case SC_RESTORE:; //assert(0);
		case SC_MAXIMIZE: case SC_MINIMIZE:
			
			if(SOM::tool==SOM_MAP.exe) //2023
			{
				SetWindowStyle(hWnd,WS_MINIMIZEBOX|WS_MAXIMIZEBOX,
				wParam==SC_MAXIMIZE?WS_MINIMIZEBOX:WS_MAXIMIZEBOX);
				extern void SOM_MAP_minmax(); SOM_MAP_minmax();
				return 0;
			}
			break;
		}
		break;

	case WM_COMMAND:  
	{
		if(IDOK==wParam)
		{
			//Reminder: IDOK can indicate the
			//Enter key is pressed inside edit
			//controls.			
			if(!som_tool_ok(hWnd)) return 0;
		}
				
		//paranoia (som_tool_pushed)
		HWND &lp = (HWND&)lParam; if(!lp) 
		{
			lp = GetDlgItem(hWnd,LOWORD(wParam));
			if(lp&&!IsWindow(lp)) lp = 0;
		}
		int dlgc = SendMessage(lp,WM_GETDLGCODE,0,0);

		switch(HIWORD(wParam))
		{
		//rationale: SELCHANGE is spammed each
		//time the keyboard highlights an item
		case CBN_SELENDOK: case C8N_SELCHANGE:
			
			//REMINDER: LBN_SELCHANGE==CBN_SELCHANGE
			//prevent SELCHANGE when not accompanied by ENDOK
			//note: SOM isn't generally concerned about ENDOK
			static bool selchanging = false; if(!selchanging)
			if(som_tool_combobox()==GetClassAtom((HWND)lParam)) //
			{
				//may be a good idea to do this
				//if the tools ever simulate CBN_SELCHANGE				
				if(C8N_SELCHANGE==HIWORD(wParam))
				{
					//if(IsWindowVisible(hWnd) 
					//&&1==SendMessage((HWND)lParam,CB_GETDROPPEDSTATE,0,0))
					return 0;				
				}
				selchanging = true;
				som_tool_subclassproc(hWnd,uMsg,wParam,lParam,scID,hlp);
				SendMessage(hWnd,uMsg,MAKEWPARAM(wParam,C8N_SELCHANGE),lParam);
				selchanging = false;
				return 0;
			}
			break;
		}	
		
		if(HIWORD(wParam)==LBN_DBLCLK) //2
		{
			som_tool_pushed = lp; //som_tool_id

			//2018: open 3D model view
			if(SOM::tool==SOM_PRM.exe) 
			PostMessage(som_tool_stack[1],WM_COMMAND,1005,0);
		}
		else if(DLGC_BUTTON&dlgc) 
		{
			som_tool_pushed = lp; //assert(!HIWORD(wParam));

			if(HIWORD(wParam)==BN_CLICKED) //0
			{
				if(hWnd==som_map_tileview)
				{
					int i = wParam-1257;					
					if(i>=0&&i<=7&&IsWindowVisible(som_map_tileview)) 
					{
						som_map_tileviewmask^=1<<i;

						if(i==6||i==7) //2021: InvalidateRect?
						{
							//6/7: fill mode needs edge mode on
							if(i==6&&~som_map_tileviewmask&0x40)
							som_map_tileviewmask&=~0x80, //!
							InvalidateRect(GetDlgItem(som_map_tileview,1264),0,0);
							if(i==7&&~som_map_tileviewmask&0x40)
							som_map_tileviewmask|=0x40, //!!
							InvalidateRect(GetDlgItem(som_map_tileview,1263),0,0);
						}

						if(i) som_map_tileviewpaint();
					}
				}
			}

			if(wParam>=2000&&wParam<=2003)
			{
				if(wParam==2000) som_tool_openscript(hWnd,lp);
				if(wParam>=2001) som_tool_openfolder(hWnd,wParam);
			}
		}

		switch(HIWORD(wParam))
		{		
		case CBN_DROPDOWN: //7
		{
			HWND &cb = lp;
			int cbs = GetWindowStyle(cb);
			//optimization: marked resized
			//(unset by som_tool_combobox)
			if(CBS_AUTOHSCROLL&cbs) break;
			//SEE som_tool_combobox FOR WHY THIS IS A BAD IDEA
			SetWindowLong(cb,GWL_STYLE,cbs|=CBS_AUTOHSCROLL);						
			LONG width = 0; SIZE extent;
			HDC dc = GetDC(cb); HGDIOBJ dc_font = 
			SelectObject(dc,(HGDIOBJ)SendMessage(cb,WM_GETFONT,0,0));
			int i, n;
			for(i=0,n=SendMessage(cb,CB_GETCOUNT,0,0);i<n;i++)
			{
				auto &text = som_tool_wector; 
				text.resize(SendMessageW(cb,CB_GETLBTEXTLEN,i,0)+1);
				if(CB_ERR!=SendMessageW(cb,CB_GETLBTEXT,i,(LPARAM)&text[0]))
				if(GetTextExtentPoint32W(dc,&text[0],text.size(),&extent))
				width = max(width,extent.cx);
			}
			if(width)
			{
				width+=GetSystemMetrics(SM_CXEDGE)*2;
				width+=GetSystemMetrics(SM_CXHSCROLL);
				SendMessage(cb,CB_SETDROPPEDWIDTH,width,0);
			}
			SelectObject(dc,dc_font);
			ReleaseDC(cb,dc);

			//hack: resize scroll-less menus
			COMBOBOXINFO cbi = {sizeof(cbi)};
			if(GetComboBoxInfo(cb,&cbi)&&n!=CB_ERR			 
			&&WS_VSCROLL&~GetWindowStyle(cbi.hwndList))
			{
				int h = cbi.rcItem.bottom-cbi.rcItem.top; 
				//h+h*n: relying on Windows to not display excess height					
				SetWindowPos(cbi.hwndList,0,0,0,width,h+h*n,SWP_NOMOVE); 
			}			
			break;
		}
		case LBN_SELCHANGE:

			switch(MAKEWPARAM(hlp,LOWORD(wParam)))
			{					
			case MAKEWPARAM(31100,1030):
			case MAKEWPARAM(34100,1219): 
			case MAKEWPARAM(35100,1220): 
			case MAKEWPARAM(36100,1023): 
				
				//HACK: SOM_PRM_extend(1013) is fixing an age old 
				//bug with the "duration" field of healing/shield
				//magics failing to register
				case MAKEWPARAM(33100,1034):

				//HACK: record, change, reload
				//HACK: if the item is unnamed, delete, else record
				if(ListBox_GetTextLen(lp,SOM_PRM_47a708.item())>6)
				SOM_PRM_extend(1013);
				//break;				
				//NEW: Remember the selections...				
				case MAKEWPARAM(32100,1029):
			//	case MAKEWPARAM(33100,1034):

				DefSubclassProc(hWnd,uMsg,wParam,lParam);

				//NEW: remember the selections
				SOM::xxiix ex;
				ex = GetWindowLong(GetDlgItem(som_tool,som_prm_tab),GWL_USERDATA);
				ex.x1 = LOWORD(wParam&0xFFFF)-1000&1023;
				ex.x2 = ListBox_GetTopIndex(lp)&1023;
				ex.x3 = ListBox_GetCurSel(lp)&1023;				
				if(hlp==35100)
				{
					//REMOVE ME? (see item() note)
					SOM_PRM_47a708.item() = ex.x3; 			
				}
				SetWindowLong(GetDlgItem(som_tool,som_prm_tab),GWL_USERDATA,ex);
			
				SOM_PRM_extend(); return 0;

			case MAKELONG(41000,1239): //maps->SOM_MAP_165?
			{
				short ch[] = {1,1213,1046,1047,1071,1075};
				int sel[2] = {ListBox_GetSelCount(lp)};
				if(1!=sel[0])
				{
					//LBN won't fire while mouse is captured
					DefSubclassProc(hWnd,uMsg,wParam,lParam);
					windowsex_enable(hWnd,ch,0);
					if(2==sel[0]) //compose?
					{
						//WARNING: assuming higher map number
						//is to be the extended layer
						ListBox_GetSelItems(lp,2,sel);
						if(0xFF!=ListBox_GetItemData(lp,sel[0]))
						{
							//lp = 0; goto compose;							
							//6: must be base map...
							int i = som_LYT_base(sel[0]); 
							i = i==-1?0:6; 
							//must not be base map...
							if(SOM_MAP.lyt[sel[1]])
							if(SOM_MAP.lyt[sel[1]][0]->data().legacy)
							i = 6;
							if(SOM_MAP.lyt[sel[0]])
							for(;i<6;i++) if(!SOM_MAP.lyt[sel[0]][i])
							break;
							if(i<6) //limiting to SOM::MPX::layer_6
							{
								lp = 0; goto compose;
							}
						}
						CheckDlgButton(hWnd,1075,0);
					}
					return 0;
				}
				else 
				{
					sel[1] = ListBox_GetCurSel(lp); 
					windowsex_enable<1>(hWnd); 
					windowsex_enable<1071>(hWnd,
					0xFF!=ListBox_GetItemData(lp,sel));

					//simplified layer interface
				    compose: int i,j;
					for(i=j=0;i<64;i++) if(SOM_MAP.lyt[i])
					if(1ULL<<sel[1]&SOM_MAP.lyt[i][0]->data().legacy)
					j++;						
					windowsex_enable<1075>(hWnd,j==(lp?1:0));
					CheckDlgButton(hWnd,1075,j==1);
					if(!lp) return 0; //compose?
				}
				break;
			}}
			break;
		}
		switch(SOM::tool)
		{
		case SOM_MAIN.exe: 
		{
			if(hWnd==som_tool) if(1002==wParam) //same as below
			{
				HRSRC found = FindResource(0,MAKEINTRESOURCE(151),RT_DIALOG);
				if(DLGTEMPLATE*locked=(DLGTEMPLATE*)LockResource(LoadResource(0,found)))
				{
					som_tool_CreateDialogIndirectParamA(0,locked,0,SOM_MAIN_151,0);						
					return 0;			
				}
				else if(hWnd==som_main_menu) //NEW
				{
					Sompaste->set(L".SOM",L""); //oldstyle SOM_RUN
				}
			}				 
			if(wParam<=IDCANCEL) switch(hlp)
			{
			case 10100: //New Project
				
				return SendMessage(hWnd,WM_COMMAND,wParam&1?1016:1006,0);
			}

		}break;		
		case SOM_EDIT.exe:
		{
			if(hWnd==som_tool) if(1000==wParam) //same as above
			{
				HRSRC found = FindResource(0,MAKEINTRESOURCE(151),RT_DIALOG);
				DLGTEMPLATE *locked = (DLGTEMPLATE*)LockResource(LoadResource(0,found)); 
				if(!locked) break; //we do all this to pass thru out hook 
				som_tool_CreateDialogIndirectParamA(0,locked,hWnd,SOM_EDIT_151,0);						
				return 0;			
			}
			else if(1004==wParam) //NEW: closing to SOM_MAIN menu?
			{
				Sompaste->set(L".MO",L""); //ensure the menu appears
			}

		}break;
		case SOM_PRM.exe:
		{			
			switch(hlp)
			{
			case 30000: //SOM_PRM

				switch(wParam) 
				{				
				//this is pretty much everything. it might
				//make more sense to record with EN_CHANGE
				//(Fighting might be an issue?)
				case 1016: //save
				case 1000: //close 

					/*SOM_PRM wants to record it
					//HACK: avoid unrecorded item?
					if(ListBox_GetTextLen((HWND)lParam,SOM_PRM_47a708.item())>6)
					break;*/
				
				case 1012: //copy
				case 1013: //record
				case 1015: //cut				
				case 1001: case 1004: case 1003: //tabs
				case 1007: case 1005: case 1006: //tabs

					SOM_PRM_extend(1013); break;

					if(1015!=wParam) break;
					//break;
				case 1011: //paste
				case 1014: //delete				
					profile_enable:
					DefSubclassProc(hWnd,uMsg,wParam,lParam);
					SOM_PRM_extend();
					return 0;
					
				case 2021: //2001 //som_tool_openfolder?
							
					extern void som_art_2021(HWND); som_art_2021(hWnd);
					return 0;
				}
				break;
				
			case 35100: //enemies

				switch(wParam)
				{
				case 1005: goto workshop; 

				case 1101: case 1104: case 1105: //wizard
				{
					//FIX: SOM_PRM forgets this when the wizard changes
					int save = SOM_PRM_47a708.save();
					//There are two save flags in fact. Which might be why the bug.
					save|=1&*(BYTE*)0x47F838;

					SOM::xxiix ex; 
					ex = GetWindowLong(GetDlgItem(som_tool,som_prm_tab),GWL_USERDATA);
					ex.ii = wParam==1101?0:wParam-3;
					SetWindowLong(GetDlgItem(som_tool,som_prm_tab),GWL_USERDATA,ex);

					SOM_PRM_extend(1013); 
					
					DefSubclassProc(hWnd,uMsg,wParam,lParam);
					SOM_PRM_47a708.save() = save;					
					break;
				}
				case MAKEWPARAM(1002,C8N_SELCHANGE): goto separator;
				}
				break;

			default: //various

				switch(wParam)
				{
				case 1005: workshop:					
					if(workshop_exe(hlp)) return 0;
					break;

				//1002: profile separators?			
				//ALSO: profile is changing. is it a trap?
				case MAKEWPARAM(1002,C8N_SELCHANGE): separator:
				{
					//NOTE: categories shouldn't be empty
					int sel = ComboBox_GetCurSel(lp);
					if(!ComboBox_GetItemData(lp,sel)) //CB_ERR==
					ComboBox_SetCurSel(lp,sel+1); //skip

					if(sel!=-1) //goto profile_enable?
					if(IsWindowVisible(som_tool_workshop)) 
					workshop_exe(30000); //2023

					//NOTE: technically this should apply to 
					//all screens, and will probably need to
					//in the future
					switch(som_prm_tab)
					{
					case 1001: case 1007: //Item? //Trap?
					
						SOM_PRM_extend(1013); goto profile_enable;
					}

					break;
				}
				case MAKEWPARAM(1171,EN_CHANGE): //Strength
				case MAKEWPARAM(1173,EN_CHANGE): //Magic
				
					//Todo? normally SOM waits for WM_KILLFOCUS
					SendDlgItemMessage(hWnd,LOWORD(wParam)-1,TBM_SETPOS,1,
					(int)GetDlgItemInt(hWnd,LOWORD(wParam),0,0)-50);
					
					//Reminder: singleline WM_SETTEXT sends EN_CHANGE
					//To get around this SOM_PRM_extend saves/restores
					//this save prompt status.
					SOM_PRM_47a708.save() = 1;
					break;				
								
				case 1000: //wizard (close button)
				{
					SOM_PRM_extend(1013);

					int next = 0; switch(hlp)
					{
					case 35200: next = 1104; break;
					case 35300: next = 1105; break;
					case 35400: next = 1101; break;
					}
					if(!next) break;
					HWND gp = GetParent(hWnd);
					SendMessage(gp,WM_COMMAND,next,0);
					SetFocus(GetDlgItem(som_tool_dialog(),1000));
					return !0; //1?
				}
				case IDCANCEL: //2018: Esc isn't working???

					assert(hlp==30200);
					SendMessage(hWnd,WM_COMMAND,1233,0); //WM_CLOSE,0,0);
					break;
				}
			}
		}break;
		case SOM_MAP.exe:
		switch(hlp)
		{
		case 40000: //som_tool

			switch(wParam)
			{
			case 1010: goto workshop;

			case IDOK: //Elevation? 

				if(GetKeyState(VK_RETURN)>>15
				&&GetFocus()==GetDlgItem(hWnd,1000))
				wParam = 1014;
				break;

			case 1213:

				if(~GetKeyState(VK_SHIFT)>>15)
				{
					GetDlgItemText(hWnd,1008,som_tool_text,MAX_PATH);
					SetDlgItemText(hWnd,1000,som_tool_text);
				}
				break;

			case 1215: case 1216: case 1220: som_map_tab = wParam;

				SetCapture(0); //hack/paranoia: seems to be sticky 
				break;

			case 1080: //Play button? 
				
				som_tool_play = 1;
				SendMessage(hWnd,WM_COMMAND,1074,0);
				som_tool_play = 0; //paranoia
				break;

			case 3000: //Overlay?

				extern void SOM_MAP_open_overlay(); SOM_MAP_open_overlay();
				goto overlay;
				//InvalidateRect(SOM_MAP.painting,0,0); 
				//HACK: painting is far down below.
				RedrawWindow(som_tool,0,0,RDW_INVALIDATE|RDW_ALLCHILDREN);
				break;

			case MAKEWPARAM(1133,CBN_SELENDOK): //Map Layer?
				//HACK: open hidden maps menu
				som_tool_refocus = lp; 				
				SendMessage(hWnd,uMsg,1227,0);				
				//break;
			case MAKEWPARAM(1133,CBN_CLOSEUP): 			
				ComboBox_SetEditSel(lp,0,0); 
				//SetFocus(SOM_MAP.painting);
				//InvalidateRect(lp,0,1);
				break;
			}		
			break;

		case 41000: //maps

			switch(wParam) //2018
			{
			case IDOK: case MAKEWPARAM(1239,LBN_DBLCLK):
			
				//2018: MOVED FROM som_tool_CreateFileA
				extern int SOM_MAP_201_open(int=1); SOM_MAP_201_open();
				windowsex_enable<1225>(som_tool,0);
				//NEW: Ben Connolly bugfix     //undo
				break;
				
			case IDCANCEL: //basic layers support
					
				//indicats via som_LYT_compose that the 
				//current map was added/removed to/from
				//a layer table. it must be reloaded or
				//else will not reflect the change... a
				//prompt is not provided since the menu
				//already provided one upon its opening
				if(lParam=GetWindowLong(hWnd,DWL_USER))
				{
					wParam = IDOK;
					SendDlgItemMessage(hWnd,1239,LB_SETCURSEL,~lParam,0);
				}
				break;
			
			case 1071:

				extern void SOM_MAP_201_play(); SOM_MAP_201_play();
				return 0;

			case 1074:
			{
				HRSRC found = FindResource(0,MAKEINTRESOURCE(165),RT_DIALOG);
				DLGTEMPLATE *locked = (DLGTEMPLATE*)LockResource(LoadResource(0,found)); 
				windowsex_enable<IDIGNORE> //disable Record button
				(som_tool_CreateDialogIndirectParamA(0,locked,hWnd,som_tool_PseudoModalDialog,0),0);
				return 0;
			}
			case 1075:

				extern void som_LYT_compose(HWND); som_LYT_compose(hWnd);
				return 0;
			}
			break;

		case 42000: case 42100: //tile view

			assert(som_map_tileview);
			if(hWnd==som_map_tileview) //42000
			{
				switch(wParam)
				{
				case 1047:
					
					//2021: need to redraw after changing
					//the profile combobox (would prefer if
					//the change took affect immediately)
					som_map_tileviewpaint();
					
					EnableWindow(lp,0); break;

				case 1265: case 1266: case 1267: 
					uMsg = wParam==1267?0x1000:0x2000;
					if(Button_GetCheck(lp)==(wParam!=1266))					
					som_map_tileviewmask|=uMsg;
					else som_map_tileviewmask&=~uMsg;
					if(1266==wParam)
					CheckDlgButton(hWnd,1265,som_map_tileviewmask&0x2000?1:0);
					return 0;

				case 1005: case 1010: 

					extern bool SOM_MAP_z_scroll(WPARAM);
					if(!SOM_MAP_z_scroll(wParam))
					return 0; goto rec42000; 					
				}
			}
			else if(hWnd==som_map_tileviewinsert) //42100
			{
				bool rec = false; 				
				if(dlgc&DLGC_HASSETSEL) switch(HIWORD(wParam))
				{
				case EN_KILLFOCUS: 
					
					//IMPORTANT/REMINDER:
					//this is preventing SOM_MAP from
					//limiting numbers to a single decimal place
					if(som_tool_slide(lp)) return 0;			
					break;

				case EN_CHANGE: rec = true;
					
					som_tool_slide(lp); break;
				}
				else switch(HIWORD(wParam))
				{
				case BN_CLICKED: //1150 is profile button
					
					rec = DLGC_BUTTON&dlgc&&wParam!=1150; 
					
					switch(wParam)  
					{
					case 1232: //STN_CLICKED
						
						//2018: Not working anymore
						//PostMessage(hWnd,WM_KEYDOWN,'Q',0);
						som_map_Q();
						return 0;

					case 1037: case 1038: //new +/- buttons
						
						HWND z = GetDlgItem(som_map_tileviewinsert,1026);
						som_tool_increment(wParam&1?1:-1,z);
						if(!IsWindowEnabled(GetDlgItem(som_map_tileview,1047)))
						som_tool_slide(z); //seems necessary 
						break;						
					}
					break;

				case C8N_SELCHANGE: rec = true; //assuming combobox								

					if(1021==LOWORD(wParam))
					windowsex_enable<1045>(som_map_tileview); //Add
					break;
				}
				//enable Record if Add and Remove are enabled				
				if(rec) rec42000: 
				if(//selecting an event also allows modification
				//HMMM: but it doesn't seem to Record changes :(
				IsWindowEnabled(GetDlgItem(som_map_tileview,1045))
				&&IsWindowEnabled(GetDlgItem(som_map_tileview,1046)))
				{
					windowsex_enable<1047>(som_map_tileview);
					
					//repaint viewport and update the sliders
					if(HIWORD(wParam)==EN_CHANGE) switch(LOWORD(wParam))
					{
					case 1063: case 1064: break; //non-graphical sliders

					default: som_map_tileviewpaint(); break;
					}			
				}
			}
			break;

		case 44000: //map

			//HACK: repair 421340 (starting height)
			if(wParam==MAKEWPARAM(1030,EN_KILLFOCUS))
			{						 
				SendDlgItemMessage(hWnd,1141,TBM_SETPOS,1,GetWindowFloat(lp)*10);
				return 0;
			}
			break;
			
		case 45000: //events
		
			//enable copy/paste
			if(wParam==MAKEWPARAM(1210,LBN_SELCHANGE))
			{
				LRESULT out = DefSubclassProc(hWnd,uMsg,wParam,lParam);								
				if(10>ListBox_GetCurSel(lp))
				{
					HWND copy = GetDlgItem(hWnd,1213);
					if(GetWindowLong(copy,GWL_USERDATA))
					windowsex_enable<1047>(hWnd); //Paste
					EnableWindow(copy,1); //Copy 											
				}				
				return out;
			}
			else switch(wParam) //show empty events
			{
			case 1213: //Copy

				SetWindowLong(lp,GWL_USERDATA,1);
				break;

			case 1046: //Delete
			case 1047: //Paste Events
			
			  /*SOM_MAP resets the list each paste/delete*/

				HWND lb = GetDlgItem(hWnd,1210);
				int sel = ListBox_GetCurSel(lb);
				sel = SendMessage(lb,LB_GETITEMDATA,sel,0);
				if(wParam==1046) //delete
				som_map_zentai_rename(sel,L"",2);

				SCROLLINFO si = {sizeof(si),SIF_POS};
				GetScrollInfo(lb,SB_VERT,&si);

				LockWindowUpdate(lb);
				LRESULT out = IsWindowVisible(hWnd)? 
				DefSubclassProc(hWnd,uMsg,wParam,lParam):!0;
													
				int count = SendMessage(lb,LB_GETCOUNT,0,0);
				if(count>=1024) return out;
					
				int index;
				for(int i=0;i<1024;i++)
				{
					index = SendMessage(lb,LB_GETITEMDATA,i,0);

					if(index<=CB_ERR) 
					{
						index = 1024; assert(i>=count);
					}
					else if(index>1023) //paranoia
					{
						index = 1024; assert(0);
					}
					while(i<index)
					{
						static wchar_t fill[] = L"[0000]  ---  ";
						swprintf(fill+1,L"%04d",i); fill[5] = ']';
						int j = SendMessageW(lb,LB_INSERTSTRING,i++,(LPARAM)fill);
						SendMessage(lb,LB_SETITEMDATA,j,j);
						assert(j==i-1||j==CB_ERR);
					}
				}
				if(index<1023) som_tool_boxen.add(lb,L'\0',1023); 

				ListBox_SetCurSel(lb,sel);
				SendMessage(lb,WM_VSCROLL,si.nPos<<16|SB_THUMBPOSITION,0);
				LockWindowUpdate(0);
				ValidateRect(hWnd,0); //flickers???				
				InvalidateRect(lb,0,0);								
				InvalidateRect((HWND)lParam,0,0);
				return out;
			}
			break;
		
		case 45100: //event 
		
			switch(wParam)
			{
			case MAKEWPARAM(1199,C8N_SELCHANGE): //class
			case MAKEWPARAM(1014,C8N_SELCHANGE): //protocol 
			{
				//System/Systemwide				
				LRESULT sel = ComboBox_GetCurSel(lp);
				//2018: Why was this not being done???
				if(1199==LOWORD(wParam))
				{
					HWND cb = GetDlgItem(hWnd,1213);
					ComboBox_SetCurSel(cb,sel>=3?-1:0);
					EnableWindow(cb,sel<3);
				}
				//2018: Looks like NO-OP!!! But does something???
				//(Setting to self???)
				ComboBox_SetCurSel(lp,sel);
				break;
			}
			case 1211: //open loop
			
				HWND lb = GetDlgItem(hWnd,1025);
				SetFocus(lb); SendMessage(lb,WM_KEYDOWN,VK_SPACE,0);
				SetFocus(lp); break;
			}
			break;
			
		case 45300: //loop
		
			switch(wParam) 
			{			
			case IDCANCEL: return SendMessage(hWnd,WM_CLOSE,0,0);

			case 1012: //comment
				//REPURPOSING
				som_tool_play = SendDlgItemMessageW(hWnd,1029,LB_FINDSTRINGEXACT,-1,
				LPARAM(som_map_instructions[EX::need_unicode_equivalent(932,(char*)0x478548)].c_str()+1))+1;
				wParam = 1009; //2022
				//break;
			case 1009: case MAKEWPARAM(1029,LBN_DBLCLK): //insert? or comment?
			{		  
				HWND program = GetDlgItem(hWnd,1026);
				//int _ = ListBox_GetCurSel(program);
				//if(_==LB_ERR) break; //2018
				lParam = DefSubclassProc(hWnd,uMsg,wParam,lParam);					
				//2018: notify to enable/disable buttons
				//ListBox_SetCurSel(program,_+1);
				SendMessage(program,WM_KEYDOWN,VK_DOWN,0);
				if(IsDlgButtonChecked(hWnd,1011)) //above
				SendMessage(hWnd,WM_COMMAND,1027,0); //raise
				if(auto*ed=SOM_MAP.evt_data(hWnd,ListBox_GetCurSel(program)))
				{
					switch(ed->code) //default manipulation
					{
					case 0: //text/comment extension

						if(som_tool_play) //tag comment
						((WORD*)ed->data[0])[0] = 0x01; break; //'\1'

					case 25: case 26: //npc/enemy warp

						ed->data[0][6] = SOM_MAP_z_index; break;

					case 102: //move/warp object

						ed->data[0][24] = SOM_MAP_z_index; break;
						
					case 60: //map change

						if(SOM_MAP.evt_code(59))
						{
							ed->data[0][20] = 16; //relative

							((WORD*)ed->data[0])[1] = 0x1211; //fog/sky
						}
						else ((WORD*)ed->data[0])[1] = 0xffff; //no fade
						
						break;

					case 61: //warp

						ed->data[0][17] = SOM_MAP_z_index; break;
					}
				}
				if(som_tool_play) //comment???
				{
					SendMessage(som_tool_pushed=program,WM_KEYDOWN,VK_SPACE,0);
					assert(!som_tool_play);
				}
				return lParam;
			}
			case 1008: case MAKEWPARAM(1026,LBN_DBLCLK): //open
			{	
				//REMINDER: GetCurSel is hooked to
				//check som_tool_play (REPURPOSED)
				HWND program = GetDlgItem(hWnd,1026);						
				int sel = ListBox_GetCurSel(program);
				som_map_instruct = ListBox_GetItemData(program,sel);
				som_map_codesel = 0;
				//*som_tool_text = '\0';
				//don't want indentation (in the window title)
				//SendMessageW(program,LB_GETTEXT,sel,(LPARAM)som_tool_text);	
				//SendDlgItemMessageW(hWnd,1029,LB_GETTEXT,som_map_instruct-1,(LPARAM)som_tool_text);	
				std::map<std::wstring,std::wstring>::iterator sort;
				for(sort=som_map_instructions.begin();sort!=som_map_instructions.end();sort++)
				if(som_map_instruct==sort->second[0])
				{
					wmemcpy(som_tool_text,sort->second.c_str()+1,sort->second.size());
					break;				
				}
				som_tool_help = 46000;   
				switch(som_map_instruct)
				{
					//REMINDER: these are off by 1 for evt_desc
				case 40:					som_tool_help+=100; //81) Standby Map (EXTENSION)
				case 38: case 39:			som_tool_help+=100; //80) Timers
				case 36:					som_tool_help+=100; //79) Loops
				case 35: case 37:			som_tool_help+=100; //78) Counters
				case 33: case 34:
				case 31: case 32:			som_tool_help+=100; //77) Branching
				case 30:					som_tool_help+=100; //76) Game Over
				case 29:					som_tool_help+=100; //75) Save
				case 28:					som_tool_help+=100; //74) System
				case 25: case 26: case 27:	som_tool_help+=100; //73) Objects
				case 23:					som_tool_help+=100; //72) Heal
				case 22:					som_tool_help+=100; //71) Magic
				case 20: case 21: case 24:	som_tool_help+=100; //70) Status
				case 18: case 19:			som_tool_help+=100; //69) Warp
				case 16: case 17:			som_tool_help+=100; //68) BGM
				case 13: case 14: case 15:	som_tool_help+=100; //67) Media
				case 11: case 12:			som_tool_help+=100; //66) Displays
				case 9:  case 10:			som_tool_help+=100; //65) Disappear
				case 7:  case 8:/*case 27:*/som_tool_help+=100; //64) Movement
				case 6:						som_tool_help+=100; //63) Shop
				case 3:  case 4:  case 5:	som_tool_help+=100; //62) Appearance
				case 1:  case 2:			som_tool_help+=100; //61) Text
				
				break; default: assert(0);
				}
				som_tool_find = 0; 
				switch(som_map_instruct)
				{
				case 3: som_tool_find = "X_136A"; break; //NPC
				case 4: som_tool_find = "X_136B"; break;
				case 5: som_tool_find = "X_136C"; break;
				case 6: som_tool_find = "X_136D"; break; //Shop
				case 9: som_tool_find = "X_136E"; break; //Kill
				case 10: som_tool_find = "X_136F"; break;
				case 11: som_tool_find = "X_136G"; break; //Lens
				case 15: som_tool_find = "X_136H"; break; //Sound
				case 17: som_tool_find = "X_136I"; break; //Mute
				case 22: som_tool_find = "X_136J"; break; //Magic
				case 30: som_tool_find = "X_136K"; break; //Game Over
				case 38: som_tool_find = "X_136L"; break; //Timer

				case 31: som_tool_find = "X_151A"; break; //IF
				case 35: som_tool_find = "X_151B"; break; //Counter
				case 37: som_tool_find = "X_151C"; break; //Random

				case 13: som_tool_find = "X_155A"; break; //Image
				case 14: som_tool_find = "X_155B"; break; //video
				case 16: som_tool_find = "X_155C"; break; //Music

				case 21: som_tool_find = "X_171A"; break; //IF
				case 25: som_tool_find = "X_171B"; break; //animate
				case 26: som_tool_find = "X_171C"; break; //show/hide
				case 28: som_tool_find = "X_171D"; break; //System
				}
				if(1==som_map_instruct&&!som_tool_play)
				if(auto*ed=SOM_MAP.evt_data(hWnd,sel))
				som_tool_play = '\1'==*ed->data[0]?-1:0;
				lParam = DefSubclassProc(hWnd,uMsg,wParam,lParam);
				som_tool_play = 0;
				return lParam;
			}}			
			break;

		}break;
		case SOM_SYS.exe:
		{
			if(som_sys_slideshow) 
			{
				if(hlp==56200) 
				{
					switch(wParam)
					{
					case 1005: //Record?

						if(GetDlgItem(hWnd,1080))
						som_tool_play = IDNO;
						//break;

					case 1080: //Play?

						wParam = 1005; goto def;
					}
				}
				else switch(wParam)
				{			
				case 1005: som_sys_slideshow = 1; break;
				case 1008: som_sys_slideshow = 2; break;
				case 1021: som_sys_slideshow = 3; break;
				case 1012: som_sys_slideshow = 4; break;
				case 1015: som_sys_slideshow = 5; break;
				case 1018: som_sys_slideshow = 6; break;
				}
			}
			else switch(wParam)
			{
			case 1022: //alternative default tab
			{
				static bool one_off = false; if(one_off++) break; //???
				HWND tab = GetWindow(hWnd,GW_CHILD); SetFocus(tab);
				if(DLGC_BUTTON&SendMessage(tab,WM_GETDLGCODE,0,0)) //paranoia 
				return SendMessage(hWnd,WM_COMMAND,GetDlgCtrlID(tab),(LPARAM)tab);
				break;
			}
			case 1018: //Retail or Debug player?

				//SETUP is expected to be 1 or 2.
				if(som_tool_initializing&&hlp==52100)
				if(2==wcstol(Sompaste->get(L"SETUP"),0,10))
				wParam = 1044; //2
				break;

			//these sliders/boxes don't work properly
			case MAKEWPARAM(1019,EN_KILLFOCUS): //walk
			case MAKEWPARAM(1020,EN_KILLFOCUS): //dash
				DefSubclassProc(hWnd,uMsg,wParam,lParam);
				//break;
			case MAKEWPARAM(1019,EN_CHANGE): //walk
			case MAKEWPARAM(1020,EN_CHANGE): //dash
				if(hlp==57100)
				SendDlgItemMessage(hWnd,LOWORD(wParam)+57,TBM_SETPOS
				,1,int(GetDlgItemFloat(hWnd,LOWORD(wParam))/0.1f+0.5f)-1);
				return 0;
			}
			break;
		}}
		break;
	}	
	case WM_SHOWWINDOW:
				
		if(wParam) if(hWnd==som_tool)
		{										
			if(som_tool_taskbar)
			{
				WCHAR text[30] = L""; //chkstk?
				GetWindowTextW(hWnd,text,EX_ARRAYSIZEOF(text));
				SetWindowTextW(som_tool_taskbar,text);
				ShowWindow(som_tool_taskbar,1);
			}

			//lower splash screen
			if(SOM::splashed(hWnd,lParam)) return 0; 
		}
		else if(hWnd==som_map_tileview)
		{
			//HWND tv = GetDlgItem(hWnd,1052);
			//SendMessage(tv,TVM_SELECTITEM,TVGN_CARET,0);
			//PostMessage(tv,TVM_SELECTITEM,TVGN_CARET,(LPARAM)TreeView_GetChild(tv,TVI_ROOT));			
			SetTimer(hWnd,'show',0,som_map_showtileview);	
		}
		break;

	case WM_VSCROLL: //trackbars part 1
	{
		int n = SendMessage((HWND)lParam,TBM_GETRANGEMIN,0,0);		
		int x = SendMessage((HWND)lParam,TBM_GETRANGEMAX,0,0);

		if(n==x) break; //0: indicative of an updown control

		//SOM ignores this but we'll keep it
		//trackbarproc overrides TBM_SET/GETPOS
		uMsg = WM_HSCROLL; switch(LOWORD(wParam)) 
		{
		case SB_THUMBTRACK: case SB_THUMBPOSITION: 
		{
			int pos = HIWORD(wParam);
			
			pos = x-(pos-n);
			wParam = MAKEWPARAM(LOWORD(wParam),pos);
			break;
		}
		case SB_BOTTOM: wParam = SB_LEFT; break;
		case SB_LINEDOWN: wParam = SB_LINELEFT; break;
		case SB_LINEUP: wParam = SB_LINERIGHT; break;
		case SB_PAGEDOWN: wParam = SB_PAGELEFT; break;
		case SB_PAGEUP: wParam = SB_PAGERIGHT; break;
		case SB_TOP: wParam = SB_RIGHT; break;
		}
		return SendMessage(hWnd,WM_HSCROLL,wParam,lParam);
	}
	case WM_HSCROLL: //trackbars part 2 
	{
		int tb = GetDlgCtrlID((HWND)lParam);
		if(tb==3001) overlay:
		{
			//REMOVE ME?
			extern HWND SOM_MAP_painting;
			if(SOM_MAP_painting) //SOM_MAIN?
			InvalidateRect(SOM_MAP_painting,0,0); 
			return 0;
		}			
		if(SOM_PRM.exe==SOM::tool) switch(tb)
		{
		case 1170: case 1172: //Strength & Magic?				

			//GOD THIS IS STUPID! (EN_CHANGE)
			//(maybe singlelineproc should handle WM_SETTEXT?)
			int a = GetDlgItemInt(hWnd,tb+1,0,0);
			int b = 50+SendMessage((HWND)lParam,TBM_GETPOS,0,0);
			if(a!=b) SetDlgItemInt(hWnd,tb+1,b,0);
			return 0;
		}
		else if(tb==1005) //if(hWnd==som_map_tileview) 
		{
			switch(hlp) //SOM_MAP_z_scroll
			{
			case 42000: case 44000:
			case 46400: case 46900: case 47300: //2022

				switch(LOWORD(wParam))
				{
				case SB_ENDSCROLL:
				case SB_THUMBPOSITION: //no movement???
					break;
				default:
					SendMessage(hWnd,WM_COMMAND,tb,0); //HACK			
				}
				return 0;

			default: assert(0); break;
			}
		}
		else if(hWnd==som_map_tileviewinsert)
		{
			//TODO: just do som_tool_slide((HWND)lParam);			

			int edit = 0;
			int x = SendMessage((HWND)lParam,TBM_GETRANGEMAX,0,0);
			switch(tb)
			{
			case 1022: edit = 1025; break;
			case 1036: edit = 1026; break;
			case 1023: edit = 1026; break;
			case 1024: edit = 1027; break;
			case 1028: edit = 1031; break;
			case 1029: edit = 1032; break; 
			case 1030: edit = 1033; break;
			case 1034: edit = 1035; break;
			}
			if(!edit) break;

			//.1 increment size slider
			if(edit==1035&&x==20) x = 0;

			float f = GetDlgItemFloat(hWnd,edit);
			float g = SendMessage((HWND)lParam,TBM_GETPOS,0,0);

			float mf = 0; switch(x)
			{		
			case 0: case 10: case 200: 
				
				mf = 0.1f; break;

			default: assert(0);					
			case 255: case 360: case 100:
			return DefSubclassProc(hWnd,uMsg,wParam,lParam);

			  ////new extended increments////
			
			case 19: mf = 1; break; //big Z slider

			case 20: case 60: mf = 0.05f; break; //center/size

			case 72: mf = 5; break; //360 degree angle
			}

			g*=mf;			
			if(edit==1026) if(x==19) //add little Z to big z
			{
				mf = 0.05f; g+=
				mf*SendDlgItemMessage(hWnd,1023,TBM_GETPOS,0,0);				
			}
			else if(x==20) //add unbounded big Z to little z
			{
				g+=som_tool_slideZ; 			
			}
			
			float fmf = fmodf(f,mf); 			
			if(mf-fabsf(fmf)>0.000003f&&fabsf(fmf)>0.000003f) 			
			g+=f<0?fmf+mf:fmf; 

			switch(x)
			{
			case 72: SetDlgItemInt(hWnd,edit,g+0.5f,0); break;
			default: som_tool_increment(g-f,GetDlgItem(hWnd,edit));
			}			
			return 0;
		}
		break;
	}
	case WM_NOTIFY:
	{	 
		NMHDR *nm = (NMHDR*)lParam;	

		switch(nm->code)
		{
		case TTN_GETDISPINFOW:

			switch(hlp) //layer sliders?
			{
			case 42000: case 44000:
			case 46400: case 46900: case 47300: //2022
			
				//TODO: show names for destination map
				if(som_map_instruct==18) break;

				extern void som_LYT_tooltip(NMTTDISPINFO&);
				som_LYT_tooltip(*(NMTTDISPINFO*)nm);
			}
			break;

		case TVN_ITEMEXPANDEDW:
		{
			NMTREEVIEW *p = (NMTREEVIEW*)lParam;

			if(p->action==TVE_COLLAPSE) //collapse recursively
			{
				NMTREEVIEWW nmtv = *p;
				HTREEITEM i = //assuming collapse always succeeds
				TreeView_GetChild(nm->hwndFrom,p->itemNew.hItem);
				if(i) do //TreeView_Expand(p->hdr.hwndFrom,i,TVE_COLLAPSE);
				{
					nmtv.itemNew.hItem = i;
					nmtv.hdr.code = TVN_ITEMEXPANDINGW;
					TreeView_GetItem(nm->hwndFrom,&nmtv.itemNew);					
					if(SendMessage(hWnd,WM_NOTIFY,nm->idFrom,(LPARAM)&nmtv)) assert(0);
					TreeView_Expand(p->hdr.hwndFrom,i,TVE_COLLAPSE);
					nmtv.hdr.code = TVN_ITEMEXPANDEDW;
					SendMessage(hWnd,WM_NOTIFY,nm->idFrom,(LPARAM)&nmtv);
				}
				while(i=TreeView_GetNextSibling(p->hdr.hwndFrom,i));
				break;
			}
			break;
		}
		case NM_CLICK: //full-row +/-/selection
		{
			//WARNING: PREVENTS FURTHER PROCESSING OF CLICKS
			//(could handling in treeviewproc but WM_CLICK?)

			HWND tv = nm->hwndFrom;
			if(som_tool_treeview()==GetClassAtom(tv))
			{	
				TVHITTESTINFO hti; GetCursorPos(&hti.pt);
				MapWindowPoints(0,tv,&hti.pt,1);
				HTREEITEM sel = TreeView_GetSelection(tv);
				HTREEITEM hit = TreeView_HitTest(tv,&hti);			
				if(hti.flags&(TVHT_ONITEMBUTTON|TVHT_ONITEMINDENT))
				{
					SetFocus(tv); som_tool_toggle(tv,hit,hti.pt);					
					return 1;
				}
				else if(hti.flags&(TVHT_ONITEM|TVHT_ONITEMRIGHT))
				{
					if(hit) if(hit!=sel) 
					{
						SetFocus(tv); TreeView_SelectItem(tv,hit);			
						return 1; 
					}
				}				
			}
			break;
		}
		case LVN_ITEMCHANGING: //muti-selection
		{
			NMLISTVIEW *p = (NMLISTVIEW*)lParam;
			
			if(p->uChanged&LVIF_STATE) 
			{
				//make keyboard act like the mouse
				int x = p->uNewState^p->uOldState;
				if(x&LVIS_SELECTED||x&LVIS_FOCUSED)
				if(GetKeyState(VK_SHIFT)>>15&&!IsLButtonDown())
				{
					static int prevfocus = -1; 
					static bool unioning = false; 
					if(x&LVIS_FOCUSED)
					{
						unioning = true;
						if(p->uNewState&LVIS_FOCUSED) 
						{
							HWND lv = nm->hwndFrom;							
							int m = ListView_GetSelectionMark(lv);							
							if(m>=0) //fails if single select
							{	
								//prevfocus: LVNI_FOCUSED is -1
								int i = p->iItem, f = prevfocus; 
								bool inverting = GetKeyState(VK_CONTROL)>>15;								
								int selected = abs(m-f)>abs(m-i)?0:LVIS_SELECTED; 														
								if(!selected) std::swap(i,f); 
								for(int inc=i<f?1:-1;i!=f;i+=inc)
								ListView_SetItemState(lv,i,inverting?~
								ListView_GetItemState(lv,i,~0):selected,LVIS_SELECTED);
								if(f==m) //select selection marker 
								ListView_SetItemState(lv,f,inverting?~
								ListView_GetItemState(lv,f,~0):LVIS_SELECTED,LVIS_SELECTED);
							}
						}
						else prevfocus = p->iItem;
						unioning = false;
					}
					else if(!unioning) 
					{						
						return 1; //keep selection
					}
				}
			}
			break;
		}
		case NM_CUSTOMDRAW:
		{
			NMCUSTOMDRAW* &p = (NMCUSTOMDRAW*&)lParam;			
			NMLVCUSTOMDRAW* &q = (NMLVCUSTOMDRAW*&)lParam;

			switch(p->dwDrawStage)
			{
			default: assert(0); break; //sniffing

			//2018: Separate from PREPAINT. Thinking that erasure
			//is not required by any of the language-pack elements
			//(Well, plain buttons use this.)
			case CDDS_PREERASE: break; //return CDRF_SKIPDEFAULT;

			case CDDS_PREPAINT: //want per item behavior?
			
				if(som_tool_listview()==GetClassAtom(p->hdr.hwndFrom)) 
				{
					//SOM_SYS: fixes unfocus sans LVS_EX_DOUBLEBUFFER
					if(SOM_SYS.exe==SOM::tool)
					q->clrFace = GetSysColor(COLOR_WINDOW);

					return CDRF_NOTIFYSUBITEMDRAW;//|CDRF_NOTIFYPOSTPAINT;
				}
				break; //return CDRF_DODEFAULT;
			
			case CDDS_ITEMPREPAINT: 
			{		 
				//filters out the spreadsheet like lists
				//that use hardcoded Class dark blue ink
				int SOM = DefSubclassProc(hWnd,uMsg,wParam,lParam);				
				//2018: Shouldn't SOM_MAIN/EDIT be enjoying this?
				//if(SOM) //return SOM;
				//---Those are't valid return flags???---
				//SEEMS THIS REQUIRED PARENTHESES!!!!!!!!
				//assert(!SOM||SOM==CDRF_SKIPDEFAULT);
				assert(SOM!=(CDDS_ITEMPREPAINT|CDDS_SUBITEM));
			 				
				HWND lv = p->hdr.hwndFrom; 
				int st = ListView_GetItemState(lv,p->dwItemSpec,~0);

				if(SOM)
				if(hlp==32100||hlp==52100)  //2018: inventories
				{
					assert(SOM==CDRF_NOTIFYSUBITEMDRAW);					

					//replace SOM's dark blue highlight with 
					//multi-select highlight?					
					if(~p->uItemState&(CDIS_SELECTED|CDIS_FOCUS)
					||1>=ListView_GetSelectedCount(lv))	 					
					{
						//return SOM;
						return CDRF_NOTIFYSUBITEMDRAW; 
					}
					else SOM = CDRF_DODEFAULT;
					
					//I don't understand what uItemState is, but
					//this works to clear SOM's focus after multi
					//selection with LVS_EX_DOUBLEBUFFER fixes.
					if(p->uItemState&CDIS_SELECTED)
					if(st&LVIS_SELECTED)
					goto lasso1;			
					q->nmcd.uItemState = 0;					
				//	return 0;					
				}
				
				if(~st&LVIS_SELECTED)
				{
					//note: we use GLOW more like CUT would 
					//be used because there is no TVIS_GLOW
					if(LVIS_CUT&st)
					q->clrText = GetSysColor(COLOR_GRAYTEXT);
					if(LVIS_GLOW&st)
					q->clrTextBk = GetSysColor(COLOR_3DFACE);
				}	   
								
				if(st&LVIS_FOCUSED) if(GetFocus()==lv) 
				{
					if(LVIS_SELECTED&st) lasso1:
					{				
						q->clrTextBk = GetSysColor(COLOR_HOTLIGHT);	
						q->clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);	
						q->nmcd.uItemState = 0;
					}
					else q->clrText = GetSysColor(COLOR_HOTLIGHT);							
				}				
				else if(LVIS_SELECTED&st)
				if(1<ListView_GetSelectedCount(p->hdr.hwndFrom))
				{
					//face is the default
					//light is darker than face on Vista/7
					//shadow is darker than face on Classic
					COLORREF a = GetSysColor(COLOR_3DFACE);
					COLORREF b = GetSysColor(COLOR_3DLIGHT);					
					q->clrTextBk = a!=b?b:GetSysColor(COLOR_3DSHADOW);
					q->nmcd.uItemState = 0;		
				}			   
				return SOM; //CDRF_DODEFAULT; //0
			}
			//2018/LVS_EX_DOUBLEBUFFER
			//Not sure if this is right, but SOM_SYS's inventory table
			//goes blank at the selection mark, and below, when using 
			//LVS_EX_DOUBLEBUFFER
			case CDDS_ITEMPREPAINT|CDDS_SUBITEM:
			//return CDRF_NOTIFYPOSTPAINT; 
			//case CDDS_ITEMPREPAINT|CDDS_SUBITEM:
			//case CDDS_ITEMPOSTPAINT|CDDS_SUBITEM: 
			
				if(hlp==32100||hlp==52100) //2018: inventories
				{
					//2018: There is a layout bug when horizontal 
					//scrolling is in effect??? 
					//REMINDER: Do this only once!
					OffsetRect(&p->rc,GetScrollPos(p->hdr.hwndFrom,SB_HORZ),0);				

					//Ultimately this seems like the only way to make
					//SOM_SYS work with LVS_EX_DOUBLEBUFFER. Not sure
					//why it's different.
					p->dwDrawStage = CDDS_ITEMPOSTPAINT|CDDS_SUBITEM;				
					int SOM = DefSubclassProc(hWnd,uMsg,wParam,lParam);	
					p->dwDrawStage = CDDS_ITEMPREPAINT|CDDS_SUBITEM;
					//HACK: SOM_SYS+LVS_EX_DOUBLEBUFFER
					if(!SOM) q->nmcd.uItemState = 0;	
					else assert(SOM==CDRF_SKIPDEFAULT);
					return SOM;
				}
				else assert(0); break;
			}
			break;
		}
		case EN_REQUESTRESIZE: //Rich Edit only extension
		{	
			//hack: allow SOM_MAIN.cpp/SOM_EDIT.cpp to override 
			LRESULT out = DefSubclassProc(hWnd,uMsg,wParam,lParam);
			if(out) return out;
		
			HWND &ed = nm->hwndFrom;			

			//TODO: Properly handle margins?
			//som_map_46100_etc is setting the margin to compensate
			//for height_adjustment :(
			//EM_SETTARGETDEVICE blows out the margin
			bool quickfix = 
			som_tool_richtext_margin!=HIWORD(SendMessage(ed,EM_GETMARGINS,0,0));
			if(quickfix) switch(hlp)
			{
			default: assert(0); case 46100: case 47700: break;

			case 20300: break; //2021: just disabling assert (SOM_EDIT Settings)
			}			

			//Reminder: this is to be applied to the text
			//boxes with standard double-byte/width sizes
			//That is all of the original multiline boxes

			static int paranoia = 0;
			if(paranoia++) break; //unsure what to do?									
			repos: //must come before es
			LONG es = GetWindowStyle(ed);
			RECT fr = Sompaste->frame(ed,hWnd);	
			if(fr.bottom-fr.top<2) return paranoia = 0; 
			RECT rc = ((REQRESIZE*)lParam)->rc; 
			HANDLE of = GetPropW(ed,L"SomEx_overflow");
			HANDLE pos = (HANDLE)MAKELONG(fr.left,fr.top); 			
			if(rc.bottom>fr.bottom&&(!of||of==pos))
			{	
				//add scrollbars and let text pass behind the
				//vertical scrollbar so the width is retained

				if(!of) //if(es&ES_AUTOVSCROLL)
				{
				SetPropW(ed,L"SomEx_overflow",pos);
				
					if(!quickfix)
					{
					//EM_SETTARGETDEVICE generates EN_REQUESTRESIZE
					SetWindowLong(ed,GWL_STYLE,es|ES_AUTOVSCROLL|ES_AUTOHSCROLL);
					//SendMessage(ed,EM_SETOPTIONS,ECOOP_OR,ES_AUTOVSCROLL|ES_AUTOHSCROLL);												
					//96: GetDeviceCaps(hDC,LOGPIXELSX)
					//4: accounting for formatting rectangle
					LPARAM wrap = (fr.right-fr.left-4)*1440/96; //twips
					SendMessage(ed,EM_SETTARGETDEVICE,(WPARAM)som_tool_dc,wrap);
					int sb = rc.bottom>GetSystemMetrics(SM_CYHSCROLL)*3?SB_BOTH:SB_VERT;
					ShowScrollBar(ed,sb,1); //important last				
					RedrawWindow(ed,0,0,RDW_FRAME|RDW_INVALIDATE);
					}

				//todo: extension to disable this behavior				
				if(ed!=SetFocus(ed)) Edit_SetSel(ed,0,-1); MessageBeep(-1);
				}
			}
			else if(of) //else if(es&ES_AUTOVSCROLL)
			{	
				if(!quickfix)
				{
					SetWindowLong(ed,GWL_STYLE,es&~(ES_AUTOVSCROLL|ES_AUTOHSCROLL));
					//SendMessage(ed,EM_SETOPTIONS,ECOOP_AND,~(ES_AUTOVSCROLL|ES_AUTOHSCROLL));												
					SendMessage(ed,EM_SETTARGETDEVICE,0,0);
					ShowScrollBar(ed,SB_BOTH,0); //important last
					RedrawWindow(ed,0,0,RDW_FRAME|RDW_INVALIDATE);
				}
				SetPropW(ed,L"SomEx_overflow",0);				

				//NEW: repositioning (SOM_MAIN_170)
				//EM_REQUESTRESIZE seems not to work at all here???
				if(of!=pos) //PostMessage(hWnd,EM_REQUESTRESIZE,0,0);
				{
					assert(!quickfix);
					goto repos;
				}
			}
			if(quickfix) break;
			paranoia = 0;
			return 1;
		}
		case EN_LINK: //auto URLs
		{
			ENLINK  *p = (ENLINK*)lParam;			
			LRESULT out = DefSubclassProc(hWnd,uMsg,wParam,lParam);
			if(!out) switch(p->msg)
			{
			case WM_SETCURSOR: 

				//18 is 4 in SOM_MAIN.cpp
				//supposed to be a WM_MOUSE code
				//assert(HIWORD(lParam)==18); 
				SetCursor(LoadCursor(0,IDC_ARROW)); 
				return 1;

			case WM_LBUTTONDOWN: 
			{				
				POINT pt; HWND ta = p->nmhdr.hwndFrom; 
				if(GetCursorPos(&pt)&&!DragDetect(ta,pt))
				{
					static WCHAR url[1024]; //chkstk
					TEXTRANGE tr = {p->chrg,url}; *url = '\0';
					if(EX_ARRAYSIZEOF(url)-1>tr.chrg.cpMax-tr.chrg.cpMin)
					SendMessageW(ta,EM_GETTEXTRANGE,0,(LPARAM)&tr);
					if(!*url
					 ||(HINSTANCE)33>ShellExecuteW(hWnd,L"openas",url,0,0,1)
					 &&(HINSTANCE)33>ShellExecuteW(hWnd,L"open",url,0,0,1))
					MessageBeep(-1);
					return 1;
				}
				break; 
			}}
			return out;
		}}

		//W->A translation for tools
		//SOM_EDIT's tool is new/found in SOM_EDIT.cpp
		//NOTE: except for number fields something needs to be done for
		//all of these cases in order to support Unicode fully
		if(SOM::tool==SOM_EDIT.exe)	break;
		if(SOM::tool==SOM_MAIN.exe)	break;				
		
		int code = nm->code;

		switch(nm->code)
		{		
		case TVN_SELCHANGEDW:
						
			if(IsWindowVisible(hWnd)) //som_map_showtileview
			{
				nm->code = TVN_SELCHANGEDA;
				som_map_tileviewmask^=0x80000000; //som_map_tileviewpaint
			}
			break;

		//case TVN_SELCHANGINGW: //UNUSED 
		case TVN_SELCHANGINGW: //nm->code = TVN_SELCHANGINGA; break;			

			extern bool SOM_MAP_tileview_1047(); //MB_YESNOCANCEL
			return !SOM_MAP_tileview_1047()?1:0;

		case TVN_GETDISPINFOW:
		case TVN_SETDISPINFOW: assert(0); break;

		case LVN_BEGINLABELEDITW: nm->code = LVN_BEGINLABELEDITA; break;
		case LVN_ENDLABELEDITW: nm->code = LVN_ENDLABELEDITA; break;
		case LVN_SETDISPINFOW: assert(0); break;
		case LVN_GETDISPINFOW:
			
			if(SOM::tool==SOM_SYS.exe) //no idea why this is sent to this listview???
			{
				//possible explanation?
				//if text is not set to "" it seems it defaults to LPSTR_TEXTCALLBACK 
				if(nm->idFrom==1015) break; //inventory
			}		
			assert(0); break;

		case UDN_DELTAPOS:
		{
			float inc = 0;
			if(hWnd==som_map_tileviewinsert) 
			switch(nm->idFrom)
			{
			case 1080: case 1081: case 1082: case 1086:

				inc = 0.01f;				
			}
			else if(hWnd==som_tool&&SOM_MAP.exe==SOM::tool) 
			{
				if(1081==nm->idFrom) //Elevation?
				{
					if(GetKeyState(VK_SHIFT)>>15)
					{
						inc = GetDlgItemFloat(som_tool,999);
						if(!inc) inc = 0.1f;
					}
					else inc = 0.5f;
				}
			}
			if(inc==0) break; 
			som_tool_increment(inc*-((NMUPDOWN*)lParam)->iDelta,GetWindow(nm->hwndFrom,GW_HWNDPREV));
			return 0;
		}}

		if(code==nm->code) break;
					
		WNDPROC A = (WNDPROC)GetPropW(hWnd,L"A"); 		
		//CallWindowProcA(A,hWnd,uMsg,wParam,lParam);
		LRESULT out = (A?A:DefSubclassProc)(hWnd,uMsg,wParam,lParam);

		if(som_map_tileview) switch(code) 
		{
		case TVN_SELCHANGEDW: //gray Record etc.
			 
			HWND &ins = som_map_tileviewinsert;
			TVITEM &tvi = ((NMTREEVIEW*)lParam)->itemNew;
			HTREEITEM gp = TreeView_GetParent(nm->hwndFrom,tvi.hItem);
			if(!gp) 
			{
				//2017: noticed profile set up button doesn't do anything
				//until a profile is selected. I think it's cleaner now to
				//blank the profile. I think that's the original way
				windowsex_enable<1045>(hWnd,0); //Add
				windowsex_enable<1150>(ins,0); //profile setup
				SendDlgItemMessage(ins,1021,CB_SETCURSEL,-1,0);
			}
			//else //moving code into SOM_MAP.cpp
			{
				extern void SOM_MAP_z_index_etc(HTREEITEM,int);
				SOM_MAP_z_index_etc(gp,tvi.mask&TVIF_PARAM?tvi.lParam:-1);
			}
			//important that Record is greyed last
			windowsex_enable<1047>(hWnd,0);
//			//was disabling Add but its simpler to fill it out
//			if(-1==SendDlgItemMessage(ins,1021,CB_GETCURSEL,0,0))
//			SendDlgItemMessage(ins,1021,CB_SETCURSEL,0,0);			
			som_map_tileviewmask^=0x80000000;
			som_map_tileviewpaint();
			break;
		}
  
		nm->code = code; return out;
	}
	case WM_CLOSE: //added by language pack

		//REMINDER: these are sent for the X button only
		switch(hlp)
		{
		case 10000: //SOM_MAIN

			SendMessage(hWnd,WM_COMMAND,1000,0); return 0;	//Exit

		case 10100: //SOM_MAIN: New Project

			SendMessage(hWnd,WM_COMMAND,1006,0); return 0;	//Cancel

		case 20000: //SOM_EDIT

			//let [X] close SOM_EDIT without opening SOM_MAIN
			//SendMessage(hWnd,WM_COMMAND,1004,0); return !0;	//Close
			//ExitProcess(0);
			goto close; //2021
		
		case 30000: //SOM_PRM

			SendMessage(hWnd,WM_COMMAND,1000,0); return 0;	//Cancel

		case 30300: //???
		case 30200: //model preview

			SendMessage(hWnd,WM_COMMAND,1233,0); return 0;	//OK

		case 30100: close: //loading screens
		case 40100: EX::is_needed_to_shutdown_immediately(0,"WM_CLOSE");

		case 40000: //SOM_MAP

			SendMessage(hWnd,WM_COMMAND,57665,0); return 0; //Cancel		

		case 42000: //SOM_MAP tile contents dialog
										
			//why on earth is this HERE???
			//and what does this todo even mean?
			//todo: confirm loss of position information?
			//windowsex_enable<1047>(hWnd,0);
			//break;

		case 45300: //SOM_MAP event programming dialog 

			//Reminder: this dialog cannot Cancel itself
			SendMessage(hWnd,WM_COMMAND,1,0); return 0; //IDOK
		
		case 50000: //SOM_SYS

			SendMessage(hWnd,WM_COMMAND,1000,0); return 0; //Cancel
								 
		case 56200: //slideshow editor 
		
			SendMessage(hWnd,WM_COMMAND,1010,0); return 0; //Cancel		
		
		default: //numerous dialogs

			if(SOM::tool==SOM_MAIN.exe
			 ||SOM::tool==SOM_EDIT.exe) break; //extended dialogs
			
			assert(SOM::tool==SOM_MAP.exe);

			if(GetDlgItem(hWnd,IDCANCEL))
			{
				SendMessage(hWnd,WM_COMMAND,IDCANCEL,0); return 0; 						
			}
			break;

		case 0: assert(som_tool_classic); break; //???
		}
		break;

	case WM_DESTROY:

		if(SOM::tool==SOM_PRM.exe&&hWnd==som_tool_stack[1])
		{
			workshop_exe(0);
		}
		else if(hWnd==som_tool)
		{
			//2023: SOM_MAP big editor isn't getting this message
			if(40000==GetWindowContextHelpId(som_tool))
			{
				PostMessage(0,WM_QUIT,0,0);
			}
		}
		break;

	case WM_NCDESTROY:
	{
		//the default dialog procedure draws the focused button
		LRESULT out = DefSubclassProc(hWnd,uMsg,wParam,lParam);
			
		//REMOVE ME?
		//must manually clear out of som_tool_stack on some systems
		//(not sure if it's slow machines, or old Windows versions)
		som_tool_stack_post_NCDESTROY(hWnd);

		switch(hlp)
		{
		case 42000: 
			
			som_map_tileview = 0;
			som_map_tileviewport = 0; 
			som_map_tileviewinsert = 0;
			break;

		case 45100: 
			
			som_map_event = 0;
			som_map_event_class = 0; 
			break;

		case 30200:

			som_prm_model = 0;
			break;
		}
		RemoveWindowSubclass(hWnd,som_tool_subclassproc,scID);		
		return out; 
		
		/*moved to HCBT_DESTROYWND 
		//support for extended dialogs
		if(WS_CHILD&~GetWindowStyle(hWnd))
		{
			HWND gp = GetParent(hWnd);	
			EnableWindow(gp,1); 
			SetActiveWindow(gp); SetForegroundWindow(gp); 
		}*/
	}}
		
def:return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
static void som_tool_togglemnemonics(HWND of)
{
	for(HWND ch=GetWindow(of,GW_CHILD);ch;ch=GetWindow(ch,GW_HWNDNEXT))	
	{
		int ws = GetWindowStyle(ch);
		if(~ws&WS_VISIBLE) continue;

		if(WS_EX_CONTROLPARENT&GetWindowExStyle(ch))
		{
			som_tool_togglemnemonics(ch);
			continue;
		}

		int dc = SendMessage(ch,WM_GETDLGCODE,0,0);
		
		if(dc&(DLGC_STATIC|DLGC_BUTTON))
		{
			//2018: Trying to keep decorative text from
			//drawing over elements.
			if(~ws&SS_NOPREFIX||dc&DLGC_BUTTON) 
			RedrawWindow(ch,0,0,RDW_INVALIDATE|RDW_ERASE);		
		}
		else if(BS_GROUPBOX==(ws&BS_TYPEMASK))
		{
			//if(som_tool_button()==GetClassAtom(ch))
			assert(som_tool_button()==GetClassAtom(ch));
			RedrawWindow(ch,0,0,RDW_INVALIDATE|RDW_ERASE);		
		}
	}
}
static void som_tool_increment(float delta, HWND em)
{
	//adjust 0.0 style slider
	char c[66],d[66], *e; float f;
	SOM::Tool.GetWindowTextA(em,d,sizeof(d));		
	f = strtod(d,&e)+delta; if(*e) f = 0; 
	e = strchr(d,'.');
	int m = e?strlen(e)-1:0;
	float r = f<0?-0.5f:0.5f;
	sprintf(c,"%f",float(int(f*10000+r))/10000);			
	if(e=strchr(c,'.'))
	{	
		int n = strlen(e)-1;
		while(n>m&&n>1&&e[n]=='0') e[n--] = '\0';			
	}	
	if(strcmp(c,d)) SOM::Tool.SetWindowTextA(em,c);
}
static bool som_tool_slide(HWND em)
{
	assert(som_map_tileviewinsert);

	extern unsigned workshop_antialias; //2021
	workshop_antialias = 1;

	HWND tb = GetWindow(em,GW_HWNDPREV);	
	if(som_tool_trackbar()!=GetClassAtom(tb))
	return false;

	if(GetFocus()==tb) return true;

	int n = SendMessage(tb,TBM_GETRANGEMIN,0,0);
	int x = SendMessage(tb,TBM_GETRANGEMAX,0,0);

	float mf = 0; switch(x)
	{
	case 10: case 200: mf = 0.1f; break;

	case 20: case 60: mf = n?0.05f:0.1f; break; //center/size

	case 72: mf = 5; break; //360 degree angle

	default: mf = 1; //instances/chances
	}

	float f = GetWindowFloat(em);
	if(n==-20&&x==20) //big vertical slider
	if(em==GetDlgItem(som_map_tileviewinsert,1026)) 
	{
		som_tool_slideZ = f;		
		HWND tb2 = GetDlgItem(som_map_tileviewinsert,1036);				
		if(GetFocus()!=tb2)
		SendMessage(tb2,TBM_SETPOS,1,min(max(som_tool_slideZ,-19),19));
		f-=som_tool_slideZ;
	}		
	
	float t = (f-mf*n)/mf;
	int pos = n+int(t+(t>0?0.5f:-0.5f));
	SendMessage(tb,TBM_SETPOS,1,min(max(pos,n),x)); return true;
}
extern void som_tool_toggle(HWND tv, HTREEITEM hit, POINT pt, int how)
{
	NMTREEVIEWW nmtv = //what a chore
	{{tv},0,{0},{TVIF_PARAM|TVIF_STATE,hit},pt};					
	TreeView_GetItem(tv,&nmtv.itemNew);					
	if(nmtv.itemNew.state&TVIS_EXPANDEDONCE)
	{
		switch(nmtv.action=how)
		{
		case TVE_EXPAND: 
			
			if(nmtv.itemNew.state&TVIS_EXPANDED) return;
			break;

		case TVE_COLLAPSE:
			
			if(~nmtv.itemNew.state&TVIS_EXPANDED) return;

		case TVE_COLLAPSE|TVE_COLLAPSERESET: break;

		case TVE_TOGGLE:

			nmtv.action = 
			nmtv.itemNew.state&TVIS_EXPANDED?TVE_COLLAPSE:TVE_EXPAND;		
			break;
		}
		HWND gp = GetParent(tv);
		nmtv.hdr.idFrom = GetDlgCtrlID(tv);
		nmtv.hdr.code = TVN_ITEMEXPANDINGW;		
		if(!SendMessage(gp,WM_NOTIFY,nmtv.hdr.idFrom,(LPARAM)&nmtv)) 
		{
			if(TreeView_Expand(tv,hit,nmtv.action))
			{
				nmtv.hdr.code = TVN_ITEMEXPANDEDW;
				SendMessage(gp,WM_NOTIFY,nmtv.hdr.idFrom,(LPARAM)&nmtv);
			}						
		}
	}
	else TreeView_Expand(tv,hit,how);
}

template<size_t N>		  
static HFONT som_tool_font(const char (&rsrc)[N], const wchar_t (&caption)[N])
{
	HFONT out = 0;
	HRSRC charset = FindResourceA(0,rsrc,(char*)RT_DIALOG);
	if(!charset) return 0;

	HGLOBAL g = LoadResource(0,charset);

	//FFFF0001: DIALOGEX 
	int ex = *(DWORD*)g==0xFFFF0001;
	int style = ((DWORD*)g)[ex?3:0];
	if(style&DS_SETFONT
	&&!wcsicmp((WCHAR*)g+(ex?13:9)+1+1,caption))
	{
		struct ds_setfont //DLGTEMPLATEEX
		{
			WORD pointsize;
			WORD weight;
			BYTE italic;
			BYTE charset;
			WCHAR typeface[1]; //stringLen

		}&sf = *(ds_setfont*)
		((WCHAR*)g+(ex?13:9)+1+1+N);
							  
		//Windows' window manager is
		//using this for ANSI messages
		int cs = SHIFTJIS_CHARSET; //sf.charset
		
		//MS Gothic does not support ClearType
		//Windows 7 does not support ANTIALIASED_QUALITY
		//0 produces ClearType on 7 if the font is compatiable		
		int q = 0; //todo: Editor extension?

		//Hmmm: should SOM be DPI aware?
		//96: GetDeviceCaps(hDC,LOGPIXELSY)
		///*-*/: don't want line height affected
		int h = /*-*/MulDiv(sf.pointsize,96,72);
		EX::INI::Editor ed; 
		if(&ed->height_adjustment)
		{
			//REMINDER: not using som_tool_enlarge
			//because the font needs to be slightly
			//smaller than the one used to size the
			//dialog boxes, but match closely to the
			//height_adjustment
			int h2 = ed->height_adjustment(h,SOM::tool)+0.5f;
			if(h2>h) h = min(h2,h*2);
		}
		out = ex?CreateFontW(h,0,0,0,sf. 
		weight,sf.italic,0,0,cs,0,0,q,0,sf.typeface)
		:CreateFontW(h,0,0,0,0,0,0,0,cs,0,0,q,0,sf.typeface);
	}
	else assert(0); //GlobalFree(g);

	return out;
}
static void som_map_46100_etc(HWND hwndDlg)
{
	//Text+Font
	//these are reversed from the normal sense
	for(int i=1036;i<=1038;i++) //3
	SendDlgItemMessage(hwndDlg,i,TBM_SETRANGE,1/*!*/,MAKELPARAM(0,255));

		//REMINDER: MIGHT NEED TO SET THE FONT UP
		//ANYWAY IF SOM_MAP IS SETTING IT TO GOTHIC
		//AND som_tool_typeset IS NOT MS Gothic

	//PITA
	//SOM_MAP sets up a larger font for event 
	//text (also the font is monospace)
	if(&EX::INI::Editor()->height_adjustment()) 
	{
		HWND hw = GetDlgItem(hwndDlg,1173);			
		#if 0 //can't seem to reliably fit a font
		{
			static HFONT hf = 0; if(!hf)
			{	
				//well, this is depressing
				//(returns 0 even while WM_SETFONT just set
				//the MF font... maybe SOM_MAP is trying to
				//keep it private???)
				hf = GetWindowFont(hw);
				assert(0==hf&&0!=som_map_46100_etc_SETFONT);
				hf = (HFONT)som_map_46100_etc_SETFONT;
						
				LOGFONT lf; GetObject((HANDLE)hf,sizeof(lf),&lf);
				RECT fr; Edit_GetRect(hw,&fr);
				fr.right-=fr.left; fr.bottom-=fr.top;
				lf.lfHeight = /*-*/fr.bottom/7;
				lf.lfWidth = fr.right/40;
				hf = CreateFontIndirect(&lf);
			}
			SetWindowFont(hw,hf,1);
			//Yes/No in branching menu
			SendDlgItemMessage(hwndDlg,1197,WM_SETFONT,(WPARAM)hf,1);
			SendDlgItemMessage(hwndDlg,1198,WM_SETFONT,(WPARAM)hf,1);
		}
		#else //assuming won't interfere with EN_REQUESTRESIZE		
		{
			SelectObject(som_tool_dc,(HFONT)som_map_46100_etc_SETFONT);
			INT w; GetCharWidthI(som_tool_dc,' ',1,0,&w);
			SelectObject(som_tool_dc,som_tool_typeset);
			RECT cr; GetClientRect(hw,&cr);
			cr.right-=som_tool_richtext_margin*2;
			INT m = cr.right%(w*40);
			if(m>=w) //assuming scrollbar will fit into m pixels
			SendMessage(hw,EM_SETMARGINS,EC_LEFTMARGIN|EC_RIGHTMARGIN,
			MAKELPARAM(som_tool_richtext_margin,m-som_tool_richtext_margin));	
		}
		#endif	
	}
}

static DWORD som_tool_id(HWND);
static DLGPROC som_tool_CreateDialogProc = 0;
static void som_map_syncup_prm_and_pr2_files();
static void som_map_code(int code, HWND dialog);
extern windowsex_SUBCLASSPROC workshop_subclassproc;
static INT_PTR CALLBACK som_tool_InitDialog(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(uMsg!=WM_INITDIALOG) 
	{
		//Resource Hacker produces corrupted
		//resources when saving over a network
		//(or even sometimes onto a local drive)
		if(!som_tool_CreateDialogProc)
		return 0; //breakpoint
		return CallWindowProcW((WNDPROC)som_tool_CreateDialogProc,hwndDlg,uMsg,wParam,lParam);
	}
	else //push window onto stack	
	{		
		if(som_tool_dialog_untouched()) //2018: IME fix
		{			
			//HACK... I think som_tool_SetThreadLocale_once
			//should do this, but it doesn't have a window
			//once this is done, IME works for all inputs
			//SendMessage(ed,EM_SETIMEOPTIONS,ECOOP_OR,IMF_FORCEENABLE);
			//seems to be the part of what IMF_FORCEENABLE
			//does that is desirable
			HIMC h = ImmGetContext(hwndDlg);
			ImmSetOpenStatus(h,1); 
			ImmReleaseContext(hwndDlg,h);		
		}

		if(!som_tool_taskbar&&!IsWindow(som_tool)) 
		SOM::initialize_taskbar(hwndDlg);
		som_tool_dialog(hwndDlg); 
	}
	
	//calculate the dialog baseunit factors
	//PGothic fails the documented explanation
	//http://support.microsoft.com/kb/145994 says
	//the x metric is computed over A-Z and a-z only	
	RECT rc = {0,0,4,8}; MapDialogRect(hwndDlg,&rc);
	som_tool_dialogunits = rc;	
	
	if(som_tool&&!som_tool_pushed)	
	if(SOM::tool==SOM_PRM.exe||SOM::tool==SOM_SYS.exe)
	{
		//hack: fake push for first tab 
		som_tool_pushed = GetWindow(som_tool,GW_CHILD);
	}

	DWORD id = som_tool_id(hwndDlg);
	DWORD hlp = GetWindowContextHelpId(hwndDlg);	
	if(id&&hlp!=id) //todo: report noncomforming IDs	
	{
		if(hlp) switch(hlp) //debugging
		{
		case 42200: case 42300: //deprecated
			assert(id==42100); break;
		default: assert(!hlp); 
		}
		SetWindowContextHelpId(hwndDlg,hlp=id); 
	}
	if(som_tool_enlarge!=1&&1==som_tool_enlarge2) 
	{
		//to do this right it seems you'd need to measure
		//the window or font one, both before/after enlarging
		GetClientRect(hwndDlg,&rc); switch(SOM::tool)
		{
		case SOM_PRM.exe:
			som_tool_enlarge2 = rc.right/990.0f;
			break; 
		case SOM_SYS.exe:
			som_tool_enlarge2 = rc.right/768.0f;
			break; 
		case SOM_MAP.exe:
			if(hlp==40000)
			som_tool_enlarge2 = rc.right/993.0f;
			break; 			
		default: som_tool_enlarge2 = som_tool_enlarge;
		}
	}

	if(id==45100)
	{
		som_map_event = hwndDlg; 
		
		extern windowsex_SUBCLASSPROC SOM_MAP_evt_subclassproc;
		SetWindowSubclass(hwndDlg,SOM_MAP_evt_subclassproc,0,0); //2022
	}

	if(!som_tool_charset) //Unicode font
	som_tool_charset = som_tool_font("CHARSET",L"CHARSET");
	if(!som_tool_typeset) //Edit controls
	som_tool_typeset = som_tool_font("TYPESET",L"TYPESET");
	//dunno how Windows manages its fonts??	
	HFONT oldfont = GetWindowFont(hwndDlg); 
	//weirdness
	if(!som_tool_charset) 
	{
	//with the oldstyle tool image's children dialogs or
	//with DIALOG sans EX style windows one. The font is
	//showing up missing. So we just set it to the top's
	//(regardless, the dialogs' units appear correct???)
	LOGFONTW lf; GetObjectW(oldfont,sizeof(lf),&lf);
	lf.lfCharSet = SHIFTJIS_CHARSET;
	som_tool_charset = CreateFontIndirectW(&lf);
	som_tool_charset_height = lf.lfHeight;
	}
	if(!som_tool_typeset)
	som_tool_typeset = som_tool_charset;
	if(som_tool_charset)
	{
		SetWindowFont(hwndDlg,som_tool_charset,0);	
		DeleteObject((HGDIOBJ)oldfont);
	}
	else assert(0);
	if(0==som_tool_space) //2018
	{
		SelectObject(som_tool_dc,som_tool_typeset);
		int w; GetCharWidthI(som_tool_dc,' ',1,0,&w);
		som_tool_space = 0xFF&w;
	}

	if(hlp==46000||!hlp) //Event Instruction Set 
	if(som_tool_help>46000&&som_tool_help<49000)
	{				
		hlp = som_tool_help; 
		wchar_t *bar = som_tool_text;
		if(som_tool_play&&hlp==46100) //comment?
		{
			bar = som_tool_bar(bar,
			GetWindowTextW(hwndDlg,bar,MAX_PATH),1);

			if(-1!=som_tool_play) //map_4660f0_set_text_evt?
			ListBox_SetCurString(som_tool_pushed,bar);
		}
		SetWindowTextW(hwndDlg,bar);
		SetWindowContextHelpId(hwndDlg,som_tool_help);
	}
	som_tool_help = hlp; //2022

	//TODO: move all of this and related
	//window procedures into Sompaste.dll
	HWND ch,ch0 = GetWindow(hwndDlg,GW_CHILD);
	//2018: help language packages with wrapping issues
	SetWindowStyle(ch0,WS_GROUP,WS_GROUP); 
	for(ch=ch0;ch;ch=GetNextWindow(ch,GW_HWNDNEXT))
	{				
		int hlp2 = GetWindowContextHelpId(ch);
		if(hlp2<100&&hlp2>0) 
		SetWindowContextHelpId(ch,hlp+hlp2);

		char ctrl[20] = "", ctrlen =
		GetClassNameA(ch,ctrl,sizeof(ctrl));
		if(*(DWORD*)ctrl==*(DWORD*)"Butt") //Button
		{
			som_tool_button(ch); //includes groupboxes			
		}
		else if(*(DWORD*)ctrl==*(DWORD*)"Stat") //Static
		{
			som_tool_static(ch); //helpful text labels
		}
		else if(*(DWORD*)ctrl==*(DWORD*)"Comb") //ComboBox
		{
			som_tool_combobox(ch); 
		}
		else if(*(DWORD*)ctrl==*(DWORD*)"List") //ListBox
		{
			som_tool_listbox(ch); 
		}		
		else if(*(DWORD*)ctrl==*(DWORD*)"Edit") 
		{
			som_tool_edittext(ch); 
		}
		else if(*(DWORD*)ctrl==*(DWORD*)"RICH")
		{
			som_tool_richtext(ch); 
		}
		else if(*(DWORD*)ctrl==*(DWORD*)"SysL") //SysListView32
		{
			if(ctrl[4]=='i'&&ctrl[5]=='s') //SysLink?
			som_tool_listview(ch); 
		}
		else if(*(DWORD*)ctrl==*(DWORD*)"SysT") //SysTreeView32
		{
			if(ctrl[4]=='r'&&ctrl[5]=='e') //SysTabControl?
			som_tool_treeview(ch); 
		}
		else if(*(DWORD*)ctrl==*(DWORD*)"SysD") //SysDateTimePick32
		{
			som_tool_datetime(ch); 
		}
		else if(*(DWORD*)ctrl==*(DWORD*)"SysI") //SysIPAddress32
		{
			som_tool_ipaddress(ch); 
		}
		else if(*(DWORD*)ctrl==*(DWORD*)"msct") //msctls_
		{
			char *msct_ = ctrl+7; if(ctrlen>7)
			{
				if(*(DWORD*)msct_==*(DWORD*)"trac") //msctls_trackbar32
				{
					som_tool_trackbar(ch);
				}
				else if(*(DWORD*)msct_==*(DWORD*)"updo") //msctls_updown32
				{
					som_tool_updown(ch);
				}		
				else if(*(DWORD*)msct_==*(DWORD*)"prog") //msctls_progress32
				{
					som_tool_progress(ch);
				}
			}
		}
	}		
	//TODO? use numeric dialog names for ID parameter	
	SetWindowSubclass(hwndDlg,SOM::tool>=PrtsEdit.exe?
	workshop_subclassproc:som_tool_subclassproc,0,hlp);
		   
	//Reminder: these go before SOM's DIALOGPROC is installed
	switch(hlp) 
	{
	case 40000: //SOM_MAP

		//ASSUMING EXCLUSIVE ACCESS TO onWrite
		SOM::PARAM::kickoff_write_monitoring_thread(hwndDlg);
		SOM::PARAM::onWrite = som_map_syncup_prm_and_pr2_files;
		//break;
	case 41000: //maps
		extern void som_LYT_open(WCHAR[2]); som_LYT_open(0);
		break; 

	case 42000: //tile viewer
	
		//HACK: the part view is refreshed before the tile view
		//is destroyed
		som_map_tileviewmask|=0x400;
		som_map_tileview = hwndDlg; 

		//2021: prevent ghost image of tile viewer on MSM view?
		DDRAW::fx2ndSceneBuffer = 0;

		//NOTE: SOM_MAP_42000_44000 does further initialization
		break;

	case 42100: //NOTE: includes items/objects/NPCs

		//2022: initialize X/Y fields if they exist so
		//SOM_MAP_rec doesn't fail if Add is used first
		SetDlgItemInt(hwndDlg,1031,0,0);
		SetDlgItemInt(hwndDlg,1033,0,0);		
		break;

	case 46900: //map/warp event instructions

		extern windowsex_SUBCLASSPROC SOM_MAP_141_142_subclassproc;
		SetWindowSubclass(hwndDlg,SOM_MAP_141_142_subclassproc,som_map_instruct,0); 
		break;

	case 47300: if(som_map_instruct!=27) break; //object?
	case 46400: //object/npc/enemy warp event instructions

		extern windowsex_SUBCLASSPROC SOM_MAP_168_172_subclassproc;
		SetWindowSubclass(hwndDlg,SOM_MAP_168_172_subclassproc,som_map_instruct,0); 
		break;

	case 48100: //standby map event instruction
		
		extern windowsex_DIALOGPROC SOM_MAP_140_dlgproc;
		som_tool_CreateDialogProc = SOM_MAP_140_dlgproc; 
		break;
	}
	if(SOM::tool==SOM_SYS.exe) 
	{
		som_sys_openscript = 0; switch(hlp)
		{
		default: som_sys_slideshow = 0; break;
		case 56100: som_sys_slideshow = 1; break;
		case 56200: //slideshow popup translation
		{
			HWND title; if(title=GetDlgItem(hwndDlg,1012))
			ShowWindow(title,!GetWindowTextLengthW(title)); 			
			extern void SOM_SYS_buttonup(HWND); 
			SOM_SYS_buttonup(hwndDlg); //Play?
			break;
		}
		case 53100: som_sys_openscript = GetDlgItem(hwndDlg,1029); break;
		case 55100: som_sys_openscript = GetDlgItem(hwndDlg,1005); break;
		}
	}	
	if(IsWindow(som_tool_pushed)&&som_tool==GetParent(som_tool_pushed))
	{
		int tab = GetDlgCtrlID(som_tool_pushed); switch(tab)
		{
		//items    //shop	  //magic	 //object	//enemy	   //NPC
		case 1001: case 1004: case 1003: case 1007: case 1005: case 1006: 

			if(SOM::tool==SOM_PRM.exe) som_prm_tab = tab; else tab = 0; break;

		//magic    //setup    //classes  //levels	//messages //MOVIE    //etc.
		case 1022: case 1018: case 1017: case 1023: case 1019: case 1020: case 1021:

			if(SOM::tool==SOM_SYS.exe) som_sys_tab = tab; else tab = 0; break;

		default: tab = 0; //NEW: for script button below
		}
		if(tab&&(som_sys_tab==tab||som_prm_tab==tab)&&tab!=1005) 
		{
			som_tool_openscript_enable(som_tool);
		}
	}
	if(SOM::tool>=PrtsEdit.exe&&som_tool==hwndDlg)
	{
		//PrtsEdit's workshop_picture_window is the 3x3 icon
		if(ch=GetDlgItem(som_tool,PrtsEdit.exe==SOM::tool?1022:1014))
		{
			extern RECT &workshop_picture_window;
			GetWindowRect(ch,&workshop_picture_window);
			MapWindowRect(0,som_tool,&workshop_picture_window);		
		}
		if(!workshop_host&&SOM::tool<EneEdit.exe)
		{
			workshop_host = hwndDlg; 
		
			HMENU m = GetMenu(hwndDlg);
			//ResEdit bug.... 
			//it screws up the menus on loading resources
			//so it's easier to just leave the menu unset		
			if(!m) m = LoadMenu(0,MAKEINTRESOURCE(129));
			workshop_menu = m; //assert(m); //PrtsEdit?

			if(SOM::tool==PrtsEdit.exe)
			{
				if(m) SetMenu(hwndDlg,m);
			}
			if(SOM::tool==ItemEdit.exe||SOM::tool==ObjEdit.exe)					
			{
				GetWindowRect(hwndDlg,&rc);
				rc.right-=rc.left; rc.bottom-=rc.top;

				//get IDirect3DDevice7::CreateSurface
				//to make the 3D surface large enough
				//to fill the *new* maximized display
				MoveWindow(ch,0,0,rc.right,rc.bottom,0);

				//REMOVE ME?
				///PART#1 TESTING 3D mouse navigation theory
				//
				// If this is not done/completed below, then
				// mouse-dragging, etc. has no effect on the
				// picture window. The mouse input should be
				// made better than it is; in which point it
				// won't be necessary to do this, but should
				// be documented, somehow/where, if not here.
				{
					//Anything that moves the window off 0,0
					//has the effect, e.g. DS_CENTER or even
					//som_tool_taskbar.			
					//MoveWindow(hwndDlg,0,0,rc.right,rc.bottom,0);
					//BIZARRE! adding workshop_throttleproc to the
					//mix makes the window appear, even if it does
					//aboslutely nothing... but 0,0,0,0 works fine
					MoveWindow(hwndDlg,0,0,0,0,0);					
				}
			}
		}
	}
	SetWindowLongW(hwndDlg,DWL_DLGPROC,(LONG)som_tool_CreateDialogProc);
	DLGPROC swap = som_tool_CreateDialogProc; som_tool_CreateDialogProc = 0;
	//2018: Trying to let subclasses like workshop.cpp process WM_INITDIALOG
	//(towards breaking up som.tool.cpp into SOM_MAP.cpp, SOM_PRM.cpp, etc.)
	#if 0
//	INT_PTR out = CallWindowProcW((WNDPROC)swap,hwndDlg,uMsg,wParam,lParam);
	#else
	INT_PTR out = SendMessageW(hwndDlg,WM_INITDIALOG,wParam,lParam);
	#endif 		//REMOVE ME?
				///PART#2 TESTING 3D mouse navigation theory			
				if(SOM::tool==ItemEdit.exe||SOM::tool==ObjEdit.exe)
				if(som_tool==hwndDlg)
				{
					MoveWindow(hwndDlg,rc.left,rc.top,rc.right,rc.bottom,1);
					
					//FOR CONSISTENCY SAKE
					//1 removes the custom menu item that shows 
					//ObjEdit's copyright/version information
					//(ItemEdit shows the same; calling itself
					//ObjEdit, and EneEdit/NpcEdit don't do so)
					GetSystemMenu(hwndDlg,1);
				}
	
	//Reminder: SOM_RUN is a copy-protected EXE that 
	//is not subject to the language pack process.
	if(SOM::tool==SOM_RUN.exe)	
	{
		//Normally SOM_RUN is run without a SOM file. But if there is one, then
		//it can be preloaded so the end-user doesn't have to enter or locate it.
		SetDlgItemTextW(hwndDlg,1008,wcscpy(som_tool_text,Sompaste->get(L".SOM")));
	}
	if(hlp>46000&&hlp<49000)
	{
		switch(som_map_instruct)
		{
		case 20: som_map_code(0x50,hwndDlg); break;
		case 24: som_map_code(0x54,hwndDlg); break;
		case 28: som_map_code(0x78,hwndDlg); break;
		case 35: som_map_code(0x90,hwndDlg); break;	
		}
	}
	switch(hlp)
	{
	case 10000: //SOM_MAIN

		//remove new button explaining to use SOM_EX.exe
		if(1000==GetDlgCtrlID(GetWindow(hwndDlg,GW_CHILD)))
		{
			DestroyWindow(GetWindow(hwndDlg,GW_CHILD));
		}
		som_main_menu = hwndDlg;
		if(Sompaste->get(L".MO",0)) //compact mode
		if(FindResource(0,MAKEINTRESOURCE(151),RT_DIALOG)) //paranoia
		SendMessage(hwndDlg,WM_COMMAND,1002,0); 
		else Sompaste->set(L".MO",L"");
		break;
				
	case 30200: //SOM_PRM model viewer
	
		som_prm_model = hwndDlg;

		if(WS_SYSMENU&GetWindowStyle(hwndDlg))
		{
			RECT wr; //ax flickering button as last resort
			if(GetWindowRect(GetDlgItem(hwndDlg,1235),&wr))
			if(AdjustWindowRectEx(&wr,GetWindowStyle(hwndDlg),0,GetWindowExStyle(hwndDlg)))
			if(MoveWindow(hwndDlg,wr.left,wr.top,wr.right-wr.left,wr.bottom-wr.top,1))
			if(SendMessage(hwndDlg,DM_SETDEFID,1233,0))
			SetFocus(GetDlgItem(hwndDlg,1233));					
		}
		break;	

	case 31100: //items tab

		extern windowsex_SUBCLASSPROC SOM_PRM_138;
		if(GetDlgItem(hwndDlg,1202))
		{
			SetWindowSubclass(hwndDlg,SOM_PRM_138,0,0);
			extern int SOM_PRM_items_mode;
			windowsex_click(hwndDlg,SOM_PRM_items_mode);
			goto som_prm_extend;
		}
		SOM_PRM_arm.init(); //2021
		break;

	case 33100: //magics tab
		
		//enable translation: magic defaults		
		SetDlgItemTextA(hwndDlg,1225,(char*)4679140);
		SetDlgItemTextA(hwndDlg,1226,(char*)4679128); 
		break;
	
	case 35100: //SOM_PRM Enemy's Wizard screen
		
		SetTimer(hwndDlg,'wiz',0,som_prm_enemywizard);

		//REMOVE ME?. See item() note.
		SOM_PRM_47a708.item() = 0;

		//break;
		
	case 34100: case 36100: goto som_prm_extend;

	case 35200: case 35300: case 35400: //enemy wizard
	
		som_tool_openscript_enable(som_tool);
		if(hlp==35200)
		{
			//UNFINISHED
			//I need to reduce the Kraken size so it
			//can move in its cave
			int todolist[SOMEX_VNUMBER<=0x102040cUL];			
			if(EX::debug) //2021: add 1 decimal place
			{
				//this works but requires reprogramming to save it
				SendDlgItemMessageW(hwndDlg,1036,EM_LIMITTEXT,4,0); //scale
				//worth reprogramming?
			//	SendDlgItemMessageW(hwndDlg,1069,EM_LIMITTEXT,5,0); //radius 
			}
		}
		if(hlp==35300) 
		{			
			som_prm_extend:			
			HWND s = GetDlgItem(hwndDlg,1170), m = GetDlgItem(hwndDlg,1172);
			SendMessage(s,TBM_SETRANGE,1/*!*/,MAKELPARAM(-50,949));
			SendDlgItemMessageW(hwndDlg,1171,EM_LIMITTEXT,3,0); //999
			SendMessage(m,TBM_SETRANGE,1/*!*/,MAKELPARAM(-50,949));
			SendDlgItemMessageW(hwndDlg,1173,EM_LIMITTEXT,3,0); //999	
			extern windowsex_SUBCLASSPROC SOM_PRM_strengthproc;
			SetWindowSubclass(s,SOM_PRM_strengthproc,(DWORD_PTR)m,0);
			SOM_PRM_extend();
		}
		break;

	case 40000: //SOM_MAP
				
		//REMOVE ME? (chicken/egg)
		extern void SOM_MAP_40000(); SOM_MAP_40000();
		break;	

	case 41000: //maps

		if(som_tool_refocus) //via layer menu?
		{			
			//TODO: can call 4130E0 instead I think
			//TODO: can call 4130E0 instead I think
			//TODO: can call 4130E0 instead I think
			//0041AF7C E8 5F 81 FF FF       call        004130E0

			//??? why doesn't SendMessage take? it can't
			//be the the item is not highlighted, because 
			//windowsex_notify works below
			//SendDlgItemMessage(hwndDlg,1239,LB_SETCURSEL,
			lParam = ComboBox_GetCurSel(som_tool_refocus);
			PostMessage(GetDlgItem(hwndDlg,1239),LB_SETCURSEL,
			ComboBox_GetItemData(som_tool_refocus,lParam),0);
			//SendMessage(hwndDlg,WM_COMMAND,IDOK,0);
			PostMessage(hwndDlg,WM_COMMAND,IDOK,0);
		}
		else windowsex_notify(hwndDlg,1239,LBN_SELCHANGE);
		break;
			   
	case 42000: //tile editor
		assert(som_map_tileview==hwndDlg);
		if(!som_tool_classic) //NEW
		som_map_tileviewport = GetWindow(ch0,GW_HWNDLAST); 
		//break;
	case 44000: //map settings		
		extern void SOM_MAP_42000_44000(HWND); SOM_MAP_42000_44000(hwndDlg);
		break;

	case 42100: 
	case 42200: case 42300: //deprecated
	
		if(som_tool_classic) break;

		SendDlgItemMessage(hwndDlg,1028,TBM_SETRANGE,1/*!*/,MAKELPARAM(0,360));
		SendDlgItemMessage(hwndDlg,1030,TBM_SETRANGE,1/*!*/,MAKELPARAM(0,360));		
			
		som_map_tileviewinsert = hwndDlg;
		SendDlgItemMessage(hwndDlg,1036,TBM_SETRANGE,1/*!*/,MAKELPARAM(-19,19));
		SendDlgItemMessage(hwndDlg,1036,TBM_SETPOS,1/*!*/,0);
		for(int i=1083;i<=1085;i++) //3
		SendDlgItemMessage(hwndDlg,i,UDM_SETRANGE32,0,360);
		SendDlgItemMessage(hwndDlg,1087,UDM_SETRANGE32,1,255);
		SendDlgItemMessage(hwndDlg,1088,UDM_SETRANGE32,0,100);
		if(GetDlgItem(hwndDlg,1036)) //meters in increments of .05
		{		
			for(int i=1022;i<=1024;i++) //3
			SendDlgItemMessage(hwndDlg,i,TBM_SETRANGE,1/*!*/,MAKELPARAM(-20,20));
			SendDlgItemMessage(hwndDlg,1034,TBM_SETRANGE,1/*!*/,MAKELPARAM(+20,60));	
			for(int i=1025;i<=1027;i++) //3
			SendDlgItemMessageW(hwndDlg,i,EM_LIMITTEXT,8,0); //-99.9999
			SendDlgItemMessageW(hwndDlg,1035,EM_LIMITTEXT,8,0);
		}
		if(GetDlgItem(hwndDlg,1084)) //degrees in increments of 5
		for(int i=1028;i<=1030;i++) //3
		SendDlgItemMessage(hwndDlg,i,TBM_SETRANGE,1/*!*/,MAKELPARAM(0,72));

		break;

	case 45000: //events
	
		//Paste: extension fills up the battery
		SendMessage(hwndDlg,WM_COMMAND,1047,0);
		break;
	
	case 45100: //130: event editor
	{
		const size_t x_s = 64; WCHAR x[x_s] = L""; 

		HWND description = GetDlgItem(hwndDlg,1024);

		GetWindowTextW(description,x,x_s);
		
		int n=-1, c, p;
		if(swscanf(x,L"@%d-%x-%x",&n,&c,&p)!=3) n = -1;
			
		HWND src = GetParent(hwndDlg);
		int src_hlp = GetWindowContextHelpId(src); //debugging
		switch(src_hlp)
		{
		case 45000: //173: events menu
		
			//NEW: overwriting in case n is a copy??
			n = SendDlgItemMessage(src,1210,LB_GETCURSEL,0,0);
			break;
		
		case 42000: //174: tile editor		

			if(n==-1)
			{
				*x = '\0'; //ensure translated below

				n = som_map_zentai_new(); 
			}
			else //NEW: in case of copy??
			{
				HWND tv = GetDlgItem(src,1052);				
				TVITEMW ti = {TVIF_TEXT,TreeView_GetSelection(tv),0,0,x,x_s};
				SendMessageW(tv,TVM_GETITEMW,0,(LPARAM)&ti);
				int i = 0; while(!isdigit(x[i])&&x[i]) i++;
				n = _wtoi(x+i);
			}
			break;

		default: assert(0);
		} 
		
		if(n>9) //regular use
		{
			bool paranoia = !*x; //NEW:
			//opening to an empty name is better anyway
			//if left blank SOM fills it in with untranslatable text
			som_map_zentai_name(n,x,x_s); assert((!*x||n==-1)==paranoia); 
			/*if(!*x||n==-1) //assuming new event
			{				
				HWND next = som_tool_x(description);
				if(next) GetWindowTextW(next,x,x_s); 
				else EX::need_unicode_equivalent(932,(char*)0x48F404,x,x_s);
			}
			else som_map_zentai_name(n,x,x_s);*/
		}
		else //special use: enable translation
		{
			if(SendDlgItemMessageW(src,1210,LB_GETTEXT,n,(LPARAM)x))
			{
				//WARNING: potentially disasterous
				if(!swscanf(x,L"[%*04d]  %*lc%*lc%*lc  %ls",x)) assert(0);
			}
			else assert(0);
		}

		if(n<10) //special use events
		{
			if(c==0xFD) //Systemwide
			SendDlgItemMessage(hwndDlg,1199,CB_SETCURSEL,1,0);
		}
		else if(c==0xFE||c==0xFD) //System/Systemwide
		{
			int item = 
			SendDlgItemMessage(hwndDlg,1044,CB_GETCURSEL,0,0);	 
			int sel = c==0xFE?3:4; //class
			SendDlgItemMessage(hwndDlg,1199,CB_SETCURSEL,sel,0);

			switch(p) //protocol
			{
			case 0x20: sel = 0; break; //continuous evaluation					
			case 0x40: sel = 1; break; //item based 

			default: sel = CB_ERR; //not good
			}					
			SendDlgItemMessage(hwndDlg,1014,CB_SETCURSEL,sel,0);								
			SendDlgItemMessage(hwndDlg,1044,CB_SETCURSEL,item,0);

			//2018: Hide NPC selection. This or C8N_SELCHANGE.
			SendDlgItemMessage(hwndDlg,1213,CB_SETCURSEL,-1,0);
		}

		SetWindowTextW(description,x);
		if(som_map_event_is_system_only)
		som_tool_openscript_enable(hwndDlg,0); //REMOVE ME?
		som_map_event_slot = n; //!

		break;
	}	
	case 45300: //Program
		
		//2018: Prevent inserting no-selection edge case so it doesn't
		//have to be handled.
		for(int i=1026;i<=1029;i+=3) 
		SendDlgItemMessage(hwndDlg,i,WM_KEYDOWN,VK_HOME,0);
		break;

	case 46100: //Text+Font 
	case 47700: //Branch?

		som_map_46100_etc(hwndDlg);
		break;

	case 49100: //skip prepare MPX window?

		//Note: this will close the dialog before CreateDialog finishes
		//It works. Not sure what it returns. Note: It also works before
		//SOM_MAP's DIALOGPROC is installed
		if(0==som_tool_play) 
		{
			extern windowsex_SUBCLASSPROC SOM_MAP_165;
			if(GetDlgItem(hwndDlg,5))
			{
				SetWindowSubclass(hwndDlg,SOM_MAP_165,0,*(WORD*)&SOM_MAP_4921ac.masking);
				if(hwndDlg!=som_tool_stack[1])
				{
					CheckDlgButton(hwndDlg,1122,SOM_MAP_4921ac.masking=1);
					CheckDlgButton(hwndDlg,1123,SOM_MAP_4921ac.lighting=1);
				}
				CheckDlgButton(hwndDlg,1001,som_map_tileviewmask&0x100?1:0);

				CheckDlgButton(hwndDlg,2022,1); //2021: Compile disable option?
			}
		}
		else SendMessage(hwndDlg,WM_COMMAND,IDOK,0);
		break;

	case 52100: //SOM_SYS player characters

		//PRETTY (LVS_SHOWSELALWAYS)
		//Is SOM_SYS selecting the first item??? 
		//Or is SOM_PRM unselecting it?? Unselect it.
		ListView_SetItemState(GetDlgItem(hwndDlg,1015),0,0,-1);
		break;

	case 57100:
	
		//these sliders/boxes don't work properly
		windowsex_notify(hwndDlg,1019,EN_CHANGE); //walk
		windowsex_notify(hwndDlg,1020,EN_CHANGE); //dash
		break;

	case 61000: //ItemEdit
		//HACK: ItemEdit redraws every frame in both modes?!
		extern windowsex_SUBCLASSPROC workshop_throttleproc;
		ch = GetDlgItem(hwndDlg,1014);
		RemoveWindowSubclass(ch,som_tool_buttonproc,1014);
		SetWindowSubclass(ch,workshop_throttleproc,1014,61000);
		//break; 
	case 60000: case 62000: //case 61000:
	
		if(1<GetEnvironmentVariableW(L".WS",0,0))
		{
			SendMessage(hwndDlg,WM_COMMAND,32771,0);
			break;
		}
		//break; 
	case 63000: case 64000: case 65000: //workshop_32772
		//Reminder: this ensures everything initializes
		//and is consistent with 32772
		if(1>=GetEnvironmentVariableW(L".WS",0,0))
		SendMessage(hwndDlg,WM_COMMAND,32772,0); 
		break;

	case 61100: //HACK: opening dummy model?			  
	case 62100:
	case 63100: //skin?	
		lParam = hlp==63100&&*som_tool_text?2:0;
		SendDlgItemMessage(hwndDlg,lParam?1016:1013,LB_ADDSTRING,0,
		(LPARAM)PathFindFileName(som_tool_text));
		//HACK: ObjEdit calls workshop_SetWindowTextA when
		//changing the model??
		SendDlgItemMessage(hwndDlg,lParam?1016:1013,LB_SETCURSEL,lParam,0);
		*som_tool_text = '\0';
	case 60100: //HACK: opening dummy "part"
		SendMessage(hwndDlg,WM_COMMAND,IDOK,0);
		break;
	}

	//NEW: Remember selection?
	if(SOM::tool==SOM_PRM.exe&&som_tool==GetParent(hwndDlg))
	{
		SOM::xxiix ex;
		ex = GetWindowLong(GetDlgItem(som_tool,som_prm_tab),GWL_USERDATA);		
		if(!ex.x1) //NEW: initialize?
		{
			//HACK: assuming listbox is first!
			ch0 = som_tool_boxen.boxen[0].first;
			std::vector<wchar_t> &v = som_tool_boxen.boxen[0].second;
			if(!v.empty()&&!v[0]&&GetClassAtom(ch0)==som_tool_listbox())
			for(size_t i=1;i<v.size();i++) if(v[i])
			{
				ex.x1 = GetDlgCtrlID(ch0)-1000&1023;
				ex.x2 = ex.x3 = i&1023; 
				goto init_som_prm_tab;
			}
		}
		else 
		{
			ch0 = GetDlgItem(hwndDlg,ex.x1+1000);
			init_som_prm_tab:
			ListBox_SetTopIndex(ch0,ex.x2);
			if(ex&&LB_ERR!=ListBox_SetCurSel(ch0,ex.x3))
			SendMessage(hwndDlg,WM_COMMAND,MAKEWPARAM(ex.x1+1000,LBN_SELCHANGE),(LPARAM)ch0);
		}
	}

	if(som_tool_refocus)
	{
		SetFocus(som_tool_refocus);	
		
		som_tool_refocus = 0; return 0;
	}
	else return out;
}

static bool som_tool_ok(HWND dlg)
{
	//this is in response to an OK button

	DWORD hlp = GetWindowContextHelpId(dlg);
	if(!hlp) return true; 

	if(SOM::tool==SOM_MAP.exe) switch(hlp)
	{	
	default:

		if(som_map_tileview)
		{
			if(45100!=hlp) //not event?
			if(som_map_tileview==GetParent(dlg))
			{
				//object profile setup?
				windowsex_enable<1047>(som_map_tileview);
				break;
			}

			if(GetKeyState(VK_RETURN)>>15
			&&som_tool_dialog()==som_map_tileviewinsert)
			{
				//Shift+Enter to Add/Copy otherwise Record
				int bt = GetKeyState(VK_SHIFT)>>15?1045:1047;			
				if(IsWindowEnabled(GetDlgItem(dlg,bt))) SendMessage(dlg,WM_COMMAND,bt,0);
				else MessageBeep(-1); return false;
			}
		}
		break;

	case 42000: //tile editor
		
		extern bool SOM_MAP_tileview_1047();
		if(!SOM_MAP_tileview_1047()) 
		return false;

		//HACK: the part view is refreshed before the tile view
		//is destroyed
		som_map_tileviewmask&=~0x400;

		//2021: prevent ghost image of tile viewer on MSM view?
		DDRAW::fx2ndSceneBuffer = 0;

		break;

	case 44000: //map settings (z-index)
	{	
		DefSubclassProc(dlg,WM_COMMAND,IDOK,0);		
		//support ahistorical heights
		SOM_MAP_4921ac.start.xzy[1] = GetDlgItemFloat(dlg,1030);
		SOM_MAP_4921ac.start.record_z_index(dlg);
		return false;
	}
	case 45100: //130: event editor
	{
		if(som_map_event_slot<0
		 ||som_map_event_slot>=1024) return true;

		LockWindowUpdate(dlg);

		const size_t x_s = 32; WCHAR x[x_s] = L""; 

		GetDlgItemTextW(dlg,1024,x,x_s);
		
		if(!*x) //ensure no-name events are deleted events
		{
			char *new_event = (char*)0x48F404;
			SetDlgItemTextA(dlg,1024,*new_event?new_event:"?");
			GetDlgItemTextW(dlg,1024,x,x_s);
		}

		som_map_zentai_rename(som_map_event_slot,x,x_s);

		HWND cwin = GetDlgItem(dlg,1199); //class
		HWND pwin = GetDlgItem(dlg,1014); //protocol
		HWND iwin = GetDlgItem(dlg,1044); //item

		int n = som_map_event_slot; som_map_event_slot = -1;
		int c = SendMessage(cwin,CB_GETCURSEL,0,0);
		int p = SendMessage(pwin,CB_GETCURSEL,0,0);

		if(n<10) //special use events
		{
			SendMessage(cwin,CB_SETCURSEL,0,0); 
			SendMessage(dlg,WM_COMMAND,MAKEWPARAM(1199,CBN_SELENDOK),(LPARAM)cwin);

			c = !c?0xFE:0xFD; //System/Systemwide
		}
		else if(c==3||c==4) //ditto
		{
			c = c==3?0xFE:0xFD;

			int item = 
			SendMessage(iwin,CB_GETCURSEL,0,0);	 		
			SendMessage(cwin,CB_SETCURSEL,0,0); 
			SendMessage(dlg,WM_COMMAND,MAKEWPARAM(1199,CBN_SELENDOK),(LPARAM)cwin);
			SendMessage(pwin,CB_SETCURSEL,p,0);
			SendMessage(dlg,WM_COMMAND,MAKEWPARAM(1014,CBN_SELENDOK),(LPARAM)pwin);
			SendMessage(iwin,CB_SETCURSEL,item,0);	 
			SendMessage(dlg,WM_COMMAND,MAKEWPARAM(1044,CBN_SELENDOK),(LPARAM)iwin);

			p = !p?0x20:0x40; //continuous evaluation/item
		}		
		else //todo: assign protocols to CB_SETITEMDATA?
		{
			if(c==2&&p>1) p++; //skip Defeat protocol for objects

			switch(p)
			{
			case 0: p = 0x01; break; //button
			case 1: p = 0x02; break; //use item
			case 2: p = 0x10; break; //npc defeat
			case 3: p = 0x04; break; //square zone
			case 4: p = 0x08; break; //circular zone
			case 5: p = 0x20; break; //continuous eval
			case 6: p = 0x40; break; //use item anywhere

			//TODO? Might want to disable the OK button?
			default: assert(0); case -1: p = 0; 
			}
		}
		if(p<0) p = 0; if(c<0) c = 0;
		if(swprintf(x,L"@%04d-%02x-%02x",n,c,p)!=11) assert(0);

		if(n>=10) //regular events
		{
			SetDlgItemTextW(dlg,1024,x);	
		}
		else if(som_map_event_memory) //special events
		{
			//SOM_MAP doesn't readback the description for these
			if(som_map_event_memory&&'@'==*som_map_event_memory//)
			||!*som_map_event_memory/*pasted over with blank?*/)
			for(size_t i=0;i<31;i++) som_map_event_memory[i] = x[i];
			else assert(0);

			som_map_event_memory = 0;
		}
		break;
	}}
	return true;
}

//SOM_MAP isn't centering, maybe som_tool_taskbar???
extern void som_tool_recenter(HWND dialog, HWND over, int edge=0)
{		
	RECT a, b; if(edge=='nc') //HACK/NEW
	{
		GetWindowRect(over,&a); edge = 0; goto nc;
	}
	GetClientRect(over,&a); //GetWindowRect
	MapWindowRect(over,0,&a); //returns bogus values on Aero
nc:	GetWindowRect(dialog,&b); OffsetRect(&b,-b.left,-b.top);
	int cx,cy = a.top+(a.bottom-a.top)/2; switch(edge) //NEW
	{	
	case +1: cx = a.left+10; break;
	case -1: cx = a.right-10-b.right; break;
	case 0: cx = a.left+(a.right-a.left)/2-b.right/2; //centered
	}
	MoveWindow(dialog,cx,cy-b.bottom/2,b.right,b.bottom,0);
	SendMessage(dialog,DM_REPOSITION,0,0);
}

extern const wchar_t *som_tool_richedit_class()
{
	static HMODULE one_off = LoadLibraryA("MSFTEDIT.DLL");
	return L"RICHEDIT50W";
}
static void som_tool_richedit_etc(void *dlgtemplateex)
{
	struct pt1 //DLGTEMPLATEEX
	{
    WORD dlgVer, signature; DWORD helpID, exStyle, style;	
	WORD cDlgItems; short x, y, cx, cy; //DWORD menu, windowClass;
	//WCHAR title[32];
	WCHAR menu_windowClass_title[32]; //2018
	};
	struct pt2 //DLGTEMPLATEEX
	{
    WORD pointsize, weight; BYTE italic, charset; WCHAR typeface[32];
	};
	
	int compile[sizeof(WCHAR)==sizeof(WORD)];

	pt1 &dlg = *(pt1*)dlgtemplateex; //2018
	WCHAR *p = ((pt1*)dlgtemplateex)->menu_windowClass_title; 
	for(int i=0;i<2;i++) if(*p++) //2018
	{
		if(*p!=0xFFFF) while(*p!='\0') p++; p++;
	}		
	extern const wchar_t* &workshop_title;
	if(SOM::tool>=PrtsEdit.exe&&!workshop_title)
	workshop_title = p;
	EX::INI::Editor ed;
	WCHAR *pp; DWORD rsrc = &ed->height_adjustment?1:0;
	for(pp=p;*p;p++)
	{
		 /*REMINDER: ResEdit injects \ in title bar with 
		//escaped characters, but fixing it here doesn't
		//help because it still adds to the titlebar each
		//time it's saved, which eventually adds up!
		//if('\\'==*p) rsrc = 1;*/
	}
	assert(!*p); p++; 

		WORD &ps = ((pt2*)p)->pointsize;
		//assuming all dialogs use the same font/size
		static int once_per_resource = ps;
		if(ps==once_per_resource)
		if(rsrc&&VirtualProtect(pp,p-pp,PAGE_READWRITE,&rsrc))
		{
			//std::fill(std::remove(pp,p-1,L'\\'),p-1,' ');

			if(&ed->height_adjustment)
			{
				int was = MulDiv(ps,96,72);
				int now = ed->height_adjustment(was,SOM::tool)+0.5f;
				if(now>was) 
				ps = MulDiv(min(now,was*2),72,96);
				now = MulDiv(ps,96,72);
				som_tool_enlarge = float(now)/was;
			}

			VirtualProtect(pp,p-pp,rsrc,&rsrc); 
		}		

	if((DS_SETFONT)&((pt1*)dlgtemplateex)->style)
	{	
		p = ((pt2*)p)->typeface; while(*p++);
	}
	for(int i=0,n=((pt1*)dlgtemplateex)->cDlgItems;i<n;i++)
	{
		if(1&(p-(WCHAR*)dlgtemplateex)) p++; //DWORD align

		struct it //DLGITEMTEMPLATEEX
		{
		DWORD helpID, exStyle, style;
		SHORT x, y, cx, cy;	DWORD id; WCHAR windowClass_title[32];
		}&ch = *(it*)p;

		WCHAR *wc = ch.windowClass_title; int watch = wc[1];

		rsrc = 0; if(0xFFFF==wc[0]) //piggybacking: etc.
		{
			//som_tool_mirrorboxproc
			//TODO? Maybe add CBS_OWNERDRAWFIXED to SOM_SYS's
			//sound effects dropdowns. They are so slow. Some
			//other dropdowns are mirrors also like the magic
			//on SOM_PRM's item screen.

			/*//piggybacking: singleline EDIT controls flicker
			if(wc[1]==129||!wcsnicmp(wc,L"EDIT",5)) 
			{
				DWORD &es = ((it*)p)->style, rsrc; 
				if(WS_EX_TRANSPARENT&((it*)p)->exStyle)
				if(VirtualProtect(p,sizeof(it),PAGE_READWRITE,&rsrc))
				{
					es|=ES_READONLY|ES_MULTILINE;				
					VirtualProtect(p,sizeof(it),rsrc,&rsrc); 
				}
			}*/
			
			//2018: som_tool_bar texts
			if(wc[1]==0x0082 //Static
			||wc[1]==0x0080&&BS_GROUPBOX==(ch.style&BS_TYPEMASK))
			switch(dlg.helpID)
			{
			case 31100: //items

				extern const WCHAR *SOM_PRM_items_titles[3];
				if(ch.id>=1199&&ch.id<=1201)
				SOM_PRM_items_titles[ch.id-1199] = wc+2; 				
				break;

			case 33100: //magic

				extern const WCHAR *SOM_PRM_magic_titles[3];
				if((ch.id==1226||wc[1]==0x0080)&&wcschr(wc+2,'|'))
				{
					if(ch.id>=1224&&ch.id<=1226)
			legacy:	SOM_PRM_magic_titles[ch.id-1224] = wc+2; 									
					else if(!SOM_PRM_magic_titles[0]
					&&VirtualProtect(p,sizeof(it),PAGE_READWRITE,&rsrc))
					{
						//Old language pack support. Can maybe remove
						//this in a few years time
						ch.id = 1224; goto legacy;
					}
				}
				break;			
			}
			else if(wc[1]==0x0083) //ListBox
			{
				//REMINDER: som_tool_DrawFocusRect is drawing the 
				//yellow focus text (there's no CUSTOMDRAW setup)

				//2022: I think it would be nice to draw comments
				//in SOM_MAP with a light gray background someday
				//https://stackoverflow.com/questions/91747/background-color-of-a-listbox-item-windows-forms

				switch(MAKELONG(dlg.helpID,ch.id)) //LBS_EXTENDEDSEL?
				{
				case MAKELONG(41000,1239): //maps
				//doing a limited rollout
				//case MAKELONG(45300,1026): //program
				//case MAKELONG(45000,1210): //events
				if(VirtualProtect(p,sizeof(it),PAGE_READWRITE,&rsrc))										
				ch.style|=LBS_EXTENDEDSEL;
				}				
			}
		}
		else if(!wcsnicmp(wc,L"RichEdit",8)&&wc[8]) //load v4.1 library
		{
			DWORD &es = ((it*)p)->style, rsrc; 
			if(VirtualProtect(p,sizeof(it),PAGE_READWRITE,&rsrc))
			{
				//these can't seem to be enabled once created
				if(es&ES_MULTILINE) es|=WS_VSCROLL|WS_HSCROLL;						
				if(~es&ES_AUTOVSCROLL) es|=ES_DISABLENOSCROLL;
				es|=ES_NOOLEDRAGDROP; 

				//This seems to be for non-unicode file addresses.
				es&=~ES_OEMCONVERT;

				static bool RICHEDIT50W = 
				!wcscmp(L"RICHEDIT50W",som_tool_richedit_class());
				assert(RICHEDIT50W);
				//
				if(wcscmp(wc,L"RICHEDIT50W")) //upgrade to 4.1									
				if(wmemcpy(wc,L"RICHEDIT",8)[8]=='2') wc[8] = '5';				
			}			
		}		
		if(rsrc) VirtualProtect(p,sizeof(it),rsrc,&rsrc); 

		if(*wc==0xFFFF) wc+=2; else while(*wc++); //windowClass
		if(*wc==0xFFFF) wc+=2; else while(*wc++); //title

		WORD extraCount = *wc; 		
		//docs say WORD aligned (they may mean DWORD)
		BYTE *q = (BYTE*)wc+2+extraCount;

		if(1&(DWORD)q) q++; //WORD align
		p = (WCHAR*)q; 
	}
}

static void som_tool_SetThreadLocale_once()
{
	if(!som_tool_dialog_untouched()) return;

	//2018: SetThreadLocale changes the input locale
	//which is undesirable! 
	//so this is restoring it, so the IME opens with
	//the locale
	HKL kl = GetKeyboardLayout(GetCurrentThreadId());

	//static BOOL ja_JP = //NEW
	//--this just prevents multilingual resources--
	//(not so; It corrects NpcEdit's GetOpenFileName's filter text)
	//The true codepage seems to be CreateFont's charset
	SetThreadLocale(MAKELANGID(LANG_JAPANESE,SUBLANG_JAPANESE_JAPAN)); 

	//2018: restore kl
	//LoadKeyboardLayout(kl,KLF_ACTIVATE);
	ActivateKeyboardLayout(kl,KLF_SETFORPROCESS);

	//HACK: currently som_tool_InitDialog is kicking the IME into gear
	//should do that here, but an HWND is required
	//HIMC h = ImmGetContext(?);
	//ImmSetOpenStatus(h,1);
	//ImmReleaseContext(?,h);
}

static wchar_t som_tool_classname[MAX_PATH+64] = L"";
static HWND WINAPI som_tool_CreateDialogIndirectParamA(HINSTANCE A, LPCDLGTEMPLATEA B, HWND C, DLGPROC D, LPARAM E) 
{		
	SOM_TOOL_DETOUR_THREADMAIN(CreateDialogIndirectParamA,A,B,C,D,E)

	EXLOG_LEVEL(7) << "som_tool_CreateDialogIndirectParamA()\n";
	
	if(A&&A!=(HINSTANCE)0x00400000) 
	return SOM::Tool.CreateDialogIndirectParamA(A,B,C,D,E);
	else A = (HINSTANCE)0x00400000;

	som_tool_SetThreadLocale_once();
		
	typedef struct {
    WORD dlgVer;
    WORD signature;
    DWORD helpID;
    DWORD exStyle;
    DWORD style;
    WORD cDlgItems;
    SHORT x;
    SHORT y;
    SHORT cx;
    SHORT cy; //26+2
	WCHAR variable[32];
    /*sz_Or_Ord menu;
    sz_Or_Ord windowClass;
    WCHAR title[titleLen];
    WORD pointsize;
    WORD weight;
    BYTE italic;
    BYTE charset;
    WCHAR typeface[stringLen];*/
	}DLGTEMPLATEEXA;
	DLGTEMPLATEEXA *peek = (DLGTEMPLATEEXA*)B; //testing*/

	bool recenter = false; //REMOVE ME?

	//FFFF0001: DIALOGEX w/ helpID
	if(B->style==0xFFFF0001&&peek->helpID
	&&2000!=EX::INI::Editor()->time_machine_mode)
	{	
		som_tool_classic = false; 
		
		som_tool_richedit_etc(peek);
		
		if(som_tool_find) //override
		{				
			HRSRC special = //ization
			FindResourceA(A,som_tool_find,(char*)RT_DIALOG);	
			DWORD debug = GetLastError();
			if(special)					
			B = (LPCDLGTEMPLATE)LoadResource(A,special);
			peek = (DLGTEMPLATEEXA*)B;
			if(!special||!B) som_tool_find = 0;
			if(!B||B->style!=0xFFFF0001) return 0;
		}
		DWORD rsrc; //might want to just make .rsrc writable
		if(VirtualProtect((void*)B,16,PAGE_READWRITE,&rsrc))
		{			
			if(som_tool_f1)
			peek->exStyle|=WS_EX_CONTEXTHELP;
			if(!som_tool_f1)
			peek->exStyle&=~WS_EX_CONTEXTHELP; //patch

			//Windows. Go figure???
			static std::set<const void*> cm;
			{
				if(cm.find(B)!=cm.end())			
				peek->style|=DS_CENTERMOUSE;
				if(peek->style&DS_CENTERMOUSE)
				{
					HWND f = GetFocus();
					som_tool_paint(1,f,UIS_INITIALIZE);
					if(f&&UISF_HIDEFOCUS&~SendMessage(f,WM_QUERYUISTATE,0,0))
					{
						peek->style&=~DS_CENTERMOUSE;
						cm.insert(B);
					}
				}
			}
			
			switch(peek->helpID)
			{
			case 40100: //SOM_MAP startup

				if(40100==peek->helpID)
				{
					static bool one_off = false; //???
					if(!one_off++)
					peek->exStyle|=WS_EX_APPWINDOW;
					else
					peek->exStyle&=~WS_EX_APPWINDOW;
				}
				//break;

			//top level windows
			//case 30000: case 30100:
			//case 40000: case 40100:
			default:

				if(~peek->style&WS_CHILD)
				peek->style|=WS_CAPTION|WS_SYSMENU|DS_MODALFRAME;
				else peek->style|=WS_GROUP; //helpful?
				break;

			//SOM uses overlay popup windows where
			//child windows should be used instead
			case 31100: case 32100: case 33100:	//SOM_PRM
			case 34100:	case 35100:	case 36100:
			case 35200: case 35300: case 35400:						
			case 51100: case 52100: case 53100: //SOM_SYS	
			case 54100:	case 55100:	case 56100: case 57100:
							
				//NOTE we don'te leave this to language packs so 
				//that SOM's EXE files won't look bad without Ex

				peek->style&=~WS_POPUP;
				peek->style|=DS_CONTROL|WS_CHILD;  		
				peek->exStyle|=WS_EX_CONTROLPARENT;
				//peek->exStyle|=WS_EX_TRANSPARENT;	//old way
				peek->exStyle&=~WS_EX_TRANSPARENT;
				peek->style|=WS_GROUP; //helpful?
				break;

			case 42100:
			case 42200: case 42300: //deprecated

				//SOM_MAP tile screen panels
				peek->exStyle|=WS_EX_TRANSPARENT;
				break;
			}
					  
			//is this just for taskbar???
			//doesn't work with SOM_MAP's
			//loading window so IDD_DUMMY
			//must be made tiny as can be
			if(!C) peek->style|=DS_CENTER;
			
			//NEW: sceduled obsolete
			if(SOM::tool==SOM_MAP.exe)			
			if(!(peek->style&(WS_CHILD|DS_CENTERMOUSE|DS_CENTER)))
			{
				recenter = true; //likely taskbar related as well
			}			
			
			if(workshop_tool&&!C)
			if(SOM::tool>=PrtsEdit.exe) //2023
			{				
				C = workshop_tool;
				peek->exStyle&=~WS_EX_APPWINDOW;
				auto *ws = Sompaste->get(L".WS");
				if(*ws)
				{
					peek->style|=WS_MINIMIZEBOX;
					peek->style&=~WS_MAXIMIZEBOX;
				}
				if(wcsstr(ws,L"\\my\\arm\\")) //workshop_moves_32767?
				{
					peek->style&=~DS_CENTER;
					peek->style|=DS_CENTERMOUSE;
				}
			}
			else assert(0);

			if(1) //EXPERIMENTAL
			//NEW: taskbar will not minimize without a minimize
			//button?!
			//Actually this seems to confuse
			//SOM_MAP, in that it assigns parentage
			//one window lower than normal in the stack???
			//(NEW: this is repaired in the else if block below)
			if(!C&&WS_MINIMIZEBOX&~peek->style)
			{	
				HOOKPROC hack = 0;
				std::swap(hack,som_tool_cbt);
				//reminder: must be first window
				if(!som_tool_taskbar) if(!*som_tool_classname)
				{						
					assert(SOM::tool==SOM_MAIN.exe //no mutex
					||SOM::tool>=PrtsEdit.exe);
					som_tool_taskbar = 				
					CreateDialogW(EX::module,MAKEINTRESOURCEW(IDD_DUMMY),0,som_tool_taskbarproc); 								
				}
				else //2017: Trying to synchronize with OpenMutex :(
				{
					//The custom class name is used with FindWindow
					WNDCLASSEXW wc; wc.cbSize = sizeof(WNDCLASSEX);
					GetClassInfoExW(0,WC_DIALOG,&wc);
					wc.lpszClassName = som_tool_classname;
					wc.style&=~CS_GLOBALCLASS;
					ATOM test = RegisterClassExW(&wc);
					som_tool_taskbar = CreateWindowEx(
					WS_EX_APPWINDOW|WS_EX_DLGMODALFRAME,
					som_tool_classname,0,
					//IDD_DUMMY's styles
					DS_MODALFRAME | DS_SETFOREGROUND | DS_FIXEDSYS | DS_NOFAILCREATE | DS_CENTER | WS_MINIMIZEBOX | WS_POPUP | WS_CAPTION | WS_SYSMENU
					,0,0,0,0,0,0,0,0);
					SetWindowLong(som_tool_taskbar,DWL_DLGPROC,(LONG)som_tool_taskbarproc);
					SendMessage(som_tool_taskbar,WM_INITDIALOG,0,0);
				}

				std::swap(hack,som_tool_cbt);
				C = som_tool_taskbar;
			}
			else if(peek->helpID==10200||peek->helpID==10300) 
			{
				C = som_tool_taskbar; //hack: SOM_MAIN stages 2 & 3
			}
			else if(som_tool_taskbar) //repair parentage
			{
				if(~peek->style&WS_CHILD)
				//NOTE: WM_ACTIVATE must be handled as well
				C = GetAncestor(som_tool_dialog(),GA_ROOT);
				if(!C) C = som_tool_taskbar;
			}
				
			if(C==som_tool_taskbar&&C)
			{
				peek->exStyle&=~WS_EX_APPWINDOW;	
			}

			#ifdef NDEBUG
			#error fix me
			#endif
			if(C==workshop_tool) C = 0; //TESTING
		}
		VirtualProtect((void*)B,16,rsrc,&rsrc); //!
	}
	else som_tool_find = 0;

	HWND out = 0; //EX_BREAKPOINT(0) //crashes

	assert(!som_tool_CreateDialogProc); //recursive?
	if(som_tool_CreateDialogProc)
	return SOM::Tool.CreateDialogIndirectParamA(A,B,C,D,E);
	som_tool_CreateDialogProc = D;

	//MessageBoxW(0,peek->variable+2,peek->variable+2,MB_OK);
	//static BOOL wclass = DestroyWindow(Sompaste->create(0,0));
	//CreateWindowExW(0,L"Sompaste.dll",peek->variable+2,WS_CAPTION|WS_DLGFRAME|WS_VISIBLE,0,0,640,480,0,0,0,0);
	//DialogBoxIndirectParamW(A,B,0,som_tool_TestDialog,E);
	//out = SOM::Tool.CreateDialogIndirectParamA(A,B,C,som_tool_InitDialog,E);
	som_tool_initializing++;	
	if((DWORD)D>SOM::image_rdata //D==SOM_EDIT_151)
	||D==som_tool_PseudoModalDialog) //SOM_MAP_165
	{
		HOOKPROC hack = 0;		
		std::swap(hack,som_tool_cbt); //garbles title?				
		out = CreateDialogIndirectParamW(A,B,C,som_tool_InitDialog,E);	
		if(SOM::tool<EneEdit.exe||out!=som_tool)
		ShowWindow(out,1); //SOM_MAP_165
		//SOM emulates modal dialogs with modeless ones
		//(probably so to centralize the message queue)
		if(out&&~GetWindowStyle(out)&WS_CHILD) 
		EnableWindow(C,0);		
		std::swap(hack,som_tool_cbt);		
	}
	else //no clue why EnableWindow must be done now??? taskbar?
	{
		out = CreateDialogIndirectParamW(A,B,C,som_tool_InitDialog,E);		

		if(!som_tool_classic) //NEW
		if(out&&~GetWindowStyle(out)&WS_CHILD) 
		{
			EnableWindow(C,0);

			if(out==som_tool)
			if(peek->helpID==30000||peek->helpID==40000)
			{
				wchar_t str[9] = L"0";_ultow((DWORD)out,str,16); 
				SetEnvironmentVariableW(L"SomEx.dll Workshop Tool",str);
			}			
		}
	}
	//som_tool_initializing--; //moved below...
	som_tool_CreateDialogProc = 0;	
	som_tool_find = 0;
	som_tool_help = 0; //2022
			
	som_tool_initializing--;
	
	//REMOVE ME?
	//(effects level 3+ subdialogs)
	if(recenter&&C&&C!=som_tool_taskbar)
	{
		if(IsWindow(out)) som_tool_recenter(out,C);
	}
	
	//hack: ensure the taskbar works
	EnableWindow(som_tool_taskbar,1);	

	if(out==som_tool)
	{
		//requires AllowSetForegroundWindow (som.exe.cpp)
		SetForegroundWindow(out);
		BringWindowToTop(out);		
	}				

	return out;
}
static INT_PTR WINAPI workshop_DialogBoxParamA(HINSTANCE A, LPCSTR B, HWND C, DLGPROC D, LPARAM E)
{
	//this is simplistic, but it's just to trick 
	//Ene/NpcEdit into changing the skin texture
	if(A||111!=(DWORD)B)
	return SOM::Tool.DialogBoxParamA(A,B,C,D,E);
	som_tool_initializing++;
	som_tool_CreateDialogProc = D;
	INT_PTR ret = DialogBoxParamW(A,(WCHAR*)B,C,som_tool_InitDialog,E);
	som_tool_CreateDialogProc = 0;
	som_tool_initializing--;
	return ret;
}

enum{ som_tool_GetWindowTextA_s=4096 };
static const wchar_t *som_map_longname(short);
static const wchar_t *som_map_profile(short part_no);
static const wchar_t *som_map_description(const wchar_t*);
static WORD som_map_workshop_number(const wchar_t *profile);
static BOOL WINAPI som_tool_SetWindowTextA(HWND A, LPCSTR C)
{	
	SOM_TOOL_DETOUR_THREADMAIN(SetWindowTextA,A,C)

	EXLOG_LEVEL(7) << "som_tool_SetWindowTextA()\n";

	const size_t x_s = 1024;
	static WCHAR x[1024]; *x = '\0';
	HWND parent = GetParent(A);
	int control = GetDlgCtrlID(A), hlp =
	GetWindowContextHelpId(control?parent:A);
	
	//if(!EX::debug)
	if(som_tool_find) //specialized common dialogs
	{
		assert(hlp>46000&&hlp<49000);		
		if((DLGC_STATIC|DLGC_BUTTON)&SendMessage(A,WM_GETDLGCODE,0,0)) 
		return TRUE;
	}

	switch(control) //file system
	{
	case 1007: case 1008: case 1010:  	

		if(SOM::tool==SOM_MAIN.exe
		 ||SOM::tool==SOM_RUN.exe
		 ||SOM::tool==SOM_EDIT.exe) //old map import tool
		{	
			if(!*C&&*som_tool_text) 			
			if(SOM::tool==SOM_RUN.exe) //SOM_MAIN.cpp
			{
				if(control==1008)
				{
					static bool one_off = false; if(!one_off++) return TRUE; //???
				}
				else if(control==1010)
				{
					static bool one_off = false; if(!one_off++) som_tool_refocus = A; //???
				}
			}		
			return SetWindowTextW(A,*C?som_tool_text:L""); 
		}
		break;

	case 1011: case 1013: //old map import tool

		if(SOM::tool==SOM_EDIT.exe)
		return SetWindowTextW(A,1011==control?som_edit_project2:SOM::Tool.project()); 
		break;
	}

	switch(SOM::tool)
	{
	case SOM_PRM.exe:
	
		switch(control)
		{		
		case 1036: //Scale
		
			//makes deletions user friendly
			if(!strcmp(C,"0.0")) C = "1.0";
			break;

		case 1225: case 1226: //Magic
		
			extern const WCHAR *SOM_PRM_magic_titles[3];
			extern void SOM_PRM_140_titles_chkstk(HWND,int);
			if(hlp==33100)
			if(SOM_PRM_magic_titles[2]) switch((DWORD)C)
			{								
			case 0x4765E4: case 0x4765F8: //offense/defense
				//2018: this now sets all
				SOM_PRM_140_titles_chkstk(GetParent(A),(DWORD)C==0x4765F8);
			case 0x4765D8: case 0x4765EC: //range/duration
				return 1; //seems to work
			}			
			break; //Magic		
		}
		break;

	case SOM_MAP.exe: 
		
		switch(hlp)
		{
		case 40100: //SOM_MAP progress

			switch(DWORD(C))
			{
			default: return TRUE;
			case 0x4902c8: case 0x490284:

				HWND next = //caption or plea?
				GetWindow(A,!control?GW_CHILD:GW_HWNDNEXT);
				if(!next||som_tool_v(next)||!GetWindowTextW(next,x,x_s)) 
				break;
				return SetWindowTextW(A,x);
			}
			break;

		case 40000: //main editor
		
			if(control==1133) //map number display
			{
				//now managing the map layers menu
				extern bool som_LYT_label(HWND,const char*);
				if(som_LYT_label(A,C)) 
				return TRUE; break;
			}
			if(control==1010) //[Profile] Description
			{
				short i = atoi(C+1);
				assert(i>=0);					
				const wchar_t *p = som_map_profile(i);
				//REPURPOSING: save for workshop_exe
				//REPURPOSING: som_files_wrote too (2022)
				extern WORD workshop_category;  
				workshop_category = som_map_workshop_number(p); 
				if(IsWindowVisible(som_tool_workshop)) //2023
				workshop_exe(40000); 

				if(!som_tool_classic)
				if(C[0]=='['&&isdigit(C[1])) //test +12
				{					
					wchar_t *sep = PathFindExtensionW(wcscpy(x,Sompaste->longname(p)));
					//&: underlines the first letter in the description
					wcscpy(wcscpy(sep,L" &")+2,som_map_description(p));				
					//NEW: this way & cannot introduce a new "mnemonic"	
					//NEW: trying out having it be a button to PrtsEdit
					if(DLGC_STATIC&SendMessage(A,WM_GETDLGCODE,0,0))
					SetWindowLong(A,GWL_STYLE,~SS_NOPREFIX&GetWindowStyle(A)|WS_DISABLED);
					//NEW: ensure that the underline will always appear
					som_tool_paint(0,A,UIS_CLEAR,UISF_HIDEACCEL);			
					int out = SetWindowTextW(A,x);
					som_tool_paint(1,A);			
					return out;
				}
				break;
			}
			//break; //!!!

		case 42000: //tile editor (FALLING THROUGH)

			switch(control) //tile information
			{
			case 1222: //checkpoint
			{
				//reminder: there isn't a 3rd option for no tile
				HWND next = som_tool_x(GetDlgItem(som_tool,1222));
				if(!next) break;  				
				char *fuka = (char*)0x48fca0; //kanou is at 48fca8
				wchar_t *p = som_tool_nth(x,GetWindowTextW(next,x,x_s),C==fuka,0);
				if(p) return SetWindowTextW(A,p);
				break;
			}
			case 1009: //0/90/180/270
			{
				HWND next = som_tool_x(GetDlgItem(som_tool,1009)); 
				if(!next) break;
				wchar_t *p = som_tool_nth(x,GetWindowTextW(next,x,x_s),atoi(C)/90,0);
				if(p) return SetWindowTextW(A,p);
				break;
			}
			case 1221: case 1229: //profile (filename minus extension)
			{
				long part_no = atoi(C);
				if(part_no>1023) //portable map profile
				{
					*PathFindExtensionW(wcscpy(x,som_map_longname(part_no))) = '\0';						
					return SetWindowTextW(A,x);
				}
				else if(part_no<=0&&*C!='0') //vacant profile
				{
					HWND next = som_tool_x(GetDlgItem(som_tool,1221)); 
					if(!next) break;
					wchar_t *p = som_tool_nth(x,GetWindowTextW(next,x,x_s),0,0);
					if(p) return SetWindowTextW(A,p);
				}
				break;
			}}
			break;

		case 42100: //tile editor's NPC insert
		  		
			switch(control)
			{
			case 1148: //Enemy/NPC placement?
			{
				if(!som_tool_v(A)) return TRUE;
				//Reminder: the dialog is not reloaded
				auto &save = som_tool_wector; //2021
				save.resize(GetWindowTextW(A,x,x_s)+1);
				wmemcpy(&save[0],x,save.size());	
				bool npc = DWORD(C)==0x490850+8;
				wchar_t *p = som_tool_bar(&save[0],save.size(),npc);				
				//works: this text has issues (flickering mirror image bypasses WM_PAINT)
				ShowWindow(A,0); SetWindowTextW(A,p); ShowWindow(A,1); 
				return TRUE;
			}
			case 1026: //layers support (Z)
			{
				//passing C somehow corrupts default
				//0.0 values???
				extern bool SOM_MAP_z_height(HWND);
				//if(SOM_MAP.base)
				if(SOM_MAP_z_height(A/*,(char*)C*/))
				return TRUE; break;
			}}
			break;

		case 42500: //Box/Corpse

			if(!control) 
			{
				//2018: Not passed as a straight value??? (was it ever so?)
				bool corpse = 0==strcmp(C,(char*)0x490960);
				return SetWindowTextW(A,som_tool_bar(x,GetWindowTextW(A,x,x_s),corpse));
			}			
			break;

		case 45100: //event dialog

			if(control==1024)
			som_map_event_memory = (char*)C; //hack
			break;
		
		case 46100: //text instruction

			if(control==1173&&*C=='\1') //2022: comment?
			{
				if(som_tool_play)
				{
					C++; Edit_LimitText(A,som_tool_GetWindowTextA_s-2);
				}
				else assert(0);
			}
			break;

		case 46400: //placement instruction

			if(control==1209) //Warp Enemy/NPC
			{
				bool npc = C==(char*)(0x4907C0+8);
				return SetWindowTextW(A,som_tool_bar(x,GetWindowTextW(A,x,x_s),npc,x));
			}
			break;
		}
		break;

	case SOM_SYS.exe:
	
		//slideshow editor dialog
		if(control==1012&&hlp==56200) 
		{
			HWND ss = parent; //GetParent(A);

			if(WS_CAPTION&GetWindowStyle(ss))
			{
				if(!som_tool_v(A)&&som_sys_slideshow) //enable translation
				{
					wchar_t *p = 
					som_tool_nth(x,GetWindowTextW(A,x,x_s),som_sys_slideshow-1,0);
					if(p) return SetWindowTextW(ss,p);
				}

				A = ss; //place text in caption
			}

		}break;	
	}

	if(!control&&hlp //title bar?
	&&WS_CHILD&~GetWindowStyle(A)&&GetWindowTextLengthW(A))
	{
		return TRUE; //enable translation
	}
		
	return SetWindowTextW(A,
	EX::need_unicode_equivalent(932,C,x,1024));
}

static int WINAPI som_tool_GetWindowTextLengthA(HWND A)
{
	SOM_TOOL_DETOUR_THREADMAIN(GetWindowTextLengthA,A)

	EXLOG_LEVEL(7) << "som_tool_GetWindowTextLengthA()\n";

	//NOTE: this is doing wchar_t to 932 conversion for
	//text boxes, so I'm not sure if x should be larger
	//than the wide buffer because I think a full-width
	//character counts as two against EM_LIMITTEXT size
	char x[som_tool_GetWindowTextA_s];
	int ret = GetWindowTextA(A,x,sizeof(x));
	assert(ret<sizeof(x)-4);
	return ret;
}
static int WINAPI som_tool_GetWindowTextA(HWND A, LPSTR C, int D) //B
{ 
	SOM_TOOL_DETOUR_THREADMAIN(GetWindowTextA,A,C,D)

	EXLOG_LEVEL(7) << "som_tool_GetWindowTextA()\n";

	//REMOVE ME? (SOM_PRM_reprogram)
	//HACK: Rushing to fix SOM_MAP bug (1.2.2.2)
	extern int SOM_PRM_items_mode;
	extern bool SOM_PRM_items_GetWindowTextA(HWND,LPSTR,int&);
	if(1202!=SOM_PRM_items_mode&&som_prm_tab==1001) //switch(C)
	{
		//The new item tabs are in play, so pull the old item
		//data from the PRM record.
		if(SOM_PRM_items_GetWindowTextA(A,C,D))
		return D;
	}

	int out = 0, control = GetDlgCtrlID(A);
	
	switch(SOM::tool) //Unicode file names		
	{
	case SOM_MAIN.exe: //new project
	
		som_tool_project = false;

		if(control==1007) //new folder
		{			
			strcpy(C,strrchr(SOMEX_(B),'\\')+1);
			return out = strlen(C);		 			
		}
		else if(control==1008) //project folder
		{
			HWND x3EF = GetDlgItem(GetParent(A),0x3EF);

			if(!GetWindowTextLengthW(A)
			 ||!GetWindowTextLengthW(x3EF)) return 0;

			//1: SOM expects a slash on the drive volume
			//For the record a trailing slash is removed
			strrchr(strcpy(C,SOMEX_(B)),'\\')[1] = '\0';
			out = strlen(C);

			//NEW: communicate .som file
			wchar_t som[MAX_PATH] = L"";			
			int cat = GetWindowTextW(A,som,MAX_PATH-1);
			som[cat++] = '\\';			
			int sep = GetWindowTextW(x3EF,som+cat,MAX_PATH-cat);
			if(!sep) return 0;
			swprintf_s(som+cat+sep+1,MAX_PATH-cat-sep-1,L"%ls.som",som+cat);
			som[cat+sep] = '\\';
			Sompaste->path(som);
			SetEnvironmentVariableW(L".SOM",som);
			return out;
		}	
		break;

	case SOM_RUN.exe: //new runtime

		som_tool_runtime = false;

		if(control==1008) //.som file name
		{				
			strcpy(C,SOMEX_(B)"\\.som");
			out = strlen(C);

			wchar_t som[MAX_PATH] = L"";	
			GetWindowTextW(A,som,MAX_PATH);
			SetEnvironmentVariableW(L".SOM",som); 
			return out;
		}
		else if(control==1007) //runtime name
		{
			strcpy(C,strrchr(SOMEX_(C),'\\')+1);
			return out = strlen(C);	
		}
		else if(control==1010) //runtime folder
		{
			HWND x3EF = GetDlgItem(GetParent(A),0x3EF);

			if(!GetWindowTextLengthW(A)
			  ||!GetWindowTextLengthW(x3EF)) return 0;
			
			//1: SOM expects a slash on the drive volume
			//For the record a trailing slash is removed
			strrchr(strcpy(C,SOMEX_(C)),'\\')[1] = '\0';
			out = strlen(C);						   		
			
			wchar_t run[MAX_PATH] = L"";			
			int cat = GetWindowTextW(A,run,MAX_PATH-1);
			run[cat++] = '\\';			
			int sep = GetWindowTextW(x3EF,run+cat,MAX_PATH-cat);
			if(!sep) return 0;
			Sompaste->path(run);
			SetEnvironmentVariableW(L".RUN",run); //hack
			return out;
		}
		break;

	case SOM_EDIT.exe: //old map import tool
	
		if(control==1008) //import project file
		{				
			som_tool_project2 = false;
			strcpy(C,SOMEX_(D)"\\.som");
			GetWindowTextW(A,som_tool_text,MAX_PATH);
			size_t n = //strip file name
			PathFindFileNameW(som_tool_text)-som_tool_text;
			if(n&&som_tool_text[n]) n--; //strip slash
			wcsncpy(som_edit_project2,som_tool_text,n);
			return strlen(C);
		}
		break;

	case ItemEdit.exe:

		//adding the new mode (3) to ItemEdit... I tried every
		//more direct approach without getting anywhere, which
		//all suggested that this way should be perfectly safe 
		switch(control)
		{
		case 1012: case 1013: case 1017:

			extern BYTE workshop_mode2; if(0==workshop_mode2)
			{
				C[0] = '0'; C[1] = '\0'; return 1;
			}
		}
		break;
	}

	//if(D) *C = '\0'; //PARANOIA
	wchar_t w[som_tool_GetWindowTextA_s];
	//comments need to prepend '\1'
	//if(!GetWindowTextW(A,w,som_tool_GetWindowTextA_s)) return 0;
	int len = GetWindowTextW(A,w,som_tool_GetWindowTextA_s);

	switch(SOM::tool)
	{
	case SOM_MAP.exe:

		if(control==1173&&som_tool_play) //2022: comment?
		{
			HWND parent = GetParent(A);
			if(46100==GetWindowContextHelpId(parent)) //text instruction
			{
				if(w[0]!='\1')
				{
					HWND lb = GetDlgItem(GetParent(parent),1026);
					ListBox_SetCurString(lb,w);
					wmemmove(w+1,w,len)[len] = '\0'; w[0] = '\1';
				}
				else assert(0);
			}
		}
	}

	return strlen(EX::need_ansi_equivalent(932,w,C,D));
	return 0;
}

static LONG APIENTRY som_main_etc_RegQueryValueExA(HKEY A, LPCSTR B, LPDWORD C, LPDWORD D, LPBYTE E, LPDWORD F)
{
	SOM_TOOL_DETOUR_THREADMAIN(RegQueryValueExA,A,B,C,D,E,F)

	EXLOG_LEVEL(7) << "som_main_etc_RegQueryValueExA()\n";

	if(*B=='I'&&!strcmp(B,"InstDir"))
	{		
		DWORD ra = (DWORD)EX::return_address();
		
		if(ra>=0x401000&&ra<SOM::image_rdata) 
		{
			if(!strcpy_s((char*)E,*F,SOMEX_(A)))
			{
				*F = sizeof(SOMEX_(A)); return ERROR_SUCCESS;
			}		
		}
	}
	
	return SOM::Tool.RegQueryValueExA(A,B,C,D,E,F);
}

//static WNDPROC som_tool_openproc2 = 0;
static OPENFILENAMEW *som_tool_open = 0;
extern wchar_t som_tool_opendirfilter[90] = L"";
static LRESULT CALLBACK som_tool_openproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scID, DWORD_PTR ofn)
{		
	switch(uMsg)
	{
	case WM_COMMAND:

		//taken from Somtext.cpp
		//(todo: share Sompaste)
		if(LOWORD(wParam)==IDOK) //select directory
		if(som_tool_opendirfilter==som_tool_open->lpstrFilter)
		{
			if(GetFocus()!=(HWND)lParam&&!som_tool_open->nFileOffset)
			{
				//5: CRLF/surrogate pairs
				wchar_t test[5]; if(GetDlgItemTextW(hWnd,1148,test,5))				
				break; //assuming navigating to a folder via the combobox
			}

			//0: finishing up
			OPENFILENAMEW *ofn = som_tool_open;
			som_tool_open = 0;

			if(!ofn->nFileOffset)
			{					
				wchar_t *p = ofn->lpstrFile; 
				if(!GetDlgItemTextW(hWnd,1148,p,MAX_PATH)||PathIsRelativeW(p))
				{
					size_t cat = SendMessageW(hWnd,CDM_GETFILEPATH,MAX_PATH,(LPARAM)p);				
					if(GetDlgItemTextW(hWnd,1148,p+cat+1,MAX_PATH-cat-1)) p[cat] = '\\';
				}			
				ofn->nFileOffset = PathFindFileNameW(p)-p;
				ofn->nFileExtension = PathFindExtensionW(p)-p;					
			}
			else assert(0);
			
			EndDialog(hWnd,1); return 0; 
		}

		switch(wParam) 
		{
		case MAKEWPARAM(1137,CBN_DROPDOWN): //ComboBox scrollbars
		{
			COMBOBOXINFO cbi = {sizeof(cbi)};
			if(GetComboBoxInfo((HWND)lParam,&cbi))
			SetWindowTheme(cbi.hwndList,L"",L""); else assert(0);
			break;
		}}
		break;
	}

	//return CallWindowProcW(som_tool_openproc2,hWnd,uMsg,wParam,lParam);
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
extern UINT_PTR CALLBACK som_tool_openhook(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
				
		som_tool_open = (OPENFILENAMEW*)lParam;
		return 1;

	case WM_NOTIFY:
	{
		OFNOTIFYW *p = (OFNOTIFYW*)lParam;

		switch(p->hdr.code)
		{
		case CDN_INITDONE:
			
			//want SetWindowTheme on combobox
			//if(som_tool_opendirfilter==som_tool_open->lpstrFilter)
			{
				//WHY NOT SetWindowSubclass?!
				//som_tool_openproc2 = (WNDPROC)			
				//SetWindowLongPtrW(p->hdr.hwndFrom,GWL_WNDPROC,(LONG_PTR)som_tool_openproc);
				SetWindowSubclass(p->hdr.hwndFrom,som_tool_openproc,0,(DWORD_PTR)p->lpOFN);
			}

			//NEW: auto-select default (also applies style)
			extern UINT_PTR CALLBACK SomEx_OpenHook(HWND,UINT,WPARAM,LPARAM);			
			return SomEx_OpenHook(hWnd,uMsg,wParam,lParam);

			//TODO: NEARLY IDENTICAL TO SOM_MAIN_openhook
			//TODO: NEARLY IDENTICAL TO SOM_MAIN_openhook
			//TODO: NEARLY IDENTICAL TO SOM_MAIN_openhook
			//TODO: NEARLY IDENTICAL TO SOM_MAIN_openhook
			//TODO: NEARLY IDENTICAL TO SOM_MAIN_openhook
		case CDN_SELCHANGE:

			//see Somtext.cpp for original implementation
			if(som_tool_opendirfilter==som_tool_open->lpstrFilter)
			{
				HWND SHELLDLL_DefView = 
				GetDlgItem(p->hdr.hwndFrom,1121);
				HWND lv = GetDlgItem(SHELLDLL_DefView,1);
				int sel = ListView_GetNextItem(lv,-1,LVNI_SELECTED);
				wchar_t buffer[MAX_PATH] = L"";				
				LVITEMW lvi = {LVIF_TEXT,0,0,0,0,buffer,MAX_PATH};
				if(sel!=-1) SendMessageW(lv,LVM_GETITEMTEXTW,sel,(LPARAM)&lvi);
				SetDlgItemTextW(p->hdr.hwndFrom,1148,buffer);
			}
			break;

		case CDN_FOLDERCHANGE: //not sure why this is necessary???

			if(som_tool_opendirfilter==som_tool_open->lpstrFilter)
			SetDlgItemTextW(p->hdr.hwndFrom,1148,L"");
		}
		break;
	}}

	return 0;
}
extern LPITEMIDLIST STDAPICALLTYPE som_tool_SHBrowseForFolderA(LPBROWSEINFOA A)
{
	SOM_TOOL_DETOUR_THREADMAIN(SHBrowseForFolderA,A)

	EXLOG_LEVEL(7) << "som_tool_SHBrowseForFolderA()\n";
	
	ITEMIDLIST *out = 0; 

	if(0) //old way
	{
		//TODO: use lpszTitle to explain to user
		//that a folder will be created inside of
		//the folder that they are about to select
		//(the message has to be localized somehow)

		//Note: SOM doesn't want lpszDisplayName
		BROWSEINFOW w; memset(&w,0x00,sizeof(w));
		w.hwndOwner = A->hwndOwner;
		w.ulFlags = BIF_USENEWUI;
		/*
		A->lpfn = 0; //disabling SOM's custom callbacks! 

		A->ulFlags&=~BIF_STATUSTEXT; //used by callbacks

		A->ulFlags|=BIF_USENEWUI; //A modest improvement
		
		//assuming dialog is same as SHBrowseForFolderW's
		LPITEMIDLIST out = SOM::Tool.SHBrowseForFolderA(A);
		*/
		//CoInitialize(0);
		out = SHBrowseForFolderW(&w);
		//CoUninitialize();

		SHGetPathFromIDListW(out,som_tool_text);
	}
	else //new way
	{
		if(!*som_tool_opendirfilter)
		{
		SHFILEINFOW si; //install: any folder will do here
		SHGetFileInfoW(SOM::Tool::install(),0,&si,sizeof(si),SHGFI_TYPENAME);		
		if(!*si.szTypeName) wcscpy(si.szTypeName,L"Folder");
		//qqqq... is a tip from the link below
		//http://www.codeproject.com/Articles/20853/Win-Tips-and-Tricks		
		swprintf_s(som_tool_opendirfilter,L"%ls%lcqqqqqqqqqqqqqqq.qqqqqqqqq%lc",si.szTypeName,'\0','\0');
		}
		*som_tool_text = '\0';
		SHGetPathFromIDListW(A->pidlRoot,som_tool_text); //empty (desktop? documents?)
		//2021: som_art_2021???
		//if(SOM::tool) assert(A->hwndOwner==som_tool_dialog());
		OPENFILENAMEW open =		
		{
		sizeof(open),A->hwndOwner/*som_tool_dialog()*/,0,
		som_tool_opendirfilter, 0, 0, 0, som_tool_text, MAX_PATH, 0, 0, som_tool_text, 0,
		OFN_ENABLEHOOK|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_ALLOWMULTISELECT|OFN_EXPLORER,
		0, 0, 0, 0, som_tool_openhook, 0, 0, 0 
		};
		bool save = A->lParam==1; //som_art_2021?
		if(!(save?GetSaveFileNameW:GetOpenFileNameW)(&open))
		{
			//Reminder: had troubles with XP (som.exe.cpp)
			DWORD err = CommDlgExtendedError(); assert(!err); 
			return 0;
		} 	
		//NOTE: probably unnecessary since the field
		//is populated with som_tool_text regardless
		SHParseDisplayName(som_tool_text,0,&out,0,0); assert(out);
		return out;
	}
	return out;
}
static BOOL APIENTRY som_tool_GetOpenFileNameA(LPOPENFILENAMEA a)
{
	SOM_TOOL_DETOUR_THREADMAIN(GetOpenFileNameA,a)

	EXLOG_LEVEL(7) << "som_tool_GetOpenFileNameA()\n";
		
	//*.som causes recent files to be empty?!
	wchar_t som[MAX_PATH*2] = L""; //"*.som"; 
	
	if(SOM::tool<EneEdit.exe) assert(a->hwndOwner==som_tool_dialog());

	OPENFILENAMEW w = 
	{
		sizeof(w),a->hwndOwner/*som_tool_dialog()*/,0,
		//Reminder: SOM doesn't 00 terminate or supply *.som as required by the docs
		L"Sword of Moonlight (*.som)\0*.som\0",0,0,0,som,MAX_PATH,0,0,0,0, 
		OFN_ENABLEHOOK|OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_FILEMUSTEXIST,
		0,0,0,0,som_tool_openhook,0,0,0
	};
	if(SOM::tool>=PrtsEdit.exe) //prf/prt?
	{
		w.Flags|=OFN_DONTADDTORECENT; //SfxEdit/EneEdit?

		//adding commandline support to Prts/Item/ObjEdit?
		if(som_tool_initializing) goto init;

		if(!*wcscpy(som,workshop_savename))
		{
			char a[16];			
			extern const char *workshop_directory(int=0,int=SOM::tool);
			sprintf(a,"%s\\%s",workshop_directory(),SOM::tool==PrtsEdit.exe?"parts":"prof");
			EX::data(a,som+1);
			*som = '\0'; w.lpstrInitialDir = som+1;
		}
		extern const wchar_t *workshop_filter(const char*,wchar_t[]);
		w.lpstrFilter = workshop_filter(a->lpstrFilter,som_tool_text);		
	}
	som[MAX_PATH+1] = '\0';
	w.lpstrFileTitle = som+MAX_PATH+1;
	w.nMaxFileTitle = 64;	

	if(!GetOpenFileNameW(&w)) 
	{
		*som_tool_text = '\0'; //workshop_filter

		//Reminder: had troubles with XP (som.exe.cpp)
		DWORD err = CommDlgExtendedError(); assert(!err); 
		return FALSE;
	}
		
	if(SOM::tool>=PrtsEdit.exe) init:
	{
		if(a->lpstrFileTitle)
		strcpy(a->lpstrFileTitle,".ws");
		if(a->lpstrFile)
		strcpy(a->lpstrFile,SOMEX_(A)"\\.ws");			
		//wcscpy(som_tool_text,w.lpstrFile);

		//HACK: workshop_file_update
		if('*'==*som_tool_text) return TRUE;

		//HACK: disable som_tool_GetWindowTextA hack
		//while opening since text is not changed if 
		//their values are 0
		extern BYTE workshop_mode2; 
		if(0==workshop_mode2 //1000?
		&&ItemEdit.exe==SOM::tool)
		{
			workshop_mode2 = 15; //1015
			PostMessage(som_tool,WM_COMMAND,1000,0);
		}

		PostMessage(som_tool,WM_APP+0,0,0);
	}
	else if(SOM::tool!=SOM_EDIT.exe)
	{
		som_tool_project = false;		
		strcpy(a->lpstrFileTitle,".som"); 
		strcpy(a->lpstrFile,SOMEX_(B)"\\.som");	
		if(SOM::tool==SOM_MAIN.exe)
		SetEnvironmentVariableW(L".SOM",w.lpstrFile);
		//if(SOM::tool==SOM_RUN.exe)
		//wcscpy(som_tool_text,w.lpstrFile);
	}
	else //NEW: old map import tool
	{
		strcpy(a->lpstrFileTitle,".som"); 
		strcpy(a->lpstrFile,SOMEX_(D)"\\.som");
		//wcscpy(som_tool_text,w.lpstrFile);
	}
	//TODO: don't know if this test is necessary but only
	//SOM_MAIN did not do this
	if(SOM::tool!=SOM_MAIN.exe)
	wcscpy(som_tool_text,w.lpstrFile); return TRUE;		
}

static bool som_tool_read = true;
static size_t som_tool_reading = 0;	
static const size_t som_tool_readmax = 8;
static HANDLE som_tool_reading_list[som_tool_readmax] = {};
extern const wchar_t *som_tool_reading_mode[som_tool_readmax] = {};
extern HANDLE &workshop_reading = som_tool_reading_list[0]; 
extern const wchar_t* &workshop_title = som_tool_reading_mode[0];

static bool som_tool_prt(int,DWORD,DWORD,BYTE*);
static bool som_tool_prf(int,DWORD,DWORD,BYTE*);
static bool som_tool_prm(int,DWORD,DWORD,BYTE*);

static bool (*som_tool_reading_f[som_tool_readmax])(int,DWORD,DWORD,BYTE*);

static void som_tool_reading_x(size_t i)
{									
	//some looking out for leaks
	assert(som_tool_reading<7);
	assert(i<som_tool_reading);

	if(i>som_tool_reading) return;

	while(++i<som_tool_reading)
	{
		som_tool_reading_list[i-1] = som_tool_reading_list[i];
		som_tool_reading_f[i-1] = som_tool_reading_f[i];
	}
	som_tool_reading--;
}

static void som_tool_pro();
static bool som_run_locked = true;
extern bool som_tool_file_appears_to_be_missing(const char *a, const wchar_t *w, wchar_t *w_ext)
{
	//2022: PathFileExistsA returns true now
	//with a SOMEX_ path. I don't think it's
	//because of changes. it shouldn't be so
	//at this point though
	assert(w||a[3]!='>'); //SOMEX_(A)?

	if(w&&!PathFileExistsW(w)
	||!w&&!PathFileExistsA(a))
	{	
		if(SOM::tool==SOM_RUN.exe&&!strncmp(a,"\\\\.\\",4))
		{
			if(som_run_locked) return true; 

			if(a[4]>='A'&&a[4]<='Z') //mounting
			if(a[5]==':'&&a[6]=='\0') return true;
			//virtual files presumed related to
			//SOM_RUN's disc-locking mechanisms
			char *vfiles[] = {"SICE","NTICE","Secdrv"};
			for(int i=0;i<EX_ARRAYSIZEOF(vfiles);i++)
			if(!strcmp(a+4,vfiles[i])) return true;
		}

#ifdef _DEBUG //then we may wanna know what's up

		if(!w) w = EX::need_unicode_path_to_file(a);
#else			
		if(!w) return true; //assume its not of sword of moonlight
#endif
		if(EX::INI::Output()->missing_whitelist(a)) //2022
		return true;

		if(w_ext&&!wcsicmp(w_ext,L".pro"))
		{
			som_tool_pro(); //does PR2 to PRO

			if(PathFileExistsW(w)) return false;

			if(SOM::tool==SOM_RUN.exe)
			{
				if(IDCANCEL==EX::messagebox(MB_OKCANCEL|MB_ICONERROR,				
				"Source will be PR~ or PR2 but destination will be PRO:\n"
				"\n"
				"For game play this is of no consequence. For projects this is a problem."))
				return false;
			}

			w_ext[3] = '~'; //PR~ (incomplete pro file)

			if(PathFileExistsW(w)) return false;

			w_ext[3] = '2'; //PR2 (read only pr2 file)

		//WARNING: I got here opening a ZIP file (by mistake) that didn't
		//have a PR2 file. either ZIP isn't fully transparent or read-only
		//project support needs a look)

			EX::messagebox(MB_OK,"Making %ls read-only.\n"
			"\n"
			"This is in order to ensure that it is not saved to as a PRO file would be.",w);
					
			SetFileAttributesW(w,FILE_ATTRIBUTE_READONLY);

			if(PathFileExistsW(w)) return false;
		}

		wchar_t msg[MAX_PATH*2] = L"";
		wchar_t pretty[MAX_PATH] = L""; Sompaste->path(wcscpy(pretty,w));

		EXLOG_ALERT(0) 
		<< "Warning!: A file appears to be missing: " << Ex%pretty << "\n";
				
		static bool suppress = false; EX::INI::Output tt;

		extern const wchar_t *x2mdl_dll;
		if(!suppress)
		if(!tt->do_missing_file_dialog_ok
		||SOM::tool==MapComp.exe //2022: what about objects?
		//HACK: ignore (nonfatal?) object
		&&!wcscmp(x2mdl_dll,L"x2msm.dll")) 
		{
			if(IDOK!=EX::messagebox(MB_OKCANCEL|MB_ICONERROR,
			"A file appears to be missing: \n\n%ls\n\n"					   
			"Press CANCEL to disable this message box.",pretty))
			{
				suppress = true; pass:;
			}
		}
	}
	else return false; return true;
}

static bool som_tool_overwrite_clock = false;
static BOOL WINAPI som_main_CopyFileA(LPCSTR A, LPCSTR B, BOOL C)
{	
	SOM_TOOL_DETOUR_THREADMAIN(CopyFileA,A,B,C)

	EXLOG_LEVEL(7) << "som_main_CopyFileA()\n";

	EXLOG_LEVEL(3) << "Copying " << A << " to " << B << '\n';

	const char *sep = strrchr(A,'\\');
	const char *dot = strrchr(sep,'.');	
		
	if(!stricmp(dot+1,"ini")) //GAME.INI (don't want it)
	{
		//return TRUE; 
		//REMOVE ME?
		//Just taking this time to copy this file instead
		//(this file works with keygen.h/cpp to port textures to DirectX 9)
		/*2021: finally ripping out [Keygen] (IMAGES.INI)
		A = SOMEX_(A)"\\data\\my\\prof\\IMAGES.INI";
		B = SOMEX_(B)"\\IMAGES.INI";*/
		return TRUE;
	}

	const wchar_t *Aw = SOM::Tool::install(A);
	const wchar_t *Bw = SOM::Tool::project(B);

	if(!Aw)
	{
		EXLOG_ALERT(0) << "WARNING! Failed to widen " << A << '\n'; 		
	}
	else EXLOG_LEVEL(3) << "Source widened to "<< Aw << '\n';

	if(!Bw)
	{
		EXLOG_ALERT(0) << "WARNING! Failed to widen " << B << '\n'; 		
	}
	else EXLOG_LEVEL(3) << "Destination widened to "<< Bw << '\n';

	if(som_tool_file_appears_to_be_missing(A,Aw,0))
	{
		return SOM::Tool.CopyFileA(A,B,C);	
	}

	if(Aw&&Bw) 
	{	
		if(som_tool_overwrite_clock)
		{
			struct _stat fst, gst; 
			
			if(_wstat(Aw,&fst)) fst.st_mtime = 0;
			if(_wstat(Bw,&gst)) gst.st_mtime = 0; 

			if(fst.st_mtime<=gst.st_mtime) return TRUE;	
		}
			
		if(!stricmp(dot,".dat"))
		if(!stricmp(sep+1,"sys.dat")) 
		{	
			BOOL out = SOM::xcopy(L"PARAM\\SYS.DAT",L"data\\my\\prof\\sys.dat");
			//NEW: piggybacking (including overwrite option...)
			som_main_CopyFileA(SOMEX_(A)"\\data\\sound\\se\\sound.dat",SOMEX_(B)"\\data\\sound\\se\\sound.dat",C);
			assert(out); return out;
		}		
		else if(!stricmp(sep+1,"sound.dat")) //NEW
		{
			if(SOM::xcopy(L"DATA\\SOUND\\SE\\sound.dat",L"data\\sound\\se\\sound.dat"))
			return TRUE; else assert(0);
		}

		return CopyFileW(Aw,Bw,C);
	}
	else assert(0);
											  
	return SOM::Tool.CopyFileA(A,B,C);	
}

static BOOL WINAPI som_run_CopyFileA(LPCSTR A, LPCSTR B, BOOL C)
{	
	SOM_TOOL_DETOUR_THREADMAIN(CopyFileA,A,B,C)

	EXLOG_LEVEL(7) << "som_run_CopyFileA()\n";
						
	EXLOG_LEVEL(3) << "Copying " << A << " to " << B << '\n';

	const wchar_t *Aw = 0, *Bw = 0; 

	if(!Aw) Aw = SOM::Tool::file(A);
	if(!Aw) Aw = SOM::Tool::install(A); //tool/som_rt.exe
	if(!Bw) Bw = SOM::Tool::runtime(B);

	//NEW: virtual data directory
	/*static*/ wchar_t vname[MAX_PATH];
	if(Aw&&!wcsnicmp(Aw,L"DATA\\",5)) 
	{
		//2018: assuming not returning this
		//static wchar_t v[MAX_PATH] = L"";
		if(EX::data(Aw+5,vname))
		Aw = vname; 	
	}

	if(!Aw)
	{
		EXLOG_ALERT(0) << "WARNING! Failed to widen " << A << '\n'; 		
	}
	else EXLOG_LEVEL(3) << "Source widened to "<< Aw << '\n';

	if(!Bw)
	{
		EXLOG_ALERT(0) << "WARNING! Failed to widen " << B << '\n'; 		
	}
	else EXLOG_LEVEL(3) << "Destination widened to "<< Bw << '\n';

	if(som_tool_file_appears_to_be_missing(A,Aw,0))
	{
		return SOM::Tool.CopyFileA(A,B,C);	
	}

	if(Aw&&Bw) 
	{	
		if(som_tool_overwrite_clock)
		{
			struct _stat fst, gst; 
			
			if(_wstat(Aw,&fst)) fst.st_mtime = 0;
			if(_wstat(Bw,&gst)) gst.st_mtime = 0; 

			if(fst.st_mtime<=gst.st_mtime) return TRUE;	
		}

		return ::CopyFileW(Aw,Bw,C);
	}
	else assert(0);
											  
	return SOM::Tool.CopyFileA(A,B,C);	
}

struct som_prm //lazy singleton
{
	const size_t sorts; //EX::INI::Editor

	//types is the total number of profiles
	enum{ item=0, my, obj, enemy, npc, types };
	//
	static const wchar_t *data(size_t type) 
	{
		const wchar_t *out[types] = 
		{L"ITEM",L"MY",L"OBJ",L"ENEMY",L"NPC"}; 
		return type<types?out[type]:L"?";
	}	

	static const wchar_t separator = -1;

	struct group //EX::INI::Editor::assorted_
	{
		const wchar_t *const index, count, table;

		size_t profile(int id, wchar_t query[MAX_PATH]);
				
		inline wchar_t operator+(int i){ return index[i]; }

		void operator=(const group &cp){ new(this)group(cp); }
	};
		
	group operator()(size_t sort);

	size_t count()
	{
		size_t i,sum;
		for(i=sum=0;i<sorts;i++) sum+=operator()(i).count;
		return sum;
	}
	int first(wchar_t find, int def)
	{	 		
		int out = 0;
		for(size_t j=0;j<sorts;j++)
		{
			group g = operator()(j);

			for(size_t k=0;k<g.count;k++,out++)
			{
				if(g.index[k]==find) return out;
			}
		}
		return def;
	}

	static size_t type(const wchar_t *path)
	{
		for(size_t i=0;i<types;i++)
		{
			const wchar_t *p, *q, *d = data(i);
			for(p=path;q=wcspbrk(p,L"\\/");p=q+1)
			if(!wcsnicmp(p,d,q-p)&&d[q-p]=='\0') return i;
			if(!wcsicmp(p,d)) return i;
		}
		return types;
	}
	som_prm(const wchar_t*p):sorts(0){ new(this)som_prm(type(p)); }
	som_prm(size_t type=-1):sorts(1+EX::INI::Editor()->assorted_()) //ugly
	{		
		if(type==size_t(-1)) type = blob_type; //todo: revisit

		if(type!=blob_type) blob.clear(); blob_type = type;

		if(blob.empty()) operator()(0); //illustrative
	}
	
	struct key //wchar_t only
	{	
		//31: surpasses 30 bytes
		wchar_t description[31];

		wchar_t profile[15], sorted:1;
		
		inline operator const wchar_t*()const
		{
			assert(description+31==profile);
			return description;
		}		
		inline const wchar_t *longname()const
		{
			return Sompaste->longname(profile);
		}		
		//hack: longname support thru profile_predicate 
		inline const wchar_t*operator+(size_t pred)const
		{
			return pred==31?longname():description+pred; 
		}
	};
	template<size_t off> struct predicate
	{
		const som_prm::key *first; //std::less

		inline bool operator()(wchar_t a, wchar_t b) 
		{
			return wcscmp(first[a]+off,first[b]+off)<0;
		}
		inline bool operator()(wchar_t a, const wchar_t *b) 
		{
			return wcscmp(first[a]+off,b)<0;
		}
		inline bool operator()(const wchar_t *a, wchar_t b) 
		{
			return wcscmp(a,first[b]+off)<0;
		}
	};	
	inline predicate<0> description_predicate()
	{
		predicate<0> out = { &operator[](0) }; return out;		
	}
	inline predicate<31> profile_predicate()
	{
		predicate<31> out = { &operator[](0) };	return out;		
	}
	inline const key &operator[](wchar_t i)
	{
		return (key&)blob[1+i*sizeof(key)/sizeof(wchar_t)];
	}	
	inline wchar_t keys(){ return blob[0]; }

    /*///// internals /////*/
		
	inline const wchar_t *legend()
	{
		return (wchar_t*)&operator[](keys()); 
	}

	static size_t blob_type/* = item*/;
	static std::vector<wchar_t> blob/*[types]*/;
		
	struct //pr2->pro
	{
		const char *pro(const char *pr2)
		{
			const char *o = strrchr(pr2,'.');
			if(o&&(o[1]=='p'||o[1]=='P')) return pr2;

			const wchar_t *w =
			EX::need_unicode_equivalent(932,pr2);
			if(!w||!*w) return pr2;

			const key *p = &(*(som_prm*)0)[0]; //hack//	
			for(size_t i=0,n=((som_prm*)0)->keys();i<n;i++)
			{
				if(!wcscmp(p[i].description,w)) return 
				pad(EX::need_ansi_equivalent(65001,p[i].longname()));
			}			
			if(pr2_mismatch>=0&&++pr2_mismatch)
			{			
				if(IDCANCEL==EX::messagebox(MB_OKCANCEL|MB_ICONERROR,
				"While converting a PR2 file to a PRO file a description \"%ls\" could not be matched.\n"
				"\n"					   
				"Press CANCEL to disable this message box.",w))
				{
					pr2_mismatch = -1;
				}
			}
			return pr2;
		}
		//transitional/scheduled obsolete
		//NEW: space pad just for partial PRO files
		/*private:*/const char *pad(const char *cinout) 
		{
			size_t len = strlen(cinout); 			
			while(len<30) (char&)cinout[len++] = ' '; 
			(char&)cinout[len] = '\0'; 
			return cinout;
		}

	}*pr2; static int pr2_mismatch;
};
int som_prm::pr2_mismatch = 0;

size_t som_prm::blob_type = som_prm::item;

std::vector<wchar_t> som_prm::blob;

static som_tool_xdata_t &som_tool_xdata_2023()
{
	//som_tool_zcopy's use of inflate_t was causing
	//a stack-overflow scenario, so take no chances
	static auto *x = new som_tool_xdata_t; return *x; //MEMORY LEAK
}
som_prm::group som_prm::operator()(size_t sort)
{		
	if(!blob.empty()) 
	{	
		const wchar_t *ls = legend()+sort; 		

		group out = {ls+*ls,1+ls[1]-*ls,blob_type};	return out;			
	}

	//makes SOM_PRM appear responsive
	RedrawWindow(som_tool_dialog(),0,0,
	RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN);
	
	//// feeding our blob ///////////////
	//
	// note this is done in operator() so 
	// to illustrate how everything works

	wchar_t directory[16] = L"";	
	wcscpy(directory,data(blob_type));
	//ZIP files require lowercase
	for(int i=0;directory[i];i++) 
	directory[i] = tolower(directory[i]);

	wchar_t query[MAX_PATH] = L""; 
	swprintf(query,L"%ls/*",directory);
	size_t n = Sompaste->database(0,query,0,0);
	
	//som_tool_xdata_t x;
	auto &x = som_tool_xdata_2023();
	wchar_t xquery[MAX_PATH] = L"", *xcat = 
	xquery+swprintf(xquery,L"data/%ls/prof/",directory);
	int xcat_s = xcat<=xquery?0:xquery+MAX_PATH-xcat;

	int compile[sizeof(key)%sizeof(wchar_t)==0];
	blob.resize(1+n*sizeof(key)/sizeof(wchar_t)+sorts+1,L'\0'); 

	blob[0] = n; assert(keys()==n);

	key *p = (key*)&blob[1]; assert(&operator[](0)==p);
	
	for(size_t i=0;i<n;i++,p++)
	{
		swprintf_s(query,L"%ls/%lc",directory,i+'0');

		wchar_t *filename = query+Sompaste->database(0,query,0,0); 
		assert(filename!=query);
		//"shortname"
		wcscpy_s(p->profile,filename); 

		if(!filename[1]) //NEW
		wcscpy(filename,Sompaste->longname(*filename));		
				
		char shift_jis[31] = ""; //932
		wcscpy_s(xcat,xcat_s,filename);
		if(!som_tool_xdata(xquery,x))
		{
			FILE *f = _wfopen(query,L"rb");

			fread(shift_jis,30,1,f); fclose(f);
		}
		else memcpy(shift_jis,x,30); shift_jis[30] = '\0';			

		EX::need_unicode_equivalent(932,shift_jis,p->description,31);
		assert(*p->description);
		//moved up top
		//wcscpy_s(p->profile,filename);
		p->sorted = false; 
	}

	assert(legend()==*p);
		   
	size_t sep = 0; //separators

	size_t l = legend()-&blob[0];

	blob[l++] = sorts+1; //after legend
		
	/*Reminder: push_back invalidates blob*/

	static std::vector<wchar_t> index; if(sorts>1) 
	{
		index.clear(); index.reserve(n);
		for(size_t i=0;i<n;i++) index.push_back(i);
		//the index is very likely to be presorted by profile
		//std::sort(index.begin(),index.end(),profile_predicate());		
	}				   
	for(size_t i=1,sep=0;i<sorts;i++,l++)
	{	
		size_t sorted = 0;
		const wchar_t *lv = 0;
		EX::INI::Editor()->sort_(i-1,&lv,0);

		if(lv&&!*lv) sep = l; else //!
		for(int k;0<swprintf(query,L"%ls/%lc",directory,sorted+'0')
		&&(k=Sompaste->database(0,query,lv,0));sorted++)
		{			
			if(sep) //insert a separator
			{		
				while(sep!=l) blob[sep++]++;
				sep = 0; blob.push_back(separator); 
			}
			//find key index of profile
			wchar_t unfiltered = *std::lower_bound 
			(index.begin(),index.end(),Sompaste->longname(query+k),profile_predicate());
			blob.push_back(unfiltered); 
			const_cast<key&>(operator[](unfiltered)).sorted = true;
		}

		blob[l] = blob[l-1]+sorted-1; if(!sorted) continue;

		std::sort(blob.end()-sorted,blob.end(),description_predicate());
	}
		
	size_t assorted = 0; 

	for(size_t i=0;i<n;i++) if(!operator[](i).sorted)
	{
		blob.push_back(i); assorted++;
	}

	blob[l] = blob[l-1]+assorted-1; 
		
	std::sort(blob.end()-assorted,blob.end(),description_predicate());

	return operator()(sort);
}

size_t som_prm::group::profile(int id, wchar_t query[MAX_PATH])
{
	*query = '\0'; if(id<0||id>=count) return 0; //paranoia

	size_t out = 0; const wchar_t *data = som_prm::data(table);

	while(query[out]=data[out]) out++; query[out++] = L'/';

	query[out++] = L'0'+index[id]; query[out] = 0; return out;
}

static bool som_tool_prototype(int i, FILE *f, FILE *g)
{
	DWORD count = 0;
		
	if(!fread(&count,4,1,f)) return false;
		
	som_prm tab(i);	int seek = 0;

	switch(i)
	{
	case som_prm::my: seek = 10; break;
	case som_prm::obj: seek = 78; break;
	case som_prm::item: seek = 58; break;  	
	}

	som_prm::pr2_mismatch = 0;

	CHAR buf[31] = {0}; buf[30] = 0;

	if(seek) fseek(g,4,SEEK_SET);
	if(seek) for(size_t i=0;i<count;i++)
	{			
		if(!fread(buf,30,1,f)
		 ||!fwrite(tab.pr2->pro(buf),30,1,g)) return false;

		fseek(f,seek,SEEK_CUR);
		fseek(g,seek,SEEK_CUR);
	}
	else for(size_t i=1;i<=1024;i++) //NPCs
	{
		if(count) 
		{
			fseek(f,count,SEEK_SET);
			fseek(g,count,SEEK_SET);

			if(count<1024*4
			 ||!fread(buf,30,1,f)
			 ||!fwrite(tab.pr2->pro(buf),30,1,g)) return false;

			fseek(f,i*4,SEEK_SET);
		}
		if(fread(&count,4,1,f))
		{
			if(!count) break; //NEW: no enemies/NPCs
			
		}else return false;
	}

	return !som_prm::pr2_mismatch;
}

static void som_tool_pro()
{
	static bool one_off = false; if(one_off++) return; //???

	size_t save = som_prm::blob_type;

	wchar_t param[MAX_PATH] = L"", param2[MAX_PATH];

	int param_s = swprintf(param,L"%ls\\PARAM\\",EX::cd());
	
	wchar_t *src = param, *dst = wcscpy(param2,param); 

	bool converted = false; //NEW
	for(int i=0;i<som_prm::types;i++)
	{			
		const wchar_t *prm = L"MAGIC";
		
		if(i!=som_prm::my) prm = som_prm::data(i);
		
		swprintf_s(src+param_s,MAX_PATH-param_s,L"%ls.PR~",prm);
		swprintf_s(dst+param_s,MAX_PATH-param_s,L"%ls.PRO",prm);
		if(PathFileExistsW(dst)) continue; //paranoia

		wchar_t *tilde = wcsrchr(src+param_s,'~');
		if(!PathFileExistsW(src)) *tilde = '2'; //PR2
		if(!CopyFileW(src,dst,TRUE)) continue; //PRO

		converted = true; //NEW

		FILE *f = _wfopen(src,L"rb"), *g = _wfopen(dst,L"rb+"); 

		bool partial = !som_tool_prototype(i,f,g); 

		fclose(f); fclose(g); if(partial)
		{			
			if(IDYES==EX::messagebox(MB_YESNO|MB_ICONERROR,
			"While converting a PR2 file to a PRO file (%ls) there were complications.\n"
			"\n"					   
			"Would you like to name the PRO file PR~ instead, so that conversion can continue at another time?",
			prm)&&*tilde!='~')
			{	
				*tilde = '~'; //dst,src: no, your eyes are not deceiving you!!
				if(!MoveFileEx(dst,src,MOVEFILE_REPLACE_EXISTING)) assert(0);
			}
		}	
	}

	if(converted)
	if(IDYES==EX::messagebox(MB_YESNO|MB_ICONERROR|MB_DEFBUTTON2,
	"Finished converting PR2 files to PRO files. The PR2 files can now be deleted.\n"
	"\n"					   
	"Would you like to delete the PR2 files from the project PARAM folder at this time?"))
	{		
		for(int i=0;i<som_prm::types;i++)
		{			
			const wchar_t *prm = L"MAGIC";			
			if(i!=som_prm::my) prm = som_prm::data(i);
			wcscat(wcscat(param+param_s,prm),L".PR2");
			SetFileAttributesW(param,0);
			DeleteFileW(param);
		}
	}

	som_prm restore(save);
}
		  
extern void som_map_refresh_model_view()
{								   
	//HACK: depends on wraparound extension	
	som_map_tileviewmask|=0x200;
	PostMessage(SOM_MAP.palette,WM_KEYDOWN,VK_LEFT,0);	
	//som_map_tileviewmask&=~0x200; //SendMessage?
	PostMessage(SOM_MAP.palette,WM_KEYDOWN,VK_RIGHT,0);	
}
/*OBSOLETE?
static bool som_map_rightclick(HWND win)
{
	if(som_tool_classic) return true;
	if(som_map_tileview) return win==som_map_tileviewport;
	return win==SOM_MAP.model||win==SOM_MAP.painting||win==SOM_MAP.palette;
}*/
static const wchar_t *som_map_profile(short part_no)
{
	int no = part_no;

	if(no<1024) //legacy
	{
		wchar_t find[16] = L"";
		swprintf_s(find,L"%04d.prt",no);
		no = SOM_MAP.prt[find].number();
	}
	else no-=1024;

	return SOM_MAP.prt[no].profile; 
}
static const wchar_t *som_map_longname(short part_no) //NEW
{
	return Sompaste->longname(som_map_profile(part_no));
}
//HACK: PROFILE MUST COME FROM som_map_profile
static const wchar_t *som_map_description(const wchar_t *profile)
{
	SOM_MAP_prt::key *nul = (SOM_MAP_prt::key*)0;
	assert(nul->description-nul->profile==15);
	return profile+15;
}
static WORD som_map_workshop_number(const wchar_t *profile)
{
	SOM_MAP_prt::key *nul = (SOM_MAP_prt::key*)0;
	assert(nul->profile-(wchar_t*)nul==31);
	return ((SOM_MAP_prt::key*)(profile-31))->number();
}
							   
//REMOVE ME?
static int som_map_zentai_new()
{
	for(int i=10;i<1024;i++) if(!*SOM_MAP.ezt[i]) return i;
	return -1; 
}
static void som_map_zentai_name(size_t name, wchar_t *x, size_t x_s)
{
	if(name<1024)
	EX::need_unicode_equivalent(932,SOM_MAP.ezt[name],x,x_s);
	else assert(0);
}
static void som_map_zentai_rename(size_t name, wchar_t *x, size_t x_s)
{
	if(name<1024)
	EX::need_ansi_equivalent(932,x,SOM_MAP.ezt[name],31);
	else assert(0);
}

static RECT som_map_FrameRgn_palette;
static HWND som_map_FrameRgn_AfxWnd42s = 0;
extern HBRUSH som_tool_red = CreateSolidBrush(0xFF);
extern HBRUSH som_tool_yellow = CreateSolidBrush(0xFFFF);
extern RECT &workshop_picture_window = som_map_FrameRgn_palette;
static BOOL WINAPI som_map_FrameRgn(HDC A,HRGN B,HBRUSH C,int D,int E)
{
	if(som_map_FrameRgn_AfxWnd42s)
	{
		assert(SOM::tool==SOM_MAP.exe);

		SOM_TOOL_DETOUR_THREADMAIN(FrameRgn,A,B,C,D,E)

		EXLOG_LEVEL(7) << "som_tool_FrameRgn()\n";
		
		//Reminder: Sandwich the overlay between the cursor.
		if(SOM_MAP.painting==som_map_FrameRgn_AfxWnd42s)
		{
			extern void SOM_MAP_sandwich_overlay(HDC,HWND);
			SOM_MAP_sandwich_overlay(A,som_map_FrameRgn_AfxWnd42s); 
		}

		LOGBRUSH lb;
		if(GetObject((HGDIOBJ)C,sizeof(lb),&lb))
		{
			switch(lb.lbColor)
			{
			default: assert(0); goto other; //???
			case 0xFF: case 0xFFFF:; //red/yellow
			}

			if(SOM_MAP.palette==som_map_FrameRgn_AfxWnd42s)
			{
				//REMOVE ME?
				//Perfectionism: som_map_refresh_model_view?
				if(1&&0x200&som_map_tileviewmask)
				{
					RECT fake = som_map_FrameRgn_palette;
					GetRgnBox(B,&som_map_FrameRgn_palette);
					SetRectRgn(B,fake.left,fake.top,fake.right,fake.bottom);				
				}
				else GetRgnBox(B,&som_map_FrameRgn_palette);
			}

			HWND f = GetFocus();			
			//synchronize keyboard input (arrow keys mainly)
			if(f==som_map_FrameRgn_AfxWnd42s&&lb.lbColor!=0xFF)
			{			
				if(~GetKeyState(VK_HOME)>>15)
				{
					keybd_event(VK_HOME,0,0,0);
					keybd_event(VK_HOME,0,KEYEVENTF_KEYUP,0);
				}
			}
			else if(f==0&&lb.lbColor==0xFF) //hack: steal focus
			{
				SetFocus(som_map_FrameRgn_AfxWnd42s);
			}
						
			//swap the colors used by SOM_MAP (elsewhere yellow means focus)
			C = f==som_map_FrameRgn_AfxWnd42s?som_tool_yellow:som_tool_red;
		}
	}
other: return SOM::Tool.FrameRgn(A,B,C,D,E);
}
static VOID CALLBACK som_map_opentileview(HWND hWnd, UINT, UINT_PTR id, DWORD)
{
	KillTimer(hWnd,id);
	SendMessage(som_tool,WM_COMMAND,1001,(LPARAM)GetDlgItem(som_tool,1001)); 
}	
static VOID CALLBACK som_map_VK_CONTROL(HWND hWnd, UINT, UINT_PTR id, DWORD)
{
	KillTimer(hWnd,id); 
	keybd_event(VK_CONTROL,0,id=='ctrl'+'v'?0:KEYEVENTF_KEYUP,0);
}
extern SHORT __stdcall SOM_MAP_GetKeyState_Ctrl(DWORD=0);
extern POINTS som_tool_tile = {}, som_tool_tilespace = {-1};
static LRESULT CALLBACK som_map_AfxWnd42sproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR hlp)
{
	switch(uMsg)
	{	
	case WM_CONTEXTMENU: //Exselector?
		
		return 0; //2022: annoying scrollbar menus

	case WM_GETDLGCODE: //prevents auto-processing of mnemonics!

		return DLGC_WANTCHARS|DefSubclassProc(hWnd,uMsg,wParam,lParam);

	case WM_PAINT: 
	{	
		som_map_FrameRgn_AfxWnd42s = hWnd;
		
		LRESULT out;		
		/*if(hWnd==SOM_MAP.palette) 
		out = ((WNDPROC)0x4684ed)(hWnd,uMsg,wParam,lParam);		
		else*/ out = DefSubclassProc(hWnd,uMsg,wParam,lParam);
		som_map_FrameRgn_AfxWnd42s = 0;
		
		//ensure border isn't pure black
		//note: this is the ONLY thing that works
		//tried WM_NCPAINT etc. might be better to remove 
		//the borders and draw them in the parent's WM_ERASEBKND
		HWND gp = GetParent(hWnd); HDC dc = GetDC(gp);
		RECT wr; GetWindowRect(hWnd,&wr); MapWindowRect(0,gp,&wr);
		static HBRUSH grey = CreateSolidBrush(0x646464); 
		FrameRect(dc,&wr,grey); 		
		ReleaseDC(gp,dc);
		return out;
	}	
	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,som_map_AfxWnd42sproc,id);		
		break;

	case WM_KILLFOCUS: //IMPORTANT //BLACK MAGIC //IMPORTANT

		//"case WM_NCLBUTTONDOWN: //iconify" depends on this

		//there's something funny about the AfxWind42s window
		//it makes SOM_MAP the active window before the mouse
		//click message is sent 
		if(GetActiveWindow()!=GetAncestor(som_tool_dialog(),GA_ROOT)) 
		{
			//MessageBeep(-1); //FlashWindow(som_tool_dialog(),1);
			return 1;
		}
		break;

	case WM_MOUSEMOVE: //2021

		//HACK? workshop_subclassproc does something like this to aid
		//the do_aa. just as a reminder it looked alright without this
		if(hWnd==(som_tool_stack[1]?som_map_tileviewport:SOM_MAP.model))
		{
			extern unsigned workshop_antialias;
			if(GetKeyState(VK_LBUTTON)>>15||GetKeyState(VK_RBUTTON)>>15)
			workshop_antialias = 1;
		}
		break;
	
	case WM_MBUTTONDOWN: //2021: send 0 wheel to scroll3D?
		
		wParam = 0; uMsg = WM_MOUSEWHEEL; goto scroll3D;

	case WM_MOUSEWHEEL: 
	
		//Reminder: Logitech mice have a bug where HWHEEL must return 1
		//or else global Logitech hooks emulate a wheel, incorrectly so 
		//
		case 0x020E: //WM_HMOUSEWHEEL:

		if(GetKeyState(VK_SHIFT)>>15) uMsg = 0x20E;
		if(SOM_MAP.palette==hWnd) //just refresh the MSM view port
		SetTimer(hWnd,uMsg,100,som_map_mousewheel);
		scroll3D: //WM_MBUTTONDOWN?
		if(hWnd==(som_tool_stack[1]?som_map_tileviewport:SOM_MAP.model)) //2021
		{
			//EXPERIMENTAL
			extern void SOM_MAP_scroll3D(WPARAM,bool);
			SOM_MAP_scroll3D(wParam,uMsg==WM_MOUSEWHEEL);
			wParam = 0; //...
		}
		if(wParam) //WM_MBUTTONDOWN?
		return som_tool_mousewheel(hWnd,wParam,uMsg==WM_MOUSEWHEEL);
		return 1;
	}

	if(hWnd==SOM_MAP.palette) switch(uMsg)
	{
	case WM_KEYDOWN:

		//0: relying on som_map_FrameRgn 
		if(wParam==VK_HOME) SetFocus(0); 
		if(wParam==VK_SPACE) goto workshop;
		break;

	case WM_LBUTTONDBLCLK: workshop:

		SendMessage(som_tool,WM_COMMAND,1010,0);
		break;

	case WM_LBUTTONDOWN: case WM_RBUTTONDOWN: 
		
		if(GetFocus()!=hWnd) SetFocus(hWnd);			
		break; 
				
	case WM_SETFOCUS: case WM_KILLFOCUS: //overkill: invalidating cursor
			
		InvalidateRect(hWnd,0,0); break;   
	}
	//2017: -1,-1 is trying to twart an issue with being thrown into the 
	//bottom-right corner. I can't think what else might be causing this
	//static POINTS tile, tileview = {-1};
	POINTS &tile = som_tool_tile, &tileview = som_tool_tilespace;
	//POSSIBLY THIS CONTROL BELIEVES THAT
	//ITS SCROLLBAR UNITS ARE FONT GLYPHS
	//(IF SO, THE MECHANISM IS A MYSTERY)
	const int sep = 10; //WHY NOT 21?????
	if(hWnd==SOM_MAP.painting) switch(uMsg)
	{	
	case WM_KEYDOWN:

		//0: relying on som_map_FrameRgn 
		if(wParam==VK_HOME) SetFocus(0); 

		/*2021: I'm combining these since it's
		//hard to press Enter left handed, but
		//it doesn't work here, so is moved to
		//som_map_dispatch
		if(wParam==VK_SPACE||wParam==VK_RETURN)
		{
			if(SOM_MAP_GetKeyState_Ctrl())
			{
				som_map_opentileview(0,0,0,0);
				return 1;
			}
			else wParam = VK_RETURN; //lays tile
		}*/

		break;

	/*doesn't work. might try Detours on BeginPaint?
	case WM_KEYUP: //Print Screen doesn't seen WM_KEYDOWN

		if(wParam==VK_SNAPSHOT)
		{
			extern void SOM_MAP_print(HWND); 
			
			if(EX::debug){ SOM_MAP_print(hWnd); return 0; }
		}
		break;*/

	case WM_LBUTTONDBLCLK: 
		
		som_tool_pushed = hWnd;

		if((DWORD&)tile==lParam) //???
		if(SOM_MAP_GetKeyState_Ctrl())
		{
			SetTimer(hWnd,'open',0,som_map_opentileview);						
			return 1;
		}
		break;

	//case WM_LBUTTONUP: //2021: SetFocus???
	case WM_LBUTTONDOWN: 			
	
		if(GetFocus()!=hWnd) SetFocus(hWnd);  		

		//ATTENTION SOM_MAP_GetKeyState_Ctrl
		//
		// 2021: THIS IS MAJORLY OVEHAULED TO 
		// WORK WITH SOM_MAP_GetKeyState_Ctrl
		// via SOM_MAP_reprogram
		//
		if(som_map_tab!=1216&&SOM_MAP_GetKeyState_Ctrl())
		{
			tile = (POINTS&)lParam; //!!

			if(!SOM::Tool::dragdetect(hWnd,WM_LBUTTONDOWN))
			{
				//SOM_MAP checks 0x80! apparently the documentation
				//is screwy here
				//som_tool_VK_CONTROL = 0x8000;
				som_tool_VK_CONTROL = 0xff80;			 
				DefSubclassProc(hWnd,WM_LBUTTONDOWN,wParam,lParam);
				DefSubclassProc(hWnd,WM_LBUTTONUP,wParam,lParam);
				som_tool_VK_CONTROL = 0;
			}
			else
			{
				som_tool_VK_CONTROL = 1;
				DefSubclassProc(hWnd,WM_LBUTTONDOWN,wParam,lParam);
				if(!GetCapture())
				som_tool_VK_CONTROL = 0; //PARANOIA
			}
			return 1;
		}
		break; 
			
	case WM_RBUTTONDOWN: 
	
		if(GetFocus()!=hWnd) SetFocus(hWnd); 
	//	if(som_map_tab!=1220) //TODO! checkpoints?
		if(SOM::Tool::dragdetect(hWnd,WM_RBUTTONDOWN)) 
		{
			int hmin, hmax, vmin, vmax;
			GetScrollRange(hWnd,SB_HORZ,&hmin,&hmax); //100
			GetScrollRange(hWnd,SB_VERT,&vmin,&vmax); //99

			tile = (POINTS&)lParam; 
			tileview.x = GetScrollPos(hWnd,SB_HORZ);
			tileview.y = GetScrollPos(hWnd,SB_VERT);
			tile.x = tile.x/sep+tileview.x; tile.y = tile.y/sep+tileview.y;
			SetCapture(hWnd); return 0;
		}
		else 
		{
			PostMessage(hWnd,WM_RBUTTONUP,wParam,lParam); //hack		

			if(som_map_tab==1216) //set start point?
			{
				//WARNING
				//historically the offsets are retained, however
				//that is confusing for changing layers, and the
				//relative offset cannot be reconstructed. there
				//is no way to say that resetting to 0,0,0 isn't
				//desirable				
				memset(SOM_MAP_4921ac.start.xzy,0x00,3*4);
				SOM_MAP_4921ac.start.ex_zindex = SOM_MAP_z_index;
			}
		}
		break; 

	case WM_RBUTTONUP: 
		
	//	if(som_map_tab!=1220) //TODO! checkpoints?
		if(GetCapture()==hWnd) 
		{	
			ReleaseCapture(); //return 0;
		}
		break;

	case WM_CAPTURECHANGED: 

		if(lParam!=(LPARAM)hWnd)
		som_tool_VK_CONTROL = 0; //2021: SOM_MAP_GetKeyState_Ctrl? 

		//2017: -1,-1 is trying to twart an issue with being thrown into the 
		//bottom-right corner. I can't think what else might be causing this
		tileview.x = -1; break;

	case WM_MOUSEMOVE: //right-dragging?

		if(-1!=tileview.x) //this--the following--was a nightmare to devise!
		{	
			assert(GetCapture()==hWnd&&wParam&MK_RBUTTON);

			//HACK: having stability issues all of a sudden in 2018 where for 
			//a single frame the scroll position bounces back and forth. last
			//night Windows 10 updated itself. this isn't a fix but is stable
			//for the most part, and seems a safe/conservative course to take
			enum{ l=1 }; 

			//REMINDER: LOGICALLY SEP SHOULD BE 21; AN INVESTIGATION IS NEEDED
			int xp = GET_X_LPARAM(lParam); int h = GetScrollPos(hWnd,SB_HORZ); 
			int yp = GET_Y_LPARAM(lParam); int v = GetScrollPos(hWnd,SB_VERT);
			int x = (int)tile.x-(xp/sep+h)+tileview.x;
			if(x<0){ if(h) SendMessage(hWnd,WM_HSCROLL,SB_TOP,0); }
			if(x>=0) if(0) SendMessage(hWnd,WM_HSCROLL,MAKEWPARAM(SB_THUMBPOSITION,x),0);
			else if(h<x) for(h+=/*xp%sep>sep/2*/l;h++<x;SendMessage(hWnd,WM_HSCROLL,SB_LINEDOWN,0));
			else if(h>x) for(h-=/*xp%sep<=sep/2*/l;h-->x;SendMessage(hWnd,WM_HSCROLL,SB_LINEUP,0));				
			int y = (int)tile.y-(yp/sep+v)+tileview.y;
			if(y<0){ if(v) SendMessage(hWnd,WM_VSCROLL,SB_TOP,0); }
			if(y>=0) if(0) SendMessage(hWnd,WM_VSCROLL,MAKEWPARAM(SB_THUMBPOSITION,y),0);
			else if(v<y) for(v+=/*yp%sep>sep/2*/l;v++<y;SendMessage(hWnd,WM_VSCROLL,SB_LINEDOWN,0));
			else if(v>y) for(v-=/*yp%sep<=sep/2*/l;v-->y;SendMessage(hWnd,WM_VSCROLL,SB_LINEUP,0));
		}
		break;
			
	case WM_SETFOCUS: case WM_KILLFOCUS: //overkill: invalidating cursor
		
		InvalidateRect(hWnd,0,0); break;  
	}

	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}

static HWND WINAPI som_tool_CreateWindowExA(DWORD A,LPCSTR B,LPCSTR C,DWORD D,int E,int F,int G,int H,HWND I,HMENU J,HINSTANCE K,LPVOID L)
{
	if(!K) K = (HINSTANCE)0x00400000;	
	else if(K!=(HINSTANCE)0x00400000)
	return SOM::Tool.CreateWindowExA(A,B,C,D,E,F,G,H,I,J,K,L);

	SOM_TOOL_DETOUR_THREADMAIN(CreateWindowExA,A,B,C,D,E,F,G,H,I,J,K,L)

	EXLOG_LEVEL(7) << "som_tool_CreateWindowExA()\n";
	
	HWND out = 0;

	if((DWORD)B>0xFFFF) if(*B=='E'&&!strcmp(B,"EDIT"))
	{
		//assuming a listview cell's input edit box
		assert(som_tool_listview()==GetClassAtom(I));
		
		//NOTE: LVS_EX_DOUBLEBUFFER is needed
		//cancel or no cancel. The flicker is
		//probably something SOM_PRM is doing
		//Maybe it is because I is disabled??
		bool cancel = GetKeyState(VK_CONTROL)>>15;

		HWND lv = I; if(0&&EX::debug) 
		{
			if(cancel) goto cancel;
			//this is likely the best way, but it's 
			//not happening so easily
			//(have tried various styles and WS_POPUP
			//without eliminating flicker)
			//Might need to handle LVN_BEGINLABELEDIT? 
			out = ListView_GetEditControl(lv);
			//parent is 0
			//SetParent(out,I); //I = GetParent(out);			
			//NOTE: DOUBLEBUFFER eliminates flicker where the edit 
			//boxes are overlaid onto the shop/inventory subitems
		}	//but makes FULLROWSELECT have an ugly lasso				
		else ListView_SetExtendedListViewStyle(lv,LVS_EX_DOUBLEBUFFER);		
		if(cancel) cancel:
		{
			EnableWindow(I,1); SetFocus(I);
			//DICEY but works on Windows 10
			return 0;
		}
		if(!out)
		out = CreateWindowExW(A,L"EDIT",0,D|ES_NUMBER,E,F,G,H,I,J,K,L);
		som_tool_editcell(out,lv);		
	}		
	else if(*B=='A'&&!strcmp(B,"AfxWnd42s"))
	{
		/*causes border to flicker???
		//fixing in DDRAW::window_rect
		//2021: GetWindowRect distorts the viewports
		if(D&WS_BORDER)
		{
			D&=~WS_BORDER; E+=2; F+=2; G-=4; H-=4; //1 //2
		}*/

		//assuming one of SOM's custom drawing areas
		out = CreateWindowExW(A,L"AfxWnd42s",0,D,E,F,G,H,I,J,K,L);

		int after = 0, tabstop = 0;

		if(out&&SOM::tool==SOM_MAP.exe)
		{
			if(!som_map_tab) som_map_tab = 1215; //initialize

			if(!SOM_MAP.model)
			{
				SOM_MAP.model = out; after = 1012;
			}			
			else if(!SOM_MAP.palette)
			{
				SOM_MAP.palette = out; after = 1013; tabstop = WS_TABSTOP;
			}
			else if(!SOM_MAP.painting)
			{
				SOM_MAP.painting = out; after = 1011; tabstop = WS_TABSTOP;
			}
			//som_map_tileviewport
			//som_tool_InitDialog does GetWindow(GW_HWNDLAST)
			//else after = 0s; //1053 
		}
		else assert(0);

		if(after) //GetDlgItem fails for 1053 (pre-deleted?)
		SetWindowPos(out,GetDlgItem(I,after),0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);				

		if(!tabstop) D&=~tabstop; 
		SetWindowLong(out,GWL_STYLE,D|WS_GROUP|tabstop);
		som_tool_subclass(out,som_map_AfxWnd42sproc);
	}
	else if(!som_tool&&SOM::tool>=EneEdit.exe)
	{	
		//EneEdit/NpcEdit are unique in not doing this. Could be an MFC
		//thing? They don't have an MFC icon, and may not use it at all
		//they don't CreateDialogIndirect either
		assert(!som_tool_cbthook&&!som_tool_msgfilterhook); 
		som_tool_cbthook = 
		SetWindowsHookEx(WH_CBT,som_tool_cbtproc,0,GetCurrentThreadId());
		som_tool_msgfilterhook = 
		SetWindowsHookEx(WH_MSGFILTER,som_tool_msgfilterproc,0,GetCurrentThreadId());

		LPCSTR rsrc = MAKEINTRESOURCEA(102);
		if(SOM::tool==SfxEdit.exe) rsrc = "X_102C";
		HRSRC found = FindResourceA(0,rsrc,(char*)RT_DIALOG);
		if(DLGTEMPLATE*locked=(DLGTEMPLATE*)LockResource(LoadResource(0,found)))
		{
			extern windowsex_DIALOGPROC workshop_102;			
			I = som_tool_CreateDialogIndirectParamA(0,locked,0,workshop_102,0);
			if(workshop_host=GetDlgItem(I,1014))
			{
				RECT &rc = workshop_picture_window;
				//RECT rc; GetClientRect(I,&rc);
				E = rc.left; F = rc.top; G = rc.right-E; H = rc.bottom-F;
				J = (HMENU)1014;
			}
			D|=WS_CHILD; D&=~(WS_POPUP|WS_OVERLAPPEDWINDOW);
		}
		else //legacy? make icon able to be hidden
		{
			D = DS_MODALFRAME|WS_SYSMENU|WS_MINIMIZEBOX;
			A|=WS_EX_DLGMODALFRAME;
		}

		//old way... for some reason returning 1014 breaks the menu???
		//Yes a child can't have a menu, but notice	D|=WS_CHILD works!
		if(1) 
		{
			som_tool_initializing++;
			const wchar_t *wB = EX::need_unicode_equivalent(932,B);
			if(!workshop_title) workshop_title = wcsdup(wB);
			som_tool = CreateWindowExW(A,wB,wB,D,E,F,G,H,I,J,K,L);						
			//doesn't work here. maybe because hooking CreateWindowEx?
			//som_tool_icon(som_tool);
			if(I)
			{
				DestroyWindow(workshop_host);
				workshop_host = som_tool;
				som_tool = I;				
			}
			else //in this case, the host is the view only
			{
				workshop_host = som_tool;
				//Sompaste is offcenter without splash???
				//Sompaste->center(som_tool,SOM::splash(),false);
			}
			/*if(1)
			{
				WNDCLASSEXW wc; wc.cbSize = sizeof(WNDCLASSEX);					
				GetClassInfoExW(GetModuleHandleW(0),wB,&wc);		
				som_tool_initializing++;
				som_tool_CreateDialogProc = (DLGPROC)wc.lpfnWndProc;
				som_tool_InitDialog(som_tool,WM_INITDIALOG,0,0);
				som_tool_initializing--;	
			}*/		
			//HACK: this is being used to initialize various things
			workshop_SetWindowTextA(workshop_host,B);
			som_tool_initializing--;
		}

		int hlp = GetWindowContextHelpId(som_tool);
		SetWindowSubclass(workshop_host,workshop_subclassproc,1014,hlp);
		return workshop_host;		
	}

	if(!out) out = CreateWindowExW(A,
	EX::need_unicode_equivalent(932,B),0,D,E,F,G,H,I,J,K,L);
	//som_tool_SetWindowTextA (below)
	if(C) SetWindowTextA(out,C); return out;
}

//B is reconstituted on the stack so we must improvise
static size_t som_tool_FindNextFileA_cursor[2] = {0,0};

//Reminder: most uses of FindFirstFile constitute abuses
static HANDLE WINAPI som_tool_FindFirstFileA(LPCSTR A, LPWIN32_FIND_DATAA B)
{	
	SOM_TOOL_DETOUR_THREADMAIN(FindFirstFileA,A,B)

	EXLOG_LEVEL(7) << "som_tool_FindFirstFileA()\n";

	EXLOG_LEVEL(3) << "Finding "<< A << '\n';

	if(SOM::tool>=PrtsEdit.exe) //e.g. 60100 
	{
		//doing just so SOMEX_(A) doesn't make
		//it into user32.dll
		if(*(DWORD*)A==*(DWORD*)SOMEX_(A))
		{
			//PrtsEdit checks for INVALID_HANDLE_VALUE but also
			//treats 0 suspiciously... not calling FindNext/FindClose
			//but it adds a blank item to the listbox, which is enough to
			//go on
			return INVALID_HANDLE_VALUE;
		}
		else assert(*(DWORD*)A==*(DWORD*)SOMEX_(A));		
	}

	//UNFINISHED: EX::user hides the project folder
	//UNFINISHED: EX::user hides the project folder
	//UNFINISHED: EX::user hides the project folder
	//UNFINISHED: EX::user hides the project folder
	const wchar_t *w = SOM::Tool::file(A); 

	if(!w&&SOM::tool==SOM_EDIT.exe) w = SOM::Tool::project2(A);

	if(!w) return SOM::Tool.FindFirstFileA(A,B);

	//multimedia files:
	//"*" was returned by SOM::Tool::project
	if(*w=='*') return INVALID_HANDLE_VALUE; 

	WIN32_FIND_DATAW dw; 

	/*static*/ wchar_t vname[MAX_PATH];
	if(!wcsnicmp(w,L"DATA\\",5)) //NEW
	{			
		size_t count = 0; 

		assert(SOM::tool!=SOM_EDIT.exe);

		const wchar_t *slash = wcschr(w+5,'\\');
		
		if(slash&&!wcsnicmp(slash+1,L"prof\\",5))
		{
			assert(SOM::tool==SOM_PRM.exe);

			som_prm tab(w+5); count = tab.count();	

			som_tool_FindNextFileA_cursor[1] = 0;
		}
		else if(slash&&!wcsnicmp(slash+1,L"parts\\",6))
		{	 			
			assert(SOM::tool==SOM_MAP.exe);

			count = SOM_MAP.prt.keys(); //initialize

			som_tool_FindNextFileA_cursor[1] = count;
		}
		else slash = 0; if(slash++) //!!
		{
			assert(wcschr(slash,'*'));

			/*won't work B is recreated in a loop*/
			if(!count) return INVALID_HANDLE_VALUE;

			som_tool_FindNextFileA_cursor[0] = 0;				
			som_tool_FindNextFileA(0,B);

			return 0; //dummy handle
		}
		else //SOM_PRM: DATA\\SFX\SFX.DAT (sic)
		{	
			assert(w[4]=='\\'&&w[5]=='\\');			
			//2018: assuming not returning this
			//static wchar_t v[MAX_PATH] = L"";
			if(EX::data(w+5,vname))
			w = vname; 
			//2018: looks catostrophic??? what to do instead?
			//if(w!=vname) w = 0; //assert(w);
			if(w!=vname){ assert(0); return INVALID_HANDLE_VALUE; }
		}		
	}

	assert(w); //2018

	EXLOG_LEVEL(3) << "Widened to "<< w << '\n';
							 
	HANDLE out = FindFirstFileW(w,&dw);

	if(out==INVALID_HANDLE_VALUE) 
	{
		wchar_t *w_ext = PathFindExtensionW(w);

		if(!wcsicmp(w_ext,L".evt"))
		SOM_MAP.ezt.create_evt_for_splicing(w);
		else 
		if(wcsicmp(w_ext,L".pro") 
		||som_tool_file_appears_to_be_missing(A,w,w_ext))
		{
			return out; 
		}		
		out = FindFirstFileW(w,&dw);
		if(out==INVALID_HANDLE_VALUE) //paranoia
		{
			assert(0); return out;
		}
	}
	memcpy(B,&dw,sizeof(*B)); 
	//for(size_t i=0;i<sizeof(B->cAlternateFileName);i++) 
	//B->cAlternateFileName[i] = dw.cAlternateFileName[i];
	for(size_t i=0;i<sizeof(B->cFileName);i++)
	B->cFileName[i] = dw.cFileName[i];	
	return out;
}

static BOOL WINAPI som_tool_FindNextFileA(HANDLE A, LPWIN32_FIND_DATAA B)
{
	SOM_TOOL_DETOUR_THREADMAIN(FindNextFileA,A,B)
		
	if(A) //dummy handle?
	{
		return SOM::Tool.FindNextFileA(A,B);
	}
	else if(SOM::tool>=PrtsEdit.exe)
	{
		//HACK: PrtsEdit disregards 0 as a handle
		//(but it does create an empty list item)
		//assert(A&&(int)A==som_tool_initializing); 
		assert(0);
		
		no_more_files:
		return(SetLastError(ERROR_NO_MORE_FILES),FALSE);
	}
	
	EXLOG_LEVEL(7) << "som_tool_FindNextFileA()\n"; //could be gratuitous
	
	/*todo: SOM probably loops over these once just to count its profiles*/

	size_t (&cursor)[2] = som_tool_FindNextFileA_cursor;

	if(SOM::tool==SOM_MAP.exe) 
	{
		if(cursor[0]>=cursor[1]) return FALSE;

		size_t key = cursor[0]++;
		
		int pn = SOM_MAP.prt[key].part_number(); 
		
		if(pn>1023) SOM_MAP.map.legacy = false;

		return 0<sprintf(B->cFileName,"%d@%d",pn,key);	
	}
	else if(SOM::tool!=SOM_PRM.exe) return FALSE; //!
	
	som_prm tab; som_prm::group g = tab(cursor[0]); 

	while(cursor[1]>=g.count&&cursor[0]<tab.sorts) 
	{
		g = tab(++cursor[0]); cursor[1] = 0;
	}		
	if(cursor[0]>=tab.sorts) goto no_more_files;

	if(g+cursor[1]==tab.separator)
	{
		cursor[1]++; return som_tool_FindNextFileA(A,B);
	}
	return 0<sprintf(B->cFileName,"@%d-%d",cursor[0],cursor[1]++);	
}

static BOOL WINAPI som_tool_FindClose(HANDLE in)
{	
	SOM_TOOL_DETOUR_THREADMAIN(FindClose,in)

	EXLOG_LEVEL(7) << "som_tool_FindClose()\n";

	if(!in) return TRUE; //som_tool_FindNextFileA_cursor

	/*
	if(SOM::tool>=PrtsEdit.exe&&(int)A==som_tool_initializing)
	{
		//HACK: PrtsEdit disregards 0 as a handle
		//(but it does create an empty list item)
		return TRUE;
	}*/

	return SOM::Tool.FindClose(in);
}

static bool som_tool_mapcomp(const wchar_t *w) //helper
{														 
	if(SOM::tool==MapComp.exe) 
	{
		const wchar_t *_prt = wcsrchr(w,'.'); 
		
		if(!_prt||wcsicmp(_prt,L".prt")) return true;			
		//if(_prt-w<4||!isdigit(_prt[-4])) return false;
		assert(_prt-w>=4&&isdigit(_prt[-4]));
			
		size_t key;
		/*2018: map layer indexing flattens this out
		if(SOM_MAP.map.version<13) 
		{
			key = SOM_MAP.prt[_prt-4].number(); 
		}
		else key = SOM_MAP.prt[_wtoi(_prt-4)].index;*/
		key = SOM_MAP.prt[_wtoi(_prt-4)].index2()[1];
		
		assert(key!=SOM_MAP.prt.missing);

		swprintf((wchar_t*)_prt-4,L"@%d",key);
	}	
	return true;
}			 

//2018: making available to workshop_exe 
extern int som_tool_profile(wchar_t* &io, int minor, int major=-1)
{
	const wchar_t *prof = 0; //returned as courtesy 

	if(major==-1) //SOM_MAP
	{
		swprintf(io,L"map/%lc",L'0'+minor);
		prof = SOM_MAP.prt[minor].profile;
	}
	else //SOM_PRM
	{
		som_prm tab; 
		if(tab(major).profile(minor,io))						
		prof = tab[tab(major)+minor].profile;
	}
								
	int sep = *io?Sompaste->database(0,io,0,0):0; 

	if(sep&&!io[sep+1]) //w=v
	wcscpy(io+sep,Sompaste->longname(io[sep])); //NEW 

	io = const_cast<wchar_t*>(prof); return sep;
}
extern HANDLE som_sys_dat = 0;
static HANDLE WINAPI som_tool_CreateFileA(LPCSTR in, DWORD A, DWORD B, LPSECURITY_ATTRIBUTES C, DWORD D, DWORD E, HANDLE F)
{	
	assert(in);
	if(/*in&&*/*in=='\\'&&!strncmp(in,"\\\\?\\",4)) //device?
	{
		return SOM::Tool.CreateFileA(in,A,B,C,D,E,F);
	}
	SOM_TOOL_DETOUR_THREADMAIN(CreateFileA,in,A,B,C,D,E,F)

	EXLOG_LEVEL(7) << "som_tool_CreateFileA()\n";

	//read rewrites are done by SOM::Tool::file (project) below
	if(A==GENERIC_WRITE) if(SOM_MAP.exe==SOM::tool) 
	{
		//historically SOM_MAP had opened to the first map
		const char cmp[] = SOMEX_(B) "\\data\\map\\";
		if(!memcmp(in,cmp,sizeof(cmp)-1)) //avoid sys.dat
		{
			//Reminder: the map menu has a Copy & Paste function
			if(41000!=GetWindowContextHelpId(som_tool_stack[1]))
			{
				//2022: somehow SOM_MAP_map2_internal seemed to
				//work even though it was failing to read/write
				//to the base map when outputting to layer maps
				A|=GENERIC_READ;

				const wchar_t *map = Sompaste->get(L".MAP");
				int base = _wtoi(map);
				switch(in[sizeof(cmp)+3])
				{
				default: assert(0);
				case 'a': break; //MAP?
				case 'v': case 'p': //EVT/MPX?
					int i = som_LYT_base(base);
					if(i!=-1) base = i;				
				}
				(char&)in[sizeof(cmp)-1] = '0'+base/10;
				(char&)in[sizeof(cmp)+0] = '0'+base%10;
				SOM_MAP.map.current = _wtoi(map); //YUCK
			}
		}
	}
	else if(som_prm_tab==1003) //2018: don't remove sword magic
	{
		//1003 only writes item.prm to remove sword magics at the
		//drop of a hat; returning an EX::temporary
		if(!strcmp(in,SOMEX_(B)"\\PARAM\\Item.prm"))
		return SOM::Tool.Mapcomp_MHM(0);
	}
	
	EXLOG_LEVEL(3) << "Creating "<< in << '\n';		

	const wchar_t *w;
	wchar_t vname[MAX_PATH];
	extern wchar_t *som_art_CreateFile_w; //2021
	extern wchar_t *som_art_CreateFile_w_cat(wchar_t[],const char*);
	if(som_art_CreateFile_w)
	{
		w = som_art_CreateFile_w_cat(vname,in);		
	}
	else 
	{
		w = SOM::Tool::file(in); 

		if(!w) switch(SOM::tool)
		{
		case SOM_RUN.exe: w = SOM::Tool.runtime(in); break;
		case SOM_EDIT.exe: w = SOM::Tool.project2(in); break;
		}

		if(w&&!wcsnicmp(w,L"DATA\\",5)) 
		{
			//2018: assuming not returning this
			//static wchar_t v[MAX_PATH] = L"";
			wchar_t *const v = vname;
			
			if(!som_tool_mapcomp(w)) return 0;

			//@: som_tool_FindNextFileA
			const wchar_t *profile = wcsrchr(w,'@');

			if(profile++) //@
			{
				wchar_t *e;
				int minor,major = wcstol(profile,&e,10);
				if(e!=profile)
				{				 
					if(*e!='-') //PRF?
					{
						minor = major; major = -1;
					}
					else minor = wcstol(e+1,&e,10); //PRT

					wchar_t *prof = v;
					if(int sep=som_tool_profile(prof,minor,major)) 
					{
						w = v;

						//hack: locate in language pack
						char *p = (char*)strrchr(in,'\\')+1;					
						EX::need_ansi_equivalent(65001,w+sep,p,MAX_PATH-(p-in));
					}
					som_tool_reading_mode[som_tool_reading] = prof;
				}
			}
			if(w!=v&&EX::data(w+5,vname))
			{
				w = vname; //v?
			}

			//this is thwarting som_tool_file_appears_to_be_missing
			//it doesn't help that PathFileExistsA thinks SOMEX_(A)
			//style path "exist" in 2022
			//if(w!=v) w = 0; //assert(w);
		}
	}
	
	wchar_t *w_ext = w?(wchar_t*)wcsrchr(w,'.'):0; 	 	
	int w_e = w_ext?tolower(w_ext[1]):0;

	switch(D) //skirt missing test when creating anew
	{
	case CREATE_ALWAYS: case CREATE_NEW: case OPEN_ALWAYS: 

		//FYI: only known cases here are Project.DAT and Project.SOM

		//also XX.map and XX.evt and Sys.dat by way of CREATE_ALWAYS
		//this may only occur when first discovered by FindFirstFile
	
		if(w_e=='m') 
		if(SOM_MAP.exe==SOM::tool&&!wcsicmp(w_ext,L".map"))
		{			
			//this is just to satisfy the naysayers
			if(!SOM_MAP.map.legacy
			&&SOM_MAP.map.versions[SOM_MAP.map.current]<13)
			{			
				//REMOVE ME?
				//(if we ever have the map in memory we can check it right here)				
				if(IDYES==EX::messagebox(MB_YESNO,
				"This project has included map pieces which if present in this map will require upconversion to a version 13 MAP file.\n"
				"\n"
				"Choose YES to convert the map at this time. Choosing NO will fail if an incompatible map piece is present within the map."))
				{
					SOM_MAP.map.versions[SOM_MAP.map.current] = 13;			
				}			  
				else if(SOM::Tool->CloseHandle)
				{
					wcscpy(w_ext,L".MA~"); 
				}
				else assert(0);
			}			
		}
		else if(MapComp.exe==SOM::tool&&!wcsicmp(w_ext,L".mpy"))
		{
			//HACK: MapComp_main uses this HANDLE to output MPY
			return SOM::Tool.Mapcomp_MHM(0);
		}
		break; 

	default: //OPEN_EXISTING TRUNCATE_EXISTING

		if(som_tool_file_appears_to_be_missing(in,w,w_ext))
		{
			if(!w) return SOM::Tool.CreateFileA(in,A,B,C,D,E,F);

			//Note: if an alternative data folder etc is used then
			//falling thru to the install data folder would be confusing!
			//We assume if we get here, the file is part of the SOM game
			//or installation
			
			missing: 
			SetLastError(ERROR_FILE_NOT_FOUND); 			
			return INVALID_HANDLE_VALUE;			
		}

		#ifdef _DEBUG
		if(w_e=='m'&&'d'==tolower(w_ext[2])) //MapComp?
		{
			w_e = w_e; //breakpoint
		}
		#endif
		
		//HACK: better to look for map/m*m?
		if(w_e=='m'&&'m'==tolower(w_ext[3])) switch(tolower(w_ext[2]))
		{
		case 's': //msm		
		{
			EX::INI::Editor ed;
			if(SOM::tool!=MapComp.exe)
			{
				if(SOM::tool!=SOM_MAP.exe
				||0==ed->tile_view_subdivisions)
				break;
			}
			else if(ed->do_subdivide_polygons)
			{
				break;
			}
			//do_aa2 (smoothing?)
			//TODO: I think subdivision is not incompatible
			//with do_aa2 if the splits can be made to coincide. I don't
			//know what the current metric is.
			return SOM::Tool.Mapcomp_MSM(w); 
		}
		case 'h': //mhm
			
				/*2022: map_442440_407c60 is drawing from
				//map/mhm
			//HACK: How can mapcomp.exe know if it's 
			//reading MSM or MHM files?
			if(!strstr(in,"\\map\\msm\\"))
			break;*/
			if(0x100&~som_map_tileviewmask) //2022
			break;
			return SOM::Tool.Mapcomp_MHM(w); //SOM_MAP_165
		}
	}

	if(!w) return SOM::Tool.CreateFileA(in,A,B,C,D,E,F);
		
	EXLOG_LEVEL(3) << "Widened to "<< w << '\n';
	
	HANDLE out = 0;
	if(w_e=='e'&&!wcsicmp(w_ext,L".evt"))
	{
		A = GENERIC_READ|GENERIC_WRITE;

		if(D==OPEN_EXISTING) //reading/splicing
		{
			//hack: counting on SOM_MAP.ezt::operator= to
			//interpret a temporary file as a read operation
			static EX::temporary tmp; if(CopyFileW(w,tmp,0)) //out = tmp;
			{
				out = tmp.dup(); //don't CloseHandle on tmp
			}
			else assert(0); assert(EX::is_temporary(out));
		}		   
	}
	if(w_e=='d'&&!wcsicmp(w_ext,L".dat"))
	if(w_ext-w>=3&&!wcsicmp(w_ext-3,L"Sys.dat"))
	{
		//seems to be erasing inventory/magic table :(
		if(SOM::tool==SOM_PRM.exe)
		{
			//NOTE: reads when item/magic tab is opened
			//writes always, on save (item/magic only?)
			return INVALID_HANDLE_VALUE; 
		}

		D = OPEN_EXISTING; //OpenMutex: SOM_MAP/SYS share this file

		out = som_sys_dat = CreateFileW(w,A,B,C,D,E,F);   
		assert(out!=0); //INVALID_HANDLE_VALUE
	}

	if(!out) out = CreateFileW(w,A,B,C,D,E,F);   	
	assert(out!=0); //INVALID_HANDLE_VALUE
	if(out==INVALID_HANDLE_VALUE||!SOM::Tool->ReadFile)
	return out; 

	bool (*f)(int,DWORD,DWORD,BYTE*) = 0;

	switch(w_e) 
	{
	case 'p': //prt, prf, prm			
	if(A&GENERIC_READ) //2018
	switch('r'!=tolower(w_ext[2])?0:tolower(w_ext[3]))
	{
	case 't': f = som_tool_prt; break;
	case 'f': f = som_tool_prf; break;
	case 'm':	//som_tool_prm; break;
	
		if(SOM::tool==SOM_MAP.exe)
		{
			auto *p = w_ext-1;
			auto &mode = som_tool_reading_mode[som_tool_reading];			
			while(p>w&&p[-1]!='\\'&&p[-1]!='/') p--;
			if(!wcsicmp(p,mode=L"item.prm")||!wcsicmp(p,mode=L"obj.prm")
			 ||!wcsicmp(p,mode=L"enemy.prm")||!wcsicmp(p,mode=L"npc.prm"))
			f = som_tool_prm; 
		}
		break;

	/*2021: SOM_MAP_reprogram has a fix for this
	case '2': case '~': case 'o': 
		
		//SOM_MAP bug where SOM_PRM has
		//left a 0 hole in the record lookup table
		if(SOM::tool==SOM_MAP.exe)
		{
			const wchar_t *p = w_ext-1,
			*&mode = som_tool_reading_mode[som_tool_reading];			
			while(p>w&&p[-1]!='\\'&&p[-1]!='/') p--;
			//nicmp: the extension can be pr2, pr~, or pro
			if(!wcsnicmp(p,mode=L"enemy.pr2",6)||!wcsnicmp(p,mode=L"npc.pr2",5))
			f = som_tool_pr2; 		
		}
		break;*/
	}
	break;	
	case 'm':
	if(!wcsicmp(w_ext,L".map")
	 ||!wcsicmp(w_ext,L".MA~")) //REMOVE ME?
	if(SOM::tool!=SOM_EDIT.exe) //old map import tool
	{	
		//2018: start map menu? (peeking at map title)
		//Note: doesn't care about MAP version numbers
		if(SOM::tool==SOM_SYS.exe)
		break;

		//YUCK: MapComp_LYT overwrites w_ext via
		//som_tool_CreateFileA
		int recursive = _wtoi(w_ext-2);

		if(~A&GENERIC_READ)
		{
			if(!som_tool_initializing)
			{
				//NEW: don't use SOM_MAP.map
				if(som_tool_stack[1]) break; //Paste?
			}
		}
		else if(SOM::tool==MapComp.exe) //RECURSIVE
		{
			extern bool MapComp_LYT(WCHAR[2]);
			if(!SOM_MAP.lyt&&!MapComp_LYT(w_ext-2))
			{
				CloseHandle(out); 
				return INVALID_HANDLE_VALUE;
			}
		}
		else if(!som_tool_initializing)
		{
			//NEW: don't use SOM_MAP.map
			if(som_tool_stack[1]) break; //Copy?
			
			//TODO? try to keep when switching layers
			//on same base map (harder than it sounds)
			//assert(!som_tool_stack[1]);

			/*2021: SOM_MAP_201_open does this when
			//opening blank maps. and it does extra
			//cleanup.
			//REMINDER: this was new char allocated?
			delete[] SOM_MAP.base; SOM_MAP.base = 0;*/
			assert(SOM::tool!=MapComp.exe);
		}
		else
		{
			//break maybe? SOM_MAP_map::read has the following
			//to say:
			//
			//  "//HACK: short-circuit Map selection menu"
		}

		//2019: should the Map menu be here? I don't know

		//NOTE: I've copid this to SOM_MAP_201_open in
		//2021
		SOM_MAP.map.current = recursive; //YUCK
		SOM_MAP.map = out; 
		break;
	}
	break;
	case 'e':
	if(!wcsicmp(w_ext,L".evt"))
	if(SOM::tool!=SOM_EDIT.exe) //old map import tool
	{	
		//if(!som_tool_initializing)
		{
			//NEW: don't use SOM_MAP.ezt
			if(som_tool_stack[1]) break; //Copy/Paste?
		}

		SOM_MAP.ezt = out; break;		
	}}

	if(f&&som_tool_read)
	if(som_tool_reading<som_tool_readmax)
	{	
		assert(A&GENERIC_READ); //2018

		som_tool_reading_f[som_tool_reading] = f;
		som_tool_reading_list[som_tool_reading] = out;
		som_tool_reading++;

		//2018: WTH is this???
		//if(som_tool_reading>5) //hack
		if(som_tool_reading==som_tool_readmax)
		{
			assert(0); //FIX ME?
			int paranoia = som_tool_reading;
			SOM::Tool.CloseHandle(som_tool_reading_list[0]);
			som_tool_reading_x(0);
			assert(paranoia==som_tool_reading+1);			
		}
	}
	else assert(0); return out;	
}

static BOOL WINAPI som_tool_ReadFile(HANDLE in, LPVOID to, DWORD sz, LPDWORD rd, LPOVERLAPPED o)
{
	SOM_TOOL_DETOUR_THREADMAIN(ReadFile,in,to,sz,rd,o)

	EXLOG_LEVEL(7) << "som_tool_ReadFile()\n";

	if(in==SOM_MAP.map)
	{	
		if(!SOM_MAP.map.finished)
		{
			assert(SOM::tool==SOM_MAP.exe||SOM::tool==MapComp.exe);

			//won't return a split CR/LF
			if(!SOM_MAP.map.read(to,sz,rd))			
			goto err2; 
		}	
		else if(!SOM::Tool.ReadFile(SOM_MAP.map.in2,to,sz,rd,o)) 
		{
			//DISPLAY ERROR MessageBox WITHOUT CRASH
			//SOM_MAP crashes on access violation if there is an
			//error after reading begins... it doesn't check for
			//errors, but it must surely look out for EOF state? 
			err2: //DWORD err = GetLastError();
			//NOTE: in2 needs to set in to EOF but also
			//this is in keeping with returning EOF
			SetFilePointer(SOM_MAP.map.in,0,0,FILE_END); //EOF?
			//SetLastError(err);
			*rd = 0; SetLastError(ERROR_HANDLE_EOF);
			return FALSE;
		}		
		/*may as well get out a head of this
		else if(sz==1) //HACK: prevent peeking (split CR/LF)
		{
			if(SOM_MAP.map.in2!=SOM_MAP.map.in) //layers system
			{
				//first is just to be safe
				SetFilePointer(SOM_MAP.map.in,1,0,FILE_CURRENT);
				SetFilePointer(SOM_MAP.map.in2,-1,0,FILE_CURRENT);
			}
		}*/
		else if('\r'==((char*)to)[*rd-1]) //assuming nonzero
		{
			--*rd; SetFilePointer(SOM_MAP.map.in2,-1,0,FILE_CURRENT);
		}
		assert(sz>1); return TRUE; //peeking?
	}
	else if(in==SOM_MAP.ezt)
	{
		return SOM_MAP.ezt.read(to,sz,rd);
	}
	else for(size_t i=0;i<som_tool_reading;i++) 
	{
		if(in==som_tool_reading_list[i])
		{		
			DWORD at = SetFilePointer(in,0,0,FILE_CURRENT);

			if(!SOM::Tool.ReadFile(in,to,sz,rd,o)) 
			{
				som_tool_reading_x(i); 
				
				return FALSE;
			}
					
			if(!som_tool_reading_f[i](i,at,*rd,(BYTE*)to))
			{		
				som_tool_reading_x(i);	
			}

			return TRUE;		
		}
	}

	return SOM::Tool.ReadFile(in,to,sz,rd,o);
}

static BOOL WINAPI som_tool_CloseHandle(HANDLE in)
{	
	SOM_TOOL_DETOUR_THREADMAIN(CloseHandle,in)

	EXLOG_LEVEL(7) << "som_tool_CloseHandle()\n";
	
	if(!in) return SOM::Tool.CloseHandle(0); //loopy

	for(size_t i=0;i<som_tool_reading;i++) 
	{
		if(in==som_tool_reading_list[i]) 
		{
			som_tool_reading_x(i); break;
		}
	}

	//IMPORTANT: MUST COME BEFORE CloseHandle!
	if(in==SOM_MAP.ezt) SOM_MAP.ezt = 0; 

	BOOL out = SOM::Tool.CloseHandle(in); //!!

	if(in==SOM_MAP.map) //REMOVE ME?
	{	
		//assert(!som_tool_stack[1]);
		//Should the Map menu even be here?
		//Should the Map menu even be here?
		//Should the Map menu even be here?
		//2019: Map menu is corrupting the new layer system.
		if(!som_tool_stack[1])
		{
			extern void SOM_MAP_map2_etc_epilog();
			SOM_MAP_map2_etc_epilog();
		}

		SOM_MAP_map &map = SOM_MAP.map;
		bool follow_up = map.version<13
		&&!map.legacy&&map.finished&&map.rw!=map.read_only;		
		if(in!=map.in2)
		{
			assert(!som_tool_stack[1]);
			SOM::Tool.CloseHandle(map.in2); //layers system
		}
		SOM_MAP.map = 0;
		if(follow_up) //triggered for version 12 files with 13 parts
		{
			//REMINDER: I guess the main use of this is to provide a 
			//copy of the version 12 file in the form of a .MA~ file
			char xx[32] = "";
			sprintf_s(xx,SOMEX_(B)"\\data\\map\\%02d.ma~",map.current);
			const wchar_t *src = SOM::Tool::project(xx), *tilde;
			if(src) if(tilde=wcsrchr(src,'~')) //!!
			{	
				wchar_t dst[MAX_PATH] = L""; 			
				wmemcpy(dst,src,tilde-src+2)[tilde-src] = 'p';
				return MoveFileEx(src,dst,MOVEFILE_REPLACE_EXISTING);
			}
		}
	}
	
	if(in==som_sys_dat) som_sys_dat = 0; return out;
}

static BOOL WINAPI som_tool_SetCurrentDirectoryA(LPCSTR lpPathName)
{
	SOM_TOOL_DETOUR_THREADMAIN(SetCurrentDirectoryA,lpPathName)

	EXLOG_LEVEL(7) << "som_tool_SetCurrentDirectoryA()\n"; 

	EXLOG_LEVEL(3) << "Changing to "<< lpPathName << "\n";

	const wchar_t *w;
	if(SOM::tool<PrtsEdit.exe)
	{
		assert(0); //2018: MONITORING?
		w = SOM::Tool::project(lpPathName);
	}
	else //TODO: do some other way (workshop_directory?)
	{
		char *a = 0; switch(SOM::tool) 
		{
		case PrtsEdit.exe: a = SOMEX_(A)"\\data\\map\\parts"; break;
		case ItemEdit.exe: a = SOMEX_(A)"\\data\\item\\prof"; break;
		case ObjEdit.exe: a = SOMEX_(A)"\\data\\obj\\prof"; break;
		case EneEdit.exe: a = SOMEX_(A)"\\data\\enemy\\prof"; break;
		case NpcEdit.exe: a = SOMEX_(A)"\\data\\npc\\prof"; break;
		case SfxEdit.exe: a = SOMEX_(A)"\\data\\my\\prof"; break;
		}
		if(a) w = SOM::Tool::install(a);
	}

	if(w) return SetCurrentDirectoryW(w);
		
	return SOM::Tool.SetCurrentDirectoryA(lpPathName); 
}
//MONITORING (so just SOM_RUN)
static BOOL WINAPI som_tool_SetEnvironmentVariableA(LPCSTR A, LPCSTR B)
{
	SOM_TOOL_DETOUR_THREADMAIN(SetEnvironmentVariableA,A,B)

	EXLOG_LEVEL(7) << "som_tool_SetEnvironmentVariableA()\n"; 

	assert(0); //2018: MONITORING?

	if(SOM::tool==SOM_RUN.exe) 
	{
		//observed setting PATH to tool folder
		//(as part of the disc-lock mechanism)	
		static int count = 0; switch(count++)
		{
		case 1: som_run_locked = false; case 0: return TRUE; 
		}
	}
	assert(0);
	return SOM::Tool.SetEnvironmentVariableA(A,B);
}
static HANDLE WINAPI som_tool_CreateMutexA(LPSECURITY_ATTRIBUTES A, BOOL B, LPCSTR C)
{
	SOM_TOOL_DETOUR_THREADMAIN(CreateMutexA,A,B,C)

	EXLOG_LEVEL(7) << "som_tool_CreateMutexA()\n"; 
	
	//each tool makes its own mutex
	//SOM_MAIN etc, all caps, except SOM_MapEditor		   

	if(!C||strncmp(C,"SOM_",4)) return SOM::Tool.CreateMutexA(A,B,C);
	
	const wchar_t *e = EX::exe();
	assert(C[4]==e[4]&&toupper(C[5])==e[5]&&toupper(C[6])==e[6]);

	//som_tool_classname is "".
	if(SOM::tool==SOM_MAIN.exe) return 0; //No reason to bar this.
	assert(*som_tool_classname);
	return CreateMutexW(A,B,som_tool_classname);	
}
static HANDLE WINAPI som_tool_OpenMutexA(DWORD A, BOOL B, LPCSTR C)
{
	SOM_TOOL_DETOUR_THREADMAIN(OpenMutexA,A,B,C)

	EXLOG_LEVEL(7) << "som_tool_OpenMutexA()\n"; 
	
	//each of the big three tools looks for the other two
	//SOM_EDIT (and likely MAIN too) looks out for itself 

	if(!C||strncmp(C,"SOM_",4)) return SOM::Tool.OpenMutexA(A,B,C);
	 
	if(SOM::tool==SOM_MAIN.exe) return 0; //no reason to bar this

	const wchar_t *peers = EX::exe();
	for(int i=0;*peers&&*peers==C[i];i++,peers++);

	if(*peers=='A' //special case
	&&!strcmp(C,"SOM_MapEditor")&&!wcscmp(EX::exe(),L"SOM_MAP"))
	peers+=2; //same
	
	if(*peers) return 0; //i.e. not the current tool

	//2017: Changing the dialog class name to try to use FindWindow below.
	if(!*som_tool_classname)
	{
		int i,n=swprintf_s(som_tool_classname,L"%hs (%ls)",C,SOM::Tool::project());
		for(int i=0;i<n;i++)
		if(som_tool_classname[i]=='\\') som_tool_classname[i] = '/'; //forbidden character
	}
	else assert(0);

	HANDLE out = OpenMutexW(A,B,som_tool_classname);

	if(out) //the tool will refuse to open iteself
	{
		HWND test = FindWindow(som_tool_classname,0); if(test) 
		{
			//SOM::splashed(0,0); SetForegroundWindow(test); //YUCK
			HWND splash = SOM::splash(); if(splash)
			{
				SendMessage(splash,WM_CLOSE,(WPARAM)test,0);
			}
			else assert(0);
		}
		else assert(0);
	}

	return out;
}
//MONITORING
static BOOL WINAPI som_tool_DeleteFileA(LPCSTR A)
{
	SOM_TOOL_DETOUR_THREADMAIN(DeleteFileA,A)

	EXLOG_LEVEL(7) << "som_tool_DeleteFileA()\n"; 

	const wchar_t *w = SOM::Tool::project(A);

	//SOM_MAP's map menu has a Delete function.
	if(w) if(41000==GetWindowContextHelpId(som_tool_stack[1]))
	{
		if(DeleteFileW(w)) return 1; assert(0); return 0;
	}
	else assert(0);

	BOOL out = SOM::Tool.DeleteFileA(A);

	if(!out)
	{
		assert(0);
		if(EX::debug) //2023
		SOM::Tool.MessageBoxA(0,A,"Debugging: inside DeleteFileA",MB_OK);
	}	
	return out;
}
static DWORD WINAPI som_tool_GetFullPathNameA(LPCSTR A,DWORD B,LPSTR C,LPSTR *D)
{
	SOM_TOOL_DETOUR_THREADMAIN(GetFullPathNameA,A,B,C,D)

	EXLOG_LEVEL(7) << "som_tool_GetFullPathNameA()\n"; 

	DWORD out = SOM::Tool.GetFullPathNameA(A,B,C,D);

	if(!out)
	{
		assert(0);
		if(EX::debug) //2023
		SOM::Tool.MessageBoxA(0,A,"Debugging: inside GetFullPathNameA",MB_OK);
	}
	return out;
}
static DWORD WINAPI som_tool_GetFileAttributesA(LPCSTR A)
{
	SOM_TOOL_DETOUR_THREADMAIN(GetFileAttributesA,A)

	EXLOG_LEVEL(7) << "som_tool_GetFileAttributesA()\n"; 
	EXLOG_LEVEL(7) << ' ' << A << '\n'; 

	int compile[sizeof(SOMEX_(A))==5];	
	if(*(DWORD*)SOMEX_(A)==*(DWORD*)A
	 ||*(DWORD*)SOMEX_(B)==*(DWORD*)A
	 ||*(DWORD*)SOMEX_(C)==*(DWORD*)A
	 ||*(DWORD*)SOMEX_(D)==*(DWORD*)A)
	{
		return FILE_ATTRIBUTE_NORMAL; //assuming not a directory
	}

	DWORD out = SOM::Tool.GetFileAttributesA(A);
	if(out==INVALID_FILE_ATTRIBUTES)
	{
		if(EX::INI::Output()->missing_whitelist(A)) return out;

		if(EX::debug) //2023
		if(SOM::tool!=SOM_RUN.exe) //\\.\Secdrv ???
		SOM::Tool.MessageBoxA(0,A,"Debugging: inside GetFileAttributesA",MB_OK);
	}
	return out;
}
static BOOL WINAPI som_tool_GetDiskFreeSpaceA(LPCSTR A,LPDWORD B,LPDWORD C,LPDWORD D,LPDWORD E)
{
	SOM_TOOL_DETOUR_THREADMAIN(GetDiskFreeSpaceA,A,B,C,D,E)

	EXLOG_LEVEL(7) << "som_tool_GetDiskFreeSpaceA()\n"; 

	//New Project
	assert(SOM::tool==SOM_MAIN.exe);
	//SOM_MAIN returns 0 here (no frame pointer?)
	//but to be safe return_address now defaults to 401000
	DWORD ra = (DWORD)EX::return_address();	
	if(ra>=0x401000&&ra<SOM::image_rdata) 
	if(A[0]&&A[1]==':'&&A[2]=='\\'&&!A[3])
	{
		const wchar_t *vol = 0;
		if(*(WORD*)SOMEX_(B)==*(WORD*)A)
		{				   
			//REMINDER: getting here requires
			//reprogramming 0x4015D1 to be jmp vs je
			//OTHERWISE A COM LIKE INTERFACE GETS USED AND
			//IT SOMETIMES WOULD WORK ON NONEXISTENT VOLUMES???
			vol = SOM::Tool::project(); 
		}		
		if(vol) //MAX_PATH? supporting network paths?
		{	
			int i = 0; wchar_t out[MAX_PATH+1] = L"";
			for(i=0;vol[i]&&(vol[i]!='\\'&&vol[i]!='/'||i<2);i++) out[i] = vol[i];
			out[i++] = '\\'; out[i] = '\0';
			if(GetDiskFreeSpaceW(out,B,C,D,E)) return TRUE;
			assert(0);
		}
		else assert(!vol&&!EX::debug); //Release assert?
	}

	BOOL out = SOM::Tool.GetDiskFreeSpaceA(A,B,C,D,E);

	if(!out)
	{
		assert(0);
		if(EX::debug) //2023
		SOM::Tool.MessageBoxA(0,A,"Debugging: inside GetDiskFreeSpaceA",MB_OK);
	}
	return out;
}
static BOOL WINAPI som_tool_GetVolumeInformationA(LPCSTR A,LPSTR B,DWORD C,LPDWORD D,LPDWORD E,LPDWORD F,LPSTR G,DWORD H)
{		
	SOM_TOOL_DETOUR_THREADMAIN(GetVolumeInformationA,A,B,C,D,E,F,G,H)

	EXLOG_LEVEL(7) << "som_tool_GetVolumeInformationA()\n"; 
	
	DWORD ra = (DWORD)EX::return_address();
	if(ra>=0x401000&&ra<SOM::image_rdata) 
	if(A[0]&&A[1]==':'&&A[2]=='\\'&&!A[3])
	{
		const wchar_t *vol = 0;
		if(*A&&!strcmp(A+1,":\\"))
		if(*(WORD*)SOMEX_(A)==*(WORD*)A)
		{
			vol = SOM::Tool::install(); 
		}
		else if(*(WORD*)SOMEX_(B)==*(WORD*)A)
		{				   
			vol = SOM::Tool::project(); 
		}
		else if(*(WORD*)SOMEX_(C)==*(WORD*)A)
		{
			//2021: if a path is passed to the MFC routines
			//that doesn't have a drive volume it's likely
			//to end up wherever the current directory is
			if(SOM::tool==SOM_RUN.exe)
			{
				vol = SOM::Tool::runtime();	
			}
			else assert(0);
		}	
		else if(*(WORD*)SOMEX_(D)==*(WORD*)A)
		{
			if(SOM::tool==SOM_EDIT.exe) //2021
			{
				vol = som_edit_project2;	
			}
			else assert(0);
		}	
		if(vol&&!B&&!G) //MAX_PATH? supporting network paths?
		{		
			int i = 0; wchar_t out[MAX_PATH+1] = L"";
			for(i=0;vol[i]&&(vol[i]!='\\'&&vol[i]!='/'||i<2);i++) out[i] = vol[i];
			out[i++] = '\\'; out[i] = '\0';
			if(GetVolumeInformationW(out,0,0,D,E,F,0,0)) return TRUE;
			assert(0);
		}
		else assert(!vol&&!EX::debug); //Release assert?
	}

	BOOL out = SOM::Tool.GetVolumeInformationA(A,B,C,D,E,F,0,0);

	if(!out)
	{
		assert(0);
		if(EX::debug) //2023
		SOM::Tool.MessageBoxA(0,A,"Debugging: inside GetVolumeInformationA",MB_OK);
	}
	return out;
}			   

static BOOL WINAPI som_tool_CreateDirectoryA(LPCSTR A, LPSECURITY_ATTRIBUTES B)
{
	SOM_TOOL_DETOUR_THREADMAIN(CreateDirectoryA,A,B)

	EXLOG_LEVEL(7) << "som_tool_CreateDirectoryA()\n";

	EXLOG_LEVEL(3) << "Creating "<< A << '\n';
	
	BOOL out = 0; const wchar_t *Aw = 0;

	const size_t N = sizeof(SOMEX_(A));

	if(strnlen(A,N)<N) //top
	{
		if(SOM::tool==SOM_MAIN.exe)
		if(PathIsPrefixA(A,SOMEX_(B))) Aw = SOM::Tool::project(); 
		if(SOM::tool==SOM_RUN.exe)
		if(PathIsPrefixA(A,SOMEX_(C))) Aw = SOM::Tool::runtime(); 
		if(Aw)
		{
			EXLOG_LEVEL(3) << "Widened to "<< Aw << '\n';

			//2021: found out today SHCreateDirectory
			//refuses to parse '/' (returns hresult 3)
			for(int i=0;Aw[i];i++) if(Aw[i]=='/')
			{
				const_cast<wchar_t&>(Aw[i]) = '\\'; //FIX ME
			}

			int err = SHCreateDirectoryExW(0,Aw,0);

			out = !err; SetLastError(err);			
		}
	} 
	if(!Aw) //internal folders
	{
		if(SOM::tool==SOM_MAIN.exe) Aw = SOM::Tool::project(A);	
		if(SOM::tool==SOM_RUN.exe) Aw = SOM::Tool::runtime(A);
		
		if(Aw) 
		{
			EXLOG_LEVEL(3) << "Widened to "<< Aw << '\n';
			
			out = CreateDirectoryW(Aw,B);	
		}
		else out = SOM::Tool.CreateDirectoryA(A,B);
	}

	if(Aw) //require that everything be in order
	if(!out&&GetLastError()==ERROR_ALREADY_EXISTS)
	{			
		static int overwrite = 0;

		if(overwrite) return overwrite>0; 

		EX::INI::Editor ed;

		if(SOM::tool==SOM_MAIN.exe&&ed&&ed->do_overwrite_project
		  ||SOM::tool==SOM_RUN.exe&&ed&&ed->do_overwrite_runtime) 
		{
			//we want the dialog to appear only for the target folder
			if(SOM::tool==SOM_MAIN.exe&&wcscmp(Aw,SOM::Tool::project())) return out;
			if(SOM::tool==SOM_RUN.exe&&wcscmp(Aw,SOM::Tool::runtime())) return out;			
			
			overwrite = EX::messagebox(MB_YESNOCANCEL,
			"You are about to overwrite the contents of %ls.\n\n"
			"Would you like to hurry things along by copying only files which are newer than the files being overwritten?\n\n"
			"Choose NO to overwrite all files. Choose CANCEL to defer to Sword of Moonlight.",
			Aw);
							   
			som_tool_overwrite_clock = overwrite==IDYES;

			if(overwrite==IDYES||overwrite==IDNO) return TRUE;

			overwrite = 0; //cancelled so reset message box

			SetLastError(ERROR_ALREADY_EXISTS);
		}
		else overwrite = -1;
	}

	return out;
}

static HIMAGELIST WINAPI som_map_ImageList_LoadImageA(HINSTANCE hi, LPCSTR in, int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags)
{
	SOM_TOOL_DETOUR_THREADMAIN(ImageList_LoadImageA,hi,in,cx,cGrow,crMask,uType,uFlags)

	if(uintptr_t(in)<=0xFFFF) //Eg. MAKEINTRESOURCE
	{
		if(in==MAKEINTRESOURCEA(198)) 
		/*2018: Noticed LR_CREATEDIBSECTION seems to do the same 	
		if(1) //treeview icons
		{	
			HIMAGELIST out = 0; //SOM_MAP ImageList_Destroy's it

			//http://www.codeproject.com/Tips/152904/Load-a-color-bitmap-properly-into-an-imagelist

			HBITMAP bm = LoadBitmapW(hi,MAKEINTRESOURCEW(198));

			BITMAP sz; GetObject(bm,sizeof(sz),&sz);

			out = ImageList_Create(sz.bmWidth/9,sz.bmHeight,ILC_COLOR32,0,0);

			ImageList_Add(out,bm,0); DeleteObject(bm);	

			return out;
		}
		else*/ uFlags|=LR_CREATEDIBSECTION;
					
	a:	return SOM::Tool.ImageList_LoadImageA(hi,in,cx,cGrow,crMask,uType,uFlags);
	}

	EXLOG_LEVEL(3) << "Loading "<< in << '\n';

	const wchar_t *w = SOM::Tool::file(in);
	if(!w) goto a;

	EXLOG_LEVEL(3) << "Widened to "<< w << '\n';

	return ImageList_LoadImageW(hi,w,cx,cGrow,crMask,uType,uFlags);
}

/*using SOM_MAIN_reprogram to do this for SOM_MANU 
static HBITMAP WINAPI som_main_LoadBitmapA(HINSTANCE hi, LPCSTR in)
{			 	
	SOM_TOOL_DETOUR_THREADMAIN(LoadBitmapA,hi,in)

	if(uintptr_t(in)<=0xFFFF) //e.g. MAKEINTRESOURCE
	{
		if(in==(char*)144&&SOM::tool==SOM_MAIN.exe) //splash?
		{			
			//Clearly, this must be to suppress the splash screen
			//for SOM_EDIT. It must think that SOM_EX is SOM_EDIT
			return 0;
		}
	}
	else assert(0);

	return SOM::Tool.LoadBitmapA(hi,in);
}*/

extern HANDLE WINAPI som_map_LoadImageA(HINSTANCE hi, LPCSTR in, UINT uType, int cx, int cy, UINT uFlags)
{
	SOM_TOOL_DETOUR_THREADMAIN(LoadImageA,hi,in,uType,cx,cy,uFlags)
			
	EXLOG_LEVEL(7) << "som_map_LoadImageA()\n";

	if(!(uFlags&LR_LOADFROMFILE)||uintptr_t(in)<=0xFFFF) //MAKEINTRESOURCE
	{
		return SOM::Tool.LoadImageA(hi,in,uType,cx,cy,uFlags);
	}

	EXLOG_LEVEL(3) << "Loading "<< in << '\n';

	const wchar_t *w;
	wchar_t vname[MAX_PATH];
	extern wchar_t *som_art_CreateFile_w; //2021
	extern wchar_t *som_art_CreateFile_w_cat(wchar_t[],const char*);
	if(som_art_CreateFile_w)
	{
		//NOTE: I added this for EneEdit skins but I think art should
		//go down the TXR path (som_TXR_4485e0)
		w = som_art_CreateFile_w_cat(vname,in); assert(0); //REMOVE ME?		
	}
	else
	{
		w = SOM::Tool::file(in);

		//NEW: virtual data directory		
		if(w&&!wcsnicmp(w,L"DATA\\",5)) 
		{
			//2018: assuming not returning this
			//static wchar_t v[MAX_PATH] = L"";
			if(EX::data(w+5,vname)) w = vname;
		}
	}

	if(!w||som_tool_file_appears_to_be_missing(in,w,0))
	{
		return SOM::Tool.LoadImageA(hi,in,uType,cx,cy,uFlags);
	}

	EXLOG_LEVEL(3) << "Widened to "<< w << '\n';

	//GIMP trouble https://gitlab.gnome.org/GNOME/gimp/issues/2010
	HANDLE ret = LoadImageW(hi,w,uType,cx,cy,uFlags);
	return ret; //breakpoint
}

static BOOL WINAPI som_prm_BitBlt(HDC dst, int xdst, int ydst, int wdst, int hdst, HDC src, int xsrc, int ysrc, DWORD rop)
{
	SOM_TOOL_DETOUR_THREADMAIN(BitBlt,dst,xdst,ydst,wdst,hdst,src,xsrc,ysrc,rop)
			
	EXLOG_LEVEL(7) << "som_prm_BitBlt()\n";
	
	//NOT USING //TESTING API
	//REMINDER: this API sees HEAVY TRAFFIC (from common-controls)
		assert(0);

		DWORD ra = (DWORD)EX::return_address();
		
		if(ra>=0x401000&&ra<SOM::image_rdata) 
		{
			int bp = 1;
		}

	return SOM::Tool.BitBlt(dst,xdst,ydst,wdst,hdst,src,xsrc,ysrc,rop);
}

static BOOL WINAPI som_tool_StretchBlt(HDC dst, int xdst, int ydst, int wdst, int hdst, HDC src, int xsrc, int ysrc, int wsrc, int hsrc, DWORD rop)
{
	SOM_TOOL_DETOUR_THREADMAIN(StretchBlt,dst,xdst,ydst,wdst,hdst,src,xsrc,ysrc,wsrc,hsrc,rop)
			
	EXLOG_LEVEL(7) << "som_tool_StretchBlt()\n";

	EXLOG_LEVEL(7) << "dst: " << xdst << 'x' << ydst << 'y' << wdst << 'w' << hdst << 'h' << '\n';
	EXLOG_LEVEL(7) << "src: " << xsrc << 'x' << ysrc << 'y' << wsrc << 'w' << hsrc << 'h' << '\n';
	
	if(!dst||dst!=SOM::HACKS::stretchblt) //REMOVE ME?
	{
		//true for SOM, either it breaks up the bitmaps or creates
		//device contexts, or adjusts the origin of the src context???
		//assert(xsrc==0); 

		if(!som_tool_classic)
		{
			SetStretchBltMode(dst,HALFTONE); SetBrushOrgEx(dst,0,0,0);
		}
		return SOM::Tool.StretchBlt(dst,xdst,ydst,wdst,hdst,src,xsrc,ysrc,wsrc,hsrc,rop);
	}

	int sx = EX::INI::Editor()->texture_subsamples;

	if(sx!=1) //HALFTONE is not supported by Windows 95~Me
	{
		SetStretchBltMode(dst,HALFTONE); SetBrushOrgEx(dst,0,0,0);
	}

	xdst = 0; ydst = 0; wdst = 256*sx; hdst = 256*sx;

	/*if(0) //testing colorkey support
	{
		for(int i=0;i<wsrc;i++) for(int j=0;j<hsrc;j++)
		{
			if(i/16%2==j/16%2) SetPixel(src,j,i,0x070707);
		}
	}*/
	return SOM::Tool.StretchBlt(dst,xdst,ydst,wdst,hdst,src,xsrc,ysrc,wsrc,hsrc,rop);
}

static bool som_tool_prt(int z, DWORD at, DWORD sz, BYTE *tile)
{		
	static HWND bar = GetDlgItem(som_tool,1053);

	if(!at&&bar) //SOM_PRM like progress bar
	{
		//todo: incorporate database and icons
		static int progress = 0; if(!progress++)
		{
			wchar_t count[MAX_PATH] = L"map/*";
			int range = Sompaste->database(0,count,0,0);
			SendMessage(bar,PBM_SETRANGE,0,MAKELPARAM(0,range));
		}  		
		SendMessage(bar,PBM_SETPOS,progress,0);
	}

	if(at>0||sz<130) //paranoia
	{
		assert(0); return false;
	}
	
	//som_tool_xdata_t xtile; //REMOVE ME?
	auto &xtile = som_tool_xdata_2023();
	static wchar_t x[MAX_PATH] = L"data\\map\\parts\\"; //15
	if(som_tool_xdata(wcscpy(x+15,som_tool_reading_mode[z])-15,xtile))
	((char*)memcpy(tile+100,xtile+100,30))[30] = '\0';

	//REMOVE ME? 
	//(collecting upon opening)
	const SOM_MAP_prt::key &k = SOM_MAP.prt[som_tool_reading_mode[z]];	
	assert(*k.description||SOM::tool!=MapComp.exe); //index2

	//DUPLICATE (som_map_syncup_prt)	
	if(!tile[32])
	if(char*e=strrchr((char*)memcpy(tile+32,tile,31),'.'))
	{
		if(!stricmp(e+1,"msm")) e[2] = islower(e[2])?'h':'H';
	}
	if(!*k.icon)
	EX::need_unicode_equivalent(65001,(char*)tile+68,(wchar_t*)k.icon,31);
	if(!*k.description)
	EX::need_unicode_equivalent(932,(char*)tile+100,(wchar_t*)k.description,31);

	FILETIME m; //2022
	GetFileTime(som_tool_reading_list[z],0,0,&m);
	k.writetime = m;
	
	if(0x100&som_map_tileviewmask) //MHM->MSM?
	{
		memcpy(tile,tile+32,32); assert(SOM::tool==MapComp.exe);
	}

	return false; //that's all folks
}

extern DWORD som_tool_prf_size = 0;
static bool som_tool_prf(int z, DWORD at, DWORD sz, BYTE *name)
{
	assert(SOM::tool==SOM_PRM.exe&&som_prm_tab);

	const wchar_t *w = som_tool_reading_mode[z];

	if(at==0&&sz>=31) //expected behavior
	{	
		som_prm scheduled_obsolete; 
		//pad: SOM_PRM requires an exact match to link PRM->PR2
		if(w) EX::need_ansi_equivalent(65001,w,(char*)name,31); 
		if(w) scheduled_obsolete.pr2->pad((char*)name);
	}
	else assert(0);

	if(!som_tool_initializing)
	if(som_prm_tab==1005||som_prm_tab==1006)
	{
		som_tool_prf_size = //SOM_PRM.cpp
		GetFileSize(som_tool_reading_list[z],0);
	}
	/*2021: is the tab unreliable? (an NPC can be 564)
	//564: indicates enemy/prof PRF file (OR DOES IT?)
	if(GetFileSize(som_tool_reading_list[z],0)>564)*/
	if(som_tool_tab==1005)
	{
		assert(at<=146&&at+sz>=146+126);

		//ATTACK DESCRIPTIONS
		//som_tool_xdata_t xprof; //REMOVE ME?
		auto &xprof = som_tool_xdata_2023();
		wchar_t x[MAX_PATH] = L"data\\enemy\\prof\\"; //16
		//REMINDER: names on the back are left blank
		//126->1: the final entries can be cut off :(
		if(som_tool_xdata(wcscpy(x+16,w)-16,xprof)>=146+6*21) 				
		if(at==0) memcpy(name+146/*-at*/,xprof+146,6*21);
		else assert(at==0); //2018
	}

	//2021: the PRF file is read any time changes are committed
	//the profile is cross-referenced by its textual name field
	if(som_prm_tab==1001)
	{
		//som_prm_profile handles separators
		BYTE i = name[62];
		/*2021: mistake? 85 is pitch extension
		bool move = 0==(WORD&)name[84];*/
		bool move = 0==name[84];
		if(i) if(move) move: //note: assuming not my/arm
		{
			switch(i)
			{
			case 1: //assuming that a weapon attacks
					i+=1<<2; break;			
			case 3: goto bow_and_arrow;			
			default: assert(i<=3);
			}
		}
		else if(i==1||i==2) moveset:
		{
			//8485 is thought to be 0 except
			//with a new style equipment PRF
			//these 4Bs are pitches that are
			//not to be set to 0
			for(int b=2;b<6;b++) if(name[82+b])
			{
				//there should be a swing ID
				//association with this slot
				i|=1<<b;
			}			

			i|=1<<6; //2021
		}
		else if(i==3) bow_and_arrow:
		{
			i|=1<<7; //2021

			//not implemented
			BYTE pos = name[72];
			name[62] = 5==pos?2:1; //shield?
			if(move) goto move; goto moveset;
		}
		else assert(0);

		if(som_tool_initializing)
		{
			SOM_PRM_items.back() = i;
			SOM_PRM_items.push_back(0); //HACK: selection
		}
		else //2021
		{
			int prf = SendDlgItemMessage
			(som_tool_stack[1],1002,CB_GETCURSEL,0,0);
			if(prf!=CB_ERR)
			{
				int cmp = SOM_PRM_items[prf];
				SOM_PRM_items[prf] = i;
				if((cmp|i)&1<<6) SOM_PRM_arm.clear(); //save?
			}
			else assert(0);
		}
	}

	return false; //that's all folks
}

static bool som_tool_prm(int z, DWORD at, DWORD sz, BYTE *buffer)
{			
	size_t record_s = 0;

	assert(SOM::tool==SOM_MAP.exe);
	//make believe all slots are used
	switch(*som_tool_reading_mode[z])
	{
	case 'i': record_s = 336; break; //item.prm
	case 'e': record_s = 488; break; //enemy.prm
	case 'n': record_s = 320; break; //npc.prm
	case 'o': record_s = 56;  break; //obj.prm
	}

	if(!record_s) return false;

	size_t i = at%record_s;
	if(i) i = record_s-i; //next profile word
	if(record_s==56) i+=36; //obj.prm adjustment
	while(i+2<sz)
	{	
		WORD &profile = (WORD&)*(buffer+i);

		if(profile==0xFFFF) profile = 0xFFFE; i+=record_s; 
	}
	assert(i>=sz);

	return true; //keep'em comin'
}

static bool som_map_reset(bool &wrote)
{
	bool out = wrote; wrote = false; return out;
}
extern void som_map_syncup_prt(int knum, wchar_t path[MAX_PATH]) //2022
{
	auto &k = SOM_MAP.prt.blob[knum];
	auto *p = SOM_MAP_4921ac.find_part_number(k.part_number());	
	FILE *f = p?_wfopen(path,L"rb"):0; assert(f);
	if(!f) return;
	BYTE tile[228] = {}; fread(tile,1,sizeof(tile),f); fclose(f);

	//DUPLICATE (som_tool_prt)	
	if(!tile[32])
	if(char*e=strrchr((char*)memcpy(tile+32,tile,31),'.'))
	{
		if(!stricmp(e+1,"msm")) e[2] = islower(e[2])?'h':'H';
	}
	EX::need_unicode_equivalent(65001,(char*)tile+68,(wchar_t*)k.icon,31);
	EX::need_unicode_equivalent(932,(char*)tile+100,(wchar_t*)k.description,31);

	//HACK: SOM_MAP_165 swaps these in order to
	//display MHM models in place of MSM models	
	auto *msm = p->msm.file, *mhm = p->mhm.file;
	if(0x100&som_map_tileviewmask)
	std::swap(msm,mhm);
	memcpy(msm,tile,31);
	memcpy(mhm,tile+32,31);	
	memcpy(p->description,tile+100,127);

	//TODO: still need to update icons (this is experimental)
	int todolist[SOMEX_VNUMBER<=0x102040cUL];
	//40f8dc has code for inserting an icon... it would take 
	//a bit of work to implement it (it's probably worthwhile
	//to reprogram 40f8dc because it's potentially very slow)
	//(I may be rewriting it soon)
	memcpy(p->icon_bmp,tile+68,31);
}
static void som_map_syncup_prm_and_pr2_files()
{	
	HWND selcb = 0;
	HTREEITEM sel = 0; 
	if(som_map_tileviewinsert)
	{
		HWND tv; sel = 
		TreeView_GetSelection(tv=GetDlgItem(som_map_tileview,1052));
		for(HTREEITEM swap;swap=TreeView_GetParent(tv,sel);)
		sel = swap;
	}

	//TODO: som_map_tileviewpaint();
	BYTE record[488]; DWORD recorded;
	#define IF_WROTE(X,x,record_s,tv,...) \
	if(som_map_reset(SOM::PARAM::X.prm->wrote)||som_map_reset(SOM::PARAM::X.pr2->wrote))\
	{\
		/*REMINDER: som_tool_read enables som_tool_prm/pr2*/\
		som_tool_read = false; /*NEW*/\
		if(sel==som_map_treeview[tv]&&sel)\
		selcb = GetDlgItem(som_map_tileviewinsert,1021);\
		HANDLE prm = CreateFileA(SOMEX_(B)"\\PARAM\\"#X".prm",SOM_TOOL_READ);\
		HANDLE pr2 = CreateFileA(SOMEX_(B)"\\PARAM\\"#X".pr2",SOM_TOOL_READ);\
		som_tool_read = true; /*NEW*/\
		DWORD pr2_s = 0; if(ReadFile(pr2,&pr2_s,4,&recorded,0))\
		if(prm!=INVALID_HANDLE_VALUE&&pr2!=INVALID_HANDLE_VALUE)\
		for(int i=0;i<EX_ARRAYSIZEOF(SOM_MAP_4921ac.x##props);i++)\
		if(ReadFile(prm,record,record_s,&recorded,0)||recorded==record_s)\
		{\
			SOM_MAP_4921ac::X##Prop &prop = SOM_MAP_4921ac.x##props[i];\
			__VA_ARGS__\
			SOM_MAP_4921ac::X##Prof &prof = SOM_MAP_4921ac.x##profs[prop.profile];\
			ReadFile(pr2,prof.model,32,&recorded,0);\
			prof.model[30] = '\0';\
		}\
		CloseHandle(prm); CloseHandle(pr2);\
	}			
	IF_WROTE(Item,item,336,3,
	{
		prop.profile = *(WORD*)record;
		//2018: som_tool_read disables som_tool_prm
		if(prop.profile==0xFFFF) prop.profile = 0xFFFE; 
		memcpy(prop.name,record+2,31);		 		
		if(prop.profile>=250||prop.profile>=pr2_s
		||SetFilePointer(pr2,4+prop.profile*88,0,FILE_BEGIN)
		==INVALID_SET_FILE_POINTER) continue;

		SOM_MAP_4921ac::ItemProf &prof = 
		SOM_MAP_4921ac.itemprofs[prop.profile];
		ReadFile(pr2,record,88,&recorded,0); 
		memcpy(prof.model,record+31,31);
		prof.model[30] = '\0';		
		//paranoia: SOM_MAP does this
		//see axes' declaration notes
		prof.axes = record[31]; 
		prof.pivot = (FLOAT&)record[64];
		prof.v = (WORD&)record[70];
		//paranoia: SOM_MAP does this
		//this byte remains a mystery
		prof.byte76 = record[76];
		continue; //important
	})
	/*IF_WROTE(Magic,magic,?
	{
		UNFINISHED (magicprofs won't fit into the macro)

	})*/
		
	SOM_MAP_4921ac::Skin skin;
	skin.file[30] = '\0';
	auto &skins = SOM_MAP_4921ac.npcskins;

	IF_WROTE(Enemy,enemy,488,0,
	{
		prop.profile = *(WORD*)record;
		//2018: som_tool_read disables som_tool_prm
		if(prop.profile==0xFFFF) prop.profile = 0xFFFE; 
		prop.size = (FLOAT&)record[4];
		memcpy(prop.name,record+8,31);
		if(prop.profile>=1024
		||SetFilePointer(pr2,4*prop.profile,0,FILE_BEGIN)
		==INVALID_SET_FILE_POINTER) continue;

		DWORD fp; 
		if(ReadFile(pr2,&fp,4,&recorded,0)&&recorded==4)
		if(!fp||SetFilePointer(pr2,fp+64,0,FILE_BEGIN)
		==INVALID_SET_FILE_POINTER) continue;
		
		if(ReadFile(pr2,&skin,31,&recorded,0)&&recorded==31)
		if(skin.skin&&skin.file[0])
		{
			if(!skins)
			memset(skins=new SOM_MAP_4921ac::Skin[2048],0x00,32*2048);
			skins[1024+i] = skin;
		}
		else if(skins) skins[1024+i].skin = 0;

		SetFilePointer(pr2,31+fp,0,FILE_BEGIN);
	})
	IF_WROTE(NPC,NPC,320,1,
	{
		prop.profile = *(WORD*)record;
		//2018: som_tool_read disables som_tool_prm
		if(prop.profile==0xFFFF) prop.profile = 0xFFFE;
		prop.size = (FLOAT&)record[4];
		memcpy(prop.name,record+8,31);		 
		if(prop.profile>=1024
		||SetFilePointer(pr2,4*prop.profile,0,FILE_BEGIN)
		==INVALID_SET_FILE_POINTER) continue;

		DWORD fp; 
		if(ReadFile(pr2,&fp,4,&recorded,0)&&recorded==4)
		if(!fp||SetFilePointer(pr2,fp+92,0,FILE_BEGIN)
		==INVALID_SET_FILE_POINTER) continue;
		
		if(ReadFile(pr2,&skin,31,&recorded,0)&&skin.skin)
		{
			if(!skins)
			memset(skins=new SOM_MAP_4921ac::Skin[2048],0x00,32*2048);
			skins[i] = skin;
		}
		else if(skins) skins[i].skin = 0;

		SetFilePointer(pr2,31+fp,0,FILE_BEGIN);
	})
	IF_WROTE(Obj,object,56,2,
	{
		memcpy(prop.name,record,31);		
		prop.size = (FLOAT&)record[32];
		prop.profile = (WORD&)record[36]; 
		//2018: som_tool_read disables som_tool_prm
		if(prop.profile==0xFFFF) prop.profile = 0xFFFE; 
		if(prop.profile>=1024||prop.profile>=pr2_s
		||SetFilePointer(pr2,4+prop.profile*108,0,FILE_BEGIN)
		==INVALID_SET_FILE_POINTER) continue;

		SOM_MAP_4921ac::ObjProf &prof = 
		SOM_MAP_4921ac.objectprofs[prop.profile];
		ReadFile(pr2,record,108,&recorded,0); 
		memcpy(prof.model,record+31,31);
		prof.model[30] = '\0';		
		prof.kind = record[82];
		prof.kind2 = record[106];
		prof.axes = record[107];		
		continue; //important
	})
	#undef IF_WROTE

	if(!selcb) return; //!!
	
	//populate props/light Record button//
	
	char *names[4] = 
	{
		SOM_MAP_4921ac.enemyprops[0].name,
		SOM_MAP_4921ac.NPCprops[0].name,
		SOM_MAP_4921ac.objectprops[0].name,
		SOM_MAP_4921ac.itemprops[0].name,
	};
	int i, stride = 34, n = 1024;		
	for(i=0;i<4&&sel!=som_map_treeview[i];i++);	
	if(i!=3) stride+=6; else n = 250;
	int CurSel = 
	ComboBox_GetCurSel(selcb);
	ComboBox_ResetContent(selcb);
	char *p = names[i];
	char *f = i==3?"[%03d] %s":"[%04d] %s";
	for(int i=0;i<n;i++,p+=stride)
	{
		sprintf((char*)record,f,i,p); 
		SendMessageA(selcb,CB_ADDSTRING,0,(LPARAM)record);
		SendMessage(selcb,CB_SETITEMDATA,i,i);
	}
	ComboBox_SetCurSel(selcb,CurSel);
	SendMessage(som_map_tileviewinsert,
	WM_COMMAND,MAKEWPARAM(1021,CBN_SELENDOK),(LPARAM)selcb);
}

static LRESULT som_prm_profile(HWND hWnd, UINT Msg, char *p)
{
	//Reminder: CB_INSERTSTRING ignores CBS_SORT

	switch(Msg)
	{
	case CB_ADDSTRING: 
	case CB_INSERTSTRING: 
	{				
		som_prm tab; som_prm::group g = tab(0);				 
		size_t sort, id = ComboBox_GetCount(hWnd);
		for(sort=0;sort<tab.sorts&&id>=g.count;g=tab(++sort)) 
		id-=g.count;	 
		if(g+id==som_prm::separator) //NEW
		{
			if(1001==som_prm_tab) //som_tool_prf
			SOM_PRM_items.insert(SOM_PRM_items.end()-1,0); 

			const wchar_t *lv = 0, *rv = L"";
			if(EX::INI::Editor()->sort_(sort,&lv,&rv)&&lv&&!*lv)
			{
				som_tool_boxen.add(hWnd,L"");
				SendMessage(hWnd,CB_INSERTSTRING,-1,(LPARAM)rv);
				return som_prm_profile(hWnd,Msg,p);
			}
		}
		const wchar_t *boxen,*format = L"%ls"; 
		EX::INI::Editor()->sort_(sort,0,&format); WCHAR string[64] = L"";	
		swprintf(string,64,format,tab[g+id].description); assert(*string);
		boxen = wcschr(format,'%'); //2018//
		som_tool_boxen.add(hWnd,string+(boxen?boxen-format:0)); 
		return SendMessageW(hWnd,CB_INSERTSTRING,-1,(LPARAM)string);
	}
	case CB_FINDSTRING: 
	case CB_FINDSTRINGEXACT:
	case CB_SELECTSTRING:
	{
		som_prm tab; const som_prm::key *key = &tab[0];

		//30: partial PRO file detection
		if(strchr(p,'.')||strlen(p)==30) //profile
		{	
			const wchar_t *w = //utf8
			EX::need_unicode_equivalent(65001,p);
			
			size_t n = wcslen(w); while(w[n-1]==' ') n--; //UTF8 padding

			//bsearch could work here.
			for(wchar_t i=0,/*n*/count=tab.keys();i<count;i++) 
			{
				int compile[EX_ARRAYSIZEOF(key[i].profile)==10+5];
				if(!wcsnicmp(w,key[i].longname(),n)) return tab.first(i,CB_ERR);
			}
		}
		for(wchar_t i=0,n=tab.keys();i<n;i++) //fallback
		{
			const wchar_t *w = //assuming 932
			EX::need_unicode_equivalent(932,p);

			if(!wcscmp(w,key[i].description)) return tab.first(i,CB_ERR);
		}		
		return CB_ERR;
	}
	default: assert(0); 
	}
	return CB_ERR;
}

static LRESULT WINAPI som_tool_SendMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{			
	SOM_TOOL_DETOUR_THREADMAIN(SendMessageA,hWnd,Msg,wParam,lParam)
		
	//guessing this could be overkill to log always
	//EXLOG_LEVEL(7) << "som_tool_SendMessageA()\n";

	const size_t w_s = 64;

	switch(Msg)
	{	
	case LB_SETHORIZONTALEXTENT: //Program?

		//2018: keep SOM_MAP from doing this
		return 0;

	case CB_GETCURSEL:

		//REMOVE ME? (SOM_PRM_reprogram)
		//HACK: rushing to fix SOM_MAP bug (1.2.2.2) (unrelated)
		//complement to SOM_PRM_items_GetWindowTextA
		extern int SOM_PRM_items_mode;	
		if(1202!=SOM_PRM_items_mode&&som_prm_tab==1001)
		if(1041==GetDlgCtrlID(hWnd)) //sword magic
		{
			//the new item tabs are in play, so pull the old item
			//data from the PRM record
			BYTE *p = SOM_PRM_47a708.data()+SOM_PRM_47a708.item()*336;
			return BYTE(p[310]+1); 
		}
		goto A;
				
	case CB_ADDSTRING: 
	case CB_INSERTSTRING: 	
	case CB_FINDSTRING: 
	case CB_FINDSTRINGEXACT:
	case CB_SELECTSTRING:	
	{
		//1002: profile combo box
		if(SOM::tool==SOM_PRM.exe&&1002==GetDlgCtrlID(hWnd)) 
		{
			return som_prm_profile(hWnd,Msg,(char*)lParam);
		}
		//break; //falling thru
	}
	case LB_ADDSTRING:
	case LB_INSERTSTRING:
	case LB_FINDSTRING:
	case LB_FINDSTRINGEXACT:
	case LB_SELECTSTRING:
	{
		EX::preset<LPARAM>
		raii(som_tool_SendMessageA_a=lParam,0);

		WCHAR w[w_s] = L"";
		EX::need_unicode_equivalent(932,(char*)lParam,w,w_s);
		LRESULT out = SendMessageW(hWnd,Msg,wParam,(LPARAM)w);
		if(out==CB_ERR&&Msg==CB_FINDSTRING)
		{				
			//56205: MOVIES->Edit popup->Image File
			//For some reason SOM_SYS looks for "nashi"
			//assert(GetWindowContextHelpId(hWnd)==56205);
			//56200 does this too, although it defaults itself
			//assert(som_sys_slideshow);
			return SendMessage(hWnd,CB_SETCURSEL,0,0);
		}
		return out;
	}			
	case CB_GETLBTEXTLEN:	
	case LB_GETTEXTLEN: lParam = 0;	Msg--;
	case CB_GETLBTEXT:
	case LB_GETTEXT: //limited to Japanese/English
	case WM_GETTEXT: //???
	{
		/*Reminder: CB is used by SOM_SYS's sound effects???*/

		WCHAR w[w_s] = L"";		
		LRESULT out = SendMessageW(hWnd,Msg,wParam,(LPARAM)w);

		if(out==-1) return out; assert(out<w_s); //-1: CB/LB_ERR

		return strlen(EX::need_ansi_equivalent(932,w,(CHAR*)lParam,40)); //!!!!!!!
	}
	case CB_GETCOUNT:
	case CB_GETITEMDATA: //separators
	{
		//On the enemies ******** tab SOM_PRM reads back the 
		//Profile combobox's item data during initialization				
		if(som_tool_initializing) switch(som_prm_tab)
		{
		default: assert(0); 

		case 1001: //Item: 2018 expansion?
		
		case 1003: //Magic (what's happening here?)

		case 1007: //2018: Trap //CB_GETITEMDATA

		case 0: break;

		case 1005: case 1006: //Enemy/NPC tabs

			if(IsWindowVisible(hWnd)) break; //Wizard

			if(1002!=GetDlgCtrlID(hWnd)) break; //Profile			

			som_prm tab; som_prm::group g = tab(0);

			if(Msg==CB_GETCOUNT) //minor detail
			{
				size_t out = 0;
				for(size_t j=0;j<tab.sorts;g=tab(++j))
				if(g.count&&g+0!=tab.separator) 
				out+=g.count;				
				return out;
			}
			else for(size_t i=wParam,j=0;/*i*/;g=tab(++j))
			{					
				if(g.count) if(g+0!=tab.separator)
				{
					if(i>=g.count) i-=g.count; else break;
				}
				else wParam++; //separator
			}
		}		
		return SendMessageW(hWnd,Msg,wParam,lParam);
	}
		/////WM_USER outside BEGIN WARNING 1///////

	case LVM_SETITEMA:		Msg = LVM_SETITEMW; break;
	case LVM_SETITEMTEXTA:	Msg = LVM_SETITEMTEXTW; break;	
	case LVM_INSERTITEMA:	Msg = LVM_INSERTITEMW; break;	
	case LVM_FINDITEMA:	 	Msg = LVM_FINDITEMW; break;
	case LVM_INSERTCOLUMNA: Msg = LVM_INSERTCOLUMNW; break;

	case TVM_SETITEMA:		Msg = TVM_SETITEMW; break;
	case TVM_INSERTITEMA:	Msg = TVM_INSERTITEMW; break;

	case TVM_DELETEITEM: Msg = Msg; goto A; //breakpoint

	//watchlist		
	case LVM_GETCOLUMNA:
	case LVM_GETSTRINGWIDTHA:
		
		assert(0); goto A; //monitoring

	case LVM_GETITEMA:	
	case LVM_GETITEMTEXTA:
	{
		LVITEMA	*iA = (LVITEMA*)lParam;

		assert(~iA->mask&LVIF_TEXT); goto A;
	}
	case TVM_GETITEMA:
	{
		TVITEMEXA *iA = (TVITEMEXA*)lParam;

		assert(~iA->mask&TVIF_TEXT); goto A;
	}
	case TBM_SETPOS:
	{
		if(som_map_tileview)		
		if(som_map_tileviewinsert==GetParent(hWnd))
		switch(GetDlgCtrlID(hWnd))
		{
		case 1022: case 1023: case 1024:
		case 1028: case 1029: case 1030:
		case 1034: if(!som_tool_classic) return 0; assert(0);
		}
		goto A;
	}
		/////END WARNING 1/////////////////////////

	default: A:

		return SOM::Tool.SendMessageA(hWnd,Msg,wParam,lParam);
	}

	CHAR **pp = 0;

	switch(Msg)
	{		
		/////WM_USER outside BEGIN WARNING 2///////

	case LVM_SETITEMW:
	case LVM_GETITEMW:
	case LVM_INSERTITEMW:	
	case LVM_SETITEMTEXTW: //???
	{
		LVITEMA	*iA = (LVITEMA*)lParam;
		if(iA->mask&LVIF_TEXT||Msg==LVM_SETITEMTEXTW)
		if(iA->pszText!=LPSTR_TEXTCALLBACKA)
		{
			pp = &iA->pszText;
		}		
		break;
	}
	case LVM_FINDITEMW:	   	
	{
		LVFINDINFOA	*fiA = (LVFINDINFOA*)lParam;
		if(fiA->flags&(LVFI_PARTIAL|LVFI_STRING)) pp = (CHAR**)&fiA->psz; 		
		break;
	}
	case LVM_INSERTCOLUMNW:	
	{
		LVCOLUMNA *cA = (LVCOLUMNA*)lParam;
		if(cA->mask&LVCF_TEXT) pp = &cA->pszText; 		
		break;
	}
	case TVM_SETITEMW:
	{
		TVITEMEXA *iA = (TVITEMEXA*)lParam;

		if(iA->mask&TVIF_TEXT)
		if(iA->pszText!=LPSTR_TEXTCALLBACKA) 
		{
			pp = &iA->pszText;
		}
		break;
	}
	case TVM_INSERTITEMW: 
	{
		TVINSERTSTRUCTA  *isA = (TVINSERTSTRUCTA*)lParam;
		
		if(isA->item.mask&TVIF_TEXT)
		if(isA->item.pszText!=LPSTR_TEXTCALLBACKA) 
		{
			pp = &isA->item.pszText;
		}
		break;
	}
	default: assert(0); //?!

		/////END WARNING 2/////////////////////////
	}
	
	if(!pp||!*pp) return SendMessageW(hWnd,Msg,wParam,lParam);
		
	EX::preset<LPARAM> raii(som_tool_SendMessageA_a=(LPARAM)*pp,0);

	char *q = *pp;
	WCHAR w[w_s] = L"";
	*(LPCWSTR*)pp = EX::need_unicode_equivalent(932,*pp,w,w_s);
	LRESULT out = SendMessageW(hWnd,Msg,wParam,lParam);
	*pp = q;
	return out;	
}

inline BOOL som_map_dispatch(LPMSG A)
{
	if(A->message>=WM_KEYFIRST&&A->message<=WM_KEYLAST)
	if(A->hwnd!=SOM_MAP.painting&&A->hwnd!=SOM_MAP.palette)
	{
		switch(A->message)
		{
		case WM_KEYUP: case WM_KEYDOWN:

			if('Q'==A->wParam)
			{
				som_map_Q(); return 0; 
			}			
			break;
		}

		dispatch:
		if(!IsDialogMessageW(GetActiveWindow(),A))
		{
			/*not clear this is required
			#ifdef NDEBUG
			if(!TranslateAccelerator())
			#endif*/
			{
				TranslateMessage(A); 
				DispatchMessageW(A); 
			}
		}
		return 1;
	}
	else //som_map_AfxWnd42sproc?
	{
		//hooked? keys never get to	som_map_AfxWnd42sproc?
		if(A->message==WM_KEYDOWN)
		if(A->hwnd==SOM_MAP.painting) //HACK 
		{
			switch(A->wParam)
			{
			case VK_RETURN: case VK_SPACE:
			
				//2021: merge Enter/Space... Enter never gets
				//to som_map_AfxWnd42sproc
				if(SOM_MAP_GetKeyState_Ctrl())
				{
					som_map_opentileview(0,0,0,0);
					return 0;
				}
				else A->wParam = VK_RETURN; break;
			
			case 'C':

				//HACK: route through som_tool_subclassproc so 
				//it can copy the elevation/ambient value into
				//the setting box
				if(GetKeyState(VK_CONTROL)>>15) 
				{
					SendMessage(som_tool,WM_COMMAND,1213,0);
					return 0;
				}
				break;
			}
		}
		else if(A->hwnd==SOM_MAP.palette) switch(A->wParam)
		{
		case VK_RIGHT: case VK_LEFT:
		{
			//this is new handling for the best file palette tiles since
			//it doesn't work automatically
			RECT cr; GetClientRect(SOM_MAP.palette,&cr);
			(POINT&)cr = (POINT&)som_map_FrameRgn_palette;
			if(cr.top>cr.bottom-42)
			{
				//HACK: the bottom row is a PITA
				SCROLLINFO si = {sizeof(si),SIF_POS|SIF_RANGE|SIF_PAGE};
				if(GetScrollInfo(A->hwnd,SB_VERT,&si))
				if(si.nPos+(int)si.nPage>=si.nMax)				
				cr.right = SOM_MAP_4921ac.partsN%(cr.right/21)*21;				
			}
			if(A->wParam==VK_RIGHT)			
			cr.left = cr.left+42>cr.right?0:cr.left+21;
			else //Add a wrapping function?
			cr.left = cr.left-21>=0?cr.left-21:cr.right-20;			
			SendMessage(A->hwnd,WM_LBUTTONDOWN,0,POINTTOPOINTS((POINT&)cr));
			SendMessage(A->hwnd,WM_LBUTTONUP,0,POINTTOPOINTS((POINT&)cr));
			//HACK: Ensure som_map_FrameRgn_palette is up-to-date.
			RedrawWindow(A->hwnd,0,0,RDW_UPDATENOW|RDW_INVALIDATE);			
			//HACK: finish up for som_map_refresh_model_view()
			som_map_tileviewmask&=~0x200;
			return 1;
		}}
	}
	return 0;
}
static bool som_tool_GetMessageA_mnemonics(LPMSG A)
{
	static WPARAM alt = 0; //sticky Alt key
	if(A->wParam==VK_CONTROL) 
	{
		//dialogs don't receive VK_CONTROL
		extern void PrtsEdit_refresh_1022();
		switch(A->message)
		{
		case WM_KEYDOWN: case WM_KEYUP: PrtsEdit_refresh_1022();
		}
	}
	else /*if(A->wParam!=VK_CONTROL)*/ switch(A->message)
	{	
	//fyi: WM_SYSKEYUP for Alt is iffy
	case WM_SYSKEYDOWN: case WM_SYSKEYUP: 

		switch(A->wParam)
		{
		case VK_LMENU: case VK_RMENU: assert(0);
		case VK_MENU: 
	
			if(A->message==WM_SYSKEYDOWN) 
			{
				alt = 1; 
				if(UISF_HIDEACCEL&SendMessage(A->hwnd,WM_QUERYUISTATE,0,0))
				{
					//winsanity: not done reliably at all?!
					HWND ga = GetAncestor(A->hwnd,GA_ROOT);
					//kind of defeats the point of som_tool_subclassproc's WM_CHANGEUISTATE
					SetWindowRedraw(ga,0); SendMessage(ga,WM_UPDATEUISTATE,UIS_INITIALIZE,0);
					SetWindowRedraw(ga,1); som_tool_togglemnemonics(ga);
				}
			}
			return true;

		default: alt = 0; som_tool_VK_MENU = 0;
		}	
		break;

	case WM_KEYDOWN: 

		switch(A->wParam)
		{
		case VK_SPACE:

			if(alt==1) //open the system menu
			{
				//for some reason this isn't automatic
				SendMessage(GetAncestor(A->hwnd,GA_ROOT),WM_SYSCOMMAND,SC_KEYMENU,' ');
				alt = 0; return true;
			}
			break;

		default: //limit &mnemonics to Alt key combos
			
			if(alt==1) alt = A->wParam;	if(alt==A->wParam) 
			{	
				assert(alt!=VK_F4);
				assert(~GetKeyState(VK_MENU)>>15);
				
				//som_tool_VK_MENU = 0x8001; //docs lie!
				som_tool_VK_MENU = 0xff81;
				A->message = WM_SYSKEYDOWN; return true;
			}		
			//not just buttons send mnemonics (AfxWnd42s)
			//DLGC_WANTALLKEYS exempts popup style labels
			enum{exempt=DLGC_WANTCHARS|DLGC_WANTALLKEYS};
			if(!(exempt&SendMessage(A->hwnd,WM_GETDLGCODE,0,0)))
			{
				if(~GetKeyState(VK_MENU)>>15
				&&~GetKeyState(VK_CONTROL)>>15) //workshop_accelerator
				{
					MessageBeep(-1);
					alt = som_tool_VK_MENU = 0; return false;
				}
				assert(!som_tool_VK_MENU&&A->wParam!=VK_F4);
			}
			//break;

		//generic blacklist
		//somehow WM_CHAR/UNICHAR need to be used
		//(reminder: the ToUnicode API looks like a bad idea??)
		case VK_LWIN: case VK_RWIN: case VK_APPS:

		//navigation blacklist
		case VK_MENU: case VK_LMENU: case VK_RMENU:
		case VK_LEFT: case VK_RIGHT: case VK_UP: case VK_DOWN: 
		case VK_TAB: case VK_PRIOR: case VK_NEXT: case VK_RETURN: 
		case VK_F4: case VK_CONTROL: case VK_SHIFT: case VK_ESCAPE: break; 

		//workshop_accelerator?
		case VK_F1: case VK_F2: case VK_F3: case VK_F5: case VK_F6:
		case VK_F7: case VK_F8: case VK_F9: case VK_F10: case VK_F11: 
		case VK_F12: break;
		}
		goto up; //break;

	case WM_LBUTTONDOWN: case WM_LBUTTONUP:
	case WM_RBUTTONDOWN: case WM_RBUTTONUP: //case WM_MOUSEMOVE:
		//if(EX::debug) break; 
	case WM_KEYUP: up:
		
		som_tool_VK_MENU = 0; 
		
		if(A->wParam!=VK_MENU&&A->wParam!=alt) alt = 0;
	}		
	return true;
}
static BOOL WINAPI som_tool_GetMessageA(LPMSG A,HWND B,UINT C,UINT D)
{		
	SOM_TOOL_DETOUR_THREADMAIN(GetMessageA,A,B,C,D)
		
	//guessing this could be overkill to log always
	//EXLOG_LEVEL(7) << "som_tool_GetMessageA()\n";

	assert(!B&&!C&&!D); again:

	//som_tool_gettingmessage = true;
	BOOL out = GetMessageW(A,B,C,D);
	//som_tool_gettingmessage = false;

	/*see som_tool_gettingmessage declaration
	//REMOVE ME?
	if(som_tool_gettingmessage_lock)
	//hack: patching system wide repaint bug
	if(EX::tick()-som_tool_gettingmessage_wait>100)
	{	
		//LockWindowUpdate(0);
		SetWindowRedraw(som_tool_gettingmessage_lock,1);
		RedrawWindow(som_tool_gettingmessage_lock,0,0,
		RDW_VALIDATE|RDW_NOINTERNALPAINT|RDW_NOERASE|RDW_NOFRAME|RDW_ALLCHILDREN); 
		ValidateRect(som_tool_gettingmessage_lock,0);
		som_tool_gettingmessage_lock = 0;		
	}*/

	if(A->wParam==VK_F4&&!som_tool_classic)
	{
		//2017: recieving Up event after closing SOM_DB
		//maybe it should close on up? other applications
		//have the same problems
		static DWORD altF4 = 0;

		//dangerous: doesn't keep a repeat count!
		if(A->message==WM_SYSKEYDOWN) 
		{
			altF4 = EX::tick(); goto again;
		}
		if(A->message==WM_SYSKEYUP)
		{
			HWND ga = GetAncestor(A->hwnd,GA_ROOT);
			if(GetWindowContextHelpId(ga)) //can WM_CLOSE?	
			{
				//just guarantee the "down" belongs to this tool
				//really it seems like Windows should tie the up
				//and down to the application (but it doesn't)
				if(EX::tick()-altF4<2000)
				{
					//SOM seems to bypass WM_SYSCOMMAND/SC_CLOSE??
					SendMessage(ga,WM_CLOSE,0,0); //goto again;
				}
			}
			goto again; //2018: still having issue... try to hide???
		}
	}
	
	if(!som_tool_classic)
	if(A->message==WM_SYSCHAR) //makes SOM close!!
	{
		//FUNNY? DOESN'T CLOSE IF som_tool_classic
		//note: this only happens to the top level
		//windows and isn't done in the procedures
		//instead it simulates pressing the button 
		//IsDialogMessageW(GetActiveWindow(),A); goto again;	
		goto syschar;
	}
	else if(!som_tool_GetMessageA_mnemonics(A)) goto again;	

	if(out>0) switch(SOM::tool)
	{
	case SOM_MAP.exe:
		
		if(som_map_dispatch(A)) goto again; break;

	case SOM_MAIN.exe: case SOM_EDIT.exe: //extended dialogs...

	case EneEdit.exe: case NpcEdit.exe: case SfxEdit.exe:
	
		syschar:
	  	if(HWND aw=GetActiveWindow())
		{
			//00402BF4 8B 15 30 76 41 00    mov         edx,dword ptr ds:[417630h]
			//NpcEdit
			//00402BE4 8B 15 18 76 41 00    mov         edx,dword ptr ds:[417618h]
			if(SOM::tool>=EneEdit.exe&&aw==som_tool)
			{
				assert(workshop_accel);
				//if(TranslateAcceleratorW(aw,*(HACCEL*)(SOM::tool==NpcEdit.exe?0x417618:0x417630),A))
				if(TranslateAcceleratorW(aw,workshop_accel,A))
				goto again;
			}
			if(IsDialogMessageW(aw,A)) goto again; 		
		}
		break;
	}
	return out;
}
static BOOL WINAPI som_tool_PeekMessageA(LPMSG A,HWND B,UINT C,UINT D,UINT E)
{
	BOOL out = PeekMessageW(A,B,C,D,E);
	
	/*crashing??? (SOM_MAP doesn't seem to use it anyway)
	if(out>0)
	if(SOM::tool==SOM_MAP.exe) while(som_map_dispatch(A))
	{
		out = PeekMessageW(A,B,C,D,E); if(out<=0) break;
	}*/

	//EneEdit GOES FROM 1% TO 30% OF PROCESSOR LOAD WHEN 
	//IT IS IN THE BACKGROUND
	if(!out&SOM::tool>=EneEdit.exe) //NpcEdit too
	{
		int i = 1; while(!som_tool_stack[i]&&i) i--;

		/*2021: it seems to be doing it in the foreground
		//as well now :(
		//if(0==GetActiveWindow()) //WaitMessage(); 
		{
			//for some reason, when deleting/pasting in
			//GetOpen/SaveFileName a dialog appears that
			//hangs here???
			//SIMPLER? if disabled, assume either inside a
			//prompt or the som_tool_workshop is in hiding
			//HWND ga = GetAncestor(GetForegroundWindow(),GA_ROOTOWNER);
			//if(ga==som_tool||ga==som_tool_taskbar||ga==som_tool_workshop)
			if(IsWindowEnabled(som_tool)||!IsWindowVisible(som_tool))*/
			if(IsWindowEnabled(som_tool_stack[i])||!IsWindowVisible(som_tool))
			{
				WaitMessage(); 
				//BEST? go ahead and give it the message?
				return PeekMessageW(A,B,C,D,E);
			}
		//}
	}

	return out;
}

extern bool som_tool_MessageBoxed = false;
int WINAPI som_tool_MessageBoxA(HWND A,LPCSTR B,LPCSTR C,UINT D)
{		
	SOM_TOOL_DETOUR_THREADMAIN(MessageBoxA,A,B,C,D)	
	
	EXLOG_LEVEL(7) << "som_tool_MessageBoxA()\n";

	//taskbar/also just passing 0 in extended boxes
	A = som_tool_dialog(); 

	EX::preset<bool> raii
	(som_tool_MessageBoxed=true,false); //testing

	static HMENU prompts = LoadMenuW(0,L"PROMPTS");
	static HMENU errors = GetSubMenu(prompts,0);
	static HMENU reports = GetSubMenu(prompts,1);
	
	int code = -1, d = -1; //testing
		
	if(prompts)	switch(SOM::tool) 
	{
	case SOM_MAIN.exe: sscanf(B,"MAIN%04d",&code); break;
	case SOM_EDIT.exe: sscanf(B,"EDIT%04d",&code); break;
	case SOM_PRM.exe: sscanf(B,"PRM%04d",&code); break;
	case SOM_MAP.exe: sscanf(B,"MAP%04d",&code); break;
	case SOM_SYS.exe: sscanf(B,"SYS%04d",&code); break;
	}

	//normalize "DXR" codes to standard codes
	if(code==-1&&sscanf(B,"DXR%03d",&code)==1)
	{
		if(SOM::tool==SOM_MAP.exe
		 ||SOM::tool==SOM_PRM.exe)
		{
			if(code==1||code==2) //TXR/BMP
			{
				//MAP035: cannot load file
				//PRM000: same thing, but made up
				code = SOM::tool==SOM_MAP.exe?35:0; 
			}
			else assert(0);
		}
		else assert(0);		
	}

	const size_t Bw_s = 512; wchar_t Bw[Bw_s] = L"";
		
	if(SOM_MAP.exe==SOM::tool)
	{
		switch((DWORD)B) //normalize?
		{
		case 0x48fba0: //keep changes?
		case 0x48fde8: //same (maps popup)

			if(SOM::tool==SOM_MAP.exe) code = 1000; break;

		//mapcomp.exe
		//case "MAP032": //precedes 012 if !CreateProcess
		//case 0x48fcd4: break; //(failure report MAP012)
		case 0x48fce8: //success
		
			if(SOM::tool==SOM_MAP.exe) code = 1012; break;
		}

		if(1==som_tool_play) //Skip?
		{
			if(code==1000) return IDYES; //Save?

			som_tool_play = 0;

			if(code==1012) //Mapcomp finished successfully
			{
				PostMessage(som_tool,WM_COMMAND,1071,0);

				return IDOK;
			}
		}
	}

	//FIX ME 
	//FIX ME (possibly casting this net too wide!)
	//FIX ME 
	if(code==-1) //??? SOM_PRM item Cut bug (1015)
	{
		if(D==0x30) //exclaimation
		if(SOM::tool==SOM_PRM.exe)
		{
			//TODO: figure out where this code originates
			return IDOK;
		}
	}

	if(code>=0&&code<1000)
	{			
		if(errors //3: GRAYED/DISABLED
		&&!(3&GetMenuState(errors,code,0))
		&&GetMenuStringW(errors,code,Bw,Bw_s,0)
		&&code==33) //extract subcode
		{	
			char *p = 0;
			strtol(B+3,&p,10); 			
			if(p==B+6) 
			{
				while(*p&&(*p<'0'||*p>'9'))
				p++;
				if(*p) d = atoi(p);
			}
		}			   
	}
	else if(reports)
	{
		if(code>=1000 //3: GRAYED/DISABLED
		&&!(3&GetMenuState(reports,code,0)))
		GetMenuStringW(reports,code,Bw,Bw_s,MF_BYCOMMAND);
	}

	if(!*Bw) EX::need_unicode_equivalent(932,B,Bw,Bw_s);

	if(d!=-1) if(wchar_t*_=wcsstr(Bw,L"%d")) //2022
	{
		wchar_t w[16]; _itow(d,w,10);

		int m = wcslen(w), n = wcslen(_)+1;

		wmemmove(_+m,_+2,n-2); wmemcpy(_,w,m);
	}

	const DWORD help = som_tool_f1?MB_HELP:0;
	//todo: consider C (in some cases it is not exe())
	MSGBOXPARAMSW mbp = {sizeof(mbp),A,0,Bw,EX::exe(),D|help,0,code,0,0};

	mbp.dwStyle|=MB_SETFOREGROUND; //SOM_RUN
	mbp.dwStyle|=MB_TOPMOST; //splash screen

	int ret = MessageBoxIndirectW(&mbp);

	if(IDCANCEL==ret) //layer menu?
	{
		extern bool som_LYT_label(HWND,const char*);
		if(code==1000&&SOM::tool==SOM_MAP.exe&&A==som_tool)
		{
			som_tool_refocus = 0; //HACK
			som_LYT_label(GetDlgItem(som_tool,1133),0);
		}
	}

	return ret;
}

static HMMIO WINAPI som_tool_mmioOpenA(LPSTR A, LPMMIOINFO B, DWORD C)
{
	SOM_TOOL_DETOUR_THREADMAIN(mmioOpenA,A,B,C)

	EXLOG_LEVEL(7) << "som_tool_mmioOpenA()\n";
	
	EXLOG_LEVEL(3) << "Opening " << A << '\n';

	const wchar_t *w = SOM::Tool::project(A);
	
	if(!w) return SOM::Tool.mmioOpenA(A,B,C);
	
	HMMIO out = mmioOpenW(const_cast<WCHAR*>(w),B,C);
		
	if(!out) //look in old som_rt.exe style folder?
	{			
		char sound[MAX_PATH] = SOMEX_(B)"\\data\\sound\\"; //bgm
		w = SOM::Tool::project(strcat(sound,A+sizeof(SOMEX_(B))+4+1));
		out = mmioOpenW(const_cast<WCHAR*>(w),B,C);
	}
	return out;
}

//supplements/validates helpIDs 
static DWORD som_tool_id(HWND guess)
{
	if(som_tool_MessageBoxed)
	{
		assert(0); return 0; //testing
	}

	if(SOM::tool==SOM_MAP.exe) 
	{	
		if(som_tool==guess)
		{
			//157: Loading & MapComp task/report
			if(!SOM_MAP.prt.loaded()) return 40100; 

			return 40000; //102: SOM_MAP main window
		}	

		//double-clicking
		if(som_tool_pushed==SOM_MAP.painting) goto som_map_1001;

		//todo: windows not opened by buttons
		DWORD cid = GetDlgCtrlID(som_tool_pushed);
		DWORD pid =	GetWindowContextHelpId(GetParent(som_tool_pushed));
		if(!cid||!pid) return 0;

		switch(pid)
		{
		case 40000: //102: SOM_MAP main window

			switch(cid)
			{
			case 1227: return 41000; //201: maps menu			
			case 1072: return 43000; //200: lights editor
			case 1069: return 44000; //153: details editor
			case 1070: return 45000; //173: events menu
			case 1001: //return 42000; //174: tile editor

				som_map_1001:
				if(40000!=GetWindowContextHelpId(GetParent(guess)))
				{
					if(WS_CAPTION&~GetWindowStyle(guess))
					return 42100; //nested children windows
					return 0;
				}
				else return 42000; //174: tile editor
			}
			break;

		case 42000: //174: SOM_MAP tile editor

			switch(cid)
			{
			case 1161: return 45100; //130: event editor
			}
			break;

		case 45000: //173: events menu

			switch(cid)
			{
			case 1210: //listbox
			case 1211: return 45100; //130: event editor
			}
			break;

		case 45100: //130: event editor

			switch(cid)
			{
			//listbox
			case 1025: return 45300; //166: loop editor
			}
		}
	}
	else if(SOM::tool==SOM_PRM.exe)
	{
		switch(GetDlgCtrlID(som_tool_pushed))
		{
		case 1005: //enemy tab/model viewer

			if(som_tool!=GetParent(som_tool_pushed))
			{
				return 30200; //model viewer
			}
			break;
		}
	}
	return 0;
}

static const bool som_map_fitness = true;
static const int som_map_code50_to_54_gap = 5;

typedef std::pair<std::wstring,int> som_map_codemenuitem;	
typedef std::vector<som_map_codemenuitem> som_map_codemenuitems;

static LRESULT CALLBACK som_map_codecbnproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scID, DWORD_PTR dwData)
{
	switch(uMsg)
	{
	case WM_COMMAND:

		if(wParam==IDOK)
		{	
			switch(HIWORD(scID))
			{
			case 0x50: case 0x54: //translation
			
				LockWindowUpdate(hWnd);
				HWND cb2 = GetDlgItem(hWnd,1044);	
				HWND cb = GetDlgItem(hWnd,LOWORD(scID));
				int sel = SendMessage(cb,CB_GETCURSEL,0,0);
				int sel2 = SendMessage(cb2,CB_GETCURSEL,0,0);				   
				if(som_map_fitness)
				{
					if(0!=sel) //0: fitness
					{
						sel+=4-1; //bump up	3
					}
					else if(sel2<4) //4: HP/MP/Strength/Magic
					{
						SendMessage(cb,CB_SETCURSEL,sel2,0);
						SendMessage(cb2,CB_SETCURSEL,0,0);
					}				
				}				
				if(0x50==HIWORD(scID)) 
				{
					//6: HP/MP/Strength/Magic/Items/Gold
					if(sel>=6) sel+=som_map_code50_to_54_gap;
				}
				wchar_t WTH[16] = L"?";	//box had CBS_UPPERCASE mode???
				while(sel>SendMessageW(cb,CB_ADDSTRING,0,(LPARAM)WTH)); //L""
				SendMessage(cb,CB_SETCURSEL,sel,0); 
			}
		}
		else switch(wParam)
		{
		case MAKEWPARAM(1000,CBN_SELENDOK): //0x50 
		case MAKEWPARAM(1083,CBN_SELENDOK): //0x54 	
		{
			WORD code = HIWORD(scID); //eg. code50

			if(LOWORD(wParam)!=LOWORD(scID)) break;

			som_map_codemenuitems &menu = 
			*(som_map_codemenuitems*)dwData;

			HWND cb = (HWND)lParam;
			HWND cb2 = GetDlgItem(hWnd,1044);
			int sel = SendMessage(cb,CB_GETCURSEL,0,0);
			int sel2 = SendMessage(cb2,CB_GETCURSEL,0,0);			

			bool initializing = !IsWindowVisible(hWnd);

			if(initializing) //translation
			{					
				if(sel==CB_ERR) 
				sel = som_map_codesel; //repairing				
				
				if(code==0x50) 
				//6: HP/MP/Strength/Magic/Items/Gold
				if(sel>=6+som_map_code50_to_54_gap)
				sel-=som_map_code50_to_54_gap;

				if(som_map_fitness)
				if(code==0x54||code==0x50) //legacy
				{					
					if(sel||sel2<4) //support Speed etc.
					{
						if(sel<4) //4: HP/MP/Strength/Magic
						{
							sel2 = sel; sel = 0; //0: fitness
						}
						else sel-=4-1; //take up the slack
					}
				}	  
				sel = SendMessage(cb,CB_SETCURSEL,sel,0);
			}										
			SendMessage(cb2,CB_RESETCONTENT,0,0); 

			COMBOBOXINFO cbi = {sizeof(cbi)};
			if(!GetComboBoxInfo(cb2,&cbi)) cbi.hwndList = 0;
			int cbscroll = GetWindowStyle(cbi.hwndList);

			int id = SendMessage(cb,CB_GETITEMDATA,sel,0);
			if(id<=0) id = sel;

			if(id>=0&&cb2) 
			{
				cbscroll|=WS_VSCROLL;

				int s = menu[id].second; if(s>=0)
				{
					cbscroll&=~WS_VSCROLL;

					while(s>=0&&menu[s].first[0]=='-')
					{
						if(menu[s].second!=-1) //hidden?
						{
							int added = //emulate items list behavior
							SendMessageW(cb2,CB_ADDSTRING,0,(LPARAM)(menu[s].first.c_str()+1));					
							SendMessage(cb2,CB_SETITEMDATA,added,added);
						}
						s = menu[s].second<=0?s+1:menu[s].second;
					}
				}
				else if(s==-10) //-10 indicates magic list
				{
					char buff[66];
					if(SOM::PARAM::Sys.dat->open()
					 &&SOM::PARAM::Magic.prm->open())
					for(int i=0;i<32;i++)
					{
						int j = SOM::PARAM::Sys.dat->magics[i]; 						
						if(j!=0xFF&&*SOM::PARAM::Magic.prm->records[j])
						if(0<sprintf(buff,"[%02d] %s",i,SOM::PARAM::Magic.prm->records[j]))
						{	
							int added = //emulate items list behavior
							SendMessageW(cb2,CB_ADDSTRING,0,(LPARAM)EX::need_unicode_equivalent(932,buff));					
							SendMessage(cb2,CB_SETITEMDATA,added,i);
						}
					}
				}
				else if(-1>s&&s>-10) //-1 indicates hidden
				{
					char buff[66];
					if(SOM::PARAM::Item.prm->open()
					&&(s==-2||SOM::PARAM::Item.pr2->open()))
					for(int i=0;i<250;i++)
					{
						/*2021
						if(s!=-2) //filtering items (unused)
						{
							int j = SOM::PARAM::Item.prm->profiles[i]; 

							const SOM::PARAM::Item::Pr2 *pr2 = SOM::PARAM::Item.pr2;

							if(j<250) switch(s)
							{
							case -3: if(!pr2->fits[j].head) continue; break;
							case -4: if(pr2->uses[j]!=pr2->weapon) continue; break;
							case -5: if(pr2->uses[j]!=pr2->shield) continue; break;
							case -6: if(!pr2->fits[j].hand) continue; break;
							case -7: if(!pr2->fits[j].body) continue; break;
							case -8: if(!pr2->fits[j].feet) continue; break;
							case -9: if(pr2->uses[j]!=pr2->effect) continue; break;
							}
						}*/
						
						//2017: Disabling this. Don't know if it is read back correctly,
						//but this is the new behavior (it probably is correct) and the
						//new som_tool_boxen code depends on every item being presented.
						//if(*SOM::PARAM::Item.prm->records[i])
						if(0<sprintf(buff,"[%03d] %s",i,SOM::PARAM::Item.prm->records[i]))
						{
							int added = //emulate items list behavior
							SendMessageW(cb2,CB_ADDSTRING,0,(LPARAM)EX::need_unicode_equivalent(932,buff));		
							SendMessage(cb2,CB_SETITEMDATA,added,i);
						}
					}
				}				
			}
			EnableWindow(cb2,1<=SendMessage(cb2,CB_GETCOUNT,0,0));	
			if(cbscroll!=SetWindowLong(cbi.hwndList,GWL_STYLE,cbscroll)) 
			{
				//THIS IS TRIGGERING som_tool_boxen::clear's assert(). NOT SURE
				//WHY WAS THIS?
				//som_tool_combobox(cb2); 
				
				EnableWindow(cbi.hwndList,1); //WTH?
			}
			if(!initializing) sel2 = 0;
			SendMessage(cb2,CB_SETCURSEL,sel2,0);
			//return 1; //important
		}
		case MAKEWPARAM(1000,C8N_SELCHANGE): //0x50 
		case MAKEWPARAM(1083,C8N_SELCHANGE): //0x54 

			//deferring to SELENDOK because we may see more than one 
			//SELCHANGE per SELENDOK because of som_tool_subclassproc
			return 1; 
		
		case MAKEWPARAM(1070,CBN_SELENDOK): //0x78 
		case MAKEWPARAM(1071,CBN_SELENDOK): //0x90 
		{
			WORD code = HIWORD(scID); //eg. code90

			if(LOWORD(wParam)!=LOWORD(scID)) break;

			if(!IsWindowVisible(hWnd)) //initialize
			{
				HWND cb = (HWND)lParam;			
				int sel = SendMessage(cb,CB_GETCURSEL,0,0);
				if(sel==CB_ERR) 
				sel = som_map_codesel; //repairing									  
				sel = SendMessage(cb,CB_SETCURSEL,sel,0);
			}	
			//return 1; //important
		}
		case MAKEWPARAM(1070,C8N_SELCHANGE): //0x78 
		case MAKEWPARAM(1071,C8N_SELCHANGE): //0x90 

			//deferring to SELENDOK because we may see more than one 
			//SELCHANGE per SELENDOK because of som_tool_subclassproc
			return 1; 
		}
		break;

	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,som_map_codecbnproc,scID);
		break;
	}

	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
typedef struct
{int a, b; const char c[16]; const char *d;
}som_map_codemenuitems_layout;
static const wchar_t *som_map_codemenuitem_match
(som_map_codemenuitems &out,
 som_map_codemenuitems_layout *layout, int in, const wchar_t *f)
{	
	const char *c = layout[in].c; int s = layout[in].a; 	

	if(*f) switch(c[0])
	{	
	case '=': case '-': 
		
		if(f[0]==c[0])
		{
			if(f[1]!=c[1])
			if(f[0]!='-'||f[1]=='-'||c[1]=='-')	f = 0; 
		}
		else f = 0; break; 
	
	default: if(*f=='-'||*f=='=') f = 0;
	}
	else f = 0;
	if(*c=='=') //guard =N= pattern
	{
		out.push_back(std::make_pair(L"===",s));
		out.back().first[1] = c[1];		
	}
	else //standard operating procedure
	{
		if(!f) f = 
		EX::need_unicode_equivalent(932,layout[in].d);	
		out.push_back(std::make_pair(*c=='-'?L"-":L"",s));
		const wchar_t *app = f; while(*app=='-') app++;
		if(c[1]=='-') //--
		{				
			size_t i, _s; //%s
			for(i=in-1;layout[i].c[1]=='-';i--);		
			_s = out.back().first.assign(out[i].first).find(L"%s");
			if(_s!=std::wstring::npos) 
			{
				out.back().first.replace(_s,2,app); app = 0;
			}
		}
		if(app) out.back().first.append(app);
	}
	return f;
}
static som_map_codemenuitems &som_map_code50(const wchar_t *next)
{
	const int fit = som_map_fitness?'fit':-1;

	size_t i, g3;
	static som_map_codemenuitems out; 
	static som_map_codemenuitems_layout layout[] = 
	{//fitness
	{0,(i=0)++,"HP","HP"},
	{0,i++,"MP","MP"},
	{0,i++,"Pow1",som_932_Powers[0]},
	{0,i++,"2",som_932_Powers[1]},
	{-2,i++,"Item",som_932_Item_Haul}, //-2 indicates item list
	{i,i++,"Gold",som_932_Gold_Haul},
	{fit,i++,"Fitness",som_932_Fitness}, //HP, MP, Pow 1-2&3
	{-1,i++,"-Pow3",som_932_Powers[2]}, //-1 indicates hidden item
	/*//som_map_code50_to_54_gap
	{i+1,i++,"Level",som_932_LVT},	
	{-10,i++,"Spells",som_932_Magic_Table},
	{i+1,i++,"Status",som_932_Status},
	{i+1,i++,"System",som_932_System},
	{i+1,i++,"Fall!",som_932_Fall},*/
	{-1,g3=i++,"=3=","=3="},
	{i+1,i++,"Head",som_932_Head}, 
	{0,i++,"-Equip ID",som_932_Item},
	{g3+2,i++,"Weapon",som_932_Weapon},
	{g3+2,i++,"Shield",som_932_Shield},
	{g3+2,i++,"Hands",som_932_Hands},
	{g3+2,i++,"Body",som_932_Body},
	{g3+2,i++,"Feet",som_932_Feet},
	{g3+2,i++,"Accessory",som_932_Accessory},
	{i+1,i++,"Magic",som_932_Magic},
	{g3+3,i++,"-Spell ID",som_932_Spell},
	};if(next)
	for(i=out.size();i<EX_ARRAYSIZEOF(layout);i++)
	if(next==som_map_codemenuitem_match(out,layout,i,next)) 
	break; return out;
}
static som_map_codemenuitems &som_map_code54(const wchar_t *next)
{
	const int fit = som_map_fitness?'fit':-1;

	size_t i, g3;
	static som_map_codemenuitems out; 
	static som_map_codemenuitems_layout layout[] = 
	{//fitness
	{0,(i=0)++,"HP","HP"},
	{0,i++,"MP","MP"},
	{0,i++,"Pow1",som_932_Powers[0]},
	{0,i++,"2",som_932_Powers[1]},
	{-2,i++,"Item",som_932_Item_Haul}, //-2 indicates item list
	{i,i++,"Gold",som_932_Gold_Haul},
	{i+1,i++,"Level",som_932_LVT},
	
		//som_map_code50_to_54_gap
		{0,i++,"-Up!",som_932_LEVEL},
		{0,i++,"-Next",som_932_LVT_Next},
		{0,i++,"-Max1",som_932_LVT_Max[0]},
		{0,i++,"-2",som_932_LVT_Max[1]},
		{0,i++,"-3",som_932_LVT_Max[2]},
		{0,i++,"-4",som_932_LVT_Max[3]},
		{-1,i++,"-5",som_932_LVT_Max[4]}, //Max Speed
		{fit,i++,"Fitness",som_932_Fitness},//HP, MP, Pow 1-2&3
		{-1,i++,"-Pow3",som_932_Powers[2]}, //-1 indicates hidden item	
		{-10,i++,"Spells",som_932_Magic_Table}, //-10 indicates magic list
		{-1,i++,"=1=","=1="},
		{i+1,i++,"Status",som_932_Status},
		{i+2,i++,"-Normal",som_932_Normal},
		{0,i++,"-%s",""}, //planning ahead
		{0,i++,"--Poison",som_932_States[0]+1},
		{0,i++,"--Palsy",som_932_States[1]+1},
		{0,i++,"--Blind",som_932_States[2]+1},
		{0,i++,"--Curse",som_932_States[3]+1},
		{0,i++,"--Slow",som_932_States[4]+1},
		{i+1,i++,"System",som_932_System},
		{0,i++,"-Map",som_932_Map},
		{0,i++,"-Setup",som_932_Player_Setup},
		{0,i++,"-Save",som_932_Save},
		{0,i++,"-Dash",som_932_Dash},
		{0,i++,"-Play",som_932_Play},
		{-1,i++,"=2=","=2="},
		{i+1,i++,"Fall!",som_932_Fall},	
		{0,i++,"-Height",som_932_Fall_Height},
		{0,i++,"-Impact",som_932_Fall_Impact},
	
	{-1,g3=i++,"=3=","=3="},
	{i+1,i++,"Head",som_932_Head},
	{i+2,i++,"-Equip ID",som_932_Item},
	{0,i++,"-%s defense",som_932_defense},
	{0,i++,"--Cut",som_932_attacks[0]},
	{0,i++,"--Hit",som_932_attacks[1]},
	{0,i++,"--Poke",som_932_attacks[2]},
	{0,i++,"--Fire",som_932_attacks[3]},
	{0,i++,"--Earth",som_932_attacks[4]},
	{0,i++,"--Wind",som_932_attacks[5]},
	{0,i++,"--Water",som_932_attacks[6]},
	{i+10,i++,"--Holy",som_932_attacks[7]},	
	{0,i++,"-%s attack",som_932_attack},
	{0,i++,"--Cut",som_932_attacks[0]},
	{0,i++,"--Hit",som_932_attacks[1]},
	{0,i++,"--Poke",som_932_attacks[2]},
	{0,i++,"--Fire",som_932_attacks[3]},
	{0,i++,"--Earth",som_932_attacks[4]},
	{0,i++,"--Wind",som_932_attacks[5]},
	{0,i++,"--Water",som_932_attacks[6]},
	{0,i++,"--Holy",som_932_attacks[7]},	
	{i+1,i++,"Weapon",som_932_Weapon},
	{g3+13,i++,"-Equip ID",som_932_Item},
	{g3+2,i++,"Shield",som_932_Shield},
	{g3+2,i++,"Hands",som_932_Hands},
	{g3+2,i++,"Body",som_932_Body},
	{g3+2,i++,"Feet",som_932_Feet},
	{g3+2,i++,"Accessory",som_932_Accessory},
	{i+1,i++,"Magic",som_932_Magic},
	{g3+13,i++,"-Spell ID",som_932_Spell},
	};if(next)
	for(i=out.size();i<EX_ARRAYSIZEOF(layout);i++)
	if(next==som_map_codemenuitem_match(out,layout,i,next)) 
	break; return out;
}
static som_map_codemenuitems &som_map_code78(const wchar_t *next)
{
	size_t i;
	static som_map_codemenuitems out; 
	static som_map_codemenuitems_layout layout[] = 	
	{{i=0,i++,"Save",som_932_Save},
	{i,i++,"Dash",som_932_Dash},
	{i,i++,"Play",som_932_Play},
	};if(next)
	for(i=out.size();i<EX_ARRAYSIZEOF(layout);i++)
	if(next==som_map_codemenuitem_match(out,layout,i,next)) 
	break; return out;
}
static som_map_codemenuitems &som_map_code90(const wchar_t *next)
{
	size_t i;
	static som_map_codemenuitems out; 
	static som_map_codemenuitems_layout layout[] = 
	{{i=0,i++,"Set",som_932_Math[0]},
	{i,i++,"Add",som_932_Math[1]},
	{i,i++,"Sub",som_932_Math[2]},
	{i,i++,"Sub2",som_932_Math[3]},
	{i,i++,"Mul",som_932_Math[4]},
	{i,i++,"Div",som_932_Math[5]},
	{i,i++,"Div2",som_932_Math[6]},
	{i,i++,"Per",som_932_Math[7]}, //Percentage
	{i,i++,"Per2",som_932_Math[8]},
	{i,i++,"Rem",som_932_Math[9]}, //Remainder
	{i,i++,"Rem2",som_932_Math[10]},
	};if(next)
	for(i=out.size();i<EX_ARRAYSIZEOF(layout);i++)
	if(next==som_map_codemenuitem_match(out,layout,i,next)) 
	break; return out;
}
static void som_map_code(int code, HWND dialog)
{		
	bool fitness = 
	som_map_fitness?code==0x50||code==0x54:false;

	int cbid = 0; 
	som_map_codemenuitems&(*init)(const wchar_t*) = 0;		
	switch(code) //ASSUMING CODE IS COMBOBOX ORIENTED
	{	
	case 0x50: cbid = 1000; init = som_map_code50; break;
	case 0x54: cbid = 1083; init = som_map_code54; break;
	case 0x78: cbid = 1170; init = som_map_code78; break;
	case 0x90: cbid = 1071; init = som_map_code90; break;

	default: assert(0); return;
	}

	HWND cb = GetDlgItem(dialog,cbid), cbx = som_tool_x(cb);
	
	som_map_codemenuitems &menu = init(0);

	if(menu.empty()) //initialize
	{
		const size_t x_s = 4096; wchar_t x[x_s] = L"";				

		if(GetWindowTextW(cbx,x,x_s)>=x_s-1) assert(0);

		if(*x!='X'||!EX::debug) for(size_t i=0,ii;x[i];)
		{
			for(ii=i;x[i]&&x[i]!='\n';i++); if(x[i]) x[i++] = '\0';

			init(x+ii); 
		}
		init(L""); //finish up
		
		if(fitness)
		{
			size_t fit=0, fit_s=0; //fitness
			while(++fit<menu.size()&&menu[fit].second!='fit')
			if(menu[fit].second&&!fit_s) fit_s = fit;
			if(fit<menu.size())
			{
				menu[fit].second = 0; 
				for(size_t i=0;i<fit_s;i++)	menu[i].first.replace(0,0,L"-");
				menu[fit_s-1].second = fit+1;
			}
		}
	}

	int cblen = 0; if(!fitness)
	/**/cblen = SendMessage(cb,CB_GETCOUNT,0,0);
	if(!cblen) SendMessage(cb,CB_RESETCONTENT,0,0);

	DestroyWindow(cbx); //hack: disable auto translation
		
	for(size_t i=cblen;i<menu.size();i++) switch(menu[i].first[0])	
	{	
	case '-': case '=': continue; 
	
	default: if(-1==menu[i].second) continue; //hidden

		int item = menu[i].second||!fitness? 
		SendMessageW(cb,CB_ADDSTRING,0,(LPARAM)menu[i].first.c_str()):
		SendMessageW(cb,CB_INSERTSTRING,0,(LPARAM)menu[i].first.c_str());
		SendMessage(cb,CB_SETITEMDATA,item,i);
	}

	SetWindowSubclass(dialog,som_map_codecbnproc,MAKELONG(cbid,code),(DWORD_PTR)&menu);
	SendMessage(dialog,WM_COMMAND,MAKEWPARAM(cbid,CBN_SELENDOK),(LPARAM)cb);
}

extern int som_tool_plusminus(int pme)
{
	//NOTE: it might be nice to ignore Shift and use Backspace
	//to input equal but it's a little weird and international
	//keyboards may have different conventions

	HWND f = GetFocus();
	if(DLGC_HASSETSEL&SendMessage(GetFocus(),WM_GETDLGCODE,0,0))
	return 0;
	switch(pme)
	{	
	default: assert(0);
	case '+': case '-': case '=': break; 
	case '_': pme = '-'; //English keyboards
	}
	int id = 0, minus = pme=='-';
	HWND root = GetAncestor(f,GA_ROOT);
	switch(GetWindowContextHelpId(root))
	{
	default: return 0;
	case 40000: //SOM_MAP
	   
		if(f!=SOM_MAP.painting) 
		{
			MessageBeep(-1); return 0;
		}
		if(pme=='=') //English keyboards
		{
			//5: CRLF/surrogate pairs
			wchar_t test[5]; if(!GetDlgItemTextW(root,1000,test,5))
			pme = '+';
		}
		id = pme=='='?1014:1003+minus; 			
		break;

	case 20200: //SOM_EDIT
	case 45300: //SOM_MAP loop editor	

		//todo: inversion extension?
		//id = 1027+minus; break; 
		id = 1028-minus; break; 
	}
	HWND bt = GetDlgItem(root,id);
	//Reminder: not a button, so no BN_CLICKED or lParam
	if(IsWindowEnabled(bt)) SendMessage(root,WM_COMMAND,id,0); 
	return 1;
}

static int som_tool_background(DWORD hlp)
{
	if(0||som_tool_classic) return 0; switch(hlp)
	{	
	case 30000: case 50000: return -1; //black 
	case 10000: return 162;	case 20000: return 142;	
	case 10100: return 164; //200 is better
	case 31100:	return 154;	case 32100:	return 158;
	case 33100:	return 156;	case 34100:	return 157;
	case 35100:	return 155;	case 35200: return 164;
	case 35300: return 163;	case 35400: return 162;
	case 36100: return 136;						   
	case 40000: return 129;	
	case 42000: /*case 42100:*/ return 208; 
	case 51100: return 153;	case 52100: return 155;
	case 53100: return 151;	case 54100: return 156;
	case 55100: return 154;	case 56100: return 157;
	case 56200: return 158;	case 57100: return 152;
	default: if(SOM::tool==SOM_MAP.exe) return 200;
	case 10700: case 30100: case 40100: return 0; 
	}
}
static HBRUSH som_tool_blackground = 0;
extern HBRUSH som_tool_erasebrush(HWND hwnd, HDC dc)
{
	static DWORD dlg = GetClassAtom(som_tool);
	int trans = WS_EX_TRANSPARENT|WS_EX_CONTROLPARENT;
	HWND bg = dlg==GetClassAtom(hwnd)?hwnd:GetParent(hwnd);
	while((GetWindowExStyle(bg)&trans)==trans) bg = GetParent(bg);
	int hlp = GetWindowContextHelpId(bg);
	if(!hlp) return 0;
	int bitmap = som_tool_background(hlp);
	if(bitmap==-1) //SOM_PRM/SYS
	{
		if(!som_tool_blackground) 
		som_tool_blackground = CreateSolidBrush(0);
		return som_tool_blackground; 
	}
	if(!bitmap) switch(SOM::tool) //REMOVE ME?
	{
	default: return 0;
	case SOM_MAIN.exe: case SOM_EDIT.exe: //extended dialogs
		
		switch(hlp)
		{
		default: bitmap = 200; break;
		case 10200:	case 20100: bitmap = 151; break; 
		//loading-like screen
		case 10700: return 0; 
		}
	}
	if(bitmap==164&&SOM::tool==SOM_MAIN.exe)
	{
		static void *found = //new project screen
		FindResource(0,MAKEINTRESOURCE(200),RT_BITMAP);
		if(found) bitmap = 200; //just looks better 
	}	
	if(bitmap==200&&SOM::tool==SOM_EDIT.exe)
	{
		static void *found = //was using 131 before
		FindResource(0,MAKEINTRESOURCE(200),RT_BITMAP);
		if(!found) bitmap = 131;
	}
	if(bitmap==129&&SOM::tool==SOM_MAP.exe) //2023
	{
		extern int SOM_MAP_midminmax[];
		if(SOM_MAP_midminmax[11])
		{
			static void *found = //was using 131 before
			FindResource(0,MAKEINTRESOURCE(130),RT_BITMAP);
			if(found) bitmap = 130;
		}
	}
	HBRUSH *p = 0; //adapated from som_tool_erasebkgnd
	//static HBITMAP bg200 = 0; //originally SOM_MAP's tile
	//HBRUSH &bg200 = som_tool_200; //using //CommCtrls 6.0 transparency 
	//static HBRUSH bg151 = 0; //originally SOM_EDIT's wallpaper
	switch(bitmap)
	{
	default: assert(0); return 0; 
	//case 151: p = &bg151; break; case 200: p = &bg200; break;
	//NEW: these are taken from som_tool_background
	#define CASE(_) case _: { static HBRUSH h = 0; p = &h; break; }
	CASE(162)CASE(142)CASE(154)CASE(158)CASE(156)CASE(157)CASE(155)
	CASE(164)CASE(163)/***162*/CASE(136)CASE(129)CASE(130)CASE(208)CASE(153)
	/***155*//***151*//***156*//***154*//***157*//***158*/CASE(152)
	CASE(131)CASE(151)CASE(200) //these are taken from above
	#undef CASE
	}	
	if(!*p) 
	{
		//TODO: should just get the width/height of som_tool_dialog
		//*p = CreatePatternBrush(LoadBitmap(GetModuleHandle(0),MAKEINTRESOURCE(bitmap)));
		HBITMAP bm; int cx = 0, cy = 0;		
		if(1!=som_tool_enlarge) 		
		if(HRSRC found=FindResource(0,MAKEINTRESOURCE(bitmap),RT_BITMAP))
		{
			HGLOBAL g = LoadResource(0,found);
			float x = som_tool_enlarge2, y = som_tool_enlarge;
			if(hlp==40000) //spotfix
			{
				y*=1-1/926.0f;
			}
			if(hlp==42000) //spotfix
			{
				y*=1-1/829.0f; //x*=1-1/1267.0f;
			}
			if(found) cx = x*((LONG*)g)[1]+0.5f;
			if(found) cy = y*((LONG*)g)[2]+0.5f;			
		}
		bm = (HBITMAP)LoadImage(GetModuleHandle(0),MAKEINTRESOURCE(bitmap),0,cx,cy,0);
		*p = CreatePatternBrush(bm);
	}
	POINT pt = {0,0};
	MapWindowPoints(hwnd,bg,&pt,1);	
	SetBrushOrgEx(dc,-pt.x,-pt.y,0); return *p;
}

extern RECT *SOM_MAIN_divider(HWND);
inline bool som_main_updowns(HWND,RECT&);
extern int som_tool_erasebkgnd(HWND hwnd, HDC dc)
{	
	if(WS_EX_TRANSPARENT&GetWindowExStyle(hwnd)) return 1;
	HBRUSH hb = som_tool_erasebrush(hwnd,dc); if(!hb) return 0;
	RECT *div = SOM::tool==SOM_MAIN.exe?SOM_MAIN_divider(hwnd):0;

	//this is incredibly simple now using the pattern brush
	//SOM_EDIT's bitmap #151 appears to use StretchBlt, but
	//that can't be done with a brush and can only look bad

	static int prm_tab = 0; //blackground	
	RECT cr,rc; if(GetClientRect(hwnd,&cr))
	{		
		int swapped = 0, clipped = 0; 
		static HRGN swap = CreateRectRgn(0,0,1,1);
		if(prm_tab==som_tool_tab)
		{
			swapped = GetClipRgn(dc,swap);
			HWND ch = GetWindow(hwnd,GW_CHILD); 
			for(;ch;ch=GetWindow(ch,GW_HWNDNEXT))
			if(WS_VISIBLE&GetWindowStyle(ch))
			//transparent is mainly for groupboxes/now
			//todo: clip at som_tool_dialogunits.bottom
			if(~GetWindowExStyle(ch)&WS_EX_TRANSPARENT) 
			if(GetWindowRect(ch,&rc)&&MapWindowRect(0,hwnd,&rc))
			if(SOM::tool!=SOM_MAIN.exe||som_main_updowns(ch,rc)) 
			clipped+=ExcludeClipRect(dc,rc.left,rc.top,rc.right,rc.bottom);
		}		
		if(div&&SetBrushOrgEx(dc,div->left,div->top,0)&&FillRect(dc,div,hb))
		clipped+=ExcludeClipRect(dc,div->left,div->top,div->right,div->bottom);
		SetBrushOrgEx(dc,0,0,0)&&FillRect(dc,&cr,hb);
		if(clipped) SelectClipRgn(dc,swapped>0?swap:0);
	}
	//this is so tab loads won't look like swiss cheese
	if(hb!=som_tool_blackground) prm_tab = som_tool_tab;
	return 1; 
}
extern bool som_main_updowns(HWND ch, RECT &rc)
{	
	static DWORD ud = 0;
	if(!ud) ud = som_tool_updown();
	if(GetClassAtom(ch)!=ud) return true; //false;
	extern void SOM_MAIN_updowns(HWND,RECT&);
	SOM_MAIN_updowns(ch,rc); return true;	
}
extern int som_tool_drawitem(DRAWITEMSTRUCT *dis, int bitmap, int st)
{
	if(dis->CtlType!=ODT_BUTTON) return 0;

	som_tool_DrawTextA_item = dis; //NEW: for flexibility
	
	HBITMAP *p = 0; COLORREF c = 0;
	
	switch(SOM::tool) //whites 
	{
	case SOM_MAIN.exe: if(bitmap!=156) c = 0xFFFFFF; break;
	case SOM_EDIT.exe: if(bitmap!=141) c = 0xFFFFFF; break;
	case SOM_PRM.exe:  if(bitmap==132) c = 0xFFFFFF; break;
	case SOM_SYS.exe:  if(bitmap==130) c = 0xFFFFFF; break;
	}
		
	//these are just for looks since HALFTONE bitmaps don't
	//work well with whatever SOM does to setup its buttons
	static HBITMAP bt152 = 0, bt153 = 0, bt154 = 0, bt155 = 0, bt156 = 0;
	static HBITMAP bt132 = 0, bt133 = 0, bt134 = 0, bt135 = 0, bt136 = 0, bt147 = 0;
	//standard buttons not already present in the above set
	static HBITMAP bt141 = 0, bt175 = 0, bt130 = 0, bt131 = 0; 

	switch(bitmap)
	{
	default: assert(0); return 0;
	//SOM_MAIN's big buttons
	case 152: p = &bt152; break; case 154: p = &bt154; break;
	case 153: p = &bt153; break; case 155: p = &bt155; break;
	//SOM_MAIN little button
	case 156: p = &bt156; break;
	//SOM_EDIT's big buttons
	case 132: p = &bt132; break; case 147: p = &bt147; break;
	case 133: p = &bt133; break; case 135: p = &bt135; break;
	case 134: p = &bt134; break; case 136: p = &bt136; break;
	//SOM_EDIT settings editor and SOM_MAP's standard buttons	
	case 141: p = &bt141; break; case 175: p = &bt175; break; 
	//SOM_SYS tabs/buttons
	case 130: p = &bt130; break; case 131: p = &bt131; break;
	}
	if(!*p) /*if(1) //2018: Trying to replicate SOM's buttons
	{
		///UPDATE: According to som_tool_ImageList_Create, this would 
		//work with ILC_COLOR8, but I don't know how SOM_PRM etc preps
		//the bitmap handed to AddMasked. It honors the list's format.

		*p = (HBITMAP)LoadImage(GetModuleHandle(0),
		MAKEINTRESOURCE(bitmap),IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);

		//Does SOM make a 2 image list? From its toggle buttons?
		HIMAGELIST hil = ImageList_Create(160,30,ILC_MASK|ILC_COLORD8,0,1);
		ImageList_AddMasked(hil,*p,0);
		
		HGDIOBJ so = SelectObject(som_tool_dc,*p); 		
		ImageList_Draw(hil,0,som_tool_dc,0,0,ILD_NORMAL|!ILD_BLEND);
		SelectObject(som_tool_dc,so); 

		ImageList_Destroy(hil);
	}
	else*/ *p = LoadBitmap(GetModuleHandle(0),MAKEINTRESOURCE(bitmap));  	
	
	HBITMAP bt = *p; RECT cr = dis->rcItem; BITMAP sz; assert(bt);
	
	struct doublebuffer : SIZE//, HBITMAP //C2516
	{
		HBITMAP bm;	HDC dc;	void size(LONG  x, LONG  y)
		{
			if(cx>=x&&cy>=y) return;
			//this seems like a compiler error
			//if(!dc) dc = CreateCompatibleDC(0);
			cx = x; cy = y; DeleteObject((HGDIOBJ)bm);
			//bm = CreateCompatibleBitmap(dc,cx,cy);
			bm = CreateCompatibleBitmap(som_tool_dc,cx,cy); //TRICKY
			SelectObject(dc,(HGDIOBJ)bm); 		
		}  
		doublebuffer():bm(0),dc(CreateCompatibleDC(0)){ cx = cy = 0; }

	}db; //singleton

	//A: all of this is expected to go through a series of hooks
	if(/*GetClientRect(dis->hwndItem,&cr)&&*/GetObject(bt,sizeof(sz),&sz))
	{
		sz.bmWidth/=2; 
		int pushed = BST_PUSHED&Button_GetState(dis->hwndItem);
		if(st==-1) st = pushed;				
		int tc = dis->itemState&ODS_SELECTED||pushed?0x00FFFF:c; //yellow
		if(tc!=0x00FFFF&&dis->itemState&ODS_FOCUS)			
		if(UISF_HIDEFOCUS&~SendMessage(dis->hwndItem,WM_QUERYUISTATE,0,0))
		tc = 0x00FFFF; 
		
		//NEW: classic flickers but only around the focus??
		//NOTE: could depend on subjectivity / environments
		//reminder: the black ones do not appear to flicker
		//but that has to be an optical effect of some kind
		//(the pushlike checkboxes do flicker if cancelled)
		HDC &dc = GetFocus()==dis->hwndItem?db.dc:dis->hDC; 

		UINT dt = DT_CENTER; 
		/*switch(BS_CENTER&GetWindowStyle(dis->hwndItem))
		{
		case BS_LEFT: dt = 0; break; case BS_RIGHT: dt = DT_RIGHT; break;
		}*/

		SetTextColor(dc,tc);
		SetBkMode(dc,TRANSPARENT);	
		SelectObject(som_tool_dc,(HGDIOBJ)bt); 		
		db.size(cr.right,cr.bottom); 
		int ex;
		if(ex=GetWindowExStyle(dis->hwndItem)&(WS_EX_CLIENTEDGE|WS_EX_DLGMODALFRAME))
		{
			//REMINDER: for SOM_MAP arrow/check overlay
			st = 0;
			int flags = 0;
			if(ex&WS_EX_CLIENTEDGE) flags|=BDR_SUNKENOUTER|BDR_SUNKENINNER;
			if(ex&WS_EX_DLGMODALFRAME) flags|=BDR_RAISEDOUTER|BDR_RAISEDINNER;
			//InvalidateRect(dis->hwndItem,&cr,0);
			//doesn't look like ResEdit/initial drawing
			DrawEdge(dc,&cr,flags,BF_RECT|BF_MIDDLE|BF_SOFT);
		//	ExcludeClipRect(dc,0,0,cr.right,cr.bottom);
			InflateRect(&cr,-3,-3);
		//	InvalidateRect(dis->hwndItem,&cr,0);
		//	InflateRect(&cr,3,3);
			BLENDFUNCTION bf = {0,0,164,0};
			AlphaBlend(dc,cr.left-1,cr.top-1,cr.right-cr.left+1,cr.bottom-cr.top+1,som_tool_dc,st?sz.bmWidth:0,0,sz.bmWidth,sz.bmHeight,bf);
		}
		else StretchBlt(dc,cr.left,cr.top,cr.right-cr.left,cr.bottom-cr.top,som_tool_dc,st?sz.bmWidth:0,0,sz.bmWidth,sz.bmHeight,SRCCOPY);
		if(st) OffsetRect(&cr,1,1); 
		if(&dc==&db.dc) //focused/white
		{			
			SelectObject(db.dc,(HGDIOBJ)GetWindowFont(dis->hwndItem));
			DrawTextA(dc,"??",1,&cr,dt|DT_VCENTER|DT_SINGLELINE);
			if(st) OffsetRect(&cr,-1,-1);
			if(ex) InflateRect(&cr,3,3);
			BitBlt(dis->hDC,0,0,cr.right,cr.bottom,db.dc,0,0,SRCCOPY);
		}
		else DrawTextA(dis->hDC,"??",1,&cr,dt|DT_VCENTER|DT_SINGLELINE);

		som_tool_DrawTextA_item = 0; //NEW
		return 1;
	}
	som_tool_DrawTextA_item = 0; //NEW
	return 0;
}

int SOM::Tool::dragdetect(HWND hw, unsigned wm, unsigned to) 
{	
	int dbl = wm&2;
	int nc = wm<WM_MOUSEFIRST?WM_NCMOUSEMOVE:WM_MOUSEFIRST;
	int vk = wm&1?VK_LBUTTON:VK_RBUTTON;

	//unreliable on Windows 7 and later
	//(no hooks can pre-filter messages?)
	//som_tool_dragdetect_hh = //optimizing
	//SetWindowsHookEx(WH_MOUSE_LL,som_tool_dragdetect_hhproc,EX::module,0);
	int out = 0;
	int drag = GetSystemMetrics(SM_CXDRAG); 			
	POINT anchor,pos; unsigned was = EX::tick();			
	//NOTE: setting up a "message pump" here yielded abyssmal response times
	for(GetCursorPos(&anchor);GetAsyncKeyState(vk)>>15&&GetCursorPos(&pos);) 			
	{
		if(out=pos.x<anchor.x-drag||pos.x>anchor.x+drag) break;
		if(out=pos.y<anchor.y-drag||pos.y>anchor.y+drag) break;
		if(out=EX::tick()-was>to) break;				
	}
	if(dbl&&!out) while(EX::tick()-was<to);
	//illustrating: remove all messages, just in case they got past the hook 
	for(MSG msg;PeekMessageW(&msg,hw,nc,nc+6,PM_REMOVE);)
	{
		switch(msg.message) //NEW
		{
		case WM_LBUTTONDBLCLK: case WM_RBUTTONDBLCLK:
		case WM_NCLBUTTONDBLCLK: case WM_NCRBUTTONDBLCLK:
		
			out = dbl; DispatchMessage(&msg);
		}
	}
	//UnhookWindowsHookEx(som_tool_dragdetect_hh); 
	return out;						
}

//2018: try to color list-box focus in with yellow?
static void som_tool_DrawFocusRect_listbox(HWND lb, HDC dc, const RECT &fr)
{
	//HMM: Turns out LBS_EXTENDEDSEL is a lot of work.
	//OWNERDRAW might be simpler. But it's helps to
	//isolate this code to one location. Note there
	//is overdraw on top of the issues around color
	//and ensuring the text lines up. The XOR usage
	//of DrawFocusRect means this is done both when
	//the stipple is removed via XOR and when drawn.

	int ih = ListBox_GetItemHeight(lb,0);
	int i,ii; RECT ir;
	if(1<ListBox_GetSelCount(lb))
	{
		i = ListBox_GetTopIndex(lb);
		GetClientRect(lb,&ir);
		ii = i+ir.bottom/ih+1;
		int ic = ListBox_GetCount(lb);
		ii = min(ii,ic);
		ListBox_GetItemRect(lb,i,&ir);
	}
	else
	{
		ir = fr; i = ii = 
		SendMessage(lb,LB_ITEMFROMPOINT,0,MAKELPARAM(fr.left,fr.top+3));
	}

	int ci = ListBox_GetCaretIndex(lb);
	int sel = SendMessage(lb,LB_GETANCHORINDEX,0,0);
	int selN = ci;
	int invsel = GetKeyState(VK_CONTROL)>>15?!ListBox_GetSel(lb,sel):0;	
	if(sel>selN) std::swap(sel,selN);
		
	SetBkMode(dc,TRANSPARENT);
	SetTextAlign(dc,TA_BOTTOM);
	int x = ir.left+2, y = ir.bottom;	
	for(;i<=ii;i++,y+=ih) 
	{
		if(1!=ListBox_GetSel(lb,i)) //LB_ERR?
		continue;

		//TODO? System Colors
		//00F2FF looks more yellow (against the blues) but
		//feels less focused (less sharp)
		enum{ yellow=0?0x00F2FF:0x00FFFF, white=0xFFFFFF}; //-1
		int hl; if(i>=sel&&i<=selN)
		{
			//This is an odd black one. But it's OK to skip.
			if(invsel) continue;

			hl = i==ci?yellow:white;
		}
		else hl = white; SetTextColor(dc,hl);
		
		auto &text = som_tool_wector; 
		int len = ListBox_GetTextLen(lb,i);
		if(len<=0) continue;		
		text.resize(ListBox_GetTextLen(lb,i)+1);
		ListBox_GetText(lb,i,&text[0]);					
		
		TextOut(dc,x,y,&text[0],text.size()); //ExtTextOut
	}
}
static BOOL WINAPI som_tool_DrawFocusRect(HDC dc, const RECT *fr)
{
	//TODO: CONFIRM FOCUS BELONGS TO SWORD OF MOONLIGHT
	HWND hw = WindowFromDC(dc);
	ATOM wc = GetClassAtom(hw);

	if(1&&wc==som_tool_listbox()) //EXPERIMENTAL/DICEY
	{
		//2018: try to distinguish listboxes?		
		som_tool_DrawFocusRect_listbox(hw,dc,*fr);		
	}
	else if(0&&wc==som_tool_listview())
	{
		//NOTE: this was a stippled lasso, but something
		//changed, and now it is a blue lasso, so this will
		//not be entered. But it may be in XP/classic mode for
		//Windows 7.
		//assert(0); //LVS_EX_DOUBLEBUFFER?
		
		//2018: trying to do multi-column-multi-select
		if(LVS_EX_FULLROWSELECT&~ListView_GetExtendedListViewStyle(hw))		
		return SOM::Tool.DrawFocusRect(dc,fr);		
	}

	//REMINDER! the purpose of hooking DrawFocusRect is 
	//to not draw the stippled/XOR rectangles. Which is
	//what this does. som_tool_DrawFocusRect_listbox is
	//a 2018 addition
	return 1;
}