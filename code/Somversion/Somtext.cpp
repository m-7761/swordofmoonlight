
#include "Somversion.pch.h"

#include "../Sompaste/Sompaste.h"
static SOMPASTE Sompaste = 0;
static HMODULE Sompaste_dll = 0;

#include "../lib/swordofmoonlight.h"	  

namespace res = SWORDOFMOONLIGHT::res;
static res::mapper_t Somtext_sekai_res = 0;
static res::mapper_t Somtext_group_res = 0;

static DWORD Somtext_threadid = 0;
static bool Somtext_texdirisrel = true;

static wchar_t Somtext_textdir[MAX_PATH] = L"";
static wchar_t Somtext_instdir[MAX_PATH] = L"";
static wchar_t Somtext_opendir[MAX_PATH] = L"";

//the gist of this is taken from
//http://www.codeproject.com/Articles/28015/
//author: Hojjat Bohlooli - software@tarhafarinin.ir
//SelectDialog - A Multiple File and Folder Select Dialog
//TODO: find a place for this dialog in Sompaste's list of APIs
//ALSO: http://www.codeproject.com/Articles/20853/Win-Tips-and-Tricks
static LPOPENFILENAMEW Somtext_Open = 0;
static WNDPROC Somtext_OpenViewProc = 0;
static WNDPROC Somtext_OpenRootProc = 0;
static LRESULT CALLBACK Somtext_OpenView(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_NOTIFY:
	{
		//NOTE: this is the only reason to be
		//subclassing SHELLDLL_DefView for now
		NMLVKEYDOWN *p = (NMLVKEYDOWN*)lParam;

		if(p->hdr.code==LVN_KEYDOWN)
		{
			if(p->wVKey==VK_RETURN)
			{
				bool zip = //assuming zip/folder
				Somtext_Open->nFilterIndex==1;
				if(zip) break;
				
				HWND root = GetParent(hWnd);
				wchar_t ms[2] = L""; //multiselect?
				if(GetDlgItemTextW(root,1148,ms,2)&&*ms=='"') 
				{	
					//two folders are selected, so assume not navigating into both
					SendMessage(root,WM_COMMAND,IDOK,(LPARAM)GetDlgItem(root,IDOK));
					return 1;
				}
			}
			break;
		}
		break;
	}
	case WM_DESTROY: //ListView is repeatedly destroyed?!

		WNDPROC onelasttime = (WNDPROC)		
		SetWindowLongPtrW(hWnd,GWL_WNDPROC,(LONG_PTR)Somtext_OpenViewProc);
		Somtext_OpenViewProc = 0;
		//trigger WM_SHOWWINDOW in Somtext_OpenRoot below
		PostMessage(GetAncestor(hWnd,GA_ROOT),WM_SHOWWINDOW,1,0);
		return CallWindowProcW(onelasttime,hWnd,uMsg,wParam,lParam);
	}

	return CallWindowProcW(Somtext_OpenViewProc,hWnd,uMsg,wParam,lParam);
}
static LRESULT CALLBACK Somtext_OpenRoot(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{	
	switch(uMsg)
	{
	case WM_SHOWWINDOW:

		if(!Somtext_OpenViewProc)
		{				
			HWND SHELLDLL_DefView = GetDlgItem(hWnd,1121);
			if(SHELLDLL_DefView)
			Somtext_OpenViewProc = (WNDPROC)
			SetWindowLongPtrW(SHELLDLL_DefView,GWL_WNDPROC,(LONG_PTR)Somtext_OpenView);				
			else assert(0);	 			
		}
		break;
	
	case WM_COMMAND:

		if(LOWORD(wParam)==IDOK)
		{
			//scheduled obsolete?
			//the ZIP option can almost flawless select
			//folders as well, all-in-one, except when you
			//choose a folder and then add a file to the list
			//CDM_GETFILEPATH reports only the folder
			//(after choosing a second file everything works)
			bool zip = Somtext_Open->nFilterIndex==1;

			if(zip||GetFocus()!=(HWND)lParam
			  &&!Somtext_Open->nFileOffset)
			{
				wchar_t ms[2] = L""; //multiselect?
				GetDlgItemTextW(hWnd,1148,ms,2);			
				//assuming navigating to a folder via the combobox
				if(*ms&&*ms!='"') break;
			}

			//0: finishing up
			OPENFILENAMEW *ofn = Somtext_Open;
			Somtext_Open = 0;

			if(!ofn->nFileOffset)
			{				
				wchar_t *p = ofn->lpstrFile;
				wcscpy(p,Somtext_opendir);
				size_t cat = ofn->nFileOffset = 1+wcslen(p);				
				if(!cat) break; //paranoia
				p+=cat; GetDlgItemTextW(hWnd,1148,p,ofn->nMaxFile-cat);
				wchar_t *pp = p, *q;

				int out = 1; //scheduled obsolete?

				//the pattern is "title" "title" "
				//fortunately extensions are included here
				for(q=p;*p;p++) if(p[0]=='"')
				{
					if(p[1]!=' '&&p[1]) continue; 
					
					p+=2; *q++ = '\0'; out++; 
				}
				else *q++ = *p; *q = '\0';

				if(out==1) ofn->lpstrFile[cat-1] = '\\';

				if(!*pp) 
				{
					pp[1] = '\0'; //necessary?

					pp = ofn->lpstrFile; //then must be absolute...
				}  
				//hack: repair absolute path
				if(out<2&&!PathIsRelativeW(pp))
				{
					wcscpy(ofn->lpstrFile,pp);

					ofn->nFileOffset = 
					PathFindFileNameW(ofn->lpstrFile)-ofn->lpstrFile;
				}
				if(out<2)
				{
					ofn->nFileExtension = 
					PathFindExtensionW(ofn->lpstrFile)-ofn->lpstrFile;
				}
				else ofn->nFileExtension = 0; //multiselect
			}

			EndDialog(hWnd,1);
			return 0;
		}
		break;
	}

	return CallWindowProcW(Somtext_OpenRootProc,hWnd,uMsg,wParam,lParam);
}
static UINT_PTR CALLBACK Somtext_OpenHook(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uiMsg)
	{
	case WM_INITDIALOG:

		//XP: loses directory
		//*Somtext_opendir = '\0';
		Somtext_Open = (OPENFILENAMEW*)lParam; 		
		break;

	case WM_NOTIFY:
	{
		OFNOTIFYW *p = (OFNOTIFYW*)lParam;

		switch(p->hdr.code)
		{
		case CDN_INITDONE:
		{				
			Somtext_Open = p->lpOFN;			
			Somtext_OpenRootProc = (WNDPROC)			
			SetWindowLongPtrW(p->hdr.hwndFrom,GWL_WNDPROC,(LONG_PTR)Somtext_OpenRoot);
			//assert(!Somtext_OpenViewProc);
			//Somtext_OpenViewProc = 0;			
			break;
		}
		case CDN_FOLDERCHANGE:
		{
			SendMessageW(p->hdr.hwndFrom,CDM_GETFOLDERPATH,MAX_PATH,(LPARAM)Somtext_opendir);
			break;
		}
		case CDN_SELCHANGE:
		{	 			
			//scheduled obsolete?
			//the ZIP option can almost flawless select
			//folders as well, all-in-one, except when you
			//choose a folder and then add a file to the list
			//CDM_GETFILEPATH reports only the folder
			//(after choosing a second file everything works)
			bool zip = Somtext_Open->nFilterIndex==1;
			if(zip) break;

			HWND SHELLDLL_DefView = 
			GetDlgItem(p->hdr.hwndFrom,1121);
			HWND lv = GetDlgItem(SHELLDLL_DefView,1);			
			//add directories to the list of selections
			int sel = ListView_GetNextItem(lv,-1,LVNI_SELECTED);			
			if(sel!=-1) //&&-1!=ListView_GetNextItem(lv,sel,LVNI_SELECTED))
			{
				Somtext_Open->nFileExtension = 0; //multiselect

				size_t cat = 1+wcslen(Somtext_opendir); assert(cat<MAX_PATH);

				//*//// POINT OF NO RETURN ////*/
				Somtext_opendir[cat-1] = '\\';

				HWND em = GetDlgItem(p->hdr.hwndFrom,1148); //file list
				//COMBOBOXINFO cbi = {sizeof(cbi)}; 
				//if(!GetComboBoxInfo(em,&cbi)) assert(0);
				//em = cbi.hwndItem; //the actual edit component
				//em = GetDlgItem(em,1148); //not a true edit combobox???
				const size_t buffer_s = MAX_PATH*16;
				wchar_t buffer[buffer_s+1] = L"", *begin = buffer+cat; 
				if(cat>=SendMessageW(p->hdr.hwndFrom,CDM_GETFILEPATH,buffer_s,(LPARAM)buffer))
				{	
					begin = wcscpy(buffer,L" \"")+1; //paranoia?
				}
				else if(*begin!='"') *wcscat(--begin,L"\"") = '"';
								
				int _ = wcslen(buffer); wchar_t app[MAX_PATH+3] = L" \"";			

				begin[-1] = ' '; assert(begin>buffer&&!wcsncmp(begin-1,app,2));
				
				LVITEMW lvi = {LVIF_TEXT,0,0,0,0,Somtext_opendir+cat,MAX_PATH-cat};			  

				bool directory = false;	do 
				{		
					size_t len = SendMessageW(lv,LVM_GETITEMTEXTW,sel,(LPARAM)&lvi);
					int fa = GetFileAttributesW(Somtext_opendir);
					if(fa!=-1&&fa&FILE_ATTRIBUTE_DIRECTORY)
					{
						wcscpy(wcscpy(app+2,Somtext_opendir+cat)+len,L"\"");
						//SendMessageW(em,EM_SETSEL,_,_);
						//SendMessageW(em,EM_REPLACESEL,0,(LPARAM)app);
						//_ = GetWindowTextLengthW(em); assert(_);						
						if(!wcsstr(begin-1,app)) //match agains L" \""
						{
							wcscpy_s(buffer+_,buffer_s-_,app); 
							_+=wcslen(app);

							if(!directory) //first selection was directory?
							{
							}
						}

						directory = true;
					}					

					sel = ListView_GetNextItem(lv,sel,LVNI_SELECTED);
				}
				while(sel!=-1); Somtext_opendir[cat-1] = '\0'; //!				
				
				if(*begin&&begin[1])
				if(wcschr(begin+1,'"')==buffer+_-1) //single?
				if(0||directory) //1: showing .ZIP 
				{
					begin++; buffer[_-1] = '\0'; //just strip the quotation marks
				}
				else //remove the file extension if that is the system preference				
				SendMessageW(p->hdr.hwndFrom,CDM_GETSPEC,MAX_PATH,(LPARAM)begin);
				SetWindowTextW(em,begin);
			}					
			break;
		}}
		break;
	}}
	return FALSE;
}
enum{ Somtext_returnfocus=WM_USER };
static int Somtext_returnfocusid = 0;
static HWND Somtext_accelerator_root = 0;
static HHOOK Somtext_accelerator_next = 0;
//http://stackoverflow.com/questions/3547945/how-to-use-an-accelerator-in-a-modal..
static LRESULT CALLBACK Somtext_accelerator(int code, WPARAM wParam, LPARAM lParam)
{
	int out = 1;

	if(code==HC_ACTION&&wParam==PM_REMOVE)
	{
		MSG *p = (MSG*)lParam;

		if(p->message==WM_LBUTTONDOWN) //hack: restore UI
		if(UISF_HIDEFOCUS&~SendMessage(Somtext_accelerator_root,WM_QUERYUISTATE,0,0))
		{
			Somtext_returnfocusid = 0;
			SendMessage(Somtext_accelerator_root,WM_UPDATEUISTATE,MAKEWPARAM(UISF_HIDEFOCUS,UIS_SET),0);
			InvalidateRect(GetFocus(),0,0);
		}

		if(p->message>=WM_KEYFIRST&&p->message<=WM_KEYLAST)		
		if(Somtext_accelerator_root==GetForegroundWindow())
		{
			if(p->message==WM_KEYDOWN) switch(p->wParam)
			{					
			case VK_TAB: //hack: keyboard UI

				Somtext_returnfocusid = 0;
				SendMessage(Somtext_accelerator_root,WM_UPDATEUISTATE,MAKEWPARAM(UISF_HIDEFOCUS,UIS_CLEAR),0);
				break;

			case VK_ESCAPE: case VK_RETURN: //prevent repeat!!

				if(p->lParam&1<<30) return p->message = 0; //WM_NULL;
				break;
			}

			HWND gf = GetFocus(); //ignore if edit-able control
			if(DLGC_HASSETSEL&~SendMessage(gf,WM_GETDLGCODE,0,0)
			  ||ES_READONLY&GetWindowStyle(gf)) 
			{
				static HACCEL ha = //leaky
				LoadAcceleratorsW(Somversion_dll(),MAKEINTRESOURCEW(IDR_SEKAI_ACCEL));
				if(TranslateAcceleratorW(Somtext_accelerator_root,ha,p))
				{
					out = 0; p->message = 0; //WM_NULL;
				}
			}
		}
	}

	if(out||code<0) //code<0: paranoia
	return CallNextHookEx(Somtext_accelerator_next,code,wParam,lParam);
	return out;
}
static void Somtext_add(HWND bt);
static bool Somtext_adding = false;
static void Somtext_edit(HWND lv);
static void Somtext_xfer(int,HWND lv);
static int  Somtext_xferring = 0;
static void Somtext_subtract(HWND lv);
static void Somtext_setfocus(int id, HWND cb);
static void Somtext_selchange(int id, HWND cb, bool=0);
static bool Somtext_selchanging = false;
static void Somtext_download(HWND dl);
static wchar_t Somtext_downloading[MAX_PATH] = L"";
static void Somtext_f1(HWND);
static void Somtext_f5(HWND,const wchar_t*sel=0);
static INT_PTR CALLBACK Somtext_DialogProc(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg)
	{
	case WM_INITDIALOG:
	{			
		SetWindowTheme(GetDlgItem(hWndDlg,IDC_SEKAI_INFO),L"",L""); //scrollbar		

		Somtext_threadid = GetCurrentThreadId();

		Sompaste_dll = LoadLibrary("Sompaste.dll");

		Somtext_accelerator_next = 
		//todo: Sompaste API like Sompaste->accel(hWndDlg,LoadAccelerator());
		SetWindowsHookExW(WH_GETMESSAGE,Somtext_accelerator,Somversion_dll(),GetCurrentThreadId());
		Somtext_accelerator_root = hWndDlg;

		HICON icon = (HICON)
		SendMessage(GetParent(hWndDlg),WM_GETICON,ICON_BIG,0);		
		SendMessage(hWndDlg,WM_SETICON,ICON_BIG,(LPARAM)icon);		

		wchar_t sel[100] = L"";
		*Somtext_textdir = '\0'; DWORD max_path = sizeof(Somtext_textdir);
		max_path = sizeof(Somtext_textdir);		
		SHGetValueW(HKEY_CURRENT_USER,
		L"SOFTWARE\\FROMSOFTWARE\\SOM\\INSTALL",L"TextDir",0,Somtext_textdir,&max_path);
		if(*Somtext_textdir)
		{
			if(Somtext_texdirisrel=PathIsRelativeW(Somtext_textdir))
			{
				wchar_t swap[MAX_PATH] = L"";
				//MUST ENSURE THAT InstDir IS A SUBSTRING OF TextDir
				//PathCombineW(Somtext_textdir,Somtext_instdir,wcscpy(swap,Somtext_textdir));
				swprintf_s(Somtext_textdir,L"%ls\\%ls",Somtext_instdir,wcscpy(swap,Somtext_textdir));		
			}

			wchar_t *fn = PathFindFileNameW(Somtext_textdir);

			if(*fn&&fn!=Somtext_textdir) 
			{
				wcscpy_s(sel,fn); 
				wcscpy_s(fn,MAX_PATH-(fn-Somtext_textdir),L"sekai.res");
				if(PathFileExistsW(Somtext_textdir)) 
				{
					fn[-1] = '\0'; //truncate /sekai.res
				}
				else *Somtext_textdir = *sel = '\0';
			}
			else *Somtext_textdir = '\0';
		}
		if(!*Somtext_textdir)
		{
			Somtext_texdirisrel = true;
			swprintf_s(Somtext_textdir,L"%ls\\text",Somtext_instdir);		
		}

		HWND lv = GetDlgItem(hWndDlg,IDC_SEKAI_TEXT);	
		ListView_SetExtendedListViewStyle(lv,LVS_EX_INFOTIP|LVS_EX_FULLROWSELECT); 
		HWND em = ListView_GetEditControl(lv);
		SendMessageW(lv,EM_SETLIMITTEXT,MAX_PATH-1,0);
		RECT cr; GetClientRect(lv,&cr);
		LVCOLUMNA lvc = {LVCF_SUBITEM|LVCF_WIDTH,0,cr.right,0,0}; 
		ListView_InsertColumn(lv,0,&lvc);
		lvc.cx = 0; lvc.iSubItem = 1;
		ListView_InsertColumn(lv,1,&lvc);
				
		wchar_t text[MAX_PATH] = L""; max_path = sizeof(text);		
		LONG err = SHGetValueW(HKEY_CURRENT_USER,
		L"SOFTWARE\\FROMSOFTWARE\\SOM\\TEXT",Somtext_instdir,0,text,&max_path);
		if(err) err = SHGetValueW(HKEY_CURRENT_USER, //try (Default)
		L"SOFTWARE\\FROMSOFTWARE\\SOM\\TEXT",L"",0,text,&(max_path=sizeof(text)));
		//TODO: replace with Sompaste->xfer operation
		if(!err) for(wchar_t *p=text,dst[MAX_PATH+1];*p;)
		{				
			while(*p&&*p==' ') p++;
			wchar_t *q = dst, *d = dst+MAX_PATH;			
			while(*p&&*p!='\r'&&*p!='\n'&&*p!=';') 
			if(q<d) *q++ = *p++; else p++; *q = '\0';
			while(*p&&(*p=='\r'||*p=='\n'||*p==';'||*p==' ')) p++;
			if(!*dst) continue;
			int i = ListView_GetItemCount(lv);
			const wchar_t *label = PathFindFileNameW(dst);	
			LVITEMW lvi = {LVIF_TEXT,i,0,0,0,(LPWSTR)label};
			SendMessageW(lv,LVM_INSERTITEMW,i,(LPARAM)&lvi);
			lvi.iSubItem = 1; lvi.pszText = Sompaste->path(dst);
			SendMessageW(lv,LVM_SETITEMTEXTW,i,(LPARAM)&lvi);
		}

		Somtext_f5(hWndDlg,sel); //load top combobox

		//MOVING TO WM_SHOWWINDOW BELOW
		//ShowWindow(GetParent(hWndDlg),SW_HIDE);
		return 1;
	}	
	case WM_SHOWWINDOW:
				
		//MOVED FROM WM_INITDIALOG ABOVE
		if(wParam&&!IsWindowVisible(hWndDlg));
		ShowWindow(GetParent(hWndDlg),SW_HIDE);
		break;

	case /*WM_USER*/Somtext_returnfocus: 
	{		
		int fid = GetDlgCtrlID(GetFocus());

		if(fid==Somtext_returnfocusid) switch(fid)
		{
		case IDC_SEKAI_GET: SetFocus(GetDlgItem(hWndDlg,IDC_SEKAI_TOP)); break;
		case IDC_SEKAI_ADD: SetFocus(GetDlgItem(hWndDlg,IDC_SEKAI_ZIP)); break;
		}

		Somtext_returnfocusid = 0;
		break;
	}
	case WM_COMMAND:

		switch(HIWORD(wParam))
		{			
		case EN_SETFOCUS: 			

			if(ES_READONLY&GetWindowStyle((HWND)lParam))
			DestroyCaret(); 
			break;

		case EN_KILLFOCUS:
				
			SendMessage((HWND)lParam,EM_SETSEL,-1,0);	
			break;

		//case CBN_SELCHANGE: //1 (moved below)
		//	
		//	Somtext_selchange(LOWORD(wParam),(HWND)lParam); 
		//	break;		
		}
		switch(LOWORD(wParam))
		{
		case IDC_SEKAI_TOP:
		case IDC_SEKAI_ZIP:

			switch(HIWORD(wParam))
			{			
			//case CBN_SELCHANGE: //(moved from above)
			case CBN_SELENDOK: 
			
				//schedule obsolete
				//todo: open/close with spacebar/Sompaste.dll
				Somtext_selchange(LOWORD(wParam),(HWND)lParam); 
				break;	

			case CBN_CLOSEUP: //CBN_SETFOCUS:

				Somtext_setfocus(LOWORD(wParam),(HWND)lParam); 
				break;	

			case CBN_SETFOCUS: 
			
				switch(LOWORD(wParam))
				{
				case IDC_SEKAI_TOP: 
					
					SendMessage(hWndDlg,DM_SETDEFID,IDC_SEKAI_GET,0); 
					break;

				case IDC_SEKAI_ZIP: 
					
					SendMessage(hWndDlg,DM_SETDEFID,IDC_SEKAI_ADD,0); 
					break;
				}
				break;
			
			case CBN_KILLFOCUS:

				if(*Somtext_downloading)
				{
					Somtext_returnfocusid = 
					0xFFFF&SendMessage(hWndDlg,DM_GETDEFID,0,0);

					if(Somtext_returnfocusid) 
					SetFocus(GetDlgItem(hWndDlg,Somtext_returnfocusid));
				}
				else SendMessage(hWndDlg,DM_SETDEFID,0,0);
				break;
			}
			break;
						
		case ID_SEKAI_GET: //Ctrl+Down
		
			//disabling ? popup because early
			//timeouts are to common with WinInet
			//and it makes the situation seem hopelss
			//(with SysMenu [?] it's unnecessary anyway)
			if(!*Somtext_downloading
			  ||'?'==*Somtext_downloading)
			Somtext_download(GetDlgItem(hWndDlg,IDC_SEKAI_GET)); 		
			//else if('?'==*Somtext_downloading)
			//Somtext_f1(hWndDlg);
			else MessageBeep(-1);
			break;

		case IDC_SEKAI_GET: //download button

			if(*Somtext_downloading) //button has X 
			{
				//disabling ? popup because early
				//timeouts are to common with WinInet
				//and it makes the situation seem hopelss
				//(with SysMenu [?] it's unnecessary anyway)
				//if('?'==*Somtext_downloading) Somtext_f1(hWndDlg);

				*Somtext_downloading = '\0'; //cancel
			}
			else Somtext_download(GetDlgItem(hWndDlg,IDC_SEKAI_GET)); 		
			break;

		case ID_SEKAI_CTRL_X: 

			if(*Somtext_downloading)
			*Somtext_downloading = '\0'; //cancel
			else Somtext_xfer('X',GetDlgItem(hWndDlg,IDC_SEKAI_TEXT));
			break;
						
		case ID_SEKAI_ADD:
		case IDC_SEKAI_ADD:
	
			if(!*Somtext_downloading)
			Somtext_add(GetDlgItem(hWndDlg,IDC_SEKAI_ADD)); 			
			else MessageBeep(-1);
			break;

		case ID_SEKAI_SUBTRACT:
		case IDC_SEKAI_SUBTRACT:

			Somtext_subtract(GetDlgItem(hWndDlg,IDC_SEKAI_TEXT)); 
			break;

		case IDC_SEKAI_OPEN_ZIP: case ID_SEKAI_CTRL_Z:		
		case IDC_SEKAI_OPEN_DIR: case ID_SEKAI_BACKSLASH:
		{
			//0/1: Zip/folder
			LPARAM filter = 1;
			switch(LOWORD(wParam))
			{
			case ID_SEKAI_CTRL_Z:
			case IDC_SEKAI_OPEN_DIR: filter = 2; break;
			}

			//go to lengths to globalize the filters
			SHFILEINFOW si, si2; *si.szTypeName = '\0';
			SHGetFileInfoW(L".zip",FILE_ATTRIBUTE_NORMAL,&si,sizeof(si),SHGFI_USEFILEATTRIBUTES|SHGFI_TYPENAME);
			SHGetFileInfoW(Somtext_textdir,FILE_ATTRIBUTE_NORMAL,&si2,sizeof(si),SHGFI_TYPENAME);
			if(!*si.szTypeName) wcscpy(si.szTypeName,L"Compressed Folder");
			if(!*si2.szTypeName) wcscpy(si.szTypeName,L"Folder");
			//qqqq... is a tip from the link below
			//(this way we can ignore the listview for now)
			//http://www.codeproject.com/Articles/20853/Win-Tips-and-Tricks
			wchar_t filters[512] = //512: swprint_f says 80 is too small???
			L"Compressed Folder (*.zip)\0*.zip\0Folder\0*qqqqqqqqqqqqqqq.qqqqqqqqq\0";
			swprintf_s(filters,
			L"%ls (*.zip)%lc*.zip%lc%ls%lc,qqqqqqqqqqqqqqq.qqqqqqqqq%lc",
			si.szTypeName,L'\0',L'\0',si2.szTypeName,L'\0',L'\0');

			const size_t sel_s = MAX_PATH*10; //OFN_ALLOWMULTISELECT
			wchar_t sel[sel_s+1] = L"";	  			
																		
			int cat = swprintf(Somtext_opendir,L"%ls\\",Somtext_textdir);
			if(cat>0&&GetDlgItemTextW(hWndDlg,IDC_SEKAI_TOP,Somtext_opendir+cat,MAX_PATH-cat))
			{
				if(!PathFileExistsW(Somtext_opendir)) Somtext_opendir[cat-1] = '\0';
			}
			else if(cat>0) Somtext_opendir[cat-1] = '\0';

			OPENFILENAMEW open =		
			{
			sizeof(open),hWndDlg,0,				
			filters, 0, 0, filter, sel+1, sel_s, 0, 0, Somtext_opendir, 0, 	
			OFN_ENABLEHOOK|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_ALLOWMULTISELECT|OFN_EXPLORER,
			0, 0, 0, 0, Somtext_OpenHook, 0, 0, 0 
			};
			if(GetOpenFileNameW(&open))
			{
				Sompaste->path(sel+1);				
				HWND lv = GetDlgItem(hWndDlg,IDC_SEKAI_TEXT);
				LVITEMW lvi = {LVIF_TEXT,0,0,0,0,sel+1+open.nFileOffset};
		
				//todo: do with a Sompaste->xfer operation
				if(open.nFileOffset&&!open.nFileExtension) //multiple
				{
					wchar_t *f = sel+1+open.nFileOffset; f[-1] = '\\';
																 					
					for(wchar_t *p=f,*q;*p;p++)
					{
						for(q=f;*p;) *q++ = *p++; *q++ = '\0';

						lvi.iSubItem = 0;
						lvi.pszText = PathFindFileNameW(f);
						SendMessageW(lv,LVM_INSERTITEMW,lvi.iItem,(LPARAM)&lvi);
						lvi.iSubItem = 1; lvi.pszText = sel+1;
						SendMessageW(lv,LVM_SETITEMTEXTW,lvi.iItem,(LPARAM)&lvi);
						lvi.iItem++;
					}
				}
				else //single
				{
					SendMessageW(lv,LVM_INSERTITEMW,0,(LPARAM)&lvi);
					lvi.iSubItem = 1; lvi.pszText = sel+1;
					SendMessageW(lv,LVM_SETITEMTEXTW,0,(LPARAM)&lvi);
				}
			}		
			break;
		}
		case ID_SEKAI_F1: 

			Somtext_f1(hWndDlg);
			break;
		
		case ID_SEKAI_F5_TRICK: 
					
			Somtext_f5(hWndDlg);
			break;

		//default button
		case IDCANCEL: goto close;
		}
		break;

	case WM_NOTIFY:
	{
		NMLVGETINFOTIPW *p = (NMLVGETINFOTIPW*)lParam;

		switch(p->hdr.code)
		{
		case NM_RETURN: //not sent??
			
			MessageBeep(-1); //ListView?
			break;

		case NM_SETFOCUS: //key nav feedback
			
			if(UISF_HIDEFOCUS&~SendMessage(hWndDlg,WM_QUERYUISTATE,0,0))
			if(-1==ListView_GetNextItem(p->hdr.hwndFrom,-1,LVNI_SELECTED))
			{
				ListView_SetItemState(p->hdr.hwndFrom,0,LVIS_FOCUSED|LVIS_SELECTED,-1);
				if(-1==ListView_GetNextItem(p->hdr.hwndFrom,-1,LVNI_SELECTED))
				Somtext_edit(p->hdr.hwndFrom);
			}
			break;

		case LVN_ENDLABELEDITW:
		case LVN_BEGINLABELEDITW:
		{				
			int out = 0;
			NMLVDISPINFOW *p = (NMLVDISPINFOW*)lParam;
			p->item.iSubItem = 1;
			if(p->hdr.code==LVN_BEGINLABELEDITW)
			{
				wchar_t path[MAX_PATH] = L""; 
				p->item.pszText = path; p->item.cchTextMax = MAX_PATH;
				SendMessageW(p->hdr.hwndFrom,LVM_GETITEMTEXTW,p->item.iItem,(LPARAM)&p->item);
				out = !SetWindowTextW(ListView_GetEditControl(p->hdr.hwndFrom),path);
			}
			else if(p->item.pszText)
			{
				if(!*p->item.pszText)
				ListView_DeleteItem(p->hdr.hwndFrom,p->item.iItem);
				wchar_t *fn = PathFindFileNameW(p->item.pszText);
				if(out=*fn)
				{
					SendMessageW(p->hdr.hwndFrom,LVM_SETITEMTEXTW,p->item.iItem,(LPARAM)&p->item);					 
					wcscpy(p->item.pszText,fn);
				}				
			}
			else //bottom row dummy?
			{
				WCHAR check[2] = L"";
				p->item.pszText = check; p->item.cchTextMax = 2;
				if(!SendMessageW(p->hdr.hwndFrom,LVM_GETITEMTEXTW,p->item.iItem,(LPARAM)&p->item))
				ListView_DeleteItem(p->hdr.hwndFrom,p->item.iItem);
			}
			p->item.iSubItem = 0;
			SetWindowLong(hWndDlg,DWL_MSGRESULT,out);
			return 1;
		}
		case LVN_GETINFOTIPW:
		{
			LVITEMW lvi = {LVIF_TEXT,p->iItem,1,0,0,p->pszText,p->cchTextMax};
			SendMessageW(p->hdr.hwndFrom,LVM_GETITEMTEXTW,p->iItem,(LPARAM)&lvi);
			break;
		}
		case LVN_BEGINDRAG:
		{
			Somtext_xfer(0,p->hdr.hwndFrom);
			break;
		} 
		case LVN_KEYDOWN: 
		{
			NMLVKEYDOWN *p = (NMLVKEYDOWN*)lParam;

			bool control = GetKeyState(VK_CONTROL)>>15;

			if(control) switch(p->wVKey)			
			{	
			case 'X': assert(0); //ID_CTRL_X
			
				//conflict: cancel downloading? 
				if(*Somtext_downloading) break; 
			
			/*case 'X':*/ case 'C':	case 'V':
				
				Somtext_xfer(p->wVKey,p->hdr.hwndFrom);
				break;

			default: return 0; //may beep
			}
			else switch(p->wVKey) //!control
			{
			case VK_DOWN: //append to bottom of list?

				if(control) break; 

				if(ListView_GetItemCount(p->hdr.hwndFrom)-1
				  ==ListView_GetNextItem(p->hdr.hwndFrom,-1,LVNI_FOCUSED)
				  &&~(GetKeyState(VK_CONTROL)|GetKeyState(VK_SHIFT))>>15)
				for(int sel=0;sel!=-1;) //clear selection for Somtext_edit
				{
					ListView_SetItemState(p->hdr.hwndFrom,sel,0,LVIS_SELECTED);
					sel = ListView_GetNextItem(p->hdr.hwndFrom,sel,LVNI_SELECTED); 
				}
				else break; //FALL THRU				
						
			
			case VK_F2: //used by Explorer->Rename
			case 'I':	Somtext_edit(p->hdr.hwndFrom); break;
			case VK_DELETE:	Somtext_subtract(p->hdr.hwndFrom); break;			
			case VK_SPACE: 
			{
				POINT pt; int pos = //LVN_ITEMACTIVATE
				ListView_GetNextItem(p->hdr.hwndFrom,-1,LVNI_FOCUSED); 				
				if(pos!=-1&&ListView_GetItemPosition(p->hdr.hwndFrom,pos,&pt))
				SendMessage(p->hdr.hwndFrom,WM_LBUTTONDBLCLK,0,POINTTOPOINTS(pt));
				break;
			}
			default: return 0; //may beep
			}
			SetWindowLong(hWndDlg,DWL_MSGRESULT,1);
			return 1;
		}
		case LVN_ITEMACTIVATE:
		{				
			wchar_t path[MAX_PATH] = L"";

			NMITEMACTIVATE *p = (NMITEMACTIVATE*)lParam;

			LVITEMW lvi = {LVIF_TEXT,p->iItem,1,0,0,path,MAX_PATH};

			if(SendMessageW(p->hdr.hwndFrom,LVM_GETITEMTEXTW,p->iItem,(LPARAM)&lvi))
			{
				if(32>(int)ShellExecuteW(hWndDlg,L"open",path,0,0,SW_SHOWNORMAL)) MessageBeep(-1);
			}
			break;
		}}
		break;
	}
	case WM_SYSCOMMAND: 

		switch(wParam)
		{
		case SC_KEYMENU: goto about; break;

		case SC_CONTEXTHELP: Somtext_f1(hWndDlg); return 1;
		}

		break;

	case WM_CONTEXTMENU: 
	{
		bool keyboard = 
		(GetKeyState(VK_F10)|GetKeyState(VK_APPS))>>15;

		HWND lv = (HWND)wParam;		
		if(lv==GetDlgItem(hWndDlg,IDC_SEKAI_TEXT))
		{
			union{ RECT ir; POINT pt; }; pt.x = pt.y = 0;

			if(keyboard)
			{						
				int i = ListView_GetNextItem(lv,-1,LVNI_FOCUSED);					
				if(ListView_GetItemRect(lv,i==-1?0:i,&ir,LVIR_BOUNDS))
				pt.y = ir.bottom; ClientToScreen(lv,&pt);
			}
			else if(!GetCursorPos(&pt)) break;
			static HMENU pu = 
			LoadMenuW(Somversion_dll(),MAKEINTRESOURCEW(IDR_TEXT));				
			switch(TrackPopupMenu(GetSubMenu(pu,0),TPM_RETURNCMD|TPM_NONOTIFY,pt.x,pt.y,0,hWndDlg,0))
			{
			case ID_EDIT_X: Somtext_xfer('X',lv); break;
			case ID_EDIT_C: Somtext_xfer('C',lv); break;
			case ID_EDIT_V:	Somtext_xfer('V',lv); break;
			case ID_EDIT_DEL: Somtext_subtract(lv); break;
			case ID_EDIT_I: Somtext_edit(lv); break;
			}
			return 1;
		}
		about:
		HMENU about, menu = GetMenu(GetParent(hWndDlg));
		if(about=GetSubMenu(menu,GetMenuItemCount(menu)-1))
		{
			int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

			if(keyboard)
			{
				RECT pt = {0,0,0,0}; GetClientRect(hWndDlg,&pt);
				ClientToScreen(hWndDlg,(POINT*)&pt); x = pt.left; y = pt.top;
			}
			else //something is horribly wrong here???
			{									  
				//scoping to non-client area shouldn't be necessary!
				POINT pt = {x,y}; RECT cr; GetClientRect(hWndDlg,&cr);
				if(!ScreenToClient(hWndDlg,&pt)||!PtInRect(&cr,pt)) break;
			}
			TrackPopupMenu(about,0,x,y,0,GetParent(hWndDlg),0);
			return 1;
		}
		return 1;
	}
	case WM_DROPFILES:
	{	
		int ins = 0;
		HDROP drop = (HDROP)wParam;				
		POINT pt = {0,0}; DragQueryPoint(drop,&pt);
		HWND lv = GetDlgItem(hWndDlg,IDC_SEKAI_TEXT);

		if(Somtext_xferring) 
		{
			//cut/copy/paste operation
			ins = ListView_GetNextItem(lv,-1,LVNI_SELECTED);
			if(ins==-1) ins = ListView_GetItemCount(lv);
		}
		else //drag and drop operation
		{
			RECT box = Sompaste->frame(lv,hWndDlg);	

			if(PtInRect(&box,pt))
			{
				pt.x-=box.left; pt.y-=box.top;
				OffsetRect(&box,-box.left,-box.top);

				RECT it; 
				ins = ListView_GetItemCount(lv);			
				for(int i=0;i<ins;i++)
				if(ListView_GetItemRect(lv,i,&it,LVIR_BOUNDS)
				  &&PtInRect(&it,pt))
				{
					ins = i; break;
				}
			}		
		} 

		wchar_t dst[MAX_PATH];
		LVITEMW lvi = {LVIF_TEXT,ins,0,0,0,dst};		
		ListView_SetItemState(lv,-1,0,-1);
		lvi.mask|=LVIF_STATE; lvi.state = LVIS_SELECTED;
		SetFocus(lv); //highlight new selection
		for(int i=-1+DragQueryFileW(drop,-1,0,0);i>=0;i--)		
		if(DragQueryFileW(drop,i,dst,MAX_PATH))
		{			
			lvi.iSubItem = 0; 
			lvi.pszText = PathFindFileNameW(dst);				
			SendMessageW(lv,LVM_INSERTITEMW,ins,(LPARAM)&lvi);
			lvi.iSubItem = 1; lvi.pszText = Sompaste->path(dst);
			SendMessageW(lv,LVM_SETITEMTEXTW,ins,(LPARAM)&lvi);
		}
		DragFinish(drop);		
		SetWindowLong(hWndDlg,DWL_MSGRESULT,1);
		return 1;
	}
	case WM_CLOSE: close:
	{
		//write list view to the registry
		const size_t reg_s = MAX_PATH*16;
		wchar_t buf[MAX_PATH], reg[reg_s+5] = 
		L"\xFEFF"
		L"Windows Registry Editor Version 5.00\r\n"
		L"\r\n"
		L"[HKEY_CURRENT_USER\\SOFTWARE\\FROMSOFTWARE\\SOM\\TEXT]\r\n"
		L"\"";
		size_t len = wcslen(reg);		
		for(size_t i=0;Somtext_instdir[i]&&i<MAX_PATH;i++) switch(Somtext_instdir[i])
		{							
		case '\\': case'"': reg[len++] = '\\'; default: reg[len++] = Somtext_instdir[i];
		}		
		wcscpy(reg+len,L"\"=\""); len+=3;
		{
			size_t first = len;
			LVITEMW lvi = {LVIF_TEXT,0,1,0,0,buf,MAX_PATH};
			HWND lv = GetDlgItem(hWndDlg,IDC_SEKAI_TEXT);

			for(int i=0,n=ListView_GetItemCount(lv);i<n;i++,*buf='\0')
			{
				if(len+MAX_PATH>=reg_s) break; //just truncating

				SendMessageW(lv,LVM_GETITEMTEXTW,i,(LPARAM)&lvi);

				size_t j, last = len;
				for(j=0;buf[j]&&j<MAX_PATH;j++) switch(buf[j])
				{							
				case '\\': case'"': reg[len++] = '\\'; default: reg[len++] = buf[j];					
				}

				if(first!=last) //remove duplicates
				{
					reg[last-1] = reg[len] = '\0'; 

					wchar_t *dup = wcsstr(reg+first,reg+last);

					reg[last-1] = ' ';

					if(dup&&dup+j<reg+last&&dup[len-last]==';') 
					{
						reg[len=last] = '\0'; //found duplicate
						continue;
					}
				}

				reg[len++] = ';'; reg[len++] = ' ';
			}
		}
		if(reg[len-1]==' ') 		
		if(reg[len-=2]!=';') assert(0);
		reg[len++] = '"'; reg[len] = '\0';		
		switch(MessageBoxW(hWndDlg,reg,L"RegEdit",MB_YESNOCANCEL))
		{
		case IDNO: break;
		case IDCANCEL: return 1;
		case IDYES:

			bool success = false;

			wchar_t xtext_reg[MAX_PATH] = L"\\Text.reg"; 
			
			FILE *f = _wfopen(Somfile(xtext_reg),L"wb");

			if(f&&fwrite(reg,len*sizeof(*reg),1,f))
			{
				fflush(f); fclose(f); //flush?

				SHELLEXECUTEINFOW si =
				{
					sizeof(si),SEE_MASK_NOCLOSEPROCESS,
					hWndDlg,L"open",xtext_reg,0,0,SW_SHOWNORMAL
				};
				if(success=ShellExecuteExW(&si))
				{
					WaitForSingleObject(si.hProcess,INFINITE);
					CloseHandle(si.hProcess);
				}
			}
			else if(f) fclose(f);
			
			if(!success) //todo: something?
			{
				//ASSUMING USER DECLINED REGEDIT FOR NOW
			}
		}

		EndDialog(hWndDlg,0);
		ShowWindow(GetParent(hWndDlg),SW_SHOW);
		return 0;
	}
	case WM_NCDESTROY:

		UnhookWindowsHookEx(Somtext_accelerator_next);
		res::close(Somtext_sekai_res);
		res::close(Somtext_group_res);
		FreeLibrary(Sompaste_dll);	   
		break;
	}																	   

	return 0;
}

