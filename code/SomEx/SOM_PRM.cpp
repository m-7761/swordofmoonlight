	
#include "Ex.h" 
EX_TRANSLATION_UNIT

#include <vector>

#include "som.extra.h"
#include "som.files.h"
#include "som.tool.hpp" 

extern HWND &som_tool;
extern int som_tool_initializing;
extern HWND som_tool_stack[16+1];
extern HWND som_tool_dialog(HWND=0);

extern int som_prm_tab;

//REMOVE ME??
extern std::vector<BYTE> 
SOM_PRM_items(1),SOM_PRM_magics(0);
extern int SOM_PRM_items_mode = 1202;
extern struct SOM_PRM_arm SOM_PRM_arm = {};
static const short SOM_PRM_hp[4][9] = 
{{1047,1099,1105, 1107,1121,1123,1125,1127, 1041}
,{1129,1131,1133, 1135,1137,1139,1141,1143, 1232}
//sliders
,{1028,1098,1104, 1106,1120,1122,1124,1126, 0}
,{1128,1130,1132, 1134,1136,1138,1140,1142, 0}
};
extern const WCHAR *SOM_PRM_items_titles[3] = {};
extern const WCHAR *SOM_PRM_magic_titles[3] = {};

//REMOVE ME?
//som_tool_GetWindowTextA SUBROUTINE
extern bool SOM_PRM_items_GetWindowTextA(HWND hw, LPSTR wt, int &len)
{									   
	//HACK: rushing to fix SOM_MAP bug (1.2.2.2)	
	int i,id = GetDlgCtrlID(hw);
	for(i=0;i<17;i++) if(id==((short*)SOM_PRM_hp)[i]) 
	break;
	if(i==17) return false;
	BYTE *p = SOM_PRM_47a708.data()+SOM_PRM_47a708.item()*336;
	//the new item tabs are in play, so pull the old item
	//data from the PRM record
	if(i>=9) i-=9; 
	len = strlen(itoa(p[300+i],wt,10)); return true;
}

