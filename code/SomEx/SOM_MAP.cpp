	
#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

#include <functional> //function
#include <set>
#include <hash_map> //Windows XP

#pragma comment(lib,"Msimg32.lib") //AlphaBlend

#include "Ex.ini.h"
#include "Ex.output.h"

#include "dx.ddraw.h" //2021

#include "som.tool.hpp" 
#include "som.932.h"
#include "som.932w.h" 
#include "som.state.h" //SOM::MT
	
#define SOMVECTOR_MATH
#include "../Somplayer/Somvector.h"
#include "../lib/swordofmoonlight.h"
#include "../x2mdl/x2mdl.h"

extern HWND &som_tool;
extern HDC som_tool_dc;
extern HWND som_tool_stack[16+1];
extern HWND som_tool_dialog(HWND=0);
extern HWND som_tool_pushed;
extern int som_tool_initializing;
extern char som_tool_play;
extern int som_map_tab;
extern HWND som_map_tileview; 
extern UINT som_map_tileviewmask; 
extern HWND som_map_tileviewport;
extern HWND som_map_tileviewinsert;
static DWORD SOM_MAP_tileviewtile; //2021
extern HTREEITEM som_map_treeview[4];
extern int SOM_MAP_ini = 0; //2023
//som.tool.cpp->som.tool.hpp
bool SOM_MAP_map::legacy = true;
int SOM_MAP_map::current = 0;
char SOM_MAP_map::versions[64] = {};
static struct SOM_MAP_prt SOM_MAP_prt;
static struct SOM_MAP_map SOM_MAP_map;
static struct SOM_MAP_ezt SOM_MAP_ezt;
extern SOM_MAP_intellisense SOM_MAP(0);
SOM_MAP_intellisense::SOM_MAP_intellisense(int)
:model(),painting(),palette()
,prt(SOM_MAP_prt),map(SOM_MAP_map),ezt(SOM_MAP_ezt),lyt()
,base(),_cs(){}
extern DWORD(&MapComp_memory)[6];
extern wchar_t som_tool_text[MAX_PATH];
extern SHORT som_tool_VK_CONTROL;
				
#ifdef SOM_TOOL_enemies2
std::vector<SOM_MAP_model> SOM_MAP_4921ac::enemies3;
std::vector<SOM_MAP_4921ac::Enemy_NPC> SOM_MAP_4921ac::enemies2;
#endif

extern HWND som_tool_x(HWND);
extern wchar_t *som_tool_nth(wchar_t*,size_t,size_t,wchar_t*);
extern ATOM som_tool_updown(HWND=0);
	
extern bool SOM_MAP_tileview_1047()
{
	if(!IsWindowEnabled(GetDlgItem(som_map_tileview,1047)))
	return true;

	if(GetKeyState(VK_RETURN)>>15)
	{
		SendMessage(som_map_tileview,WM_COMMAND,1047,0);
		return false;
	}
	
	int ok = som_map_tileviewmask&0x2000;
	if(som_map_tileviewmask&0x1000)
	{
		ok = ok?MB_DEFBUTTON1:MB_DEFBUTTON2;
		ok|=MB_YESNOCANCEL|MB_ICONWARNING;
		ok = MessageBoxA //som_tool_MessageBoxA
		(som_map_tileview,(char*)0x48fba0,0,ok);
	}	
	switch(ok)
	{	
	case IDYES: default:
	SendMessage(som_map_tileview,WM_COMMAND,1047,0);
	case IDNO: case 0: return true;
	case IDCANCEL: return false;
	}
}

static void __cdecl SOM_MAP_464a5f(char **A, char *B, DOUBLE C)
{
	assert(B==(char*)0x48FCC0);
	if(sprintf(*A,"%.5f",C)<=0) return; //B is %.1f
	B = *A; while(*B) B++; while(B[-1]=='0'&&B[-2]!='.') *--B = '\0';		
}

static SOM_MAP_4921ac::Start *SOM_MAP_z(HTREEITEM gp, int lp)
{
	int i; for(i=0;i<4;i++) //can i be returned?
	if(gp==som_map_treeview[i]) break; switch(i)
	{
	case 0: return &SOM_MAP_4921ac.enemies[lp]; break;
	case 1: return SOM_MAP_4921ac.NPCs+lp; break;
	case 2: return SOM_MAP_4921ac.objects+lp; break;	
	case 3: return SOM_MAP_4921ac.items+lp; break;
	default: return 0;
	}
}
extern void SOM_MAP_z_index_etc2(HWND hw, SOM_MAP_4921ac::Start *base)
{
	if(HWND tb=GetDlgItem(hw,1005))
	{	
		int zi = base?base->ex_zindex:0;
		int def = SendMessage(tb,TBM_GETRANGEMIN,0,0);
		int pos = 0; if(base)
		{
			pos = zi+def; 
			if(pos>SendMessage(tb,TBM_GETRANGEMAX,0,0))
			SendMessage(tb,TBM_SETRANGEMAX,1,pos);
		}
		SetWindowLong(tb,GWL_USERDATA,(LONG)base);
		//sends SB_ENDSCROLL which looks like clicking to see the tooltip
		//SendMessage(tb,TBM_SETPOSNOTIFY,0,pos);
		SendMessage(tb,TBM_SETPOS,1,pos);SendMessage(hw,WM_COMMAND,1005,0);
	}
}
extern void SOM_MAP_z_index_etc(HTREEITEM gp, int lp)
{
	SOM_MAP_4921ac::Start *base = SOM_MAP_z(gp,lp);
	
	//NEW: event data is populated, but not recorded (yet)
	//TODO? disable layer trackbar also
	EnableWindow(som_map_tileviewinsert,base||!gp?1:0);
	if(!base&&gp) base = SOM_MAP_z(gp,lp);

	struct SOM_MAP_4921ac &s = SOM_MAP_4921ac;

	//if(i<2) //2nd/3rd axis rotation? (EXPERIMENTAL)
	//if(base>=SOM_MAP_4921ac.enemies&&base<SOM_MAP_4921ac.items)
	if(base>=s.NPCs&&base<s.NPCs+s.NPCs_size
	 ||base>=&s.enemies[0]&&base<&s.enemies[s.enemies_size])
	{	
		//HWND ins = som_tool_stack[2];
	//	int ch[] = {1028,1030,1031,1033}; 		
	//	windowsex_enable(som_map_tileviewinsert,ch,0); //som_map_Q_lock?
		//assert(tvi.mask&TVIF_PARAM&&tvi.lParam<128);
		for(int i=0;i<=2;i+=2)
		{
			int div = 5; //som_tool_classic?1:5;
			SetDlgItemInt(som_map_tileviewinsert,1031+i,base->uwv[i],0);
			SendDlgItemMessage(som_map_tileviewinsert,1028+i,TBM_SETPOS,1,base->uwv[i]/div);
		}
	}

	if(!gp||base) //i.e. not event	
	SOM_MAP_z_index_etc2(som_map_tileview,base);
}
extern BYTE SOM_MAP_z_index = 0;
static bool SOM_MAP_z_height2(HWND hw, SOM_MAP_4921ac::Start *base)
{
	if(!base||base->ex_zindex==SOM_MAP_z_index) 
	return false;

	//inverse of SOM_MAP_rec
	float z = base->xzy[1];
	int xy = base->xyindex(); 
	if(SOM_MAP.base)
	z+=SOM_MAP.base[xy].z;
	z-=SOM_MAP_4921ac.grid[xy].z;
	char c[32], *C = c;
	SOM_MAP_464a5f(&C,(char*)0x48FCC0,z);
	SOM::Tool.SetWindowTextA(hw,C); return true;
}
extern bool SOM_MAP_z_height(HWND hw/*, LPSTR C*/)
{
	//if(!SOM_MAP.base) return;

	//THERE'S GOT TO BE A BETTER WAY?
	HWND tv = GetDlgItem(som_map_tileview,1052);
	HTREEITEM gp; TVITEM tvi = {TVIF_PARAM};
	gp = TreeView_GetSelection(tv);
	SOM_MAP_4921ac::Start *base = 0;	
	while(gp=TreeView_GetParent(tv,tvi.hItem=gp))
	{
		TreeView_GetItem(tv,&tvi);
		if(base=SOM_MAP_z(gp,tvi.lParam)) 
		break;
	}
	return SOM_MAP_z_height2(hw,base);
}
extern bool SOM_MAP_z_scroll(WPARAM id)
{	
	HWND z = GetDlgItem(som_map_tileview,1010);
	HWND tb = GetDlgItem(som_map_tileview,1005);
	BYTE *base = (BYTE*)GetWindowLong(tb,GWL_USERDATA);
	int pos = SendMessage(tb,TBM_GETPOS,0,0);
	int def = SendMessage(tb,TBM_GETRANGEMIN,0,0);
		
	bool beep = false;

	if(id==1005) //WM_VSCROLL
	{
		Button_SetCheck(z,0!=pos);		
	}
	else 
	{		
		assert(id==1010); //z
	
		int cmp = pos;
		if(base&&Button_GetCheck(z))
		{
			//not disabling the Z button
			int cmp = def+base[3]; //ex_original_zindex			
		}
		else cmp = 0;			

		beep = pos==cmp&&!pos; pos = cmp;
		SendMessage(tb,TBM_SETPOS,1,pos);
	}	

	windowsex_enable<1008>(som_map_tileview,0==pos);			
	//not disabling for Ctrl+Up/Down access to slider
	//EnableWindow(z,base&&(pos-def!=*base||base[0]!=base[3]));		
	if(beep)
	{
		MessageBeep(-1); 
		Button_SetCheck(z,0);
		extern void som_tool_highlight(HWND,int);
		som_tool_highlight(z,0);
	}
	return !beep;
}

//PLAY button. this is based on SOM_MAIN_updownproc
static LRESULT CALLBACK SOM_MAP_updownproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scID, DWORD_PTR)
{
	switch(uMsg)
	{	
	case WM_ERASEBKGND:

		return 1; //helping?

	case WM_PAINT: 
		
		//Surprised the compiler is okay with this :(
		goto paint;

	case WM_LBUTTONUP:
	case WM_LBUTTONDOWN: 
	{	
		HWND gp = GetParent(hWnd);
		int id = GetDlgCtrlID(hWnd), hl = 0; 			
		int hlp = GetWindowContextHelpId(gp);
		if(uMsg==WM_LBUTTONDOWN) 
		{
			switch(id+hlp)
			{
			case 1080+40000: //play

				hl = 1074; break;

			case 1082+40000: //redo

				if(!IsWindowEnabled(GetDlgItem(gp,1225)))
				MessageBeep(-1);

				hl = 1225; break;
			}			
		}	
		paint: if(UDS_HORZ&GetWindowStyle(hWnd))
		{
			RECT cr; if(GetClientRect(hWnd,&cr))
			{
				//give theme pack some control
				bool right = UDS_ALIGNRIGHT&GetWindowStyle(hWnd);

				LONG split = (cr.right-cr.left)/2;
				if(right==(uMsg==WM_PAINT)) //accuracy 
				cr.left = cr.right-split; else cr.right-=split;

				switch(uMsg) //cut the updown in half
				{
				case WM_PAINT: //hack: could clip instead
					
					ValidateRect(hWnd,&cr); break;

				case WM_LBUTTONUP: 
				
					InflateRect(&cr,3,3); //whatever works...

				case WM_LBUTTONDOWN:

					POINT pt = {GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};					

					if(!PtInRect(&cr,pt)) 
					{				
						//prevent clicks
						//MessageBeep: SOM_MAP_buttonproc
						if(uMsg&1) return MessageBeep(-1); 
						//prevent release
						lParam = cr.right*4; 
					}					
					break; 
				}
			}
		}
		if(uMsg==WM_LBUTTONDOWN)		
		{
			extern void som_tool_highlight(HWND,int);
			if(hl) som_tool_highlight(gp,hl);
		}
		if(uMsg==WM_LBUTTONUP&&hWnd==GetCapture()) 
		{	
			RECT cr; GetClientRect(hWnd,&cr); 
			POINT pt = {GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
			InflateRect(&cr,3,3); //whatever works
			if(!PtInRect(&cr,pt)) 
			break;
			else switch(id+hlp)
			{
			case 1080+40000: //play

				SendMessage(gp,WM_COMMAND,id,0);
				break; 
			}
		}
		break;
	}
	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,SOM_MAP_updownproc,scID);
		break;
	}					  	
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}     
static VOID CALLBACK SOM_MAP_control(HWND win, UINT, UINT_PTR id, DWORD)
{
	if(~GetKeyState(id)>>15) KillTimer(win,id);	else return;
	
	//send back thru SOM_MAP_buttonproc to pick which side to push
	if(GetKeyState(VK_CONTROL)>>15) SendMessage(win,WM_KEYUP,id,0);
}
static LRESULT CALLBACK SOM_MAP_buttonproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scID, DWORD_PTR)
{
	static WPARAM repeat = 0;

	switch(uMsg) 
	{
	case WM_KEYUP: if(repeat!=wParam) break; //double beeps
	case WM_KEYDOWN: 			
	if(GetKeyState(VK_CONTROL)>>15
	&&wParam!=VK_CONTROL&&wParam!=VK_SHIFT
	&&!iswalpha(wParam)&&!iswdigit(wParam)) //&Mneumonics
	{				
		if(repeat!=wParam) 
		if(!GetCapture()) repeat = wParam; //driving ticker
		else break; 		

		//VK_LBUTTON: som_tool_buttonproc masking VK_SPACE
		bool space = wParam==VK_SPACE||wParam==VK_LBUTTON;
		//bool u = wParam==VK_UP, d = wParam==VK_DOWN;
		bool r = wParam==VK_RIGHT, l = wParam==VK_LEFT;

		HWND gp = GetParent(hWnd);  
		int ud = 0, hid = GetWindowContextHelpId(gp);		
		switch(GetWindowID(hWnd)+hid)
		{ 
		case 1074+40000: //split-button ticker
		case 1071+40000:
					
			if(r||l||space) ud = 1080; break; //play

		case 1225+40000: //2024
		case 1046+40000:

			if(r||l||space) ud = 1082; break; //redo
		}		
				
		HWND tick = ud?GetDlgItem(gp,ud):0;		
		if(!IsWindowEnabled(tick)) 
		tick = 0;		
		if(!tick)
		{
			repeat = 0;	return MessageBeep(-1);
		}
		if(space||l) r = UDS_ALIGNRIGHT&~GetWindowStyle(tick);
		LPARAM lp = 0; if(r/*||d*/)
		{
			RECT cr = {0,0,0,0}; GetClientRect(tick,&cr);

			lp = r?cr.right/2+4:(cr.bottom/2+4)<<16;
		}
		else /*if(!l&&!u)*/ return MessageBeep(-1);
		uMsg = uMsg==WM_KEYDOWN?WM_LBUTTONDOWN:WM_LBUTTONUP;		
		SendMessage(tick,uMsg,0,lp);
		if(uMsg==WM_KEYDOWN)
		SetTimer(hWnd,wParam,100,SOM_MAP_control);
		return 0;
	}
	else break;
	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,SOM_MAP_buttonproc,scID);
		break;
	}					  	
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
extern void SOM_MAP_buttonup()
{
	HWND &dlg = som_tool; //40000 (102)

	//Tie horizontal updowns to their buttons.
	HWND play = GetDlgItem(dlg,1080);
	if(GetClassAtom(play)==som_tool_updown())
	{
		SetWindowSubclass(play,SOM_MAP_updownproc,0,0);
		SetWindowSubclass(GetDlgItem(dlg,1071),SOM_MAP_buttonproc,0,0);
		SetWindowSubclass(GetDlgItem(dlg,1074),SOM_MAP_buttonproc,0,0);
	}
	HWND redo = GetDlgItem(dlg,1082); //2024
	if(GetClassAtom(redo)==som_tool_updown())
	{
		SetWindowSubclass(redo,SOM_MAP_updownproc,0,0);
		SetWindowSubclass(GetDlgItem(dlg,1225),SOM_MAP_buttonproc,0,0);
		SetWindowSubclass(GetDlgItem(dlg,1046),SOM_MAP_buttonproc,0,0);
	}

	//UNRELATED // EXPERIMENTAL
		
	RECT rc; GetWindowRect(GetDlgItem(dlg,1011),&rc);
	MapWindowRect(0,som_tool,&rc);
	int cx = GetSystemMetrics(SM_CXVSCROLL);
	int cy = GetSystemMetrics(SM_CYHSCROLL);
	//2017: 3000 is set aside for the overlay/layers tray.
	HWND overlay = GetDlgItem(dlg,3000); if(overlay)
	{
		SetWindowPos(overlay,0,rc.right-cx-1,rc.bottom-cy-1,cx,cy+1,SWP_NOZORDER);		
		EnableWindow(overlay,1);
	}
	HWND opacity = GetDlgItem(dlg,3001); if(opacity)
	{
		SendMessage(opacity,TBM_SETRANGE,1,MAKELONG(-127,128));		
		EnableWindow(opacity,1);
	}
}
extern int SOM_MAP_midminmax[1+2+2+6+1] = {-1}; //12
extern void SOM_MAP_40000()
{
	SOM_MAP_buttonup(); //Play?
		
	//historically there's a gap in the palette
	//the box art doesn't show the gap. and it's just taking up space
	RECT cr; GetClientRect(SOM_MAP.palette,&cr);
	int w = cr.right/21;
	auto vp = (DWORD*)SOM_MAP_app::CWnd(SOM_MAP.palette);
	//apparently both must be the same. horizontal scroll?
	//0042FABD 8B 4D 5C             mov         ecx,dword ptr [ebp+5Ch]
	vp[0x54/4] = vp[0x5C/4] = w; //9;
	int h = SOM_MAP_prt.missing; //SOM_MAP_prt.blob.size();
	if(h%w==0) h--; h/=w;

	h++; //SOM_MAP_expand_icons? 

	SetScrollRange(SOM_MAP.palette,1,0,h,0);

	windowsex_enable<1010>(som_tool); //PrtsEdit
	windowsex_enable<1133>(som_tool); //layers menu

	EX::INI::Editor ed;
	if(float inc=ed->tile_elevation_increment)
	{
		wchar_t w[32]; swprintf(w,L"%g",inc);
		SetDlgItemText(som_tool,999,w);
	}

	if(SOM_MAP_ini&2) SendDlgItemMessage(som_tool,1263,BM_SETCHECK,1,0);
	if(SOM_MAP_ini&4) SendDlgItemMessage(som_tool,1264,BM_SETCHECK,1,0);

	extern void SOM_MAP_load_overlay(wchar_t*,bool);
	{
		wchar_t w[MAX_PATH+2];
		if(!GetPrivateProfileString(L"editor",L"mapOverlay",L"",w,MAX_PATH+2,SOM::Game::title('.ini')))
		w[0] = '\0';
		SOM_MAP_load_overlay(w,false);
	}

	//SOM_MAP_midminmax?
	{					
		auto *p = SOM_MAP_app::CWnd(som_tool);
		SendDlgItemMessage(som_tool,1224,BM_SETCHECK,SOM_MAP_midminmax[11],0);
		switch(som_map_tab)
		{
		case 1215: ((int(__thiscall*)(void*))0x41abe0)(p); break;
		case 1216: ((int(__thiscall*)(void*))0x41acc0)(p); break;
		case 1220: ((int(__thiscall*)(void*))0x41ad90)(p); break;
		}
	}
}
extern void SOM_MAP_42000_44000(HWND hw)
{	
	if(hw==som_map_tileview) //HACK (42000)
	{
		//2021: patching old button text?
		wchar_t patch[8];
		if(GetDlgItemTextW(hw,1261,patch,8))
		if(0x006e0069006c0026ull==*(QWORD*)patch) //&lines
		{
			SetDlgItemTextW(hw,1261,L"&neighbors");
			SetDlgItemTextW(hw,1262,L"&layers");
		}
		else if(0x000030f330a430e9ull==*(QWORD*)patch) //Ra-I-N
		{
			SetDlgItemTextW(hw,1261,som_932w_tile_view_patch[0]);
			SetDlgItemTextW(hw,1262,som_932w_tile_view_patch[1]);
		}

		if(~som_map_tileviewmask&1) //2021
		PostMessage(GetDlgItem(hw,1257),BM_CLICK,0,0);

		for(int i=1;i<8;i++)
		if(som_map_tileviewmask&1<<i) //BM_CLICK
		CheckDlgButton(hw,1257+i,1);

		windowsex_enable(hw,1263,1267); //line/rec		
		//2022: disable (new) subdivision function for MHM mode
		//since MHM can't be subdivided (it's for MHM)
		if(som_map_tileviewmask&0x100) windowsex_enable<1264>(hw,0);

		CheckDlgButton(hw,1267,som_map_tileviewmask&0x1000?1:0);
		CheckRadioButton(hw,1265,1266,0x2000&som_map_tileviewmask?1265:1266);

		windowsex_enable<1010>(hw); //Z
	}

	if(HWND tb=GetDlgItem(hw,1005))		
	{
		//limiting to 7 so it is possible
		//to convert object-type IDs into
		//0
		int def = 0, top = 7; //0 //pretty	
		if(HWND cb=GetDlgItem(som_tool,1133))
		{
			int base = ComboBox_GetItemData(cb,0);
			if(base>=0&&base<64)
			if(som_LYT**lyt=SOM_MAP.lyt[base])
			{
				def = ComboBox_GetCurSel(cb);
				if(def>0) def = lyt[def-1]->group;		
			//	for(int i=0;lyt[i];i++) 
			//	top = max(top,(int)lyt[i]->group);
			}
			else assert(0);
			else assert(base==-1); //CB_ERR?
		}
		EnableWindow(tb,1);		
		SendMessage(tb,TBM_SETRANGE,1,MAKELPARAM(-def,top-def));
	}	

	if(hw!=som_map_tileview) //HACK (44000)
	{
		HWND z = GetDlgItem(hw,1030);
		Edit_LimitText(z,0);

		//there is a z-index in the file. may
		//as well use it
		//SOM_MAP_z_index_etc2(hw,(int)GetDlgItemInt(hw,1032,0,1)/360);
		SOM_MAP_z_index_etc2(hw,&SOM_MAP_4921ac.start);		
		SOM_MAP_z_height2(z,&SOM_MAP_4921ac.start);		
		//update slider		
		SendMessage(hw,WM_COMMAND,MAKEWPARAM(1030,EN_KILLFOCUS),(LPARAM)z);

			//TODO: WOULD LIKE TO NOT LIMIT FOG TO 0.1, 0.2, ETC.

		//HACK: fog this is set wrong first time it's opened, it must
		//be a rounding issue, because it's reproduced here. the MSVC
		//code highlight tool doesn't round down but the code does so
		//NOTE: wcstod converts 0.9 to 0.8999997 or something like it
		float fog = GetDlgItemFloat(hw,1077);
		SendDlgItemMessage(hw,1145,TBM_SETPOS,1,fog*10+0.5f); //fog*10
	}
}

extern int SOM_MAP_alpha[2] = {1,1};
extern int SOM_MAP_overlay_size = 0;
extern HBITMAP SOM_MAP_overlay_BMP = 0;
extern BYTE *SOM_MAP_overlay_ambient2 = 0;
extern void SOM_MAP_sandwich_overlay(HDC dc)
{
	HBITMAP &bmp = SOM_MAP_overlay_BMP;
	if(0==bmp) return;
	
	int alpha = SOM_MAP_alpha[1];
	if(alpha<=0) return;
	alpha = min(alpha*2,255);

	BITMAP sz; GetObject(bmp,sizeof(sz),&sz);
	int scale = sz.bmHeight/100;

	RECT cr; GetClientRect(SOM_MAP.painting,&cr);
	int w = (cr.right+20)/21;
	int h = (cr.bottom+20)/21;
	int x = GetScrollPos(SOM_MAP.painting,SB_HORZ);
	int y = GetScrollPos(SOM_MAP.painting,SB_VERT);

	//assuming 100x100
	//AlphaBlend errors if out-of-bounds
	w+=min(0,100-(x+w)); h+=min(0,100-(y+h));
	cr.right = w*21; cr.bottom = h*21;

	x*=scale; y*=scale; w*=scale; h*=scale;
					
	BLENDFUNCTION bf = {0,0,alpha,0};
	HGDIOBJ so = SelectObject(som_tool_dc,bmp);
	AlphaBlend(dc,0,0,cr.right,cr.bottom,som_tool_dc,x,y,w,h,bf);
	SelectObject(som_tool_dc,so);
}
extern void SOM_MAP_update_illumination()
{
	if(SOM_MAP_overlay_BMP)
	{
		char *cw = (char*)SOM_MAP_app::CWnd(SOM_MAP.painting);
		int x = *(int*)(cw+0x78);
		int y = *(int*)(cw+0x7c);

		BYTE *pp = SOM_MAP_overlay_ambient2;

		if(!pp||SOM_MAP_alpha[1]>0)
		{
			x*=SOM_MAP_overlay_size/100;
			y*=SOM_MAP_overlay_size/100;
			auto so = SelectObject(som_tool_dc,SOM_MAP_overlay_BMP);
			COLORREF cr = GetPixel(som_tool_dc,x,y);
			SelectObject(som_tool_dc,so);
			SetDlgItemInt(som_tool,1030,cr&0xff,0);
			SetDlgItemInt(som_tool,1031,cr>>8&0xff,0);
			SetDlgItemInt(som_tool,1032,cr>>16&0xff,0);
		}
		else
		{
			BYTE *pp = SOM_MAP_overlay_ambient2;		
			y = 99-y;
			pp+=3*(y*100+x); 
			SetDlgItemInt(som_tool,1030,pp[0],0);
			SetDlgItemInt(som_tool,1031,pp[1],0);
			SetDlgItemInt(som_tool,1032,pp[2],0);
		}
	}
	else
	{
		SetDlgItemText(som_tool,1030,L"");
		SetDlgItemText(som_tool,1031,L"");
		SetDlgItemText(som_tool,1032,L"");
	}
}
extern void SOM_MAP_load_overlay(wchar_t *path_filter, bool write_ini=true)
{
	if(SOM_MAP_overlay_BMP) DeleteObject(SOM_MAP_overlay_BMP); 
	
	SOM_MAP_overlay_BMP = 0; SOM_MAP_overlay_size = 0;

	if(write_ini) WritePrivateProfileString
	(L"editor",L"mapOverlay",path_filter,SOM::Game::title('.ini'));

	if(path_filter)
	{
		auto sep = wcschr(path_filter,'?');
		int size = 300; if(sep)
		{
			*sep = '\0';

			switch(_wtoi(path_filter)) //open.nFilterIndex
			{
			//REMINDER: filterIndex is 1-based
			case 2: size = 700; break; case 3: size = 2100; break;
			}
		}
		else sep = path_filter-1;

		if(SOM_MAP_overlay_BMP=(HBITMAP)LoadImageW(0,sep+1,0,size,size,LR_LOADFROMFILE))
		SOM_MAP_overlay_size = size;
	}

	SendDlgItemMessage(som_tool,3001,TBM_SETPOS,1,SOM_MAP_alpha[SOM_MAP_overlay_BMP?1:0]);
			
	SOM_MAP_update_illumination();

	InvalidateRect(SOM_MAP.painting,0,0);
}
extern void SOM_MAP_open_overlay()
{
	static wchar_t bmp[MAX_PATH] = L"";		
	extern UINT_PTR CALLBACK som_tool_openhook(HWND,UINT,WPARAM,LPARAM);
	OPENFILENAMEW open = 
	{
		sizeof(open),som_tool,0,
		L"300 x 300 Bitmap (*.bmp)\0*.bmp\0"
		L"700 x 700\0*.bmp\0"
		L"2100 x 2100\0*.bmp\0",	
		0, 0, 0, bmp, MAX_PATH, 0, 0, 0, 0, 
		//this hook is just to change the look
		OFN_ENABLEHOOK|OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_FILEMUSTEXIST,
		0, 0, 0, 0, som_tool_openhook, 0, 0, 0
	};
	
	if(GetOpenFileNameW(&open)) 
	{		
		wchar_t w[MAX_PATH+2]; swprintf(w,L"%d?%s",open.nFilterIndex,bmp);

		if(SOM_MAP_alpha[1]<=16) SOM_MAP_alpha[1] = 64;

		SOM_MAP_load_overlay(w);

		//???: the whole tool goes haywire and the slide disappears after
		//the GetOpenFileName dialog
		//InvalidateRect(som_tool,0,0);
	}
	else SOM_MAP_load_overlay(0);
}

//experiment to Print Screen the 2100x2100 map
/*doesn't work. might try Detours on BeginPaint?
extern void SOM_MAP_print(HWND hw)
{		
	//hw = GetDlgItem(som_tool,0x3e8); //checks out

	RECT cr; GetClientRect(hw,&cr);
	HBITMAP bmp = CreateCompatibleBitmap(som_tool_dc,cr.right,cr.bottom);
	HGDIOBJ so = SelectObject(som_tool_dc,bmp);
	SendMessageA(hw,WM_PRINTCLIENT,(WPARAM)som_tool_dc,PRF_CLIENT);
	SelectObject(som_tool_dc,so);

	OpenClipboard(0);
	EmptyClipboard();
	SetClipboardData(CF_BITMAP,bmp);
	CloseClipboard();
}*/

static int SOM_MAP_colorred(HWND hw, int hlp, int id=0)
{
	int r = 0; 
	
	switch(hlp+(id?id:GetDlgCtrlID(hw))) 
	{		
	case 43000+1080: r = 1085; break; //200: light
	case 43000+1081: 
	case 43000+1082: 
	case 43000+1083: r = 10*(id-1081)+1093; break; //200
	
	case 44000+1084: r = 1078; break; //153: fog

	case 46000+100+1080: //137: font

	case 42400+1080: r = 1039; break; //159: lamp
	}
	return r;
}
static int SOM_MAP_colorgreen(int r)
{
	return r+(r==1078||r==1039?1:2);
}
static int SOM_MAP_colorblue(int r)
{
	return r+(r==1039?2:4);
}
static COLORREF SOM_MAP_colorref(HWND hWnd, int r)
{
	if(r>=10000) r = SOM_MAP_colorred(hWnd,r);
	HWND gp = GetParent(hWnd);
	return GetDlgItemInt(gp,r,0,0)
	|GetDlgItemInt(gp,SOM_MAP_colorgreen(r),0,0)<<8
	|GetDlgItemInt(gp,SOM_MAP_colorblue(r),0,0)<<16;
}
extern LRESULT CALLBACK SOM_MAP_colorproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR hlp)
{	
	switch(uMsg)
	{			
	case WM_SETCURSOR: 
	{
		RECT fr; Edit_GetRect(hWnd,&fr);
		POINT pt; GetCursorPos(&pt); MapWindowPoints(0,hWnd,&pt,1);
		SetCursor(LoadCursor(0,PtInRect(&fr,pt)?IDC_IBEAM:IDC_ARROW));
		return 0;
	}
	case WM_LBUTTONDOWN:
	
		if(GetCursor()==LoadCursor(0,IDC_ARROW))
		return 0;
		break;

	case WM_LBUTTONUP:
	
		if(GetCursor()==LoadCursor(0,IDC_ARROW))
		goto choosecolor;
		break;

	case WM_KEYDOWN:

		//Reminder: some dialogs have an OK button.
		if(wParam==VK_SPACE/*||wParam==VK_RETURN*/) choosecolor:
		{
			//TODO? Extract the HLS value so luminance isn't destructive.
																	
			wParam = SOM_MAP_colorred(hWnd,hlp,id);			
			lParam = SOM_MAP_colorref(hWnd,wParam);
			extern BOOL workshop_choosecolor(HWND,COLORREF*);
			if(workshop_choosecolor(hWnd,(COLORREF*)&lParam))
			{
				HWND gp = GetParent(hWnd);
				EnableWindow(hWnd,0); //HACK#1 Lock it...
				SetDlgItemInt(gp,wParam,lParam&0xFF,0);
				SetDlgItemInt(gp,SOM_MAP_colorgreen(wParam),lParam>>8&0xFF,0);
				EnableWindow(hWnd,1); //HACK#2 EN_CHANGE...
				SetDlgItemInt(gp,SOM_MAP_colorblue(wParam),lParam>>16&0xFF,0);				
			}
			return 0;
		}
		break;
	
		//WTH??? sending WM_MOUSEWHEEL to the updown at high rates occassionally 
		//pulses a dialog wide repaint on Windows 10. It happens with wheels too
		//(It causes slight flickering elsewhere)
		//REMINDER: Seems to be limited to the transparent groupbox beneath this
	case WM_SETFOCUS:		
		SetWindowLong(hWnd,GWL_EXSTYLE,~WS_EX_TRANSPARENT&GetWindowExStyle(hWnd));
		break;
	case WM_KILLFOCUS:
		if(hlp==43000||hlp==42400) //assuming lights are tranparent
		SetWindowLong(hWnd,GWL_EXSTYLE,WS_EX_TRANSPARENT|GetWindowExStyle(hWnd));
		break;

	case WM_ENABLE:

		//HACK: using EnableWindow as a makeshift flag to prevent a change from 
		//setting off a back-and-forth chain reaction (ie: RGB->HLS->RGB->...)
		return 0; 

	case WM_TIMER: //throttling WM_ERASEBKGND
	{	
		enum{ refresh=33 };
		static DWORD throttle = GetTickCount()-refresh;
		KillTimer(hWnd,wParam);
		if(GetTickCount()-throttle>=refresh) 
		{
			throttle = GetTickCount(); InvalidateRect(hWnd,0,1);
		}
		else SetTimer(hWnd,wParam,refresh,0);
		return 0;
	}
	case WM_ERASEBKGND:
	{
		if(GetFocus()==hWnd
		||~GetWindowExStyle(hWnd)&WS_EX_TRANSPARENT) 
		break;
		extern HBRUSH som_tool_erasebrush(HWND,HDC);
		DWORD hs = GetWindowLong(hWnd,GWL_USERDATA);		
		HBITMAP bm = CreateCompatibleBitmap((HDC)wParam,1,1);			
		HGDIOBJ so = SelectObject(som_tool_dc,bm);				
		SetPixel(som_tool_dc,0,0,HIWORD(hs)==0?0x808080:ColorHLSToRGB(LOWORD(hs),120,HIWORD(hs)));
		RECT cr; GetClientRect(hWnd,&cr);
		BLENDFUNCTION bf = {0,0,127,0};
		FillRect((HDC)wParam,&cr,som_tool_erasebrush(hWnd,(HDC)wParam));
		AlphaBlend((HDC)wParam,0,0,cr.right,cr.bottom,som_tool_dc,0,0,1,1,bf);
		SelectObject(som_tool_dc,so);
		DeleteObject((HGDIOBJ)bm);
		return 1;
	}
	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,SOM_MAP_colorproc,id);
		break;
	}

	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
extern LRESULT CALLBACK SOM_MAP_colorfulproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR hlp)
{
	switch(uMsg)
	{	
	case WM_CTLCOLOREDIT: //SOM_MAP_colorproc?
	{
		int id = GetDlgCtrlID((HWND)lParam);
		int r = SOM_MAP_colorred((HWND)lParam,hlp,id); if(r)
		{
			SetBkMode((HDC)wParam,TRANSPARENT);
			COLORREF bg = SOM_MAP_colorref((HWND)lParam,r);
			//if(0==(0x808080&bg)) SetTextColor((HDC)wParam,0xFFFFFF);
			if(GetDlgItemInt(hWnd,id,0,0)<100) SetTextColor((HDC)wParam,0xFFFFFF);
			static LRESULT hb = 0; DeleteObject((HGDIOBJ)hb);
			return hb = (LRESULT)CreateSolidBrush(bg);
		}
		break;
	}
	case WM_COMMAND:

		switch(HIWORD(wParam))
		{
		case EN_CHANGE: //Reminder: Even arrow keys trigger EN_CHANGE.

			int r = 0, box = 0;
			switch(hlp+LOWORD(wParam))
			{
			case 43000+1080:
			case 43000+1085: case 43000+1087: case 43000+1089:

				r = 1085; box = 1080; break;

			case 43000+1081:
			case 43000+1093: case 43000+1095: case 43000+1097:

				r = 1093; box = 1081; break;

			case 43000+1082:
			case 43000+1103: case 43000+1105: case 43000+1107:

				r = 1103; box = 1082; break;

			case 43000+1083:
			case 43000+1113: case 43000+1115: case 43000+1117:

				r = 1113; box = 1083; break;
								
			case 44000+1084:
			case 44000+1078: case 44000+1079: case 44000+1082:
				
				r = 1078; box = 1084; break;

			case 46100+1080: //+100
			case 46100+1039: case 46100+1040: case 46100+1041: 
			case 42400+1080: 
			case 42400+1039: case 42400+1040: case 42400+1041: 

				r = 1039; box = 1080; break;
			}
			if(r==0) break;
			
			HWND bh = LOWORD(wParam)==box?(HWND)lParam:GetDlgItem(hWnd,box);
			if(!IsWindowEnabled(bh)) 
			break;
						
			HWND gf = GetFocus(); //EnableWindow(0) changes the focus

			EnableWindow(bh,0); //HACK#1
			{
				if(LOWORD(wParam)==box) //driving the RGB boxes?
				{	
					DWORD hs = GetWindowLong(bh,GWL_USERDATA);
					COLORREF rgb = ColorHLSToRGB(LOWORD(hs),GetDlgItemInt(hWnd,box,0,0),HIWORD(hs));
					SetDlgItemInt(hWnd,r,rgb&0xFF,0);
					SetDlgItemInt(hWnd,SOM_MAP_colorgreen(r),rgb>>8&0xFF,0);
					SetDlgItemInt(hWnd,SOM_MAP_colorblue(r),rgb>>16&0xFF,0);
					if(1039==r) //SOM updates sliders when changing controls.
					{
						WPARAM en = MAKEWPARAM(LOWORD(r),EN_KILLFOCUS);
						SendMessage(hWnd,uMsg,en,(LPARAM)GetDlgItem(hWnd,LOWORD(en)));
						en = MAKEWPARAM(LOWORD(SOM_MAP_colorgreen(r)),EN_KILLFOCUS);
						SendMessage(hWnd,uMsg,en,(LPARAM)GetDlgItem(hWnd,LOWORD(en)));
						en = MAKEWPARAM(SOM_MAP_colorblue(r),EN_KILLFOCUS);
						SendMessage(hWnd,uMsg,en,(LPARAM)GetDlgItem(hWnd,LOWORD(en)));
					}
				}
				else //driving the new color boxes?
				{
					WORD hls[3];		
					ColorRGBToHLS(SOM_MAP_colorref(bh,r),hls,hls+1,hls+2);		
					SetWindowLong(bh,GWL_USERDATA,(int)hls[2]<<16|hls[0]);						
					SetDlgItemInt(hWnd,box,hls[1],0);					
				}
			}
			EnableWindow(bh,1); //HACK#2

			if(LOWORD(wParam)!=box||gf!=bh)
			{
				SendMessage(bh,WM_TIMER,r,0); //InvalidateRect(bh,0,1);
			}

			if(gf==bh) SetFocus(bh); //HACK: restore focus

			break;
		}
		break;

	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,SOM_MAP_colorfulproc,id);
		break;
	}

	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
 
extern int som_LYT_base(int);
extern char SOM_MAP_layer(WCHAR w[]=0, DWORD wN=0)
{
	WCHAR def[4]; if(!w){ w = def; wN = 4; };
	if(2!=GetEnvironmentVariableW(L".MAP",w,wN)
	||!isdigit(w[0])||!isdigit(w[1]))
	{
		*w = '\0';
		assert(0); return -1; //this constraint can easily be relaxed
	}
	int i = _wtoi(w); return i<64?i&0xFF:-1;
}
extern int SOM_MAP_201_open(int st=1)
{
	int map = 0; if(st) 
	{
		SendDlgItemMessage(som_tool_stack[1],1239,LB_GETSELITEMS,1,(LPARAM)&map);
	}
	else //2023
	{
		map = SendDlgItemMessage(som_tool_stack[0],1133,CB_GETCURSEL,0,0);
		if(map==-1) 
		{
			map = _wtoi(Sompaste->get(L".MAP")); //YUCK			
		}
		else map = SendDlgItemMessage(som_tool_stack[0],1133,CB_GETITEMDATA,map,0);
	}
	WCHAR _map2[32];
	swprintf(_map2,L"%02d",map);
	SetEnvironmentVariableW(L".MAP",_map2); 

	//2021: woops!! cleanup code
	{
		//unrelated, base wasn't being deleted
		//on new (blank) map creation, and the
		//enemies over 128 still lingered on a
		//new map (blank or not)

		/*2021
		//REMINDER: this was new char allocated? 
		delete[] SOM_MAP.base; SOM_MAP.base = 0;*/
		for(int i=EX_ARRAYSIZEOF(SOM_MAP.layers);i-->0;)
		delete[] SOM_MAP.layers[i];
		memset(SOM_MAP.layers,0x00,sizeof(SOM_MAP.layers));

		//NOTE: 40ec10 does teardown but it's a
		//thiscall so it's easier to do this way
		for(DWORD i=128;i<SOM_MAP_4921ac.enemies_size;i++)
		SOM_MAP_4921ac.enemies2[i].props = 0xFFFF;

		//duplicating som_tool_CreateFileA's code?
		{
			//NOTE: I can't remember if these are 
			//relevant outside read/write context
			assert(!SOM_MAP.map);
			SOM_MAP.map.current = map; 
			SOM_MAP.map.versions[map] = 0; //13
		}
	}

	return map;
}
extern void SOM_MAP_201_play()
{
	WCHAR backup[4];
	if(-1==SOM_MAP_layer(backup,4))
	return;

	SOM_MAP_201_open();
	SendMessage(som_tool,WM_COMMAND,1071,0);	
	SetEnvironmentVariableW(L".MAP",backup); //restore
}
extern int SOM_MAP_201_multisel(int (&maps)[64])
{
	HWND lb = GetDlgItem(som_tool_stack[1],1239);
	enum{ mapsN=EX_ARRAYSIZEOF(maps) };
	int iN = ListBox_GetSelItems(lb,mapsN,maps);

	//BASIC LAYER SUPPORT
	for(int i=1;i<iN;i++) 
	assert(maps[i]>maps[i-1]);
	memset(maps+iN,0xff,sizeof(int)*(mapsN-iN));
	for(int i=iN;i-->0&&i!=maps[i];) 
	{	
		maps[maps[i]] = maps[i]; maps[i] = -1;
	}
	for(int i=0;i<mapsN;i++) if(-1!=maps[i])
	{
		//might as well filter empty map slots
		//before trying to determine if layers
		if(0xFF!=ListBox_GetItemData(lb,i))
		{
			int j = som_LYT_base(i);
			if(j!=-1){ maps[i] = -1; maps[j] = j; }
		}
		else maps[i] = -1; //0xff
	}
	
	//NOTE: empty MAP slots are now filtered
	//out with layers
	int mpx = 0;
	for(int i=0;i<mapsN;i++) if(-1!=maps[i])
	mpx++; return mpx;
}
extern void som_tool_recenter(HWND,HWND,int=0);
extern windowsex_DIALOGPROC som_tool_PseudoModalDialog;
extern HWND SOM_MAP_or_SOM_PRM_progress(HWND p, int range)
{
	int rsrc = SOM::tool==SOM_PRM.exe?149:157;
	HRSRC found = FindResource(0,MAKEINTRESOURCE(rsrc),RT_DIALOG);
	DLGTEMPLATE *locked = (DLGTEMPLATE*)LockResource(LoadResource(0,found)); 
	HWND task = CreateDialogIndirectParamA(0,locked,p,som_tool_PseudoModalDialog,0);
	HWND pb = GetDlgItem(task,1053);
	if(rsrc==149) som_tool_recenter(task,p); //why does SOM_MAP not need this???
	SendMessage(pb,PBM_SETSTEP,1,0); //2021: does SOM_MAP need this???
	SendMessage(pb,PBM_SETRANGE32,0,range);	
	//som_tool_progressproc implements a message pump
	SendMessage(pb,PBM_SETPOS,0,0); return pb;
}
extern void SOM_MAP_error_033(int exit_code)
{
	char buf[64]; //44
	sprintf(buf,(char*)0x0048FD0C,exit_code);
	MessageBoxA(0,buf,0,MB_ICONERROR); //"MAP033"
}
extern void SOM_MAP_165_maps(HWND pb2=0)
{
	WCHAR backup[4];
	if(-1==SOM_MAP_layer(backup,4))
	return;

	char replay = som_tool_play;
	
	enum{ mapsN=64 }; int maps[mapsN]; 
	
	int mpx = SOM_MAP_201_multisel(maps); //2021

	DestroyWindow(som_tool_stack[2]);
	HWND pb = pb2?pb2:SOM_MAP_or_SOM_PRM_progress(som_tool_stack[1],mpx);	
	
	LPSTR info = (LPSTR)0x48fce8, info2 = (LPSTR)0x48fd00;  
	PROCESS_INFORMATION pi;
	if(mpx==0) goto empty;
	for(int i=0;i<mapsN;i++) if(-1!=maps[i]) //0xff/layer?
	{		
		WCHAR _map2[32];
		swprintf(_map2,L"%02d",i);
		SetEnvironmentVariableW(L".MAP",_map2); 

		som_tool_play = replay;
		if(CreateProcessA(0,"mapcomp",0,0,1,0,0,0,0,&pi))
		{
			WaitForSingleObject(pi.hProcess,INFINITE);

			//I'm not sure if MapComp even generates a nonzero exit code?
			DWORD ec; if(!GetExitCodeProcess(pi.hProcess,&ec))
			{
				assert(0); //UNTESTED
			}
			else if(ec) //EXPERIMENTAL
			{
				//UNIFINISHED: theres a %d formatter in this
				//code's string that might be the error code
				//MessageBoxA(0,"MAP033",0,MB_ICONERROR);
				SOM_MAP_error_033(ec);
			}

			CloseHandle(pi.hThread); //redundant?
			CloseHandle(pi.hProcess);

			SendMessage(pb,PBM_STEPIT,0,0);
		}
		else empty: //UNLIKELY
		{
			//032 precedes 012. it may be blank. probably means !CreateProcess 
			//MessageBoxA(som_tool_stack[1],"MAP032","",MB_ICONWARNING);
			//MessageBoxA(som_tool_stack[1],(LPSTR)0x48fcd4,"",MB_ICONWARNING);
			info = (LPSTR)0x48fcd4; info2 = "";
			break;
		}		
	}
	som_tool_play = 0;

	if(mpx>1) //act like usual?
	for(int i=10;i-->0;EX::sleep(100))	
	SendMessage(pb,PBM_SETPOS,mpx,0);
	if(!pb2)
	if(DestroyWindow(GetParent(pb))) //egg timer?
	{
		EX::sleep(100); //2021: transition?
		MessageBoxA(som_tool_stack[1],info,info2,MB_ICONINFORMATION);
	}

	SetEnvironmentVariableW(L".MAP",backup); //restore
}
extern LRESULT CALLBACK SOM_MAP_165(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR st)
{
	switch(uMsg)
	{	
	case WM_COMMAND:

		switch(wParam)
		{
		case IDOK: 

			if(!GetDlgItem(hWnd,1000)) //2021
			if(SendDlgItemMessage(hWnd,1001,BM_GETCHECK,0,0))
			{
				wParam = 1000; //use clip checkbox?
			}
			//break;

		case 1000: //make MPX out of MHM geometery?		
		
			//HACK: inform som_tool_CreateProcessA 
			//2/3: MHM/MSM mode. 8/16 are settings
			som_tool_play = 2|1&wParam;
			som_tool_play|=SOM_MAP_4921ac.masking<<2;
			som_tool_play|=SOM_MAP_4921ac.lighting<<3;
		
			//2021: art/convert checkbox?
			if(SendDlgItemMessage(hWnd,2021,BM_GETCHECK,0,0))
			{
				extern void som_art_2021(HWND);
				som_art_2021(hWnd); 
				som_tool_play = 0; return 0;
			}
			if(hWnd==som_tool_stack[2])
			{
				SOM_MAP_165_maps(); return 0;
			}

			wParam = IDOK; break; //MapComp?

		case 5: //save settings without MPX?
		
			if(st!=*(WORD*)&SOM_MAP_4921ac.masking)
			{
				SOM_MAP_4921ac.modified[0] = 1; //Save?
				SetWindowSubclass(hWnd,SOM_MAP_165,0,*(WORD*)&SOM_MAP_4921ac.masking);
			}
			return 0;	
		
		case 1001: //show MHM in editor?
			som_map_tileviewmask^=0x100; 
			for(DWORD i=0,iN=SOM_MAP_4921ac.partsN;i<iN;i++)
			std::swap(SOM_MAP_4921ac.parts[i].msm,SOM_MAP_4921ac.parts[i].mhm);			
			extern void som_map_refresh_model_view();
			som_map_refresh_model_view();
			return 0;
		case 1122: SOM_MAP_4921ac.masking = !SOM_MAP_4921ac.masking; 
			return 0;
		case 1123: SOM_MAP_4921ac.lighting = !SOM_MAP_4921ac.lighting; 
			return 0;
		case IDCANCEL: goto close;
		
		case 2021: //2021: compile checkbox? (som.art.cpp)
		
			if(HWND c=GetDlgItem(hWnd,2022))
			{
				int x = Button_GetCheck((HWND)lParam);				
				EnableWindow(c,x);
				if(!x&&!Button_GetCheck(c)) 
				{
					Button_SetCheck(c,1);
					SendMessage(hWnd,WM_COMMAND,0,(LPARAM)c); //text...
				}
			}
			break;

		case 2022: //falling thorugh

			if(HWND c=GetDlgItem(hWnd,2023)) //group box text?
			{
				int x = Button_GetCheck((HWND)lParam);
				if(GetWindowTextW(c,som_tool_text,MAX_PATH))				
				if(wchar_t*p=wcschr(som_tool_text,'('))
				{
					wcscpy(p+1,x?L"MapComp.exe":L"x2mdl.dll");
					SetWindowTextW(c,som_tool_text);
				}
				else if(wchar_t*p=wcschr(som_tool_text,L'\xff08'))
				{
					wcscpy(p+1,x?L"MapComp.exe\xff09":L"x2mdl.dll\xff09");
					SetWindowTextW(c,som_tool_text);
				}
			}
			break;
		}
		break;
	
	case WM_CLOSE: close: //maps?

		if(hWnd==som_tool_stack[2])
		{
			//REMINDER: SOM (MFC?) uses modeless dialogs exclusively
			//EndDialog(hWnd,0);
			DestroyWindow(hWnd); return 0;
		}
		break;

	case WM_NCDESTROY:

		//implement Cancel->Close sea-change
		*(WORD*)&SOM_MAP_4921ac.masking = st;

		RemoveWindowSubclass(hWnd,SOM_MAP_165,id);
		break;
	}

	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
EX::temporary &SOM::Tool::Mapcomp_MHM_temporary()const
{
	static EX::temporary tmp; return tmp;
}
HANDLE SOM::Tool::Mapcomp_MHM(const wchar_t *w)const
{
	//hack: sharing tmp...
	//static EX::temporary tmp;
	EX::temporary &tmp = Mapcomp_MHM_temporary(); 		
	if(!w) dup: return tmp.dup(); 

	namespace mhm = SWORDOFMOONLIGHT::mhm;
	namespace msm = SWORDOFMOONLIGHT::msm;
	mhm::image_t in; mhm::maptofile(in,w); 
	if(!in) bad:
	{
		SetLastError(ERROR_FILE_NOT_FOUND); 
		assert(0); 
		return INVALID_HANDLE_VALUE;
	}

	mhm::face_t *fp;	
	mhm::index_t *ip;
	mhm::vector_t *vp,*np;
	mhm::header_t &hd = mhm::imageheader(in);
	int npN = mhm::imagememory(in,&vp,&np,&fp,&ip);
	if(!npN&&!in) goto bad;

	//4023A0 inserts unique vertices into a sorted
	//buffer, and so should be avoided at all cost
	bool call_4023A0 = 
	SOM::tool==MapComp.exe&&-1!=MapComp_memory[1];
		
	SetFilePointer(tmp,0,0,FILE_BEGIN);
	//WORD wd[2] = {0,npN}; 
	struct
	{
		WORD tN; char bmp[10];
		WORD vN;
	}wd = {1,"atari.bmp",npN};	
	DWORD wr; 
	WriteFile(tmp,&wd,sizeof(wd),&wr,0); 
	unsigned i,j,k,l,iN = hd.facecount,jN,kN;
	for(i=j=0;i<iN;i++,j=0)	
	{
		mhm::vector_t &n = np[fp[i].normal];
		float tnl[3+3+2] = {0,0,0,n[0],n[1],n[2]};
		for(jN=fp[i].ndexcount;j<jN;j++)
		{		
			mhm::vector_t &v = vp[*ip++];
			memcpy(&tnl,&v,sizeof(v));
			if(1) //UV map?
			{
				//NOTE: 0.001953125f is 1/256/2
				//TODO! it seems like there is a bug here...
				//for non-256 textures the half pixel offset
				//is required. (MDLs have 8-bit UVs)
				tnl[6] = v[0]; //+0.001953125f;
				tnl[7] = v[2]; //+0.001953125f;
			}
			WriteFile(tmp,tnl,sizeof(tnl),&wr,0); 

			//HACK: good time to add indices to MapComp's vbuffer
			if(call_4023A0)
			{
				tnl[3] = tnl[6]; tnl[4] = tnl[7];
				((DWORD(__cdecl*)(float*))0x4023A0)(tnl);				
				//YUCK
				tnl[3] = n[0]; tnl[4] = n[1]; 
			}
		}
	}
	
	enum{ pbufN=64 };
	WORD pbuf[pbufN];
	pbuf[0] = (WORD)iN; 
	for(i=l=0,j=1;i<iN;i++)
	{
		mhm::face_t &f = fp[i];
		kN = f.ndexcount;
		if(j+3+kN>=pbufN)
		{
			WriteFile(tmp,pbuf,2*j,&wr,0);
			j = 0;
		}		
		pbuf[j++] = 0;
		pbuf[j++] = kN;
		for(k=0;k<kN;k++) pbuf[j++] = l++;
		pbuf[j++] = 0;
	}	
	WriteFile(tmp,pbuf,2*j,&wr,0);			
	
	SetEndOfFile(tmp);
	
	mhm::unmap(in);

	SetFilePointer(tmp,0,0,FILE_BEGIN); 
	
	goto dup; //hack...
}
HANDLE SOM::Tool::Mapcomp_MSM(const wchar_t *w)const
{	
	//TODO: it would be better to be able to coax MapComp
	//into full tessellation so it can use the simplified
	//model for shadows, if that's indeed how it operates

	HANDLE tmp = Mapcomp_MHM(0); //hack: sharing tmp

	namespace msm = SWORDOFMOONLIGHT::msm;
	msm::image_t in; msm::maptofile(in,w); 
	if(!in) return INVALID_HANDLE_VALUE;

	/*TODO? add blend option to MAP file
	const msm::softvertex_t *sv = msm::softvertex(in); 
	extern int SoftMSM(const wchar_t*,int=0); if(!sv)
	{
		//opportunistically convert MSM to soft MSM?
		//NOTE: MapComp will fail if an error occurs
		//but that should not be happening under any
		//circumstances here
		
			msm::unmap(in);

			int err = SoftMSM(w); assert(!err);
			msm::maptofile(in,w); if(!in) //???
			{
				assert(0);
				return INVALID_HANDLE_VALUE;
			}
			sv = msm::softvertex(in);
	}*/

	msm::vertices_t &v = msm::vertices(in); 	

	msm::polystack_t polystack = msm::polystack;

	if(SOM::tool==MapComp.exe)
	{
		//2022: allow for 0 configuration
		//MapComp_memory[4] = sv?min(v.count,*sv):v.count;
	//	MapComp_memory[4] = sv?~min(v.count,*sv):v.count;
		MapComp_memory[4] = v.count;
	}
	else
	{
		polystack.max = EX::INI::Editor()->tile_view_subdivisions;
	}

	auto *t = msm::terminator(in); //2022
	assert(t);

	SetFilePointer(tmp,0,0,FILE_BEGIN);
	//this is to support "antialiasing" (do_aa2)
	//make the final subdivision level the only level
	const char *top = in.header<char>();
	size_t i=0; WORD n=0,eof=0; DWORD wr,fp=0; 
	const msm::polygon_t *p,*q = msm::firstpolygon(in);	
	if(q) WriteFile(tmp,top,(char*)q-top,&fp,0); char swap[1024];
	//HAVING problems with reading past page boundary
	//if(q) while(in>=(q=msm::shiftpolygon(p=q))) if(msm::polystack<<p)	
	//if(q) while(in>q+1) //works in release (SWORDOFMOONLIGHT_N)
	//don't understand this hack (it's 0-terminated)
	//if(q) while(in>=q->indices+3) //HACK: yk1104.msm (0003.prt)
	if(q&&t) while(q<t)
	{
		q = msm::shiftpolygon(p=q); if(polystack<<p)		
		{	
			size_t len = (char*)q-(char*)p;	if(i+len>sizeof(swap))
			{
				WriteFile(tmp,swap,i,&wr,0); i = 0;
			}		
			((msm::polygon_t*)memcpy(swap+i,p,len))->subdivs = 0; 
		
			n++; i+=len; 
		}
	}
	if(i) WriteFile(tmp,swap,i,&wr,0);
	
	if(!msm::polystack) assert(0); //reset/validate

	WORD zt = 0; 
	WriteFile(tmp,&zt,2,&wr,0); 
	SetEndOfFile(tmp);
	SetFilePointer(tmp,fp,0,FILE_BEGIN);
	WriteFile(tmp,&n,2,&wr,0); 
	
	//MapComp (layers present)	
	if(SOM::tool==MapComp.exe&&-1!=MapComp_memory[1])
	{
		//preinsert so layers end up with the same indices		
		float v5[5]; for(int i=v.count;i-->0;)
		{	
			//004021A1 E8 FA 01 00 00       call        004023A0
			memcpy(v5,v[i].pos,12); memcpy(v5+3,v[i].uvs,8);
			((DWORD(__cdecl*)(float*))0x4023A0)(v5);
		}
	}

	msm::unmap(in);	
	SetFilePointer(tmp,0,0,FILE_BEGIN); return tmp;
}

void SOM_MAP_4921ac::Start::elevate(Tile *a, Tile *b)
{
	int xy = xyindex(); 
	if(SOM_MAP_z_index==ex_zindex)
	{	
		if(SOM_MAP.base)		
		{		
			xzy[1]+=a[xy].z; xzy[1]-=b[xy].z;
		}
	}
	else if(!SOM_MAP.base) 
	{
		if(a) xzy[1]-=a[xy].z; if(b) xzy[1]+=b[xy].z;
	}
}
static void SOM_MAP_read_epilog(SOM_MAP_4921ac::Start &el)
{
	//MapComp treats this byte as a z-index
	//except for objects. It must have been
	//changed last minute
	/*int zi;
	if(obj) //&el<&SOM_MAP_4921ac.enemies
	{
		zi=el.uwv[1]/360;el.uwv[1]%=360;
		if(zi<0){ zi = -zi; assert(0); }
		if(el.objtype_or_zindex>7)
		el.objtype_or_zindex = 0;
	}
	else zi = el.objtype_or_zindex;
	el.ex_zindex = 0xFF&zi;
	el.ex_original_zindex = 0xFFFF&&zi;*/
	//legacy objects have this set to their object-type
	//otherwise just restricting the value
	if(el.ex_zindex>7) el.ex_zindex = 0;
	el.ex_original_zindex = el.ex_zindex;	
	el.elevate(SOM_MAP.base,SOM_MAP_4921ac.grid);
}
static void SOM_MAP_write_prolog(SOM_MAP_4921ac::Start &el)
{
	/*if(obj) //&el<&SOM_MAP_4921ac.enemies
	{
		int i = 360*el.ex_zindex;
		if(el.uwv[1]<0) i = -i; el.uwv[1]+=i;
	}
	else el.objtype_or_zindex = el.ex_zindex;
	*/
	el.elevate(SOM_MAP_4921ac.grid,SOM_MAP.base);
}
static void SOM_MAP_write_epilog(SOM_MAP_4921ac::Start &el)
{
	//if(obj&&el.ex_zindex) //&el<&SOM_MAP_4921ac.enemies
	//el.uwv[1]%=360;
	el.elevate(SOM_MAP.base,SOM_MAP_4921ac.grid);
}
static void SOM_MAP_foreach(void(*ea)(SOM_MAP_4921ac::Start&))
{	
	ea(SOM_MAP_4921ac.start);

	//assuming memory is zeroed
	#define _(x) if(x.props!=0xFFFF) ea(x)
	for(int i=0;i<512;i++) _(SOM_MAP_4921ac.objects[i]);
	for(DWORD i=0;i<SOM_MAP_4921ac.enemies_size;i++) 
	_(SOM_MAP_4921ac.enemies[i]);
	for(int i=0;i<128;i++) _(SOM_MAP_4921ac.NPCs[i]);
	for(int i=0;i<256;i++) _(SOM_MAP_4921ac.items[i]);
	#undef _
}
extern void SOM_MAP_map2_etc_epilog() //som_tool_CloseHandle
{
	//2019: Map menu is corrupting the new layer system
	assert(!som_tool_stack[1]);

	bool ro = SOM_MAP.map.rw==SOM_MAP.map.read_only;
	if(!ro&&SOM_MAP.map.rw!=SOM_MAP.map.write_only2)
	{
		//NEW: ensure SOM_MAP_write_prolog has been applied
		//before undoing with SOM_MAP_write_epilog. I think
		//this may be how my MAP file got corrupted one day
		assert(0); return; 
	}
	if(!ro||SOM::tool==SOM_MAP.exe)	
	SOM_MAP_foreach(ro?SOM_MAP_read_epilog:SOM_MAP_write_epilog);
}

static bool SOM_MAP_map2_internal(const char *p, bool ro, int i, SOM_MAP_4921ac::Tile* &layer, WCHAR *w2=0)
{
	//2021: this was SOM_MAP_map2 but in order to call it
	//in a loop for each layer it seems best to remove it
	//to its own function, mainly because reading/writing
	//are very different

	struct SOM_MAP_map &map = SOM_MAP_map;

	bool is_base = &layer==&SOM_MAP.base;

	//previously SOM_MAP::base2
	//WARNING: this is used to calculate the size of the back
	//of the storage area when "layer" is 0 (kind of tricky)
	auto SOM_MAP_base2 = [&layer]()->DWORD*
	{
		return (DWORD*)(layer+100*100); //base
	};

	//ALGORITHM
	//opening a layer requires the base map to be opened also
	//this stops reading at line 104 and switches to the base
	//map. the rest of the layer is retained so it's lossless
	
	DWORD rw; if(!ro) //WRITING? (only the base file)
	{
		assert(layer==SOM_MAP.base);

		//write the back half of the layer MAP as retrieved
		//when it was read in
		DWORD *b2 = SOM_MAP_base2();
		DWORD sz = *b2-sizeof(*b2)-1; rw = 0;
		SOM::Tool.WriteFile(map,"\r\n",2,&rw,0); //wrinkle
		SOM::Tool.WriteFile(map,b2+1,sz,&rw,0);
		if(rw!=sz){ assert(0); return false; }
	}
	else assert(!layer);

	char a[MAX_PATH]; //copying MapComp_LYT
	sprintf(a,w2?"%s%ls":"%s%02d.map",SOMEX_(B)"\\data\\map\\",w2?(int)w2:i);	
	const wchar_t *w = SOM::Tool.project(a);
	if(!w){ assert(0); return false; }
	HANDLE h = CreateFile(w,
	ro?GENERIC_READ:GENERIC_WRITE,FILE_SHARE_READ,0,
	ro?OPEN_EXISTING:CREATE_ALWAYS,0,0);
	if(h==INVALID_HANDLE_VALUE)
	{
		if(ro) //SOM_MAP crashes (access violation) 
		{
			//I think this is fixed by som_tool_ReadFile returning
			//premature EOF
			//return false; 
		}
		assert(0); return false; //MessageBox
	}
	
	if(map.buffer.empty()) p = 0;
	const int os = p?p-&map.buffer[0]:0; 
	const int rem = p?(int)map.buffer.size()-os:0;
	const DWORD fp = SetFilePointer(map,ro?-rem:0,0,FILE_CURRENT);
			
	if(!ro) //WRITING? 
	{
		if(layer==SOM_MAP.base)
		{
			//HACK: put map.buffer on the back of the layer MAP so it
			//can be read back/restored just as the reading path does
			//
			// 2022: what does "just as the reading path does" mean?
			// the writing path didn't have GENERIC_READ permissions
			// for the ReadFile call below... I'm wondering if reads
			// are missing GENERIC_WRITE permissions now (my feeling
			// is most likely this comment is misworded)
			//
			if(p&&!SOM::Tool.WriteFile(map,p,rem,&rw,0))
			{
				DWORD err = GetLastError();

				assert(0); return false;
			}
		}
		else //2021: programmer error?
		{
			assert(0); return false;
		}
	}
	else if(!layer) //READING?
	{
		DWORD sz = 0; if(is_base)
		{
			//TODO? the 0-terminator is not actually used
			DWORD eof = SetFilePointer(map,0,0,FILE_END);	
			sz = sizeof(DWORD)+eof-fp+sizeof('\0');	
		}
		char *mem = new char[(int)SOM_MAP_base2()+sz];
		new((void*&)layer=mem) SOM_MAP_4921ac::Tile[100*100]; 
		if(is_base)
		{
			auto new_base2 = SOM_MAP_base2(); //RECOMPUTING

			new_base2[0] = sz;
			mem = (char*)&new_base2[1];
			mem[sz-=sizeof(DWORD)+sizeof('\0')] = '\0';
			SetFilePointer(map,fp,0,FILE_BEGIN);
			if(!SOM::Tool.ReadFile(map,mem,sz,&rw,0)||rw!=sz)	
			{
				SOM::Tool.CloseHandle(h);

				assert(0); return false;
			}
			//can't close the handle because the fread/fwrite
			//implementation requires it for some purpose
			//SOM::Tool.CloseHandle(map);	
			//HACK: can't be left at end-of-file
			SetFilePointer(map,fp,0,FILE_BEGIN); 
		}
	}
	else //2021: programmer error?
	{
		assert(!layer); return false;
	}

	std::swap(i,map.current);	
	auto v = map.versions[i];
	{
		int yuck = map.rw; //write_only2?

		//HACK: reset handles (in2) but retain
		//the original handle (now closed) that
		//SOM_MAP created 
		//(NOTE: it's not actually closed)			
		HANDLE fake = map.in;
		map.in = 0; map = h; map.in = fake; //operator=(HANDLE)
		map.rw = yuck; 

		//read/write always reserve 4096+128
		char buf[4096]; 
		assert(map.buffer.capacity()>=sizeof(buf)+128);

		//UNION
		//hiding base map from SOM_MAP_map::write as
		//writing the base map itself, and so not to
		//reenter this subroutine
		auto *swap = SOM_MAP.layer;
		SOM_MAP.layer = ro?layer:0;

		if(ro) //READING?
		{
			bool clear_104 = false;
			for(;;) //while(map.line<104) //100x100
			{
				rw = sizeof(buf);
				if(!SOM_MAP.map.read(buf,rw,&rw))
				break;
				assert(!SOM_MAP.map.finished);

				//HACK: the line is rolled back to 103
				//since it's incomplete
				if(map.line==103)
				{
					//I think this should always be but
					//it can't hurt to check to be sure
					if(map.tile==map.area)
					{
						clear_104 = true; break;
					}
					else assert(0);
				}
			}				
			if(!clear_104)
			{
				assert(clear_104); return false;
			}
			else map.line = 104;

			if(!is_base) //YUCK
			{
				CloseHandle(h); h = 0;

				map.in2 = map.in; assert(map.in==fake);
			}
		}
		else //WRITING? (only the base file)
		{
			auto base = swap; //SOM_MAP.base

			//HACK: sourcing title from UI title
			HWND cb = GetDlgItem(som_tool,1133);
			DWORD len = GetDlgItem(cb,1001)
			?SOM::Tool.SendMessageA(cb,CB_GETLBTEXT,0,(LPARAM)buf)
			:SOM::Tool.GetWindowTextA(cb,buf,40);
			assert(*buf=='['); //"[00] "
			memcpy(buf+1,"12\r\n",4);
			memcpy(buf+len,"\r\n100,100\r\n",11);
			if(!map.write(buf+1,-1+len+11,&rw))
			{
				assert(0); return false;
			}
			//auto *b = base; //upside down :(
			auto *b = base+100*100-100; rw = 0;
			for(int j,i=0;i<100;i++,b-=200)
			{
				for(j=0;j<100&&rw<sizeof(buf)-32;j++,b++) j:
				{
					if(b->part==0xFFFF)
					{
						//TODO? wmemcpy?
						buf[rw++] = '-';
						buf[rw++] = ',';
					}
					else
					{
						rw+=sprintf(buf+rw,"%d/%g/%d/%c,",
						b->part,b->z,(int)b->spin,b->ev);							
					}
				}				
				if(j==100)
				if(i!=99) //HACK: write has the last CRLF
				{
					buf[rw++] = '\r';
					buf[rw++] = '\n';
				}
				if(!map.write(buf,rw,&rw))
				{
					SOM_MAP.base = base; //YUCK

					assert(0); return false;
				}
				rw = 0;
				if(j!=100) goto j;
			}			
		}

		SOM_MAP.layer = swap;
	}
	//HACK: this is for CloseHandle which currently
	//backs up version 12 files to a ".MA~" file or
	//replaces the MAP file if upgrade to 13 is not
	//required	
	std::swap(i,map.current); map.version = v;
	
	//RESUMING SOM_MAP_map::read/write
	
	if(!ro) //WRITING? (only the base file)
	{	
		h = map; //!!
		SetFilePointer(h,fp,0,FILE_BEGIN);
	}
	else if(is_base) //h?
	{
		SetFilePointer(h,-(int)map.buffer.size(),0,FILE_CURRENT);		
	}
	if(p) //READ+WRITE???
	{
		map.buffer.resize(os+rem); //truncating?
		if(p!=&map.buffer[os]) //paranoia
		{
			//can't let the vector reallocate itself :(
			assert(0); return false;
		}
		if(!SOM::Tool.ReadFile(h,(char*)p,rem,&rw,0))
		{
			DWORD err = GetLastError();

			assert(0); return false;
		}
		if((DWORD)rem>rw) //assuming EOF
		{
			//HACK: can't shrink SOM_MAP_map.read's buffer
			memset((char*)p+rw,' ',rem-rw); 
		}
	}
	if(!ro) //WRITING? (only the base file)
	{
		//recall that write's buffer was backed up on
		//the back of the layer's MAP file
		SetFilePointer(h,fp,0,FILE_BEGIN);
		//note reading must set the file pointer back
		//from the EOF... but write operations sit on
		//the back of the file, and so should be fine
		SetEndOfFile(h); 
	}

	return true;
}
static bool SOM_MAP_map2(const char *p) //SUBROUTINE
{	
	struct SOM_MAP_map &map = SOM_MAP_map;

  //NOTE: there's so much commonality between the read
  //and write operations that they are merged into one 

	bool ro = map.rw==map.read_only;
	if(!ro) //WRITING? 
	{
		if(!SOM_MAP.base) return true;
	}

	//LEGACY ONLY
	//passing map.current to this routine restricts layers to
	//the 64 two-digit MAP files in DATA/MAP where it would be
	//nice if that requirement only applied to the base layer
	int i = som_LYT_base(map.current);

	if(ro) //2021: reading in all layers for the tile viewer
	{
		auto lyt = SOM_MAP.lyt[i==-1?map.current:i];

		if(!lyt) return true;

		//NOTE: SOM_MAP_map2_internal would need to do this
		//for each layer, whereas here it's just once
		std::vector<char> backup = SOM_MAP_map.buffer; //YUCK
		
		for(;*lyt;lyt++)
		{
			auto &y = *lyt;

			int l = y->group;

			//YUCK: assuming the MAP file must be a legacy name
			//so that this works to screen out the current file
			//from being represented twice
			/*this should hold until synthetic layers are setup
			if(i==map.current) continue;*/
			if(l==SOM_MAP_z_index) continue;
			assert(i!=map.current);			

			int err = 0;
			if(l<=0) err = 1; //UNIMPLEMENTED
			else if(l>=EX_ARRAYSIZEOF(SOM_MAP.layers)) err = 2; //IMPOSSIBLE
			else if(SOM_MAP.layers[l]) err = 3; //UNIMPLEMENTED
			if(err)
			{
				#ifdef NDEBUG
				//#error need a MessageBox here to explain this
				int todolist[SOMEX_VNUMBER<=0x1020602UL];
				#endif
				assert(0); return true; 
			}

			//copying MapComp_LYT
			int j = y->legacy();
			auto w2 = *y->w2?y->w2:0; //YUCK
			if(!SOM_MAP_map2_internal(0,ro,j,SOM_MAP.layers[l],w2))
			return false;		
		}

		SOM_MAP_map.buffer = backup; //YUCK
	}

	//IMPORTANT: this swaps SOM_MAP.map onto the 
	//base map in order to have the original map
	//reading routine read its post map contents
	if(i!=-1)
	if(!SOM_MAP_map2_internal(p,ro,i,SOM_MAP.base))
	return false;
	return true;
}
extern DWORD MapComp_408ea0(DWORD); //2022
extern const wchar_t *som_tool_reading_mode[];
BOOL SOM_MAP_map::read(LPVOID to, DWORD sz, LPDWORD rd)
{	
	char *_ = (char*)to, *_s = _+sz; 
	
	LPCVOID tc = to; to = 0; DWORD cp = *rd = 0; 
	
	BOOL out = SOM::Tool.ReadFile(in2,_,sz/4*3,&cp,0); 

	buffer.reserve(4096+128); assert(sz<=4096);

	buffer.insert(buffer.end(),_,_+cp); 		
	tc = buffer.data(); 
	cp = buffer.size(); 

	if(!cp) return out; //som_tool_ReadFile handles EOF

	//int finish_line = SOM::tool==MapComp.exe?1138:104;
	const int finish_line = 104;
	 		
	const char *p,*q,*d; 
	for(p=(char*)tc,q=p,d=p+cp;q<d;p=q)
	{	
		while(q!=d&&*q!='\n') q++;

		//WARNING: DON'T DO MATH WITH line SINCE IT'S NOT
		//INCREMENTED UNTIL THE LINE ENDING IS BUFFERED!!
		//WARNING: DON'T DO MATH WITH line SINCE IT'S NOT
		//INCREMENTED UNTIL THE LINE ENDING IS BUFFERED!!
		//WARNING: DON'T DO MATH WITH line SINCE IT'S NOT
		//INCREMENTED UNTIL THE LINE ENDING IS BUFFERED!!

		if(*q=='\n'&&q!=d) line++; //q++;

		assert(line<=103||tile>=area); //debugging (100x100)

		if(line<4)
		{
			if(line==1)
			{													
				version = atoi(p); 

				rw|=read_only; assert(!SOM_MAP_prt.indexed);

				//REMOVE ME?
				versions[current] = version;

				if(version==13)	(char&)p[1] = '2'; //12

				if(in==in2) //YUCK: precompute this?
				{
					//2019: map menu is corrupting the new layer system
					if(!som_tool_stack[1])
					{
						SOM_MAP_z_index = 0;
						int base = som_LYT_base(current); 
						if(base!=-1) 
						if(som_LYT**lyt=SOM_MAP.lyt[base])					
						while(*lyt++)
						if(current==lyt[-1]->legacy())					
						SOM_MAP_z_index = lyt[-1]->group;

						//2023: reliably load ambient2 data?
						extern void som_MPX_411a20_ambient2(const unsigned,int);
						som_MPX_411a20_ambient2(base==-1?current:base,SOM_MAP_z_index);

						extern void SOM_MAP_clear_undo(); //2024
						if(SOM::tool==SOM_MAP.exe)
						SOM_MAP_clear_undo(); //HACK
					}
				}
			}
			else if(line==3)
			{
				if(2!=_snscanf(p,q-p,"%d,%d",&width,&height))
				{
					assert(0); return FALSE; //let SOM_MAP deal
				}
				else area = width*height;
				
				assert(100*100==area);
				if(area<1||100*100!=area)
				{
					assert(0); return FALSE;
				}

				//HACK: short-circuit Map selection menu
				if(som_tool_initializing
				&&41000==GetWindowContextHelpId(som_tool_stack[1]))
				{
					_s = _+(q-p)+1; finished = true;
				}
			}
		} 
		else if(tile<area) //paranoia
		{
			typedef SOM_MAP_prt::key key;

			if(*p==',') //NEW
			{
				//scanf succeeds without the ,
				if(p==tc) p++; else assert(0);
			}

			enum{ pro_s = 31+4, ico_s = 31 };
			char mem[128],pro[pro_s],ico[ico_s];			

			do //conversion //retry:
			{
				const char *skip;
				for(skip=p;*p=='-'&&p<q-1;p++) 
				{
					p++; if(*p!=',')
					{
						assert(0); return FALSE;
					}
				}
				if(skip!=p) //-,
				{						
					size_t n = min(_s-_,p-skip); 

					if(n%2) n--; tile+=n/2; p = skip+n; 
					
					memcpy(_,skip,n); *rd+=n; _+=n; 
					
					//2022: hitting *p!='\r' test below
					if(tile>=area) if(tile>area)
					{
						assert(tile==area); return FALSE;
					}
					else continue;
				}				
				//avoid double indexes
				//and unskipped dashes
				if(_s-_<32) 
				{
					goto overflow; //won't fit //breakpoint
				}

				size_t pc = SOM_MAP.prt.missing; 
				
				unsigned no; float z; char spin, ev; *pro = *ico = '\0';

				if(4==_snscanf(p,q-p,"%d/%f/%c/%c,",&no,&z,&spin,&ev))
				{
					if(version<13) //hack
					{
						wchar_t find[16] = L"";
						swprintf_s(find,L"%04d.prt",no);
						pc = SOM_MAP.prt[find].number(); //cannot index
					}
					else pc = SOM_MAP.prt[no].index; 
				}
				else if(version>=13&&6==
				_snscanf_s(p,q-p,"%d:%[^:]:%[^/]/%f/%c/%c,",&no,pro,pro_s,ico,ico_s,&z,&spin,1,&ev,1))
				{	
					const key &k = SOM_MAP.prt[EX::need_unicode_equivalent(65001,pro)];

					if(!k) //rename?
					{
						const wchar_t *w = EX::need_unicode_equivalent(65001,ico);

						for(size_t i=0,iN=SOM_MAP.prt.keys();i<iN;i++) 
						{
							const wchar_t *icon = SOM_MAP.prt[i].iconID;
							if(*w==*icon&&!wcscmp(w,icon)){ pc = i; break; }
						}
					}
					else pc = k.number(); 
					
					if(SOM_MAP.prt[pc].reverse_indexed(no))
					{
						assert(0); return FALSE;
					}
				}
				else if(q==d) //partial
				{
					goto overflow; //should suffice

					//char app[128];
					//out = SOM::Tool.ReadFile(in2,app,sizeof(app),&cp,0);
					//buffer.append(app,cp);
					//goto retry;
				}
				else if(*p!='\r')
				{
					assert(0); return FALSE;
				}		
				else continue;
				
				const key &k = SOM_MAP.prt[pc];

				if(!k) 
				{
					//HACK: adapting this to EX::messagebox... what is the 3rd path?
					int msg; wchar_t v12[10];
					if(!*pro&&version<13){ msg = 12; swprintf(v12,L"%04d.prt",no); }
					else msg = *pro?1:0;					
					if(msg!=0) EX::messagebox(MB_OK|MB_ICONERROR,
					"This map cannot be read because it is comprised of one or more pieces missing from this project.\n"
					"\n"
					"Missing pieces are not yet supported, however they are a planned feature. The following profile prevented the reading of this map:\n"
					"\n"
					"Project:\\data\\map\\parts\\%ls",(msg==12?v12:EX::need_unicode_equivalent(65001,pro)));
					else assert(0); 
					return FALSE;
				}	   				
				
				while(*p!=','&&p!=q) p++; 

					//upside down :(
					assert(area==10000&&width==100);
					//2020 Sep. this feels like groundhog day?
					//int tile2 = tile%100+10000-tile/100*100;
					int tile2 = tile%100+10000-(tile/100+1)*100;				
					assert(tile2>=0&&tile2<10000);

				bool yes = version>12&&SOM::tool!=MapComp.exe;
				int n = yes?k.part_number():no;
				{
					//work with MapComp_LYT to register 
					//PRT files of every layer with the MPX
					if(SOM::tool==MapComp.exe) 
					{
						size_t &i2 = k.index2()[0];
						if(i2==SOM_MAP.prt.missing)
						{
							n = MapComp_memory[0]++;
							
							//ORDER-IS-IMPORTANT
							//HACK: som_tool_mapcomp expects this
							//som_tool_prt will overwrite i2 if 0
							SOM_MAP.prt[n].index2()[1] = pc;
							char* &prt = ((char**)0x486FF0)[n];
							if(n>=1024) //OVERFLOW (CRASH?!)
							{
								//1024 is MapComp's limit. does it
								//signal an error code to SOM_MAP?
								assert(0); 
							}
							else if(!prt) //inject layer MHM files
							{
								//SoftMSM feature
								//0 is guarding against a profile
								//that doesn't load an MSM
								DWORD swap = MapComp_memory[4];
								MapComp_memory[4] = 0;

								//2022: need to tie into new art system 
							//	auto err = ((DWORD(*)(DWORD))0x408EA0)(n);
								auto err = MapComp_408ea0(n);

								if(err) //2022: error handling?
								{
									//2022: main does something like this in response to this
									//forwarded error code (I think this is more a debug log)
									((DWORD(*)(DWORD,...))0x40bf34)(0x4150d8,err,buffer.data());

									//2022: give hint in case of a bad MSM or MHM reference?
									//
									// 0x19 is invented by MapComp_408ea0 to mark failure to
									// open a PRT file... in which case som_tool_reading_mode
									// is unreliable. som_tool_file_appears_to_be_missing can
									// cover 0x19 (25)
									//
									if(err!=0x19)
									if(auto*pt=*som_tool_reading_mode)									
									if(!wcsicmp(L".prt",PathFindExtensionW(pt)))
									EX::messagebox(MB_OK,"MapComp error (#%d) hint: %ls",err,pt);
									
									EX::detached = true; //YUCK (Ex.number.cpp)
									exit(err); //simulate main?

									return 0;
								}

								//ANNOYANCE
								//MapComp_main sets these up
								//TODO: MapComp doesn't do it
								//but these can be manipulated
								//to emit fewer MHM files where
								//PRTs share MHM files
								*(*(WORD**)0x4322B8)++ = n; //MHM
								*(*(WORD**)0x4322A0)++ = n; //MSM

								//if(prt) //408ea0 returns success?
								{
									//HACK: overwrite PRT description
									//(unused) with soft-vertex index
									DWORD &sv = (DWORD&)prt[131+1];

									//REMOVE ME (this is too messy)
									DWORD sv2 = MapComp_memory[4];
									if(sv2<0x8fffffff)
									{
										sv = sv2;

										//THIS IS JUST SO MapComp_SoftMSM
										//KNOWS WHEN SOFTENING IS IN PLAY
										MapComp_memory[4] = swap;
									}
									else sv = ~sv2;
								}
							}

							i2 = n; 
						}				
						else n = i2;

						if(-1==MapComp_memory[2]) //HACK
						{
							//these are for SoftMSM/MapComp_LYT
							MapComp_43e638[tile2].prt = 0xFFFF&n;
							MapComp_43e638[tile2].rotation = spin-'0'; 
							MapComp_43e638[tile2].elevation = z;
						}
					}
				}	
				 				
				if(SOM_MAP.layer) //EXPERIMENTAL
				{				
					SOM_MAP.layer[tile2].part = n;
					SOM_MAP.layer[tile2].spin = spin-'0';
					SOM_MAP.layer[tile2].ev = ev;
					SOM_MAP.layer[tile2].z = z;
				}
				n = sprintf_s(mem,"%d/%g/%c/%c,",n,z,spin,ev);
				assert(_s-_>=n);
								
				memcpy(_,mem,n); *rd+=n; _+=n; 

				tile++; 
				
			}while(p<q&&*p==','&&p++&&tile<area);
		}
		else //breakpoint			
		{
			if(SOM::tool==SOM_MAP.exe)
			{
				if(104==line) //layers? 100*100
				{
					if(!SOM_MAP.layer)
					{
						if(!SOM_MAP_map2(p)) //RECURSIVE
						{
							assert(0); return FALSE;
						}
					}
					else //signal to SOM_MAP_read2
					{
						goto overflow; 
					}
				}
			}
			
			//NEW: want to deal with whole lines
			//in order to make sure these things
			//work
			if(q==d&&line<=finish_line)
			goto overflow;
		}

		if(q!=d&&*q++!='\n') 			
		{
			assert(0); return FALSE; //corrupted?
		}

		size_t n = min(_s-_,q-p); memcpy(_,p,n); 		

		*rd+=n; _+=n; p+=n; 
		
		if(_==_s) //2018 (YIKES??? how did this ever work without this?) 
		{
			break; //breakpoint
		}
	}

	if(0) //2018/HACK: revoke line ending?
	{
		overflow: if(q!=d&&*q=='\n') line--; 
	}	

	//returning split CR/LF causes peeking/file-pointer
	//manipulation
	if(*rd&&'\r'==_[-1]){ (char&)*--p = '\r'; --*rd; }

	//buffer.assign(p,d-p); //can't be too careful
	memmove(&buffer[0],p,d-p); buffer.resize(d-p);

	if(buffer.empty()&&line>=finish_line) 
	{
		finished = true; //finished = tile>=area;
	}
		
	assert(!finished||tile==area||0==tile); return out;
}
BOOL SOM_MAP_map::write(LPCVOID at, DWORD sz, LPDWORD wr)
{	
	*wr = sz; BOOL out = TRUE;

	DWORD compile; wr = &compile;

	buffer.reserve(4096+128); assert(sz<=4096);

	if(!buffer.empty())
	{	
		buffer.insert(buffer.end(),(char*)at,(char*)at+sz);
		at = buffer.data();
		sz = buffer.size();
	}

	const char *p, *q, *d;		
	for(p=(char*)at,q=p,d=p+sz;p<d;p=q)
	{	
		while(q!=d&&*q!='\n') q++;

		if(*q=='\n') line++; //q++;

		if(line<4)
		{
			if(line==1)
			{
				if(!version)
				{
					version = legacy?12:13;
				}
				
				if(version==13)
				{
					(char&)p[1] = '3';
					out = SOM::Tool.WriteFile(in2,p,q-p,wr,0); 
					(char&)p[1] = '2'; //paranoia
					p = q; //!
				}

				rw|=write_only; assert(!SOM_MAP.prt.indexed);

				if(in==in2) if(rw==write_only)
				{	
					//NEW: write_only2 ensures SOM_MAP_write_epilog is
					//invoked only if SOM_MAP_write_prolog was invoked
					rw = write_only2;
					SOM_MAP_foreach(SOM_MAP_write_prolog);										

					//write starting point z-index (read works)
					//004127B7 68 F8 F6 48 00       push        48F6F8h  
					//004127BC 57                   push        edi 
					//004127BD E8 A7 54 04 00       call        00457C69
					//zindex can be up to 3 chars, which won't fit 
					//into the available memory 
					//FORTUNATELY this is a .DATA segment address!
					//0x0048F6F8 "0,%d,%d,%.6f,%.6f,%.6f,%d\n\0\0"	
					//REMINDER: "50.000000,0.100000,%.6f\n" is nearby
					//sprintf((char*)0x48F6F8,"%d%s",SOM_MAP_4921ac.start.ex_zindex,",%d,%d,%g,%g,%g,%d\n");
					*(char*)0x48F6F8 = '0'+min(7,SOM_MAP_4921ac.start.ex_zindex);
				
					if(version==12)
					if(legacy&&-1==som_LYT_base(current))
					{
						finished = true; //!!

						out = SOM::Tool.WriteFile(in2,at,sz,wr,0);
						return out;
					}
				}
				else assert(0);
			}
			else if(line==3)
			{
				if(2!=_snscanf(p,q-p,"%d,%d",&width,&height))
				{
					assert(0); return FALSE; //let SOM_MAP deal
				}
				else area = width*height;

				if(area<1)
				{
					assert(0); return FALSE;
				}
			}
		}
		else if(tile<area) //paranoia
		{	
			typedef SOM_MAP_prt::key key;

			if(*p==',') //NEW
			{
				//scanf succeeds without a comma (,)
				if(p==(char*)at) p++; else assert(0);
			}

			enum{ pro_s = 31+4, ico_s = 31 };

			char mem[128], pro[pro_s], ico[ico_s];

			do //conversion
			{
				const char *skip;
				for(skip=p;*p=='-'&&p<q-1;p++) 
				{
					if(p[1]!=',')
					{
						assert(0); return FALSE;
					}
					else p++;
				}
				if(skip!=p) //-,
				{
					tile+=(p-skip)/2; //>>1
					out = SOM::Tool.WriteFile(in2,skip,p-skip,wr,0);
				}

				int n = 0; unsigned no; float z; char spin, ev;
				if(4==_snscanf(p,q-p,"%d/%f/%c/%c,",&no,&z,&spin,&ev))
				{
					tile++; while(*p!=','&&p!=q) p++;

					if(version>12)
					{
						assert(version==13);

						if(version!=13)
						{
							assert(0); return FALSE;
						}

						if(no<1024) //legacy
						{
							wchar_t find[16] = L"";
							swprintf_s(find,L"%04d.prt",no);
							no = SOM_MAP.prt[find].number();
						}
						else no-=1024;

						//assuming cannot be "missing"
						const key &k = SOM_MAP.prt[no]; assert(k);

						if(!k.indexed()) //ugly
						{
							*pro = *ico = '\0'; //paranoia							
							EX::need_ansi_equivalent(65001,k.iconID,ico,ico_s);
							EX::need_ansi_equivalent(65001,k.longname(),pro,pro_s);
							n = sprintf_s(mem,"%d:%s:%s/%g/%c/%c,",k.index,pro,ico,z,spin,ev);
						}
						else n = sprintf_s(mem,"%d/%g/%c/%c,",k.index,z,spin,ev);
					}
					else  
					{
						if(!legacy&&no>=1024) //2018: how does this work?
						{
							//make good on upconversion claim to failure on No
							assert(0); return FALSE; 
						}

						n = sprintf_s(mem,"%d/%g/%c/%c,",no,z,spin,ev);
					}
										
					out = n>0?SOM::Tool.WriteFile(in2,mem,n,wr,0):FALSE;					
				}
				else if(q==d) //partial
				{
					if(tile>=area)
					goto happens;
					goto partial; //return TRUE
				}
				else if(*p!='\r')
				{
					assert(0); return FALSE;
				}
				
			}while(p<q&&*p==','&&p++&&out);

			if(tile>=area) happens: 
			{
				finished = true;

				//emiting final CRLF
				if(!SOM_MAP_map2(p)) //RECURSIVE
				{
					assert(0); return FALSE;
				}
				
				assert(tile==area);				
				if(out) out = SOM::Tool.WriteFile(in2,p,d-p,wr,0);
				buffer.clear();

				return out; 
			}
		}
		else if(EX::debug) 
		{
			static bool one_off = false;
			assert(one_off);one_off = true;
		}

		if(q!=d&&*q++!='\n') assert(0);

		if(out) out = SOM::Tool.WriteFile(in2,p,q-p,wr,0);		
	}   
					
	partial: 
	if(buffer.empty()) buffer.resize(d-p); //YUCK!!
	memmove(buffer.data(),p,d-p); buffer.resize(d-p); return out;
}
extern BOOL WINAPI SOM_MAP_WriteFile(HANDLE in, LPCVOID at, DWORD sz, LPDWORD wr, LPOVERLAPPED o)
{
	extern HANDLE som_sys_dat;
//	extern BOOL WINAPI SOM_SYS_WriteFile(HANDLE,LPCVOID,DWORD,LPDWORD,LPOVERLAPPED);
//	if(in==som_sys_dat) return SOM_SYS_WriteFile(in,at,sz,wr,o);

	//Then this is logging: so lets not log logging!!
	if(in==EX::monolog::handle) return SOM::Tool.WriteFile(in,at,sz,wr,o);
	
	SOM_TOOL_DETOUR_THREADMAIN(WriteFile,in,at,sz,wr,o)

	EXLOG_LEVEL(7) << "som_map_WriteFile()\n";

	if(in==SOM_MAP.map)
	{
		if(!SOM_MAP.map.finished)
		{
			return SOM_MAP.map.write(at,sz,wr);
		}	
		else return SOM::Tool.WriteFile(SOM_MAP.map.in2,at,sz,wr,o);
	}
	else if(in==SOM_MAP.ezt) 
	{
		return SOM_MAP.ezt.write(at,sz,wr);
	}
	
	return SOM::Tool.WriteFile(in,at,sz,wr,o);
}

BOOL SOM_MAP_ezt::read(LPVOID to, DWORD sz, LPDWORD rd)
{	
	//FYI: this implementation is overly complicated as
	//SOM_MAP loads its header and records all at once!
	//(actually SOM_MAP and som_db read 258048 bytes at
	//the top. Which is 4 shy of the records table. and
	//would seem like a bug, except the 4 are not lost)

	if(recorded>=1024) return SOM::Tool.ReadFile(evt,to,sz,rd,0);

	if(!header) //first things first
	{	
		if(!SOM::Tool.ReadFile(evt,(CHAR*)to,4,rd,0)) return 0;

		header = 1024; if(*(DWORD*)to!=1024||*rd!=4) return 0; 

		if(!read((CHAR*)to+4,sz-4,rd)) return 0;

		*rd+=4; return 1;
	}
	
	if(buffered) //straddles where we last left off
	{
		DWORD cpy = min(sz,sizeof(buffer)-buffered);

		memcpy(to,buffer+buffered,cpy); buffered+=cpy;

		if(buffered==sizeof(buffer)){ recorded++; buffered = 0; }

		if(!buffered&&sz>cpy)
		if(!read((CHAR*)to+cpy,sz-cpy,rd)) return 0;

		*rd+=cpy; return 1;
	}
	else if(!SOM::Tool.ReadFile(evt,to,sz,rd,0)) return 0;
															  
	BYTE *p = (BYTE*)to, *record;
	for(DWORD r=*rd;recorded<1024;recorded++) 
	{	
		if(r<sizeof(buffered))
		{
			memcpy(buffer,p,buffered=r); DWORD dw;
			if(!SOM::Tool.ReadFile(evt,buffer+r,sizeof(buffer)-r,&dw,0)
			||dw!=sizeof(buffer)-r) 
			return 0;
			//Notice: 
			//buffer IS FULL, WHEREAS buffered STRADDLES WHERE WE LEFT OFF
			record = buffer; buffered = r;
		}
		else //unbuffered
		{
			record = p; p+=sizeof(buffer); r-=sizeof(buffer);
		}

		if(recorded<10) //deleted system events
		if(record[CLASS]==0xFF) record[CLASS] = 0xFE;
		if(record[CLASS]!=0xFF) 
		{				
			int n = recorded, c = record[CLASS], p = record[PROTO];

			if(!*record) //ensure no-name events are deleted events				
			{
				char *new_event = (char*)0x48F404;
				strncpy((char*)record,*new_event?new_event:"?",NAMED);
			}

			memcpy(names[recorded],record,NAMED); names[recorded][NAMED] = '\0';

			if(sprintf((char*)record,"@%04d-%02x-%02x",n,c,p)!=11) assert(0);

			//Extended classes (SOM_MAP does not understand these)
			//
			if(recorded<10) //special use
			{
				record[CLASS] = 0xFE; //System
			}
			else if(c>=0xFD) //Systemwide/System (extended)
			{
				record[CLASS] = 0; //NPC
			}
		}
		else *names[recorded] = *record = '\0'; //deletion

		if(record==buffer) //partial record!
		{
			memcpy(p,buffer,r);	return 1;
		}
	}
	return 1;
}
BOOL SOM_MAP_ezt::write(LPCVOID at, DWORD sz, LPDWORD wr)
{	
	//FYI: this implementaiton is overly complicated as
	//SOM_MAP fills its header and records all at once!

	if(recorded>=1024)
	return SOM::Tool.WriteFile(evt,at,sz,wr,0);

	if(!header) //first things first
	{	
		header = 1024; 
		if(*(DWORD*)at!=1024||sz<4) return 0; 
		if(!SOM::Tool.WriteFile(evt,(BYTE*)at,4,wr,0)||*wr!=4) return 0;		
		if(!write((CHAR*)at+4,sz-4,wr)) return 0;
		*wr+=4; return 1;	
	}

	BYTE *p = (BYTE*)at;
	for(DWORD r=sz;recorded<1024;recorded++) 
	{
		if(buffered+r<sizeof(buffer)) //252
		{
			memcpy(buffer+buffered,p,r); buffered+=r; 
			
			*wr = sz; return 1;
		}
		else if(buffered) //remaining portion
		{
			size_t remaining = 
			sizeof(buffer)-buffered;
			memcpy(buffer+buffered,p,remaining);			
			p+=remaining; r-=remaining; buffered = 0;
		}
		else //unbuffered (simplest scenario)
		{
			//using buffer here so not to 
			//clobber SOM_MAP's working copy
			memcpy(buffer,p,sizeof(buffer));
			p+=sizeof(buffer); r-=sizeof(buffer); 
		}
	
		if(buffer[CLASS]!=0xFF) 
		{
			int n, c, p;
			if(3!=sscanf((char*)buffer,"@%d-%x-%x",&n,&c,&p))
			{
				assert(0); return 0; //this one would be on Ex
			}
			//else assert(n==recorded); //legit for copy/paste to fail

			buffer[CLASS] = c; buffer[PROTO] = p;
			memcpy(buffer,names[recorded],NAMED); buffer[NAMED] = '\0';
		}
		else memset(buffer,0x00,NAMED); //deletion		
		
		if(!SOM::Tool.WriteFile(evt,buffer,sizeof(buffer),wr,0)) return 0;		
	}
	if(p!=(BYTE*)at+sz)
	if(!SOM::Tool.WriteFile(evt,p,(BYTE*)at+sz-p,wr,0)) return 0;		
	*wr = sz; return 1;
}

//2022: event comments/map streaming
SOM_MAP_intellisense::evt_descriptor* __cdecl
SOM_MAP_intellisense::evt_code(unsigned short code) //40e530
{
	void *ret; if(code==59) //EXTENSION
	{
		static evt_descriptor *ext = 0; if(!ext) //code3b_t
		{
			if(FindResource(0,MAKEINTRESOURCE(140),RT_DIALOG))
			{
				memset(ext=new evt_descriptor,0x00,sizeof(*ext));
				strcpy(ext->name_etc,som_932_SOM_MAP_load_standby_map);
				ext->code = 59;
				ext->data[0] = 8; //swordofmoonlight_evt_code3b_t
			}
		}
		ret = ext; //assuming 59
	}
	else ret = ((void*(__cdecl*)(DWORD))0x40e530)(code);

	return (evt_descriptor*)ret;
}
SOM_MAP_intellisense::evt_descriptor* __cdecl
SOM_MAP_intellisense::evt_desc(unsigned short desc) //40e570
{
	//REMINDER: som_map_instruct is 39+1 :(
	if(desc>=39) return desc==39?evt_code(59):0; //EXTENSION
	
	return (evt_descriptor*)0x478548+desc;
}
SOM_MAP_intellisense::evt_instruction*
SOM_MAP_intellisense::evt_data(HWND loop, int sel)
{
	if(sel<0) sel = SendDlgItemMessage(loop,1026,LB_GETCURSEL,0,0);

	//need to find the stack memory with the string and length
	auto esi = SOM_MAP_app::CWnd(loop);

	DWORD ecx = *(DWORD*)((char*)esi+0x5c)+0xc;

	//0042B205 8B 4E 5C             mov         ecx,dword ptr [esi+5Ch]  
	//0042B208 BB 0C 00 00 00       mov         ebx,0Ch  
	//0042B20D 57                   push        edi  
	//0042B20E 03 CB                add         ecx,ebx  
	//0042B210 E8 4B A0 03 00       call        00465260 
	if(WORD*eax=((WORD*(__thiscall*)(DWORD,DWORD))0x465260)(ecx,sel))
	{
		//NOTE: presumably there's more data here prior to indirection?
		eax = *(WORD**)(eax+4);
		//2022: doesn't seem to hold
		//142 is ELSE clause (143 is ENDIF)
		//assert(!eax||*(DWORD*)(eax+2)||*eax>=142); //143 too
		assert(eax);
		return (evt_instruction*)eax; 
	}
	return 0;
}
static void SOM_MAP_140(void *lp)
{
	HRSRC found = FindResource(0,MAKEINTRESOURCE(140),RT_DIALOG);
	DLGTEMPLATE *locked = (DLGTEMPLATE*)LockResource(LoadResource(0,found)); 	
	CreateDialogIndirectParamA(0,locked,som_tool_dialog(),som_tool_PseudoModalDialog,(LPARAM)lp);
}
static void SOM_MAP_140_init(HWND hwndDlg) //stack concerns
{
	for(int rd,k,j,i=0;i<64;i++)
	{
		char a[64]; 
		//YUCK: project is set up to modify the
		//map number if it has an MPX extension
		//sprintf(a,SOMEX_(B)"\\data\\map\\%02d.mpx",i);		
		sprintf(a,"map\\%02d.map",i);
		wchar_t fix_me[MAX_PATH];
		if(rd=EX::data(a,fix_me))
		{
			FILE *f = _wfopen(fix_me,L"rb");
			rd = fread(a+8,1,sizeof(a)-8,f)-1;
			for(j=k=8;j<rd;j++) if(a[j]=='\n')			
			for(k=++j;j<rd;j++) if(a[j]=='\n')
			{
				if(a[j-1]=='\r') j--;

				rd = k; a[j--] = '\0'; break;
			}
			if(a[j]) rd = 0; fclose(f);
		}
		sprintf(a,"[%02d] %s",i,rd>0?a+rd:(char*)0x49003d+2);
		SendDlgItemMessageA(hwndDlg,1182,CB_ADDSTRING,0,(WPARAM)a);
	}
}
extern INT_PTR CALLBACK SOM_MAP_140_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
	{
		//REMINDER: windowprocs should miminize stack growth
		SOM_MAP_140_init(hwndDlg); 		
		SendDlgItemMessage(hwndDlg,1186,UDM_SETRANGE32,0,99);
		SendDlgItemMessage(hwndDlg,1187,UDM_SETRANGE32,0,99);
		//REMINDER: evt_code sets sizes for new instructions
		//SOM_MAP_reprogram sets it for the old instructions
		auto data = (swordofmoonlight_evt_code3b_t*)(((BYTE**)lParam)[1]-4);
		SendDlgItemMessage(hwndDlg,1182,CB_SETCURSEL,data->map,0);		
		SetDlgItemInt(hwndDlg,1050,data->setting[0],0);
		SetDlgItemInt(hwndDlg,1051,data->setting[1],0);
		if(data->nosettingmask&2)
		SendDlgItemMessage(hwndDlg,1052,BM_SETCHECK,1,0);
		SetDlgItemFloat(hwndDlg,1053,data->zsetting,L"%.3f");
		if(data->nosettingmask&1)
		PostMessage(GetDlgItem(hwndDlg,1100),BM_CLICK,0,0); //last
		break;
	}
	case WM_COMMAND:

		switch(wParam)
		{
		case 1100:
		{
			//1053? (TOO COMPLICATED)
			int z = SendDlgItemMessage(hwndDlg,1052,BM_GETCHECK,0,0);
			int e = Button_GetCheck((HWND)lParam);
			if(e&&!z) z = 0; 
			if(!z&&!e) z = 1;
			windowsex_enable(hwndDlg,1050,1052+z,e);
			break;
		}
		case 1052:

			//windowsex_enable<1053>(hwndDlg,Button_GetCheck((HWND)lParam));
			EnableWindow(GetDlgItem(hwndDlg,1053),Button_GetCheck((HWND)lParam));
			break;

		case IDOK:
		{
			if(auto*ed=SOM_MAP.evt_data(GetParent(hwndDlg)))
			{
				auto data = (swordofmoonlight_evt_code3b_t*)(ed->data[0]-4); 

				assert(data&&ed->code==59&&ed->size==12);
		
				data->map = SendDlgItemMessage(hwndDlg,1182,CB_GETCURSEL,0,0);
				data->nosettingmask = SendDlgItemMessage(hwndDlg,1100,BM_GETCHECK,0,0);
				data->nosettingmask|=2*SendDlgItemMessage(hwndDlg,1052,BM_GETCHECK,0,0);
				data->setting[0] = (BYTE)GetDlgItemInt(hwndDlg,1050,0,0);
				data->setting[1] = (BYTE)GetDlgItemInt(hwndDlg,1051,0,0);
				(float&)data->zsetting = GetDlgItemFloat(hwndDlg,1053);
			}
			//break;
		}
		case IDCANCEL: return DestroyWindow(hwndDlg);
		}
		break;
	}					  	
	return 0;
}
extern LRESULT CALLBACK SOM_MAP_141_142_subclassproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scID, DWORD_PTR)
{
	switch(uMsg)
	{	
	case WM_COMMAND: //add relative flag to mask (and zindex) 
	
		if(wParam==IDOK) case WM_INITDIALOG: //!
		{		
			//need to find the stack memory with the string and length
			char *ed = (char*)SOM_MAP_app::CWnd(hWnd)+88;
			//som_map_instruct
			if(scID==18) ed+=4; //61 is a subset of 60
			auto data = (swordofmoonlight_evt_code3d_t*)ed;
			if(uMsg==WM_INITDIALOG)
			{
				auto ret = DefSubclassProc(hWnd,uMsg,wParam,lParam);

				if(scID==18) for(int j,i=2;i-->0;)
				{
					SendDlgItemMessage(hWnd,1180+i,CB_SETITEMDATA,j=
					SendDlgItemMessageA(hWnd,1180+i,CB_ADDSTRING,0,(LPARAM)som_932_SOM_MAP_blend[i]),17+i);
					if(ed[2+i]==17+i)
					SendDlgItemMessage(hWnd,1180+i,CB_SETCURSEL,j,0);
				}

				if(16&data->settingmask)
				SendDlgItemMessage(hWnd,1101,BM_SETCHECK,1,0);
				SendDlgItemMessage(hWnd,1005,TBM_SETRANGE,0,MAKELPARAM(0,7));
				SendDlgItemMessage(hWnd,1005,TBM_SETPOS,1,data->zindex);

				return ret;
			}
			else //IDOK
			{
				if(SendDlgItemMessage(hWnd,1101,BM_GETCHECK,0,0))
				data->settingmask|=16;
				else data->settingmask&=~16;
				data->zindex = SendDlgItemMessage(hWnd,1005,TBM_GETPOS,0,0);
			}
		}
		break;	
	
	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,SOM_MAP_141_142_subclassproc,scID);
		break;
	}

	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
extern LRESULT CALLBACK SOM_MAP_168_172_subclassproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scID, DWORD_PTR)
{
	switch(uMsg)
	{	
	case WM_COMMAND: //add zindex
		
		//these are 42b1b6 stack addresses
		//25: 0x00198268 0x001982c8 = 0x60
		//26: 0x00198574 0x001985e8 = 0x74
		//102: 0x00198268 0x001982c4 = 0x5C;
		if(wParam==IDOK) case WM_INITDIALOG: //!
		{
			BYTE *zi; if(scID==27) //extended memory?
			{
				auto *ed = SOM_MAP.evt_data(GetParent(hWnd));
				zi = (BYTE*)ed->data[0]+24;

				//TODO: som_db.exe treats these as unsigned but it would
				//be good to be able to rotate objects in reverse. these
				//aren't going negative but SOM_MAP might have code that
				//is preventing it (have to dig deeper)
				//SendDlgItemMessage(hWnd,1191,UDM_SETRANGE32,-360,360);
				//SendDlgItemMessage(hWnd,1137,UDM_SETRANGE32,-360,360);
				//SendDlgItemMessage(hWnd,1138,UDM_SETRANGE32,-360,360);
			}
			else
			{
				zi = (BYTE*)SOM_MAP_app::CWnd(hWnd);
				switch(scID) //som_map_instruct
				{
				case 7: zi+=0x60+6; break; //evt::code19_t //25
				case 8: zi+=0x74+6; break; //evt::code1a_t //26
				//YUCK: extended bytes not stored on stack
				//case 27: zi+=0x5c+24; break; //evt::code66_t //102
				default: assert(0); break;
				}
			}			
			if(uMsg==WM_INITDIALOG)
			{
				SendDlgItemMessage(hWnd,1005,TBM_SETRANGE,0,MAKELPARAM(0,7));
				SendDlgItemMessage(hWnd,1005,TBM_SETPOS,1,*zi);
			}
			else //IDOK
			{
				*zi = SendDlgItemMessage(hWnd,1005,TBM_GETPOS,0,0);
			}
		}
		break;

	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,SOM_MAP_168_172_subclassproc,scID);
		break;
	}

	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}

static HBITMAP SOM_MAP_arrows[4] = {};
void SOM_MAP_arrows_init()
{
	BITMAPINFO bmi = {sizeof(BITMAPINFOHEADER),21,21,1,32,BI_RGB,21*21*4};
	auto hil = ImageList_LoadImageW((HINSTANCE)0x400000,MAKEINTRESOURCEW(169),21,4,0,0,LR_CREATEDIBSECTION);
	for(int i=4;i-->0;)
	{
		VOID *pvBits;
		SOM_MAP_arrows[i] = CreateDIBSection(som_tool_dc,&bmi,DIB_RGB_COLORS,&pvBits,NULL,0);
		auto so = SelectObject(som_tool_dc,SOM_MAP_arrows[i]);
		ImageList_Draw(hil,i,som_tool_dc,0,0,0);
		SelectObject(som_tool_dc,so);

		BYTE *p = (BYTE*)pvBits;
		for(int i=21*21;i-->0;p+=4) //assuming monochrome
		{
			//gets artifacted by AlphaBlend???
			//double d = pow(p[0]/255.0,2)*255; 
			//p[3] = (BYTE)min(d,128);
			p[3] = p[0]>15?min(255,p[0]*5):0; //15 is removing darkening
		}
	}
	ImageList_Destroy(hil);
}

static HBITMAP WINAPI SOM_MAP_CreateCompatibleBitmap(HDC hdc, int cx, int cy)
{
	BITMAPINFO bmi = {sizeof(BITMAPINFOHEADER),cx,cy,1,32,BI_RGB,cx*cy*4}; //24 3
	VOID *pvBits;
	HBITMAP ret = CreateDIBSection(hdc,&bmi,DIB_RGB_COLORS,&pvBits,NULL,0); //som_tool_dc
	assert(ret); //MEMORY LEAK?

	//if(0&&EX::debug) memset(pvBits,0x00,cx*cy*4);

	//inverting the alpha channel makes this unnecessary
	//if(cx<cy) //nop gray clear
	{
	//	DWORD gray = 0xff595959; for(int i=cx*cy;i-->0;) ((DWORD*)pvBits)[i] = gray;
	}
	return ret;
}
namespace SOM_MAP_icons
{ 
	struct float4
	{
		union{ float x,r,h; };
		union{ float y,g,s,c; };
		union{ float z,b,v,l; };
		union{ float w,a; };
		float4(float x, float y, float z, float a=1):x(x),y(y),z(z),a(a){}
		float4(BYTE *c):x(c[2]/255.0f),y(c[1]/255.0f),z(c[0]/255.0f),a(1)
		{
			r = linear(r); g = linear(g); b = linear(b);
		}
		float &operator[](int i){ return (&x)[i]; }

		float linear(float o) //TESTING
		{
			//return o*(o*(o*0.305306011+0.682171111)+0.012522878);
			return o;
		}
		float sRGB(float o)
		{
			//return max(0,1.055*powf(o,0.416666667)-0.055);
			return o;
		}
	};

	//https://forums.getpaint.net/topic/123095-hue-math-formula-it-matches-microsofts-but-not-others-what-is-it/
	float4 HSVtoRGB(float4 HSV)
	{
		float H = HSV.h*360;	
    
		// The color wheel consists of 6 sectors.
		// Figure out which sector you're in.
		double sectorPos = H/60.0;
		int sectorNumber = (int)(floor(sectorPos));

		// get the fractional part of the sector.
		// That is, how many degrees into the sector
		// are you?
		double fractionalSector = sectorPos - sectorNumber;

		// Calculate values for the three axes
		// of the color. 
		float p = HSV.v*(1-HSV.s);
		float q = HSV.v*(1-(HSV.s*fractionalSector));
		float t = HSV.v*(1-(HSV.s*(1-fractionalSector)));

		// Assign the fractional colors to r, g, and b
		// based on the sector the angle is in.
		float r,g,b;
		switch(sectorNumber)
		{
		default:
		case 0:	r = HSV.v; g = t; b = p; break;
		case 1: r = q; g = HSV.v; b = p; break;
		case 2: r = p; g = HSV.v; b = t; break;
		case 3: r = p; g = q; b = HSV.v; break;
		case 4: r = t; g = p; b = HSV.v; break;
		case 5: r = HSV.v; g = p; b = q; break;
		}

		return float4(r,g,b);
	}
	float4 RGBtoHSV(float4 RGB)
	{
		float &r = RGB.r;
		float &g = RGB.g;
		float &b = RGB.b;
		
		float min,max,delta;
		min = min(min(r,g),b);
		max = max(max(r,g),b);
		delta = max-min;
		
		float h,s,v = max;

		if(max==0||delta==0)
		{
			// R, G, and B must be 0, or all the same.
			// In this case, S is 0, and H is undefined.
			// Using H = 0 is as good as any...
			s = 0;
			h = 0;
		}
		else
		{
			s = delta/max;

			if(r==max) // Between Yellow and Magenta
			{				
				h = (g-b)/delta;
			}
			else if(g==max) // Between Cyan and Yellow
			{				
				h = 2+(b-r)/delta;
			}
			else // Between Magenta and Cyan
			{				
				h = 4+(r-g)/delta;
			}

		}
		// Scale h to be between 0 and 360. 
		// This may require adding 360, if the value
		// is negative.
		h*=60; if(h<0) h+=360;

		return float4(h/360,s,v);
	}
}
extern int SOM_MAP_icons_hue(HBITMAP hb, int cc)
{
	/*ROYGBIV
	case 1: cr = 0x752534; break; //red	(320/300) (0666)
	case 2: cr = 0x68372C; break; //orange (20/360)
	case 3: cr = 0x66771D; break; //yellow (70/50)
	case 4: cr = 0x38752D; break; //green (110/90) (grass)
	case 5: cr = 0x34615A; break; //blue (170/150)
	case 6: cr = 0x214873; break; //indigo (210/190)
	case 7: cr = 0x443461; break; //violet (260/240)
	*/
	int dst,sat,val,s2,v2; switch(cc) //NOTE: +/- are tweaks
	{
	case '!': dst = 321; sat = 63; val = 43; s2 = 66; v2 = 70; break; //1
	case '@': dst = 11; sat = 57; val = 40; s2 = 57+4; v2 = 65+2; break; //2
	case '#': dst = 71; sat = 75; val = 46; s2 = 75; v2 = 74; break; //3
	case '$': dst = 110; sat = 82; val = 49; s2 = 82-4; v2 = 77-2; break; //4
	case '%': dst = 170; sat = 46; val = 38; s2 = 46+2; v2 = 60; break; //5
	case '^': dst = 211; sat = 71; val = 45; s2 = 71-2; v2 = 72-1; break; //6
	case '?': dst = 261; sat = 46; val = 38; s2 = 46+4; v2 = 60+4; break;  //7
	default: dst = sat = val = s2 = v2 = 0; //return 0; //auto-hue?
	}
	int dst2 = (360+dst-20)%360;
	
	WORD h,l,s;

	BITMAP bm; GetObject(hb,sizeof(BITMAP),&bm);
	
	int bpp = bm.bmBitsPixel/8;

	//something like this is needed for orange
	//tiles since they straddle the wraparound
	stdext::hash_map<WORD,WORD> most;

	union{ DWORD cr; BYTE cb[3]; };

	float half = 0; //0.5f

	BYTE *pp = (BYTE*)bm.bmBits;
	for(int i=20;i-->0;pp+=bm.bmWidthBytes)
	{
		BYTE *p = pp; for(int j=20;j-->0;p+=bpp)
		{
			cr = *(DWORD*)p; if(cr&0xffffff)
			{
			//	std::swap(cb[0],cb[2]); //???
				//ColorRGBToHLS(0xffffff&cr,&h,&l,&s);
				h = 360*SOM_MAP_icons::RGBtoHSV(cb).h+half;
				most[h]++;
			}
			if(bpp==4) *(DWORD*)p|=0xff000000; //HACK
		}
	}

	//float x = 240/360.0f; //ColorRGBToHLS
	//float l_x = 360/240.0f; //1/x
	float x = 1, l_x = 1;

	int ret = 0;
	int cmp = 0, best = 0;
	for(auto it=most.begin();it!=most.end();it++)	
	if(it->second>cmp)
	{
		cmp = it->second; best = it->first; 
	}
	int src = (int)(best*l_x+half);
	if(src>=330||src<40)
	{
		ret = 2;
		src = 11; sat-=57; val-=40; s2-=57; v2-=65; //orange?
	}
	else if(src<80) 
	{
		ret = 3;
		src = 71; sat-=75; val-=46; s2-=75; v2-=74; //yellow?
	}
	else if(src<140)
	{
		ret = 4;
		src = 110; sat-=82; val-=49; s2-=82; v2-=77; //green?
	}
	else if(src<180) 
	{
		ret = 5;
		src = 170; sat-=46; val-=38; s2-=46; v2-=60; //blue?
	}
	else if(src<230)
	{
		ret = 6;
		src = 211; sat-=71; val-=45; s2-=71; v2-=72; //indigo?
	}
	else if(src<280)
	{
		ret = 7;
		src = 261; sat-=46; val-=38; s2-=46; v2-=60; //violet?
	}
	else
	{
		ret = 1;
		src = 320; sat-=63; val-=43; s2-=66; v2-=70; //red?
	}
	if(!dst) return ret;

	int src2 = (360+src-20)%360;

	int del = x*(dst-src)+half, del2 = x*(dst2-src2)+half;

	int cutoff = x*(src2)+half; //ARBITRARY
			
	pp = (BYTE*)bm.bmBits; if(!del&&x!=1) //LOSSY???
	{
		//FIX ME: ColorRGBToHLS!=ColorHLSToRGB
	}
	else for(int i=20;i-->0;pp+=bm.bmWidthBytes)
	{
		BYTE *p = pp; for(int j=20;j-->0;p+=bpp)
		{
			cr = *(DWORD*)p;
			if(cr&0xffffff) 
			{
			//	std::swap(cb[0],cb[2]); //???
			//	ColorRGBToHLS(cr,&h,&l,&s);
				auto c = SOM_MAP_icons::RGBtoHSV(cb);
				h = (WORD)(360*c.h+half);

				int hh; if(fabsf(h-cutoff)<10) //ARIBTRARY (20/2)
				{
					hh = h+del2;

					c.s+=s2/100.0f; c.v+=v2/100.0f;
				}
				else
				{
					hh = h+del;

					c.s+=sat/100.0f; c.v+=val/100.0f; 
				}
			//	h = (240+hh)%240;
				c.h = (360+hh)%360/360.0f;				
				c.s = max(0,min(1,c.s));
				c.v = max(0,min(1,c.v));

			//	cr = ColorHLSToRGB(h,l,s?s:1); //BUG (0 turns black)
				c = SOM_MAP_icons::HSVtoRGB(c);
				cb[0] = (BYTE)(c.sRGB(c.r)*255);
				cb[1] = (BYTE)(c.sRGB(c.g)*255);
				cb[2] = (BYTE)(c.sRGB(c.b)*255);

				if(bpp==3)
				{
					p[0] = cb[2];
					p[1] = cb[1];
					p[2] = cb[0];
				}
				else *(DWORD*)p = cr|0xff000000; //HACK
			}
		}
	}

	return ret;
}

 //REPROGRAMMING //REPROGRAMMING //REPROGRAMMING //REPROGRAMMING

static void SOM_MAP_InvertRgn_InvertRgn(RECT r, BITMAP &b)
{
	r.right++; r.bottom++; //20->21

	r.left = max(r.left,0);
	r.top = max(r.top,0);

	r.right = min(r.right,b.bmWidth);
	r.bottom = min(r.bottom,b.bmHeight);

	BYTE *pp = (BYTE*)b.bmBits; //SOM_MAP_CreateCompatibleBitmap

	pp+=(b.bmHeight-1)*b.bmWidthBytes; //upside-down?
	pp-=r.top*b.bmWidthBytes;

	for(int i=r.bottom-r.top;i-->0;pp-=b.bmWidthBytes)
	{
		BYTE *p = pp+r.left*4;
		
		for(int j=r.right-r.left;j-->0;p+=4)
		{
			p[0] = 255-p[0];
			p[1] = 255-p[1];
			p[2] = 255-p[2];
		}
	}	
}
static BOOL WINAPI SOM_MAP_InvertRgn(HDC dc, HRGN _=0)
{
	if(0) return InvertRgn(dc,_);

	char *cw = (char*)SOM_MAP_app::CWnd(SOM_MAP.painting); 
	bool sel = *(int*)(cw+0x60)!=0;

	RECT sr = //selection region //444753
	{
		*(int*)(cw+0x88)-*(int*)(cw+0x70),
		*(int*)(cw+0x8c)-*(int*)(cw+0x74),
		*(int*)(cw+0x90)-*(int*)(cw+0x70),
		*(int*)(cw+0x94)-*(int*)(cw+0x74)
	};
	RECT rc = //HRGN
	{
		sr.left*21,sr.top*21,sr.right*21+22,sr.bottom*21+22
	};
	auto hb = (HBITMAP)GetCurrentObject(dc,OBJ_BITMAP);
	BITMAP bm; GetObject(hb,sizeof(BITMAP),&bm);
		
	bool shift_click = sel&&0==sr.right-sr.left&&0==sr.bottom-sr.top&&~GetKeyState(VK_LBUTTON)>>15;
	bool checkpoints = !shift_click&&(SOM_MAP_ini&2)!=0&&som_map_tab!=1220;

	if(sel&&!shift_click) //draw selection region
	{
		SOM_MAP_InvertRgn_InvertRgn(rc,bm); //alpha?
	}

	if(shift_click||checkpoints)
	{		
		//cursor moves when dragged
		//int xy = *(int*)(cw+0x78)+100*(99-*(int*)(cw+0x7c));
		int xy = *(int*)(cw+0x88)+(99-*(int*)(cw+0x8c))*100;

		auto prt = !shift_click?-1:SOM_MAP_4921ac.grid[xy].part;
		
		int x = *(int*)(cw+0x70); 
		int y = 99-*(int*)(cw+0x74);
		int w = *(int*)(cw+0x80);
		int h = *(int*)(cw+0x84);

		auto *p = SOM_MAP_4921ac.grid;		
		auto *pp = p+100*y+x;
		for(int i=0;i<h;i++,pp-=100)
		if(pp>=SOM_MAP_4921ac.grid) //YUCK
		{
			p = pp;
			for(int j=0;j<w;j++,p++)	
			{
				if(checkpoints&&isupper(p->ev)) //2023
				if(HWND next=som_tool_x(GetDlgItem(som_tool,1222)))
				{
					static WCHAR x[32];
					int xlen = GetWindowTextW(next,x,32);
					wchar_t *n2 = som_tool_nth(x,xlen,2,0);
					wchar_t *n3 = som_tool_nth(x,xlen,3,0);
					
					SetBkMode(dc,TRANSPARENT);
					SetTextColor(dc,0x000000);
					int l = 1+j*21+1;
					int t = 1+i*21+1;
					SetRect(&rc,l,t,l+20,t+20);
					DrawTextW(dc,n2,1,&rc,DT_CENTER|DT_VCENTER);
					SetTextColor(dc,0xffffff);
					DrawTextW(dc,n3,1,&rc,DT_CENTER|DT_VCENTER);
				}

				if(prt==p->part||tolower(p->ev)=='v'&&checkpoints)
				{
					int l = 1+j*21;
					int t = 1+i*21;
					//SetRectRgn(r,l,t,l+20,t+20);
					SetRect(&rc,l,t,l+20,t+20);
					SOM_MAP_InvertRgn_InvertRgn(rc,bm); //alpha?
				}
			}
		}
	}	

	return TRUE;
}

//2021: enhanced view for gray grids?
static BYTE(*map_40f9d0_lut)[96] = 0;
extern int SOM_MAP_gen_icons = 0;
extern bool SOM_MAP_expand_icons = false;
static HBITMAP SOM_MAP_icon_bitmap = SOM_MAP_CreateCompatibleBitmap(GetDC(0),25,25);
#pragma optimize("",off) //&dst+0xa8
static BOOL __stdcall SOM_MAP_PlgBlt_icons(HDC dst, int x, int y, int w, int h, HDC src, int xsrc, int ysrc, int wsrc, int hsrc, DWORD rop_or_spin)
{
	//TODO: maybe do all this manually?

	int sz = SOM_MAP_gen_icons; 

	int xx = x, yy = y; HDC d = dst;
	
	extern HDC som_tool_dc2; if(sz==25) //border? 
	{
		if(!som_tool_dc2)
		{
			som_tool_dc2 = CreateCompatibleDC(0);
			SelectObject(som_tool_dc2,SOM_MAP_CreateCompatibleBitmap(GetDC(0),25,25));
		}
		x = y = 0; d = som_tool_dc2;
	}

	//PlgBlt is used for sideways tiles so that a second set of
	//tiles can be removed (actually repurposed for gray tiles)

	//int b = sz==25?2:0; sz-=b*2; //border?

	w = h = sz;

	if(rop_or_spin==0xcc0020) //internal or external?
	{
		auto tile = (SOM_MAP_4921ac::Tile*)((BYTE*)&dst+0xa8); //esp+a8
		rop_or_spin = tile->spin;
	}
	/*this draws 25-b*2
	switch(rop_or_spin)
	{
	case 0: xsrc = ysrc = b; wsrc = hsrc = sz; break;
	case 1: xsrc = ysrc = b; wsrc = hsrc = sz; goto plg; 
	case 2: xsrc = ysrc = sz+b-1; wsrc = hsrc = -sz; break; //N
	//NOTE: ysrc is 19? which doesn't work for PlgBlt
	case 3: ysrc = xsrc = sz+b; wsrc = hsrc = -sz; goto plg; //sz-1
	}*/
	switch(rop_or_spin)
	{
	case 0: xsrc = ysrc = 0; wsrc = hsrc = sz; break;
	case 1: xsrc = ysrc = 0; wsrc = hsrc = sz; goto plg; 
	case 2: xsrc = ysrc = sz-1; wsrc = hsrc = -sz; break; //N
	//NOTE: ysrc is 19? which doesn't work for PlgBlt
	case 3: ysrc = xsrc = sz; wsrc = hsrc = -sz; goto plg; //sz-1
	}

	SOM::Tool.StretchBlt(d,x,y,w,h,src,xsrc,ysrc,wsrc,hsrc,0xcc0020); //rop

	goto arrow;
	
	plg: POINT plg[3] = {{x,y+h},{x,y},{x+w,y+h}}; 

	PlgBlt(d,plg,src,xsrc,ysrc,wsrc,hsrc,0,0,0);

	arrow: //TODO: move to SOM_MAP_BitBlt_grid?

	int arrow = SOM_MAP_alpha[0];
	if(arrow>0&&SOM_MAP_ini&4)
	{
		arrow = arrow*1.5f;
		
		if(!SOM_MAP_arrows[0]) SOM_MAP_arrows_init();
	}
	else arrow = 0;

	if(sz==25) //fill in border region?
	{
		HBITMAP hb = (HBITMAP)GetCurrentObject(dst,OBJ_BITMAP);
		HBITMAP hb2 = (HBITMAP)GetCurrentObject(som_tool_dc2,OBJ_BITMAP);

		BITMAP bm; GetObject(hb,sizeof(BITMAP),&bm);
		BITMAP bm2; GetObject(hb2,sizeof(BITMAP),&bm2);

		BYTE *pp = (BYTE*)bm2.bmBits;
		BYTE *qq = (BYTE*)bm.bmBits; //SOM_MAP_CreateCompatibleBitmap

		pp+=25*4*(25-1); //upside-down :(
		qq+=bm.bmWidthBytes*(bm.bmHeight-1); 

		qq+=xx*4-yy*bm.bmWidthBytes;

		w = 25; h = 25; //REPURPOSING

		if(xx<=1){ pp+=2*4; w-=2; }
		else{ x+=2; qq-=2*4; }
		if(xx+w>bm.bmWidth) w = bm.bmWidth-xx;
		if(yy<=1){ pp-=2*25*4; h-=2; }
		else{ y+=2; qq+=2*bm.bmWidthBytes; }
		if(yy+h>=bm.bmHeight) h = bm.bmHeight-yy;

		auto qqq = qq; //HACK

		xx = x+21; yy = y+21; //REPURPOSING

		for(int i=0;i<h;i++,qq-=bm.bmWidthBytes,pp-=25*4)
		{	
			BYTE *p = pp, *q = qq;
			for(int j=0;j<w;j++,q+=4,p+=4)
			{
				if(j>=x&&j<xx&&i>=y&&i<yy)
				{
					q[0] = p[0];
					q[1] = p[1];
					q[2] = p[2];
					q[3] = p[3]?max(q[3],p[3]):0; //wall?
				}
				else if(q[3]||j>=xx||i>=yy)
				{					
					q[3] = max(q[3],p[3]);
				}
			}
		}

		if(arrow) //arrow?
		{
			//NOTE: AlphaBlend affects alpha channel

			GetObject(SOM_MAP_arrows[rop_or_spin],sizeof(BITMAP),&bm2);

			pp = (BYTE*)bm2.bmBits+21*4*(21-1); //upside-down :(
			qq = qqq;

			float l_255 = 1/255.0f;
			float aa = arrow*l_255;
			aa = sqrtf(aa);
			for(int i=0;i<h;i++,qq-=bm.bmWidthBytes)
			{	
				if(i>=y&&i<yy)
				{
					BYTE *p = pp, *q = qq;
					for(int j=0;j<w;j++,q+=4)
					{
						if(j>=x&&j<xx)
						{
							float a = aa*p[3]*l_255;
							float l_a = 1-a;
							q[0] = (q[0]*l_a+p[0]*a);
							q[1] = (q[1]*l_a+p[1]*a);
							q[2] = (q[2]*l_a+p[2]*a);
							p+=4;
						}
					}
					pp-=21*4;
				}
			}
		}
	}
	else if(arrow) 
	{
		BLENDFUNCTION bf = {AC_SRC_OVER,0,(BYTE)arrow,AC_SRC_ALPHA};			
		int two = sz==25?2:0;
		HGDIOBJ so2 = SelectObject(src,SOM_MAP_arrows[rop_or_spin]);
		AlphaBlend(d,x+two,y+two,21,21,src,0,0,21,21,bf);
		SelectObject(src,so2);
	}

	return 1;
}
#pragma optimize("",on) 
static BOOL __stdcall SOM_MAP_TransparentBlt_grays(HDC dst, int x, int y, int w, int h, HDC src, int xsrc, int ysrc, DWORD rop)
{
	return TransparentBlt(dst,x,y,w,h,src,xsrc,ysrc,w,h,0x404040); 
}
static HBITMAP SOM_MAP_grid = 0;
static std::vector<WORD> SOM_MAP_my_plt;
static std::pair<WORD,WORD> SOM_MAP_drag_palette_sel(~0,~0);
static void SOM_MAP_PlgBlt_palette(HDC dst, int x, int y, int w, int h, HDC src, int xsrc, int ysrc, int wsrc, int hsrc, DWORD spin)
{
	//TODO: maybe do all this manually?

	int sz = SOM_MAP_gen_icons;

	//PlgBlt is used for sideways tiles so that a second set of
	//tiles can be removed (actually repurposed for gray tiles)

	int b = sz==25?2:0; sz-=b*2; //border?

	w = h = sz;

	switch(spin)
	{
	case 0: xsrc = ysrc = b; wsrc = hsrc = sz; break;
	case 1: xsrc = ysrc = b; wsrc = hsrc = sz; goto plg; 
	case 2: xsrc = ysrc = sz+b-1; wsrc = hsrc = -sz; break; //N
	//NOTE: ysrc is 19? which doesn't work for PlgBlt
	case 3: ysrc = xsrc = sz+b; wsrc = hsrc = -sz; goto plg; //sz-1
	}

	SOM::Tool.StretchBlt(dst,x,y,w,h,src,xsrc,ysrc,wsrc,hsrc,0xcc0020); //rop	
	return;

	plg: POINT plg[3] = {{x,y+h},{x,y},{x+w,y+h}}; 

	PlgBlt(dst,plg,src,xsrc,ysrc,wsrc,hsrc,0,0,0);
}
static BOOL WINAPI SOM_MAP_ImageList_Draw_palette(HIMAGELIST himl, int i, HDC hdcDst, int x, int y, UINT fStyle)
{
	int *vp = (int*)SOM_MAP_app::CWnd(SOM_MAP.palette);
	int w = vp[0x54/4];

	int xx = x/21+vp[0x40/4], yy = y/21+vp[0x44/4];	
	int part = yy*w+xx;

	auto &sel = SOM_MAP_drag_palette_sel;
	bool inv = false;
	if(sel.second!=65535)		
	if(part>=sel.first&&part<=sel.second
	 ||part<=sel.first&&part>=sel.second)
	{
		inv = true;
	}

	int spin = SOM_MAP_4921ac.parts[SOM_MAP_my_plt[part]].rot;

	int gi = SOM_MAP_gen_icons;
	int sq = SOM_MAP_expand_icons?21:20;
	if(25==gi||sq==21)
	{
		x+=x/21; y+=y/21; //20->21

	//	IntersectClipRect(hdcDst,x,y,x+sq,y+sq); //PlgBlt?
		
	//	if(25==gi){ x-=2; y-=2; }
	}

	//BOOL ret = ImageList_Draw(himl,i,hdcDst,x,y,fStyle);
	{
		HDC dc2 = som_tool_dc;
		auto swap = SelectObject(dc2,SOM_MAP_icon_bitmap);	
		ImageList_Draw(himl,i,dc2,0,0,0);
		//HACK: just draw dark highlight on bottom/right side
		if(sq==21&&gi!=25)
		{
			SOM::Tool.BitBlt(hdcDst,x,y,21,21,dc2,0,0,0xcc0020);		
			if(0==spin) goto opt;
		}
		SOM_MAP_PlgBlt_palette(hdcDst,x,y,20,20,dc2,19,19,-20,-20,spin);
	opt:SelectObject(dc2,swap);
	}
	
	if(SOM_MAP_ini&4) //arrow?
	{
		if(!SOM_MAP_arrows[0]) SOM_MAP_arrows_init();

		int alpha = SOM_MAP_alpha[0]; if(alpha>0)
		{
			float l_255 = 1/255.0f;
			float aa = alpha*l_255; aa = sqrtf(aa);

			/*affects the alpha channel???
			BLENDFUNCTION bf = {AC_SRC_OVER,0,(BYTE)alpha,AC_SRC_ALPHA};
			HGDIOBJ so2 = SelectObject(src,SOM_MAP_arrows[spin]);
			AlphaBlend(hdcDst,x,y,21,21,src,0,0,21,21,bf);
			SelectObject(src,so2);*/
			{			
				HBITMAP hb = (HBITMAP)GetCurrentObject(hdcDst,OBJ_BITMAP);
				HBITMAP hb2 = SOM_MAP_arrows[spin];

				BITMAP bm; GetObject(hb,sizeof(BITMAP),&bm);
				BITMAP bm2; GetObject(hb2,sizeof(BITMAP),&bm2);

				BYTE *pp = (BYTE*)bm2.bmBits;
				BYTE *qq = (BYTE*)bm.bmBits; //SOM_MAP_CreateCompatibleBitmap

				pp+=21*4*(21-1); //upside-down :(
				qq+=bm.bmWidthBytes*(bm.bmHeight-1); 

				qq+=x*4-y*bm.bmWidthBytes;

				int w = 21, h = 21;
				if(x+w>bm.bmWidth) w = bm.bmWidth-x;
				if(y+h>=bm.bmHeight) h = bm.bmHeight-y;

				int xx = x+21, yy = y+21; 
				for(int i=0;i<h;i++,qq-=bm.bmWidthBytes)
				{	
					BYTE *p = pp, *q = qq;
					for(int j=0;j<w;j++,q+=4)
					{							
						float a = aa*p[3]*l_255;
						float l_a = 1-a;
						q[0] = (q[0]*l_a+p[0]*a);
						q[1] = (q[1]*l_a+p[1]*a);
						q[2] = (q[2]*l_a+p[2]*a);
						p+=4;							
					}
					pp-=21*4;
				}
			}
		}
	}

	if(inv)
	{
	//	if(25==gi){ x+=2; y+=2; }

		RECT rc = {x,y,x+21,y+21};
		HBITMAP b = (HBITMAP)GetCurrentObject(hdcDst,OBJ_BITMAP);
		BITMAP bm; GetObject(b,sizeof(BITMAP),&bm);
		SOM_MAP_InvertRgn_InvertRgn(rc,bm);
	}

	//SelectClipRgn(hdcDst,0); //PlgBlt?
	
	return 1; //return ret;
}
static BOOL WINAPI SOM_MAP_OffsetRect_palette(LPRECT lprc, int dx, int dy)
{
	if(SOM_MAP_expand_icons)
	{
		dx+=dx/21; dy+=dy/21; //20->21
	}

	OffsetRect(lprc,dx,dy);

	if(SOM_MAP_expand_icons)
	{
		lprc->right++; lprc->bottom++; //20->21
	}
	
	return 1;
}
static void SOM_MAP_draw_grid(HDC dc, int i, int n, int e)
{
	POINT pts[2*101]; DWORD lns[101];

	for(int j=n;j-->0;) lns[j] = 2; e*=21;

	for(int j=0,ln=0,jn=n*21;j<jn;j+=21,ln+=2)
	{
		auto *pt = pts+ln; 
		
		(i?pt[0].x:pt[0].y) = 0; (i?pt[0].y:pt[0].x) = j;
		(i?pt[1].x:pt[1].y) = e; (i?pt[1].y:pt[1].x) = j;
	}
		
	PolyPolyline(dc,pts,lns,n);
}
static BOOL WINAPI SOM_MAP_BitBlt_grid(HDC dst, int xdst, int ydst, int wdst, int hdst, HDC src, int xsrc, int ysrc, DWORD rop)
{
	int w = wdst/21+1, h = hdst/21+1;

	//draw selection region in other views
	if(w>h&&som_map_tab!=1215) SOM_MAP_InvertRgn(src);

	HBITMAP b = (HBITMAP)GetCurrentObject(src,OBJ_BITMAP);

	BITMAP bm; GetObject(b,sizeof(BITMAP),&bm);

	if(25==SOM_MAP_gen_icons)
	{
		BYTE *pp = (BYTE*)bm.bmBits; //SOM_MAP_CreateCompatibleBitmap

		for(int i=hdst;i-->0;pp+=bm.bmWidthBytes)
		{
			BYTE *p = pp; for(int j=wdst;j-->0;p+=4)
			{
				//NOTE: this is the alpha channel in the art icons
				//HACK! map_40f9d0 inverts it
				float a = 1-p[3]/255.0f;
				p[0]*=a;
				p[1]*=a;
				p[2]*=a;
				p[3] = 255;
			}
		}
	}
 
	if(w>h&&SOM_MAP_overlay_BMP) 
	{
		float a = (128+SOM_MAP_alpha[1])/128.0f;
		float l_a = 1-a;
		if(a<1) if(BYTE*pp=SOM_MAP_overlay_ambient2)
		{
			char *cw = (char*)SOM_MAP_app::CWnd(SOM_MAP.painting);
			int x = *(int*)(cw+0x70), y = 99-*(int*)(cw+0x74);

			BYTE *qq = (BYTE*)bm.bmBits; //SOM_MAP_CreateCompatibleBitmap

			//qq+=3+bm.bmWidthBytes; //grid

			qq+=(hdst-1)*bm.bmWidthBytes;

			pp+=3*(y*100+x); 
			for(int i=hdst;i-->0;qq-=bm.bmWidthBytes)
			{
				BYTE *d = qq+bm.bmWidthBytes;

				if(i%21==0) pp-=100*3;

				auto *q = qq;
				auto *p = pp; for(int j=w;j-->0;p+=3)
				{
					float b = a+l_a*p[0]/255.0f;
					float g = a+l_a*p[1]/255.0f;
					float r = a+l_a*p[2]/255.0f;
					for(int l=21;l-->0&&q<d;q+=4)
					{
						q[0]*=b; q[1]*=g; q[2]*=r;
					}
				}
			}
		}

		//2023: can just do this here now since redrawing focus rect down below
		SOM_MAP_sandwich_overlay(src);
	}

	#define GEN_ICONS_SZ 25 //21
	if(w>h) 
	if(SOM_MAP_gen_icons==GEN_ICONS_SZ)
	{
		if(!SOM_MAP_grid)
		{
			//SOM_MAP_grid = CreateCompatibleBitmap(som_tool_dc,wdst,hdst);
			BITMAPINFO bmi = {sizeof(BITMAPINFOHEADER),wdst,hdst,1,32,BI_RGB,wdst*hdst*4};
			VOID *pvBits;
			SOM_MAP_grid = CreateDIBSection(som_tool_dc,&bmi,DIB_RGB_COLORS,&pvBits,NULL,0);
			memset(pvBits,0x80,bmi.bmiHeader.biSizeImage);
			HGDIOBJ so = SelectObject(som_tool_dc,SOM_MAP_grid);			

			//??? this just fills in black, with or without alpha
			HGDIOBJ so2 = SelectObject(som_tool_dc,GetStockObject(DC_PEN)); //p
			SetDCPenColor(som_tool_dc,0x999999);
							
				SOM_MAP_draw_grid(som_tool_dc,0,w,h); SOM_MAP_draw_grid(som_tool_dc,1,h,w);

				SelectObject(som_tool_dc,so2); SelectObject(som_tool_dc,so);
			
			for(auto*p=(DWORD*)pvBits,*q=p+bmi.bmiHeader.biSizeImage/4;p<q;p++)
			{
			//	*p = *p==0x80808080?0:0x80999999; //only 0x80 works without color artifacts???
				*p = *p==0x80808080?0:0x70666666; //16-bit?
			}
		}

		int alpha = SOM_MAP_alpha[0];
		if(alpha<0) alpha = 0xff+alpha*2; else alpha = 0xff;
		if(alpha)
		{
			alpha = 255*sqrtf(alpha/255.0f);

			BLENDFUNCTION bf = {AC_SRC_OVER,0,(BYTE)alpha,AC_SRC_ALPHA};
			HGDIOBJ so = SelectObject(som_tool_dc,SOM_MAP_grid);
		//	AlphaBlend(src,0,0,wdst,hdst,som_tool_dc,0,0,wdst,hdst,bf);
			{
				char *cw = (char*)SOM_MAP_app::CWnd(SOM_MAP.painting);
				int x = *(int*)(cw+0x70), y = 99-*(int*)(cw+0x74);
				if(x+w>100) w+=100-(x+w);
				if(99-y+h>100) h+=100-(99-y+h);
				int ww = min(wdst,w*21+1);
				int hh = min(hdst,h*21+1);
				AlphaBlend(src,0,0,ww,hh,som_tool_dc,0,0,ww,hh,bf);
			}
			SelectObject(som_tool_dc,so); 
		}
	}
	else //NOTE: SOM_MAP_ImageList_Draw_palette masks palette
	{				
		//DC_PEN is the only thing that works consistently?
		//HPEN p = CreatePen(0,1,0x595959); //153
		auto so = SelectObject(src,GetStockObject(DC_PEN)); //p
	//	SetDCPenColor(src,h>w?0x595959:0x999999);
	//	SetDCPenColor(src,0x999999);
		SetDCPenColor(src,0x888888);
		
		//int sep = w<h&&SOM_MAP_expand_icons?22:21;
				
		SOM_MAP_draw_grid(src,0,w,h); SOM_MAP_draw_grid(src,1,h,w); //sep

		SelectObject(src,so); //DeleteObject(p);
	}

	SOM::Tool.BitBlt(dst,xdst,ydst,wdst,hdst,src,xsrc,ysrc,rop);

	//HACK: restore 2px cursor region?
	extern HWND som_map_FrameRgn_AfxWnd42s;
	extern HBRUSH som_tool_red,som_tool_yellow;
	HBRUSH br = som_tool_red;
	if(som_map_FrameRgn_AfxWnd42s==GetFocus()) br = som_tool_yellow;
	extern HRGN som_map_FrameRgn_copy;
	SOM::Tool.FrameRgn(dst,som_map_FrameRgn_copy,br,2,2);

	return 1;
}

static BOOL SOM_MAP_msm_view(int key)
{
	short views[] = {!0,315,0,45,270,!0,90,225,180,135};
	int a = views[key];
	if(a==(int)!0) return MessageBeep(-1);
	
	int pt;
	{
		int *vp = (int*)SOM_MAP_app::CWnd(SOM_MAP.palette);
		int w = vp[0x54/4];
		int x = vp[0x48/4];
		int y = vp[0x4c/4];
		pt = SOM_MAP_my_plt[y*w+x];
	}

	auto &k = SOM_MAP.prt[pt];	
	wchar_t w[MAX_PATH],v[MAX_PATH];
	swprintf(w,L"map/parts/%s",k.longname());
	if(!EX::data(w,v)) return MessageBeep(-1);

	swordofmoonlight_prt_part_t p;

	size_t rw = 0;
	if(FILE*f=_wfopen(v,L"rb"))
	{
		rw = fread(&p,sizeof(p),1,f);
		fclose(f);
	}
	if(!rw) return MessageBeep(-1);

	p.aim = a/45; rw = 0; 

	if(FILE*f=_wfopen(v,L"wb"))
	{
		rw = fwrite(&p,sizeof(p),1,f);
		fclose(f);
	}

	if(!rw) MessageBeep(-1); return 0;
}

extern POINTS som_map_tile = {}, som_map_tilespace = {-1};
static bool SOM_MAP_in_selection()
{
	char *cw = (char*)SOM_MAP_app::CWnd(SOM_MAP.painting); 
	if(*(int*)(cw+0x60)) //selection?
	{
		int xx = som_map_tile.x/0x15+*(int*)(cw+0x70);
		int yy = som_map_tile.y/0x15+*(int*)(cw+0x74);

		RECT &sr = *(RECT*)(cw+0x88);
		if(xx>=sr.left&&xx<=sr.right&&yy>=sr.top&&yy<=sr.bottom)
		return true;
	}
	return false;
}
static bool SOM_MAP_fill_selection()
{
	if(!SOM_MAP_in_selection()) return false;

	auto *p = SOM_MAP_app::CWnd(som_tool);
	char *cw = (char*)SOM_MAP_app::CWnd(SOM_MAP.painting); 	
	RECT &sr = *(RECT*)(cw+0x88);
	auto pen = *(SOM_MAP_4921ac::Tile*)(cw+0xa0);
	if(pen.part==65535) //not repair_pen_selection
	{
		pen.part = *(WORD*)((BYTE*)p+200); //palette?
		pen.z = GetDlgItemFloat(som_tool,1000);
		pen.spin = SOM_MAP_4921ac.find_part_number(pen.part)->rot; 
	}
	SOM_MAP_4921ac.modified[0] = 1;
		
	((void(__thiscall*)(void*))0x41ae60)(p); //undo
	auto *grid = SOM_MAP_4921ac.grid;
	for(int yy=sr.bottom+1;yy-->sr.top;)
	{
		auto *row = grid+(99-yy)*100;
		for(int xx=sr.right+1;xx-->sr.left;) row[xx] = pen;
	}
	return true;
}

extern SHORT __stdcall SOM_MAP_GetKeyState_Ctrl(DWORD=0) //2021
{
	//ATTENTION SOM_MAP_AfxWnd42sproc
	//
	// this code is collaborating with SOM_MAP_AfxWnd42sproc
	// via som_tool_VK_CONTROL. before adding this using the
	// Ctrl key manually to override "CtrlLock" still worked
	// like the old Ctrl combination 
	//
	SHORT s; if(!som_tool_VK_CONTROL)
	{
		//HACK: this prevents Ctrl+Shift combos (which aren't
		//meaningful I believe) so pressing shift in CtrlLock
		//mode is able to move the cursor without selecting a
		//tile
		//NOTE: shift multi-selection requires the arrow keys
		//it would be nice if the mouse worked
		if(SOM::Tool.GetKeyState(VK_SHIFT)>>15) return 0;

		s = SOM::Tool.GetKeyState(VK_CONTROL)&~1;

		if(HWND ctrlock=GetDlgItem(som_tool,1224)) //CtrlLock?
		{
			//SOM_MAP checks 0x80! apparently the documentation
			//is screwy here
			if(!Button_GetCheck(ctrlock)) s = s?0:0xff80; //0x8000
		}
	}
	else s = som_tool_VK_CONTROL; return s;
}
extern SHORT __cdecl SOM_MAP_GetKeyState_Ctrl_void() //2022
{
	return SOM_MAP_GetKeyState_Ctrl(); 
}
extern HWND som_map_FrameRgn_AfxWnd42s = 0;
extern HBRUSH som_tool_red = CreateSolidBrush(0xFF);
extern HBRUSH som_tool_yellow = CreateSolidBrush(0xFFFF);
extern HRGN som_map_FrameRgn_copy = 0;
extern BOOL WINAPI som_map_FrameRgn(HDC A,HRGN B,HBRUSH C,int D,int E)
{
	if(som_map_FrameRgn_AfxWnd42s)
	{
		assert(SOM::tool==SOM_MAP.exe);

		SOM_TOOL_DETOUR_THREADMAIN(FrameRgn,A,B,C,D,E)

		EXLOG_LEVEL(7) << "som_tool_FrameRgn()\n";
		
		/*Reminder: Sandwich the overlay between the cursor
		if(SOM_MAP.painting==som_map_FrameRgn_AfxWnd42s)
		{
			extern void SOM_MAP_sandwich_overlay(HDC);
			SOM_MAP_sandwich_overlay(A); 
		}*/

		LOGBRUSH lb;
		if(GetObject((HGDIOBJ)C,sizeof(lb),&lb))
		{
			switch(lb.lbColor)
			{
			default: assert(0); goto other; //???
			case 0xFF: case 0xFFFF:; //red/yellow
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

		//NOTE: SOM_MAP_BitBlt_grid now redraws these

		if(!som_map_FrameRgn_copy)
		som_map_FrameRgn_copy = CreateRectRgn(0,0,0,0);
		CopyRgn(som_map_FrameRgn_copy,B);

other: return SOM::Tool.FrameRgn(A,B,C,D,E);
}
extern void SOM_MAP_drag_palette(int);
extern int som_tool_mousewheel(HWND,WPARAM,int);
static bool SOM_MAP_445fd0_dblclk(int,int,bool);
extern windowsex_TIMERPROC som_map_mousewheel_timer;
extern windowsex_TIMERPROC som_map_opentileview_timer;
extern LRESULT CALLBACK SOM_MAP_AfxWnd42sproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR hlp)
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
		//the borders and draw them in the parent's WM_ERASEBKGND
		HWND gp = GetParent(hWnd); HDC dc = GetDC(gp);
		RECT wr; GetWindowRect(hWnd,&wr); MapWindowRect(0,gp,&wr);
		static HBRUSH grey = CreateSolidBrush(0x646464); 
		FrameRect(dc,&wr,grey); 		
		ReleaseDC(gp,dc);
		return out;
	}	
	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,SOM_MAP_AfxWnd42sproc,id);		
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
		SetTimer(hWnd,uMsg,100,som_map_mousewheel_timer);
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

		if(wParam>=VK_NUMPAD0&&wParam<=VK_NUMPAD9
		 ||wParam>='0'&&wParam<='9') //no keypad?
		{
			int ctrlk = SendDlgItemMessage(som_tool,1224,BM_GETCHECK,0,0);
			if(ctrlk||GetKeyState(VK_CONTROL)>>15)			
			SOM_MAP_msm_view(wParam-(wParam<='9'?'0':VK_NUMPAD0));
		}

		switch(wParam)
		{
		//0: relying on som_map_FrameRgn 
		case VK_HOME: SetFocus(0); break;
		case VK_SPACE: goto workshop;
		}
		break;

	case WM_RBUTTONDBLCLK: return 1; //??? scrolls

	case WM_LBUTTONDBLCLK: workshop:

		SendMessage(som_tool,WM_COMMAND,1010,0);
		break;
	
	case WM_MOUSEMOVE: //2023
	case WM_LBUTTONDOWN: 
	case WM_RBUTTONDOWN: //???
		
		extern bool SOM_MAP_expand_icons;
		if(SOM_MAP_expand_icons)
		{
			auto &pts = (POINTS&)lParam;

			pts.x-=pts.x/21; pts.y-=pts.y/21;
		}
		
		if(GetFocus()!=hWnd) SetFocus(hWnd); 
				
		if(uMsg!=WM_MOUSEMOVE)
		{
			DefSubclassProc(hWnd,WM_LBUTTONDOWN,wParam,lParam);
			if(SOM::Tool::dragdetect(hWnd,uMsg))			
			SetCapture(hWnd);
			if(uMsg==WM_LBUTTONDOWN)
			{
				SOM_MAP_drag_palette(-1);
				if(~GetAsyncKeyState(VK_LBUTTON)>>15)
				goto lup;
			}			
			else
			{
				//hits twice (so does VK_LBUTTON above)
				//if(~GetAsyncKeyState(VK_RBUTTON)>>15)
				//goto rup;
			}			
		}
		if(GetCapture()==hWnd&&uMsg==WM_MOUSEMOVE)
		{
			int *vp = (int*)SOM_MAP_app::CWnd(SOM_MAP.palette);
			int w = vp[0x54/4];
			auto &pts = (POINTS&)lParam;
			int cmp1 = vp[0x4c/4]*w+vp[0x48/4];
			int x = pts.x/21+vp[0x40/4] ;
			int y = pts.y/21+vp[0x44/4];
			int cmp2 = y*w+x;
			if(cmp1!=cmp2&&cmp2<(int)SOM_MAP_4921ac.partsN)
			{
				int ctrlk = SendDlgItemMessage(som_tool,1224,BM_GETCHECK,0,0);

				vp[0x48/4] = (DWORD)x;
				vp[0x4c/4] = (DWORD)y;
				if(wParam&MK_LBUTTON)
				if(ctrlk||wParam&(MK_CONTROL|MK_SHIFT)||SOM_MAP_drag_palette_sel.second!=65535)
				SOM_MAP_drag_palette(cmp2);
				InvalidateRect(hWnd,0,0);
			}

			return 1;
		}
		break; 		

	case WM_LBUTTONUP: lup: //2023
	
		if(GetCapture()==hWnd)
		ReleaseCapture();
		extern void SOM_MAP_drag_palette_up();
		if(wParam&MK_SHIFT)
		SOM_MAP_drag_palette_up();

		//2023: just clear in case textureless MSM is loaded since
		//that doesn't seem to refresh the view window
		InvalidateRect(SOM_MAP.model,0,0);

		break;

	case WM_RBUTTONUP: rup: //2023
	
		if(GetCapture()==hWnd)
		{
			ReleaseCapture(); //TODO: scroll?
		}
		else
		{
			int ctrlk = SendDlgItemMessage(som_tool,1224,BM_GETCHECK,0,0);

			extern void SOM_MAP_spin_palette_up();		
			if(ctrlk||wParam&MK_CONTROL||SOM_MAP_drag_palette_sel.second!=65535)
			SOM_MAP_spin_palette_up();
		}
		break;
				
	case WM_SETFOCUS: case WM_KILLFOCUS: //overkill: invalidating cursor
			
		InvalidateRect(hWnd,0,0); break;   
	}
	//2017: -1,-1 is trying to twart an issue with being thrown into the 
	//bottom-right corner. I can't think what else might be causing this
	//static POINTS tile, tileview = {-1};
	POINTS &tile = som_map_tile, &tileview = som_map_tilespace;
	//POSSIBLY THIS CONTROL BELIEVES THAT
	//ITS SCROLLBAR UNITS ARE FONT GLYPHS
	//(IF SO, THE MECHANISM IS A MYSTERY)
	const int sep = 10; //WHY NOT 21?????
	if(hWnd==SOM_MAP.painting) switch(uMsg)
	{
	case WM_SETCURSOR: //Note: palette already does this?!

		if(GetFocus()!=hWnd) SetFocus(hWnd); break; //2024

	case WM_KEYDOWN:

		switch(wParam)
		{
		//0: relying on som_map_FrameRgn 
		case VK_HOME: SetFocus(0); break;

		/*2021: I'm combining these since it's
		//hard to press Enter left handed, but
		//it doesn't work here, so is moved to
		//som_map_dispatch
		case VK_SPACE: case VK_RETURN:
		{
			if(SOM_MAP_GetKeyState_Ctrl())
			{
				SetTimer(0,0,0,som_map_opentileview_timer);
				return 1;
			}
			else wParam = VK_RETURN; //lays tile
		}*/
		}
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
		
		if(GetKeyState(VK_MENU)>>15) return 1; //2023

		som_tool_pushed = hWnd;

		if((DWORD&)tile==lParam) //???
		if(SOM_MAP_GetKeyState_Ctrl())
		{
			SetTimer(hWnd,'open',0,som_map_opentileview_timer);						
			return 1;
		}
		break;

	case WM_LBUTTONUP: //2023: SOM_MAP_InvertRgn 

		InvalidateRect(hWnd,0,0);
		break;

	case WM_LBUTTONDOWN: 			
	
		if(GetFocus()!=hWnd) SetFocus(hWnd);  		

		tile = (POINTS&)lParam; //!!
		
		if(GetKeyState(VK_MENU)>>15) goto alt; //2023

		if(!SOM_MAP_GetKeyState_Ctrl()) //2024: fill selection?
		{
			if(SOM_MAP_fill_selection()) return 1; //consumes tile
		}
		else if(som_map_tab!=1216) //prefer classic ctrl down mode?
		{
			if(!SOM::Tool::dragdetect(hWnd,WM_LBUTTONDOWN)) alt:
			{	
				//SOM_MAP checks 0x80! apparently the documentation
				//is screwy here
				//som_tool_VK_CONTROL = 0x8000;
				som_tool_VK_CONTROL = 0xff80;			 
				DefSubclassProc(hWnd,WM_LBUTTONDOWN,wParam,lParam);
				DefSubclassProc(hWnd,WM_LBUTTONUP,wParam,lParam);
				som_tool_VK_CONTROL = 0;

				if(GetKeyState(VK_MENU)>>15) //Alt
				{
					extern bool workshop_exe(int);
					workshop_exe(40000);
				}
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

		tile = (POINTS&)lParam; 

		if(GetFocus()!=hWnd) SetFocus(hWnd); 
	//	if(som_map_tab!=1220) //TODO! checkpoints?
		if(int dbl=SOM::Tool::dragdetect(hWnd,
		SOM_MAP_in_selection()?WM_RBUTTONDBLCLK:WM_RBUTTONDOWN,
		SOM_MAP_in_selection()?300:500)) 
		{
			if(dbl==1) //drag view?
			{
				//int hmin, hmax, vmin, vmax;
				//GetScrollRange(hWnd,SB_HORZ,&hmin,&hmax); //100
				//GetScrollRange(hWnd,SB_VERT,&vmin,&vmax); //99

				tileview.x = GetScrollPos(hWnd,SB_HORZ);
				tileview.y = GetScrollPos(hWnd,SB_VERT);
				tile.x = tile.x/sep+tileview.x; tile.y = tile.y/sep+tileview.y;
				SetCapture(hWnd); return 0;
			}
			else if(dbl==2) //2024: flip selection?
			{
				if(SOM_MAP_445fd0_dblclk(tile.x,tile.y,true))
				return 1;
			}
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
			assert(GetCapture()==hWnd);
			
			if(~wParam&MK_RBUTTON) break; //2024

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

SOM_MAP_4921ac::Skin *SOM_MAP_4921ac::npcskins = 0;
static int __cdecl SOM_MAP_45736f_skin(char **buf, int rd, void *fread, int enemy)
{
	auto &skins = SOM_MAP_4921ac.npcskins;
	if(!((int(__cdecl*)(void*,int,int,void*))0x45736f)(buf,rd,1,fread))
	return 0;
	for(int i=0;i<1024;i++) if(char*p=buf[i])
	{
		p+=(DWORD)buf+(enemy?64:92); if(!*p||!p[1]) continue;

		if(!skins) memset(skins=new SOM_MAP_4921ac::Skin[2048],0x00,32*2048);

		memcpy(skins+enemy+i,p,31);
	}
	return 1;
}
static int __cdecl SOM_MAP_45736f_npc_pr2(char **buf, int rd, int, void *fread)
{
	return SOM_MAP_45736f_skin(buf,rd,fread,0);
}
static int __cdecl SOM_MAP_45736f_enemy_pr2(char **buf, int rd, int, void *fread)
{
	return SOM_MAP_45736f_skin(buf,rd,fread,1024);
}

static int __cdecl SOM_MAP_evt_45736f(swordofmoonlight_evt_header_t *buf, int, int rd, void *fread)
{
	if(!((int(__cdecl*)(void*,int,int,void*))0x45736f)(buf,rd,1,fread))
	return 0;

	//NOTE: 4128d0 doesn't do any checks here
	auto *e = buf->list;
	for(int i=1024;i-->0;e++)	
	if(e->ext_zr_flags||(int&)e->square_x||(int&)e->square_y) //compressing?
	{
		bool sq = 4==e->protocol;

		using SWORDOFMOONLIGHT::mdl::round; //YUCK

		SOM_MAP_4921ac::Event::zr zr;
		zr.xy = round(10*e->square_x); 
		zr.z = round(10*(sq?e->ext.square_z1:e->ext.radius_z1));
		zr.flag = 0!=(1&e->ext_zr_flags);

		(int&)e->square_x = zr.i;

		zr.xy = round(10*e->square_y); 
		zr.z = round(10*(sq?e->ext.square_z2:e->ext.radius_z2));
		zr.flag = 0!=(2&e->ext_zr_flags);

		(int&)e->square_y = zr.i;	
	}
	return rd;
}
static void __fastcall SOM_MAP_evt_427560(DWORD ecx) //__thiscall
{
	((void(__fastcall*)(DWORD))0x427560)(ecx); //__thiscall

	HWND dlg = *(HWND*)(ecx+28);

	auto *e = *(SOM_MAP_4921ac::Event**)(ecx+0x6c);

	int ep = e->protocol;

	if(e->type==0xff) //NEW
	{
		WORD ch[] = {1044,1164,1016,1017,1018};
		windowsex_enable(dlg,ch,0);
	}
	else switch(ep)
	{
	case 1: case 2: case 4: case 8:

		windowsex_enable(dlg,1069+(ep!=4),1072);
	}

	if(e->sx.i||e->sy.i) //decompressing?
	{
		bool sq = ep==4;

		if(sq) //clear real z1/z2 data?
		{
			e->cr = 0; e->cone = 360;
		}

		auto zr = e->sx; bool z = zr.flag!=0;

		if(z) SendDlgItemMessage(dlg,1070,BM_SETCHECK,1,0);
		if(z) SetDlgItemFloat(dlg,1071,zr.z*0.1f,L"%.1f");

		e->sx.f = sq?zr.xy*0.1f:0; //same

		zr = e->sy; bool r = zr.flag!=0;

		if(r) SendDlgItemMessage(dlg,1069,BM_SETCHECK,1,0);
		if(z) SetDlgItemFloat(dlg,1072,zr.z*0.1f,L"%.1f");

		e->sy.f = sq?zr.xy*0.1f:0; //same
	}
}
extern LRESULT CALLBACK SOM_MAP_evt_subclassproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scID, DWORD_PTR)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:

		SendDlgItemMessageW(hWnd,1071,EM_LIMITTEXT,5,0); //z1
		SendDlgItemMessageW(hWnd,1072,EM_LIMITTEXT,5,0); //z2
		break;

	case WM_COMMAND: //add zindex
		
		switch(int i=wParam)
		{
		case MAKEWPARAM(1199,CBN_SELENDOK):

			//NEW: at least on the first time the subject type changes
			//from nothing (empty) the first protocol's fields aren't 
			//enabled (SOM_MAP_evt_427560 is disabling them initially)
			PostMessage(hWnd,WM_COMMAND,MAKEWPARAM(1014,CBN_SELENDOK),(LPARAM)GetDlgItem(hWnd,1014));
			break;

		case MAKEWPARAM(1014,CBN_SELENDOK):

			//note: this depends on the subject's type
			i = ComboBox_GetItemData((HWND)lParam,ComboBox_GetCurSel((HWND)lParam));
			switch(i)
			{
			default: i = -1;
			case 1: case 2: case 8:
				if(i!=-1) windowsex_enable<1069>(hWnd,0);
				case 4: 
				windowsex_enable(hWnd,1069+(i!=4),1072,i!=-1);
			}
			break;

		case MAKEWPARAM(1071,EN_KILLFOCUS):
		case MAKEWPARAM(1072,EN_KILLFOCUS):
		
			using SWORDOFMOONLIGHT::mdl::round; //YUCK

			//REMINDER: these are compressed to fit
			//only within this range (it also mimics
			//the trip zone's parameters except that
			//negative values are permitted)
			i = round(10*GetWindowFloat((HWND)lParam));
			i = max(-2000,min(2000,i));
			SetWindowFloat((HWND)lParam,i*0.1f,L"%.1f");
			break;
		}
		break;

	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,SOM_MAP_evt_subclassproc,scID);
		break;
	}

	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
#pragma optimize("",off) 
static int __fastcall SOM_MAP_evt_4677F8_jmp(DWORD ecx, DWORD edx) //__thiscall
{
	DWORD dst = *(DWORD*)(ecx+0x64);
	DWORD err = *(char*)(ecx+0x5C)?0:~0ul;
	if(edx==err) end_dlg:
	return ((int(__fastcall*)(DWORD))0x4677F8)(ecx); //__thiscall

	HWND dlg = *(HWND*)(ecx+28);

	auto *e = SOM_MAP_4921ac.eventptrs[dst];

	int ep = e->protocol;
	
	char z,r = false; switch(ep)
	{
	default: z = false; break; 
		
	case 4: //square only?

		r = 0!=SendDlgItemMessage(dlg,1069,BM_GETCHECK,1,0);
		//break;

	case 1: case 2: case 8:

		z = 0!=SendDlgItemMessage(dlg,1070,BM_GETCHECK,1,0);
	}

	if(ep!=2&&ep!=64) e->item = 0xff; //clearing (new)

	if(ep!=1&&ep!=2) e->cone = 360; //same

	if(ep!=8) e->cr = 0; //same

	if(ep!=4&&!z) e->sx.i = e->sy.i = 0; //same

	if(r||z||e->sx.i||e->sy.i) //recompressing?
	{
		using SWORDOFMOONLIGHT::mdl::round; //YUCK

		auto zr = e->sx;
		zr.xy = round(zr.f*10); 
		zr.z = z?round(10*GetDlgItemFloat(dlg,1071)):0;
		zr.flag = z;

		e->sx = zr;

		zr = e->sy;
		zr.xy = round(zr.f*10); 
		zr.z = z?round(10*GetDlgItemFloat(dlg,1072)):0;
		zr.flag = r;

		e->sy = zr;
	}

	goto end_dlg;
}
static int __declspec(naked) SOM_MAP_evt_4677F8() 
{
	__asm mov edx,eax;
	__asm jmp SOM_MAP_evt_4677F8_jmp; //forward eax via edx
}
#pragma optimize("",on) 
static int __cdecl SOM_MAP_evt_457783(swordofmoonlight_evt_header_t::_item *buf, int, int, void *fwrite)
{
	auto *e = buf;
	for(int i=1024;i-->0;e++)	
	if((int&)e->square_x||(int&)e->square_y) //decompressing?
	{
		bool sq = 4==e->protocol;

		SOM_MAP_4921ac::Event::zr zr;
		zr.i = (int&)e->square_x;

		e->square_x = zr.xy*0.1f;
		(sq?e->ext.square_z1:e->ext.radius_z1) = zr.z*0.1f;
		e->ext_zr_flags = zr.flag;
		
		zr.i = (int&)e->square_y;

		e->square_y = zr.xy*0.1f;
		(sq?e->ext.square_z2:e->ext.radius_z2) = zr.z*0.1f;
		e->ext_zr_flags|=zr.flag<<1;
	}

	return ((int(__cdecl*)(void*,int,int,void*))0x457783)(buf,0xfc,0x400,fwrite);
}

static char *__cdecl SOM_MAP_456f00(char *str, char *substr)
{
	return PathFindExtensionA(str); (void)substr;
}

union SOM_MAP_this
{
	int _i[0x81]; float _f[0x81]; char _c[0x204]; //2021

	//sets cursor in the map painting/palette views	
	void __thiscall map_446190(int,int),map_446530(int,int);
	void __thiscall map_445fd0(int,int);
	
	void __thiscall tileview();

	//X/Y axes for enemies/NPCs
	//(now also does sizing/centering)
	DWORD __thiscall map_414170(DWORD), map_4142B0(DWORD,DWORD);
	DWORD __thiscall map_4141C0(DWORD), map_414300(DWORD,DWORD);
	//same deal for objects/items 
	DWORD __thiscall map_414210(DWORD), map_414350(DWORD,DWORD);
	DWORD __thiscall map_414260(DWORD), map_4143A0(DWORD,DWORD);
	
	/////////// 2021 ////////////////////

	struct texture_t //32B
	{
		DWORD _2; //2 for TXR, 3 for TIM?

		char *name;

		BYTE *TIM_ptr;

		DWORD width,height; //12

		DWORD _1; //1

		DX::IDirectDrawSurface4 *dds4; //??
		DX::IDirect3DTexture2 *d3dt2; //???
	};
	
	//MSM view (D3D6->D3D7)
	DWORD __thiscall map_442270(char*,int); //d3d init support
	DWORD __thiscall map_440ee0(); //init
	DWORD __thiscall map_442830_407470(texture_t*,char*); //TXR
	void __thiscall map_441ef0(); //draw
	//tile view init subroutine
	//WARNING 40be60 also has a path to intialize
	//the tile view
	DWORD __thiscall map_404530()
	{
		//NOTE: here 0x3c is added before calling
		//0044C658 8D 6B 3C             lea         ebp,[ebx+3Ch]

		return *(int*)((BYTE*)this+0xa9cac) = 1;
	}
	DWORD __thiscall map_44c800() //draw
	{
		//WEIRDNESS
		//0044C80A 83 C1 3C             add         ecx,3Ch  
		//0044C80D E9 4E DE FB FF       jmp         0040A660
		return ((SOM_MAP_this*)((BYTE*)this+0x3c))->map_44c800_3c();
	}
	DWORD __thiscall map_44c800_3c(); //draw+0x3c
	DWORD __thiscall map_408d00_409210_4096e0(texture_t*,char*,BYTE*); //TIM
	//DWORD __thiscall map_407470(texture_t*,char*); //TXR (MDO)
	//void __thiscall map_403780(); //teardown (SOM_MAP_403780)

	void init_lights();
	void draw_model(BYTE*); //40ac40
	void draw_lines(BYTE*); //40A870
	void draw_guidelines(); //40a990
	void draw_transparent_elements();

	void __thiscall map_41baa0(); //inject neighbors/layers

	//OPTIMIZING
	typedef SOM_MAP_4921ac::Part Part;
	Part *bsearch_for_413a30(Part *o, WORD part_number);
	void *__thiscall map_4139c0_palette(Part *o, WORD part);
	int __thiscall map_413aa0_palette(WORD part);
	void __thiscall map_413b30_palette(int,int,SOM_MAP_4921ac::Tile*);
	void __thiscall map_430180_palette(int);

	//MDL+MDO
	int __thiscall map_4084d0(SOM_MAP_model*,char*); 
	//D3D6? "tileview" has this covered!
	//void __thiscall map_40b190(BYTE*);
	int __thiscall map_409eb0(int,char*); //npc skin
	int __thiscall map_409e00(int,char*); //enemy skin
	//MSM (art)
	int __thiscall map_442440(SOM_MAP_model*m,char*a,char*b)
	{
		return map_442440_407c60(m,a,b,0x442440); //how to distinguish?
	}
	int __thiscall map_407c60(SOM_MAP_model*m,char*a,char*b)
	{
		return map_442440_407c60(m,a,b,0x407c60); //how to distinguish?
	}
	int __thiscall map_442440_407c60(SOM_MAP_model*,char*,char*,DWORD);

	//EXPERIMENTAL
	//enhanced view for gray grids?
	//add to ImageList(s)
//	DWORD __thiscall map_40f9d0(char**);
	DWORD __thiscall map_40f9d0(HBITMAP,HICON);
	DWORD __thiscall map_40f830(char*);	
	//CDC::FillSolidRect
	void __thiscall map_46cbc8(RECT*,DWORD);
	//checkpoint input
	typedef SOM_MAP_4921ac::Tile Tile;
	void __thiscall map_413b30_checkpoint(DWORD,DWORD,Tile*);

	//HACK: 42b1b6 open event instruction
	void *__thiscall map_465260_standby_map(WPARAM sel)
	{
		SOM_MAP_intellisense::evt_instruction **ret;
		(void*&)ret = ((void*(__thiscall*)(void*,WPARAM))0x465260)(this,sel);
		switch(ret&&ret[2]?ret[2]->code:0)
		{
		case 59: SOM_MAP_140(ret[2]); return 0; default: return ret;
		}
	}

	//2022: hotkeys 419eb8
	int __thiscall map_419fc0_keydown(int wParam, int lParam);
	//I can't believe I didn't notice/think of this until now!
	Tile &repair_pen_selection()
	{
		int y = 99-_i[0x7C/4], x = _i[0x78/4];
		auto &sel = SOM_MAP_4921ac.grid[100*y+x];
		if(sel.part!=0xffff) *(Tile*)(_c+0xa0) = sel; return sel;
	}

	int __thiscall map_46750D_minmax() //2023
	{
		while(40000!=GetWindowContextHelpId(som_tool))		
		{
			int ret = -1;
			SOM_CDialog *tp = (SOM_CDialog*)this;
			//0x0289d130
			tp->m_lpszTemplateName = MAKEINTRESOURCEA(102+(SOM_MAP_ini&1));
			ret = ((int(__thiscall*)(void*))0x46750D)(tp);
		
			if(*SOM_MAP_midminmax!=-1)
			{
				//NOTE: som_tool_subclassproc must post WM_QUIT on WM_DESTROY
				//since som state is missing (it may think it's already quit)

				MSG msg;
				while(PeekMessageW(&msg,0,0,0,1)) //som_tool_GetMessageA
				{
					if(msg.message!=WM_QUIT) //clear WM_QUIT
					{	
						TranslateMessage(&msg); DispatchMessage(&msg);
					}
				}
			}
			else break;			
		}
		return -1; //return ret;
	}
	int __thiscall map_4130e0_minmax_map(int map)
	{
		if(*SOM_MAP_midminmax!=-1)
		{	
			map = *SOM_MAP_midminmax; *SOM_MAP_midminmax = -1;	

		//	int ret = ((int(__thiscall*)(void*,int))0x4130e0)(this,map);

			auto *p = SOM_MAP_app::CWnd(SOM_MAP.painting);
			SetScrollPos(SOM_MAP.painting,SB_VERT,SOM_MAP_midminmax[1],1);
			SetScrollPos(SOM_MAP.painting,SB_HORZ,SOM_MAP_midminmax[2],1);
			for(int i=0;i<6-2;i++)
			*(int*)((char*)p+0x70+i*4) = SOM_MAP_midminmax[5+i];
			((void(__thiscall*)(void*))0x446980)(p);
		//	InvalidateRect(SOM_MAP.painting,0,0);
			SetWindowPos(som_tool,0,SOM_MAP_midminmax[3],SOM_MAP_midminmax[4],0,0,SWP_NOSIZE|SWP_NOZORDER);

		//	return ret;
			return 1;
		}
		else return ((int(__thiscall*)(void*,int))0x4130e0)(this,map);
	}
};
extern void SOM_MAP_minmax()
{
	DeleteObject(SOM_MAP_grid); SOM_MAP_grid = 0;

	bool mini = WS_MAXIMIZEBOX&GetWindowStyle(som_tool);

	auto *p = SOM_MAP_app::CWnd(som_tool);
	auto *q = SOM_MAP_app::CWnd(SOM_MAP.painting);

	//HACK: SOM_MAP_map2 crashes on lyt (err 3)
	SOM_MAP_midminmax[0] = SOM_MAP_201_open(0);
	{
		SOM_MAP_midminmax[1] = GetScrollPos(SOM_MAP.painting,SB_VERT);
		SOM_MAP_midminmax[2] = GetScrollPos(SOM_MAP.painting,SB_HORZ);
		RECT wr;
		GetWindowRect(som_tool,&wr);
		SOM_MAP_midminmax[3] = wr.left;
		SOM_MAP_midminmax[4] = wr.top;
		for(int i=0;i<6;i++)
		SOM_MAP_midminmax[5+i] = *(int*)((char*)q+0x70+i*4);
	}
	SOM_MAP_midminmax[11] = SendDlgItemMessage(som_tool,1224,BM_GETCHECK,0,0);
	if(!mini) SOM_MAP_ini|=1; else SOM_MAP_ini&=~1; 
	WCHAR w[32]; _itow(SOM_MAP_ini,w,10);
	WritePrivateProfileString(L"editor",L"toggleSets",w,SOM::Game::title('.ini'));
		
	void *f = p->_vtable[0x58/4]; //vtable

	if(0) //FIX ME: this is reloading the map
	{
		//save prompt/close?
		((void(__thiscall*)(void*))0x417450)(p);
	}
	else //map_4130e0_minmax_map doesn't reload
	{
		//DestroyWindow(som_tool); 
		((void(__thiscall*)(void*))f)(p); //0x47b3f4
	}

	som_tool = 0; assert(!som_tool_stack[1]); //UNUSED

	//BOOL _ = DestroyWindow(SOM_MAP.model);
	SOM_MAP.model = 0; 
	SOM_MAP.palette = 0;
	SOM_MAP.painting = 0; //MEMORY LEAK?

	PostMessage(0,WM_QUIT,0,0); //second time doesn't send it???
}
extern void som_map_refresh_model_view()
{
	char *cw = (char*)SOM_MAP_app::CWnd(som_tool);	
	int *vp = (int*)SOM_MAP_app::CWnd(SOM_MAP.palette);
	int w = vp[0x54/4];
	int x = vp[0x48/4];
	int y = vp[0x4c/4];
	int pt = y*w+x; //pt = SOM_MAP_my_plt[pt];
	((void(__thiscall*)(void*,int,int))0x417250)(cw,pt,1);
	InvalidateRect(SOM_MAP.model,0,0);
}

static void SOM_MAP_read_ev(char *p, char *ev)
{
	switch(*ev)
	{
	case 'e': case 'v': case 'E': case 'V': break;

	default: assert(0); p[0x4b] = 'v'; return;
	}
	p[0x4b] = *ev;
}

struct SOM_MAP_undo_t //2024
{
	unsigned undo; WORD y,x; QWORD a,b;
};
static size_t SOM_MAP_redo_sz = 0;
static std::vector<SOM_MAP_undo_t> SOM_MAP_undo_v;
extern void SOM_MAP_clear_undo()
{
	EX::section(*SOM_MAP._cs); //paranoia?

	windowsex_enable<1225>(som_tool,0); SOM_MAP_undo_v.clear();
	windowsex_enable<1082>(som_tool,0); SOM_MAP_redo_sz = 0;
}
static void __stdcall SOM_MAP_undo(HWND painting=SOM_MAP.painting, RECT*_=0, BOOL=0) //041ab60
{
	if(0==SOM_MAP_redo_sz)
	{
		MessageBeep(-1); return;
	}

	EX::section(*SOM_MAP._cs); //paranoia?

	auto *p = SOM_MAP_app::CWnd(som_tool); p->c[0xa0] = 1; //save?

	InvalidateRect(painting,0,0);

	if(SOM_MAP_undo_v.empty()) //remove me?
	{
		assert(0);
		memcpy(SOM_MAP_4921ac.grid,*(void**)(p->c+0x88),sizeof(SOM_MAP_4921ac.grid));
		return;
	}

	HWND redo = GetDlgItem(som_tool,1082);
	if(redo&&GetClassAtom(redo)!=som_tool_updown())
	EnableWindow(redo,1);

	size_t sz = SOM_MAP_redo_sz;

	auto itt = SOM_MAP_undo_v.begin()+sz;
	auto it = itt;
	for(;sz;sz--,it--)
	if(it[-1].undo!=itt[-1].undo) 
	break;
	SOM_MAP_redo_sz = sz;

	auto &c = *it;
	auto *a = (QWORD*)SOM_MAP_4921ac.grid;	
	auto iit = it;
	for(it=itt;it-->iit;)
	a[100*it->y+it->x] = it->b;

	windowsex_enable<1225>(som_tool,sz!=0);

	auto *cw = SOM_MAP_app::CWnd(SOM_MAP.painting);
	*(int*)(cw->c+0x78) = c.x;
	*(int*)(cw->c+0x7c) = 99-c.y;
	((void(__thiscall*)(void*,int,int))0x418110)(p,c.y,c.x);
	((void(__thiscall*)(void*))0x446980)(cw);
}
extern void SOM_MAP_redo()
{
	size_t sz = SOM_MAP_redo_sz;

	if(SOM_MAP_undo_v.size()<=sz)
	{
		MessageBeep(-1); return;
	}

	EX::section(*SOM_MAP._cs); //paranoia?
	
	auto *p = SOM_MAP_app::CWnd(som_tool); p->c[0xa0] = 1; //save?

	InvalidateRect(SOM_MAP.painting,0,0);

	HWND redo = GetDlgItem(som_tool,1082);

	auto itt = SOM_MAP_undo_v.end();
	auto iit = SOM_MAP_undo_v.begin()+sz;
	auto it = iit;
	for(;it!=itt;it++,sz++)
	if(iit->undo!=it->undo)
	{
		itt = it; break;
	}
	SOM_MAP_redo_sz = sz;

	auto &c = itt[-1];
	auto *a = (QWORD*)SOM_MAP_4921ac.grid;
	for(it=iit;it<itt;it++)
	a[100*it->y+it->x] = it->a;

	if(sz==SOM_MAP_undo_v.size())
	{
		if(redo&&GetClassAtom(redo)!=som_tool_updown())
		EnableWindow(redo,0);
	}

	windowsex_enable<1225>(som_tool,1);

	auto *cw = SOM_MAP_app::CWnd(SOM_MAP.painting);
	*(int*)(cw->c+0x78) = c.x;
	*(int*)(cw->c+0x7c) = 99-c.y;
	((void(__thiscall*)(void*,int,int))0x418110)(p,c.y,c.x);
	((void(__thiscall*)(void*))0x446980)(cw);
}
void CALLBACK SOM_MAP_poll_undo_timer(HWND win, UINT, UINT kill, DWORD)
{
	if(GetCapture()==win) return;

	EX::section(*SOM_MAP._cs); //paranoia?

	//erase redo data?
	{
		HWND redo = GetDlgItem(som_tool,1082);
		if(redo&&GetClassAtom(redo)!=som_tool_updown())
		EnableWindow(redo,0);
		SOM_MAP_undo_v.resize(SOM_MAP_redo_sz); 
	}
	//store diff data?
	{		
		auto *a = (QWORD*)SOM_MAP_4921ac.grid;	
		auto *b = *(QWORD**)(SOM_MAP_app::CWnd(som_tool)->c+0x88);

		SOM_MAP_undo_t undo = {EX::tick()};
		if(!SOM_MAP_undo_v.empty())
		{
			auto *p = &SOM_MAP_undo_v.back();
			auto *d = SOM_MAP_undo_v.data();
			if(undo.undo-p->undo<750)
			for(;;)
			if(p>d&&p[-1].undo==p->undo)
			{
				p->undo = undo.undo; p--;
			}
			else 
			{
				p->undo = undo.undo; break;
			}
		}

		for(int i=0;i<100*100;i++,a++,b++) if(*a!=*b) 
		{
			undo.y = (WORD)(i/100);
			undo.x = (WORD)(i%100);
			undo.a = *a;
			undo.b = *b;
			SOM_MAP_undo_v.push_back(undo);
		}
		SOM_MAP_redo_sz = SOM_MAP_undo_v.size();
	}

	KillTimer(win,kill);
}
static SOM_CWnd *__stdcall SOM_MAP_poll_undo(DWORD)
{
	HWND o = GetDlgItem(som_tool,1225); //46ab6c

	SetTimer(o,0,100,SOM_MAP_poll_undo_timer); 
	
	return SOM_MAP_app::CWnd(o);
}

static SOM_MAP_model *SOM_MAP_48_msm_mem = 0;

//HACK: really __thiscall
static void __fastcall SOM_MAP_403780(DWORD ecx)
{
	//TODO: I DID THIS THIS WAY JUST BECAUSE I DON'T
	//WANT TO THINK ABOUT HOW TO DO IT THE RIGHT WAY
	//IT SHOULDN'T BE HARD. I'VE MADE A NOT TO DO IT

	//HACK: piggyback on model deallocation to empty
	//out expanded enemy model data
	//403780 just calls "free" on pointer members. it 
	//would be better than this to reimplement it
	#ifdef SOM_TOOL_enemies2
	int size = SOM_MAP_4921ac.enemies_size;
	int rem = size;
	SOM_MAP_model *p = SOM_MAP_4921ac.enemies3.data();
	char *l28 = (char*)(ecx+0x1040C-8);	
	int mod = rem==128?128:rem%128;
	memcpy(l28,p,sizeof(*p)*mod);
	#endif
	((void(__fastcall*)(DWORD))0x403780)(ecx);

	//NOTE: I guess it would be better to do this
	//manually like the MSM models below, but the
	//MSM code is mercifully simple by comparison
	#ifdef SOM_TOOL_enemies2
	for(p+=mod,rem-=mod;rem;p+=128)
	{
		memcpy(l28,p,sizeof(*p)*128); if(rem-=128)
		{
			//NICE? put enemy models in (doomed)
			//NPC memory 
			p+=128;
			memcpy(l28-0x10200,p,sizeof(*p)*128);
			rem-=128;
		}
		((void(__fastcall*)(DWORD))0x403780)(ecx);
	}
	memset(p-=size,0,size*sizeof(*p));
	#endif

	//2021: free layer MSM memory 
	if(auto*m=SOM_MAP_48_msm_mem) 
	for(int i=48;i-->0&&m->c[0];m++)
	{
		//403a1d has example code
		
		//these call HeapFree via a multi-threaded list
		//I don't see ref counting (InterlockedIncrement)
		if(m->i[1]) ((void(__cdecl*)(DWORD))0x465768)(m->i[1]);
		if(m->i[2]) ((void(__cdecl*)(DWORD))0x465768)(m->i[2]);

		memset(m,0x00,sizeof(*m));
	}
}
extern void SOM_MAP_kfii_reprogram() //enemies limit
{
	//REMINDER: it's useful to be able to disable this
	//in order to be able to locate code that won't be
	//executed if it's not using the new table pointer
	#ifdef SOM_TOOL_enemies2

	//tileview 3D model data table is allocated here
	//SOM_MAP_app::CWnd is used to extract the pointer
	//allocating 0AA29C bytes
	//0041B974 68 9C A2 0A 00       push        0AA29Ch
	//0041B979 E8 C1 9D 04 00       call        0046573F
	//note: 406ad0 fills the back of this structure with
	//odd data, so it's probably impossible to extend it
	//(it's CWin data?)
	//initialization
	//0041B991 E8 AA 0B 03 00       call        0044C540
	//destructor?
	//0040382E F3 AB                rep stos    dword ptr es:[edi]
	//deallocating models?
	//note: every call is to "free"
	//00403780 83 EC 08             sub         esp,8
	//call site
	//004043FA E8 81 F3 FF FF       call        00403780
	*(DWORD*)0x4043FB = (DWORD)SOM_MAP_403780-0x4043FF;
	
	//NOTE: I should probably just let this go up to 65535
	//but it should rightly be 65536 since this is a count
	//2022: som_MPX_411a20_ltd is rejecting more than 4096
	WORD lim = (WORD)EX::INI::Editor()->map_enemies_limit;
	if(lim>4096) lim = 4096;

	SOM_MAP_4921ac.enemies2.resize(lim);
	SOM_MAP_4921ac.enemies3.resize(lim);

	void *data2 = SOM_MAP_4921ac.enemies2.data();
	void *data3 = SOM_MAP_4921ac.enemies3.data();

	DWORD x188e8 = (DWORD)data2-0x4921ac; 
	DWORD x14820 = (DWORD)data3;

	//enemies pointer (reading)
	//0041146E 66 3D 80 00          cmp         ax,80h  
	//00411472 66 89 83 E4 88 01 00 mov         word ptr [ebx+188E4h],ax  
	//0x411472
	//00411479 74 12                je          0041148D  
	//0041147B 8B 7C 24 18          mov         edi,dwor
	//00411482 66 C7 87 E4 88 01 00 80 00 mov         word ptr [edi+188E4h],80h  
	//0041148B EB 09                jmp         00411496  
	//0041148D 8B 7C 24 18          mov         edi,dword ptr [esp+18h]  
	//00411491 BB 80 00 00 00       mov         ebx,80h  
	//00411496 33 ED                xor         ebp,ebp
	/*
		0041146E 66 3D 00 02          cmp ax,200h  
		00411472 66 89 83 E4 88 01 00 mov word ptr [ebx+188E4h],ax  
		00411479 0F BF D8             movsx ebx,ax  
		0041147C 8B 7C 24 18          mov edi,dword ptr [esp+18h]  
		00411480 7E 0E                jle 00411496h  
		00411482 66 C7 87 E4 88 01 00 00 02 mov word ptr [edi+188E4h],200h  
		0041148B BB 00 02 00 00       mov ebx,200h
	*/
	char code1[] = "\x66\x3d\x00\x02\x66\x89\x83\xe4\x88\x01\x00\x0f\xbf\xd8\x8b\x7c\x24\x18\x7e\x0e\x66\xc7\x87\xe4\x88\x01\x00\x00\x02\xbb\x00\x02\x00\x00";
	memcpy((char*)0x41146e,code1,sizeof(code1)-1);
	memset((char*)0x41146e+sizeof(code1)-1,0x90,max(0,0x96-0x6e-int(sizeof(code1)-1)));
	*(WORD*)0x411470 = lim;
	*(WORD*)0x411489 = lim;
	*(DWORD*)0x41148C = lim;
	//00411498 8D B7 E8 88 01 00    lea         esi,[edi+188E8h] 
	*(DWORD*)0x41149a = x188e8;
	//(writing)
	//00412479 E8 EB 57 04 00       call        00457C69
	//0041247E 83 C4 0C             add         esp,0Ch
	//00412481 8D B3 E8 88 01 00    lea         esi,[ebx+188E8h]
	//00412487 C7 44 24 14 80 00 00 00 mov         dword ptr [esp+14h],80h
	/*
		//flipping around so this code can loop over the real count

		00412479 89 54 24 2E          mov         dword ptr [esp+20h],edx
		0041247D E8 60 7C 45 00       call        00457C69h
		00412482 83 C4 0C             add         esp,0Ch
		00412485 8D B3 E8 88 01 00    lea         esi,[ebx+188E8h]
	*/
	char code2[] = "\x89\x54\x24\x20\xe8\xe7\x57\x04\x00\x83\xc4\x0c\x8d\xb3\xe8\x88\x01\x00\x90\x90\x90\x90";
	memcpy((char*)0x412479,code2,sizeof(code2)-1);
	*(DWORD*)0x412487 = x188e8;
	//(more potential references)
	//comparing x/y
	//004148BF 8D 81 E9 88 01 00    lea         eax,[ecx+188E9h] 
	*(DWORD*)0x4148C1 = x188e8+1;
	//(Add button)
	//00414174 8D 8A 04 89 01 00    lea         ecx,[edx+18904h]
	//00414184 3D 80 00 00 00       cmp         eax,80h
	//00414191 8D 8C C0 3A 62 00 00 lea         ecx,[eax+eax*8+623Ah]
	*(DWORD*)0x414176 = x188e8+(0x904-0x8e8);
	*(DWORD*)0x414185 = lim;
	*(DWORD*)0x414194 = x188e8/4;
	//(Rec button)
	//004142BA 3D 80 00 00 00       cmp         eax,80h
	//004142C4 66 83 BC 8A 04 89 01 00 FF cmp         word ptr [edx+ecx*4+18904h],0FFFFh  
	//004142CF 8D 84 C0 3A 62 00 00 lea         eax,[eax+eax*8+623Ah]
	//00414404 8D 84 C0 3A 62 00 00 lea         eax,[eax+eax*8+623Ah]
	*(DWORD*)0x4142BB = lim;
	*(DWORD*)0x4142C8 = x188e8+(0x904-0x8e8);
	*(DWORD*)0x4142D2 = x188e8/4;
	*(DWORD*)0x414407 = x188e8/4;
	//(Remove button)
	//0041476C 81 FA 80 00 00 00    cmp         edx,80h  
	//00414772 7D 36                jge         004147AA
	//00414777 66 83 BC 86 04 89 01 00 FF cmp         word ptr [esi+eax*4+18904h],0FFFFh
	//00414780 8D 9C 86 04 89 01 00 lea         ebx,[esi+eax*4+18904h]
	//00414789 8D 94 D2 3A 62 00 00 lea         edx,[edx+edx*8+623Ah]
	memset((void*)0x41476C,0x90,8);
	*(DWORD*)0x41477b = x188e8+(0x904-0x8e8);
	*(DWORD*)0x414783 = x188e8+(0x904-0x8e8);
	*(DWORD*)0x41478c = x188e8/4;
	//NOTE: the next subroutine the caller calls removes events bound
	//to the removed element:
	//0041D6E5 E8 D6 78 FF FF       call        00414FC0
	//THEN: this removes the model data (and 403b38 too? 10408h?)
	//0041D6EE E8 CD 05 03 00       call        0044DCC0
	//...
	//0040A499 8D 9F 34 48 01 00    lea         ebx,[edi+14834h]
	{
		//model removal business

		*(BYTE*)0x40A49a = 0x9f; *(DWORD*)0x40A49b = x14820+(0x34-0x20);
		//0040A50E 8A 84 87 20 48 01 00 mov         al,byte ptr [edi+eax*4+14820h]
		*(DWORD*)0x40A50E = 0x85048a; *(DWORD*)0x40A511 = x14820;
		//0040A519 39 B5 34 48 01 00    cmp         dword ptr [ebp+14834h],esi
		*(BYTE*)0x40A51a = 0x35; *(DWORD*)0x40A51b = x14820+(0x34-0x20);
		//0040A533 8B 8C 87 38 48 01 00 mov         ecx,dword ptr [edi+eax*4+14838h]
		*(WORD*)0x40A534 = 0x850c; *(DWORD*)0x40A536 = x14820+(0x38-0x20);
		//0040A546 8B 85 34 48 01 00    mov         eax,dword ptr [ebp+14834h]
		*(WORD*)0x40A546 = 0xa190; *(DWORD*)0x40A548 = x14820+(0x34-0x20);

		//WHAT ON EARTH?
		//this offset places the would be "this" pointer smack in the middle
		//of the NPC model table
		//00409E70 8D 8D 1C 44 00 00    lea         ecx,[ebp+441Ch]
		//00409E8F 8D 8D 1C 44 00 00    lea         ecx,[ebp+441Ch]
		//0040A4D0 8D 8F 1C 44 00 00    lea         ecx,[edi+441Ch]
		//0040A552 8D 8F 1C 44 00 00    lea         ecx,[edi+441Ch] 
		//NUTS
		//this is derived from 0x81*0x204 within the subroutine these
		//lines call into
		DWORD x441C = x14820-0x10404; 
		*(BYTE*)0x409E71 = 0xd; *(DWORD*)0x409E72 = x441C;
		*(BYTE*)0x409E90 = 0xd; *(DWORD*)0x409E91 = x441C;
		*(BYTE*)0x40A4D1 = 0xd; *(DWORD*)0x40A4D2 = x441C;
		*(BYTE*)0x40A553 = 0xd; *(DWORD*)0x40A554 = x441C;
	}
	//(Event button)
	//00428960 81 FE 80 00 00 00    cmp         esi,80h
	*(DWORD*)0x428962 = lim;
	//(kill enemy instruction)
	//00432B60 81 FF 80 00 00 00    cmp         edi,80h
	*(DWORD*)0x432B62 = lim;
	//(place enemy instruction)
	//0043F89E 81 FF 80 00 00 00    cmp         edi,80h
	*(DWORD*)0x43F8a0 = lim;
	//(place enemy instruction)
	//0043227B 81 FF 80 00 00 00    cmp         edi,80h
	*(DWORD*)0x43227d = lim;
	/*initialization code
	    *(undefined2 *)(puVar2 + -2) = 0xffff;
		*puVar2 = 100;
		puVar2[2] = 1;
		puVar2[4] = 1;
		puVar2 = puVar2 + 0x24;
	*/
	//NOTE: leaving this alone (128) keeps the placeholders down to 
	//128 until that's exceeded at which point it will rachet it up
	//0040EC6C BA 80 00 00 00       mov         edx,80h
	//0040EC71 8D 86 06 89 01 00    lea         eax,[esi+18906h]
	//0040EC77 66 89 96 E4 88 01 00 mov         word ptr [esi+188E4h],dx
	*(DWORD*)0x40EC73 = x188e8+(0x906-0x8e8);
	//tree view initialization
	//0041BD80 3D 80 00 00 00       cmp         eax,80h 
	*(DWORD*)0x41BD81 = lim;

	//note: SOM_MAP_this::tileview implements this code
	//0040AB85 BB 80 00 00 00       mov         ebx,80h
	//?
	//tileview+0x14820 has another table including MDL data
	//0040AB8A 8D B7 20 48 01 00    lea         esi,[edi+14820h]
	*(BYTE*)0x40AB8B = 0x35;
	*(DWORD*)0x40AB8C = x14820;
	//more? 14820?
	{
		//note: this subroutine is called by the model loading 
		//routine, but it looks suspiciously like the deleting
		//routine. its "x441C" code is above with the deleting
		//routine

		//00409E15 81 FF 80 00 00 00    cmp         edi,80h  
		//00409E1B 7D 7D                jge         00409E9A
		memset((void*)0x409E15,0x90,8);
		//00409E2D 8D B4 85 20 48 01 00 lea         esi,[ebp+eax*4+14820h]
		*(WORD*)0x409E2E = 0x8534;
		*(DWORD*)0x409E30 = x14820;	
		//00409E34 8A 84 85 20 48 01 00 mov         al,byte ptr [ebp+eax*4+14820h]	
		*(WORD*)0x409E35 = 0x8504;
		*(DWORD*)0x409E37 = x14820; 

		//this prevents rotation/scale from being applied to
		//models upon opening the tile. there's NPC code too
		//0040BB76 3D 80 00 00 00       cmp         eax,80h  
		//0040BB7B 7D 4A                jge         0040BBC7  
		memset((void*)0x40BB76,0x90,7);
	}
	//0040BB84 8D 84 91 20 48 01 00 lea         eax,[ecx+edx*4+14820h]
	*(WORD*)0x40BB85 = 0x9504;
	*(DWORD*)0x40BB87 = x14820;
	//0040BC76 3D 80 00 00 00       cmp         eax,80h  
	//0040BC7B 7D 4E                jge         0040BCCB  
	//0040BC84 8D 84 91 20 48 01 00 lea         eax,[ecx+edx*4+14820h]
	memset((void*)0x40BC76,0x90,7);
	*(WORD*)0x40BC85 = 0x9504;
	*(DWORD*)0x40BC87 = x14820;
	//0040BD94 8D 84 91 20 48 01 00 lea         eax,[ecx+edx*4+14820h]
	*(WORD*)0x40BD95 = 0x9504;
	*(DWORD*)0x40BD97 = x14820;	

	//VARIOUS DEFENSIVE PROGRAMMING
	//
	// I'm not sure if  these are enemy or NPCs or both
	//
	//TPN business? I think this is unrelated
	//004013D0 81 FE 80 00 00 00    cmp         esi,80h  
	//004013D6 7F 10                jg          004013E8 
	//memset((void*)0x4013D0,0x90,7);
	//??
	//0041430A 3D 80 00 00 00       cmp         eax,80h  
	//0041430F 7D 31                jge         00414342
	memset((void*)0x41430A,0x90,7);
	//this prevents creation of HTREEITEM items
	//004143FD 3D 80 00 00 00       cmp         eax,80h  
	//00414402 7D 1F                jge         00414423 
	memset((void*)0x4143FD,0x90,7);
	//??
	//0041445D 3D 80 00 00 00       cmp         eax,80h  
	//00414462 7D 1F                jge         00414483
	memset((void*)0x41445D,0x90,7);

	//map icons? (ImageList_Draw?)
	//
	// DEAD CODE?
	// this is called on the tile contents screen but disabling it has
	// no effect.
	//
	//0042FBB7 FF 15 24 80 47 00    call        dword ptr ds:[478024h]
	//memset((void*)0x41445D,0x90,6);
	//more instances of ImageList_Draw
	//
	// This seems to draws the tile icons, rotated?
	//
	//this code defeats the four value switch-statement
	//memcpy((void*)0x4444a9,"\xb8\x04\x00\x00\x00",5); //mov eax,4
	/*
		this switch seems to (this) call routines for each of the
		map display modes

	004440AB E8 50 00 00 00       call        00444100  //MODE 1
	004440B0 EB 22                jmp         004440D4  
	004440B2 3C 01                cmp         al,1  
	004440B4 75 0E                jne         004440C4  
	004440B6 8D 4C 24 04          lea         ecx,[esp+4]  
	004440BA 51                   push        ecx  
	004440BB 8B CE                mov         ecx,esi  
	004440BD E8 FE 09 00 00       call        00444AC0  //MODE 2  
	004440C2 EB 10                jmp         004440D4  
	004440C4 3C 02                cmp         al,2  
	004440C6 75 0C                jne         004440D4  
	004440C8 8D 54 24 04          lea         edx,[esp+4]  
	004440CC 8B CE                mov         ecx,esi  
	004440CE 52                   push        edx  
	004440CF E8 0C 13 00 00       call        004453E0  //MODE 3
	*/
	//this is a subroutine called before BitBlt in 444AC0 MODE 2
	//004148E1 81 FA 80 00 00 00    cmp         edx,80h
	*(DWORD*)0x4148E3 = lim;

	#endif //kfii
}
extern void som_state_word(DWORD,WORD,WORD); //REMOVE ME
extern void SOM_MAP_reprogram()
{
	SOM_MAP_kfii_reprogram(); //i.e. lift enemies limit?

	//NOTES 
	//reading MAP file 
	//0041AF7C E8 5F 81 FF FF       call        004130E0
	//
	//writing MAP file (call 457C69 is fprintf/used exclusively)
	//0041381E E8 6D E9 FF FF       call        00412190
	//objects...
	//004122EB 8D B3 00 39 01 00    lea         esi,[ebx+13900h]
	//load profile into EAX (not AX)
	//004122F9 66 8B 06             mov         ax,word ptr [esi]  
	memcpy((void*)0x4122F9,"\x0f\xbf\x06",3);//movsx eax,word ptr [esi]
	//004122FC 66 3D FF FF          cmp         ax,0FFFFh  
	//00412300 0F 84 45 01 00 00    je          0041244B  
	//
	//	Restoring object z-index...
	//
	/*THIS CODE NEEDS TO BE REORDERED TO MAKE ROOM TO LOAD EAX
	00412306 0F BF D0             movsx       edx,ax  
	00412309 52                   push        edx  
	0041230A 8B CB                mov         ecx,ebx  
	0041230C E8 5F 22 00 00       call        00414570  
	00412311 0F BE 4E 03          movsx       ecx,byte ptr [esi+3]  
	00412315 D9 46 FC             fld         dword ptr [esi-4]  
	00412318 0F BE 56 02          movsx       edx,byte ptr [esi+2]  
	0041231C 51                   push        ecx  
	0041231D 52                   push        edx  
	0041231E 0F BF 0E             movsx       ecx,word ptr [esi]  
	00412321 0F BF 56 F8          movsx       edx,word ptr [esi-8]  
	00412325 51                   push        ecx  
	//NOTE: esp is 19dff4 (shouldn't matter)
	00412326 89 44 24 1C          mov         dword ptr [esp+1Ch],eax  
	*/
	//36B->30B (+4B extra to load EAX...)	
	/*		
	00412311 0F BE 4E 03          movsx       ecx,byte ptr [esi+3]  
	00412315 D9 46 FC             fld         dword ptr [esi-4]  
	00412318 0F BE 56 02          movsx       edx,byte ptr [esi+2]  
	0041231C 51                   push        ecx  
	0041231D 52                   push        edx  
	//0041231E 0F BF 0E             movsx       ecx,word ptr [esi]  	
	//00412325 51                   push        ecx  
		//00412306 0F BF D0             movsx       edx,ax  	
		//00412309 52                   push        edx  
		50 push eax
		50 push eax
		0041230A 8B CB                mov         ecx,ebx  
		0041230C E8 5F 22 00 00       call        00414570  
		00412326 89 44 24 1C          mov         dword ptr [esp+1Ch],eax  
	00412321 0F BF 56 F8          movsx       edx,word ptr [esi-8]  
	//finally, move z-index into EAX
	0f be 46 e4 movsx eax,byte ptr [esi-1Ch]
	*/		
		#ifdef _DEBUG
		BYTE *p = (BYTE*)0x412306;
		memmove(p,p+11,13);
		p+=13;
		*(DWORD*)p = 0xCB8B5050; //push eax; push eax; mov ecx,ebx
		p+=4;
		//0041230C E8 5F 22 00 00       call        00414570 
		*p = 0xE8; *(DWORD*)(p+1) = 0x414570-DWORD(p+5);
		p+=5;
		//00412326 89 44 24 1C          mov         dword ptr [esp+1Ch],eax  
		*(DWORD*)p = 0x1c244489;
		p+=4;
		//00412321 0F BF 56 F8          movsx       edx,word ptr [esi-8]  
		*(DWORD*)p = 0xF856Bf0F;
		p+=4;		
		//LOADING EAX
		*(DWORD*)p = 0xe446BE0F; //movsx eax,byte ptr [esi-1Ch] 
		p+=4; 
		*p++ = 0x90; *p++ = 0x90; assert(0x41232A==(DWORD)p); //last 2B
		#else
		memcpy((void*)0x412306,
		"\x0F\xBE\x4E\x03"	//0F BE 4E 03          movsx       ecx,byte ptr [esi+3]  
		"\xD9\x46\xFC"		//D9 46 FC             fld         dword ptr [esi-4]  
		"\x0F\xBE\x56\x02"	//0F BE 56 02          movsx       edx,byte ptr [esi+2]  
		"\x51"	//51                   push        ecx  
		"\x52"	//52                   push        edx  
		"\x50"	//50                   push        eax  
		"\x50"	//50                   push        eax  
		"\x8B\xCB"			//8B CB                mov         ecx,ebx  
		"\xE8\x54\x22\0\0"	//E8 54 22 00 00       call        00414570  
		"\x89\x44\x24\x1C"	//89 44 24 1C          mov         dword ptr [esp+1Ch],eax  
		"\x0F\xBF\x56\xF8"	//0F BF 56 F8          movsx       edx,word ptr [esi-8]  
		"\x0F\xBE\x46\xE4" 	//0F BE 46 E4          movsx       eax,byte ptr [esi-1Ch]  
		"\x90"	//90                   nop  
		"\x90"	//90                   nop  
		,36);
		#endif

	//Weird? fix SOM_MAP's broken "Save" button
	//TODO: do from som.tool.cpp same as som.state.cpp does for games
	//In Release-mode builds only the Save button throws up a disk-full error message???	
	{
		 //EDI here is 0x77d63cbb KernelBase.dll
		//Release mode returns 0 via ESP+2c, debug 0x749bf44c ???
		/*(both modes return 0 via EAX, so it's possible that ESP+2c holds garbage)
		00413644 FF D7            call        edi  
		00413646 8B 44 24 2C      mov         eax,dword ptr [esp+2Ch] 
		0041364A 85 C0            test        eax,eax 
		0041364C 0F 87 36 FE FF FF ja          00413488 
		00413652 72 0E            jb          00413662
		//100000h is probably 1MB
		00413654 81 7C 24 28 00 00 10 00 cmp         dword ptr [esp+28h],100000h 
		0041365C 0F 83 26 FE FF FF jae         00413488 
		*/
		som_state_word(0x1264C,0x870F,0xE990);
	}

	//save instructions 0x50 and 0x54
	{
		//4 here is the "item" quantity query mode
		//we want to knockout this test so all modes
		//are able to save 4's extra combobox dropdown
				
		//0043AD63 3C 04            cmp         al,4 
		//0043AD65 88 46 5C         mov         byte ptr [esi+5Ch],al 
		//0043AD68 75 2B            jne         0043AD95 
		som_state_word(0x39D68,0x2B75,0x9090); //NOP,NOP

		//004384DC 3C 04            cmp         al,4 
		//004384DE 88 46 5C         mov         byte ptr [esi+5Ch],al 
		//004384E1 75 2B            jne         0043850E
		som_state_word(0x374E1,0x2B75,0x9090); //NOP,NOP

		//not just saving, but loading is required too

		//0043AB81 FF D7            call        edi  
		//0043AB83 80 7E 5C 04      cmp         byte ptr [esi+5Ch],4 
		//0043AB87 75 4C            jne         0043ABD5 
		som_state_word(0x39B87,0x4C75,0x9090); //NOP,NOP

		//004383BF FF D5            call        ebp  
		//004383C1 80 7E 5C 04      cmp         byte ptr [esi+5Ch],4 
		//004383C5 75 4C            jne         00438413 
		som_state_word(0x373C5,0x4C75,0x9090); //NOP,NOP
	}

	//knockout XY axes flag for object visualization
	{
		//0044D907 E8 A4 6C FC FF   call        004145B0 
		//0044D90C 85 C0            test        eax,eax 
		//0044D90E 74 49
		som_state_word(0x4C90E,0x4974,0x9090); //NOP,NOP
	}

		//I think (2017) this is a trick to get the address of
		//__thiscall subroutines
		//2021: switching to auto and pulling out of use sites
		#define _(mf) auto mf = &SOM_MAP_this::mf;
		//adding XY axes to enemies/NPCs
		_(map_414170)_(map_4142B0)
		_(map_4141C0)_(map_414300)
		_(map_414210)_(map_414350)
		_(map_414260)_(map_4143A0)
		//2017: fix for ctrl+click elevation/rotation clobbering bug
		_(map_446190)_(map_446530)
		//2024: right click (block rotate)
		_(map_445fd0)
		//2017?
		_(tileview)	
		//2021: D3D7/9 MSM viewer
		_(map_442270)_(map_440ee0)_(map_441ef0)
		//2021: D3D7/9tile viewer
		_(map_404530)_(map_44c800)
		//2021: D3D7/9 textures
		_(map_442830_407470)_(map_408d00_409210_4096e0)
		//2021: tile viewer enhancement
		_(map_41baa0)_(bsearch_for_413a30)
		//2021: MDL+MDO
		_(map_4084d0)_(map_409eb0)_(map_409e00)_(map_442440)_(map_407c60)
		//2021: enhanced view for gray grids?
		_(map_46cbc8)_(map_40f830) //_(map_40f9d0)
		//checkpoint input (related)
		_(map_413b30_checkpoint)
		//standby map event
		_(map_465260_standby_map)
		//hotkeys
		_(map_419fc0_keydown)
		//big map editor
		_(map_46750D_minmax)_(map_4130e0_minmax_map)
		//palette reorder
		_(map_4139c0_palette)_(map_413aa0_palette)_(map_413b30_palette)
		//keyboard nav
		_(map_430180_palette)
		#undef _
	
	//adding XY axes to enemies/NPCs
	//0041D120 E8 4B 70 FF FF   call        00414170
	*(DWORD*)0x41D121 = (DWORD&)map_414170-0x41D125;
	//0041D940 E8 6B 69 FF FF   call        004142B0
	*(DWORD*)0x41D941 = (DWORD&)map_4142B0-0x41D945;
	//0041D256 E8 65 6F FF FF   call        004141C0 
	*(DWORD*)0x41D257 = (DWORD&)map_4141C0-0x41D25B;
	//0041DA30 E8 CB 68 FF FF   call        00414300
	*(DWORD*)0x41DA31 = (DWORD&)map_414300-0x41DA35;
	//0041D395 E8 76 6E FF FF   call        00414210
	*(DWORD*)0x41D396 = (DWORD&)map_414210-0x41D39A;
	//0041DB29 E8 22 68 FF FF   call        00414350
	*(DWORD*)0x41DB2A = (DWORD&)map_414350-0x41DB2E;
	//0041D4CE E8 8D 6D FF FF   call        00414260
	*(DWORD*)0x41D4CF = (DWORD&)map_414260-0x41D4D3;
	//0041DC1C E8 7F 67 FF FF   call        004143A0
	*(DWORD*)0x41DC1D = (DWORD&)map_4143A0-0x41DC21;
		
	//__thiscall (tile renderer)
	//note the memory is contiguous
	//these call the subroutines below
	//this+85620h+204h*16 tiles (25x25 blocks?)
	//0040A779 8B CE            mov         ecx,esi 
	//0040A77B E8 30 03 00 00   call        0040AAB0 
	*(DWORD*)0x40A77C = (DWORD&)tileview-0x40A780;
	//this+65220h+204h*256 items
	//0040A780 8B CE            mov         ecx,esi 	
	//0040A782 E8 69 03 00 00   call        0040AAF0 
	*(DWORD*)0x40A783 = (DWORD&)tileview-0x40A787;
	//this+24A20h+204h*512 objects
	//0040A787 8B CE            mov         ecx,esi 	
	//0040A789 E8 A2 03 00 00   call        0040AB30 
	*(DWORD*)0x40A78A = (DWORD&)tileview-0x40A78E;
	//this+14820h+204h*128 npcs
	//0040A78E 8B CE            mov         ecx,esi 	
	//0040A790 E8 EB 03 00 00   call        0040AB80 
	*(DWORD*)0x40A791 = (DWORD&)tileview-0x40A795;
	//this+4620h+204h*128 enemies 
	//0040A795 8B CE            mov         ecx,esi	
	//0040A797 E8 24 04 00 00   call        0040ABC0 
	*(DWORD*)0x40A798 = (DWORD&)tileview-0x40A79C;
	//todo: see what this is (lone tile?)
	//0040A79C 8B CE            mov         ecx,esi 
	//0040A79E E8 5D 04 00 00   call        0040AC00
	*(DWORD*)0x40A79F = (DWORD&)tileview-0x40A7A3;

	//2017: fix for ctrl+click elevation/rotation clobbering bug
	//00445ECF E8 BC 02 00 00       call        00446190
	*(DWORD*)0x445ED0 = (DWORD&)map_446190-0x445ED4;
	//445e86: dragging
	*(DWORD*)0x445e87 = (DWORD&)map_446530-0x445e8b;
	//2024: block rotate?
	//00445fbf e8 0c 00 00 00       call        00445fd0
	*(DWORD*)0x445fc0 = (DWORD&)map_445fd0-0x445fc4;

	//////////// 2017? ////////////////////
	// 
	//TURNS OUT ONLY OBJECTS TAKE THIS PATH
	//(can't hurt to knock all out)
	//knockout rounding prior to SetWindowTextA
	//48FCC0 "%.1f"
	DWORD calls[] =
	{
	//OBJECTS
	//0045405E 68 C0 FC 48 00       push        48FCC0h  
	//00454063 51                   push        ecx 
	//00454064 E8 F6 09 01 00       call        00464A5F  
	0x454064, //1025
	//0045409D E8 BD 09 01 00       call        00464A5F  
	0x45409D, //1026
	//004540D6 E8 84 09 01 00       call        00464A5F  
	0x4540D6, //1027
	//004542A4 E8 B6 07 01 00       call        00464A5F
	0x4542A4, //1035

	//not objects... possibly tile elevation (desirable)
	//00418218 E8 42 C8 04 00       call        00464A5F
	0x418218,
	//0041B86F E8 EB 91 04 00       call        00464A5F
	0x41B86F,

	//ENEMY
	//0044F70E E8 4C 53 01 00       call        00464A5F
	0x44F70E, //1025
	//0044F747 E8 13 53 01 00       call        00464A5F
	0x44F747, //1026
	//0044F780 E8 DA 52 01 00       call        00464A5F
	0x44F780, //1027
	//0044F8A0 E8 BA 51 01 00       call        00464A5F
	0x44F8A0, //1035
	
	//NPC
	//0044F214 E8 46 58 01 00       call        00464A5F
	0x44F214, //1025
	//0044F250 E8 0A 58 01 00       call        00464A5F
	0x44F250, //1026
	//0044F28C E8 CE 57 01 00       call        00464A5F
	0x44F28C, //1027
	//0044F3BE E8 9C 56 01 00       call        00464A5F
	0x44F3BE, //1035

	//ITEMS
	//00451D4E E8 0C 2D 01 00       call        00464A5F
	0x451D4E, //1025
	//00451D87 E8 D3 2C 01 00       call        00464A5F
	0x451D87, //1026
	//00451DC0 E8 9A 2C 01 00       call        00464A5F
	0x451DC0, //1027

	//starting point
	//0041F920 E8 3A 51 04 00       call        00464A5F
	0x41F920,
	//0041F994 E8 C6 50 04 00       call        00464A5
	0x41F994,
	//0041FB20 E8 3A 4F 04 00       call        00464A5F
	0x41FB20,
	//0041FB8D E8 CD 4E 04 00       call        00464A5F
	0x41FB8D,
	//0041FBFA E8 60 4E 04 00       call        00464A5F
	0x41FBFA,	
	//00421400 E8 5A 36 04 00       call        00464A5F
	0x421400, //421340 (Z start offset)	
	//0042151A E8 40 35 04 00       call        00464A5F
	//0x42151A, //421460 (Y start offset)
	};
	for(int i=0;i<EX_ARRAYSIZEOF(calls);i++)
	*(DWORD*)(calls[i]+1) = (DWORD)SOM_MAP_464a5f-(calls[i]+5);	
	#if 0 //DANGER
	if(EX::debug) //reducing noise (will cause problems)
	{
		DWORD noise[] = //calls after loading, etc. 
		{
		//48FCC8 %d
		//00418181 E8 D9 C8 04 00       call        00464A5F
		0x418181,
		//004181AC E8 AE C8 04 00       call        00464A5F
		0x4181AC,
		//004181E4 E8 76 C8 04 00       call        00464A5F
		0x4181E4,
		//0041B800 E8 5A 92 04 00       call        00464A5F
		0x41B800,
		//0041B835 E8 25 92 04 00       call        00464A5F
		0x41B835,
		//0041B8AD E8 AD 91 04 00       call        00464A5F
		0x41B8AD,
		//0044F845 E8 15 52 01 00       call        00464A5F
		0x44F845, //enemy
		//0044F35D E8 EE 70 E3 58       call        00464A5F
		0x44F35D, //npc
		//0041F9FE E8 5C 50 04 00       call        00464A5F
		//0x41F9FE, //starting point
		//0041FA37 E8 23 50 04 00       call        00464A5F
		//0x41FA37, //starting point
		//0041FA74 E8 E6 4F 04 00       call        00464A5F
		//0x41FA74, //starting point
		//0041FAAC E8 AE 4F 04 00       call        00464A5F
		//0041FAE4 E8 76 4F 04 00       call        00464A5F
		//0041FC63 E8 F7 4D 04 00       call        00464A5F

		//48FCB0 %04d
		//00418253 E8 07 C8 04 00       call        00464A5F
		0x418253,
		//0041B8E9 E8 71 91 04 00       call        00464A5F
		0x41B8E9,

		//48FB98 [%04d]
		//004172F5 E8 65 D7 04 00       call        00464A5F 
		0x4172F5,
		//0041BEBC E8 9E 8B 04 00       call        00464A5F
		0x41BEBC,
		//0044EC4C E8 0E 5E 01 00       call        00464A5F
		0x44EC4C, //npc?
		//0044F03C E8 1E 5A 01 00       call        00464A5F
		0x44F03C, //enemy??

		//48FE1C [%03d]
		//0041BE18 E8 42 8C 04 00       call        00464A5F
		0x41BE18,
		//00451C45 E8 15 2E 01 00       call        00464A5F
		0x451C45, //npc?
		//0041BC4E E8 0C 8E 04 00       call        00464A5F
		0x41BC4E, //enemy?
		//0044F0ED E8 6D 59 01 00       call        00464A5F
		0x44F0ED, //enemy?
		//0044ECFD E8 5D 5D 01 00       call        00464A5F
		0x44ECFD, //npc?

		//48F3F0 "TIM %02d"
		//00408B62 E8 F8 BE 05 00       call        00464A5F
		0x408B62,

		//48FDE0 "%.0f" (41A370 like?)
		//004213AE E8 AC 36 04 00       call        00464A5F
		//starting point?
		};
		for(int i=0;i<EX_ARRAYSIZEOF(noise);i++)
		memset((void*)noise[i],0x90,5);
	}
	#endif

	//setting tile height/elevation (41A370)
	//
	//  REMINDER: 42133F/421460/421340 look very similar (start point)	
	//  421340 is used to set the elevation
	//
	//0041A3DF D8 0D 7C B4 47 00    fmul        dword ptr ds:[47B47Ch]
	memset((void*)0x41A3DF,0x90,6);
	//0041A3EA DD 1C 24             fstp        qword ptr [esp]
	memset((void*)0x41A3EA,0x90,3);
	//0041A4FF DB 44 24 10          fild        dword ptr [esp+10h] 
	memset((void*)0x41A4FF,0x90,4);
	//0041A50B D8 0D 78 B4 47 00    fmul        dword ptr ds:[47B478h]
	memset((void*)0x41A50B,0x90,6);
		//2019: MULTI-SELECT VERSION OF = BUTTON? (it's broken)
	//0041A482 DB 44 24 14          fild        dword ptr [esp+14h]
	memset((void*)0x41A482,0x90,4);
	//0041A48E D8 0D 78 B4 47 00    fmul        dword ptr ds:[47B478h]
	memset((void*)0x41A48E,0x90,6);
	//0041A495 D9 5C 24 2C          fstp        dword ptr [esp+2Ch]
	*(BYTE*)0x41A496 = 0x54; //fst
	//
	//setting starting height (421340)
	//
	//NOTE: breaks slider on EN_KILLFOCUS
	//0042139A D8 0D 7C B4 47 00    fmul        dword ptr ds:[47B47Ch]
	memset((void*)0x42139A,0x90,6);
	//004213A5 DD 1C 24             fstp        qword ptr [esp] 
	memset((void*)0x4213A5,0x90,3);
	//004213E6 DB 44 24 10          fild        dword ptr [esp+10h]
	memset((void*)0x4213E6,0x90,4);
	//004213F1 D8 0D 78 B4 47 00    fmul        dword ptr ds:[47B478h]
	memset((void*)0x4213F1,0x90,6);

	/////////// 2021 ////////////////////////////////
	
	//Direct3D 9?
	//supporting 7 will take some work
	if(EX::directx()>=9) 
	{
		DDRAW::target = 'dx9c'; //HACK: signal to SomEx.cpp?

		char add_esp[] = "\x83\xc4\x00\x90\x90"; //add esp,0

		//MSM viewer init subroutine
		//seems more clean to knockout this enumeration code
		//00440C0C E8 5F 00 00 00       call        00440C70
		memset((void*)0x440C0C,0x90,5);
		//00443E10 E8 CB D0 FF FF       call        00440EE0
		//00440C13 E8 C8 02 00 00       call        00440EE0
		//apparently this isn't used... Ghidra couldn't see
		//the second, but tracing it was easy enough
		//*(DWORD*)0x443E11 = (DWORD&)map_440ee0-0x443E15;
		*(DWORD*)0x440C14 = (DWORD&)map_440ee0-0x440C18;
		//004173BE E8 AD AE 02 00       call        00442270 
		*(DWORD*)0x4173Bf = (DWORD&)map_442270-0x4173c3;
		//
		//MSM viewer texture subroutine
		//00442660 E8 CB 01 00 00       call        00442830
		*(DWORD*)0x442661 = (DWORD&)map_442830_407470-0x442665;		
		{
			//don't ALL-CAPS textures for art system consistency
			//00407DF6 E8 85 F1 04 00       call        00456F80
			//00408286 E8 F5 EC 04 00       call        00456F80
			//004425FF E8 7C 49 01 00       call        00456F80
			memset((void*)0x407DF6,0x90,5); //MSM viewer
			memset((void*)0x408286,0x90,5); //MDO
			memset((void*)0x4425FF,0x90,5); //tile viewer

			//YUCK: have to deal with strstr(".BMP") //48f3e9
			//could convert to "." but would rather it use
			//strrchr (no strrstr option)
			//00407E08 E8 F3 F0 04 00       call        00456F00 
			//00408298 E8 63 EC 04 00       call        00456F00
			//00442611 E8 EA 48 01 00       call        00456F00 
			*(DWORD*)0x407E09 = (DWORD)SOM_MAP_456f00-0x407E0d;
			*(DWORD*)0x408299 = (DWORD)SOM_MAP_456f00-0x40829d;
			*(DWORD*)0x442612 = (DWORD)SOM_MAP_456f00-0x442616;

			//conform with other tools/game
			*(DWORD*)0x48f3e0 = 0x7278742E; //".TXR"->".txr"
			//48F3FC is "MDL" (Ghidra can't find a reference)
			//48f400 is "MDO" (can't be for loading objects?)
		//	*(DWORD*)0x48F3FC = 0x006C646D;
		//	*(DWORD*)0x48f400 = 0x006f646D;

			//default MSM/textures to map/model?
			//the msm path steps on the texture path
			//memcpy((void*)0x48FA9A,"model\\\0",8); //0x48fa90 //msm
			memcpy((void*)0x48FAAA,"model\\\0",8); //0x48faa0 //texture
			//use texture path?
			//0044C91A 68 90 FA 48 00       push        48FA90h
			//004156A0 68 90 FA 48 00       push        48FA90h
			*(BYTE*)0x44C91b = *(BYTE*)0x4156A1 = 0xA0; 
		}
		//
		//MSM viewer drawing subroutine
		//00441EBF E8 2C 00 00 00       call        00441EF0
		//00442426 E8 C5 FA FF FF       call        00441EF0
		//004434D1 E8 1A EA FF FF       call        00441EF0
		//mouse proc... for some reason Ghidra didn't see it
		//00443278 E8 73 EC FF FF       call        00441EF0
		//double-click
		//0044332D E8 BE EB FF FF       call        00441EF0
		*(DWORD*)0x441EC0 = (DWORD&)map_441ef0-0x441EC4;
		*(DWORD*)0x442427 = (DWORD&)map_441ef0-0x44242b;
		*(DWORD*)0x4434D2 = (DWORD&)map_441ef0-0x4434D6;
		*(DWORD*)0x443279 = (DWORD&)map_441ef0-0x44327d;
		*(DWORD*)0x44332e = (DWORD&)map_441ef0-0x443332;
		//REMOVE ME
		// 
		// this routine seems to recover on "lost device" 
		// state but is being entered likely erroneously
		// map_443450 is a dummy that does nothing until
		// I better understand the situation
		// 
		// FIX? setting 0xc0 to a nonzero value might fix
		// it. [c0] is 0x031b0a28
		// 
		//004183FB E8 50 B0 02 00       call        00443450
		//443450 calls through to 441ef0
		//*(DWORD*)0x4183Fc = (DWORD&)map_443450-0x418400;
		*(DWORD*)0x4183Fc = (DWORD&)map_441ef0-0x418400;

		//tile viewer init subroutine?
		//WARNING: 40be60 also has a path to intialize
		//the tile view
		//seems more clean to knockout this enumeration code
		//0044C65F E8 CC 79 FB FF       call        00404030
		/*maybe this subroutine is needed to setup the HWND?
		memcpy((void*)0x44C65F,&(add_esp[2]=4)-2,5); //add esp,4
		//004040F8 E8 13 00 00 00       call        00404110*/
		memset((void*)0x4040F8,0x90,5);
		//0044C666 E8 C5 7E FB FF       call        00404530
		*(DWORD*)0x44C667 = (DWORD&)map_404530-0x44C66b;
		//can't call SetTransform
		//0044C745 E8 D6 9F FB FF       call        00406720
		memcpy((void*)0x44C745,&(add_esp[2]=36)-2,5); //add esp,36
		//2023: this releases textures and reinitializes
		//00443C5D E8 1E 00 00 00       call        00443C80
		memset((void*)0x443C5D,0x90,5);
		//
		//tile viewer texture subroutines
		//these are for TIM images, each is a different mode
		//00408C58 E8 A3 00 00 00       call        00408D00 //TIM
		//00408C42 E8 C9 05 00 00       call        00409210 //TIM
		//00408C04 E8 D7 0A 00 00       call        004096E0 //TIM
		*(DWORD*)0x408C59 = (DWORD&)map_408d00_409210_4096e0-0x408C5d;
		*(DWORD*)0x408C43 = (DWORD&)map_408d00_409210_4096e0-0x408C47;
		*(DWORD*)0x408C05 = (DWORD&)map_408d00_409210_4096e0-0x408C09;
		//00407000 E8 6B 04 00 00       call        00407470 //TXR
		*(DWORD*)0x407001 = (DWORD&)map_442830_407470-0x407005;
		//
		//tile viewer drawing subroutine
		//0041D5C7 E8 34 F2 02 00       call        0044C800 //model load?
		//0041DC9F E8 5C EB 02 00       call        0044C800 //model load?
		//0044C7CF E8 2C 00 00 00       call        0044C800 //repaint
		//0044E29A E8 61 E5 FF FF       call        0044C800 //setup //probably unused
		//0044E31A E8 E1 E4 FF FF       call        0044C800 //manip? not sure
		//manip and double-clicks (invisible to Ghidra)
		//0044E162 E8 99 E6 FF FF       call        0044C800
		//0044E20D E8 EE E5 FF FF       call        0044C800
		*(DWORD*)0x41D5C8 = (DWORD&)map_44c800-0x41D5Cc;
		*(DWORD*)0x41DCa0 = (DWORD&)map_44c800-0x41DCa4;
		*(DWORD*)0x44C7d0 = (DWORD&)map_44c800-0x44C7d4;
		*(DWORD*)0x44E29b = (DWORD&)map_44c800-0x44E29f;
		*(DWORD*)0x44E31b = (DWORD&)map_44c800-0x44E31f;
		*(DWORD*)0x44E163 = (DWORD&)map_44c800-0x44E167;
		*(DWORD*)0x44E20e = (DWORD&)map_44c800-0x44E212;
		//44E31A subroutine (40BE00) calls IDirectDraw4::RestoreAllSurfaces
		//0044E2BE E8 3D DB FB FF       call        0040BE00
		//0040BE22 FF 51 64             call        dword ptr [ecx+64h] 
		/*looks best to cut out the whole subroutine
		memset((void*)0x40BE22,0x90,3);*/
		memset((void*)0x44E2BE,0x90,5);
	}
	//tileview population (layers/neighborhood?)	
	//0041BA2F E8 6C 00 00 00       call        0041BAA0
	*(DWORD*)0x41BA30 = (DWORD&)map_41baa0-0x41BA34;
	//teardown
	//SOM_MAP_403780 //this function had already existed
	//OPTIMIZING
	//drawing?
	//0044449D E8 8E F5 FC FF       call        00413A30
	//CRUD: 9 tiles? (41B9CE)		
	//0044C944 E8 E7 70 FC FF       call        00413A30
	//0044CA5D E8 CE 6F FC FF       call        00413A30
	//0044CB5E E8 CD 6E FC FF       call        00413A30 	
	//0044CC5F E8 CC 6D FC FF       call        00413A30
	//0044CD60 E8 CB 6C FC FF       call        00413A30
	//0044CE61 E8 CA 6B FC FF       call        00413A30
	//0044CF62 E8 C9 6A FC FF       call        00413A30 
	//0044D063 E8 C8 69 FC FF       call        00413A30 
	//0044D164 E8 C7 68 FC FF       call        00413A30
	*(DWORD*)0x44449e = (DWORD&)bsearch_for_413a30-0x4444a2;
	WORD _9_words[10] = {0xCC5F,0xC944,0xCA5D,0xCB5E,0xCD60,0xCE61,0xCF62,0xD063,0xD164};
	for(WORD*w=_9_words;*w;w++)
	*(DWORD*)(0x440000+*w+1) = (DWORD&)bsearch_for_413a30-(0x440000+*w+5);

	//MDL+MDO
	//00409DA4 E8 27 E7 FF FF       call        004084D0
	//00409E84 E8 47 E6 FF FF       call        004084D0
	//00409F34 E8 97 E5 FF FF       call        004084D0
	//som.art.cpp (MDO)
	//00409D5E E8 AD E2 FF FF       call        00408010 //object
	//00409C44 E8 C7 E3 FF FF       call        00408010 //item
	*(DWORD*)0x409DA5 = (DWORD&)map_4084d0-0x409DA9;
	*(DWORD*)0x409E85 = (DWORD&)map_4084d0-0x409E89;
	*(DWORD*)0x409F35 = (DWORD&)map_4084d0-0x409F39;
	*(DWORD*)0x409D5F = (DWORD&)map_4084d0-0x409D63; //object
	*(DWORD*)0x409C45 = (DWORD&)map_4084d0-0x409C49; //item
	//D3D6? "tileview" has this covered! 
	//0040AB98 E8 F3 05 00 00       call        0040B190
	//0040ABD8 E8 B3 05 00 00       call        0040B190
	//*(DWORD*)0x40AB99 = (DWORD&)map_40b190-0x40AB9d;
	//*(DWORD*)0x40ABD9 = (DWORD&)map_40b190-0x40ABDd;
	//skins?
	//0044D601 E8 AA C8 FB FF       call        00409EB0 //npc
	*(DWORD*)0x44D602 = (DWORD&)map_409eb0-0x44D606;
	//0044D391 E8 6A CA FB FF       call        00409E00 //enemy
	*(DWORD*)0x44D392 = (DWORD&)map_409e00-0x44D396;
	//bypass object MDO/MDL extension tests (art)
	//overwriting these (they don't matter)
	//00409D28 8D 44 24 2C          lea         eax,[esp+2Ch]
	*(DWORD*)0x409D28 = 0x74eb9090; //jmp 409da0
	//MSM (2022)
	//00442356 E8 E5 00 00 00       call        00442440 
	//00409B8A E8 D1 E0 FF FF       call        00407C60
	*(DWORD*)0x442357 = (DWORD&)map_442440-0x44235b;
	*(DWORD*)0x409B8b = (DWORD&)map_407c60-0x409B8f;
	
	if(1) //EXPERIMENTAL
	{
		//2021: enhanced view for gray grids? and startup optimization
		
		//SOM_MAP makes two sets of tiles and uses GetPixel/SetPixelV
		//to build the second set, which is extremely slow, even slower
		//than loading everything else. I found this out while trying
		//to add another set of gray tiles for the other views. since
		//this code is able to use PlgBlt to avoid the second set, it
		//can use that second set to store the gray tiles

		//can this draw the tiles on the elements view?
		//CDC::FillSolidRect
		//00444F8D E8 36 7C 02 00       call        0046CBC8 //gray
		//00444F6B E8 58 7C 02 00       call        0046CBC8 //start
		//0044576B E8 58 74 02 00       call        0046CBC8 //checkpoint
		*(DWORD*)0x444F8e = (DWORD&)map_46cbc8-0x444F92;
		*(DWORD*)0x444F6c = (DWORD&)map_46cbc8-0x444F70;
		*(DWORD*)0x44576c = (DWORD&)map_46cbc8-0x445770;

		//OPTIMIZING
		//don't grow image lists 1 at a time!
		//0040F871 6A 01                push        1
		//0040F8B0 6A 01                push        1
		*(BYTE*)0x40F872 = *(BYTE*)0x40F8B1 = 256;

		//0040F963 E8 68 00 00 00       call        0040F9D0
	//	*(DWORD*)0x40F964 = (DWORD&)map_40f9d0-0x40F968;
		int sz_1 = sizeof(*map_40f9d0_lut)-1;
		float l_sz = 1.0f/(sz_1);
		auto *lut = new BYTE[sz_1+1];
		(void*&)map_40f9d0_lut = lut; for(int i=sz_1;i-->0;)
		{
			lut[i] = 48+(BYTE)(sz_1*(1-powf(1-i*l_sz,4.0f)))/2;
		}
		lut[0] = 0; //2023 (by request)

		//if(EX::debug) //art system icons
		{
			//2023: trying to load x2mdl.dll icons
			//0040F192 E8 99 06 00 00       call        0040F830
			*(DWORD*)0x40F193 = (DWORD&)map_40f830-0x40F197;
		}

		//if(1)
		{
			//always draw with StretchBlt?
			//004444C0 FF 24 85 A8 4A 44 00 jmp         dword ptr [eax*4+444AA8h]
			*(WORD*)0x4444C0 = 0x9090; //nop
			*(BYTE*)0x4444C2 = 0xe9; //jmp
			*(DWORD*)0x4444C3 = 268; //case 2 (eax is 2)
			//004446D3 FF 15 54 80 47 00    call        dword ptr ds:[478054h] 
			*(WORD*)0x4446D3 = 0xe890; //nop; call
			*(DWORD*)0x4446D5 = (DWORD)SOM_MAP_PlgBlt_icons-0x4446D9;
			//2023: increase icons to 21x21
			//004442b1 6a 14           PUSH       0x14
			//004442b3 6a 14           PUSH       0x14
			*(BYTE*)0x4442b2 = *(BYTE*)0x4442b4 = GEN_ICONS_SZ; //20->25
			//2023: remove grid lines?
			//0044437D 7C 44                jl          004443C3
			//004443C5 7C 3C                jl          00444403
			*(BYTE*)0x44437d = *(BYTE*)0x4443C5 = 0xeb; //jmp
			//00444DB5 7C 4E                jl          00444E05 
			//00444E07 7C 44                jl          00444E4D 
			*(BYTE*)0x444DB5 = *(BYTE*)0x444E07 = 0xeb; 
			//004455F3 7C 41                jl          00445636
			//00445638 7C 39                jl          00445673
			*(BYTE*)0x4455F3 = *(BYTE*)0x445638 = 0xeb; 
			//2023: grid supplement //444070
			//HACK: 444070 might require something like Detours
			//004449AD FF 15 7C 80 47 00    call        dword ptr ds:[47807Ch] 
			//004452E3 FF 15 7C 80 47 00    call        dword ptr ds:[47807Ch]
			//0044593E FF 15 7C 80 47 00    call        dword ptr ds:[47807Ch]
			*(WORD*)0x4449AD = 0xe890;
			*(DWORD*)0x4449Af = (DWORD)SOM_MAP_BitBlt_grid-0x4449b3;
			*(WORD*)0x4452E3 = 0xe890;
			*(DWORD*)0x4452E5 = (DWORD)SOM_MAP_BitBlt_grid-0x4452E9;
			*(WORD*)0x44593E = 0xe890;
			*(DWORD*)0x445940 = (DWORD)SOM_MAP_BitBlt_grid-0x445944;
			//0042FD6E FF 15 7C 80 47 00    call        dword ptr ds:[47807Ch]
			*(WORD*)0x42FD6E = 0xe890;
			*(DWORD*)0x42FD70 = (DWORD)SOM_MAP_BitBlt_grid-0x42FD74; //palette
			//0044421A 8B 3D 8C 80 47 00    mov         edi,dword ptr ds:[47808Ch]
			*(WORD*)0x44421A = 0xbf90; //mov edi,imm
			*(void**)0x44421c = SOM_MAP_CreateCompatibleBitmap;
			//0042FA3C FF 15 8C 80 47 00    call        dword ptr ds:[47808Ch] 
			*(WORD*)0x42FA3C = 0xe890;
			*(DWORD*)0x42FA3e = (DWORD)SOM_MAP_CreateCompatibleBitmap-0x42FA42;
			//00444C65 FF 15 8C 80 47 00    call        dword ptr ds:[47808Ch] 
			*(WORD*)0x444C65 = 0xe890;
			*(DWORD*)0x444C67 = (DWORD)SOM_MAP_CreateCompatibleBitmap-0x444C6b;
			//445501 //checkpoints
			*(WORD*)0x445501 = 0xe890;
			*(DWORD*)0x445503 = (DWORD)SOM_MAP_CreateCompatibleBitmap-0x445507;
			//0042FBB7 FF 15 24 80 47 00    call        dword ptr ds:[478024h]
			*(WORD*)0x42FBB7 = 0xe890;
			*(DWORD*)0x42FBB9 = (DWORD)SOM_MAP_ImageList_Draw_palette-0x42FBBd;
			//42fc78: adjust focus (SOM_MAP_AfxWnd42sproc adjusts click)
			*(WORD*)0x42fc78 = 0xe890;
			*(DWORD*)0x42fc7a = (DWORD)SOM_MAP_OffsetRect_palette-0x42fc7e;
			//inverting the alpha channel makes this unnecessary
			//0042fab2: nop gray clear (SOM_MAP_CreateCompatibleBitmap clears)
			//memcpy((void*)0x42fab2,"\x83\xc4\x14\x90\x90",5); //add esp,0x14

			//remove gray from 4 colored circles?
			//
			// NOTE: there's some AA artifacts in the original images that this
			// won't remove by itself. the SOM_MAP.exe language pack can remove
			// them... it's just a 1 shade difference, probably not intentional
			// 
			//00444FE8 FF 15 7C 80 47 00    call        dword ptr ds:[47807Ch]
			//0044504A FF 15 7C 80 47 00    call        dword ptr ds:[47807Ch]
			//004450A0 FF 15 7C 80 47 00    call        dword ptr ds:[47807Ch]
			//004450FE FF 15 7C 80 47 00    call        dword ptr ds:[47807Ch]
			WORD _[4] = {0x4FE8,0x504A,0x50A0,0x50FE}; for(int i=4;i-->0;)
			{
				DWORD addr = 0x440000+_[i]; *(WORD*)addr = 0xe890; //nop; call
				*(DWORD*)(addr+2) = (DWORD)SOM_MAP_TransparentBlt_grays-(addr+6);
			}
			//CHECKPOINT INPUT POLISH
			//004462CD E8 5E D8 FC FF       call        00413B30 //down?
			//00446635 E8 F6 D4 FC FF       call        00413B30 //drag?
			//004460FF E8 2C DA FC FF       call        00413B30 //right?
			//004464EF E8 3C D6 FC FF       call        00413B30 //right drag?
			*(DWORD*)0x4462Ce = (DWORD&)map_413b30_checkpoint-0x4462d2;
			*(DWORD*)0x446636 = (DWORD&)map_413b30_checkpoint-0x44663a;
			*(DWORD*)0x446100 = (DWORD&)map_413b30_checkpoint-0x446104;
			//knockout the branch into the calling subroutine
			//*(DWORD*)0x4464f0 = (DWORD&)map_413b30_checkpoint-0x4464f4; //UNUSED
			*(BYTE*)0x445e9b = 0xeb; //jmp
			//test Ctrl state (inverting?)
			//004462E1 FF 15 54 84 47 00    call        dword ptr ds:[478454h]
			*(WORD*)0x4462E1 = 0xe890; //nop; call
			*(DWORD*)0x4462E3 = (DWORD)SOM_MAP_GetKeyState_Ctrl-0x4462E7;
			//00446654 8B 2D 54 84 47 00    mov         ebp,dword ptr ds:[478454h]
			/*this strategy overrides VK_SHIFT too (at 44667B)
			*(WORD*)0x446654 = 0xbd90; //mov immediate
			*(DWORD*)0x446656 = (DWORD)SOM_MAP_GetKeyState_Ctrl;*/
			//this frees up one instruction (VK_CONTROL isn't needed) for call
			*(BYTE*)0x446654 = 0xbd; //mov ebp,GetKeyState
			*(DWORD*)0x446655 = (DWORD)GetKeyState;
			*(BYTE*)0x446659 = 0xe8; //call
			*(DWORD*)0x44665a = (DWORD)SOM_MAP_GetKeyState_Ctrl_void-0x44665e;
		}		

		//2023: checkpoints overlay?
		//00444716 0F 84 F0 00 00 00    je          0044480C
		memset((void*)0x444716,0x90,6);
		//004447C9 FF 15 A4 80 47 00    call        dword ptr ds:[4780A4h] 
		*(WORD*)0x4447C9 = 0xe890;
		*(DWORD*)0x4447Cb = (DWORD)SOM_MAP_InvertRgn-0x4447Cf;
	}

	//2021: skins and pr2 reading bugs (formerly som_tool_pr2)
	{
		//0041022C E8 3E 71 04 00       call        0045736F
		*(DWORD*)0x41022d = (DWORD)SOM_MAP_45736f_npc_pr2-0x410231;
		//0041024F 74 0F                je          00410260
		*(BYTE*)0x410250-=3;
		//00410331 E8 39 70 04 00       call        0045736F //fread
		*(DWORD*)0x410332 = (DWORD)SOM_MAP_45736f_enemy_pr2-0x410336;
		//00410351 74 0F                je          00410362
		*(BYTE*)0x410352-=3;
	}

	/////////// 2022 ////////////////////////////////

	//if(EX::debug) //new map streaming instruction?
	{
		//add room for zindex extension
		//REMINDER: som_map_instruct is 26+1 :(
		auto ed = SOM_MAP.evt_desc(27-1); //move object
		ed->data[0]+=4;
		//0042CFDF 66 C7 45 02 1C 00    mov         word ptr [ebp+2],1Ch 
		*(BYTE*)0x42CFe3 = 0x1c+4;

		//convert evt code to descriptor		
		//0040E758 E8 D3 FD FF FF       call        0040E530 
		//00412ADE E8 4D BA FF FF       call        0040E530  
		//00412F37 E8 F4 B5 FF FF       call        0040E530  
		//0042A52D E8 FE 3F FE FF       call        0040E530  
		//0042A9D6 E8 55 3B FE FF       call        0040E530  
		//0042ABEA E8 41 39 FE FF       call        0040E530 //Ghidra?
		//0042AEF5 E8 36 36 FE FF       call        0040E530
		//0042AF9C E8 8F 35 FE FF       call        0040E530 //Ghidra?
		*(DWORD*)0x40E759 = (DWORD)SOM_MAP.evt_code-0x40E75d;
		*(DWORD*)0x412ADf = (DWORD)SOM_MAP.evt_code-0x412Ae3;
		*(DWORD*)0x412F38 = (DWORD)SOM_MAP.evt_code-0x412F3c;
		*(DWORD*)0x42A52e = (DWORD)SOM_MAP.evt_code-0x42A532;
		*(DWORD*)0x42A9D7 = (DWORD)SOM_MAP.evt_code-0x42A9Db;
		*(DWORD*)0x42ABEb = (DWORD)SOM_MAP.evt_code-0x42ABEf;
		*(DWORD*)0x42AEF6 = (DWORD)SOM_MAP.evt_code-0x42AEFa;
		*(DWORD*)0x42AF9d = (DWORD)SOM_MAP.evt_code-0x42AFa1;
		//index descriptor
		//0042A4D0 E8 9B 40 FE FF       call        0040E570
		//0042A50A E8 61 40 FE FF       call        0040E570
		*(DWORD*)0x42A4D1 = (DWORD)SOM_MAP.evt_desc-0x42A4D5;
		*(DWORD*)0x42A50b = (DWORD)SOM_MAP.evt_desc-0x42A50f;

		//HACK: have to inject this one
		//0042B210 E8 4B A0 03 00       call        00465260 
		*(DWORD*)0x42B211 = (DWORD&)map_465260_standby_map-0x42B215;
	}

	//if(EX::debug) //extending event protocols (z/rotation)
	{
		//when I designed the layout for this I didn't think about
		//how SOM_MAP would restructure the data (instead of using
		//the EVT layout) so I had to get creative to make it work
		//unfortunately to do this (without too much complexity) I
		//had to keep the existing limit of 1 decimal place so the
		//new data can be compressed and uncompressed

		//0041299B E8 CF 49 04 00       call        0045736F //fread
		*(DWORD*)0x41299c = (DWORD)SOM_MAP_evt_45736f-0x4129a0;

		//this subroutine creates a temporary copy of the event
		//object prior to creating the dialog... it's a good time
		//to decode the new data. note, the data is passed through
		//sprintf so it's cast to (double) although that cast can be
		//removed, it would be more invasive
		//004274DA E8 81 00 00 00       call        00427560
		*(DWORD*)0x4274Db = (DWORD)SOM_MAP_evt_427560-0x4274Df;
		//this calls EndDialog (re/encoding)
		//00428554 E8 9F F2 03 00       call        004677F8
		*(DWORD*)0x428555 = (DWORD)SOM_MAP_evt_4677F8-0x428559;

		//00412EDD E8 A1 48 04 00       call        00457783 //fwrite
		*(DWORD*)0x412EDe = (DWORD)SOM_MAP_evt_457783-0x412Ee2;

		//don't default to protocol #1 so that som_db.exe won't
		//pass unused events to 408cc0
		//0040E953 C6 46 08 01          mov         byte ptr [esi+8],1
		*(BYTE*)0x40E956 = 0;
	}

	//2022: hotkeys?
	//00419EB8 E8 03 01 00 00       call        00419FC0
	*(DWORD*)0x419EB9 = (DWORD&)map_419fc0_keydown-0x419EBd;

	//2023: big map editor?
	//00415FCC E8 3C 15 05 00       call        0046750D  
	*(DWORD*)0x415FCd = (DWORD&)map_46750D_minmax-0x415Fd1;
	//0041705F E8 7C C0 FF FF       call        004130E0
	*(DWORD*)0x417060 = (DWORD&)map_4130e0_minmax_map-0x417064;
	//this knocks out the MAXIMIZE button and 1 other (close probably)
	//00416d1d 74 22           JZ         LAB_00416d41
	*(BYTE*)0x416d1d = 0xEB; //jmp

	if(1) //palette reorder?
	{
		//004172CD E8 EE C6 FF FF       call        004139C0
		//0042FB52 E8 69 3E FE FF       call        004139C0
		*(DWORD*)0x4172Ce = (DWORD&)map_4139c0_palette-0x4172d2;
		*(DWORD*)0x42FB53 = (DWORD&)map_4139c0_palette-0x42FB57;
		//0043009F E8 FC 39 FE FF       call        00413AA0 
		*(DWORD*)0x4300a0 = (DWORD&)map_413aa0_palette-0x4300a4;
		//00418312 E8 19 B8 FF FF       call        00413B30
		*(DWORD*)0x418313 = (DWORD&)map_413b30_palette-0x418317;

		//direct fix for dynamic palette width/wraparound
		//00419F57 E8 24 62 01 00       call        00430180
		*(DWORD*)0x419F58 = (DWORD&)map_430180_palette-0x419F5c;	

		//fix scrolling on bottom margin (SOM_MAP_40000 adds 1)
		//004302be 8d 5a ff        LEA        EBX,[EDX + -0x1]
		*(char*)0x4302c0 = -2;
		//004302c9 83 c0 02        ADD        EAX,0x2
		*(BYTE*)0x4302cb = 3;
	}

	if(1) //pin tiles for bsp?
	{
		//00410d93 74 3e           JZ         LAB_00410dd3
		*(BYTE*)0x410d93 = 0xEB; //jmp
		*(BYTE*)0x410d94-=10;

		*(BYTE*)0x410dc9 = 0x50; //push eax
		*(BYTE*)0x410dca = 0x57; //push edi
		*(BYTE*)0x410dcb = 0xe8; //call
		*(DWORD*)0x410dcc = (DWORD)SOM_MAP_read_ev-0x410dd0;
		memcpy((void*)0x410dd0,"\x83\xc4\x08",3); //add esp,8
	}

	if(1) //SOM_MAP_undo //2024
	{
		//let Ctrl+Click copy empty cell data (to emulate Delete)
	//	*(WORD*)0x4300a9 = 0x9090;

		//undo callback
		*(WORD*)0x41ab8a = 0x9090; //rep movs
		*(WORD*)0x41AB98 = 0xE890; //InvalidateRect
		*(DWORD*)0x41AB9a = (DWORD)SOM_MAP_undo-0x41AB9e;

		//track undo state after undo buffer capture (41ae60)
		*(DWORD*)0x41ae9c = (DWORD)SOM_MAP_poll_undo-0x41aea0;
	}
}

//2017: fix for ctrl+click elevation/rotation clobbering bug
void __thiscall SOM_MAP_this::map_446190(int x, int y)
{	
	bool capture = 0!=GetCapture(); //446190 sets this
	int before = GetScrollPos(SOM_MAP.palette,SB_VERT); //2021

	if(som_map_tab!=1215&&GetKeyState(VK_SHIFT)>>15) //2023: selection
	{
		char *cw = (char*)this; 
		int iVar3 = x/0x15+*(int*)(cw+0x70);
		int yTop = y/0x15+*(int*)(cw+0x74);
		*(int*)(cw+0x78) = *(int*)(cw+0x98) = iVar3;
        *(int*)(cw+0x7c) = *(int*)(cw+0x9c) = yTop;
		SetRect((LPRECT)(cw+0x88),iVar3,yTop,iVar3,yTop);
		*(int*)(cw+0x60) = 1;
		*(int*)(cw+0x68) = 1;
		SetCapture(SOM_MAP.painting);
		//SetTimer(SOM_MAP.painting,2,500,(TIMERPROC)0x0);
	}
	else ((void(__thiscall*)(void*,int,int))0x446190)(this,x,y);

	//I think this converts an HWND into a CWnd (19f71c)
	auto *p = SOM_MAP_app::CWnd(som_tool);
		
	//auto *_i = SOM_MAP_app::CWnd(SOM_MAP.painting);	
	int yy = 99-_i[0x7C/4], xx = _i[0x78/4]; //grid coordinates?

	//2022: update palette/viewer for consistency
	if(som_map_tab!=1215) //1220?
	{
		if(som_map_tab==1220)
		if(HWND ctrlock=GetDlgItem(som_tool,1224)) //CtrlLock?
		{
			if(Button_GetCheck(ctrlock)) capture = true; //disable
		}
		else capture = true; /*legacy?*/ if(!capture)
		{
			auto &sel = repair_pen_selection();
			if(sel.part!=0xffff)
			((void(__thiscall*)(void*,int))0x41b1f0)(p,sel.part); //?
		}
	}			 

	//HACK: this is to fix a bug where the palette view's scrollbar
	//moves and that makes it select a tile, clobbering the rotation
	//and elevation information. so the first call may do this and the
	//second (added) call will not, since the palette's been resituated
	if(before!=GetScrollPos(SOM_MAP.palette,SB_VERT)) //4463ca
	{
		if(repair_pen_selection().part==0xffff) assert(0);

		((void(__thiscall*)(void*,int,int))0x418110)(p,yy,xx);
	}

	Sleep(7); //2024: draw-dragging is skipping cells //BLACK MAGIC
}
void __thiscall SOM_MAP_this::map_446530(int x, int y)
{
	Sleep(7); //2024: draw-dragging is skipping cells //BLACK MAGIC

	if(GetKeyState(VK_SHIFT)>>15) //som_map_tab!=1215
	{
	//	char *cw = (char*)SOM_MAP_app::CWnd(SOM_MAP.painting); 
		char *cw = (char*)this; 
		int iVar4 = x/0x15+*(int*)(cw+0x70);
		int iVar6 = y/0x15+*(int*)(cw+0x74);
		iVar4 = max(0,min(99,iVar4));
		iVar6 = max(0,min(99,iVar6));
		*(int*)(cw+0x78) = iVar4;
		*(int*)(cw+0x7c) = iVar6;
		if(*(int*)(cw+0x60))
		{
			//2023: maintain selection in other views //446681
			int iVar1 = *(int*)(cw+0x98);
			if(iVar4<iVar1) 
			{
				*(int*)(cw+0x88) = iVar4;
				*(int*)(cw+0x90) = iVar1;
			}
			else
			{
				*(int*)(cw+0x88) = iVar1;
				*(int*)(cw+0x90) = iVar4;
			}
			int iVar4 = *(int*)(cw+0x9c);
			if(iVar6<iVar4)
			{
				*(int*)(cw+0x8c) = iVar6;
				*(int*)(cw+0x94) = iVar4;
			}
			else
			{
				*(int*)(cw+0x8c) = iVar4;
				*(int*)(cw+0x94) = iVar6;
			}
			y = 99-_i[0x7C/4]; x = _i[0x78/4];
			((void(__thiscall*)(void*))0x446980)(this);
			auto *p = SOM_MAP_app::CWnd(som_tool);
			((void(__thiscall*)(void*,int,int))0x418110)(p,y,x);
			InvalidateRect(SOM_MAP.painting,0,0);
			return;
		}
	}

	((void(__thiscall*)(void*,int,int))0x446530)(this,x,y);
}
void __thiscall SOM_MAP_this::map_445fd0(int x, int y)
{
	if(!SOM_MAP_445fd0_dblclk(x,y,false))
	return ((void(__thiscall*)(void*,int,int))0x445fd0)(this,x,y);
}
static bool SOM_MAP_445fd0_dblclk(int x, int y, bool dblclk)
{
			dblclk = false; //NOT WORKING :(

	char *cw = (char*)SOM_MAP_app::CWnd(SOM_MAP.painting); //this;

	const float _ = 0.25f;
	float xf = (float)x/0x15+*(int*)(cw+0x70);
	float yf = (float)y/0x15+*(int*)(cw+0x74);
	xf = max(-0.25,min(99+_,xf));
	yf = max(-0.25,min(99+_,yf));

	RECT &sr = *(RECT*)(cw+0x88);

	if(!*(int*)(cw+0x60) //no selection?
	||xf<sr.left-_||xf>sr.right+1+_||yf<sr.top-_||yf>sr.bottom+1+_)
	return false;

	*(int*)(cw+0x78) = (int)xf;
	*(int*)(cw+0x7c) = (int)yf;

	auto *p = SOM_MAP_app::CWnd(som_tool);

	((void(__thiscall*)(void*))0x41ae60)(p);
	auto *undo = *(SOM_MAP_4921ac::Tile**)((int)p+0x88);
	auto *grid = SOM_MAP_4921ac.grid;

	float c = 0, s = 1; 
	//c = cosf(M_PI_2*(1+dblclk));
	//s = sinf(M_PI_2*(1+dblclk));
	int ctrl = SOM::Tool.GetKeyState(VK_CONTROL)>>15;
	if(HWND ctrlock=GetDlgItem(som_tool,1224)) //CtrlLock?
	if(Button_GetCheck(ctrlock)) ctrl = !ctrl;
	int shift = GetKeyState(VK_SHIFT)>>15;
	if(shift){ c = -c; s = -s; }
	
	if(!ctrl) //DELETE existing selection?
	((void(__thiscall*)(void*,int,int,int,int))0x413b70)
	(&SOM_MAP_4921ac,sr.left,sr.top,sr.right,sr.bottom);

	float piv[3][3] = {{1,0,0},{0,1,0},{-xf,-yf,1}};
	float mat[3][3] = {{c,-s,0},{s,c,0},{xf,yf,1}};
	Somvector::map(mat).postmultiply<3,3>(piv);
	float inv[3][3];
	Somvector::map(inv).copy<3,3>(mat).invert<3,3>();		
	
	//EXPERIMENTAL (unfinished)
	int mirror = 0;	
	/*todo? work out spin according to quadrants below?
	if(mirror=dblclk&&ctrl)
	{		
		float cx = (sr.right+sr.left)*0.5f;
		float cy = (sr.bottom+sr.left)*0.5f;
		float th = 0.25f;
		mirror|=yf>cy+th||yf<cy-th?2:0;
		float ty = 1.0f/(sr.bottom-sr.top);
		mirror&=xf>cx+th||xf<cx-th;
	}*/

	int passes = 1+dblclk; //90+90=180?
	for(int pass=1;pass<=passes;pass++)
	{
		float lt[3] = {sr.left+0.5f,sr.top+0.5f,1.0f};
		Somvector::map(lt).premultiply<3>(mat);
		float rb[3] = {sr.right+0.5f,sr.bottom+0.5f,1.0f};
		Somvector::map(rb).premultiply<3>(mat);
	
		sr.left = (int)(shift?rb:lt)[0];
		sr.top = (int)(shift?lt:rb)[1];
		sr.right = (int)(shift?lt:rb)[0];
		sr.bottom = (int)(shift?rb:lt)[1];	

		sr.left = max(0,min(99,sr.left));
		sr.top = max(0,min(99,sr.top));
		sr.right = max(0,min(100,sr.right));
		sr.bottom = max(0,min(99,sr.bottom));

		//NOTE: the following loop doesn't work to set
		//c and s to pi
		if(pass!=passes) continue;
		
		int my = sr.top; //mirror?

		for(int yy=sr.bottom+1;yy-->sr.top;my++)
		{
			auto *row = grid+(99-yy)*100;

			int mx = sr.left, myy = mirror&1?my:yy;

			for(int xx=sr.right+1;xx-->sr.left;mx++)
			{
				int mxx = mirror&2?mx:xx; //mirror?

				float r[3] = {mxx+0.5f,myy+0.5f,1.0f};

				Somvector::map(r).premultiply<3>(inv);

				row[xx] = undo[(int)r[0]+(99-(int)r[1])*100];

				if(shift)
				{
					if(255==--row[xx].spin) row[mxx].spin = 3;
				}
				else row[xx].spin = (row[mxx].spin+1)%4;
			}
		}
	}

	((void(__thiscall*)(void*,int,int))0x418110)(p,y,x);
	InvalidateRect(SOM_MAP.painting,0,0);

	return true;
}

void __thiscall SOM_MAP_this::tileview()
{
	/*REMOVE ME
	auto stack = (DWORD*)SOM_MAP_app::CWnd(som_map_tileview);
	auto test = stack[26]+0x3C;
	assert((DWORD)this==test);*/

	DWORD count, begin;
	//NOTE: the model table is stored in the reverse order
	//of this switch table's enum values
	static int f = 0; switch(++f)
	{
	case 7: f = 1;
	case 1: //tiles

		if(som_map_tileviewmask&4) //EXTENSION
		{
			bool d3d9 = DDRAW::compat=='dx9c'; //2021
			
			auto m = (BYTE*)this+0x85620;

			//for some reason there's room for 16
			//but only 9 are ever displayed, so in
			//the remaining 7 just the cores of the
			//extra layers are stored
			int n = som_map_tileviewmask&0x20?16:9;

			for(int i=0;i<n;i++,m+=0x204)
			{
				if(*m&&m[256]&1) if(d3d9)
				{
					draw_model(m); //40ac40
				}
				else ((void (__thiscall*)(void*,void*))0x40ac40)(this,m);
			}

			m = (BYTE*)SOM_MAP_48_msm_mem;
			n = m&&n==16&&som_map_tileviewmask&1?6*8:0;
			for(int i=0;i<n&&*m;i++,m+=0x204)
			{
				if(d3d9) draw_model(m); //40ac40				
				else ((void (__thiscall*)(void*,void*))0x40ac40)(this,m);
			}
		}
		return;		

	case 2: //items

		count = 256; begin = 0x65220; break;

	case 3: //objects

		count = 512; begin = 0x24A20; break;

	case 4: case 5: //enemies/NPCs 

		count = 128; begin = f==4?0x14820:0x4620; break;

	case 6: //mystery??? //single mode
						
		assert(0==*((BYTE*)this+0x441c));

		if(!DDRAW::compat) //'_dx6'?
		{
			//just passing through for now
			((void (__thiscall*)(void*))0x0040AC00)(this);
		}
		return; //break;
	}

	DWORD i,selection = i = 0;

	DWORD esi = (DWORD)this+begin;
		
	#ifdef SOM_TOOL_enemies2
	{
		if(f==4)
		{
			count = SOM_MAP_4921ac.enemies_size; 
			esi = (DWORD)SOM_MAP_4921ac.enemies3.data();
		}
	}
	#endif
		
	if(f>=2&&f<=5) 
	{
		bool clip = 2&~som_map_tileviewmask;

		HWND tv = GetDlgItem(som_map_tileview,1052);

		HTREEITEM e, gp, sel = TreeView_GetSelection(tv);

		if(sel&&(gp=TreeView_GetParent(tv,sel))) 
		{
			if(e=TreeView_GetParent(tv,gp)){ sel = gp; gp = e; }
		}
		else sel = gp = 0;

		int top = 0; 
		while(gp=TreeView_GetPrevSibling(tv,gp)) 
		top++;
		if(sel&&f==top+2)
		{
			wchar_t num[64] = L""; 
			TVITEMEXW tvi = {TVIF_TEXT,sel};
			tvi.pszText = num; tvi.cchTextMax = EX_ARRAYSIZEOF(num);
			if(SendMessageW(tv,TVM_GETITEMW,0,(LPARAM)&tvi)&&*num)
			{
				wchar_t *ep;
				i = wcstoul(num+1,&ep,10);				
				selection = ep==num+1?0:esi+i*0x204;

				if(clip)
				{
					if(!selection) return;

					esi = selection; 

					count = 1; 
				}
			}
			else if(clip) return;
		}
		else if(clip) return;
	}

	typedef float debug[0x204/4];	
	debug &db = *(debug*)selection;
	struct{ FLOAT mat[4][4], unknown[2], xyz[3]; }swap;
	
	bool rec = false; if(selection)
	{
		if(count==1) som_map_tileviewmask^=2; 
		memcpy(&swap,(BYTE*)selection+0xA8,sizeof(swap));		
		if(IsWindowEnabled(GetDlgItem(som_map_tileview,1047)))
		{
			rec = true; //2021

			enum{ dN=10 }; WCHAR d[dN];
			float terms[7] = {0,0,0,0,0,0,1};						
			if(f==4) terms[4] = SOM_MAP_4921ac.enemies[i].uwv[0];
			if(f==4) terms[5] = SOM_MAP_4921ac.enemies[i].uwv[2];
			if(f==5) terms[4] = SOM_MAP_4921ac.NPCs[i].uwv[0];
			if(f==5) terms[5] = SOM_MAP_4921ac.NPCs[i].uwv[2];
			int ids[7] = {1025,1026,1027,1031,1032,1033,1035};
			for(int i=0;i<7;i++)
			if(GetDlgItemText(som_map_tileviewinsert,ids[i],d,dN))
			terms[i] = wcstod(d,0);	
			for(int i=3;i<6;i++)
			terms[i]*=SOMVECTOR_PI/180;
			int props = 0xFFFF; switch(f)
			{
			case 2: props = SOM_MAP_4921ac.items[i].props; break;
			case 3:	props = SOM_MAP_4921ac.objects[i].props; break;
			case 4: props = SOM_MAP_4921ac.enemies[i].props; break;
			case 5: props = SOM_MAP_4921ac.NPCs[i].props; break;
			}
			float sz = terms[6];
			if(props<1024) switch(f)
			{
			case 5:	sz*=SOM_MAP_4921ac.NPCprops[props].size; break;
			case 4: sz*=SOM_MAP_4921ac.enemyprops[props].size; break;			
			case 3:	sz*=SOM_MAP_4921ac.objectprops[props].size; break;			
			}			
			float quat[4]; //yuck: mixing AA and Euler
			float uv[3] = {terms[3],terms[4],0}, w[4];
			Somvector::map(w).quaternion<0,0,1>(terms[5]);
			Somvector::map(quat).quaternion(uv).premultiply(w);	
			float size[4][4]={{sz,0,0,0},{0,sz,0,0},{0,0,sz,0},{0,0,0,1}};			
			if(f==2&&props<250)
			{
				size_t profile = SOM_MAP_4921ac.itemprops[props].profile;

				if(profile<250) //repurposing w
				{
					size[3][1] = -SOM_MAP_4921ac.itemprofs[profile].pivot;

					Somvector::map(w).quaternion<1,0,0> 
					(SOMVECTOR_PI/180*SOM_MAP_4921ac.itemprofs[profile].v);
					Somvector::map(quat).premultiply(w);
				}
			}
			FLOAT *mat = (float*)(selection+0xA8);
			Somvector::map<4,4>(mat).copy_quaternion<4,4>(quat).postmultiply<4,4>(size);
			FLOAT *xyz = (float*)(selection+0xF0);
			Somvector::map<3>(xyz).copy<3>(terms);			
		}
	}

	//2021: hide neighbors
	auto tp = (BYTE*)this;	
	auto cmp = (WORD)SOM_MAP_tileviewtile;

	//2021: there was a bug where i was 0 even though
	//esi was advanced to the selection (only the new
	//layer feature seemed to be comprimised by this)
	DWORD n = count; if(n>1) i = 0; else n+=i;

	for(;i<n;i++,esi+=0x204) if(*(BYTE*)esi)
	{	 
		float *z = 0, baseZ;
		SOM_MAP_4921ac::Start *base; //if(SOM_MAP.base)
		{
			switch(f)
			{
			case 2: base = SOM_MAP_4921ac.items+i; break;
			case 3:	base = SOM_MAP_4921ac.objects+i; break;
			case 4: base = &SOM_MAP_4921ac.enemies[i]; break;
			case 5: base = SOM_MAP_4921ac.NPCs+i; break;
			}

			//2021: hide neighbors?
			if(cmp!=*(WORD*)base->tile)
			{
				if((som_map_tileviewmask&0x11)!=0x11)
				{
					continue; //NOTE: can't be selection
				}
			}

			if(base->ex_zindex!=SOM_MAP_z_index)
			{
				if(~som_map_tileviewmask&0x20) 
				{
					if(esi!=selection) continue;
				}
				//2021: record button may wait until the
				//selection is edited
				if(!rec||esi!=selection)
				{
					z = (float*)(esi+0xF4);
					baseZ = *z;
					int xy = base->xyindex();
					if(SOM_MAP.base)
					*z+=SOM_MAP.base[xy].z;
					*z-=SOM_MAP_4921ac.grid[xy].z;
				}
			}
		}

		if(som_map_tileviewmask&2) //2021
		{
			auto m = (BYTE*)esi; if(!DDRAW::compat) //2021
			{	
				if(*m==3) //f==4||f==5||f==3&&3==*(BYTE*)esi //MDL?
				{
					((void(__thiscall*)(void*,DWORD))0x40B190)(this,esi);
				}
				else if(*m==2) //f==2||f==3 //MDO?
				{
					((void(__thiscall*)(void*,DWORD))0x40AE80)(this,esi);
				}
				else assert(0);
				//2021: 40A870 draws bounding box? unused?
			//	((void(__thiscall*)(void*,DWORD))0x40A870)(this,esi);
				assert(~m[256]&2);
			}
			else draw_model(m); //2021
		}

		if(z) *z = baseZ;
	}
	   	
	if(selection)
	{
		if(count==1) som_map_tileviewmask^=2; 

		memcpy((BYTE*)selection+0xA8,&swap,sizeof(swap));
	}	
}

static bool SOM_MAP_rec(DWORD dw, const SOM_MAP_4921ac::Start *ref)
{	
	//REMINDER: som_tool_ok implements Z-index logic for start point

	SOM_MAP_4921ac::Start *rec = (SOM_MAP_4921ac::Start*)dw;
	 		
	enum{ dN=10 };
	WCHAR d[dN],*e; float f; 
	for(int i=0;i<3;i++)
	if(GetDlgItemText(som_map_tileviewinsert,1025+i,d,dN))
	{
		f = wcstod(d,&e); if(*e||!*d) return false; rec->xzy[i] = f;
	}
	
	//EXCLUDING ITEMS?
	if(ref>=SOM_MAP_4921ac.objects&&ref<=SOM_MAP_4921ac.NPCs
	 ||ref==&SOM_MAP_4921ac.enemies[0])
	{
		GetDlgItemText(som_map_tileviewinsert,1035,d,dN); //scale?	

		f = wcstod(d,&e); if(*e||!*d) return false; *(FLOAT*)(rec+1) = f;
	}
	
	//NPC/enemy rotation about the X/Y ground axes
	for(int i=0;i<=2;i+=2)
	{
		BOOL x;
		rec->uwv[i] = GetDlgItemInt(som_map_tileviewinsert,1031+i,&x,0);
		if(x) continue;
		
		//NOTE: this is optional for legacy theme packages
		HWND ch = GetDlgItem(som_map_tileviewinsert,1031+i);  
				
		//2022: allow empty field to avoid a cryptic error
		//NOTE: it should be impossible to input whitespace
		if(ch&&GetWindowTextLength(ch)) return false;
	}	

	//REMINDER: should come after above GetDlgItemText calls
	rec->record_z_index(som_map_tileview); return true;
}
void SOM_MAP_4921ac::Start::record_z_index(HWND hw)
{
	//2018: restoring layer system
	if(HWND tb=GetDlgItem(hw,1005))
	{	
		int zi = SendMessage(tb,TBM_GETPOS,0,0);
		zi-=SendMessage(tb,TBM_GETRANGEMIN,0,0);
		this->ex_zindex = 0xFF&zi;
	}	
	if(SOM_MAP_z_index!=this->ex_zindex)
	{
		//inverse of SOM_MAP_z_height
		int xy = this->xyindex();
		if(SOM_MAP.base)
		this->xzy[1]-=SOM_MAP.base[xy].z;
		this->xzy[1]+=::SOM_MAP_4921ac.grid[xy].z;
	}
}

//these copy the tileview's settings
DWORD __thiscall SOM_MAP_this::map_414170(DWORD add)
{
	if(SOM_MAP_rec(add,&SOM_MAP_4921ac.enemies[0])) 
	{
		DWORD &es = SOM_MAP_4921ac.enemies_size;
		DWORD o = ((DWORD(__thiscall*)(void*,DWORD))0x414170)(this,add);
		if(o!=~0u)
		{
			assert(o<=es);
			if(o>=es) es = o+1; 
		}
		return o;
	}
	return(MessageBeep(-1),~0u);
}
DWORD __thiscall SOM_MAP_this::map_4142B0(DWORD i, DWORD rec)
{
	if(SOM_MAP_rec(rec,&SOM_MAP_4921ac.enemies[0]))
	return((DWORD(__thiscall*)(void*,DWORD,DWORD))0x4142B0)(this,i,rec);
	return(MessageBeep(-1),0);
}
DWORD __thiscall SOM_MAP_this::map_4141C0(DWORD add)
{
	if(SOM_MAP_rec(add,SOM_MAP_4921ac.NPCs))
	return((DWORD(__thiscall*)(void*,DWORD))0x4141C0)(this,add);
	return(MessageBeep(-1),~0u);
}
DWORD __thiscall SOM_MAP_this::map_414300(DWORD i, DWORD rec)
{
	if(SOM_MAP_rec(rec,SOM_MAP_4921ac.NPCs))
	return((DWORD(__thiscall*)(void*,DWORD,DWORD))0x414300)(this,i,rec);
	return(MessageBeep(-1),0);
}
DWORD __thiscall SOM_MAP_this::map_414210(DWORD add)
{
	if(SOM_MAP_rec(add,SOM_MAP_4921ac.objects))
	return((DWORD(__thiscall*)(void*,DWORD))0x414210)(this,add);
	return(MessageBeep(-1),~0u);
}
DWORD __thiscall SOM_MAP_this::map_414350(DWORD i, DWORD rec)
{
	if(SOM_MAP_rec(rec,SOM_MAP_4921ac.objects))
	return((DWORD(__thiscall*)(void*,DWORD,DWORD))0x414350)(this,i,rec);
	return(MessageBeep(-1),0);
}
DWORD __thiscall SOM_MAP_this::map_414260(DWORD add)
{
	if(SOM_MAP_rec(add,SOM_MAP_4921ac.items))
	return((DWORD(__thiscall*)(void*,DWORD))0x414260)(this,add);
	return(MessageBeep(-1),~0u);
}
DWORD __thiscall SOM_MAP_this::map_4143A0(DWORD i, DWORD rec)
{
	if(SOM_MAP_rec(rec,SOM_MAP_4921ac.items))
	return((DWORD(__thiscall*)(void*,DWORD,DWORD))0x4143A0)(this,i,rec);
	return(MessageBeep(-1),0);
}

extern "C" HRESULT WINAPI DirectDrawCreateEx(GUID*,LPVOID*,REFIID,IUnknown*);

extern bool (*som_exe_onflip_passthru)();
static bool SOM_MAP_onflip()
{
	bool out = som_exe_onflip_passthru();

	if(DDRAW::dejagrate)
	{
		assert(2==DDRAW::dejagrate);

		extern unsigned workshop_antialias;
		if(workshop_antialias) //HACK: WM_MOUSEMOVE
		{
			workshop_antialias = 0; goto aa;
		}
		
		//DUPLICATE //workshop_onflip
		if(DDRAW::noFlips%2||DDRAW::noTicks>66||DDRAW::noFlips<4)
		{
		aa: //2021: post?
			//InvalidateRect(DDRAW::window,0,0);
			RedrawWindow(DDRAW::window,0,0,RDW_INVALIDATE);
		}
	}

	if(int i=-DDRAW::fxInflateRect) //DUPLICATE (workshop.cpp)
	{
		DDRAW::doNothing = true;
		DX::D3DVIEWPORT7 vp; DDRAW::Direct3DDevice7->GetViewport(&vp);
		DX::D3DRECT clear = {0,0,vp.dwWidth,i}; //top
		//DDRAW::Direct3DDevice7->Clear(1,&clear,D3DCLEAR_TARGET,0xFF000000,0.0,0);
		DX::D3DRECT buf[4];
		buf[0] = clear;
		clear.y1 = clear.y2 = vp.dwHeight; clear.y1-=i; //bottom
		//DDRAW::Direct3DDevice7->Clear(1,&clear,D3DCLEAR_TARGET,0xFF000000,0.0,0);
		buf[1] = clear;
		clear.y1 = 0; clear.x1 = clear.x2-i; //right
		//DDRAW::Direct3DDevice7->Clear(1,&clear,D3DCLEAR_TARGET,0xFF000000,0.0,0);
		buf[2] = clear;
		clear.x1 = 0; clear.x2 = i; //left
		//DDRAW::Direct3DDevice7->Clear(1,&clear,D3DCLEAR_TARGET,0xFF000000,0.0,0);
		buf[3] = clear;
		DDRAW::Direct3DDevice7->Clear(4,buf,D3DCLEAR_TARGET,0xFF000000,0.0,0);
		DDRAW::doNothing = false;
	}

	return out;
}

//EXPERIMENTAL
std::vector<IUnknown*> SOM_MAP_d3d_release;
DWORD __thiscall SOM_MAP_this::map_442270(char *_1, int _2)
{
	if(!_1) //???
	{
		return 0; //breakpoint
	}

	//seems to go dead after a while and not spring back
	//at which point new msm models don't load
	auto &init = *(DWORD*)((char*)this+0x22fbc);
	if(0==init)
	{
		for(size_t i=SOM_MAP_d3d_release.size();i-->0;)
		SOM_MAP_d3d_release[i]->Release();
		SOM_MAP_d3d_release.clear();		
		assert(!DDRAW::DirectDraw7); //release?

		init = map_440ee0();
	}

	return ((DWORD(__thiscall*)(void*,char*,int))0x442270)(this,_1,_2);
}
DWORD __thiscall SOM_MAP_this::map_440ee0() //2021
{
	static int one_off = 0; if(!one_off)
	{
		one_off = true;
		som_exe_onflip_passthru = DDRAW::onFlip;
		DDRAW::onFlip = SOM_MAP_onflip;
	}

	//IDirect3D9 says this is a focus window?
	DDRAW::window = som_tool; assert(som_tool);

	//need to knockout any enumeration code to
	//avoid setting this since there's some code
	//in dx_ddraw_DirectDrawCreateEx that prevents
	//making two of them
//	assert(!DDRAW::DirectDraw7);
	if(DDRAW::DirectDraw7) return 1; //big map editor?

	DX::IDirectDraw7 *dd = 0;
	DX::IDirectDrawSurface7 *ps = 0, *ds = 0;
	DX::IDirect3D7 *d3d;
	DX::IDirect3DDevice7 *dev;	
	if(!DirectDrawCreateEx(0,(void**)&dd,DX::IID_IDirectDraw7,0))
	{		
		SOM_MAP_d3d_release.push_back(dd);

		//HACK: DirectDrawCreateEx is what seems
		//to trigger EX::is_needed_to_initialize
		
		//DUPLICATE //workshop.cpp
		DWORD z = DDRAW::doDither?16:32; SOM::bpp = z;
		extern bool som_hacks_setup_shaders(DWORD &z);
		if(som_hacks_setup_shaders(z))
		{
			DDRAW::vs = DDRAW::ps = 3;
		}
	}
	if(dd&&!dd->QueryInterface(DX::IID_IDirect3D7,(void**)&d3d))
	{
		SOM_MAP_d3d_release.push_back(d3d);

		DX::DDSURFACEDESC2 desc = {sizeof(desc)};
		//HACK: passing manual primary surface size
		//isn't allowed, but otherwise dx.d3d9c.cpp
		//uses DDRAW::window
		#ifdef NDEBUG
		//#error extract tile view width? or resize effects buffer?
		int todolist[SOMEX_VNUMBER<=0x1020602UL];
		#endif	
		desc.dwFlags = 0x27; //7|DDSD_BACKBUFFERCOUNT
		desc.dwWidth = 512; //401?
		desc.dwHeight = 512; //378?
		//copying som_db.exe
		//DDSCAPS_PRIMARYSURFACE|DDSCAPS_3DDEVICE|DDSCAPS_FLIP|DDSCAPS_COMPLEX;
		desc.ddsCaps.dwCaps = 0x2218;
		desc.dwBackBufferCount = 1;
		dd->CreateSurface(&desc,&ps,0);
		SOM_MAP_d3d_release.push_back(ps);
		desc.dwFlags = 0x1007; //7|DDSD_PIXELFORMAT	
		desc.ddsCaps.dwCaps = 0x24000; //DDSCAPS_ZBUFFER|DDSCAPS_VIDEOMEMORY
		desc.ddpfPixelFormat.dwSize = 32;
		desc.ddpfPixelFormat.dwFlags = 0x400;
		desc.ddpfPixelFormat.dwZBufferBitDepth = 16;
		dd->CreateSurface(&desc,&ds,0);
		SOM_MAP_d3d_release.push_back(ds);
		if(ps&&ds&&!d3d->CreateDevice(DX::IID_IDirect3DTnLHalDevice,ps,&dev))
		{
			SOM_MAP_d3d_release.push_back(dev);

			//needed just in case D3DFVF_LVERTEX leaks
			extern void *workshop_DrawIndexedPrimitive(HRESULT*,DDRAW::IDirect3DDevice7*,DX::D3DPRIMITIVETYPE&,DWORD&,LPVOID&,DWORD&,LPWORD&,DWORD&,DWORD&);
			DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_DRAWINDEXEDPRIMITIVE_HACK,workshop_DrawIndexedPrimitive); //2021

			//may need to do this on switching model/tile view
			float proj[4][4] = 
			{
				//just copying this out of memory at 441BD2 
				{0.923883975f,0.000000000f, 0.000000000f,0.000000000f},
				{0.000000000f,0.923883975f, 0.000000000f,0.000000000f},
				{0.000000000f,0.000000000f, 0.382682294f,0.382672727f},
				{0.000000000f,0.000000000f,-0.382682294f,0.000000000f},
			};
			dev->SetTransform(DX::D3DTRANSFORMSTATE_PROJECTION,(DX::LPD3DMATRIX)proj);
			
			dev->SetRenderState(DX::D3DRENDERSTATE_ZENABLE,1);
			dev->SetTextureStageState(0,DX::D3DTSS_MINFILTER,DX::D3DTFN_LINEAR);
			dev->SetTextureStageState(0,DX::D3DTSS_MAGFILTER,DX::D3DTFG_LINEAR);
			dev->SetTextureStageState(0,DX::D3DTSS_MIPFILTER,DX::D3DTFP_LINEAR);
			//TESTING (NOT GETTING A PICTURE)
			dev->SetRenderState(DX::D3DRENDERSTATE_LIGHTING,1); //DDRAW::isLit
		//	dev->SetRenderState(DX::D3DRENDERSTATE_AMBIENT,~0);
		//	dev->SetRenderState(DX::D3DRENDERSTATE_CULLMODE,DX::D3DCULL_NONE);

			//EXTENSION
			dev->SetRenderState(DX::D3DRENDERSTATE_ZBIAS,1);
			
			//copy ItemEdit???		
		//	dev->SetTextureStageState(0,DX::D3DTSS_COLORARG1,DX::D3DTA_TEXTURE);
		//	dev->SetTextureStageState(0,DX::D3DTSS_ALPHAARG1,DX::D3DTA_TEXTURE);
			//can't be both???
		//	dev->SetTextureStageState(0,DX::D3DTSS_COLORARG2,DX::D3DTA_DIFFUSE);
		//	dev->SetTextureStageState(0,DX::D3DTSS_COLORARG2,DX::D3DTA_CURRENT);
			//can't be both???
		//	dev->SetTextureStageState(0,DX::D3DTSS_ALPHAOP,DX::D3DTOP_SELECTARG2);
	//		dev->SetTextureStageState(0,DX::D3DTSS_ALPHAOP,DX::D3DTOP_MODULATE); //som.hacks.cpp?
		//	dev->SetTextureStageState(0,DX::D3DTSS_COLOROP,DX::D3DTOP_MODULATE); //default

			//HACK: popuplate SOM::material_textures?
			SOM::MT::lookup("");

			//2022: add definition to untextured MSM view?
			float w = 0.5f;
			DX::D3DLIGHT7 lit7 = {DX::D3DLIGHT_DIRECTIONAL,{w,w,w},{},{},{},{0.5,0.25,0.5}};
			float *yuck = &lit7.dvDirection.x;
			Somvector::map<3>(yuck).unit<3>();
			dev->SetLight(3,&lit7);
			yuck[1] = - yuck[1];
			dev->SetLight(4,&lit7);
		}
	}
	assert(DDRAW::Direct3DDevice7);

	return 1; //sets 0x22fbc to 1
}
extern bool Ex_output_lines(float*,int,int,WORD*,DWORD=0);
extern const DX::DDPIXELFORMAT &dx_d3d9c_format(D3DFORMAT f);
static bool SOM_MAP_texture(SOM_MAP_this::texture_t *o, char *name, int w, int h, std::function<int(BYTE*,int)> f)
{
	//HACK: this can prevent a glitch effect that 
	//happens when changing the MSM model if there's
	//a long delay between the two update frames caused
	//by mouse input events like dragging the main area
	DDRAW::fx2ndSceneBuffer = false;

	auto *dd = DDRAW::DirectDraw7;

	if(!dd){ assert(0); return false; }

	//DUPLICATE //workshop.cpp
	DX::DDSURFACEDESC2 desc = {sizeof(desc),0x1007};		
	desc.ddsCaps.dwCaps = 0x5000; 
	desc.dwWidth = w;
	desc.dwHeight = h;
	desc.ddpfPixelFormat = dx_d3d9c_format((D3DFORMAT)22); //D3DFMT_X8R8G8B8 

		int type;
		/*worth it?
		bool npt;
		int i = 0, j = 0;
		while(w<1<<i&&i<9) i++;
		while(h<1<<j&&j<9) j++;
		i = 1<<i; j = 1<<j;
		if(npt=w!=i||h!=j) //non-power-2? (Ex.mipmap.cpp)
		{
			desc.dwWidth = i;
			desc.dwHeight = j;
		}*/

	DX::IDirectDrawSurface7 *s = 0;	
	dd->CreateSurface(&desc,&s,0);
	if(s&&!s->Lock(0,&desc,DDLOCK_WAIT|DDLOCK_WRITEONLY,0))
	{
		/*worth it?
		if(npt)
		{
			w = i; h = j;
		}
		else*/ type = f((BYTE*)desc.lpSurface,desc.lPitch);

		s->Unlock(0);

		if(type)
		{
			DX::DDCOLORKEY black = {}; //allow black knockout 
			s->SetColorKey(DDCKEY_SRCBLT,&black);

			(void*&)o->dds4 = s; s->AddRef(); //I guess?
			(void*&)o->d3dt2 = s;

			//these are invalid in map_442270
			//SOM_MAP_d3d_release.push_back(s); //testing

			size_t len = strlen(name)+1;
			o->name = ((char*(__cdecl*)(DWORD))0x46573f)(len);
			memcpy(o->name,name,len);

			o->_1 = 1; o->_2 = type; //2

			o->width = w; o->height = h;

			o->TIM_ptr = 0; return true;
		}
	}
	if(s) s->Release(); return false;
}
extern char *som_MDL_skin; 
extern char SomEx_utf_xbuffer[];
extern wchar_t *som_art_CreateFile_w; 
extern int som_art(const wchar_t*, HWND);
extern int som_art_model(const char*, wchar_t[MAX_PATH]);
DWORD __thiscall SOM_MAP_this::map_442830_407470(texture_t *o, char *name) //2021
{
	if(som_MDL_skin) name = som_MDL_skin;

	bool padding = false; //2024

	if(name[20]=='.'&&name[19]=='\\') //2024
	{
		padding = true; goto padding;
	}

	bool map_texture = false; map_texture: //2024

	wchar_t art[MAX_PATH]; 	
	int e = som_art_model(name,art); //2021
	using namespace x2mdl_h;
	if(e&_art&&~e&_lnk)	
	if(!som_art(art,0)) //x2mdl exit code?
	{
		e = som_art_model(name,art); //retry?
	}
	if(0==(e&_txr)) //2022: fall back on map/texture?
	{
		if(!map_texture)
		if(*(DWORD*)(name+10)==0x5C70616D) //map/model?
		{
			//REMINDER: name needs to be preserved
			//to match the texture reuse lookup table
			char aa[64];
			memcpy(aa,"map\\texture",12);
			memcpy(aa+11,name+19,33);
			//if(EX::data(aa,art)) e|=_txr;
			strcpy(name,aa); 
			map_texture = true; goto map_texture;
		}
	}
	if(0==(e&_txr)) //return 0; //2021
	{
		//the empty string name isn't stored
		//in the texture lookup table. there
		//should be some way to avoid making 
		//dummy textures
		//
		// NOTE: I've fixed this for serious
		// matters but this technically does
		// count against the texture maximum
		//
		//might help to preempt this earlier
		int todolist[SOMEX_VNUMBER<=0x1020602UL];

		//2022: MSM models may be padded with 
		//empty textures to 32-bit align data
		padding:
		if(char*fn=strrchr(name,'\\'))
		if(!memcmp(fn,"\\.txr",6)) //was .TXR
		{
			assert(padding); //2024
			return 1;
		}
		return 0; //TODO: handle this error
	}
	////////////////////////////
	/////POINT-OF-NO-RETURN/////
	////////////////////////////
	//HACK: this is used to load files from
	//nonstandard locations
	auto swap = som_art_CreateFile_w;
	som_art_CreateFile_w = art;

	namespace txr = SWORDOFMOONLIGHT::txr;
	txr::image_t img;
	txr::maptofile(img,art,'r',1); if(img)
	{
		const auto map = txr::mipmap(img,0);

		//LAMBDA
		auto f = [&](BYTE *lp, int lpitch)->int
		{
			namespace txr = SWORDOFMOONLIGHT::txr;

			auto d = (txr::palette_t*)lp; 

			if(map.depth==8)
			{
				const txr::palette_t *pal = txr::palette(img);

				for(int i=0;i<map.height;i++,(char*&)d+=lpitch)
				{
					const txr::pixrow_t &row = map[i];					
					for(int j=0,jN=map.width;j<jN;j++)
					{
						d[j] = pal[row.indices[j]];
						//REMOVE ME?
						d[j]._ = 255;
					}
				}
			}
			else if(map.depth==24)
			{
				for(int i=0;i<map.height;i++,(char*&)d+=lpitch)
				{
					const txr::pixrow_t &row = map[i];					
					for(int j=0,jN=map.width;j<jN;j++)
					{
						d[j] = *(txr::palette_t*)&row.truecolor[j];
						//REMOVE ME? (SetColorKey has some logic like this)
						//TODO: DRAW::IDirectDrawSurface7::updating_texture
						//may need to do this when there's no colorkey data
						#ifdef NDEBUG
						//#error really should fix no colorkey (SetColorKey)
					//	int todolist[SOMEX_VNUMBER<=0x1020602UL];
						#endif
						d[j]._ = 255;
					}
				}
			}
			else{ assert(0); return 0; }return 2; //32?
		};
		if(map&&SOM_MAP_texture(o,name,map.width,map.height,f))
		{
			if(SOM::material_textures)
			{
				//TODO: just check som_map_tileviewport?
				auto tp = (BYTE*)this;
				int os = (BYTE*)o-tp;
				int mt = os>=0x22db8?1024-16:0; //texos?
				mt+=o-(texture_t*)(tp+(mt?0x22db8:0x41C));				
				if((unsigned)mt<1024)
				{
					//FIX ME?
					//this will skip / if used in the model
					//file, which x2mdl prefers to use, but
					//maybe it shouldn't depend on that
					//DUPLICATE: som_game_449530
					char *fn = strrchr(name,'\\'); assert(fn);
					(*SOM::material_textures)[mt] = fn?SOM::MT::lookup(fn+1):0;
				}
				else assert(0);
			}
		}
		else img.bad = 1; //HACK
	}

	som_art_CreateFile_w = swap; //POINT-OF-NO-RETURN

	return !txr::unmap(img);
}
DWORD __thiscall SOM_MAP_this::map_408d00_409210_4096e0(texture_t *o, char *name, BYTE *pp)
{
	namespace tim = SWORDOFMOONLIGHT::tim;
	//tim::maptorom(img,pp);
	const auto &head = *(tim::header_t*)pp;
	/*I guess these are lifted out of the file?
	const auto *data = (tim::pixmap_t*)(&head+1);
	const auto *clut = head.cf?data:0; if(clut)
	{
		size_t align = clut->bnum; 
		if(align%4) align+=4-align%4; 
		data = (const tim::pixmap_t*)((char*)clut+align);
	}*/
	const auto *clut = (tim::pixmap_t*)*(void**)(pp+8);
	const auto *data = (tim::pixmap_t*)*(void**)(pp+12);
	if(!data->operator!()) //YUCK
	{
		auto &pal = *clut, &map = *data;
		int map_width = map(head.pmode);
		int map_height = map.h;

		//LAMBDA
		auto f = [&](BYTE *lp, int lpitch)->int
		{
			namespace tim = SWORDOFMOONLIGHT::tim;

			int jN = map_width;
			if(tim::codexmode==head.pmode)
			jN/=2;
			for(int i=0;i<map_height;i++)
			{
				BYTE *p = lp; lp+=lpitch;

				const tim::pixrow_t &row = (*data)[i];
				
				switch(head.pmode)
				{
				case tim::truecolormode:
						
					assert(0); //not expecting this
					
					for(int j=0;j<jN;j++)
					{
						auto rgb = row.truecolor[j]; 
						p[0] = rgb.r;
						p[1] = rgb.g;
						p[2] = rgb.b;
						p[3] = 0xff; //REMOVE ME?
						p+=4;
					}					
					break;

				case tim::highcolormode:

					for(int j=0;j<jN;j++)
					{
						int rgb = row.highcolor[j].bits;
						p[2] = rgb<<3;
						p[1] = rgb>>2&0xf8;
						p[0] = rgb>>7&0xf8;;
						p[3] = 0xff; //REMOVE ME?
						p+=4;
					}
					break;

				case tim::indexmode:

					for(int j=0;j<jN;j++)
					{
						int rgb = (*clut)[0].highcolor[row.indices[j]].bits;
						p[2] = rgb<<3;
						p[1] = rgb>>2&0xf8;
						p[0] = rgb>>7&0xf8;
						p[3] = 0xff; //REMOVE ME?
						p+=4;
					}
					break;

				case tim::codexmode:

					for(int j=0;j<jN;j++)
					{
						int rgb = (*clut)[0].highcolor[row.codices[j].lo].bits;
						p[2] = rgb<<3;
						p[1] = rgb>>2&0xf8;
						p[0] = rgb>>7&0xf8;
						p[3] = 0xff; //REMOVE ME?
						p+=4;

						rgb = (*clut)[0].highcolor[row.codices[j].hi].bits;
						p[2] = rgb<<3;
						p[1] = rgb>>2&0xf8;
						p[0] = rgb>>7&0xf8; 
						p[3] = 0xff; //REMOVE ME?
						p+=4;
					}
					break;
				}
			}
			return 3;
		};
		if(SOM_MAP_texture(o,name,map_width,map_height,f))
		{
			o->TIM_ptr = pp;

			return 1;
		}
	}
	return 0;
}
struct SOM_MAP_trans
{
	SOM_MAP_model *model;

	float dist, pos[3], rot[16];

	SOM_MAP_trans(SOM_MAP_model *cp):model(cp)
	{
		memcpy(pos,cp->f+0xf0/4,sizeof(pos));
		memcpy(rot,cp->f+0xa8/4,sizeof(rot));
	}
	operator SOM_MAP_model()
	{
		SOM_MAP_model tmp = *model;
		memcpy(tmp.f+0xf0/4,pos,sizeof(pos));
		memcpy(tmp.f+0xa8/4,rot,sizeof(rot)); return tmp;
	}
};
static char SOM_MAP_ambient[2] = {-32,32}; //!!
static std::vector<SOM_MAP_trans> SOM_MAP_tileview2;
static bool SOM_MAP_draw_msm(BYTE *m, SOM_MAP_this::texture_t *tp)
{
	auto d = DDRAW::Direct3DDevice7;

	int i = *(int*)(m+20); //textures

	//2023: I think it's empty?? the pointer is 0 and so
	if(!i||!*(int*)(m+8)) return false; 

	char *pp = (char*)(*(int*)(m+8)+2); //textures names
	for(;i-->0;pp++) 
	while(*pp) pp++; //scanning past texture names

	int verts = *(WORD*)pp; pp+=2;

	//MSM memory is unaligned
	// 	 
	// can shift it over in memory?
	//	
	int todolist[SOMEX_VNUMBER<=0x1020602UL];
	//assert((size_t)pp%4==0);
	//
	void *pverts = pp; pp+=verts*32; //polygon data
		
	extern std::vector<WCHAR> som_tool_wector; //OPTIMIZING
	auto &w = som_tool_wector; w.clear();
	auto drawprims = [&]()
	{
		if(!w.empty())
		{
			d->DrawIndexedPrimitive(DX::D3DPT_TRIANGLELIST,
			0x112,pverts,verts,(WORD*)w.data(),w.size(),1);
			w.clear();
		}
	};

	namespace msm = SWORDOFMOONLIGHT::msm;
	auto *p = (msm::polygon_t*)pp;
	int texture = -2;
	SOM::MT *material = 0;
	bool transparent = false;

	//NOTE: this is SOM_MAP's algorithm. I'd never
	//seen MSM processing code before work on this
	for(i=p->subdivs;i-->0;)
	{
		if(!tp) goto wireframe;

		//NOTE: it really looks like SOM_MAP
		//calls SetTexture for every polygon
		if(texture!=p->texture)
		{
			//HACK: need to resolve transparency
			//texture = p->texture;

			drawprims(); //OPTIMIZING

			SOM::MT *mt = 0;
			auto t = (DWORD)p->texture; //texture
			void *s; if(t<16)
			{
				t = *(DWORD*)(m+24+t*4);
				//assert(t<(texos==438?256u:16u));
				assert(t<256);

				if(SOM::material_textures)
				{
					DWORD t2 = DDRAW::window==som_map_tileviewport?t:t+1024-16;
					assert(t2<1024);
					mt = t2<1024?(*SOM::material_textures)[t2]:0;
				}

				s = tp[t].d3dt2;
			}
			else{ s = 0; mt = 0; }
				
			bool be = mt&&0!=(mt->mode&0x102);

			if(som_map_tileviewmask&0x800?be:!be)
			{
				if(be) transparent = true; //continue;

				//DUPLICATE
				auto cmp = p->texture;
				do //TRICKY
				for(int j=(p=msm::shiftpolygon(p))->subdivs;j-->0;)
				for(int k=(p=msm::shiftpolygon(p))->subdivs;k-->0;)
				{
					p = msm::shiftpolygon(p);
				}
				while(i>0&&cmp==p->texture&&i--); continue;
			}

			texture = p->texture;

			if(material!=mt)
			{
				material = mt;

				if(!mt)
				{
					DX::D3DMATERIAL7 mat7 = {{1,1,1,1},{1,1,1,1}};
					d->SetMaterial(&mat7);
					//d->SetRenderState(DX::D3DRENDERSTATE_ALPHABLENDENABLE,0);
				}
				else
				{
					DX::D3DMATERIAL7 mat7 = {};

					memcpy(&mat7.diffuse,mt->data,4*sizeof(float));
					memcpy(&mat7.ambient,mt->data,4*sizeof(float));
					memcpy(&mat7.emissive,mt->data+4,3*sizeof(float));

					d->SetMaterial(&mat7);
					
					//d->SetRenderState(DX::D3DRENDERSTATE_ALPHABLENDENABLE,be!=0);
					if(be) d->SetRenderState(DX::D3DRENDERSTATE_SRCBLEND,DX::D3DBLEND_SRCALPHA);
					if(be) d->SetRenderState(DX::D3DRENDERSTATE_DESTBLEND,mt->mode&0x100?DX::D3DBLEND_ONE:DX::D3DBLEND_INVSRCALPHA);
				}
			}

			d->SetTexture(0,(DX::IDirectDrawSurface7*)s);
		}

		wireframe:

		if(0&&EX::debug) //2022: DEBUGGING (x2mdl/x2msm)
		{
			//RESULTS: it looks like if a triangle is larger than 2m then lv1 is
			//cut on a 2m grid, otherwise lv1 is cut on a 1m grid. if lv1 is cut
			//on a 2m grid, then lv2 is cut on a 1m grid

			enum{ lv=1 }; if(lv==0)
			{
				auto *pi = p->indices;		
				for(int n=p->corners-1,j=1;j<n;j++)
				{
					w.push_back(pi[0]); w.push_back(pi[j]); w.push_back(pi[j+1]);
				}
			}
			for(int j=(p=msm::shiftpolygon(p))->subdivs;j-->0;)
			{
				if(lv==1)
				{
					auto *pi = p->indices;		
					for(int n=p->corners-1,j=1;j<n;j++)
					{
						w.push_back(pi[0]); w.push_back(pi[j]); w.push_back(pi[j+1]);
					}
				}

				for(int k=(p=msm::shiftpolygon(p))->subdivs;k-->0;)
				{
					if(lv==2)
					{
						auto *pi = p->indices;		
						for(int n=p->corners-1,j=1;j<n;j++)
						{
							w.push_back(pi[0]); w.push_back(pi[j]); w.push_back(pi[j+1]);
						}
					}

					p = msm::shiftpolygon(p);
				}
			}
		}
		else if(0&&EX::debug||!tp&&som_map_tileviewmask&0x80) //2022: subdivide?
		{
			auto *q = msm::shiftpolygon(p);

			if(int i=q->subdivs) for(p=q;i-->0;p=q)
			{
				q = msm::shiftpolygon(p);

				if(int k=q->subdivs) for(p=q;k-->0;q=p=msm::shiftpolygon(p))
				{
					auto *pi = p->indices;		
					for(int n=p->corners-1,j=1;j<n;j++)
					{
						w.push_back(pi[0]); w.push_back(pi[j]); w.push_back(pi[j+1]);
					}
				}
				else
				{
					auto *pi = p->indices;		
					for(int n=p->corners-1,j=1;j<n;j++)
					{
						w.push_back(pi[0]); w.push_back(pi[j]); w.push_back(pi[j+1]);
					}
				}
			}
			else goto top_only;
		}
		else top_only: //lv==0
		{
			//NOTE: 4421cc accesses corners as "char" and
			//promotes (sign extended) to unsigned
			//TODO: might be a good idea to buffer these?
			//d->DrawIndexedPrimitive(DX::D3DPT_TRIANGLEFAN,0x112,pverts,verts,p->indices,p->corners,1);
			auto *pi = p->indices;		
			for(int n=p->corners-1,j=1;j<n;j++)
			{
				w.push_back(pi[0]); w.push_back(pi[j]); w.push_back(pi[j+1]);
			}

			//DUPLICATE
			//I'm not sure this makes sense but it's what
			//the code here seems to be doing to skip the
			//subdivisions
			for(int j=(p=msm::shiftpolygon(p))->subdivs;j-->0;)
			for(int k=(p=msm::shiftpolygon(p))->subdivs;k-->0;)
			{
				p = msm::shiftpolygon(p);
			}
		}
	}
	drawprims(); //OPTIMIZING

	//if(blendenable) d->SetRenderState(DX::D3DRENDERSTATE_ALPHABLENDENABLE,0);

	return transparent;
}
static bool SOM_MAP_draw_mdo(BYTE *m, SOM_MAP_this::texture_t *tp)
{
	auto d = DDRAW::Direct3DDevice7;

	char *top = *(char**)(m+8);

	int i = *(int*)(m+20); //textures
	char *pp = top+4; //textures names
	for(;i-->0;pp++) 
	while(*pp) pp++; //scanning past texture names

	while((size_t)pp%4) pp++; //MDO is 32-bit aligned

	unsigned mats = *(int*)pp; pp+=4;

	auto pmats = (float*)pp; pp+=4*(8*mats+12);

	int chans = *(int*)pp; pp+=4;

	namespace mdo = SWORDOFMOONLIGHT::mdo;
	auto p = (mdo::channel_t*)pp; pp+=chans*sizeof(*p);

	int texture = -2;
	int material = -2;
	int blendenable = 0;
	bool transparent = false;

	DX::D3DMATERIAL7 mat7 = {};

	for(i=chans;i-->0;p++)
	{
		if(!tp) goto wireframe;
		
		float *diffuse = pmats+8*p->matnumber;
		int be = p->blendmode?1:diffuse[3]!=1?2:0;
		if(som_map_tileviewmask&0x800?be:!be)
		{
			if(be) transparent = true; continue;
		}

		if(texture!=p->texnumber)
		{
			texture = p->texnumber;

			auto t = (DWORD)texture;
			void *s; if(t<16)
			{
				t = *(DWORD*)(m+24+t*4);
				assert(t<256);
				s = tp[t].d3dt2;
			}
			else s = 0;

			d->SetTexture(0,(DX::IDirectDrawSurface7*)s);
		}

		if(material!=p->matnumber)
		{
			material = p->matnumber;
		
			if((unsigned)material<mats)
			{
				memcpy(&mat7.diffuse,diffuse,16);
				memcpy(&mat7.ambient,diffuse,16);
				memcpy(&mat7.emissive,diffuse+4,12);
			}
			else for(int i=4;i-->0;)
			{
				(&mat7.diffuse.r)[i] = 1; 
				(&mat7.ambient.r)[i] = 1; 
				(&mat7.emissive.r)[i] = 0;
			}
			d->SetMaterial(&mat7);
		}

		if(blendenable!=be)
		{
			blendenable = be;

			//d->SetRenderState(DX::D3DRENDERSTATE_ALPHABLENDENABLE,be!=0);
			if(be) d->SetRenderState(DX::D3DRENDERSTATE_SRCBLEND,DX::D3DBLEND_SRCALPHA);
			if(be) d->SetRenderState(DX::D3DRENDERSTATE_DESTBLEND,be==1?DX::D3DBLEND_ONE:DX::D3DBLEND_INVSRCALPHA);
		}

		wireframe:

		int verts = p->vertcount;
		auto pverts = top+p->vertblock;
		int indices = p->ndexcount;
		auto pindices = top+p->ndexblock;
		d->DrawIndexedPrimitive(DX::D3DPT_TRIANGLELIST,0x112,pverts,verts,(WORD*)pindices,indices,1);
	}
	//if(blendenable) d->SetRenderState(DX::D3DRENDERSTATE_ALPHABLENDENABLE,0);

	return transparent;
}
static bool SOM_MAP_draw_mdl(BYTE *m, SOM_MAP_this::texture_t *tp)
{
	auto d = DDRAW::Direct3DDevice7;

	char *pp = *(char**)(m+12); //???

	//namespace mdl = SWORDOFMOONLIGHT::mdl;
	//auto p = *(mdl::primitivechannel**)(pp+40);
	int *p = (int*)(pp+40); //8B?

	int texture = -2;
	int material = -2;
	int blendenable = -1;
	bool transparent = false;

	DX::D3DMATERIAL7 mat7 = {{1,1,1,1},{1,1,1,1}};

	for(int n=*(short*)(pp+36);n-->0;p+=8)
	{
		int fvf,verts = *(WORD*)(p+3);

		float *pverts; if(pverts=(float*)p[4]) //D3DFVF_VERTEX
		{
			assert(!p[5]); //seems one or the other

			fvf = 0x112;
		}
		else if(pverts=(float*)p[5]) //D3DFVF_LVERTEX
		{
			fvf = 0x1e2; 
			
			assert(0); //materials take a different path?
		}
		else{ assert(0); continue; } //!

		if(!tp) goto wireframe;

		//PlayStation ABR blend rops
		//-1: as if ABE flag is zero
		//00:  50%back +  50%polygon (0)
		//01: 100%back + 100%polygon (1)
		//10: 100%back - 100%polygon (2)
		//11: 100%back +  25%polygon (3)
		int cmp = p[1];		
		bool be = cmp!=-1;
		if(som_map_tileviewmask&0x800?be:!be)
		{
			if(be) transparent = true; continue;
		}		
		
		if(texture!=p[2])
		{
			texture = p[2];

			auto t = (DWORD)texture;
			void *s; if(t<16)
			{
				t = *(WORD*)(*(int*)(pp+0x83c)+32+t*36); //???
				t = *(DWORD*)(m+24+t*4);
				assert(t<256);
				s = tp[t].d3dt2;
			}
			else s = 0;

			d->SetTexture(0,(DX::IDirectDrawSurface7*)s);
		}

		if(blendenable!=cmp)
		{
			blendenable = cmp;

			//about mode 2, I guess that INVSRCAPHA means source color*alpha
			//in which case INVSRCCOLOR would be equivalent in this case, as
			//alpha is 100%. INVSRCAPHA is the same value as SOM_MAP prefers

			//d->SetRenderState(DX::D3DRENDERSTATE_ALPHABLENDENABLE,be);
			if(be) d->SetRenderState(DX::D3DRENDERSTATE_SRCBLEND,cmp==2?DX::D3DBLEND_ZERO:DX::D3DBLEND_SRCALPHA);
			if(be) d->SetRenderState(DX::D3DRENDERSTATE_DESTBLEND,cmp?DX::D3DBLEND_ONE:DX::D3DBLEND_INVSRCALPHA);

			//TODO? there's also a fair amount of SetTextureStageState stuff
			//i.e. COLOROP/ALPHAOP that I can't tell exactly why the changes
			//are required on the basis of the blend modes. some seem to not
			//use the color values, but I'm not sure why white wouldn't work
			//out the same way. they use SELECTARG1 to choose just the color 
			//or just the texture
		}		
		
		switch(cmp)
		{
		case -1: case 1: case 2: cmp = 100; break;
		case 0: cmp = 5050; break; case 3: cmp = 25; break;			
		}
		if(material!=cmp)
		{
			material = cmp;

			switch(cmp)
			{
			case 100: mat7.diffuse.a = mat7.ambient.a = 1; break;
			case 5050: mat7.diffuse.a = mat7.ambient.a = 0.5f; break;
			case 25: mat7.diffuse.a = mat7.ambient.a = 0.25f; break;
			}
			d->SetMaterial(&mat7);
		}

		wireframe:

		int tricount = *(WORD*)(p+6);

		auto indices = (WORD*)p[7];

		d->DrawIndexedPrimitive(DX::D3DPT_TRIANGLELIST,fvf,pverts,verts,indices,3*tricount,1);
	}
	//if(blendenable!=-1) d->SetRenderState(DX::D3DRENDERSTATE_ALPHABLENDENABLE,0);

	return transparent;
}
extern "C" //#include <d3dx9math.h>
{
	struct D3DXMATRIX* WINAPI D3DXMatrixRotationY(struct D3DXMATRIX*,FLOAT);
}
static void SOM_MAP_msm_view(BYTE *msm, float(&io)[4][4])
{
	if(!SOM_MAP.palette) return; //init?
	int *vp = (int*)SOM_MAP_app::CWnd(SOM_MAP.palette);
	int w = vp[0x54/4];
	int x = vp[0x48/4];
	int y = vp[0x4c/4];
	if(int view=SOM_MAP_4921ac.parts[SOM_MAP_my_plt[y*w+x]].icon_bmp[31]>>2&7)
	{
		float m[4][4];
		D3DXMatrixRotationY((struct D3DXMATRIX*)m,view*M_PI/4);
		Somvector::map(io).postmultiply<4,4>(m);
	}

	int i = *(int*)(msm+20); //textures

	//2023: I think it's empty?? the pointer is 0 and so
	//there's no way to get the vertex count
	if(!i||!*(int*)(msm+8)) return; 
	
	char *pp = (char*)(*(int*)(msm+8)+2); //textures names
	for(;i-->0;pp++) 
	while(*pp) pp++; //scanning past texture names

	i = *(WORD*)pp; pp+=2;

	auto p = (SWORDOFMOONLIGHT::msm::vertex_t*)pp;

	float ym[2] = {1000,-1000};
	for(;i-->0;p++)
	{
		ym[0] = min(ym[0],p->pos[1]);
		ym[1] = max(ym[1],p->pos[1]);
	}

	float d = ym[1]-ym[0], s = 3/d;

	if(s<1) 
	{
		s+=(1-s)*0.2f;
	
		for(i=12;i-->0;) io[0][i]*=s;

		io[3][1]+=(1.5f-(ym[0]*s+d*s/2));
	}
	else if(ym[0]<0)
	{
		io[3][1]-=ym[0];
	}
}
void SOM_MAP_this::draw_model(BYTE *m)
{
	int m44os,texos; 
	if(SOM_MAP.model==DDRAW::window)
	{
		m44os = 0x8b8; texos = 0x22db8;
	}
	else //som_map_tileviewport
	{
		m44os = 0xaa124; texos = 0x41C;
	}

	auto tp = (BYTE*)this;
	auto t = (texture_t*)(tp+texos);
	auto d = DDRAW::Direct3DDevice7;

	float tmp[4][4];
	memcpy(tmp,m+0xa8,12*sizeof(float)); 
	memcpy(tmp[3],m+0xf0,3*sizeof(float)); tmp[3][3] = 1;
	Somvector::map(tmp).premultiply<4,4>(*(float(*)[4][4])(tp+m44os));
	if(SOM_MAP.model==DDRAW::window) SOM_MAP_msm_view(m,tmp);
	d->SetTransform(DX::D3DTRANSFORMSTATE_WORLD,(DX::LPD3DMATRIX)tmp);

	int i,mhm,wireframe; switch(*m)
	{
	case 1: mhm = 0!=(som_map_tileviewmask&0x100);
		//2022: I'm removing "wireframe_only" mode
		//see supersampling comment/rationale below
		if(wireframe=0!=(som_map_tileviewmask&0x40))
		{
			if(m44os!=0x8b8) mhm = false; break;
		}
		if(wireframe=mhm) break;	
		if(m44os==0x8b8) break;
	default: mhm = false;
		wireframe = 0!=(som_map_tileviewmask&0x40); 
		break;
	case 0: wireframe = false;
	}

	DX::D3DMATERIAL7 mat7; if(wireframe)
	{
		/*2022: with supersampling textured wireframe
		//over black negative space isn't much to look
		//at, so I'm changing this button to render the
		//MSM geometry with subdivisions to aid analysis
		if(~som_map_tileviewmask&0x80&&!mhm)
		{
			i = 0; goto wireframe_only;
		}
		else*/ t = 0; //no materials

		memset(&mat7,0x00,sizeof(mat7));

		if(DDRAW::TextureSurface7) d->SetTexture(0,0);

		if(mhm) //DUPLICATE
		{
			//som_hacks_cliptexture //0xff2787a7
			DWORD c = EX::INI::Editor()->clip_model_pixel_value;			
			float a = SOM_MAP_ambient[m44os==0xaa124];
			if(m44os==0xaa124&SOM_MAP_4921ac.lighting)
			{
				//HACK: trying to darken lit scene
				//but not colorize
				auto &l = SOM_MAP_4921ac.lights[0];
				if(l.lit[0]&&l.lit[1])
				{
					float bw = 0;
					bw+=l.color[2]*0.222f;
					bw+=l.color[1]*0.707f;
					bw+=l.color[0]*0.071f;
					//0.6 is a fudge because diffuse 
					//lights aren't incorporated ATM
					a-=(255-bw)*0.666f; 
				}
			}
			a*=0.003921568627f; for(int i=3;i-->0;)
			{
				float &e = (&mat7.emissive.r)[i]; //ambient				
				e = ((BYTE*)&c)[2-i]*0.003921568627f; e+=e*a;
			}
		}
		else
		{
			mat7.emissive.r =
			mat7.emissive.g =
			mat7.emissive.b = 1; //ambient
		}
		d->SetMaterial(&mat7);
	}

	bool transparent = false;

	for(i=wireframe;i-->=0;) 
	{
		switch(*m)
		{
		case 0: return; //draw_lines?
		case 1: transparent = SOM_MAP_draw_msm(m,t); break; //40ac40
		case 2: transparent = SOM_MAP_draw_mdo(m,t); break; //40AE80
		case 3: transparent = SOM_MAP_draw_mdl(m,t); break; //40B190
		default: assert(0); //40b9b0? (40ac15)
		}

		if(!i) //wireframe
		{
			mat7.emissive.r = //ambient
			mat7.emissive.g = //DUPLICATE
			mat7.emissive.b = mhm?1:7/255.0f; //som_hacks_edgetexture 
			d->SetMaterial(&mat7);
		//	wireframe_only:
			d->SetRenderState(DX::D3DRENDERSTATE_ZBIAS,0);
			d->SetRenderState(DX::D3DRENDERSTATE_FILLMODE,DX::D3DFILL_WIREFRAME);
		}
	}
	if(wireframe)
	{
		d->SetRenderState(DX::D3DRENDERSTATE_ZBIAS,1);
		d->SetRenderState(DX::D3DRENDERSTATE_FILLMODE,DX::D3DFILL_SOLID);
	}

	assert(~m[256]&2); //bounding box? //40a870

	if(transparent)
	{
		float *p = tmp[3], d = p[0]*p[0]+p[1]*p[1]+p[2]*p[2];

		SOM_MAP_tileview2.push_back((SOM_MAP_model*)m);
		SOM_MAP_tileview2.back().dist = d;
	}
}
void SOM_MAP_this::draw_lines(BYTE *m)
{
	BYTE swap = *m; 
	{
		*m = 0; draw_model(m); //setup the transform
	}
	*m = swap;

	auto d = DDRAW::Direct3DDevice7;

	auto pverts = (DWORD*)m+0x104/4; //this isn't wireframes??

	//YUCK: alpha? som.shader.cpp seems to be fine without alpha
	//but white seems to be better for this, especially when the
	//lines overalap the gridlines
	if(~pverts[4]>>24) for(int i=4;i<8*8;i+=8)
	{
		pverts[i]|=1?~0:0xff000000; //yellow?		
	}

	d->SetTexture(0,0);
	d->DrawIndexedPrimitive(DX::D3DPT_LINELIST,0x1e2,pverts,8,(WORD*)this+0x87660/2,24,1);
}
static DX::IDirect3DDevice7*
SOM_MAP_begin_scene(HWND hw, float(&tmp)[4][4])
{
	DDRAW::window = hw;

	if(DDRAW::inScene) //???
	{
		//this seems to be happenening when adding an 
		//NPC to a tile???
		//outer call: 0041D5C7 E8 79 B3 62 7B       call        SOM_MAP_this::map_44c800
		//inner call: 0044C7CF E8 71 C1 5F 7B       call        SOM_MAP_this::map_44c800
		//InvalidateRect doesn't seem to post
		//InvalidateRect(hw,0,0);
		assert(hw==som_map_tileviewport);
		//doesn't help anything?
		//if(hw) RedrawWindow(hw,0,0,RDW_INTERNALPAINT);
		//if(hw) RedrawWindow(hw,0,0,RDW_UPDATENOW|RDW_INVALIDATE|RDW_NOERASE);
		return 0;
	}

	auto d = DDRAW::Direct3DDevice7;

	if(!d||!hw){ assert(0); return 0; }

	d->BeginScene();

	DX::D3DVIEWPORT7 vp; 
	GetClientRect(hw,(RECT*)&vp);
	vp.dvMinZ = 0;
	vp.dvMaxZ = 1.0f;
	d->SetViewport(&vp);
	d->Clear(0,0,3,0,1.0f,0); 
		
	//HACK: need to do this in order to share the D3D
	//device with the tile viewer, because dx.ddraw.h
	//only has globals for one device instance
	{
		//TODO: IDirect3DMaterial3::SetMaterial?	
		//FOR SOME REASON som.shader.cpp HAS matAmbient???
		/*FAKE: using D3DRENDERSTATE_AMBIENT for now...
		const float l = 0.66666f; //1
		DX::D3DMATERIAL7 white = {{l,l,l,1},{l,l,l,1}};*/
		DX::D3DMATERIAL7 white = {{1,1,1,1},{1,1,1,1}};
		d->SetMaterial(&white);
		
		Somvector::map(tmp).identity<3,4>();
	//	Somvector::series(tmp[3],0,-1.5f,5.5f,1); //441B48
		d->SetTransform(DX::D3DTRANSFORMSTATE_VIEW,(DX::LPD3DMATRIX)tmp);
	}

	//D3DRENDERSTATE_AMBIENT?
	{
		int i = hw==som_map_tileviewport;
		int a = SOM_MAP_ambient[i];
		union{ DWORD rs; BYTE rgb[3]; };
		auto &l = SOM_MAP_4921ac.ambient;
		bool sel = i&&SOM_MAP_4921ac.lighting;
		rs = sel?l.lit[0]&&l.lit[1]?l.color:0:0xd0d0d0;	
		for(int j=3;j-->0;)
		rgb[j] = max(0,min(255,a+rgb[j]));
		d->SetRenderState(DX::D3DRENDERSTATE_AMBIENT,rs|255<<24);	
	}

	return d;
}
static void SOM_MAP_end_scene()
{
	DDRAW::Direct3DDevice7->EndScene();
	DDRAW::PrimarySurface7->Flip(0,DDFLIP_WAIT);
}
void __thiscall SOM_MAP_this::map_441ef0() //2021
{
	//following som_hacks_DrawIndexedPrimitive3's lead 
	//som_map_refresh_model_view?
	if(0x200&som_map_tileviewmask) return;

	float tmp[4][4];
	Somvector::series(tmp[3],0,-1.5f,5.5f,1); //441B48

	if(auto*d=SOM_MAP_begin_scene(SOM_MAP.model,tmp))
	{
		d->LightEnable(3,1); //2022
		d->LightEnable(4,1); //2023

		auto m = (BYTE*)this+0x22bb4;

		som_map_tileviewmask|=0x800; //transparent?

		draw_model(m); 

		som_map_tileviewmask&=~0x800; //transparent?

		draw_transparent_elements();
		
		d->LightEnable(3,0); //2022
		d->LightEnable(4,0); //2023

		SOM_MAP_end_scene();
	}
}
void SOM_MAP_this::init_lights()
{
	auto d = DDRAW::Direct3DDevice7;

	/*MOVED: SOM_MAP_begin_scene
	auto &a = SOM_MAP_4921ac.ambient;
	DWORD c = a.color;
	if(!a.lit[0]||!a.lit[1]) c = 0; //??? //422029
	d->SetRenderState(DX::D3DRENDERSTATE_AMBIENT,c);*/

	DX::D3DLIGHT7 lit7 = {DX::D3DLIGHT_DIRECTIONAL};
	for(int i=3;i-->0;) 
	{
		auto &l = SOM_MAP_4921ac.lights[i];

		if(l.lit[0]&&l.lit[1]) //??? //42219f
		{
			lit7.dcvAmbient.r =
			lit7.dcvDiffuse.r = l.color[2]/255.0f;
			lit7.dcvAmbient.g =
			lit7.dcvDiffuse.g = l.color[1]/255.0f;
			lit7.dcvAmbient.b =
			lit7.dcvDiffuse.b = l.color[0]/255.0f;

			auto &world = *(float(*)[4][4])((BYTE*)this+0xaa124);

			float u = l.ray[0]*SOMVECTOR_PI/180;
			float v = l.ray[1]*SOMVECTOR_PI/180;
			float quat[4], uv[3] = {u,v,0};
			Somvector::map(quat).quaternion(uv);
			uv[0] = uv[1] = 0; uv[2] = 1;
			Somvector::map(uv).rotate<3>(quat).premultiply<3>(world);
			memcpy(&lit7.dvDirection,uv,sizeof(uv));

			d->SetLight(i,&lit7); d->LightEnable(i,1);
		}
	}
}
DWORD __thiscall SOM_MAP_this::map_44c800_3c() //2021
{
	HWND hw = som_map_tileviewport;

	float tmp[4][4];
	/*dropping down reduces gridlines bunching
	//and seems to be the actual center. 10.5
	//keeps the tips from being clipped 
	Somvector::series(tmp[3],0,-1,10,1); //406782*/
	Somvector::series(tmp[3],0,-1.5f,10.5f,1);

	auto d = SOM_MAP_begin_scene(hw,tmp);
	if(!d) return 0;

	bool lit = SOM_MAP_4921ac.lighting;
	if(lit) init_lights();

	if(~som_map_tileviewmask|0x80)
	{
		som_map_tileviewmask|=0x800; //transparent?
	}

	//REMINDER: when using Ex_output_lines it's
	//not possible to apply a negative bias to 
	//the lines because lateral lines don't seem
	//to have a slope, but also bias doesn't work
	//on D3DPT_LINELIST
	//if(som_map_tileviewmask&0x1c8)
	//d->SetRenderState(DX::D3DRENDERSTATE_ZBIAS,1);
	tileview(); //tiles
	tileview(); //items
	tileview(); //objects
	tileview(); //enemies
	tileview(); //NPCs
	tileview(); //0x441c?
	//if(som_map_tileviewmask&0x1c8)
	d->SetRenderState(DX::D3DRENDERSTATE_ZBIAS,0); //EXTENSION

	som_map_tileviewmask&=~0x800; //transparent?

	if(lit) d->SetRenderState(DX::D3DRENDERSTATE_LIGHTING,0); 

	/*UNFINISHED (bounding boxes?)
	if(som_map_tileviewmask&0x100)
	{
		auto m = (BYTE*)this+0x85620;			
		for(int i=0;i<16;i++,m+=0x204)
		{
			//40A870 is bounding boxes
			if(*m) draw_lines(m); //40A870
		}
	}*/
	//if(*(int*)(tp+0xa98c4)) //40a7a3 //unused?
	if(som_map_tileviewmask&8) draw_guidelines(); //40a990

	if(lit) d->SetRenderState(DX::D3DRENDERSTATE_LIGHTING,1); 

	d->SetRenderState(DX::D3DRENDERSTATE_ZBIAS,1);

	draw_transparent_elements();
	
	if(lit) for(int i=3;i-->0;) d->LightEnable(i,0);
	if(lit) d->SetRenderState(DX::D3DRENDERSTATE_AMBIENT,~0);

	SOM_MAP_end_scene(); return 1; //unused?
}
void SOM_MAP_this::draw_transparent_elements()
{
	auto &tv2 = SOM_MAP_tileview2; if(tv2.empty()) return;

	auto d = DDRAW::Direct3DDevice7;

	d->SetRenderState(DX::D3DRENDERSTATE_ZWRITEENABLE,0);
	d->SetRenderState(DX::D3DRENDERSTATE_ALPHABLENDENABLE,1);

	auto f = [](const SOM_MAP_trans &a, const SOM_MAP_trans &b)->bool
	{
		return a.dist<b.dist;
	};
	std::sort(tv2.begin(),tv2.end(),f);

	for(size_t i=tv2.size();i-->0;)
	{
		SOM_MAP_model tmp = tv2[i]; draw_model((BYTE*)&tmp);
	}
	tv2.clear();

	d->SetRenderState(DX::D3DRENDERSTATE_ZWRITEENABLE,1);
	d->SetRenderState(DX::D3DRENDERSTATE_ALPHABLENDENABLE,0);
}	
void SOM_MAP_this::draw_guidelines()
{
	auto tp = (BYTE*)this;
	auto vb = (float*)(tp+0xa98c8);
	auto ib = (WORD*)(tp+0xa9c68);
	auto d = DDRAW::Direct3DDevice7;

	assert(*(int*)(tp+0xa98c4)); //40a7a3 //unused?

	d->SetTexture(0,0); //DrawIndexedPrimitive?
	d->SetTransform(DX::D3DTRANSFORMSTATE_WORLD,(DX::LPD3DMATRIX)(tp+0xaa124));

	if(0==(DWORD&)vb[4]>>24) //YUCK
	for(int i=4;i<29*8;i+=8) (DWORD&)vb[i]|=0xFF000000;

	//REMINDER: I tried every trick in the book to get the RGB lines
	//to not be overpowered by the white lines, nothing much matters 
	if(!Ex_output_lines((float*)vb,8,34,ib))
	d->DrawIndexedPrimitive(DX::D3DPT_LINELIST,0x1e2,vb,29,ib,34,1);
}

static int SOM_MAP_413a30_pred(const void *a, const void *b)
{
	//return (int)a-(int)((Part*)b)->part_number;
	return (int)a-(int)*(WORD*)b;
}
SOM_MAP_this::Part*
SOM_MAP_this::bsearch_for_413a30(Part *o, WORD part_number)
{
	//this is easier to use (see map_41baa0)
	//auto tp = (struct SOM_MAP_4921ac*)this;
	auto tp = (struct SOM_MAP_4921ac*)0x4921ac;

		//I don't know if this will always hold, say if
		//a tile is called 123.prt instead of 0123.prt?
		#ifdef _DEBUG
		static Part *pp = 0; if(!pp)
		{
			auto p = pp = tp->parts;
			for(int i=tp->partsN-1;i-->0;)
			assert(p[i].part_number<p[i+1].part_number);
		}
		#endif

	Part *p = tp->find_part_number(part_number);
	assert(p);
	return !o?p:(Part*)(p?memcpy(o,p,sizeof(*o)):memset(o,0x00,sizeof(*o)));
}
SOM_MAP_4921ac::Part *SOM_MAP_4921ac::find_part_number(WORD n)
{
	return (Part*)::bsearch((void*)n,parts,partsN,sizeof(Part),SOM_MAP_413a30_pred);
}

static void SOM_MAP_save_my_plt()
{
	int n = SOM_MAP_4921ac.partsN;

	auto *p = SOM_MAP_my_plt.data(); if(!p) return;

	const wchar_t *w = SOM::Tool.project(SOMEX_(B)"\\data\\map\\my.plt");

	if(FILE*f=_wfopen(w,L"wb"))
	{
		for(int i=0;i<n;i++)
		{
			int no = p[i];

			const auto &k = SOM_MAP.prt[no];

			int rot = SOM_MAP_4921ac.parts[no].rot;

			fprintf(f,"%d:%d:%ls:%ls\n",rot,no,k.longname(),k.iconID);
		}
		fclose(f);
	}
}
static VOID CALLBACK SOM_MAP_save_my_plt(HWND, UINT, UINT_PTR id, DWORD)
{
	if(~GetAsyncKeyState(VK_LBUTTON)>>15) KillTimer(0,id); else return;

	SOM_MAP_save_my_plt();
}
static void SOM_MAP_load_my_plt()
{
	int i = 0, n = SOM_MAP_4921ac.partsN;
 
	wchar_t w[MAX_PATH]; EX::data("map/my.plt",w);

	std::vector<WORD> v;
	std::vector<char> c(n);

	bool miss = false;

	if(*w) if(FILE*f=_wfopen(w,L"rb"))
	{
		char ln[69], pro[31], ico[31]; int rot,no;
		while(fgets(ln,sizeof(ln),f))		
		if(4==_snscanf_s(ln,sizeof(ln),"%d:%d:%[^:]:%[^\n],",&rot,&no,pro,30,ico,30))
		{
			EX::need_unicode_equivalent(65001,pro,w,MAX_PATH);

			if(no<n&&!wcscmp(w,SOM_MAP.prt[no].profile))
			{
				if(!c[no]) v.push_back((WORD)no); 
					
				c[no] = 1; //nonzero
			}
			else 
			{
				miss = true;

				auto &k = SOM_MAP.prt[w]; if(!k)
				{
					const wchar_t *w = EX::need_unicode_equivalent(65001,ico);

					size_t i,iN = SOM_MAP.prt.keys();
					for(i=0;i<iN;i++) 
					{
						const wchar_t *icon = SOM_MAP.prt[i].iconID;
						if(*w==*icon&&!wcscmp(w,icon)) break; 
					}
					if(i==iN) continue; //missing?

					no = SOM_MAP.prt[i];
				}
				else no = k.number();

				if(!c[no]) v.push_back((WORD)no);

				c[no] = 1; //nonzero
			}

			SOM_MAP_4921ac.parts[no].rot = (BYTE)rot;
		}
		fclose(f);
	}

	SOM_MAP_my_plt.resize(n);
	WORD *p = SOM_MAP_my_plt.data();
	if(v.empty()) 
	{		
		miss = true;

		for(i=n;i-->0;) p[i] = (WORD)i;
	}	
	else
	{
		int sz = (int)v.size();

		for(i=0;i<n&&!c[i];i++)
		{
			*p++ = i;
		}
		for(i=0;i<sz;i++) 
		{
			int vi = v[i];

			*p++ = (WORD)vi++;

			while(vi<n&&!c[vi]) 
			{
				*p++ = (WORD)vi++;
			}
		}
		assert(p==SOM_MAP_my_plt.data()+n);
	}

	//note: this just optimizes for cache hits
	if(miss) SOM_MAP_save_my_plt();
}
void *__thiscall SOM_MAP_this::map_4139c0_palette(Part *o, WORD part)
{
	if(part>=SOM_MAP_4921ac.partsN)
	{
		memset(o,0x00,sizeof(o)); return o;
	}
	if(!SOM_MAP_my_plt.empty()) //EXTENSION
	{
		part = SOM_MAP_my_plt[part];
	}	
	return memcpy(o,SOM_MAP_4921ac.parts+part,sizeof(*o));
}
extern int SOM_MAP_413aa0_inverse(WORD part) //som.files.cpp
{
	int ret;
	auto it = std::find(SOM_MAP_my_plt.begin(),SOM_MAP_my_plt.end(),part);
	if(it!=SOM_MAP_my_plt.end())
	ret = it-SOM_MAP_my_plt.begin();
	else ret = -1; 
	return ret;
}
int __thiscall SOM_MAP_this::map_413aa0_palette(WORD part)
{
	auto p = SOM_MAP_4921ac.find_part_number(part);
	if(!p) return -1;
	part = p-SOM_MAP_4921ac.parts;
	return SOM_MAP_413aa0_inverse(part);
}
void __thiscall SOM_MAP_this::map_413b30_palette(int y, int x, SOM_MAP_4921ac::Tile *cp)
{
	cp->spin = SOM_MAP_4921ac.find_part_number(cp->part)->rot;

	cp->z = GetDlgItemFloat(som_tool,1000); //2024

	((void(__thiscall*)(void*,int,int,void*))0x413b30)(this,y,x,cp);
}
extern void SOM_MAP_spin_palette_up()
{
	int *vp = (int*)SOM_MAP_app::CWnd(SOM_MAP.palette);
	int w = vp[0x54/4];

	int a = SOM_MAP_drag_palette_sel.first;
	int b = SOM_MAP_drag_palette_sel.second;
	if(b!=65535)
	{
		if(a>b) std::swap(a,b);

		for(;a<=b;a++)
		{
			BYTE &spin = SOM_MAP_4921ac.parts[SOM_MAP_my_plt[a]].rot;
			spin = (spin+1)%4;
		}	
	}
	else //CONTROL?
	{	
		int x = vp[0x48/4];
		int y = vp[0x4c/4];
		BYTE &spin = SOM_MAP_4921ac.parts[SOM_MAP_my_plt[y*w+x]].rot;
		spin = (spin+1)%4;		
	}

	InvalidateRect(SOM_MAP.palette,0,0);

	SetTimer(0,'wplt',2000,SOM_MAP_save_my_plt);
}
extern void SOM_MAP_drag_palette_up()
{
	//NOTE: this is for changing selection
	char *cw = (char*)SOM_MAP_app::CWnd(SOM_MAP.painting); 

	auto &sel = SOM_MAP_drag_palette_sel;
	if(sel.first==sel.second&&sel.first!=65535)
	{
		WORD part = SOM_MAP_my_plt[sel.first];
		part = SOM_MAP_4921ac.parts[part].part_number;

		auto *p = SOM_MAP_4921ac.grid;
		
		for(int i=100*100;i-->0;)
		if(p[i].part!=65535)
		if(p[i].part==part)
		{			
			int y = 99-i/100, x = i%100;
			*(int*)(cw+0x88) = x;
			*(int*)(cw+0x8c) = y;
			*(int*)(cw+0x90) = x;
			*(int*)(cw+0x94) = y;
			*(int*)(cw+0x60) = 1;			
			InvalidateRect(SOM_MAP.painting,0,0);
			return;
		}
	}

	if(*(int*)(cw+0x60))
	{
		*(int*)(cw+0x60) = 0;
		InvalidateRect(SOM_MAP.painting,0,0);
	}
}
extern void SOM_MAP_drag_palette(int pos)
{
	int *vp = (int*)SOM_MAP_app::CWnd(SOM_MAP.palette);
	int w = vp[0x54/4];
	WORD part = (WORD)(vp[0x4c/4]*w+vp[0x48/4]);
	auto &sel = SOM_MAP_drag_palette_sel;
	BOOL shift = GetKeyState(VK_SHIFT)>>15;
	int a = sel.first;
	int b = sel.second;
	if(a>b) std::swap(a,b);
	if(pos==-1)
	{
		if(b!=65535&&!shift
		&&a<=(int)part&&b>=(int)part			
		&&GetCapture()==SOM_MAP.palette)
		{
			return; //selection?
		}

		sel.first = part;
		sel.second = shift?part:65535;
		return;
	}
	if(shift)
	{
		sel.second = part; return;
	}
	if(b==65535) b = a;

	if(part>a) part-=min(part,b-a); //post erase

	std::vector<WORD> swap;
	auto &v = SOM_MAP_my_plt;	
	swap.assign(v.begin()+a,v.begin()+b+1);
	v.erase(v.begin()+a,v.begin()+b+1);
	v.insert(v.begin()+part,swap.begin(),swap.end());

	SetTimer(0,'wplt',2000,SOM_MAP_save_my_plt);

	sel.first = part;
	if(sel.second!=65535)
	sel.second = part+(b-a);
}

void __thiscall SOM_MAP_this::map_41baa0()
{
	((void(__thiscall*)(void*))0x41baa0)(this);

	auto tp = (BYTE*)this;
	
	//NOTE: Ghidra loses these details in its disassembly???
	//0041BD73 8B 4B 68             mov         ecx,dword ptr [ebx+68h]
	//0044D5FA 8D 5F 3C             lea         ebx,[edi+3Ch]
	auto tp2 = *(BYTE**)(tp+0x68), tp3 = tp2+0x3c;

	int x = *(int*)(tp+0x94);
	int y = *(int*)(tp+0x98);

	SOM_MAP_tileviewtile = MAKEWORD(x,y); //UNRELATED

	int xy = 100*y+x; 	
	
	float z = SOM_MAP_4921ac.grid[xy].z;

	//SOM_MAP_48_msm_mem?
	auto p = SOM_MAP.layers;
	int i; for(i=7;i-->0;) if(p[i]) break;
	if(i==-1) goto no_layers;

	/*//// layers ////////////////////////*/{
		
	if(!SOM_MAP_48_msm_mem)
	SOM_MAP_48_msm_mem = new SOM_MAP_model[48]();
	auto m = (SOM_MAP_model*)(tp3+0x85620);
	auto tile = [&](int xy)->bool
	{		
		auto t = *p+xy;

		if(t->part==0xFFFF) return false;

		//NOTE: should be casting to 4921ac->413a30
		//but this is original code so I don't know
		//auto &prt = SOM_MAP_4921ac.parts[t->part];
		Part prt; bsearch_for_413a30(&prt,t->part);

		//map_407c60 overwrites the path
		//memcpy(msm+cat,prt.msm.file,32);
		char msm[64];
		sprintf(msm,"%s%s",SOMEX_(A)"\\data\\map\\model\\",prt.msm.file,32);
//		if(!((int(__thiscall*)(void*,void*,char*,char*))0x407c60)
		if(!((SOM_MAP_this*)tp3)->map_407c60
//		(tp3,m+i,msm,SOMEX_(A)"\\data\\map\\texture\\"))
		(m+i,msm,SOMEX_(A)"\\data\\map\\model\\"))
		{
			assert(0); return false;
		}
		m[i].i[256/4] = 1; //make visible

		m[i].f[0xf0/4+1] = t->z-z; //elevation

		auto &r = *(float(*)[4][4])(m[i].f+0xa8/4); //rotation matrix
		switch(t->spin)
		{
		case 1: r[0][0] = r[2][2] = 0; r[0][2] = 1; r[2][0] = -1; break;
		case 2: r[0][0] = r[2][2] = -1; break;
		case 3: r[0][0] = r[2][2] = 0; r[0][2] = -1; r[2][0] = 1; break;
		}

		return true;
	};	
	for(i=9;i<=14;i++,p++) if(*p)
	{
		tile(xy);
	}
	p = SOM_MAP.layers;
	m = SOM_MAP_48_msm_mem;
	assert(!m->c[0]);
	for(int j=i=0;j<7;j++,p++) if(*p)
	{	
		//44c820 works in this order
		static const char d[8][2] =
		{
			{-1,-1}, //1: -2,-2
			{-1, 0}, //2: -2, 0
			{-1,+1}, //3: +2,-2
			{ 0,-1}, //4:  0,-2
			{ 0,+1}, //5:  0,+2
			{+1,-1}, //6: +2,-2
			{+1, 0}, //7: +2, 0
			{+1,+1}, //8: +2,+2
		};
		for(int k=0;k<8;k++)
		{
			int xx = x+d[k][0];
			int yy = y+d[k][1];
			if(xx>=0&&xx<=99&&yy>=0&&yy<=99)
			{
				if(tile(100*yy+xx))
				{
					m[i].f[0xf0/4+0]+=2*d[k][0];
					m[i].f[0xf0/4+2]+=2*d[k][1];
					i++;
				}
			}
		}
	}

	}/*//// content ///////////////////////*/{

		no_layers:

	auto neighbor = [x,y](BYTE *cmp)->long
	{
		int xx = cmp[0]-x, yy = cmp[1]-y;

		return abs(xx)<=1&&abs(yy)<=1?MAKELONG(xx,yy):0;
	};
	auto recenter = [x,y,z](long xy, SOM_MAP_model *p)
	{
		int xx = ((short*)&xy)[0], yy = ((short*)&xy)[1];

		float zz = SOM_MAP_4921ac.grid[100*(y+yy)+(x+xx)].z;

		p->f[0xf0/4+0]+=2*xx;
		p->f[0xf0/4+1]+=zz-z;
		p->f[0xf0/4+2]+=2*yy;
	};

	//44d2a0 enemies
	#ifdef SOM_TOOL_enemies2
	int es = SOM_MAP_4921ac.enemies_size;
	auto *ep = SOM_MAP_4921ac.enemies2.data();
	#else
	int es = 128;
	auto *ep = SOM_MAP_4921ac.enemies;
	#endif	
	for(i=0;i<es;i++,ep++) if(long xy=neighbor(ep->tile))
	{
		((int(__thiscall*)(void*,int,void*))0x44d2a0)(tp2,i,ep);
		#ifdef SOM_TOOL_enemies2
		recenter(xy,SOM_MAP_4921ac.enemies3.data()+i);
		#else
		recenter(xy,(SOM_MAP_model*)(tp3+0x14820)+i);
		#endif
	}
	//44d510 NPCs	
	auto *np = SOM_MAP_4921ac.NPCs;
	for(i=0;i<128;i++,np++) if(long xy=neighbor(np->tile))
	{
		((int(__thiscall*)(void*,int,void*))0x44d510)(tp2,i,np);
		recenter(xy,(SOM_MAP_model*)(tp3+0x4620)+i);
	}
	//44d780 objects
	auto *op = SOM_MAP_4921ac.objects;
	for(i=0;i<512;i++,op++) if(long xy=neighbor(op->tile))
	{
		((int(__thiscall*)(void*,int,void*))0x44d780)(tp2,i,op);
		recenter(xy,(SOM_MAP_model*)(tp3+0x24A20)+i);
	}
	//44da30 items
	auto *ip = SOM_MAP_4921ac.items;
	for(i=0;i<256;i++,ip++) if(long xy=neighbor(ip->tile))
	{
		((int(__thiscall*)(void*,int,void*))0x44da30)(tp2,i,ip);
		recenter(xy,(SOM_MAP_model*)(tp3+0x65220)+i);
	}
	
		}/*END*/
}

extern void SOM_MAP_scroll3D(WPARAM w, bool) //EXPERIMENTAL
{
	int i = GET_WHEEL_DELTA_WPARAM(w);
	i/=WHEEL_DELTA;

	char &a = SOM_MAP_ambient[som_tool_stack[1]!=0];
	a = w?max(-127,min(127,a+i)):0;

	HWND hw = som_tool_stack[1]?som_map_tileviewport:SOM_MAP.model;
	if(hw) InvalidateRect(hw,0,0); else assert(0);
}

int __thiscall SOM_MAP_this::map_409eb0(int npc, char *a)
{
	if(auto*skins=SOM_MAP_4921ac::npcskins)
	{
		int prf = SOM_MAP_4921ac.NPCs[npc].props;
		if(prf!=0xffff) 
		prf = SOM_MAP_4921ac.NPCprops[prf].profile;
		if(prf!=0xffff)
		if(skins[prf].skin)
		{
			som_MDL_skin = SomEx_utf_xbuffer;
			sprintf(som_MDL_skin,"npc\\texture\\%s",skins[prf].file);
		}
	}
	int ret = ((int(__thiscall*)(void*,int,char*))0x409eb0)(this,npc,a);
	som_MDL_skin = 0; return ret;
}
int __thiscall SOM_MAP_this::map_409e00(int enemy, char *a)
{
	if(auto*skins=SOM_MAP_4921ac::npcskins)
	{
		int prf = SOM_MAP_4921ac.enemies[enemy].props;
		if(prf!=0xffff) 
		prf = SOM_MAP_4921ac.enemyprops[prf].profile;
		if(prf!=0xffff)
		if(skins[1024+prf].skin)
		{
			som_MDL_skin = SomEx_utf_xbuffer;
			sprintf(som_MDL_skin,"enemy\\texture\\%s",skins[1024+prf].file);
		}
	}
	int ret = ((int(__thiscall*)(void*,int,char*))0x409e00)(this,enemy,a);
	som_MDL_skin = 0; return ret;
}
int __thiscall SOM_MAP_this::map_4084d0(SOM_MAP_model *m, char *a)
{
	wchar_t art[MAX_PATH]; 	
	int e = som_art_model(a,art); //2021
	using namespace x2mdl_h;
	if(e&_art&&~e&_lnk)	
	if(!som_art(art,0)) //x2mdl exit code?
	{
		e = som_art_model(a,art); //retry?
	}
	if(0==(e&(_mdo|_bp|_mdl))) return 0; //2021
	////////////////////////////
	/////POINT-OF-NO-RETURN/////
	////////////////////////////
	//HACK: this is used to load files from
	//nonstandard locations
	som_art_CreateFile_w = art;

	char *ext = PathFindExtensionA(a);

	if(e&(_mdo|_bp)) //BP??
	{
		memcpy(ext,e&_mdo?".mdo":".bp",5);
		int ret = ((int(__thiscall*)(void*,void*,char*))0x408010)(this,m,a);
		memcpy(ext,".mdl",5);

		if(ret) som_art_CreateFile_w = 0; //POINT-OF-NO-RETURN
		if(ret) return ret;
	}
	else
	{
		//REMINDER: art extension won't always
		//be MDL (I think)
		memcpy(ext,".mdl",5);
	}
	int ret = e&_mdl?((int(__thiscall*)(void*,void*,char*))0x4084d0)(this,m,a):0;

	som_art_CreateFile_w = 0; //POINT-OF-NO-RETURN

	return ret;
}
extern const wchar_t *x2mdl_dll; //HACK
int __thiscall SOM_MAP_this::map_442440_407c60(SOM_MAP_model *m, char *a, char *b, DWORD x)
{
	wchar_t art[MAX_PATH]; 	
	int e = som_art_model(a,art); //2022
	using namespace x2mdl_h;
	if(e&_art&&~e&_lnk)	
	{
		//HACK: communicate MSM/MHM is desired		
		auto swap = x2mdl_dll; 
		{
			x2mdl_dll = L"x2msm.dll"; //HACK

			if(!som_art(art,0)) //x2mdl exit code?
			{
				e = som_art_model(a,art); //retry?
			}
		}
		x2mdl_dll = swap;
	}
	bool mhm = 0x100&som_map_tileviewmask;
	auto cmp = mhm?".mhm":".msm";
	int _mxm = mhm?_mhm:_msm;
	if(0!=(e&_mxm)) //overwrite art extension?
	{
		memcpy(PathFindExtensionA(a+19),cmp,5);
	}
	else //fall back on map/msm (or map/mhm) folder?
	{
		const char*a_19,*ext;
		if(*(DWORD*)(a+10)==0x5C70616D) //map/model?
		if((ext=strrchr(a_19=a+19,'.'))&&!stricmp(ext,cmp))
		{
			memmove((char*)a_19-2,a_19,ext-(a_19)+5);
			memcpy(a+14,cmp+1,3);
			if(EX::data(a+10,art)) e|=_mxm;
		}
	}
	if(0==(e&_mxm)) return 0; //2021
	////////////////////////////
	/////POINT-OF-NO-RETURN/////
	////////////////////////////
	//HACK: this is used to load files from
	//nonstandard locations
	som_art_CreateFile_w = art;

	//can't find a good way to tell these apart
	//DWORD x = som_map_tileviewport?0x407c60:0x442440;
	int ret = ((int(__thiscall*)(void*,void*,char*,char*))x)(this,m,a,b);

	som_art_CreateFile_w = 0; //POINT-OF-NO-RETURN

	return ret;
}

struct SOM_MAP_icon_kv
{
	char k[32];
	SOM_MAP_4921ac::Icon *v;
	bool operator<(const SOM_MAP_icon_kv &b)const
	{
		return strcmp(k,b.k)<0;
	}
};
//static DWORD SOM_MAP_40f830_tick = 0;
extern void SOM_MAP_40f830()
{
//	DWORD ick = EX::tick()-SOM_MAP_40f830_tick;

//	if(ick<5000) Sleep(5000-ick); //ick

	//NOTE: doing this in rapid succession will crash
	//on what looks like exhausting a garbage collector
	//because CreateCompatibleBitmap returned 0

	//static volatile long reentry = 0; //REMOVE ME
	//if(InterlockedIncrement(&reentry)) //avoid crash???
	((SOM_MAP_this*)&SOM_MAP_4921ac)->map_40f830(SOMEX_(A)"\\data\\map\\icon\\");
	//InterlockedDecrement(&reentry);
}
extern SOM_MAP_4921ac::Icon *SOM_MAP_40f830_icon(SOM_MAP_4921ac::Icon *prev,
WCHAR art[MAX_PATH], SOM_MAP_4921ac::Part *pt, std::set<SOM_MAP_icon_kv> *opt)
{
	char *p = pt->icon_bmp;
			
	SOM_MAP_icon_kv kv;
	memcpy(kv.k,*art?pt->msm.file:p,32); 
	CharUpperA(kv.k);		
	p = kv.k; //!
	int punc, hue = '*';
	{
		//incorporate hue into keys?
		extern int PrtsEdit_punc(char*,char**);
		if(punc=PrtsEdit_punc(pt->icon_bmp,0))
		{			
			hue = pt->icon_bmp[punc]; 
				
			//remove 22 character UUID portion?
			if(!*art) memcpy(p+punc+1,"bmp",4);
		}		
	}
	
	SOM_MAP_4921ac::Icon *v = nullptr;
	if(opt)
	{	
		auto ins = opt->insert(kv); //OPTIMIZING
		if(ins.second) 
		{
			opt = nullptr;
			(void*&)ins.first->v = ((void*(__cdecl*)(size_t))0x46573f)(40);
		}
		v = ins.first->v;
	}
	else (void*&)v = ((void*(__cdecl*)(size_t))0x46573f)(40);
	if(!opt)
	{
		memcpy(v->icon_bmp,kv.k,32);

		HBITMAP hb = 0; HICON hi = 0;
		
		if(*art) //EXTENSION
		{
			int sq = SOM_MAP_gen_icons; //gen?GEN_ICONS_SZ:20; //0 //20

			hi = (HICON)LoadImageW(0,art,IMAGE_ICON,sq,sq,LR_LOADFROMFILE|LR_CREATEDIBSECTION);

			//falling back to icon...
			//WARNING: this is erroneous where this MSM file is shared!!
			if(!hi) memcpy(p,pt->icon_bmp,32);
		}
		if(!hi)
		{				
			if(punc) memcpy(p+punc,".bmp",5);

			char buf[MAX_PATH]; sprintf(buf,SOMEX_(A)"\\data\\map\\icon\\%s",p);

			int sq = SOM_MAP_gen_icons==25&&SOM_MAP_expand_icons?21:20;

			hb = (HBITMAP)LoadImageA(0,buf,IMAGE_BITMAP,sq,sq,LR_LOADFROMFILE|LR_CREATEDIBSECTION);				

			if(hb&&hue!='*') SOM_MAP_icons_hue(hb,hue);
		}
		if(!hb&&!hi)
		{
			hb = (HBITMAP)LoadImageA((HINSTANCE)0x400000,MAKEINTRESOURCEA(168),IMAGE_BITMAP,20,20,LR_CREATEDIBSECTION);

			if(!hb) //I think this is a resource exhaustion bug
			{
				if(EX::debug) MessageBeep(-1);

				//if this function is reentered too rapidly it tends to run out of systemwide HBITMAP handles
				//or something. I think they're garbage collected (Aug. 2023)
			//	if(exhausted++<10)
				{
			//		Sleep(250*exhausted); goto exhausted; //doesn't work
				}
			}
		}

		v->il_index = ((SOM_MAP_this*)0x4921ac)->map_40f9d0(hb,hi);

		v->previous = prev; prev = v;
	}
	else InvalidateRect(SOM_MAP.palette,0,0); //HACK
	
	pt->icon = v; return prev;
}
DWORD __thiscall SOM_MAP_this::map_40f830(char*)
{
	if(SOM_MAP_my_plt.empty())
	{	
		SOM_MAP_load_my_plt(); //SOM_MAP_save_my_plt();
	}

//	if(1) return ((DWORD(__thiscall*)(void*,char*))0x40f830)(this,_1);
	
	EX::INI::Editor ed;
	bool gen = !ed->do_not_generate_icons&&128!=SOM_MAP_alpha[0];
	SOM_MAP_gen_icons = gen?GEN_ICONS_SZ:20;
	
//	SOM_MAP_expand_icons = 25==SOM_MAP_gen_icons&&17<=GetSystemMetrics(SM_CXVSCROLL);
	SOM_MAP_expand_icons = 17<=GetSystemMetrics(SM_CXVSCROLL);
		
	auto *tp = (struct SOM_MAP_4921ac*)this;

	int cur = -1; //OPTIMIZING (SPEED)

//	int exhausted = 0; exhausted: //Microsoft bug? //UNCONFIRMED FIX

	for(int i=2;i-->0;) //new allocate
	{
	//	WTH? cur=0 doesn't work when starting out with the classic icons
		if(tp->icons[i]) 
		{
			//ImageList_Destroy(tp->icons[i]->hil);
			((BOOL(__thiscall*)(void*))0x464fd6)(tp->icons[i]);						
	//		cur = 0;
		}
		else (void*&)tp->icons[i] = ((void*(__cdecl*)(size_t))0x46573f)(8);

		//FUN_00464f5a
		tp->icons[i]->_vtable = (void*)0x48052c; 
		tp->icons[i]->hil = 0;
		//tp->icons[i]->hil = ImageList_Create(25,25,32,0,1);		
		((DWORD(__thiscall*)(void*,int,int,int,int,int))0x46501f)(tp->icons[i],25,25,32,0,1); //24		
	}

	std::set<SOM_MAP_icon_kv> optimizing;

	auto swap = x2mdl_dll; 	
	x2mdl_dll = L"x2msm.dll"; //HACK
	enum{ a_s=20 };
	char a[MAX_PATH] = SOMEX_(A)"\\data\\map\\model\\";
	wchar_t art[MAX_PATH]; art[0] = '\0'; //gen?	

	int hlp = GetWindowContextHelpId(som_tool);

	HWND text = 0;

	for(auto*p=tp->icons_llist;p;)
	{
		auto *q = p; p = p->previous;

		((void(__cdecl*)(void*))0x465768)(q); //free?
	}

	SOM_MAP_4921ac::Icon *prev = nullptr;	
	auto *pt = tp->parts;	
	//for(int i=tp->partsN;i-->0;pt++)
	for(int i=0;i<(int)tp->partsN;i++,pt++)
	{		
		//NOTE: convert art at start in case using classic icons
		//(in order to avoid loading when sliding slider)
		if(gen||40100==hlp)
		{
			memcpy(a+a_s,pt->msm.file,32);
		//	memcpy(PathFindExtensionA(a+a_s),".ico",4);
			using namespace x2mdl_h;
			#if 0 && defined(_DEBUG)
			//this always generates icons (models too)
			extern bool som_art_nofollow;
			som_art_nofollow = true;
			int e = som_art_model(a,art);
			som_art_nofollow = false;
			if(e&_art)
			#else
			int e = som_art_model(a,art);
			if(e&_art&&~e&_lnk)
			#endif
			{
				if(40100==hlp) //keep loading prompt alive
				{
					if(!text)
					{
						text = GetDlgItem(som_tool,2021);	
						if(text=GetWindow(text,GW_HWNDNEXT))
						if(GetWindowTextW(text,som_tool_text,MAX_PATH))
						SetDlgItemTextW(som_tool,1151,som_tool_text);
					}
					MSG keepalive; 
					while(PeekMessage(&keepalive,0,0,0,PM_REMOVE))
					{
						TranslateMessage(&keepalive);
						DispatchMessage(&keepalive); 
					}
				}				

				if(!som_art(art,0)) //x2mdl exit code?
				{
					e = som_art_model(a,art); //retry?
				}
			}
			if(e&_ico&&gen)
			wmemcpy(PathFindExtension(art),L".ico",4);
			else *art = '\0';
		}

		prev = SOM_MAP_40f830_icon(prev,art,pt,&optimizing);		
	}
	tp->icons_llist = prev;

	x2mdl_dll = swap;
	
	return 1; //HACK
}
//DWORD __thiscall SOM_MAP_this::map_40f9d0(char **bmp)
DWORD __thiscall SOM_MAP_this::map_40f9d0(HBITMAP b, HICON hi)
{
	ICONINFO ii = {}; if(hi) 
	{
		GetIconInfo(hi,&ii); b = ii.hbmColor;
	}

	//NOTE: SOM_MAP's code uses LR_CREATEDIBSECTION
	//and if the file fails tries to load a resource
//	auto b = (HBITMAP)LoadImageA(0,*bmp,0,0,0,LR_LOADFROMFILE);
	if(!b) return ~0;	
	
	//HARD TO BELIEVE MOST OF 40f9d0 BOILS
	//DOWN TO 3 LINES OF CODE
//	auto ret = ImageList_AddMasked(il,b,0);
	//ImageList_AddMasked returns the index
	//return ret?ImageList_GetImageCount(il)-1:~0;
//	if(0){ DeleteObject(b); return ret; }

	//BITMAPINFO bmi = {sizeof(BITMAPINFOHEADER)}; //bmi	
	
	HDC dc = som_tool_dc;
	//auto swap = SelectObject(dc,b);
	//WARNING: writes into bmi.bmiColors?!
	//int test20 = GetDIBits(dc,b,0,!20,0,&bmi,0); //buf
	
	int sq = 25==SOM_MAP_gen_icons?25:20;

	if(sq==20&&SOM_MAP_expand_icons) sq = 21;

	int sq2 = sq*sq;

	BITMAPINFO bmi = {sizeof(bmi),sq,sq,1,32,0,4*sq*sq};

	RGBQUAD buf[25*25];

	auto tp = (struct SOM_MAP_4921ac*)this;

	int test20 = GetDIBits(dc,b,0,sq,buf,&bmi,0);

	static const float lum[3] = { 0.00087058632f,0.00277254292f,0.00027843076f};

	if(sq==25) //2024: classic icons are bright enough
	{
		float mib = EX::INI::Editor()->map_icon_brightness;
		if(float x=mib)
		{
			if(x<0||x>1)
			{
				if(x>1) x-=1;

				for(int i=sq2;i-->0;)
				{
					float r = buf[i].rgbRed+x*buf[i].rgbRed;
					float g = buf[i].rgbGreen+x*buf[i].rgbGreen;
					float b = buf[i].rgbBlue+x*buf[i].rgbBlue;

					buf[i].rgbRed = (BYTE)min(255,max(0,r));
					buf[i].rgbGreen = (BYTE)min(255,max(0,g));
					buf[i].rgbBlue = (BYTE)min(255,max(0,b));
				}
			}
			else for(int i=sq2;i-->0;)
			{
				buf[i].rgbRed*=x;
				buf[i].rgbGreen*=x;
				buf[i].rgbBlue*=x;
			}
		}
	}
	/*if(arrows&&*arrows) //SOM_MAP_PlgBlt_icons
	{
		float l = 0;
		for(int i=sq2;i-->0;)
		l+=buf[i].rgbRed*lum[0]+buf[i].rgbGreen*lum[1]+buf[i].rgbBlue*lum[2];
		l/=sq2;
		int invert = l>0.375f?-1:1;
		//HACK? playing with luminosity cancels out at gray
		//so this just makes it so gray is the most brilliant
		//it tends to work alright
		l = 1-(l-0.5f);
	//	if(invert!=-1) //HACK
	//	l/=2;
		for(int i=sq2;i-->0;)
		{
			int a = l*invert*(*arrows)[sq2-1-i];
			buf[i].rgbRed = (BYTE)min(255,max(0,buf[i].rgbRed+a));
			buf[i].rgbGreen = (BYTE)min(255,max(0,buf[i].rgbGreen+a));
			buf[i].rgbBlue = (BYTE)min(255,max(0,buf[i].rgbBlue+a));
		}
	}*/
	if(!hi)
	{			
		for(int i=25*25;i-->0;) //HACK: 
		{
			buf[i].rgbReserved = 0; //255; //UNNECESSARY?
		}

		if(25==sq)
		{
			int sq21 = SOM_MAP_expand_icons?21:20;
		
			//for(int i=22;i-->2;) //upside-down?
			for(int i=2;i<=22;i++)
			{
			 	auto *dst = buf+i*25;				
			//	memmove(dst+2,dst-25*2,4*sq21);
				memmove(dst+2,dst+25*2,4*sq21);
			}
		}
		else if(21==sq) //21
		{
			for(int i=21;i-->0;) 
			{
				DWORD *dst = (DWORD*)buf+i*21;

				if(i==0) //upside-down
				{
					memset(dst,0x20,4*21);
				}
				else dst[20] = 0x20202020;
			}
		}
	}
	else if(sq==25) //2024: classic icons are bright enough
	{
		float mib = EX::INI::Editor()->map_icon_brightness;
		float brighten = 20/(mib?mib:1);

		for(int i=25*25;i-->0;) //HACK
		{
			auto &alpha = buf[i].rgbReserved;

			if(alpha!=255) //non-wall/black? 
			{
				float a = alpha/255.0f, l_a = 1/a;
				float contrast = powf(2.2f-a,0.666f); //0.5f

				int bright = 1.5f*((a-0.5f)*brighten); //compensate?

				for(int k=3;k-->0;) //4
				{
					auto &kc = ((BYTE*)(buf+i))[k];
				//	float l = kc/255.0f*a-0.5f; //premultiply?
					float l = kc/255.0f-0.5f;
					l = 0.5f+l*contrast;
				//	l*=l_a; //premultiply?
					kc = (BYTE)min(255,max(0,bright+255*l));
				}
			
				//HACK: invert alpha channel so that icons
				//will draw full alpha as 0, and same for
				//palette background
				alpha = 255-alpha;
			}
			else //TODO: brighten walls?
			{
				alpha = 0;

				for(int k=3;k-->0;) 
				if(*(DWORD*)(buf+i))
				{
					auto &kc = ((BYTE*)(buf+i))[k];

					kc = (BYTE)min(255,brighten+kc);
				}
			}
		}
	}

	auto &b2 = SOM_MAP_icon_bitmap;

	//auto testo = SelectObject(dc,SOM_MAP_icon_bitmap);

	test20 = SetDIBits(dc,b2,0,sq,buf,&bmi,0);

	//HACK: use 32-bit b2 if falling back to BMP file
	//int ret = ImageList_Add(tp->icons[0]->hil,b,ii.hbmMask);
	int ret = ImageList_Add(tp->icons[0]->hil,b2,0);

	if(sq==test20)
	{		
		enum{ lut_s=96 };
		BYTE (&lut)[lut_s] = *map_40f9d0_lut;

		//NOTE: dark blues get masked unless 0.9
		//is added before converting to integers
		for(int i=sq2;i-->0;) if((DWORD&)buf[i])
		{
			auto p = (BYTE*)&buf[i];

			float l = 
			p[2]*lum[0]+ //R=0.222
			p[1]*lum[1]+ //G=0.707
			p[0]*lum[2]; //B=0.071

			p[0] = p[1] = p[2] = lut[(int)((lut_s-1)*l)];
		}
		test20 = SetDIBits(dc,b2,0,sq,buf,&bmi,0);
	}
	else //not supporting irregular icons!!
	{
		//it would be a problem if the indices
		//were to not match!

		//emulate map_46cbc8
		RECT r = {0,0,sq,sq};
		SetBkColor(dc,0x404040);
		ExtTextOutA(dc,0,0,2,&r,0,0,0);
	}
	//swap = SelectObject(dc,swap);
	
	//NOTE: AddIcon also exists/works
//	int same = ImageList_AddMasked(tp->icons[1]->hil,b2,0);	
	int same = ImageList_Add(tp->icons[1]->hil,b2,0);
	
	assert(same==ret);

	if(hi) DeleteObject(ii.hbmMask);
	if(hi) DestroyIcon(hi); //DeleteObject is a memory leak

	DeleteObject(b); //DeleteObject(b2);
	
	return ret;
}
void __thiscall SOM_MAP_this::map_46cbc8(RECT *r, DWORD s)
{
	HDC dc = *(HDC*)((BYTE*)this+4);

	int os; switch(s)
	{
	case 0xffffff: case 0x404040: os = 0x84; break; //contents?
	case 0x666666: case 0xcccccc: os = 0x4c; break; //checkpoints?
	}
	
	auto tile = (SOM_MAP_4921ac::Tile*)((BYTE*)&r+os); //esp+84

	Part *p = 0;	
	if(tile->part!=0xffff)
	p = bsearch_for_413a30(0,tile->part);
	if(!p) //CDC::FillSolidRect (46cbc8)
	{
		assert(tile->part==0xffff);

		SetBkColor(dc,s); ExtTextOutA(dc,0,0,2,r,0,0,0);

		return;
	}

	HDC dc2 = som_tool_dc;
	auto il = SOM_MAP_4921ac.icons[1]->hil;
	auto swap = SelectObject(dc2,SOM_MAP_icon_bitmap);	
	ImageList_Draw(il,p->icon->il_index,dc2,0,0,0);
	SOM_MAP_PlgBlt_icons(dc,r->left,r->top,20,20,dc2,19,19,-20,-20,tile->spin);
	SelectObject(dc2,swap);

	if(os==0x4c&&isupper(tile->ev)) //2023
	{
		s = tolower(tile->ev)=='e'?0x666666:0xcccccc;

		if(HWND next=som_tool_x(GetDlgItem(som_tool,1222)))
		{
			static WCHAR x[32];
			int xlen = GetWindowTextW(next,x,32);
			wchar_t *n2 = som_tool_nth(x,xlen,2,0);
			wchar_t *n3 = som_tool_nth(x,xlen,3,0);
					
			SetBkMode(dc,TRANSPARENT);
			SetTextColor(dc,0x000000);
			DrawTextW(dc,n2,1,r,DT_CENTER|DT_VCENTER);
			SetTextColor(dc,0xffffff);
			DrawTextW(dc,n3,1,r,DT_CENTER|DT_VCENTER);
		}
	}

	if(s>0x666666) //PlgBlt doesn't have rops
	{
		auto hb = (HBITMAP)GetCurrentObject(dc,OBJ_BITMAP);
		BITMAP bm; GetObject(hb,sizeof(BITMAP),&bm);

		//HRGN rgn = CreateRectRgnIndirect(r);
		//InvertRgn(dc,rgn);
		SOM_MAP_InvertRgn_InvertRgn(*r,bm);
		//DeleteObject((HGDIOBJ)rgn);		
	}
}
void __thiscall SOM_MAP_this::map_413b30_checkpoint(DWORD y, DWORD x, Tile *p)
{
	//HACK: this just works to identify the callers
	//NOTE: right-drag knocked out and right will probably be removed
	//when context-menus are added. it's just better to do something
	//than nothing	
	auto cmp = (char*)p-(char*)&y; enum{ right=36, down=40, drag=44 };

	//since the classic way used the right button for one color and the
	//left for the other, this way only the left button is used and the
	//color is inverted
	if(cmp!=drag)
	{
		if(GetKeyState(VK_SHIFT)>>15)
		{
			SOM_MAP_tileviewtile = ~0; return;
		}		

		BYTE ev = SOM_MAP_4921ac.grid[x+y*100].ev;

		auto il = islower(ev);
		
		if(cmp==down)
		{
			ev = tolower(ev)=='e'?'v':'e';

			if(!il) ev = toupper(ev);
		}
		else //right
		{
			ev = (BYTE)(il?toupper:tolower)(ev);
		}
				
		SOM_MAP_tileviewtile = p->ev = ev;

		if(cmp==down&&SOM_MAP_GetKeyState_Ctrl()) return;
	}
	else if(~0!=SOM_MAP_tileviewtile)
	{
		p->ev = (BYTE)SOM_MAP_tileviewtile;
	}
	else return; //reserving/nondestructive way to select the tile

	((void(__thiscall*)(void*,int,int,void*))0x413b30)(this,y,x,p);
}

extern int som_tool_plusminus(int);
int __thiscall SOM_MAP_this::map_419fc0_keydown(int w, int l)
{
	//TODO: I don't think protecting this is necessary
	//auto swap = som_tool_VK_CONTROL;
	assert(!som_tool_VK_CONTROL);

	auto ctrl = SOM::Tool.GetKeyState(VK_CONTROL)>>15;
	int shift = GetKeyState(VK_SHIFT)>>15;

	if(w>='1'&&w<='6') //layer switch?
	{
		HWND cb = GetDlgItem(som_tool,1133);
		if(-1==ComboBox_SetCurSel(cb,w-'1'))
		return MessageBeep(-1);
		return PostMessage(som_tool,WM_COMMAND,
		MAKEWPARAM(1133,CBN_SELENDOK),(LPARAM)cb); 
	}
	else switch(w) //2022: all but VK_RETURN are new combos
	{
	case 192: //`

		SendDlgItemMessage(som_tool,1133,CB_SHOWDROPDOWN,1,0); 
		SetFocus(GetDlgItem(som_tool,1133));
		break;

	case VK_F1: case VK_F2: case VK_F3:

		w = '1'+(w-VK_F1); //case '1': case '2': case '3':

		if(!ctrl) som_tool_VK_CONTROL = 0xff80; break;

	case VK_F4: return PostMessage(som_tool,WM_COMMAND,1227,0);

	case 'A': if(!ctrl) w = VK_ADD; break; //TODO: Select All?

	case 'S': if(!ctrl) w = VK_SUBTRACT; break; //Ctrl+S saves

	case 'E': if(!ctrl) return som_tool_plusminus('=');
		
		w = 'I'; break; //Ctrl+E -> Ctrl+I (I is odd/far away)
			  
	case 'F': case 'G': if(ctrl) break; //flip?
	
		som_tool_VK_CONTROL = 1; w = w=='F'?'Z':'X';

		if(!shift) //simulate 2 presses?
		((int(__thiscall*)(void*,int,int))0x419fc0)(this,w,0);

		break;
	
	case 'C': if(ctrl) break; //C -> Ctrl+P
	
		som_tool_VK_CONTROL = 0xff80; w = 'P'; break;

	case 'D': //if(!ctrl) break; //Ctrl+D -> Delete

		if(!ctrl) //D to copy elevation
		{
			//TODO? it would be nice if the buttons
			//could do this with Shift, however the
			//key inputs don't work well with Shift

			auto *ecx = (SOM_MAP_this*&)_c[0xbc];
			int y = 99-ecx->_i[0x7C/4], x = ecx->_i[0x78/4];
			auto &sel = SOM_MAP_4921ac.grid[100*y+x];
			SetDlgItemFloat(som_tool,1000,sel.z);
			return 1;
		}				

		som_tool_VK_CONTROL = 1; w = VK_DELETE; break;

	case 'V': if(!ctrl) w = VK_RETURN; break; //V -> Return/Space
	
	case VK_RETURN: 
		
		//NOTE: Ctrl+Space/Return was not handled by the previous
		//extension code (adding V makes this mainly theoretical)
		if(ctrl) som_tool_VK_CONTROL = 1; break;

	case VK_UP: case VK_DOWN: if(!ctrl) break; //standardize this

		som_tool_VK_CONTROL = 1; w = w==VK_UP?VK_PRIOR:VK_NEXT; break;

	case 'L': if(!ctrl) break; //Ctrl+L

		if(auto ctrlock=GetDlgItem(som_tool,1224))
		{
			Button_SetCheck(ctrlock,!Button_GetCheck(ctrlock));
			return 1;
		}
		break;

	case 'Z': if(!ctrl) break;

		if(shift) goto redo;

		SOM_MAP_undo(); return 1;

	case 'Y': if(!ctrl) break; redo:
		   
		SOM_MAP_redo(); return 1;
	}

	//NOTE: som_tool_plusminus is catching regular input
	//via +/-/= which is not able to use shift very well
	//because shift+= is + and shift+_ is - ... plus the
	//buttons don't have this shift functionality
	bool as = (w==VK_ADD||w==VK_SUBTRACT)&&shift; if(as)
	{
		float inc = GetDlgItemFloat(som_tool,999); 
		*(float*)0x478518 = inc?inc:0.1f; //41a550/41a6b0
	}

	//REMINDER: VK_RETURN processess lParam bit 31 (repeat)
	l = ((int(__thiscall*)(void*,int,int))0x419fc0)(this,w,l);	

	if(as) *(float*)0x478518 = 0.5f; //41a550/41a6b0

	if(l&&w=='P') //C/Ctrl+P
	{
		auto *ecx = (SOM_MAP_this*&)_c[0xbc];
		ecx->repair_pen_selection();
	}

	som_tool_VK_CONTROL = 0; return l;
}

void __thiscall SOM_MAP_this::map_430180_palette(int vk)
{
	auto vp = (DWORD*)SOM_MAP_app::CWnd(SOM_MAP.palette);
	auto&x = vp[0x48/4];
	auto y = vp[0x4c/4];
	auto w = vp[0x54/4];
	if(y>=SOM_MAP_4921ac.partsN/w)
	w = SOM_MAP_4921ac.partsN%w;

	if(vk==2)
	{
		if(!x--) x = w-1; //wrap?

		vk = 6; //nop
	}
	else if(vk==3)
	{
		if(++x==w) x = 0; //wrap

		vk = 6; //nop
	}

	((void(__thiscall*)(void*,int))0x430180)(this,vk);
}