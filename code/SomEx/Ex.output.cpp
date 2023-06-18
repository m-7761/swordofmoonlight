
#include "Ex.h" 
EX_TRANSLATION_UNIT

#define _USE_MATH_DEFINES
					   
#include <cmath>
#include <time.h>

#include <d3dx9.h>
#include <d3dx9math.h>

#pragma comment(lib,"version.lib")

#include "dx.ddraw.h"
#include "dx.dsound.h"

#include "Ex.ini.h"
#include "Ex.fonts.h"
#include "Ex.cursor.h"
#include "Ex.input.h"
#include "Ex.output.h"
#include "Ex.memory.h"
#include "Ex.window.h"

//REMOVE ME?
#include "som.932w.h"
#include "som.files.h"
#include "som.state.h"					   
#include "som.title.h" //japanese
#include "som.status.h"
#include "som.extra.h"

#include "../Exselector/Exselector.h" //OpenGL font?
extern class Exselector *Exselector;

namespace DSOUND
{
	extern int doReverb_i3dl2[2];
}

namespace Ex_detours
{						  
	extern int(WINAPI*DrawTextW)(HDC,LPCWSTR,int,LPRECT,UINT); //HACK
}

extern float EX::stereo_font = 0.0f;

extern const wchar_t *EX::shadermodel = 0;

extern float EX::fps = 0.0f;
extern float EX::dpi = 0.0f; //not known

extern float EX::clip = 0.0f; //clipped fps
extern float EX::kmph = 0.0f; //kilometers per hour
extern float EX::kmph2 = 0.0f, EX::e = 0.0f; 

extern float EX::mouse[3] = {0,0,0}; //debugging

extern BYTE EX::keys[256] = {0};

extern DWORD EX::polls[256] = {0};

#ifndef EX_DBGMSG_DEFINED //NDEBUG
static char Ex_output_dbgmsg[1] = "";
#else
static char Ex_output_dbgmsg[MAX_PATH*4] = "";
static std::wstring Ex_output_dbgmsg_str; //2020
extern void EX::dbgmsg(const char *f,...)
{
	va_list va; va_start(va,f); 
	int len = _vsnprintf_s(Ex_output_dbgmsg,_TRUNCATE,f,va); va_end(va);

	static unsigned frame = 0; if(frame!=DDRAW::noFlips)
	{
		frame = DDRAW::noFlips; Ex_output_dbgmsg_str.clear();
	}
	Ex_output_dbgmsg_str.append(Ex_output_dbgmsg,Ex_output_dbgmsg+len);
	Ex_output_dbgmsg_str.push_back('\n');
}
#endif

extern int EX::messagebox(int mb, const char *f,...)
{
	va_list va; va_start(va,f);
	enum{ N=512 };
	wchar_t msg[N],fmt[N];
	int i = 0; while(i<N-1&&(fmt[i]=f[i])) i++;
	fmt[i] = '\0';
	_vsnwprintf_s(msg,_TRUNCATE,fmt,va); va_end(va);
	HWND aw = GetActiveWindow(); 
	if(!aw) mb|=MB_SYSTEMMODAL;
	return MessageBoxW(aw,msg,EX::exe(),mb);
}

static ID3DXLine *Ex_output_line = 0;
extern D3DXMATRIX dx_d3d9c_worldviewproj;
static void Ex_output_lines_line(DWORD c, float *v1, float *v2)
{
	D3DXVECTOR4 h[2];		
	D3DXVec3Transform(h+0,(D3DXVECTOR3*)v1,&dx_d3d9c_worldviewproj);
	D3DXVec3Transform(h+1,(D3DXVECTOR3*)v2,&dx_d3d9c_worldviewproj);
		
	//REMINDER: I tried to use Draw but it has no concept of
	//depth for the zbuffer, so it's pretty useless, and I
	//tried to use -zB and zA as interpolation factors on
	//v1 and v2 and they didn't transfer even though it seems
	//like they should, I also tried multiplying/dividing them
	//by "w". so the only thing left was to pass an identity 
	//matrix

	bool a = h[0].w<=0, b = h[1].w<=0; if(a||b)
	{
		if(a&&b) return; //both behind near plane?

		//these are theoretically the result of multiplying 
		//the points by the Z plane at D=0
		float zA = h[0].z, zB = h[1].z;

		//it's always confusing reading math peoples' writings 
		//onilne. this seems to work, z is the plane equation
		//(normal) at 0, and the only thing I felt uneasy about
		//was what does it mean to have a plane in homogeneous
		//coordinates and what to do with the vectors' w coords
		D3DXVECTOR4 &pt = h[b]; for(int i=4;i-->0;)
		{
			pt[i] = -zB*h[0][i]+zA*h[1][i]; //intersection point?
		}
	}
	D3DXVECTOR3 v[2]; for(int i=2;i-->0;)
	{
		auto &pt = v[i]; auto &v4 = h[i];
		pt.x = v4.x/v4.w; pt.y = v4.y/v4.w;			
		pt.z = v4.z/v4.w;
	}
	//why is Draw in 2D coordinates? 
	// 
	// NOTE: Draw requires converting to "screen space" as the docs
	// uselessly say. the formula I found was x = (1+x)*w, y = (1-y)*h
	// where w and h are half the viewport's dimensions 
	//
	//Ex_output_line->DrawTransform(v,2,&dx_d3d9c_worldviewproj,c);
	//Ex_output_line->Draw(v,2,c);
	Ex_output_line->DrawTransform(v,2,(D3DXMATRIX*)&DDRAW::Identity,c);
}
extern bool Ex_output_lines(float *p, int s, int n, WORD *q, DWORD col)
{
	if(DDRAW::target!='dx9c') return false;
	
	IDirect3DDevice9 *d3dd9 = DDRAW::Direct3DDevice7->proxy9;

	if(!Ex_output_line)
	{
		if(D3DXCreateLine(d3dd9,&Ex_output_line)) 
		assert(0);
		if(!Ex_output_line) return false;

		//2021: increasing thickness helps to remove variation
		//in pixels. the caller needs to add some transparency
		//so it's not so bright
		assert(!DDRAW::inStereo); //assuming 2x supersampling?
		Ex_output_line->SetWidth(DDRAW::doSuperSampling?3.7f:2.3f);
		Ex_output_line->SetAntialias(1);		
	}

	extern void dx_d3d9c_vstransform(int=0);
	dx_d3d9c_vstransform();

	Ex_output_line->Begin();

	if(q) for(WORD*d=q+n;q<d;q+=2) //2021
	{
		float *v1 = p+q[0]*s;

		DWORD c = col?col:*(DWORD*)(v1+4); //2021

		Ex_output_lines_line(c,v1,p+q[1]*s);
	}
	else for(float*d=p+n;p<d;p+=s*2)
	{
		DWORD c = col?col:*(DWORD*)(p+4); //2021

		Ex_output_lines_line(c,p,p+s);
	}

	Ex_output_line->End();

	//End must assume identity matrix???
	//d3dd9->SetTransform(&dx_d3d9c_worldviewproj);

	return true;
}

extern bool EX::output_overlay_f[1+12] = {0,0,0,0,0,1,1,0,0,0,0,0};

extern bool 
&EX::f1 = EX::output_overlay_f[1],
&EX::f2 = EX::output_overlay_f[2],
&EX::f3 = EX::output_overlay_f[3],
&EX::f4 = EX::output_overlay_f[4],
&EX::f5 = EX::output_overlay_f[5],
&EX::f6 = EX::output_overlay_f[6],
&EX::f7 = EX::output_overlay_f[7],
&EX::f8 = EX::output_overlay_f[8],
&EX::f9 = EX::output_overlay_f[9],
&EX::f10 = EX::output_overlay_f[10],
&EX::f11 = EX::output_overlay_f[11],
&EX::f12 = EX::output_overlay_f[12];

static wchar_t Ex_output_buffer[2056]; 

static DWORD Ex_output_format = DT_RIGHT|DT_TOP|DT_NOCLIP|DT_SINGLELINE;

static const RECT Ex_output_default_margin = { 0, 0, 640, 480 };

static RECT Ex_output_margin = Ex_output_default_margin;

static RECT Ex_output_calculation;

static ID3DXSprite *Ex_output_sprite = 0; //ID3DXFont::DrawText 

static const auto Ex_output_fontfailedtoload = (ID3DXFont*)-1;
static const auto Ex_output_spritefailedtoload = (ID3DXSprite*)-1;

static ID3DXFont *Ex_output_font = 0;
static ID3DXSprite *Ex_output_cursor = 0;

//OpenGL
enum{ Ex_output_16=16 };
static int Ex_output_gl = 0; 
static HDC Ex_output_hdc = 0;

static bool Ex_output_underline = false;

static D3DCOLOR Ex_output_color = 0xFFFFFFFF;
static D3DCOLOR Ex_output_shadow = 0x00000000;

static int CALLBACK Ex_output_enumFFE(const LOGFONTW *lpelfe, const TEXTMETRICW *lpntme, DWORD FontType, LPARAM lParam)
{
	if(1||!EX::debug) if(~FontType&TRUETYPE_FONTTYPE) return 1;

	memcpy((void*)lParam,lpelfe->lfFaceName,sizeof(lpelfe->lfFaceName)); return 0;
}
static int Ex_output_print(const wchar_t *X) 
{
	assert(DDRAW::inScene);

	static INT drawn = 0; //hack...

	if(Ex_output_margin.top 
	+drawn>Ex_output_margin.bottom) //clipping
	{
		Ex_output_margin.top+=drawn; return 0;
	}
	else if(!X||!*X) return 0;	

	//ID3DXFont DT_CALCRECT is way better DrawText/DrawTextEx??
	ID3DXFont *f = Ex_output_font; 
	ID3DXSprite *_ = 0;	
	int gl = DDRAW::gl; if(!gl) //D3DX?
	{
		_ = Ex_output_cursor; //f = Ex_output_font;

		if(!f||f==Ex_output_fontfailedtoload) return 0;	

		if(_==Ex_output_spritefailedtoload) _ = 0;
	}
	else gl = Ex_output_gl;

	if(!f||f==Ex_output_fontfailedtoload) return 0;	

	RECT *rect = &Ex_output_margin;
	auto format = Ex_output_format;
	if(format&DT_CALCRECT)
	{
		Ex_output_calculation = Ex_output_margin;
		rect = &Ex_output_calculation;
	}	
		
	static const wchar_t u[4] = {'_','_','_','_'};
	int len = wcslen(X);	
	assert(!Ex_output_underline||len<=4);
	
	if(Ex_output_shadow&0xFF000000)
	if(~format&DT_CALCRECT)
	{
		Ex_output_margin.top++; //assuming top

		if(format&DT_RIGHT) Ex_output_margin.right++; else Ex_output_margin.left++;
				
		if(gl) //2021
		drawn = Exselector->svg_draw(gl,(X),len,rect,format,Ex_output_shadow);
		else
		drawn = f->DrawTextW(_,(X),len,rect,format,Ex_output_shadow);
				
		if(Ex_output_underline) //assuming monospace
		if(gl) //2021
		Exselector->svg_draw(gl,u,len,rect,format,Ex_output_shadow);
		else
		f->DrawTextW(_,u,len,rect,format,Ex_output_shadow);
		
		if(!drawn&&_)
		{
			Ex_output_calculation = Ex_output_margin;

			_->End(); _->Release(); _ = Ex_output_cursor = 0; //sprites sometimes go bad??

			f->DrawTextW(0,(X),len,rect,format,Ex_output_shadow);
					
			if(Ex_output_underline) //assuming monospace
			f->DrawTextW(_,u,len,rect,format,Ex_output_shadow);		
		}

		if(format&DT_RIGHT) Ex_output_margin.right--; else Ex_output_margin.left--;									

		Ex_output_margin.top--; //assuming top
	}
	
	//if(gl&&format&DT_CALCRECT) //2021
	//drawn = Ex_detours::DrawTextW(Ex_output_hdc,(X),len,rect,format);
	//else 
	if(gl&&~format&DT_CALCRECT)
	drawn = Exselector->svg_draw(gl,(X),len,rect,format,Ex_output_color);
	else
	drawn = f->DrawTextW(_,(X),len,rect,format,Ex_output_color);

	if(Ex_output_underline) //assuming monospace
	if(~format&DT_CALCRECT) //2021
	{
		if(gl) //2021
		drawn = Exselector->svg_draw(gl,u,len,rect,format,Ex_output_color);
		else
		f->DrawTextW(_,u,len,rect,format,Ex_output_color);
	}

	if(!drawn&&_)
	{
		Ex_output_calculation = Ex_output_margin;

		_->End(); _->Release(); _ = Ex_output_cursor = 0; //sprites sometimes go bad??

		drawn = f->DrawTextW(0,(X),len,rect,format,Ex_output_color);

		if(Ex_output_underline) //assuming monospace
		if(~format&DT_CALCRECT) //2021
		f->DrawTextW(_,u,len,rect,format,Ex_output_color);
	}
		
	Ex_output_margin.top+=drawn; 

	return drawn?1:0;
}

#define EX_OUTPUT_PRINT(X) Ex_output_print(X);

#define EX_OUTPUT_PRINT_F(...)\
(swprintf_s(Ex_output_buffer,2056,__VA_ARGS__),Ex_output_print(Ex_output_buffer));

#define EX_OUTPUT_FONTS 32 //TODO: dynamic implementation

static const EX::Font *Ex_output_fonts[EX_OUTPUT_FONTS];

static ID3DXFont *Ex_output_create_logical_font(const LOGFONTW *logical_font)
{
	if(!logical_font	
	||DDRAW::compat!='dx9c'||!DDRAW::Direct3DDevice7)
	{
		assert(0); return 0;
	}

	ID3DXFont *out = 0;
		
	int h = logical_font->lfHeight;
	int w = logical_font->lfWidth;

	HRESULT ok = D3DXCreateFontW
	(
		DDRAW::Direct3DDevice7->proxy9,
		h,
		w,
		logical_font->lfWeight,
		1, //mipmap levels
		logical_font->lfItalic,
		logical_font->lfCharSet,
		logical_font->lfOutPrecision,
		logical_font->lfQuality,
		logical_font->lfPitchAndFamily,
		logical_font->lfFaceName,
		&out
	);

	if(ok!=D3D_OK) out = Ex_output_fontfailedtoload;

	return out;
}	

#define EX_OUTPUT_SQUARE_MANAGED_FONT_OR(X,DO) \
if(!X||X<Ex_output_fonts||X>=Ex_output_fonts+EX_OUTPUT_FONTS) DO;