//2019: Disabling IDC_SEKAI_ZIP prior to downloading
//hampers interaction.
static bool Somtext_download_exists = false;
static bool Somtext_download_in_progress = false;

//WM_UPDATEUISTATE hates the disabled!
void Somtext_toggleaddbutton(HWND dlg)
{
	if(Somtext_adding) return;																				

	HWND add = GetDlgItem(dlg,IDC_SEKAI_ADD);
	HWND zip = GetDlgItem(dlg,IDC_SEKAI_ZIP);

	bool gray = -1==SendMessageW(zip,CB_GETCURSEL,0,0);

	//2019: Yes, this is a hack...
	//if(!IsWindowEnabled(zip)) gray = true; //hack?
	if(!Somtext_download_exists||Somtext_download_in_progress) 
	gray = true;

	const wchar_t addition_sign[] = {'&',0xff0b,0};	

	SetWindowTextW(add,addition_sign+gray); EnableWindow(add,!gray);
}

static INT_PTR CALLBACK Somtext_BackgroundProc(HWND,UINT,WPARAM,LPARAM)
{
	return FALSE;
}

static int Somtext_focus = 0; //hack	

static void Somtext_destination(HWND dlg, wchar_t out[MAX_PATH])
{	
	HWND url = GetDlgItem(dlg,IDC_SEKAI_URL);
	HWND top = GetDlgItem(dlg,IDC_SEKAI_TOP);
	HWND zip = GetDlgItem(dlg,IDC_SEKAI_ZIP);

	//hack: let Somtext_setfocus override zip
	if(Somtext_focus==IDC_SEKAI_TOP) zip = 0;

	const size_t text_s = 64;
	wchar_t toptext[text_s+4] = L""; GetWindowTextW(top,toptext,text_s);
	wchar_t curtext[text_s+5] = L"/"; GetWindowTextW(zip,curtext+1,text_s);
	wchar_t *curtext_or_res = L".res";
	
	//will be .res if zip's text is empty, else /curtext.zip
	if(curtext[1]) curtext_or_res = wcscat(curtext,L".zip");

	swprintf_s(out,MAX_PATH,L"%ls\\%ls%s",Somtext_textdir,toptext,curtext_or_res);
}