void SOM_PRM_extend_items_magic(int id, int ew, BYTE find)
{
	HWND cb = GetDlgItem(som_tool_stack[1],id);
	
	//hide the attack #4 box?
	//seems to look/feel better as long as it's unlabeled
	if(id==1232) ShowWindow(cb,1204==SOM_PRM_items_mode);

	EnableWindow(cb,ew); if(ew)
	{
		//HACK: Rushing to fix SOM_MAP bug (1.2.2.2)
		#ifdef NDEBUG
		//#error how does SOM_PRM do this?
		#endif
		ew = 0;
		if(find!=0xFF)
		for(size_t i=1;i<SOM_PRM_magics.size();i++)			
		if(find==SOM_PRM_magics[i])
		ew = i;
	}
	else ew = -1; ComboBox_SetCurSel(cb,ew);
}
void SOM_PRM_extend_items_rates(int i, int j, int n, int ew, BYTE *p)
{
	wchar_t a[16] = L"0"; for(;n-->0;j++)
	{
		HWND et = GetDlgItem(som_tool_stack[1],SOM_PRM_hp[i][j]);
		HWND tb = GetDlgItem(som_tool_stack[1],SOM_PRM_hp[i+2][j]);
		EnableWindow(et,ew); SetWindowText(et,p?_itow(p[j],a,10):a);				
		EnableWindow(tb,ew); SendMessage(tb,TBM_SETPOS,1,p?p[j]:0);
	}
}
extern void SOM_PRM_140_titles_chkstk(HWND hw, int sel) //chkstk
{
	WCHAR buf[512]; for(int i=0;i<3;i++) if(SOM_PRM_magic_titles[i])
	{
		//Labels
		//NOTE: If not done before hiding the comboboxes, a hole
		//is left in an underlying groupbox's border.
		int len = wsprintf(buf,L"%s",SOM_PRM_magic_titles[i]);
		if(len<=0) continue;
		extern wchar_t *som_tool_bar(wchar_t*,size_t,size_t);				
		SetDlgItemTextW(hw,1224+i,som_tool_bar(buf,len,sel));
	}
}
static void SOM_PRM_138_titles_chkstk(HWND hw, WPARAM wp=SOM_PRM_items_mode) //chkstk
{
	int sel = 3==(3&SOM_PRM_items.back()); //magic or round?
	WCHAR buf[512]; for(int i=0;i<3;i++) if(SOM_PRM_items_titles[i])
	{
		//Labels
		//NOTE: if not done before hiding the comboboxes, a hole
		//is left in an underlying groupbox's border
		int len = wsprintf(buf,L"%s",SOM_PRM_items_titles[i]);
		if(len<=0) continue;
		extern wchar_t *som_tool_bar(wchar_t*,size_t,size_t);		
		//DONE: 1199 is 1 for bow/gun weapons
		//TODO: load items over sword magics!
		if(i) sel = wp-1202;
		SetDlgItemTextW(hw,1199+i,som_tool_bar(buf,len,sel));
	}
}
extern void SOM_PRM_extend(int rec=0);
extern LRESULT CALLBACK SOM_PRM_138(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scID, DWORD_PTR)
{
	switch(uMsg)
	{
	case WM_COMMAND:

		switch(wParam)
		{
		case 1202: case 1203: case 1204: case 1205:
		
			//HACK: BM_CLICK doesn't reliably do BM_SETCHECK
			//SOM_PRM_138_titles_chkstk should be called too
			if(som_tool_initializing)
			{
				SOM_PRM_items_mode = -1;
				//the problem is BM_CLICK sends BN_CLICKED first
				//before it sets the BM_GETCHECK state. Skipping
				//to the next block works too, but takes chances
				CheckDlgButton(hWnd,wParam,1);
			}
			//radios report clicked on focus... but don't check
			//themselves???	som_tool_pushlikeproc can't help it
			if(IsDlgButtonChecked(hWnd,wParam))
			{
				//HACK: disabled radios don't change themself!
				//InvalidateRect((HWND)lParam,0,0);
				if(1204==SOM_PRM_items_mode)
				CheckDlgButton(hWnd,1204,0);

				//moving this here to minimize same name flicker
				if(wParam!=SOM_PRM_items_mode)
				{
					//NEW: helps flicker... to where is above 
					//comment referring?
					if(!som_tool_initializing) EX::vblank();

					SOM_PRM_138_titles_chkstk(hWnd,wParam);					
				}

				if(!som_tool_initializing)
				SOM_PRM_extend(1013);
				SOM_PRM_items_mode = wParam;			
				SOM_PRM_extend();
			}
			break;		
		
		case 1211: case 1212: case 1213: case 1214:
		{
			int i = wParam-1211+1202; if(i>1204) i--;
			if(!IsDlgButtonChecked(hWnd,i))
			windowsex_click(hWnd,i);
			if(IsDlgButtonChecked(hWnd,wParam))
			{					
				const short (&hp)[9] = SOM_PRM_hp[1214==wParam];
				for(int i=0;i<3;i++)
				{
					SetDlgItemInt(hWnd,hp[i],0,0);
					//not happening automatically :(
					SendDlgItemMessage(hWnd,hp[i+18],TBM_SETPOS,1,0);
				}
				HWND cb = GetDlgItem(hWnd,hp[8]);
				ComboBox_SetCurSel(cb,0);
				SendMessage(hWnd,WM_COMMAND,MAKEWPARAM(hp[8],CBN_SELENDOK),(LPARAM)cb);
			}
			break;
		}
		case MAKEWPARAM(1231,CBN_SELENDOK):
		case MAKEWPARAM(1232,CBN_SELENDOK):

			SOM_PRM_47a708.save() = 1; //new magic
			return 0;

		//carbon-copy the redundant inputs?
		case MAKEWPARAM(1039,CBN_SELENDOK): //1052 //side effect

			SendDlgItemMessage(hWnd,1052,CB_SETCURSEL,ComboBox_GetCurSel((HWND)lParam),0);
			break;
					
		case MAKEWPARAM(1042,EN_KILLFOCUS): //weight
		case MAKEWPARAM(1025,EN_KILLFOCUS): //weight

			return 0; //allow weights down to 1g...

		case MAKEWPARAM(1040,EN_CHANGE): //1053	//odds
		case MAKEWPARAM(1025,EN_CHANGE): //1042 //weight
		case MAKEWPARAM(1025,EN_UPDATE): //1042
		{
			enum{ cpyN=8 }; wchar_t cpy[cpyN];			
			cpy[GetWindowText((HWND)lParam,cpy,cpyN)] = '\0';
			if(EN_UPDATE!=HIWORD(wParam))
			{
				//duplicate into hidden/redundant settings
				SetDlgItemText(hWnd,LOWORD(wParam)==1025?1042:1053,cpy);			
			}
			else //add extra place after the decimal point
			{
				int i,j; bool dp = false, beep = false;
				for(i=j=0;i<cpyN;i++,j++)
				if(!isdigit(cpy[j]=cpy[i])) switch(cpy[j])
				{
				case '.': if(!dp){ dp = true; break; }
				
				default: j--; beep = true; break;

				case '\0': i = cpyN; //hack
				}
				if(!beep) break;
				MessageBeep(-1);
				SetDlgItemText(hWnd,LOWORD(wParam),cpy);			
			}
			break;
		}}
		break;
	
	case WM_NCDESTROY:

		SOM_PRM_items.clear();
		SOM_PRM_items.push_back(0); //HACK: selection
		SOM_PRM_magics.clear();
		RemoveWindowSubclass(hWnd,SOM_PRM_138,scID);
		break;
	}		  
	
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}

