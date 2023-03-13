
#include "Ex.h" 
EX_TRANSLATION_UNIT

#include <vector> 
#include <algorithm> 

#include "dx.ddraw.h"

#include "Ex.ini.h"
#include "Ex.cursor.h"
#include "Ex.output.h"

#include "som.932.h"
#include "som.state.h"
#include "som.extra.h"
#include "som.tool.hpp" 
			
#include "../lib/swordofmoonlight.h" //prf
#include "../x2mdl/x2mdl.h"

namespace workshop_cpp
{
	namespace prf = SWORDOFMOONLIGHT::prf;

	typedef swordofmoonlight_prt_part_t part_prt;

	//WARNING INHERITS ALIGNMENT
	struct prf_header
	{
		char name[31],model[31]; 
	};
	template<class T=prf::move_t>
	//TODO: use prf::image_t instead of this way
	struct item_prf : prf_header, prf::item_t, T
	{
		char icard[97];

		template<class TT>
		TT &get(){ T *p = this; return *(TT*)p; }
	};
	struct object_prf : prf_header, prf::object_t
	{
		char icard[97];
	};
	struct enemy_prf : prf_header, prf::enemy_t
	{
		//char icard[97];
															
		//HACK: SfxEdit data stored in EneEdit_4173DC
		//WORD &my_sfx(){ return *(WORD*)&_unknown5; }	
		WORD &my_sfx(){ return *(WORD*)&inclination; }			
		BYTE &my_screeneffect(){ return *(BYTE*)(&my_sfx()+1); }
		char *my_icard(){ return descriptions[0]; }
	};
	struct npc_prf : prf_header, prf::npc_t
	{
		//char icard[97];
	};
	struct prf_header2
	{
		char name[31]; 
	};	
	struct my_prf : prf_header2, prf::magic_t
	{
		char icard[97];
	};

	static int datamask = 0; struct data
	{
		BYTE time,hit;
		WORD sfx,snd; BYTE cp; signed char pitch;

		bool operator<(data &cmp)const{ return time<cmp.time; }

		bool operator+=(data &add)
		{
			//NOTE: assuming the 0th SFX and SND is not
			//possible, so that swprintf can be used to
			//display effects
			//NOTE: allowing identical sounds to occupy
			//the same times
			if(snd&&add.snd||sfx&&add.sfx) return false;
			hit+=add.hit;
			sfx+=add.sfx; cp+=add.cp;
			snd+=add.snd; pitch+=add.pitch; return true;
		}

		int print(wchar_t (&w)[64])
		{
			//NOTE: treating -1 & 0 as equivalent so the
			//table is easier to read
			wchar_t x[3] = {hit,' '};
			return swprintf(w,L"|%02d|   %s%4d@%-3d  %4d%-+3d  |%02d|",time,x,sfx,cp,snd,pitch,time);
		}
		int scan(const wchar_t *p)
		{
			const wchar_t *pp;
			for(int j=0;j<6;j++)
			{
				while(!isdigit(*p)) 
				p++;
				pp = p;
				while(isdigit(*p))
				p++;
				if(!*p) return j;
				switch(j)
				{
				case 1: if('@'==*p){ j++; hit = 0; }
						break;
				case 5: if('-'==pp[-1]) pp--; 
						break;
				}		
				int i = _wtoi(pp);
				switch(j)
				{
				case 0: time = i; break;
				case 1: hit = '0'+i; break;
				case 2: sfx = i; break;
				case 3: cp = i; break;
				case 4: snd = i; break;
				case 5: pitch = i; break;
				}				
			}
			return 6;
		}
	};

	struct sfx_record //48B
	{
		//SfxEdit_ReadFile may still contain a study of these 
		//fields
		BYTE procedure;		
		BYTE model;							
		BYTE unknown6[6];	
		FLOAT floats8[8];		
		SHORT sound2;	
		WORD extend1;
		CHAR pitch24;
		BYTE zero3[3];
	};
};

//Reminder: this is found just after GetMessageA is called
//I couldn't find where this comes from but I worked it out
//backward from the following code that accesses m_pMainWnd
//0041C1AD 81 7E 34 6A 03 00 00 cmp         dword ptr [esi+34h],36Ah 
static struct ItemEdit_471ab0 : SOM_CWinApp
{
	BYTE unknown[72]; 
	
	//MOVING 4*17B into 471B80 from 471bc8? (4*18B apart) 
	//0040701B B9 11 00 00 00       mov         ecx,11h  
	//00407020 BF 80 1B 47 00       mov         edi,471B80h  
	//00407025 F3 A5                rep movs    dword ptr es:[edi],dword ptr [esi]
	/*Looks like this
	0x00471B80  00000001 00000000 00000000 0000ffff  ................
	0x00471B90  00000002 00000001 00000004 00000002  ................
	0x00471BA0  00000001 00000002 00000002 00000001  ................
	0x00471BB0  00000001 00000001 80000000 80000000  ................
	0x00471BC0  00000000 padding..471BC8...repeated
	*/
	//ItemEdit
	//00407B1B 39 05 B8 1B 47 00    cmp         dword ptr ds:[471BB8h],eax  
	//00407B21 74 1E                je          00407B41  
	//00407B23 A3 B8 1B 47 00       mov         dword ptr ds:[00471BB8h],eax 
	int _471BB8; //???

	//NOTE: It seems like ONLY ObjEdit does this... BUT NOT
	//SO... ItemEdit's prompt has no rhyme or reason????????
	//004057F9 8A 86 00 05 00 00    mov         al,byte ptr [esi+500h]
	BYTE &save(){ return ((BYTE*)m_pMainWnd)[0x500]; }

	//19FE50
	typedef workshop_cpp::item_prf<> view;
	view &data(){ return (view&)((BYTE*)m_pMainWnd)[0x504]; }

	struct MainWnd: SOM_CDialog //19F94C
	{
		DWORD unknown0; //zero? type?

		char *shift_jis; //wild guess

		char *mdo_filename;

		//menu/prize
		int tilt1,tilt2; float center; //19F9B0 

		//0 is item view?
		//1 is prize view? 
		//2 is set when neither is checked??? can't 
		//recall if workshop.cpp is setting this to
		//2, but it is set to 2
		DWORD mode; 

		DWORD unknown3; //34302f1? 40abf? (hard to say)
		FLOAT unknown4; //-3.0 (float? more follow)
	};

	MainWnd &main(){ return *(MainWnd*)m_pMainWnd; }

}&ItemEdit_471ab0 = *(struct ItemEdit_471ab0*)0x471ab0;
static struct ObjEdit_476B00 : SOM_CWinApp
{
	//0040A5A9 8A 86 04 05 00 00    mov         al,byte ptr [esi+504h] 
	BYTE &save(){ return ((BYTE*)m_pMainWnd)[0x504]; }

	typedef workshop_cpp::object_prf view;
	view &data() 
	{
		//REMINDER: this note seems worth saving?
		//ObjEdit
		//x0042bc50 points to opened PRF
	//	assert(!"GUESSING similar to item edit");		
		return (view&)((BYTE*)m_pMainWnd)[0x508];
	}

}&ObjEdit_476B00 = *(struct ObjEdit_476B00*)0x476B00;

static workshop_cpp::enemy_prf
/*EneEdit, ebx holds static part of the read PRF file
004033FE 89 1D DC 73 41 00    mov         dword ptr ds:[4173DCh],ebx  
00403404 89 0D A8 70 41 00    mov         dword ptr ds:[4170A8h],ecx  
//this is the MDL with various pointers into the file
//as-is, followed by other pointers to data generated
//from the file... maybe the 16-bit data is discarded
//the code that deletes it is just above it
0040340A 89 2D A0 70 41 00    mov         dword ptr ds:[4170A0h],ebp  
00403410 89 15 CC 71 41 00    mov         dword ptr ds:[4171CCh],edx  
00403416 FF 15 8C 41 41 00    call        dword ptr ds:[41418Ch]  
*/
*&EneEdit_4173DC = *(workshop_cpp::enemy_prf**)0x4173DC;
			
static workshop_cpp::npc_prf
/*NpcEdit
004033DE 89 1D 1C 76 41 00    mov         dword ptr ds:[41761Ch],ebx 
*/
*&NpcEdit_41761C = *(workshop_cpp::npc_prf**)0x41761C;

extern HWND &som_tool;
extern HDC som_tool_dc;
extern char som_tool_play;
extern HWND som_tool_taskbar; 
extern int som_tool_initializing; 
extern HWND som_tool_stack[16+1];
extern std::vector<WCHAR> som_tool_wector; 
extern POINTS som_tool_tile, som_tool_tilespace;
extern HANDLE WINAPI som_map_LoadImageA(HINSTANCE,LPCSTR,UINT,int,int,UINT);

//0: maximized 1: minimized
extern BYTE workshop_mode = 0;
//unique across workshop tools
//0: Item model mode
//1: PrtsEdit pen mode
//15: Item display mode
//16: Item lying down mode
//0~31: Ene/NpcEdit animation ID
extern BYTE workshop_mode2 = 15;
extern HMENU &workshop_mode2_15_submenu;
extern WORD workshop_category = 0;
static LPARAM workshop_cursor = 0;
extern HWND &workshop_tool; 
extern HWND &workshop_host;
extern HMENU &workshop_menu;
extern HANDLE &workshop_reading; 
extern RECT &workshop_picture_window;	
extern const wchar_t* &workshop_title;
typedef struct 
{
	char icard[97]; std::wstring xicard;

}workshop_t;
static workshop_t *workshop = 0;

extern wchar_t som_tool_text[MAX_PATH];
extern wchar_t (&workshop_savename)[MAX_PATH];

//NOTE: mainly for throttling ItemEdit
extern unsigned workshop_antialias = 0;
enum{ workshop_throttle=!EX::debug||1 };
static void workshop_redraw()
{
	if(SOM::tool==PrtsEdit.exe) return;
	HWND pw = GetDlgItem(som_tool,1014);
	if(SOM::tool==ItemEdit.exe&&workshop_throttle) 
	{
		SendMessage(pw,WM_SETREDRAW,1,0);		
		workshop_antialias = DDRAW::noFlips;
		
		if(0&&EX::debug) //visualizing throttle
		{
			SetDlgItemInt(som_tool,1050,DDRAW::noFlips,0);
		}
	}
	//2021: post?
	//InvalidateRect(pw,0,0);
	RedrawWindow(pw,0,0,RDW_INVALIDATE);
}

//TODO: move this elsewhere, though the tools don't need it
static void workshop_accel2(HMENU hm, std::vector<ACCEL> &av)
{
	MENUITEMINFO mii = {sizeof(mii),MIIM_ID|MIIM_STRING|MIIM_SUBMENU};
	for(int i=0,iN=GetMenuItemCount(hm);i<iN;i++)
	{
		WCHAR w[64],u;
		mii.cch  = 64; mii.dwTypeData = w; 
		if(!GetMenuItemInfo(hm,i,1,&mii)) continue;
		if(mii.hSubMenu){ workshop_accel2(mii.hSubMenu,av); continue; }	

		switch(mii.wID)
		{
		case 34001:

			if(ItemEdit.exe==SOM::tool)
			{
				workshop_mode2_15_submenu = hm;
			}
			break;
		}

		LPWSTR p = wcschr(mii.dwTypeData,'\t'); if(!p) continue;
		ACCEL a = {FVIRTKEY,0,mii.wID};		
		while(*++p) switch(u=toupper(*p))
		{
			//for what it's worth EneEdit had
			//such shortcut names even though
			//otherwise written with Japanese
		case 'C': 
			if(wcsnicmp(p,L"Ctrl",4)||p[4]!='+')
			goto def;
			a.fVirt|=FCONTROL; p+=4; break;
		case 'A':
			if(wcsnicmp(p,L"Alt",3)||p[3]!='+')
			goto def;
			a.fVirt|=FALT; p+=3; break;
		case 'S':
			if(wcsnicmp(p,L"Shift",5)||p[5]!='+')
			goto def;
			a.fVirt|=FSHIFT; p+=5; break;				
		default: def: 			
			if(int l=wcslen(p)) switch(l)
			{
			case 1: 
				if(isalpha(u)) a.key = u;
				break;
			case 2: case 3: 
				if('F'==u&&isdigit(p[1]))
				{
					int i = _wtoi(p+1);
					if(i>=1&&i<=12) a.key = VK_F1+(i-1);
				}
			default: p+=l-1; //THE END
			}
		case ' ': break;
		}
		//F1?
		//if(a.fVirt!=FVIRTKEY&&a.key) av.push_back(a);
		if(a.key) av.push_back(a);
	}
}
extern HACCEL &workshop_accel;
extern HACCEL workshop_accelerator(HMENU hm)
{
	assert(hm||SOM::tool>=EneEdit.exe);
	if(!workshop_menu) hm = LoadMenu(0,LPWSTR(SOM::tool==SfxEdit.exe?129:101));
	std::vector<ACCEL> av;
	workshop_accel2(hm,av);
	if(!workshop_menu) DestroyMenu(hm);
	return workshop_accel = av.empty()?0:CreateAcceleratorTable(&av[0],av.size());
}

static void workshop_menu_COMMAND(WPARAM wp)
{
	MENUITEMINFO mii = {sizeof(mii),MIIM_STATE};
	GetMenuItemInfo(workshop_menu,wp,0,&mii);
	mii.fState^=MFS_CHECKED;
	if(SOM::tool==ItemEdit.exe)		
	SetMenuItemInfo(workshop_menu,wp,0,&mii);	
}
static void workshop_submenu(HMENU hm)
{
	POINT pt; GetCursorPos(&pt);
	//how to pause the (choppy) item animation like the window menu?
	DDRAW::isPaused = true; //workshop_throttleproc
	BOOL cmd = TrackPopupMenuEx(hm,TPM_RETURNCMD,pt.x,pt.y,som_tool,0);		
	DDRAW::isPaused = false; 
	SendMessage(som_tool,WM_COMMAND,cmd,0);
}

static windowsex_DIALOGPROC workshop_32768;
static void workshop_icard_32768()
{	
	int i = -1;
	const wchar_t *icard;
	if(workshop->xicard.empty())
	{
		if(!*workshop->icard)
		goto edit;
		icard = EX::need_unicode_equivalent(932,workshop->icard);
	}
	else icard = workshop->xicard.c_str();	
	while(i<5) 
	if(32768==GetMenuItemID(workshop_menu,++i))
	{
		RECT rc;
		GetMenuItemRect(som_tool,workshop_menu,i,&rc);				
		HMENU hm = CreatePopupMenu();
		wchar_t *pp,*p;
		for(p=pp=const_cast<wchar_t*>(icard);*p;pp=p)
		{
			while(*p&&*p!='\n') p++; 
			wchar_t d = *p; *p = '\0';
			InsertMenu(hm,-1,MF_BYPOSITION,1,pp); 
			*p = d;	if(d) p++;		
		}	
		if(i=TrackPopupMenuEx(hm,TPM_RETURNCMD,rc.left,rc.bottom,som_tool,0))
		{
			//TODO? could pass the menu item to WM_INITDIALOG, except that
			//they don't necessarily correspond, although usually they are
			//the same unless the language/theme pack has additional lines
	edit:	HRSRC found = FindResource(0,MAKEINTRESOURCE(32768),RT_DIALOG);
			if(DLGTEMPLATE*locked=(DLGTEMPLATE*)LockResource(LoadResource(0,found)))			
			CreateDialogIndirectParamA(0,locked,som_tool,workshop_32768,0);				
		}
		DestroyMenu(hm); return;
	}
}

static VOID CALLBACK workshop_tool_timer(HWND,UINT,UINT_PTR,DWORD)
{
	if(!workshop_tool) 
	EX::is_needed_to_shutdown_immediately(0,"workshop_tool_timer");

	if(IsWindow(workshop_tool)) return;
	
	workshop_tool = 0; PostQuitMessage(0);
}

static void workshop_skin(const char *txr=0)
{
	if(!workshop_menu) return;

	assert(SOM::tool>=EneEdit.exe&&SOM::tool!=SfxEdit.exe);

	wchar_t w[64]; 
	static const wchar_t *reset = 0; if(!reset)
	{
		if(!GetMenuString(workshop_menu,40074,w,64,0))
		return;
		reset = wcsdup(w);
	}

	MENUITEMINFO mii = {sizeof(mii),MIIM_STRING};
	if(txr&&*txr) 
	{
		const wchar_t *t = wcschr(reset,'\t');
		const wchar_t *u = EX::need_unicode_equivalent(932,txr);
		if(t) while('.'==t[-1]&&t>reset) t--;

		swprintf(w,L"%s%s",u,t?t:L"");
		mii.dwTypeData = w;
	}
	else mii.dwTypeData = (LPWSTR)reset;
	SetMenuItemInfo(workshop_menu,40074,0,&mii);
}

static void workshop_titles(const wchar_t *t=workshop_title)
{	
	if(t==0||!*t)
	{
		t = workshop_title;
		if(!t||!*t) return;
	}

	if(som_tool_taskbar) 
	{
		SetWindowText(som_tool_taskbar,t);
		
		/*if(t!=workshop_title&&workshop_mode)
		{
			wchar_t w[96];
			swprintf(w,L"%s  <%s>  %s",t,EX::exe(),workshop_title);
			SetWindowTextW(som_tool,w);
			return;
		}*/
	}
	SetWindowTextW(som_tool,t);
}
extern const char *workshop_directory(int sfx=false)
{
	switch(SOM::tool)
	{
	case ItemEdit.exe: return "item";
	case ObjEdit.exe: return "obj";
	case EneEdit.exe: return "enemy";
	case NpcEdit.exe: return "npc";
	case SfxEdit.exe: return sfx?"sfx":"my"; 
	default: assert(SOM::tool==PrtsEdit.exe); return "map";
	}
}
static const char *workshop_subdirectory(int id)
{
	/*2022: preferring map/model
	switch(id)
	{	
	case 40074: return "texture"; 
	case 1006: return "mhm"; case 1013: return "icon";
	default: assert(0);
	case 1005: return PrtsEdit.exe==SOM::tool?"msm":"model"; 
	}*/
	switch(id)
	{	
	case 40074: return "texture"; 
	case 1013: return "icon";
	default: assert(0);
	case 1005: case 1006: return "model"; 
	}
}
static void workshop_xtitles()
{		
	//following som_tool_prf's lead here
	typedef SWORDOFMOONLIGHT::zip::inflate_t xprof;
	const char *dir = workshop_directory();
	wchar_t w[MAX_PATH];
	int i = swprintf(w,L"data\\%hs\\%hs\\",
	dir,SOM::tool==PrtsEdit.exe?"parts":"prof");
	const wchar_t *p = PathFindFileNameW(workshop_savename);
	for(;*p&&i<MAX_PATH-1;i++) w[i] = *p++;
	w[i] = '\0';	
	extern size_t som_tool_xdata(const wchar_t*,xprof&);
	xprof x;
	size_t x_s = som_tool_xdata(w,x);
	workshop_cpp::enemy_prf &e = *(workshop_cpp::enemy_prf*)&x[0];	
	if(PrtsEdit.exe==SOM::tool)
	{
		if(x_s>=228)
		{
			char *name = ((workshop_cpp::part_prt*)&x[0])->single_line_text;
			name[30] = '\0'; 
			p = EX::need_unicode_equivalent(932,name);
		}
		else
		{
			p = w; w[0] = 30;
			w[SendDlgItemMessage(som_tool,1008,EM_GETLINE,0,(LPARAM)w)] = '\0';
		}
		SetDlgItemTextW(som_tool,1023,p);
	}
	else if(x_s>=31)
	{
		e.name[30] = '\0'; 
		p = EX::need_unicode_equivalent(932,e.name);
	}
	else 
	{	
		p = w; *w = '\0';
		GetDlgItemTextW(som_tool,1006,w,MAX_PATH);
	}
		   
	if(p!=workshop_savename) //Shift JIS->Unicode
	{
		//unfortunately, these depend on fonts :(
		for(wchar_t *q=(wchar_t*)p;*q;q++) switch(*q)
		{
			//these work with the WordPress font
			//but aren't helping with Windows' 

			//Sword magic PlayStation buttons

			//square to LARGE SQUARE?
			//square is smeared with English font???
			//hoping for a fix :(
			case 0x25A1: *q = 0x2b1c; break;
			//medium is smeared also???
			//case 0x25A1: *q = 0x25FB; break;
			//not bad? drop shadowed square
			//case 0x25A1: *q = 0x274F; break;

			//circle to Ideographic Zero?
			//not closer to triangle with English font			
			//case 0x25cB: *q = 0x3007; break;
			//large-circle is HUGE
			//case 0x25cB: *q = 0x25EF; break;
		}
	}
	else assert(0);

	workshop_titles(p);	

	char *icard = workshop->icard; 
	//HACK: taking for granted langague/theme packages are self imposing
	//a 97B limit and are not Unicode text files (translation: this will
	//need more work in the future but is sufficient for the time being)
	if(x_s>31+97) icard = (char*)&x[x_s-97];
	workshop->xicard = EX::need_unicode_equivalent(932,icard);

	if(SOM::tool==EneEdit.exe) 
	if(x_s>=146+6*21) for(size_t i=0;i<6;i++)
	{
		HWND bt = GetDlgItem(som_tool,i<3?2010+i:2015+i-3);		
		if(1==GetWindowTextW(bt,w,4)&&w[0]>='1'&&w[0]<='3') 
		continue;
		e.descriptions[i][20] = '\0';
		SetWindowTextW(bt,EX::need_unicode_equivalent(932,e.descriptions[i]));			
	}
	else assert(!x_s);
}
static int workshop_page = 0;
static HWND workshop_page2 = 0;
extern INT_PTR CALLBACK workshop_page2_init(HWND hwndDlg, UINT uMsg, WPARAM w, LPARAM l)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
	{
		//HACK: see below Microsoft workaround
		extern windowsex_SUBCLASSPROC workshop_subclassproc;
		RemoveWindowSubclass(hwndDlg,workshop_subclassproc,0);	

		HWND ch = GetWindow(hwndDlg,GW_CHILD);
		HWND prev = GetWindow(GetWindow(som_tool,GW_CHILD),GW_HWNDLAST);
		HWND next = 0;
		workshop_page2 = ch; while(ch)
		{
			next = GetWindow(ch,GW_HWNDNEXT);

			//SetWindowPos doesn't cover this?
			//probably one would do, not sure which (they're not equivalent)
			SetParent(ch,som_tool); SetWindowLong(ch,GWL_HWNDPARENT,(LONG)som_tool);

			SetWindowPos(ch,prev,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_HIDEWINDOW);

			prev = ch; ch = next; 
		}
		//WINSANITY REMINDER
		//https://www.betaarchive.com/wiki/index.php/Microsoft_KB_Archive/104069
		//DestroyWindow(hwndDlg);
		SetWindowPos(hwndDlg,next,0,0,0,0,SWP_HIDEWINDOW); break;
	}   //...
	case WM_COMMAND: return SendMessage(som_tool,uMsg,w,l);
	}
	return 0;
}