extern const EX::Font **EX::reserving_output_font(LOGFONTW &in, const wchar_t *description)
{		
	if(DDRAW::compat!='dx9c'){ assert(0); return 0; }

	int first_available = EX_OUTPUT_FONTS;
	int space_available = EX_OUTPUT_FONTS;

	for(int i=0;i<EX_OUTPUT_FONTS;i++) 
	if(Ex_output_fonts[i]&&Ex_output_fonts[i]->refs)
	{
		if(!memcmp(&in,&Ex_output_fonts[i]->log,sizeof(in)))
		{
			Ex_output_fonts[i]->refs++; return &Ex_output_fonts[i];
		}
		else space_available--;
	}
	else if(first_available==EX_OUTPUT_FONTS)
	{
		first_available = i;
	}

	if(!space_available) //TODO: dynamic implementation
	{
		//TODO: clear alert dialog (IDirect3DDevice9::SetDialogBoxMode)  

		assert(0); return 0; //FAIL
	}
	
//	memcpy(&Ex_output_fonts[first_available].log,&logical_font,sizeof(LOGFONTW));

//	Ex_output_fonts[first_available] = Ex_output_create_logical_font(logical_font);

	Ex_output_fonts[first_available] = EX::creating_font(in,description);
	
	if(!Ex_output_fonts[first_available]) return 0;

	Ex_output_fonts[first_available]->refs = 1; 

	return &Ex_output_fonts[first_available];
}

extern int dx_d3d9x_gl_viewport[4];
static int(*Ex_output_Exselector_xr_callback)(int) = 0;
extern void EX::beginning_output_font(bool upsidedown, int(*xr_f)(int))
{
	Ex_output_Exselector_xr_callback = DDRAW::xr?xr_f:nullptr;

	if(DDRAW::gl&&!DDRAW::xr)
	{
		int &w = dx_d3d9x_gl_viewport[2];
		int &h = dx_d3d9x_gl_viewport[3];
		float m[4*4] = 
		{
			2.0f/w,0,0,0,
			0,-2.0f/h,0,0,
			0,0,0,0,
			-1,1,0,1,
		};
		if(upsidedown) m[5] = -m[5]; 
		if(upsidedown) m[13] = -m[13];

		Exselector->svg_view(m);
	}
}
static int Ex_output_glyphs(const EX::Font::glyphset_t &g,
DWORD how, const wchar_t *txt, D3DCOLOR col, RECT &line, RECT &box, int clipx, HDC glDC)
{
	IDirect3DDevice9 *d3dd9 = DDRAW::Direct3DDevice7->proxy9;

	int i; for(i=0;i<2;i++) //sprites sometimes go bad??
	{
		ID3DXFont *q; ID3DXSprite *_;
		
		//ID3DXFont DT_CALCRECT is way better DrawText/DrawTextEx??
		if(!*g.d3dxfont) 
		{
			*g.d3dxfont = Ex_output_create_logical_font(g.lfont);	
			if(!*g.d3dxfont) 
			*g.d3dxfont = Ex_output_fontfailedtoload;		
		}									
		q = *g.d3dxfont;

		if(!q||q==Ex_output_fontfailedtoload) continue;			

		const bool gl = DDRAW::gl;

		if(!gl) //OpenGL needs to size KF2's gauges/compass
		{
			if(!*g.d3dxsprite)
			{
				if(D3DXCreateSprite(d3dd9,g.d3dxsprite))
				*g.d3dxsprite = Ex_output_spritefailedtoload;		
			}
			_ = *g.d3dxsprite; 
			if(_==Ex_output_spritefailedtoload)	_ = 0;	

			if(~how&DT_CALCRECT) //2021
			{
				//REMINDER: without this sprites are just squares
				DWORD st = D3DXSPRITE_ALPHABLEND; 
				
				//if(DDRAW::inStereo)
				//st|=D3DXSPRITE_OBJECTSPACE; //EXPERIMENTAL

				if(!_||_->Begin(st)!=D3D_OK)
				{
					if(_) _->Release();
					_ = *g.d3dxsprite = 0;
					continue;
				}
			}
		}
		else //2021: OpenGL?
		{
			_ = 0; //q = 0;

			if(~how&DT_CALCRECT)
			{
				//2022: caller must have called EX::beginning_output_font
				//Exselector->svg_view();

				Exselector->svg_begin();

				//HACK: OpenGL ES HAS ZERO STATE MANAGEMENT FACILITIES
				DDRAW::Direct3DDevice7->queryGL->state.apply_nanovg();

				if(!*g.glfont) *g.glfont = Exselector->svg_font(g.hfont);
			}
		}		

		RECT calc = line; int drawn; if(q)
		{
			drawn = q->DrawTextW(_,txt,g.selected,&calc,how|DT_CALCRECT,col);		
		}
		/*else TODO: rely on ID3DXFont?
		{
			//note: this has the same size for SOM::Status "X" but for some reason
			//I think (may be wrong) kf2's gauges box comes out considerably wider

			//TESTING
			//does DrawTextExW work like ID3DXFont?
		//	drawn = Ex_detours::DrawTextW(glDC,txt,g.selected,&calc,how|DT_CALCRECT);			
			//SetTextAlign(glDC,0); //TESTING
			drawn = DrawTextExW(glDC,(wchar_t*)txt,g.selected,&calc,how|DT_CALCRECT,0);

			//apparently ID3DXFont does this for free??? //YUCK
			if(how&(DT_CENTER|DT_VCENTER))
			{
				//doesn't hold in Yes/No popup menu?
				//assert(calc.left<=1&&calc.top<=1);
				int cx = how&DT_CENTER?line.right-calc.right:0;
				int cy = how&DT_VCENTER?line.bottom-calc.top:0;
				OffsetRect(&calc,cx/2,cy/2);
			}
			else if(how&(DT_RIGHT)) //:(
			{
				int cx = line.right-calc.right;
				OffsetRect(&calc,cx,0);
			}
		}*/
		else assert(0);

		int clipped = g.selected;
		if(clipx&&calc.right-box.left>clipx)
		{
			int l = 0, r = clipped;
			float x = calc.right-box.left;
			int pivot = float(clipped)*clipx/x;
			for(RECT clip=calc;r-l>1/*&&q->DrawTextW
			(_,txt,pivot,&clip,how|DT_CALCRECT,col)*/;pivot=l+(r-l)/2)
			{
				if(q&&!q->DrawTextW(_,txt,pivot,&clip,how|DT_CALCRECT,col))
				break;
			//	if(!q&&!Ex_detours::DrawTextW(glDC,txt,pivot,&clip,how|DT_CALCRECT))
			//	break;
				(clip.right>box.left+clipx?r:l) = pivot; 
			}
			clipped = pivot;
		}

		bool scissor = false; if(drawn)
		{
			//Reminder: STEREO DISPLACES calc
			if(how&DT_RIGHT)
			{
				line.right = calc.left; //!!
			}
			else line.left = calc.right; //!!

			if(how&DT_CALCRECT) 
			{
				UnionRect(&box,&box,&calc);
			}
			else //drawn = q->DrawTextW(_,txt,clipped,&calc,how,col); 
			{
				//TESTING: I'M AT A LOSS :(
				//this works/helps but the menu frames seem to be off
				//too... although this is independent of dx.d3d9c.cpp
				//+1 here isn't accurate but is as close as it can be
				//without D3DXSPRITE_OBJECTSPACE 
				/*trying to do this in the mipmap only
				int inc = DDRAW::xyMapping2[0];
				if(inc) OffsetRect(&calc,inc,inc);*/

				//HACK?
				//NOTE: using clip-planes since for VR the text will
				//have to freely rotate around the audience since it
				//doesn't otherwise fit
				if(DDRAW::inStereo)
				{	
					scissor = !EX::stereo_font;

					RECT sr; if(DDRAW::window_rect(&sr))
					{
						DDRAW::doSuperSamplingMul(sr.right,sr.bottom);

						int x = sr.right/4; sr.right/=2;

						int sep = DDRAW::stereo*EX_INI_MENUDISTANCE*sr.right;
						
						//NOTE: D3D7 doesn't seem to have a scissor state??? odd
						if(scissor) d3dd9->SetScissorRect(&sr);
						if(scissor) d3dd9->SetRenderState(D3DRS_SCISSORTESTENABLE,1);
						calc.left-=x; calc.right-=x;
						OffsetRect(&calc,sep,0);
						assert(q); //glScissor?
						if(q)
						q->DrawTextW(_,txt,clipped,&calc,how,col);
					//	else
					//	Exselector->svg_draw(*g.glfont,txt,clipped,&calc,how,col);
						OffsetRect(&calc,-2*sep,0);
						if(_) _->Flush();						
						x*=2; //!!							
						OffsetRect(&sr,x,0);
						if(scissor) d3dd9->SetScissorRect(&sr);
						calc.left+=x; calc.right+=x;
					}
					else assert(DDRAW::window);
				}
				if(!gl) //q
				drawn = q->DrawTextW(_,txt,clipped,&calc,how,col);
				else
				drawn = Exselector->svg_draw(*g.glfont,txt,clipped,&calc,how,col);

				/*trying to do this in the mipmap only
				if(inc) OffsetRect(&calc,-inc,-inc);*/
			}
		}

		if(!gl) //q
		{
			if(~how&DT_CALCRECT) //2021
			{
				if(_) _->End(); //!

				if(scissor) d3dd9->SetRenderState(D3DRS_SCISSORTESTENABLE,0);
			}

			if(!drawn) 
			{
				_->Release();				
				_ = *g.d3dxsprite = 0; 
				continue;
			}	
		}
		else if(~how&DT_CALCRECT) 
		{
			Exselector->svg_end(Ex_output_Exselector_xr_callback);
		}

		return drawn;
	}
	return 0;
}

extern int EX::displaying_output_font(const EX::Font **in, HDC hdc, DWORD how, RECT &box, const wchar_t *txt, int len, int clipx)
{
	//NOTE: looks agnostic to D3D9/OpenGL (keep it that way)
	if(!DDRAW::Direct3DDevice7||DDRAW::compat!='dx9c'){ assert(0); return 0; }

	if(!hdc) return 0; //window was closed

	if(!in||!*in||!len||!txt||!*txt) return 0;

	EX_OUTPUT_SQUARE_MANAGED_FONT_OR(in,return 0)

	if(len<0) len = len==-1?wcslen(txt):0; //DrawText

	DDRAW::PushScene(); //BeginScene?
	
	//warning: assuming DC is the client area of the app window

	if(box.left==box.right&&box.top==box.bottom) //TextOut mode
	{
		assert(0); //move to Ex.detours.cpp
	}
		
	//TODO: calculate rect
	if(how&DT_BOTTOM) how|=DT_SINGLELINE; //D3DX requirement

	const DWORD supported_flags = //DrawText is finicky
	DT_BOTTOM|DT_CALCRECT|DT_CENTER|DT_EXPANDTABS|DT_RIGHT|
	DT_RTLREADING|DT_SINGLELINE|DT_TOP|DT_VCENTER|DT_WORDBREAK|
	DT_NOCLIP|DT_NOPREFIX;

	assert(0==(how&~supported_flags)); //2018

	how&=supported_flags; //so everything is kosher

	D3DCOLOR tc = GetTextColor(hdc);

	D3DCOLOR col = tc&0xFF00FF00; //alpha/green
	if(0==col>>24) col|=0xFF000000;	
	col|=(tc&0xFF)<<16|(tc&0xFF0000)>>16; //red/blue (swap)

	how|=DT_EXPANDTABS; //return non-zero on all tabs line	 
	how|=DT_NOCLIP;
	how&=~DT_NOPREFIX; //2020: Not working for DT_RIGHT???
	
	INT out = 0; const EX::Font *p = *in; 
	
	RECT line = box; 	
	LONG &margin = how&DT_RIGHT?line.right:line.left;
	LONG creturn = margin;

	if(how&DT_CALCRECT) memset(&box,0x00,sizeof(box));

	float ltrim = 0, rtrim = 0; //goto hack; //float??

	//HACK: draw RT_RIGHT backward... doesn't work for 
	//more than a single-line
	//TODO: should use RichText for this in the future
	//no reason to develop a complicated algorithm now
	enum{ hackN=64 };
	wchar_t hack[hackN]; 
	const wchar_t *span;
	const wchar_t *txt2 = txt;
	int g = p->select(txt2,len); 
	int sel = p->glyphs[g].selected;
	if(sel<len)
	{	
		if(how&DT_RIGHT)
		{
			assert(len<hackN); 
			if(len>hackN) len = hackN;
			for(int i=len,j=0;i-->0;j++)
			{
				hack[i] = txt[j]; //hack: reversing select
			}
			txt2 = hack; txt+=len;
			//goto hack;
		}
		else if(how&DT_CENTER)
		{
			//HACK: Must reckon the width before it
			//can be displayed.
			int how2 = how&~DT_CENTER|DT_CALCRECT;
			RECT box2 = {0,0,10000,10000};
			EX::displaying_output_font(in,hdc,how2,box2,txt,len); //clipx
			int w = line.right-line.left-box2.right;
			line.left+=w/2; line.right-=w/2;
			how&=~DT_CENTER;
			goto hack;
		}		
	}
	else goto hack;

	for(bool midline=false;len>0;)
	{					  
		g = p->select(txt2,len,midline);
		sel = p->glyphs[g].selected;
		
		//2020: adding synthetic space between fonts
		//(why wasn't this implemented?)
		//moved ltrim/rtrim out of loop to remember
		//previous trim
		if(how&DT_RIGHT) //backward? (untested)
		line.right-=ltrim; else line.left+=rtrim;
		ltrim = rtrim = 0;

		hack: //hybrid RT_RIGHT fix
		
		//2018: if font changes on a \n, removing the partial line
		if(p->glyphs[g].nselected)
		{
			if(sel<len||creturn!=margin)
			{
				while(txt2[sel-1]!='\n') sel--;			
			}
			midline = txt2[sel-1]!='\n';
		}
		else midline = true;
				
		if(txt==txt2) //hack
		{
			span = txt; txt+=sel;
		}
		else span = txt-=sel;

		txt2+=sel; len-=sel;

		for(int i=0;i<sel;i++) switch(span[i])
		{
		case ' ': ltrim+=p->space; break;

		case L'\u3000': ltrim+=p->u3000; break;

		default: span+=i; sel-=i;
				
			i = sel; //break out of loop
		}

		if(ltrim&&how&DT_CALCRECT&&~how&DT_RIGHT) 
		{
			RECT calc = {line.left,line.top,line.left+ltrim,line.bottom};
			UnionRect(&box,&box,&calc);			
		}

		line.left+=ltrim;

		if(sel==0) continue;

		for(int i=sel;i-->0;) switch(span[i])
		{
		case ' ': case L'\u3000': 

			rtrim+=span[i]==' '?p->space:p->u3000; 

			break;

		default: i = 0; //break out of loop
		}

		line.right-=rtrim;

		if(rtrim&&how&DT_CALCRECT&&how&DT_RIGHT) 
		{
			RECT calc = {line.right-rtrim,line.top,line.right,line.bottom};
			UnionRect(&box,&box,&calc);			
		}
				
		//collapsing into subroutine in order to 
		//try to render DT_RIGHT here, backwards
		p->glyphs[g].selected = sel;
		int drawn = Ex_output_glyphs(p->glyphs[g],how,span,col,line,box,clipx,hdc);
		if(!drawn) break;		
								  
		//NEW: I think this has to be wrong
		//out = max(drawn,out);
		if(~how&DT_BOTTOM) //wild guess :(
		{
			out = line.top+drawn-box.top;
		}
		else out = max(drawn,out); //???	
				
		//Ex_output_glyphs is better positioned
		//to make this adjustment
		//line.left = calc.right; 

		//REMOVE ME?
		if(p->glyphs[g].nselected) //'\n' count
		{
			//hack: vertical spacing for hybrids
			line.top+=drawn; line.bottom-=drawn; 

			//2018: correct? fix for hybrid font
			margin = creturn;
		}
	}

	DDRAW::PopScene(); //EndScene?
	
	return out;
}

