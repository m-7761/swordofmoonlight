
#include "Somplayer.pch.h"

#include <commdlg.h> //ChooseColor
#include <richedit.h>

#include "../Sompaste/Sompaste.h"

static SOMPASTE Sompaste = 0;

#include "Somthread.h"
#include "Somcontrol.h"
#include "Somwindows.h"

namespace Somwindows_cpp
{
	struct tabprops
	:
	public Somwindows_h::windowman 
	{		
		HWND propsheet; 

		tabprops(HWND wm, HWND ps=0) 
		: 
		Somwindows_h::windowman(wm,"tearoff")
		{
			propsheet = ps;
		}
		~tabprops() 
		{
			delete props; props = 0;
		}

		inline HWND tabs()
		{
			return PropSheet_GetTabControl(propsheet);
		}  
		int tab()
		{	
			return TabCtrl_GetCurSel(tabs());
		}
	};

	//tex here is short for texture
	struct texprops : public tabprops
	{
		bool noted; //notepad

		wchar_t source[MAX_PATH];
		wchar_t rename[MAX_PATH];

		const Somtexture *pal; //NEW

		struct lvchannel
		{
			int id; //const: want default constructors

			static const int bytes_s = 64;

			wchar_t *palid, *primary, bytes[bytes_s], *units, *tip; 
			
			bool use, add, deferuse, deferadd; 

			lvchannel(){}
			lvchannel(int ch, const Somtexture *pal):id(ch)
			{			
				switch(ch)
				{
				case pal->RGB:	 palid=L"IMAGE"; units=L"3 Bpp";  tip = L"Per pixel RGB"; break;
				case pal->PAL:	 palid=L"TONES"; units=L"4 Byte"; tip = L"Tones palette"; break;
				case pal->INDEX: palid=L"INDEX"; units=L"1 Bpp";  tip = L"Painting mask"; break;
				case pal->BLACK: palid=L"BLACK"; units=L"9 bit";  tip = L"TXR colorkey";  break;
				case pal->ALPHA: palid=L"ALPHA"; units=L"1 Bpp";  tip = L"Opacity mask";  break;

				default: assert(0);
				}

				deferuse = use = ch==pal->primary();
				deferadd = add = pal->channel(ch); 

				*this = pal; //...
			}
			void operator=(const Somtexture *pal) //update primary/bytes
			{	
				primary = add?(use?L"yes":L"no"):L"--";

				if(add)
				{
					wchar_t temp[bytes_s] = L""; 
					_itow(pal->memory(id,id),temp,10);
									
					NUMBERFMTW nf = {0,0,3,L".",L",",1}; //. should not appear

					//default behavior adds .00 to the end for for no reason at all
					if(!GetNumberFormatW(LOCALE_USER_DEFAULT,0,temp,&nf,bytes,bytes_s))
					{
						wcscpy(bytes,L"NaN"); //paranoia
					}
				}
				else wcscpy(bytes,L"--");
			}
			bool operator!=(const Somtexture *pal) 
			{
				//TODO: look into bool to integer equality
				if(pal->primary()==id&&!deferuse) return true;
				if((bool)pal->channel(id)!=deferadd) return true;

				return false;
			}
			void operator()(const Somtexture *pal)
			{					
				if(add) pal->add(id); else pal->remove(id);
				if(use) pal->use(id); 

				deferuse = use = id==pal->primary();
				deferadd = add = pal->channel(id); 

				*this = pal;
			}	
		};

		int primary, channel; 

		static const int lv_s = 5; lvchannel lv[lv_s+1]; 

		inline int lvi(int ch)
		{
			int i; for(i=0;i<lv_s;i++) if(lv[i].id==ch) break; return i;
		}

		texprops(HWND wm, HWND ps) : tabprops(wm,ps)
		{				
			Somwindows_h::textureman tm(wm);

			*source = *rename = 0; noted = false; 

			pal = tm?tm->pal->addref():0; assert(pal); 
			
			lv[0] = lvchannel(pal->RGB,pal);
			lv[1] = lvchannel(pal->PAL,pal);
			lv[2] = lvchannel(pal->INDEX,pal);
			lv[3] = lvchannel(pal->BLACK,pal);
			lv[4] = lvchannel(pal->ALPHA,pal);

			memset(lv+5,0x00,sizeof(lvchannel)); //paranoia

			channel = primary = lvi(pal->primary());
		}
		~texprops()
		{
			pal->release();
		}
		bool apply(bool force=false)
		{
			if(!force) for(int i=0;i<lv_s;i++) if(lv[i]!=pal) return false;

			//for(int i=0;i<lv_s;i++) assert((primary==i)==lv[i].use); //???

			for(int i=0;i<lv_s;i++) lv[i](pal); return true;
		}
	};

	//// controller dialog notes /////////////////
	//
	// We want the player to be able to experiment
	// with the controls without pressing an Apply 
	// or OK button so the changes are immediately
	// in effect but will be reversed if Cancelled
	//
	// We also want the player to feel comfortable
	// working with multiple tabs at the same time
	// The only down to earth way to go about that
	// is to make duplicate property sheets appear
	// to be an alternative view of the same sheet

	//hid for Human Interface Device
	struct hidprops : public tabprops
	{	
	private: //mindmeld

		struct multitap 
		{			
			std::list<hidprops*> props; 
						
			Somcontrol_h::stack ports[Somconsole::ports];
			Somcontrol_h::stack cancel[Somconsole::ports];

			void inform(hidprops *p) //assuming unique
			{
				props.push_back(p); p->ports = ports; 
			}			
		};

		//may one day need to be broader than Somconsole* 
		typedef std::map<Somconsole*,multitap*> mindmeld;
						
		typedef mindmeld::iterator multitap_it; 
		
		static mindmeld multitaps; 

	public: const Somcontrol *tap; 

		Somcontrol_h::stack *ports;

		//[] conflicts with windowman's HWND conversion (HWND is a pointer)
		inline Somcontrol_h::stack &operator()(size_t i){ return ports[i]; }
			
		hidprops(HWND wm, HWND ps) : tabprops(wm,ps)
		{
			Somthread_h::section cs; assert(ps);

			multitap_it it = multitaps.find(props->console);

			if(it==multitaps.end()) 
			{
				multitap *p = new multitap;

				multitaps[props->console] = p; p->inform(this);
				
				tryout(); //hack: refresh tabs			

				for(int i=0;i<Somconsole::ports;i++) 
				{
					if(ports[i]) p->cancel[i] = ports[i];
				}				
			}
			else //hack: synchronize Apply buttons
			{				
				HWND ps0 = it->second->props.front()->propsheet;

				if(IsWindowEnabled(GetDlgItem(ps0,ID_APPLY_NOW)))				
				{
					EnableWindow(GetDlgItem(ps,ID_APPLY_NOW),1);
				}

				it->second->inform(this); //!
			}

			memset(buttons,0x00,sizeof(buttons));

			tap = 0; post = -1; //monitoring			
		}
		~hidprops() 
		{
			Somthread_h::section cs; 

			multitap *p = multitaps[props->console];

			p->props.remove(this); if(!p->props.empty()) return;

			delete p; multitaps.erase(props->console);
		}
		
		void tryout(int port=-1) //PSM_CHANGED
		{
			Somthread_h::section cs; 
			
			if(consolelock()) 
			{
				if(port==-1) //multitap
				{
					int i, j; Somcontrol *p = 0; 
					for(i=0,j=0;i<Somconsole::ports;i++) 
					{
						if(p=props->console->control(i))
						{						
							if(*p) ports[j++] = *p; 
						}
						else if(!ports[j++].unplug()) break;
					}
				}
				else //try the configuration
				{
					//overkill: usually one button would suffice
					Somcontrol *q = props->console->tap(ports+port);

					if(q&&q->portholder==ports[port].portholder) 
					{
						if(htic>=0&&htic<q->CONTEXTS) 
						{
							q->contexts[htic] = ports[port].contexts[htic];
						}
						else assert(0); //paranoia
					}
					else assert(0); //not good
				}

				props->console->unlock();
			}
			
			multitap *p = multitaps[props->console];

			std::list<hidprops*>::iterator it = p->props.begin();

			while(it!=p->props.end()) //update any open propsheets
			{
				hidprops &hid = **it++; 

				HWND mtap = PropSheet_IndexToHwnd(hid.propsheet,0);
				HWND page = PropSheet_GetCurrentPageHwnd(hid.propsheet);

				if(hid.buttons[0]) //hack: differentiate between tab types
				{
					if(port==-1) hid.htic = -1; //hack: force full refresh

					//TODO: might want to filter the selected tab according to port
					if(page!=mtap) SendMessage(page,WM_VSCROLL,TB_THUMBPOSITION,0);
				}
				else if(port==-1) SendMessage(mtap,WM_COMMAND,IDC_REFRESH,0);
				
				if(IsWindowVisible(page)) //hack: see tryout in ctor
				{
					PropSheet_Changed(hid.propsheet,page);
				}
			}
		}
		void apply() //PSN_APPLY
		{
			Somthread_h::section cs; 

			multitap *p = multitaps[props->console];
						
			Somconsole *c = consolelock(); if(!c) return;

			for(int i=0;i<Somconsole::ports;i++) 
			{
				//write controller configurations to registry			
				ports[i].apply(c->player(ports[i].portholder));

				if(ports[i]||p->cancel[i]) p->cancel[i] = ports[i];
			}
			
			c->apply(); c->unlock(); //write multitap to registry

			std::list<hidprops*>::iterator it = p->props.begin();

			while(it!=p->props.end()) //hack: synchronize Apply buttons
			{
				EnableWindow(GetDlgItem((*it++)->propsheet,ID_APPLY_NOW),0);
			}
		}
		void cancel() //PSN_QUERYCANCEL
		{
			Somthread_h::section cs; 

			multitap *p = multitaps[props->console];

			if(p->props.size()>1) return; //there can be only one

			if(!consolelock()) return;			

			//assuming ~hidprops is imminent
			for(int i=0;i<Somconsole::ports;i++) 
			{
				Somcontrol *q = props->console->control(i); if(!q) break;
				
				if(*q||p->cancel[i]) *q = p->cancel[i];
			}

			props->console->unlock();
		}

	  //// Multitap tabs do not use these ////

		static const size_t buttons_s = 10;
		
		HWND buttons[buttons_s];

		int vtic, htic, post;		
	};

	hidprops::mindmeld hidprops::multitaps;
};

//SCHEDULED FOR REMOVAL
static void Somwindows_BLACK(HWND win, int mode) //UTILITY
{
	struct _
	{
		static COLORREF zero(COLORREF in){ return 0; }
		static COLORREF gray(COLORREF in){ return 0x080808; }
		static COLORREF tone(COLORREF in)
		{
			if(in==0) return 0x080808; //0x050408; //Is blue darker?
			float x = (in&0xFF), y = (in>>8&0xFF), z = (in>>16&0xFF);
			float r = 8.5f/sqrt(x*x+y*y+z*z); x*=r; y*=r; z*=r;
			return int(x)+int(y)<<8+int(z)<<16;
		}
		static COLORREF rand(COLORREF)
		{
			//The legacy holes are usually quantized to be black. I
			//worry that uniform gray will stand out more than the 
			//neighboring pixels, and averages might not contrast.
			return 8<<::rand()%3*8|8<<::rand()%3*8; 
		}
	};
	COLORREF (*f)(COLORREF); switch(mode)
	{
	case ID_EDIT_COLORKEY: f = _::zero; break;
	case ID_EDIT_DARKGRAY: f = _::gray; break;
	case ID_EDIT_DARKTONE: f = _::tone; break;
	case ID_EDIT_DARKRAND: f = _::rand; break;
	}

	Somwindows_h::windowman wm(win);
	Somwindows_h::textureman tm(wm);
	
	if(!tm.image->pal->where_BLACK(Somtexture::RGB,f)
	 &&!tm.image->pal->where_BLACK(Somtexture::PAL,f))
	{
		MessageBeep(-1);
	}

	//And the palette window????????????????
	#ifndef _DEBUG
	#error This is an issue. Somtexture had plans for a notification loop.
	#endif
	InvalidateRect(wm,0,0); //2017: Just need to repair the MSM textures...
}

static BOOL CALLBACK SomwindowsResizeEnum(HWND window, LPARAM lparam=0)
{
	if(lparam&&GetParent(window)!=(HWND)lparam) return TRUE;

	RECT client; GetClientRect(window,&client);

	SendMessage(window,WM_SIZE,SIZE_RESTORED,MAKELPARAM(client.right,client.bottom));

	return TRUE;
}

//hack: these are updated later
static int Somwindows_tonepads[2] = {8,4};

static const int Somwindows_tonepad = 4;
static const int Somwindows_toneseps[2] = {2,0};
static const int Somwindows_tonesizes[2] = {12,28};

//scheduled obsolete: we will find a better way
static HWND Somwindows_tonetemplates[2] = {0,0};

static LRESULT CALLBACK SomwindowsTonesProc(HWND,UINT,WPARAM,LPARAM,UINT_PTR,DWORD_PTR);
static LRESULT CALLBACK SomwindowsEraseProc(HWND,UINT,WPARAM,LPARAM,UINT_PTR,DWORD_PTR);

static int Somwindows_New_Tone(HWND win, int well, int tone, COLORREF ref=0) 
{		
	assert(tone>=0&&tone<=255);

	const int cksz = Somwindows_tonesizes[0];
	const int tzsz = Somwindows_tonesizes[1]; 

	//TODO: OOP copy framework for this stuff

	static HWND &ck = Somwindows_tonetemplates[0];
	static HWND &tz = Somwindows_tonetemplates[1];

	static DWORD ckexstyle = GetWindowLong(ck,GWL_EXSTYLE);
	static DWORD tzexstyle = GetWindowLong(tz,GWL_EXSTYLE);

	static DWORD ckstyle = GetWindowLong(ck,GWL_STYLE);
	static DWORD tzstyle = GetWindowLong(tz,GWL_STYLE);

	HWND tm = GetDlgItem(win,IDC_TONE_MAP);
	HWND wz = GetDlgItem(win,IDC_UNTITLED+well);
		
	//hack (seriously should not be here)
	HWND bg = GetDlgItem(tm,IDC_BACKGROUND); 
	if(!bg) SetParent(GetDlgItem(win,IDC_BACKGROUND),tm); //whole group box

	HMENU id = (HMENU)(tone+1000); //Win32 craziness
		
	if(GetDlgItem(tm,(int)id)){ assert(0); return (int)id; } //sanity check

	HWND nck = CreateWindowEx(ckexstyle,"STATIC",0,ckstyle,0,0,cksz,cksz,tm,id,0,0);
	HWND ntz = CreateWindowEx(tzexstyle,"STATIC",0,tzstyle,0,0,tzsz,tzsz,wz,id,0,0);
	
	//The Color Chooser can't set the subclass 
	//data across threads. USERDATA is lighter.
	SetWindowSubclass(nck,SomwindowsEraseProc,0,0); //ref&0xFFFFFF);
	SetWindowLong(nck,GWL_USERDATA,ref&0xFFFFFF);
	SetWindowSubclass(ntz,SomwindowsEraseProc,0,0); //ref&0xFFFFFF);
	SetWindowLong(ntz,GWL_USERDATA,ref&0xFFFFFF);	

	ShowWindow(nck,SW_SHOW); ShowWindow(ntz,SW_SHOW);

	return nck&&ntz?(int)id:0;
}