static void workshop_32772();
extern BOOL WINAPI workshop_SetWindowTextA(HWND A, LPCSTR B) 
{		
	SOM_TOOL_DETOUR_THREADMAIN(SetWindowTextA,A,B)

	EXLOG_LEVEL(7) << "workshop_SetWindowTextA()\n";

	//disable putting PRF/PRT file name in the titlebar
	if(A==workshop_host)
	{			
		if(som_tool_initializing) //PrtsEdit?
		{
			//2022: only PrtsEdit really needs this to
			//open a the file dialog the first time as
			//it's using som_art_link2 as hack to tell
			//som_art_model to not follow the shortcut 
			extern wchar_t *som_art_path(); som_art_path();
		}

		if(*(DWORD*)B==*(DWORD*)SOMEX_(A)
		&&*(DWORD*)(B+4)==*(DWORD*)"\\.ws")		
		{	
			//WARNING using this to update Ene/NpcEdit MDL
			//ObjEdit's model dialog sets the main title??
			if(!*som_tool_text) //som_tool_InitDialog hack
			return 1;

			//also, remember the file location for later on		
			//NOTE: after this point som_tool_text may hold
			//other paths used by the dialog's buttons, etc
			wmemcpy(workshop_savename,som_tool_text,MAX_PATH);

			//let som_tool_text be available for other uses
			*som_tool_text = '\0'; 
		}
		else if(som_tool_initializing)
		{
			//REMINDER: ItemEdit spams the main window
			//title and Save menu option, like several
			//times while som_tool_initializing... not
			//sure if workshop_ReadFile_item causes it
			//or if it's normal, but it would be worth
			//fixing (somehow?) (405f70)
			static bool one_off = false; if(!one_off) //HACK!
			{
				one_off = true; //ItemEdit sets its name 4 times???
				
				//HACK: don't show window until the art is ready
				extern int som_art(const wchar_t*,HWND);
				extern int som_art_model(const char*, wchar_t[MAX_PATH]);
				if(!*som_tool_text)
				{
					wcscpy(som_tool_text,L".WS");
					if(!GetEnvironmentVariableW(som_tool_text,som_tool_text,MAX_PATH))
					{
						//TODO: I'm not sure where's the best place
						//to do this since I'm rusty on this system

						//HACK: EneEdit_4173DC is never initialized
						//when not opened to a profile because these
						//tools never supported creating new profiles
						if(SOM::tool>=EneEdit.exe) workshop_32772();
					}
					else if(FILE*f=_wfopen(som_tool_text,L"rb"))
					{
						char a[62]; if(fread(a,62,1,f)) //my/prof?
						{
							a[61] = '\0';
							sprintf(a,"%s\\model\\%s",workshop_directory(),a+31);
							int e = som_art_model(a,som_tool_text);
							if(e&x2mdl_h::_art&&~e&x2mdl_h::_lnk)
							som_art(som_tool_text,0);
						}						
						fclose(f); 
					}
					*som_tool_text = '\0';
				}
				else assert(0);

				//add some ambient light so SOM_PRM isn't dark/dull
				//2021: this is simplified so WM_MBUTTONDOWN can be
				//pressed to default to workshop_ambient
				PostMessage(som_tool,WM_MOUSEWHEEL,0,0);

				if(SOM::tool==ItemEdit.exe)
				windowsex_click(som_tool,1000);
				
				//REMOVE ME??
				//HACK: suddenly ItemEdit and ObjEdit never trigger 
				//workshop_subclassproc to show som_tool_taskbar, even
				//though the window somehow becomes visible
				//(NOTE: this was not always the case)			
				PostMessage(som_tool,WM_SHOWWINDOW,1,0);
			}
		}
		else assert(!*workshop_savename); //*workshop_savename = '\0';

		//NOTE: disabling doesn't work??? Don't know why
		//it's done when 32772 is pressed instead
		if(*workshop_savename)
		EnableMenuItem(workshop_menu,32773,0);		

		//TODO: want to put the translated name in title
		if(!*workshop_savename)
		workshop_titles(workshop_title?0:EX::need_unicode_equivalent(932,B));
		else workshop_xtitles();
		return TRUE;
	}
	else switch(GetDlgCtrlID(A))
	{
	case 1008: //HACK: keep workshop_ReadFile_part setting

		if(SOM::tool==PrtsEdit.exe) 
		return TRUE;
		break;
	}

	//return SOM::Tool.SetWindowTextA(A,B);
	return SetWindowTextW(A,EX::need_unicode_equivalent(932,B));	
}
extern const wchar_t *workshop_filter(const char *a, wchar_t w[64+8])
{
	int id = a==(void*)0x416068?40075:32771;
	if(int i=GetDlgItemText(som_tool,id,w,64))
	{
		w[++i] = '*';
		w[++i] = '.';
		w[++i] = id==40075?'b':'p';
		w[++i] = id==40075?'m':'r';
		w[++i] = id==40075?'p':'f';
		w[++i] = '\0';
		w[++i] = '\0';
		if(SOM::tool==PrtsEdit.exe)
		w[i-2] = 't';
	}
	else
	{
		int len00 = strlen(a); 
		len00+=strlen(a+len00+1)+2;
		assert('\0'==a[len00-1]);
		EX::Convert(932,&a,&len00,const_cast<const wchar_t**>(&w));
	}
	return w;
}
static HANDLE workshop_temporary(void *cp=0, DWORD wr=0)
{
	//HACK: repurposing this static global 
	HANDLE tmp = SOM::Tool.Mapcomp_MHM(0); 
	SetFilePointer(tmp,0,0,FILE_BEGIN);
	if(wr) WriteFile(tmp,cp,wr,&wr,0);
	SetEndOfFile(tmp);
	SetFilePointer(tmp,0,0,FILE_BEGIN); return tmp;
}  
static VOID CALLBACK workshop_32772_timer(HWND win, UINT, UINT_PTR id, DWORD)
{
	KillTimer(win,id); EnableMenuItem(workshop_menu,32773,MF_GRAYED);
}
static void workshop_32772()
{
	assert(!*som_tool_text);
	*som_tool_text = '\0';
	*workshop_savename = '\0';
	SetEnvironmentVariableW(L".WS",L"");
	som_tool_initializing++;
	SendMessage(som_tool,WM_COMMAND,32771,0);
	som_tool_initializing--;
	
	//HACK: moved this here after determining that ItemEdit can't
	//do this in CreateFile/SetWindowText but only appeared to be
	//able to because button 1000 seems to turn saving on and off
	//somehow. the button was not included in the original dialog
	//ObjEdit doesn't work :(
	//EnableMenuItem(workshop_menu,32773,MF_GRAYED);
	//Note: ObjEdit reads the file before SetTimer, so I/O is not 
	//at issue... still it's really weird
	SetTimer(som_tool,32772,250,workshop_32772_timer);
}
static void EneEdit_default_descriptions(workshop_cpp::enemy_prf &enemy)
{
	//not necessary, however, all of the original profiles
	//have these default strings filled in this way, and it
	//gives buttons a default for their form's on/off switch
	char kanji[4] = "\x92\xbc";
	for(int i=enemy.direct;i<3;i++)
	sprintf(enemy.descriptions[i],som_932_EneEdit_attack123,kanji,som_932_Numerals[1+i]);	
	kanji[0] = '\x8a', kanji[1] = '\xd4';
	for(int i=enemy.indirect;i<3;i++)
	sprintf(enemy.descriptions[3+i],som_932_EneEdit_attack123,kanji,som_932_Numerals[1+i]);		
}
static HANDLE workshop_CreateFile_32772()
{
	union
	{
		workshop_cpp::my_prf my;
		workshop_cpp::npc_prf npc;
		workshop_cpp::enemy_prf enemy;
		workshop_cpp::item_prf<> item;
		workshop_cpp::object_prf object;
	};	
	memset(&enemy,0x00,sizeof(enemy));

	size_t sz; switch(SOM::tool)
	{
	case PrtsEdit.exe: sz = 228; break; 
	case ItemEdit.exe: sz = 88;
		
		item.SND = -1; 
		item.up_angles[0] = 360*4; //default to kf2 style?
		break;

	case ObjEdit.exe: sz = 108; 		
		object.openingSND = -1;
		object.closingSND = -1;
		object.loopingSND = -1;
		object.flameSFX = -1; 
		object.trapSFX = -1; break;
	
	case EneEdit.exe: sz = sizeof(enemy);
		enemy.flameSFX = -1; 
		EneEdit_default_descriptions(enemy);		
		break;
	
	case NpcEdit.exe: sz = sizeof(npc);		
		npc.flameSFX = -1; break;

	default: assert(0); 
		
		return INVALID_HANDLE_VALUE;
	}

	return workshop_temporary(&enemy,sz);
}
static HANDLE SfxEdit_1000(HWND cb, int wr=0)
{	
	wchar_t w[MAX_PATH]; int fp; if(cb)
	{
		GetWindowText(cb,w,32); 
		if(*w=='-')
		{
			ComboBox_SetCurSel(cb,1+ComboBox_GetCurSel(cb));
			goto sep;
		}
		fp = 48*_wtoi(w);
		if(fp>=1024*48)
		{
			SetWindowText(cb,L"1023");
		sep:windowsex_notify(som_tool,1000,CBN_EDITCHANGE);
			return 0;
		}
		if(fp==GetWindowLong(cb,GWL_USERDATA))
		return 0;
		SetWindowLong(cb,GWL_USERDATA,fp);
	}	

	HANDLE h = INVALID_HANDLE_VALUE;
	if(EX::data("sfx\\Sfx.dat",w))
	h = CreateFileW(w,GENERIC_READ|wr,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
	if(cb)
	{
		workshop_cpp::sfx_record sfx;		
		DWORD rd;
		if(fp==SetFilePointer(h,fp,0,FILE_BEGIN)
		&&ReadFile(h,&sfx,48,&rd,0)&&rd==48)
		{
			SetDlgItemInt(som_tool,1040,sfx.model,0);			
			SetDlgItemInt(som_tool,1041,sfx.procedure,0);
			for(int i=0;i<6;i++)
			SetDlgItemInt(som_tool,1042+i,sfx.unknown6[i],0);
			for(int i=0;i<8;i++)
			{
				swprintf(w,L"%g",sfx.floats8[i]);			
				SetDlgItemText(som_tool,1048+i,w);
			}
			SetDlgItemInt(som_tool,1056,sfx.sound2,1);
			int p = sfx.pitch24; if(sfx.sound2>0) p-=24;
			SetDlgItemInt(som_tool,1057,p,1);
			SetDlgItemInt(som_tool,1058,sfx.extend1,0);
		}
		else MessageBeep(-1);

		CloseHandle(h);
	}	
	return h;	
}
static VOID CALLBACK SfxEdit_1000_timer(HWND win, UINT, UINT_PTR id, DWORD)
{
	KillTimer(win,id); SfxEdit_1000(win);
}
static HANDLE SfxEdit_CreateFile(DWORD A, DWORD D)
{		
	workshop_cpp::my_prf my;
	workshop_cpp::enemy_prf enemy;
	memset(&enemy,0x00,sizeof(enemy));
	DWORD rd = 0; //used below
	if(*som_tool_text)
	{
		assert(A&GENERIC_READ);
		HANDLE h = CreateFileW(som_tool_text,A,0,0,D,0,0);
		if(h==INVALID_HANDLE_VALUE) return h;
		ReadFile(h,&my,sizeof(my),&rd,0); 
		CloseHandle(h);
	}
	else 
	{
		memset(&my,0x00,sizeof(my));
		my.friendlySFX = GetDlgItemInt(som_tool,1000,0,0);
	}

	assert(0==my._unknown1);
	assert(0==my._unknown2);
	enemy.my_sfx() = my.friendlySFX;
	enemy.my_screeneffect() = my.onscreen;
	memcpy(enemy.name,my.name,sizeof(my.name));
	if(rd==40+97)
	memcpy(enemy.my_icard(),my.icard,sizeof(my.icard));

	//HACK: SfxEdit_CreateFile_TXR disables culling
	DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_CULLMODE,DX::D3DCULL_CCW);

	return workshop_temporary(&enemy,sizeof(enemy));
}
static DX::IDirectDrawSurface7* &workshop_texture()
{
	extern DX::IDirect3DTexture2 *som_hacks_edgetexture;
	return (DX::IDirectDrawSurface7*&)som_hacks_edgetexture;
}
static HANDLE SfxEdit_CreateFile_TXR(wchar_t *in, wchar_t *txr_reference)
{
	namespace som = SWORDOFMOONLIGHT;

	struct : swordofmoonlight_mdl_header_t, swordofmoonlight_mdl_primch_t 
	{
		DWORD parts;
		som::mdl::uvpart_t uvpart;		
		DWORD x07_prims;
		som::mdl::prim07_t uvprim;
		//004056A1 83 F9 FF             cmp         ecx,0FFFFFFFFh 
		DWORD ffffffff;
		//x2mdl places these after primitives
		//WARNING: without the extra TIM section
		//on the end, I had to add extra elements
		//to these (5 and 3) or the last vertex is
		//ignored, and lighting worked differently
		//for 1 2 or 3 normals
		som::mdl::vertex_t v[4]; //[5]
		som::mdl::normal_t n[1]; //[3]
		//REQUIRED? appears so
		som::tim::header_t tim;
	}mdl;
	memset(&mdl,0x00,sizeof(mdl));
	//HACK: this crashes occasionally if the MDL is a 0 sized file
	//00403AA4 8B 11                mov         edx,dword ptr [ecx]
	DWORD wr; if(!in) 
	{
		wr = sizeof(swordofmoonlight_mdl_header_t);
		goto blank;
	}
	else //change from MDL to TXR
	{
		wr = sizeof(mdl);

		//strncpy(EneEdit_4173DC->name,txr_reference,31);
		SetDlgItemText(som_tool,1005,txr_reference);

		//HACK: SfxEdit_CreateFile sets to D3DCULL_CCW (speculatively)
		DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_CULLMODE,DX::D3DCULL_NONE);
		
		if(workshop_texture()) 
		{
			workshop_texture()->Release();
			workshop_texture() = 0;
		}
		
		//DUPLICATE //SOM_MAP.cpp
		namespace txr = SWORDOFMOONLIGHT::txr;	
		txr::image_t img;
		txr::maptofile(img,in,'r',1);
		if(const txr::mipmap_t map=txr::mipmap(img,0))
		{	
			//TODO! as soon as this is needed elsewhere, this should go into
			//a standard SOM compatible texture creator!			
			DX::DDSURFACEDESC2 desc = { sizeof(desc),
			DDSD_WIDTH|DDSD_HEIGHT|DDSD_CAPS|DDSD_PIXELFORMAT/*|DDSD_CKSRCBLT*/,
			map.height,map.width };
			desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE|DDSCAPS_VIDEOMEMORY; 
			DX::DDPIXELFORMAT pf = {0,DDPF_RGB,0,32,0xff0000,0x00ff00,0x0000ff};
			desc.ddpfPixelFormat = pf;
			if(!DDRAW::DirectDraw7->CreateSurface(&desc,&workshop_texture(),0)
			&&!workshop_texture()->Lock(0,&desc,DDLOCK_WAIT|DDLOCK_WRITEONLY,0))
			{
				//primary color screen effect?
				int ff = GetDlgItemInt(som_tool,1001,0,0);
				if(ff<12&&IsDlgButtonChecked(som_tool,1002))
				{
					if(!(ff/=3)) ff = 2; else if(ff==2) ff = 0; //swap?
				}
				else ff = 3;

				const txr::palette_t *p = txr::palette(img);
				txr::palette_t *d; (void*&)d = desc.lpSurface; 
				for(int i=0;i<map.height;i++,(char*&)d+=desc.lPitch)
				{
					const txr::pixrow_t &row = map[i];					
					for(int j=0;j<map.width;j++)			
					{
						if(p) d[j] = p[row.indices[j]];					
						if(!p) d[j] = *(txr::palette_t*)&row.truecolor[j];
						d[j].bgr[ff] = 0xFF;
					}
				}
				workshop_texture()->Unlock(0);

				DX::DDCOLORKEY black = {}; //allow black knockout
				workshop_texture()->SetColorKey(DDCKEY_SRCBLT,&black);
			}
			else assert(0);
		} 
		txr::unmap(img);
	}

	/*JUMP TABLE OF IMPLEMENTED MODES?
	00405691 FF 14 85 A8 61 41 00 call        dword ptr [eax*4+4161A8h]
	//0x004161A8  004056c0 00405ae0 00405d80 00405dd0
	//0x004161B8  00405f70 00406210 00406260 00406470 (7)
	//0x004161C8  00406780 004067e0 004069f0 00406d00
	//0x004161D8  00000000 00406d50 00000000 00407220
	//0x004161E8  00000000 00407540 00000000 00407880
	*/
	//mdl.leadflags = 1; //1 tries to animate
	//00403BA1 8A 51 02             mov         dl,byte ptr [ecx+2]
	//mdl.hardanims = 0;
	//00403BA1 8A 51 02             mov         dl,byte ptr [ecx+2]
	//mdl.softanims = 0;
	//00403BB4 8A 51 03             mov         dl,byte ptr [ecx+3]
	//mdl.timblocks = 1; 
	//00403A95 8A 4A 04             mov         cl,byte ptr [edx+4]  
	//00403C4F 8A 48 04             mov         cl,byte ptr [eax+4]
	//00403C6A 8A 48 04             mov         cl,byte ptr [eax+4]
	//00403CB3 8A 4A 04             mov         cl,byte ptr [edx+4]
	mdl.primchans = 1;
	//MYSTERY SOLVED
	//turns out this is the number of "uvparts" blocks
	//00403B6D 8A 51 05             mov         dl,byte ptr [ecx+5]
	//00403A66 8A 51 05             mov         dl,byte ptr [ecx+5]
	mdl.uvsblocks = 1;
	DWORD *body = (DWORD*)&mdl.vertsbase;
	mdl.primchanwords = (DWORD*)&mdl.tim-body;
	mdl.vertsbase = (DWORD*)mdl.v-body;
	mdl.vertcount = 4;
	mdl.normsbase = (DWORD*)mdl.n-body;
	mdl.normcount = 1;
	mdl.primsbase = &mdl.x07_prims-body;
	mdl.primcount = 1;
	mdl.parts = 1;
	mdl.uvpart.u1 = 1;
	mdl.uvpart.v2 = 1;
	mdl.uvpart.u3 = 1;
	mdl.uvpart.v3 = 1;	
	mdl.x07_prims = 0x10007;	
	mdl.uvpart.tsb = 1<<5; //blending mode 0? (ABR)
	mdl.uvprim.modeflags = 1<<9; //blending enable? (ABE)
	//triangles from 00406470 are (0,1,2) and (2,1,3)
	mdl.uvprim.pos0 = 0;
	mdl.uvprim.pos1 = 1;
	mdl.uvprim.pos2 = 3;
	mdl.uvprim.pos3 = 2;
	//004056A1 83 F9 FF             cmp         ecx,0FFFFFFFFh 
	mdl.ffffffff = 0xFFFFFFFF;
	//if TIM is not present v[3] and n[0] are ignored... there
	//must be some really weird coding inside of EneEdit.exe??
	mdl.v[0].x = mdl.v[0].y = mdl.v[1].y = mdl.v[3].x = -1000;
	mdl.v[1].x = mdl.v[2].x = mdl.v[2].y = mdl.v[3].y = +1000;
	mdl.n[0].z = -4096;
	mdl.tim.sixteen = 16;

	blank: return workshop_temporary(&mdl,wr);
}
extern HANDLE WINAPI workshop_CreateFileA(LPCSTR in, DWORD A, DWORD B, LPSECURITY_ATTRIBUTES C, DWORD D, DWORD E, HANDLE F)
{		
	SOM_TOOL_DETOUR_THREADMAIN(CreateFileA,in,A,B,C,D,E,F)

	EXLOG_LEVEL(7) << "workshop_CreateFileA()\n";

	//NOTE: once A:\>\.ws was sent with A a 0-terminator???
	//00418829 80 24 38 00          and         byte ptr [eax+edi],0
	assert(in&&*in);
	if(/*in&&*/*in=='\\'&&!strncmp(in,"\\\\?\\",4)) //device?
	{
		return SOM::Tool.CreateFileA(in,A,B,C,D,E,F);
	}
	SOM_TOOL_DETOUR_THREADMAIN(CreateFileA,in,A,B,C,D,E,F)

	EXLOG_LEVEL(7) << "workshop_CreateFileA()\n";

	//note, dxgi.dll (opengl?) opens ItemEdit.exe, etc.
	//assert(!workshop_reading);
	if(*(DWORD*)in!=*(DWORD*)SOMEX_(A)) a:
	return SOM::Tool.CreateFileA(in,A,B,C,D,E,F);	
	assert(!workshop_reading);
	wchar_t w[MAX_PATH]; 
	HANDLE out = INVALID_HANDLE_VALUE; 
	if(*(DWORD*)(in+4)!=*(DWORD*)"\\.ws") //model/texture?
	{	
		assert(~A&GENERIC_WRITE);

		const char *ext = PathFindExtensionA(in);

		extern wchar_t *som_art_CreateFile_w; //2021
		extern wchar_t *som_art_CreateFile_w_cat(wchar_t[],const char*);
		if(som_art_CreateFile_w)
		if(som_art_CreateFile_w_cat(w,in)) goto art;

		if(!*ext) 
		{
			if(SOM::tool==PrtsEdit.exe) //"A:\>\data\map\parts\"
			{
				assert(strstr(in,"\\parts\\"));
				goto PrtsEdit_60100;
			}

			//HACK: model field is empty?
			if(ext[-1]=='\\') quiet:
			{	
				//HACK: crashing here?
				//00403AA4 8B 11                mov         edx,dword ptr [ecx]
				//return workshop_temporary();				
				return SfxEdit_CreateFile_TXR(0,0);
			}
			assert(!strcmp(ext-3,"\\cp"));
			return INVALID_HANDLE_VALUE;
		}

		if(SOM::tool==SfxEdit.exe)
		{
			//NOTE: workshop_reprogram can overwrite these strings, and must
			//for any that impact som.art.cpp
			if(memcmp((char*)in+10,"sfx",3))
			{
				memcpy(memmove((char*)in+10,in+12,strlen(in+12)+1),"sfx",3);
				ext-=2;
			}
		}

		//Reminder: som_tool_text won't work here because
		//it needs to be remembered for when the title is
		//changed, which happens after models are read in		
		if(EX::data(in+10,w)) art:
		out = CreateFileW(w,A,B,C,D,E,F);			
		
		if(out==INVALID_HANDLE_VALUE)
		{
			//note: ObjEdit is interested in CPs
			if(stricmp(ext,".cp"))
			{
				if(SOM::tool==SfxEdit.exe)
				{
					if(!strcmp(ext,".mdl"))
					{
						memcpy((char*)ext+1,"txr",4);
						int len;
						if(som_art_CreateFile_w) //som_MDL_440030?
						{
							len = wcslen(w); wmemcpy(w+len-3,L"txr",4);
						}
						else len = EX::data(in+10,w);
						if(len)
						return SfxEdit_CreateFile_TXR(w,w+len-4-(ext-(in+20)));
					}
				}
				//this is for manual entry, to be
				//quiet on partial input/failures
				//assert(out!=INVALID_HANDLE_VALUE);				
				if(!stricmp(ext,".mdo"))
				{
					/*ItemEdit crashes
					00401BAC 8B 44 24 1C          mov         eax,dword ptr [esp+1Ch]  
					00401BB0 8B 48 08             mov         ecx,dword ptr [eax+8]  
					00401BB3 66 8B 14 91          mov         dx,word ptr [ecx+edx*4] 
					*/
					//not really a problem since ItemEdit/ObjEdit don't complain
					//clearing AA afterimage (ItemEdit)
					PostMessage(som_tool,WM_APP+0,0,0);
				}
				else goto quiet;
			}
		}		
		else //2021: new colorkey standard?
		{
			extern int Ex_mipmap_colorkey;
			//UNTESTED: assuming TIM textures are loaded
			//immediately after, etc. and skin TXR files
			//will switch to 0
			if(EX::INI::Bugfix()->do_fix_colorkey)
			{
				Ex_mipmap_colorkey = !stricmp(ext,".mdl")?7:0;
			}
			else Ex_mipmap_colorkey = 7;
		}
		
		return out;
	}	
	PrtsEdit_60100: if(!*som_tool_text) 	
	{
		assert(~A&GENERIC_WRITE);
		wcscpy(som_tool_text,L".WS");
		if(!GetEnvironmentVariableW(som_tool_text,som_tool_text,MAX_PATH))
		som_tool_text[0] = '\0'; 
		else assert(!PathIsRelative(som_tool_text));
	}
	else if('*'==*som_tool_text) //HACK: reset MDL 
	{
		som_tool_text[0] = '\0'; 
		
		//assuming EneEdit doesn't care about SFX/SND data
		void *p; size_t sz;
		if(SOM::tool==NpcEdit.exe)
		{
			p = NpcEdit_41761C; sz = sizeof(*NpcEdit_41761C);			
		}
		else
		{
			p = EneEdit_4173DC; sz = sizeof(*EneEdit_4173DC);			
		}		
		GetDlgItemText(som_tool,1005,w,31);
		EX::need_ansi_equivalent(932,w,(char*)p+31,31);
		return workshop_temporary(p,sz);
	}
			
	int gray = 0; if(~A&GENERIC_WRITE)
	{
		//TODO: disable Save for public profiles
		//Bug fix.... ItemEdit/ObjEdit NEVER enable the Save 
		//option. Whereas EneEdit & NpcEdit do (but may not disable it)		
		//NOTE: not messing with original EneEdit/NpcEdit code 40072
		if(gray=*som_tool_text?0:MF_GRAYED)
		workshop_titles();
		
		//REMINDER: it seems that button 1000 may have been a feature
		//to enable/disable Save via the menu. without it, this isn't
		//working here. Note this applies to ItemEdit, maybe ObjEdit?
		//EnableMenuItem(workshop_menu,32773,gray);		
	}

	if(SOM::tool!=SfxEdit.exe||A&GENERIC_WRITE)		
	if(!gray) out = CreateFileW(som_tool_text,A,B,C,D,E,F);
	else return workshop_reading = workshop_CreateFile_32772();	
	else out = SfxEdit_CreateFile(A,D);

	if(A&GENERIC_WRITE)
	{
		assert(~A&GENERIC_READ); //40075?
		*som_tool_text = '\0'; //paranoia
	}
	else workshop_reading = out; return out;
}
static bool workshop_ReadFile_part(workshop_cpp::part_prt &part, size_t part_s)
{
	if(part_s<228) return false; assert(sizeof(part)==228);

	assert(!part._unused1&&!part._unused2&&!part._unused3/*&&!part._unused4*/);

	//HACK: coax icons to updating themself
	PostMessage(GetDlgItem(som_tool,1013),EM_SETMODIFY,1,0);
	PostMessage(som_tool,WM_COMMAND,IDOK,0);

	memcpy(workshop->icard,part.text_and_history,96);

	wchar_t w[31+2+97];
	//2022: this isn't terminated (length is 21, not 30)
	//I can't tell what I was thinking here, but it had appeared to
	//work until today (1000s of times) 
	//in today's case the input was 30 followed by the 0-terminator
	//int i; MultiByteToWideChar(932,0,part.single_line_text,30,w,30);
	//i = wcslen(w); //if(i) i--;
	int i = MultiByteToWideChar(932,0,part.single_line_text,30,w,30);	
	if(part.text_and_history[0])
	{
		w[i++] = '\n';
		//2022: why -1?
	//	i+=MultiByteToWideChar(932,0,part.text_and_history,96,w+i,96)-1;
		i+=MultiByteToWideChar(932,0,part.text_and_history,96,w+i,96);
	}
	w[i] = '\0'; 
	
	HWND hw = GetDlgItem(som_tool,1008);
	SetWindowText(hw,w);
	Edit_SetSel(hw,0,Edit_LineLength(hw,0));
	//SetFocus(hw); //'stnf'
	
	//HACK: emulate the other tools
	workshop_SetWindowTextA(som_tool,SOMEX_(A)"\\.ws"); return true;
}
static void workshop_click_radio(int a, int z, int id)
{
	if(z==2000+0x1F) //HACK: ObjEdit
	{
		//2022: BM_CLICK below was showing None (1007)
		//checked, but it could be a graphical glitch
		//since it only happens in the maximized mode
		CheckRadioButton(som_tool,2000,z,id);
		CheckRadioButton(som_tool,1007,1009,id);
		return;
	}

	//HACK: SendMessage(BM_CLICK) checks both radios when
	//maximized???
	/*2021: I think it happens when checkboxes are disabled
	//(it's happening with ItemEdit File->New)
	if(workshop_mode==0)*/
	if(!IsWindowEnabled(GetDlgItem(som_tool,id)))
	{	
		if(z==2000+0x1F) //HACK: ObjEdit
		{
			//there is a bug in CheckRadioButton that checks
			//non-radio buttons
			assert(SOM::tool==ObjEdit.exe);
			if(id<2000)
			{
				CheckRadioButton(som_tool,2000,z,id);
				z = 1009;				
			}
		}
		CheckRadioButton(som_tool,a,z,id);
	}
	
	SendDlgItemMessage(som_tool,id,BM_CLICK,0,0);
}
static WCHAR *workshop_default(WCHAR *w, int i, int def=0)
{
	return i!=def?_itow(i,w,10):&(*w='\0');
}
static VOID CALLBACK workshop_click_radio_timer(HWND win, UINT, UINT_PTR id, DWORD)
{
	KillTimer(win,id); workshop_click_radio(LOWORD(id),HIWORD(id),GetDlgCtrlID(win));
}
static bool workshop_ReadFile_item(workshop_cpp::item_prf<> &item, size_t item_s)
{
	//TODO: prf::maptoram
	if(item_s!=88&item_s!=88+97) return false;

	if(item_s==88+97) memcpy(workshop->icard,item.icard,96);
	
	//assert(0==item.my);

	namespace prf = SWORDOFMOONLIGHT::prf;
	prf::moveset_t &item2 = item.get<prf::moveset_t>();

	//ATTENTION
	//this is a union that is valid only if equip is 1
	//if 2 it can be an animation ID to use instead of 
	//item2.position
	if(1!=item.equip) item.my = 0;

	//2021: this is a union
	unsigned movement2 = item.movement2; //2021
	if(item.my)
	item.up_angles[0] = item.up_angles[1] = 0;

	int i = item.up_angles[0];
	int id = i/360; i%=360;
	item.up_angles[0] = i&0xffff;
	for(i=1;i<=4;i<<=1)
	CheckMenuItem(workshop_menu,34000+i,id&i?MF_CHECKED:0);
	item.up_angles[1]%=360; 		
	EnableMenuItem(workshop_menu,34999,MF_GRAYED); //???

	id = item.my==5?1008:1009; 
	if(!item.my) switch(item.equip)
	{
	case 0: id = 1010; break;
	case 1: id = 1007; break;
	case 3: if(5!=item2.position) //gun
			id = 1007;			  //break;
	case 2:	if(5==item2.position) //bow/shield
			id = 1008; 
	}
	workshop_click_radio(1007,1010,id);

	id = -1; 
	if(item.equip==2) id = item2.position;
	if(item.my) id = 1==item.my?0:item.my;
	switch(id)
	{
	case 0: id = 1020; break;
	case 1: id = 1021; break;
	case 2: id = 1022; break;
	case 3: id = 1024; break;
	case 4: id = 1025; break;
	//case 5: id = 1023; break; //shield?
	case 6: id = 1026; break;
	default: id = -1; break;
	}
	CheckRadioButton(som_tool,1020,1026,id);

	if(item.my)
	{		 
		id = 1031; assert(1==item.equip);
	}
	else switch(item.equip) 
	{
	default: id = 1030; break; //2021: File->New?
	case 1:	id = item2.SND_pitch[0]?1032:1030; break;
	case 2: id = *(DWORD*)item2.SND_pitch?1032:1030; break;
	case 3: id = 1033; break;
	}	
	workshop_click_radio(1030,1033,id);
	bool movement = id<=1031&&item.equip==1;

	WCHAR w[16];
	BYTE rcpt = item.equip?0:item.get<prf::item2_t>().receptacle;
	HWND hw = GetDlgItem(som_tool,1019);
	SetWindowText(hw,workshop_default(w,rcpt));

	for(id=1040;id<=1042;id++)
	{
		bool move = item.my&&id==1042; //2021

		hw = GetDlgItem(som_tool,id);
		HWND ch = GetWindow(hw,GW_CHILD);
		for(int j,i=3;ch;ch=GetWindow(ch,GW_HWNDNEXT),i--) 
		{
			*w = '\0';
			if(item.equip&&(i==0||!movement||move))
			{
				//HACK: reserving first two non-weapon
				//moves, since the first is a fixed ID
				if(1!=item.equip&&!move)
				{
					//2 is enough for now
					//j = (i+2)%4; 
					j = i+2; if(j>3) goto limit_of_2;
				}
				else j = i;				
				if(item2.SND_pitch[j]||movement)
				switch(id)
				{
				case 1040:		

					if(!movement) 
					j = item2.SND[j];
					else j = item.SND;					
					if(j>=0) goto j; break;

				case 1041: 

					if(!movement)				
					j = item2.SND_pitch[j];
					else 
					j = item.SND_pitch;
					if(j<-24||j>20)
					{
						//2021: this is hack to let 0 be set
						//in movesets to override the move's
						//setting
						if(j==-128)
						{
							j = 0; goto j;
						}

						j = 0;
					}
					if(j) goto j; break;
				
				case 1042: 
				
					if(move) 
					{
						switch(i) //2021
						{
						case 3: j = movement2>>20&0x3FF; break;
						case 2: j = movement2>>10&0x3FF; break;
						case 1: j = movement2&0x3FF; break;
						case 0: j = item2.move[0]|movement2>>30<<8; break;
						}
						if(j<=999) goto j;
					}
					else
					{
						j = item2.move[j]; 
						if(j!=0xff) j:_itow(j,w,10); 
					}
					break;
				}	
			}			
			limit_of_2: SetWindowText(ch,w);
		}
	}

	if(movement)
	{
		SetDlgItemInt(som_tool,1050,item.swordmagic_window[0],0);
		SetDlgItemInt(som_tool,1051,item.swordmagic_window[1],0);
		SetDlgItemInt(som_tool,1052,item.hit_window[0],0);
		SetDlgItemInt(som_tool,1053,item.hit_window[1],0);		
		swprintf_s(w,L"%g",item.hit_radius); 
		SetDlgItemText(som_tool,1054,w);
		SetDlgItemInt(som_tool,1055,item.hit_pie,0);
		SetDlgItemInt(som_tool,1056,item.SND_delay,0);
	}
	else for(id=1050,*w='\0';id<=1056;id++) 
	{
		SetDlgItemText(som_tool,id,w);
	}

	return true;	
}
extern HWND som_tool_neighbor_focusing; //HACK
static bool workshop_ReadFile_obj(workshop_cpp::object_prf &obj, size_t obj_s)
{
	//TODO: prf::maptoram
	if(obj_s!=108&obj_s!=108+97) return false;

	if(obj_s==108+97) memcpy(workshop->icard,obj.icard,96);

	//1 for chest/switch/door/trap...
	//setting to 0 doesn't appear to change things
	//assert((obj.operation==0x15)==obj.interactive); 
	//assert(0==obj._unused1); //1.0 for 0000/0001.prf
	assert(0==obj._unknown3);

	//Reminder: ObjEdit fails to set these if 1001
	//comes before 1000 in window order (weird???)
	int id; switch(obj.clipmodel)
	{
	case 0:	id = 1000; break;
	case 1: id = 1001; break;
	default: id = 999; assert(2==obj.clipmodel);
	}
	//PostMessage(GetDlgItem(som_tool,id),BM_CLICK,0,0);
	SetTimer(GetDlgItem(som_tool,id),MAKELONG(999,1001),0,workshop_click_radio_timer);
	//display 1 dimension for disc shaped objects?
	//HACK: NOT WORKING? passing buck to SetTimer(som_tool,'stnf',0,0)
	//if(id==1000) 
	//PostMessage(GetDlgItem(som_tool,1004),WM_SETTEXT,0,(LPARAM)L"");

	id = 1007; switch(obj.operation)
	{	
	case 0: id = 1007; break; //none
	case 0x14: id = 1008; break; //box
	case 0x16: id = 1009; break; //corpse

	default: assert(!obj.operation);	

	case 0x15: //chest
	case 0x0B: //door (secret door perhaps)
	//0040ef0b 66 83 ff 0c     cmp        di,0xc
    //0040ef0f 74 10           jz         lab_0040ef21 (Ghidra)
    //0040ef11 66 83 ff 0e     cmp        di,0xe
    //0040ef15 74 0a           jz         lab_0040ef21 (Ghidra)
		//NOTE: I think this is like elevator doors, but with 
		//changes I've made to SomEx it should be equivalent
		//to D. B is still interesting becuse the door holding
		//logic is different. C could have different logic but
		//a double vertical door doesn't seem worth the effort
	//case 0x0C //door (double?) //UNUSED?
	case 0x0D: //door (swinging door)
	case 0x0E: //door (double?)
	case 0x0A: //lamp
	case 0x29: //receptacle
	case 0x28: //switch
	case 0x1E: //trap (spear variety perhaps)
	case 0x1F: //trap (arrow variety perhaps)	

		id = 2000+obj.operation; 
	}
	//REMINDER: MAY BE SOURCE OF FUTURE BUGS
	//REMINDER: MAY BE SOURCE OF FUTURE BUGS
	//REMINDER: MAY BE SOURCE OF FUTURE BUGS
	//PostMessage(GetDlgItem(som_tool,id),BM_CLICK,0,0);
	//this sets unrelated checkboxes (not radio buttons)
	//it's easier to let the callback is handle the glitch
	SetTimer(GetDlgItem(som_tool,id),MAKELONG(1007,2000+0x1F),0,workshop_click_radio_timer);
	
	WCHAR w[16];
	BYTE rcpt = id!=2041?0:obj.receptacle;	
	SetDlgItemText(som_tool,1019,workshop_default(w,rcpt));

	SetDlgItemText(som_tool,1030,workshop_default(w,obj.trapSFX,-1));
	CheckDlgButton(som_tool,1031,obj.trapSFX_orientate);
	//switching the sense of this around to have the obj.invisible wording
	CheckDlgButton(som_tool,1032,id!=2031?0:!obj.trapSFX_visible);

	SetDlgItemText(som_tool,1040,workshop_default(w,obj.flameSFX,-1));
	SetDlgItemText(som_tool,1041,workshop_default(w,obj.flameSFX_periodicty));

	CheckDlgButton(som_tool,1050,obj.loopable);
	CheckDlgButton(som_tool,1051,obj.sixDOF);
	CheckDlgButton(som_tool,1052,obj.invisible);

	SetDlgItemText(som_tool,1060,workshop_default(w,obj.openingSND,-1));
	SetDlgItemText(som_tool,1061,workshop_default(w,obj.openingSND_pitch));
	SetDlgItemText(som_tool,1062,workshop_default(w,obj.openingSND_delay));
	SetDlgItemText(som_tool,1063,workshop_default(w,obj.closingSND,-1));
	SetDlgItemText(som_tool,1064,workshop_default(w,obj.closingSND_pitch));
	SetDlgItemText(som_tool,1065,workshop_default(w,obj.closingSND_delay));
	SetDlgItemText(som_tool,1066,workshop_default(w,obj.loopingSND,-1));
	SetDlgItemText(som_tool,1067,workshop_default(w,obj.loopingSND_pitch));
	SetDlgItemText(som_tool,1068,workshop_default(w,obj.loopingSND_delay));
	 
	CheckDlgButton(som_tool,1070,obj.billboard);
	windowsex_enable(som_tool,1081,1082,0!=obj.animateUV);
	CheckDlgButton(som_tool,1080,0!=obj.animateUV);
	CheckRadioButton(som_tool,1081,1082,1080+obj.animateUV);
	
	return true;
}

