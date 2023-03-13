
#include "Ex.h" 
EX_TRANSLATION_UNIT

#include <vector>
#include <unordered_map>

#include "dx.ddraw.h"

#include "Ex.detours.h" //hack?

#include "som.game.h"
#include "som.state.h"
#include "SomEx.res.h" //IDD_PAUSE_S

namespace EX
{
	extern bool &f12; //Ex.output.h
	extern HWND client,display(); //Ex.window.h	
}

static struct 
{
	HANDLE gif;
	LOGFONT titleFont;
	wchar_t title[MAX_PATH];
	bool foverlay, lessfx, aniGIF; 

	//dialog usage only	
	int type; bool time,utc, titled;
	wchar_t path[MAX_PATH], name[MAX_PATH]; 

}som_record = {0,{0},{0},0,0,1,-1,1,0,1,{0},{0}};
static UINT_PTR CALLBACK som_record_dummyCFHookProc(HWND dlg,UINT msg,WPARAM,LPARAM lp)
{
	if(msg==WM_INITDIALOG) //pretty
	{
		RECT wr; GetWindowRect((HWND)((CHOOSEFONTW*)lp)->lCustData,&wr);		
		//SetWindowLong(dlg,GWL_STYLE,~(WS_CAPTION|WS_SYSMENU)&GetWindowStyle(dlg));
		SetWindowPos(dlg,0,wr.left,wr.bottom,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);
	}
	return 0; 
}
static INT_PTR CALLBACK som_record_proc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	auto &rec = som_record;
	wchar_t (&path)[MAX_PATH] = rec.path; //chstk
	static const wchar_t *timef = L"%04d.%02d.%02d-%02d.%02d.%02d";

	switch(uMsg)
	{
	case WM_INITDIALOG:
	{			 		
		//reminder: hard coding this order
		HWND cb = GetDlgItem(hwndDlg,1024);
		ComboBox_AddString(cb,L"Animated GIF");
		ComboBox_AddString(cb,L"To Clipboard");				
		ComboBox_AddString(cb,L"Print Screen");
		ComboBox_AddString(cb,L"Print Window");		
		if(rec.type==-1) //initializing
		{
			const wchar_t *record = 
			L"record", *ini = SOM::Game::title('.ini');
			rec.type = GetPrivateProfileInt(record,L"type",-1,ini);
			if(rec.type<0)
			{
				//1: let GIF mode be a curiosity				
				rec.type = 1; //0
				SHGetFolderPath(0,CSIDL_MYPICTURES,0,0,rec.path);
				wcscpy_s(rec.title,SOM::Game::title());	
				if((GetVersion()&0xFF)<6) //XP?
				GetObjectW(GetWindowFont(hwndDlg),sizeof(rec.titleFont),&rec.titleFont);
				else //just like these settings
				{					
					rec.titleFont.lfHeight = -19;									
					rec.titleFont.lfWeight = 400;
					wcscpy(rec.titleFont.lfFaceName,L"Segoe Script");										
				}
				rec.titleFont.lfCharSet = 0;
			}
			else //[record]
			{
				memset(&rec.titleFont,0x00,sizeof(rec.titleFont));
				int time = GetPrivateProfileInt(record,L"time",0,ini);
				rec.time = time==1; rec.utc = time==2;
				GetPrivateProfileString(record,L"dirName",0,rec.path,MAX_PATH,ini);
				GetPrivateProfileString(record,L"fileName",0,rec.name,MAX_PATH,ini);								
				rec.foverlay = GetPrivateProfileInt(record,L"fOverlay",0,ini);
				rec.lessfx = GetPrivateProfileInt(record,L"recMaster",0,ini);
				rec.titled = GetPrivateProfileInt(record,L"title",0,ini);
				GetPrivateProfileString(record,L"titleName",0,rec.title,MAX_PATH,ini);
				GetPrivateProfileString(record,L"titleFont",0,rec.titleFont.lfFaceName,MAX_PATH,ini);
				rec.titleFont.lfHeight = GetPrivateProfileInt(record,L"titleHeight",0,ini);
				rec.titleFont.lfWeight = GetPrivateProfileInt(record,L"titleWeight",0,ini);
				rec.titleFont.lfItalic = GetPrivateProfileInt(record,L"titleItalic",0,ini);
				rec.aniGIF = GetPrivateProfileInt(record,L"aniGIF",0,ini);				
			}
		}
		ComboBox_SetCurSel(cb,rec.type);
		SendMessage(hwndDlg,WM_COMMAND,MAKEWPARAM(1024,CBN_SELENDOK),(LPARAM)cb);
		SetDlgItemText(hwndDlg,1008,rec.path);
		SetDlgItemText(hwndDlg,1007,rec.name);		
		SetDlgItemText(hwndDlg,1025,rec.title);		
		cb = GetDlgItem(hwndDlg,1200); 
		Button_SetCheck(cb,rec.lessfx);		
		cb = GetDlgItem(hwndDlg,1201); 
		Button_SetCheck(cb,rec.foverlay);
		cb = GetDlgItem(hwndDlg,1202); 
		Button_SetCheck(cb,rec.titled);
		SendMessage(hwndDlg,WM_COMMAND,1202,(LPARAM)cb);
		cb = GetDlgItem(hwndDlg,1203); 
		Button_SetCheck(cb,rec.aniGIF);					
		cb = GetDlgItem(hwndDlg,1210);
		Button_SetCheck(cb,rec.time); if(rec.time)
		SendMessage(hwndDlg,WM_COMMAND,1210,(LPARAM)cb);
		cb = GetDlgItem(hwndDlg,1211);
		Button_SetCheck(cb,rec.utc); if(rec.utc) 
		SendMessage(hwndDlg,WM_COMMAND,1211,(LPARAM)cb);	
													
		//scroll will not combine with control
		if(GetKeyState(VK_SCROLL)>>15) goto ok;
		if(GetKeyState(VK_CONTROL)>>15) goto ok;

		//stealing default button
		if(IsWindowEnabled((HWND)wParam))
		return 1;
		SetFocus(GetDlgItem(hwndDlg,IDOK));
		return 0;
	}
	case WM_COMMAND:

		switch(wParam)
		{
		case 1009: //browse
		{
			extern LPITEMIDLIST STDAPICALLTYPE 
			som_tool_SHBrowseForFolderA(LPBROWSEINFOA);
			BROWSEINFOA a = {hwndDlg,0,0,0,0,0,0,0};			
			GetDlgItemText(hwndDlg,1008,path,MAX_PATH);
			SHParseDisplayName(path,0,(PIDLIST_ABSOLUTE*)&a.pidlRoot,0,0);		
			PIDLIST_ABSOLUTE ok = som_tool_SHBrowseForFolderA(&a); if(ok)
			{				
				*path = '\0'; SHGetPathFromIDList(ok,path);
				if(*path) SetDlgItemText(hwndDlg,1008,path);
				ILFree(ok);
			}
			ILFree((PIDLIST_ABSOLUTE)a.pidlRoot);
			break;	   
		}
		case 1202: //title
		{
			rec.titled = Button_GetCheck((HWND)lParam);			
			EnableWindow(GetDlgItem(hwndDlg,1203),rec.titled&&rec.type==0);
			EnableWindow(GetDlgItem(hwndDlg,1025),rec.titled&&rec.type<=1);
			break;
		}
		case 1210: case 1211: //time checkboxes
		{
			BOOL check = Button_GetCheck((HWND)lParam);
			EnableWindow(GetDlgItem(hwndDlg,1007),!check);			
			CheckDlgButton(hwndDlg,wParam^1,0);
			if(!check) break;
			SYSTEMTIME st; (wParam&1?GetSystemTime:GetLocalTime)(&st); 
			wcscpy(path,L"Ex. "); swprintf_s(path+4,MAX_PATH-4,
			timef,st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);			
			SetDlgItemText(hwndDlg,1007,path);
			break;
		}
		case MAKEWPARAM(1024,CBN_SELENDOK): 
		{
			rec.type = ComboBox_GetCurSel((HWND)lParam);		 	
			BOOL gif = !rec.type; HWND x[2] = //time/UTC
			{GetDlgItem(hwndDlg,1210),GetDlgItem(hwndDlg,1211)};	
			EnableWindow(x[0],gif); EnableWindow(x[1],gif);
			EnableWindow(GetDlgItem(hwndDlg,1007),gif 
			&&!Button_GetCheck(x[0])&&!Button_GetCheck(x[1]));			
			EnableWindow(GetDlgItem(hwndDlg,1008),gif);	
			EnableWindow(GetDlgItem(hwndDlg,1202),rec.type<=1); 
			EnableWindow(GetDlgItem(hwndDlg,1203),gif&&rec.titled);	
			EnableWindow(GetDlgItem(hwndDlg,1025),rec.type<=1&&rec.titled);	
			break;
		}
		case MAKEWPARAM(1025,CBN_DROPDOWN): 
		{
			COMBOBOXINFO cbi = {sizeof(cbi)};
			if(GetComboBoxInfo((HWND)lParam,&cbi))
			SetWindowLong(cbi.hwndList,GWL_STYLE,0);
			PostMessage((HWND)lParam,CB_SHOWDROPDOWN,0,0);
			break;
		}
		case MAKEWPARAM(1025,CBN_CLOSEUP): 
		{
			//SCREENFONTS: required or XP says there are no fonts
			//ENABLEHOOK: force old style dialog (crashing on Windows 7/10!?)
			//CF_NOSTYLESEL
			CHOOSEFONTW cf = {sizeof(cf),hwndDlg,0,&rec.titleFont,0,CF_ENABLEHOOK|CF_SCREENFONTS|CF_NOVERTFONTS|CF_INITTOLOGFONTSTRUCT};
			cf.lpfnHook = som_record_dummyCFHookProc; cf.lCustData = lParam;
			ChooseFont(&cf); break;	
		}
		case IDOK: ok: 
		{			
			//2020: som_game_4023c0 freezes time for 1 in 60fps mode only
			SOM::recording = 1; rec.gif = 0;
			if(!SendDlgItemMessage(hwndDlg,1024,CB_GETCURSEL,0,0))
			{
				SOM::recording = max(DDRAW::dejagrate,1);
				int cat = GetDlgItemText(hwndDlg,1008,path,MAX_PATH);
				path[cat++] = '\\';
				HWND x[2] = {GetDlgItem(hwndDlg,1210),GetDlgItem(hwndDlg,1211)};
				if(Button_GetCheck(x[0])||Button_GetCheck(x[1]))
				{
					SYSTEMTIME st; (Button_GetCheck(x[1])?GetSystemTime:GetLocalTime)(&st); 
					cat+=swprintf_s(path+cat,MAX_PATH-cat,timef,st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);			
				}
				else cat+=GetDlgItemText(hwndDlg,1007,path,MAX_PATH-cat);				
				wcscpy_s(path+cat,MAX_PATH-cat,L".gif"); 
				rec.gif = CreateFile(path,GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0);
				if(rec.gif==INVALID_HANDLE_VALUE)
				{
					//2020: don't kill over a misinputted path?
					//EX::ok_generic_failure(L"Access Denied: %s",path);
					wchar_t msg[30+MAX_PATH]; swprintf(msg,L"Unable to write to %s",path);
					MessageBoxW(0,msg,L"Access Denied",MB_OK);

					SOM::recording = 0; break;
				}
				else assert(rec.gif);
			}
			//get it out of the screenshot
			MoveWindow(hwndDlg,0,0,0,0,1); 
			//break;
		}
		case IDCANCEL: goto close;
		}
		break;
	case WM_CLOSE: close:

		//saving for reopening/recording
		//TODO? save to registry somehow
		GetDlgItemText(hwndDlg,1007,rec.name,MAX_PATH);
		GetDlgItemText(hwndDlg,1008,rec.path,MAX_PATH);
		rec.type = 
		SendDlgItemMessage(hwndDlg,1024,CB_GETCURSEL,0,0);
		rec.lessfx = IsDlgButtonChecked(hwndDlg,1200);
		rec.foverlay = IsDlgButtonChecked(hwndDlg,1201);		
		rec.titled = IsDlgButtonChecked(hwndDlg,1202);
		GetDlgItemText(hwndDlg,1025,rec.title,MAX_PATH);
		rec.aniGIF = IsDlgButtonChecked(hwndDlg,1203);  
		rec.time = IsDlgButtonChecked(hwndDlg,1210);  
		rec.utc = IsDlgButtonChecked(hwndDlg,1211);  

		SetForegroundWindow(GetParent(hwndDlg)); //winsanity
		EndDialog(hwndDlg,SOM::recording);
		break;
	}
	return 0;
}			
extern bool SOM::record_ini()
{
	static bool one_off = false; if(one_off++) return false; //??? 
	if(som_record.type==-1) return true;
	const wchar_t f[] = 
	L"dirName=%ls%lc"
	L"fileName=%ls%lc"L"time=%d%lc"
	L"type=%d%lc"	
	L"fOverlay=%d%lc"L"recMaster=%d%lc"
	L"title=%d%lc"L"titleName=%ls%lc"L"titleFont=%ls%lc"L"titleHeight=%d%lc"
	L"titleWeight=%d%lc"L"titleItalic=%d%lc" //required
	L"aniGIF=%d%lc";	
	wchar_t record[3*MAX_PATH+32+8*33+EX_ARRAYSIZEOF(f)] = L"\0";
	swprintf_s(record,f,
	som_record.path,0,som_record.name,0,
	som_record.time?1:som_record.utc?2:0,0,
	som_record.type,0,
	som_record.foverlay,0,som_record.lessfx,0,
	som_record.titled,0,som_record.title,0,
	som_record.titleFont.lfFaceName,0,som_record.titleFont.lfHeight,0,
	som_record.titleFont.lfWeight,0,som_record.titleFont.lfItalic,0,
	som_record.aniGIF,0);
	WritePrivateProfileSection(L"record",record,SOM::Game::title('.ini'));
	return true;
}