extern bool EX::releasing_output_font(const EX::Font **in, bool d3d_only)
{
	if(!in||!*in) return false;

	EX_OUTPUT_SQUARE_MANAGED_FONT_OR(in,return false)

	const EX::Font *p = *in; assert(p->refs);

	//TODO: this should be able to handle OpenGL as well as D3D9
	if(d3d_only||p->refs&&!--p->refs)
	{
		for(int i=0;p->glyphs[i];i++) 
		{
			if(!d3d_only)
			if(*p->glyphs[i].refs!=1) continue;

			*p->glyphs[i].glfont = 0; //OpenGL?

			if(*p->glyphs[i].d3dxfont)
			if(*p->glyphs[i].d3dxfont!=Ex_output_fontfailedtoload) 
			{
				ULONG paranoia = (*p->glyphs[i].d3dxfont)->Release();

				assert(paranoia==0);
			}
			*p->glyphs[i].d3dxfont = 0;

			if(*p->glyphs[i].d3dxsprite)
			if(*p->glyphs[i].d3dxsprite!=Ex_output_spritefailedtoload) 
			{
				ULONG paranoia = (*p->glyphs[i].d3dxsprite)->Release();

				assert(paranoia==0);
			}
			*p->glyphs[i].d3dxsprite = 0;
		}

		if(!d3d_only)
		{
			//REMOVE ME?
			p->destruct(); *in = 0;
		}
	}

	return true;
}

extern HFONT EX::need_a_client_handle_for_output_font(const EX::Font **in)
{
	if(!in) return 0;

	EX_OUTPUT_SQUARE_MANAGED_FONT_OR(in,return 0)

	return *in?(*in)->glyphs[0].hfont:0;
}

static wchar_t *Ex_output_uniface(const wchar_t *description);

extern const wchar_t *Ex_describing_font(const LOGFONTW&, const wchar_t*, va_list); //Ex.fonts.cpp

const wchar_t *EX::describing_output_font(LOGFONTW &logical_font, const wchar_t *description,...)
{
	va_list va; va_start(va,description);

	const wchar_t *out = Ex_describing_font(logical_font,description,va);
	
	if(out&&!EX::is_able_to_facilitate_font_output()) 
	{
		out = Ex_output_uniface(out);
	}

	return out?out:logical_font.lfFaceName;
}

extern const wchar_t *EX::need_a_logical_face_for_output_font(int cp, const char *log, const wchar_t *description)
{
	if(!description) return 0;

	static wchar_t out[LF_FACESIZE]; //LOGFONTW

	out[0] = out[LF_FACESIZE-1] = '\0'; //wcsncpy()

	const wchar_t *uni = Ex_output_uniface(description);
	
	if(!uni||!*uni)
	{
		EX::need_unicode_equivalent(cp,log,out,LF_FACESIZE);
	}
	else wcsncpy(out,uni,LF_FACESIZE-1);		

	return out?out:L""; //not good
}

static wchar_t *Ex_output_uniface(const wchar_t *in)
{
	if(!in) return 0;

	//warning: untested

	static wchar_t out[LF_FACESIZE]; //LOGFONTW

	const wchar_t *p = in;

	bool whitespace = false;

	wchar_t quote = 0; 

	while(*p) switch(*p)
	{
	case ' ': case '\t':
		
	case L'\u3000': //TODO: full Unicode

	case '\r': case '\n': 
		
		p++; whitespace = false; break; 

	case ',': return 0; 

	default: if(whitespace) return 0;

		if(*p>='0'&&*p<='9') //a number
		{
			wchar_t *q; wcstod(p,&q); 

			whitespace = true; if(p==q) return 0; 
			
			p = q; if(*p=='%') p++;	break;
		}
		else if(*p=='U'&&p[1]=='+') //Unicode range
		{
			return 0; 
		}
		else //assuming the font face in question
		{
			if(*p=='\''||*p=='"') quote = *p;

			if(quote) p++; goto out;
		}
	}

	return 0;

out: wchar_t *q = out;

	while(q-out<LF_FACESIZE-1) switch(*p)
	{
	case '\'': case '"':
				
		if(*p!=quote) return 0; //FALL THRU

	case ',': case '\0': 
		
		*q = '\0'; return out;

	case ' ': 		
	case L'\u3000': //TODO: full Unicode

		if(p[1]>='0'&&p[1]<'9')
		{
			*q = '\0'; return out;
		}
		else if(p[1]=='U'&&p[2]=='+')
		{
			*q = '\0'; return out;
		}
		else; //FALLTHRU

	default: *q++ = *p++;	
	}

	if(*p=='\0')
	{
		*q = '\0'; return out;
	}
	else return 0;
}

static RECT Ex_output_overlay_margin;

extern int Ex_output_overlay_focus = 0; //SomEx.cpp

static ID3DXFont *Ex_output_overlay_font = 0;

static WORD Ex_output_overlay_matrix[1+12] = {0,0,0,0,0,0,0,0,0,0,0,0,0}; 

static BYTE Ex_output_overlay_order[12] = {12,11,10,9,8,7,6,5,4,3,2,1};

static int Ex_output_overlay_input = 0; 

static void Ex_output_overlay(int f)
{
	assert(f>=1&&f<=12);

	if(f<1||f>12) return;

	int i = 0; while(i<12&&Ex_output_overlay_order[i]!=f) i++;

	assert(i<12); if(i>11||i==0) return;

	memmove(Ex_output_overlay_order+1,Ex_output_overlay_order,i);

	Ex_output_overlay_order[0] = f;
}

static int Ex_output_counter_pointer = 0;
static int Ex_output_counter_counter = 0; //hack

static void Ex_output_move_counter_pointer(int n, bool scroll=true)
{	
	if(n==0) return;

	char(&cnames)[1024][31] = SOM::L.counter_names;

	if(scroll)
	if(GetKeyState(VK_SCROLL)&1)
	{
		n*=Ex_output_counter_counter;
	}

	int c = Ex_output_counter_pointer;

	while(n>0&&c<1023)
	{
		while(c<1023)		
		if(*cnames[c+1]||SOM::L.counters[c+1])
		{
			Ex_output_counter_pointer = ++c; break;
		}
		else c++; n--;
	}
	
	while(n<0&&c>0)
	{
		while(c>0)		
		if(cnames[c-1]||SOM::L.counters[c-1])
		{
			Ex_output_counter_pointer = --c; break;
		}
		else c--; n++;
	}
}

static void Ex_output_scan_counter_pointer(int to)
{	
	int n = to-Ex_output_counter_pointer; 

	char(&cnames)[1024][31] = SOM::L.counter_names;

	int c = Ex_output_counter_pointer;

	while(n>0&&c<1023)
	{
		c++; n--;

		if(*cnames[c]||SOM::L.counters[c])
		{
			Ex_output_counter_pointer = c;
		}
	}
	
	while(n<0&&c>0)
	{
		c--; n++;
				
		if(*cnames[c]||SOM::L.counters[c])
		{
			Ex_output_counter_pointer = c;
		}		
	}

	if(Ex_output_overlay_focus!=1
	||!Ex_output_overlay_input) return;

	if(!*cnames[c]&&!SOM::L.counters[c])
	{
		Ex_output_overlay_input = 0; return;
	}

	int i;
	for(i=Ex_output_overlay_input-1;c&&i;c--)
	{
		if(*cnames[c]||SOM::L.counters[c])
		{
			Ex_output_counter_pointer = c-1; i--;
		}
	}
	while(i&&Ex_output_overlay_input>1)
	{
		if(*cnames[c]||SOM::L.counters[c])
		{
			c++; Ex_output_overlay_input--; i--;
		}
	}
}

//TODO: make f1/2 generic with handlers
static void Ex_output_f1_counter_battery()
{
	if(!SOM::field) return;

	static bool counterless = false; 
	
	if(counterless) return; //hack: optimization

	char(&cnames)[1024][31] = SOM::L.counter_names;

	Ex_output_margin = Ex_output_overlay_margin;  

	Ex_output_format = DT_LEFT|DT_TOP|DT_NOCLIP|DT_SINGLELINE;
	
	Ex_output_margin.left+=120; //hack

	EX_OUTPUT_PRINT(L" ") //blank line

	if(Ex_output_counter_pointer<0)
	{
		Ex_output_counter_pointer = 0;
	}
	else if(Ex_output_counter_pointer>=1024)
	{
		Ex_output_counter_pointer = 1023;
	}

	wchar_t *num = EX::numlock()?L"<num> ":L"";
//	wchar_t *caps = GetKeyState(VK_CAPITAL)&1?L"<caps> ":L"";
	wchar_t *scroll = GetKeyState(VK_SCROLL)&1?L"<scroll> ":L"";

	if(Ex_output_counter_counter) //hack
	EX_OUTPUT_PRINT_F(L" counters %s%s",num,scroll) 	
	else EX_OUTPUT_PRINT(L" ");	//blank line
	
	int ln = 0, i = -1;
	for(int c=Ex_output_counter_pointer;c<1024
	&&Ex_output_margin.top<Ex_output_margin.bottom;c++)
	{
		if(*cnames[c]||SOM::L.counters[c])
		{			
			const wchar_t *unicode = 
			EX::need_unicode_equivalent(932,cnames[c]);

			int printed = 0;
			if(ln==Ex_output_overlay_input-1
			&&Ex_output_overlay_focus==1
			&&EX::inquiring_regarding_input(false)=='pos')
			{
				const wchar_t *r = EX::displaying_unicode_input(L"_",5);

				if(r[-1]==0)
				{
					wchar_t flash[32] = L"";
					
					if(!*num)
					{					
						_itow(SOM::L.counters[c],flash,10);						
					}
					else _itow(c,flash,10);

					r = EX::displaying_unicode_input(flash,5);
				}

				printed = EX_OUTPUT_PRINT_F(L"[%-5s]%s",r,unicode) 

				if(printed) i = c; //input (saving for later)
			}
			else if(EX::numlock())
			{
				printed = EX_OUTPUT_PRINT_F(L" %-5d %s",c,unicode)
			}
			else printed = EX_OUTPUT_PRINT_F(L" %-5d %s",SOM::L.counters[c],unicode)

			ln+=printed;
		}
	}
		
	if(ln<Ex_output_overlay_input)
	{
		Ex_output_overlay_input = 1; //hack: rotate
	}

	if(i>=0)
	if(Ex_output_overlay_input
	&&Ex_output_overlay_focus==1
	&&EX::inquiring_regarding_input())
	{
		double r = EX::retrieving_numerical_input(-1); 

		if(r>=0) 
		{
			if(!*num)
			{					
				if(r<=15) SOM::L.counters[i] = r;
			}
			else Ex_output_scan_counter_pointer(r);

			EX::requesting_ascii_input('new'); 		
		}
	}

	static bool paranoia = false;

	if(ln==0&&!paranoia)
	{
		counterless = true; 
	}
	else Ex_output_counter_counter = ln;

	paranoia = true;
}

static int Ex_output_event_pointer = 0;
static int Ex_output_event_counter = 0; //hack

static const char *Ex_output_event_records(int c)
{	
	if(SOM::mpx>=0&&SOM::mpx<=63)
	{
		const char *out = 0;
		if(!SOM::DATA::Map::sys.ezt) 
		SOM::DATA::Map::sys.ezt->open();
		if(!SOM::DATA::Map[SOM::mpx].evt) 
		SOM::DATA::Map::Evt::open(SOM::mpx);
		if(!SOM::DATA::Map[SOM::mpx].evt->error)
		out = SOM::DATA::Map[SOM::mpx].evt->records[c];	   
		if(out&&((BYTE*)out)[31]==0xFF)
		if(!SOM::DATA::Map::sys.ezt->error)
		out = SOM::DATA::Map::sys.ezt->records[c];
		if(out) return out;
	}
	static char error[252] = {'\0'}; 
	return error;
}

static void Ex_output_move_event_pointer(int n, bool scroll=true)
{	
	if(n==0) return;

	if(scroll)
	if(GetKeyState(VK_SCROLL)&1)
	{
		n*=Ex_output_event_counter;
	}

	int c = Ex_output_event_pointer;

	while(n>0&&c<1023)
	{
		while(c<1023) if(1)		
		if(SOM::L.leafnums[c+1]
		||*Ex_output_event_records(c+1))
		{
			Ex_output_event_pointer = ++c; break;
		}
		else c++; n--;
	}
	
	while(n<0&&c>0)
	{
		while(c>0) if(1)		
		if(SOM::L.leafnums[c-1]
		||*Ex_output_event_records(c-1))
		{
			Ex_output_event_pointer = --c; break;
		}
		else c--; n++;
	}
}

