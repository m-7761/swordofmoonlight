			
#include <locale.h> //setlocale()
#include <stdio.h> 
#include <iostream>
#include <iomanip>
#include <fstream>
#include <assert.h>
#include <io.h> //_open_osfhandle

//#define _WIN32_WINNT 0x0500 //AttachConsole
#define _WIN32_WINNT 0x0600 //ChangeWindowMessageFilter
#define WIN32_LEAN_AND_MEAN	
#include <windows.h> 
#include <windowsx.h> //GET_X_LPARAM
#include <afxres.h> //ID_EDIT_UNDO								  
#include <wchar.h> //_wrename()

//REMINDER: GetThemeMetric vs GetSystemMetrics
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib,"comctl32.lib")
#include <uxtheme.h>
#pragma comment(lib,"uxtheme.lib")

#include <Shellapi.h> //CommandLineToArgvW()
#include <shlwapi.h> //PathFileExistsW()   
#pragma comment(lib,"shlwapi.lib")	

#include <commctrl.h> //tabs control
#include <commdlg.h> //GetOpenFileName
											 
#include "../Sompaste/Sompaste.h"
#include "../Somplayer/Somplayer.h"
#include "../Somversion/Somversion.h"

#include "PalEdit.h"

SOMPASTE Sompaste = 0;
SOMPLAYER Somplayer = 0;

HICON icon = 0;
HMENU menu = 0;

HWND clipboard = 0;

const wchar_t *model = 0; 

HWND tabs = 0; int tab = 0;

HMENU tabsmenu = LoadMenu(0,MAKEINTRESOURCE(IDR_TABS_MENU));

const int wins = 3;

HWND windows[wins] = {0,0,0};

HWND &PalEdit = windows[0], texture = 0;
HWND &Palette = windows[1], palette = 0;
HWND &Picture = windows[2], picture = 0;

RECT memory[wins]; 
int _sticky[wins];

int *sticky = _sticky;

void PalEditSticky()
{
	sticky = sticky?0:_sticky;

	CheckMenuItem(menu,ID_WINDOW_STICKY,sticky?MF_CHECKED:MF_UNCHECKED);
}
void PalEditClipboard()
{		
	if(!IsWindowVisible(clipboard))
	{	
		clipboard = Sompaste->clipboard(PalEdit);
		ShowWindow(clipboard,SW_SHOWNOACTIVATE);
	}
	else if(IsIconic(clipboard))
	{
		ShowWindow(clipboard,SW_SHOWNOACTIVATE);
	}
	else ShowWindow(clipboard,SW_MINIMIZE);

	CheckMenuItem(menu,ID_WINDOW_CLIPBOARD,clipboard?MF_CHECKED:MF_UNCHECKED);
}

UINT PalEditActive(HWND hWndDlg,UINT Msg) //experimental
{	
	Sleep(5); //1: we want the active window to stay down
	
	Sompaste->sticky(windows,sticky,memory); return 0;
}
		
void PalEditRestore(HWND hWndDlg)
{
	RECT client; GetClientRect(hWndDlg,&client);
	SendMessage(hWndDlg,WM_SIZE,SIZE_RESTORED,MAKELPARAM(client.right,client.bottom));
}

int theme = 0; //Sword of Moonlight or Explorer

BOOL CALLBACK ThemeCallback(HWND hwnd, LPARAM lParam)
{
	//Can't let top-level windows be themed
	//because the nonclient area is changed
	//if(!GetAncestor(hwnd,GA_PARENT)) return 1;
	//good grief windows can be pretty f'd!
	if(!IsChild(GetParent(hwnd),hwnd)) return 1;

	if(hwnd==texture||hwnd==palette) //hack: scrollbars/texture
	{
		EnableThemeDialogTexture(texture,theme?ETDT_ENABLETAB:ETDT_DISABLE);
		
		SendMessage(hwnd,WM_THEMECHANGED,0,0); return 1; //notify of change
	}

	static HFONT gothic = (HFONT)SendMessage(PalEdit,WM_GETFONT,0,0);

	if(SendMessage(hwnd,WM_GETDLGCODE,0,0)&DLGC_STATIC)
	{	
		//Deep border looks good with 3D buttons
		//Sunken looks better with modern Explorer
		if(SS_SUNKEN&GetWindowLong(hwnd,GWL_STYLE))
		{	
			DWORD exstyle = GetWindowLong(hwnd,GWL_EXSTYLE);		

			if(lParam>0) //Explorer
			{
				SetWindowLong(hwnd,GWL_EXSTYLE,exstyle&~WS_EX_CLIENTEDGE);
			}
			else //Moonlight
			{				
				SetWindowLong(hwnd,GWL_EXSTYLE,exstyle|WS_EX_CLIENTEDGE);
			}

			SetWindowPos(hwnd,0,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED);

			if(GetParent(hwnd)==Picture) //hack
			{
				PalEditRestore(Picture); //IDC_PICTURE
			}
		}
	}

	if(lParam>0) //Explorer
	{
		SetWindowTheme(hwnd,L"Explorer",0); 

		static HFONT guifont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

		SendMessage(hwnd,WM_SETFONT,(WPARAM)guifont,MAKELPARAM(1,0)); 
	}
	else //Moonlight
	{
		SetWindowTheme(hwnd,L" ", L" "); 	

		SendMessage(hwnd,WM_SETFONT,(WPARAM)gothic,MAKELPARAM(1,0)); 
	}
	
	return 1;
}