static void workshop_ReadFile_init(HWND cb, bool hits)
{
	HWND ec = GetDlgItem(cb,1001);
	SetWindowStyle(ec,ES_READONLY|ES_RIGHT);
	Edit_SetReadOnly(ec,1);			
	int w = 30; if(hits) w+=2;
	extern char som_tool_space;
	SendMessage(cb,CB_SETDROPPEDWIDTH,
	w*som_tool_space
	+3*GetSystemMetrics(SM_CXEDGE)
	//TODO? detect the scroll bar
	+GetSystemMetrics(SM_CXHSCROLL),0);
}
static bool workshop_ReadFile_data(bool init, HWND cb, 
std::vector<workshop_cpp::data> &data, void *b0, int bs, WORD *sfx, WORD *snd)
{
	bool hits = !data.empty(); bs-=4;
		
	if(!init) ComboBox_ResetContent(cb);
	else workshop_ReadFile_init(cb,hits);
	
	typedef workshop_cpp::prf::data_t D;

	int jN = sfx[0], s = sfx[1];
	if(s+jN*4>=bs){ assert(0); return false; }			
	D *d = (D*)((char*)b0+s);
	for(int j=0;j<jN;j++,d++)
	{
		assert(d->time>0&&d->effect>0);

		workshop_cpp::data cp = {};
		cp.time = d->time; 
		cp.sfx = d->effect;
		cp.cp = d->CP; data.push_back(cp);
	}
	jN = snd[0]; s = snd[1];
	if(s+jN*4>=bs){ assert(0); return false; }
	d = (D*)((char*)b0+s);
	for(int j=0;j<jN;j++,d++)
	{
		assert(d->time>0&&d->effect>0);

		workshop_cpp::data cp = {};
		cp.time = d->time; 
		cp.snd = d->effect;
		cp.pitch = d->pitch; data.push_back(cp);
	}

	//looking for original files with t==0
	assert(data.empty()||data[0].time!=0);

	std::sort(data.begin(),data.end());		
	wchar_t w[64];
	for(int j=0,jN=data.size();j<jN;)
	{
		workshop_cpp::data d = data[j++];
		while(j<jN&&d.time==data[j].time)
		{
			if(d+=data[j]) j++; else break;
		}
		
		//swprintf(w,L"|%02d|   %s%4d@%-3d  %4d%-+3d  |%02d|",d.time,hits?x:x+2,d.sfx,d.cp,d.snd,d.pitch,d.time);
		if(hits&&!d.hit) 
		d.hit = ' ';
		d.print(w); 
		ComboBox_AddString(cb,w);
	}		

	return true;
}
static bool workshop_ReadFile_enemy(workshop_cpp::enemy_prf &enemy, int enemy_s)
{
	if(enemy_s<sizeof(enemy)||enemy_s>65535) return false;

	if(bool icard=enemy_s-97>=sizeof(enemy)) //HACK?
	{
		WORD m = (WORD)enemy_s-97;
		for(WORD*p=*enemy.dataSFX,*d=p+32*2*2;p<d;p+=2)
		{
			if(p[0]!=0&&p[1]+p[0]*4>m) icard = false;
		}
		if(icard) memcpy(workshop->icard,(char*)&enemy+m,96);
	}

	/*2020: these are all 0 in the original files
	auto test1 = enemy._unknown1; assert(!test1);
	auto test2 = enemy._unknown2; assert(!test2);
	auto test3 = enemy._unknown3; assert(!test3);
	auto test4 = enemy._unknown4; assert(!test4);
	auto test5 = enemy._unknown5; assert(!test5);
	auto test6 = enemy._unknown6; assert(!test6);
	if(enemy.flying_related[0])
	{
		int bp = 0; //breakpoint
	}*/

	workshop_skin(enemy.skin?enemy.skinTXR:0);

	SetDlgItemText(som_tool,1005,EX::need_unicode_equivalent(932,enemy.model));
	SetDlgItemText(som_tool,1006,EX::need_unicode_equivalent(932,enemy.name));

	WCHAR w[32];
	swprintf_s(w,L"%g",enemy.height); SetDlgItemText(som_tool,1002,w);	
	swprintf_s(w,L"%g",enemy.shadow/2); SetDlgItemText(som_tool,1003,w);
	swprintf_s(w,L"%g",enemy.diameter/2); SetDlgItemText(som_tool,1004,w);

	//TODO: need to extract translations 
	//from language package
	w[0] = '1'; w[1] = '\0'; 
	for(int i=0;i<3;i++,w[0]++)
	{
		char *p = enemy.descriptions[i];
		SetDlgItemText(som_tool,2010+i,
		!*p||i>=enemy.direct?w:EX::need_unicode_equivalent(932,p));
	}w[0] = '1';
	for(int i=0;i<3;i++,w[0]++)
	{
		char *p = enemy.descriptions[3+i];
		SetDlgItemText(som_tool,2015+i,
		!*p||i>=enemy.indirect?w:EX::need_unicode_equivalent(932,p));
	}
	
	for(int i=0;i<3;i++)
	{
		//2021: I got these wrong and am reversing them
		//so all legacy monsters will swing before they
		//can hit you (more dramatic)
		swprintf_s(w,L"%g",enemy.radii[i]);
		SetDlgItemText(som_tool,1032+i,w); //1029
		swprintf_s(w,L"%g",enemy.larger_radii[i]);
		SetDlgItemText(som_tool,1029+i,w); //1032
		SetDlgItemInt(som_tool,1035+i,enemy.pies[i],0);
	}

	int id; 
		
	switch(enemy.locomotion)
	{
	default: assert(0); 
	case 5: id = 1010; break; //none?
	case 2: id = 1008; break; //fly?
	case 1: id = 1009; break; //turn?
	case 0: id = 1007; break; //walk?
	}
	CheckRadioButton(som_tool,1007,1010,id);

	//2020: 2 means no shadow... the rest aren't
	//meaningful to som_db but to SOM_PRM
	//(page 2 has facilities too set these manually)
	/*PROBING
	//1: SOM_PRM unrelated (various yes/no) (yes/no?)
	//2: no shadow (pixie/imp/jelly/face)
	//4: evade (SOM_PRM excludes 8+8) (4|8?)
	//8: defend (ditto)
	_itow(enemy.countermeasures,w,2);
	SetDlgItemText(som_tool,1002,w);
	*/

	assert(enemy.activation<=1);
	CheckDlgButton(som_tool,2009,enemy.activation);	
	CheckDlgButton(som_tool,1015,-1!=enemy.flameSFX);	
		
	int mask = ~workshop_cpp::datamask;
	std::vector<workshop_cpp::data> data;
	for(int i=0;i<32;i++,mask>>=1)
	{			
		data.clear();
		int hits = 0; if(i>=10&&i<=12) 
		{	
			for(int j=0;j<3;j++)
			if(enemy.hit_delay[i-10][j])
			{
				hits++;
				workshop_cpp::data cp = {};
				cp.time = enemy.hit_delay[i-10][j]; 
				cp.hit = '1'+j; 
				data.push_back(cp);
			}
			w[0] = '0'+hits; w[1] = '\0';
			SetDlgItemText(som_tool,1020+i-10,w);
		}
		if(0==enemy.dataSFX[i][0]+enemy.dataSND[i][0])
		{
			if(mask&1&&!hits) continue;
		}
		workshop_cpp::datamask|=1<<i;
		
		HWND cb = GetDlgItem(som_tool,4000+i); 
		if(!cb&&!workshop_page) //2020
		{
			SendMessage(som_tool,WM_COMMAND,30002,0);
			if(workshop_page2)
			{
				cb = GetDlgItem(som_tool,4000+i);
				assert(cb);
			}
		}
		if(!cb) //LOSSLESS: nonstandard? (OBSOLETE)
		{
			assert(cb); //Mantrap #2 has 1 sound? Viper too
			extern ATOM som_tool_combobox(HWND=0);
			cb = CreateWindow((WCHAR*)som_tool_combobox(),0,WS_CHILD|CBS_DROPDOWN,0,0,0,0,som_tool,HMENU(4000+i),0,0);
		}

		if(!workshop_ReadFile_data(mask&1,cb,data,&enemy,enemy_s,enemy.dataSFX[i],enemy.dataSND[i]))
		return false;

		int def = 0; switch(i)
		{
		case 1: if(enemy.locomotion<=2) def = -1; break;
		case 6:	if(8&enemy.countermeasures) def = -1; break;
		case 7: if(4&enemy.countermeasures) def = -1; break;
		case 9: if(enemy.activation) def = -1; break;
		}
		ComboBox_InsertString(cb,0,workshop_default(w,ComboBox_GetCount(cb),def));
		ComboBox_SetCurSel(cb,0);
	}	

	if(workshop_page2)
	{
		//initialize extension when 0?
		float t = enemy.turning_ratio; if(!t)
		{
			enemy.turning_ratio = t = 1; //version marker

			//40: defend? 
			//7<<15: indirect attacks?
			//note: classically this should be 1
			enemy.turning_table = 0xe|0x40|7<<15|3<<23|3<<30;
			//emulate classic behavior		
			enemy.inclination[1] = 90;
			//for animations #23 & #24
			memset(enemy.turning_cycle,0x00,4);
		}
		//convert to real speed?
		t = !_finite(t)?0:180/t;

		swprintf_s(w,L"%g",t);
		SetDlgItemText(som_tool,1040,w);
		SetDlgItemInt(som_tool,1041,enemy.inclination[0],0);
		SetDlgItemInt(som_tool,1042,enemy.inclination[1],0);
		swprintf_s(w,L"%g",enemy.flight_envelope);
		SetDlgItemText(som_tool,1043,w);

		SetDlgItemInt(som_tool,1044,enemy.defend_window[0],0);	
		SetDlgItemInt(som_tool,1045,enemy.defend_window[1],0);	

		SetDlgItemInt(som_tool,1046,enemy.turning_cycle[0][0],0);
		SetDlgItemInt(som_tool,1047,enemy.turning_cycle[0][1],0);
		SetDlgItemInt(som_tool,1048,enemy.turning_cycle[1][0],0);
		SetDlgItemInt(som_tool,1049,enemy.turning_cycle[1][1],0);

		int mask = ~enemy.turning_table; //EXTENSION
		for(int i=0;i<32;i++)	
		CheckDlgButton(som_tool,5000+i,mask&1<<i?1:0);	

		//I don't know what bit 1 is and there's no 
		//other way to set bit 2 (shadow)
		//1:? 2/4/8:shadow/evade/defend
		//(note: 2 is som_db's, rest are SOM_PRM's?)
		mask = ~enemy.countermeasures;
		for(int i=0;i<4;i++)	
		CheckDlgButton(som_tool,5991+i,mask&1<<i?1:0);	
	}
	return true;
}
static bool workshop_ReadFile_npc(workshop_cpp::npc_prf &npc, size_t npc_s)
{
	if(npc_s<sizeof(npc)||npc_s>65535) return false;

	if(bool icard=npc_s-97>=sizeof(npc)) //HACK?
	{
		WORD m = (WORD)npc_s-97;
		for(WORD*p=*npc.dataSFX,*d=p+32*2*2;p<d;p+=2)
		{
			if(p[1]+p[0]*4>m) icard = false;
		}
		if(icard) memcpy(workshop->icard,(char*)&npc+m,96);
	}

	workshop_skin(npc.skin?npc.skinTXR:0);

	assert(npc._unknown1==0x100
	||npc._unknown1==0); //dragon
	assert(npc._unknown2==0);
	assert(npc._unknown3==0);
	assert(npc._unknown4==0);

	SetDlgItemText(som_tool,1005,EX::need_unicode_equivalent(932,npc.model));
	SetDlgItemText(som_tool,1006,EX::need_unicode_equivalent(932,npc.name));

	//extension? 
	CheckDlgButton(som_tool,1007,1); //walking NPC

	WCHAR w[32];
	swprintf_s(w,L"%g",npc.height); SetDlgItemText(som_tool,1002,w);
	swprintf_s(w,L"%g",npc.shadow/2); SetDlgItemText(som_tool,1003,w);
	swprintf_s(w,L"%g",npc.diameter/2);	SetDlgItemText(som_tool,1004,w);
			 
	CheckDlgButton(som_tool,1015,-1!=npc.flameSFX);	

	int mask = ~workshop_cpp::datamask;
	std::vector<workshop_cpp::data> data;
	for(int i=0;i<32;i++,mask>>=1)
	{			
		data.clear();

		if(!npc.dataSFX[i][0]
		 &&!npc.dataSND[i][0])
		{
			if(mask&1) continue;
		}
		workshop_cpp::datamask|=1<<i;
		
		HWND cb = GetDlgItem(som_tool,4000+i);
		if(!cb&&!workshop_page) //2020
		{
			SendMessage(som_tool,WM_COMMAND,30002,0);
			if(workshop_page2)
			{
				cb = GetDlgItem(som_tool,4000+i);
				assert(cb);
			}
		}
		if(!cb) //LOSSLESS: nonstandard?
		{
			assert(cb);
			extern ATOM som_tool_combobox(HWND=0);
			cb = CreateWindow((WCHAR*)som_tool_combobox(),0,WS_CHILD|CBS_DROPDOWN,0,0,0,0,som_tool,HMENU(4000+i),0,0);
		}

		if(!workshop_ReadFile_data(mask&1,cb,data,&npc,npc_s,npc.dataSFX[i],npc.dataSND[i]))
		return false;

		ComboBox_InsertString(cb,0,workshop_default(w,ComboBox_GetCount(cb)));
		ComboBox_SetCurSel(cb,0);
	}

	if(HWND ch=GetDlgItem(som_tool,1040)) //workshop_page2?
	{
		float t = npc.turning_ratio;
		swprintf_s(w,L"%g",t?180/t:0); 
		SetDlgItemText(som_tool,1040,w);

		for(int i=0;i<4;i++)
		SetDlgItemInt(som_tool,1044+i,npc.title_frames[i],0); 
	}

	return true;
}
static bool workshop_ReadFile_my(workshop_cpp::enemy_prf &enemy, int my_s)
{
	assert(my_s==sizeof(enemy));

	memcpy(workshop->icard,enemy.my_icard(),96);
	
	//HACK: this differs from SfxEdit_ReadFile in that
	//EneEdit_4173DC is nonzero								
	//workshop_cpp::enemy_prf *hack = EneEdit_4173DC; 
	//EneEdit_4173DC = &enemy;
	if(!EneEdit_4173DC) EneEdit_4173DC = &enemy;
	
	int id = enemy.my_screeneffect()?1002:1003;
	workshop_click_radio(1002,1003,id);

	HWND cb = GetDlgItem(som_tool,1000);
	HWND se = GetDlgItem(som_tool,1001);
	SetWindowLong(cb,GWL_USERDATA,-1); //SfxEdit_1000
	wchar_t w[32];
	WORD sfx = enemy.my_sfx();
	_itow(sfx,w,10);
	//the following subroutine initializes screen
	//effects magic... ebx is "friendlySFX" and it
	//must be less than 16
	//00428220 53                   push        ebx
	BYTE model = 0xFF;
	if(!enemy.my_screeneffect())
	{	
		int i = ComboBox_FindStringExact(cb,-1,w);	
		if(i!=CB_ERR)
		{
			LPARAM lp = ComboBox_GetItemData(cb,i);
			model = lp>>8&0xFF;			
			ComboBox_SetCurSel(cb,i);			
		}	
		else SetWindowText(cb,w);
		windowsex_notify(som_tool,1000,CBN_EDITCHANGE);
		*w = '\0'; //SetWindowText(se,L""); 
	}
	else 
	{	
		//historically these are hardcoded by 42D5B0 (som_db.exe)
		if(sfx<12) model = 207; //bubbles
		else model = 208+sfx-12; //shields		
	}
	if(0xFF!=model)
	sprintf(enemy.model,"%04d.mdl",model);

	SetDlgItemText(som_tool,1005,EX::need_unicode_equivalent(932,enemy.model));
	SetWindowText(se,w);
	SetDlgItemText(som_tool,1006,EX::need_unicode_equivalent(932,enemy.name));
	
	//EneEdit_4173DC = hack; 
	if(&enemy==EneEdit_4173DC) EneEdit_4173DC = 0;	
	
	return true;
}
extern BOOL WINAPI workshop_ReadFile(HANDLE in, LPVOID to, DWORD sz, LPDWORD rd, LPOVERLAPPED o)
{
	SOM_TOOL_DETOUR_THREADMAIN(ReadFile,in,to,sz,rd,o)

	EXLOG_LEVEL(7) << "workshop_ReadFile()\n";

	if(!SOM::Tool.ReadFile(in,to,sz,rd,o))
	return 0;

	if(in==workshop_reading)
	{
		//ADEQUATE?
		assert(*rd==SetFilePointer(in,0,0,FILE_CURRENT));
		
		//HACK: BM_CLICK changes focus/takes time to do so
		//NOTE: if drag-dropping, the window is not focused
		//and so the best it can do is to select the default
		//just defaulting always, for the sake of consistency 
		//som_tool_neighbor_focusing = GetFocus();
		SetTimer(som_tool,'stnf',0,0);
		
		workshop->xicard.clear(); 
		memset(workshop->icard,0x00,97);

		//HACK: allow disallowed items/objects		
		int cat = 0; switch(SOM::tool)
		{
		case PrtsEdit.exe: 
			
			if(!workshop_ReadFile_part(*(workshop_cpp::part_prt*)to,*rd))
			assert(0); break;

		case ItemEdit.exe: 
			
			if(workshop_ReadFile_item(*(workshop_cpp::item_prf<>*)to,*rd))
			cat = 62; else assert(0); break;

		//0040A7B5 8B 44 24 7E          mov         eax,dword ptr [esp+7Eh]
		case ObjEdit.exe:
			
			//*will need to add these
			/*switch(substr($cat,82,1))
			{			  
			case "\x14": //box
			case "\x15": //chest*
			case "\x16": //corpse
			case "\x0B": //door* (secret door perhaps)
			case "\x0D": //door* (wood swinging door)
			case "\x0E": //door*
			case "\x0A": //lamp*
			case "\x29": //receptacle*
			case "\x28": //switch*
			case "\x1E": //trap* (spear variety perhaps)
			case "\x1F": //trap* (arrow variety perhaps)
			case "\x00": //inert (may be ornamental)			 
			$box = substr($cat,80,1);			  
			if($box==="\0"||$box==="\1")
			{
				for($i=64;$i<($box==="\1"?76:72);$i+=4) 
				if(substr($cat,$i,4)==="\0\0\0\0") 
				{
					$pc_cat = 45000; //ornaments
				}
			}
			else $err = 1; //other???	      	     
			}*/
			if(workshop_ReadFile_obj(*(workshop_cpp::object_prf*)to,*rd))
			cat = 82; else assert(0); break;

		case EneEdit.exe: 

			if(!workshop_ReadFile_enemy(*(workshop_cpp::enemy_prf*)to,*rd))
			assert(0); break;		

		case NpcEdit.exe:

			if(!workshop_ReadFile_npc(*(workshop_cpp::npc_prf*)to,*rd))
			assert(0); break;

		case SfxEdit.exe:

			if(!workshop_ReadFile_my(*(workshop_cpp::enemy_prf*)to,*rd))
			assert(0); break;
		}
		WORD &w = (WORD&)((BYTE*)to)[cat];
		if(cat) workshop_category = w;
		if(cat) w = 0;

		workshop_reading = 0; 
	}

	return 1;
}

static void workshop_file_update(int);
static void workshop_file_system(int);
static void workshop_DROPFILES(WPARAM wParam)
{
	HDROP &drop = (HDROP&)wParam;
	UINT n = DragQueryFileW(drop,-1,0,0);
	if(0==n&&workshop_tool)
	{
		if(*workshop_savename)
		SendMessage(som_tool,WM_COMMAND,32772,0); //new

		if(0==workshop_mode)
		SendMessage(som_tool,WM_SYSCOMMAND,SC_MINIMIZE,0);
	}
	else for(UINT i=0,prf=0,beep=0;i<n;i++)
	{
		DragQueryFileW(drop,i,som_tool_text,MAX_PATH);

		LPCWSTR ext = PathFindExtension(som_tool_text);
		if(*ext++)		
		if(SOM::tool==PrtsEdit.exe)
		{
			if(!wcsicmp(ext,L"prt")) goto prt;
			if(!wcsicmp(ext,L"msm")) workshop_file_update(1005);
			if(!wcsicmp(ext,L"mhm")) workshop_file_update(1006);			
			if(!wcsicmp(ext,L"bmp")) workshop_file_update(1013);
		}
		else if(!wcsicmp(ext,L"prf")) prt:
		{
			if(prf++) goto beep;

			//HACK: simulate file by command-line argument
			SetEnvironmentVariableW(L".WS",som_tool_text);
			som_tool_initializing++;
			SendMessage(workshop_host,WM_COMMAND,SOM::tool<EneEdit.exe?32771:40001,0);
			som_tool_initializing--;
		}		
		else if(SOM::tool==SfxEdit.exe)
		{
			//NOTE: the names are 4 digits, however the Sfx.dat
			//addressing scheme has 1 byte, or 3 digits (0~255)
			//TODO: investigate SOM::L.SFX_dat_file (0x1C91D30) 
			const wchar_t *p = PathFindFileName(som_tool_text);
			for(int i=0;i<4;i++) if(!isdigit(p[i])) goto beep;
			SetDlgItemInt(som_tool,1040,_wtoi(p),0);
		}
		else if(SOM::tool!=ItemEdit.exe
		&&!wcsicmp(ext,L"mdl")
		||!wcsicmp(ext,L"mdo")
		&&(SOM::tool==ItemEdit.exe||SOM::tool==ObjEdit.exe))
		{
			workshop_file_update(1005);
		}
		else if(SOM::tool>=EneEdit.exe
		&&!wcsicmp(ext,L"txr"))
		{
			workshop_file_update(40074); //skin
			SendMessage(workshop_host,WM_COMMAND,40074,0);
		}
		else beep:
		{
			if(!beep++) MessageBeep(-1); 
		}
	}	
	DragFinish(drop);
}
static void workshop_MOUSEWHEEL(int delta)
{
	DWORD rs; if(delta) //WM_MBUTTONDOWN?
	{
		float amount = delta/WHEEL_DELTA*0.005f;
		DDRAW::Direct3DDevice7->GetRenderState(DX::D3DRENDERSTATE_AMBIENT,&rs);
		float a = (rs&0xff)/255.0f+amount; 
		if(a>1) a = 1; if(a<0) a = 0;
		rs = a*255; rs|=rs<<8|rs<<16;
	}
	else //NOTE: workshop_SetWindowTextA triggers this at startup
	{
		//ItemEdit's lights are darker for some reason. it may've
		//depended on the original background colors, but I don't
		//think so. this is just compensating instead of figuring
		//out what's different between them
		switch(SOM::tool)
		{
		//I guses this is useful to retain... was going to delete
		//0x00202020 (ItemEdit)
		//0040669F 68 20 20 20 00       push        202020h
		//nothing? (ObjEdit) 
		//0x00606060 EneEdit/NpcEdit
		//00402761 68 60 60 60 00       push        606060h //Ene
		//00402751 68 60 60 60 00       push        606060h //NPC
		 
		//ItemEdit may not be lit at all???
		case ItemEdit.exe: rs = 0xff808080; break; //202020?
		case ObjEdit.exe: rs = 0xff303030; break; //0?
		default: rs = 0xff606060; break; //606060
		}
	}
	DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_AMBIENT,rs);
	workshop_redraw();
}

extern "C" //#include <d3dx9math.h>
{
	struct D3DXMATRIX* WINAPI D3DXMatrixPerspectiveFovLH
	(D3DXMATRIX*,FLOAT,FLOAT,FLOAT,FLOAT);
	struct D3DXMATRIX* WINAPI D3DXMatrixPerspectiveOffCenterLH
	(D3DXMATRIX*,FLOAT,FLOAT,FLOAT,FLOAT,FLOAT,FLOAT);
}

static void workshop_minmax(bool redraw=true)
{
	if(redraw) workshop_redraw();

	BYTE diff = workshop_mode; //REMOVE ME?

	bool mini = WS_MAXIMIZEBOX&GetWindowStyle(som_tool);
	workshop_mode = mini;	

	diff-=workshop_mode;

	bool pretty = diff;
	if(pretty) SetWindowRedraw(som_tool,0);

	if(workshop_tool) if(!mini) //e.g. SOM_PRM
	{
		if(!*workshop_savename)
		{
			WCHAR name[64];	
			if(GetWindowTextW(workshop_tool,name,64))
			SetWindowTextW(som_tool,name);
		}

		SendMessage(som_tool,WM_SETICON,ICON_BIG,0);
		SendMessage(som_tool,WM_SETICON,ICON_SMALL,0);		
	}
	else  
	{	
		if(!*workshop_savename&&workshop_title)
		SetWindowTextW(som_tool,workshop_title);

	hack: //WM_SETICON doesn't work with som_tool_CreateWindowExA

		extern void som_tool_icon(HWND); som_tool_icon(som_tool);
	}	
	else if(SOM::tool>=EneEdit.exe) goto hack;
	
	//REMINDER: helps to do after hiding/showing 
	//children... but it gets the maximized size
	//wrong if so
	if(workshop_menu)
	{	
		SetMenu(som_tool,mini?workshop_menu:0);
		//DrawMenuBar(som_tool);
	}	

	HWND pw = 0;
	HWND picture_frame = 0;
	if(SOM::tool==PrtsEdit.exe) 
	{
		//pw = GetDlgItem(som_tool,1022);
		picture_frame = GetDlgItem(som_tool,1023);
		ShowWindow(picture_frame,!mini);
	}
	else pw = GetDlgItem(som_tool,1014);

	DX::D3DVIEWPORT7 vp = {0,0,0,0,0,1};
	DWORD &cx = vp.dwWidth, &cy = vp.dwHeight;
	RECT rc; GetClientRect(som_tool,&rc);	
	if(!IsRectEmpty(&workshop_picture_window)) //legacy?
	if(mini) rc = workshop_picture_window;	
	cx = rc.right-rc.left; cy = rc.bottom-rc.top;
	if(pw) MoveWindow(pw,rc.left,rc.top,cx,cy,1);	
	
	//HACK: diff is protecting som_tool_neighbor_focusing 
	//vis-a-vis workshop_ReadFile_obj
	//HACK: must ShowWindow when a file is opened because
	//either setting the radios shows them as side effect
	//or something ObjEdit, etc. does does so
	//if(diff||som_tool_initializing)
	bool focus = diff||som_tool_initializing;
	{
		//WINSANITY: the focus is advanced to the next 
		//tabstop with each toggle???
		if(!mini&&focus&&!som_tool_neighbor_focusing)		
		som_tool_neighbor_focusing = GetFocus();
		{
			int pg = workshop_page2&&mini?1:0;
			HWND ch0 = GetWindow(som_tool,GW_CHILD);
			for(HWND ch=ch0;ch;ch=GetNextWindow(ch,GW_HWNDNEXT))
			if(ch!=pw&&ch!=picture_frame)
			{
				if(pg) //2020
				{
					if(ch==workshop_page2)
					pg = 2;
					if(pg!=workshop_page)					
					switch(GetDlgCtrlID(ch))
					{
					//YUCK: assume EneEdit OFF button
					case 1001: case 2999:						
					ShowWindow(ch,1); continue;
					default:
					ShowWindow(ch,0); continue;
					}
				}
				ShowWindow(ch,mini); 
			}
		}
		if(mini&&focus) if(som_tool_neighbor_focusing)
		{			
			SetFocus(som_tool_neighbor_focusing); 
			som_tool_neighbor_focusing = 0;
		}
		//else assert(som_tool_neighbor_focusing);
	}	

	if(cx&&DDRAW::Direct3DDevice7)
	{
		//REMINDER: SOM uses 50. 30 cuts the heads off if not adjusted for height
		//or the view matrix is pulled back further? will have to figure that out
		enum{ fov=0?30:50 };
		float zn = 0.1f, zf = 30;

		DDRAW::Direct3DDevice7->SetViewport(&vp); 

		//////D3DXMatrixPerspectiveLH SET UP/////////
		//
		//guess work: trying to get close to SOM_PRM's
		float factor; switch(SOM::tool)
		{
		case SfxEdit.exe:
		case ItemEdit.exe: //factor = 1650; break;
		case ObjEdit.exe: //factor = 3100; break;
			//Ene/NpcEdit seem to zoom in LMB+RMB more by default that SOM_PRM
			//so this is likely completely off. The others may too. The dragon
			//is good for checking this. WILL HAVE TO LOCATE THE ZOOM VARIABLE
		default: factor = 4800*2; break;
		}
		float x = cx/factor, y = cy/factor;
		EX::INI::Editor ed;
		float zoom = ed->clip_volume_multiplier;
		//if(ed->do_enlarge)
		{
			//matches SOM_PRM behavior
			extern float som_tool_enlarge;
			zoom/=som_tool_enlarge;
		}
		if(mini) zoom*=(float)rc.bottom/cy;
		if(zoom>0) //som_hacks_SetViewport2?
		{
			x*=zoom; y*=zoom;			
		}	
		//TODO: THIS PROBABLY DEFIES EXPECTATIONS IN TERMS OF A VISUAL CENTER
		//this looks so much better up close, and more importantly matches how
		//SOM_PRM is. but I don't know if the perspective is the same, since it
		//puts the visual center much lower than the center of the screen itself
		/*2020: needs fix... if height is 0 som_db's math breaks down, so some
		//objects may use small negative values
		WCHAR minus[2] = {};
		GetDlgItemTextW(som_tool,1002,minus,2);*/
		bool minus = GetDlgItemFloat(som_tool,1002)<-0.01f;
		float offcenter;
		if(SOM::tool==ItemEdit.exe)
		{
			//NOTE: centering ground mode so it can be tilted down
			if(!IsDlgButtonChecked(som_tool,1000))
			goto sfx;
			else goto SOM_PRM_like;
		}
		else if(SOM::tool==SfxEdit.exe) sfx:
		{
			offcenter = 0.5f;
		}
		else if(mini)
		{				
			//Trying to maximize small picture window... also gets grid out of the way
			//BUT THE GRIDLINES ARE HIDDEN NOW ANYWAY, EXCEPT IN ITEM/GROUND EDIT MODE
			//(might bring them back as an option if EneEdit/NpcEdit can display them)
			offcenter = minus; //'-'==minus[0];
		}
		else SOM_PRM_like: //try to match SOM_PRM
		{
			offcenter = minus?0.85f:0.15f; //'-'==minus[0]
		}
		float ty = -y*offcenter, by = y*(1-offcenter);
		
		extern float som_hacks_inventory[4][4];
		DX::LPD3DMATRIX m = (DX::LPD3DMATRIX)som_hacks_inventory;			
		if(0) D3DXMatrixPerspectiveFovLH((D3DXMATRIX*)m,M_PI/180*fov,(float)cx/cy,zn,zf);
		else D3DXMatrixPerspectiveOffCenterLH((D3DXMATRIX*)m,-x/2,x/2,ty,by,zn,zf);
		//m->_41-=0.50; //stereo?
		DDRAW::Direct3DDevice7->SetTransform(DX::D3DTRANSFORMSTATE_PROJECTION,m);
	}
	
	if(pretty) //MAYBE PUT INSIDE TIMER?
	{
		SetWindowRedraw(som_tool,1); 
		EX::vblank();
		//crashes EneEdit.exe and glitches on others
		//RedrawWindow(som_tool,0,0,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN|RDW_ERASE|RDW_ERASENOW);	
		InvalidateRect(som_tool,0,1);		
	}
}
 