static void Somtext_opengroup(HWND dlg, wchar_t resfile[MAX_PATH])
{
	wchar_t text[128] = L"";
	HWND zip = GetDlgItem(dlg,IDC_SEKAI_ZIP);
	GetWindowTextW(zip,text,sizeof(text)/sizeof(*text));

	res::close(Somtext_group_res);

	SendMessageW(zip,CB_RESETCONTENT,0,0);

	bool enable = false;
	if(PathFileExistsW(resfile)
	 &&res::open(Somtext_group_res,resfile))
	{
		res::resources_t r = res::resources(Somtext_group_res);
		for(size_t i=0,n=res::count(Somtext_group_res);i<n;i++)
		{
			if(res::dialog==res::type(r[i]))
			{
				int j = 
				SendMessageW(zip,CB_ADDSTRING,0,(LPARAM)r[i].name);
				SendMessageW(zip,CB_SETITEMDATA,j,i);
				enable = true;
			}
		}
	}
	EnableWindow(zip,enable);

	int found = //restore selection
	SendMessageW(zip,CB_FINDSTRINGEXACT,-1,(LPARAM)text);
	SendMessageW(zip,CB_SETCURSEL,found,0);
	Somtext_toggleaddbutton(dlg);
}

static void Somtext_setfocus(int id, HWND cb)
{
	return Somtext_selchange(id,cb,true);
}
static void Somtext_selchange(int id, HWND cb, bool refocus)
{								  
	//static int focus = 0; //NEW: using Somtext_focus instead
	if(refocus&&Somtext_focus==id) return; Somtext_focus = id;

	int sid = SendMessage(cb,CB_GETCURSEL,0,0);
	sid = SendMessage(cb,CB_GETITEMDATA,sid,0);

	if(sid==CB_ERR) return;

	res::resources_t r = 
	res::resources(id==IDC_SEKAI_TOP?Somtext_sekai_res:Somtext_group_res);
	const void *db = res::database(r[sid]);
		
	HWND bg = !db?0:
	CreateDialogIndirectW(Somversion_dll(),(LPCDLGTEMPLATEW)db,cb,Somtext_BackgroundProc);
	if(!bg) return;

	HWND dlg = GetParent(cb);
	HWND url = GetDlgItem(dlg,IDC_SEKAI_URL);
	HWND bgurl = GetDlgItem(bg,IDC_SEKAI_URL);
	HWND tool = GetDlgItem(dlg,IDC_SEKAI_TOOL);
	HWND data = GetDlgItem(dlg,IDC_SEKAI_DATA);
	HWND info = GetDlgItem(dlg,IDC_SEKAI_INFO);
	HWND bginfo = GetDlgItem(bg,IDC_SEKAI_INFO);	

	const int swap_s = 8192; 	
	wchar_t swap[8192+4] = L"";
	if(id==IDC_SEKAI_TOP)	
	GetWindowTextW(bg,swap,swap_s); 
	GetWindowTextW(bgurl,swap,swap_s); SetWindowTextW(url,swap);
	GetWindowTextW(bginfo,swap,swap_s); SetWindowTextW(info,swap);
	
	if(id==IDC_SEKAI_TOP) //clear right menu selection
	{
		if(!refocus)
		{
			HWND zip = 
			GetDlgItem(dlg,IDC_SEKAI_ZIP);
			//SendMessage(zip,CB_SETCURSEL,-1,0);
			SendMessage(zip,CB_RESETCONTENT,-1,0);
			//causes an annoying gray flicker effect
			//EnableWindow(zip,0);

			//NEW: just update TextDir here
			wchar_t textdir[MAX_PATH] = L"";
			const wchar_t *text = Somtext_textdir;
			if(Somtext_texdirisrel) text+=wcslen(Somtext_instdir)+1;
			swprintf_s(textdir,L"%ls\\%ls",text,r[sid].name);		
			SHSetValueW(HKEY_CURRENT_USER,
			L"SOFTWARE\\FROMSOFTWARE\\SOM\\INSTALL",L"TextDir",
			REG_SZ,textdir,wcslen(textdir)*sizeof(wchar_t));	
		}

		SendMessage(tool,BM_SETCHECK,0,0);
		SendMessage(data,BM_SETCHECK,0,0);
	}
	else //fill in checkboxes
	{
		HWND bgtool = GetDlgItem(bg,IDC_SEKAI_TOOL);
		HWND bgdata = GetDlgItem(bg,IDC_SEKAI_DATA);
		SendMessage(tool,BM_SETCHECK,IsWindowEnabled(bgtool),0);
		SendMessage(data,BM_SETCHECK,IsWindowEnabled(bgdata),0);
	}

	DestroyWindow(bg);

	Somtext_selchanging = true;
	Somtext_download(GetDlgItem(dlg,IDC_SEKAI_GET));
	Somtext_selchanging = false;

	Somtext_toggleaddbutton(dlg);
}