void PalEditTheme(HWND window, int Theme=-1)
{
	//DOES NOT DO A DAMN THING!?!
	//Note: importantly omitting STAP_ALLOW_NONCLIENT
	//SetThemeAppProperties(STAP_ALLOW_CONTROLS|STAP_ALLOW_WEBCONTENT);	

	if(Theme==-1) Theme = ::theme;

	ThemeCallback(window,Theme);
	EnumChildWindows(window,ThemeCallback,Theme);	
}

void PalEditChangeTheme(int Theme)
{
	theme = Theme;

	CheckMenuRadioItem(menu,ID_THEME_MOONLIGHT,ID_THEME_EXPLORER,ID_THEME_MOONLIGHT+theme,0);

	for(int i=0;i<wins;i++) if(windows[i])
	{
		PalEditTheme(windows[i]); //hmm: doesn't touch Properties etc...

		//Though on second thought Properties looks better with Explorer..
		//And the ListView header doesn't even yet work without it???

		InvalidateRect(windows[i],0,TRUE);
		UpdateWindow(windows[i]);
	}
}

wchar_t box[MAX_PATH+32] = L"PalEdit";

DWORD status = 0; 

INT_PTR CALLBACK PalEditStatusDialog(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg)
	{
	case WM_INITDIALOG: 
		
		status = !Somplayer->status; //lazy: FALL THRU

		PalEditTheme(hWndDlg);
	
	case WM_TIMER:

		if(Somplayer->status!=status)
		{			
			SetDlgItemTextW(hWndDlg,IDC_STATUSMSG,Somplayer->message);

			status = Somplayer->status;
		}

		SendMessage(GetDlgItem(hWndDlg,IDC_PROGRESS),PBM_SETPOS,Somplayer->progress*100,0);

		SetTimer(hWndDlg,1,500,0); break;

	case WM_COMMAND:

		switch(wParam)
		{
		case IDCANCEL: goto close;
		}		

		break;

	case WM_CLOSE: KillTimer(hWndDlg,1);

close:	EndDialog(hWndDlg,status=Somplayer->status);		
	}

	return FALSE;
}

INT_PTR CALLBACK PalEditModelDialog(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	//goto close; //testing

	switch(Msg)
	{
	case WM_INITDIALOG:

#ifdef NDEBUG

		if(!model) goto close; //phantom check
#endif
		if(Picture) goto close; //there can be only one 
			   		
		picture = Sompaste->create(Picture=hWndDlg); //make a first class window

		PalEditRestore(hWndDlg); break;

	case WM_NCACTIVATE:
	case WM_ACTIVATE: return PalEditActive(hWndDlg,Msg); //experimental	
	case WM_SETFOCUS: SetFocus(picture); return 1;
	case WM_SIZE:
	{			
		int w = LOWORD(lParam), h = HIWORD(lParam); //debugging

		int x = GetSystemMetrics(SM_CXVSCROLL)-GetSystemMetrics(SM_CXEDGE);
		int y = GetSystemMetrics(SM_CYHSCROLL)-GetSystemMetrics(SM_CXEDGE);
				
		HWND frame = GetDlgItem(Picture,IDC_PICTURE);

		RECT rect = Sompaste->frame(frame,hWndDlg); //for later...

		MoveWindow(frame,x,y,LOWORD(lParam)-x*2,HIWORD(lParam)-y*2,0);					

		RECT pic = Sompaste->pane(frame,hWndDlg);

		MoveWindow(picture,pic.left,pic.top,pic.right-pic.left,pic.bottom-pic.top,0);
						
		//avoid erasing behind picture
		InvalidateRect(hWndDlg,&rect,1);		
		rect = Sompaste->frame(frame,hWndDlg);
		InvalidateRect(hWndDlg,&rect,1);	
		ValidateRect(hWndDlg,&pic);		
		UpdateWindow(hWndDlg);
		rect = Sompaste->pane(frame,frame);
		InvalidateRect(frame,0,1);		
		ValidateRect(frame,&rect);				
		UpdateWindow(frame);

	}break;
	case WM_CONTEXTMENU:
	{	
		WINDOWINFO info = {sizeof(WINDOWINFO)};

		if(GetWindowInfo(picture,&info))
		{
			POINT pt = { LOWORD(lParam), HIWORD(lParam) };

			if(PtInRect(&info.rcClient,pt)) 
			{
				return SendMessage(picture,Msg,wParam,lParam);
			}
		}

	}break;	
	case WM_CLOSE:

	close: Picture = picture = 0;
		
		DestroyWindow(hWndDlg);
	}

	return 0;
}