//0xB4 has a more even dither pattern, but I feel like 0xBE is crisper
//I think mainly 0xBE has better contrast--for the models that need it
//0xBE is 0xA0 plus some extra helps make the silhouettes appear crisp 
//with do_smooth and do_aa2
static COLORREF workshop_colors[2+16] = {0,!0xBE0000, //SfxEdit?
//these are the original colors of ItemEdit, ObjEdit/EneEdit & NpcEdit
0xFF0000,0xBE0000,0x800000,0x400000,0x300000,0,0x808000,0xA78727,
//just pretty colors
0,0xD0BB50,0xE08E70};
extern BOOL workshop_choosecolor(HWND hw=som_tool, COLORREF *ref=workshop_colors+workshop_mode)
{
	CHOOSECOLOR cc = {sizeof(cc),hw,0,*ref,workshop_colors+2,CC_RGBINIT|CC_FULLOPEN};
	if(!ChooseColor(&cc)) return 0; *ref = cc.rgbResult; return 1;
}
static void *workshop_CreateDevice(HRESULT*hr,DDRAW::IDirect3D7*,const GUID*&x,DX::LPDIRECTDRAWSURFACE7&,DX::LPDIRECT3DDEVICE7*&z)
{
	//DUPLICATE //SOM_MAP.cpp
	DWORD _ = DDRAW::doDither?16:32; SOM::bpp = _; //z
	extern bool som_hacks_setup_shaders(DWORD&);
	if(som_hacks_setup_shaders(_))
	{
		//sets blit shader
		//DDRAW::hack_interface(DDRAW::DIRECTDRAWSURFACE7_BLT_HACK,som_hacks_Blt);
		//DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_DRAWPRIMITIVE_HACK,som_hacks_DrawPrimitive);
		//sets unlit/blended shader
		//DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_DRAWINDEXEDPRIMITIVEVB_HACK,som_hacks_DrawIndexedPrimitiveVB);

		//using BLENDED shader exclusively for now
		DDRAW::vs = DDRAW::ps = 3; 
		assert(16==DDRAW::fx); //DDRAW::fx = 16;
	}
	return 0;
}
static void *workshop_CreateSurface(HRESULT *hr,DDRAW::IDirectDraw7*p,DX::LPDDSURFACEDESC2&x,DX::LPDIRECTDRAWSURFACE7*&,IUnknown*&)
{
	//software z-buffer is inadequate. not sure why these tools select
	//the software devices???
	if(x->ddsCaps.dwCaps&(DDSCAPS_ZBUFFER|DDSCAPS_3DDEVICE))
	{
		//HACK: using this as a way to opt out of forcing to use hardware 
		if(!EX::INI::Editor()->do_hide_direct3d_hardware)
		{
			x->ddsCaps.dwCaps&=~DDSCAPS_SYSTEMMEMORY;
			x->ddsCaps.dwCaps|=DDSCAPS_VIDEOMEMORY;
		}  
		
		if(SOM::tool>=EneEdit.exe)
		{
			RECT rc; GetClientRect(som_tool,&rc); 
			MoveWindow(workshop_host,0,0,rc.right,rc.bottom,0);
			x->dwFlags&=~(DDSD_WIDTH|DDSD_HEIGHT);
			x->dwHeight = rc.bottom; x->dwWidth = rc.right;
		}
	}
	return 0;
}
static void *workshop_SetTexture(HRESULT*,DDRAW::IDirect3DDevice7*,DWORD&,DX::LPDIRECTDRAWSURFACE7&y)
{
	if(!y) y = workshop_texture(); return 0;
}
static void *workshop_Clear(HRESULT*,DDRAW::IDirect3DDevice7*,DWORD&,DX::LPD3DRECT&y,DWORD&z,DX::D3DCOLOR&w,DX::D3DVALUE&q,DWORD&)
{
	if(z!=D3DCLEAR_TARGET) //fxInflateRect?	
	{
		if(SOM::tool>=EneEdit.exe) //do_enlarge
		{
			assert(y); 
			if(0==workshop_mode)			
			GetClientRect(som_tool,(RECT*)y); 							
		}

		w = workshop_colors[workshop_mode]; 
		if(0==w) w = EX::INI::Editor()->default_pixel_value;
		else std::swap(*(BYTE*)&w,*((BYTE*)&w+2)); 
	}
	return 0;
}
static void *workshop_BeginScene(HRESULT*,DDRAW::IDirect3DDevice7*)
{
	if(DDRAW::inScene)
	{
		DDRAW::Direct3DDevice7->EndScene(); 
		
		//EneEdit.exe is triggering this when a TXR
		//skin having PRF is opened after a non TXR
		//having skin PRF is opened
		//(see also DDRAW_ASSERT_REFS(queryX->group,==2) in dx.d3d9c.cpp)
//		assert(workshop_mode2==16);
	}
	return 0;
}
static void *workshop_DrawPrimitive(HRESULT*hr,DDRAW::IDirect3DDevice7*in,DX::D3DPRIMITIVETYPE&x,DWORD&y,LPVOID&z,DWORD&w,DWORD&q)
{	
	if(hr)
	{
		DDRAW::vs = DDRAW::ps = 3; return 0; //2021
	}

	//Reminder: this is needed whether or not ItemEdit's prior mode has the grid
	if(x==DX::D3DPT_LINELIST)
	{
		if(SOM::tool!=ItemEdit.exe||IsDlgButtonChecked(som_tool,1000))
		{
			w = 0; 
		}
		else if(DDRAW::dejagrate) if(DDRAW::compat=='dx9c')
		{
			if(w==28&&y==D3DFVF_LVERTEX) //0x1e2 //!!!!!!!! 
			{
				//experimenting with ID3DXLine
				extern bool Ex_output_lines(float*,int,int,WORD*,DWORD);
				if(Ex_output_lines((float*)z,8,28*8,0,0xB0FFFFFF))
				{
					w = 0; 
				}
			}
			else assert(0);
		}
		else assert(0);
	}
	else assert(0); 
	
	if(y==D3DFVF_LVERTEX&&w) //2021
	{
		DDRAW::vs = DDRAW::ps = 2; return workshop_DrawPrimitive;
	}
	else return 0;
}
extern void *workshop_DrawIndexedPrimitive(HRESULT*hr,DDRAW::IDirect3DDevice7*in,DX::D3DPRIMITIVETYPE&x,DWORD&y,LPVOID&z,DWORD&w,LPWORD&q,DWORD&r,DWORD&s)
{
	//REMINDER: SOM_MAP.cpp REUSES THIS

	if(y==D3DFVF_LVERTEX) //2021
	{
		DDRAW::vs = DDRAW::ps = hr?3:2; 
		
		return workshop_DrawIndexedPrimitive;
	}
	else return 0;
}
static void *workshop_SetTransform(HRESULT*,DDRAW::IDirect3DDevice7*,DX::D3DTRANSFORMSTATETYPE&x,DX::LPD3DMATRIX&y)
{
	if(x==DX::D3DTRANSFORMSTATE_VIEW)
	{
		assert(y->_11==y->_22==y->_33); //1,1,1
		if(SOM::tool>=EneEdit.exe)
		{
			assert(y->_43==6); //always ID+6z?
			//12 is quite close to SOM_PRM
			//y->_43 = 12;	
		}
		else if(SOM::tool==ObjEdit.exe)
		{
			assert(y->_43==4); //always ID+4z?
			//12 is quite close to SOM_PRM
			//y->_43 = 12;	
		}
		else
		{
			assert(y->_43==2); //always ID+2z?
			//y->_43 = 12;
		}
		//12.751 accommodates the full height of the smaller doors into the 
		//original thumbnail size, but is somewhat smaller than SOM_PRM
		//(WTH? ItemEdit becomes larger????)
		y->_43 = 12; //12.75; //12;
	}
	else if(x==DX::D3DTRANSFORMSTATE_WORLD)	//426258 is ID?
	{
		//WON'T WORK. y is VIEW+WORLD (probably so lights don't have to be updated)
		//if(SOM::tool==ItemEdit.exe&&IsDlgButtonChecked(som_tool,1000))
		//y = (DX::LPD3DMATRIX)0x426258;

		//FIX: ItemEdit seems not to implement this correctly
		//the player never moves the item vertically, whereas
		//ItemEdit doesn't seem to vertically center properly		
		if(15==workshop_mode2 //1015?
		&&SOM::tool==ItemEdit.exe)
		{
			//401C80 looks like could be matrix set up code
			//00401FF3 E8 88 FC FF FF       call        00401C80
			//builds translation matrix?
			//00401DC2 E8 89 2C 00 00       call        00404A50
			//ItemEdit_471ab0::MainWnd *test = &ItemEdit_471ab0.main();

			//Reminder: I think 0.5 was compensating for a bug
			//induced by workshop_mode switches
			//y->_42 = 0.5f;
			//y is always changing
			if(0x426258!=(DWORD)y) //identity I think
			{
				/*2020, April: It's changed today?
				//19E934 seems to point to the matrix
				//just happened to notice this memory
				//it's just local stack memory. seems
				//to be a revolving pool. hard to say
				assert((DWORD)y==*(DWORD*)0x19E934);
				*/
			//	assert((DWORD)y==*(DWORD*)0x2312cd0); //higher memory?

				y->_42 = -0.25;
			}
		}
	}
	return 0;
}
static BYTE __cdecl ItemEdit_401C80(FLOAT *_1, DWORD _2)
{
	bool z = 15==workshop_mode2
	&&GetMenuState(workshop_mode2_15_submenu,34004,0);
	if(z) std::swap(_1[7],_1[9]);
	BYTE ret = ((BYTE(__cdecl*)(FLOAT*,DWORD))0x401C80)(_1,_2);
	if(z) std::swap(_1[7],_1[9]);
	return ret;
}

static BOOL PrtsEdit_Open_or_Save_chkstk(bool save)
{
	OPENFILENAMEA a = {sizeof(a),som_tool,0,som_932_PrtsEdit_filter};
	return (save?GetSaveFileNameA:GetOpenFileNameA)(&a);
}
static void PrtsEdit_Pen_Mode()
{
	if(SOM::tool!=PrtsEdit.exe) return;
	workshop_mode2 = workshop_mode2==1?-1:1;
	CheckMenuItem(workshop_menu,35005,workshop_mode2==1?MF_CHECKED:0);
}
static void PrtsEdit_SoftMSM(int wp)
{	
	if(SOM::tool!=PrtsEdit.exe) return;
	extern int SoftMSM(const wchar_t*,int); 

	workshop_file_system(-1005); //set som_tool_text to msm path

	if(SoftMSM(som_tool_text,wp&3)) MessageBeep(-1);
}

static wchar_t*workshop_filters[3] = {};
static void workshop_init_filter(int id)
{
	int i; wchar_t filter[64+8];
	if(i=GetDlgItemText(som_tool,id,filter,64))
	SetDlgItemText(som_tool,id,L"");
	else return;
	const char *a = "", *b = a, *art = 0;
	wchar_t **f = workshop_filters;
	switch(id)
	{
	case 1005: art = a = "*.mdl;";

		switch(SOM::tool)
		{		
		case ItemEdit.exe: a = "";
		case ObjEdit.exe: b = "*.mdo"; break;
		case PrtsEdit.exe: a = "*.msm"; break;
		case SfxEdit.exe: b = "*.txr"; break;
		}		
		break;

	case 1006: art = a = "*.mhm"; f++; break;
	case 40074: art = a = "*.txr"; f++; break;
	//case 40075:
	case 1013: a = "*.bmp"; f+=2; break;
	}
	art = art?";*.*":""; //2021
	i+=swprintf(filter+i+1,L"%hs%hs%hs%lc",a,b,art,0);	
	wmemcpy(*f=new wchar_t[i+2],filter,i+2);
}
static void PrtsEdit_INITDIALOG()
{			  
	workshop_init_filter(1006);
	workshop_init_filter(1013);

	if(workshop_menu)
	workshop_accelerator(workshop_menu);
	else assert(0);

	char hide[] = {7,9,10,12,25};
	for(int i=0;i<sizeof(hide);i++)
	MoveWindow(GetDlgItem(som_tool,1000+hide[i]),0,0,0,0,0);
	
	//PrtsEdit seems to limit these to 31
	char lim[] = {5,6,13};
	for(int i=0;i<sizeof(lim);i++)
	PostMessage(GetDlgItem(som_tool,1000+lim[i]),EM_LIMITTEXT,30,0);
	//imposing 2 0-terminators on the 128
	//character buffer, so the title must
	//be standard 30 chars
	PostMessage(GetDlgItem(som_tool,1008),EM_LIMITTEXT,126,0);

	som_tool_tilespace.x = 0; //SOM_MAP sets it to -1

	SendDlgItemMessage(som_tool,1026,UDM_SETRANGE32,0,255);

	//this is a feature to use a default turn setting
	//in SOM_MAP to standardize its palette window
	if(workshop_menu) 
	{ 
		for(int i=34000;i<=34270;i+=90)
		{
			int check = i==34000?i:0;
			CheckMenuRadioItem(workshop_menu,i,i,check,0);
			if(!check) //feature is unimplemented
			EnableMenuItem(workshop_menu,i,MF_GRAYED);
		}
		for(int i=35001;i<=35004;i++)
		{
			int check = i==35001?i:0;
			CheckMenuRadioItem(workshop_menu,i,i,check,0);
			if(!check) //feature is unimplemented
			EnableMenuItem(workshop_menu,i,MF_GRAYED);
		}
		//feature is unimplemented
		EnableMenuItem(workshop_menu,35000,MF_GRAYED);
		for(int i=35006;i<=35010;i++)
		EnableMenuItem(workshop_menu,i,MF_GRAYED);
	}
	HBITMAP map_icon = (HBITMAP)som_map_LoadImageA
	(0,"DATA\\MENU\\map_icon.bmp",0,0,0,LR_LOADFROMFILE);
	for(int i=1021;i<=1022;i++)
	if(HWND pc=GetDlgItem(som_tool,i))
	{
		//not even BS_BITMAP can produce a square/exact picture
		//(or, at least not with ResEdit?)
		RECT rc; GetClientRect(pc,&rc);		
		{
			int fit = i&1?2:3;
			rc.right = rc.right/fit*fit; 
			rc.bottom = rc.right;
			AdjustWindowRectEx(&rc,GetWindowStyle(pc),0,GetWindowExStyle(pc));
			rc.right-=rc.left; rc.bottom-=rc.top;
			SetWindowPos(pc,0,0,0,rc.right,rc.bottom,SWP_NOMOVE|SWP_NOZORDER);
			if(~i&1)
			{
				workshop_picture_window.right =
				workshop_picture_window.left+rc.right;
				workshop_picture_window.bottom =
				workshop_picture_window.top+rc.bottom;
			}
		}		
		SetWindowStyle(pc,SS_TYPEMASK,SS_OWNERDRAW);
		//som_tool_dc is b/w by default... how to change that???
		HDC hwdc = GetWindowDC(som_tool); 
		if(i==1022){ rc.right = 4+60*3; rc.bottom = 4+13*3*3; }
		HBITMAP bm = CreateCompatibleBitmap(hwdc,rc.right,rc.bottom);
		SetWindowLong(pc,GWL_USERDATA,(LONG)bm);
		HGDIOBJ so = SelectObject(som_tool_dc,bm);			
		if(~i&1)
		{
			HDC tmp = CreateCompatibleDC(som_tool_dc);
			HGDIOBJ so2 = SelectObject(tmp,map_icon);
			SetStretchBltMode(som_tool_dc,COLORONCOLOR);
			SOM::Tool.StretchBlt(som_tool_dc,2,2,60*3,13*3*3,tmp,0,0,60,13*3,SRCCOPY); 
			SelectObject(tmp,so2); DeleteDC(tmp);			
		}
		//else FillRect(som_tool_dc,&cr,(HBRUSH)GetStockObject(BLACK_BRUSH));
		SelectObject(som_tool_dc,so);
		ReleaseDC(som_tool,hwdc);
	}
	DeleteObject(map_icon);
}
static HWND SfxEdit_INITDIALOG_right(HWND hw=som_tool)
{
	HWND cb; for(int pass=1;pass<=2;pass++)
	if(cb=GetDlgItem(hw,pass==1?1041:1000))
	{
		SetWindowStyle(GetDlgItem(cb,1001),ES_RIGHT|ES_NUMBER);
		COMBOBOXINFO cbi = {sizeof(cbi)};
		if(GetComboBoxInfo(cb,&cbi))
		SetWindowExStyle(cbi.hwndList,WS_EX_RIGHT);
		if(pass==1&&hw==som_tool)
		{
			BYTE r[] = {0,15, 30,35, 40,46, 100,102, 128,134};
			for(int i=0;i<sizeof(r);i+=2)
			{
				wchar_t w[32];
				if(r[i]==100||r[i]==128)
				ComboBox_AddString(cb,L"---");
				while(r[i]<=r[i+1])			
				ComboBox_AddString(cb,_itow(r[i]++,w,10));
			}
		}
	}
	return cb;
}
static void SfxEdit_INITDIALOG()
{
	SendDlgItemMessage(som_tool,1019,TBM_SETRANGEMAX,1,2);
		
	HANDLE h = SfxEdit_1000(0);
	DWORD rd,sz = GetFileSize(h,0);
	char *buf = new char[sz];
	ReadFile(h,buf,sz,&rd,0);
	CloseHandle(h); 
	HWND cb = SfxEdit_INITDIALOG_right();
	int prev = 1025*48;
	for(int fp=0;fp<(int)rd;fp+=48)
	{
		struct record //REMOVE ME
		{
			//if 42ED10 returns zero floats8[0] and [1] are
			//processed @42E63D					
			//0042E631 E8 DA 06 00 00       call        0042ED10 
			//...
			//0042ED2B 8A 88 30 1D C9 01    mov         cl,byte ptr [eax+1C91D30h]
			//<100 returns 0
			//>127 returns 2
			//100~127 (then) returns 1
			//...
			//THESE ARE LEGACY PROCEDURE VALUES
			//0042EA51 8B 14 CD 48 E6 45 00 mov         edx,dword ptr [ecx*8+45E648h]
			//0~15
			//20~22
			//30~34
			//40~46
			//100~102 (type 1)
			//128~134 (type 2)
			//
			BYTE procedure;
			//0042E5E8 66 0F B6 BE 31 1D C9 01 movzx       di,byte ptr [esi+1C91D31h] 
			BYTE model;							
			BYTE unknown1;
			//0042F00B 8A 90 33 1D C9 01    mov         dl,byte ptr [eax+1C91D33h]
			BYTE unknown2;
			//004314E4 8A 50 04             mov         dl,byte ptr [eax+4]
			BYTE unknown3;
			BYTE unknown4;
			BYTE unknown5;
			BYTE unknown6;
			//0
			//0042E63D D9 86 38 1D C9 01    fld         dword ptr [esi+1C91D38h]
			//1
			//0042E64E D9 86 3C 1D C9 01    fld         dword ptr [esi+1C91D3Ch]
			//2
			//00431452 05 30 1D C9 01       add         eax,1C91D30h  
			//...
			//004314A8 8B 48 10             mov         ecx,dword ptr [eax+10h]
			//3
			//00431452 05 30 1D C9 01       add         eax,1C91D30h  
			//00431457 8B 48 14             mov         ecx,dword ptr [eax+14h]
			FLOAT floats8[8];
			//
			//entered when equipping magic?
			//43F420 appears to increase SND reference counts
			//
			//0042E5FD 66 8B 86 58 1D C9 01 mov         ax,word ptr [esi+1C91D58h]  
			//0042E604 66 3D FF FF          cmp         ax,0FFFFh 
			//0042E608 74 10                je          0042E61A  
			//0042E60A 25 FF FF 00 00       and         eax,0FFFFh  
			//0042E60F 6A 00                push        0  
			//0042E611 50                   push        eax  
			//0042E612 E8 09 0E 01 00       call        0043F420  
			//0042E617 83 C4 08             add         esp,8  
			//
			//execution context (windcutter)
			//0042EAB6 66 8B 88 58 1D C9 01 mov         cx,word ptr [eax+1C91D58h]
			//...
			//0042EAE9 E8 C2 0A 01 00       call        0043F5B0
			//(will call DSOUND::IDirectSound3DBuffer::SetPosition)
			WORD ffff;				
			//0042E5C0 this is RECURSIVE... checking WORD is 0 or 1???? WORD?
			//(seems to call 42E5C0 again, on the next Sfx.dat entry)
			//...
			//0042E61A 66 83 BE 5A 1D C9 01 00 cmp         word ptr [esi+1C91D5Ah],0  
			//0042E622 74 0C                je          0042E630  
			//0042E624 8D 43 01             lea         eax,[ebx+1]  
			//0042E627 50                   push        eax  
			//0042E628 E8 93 FF FF FF       call        0042E5C0  
			//0042E62D 83 C4 04             add         esp,4  
			WORD zero1;
			//this is PITCH ... for some reason it is unsigned
			//(24 is subtracted from it)
			//0042EACA 8A 80 5C 1D C9 01    mov         al,byte ptr [eax+1C91D5Ch]
			//..
			//0042EADE 2C 18                sub         al,18h
			BYTE zero2; //24 for #800 (38400)
			//padding?
			BYTE zero3[3];
		}&rec = *(record*)(buf+fp);				
		//0042EA32 8A 88 30 1D C9 01    mov         cl,byte ptr [eax+1C91D30h]  
		//0042EA38 81 F9 00 01 00 00    cmp         ecx,100h  
		//0042EA3E 0F 8D B2 00 00 00    jge         0042EAF6  
		//0042EA44 80 B8 31 1D C9 01 FF cmp         byte ptr [eax+1C91D31h],0FFh  
		//0042EA4B 0F 83 A5 00 00 00    jae         0042EAF6  
		if(0xffff==*(WORD*)&rec)
		{
			for(int i=2;i<sizeof(rec);i+=2)
			assert(0==*(WORD*)(&rec.procedure+i));
			continue;
		}		
		if(0xffff!=rec.ffff||0!=rec.zero2) switch(fp) //oddballs?
		{
		default: //assert(0); //INCOMPLETE
		//
		//815 is a PC equipped attack magic that sets
		//rec.ffff to 182
		//rec.ffff looks like a sound effect number
		//
		case 38400: //800: 820/24 procedure is 0???
		case 38448: //801: 167/24 procedure is 0x14
		case 38544: //803: 182/27 procedure is 0x04
		case 38592: //804: 171/27 procedure is 0x04 
				
		//CONTINUED BELOW
		case 39360: //820: 194/24 procedure is 0x28 
		case 39408: //negative (ffff/0)
		case 39456: //negative (ffff/0)

			int bp = 0; break; //breakpoint
		}
		if(0!=rec.zero1) switch(fp) //oddballs?
		{
		//INCOMPLETE
		case 39360: //820: 1
		case 39408: //821: 1 procedure is 0x29
		case 39456: //822: 1 procedure is 0x2a

			//RECURSIVE if nonzero... into the next
			//Sfx.dat entry
			//0042E61A 66 83 BE 5A 1D C9 01 00 cmp         word ptr [esi+1C91D5Ah],0 
			default: assert(1==rec.zero1); 
		}		
		assert(0==rec.zero3[0]&&0==rec.zero3[1]&&0==rec.zero3[2]);
		/*som_db
		//45E648 holds a procedure table... two pointers apiece
		0042EA51 8B 14 CD 48 E6 45 00 mov         edx,dword ptr [ecx*8+45E648h]  
		0042EA58 85 D2                test        edx,edx  
		0042EA5A 0F 84 96 00 00 00    je          0042EAF6  
		0042EA60 8B 04 CD 4C E6 45 00 mov         eax,dword ptr [ecx*8+45E64Ch]
		*/
		if(0) //INCOMPLETE
			/*complete list follows
			//0~15
			//20~22
			//30~34
			//40~46
			//100~102 (type 1)
			//128~134 (type 2)
			*/
		switch(rec.procedure) 
		{
		case 0xFF:
			//assert(rec.model==0xFF);
			break;

			//LINKED/SOUND
			//42E5C0 is interested in floats8[0~1]
			//(integers?)
			//I think floats8[0] here is a link to
			//another SFX field. Or 65535 if there
			//is not a link
			//floats8[1] seems to be an index also
			//43F420 compares it to 1024. it seems
			//to be a SND reference, into 1D11E54h
			//(SOM::L.SND_ref_counts)

			//42ED10 type 0 (0~0x63)

		//0~15
		case 0x00: //80
		case 0x03:
		case 0x04: case 0x05: case 0x09: 
		case 0x0C: //54.MDL (firewall?)
		case 0x0F:

		//20~22
		case 0x14: 
			//MDL & TXR :(
		case 0x15: //110.mdl //168.txr
					
		//30~34
		case 0x1e: //poison cloud (texture only)
		case 0x22:
				
			//LEAF EFFECTS?
			//42E5C0 is uninterested in floats8[0~1]
			//(scale factors?)

			//42ED10 type 1 (0x64~0x7f)

		//100~102 (type 1)
		case 0x64: //explosion?	(textures 10 & 181)
		case 0x65: //54.MDL (firewall?) 

			case 0x66: //UNUSED? //2020
				
			//42ED10 type 2 (0x80~0xff)

		//128~134 (type 2)
		case 0x80: 
		case 0x81: //191.mdl (lizardman fireball)

			case 0x82: //UNUSED? //2020

				//dimmer billboard?

		case 0x83: 
		case 0x84: //7.txr //50.txr //177.txr (flame)
		
				//bright billboard (standard)

			case 0x85: //UNUSED? //2020

		case 0x86:
			break;				
		default: assert(0);
		}		
		
		if(fp-prev>48)
		ComboBox_AddString(cb,L"---");
		prev = fp;

		wchar_t w[32];
		_itow(fp/sizeof(rec),w,10);
		int i = ComboBox_AddString(cb,w);
		ComboBox_SetItemData(cb,i,*(LPARAM*)&rec);
	}
	delete[] buf;
	if(!ComboBox_GetCount(cb)) //TODO: translate 
	ComboBox_AddString(cb,L"Sfx.dat did not load");
}

static void workshop_limitpitch(int id, HWND p=som_tool)
{
	BOOL t; int pitch = GetDlgItemInt(p,id,&t,1); 	
	if(pitch<-24) pitch = -24; else if(pitch>20) pitch = 20;
	else return; if(t) SetDlgItemInt(p,id,pitch,1);
}
static LRESULT CALLBACK workshop_ipaddressproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR hlp)
{
	switch(uMsg)
	{	
	case WM_COMMAND:

		switch(HIWORD(wParam))
		{
		case EN_KILLFOCUS:
			
			if(id==1041)
			workshop_limitpitch(LOWORD(wParam),hWnd);
			if(id==1042) //2021
			if(!IsDlgButtonChecked(som_tool,1031))
			{
				BOOL t; int mv = 
				GetDlgItemInt(hWnd,LOWORD(wParam),&t,0);				
				if(t&&mv>249)
				{
					//can go up to 254? SOM_PRM limits 8-bit
					//indices to 249
					SetDlgItemInt(hWnd,LOWORD(wParam),249,0);
					MessageBeep(-1);
				}
			}
			break;
		}
		break;

	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,workshop_ipaddressproc,id);
		break;
	}

	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
