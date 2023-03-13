
#include "Sompaste.pch.h"

BOOL Somprops_broadcast(HWND hWndDlg, WPARAM nm, LPARAM lparam=0)
{
	HWND *pages = (HWND*)GetProp(hWndDlg,"Sompaste_propsheet");

	int count =	TabCtrl_GetItemCount(GetDlgItem(hWndDlg,IDC_TABS));

	BOOL out = 0;

	for(int i=0;i<count;i++)
	{
		PSHNOTIFY pn = {hWndDlg,i,nm,lparam};

		//note: does not handle PSNRET_INVALID_NOCHANGEPAGE
		out|=SendMessage(pages[i],WM_NOTIFY,0,(WPARAM)&pn);
	}	  

	return out;
}

static void Somprops_adjust(HWND hWndDlg, HWND page=0)
{	
	HWND tabs = GetDlgItem(hWndDlg,IDC_TABS);

	HWND *pages = (HWND*)GetProp(hWndDlg,"Sompaste_propsheet");
	
	RECT rect = SOMPASTE_LIB(Pane)(tabs);

	TabCtrl_AdjustRect(tabs,FALSE,&rect); 
		
	POINT pt = { rect.left, rect.top };
	POINT sz = { rect.right-rect.left, rect.bottom-rect.top };

	if(!page)
	{
		int count = TabCtrl_GetItemCount(tabs);

		for(int i=0;i<count;i++) if(pages[i]) if(i==0||pages[i]!=pages[i-1])
		{	
			if(pages[i]==hWndDlg){ assert(0); break; } //terminator

			SetWindowPos(pages[i],0,pt.x,pt.y,sz.x,sz.y,SWP_NOACTIVATE|SWP_NOZORDER);		
		}
	}
	else SetWindowPos(page,0,pt.x,pt.y,sz.x,sz.y,SWP_NOACTIVATE|SWP_NOZORDER);
}

static LRESULT CALLBACK SompropsTabs(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scid, DWORD_PTR rows) 
{		
	switch(uMsg) 
    { 	
	case TCM_SETITEMA: case TCM_INSERTITEMA: assert(0);

	case WM_SIZE: case TCM_SETITEMW: case TCM_INSERTITEMW: 
	{
		LRESULT out = DefSubclassProc(hwnd,uMsg,wParam,lParam);

		if(TabCtrl_GetRowCount(hwnd)==rows) return out; 
		
		SetWindowSubclass(hwnd,SompropsTabs,0,rows=TabCtrl_GetRowCount(hwnd)); 
				
		Somprops_adjust(GetParent(hwnd));

		return out;

	}break;
	case WM_NCDESTROY:

		RemoveWindowSubclass(hwnd,SompropsTabs,scid);

		break;
	}

	return DefSubclassProc(hwnd,uMsg,wParam,lParam);
}

