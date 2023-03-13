
#include "Somplayer.pch.h"

#pragma comment(lib,"comctl32.lib")
//#include <commctrl.h>
//Sword of Moonlight Mission Control... can you read me?

//Note: may have this file be outside Somplayer.dll one day

//This unit implements a bunch of Win32 control routines that
//are just way too long (due to the nature of Win32) which we keep
//here as to not make our otherwise pistine sources appear too unkempt
  
//Because there is no GetWindowPos API (you're welcome Microsoft)
extern bool Sommctrl_position(RECT *out, HWND client, HWND relative=0)
{	
	if(relative&&!GetWindowRect(relative,out)) return false;
			
	return ScreenToClient(client,(POINT*)&out->left)
		 &&ScreenToClient(client,(POINT*)&out->right);
}

//See Somconsole_cpp::scrollbar (from WM_VSCROLL example in Microsoft docs)
extern bool Sommctrl_scroll(WPARAM wParam, SCROLLINFO &si, bool h, HWND hwnd)
{		
	int Delta, NewPos, v = !h;

	int btsz = GetSystemMetrics(v?SM_CYVSCROLL:SM_CXHSCROLL);

	switch(LOWORD(wParam)) 
	{ 
	case SB_PAGEUP: NewPos = si.nPos-si.nPage; break; 

	case SB_PAGEDOWN: NewPos = si.nPos+si.nPage; break; 

	case SB_LINEUP: NewPos = si.nPos-btsz; break; 

	case SB_LINEDOWN: NewPos = si.nPos+btsz; break;

	case SB_THUMBTRACK: 
	case SB_THUMBPOSITION: NewPos = HIWORD(wParam); break; 

	default: NewPos = si.nPos; 
	} 

	NewPos = std::max(0,NewPos); 
	NewPos = std::min(si.nMax,NewPos); 

	if(NewPos==si.nPos) return false;

	Delta = NewPos-si.nPos; si.nPos = NewPos; 

	//ScrollWindowEx(hwnd,!v?-Delta:0,v?-Delta:0,0,0,0,0,SW_INVALIDATE); 
	//UpdateWindow(hwnd); 

	si.fMask = SIF_POS; 
	SetScrollInfo(hwnd,v?SB_VERT:SB_HORZ,&si,TRUE); 

	return true;
}