INT_PTR CALLBACK PalEditPaletteDialog(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg)
	{
	case WM_INITDIALOG:

#ifdef NDEBUG

		if(!model) goto close; //phantom check
#endif
		if(Palette) goto close; //there can be only one (for now)

		//palette = GetDlgItem(Palette=hWndDlg,IDC_PALETTE);	

		palette = Sompaste->create(Palette=hWndDlg); //make a first class window		

		PalEditRestore(hWndDlg); break;

	case WM_NCACTIVATE:
	case WM_ACTIVATE: return PalEditActive(hWndDlg,Msg); //experimental
	
	case WM_SIZE:
	{			
		MoveWindow(palette,0,0,LOWORD(lParam),HIWORD(lParam),1); break;
	}		
	case WM_CLOSE:

	close: Palette = palette = 0;
		
		DestroyWindow(hWndDlg);
	}

	return 0;
}

HWND PalEditCheckWindow(HWND &out, UINT id, UINT idd, DLGPROC dialog, bool check)
{	 	
	int state = GetMenuState(menu,id,MF_BYCOMMAND);

	if(state==-1){ assert(0); return 0; } //TODO: spaz out (error)
										 
	bool checked = state&MF_CHECKED;

	if(check)
	{
		UINT check = MF_BYCOMMAND|(checked?MF_UNCHECKED:MF_CHECKED);

		if(CheckMenuItem(menu,id,check)!=-1) checked = !checked;
	}

	if(checked) //TODO: Save/restore position/dimensions
	{
		//Assuming W may produce a more Unicode friendly experience
		if(!out) out = CreateDialogW(0,MAKEINTRESOURCEW(idd),PalEdit,dialog);
	}
	else if(out&&DestroyWindow(out)) out = 0; 

	return out;
}

void PalEditPictureWindow(bool check=false)
{	
	if(PalEditCheckWindow(Picture,ID_VIEW_MODEL,IDD_MODEL,PalEditModelDialog,check))
	{
		Somplayer->capture(0); Somplayer->picture(picture);	Somplayer->control(picture);
		Somplayer->release(); PalEditTheme(Picture);

		SendMessage(picture,WM_SETFOCUS,0,0); //hack: make player character
	}
}

void PalEditPaletteWindow(bool check=false)
{
	if(PalEditCheckWindow(Palette,ID_VIEW_PALETTE,IDD_PALETTE,PalEditPaletteDialog,check))
	{
		Somplayer->capture(model); Somplayer->palette(palette,tab);	Somplayer->capture(0);
		Somplayer->release(); PalEditTheme(Palette);
	}
}

void PalEditShowTab(int n)
{				
	Somplayer->capture(model);
	Somplayer->texture(texture,tab=n);
	Somplayer->palette(palette,n);
	Somplayer->release();
				   	
	PalEditTheme(palette); 
}