extern HWND workshop_ipchangeling(int id, int i, HWND ch, HWND ip)
{
	//if(id==1042) return ch;

	extern int som_tool_subclass(HWND,SUBCLASSPROC);
	if(i==0) som_tool_subclass(ip,workshop_ipaddressproc);

	RECT rc; GetWindowRect(ch,&rc); MapWindowRect(0,ip,&rc);
	int ws = WS_CHILD|WS_VISIBLE; if(id!=1041) ws|=ES_NUMBER; 
	HWND ch2 = CreateWindow(L"EDIT",0,ES_CENTER|ES_AUTOHSCROLL|ws,0,0,0,0,ip,(HMENU)/*id*/i,0,0);
	SetWindowPos(ch2,ch,rc.left,rc.top,rc.right-rc.left-2,rc.bottom-rc.top,0);	
	Edit_LimitText(ch2,3);

	extern HFONT som_tool_typeset;
	SetWindowFont(ch2,som_tool_typeset,0);	 
	SetWindowTheme(ch2,L"",L"");

	DestroyWindow(ch); //EnableWindow(ch,0); ShowWindow(ch,0);
	return ch2;
}
static void workshop_drawitem(DRAWITEMSTRUCT *dis)
{
	switch(int id=dis->CtlID)
	{
	case 1021: case 1022: 

		if(SOM::tool==PrtsEdit.exe)
		{	
			HBITMAP bm = (HBITMAP)
			GetWindowLong(dis->hwndItem,GWL_USERDATA);			
			HGDIOBJ so = SelectObject(som_tool_dc,bm);
			
			if(id&1) 
			{
				if(HWND ec=GetDlgItem(som_tool,1013))
				if(Edit_GetModify(ec))
				{
					Edit_SetModify(ec,0); 
					char a[MAX_PATH] = "data\\map\\icon\\";
					SOM::Tool.GetWindowTextA(ec,a+14,MAX_PATH-14);
					if(HBITMAP bm2=(HBITMAP)som_map_LoadImageA(0,a,0,0,0,LR_LOADFROMFILE))
					{
						HDC tmp = //dis->hDC 
						CreateCompatibleDC(som_tool_dc); 
						HGDIOBJ so2 = SelectObject(tmp,bm2);
						SetStretchBltMode(som_tool_dc,COLORONCOLOR);
						SOM::Tool.StretchBlt(som_tool_dc,0,0,dis->rcItem.right,dis->rcItem.bottom,tmp,0,0,20,20,SRCCOPY);
						SelectObject(tmp,so2); DeleteDC(tmp);
						DeleteObject(bm2); 
					}
					else FillRect(som_tool_dc,&dis->rcItem,(HBRUSH)GetStockObject(BLACK_BRUSH));				
				}
				BitBlt(dis->hDC,0,0,dis->rcItem.right,dis->rcItem.bottom,som_tool_dc,0,0,SRCCOPY);
			}
			else 
			{			
				union{ POINT pt; RECT rc; };
				GetCursorPos(&pt);
				MapWindowPoints(0,dis->hwndItem,&pt,1);	
				bool captured = EX::x!=0; 
				int mode = 0; POINTS pixel;
				if(!captured&&!PtInRect(&dis->rcItem,pt))
				{			
					pen_mode:
					int i = GetDlgItemInt(som_tool,1011,0,0);
					int ix = 2+i%20*9, iy = 2+i/20*9;
					if(1==mode)
					if(int pen=(IsLButtonDown()?1:0)|(IsRButtonDown()?2:0))
					{
						bool shift = GetKeyState(VK_SHIFT)>>15;
						COLORREF cr; switch(pen)
						{
						case 1: cr = shift?0xB3B3B3:0xFFFFFF; break;
						case 2: cr = shift?0x737373:0; break;
						case 3: cr = shift?0xB3B3B3:0x737373; break;
						}

						pixel.x = ix+pixel.x*3; pixel.y = iy+pixel.y*3;
						if(cr!=GetPixel(som_tool_dc,pixel.x,pixel.y))
						{
							if(99==i&&cr) //99 appears unused, but is actually a dummy icon
							{
								//box is triggered a second time on pressing OK without this
								//check
								if(som_tool==GetActiveWindow()) 
								MessageBoxA(som_tool,som_932_PrtsEdit_99,0,MB_OK|MB_ICONHAND);
							}
							else for(int x=0;x<3;x++) for(int y=0;y<3;y++)
							{
								SetPixel(som_tool_dc,pixel.x+x,pixel.y+y,cr);
							}
						}
					}
					SetStretchBltMode(dis->hDC,COLORONCOLOR);
					SOM::Tool.StretchBlt(dis->hDC,0,0,dis->rcItem.right,dis->rcItem.bottom,som_tool_dc,ix,iy,9,9,SRCCOPY);								
				}
				else if(1==workshop_mode2!=GetKeyState(VK_CONTROL)<0) //Pen Mode On? 
				{	
					mode = 1;

					int px = dis->rcItem.right/3;

					pixel.x = pt.x/px; pixel.y = pt.y/px;
					pt.x = pt.x/px*px; rc.right = pt.x+px;
					pt.y = pt.y/px*px; rc.bottom = pt.y+px;

					goto pen_mode;					
				}
				else
				{
					mode = 2;

					if(!captured)
					{	
						som_tool_tile.x = 2+(pt.x+som_tool_tilespace.x)/9*9;
						som_tool_tile.y = 2+(pt.y+som_tool_tilespace.y)/9*9;
					}
					else
					{	
						short *xy = (short*)&workshop_cursor;
					
						for(int i=0;i<2;i++)
						{
							LONG &xx = (&pt.x)[i];
							int a = xy[i]/9;
							int b = xx/9;
							if(a!=b) xy[i] = b*9; //discretize
							//else continue; //no point...illustrating
							int d = (b-a)*9;
							//changing +=d to -=d scrolls like SOM_MAP
							//+= seems like a more natural fit in this
							//case
							(&som_tool_tile.x)[i]+=d;
							(&som_tool_tilespace.x)[i]+=d;
						}							
						LONG lim = 20-dis->rcItem.right/9;
						LONG lim2 = lim-7;
						som_tool_tilespace.x = max(0,min(lim*9,som_tool_tilespace.x));
						som_tool_tilespace.y = max(0,min(lim2*9,som_tool_tilespace.y));						
					}	
					som_tool_tile.x = max(2,min(2+20*9-9,som_tool_tile.x));
					som_tool_tile.y = max(2,min(2+13*9-9,som_tool_tile.y));

					BitBlt(dis->hDC,0,0,dis->rcItem.right,dis->rcItem.bottom,som_tool_dc,som_tool_tilespace.x,som_tool_tilespace.y,SRCCOPY);
					
					pt.x = som_tool_tile.x-som_tool_tilespace.x-2;
					rc.right = pt.x+13;
					pt.y = som_tool_tile.y-som_tool_tilespace.y-2; 
					rc.bottom = pt.y+13;
					//HACK: recentering mouse after it is released
					if(captured)
					{
						EX::x = pt.x+7; EX::y = pt.y+6; 
						if(workshop_mode)
						{
							EX::x+=workshop_picture_window.left;
							EX::y+=workshop_picture_window.top;
						}
					}
					
				}
				if(mode!=0)
				{
					extern HBRUSH som_tool_red,som_tool_yellow;					
					FrameRect(dis->hDC,&rc,som_tool_red);
					DrawFocusRect(dis->hDC,&rc);
					InflateRect(&rc,-1,-1);
					FrameRect(dis->hDC,&rc,som_tool_yellow);												
					DrawFocusRect(dis->hDC,&rc);
				}
			}

			SelectObject(som_tool_dc,so);

			return;
		}
		break;
	}
	//assert(0); //TODO: INVESTIAGE ObjEdit
}
extern void PrtsEdit_refresh_1022()
{
	if(SOM::tool==PrtsEdit.exe)
	if(HWND pw=GetDlgItem(som_tool,1022))
	InvalidateRect(pw,0,0);
}
static void ItemEdit_movesets_enable(WPARAM wp)
{
	//2021: extending 1031 to 4 10-bit indices
	BOOL e = wp>=1032;
	int limit = wp==1033||workshop_category!=1?2:0;	
	bool move = wp==1031;
	for(int i=1042;i>=1040;i--,move=false)
	{
		HWND hw = GetDlgItem(som_tool,i);
		HWND ch = GetWindow(hw,GW_CHILD);
		for(int j=0;j<3;j++)
		{
			EnableWindow(ch,move?1:j<limit?0:e);
			ch = GetWindow(ch,GW_HWNDNEXT);
		}
	}
}
static void workshop_model_update();				
static VOID CALLBACK workshop_model_update_timer(HWND win, UINT, UINT_PTR id, DWORD)
{
	KillTimer(win,id); workshop_model_update(); 
}
static bool workshop_WriteFile(const wchar_t*);
extern LRESULT CALLBACK workshop_subclassproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scID, DWORD_PTR hlp)
{			
	enum{ lr=(MK_LBUTTON|MK_RBUTTON) };

	switch(uMsg)
	{
	case WM_INITDIALOG:

		if(hWnd==som_tool)
		{
			workshop = new workshop_t;

			workshop_init_filter(1005);
			//height_adjustment SHOWS THESE... MAYBE THE
			//ADJUSTMENT SHOULD ACCOUNT FOR WINDOW MENUS
			int hide[] = {32771,40074,40075};			
			for(int i=0;i<EX_ARRAYSIZEOF(hide);i++)
			MoveWindow(GetDlgItem(som_tool,hide[i]),0,0,0,0,0);

			//this is in case the container is terminated 
			//without closing this workshop window
			if(workshop_tool)
			SetTimer(0,0,EX::debug?1000:5000,workshop_tool_timer);
			PostMessage(workshop_tool,WM_APP+'ws',(WPARAM)hWnd,0);
		}

		switch(hlp)
		{
		case 60000: //PtrsEdit

			PrtsEdit_INITDIALOG();			
			break;

		case 61000: //ItemEdit
		{
			char ch[] = {12,13,19,50,51,52,53,55,56};
			for(int i=0;i<EX_ARRAYSIZEOF(ch);i++)
			SendDlgItemMessage(hWnd,1000+ch[i],EM_LIMITTEXT,3,0);
			//VANITY: assuming original layouts with MS Gothic 9
			SendDlgItemMessage(hWnd,1057,EM_SETMARGINS,EC_LEFTMARGIN,0);
			break;
		}
		case 61100: //model change

			workshop_redraw(); break;
		
		case 62000: //ObjEdit
		{
			char ch[] = {41,61,62,64,65,67,68};
			for(int i=0;i<EX_ARRAYSIZEOF(ch);i++)
			SendDlgItemMessage(hWnd,1000+ch[i],EM_LIMITTEXT,3,0);
			break;
		}
		case 63000: case 64000: //Ene/NpcEdit

			workshop_init_filter(40074);
			//workshop_init_filter(40075);
			CheckDlgButton(hWnd,2999,1);
						
			//it's more responsive to load it in advance
			if(DLGTEMPLATE*locked=(DLGTEMPLATE*)LockResource
			(LoadResource(0,FindResourceA(0,"X_PAGE_2",(char*)RT_DIALOG))))
			{
				workshop_page = 1;
				EnableMenuItem(workshop_menu,30001,MF_GRAYED);

				//som_tool_CreateDialogIndirectParamA
				CreateDialogIndirectParamA(0,locked,som_tool,workshop_page2_init,0);
			}			
			break;

		case 65000: //SfxEdit

			SfxEdit_INITDIALOG();			
			break;
		}
		goto highlight_name; //Ene/NpcEdit

	case WM_APP+0: 

		workshop_minmax();				
		//REMOVE ME??
		//ObjEdit has funky rotation when a file is opened
		//by double-clicking it in the GetOpenFileName box
		//NOTE: one of those situations where only magical
		//timer works???
		SetTimer(hWnd,WM_LBUTTONDBLCLK,25,0);
		highlight_name:
		PostMessage(GetDlgItem(hWnd,1006),EM_SETSEL,0,-1);
		break;

	case WM_TIMER:
		
		switch(wParam)
		{
		case WM_LBUTTONDBLCLK:
		
			SendMessage(hWnd,wParam,0,0);
			break; //KillTimer

		case 'stnf':

			//when drag-dropping the focus may be unavailable
			//just defaulting always, for the sake of consistency 
			//if(!som_tool_neighbor_focusing)
			//som_tool_neighbor_focusing = GetDlgItem(hWnd,1006);
			//SetFocus(som_tool_neighbor_focusing);
			//som_tool_neighbor_focusing = 0;
			if(0!=workshop_mode)
			SetFocus(GetDlgItem(hWnd,SOM::tool==PrtsEdit.exe?1008:1006));

			//HACK: workshop_ReadFile_obj can't make this work
			if(SOM::tool==ObjEdit.exe)
			if(0==ObjEdit_476B00.data().clipmodel)
			{
				HWND hw = GetDlgItem(som_tool,1004);
				SetWindowText(hw,L""); InvalidateRect(hw,0,0);
			}

			break; //KillTimer

		default: //assert(0); //ItemEdit pops up infinite boxes???
			
			goto def; //KillTimer?
		}
		KillTimer(hWnd,wParam); return 0;

	/*case WM_SETTEXT:

		if(hWnd==som_tool)
		{
			if(som_tool_taskbar) 
			SendMessage(som_tool_taskbar,uMsg,wParam,lParam);
		}
		break;*/

	case WM_ERASEBKGND:

		if(hWnd!=som_tool)
		{
			if(hWnd==workshop_host) 
			return 0;
			break;
		}

		if(1==workshop_mode)
		break;

		//DrawText doesn't work here... see WM_PAINT
		if(hlp==60000) //PrtsEdit.exe
		{
			HWND pf = GetDlgItem(hWnd,1023);
			if(pf) pf = hWnd;
			RECT rc; GetClientRect(pf,&rc);
			MapWindowRect(pf,hWnd,&rc);
			InflateRect(&rc,-40,-60); //groupbox?

			lParam = DefSubclassProc(hWnd,uMsg,wParam,lParam);
			HDC &dc = (HDC&)wParam;
			extern HFONT som_tool_charset;
			SelectObject(dc,som_tool_charset);
			DrawText(dc,workshop->xicard.c_str(),-1,&rc,DT_CENTER|DT_NOPREFIX);			
			return lParam;
		}
		return 0;

	case WM_DRAWITEM:

		workshop_drawitem((DRAWITEMSTRUCT*)lParam);
		break;

	case WM_SYSCOMMAND: 

		switch(wParam)
		{	
		//NOTE: ObjEdit seems to go by this X button-wise
		case SC_CLOSE: 
			
			//PrtsEdit closes via IDCANCEL?
			if(SOM::tool==PrtsEdit.exe)
			uMsg = WM_CLOSE; 
			
			goto close;

		//case SC_RESTORE:; //assert(0);
		case SC_MAXIMIZE: case SC_MINIMIZE: ncdblclk:
			
			SetWindowStyle(hWnd,WS_MINIMIZEBOX|WS_MAXIMIZEBOX,
			wParam==SC_MAXIMIZE?WS_MINIMIZEBOX:WS_MAXIMIZEBOX);
			workshop_minmax();
			return 0;
		}
		break;

	case WM_NCLBUTTONDOWN: //ICONIFY (COPIED FROM som_tool_subclassproc)
	{
		if(som_tool_taskbar&&wParam==HTCAPTION)
		{	
			//2018: seems to work to minimize from a popup window
			//if(hWnd!=som_tool) break;
	
			//POINT pt; POINTSTOPOINT(pt,(POINTS&)lParam);					
			//if(DragDetect(hWnd,pt)) break;
			if(0!=SOM::Tool::dragdetect(hWnd,WM_NCLBUTTONDBLCLK)) 
			break;
			//WARNING! CODE TAILORED TO DragDetect MAY
			//NOT WORK THE SAME WITH SOM::Tool::dragdetect
			if(hWnd!=GetAncestor(GetFocus(),GA_ROOT))
			{
				//NCLBUTTONDOWN is being echoed back??
				SetForegroundWindow(som_tool); 
			}
			else SendMessage(som_tool_taskbar,WM_SYSCOMMAND,SC_MINIMIZE,0);
			return 0; 
		}
		break;
	}
	case WM_NCLBUTTONDBLCLK: 

		//NOTE: Windows expands, bypassing WM_SYSCOMMAND 
		//on double click, necessitating this
		//TODO: minimize on click like som_tool_subclass 
		if(hWnd!=som_tool) break;
		wParam = workshop_mode?SC_MAXIMIZE:SC_MINIMIZE;
		goto ncdblclk;

	case WM_WINDOWPOSCHANGING:

		if(((WINDOWPOS*)lParam)->flags&SWP_SHOWWINDOW)
		{
		showing:

			//TODO: WOULD REALLY LIKE TO WAIT UNTIL THE FIRST
			//MODEL IS LOADED/DISPLAYED BUT I CAN'T GET IT TO
			//WORK WITHOUT ISSUE

			if(som_tool!=hWnd) break;

			//HACK? It's simpler to take the splash screen down
			//first, otherwise the tools appear behind it while
			//it is being displayed			
			SOM::splashed(0,0);
						
			workshop_minmax();

			if(som_tool_taskbar)
			{
			//	WCHAR text[30] = L""; //chkstk?
			//	GetWindowTextW(hWnd,text,EX_ARRAYSIZEOF(text));
			//	SetWindowTextW(som_tool_taskbar,text);
				ShowWindow(som_tool_taskbar,1);
			}
			else if(workshop_tool)
			{			
				static bool one_off = false; if(!one_off)
				{
					one_off = true; //HACK: leave in user reposition 

					//HACK: added 'nc' just for SOM_MAP since the
					//window opens up in a nice place by accident
					//but not nice if centered on the client area
					extern void som_tool_recenter(HWND,HWND,int);
					som_tool_recenter(hWnd,workshop_tool,'nc');
				}
			}
		} 
		break;

	case WM_SHOWWINDOW:
				
		if(wParam) if(hWnd==som_tool)
		{	
			goto showing; //SWP_SHOWWINDOW???			
		}
		break;
				
	case WM_COMMAND:
		 
		//HACK: accelerators have HIWORD set 
		//to 1... ItemEdit/ObjEdit translate
		//it somehow
		if(LOWORD(wParam)>30000)
		wParam = LOWORD(wParam);

		switch(wParam)
		{
		case IDOK: //VK_RETURN?

			//workshop_model_update?
			if(som_tool==hWnd) 
			{
				//emulating SOM_PRM
				if(workshop_tool&&0==workshop_mode)
				{
					if(GetKeyState(VK_RETURN)>>15) //PrtsEdit
					goto close;
				}

				lParam = (LPARAM)GetFocus();
				if(1005==GetDlgCtrlID((HWND)lParam))
				{	
					goto ok;
					case MAKEWPARAM(1005,EN_KILLFOCUS):
					if(som_tool!=hWnd
					||!Edit_GetModify((HWND)lParam))
					break;
					ok:	Edit_SetModify((HWND)lParam,0);
					GetWindowText((HWND)lParam,som_tool_text,MAX_PATH);

					if(hlp==61000 //ItemEdit					
					&&ES_NUMBER&GetWindowStyle((HWND)lParam))
					{
						//2021: communicate animation ID limit
						wchar_t *e; int l =
						wcstol(som_tool_text,&e,10);
						if(*e||l>65535) 
						SetDlgItemInt(hWnd,1005,65535,0);
					}
					else workshop_model_update();

					if(hlp!=61000) //ItemEdit		
					return 0;
				}
			}

			//simulate BS_DEFPUSHBUTTON?
			//(without its distracting outline)
			switch(hlp)
			{
			case 61000: case 65000: //ItemEdit/SfxEdit
			
				SendMessage(hWnd,WM_COMMAND,1018,0);							
				break;

			case 60000: //PrtsEdit
			
				refresh_1021:
				InvalidateRect(GetDlgItem(hWnd,1021),0,0);
				//PrtsEdit closes on IDOK
				return 0; 
			}			
			break;

		case IDCANCEL: 
			
			if(som_tool==hWnd)
			{
				if(workshop_tool)
				goto close;
				//PrtsEdit.exe wants to close out
				//note: this breaks WM_CLOSE somehow
				//but it is handled
				return 0; 
			}
			break;

		case 1014: //case 1015: case 1016:
			if(hlp!=60000) 
			break;
		case 1011: file_system: //PtrsEdit 		

			if(!som_tool_initializing)
			{
				int id = 1005;
				if(wParam==1015) id++; 
				if(wParam==1016) id = 1013;
				workshop_file_system(id);
				return 0;
			}
			break;

		case MAKEWPARAM(1011,EN_CHANGE): 
			PrtsEdit_refresh_1022();
			break;			
		case MAKEWPARAM(1013,EN_KILLFOCUS): 
			if(hlp==60000) //PrtsEdit 
			goto refresh_1021;
			//break;
		case MAKEWPARAM(1012,EN_KILLFOCUS): 
		case MAKEWPARAM(1019,EN_KILLFOCUS): 
		case MAKEWPARAM(1050,EN_KILLFOCUS):
		case MAKEWPARAM(1051,EN_KILLFOCUS):
		case MAKEWPARAM(1052,EN_KILLFOCUS): 
		case MAKEWPARAM(1053,EN_KILLFOCUS): 
		case MAKEWPARAM(1055,EN_KILLFOCUS): 
		case MAKEWPARAM(1056,EN_KILLFOCUS): 
			
			if(hlp==61000) //ItemEdit
			{
				at_most_255:
				WORD lw = LOWORD(wParam);
				int most,i = GetDlgItemInt(hWnd,lw,0,0);
				switch(lw)
				{
				//these 2 have error messages
				case 1012: case 1013: 
				case 1055: most = 360; break;
				default: most = 255;
				}
				if(i>most) SetDlgItemInt(hWnd,lw,most,0);
			}
			break;

		case MAKEWPARAM(1041,EN_KILLFOCUS): 		
		case MAKEWPARAM(1062,EN_KILLFOCUS): 
		case MAKEWPARAM(1065,EN_KILLFOCUS): 
		case MAKEWPARAM(1068,EN_KILLFOCUS): 

			if(hlp==62000||hlp==63000) //ObjEdit/EneEdit (page 2)
			{
				goto at_most_255;
			}
			break;

		case MAKEWPARAM(1061,EN_KILLFOCUS): 
		case MAKEWPARAM(1064,EN_KILLFOCUS): 
		case MAKEWPARAM(1067,EN_KILLFOCUS): 

			if(hlp==62000) //ObjEdit
			{
				workshop_limitpitch(LOWORD(wParam));
			}
			break;

		case 1015: case 1016:
			if(hlp==60000) goto file_system; //PtrsEdit?
		case 1000: 

			if(hlp==61000) //ItemEdit
			if(!lParam||Button_GetCheck((HWND)lParam)) //WINSANITY
			{
				workshop_mode2 = wParam-1000;

				//hit the U button and let som_tool_GetWindowTextA hide the
				//values... should be safe since nothing else works!!!
				SendMessage(som_tool,WM_COMMAND,1018,0);

				//??? doing after makes the take/menu view synchronize itself
				//(its unclear if it's just retaining the drop view's state or
				//if it doesn't work identically to the player)
				//workshop_minmax();
				SendMessage(hWnd,WM_LBUTTONDBLCLK,0,0);
				workshop_minmax();

				//SECRET BUTTON?
				//WTH? Seems to toggle Save menu item's enable/disable state
				//as a side effect
				if(1000==wParam) return 0;
			}
			break;

		case 1018: //HACK

			if(hlp==61000) //ItemEdit
			{
				workshop_redraw();
			}
			break;

		case 1007: case 1008: case 1009: case 1010: 
		case 1030: case 1031: case 1032: case 1033: 

			if(hlp==61000) //ItemEdit
			{	  
				if(lParam&&!Button_GetCheck((HWND)lParam)) //WINSANITY
				break;
				
				if(wParam>=1030&&wParam<=1033) //2021
				{
					//NOTE: wParam IS OVERRIDDEN BELOW

					windowsex_enable(hWnd,1011,1013,wParam!=1031);
					HWND mdo = GetDlgItem(hWnd,1005);
					SetWindowStyle(mdo,ES_NUMBER|ES_CENTER,wParam==1031?~0:0);
					InvalidateRect(mdo,0,1); //doesn't reflect ES_CENTER
					for(int i=1;i<=4;i<<=1)
					EnableMenuItem(workshop_menu,34000+i,wParam==1031?MF_GRAYED:0);
				}
				else if(wParam<=1010) //!!
				{
					switch(wParam) //wise?
					{
					case 1007: workshop_category = 1; break;
					case 1008:
					case 1009: workshop_category = 2; break;
					case 1010: workshop_category = 0; break;
					}
					windowsex_enable<1019>(hWnd,wParam==1010);
					windowsex_enable(hWnd,1020,1026,wParam==1009);
					windowsex_enable(hWnd,1030,1033,wParam!=1010);

					//ATTENTION
					wParam = GetDlgRadioID(hWnd,1030,1033); //!!
				}
				windowsex_enable(hWnd,1040,1042,1==workshop_category||wParam>=1031);
				ItemEdit_movesets_enable(wParam);
				lParam = wParam==1030?1==workshop_category:wParam==1031;
				if(lParam!=IsWindowEnabled(GetDlgItem(hWnd,1050)))
				{
					EX::vblank(); //flickers badly
					windowsex_enable(hWnd,1050,1056,lParam);						
					RedrawWindow(hWnd,0,0,RDW_UPDATENOW|RDW_ALLCHILDREN);	
				}
				return 0;
			}
			break;

		case 1080: //UV animation

			if(hlp==62000) //ObjEdit 
			{
				assert(lParam);
				windowsex_enable(som_tool,1081,1082,Button_GetCheck((HWND)lParam));
			}
			break;

		case 2031: case 2041: //ObjEdit receptacle?

			//WINSANITY
			//som_tool_checkboxproc is handling this radio relationship
			//so that this doesn't have to be an exhaustive list of IDs
			break;

		case 3000: case 3004: case 3005: //UNIMPLEMENTED 

			//these buttons are labels that play an animation
			MessageBeep(-1); Button_SetCheck((HWND)lParam,0);
			break;

		case 35005:

			PrtsEdit_Pen_Mode();
			break;

		case 36000: case 36001: case 36002:

			PrtsEdit_SoftMSM(wParam);
			break;

		case 32767: //Move

			MessageBeep(-1); //UNIMPLEMENTED
			break;

		case 32768: workshop_icard_32768(); 
			break;

		//disable ObjEdit's save prompt?
		case 32772:
				
			/*OBSOLETE?
			//clean up picture window after New option
			SetTimer(hWnd,WM_LBUTTONDBLCLK,25,0);
			//break;*/
			workshop_32772();
			return 0;
		
		case 32771: case 32778: ax_save_prompt:

			if(SOM::tool==PrtsEdit.exe)
			{
				//NOTE: WM_CLOSE was geneating IDCANCEL?
				//and that was closing PrtsEdit... but 
				//Esc should not close it
				if(wParam==32778||uMsg==WM_CLOSE)
				{
					wParam = 1012; uMsg = WM_COMMAND;
				}
				/*OBSOLETE?
				else if(wParam==32772)
				PrtsEdit_New();*/
				if(wParam==32771
				&&PrtsEdit_Open_or_Save_chkstk(0))
				wParam = 1010; 				
			}
			if(SOM::tool==ObjEdit.exe)
			ObjEdit_476B00.save() = 0;
			//ItemEdit does have a save prompt... there's
			//just no rhyme or reason to it... in fact it
			//seems to appear only when nothing's in need
			//of saving
			if(SOM::tool==ItemEdit.exe)
			{
				ItemEdit_471ab0.save() = 0;
				/*OBSOLETE?
				if(wParam==32772) //New (nonfunctioning?)
				{
					ItemEdit_New(); return 0;
				}*/
			}
			break;
		
		case 32773: //don't involve tool

			workshop_WriteFile(workshop_savename);
			return 0;
		
		case 32774: 

			if(SOM::tool==PrtsEdit.exe)
			{
				//if(wParam==32774
				//&&!PrtsEdit_Open_or_Save_chkstk(1))
				//break;
				//wParam = 1009;
				PrtsEdit_Open_or_Save_chkstk(1);
				return 0;
			}
			break;

		case 34001: case 34002: case 34004: 

			workshop_menu_COMMAND(wParam);
			break;
		}
		break;

	case WM_LBUTTONDBLCLK:
	
		workshop_redraw();
		if(workshop_mode2==16) //1016?
		if(hlp==61000) //ItemEdit
		{						
			//HMMM??? When clicking over to (2) the view tilts at an angle that is hard
			//to say if it's meant to be stylish, but the effect is lost if so since the
			//grid is flattened to a single line (it might look nice from a higher vantage)
			//trouble is because of how rotation is (different from SOM_PRM) there's no way 
			//to lift the back corner up... so better to just use the straightforward vantage
			DefSubclassProc(hWnd,uMsg,wParam,lParam);
			DefSubclassProc(hWnd,WM_LBUTTONDOWN,MK_LBUTTON,0);
			DefSubclassProc(hWnd,WM_MOUSEMOVE,MK_LBUTTON,50<<16);
			DefSubclassProc(hWnd,WM_LBUTTONUP,MK_LBUTTON,50<<16);			
			return 0;
		}
		break;

	//TODO? would prefer a single click (no drag) to double
	case WM_RBUTTONDBLCLK:

		if(workshop_choosecolor())
		workshop_redraw();
		return 0;

	case WM_MBUTTONDOWN: //2021: mimicking SOM_MAP_scroll3D
		
		wParam = 0; //break;
			
	case WM_MOUSEWHEEL:
		
		if(hWnd==som_tool||hWnd==workshop_host)
		workshop_MOUSEWHEEL(GET_WHEEL_DELTA_WPARAM(wParam));
		return 0;
	
	case WM_LBUTTONDOWN:
	
		if(hlp==60000) //PrtsEdit
		goto PrtsEdit_3x3; 
		//break;
	
	case WM_RBUTTONDOWN: //case WM_LBUTTONDOWN:
	{
		if(hWnd!=workshop_host)
		break;

		if(0==workshop_mode) //maximized?
		{	
			if(hlp==60000) break; //PrtsEdit

C4533:		workshop_cursor = lParam; 

			if(15==workshop_mode2&&workshop_mode2_15_submenu)		
			{
				assert(ItemEdit.exe==SOM::tool);
				if(WM_LBUTTONDOWN==uMsg||WM_RBUTTONDOWN==uMsg)
				{
					//hack: leave background color menu in place
					wParam = WM_LBUTTONDOWN==uMsg?uMsg:WM_RBUTTONDBLCLK;
					if(2!=SOM::Tool::dragdetect(hWnd,wParam))
					workshop_submenu(workshop_mode2_15_submenu);
				}
				return 0;
			}

			if(uMsg!=WM_MOUSEMOVE) //HACK (PrtsEdit)
			{
				if(hWnd!=SetCapture(hWnd))				
				if(hlp==60000) //PrtsEdit
				{
					ShowCursor(0);
					EX::x = 1+GET_X_LPARAM(lParam); //HACK
					EX::y = GET_Y_LPARAM(lParam); //HACK						
					POINTS &pts = (POINTS&)workshop_cursor;
					if(workshop_mode)
					{
						pts.x-=workshop_picture_window.left;
						pts.y-=workshop_picture_window.top;
					}
				}
			}
			else PrtsEdit_refresh_1022();
			return 0;
		}		
PrtsEdit_3x3:
		POINT pt = {GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};		
		if(som_tool!=workshop_host)
		{
			pt.x+=workshop_picture_window.left;
			pt.y+=workshop_picture_window.top;
		}
		if(PtInRect(&workshop_picture_window,pt))
		{
			if(hlp==60000) //PrtsEdit
			{
				if(1==workshop_mode2!=GetKeyState(VK_CONTROL)<0) //Pen Mode On? 
				{
					PrtsEdit_refresh_1022();					
				}
				else if(uMsg==WM_LBUTTONDOWN)
				{
					int i = som_tool_tile.y/9*20+som_tool_tile.x/9;
					if(i>=255) i = 255;				
					HWND ec = GetDlgItem(som_tool,1011);
					SetDlgItemInt(som_tool,1011,i,0);
					SendMessage(ec,EM_SETSEL,0,-1);
					SetFocus(ec);
					return 0;
				}
			}
			goto C4533; //warning
		}
		else if(uMsg==WM_MOUSEMOVE) //HACK (PrtsEdit)
		{
			//REMINGER: this test is stopping recursion within this procedure call
			if(lParam!=workshop_cursor)
			{
				lParam = workshop_cursor;
				goto PrtsEdit_3x3;
			}
		}
		return 0;
	}
	case WM_LBUTTONUP:
	
		if(hlp==60000) //PrtsEdit
		{		
			ReleaseCapture(); return 0;
		}
		//break;

	case WM_RBUTTONUP:

		if(lr&~wParam) 
		ReleaseCapture(); return 0;

	case WM_CAPTURECHANGED:

		if(hlp==60000) //PrtsEdit
		{
			if(!lParam&&EX::x)
			{
				EX::cursor = true;
				RECT wr; GetClientRect(hWnd,&wr);
				MapWindowRect(hWnd,0,&wr);
				SetCursorPos(wr.left+EX::x-1,wr.top+EX::y); //HACK
				EX::x = 0; //HACK
				ShowCursor(1);
			}
			PrtsEdit_refresh_1022();
		}
		break;

	case WM_MOUSEMOVE:
	
		if(hWnd!=GetCapture())
		{
			if(hlp==60000) //PrtsEdit
			goto PrtsEdit_3x3;
		}
		else if(wParam&lr)
		{	
			if(hlp==60000) //PrtsEdit
			{
				PrtsEdit_refresh_1022();
				break;
			}

			LPARAM &c = workshop_cursor;
			RECT &pw =  workshop_picture_window;
			union{ SHORT d[2]; LPARAM dlp; };
			union{ SHORT r[2]; LPARAM ref; };
			d[0] = GET_X_LPARAM(lParam)-GET_X_LPARAM(c); 
			d[1] = GET_Y_LPARAM(lParam)-GET_Y_LPARAM(c); 
			//WTH? WM_MOUSEMOVE is being sent while dragging
			//the buttons, moving or not
			if(!d[0]&&!d[1])
			{
				return 0; //avoid workshop_redraw
			}
			c = lParam;
													
			if(wParam&MK_CONTROL)
			{
				//todo? GetMouseMovePointsEx
				d[abs(d[0])>abs(d[1])] = 0;
			}

			//HACK: zoom is insensitive
			if(lr==(lr&wParam)) d[1]*=3;

			//there is discontinuity crossing 0/0
			RECT cr; GetClientRect(hWnd,&cr);
			if(d[0]<0)
			{
				r[0] = cr.right; d[0]+=cr.right;
			}
			else{ r[0] = cr.left; d[0]+=cr.left; }
			if(d[1]<0)
			{	
				r[1] = cr.bottom; d[1]+=cr.bottom;
			}
			else{ r[1] = cr.top; d[1]+=cr.top; }
						   
			if(wParam&MK_LBUTTON)
			DefSubclassProc(hWnd,WM_LBUTTONDOWN,wParam,ref);
			if(wParam&MK_RBUTTON)
			DefSubclassProc(hWnd,WM_RBUTTONDOWN,wParam,ref);
			DefSubclassProc(hWnd,WM_MOUSEMOVE,wParam,ref);
			DefSubclassProc(hWnd,WM_MOUSEMOVE,wParam,dlp);
			if(wParam&MK_LBUTTON)
			DefSubclassProc(hWnd,WM_LBUTTONUP,0,dlp);
			if(wParam&MK_RBUTTON)
			DefSubclassProc(hWnd,WM_RBUTTONUP,0,dlp);

			//HACK: want to stop on crisp afterimage
			//NOTE: workshop_redraw should just work 
			//cannot depend on InvalidateRect timing 
			if(SOM::tool!=ItemEdit.exe)
			workshop_antialias = 1;
			workshop_redraw();
		}
		return 0;
	
	case WM_DROPFILES: 

		//REMINDER! EneEdit/NpcEdit processes WM_DROPFILES
		workshop_DROPFILES(wParam); return 0;

	case WM_CLOSE: close: 
		
		if(hWnd==som_tool)
		{
			if(workshop_tool&&IsWindow(workshop_tool))
			{
				EX::vblank(); //better?

				assert(!som_tool_taskbar);
				EnableWindow(workshop_tool,1); //1.
				ShowWindow/*Async*/(hWnd,SW_HIDE); //2.

				//once the windows were mutually disabled (!) on
				//clicking the X button
				EX::sleep(20); 
				//without this the taskbar is forcing the window
				//to be shown
				EnableWindow(hWnd,0);
				
				return 0;
			}
			
			//not working from taskbar with Item/ObjEdit???
			//(why did it take so long for me to notice???)
			//goto ax_save_prompt;
			return SendMessage(hWnd,WM_COMMAND,32778,0);
		}
		break;		  

	case WM_DESTROY: //som_tool_workshop_exe support

		//som_tool_workshop_exe is using WM_DESTROY to end its
		//session. I don't know if doing so destroys the process
		//but, DestroyWindow only works on the window's own thread
		//(NOTE: WM_CLOSE is hiding, not closing with workshop_tool)
		if(hWnd==som_tool) PostQuitMessage(0);
		break;

	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,workshop_subclassproc,scID);		
		break;
	}