static HWND Somwindows_New_Well(HWND win, int num, const wchar_t *title)
{		
	HWND ut = GetDlgItem(win,IDC_UNTITLED); 
	HWND nw = GetDlgItem(win,IDC_UNTITLED+num); assert(ut&&!nw);
	
	if(!ut||nw) return 0;

	DWORD exstyle = GetWindowLong(ut,GWL_EXSTYLE);		
	DWORD style = GetWindowLong(ut,GWL_STYLE)|WS_VISIBLE;
	WORD atom = GetClassLong(ut,GCW_ATOM);

	HMENU id = (HMENU)(IDC_UNTITLED+num); //Win32 craziness
		
	extern bool Sommctrl_position(RECT*,HWND,HWND);

	RECT pos, hack; Sommctrl_position(&pos,win,ut); GetClientRect(win,&hack);
	nw = CreateWindowExW(exstyle,(wchar_t*)atom,title,style,pos.left,pos.top+hack.bottom,0,0,win,id,0,0);

	Somwindows_h::windowman wm(win);

	if(wm&&wm->font) SendMessage(nw,WM_SETFONT,(WPARAM)wm->font,MAKELPARAM(1,0)); 

	SetWindowSubclass(nw,SomwindowsTonesProc,0,0);
	
	return nw; 
}

static INT_PTR CALLBACK SomwindowsPaletteInit(HWND win, UINT Msg, WPARAM, LPARAM)
{
	//scheduled obsolete: there's not much left really

	switch(Msg)
	{
	case WM_INITDIALOG:

		HWND ck = GetDlgItem(win,IDC_COLORKEY);
		HWND tz = GetDlgItem(win,IDC_TONE_ZERO);
		HWND tm = GetDlgItem(win,IDC_TONE_MAP);
		HWND wz = GetDlgItem(win,IDC_UNTITLED);
		HWND nw = GetDlgItem(win,IDC_NEW_WELL);
		HWND nt = GetDlgItem(win,IDC_NEW_TONE);

		if(*Somwindows_tonetemplates)
		{
			DestroyWindow(ck); DestroyWindow(tz); //ditch
		}
		else
		{
			SetParent(Somwindows_tonetemplates[0]=ck,0);
			SetParent(Somwindows_tonetemplates[1]=tz,0);
		}

		SetWindowSubclass(tm,SomwindowsTonesProc,0,0);
		SetWindowSubclass(wz,SomwindowsTonesProc,0,0);

		return FALSE;
	}

	return 0;
}

static INT_PTR CALLBACK SomwindowsMultitapTab(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg)
	{
	case WM_INITDIALOG:
	{			
		HWND ps = GetParent(hWndDlg);

		Somwindows_h::windowman wm((HWND)lParam);	
		
		if(!wm||!wm->portholder) goto close;

		EnableThemeDialogTexture(hWndDlg,ETDT_ENABLETAB);
		
		Somwindows_cpp::hidprops *props = new Somwindows_cpp::hidprops(wm,ps);

		SetWindowLongPtrW(hWndDlg,GWLP_USERDATA,(LONG_PTR)props);

		props->window = (HWND)lParam;

		HWND tabs = PropSheet_GetTabControl(ps);

		TCITEMW tab = { TCIF_TEXT,0,0,L"Multitap",0,0};
		SendMessageW(tabs,TCM_SETITEMW,0,(LPARAM)&tab);
				
		HWND lv = GetDlgItem(hWndDlg,IDC_LISTVIEW);		
		DWORD lvs_ex_style = LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_INFOTIP;	  
		ListView_SetExtendedListViewStyle(lv,lvs_ex_style);

		//TODO: figure out why classic theme does not show
		//HWND lvh = (HWND)SendMessage(lv,LVM_GETHEADER,0,0);

		LVCOLUMNW lvc = {LVCF_TEXT|LVCF_FMT|LVCF_WIDTH};  
		lvc.fmt = LVCFMT_LEFT;

		RECT room; LONG &remaining = room.right; 			
		GetClientRect(lv,&room);

		lvc.pszText = L"Controller"; remaining-=lvc.cx = 62; 
		SendMessage(lv,LVM_INSERTCOLUMNW,0,(LPARAM)&lvc);
		lvc.pszText = L"#"; remaining-=lvc.cx = 20;
		SendMessage(lv,LVM_INSERTCOLUMNW,1,(LPARAM)&lvc);
		lvc.pszText = L"Peripheral"; lvc.cx = remaining; 
		SendMessage(lv,LVM_INSERTCOLUMNW,2,(LPARAM)&lvc); 	

		SendMessage(hWndDlg,WM_COMMAND,IDC_REFRESH,0);

		ShowWindow(hWndDlg,SW_SHOW); 

		return FALSE;
	}		 
	case WM_NOTIFY:
	{
		PSHNOTIFY *p = (PSHNOTIFY*)lParam;

		switch(p->hdr.code)
		{
		case NM_SETFOCUS: //FALL THRU

			if(p->hdr.idFrom!=IDC_LISTVIEW) break; 
	
			//wait for focus before turning on ch buttons...

		case LVN_ITEMCHANGED: 
		{	
			//pre-initialization?
			if(!IsWindowVisible(hWndDlg)) break; 
					
			Somwindows_cpp::hidprops &props =
			*(Somwindows_cpp::hidprops*)GetWindowLongPtrW(hWndDlg,GWLP_USERDATA);
			
			HWND lv = GetDlgItem(hWndDlg,IDC_LISTVIEW);	

			bool use = false, add = false, remove = false;

			int i = ListView_GetNextItem(lv,-1,LVNI_SELECTED);
			
			LVITEM lvi = {LVIF_PARAM};

			while(i!=-1) 
			{
				lvi.iItem = i;

				i = ListView_GetNextItem(lv,i,LVNI_SELECTED);

				if(ListView_GetItem(lv,&lvi))
				{
					if(lvi.lParam) remove = true; else add = true;

					if(lvi.lParam!=props->portholder) use = true;
				}				
			}

			EnableWindow(GetDlgItem(hWndDlg,IDC_USE),use); 
			EnableWindow(GetDlgItem(hWndDlg,IDC_ADD),add); 				
			EnableWindow(GetDlgItem(hWndDlg,IDC_REMOVE),remove); 

			break;
		}
		case PSN_SETACTIVE:
		
			SetWindowTextW(GetParent(hWndDlg),L"Somplayer.dll - Port Authority");

		case PSN_KILLACTIVE: //FALLING THRU
		{
			ShowWindow(hWndDlg,p->hdr.code==PSN_SETACTIVE?SW_SHOW:SW_HIDE);
		
			SetWindowLong(hWndDlg,DWL_MSGRESULT,FALSE); return TRUE;
		}
		case PSN_APPLY:
		case PSN_QUERYCANCEL:
		{				
			Somwindows_cpp::hidprops &props =
			*(Somwindows_cpp::hidprops*)GetWindowLongPtrW(hWndDlg,GWLP_USERDATA);

			if(p->hdr.code==PSN_APPLY) props.apply(); else props.cancel();

		}break;		
		}

	}break;
	case WM_COMMAND:
	{
		Somwindows_cpp::hidprops &props =
		*(Somwindows_cpp::hidprops*)GetWindowLongPtrW(hWndDlg,GWLP_USERDATA);

		if(!&props) return 0; //hack: pre WM_INITDIALOG

		switch(LOWORD(wParam))
		{				
		case ID_REFRESH: 
		{					
			if(Somcontrol::multitap(Somcontrol::DISCOVER))
			{
				props.tryout(); //aka: IDC_REFRESH 
			}

		}break;
		case IDC_REFRESH: //hack: this is a hidden button
		{
			HWND lv = GetDlgItem(hWndDlg,IDC_LISTVIEW);		
			
			ListView_DeleteAllItems(lv);

			HWND tabs = props.tabs();
			
			const Somcontrol *p = props.ports;
			const Somcontrol *q = Somcontrol::multitap(0);

			wchar_t *unplugged = (wchar_t*)L"unplugged";

			static const size_t text_s = 48; wchar_t text[text_s] = L""; 

			//LVIF_PARAM should be obsolete at this point
			LVITEMW lvi = {LVIF_TEXT|LVIF_STATE|LVIF_PARAM};
			TCITEMW tci = {LVIF_TEXT|TCIF_PARAM,0,0,text,text_s,0,0};

			for(size_t i=0,j=0,k=0;q;i++)
			{	
				lvi.iItem = i; lvi.lParam = 0;
								
				if(p&&p->tap==q->tap)
				{					
					if(p->portholder
					 &&props.consolelock())
					{
						tci.lParam = p->tap; 
						lvi.lParam = p->portholder;					
											
						swprintf_s(text,L"%d  %s",k+1,q->device());				

						SendMessageW(tabs,TCM_SETITEMW,k+1,(LPARAM)&tci);

						lvi.pszText = (wchar_t*)props->console->player(p->portholder);

						k++; props->console->unlock();
					}
					else lvi.pszText = unplugged;

					p = ++j<Somconsole::ports?p+1:0;
				}	
				else lvi.pszText = unplugged;
						
				lvi.iSubItem = 0; //Controller

				SendMessage(lv,LVM_INSERTITEMW,0,(LPARAM)&lvi);	
				
				lvi.iSubItem = 1; //#

				if(lvi.pszText!=unplugged)
				{
					lvi.pszText = text; swprintf_s(text,L"%d",k); 
				}
				else lvi.pszText = L"";
												
				SendMessageW(lv,LVM_SETITEMTEXTW,i,(LPARAM)&lvi);

				lvi.iSubItem = 2; //Peripheral
				
				lvi.pszText = (wchar_t*)q->device(); 

				if(*q->device2()) assert(0); //TODO: put device2 in parenthesis

				SendMessage(lv,LVM_SETITEMTEXTW,i,(LPARAM)&lvi);				

				if(q=Somcontrol::multitap(i+1)) continue;

				for(tci.lParam=-1;k<Somconsole::ports;k++)
				{
					swprintf_s(text,L"%d  ",k+1);				
					SendMessageW(tabs,TCM_SETITEMW,k+1,(LPARAM)&tci);
				}
			} 

			//hack: could preserve selection
			EnableWindow(GetDlgItem(hWndDlg,IDC_USE),FALSE); 
			EnableWindow(GetDlgItem(hWndDlg,IDC_ADD),FALSE); 				
			EnableWindow(GetDlgItem(hWndDlg,IDC_REMOVE),FALSE);

			//black magic: seems necessary
			InvalidateRect(hWndDlg,0,TRUE);

		}break;	
		case IDC_USE: case IDC_ADD: case IDC_REMOVE:
		{
			HWND lv = GetDlgItem(hWndDlg,IDC_LISTVIEW);	

			int i = ListView_GetNextItem(lv,-1,LVNI_SELECTED);			
			
			int user = LOWORD(wParam)==IDC_USE?props->portholder:-1;

			if(props.consolelock())
			{				
				const Somcontrol *p; Somcontrol *q;

				while(i!=-1) 
				{
					p = Somcontrol::multitap(i);
					i = ListView_GetNextItem(lv,i,LVNI_SELECTED);
					q = props->console->tap(p,user);

					if(LOWORD(wParam)==IDC_REMOVE) q->unplug();
				}

				props->console->unlock();
			}

			props.tryout(); //aka: IDC_REFRESH 

		}break;
		}
		
	}break;
	case WM_DESTROY: //WM_NCDESTROY
	{	
	close: 

		delete (Somwindows_cpp::hidprops*)GetWindowLongPtrW(hWndDlg,GWLP_USERDATA);				
		
		if(Msg!=WM_DESTROY) DestroyWindow(hWndDlg); //WM_NCDESTROY
	}
	}//switch

	return 0;
}

static LRESULT CALLBACK SomwindowsButtonEdit(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scid, DWORD_PTR param) 
{
	switch(uMsg)
	{		
	case WM_CONTEXTMENU: //prevent built-in context menu
		
		//defer to the parent menu (SomwindowsButton) below
		return SendMessage(GetParent(hwnd),uMsg,wParam,lParam);
		
	case WM_GETDLGCODE: //try to prevent Enter from ending the dialog
				
		return 137|DLGC_WANTALLKEYS;  //hack: emulate Rich Edit control
	
	case WM_NCDESTROY: 
	
		RemoveWindowSubclass(hwnd,SomwindowsButtonEdit,scid);
		break;
	}

	return DefSubclassProc(hwnd,uMsg,wParam,lParam);
}

static LRESULT CALLBACK SomwindowsButtonButton(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scid, DWORD_PTR param) 
{
	switch(uMsg)
	{		
	case WM_LBUTTONUP:
	case WM_LBUTTONDOWN: return 1;

	case WM_CONTEXTMENU: //prevent built-in context menu
		
		//defer to the parent menu (SomwindowsButton) below
		return SendMessage(GetParent(hwnd),uMsg,wParam,lParam);
		
	case WM_GETDLGCODE: //try to prevent Enter from ending the dialog
				
		return 0;  //hack: disable button (no force feedback)
	
	case WM_NCDESTROY: 
	
		RemoveWindowSubclass(hwnd,SomwindowsButtonButton,scid);
		break;
	}

	return DefSubclassProc(hwnd,uMsg,wParam,lParam);
}

static INT_PTR CALLBACK SomwindowsPower(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg)
	{
	case WM_INITDIALOG:
	
		//EnableThemeDialogTexture(hWndDlg,ETDT_ENABLETAB);

		SetDlgItemText(hWndDlg,IDC_HELPBOX,
		"For mice and keyboards it is necessary to choose a \"power\" button.\r\n"
		"\r\n"
		"1. The power function switches between video-game-like play mode and \"desktop\" mode.\r\n"
		"\r\n"
		"2. Without a power button a mouse would be unable to exit video game play mode.\r\n"
		"\r\n"
		"3. A keyboard in play mode is unable to directly access \"hotkey\" shortcuts to popup menu items.\r\n"		
		"\r\n"
		"4. Alternatively a master switch is possible by assigning a so-called \"System\" power function to peripherals.\r\n"
		"\r\n"
		"5. Power buttons can interfere with desktop mode. Sometimes finding an ideal arrangement can come down to trial and error.\r\n"
		"\r\n"
		"6. Any peripheral can use a power button to put itself into standby mode and or gain access to system menus.\r\n"
		"\r\n"
		"If you have any questions or comments please direct them to the WWW.SWORDOFMOONLIGHT.NET internet forums.");	
		Sompaste->center(hWndDlg);
		return 1;

	case WM_COMMAND:

		switch(LOWORD(wParam))
		{
		case IDOK: case IDCANCEL: goto close;
		}
		break;

	case WM_CLOSE:

	close: EndDialog(hWndDlg,0);
	}
	return 0;
}

