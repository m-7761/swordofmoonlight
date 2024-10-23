	
#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

#include "som.tool.h" 

#pragma optimize("",off) //2024
							
extern HWND &som_tool;
extern char som_tool_play;

//PLAY button. This based on SOM_MAIN_updownproc.
static LRESULT CALLBACK SOM_SYS_updownproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scID, DWORD_PTR)
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
		int hid = GetWindowContextHelpId(gp);
		if(uMsg==WM_LBUTTONDOWN) 
		{
			switch(id+hid)
			{
			case 1080+56200: //play

				hl = 1005; break;
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
						//MessageBeep: SOM_SYS_buttonproc
						if(uMsg&1) return MessageBeep(-1); 
						//prevent release
						lParam = cr.right*4; 
					}					
					break; 
				}
			}
		}
		if(uMsg==WM_LBUTTONDOWN)
		if(hl) 
		{
			extern void som_tool_highlight(HWND,int=0);
			som_tool_highlight(gp,hl);
		}
		if(uMsg==WM_LBUTTONUP&&hWnd==GetCapture()) 
		{	
			RECT cr; GetClientRect(hWnd,&cr); 
			POINT pt = {GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
			InflateRect(&cr,3,3); //whatever works
			if(!PtInRect(&cr,pt)) break;
		
			switch(id+hid)
			{
			case 1080+56200: //play

				SendMessage(gp,WM_COMMAND,id,0); 
			}
		}
		break;
	}
	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,SOM_SYS_updownproc,scID);
		break;
	}					  	
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}     
static VOID CALLBACK SOM_SYS_control(HWND win, UINT, UINT_PTR id, DWORD)
{
	if(~GetKeyState(id)>>15) KillTimer(win,id);	else return;
	
	//send back thru SOM_MAIN_buttonproc to pick which side to push
	if(GetKeyState(VK_CONTROL)>>15) SendMessage(win,WM_KEYUP,id,0);
}
static LRESULT CALLBACK SOM_SYS_buttonproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scID, DWORD_PTR)
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
		case 1005+56200: //split-button ticker
			
			if(r||l||space) ud = 1080; break; //play
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
		SetTimer(hWnd,wParam,100,SOM_SYS_control);
		return 0;
	}
	else break;
	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,SOM_SYS_buttonproc,scID);
		break;
	}					  	
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
extern void SOM_SYS_buttonup(HWND dlg) //56200 (145)
{
	//Tie horizontal updowns to their buttons.
	HWND play = GetDlgItem(dlg,1080);	
	extern ATOM som_tool_updown(HWND=0);
	if(GetClassAtom(play)==som_tool_updown())
	{
		SetWindowSubclass(play,SOM_SYS_updownproc,0,0);
		SetWindowSubclass(GetDlgItem(dlg,1005),SOM_SYS_buttonproc,0,0);
	}
}			   

static void __cdecl SOM_SYS_4131c0_play_wav(char *a, int, int)
{
	if(-1==((int(__cdecl*)(char*,int,int))0x4131C0)(a,0,0))
	{
		memcpy(PathFindExtensionA(a)+1,"wav",4);
		((int(__cdecl*)(char*,int,int))0x4131C0)(a,0,0);
	}
}

extern void SOM_SYS_reprogram()
{
	extern void som_state_dword(DWORD,DWORD,DWORD);

	//level up table uses no less than five 0s to pad power ups
	som_state_dword(0x4A464,*(DWORD*)"%05d",*(DWORD*)"%d\0\0");

	//2023: play WAV files
	//00405BE6 E8 D5 D5 00 00       call        004131C0
	*(DWORD*)0x405BE7 = (DWORD)SOM_SYS_4131c0_play_wav-0x405BEb;
}