static INT_PTR CALLBACK SompropsProc(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg) 
	{
	case WM_INITDIALOG: 
	{	
		assert(lParam); //todo: frameless property sheet

		DefWindowProc(hWndDlg,WM_SETICON,ICON_SMALL,LPARAM(Somicon()));
		DefWindowProc(hWndDlg,WM_SETICON,ICON_BIG,LPARAM(Somicon()));
		
		if(lParam) SetWindowTextW(hWndDlg,(wchar_t*)lParam);

		return TRUE;
	}
	case WM_PARENTNOTIFY:
			
		//Note that this is pretty much useless for dialogs

		switch(LOWORD(wParam)) 
		{
		case WM_CREATE:

			HWND *pages = (HWND*)GetProp(hWndDlg,"Sompaste_propsheet");

			int i, count = TabCtrl_GetItemCount(GetDlgItem(hWndDlg,IDC_TABS));

			for(i=0;i<count;i++) if(!pages[i])
			{
				wchar_t title[64]; //This is ignored by dialogs
				if(GetWindowTextW((HWND)lParam,title,64)&&*title)
				//if(SendMessageW((HWND)lParam,WM_GETTEXT,64,(LPARAM)title)&&*title)
				{
					TCITEMW tab = { TCIF_TEXT,0,0,title,0,0};
					SendDlgItemMessage(hWndDlg,IDC_TABS,TCM_SETITEMW,i,(LPARAM)&tab); 
				}						  
				pages[i] = (HWND)lParam; 
								
				Somprops_adjust(hWndDlg,pages[i]);

				break;
			}
			assert(i<count);

			if(i>0) break; //activate first tab...

			PSHNOTIFY pn = { {hWndDlg, 0, PSN_SETACTIVE}, 0 };

			SendMessage(pages[0],WM_NOTIFY,0,(WPARAM)&pn);
			
		}break;

	case WM_NOTIFY:
	{
		NMHDR *n = (LPNMHDR)lParam;
		
		if(n->code==TCN_SELCHANGING||n->code==TCN_SELCHANGE)
		{
			TCITEM item = {TCIF_PARAM}; 
			
			int tab = TabCtrl_GetCurSel(n->hwndFrom);
			
			HWND *pages = (HWND*)GetProp(hWndDlg,"Sompaste_propsheet");

			int psn = n->code==TCN_SELCHANGING?PSN_KILLACTIVE:PSN_SETACTIVE;

			PSHNOTIFY pn = { {hWndDlg, tab, psn}, 0 };

			if(!SendMessage(pages[tab],WM_NOTIFY,0,(WPARAM)&pn)) break;

			if(GetWindowLong(pages[tab],DWL_MSGRESULT))
			{
				if(n->code==TCN_SELCHANGING) return TRUE; assert(0); 
			}
		}	  
		break;
	}
	case PSM_GETTABCONTROL: 

		SetWindowLongPtr(hWndDlg,DWLP_MSGRESULT,(LONG_PTR)GetDlgItem(hWndDlg,IDC_TABS));

		return TRUE;

	case PSM_GETCURRENTPAGEHWND: //FALLS THRU
		
		wParam = TabCtrl_GetCurSel(GetDlgItem(hWndDlg,IDC_TABS));

	case PSM_INDEXTOHWND: //FALLING THRU
	{			
		HWND *pages = (HWND*)GetProp(hWndDlg,"Sompaste_propsheet");		

		if(wParam<TabCtrl_GetItemCount(GetDlgItem(hWndDlg,IDC_TABS)))
		{
			SetWindowLongPtr(hWndDlg,DWLP_MSGRESULT,(LONG_PTR)pages[wParam]);		
		}
		else SetWindowLongPtr(hWndDlg,DWLP_MSGRESULT,0);		

	}return TRUE;
	case PSM_CHANGED:

		EnableWindow(GetDlgItem(hWndDlg,ID_APPLY_NOW),TRUE); break;

	case PSM_PRESSBUTTON:
	
		switch(wParam)
		{
		case PSBTN_OK: wParam = IDOK; goto ok;

		default: assert(0); return 1; //unimplemented
		}  

	case WM_COMMAND:

		if(lParam==0) //assuming origin is a menu
		{
			//assuming menu item: forwarding to first tab

			HWND *pages = (HWND*)GetProp(hWndDlg,"Sompaste_propsheet");

			SendMessage(pages[0],WM_COMMAND,wParam,lParam);	break;
		}
		else switch(LOWORD(wParam))
		{		
		case ID_APPLY_NOW: case IDOK: //FALLS THRU
		{
		ok:	BOOL ret = Somprops_broadcast(hWndDlg,PSN_APPLY,LOWORD(wParam)==IDOK);
						
			if(LOWORD(wParam)==ID_APPLY_NOW||ret)
			{
				EnableWindow(GetDlgItem(hWndDlg,ID_APPLY_NOW),ret); return 0; 
			}			
		}
		case IDCANCEL:; //FALL THRU
		}

	case WM_CLOSE: //FALLING THRU
		
		//note: docs seem to suggest that a close calls for apply
		if(Msg==WM_CLOSE||Msg==WM_COMMAND&&LOWORD(wParam)==IDCANCEL)
		{
			//alert tabs of cancel (and allow tab to abort the operation)
			if(Somprops_broadcast(hWndDlg,PSN_QUERYCANCEL,LOWORD(wParam)==IDOK)) break;
		}

		DestroyWindow(hWndDlg); break; 	

	case WM_NCDESTROY:

		HWND *pages = (HWND*)GetProp(hWndDlg,"Sompaste_propsheet");

		delete [] pages; RemoveProp(hWndDlg,"Sompaste_propsheet");

		break;
	}

	return 0;
}

extern HWND *Somprops(HWND owner, int n, const wchar_t *title, HMENU menu)
{
	if(n<1) return 0; 

	HWND props = CreateDialogParamW(Sompaste_dll(),MAKEINTRESOURCEW(IDD_PROPERTIES),owner,SompropsProc,(LPARAM)title);

	if(menu)
	{
		RECT in, out; 
		GetClientRect(props,&in);

		if(SetMenu(props,menu))
		{
			GetClientRect(props,&out);

			LONG diff = in.bottom-out.bottom, flags = SWP_NOMOVE|SWP_NOZORDER|SWP_FRAMECHANGED;
			
			//Prefer a lip below the menu for now: should be customizable via Sompaint.dll
			SetWindowLong(props,GWL_EXSTYLE,WS_EX_CLIENTEDGE|GetWindowLong(props,GWL_EXSTYLE));
			
			GetWindowRect(props,&out);
			SetWindowPos(props,0,0,0,out.right-out.left,diff=out.bottom-out.top+diff,flags);

		//// NEW: adjust along horizontal ////

			GetClientRect(props,&out);

			LONG diff2 = in.right-out.right;

			GetWindowRect(props,&out);
			SetWindowPos(props,0,0,0,out.right-out.left+diff2,out.bottom-out.top,flags);
		}
	}

	HWND tabs = GetDlgItem(props,IDC_TABS); if(!tabs) return 0;

	HWND *out = new HWND[n+1]; memset(out,0x00,sizeof(HWND)*n); 

	//Reminder: let the user have LVIF_LPARAM
	TCITEMW tab = { TCIF_TEXT|TCIF_STATE,0,0,L"",1,0};

	for(int i=0;i<n;i++)
	{
		out[i] = 0; SendMessage(tabs,TCM_INSERTITEMW,i,(LPARAM)&tab); 		
	}

	out[n] = props;		
	SetProp(props,"Sompaste_propsheet",(HANDLE)out);

	SetWindowSubclass(tabs,SompropsTabs,0,TabCtrl_GetRowCount(tabs)); 

	return out; 
}

extern int Somprops_confirm(HWND owner, const wchar_t *title) //Sompaste.pch.h
{
	std::wstring caption = L"Sompaste.dll - "; caption+=title;

	//TODO: would like to expose but it's not obvious if there is a relevant PSM_ message

	//Copied from the MSVC2005 config dialog. Looks like it is generic to property sheets
	wchar_t *text = L"Do you want to save the changes you've made in the property pages?";

	return MessageBoxW(owner,text,caption.data(),MB_YESNOCANCEL|MB_ICONINFORMATION);
}