static void Ex_output_scan_event_pointer(int to)
{	
	int c = Ex_output_event_pointer;
	int n = to-Ex_output_event_pointer; 	

	while(n>0&&c<1023)
	{
		c++; n--;

		if(SOM::L.leafnums[c]
		||*Ex_output_event_records(c))
		{
			Ex_output_event_pointer = c;
		}
	}
	
	while(n<0&&c>0)
	{
		c--; n++;
				
		if(SOM::L.leafnums[c]
		||*Ex_output_event_records(c))
		{
			Ex_output_event_pointer = c;
		}		
	}

	if(Ex_output_overlay_focus!=2
	  ||!Ex_output_overlay_input) return;

	if(!SOM::L.leafnums[c]
	&&!*Ex_output_event_records(c))
	{
		Ex_output_overlay_input = 0; return;
	}

	int i;
	for(i=Ex_output_overlay_input-1;c&&i;c--)
	{
		if(SOM::L.leafnums[c-1]
		||*Ex_output_event_records(c-1))
		{
			Ex_output_event_pointer = c-1; i--;
		}
	}

	while(i&&Ex_output_overlay_input>1)
	{
		if(SOM::L.leafnums[c+1]
		||*Ex_output_event_records(c+1))
		{
			c++; Ex_output_overlay_input--; i--;
		}
	}
}

//TODO: make f1/2 generic with handlers
static void Ex_output_f2_event_battery()
{
	if(!SOM::field) return;

	if(SOM::mpx<0||SOM::mpx>63) return;

	Ex_output_margin = Ex_output_overlay_margin;  
	Ex_output_format = DT_LEFT|DT_TOP|DT_NOCLIP|DT_SINGLELINE;	
	Ex_output_margin.left+=360; //hack

	EX_OUTPUT_PRINT(L" ") //blank line

	if(Ex_output_event_pointer<0)
	{
		Ex_output_event_pointer = 0;
	}
	else if(Ex_output_event_pointer>=1024)
	{
		Ex_output_event_pointer = 1023;
	}

	wchar_t *num = EX::numlock()?L"<num> ":L"";
//	wchar_t *caps = GetKeyState(VK_CAPITAL)&1?L"<caps> ":L"";
	wchar_t *scroll = GetKeyState(VK_SCROLL)&1?L"<scroll> ":L"";

	if(Ex_output_event_counter) //hack
	EX_OUTPUT_PRINT_F(L" events %s%s",num,scroll) 
	else EX_OUTPUT_PRINT(L" ");	//blank line

	int ln = 0, i = -1;
	for(int c=Ex_output_event_pointer;c<1024&&
	Ex_output_margin.top<Ex_output_margin.bottom;c++)
	{
		const char *record_c = Ex_output_event_records(c);

		if(SOM::L.leafnums[c]||*record_c)
		{				
			const wchar_t *unicode = 
			EX::need_unicode_equivalent(932,record_c);

			int printed = 0;
			if(ln==Ex_output_overlay_input-1
			&&Ex_output_overlay_focus==2
			&&EX::inquiring_regarding_input(false)=='pos')
			{
				const wchar_t *r = EX::displaying_unicode_input(L"_",5);

				if(r[-1]==0)
				{
					wchar_t flash[32] = L"";
					
					if(!*num)
					{					
						_itow(SOM::L.leafnums[c],flash,10);						
					}
					else _itow(c,flash,10);

					r = EX::displaying_unicode_input(flash,5);
				}

				printed = EX_OUTPUT_PRINT_F(L"[%-5s]%s",r,unicode) 				

				if(printed) i = c; //input (saving for later)
			}
			else if(EX::numlock())
			{
				printed = EX_OUTPUT_PRINT_F(L" %-5d %s",c,unicode)
			}
			else 
			{
				printed = EX_OUTPUT_PRINT_F(L" %-5d %s",SOM::L.leafnums[c],unicode)
			}

			ln+=printed;
		}
	}
		
	if(ln<Ex_output_overlay_input)
	{
		Ex_output_overlay_input = 1; //hack: rotate
	}

	if(i>=0)
	if(Ex_output_overlay_input
	&&Ex_output_overlay_focus==2
	&&EX::inquiring_regarding_input())
	{
		double r = EX::retrieving_numerical_input(-1); 

		if(r>=0) 
		{
			if(!*num)
			{	
				if(r<=15) SOM::L.leafnums[i] = r;
			}
			else Ex_output_scan_event_pointer(r);

			EX::requesting_ascii_input('new'); 		
		}
	}

	Ex_output_event_counter = ln;
}

static HRESULT (__stdcall*HasExpandedResources)(BOOL*);
static void Ex_output_f5_system_information()
{
	EX::INI::Output tt;

	Ex_output_margin = Ex_output_overlay_margin;
	Ex_output_format = DT_LEFT|DT_TOP|DT_NOCLIP|DT_SINGLELINE;

	EX_OUTPUT_PRINT(EX::shadermodel?EX::shadermodel:L" ");
	EX_OUTPUT_PRINT(L" ") //blank line

	wchar_t *emu_ = SOM::emu?L"emu ":L"";
	wchar_t *log_ = SOM::log?L"log ":L"";

	wchar_t listen[34] = L"";
	if(0&&EX::debug)
	swprintf(listen,L"(%.2f) ",SOM::motions.step*1000);
	else if(DSOUND::listenedFor>3) //watching
	swprintf(listen,L"(%d) ",DSOUND::listenedFor);

	char her = '\0';
	if(HasExpandedResources) //Game Mode? seems to be always on
	{
		BOOL b = 0; HasExpandedResources(&b); if(b) her = '*';
	}
	if(0&&EX::debug)
	{
		float t = 0;
		enum{ n=EX_ARRAYSIZEOF(SOM::onflip_triple_time) };
		for(int i=n;i-->0;)
		t+=SOM::onflip_triple_time[i];
		t/=n; t/=1000/DDRAW::refreshrate; t*=DDRAW::refreshrate;
		EX_OUTPUT_PRINT_F(L"%.2f fps %s%s%s",t,listen,emu_,log_)
	}
	else if(1||EX::debug) //2021
	{
		EX_OUTPUT_PRINT_F(L"%d fps %s%s%s%c",DDRAW::fps,listen,emu_,log_,her)
	}
	else if(EX::clip>=10&&_finite(EX::clip))
	EX_OUTPUT_PRINT_F(L"%d fps %s%s%s%c",int(EX::clip),listen,emu_,log_,her)	
	else EX_OUTPUT_PRINT_F(L"<5 fps %s%s%c",emu_,log_,her)

	//if(tt->do_f5_triple_buffer_fps)
	{
		enum{ n=EX_ARRAYSIZEOF(SOM::onflip_triple_time) }; if(n!=1) 
		{
			bool trip = tt->do_f5_triple_buffer_fps;
			if(DDRAW::noFlips>500)
			for(int i=0;i<n;i++)
			if(SOM::onflip_triple_time[i]>(EX::debug?22:33)) 
			trip = true;
			if(trip)
			{
				wchar_t ln[33*n],*p=ln; for(int i=0;i<n;i++)
				p+=swprintf(p,L"%d.",SOM::onflip_triple_time[i]);
				*--p = '\0';
				EX_OUTPUT_PRINT(ln)
			}
		}	
	}

	EX_OUTPUT_PRINT(L" ") //blank line

	//NEW: [2] is masterVol, I'm not sure how to interpret
	//it frankly since the PrimaryBuffer mixes it, i.e. is
	//mixing just subtraction?
	int dBs[2] = 
	{
		SOM::decibels[0]+SOM::decibels[2]-0.5f,
		SOM::decibels[1]+SOM::decibels[2]-0.5f,
	};
	if(dBs[0]!=dBs[1])
	EX_OUTPUT_PRINT_F(L"%d/%d -dBs",-dBs[0],-dBs[1])	
	else EX_OUTPUT_PRINT_F(L"%d -dBs",-dBs[0])
	
	static const char *i3dl2[] =
	{
	//"studio", //i.e. soundproof (how to communicate this??)
	"no fx",
	"reverb", //DSFX_I3DL2_ENVIRONMENT_PRESET_GENERIC
    "padded cell",
    "room",
    "bathroom",
    "living room",
    "stone room",
    "auditorium",
    "concert hall",
    "cave",
    "arena",
    "hangar",
    "carpeted hallway",
    "hallway",
    "stone corridor",
    "alley",
    "forest",
    "city",
    "mountains",
    "quarry",
    "plain",
    "parking lot",
    "sewer pipe",
    "underwater", //(23)
	//note: these are said to be "'musical'" and so may not be
	//appropriate (a "room" and "hall" mean performance areas)
    "'small room'", //"approximately 5 meter room"
    "'large room'", //"approximately 10 meter room"
    "'large room'", //"A large size room suitable for live performances"
    "'medium hall'", //A medium size concert hall
    "'large hall'", //A large size concert hall suitable for a full orchestra
    "'plate reverb'" //A plate reverb simulation (29)
	};
	int x = DSOUND::doReverb_mask;
	EX_OUTPUT_PRINT_F(L"%hs%hs",i3dl2[x&1?0:DSOUND::doReverb_i3dl2[0]],x==2?"!":"");
		
	EX_OUTPUT_PRINT(L" ") //blank line

	if(EX::kmph!=EX::kmph2)
	EX_OUTPUT_PRINT_F(L"%02.0f km/h+%1.0f",EX::kmph2,EX::kmph-EX::kmph2)
	else EX_OUTPUT_PRINT_F(L"%02.0f km/h",EX::kmph)
	EX_OUTPUT_PRINT_F(L"E= %0.2f m",EX::e)
	EX_OUTPUT_PRINT_F(L"%.2f/%.2f c",SOM::arch,fabsf(SOM::slope)) //-0?

	#ifdef EX_DBGMSG_DEFINED
	if(/*EX::debug&&*/*Ex_output_dbgmsg)
	{	
		Ex_output_format&=~DT_SINGLELINE; //2017
		{
			EX_OUTPUT_PRINT(L" ") //blank line				
			//EX_OUTPUT_PRINT_F(L"%S",Ex_output_dbgmsg) //debugging
			EX_OUTPUT_PRINT(Ex_output_dbgmsg_str.c_str()) //debugging
		}
		Ex_output_format|=DT_SINGLELINE;
	}
	#endif

	EX_OUTPUT_PRINT(L" ") //blank line

//#ifdef _DEBUG
#define _z L"%dz"
//#else
//#define _z
//#endif			

	if(EX::is_captive())	
	EX_OUTPUT_PRINT_F(L"%.0fx%.0fy"_z,EX::mouse[0],EX::mouse[1],SOM::cursorZ)
	else EX_OUTPUT_PRINT_F(L"%dx%dy"_z L"%c",EX::x,EX::y,SOM::cursorZ,EX::debug&&EX::cursor?'!':0)

#undef _z

	bool analog = true;			   
					
	EX_OUTPUT_PRINT(L" ");

	for(int i=0,j=0;i<256&&j<256;i++,j++)
	{			
		if(0) //old way
		{
			if(!EX::keys[i]&&!EX::polls[j]) break;			
			//EX_OUTPUT_PRINT_F(L"%02x %d",EX::keys[i],EX::polls[j])
			EX_OUTPUT_PRINT_F(L"%02x",EX::keys[i])

			if(!EX::keys[i]) i--; if(!EX::polls[j]) j--; 
		}
		else if(EX::keys[i]&&EX::polls[j]) //new way
		{	
			wchar_t s[16+1] = L""; BYTE *k = EX::keys+i; if(i+8>=256) break;

			swprintf(s,L"%02x%02x%02x%02x%02x%02x%02x",k[0],k[1],k[2],k[3],k[4],k[5],k[6],k[7]);

			int ii = i; while(EX::keys[i]) i++; s[min(i-ii,8)*2] = '\0'; 

			EX_OUTPUT_PRINT_F(s)			
		}
		else break;

		analog = false;
	}

	if(analog) EX_OUTPUT_PRINT(L"analog");

	for(int i=0,blank=0;i<EX_JOYPADS;i++) 
	{
		if(EX::Joypads[i].active) 
		for(int j=0;j<8;j++) if(EX::Joypads[i].analog[j])
		{
			wchar_t s[8] = L"|||||||";

			if(EX::Joypads[i].analog[j]&1)
			{
				for(int k=1;k<7;k++) s[k] = EX::Joypads[i].analog[j]&1<<k?'|':' ';
			}
			else wcscpy(s,L"1/10"); //NEW

			if(blank++==0) EX_OUTPUT_PRINT(L" ") 

			EX_OUTPUT_PRINT(s);
		}
	}
		
	//if(EX::INI::Option()->do_pedal) //deprecated
	{		
		EX_OUTPUT_PRINT(L" ") 

		bool pedals = true; 
		for(int i=0;i<EX_AFFECTS;i++) 
		{							
			if(EX::Affects[i].active) 
			for(int j=0;j<6;j++) if(EX::Affects[i].analog[j]) //&1
			{
				wchar_t s[8] = L"|||||||";

				if(EX::Affects[i].analog[j]&1)
				{
					for(int k=1;k<7;k++) s[k] = EX::Affects[i].analog[j]&1<<k?'|':' ';
				}
				else swprintf(s,L"1/%d",EX::Affects[i].analog[j]<<1>>2);

				EX_OUTPUT_PRINT(s);

				pedals = false;
			}
		}

		if(pedals) EX_OUTPUT_PRINT(L"pedals");		
	}
}