static INT_PTR CALLBACK SomwindowsButton(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg)
	{
	case WM_INITDIALOG:
	{
		SetWindowSubclass(GetDlgItem(hWndDlg,IDC_EDIT),SomwindowsButtonEdit,0,0);
		SetWindowSubclass(GetDlgItem(hWndDlg,IDC_BUTTON),SomwindowsButtonButton,0,0); 

	}break;
	case WM_CONTEXTMENU:
	{	
		int bt = -1+GetDlgItemInt(hWndDlg,IDC_BUTTON,0,0);

		Somwindows_cpp::hidprops &props =
		*(Somwindows_cpp::hidprops*)GetWindowLongPtrW(GetParent(hWndDlg),GWLP_USERDATA);
		
		int port = props.tab()-1; if(port<0||port>=Somconsole::ports) break;

		Somcontrol_h::stack &hid = props(port); if(!hid) break;

		int ctx = props.htic, cbt = hid.contexts_button(bt);

		if(ctx<0||ctx>=hid.CONTEXTS
		 ||cbt<0||cbt>=Somcontrol_h::BUTTONS) break; //bt may be -1

		int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

		static HMENU menu = //hack
		LoadMenu(Somplayer_dll(),MAKEINTRESOURCE(IDR_BUTTON_MENU)); 	 
		
		BOOL id = TrackPopupMenu(GetSubMenu(menu,0),TPM_RETURNCMD,x,y,0,hWndDlg,0);

		switch(id)
		{
		case 0: return 1; //cancelled popup menu

		case ID_HELP_POWER:

			DialogBox(Somplayer_dll(),MAKEINTRESOURCE(IDD_POWER_HELP),hWndDlg,SomwindowsPower);
			return 0;

		case ID_INVERT: //swap odd/even assignments 
		{
			int compile[Somcontrol_h::BUTTONS%2==0];
			int compile2[Somcontrol_h::ACTIONS%2==0];
									
			if(bt%2) cbt = hid.contexts_button(--bt);
						
			if(!*hid.button_label(bt)) break; //paranoia
			if(!*hid.button_label(bt+1)) break; //odd button

			int inv = hid.contexts_button(bt+1); //opposite left

			std::swap(hid.contexts[ctx][cbt],hid.contexts[ctx][inv]);	

		}break;		
		default: hid.contexts[ctx][cbt] = id==ID_BLANK?0:id; 
		}

		props.tryout(port); 

		return 1;

	}break;
	}

	return 0;
}

static INT_PTR CALLBACK SomwindowsDeviceTabs(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg)
	{
	case WM_INITDIALOG:
	{			
		HWND ps = GetParent(hWndDlg);

		Somwindows_h::windowman wm((HWND)lParam);	
		
		if(!wm||!wm->portholder) goto close;

		EnableThemeDialogTexture(hWndDlg,ETDT_ENABLETAB);
		
		Somwindows_cpp::hidprops *props = new Somwindows_cpp::hidprops(wm,ps);

		SetWindowLongPtrW(hWndDlg,GWLP_USERDATA,(LONG_PTR)props);

		//horizontal trackbar: set it and forget it
		SendDlgItemMessage(hWndDlg,IDC_CONTEXT,TBM_SETRANGEMAX,TRUE,SOMCONTROL_CONTEXTS-1);

		props->window = (HWND)lParam;
	
		//ShowWindow(hWndDlg,SW_SHOW); 

		return FALSE;
	}		
	case WM_NOTIFY:
	{
		PSHNOTIFY *p = (PSHNOTIFY*)lParam;
				
		switch(p->hdr.code)
		{	
		case PSN_SETACTIVE: //FALLS THRU
		
			//TODO: acquire an actual context descriptor
			SetWindowTextW(GetParent(hWndDlg),L"Somplayer.dll - Primary Controls");
						
		case PSN_KILLACTIVE: //FALLING THRU
		{	
			Somwindows_cpp::hidprops &props =
			*(Somwindows_cpp::hidprops*)GetWindowLongPtrW(hWndDlg,GWLP_USERDATA);

			props.vtic = props.htic = -1;
			SendMessage(hWndDlg,WM_VSCROLL,TB_THUMBPOSITION,0); //does the work

			//hack: if so the tab will still be non-zero at this point
			if(p->hdr.code==PSN_KILLACTIVE) ShowWindow(hWndDlg,SW_HIDE);
		
			SetWindowLong(hWndDlg,DWL_MSGRESULT,FALSE); return TRUE;
		}
		case PSN_APPLY: case PSN_QUERYCANCEL:
		{
			//should be covered by SomwindowsMultitapTab's PSN_APPLY

		}break;		
		}

	}break;
	case WM_VSCROLL: case WM_HSCROLL:
	{
		if(LOWORD(wParam)==TB_THUMBTRACK) break;
				
		Somwindows_cpp::hidprops &props =
		*(Somwindows_cpp::hidprops*)GetWindowLongPtrW(hWndDlg,GWLP_USERDATA);

		int tab = props.tab(); const Somcontrol *cmp = 0;
		
		if(tab)	cmp = Somcontrol::multitap(props(tab-1).tap);

		if(props.tap!=cmp) //changing controllers
		{				
			props.tap->stop_messages_to(hWndDlg,props.post);
			props.post = cmp->post_messages_to(hWndDlg,WM_USER,0);
			props.tap = cmp;
						
			size_t btx = cmp->button_count()+cmp->action_count()*2;
			
			//vertical track bar: one page per 10 button child dialogs
			SendDlgItemMessage(hWndDlg,IDC_SCREEN,TBM_SETRANGEMAX,TRUE,btx/10+(btx%10?1:0)-1);	
		}
		
		ShowWindow(hWndDlg,cmp?SW_SHOW:SW_HIDE); if(!cmp) break;

		size_t hpos = SendDlgItemMessage(hWndDlg,IDC_CONTEXT,TBM_GETPOS,0,0);
		size_t vpos = SendDlgItemMessage(hWndDlg,IDC_SCREEN,TBM_GETPOS,0,0);

		bool flicker = props.vtic==vpos&&props.htic==hpos; //hack...

		props.vtic = vpos; props.htic = hpos; assert(hpos<=SOMCONTROL_CONTEXTS);

		Somcontrol_h::stack &hid = props(tab-1); if(!hid) break;

		HWND bt; RECT rect; 
		
		size_t bt0 = 10*vpos, bts = hid.button_count(), odd = bts-1-bt0;

		//10 buttons per screen
		for(size_t i=0;i<10;i++) if(!flicker) 
		{
			const wchar_t *label = hid.button_label(bt0+i);

			if(!*label&&i!=odd){ ShowWindow(props.buttons[i],SW_HIDE); continue; }
						
			if(!props.buttons[i]) props.buttons[i] =
			CreateDialogW(Somplayer_dll(),MAKEINTRESOURCEW(IDD_BUTTON),hWndDlg,SomwindowsButton);
			
			ShowWindow(bt=props.buttons[i],SW_SHOW); if(!i) GetClientRect(bt,&rect);
			
			int cbt = hid.contexts_button(bt0+i,bts);
			SetWindowLong(bt,GWL_USERDATA,hid.contexts[hpos].buttons_id(cbt));

			if(i%2)
			{
				SetWindowPos(bt,0,12+rect.right,6+i/2*rect.bottom,0,0,SWP_NOSIZE|SWP_NOZORDER);							
			}
			else SetWindowPos(bt,0,5,6+i/2*rect.bottom,0,0,SWP_NOSIZE|SWP_NOZORDER);
	
			SetDlgItemTextW(bt,IDC_LABEL,bt0+i<bts||i%2==0?label:L""); 
			SetDlgItemTextW(bt,IDC_EDIT,hid.contexts[hpos].buttons_text(cbt));
			SendMessage(GetDlgItem(bt,IDC_BUTTON),BM_SETSTATE,hid.button_state(bt0+i),0);

			//draw over label
			InvalidateRect(GetDlgItem(bt,IDC_BUTTON),0,0); 
			if(*label) SetDlgItemInt(bt,IDC_BUTTON,bt0+i+1,FALSE);
			if(!*label) SetDlgItemText(bt,IDC_BUTTON,"");
		}
		else if(bt=props.buttons[i]) //hack
		{
			//this actually has a use in so far as
			//SetWindowText wipes the undo history

			int cbt = hid.contexts_button(bt0+i,bts);
			int cid = hid.contexts[hpos].buttons_id(cbt);

			//redraw assignments only if necessary
			if(GetWindowLong(bt,GWL_USERDATA)!=cid)
			{					
				SETTEXTEX st = {ST_KEEPUNDO,1200};

				const wchar_t *text = Somcontrol_h::button(cid).text();

				//SetDlgItemTextW(bt,IDC_EDIT,text);
				SendDlgItemMessage(bt,IDC_EDIT,EM_SETSEL,0,-1);
				SendDlgItemMessageW(bt,IDC_EDIT,EM_REPLACESEL,TRUE,(LPARAM)text);

				SetWindowLong(bt,GWL_USERDATA,cid);
			}
		}

	}break;
	case WM_USER: //Somcontrol::post_messages_to
	{
		Somwindows_cpp::hidprops &props =
		*(Somwindows_cpp::hidprops*)GetWindowLongPtrW(hWndDlg,GWLP_USERDATA);
		
		if(props.tap!=(Somcontrol*)wParam) break; //cancelled (paranoia)

		size_t vpos = SendDlgItemMessage(hWndDlg,IDC_SCREEN,TBM_GETPOS,0,0);

		HWND bt; size_t bt0 = 10*vpos, bts = props.tap->button_count();

		for(size_t i=0;i<10;i++) //10 buttons per screen
		{
			if(bt=props.buttons[i])
			{
				float st = props.tap->button_state(bt0+i);

				SendMessage(GetDlgItem(bt,IDC_BUTTON),BM_SETSTATE,fabs(st)>0.3,0);
			}
		}

	}break;
	case WM_COMMAND:
	{
		Somwindows_cpp::hidprops &props =
		*(Somwindows_cpp::hidprops*)GetWindowLongPtrW(hWndDlg,GWLP_USERDATA);

		switch(LOWORD(wParam))
		{		
		case IDC_CPANEL:

			if(!props.tap->popup_control_panel(hWndDlg)) assert(0);				
			break;
		}
		
	}break;
	case WM_DESTROY: //WM_NCDESTROY
	{	
	close: 

		delete (Somwindows_cpp::hidprops*)GetWindowLongPtrW(hWndDlg,GWLP_USERDATA);				
		
		if(Msg!=WM_DESTROY) DestroyWindow(hWndDlg); //WM_NCDESTROY
	}
	}//switch

	return 0;
}

static LRESULT CALLBACK SomwindowsPictureProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam, UINT_PTR scid, DWORD_PTR p) 
{	
	Somwindows_h::windowman wm(hwnd);

	switch(Msg)
	{
	case WM_SETFOCUS:
	{
		Somcontrol::focus(hwnd);

		Somwindows_h::pictureman(wm).focus();

	}break;
	case WM_CONTEXTMENU:	
	{	
		HMENU menu = wm.context('main');

		Somwindows_h::pictureman pm(wm); if(!pm) break;		

		int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

		BOOL cmd = TrackPopupMenu(menu,TPM_RETURNCMD,x,y,0,hwnd,0);
				
		switch(cmd)
		{
		case ID_INPUT:
		{						
			INITCOMMONCONTROLSEX cc = //hack
			{
				sizeof(INITCOMMONCONTROLSEX),ICC_LISTVIEW_CLASSES
			};
			static BOOL hack = InitCommonControlsEx(&cc); 

			const int tabs = Somconsole::ports+1;
			HWND *sheet = Sompaste->propsheet(hwnd,tabs,L"Somplayer.dll - Port Authority"); if(!sheet) return 1;
			HWND multi = CreateDialogParamW(Somplayer_dll(),MAKEINTRESOURCEW(IDD_MULTITAP_TAB),sheet[tabs],SomwindowsMultitapTab,(LPARAM)hwnd);			
			HWND props = CreateDialogParamW(Somplayer_dll(),MAKEINTRESOURCEW(IDD_DEVICE_TAB),sheet[tabs],SomwindowsDeviceTabs,(LPARAM)hwnd);			
			if(sheet[1]==props) for(int i=2;i<tabs;i++) sheet[i] = props; 
			ShowWindow(sheet[tabs],SW_SHOW);
			BringWindowToTop(sheet[tabs]);

		}break;
		}

		return 0;
		
	}break;
	case WM_SIZE:
	{
		if(IsIconic(hwnd)) break;

		int x = LOWORD(lParam), y = HIWORD(lParam);

		Somwindows_h::pictureman pm(wm); if(!pm||!pm->target) break;
		
		pm->target->width = x; pm->target->height = y;

	}break;
	case WM_PAINT: 
	{
		Somwindows_h::pictureman pm(wm);

		if(!pm||!pm->target) //testing
		{
			HDC dc = GetDC(hwnd); 
			
			RECT fill; GetClientRect(hwnd,&fill);

			static HBRUSH darkcyan = CreateSolidBrush(RGB(0,0x80,0x80));

			int success = FillRect(dc,&fill,darkcyan);

			ReleaseDC(hwnd,dc);
		}
		else if(!pm->wet) //debugging
		{
			pm->wet = true; 

			try{ //wanna be sure wet is unset
			
			//note: this is not an ideal circumstance
			if(!pm->target->ready()) pm->refresh(wm);

			PAINTSTRUCT ps;	

			if(BeginPaint(hwnd,&ps)) 
			if(!pm->target->present(hwnd,&ps.rcPaint,&ps.rcPaint))
			{
				//TODO: should probably do something at this point			
			}
			EndPaint(hwnd,&ps);
			}catch(...){}

			pm->wet = false;
		}
		else return 1;
		
	}break;
	}

	return wm(hwnd,Msg,wParam,lParam,scid,p);	
}

static INT_PTR SomwindowsTextureZoom(Somwindows_h::windowman &wm, int z)
{
	Somwindows_h::textureman tm(wm); 
	
	if(tm&&z!=tm->zoom) 
	{		
		const int zooms[] = {0,ID_ZOOM_1X,ID_ZOOM_2X,0,ID_ZOOM_4X,0,0,0,ID_ZOOM_8X};

		CheckMenuRadioItem(wm->context,ID_ZOOM_1X,ID_ZOOM_8X,zooms[z>8?z:0],MF_BYCOMMAND);
		
		tm->zoom = z; tm.crop("repaint");
	}
	return 0; 
}

static INT_PTR SomwindowsTextureNib(Somwindows_h::windowman &wm, int x, int y, int shape) 
{
	Somwindows_h::textureman tm(wm); if(!tm||!tm->nib(x,y)) return 0; //paranoia

	switch(shape)
	{
	case Somwindows_h::SQUARE:
	case Somwindows_h::CIRCLE:

		for(int i=0;i<x;i++) for(int j=0;j<x;j++) tm->nib.shape[i][j] = 'o'; 

		if(shape==Somwindows_h::SQUARE) break;

		if(x!=y||x<4||x>8||x==5) return 0; //circles
							  
		tm->nib.shape[0][0] = 'x'; tm->nib.shape[x-1][0] = 'x'; //4 corners
		tm->nib.shape[0][y-1] = 'x'; tm->nib.shape[x-1][y-1] = 'x'; if(x==4) break;

		tm->nib.shape[1][0] = 'x'; tm->nib.shape[0][1] = 'x'; //top left
		tm->nib.shape[x-2][0] = 'x'; tm->nib.shape[x-1][1] = 'x'; //top right
		tm->nib.shape[x-2][y-1] = 'x'; tm->nib.shape[x-1][y-2] = 'x'; //bottom right
		tm->nib.shape[0][y-2] = 'x'; tm->nib.shape[1][y-1] = 'x'; //bottom left

		break;

	case Somwindows_h::FOURPT: if(x%1||x<3||x>7) return 0;
	   
		switch(x) //Note: would not be hard to he patern is very simple
		{
		case 3: strcpy(tm->nib.shape[0],"xox");
				strcpy(tm->nib.shape[1],"ooo");
				strcpy(tm->nib.shape[2],"xox"); break;
		case 5:	strcpy(tm->nib.shape[0],"xxoxx");
				strcpy(tm->nib.shape[1],"xooox");
				strcpy(tm->nib.shape[2],"ooooo");
				strcpy(tm->nib.shape[3],"xooox");
				strcpy(tm->nib.shape[4],"xxoxx"); break;		
		case 7:	strcpy(tm->nib.shape[0],"xxxoxxx");
				strcpy(tm->nib.shape[1],"xxoooxx");
				strcpy(tm->nib.shape[2],"xooooox");
				strcpy(tm->nib.shape[3],"ooooooo");
				strcpy(tm->nib.shape[4],"xooooox");
				strcpy(tm->nib.shape[5],"xxoooxx");
				strcpy(tm->nib.shape[6],"xxxoxxx"); break;
		}break;

	default: return 0;
	}

	tm->nib.x = x; tm->nib.y = y; 
	
	return 0; //always 0 for SomwindowsTextureProc
}