void PalEditFileImport(HWND hwnd, const wchar_t *drop=0)
{	
 //todo: OFN_NOREADONLYRETURN?
	static wchar_t cust[MAX_PATH] = L"";
	static wchar_t file[MAX_PATH] = L"";
	static wchar_t name[MAX_PATH] = L"";	

	static OPENFILENAMEW open = 
	{
		sizeof(OPENFILENAMEW),hwnd,0,

		L"Sword of Moonlight: txr, msm, mdo, mdl\0*.txr;*.msm;*.mdo;*.mdl;\0"		
		L"D3DXLoadSurfaceFromFile: bmp, dds, png, jpg, tga, dib, hdr, pfm, ppm\0"
		L"*.bmp;*.dds;*.dib;*.hdr;*.jpg;*.pfm;*.png;*.ppm;*.tga\0"
		L"Sony PlayStation: tim\0*.tim;\0",	

		cust, MAX_PATH, 0, file, MAX_PATH, name, MAX_PATH, 0,

		//OFN_NOVALIDATE: the MDL browsing extension for some reason
		//causes an ugly "Path does not exist" message box to appear 
		//in Vista. No clue why; everything seems in order. The docs
		//for OFN_NOVALIDATE do not suggest that it is of any use to
		//us, however the docs for FOS_NOVALIDATE are way more clear

		L"PalEdit Import", OFN_NOVALIDATE,

		0, 0, 0, 0, 0, 0, 0, 0
	};
	
	if(drop)
	{
		//if(!PathFileExistsW(drop)) return;

		if(PathIsRelativeW(drop))
		{
			if(!GetCurrentDirectoryW(MAX_PATH,name)) return;

			swprintf_s(file,L"%s\\%s",name,drop);
		}
		else wcscpy(file,drop); wcscpy(name,PathFindFileNameW(file));
	}
	else if(!GetOpenFileNameW(&open)) return; 	
	
	if(!Somplayer->open(file)) return; 		
	
	wchar_t text[MAX_PATH];	swprintf(text,MAX_PATH,L"PalEdit Importing %s",name);

	if(!*Somplayer|| //display status dialog if file is not already opened by now
	   !DialogBoxParamW(0,MAKEINTRESOURCEW(IDD_STATUS),0,PalEditStatusDialog,(LPARAM)text))
	{
		Sompaste->reset(); //undo history

		Somplayer->capture(0);
		Somplayer->surrounding(&model,1);
		Somplayer->capture(model);
				
		swprintf(box,MAX_PATH,L"%s - PalEdit",name);
		SetWindowTextW(hwnd,box);		
			
		int n = Somplayer->texture();

		TabCtrl_DeleteAllItems(tabs); 

		HIMAGELIST il = 
		TabCtrl_GetImageList(tabs); 
		ImageList_SetImageCount(il,0);

		for(tab=0;tab<n;tab++) 
		{				
			TCITEMW item = { TCIF_IMAGE, 0, 0, 0, 0, tab }; 
			SendMessage(tabs,TCM_INSERTITEMW,tab,(LPARAM)&item);

			//texture's wndproc Will setup
			//the tab titles inadvertantly
			Somplayer->texture(texture,tab);			
			
			HICON ico =	(HICON)
			SendMessage(texture,WM_GETICON,ICON_SMALL,0);
			ImageList_AddIcon(il,ico);
		}
		
		Somplayer->release();

		HWND lock = GetForegroundWindow();

		PalEditPaletteWindow(); 
		PalEditPictureWindow();

		SetForegroundWindow(lock);

		swprintf(text,MAX_PATH,L"%s Palette View",box);
		SetWindowTextW(Palette,text);
		swprintf(text,MAX_PATH,L"%s Model View",box);
		SetWindowTextW(Picture,text);

		PalEditShowTab(0);

		Sompaste->arrange(windows);

		UpdateWindow(tabs);
	}
	else 
	{
		SetWindowTextW(hwnd,wcscpy(box,L"PalEdit"));		

		Somplayer->open(0); //close
	}
}

/*INT_PTR PalEditSynthKey(int vk, bool shift=false)
{
	INPUT i = {INPUT_KEYBOARD}; memset(&i.ki,0x00,sizeof(i.ki));

	i.ki.wVk = VK_SHIFT; if(shift) SendInput(1,&i,sizeof(i));

	i.ki.wVk = vk; SendInput(1,&i,sizeof(i));		

	i.ki.dwFlags = KEYEVENTF_KEYUP; SendInput(1,&i,sizeof(i));

	i.ki.wVk = VK_SHIFT; if(shift) SendInput(1,&i,sizeof(i));

	return 1;
}*/