//Sort the Sword_of_Moonlight_Media_Library_Texture area
extern void Sommctrl_texture(int width, int height, 
						  
	RECT &crop, int space, RECT &fill, SCROLLINFO *scrollbars, HWND window)
{
	RECT client, swap; GetClientRect(window,&client);

	LONG style = GetWindowLong(window,GWL_STYLE), newstyle = style;
				
	int hm = GetSystemMetrics(SM_CYHSCROLL); if(style&WS_HSCROLL) client.bottom+=hm;
	int vm = GetSystemMetrics(SM_CXVSCROLL); if(style&WS_VSCROLL) client.right+=vm;					

	POINT margin = { GetSystemMetrics(SM_CXHSCROLL)+1, GetSystemMetrics(SM_CYVSCROLL)+1};

	SetRect(&swap,0,0,width,height); OffsetRect(&swap,margin.x,margin.y);
				
	client.right-=margin.x; client.bottom-=margin.y; 

	IntersectRect(&fill,&client,&swap); //sans scrollbars

	if(fill.right==client.right) newstyle|=WS_HSCROLL; else newstyle&=~WS_HSCROLL;			
	if(fill.bottom==client.bottom) newstyle|=WS_VSCROLL; else newstyle&=~WS_VSCROLL;			

	if(newstyle&WS_HSCROLL) client.bottom-=hm; if(newstyle&WS_VSCROLL) client.right-=vm;
				
	////not a mirage: we have to do this twice (or come up with a better solution)
		
	IntersectRect(&fill,&client,&swap); 

	if((newstyle&WS_VSCROLL)==0&&fill.bottom==client.bottom)
	{
		newstyle|=WS_VSCROLL; fill.right-=hm; client.right-=vm;
	}			
	if((newstyle&WS_HSCROLL)==0&&fill.right==client.right)
	{
		newstyle|=WS_HSCROLL; fill.bottom-=hm; client.bottom-=hm;
	}

	//////////////////////////////////////////////////////////////////////////////
	
	RECT temp; swap = fill; 
	OffsetRect(&swap,-margin.x,-margin.y);
	SetRect(&temp,0,0,width,height);			
	IntersectRect(&crop,&temp,&swap);

	if(newstyle&WS_VSCROLL)
	{
		scrollbars[0].nPage = fill.bottom;
		scrollbars[0].nMax = margin.y+height;	
		scrollbars[0].fMask = SIF_PAGE|SIF_RANGE;
		SetScrollInfo(window,SB_VERT,scrollbars+0,FALSE);
	}
	else
	{
		scrollbars[0].nPos = 0;

		switch(space)
		{
		case 4: case 5: case 6: 
		
			fill.top = margin.y+(client.bottom-margin.y-height)/2; break;

		case 1: case 2: case 3: 
			
			fill.top = client.bottom-height; break;
		}

		fill.bottom = fill.top+height;
	}
	if(newstyle&WS_HSCROLL)
	{
		scrollbars[1].nPage = fill.right;
		scrollbars[1].nMax = margin.x+width;	
		scrollbars[1].fMask = SIF_PAGE|SIF_RANGE;
		SetScrollInfo(window,SB_HORZ,scrollbars+1,FALSE);
	}
	else
	{
		scrollbars[1].nPos = 0;

		switch(space)
		{
		case 2: case 5: case 8: 
			
			fill.left = margin.x+(client.right-margin.x-width)/2; break;

		case 3: case 6: case 9:	
			
			fill.left = client.right-width; break;
		}

		fill.right = fill.left+width;
	}

	if(newstyle!=style) 
	{
		SetWindowLong(window,GWL_STYLE,newstyle); //will cause double redraw maybe...

		const DWORD flags = SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOSENDCHANGING; //SWP_NOREDRAW

		SetWindowPos(window,0,0,0,0,0,SWP_FRAMECHANGED|SWP_DRAWFRAME|flags); 
	}

	OffsetRect(&crop,scrollbars[1].nPos,scrollbars[0].nPos);
}