extern LRESULT CALLBACK SOM_PRM_strengthproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR tie, DWORD_PTR)
{
	extern void *SOM_PRM_strength;
	bool *st = (bool*)SOM_PRM_strength; assert(1==sizeof(bool));

	//FYI: This procedure tries to make the Strength trackbar move 
	//the Magic parameter if Strength and Magic are the same value.
	int pos; switch(uMsg)
	{
	case TBM_SETPOS: case TBM_SETPOSNOTIFY: 

		if(!*st)
		{
			pos = SendMessage(hWnd,TBM_GETPOS,0,0);
			if(pos!=lParam
			&&pos==SendMessage((HWND)tie,TBM_GETPOS,0,0))
			goto move2;
		}
		break;

	case WM_TIMER: 
		
		if(wParam=='str') 
		{
			*st = false; 
			st[1] = false; //HACK
			KillTimer(hWnd,wParam);
		}
		break;

	case WM_PAINT:

		//HACK: Assuming the bar is pressed.
		if(*st&&GetKeyState(VK_LBUTTON)>>15)
		if(hWnd==GetCapture()) //better
		{
			pos = SendMessage((HWND)tie,TBM_GETPOS,0,0); 
			goto move2; 
		}
		break;

	case WM_MOUSEWHEEL: case 0x020E: //WM_MOUSEHWHEEL

		SetTimer(hWnd,'str',250,0);
		if(*st) goto move; goto down;
		
	case WM_LBUTTONUP: 

		*st = false; break;

	case WM_KEYUP:

		if(~GetKeyState(VK_CONTROL)>>15)
		{	
			*st = st[1] = false; 
		}
		break;

	case WM_KEYDOWN:
			
		//HACK: Ensure edit routed
		//keys are uplifted.
		SetTimer(hWnd,'str',1000,0);
	
		//Arrow keys always repeat???
		/*if(LOWORD(lParam)!=1) 
		{
			goto repeat; //breakpoint
		}*/		
		if(st[1]) goto repeat;

		st[1] = true; //break;

	case WM_LBUTTONDOWN: down:

		pos = SendMessage(hWnd,TBM_GETPOS,0,0);
		*st = pos==SendMessage((HWND)tie,TBM_GETPOS,0,0);
		if(!*st) break; goto move2;
	
	case WM_MOUSEMOVE: repeat:

		if(*st) move:
		{
			pos = SendMessage(hWnd,TBM_GETPOS,0,0);
			move2:
			LRESULT ret = DefSubclassProc(hWnd,uMsg,wParam,lParam);
			//just a click generates EN_CHANGE
			int pos2 = SendMessage(hWnd,TBM_GETPOS,0,0);			
			if(pos!=pos2) 
			SendMessage((HWND)tie,TBM_SETPOSNOTIFY,0,pos2);
			return ret;
		}
		break;

	case WM_NCDESTROY:

		st[0] = st[1] = false;
		RemoveWindowSubclass(hWnd,SOM_PRM_strengthproc,tie);
		break;
	}		  
	
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
 
static bool SOM_PRM_extend_items(int rec, HWND hw)
{
	int i = SOM_PRM_47a708.item();
	BYTE *p = SOM_PRM_47a708.data()+i*336;
	int prf = SendDlgItemMessage(hw,1002,CB_GETCURSEL,0,0);
	prf = prf==CB_ERR?0:SOM_PRM_items[prf];
	SOM_PRM_items.back() = prf; //HACK: selection
	if(!rec) SOM_PRM_138_titles_chkstk(hw);
	BYTE &kf2 = p[291];
	float &wt2 = (float&)p[292];
	struct equip //36B
	{		
		//+300
		BYTE hp[8],fx,fx2,sm;
		//EXTENDED
		BYTE assist[5];
		BYTE attack2[4];
		union
		{
			BYTE attack34[2][4];
			BYTE attack1[8];
		};
		BYTE shield[8]; //328
		
	}&eq = (equip&)p[300];
	BYTE *hp[4] = 
	{
		prf&1?eq.hp:eq.attack1,
		eq.attack2,eq.attack34[0],eq.attack34[1]
	};
	if(rec==0) //initialize?
	{
		int save = SOM_PRM_47a708.save();

		//weight (down to 1g)
		HWND wt = GetDlgItem(hw,1025);
		EnableWindow(wt,1);
		Edit_LimitText(wt,5);
		wchar_t f[32] = L"";
		int n = swprintf(f,L"%.3f",prf==0?wt2:(float&)p[296]);
		if(n>1&&f[n-1]=='0') f[--n] = '\0';
		if(n>1&&f[n-1]=='0') f[--n] = '\0';
		SetWindowText(wt,f);

		//tabs
		int modes = 0; switch(prf&3)
		{		
		//HACK: let PRFs be overridden? to some end??
		//REMINDER: might want 2 mode weapons one day
		case 1: modes = 4; break; case 2: modes = 2;
		}
		windowsex_enable(hw,1202,1205,prf!=0);
		if(2==modes) 
		windowsex_enable<1204>(hw,0);
		
		//Magic
		int a = 0, b = 0;
		switch(SOM_PRM_items_mode)
		{		
		case 1202: 
		case 1203: a = 0!=(3&prf); break;
		case 1204: a = b = 1&prf;  break;
		}
		BYTE sm = SOM_PRM_items_mode-1202;
		if(a) sm = sm?hp[sm][3]-1:eq.sm;
		SOM_PRM_extend_items_magic(1041,a,sm);
		SOM_PRM_extend_items_magic(1232,b,hp[3][3]-1);
		SOM_PRM_extend_items_magic(1231,1,kf2-1);

		//Attacks				
		for(int i=0;i<4;i++)
		{
			int ea = (prf>>i+2)&1;	
			sm = i?hp[i][3]-1:eq.sm;
			bool ff000000 = i>=modes //...
			||0xFF==sm&&!hp[i][0]&&!hp[i][1]&&!hp[i][2];
			HWND ch = GetDlgItem(hw,1211+i);			
			Button_SetCheck(ch,ea&&ff000000);
			EnableWindow(ch,modes?ea||!ff000000:0);
		}

		if(prf!=0)
		{
			int c,d;
			switch(SOM_PRM_items_mode)
			{		
			case 1205:
			case 1202: a = b = 1; c = d = modes==2; break;
			case 1203: a = 1; b = 0; c = 0; d = 1; break;
			case 1204: a = c = modes==4; b = d = 0; break;
			}
			BYTE *ap,*bp,*cp,*dp;
			ap=bp=cp=dp=prf&1?eq.hp:eq.attack1;
			switch(SOM_PRM_items_mode)
			{
			case 1202: 
				
				cp = dp = prf&1?0:eq.hp; break;

			case 1203:
				
				ap = eq.attack2;
				cp = 0; dp = eq.assist-3; break;

			case 1204:

				ap = modes!=4?0:eq.attack34[0];
				cp = modes!=4?0:eq.attack34[1]; break;

			case 1205: 
				
				ap = bp = eq.shield; 				
				cp = dp = prf&1?0:eq.hp; break; 
			}
			SOM_PRM_extend_items_rates(0,0,3,a,ap);
			SOM_PRM_extend_items_rates(0,3,5,b,bp);
			SOM_PRM_extend_items_rates(1,0,3,c,cp);
			SOM_PRM_extend_items_rates(1,3,5,d,dp);		
					   			
			if(prf&2) //clothing?
			{
				int ch[] = {1039,1040,1025,1003};
				windowsex_enable(hw,ch);
				SendDlgItemMessage(hw,1039,CB_SETCURSEL,p[308],0);
				SetDlgItemInt(hw,1040,p[309],0);
			}

			//Strength & Magic
			SetDlgItemInt(hw,1171,0,0);
			SetDlgItemInt(hw,1172,0,0);
			windowsex_enable(hw,1170,1173,0); 			
		}

		SOM_PRM_47a708.save() = save;
	}	
	else //if(SOM_PRM_47a708.save()) //recording?
	{
		assert(rec==1013);

		kf2 = SendDlgItemMessage(hw,1231,CB_GETCURSEL,0,0);
				 		
		if(0==prf) //item?
		{
			wchar_t cpy[8];			
			cpy[GetDlgItemText(hw,1025,cpy,8)] = '\0';
			wt2 = wcstod(cpy,0);
		}
		else switch(SOM_PRM_items_mode-1202)
		{
		case 0: //attack/defense 
			
			for(int i=0;i<8;i++)
			hp[0][i] = GetDlgItemInt(hw,SOM_PRM_hp[0][i],0,0);
			eq.sm = SendDlgItemMessage(hw,1041,CB_GETCURSEL,0,0)-1;
			shield2:
			if(~prf&2) break;
			for(int i=0;i<8;i++)
			eq.hp[i] = GetDlgItemInt(hw,SOM_PRM_hp[1][i],0,0);
			break;

		case 1: //attack 2/assist

			for(int i=0;i<3;i++)
			hp[1][i] = GetDlgItemInt(hw,SOM_PRM_hp[0][i],0,0);
			hp[1][3] = SendDlgItemMessage(hw,1041,CB_GETCURSEL,0,0);
			for(int i=0,j=3;i<5;i++,j++)
			eq.assist[i] = GetDlgItemInt(hw,SOM_PRM_hp[1][j],0,0);
			break;

		case 2: //attack 3/4

			for(int i=0;i<3;i++)
			hp[2][i] = GetDlgItemInt(hw,SOM_PRM_hp[0][i],0,0);
			hp[2][3] = SendDlgItemMessage(hw,1041,CB_GETCURSEL,0,0);
			for(int i=0;i<3;i++)
			hp[3][i] = GetDlgItemInt(hw,SOM_PRM_hp[1][i],0,0);
			hp[3][3] = SendDlgItemMessage(hw,1232,CB_GETCURSEL,0,0);
			break;

		case 3: //shield

			for(int i=0;i<8;i++)
			eq.shield[i] = GetDlgItemInt(hw,SOM_PRM_hp[0][i],0,0);
			goto shield2;
		}		
	}
	return 0==prf;
}
extern void SOM_PRM_extend(int rec)
{
	HWND hw1 = som_tool_stack[1];
	if(!hw1) return; //too soon?

	if(1003==som_prm_tab) //magic? unrelated
	{
		if(rec==1013) //saving?
		{
			//BUGFIX: it seems SOM_PRM has never saved
			//the "duration" field when the profile is
			//a shield style magic
			BYTE *p = SOM_PRM_47a708.data();
			p+=320*SOM_PRM_47a708.item();
			(FLOAT&)p[56] = GetDlgItemFloat(hw1,1061);
		}
	}

	//object, enemy, NPC?
	WORD *strength,*magic; 
	if(som_prm_tab<1005||som_prm_tab>1007) 	
	if(1001!=som_prm_tab||!SOM_PRM_extend_items(rec,hw1))
	return;
	
	static int test = 0; 
	assert(test++==0);
	int i = SOM_PRM_47a708.item();
	assert(i>=0&&i<=1023);
	BYTE *p = SOM_PRM_47a708.data();
	switch(SOM_PRM_47a708.main().tab)
	{
	case 0: p+=i*336+274; i = 1; break; //item
	case 3: p+=i*56+38; i = 8; break; //object
	case 4: p+=i*488+282; i = 1; break; //enemy
	case 5: p+=i*320+302; i = 1; break; //NPC
	}
	strength = (WORD*)p; magic = strength+i;
	SOM::xxiix sm; BOOL got; 
	sm = *strength|*magic<<16;
	HWND dlg = som_tool_dialog();
	//10 bits each. a 3rd stat (speed) may be 
	//added. plus! there are 2 remaining bits
	if(rec==1013)
	{	
		//if(SOM_PRM_47a708.save())
		{
			rec = GetDlgItemInt(dlg,1171,&got,0);
			if(got) sm.x1 = rec+1;
			if(got) *strength = sm[0];		
			rec = GetDlgItemInt(dlg,1173,&got,0);
			if(got) sm.x2 = rec+1;
			if(got) *magic = sm[1];
		}
	}
	else //initializing?
	{
		i = 1007!=som_prm_tab?1: //traps, etc.
		IsWindowEnabled(GetDlgItem(dlg,1049));
		windowsex_enable(dlg,1170,1173,i); //initialize
		//HACK? singleline WM_SETTEXT sends EN_CHANGE
		int save = SOM_PRM_47a708.save();
		for(int j=1170;j<=1172;j+=2)
		{
			rec = j==1170?sm.x1:sm.x2;
			rec = rec==0?i?50:0:rec-1;			
			if(got=SetDlgItemInt(dlg,j+1,rec,0))
			{
				/*MAGIC: always us the TBM_SET* redraw flag!!
				//HACK??? no clue. paint wizard->wizard change
				//InvalidateRect doesn't help. SETPOSNOTIFY does?
				//SendDlgItemMessage(dlg,j,TBM_SETPOS,1,rec-50);
				SendDlgItemMessage(dlg,j,TBM_SETPOSNOTIFY,0,rec-50);				
				*/
			}
		}		
		SOM_PRM_47a708.save() = save;
	}
	if(som_tool_stack[2]) //35100 (142)
	{
		SetDlgItemInt(hw1,1171,sm.ryoku(1),0);
		SetDlgItemInt(hw1,1173,sm.ryoku(2),0);
	}
	assert(test--==1);
}

void SOM_PRM_arm::clear(DWORD save)
{
	filetime.dwHighDateTime = 0;
	filetime.dwLowDateTime = save;
}
void SOM_PRM_arm::init()
{
	read(true);
}
bool SOM_PRM_arm::read(bool overwrite)
{
	//som_tool_CreateFileA
	HANDLE h = CreateFileA(SOMEX_(B)"\\PARAM\\ITEM.ARM",SOM_TOOL_READ);
	if(h==INVALID_HANDLE_VALUE)
	{
		if(ERROR_FILE_NOT_FOUND==GetLastError())
		{
			clear(0); delete[] file; file = 0;
		}
		else MessageBoxA(0,"PRM100","SOM_PRM",MB_OK|MB_ICONERROR);

		return true;
	}
	bool ret = false;
	FILETIME ft; 
	if(GetFileTime(h,0,0,&ft))
	if(CompareFileTime(&ft,&filetime)) //OPTIMIZING
	{
		filetime = ft;		

		ret = true; if(overwrite)
		{
			enum{ fsz=88*1024 };
			if(!file) file = new BYTE[fsz];
			DWORD rd = 0;
			ReadFile(h,file,fsz,&rd,0);
			memset(file+rd,0x00,fsz-rd);
		}
	}
	CloseHandle(h); return ret;
}
void SOM_PRM_arm::save()
{
	if(!filetime.dwLowDateTime //OPTIMIZING
	&&!filetime.dwHighDateTime||!read(false))
	{
		return; //no movesets or no changes?
	}

	//HACK: arm->clear() is called below and
	//it's being used as a tmp buffer
	auto &tmp = SOM::PARAM::Item.arm;
	auto *old = tmp;

	//som_tool_CreateFileA
	HANDLE h = CreateFileA
	(SOMEX_(B)"\\PARAM\\ITEM.ARM",GENERIC_WRITE,0,0,OPEN_ALWAYS,0,0);
	if(h==INVALID_HANDLE_VALUE) prm101:
	{
		MessageBoxA(0,"PRM101","SOM_PRM",MB_OK|MB_ICONERROR);
		return;
	}
	if(!tmp->open())
	{
		CloseHandle(h);
		MessageBoxA(0,"PRM102","SOM_PRM",MB_OK|MB_ICONERROR);
		return;
	}

	enum{ fsz=88*1024 }; if(!file) //new file?
	{
		file = new BYTE[fsz];
		GetFileTime(h,0,0,&filetime); //wasn't set?
	}
	else //ensure unfound PRF files don't wipeout ITEM.ARM?
	{
		(BYTE*&)old = file;
		for(int i=0;i<1024;i++) //not pruning
		if(tmp->records[i].my!=old->records[i].my)
		if(old->records[i].my)
		memcpy((void*)&tmp->records[i],&old->records[i],88);
	}
	memcpy(file,tmp,fsz);

	//this seems heavy handed but SOM_PRM reads the
	//regular profiles every time an item is edited	
	tmp->clear();

	DWORD wr = 0;
	WriteFile(h,file,fsz,&wr,0);
	CloseHandle(h);

	if(wr!=fsz) goto prm101;
}
void __cdecl SOM_PRM_41bf19()
{
	//"this" is a SOM_CDialog
	BYTE *p = (BYTE*)&SOM_PRM_47a708.page();
	int del = *(int*)(p+0x8c);

	SOM_PRM_arm.save(); //NEW

	if(del!=1) return; //NEW (don't write if no deletions)

	const char base[] = SOMEX_(B)"\\PARAM\\";
	char buf[64]; memcpy(buf,base,sizeof(base));
	const char *app[] = {"Item.prm","Shop.dat","Enemy.prm","Npc.prm","Sys.dat"};
	int app2[] = {0x104c,84000,0x15ad8,0x2ff00,0x59258,0x7a000,0xd3258,0x50000,0x123258,0x83e0};

	for(int i=0;i<5;i++)
	{
		strcpy(buf+sizeof(base)-1,app[i]);
		//som_tool_CreateFileA
		HANDLE h = CreateFileA(buf,GENERIC_WRITE,0,0,OPEN_EXISTING,0,0);
		if(h!=INVALID_HANDLE_VALUE)
		{
			DWORD wr = app2[i*2+1];
			WriteFile(h,p+app2[i*2],wr,&wr,0);
			CloseHandle(h);
		}
		else assert(0); //YIKES
	}
}

/*REFERENCE: MAY NEED LATER
#pragma runtime_checks("",off) // mov         ecx,0Bh
void __stdcall SOM_PRM_45A9FB(DWORD x, DWORD y, DWORD _cx, DWORD cy, DWORD clr)
{
	DWORD _this;	
	__asm
	{
		mov _this,ecx				
	}
	HDC dc = ((HDC*)_this)[1];
	
	//actually this needs to go in reverse unless the bitmap
	//held in dc is created larger
	//extern float som_tool_enlarge,som_tool_enlarge2;
	//XFORM xf =  {som_tool_enlarge2,0,0,som_tool_enlarge,0,0};		
	//SetGraphicsMode(dc,GM_ADVANCED); SetWorldTransform(dc,&xf);	
	
	//Doesn't appear to be necessary (what about in XP/Classic Mode?)
	//moving here from som_tool_subclassproc so not to repeat the
	//do_enlarge logic... plus it's crowded already
	//IntersectClipRect(dc,x,y,_cx,cy);

	//TODO: SOM_MAP.cpp has a way for dealing with __thiscall pointers
	__asm
	{
		mov ecx,_this
		push [clr]
		push [cy]
		push [_cx]
		push [y]
		push [x]
		mov eax,45A9FBh
		call eax
	}
}
#pragma runtime_checks("",restore)  
*/
//TODO: LET LANGUAGE PACKAGE MOVE THE GRAPH LOCATION
static BOOL WINAPI SOM_PRM_BitBlt(HDC dst, int xdst, int ydst, int wdst, int hdst, HDC src, int xsrc, int ysrc, DWORD rop)
{
	SetStretchBltMode(dst,COLORONCOLOR);
	extern float som_tool_enlarge,som_tool_enlarge2;
	xdst*=som_tool_enlarge2; ydst*=som_tool_enlarge;
	int w = wdst*som_tool_enlarge2, h = hdst*som_tool_enlarge;
	return SOM::Tool.StretchBlt(dst,xdst,ydst,w,h,src,xsrc,ysrc,wdst,hdst,rop);
}

struct SOM_PRM_this
{
	//SOM_PRM has a very annoying pattern where every call is
	//seemingly wrapped in a destructor object (RAII) that calls 
	//a "release" API (4566a7) on itself on exit (I can't tell
	//where the ref count gets increased)

	//NOTE: this==&SOM_PRM_47a708.page()
	DWORD __thiscall add_profile(BYTE*);
	//relative to?
	//+0x1586c/0x158c8 //items profile linked-list (size/ptr)
	//+0x140e0/0x1410c //magic
	//+0xe634/0xe6a4 //objects
	//global? range?
	//0x47A83C-0x47c83c //enemies (two pointers ea.)
	//0x47d83c-0x47f83c //npc
	BYTE *find_profile(int i, DWORD &span)
	{
		if(i==-1) return 0;
		char size[7] = {88,0,40,0,8,8,108};
		int sz = size[som_prm_tab-1001];
		span = sz;
		if(sz==8) //size/pointer pair?
		{
			sz = som_prm_tab==1005?0x47A838:0x47d838;
			return (BYTE*)(sz+i*8);
		}
		switch(som_prm_tab)
		{
		case 1001: sz = 0x158c8; break; //item
		case 1003: sz = 0x1410c; break; //magic
		case 1007: sz = 0xe6a4; break; //object
		default: assert(0); return 0;
		}
		BYTE *ll = *(BYTE**)((BYTE*)this+sz);
		while(i-->0) ll = *(BYTE**)(ll+span); return ll;
	}
}; 

extern void SOM_PRM_reprogram()
{
	//TODO: LET LANGUAGE PACKAGE MOVE THE GRAPH LOCATION
	if(1) //EX::INI::Editor ed; if(ed->do_enlarge)
	{
		//REFERENCE: MAY NEED LATER
		//45A9FB draws the gray background of the pie graphs
		//through some odd MFC like route
		//the HDC just needs to be moved
		//(Actually there is some legacy masking behavior that
		//is easier to do here)
		/*
		004253CF 68 C0 C0 C0 00       push        0C0C0C0h  
		004253D4 68 FA 00 00 00       push        0FAh  
		004253D9 68 0E 01 00 00       push        10Eh  
		004253DE 53                   push        ebx  
		004253DF 53                   push        ebx  
		004253E0 8D 4C 24 54          lea         ecx,[esp+54h]  
		004253E4 89 44 24 28          mov         dword ptr [esp+28h],eax  
		004253E8 E8 0E 56 03 00       call        0045A9FB 
		*/		
		//*(DWORD*)0x4253E9 = (DWORD)SOM_PRM_45A9FB-0x4253ED;		

		//BitBlt calls (from inside SOM_PRM... there are many calls)
		//00425683 FF 15 38 40 46 00    call        dword ptr ds:[464038h]
		*(WORD*)0x425683 = 0xE890;
		*(DWORD*)0x425685 = (DWORD)SOM_PRM_BitBlt-0x425689;		
		//not sure what this is
		//00422427 FF 15 38 40 46 00    call        dword ptr ds:[464038h]
//		*(WORD*)0x422427 = 0xE890;
//		*(DWORD*)0x422429 = (DWORD)SOM_PRM_BitBlt-0x42242D;		
		//00407E9D FF 15 38 40 46 00    call        dword ptr ds:[464038h]
		*(WORD*)0x407E9D = 0xE890;
		*(DWORD*)0x407E9F = (DWORD)SOM_PRM_BitBlt-0x407EA3;		
	}
	
		//I think (2017) this is a trick to get the address of
		//__thiscall subroutines
		//2021: switching to auto and pulling out of use sites
		#define _(mf) auto mf = &SOM_PRM_this::mf;
		_(add_profile)
		#undef _
	
	//2021: fix shared profiles update (and enemy/npc sizes)
	{
		//0041B8B3 E8 F8 04 00 00       call        0041BDB0
			//item
		//00420401 E8 EA 02 00 00       call        004206F0 
			//magic
		//0042B61E E8 8D 02 00 00       call        0042B8B0
			//object
		//this one is different and I fear the size of the
		//file isn't maintained
		//004034F4 E8 E7 02 SOM_PRM_items_mode 00       call        004037E0 
			//enemy
		//00426CA0 E8 7B 04 00 00       call        00427120
			//npc (same)		
		*(DWORD*)0x41B8B4 = (DWORD&)add_profile-0x41B8B8;
		*(DWORD*)0x420402 = (DWORD&)add_profile-0x420406;
		*(DWORD*)0x42B61f = (DWORD&)add_profile-0x42B623;
		*(DWORD*)0x4034F5 = (DWORD&)add_profile-0x4034F9;
		*(DWORD*)0x426CA1 = (DWORD&)add_profile-0x426CA5;
	}
	//2021: save PARAM/ITEM.ARM (+ beginning of move/delete help)
	{	
		memset((void*)0x41bf19,0x90,219);
		*(BYTE*)0x41bf19 = 0xe8; //call
		*(DWORD*)0x41bf1a = (DWORD)SOM_PRM_41bf19-0x41bf1e;
	}
}

extern DWORD som_tool_prf_size;
DWORD __thiscall SOM_PRM_this::add_profile(BYTE *cp)
{
	void *_this = this; //on stack I think (SOM_PRM_47a708?)
	SOM_CDialog *_cmp = &SOM_PRM_47a708.page(); 
	assert(_this==_cmp);

	//items,?,magic,shop,enemy,npc,object
	DWORD subs[7] = {0x41BDB0,0,0x4206F0,0,0x4037E0,0x427120,0x42B8B0};
	DWORD sz = subs[som_prm_tab-1001];

	som_tool_prf_size = 0;
	DWORD ret = ((DWORD(__thiscall*)(void*,BYTE*))sz)(this,cp);
	BYTE *prf = find_profile(ret,sz);

	if(prf) if(sz==8)
	{
		//NOTE: som_tool_prf_size is an optimization
		//(not really) to avoid reallocating/reading
		//most of the time
		bool enemy = som_prm_tab==1005;
		DWORD &fsz = *(DWORD*)(cp+(enemy?0xe0:0x60));
		if(!som_tool_prf_size||fsz!=som_tool_prf_size)
		{
			char *fmt = *(char**)(enemy?0x47a7ec:0x47A7DC);
			char buf[MAX_PATH];
			sprintf(buf,fmt,cp+31);
			//som_tool_CreateFileA
			HANDLE h = CreateFileA(buf,SOM_TOOL_READ);
			assert(h!=INVALID_HANDLE_VALUE);			
			*(DWORD*)prf = fsz = GetFileSize(h,0);
			void* &ptr = *(void**)(prf+4);			
			((void(*)(void*))0x456411)(ptr); //free (realloc?)
			ptr = ((void*(*)(DWORD))0x4563e8)(fsz); //malloc
			ReadFile(h,ptr,fsz,&fsz,0);
			assert(fsz==som_tool_prf_size);
			CloseHandle(h);
		}
	}
	else memcpy(prf,cp,sz); return ret;
}