static INT_PTR CALLBACK SomwindowsTextureProps(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg)
	{
	case WM_INITDIALOG:
	{			
		HWND ps = GetParent(hWndDlg);

		EnableThemeDialogTexture(hWndDlg,ETDT_ENABLETAB);

		Somwindows_h::windowman wm((HWND)lParam);	
		Somwindows_h::textureman tm((HWND)lParam); 
		
		if(!tm||!tm->pal) goto close; //TODO: Allow for "new" image
		
		Somwindows_cpp::texprops *props = new Somwindows_cpp::texprops(wm,ps);

		SetWindowLongPtrW(hWndDlg,GWLP_USERDATA,(LONG_PTR)props);
		
		props->window = (HWND)lParam;

		HWND pic = GetDlgItem(hWndDlg,IDC_PIC); 
		SendMessage(pic,STM_SETIMAGE,IMAGE_BITMAP,(LPARAM)props->pal->thumb(32,32,"enlarge"));
		SetWindowPos(GetDlgItem(hWndDlg,IDC_PIC2),0,0,0,32,32,SWP_NOMOVE);
		SetWindowPos(pic,GetDlgItem(hWndDlg,IDC_PIC2),0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);	

		GetWindowTextW(wm,props->rename,MAX_PATH-1);

		wchar_t caption[MAX_PATH]; //a short detour...
		swprintf_s(caption,L"%s Properties",props->rename);
		SetWindowTextW(ps,caption);

		wchar_t *ext = PathFindExtensionW(props->rename);

		if(Somconsole::type(ext)!=Somconsole::_PAL)
		{
			size_t diff = ext-props->rename;

			const wchar_t *pal = isupper(ext[*ext=='.'])?L".PAL":L".pal";

			wcsncpy(props->rename+diff,pal,MAX_PATH-diff-1);
		}		

		if(wm.consolelock())
		{
			wm->console->source(wm.texture(!'lock'),props->source);
			wm->console->unlock();
		}

		SetDlgItemTextW(hWndDlg,IDC_RENAME,props->rename); 
		SetDlgItemTextW(hWndDlg,IDC_SOURCE,props->source);

		if(!*props->source)
		{
			SetDlgItemTextW(hWndDlg,IDC_SOURCE,L"Unknown: console closed");
		}
		else EnableWindow(GetDlgItem(hWndDlg,IDC_LOCATION),TRUE);

		HWND tabs = PropSheet_GetTabControl(ps);

		TCITEMW tab = { TCIF_TEXT,0,0,L"Channels",8,0};
		SendMessageW(tabs,TCM_SETITEMW,0,(LPARAM)&tab); 
		tab.pszText = L"Notepad";  
		SendMessageW(tabs,TCM_SETITEMW,1,(LPARAM)&tab);

		HWND notepad = GetDlgItem(hWndDlg,IDC_NOTEPAD);

		SendMessage(notepad,EM_SETTEXTMODE,TM_PLAINTEXT,0); 

		wchar_t notes_s = tm->pal->notes_characters(), *notes = new wchar_t[notes_s];

		if(tm->pal->notes(notes,notes_s)) SetWindowTextW(notepad,notes); delete [] notes;
	
		HWND lv = GetDlgItem(hWndDlg,IDC_LISTVIEW);	
		DWORD lvs_ex_style = LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_INFOTIP;	  
		ListView_SetExtendedListViewStyle(lv,lvs_ex_style);
		
		static HFONT lvfont = 0; if(!lvfont) //monospace sans-serif
		{
			HFONT guifont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

			LOGFONT mono; GetObject(guifont,sizeof(mono),&mono); 

			mono.lfPitchAndFamily = FIXED_PITCH|FF_MODERN; //best?

			lvfont = CreateFontIndirect(&mono); //close enough...
		}			 
		SendMessage(lv,WM_SETFONT,(WPARAM)lvfont,MAKELPARAM(1,0)); 
				
		LVITEMW lvi = {LVIF_TEXT}; 
		LVCOLUMNW lvc = {LVCF_TEXT|LVCF_FMT|LVCF_WIDTH};
		
		lvc.fmt = LVCFMT_LEFT;

		RECT room; LONG &remaining = room.right; 
		
		GetClientRect(lv,&room);
		lvc.pszText = L"PAL-ID"; remaining-=lvc.cx = 50; 
		SendMessage(lv,LVM_INSERTCOLUMNW,0,(LPARAM)&lvc);  		
		lvc.fmt = LVCFMT_CENTER;
		lvc.pszText = L"Primary"; remaining-=lvc.cx = 50;
		SendMessage(lv,LVM_INSERTCOLUMNW,1,(LPARAM)&lvc);  				
		lvc.pszText = L"Bytes"; remaining-=lvc.cx = 50; 
		SendMessage(lv,LVM_INSERTCOLUMNW,2,(LPARAM)&lvc);
		lvc.pszText = L"Units"; remaining-=lvc.cx = 50; 
		SendMessage(lv,LVM_INSERTCOLUMNW,3,(LPARAM)&lvc);
		lvc.fmt = LVCFMT_LEFT;
		lvc.pszText = L"Description"; lvc.cx = remaining; 
		SendMessage(lv,LVM_INSERTCOLUMNW,4,(LPARAM)&lvc);
						
		for(int i=0;i<props->lv_s;i++)
		{	
			lvi.iItem = i; 
			lvi.iSubItem = 0; lvi.pszText = props->lv[i].palid;
			SendMessage(lv,LVM_INSERTITEMW,0,(LPARAM)&lvi);			
			lvi.iSubItem = 1; lvi.pszText = props->lv[i].primary;
			SendMessage(lv,LVM_SETITEMTEXTW,i,(LPARAM)&lvi);   
			lvi.iSubItem = 2; lvi.pszText = props->lv[i].bytes;
			SendMessage(lv,LVM_SETITEMTEXTW,i,(LPARAM)&lvi);   
			lvi.iSubItem = 3; lvi.pszText = props->lv[i].units;
			SendMessage(lv,LVM_SETITEMTEXTW,i,(LPARAM)&lvi);
			lvi.iSubItem = 4; lvi.pszText = props->lv[i].tip;
			SendMessage(lv,LVM_SETITEMTEXTW,i,(LPARAM)&lvi);
		}

		//TODO: figure out why classic theme does not show
		//HWND lvh = (HWND)SendMessage(lv,LVM_GETHEADER,0,0);
																	
		int st = LVIS_SELECTED|LVIS_FOCUSED;
		ListView_SetItemState(lv,props->channel,st,st); 

		ShowWindow(hWndDlg,SW_SHOW); 

		return FALSE;
	}	
	case WM_DRAWITEM: 

		if(wParam==IDC_PIC2)
		{
			DRAWITEMSTRUCT *di = (DRAWITEMSTRUCT*)lParam;

			RECT fill, a = {0,0,32,32}, b;
			
			GetClientRect(GetDlgItem(hWndDlg,IDC_PIC),&b);

			if(SubtractRect(&fill,&a,&b))
			FillRect(di->hDC,&fill,(HBRUSH)GetStockObject(BLACK_BRUSH));
		}
		break;

	case WM_NOTIFY:
	{
		PSHNOTIFY *p = (PSHNOTIFY*)lParam;

		switch(p->hdr.code)
		{
		case NM_SETFOCUS: //FALL THRU

			if(p->hdr.idFrom!=IDC_LISTVIEW) break; 
	
			//wait for focus before turning on ch buttons...

		case LVN_ITEMCHANGED: 
		{	
			//pre-initialization?
			if(!IsWindowVisible(hWndDlg)) break; 

			NMLISTVIEW *q = (LPNMLISTVIEW)lParam;

			Somwindows_cpp::texprops *props =
			(Somwindows_cpp::texprops*)GetWindowLongPtrW(hWndDlg,GWLP_USERDATA);			

			if(p->hdr.code!=NM_SETFOCUS) 
			{
			//Note: never notified of text changes...

				if(~q->uChanged&LVIF_TEXT)
				{
					assert(q->iSubItem==0); //paranoia

					if(~q->uChanged&LVIF_STATE) break;

					if(~q->uNewState&LVIS_FOCUSED) break;

					props->channel = q->iItem;
				}
				else assert(0);
			}

			int i = props->channel;

			BOOL add = !props->lv[i].add, use = !add&&!props->lv[i].use;

			if(!Somtexture::primary_candidate(props->lv[i].id)) use = FALSE;

			EnableWindow(GetDlgItem(hWndDlg,IDC_USE),use);
			EnableWindow(GetDlgItem(hWndDlg,IDC_ADD),add);
			EnableWindow(GetDlgItem(hWndDlg,IDC_REMOVE),!add);

			break;
		}
		case PSN_SETACTIVE: case PSN_KILLACTIVE: 
		{
			int show = p->hdr.code==PSN_SETACTIVE?SW_SHOW:SW_HIDE;

			switch(p->hdr.idFrom)
			{
			case 0: //should be Channels

				ShowWindow(GetDlgItem(hWndDlg,IDC_USE),show); 
				ShowWindow(GetDlgItem(hWndDlg,IDC_ADD),show); 				
				ShowWindow(GetDlgItem(hWndDlg,IDC_REMOVE),show); 
				ShowWindow(GetDlgItem(hWndDlg,IDC_LISTVIEW),show); break;

			case 1: //should be Notepad
				
				ShowWindow(GetDlgItem(hWndDlg,IDC_NOTEPAD),show); break;

			default: assert(0);
			}

			SetWindowLong(hWndDlg,DWL_MSGRESULT,FALSE); return TRUE;
		}
		case PSN_APPLY:
		{
			if(p->hdr.idFrom==1) break; //Notepad (avoid double apply)

			Somwindows_cpp::texprops *props =
			(Somwindows_cpp::texprops*)GetWindowLongPtrW(hWndDlg,GWLP_USERDATA);
						
			wchar_t rename[MAX_PATH] = {0};
			GetDlgItemTextW(hWndDlg,IDC_RENAME,rename,MAX_PATH);

			Somwindows_h::textureman tm(props->window); //yikes...

			if(tm&&tm->pal==props->pal) //NEW (not good enough)
			if(wcscmp(rename,props->rename)&&PathIsFileSpecW(rename))
			{
				SetWindowTextW(props->window,rename);
			}

			if(!props->apply())
			{
				assert(0); //TODO: MessageBox()

				props->apply("force");				
			}

			HWND lv = GetDlgItem(hWndDlg,IDC_LISTVIEW);	
				
			LVITEMW lvi = {LVIF_TEXT}; 
						
			for(int i=0;i<props->lv_s;i++)
			{	
				lvi.iSubItem = 1; lvi.pszText = props->lv[i].primary;
				SendMessage(lv,LVM_SETITEMTEXTW,i,(LPARAM)&lvi);   
				lvi.iSubItem = 2; lvi.pszText = props->lv[i].bytes;
				SendMessage(lv,LVM_SETITEMTEXTW,i,(LPARAM)&lvi);   
			}

			if(props->noted)
			{	
				HWND notepad = GetDlgItem(hWndDlg,IDC_NOTEPAD);

				wchar_t notes_s = GetWindowTextLength(notepad), *notes = new wchar_t[notes_s];
																				
				if(GetDlgItemTextW(hWndDlg,IDC_NOTEPAD,notes,notes_s)) props->pal->notes(0,notes_s,0,notes);
				
				props->noted = false; delete [] notes; 
			}

			RedrawWindow(props->window,0,0,RDW_INTERNALPAINT);

		}break;		
		}

	}break;
	case WM_COMMAND:
	{
		Somwindows_cpp::texprops *props =
		(Somwindows_cpp::texprops*)GetWindowLongPtrW(hWndDlg,GWLP_USERDATA);

		switch(LOWORD(wParam))
		{		
		case IDC_NOTEPAD: props->noted = true; //notes changed
		case IDC_RENAME:  
		
			if(HIWORD(wParam)==EN_UPDATE) //EN_CHANGE: richedit2 sends en_update by default
			{
				if(IsWindowVisible(hWndDlg)) //turn on the Apply button
				SendMessage(GetParent(hWndDlg),PSM_CHANGED,(WPARAM)hWndDlg,0);

			}break;
		
		case IDC_LOCATION: 
		{
			wchar_t args[MAX_PATH*2]; //explorer[MAX_PATH];						
			//7 can't seem to find Explorer: seems unnecessary anyway
			//if(PathFindOnPathW(wcscpy(explorer,L"Explorer"),0))
			{
				swprintf_s(args,L"/select,\"%s\"",props->source);			

				//Reminder: "explore" verb cannot hilight files 
				ShellExecuteW(0,L"open",L"Explorer",args,0,SW_SHOW); 
			}

		}break;
		case IDC_READONLY: case IDC_OVERWRITE: 
		case IDC_USE: case IDC_ADD: case IDC_REMOVE:
		{				
			int i = props->channel, j = props->primary; 

			switch(LOWORD(wParam))
			{												
			case IDC_USE: props->primary = props->channel;
				
				props->lv[j].use = false; props->lv[i].use = true; break;

			case IDC_ADD: props->lv[i].add = true; break;

			case IDC_REMOVE: props->lv[i].add = false; 
				
				if(i==j) props->primary = props->lv_s;

				props->lv[i].use = false; break;			
			}
						
			props->lv[i] = props->pal; //yuck
			props->lv[j] = props->pal; //ouch
			
			LVITEMW lvi = {LVIF_TEXT}; 
			HWND lv = GetDlgItem(hWndDlg,IDC_LISTVIEW);			
			lvi.iSubItem = 1; lvi.pszText = props->lv[j].primary;
			SendMessage(lv,LVM_SETITEMTEXTW,j,(LPARAM)&lvi);   
			lvi.iSubItem = 1; lvi.pszText = props->lv[i].primary;
			SendMessage(lv,LVM_SETITEMTEXTW,i,(LPARAM)&lvi);   
			lvi.iSubItem = 2; lvi.pszText = props->lv[i].bytes;
			SendMessage(lv,LVM_SETITEMTEXTW,i,(LPARAM)&lvi);   
				
			//return focus to listview & update buttons in process
			SendMessage(GetParent(hWndDlg),WM_NEXTDLGCTL,(WPARAM)lv,1); 			

			//turn on the Apply button
			SendMessage(GetParent(hWndDlg),PSM_CHANGED,(WPARAM)hWndDlg,0);

		}break;
		}
		break;
	}
	case WM_DESTROY: //WM_NCDESTROY
	{	
	close: 

		HWND pic = GetDlgItem(hWndDlg,IDC_PIC); //paranoia?
		DeleteObject((HGDIOBJ)SendMessage(pic,STM_SETIMAGE,IMAGE_BITMAP,0));
				
		delete (Somwindows_cpp::texprops*)GetWindowLongPtrW(hWndDlg,GWLP_USERDATA);				
		
		if(Msg!=WM_DESTROY) DestroyWindow(hWndDlg); //WM_NCDESTROY
	}
	}//switch

	return 0;
}