static inline float Ex_output_f6_zero(float f)
{
	return fabs(f)>0.009?f:0;
}
static const wchar_t *Ex_output_f6_equip_num(unsigned char i)
{
	if(SOM::PARAM::Item.prm->records[i][0]=='-'
	 &&SOM::PARAM::Item.prm->records[i][1]=='-'
	 &&SOM::PARAM::Item.prm->records[i][2]=='\0') return L"--";

	static wchar_t out[4]; return _itow(i,out,10);
}
static const wchar_t *Ex_output_f6_magic_num(unsigned char i)
{
	i = i>31?0xFF:SOM::L.magic32[i];

	if(SOM::PARAM::Magic.prm->records[i][0]=='-'
	 &&SOM::PARAM::Magic.prm->records[i][1]=='-'
	 &&SOM::PARAM::Magic.prm->records[i][2]=='\0') return L"--";

	static wchar_t out[4]; return _itow(i,out,10);
}
//todo: have handlers fill this screen out
extern float Ex_output_f6_head[7+3] = {};
static void Ex_output_f6_player_information()
{	
	Ex_output_margin = Ex_output_overlay_margin;
	Ex_output_format = DT_RIGHT|DT_TOP|DT_NOCLIP|DT_SINGLELINE;

	EX_OUTPUT_PRINT(L" |") //blank line	
	EX_OUTPUT_PRINT_F(L"%s |",(const wchar_t*)SOM::context)

	if(!SOM::field) return;

	EX_OUTPUT_PRINT(L" |") //blank line

	static int powmax = 0, magmax = 0;

	int pow = SOM::L.pcstatus[SOM::PC::p]; 
	int mag = SOM::L.pcstatus[SOM::PC::m]; 
	
	//if(powmax!=pow)
	{
		powmax = pow; 
		pow = float(min(powmax?pow:5000,5000))/5000*20;				
	}
	//else pow = 20; 

	//2017: The "Nothing" extensions will twart this
	//most of the time.
	bool spell = SOM::L.pcequip[7]<32;
	if(spell) //if(magmax!=mag&&spell)
	//2017: I think this is a special state for SOM. It's 
	//only wrong until a magic is used, but still displays
	//as empty. 
	if(0!=SOM::L.pcstatus[SOM::PC::mp])
	{
		magmax = mag;
		float wtf = min(magmax?mag:5000,5000);
		assert(wtf<=5000); //WTH: The outer assert (below) is failing?!?!?!?!?!
		mag = wtf/5000*20;	
	}
	else mag = 20;

	wchar_t powgauge[] = L"[||||||||||||||||||||]"; //20
	wchar_t maggauge[] = L"[||||||||||||||||||||]"; 

	//WTH???? mag is 5000? THAT'S IMPOSSIBLE!
	assert(pow>=0&&pow<=20&&mag>=0&&mag<=20);
	//>=: Sometimes it's going over??? On Load? 2017.
	if(pow>=20) powgauge[0] = '\0';
	else for(int i=0;i<pow;i++) powgauge[i+1] = ' ';
	if(mag>=20) maggauge[0] = '\0';
	else for(int i=0;i<mag;i++) maggauge[i+1] = ' ';
				
	EX_OUTPUT_PRINT_F(L"%s %d HP |",powgauge,SOM::L.pcstatus[SOM::PC::hp])
	EX_OUTPUT_PRINT_F(L"%s %d MP |",maggauge,SOM::L.pcstatus[SOM::PC::mp])

	auto &xxx = som_932w_States;
	if(int x=SOM::L.pcstatus[SOM::PC::xxx])
	if(SOM::japanese())
	EX_OUTPUT_PRINT_F(L"%s%s%s%s%s |",x&1?xxx[0]:L"",x&2?xxx[1]:L"",x&4?xxx[2]:L"",x&8?xxx[3]:L"",x&16?xxx[4]:L"",x)
	else
	EX_OUTPUT_PRINT_F(L"%s%s%s%s%s |",x&1?L" poison":L"",x&2?L" palsy":L"",x&4?L" dark":L"",x&8?L" curse":L"",x&16?L" slow":L"",x)	
	else 
	EX_OUTPUT_PRINT_F(L"-- |")

	EX_OUTPUT_PRINT(L" |") //blank line

	//NEW: heading incorporates motion effects
	float xz = SOM::emu?SOM::uvw[1]:SOM::heading; 

	float turn = fmod(xz+M_PI*4,M_PI*2);

	float deg = turn*180/M_PI;
		
	wchar_t *by[] = {L"NW",L"SW",L"SE",L"NE",L"??"};		 

	if(deg<0) deg = 0; if(deg>=360) deg = 359.99998f;

	float deviation[] = {deg,180-deg,deg-180,360-deg,0};	

	float dev = Ex_output_f6_zero(deviation[unsigned(deg)/90%4]);

	//u00B0: shift_jis degree symbol
	EX_OUTPUT_PRINT_F(L"heading %s |",by[unsigned(deg)/90%4])
	EX_OUTPUT_PRINT_F(L"%.1f\u00B0 |",dev) 

	//NEW: incorporate bounce and buckle effects
	float nod = SOM::emu?SOM::uvw[0]:SOM::incline; //??? 

	deg = nod*180/M_PI;	//deg = SOM::uvw[0]*180/M_PI;

	const wchar_t *incline = L"=0";
		
	if(int(fabsf(deg))) incline = deg>0?L">0":L"<0";

	//u00B0: shift_jis degree symbol
	EX_OUTPUT_PRINT_F(L"incline %s |",incline)
	EX_OUTPUT_PRINT_F(L"%.1f\u00B0 |",Ex_output_f6_zero(fabsf(deg))) 

	EX_OUTPUT_PRINT(L" |") //blank line

	if(SOM::mpx==SOM::mpx2) //2022
	EX_OUTPUT_PRINT_F(L"map %02d |",SOM::mpx)
	else
	EX_OUTPUT_PRINT_F(L"map %02d/%02d |",SOM::mpx,SOM::mpx2)

	EX_OUTPUT_PRINT(L" |") //blank line

	//REMINDER: -1.#IND0000 prints "-1.#J". In the past this means
	//SOM::L.pcstate[0] is NaN. I'm not sure where this originates
	//som_mocap::engine::operator() repairs it, so it may never be
	//shown long enough to be seen
	EX_OUTPUT_PRINT(L"mapcoords |")
	EX_OUTPUT_PRINT_F(L"%8.2fx |",SOM::xyz[0]) 
	EX_OUTPUT_PRINT_F(L"%8.2fy |",SOM::xyz[2])
	EX_OUTPUT_PRINT(L"elevation |")
	EX_OUTPUT_PRINT_F(L"%+8.2f |",SOM::xyz[1]); 
	EX_OUTPUT_PRINT(L"eyecoords |")	
	EX_OUTPUT_PRINT_F(L"%+8.2f |",Ex_output_f6_zero(SOM::eye[0]))
	EX_OUTPUT_PRINT_F(L"%+8.2f |",Ex_output_f6_zero(SOM::eye[2]))
	EX_OUTPUT_PRINT_F(L"%+8.2f |",Ex_output_f6_zero(SOM::eye[1]))
	EX_OUTPUT_PRINT_F(L"%+8.2f |",SOM::eye[3]?SOM::eye[3]:-0.001f)
	if(DDRAW::inStereo)
	{
		EX_OUTPUT_PRINT(L"setcoords |")		
		int n = 7;
		for(int i=7;i<10;i++) if(Ex_output_f6_head[i]) //6DOF?
		n = 10;
		for(int i=4;i<n;i++)
		EX_OUTPUT_PRINT_F(L"%+8.2f |",Ex_output_f6_zero(Ex_output_f6_head[i])) 		
	}	

	//0: thinking looks better without?
	if(0&&SOM::height>600)
	{
	EX_OUTPUT_PRINT(L" |") //blank line

	EX_OUTPUT_PRINT_F(L"level %d |",SOM::L.pcstatus[SOM::PC::lv])

	EX_OUTPUT_PRINT(L" |") //blank line

	EX_OUTPUT_PRINT(L"points |")
	EX_OUTPUT_PRINT_F(L"%d |",*(DWORD*)&SOM::L.pcstatus[SOM::PC::pts])
	EX_OUTPUT_PRINT(L"credits |")
	EX_OUTPUT_PRINT_F(L"%d |",SOM::L.pcstatus[SOM::PC::c])	
	EX_OUTPUT_PRINT(L"strength |")
	int diff = int(SOM::L.pcstatus[SOM::PC::str])-SOM::L.pcstatus[SOM::PC::_str];

	if(diff)	
	EX_OUTPUT_PRINT_F(L"%d%+d |",SOM::L.pcstatus[SOM::PC::_str],diff)
	else EX_OUTPUT_PRINT_F(L"%d |",SOM::L.pcstatus[SOM::PC::str])

	EX_OUTPUT_PRINT(L"magic |")

	diff = int(SOM::L.pcstatus[SOM::PC::mag])-SOM::L.pcstatus[SOM::PC::_mag];

	if(diff)
	EX_OUTPUT_PRINT_F(L"%d%+d |",SOM::L.pcstatus[SOM::PC::_mag],diff)
	else EX_OUTPUT_PRINT_F(L"%d |",SOM::L.pcstatus[SOM::PC::mag])	

	//if(!SOM::L.pcequip) return;

	EX_OUTPUT_PRINT(L" |") //blank line
																			   
	if(!SOM::PARAM::Item.prm->open()
	 ||!SOM::PARAM::Magic.prm->open()||!SOM::PARAM::Sys.dat->open()) return;
		
	if(EX::numlock()) //return: assuming the last thing that there is to do
	{																  
	EX_OUTPUT_PRINT_F(L"%s A |",Ex_output_f6_equip_num(SOM::L.pcequip[1])) //A
	EX_OUTPUT_PRINT_F(L"%s T |",Ex_output_f6_equip_num(SOM::L.pcequip[2])) //T
	EX_OUTPUT_PRINT_F(L"%s M |",Ex_output_f6_equip_num(SOM::L.pcequip[3])) //M					
	EX_OUTPUT_PRINT_F(L"%s L |",Ex_output_f6_equip_num(SOM::L.pcequip[4])) //L
	EX_OUTPUT_PRINT_F(L"%s O |",Ex_output_f6_equip_num(SOM::L.pcequip[5])) //O
	EX_OUTPUT_PRINT_F(L"%s Y |",Ex_output_f6_equip_num(SOM::L.pcequip[6])) //Y
	EX_OUTPUT_PRINT_F(L"%s 7 |",Ex_output_f6_equip_num(SOM::L.pcequip[0])) //Z
	EX_OUTPUT_PRINT_F(L"%s H |",Ex_output_f6_magic_num(SOM::L.pcequip[7])) //H
	return;
	}									

	const char *main = SOM::PARAM::Item.prm->records[SOM::L.pcequip[0]];
	const char *head = SOM::PARAM::Item.prm->records[SOM::L.pcequip[1]];
	const char *core = SOM::PARAM::Item.prm->records[SOM::L.pcequip[2]];
	const char *arms = SOM::PARAM::Item.prm->records[SOM::L.pcequip[3]];
	const char *legs = SOM::PARAM::Item.prm->records[SOM::L.pcequip[4]];
	const char *acc0 = SOM::PARAM::Item.prm->records[SOM::L.pcequip[5]];
	const char *acc1 = SOM::PARAM::Item.prm->records[SOM::L.pcequip[6]];
	
	//idea: it would be sweet to have a custom font with our own icons 
	EX_OUTPUT_PRINT_F(L"%s A |",EX::need_unicode_equivalent(932,head)) //A
	EX_OUTPUT_PRINT_F(L"%s T |",EX::need_unicode_equivalent(932,core)) //T
	EX_OUTPUT_PRINT_F(L"%s M |",EX::need_unicode_equivalent(932,arms)) //M					
	EX_OUTPUT_PRINT_F(L"%s L |",EX::need_unicode_equivalent(932,legs)) //L
	EX_OUTPUT_PRINT_F(L"%s O |",EX::need_unicode_equivalent(932,acc0)) //O
	EX_OUTPUT_PRINT_F(L"%s Y |",EX::need_unicode_equivalent(932,acc1)) //Y
	//todo: use Z when two-handed is mode enabled (do_z) via the Z key
	//todo: consider using @ again to symbolize an empty hand and fist
	EX_OUTPUT_PRINT_F(L"%s 7 |",EX::need_unicode_equivalent(932,main)) //Z
		
	if(!SOM::PARAM::Magic.prm->open()||!SOM::PARAM::Sys.dat->open()) return;
		
	int magic = SOM::L.pcequip[7]>31?0xFF:SOM::L.magic32[SOM::L.pcequip[7]];

	const char *sub0 = SOM::PARAM::Magic.prm->records[magic];

	//todo: use N whenever magic is disable (eg. if using shield mode)
	//Reminder: was using $ for a while to symbolize a staff in a hand
	EX_OUTPUT_PRINT_F(L"%s H |",EX::need_unicode_equivalent(932,sub0)) //H
	}
}
//todo: have handlers fill this screen out
static void Ex_output_f7_enemy_information()
{								
	if(!SOM::field) return; 

	int x = SOM::Versus(); 	 
	int ai = SOM::Versus[x].enemy;
	int aj = SOM::Versus[x].npc;	
	int max = SOM::Versus[x].HP; //2017
	int hit = SOM::Versus[x].combo;

	enum{ out_s = 256 }; wchar_t out[out_s+2];
	
	switch((int)SOM::context) //2019
	{
	case SOM::Context::taking_item:
	case SOM::Context::browsing_menu:
	case SOM::Context::browsing_shop:
	{
		extern bool som_state_browsing_menu;
		extern int som_hacks_inventory_item;		
		const char *tag = 0; 
		int i = som_hacks_inventory_item; 
		if(i<0&&i>=-32)
		{
			i = SOM::L.magic32[-i-1];
			if(SOM::PARAM::Magic.prm->open())
			tag = SOM::PARAM::Magic.prm->records[i];
		}
		else if(i<250)
		{
			if(SOM::PARAM::Item.prm->open())
			tag = SOM::PARAM::Item.prm->records[i];
		}
		else return;
		const wchar_t *unicode = 
		tag&&*tag?EX::need_unicode_equivalent(932,tag):L"<no-name-given>";
		swprintf(out,L"%ls\n%d",unicode,i);
		goto out;	
	}
	case SOM::Context::playing_game: 
		
		if(!SOM::field) default: return;
	}

	wcscpy(out,L"Inflict damage upon an enemy to begin");

	if(ai>=0||aj>=0) if(max||hit) //2017
	{	
		SOM::MDL *mdl = 0; //debugging

		WORD cur = 0;
		const char *tag = 0; if(ai>=0)
		{
			cur = SOM::L.ai[ai][SOM::AI::hp]; 
			WORD enemy = SOM::L.ai[ai][SOM::AI::enemy]; 		
			if(SOM::PARAM::Enemy.prm->open())
			tag = enemy<1024?SOM::PARAM::Enemy.prm->records[enemy]:0;

			if(1&&EX::debug) 
			mdl = (SOM::MDL*)SOM::L.ai[ai][SOM::AI::mdl];
		}
		else if(aj>=0) //NEW: NPC information
		{
			cur = SOM::L.ai2[aj][SOM::AI::hp2]; 
			WORD npc = SOM::L.ai2[aj][SOM::AI::npc]; 		
			if(SOM::PARAM::NPC.prm->open()) 
			tag = aj<1024?SOM::PARAM::NPC.prm->records[npc]:0;

			if(0&&EX::debug) 
			mdl = (SOM::MDL*)SOM::L.ai[ai][SOM::AI::mdl2];
		}

		if(cur||hit) //2017
		{
			const wchar_t *unicode = tag?L"<no-name-given>":L"<not-an-enemy>"; 			
			if(tag&&*tag) unicode = EX::need_unicode_equivalent(932,tag);

			wchar_t *end = out+swprintf_s(out,L"%s\nHP %d/%d",unicode,cur,max);
		
			if(SOM::Versus[x].timer>0) //the new way
			{
				if(SOM::Versus[x].combo&&end>out) //paranoia
				end+=swprintf_s(end,out+out_s-end,L" %d",-SOM::Versus[x].combo);			
				if(SOM::Versus[x].multi>1&&end>out)
				end+=swprintf_s(end,out+out_s-end,L" hit %d time(s)",SOM::Versus[x].multi);			
			}
			/*multi-hit: doesn't seem to happen on its own
			if(end>out)	for(int i=0;i<vs_s;i+=2) if(vs[i]) //the old way
			{
				if(vs[i]>0) *end++ = '+'; *end = '\0'; _itow_s(vs[i],end,out+out_s-end,10);
				end+=wcslen(end); *end++ = ' '; *end = '\0';
			}*/
			if(end<=out) //paranoia
			wcscpy(out,L"Inflict damage upon an enemy to begin");
			else if(ai>=0) end+=swprintf_s(end,out+out_s-end,L"\nAI %d",ai);
			else if(aj>=0) end+=swprintf_s(end,out+out_s-end,L"\nNPC %d",aj);

			if(mdl&&EX::debug) //debugging
			{
				end+=swprintf_s(end,out+out_s-end,L"\nclip %d (%d)",
				mdl->ext.clip.layers&0x3f,mdl->ext.clip.noentry);
				/*
				end+=swprintf_s(end,out+out_s-end,L"\nclip %f (%f)",
				mdl->ext.clip.soft2[3],mdl->ext.clip.reshape2);
				end+=swprintf_s(end,out+out_s-end,L"\nclinging %f (%f)",
				mdl->ext.clip.clinging,mdl->ext.clip.npcstep);*/
				end+=swprintf_s(end,out+out_s-end,L"\nfalling %f (%f)",
				mdl->ext.clip.falling,mdl->ext.clip.falling2);
			}
		}
	}
out:
	Ex_output_margin = Ex_output_overlay_margin;  

	Ex_output_format = DT_LEFT|DT_TOP|DT_NOCLIP|DT_CALCRECT;

	EX_OUTPUT_PRINT(out); //calculating

	Ex_output_margin.top = //bottom left corner
	Ex_output_calculation.top-Ex_output_calculation.bottom;
	Ex_output_margin.top+=Ex_output_margin.bottom;
	Ex_output_margin.top-=2; //DDRAW::fxInflate

	Ex_output_format&=~DT_CALCRECT;

	EX_OUTPUT_PRINT(out);
}