//Sort the Sword_of_Moonlight_Media_Library_Palette area
extern bool Sommctrl_palette(SCROLLINFO *scrollbars, HWND hWndDlg, int *wells, bool size)
{
again: const int sep = 6, v = -scrollbars[0].nPos;
	
	HWND tm = GetDlgItem(hWndDlg,IDC_TONE_MAP);
	HWND nw = GetDlgItem(hWndDlg,IDC_NEW_WELL);
	HWND nt = GetDlgItem(hWndDlg,IDC_NEW_TONE);
	HWND ut = GetDlgItem(hWndDlg,wells[0]); //IDC_UNTITLED
			
	RECT screen; GetWindowRect(ut,&screen);

	int utsz = screen.right-screen.left;
	int btcx = 79, btcy; //79: MS Gothic (todo: better)

	btcx = std::min(btcx*2,std::max(btcx,(utsz-sep)/2));

	const DWORD flags = SWP_NOZORDER|SWP_NOSENDCHANGING; 
	
	int h, scrollheight = sep;

	GetWindowRect(tm,&screen);	
	h = screen.bottom-screen.top;
	ScreenToClient(hWndDlg,(POINT*)&screen);
	SetWindowPos(tm,0,screen.left,scrollheight+v,0,0,flags|SWP_NOSIZE); 
	scrollheight+=h+sep;

	GetWindowRect(nw,&screen);	
	h = screen.bottom-screen.top;
	ScreenToClient(hWndDlg,(POINT*)&screen);
	SetWindowPos(nw,0,screen.left,scrollheight+v,btcx,h,flags); 
	SetWindowPos(nt,0,screen.left+btcx+sep,scrollheight+v,btcx,h,flags); 	
	scrollheight+=h+sep;

	for(int i=0;wells[i];i++)
	{
		HWND wi = GetDlgItem(hWndDlg,wells[i]);

		GetWindowRect(wi,&screen);	
		h = screen.bottom-screen.top;
		ScreenToClient(hWndDlg,(POINT*)&screen);
		SetWindowPos(wi,0,screen.left,scrollheight+v,0,0,flags|SWP_NOSIZE); 		
		scrollheight+=h+sep;
	}

	if(!size) goto update;

	RECT client; GetClientRect(hWndDlg,&client);			
	LONG style = GetWindowLong(hWndDlg,GWL_STYLE), newstyle = style;
					
	if(style&WS_VSCROLL)
	{
		if(scrollheight<client.bottom) newstyle = style&~WS_VSCROLL;			
	}
	else if(scrollheight>client.bottom) newstyle = style|WS_VSCROLL;
	
	if(newstyle&WS_VSCROLL)
	{	
		scrollbars[0].nPos = 
		std::min<int>(-v,scrollheight-client.bottom);	

		if(-v!=scrollbars[0].nPos) goto again;

		scrollbars[0].nMax = scrollheight;	
		scrollbars[0].nPage = client.bottom;
		scrollbars[0].fMask = SIF_PAGE|SIF_RANGE|SIF_POS;
		SetScrollInfo(hWndDlg,SB_VERT,scrollbars+0,FALSE);		
	}
	else scrollbars[0].nPos = 0;

	if(newstyle!=style)
	{
		SetWindowLong(hWndDlg,GWL_STYLE,newstyle);

		const DWORD flags = SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOSENDCHANGING; 

		SetWindowPos(hWndDlg,0,0,0,0,0,SWP_FRAMECHANGED|SWP_DRAWFRAME|flags); 
																	 
		return false; //signal second pass required
	}

update:	//TODO: encapsulate in windowman...

	//Winsanity: don't erase scrollbar!
	WINDOWINFO info = {sizeof(WINDOWINFO)};

	GetWindowInfo(hWndDlg,&info);
	HWND parent = GetParent(hWndDlg);
	Sommctrl_position(&info.rcClient,parent);
	InvalidateRect(parent,&info.rcClient,TRUE);		
	//UpdateWindow(parent);

	return true;
}

extern int Sommctrl_hitbox(HWND parent, bool insert, RECT *box, HWND client=0)
{
	if(client) Sommctrl_position(box,client);

	POINT pt = {box->right,box->bottom};
	HWND child = ChildWindowFromPointEx(parent,pt,CWP_SKIPTRANSPARENT);

	if(child==parent) child = 0; //undocumented behavior??

	if(pt.x==box->right&&pt.y==box->top) goto hit;

	for(int i=0;i<4;i++) //try all corners and return spatial result
	{
		if(child)
		{
hit:		if(insert&&child) 
			{
				RECT client; GetClientRect(child,&client);

				ClientToScreen(parent,&pt); ScreenToClient(child,&pt);

				return GetDlgCtrlID(child)+(pt.x>=client.right/2?1:0);
			}
			else return GetDlgCtrlID(child);
		}

		switch(i)
		{
		case 0: pt.x = box->left;  break; case 1: pt.y = box->top; break;
		case 2: pt.x = box->right; break; case 3: return 0;
		}

		child = ChildWindowFromPointEx(parent,pt,CWP_SKIPTRANSPARENT);

		if(child==parent) child = 0; //undocumented behavior??
	}				
	return 0;
}

extern int Sommctrl_select(HWND parent, RECT *box, HWND client=0)
{
	return Sommctrl_hitbox(parent,false,box,client);
}

extern int Sommctrl_insert(HWND parent, RECT *box, HWND client=0)
{
	return Sommctrl_hitbox(parent,true,box,client);
}