static LRESULT CALLBACK SomwindowsFolderProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scid, DWORD_PTR param) 
{
	switch(uMsg)
	{
	case WM_SHOWWINDOW:
	{		
		Somwindows_cpp::tabprops *props = 
		(Somwindows_cpp::tabprops*)GetWindowLongPtrW(hwnd,GWLP_USERDATA);				
		
		if(!props) props = new Somwindows_cpp::tabprops((HWND)param);

		SetWindowLongPtrW(hwnd,GWLP_USERDATA,(LONG_PTR)props);	
		
	}break;
	case WM_DESTROY: //catch selection
	{
		wchar_t *select = (wchar_t*)GetProp(hwnd,"Sompaste_select");
		
		if(select&&*select) 
		{
			Somwindows_cpp::tabprops &props = 
			*(Somwindows_cpp::tabprops*)GetWindowLongPtrW(hwnd,GWLP_USERDATA);						

			if(props.consolelock()) //assuming texture
			{				
				const wchar_t *file = props.resource(!'lock'); 

				if(file) props->console->browse(file,select);

				props->console->unlock();
			}
		}

	}break;
	case WM_NCDESTROY: 
	{
		RemoveWindowSubclass(hwnd,SomwindowsFolderProc,scid);

		Somwindows_cpp::tabprops *props = 
		(Somwindows_cpp::tabprops*)GetWindowLongPtrW(hwnd,GWLP_USERDATA);						

		delete props;  

	}break;
	}

	return DefSubclassProc(hwnd,uMsg,wParam,lParam);
}

static LRESULT CALLBACK SomwindowsTextureProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam, UINT_PTR scid, DWORD_PTR p) 
{	
	Somwindows_h::windowman wm(hwnd);	

	switch(Msg)
	{
	case WM_KEYDOWN:

		switch(wParam)
		{
		case VK_SPACE: //layouts
		case VK_NUMPAD7: case VK_NUMPAD8: case VK_NUMPAD9:
		case VK_NUMPAD4: case VK_NUMPAD5: case VK_NUMPAD6:
		case VK_NUMPAD1: case VK_NUMPAD2: case VK_NUMPAD3:
		{
			Somwindows_h::textureman tm(wm); if(!tm) return 0;

			if(wParam==VK_SPACE)
			{
				tm->space+=GetKeyState(VK_SHIFT)&0x8000?-1:+1; 

				if(tm->space==10) tm->space = 1;
				if(tm->space==0) tm->space = 9;
			}			
			else tm->space = wParam-VK_NUMPAD1+1;

			tm.crop("repaint"); return tm->space;

		}break;			
		}
		break;

	case WM_CONTEXTMENU:	
	{			
		Somwindows_h::textureman tm(wm);

		int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

		if(tm)
		{
			//TODO: invalidate somehow???			
			tm->edit.x = x; tm->edit.y = y; 

			ScreenToClient(hwnd,&tm->edit);
		}
						
		HMENU menu = wm.context('main');
		
		const Somtexture *p = tm?tm->pal:0; //1: MF_GRAYED
		EnableMenuItem(menu,ID_FILE_RGB,p->channel(p->RGB)?0:1);
		EnableMenuItem(menu,ID_FILE_PAL,p->channel(p->PAL)?0:1);
		EnableMenuItem(menu,ID_FILE_16X16,p->channel(p->MAP)?0:1);
		EnableMenuItem(menu,ID_FILE_INDEX,p->channel(p->INDEX)?0:1);
		EnableMenuItem(menu,ID_FILE_ALPHA,p->channel(p->ALPHA)?0:1);
		
		if(tm) for(int i=ID_FILE_RGB;i<=ID_FILE_ALPHA;i++) //CheckMenuRadioItem 
		CheckMenuItem(menu,i,MF_BYCOMMAND|(tm->file==i?MF_CHECKED:MF_UNCHECKED));		

		return TrackPopupMenu(menu,0,x,y,0,hwnd,0); 
	}
	case WM_COMMAND: 
	{
	//returning 1 to make way for custom menu translation
	//Reminder: as in Somplayer->context(...,"+ Custom",&ID)

		switch(LOWORD(wParam))
		{
		case ID_FILE_RGB: case ID_FILE_ALPHA:
		case ID_FILE_PAL: case ID_FILE_16X16: case ID_FILE_INDEX:
		{
			Somwindows_h::textureman tm(wm); tm.show(LOWORD(wParam)); return 1;		
		}
		case ID_FILE_REASSIGN:
		{
			Somwindows_h::textureman tm(wm); 			
			if(tm&&tm->pal->sort()&&tm->file==ID_FILE_INDEX) tm.crop("repaint"); 
			return 1; 
		}
		case ID_FILE_PROPERTIES:
		{	
			INITCOMMONCONTROLSEX cc = //hack
			{
				sizeof(INITCOMMONCONTROLSEX),ICC_LISTVIEW_CLASSES
			};
			static BOOL hack1 = InitCommonControlsEx(&cc); 

			static HMODULE hack2 = LoadLibrary("Riched20.dll"); 

			const int tabs = 2;
			HWND *sheet = Sompaste->propsheet(hwnd,tabs); if(!sheet) return 1;
			HWND props = CreateDialogParamW(Somplayer_dll(),MAKEINTRESOURCEW(IDD_TEXTURE_PROPS),sheet[tabs],SomwindowsTextureProps,(LPARAM)hwnd);
			if(sheet[0]==props) sheet[1] = props; //Notepad						
			Sompaste->wallpaper(sheet[tabs]);

		}return 1;
		case ID_FILE_FOLDER:
		{
			wchar_t src[MAX_PATH] = L"";								

			if(wm.consolelock()) //assuming 
			{									
				wm->console->source(wm.texture(!'lock'),src,!'link');	
				wm->console->unlock();
			}

			//wchar_t text[MAX_PATH+10] = L"";
			//GetWindowTextW(hwnd,text,MAX_PATH); wcscat(text,L" Folder");
			HWND modeless = Sompaste->folder(hwnd,src,L"pal; txr; tim; bmp",L"","modeless"); //,text);
			SetWindowSubclass(modeless,SomwindowsFolderProc,0,(DWORD_PTR)hwnd);
			
			ShowWindow(modeless,SW_SHOW); assert(IsWindowVisible(modeless));				
			Sompaste->wallpaper(modeless);

		}return 1;
		copy_BLACK: //<---REMOVE ME
		case ID_EDIT_COPY: //2017: Was Extracting SFX textures (TIMs.)
		{
			//Sompaste->xfer("C clip:clip.Windows");
			OpenClipboard(0);
			EmptyClipboard();
			SetClipboardData(CF_BITMAP,Somwindows_h::textureman(wm).image->pal->thumb());
			CloseClipboard();
		
		}return 1;
		case ID_EDIT_COLORKEY: 
		case ID_EDIT_DARKGRAY: case ID_EDIT_DARKTONE: case ID_EDIT_DARKRAND:
		{
			//2017: Removing false color key texels from legacy TXRs.
			Somwindows_BLACK(wm,LOWORD(wParam));

			#ifdef _DEBUG
			goto copy_BLACK; //REMOVE ME
			#endif

		}return 1;
		}

		using namespace Somwindows_h; //SQUARE/PENCIL
		
		switch(LOWORD(wParam))
		{
		case ID_ZOOM_1X:   return SomwindowsTextureZoom(wm,1);
		case ID_ZOOM_2X:   return SomwindowsTextureZoom(wm,2);
		case ID_ZOOM_3X:   return SomwindowsTextureZoom(wm,3);
		case ID_ZOOM_4X:   return SomwindowsTextureZoom(wm,4);
		case ID_ZOOM_5X:   return SomwindowsTextureZoom(wm,5);
		case ID_ZOOM_6X:   return SomwindowsTextureZoom(wm,6);
		case ID_ZOOM_7X:   return SomwindowsTextureZoom(wm,7);		
		case ID_ZOOM_8X:   return SomwindowsTextureZoom(wm,8);
		case ID_ZOOM_9X:   return SomwindowsTextureZoom(wm,9);
		case ID_ZOOM_10X:   return SomwindowsTextureZoom(wm,10);
		case ID_ZOOM_11X:   return SomwindowsTextureZoom(wm,11);
		case ID_ZOOM_12X:   return SomwindowsTextureZoom(wm,12);
		case ID_ZOOM_13X:   return SomwindowsTextureZoom(wm,13);
		case ID_ZOOM_14X:   return SomwindowsTextureZoom(wm,14);
		case ID_ZOOM_15X:   return SomwindowsTextureZoom(wm,15);
		case ID_ZOOM_16X:   return SomwindowsTextureZoom(wm,16);		
		case ID_SQUARE_1X1: return SomwindowsTextureNib(wm,1,1,SQUARE);
		case ID_SQUARE_2X2: return SomwindowsTextureNib(wm,2,2,SQUARE);
		case ID_SQUARE_3X3: return SomwindowsTextureNib(wm,3,3,SQUARE);
		case ID_SQUARE_4X4: return SomwindowsTextureNib(wm,4,4,SQUARE);
		case ID_SQUARE_5X5: return SomwindowsTextureNib(wm,5,5,SQUARE);
		case ID_SQUARE_6X6: return SomwindowsTextureNib(wm,6,6,SQUARE);
		case ID_SQUARE_7X7: return SomwindowsTextureNib(wm,7,7,SQUARE);
		case ID_SQUARE_8X8: return SomwindowsTextureNib(wm,8,8,SQUARE);
		case ID_CIRCLE_4X4: return SomwindowsTextureNib(wm,4,4,CIRCLE);
		case ID_CIRCLE_6X6: return SomwindowsTextureNib(wm,6,6,CIRCLE);
		case ID_CIRCLE_7X7: return SomwindowsTextureNib(wm,7,7,CIRCLE);
		case ID_CIRCLE_8X8: return SomwindowsTextureNib(wm,8,8,CIRCLE);
		case ID_POINT_3X3:  return SomwindowsTextureNib(wm,3,3,FOURPT);
		case ID_POINT_5X5:  return SomwindowsTextureNib(wm,5,5,FOURPT);
		case ID_POINT_7X7:  return SomwindowsTextureNib(wm,7,7,FOURPT);
		}
		
		int tool = -1; //hack

		switch(LOWORD(wParam)) 
		{
		case ID_TOOLS_PENCIL:  tool = PENCIL;  break;
		case ID_TOOLS_ERASER:  tool = ERASER;  break;
		case ID_TOOLS_STAMP:   tool = STAMP;   break;
		case ID_TOOLS_MARKER:  tool = MARKER;  break;
		case ID_TOOLS_STENCIL: tool = STENCIL; break;
		case ID_TOOLS_CURSOR:  tool = CURSOR;  break;
		}

		if(tool!=-1)
		{
			Somwindows_h::textureman tm(wm); if(!tm||tm->tool==tool) return 1;
			CheckMenuRadioItem(wm->context,ID_TOOLS_PENCIL,ID_TOOLS_CURSOR,LOWORD(wParam),MF_BYCOMMAND);
			return 1;
		}

		if(lParam) assert(0); //TODO: custom menu translation
		
		break;
	}
	case WM_SIZE: case WM_HSCROLL: case WM_VSCROLL: 
	{
		Somwindows_h::textureman tm(wm); if(!tm) break;
				
		if(Msg==WM_VSCROLL&&!tm->scrollbars[0](hwnd,Msg,wParam,lParam)) break;
		if(Msg==WM_HSCROLL&&!tm->scrollbars[1](hwnd,Msg,wParam,lParam)) break;

		tm.crop("repaint"); break;
	}
	case WM_PAINT: 
	{	
		Somwindows_h::textureman tm(wm); 
		
		if(!tm||tm->wet) break;

		if(tm.mote()) tm.crop();  

		if(!tm->pal) //do not enter
		{
			HDC dc = GetDC(hwnd); assert(0); //obsolete code
		
			static HBRUSH darkcyan = CreateSolidBrush(RGB(0,0x80,0x80));

			int success = FillRect(dc,&tm->fill,darkcyan); ReleaseDC(hwnd,dc);
		}
		else 
		{
			tm->wet = true; //hack: avoid pile up on assert/lag

			try{ //wanna be sure wet is unset
			if(!tm->pal->present(tm->mode,hwnd,&tm->crop,tm->zoom,&tm->fill,128))
			{					   
				tm.show(0); //Assuming channel deleted via Properties menu
			}
			}catch(...){}

			tm->wet = false;
		}
		
		ValidateRect(hwnd,&tm->fill); //paranoia

		//REMINDER: When returning 1 the messaqe queue was flooded with WM_PAINT messages
		//And that caused SetTimer in Sompaste.dll (Paper API) to break down :/
		break; //return 1;
	}	
	case WM_GETICON: 
	{
		if(GetObjectType(wm->icon)) return (INT_PTR)wm->icon;

		Somwindows_h::textureman tm(wm); if(!tm||!tm->pal) return 0;

		HBITMAP bitmap = tm->pal->thumb(16,16); if(!bitmap) return 0;
				
		//16x16 black AND bitmask: *2 word align?
		char and[32*2]; memset(and,0x00,sizeof(and));					
		HBITMAP bitmask = CreateBitmap(16,16,1,1,and); 			

		ICONINFO iconinfo = {TRUE,0,0,bitmask,bitmap};					
		wm->icon = CreateIconIndirect(&iconinfo); 

		DeleteObject((HGDIOBJ)bitmask);
		DeleteObject((HGDIOBJ)bitmap);

		return (INT_PTR)wm->icon;

	}break;
	}//switch

	return wm(hwnd,Msg,wParam,lParam,scid,p);	
}