static bool Somtext_progress(const int stats[4], HWND dl)
{	
	wchar_t label[64] = L"";
	bool online = stats[1]>=0;

	if(stats[1]==0) //indicates WinInet bug/timeout
	{	
		if(*Somtext_downloading)
		if(*Somtext_downloading!='?')
		{					
			//SEE ALSO Somdialog.cpp
			//hack: more sleep may help WinInet cope better :/
			if(Somdownload_sleep<1000) //Somdownload_sleep+=100; 		
			if(IDYES==MessageBoxW(dl,
			L"Would you like to try a slower download speed. This usually works, but is slower.\r\n"
			L"This will slow the downloader down by half, and will be in affect until this window is closed.\r\n",
			L"Somversion.dll - WinInet Trouble)",
			MB_YESNO)) Somdownload_sleep+=Somdownload_sleep;

		lock: *Somtext_downloading = '?'; //lock: CopyFile locked up

			const wchar_t question_mark[] = {'&',0xFF1F,0};	
			swprintf_s(label,L"0%%  %dB  %ls",stats[0],question_mark);
			SetWindowTextW(dl,label);				
		}		

		Somtext_download_in_progress = false; //2022 (see below comment)

		return false;
	}	

	bool cancel = !*Somtext_downloading;
	int percent = !online?0:float(stats[0])/stats[1]*100;		
	const wchar_t downwards_arrow[] = {'&',0x2193,0};
	const wchar_t multiplication_sign[] = {'&',0xD7,0};		

	Somtext_download_exists = percent==100; //2019

	if(percent==100||cancel)
	{	
		//2022: nothing is unsetting this and so Somtext_toggleaddbutton 
		//is failing (why?)
		Somtext_download_in_progress = false; //NEW

		HWND dlg = GetParent(dl);
		wchar_t dst[MAX_PATH] = L"";
		Somtext_destination(dlg,dst);
		if(!cancel)
		{
			bool group = ! 
			wcsicmp(PathFindExtensionW(dst),L".res");
			if(!group)
			{
				wchar_t *fn = PathFindFileNameW(dst);

				if(*fn&&fn!=dst) //clear a path
				{
					fn[-1] = '\0'; CreateDirectoryW(dst,0);
					fn[-1] = '\\';
				}
			}
			//must release lock on file for CopyFile
			if(group) res::close(Somtext_group_res);
			//todo: CopyFileExW with progress indicator?
			cancel = !CopyFileW(Somtext_downloading,dst,0);		
			if(!cancel)
			swprintf_s(label,L"100%%  %dB  %ls",stats[1],downwards_arrow);						
			if(group) Somtext_opengroup(dlg,dst);
			if(cancel) goto lock;
		}		
		else wcscpy(label,downwards_arrow);

		EnableWindow(GetDlgItem(dlg,IDC_SEKAI_TOP),1);
		if(res::count(Somtext_group_res))
		EnableWindow(GetDlgItem(dlg,IDC_SEKAI_ZIP),1);

		Somtext_toggleaddbutton(dlg);
		
		if(Somtext_returnfocusid)
		PostMessage(dlg,Somtext_returnfocus,0,0);

		*Somtext_downloading = '\0'; //!!

		if(Somtext_adding)
		{
			Somtext_adding = false;
			PostMessage(dlg,WM_COMMAND,ID_SEKAI_ADD,0);
		}

		cancel = true; //Somdownload seems to want this
	}	
	else assert(percent<100);

	if(!*label)
	{
		const wchar_t *icon = 
		percent<100&&!cancel?multiplication_sign:downwards_arrow;
		if(online) 
		swprintf_s(label,L"%d%%  %d / %dB (%dB/s)  %ls",percent,stats[0],stats[1],stats[2],icon);
		else swprintf_s(label,L"0%%  0 / ?B (0B/s)  %ls",icon);		
	}
		
	if(*label) SetWindowTextW(dl,label);	
	
	return !cancel;
}