static bool som_record_onflip();
static void som_record_aniGIF(HBITMAP);
static bool (*som_record_onflip_passthru)() = 0;
extern bool SOM::record()
{		
	if(SOM::recording) return(MessageBeep(-1),false);

	assert(SOM::paused);   

	//hack? disabling Detours!
	EX::cleaning_up_Detours();
	DialogBox(EX::module,MAKEINTRESOURCE(IDD_PAUSE_S),EX::display(),som_record_proc);
	EX::setting_up_Detours();

	if(SOM::recording)
	if(!som_record_onflip_passthru) //install
	{	
		som_record_onflip_passthru = DDRAW::onFlip;
		DDRAW::onFlip = som_record_onflip;		
	}
	return SOM::recording;
}

static bool som_record_onflip()
{
	bool out = som_record_onflip_passthru(); 	

	if(!SOM::recording) return out; 
														 
	const int wait = 2+0;

	static struct //pre-recording state
	{
		int waiting, dimmer,bright, dejag,bike, smooth,f12,fx; 

	}pre = {wait}; if(pre.waiting==wait)
	{
		pre.f12 = EX::f12; 
		EX::f12 = som_record.foverlay;
		pre.dimmer = DDRAW::dimmer; 
		pre.bright = DDRAW::bright;		
		pre.smooth = DDRAW::doSmooth; 
		if(som_record.lessfx) DDRAW::doSmooth = false;
		if(som_record.lessfx) DDRAW::dimmer = DDRAW::bright = 0;		
		pre.fx = DDRAW::toggleEffectsBuffer(!som_record.lessfx);
		pre.dejag = DDRAW::dejagrate; 
		//probably better overall to not do this???
		//if(!som_record.gif) DDRAW::dejagrate = 0;
		pre.bike = DDRAW::fxStrobe; DDRAW::fxStrobe = 0; 	
	}
	//skipping first PAUSED frame/waiting for GPU to finish
	if(0<=--pre.waiting) return out; 

	//PREVENT SYNCHRONIZATION ISSUSE??	
	//(sometimes 3's window frame does not appear)
	//(maybe a good solution for triple buffering)
	EX::sleep(33); 
	
	HWND dtw = GetDesktopWindow();
	HWND win = 0; switch(som_record.type)
	{
	default: win = EX::client; break;		
	case 2: win = dtw; break;
	case 3: win = EX::display(); 
	}
	int x = 0, y = 0;	
	//D3DSWAPEFFECT_FLIPEX can only grab from the desktop
	//doing it this way is not bad, just counterintuitive
	//HDC src = GetWindowDC(win);	 
	HDC src = GetWindowDC(dtw);
	HDC dst = CreateCompatibleDC(src);	
	RECT rc; GetWindowRect(win,&rc);		
	if(3==som_record.type) //print window/DWM
	{
		RECT efb = rc; void *p = GetProcAddress
		(GetModuleHandle(L"Dwmapi.dll"),"DwmGetWindowAttribute");
		//9: DWMWA_EXTENDED_FRAME_BOUNDS
		if(p&&!((HRESULT(__stdcall*)(HWND,DWORD,PVOID,DWORD))p)(win,9,&efb,sizeof(RECT)))
		{
			//NEW: for D3DSWAPEFFECT_FLIPEX support
			//x = efb.left-rc.left; y = efb.top-rc.top; 
			rc = efb;
		}
	}			
	//NEW: for D3DSWAPEFFECT_FLIPEX support
	{
		//NOTE: clipping is required with or without FLIPEX
		RECT clip; GetWindowRect(dtw,&clip);
		IntersectRect(&clip,&rc,&clip);
		x = rc.left; y = rc.top;		
		rc = clip;
	}
	OffsetRect(&rc,-rc.left,-rc.top);

	LONG title = 0;
	if(som_record.type<2&&som_record.titled)
	title = abs(som_record.titleFont.lfHeight)*8/6;

	static HBITMAP bm = 0; if(!bm)
	{	
		if(som_record.gif)
		{	
			BITMAPINFO bmi = //-/32: som_record_gif requirements
			{{sizeof(bmi),rc.right,-rc.bottom-title,1,32,0,0,0,0,0,0}};			
			bm = CreateDIBSection(dst,&bmi,DIB_RGB_COLORS,0,0,0);
		}
		else bm = CreateCompatibleBitmap(src,rc.right,rc.bottom+title);

		if(title)
		{
			HGDIOBJ old = SelectObject(dst,bm);
			HFONT f = CreateFontIndirect(&som_record.titleFont);
			RECT tr = rc; tr.left+=title/5;
			tr.top = rc.bottom; tr.bottom = tr.top+title;
			SelectObject(dst,f); SetBkColor(dst,0); SetTextColor(dst,0xFFFFFF);
			DrawText(dst,som_record.title,-1,&tr,DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
			if(som_record.gif&&som_record.aniGIF)
			DrawText(dst,L"60FPS AniGIF ",-1,&tr,DT_VCENTER|DT_RIGHT|DT_SINGLELINE);   			
			DeleteObject(f); SelectObject(dst,old); 
		}
	}
	bool clipboard = !som_record.gif&&SOM::recording==1; 
	HGDIOBJ old = SelectObject(dst,bm); assert(bm&&old);		
	BitBlt(dst,0,0,rc.right,rc.bottom,src,x,y,SRCCOPY|CAPTUREBLT);			
	
	if(!clipboard) //GIF
	{
		som_record_aniGIF(bm);	
	}
	else if(clipboard=OpenClipboard(win))
	{				
		EmptyClipboard();
		clipboard = SetClipboardData(CF_BITMAP,bm);
		CloseClipboard();
	}

	SelectObject(dst,old); DeleteDC(dst); 
	ReleaseDC(win,src); 

	if(--SOM::recording<=0) //cleanup
	{
		SOM::recording = 0;

		if(!clipboard) DeleteBitmap(bm); bm = 0;
		 
		pre.waiting = wait;
		DDRAW::dimmer = pre.dimmer;
		DDRAW::bright = pre.bright;
		DDRAW::doSmooth = pre.smooth;
		EX::f12 = pre.f12; DDRAW::toggleEffectsBuffer(pre.fx);
		DDRAW::dejagrate = pre.dejag; DDRAW::fxStrobe = pre.bike;
		if(som_record.gif) CloseHandle(som_record.gif); 
		som_record.gif = 0;
	}	

	return out;
}
namespace som_record_gif
{
	//GIF flavored LZW based on:
	//rosettacode.org/wiki/LZW_compression#C		
	static std::vector<char> encoded;	
	template<class T> static size_t encode(T in)
	{	
		//2-color must use 2
		const int colorbits = 8; 		
		enum{cc=2<<(colorbits-1),eoi,c0}; 
		auto &out = encoded; out.clear();	 		
		
		int next_code = c0;
		int next_shift = 2<<colorbits;				
		//encode_t dictionary[4096];
		struct encode_t{ short next[256]; };
		static std::vector<encode_t> dictionary;
		dictionary.resize(next_shift); 	

		int len = in.size();		
		int o = 0, o_bits = 0;		
		int c_bits = colorbits+1;
		unsigned c,nc, code = cc; do
		{
			c = *in; ++in; //c holds a byte
			if(!(nc=dictionary[code].next[c])) cc:
			{
				o|=code<<o_bits;
				for(o_bits+=c_bits;o_bits>=8;o>>=8)
				{
					o_bits-=8; out.push_back(o&0xFF);					
				}	 
				if(next_code>=next_shift) 
				{
					if(++c_bits>12) //12 is GIF's limit
					{	
						c_bits = 12; //13? spec is unclear

						next_code = c0;	
						next_shift = 2<<colorbits;							
						dictionary.clear();
						dictionary.resize(next_shift); 
						code = cc; goto cc;
					} 
					else dictionary.resize(next_shift*=2); 
				}
				if(code!=cc)
				nc = dictionary[code].next[c] = next_code++;									
				else if(c_bits>=12) c_bits = colorbits+1;
				code = c;
			}
			else code = nc;
			
		}while(--len>0); switch(len)
		{
		case  0: goto cc; //truncate
		case -1: code = eoi; goto cc; 
		case -2: c_bits = 8; if(o_bits) goto cc;
		}
		dictionary.clear();
		return out.size();
	}

	//these work as a team
	static std::unordered_map<int,int> colors; 
	//
	typedef union //color 
	{
		DWORD rgbx; struct{ BYTE r,g,b,x; };

		void operator=(int &i) //GIF colors
		{
			if(!i) i = colors.size(); x = i-1;
		}
		void operator=(const int &i) //GIF palettes
		{
			r = i>>16&0xFF; g = i>>8&0xFF; b = i&0xFF;
		}

	}color;
}
static void som_record_aniGIF(HBITMAP hbm) //GIF
{
	using namespace som_record_gif; //colors

	BITMAP bm; 
	if(!GetObject(hbm,sizeof(bm),&bm)||bm.bmWidthBytes!=4*bm.bmWidth)
	{
		EX::ok_generic_failure(L"Unsupported System Bitmap Profile");
		SOM::recording = 0; return;
	}
	
	#pragma pack(push,1) 
	struct fileheader
	{
		CHAR GIF89a[6];
		WORD width,height;
		BYTE x700000[3]; 
		//loop extension
		BYTE x21FF0B[3];
		CHAR NETSCAPE2_0[11];
		BYTE x0301loop00[5];

	}fh = { //infinite loop
	"GIF89"/*a*/,bm.bmWidth,bm.bmHeight,{0x70,0,0},
	{33,0xFF,11},"NETSCAPE2."/*0*/,{3,1,0,0,'\0'}};	
	fh.GIF89a[5] = 'a'; fh.NETSCAPE2_0[10] = '0';	
	struct graphicontrol
	{	
		BYTE x21F904[3];
		BYTE tc:1,ui:1,dispsal:1;
		WORD delay; //50~60FPS
		BYTE x0000[2];

	}gc = {{33,0xF9,4},0,0,1,2,0,'\0'};
	struct tileheader
	{
		CHAR x2C;
		WORD x,y,width,height;
		//must colorsize match the index size?
		BYTE colorsize:3,x0:4,x1:1;
		BYTE colors[256][3];
		BYTE lzwcodesize;

	}th = {0x2C,0,0,0,0,7,0,1};
	th.lzwcodesize = 8;
	#pragma pack(push,1)

	DWORD wr; //write header
	if(!SetFilePointer(som_record.gif,0,0,FILE_CURRENT))
	WriteFile(som_record.gif,&fh,sizeof(fh),&wr,0);		
	
	struct scale : RECT
	{	
		POINT ptsofinterest[2]; bool ofinterest; 

		scale(tileheader &th) : ofinterest(true)
		{
			left = th.x; right = left+th.width;
			top = th.y; bottom = top+th.height;
			ptsofinterest[0].x = left; ptsofinterest[1].x = right; 
			ptsofinterest[0].y = bottom; ptsofinterest[1].y = top;
		}
	};
	static std::vector<scale> scales;  
	
	enum{ hz0=0,hz50=2,hz30 }framerate = hz50;

	BYTE tmp; gc.delay = hz0; do 
	{	
		//DETERMINE DIMENSIONS BY BUILDING COLOR TABLE

		WORD width = bm.bmWidth-th.x;
		WORD height = bm.bmHeight-th.y;
		BYTE *pos = (BYTE*)bm.bmBits+th.x*4+th.y*bm.bmWidthBytes;		
		th.width = min(16,width);
		th.height = min(16,height);
		{
			color *p = (color*)pos;
			for(int i=0;i<th.height;i++,p+=bm.bmWidth)
			for(int i=0;i<th.width;i++)	p[i] = colors[0xFFFFFF&p[i].rgbx];
		}
		for(int alt=1;colors.size()<=256;alt=!alt)
		{
			if(alt) if(th.width<width)
			{
				color *p = (color*)pos+th.width;
				for(int i=0;i<th.height;i++,p+=bm.bmWidth)
				*p = colors[0xFFFFFF&p->rgbx];					
				if(colors.size()<=256&&++th.width>=width)
				if(th.height>=height) break;
			}
			else alt = 0; if(!alt) if(th.height<height)
			{
				color *p = (color*)pos+th.height*bm.bmWidth;				
				for(int i=0;i<th.width;i++)
				p[i] = colors[0xFFFFFF&p[i].rgbx];
				if(colors.size()<=256&&++th.height>=height)
				if(th.width>=width) break;
			}
		}
		for(auto it=colors.begin();it!=colors.end();it++) 
		if(it->second<=256)	(color&)*th.colors[it->second-1] = it->first;	
		
		//BACKUP A COPY OF THE CURRENT TILE FOR LATER ENCODING

		struct tiledata
		{
			BYTE *pos,*wrapos;
			size_t width,height,pitch;
			inline void operator++()
			{
				if((pos+=4)<wrapos) return;
				pos = pos-width*4+pitch; wrapos = pos+width*4;
			}
			inline BYTE operator*(){ return pos[3]; }	
			inline size_t size(){ return width*height; }

		}td = {pos,pos+th.width*4,th.width,th.height,bm.bmWidthBytes};		
				
		//DISCOVER NEXT TILE (SO WE KNOW IF THIS IS THE LAST TILE)

		scales.push_back(th);		
		scale &s = scales.back();
		size_t reuse = scales.size();
		if(s.right>=bm.bmWidth)	s.ptsofinterest[1].y = s.bottom;
		if(s.bottom>=bm.bmHeight) s.ptsofinterest[0].x = s.right;
		size_t i; LONG x = bm.bmWidth, y = bm.bmHeight;
		for(i=0;i<scales.size();i++) if(scales[i].ofinterest)
		{
			scale &t = scales[i];			
			POINT *pp = s.ptsofinterest; 
			if(PtInRect(&t,pp[0])) pp[0].x = max(t.right,pp[0].x);
			if(PtInRect(&t,pp[1])) pp[1].y = max(t.bottom,pp[1].y);
			pp = t.ptsofinterest;
			if(PtInRect(&s,pp[0])) pp[0].x = max(s.right,pp[0].x);
			if(PtInRect(&s,pp[1])) pp[1].y = max(s.bottom,pp[1].y);
			if(pp[1].y<t.bottom)
			{
				if(pp[1].y<y) //if(pp[1].x<x)
				{
					x = pp[1].x; y = pp[1].y;
				}
			}
			else t.ofinterest = false;

			if(pp[0].x<t.right)
			{
				t.ofinterest = true; //!!

				if(pp[0].x<x) if(pp[0].y<y)
				{
					x = pp[0].x; y = pp[0].y;
				}		   
			}
		} 
		else reuse = i; if(reuse!=i) //optimizing
		{
			scales[reuse] = s; scales.pop_back();
		}			

		//FINAL SUB-IMAGE BLOCK?
		//IF SO INSERT AN ANIMATION DELAY
		if(x>=bm.bmWidth&&y>=bm.bmHeight)
		gc.delay = framerate;

		//WRITE THIS TILE TO FILE
		WriteFile(som_record.gif,&gc,sizeof(graphicontrol),&wr,0);
		WriteFile(som_record.gif,&th,sizeof(th),&wr,0);
		size_t rem = encode(td); for(i=0;i+255<rem;i+=255)
		{
			WriteFile(som_record.gif,&(tmp=255),1,&wr,0);
			WriteFile(som_record.gif,&encoded[i],255,&wr,0);
		}
		if(i<rem) WriteFile(som_record.gif,&(tmp=rem-i),1,&wr,0);
		if(i<rem) WriteFile(som_record.gif,&encoded[i],rem-i,&wr,0);
		WriteFile(som_record.gif,&(tmp=0),1,&wr,0);	 

	colors.clear();	
	th.x = x; th.y = y;
	}while(gc.delay==hz0);
	scales.clear();	
	if(SOM::recording<=1)
	WriteFile(som_record.gif,&(tmp=';'),1,&wr,0);
}