static LRESULT CALLBACK SomwindowsPaletteProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam, UINT_PTR scid, DWORD_PTR p) 
{	
	Somwindows_h::windowman wm(hwnd);		
		
	switch(Msg)					  
	{
	case WM_THEMECHANGED:
		
		SetTimer(hwnd,'uxt',500,0); break; //hack...

	case WM_TIMER: KillTimer(hwnd,wParam);

		if(wParam=='uxt') wm.resize(); break;

	case WM_LBUTTONDOWN: 
	{
		HWND f = GetFocus();

		if(f==GetDlgItem(hwnd,IDC_NEW_TONE)) break;
		if(f==GetDlgItem(hwnd,IDC_NEW_WELL)) break;

		HWND tm = GetDlgItem(hwnd,IDC_TONE_MAP); 

		SetFocus(tm); break;
	}
	case WM_COMMAND:
	{	
		Somwindows_h::paletteman pm(wm); 

		const Somtexture *pal = pm?pm->pal:0; if(!pal) break;

		switch(LOWORD(wParam))
		{
		case IDC_NEW_WELL: 
		case ID_NEW_COLORKEY:
		{
			if(!pm.edit()) break;

			int a = 256, z = 256;

			if(pm->selection[0])
			if(pm->infocus||pm->floating) //TODO: pm.pick??
			{
				a = pm->selection[0], z = pm->selection[1]; 
				
				if(z<a) std::swap(a,z);	a-=1000; z-=1000; 				

				if(a==0&&pal->colorkey()) if(z==a++) a = -1; //paranoia
			}
			pm->floating = false;
			
			if(LOWORD(wParam)==ID_NEW_COLORKEY)
			{
				PALETTEENTRY cyan = {0,0x80,0x80,0};				
				if(a==0||pm.clear(pal->map(a,a,0,pal->BEHIND)))
				pm.clear(pal->paint(pal->well(pal->COLORKEY),0,0,a==256?&cyan:0));
			}
			else pm.clear(pal->paint(0,a,z));

		}break;
		case IDC_NEW_TONE: 
		{
			if(!pm.edit()) break;

			const wchar_t *well = pal->well(-1);   
			const wchar_t *tones = pal->tones(well);

			int a = 256, z = 256, aa = tones?tones[1]+1:0, op = pal->BEHIND;

			if(pm->selection[1]>=1000)
			if(pm->infocus||pm->floating) //TODO: pm.pick??
			{
				well = pal->palette_well(pm->selection[1]-1000);
				
				aa = pm->selection[1]-1000; op = pal->BEFORE;
			}	
			pm->floating = false;

			if(pal->id(well)==pal->COLORKEY)
			{
				if(aa&&pm.clear(pal->map(a,z,aa,op))) 
				{						
					pm.clear(pal->paint(pal->well(pal->UNTITLED),aa,aa)); 
				}
			}
			else pm.clear(pal->map(a,z,aa,op)); 

		}break;
		case ID_REVEAL_CUTS:
		{
			HMENU menu = wm.context('main');

			bool check = ~GetMenuState(menu,ID_REVEAL_CUTS,MF_BYCOMMAND)&MF_CHECKED;
			if(CheckMenuItem(menu,ID_REVEAL_CUTS,check?MF_CHECKED:MF_UNCHECKED)!=-1)
			{
				pm.edit();

				const wchar_t *r = pal->well(pal->RESERVED), *t = pal->tones(r); 

				HWND tm = GetDlgItem(hwnd,IDC_TONE_MAP), rw = GetDlgItem(wm,IDC_RESERVED);

				if(!rw) rw = Somwindows_New_Well(hwnd,pal->RESERVED-pal->UNTITLED,r);

				EnableWindow(rw,check); ShowWindow(rw,t&&check?SW_SHOW:SW_HIDE);

				if(t) for(int i=t[0],z=t[1];i<=z&&z<256;i++) if(check) //256: paranoia
				{	
					Somwindows_New_Tone(hwnd,pal->RESERVED-pal->UNTITLED,i,pal->palette[i]);
				}
				else DestroyWindow(GetDlgItem(tm,1000+i)); if(t) wm.resize();

				if(t&&!check&&pm->selection[0]) 
				{
					//Reminder: may be a nice member function in here

					int a = pm->selection[0];
					int z = pm->selection[1]; if(a>z) std::swap(a,z);

					if(a<t[0]+1000)
					{
						a = std::min<int>(pm->selection[0],t[0]+999);
						z = std::min<int>(pm->selection[1],t[0]+999);
					}
					else a = z = 0;

					if(a>=1000&&z>=1000) pm.xor(a,z); else pm.xor(0);
				}
			}

		}break;
		case ID_FILE_REASSIGN:
		{
			if(pm) pm->pal->sort(); assert(0); //TODO: Somtexture::loop?
			
		}break;
		} 
		
	}break;
	case WM_SIZE: case WM_VSCROLL: 
	{	
		Somwindows_h::paletteman pm(wm); 
		
		if(!pm||!pm->pal) break;

		if(pm->pal!=pm->fresh) //hack: should not do this here
		{
			pm->fresh = pm->pal; 

			const wchar_t *well = 0; int ck = 0, ut = 0; 

			//pretty: looks better to go ahead and hide this away
			ShowWindow(GetDlgItem(hwnd,IDC_UNTITLED),SW_HIDE);			

			if(!pm->pal->palette) //hack: need at least a spacer
			{
				ut++; Somwindows_New_Tone(hwnd,0,0,0x808000);
			}
			else for(int i=0;well=pm->pal->well(i);i++)
			{
				const wchar_t *tones = pm->pal->tones(well); assert(tones);

				int num = pm->pal->id(well)-Somtexture::UNTITLED;

ut:				if(num==0&&++ut) //hack
				{		
					if(pm->pal->id(well)==Somtexture::UNTITLED)
					{
						SetDlgItemTextW(hwnd,IDC_UNTITLED,well);	
					}
				}
				else if(num<0) //assuming colorkey
				{
					assert(num+pm->pal->UNTITLED==pm->pal->COLORKEY);

					ck = 1;	if(pm->pal->well(1)) continue;

					ck = num = 0; goto ut; //hack
				}											  
				else Somwindows_New_Well(hwnd,num,well);

				if(ck&&ck--) //hack: add colorkey to first well
				Somwindows_New_Tone(hwnd,num,0,pm->pal->palette[0]);

				for(int j=tones[0],k=tones[1];j<=k;j++)
				Somwindows_New_Tone(hwnd,num,j,pm->pal->palette[j]);				
			}

			if(ut) ShowWindow(GetDlgItem(hwnd,IDC_UNTITLED),SW_SHOW);
			
			HMENU menu = wm.context('main');
			CheckMenuItem(menu,ID_REVEAL_CUTS,MF_UNCHECKED); //hack
			pm.text(); 

			SomwindowsResizeEnum(hwnd);//defer 
		}
				
		if(Msg==WM_SIZE) EnumChildWindows(hwnd,SomwindowsResizeEnum,0);	

		if(Msg==WM_VSCROLL&&!pm->scrollbars[0](hwnd,Msg,wParam,lParam)) break;

		int i, wells[33] = {0,0}, ck = pm->pal->colorkey(1)?1:0;

		bool rw = IsWindowVisible(GetDlgItem(hwnd,IDC_RESERVED));

		for(i=ck;i<31&&pm->pal->well(i);i++) //-1: constant well id 
		wells[i-ck] = IDC_UNTITLED+(pm->pal->well(i)[-1]-Somtexture::UNTITLED);
		if(i==ck) //Assuming COLORKEY stands alone
		wells[i-ck] = IDC_UNTITLED; if(i==ck) i++;
		wells[i-ck] = rw?IDC_RESERVED:0; if(rw) i++;
		wells[i-ck] = 0; 

		if(wells[0]==0) wells[0] = IDC_UNTITLED;

		extern bool Sommctrl_palette(SCROLLINFO*,HWND,int*,bool);
		if(!Sommctrl_palette(pm->scrollbars,hwnd,wells,Msg==WM_SIZE))
		{
			//scrollbar added so we get to go again...
			EnumChildWindows(hwnd,SomwindowsResizeEnum,0);	
			Sommctrl_palette(pm->scrollbars,hwnd,wells,false);
		}		
		//UpdateWindow(hwnd);
	
		break;
	}
	case WM_ERASEBKGND:
	{
		HWND tm = GetDlgItem(hwnd,IDC_TONE_MAP);

		//XP: This logic was moved here from SomwindowsTonesProc
		//There is a thorough explanation in the reference code there
		//For the record, logically this code really does not belong here 
		if(!GetWindowTheme(tm)) //hack: there are ~3 different behaviors
		{
			//Reminder: still need to confirm XP Style (gummy) theme

			Somwindows_h::paletteman pm(wm); if(!pm||!pm->pal) break;

			pm->xorselect[0] = pm->xorselect[1] = 0; pm->xor = false; 
			
			HWND bg = GetDlgItem(tm,IDC_BACKGROUND); //paranoia

			if(bg) InvalidateRect(bg,0,FALSE);
		}
			
	}break;
	}//switch

	return wm(hwnd,Msg,wParam,lParam,scid,p);
}

//scheduled obsolete
namespace Somwindows_cpp
{
	struct TonesEnumRect : public RECT
	{
		int start; //first tone number
	};
}

//scheduled obsolete
static BOOL CALLBACK SomwindowsTonesEnum(HWND tone, LPARAM lparam)
{
	Somwindows_cpp::TonesEnumRect &room = 
	*(Somwindows_cpp::TonesEnumRect*)lparam;

	RECT size; GetWindowRect(tone,&size);

	POINT pos; int sz = size.right-size.left;

	int tonemap = sz==Somwindows_tonesizes[0];

	const int pad = Somwindows_tonepads[!tonemap];
	const int sep = Somwindows_toneseps[!tonemap];
	
	//Reference implementation
	/*//Reminder: enumeration is in no particular order (apparently)
	if(room.left+sep+sz+pad>room.right) //end of a row
	{
		room.top+=sz; room.bottom-=pad; room.left = 0; //...
	}

	if(room.left==0) //beginning of a new row
	{
		room.left+=pad; room.top+=sep; room.bottom+=sep+sz+pad; 
	}
	else room.left+=sep; 

	pos.x = room.left; room.left+=sz
	pos.y = room.top;
	*/

	//Random enumeration implementation...

	int id = GetDlgCtrlID(tone)-1000;

	if(id>256) return TRUE; //IDC_BACKGROUND

	int fit = (room.right-pad*2)/(sz+sep); 
	
	if(fit==0) fit = 1; //paranoia

	int col = (id-room.start)%fit, row = (id-room.start)/fit;
		
	pos.x = pad+col*(sz+sep); pos.y = room.top+sep+row*(sz+sep);

	room.bottom = std::max<LONG>(room.bottom,room.top+sep+(row+1)*(sz+sep)+pad);

	const int flags = SWP_NOSIZE|SWP_NOSENDCHANGING|SWP_NOREDRAW;

	SetWindowPos(tone,0,pos.x,pos.y,0,0,flags);

	return TRUE;
}

namespace Somwindows_cpp
{
	template<int N> struct rename
	{	
		size_t n; void *user;

		typedef void (*callback)(HWND,rename<0>*,void*,bool);

		template<typename T>
		rename(const wchar_t *init, HWND win, callback cb, T data)
		{
			wcsncpy(input,init,n=N-1); input[n] = 0; user = (void*)data;

			window = win; enter = cb; assert(cb);
		}

		bool empty(){ return !this||!n||!*input; }

		void cancel(){ enter(window,this,user,false); }

		void ok(){ enter(window,this,user,true); }
		
		HWND window; callback enter;

		wchar_t input[N];
	};
}

static void SomwindowsRenameWell(HWND well, Somwindows_cpp::rename<0> *re, void *data, bool ok)
{	
	if(ok&&IsWindow(well)&&!re->empty())
	{
		int id = (int)data;

		Somwindows_h::paletteman pm(GetParent(well));

		if(pm&&pm->pal->title(pm->pal->well(id),re->input,re->n))
		{
			switch(id)
			{
			case Somtexture::COLORKEY: break; //case Somtexture::RESERVED:

			default: //"                                ": Wow Somwindows Wow
				
				SetWindowTextW(well,L"                                "); 
				
				SetWindowTextW(well,pm->pal->well(id)); break;
			}
		}		
	}

	delete re;
}

static INT_PTR CALLBACK SomwindowsRenameProc(HWND hdlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg) //May want to do more with this in future
	{
	case WM_INITDIALOG: 
	{				
		Somwindows_cpp::rename<0> *re = 0; 
		
		*(void**)&re = (void*)lParam; assert(re); if(!re) goto close;
		
		SetWindowLongPtr(hdlg,GWLP_USERDATA,lParam);

		SetDlgItemTextW(hdlg,IDC_INPUT,re->input);
		
		Sompaste->wallpaper(hdlg); return TRUE;
	}
	case WM_COMMAND:
		
		switch(LOWORD(wParam))
		{
		case IDOK:

			Somwindows_cpp::rename<0> *re = 0; 
		
			*(void**)&re = (void*)GetWindowLongPtr(hdlg,GWLP_USERDATA);

			GetDlgItemTextW(hdlg,IDC_INPUT,re->input,re->n); 

			SetWindowLongPtr(hdlg,GWLP_USERDATA,0);

			re->ok(); DestroyWindow(hdlg); 
		}	  
		break;

	case WM_CLOSE:
	{	
close:	Somwindows_cpp::rename<0> *re = 0; 
		
		*(void**)&re = (void*)GetWindowLongPtr(hdlg,GWLP_USERDATA);

		if(re) re->cancel(); DestroyWindow(hdlg); break;
	}
	}return 0;
}		
 
