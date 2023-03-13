
#include "Sompaste.pch.h"

#include <commdlg.h> //ChooseColor

static HWND SomcolorProcActive = 0;
static COLORREF SomcolorProcRef = 0;
static HWND SomcolorProcSaveReset = 0;
static POINT SomcolorProcSavePoint = {CW_USEDEFAULT,30};
static UINT_PTR CALLBACK SomcolorProc(HWND hdlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	typedef int(*SomwindowsColorProc)(HWND,COLORREF*,bool);

	switch(Msg) 
	{
	case WM_INITDIALOG: 
	{	
		SomcolorProcActive = hdlg;
			
		SetWindowLongPtr(hdlg,GWLP_USERDATA,lParam);

		CHOOSECOLORW &cc = *(CHOOSECOLORW*)lParam;
		
		Sword_of_Moonlight_Client_Library_Center(hdlg,0,0);

		if(cc.lpTemplateName)
		{		
			wchar_t title[64]; 
			_snwprintf(title,63,L"Choose %s",cc.lpTemplateName); 
			title[7] = toupper(title[7]);		
			title[63] = 0; 

			SetWindowTextW(hdlg,title);
		}
		else SetWindowTextW(hdlg,L"Choose Color");

		if(cc.lCustData) //modeless
		{
			SomcolorProcRef = cc.rgbResult; 
			SendMessage(hdlg,WM_TIMER,'rgb',0);
		}					
		
		return TRUE;		
	}
	case WM_TIMER:
	{	
		HWND Color = GetDlgItem(hdlg,0x2C5); HDC dc = GetDC(Color);

		COLORREF sample = GetPixel(dc,1,1); ReleaseDC(Color,dc);

		CHOOSECOLORW &cc = *(CHOOSECOLORW*)GetWindowLongPtr(hdlg,GWLP_USERDATA);

		if(sample!=SomcolorProcRef) 
		{
			if(sample!=CLR_INVALID) SomcolorProcRef = sample;

			SOMPASTE_LIB(cccb) proc = (SOMPASTE_LIB(cccb))cc.lCustData;

			int tm = proc(hdlg,cc.hInstance,(void**)&SomcolorProcRef,1,(wchar_t*)cc.lpTemplateName);

			if(tm) SetTimer(hdlg,'rgb',tm,0); else KillTimer(hdlg,'rgb');
		}
		break;
	}
	case WM_DESTROY: KillTimer(hdlg,'rgb');
	
		//SomcolorProcActive = 0;

		//DestroyWindow(hdlg); 

		break;
	}

	return 0;
}

static COLORREF Somcolor_customs[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static COLORREF WINAPI SomcolorProcEntryPoint(LPVOID arg)
{	
	CHOOSECOLORW &cc = *(CHOOSECOLORW*)arg; 
	
	COLORREF c[16]; memcpy(cc.lpCustColors=c,Somcolor_customs,sizeof(c));

	if(c[15]!=cc.rgbResult)
	{
		for(int i=8;i<15;i++) c[i] = c[i+1]; c[15] = cc.rgbResult;
	}			 
	if(SomcolorProcActive)  //IsWindow
	{	
		PostMessage(SomcolorProcActive,WM_COMMAND,IDOK,0);
		SomcolorProcActive = 0;
		
		if(c[7]!=SomcolorProcRef)
		{
			for(int i=0;i<7;i++) c[i] = c[i+1];	c[7] = SomcolorProcRef;
		}
	}

	memcpy(Somcolor_customs,c,sizeof(c)); ChooseColorW(&cc);

	if(cc.lCustData) //modeless
	{
		SOMPASTE_LIB(cccb) proc = (SOMPASTE_LIB(cccb))cc.lCustData;

		proc(0,cc.hInstance,(void**)&cc.rgbResult,0,(wchar_t*)cc.lpTemplateName);
	}

	COLORREF out = cc.rgbResult; 
	
	if(cc.lCustData) delete &cc; //modeless

	return out; 
}

extern COLORREF Somcolor(wchar_t *args, HWND window, SOMPASTE_LIB(cccb) proc)
{				
	COLORREF out = -1; //CLR_INVALID 

	if(proc) proc(0,window,(void**)&out,1,args); 	

	if(out==-1) out = 0x808080; //hack
		
	CHOOSECOLORW cc =
	{
		sizeof(cc),proc?0:window,window,out,Somcolor_customs,
		CC_ENABLEHOOK|CC_ANYCOLOR|CC_RGBINIT|CC_FULLOPEN|CC_SOLIDCOLOR,
		(LPARAM)proc,SomcolorProc,args 
	};

	if(!proc) return SomcolorProcEntryPoint(&cc); //thread modal

	CHOOSECOLORW *tls = new CHOOSECOLORW; memcpy(tls,&cc,sizeof(cc));

	//TODO: REALLY THE CALLBACK SHOULD BE SENT ON THE SAME THREAD
	//AS THE CALLER. 
	//Somplayer is having issues because SetWindowSubclass refuses
	//to cross threads. Window procedures should stay on the thread.

	CreateThread(0,0,SomcolorProcEntryPoint,tls,0,0); 
	
	return out; //modeless
}