static void Somtext_download(HWND dl)
{
	HWND dlg = GetParent(dl);

	const wchar_t downwards_arrow[] = {'&',0x2193,0};
	
	wchar_t label[64] = {'&',0x2193,0}; //downwards_arrow; 
	
	//Somtext_download_exists = true; //2019
	//Somtext_download_in_progress = false; //2019

	if(Somtext_selchanging)
	{
		Somtext_download_exists = true; //2019

		wchar_t dst[MAX_PATH] = L"";		
		Somtext_destination(dlg,dst);

		if(PathFileExistsW(dst))
		{
			FILE *f = _wfopen(dst,L"rb");

			if(f&&!fseek(f,0,SEEK_END))
			{
				size_t Bs = ftell(f);
				swprintf_s(label,L"100%%  %dB  %ls",Bs,downwards_arrow);
				if(!wcsicmp(PathFindExtensionW(dst),L".res"))
				Somtext_opengroup(dlg,dst);
			}		
			else swprintf_s(label,L"?%%  ?B  %ls",downwards_arrow);

			if(f) fclose(f);
		}
		else  //2019: I don't know, but disabling this seems wrong???
		{
			//This hampers interaction.
			//EnableWindow(GetDlgItem(dlg,IDC_SEKAI_ZIP),0);
			Somtext_download_exists = false;
			
		}
	}
	else //downloading...
	{
		Somtext_download_in_progress = true;

		const wchar_t multiplication_sign[] = {'&',0xD7,0};	
		swprintf_s(label,L"0%%  0 / ?B (0B/s)  %ls",multiplication_sign);
	
		//it's important to the default button setup...
		HWND url = GetDlgItem(dlg,IDC_SEKAI_URL);
		if(GetWindowTextW(url,Somtext_downloading,MAX_PATH))
		Somdownload(Somtext_downloading,(Somdownload_progress)Somtext_progress,dl);
		//that these are disabled only after Somtext_downloading is setup
		EnableWindow(GetDlgItem(dlg,IDC_SEKAI_TOP),0);
		EnableWindow(GetDlgItem(dlg,IDC_SEKAI_ZIP),0);
		Somtext_toggleaddbutton(dlg);
	}
		
	SetWindowTextW(dl,label);
}

