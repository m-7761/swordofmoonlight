
#include "Ex.h" 
EX_TRANSLATION_UNIT

#include <vector>
#include "detours.h"

//should be a release build
#pragma comment(lib,"detours.lib")

#include "Ex.ini.h"
#include "Ex.output.h"
#include "Ex.window.h"
#include "Ex.fonts.h"
#include "Ex.detours.h" 

#include "dx.ddraw.h"

//REMOVE ME?
#include "som.state.h"

#define EX_DETOURS_CELL_VS_CHAR(h) abs(h) //cell //???

extern void (*EX::detours)(LONG (WINAPI*)(PVOID*,PVOID)) = 0; 

namespace Ex_detours
{						  
	static BOOL (WINAPI *SetWindowPos)(HWND,HWND,int,int,int,int,UINT) = ::SetWindowPos;

	//W: scheduled obsolete
	extern int (WINAPI *DrawTextW)(HDC,LPCWSTR,int,LPRECT,UINT) = ::DrawTextW; //lazy
	static int (WINAPI *DrawTextA)(HDC,LPCSTR,int,LPRECT,UINT) = ::DrawTextA;
	static BOOL (WINAPI *TextOutA)(HDC,int,int,LPCSTR,int) = ::TextOutA;

	static HFONT (WINAPI *CreateFontA)(int,int,int,int,int,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,LPCSTR) = ::CreateFontA;

	static HGDIOBJ (WINAPI *SelectObject)(HDC,HGDIOBJ) = ::SelectObject;

	static BOOL (WINAPI *DeleteObject)(HGDIOBJ) = ::DeleteObject;
}

#define EX_DETOURS_(d) Ex_detours::d //::d 

#define EX_DETOURS_DETOUR_THREADMAIN(f,...) \
	if(GetCurrentThreadId()!=EX::threadmain)\
	{\
		return EX_DETOURS_(f)(__VA_ARGS__);\
	}

static bool Ex_detours_cmp(HWND hWnd, int X, int Y, int cx, int cy, UINT uFlags)
{	
	RECT test; 
	
	if(GetWindowRect(hWnd,&test))
	{
		if(uFlags&SWP_NOMOVE||X==test.left&&Y==test.top)
		{
			int w = test.right-test.left;
			int h = test.bottom-test.top;

			if(uFlags&SWP_NOSIZE||cx==w&&cy==h)
			{
				return false; //warning: ignoring Z order
			}
		}
	}

	return true;
}

static BOOL WINAPI Ex_detours_SetWindowPos(HWND hWnd, HWND after, int X, int Y, int cx, int cy, UINT uFlags)
{
	//Windows 10 has a bug; it injects WS_EX_TOPMOST???
	//2018: This is sent when DDRAW::fullscreen is used
	//assert(after!=HWND_TOPMOST);
	if(after!=HWND_TOPMOST)
	{
		int bp = 1;
	}

	EX_DETOURS_DETOUR_THREADMAIN(SetWindowPos,hWnd,after,X,Y,cx,cy,uFlags)

	EXLOG_LEVEL(7) << "Ex_detours_SetWindowPos()\n";

	if(!EX::client||hWnd!=EX::client) 
	{	
		if(hWnd==EX::window) //fishy: added condition today???
		if(!Ex_detours_cmp(hWnd,X,Y,cx,cy,uFlags)) return 1;

		return EX_DETOURS_(SetWindowPos)(hWnd,after,X,Y,cx,cy,uFlags);
	}

	//EXLOG_LEVEL(7) << "SetWindowPos " << std::dec << X << ", "  << Y << ", " << cx << 'x' << cy << '\n';

	if(SOM::frame-SOM::option>1) 
	if((uFlags&SWP_NOSIZE)==0&&cx==640&&cy==480)
//	if(EX::INI::Window()->do_scale_640x480_modes_to_setting) //401a22 obsoletes this
	{
		cx = SOM::width; cy = SOM::height;
	}

	if((uFlags&SWP_NOSIZE)==0)
	{
		if(!EX::window) EX::capturing_screen();
		int sx = EX::screen.right-EX::screen.left;
		int sy = EX::screen.bottom-EX::screen.top;
		//2019: standardizing on having the margin
		//be the outer window
		//http://www.swordofmoonlight.com/bbs/index.php?topic=1144.msg13710#msg13710
		//if(cx>=sx||cy>=sy)
		if(cx>sx||cy>sy)
		{
			cx = sx; cy = sy;
		}
	}

	//EXLOG_LEVEL(7) << ' ' << X << ", "  << Y << ", " << cx << 'x' << cy << '\n';
				 
	if(EX::window&&(uFlags&SWP_NOSIZE)==0) EX::adjusting_window(cx,cy);

	if(!Ex_detours_cmp(hWnd,X,Y,cx,cy,uFlags)) return 1;

	return EX_DETOURS_(SetWindowPos)(hWnd,after,X,Y,cx,cy,uFlags);
}
	   