def: return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}

static windowsex_DIALOGPROC workshop_103_104;
static bool workshop_null_and_void(HWND a, WCHAR *w, LPCWSTR nul)
{
	if(a==som_tool) return wcscmp(w,nul); if(!*w) wcscpy(w,nul); return true;
}
extern INT_PTR CALLBACK workshop_102c_ext(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{	
	case WM_INITDIALOG:
	{
		if(lParam) //lParam is 1, 2, or 3
		{
			extern void som_tool_recenter(HWND,HWND,int);
			som_tool_recenter(hwndDlg,workshop_host,0);
			RECT rc;
			GetWindowRect(som_tool,&rc);
			int bt = rc.bottom;
			GetWindowRect(hwndDlg,&rc);
			OffsetRect(&rc,0,bt-rc.bottom);
			SetWindowPos(hwndDlg,som_tool,rc.left,rc.top,0,0,SWP_NOSIZE);
			SfxEdit_INITDIALOG_right(hwndDlg);
		}
		HWND a = lParam?som_tool:hwndDlg;
		HWND b = lParam?hwndDlg:som_tool;
		int i; for(i=1057;i>=1040;i--) 
		{			
			l000: 
			WCHAR w[16]; LPCWSTR x=0;
			GetDlgItemText(a,i,w,16);			
			switch(i)
			{
			case 1048: 
			case 1049: x = L"65535"; break;
			case 1056: x = L"-1"; break;
			case 1057: x = L"0"; break;
			}
			if(x) if(a==som_tool) 
			{
				if(!wcscmp(w,x)) continue;
			}
			else if(!*w) wcscpy(w,x);
			SetDlgItemText(b,i,w);
		}
		if(i!=999){ i = 1000; goto l000; }
		CheckDlgButton(b,1059,IsDlgButtonChecked(a,1059));
		if(!lParam)
		{
			windowsex_notify(som_tool,1000,CBN_EDITCHANGE);
			windowsex_notify(som_tool,1040,EN_CHANGE);
			windowsex_notify(som_tool,1041,CBN_EDITCHANGE);
			windowsex_notify(som_tool,1059);
		}
		return 1;
	}
	case WM_COMMAND:
		
		switch(wParam)
		{
		case IDOK: case IDCANCEL: goto close;
		}
		break;
		   
	case WM_CLOSE: close: 
		
		SendMessage(hwndDlg,WM_INITDIALOG,0,0);
		DestroyWindow(hwndDlg);
		break;
	} 
	return 0;
}
static void SfxEdit_1018()
{
	if(!IsWindowEnabled(GetDlgItem(som_tool,1018))) //IDOK?
	return;

	workshop_cpp::sfx_record r,w;	
	w.procedure = GetDlgItemInt(som_tool,1041,0,0);
	w.model = GetDlgItemInt(som_tool,1040,0,0);
	for(int i=0;i<6;i++)
	w.unknown6[i] = GetDlgItemInt(som_tool,1042+i,0,0);
	for(int i=0;i<8;i++)
	w.floats8[i] = GetDlgItemFloat(som_tool,1048+i);
	w.sound2 = GetDlgItemInt(som_tool,1056,0,1);
	w.pitch24 = GetDlgItemInt(som_tool,1057,0,1);
	if(w.sound2>0) w.pitch24+=24;
	w.extend1 = GetDlgItemInt(som_tool,1058,0,1);
	for(int i=0;i<3;i++) w.zero3[i] = 0;

	DWORD rw;
	int fp = 48*GetDlgItemInt(som_tool,1000,0,1);
	HANDLE h = SfxEdit_1000(0,GENERIC_WRITE);
	if(fp==SetFilePointer(h,fp,0,FILE_BEGIN)
	&&ReadFile(h,&r,48,&rw,0)&&rw==48
	&&fp==SetFilePointer(h,fp,0,FILE_BEGIN))
	{	
		if(!WriteFile(h,&w,48,&rw,0)||rw!=48)
		rw = 0;
	}
	else rw = 0; 
	//if(!rw) MessageBeep(-1); //MessageBox
	if(!rw) EX::ok_generic_failure(L"Failed to update Sfx.dat file."); //2022
	CloseHandle(h); 
}
extern INT_PTR CALLBACK workshop_102(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{	
	//REMINDER: ONLY INCLUDES EneEdit/NpcEdit SINCE THEY DO NOT ORIGINALLY
	//HAVE 102 DIALOGS
	switch(uMsg)
	{	
	case WM_MOUSEMOVE:

		if(wParam&(MK_LBUTTON|MK_RBUTTON))
		workshop_redraw();
		break;

	case WM_MOUSEWHEEL:
		
		workshop_MOUSEWHEEL(GET_WHEEL_DELTA_WPARAM(wParam));
		return 0;

	case WM_DROPFILES: 
		
		workshop_DROPFILES(wParam); return 0;

	case WM_VSCROLL:

		assert(SOM::tool==SfxEdit.exe);
		if(LOWORD(wParam)==SB_THUMBTRACK)
		{				
			EX::vblank(); wParam>>=16;
			windowsex_enable<1018>(hwndDlg,0!=wParam);
			windowsex_enable(hwndDlg,1040,1060,1!=wParam);					
			RedrawWindow(hwndDlg,0,0,RDW_UPDATENOW|RDW_ALLCHILDREN);
			return 0;
		}
		break;

	case WM_COMMAND:

		switch(HIWORD(wParam))
		{
		case CBN_SELENDOK:

			//if(SOM::tool>=EneEdit.exe)
			if(LOWORD(wParam)>=4000)
			wParam = LOWORD(wParam)-4000+2000;
			break;
		}

		switch(wParam)
		{
		default: 			

			//Reminder: this procedure is Ene/NpcEdit only
			if(wParam>=5000&&wParam<=5999)
			{			
				if(SOM::tool==EneEdit.exe)
				{
					if(wParam>=5991)
					{
						int f = 1<<wParam-5991;
						if(Button_GetCheck((HWND)lParam))
						EneEdit_4173DC->countermeasures&=~f;		
						else EneEdit_4173DC->countermeasures|=f;
					}
					else
					{
						int f = 1<<wParam-5000;
						if(Button_GetCheck((HWND)lParam))
						EneEdit_4173DC->turning_table&=~f;		
						else EneEdit_4173DC->turning_table|=f;
						break;
					}
				}
				break;
			}

			if(wParam<2000||wParam>2031)
			break; //break;
		case 1015: //FALLING THRU
		{
			HRSRC found = FindResource(0,MAKEINTRESOURCE(wParam==1015?104:103),RT_DIALOG);
			if(DLGTEMPLATE*locked=(DLGTEMPLATE*)LockResource(LoadResource(0,found)))
			{
				//som_tool_CreateDialogIndirectParamA
				CreateDialogIndirectParamA(0,locked,som_tool,workshop_103_104,wParam);
				return 0;
			}
			else assert(locked); break;
		}
		case 1018: //IDOK?

			if(SfxEdit.exe==SOM::tool) SfxEdit_1018();
			break;

		case MAKEWPARAM(1000,CBN_SELENDOK):
		case MAKEWPARAM(1000,C8N_SELCHANGE):
		case MAKEWPARAM(1000,CBN_EDITCHANGE):
		case MAKEWPARAM(1000,CBN_SELENDCANCEL):

			if(SOM::tool==SfxEdit.exe)
			{
				//this is locked mode for copying SFX
				if(!*som_tool_text //HACK
				&&1==SendDlgItemMessage(som_tool,1019,TBM_GETPOS,0,0))
				break;

				//Note: the text box is unchanged by list based
				//selection, however some delay is helpful also
				if(lParam) //SfxEdit_1000(wParam,(HWND)lParam);
				SetTimer((HWND)lParam,0,//500
				-1==GetWindowLong((HWND)lParam,GWL_USERDATA)?0:
				wParam==MAKEWPARAM(1000,CBN_EDITCHANGE)?500:250
				,SfxEdit_1000_timer);
				else assert(0);				
			}
			break;

		case 1002: case 1003:
		{
			if(SOM::tool==SfxEdit.exe
			&&Button_GetCheck((HWND)lParam))
			{
				BYTE se = wParam==1002; //bool
				windowsex_enable<1001>(hwndDlg,se);				
				if(se!=EneEdit_4173DC->my_screeneffect())
				{
					EneEdit_4173DC->my_screeneffect() = se;
					wParam = wParam==1002?1001:1040;
					lParam = (LPARAM)GetDlgItem(hwndDlg,wParam);
					goto SfxEdit_1002_1003_continued;
				}
			}
			break;
		}			
		case MAKEWPARAM(1001,EN_CHANGE):
		case MAKEWPARAM(1040,EN_CHANGE):
		
			if(SOM::tool==SfxEdit.exe
			&&(LOWORD(wParam)==1001)==
			EneEdit_4173DC->my_screeneffect())
			{
				SfxEdit_1002_1003_continued:
				if(workshop_reading) //HACK?
				break;
				WCHAR w[10];
				GetWindowText((HWND)lParam,w,8);
				*w = _wtoi(w);
				HWND ch = GetDlgItem(som_tool,1005);
				GetWindowText(ch,w+1,5);
				if(LOWORD(wParam)!=1001) 
				{
					if(*w>255) 
					return SetWindowText((HWND)lParam,L"255");
					if(wParam!=1040&&*w==_wtoi(w+1))
					break;
					EneEdit_4173DC->my_sfx() = *w;
				}
				else if(wParam==1001
				||*w!=EneEdit_4173DC->my_sfx())
				{
					EneEdit_4173DC->my_sfx() = *w;
					switch(*w)
					{
					default: *w = *w<12?207:0; break;
					case 12:
					case 13:
					case 14: *w = 208+(*w-12); break;
					}
					if(!*w) 
					return SetWindowText((HWND)lParam,L"14");
				}
				else break;
					
				swprintf(w,L"%04d.mdl",*w);
				SetWindowText(ch,w);
				//2021: give a little time to input more than one digit
				//workshop_model_update();
				SetTimer(ch,'sfx',300,workshop_model_update_timer);
			}
			break;

		case MAKEWPARAM(1058,EN_CHANGE):
		case 1059:

			if(SOM::tool==SfxEdit.exe)
			if(wParam==1059) 
			SetDlgItemInt(hwndDlg,1058,Button_GetCheck((HWND)lParam),0);
			else CheckDlgButton(hwndDlg,1059,GetDlgItemInt(hwndDlg,1058,0,0)?1:0);
			break;

		case MAKEWPARAM(1041,CBN_SELENDOK):		
			PostMessage(hwndDlg,WM_COMMAND,MAKEWPARAM(1041,CBN_EDITCHANGE),0);
			break;
		case MAKEWPARAM(1041,CBN_EDITCHANGE):
		case 1060:
		{
			if(SOM::tool!=SfxEdit.exe)
			break;

			HWND cb = GetDlgItem(hwndDlg,1041);
			WCHAR w[4]; GetWindowText(cb,w,4);
			if('-'==w[0])
			{
				ComboBox_SetCurSel(cb,1+ComboBox_GetCurSel(cb));
				break;
			}
			int i = _wtoi(w);
			if(i>255)
			{
				SetWindowText(cb,L"255");
				break;
			}
			if(wParam!=1060)
			break;
				  
			//TODO: change to X_102C_2 and X_102C_3 according
			//to i, falling back on X_102C_1 if doesn't exist
			char rsrc[] = "X_102C_1";
			if(DLGTEMPLATE*locked=(DLGTEMPLATE*)LockResource
			(LoadResource(0,FindResourceA(0,rsrc,(char*)RT_DIALOG))))
			{
				//som_tool_CreateDialogIndirectParamA
				CreateDialogIndirectParamA(0,locked,som_tool,workshop_102c_ext,rsrc[7]);
				return 0;			
			}
			break;
		}
		case MAKEWPARAM(1041,EN_KILLFOCUS): //inclination 
		case MAKEWPARAM(1042,EN_KILLFOCUS): 
		case MAKEWPARAM(1044,EN_KILLFOCUS): //defend_window
		case MAKEWPARAM(1045,EN_KILLFOCUS): 
		case MAKEWPARAM(1046,EN_KILLFOCUS): //turning_cycle
		case MAKEWPARAM(1047,EN_KILLFOCUS): 
		case MAKEWPARAM(1048,EN_KILLFOCUS):
		case MAKEWPARAM(1049,EN_KILLFOCUS):
		{
			if(SOM::tool==EneEdit.exe) //page 2
			{
				int i = GetDlgItemInt(som_tool,LOWORD(wParam),0,0);
				if(i>255) SetWindowText((HWND)lParam,L"255");
			}
			break;
		}

			////MENUS ////MENUS ////MENUS ////MENUS
				  
		//standardizing (to Item/ObjEdit's codes)
		case 32771: wParam = 40001; goto forward;
		//case 32772: assert(0); break; //New?
		//ctrl+s?
		//case 32773: wParam = 40072; goto forward;
		case 32773: case 40072: assert(0); return 0; 
		case 32774: wParam = 40073; goto forward;
		case 32778: wParam = 40023; goto forward;
		//Ene/NpcEdit
		case 40074:	//name/skin
		workshop_file_system(40074);
		case 40075: //texture export utility	
		//case 40072: //save
		case 40073: //save-as
		case 40001: //open		
		case 40023: //exit				
			forward:
			SendMessage(workshop_host,uMsg,wParam,lParam);
			break;
				
		case IDOK: //HACK
			if(SOM::tool!=EneEdit.exe)
			break;
		case 30000: click_bg: //next page?
			wParam = workshop_page==2?30001:30002; //break; 
		case 30001: case 30002: //page 1/2?

			if(workshop_page2)
			{
				workshop_page = wParam-30000; workshop_minmax();

				EnableMenuItem(workshop_menu,wParam,MF_GRAYED);	
				EnableMenuItem(workshop_menu,wParam==30001?30002:30001,0);
			}
		}
		break;

	case WM_LBUTTONDOWN: //click_bg?

		if(workshop_page2)
		{
			POINT pt = {GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
			HWND ch = ChildWindowFromPointEx(som_tool,pt,CWP_SKIPINVISIBLE);
			if(som_tool==ch)
			{
				wParam = 30000; goto click_bg;
			}
			else if(ch&&BS_GROUPBOX==(BS_TYPEMASK&GetWindowStyle(ch))) //YUCK
			{
				extern int som_tool_charset_height;				
				pt.y+=som_tool_charset_height/2;
				if(som_tool==ChildWindowFromPointEx(som_tool,pt,CWP_SKIPINVISIBLE))
				{
					wParam = 30000; goto click_bg;
				}
			}
		}
		break;

	case WM_CLOSE:

		assert(!workshop_tool); //hide?

		//Note: nothing else really works... probably need to post
		PostQuitMessage(0);	break;
	}

	return 0;
}
	
static void workshop_103_select(HWND hw, HWND lv, int i)
{
	LVITEMW lvi = {LVIF_TEXT,i}; 
	lvi.cchTextMax = 64;
	wchar_t text[64]; lvi.pszText = text; 
	for(int &i=lvi.iSubItem;i<6;i++)
	{
		if(!ListView_GetItem(lv,&lvi)) 
		*text = '\0';
		char ch[6] = {20,21,24,25,28,29 };
		if(i==1)
		CheckDlgButton(hw,1021,*text?1:0);
		else SetDlgItemText(hw,1000+ch[i],text);
	}
}
static void workshop_103_set(HWND lv, LVITEMW &lvi, workshop_cpp::data &d)
{	 	
	lvi.lParam = d.time;
	for(int k,&j=lvi.iSubItem=0;j<6;j++) 
	{
		switch(j)
		{
		case 0: k = d.time; break;
		case 1: k = d.hit-'0'; break;
		case 2: k = d.sfx; break;
		case 3: k = d.cp; break;
		case 4: k = d.snd; break;
		case 5: k = d.pitch; break;
		}
		if(k<1&&(j!=3||!d.sfx)&&(j!=5||k<-24||k==0)) 
		{			
			if(k!=j) //paranoia: allow frame 0?
			continue;
		}
		_itow(k,lvi.pszText,10);
		SendMessage(lv,j?LVM_SETITEMTEXTW:LVM_INSERTITEMW,lvi.iItem,(LPARAM)&lvi);
	}
}
static void workshop_103_get(HWND lv, LVITEMW &lvi, workshop_cpp::data &d)
{	 	
	for(int &j=lvi.iSubItem=0;j<6;j++)
	{		
		ListView_GetItem(lv,&lvi);
		int k = _wtoi(lvi.pszText);
		switch(j)
		{
		case 0: d.time = k; break;
		case 1: d.hit = k?k+'0':0; break;
		case 2:	d.sfx = k; break;
		case 3: d.cp = k; break;
		case 4: d.snd = k; break;
		case 5: d.pitch = k; break;
		}
	}
}
static int workshop_103_paste(HWND lv, WCHAR *p)
{
	SetWindowLong(lv,GWL_USERDATA,1); //workshop_103_104_close

	LVITEMW lvi = {LVIF_PARAM|LVIF_TEXT}; 	
	wchar_t text[64];
	lvi.cchTextMax = 64;
	lvi.pszText = text;
	int insN = ListView_GetItemCount(lv);
	int insDisplace,tDisplace = -1;
	if(1==ListView_GetSelectedCount(lv))
	{
		insDisplace =
		lvi.iItem = ListView_GetNextItem(lv,-1,LVIS_SELECTED);
		ListView_GetItem(lv,&lvi);
		tDisplace = lvi.lParam;
	}

	BYTE hits[3],hitsN=0,*hitp = 0;

	workshop_cpp::data d; do 
	{
		if(6!=d.scan(p))
		{
			MessageBeep(-1); return lvi.iItem; //FIX ME
		}
		if(!d.time) d.time = 1; //forbid 0?
		int t = d.time; 

		lvi.iItem = insN;
		for(int &i=lvi.iItem;--i>=0;)
		{
			ListView_GetItem(lv,&lvi);
			if(t>=lvi.lParam)
			{
				lvi.iItem = t==tDisplace?insDisplace:i+1;				
				goto ins;
			}
		}
		/*2021: I don't know what this was meant to accomplish but
		//it causes inserting a frame before the first frame to be
		//inserted onto the back of the list
		lvi.iItem = insN;*/
		lvi.iItem = 0; ins: insN++;
		
		bool hit = d.hit; d.hit = 0;
		workshop_103_set(lv,lvi,d); 
		if(hit&&SOM::tool==EneEdit.exe&&workshop_mode2-10u<3)
		{
			//Reminder: rewritten to delay deletions/insertions
			//until the end, to be a less brute force algorithm
			hitp = EneEdit_4173DC->hit_delay[workshop_mode2-10];
			BYTE i,j,hit;
			for(i=hitsN/*0*/;i<3;i++) if(hitp[i])
			{
				hit = hitp[i]; 
				hit:
				bool same = false; 
				for(j=0;j<hitsN;j++) if(hits[j]==hit) 
				same = true;
				//TODO: MessageBox with option to ListView_DeleteItem
				if(!same) if(hitsN<3) hits[hitsN++] = hit;
				else MessageBeep(-1);
				//HACK: insert this new hit?
			}if(i==3){ hit = 0xFF&t; goto hit; }			
		}						

		p = wcschr(p,'\n');

	}while(p++&&*p); if(hitp) //last but not least?
	{
		std::sort(hits,hits+hitsN);

		lvi.iSubItem = 1;
		*text = '\0';			
		LVFINDINFO lvfi = {LVFI_PARAM};
		BYTE i;
		for(i=0;i<3;i++) if(lvfi.lParam=hitp[i])
		for(int fi=-1;-1!=(fi=ListView_FindItem(lv,fi,&lvfi));)
		SendMessage(lv,LVM_SETITEMTEXTW,fi,(LPARAM)&lvi);			
		
		for(i=0;i<hitsN;i++) 
		{
			lvfi.lParam = hits[i];
			int fi = ListView_FindItem(lv,-1,&lvfi);
			_itow(i+1,text,10); 
			SendMessage(lv,LVM_SETITEMTEXTW,fi,(LPARAM)&lvi);
			hitp[i] = hits[i];
		}		
		for(;i<3;i++) hitp[i] = 0;
	}
	
	return lvi.iItem;
}	
static void workshop_103_op(HWND hw, WPARAM wp)
{
	HWND lv = GetDlgItem(hw,1016);
	if(wp==1011) //paste?
	{
		HANDLE got = 0;
		if(OpenClipboard(som_tool))		
		if(got=GetClipboardData(CF_UNICODETEXT))
		{
			WCHAR *lock = (WCHAR*)GlobalLock(got);
			if(lock&&*lock==0xfeff) lock++; //BOM?
			if(!lock) lock = L""; 
			som_tool_wector.assign(lock,lock+wcslen(lock)+1);
			GlobalUnlock(got);
			workshop_103_paste(lv,&som_tool_wector[0]);
		}
		CloseClipboard(); if(!got) MessageBeep(-1);	
		return;
	}
	if(wp!=1014) //cut/copy?
	{
		som_tool_wector.clear();
		LVITEMW lvi = {LVIF_TEXT}; 
		wchar_t w[64];
		lvi.cchTextMax = 64; lvi.pszText = w;	
		for(int i=-1;-1!=(i=ListView_GetNextItem(lv,i,LVIS_SELECTED));)
		{
			workshop_cpp::data d;
			lvi.iItem = i;
			workshop_103_get(lv,lvi,d);			
			int wN = d.print(w);
			w[wN] = '\r'; w[wN+1] = '\n';
			som_tool_wector.insert(som_tool_wector.end(),w,w+wN+2);
		}		
		if(!som_tool_wector.empty()&&OpenClipboard(som_tool))
		{		
			som_tool_wector.push_back(L'\0');
			size_t cp = som_tool_wector.size()*sizeof(WCHAR);
			HANDLE c2c = GlobalAlloc(GMEM_MOVEABLE,cp);	
			memcpy((WCHAR*)GlobalLock(c2c),&som_tool_wector[0],cp);
			GlobalUnlock(c2c);
			EmptyClipboard();
			bool out = //winsanity: c2c is freed and then returned!
			c2c==SetClipboardData(CF_UNICODETEXT,c2c); assert(out);
			CloseClipboard();
			if(!out) beep: MessageBeep(-1);		
		}
		else goto beep;
	}	
	if(wp!=1012) //cut/delete?
	{
		SetWindowLong(lv,GWL_USERDATA,1); //workshop_103_104_close

		for(int i=-1;-1!=(i=ListView_GetNextItem(lv,i,LVIS_SELECTED));)
		ListView_DeleteItem(lv,i--);
	}
}
static void workshop_103_enter(HWND hw)
{
	workshop_cpp::data d;
	d.time = GetDlgItemInt(hw,1020,0,0);
	d.hit = IsDlgButtonChecked(hw,1021)?'1':0;
	d.sfx = GetDlgItemInt(hw,1024,0,0);
	d.cp = GetDlgItemInt(hw,1025,0,0);
	d.snd = GetDlgItemInt(hw,1028,0,0);
	d.pitch = GetDlgItemInt(hw,1029,0,1);
	wchar_t w[64];
	d.print(w);
	HWND lv = GetDlgItem(hw,1016);
	int i = workshop_103_paste(lv,w);
	ListView_SetItemState(lv,-1,0,~0);
	ListView_SetItemState(lv,i,~0,LVIS_SELECTED|LVIS_FOCUSED);
}
static void workshop_103_104_close(HWND hw)
{
	if(32==workshop_mode2) //flame?
	{
		BOOL x;
		WORD sfx = (WORD)GetDlgItemInt(hw,1040,&x,0);
		BYTE per = (BYTE)GetDlgItemInt(hw,1041,0,0);
		BYTE cpt = (BYTE)GetDlgItemInt(hw,1042,0,0);
		short *flame;
		if(SOM::tool==EneEdit.exe)
		flame = &EneEdit_4173DC->flameSFX;		
		else flame = &NpcEdit_41761C->flameSFX;
		*flame = x?sfx:-1;
		BYTE *etc = (BYTE*)flame+2;
		etc[0] = cpt;
		etc[1] = per;

		CheckDlgButton(som_tool,1015,x);

		return; //!
	}

	bool on = IsDlgButtonChecked(hw,1007);		

	if(SOM::tool==EneEdit.exe) switch(workshop_mode2)
	{			
	case 6: case 7:
	{
		int f = workshop_mode2==7?4:8;
		if(on) EneEdit_4173DC->countermeasures|=f;		
		else EneEdit_4173DC->countermeasures&=~f;		

		//Page 2?
		if(HWND bt=GetDlgItem(som_tool,workshop_mode2==7?5993:5994))
		Button_SetCheck(bt,!on);

		break;
	}
	case 9: EneEdit_4173DC->activation = on;		

		CheckDlgButton(som_tool,2009,on);
		break;
	}

	LVITEMW lvi = {LVIF_TEXT}; 	
	wchar_t w[64];
	lvi.cchTextMax = 64; lvi.pszText = w;		

	HWND bt = 0;
	bool hits = false;
	if(SOM::tool==EneEdit.exe)
	if((hits=workshop_mode2-10u<3)||workshop_mode2-15u<3)
	{	
		HWND ch = GetDlgItem(hw,1006);
		GetWindowText(ch,w,21);

		//NOTE: disallowing single digit name
		wchar_t btext[4]; 
		bt = GetDlgItem(som_tool,2000+workshop_mode2); 
		if(1==GetWindowText(bt,btext,4)&&isdigit(btext[0]))
		{
			if(on) SetWindowTextW(bt,w);
		}
		else if(!on)
		{
			btext[0] = '1'+workshop_mode2-(hits?10:15);
			btext[1] = '\0'; 
			SetWindowTextW(bt,btext);
		}
		
		if(Edit_GetModify(ch))
		{	
			const char *a = EX::need_ansi_equivalent(932,w,
			EneEdit_4173DC->descriptions[workshop_mode2-(hits?10:12)],20);

			//overwrite the translation?
			//REVERSE TRANSLATING TO SHOW IF THERE IS AN ERROR OR NOT
			if(on) SetWindowTextW(bt,EX::need_unicode_equivalent(932,a));
		}
	}
	
	HWND cb = GetDlgItem(som_tool,4000+workshop_mode2); 
	int gc = ComboBox_GetCount(cb); 
	if(!gc) workshop_cpp::datamask|=1<<workshop_mode2;
	if(!gc) workshop_ReadFile_init(cb,hits);

	HWND lv = GetDlgItem(hw,1016);
	if(GetWindowLong(lv,GWL_USERDATA)) //modified?
	{
		ComboBox_ResetContent(cb);
		int iN = ListView_GetItemCount(lv);
		if(iN||on)
		ComboBox_AddString(cb,_itow(iN,w,10));
		for(int &i=lvi.iItem;i<iN;i++)
		{
			workshop_cpp::data d;		
			workshop_103_get(lv,lvi,d);			
			if(hits&&!d.hit) 
			d.hit = ' ';
			d.print(w);
			ComboBox_AddString(cb,w);
		}		
	}
	else if(bt&&gc<=1&&on!=(gc==1))
	{			
		if(!on) ComboBox_AddString(cb,L"0");
		else ComboBox_ResetContent(cb);
	}
}
static int workshop_103_reset(HWND hw, int bt=1030)
{
	//TODO? must save this somewhere, but USERDATA?	
	if(SOM::tool==EneEdit.exe&&workshop_mode2-10u<3)
	{			
		LONG *hd = (LONG*)
		EneEdit_4173DC->hit_delay[workshop_mode2-10];
		if(!som_tool_initializing) //reset?
		*hd = GetWindowLong(hw,GWL_USERDATA);
		else SetWindowLong(hw,GWL_USERDATA,*hd);
	}
	if(bt==1031) return 0; //Cancel

	HWND lv = GetDlgItem(hw,1016);
	ListView_DeleteAllItems(lv);
	HWND cb = GetDlgItem(som_tool,4000+workshop_mode2); 
	int iN = ComboBox_GetCount(cb)-1;
	LVITEMW lvi = {LVIF_TEXT|LVIF_PARAM}; 
	wchar_t text[64]; lvi.pszText = text;
	for(int &i=lvi.iItem;i<iN;i++)
	{
		ComboBox_GetLBText(cb,1+i,text);

		workshop_cpp::data d;
		if(6!=d.scan(text))
		{
			assert(0); //impossible
		}

		workshop_103_set(lv,lvi,d);
	}
	int i = ComboBox_GetCurSel(cb);
	if(i<=0) return 1;
	ListView_SetItemState(lv,i-1,~0,LVIS_SELECTED|LVIS_FOCUSED);
	ListView_EnsureVisible(lv,i-1,0); 
	SetFocus(lv); return 0;
}
static int workshop_103_104_INITDIALOG_chkstk(HWND hw, LPARAM lp)
{		
	if(lp==1015) //104/flame?
	{
		workshop_mode2 = 32; //104 
		short *flame;
		if(SOM::tool==EneEdit.exe)
		flame = &EneEdit_4173DC->flameSFX;		
		else flame = &NpcEdit_41761C->flameSFX;
		if(*flame>0) SetDlgItemInt(hw,1040,*flame,0);
		BYTE *etc = (BYTE*)flame+2;
		if(*flame>0||etc[0]) 
		SetDlgItemInt(hw,1042,etc[0],0); //flameSFX_greenofRGB
		if(etc[1]) 
		SetDlgItemInt(hw,1041,etc[1],0); //flameSFX_periodicty
		return 0;
	}

	HWND bt = GetDlgItem(som_tool,lp); 
	workshop_mode2 = lp-=2000; 
	wchar_t text[64];
	GetWindowText(bt,text,64);
	wchar_t *pp=text,*p=pp; 
	do if('-'==*p&&p[1]=='\r'&&p[2]=='\n')
	p+=3; else *pp++=*p++; 
	while(p[-1]);		
	SetWindowText(hw,text);	
	_itow(lp,text,10);
	SetDlgItemText(hw,1005,text);
	int on_off = 1007;
	if(EneEdit.exe==SOM::tool) switch(lp)
	{
	case 10: case 11: case 12:		
		windowsex_enable<1021>(hw);
	case 15: case 16: case 17:				
		windowsex_enable<1006>(hw);
		SetDlgItemText(hw,1006,
		EX::need_unicode_equivalent(932,EneEdit_4173DC->descriptions[lp-(lp>=15?12:10)]));
		//NOTE: this is a little confusing, but 
		//assuming that if the attack is opened
		//that it's going to be enabled so that
		//clicking this button is not busy work
		goto on;
	case 6: if(8&EneEdit_4173DC->countermeasures)
			goto on; goto off;
	case 7: if(4&EneEdit_4173DC->countermeasures)
			goto on; goto off;
	case 9: if(EneEdit_4173DC->activation)
			goto on; goto off; 
	off: on_off = 1008; 
	on: windowsex_enable(hw,1007,1008);		
	}	
	//hack: preventing WS_TABSTOP from being added
	//CheckDlgButton(hw,on_off,1);	
	SendDlgItemMessage(hw,on_off,BM_CLICK,1,0);	
	//MoveWindow
	RECT rc; GetWindowRect(hw,&rc); 
	int h = rc.bottom-rc.top, w = rc.right-rc.left;	
	GetWindowRect(bt,&rc); MapWindowRect(0,som_tool,&rc);
	int l = workshop_picture_window.right;
	if(rc.right<workshop_picture_window.right)
	l = workshop_picture_window.left-w;	
	GetWindowRect(som_tool,&rc); 
	rc.left+=l; rc.right = rc.left+w;
	MoveWindow(hw,rc.left,rc.top,w,h,0);
	SendMessage(hw,DM_REPOSITION,0,0);
	//up/downs
	SendDlgItemMessage(hw,1080,UDM_SETRANGE32,1,255); //time
	SendDlgItemMessage(hw,1085,UDM_SETRANGE32,0,255); //CP
	SendDlgItemMessage(hw,1089,UDM_SETRANGE32,-24,20); //pitch
	//HACK: up/downs initialize to "0" (when not even in range)
	SetDlgItemText(hw,1020,L"1"); //time
	SetDlgItemText(hw,1025,L""); //CP
	SetDlgItemText(hw,1029,L""); //pitch
	//ListView
	HWND lv = GetDlgItem(hw,1016);
	GetClientRect(lv,&rc);
	int sm = GetSystemMetrics(SM_CXVSCROLL);
	int cw = (rc.right-sm)/6;	
	//int sm_4 = sm/4;
	LVCOLUMNW lvc = {LVCF_TEXT|LVCF_WIDTH|LVCF_FMT,0,cw,L"??"};
	//lvc.cx = cw-sm_4+cw%sm_4+rc.right%cw-1;
	ListView_InsertColumn(lv,0,&lvc); //lvc.cx-=sm_4*3; 
	lvc.fmt = LVCFMT_CENTER;
	ListView_InsertColumn(lv,1,&lvc); //lvc.cx = cw;
	for(int i=2;i<6;i++)
	{
		lvc.fmt = i%2?LVCFMT_LEFT:LVCFMT_RIGHT;
		if(i==5) lvc.cx+=rc.right%cw;
		ListView_InsertColumn(lv,i,&lvc);
	}
	ListView_SetExtendedListViewStyle(lv,LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT);
	return workshop_103_reset(hw);
}
static INT_PTR CALLBACK workshop_103_104(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{	
	switch(uMsg)
	{	
	case WM_INITDIALOG:

		return workshop_103_104_INITDIALOG_chkstk(hwndDlg,lParam);	

	case WM_NOTIFY:
	{
		NMLISTVIEW* &p = (NMLISTVIEW*&)lParam;
		NMLVKEYDOWN * &pk = (NMLVKEYDOWN *&)lParam;

		switch(p->hdr.code)
		{
		case LVN_ITEMCHANGED:
		
			if((p->uNewState^p->uOldState)&LVIS_SELECTED)
			if(1==ListView_GetSelectedCount(p->hdr.hwndFrom))
			if(~GetKeyState(VK_CONTROL)>>15) 
			workshop_103_select(hwndDlg,p->hdr.hwndFrom,p->iItem);
			break;

		case LVN_KEYDOWN:

			assert(pk->hdr.idFrom==1016);
			switch(pk->wVKey)
			{
			case VK_DELETE:
				wParam = 1014; 
				goto op;
			case 'X': wParam = 1015; goto ctrl_op;
			case 'C': wParam = 1012; goto ctrl_op;
			case 'V': wParam = 1011; goto ctrl_op; 		
				ctrl_op:
				if(GetKeyState(VK_CONTROL)>>15) 
				goto op; return 0;
			}
		}
		break;
	}
	case WM_COMMAND:
		
		switch(wParam)
		{
		case IDOK: case 1018:

			if(workshop_mode2<32)
			workshop_103_enter(hwndDlg);
			break;

		case IDCANCEL: goto close;
		   
		case 1007: case 1008: //HACK!

			//HACK: assuming this radio group is low
			//priority. this prevents a tabstop from
			//being created from whole cloth
			if(lParam)
			SetWindowStyle((HWND)lParam,WS_TABSTOP,0);
			break;
		
		case 1014: //Delete
		case 1015: //Cut
		case 1012: //Copy
		case 1011: //Paste
		
		op:	workshop_103_op(hwndDlg,wParam);
			return 0; 

		case 1030: //Reset
			workshop_103_reset(hwndDlg); 
			break; 
		case 1031: //Cancel
			workshop_103_reset(hwndDlg,1031);
			goto cancel;
		}
		break;

	case WM_CLOSE: 
		close:
		workshop_103_104_close(hwndDlg);		
		cancel: 
		if(workshop_mode2<32)
		ComboBox_SetCurSel(GetDlgItem(som_tool,4000+workshop_mode2),0); 
		DestroyWindow(hwndDlg);
		break;
	} 
	return 0;
}

static INT_PTR CALLBACK workshop_32768(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
	{
		HWND ch = GetDlgItem(hwndDlg,1008);
		
		//good grief
		SendMessage(ch,EM_SETEVENTMASK,0,ENM_KEYEVENTS|SendMessage(ch,EM_GETEVENTMASK,0,0));

		
		Edit_LimitText(ch,96);		
		SetWindowText(ch,EX::need_unicode_equivalent(932,workshop->icard));
		return 1;
	}
	case WM_COMMAND:

		switch(wParam) 
		{
		case MAKEWPARAM(1008,EN_CHANGE):
		{
			GetDlgItemTextW(hwndDlg,1008,som_tool_text,97);
			const char *a = EX::need_ansi_equivalent(932,som_tool_text);
			size_t aN = strlen(a);
			*som_tool_text = '\0'; ((char*)a)[96] = '\0'; 
			if(aN>96) MessageBeep(-1);
			HWND ch = GetDlgItem(hwndDlg,1009); 
			//scrollbar/mouse wheel don't work!
			SetWindowText(ch,EX::need_unicode_equivalent(932,a));
			ShowScrollBar(ch,SB_VERT,Edit_GetLineCount(ch)>4);
			break;
		}
		case IDCANCEL: goto close; //NOT WORKING FOR Rich Edit :(
		}
		break;

	case WM_NOTIFY:
	{
		MSGFILTER* &p = (MSGFILTER*&)lParam; 
		if(p->nmhdr.code==EN_MSGFILTER) switch(p->msg)
		{
		case WM_KEYDOWN:
			
			if(p->wParam==VK_ESCAPE) //goto close; //crashes???
			{
				PostMessage(hwndDlg,WM_CLOSE,0,0);
			}
			break;
		}
		break;
	}
	case WM_CLOSE: close:
		
		GetDlgItemTextW(hwndDlg,1008,som_tool_text+2,97);
		EX::need_ansi_equivalent(932,som_tool_text+2,workshop->icard,97);
		if(SOM::tool==PrtsEdit.exe)
		{
			HWND ch = GetDlgItem(som_tool,1008);
			int ins = Edit_LineLength(ch,0);
			Edit_SetSel(ch,ins,-1);
			som_tool_text[0] = '\r';
			som_tool_text[1] = '\n';
			SendMessage(ch,EM_REPLACESEL,1,(LPARAM)som_tool_text);
			Edit_SetModify(ch,0);
		}
		*som_tool_text = '\0';
		
		DestroyWindow(hwndDlg);
	}
	return 0;
}

extern LRESULT CALLBACK workshop_throttleproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR hlp)
{
	//REMINDER: WOULD PREFER TO FIND THE CODE THAT "INVALIDATES" 
	//THIS WINDOW EVERY X MILLISECONDS
	switch(uMsg)
	{
	case WM_PAINT: 

		if(!DDRAW::isPaused) //workshop_submenu
		DefSubclassProc(hWnd,uMsg,wParam,lParam);
		if(workshop_throttle&&workshop_mode2!=15)
		{
			//don't know if this should be called
			//in the WM_PAINT context, or if there
			//being a delay could be just as bad if
			//not worse
			//SetWindowRedraw(hWnd,0);
			if(DDRAW::noFlips-workshop_antialias>DDRAW::dejagrate)			
			PostMessage(hWnd,WM_SETREDRAW,0,0);			
		}
		if(0&&EX::debug) //visualizing throttle
		{
			SetDlgItemInt(som_tool,1019,DDRAW::noFlips,0);
		}
		return 0;

	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,workshop_throttleproc,id);
		break;
	}

	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}