static unsigned Ex_output_f11_blank = 0;

static int Ex_output_f11_linesperpage = 0; 

static const char *Ex_output_f11_error = 0;

static void Ex_output_have_f11_input()
{
	const char *input = EX::retrieving_ascii_input(); 

	if(!input)
	{
		Ex_output_overlay_input = 0; return;
	}

	Ex_output_f11_error = "";

	if(!*input) //blank return
	{
		EX::scan_view_of_memory(+1); return;
	}
	else if(input[0]=='\\'&&input[1]=='\0') //backslash
	{
		EX::scan_view_of_memory(-1); return;
	}

	Ex_output_f11_error = 
	EX::command_view_of_memory(input);
}

static void Ex_output_print(const EX::Section *in, size_t offset)
{		
	wchar_t *OOB = offset>=in->info.RegionSize?L"B!":L"";

	if(*OOB) offset = 0; //eg. scroll lock+brackets navigation
		
	const wchar_t *name = EX::name_of_memory(in);
		
	const wchar_t *num = L"", *caps = L"", *scroll = L"";

	static unsigned onceperframe = 0; //hack

	if(onceperframe!=DDRAW::noFlips)
	{
		num = EX::numlock()?L"<num> ":L"";
		caps = GetKeyState(VK_CAPITAL)&1?L"<caps> ":L"";				
		scroll = GetKeyState(VK_SCROLL)&1?L"<scroll> ":L"";				

		if(*OOB&&*scroll) //out of bounds
		{
			scroll = L"<scroll-out-of-bounds>"; //long form
		}

		onceperframe = DDRAW::noFlips;
	}

	DWORD top = offset; 
		
	if(in->code) top = in->code->opcodes[offset];

	static char inputting[2] = "";

	if(!Ex_output_overlay_input
	||Ex_output_overlay_focus!=11
	||!EX::inquiring_regarding_input(false))
	{	
		EX_OUTPUT_PRINT_F(L"%s+%02x%s %s%s%s",name,top,OOB,num,caps,scroll)
	}
	else
	{
		wchar_t ncs[4] = L"ncs";

		if(*num) ncs[0] = 'N'; if(*caps) ncs[1] = 'C'; if(*scroll) ncs[2] = 'S'; 

		const wchar_t *r = EX::displaying_unicode_input(L"_",20);
				
		if(r==EX::returning_unicode)
		{
			const char *err = Ex_output_f11_error;

			if(!err) err = ""; assert(Ex_output_f11_error);

			const wchar_t *___ = *err?L"\u2026":L" "; 

			EX_OUTPUT_PRINT_F(L"%s+%02x%s[\u21b2%s%-18S]%s",name,top,OOB,___,err,ncs)		
		
			//cause return to blank for one frame
			{
				static unsigned returning = 0;

				if(returning<DDRAW::noFlips-1)
				{
					Ex_output_f11_blank = DDRAW::noFlips;
				}

				returning = DDRAW::noFlips;
			}
		}
		else
		{
			EX_OUTPUT_PRINT_F(L"%s+%02x%s[%-20s]%s",name,top,OOB,r,ncs)		

			Ex_output_f11_error = ""; 
		}
	}	
}

static void Ex_output_f11_memory_inspector()
{
	//from: Ex.memory.cpp
	//todo: better than this
	extern int Ex_memory_depth; 
	extern int Ex_memory_align;
	extern long long Ex_memory_pointers[8];
	extern const EX::Section *Ex_memory_sections[8];
	extern void *Ex_memory_entrypoints[8];

	Ex_output_margin = Ex_output_overlay_margin; 

	Ex_output_format = DT_LEFT|DT_TOP|DT_NOCLIP|DT_SINGLELINE;

	if(!Ex_memory_sections[0]) 
	{
		Ex_memory_sections[0] = SOM::text;
		Ex_memory_pointers[0] = 0;

		Ex_memory_depth = 0;
	}
	
//#ifdef _DEBUG

	static bool test = true;

	int round = 1; const void *round2target = 0;

	if(0&&test) //start at some memory
	{
		int n = 0; //13; //n = 11;

		const EX::Section *p = SOM::memory;

		while(p&&n--) p = p->next;
				
		//DWORD(*(void**)0x401afc)+0x20

		Ex_memory_pointers[0] = DWORD(*(void**)0x40389c)-0x400000;

		//assert(Ex_memory_pointers[0]<p->info.RegionSize);

		while(p->next&&Ex_memory_pointers[0]>p->info.RegionSize)
		{
			Ex_memory_pointers[0]-=
				DWORD(p->next->info.BaseAddress)-DWORD(p->info.BaseAddress);

			p = p->next; 			
		}

		round2target = (BYTE*)p->info.BaseAddress+Ex_memory_pointers[0];

		Ex_memory_sections[0] = p;

		round = 2; 
	}

	//test = false;

	if(1&&test) //scan for some value in memory
	{
		test = false;

		again: int skip = 0; 

		for(const EX::Section *p=SOM::memory;p;p=p?p->next:0)
		{
			if(p==SOM::text&&round==1) continue;

			const unsigned short *mem = 
				(const unsigned short*)p->info.BaseAddress;

			if(round==1)
			for(size_t i=0;p&&i<p->info.RegionSize/4;i++)
			{	
				//mem[i*2]==12345&&mem[i*2+1]==6789
				if(0x10FC0727==*(unsigned*)(mem+i*2)&&!skip--)
				{		  
					//round2target = mem+i*2; //for round 2 

					i*=4; Ex_memory_sections[0] = p;
					
					Ex_memory_pointers[0] = i-i%4;

					//round = 2; goto again;

					p = 0; //breakout
				}
			}

			const void **mem2 = 
				(const void **)(p?p->info.BaseAddress:0);

			if(round==2)
			for(size_t i=0;p&&i<p->info.RegionSize/4;i++)
			{				
				//if(p->number>3) break;

				if(unsigned(mem2[i])%4==0)
				if(mem2[i]>=Ex_memory_sections[0]->info.BaseAddress
				  &&mem2[i]<(VOID*)((BYTE*)
							Ex_memory_sections[0]->info.BaseAddress
							 +Ex_memory_sections[0]->info.RegionSize))
				{
					//if(p->number==1)					
					//EXLOG_LEVEL(0) << "MEMORY(" << std::hex << unsigned(mem2[i]) << "," << unsigned(mem2+i) << ")\n";

					if(unsigned(mem2[i])>=unsigned(round2target)-256
						&&unsigned(mem2[i])<=unsigned(round2target))
					EXLOG_LEVEL(0) << unsigned(mem2+i) << "/////CLOSE CALL///////////////\n";
					EXLOG_LEVEL(0) << p->number << ':' << std::hex << unsigned(mem2[i]) << '\n';

					if(mem2[i]==round2target)
					{
						i*=4; Ex_memory_sections[0] = p;
						
						Ex_memory_pointers[0] = i-i%4;

						p = 0; //breakout	 
					}
				}
			}			
		}
	}
	
	test = false;

//#endif

	int d = Ex_memory_depth;

	const EX::Section *p = Ex_memory_sections[0], *q = 0;

	if(p) Ex_output_print(p,Ex_memory_pointers[0]); else return;
	
	DWORD offset = Ex_memory_pointers[0];
	
	if(!p->code) offset-=offset%Ex_memory_align;

	if(offset>=p->info.RegionSize) offset = 0;
		
	wchar_t depth[] = L"\u2514        "; 

	int i;
	for(i=1;i<=d;i++)
	if(Ex_memory_sections[i])
	{
		depth[i] = '\0';

		p = Ex_memory_sections[i];

		offset = Ex_memory_pointers[i];

		if(!p->code) offset-=offset%Ex_memory_align;

		wchar_t *OOB = offset>=p->info.RegionSize?L"B!":L"";

		if(*OOB) offset = 0; //eg. scroll lock+brackets navigation

		wchar_t *out_of_bounds = L""; //long form

		if(*OOB&&i==d) out_of_bounds = L"out of bounds";

		void *ep = Ex_memory_entrypoints[i];
		
		const wchar_t *name = EX::name_of_memory(p);

		DWORD top = offset; 
		
		if(p->code) top = p->code->opcodes[offset];

		int diff = (long long)p->info.BaseAddress+top-(DWORD)ep;

		if(diff)
		{
			wchar_t *sign = diff<0?L"-":L"+"; 

			DWORD curr = DWORD(ep)-0x400000+diff; diff = abs(diff);

			EX_OUTPUT_PRINT_F(L"%s%s+%02x%s\u2500\u2500%08x%c%x=%x %s",depth,name,top,OOB,DWORD(ep)-0x400000,*sign,diff,curr,out_of_bounds)
		}
		else EX_OUTPUT_PRINT_F(L"%s%s+%02x%s\u2500\u2500%08x %s",depth,name,top,OOB,DWORD(ep)-0x400000,out_of_bounds)

		depth[i] = L'\u2514';

		depth[i-1] = ' ';
	}
	else break;
			
	if(i<=d||!Ex_memory_sections[d]) 
	{
		assert(0); return; //should not be so
	}
	
	//hack: counting every frame
	Ex_output_f11_linesperpage = 0; 

	DWORD x = 0, y = offset;

	for(int i=0;Ex_output_margin.top<Ex_output_margin.bottom;i++)
	{
		x = offset+i*Ex_memory_align;

		if(p->code)
		{
			x = offset+i; //per opcode

			if(x>=p->code->opcount) goto next;
		}
		else if(x>=p->info.RegionSize)
		{
next:		p = p->next; if(!p) break;

			Ex_output_print(p,x=offset=i=0);			
		}					
					
		const BYTE *mem = (const BYTE*)p->at(x); 

		DWORD pointer = DWORD(*(void**)mem);
			
		if(!p->code) //unimplemented
		if(offset%4==0) //hack (unaligned)
		if(pointer%4==0) //potential pointer
		{
			q = SOM::memory->by(*(void**)mem,&pointer);

			if(q) //if pointing to code (unlikely) must point to an opcode
			if(q->code&&q->code->opcodes[q->code->by(mem)]!=pointer) q = 0;
		}

		if(!q||Ex_output_f11_linesperpage==1)
		{
			if(p->code)
			{
				if(x>0)
				if((p->code->opcodes[x]&0xF00)
				!=(p->code->opcodes[x-1]&0xF00)) 		    
				{
					EX_OUTPUT_PRINT_F(L"+%x",p->code->opcodes[x])
				}

			}
			else if(x&&(x&0xF00)!=(y&0xF00)) 
			{
				EX_OUTPUT_PRINT_F(L"+%x",x)
			}
		}

		y = x;

		if(q)
		{
			wchar_t *doc = L"undocumented pointer candidate"; //documentation

			if(q->name)
			EX_OUTPUT_PRINT_F(L"%08x: %s+%02x %s",DWORD(*(void**)mem)-0x400000,q->name,pointer,doc)			
			else EX_OUTPUT_PRINT_F(L"%08x: %d+%02x %s",DWORD(*(void**)mem)-0x400000,q->number,pointer,doc)

			//pointer preview on hover
			if(!(GetKeyState(VK_CAPITAL)&1))
			if(Ex_output_f11_linesperpage==0) //hack
			{
				p = q; offset = pointer; i = 0; 

				if(!p->code)
				{					
					 mem = (const BYTE*)p->at(x=y=offset);
				}
				else offset = p->code->by(mem);
			}

			q = 0;
		}

		unsigned char txt[8]; 
		
		if(!p->code)
		{
			memcpy(txt,mem,Ex_memory_align);		 

			for(int j=0;j<Ex_memory_align;j++) 
			{
				if(txt[j]<32||txt[j]>126) txt[j] = '`';
			}
		}

		int paranoia = Ex_output_margin.top;

		if(p->code)	//print disassembly
		{
			const char *s = 0;

			if(GetKeyState(VK_CAPITAL)&1)
			{
				const int bin_s = 32;
				static char bin[bin_s+1] = ""; 

				int stop = min(bin_s,2*p->code->opchars[x]);

				unsigned char *mem = (unsigned char*)p->code->at(x);

				for(int i=0;i<stop;i++)	sprintf(bin+i*2,"%02x",mem[i]);
				
				s = bin; bin[stop] = '\0';
			}
			else s = p->code->x(x);
			
			EX_OUTPUT_PRINT_F(L"%02x:  %S",p->code->opcodes[x]&0xFF,s)
		}
		else if(Ex_memory_align==8) //assuming double mode
		{
			if(EX::numlock())
			{
				//+4 is not necessary: doubles must be aligned
				double d = *(double*)mem, u = *(double*)(mem+4); 

				EX_OUTPUT_PRINT_F(
				L"%02x:  %02x%02x %02x%02x %02x%02x %02x%02x  %1c%1c%1c%1c%1c%1c%1c%1c  %20.12g %20.12g",
				x&0xFF,
				mem[0],mem[1],mem[2],mem[3],mem[4],mem[5],mem[6],mem[7],
				txt[0],txt[1],txt[2],txt[3],txt[4],txt[5],txt[6],txt[7],
				d,u)
			}
			else //64bit integer mode
			{
				__int64 d = *(__int64*)mem, u = *(__int64*)(mem+4); 

				EX_OUTPUT_PRINT_F(
				L"%02x:  %02x%02x %02x%02x %02x%02x %02x%02x  %1c%1c%1c%1c%1c%1c%1c%1c  %20I64d %20I64d",
				x&0xFF,
				mem[0],mem[1],mem[2],mem[3],mem[4],mem[5],mem[6],mem[7],
				txt[0],txt[1],txt[2],txt[3],txt[4],txt[5],txt[6],txt[7],
				d,u)
			}
		}
		else if(EX::numlock())
		{				
			const unsigned short *s = (const unsigned short*)mem;	
			
			int n = *(const int*)mem; float f = *(const float*)mem;	 

			EX_OUTPUT_PRINT_F(
			L"%02x:  %02x%02x %02x%02x  %1c%1c%1c%1c  %5d %-5d  %11d  %13.6g",
			x&0xFF,
			mem[0],mem[1],mem[2],mem[3],
			txt[0],txt[1],txt[2],txt[3],
			s[0],s[1],n,f)
		}
		else
		{
			//shadowing
			char s8s[4][34] = {"    ","    ","    ","    "};
			{
				for(int i=0;i<4;i++) if((__int8)mem[i]<0)
				{
					for(int j=0;j<4;j++)
					{
						if((__int8)mem[j]>=0)
						{
							itoa((__int8)mem[j],s8s[j]+1,10);
						}
						else itoa((__int8)mem[j],s8s[j],10);

						int k;
						for(k=0;k<5;k++) if(s8s[j][k]=='\0')
						{							
							s8s[j][k] = ' ';  //left align
						}
						s8s[j][k] = '\0'; //itoa is greedy
					} 
					break;
				}
			}

			EX_OUTPUT_PRINT_F(
			L"%02x:  %02x%02x %02x%02x  %1c%1c%1c%1c  %3d %3d %3d %3d  %S %S %S %S",
			x&0xFF,
			mem[0],mem[1],mem[2],mem[3],
			txt[0],txt[1],txt[2],txt[3],
			mem[0],mem[1],mem[2],mem[3],
			s8s[0],s8s[1],s8s[2],s8s[3])
		}

		if(Ex_output_margin.top<=Ex_output_margin.bottom)
			Ex_output_f11_linesperpage++;

		if(Ex_output_margin.top==paranoia)
		{	
			EX_OUTPUT_PRINT(L"ERROR: Overlay line failed to print! Skipping")   

			if(Ex_output_margin.top==paranoia) break; //todo: log once
		}
	}

	if(Ex_output_overlay_input
	  &&Ex_output_overlay_focus==11
	   &&EX::inquiring_regarding_input())
	{
		if(Ex_output_f11_blank!=DDRAW::noFlips)
		{
			Ex_output_have_f11_input(); 
		}
	}
}