extern "C"
static int SomwindowsColorTone(HWND dialog, HWND window, COLORREF *color, int refresh, wchar_t*)
{		
	if(*color==-1) //initialization callback
	{
		return *color = GetWindowLong(window,GWL_USERDATA); 
	}
	else if(!IsWindowVisible(dialog)&&IsWindow(dialog))
	{
		HWND wins[2] = {GetParent(window),dialog};

		Sompaste->cascade(wins); Sompaste->arrange(wins); Sompaste->desktop(dialog);

		SetWindowPos(dialog,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
	}

	Somwindows_h::paletteman pm(GetParent(GetParent(window))); 
	
	if(!pm||!pm.edit()) return 0;

	int tid = GetDlgCtrlID(window)-1000;

	const wchar_t *well = pm->pal->palette_well(tid);

	PALETTEENTRY paint = *(PALETTEENTRY*)color;
	//Reminder: clear seems like overkill for this purpose.
	//Reminder: it sets both the palette and the well boxes.
	if(!well||!pm.clear(pm->pal->paint(well,tid,tid,&paint)))	
	{
		static int once = 0; assert(once++); return 0; 
	}
	
	if(refresh) return 50;

	//TODO: pm->pal->compare()

	return 0;
}

static LRESULT CALLBACK SomwindowsTonesProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam, UINT_PTR scid, DWORD_PTR width)
{
	int tm = GetDlgCtrlID(hwnd)==IDC_TONE_MAP;

	extern int Sommctrl_select(HWND,RECT*,HWND client=0);
		
	switch(Msg)
	{		
	case WM_KEYDOWN:

		if((lParam&(1<<30))==0) //not repeating
		if(GetKeyState(VK_CONTROL)&0x8000) switch(wParam)
		{
		case 'T': //Ctrl+T
		{
			SendMessage(GetParent(hwnd),WM_COMMAND,(WPARAM)IDC_NEW_TONE,0); return 1;
		}
		case 'W': //Ctrl+W
		{
			SendMessage(GetParent(hwnd),WM_COMMAND,(WPARAM)IDC_NEW_WELL,0); return 1;
		}
		case 'A': //Ctrl+A
		{
			Somwindows_h::paletteman pm(GetParent(hwnd)); if(!pm||!pm->pal) break;

			const wchar_t *t0 = pm->pal->tones(pm->pal->palette_well(pm->selection[0]-1000));
			const wchar_t *t1 = pm->pal->tones(pm->pal->palette_well(pm->selection[1]-1000));

			if(!t0||!t1||pm->selection[0]-1000==t0[0]&&pm->selection[1]-1000==t1[1])
			{				
				t0 = pm->pal->tones(pm->pal->well(Somtexture::RESERVED));
			
				pm.xor(pm->pal->colorkey()?1001:1000,t0?t0[1]-1+1000:1255);
			}
			else pm.xor(t0?t0[0]+1000:0,t1?t1[1]+1000:0); break;
		}
		case 'C': case 'X': //Ctrl+C/X
		{
			Somwindows_h::paletteman pm(GetParent(hwnd)); pm.copy(0,wParam=='X'); break;
		}
		case 'V': //Ctrl+[Shift]+V
		{
			Somwindows_h::paletteman pm(GetParent(hwnd)); 
			
			pm.paste(0,GetKeyState(VK_SHIFT)&0x8000); break;
		}
		case 'R': //Ctrl+R
			
			SendMessage(GetParent(hwnd),WM_COMMAND,ID_REVEAL_CUTS,0); break;
			
		}break;

	case WM_SIZE: 
	{		   
		WINDOWINFO info = {sizeof(WINDOWINFO)};

		GetWindowInfo(hwnd,&info);

		//Annoying: include nonclient border
		RECT &c = info.rcClient, &w = info.rcWindow;		

		int ncx = abs(c.left-w.left)+abs(c.right-w.right);  
		int ncy = abs(c.top-w.top)+ abs(c.bottom-w.bottom);		
				
		if(!tm) //hack: assuming windows will remain constant
		{
			Somwindows_tonepads[0] = Somwindows_tonepad+ncx/2; //-info.cxWindowBorders;
			Somwindows_tonepads[0]+=Somwindows_toneseps[0]/2;
		}

		HWND pal = GetParent(hwnd);

		Somwindows_h::paletteman pm(pal); if(!pm) break;

		Somwindows_cpp::TonesEnumRect room; GetClientRect(pal,&room);

//		RECT room; GetClientRect(pal,&room);			
		RECT delta;	GetWindowRect(pal,&delta);	

		int border = w.left-delta.left;

		room.right-=border*2+ncx;
		
		//fontheight: going sans bottom spacer for now
		room.top = Somwindows_h::windowman(pal)->fontheight;

		room.bottom = room.top; //empty

		room.start = 0;

		if(!tm)
		{
			int num = GetDlgCtrlID(hwnd)-IDC_UNTITLED;

			const wchar_t *well = pm->pal->well(pm->pal->UNTITLED+num);
			const wchar_t *tones = pm->pal->tones(well); 
			
			if(num!=IDC_RESERVED-IDC_UNTITLED) //!RESERVED
			{
				room.start = !tones||tones[0]==1&&pm->pal->colorkey()?0:tones[0];
			}
			else room.start = tones?tones[0]:0; //paranoia
		}

		EnumChildWindows(hwnd,SomwindowsTonesEnum,(LPARAM)&room);	

		room.right+=ncx; room.bottom+=ncy; 
				
		HWND bg = GetDlgItem(hwnd,IDC_BACKGROUND);
		SetWindowPos(bg,0,0,room.top/2,room.right,room.bottom,SWP_NOREDRAW);

		const int flags = SWP_NOMOVE|SWP_NOSENDCHANGING|SWP_NOREDRAW;
		SetWindowPos(hwnd,0,0,0,room.right,room.bottom,flags);
		
	}break;
	case WM_CONTEXTMENU:
	{
		POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

		RECT box = {pt.x-tm*2, pt.y-tm*2, pt.x+tm*2, pt.y+tm*2};
				
		int hit = Sommctrl_select(hwnd,&box,hwnd);
				
		Somwindows_h::windowman wm(GetParent(hwnd));		

		Somwindows_h::paletteman pm(wm); if(!pm||!pm->pal) break;

		HMENU menu = wm.context('main'); 
		
		HWND tone = 0; if(hit) tone = GetDlgItem(hwnd,hit);

		bool cancel = GetKeyState(VK_LBUTTON)&0x8000;

		if(!cancel)
		{
			wchar_t choosecolor[64] = L"&Choose Color";
			if(tone) swprintf_s(choosecolor+13,32,L" \t#%d",pm->pal->palette_index(hit-1000)); 
				
			ModifyMenuW(menu,ID_CHOOSECOLOR,tone?MF_ENABLED:MF_GRAYED,ID_CHOOSECOLOR,choosecolor);		
		}
		else ModifyMenuW(menu,ID_CHOOSECOLOR,tm?MF_ENABLED:MF_GRAYED,ID_CHOOSECOLOR,L"&Cancel");		
		
		int ck = pm->pal->colorkey(1)?1:0;
		int id = hit?pm->pal->id(pm->pal->palette_well(hit-1000)):0;

		if(!id&&!tm&&!hit) id = GetDlgCtrlID(hwnd)-IDC_UNTITLED+Somtexture::UNTITLED;

		if(!id) id = tone?Somtexture::COLORKEY:Somtexture::UNTITLED;
															 
		const wchar_t *title = pm->pal->well(id);
		ModifyMenuW(menu,2,MF_BYPOSITION|MF_POPUP,(UINT_PTR)GetSubMenu(menu,2),title);		
		
		DWORD SelectAll = id==Somtexture::COLORKEY?MF_GRAYED:MF_ENABLED;
		DWORD Paste = id==Somtexture::RESERVED?MF_GRAYED:SelectAll;
		DWORD Rotate = id==pm->pal->id(pm->pal->well(ck))?MF_GRAYED:Paste;		
		EnableMenuItem(menu,ID_EDIT_SELECT_ALL,SelectAll);		
		EnableMenuItem(menu,ID_EDIT_PASTE_EOW,Paste);
		EnableMenuItem(menu,ID_EDIT_ROTATE,Rotate);
		
		SetMenuDefaultItem(menu,tm?ID_CHOOSECOLOR:-1,0); 
		SetMenuDefaultItem(GetSubMenu(menu,2),ID_EDIT_SELECT_ALL,0); 
		SetMenuDefaultItem(GetSubMenu(menu,4),ID_SELECT_INDEX,0); 
		SetMenuDefaultItem(GetSubMenu(menu,6),ID_EDIT_CUT,0); 

		int cut = pm->selection[0]||hit?MF_ENABLED:MF_GRAYED;

		if(cut==MF_ENABLED&&hit&&!pm.test(hit)&&tone)
		{	
			wchar_t index[16] = L"Colorkey"; 
			if(!ck||hit!=1000) swprintf_s(index,L"#%d",pm->pal->palette_index(hit-1000)); 
			ModifyMenuW(menu,ID_EDIT_CUT,id==pm->pal->RESERVED?MF_GRAYED:cut,ID_EDIT_CUT,index);		
		}
		else ModifyMenuW(menu,ID_EDIT_CUT,cut,ID_EDIT_CUT,L"Selection");		

		UINT index = pm->pal->channel(Somtexture::RGB);
		if(id<Somtexture::UNTITLED||id>=Somtexture::TITLED+30) index = 0;
		EnableMenuItem(menu,ID_NEW_INDEX,index?MF_ENABLED:MF_GRAYED);

		DWORD Colorkey = ck?MF_GRAYED:MF_ENABLED;
		EnableMenuItem(menu,ID_NEW_COLORKEY,Colorkey);

		BOOL cmd = TrackPopupMenu(menu,TPM_RETURNCMD,pt.x,pt.y,0,hwnd,0);
				
		switch(cmd)
		{
		case ID_CHOOSECOLOR:
		{
			if(cancel)
			{
				pm.xor(pm->selcancel); break;
			}
			else if(!tone) break; //paranoia

			Sompaste->choose(L"color",tone,SomwindowsColorTone);

		}break;
		case ID_EDIT_RENAME:
		{
			HWND well = hwnd; 
			if(tm&&id>=Somtexture::UNTITLED&&id<Somtexture::TITLED+30)
			{			
				well = GetDlgItem(wm,id-Somtexture::UNTITLED+IDC_UNTITLED);
			}

			if(well&&title)
			{
				void *re = new Somwindows_cpp::rename<32>(title,well,SomwindowsRenameWell,id);

				HWND dlg = CreateDialogParamW(Somplayer_dll(),MAKEINTRESOURCEW(IDD_RENAME),hwnd,SomwindowsRenameProc,(LPARAM)re);

				Sleep(2); SendMessage(dlg,WM_NEXTDLGCTL,(WPARAM)GetDlgItem(dlg,IDC_INPUT),1); //hack
			}

		}break;
		case ID_EDIT_SELECT_ALL:
		{
			const wchar_t *tones = pm->pal->tones(title);

			pm.xor(tones?tones[0]+1000:0,tones?tones[1]+1000:0);

		}break;
		case ID_EDIT_ROTATE:
		{
			const wchar_t *tones = pm->pal->tones(title);
			const wchar_t *final = pm->pal->tones(pm->pal->well(-1));
			pm.clear(tones&&final&&pm->pal->map(tones[0],final[1],pm->pal->colorkey()?1:0));

		}break;		
		case ID_NEW_WELL: 
			
			SendMessage(pm.window,WM_COMMAND,(WPARAM)IDC_NEW_WELL,0); break;

		case ID_NEW_TONE: 
			
			SendMessage(pm.window,WM_COMMAND,(WPARAM)IDC_NEW_TONE,0); break;

		case ID_NEW_COLORKEY: 
			
			SendMessage(pm.window,WM_COMMAND,(WPARAM)ID_NEW_COLORKEY,0); break;

		case ID_EDIT_CUT: pm.cut(tone); break;

		case ID_EDIT_COPY: pm.copy(tone); break;

		case ID_EDIT_PASTE: pm.paste(tone); break;

		case ID_EDIT_PASTE_EOW: pm.paste(tone,"to end of well"); break;

		case ID_REVEAL_CUTS: SendMessage(pm.window,WM_COMMAND,cmd,0); break;
		}

		return 0;
		
	}break;	

	//// Begin XOR selection centric code ////////////////////////////////////////////

	//NOTES: this all seems pretty stable. The only trouble is if/when the selection area
	//goes off screen (and possibly if other windows obscure it somehow) while being drawn
	//But the user seems to always be able to wipe up the XOR logic by scrolling/resizing

	case WM_ERASEBKGND: if(!tm) break; 
	{
		//XP: This logic was moved to SomwindowsPaletteProc 
		//because XP would repaint this control on SetCapture and
		//on ReleaseCapture, which would interfere with the XOR logic 
		//This would not be a problem, except that this control is a group
		//element, which presumablyh does not really have a background per se' 
		//So what you would end up with is no erasing and the XOR XOR'ing itself 
		//The code is kept here (where it logically belongs) for reference...
		if(GetWindowTheme(hwnd)) //hack: there are ~3 different behaviors
		{
			//Reminder: still need to confirm XP Style (gummy) theme

			HWND palwin = GetParent(hwnd);
			Somwindows_h::paletteman pm(palwin); if(!pm) break;

			pm->xorselect[0] = pm->xorselect[1] = 0; pm->xor = false; 
			
			HWND bg = GetDlgItem(hwnd,IDC_BACKGROUND); //paranoia

			if(bg) InvalidateRect(bg,0,FALSE); 
		}
	
	}break;
	case WM_SETFOCUS:
	case WM_KILLFOCUS: if(!tm) break;
	{
	//Seems to work to make the selection cutout go way when inactive
	//And while tabbing... so that it behaves like tab focus rectangles

		HWND palwin = GetParent(hwnd);
		Somwindows_h::paletteman pm(palwin); if(!pm) break;

		if(Msg==WM_KILLFOCUS)
		{
			if(!(GetKeyState(VK_TAB)&0x8000))
			if(GetParent((HWND)wParam)==GetParent(hwnd))
			{
				switch(GetDlgCtrlID((HWND)wParam))
				{
				case IDC_NEW_WELL: case IDC_NEW_TONE:

					pm->floating = true; break;

				default: pm->floating = false;
				}
			}
		}
		else if(Msg==WM_SETFOCUS) pm->floating = false;

		//if(pm->floating) break; //some serious voodoo

		pm->infocus = Msg==WM_SETFOCUS;
		
		HWND bg = GetDlgItem(hwnd,IDC_BACKGROUND); 
		if(bg) RedrawWindow(bg,0,0,RDW_ERASE|RDW_INVALIDATE|RDW_UPDATENOW);
		//if(bg) InvalidateRect(bg,0,FALSE); 
		
	}break;
	case WM_DRAWITEM:
	{
		if(wParam==IDC_BACKGROUND)
		{				
			HWND palwin = GetParent(hwnd);
			Somwindows_h::paletteman pm(palwin); if(!pm) break;

			DRAWITEMSTRUCT *p = (DRAWITEMSTRUCT*)lParam; RECT sep = {-2,-2,0,0};			
			
			extern bool Sommctrl_cutout(HWND,HDC,HWND,int,int,RECT*,bool,COLORREF bg=-1);

			int lo = pm->xorselect[0], hi = pm->xorselect[1]; if(hi<lo) std::swap(hi,lo);				
			if(pm->xor&&Sommctrl_cutout(p->hwndItem,p->hDC,hwnd,lo,hi,&sep,!pm->xor)) pm->xor = !pm->xor; 

			if(!pm->xor&&pm->infocus) 
			{	
				lo = pm->selection[0], hi = pm->selection[1]; if(hi<lo) std::swap(hi,lo);
				if(Sommctrl_cutout(p->hwndItem,p->hDC,hwnd,lo,hi,&sep,!pm->xor)) pm->xor = !pm->xor;

				pm->xorselect[0] = pm->selection[0]; pm->xorselect[1] = pm->selection[1]; 
				
				static int once = 0; assert(pm->xor||!lo||once++);
			}			
		}

	}break;	
    case WM_LBUTTONDOWN: 
		
		if(!tm) SetFocus(hwnd); break;

	case WM_COMMAND: 
	
		if(HIWORD(wParam)==STN_CLICKED
		  ||HIWORD(wParam)==STN_DBLCLK) //ChooseColor
		{
			if(!tm) SetFocus(hwnd);

			if(LOWORD(wParam)==IDC_BACKGROUND)
			if(GetKeyState(VK_LBUTTON)&0x8000)
			{
				HWND palwin = GetParent(hwnd);	 
				Somwindows_h::paletteman pm(palwin); if(!pm) break;
				  				
				DWORD pos = GetMessagePos(); RECT box = 
				{
					GET_X_LPARAM(pos)-2, GET_Y_LPARAM(pos)-2,
					GET_X_LPARAM(pos)+2, GET_Y_LPARAM(pos)+2
				};
				
				pm->hit = Sommctrl_select(hwnd,&box,hwnd);
								
				if(pm->hit)
				{
					if(GetFocus()!=hwnd) SetFocus(hwnd); 

					SetCapture(hwnd); 

					if(pm->dragging=pm.test(pm->hit))
					{
						SetCursor(LoadCursor(0,IDC_IBEAM));
					}
				}
				else if(GetFocus()!=hwnd)
				{	
					SetFocus(hwnd);
				}
				else pm.xor(0);

				pm->selcancel[0] = pm->selection[0];
				pm->selcancel[1] = pm->selection[1];
			}
		}		

	break;
	case WM_MOUSEMOVE: 
	{			
		if(!tm||GetCapture()!=hwnd) break;
		
		Somwindows_h::paletteman pm(GetParent(hwnd)); if(!pm) break; 
		
		if(pm->dragging) //manage cursor and break
		{
			RECT client; GetClientRect(hwnd,&client);

			int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

			if(pm->x!=(x<0||x>client.right||y<0||y>client.bottom))
			{
				pm->x = !pm->x; SetCursor(LoadCursor(0,pm->x?IDC_NO:IDC_IBEAM)); 
			}
		}
		
		RECT box = 
		{ 
			GET_X_LPARAM(lParam)-2, GET_Y_LPARAM(lParam)-2,
			GET_X_LPARAM(lParam)+2, GET_Y_LPARAM(lParam)+2 
		};

		int id = Sommctrl_select(hwnd,&box); 

		int ck = pm->pal->colorkey(1)?1000:-1; 
		
		if(ck&&pm->hit==ck)
		{
			if(id!=ck) pm->hit++; else break;
		}
		else if(id==ck) id++; 

		if(id!=pm->hit&&id!=pm->selection[1])
		{
			if(!pm->dragging)
			{
				if(pm->hit) //let the selection begin
				{
					pm->selection[0] = pm->hit; pm->hit = 0;
				}

				pm->selection[1] = id;
				HWND bg = GetDlgItem(hwnd,IDC_BACKGROUND);
				if(bg) InvalidateRect((HWND)bg,0,FALSE);			
			}
			else pm->hit = 0;
		}		

	}break;	
	case WM_CAPTURECHANGED:
	{
		if((HWND)lParam!=hwnd) SetCursor(LoadCursor(0,IDC_ARROW));

		return 1;

	}break;
	case WM_LBUTTONUP: 
	{
		if(GetCapture()==hwnd) ReleaseCapture(); 

		HWND palwin = GetParent(hwnd); if(!tm) break;
		Somwindows_h::paletteman pm(palwin); if(!pm) break;		
				
		if(pm->hit) //clicking
		{
			if(pm->hit==1000&&pm->pal->colorkey(1)
			 ||pm->selection[0]==pm->hit&&pm->selection[1]==pm->hit)
			{				
				HWND tone = GetDlgItem(hwnd,pm->hit?pm->hit:1000);

				Sompaste->choose(L"color",tone,SomwindowsColorTone);
			}
			else pm.xor(pm->hit);
		}
		else if(pm->dragging&&!pm->x&&pm->selection[0]) //dropping
		{				
			extern int Sommctrl_insert(HWND,RECT*,HWND client=0);

			RECT box = 
			{ 
				GET_X_LPARAM(lParam)-2, GET_Y_LPARAM(lParam)-2,
				GET_X_LPARAM(lParam)+2, GET_Y_LPARAM(lParam)+2 
			};

			int a = pm->selection[0]-1000;
			int z = pm->selection[1]-1000; if(z<a) std::swap(a,z);

			int aa = Sommctrl_insert(hwnd,&box)-1000; 

			//hack#1: hose xor prior to pm.clear()
			SendMessage(hwnd,WM_KILLFOCUS,0,0); 

			if(aa<0)
			{
				const wchar_t *ut = 
				pm->pal->tones(pm->pal->well(pm->pal->UNTITLED));
				
				if(pm->pal->map(a,z,aa=ut?ut[1]+1:256)) pm.clear();

				ut = pm->pal->well(pm->pal->UNTITLED); 

				if(aa==256) //hack: borrowing ut 
				{
					const wchar_t *tz =
					pm->pal->tones(pm->pal->well(-1)); 
					pm->pal->paint(ut,tz?tz[1]-(z-a):-1,aa=tz?tz[1]:-1); 
					aa++;
				}
				else if(aa>a) //need a function for this #1
				{
					pm->pal->paint(ut,aa-1,a-z+aa-1);
				}
				else pm->pal->paint(ut,aa,z-a+aa); 			
			}
			else 
			{
				int hit = Sommctrl_select(hwnd,&box)-1000;

				int op = hit==aa?pm->pal->BEFORE:pm->pal->BEHIND;

				int ck = pm->pal->colorkey()?1:0; if(aa==ck-1) aa++;

				if(aa==ck&&op==pm->pal->BEHIND) //tricky
				{
					const wchar_t *ta = 
					pm->pal->tones(pm->pal->palette_well(a));

					if(!ta||ta&&ta[0]!=a) op = pm->pal->BEFORE;
				}
				//else if(op==pm->pal->BEFORE) aa--;

				pm->pal->map(a,z,aa,op);
			}

			bool xor = pm.clear();
			
			//hack#2: restore xor state to normal
			SendMessage(hwnd,WM_SETFOCUS,0,0); 

			if(xor)	if(aa>a) //need a function for this #2
			{
				pm.xor(aa-1+1000,a-z+aa-1+1000);
			}
			else pm.xor(aa+1000,z-a+aa+1000); 			
		}
		
		return 1;

	}break;

	//// End of XOR selection centric code ///////////////////////////////////////////

	case WM_NCDESTROY:
	
		RemoveWindowSubclass(hwnd,SomwindowsTonesProc,scid);
		break;
	}

	return DefSubclassProc(hwnd,Msg,wParam,lParam);
}