static void Somtext_add(HWND bt)
{
	if(Somtext_adding) //cancelling
	{
		*Somtext_downloading = '\0'; return;
	}
		
	HWND dlg = GetParent(bt);	
	wchar_t dst[MAX_PATH] = L"";
	Somtext_destination(dlg,dst);

	const wchar_t addition_sign[] = {'&',0xff0b,0};	
	const wchar_t multiplication_sign[] = {'&',0xD7,0};	

	if(!PathFileExistsW(dst))
	{	
		Somtext_adding = true;
		SetWindowTextW(bt,multiplication_sign);			
		Somtext_download(GetDlgItem(dlg,IDC_SEKAI_GET));
		return;
	}

	//incase Somtext_adding was true
	SetWindowTextW(bt,addition_sign);

	HWND lv = GetDlgItem(dlg,IDC_SEKAI_TEXT);	
	const wchar_t *label = PathFindFileNameW(dst);	
	LVITEMW lvi = {LVIF_TEXT,0,0,0,-1,(LPWSTR)label};
	ListView_SetItemState(lv,-1,0,-1);
	lvi.mask|=LVIF_STATE; 
	lvi.state = LVIS_FOCUSED|LVIS_SELECTED;
	SendMessageW(lv,LVM_INSERTITEMW,0,(LPARAM)&lvi);
	lvi.iSubItem = 1;
	lvi.pszText = Sompaste->path(dst);
	SendMessageW(lv,LVM_SETITEMTEXTW,0,(LPARAM)&lvi);
}