extern void EX::simulcasting_output_overlay_dik(unsigned char dik, unsigned char unx)
{		
	if(dik==0x58) //F12 (master key)
	{
		if(EX::alt()) return; //hack

		if(!EX::output_overlay_f[12]) goto up;           
	}
	else if(dik>=0x3B&&dik<=0x44||dik==0x57) //F1~F11
	{			
		//HACK: exempting F4 from alt test assuming
		//alt+F4 quits the program under Windows so
		//that F4 reliably enters SOM's flying mode
		//in an emergency (falling to certain doom)

		if(EX::alt()&&dik!=0x3E) return; //hack
		if(dik>=0x42&&dik<=0x44) return; //F8~F10

		EX::INI::Output tt;

		int n = dik==0x57?11:dik-0x3B+1;
		
		if(tt->function_overlay_tint[n]&0xFF000000)
		{	
			int cancel = Ex_output_overlay_focus; //hack

			if(n==3||n==4) //god/fly modes			 
			{
				Ex_output_overlay_focus = 0;

				EX::output_overlay_f[n] = !EX::output_overlay_f[n];
			}			
			else if(EX::output_overlay_f[n])					
			{
				//12: bring up overlays (n on top) if hidden
				if(Ex_output_overlay_focus==n&&EX::output_overlay_f[12]) 
				{
					Ex_output_overlay_focus = 0; EX::output_overlay_f[n] = false;
				}
				else Ex_output_overlay_focus = n;				
			}
			else
			{
				EX::output_overlay_f[n] = true;

				Ex_output_overlay_focus = n;				
			}

			if(Ex_output_overlay_focus==0&&cancel)
			{
				EX::cancelling_requested_input();
			}

			Ex_output_overlay(n);
			
			goto up;
		}
	}	

	if(!EX::output_overlay_f[12]) return;

	switch(dik)
	{
	case 0x58: //DIK_F12            
		
		if(Ex_output_overlay_focus)
		{
			//emergency/last ditch cancel

			EX::cancelling_requested_input(); 
			
			Ex_output_overlay_focus = 0;
		}

		EX::output_overlay_f[12] = false; return;

	case 0x0C: //DIK_MINUS            

		if(Ex_output_overlay_focus==1)
		Ex_output_move_counter_pointer(-1); 
		if(Ex_output_overlay_focus==2)
		Ex_output_move_event_pointer(-1); 		
		if(Ex_output_overlay_focus==11)
		EX::move_view_of_memory(-Ex_output_f11_linesperpage); 		

		return;

	case 0x0D: //DIK_EQUALS (plus key)            

		if(Ex_output_overlay_focus==1)
		Ex_output_move_counter_pointer(+1); 
		if(Ex_output_overlay_focus==2)
		Ex_output_move_event_pointer(+1); 
		if(Ex_output_overlay_focus==11)
		EX::move_view_of_memory(Ex_output_f11_linesperpage); 
		
		return;
		
	case 0x1A: //LBRACKET            

		if(Ex_output_overlay_focus==11)
		EX::next_view_of_memory(-1); return;		

	case 0x1B: //RBRACKET           
									 
		if(Ex_output_overlay_focus==11)
		EX::next_view_of_memory(+1);  return;

	case 0x2B: //BACKSLASH

		if(!Ex_output_overlay_input)
		{
			Ex_output_overlay_input = 1;
		}

		if(Ex_output_overlay_focus==1
		 ||Ex_output_overlay_focus==2)
		{
			EX::requesting_ascii_input('pos',unx);
		}
		else if(Ex_output_overlay_focus==11)
		{				
			EX::requesting_ascii_input(unx);
		}

		return;
	}

	switch(unx)
	{
	case 0x0F: case 0x2B: //TAB/BACKSLASH
	case 0x2A: case 0x36: //LSHIFT/RSHIFT

	case 0x29: //GRAVE for Sticky Keys needs

		bool shift = unx!=0x0F&&unx!=0x2B;
		if(!shift&&GetKeyState(VK_SHIFT)>>15)
		{
			break; //courtesy: ignore Shift+Tab
		}

		if(Ex_output_overlay_focus)
		if(EX::inquiring_regarding_input(false))
		{
			if(Ex_output_overlay_focus==1
			 ||Ex_output_overlay_focus==2)
			{			
				if(shift) //NEW
				{						
					if(Ex_output_overlay_input==1) //hack
					{
						if(1==Ex_output_overlay_focus)
						Ex_output_overlay_input = Ex_output_counter_counter;
						if(2==Ex_output_overlay_focus)
						Ex_output_overlay_input = Ex_output_event_counter;
					}
					else Ex_output_overlay_input--;
				}
				else Ex_output_overlay_input++;
			}
			else if(Ex_output_overlay_focus==11)
			{
				if(!shift) 
				Ex_output_overlay_input = !Ex_output_overlay_input;
			}

			if(!Ex_output_overlay_input)
			{
				EX::suspending_requested_input(unx);
			}
		}
		break;
	}
	return;
up:	EX::output_overlay_f[12] = true;
}

static bool (*Ex_output_onflip_passthru)() = 0;
static void (*Ex_output_onreset_passthru)() = 0;
static void (*Ex_output_oneffects_passthru)() = 0;

static void Ex_output_onreset()
{
	Ex_output_onreset_passthru();

	if(Ex_output_line) Ex_output_line->Release();
	Ex_output_line = 0;

	if(Ex_output_sprite==Ex_output_spritefailedtoload) 
	Ex_output_sprite = 0;
	if(Ex_output_sprite) Ex_output_sprite->Release();

	if(Ex_output_overlay_font==Ex_output_fontfailedtoload) 
	Ex_output_overlay_font = 0;	 
	if(Ex_output_overlay_font) Ex_output_overlay_font->Release();

	Ex_output_font = Ex_output_overlay_font = 0;

	Ex_output_sprite = 0;

	if(Exselector)
	{
		Ex_output_gl = 0; Exselector->svg_reset(); //2021	
	}

	for(int i=0;i<EX_OUTPUT_FONTS;i++) if(Ex_output_fonts[i]) 
	{
		/*2021: releasing_output_font is identical (REMOVE ME)
		for(int j=0;Ex_output_fonts[i]->glyphs[j];j++)
		{
			if(*Ex_output_fonts[i]->glyphs[j].d3dxfont)
			if(*Ex_output_fonts[i]->glyphs[j].d3dxfont!=Ex_output_fontfailedtoload)
			{
				(*Ex_output_fonts[i]->glyphs[j].d3dxfont)->Release();
			}
			*Ex_output_fonts[i]->glyphs[j].d3dxfont = 0;

			if(*Ex_output_fonts[i]->glyphs[j].d3dxsprite)
			if(*Ex_output_fonts[i]->glyphs[j].d3dxsprite!=Ex_output_spritefailedtoload)
			{
				(*Ex_output_fonts[i]->glyphs[j].d3dxsprite)->Release();
			}
			*Ex_output_fonts[i]->glyphs[j].d3dxsprite = 0;
		}*/
		if(!EX::releasing_output_font(Ex_output_fonts+i,true)) //2021
		{
			assert(0); //just being sure this works
		}
	}
}

extern void (*EX::speedometer)(float speeds[3]) = 0;