static HDC Ex_detours_selected_fonthdc = 0;

static const EX::Font **Ex_detours_selected_d3dfont = 0;
									 
static int WINAPI Ex_detours_DrawTextW(HDC hdc, LPCWSTR wtxt, int wlen, LPRECT box, UINT how) 
{
	EX_DETOURS_DETOUR_THREADMAIN(DrawTextW,hdc,wtxt,wlen,box,how)

 	EXLOG_LEVEL(7) << "Ex_detours_DrawTextW()\n";
		
	//REMOVE ME? (this is really inappropriate)
	if(Ex_detours_selected_d3dfont&&Ex_detours_selected_fonthdc==hdc)
	{
		assert(box); if(!box) return 0;

		if(box) //how&DT_CALCRECT
		{
			//hack: put displaying_output_font in DrawText mode

			if(box->right==box->left)
			{
				if(how&DT_RIGHT) box->left--; else box->right++;
			}

			if(box->top==box->bottom) 
			{
				if(how&DT_BOTTOM) box->top--; else box->bottom++;
			}
		}

		return EX::displaying_output_font
		(Ex_detours_selected_d3dfont,hdc,how|DT_NOPREFIX,*box,wtxt,wlen);
	}	

	return EX_DETOURS_(DrawTextW)(hdc,wtxt,wlen,box,how);
}