extern bool Sommctrl_cutout(HWND dcwin, HDC dc, 
							
	HWND win, int lo, int hi, RECT *sep, bool xor, COLORREF bg=-1)
{
	bool out = false; 
	
	if(hi<lo) std::swap(hi,lo);

	if(lo<1000) return false;

	int safe = std::min(256,hi-lo);

	int rop2 = SetROP2(dc,bg==-1?R2_NOT:R2_XORPEN);

	COLORREF contrast = rop2||xor?bg:~bg&0xFFFFFF;

	//Built in hatching sucks, too spacious 
	LOGBRUSH solid = {BS_SOLID,contrast,0}; 
	//LOGBRUSH hatched = {BS_HATCHED,contrast,HS_BDIAGONAL}; 

	HPEN pen = ExtCreatePen(PS_COSMETIC|PS_ALTERNATE,1,&solid,0,0);
	//HPEN pen = ExtCreatePen(PS_GEOMETRIC|PS_ENDCAP_SQUARE,3,&hatched,0,0);

	HGDIOBJ old = SelectObject(dc,pen); 

	POINT nice; MoveToEx(dc,0,0,&nice);

	while(dc) //1
	{
		POINT pts[8];
		//8 sides, 8 points
		//	 t0____r1
		//6___|    |
		//|   7  __|2
		//|_____|3	
		//l5   b4
		
		HWND t0 = GetDlgItem(win,lo), b4 = GetDlgItem(win,hi); 

		for(int i=0;i<safe&&!t0&&++lo!=hi;i++) t0 = GetDlgItem(win,lo);
		for(int i=0;i<safe&&!b4&&--hi!=lo;i++) b4 = GetDlgItem(win,hi);
				
		HWND l5 = t0, r1 = b4; if(!t0||!b4) break;

		RECT t0pos, r1pos, b4pos, l5pos;		
		if(!GetWindowRect(t0,&t0pos)||!GetWindowRect(b4,&b4pos)) break;
		
		if(t0pos.top!=b4pos.top) //Assuming multi-line selection
		{
			HWND backup = t0; 

			for(int i=lo+1;i<hi;i++) //could use a binary lookup here
			{
				if(GetWindowRect(r1=GetDlgItem(win,i),&r1pos))
				{												  
					if(r1pos.top>=b4pos.top||r1pos.left<t0pos.right) break;

					backup = r1;
				}
			}

			GetWindowRect(r1=backup,&r1pos); backup = b4; 

			for(int i=hi-1;i>lo;i--) 
			{
				if(GetWindowRect(l5=GetDlgItem(win,i),&l5pos))
				{												  
					if(l5pos.bottom<=t0pos.bottom||l5pos.right>b4pos.left) break;

					backup = l5;
				}
			}

			GetWindowRect(l5=backup,&l5pos);
		}		
		else //Assuming same line selection
		{
			l5pos = t0pos; r1pos = b4pos;
		}

		if(!Sommctrl_position(&t0pos,dcwin)) break; 
		if(!Sommctrl_position(&r1pos,dcwin)) break; 
		if(!Sommctrl_position(&b4pos,dcwin)) break; 
		if(!Sommctrl_position(&l5pos,dcwin)) break; 

		//Reminder: if xor is not working you will see
		//something like this:		________________
		//  _______________________|________________|
		// |______________|
		//				   //Not a huge deal (I guess)
		////////////////////

		pts[0].x = t0pos.left;  pts[0].y = t0pos.top;
		pts[1].x = r1pos.right; pts[1].y = t0pos.top;
		pts[2].x = r1pos.right; pts[2].y = b4pos.top;
		pts[3].x = b4pos.right; pts[3].y = b4pos.top;
		pts[4].x = b4pos.right; pts[4].y = b4pos.bottom;
		pts[5].x = l5pos.left;  pts[5].y = b4pos.bottom;
		pts[6].x = l5pos.left;  pts[6].y = t0pos.bottom;
		pts[7].x = t0pos.left;  pts[7].y = t0pos.bottom;

		if(sep) for(int i=0;i<4;i++)
		{
			pts[(5+i)%8].x+=sep->left; pts[(1+i)%8].x+=sep->right;
			pts[(0+i)%8].y+=sep->top; pts[(4+i)%8].y+=sep->bottom;
		}

		MoveToEx(dc,pts[7].x,pts[7].y,0);
		for(int i=0;i<8;i++) LineTo(dc,pts[i].x,pts[i].y);		
		out = true;

		break;
	}
	MoveToEx(dc,nice.x,nice.y,0);
		
	if(rop2) SetROP2(dc,rop2);

	SelectObject(dc,old);
	DeleteObject(pen);

	return out;
}