INT_PTR CALLBACK PalEditMainDialog(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{		
	static int dragging = 0, fullscreen = 0;

	static WNDPROC tabctrlproc = 0; //hack
	static WNDPROC textureproc = 0; //hack

	if(Msg==WM_DROPFILES)
	{
		wchar_t drop[MAX_PATH];

		UINT sz = DragQueryFileW((HDROP)wParam,0,drop,MAX_PATH);

		if(sz&&sz<MAX_PATH)	PalEditFileImport(PalEdit,drop);		

		DragFinish((HDROP)wParam); 
		
		return 0;
	}

	//hack: fast switching
	static int tabhittest = -1;

	if(PalEdit&&hWndDlg!=PalEdit) 
	{
		if(hWndDlg==tabs) switch(Msg)
		{
		case WM_CONTEXTMENU:
		{
			TCHITTESTINFO ht = 
			{
				{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)}

			}; ScreenToClient(tabs,&ht.pt);
			tabhittest = TabCtrl_HitTest(tabs,&ht); ClientToScreen(tabs,&ht.pt);								
			BOOL cmd = TrackPopupMenu(GetSubMenu(tabsmenu,0),TPM_RETURNCMD,ht.pt.x,ht.pt.y,0,tabs,0);
			SendMessage(PalEdit,WM_COMMAND,cmd,(LPARAM)tabs); //tabhittest
			tabhittest = -1;

		}break;
		case WM_KEYUP:
		case WM_KEYDOWN:
		case WM_COMMAND: SendMessage(PalEdit,Msg,wParam,lParam);
		}
		if(hWndDlg==texture) switch(Msg)
		{
		case WM_SETTEXT: if(tabhittest!=-1) break; //hack
		{
			TCITEMW item = { TCIF_TEXT };

			wchar_t tabtitle[35] = {0}; item.cchTextMax = 34;

			//_snwprintf_s(tabtitle,35,34,L"%d:%s",tab+1,(wchar_t*)lParam);
			wcsncpy(tabtitle,(wchar_t*)lParam,34);

			item.pszText = tabtitle;			
			SendMessage(tabs,TCM_SETITEMW,tab,(LPARAM)&item);

		}break;
		}

		if(hWndDlg==tabs&&tabctrlproc)
		{
			return CallWindowProc(tabctrlproc,hWndDlg,Msg,wParam,lParam);		
		}
		if(hWndDlg==texture&&textureproc)
		{
			return CallWindowProc(textureproc,hWndDlg,Msg,wParam,lParam);		
		}

		return 0;
	}

	switch(Msg)
	{
	case WM_INITDIALOG:
	{		
		//Reminder: even if UAC is not a problem, we have the 
		//drag&drop message which seems dumb if it doesn't work!

		//Required for WM_DROPFILES (above) to work in 7 with UAC
		BOOL (WINAPI *ChangeWindowMessageFilter)(UINT,DWORD);		

		if(*(void**)&ChangeWindowMessageFilter=
		GetProcAddress(GetModuleHandle("user32.dll"),"ChangeWindowMessageFilter"))
		{
		ChangeWindowMessageFilter(WM_DROPFILES,1); //MSGFLT_ADD
		ChangeWindowMessageFilter(WM_COPYDATA,1);
		ChangeWindowMessageFilter(0x0049,1); 
		}

		PalEdit = hWndDlg;

		SetProp(hWndDlg,"Sompaste_caption",(HANDLE)L"PalEdit");

		Sword_of_Moonlight_Client_Library_Window(hWndDlg);

		menu = GetMenu(hWndDlg);

		//Apparently:
		//If you want min/max buttons you are stuck with an icon...
		//Removing the icon just sticks you with a fugly one (hack)			    
		DefWindowProc(hWndDlg,WM_SETICON,ICON_SMALL,LPARAM(icon));
		DefWindowProc(hWndDlg,WM_SETICON,ICON_BIG,LPARAM(icon));
		
		RECT client = {0,0,0,0};
		//GetClientRect returning garbage???
		while(!GetClientRect(hWndDlg,&client)
		||client.left||client.top||!client.right||!client.bottom)
		{
			static int Zzz = 0; Sleep(100); if(Zzz++>10) break;
		}
						
		TCITEM test = { TCIF_TEXT };

		char begin[] = "Drag && Drop or File->Import to begin...";
		
		test.pszText = begin; test.cchTextMax = sizeof(begin);

		tabs = GetDlgItem(hWndDlg,IDC_TABS);
		tabctrlproc = (WNDPROC) //For WM_DROPFILES (up above)
		SetWindowLongPtrW(tabs,GWL_WNDPROC,(LONG_PTR)PalEditMainDialog);		
		TabCtrl_SetPadding(tabs,6,5);
		TabCtrl_InsertItem(tabs,0,&test); 

		//8: maximum number of tabs
		//TODO: get dimensions from tabs if possible
		HIMAGELIST il = ImageList_Create(16,16,ILC_COLOR32,0,8); 
		TabCtrl_SetImageList(tabs,il);

		//texture = Sompaste->create(hWndDlg);  
		//textureproc = (WNDPROC) //For WM_DROPFILES (up above)
		//SetWindowLongPtrW(texture,GWL_WNDPROC,(LONG_PTR)PalEditMainDialog);
		//DragAcceptFiles(texture,TRUE); 
		//EnableThemeDialogTexture is required (it seems)
		texture = CreateDialog(0,MAKEINTRESOURCE(IDD_TEXTURE),PalEdit,PalEditMainDialog);
		//EnableThemeDialogTexture(texture,ETDT_ENABLETAB);						

		SetWindowPos(tabs,texture,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE);

		PalEditRestore(hWndDlg);
			
		//TODO: hunt for hole in desktop?
		//Hiding the window seems to be impossible
		//Maybe because it has a system menu or modal???
		//Either way the dialog is setup center/hidden
		//for now, so just in case 7 is different...
		ShowWindow(hWndDlg,SW_SHOW);
		SetForegroundWindow(hWndDlg);

		UpdateWindow(tabs);
		RedrawWindow(tabs,0,0,RDW_INTERNALPAINT);

		//TODO: retrieve layout from registry
		CheckMenuRadioItem(GetSubMenu(tabsmenu,0),ID_LAYOUTS_1,ID_LAYOUTS_9,ID_LAYOUTS_7,MF_BYCOMMAND);
		CheckMenuRadioItem(menu,ID_THEME_MOONLIGHT,ID_THEME_EXPLORER,ID_THEME_MOONLIGHT+theme,MF_BYCOMMAND);

		if(lParam) //App was started up with an input
		{
			PalEditFileImport(hWndDlg,(wchar_t*)lParam);
		}

		break;
	}
	case WM_NCACTIVATE:
	case WM_ACTIVATE: 
	{
		UINT out = PalEditActive(hWndDlg,Msg); //expirimental
		
		return out;
	}
	case WM_ERASEBKGND: return 1;
	case WM_SIZE:
	{					   		
		switch(wParam)
		{
		case SIZE_RESTORED: fullscreen = 0; break;
		}

		int w = LOWORD(lParam), h = HIWORD(lParam); 

		MoveWindow(tabs,-2,-2,w+4+3,h+4,0); //3: Explorer theme

		int off = 0; //texture border/offset (left/top only)

		RECT tbot; TabCtrl_GetItemRect(tabs,0,&tbot);
		MoveWindow(texture,off,tbot.bottom+off,w-off,h-tbot.bottom-off,1);		

		break;
	}
	case WM_NCLBUTTONDOWN:

		if(wParam==HTMAXBUTTON)
		{
			fullscreen = 1; dragging = 0;
		}
		else if(sticky&&!fullscreen) dragging = 1; break; //...

	case WM_WINDOWPOSCHANGING:
	{
		if(!IsIconic(PalEdit)&&dragging++==1)
		{
			Sompaste->sticky(windows,sticky,memory); dragging++;
			
		}break; //...
	}
	case WM_WINDOWPOSCHANGED:
	{
		//seems to fire constantly???
		//static WINDOWPOS delta = *(WINDOWPOS*)lParam;

		if(sticky&&!fullscreen&&dragging>2&&dragging++) //fullscreen: paranoia 
		{						
			//WINDOWPOS cmp = *(WINDOWPOS*)lParam;
			
			//if(cmp.flags&SWP_NOMOVE||cmp.x==delta.x&&cmp.y==delta.y)
			//if(cmp.flags&SWP_NOSIZE||cmp.cx==delta.cx&&cmp.cy==delta.cy)
			{
			//	break; //false alarm folks... go home
			}
			//else delta = cmp;
						
			Sompaste->arrange(windows,sticky,memory); 			
		}
		break; //...		
	}	
	//case WM_NCLBUTTONUP: 
	//case WM_NCMOUSELEAVE:
	case WM_NCMOUSEMOVE: //ironically, best way to go

		dragging = 0; break; //guaranteed to fire???

	case WM_COMMAND:

		switch(LOWORD(wParam))
		{
		case ID_FILE_IMPORT: 
			
			PalEditFileImport(hWndDlg); break;

		case ID_FILE_FOLDER: 
		case ID_FILE_PROPERTIES: 
		case ID_FILE_REASSIGN: 
		{			
			const char *cmd = 0;

			switch(LOWORD(wParam))
			{										   
			case ID_FILE_FOLDER:     cmd = "Folder";     break;
			case ID_FILE_PROPERTIES: cmd = "Properties"; break;
			case ID_FILE_REASSIGN:   cmd = "Reassign";   break;
			}

			int cur = TabCtrl_GetCurSel(tabs);

			if(tabhittest!=-1&&tabhittest!=cur)
			{
				LockWindowUpdate(texture);
				Somplayer->capture(model);
				Somplayer->texture(texture,tabhittest);
				Somplayer->context(model,texture,cmd);
				Somplayer->texture(texture,cur);
				Somplayer->release();
				LockWindowUpdate(0);
			}
			else Somplayer->context(model,texture,cmd); 

		}break;		
		case ID_FILE_SAVE: 
			
			Somplayer->context(model,texture,"Save"); break;

		case ID_FILE_SAVE_AS: 
			
			Somplayer->context(model,texture,"Save As"); break;

		case ID_FILE_SAVE_ALL: 
		{
			size_t n = Somplayer->texture();

			for(size_t i=0;i<n;i++)
			{
				Somplayer->texture(texture,i);
				Somplayer->context(model,texture,"Save"); 
			}

			Somplayer->texture(texture,tab); break;
		}		
		case ID_FILE_EXIT: goto close;		

		case ID_EDIT_UNDO: Sompaste->undo(); break;
		case ID_EDIT_REDO: Sompaste->redo(); break;

		case ID_EDIT_CUT: SendMessage(texture,WM_COMMAND,ID_EDIT_CUT,0); break;
		case ID_EDIT_COPY: SendMessage(texture,WM_COMMAND,ID_EDIT_COPY,0); break;
		case ID_EDIT_PASTE: SendMessage(texture,WM_COMMAND,ID_EDIT_PASTE,0); break;

		case ID_VIEW_PALETTE: PalEditPaletteWindow(true); break;
		case ID_VIEW_MODEL: PalEditPictureWindow(true); break;

		case ID_THEME_MOONLIGHT: PalEditChangeTheme(0); break;
		case ID_THEME_EXPLORER: PalEditChangeTheme(1); break;

		case ID_TOOLS_PREVIEW:
		{
			HRESULT (CALLBACK *proc)(BOOL,LPCWSTR) = 0;
			HMODULE dll = GetModuleHandle("Somplayer.dll");
			*(void**)&proc = GetProcAddress(dll,"DllInstall");

			if(proc) proc(TRUE,0); break; //open dialog
		}
		case ID_WINDOW_PALETTE: 
			
			if(!Palette) PalEditPaletteWindow(false); 
			
			BringWindowToTop(Palette); break;

		case ID_WINDOW_MODEL:
			
			if(!Picture) PalEditPictureWindow(false); 
			
			BringWindowToTop(Picture); break;

		case ID_WINDOW_STICKY: PalEditSticky(); break;

		case ID_WINDOW_CLIPBOARD: PalEditClipboard(); break;

		case ID_WINDOW_CASCADE: Sompaste->cascade(windows);	break;

		case ID_HELP_ABOUT:
		 
			//TODO: global about dialog??
			//DialogBox(hinstance,MAKEINTRESOURCE(IDD_ABOUT),hWndDlg,AboutDialog);

			break;

 		case ID_LAYOUTS_NEXT: case ID_LAYOUTS_PREV: 
		{
 			UINT ret; if(ret=SendMessage(texture,WM_KEYDOWN,VK_SPACE,0))
			CheckMenuRadioItem(GetSubMenu(tabsmenu,0),ID_LAYOUTS_1,ID_LAYOUTS_9,ID_LAYOUTS_1-1+ret,MF_BYCOMMAND);
			return 1;
		}				
		case ID_LAYOUTS_7: case ID_LAYOUTS_8: case ID_LAYOUTS_9:
		case ID_LAYOUTS_4: case ID_LAYOUTS_5: case ID_LAYOUTS_6:
		case ID_LAYOUTS_1: case ID_LAYOUTS_2: case ID_LAYOUTS_3:
		{
			UINT ret; if(ret=SendMessage(texture,WM_KEYDOWN,VK_NUMPAD1+LOWORD(wParam)-ID_LAYOUTS_1,0))
			CheckMenuRadioItem(GetSubMenu(tabsmenu,0),ID_LAYOUTS_1,ID_LAYOUTS_9,ID_LAYOUTS_1-1+ret,MF_BYCOMMAND);
			return 1;
		}
		}break;

	case WM_NOTIFY:
	{
		NMHDR *n = (LPNMHDR)lParam;
		
		if(n->code==TCN_SELCHANGING||n->code==TCN_SELCHANGE)
		{
			PalEditShowTab(TabCtrl_GetCurSel(tabs));
		}	  
		break;
	}
	case WM_CLOSE: 

close:	DestroyWindow(Palette);
		DestroyWindow(Picture);
		DestroyWindow(PalEdit);

		PostQuitMessage(0);

		PalEdit = 0; 
		
		break;
	}

	return 0;
}