static int WINAPI Ex_detours_DrawTextA(HDC hdc, LPCSTR txt, int len, LPRECT pbox, UINT how)
{		
	EX_DETOURS_DETOUR_THREADMAIN(DrawTextA,hdc,txt,len,pbox,how)

 	EXLOG_LEVEL(7) << "Ex_detours_DrawTextA()\n";
	
	RECT box; if(pbox) box = *pbox; //2020

	int clip = 0;
	assert(len>=0);
	if(len=-1) len = strlen(txt);
	RECT *_box = &box;
	int cp = EX::Translate(::DrawTextA,hdc,&txt,&len,&_box,&how,&clip);		
	if(!len||!*txt) return 0;
	
	//2018: doing AFTER EX::Translate instead of BEFORE in order to be
	//constistent with DDRAW::hack_interface/because it's convenient to
	if(pbox) 
	{
		/*DDRAW::ps2ndSceneMipmapping2?
		box.top = DDRAW::xyScaling[1]*box.top+DDRAW::xyMapping[1];
		box.left = DDRAW::xyScaling[0]*box.left+DDRAW::xyMapping[0];		
		box.right = DDRAW::xyScaling[0]*box.right+DDRAW::xyMapping[0];
		box.bottom = DDRAW::xyScaling[1]*box.bottom+DDRAW::xyMapping[1];
		DDRAW::doSuperSamplingMul(box);*/
		DDRAW::xyRemap2(box);
	}		

	const wchar_t *wtxt = L""; //Unicode ouput	
	int wlen = EX::Convert(cp,&txt,&len,&wtxt);

	int ret;
	if(Ex_detours_selected_d3dfont
	 &&Ex_detours_selected_fonthdc==hdc)
	{
		//FYI: SOM::Print has code for generating a default box
		assert(pbox); if(!pbox) return 0;

		if(how&DT_CALCRECT&&pbox) 
		{
			//hack: put displaying_output_font in DrawText mode

			if(box.right==box.left)
			{
				if(how&DT_RIGHT) box.left--; else box.right++;
			}

			if(box.top==box.bottom) 
			{
				if(how&DT_BOTTOM) box.top--; else box.bottom++;
			}
		}

		ret = EX::displaying_output_font
		(Ex_detours_selected_d3dfont,hdc,how|DT_NOPREFIX,box,wtxt,wlen,clip);
	}
	else 
	{
		assert(!clip); //unimplemented	
		
		ret = EX_DETOURS_(DrawTextW)(hdc,wtxt,wlen,pbox?&box:0,how|DT_NOPREFIX); 	
	}

	if(how&DT_CALCRECT&&pbox)
	{
		/*DDRAW::ps2ndSceneMipmapping2?
		//2020: note, Ex_detours_DrawTextA isn't reversing afterward
		box.top = (box.top-DDRAW::xyMapping[1])/DDRAW::xyScaling[1]+0.5f;
		box.left = (box.left-DDRAW::xyMapping[0])/DDRAW::xyScaling[0]+0.5f;
		box.right = (box.right-DDRAW::xyMapping[0])/DDRAW::xyScaling[0]+0.5f;
		box.bottom = (box.bottom-DDRAW::xyMapping[1])/DDRAW::xyScaling[1]+0.5f;
		DDRAW::doSuperSamplingDiv(box);*/
		DDRAW::xyUnmap2(box);

		*pbox = box;
	}

	return ret;
}
namespace SOM{ extern int title_pixels; } //FIX ME
static BOOL WINAPI Ex_detours_TextOutA(HDC hdc, int x, int y, LPCSTR txt, int len) 
{
	EX_DETOURS_DETOUR_THREADMAIN(TextOutA,hdc,x,y,txt,len)

 	EXLOG_LEVEL(7) << "Ex_detours_TextOutA()\n";
		
	int clipx = 0;
	int cp = EX::Translate(::TextOutA,hdc,&x,&y,&txt,&len,&clipx);	
	if(!len||!*txt) return 0;

	//2022: I had to move this out of EX::Translate because it
	//because known to me that TA_CENTER is 4|TA_RIGHT, so that
	//they're not bitwise flags. but it's still not good to do
	//this here... still SOM::title_pixels shouldn't be handled
	//here since this isn't a "som" cide file
	int uncenter = 0;
	if(SOM::title_pixels<0||*txt=='$') //FIX ME
	{
		if(*txt=='$'){ txt++; len--; } //centering?

		uncenter = GetTextAlign(hdc);

		if(TA_CENTER!=(7&uncenter))
		{	
			SetTextAlign(hdc,uncenter&~7|TA_CENTER);

			uncenter = ~uncenter;
		}
		else uncenter = 0;
		
		//x = SOM::width/2;
		x = SOM::fov[0]/DDRAW::xyScaling[0]/2;
	}
	else
	{
		clipx = SOM::title_pixels; //FIX ME

		if(float sf=EX::stereo_font) clipx*=sf; //HACK?
	}

	//2018: doing AFTER EX::Translate instead of BEFORE in order to be
	//constistent with DDRAW::hack_interface/because it's convenient to
	x*=DDRAW::xyScaling[0];
	y*=DDRAW::xyScaling[1];
	
	if(!DDRAW::gl) //???
	{
		x+=DDRAW::xyMapping[0];	
		y+=DDRAW::xyMapping[1];
	}

	DDRAW::doSuperSamplingMul(x,y);
	
	const wchar_t *wtxt = L""; //Unicode ouput
	
	int wlen = EX::Convert(cp,&txt,&len,&wtxt);

	int out; if(1) //support newlines out of EX::Translate
	{
		RECT box = {x,y,x,y}; assert(EX::client);

		UINT dt = 0, ta = GetTextAlign(hdc);

		if(GetClientRect(EX::client,&box)) //assuming single line
		{
			DDRAW::doSuperSamplingMul(box.right,box.bottom);

			box.left = x; box.right-=x; box.top = y; box.bottom-=y;  
		}
		else box.right = box.bottom = 2*4096; //arbitrarily huge number		

		/*2022
		//it turns out TA_CENTER==4|TA_RIGHT
		if(ta&TA_CENTER)	dt|=DT_CENTER;
		if(ta&TA_LEFT)		dt|=DT_LEFT;
		if(ta&TA_RIGHT)		dt|=DT_RIGHT;*/
		switch(ta&7)
		{
		case TA_LEFT: dt|=DT_LEFT; break; //0
		case TA_RIGHT: dt|=DT_RIGHT; break; //2
		case TA_CENTER: dt|=DT_CENTER; break; //6 (2+4)
		}
		if(ta&TA_TOP)		dt|=DT_TOP;		
		if(ta&TA_RTLREADING)dt|=DT_RTLREADING;
		if(ta&TA_UPDATECP) //todo: MoveToEx
		{
			assert(ta==GDI_ERROR); //unimplemented
		}

		if(Ex_detours_selected_d3dfont
		 &&Ex_detours_selected_fonthdc==hdc)
		{	
			out = EX::displaying_output_font
			(Ex_detours_selected_d3dfont,hdc,dt,box,wtxt,wlen,clipx);
		}
		else 
		{
			assert(!clipx); //unimplemented
	
			SetTextAlign(hdc,0); //docs say to do this
			out = DrawTextW(hdc,wtxt,wlen,&box,dt);
			SetTextAlign(hdc,ta);
		}
	}
	else out = TextOutW(hdc,x,y,wtxt,wlen);

	if(uncenter) SetTextAlign(hdc,~uncenter); return out;
}