static bool Ex_output_onflip_or_oneffect(bool oneffects)
{
	bool out = true;

	EX::INI::Option op;	EX::INI::Detail dt;
	
	//REMOVE ME
	//if(SOM::L.f3) //god mode
	{
		//2018: switching to SOM::f3 for a more
		//dynamic experience... maybe do_f3?
		//*SOM::L.f3 = EX::output_overlay_f[3]?1:0; 
		assert(0==SOM::L.f3);
	}
	//if(SOM::L.f4) //fly mode (reversed)
	{
		BYTE cmp = SOM::L.f4; //2020
		SOM::L.f4 = BYTE(EX::output_overlay_f[4]?0:1); 
		if(SOM::L.f4!=cmp)
		for(int i=0;i<EX_AFFECTS;i++) 
		EX::Affects[i].clear();
	}

	if(op->do_log)
	{	
		static bool itimeout = false;
				
		if(dt->log_initial_timeout&&!itimeout)
		{
			if(DDRAW::noFlips>=dt->log_initial_timeout)
			{
				*EXLOG::master = 0; 
				
				itimeout = true;
			}
		}
		else if(dt->log_subsequent_timeout)
		{
			static unsigned stimeout = 0;

			if(!itimeout)
			{
				if(*EXLOG::master<=0) itimeout = true;
			}
			else if(*EXLOG::master>0)
			{
				if(stimeout>=dt->log_subsequent_timeout)
				{
					*EXLOG::master = 0; //timed out
				}
				else stimeout++;
			}

			if(*EXLOG::master<=0) stimeout = 0;				
		}
	}

	if(oneffects) //2022
	{
		Ex_output_oneffects_passthru();
	}
	else out = Ex_output_onflip_passthru();

	//frames per second
	{
		DWORD now = EX::tick();

		static DWORD ref = EX::tick();

		if(now-ref>0) //extreme paranoia
		{
			EX::fps = 1000.0f/(now-ref); 
		}
		
		ref = now;		
	}

	static float tic = EX::tick(); 
	static float sum = 0, avg = 0;
	static float ksum = 0, ksum2 = 0, e = 0;
										 
	if(EX::tick()-tic>=250)
	{
		tic = EX::tick();

		EX::clip = avg?max(0,int(sum/avg+0.5f)/5*5):0;  		

		EX::kmph = avg?ksum/avg:0; ksum = 0.0f;
		EX::kmph2 = avg?ksum2/avg:0; ksum2 = 0.0f;

		EX::e = avg?e/avg:0; e = 0.0f; //rror

		sum = avg = 0.0f;
	}
	else								   
	{
		//REMOVE ME?
		//formerly SOM::speed, SOM::speed2, SOM::error
		float speeds[3] = {0,0,0};
		if(EX::speedometer)	EX::speedometer(speeds);

		ksum+=speeds[0]*60*60/1000;
		ksum2+=speeds[1]*60*60/1000; e+=speeds[2]; //error
		sum+=EX::fps; avg++;		
	}	

		if(DDRAW::xr) return out; //NOT DRAWING FOR NOW

	int f = 1; while(f<=12)	
	if(EX::output_overlay_f[f++]) break; 	
	if(f>12) return out;
	
	if(!DDRAW::Direct3DDevice7) return out;
	if(!EX::output_overlay_f[12]) return out;
	
	if(Ex_output_overlay_font==Ex_output_fontfailedtoload) return out;

	const bool gl = DDRAW::gl; if(!gl)
	{
		if(!Ex_output_overlay_font)
		{	
			//2021: not sure why 'dx7a' gets here?
			if(DDRAW::target!='dx9c')
			{
				assert('dx7a'==DDRAW::target);

				Ex_output_overlay_font = Ex_output_fontfailedtoload; return out;
			}

			HRESULT hr = D3DXCreateFont
			(
				//NOTE: 0 is selecting "Courier" on my system, but this may not be guaranteed
				//but it probably is since the first alphabetically is Arial
				//NOTE: the docs make ANSI_CHARSET sound like ASCII and DEFAULT_CHARSET sound
				//like ANSI. I wish I could specify ASCII here
				DDRAW::Direct3DDevice7->proxy9,				
				Ex_output_16,0,FW_DONTCARE,1,0,ANSI_CHARSET,OUT_RASTER_PRECIS,NONANTIALIASED_QUALITY,FIXED_PITCH,				
				//TODO: hardwire L"Courier"? 
				//TODO: add Courier support to Exselector? (it's using "Verdana")
				//NOTE: Courier is a FON file that's really an EXE like resource
				//file with 3 FNT resources that can also function as standalone
				//fonts in Windows' fonts folder
				0, //L"Courier"?
				&Ex_output_overlay_font
			);

			if(hr!=S_OK)
			{
				Ex_output_overlay_font = Ex_output_fontfailedtoload; return out;
			}

			Ex_output_overlay_font->PreloadCharacters(0,255);
		}

		if(!Ex_output_sprite)
		{
			HRESULT hr = D3DXCreateSprite(DDRAW::Direct3DDevice7->proxy9,&Ex_output_sprite);

			if(hr!=S_OK) Ex_output_sprite = Ex_output_spritefailedtoload;		
		}
	}
	else if(!Ex_output_gl||!Ex_output_overlay_font) //OpenGL?
	{
		//REMINDER: can't use CreateFont with "" face name because
		//GetFont reports "" for its LOGFONT face name, which can't
		//be used with GetFontData for some reason. "Courier" comes
		//from a test, until now I didn't know what font "" had got				
		//Verdana is better/closer to Courier than
		//"Couier New"
		//L"Courier New"; //Courier isn't TrueType				
		//Verdana is sans-serif
		enum{ plus=-1 }; //-2
		#define _ L"Courier New Bold" //L"Verdana"
		LOGFONTW lf = {
		Ex_output_16+plus,0,0,0,FW_DONTCARE,0,0,0,ANSI_CHARSET,OUT_RASTER_PRECIS, 
		//CLIP_DEFAULT_PRECIS, //testing, L"Courier"?
		OUT_TT_ONLY_PRECIS, //nanovg reads TTF format
		NONANTIALIASED_QUALITY,FIXED_PITCH,_};
		#undef _

		if(!Ex_output_hdc)
		{
			HFONT h; HDC dc = CreateCompatibleDC(0); 

			//NOTE: could use this font instead of "Courier" but there's
			//no reason to do this if passing the font's face name, and
			//I don't know if it should be left up to chance				
			//EnumFontFamiliesEx(dc,&lf,Ex_output_enumFFE,(LPARAM)lf.lfFaceName,0);
			h = CreateFontIndirect(&lf);
			//MS Gothic really doesn't work well here
		//	if(!h) wmemcpy(lf.lfFaceName,L"MS Gothic",10); //in case no Courier?!
		//	if(!h) h = CreateFontIndirect(&lf);
			if(!h) //ultimate fallback? Arial?
			{
				lf.lfFaceName[0] = '\0';					
				EnumFontFamiliesEx(dc,&lf,Ex_output_enumFFE,(LPARAM)lf.lfFaceName,0);
				h = CreateFontIndirect(&lf);
			}

			//DeleteDC(dc);
			//saving these for DT_CALCRECT
			Ex_output_hdc = dc; SelectObject(dc,h);
		}
		if(!Ex_output_gl)
		{
			HFONT h = (HFONT)GetCurrentObject(Ex_output_hdc,OBJ_FONT);
			//if(Ex_output_16!=Exselector->svg_font(h)) 
			//assert(0);
		
			Ex_output_gl = Exselector->svg_font(h);
			//DeleteFont(h);
		}
		if(!Ex_output_overlay_font) //DT_CALCRECT?
		{	
			if(Ex_output_overlay_font) //PARANOID
			if(Ex_output_overlay_font!=Ex_output_fontfailedtoload)
			{
				Ex_output_overlay_font->Release(); assert(0);
				Ex_output_overlay_font = 0; 
			}

			HRESULT hr = D3DXCreateFont(DDRAW::Direct3DDevice7->proxy9,				
			lf.lfHeight,0,FW_DONTCARE,1,0,ANSI_CHARSET,OUT_RASTER_PRECIS,NONANTIALIASED_QUALITY,FIXED_PITCH,
			lf.lfFaceName,&Ex_output_overlay_font);

			if(hr!=S_OK)
			{
				Ex_output_overlay_font = Ex_output_fontfailedtoload; return out;
			}

		//	Ex_output_overlay_font->PreloadCharacters(0,255); //DT_CALCRECT only
		}
	}
	
	Ex_output_font = 0; Ex_output_cursor = 0; //cursor

	if(Ex_output_overlay_font!=Ex_output_fontfailedtoload)
	{
		Ex_output_font = Ex_output_overlay_font;
	}

	if(!Ex_output_font) return out;

	DDRAW::doNothing = true; //// POINT OF NO RETURN ////

	//TODO: should Flip do this in advance?
	RECT client = {0,0,640,480};
	DDRAW::window_rect(&client);
	if(oneffects) DDRAW::doSuperSamplingMul(client); //2022
	DX::D3DVIEWPORT7 vp;
	DDRAW::Direct3DDevice7->GetViewport(&vp);
	DX::D3DVIEWPORT7 fullscreen = {0,0,client.right,client.bottom,0.0f,1.0f};
	DDRAW::Direct3DDevice7->SetViewport(&fullscreen);

	DDRAW::Direct3DDevice7->BeginScene(); //onflip

	if(!gl)
	{
		if(Ex_output_sprite!=Ex_output_spritefailedtoload)
		{
			Ex_output_cursor = Ex_output_sprite;		
		}

		//REMINDER: without this sprites are just squares
		DWORD st = D3DXSPRITE_ALPHABLEND; 

		//sprites sometimes go bad??
		if(Ex_output_cursor&&Ex_output_cursor->Begin(st)!=D3D_OK)
		{
			Ex_output_cursor->Release(); Ex_output_cursor = 0; 
		}
	}
	else
	{
		int w = client.right;
		int h = client.bottom;
		float m[4*4] = 
		{
			2.0f/w,0,0,0,
			0,-2.0f/h,0,0,
			0,0,0,0,
			-1,1,0,1,
		};
		if(oneffects) m[5] = -m[5]; 
		if(oneffects) m[13] = -m[13];

		Exselector->svg_view(m);

		Exselector->svg_begin();

		//HACK: OpenGL ES HAS ZERO STATE MANAGEMENT FACILITIES
		DDRAW::Direct3DDevice7->queryGL->state.apply_nanovg();
	}

	Ex_output_margin = Ex_output_default_margin;

	GetClientRect(EX::client,&Ex_output_margin);

	Ex_output_format = DT_RIGHT|DT_TOP|DT_NOCLIP|DT_SINGLELINE;

	EX::INI::Output tt;
	Ex_output_color = tt->function_overlay_tint[12];
	Ex_output_shadow = tt->function_overlay_contrast[12];
		
	WORD fmask = 0x00000; 

	for(int i=0;i<12;i++) if(EX::output_overlay_f[12-i]) fmask|=1<<i;

	//if(1) //new way (monospace)
	{			
		Ex_output_margin.right+=8;

		RECT reset = Ex_output_margin; //hack

		static int F1 = 0, F11 = 0; if(!F1||!F11)
		{
			Ex_output_format|=DT_CALCRECT;

			EX_OUTPUT_PRINT(L"F1");

			F1 = Ex_output_calculation.right-Ex_output_calculation.left;

			EX_OUTPUT_PRINT(L"F11");

			F11 = Ex_output_calculation.right-Ex_output_calculation.left;

			Ex_output_format&=~DT_CALCRECT;
		}

		if(tt->function_overlay_tint[12]&0xFF000000)
		{
			EX_OUTPUT_PRINT_F(L"%04x |",fmask)

			Ex_output_margin.top = reset.top;
		}

		Ex_output_margin.right-=F11*2+(gl?F1:0); //0000 |

		for(int i=12;i>0;i--)
		if(tt->function_overlay_tint[f]&0xFF000000)
		{
			Ex_output_margin.right-=F11-F1; //space

			if(EX::output_overlay_f[i])
			{
				Ex_output_color = tt->function_overlay_tint[i];
				Ex_output_shadow = tt->function_overlay_contrast[i];

				Ex_output_underline = Ex_output_overlay_focus==i;

				EX_OUTPUT_PRINT_F(L"F%d",i)

				Ex_output_underline = false;
			}

			Ex_output_margin.right-=i>9?F11:F1;

			Ex_output_margin.top = reset.top;
		}

		Ex_output_margin = reset;

		EX_OUTPUT_PRINT(L" ");
	}
	/*2021
	else if(tt->function_overlay_tint[12]&0xFF000000) //hack
	{
	//Todo: colors/current focus (GetTextExtentPoint32)
	//Note: ID3DXFont won't print trailing spaces??
	Ex_output_margin.right+=8; //hide pipe in this line offscreen
	EX_OUTPUT_PRINT_F(L"%s %s %s %s %s %s %s %s %s %s %s %s %04x |",
	EX::output_overlay_f[1]?L"F1":L"  ",
	EX::output_overlay_f[2]?L"F2":L"  ",
	EX::output_overlay_f[3]?L"F3":L"  ",
	EX::output_overlay_f[4]?L"F4":L"  ",
	EX::output_overlay_f[5]?L"F5":L"  ",
	EX::output_overlay_f[6]?L"F6":L"  ",
	EX::output_overlay_f[7]?L"F7":L"  ",
	EX::output_overlay_f[8]?L"F8":L"  ",
	EX::output_overlay_f[9]?L"F9":L"  ",
	EX::output_overlay_f[10]?L"F10":L"   ",
	EX::output_overlay_f[11]?L"F11":L"   ",
	EX::output_overlay_f[12]?L"F12":L"   ",fmask)	
	}
	else Ex_output_margin.right+=8;*/

	Ex_output_margin.left+=8;

	Ex_output_overlay_margin = Ex_output_margin;
		
	WORD emask = 0x00000; //eclipse
	
	int zorder[12] = {0,0,0,0,0,0,0,0,0,0,0,0};

	for(int i=0;i<12;i++)	
	{			
		int f = Ex_output_overlay_order[i]; assert(f);

		if(EX::output_overlay_f[f])
		if(tt->function_overlay_tint[f]&0xFF000000) 
		{
			emask|=1<<12-f;

			if(!(Ex_output_overlay_matrix[f]&emask))			
			{
				zorder[11-i] = f;
			}
		}
	}

	//draw back to front
	for(int i=0;i<12;i++) if(zorder[i])
	{
		int f = zorder[i];

		Ex_output_color = tt->function_overlay_tint[f];
		Ex_output_shadow = tt->function_overlay_contrast[f];

		//todo: user handler array
		if(EX::output_overlay_f[f]) switch(f)
		{
		case  1: Ex_output_f1_counter_battery();     break;
		case  2: Ex_output_f2_event_battery();		 break;
		case  5: Ex_output_f5_system_information();  break;
		case  6: Ex_output_f6_player_information();  break;
		case  7: Ex_output_f7_enemy_information();	 break;
		case 10: /*traditionally F10 is like Alt*/   break;
		case 11: Ex_output_f11_memory_inspector();	 break;
		//case 12: Ex_output_f12_fullscreen_console(); break; //???
		}
	}
	
	if(!gl)
	{
		if(Ex_output_cursor) Ex_output_cursor->End();
	}
	else Exselector->svg_end(Ex_output_Exselector_xr_callback);
	
	DDRAW::Direct3DDevice7->EndScene(); //onflip
	
	//TODO: should Flip do this in advance?
	DDRAW::Direct3DDevice7->SetViewport(&vp);
	DDRAW::doNothing = false;
	
	return out;
}
static bool Ex_output_onflip()
{
	if(DDRAW::WGL_NV_DX_interop2)
	return Ex_output_onflip_passthru();
	return Ex_output_onflip_or_oneffect(false);
}
static void Ex_output_oneffects()
{
	if(DDRAW::WGL_NV_DX_interop2)
	Ex_output_onflip_or_oneffect(true);
	else Ex_output_oneffects_passthru();
}

extern void EX::initialize_output_overlay()
{
	static int one_off = 0;

	if(!one_off) one_off = 1; else return;

	if(!SOM::game) return; //2021

	//2021: I think this is the only thing that
	//releases the fonts. it hadn't been set up
	//if EX::INI::Output was empty
	Ex_output_onreset_passthru = DDRAW::onReset;
	DDRAW::onReset = Ex_output_onreset;	
	
	if(0&&EX::debug) //TESTING
	if(HMODULE gm=LoadLibraryA("Gamemode.dll"))	
	(void*&)HasExpandedResources = GetProcAddress(gm,"HasExpandedResources");

	EX::INI::Output tt; if(!tt) return;

	Ex_output_onflip_passthru = DDRAW::onFlip;
	DDRAW::onFlip = Ex_output_onflip;
	//2022: WGL_NV_DX_interop2?
	Ex_output_oneffects_passthru = DDRAW::onEffects;
	DDRAW::onEffects = Ex_output_oneffects;

	/*2021: release the titles fonts?
	Ex_output_onreset_passthru = DDRAW::onReset;
	DDRAW::onReset = Ex_output_onreset;*/

	for(int i=0;i<12;i++)
	{
		EX::output_overlay_f[12-i] = tt->function_overlay_mask&1<<i;

		//build inverse of eclipse matrix
		if(tt->function_overlay_eclipse[12-i])
		{
			for(int j=0;j<12;j++) if(i!=j)
			if(tt->function_overlay_eclipse[12-i]&1<<j)
			{
				Ex_output_overlay_matrix[12-j]|=1<<i;
			}
		}
	}
}