static LRESULT CALLBACK SomwindowsEraseProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam, UINT_PTR scid, DWORD_PTR /*bg*/)
{
	switch(Msg)
	{
	case WM_ERASEBKGND: 
	{
		//The user data can't be set across threads. Probably the Color
		//Chooser shouldn't send its callback across threads.
		HBRUSH brush = CreateSolidBrush(GetWindowLong(hwnd,GWL_USERDATA)); //bg

		RECT fill; GetClientRect(hwnd,&fill);	

		int success = FillRect((HDC)wParam,&fill,brush);

		DeleteObject(brush); 

	}return 1;
	case WM_NCDESTROY:
	
		RemoveWindowSubclass(hwnd,SomwindowsEraseProc,scid);
		break;
	}
									
	return DefSubclassProc(hwnd,Msg,wParam,lParam);
}

bool Somwindows_h::windowman::subclass(const wchar_t *assignment, int type, int id)
{
	if(!props) return false;
			
	if(type==Somconsole::CONTROL)
	{
		switch(props->windowtype)
		{
		case Somconsole::CONTROL: 
		case Somconsole::PICTURE: break;

		//TEXTURE/PALETTE do not require controls

		default: assert(0); return false;
		}
	}
	else if(props->subclass)
	{
		if(props->windowtype==type)
		{
			destroy(props); //hack: hold user workspace together
						
			if(props->release) props->release(props->windowdata);

			DestroyIcon(props->icon); props->icon = 0; //hack
		}
		else destroy(new wm_props);
	}
	
	if(type!=Somconsole::CONTROL)
	{
		props->content = assignment;
		props->windowtype = type; props->resourceid = id;
	}
	else
	{
		props->subject = assignment;
		props->controller = id;
	}

	switch(type)
	{
	case Somconsole::TEXTURE: 
		
		props->subclass = SomwindowsTextureProc; break;
	
	case Somconsole::PALETTE: 
	{
		const char *pal = MAKEINTRESOURCE(IDD_PALETTE);

		//TODO: try to make this work... similar to Properties
		//CreateDialog(Somplayer_dll(),pal,window,SomwindowsPaletteProc???);

		EndDialog(adopt(CreateDialog(Somplayer_dll(),pal,window,SomwindowsPaletteInit)),0);
							
		props->subclass = SomwindowsPaletteProc; break; 
	}
	case Somconsole::PICTURE: 
		
		props->subclass = SomwindowsPictureProc; break;

	case Somconsole::CONTROL: if(props->windowtype) break; //PICTURE+CONTROL

		assert(0); //props->subclass = SomwindowsControlProc;
	}

	SetWindowSubclass(window,props->subclass,0,0);

	return true;
}

HWND Somwindows_h::paletteman::match(HWND tone, HWND set)
{
	HWND out = 0;

	int tid = GetDlgCtrlID(tone);
	if(!set||GetDlgCtrlID(set)==IDC_TONE_MAP)
	{
		const Somtexture *pal = image?image->pal:0;

		int wid = pal->id(pal->palette_well(tid-1000)); 

		switch(wid)				
		{
		case pal->COLORKEY: 

			wid = pal->id(pal->palette_well(1)); 

			if(!wid||wid==pal->RESERVED) wid = pal->UNTITLED; break;

		case pal->RESERVED: case 0: assert(0); return 0;
		}
		
		out = GetDlgItem(GetDlgItem(window,wid-pal->UNTITLED+IDC_UNTITLED),tid);
	}

	if(!out||out==tone) out = GetDlgItem(GetDlgItem(window,IDC_TONE_MAP),tid);

	return out;
}	

size_t Somwindows_h::paletteman::clipboard_s = 0;
PALETTEENTRY Somwindows_h::paletteman::clipboard[256];

void Somwindows_h::paletteman::copy(HWND tone, bool cut)
{
	int tn = GetDlgCtrlID(tone)-1000; RECT pick = clip(tn);
	
	if(IsRectEmpty(&pick)||!edit(tn<0)){ MessageBeep(-1); return; }

	const wchar_t *tones = image->pal->tones(image->pal->well(-1));

	int ck = image->pal->colorkey()?1:0, tz = tones?tones[1]+1:0; 

	int a = pick.left, z = pick.right-1; clipboard_s = z-a+1;

	memcpy(clipboard,image->pal->palette+a,sizeof(PALETTEENTRY)*clipboard_s);

	if(cut&&clear(tz-clipboard_s>0&&image->pal->map(a,z,256,Somtexture::BEFORE)))
	{
		if(a!=tn||z!=tn||tn==tz-1) xor(0); 
	}
}

void Somwindows_h::paletteman::paste(HWND tone, bool ontoendofwell)
{		
	if(!image||!image->pal||!image->pal->palette) return;

	if(!clipboard_s){ MessageBeep(-1); return; } //paranoia

	int tn = GetDlgCtrlID(tone)-1000; RECT pick = clip(tn);

	const wchar_t *final = image->pal->well(-1);
	const wchar_t *tones = image->pal->tones(final), *well;

	int ck = image->pal->colorkey()?1:0, tz = tones?tones[1]+1:0; 

	int a = pick.left, z = pick.right-1, rgb = 0xFFFFFF; //mask

	if(!IsRectEmpty(&pick))
	{
		int cut = tn<0&&image->selection[0]||test(tn)?z-a+1:0;

		if(ontoendofwell) //add clipboard to the end
		{
			if(clipboard_s<=256-tz)
			{
				well = image->pal->palette_well(z); 
				
				tones = image->pal->tones(well); z = tones?tones[1]+1:tz;

				if(!clear(image->pal->map(256,256+clipboard_s-1,z,Somtexture::BEHIND))) 
				return;

				image->pal->paint(well,z,z+clipboard_s-1,clipboard,rgb,0); 
			}
		}
		else if(cut) //replace selection with clipboard
		{
			if(clipboard_s<=256-tz+cut) 
			{
				well = image->pal->palette_well(a); 

				if(cut>clipboard_s)
				{
					if(!clear(image->pal->map(a+clipboard_s,z,256))) return;
				}
				else if(cut<clipboard_s)
				{
					if(!clear(image->pal->map(256,256+clipboard_s-cut-1,a+cut))) return; 					
				}

				image->pal->paint(well,a,a+clipboard_s-1,clipboard,rgb,0); 

				xor(0);
			}
		}
		else if(clipboard_s<=256-tz) //add clipboard beofre a~z
		{
			well = image->pal->palette_well(a); assert(a-z==0);

			if(!clear(image->pal->map(256,256+clipboard_s-1,a))) return;

			image->pal->paint(well,a,a+clipboard_s-1,clipboard,rgb,0); 
		}
	}
	else if(clipboard_s<=256-tz) //tack onto end of map
	{
		if(!clear(image->pal->map(tz,tz+clipboard_s-1,tz,Somtexture::BEHIND))) return;
		image->pal->paint(final,tz,tz+clipboard_s-1,clipboard,rgb,0); 
	}
	
	clear();
}

bool Somwindows_h::paletteman::clear(bool ok)
{				
	if(!ok) return MessageBeep(-1),false;

	bool resize = false; int newtones = 0;

	const Somtexture *pal = image?image->pal:0;

	HWND map = GetDlgItem(window,IDC_TONE_MAP);
	
	int ck = pal->colorkey()?1:0, id, j, z;
	int num = pal->id(pal->well(ck))-pal->UNTITLED; //default

	const wchar_t *p = 0, *q = 0, *tones = 0; 
	for(int i=0;tones=pal->tones(pal->reserve(i));i++) 
	if(tones[0]==ck) num = pal->id(pal->title(pal->reserve(i)))-pal->UNTITLED;

	HWND w0 = GetDlgItem(window,IDC_UNTITLED+(num<0?0:num));
	HWND rw = GetDlgItem(window,IDC_RESERVED);

	if(!w0) w0 = GetDlgItem(window,IDC_UNTITLED);

	bool cuts = IsWindowEnabled(rw);
 
	int beep = -1, cut = 0, fin = 0; 
	
	tones = pal->tones(p=pal->reserve(0));

	for(int i=1;tones||!fin;i++)
	{	
		HWND src = 0, dst; //if(!fin||cuts)
		{
			num = 1+pal->no(p=pal->title(p)); //&~RESERVED

			src = num<0?w0:GetDlgItem(window,IDC_UNTITLED+num);
		}			
		dst = src; int pnum = num; 

		if(tones) for(beep=id=0,j=tones[0],z=tones[1];j<=z;j++)
		{
			if(id!=pal->palette[j].paint)
			{					
				num = 1+pal->no(q=pal->well(id=pal->palette[j].paint)); 

				dst = num<0?w0:GetDlgItem(window,IDC_UNTITLED+num);

				if(!IsWindowVisible(dst)&&q&&id!=pal->RESERVED)
				{
					if(!dst) dst = Somwindows_New_Well(window,num,q);
					//hack: keep IDC_UNTITLED around so that it can 
					//be used by Somwindows_New_Well to make new wells
					if(num==0) ShowWindow(dst,SW_SHOW);
				}

				if(num<0) text(); //hack
			}

			if(j==1&&ck&&src!=dst&&dst) //COLORKEY
			{
				if(!src) ShowWindow(w0,SW_HIDE); //UNTITLED

				SetParent(GetDlgItem(w0,1000),dst); 
			}
						
			COLORREF c = (*(COLORREF*)&pal->palette[j])&0x00FFFFFF; 

			if(!newtones)
			{
				HWND a = GetDlgItem(map,j+1000), b = GetDlgItem(src,j+1000);
									
				if(id==pal->RESERVED&&++cut) if(!cuts)
				{	
					DestroyWindow(a); DestroyWindow(b); resize = true; continue;
				}
				else ShowWindow(rw,SW_SHOW);
					
				//2017: Windows 10 won't do this across threads. Or I changed
				//the Sompaste color picker API to be modeless for a callback.
				//SetWindowSubclass(a,SomwindowsEraseProc,0,c);
				//SetWindowSubclass(b,SomwindowsEraseProc,0,c);
				SetWindowLong(a,GWL_USERDATA,c); SetWindowLong(b,GWL_USERDATA,c);

				if(a) InvalidateRect(a,0,TRUE); if(b) InvalidateRect(b,0,TRUE);

				if(src!=dst){ SetParent(b,dst); resize = true; }
			}
			else
			{
				if(num<0) num = GetDlgCtrlID(w0)-IDC_UNTITLED; //hack

				int t = Somwindows_New_Tone(window,num,j,c); assert(t==j+1000);
				
				newtones--; resize = true; 
			}	
		}

		if(!ck||src!=w0)
		if(!pal->tones(p)&&pnum>=0) //!fin
		if(pnum!=pal->RESERVED-pal->UNTITLED) //hack
		{
			if(pnum>0) pal->title(p,L"",1); //TITLED+N
			if(pnum>0) DestroyWindow(src); resize = true;			

			//hack: keep IDC_UNTITLED around so that it can 
			//be used by Somwindows_New_Well to make new wells
			if(pnum==0) ShowWindow(src,SW_HIDE); 
		}
		else ShowWindow(src,SW_HIDE);

		if(tones=pal->tones(p=pal->reserve(i))) continue;

		if(tones=pal->tones(p=pal->reserve(pal->REPLACED))) 
		{
			if(!cuts) newtones = tones[1]-tones[0]+1;
		}

		fin = 1; pal->reserve(pal->FINISHED);
	}
	
	if(resize) 
	{
		text(); windowman(window).resize(); 
	}

	if(beep) MessageBeep(beep);

	return beep!=-1;
}

void CALLBACK Somwindows_h__pictureman__play_cb(UINT uID, UINT, DWORD_PTR dwUser, DWORD, DWORD)
{								  
	Somwindows_h::windowman wm((HWND)dwUser); 

	Somwindows_h::pictureman pm(wm); if(pm) pm->refresh(wm); else timeKillEvent(uID); 	
}

bool Somwindows_h::pictureman::play(int fps)
{
	if(!video) return false;

	//TODO: consider TIME_KILL_SYNCHRONOUS
	if(video->timer) timeKillEvent(video->timer); video->timer = 0;

	if(fps==-1) video->refresh_rate = 0; //stopped

	if(fps>60) fps = 60; else if(fps<=0) return true;

	int ms = 1000/fps, res = ms/2; //a smaller value will result in more accuracy		

	video->timer = 
	timeSetEvent(fps,res,Somwindows_h__pictureman__play_cb,(DWORD_PTR)window,TIME_PERIODIC);	

	return video->timer;
}