int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR cli, int)
{	
	CoInitialize(0);

	//assert(0); //debugging
				
	int argc = 0; wchar_t **argv = //...
		
	CommandLineToArgvW(GetCommandLineW(),&argc);
		
	icon = ExtractIconW(0,argv[0],0); //hack

	setlocale(LC_ALL,"Japanese_Japan.932");

	LPARAM drop = argc>1?(LPARAM)argv[1]:0;

	wchar_t v[4] = {SOMPLAYER_VERSION};
	Somplayer = Sword_of_Moonlight_Media_Library_Connect(v);			

	//Assuming W may produce a more Unicode friendly experience
	CreateDialogParamW(0,MAKEINTRESOURCEW(IDD_MAIN),0,PalEditMainDialog,drop); 

	PalEditChangeTheme(theme); //hack: became necessary at some point???

	HACCEL acc = LoadAcceleratorsW(0,MAKEINTRESOURCEW(IDR_ACCELERATOR));

	char debug[33];	
	MSG msg; while(GetMessageW(&msg,0,0,0)) //ignoring -1
	{ 			
	//Warning: find PostMessage in here is too volatile

#ifdef NDEBUG
		try //// wouldn't want anyone to lose any work ////
		{
#endif	
#ifdef _DEBUG
		//sprintf(debug,"%d: %x\n",msg.message,msg.hwnd); 		
		//OutputDebugString(debug);
#endif
		if(Sompaste->wallpaper(0,&msg)) continue;

		bool noacc = false;

		HWND fore = GetForegroundWindow();

		switch(msg.message)
		{
		case WM_KEYDOWN: case WM_KEYUP: 	

		case WM_SYSKEYDOWN: case WM_SYSKEYUP: //VK_F10
						
			if(msg.wParam=='Z') //Important: Ctrl+Shift+Z redo
			{
				if(GetKeyState(VK_SHIFT)&0x8000&&GetKeyState(VK_CONTROL)&0x8000)
				{
					switch(msg.message) //Assuming edit controls
					{
					case WM_KEYDOWN: SendMessage(msg.hwnd,WM_USER+84,0,0); //EM_REDO

					case WM_KEYUP: continue; //swallowing regardless
					}
				}
			}

			LRESULT bt = SendMessage(GetFocus(),WM_GETDLGCODE,0,0);
 
			//Want to detect edit control has ES_WANTRETURN but can't?? 
			//http://blogs.msdn.com/b/oldnewthing/archive/2007/08/20/4470527.aspx
			//if(SendMessage(msg.hwnd,WM_GETDLGCODE,0,0)&DLGC_WANTALLKEYS) break;
			if(noacc=bt&DLGC_WANTALLKEYS) break;
			
			//Emulate tab key (cause it's not working)
			if(msg.wParam==VK_TAB) switch(msg.message)
			{					
			case WM_KEYDOWN: if(fore==clipboard) break; //hack
			{	
				//Note: For now we assert that no controls should have tab 
				//if(SendMessage(msg.hwnd,WM_GETDLGCODE,0,0)&DLGC_WANTTAB) break;

				//This is needed to consistently generate a focus rectangle
				//NOTE: For no apparent reason UISF_HIDEACCEL is also required under Windows 7 ???
				SendMessage(fore,WM_UPDATEUISTATE,MAKEWPARAM(UISF_HIDEACCEL|UISF_HIDEFOCUS,UIS_SET),0); 			

				try{ /*try: seems like bad code on MS's end; maybe not*/
				//went to great lengths to emulate 7 but just needed this :/
				//http://blogs.msdn.com/b/oldnewthing/archive/2004/08/02/205624.aspx			
				SendMessage(fore,WM_NEXTDLGCTL,GetKeyState(VK_SHIFT)&0x8000?1:0,0); 			
				}catch(...){ /*access violation when a window is destroyed*/ }
			}
			case WM_KEYUP: if(fore==clipboard) break; //hack
				
				continue;
			}				
			//Emulate enter key (cause it's not working)
			if(msg.wParam==VK_RETURN) switch(msg.message)
			{
			case WM_KEYDOWN: case WM_KEYUP: 	
			{	
				const int buttons = 
				DLGC_BUTTON|DLGC_DEFPUSHBUTTON|DLGC_UNDEFPUSHBUTTON;

				//TODO: BN_CLICKED (assuming Enter counts as a click!)

				HWND push = 0; int cmd = 0;

				if(bt&buttons)  
				{
					cmd = GetDlgCtrlID(push=msg.hwnd); 
				}
				else //Note: GETDEFID returning the original button
				{
					LRESULT def = SendMessage(fore,DM_GETDEFID,0,0);

					if(def) //have default push button
					{
						cmd = LOWORD(def); push = GetDlgItem(fore,cmd);
					}
					else break;
				}

				//Note: opening dialog via menu would kill it
				//Still will if you hold the key down for long
				//But Explorer is the same way (worse actually)
				LRESULT st = SendMessage(push,BM_GETSTATE,0,0);		

				//want to post before command destroys the window
				SendMessage(push,BM_SETSTATE,msg.message==WM_KEYDOWN,0);		
							
				if(msg.message==WM_KEYUP&&st&BST_PUSHED)
				SendMessage(GetParent(push),WM_COMMAND,cmd,(LPARAM)push);
				
			}continue; //mission accomplished
			}		  
		}

		if(fore==PalEdit||fore==Palette||fore==Picture)
		{
			//VK_F10: make available for keyboard players

			if(msg.wParam==VK_MENU
			 ||msg.wParam==VK_F10&&!noacc) switch(msg.message)
			{
			case WM_SYSKEYDOWN: case WM_SYSKEYUP:
				
				//So alt to access menu gets forwarded to PalEdit window
				if(fore!=PalEdit) SetForegroundWindow(fore=msg.hwnd=PalEdit); 			
			}

			if(!noacc)
			//Reminder: layout hotkeys take VK_SPACE from buttons
			if(TranslateAcceleratorW(PalEdit,acc,&msg)) continue;
		}

		//WINSANITY...
		//Note: passing msg.hwnd to isDialogMessage
		//would cause some problems, however unclear
		//it may be as to whether this is technically 
		//a correct fix or not. The only sure fire way
		//would seem to be having Sompaste.dll managing
		//all of the dialog windows :/

		if(fore==msg.hwnd&&IsDialogMessageW(fore,&msg)) continue;
		//if(IsDialogMessageW(msg.hwnd,&msg)) continue;

		TranslateMessage(&msg); DispatchMessageW(&msg); 

#ifdef NDEBUG
		//TODO: save a backup of any outstanding files
		}catch(...){ /*don't want to lose our work*/ }
#endif
	} 
		
	Sword_of_Moonlight_Media_Library_Disconnect(Somplayer);

#ifdef _DEBUG

		Sleep(2000); //want to see Somplayer.dll unload
#endif

	CoUninitialize();

	LocalFree(argv); 	

_0:	return status;

_1:	status = 1;

	goto _0;
}
 