static void Somtext_subtract(HWND lv)
{
	int sel = ListView_GetNextItem(lv,-1,LVNI_SELECTED);

	if(sel==-1) sel = 0; do
	{		
		ListView_DeleteItem(lv,sel);
	
		sel = ListView_GetNextItem(lv,-1,LVNI_SELECTED);
	}	
	while(sel!=-1);
}

static void Somtext_edit(HWND lv)
{	
	if(GetFocus()!=lv){	MessageBeep(-1); return; }

	int i = ListView_GetNextItem(lv,-1,LVNI_FOCUSED);		
	if(-1==ListView_GetNextItem(lv,-1,LVNI_SELECTED))
	{
		i = ListView_GetItemCount(lv);
		LVITEMW lvi = {LVIF_STATE,i,0,LVIS_FOCUSED|LVIS_SELECTED,-1};
		SendMessageW(lv,LVM_INSERTITEMW,0,(LPARAM)&lvi);
	}
	ListView_EditLabel(lv,i);
}

static void Somtext_xfer(int op, HWND lv)
{
	bool beep = true;

	Somtext_xferring = op;

	HWND hWndDlg = GetParent(lv);

	//!: LVN_BEGINDRAG is out of focus
	if(!op||lv==GetFocus()) switch(op)
	{	
	case 'V': //hack: forwarding to WM_DROPFILES

		if(Sompaste->xfer("Paste list:file.Windows",hWndDlg))
		beep = false;
		break;

	case 'X': case 'C': case 0:
				
		//1: L"" is the list's separators list
		const size_t list_s = MAX_PATH*8, empty = 1;
		wchar_t list[MAX_PATH*8] = L""; size_t listed = empty;
		LVITEMW lvi = {LVIF_TEXT,0,1};
		for(int sel=ListView_GetNextItem(lv,-1,LVNI_SELECTED);sel!=-1;)
		{
			if(op!='C')
			ListView_SetItemState(lv,sel,LVIS_CUT,LVIS_CUT);

			lvi.iItem = sel; 
			lvi.pszText = list+listed; lvi.cchTextMax = list_s-listed;
			size_t len = SendMessageW(lv,LVM_GETITEMTEXTW,sel,(LPARAM)&lvi);
			if(len) listed+=len+1;				
			
			sel = ListView_GetNextItem(lv,sel,LVNI_SELECTED);
		}	
		if(listed==empty) break; //beep!

		char *xop = "Copy list:file.Windows";
		if(!op) xop+=sizeof("Copy"); //drag & drop
		HWND x = Sompaste->xfer(xop,list,listed*sizeof(wchar_t));
		if(!x) MessageBeep(-1);

		//drag/dropped onto an other application
		if(!op&&hWndDlg!=GetAncestor(x,GA_ROOT)) x = 0;

		int xfocus = -1;
		
		if(op!='C')
		for(int cut=ListView_GetNextItem(lv,-1,LVNI_CUT);cut!=-1;)
		{
			if(x) ListView_DeleteItem(lv,xfocus=cut);
			else ListView_SetItemState(lv,cut,0,LVIS_CUT);
			cut = ListView_GetNextItem(lv,x?-1:cut,LVNI_CUT);
		}

		if(xfocus!=-1)
		ListView_SetItemState(lv,xfocus,LVIS_FOCUSED|LVIS_SELECTED,-1);

		beep = false;
		break;
	}	

	if(beep) MessageBeep(-1);

	Somtext_xferring = 0;
}