static void workshop_model_update()
{
	switch(SOM::tool)
	{
	case PrtsEdit.exe: break;

	case ItemEdit.exe: case ObjEdit.exe:
			
		som_tool_initializing++;
		SendMessage(som_tool,WM_COMMAND,1011,0);
		som_tool_initializing--;
		break;

	default: assert(SOM::tool>=EneEdit.exe);

		memcpy(som_tool_text,L"*",4);
		som_tool_initializing++;
		SendMessage(workshop_host,WM_COMMAND,40001,0);
		som_tool_initializing--;
		break;
	}
}
static void workshop_file_update(int id)
{	
	//ALGORITHM
	//this is to let subdirectories exists inside of
	//the standard data directory tree
	const wchar_t *fn,*w = fn = 
	PathFindFileNameW(som_tool_text);	
	const char *subdir = workshop_subdirectory(id);	
	const char *topdir = workshop_directory('sfx'); //2022
	for(int i,ii=w-som_tool_text;ii>1;ii=i)
	{
		w--;
		while(w>som_tool_text&&w[-1]!='\\'&&w[-1]!='/')
		{
			if(*w>=128) goto non_ascii;
			w--;
		}
		//don't scan after there's no way to fit into
		//30 characters... +6 is allowance for model/
		if(fn-w>30/*-4-1-1+6*/) non_ascii: 
		break;

		//2022: adding msm/mhm alterantive directories
		//and check to ensure the top directory isn't
		//matched (note: elsewhere I've resorted to 
		//using / in references and assuming paths are
		//using \ instead. I'm just adapting this code
		//for map/model in short order
		for(int pass=1;pass<=2;pass++)
		{
			//i = w-som_tool_text;
			int i2 = w-som_tool_text;
			const char *subdir2 = pass==1?topdir:subdir;		
		
			int j=0,jN=ii-i2-1; subdir2:
			while(j<jN&&subdir2[j]==tolower(w[j]))
			j++;
			if(j==jN&&subdir2[j]=='\0')
			{
				if(pass==2)
				{
					//if(wcslen(w+=j+1)<=30)
					if(wcslen(w+j+1)<=30)
					{
						fn = w+=j+1; //fn = w;
					}
				}
				goto breakout; //break;
			}
			else if(j==1&&jN==3&&pass==2)
			{
				//NOTE: workshop_model_update doesn't do
				//anything to MSM/MHM for PrtsEdit, so this
				//code doesn't technically have to do anything

				if(SOM::tool==PrtsEdit.exe)
				if(id==1005||id==1006)
				{
					subdir2 = id==1005?"msm":"mhm";
					goto subdir2;
				}
			}
		}
		i = w-som_tool_text;

	}breakout:;

	if(id==40074)
	{
		workshop_skin(EX::need_ansi_equivalent(932,fn));
		return;
	}

	HWND ch = GetDlgItem(som_tool,id);
	SetWindowText(ch,fn);	
	switch(SOM::tool)
	{
	case PrtsEdit.exe: //icon?

		if(id==1013) Edit_SetModify(ch,1);
		if(id==1013) SendMessage(som_tool,WM_COMMAND,IDOK,0);		
		break;

	default: workshop_model_update();
	}
}
static BOOL CALLBACK workshop_alt_click_cb(HWND w, LPARAM l)
{
    auto lp = (DWORD*)l;
    GetWindowThreadProcessId(w,(DWORD*)&l);
    if(l=l==*lp) *lp = (DWORD)w; return !l;
}
static void workshop_file_system(int id)
{
	bool open_dlg = id>=0; //PrtsEdit_SoftMSM
	if(!open_dlg) id = -id;

	wchar_t **f = workshop_filters; 
	switch(id)
	{
	case 40074: //skin?
	case 1006: f++;	break; //mhm?	
	case 1013: f+=2; break; //bmp? save skin?
	}

	const char *subdir = workshop_subdirectory(id);

	bool subdir2 = false; subdir2: //2022: map/msm or map/mhm?

	if(id==40074)
	{
		char *skin = SOM::tool==NpcEdit.exe?
		NpcEdit_41761C->skinTXR:EneEdit_4173DC->skinTXR;		
		EX::need_unicode_equivalent(932,skin[-1]?skin:"",som_tool_text,31);
	}
	else GetDlgItemText(som_tool,id,som_tool_text,31);
	som_tool_text[30] = '\0';
	char a[MAX_PATH+1];
	const char *slash = "\\"+(*som_tool_text?0:1);
	miss:
	sprintf(a,"%s\\%s%s%ls",workshop_directory('sfx'),subdir,slash,som_tool_text);
	if(!EX::data(a,som_tool_text+(*slash?0:1)))
	{
		if(*slash)
		{
			if(!subdir2&&wcsstr(*f+wcslen(*f)+1,L"*.*")) //2021: art?
			{
				extern IShellLinkW *som_art_link;
				extern int som_art_model(const char*, wchar_t[MAX_PATH]);

				int art; //goto

				//HACK: disable following the shortcut
				auto swap = som_art_link; som_art_link = 0;
				{
					#ifdef NDEBUG
//					#error MSM/MHM art has to use map/model somehow
					#endif
					art = som_art_model(a,som_tool_text);
				}
				if(swap) som_art_link = swap; 

				if(art&x2mdl_h::_art) goto art;

				if(SOM::tool==PrtsEdit.exe) //2022: map/msm or map/mhm?
				if(id==1005||id==1006)
				{
					subdir = id==1005?"msm":"mhm";

					subdir2 = true; goto subdir2;
				}
			}	

			//if the file isn't among the DATA folders, open the 
			//appropriate folder instead so Windows doesn't open
			//up to an inappropriate location
			if(*slash)
			{
				//2022: I'm pretty sure this was supposed to be
				//done, I may have removed it, but it's crashing
				//when "goto art" isn't taken
				som_tool_text[0] = '\0'; 

				slash++; goto miss; art:;
			}
		}
	}

	if(!open_dlg) return; //return som_tool_text?

	if(GetKeyState(VK_MENU)>>15) //2022: open 3D models?
	{
		SHELLEXECUTEINFO si={sizeof(si)};
		si.lpFile = som_tool_text; si.nShow = 1;
		si.fMask = SEE_MASK_NOCLOSEPROCESS|SEE_MASK_NOASYNC|SEE_MASK_WAITFORINPUTIDLE;

		#ifdef NDEBUG
//		#error need DROP solution?
		#endif
		//https://stackoverflow.com/questions/17166178/shell-execute-bring-window-to-front
		//if(32>(int)ShellExecute(0,0,som_tool_text,0,0,1))
		if(!ShellExecuteEx(&si)) MessageBeep(-1); else
		{
			LPARAM lp = GetProcessId(si.hProcess);
			AllowSetForegroundWindow(lp);
			if(!EnumWindows(workshop_alt_click_cb,(LPARAM)&lp))
			{
				SetForegroundWindow((HWND)lp);
				SetActiveWindow((HWND)lp); //???
			}
			CloseHandle(si.hProcess);
		}		
		return;
	}

	extern UINT_PTR CALLBACK SomEx_OpenHook(HWND,UINT,WPARAM,LPARAM);
	OPENFILENAMEW w = 
	{
		sizeof(w),som_tool,0,*f,0,0,0,
		som_tool_text,MAX_PATH,0,0,0,L"7-bit ASCII", 
		OFN_DONTADDTORECENT|

			//TESTING (FIX ME)
			//the new map/model folder is extremely slow opening if
			//it's full of art shortcuts. I'm sure GetOpenFileName 
			//is scanning every shortcut with IShellLink
			// 
			//NOTE: it won't even open if not using old style mode
			//but the window is deactivated
			// 
			// what I do is hide the shortcuts and disable showing
			// hidden files in Windows Explorer
			// 
			//OFN_ENABLEINCLUDENOTIFY| //this doesn't work on files
			OFN_NODEREFERENCELINKS|

		OFN_ENABLEHOOK|OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_FILEMUSTEXIST,
		0,0,0,0,SomEx_OpenHook,0,0,0
	};
	if(!*slash) *som_tool_text = '\0';
	if(!*slash) w.lpstrInitialDir = som_tool_text+1; 
	if(GetOpenFileNameW(&w)&&SOM::tool!=SfxEdit.exe) 
	{
		workshop_file_update(id);
	}
	else if(id==40074)
	{
		workshop_skin(0); *som_tool_text = '\0';
	}
}

static void workshop_WriteFile_text(int id, char (&a)[31])
{
	WCHAR w[31]; GetDlgItemText(som_tool,id,w,31);
	EX::need_ansi_equivalent(932,w,a,31);
}
static void workshop_WriteFile_names(char (*name)[31])
{
	workshop_WriteFile_text(1006,name[0]);
	workshop_WriteFile_text(1005,name[1]);
}
static void workshop_WriteFile_shape(char (*name)[31], float *height)
{
	workshop_WriteFile_names(name);
	for(int i=1002;i<=1004;i++)
	*height++ = GetDlgItemFloat(som_tool,i);
}
static int workshop_WriteFile_int(int id, int def=0)
{
	BOOL x; int o = GetDlgItemInt(som_tool,id,&x,1); return x?o:def;
}
static void workshop_WriteFile_part()
{
	workshop_cpp::part_prt part = {};
	workshop_WriteFile_text(1005,part.msm);
	workshop_WriteFile_text(1006,part.mhm);
	workshop_WriteFile_text(1013,part.bmp);
	workshop_WriteFile_text(1008,part.single_line_text);
	char*p,*n;if(n=strchr(p=part.single_line_text,'\n'))
	{
		if(n[-1]=='\r'&&n>p) 
		n--; else assert(0); memset(n,0x00,30-(n-p));		
	}
	part.iconbyte = GetDlgItemInt(som_tool,1011,0,0);
	part.blinders|=IsDlgButtonChecked(som_tool,1000);
	part.blinders|=IsDlgButtonChecked(som_tool,1001)<<2;
	part.blinders|=IsDlgButtonChecked(som_tool,1002)<<1;
	part.blinders|=IsDlgButtonChecked(som_tool,1003)<<3;
	part.features|=IsDlgButtonChecked(som_tool,1019);
	part.features|=IsDlgButtonChecked(som_tool,1020)<<1;

	HWND ch = GetDlgItem(som_tool,1008);
	if(Edit_GetModify(ch))
	{
		Edit_SetModify(ch,0);
		WCHAR *p,w[128]; //som_tool_text
		GetWindowText(ch,w,128);
		if(p=wcschr(w,'\n'))		
		EX::need_ansi_equivalent(932,p+1,workshop->icard,97);
		else *workshop->icard = '\0';
	}

	som_tool_wector.assign((WCHAR*)&part,
	(WCHAR*)&part+(sizeof(part)-sizeof(part.text_and_history))/2+1);
}
static void workshop_WriteFile_obj()
{
	workshop_cpp::object_prf obj = ObjEdit_476B00.data();
	workshop_WriteFile_shape(&obj.name,&obj.height);

	switch(GetDlgRadioID(som_tool,999,1001))
	{
	case 1000: obj.clipmodel = 0; //disc (radius or diameter???)

		//SOMETHING IS TRULY BROKEN (in som_db.exe)
		//HACK: do particle effects explode against this radius?
		//No??? but legacy profiles do this, so maybe it's best?
		//obj.depth = obj.width;
		//barrels have this set to 1, but is it height? doesn't
		//appear to effect particle clipping
		//obj._unused1 = 1.0f;
		//obj._unused1 = obj.height; //?
		break;

	case 1001: obj.clipmodel = 1; break; //box
	case 999: obj.clipmodel = 2; break;	
	}
	//CONSEQUENTIAL?
	//chests,switches,doors,traps... seems not
	//to do anything
	//does it mean a MDL has an animation present?
	//obj.openable = ?;
	//obj._unused1 = ?; //float type activation???
	obj.operation = workshop_category;
	obj.receptacle = workshop_WriteFile_int(1019);
	obj.trapSFX = workshop_WriteFile_int(1030,-1);
	obj.trapSFX_orientate = IsDlgButtonChecked(som_tool,1031);
	//2022: trapSFX test is because original PRF files
	//have 0 here, so saving them triggers SVN changes
	obj.trapSFX_visible = obj.trapSFX?!IsDlgButtonChecked(som_tool,1032):0;
	obj.flameSFX = workshop_WriteFile_int(1040,-1);
	obj.flameSFX_periodicty = workshop_WriteFile_int(1041);
	obj.loopable = IsDlgButtonChecked(som_tool,1050);
	obj.sixDOF = IsDlgButtonChecked(som_tool,1010);
	obj.invisible = IsDlgButtonChecked(som_tool,1052);
	obj.openingSND = workshop_WriteFile_int(1060,-1);
	obj.openingSND_pitch = workshop_WriteFile_int(1061);
	obj.openingSND_delay = workshop_WriteFile_int(1062);
	obj.closingSND = workshop_WriteFile_int(1063,-1);
	obj.closingSND_pitch = workshop_WriteFile_int(1064);
	obj.closingSND_delay = workshop_WriteFile_int(1065);
	obj.loopingSND = workshop_WriteFile_int(1066,-1);
	obj.loopingSND_pitch = workshop_WriteFile_int(1067);
	obj.loopingSND_delay = workshop_WriteFile_int(1068);
	obj.billboard = IsDlgButtonChecked(som_tool,1070);
	int id = IsDlgButtonChecked(som_tool,1080)?GetDlgRadioID(som_tool,1081,1082):0;
	obj.animateUV = id?id-1080:0;

	som_tool_wector.assign((WCHAR*)&obj,
	(WCHAR*)&obj+(sizeof(obj)-sizeof(obj.icard))/2);
}
static void workshop_WriteFile_item()
{
	workshop_cpp::item_prf<> item = ItemEdit_471ab0.data();
	workshop_WriteFile_names(&item.name);
	
	namespace prf = SWORDOFMOONLIGHT::prf;
	prf::moveset_t &item2 = item.get<prf::moveset_t>();

	item.center = GetDlgItemFloat(som_tool,1017);
	item.up_angles[0] = workshop_WriteFile_int(1012);	
	for(int i=1;i<=4;i<<=1)
	if(MF_CHECKED&GetMenuState(workshop_menu,34000+i,0))
	item.up_angles[0]+=360*i;
	item.up_angles[1] = workshop_WriteFile_int(1013);

	item.equip = workshop_category;		
	if(0==item.equip)
	{			
		item.my = 0;
		item.get<prf::item2_t>().receptacle = workshop_WriteFile_int(1019);
	}
	else 
	{
		item.SND = -1; item.SND_pitch = 0;
		memset(item2.SND,0xFF,sizeof(item2.SND));
		memset(item2.move,0xFF,sizeof(item2.move));
		memset(item2.SND_pitch,0x00,sizeof(item2.SND_pitch));

		bool my = IsDlgButtonChecked(som_tool,1031);
		bool ms = IsDlgButtonChecked(som_tool,1032);
		bool bl = IsDlgButtonChecked(som_tool,1033);
		if(bl) ms = true; //bow like?
		if(bl) item.equip = 3;		
		
		int pos = 0, rad1 = 0, rad2 = 0;
		if(IsDlgButtonChecked(som_tool,1008))
		pos = 1023;
		else if(2==workshop_category)
		pos = GetDlgRadioID(som_tool,1020,1026);		
		switch(pos)
		{
		case 1020: //head
		item.my = 1; item2.position = 0; break;
		case 1021: //body
		item.my = 4; item2.position = 1; break;
		case 1022: //arm
		item.my = 2; item2.position = 2; break;
		case 1023: //shield
		//5 becomes 2 below
		item.my = 5; item2.position = 5; break;
		case 1024: //legs
		item.my = 3; item2.position = 3; break;
		case 1025: //suit
		item.my = 4; item2.position = 4; break;
		case 1026: //accessory
		item.my = 4; item2.position = 6; break;
		default:
			//fill in form with default?
			//NOTE: this is because when 
			//the body mode is chosen it
			//doesn't choose a body part
			if(bl)
			{
				if(1!=workshop_category)
				{
					rad1 = 1008; 
					item2.position = 5; //2 arm bow/gun
				}
				//else item2.position = -1; //1 arm gun...
			}
			else if(2==workshop_category)
			{
				rad2 = 1025;
				item.my = item2.position = 4;
			}
		}		
		//HACK: undoing after is simpler		
		if(my||1==workshop_category) //1 arm gun, etc? 
		item2.position = -1;
		//HACK: -1 is setting item.position2
		if(!my) item.my = 1==item.equip?0:-1;
		
		int my3d = 0; if(my) //animation ID?
		{
			char *e;
			my3d = strtol(item.model,&e,10);
			if(*e||e==item.model)
			{
				my3d = -1; //itoa(n,item.model,10);
			}
			else if(my3d<0||my3d>65535)
			{
				my3d = max(0,min(65535,my3d));
				itoa(my3d,item.model,10);
			}
		}

		//fill in form with defaults?
		//the body and accessory equips default to
		//the full body suit (or full body move set) 
		//this may end up being a pseudo set, wherein
		//each of the 3 or 4 moves refers to body parts
		//(1 apiece)
		if(item.my==4) rad2 = 1025;
		//switch to arm?
		//(a weapon is not a body part)
		if(my&&!item.my||5==item.my)
		{
			item.my = 2; rad1 = 1009; rad2 = 1022;
		}
		if(rad1) workshop_click_radio(1007,1010,rad1);
		if(rad2) workshop_click_radio(1020,1026,rad2);

		bool move,movement = 
		my||!ms&&1==item.equip;		
		if(2!=item.equip||my||ms)
		for(int id=1040;id<=1042;id++)
		{
			if(move=my&&id==1042) item.movement2 = ~0; //2021

			HWND ch = GetDlgItem(som_tool,id);
			if(!ch){ assert(0); continue; }
			ch = GetWindow(ch,GW_CHILD);
			for(int j,i=3;ch;ch=GetWindow(ch,GW_HWNDNEXT),i--) 
			{
				//HACK: reserving first two non-weapon
				//moves, since the first is a fixed ID
				j = movement||1==item.equip||move?i:(i+2)%4;

				WCHAR w[8];
				if(!GetWindowText(ch,w,8)||!IsWindowEnabled(ch))
				{
					if(my3d==-1&&move&&i==0) *item.model = '\0';

					//HACK: this is how to detect the
					//presence of movesets
					//2021: 127 is default
					if(1042==id&&!movement) item2.SND_pitch[j] = 127;

					continue;
				}

				int n = _wtoi(w);
				switch(id)
				{
				case 1040: //sound
				(movement?item.SND:item2.SND[j]) = n;
				break;
				case 1041: //pitch
				if(n<-24||n>20) //paranoia //Ctrl+S?
				{
					n = 0; assert(0);
				}				
				if(!n&&!movement)
				{
					//HACK: this is how to detect the
					//presence of movesets
					//2021: I've been using 127 to mean default
					//-128 is just any out-of-bounds value which
					//will be treated as an error?
					n = -128; //127
				}
				(movement?item.SND_pitch:item2.SND_pitch[j]) = n;
				break;				
				case 1042: //move

					if(move) if(j) //2021
					{	
						item.movement2&=~(0x3ff<<10*(i-1));
						item.movement2|=(n&0x3ff)<<10*(i-1);
						continue;
					}
					else
					{
						if(my3d==-1) itoa(n,item.model,10);
		
						item.movement2&=~(3<<30);
						item.movement2|=n>>8<<30;
						n&=0xFF;
					}
					item2.move[j] = n; break;
				}	
			}
		}		

		if(movement)
		{
			item.equip = 1;			
			item.hit_pie = workshop_WriteFile_int(1055);
			item.hit_radius = GetDlgItemFloat(som_tool,1054);			
			item.hit_window[0] = workshop_WriteFile_int(1052);
			item.hit_window[1] = workshop_WriteFile_int(1053);			
			item.swordmagic_window[0] = workshop_WriteFile_int(1050);
			item.swordmagic_window[1] = workshop_WriteFile_int(1051);
			item.SND_delay = workshop_WriteFile_int(1056);

			//HACK: this is how to detect the
			//absence of movesets
			//e.g. item.zero = 0;
			*item2.SND_pitch = 0; 
		}
		else if(ms&&0==*item2.SND_pitch)
		{
			assert(0); //REMOVE ME?

			//HACK: this is how to detect the
			//presence of movesets
			*item2.SND_pitch = 127;
		}  
	}

	som_tool_wector.assign((WCHAR*)&item,
	(WCHAR*)&item+(sizeof(item)-sizeof(item.icard))/2);
}
static void workshop_WriteFile_data(WORD size, WORD *p, BYTE (*hit_delay)[3][3]=0)
{
	int pass = 1; pass2:
	int mask = ~workshop_cpp::datamask;		
	for(int i=0;i<32;i++,p+=2,mask>>=1)
	{			
		if(mask&1) continue;
		HWND cb = GetDlgItem(som_tool,4000+i); 
		if(!cb||2>ComboBox_GetCount(cb)) 
		continue;		
		
		p[1] = size;
		wchar_t w[64]; int hit = 0;
		int jN = ComboBox_GetCount(cb)-1;
		for(int j=0;j<jN;j++)
		{
			ComboBox_GetLBText(cb,1+j,w);			
			workshop_cpp::data d; d.scan(w);

			union
			{
				workshop_cpp::prf::data_t dd;
				WCHAR dd2[2];
			};
												
			if(pass==1)
			{
				if(d.hit)
				{
					(*hit_delay)[i-10][hit++] = d.time;
				}
				if(!d.sfx) continue;
				
				dd.effect = d.sfx; dd.CP = d.cp;
			}
			else if(d.snd) 
			{	
				dd.effect = d.snd; dd.pitch = d.pitch;
			}
			else continue;

			dd.time = d.time;
			som_tool_wector.push_back(dd2[0]);
			som_tool_wector.push_back(dd2[1]);
			p[0]++; size+=4;
		}
	}	
	if(1==pass++) goto pass2;
}
static void workshop_WriteFile_enemy()
{
	workshop_cpp::enemy_prf enemy = *EneEdit_4173DC; 	
	memset(enemy.dataSFX,0x00,2*sizeof(enemy.dataSFX));
	workshop_WriteFile_shape(&enemy.name,&enemy.height);
	enemy.diameter*=2; enemy.shadow*=2; //2020

	for(int i=0;i<3;i++)
	{
		//2021: I got these wrong and am reversing them
		//so all legacy monsters will swing before they
		//can hit you (more dramatic)
		enemy.pies[i] = workshop_WriteFile_int(1035+i);
		enemy.radii[i] = GetDlgItemFloat(som_tool,1032+i); //1029
		enemy.larger_radii[i] = GetDlgItemFloat(som_tool,1029+i); //1032
	}		
	//compact attacks... buttons untitled or 1 2 or 3 are skipped
	enemy.direct = 0;
	for(int i=2010;i<=2012;i++)
	{
		WCHAR x[4]; 
		if(GetDlgItemText(som_tool,i,x,4)!=1&&*x||*x<'1'||*x>'3')
		{
			char *p = enemy.descriptions[enemy.direct++];
			char *q = enemy.descriptions[i-2010];
			if(p!=q) strncpy(p,q,20);
		}
	}
	enemy.indirect = 0;
	for(int i=2015;i<=2017;i++)
	{
		WCHAR x[4]; 
		if(GetDlgItemText(som_tool,i,x,4)!=1&&*x||*x<'1'||*x>'3')
		{
			char *p = enemy.descriptions[enemy.indirect++];
			char *q = enemy.descriptions[i-2015];
			if(p!=q) strncpy(p,q,20);
		}
	}	
	EneEdit_default_descriptions(enemy);

	switch(GetDlgRadioID(som_tool,1007,1010))
	{
	case 1010: enemy.locomotion = 5; break;
	case 1008: enemy.locomotion = 2; break;
	case 1009: enemy.locomotion = 1; break;
	default:
	case 1007: enemy.locomotion = 0; break;
	}

	if(workshop_page2)
	{
		float t = GetDlgItemFloat(som_tool,1040);
		enemy.turning_ratio = 180/t;
		assert(t||!_finite(enemy.turning_ratio)&&!_isnan(t));

		enemy.inclination[0] = GetDlgItemInt(som_tool,1041,0,0); 
		enemy.inclination[1] = GetDlgItemInt(som_tool,1042,0,0);

		enemy.flight_envelope = GetDlgItemFloat(som_tool,1043);

		enemy.defend_window[0] = GetDlgItemInt(som_tool,1044,0,0); 
		enemy.defend_window[1] = GetDlgItemInt(som_tool,1045,0,0);

		enemy.turning_cycle[0][0] = GetDlgItemInt(som_tool,1046,0,0); 
		enemy.turning_cycle[0][1] = GetDlgItemInt(som_tool,1047,0,0); 
		enemy.turning_cycle[1][0] = GetDlgItemInt(som_tool,1048,0,0);
		enemy.turning_cycle[1][1] = GetDlgItemInt(som_tool,1049,0,0);
	}

	memset(enemy.hit_delay,0x00,sizeof(enemy.hit_delay));	
	workshop_WriteFile_data(sizeof(enemy),*enemy.dataSFX,&enemy.hit_delay);
	som_tool_wector.insert(som_tool_wector.begin(),
	(WCHAR*)&enemy,(WCHAR*)&enemy+sizeof(enemy)/2);
}
static void workshop_WriteFile_npc()
{
	workshop_cpp::npc_prf npc = *NpcEdit_41761C;
	memset(npc.dataSFX,0x00,2*sizeof(npc.dataSFX));
	workshop_WriteFile_shape(&npc.name,&npc.height);
	npc.diameter*=2; npc.shadow*=2; //2020

	if(HWND ch=GetDlgItem(som_tool,1040)) //workshop_page2?
	{
		npc.turning_ratio = 180/GetWindowFloat(ch);
		for(int i=0;i<4;i++)
		npc.title_frames[i] = GetDlgItemInt(som_tool,1044+i,0,0); 
	}

	workshop_WriteFile_data(sizeof(npc),*npc.dataSFX);
	som_tool_wector.insert(som_tool_wector.begin(),
	(WCHAR*)&npc,(WCHAR*)&npc+sizeof(npc)/2);
}
static void workshop_WriteFile_my()
{
	workshop_cpp::my_prf my; 
	enum{ sizeof_my = sizeof(my)-sizeof(my.icard) };
	memset(&my,0x00,sizeof_my);
	workshop_WriteFile_text(1006,my.name);
	my.onscreen = IsDlgButtonChecked(som_tool,1002);
	my.friendlySFX = GetDlgItemInt(som_tool,my.onscreen?1001:1000,0,0);

	som_tool_wector.assign((WCHAR*)&my,(WCHAR*)&my+sizeof_my/2);
}
static bool workshop_WriteFile(const wchar_t *to)
{		
	if(!*to)
	{
		assert(*to);
		beep: MessageBeep(-1); //MessageBox?
		return false; 
	}
	som_tool_wector.clear();
	switch(SOM::tool)
	{
	case PrtsEdit.exe: workshop_WriteFile_part(); break;
	case ItemEdit.exe: workshop_WriteFile_item(); break;			
	case ObjEdit.exe: workshop_WriteFile_obj(); break;
	case EneEdit.exe: workshop_WriteFile_enemy(); break;
	case NpcEdit.exe: workshop_WriteFile_npc(); break;
	case SfxEdit.exe: workshop_WriteFile_my(); break;
	}

	//2022: one day ObjEdited saved the PRF file to an MMD3D model
	//file path. I can't see how that's possible short of a system
	//malfunction. to is workshop_savename but I can't induce this
	//error to debug it. it's happened a few times but I wasn't in
	//the mood to put everything down to fix it. when I try to fix
	//it I can't make it happen!
	auto *ext = PathFindExtension(to);
	auto *cmp = SOM::tool==PrtsEdit.exe?L".PRT":L".PRF";
	//this seems like the most likely explanation but I see nothing
	//to suggest this should be possible (som.MDL.cpp)
	extern wchar_t *som_art_CreateFile_w;
	assert(!som_art_CreateFile_w); 
	som_art_CreateFile_w = 0; 
	if(wcsicmp(cmp,ext))
	{
		wchar_t buf[256];
		swprintf(buf,L"File extension is not %s."
		L"\n\nPlease report if this is a programmer error."
		L"\n\nFile info: %s",cmp,to);
		MessageBoxW(som_tool,buf,EX::exe(),MB_OK|MB_ICONHAND);
		return false; 
	}

	HANDLE h = CreateFile(to,GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0);
	if(h==INVALID_HANDLE_VALUE)
	{
		//goto beep; //2022
		EX::ok_generic_failure(L"Couldn't open file for writing");
		return false;
	}	

	//HACK: signaling modification flag to workshop_tool (SOM_PRM)
	//this is just a trick to force the PR2 files to track changes
	//in most instances, although imperfect
	SetWindowLong(som_tool,GWL_USERDATA,1);

	DWORD wr = 2*som_tool_wector.size();
	if(SOM::tool==PrtsEdit.exe) 
	wr-=1;
	WriteFile(h,&som_tool_wector[0],wr,&wr,0);

		//TODO: Sompaste.dll MUST BE UPDATED TO TAKE ADVANTAGE OF
		//THIS!
		//this scheme identifies the type of a PRF file with only
		//its size on disk, since a project can filter which ones
		//to include by their type and file names using SET files
		if(NpcEdit.exe==SOM::tool) 
		{
			//NOTE: F is chosen to look like FX, but lowercase so
			//to be more human-readable in case icard begins with
			//X (a capital title beginning with X)

			WriteFile(h,"f",1,&wr,0); //any single letter will do
		}
		if(SfxEdit.exe==SOM::tool) 
		{
			/*2020: I think 88 was a mistake
			if(wr!=88) //FX? any 2 will do*/
			if(wr!=40)
			{
				//RESERVING for a new variable-length special effects
				//format... it will be developed shortly, but may not
				//even use PRF for its extension
				assert(0);
			//	assert(!wcsicmp(L".prf",ext)); //???
				WriteFile(h,"fx",2,&wr,0);
			}
		}

	workshop->icard[96] = '\0';
	size_t zf = strlen(workshop->icard);
	memset(workshop->icard+zf,0x00,97-zf);	
	WriteFile(h,workshop->icard,97,&wr,0);

	CloseHandle(h); return true;
}
extern BOOL APIENTRY workshop_GetSaveFileNameA(LPOPENFILENAMEA a)
{
	SOM_TOOL_DETOUR_THREADMAIN(GetSaveFileNameA,a) 

	EXLOG_LEVEL(7) << "workshop_GetSaveFileNameA()\n";
	
	wcscpy(som_tool_text,workshop_savename);		
	bool skin = (void*)0x416068==a->lpstrFilter;
	if(skin) //40075
	wcscpy(PathFindExtension(som_tool_text)+1,L"bmp");
	extern UINT_PTR CALLBACK SomEx_OpenHook(HWND,UINT,WPARAM,LPARAM);
	OPENFILENAMEW w = 
	{
		sizeof(w),a->hwndOwner,0,0,0,0,0,
		som_tool_text,MAX_PATH,0,0,0,0, 
		OFN_DONTADDTORECENT|
		OFN_ENABLEHOOK|OFN_EXPLORER|OFN_ENABLESIZING,
		0,0,0,0,SomEx_OpenHook,0,0,0
	};
	wchar_t filter[64+8];
	w.lpstrFilter = workshop_filter(a->lpstrFilter,filter);
	if(GetSaveFileNameW(&w)) if(skin)
	{	
		//HACK: normally .ws is for profiles, but it's less
		//code in CreateFileA to handle this, since this is
		//the only write operation done by the tool program
		strcpy(a->lpstrFileTitle,".ws");
		strcpy(a->lpstrFile,SOMEX_(A)"\\.ws");
		return TRUE;	
	}
	else 
	{
		if(workshop_WriteFile(som_tool_text))
		wcscpy(workshop_savename,som_tool_text);
		*som_tool_text = '\0';
	}

	return FALSE; //don't involve the tool program
}