#define EX_DETOURS_FONTLOGS 8

static HFONT Ex_detours_fontobjs[EX_DETOURS_FONTLOGS];
static LOGFONTW Ex_detours_logfonts[EX_DETOURS_FONTLOGS];
static bool Ex_detours_logfonts_status[EX_DETOURS_FONTLOGS];

static const EX::Font **Ex_detours_d3dfonts[EX_DETOURS_FONTLOGS] = {};

//HACK: trying to switch font sizes in VR mode
extern void Ex_detours_d3dfonts_release_d3d_interfaces()
{
	for(int i=EX_DETOURS_FONTLOGS;i-->0;)
	if(Ex_detours_d3dfonts[i])
	EX::releasing_output_font(Ex_detours_d3dfonts[i],true);
}

static bool Ex_detours_fontslogging = 0;
static int Ex_detours_fontslogged = 0;

static HFONT WINAPI Ex_detours_CreateFontA
( 
int h, //cHeigh, 
int w, //cWidth, 
int e, //cEscapement, 
int o, //cOrientation, 
int wt, //cWeight, 
DWORD it, //bItalic,
DWORD ul, //bUnderline, 
DWORD so, //bStrikeOut,												 
DWORD cs, //iCharSet, 
DWORD op, //iOutPrecision, 
DWORD cp, //iClipPrecision,
DWORD qu, //iQuality, 
DWORD pf, //iPitchAndFamily, 
LPCSTR face /*pszFaceName*/)
{
	EX_DETOURS_DETOUR_THREADMAIN(CreateFontA,h,w,e,o,wt,it,ul,so,cs,op,cp,qu,pf,face)

 	EXLOG_LEVEL(7) << "Ex_detours_CreateFontA()\n";

	//WARNING: MAY NOT BE SAFE FOR JAPANESE MACHINES
	if(cs!=SHIFTJIS_CHARSET) 
	{
		//TODO: find out if SOM always uses 1000 weight	
		//2018: esi	is 0x019AAAA0 (in SOM::L.menupcs)
		//00422A10 8B 4E 10             mov         ecx,dword ptr [esi+10h]
		//
		
		return EX_DETOURS_(CreateFontA)(h,w,e,o,wt,it,ul,so,cs,op,cp,qu,pf,face);
	}
	
	bool status = false; //onscreen elements

	//2020: I don't think EX_DETOURS_CELL_VS_CHAR is correct???
	//NOTE: I think negative means the top of M is the height but 
	//I'm not certain, the documentation's wording isn't very clear
	assert(h>0);

	EX::INI::Script sc;

	if(!face&&SOM::game)
	{
		face = SOM::fontface;

		if(&sc->status_fonts_height
		||*sc->status_fonts_to_use)	 
		{
			status = true;

			//status_fonts_height code is moved below
		}
	}
	else assert(face); //debugging

	bool untouched = !face; //MS Gothic???

	h = EX_DETOURS_CELL_VS_CHAR(h); //???
				  
	assert(!SOM::tool);
	if(EX::INI::Bugfix()->do_fix_widescreen_font_height)
	{		
		float widen = //ratio of the new size to the old size
		SOM::fontsize(1.333334f*SOM::fov[1]/DDRAW::xyScaling[1]); //YUCK
		widen/=SOM::fontsize(SOM::fov[0]/DDRAW::xyScaling[0]); //YUCK

		//NOTE: all fonts are subject to this. It's hard to 
		//separate menu fonts from others, even if it's worth
		//getting the requested font size exactly right. I mean
		//menu fonts are MS Gothic, so it'd be easy to tell other
		//fonts apart, but it's just not a priority
		h = widen*DDRAW::xyScaling[1]*h;
	}
	else h = DDRAW::xyScaling[1]*h; w = 0; //assert(!w);		
	if(DDRAW::inStereo)
	if(EX::stereo_font) //TESTING	
	{
		h = h*EX::stereo_font+0.5f; 
		w = w*EX::stereo_font+0.5f;
	}
	DDRAW::doSuperSamplingMul(h,w);
	
	//I'm doing this after do_fix_widescreen_font_height
	//because SOM::fontsize uses the horizontal resolution
	//as its metric, which isn't the best input into the
	//extension. I would like to change SOM::fontsize to
	//use the vertical metric but some variable in memory 
	//needs to change accordingly
	if(status)
	if(&sc->status_fonts_height)	
	if(float hh=sc->status_fonts_height(h))
	{
		assert(h>=16); //if(h>=16) //???
		{
			//w = 0; h = (int)(h<0?hh-0.5f:hh+0.5f); 
			w = 0; h = (int)(hh+0.5f); 
		}
	}	

	const wchar_t *wface = 0;

	if(face) 
	{	
		int s = strlen(face);

		const char *uface = face; 

		EX::Convert(932,&uface,&s,&wface); 

		bool runtime = !strcmp(face,SOM::fontface);
		
		if(status&&*sc->status_fonts_to_use)
		{
			wface = sc->status_fonts_to_use;
		}
		else if(runtime&&*sc->system_fonts_to_use)
		{
			wface = sc->system_fonts_to_use;
		}
		else if(!runtime&&sc->fonts_to_use_instead_of_(wface))
		{
			wface = sc->fonts_to_use_instead_of_(wface);						
		}
		else untouched = true;
		
		if(!untouched)
		{
			wt = FW_DONTCARE;
			cs = DEFAULT_CHARSET;
			op = OUT_DEFAULT_PRECIS;
			qu = CLEARTYPE_QUALITY; //DEFAULT_QUALITY; 
			pf = FF_DONTCARE;
		}

		assert(wface);
	}

	const wchar_t *lface = wface;
	
	if(!untouched) //make a 31 character name
	{
		lface = EX::need_a_logical_face_for_output_font(932,face,wface); 
	}

	if(!Ex_detours_fontslogging) //hack: forcing for now
	{
		memset(Ex_detours_fontobjs,0x00,EX_DETOURS_FONTLOGS*sizeof(HFONT));
		memset(Ex_detours_logfonts,0x00,EX_DETOURS_FONTLOGS*sizeof(LOGFONTW));
		memset(Ex_detours_d3dfonts,0x00,EX_DETOURS_FONTLOGS*sizeof(void*));
				
		Ex_detours_fontslogged = 0; //paranoia

		Ex_detours_fontslogging = true;
	}

	//hack: must differentiate customized status fonts
	bool status2 = status&&*sc->status_fonts_to_use; 

	for(int i=0;i<EX_DETOURS_FONTLOGS;i++)
	{
		if(Ex_detours_fontobjs[i])
		{				
			LOGFONTW &cmp = Ex_detours_logfonts[i]; 

			if(cmp.lfHeight==h)
			if(cmp.lfWidth==w)
			if(cmp.lfEscapement==e)
			if(cmp.lfOrientation==o)
			if(cmp.lfWeight==wt)
			if(cmp.lfItalic==it)
			if(cmp.lfUnderline==ul)
			if(cmp.lfStrikeOut==so)
			if(cmp.lfCharSet==cs)
			if(cmp.lfOutPrecision==op)
			if(cmp.lfClipPrecision==cp)
			if(cmp.lfQuality==qu)
			if(cmp.lfPitchAndFamily==pf)			
			if(Ex_detours_logfonts_status[i]==status2)
			if(!wcsncmp(cmp.lfFaceName,lface,LF_FACESIZE-1)) 			
			{
				if(!GetObject(Ex_detours_fontobjs[i],0,0))
				{
					Ex_detours_fontobjs[i] = 0; Ex_detours_fontslogged--;
				}
				else if(!status&&SOM::fontface&&!strcmp(face,SOM::fontface))
				{
					return SOM::font = Ex_detours_fontobjs[i]; //hack
				}
				else return Ex_detours_fontobjs[i];
			}
		}
	}

	EXLOG_LEVEL(7) << " hdc/face is " << wface << " (" << w << 'x' << h << ")\n";
	
	LOGFONTW logfont; 
										
	logfont.lfHeight = h;
	logfont.lfWidth = w;
	logfont.lfEscapement = e;
	logfont.lfOrientation = o;
	logfont.lfWeight = wt;
	logfont.lfItalic = it;
	logfont.lfUnderline = ul;
	logfont.lfStrikeOut = so;
	logfont.lfCharSet = cs;
	logfont.lfOutPrecision = op;
	logfont.lfClipPrecision = cp;
	logfont.lfQuality = qu;
	logfont.lfPitchAndFamily = pf;
	
	wcsncpy(logfont.lfFaceName,lface,LF_FACESIZE-1);

	logfont.lfFaceName[LF_FACESIZE-1] = '\0'; //docs: wcsncpy

	HFONT out; const EX::Font **p = 0;

	if(!untouched) //hack
	{
		wface = EX::describing_output_font(logfont,L"%s, 1000 %s",wface,SOM::wideface);
	}
	else wface = EX::describing_output_font(logfont,wface);

	if(EX::is_able_to_facilitate_font_output())
	{
		p = EX::reserving_output_font(logfont,wface);

		if(!p) //for paranoia
		{
			assert(0); return 0; //should not happen
		}
	}
	else out = CreateFontW(h,w,e,o,wt,it,ul,so,cs,op,cp,qu,pf,wface);

	if(p) out = EX::need_a_client_handle_for_output_font(p);

	if(!out)
	{
		if(p) EX::releasing_output_font(p);

		return out;
	}

	if(Ex_detours_fontslogged==EX_DETOURS_FONTLOGS) //gotta clear the logs
	{
		Ex_detours_fontslogging = false; //see: EX::DeleteObject()

		for(int i=0;i<EX_DETOURS_FONTLOGS;i++) if(Ex_detours_fontobjs[i])
		{	
			Ex_detours_logfonts_status[i] = false; //NEW

			EX_DETOURS_(DeleteObject)(Ex_detours_fontobjs[i]); 
			
			Ex_detours_fontobjs[i] = 0;	

			if(Ex_detours_d3dfonts[i])
			{
				EX::releasing_output_font(Ex_detours_d3dfonts[i]);

				Ex_detours_d3dfonts[i] = 0;
			}
		}

		Ex_detours_fontslogging = true;  
		Ex_detours_fontslogged = 0;
	}

	int i;
	for(i=0;i<EX_DETOURS_FONTLOGS;i++) if(!Ex_detours_fontobjs[i])
	{
		Ex_detours_logfonts_status[i] = status2; //NEW

		Ex_detours_logfonts[i] = logfont; 
		Ex_detours_fontobjs[i] = out;					
		Ex_detours_d3dfonts[i] = p;

		break;
	}
	assert(i<EX_DETOURS_FONTLOGS); //paranoia

	Ex_detours_fontslogged++; 

	if(!status)
	if(SOM::fontface&&!strcmp(face,SOM::fontface)) 
	{
		RECT win; if(GetClientRect(EX::client,&win)) 
			
		EXLOG_LEVEL(7) << " Current resolution is " << win.right << 'x' << win.bottom << '\n';

		EXLOG_LEVEL(7) << " Font details are e"<<e<<" o"<<o<<" wt"<<wt<<" it"<<it<<" ul"<<ul<<" so"<<so<<" cs"<<cs<<" op"<<op<<" cp"<<cp<<" qu"<<qu<<" pf"<<pf<<'\n';

		SOM::font = out; //hack??
	}
		
	return out;
}		