static INT_PTR CALLBACK Somtext_HelpProc(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg)
	{
	case WM_INITDIALOG:
	{
		HWND parent = GetParent(hWndDlg);

		HICON icon = (HICON)
		SendMessage(parent,WM_GETICON,ICON_SMALL,0);		
		SendMessage(hWndDlg,WM_SETICON,ICON_SMALL,(LPARAM)icon);		

		HFONT font = (HFONT) 
		SendDlgItemMessage(parent,IDC_SEKAI_INFO,WM_GETFONT,0,0);
		SendDlgItemMessage(hWndDlg,IDC_SEKAI_HELP,WM_SETFONT,(WPARAM)font,0);
		SendDlgItemMessage(hWndDlg,IDC_SEKAI_HELP2,WM_SETFONT,(WPARAM)font,0);

		wchar_t uri[MAX_PATH*4] = L"";		
		if(GetDlgItemTextW(parent,IDC_SEKAI_URL,uri,sizeof(uri)/sizeof(*uri)));
		SetDlgItemTextW(hWndDlg,IDC_SEKAI_HELP,uri);

		*uri = '\0'; Somtext_destination(parent,uri);
		SetDlgItemTextW(hWndDlg,IDC_SEKAI_HELP2,uri);

		static HMENU menu = 0; if(!menu) //HACK
		{
			HMENU dl = GetMenu(GetParent(GetParent(hWndDlg)));		
			dl = GetSubMenu(dl,GetMenuItemCount(dl)-2);
			if(ID_DOWNLOAD_WINSOCK==GetMenuItemID(dl,0))
			{
				menu = CreateMenu();				
				AppendMenuW(menu,MF_POPUP,(UINT_PTR)dl,L"Change &Download Method");			
				
			}
		}
		SetMenu(hWndDlg,menu);
		return 1;
	}
	case WM_CTLCOLORSTATIC:

		SetBkMode((HDC)wParam,TRANSPARENT); break;

	case WM_COMMAND:

		switch(HIWORD(wParam))
		{			
		case EN_SETFOCUS: 		

			SendMessage((HWND)lParam,EM_SETSEL,0,-1);	
			DestroyCaret(); 
			break;

		case EN_KILLFOCUS:
				
			SendMessage((HWND)lParam,EM_SETSEL,-1,0);	
			break;
		}

		switch(LOWORD(wParam))
		{		
		//default button
		case IDCANCEL: goto close;

		case ID_DOWNLOAD_WINSOCK: case ID_DOWNLOAD_WININET:

			SendMessage(GetParent(GetParent(hWndDlg)),Msg,wParam,lParam);
			break;
		}
		break;

	case WM_CLOSE: close:
			
		SetMenu(hWndDlg,0); //HACK	
		EndDialog(hWndDlg,0);		
		return 0;
	}

	return 0;
}
static void Somtext_f1(HWND dlg)
{
	if('?'==*Somtext_downloading) 
	{
		*Somtext_downloading = '\0';

		//hack: restore down arrow
		Somtext_selchanging = true;
		Somtext_download(GetDlgItem(dlg,IDC_SEKAI_GET));
		Somtext_selchanging = false;
	}
	DialogBoxW(Somversion_dll(),MAKEINTRESOURCEW(IDD_SEKAI_HELP),dlg,Somtext_HelpProc);
}

void Somtext_f5(HWND hWndDlg, const wchar_t *sel)
{
	HWND top = GetDlgItem(hWndDlg,IDC_SEKAI_TOP);	 

	wchar_t tmp[MAX_PATH] = L"";
	res::close(Somtext_sekai_res);
	swprintf_s(tmp,L"%ls\\sekai.res",Somtext_textdir);		
	res::open(Somtext_sekai_res,tmp);

	if(!sel) //overwriting tmp 
	{
		sel = tmp; *tmp = '\0';

		GetWindowTextW(top,tmp,MAX_PATH);
	}

	SendMessage(top,CB_RESETCONTENT,0,0);

	size_t lc = (WORD) //LANGIDFROMLCID 
	ConvertDefaultLocale(LOCALE_SYSTEM_DEFAULT),
	bestfit = 0; res::unicode_t bestsel = 0; 
	res::resources_t r = res::resources(Somtext_sekai_res);
	for(size_t i=0,n=res::count(Somtext_sekai_res);i<n;i++)
	{
		if(res::dialog==res::type(r[i]))
		{
			//0x3ff: common language match
			if((0x3ff&*r[i].lcid)==(0x3ff&lc))  
			{
				if((0x3ff&bestfit)!=(0x3ff&lc))
				{
					bestsel = r[i].name; bestfit = lc;
				}
			}
			//perfect language/region match
			if(*r[i].lcid==lc&&bestfit!=lc) 
			{
				bestsel = r[i].name; bestfit = lc;
			}
			//0800: Neutral so says ResEdit
			//Note: this should be Japanese 
			if(*r[i].lcid==0x0800&&!bestfit)
			{
				bestsel = r[i].name; bestfit = lc;
			}

			int j = 
			SendMessageW(top,CB_ADDSTRING,0,(LPARAM)r[i].name);
			SendMessageW(top,CB_SETITEMDATA,j,i);
		}
	}
	int j = !*sel?-1:
	SendMessageW(top,CB_FINDSTRINGEXACT,-1,(LPARAM)sel);
	if(j<0) j = !bestsel?0:
	SendMessageW(top,CB_FINDSTRINGEXACT,-1,(LPARAM)bestsel);
	SendMessageW(top,CB_SETCURSEL,j,0);
	Somtext_selchange(IDC_SEKAI_TOP,top);
}

extern void Somtext(HWND parent)
{
	if(!*Somtext_instdir)
	{	
		DWORD max_path = sizeof(Somtext_instdir);		
		SHGetValueW(HKEY_CURRENT_USER,
		L"SOFTWARE\\FROMSOFTWARE\\SOM\\INSTALL",L"InstDir",0,Somtext_instdir,&max_path);
		if(!*Somtext_instdir) return;
	}

	if(!parent) //setup language for the first time?
	{				
		DWORD test = 0;
		SHGetValueW(HKEY_CURRENT_USER,
		L"SOFTWARE\\FROMSOFTWARE\\SOM\\TEXT",Somtext_instdir,0,0,&test);
		if(!test) //try (Default)
		SHGetValueW(HKEY_CURRENT_USER,
		L"SOFTWARE\\FROMSOFTWARE\\SOM\\TEXT",L"",0,0,&test);
		if(test) return;

		int id = ID_TOOLS_SEKAI;
		Somdialog(id,0,0,SOMVERSION(0,0,0,0),0,0); //hack mode
		return;
	}
	
	if(parent)	
	DialogBoxW(Somversion_dll(),MAKEINTRESOURCEW(IDD_SEKAI),parent,Somtext_DialogProc);
}