//HACK: this ensures that failure in the workshop tool doesn't leave the
//container in a disabled state
static VOID CALLBACK som_tool_workshop_timer(HWND,UINT,UINT_PTR id,DWORD)
{
	extern HWND som_tool_workshop;
	if(IsWindowEnabled(som_tool)||!IsWindow(som_tool_workshop))
	{
		KillTimer(0,id); EnableWindow(som_tool,1);
	}
}

extern HWND som_tool_workshop;
extern bool som_tool_workshop_exe(int hlp)
{	
	//HACK: open built-in SOM_PRM viewer 
	//2021: debug only until som_art.cpp
	//works for this path
	if(EX::debug&&GetKeyState(VK_MENU)>>15)
	{
		#ifdef NDEBUG
		//signal workshop_file_system and hide?
//		#error can this open the 3D model file?
		#endif

		//window is white/without picture for 
		//some reason when Alt is held down. F10
		//is like an Alt key, so should be harmless
		keybd_event(VK_F10,0,0,0);
		keybd_event(VK_F10,0,KEYEVENTF_KEYUP,0);
		return false;
	}

	//HACK: repurposing so not to keep
	//trying to dig into the language package
	//if the first attempt is unsuccessful
	if(workshop_cursor) return false;

	if(!hlp)
	{
		if(!som_tool_workshop) 
		return false;
		//DestroyWindow only works on the window's own
		//thread... so this may just be a hint to have
		//workshop_subclassproc quit itself
		//DestroyWindow(som_tool_workshop);
		PostMessage(som_tool_workshop,WM_DESTROY,0,0);
		som_tool_workshop = 0;
		return true;
	}		

	wchar_t *ws = som_tool_text, *_;
	int minor,major = hlp==40000?-1:0;
	if(major!=-1) //yuck: can't think of another way?
	{
		HWND cb = GetDlgItem(som_tool_stack[1],1002);
		int i = ComboBox_GetCurSel(cb);
		if(i==-1){ *ws = '\0'; goto new_profile; }
		char *id = (char*)ComboBox_GetItemData(cb,i)+31;
		assert(*id=='@'); major = strtol(id+1,&id,10);
		assert(*id=='-'); minor = strtol(id+1,&id,10);
	}
	else //HACK/REPURPOSING
	{
		//2022: this is SOM_MAP. som.files.cpp is also
		//using this... I think it should be a regular
		//variable
		
		//repurposing workshop_category here... in fact
		//aside from this it's outlived its earlier use
		minor = workshop_category; 
	}
	extern int som_tool_profile(wchar_t*&,int,int);
	som_tool_profile(_=ws,minor,major);

	new_profile:
	if(IsWindow(som_tool_workshop))
	{					
		struct hdrop : DROPFILES
		{
			wchar_t buf[MAX_PATH+1];
		};
		HANDLE h = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,sizeof(hdrop));	
		hdrop *hd = (hdrop*)GlobalLock(h);
		wcscpy(hd->buf,ws);
		hd->fWide = 1;
		hd->pFiles = sizeof(DROPFILES);
		GlobalUnlock(h);
		PostMessage(som_tool_workshop,WM_DROPFILES,(WPARAM)hd,0);

		//BLACK MAGIC
		//this is a lot of trial and error to eliminate flicker...
		//mainly when double-clicking the listbox, and so making
		//the workshop window appear... there is basically two
		//different scenarios depending on if the window is 
		//being created or not, since it behaves very differently
		//(on Windows 10 there is no flicker, but changing any
		//little thing here can change that!)
		
		//I think disabling first can curb flicker
		maybe_disable: if(1)
		{
			if(som_tool_workshop)
			EX::vblank(); //better?

			//may want to not disable if the window is a tool
			//window (always on top)
			EnableWindow(som_tool,0);
			SetTimer(0,0,1000,som_tool_workshop_timer);

			//FLICKERS
			//once the windows were mutually disabled (!) on
			//clicking the X button
			//EX::sleep(20); 
		}
		
		if(som_tool_workshop) //maybe_disable
		{
			EX::vblank(); //better?

			EnableWindow(som_tool_workshop,1);		
			//cannot seem do this attractively, as when 
			//opened for the first time :(
			ShowWindow/*Async*/(som_tool_workshop,SW_SHOW);		
		}
		
		return true;
	}

	const char *shop = 0; switch(hlp)
	{	
	case 31100: shop = "ItemEdit"; break;	
	case 33100:	shop = "SfxEdit"; break;
	case 34100: shop = "ObjEdit"; break;
	case 35100: shop = "EneEdit"; break;
	case 36100: shop = "NpcEdit"; break;
	case 40000: shop = "PrtsEdit"; break;
	default: assert(0); return false;
	}	

	SetEnvironmentVariableW(L".WS",ws);

	swprintf(ws,L"%hs %hs",shop,!*ws?"":SOMEX_(A)"\\.ws");

	if(!SOM::exe(ws)) 
	{
		workshop_cursor = 1; return false;
	}
	
	goto maybe_disable;	
}

extern bool (*som_exe_onflip_passthru)();
static bool workshop_onflip()
{	
	bool out = som_exe_onflip_passthru();

	//do_aa2? note, updating every frame locks the dialog
	//and makes parts disappear on Windows 10
	if(DDRAW::dejagrate) if(SOM::tool!=ItemEdit.exe)
	{
		assert(2==DDRAW::dejagrate);

		if(workshop_antialias) //HACK: WM_MOUSEMOVE
		{
			workshop_antialias = 0; goto aa;
		}
		
		//I don't really understand why something like the 
		//following 2 conditions don't work, but it doesn't
		//matter as long as this works
		//if(DDRAW::noFlips%2==?) 
		//if(DDRAW::noTicks-1>66) 
		//DDRAW::noFlips<4 is because noTicks is initially
		//0/undefined... it could just be initialized to a
		//large value
		//if(DDRAW::noFlips%2||DDRAW::noTicks-1u>65u)		
		if(DDRAW::noFlips%2||DDRAW::noTicks>66||DDRAW::noFlips<4) aa:		
		workshop_redraw(); 
	}

	//in theory this must only be done when changing out
	//of maximized mode... but anyway, redraw the border
	DX::D3DRECT buf[4]; DWORD j = 0;
	if(int i=-DDRAW::fxInflateRect) //DUPLICATE (SOM_MAP.cpp)
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
extern void SOM::initialize_workshop_cpp()
{
	if(som_exe_onflip_passthru) return;

	som_exe_onflip_passthru = DDRAW::onFlip;

	DDRAW::onFlip = workshop_onflip;
}

extern void workshop_reprogram()
{	
	//workshop_tool should probably be set up here
	EX::INI::Editor ed;
	//LETTING SfxEdit BE BLACK
	//these should agree on the same shade of blue
	//assert(workshop_colors[1]==ed->default_pixel_value2.enum_memset<<16);
	if(SOM::tool==SfxEdit.exe)
	workshop_colors[1] = ed->default_pixel_value3;
	else
	workshop_colors[1] = ed->default_pixel_value2;
	BYTE *bgr = (BYTE*)workshop_colors; std::swap(bgr[4],bgr[6]);
	if(!workshop_tool) workshop_colors[0] = workshop_colors[1];

	DDRAW::hack_interface(DDRAW::DIRECT3D7_CREATEDEVICE_HACK,workshop_CreateDevice);
	DDRAW::hack_interface(DDRAW::DIRECTDRAW7_CREATESURFACE_HACK,workshop_CreateSurface);
	DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_CLEAR_HACK,workshop_Clear);
	DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_BEGINSCENE_HACK,workshop_BeginScene);
	DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_DRAWPRIMITIVE_HACK,workshop_DrawPrimitive);
	DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_SETTRANSFORM_HACK,workshop_SetTransform);
	DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_SETTEXTURE_HACK,workshop_SetTexture);
	DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_DRAWINDEXEDPRIMITIVE_HACK,workshop_DrawIndexedPrimitive); //2021

	//this is the non-single-texture rejection test	
	{	
		//NOTE: THESE ARE ONLY 32 BYTES APART
		BYTE *cmp = 0;
		//0040329F 80 7A 03 01          cmp         byte ptr [edx+3],1  
		//004032A3 74 25                je          004032CA  
		if(SOM::tool==EneEdit.exe||SOM::tool==SfxEdit.exe)
		cmp = (BYTE*)0x40329F;		
		//0040327F 80 7A 03 01          cmp         byte ptr [edx+3],1  
		//00403283 74 25                je          004032AA
		if(SOM::tool==NpcEdit.exe)
		{
			cmp = (BYTE*)0x40327F;	 			
		}
		if(cmp) cmp[4] = 0xEB; //JMP
		assert(!cmp||*(DWORD*)cmp==0x01037A80);
	}

	//REFRERENCE
	/*WORKS WITH A cp FILE... BUT STILL SHOULDN'T CRASH
	//NpcEdit is crashing. I think it's because of CPs in the model.
	//I've ruled out animation IDs, soft-animations, and 2 textures.
	if(0&&SOM::tool==NpcEdit.exe) 
	{			
		//0x407FB9 calls into this subroutine. NPCs never get
		//to the CALL.
		//Just knocking out the CALL and zeroing eax to be safe.
		//
		//004054B0 53                   push        ebx 
		//...
		//HERE edx/eax come up 0. There is something that is unset
		//that EneEdit probably fills out.
		//(MAYBE because NPCs reserve the use of the CPs or see them
		//as regular model geometry.)
		//00405514 8B 84 82 88 00 00 00 mov         eax,dword ptr [edx+eax*4+88h]		
		if(SOM::tool==NpcEdit.exe)
		{
			BYTE *call = (BYTE*)0x407FB9;
			//HACK: Knock out this crashing subroutine.
			//What does it do?
			//00407FB9 E8 F2 D4 FF FF       call        004054B0
			memset(call,0x90,5); call[0] = 0x33; call[1] = 0xC0; //xor eax,eax
		}
	}*/

	//ObjEdit has the weird habit of wiping itself when the Open menu option
	//is used, so that after canceling it is in an unusual state
	//(it also has a Save confirmation box that ItemEdit does not enjoy)
	//this prevents the wipe	
	if(SOM::tool==ObjEdit.exe)
	{	
	//WEIRD... this code is not entered any longer... maybe save()=0 stuff? 
	//But this other one is...
		//NOTE: this subroutine is probably the same used by the New option
		//it leaves the picture window dirty It WILL HAVE TO BE FIXED LATER
		//0040A616 E8 D5 FE FF FF       call        0040A4F0			   
		//Open->No?
//		memset((void*)0x40a616,0x90,5);
	//Open/New
	//0040A663 E8 88 FE FF FF       call        0040A4F0
//		memset((void*)0x40A663,0x90,5);
	//startup
	//0040A0F6 E8 F5 03 00 00       call        0040A4F0 
	//	memset((void*)0x40A0F6,0x90,5);
		//Open (outer)
		//0040A6B2 E8 D9 FE FF FF       call        0040A590
		BYTE *call = (BYTE*)0x40A6B2;
		memset(call,0x90,5); call[0] = 0xB0; call[1] = 0x01; //mov al,1

		//2022: defeat annoying number entry message box
		//that spams if a window is empty disabled
		//00421B4C 75 14                jne         00421B62
		*(BYTE*)0x421B4C = 0xEB;
	}
	else if(SOM::tool==ItemEdit.exe)
	{
		//REMINDER: ItemEdit's New OPTION APPEARS TO BE NONFUNCTIONAL

		//this makes the opened PRF's string into an empty string ahead of
		//the Open option, which makes the Save option fail if the Open is
		//canceled
		//00405914 E8 C7 FE FF FF       call        004057E0
		BYTE *call = (BYTE*)0x405914;
		memset(call,0x90,5); call[0] = 0xB0; call[1] = 0x01; //mov al,1
	}
	//TODO? may want to reneable this as long as it is added to Item/ObjEdit
	//first... however users may just find it annoying, since SOM doesn't do
	//this in any other instance
	if(SOM::tool>=EneEdit.exe)
	{
		//suppress Save overwrite confirmation FOR CONSISTENCY SAKE
		//00403490 E8 7B 04 00 00       call        00403910
		//00403470 E8 7B 04 00 00       call        004038F0		
		BYTE *call = (BYTE*)(SOM::tool==NpcEdit.exe?0x403470:0x403490);
		memset(call,0x90,5); call[0] = 0xB0; call[1] = IDYES; //mov al,6
	}

	if(SOM::tool==ItemEdit.exe) //trying to change tilt axis
	{
		//00401FF3 E8 88 FC FF FF       call        00401C80
		*(DWORD*)0x401FF4 = (DWORD)ItemEdit_401C80-0x401FF8;
	}

	//TODO: HANDLE MISSING CP FILE GRACEFULLY
	//
	/*NOTE: som_db 004418b0 is identical to this subroutine
	//
	// (it's CP lookup code)
	//
	//DISABLING CALL 00402ac0!	
	if(SOM::tool==ObjEdit.exe) //crashes on vertex-animation
	{
		//REMINDER: ObjEdit doesn't render animations, so for now just
		//disabling this so it won't crash

		//00402AE6 8A 4D 54             mov         cl,byte ptr [ebp+54h]  
		//00402AE9 84 C9                test        cl,cl
		memcpy((void*)0x0402AE6,"\xb1\x01\x90",3); //mov cl,1 //nop
	}*/

	if(SOM::tool==SfxEdit.exe) //som.art.cpp
	{
		//overwrite "\data\enemy\model" path
		memcpy((char*)0x416108+6,"sfx\\model",10);
	}
}

//HACK: theses are defined in workshop.cpp only because the data 
//structures are already defined here... they could be moved out
template<class T>
static void som_game_60fps_flame_132(T *p)
{
	if(p->flameSFX>=0&&p->flameSFX<1024)	
	switch((BYTE)SOM::L.SFX_dat_file[p->flameSFX].c[0])
	{
	case 132: p->flameSFX_periodicty*=2; break;
	}
}
extern void som_game_60fps()
{
	bool fix = EX::INI::Bugfix()->do_fix_animation_sample_rate;

	BYTE *o = SOM::L.obj_pr2_file->uc;

	if(fix) for(int i=1024;i-->0;o+=108)
	{
		if(auto p=(workshop_cpp::object_prf*)o)
		{
			som_game_60fps_flame_132(p);

			p->loopingSND_delay*=2;
			p->openingSND_delay*=2;
			p->closingSND_delay*=2;
		}
		/*som_game_60fps_npc_etc does this overflow safety checks
		if(auto p=(workshop_cpp::npc_prf*)SOM::L.NPC_pr2_data[i])
		{
			//REMOVED
		}
		if(auto p=(workshop_cpp::enemy_prf*)SOM::L.enemy_pr2_data[i])
		{
			//REMOVED
		}*/
		SOM::L.enemy_prm_file[i].c[318]*=2; //post_attack_vulnerability

		if(auto p=(workshop_cpp::sfx_record*)&SOM::L.SFX_dat_file[i])
		{
			//extend life of particle so som_MDL_43a350 can restore
			//its frame rate to 30fps
			//this list is selective because often 60fps seems like 
			//an improvement
			//sometimes 30fps feels really bad and way way too slow
			switch(p->procedure)
			{
			case 132: //candle flames		

				p->unknown6[0]*=2; break;

			case 9: //the bird seems to travel further
			
			case 32: //dragon differs every other time

					//note: there's probably more like
					//these	(if not most)
					//note: this extends the length of
					//the animation but not the actual
					//range of the magic
				
				p->unknown6[1]*=2; break;

			case 21: //firewall
				
				//0043bff4 8a 46 05        MOV        AL,byte ptr [ESI + 0x5]
				p->unknown6[1]*=2;
				p->unknown6[3]*=2; break;
			}
			if(1) //compound lightning SFX (surveying)
			{
				switch(p->procedure)
				{
				case 40: //lightning bolt
			
				//hardcoded at 4370c4?
				//case 41: //lightning strike (2x2 image)

					//I think this effects overall timing

					p->unknown6[0]*=2; //break		
			
				//these pause in the middle? looks better
				//at 2x speed anyway
				//case 42: case 43: //lightning fallout

					p->unknown6[1]*=2; break;
				}
			}
		}
	}

	//2021: som.files.cpp needs this for item.arm
	extern void som_game_60fps_move(SOM::Struct<22>[250],int);
	som_game_60fps_move(SOM::L.item_pr2_file,250);	
}
extern void som_game_60fps_move(SOM::Struct<22> p[], int n)
{	
	bool other = p!=&SOM::L.item_pr2_file; //2021

	//som.MDL.cpp
	bool arm60 = other||60==SOM::arm[0];	

	//REMOVE ME
	int todolist[SOMEX_VNUMBER<=0x1020402UL];
	//UNFINISHED: this needs to be able to change on the fly
	extern float som_MDL_arm_fps; //0.06f	
	int armX = 1+(int) 
	(som_MDL_arm_fps*(300-EX::INI::Player()->arm_ms_windup));

	namespace prf = SWORDOFMOONLIGHT::prf;
	struct:workshop_cpp::prf_header,prf::item_t,prf::move_t{}*i;
	//(BYTE*&)i = SOM::L.item_pr2_file->uc;
	(BYTE*&)i = p->uc;

	for(auto e=i+n;i<e;i++) if(other||i->equip==1&&i->zero_if_move_t==0)
	{
		if(arm60) 
		{
			i->SND_delay*=2;
			i->hit_window[0]*=2; i->swordmagic_window[0]*=2;
			i->hit_window[1]*=2; i->swordmagic_window[1]*=2;
		}
		
		//REMOVE ME
		//arm_ms_windup is variable, so this way will break
		//if an Ex.ini file changes it on the fly, e.g. for
		//different weapons
		if(i->SND_delay<armX&&i->SND_delay!=0)
		{
			i->SND_delay = armX; //sound won't play otherwise
		}
	}
}
static void som_game_60fps_sxx(bool upscale, void *b0, void *bs, WORD *sxx)
{
	//NOTE: this is run whether upscaling or not in order
	//to repair (amputate) structural errors so the game
	//doesn't crash. these checks are needed to upscale

	typedef workshop_cpp::prf::data_t D; 

	if(int jN=sxx[0])
	{
		int s = sxx[1]; 

		D *d = (D*)((char*)b0+s); 
				
		for(int j=0;j<jN;j++,d++)
		{
			if(d+1>bs) //TODO: log errors
			{
			bad: sxx[0] = j; continue;
			}

			//PRF Editor v1.6.exe or SOM_PRM?
			//*Moratheia's enemy.pr2 file is cut
			//off by 8B (baadf00d) so I extended
			//it
			if(d->effect>=1024||d->effect<-1)
			{
				memset(d,0x00,4); goto bad;
			}//*/
			
			//NOTE: just converting 0 to 1 since I
			//don't think 0 is a valid time but it
			//is in some Moratheia files
			if(d->time)
			{			
				//if(upscale)
				assert(d->time<127); //overflow?

				if(upscale)
				d->time = 1+(d->time-1)*2; //breakpoint
			}
			else d->time = 1;
		}
	}
}
extern void som_game_60fps_npc_etc(std::vector<BYTE*> &v)
{
	bool enemy = !SOM::L.NPC_pr2_file;

	bool upscale = EX::INI::Bugfix()->do_fix_animation_sample_rate;

	std::sort(v.begin(),v.end()); for(size_t i=v.size()-1;i-->0;)
	{
		BYTE *e = v[i+1];

		if(!enemy) if(auto p=(workshop_cpp::npc_prf*)v[i])
		{
			if(upscale) som_game_60fps_flame_132(p);

			for(int i=0;i<4;i++) p->title_frames[i]*=2;

			for(int i=0;i<32;i++)
			{
				som_game_60fps_sxx(upscale,p,e,p->dataSFX[i]);
				som_game_60fps_sxx(upscale,p,e,p->dataSND[i]);
			}
		}
		if(enemy) if(auto p=(workshop_cpp::enemy_prf*)v[i])
		{
			if(upscale) som_game_60fps_flame_132(p);
			if(upscale)
			for(int i=3*3;i-->0;) p->hit_delay[0][i]*=2;

			for(int i=0;i<32;i++)
			{
				som_game_60fps_sxx(upscale,p,e,p->dataSFX[i]);
				som_game_60fps_sxx(upscale,p,e,p->dataSND[i]);
			}

			if(!p->turning_ratio)
			{
				p->turning_ratio = 1;
				p->turning_table = 7<<1;
			}
			
			//larger_radii is now used to trigger attacks
			//I believe all of SOM's classic monsters are
			//equal too or about 0.5 more than the damage
			//radius, however at least Moratheia has this
			//set to 0 for some or all monsters
			for(int i=0;i<3;i++)
			if(!p->larger_radii[0]) //Moratheia?
			{
				p->larger_radii[0] = p->radii[0];
			}
		}
	}
}