static HGDIOBJ WINAPI Ex_detours_SelectObject(HDC hdc,HGDIOBJ ho)
{
	EX_DETOURS_DETOUR_THREADMAIN(SelectObject,hdc,ho)

 	EXLOG_LEVEL(7) << "Ex_detours_SelectObject()\n";

	DWORD t = GetObjectType(ho);

	if(t==OBJ_FONT)	
	if(Ex_detours_fontslogging)
	if(EX::is_able_to_facilitate_font_output())
	{
		//Ex_detours_selected_d3dfont = 0;
		//Ex_detours_selected_fonthdc = 0;

		for(int i=0;i<EX_DETOURS_FONTLOGS;i++)
		{
			if(Ex_detours_fontobjs[i]==ho)
			{
				Ex_detours_selected_d3dfont = Ex_detours_d3dfonts[i];
				Ex_detours_selected_fonthdc = hdc;
			}
		}		
	}	

	return EX_DETOURS_(SelectObject)(hdc,ho);
}		

static BOOL WINAPI Ex_detours_DeleteObject(HGDIOBJ ho)
{
	EX_DETOURS_DETOUR_THREADMAIN(DeleteObject,ho)

 	EXLOG_LEVEL(7) << "Ex_detours_DeleteObject()\n";

	DWORD t = GetObjectType(ho);

	if(Ex_detours_fontslogging)
	if(t==OBJ_FONT&&GetObject(ho,0,0))
	for(int i=0;i<EX_DETOURS_FONTLOGS;i++)
	{
		if(Ex_detours_fontobjs[i]==ho)
			
		return true; //hold onto fonts
	}	

	const char *log = 0;

#ifdef _DEBUG
	switch(t)
	{
	case OBJ_BITMAP: log =  "(Bitmap)"; break; 
	case OBJ_BRUSH: log = " (Brush)"; break;
	case OBJ_COLORSPACE: log = " (Color space)"; break;
	case OBJ_DC: log = " (Devicecontext)"; break;
	case OBJ_ENHMETADC: log = " (Enhanced metafile DC)"; break;
	case OBJ_ENHMETAFILE: log = " (Enhanced metafile)"; break;
	case OBJ_EXTPEN: log = " (Extended pen)"; break;
	case OBJ_FONT: log = " (Font)"; break;
	case OBJ_MEMDC: log = " (Memory DC)"; break;
	case OBJ_METAFILE: log = " (Metafile)"; break;
	case OBJ_METADC: log = " (Metafile DC)"; break;
	case OBJ_PAL: log = " (Palette)"; break;
	case OBJ_PEN: log = " (Pen)"; break;
	case OBJ_REGION: log = " (Region)"; break;
	}
#endif

	EXLOG_LEVEL(4) << "DeleteObject: hdi object is " << ho << (log?log:"") << '\n';

	return EX_DETOURS_(DeleteObject)(ho);
}

static bool Ex_detours_Detoured = false;

extern bool EX::is_Detoured()
{
	return Ex_detours_Detoured;
}

static void Ex_detours_f(LONG (WINAPI *f)(PVOID*,PVOID))
{	
	//REMOVE ME?
	if(!SOM::game) return;

	#define DETOUR(g) \
	f(&(PVOID&)Ex_detours::g,Ex_detours_##g);		

	EX::INI::Bugfix bf; //EX::INI::Window wn;
	bool gdifonts = bf->do_fix_lifetime_of_gdi_objects;	
	//bool d3dfonts = wn->directx_version_to_use_for_window==9;
	bool d3dfonts = 9==EX::directx(); 
	
	DETOUR(DrawTextA)
	DETOUR(TextOutA)
		
	if(d3dfonts||gdifonts) DETOUR(CreateFontA)
	if(d3dfonts||gdifonts) DETOUR(DeleteObject);
	
	if(d3dfonts) DETOUR(SelectObject)
	if(d3dfonts) DETOUR(DrawTextW)

	DETOUR(SetWindowPos)

	#undef DETOUR
}

static std::vector<void(*)(LONG(WINAPI*)(PVOID*,PVOID))> Ex_detouring;

extern int EX::detouring(void (*f)(LONG(WINAPI*)(PVOID*,PVOID)))
{
	if(!EX::is_Detoured())
	{
		if(Ex_detouring.empty()) Ex_detouring.reserve(8);

		for(size_t i=0;i<Ex_detouring.size();i++) //paranoia
		{
			if(Ex_detouring[i]!=f) continue; assert(0); return 0;
		}

		Ex_detouring.push_back(f); return Ex_detouring.size(); 
	}
	else assert(0); return 0;
}

static void Ex_detours_Detours(ULONG threadID, LONG (WINAPI*op)(PVOID*,PVOID))
{
	if(op==DetourAttach)
	{
		if(Ex_detours_Detoured++) return;
	}
	else if(op==DetourDetach)
	{
		if(!Ex_detours_Detoured) return;
	}
	else return;
	
	HANDLE hthread = 0;

	if(threadID&&threadID!=GetCurrentThreadId())
	{
		//dunno what kind of access is appropriate if any
		//hthread = OpenThread(THREAD_ALL_ACCESS,0,threadID);
		hthread = OpenThread(THREAD_QUERY_INFORMATION|THREAD_SUSPEND_RESUME,0,threadID);
		DWORD debug = GetLastError();
		assert(hthread);
	}
	else hthread = GetCurrentThread();

	if(DetourTransactionBegin()
    ||DetourUpdateThread(hthread))
	assert(!"Detours");

	//assuming order is unimportant
	//
	Ex_detours_f(op);
	//
	if(EX::detours) EX::detours(op);
	//
	for(size_t i=0;i<Ex_detouring.size();i++)
	{
		Ex_detouring[i](op);
	}

    if(DetourTransactionCommit())
	assert(!"Detours");
	
	if(threadID) 
	CloseHandle(hthread);

	if(op==DetourDetach) 
	{
		Ex_detours_Detoured = false;
	}
}
extern void EX::setting_up_Detours(ULONG threadID)
{
	return Ex_detours_Detours(threadID,DetourAttach);
}
extern void EX::cleaning_up_Detours(ULONG threadID)
{
	return Ex_detours_Detours(threadID,DetourDetach);
}