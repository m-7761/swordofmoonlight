
#include "directx.h" 
DX_TRANSLATION_UNIT

#define DIRECT3D_VERSION 0x0700

#include "dx8.1/ddraw.h"
#include "dx8.1/d3d.h" 

#define DDRAW_OVERLOADS

#include "dx.ddraw.h"

//#define CALLBACK __stdcall

namespace DDRAW //experimental
{
	//HACK: SOM goes nuts without these
	DWORD force_color_depths = DDBD_32|DDBD_16;
}

//2021: this is a (semi-working) test to improve tool
//graphics on DPI scaled systems by enlarging the D3D
//backbuffer. by default it's scaled with 2x aliasing
//I've only implemented it on DX6, DX7 (workshop.cpp)
//needs it to work as well. 
//NOTE: line drawing looks uneven, at places the line
//is 2x and others 1px. I don't get it. on actual DPI
//systems the lines may be too thin to see
enum{ dx_DPI_testing_2021=0&&DX::debug };

extern HMODULE DDRAW::dll = 0;

extern bool DDRAW::doD3D12 = false;
extern bool DDRAW::doD3D9on12 = false;
extern ID3D12Device *DDRAW::D3D12Device = 0;
//FAILED EXPERIMENT
//extern DDRAW::doD3D12_backbuffer DDRAW::doD3D12_backbuffers[3] = {};

extern int 
DDRAW::target = 'dx7a',
DDRAW::target_backbuffer = 0,
DDRAW::compat = 0, //2021
DDRAW::compat_moving_target = 0, //2022
DDRAW::compat_glUniformBlockBinding_1st_of_6 = 0,
DDRAW::shader = 0, //client mode
DDRAW::sRGB = 0;
extern bool DDRAW::ff = true;
extern bool DDRAW::gl = false;
extern BOOL DDRAW::xr = false;

extern unsigned DDRAW::textures = 1;
extern unsigned DDRAW::dejagrate = 0; 
extern unsigned DDRAW::dejagrate_divisor_for_OpenXR = 3100;

extern bool DDRAW::inStereo = 0;
extern float DDRAW::stereo = 0.0001f; //functionally mono
extern float DDRAW::stereo2 = 0; //perspective matrix asymmetry

extern bool DDRAW::fullscreen = false;

void(*DDRAW::IDirectDrawSurface::Query::clientdtor)(void*&) = 0;

extern DDRAW::IDirectDraw7 *DDRAW::DirectDraw7 = 0;

extern DDRAW::IDirectDrawSurface7
*DDRAW::PrimarySurface7 = 0,
*DDRAW::BackBufferSurface7 = 0,
*DDRAW::DepthStencilSurface7 = 0,
*DDRAW::TextureSurface7[8] = {};

extern DDRAW::IDirect3DDevice7 *DDRAW::Direct3DDevice7 = 0;

extern HMONITOR DDRAW::monitor = 0;

extern HWND DDRAW::window = 0;
extern BOOL DDRAW::window_rect(RECT *client, HWND window)
{
	if(!window) return 0;

	//2021: WS_BORDER in SOM_MAP is causing the
	//viewport to not be centered and smaller	
	if(WS_CHILD&GetWindowLong(window,GWL_STYLE))
	{
		//this is actually larger than GetWindowRect
		return GetClientRect(window,client);
	}

	//I think Windows 10's invisible borders are throwing 
	//this off.	
	//return GetClientRect(DDRAW::window,client);
	if(GetWindowRect(/*DDRAW::*/window,client)) 
	client->right-=client->left; else return 0;
	client->bottom-=client->top; 
	client->left = client->top = 0; return 1;
}

extern DX::DDGAMMARAMP DDRAW::gammaramp = {};

extern bool
DDRAW::midFlip = false,
DDRAW::doFlipDiscard = false,
DDRAW::doFlipEx = false,
DDRAW::doNvidiaOpenGL = false,
DDRAW::WGL_NV_DX_interop2 = false,
DDRAW::isStretched = true,
DDRAW::isInterlaced = false,
DDRAW::isSynchronized = true,
DDRAW::doMultiThreaded = false,
DDRAW::doSoftware = false,
DDRAW::doMSAA = false,
DDRAW::doWait = false,
DDRAW::do565 = false,
DDRAW::doFog = true,
DDRAW::doWhite = false,
DDRAW::doEffects = true,
DDRAW::doGamma = false,
DDRAW::doDither = false,
DDRAW::doMipmaps = false,
DDRAW::doSmooth = false, DDRAW::doSharp = false,
DDRAW::doInvert = false,
DDRAW::doTripleBuffering = false,
DDRAW::doSuperSampling = false,
DDRAW::do2ndSceneBuffer = false; //...
extern unsigned
DDRAW::doMipSceneBuffers = false; //EXPERIMENTAL
extern float
DDRAW::xrSuperSampling = 0.0f;
						 
extern bool
DDRAW::doScaling = true,
DDRAW::doMapping = true,
DDRAW::doNothing = false;
extern int
DDRAW::xyMapping2[2] = {}; //2021
extern float
DDRAW::xyMapping[2] = {0,0},
DDRAW::xyScaling[2] = {1,1};
extern DWORD
DDRAW::xyMinimum[2] = {1,1};

//EXPERIMENTAL
void DDRAW::xyRemap2(RECT &box, bool offset)
{
	if(0&&DX::debug) offset = 1; //DUPLICATE

	float os[2] = {}; 
	if(offset) os[0] = DDRAW::xyMapping[0];
	if(offset) os[1] = DDRAW::xyMapping[1];

	float t = os[1]+DDRAW::xyScaling[1]*box.top;
	float l = os[0]+DDRAW::xyScaling[0]*box.left;
	float r = os[0]+DDRAW::xyScaling[0]*box.right;
	float b = os[1]+DDRAW::xyScaling[1]*box.bottom;

	DDRAW::doSuperSamplingMul(t,l,r,b);
	
	box.top = (int)(t+0.5f);
	box.left = (int)(l+0.5f);
	box.right = (int)(r+0.5f);
	box.bottom = (int)(b+0.5f);
}
void DDRAW::xyUnmap2(RECT &box, bool offset)
{
	if(0&&DX::debug) offset = 1; //DUPLICATE

	float os[2] = {};
	if(offset) os[0] = DDRAW::xyMapping[0];
	if(offset) os[1] = DDRAW::xyMapping[1];

	float t = (box.top-os[1])/DDRAW::xyScaling[1];
	float l = (box.left-os[0])/DDRAW::xyScaling[0];
	float r = (box.right-os[0])/DDRAW::xyScaling[0];
	float b = (box.bottom-os[1])/DDRAW::xyScaling[1];

	DDRAW::doSuperSamplingDiv(t,l,r,b);
	
	box.top = (int)(t+0.5f);
	box.left = (int)(l+0.5f);
	box.right = (int)(r+0.5f);
	box.bottom = (int)(b+0.5f);
}

extern DWORD
DDRAW::aspectratio = 0,
DDRAW::bitsperpixel = 32,
DDRAW::resolution[2] = {0,0},
DDRAW::refreshrate = 0;

//REMOVE ME? hacks
extern bool DDRAW::linear = false;
extern int DDRAW::anisotropy = 0;

extern int DDRAW::bright = 0, DDRAW::dimmer = 0; 
extern unsigned char DDRAW::brights[9+8] = {0,0,0,0,0,0,0,0,0};
extern unsigned char DDRAW::dimmers[9+8] = {0,0,0,0,0,0,0,0,0};
extern DWORD DDRAW::lighting[16+1] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
extern int DDRAW::lights = 0, DDRAW::maxlights = 16, DDRAW::minlights = 0;

extern bool DDRAW::ofApplyStateBlock = false,
DDRAW::isLit = false, DDRAW::inScene = false, DDRAW::inFog = false;

extern unsigned DDRAW::fps = 0,
DDRAW::noFlips = 0, DDRAW::noTicks = 0, DDRAW::flipTicks = 0;
static bool dx_ddraw_onflip()
{
	DDRAW::noFlips++;

	DWORD now = DX::tick();
	
	static DWORD time = now, second = now, fps = 0, second2 = 0, twice = 0; 
	
	if(now<time){ now+=~0u-time; time = 0; } //paranoia (rollover)

	DDRAW::noTicks = DDRAW::isPaused?0:now-time; time = now; 

	fps++; if(now-second>=1000-second2)
	{
		//REMINDER: breaking on 1 and then unpausing
		//and breaking on !1 can capture a whole sec
		if(fps==1)
		{
			fps = fps; //breakpoint
		}
		else
		{
			fps = fps; //breakpoint
		}
		DDRAW::fps = fps; fps = 0; 

		int delta = max(1000,now-second);
		second2 = delta%(1000/max(1,DDRAW::refreshrate));
		if(second2<0) second2 = 0;
		second = now;		
		
		if(0&&DX::debug)
		if(DDRAW::fps+1>DDRAW::refreshrate&&DDRAW::refreshrate&&++twice==2)
		{
			DDRAW::DirectDraw7->WaitForVerticalBlank(1,0); twice = 0;
		}
		else if(twice) twice--;
	}

	/*2021: haven't used GetTickCount in a long time
	//GetTickCount's resolution is about ~16ms
	if(!DDRAW::noTicks&&!DDRAW::isPaused)
	{
		//trying to do immediate (triple buffering?)
		//in which case noTicks must be compared to doFlipEX_MaxTicks
		if(!DDRAW::doFlipEx) 
		DDRAW::noTicks = 15;	
	}*/
						
	return true; 
}
extern bool(*DDRAW::onFlip)() = dx_ddraw_onflip; 

static void dx_ddraw_oneffects(){}
extern void(*DDRAW::onEffects)() = dx_ddraw_oneffects;

extern bool DDRAW::doPause = false;
extern bool DDRAW::isPaused = false;
//return is an undocumented hack (dx.d3d9.cpp)
static bool dx_ddraw_onpause(){ return true; }
extern bool(*DDRAW::onPause)() = dx_ddraw_onpause;

extern unsigned DDRAW::noResets = 0;
static void dx_ddraw_onreset(){ DDRAW::noResets++; DDRAW::noFlips++; }
extern void(*DDRAW::onReset)() = dx_ddraw_onreset;

extern int(*DDRAW::colorkey)(DX::DDSURFACEDESC2*,D3DFORMAT) = 0;	
extern void*(*DDRAW::mipmap)(const DX::DDSURFACEDESC2*,DX::DDSURFACEDESC2*) = 0;

extern DX::D3DLIGHT7 *(*DDRAW::light)(DWORD,intptr_t) = 0;

extern void(*DDRAW::lightlost)(DWORD,bool) = 0;

extern int DDRAW::vsPresentState = 0;
extern int DDRAW::vsProj[4]				= {0,0,0,0};
extern int DDRAW::vsWorldViewProj[4]    = {0,0,0,0};
extern int DDRAW::vsWorldView[4]        = {0,0,0,0};
extern int DDRAW::vsInvWorldView[4]     = {0,0,0,0};
extern int DDRAW::vsWorld[4]	        = {0,0,0,0};
extern int DDRAW::vsView[4]		        = {0,0,0,0};
extern int DDRAW::vsInvView[4]          = {0,0,0,0}; 
extern int DDRAW::vsTexture0[4]         = {0,0,0,0};
extern int DDRAW::vsFogFactors          = 0;
extern int DDRAW::vsColorFactors		= 0;
extern int DDRAW::vsMaterialAmbient     = 0;
extern int DDRAW::vsMaterialDiffuse     = 0;
extern int DDRAW::vsMaterialEmitted     = 0;
extern int DDRAW::vsGlobalAmbient       = 0;
extern int DDRAW::vsLightsAmbient[16]   = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
extern int DDRAW::vsLightsDiffuse[16]   = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
extern int DDRAW::vsLightsVectors[16]   = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
extern int DDRAW::vsLightsFactors[16]   = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

extern int DDRAW::vsI = 0, DDRAW::vsB = 0;

extern int DDRAW::vsLights = 0;

extern int DDRAW::psColorkey = DDRAW::psColorkey_nz;

extern int DDRAW::psColorize = 0;
extern int DDRAW::psFarFrustum = 0;
//extern int DDRAW::psTextureState = 0; //UNUSED/EXPERIMENTAL

extern int DDRAW::ps2ndSceneMipmapping2 = 0;

extern int DDRAW::psI = 0, DDRAW::psB = 0;

extern const DWORD *DDRAW::vshaders9[16+4] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; 
extern const DWORD *DDRAW::pshaders9[16+4] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; 

extern const char *DDRAW::vshadersGL_include = 0, *DDRAW::pshadersGL_include = 0;

extern int DDRAW::vs = 0, DDRAW::ps = 0, DDRAW::fx = 16;	

extern DWORD DDRAW::fxColorize = 0;
extern int DDRAW::fxInflateRect = 0;
extern unsigned DDRAW::fxStrobe = 0;
extern bool DDRAW::fx2ndSceneBuffer = 0;
extern float DDRAW::fxCounterMotion = 0;

extern D3DFORMAT DDRAW::mrtargets9[4] = 
{(D3DFORMAT)0,(D3DFORMAT)0,(D3DFORMAT)0,(D3DFORMAT)0};
extern D3DFORMAT DDRAW::altargets9[4] = 
{(D3DFORMAT)0,(D3DFORMAT)0,(D3DFORMAT)0,(D3DFORMAT)0};
extern int DDRAW::doClearMRT = false; 
extern const char *DDRAW::ClearMRT[4] = {0,0,0,0};

static DWORD dx_ddraw_getDWORD;

static DX::D3DMATRIX dx_ddraw_getD3DMATRIX;

extern DWORD *DDRAW::getDWORD = &dx_ddraw_getDWORD;

extern DX::D3DMATRIX *DDRAW::getD3DMATRIX = &dx_ddraw_getD3DMATRIX;

static const DDPIXELFORMAT dx_ddraw_X8R8G8B8 =
{ 
	sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0 
};
static const DDPIXELFORMAT dx_ddraw_R5G6B5 =
{ 
	sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 16, 0x0000f800, 0x000007e0, 0x0000001f, 0 
};

static IDirectDrawSurface7 *dx_ddraw_brights[9] = {0,0,0,0,0,0,0,0,0};
static IDirectDrawSurface7 *dx_ddraw_badbright = (::IDirectDrawSurface7*)1;

static int dx_ddraw_brightsonoff(int on)
{
	int clamp = max(min(DDRAW::bright,8),0);
/*
	DWORD test;
	if(DDRAW::Direct3DDevice7->
	GetRenderState(DX::D3DRENDERSTATE_LIGHTING,&test)==D3D_OK)
	DDRAW::isLit = test?true:false; else assert(0);
*/
	int lv = DDRAW::isLit?DDRAW::brights[clamp]:0;

	if(0) if(on==lv) //hack...
	{
		if(on&&dx_ddraw_brights[clamp]) //what the hell is going on here???
		{
			//something somewhere (outside of Ex) is somehow unsetting this
			//DDRAW::Direct3DDevice7->proxy7->SetTexture(1,dx_ddraw_brights[clamp]);

			::LPDIRECTDRAWSURFACE7 test;
			if(DDRAW::Direct3DDevice7->proxy7->GetTexture(DDRAW_TEX0,&test)==D3D_OK)
			{
				if(test==0)
				DDRAW::Direct3DDevice7->proxy7->SetTexture(DDRAW_TEX0,dx_ddraw_brights[clamp]);

				else assert(test==dx_ddraw_brights[clamp]);

				//DDRAW::Direct3DDevice7->proxy7->SetTexture(0,0);
				//DDRAW::Direct3DDevice7->proxy7->SetTexture(0,test);
			}
			else assert(0);
		}

		return on; 
	}
	else on = lv;

	if(on!=lv) on = lv; else return on; 

	IDirect3DDevice7 *d3dd7 = DDRAW::Direct3DDevice7->proxy7;	

	if(on)
	{
		if(!dx_ddraw_brights[clamp])
		{
			DDSURFACEDESC2 desc = {sizeof(DDSURFACEDESC2)};

			desc.dwFlags|=DDSD_HEIGHT|DDSD_WIDTH|DDSD_CAPS|DDSD_PIXELFORMAT;
			desc.dwWidth = desc.dwHeight = 1;

			desc.ddpfPixelFormat = dx_ddraw_X8R8G8B8;
			desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;

			if(DDRAW::DirectDraw7->proxy7->
			CreateSurface(&desc,&dx_ddraw_brights[clamp],0)==DD_OK)
			{
				DDBLTFX fill = {sizeof(DDBLTFX)};

				fill.dwFillColor = 0xFF000000|lv<<16|lv<<8|lv;

				HRESULT debug = dx_ddraw_brights[clamp]->Blt(0,0,0,DDBLT_COLORFILL,&fill);

				assert(debug==DD_OK);
			}
			else dx_ddraw_brights[clamp] = dx_ddraw_badbright;
		}

		if(dx_ddraw_brights[clamp]==dx_ddraw_badbright) return on;
		
		HRESULT ok = D3D_OK;

		ok|=d3dd7->SetTexture(DDRAW_TEX0,dx_ddraw_brights[clamp]);

		ok|=d3dd7->SetTextureStageState(DDRAW_TEX0,D3DTSS_COLOROP,D3DTOP_ADDSMOOTH);
		ok|=d3dd7->SetTextureStageState(DDRAW_TEX0,D3DTSS_COLORARG1,D3DTA_CURRENT);
		ok|=d3dd7->SetTextureStageState(DDRAW_TEX0,D3DTSS_COLORARG2,D3DTA_TEXTURE);
		ok|=d3dd7->SetTextureStageState(DDRAW_TEX0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);
		ok|=d3dd7->SetTextureStageState(DDRAW_TEX0,D3DTSS_ALPHAARG1,D3DTA_CURRENT);

		assert(ok==D3D_OK);
	}
	else
	{
		d3dd7->SetTextureStageState(DDRAW_TEX0,D3DTSS_COLOROP,D3DTOP_DISABLE);
		d3dd7->SetTextureStageState(DDRAW_TEX0,D3DTSS_ALPHAOP,D3DTOP_DISABLE);

		d3dd7->SetTexture(DDRAW_TEX0,0);
	}

	return on;
}

static DWORD dx_ddraw_dimmer = 256; //black light

static int dx_ddraw_dimmingonoff(int on)
{			
	int clamp = max(min(DDRAW::dimmer,8),0);

	int lv = DDRAW::isLit?DDRAW::dimmers[clamp]:0;

	if(on!=lv) on = lv; else return on; 

	IDirect3DDevice7 *d3dd7 = DDRAW::Direct3DDevice7->proxy7;	
	
	if(on)
	{
		::D3DLIGHT7 lit; memset(&lit,0x00,sizeof(::D3DLIGHT7));

		lit.dcvAmbient.r = lit.dcvAmbient.g = lit.dcvAmbient.b = -float(lv)/256.0f;

		lit.dltType = ::D3DLIGHT_POINT; lit.dvRange = 10000; //DX7 max not FLT_MAX

		lit.dvAttenuation0 = 1.0f; //no attenuation

		d3dd7->SetLight(dx_ddraw_dimmer,&lit);
		d3dd7->LightEnable(dx_ddraw_dimmer,1);
	}
	else d3dd7->LightEnable(dx_ddraw_dimmer,0);

	return on;
}

static void dx_ddraw_level(bool reset = false)
{		
	int clampb = max(min(DDRAW::bright,8),0);
	int clampd = max(min(DDRAW::dimmer,8),0);

	//brights
	{
		static int on = 0;

		if(!reset) 
		{
			static bool was_ever_on = false;

			if(was_ever_on)
			{
				on = dx_ddraw_brightsonoff(on); 
			}
			else if(DDRAW::brights[clampb])
			{
				on = dx_ddraw_brightsonoff(on); 

				was_ever_on = true;
			}
		}
		else on = 0;
	}
	//dimmers
	{
		static int on = 0;

		if(!reset) 
		{
			static bool was_ever_on = false;

			if(was_ever_on)
			{
				on = dx_ddraw_dimmingonoff(on); 
			}
			else if(DDRAW::dimmers[clampd])
			{
				on = dx_ddraw_dimmingonoff(on); 

				was_ever_on = true;
			}
		}
		else on = 0;
	}
}
static bool dx_ddraw_d3ddrivercaps(DX::D3DDEVICEDESC7 &in, DX::D3DDEVICEDESC *hal, DX::D3DDEVICEDESC *hel=0) 
{		
	if(hal&&hel)
	{
		assert(hal->dwSize==hel->dwSize);

		if(hal->dwSize!=hel->dwSize) return false;
	}
	else if(!hal&&!hel) return false;
			
	static const size_t _fsz = size_t(&((LPD3DDEVICEDESC)0)->dwMaxVertexCount)+sizeof(DWORD);
	static const size_t _5sz = size_t(&((LPD3DDEVICEDESC)0)->dwMaxStippleHeight)+sizeof(DWORD);
	static const size_t _6sz = size_t(&((LPD3DDEVICEDESC)0)->wMaxSimultaneousTextures)+sizeof(WORD);
		
	DX::LPD3DDEVICEDESC inout = hal?hal:hel;
	
	if(inout->dwSize<_fsz) return false;

	inout->dwFlags = 0xFFF; //all in

	inout->dcmColorModel = D3DCOLOR_MONO;

	inout->dtcTransformCaps.dwSize = sizeof(D3DTRANSFORMCAPS);
	inout->dtcTransformCaps.dwCaps = D3DTRANSFORMCAPS_CLIP; //assuming

	inout->bClipping = TRUE;

	inout->dlcLightingCaps.dwSize = sizeof(_D3DLIGHTINGCAPS);
	inout->dlcLightingCaps.dwCaps = D3DLIGHTCAPS_POINT|D3DLIGHTCAPS_DIRECTIONAL;
	inout->dlcLightingCaps.dwLightingModel = D3DLIGHTINGMODEL_RGB;
	inout->dlcLightingCaps.dwNumLights = 65535; //bogus: most likely unlimited

	inout->dpcLineCaps = in.dpcLineCaps;
	inout->dpcTriCaps = in.dpcTriCaps;

	inout->dwDeviceRenderBitDepth	= in.dwDeviceRenderBitDepth;
	inout->dwDeviceZBufferBitDepth	= in.dwDeviceZBufferBitDepth;

	inout->dwMaxBufferSize = 65535*sizeof(D3DTLVERTEX); //bogus
	inout->dwMaxVertexCount = 65535; //bogus: most likely unlimited

	if(inout->dwSize>=_5sz)
	{	
		inout->dwMinTextureWidth = in.dwMinTextureWidth;
		inout->dwMinTextureHeight = in.dwMinTextureHeight;
		inout->dwMaxTextureWidth = in.dwMaxTextureWidth;
		inout->dwMaxTextureHeight = in.dwMaxTextureHeight;

		inout->dwMinStippleWidth = inout->dwMinStippleHeight = 2;
		inout->dwMaxStippleWidth = inout->dwMaxStippleHeight = 32;
	}

	if(inout->dwSize>=_6sz)
	{
		//these two structures are identical between these ranges
		size_t cp = (BYTE*)(inout+1)-(BYTE*)&inout->dwMaxTextureRepeat;
		memcpy(&inout->dwMaxTextureRepeat,&in.dwMaxTextureRepeat,cp);
	}

	if(hal&&hel) memcpy(hel,&hal,hel->dwSize);

	inout->dwDevCaps = 
	D3DDEVCAPS_FLOATTLVERTEX|
	D3DDEVCAPS_EXECUTESYSTEMMEMORY|
	D3DDEVCAPS_TLVERTEXSYSTEMMEMORY|
	D3DDEVCAPS_TEXTURESYSTEMMEMORY|
	D3DDEVCAPS_DRAWPRIMTLVERTEX|
	D3DDEVCAPS_CANRENDERAFTERFLIP| //assuming
	D3DDEVCAPS_DRAWPRIMITIVES2|
//	D3DDEVCAPS_SEPARATETEXTUREMEMORIES|
	D3DDEVCAPS_DRAWPRIMITIVES2EX;

	if(hal&&hel) hel->dwDevCaps = hal->dwDevCaps;

	if(inout!=hel)
	inout->dwDevCaps|=
	D3DDEVCAPS_EXECUTEVIDEOMEMORY|
	D3DDEVCAPS_TLVERTEXVIDEOMEMORY|
	D3DDEVCAPS_TEXTUREVIDEOMEMORY|
	D3DDEVCAPS_TEXTURENONLOCALVIDMEM;

	return true;
}

void DDRAW::log_display_mode() 
{
	if(8>DDRAWLOG::debug) return;

	DEVMODE out; out.dmSize = sizeof(DEVMODE); //overkill??

	out.dmDriverExtra = 0; //what is this again?

	if(!EnumDisplaySettings(0,ENUM_CURRENT_SETTINGS,&out)) return;

	DDRAW_LEVEL(8) << "Display Mode is " << out.dmPelsWidth << 'x' << out.dmPelsHeight << '\n';	
}

//DDRAW::log_surface_desc
#define DWORD_(X) if(desc->X){ DDRAW_LEVEL(8) << #X " is " << *(dwOp=&desc->X) << "\n"; }else dwOp = 0;

#define DWORD_AND(X) if(dwOp&&*dwOp&X){ DDRAW_LEVEL(8) << #X "\n"; }

#define LONG_(X) if(desc->X){ DDRAW_LEVEL(8) << #X " is " << desc->X << "\n"; }

extern void DDRAW::log_surface_desc(DX::DDSURFACEDESC2 *desc)
{
	if(!desc||8>DDRAWLOG::debug) return;

	DWORD *dwOp = 0;

	#include "log.ddsurfdesc.inl"
}

#undef DWORD_
#undef DWORD_AND
#undef LONG_

//DDRAW::log_surface_bltfx
#define DWORD_(X) if(fx->X){ DDRAW_LEVEL(8) << #X " is " << *(dwFx=&fx->X) << "\n"; }else dwFx = 0;

#define DWORD_AND(X) if(dwFx&&*dwFx&X){ DDRAW_LEVEL(8) << #X "\n"; }

#define DWORD_(X) if(fx->X){ DDRAW_LEVEL(8) << #X " is " << *(dwFx=&fx->X) << "\n"; }else dwFx = 0;

#define DWORD_AND(X) if(dwFx&&*dwFx&X){ DDRAW_LEVEL(8) << #X "\n"; }
void DDRAW::log_surface_bltfx(DX::DDBLTFX *fx)
{
	if(!fx||8>DDRAWLOG::debug) return;

	DWORD *dwFx = 0;

	#include "log.ddbltfx.inl"
}

#undef DWORD_
#undef DWORD_AND

//DDRAW::log_driver_caps
#define DWORD_(x) if(hal&&hal->x||hel&&hel->x){ DDRAW_LEVEL(3) << #x " is ";\
	if(hal){ DDRAW_LEVEL(8) << *(dwHal=&hal->x) << " "; }else dwHal = 0;\
	if(hel){ DDRAW_LEVEL(8) << "(" << *(dwHel=&hel->x) << ")"; }else dwHel = 0;\
			DDRAW_LEVEL(8) << "\n";}
#define DWORD_I(x,i) if(hal&&hal->x[i]||hel&&hel->x[i]){ DDRAW_LEVEL(3) << #x "[" << i << "] is ";\
	if(hal) DDRAW_LEVEL(8) << *(dwHal=&hal->x[i]) << " "; else dwHal = 0;\
	if(hel) DDRAW_LEVEL(8) << "(" << *(dwHel=&hel->x[i]) << ")"; else dwHel = 0;\
			DDRAW_LEVEL(8) << "\n";}		   
#define DWORD_AND(x)\
	if((dwHal&&*dwHal&x)||(dwHel&&*dwHel&x)){\
	if(dwHal&&*dwHal&x) DDRAW_LEVEL(8) << #x;\
	if(dwHel&&*dwHel&x) DDRAW_LEVEL(8) << "(" << #x << ")";\
						DDRAW_LEVEL(8) << "\n";}
/*
#define DWORD_(x) {\
	if(hal) DDRAW_LEVEL(8) << "if(hal) hal->" #x " = " << hal->x << ";\n";\
	if(hel) DDRAW_LEVEL(8) << "if(hel) hel->" #x " = " << hel->x << ";\n";}
#define DWORD_I(x,i)  {\
	if(hal) DDRAW_LEVEL(8) << "if(hal) hal->" #x "["<< i << "] = " << hal->x[i] << ";\n";\
	if(hel) DDRAW_LEVEL(8) << "if(hel) hel->" #x "["<< i << "] = " << hel->x[i] << ";\n";}
#define DWORD_AND(x)
*/

extern void DDRAW::log_driver_caps(DX::DDCAPS_DX7 *hal, DX::DDCAPS_DX7 *hel)
{
	if(!hal&&!hel) return;

	if(8>DDRAWLOG::debug) return;

	DDRAW_LEVEL(8) << "Logging driver capabilities (HEL caps in parenthesis)\n"; 

	DWORD *dwHal = 0, *dwHel = 0;

	#include "log.ddcaps.inl"

}

#undef DWORD_
#undef DWORD_I
#undef DWORD_AND

//DDRAW::log_device_desc
#define WORD_(X) if(desc->X){ DDRAW_LEVEL(8) << #X " is " << (int)desc->X << "\n"; }
#define DWORD_(X) if(desc->X){ DDRAW_LEVEL(8) << #X " is " << *(dwOp=&desc->X) << "\n"; }else dwOp = 0;
#define DWORD_AND(X) if(dwOp)if(*dwOp&X){ DDRAW_LEVEL(8) << #X "\n"; }
#define D3DVALUE_(X) if(desc->X){ DDRAW_LEVEL(8) << #X " is " << desc->X << "\n"; }
#define GUID_(X) { StringFromGUID2(desc->X,g,64);\
	DDRAW_LEVEL(8) << #X " is " << ' ' << g << "\n"; }

extern void DDRAW::log_device_desc(DX::D3DDEVICEDESC *desc, const char *pre)
{
	if(!desc||8>DDRAWLOG::debug) return;

	if(pre) DDRAW_LEVEL(8) << pre;

	DWORD *dwOp = 0; OLECHAR g[64];

	#define D3DDEVICEDESC_VERSION 6
	#include "log.3ddevdesc.inl"
	#undef D3DDEVICEDESC_VERSION
}

extern void DDRAW::log_device_desc(DX::D3DDEVICEDESC7 *desc, const char *pre)
{
	if(!desc||8>DDRAWLOG::debug) return;

	if(pre) DDRAW_LEVEL(8) << pre;

	DWORD *dwOp = 0; OLECHAR g[64];

	#define D3DDEVICEDESC_VERSION 7
	#include "log.3ddevdesc.inl"
	#undef D3DDEVICEDESC_VERSION
}

#undef WORD_
#undef DWORD_
#undef DWORD_AND
#undef D3DVALUE_
#undef GUID_	

enum{ dx_ddraw_enum_setsN=4 }; //32 //recursive???
static void *dx_ddraw_enum_sets[dx_ddraw_enum_setsN][3];
static void *dx_ddraw_reserve_enum_set(void *callback, void *original_context)
{
	for(int i=0;i<dx_ddraw_enum_setsN;i++) if(!dx_ddraw_enum_sets[i][2])
	{
		dx_ddraw_enum_sets[i][0] = callback;
		dx_ddraw_enum_sets[i][1] = original_context;
		dx_ddraw_enum_sets[i][2] = (void*)1;

		//assert(i<1); //2021: recursive??? //muiltiple APIs share this system
		assert(i<2); //SOM_MAP

		return (void*)i;		
	}
	assert(0); return 0;
}
static void **dx_ddraw_enum_set(void *new_context)
{
	if((int)new_context>=0&&(int)new_context<dx_ddraw_enum_setsN) 
	if(dx_ddraw_enum_sets[(int)new_context][2])		
	return dx_ddraw_enum_sets[(int)new_context];
	assert(0); return 0;
}
static void dx_ddraw_return_enum_set(void *new_context)
{
	dx_ddraw_enum_sets[int(new_context)][2] = 0;
}

//sharing with dx.d3d9c.cpp 
extern void **dx_ddraw_enums = 0; 

extern bool DDRAW::intercept_callback(DDRAW::Enum e, void *f, void *g)
{	
	assert(!g); //unimplemented

	if(e<0||e>=DDRAW::TOTAL_HACKS) return false;

	if(!dx_ddraw_enums)
	{
		if(!f) return true;
		
		dx_ddraw_enums = new void*[DDRAW::TOTAL_ENUMS];

		memset(dx_ddraw_enums,0x00,sizeof(void*)*DDRAW::TOTAL_ENUMS);
	}

	dx_ddraw_enums[e] = f; return true;
}

#define DDRAW_HAVE_ENUM(e) dx_ddraw_enums&&dx_ddraw_enums[DDRAW::e##_ENUM]

#define DDRAW_PASS_ENUM(e,ret,cb,...) ((ret(*)(void*,__VA_ARGS__))(dx_ddraw_enums[DDRAW::e##_ENUM]))

static HRESULT CALLBACK dx_ddraw_enumdevices3cb(GUID *guid, LPSTR description, LPSTR name, DX::LPD3DDEVICEDESC hal, DX::LPD3DDEVICEDESC hel, LPVOID passthru)
{
	DDRAW_LEVEL(7) << "Logging Direct3D3::EnumDevices Enumeration...\n";

	void **p = dx_ddraw_enum_set(passthru); //assert(0);
	
	if(!p) DDRAW_ERROR(0) << "ERROR! Passthru not setup correctly. Logging anyway.\n";
			
	DDRAW_LEVEL(7) << description << '\n' << name  << '\n';

	if(hal)
	{
		DDRAW::log_device_desc(hal,"\nLogging HAL Device Descriptor\n");
	}																 
	if(hel)
	{
		DDRAW::log_device_desc(hel,"\nLogging HEL Device Descriptor\n");
	}
	
	//hack! and theoretically dangerous
	if(hal&&hal->dwDeviceRenderBitDepth)
	hal->dwDeviceRenderBitDepth|=DDRAW::force_color_depths; //hack
	if(hel&&hel->dwDeviceRenderBitDepth) 
	hel->dwDeviceRenderBitDepth|=DDRAW::force_color_depths; //hack
	
	if(p) //extreme paranoia
	if(DDRAW_HAVE_ENUM(DIRECT3D3_ENUMDEVICES))
	{
		return DDRAW_PASS_ENUM(DIRECT3D3_ENUMDEVICES,
		HRESULT,DX::LPD3DENUMDEVICESCALLBACK,GUID*,LPSTR,LPSTR,
		DX::LPD3DDEVICEDESC,DX::LPD3DDEVICEDESC,LPVOID)(p[0],guid,description,name,hal,hel,p[1]);
	}
	else return ((DX::LPD3DENUMDEVICESCALLBACK)p[0])(guid,description,name,hal,hel,p[1]);
	
	return D3DENUMRET_OK;
}
static HRESULT CALLBACK dx_ddraw_enumdevices3cb(GUID *guid, LPSTR description, LPSTR name, ::LPD3DDEVICEDESC hal, ::LPD3DDEVICEDESC hel, LPVOID passthru)
{
	return dx_ddraw_enumdevices3cb(guid,description,name,(DX::LPD3DDEVICEDESC)hal,(DX::LPD3DDEVICEDESC)hel,passthru);
}

static HRESULT CALLBACK dx_ddraw_enumdevices723cb(LPSTR description, LPSTR name, DX::LPD3DDEVICEDESC7 desc, LPVOID passthru)
{
	assert(0); //TODO: make sure hal/hel look more like dx_ddraw_enumdevices3cb

	DDRAW_LEVEL(7) << "Logging Direct3D3::EnumDevices Enumeration...\n";

	void **p = dx_ddraw_enum_set(passthru); 
	
	if(!p) DDRAW_ERROR(0) << "ERROR! Passthru not setup correctly. Logging anyway.\n";
			
	DDRAW_LEVEL(7) << description << '\n' << name  << '\n';

	DDRAW::log_device_desc(desc);

	if(!desc) return D3DENUMRET_OK;
			
	DX::D3DDEVICEDESC hal = {sizeof(D3DDEVICEDESC)}, hel = {sizeof(D3DDEVICEDESC)};
	
	dx_ddraw_d3ddrivercaps(*desc,&hal,&hel);

	if(hal.dwDeviceRenderBitDepth) 
	hal.dwDeviceRenderBitDepth|=DDRAW::force_color_depths; //hack
	if(hal.dwDeviceRenderBitDepth) 
	hel.dwDeviceRenderBitDepth|=DDRAW::force_color_depths; //hack 

	if(p) return ((DX::LPD3DENUMDEVICESCALLBACK)p[0])(&desc->deviceGUID,description,name,&hal,&hel,p[1]);

	return D3DENUMRET_OK;
}

static HRESULT CALLBACK dx_ddraw_enumdevices7cb(LPSTR description, LPSTR name, DX::LPD3DDEVICEDESC7 desc, LPVOID passthru)
{
	DDRAW_LEVEL(7) << "Logging Direct3D7::EnumDevices Enumeration...\n";

	void **p = dx_ddraw_enum_set(passthru); 
	
	if(!p) DDRAW_ERROR(0) << "ERROR! Passthru not setup correctly. Logging anyway.\n";
			
	DDRAW_LEVEL(7) << description << '\n' << name  << '\n';

	DDRAW::log_device_desc(desc);

	if(desc) desc->dwDeviceRenderBitDepth|=DDRAW::force_color_depths; //hack

	if(p) return ((DX::LPD3DENUMDEVICESCALLBACK7)p[0])(description,name,desc,p[1]);

	return D3DENUMRET_OK;
}

static BOOL CALLBACK dx_ddraw_directdrawenumerateexacb(GUID *guid, LPSTR a, LPSTR b, LPVOID passthru, HMONITOR hm)
{
	DDRAW_LEVEL(7) << "Logging DirectDrawEnumerateExA Enumeration...\n";

	void **p = dx_ddraw_enum_set(passthru); 
	
	if(!p) DDRAW_ERROR(0) << "ERROR! Passthru not setup correctly. Logging anyway.\n";

	OLECHAR g[64] = OLESTR("{guid is not defined}"); 
	
	if(guid) StringFromGUID2(*guid,g,64);

	DDRAW_LEVEL(7) << g << '\n' << a << '\n' << b  << '\n' << hm  << '\n';

	if(p) 
	{
		HRESULT debug = ((DX::LPDDENUMCALLBACKEXA)p[0])(guid,a,b,p[1],hm);

		return debug;
	}

	return DDENUMRET_OK;
}

static HRESULT CALLBACK dx_ddraw_enumdisplaymodescb(int bpp, DX::LPDDSURFACEDESC2 desc, LPVOID passthru)
{
	DDRAW_LEVEL(7) << "Logging EnumDisplayModes enumeration...\n";

	if(!desc) return DDENUMRET_OK; //paranoia

	if(desc->ddpfPixelFormat.dwRGBBitCount!=bpp) return DDENUMRET_OK;
	
	if(desc->dwWidth!=DDRAW::resolution[0]||desc->dwHeight!=DDRAW::resolution[1]) //safemode
	{
		if((int)desc->dwWidth<DDRAW::xyMinimum[0]
		||(int)desc->dwHeight<DDRAW::xyMinimum[1]) return DDENUMRET_OK;

		if(DDRAW::aspectratio=='4:3')
		{
			if(fabs(float(desc->dwWidth)/float(desc->dwHeight)-1.333333f)>0.00001f)
			return DDENUMRET_OK;
		}
	}

	if(!DDRAW::fullscreen) //if(DDRAW::inWindow)
	{
		UINT vw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
		UINT vh = GetSystemMetrics(SM_CYVIRTUALSCREEN);

		if(vw&&vh) //see resolution fits into virtual screen space
		{
			if(desc->dwWidth>vw||desc->dwHeight>vh) return DDENUMRET_OK;
		}
	}

	void **p = dx_ddraw_enum_set(passthru); 
	
	if(!p) DDRAW_ERROR(0) << "ERROR! Passthru not setup correctly. Logging anyway.\n";

	DDRAW::log_surface_desc(desc); 

	if(p) return ((DX::LPDDENUMMODESCALLBACK2)p[0])(desc,p[1]);

	return DDENUMRET_OK;
}
static HRESULT CALLBACK dx_ddraw_enumdisplaymodes16(DX::LPDDSURFACEDESC2 desc, LPVOID passthru)
{
	return dx_ddraw_enumdisplaymodescb(16,desc,passthru);
}
static HRESULT CALLBACK dx_ddraw_enumdisplaymodes32(DX::LPDDSURFACEDESC2 desc, LPVOID passthru)
{
	return dx_ddraw_enumdisplaymodescb(32,desc,passthru);
}

static const char *dx_draw_display(HMONITOR hmonitor);

static int dx_ddraw_restoredisplaymode[5] = {0,0,0,0,0};

bool DDRAW::SetDisplayMode(DWORD x, DWORD y, DWORD z, DWORD w, DWORD q)	 
{
	if(z) DDRAW::bitsperpixel = z;

	assert(!z||z==16||z==32); //bits per pixel (want to know otherwise)

	//defaults: fingers crossed (just copying input)
	dx_ddraw_restoredisplaymode[0] = x;
	dx_ddraw_restoredisplaymode[1] = y;
	dx_ddraw_restoredisplaymode[2] = z;
	dx_ddraw_restoredisplaymode[3] = w; //ignoring refresh rate
	dx_ddraw_restoredisplaymode[4]++; //hack: counting nesting

	assert(dx_ddraw_restoredisplaymode[4]==1);

	if(!DDRAW::fullscreen) //DDRAW::inWindow
	{
		RECT client; int bpp = DDRAW::bitsperpixel; //hack??

		//note: assuming undecorated window
		if(DDRAW::window_rect(&client))
		{
			dx_ddraw_restoredisplaymode[0] = client.right;
			dx_ddraw_restoredisplaymode[1] = client.bottom;
			dx_ddraw_restoredisplaymode[2] = bpp;
		}
	}
	else //fullscreen
	{
		DEVMODEA dm = { "",0,0,sizeof(dm),0 };

		const char *display = dx_draw_display(DDRAW::monitor);

		if(EnumDisplaySettingsExA(display,ENUM_CURRENT_SETTINGS,&dm,0))
		{
			dx_ddraw_restoredisplaymode[0] = dm.dmFields&DM_PELSWIDTH?dm.dmPelsWidth:0;
			dx_ddraw_restoredisplaymode[1] = dm.dmFields&DM_PELSHEIGHT?dm.dmPelsHeight:0;
			dx_ddraw_restoredisplaymode[2] = dm.dmFields&DM_BITSPERPEL?dm.dmBitsPerPel:0;
		}

		//TODO: do_not_compromise_fullscreen_mode
		if(!z||DDRAW::target=='dx9c') z = dm.dmFields&DM_BITSPERPEL?dm.dmBitsPerPel:32; //hack

		if(dx_ddraw_restoredisplaymode[0]!=x //avoids the screen going all googly 
		 ||dx_ddraw_restoredisplaymode[1]!=y) //warning: not checking stretch mode
		for(DWORD i=0;EnumDisplaySettingsExA(display,i,&dm,0);i++)
		{
			if(dm.dmFields&(DM_PELSWIDTH|DM_PELSHEIGHT|DM_BITSPERPEL))
			{
				if(dm.dmPelsWidth==x&&dm.dmPelsHeight==y&&dm.dmBitsPerPel==z)
				{
					if(w&&(dm.dmFields&DM_DISPLAYFREQUENCY))
					if(dm.dmDisplayFrequency!=w&&dm.dmDisplayFrequency>1) 
					continue;

					dm.dmDisplayFixedOutput = DDRAW::isStretched?DMDFO_STRETCH:DMDFO_CENTER;

					if(ChangeDisplaySettingsExA(display,&dm,0,CDS_FULLSCREEN,0)!=DISP_CHANGE_SUCCESSFUL)
					{
						dm.dmDisplayFixedOutput = DMDFO_DEFAULT;

						ChangeDisplaySettingsExA(display,&dm,0,CDS_FULLSCREEN,0);
					} 
					break;
				}
			}
		}
	}

	//note: assuming undecorated window
	//2018: seems to somehow change the window position???
	//when did this start??? 
	//calls SetDisplayMode...
	//DDRAW::SetDisplayMode calls SetWindowPos on child window
	//user32.dll moves this, the parent window???!??!?!?!
	//SetWindowPos(DDRAW::window,0,0,0,x,y,SWP_NOMOVE|SWP_NOZORDER);
	HWND winsanity = GetAncestor(DDRAW::window,GA_ROOT);
	RECT save; GetWindowRect(winsanity,&save);
	{
		SetWindowPos(DDRAW::window,0,0,0,x,y,SWP_NOMOVE|SWP_NOZORDER|SWP_NOSENDCHANGING);
	}
	SetWindowPos(winsanity,0,save.left,save.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOSENDCHANGING);

	return true;
}

bool DDRAW::RestoreDisplayMode()
{
	int x = dx_ddraw_restoredisplaymode[0], y = dx_ddraw_restoredisplaymode[1];
	int z = dx_ddraw_restoredisplaymode[2], w = dx_ddraw_restoredisplaymode[3];
		
	dx_ddraw_restoredisplaymode[4]--; //couting nesting

	assert(dx_ddraw_restoredisplaymode[4]==0);
		
	if(DDRAW::fullscreen) //!DDRAW::inWindow
	{
		DEVMODEA dm = { "",0,0,sizeof(dm),0 };

		const char *display = dx_draw_display(DDRAW::monitor);
				
		if(EnumDisplaySettingsExA(display,ENUM_CURRENT_SETTINGS,&dm,0))
		{
			dx_ddraw_restoredisplaymode[0] = dm.dmFields&DM_PELSWIDTH?dm.dmPelsWidth:0;
			dx_ddraw_restoredisplaymode[1] = dm.dmFields&DM_PELSHEIGHT?dm.dmPelsHeight:0;
			dx_ddraw_restoredisplaymode[2] = dm.dmFields&DM_BITSPERPEL?dm.dmBitsPerPel:0;
		}

		//TODO: do_not_compromise_fullscreen_mode
		if(!z||DDRAW::target=='dx9c') z = dm.dmFields&DM_BITSPERPEL?dm.dmBitsPerPel:32; //hack

		if(dx_ddraw_restoredisplaymode[0]!=x //avoids the screen flicker
		 ||dx_ddraw_restoredisplaymode[1]!=y) //warninge: not checking stretch mode
		for(DWORD i=0;EnumDisplaySettingsExA(display,i,&dm,0);i++)
		{
			if(dm.dmFields&(DM_PELSWIDTH|DM_PELSHEIGHT|DM_BITSPERPEL))
			{
				if(dm.dmPelsWidth==x&&dm.dmPelsHeight==y&&dm.dmBitsPerPel==z)
				{
					if(w&&(dm.dmFields&DM_DISPLAYFREQUENCY))
					if(dm.dmDisplayFrequency!=w&&dm.dmDisplayFrequency>1) 
					continue;

					dm.dmDisplayFixedOutput = DDRAW::isStretched?DMDFO_STRETCH:DMDFO_CENTER;

					if(ChangeDisplaySettingsExA(display,&dm,0,CDS_FULLSCREEN,0)!=DISP_CHANGE_SUCCESSFUL)
					{
						dm.dmDisplayFixedOutput = DMDFO_DEFAULT;

						ChangeDisplaySettingsExA(display,&dm,0,CDS_FULLSCREEN,0);
					}
					break;
				}
			}
		}
	}

	if(z) DDRAW::bitsperpixel = z;

	//note: assuming undecorated window
	SetWindowPos(DDRAW::window,HWND_TOP,0,0,x,y,SWP_NOMOVE|SWP_NOOWNERZORDER);
	
	return true;
}

static int dx_ddraw_scenestate = 0;
static int dx_ddraw_scenestack = 0;

extern bool DDRAW::PushScene()
{
	//REMINDER: according to the MSDN documentation calling BeginScene
	//and EndScene more than once before Present is not intended but is
	//allowed... so this practice should be avoided ideally... I didn't
	//know that when I set it up, I assumed it was like glBegin/glEnd()

	if(!DDRAW::Direct3DDevice7) return false;

	dx_ddraw_scenestack++; assert(dx_ddraw_scenestack>=1);

	if(!DDRAW::inScene)
	{
		assert(dx_ddraw_scenestack==1);

		if(DDRAW::Direct3DDevice7->BeginScene()!=D3D_OK) goto reset;

		dx_ddraw_scenestate = 1; 
	}	

	return true;
		
reset: assert(0); //not good

	dx_ddraw_scenestack = dx_ddraw_scenestate = 0;

	return false; 
}

extern bool DDRAW::PopScene()
{
	if(!DDRAW::Direct3DDevice7) return false;

	dx_ddraw_scenestack--; assert(dx_ddraw_scenestack>=0);

	if(DDRAW::inScene&&dx_ddraw_scenestack==0&&dx_ddraw_scenestate)
	{
		if(DDRAW::Direct3DDevice7->EndScene()!=D3D_OK) goto reset;

		dx_ddraw_scenestate = 0; 
	}

	return true;

reset: assert(0); //not good

	dx_ddraw_scenestack = dx_ddraw_scenestate = 0;

	return false; 
}

extern bool DDRAW::LightEnable(DWORD iL, bool on, DWORD **lost)
{
	if(lost) *lost = 0;

	if(on)									
	{
		for(int i=0;i<DDRAW::lights;i++) 		
		if(DDRAW::lighting[i]==iL) 
		{
			//2017: moving to front of list to handle overflow
			//return false;			
			memmove(DDRAW::lighting+1,DDRAW::lighting,
			sizeof(*DDRAW::lighting)*i);

			DDRAW::lighting[0] = iL;

			return false; //Not added.
		}

		memmove(DDRAW::lighting+1,DDRAW::lighting,
		sizeof(*DDRAW::lighting)*min(DDRAW::lights,16));

		if(DDRAW::lights>=16) 
		{
			assert(DDRAW::lights==16);

			if(lost) *lost = DDRAW::lighting+16;
		}
		else DDRAW::lights++;
		
		DDRAW::lighting[0] = iL; return true;
	}
	else for(int i=0;i<DDRAW::lights;i++) if(DDRAW::lighting[i]==iL) //off
	{			
		memmove(DDRAW::lighting+i,DDRAW::lighting+i+1,
		sizeof(*DDRAW::lighting)*(DDRAW::lights-i-1));

		DDRAW::lights--; return true;		
	}	

	return false; //Not removed.
}

void DDRAW::ApplyStateBlock()
{		
	//developers should add any state you need here//

	DDRAW::IDirect3DDevice7 *p = DDRAW::Direct3DDevice7; 
	
	assert(p); if(!p) return;

	static int paranoia = 0;

	DDRAW::ofApplyStateBlock = true; paranoia++;

	HRESULT hr; 

#define _RENDERSTATE(st)\
	hr = p->GetRenderState(DX::D3DRENDERSTATE_##st,DDRAW::getDWORD);\
	if(hr==D3D_OK) p->SetRenderState(DX::D3DRENDERSTATE_##st,*DDRAW::getDWORD);\

	_RENDERSTATE(LIGHTING)
	_RENDERSTATE(FOGENABLE)

#define _TEXTURESTAGESTATE(st)\
	hr = p->GetTextureStageState(0,DX::D3DTSS_##st,DDRAW::getDWORD);\
	if(hr==D3D_OK) p->SetTextureStageState(0,DX::D3DTSS_##st,*DDRAW::getDWORD);\

	_TEXTURESTAGESTATE(COLOROP)
	_TEXTURESTAGESTATE(COLORARG1)
	_TEXTURESTAGESTATE(COLORARG2)
	_TEXTURESTAGESTATE(ALPHAOP)
	_TEXTURESTAGESTATE(ALPHAARG1)
	_TEXTURESTAGESTATE(ALPHAARG2)

#define _TRANSFORMSTATE(st)\
	hr = p->GetTransform(DX::D3DTRANSFORMSTATE_##st,DDRAW::getD3DMATRIX);\
	if(hr==D3D_OK) p->SetTransform(DX::D3DTRANSFORMSTATE_##st,DDRAW::getD3DMATRIX);\

	_TRANSFORMSTATE(WORLD)
	_TRANSFORMSTATE(VIEW)
	_TRANSFORMSTATE(PROJECTION)

#undef _TRANSFORMSTATE
#undef _TEXTURESTAGESTATE
#undef _RENDERSTATE

	DX::IDirectDrawSurface7 *gettexture;

	hr = p->GetTexture(0,&gettexture);

	if(hr==D3D_OK) 
	{	
		p->SetTexture(0,gettexture);

		if(gettexture) gettexture->Release();
	}

	switch(DDRAW::compat) //DDRAW::target
	{
	case 'd39c':
		
		DDRAW::refresh_state_dependent_shader_constants(); break;

	default: dx_ddraw_level(true);
	}

	if(!--paranoia)
	DDRAW::ofApplyStateBlock = false;

	assert(!paranoia);
}

static const char *dx_draw_display(HMONITOR hmonitor)
{
	DWORD out = 0; if(!hmonitor) return 0;

	//Note: this code is also maintained in EX::capturing_screen(HMONITOR)

	static HMONITOR cachehit = 0; static DWORD cacheval = 0;

	//not thread safe (probably not a problem)
	static DEVMODEA dm = { "",0,0,sizeof(dm),0 };
	static DISPLAY_DEVICEA dev = { sizeof(dev) };

	if(hmonitor!=cachehit) //could use critical section
	{
		//not thread safe (probably not a problem)
		static MONITORINFO mon = { sizeof(MONITORINFO) };

		if(GetMonitorInfo(hmonitor,&mon))
		{
			if(0&&mon.dwFlags&MONITORINFOF_PRIMARY)
			{
				out = cacheval = 0; cachehit = hmonitor;
			}
			else for(DWORD i=0;EnumDisplayDevicesA(0,i,&dev,0);i++)
			{
				//Note: ChangeDisplaySettings (sans Ex) does not expose DM_POSITION 
				if(EnumDisplaySettingsExA(dev.DeviceName,ENUM_CURRENT_SETTINGS,&dm,0))
				{
					if(dm.dmPosition.x==mon.rcMonitor.left&&
					dm.dmPosition.y==mon.rcMonitor.top) //should be enough
					{
						assert(dm.dmPosition.x+dm.dmPelsWidth==mon.rcMonitor.right);
						assert(dm.dmPosition.x+dm.dmPelsHeight==mon.rcMonitor.bottom);

						out = cacheval = i+1; cachehit = hmonitor;
						
						break; //found display identifier for monitor!
					}
				}
			}
		}
		else out = 0; //assuming dx device will be lost soon
	}
	else return *dev.DeviceName?dev.DeviceName:0; 

	if(!out||!EnumDisplayDevicesA(0,out-1,&dev,0)) *dev.DeviceName = 0;

	return *dev.DeviceName?dev.DeviceName:0; 
}

extern void DDRAW::Yelp(const char *Interface)
{
	DDRAW_LEVEL(7) << '~' << Interface << "()\n";
}

#ifdef _DEBUG
extern void DDRAW::Here(void *This)
{
	DDRAW_LEVEL(7) << This << '\n';
}
#endif

//TODO: find a better way
DDRAW::IDirectDrawSurface*
DDRAW::IDirectDrawSurface::get(void* in) //proxy pointer
{
	if(!this||proxy==in) return this; //TODO: paranoia

	DDRAW::IDirectDrawSurface *out;
	for(out=get_next;out&&out!=this;out=out->get_next)	
	if((void*)out->proxy==in) return out; return 0; //TODO: assert(out)
}
DDRAW::IDirectDrawSurface4*
DDRAW::IDirectDrawSurface4::get(void *in) //proxy pointer
{			
	if(!in||!this) return 0;

	if(target=='dx9c') //many possibilities
	{
		if(proxy==in||query9->group==in||query9->update==in) return this; //TODO: paranoia

		DDRAW::IDirectDrawSurface4 *out;
		for(out=get_next;out&&out!=this;out=out->get_next)
	
		if((void*)out->proxy==in||(void*)out->query9->group==in||(void*)out->query9->update==in)				
		return out; return 0; //TODO: assert(out)
	}
	else
	{
		if(proxy==in) return this; //TODO: paranoia

		DDRAW::IDirectDrawSurface4 *out;
		for(out=get_next;out&&out!=this;out=out->get_next)	
		if((void*)out->proxy==in) return out;
		return 0; //TODO: assert(out)
	}
}
DDRAW::IDirectDrawSurface7*
DDRAW::IDirectDrawSurface7::get(void *in) //proxy pointer
{			
	if(!in||!this) return 0;

	if(target=='dx9c') //many possibilities
	{
		if(proxy==in||query9->group==in||query9->update==in) return this; //TODO: paranoia

		DDRAW::IDirectDrawSurface7 *out;
		for(out=get_next;out&&out!=this;out=out->get_next)	
		if((void*)out->proxy==in||(void*)out->query9->group==in||(void*)out->query9->update==in)
		return out; return 0; //TODO: assert(out)
	}
	else
	{
		if(proxy==in) return this; //TODO: paranoia

		DDRAW::IDirectDrawSurface7 *out;
		for(out=get_next;out&&out!=this;out=out->get_next)	
		if((void*)out->proxy==in) return out; 
		return 0; //TODO: assert(out)
	}
}

static void dx_ddraw_unload()
{
	if(!DDRAW::IDirect3DTexture2::get_head) return;

	DDRAW::IDirect3DTexture2 *up = 
	DDRAW::IDirect3DTexture2::get_head, *stop = up;

	up->surface6->loaded = false;
	for(up=up->get_next;up!=stop;up=up->get_next)
	{
		up->surface6->loaded  = false;
	}
}

////////////////////////////////////////////////////////
//              DIRECTX7 INTERFACES                   //
////////////////////////////////////////////////////////
							

//OBSOLETE?
//what is this even for?
//NEW: returning final reference count
extern ULONG dx_ddraw_try_directdraw7_release()
{
	ULONG out = DDRAW::DirectDraw7->clientrefs;
	if(out==1)
	if(!DDRAW::DirectDraw7->query->dxf)
	if(!DDRAW::DirectDraw7->query->dx4)
	return DDRAW::DirectDraw7->Release(); return out;
}//OBSOLETE?
/*something is not right about these
//I'm having IDirectDraw7::Release release
//PrimarySurface, etc.
extern ULONG dx_ddraw_try_primarysurface7_release()
{
	//NOTE: I think maybe dx.ddraw.cpp may not AddRef
	//like dx.d3d9c.cpp does?
	ULONG out = DDRAW::PrimarySurface->clientrefs;
	if(out==1)
	if(!DDRAW::PrimarySurface->query->dxf)
	if(!DDRAW::PrimarySurface->query->dx4)
	return DDRAW::PrimarySurface->Release(); return out;
}*///OBSOLETE?
extern ULONG dx_ddraw_try_direct3ddevice7_release()
{
	ULONG out = DDRAW::Direct3DDevice7->clientrefs;
	if(out==1)
	if(!DDRAW::Direct3DDevice7->query->dx3)
	return DDRAW::Direct3DDevice7->Release(); return out;
}	

//sharing with dx.d3d9c.cpp 
extern void *dx_ddraw_hacks[DDRAW::TOTAL_HACKS] = {0}; 
static void *dx_ddraw_hacks_paranoia = memset(dx_ddraw_hacks,0x00,sizeof(dx_ddraw_hacks));
extern void *DDRAW::hack_interface(DDRAW::Hack h, void *f)
{	
	if(h<0||h>=DDRAW::TOTAL_HACKS) return 0;
	void *out = dx_ddraw_hacks[h]; dx_ddraw_hacks[h] = f; 
	return out;
}

#define DDRAW_PUSH_HACK(h,...)\
\
	HRESULT out = 1; void *hP,*hK;\
	for(hP=0,hK=dx_ddraw_hacks?dx_ddraw_hacks[DDRAW::h##_HACK]:0;hK;hK=0)hP=((void*(*)(HRESULT*,DDRAW::__VA_ARGS__))hK)

#define DDRAW_POP_HACK(h,...)\
\
	pophack: if(hP) ((void*(*)(HRESULT*,DDRAW::__VA_ARGS__))hP)

#define DDRAW_PUSH_HACK_IF(cond,h,...)\
\
	HRESULT out = 1;\
	void *hP=0,*hK=dx_ddraw_hacks?dx_ddraw_hacks[DDRAW::h##_HACK]:0;\
	if(cond) for(;hK;hK=0)hP=((void*(*)(HRESULT*,DDRAW::__VA_ARGS__))hK)


#define DDRAW_IF_NOT_TARGET_RETURN(...)\
	if(target!=DDRAW::target) return(assert(0),__VA_ARGS__);

#define DDRAW_IF__ANY__TARGET_RETURN(...)\
	return(assert(0),__VA_ARGS__);

#define DDRAW_IF__COMPAT__TARGET_RETURN(...)\
	if(DDRAW::compat||target!=DDRAW::target) return(assert(0),__VA_ARGS__);

#define DDRAW_IF_NOT_DX7A_RETURN(...)\
	if(DX::debug&&target!='dx7a') return(assert(0),__VA_ARGS__);

#define DDRAW_IF_NOT__DX6_RETURN(...)\
	if(DX::debug&&target!='_dx6') return(assert(0),__VA_ARGS__);

#define DDRAW_ADDREF DDRAW::referee(this,+1);

#define DDRAW_ADDREF_RETURN(zero) \
	if(!this) return zero; DDRAW_ADDREF return clientrefs;


#define DDRAW_IF_NOT_TARGET_AND_RELEASE(snapshot,...) \
	DDRAW_IF_NOT_TARGET_RETURN(__VA_ARGS__,0)\
	ULONG snapshot = DDRAW::referee(this,-1);


EXTERN_C const IID IID_IMediaStream; //mmstream.h
EXTERN_C const IID IID_IAMMediaStream; //amstream.h

HRESULT DDRAW::IDirectDraw::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw::QueryInterface()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)	

	if(7<=DDRAWLOG::debug)
	{
		LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';
	}

	if(riid==DX::IID_IDirectDraw) //(6C14DB80-A733-11CE-A521-0020AF0BE560)
	{
		DDRAW_LEVEL(7) << "(IDirectDraw)\n";		

		*ppvObj = this; AddRef(); return S_OK;
	}
	else if(riid==DX::IID_IDirectDraw4 //(9C59509A-39BD-11D1-8C4A-00C04FD930C5)
		  ||riid==DX::IID_IDirectDraw7) //(15E65EC0-3B9C-11D2-B92F-00609797EA5B)
	{	
		DDRAW_LEVEL(7) 
		<< " (IDirectDraw" 
		<< dx%(riid==DX::IID_IDirectDraw4?"4":"7") 
		<< ")\n";		

		if(riid==DX::IID_IDirectDraw4&&query->dx4)
		{
			query->dx4->AddRef(); *ppvObj = query->dx4; return S_OK;
		}
		else if(riid==DX::IID_IDirectDraw7&&query->dx7)
		{
			query->dx7->AddRef(); *ppvObj = query->dx7;	return S_OK;
		}

		void *q = 0; HRESULT out = 
		proxy->QueryInterface(riid,(LPVOID*)&q); 
		
		if(out!=S_OK)
		{
			DDRAW_LEVEL(7) << "QueryInterface FAILED\n";
			DDRAW_RETURN(out) 
		}

		if(riid==DX::IID_IDirectDraw4&&DDRAW::doIDirectDraw4)
		{	
			::IDirectDraw4 *dd4 = (::IDirectDraw4*)q;
			
			DDRAW::IDirectDraw4 *p = new DDRAW::IDirectDraw4(target,query);

			*ppvObj = query->dx4 = p; p->proxy7 = dd4;
		}
		else if(riid==DX::IID_IDirectDraw7&&DDRAW::doIDirectDraw7)
		{	
			::IDirectDraw7 *dd7 = (::IDirectDraw7*)q;
			
			DDRAW::IDirectDraw7 *p = new DDRAW::IDirectDraw7(target,query);

			*ppvObj = query->dx7 = p; p->proxy7 = dd7; 
		}
		else *ppvObj = q;

		DDRAW_RETURN(out) 
	}
/*	else if(riid==DX::IID_IMediaStream) //{B502D1BD-9A57-11D0-8FDE-00C04FD9189D}
	{
		DDRAW_LEVEL(7) << "(IMediaStream)\n";		

		IMediaStream *q = 0; 

		HRESULT out = proxy->QueryInterface(riid,(LPVOID FAR*)&q); 
		
		if(out!=S_OK){ DDRAW_LEVEL(7) << "QueryInterface FAILED\n"; DDRAW_RETURN(out)  }

		if(DSHOW::doIMediaStream)
		{	
			IMediaStream *ms = (::IMediaStream*)q;
			
			DSHOW::IMediaStream *p = new DSHOW::IMediaStream;

			p->proxy = ms; *ppvObj = p;
		}
		else *ppvObj = q;

		DDRAW_RETURN(out) 
	}
	else  if(riid==DX::IID_IAMMediaStream) //{BEBE595D-9A6F-11D0-8FDE-00C04FD9189D}
	{
		DDRAW_LEVEL(7) << "(IAMMediaStream)\n";		

		IAMMediaStream *q = 0; 

		HRESULT out = proxy->QueryInterface(riid,(LPVOID FAR*)&q); 
		
		if(out!=S_OK){ DDRAW_LEVEL(7) << "QueryInterface FAILED\n"; DDRAW_RETURN(out)  }

		if(DSHOW::doIAMMediaStream)
		{	
			::IAMMediaStream *amms = (::IAMMediaStream*)q;
			
			DSHOW::IAMMediaStream *p = new DSHOW::IAMMediaStream;

			p->proxy = amms; *ppvObj = p;
		}
		else *ppvObj = q;

		DDRAW_RETURN(out) 
	}
*/
	assert(riid==IID_IMediaStream||riid==IID_IAMMediaStream);

	return proxy->QueryInterface(riid,ppvObj);
}
ULONG DDRAW::IDirectDraw::AddRef() 
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw::AddRef()\n";

	DDRAW_ADDREF_RETURN(0)
}
ULONG DDRAW::IDirectDraw::Release()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw::Release()\n";

	DDRAW_IF_NOT_TARGET_AND_RELEASE(out,0)

	//REMOVE ME?
	if(out==1) return dx_ddraw_try_directdraw7_release();

	if(out==0) delete this; return out;
}
HRESULT DDRAW::IDirectDraw::Compact()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw::Compact()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	return proxy->Compact();
}
HRESULT DDRAW::IDirectDraw::CreateClipper(DWORD x, DX::LPDIRECTDRAWCLIPPER *y, IUnknown *z)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw::CreateClipper()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	return proxy->CreateClipper(x,y,z);
}
HRESULT DDRAW::IDirectDraw::CreatePalette(DWORD x, LPPALETTEENTRY y, DX::LPDIRECTDRAWPALETTE *z, IUnknown *w)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw::CreatePalette()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	return proxy->CreatePalette(x,y,z,w);
}
HRESULT DDRAW::IDirectDraw::CreateSurface(DX::LPDDSURFACEDESC x, DX::LPDIRECTDRAWSURFACE *y, IUnknown *z)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw::CreateSurface()\n";
	
	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
		
	DDRAW_PUSH_HACK(DIRECTDRAW_CREATESURFACE,IDirectDraw*,
	DX::LPDDSURFACEDESC&,DX::LPDIRECTDRAWSURFACE*&,IUnknown*&)(0,this,x,y,z);		
	
	if(!x||!y) DDRAW_FINISH(out)

	DX::LPDIRECTDRAWSURFACE q = 0; 

	out = proxy->CreateSurface(x,y?&q:0,z);

	if(out!=DD_OK){ DDRAW_LEVEL(7) << "CreateSurface FAILED\n"; DDRAW_FINISH(out) }

	if(DDRAW::doIDirectDrawSurface)
	{	
		*y = q; LPDIRECTDRAWSURFACE dds = *(LPDIRECTDRAWSURFACE*)y;
		
		DDRAW::IDirectDrawSurface *p = new DDRAW::IDirectDrawSurface(target);
					
		if(x->dwFlags&DDSD_CAPS&&x->ddsCaps.dwCaps&DDSCAPS_PRIMARYSURFACE)
		{
			DDRAW_LEVEL(1) << " Created primary surface\n";

			p->query->isPrimary = true; //important to flip/blit operations
		}

		p->proxy7 = dds; *y = p;
	}
	else *y = q; 

	DDRAW_POP_HACK(DIRECTDRAW_CREATESURFACE,IDirectDraw*,
	DX::LPDDSURFACEDESC&,DX::LPDIRECTDRAWSURFACE*&,IUnknown*&)(&out,this,x,y,z);

	if(!x||!y) return out; //hack: assuming short-circuited by hack

	DDRAW_RETURN(out)
}
HRESULT DDRAW::IDirectDraw::DuplicateSurface(DX::LPDIRECTDRAWSURFACE x, DX::LPDIRECTDRAWSURFACE *y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw::DuplicateSurface()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	auto p = DDRAW::is_proxy<IDirectDrawSurface>(x);
	
	if(p) x = p->proxy;

	DX::LPDIRECTDRAWSURFACE q = 0;

	HRESULT out = proxy->DuplicateSurface(x,&q);

	if(out!=DD_OK){ DDRAW_LEVEL(7) << "DuplicateSurface FAILED\n"; DDRAW_RETURN(out) }

	if(DDRAW::doIDirectDrawSurface)
	{
		*y = q; LPDIRECTDRAWSURFACE dds = *(LPDIRECTDRAWSURFACE*)y;
	
		DDRAW::IDirectDrawSurface *p = new DDRAW::IDirectDrawSurface(target);

		p->proxy7 = dds; *y = p;
	}
	else *y = q;

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDraw::EnumDisplayModes(DWORD x, DX::LPDDSURFACEDESC y, LPVOID z, DX::LPDDENUMMODESCALLBACK w)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw::EnumDisplayModes()\n";
	
	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	if(y) DDRAW_LEVEL(7) << "PANIC! LPDDSURFACEDESC was passed...\n";

	return proxy->EnumDisplayModes(x,y,z,w);
}
HRESULT DDRAW::IDirectDraw::EnumSurfaces(DWORD x, DX::LPDDSURFACEDESC y, LPVOID z, DX::LPDDENUMSURFACESCALLBACK w)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw::EnumSurfaces()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	if(w) DDRAW_LEVEL(7) << "PANIC! LPDDENUMSURFACESCALLBACK was passed...\n";

	return proxy->EnumSurfaces(x,y,z,w);
}
HRESULT DDRAW::IDirectDraw::FlipToGDISurface()   
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw::FlipToGDISurface()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	return proxy->FlipToGDISurface();
}
HRESULT DDRAW::IDirectDraw::GetCaps(DX::LPDDCAPS x, DX::LPDDCAPS y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw::GetCaps()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	return proxy->GetCaps(x,y);
}
HRESULT DDRAW::IDirectDraw::GetDisplayMode(DX::LPDDSURFACEDESC x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw::GetDisplayMode()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	return proxy->GetDisplayMode(x);
}

HRESULT DDRAW::IDirectDraw::GetFourCCCodes(LPDWORD x, LPDWORD y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw::GetFourCCCodes()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	return proxy->GetFourCCCodes(x,y);
}
HRESULT DDRAW::IDirectDraw::GetGDISurface(DX::LPDIRECTDRAWSURFACE *x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw::GetGDISurface()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	if(1) DDRAW_LEVEL(7) << "PANIC! GetGDISurface() was called...\n";

	return proxy->GetGDISurface(x);
}
HRESULT DDRAW::IDirectDraw::GetMonitorFrequency(LPDWORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw::GetMonitorFrequency()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	return proxy->GetMonitorFrequency(x);
}
HRESULT DDRAW::IDirectDraw::GetScanLine(LPDWORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw::GetScanLine()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	return proxy->GetScanLine(x);
}
HRESULT DDRAW::IDirectDraw::GetVerticalBlankStatus(LPBOOL x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw::GetVerticalBlankStatus()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	return proxy->GetVerticalBlankStatus(x);
}
HRESULT DDRAW::IDirectDraw::Initialize(GUID *x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw::Initialize()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	if(1) DDRAW_LEVEL(7) << "PANIC! Initialize() was called...\n";

	return proxy->Initialize(x);
}
HRESULT DDRAW::IDirectDraw::RestoreDisplayMode()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw::RestoreDisplayMode()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	DDRAW_PUSH_HACK(DIRECTDRAW_RESTOREDISPLAYMODE,IDirectDraw*)(0,this);

	//if(DDRAW::inWindow) return DD_OK; 
	
	//HRESULT out = proxy->RestoreDisplayMode();

	out = DDRAW::RestoreDisplayMode()?DD_OK:!DD_OK;

	DDRAW_POP_HACK(DIRECTDRAW_RESTOREDISPLAYMODE,IDirectDraw*)(&out,this);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDraw::SetCooperativeLevel(HWND x, DWORD y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw::SetCooperativeLevel()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	DDRAW_LEVEL(7) << "HWND is " << int(x) << '\n';
	
	HRESULT out = !DD_OK;

	if(!DDRAW::fullscreen) //if(DDRAW::inWindow)
	y = DDSCL_NORMAL;

	y|=DDSCL_NOWINDOWCHANGES|DDSCL_ALLOWREBOOT;

	out = proxy->SetCooperativeLevel(x,y);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDraw::SetDisplayMode(DWORD x, DWORD y, DWORD z)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw::SetDisplayMode()\n";
 
	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	//if(DDRAW::inWindow) return DD_OK; 
	
	//DDRAW_LEVEL(7) << ' ' << x << 'x' << y << '\n';
		
	DDRAW_PUSH_HACK(DIRECTDRAW_SETDISPLAYMODE,IDirectDraw*,
	DWORD&,DWORD&,DWORD&)(0,this,x,y,z);
	
	//HRESULT out = proxy->SetDisplayMode(x,y,z);

	out = DDRAW::SetDisplayMode(x,y,z,0,0)?DD_OK:!DD_OK;

	DDRAW_POP_HACK(DIRECTDRAW_SETDISPLAYMODE,IDirectDraw*,
	DWORD&,DWORD&,DWORD&)(&out,this,x,y,z);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDraw::WaitForVerticalBlank(DWORD x, HANDLE y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw::WaitForVerticalBlank()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	return proxy->WaitForVerticalBlank(x,y);
}






HRESULT DDRAW::IDirectDraw4::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw4::QueryInterface()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	if(7<=DDRAWLOG::debug)
	{
		LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';
	}

	if(riid==DX::IID_IDirectDraw4) //(9C59509A-39BD-11D1-8C4A-00C04FD930C5)
	{
		DDRAW_LEVEL(7) << "(IDirectDraw4)\n";		

		AddRef(); return S_OK;
	}
	else if(riid==DX::IID_IDirectDraw //(6C14DB80-A733-11CE-A521-0020AF0BE560)
		  ||riid==DX::IID_IDirectDraw7) //(15E65EC0-3B9C-11D2-B92F-00609797EA5B)
	{	
		DDRAW_LEVEL(7) 
		<< " (IDirectDraw" 
		<< dx%(riid==DX::IID_IDirectDraw7?"7":"") 
		<< ")\n";		

		if(riid==DX::IID_IDirectDraw&&query->dxf)
		{
			query->dxf->AddRef(); *ppvObj = query->dxf;	return S_OK;
		}
		else if(riid==DX::IID_IDirectDraw7&&query->dx7)
		{
			query->dx7->AddRef(); *ppvObj = query->dx7;	return S_OK;
		}

		void *q = 0; HRESULT out = 
		proxy->QueryInterface(riid,(LPVOID*)&q); 
		
		if(out!=S_OK)
		{
			DDRAW_LEVEL(7) << "QueryInterface FAILED\n"; 
			DDRAW_RETURN(out) 
		}

		if(riid==DX::IID_IDirectDraw&&DDRAW::doIDirectDraw)
		{	
			::IDirectDraw *dd = (::IDirectDraw*)q;
			
			DDRAW::IDirectDraw *p = new DDRAW::IDirectDraw(target,query);

			*ppvObj = query->dxf = p; p->proxy7 = dd; 
		}
		else if(riid==DX::IID_IDirectDraw7&&DDRAW::doIDirectDraw7)
		{	
			::IDirectDraw7 *dd7 = (::IDirectDraw7*)q;
			
			DDRAW::IDirectDraw7 *p = new DDRAW::IDirectDraw7('dx7a',query);

			*ppvObj = query->dx7 = p; p->proxy7 = dd7;
		}
		else *ppvObj = q;

		DDRAW_RETURN(out) 
	}
	else if(riid==DX::IID_IDirect3D3 //(BB223240-E72B-11D0-A9B4-00AA00C0993E)
		  ||riid==DX::IID_IDirect3D7) //(F5049E77-4861-11D2-A407-00A0C90629A8)
	{	
		DDRAW_LEVEL(7) 
		<< " (IDirect3D" 
		<< dx%(riid==DX::IID_IDirect3D3?"3":"7") 
		<< ")\n";		

		if(query->d3d)
		if(riid==DX::IID_IDirect3D3&&query->d3d->dx3)
		{
			query->d3d->dx3->AddRef(); *ppvObj = query->d3d->dx3; return S_OK;
		}
		else if(riid==DX::IID_IDirect3D7&&query->d3d->dx7)
		{
			query->d3d->dx7->AddRef(); *ppvObj = query->d3d->dx7; return S_OK;
		}

		if(target=='_dx6')
		{
			assert(riid==IID_IDirect3D3);

			::IDirect3D3 *q = 0; HRESULT out = E_NOINTERFACE;
				
			if(riid==IID_IDirect3D3)
			out = proxy->QueryInterface(riid,(LPVOID*)&q); 

			if(out==S_OK&&DDRAW::doIDirect3D3)
			{
				DDRAW::IDirect3D3 *p = new DDRAW::IDirect3D3('_dx6',query->d3d);

				query->d3d = p->query; p->query->ddraw = query;

				p->proxy6 = q; *ppvObj = p;
			}
			else *ppvObj = q;

			DDRAW_RETURN(out)
		}

		::IDirect3D7 *q = 0;

		REFIID qiid = DDRAW::doIDirect3D7?DX::IID_IDirect3D7:riid;

		HRESULT out = proxy->QueryInterface(qiid,(LPVOID*)&q); 
		
		if(out!=S_OK)
		{
			DDRAW_LEVEL(7) << "QueryInterface FAILED\n"; 
			DDRAW_RETURN(out) 
		}

		if(riid==DX::IID_IDirect3D3&&DDRAW::doIDirect3D3)
		{	
			DDRAW::IDirect3D3 *p = new DDRAW::IDirect3D3('dx7a',query->d3d);

			query->d3d = p->query; p->query->ddraw = query;

			p->proxy7 = q; *ppvObj = p;
		}
		else if(riid==DX::IID_IDirect3D7&&DDRAW::doIDirect3D7)
		{
			DDRAW::IDirect3D7 *p = new DDRAW::IDirect3D7('dx7a',query->d3d);

			query->d3d = p->query; p->query->ddraw = query;

			p->proxy7 = q; *ppvObj = p;
		}
		else *ppvObj = q;

		DDRAW_RETURN(out)
	}

	return proxy->QueryInterface(riid,ppvObj);
}
ULONG DDRAW::IDirectDraw4::AddRef()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw4::AddRef()\n";

	DDRAW_ADDREF_RETURN(0)
}
ULONG DDRAW::IDirectDraw4::Release()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw4::Release()\n";

	DDRAW_IF_NOT_TARGET_AND_RELEASE(out,0)

	//REMOVE ME?
	if(out==1) return dx_ddraw_try_directdraw7_release();

	if(out==0) delete this; 	

	return out;
}	
HRESULT DDRAW::IDirectDraw4::Compact()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw4::Compact()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	return proxy->Compact();
}
HRESULT DDRAW::IDirectDraw4::CreateClipper(DWORD x, DX::LPDIRECTDRAWCLIPPER *y, IUnknown *z)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw4::CreateClipper()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	return proxy->CreateClipper(x,y,z);
}
HRESULT DDRAW::IDirectDraw4::CreatePalette(DWORD x, LPPALETTEENTRY y, DX::LPDIRECTDRAWPALETTE *z, IUnknown *w)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw4::CreatePalette()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	return proxy->CreatePalette(x,y,z,w);
}
HRESULT DDRAW::IDirectDraw4::CreateSurface(DX::LPDDSURFACEDESC2 x, DX::LPDIRECTDRAWSURFACE4 *y, IUnknown *z)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw4::CreateSurface()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)
		
	DDRAW_PUSH_HACK(DIRECTDRAW4_CREATESURFACE,IDirectDraw4*,
	DX::LPDDSURFACEDESC2&,DX::LPDIRECTDRAWSURFACE4*&,IUnknown*&)(0,this,x,y,z);

	if(!x||!y) DDRAW_FINISH(out)

	if(DDRAWLOG::debug>=8) 
	{
#ifdef _DEBUG
#define OUT(X) if(x->ddsCaps.dwCaps&DDSCAPS_##X) DDRAW_LEVEL(8) << ' ' << #X << '\n';
						   
	OUT(PRIMARYSURFACE)
	OUT(FRONTBUFFER)
	OUT(BACKBUFFER)
	OUT(ZBUFFER)
	OUT(ALPHA)
	OUT(FLIP)
	OUT(OFFSCREENPLAIN)
	OUT(PALETTE)
	OUT(SYSTEMMEMORY)
	OUT(TEXTURE)
	OUT(3DDEVICE)
	OUT(VIDEOMEMORY)
	OUT(VISIBLE)
	OUT(WRITEONLY)
	OUT(OWNDC)
	OUT(LIVEVIDEO)
	OUT(HWCODEC)
	OUT(MIPMAP)
	OUT(ALLOCONLOAD)
	OUT(VIDEOPORT)
	OUT(LOCALVIDMEM)
	OUT(NONLOCALVIDMEM)
	OUT(STANDARDVGAMODE)
	OUT(OPTIMIZED)

#undef OUT
#endif
	}  

	DDRAW_LEVEL(7) << x->dwWidth << " x " << x->dwHeight << "\n";

	assert(x->dwSize==sizeof(DX::DDSURFACEDESC2)); //paranoia
				  
	DX::LPDIRECTDRAWSURFACE4 q = 0;

	//2018 Reminder: Primary surfaces seem required to have 0,0 
	//dimensions? Except for software rendering; or at least it
	//is so for all of SOM's programs.
	if(target!='_dx6')
	if((x->dwFlags&(DDSD_HEIGHT|DDSD_WIDTH)))
	{	
		if(x->dwFlags&DDSD_CAPS //software backbuffer?
		&&x->ddsCaps.dwCaps&(DDSCAPS_ZBUFFER|DDSCAPS_3DDEVICE)) 
		{
			RECT client = {0,0,x->dwWidth,x->dwHeight};	
			DDRAW::window_rect(&client);			

			if(DDRAW::BackBufferSurface7)
			{
				if((x->ddsCaps.dwCaps&DDSCAPS_ZBUFFER)==0)
				{	
					HRESULT out = //software backbuffer behavior
					DDRAW::BackBufferSurface7->QueryInterface(DX::IID_IDirectDrawSurface4,(LPVOID*)y);
					DDRAW_FINISH(out)
				}

				DDSURFACEDESC2 desc = { sizeof(DDSURFACEDESC2)};

				if(DDRAW::BackBufferSurface7->proxy7->GetSurfaceDesc(&desc)==DD_OK)
				{
					if(desc.dwFlags&(DDSD_HEIGHT|DDSD_WIDTH))
					{
						client.right = desc.dwWidth; client.bottom = desc.dwHeight;
					}
				}
			}
			
	 		x->dwWidth = client.right; x->dwHeight = client.bottom;
		}
	}		

	if(x->ddsCaps.dwCaps&DDSCAPS_FLIP)
	{
		//Set/RestoreDisplayMode are being bypassed so even
		//in fullscreen mode a flip chain is not compatible

		memset(&x->ddpfPixelFormat,0x00,sizeof(DDPIXELFORMAT));
		
		assert(x->ddsCaps.dwCaps&DDSCAPS_FRONTBUFFER); //2021

		x->ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE|DDSCAPS_3DDEVICE;

		//x->ddsCaps.dwCaps&=~DDSCAPS_FLIP;

				//UNTESTED
		//REMINDER: seemed to work fine without
		//docs say this accompanies DDXCAPS_FLIP
		x->ddsCaps.dwCaps|=DDSCAPS_FRONTBUFFER; //2021

		x->dwBackBufferCount = 0;

		x->dwFlags = DDSD_CAPS;
	}

	//TESTING DPI EFFECTS
	if(dx_DPI_testing_2021)	
	if(x->dwFlags&DDSD_CAPS)
	if(x->ddsCaps.dwCaps&(DDSCAPS_BACKBUFFER|DDSCAPS_ZBUFFER) 
	||x->ddsCaps.dwCaps==0x6040) //??? //SOM_MAP
	{
		x->dwWidth*=1.5f; x->dwHeight*=1.5f;

		DDRAW::xyScaling[0] = DDRAW::xyScaling[1] = 1.5f;
	}

	out = proxy->CreateSurface(x,(DX::LPDIRECTDRAWSURFACE4*)&q,z);

	if(out!=DD_OK){ DDRAW_LEVEL(7) << "CreateSurface FAILED\n"; DDRAW_FINISH(out) }

	if(DDRAW::doIDirectDrawSurface4)
	{					
		DDRAW::IDirectDrawSurface4 *p = new DDRAW::IDirectDrawSurface4(target);
					
		if(x->dwFlags&DDSD_CAPS&&x->ddsCaps.dwCaps&DDSCAPS_PRIMARYSURFACE) 
		{
			DDRAW_LEVEL(1) << " Created primary surface\n";

			p->query->isPrimary = true; 
		}

		p->proxy = q; *y = p;
				
		if(target!='_dx6')
		if(p->query->isPrimary)
		{			
			assert(!DDRAW::PrimarySurface7); 
			
			HRESULT paranoia = //in theory this should work out for the best
			p->QueryInterface(DX::IID_IDirectDrawSurface7,(LPVOID*)&DDRAW::PrimarySurface7);

			assert(paranoia==S_OK);

			if(!DDRAW::fullscreen) //if(DDRAW::inWindow)
			{
				::LPDIRECTDRAWCLIPPER ddc;   

				if(DDRAW::DirectDraw7->proxy7->CreateClipper(0,&ddc,0)==D3D_OK)
				{						
					ddc->SetHWnd(0,DDRAW::window); 

					DDRAW::PrimarySurface7->proxy7->SetClipper(ddc);   

					ddc->Release();
				}
			}	     

			RECT client = {0,0,640,480};
			DDRAW::window_rect(&client);

			::DDSURFACEDESC2 desc = {sizeof(::DDSURFACEDESC2)};
		   
			desc.dwFlags = DDSD_WIDTH|DDSD_HEIGHT|DDSD_CAPS;   
			desc.dwWidth = client.right; desc.dwHeight = client.bottom;
			desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN|DDSCAPS_3DDEVICE; 
		   
			switch(DDRAW::bitsperpixel)
			{
			default: desc.ddpfPixelFormat = dx_ddraw_X8R8G8B8; break;
			case 16: desc.ddpfPixelFormat = dx_ddraw_R5G6B5;   break;
			}
			
			::LPDIRECTDRAWSURFACE7 dds7 = 0;

			assert(!DDRAW::BackBufferSurface7); //assuming created after primary
			if(DDRAW::DirectDraw7->proxy7->CreateSurface(&desc,&dds7,0)==D3D_OK)
			{
				DDRAW::BackBufferSurface7 = new DDRAW::IDirectDrawSurface7('dx7a');
				DDRAW::BackBufferSurface7->proxy7 = dds7;
			}
		}
	}
	else *y = q; 
	
	DDRAW_POP_HACK(DIRECTDRAW4_CREATESURFACE,IDirectDraw4*,
	DX::LPDDSURFACEDESC2&,DX::LPDIRECTDRAWSURFACE4*&,IUnknown*&)(&out,this,x,y,z);

	//hack: SOM_MAP.exe wants a 0 width texture up top??
	if(out==E_INVALIDARG) return E_INVALIDARG; 

	if(!x||!y) return out; //hack: assuming short-circuited by hack

	DDRAW_RETURN(out)
}
HRESULT DDRAW::IDirectDraw4::DuplicateSurface(DX::LPDIRECTDRAWSURFACE4 x, DX::LPDIRECTDRAWSURFACE4 *y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw4::DuplicateSurface()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	auto p = DDRAW::is_proxy<IDirectDrawSurface4>(x);
	
	if(p) x = p->proxy;
						 
	DX::LPDIRECTDRAWSURFACE4 q = 0; 

	HRESULT out = proxy->DuplicateSurface(x,&q);

	if(out!=DD_OK){ DDRAW_LEVEL(7) << "DuplicateSurface FAILED\n"; DDRAW_RETURN(out) }

	if(DDRAW::doIDirectDrawSurface4)
	{
		*y = q; ::LPDIRECTDRAWSURFACE4 dds7 = *(::LPDIRECTDRAWSURFACE4*)y;
		
		DDRAW::IDirectDrawSurface4 *p = new DDRAW::IDirectDrawSurface4(target);

		p->proxy7 = dds7; *y = p;
	}
	else *y = q; 

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDraw4::EnumDisplayModes(DWORD x, DX::LPDDSURFACEDESC2 y, LPVOID z, DX::LPDDENUMMODESCALLBACK2 w)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw4::EnumDisplayModes()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)
	
	if(w) DDRAW_LEVEL(7) << "PANIC! LPDDENUMMODESCALLBACK2 was passed...\n";
		
	void *passthru = dx_ddraw_reserve_enum_set(w,z);

	//Note: filtering display modes so 16bpp comes first then 32bpp and nothing else

	HRESULT out = proxy->EnumDisplayModes(x,y,passthru,dx_ddraw_enumdisplaymodes16);

	out&=proxy->EnumDisplayModes(x,y,passthru,dx_ddraw_enumdisplaymodes32);

	dx_ddraw_return_enum_set(passthru); DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDraw4::EnumSurfaces(DWORD x, DX::LPDDSURFACEDESC2 y, LPVOID z, DX::LPDDENUMSURFACESCALLBACK2 w)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw4::EnumSurfaces()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	if(w) DDRAW_LEVEL(7) << "PANIC! LPDDENUMSURFACESCALLBACK7 was passed...\n";

	return proxy->EnumSurfaces(x,y,z,w);
}
HRESULT DDRAW::IDirectDraw4::FlipToGDISurface()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw4::FlipToGDISurface()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	return proxy->FlipToGDISurface();
}
HRESULT DDRAW::IDirectDraw4::GetCaps(DX::LPDDCAPS x, DX::LPDDCAPS y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw4::GetCaps()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	HRESULT out = proxy->GetCaps(x,y);

	DDRAW::log_driver_caps(x,y);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDraw4::GetDisplayMode(DX::LPDDSURFACEDESC2 x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw4::GetDisplayMode()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	return proxy->GetDisplayMode(x);
}
HRESULT DDRAW::IDirectDraw4::GetFourCCCodes(LPDWORD x, LPDWORD y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw4::GetFourCCCodes()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	return proxy->GetFourCCCodes(x,y);
}
HRESULT DDRAW::IDirectDraw4::GetGDISurface(DX::LPDIRECTDRAWSURFACE4 *x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw4::GetGDISurface()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	if(1) DDRAW_LEVEL(7) << "PANIC! GetGDISurface() was called...\n";

	return proxy->GetGDISurface(x);
}
HRESULT DDRAW::IDirectDraw4::GetMonitorFrequency(LPDWORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw4::GetMonitorFrequency()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	return proxy->GetMonitorFrequency(x);
}
HRESULT DDRAW::IDirectDraw4::GetScanLine(LPDWORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw4::GetScanLine()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	return proxy->GetScanLine(x);
}
HRESULT DDRAW::IDirectDraw4::GetVerticalBlankStatus(LPBOOL x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw4::GetVerticalBlankStatus()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	return proxy->GetVerticalBlankStatus(x);
}
HRESULT DDRAW::IDirectDraw4::Initialize(GUID *x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw4::Initialize()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	if(1) DDRAW_LEVEL(7) << "PANIC! Initialize() was called...\n";

	return proxy->Initialize(x);
}
HRESULT DDRAW::IDirectDraw4::RestoreDisplayMode()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw4::RestoreDisplayMode()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)
	
	DDRAW_PUSH_HACK(DIRECTDRAW4_RESTOREDISPLAYMODE,IDirectDraw4*)(0,this);
							
	out = DDRAW::RestoreDisplayMode()?DD_OK:!DD_OK;

	DDRAW_POP_HACK(DIRECTDRAW4_RESTOREDISPLAYMODE,IDirectDraw4*)(&out,this);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDraw4::SetCooperativeLevel(HWND x, DWORD y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw4::SetCooperativeLevel()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	DDRAW_PUSH_HACK(DIRECTDRAW4_SETCOOPERATIVELEVEL,IDirectDraw4*,
	HWND&,DWORD&)(0,this,x,y);

	DDRAW_LEVEL(7) << "HWND is " << int(x) << '\n';
	
	DDRAW::window = x;  //capture window
			
	////Why were their 3 exactly?
	//if(DDRAW::inWindow||!DDRAW::isExclusive) 
	if(!DDRAW::fullscreen)
	y = DDSCL_NORMAL;

	y|=DDSCL_NOWINDOWCHANGES|DDSCL_ALLOWREBOOT;

	out = proxy->SetCooperativeLevel(x,y);

	DDRAW_POP_HACK(DIRECTDRAW4_SETCOOPERATIVELEVEL,IDirectDraw4*,
	HWND&,DWORD&)(&out,this,x,y);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDraw4::SetDisplayMode(DWORD x, DWORD y, DWORD z, DWORD w, DWORD q)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw4::SetDisplayMode()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)
	
	//if(DDRAW::inWindow) return DD_OK; 
	
	//DDRAW_LEVEL(7) << ' ' << x << 'x' << y << '\n';
		
	DDRAW_PUSH_HACK(DIRECTDRAW4_SETDISPLAYMODE,IDirectDraw4*,
	DWORD&,DWORD&,DWORD&,DWORD&,DWORD&)(0,this,x,y,z,w,q);
	
	out = DDRAW::SetDisplayMode(x,y,z,w,q)?DD_OK:!DD_OK;

	DDRAW_POP_HACK(DIRECTDRAW4_SETDISPLAYMODE,IDirectDraw4*,
	DWORD&,DWORD&,DWORD&,DWORD&,DWORD&)(&out,this,x,y,z,w,q);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDraw4::WaitForVerticalBlank(DWORD x, HANDLE y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw4::WaitForVerticalBlank()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	return proxy->WaitForVerticalBlank(x,y);
}
HRESULT DDRAW::IDirectDraw4::GetAvailableVidMem(DX::LPDDSCAPS2 x, LPDWORD y, LPDWORD z)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw4::GetAvailableVidMem()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	return proxy->GetAvailableVidMem(x,y,z);
}
HRESULT DDRAW::IDirectDraw4::GetSurfaceFromDC(HDC x, DX::LPDIRECTDRAWSURFACE4 *y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw4::GetSurfaceFromDC()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	if(1) DDRAW_LEVEL(7) << "PANIC! GetSurfaceFromDC() was called...\n";

	return proxy->GetSurfaceFromDC(x,y);
}
HRESULT DDRAW::IDirectDraw4::RestoreAllSurfaces()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw4::RestoreAllSurfaces()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	DDRAW_RETURN(proxy->RestoreAllSurfaces())
}
HRESULT DDRAW::IDirectDraw4::TestCooperativeLevel()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw4::TestCooperativeLevel()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	return proxy->TestCooperativeLevel();
}
HRESULT DDRAW::IDirectDraw4::GetDeviceIdentifier(DX::LPDDDEVICEIDENTIFIER x, DWORD y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw4::GetDeviceIdentifier()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	if(1) DDRAW_LEVEL(7) << "PANIC! GetDeviceIdentifier() was called...\n";

	return proxy->GetDeviceIdentifier(x,y);
}






HRESULT DDRAW::IDirectDraw7::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw7::QueryInterface()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	if(7<=DDRAWLOG::debug)
	{
		LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';
	}

	if(riid==DX::IID_IDirectDraw7) //(15E65EC0-3B9C-11D2-B92F-00609797EA5B)
	{
		DDRAW_LEVEL(7) << "(IDirectDraw7)\n";		

		AddRef(); return S_OK;
	}
	else if(riid==DX::IID_IDirectDraw //(6C14DB80-A733-11CE-A521-0020AF0BE560)
		  ||riid==DX::IID_IDirectDraw4) //(9C59509A-39BD-11D1-8C4A-00C04FD930C5)
	{	
		DDRAW_LEVEL(7) 
		<< " (IDirectDraw" 
		<< dx%(riid==DX::IID_IDirectDraw4?"4":"") 
		<< ")\n";		

		if(riid==DX::IID_IDirectDraw&&query->dxf)
		{
			query->dxf->AddRef(); *ppvObj = query->dxf;	return S_OK;
		}
		else if(riid==DX::IID_IDirectDraw4&&query->dx4)
		{
			query->dx4->AddRef(); *ppvObj = query->dx4;	return S_OK;
		}

		void *q = 0; HRESULT out = 
		proxy->QueryInterface(riid,(LPVOID*)&q); 
		
		if(out!=S_OK)
		{
			DDRAW_LEVEL(7) << "QueryInterface FAILED\n";
			DDRAW_RETURN(out) 
		}

		if(riid==DX::IID_IDirectDraw&&DDRAW::doIDirectDraw)
		{	
			::IDirectDraw *dd = (::IDirectDraw*)q;
			
			DDRAW::IDirectDraw *p = new DDRAW::IDirectDraw('dx7a',query);

			*ppvObj = query->dxf = p; p->proxy7 = dd; 
		}
		else if(riid==DX::IID_IDirectDraw4&&DDRAW::doIDirectDraw4)
		{	
			::IDirectDraw4 *dd4 = (::IDirectDraw4*)q;
			
			DDRAW::IDirectDraw4 *p = new DDRAW::IDirectDraw4('dx7a',query);

			*ppvObj = query->dx4 = p; p->proxy7 = dd4;
		}
		else *ppvObj = q;

		DDRAW_RETURN(out) 
	}
	else if(riid==DX::IID_IDirect3D3 //(BB223240-E72B-11D0-A9B4-00AA00C0993E)
		  ||riid==DX::IID_IDirect3D7) //(F5049E77-4861-11D2-A407-00A0C90629A8)
	{	
		DDRAW_LEVEL(7) 
		<< " (IDirect3D" 
		<< dx%(riid==DX::IID_IDirect3D3?"3":"7") 
		<< ")\n";		

		if(query->d3d)
		if(riid==DX::IID_IDirect3D3&&query->d3d->dx3)
		{
			query->d3d->dx3->AddRef(); *ppvObj = query->d3d->dx3; return S_OK;
		}
		else if(riid==DX::IID_IDirect3D7&&query->d3d->dx7)
		{
			query->d3d->dx7->AddRef(); *ppvObj = query->d3d->dx7; return S_OK;
		}

		::IDirect3D7 *q = 0;

		REFIID qiid = DDRAW::doIDirect3D7?DX::IID_IDirect3D7:riid;

		HRESULT out = proxy->QueryInterface(qiid,(LPVOID*)&q); 
		
		if(out!=S_OK)
		{
			DDRAW_LEVEL(7) << "QueryInterface FAILED\n"; 
			DDRAW_RETURN(out) 
		}

		if(riid==DX::IID_IDirect3D3&&DDRAW::doIDirect3D3)
		{	
			DDRAW::IDirect3D3 *p = new DDRAW::IDirect3D3('dx7a',query->d3d);

			query->d3d = p->query; p->query->ddraw = query;

			p->proxy7 = q; *ppvObj = p;
		}
		else if(riid==DX::IID_IDirect3D7&&DDRAW::doIDirect3D7)
		{
			DDRAW::IDirect3D7 *p = new DDRAW::IDirect3D7('dx7a',query->d3d);

			query->d3d = p->query; p->query->ddraw = query;

			p->proxy7 = q; *ppvObj = p;
		}
		else *ppvObj = q;

		DDRAW_RETURN(out) 
	}

	HRESULT out = proxy->QueryInterface(riid,ppvObj);
	DDRAW_RETURN(out)
}
ULONG DDRAW::IDirectDraw7::AddRef()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw7::AddRef()\n";

	DDRAW_ADDREF_RETURN(0)
}
ULONG DDRAW::IDirectDraw7::Release()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw7::Release()\n";

	DDRAW_IF_NOT_TARGET_AND_RELEASE(out,0)
	
	//REMOVE ME?
	//no it still seems to be in play (2021) something to
	//do with IDirectDraw and dx_ddraw_DirectDrawCreateEx
	//has a thing where if a second IDirectDraw is sought
	//it's not a proxy!
	if(out==1) return dx_ddraw_try_directdraw7_release();

	if(out==0)
	{
		if(DDRAW::DirectDraw7==this)
		{
			int i; //NOTE: if assert fails technically it can be a client releases out-of-order
			for(i=0;DDRAW::PrimarySurface7;){ assert(!i++); DDRAW::PrimarySurface7->Release(); }
			for(i=0;DDRAW::BackBufferSurface7;){ assert(!i++); DDRAW::BackBufferSurface7->Release(); }
			for(i=0;DDRAW::DepthStencilSurface7;){ assert(!i++); DDRAW::DepthStencilSurface7->Release(); }

			DDRAW::DirectDraw7 = 0;			
		}
		else assert(0);

		delete this; 
	}
	return out;
}	
HRESULT DDRAW::IDirectDraw7::Compact()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw7::Compact()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->Compact();
}
HRESULT DDRAW::IDirectDraw7::CreateClipper(DWORD x, DX::LPDIRECTDRAWCLIPPER *y, IUnknown *z)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw7::CreateClipper()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	return proxy->CreateClipper(x,y,z);
}
HRESULT DDRAW::IDirectDraw7::CreatePalette(DWORD x, LPPALETTEENTRY y, DX::LPDIRECTDRAWPALETTE *z, IUnknown *w)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw7::CreatePalette()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->CreatePalette(x,y,z,w);
}
HRESULT DDRAW::IDirectDraw7::CreateSurface(DX::LPDDSURFACEDESC2 x, DX::LPDIRECTDRAWSURFACE7 *y, IUnknown *z)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw7::CreateSurface()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)  
		
	DDRAW_PUSH_HACK(DIRECTDRAW7_CREATESURFACE,IDirectDraw7*,
	DX::LPDDSURFACEDESC2&,DX::LPDIRECTDRAWSURFACE7*&,IUnknown*&)(0,this,x,y,z);
		
	if(!x||!y) DDRAW_FINISH(out)

	if(DDRAWLOG::debug>=8) 
	{
#ifdef _DEBUG
#define OUT(X) if(x->ddsCaps.dwCaps&DDSCAPS_##X) DDRAW_LEVEL(8) << ' ' << #X << '\n';
						   
	OUT(PRIMARYSURFACE)
	OUT(FRONTBUFFER)
	OUT(BACKBUFFER)
	OUT(ZBUFFER)
	OUT(ALPHA)
	OUT(FLIP)
	OUT(OFFSCREENPLAIN)
	OUT(PALETTE)
	OUT(SYSTEMMEMORY)
	OUT(TEXTURE)
	OUT(3DDEVICE)
	OUT(VIDEOMEMORY)
	OUT(VISIBLE)
	OUT(WRITEONLY)
	OUT(OWNDC)
	OUT(LIVEVIDEO)
	OUT(HWCODEC)
	OUT(MIPMAP)
	OUT(ALLOCONLOAD)
	OUT(VIDEOPORT)
	OUT(LOCALVIDMEM)
	OUT(NONLOCALVIDMEM)
	OUT(STANDARDVGAMODE)
	OUT(OPTIMIZED)

#undef OUT
#endif
	} 
	
	assert(x->dwSize==sizeof(DX::DDSURFACEDESC2)); //paranoia
 
	DX::LPDIRECTDRAWSURFACE7 q = 0;

	//ItemEdit & co?
	//HACK: my Intel drivers seem to only allow 16/32-bit zbuffer
	//but it turns out these programs prefer the software device!
	//TODO: do this inside workshop_CreateSurface? Why software??
	bool z16 = false; 

	//2018 Reminder: Primary surfaces seem required to have 0,0 
	//dimensions? Except for software rendering; or at least it
	//is so for all of SOM's programs.
	if(x->dwFlags&(DDSD_HEIGHT|DDSD_WIDTH))
	{	
		DDRAW_LEVEL(7) << x->dwWidth << " x " << x->dwHeight << "\n";

		if(x->dwFlags&DDSD_CAPS //software backbuffer
		&&x->ddsCaps.dwCaps&(DDSCAPS_ZBUFFER|DDSCAPS_3DDEVICE)) 
		{
			RECT client = {0,0,x->dwWidth,x->dwHeight};
			DDRAW::window_rect(&client);			

			if(DDRAW::BackBufferSurface7)
			{
				if((x->ddsCaps.dwCaps&DDSCAPS_ZBUFFER)==0)
				{									  
					*y = DDRAW::BackBufferSurface7; //software backbuffer behavior

					DDRAW::BackBufferSurface7->AddRef();

					DDRAW_FINISH(DD_OK)					
				}
				else z16 = 16==x->ddpfPixelFormat.dwZBufferBitDepth;

				DDSURFACEDESC2 desc = {sizeof(DDSURFACEDESC2)};

				if(DDRAW::BackBufferSurface7->proxy7->GetSurfaceDesc(&desc)==DD_OK)
				{
					if(desc.dwFlags&(DDSD_HEIGHT|DDSD_WIDTH))
					{
						client.right = desc.dwWidth; client.bottom = desc.dwHeight;
					}
				}
			}
			
	 		x->dwWidth = client.right; x->dwHeight = client.bottom;
		}
	}		

	if(x->ddsCaps.dwCaps&DDSCAPS_FLIP)
	{
		//Set/RestoreDisplayMode are being bypassed so even
		//in fullscreen mode a flip chain is not compatible

		memset(&x->ddpfPixelFormat,0x00,sizeof(DDPIXELFORMAT));
												  
		x->ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE|DDSCAPS_3DDEVICE;

		//x->ddsCaps.dwCaps&=~DDSCAPS_FLIP;

		x->dwBackBufferCount = 0;

		x->dwFlags = DDSD_CAPS;
	}

	if(z16) //try to upgrade zbuffer?
	{
		//Reminder: dx.d3d9c.cpp does something similar in CreateDevice
		assert(0==x->ddpfPixelFormat.dwStencilBitDepth);		
		x->ddpfPixelFormat.dwZBufferBitDepth = 32;
		x->ddpfPixelFormat.dwZBitMask = 0xFFFFFFFF;
		if(out=proxy->CreateSurface(x,(DX::LPDIRECTDRAWSURFACE7*)&q,z))
		{
			x->ddpfPixelFormat.dwZBufferBitDepth = 24;
			x->ddpfPixelFormat.dwZBitMask = 0xFFFFFF;
			if(out=proxy->CreateSurface(x,(DX::LPDIRECTDRAWSURFACE7*)&q,z))			
			{
				x->ddpfPixelFormat.dwZBufferBitDepth = 16;
				x->ddpfPixelFormat.dwZBitMask = 0xFFFF;
			}
		}
		if(out==DD_OK) goto z16;
	}

	out = proxy->CreateSurface(x,(DX::LPDIRECTDRAWSURFACE7*)&q,z);
	if(out!=DD_OK)
	{
		DDRAW_LEVEL(7) << "CreateSurface FAILED\n"; 
		DDRAW_FINISH(out) 
	}

z16:if(DDRAW::doIDirectDrawSurface7)
	{	
		*y = q; ::LPDIRECTDRAWSURFACE7 dds7 = *(::LPDIRECTDRAWSURFACE7*)y;

		DDRAW::IDirectDrawSurface7 **pp = 0;
			
		if(x->dwFlags&DDSD_CAPS) //???
		if(x->ddsCaps.dwCaps&DDSCAPS_PRIMARYSURFACE)
		{	
			DDRAW_LEVEL(1) << " Created primary surface\n";

			//assert(x->ddsCaps.dwCaps&DDSCAPS_VIDEOMEMORY);

			pp = &DDRAW::PrimarySurface7;
		}
		else if(x->ddsCaps.dwCaps&DDSCAPS_ZBUFFER) //2021
		{
			//ASSUMING SINGLE ZBUFFER

			pp = &DDRAW::DepthStencilSurface7;
		}		
		else if(x->ddsCaps.dwCaps&DDSCAPS_BACKBUFFER) //2021
		{
			//I think only GetAttachedSurface can get the backbuffer
			//but just to cover all the bases
			assert(0);

			pp = &DDRAW::BackBufferSurface7;
		}
		
		auto p = pp&&*pp?*pp:new DDRAW::IDirectDrawSurface7('dx7a');

		if(pp) if(!*pp) //2021
		{
			*pp = p; 
			
			//IDirectDraw7::Release should release them
			{
				//HACK: just preventing AddRef() assert 
				p->proxy7 = dds7;
				p->AddRef();
			}

			p->query->isPrimary = p==DDRAW::PrimarySurface7; //important to flip/blit operations
		}
		else //YUCK
		{
			if(p->proxy7) p->proxy7->Release(); //...
		}
		p->proxy7 = dds7; *y = p;

		if(p->query->isPrimary)
		{
			if(!DDRAW::fullscreen) //if(DDRAW::inWindow)
			{
				LPDIRECTDRAWCLIPPER ddc;   

				if(proxy7->CreateClipper(0,&ddc,0)==D3D_OK)
				{						
					ddc->SetHWnd(0,DDRAW::window); 

					p->proxy7->SetClipper(ddc);   

					ddc->Release();
				}
			}	     

		//WARNING //WARNING //WARNING //WARNING //WARNING 
			
		//there is a bug here when programs call setViewport with
		//a larger value than the backbuffer. I can't see a way to
		//compensate. usually the difference is just the non-client
		//border thickness. Maybe the trick is to leave the size not
		//specified?

			RECT client = {0,0,640,480};
			
			//I think Windows 10's invisible borders are throwing 
			//this off.
			//The primary dimensions are full screen, and don't seem
			//to be able to be set??? Should probably try again
			//GetClientRect(DDRAW::window,&client);
			DDRAW::window_rect(&client);
			::DDSURFACEDESC2 desc = {sizeof(::DDSURFACEDESC2)};
			//dds7->GetSurfaceDesc(&desc);
			desc.dwWidth = client.right; desc.dwHeight = client.bottom;
		   
			desc.dwFlags = DDSD_WIDTH|DDSD_HEIGHT|DDSD_CAPS;   			

			desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN|DDSCAPS_3DDEVICE;   
		   
			switch(DDRAW::bitsperpixel)
			{
			default: desc.ddpfPixelFormat = dx_ddraw_X8R8G8B8; break;
			case 16: desc.ddpfPixelFormat = dx_ddraw_R5G6B5;   break;
			}

			//warning! assuming created after primary
			if(proxy7->CreateSurface(&desc,&dds7,0)==D3D_OK)
			{
				assert(!DDRAW::BackBufferSurface7);

				DDRAW::BackBufferSurface7 = new DDRAW::IDirectDrawSurface7('dx7a');

				DDRAW::BackBufferSurface7->proxy7 = dds7;
			}
		}
	}
	else *y = q;	 

	DDRAW_POP_HACK(DIRECTDRAW7_CREATESURFACE,IDirectDraw7*,
	DX::LPDDSURFACEDESC2&,DX::LPDIRECTDRAWSURFACE7*&,IUnknown*&)(&out,this,x,y,z);

	if(!x||!y) return out; //hack: assuming short-circuited by hack

	DDRAW_RETURN(out)
}
HRESULT DDRAW::IDirectDraw7::DuplicateSurface(DX::LPDIRECTDRAWSURFACE7 x, DX::LPDIRECTDRAWSURFACE7 *y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw7::DuplicateSurface()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	auto p = DDRAW::is_proxy<IDirectDrawSurface7>(x);
	
	if(p) x = p->proxy;
						 
	DX::LPDIRECTDRAWSURFACE7 q = 0; 

	HRESULT out = proxy->DuplicateSurface(x,&q);

	if(out!=DD_OK)
	{
		DDRAW_LEVEL(7) << "DuplicateSurface FAILED\n"; 
		DDRAW_RETURN(out)  
	}

	if(DDRAW::doIDirectDrawSurface7)
	{
		*y = q; ::LPDIRECTDRAWSURFACE7 dds7 = *(::LPDIRECTDRAWSURFACE7*)y;
		
		DDRAW::IDirectDrawSurface7 *p = new DDRAW::IDirectDrawSurface7('dx7a');

		p->proxy7 = dds7; *y = p;
	}
	else *y = q; 

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDraw7::EnumDisplayModes(DWORD x, DX::LPDDSURFACEDESC2 y, LPVOID z, DX::LPDDENUMMODESCALLBACK2 w)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw7::EnumDisplayModes()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	if(w) DDRAW_LEVEL(7) << "PANIC! LPDDENUMMODESCALLBACK2 was passed...\n";
		
	void *passthru = dx_ddraw_reserve_enum_set(w,z);

	//Note: filtering display modes so 16bpp comes first then 32bpp and nothing else

	HRESULT out = proxy->EnumDisplayModes(x,y,passthru,dx_ddraw_enumdisplaymodes16);

	out&=proxy->EnumDisplayModes(x,y,passthru,dx_ddraw_enumdisplaymodes32);

	dx_ddraw_return_enum_set(passthru); DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDraw7::EnumSurfaces(DWORD x, DX::LPDDSURFACEDESC2 y, LPVOID z, DX::LPDDENUMSURFACESCALLBACK7 w)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw7::EnumSurfaces()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	if(w) DDRAW_LEVEL(7) << "PANIC! LPDDENUMSURFACESCALLBACK7 was passed...\n";

	return proxy->EnumSurfaces(x,y,z,w);
}
HRESULT DDRAW::IDirectDraw7::FlipToGDISurface()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw7::FlipToGDISurface()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->FlipToGDISurface();
}
HRESULT DDRAW::IDirectDraw7::GetCaps(DX::LPDDCAPS x, DX::LPDDCAPS y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw7::GetCaps()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	HRESULT out = proxy->GetCaps(x,y);

	DDRAW::log_driver_caps(x,y);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDraw7::GetDisplayMode(DX::LPDDSURFACEDESC2 x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw7::GetDisplayMode()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->GetDisplayMode(x);
}
HRESULT DDRAW::IDirectDraw7::GetFourCCCodes(LPDWORD x, LPDWORD y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw7::GetFourCCCodes()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->GetFourCCCodes(x,y);
}
HRESULT DDRAW::IDirectDraw7::GetGDISurface(DX::LPDIRECTDRAWSURFACE7 *x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw7::GetGDISurface()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	if(1) DDRAW_LEVEL(7) << "PANIC! GetGDISurface() was called...\n";

	return proxy->GetGDISurface(x);
}
HRESULT DDRAW::IDirectDraw7::GetMonitorFrequency(LPDWORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw7::GetMonitorFrequency()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->GetMonitorFrequency(x);
}
HRESULT DDRAW::IDirectDraw7::GetScanLine(LPDWORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw7::GetScanLine()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->GetScanLine(x);
}
HRESULT DDRAW::IDirectDraw7::GetVerticalBlankStatus(LPBOOL x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw7::GetVerticalBlankStatus()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->GetVerticalBlankStatus(x);
}
HRESULT DDRAW::IDirectDraw7::Initialize(GUID *x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw7::Initialize()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	if(1) DDRAW_LEVEL(7) << "PANIC! Initialize() was called...\n";

	return proxy->Initialize(x);
}
HRESULT DDRAW::IDirectDraw7::RestoreDisplayMode()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw7::RestoreDisplayMode()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	DDRAW_PUSH_HACK(DIRECTDRAW7_RESTOREDISPLAYMODE,IDirectDraw7*)(0,this);

	out = DDRAW::RestoreDisplayMode()?DD_OK:!DD_OK;

	DDRAW_POP_HACK(DIRECTDRAW7_RESTOREDISPLAYMODE,IDirectDraw7*)(&out,this);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDraw7::SetCooperativeLevel(HWND x, DWORD y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw7::SetCooperativeLevel()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	DDRAW_PUSH_HACK(DIRECTDRAW7_SETCOOPERATIVELEVEL,IDirectDraw7*,
	HWND&,DWORD&)(0,this,x,y);

	DDRAW_LEVEL(7) << "HWND is " << int(x) << '\n';
	
	DDRAW::window = x;  //capture window

	//Why were their 3 exactly?
	//if(DDRAW::inWindow||!DDRAW::isExclusive)
	if(!DDRAW::fullscreen) y = DDSCL_NORMAL;

	//TODO? remove DDSCL_FPUPRESERVE? (EneEdit.exe)

	y|=DDSCL_NOWINDOWCHANGES|DDSCL_ALLOWREBOOT;

	out = proxy->SetCooperativeLevel(x,y);

	DDRAW_POP_HACK(DIRECTDRAW7_SETCOOPERATIVELEVEL,IDirectDraw7*,
	HWND&,DWORD&)(&out,this,x,y);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDraw7::SetDisplayMode(DWORD x, DWORD y, DWORD z, DWORD w, DWORD q)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw7::SetDisplayMode()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	//if(DDRAW::inWindow) return DD_OK; 
	
	//DDRAW_LEVEL(7) << ' ' << x << 'x' << y << '\n';
		
	DDRAW_PUSH_HACK(DIRECTDRAW7_SETDISPLAYMODE,IDirectDraw7*,
	DWORD&,DWORD&,DWORD&,DWORD&,DWORD&)(0,this,x,y,z,w,q);
	
	out = DDRAW::SetDisplayMode(x,y,z,w,q)?DD_OK:!DD_OK;

	DDRAW_POP_HACK(DIRECTDRAW7_SETDISPLAYMODE,IDirectDraw7*,
	DWORD&,DWORD&,DWORD&,DWORD&,DWORD&)(&out,this,x,y,z,w,q);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDraw7::WaitForVerticalBlank(DWORD x, HANDLE y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw7::WaitForVerticalBlank()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->WaitForVerticalBlank(x,y);
}
HRESULT DDRAW::IDirectDraw7::GetAvailableVidMem(DX::LPDDSCAPS2 x, LPDWORD y, LPDWORD z)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw7::GetAvailableVidMem()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->GetAvailableVidMem(x,y,z);
}
HRESULT DDRAW::IDirectDraw7::GetSurfaceFromDC(HDC x, DX::LPDIRECTDRAWSURFACE7 *y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw7::GetSurfaceFromDC()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	if(1) DDRAW_LEVEL(7) << "PANIC! GetSurfaceFromDC() was called...\n";

	return proxy->GetSurfaceFromDC(x,y);
}
HRESULT DDRAW::IDirectDraw7::RestoreAllSurfaces()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw7::RestoreAllSurfaces()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->RestoreAllSurfaces();
}
HRESULT DDRAW::IDirectDraw7::TestCooperativeLevel()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw7::TestCooperativeLevel()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->TestCooperativeLevel();
}
HRESULT DDRAW::IDirectDraw7::GetDeviceIdentifier(DX::LPDDDEVICEIDENTIFIER2 x, DWORD y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw7::GetDeviceIdentifier()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	if(1) DDRAW_LEVEL(7) << "PANIC! GetDeviceIdentifier() was called...\n";

	return proxy->GetDeviceIdentifier(x,y);
}
HRESULT DDRAW::IDirectDraw7::StartModeTest(LPSIZE x, DWORD y, DWORD z)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw7::StartModeTest()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->StartModeTest(x,y,z);
}
HRESULT DDRAW::IDirectDraw7::EvaluateMode(DWORD x, DWORD *y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDraw7::EvaluateMode()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->EvaluateMode(x,y);
}







HRESULT DDRAW::IDirectDrawClipper::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawClipper::QueryInterface()\n";

	DDRAW_IF__ANY__TARGET_RETURN(!DD_OK)

	if(7<=DDRAWLOG::debug)
	{
		LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';
	}

	return proxy->QueryInterface(riid,ppvObj);
}
ULONG DDRAW::IDirectDrawClipper::AddRef()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawClipper::AddRef()\n";

	DDRAW_ADDREF_RETURN(0)
}
ULONG DDRAW::IDirectDrawClipper::Release()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawClipper::Release()\n";

	DDRAW_IF_NOT_TARGET_AND_RELEASE(out,0)

	if(out==0) delete this; 

	return out;
}
HRESULT DDRAW::IDirectDrawClipper::GetClipList(LPRECT x, LPRGNDATA y, LPDWORD z)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawClipper::GetClipList()\n";

	DDRAW_IF__ANY__TARGET_RETURN(!DD_OK)

	return proxy->GetClipList(x,y,z);
}
HRESULT DDRAW::IDirectDrawClipper::GetHWnd(HWND *x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawClipper::GetHWnd()\n";

	DDRAW_IF__ANY__TARGET_RETURN(!DD_OK)

	return proxy->GetHWnd(x);
}
HRESULT DDRAW::IDirectDrawClipper::Initialize(DX::LPDIRECTDRAW x, DWORD y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawClipper::Initialize()\n";

	DDRAW_IF__ANY__TARGET_RETURN(!DD_OK)

	auto p = DDRAW::is_proxy<IDirectDraw>(x);
	
	if(p) x = p->proxy;

	return proxy->Initialize(x,y);
}
HRESULT DDRAW::IDirectDrawClipper::IsClipListChanged(BOOL *x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawClipper::IsClipListChanged()\n";

	if(compat=='dx9c'){ *x = 0; return DD_OK; } //EXPERIMENTAL

	DDRAW_IF__ANY__TARGET_RETURN(!DD_OK)

	return proxy->IsClipListChanged(x);
}
HRESULT DDRAW::IDirectDrawClipper::SetClipList(LPRGNDATA x, DWORD y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawClipper::SetClipList()\n";

	if(compat=='dx9c') return DD_OK; //EXPERIMENTAL

	DDRAW_IF__ANY__TARGET_RETURN(!DD_OK)

	return proxy->SetClipList(x,y);
}
HRESULT DDRAW::IDirectDrawClipper::SetHWnd(DWORD x, HWND y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawClipper::SetHWnd()\n";

	if(compat=='dx9c') //EXPERIMENTAL
	{
		assert(y==DDRAW::window); 
		return DD_OK; 
	}

	DDRAW_IF__ANY__TARGET_RETURN(!DD_OK)

	return proxy->SetHWnd(x,y);
}







HRESULT DDRAW::IDirectDrawPalette::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawPalette::QueryInterface()\n";

	DDRAW_IF__ANY__TARGET_RETURN(!DD_OK)

	if(7<=DDRAWLOG::debug)
	{
		LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';
	}

	return proxy->QueryInterface(riid,ppvObj);
}
ULONG DDRAW::IDirectDrawPalette::AddRef()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawPalette::AddRef()\n";

	DDRAW_ADDREF_RETURN(0)
}
ULONG DDRAW::IDirectDrawPalette::Release()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawPalette::Release()\n";

	DDRAW_IF_NOT_TARGET_AND_RELEASE(out,0)

	if(out==0) delete this; 

	return out;
}
HRESULT DDRAW::IDirectDrawPalette::GetCaps(LPDWORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawPalette::GetCaps()\n";

	DDRAW_IF__ANY__TARGET_RETURN(!DD_OK)

	return proxy->GetCaps(x);
}
HRESULT DDRAW::IDirectDrawPalette::GetEntries(DWORD x, DWORD y, DWORD z, LPPALETTEENTRY w)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawPalette::GetEntries()\n";

	DDRAW_IF__ANY__TARGET_RETURN(!DD_OK)

	return proxy->GetEntries(x,y,z,w);
}
HRESULT DDRAW::IDirectDrawPalette::Initialize(DX::LPDIRECTDRAW x, DWORD y, LPPALETTEENTRY z)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawPalette::Initialize()\n";

	DDRAW_IF__ANY__TARGET_RETURN(!DD_OK)

	auto p = DDRAW::is_proxy<IDirectDraw>(x);
	
	if(p) x = p->proxy;

	return proxy->Initialize(x,y,z);
}
HRESULT DDRAW::IDirectDrawPalette::SetEntries(DWORD x, DWORD y, DWORD z, LPPALETTEENTRY w)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawPalette::SetEntries()\n";

	DDRAW_IF__ANY__TARGET_RETURN(!DD_OK)

	return proxy->SetEntries(x,y,z,w);
}

			




HRESULT DDRAW::IDirectDrawGammaControl::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawGammaControl::QueryInterface()\n";	

	DDRAW_IF__ANY__TARGET_RETURN(!DD_OK)
	
	if(7<=DDRAWLOG::debug)
	{
		LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';
	}

	if(proxy) return proxy->QueryInterface(riid,ppvObj);

	return !S_OK;
}
ULONG DDRAW::IDirectDrawGammaControl::AddRef()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawGammaControl::AddRef()\n";	

	DDRAW_ADDREF_RETURN(0)
} 
ULONG DDRAW::IDirectDrawGammaControl::Release()
{	
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawGammaControl::Release()\n";	

	DDRAW_IF_NOT_TARGET_AND_RELEASE(out,0)
	
	if(out==0) delete this; 

	return out;
}
HRESULT DDRAW::IDirectDrawGammaControl::GetGammaRamp(DWORD x, DX::LPDDGAMMARAMP y)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawGammaControl::GetGammaRamp()\n";	

	DDRAW_IF__COMPAT__TARGET_RETURN(!DD_OK)	if(!y) return !DD_OK;
	
	if(DDRAW::doGamma)
	{
		if(proxy) return proxy->GetGammaRamp(x,y);
	}
	else memcpy(y,gammaramp,sizeof(DX::DDGAMMARAMP));
	
	return DD_OK;
}
HRESULT DDRAW::IDirectDrawGammaControl::SetGammaRamp(DWORD x, DX::LPDDGAMMARAMP y) 
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawGammaControl::SetGammaRamp()\n";	

	DDRAW_IF__COMPAT__TARGET_RETURN(!DD_OK) if(!y) return !DD_OK;

	DDRAW_PUSH_HACK(DIRECTDRAWGAMMACONTROL_SETGAMMARAMP,IDirectDrawGammaControl*,
	DWORD&,DX::LPDDGAMMARAMP&)(0,this,x,y);
			
	memcpy(gammaramp,y,sizeof(DX::DDGAMMARAMP));

	y = gammaramp; //for push/pop (just in case)

	out = DD_OK;

	if(DDRAW::doGamma)
	{
		if(proxy) out = proxy->SetGammaRamp(x,y);
	}

	DDRAW_POP_HACK(DIRECTDRAWGAMMACONTROL_SETGAMMARAMP,IDirectDrawGammaControl*,
	DWORD&,DX::LPDDGAMMARAMP&)(&out,this,x,y);

	DDRAW_RETURN(out) 
}






DDRAW::IDirectDrawSurface *DDRAW::IDirectDrawSurface::get_head = 0; //static

HRESULT DDRAW::IDirectDrawSurface::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::QueryInterface()\n";	

	DDRAW_IF__COMPAT__TARGET_RETURN(!DD_OK)
	
	if(7<=DDRAWLOG::debug)
	{
		LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';
	}
	 										 
	if(riid==DX::IID_IDirectDrawSurface) //(6C14DB81-A733-11CE-A521-0020AF0BE560)
	{
		DDRAW_LEVEL(7) << "(IDirectDrawSurface)\n";		

		AddRef(); return S_OK;
	}
	else if(riid==DX::IID_IDirectDrawSurface4 //(0B2B8630-AD35-11D0-8EA6-00609797EA5B)
		  ||riid==DX::IID_IDirectDrawSurface7) //(06675a80-3b9b-11d2-b92f-00609797ea5b)
	{
		DDRAW_LEVEL(7) 
		<< " (IDirectDrawSurface" 
		<< dx%(riid==DX::IID_IDirectDrawSurface4?"4":"7") 
		<< ")\n";		

		if(riid==DX::IID_IDirectDrawSurface4&&query->dx4)
		{
			query->dx4->AddRef(); *ppvObj = query->dx4;	return S_OK;
		}
		else if(riid==DX::IID_IDirectDrawSurface7&&query->dx7)
		{
			query->dx7->AddRef(); *ppvObj = query->dx7;	return S_OK;
		}

		void *q = 0; HRESULT out = 
		proxy->QueryInterface(riid,(LPVOID*)&q); 
		
		if(out!=S_OK)
		{
			DDRAW_LEVEL(7) << "QueryInterface FAILED\n"; 
			DDRAW_RETURN(out)  
		}

		if(riid==DX::IID_IDirectDrawSurface4&&DDRAW::doIDirectDrawSurface4)
		{	
			::IDirectDrawSurface4 *dds4 = (::IDirectDrawSurface4*)q;
			
			DDRAW::IDirectDrawSurface4 *p = new DDRAW::IDirectDrawSurface4(target,query);

			*ppvObj = query->dx4 = p; p->proxy7 = dds4; 
		}
		else if(riid==DX::IID_IDirectDrawSurface7&&DDRAW::doIDirectDrawSurface7)
		{	
			::IDirectDrawSurface7 *dds7 = (::IDirectDrawSurface7*)q;
			
			DDRAW::IDirectDrawSurface7 *p = new DDRAW::IDirectDrawSurface7('dx7a',query);

			*ppvObj = query->dx7 = p; p->proxy7 = dds7; 
		}
		else *ppvObj = q;

		DDRAW_RETURN(out) 
	}
	else assert(0);

	HRESULT out = proxy->QueryInterface(riid,ppvObj);
	DDRAW_RETURN(out) 
}
ULONG DDRAW::IDirectDrawSurface::AddRef()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::AddRef()\n";	

	DDRAW_ADDREF_RETURN(0)
} 
ULONG DDRAW::IDirectDrawSurface::Release()
{	
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::Release()\n";	

	DDRAW_IF_NOT_TARGET_AND_RELEASE(out,0)
	
	//REMOVE ME?
	//if(out==1&&query->isPrimary)
	//return dx_ddraw_try_primarysurface7_release();
	
	if(out==0) delete this; 

	return out;
}
HRESULT DDRAW::IDirectDrawSurface::AddAttachedSurface(DX::LPDIRECTDRAWSURFACE x)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::AddAttachedSurface()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	auto p = DDRAW::is_proxy<IDirectDrawSurface>(x);
	
	if(p) x = p->proxy;
	
	return proxy->AddAttachedSurface(x);
}
HRESULT DDRAW::IDirectDrawSurface::AddOverlayDirtyRect(LPRECT x)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::AddOverlayDirtyRect()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	return proxy->AddOverlayDirtyRect(x);
}
HRESULT DDRAW::IDirectDrawSurface::Blt(LPRECT x, DX::LPDIRECTDRAWSURFACE y, LPRECT z, DWORD w, DX::LPDDBLTFX q)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::Blt()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	if(q) DDRAW_LEVEL(7) << "PANIC! LPDDBLTFX was passed...\n";

	if(DDRAWLOG::debug>=7) //#ifdef _DEBUG
	{
#define OUT(x) if(w&x) DDRAW_LEVEL(7) << ' ' << #x << '\n';

	OUT(DDBLT_ALPHADEST)
	OUT(DDBLT_ALPHADESTCONSTOVERRIDE)
	OUT(DDBLT_ALPHADESTNEG)
	OUT(DDBLT_ALPHADESTSURFACEOVERRIDE)
	OUT(DDBLT_ALPHAEDGEBLEND)
	OUT(DDBLT_ALPHASRC)
	OUT(DDBLT_ALPHASRCCONSTOVERRIDE)
	OUT(DDBLT_ALPHASRCNEG)
	OUT(DDBLT_ALPHASRCSURFACEOVERRIDE)
	OUT(DDBLT_ASYNC)
	OUT(DDBLT_COLORFILL)
	OUT(DDBLT_DDFX)
	OUT(DDBLT_DDROPS)
	OUT(DDBLT_KEYDEST)
	OUT(DDBLT_KEYDESTOVERRIDE)
	OUT(DDBLT_KEYSRC)
	OUT(DDBLT_KEYSRCOVERRIDE)
	OUT(DDBLT_ROP)
	OUT(DDBLT_ROTATIONANGLE)
	OUT(DDBLT_ZBUFFER)
	OUT(DDBLT_ZBUFFERDESTCONSTOVERRIDE)
	OUT(DDBLT_ZBUFFERDESTOVERRIDE)
	OUT(DDBLT_ZBUFFERSRCCONSTOVERRIDE)
	OUT(DDBLT_ZBUFFERSRCOVERRIDE)
	OUT(DDBLT_WAIT)
	OUT(DDBLT_DEPTHFILL)
	OUT(DDBLT_DONOTWAIT)    

#undef OUT

	}//#endif

	RECT dst; if(x&&!DDRAW::doNothing) 
	{
		DX::DDSCAPS caps; proxy->GetCaps(&caps);

		if(caps.dwCaps&DDSCAPS_BACKBUFFER
		  ||caps.dwCaps&DDSCAPS_FRONTBUFFER
		  ||caps.dwCaps&DDSCAPS_PRIMARYSURFACE
		  ||caps.dwCaps&DDSCAPS_3DDEVICE)
		{
			dst = *x; //not strictly thread safe

			if(DDRAW::doScaling) 
			{
				dst.top = DDRAW::xyScaling[1]*x->top;
				dst.left = DDRAW::xyScaling[0]*x->left;
				dst.right = DDRAW::xyScaling[0]*x->right;
				dst.bottom = DDRAW::xyScaling[1]*x->bottom;
			}
			if(DDRAW::doMapping) 
			{
				dst.top+=DDRAW::xyMapping[1];
				dst.left+=DDRAW::xyMapping[0];
				dst.right+=DDRAW::xyMapping[0];
				dst.bottom+=DDRAW::xyMapping[1];
			}			

			x = &dst;
		}
	}

	bool present = true; DWORD ticks_before_wait;

	//RECT fill;
	RECT src;
	if(query->isPrimary) //assuming "swapping" buffers
	{
		DDRAW_LEVEL(7) << " Destination is primary surface\n";

		if(z&&!DDRAW::doNothing) 
		{
			src = *z; //not strictly thread safe

			if(DDRAW::doScaling) 
			{
				src.top = DDRAW::xyScaling[1]*z->top;
				src.left = DDRAW::xyScaling[0]*z->left;
				src.right = DDRAW::xyScaling[0]*z->right;
				src.bottom = DDRAW::xyScaling[1]*z->bottom;
			}
			if(DDRAW::doMapping) 
			{
				src.top+=DDRAW::xyMapping[1];
				src.left+=DDRAW::xyMapping[0];
				src.right+=DDRAW::xyMapping[0];
				src.bottom+=DDRAW::xyMapping[1];
			}	

			z = &src;
		}
			
		/*2018: I think was a mistake all along...
		//SetCooperativeLevel receives the top-level
		//window, according to its doc
		if(!DDRAW::fullscreen) //if(DDRAW::inWindow)
		{
			if(GetWindowRect(DDRAW::window,&fill))
			{
				if(x)
				{
					fill.right = x->right-x->left+fill.left;
					fill.bottom = x->bottom-x->top+fill.top;					
				}

				x = &fill;
			}
		}*/

		DDRAW::onEffects();

		if(DDRAW::isPaused) DDRAW::onPause();

		present = DDRAW::onFlip();

		ticks_before_wait = DX::tick(); //EXPEREMENTAL

		if(DDRAW::doWait&&DDRAW::DirectDraw7)
		{
			DDRAW::DirectDraw7->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN,0);
		}
	}

	HRESULT out = present?!DD_OK:DD_OK;

	if(!y) 
	{
		if(present) out = proxy->Blt(x,y,z,w,q); 
		
		goto out;
	}

	auto p = DDRAW::is_proxy<IDirectDrawSurface>(y); 

	if(p) y = p->proxy;
	
	if(present) out = proxy->Blt(x,y,z,w,q);

out:if(out!=DD_OK)
	{
		DDRAW_LEVEL(7) << "Blt FAILED\n";	
	}
	else if(present) DDRAW::flipTicks = DX::tick()-ticks_before_wait;

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDrawSurface::BltBatch(DX::LPDDBLTBATCH x, DWORD y, DWORD z)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::BltBatch()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	if(x) DDRAW_LEVEL(7) << "PANIC! LPDDBLTBATCH was passed...\n";
	
	return proxy->BltBatch(x,y,z);
}
HRESULT DDRAW::IDirectDrawSurface::BltFast(DWORD x, DWORD y, DX::LPDIRECTDRAWSURFACE z, LPRECT w, DWORD q)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::BltFast()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	if(DDRAWLOG::debug>=7) //#ifdef _DEBUG
	{
#define OUT(x) if(q&x) DDRAW_LEVEL(7) << ' ' << #x << '\n';

	OUT(DDBLTFAST_DESTCOLORKEY)
	OUT(DDBLTFAST_NOCOLORKEY)
	OUT(DDBLTFAST_SRCCOLORKEY)
	OUT(DDBLTFAST_WAIT)

#undef OUT

	} //#endif

	if(w&&!DDRAW::doNothing
	  &&(DDRAW::xyMapping[0]||DDRAW::xyScaling[0]!=1.0f
	   ||DDRAW::xyMapping[1]||DDRAW::xyScaling[1]!=1.0f)) 
	{
		DX::DDSCAPS caps; proxy->GetCaps(&caps);

		if(caps.dwCaps&DDSCAPS_BACKBUFFER) //need to change to normal blt
		{	
			DWORD flags = 0x00000000;

			if(q&DDBLTFAST_WAIT)         flags|=DDBLT_WAIT;
			if(q&DDBLTFAST_DESTCOLORKEY) flags|=DDBLT_KEYDEST;
			if(q&DDBLTFAST_SRCCOLORKEY)  flags|=DDBLT_KEYSRC;

			RECT dest = { x, y, 0, 0 };

			auto p = DDRAW::is_proxy<IDirectDrawSurface>(z);

			if(!p) DDRAW_RETURN(!DD_OK)

			RECT src; if(!w)   
			{
				DX::DDSURFACEDESC desc; desc.dwSize = sizeof(DX::DDSURFACEDESC);

				if(p->proxy->GetSurfaceDesc(&desc)!=DD_OK) DDRAW_RETURN(!DD_OK)

				src.left = src.top = 0;
				src.right = desc.dwWidth;
				src.bottom = desc.dwHeight;

				w = &src;
			}

			dest.right = dest.left+(w->right-w->left);
			dest.bottom = dest.top+(w->bottom-w->top);

			return Blt(&dest,z,w,flags,0);
		}
	}

	auto p = DDRAW::is_proxy<IDirectDrawSurface>(z);
	
	if(p) z = p->proxy;

	HRESULT out = proxy->BltFast(x,y,z,w,q);

	if(out!=DD_OK)
	{
		DDRAW_LEVEL(7) << "BltFast FAILED\n";	
	}

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDrawSurface::DeleteAttachedSurface(DWORD x, DX::LPDIRECTDRAWSURFACE y)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::DeleteAttachedSurface()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	auto p = DDRAW::is_proxy<IDirectDrawSurface>(y);
	
	if(p) y = p->proxy;

	return proxy->DeleteAttachedSurface(x,y);
}
HRESULT DDRAW::IDirectDrawSurface::EnumAttachedSurfaces(LPVOID x, DX::LPDDENUMSURFACESCALLBACK y)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::EnumAttachedSurfaces()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	return proxy->EnumAttachedSurfaces(x,y);
}
HRESULT DDRAW::IDirectDrawSurface::EnumOverlayZOrders(DWORD x, LPVOID y, DX::LPDDENUMSURFACESCALLBACK z)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::EnumOverlayZOrders()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	if(z) DDRAW_LEVEL(7) << "PANIC! LPDDENUMSURFACESCALLBACK was passed...\n";

	return proxy->EnumOverlayZOrders(x,y,z);
}
HRESULT DDRAW::IDirectDrawSurface::Flip(DX::LPDIRECTDRAWSURFACE x, DWORD y)
{								   
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::Flip()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	return proxy->Flip(x,y);
}
HRESULT DDRAW::IDirectDrawSurface::GetAttachedSurface(DX::LPDDSCAPS x, DX::LPDIRECTDRAWSURFACE *y)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::GetAttachedSurface()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	HRESULT out = proxy->GetAttachedSurface(x,y);

	if(out==DD_OK)
	{
		//if(DDRAW::inWindow) assert(0);

		DDRAW::IDirectDrawSurface *p = get(*y);

		if(!p) p = new DDRAW::IDirectDrawSurface(target); p->proxy = *y;

		*y = (DX::IDirectDrawSurface*)p; 
	}
	
	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDrawSurface::GetBltStatus(DWORD x)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::GetBltStatus()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	return proxy->GetBltStatus(x);
}
HRESULT DDRAW::IDirectDrawSurface::GetCaps(DX::LPDDSCAPS x)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::GetCaps()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	return proxy->GetCaps(x);
}
HRESULT DDRAW::IDirectDrawSurface::GetClipper(DX::LPDIRECTDRAWCLIPPER *x)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::GetClipper()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
		assert(0); //2021: DDRAW::IDirectDrawClipper?

	return proxy->GetClipper(x);
}
HRESULT DDRAW::IDirectDrawSurface::GetColorKey(DWORD x, DX::LPDDCOLORKEY y)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::GetColorKey()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	return proxy->GetColorKey(x,y); 
}
HRESULT DDRAW::IDirectDrawSurface::GetDC(HDC *x)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::GetDC()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	HRESULT out = proxy->GetDC(x);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDrawSurface::GetFlipStatus(DWORD x)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::GetFlipStatus()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	return proxy->GetFlipStatus(x);
}
HRESULT DDRAW::IDirectDrawSurface::GetOverlayPosition(LPLONG x, LPLONG y)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::GetOverlayPosition()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	return proxy->GetOverlayPosition(x,y);
}
HRESULT DDRAW::IDirectDrawSurface::GetPalette(DX::LPDIRECTDRAWPALETTE *x)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::GetPalette()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	return proxy->GetPalette(x);
}
HRESULT DDRAW::IDirectDrawSurface::GetPixelFormat(DX::LPDDPIXELFORMAT x)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::GetPixelFormat()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	return proxy->GetPixelFormat(x);
}
HRESULT DDRAW::IDirectDrawSurface::GetSurfaceDesc(DX::LPDDSURFACEDESC x)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::GetSurfaceDesc()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	HRESULT out = proxy->GetSurfaceDesc(x);
	
	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDrawSurface::Initialize(DX::LPDIRECTDRAW x, DX::LPDDSURFACEDESC y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::Initialize()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	return proxy->Initialize(x,y);
}
HRESULT DDRAW::IDirectDrawSurface::IsLost()
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::IsLost()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	return proxy->IsLost();
}
HRESULT DDRAW::IDirectDrawSurface::Lock(LPRECT x, DX::LPDDSURFACEDESC y, DWORD z, HANDLE w)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::Lock()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	return proxy->Lock(x,y,z,w);
}
HRESULT DDRAW::IDirectDrawSurface::ReleaseDC(HDC x)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::ReleaseDC()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	HRESULT out = proxy->ReleaseDC(x); DDRAW_RETURN(out)
}
HRESULT DDRAW::IDirectDrawSurface::Restore()
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::Restore()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	HRESULT out = proxy->Restore(); DDRAW_RETURN(out)
}
HRESULT DDRAW::IDirectDrawSurface::SetClipper(DX::LPDIRECTDRAWCLIPPER x) 
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::SetClipper()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	return proxy->SetClipper(x);
}
HRESULT DDRAW::IDirectDrawSurface::SetColorKey(DWORD x, DX::LPDDCOLORKEY y)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::SetColorKey()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	return proxy->SetColorKey(x,y);
}
HRESULT DDRAW::IDirectDrawSurface::SetOverlayPosition(LONG x, LONG y)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::SetOverlayPosition()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	return proxy->SetOverlayPosition(x,y);
}
HRESULT DDRAW::IDirectDrawSurface::SetPalette(DX::LPDIRECTDRAWPALETTE x)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::SetPalette()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
		
	auto p = DDRAW::is_proxy<IDirectDrawPalette>(x);
	
	if(p) x = p->proxy;

	return proxy->SetPalette(x);
}
HRESULT DDRAW::IDirectDrawSurface::Unlock(LPVOID x)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::Unlock()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	return proxy->Unlock(x);
}
HRESULT DDRAW::IDirectDrawSurface::UpdateOverlay(LPRECT x, DX::LPDIRECTDRAWSURFACE y, LPRECT z, DWORD w, DX::LPDDOVERLAYFX q)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::UpdateOverlay()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	if(q) DDRAW_LEVEL(7) << "PANIC! LPDDOVERLAYFX was passed...\n";

	auto p = DDRAW::is_proxy<IDirectDrawSurface>(y);
	
	if(p) y = p->proxy;
	
	return proxy->UpdateOverlay(x,y,z,w,q);
}
HRESULT DDRAW::IDirectDrawSurface::UpdateOverlayDisplay(DWORD x)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::UpdateOverlayDisplay()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	return proxy->UpdateOverlayDisplay(x);
}
HRESULT DDRAW::IDirectDrawSurface::UpdateOverlayZOrder(DWORD x, DX::LPDIRECTDRAWSURFACE y)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface::UpdateOverlayZOrder()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	auto p = DDRAW::is_proxy<IDirectDrawSurface>(y);
	
	if(p) y = p->proxy;

	return proxy->UpdateOverlayZOrder(x,y); 
}





DDRAW::IDirectDrawSurface4 *DDRAW::IDirectDrawSurface4::get_head = 0; //static 

HRESULT DDRAW::IDirectDrawSurface4::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::QueryInterface()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	if(7<=DDRAWLOG::debug)
	{
		LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';
	}
	
	if(riid==DX::IID_IDirectDrawGammaControl) //{69C11C3E-B46B-11D1-AD7A-00C04FC29B4E}
	{
		DDRAW_LEVEL(7) << "(IDirectDrawGammaControl)\n";

		if(DDRAW::doIDirectDrawGammaControl)
		{
			if(query->gamma)			
			{
				*ppvObj = query->gamma; query->gamma->AddRef(); 
				
				return S_OK;
			}

			::IDirectDrawGammaControl *q = 0; 

			if(DDRAW::doGamma)
			{
				HRESULT out = proxy->QueryInterface(riid,(LPVOID*)&q); 
				
				if(out!=S_OK) q = 0; //don't panic! assuming not in fullscreen mode
			}
										
			DDRAW::IDirectDrawGammaControl *p = new DDRAW::IDirectDrawGammaControl(target);

			p->proxy7 = q; query->gamma = p; p->surface = query; 
			
			*ppvObj = p; p->AddRef(); return S_OK;
		}
	}
	else if(riid==DX::IID_IDirectDrawSurface4) //(0B2B8630-AD35-11D0-8EA6-00609797EA5B)
	{
		DDRAW_LEVEL(7) << "(IDirectDrawSurface7)\n";		

		AddRef(); return S_OK;
	}
	else if(riid==DX::IID_IDirectDrawSurface //(6C14DB81-A733-11CE-A521-0020AF0BE560)
		  ||riid==DX::IID_IDirectDrawSurface7) //(06675a80-3b9b-11d2-b92f-00609797ea5b)
	{
		DDRAW_LEVEL(7) 
		<< " (IDirectDrawSurface" 
		<< dx%(riid==DX::IID_IDirectDrawSurface7?"7":"") 
		<< ")\n";		

		if(riid==DX::IID_IDirectDrawSurface&&query->dxf)
		{
			query->dxf->AddRef(); *ppvObj = query->dxf;	return S_OK;
		}
		else if(riid==DX::IID_IDirectDrawSurface7&&query->dx7)
		{
			query->dx7->AddRef(); *ppvObj = query->dx7;	return S_OK;
		}

		void *q = 0; HRESULT out = 
		proxy->QueryInterface(riid,(LPVOID*)&q); 
		
		if(out!=S_OK)
		{
			DDRAW_LEVEL(7) << "QueryInterface FAILED\n"; 
			DDRAW_RETURN(out)  
		}

		if(riid==DX::IID_IDirectDrawSurface&&DDRAW::doIDirectDrawSurface)
		{	
			::IDirectDrawSurface *dds = (::IDirectDrawSurface*)q;
			
			DDRAW::IDirectDrawSurface *p = new DDRAW::IDirectDrawSurface(target,query);

			*ppvObj = query->dxf = p; p->proxy7 = dds; 
		}
		else if(riid==DX::IID_IDirectDrawSurface7&&DDRAW::doIDirectDrawSurface7)
		{	
			::IDirectDrawSurface7 *dds7 = (::IDirectDrawSurface7*)q;
			
			DDRAW::IDirectDrawSurface7 *p = new DDRAW::IDirectDrawSurface7('dx7a',query);

			*ppvObj = query->dx7 = p; p->proxy7 = dds7; 
		}
		else *ppvObj = q;

		DDRAW_RETURN(out) 
	}
	else if(riid==DX::IID_IDirect3DTexture2) //{93281502-8CF8-11D0-89AB-00A0C9054129}
	{
		DDRAW_LEVEL(7) << " (IDirect3DTexture2)\n";

		if(target=='_dx6')
		{
			::IDirect3DTexture2 *q = 0; 
			
			HRESULT out = proxy->QueryInterface(riid,(LPVOID*)&q); 

			if(doIDirect3DTexture2)
			{
				DDRAW::IDirect3DTexture2 *p = new DDRAW::IDirect3DTexture2('_dx6');

				*ppvObj = p; p->proxy6 = q; p->surface = query; AddRef();
			}
			else *ppvObj = q;

			DDRAW_RETURN(out)
		}
		else if(target=='dx7a')
		{				
			DDRAW::IDirect3DTexture2 *p = new DDRAW::IDirect3DTexture2('dx7a');

			*ppvObj = p; p->proxy = proxy; proxy->AddRef();

			return S_OK;
		}
		else assert(0);
	}
	else assert(0);

	return proxy->QueryInterface(riid,ppvObj);
}
ULONG DDRAW::IDirectDrawSurface4::AddRef()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::AddRef()\n";

	DDRAW_ADDREF_RETURN(0)
}
ULONG DDRAW::IDirectDrawSurface4::Release()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::Release()\n";

	DDRAW_IF_NOT_TARGET_AND_RELEASE(out,0)

	//REMOVE ME?
	//if(out==1&&query->isPrimary)
	//return dx_ddraw_try_primarysurface7_release();

	if(out==0) delete this; 

	return out;
}
HRESULT DDRAW::IDirectDrawSurface4::AddAttachedSurface(DX::LPDIRECTDRAWSURFACE4 x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::AddAttachedSurface()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)
		
	auto p = DDRAW::is_proxy<IDirectDrawSurface4>(x);
	
	if(p) x = p->proxy;
		
	HRESULT out = proxy->AddAttachedSurface(x);	

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDrawSurface4::AddOverlayDirtyRect(LPRECT x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::AddOverlayDirtyRect()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	return proxy->AddOverlayDirtyRect(x);
}
HRESULT DDRAW::IDirectDrawSurface4::Blt(LPRECT x, DX::LPDIRECTDRAWSURFACE4 y, LPRECT z, DWORD w, DX::LPDDBLTFX q)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::Blt()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	DDRAW_PUSH_HACK(DIRECTDRAWSURFACE4_BLT,IDirectDrawSurface4*,
	LPRECT&,DX::LPDIRECTDRAWSURFACE4&,LPRECT&,DWORD&,DX::LPDDBLTFX&)(0,this,x,y,z,w,q);

	if(x&&IsRectEmpty(x)) return DD_OK; //hack: allow short circuiting

	DDRAW_LEVEL(7) << "source is " << (UINT)y << "(" << (UINT)z << ")\n";

	if(DDRAWLOG::debug>=7) //#ifdef _DEBUG
	{
#define OUT(x) if(w&x) DDRAW_LEVEL(7) << ' ' << #x << '\n';

	OUT(DDBLT_ALPHADEST)
	OUT(DDBLT_ALPHADESTCONSTOVERRIDE)
	OUT(DDBLT_ALPHADESTNEG)
	OUT(DDBLT_ALPHADESTSURFACEOVERRIDE)
	OUT(DDBLT_ALPHAEDGEBLEND)
	OUT(DDBLT_ALPHASRC)
	OUT(DDBLT_ALPHASRCCONSTOVERRIDE)
	OUT(DDBLT_ALPHASRCNEG)
	OUT(DDBLT_ALPHASRCSURFACEOVERRIDE)
	OUT(DDBLT_ASYNC)
	OUT(DDBLT_COLORFILL)
	OUT(DDBLT_DDFX)
	OUT(DDBLT_DDROPS)
	OUT(DDBLT_KEYDEST)
	OUT(DDBLT_KEYDESTOVERRIDE)
	OUT(DDBLT_KEYSRC)
	OUT(DDBLT_KEYSRCOVERRIDE)
	OUT(DDBLT_ROP)
	OUT(DDBLT_ROTATIONANGLE)
	OUT(DDBLT_ZBUFFER)
	OUT(DDBLT_ZBUFFERDESTCONSTOVERRIDE)
	OUT(DDBLT_ZBUFFERDESTOVERRIDE)
	OUT(DDBLT_ZBUFFERSRCCONSTOVERRIDE)
	OUT(DDBLT_ZBUFFERSRCOVERRIDE)
	OUT(DDBLT_WAIT)
	OUT(DDBLT_DEPTHFILL)
	OUT(DDBLT_DONOTWAIT)    

#undef OUT
	} //#endif

	if(q)
	{
		DDRAW_LEVEL(7) << "PANIC! LPDDBLTFX was passed...\n";

		DDRAW::log_surface_bltfx(q);
	}		

	//TODO: needs DPI aware counter-scaling
	//SOM_MAP has these in HWND coordinates
	RECT dst;
	if(!dx_DPI_testing_2021)
	if(x&&!DDRAW::doNothing) 
	{
		DX::DDSCAPS2 caps; proxy->GetCaps(&caps);

		if(caps.dwCaps&DDSCAPS_BACKBUFFER
		  ||caps.dwCaps&DDSCAPS_FRONTBUFFER
		  ||caps.dwCaps&DDSCAPS_PRIMARYSURFACE
		  ||caps.dwCaps&DDSCAPS_3DDEVICE)
		{
			dst = *x; 

			if(DDRAW::doScaling) 
			{
				dst.top = DDRAW::xyScaling[1]*x->top;
				dst.left = DDRAW::xyScaling[0]*x->left;
				dst.right = DDRAW::xyScaling[0]*x->right;
				dst.bottom = DDRAW::xyScaling[1]*x->bottom;
			}
			if(DDRAW::doMapping) 
			{
				dst.top+=DDRAW::xyMapping[1];
				dst.left+=DDRAW::xyMapping[0];
				dst.right+=DDRAW::xyMapping[0];
				dst.bottom+=DDRAW::xyMapping[1];
			}			

			x = &dst;
		}
	}

	bool present = true; DWORD ticks_before_wait;

	//RECT fill;
	RECT src;
	if(query->isPrimary) //assuming "swapping" buffers
	{
		DDRAW_LEVEL(7) << " Destination is primary surface\n";

		if(z&&!DDRAW::doNothing) 
		{
			src = *z;

			if(DDRAW::doScaling) 
			{
				src.top = DDRAW::xyScaling[1]*z->top;
				src.left = DDRAW::xyScaling[0]*z->left;
				src.right = DDRAW::xyScaling[0]*z->right;
				src.bottom = DDRAW::xyScaling[1]*z->bottom;
			}
			if(DDRAW::doMapping) 
			{
				src.top+=DDRAW::xyMapping[1];
				src.left+=DDRAW::xyMapping[0];
				src.right+=DDRAW::xyMapping[0];
				src.bottom+=DDRAW::xyMapping[1];
			}	

			z = &src;
		}

		/*2018: I think was a mistake all along...
		//SetCooperativeLevel receives the top-level
		//window, according to its doc
		if(!DDRAW::fullscreen) //if(DDRAW::inWindow)
		{	
			RECT fill;

			if(GetWindowRect(DDRAW::window,&fill))
			{
				if(x)
				{
					fill.right = x->right-x->left+fill.left;
					fill.bottom = x->bottom-x->top+fill.top;					
				}

				x = &fill;
			}
		}*/

		DDRAW::onEffects();

		if(DDRAW::isPaused) DDRAW::onPause();

		present = DDRAW::onFlip();

		ticks_before_wait = DX::tick(); //EXPEREMENTAL
		 		
		if(DDRAW::doWait&&DDRAW::DirectDraw7)
		{
			DDRAW::DirectDraw7->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN,0);
		}
	}
	else ticks_before_wait = DX::tick(); //EXPEREMENTAL

	out = present?!DD_OK:DD_OK;

	if(!y) 
	{
		if(present) out = proxy->Blt(x,y,z,w,q); 
		
		goto out;
	}

	auto p = DDRAW::is_proxy<IDirectDrawSurface4>(y);
	
	if(p&&p->query->isPrimary) 
	DDRAW_LEVEL(7) << " Source is primary surface\n";

	if(p) y = p->proxy; 
	
	if(present) out = proxy->Blt(x,y,z,w,q);

out:if(out!=DD_OK)
	{
		DDRAW_LEVEL(7) << "Blt FAILED\n";	
	}
	else if(present) DDRAW::flipTicks = DX::tick()-ticks_before_wait;

	DDRAW_POP_HACK(DIRECTDRAWSURFACE4_BLT,IDirectDrawSurface4*,
	LPRECT&,DX::LPDIRECTDRAWSURFACE4&,LPRECT&,DWORD&,DX::LPDDBLTFX&)(&out,this,x,y,z,w,q);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDrawSurface4::BltBatch(DX::LPDDBLTBATCH x, DWORD y, DWORD z)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::BltBatch()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	if(x) DDRAW_LEVEL(7) << "PANIC! LPDDBLTBATCH was passed...\n";

	return proxy->BltBatch(x,y,z);
}
HRESULT DDRAW::IDirectDrawSurface4::BltFast(DWORD x, DWORD y, DX::LPDIRECTDRAWSURFACE4 z, LPRECT w, DWORD q)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::BltFast()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)
	
#ifdef _DEBUG
#define OUT(x) if(q&x) DDRAW_LEVEL(7) << ' ' << #x << '\n';

	OUT(DDBLTFAST_DESTCOLORKEY)
	OUT(DDBLTFAST_NOCOLORKEY)
	OUT(DDBLTFAST_SRCCOLORKEY)
	OUT(DDBLTFAST_WAIT)

#undef OUT
#endif

	if(w&&!DDRAW::doNothing
	  &&(DDRAW::xyMapping[0]||DDRAW::xyScaling[0]!=1.0f
	   ||DDRAW::xyMapping[1]||DDRAW::xyScaling[1]!=1.0f)) 
	{
		DX::DDSCAPS2 caps; proxy->GetCaps(&caps);

		if(caps.dwCaps&DDSCAPS_BACKBUFFER) //need to change to normal blt
		{	
			DWORD flags = 0x00000000;

			if(q&DDBLTFAST_WAIT)         flags|=DDBLT_WAIT;
			if(q&DDBLTFAST_DESTCOLORKEY) flags|=DDBLT_KEYDEST;
			if(q&DDBLTFAST_SRCCOLORKEY)  flags|=DDBLT_KEYSRC;

			RECT dst = { x, y, 0, 0 };

			auto p = DDRAW::is_proxy<IDirectDrawSurface4>(z);

			if(!p) DDRAW_RETURN(!DD_OK)

			RECT src; if(!w)   
			{
				DX::DDSURFACEDESC2 desc; desc.dwSize = sizeof(DX::DDSURFACEDESC2);

				if(p->proxy->GetSurfaceDesc(&desc)!=D3D_OK) DDRAW_RETURN(!DD_OK)

				src.left = src.top = 0;
				src.right = desc.dwWidth;
				src.bottom = desc.dwHeight;

				w = &src;
			}

			dst.right = dst.left+(w->right-w->left);
			dst.bottom = dst.top+(w->bottom-w->top);

			return Blt(&dst,z,w,flags,0);
		}
	}

	auto p = DDRAW::is_proxy<IDirectDrawSurface4>(z);
	
	if(p) z = p->proxy;

	HRESULT out = proxy->BltFast(x,y,z,w,q);

	if(out!=DD_OK)
	{
		DDRAW_LEVEL(7) << "BltFast FAILED\n";	
	}

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDrawSurface4::DeleteAttachedSurface(DWORD x, DX::LPDIRECTDRAWSURFACE4 y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::DeleteAttachedSurface()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)
	
	auto p = DDRAW::is_proxy<IDirectDrawSurface4>(y);
	
	if(p) y = p->proxy;

	DDRAW_RETURN(proxy->DeleteAttachedSurface(x,y))
}
HRESULT DDRAW::IDirectDrawSurface4::EnumAttachedSurfaces(LPVOID x, DX::LPDDENUMSURFACESCALLBACK2 y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::EnumAttachedSurfaces()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	if(y) DDRAW_LEVEL(7) << "PANIC! LPDDENUMSURFACESCALLBACK7 was passed...\n";

	return proxy->EnumAttachedSurfaces(x,y);
}
HRESULT DDRAW::IDirectDrawSurface4::EnumOverlayZOrders(DWORD x, LPVOID y, DX::LPDDENUMSURFACESCALLBACK2 z)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::EnumOverlayZOrders()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	if(z) DDRAW_LEVEL(7) << "PANIC! LPDDENUMSURFACESCALLBACK7 was passed...\n";

	return proxy->EnumOverlayZOrders(x,y,z);
}
HRESULT DDRAW::IDirectDrawSurface4::Flip(DX::LPDIRECTDRAWSURFACE4 x, DWORD y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::Flip() " << DDRAW::noFlips << '\n';

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)
		
	DDRAW_PUSH_HACK(DIRECTDRAWSURFACE4_FLIP,IDirectDrawSurface4*,
	DX::LPDIRECTDRAWSURFACE4&,DWORD&)(0,this,x,y);
	
	auto p = DDRAW::is_proxy<IDirectDrawSurface4>(x);
	
	if(p) x = p->proxy;

	if(DDRAW::isPaused) DDRAW::onPause();

	bool present = DDRAW::onFlip();

	DWORD ticks_before_wait = DX::tick();

	//Why were their 3 exactly?
	//if(!DDRAW::inWindow&&DDRAW::isExclusive)
	if(DDRAW::fullscreen)
	{	
		if(present)
		{
			DDRAW_FINISH(proxy->Flip(x,y)) //... 
		}
		else DDRAW_FINISH(DD_OK) 
	}

	//C4533
	{
		RECT z = {0,0,640,480};	
		DDRAW::window_rect(&z);
	
		RECT fill = z;

		GetWindowRect(DDRAW::window,&fill);
		
		if(DDRAW::doWait&&DDRAW::DirectDraw7)
		{
			DDRAW::DirectDraw7->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN,0);
		}

		if(present)
		{	
			out = DDRAW::PrimarySurface7->proxy7->
			Blt(&fill,DDRAW::BackBufferSurface7->proxy7,&z,DDBLT_WAIT,0); 
		}
		else out = DD_OK;
	}

	switch(out)
	{
	case DD_OK: break;

	case DDERR_SURFACELOST: assert(0); break;

	default: break;
	}	

	if(!out&&present) DDRAW::flipTicks = DX::tick()-ticks_before_wait;
  	
	DDRAW_POP_HACK(DIRECTDRAWSURFACE4_FLIP,IDirectDrawSurface4*,
	DX::LPDIRECTDRAWSURFACE4&,DWORD&)(&out,this,x,y);
		
	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDrawSurface4::GetAttachedSurface(DX::LPDDSCAPS2 x, DX::LPDIRECTDRAWSURFACE4 *y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::GetAttachedSurface()\n";				 

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

#ifdef _DEBUG	 
#define OUT(X) if(x->dwCaps&X) DDRAW_LEVEL(7) << ' ' << #X << '\n';

	OUT(DDSCAPS_ALPHA)
	OUT(DDSCAPS_BACKBUFFER)
	OUT(DDSCAPS_COMPLEX)
	OUT(DDSCAPS_FLIP)
	OUT(DDSCAPS_FRONTBUFFER)
	OUT(DDSCAPS_OFFSCREENPLAIN)
	OUT(DDSCAPS_OVERLAY)
	OUT(DDSCAPS_PALETTE)
	OUT(DDSCAPS_PRIMARYSURFACE)
	OUT(DDSCAPS_SYSTEMMEMORY)
	OUT(DDSCAPS_TEXTURE)
	OUT(DDSCAPS_VIDEOMEMORY)
	OUT(DDSCAPS_VISIBLE)
	OUT(DDSCAPS_WRITEONLY)
	OUT(DDSCAPS_ZBUFFER)
	OUT(DDSCAPS_OWNDC)
	OUT(DDSCAPS_HWCODEC)
	OUT(DDSCAPS_MIPMAP)	 
	OUT(DDSCAPS_ALLOCONLOAD)
	OUT(DDSCAPS_LOCALVIDMEM)
	OUT(DDSCAPS_NONLOCALVIDMEM)
	OUT(DDSCAPS_STANDARDVGAMODE)
	OUT(DDSCAPS_OPTIMIZED)

#undef OUT
#endif

	HRESULT out = !DD_OK;

	if(x->dwCaps&DDSCAPS_BACKBUFFER)
	{	
		if(y&&DDRAW::BackBufferSurface7) 
		{
			DDRAW::BackBufferSurface7->QueryInterface(DX::IID_IDirectDrawSurface7,(LPVOID*)y);

			return DD_OK; //DDRAW_FINISH(DD_OK)
		}
		else out = !DD_OK;
	}
	else out = proxy->GetAttachedSurface(x,y);
		
	if(out==DD_OK)
	{
		DDRAW::IDirectDrawSurface4 *p = get(*y);

		if(!p) p = new DDRAW::IDirectDrawSurface4(target); p->proxy = *y;

		*y = (DX::IDirectDrawSurface4*)p; 
	}
	else DDRAW_LEVEL(7) << "GetAttachedSurface FAILED\n";		
	
	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDrawSurface4::GetBltStatus(DWORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::GetBltStatus()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	return proxy->GetBltStatus(x);
}
HRESULT DDRAW::IDirectDrawSurface4::GetCaps(DX::LPDDSCAPS2 x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::GetCaps()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	return proxy->GetCaps(x);
}
HRESULT DDRAW::IDirectDrawSurface4::GetClipper(DX::LPDIRECTDRAWCLIPPER *x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::GetClipper()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

		assert(0); //2021: DDRAW::IDirectDrawClipper?

	DDRAW_RETURN(proxy->GetClipper(x))
}
HRESULT DDRAW::IDirectDrawSurface4::GetColorKey(DWORD x, DX::LPDDCOLORKEY y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::GetColorKey()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	DDRAW_RETURN(proxy->GetColorKey(x,y))
}
HRESULT DDRAW::IDirectDrawSurface4::GetDC(HDC *x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::GetDC()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)
	
	DDRAW_PUSH_HACK(DIRECTDRAWSURFACE4_GETDC,IDirectDrawSurface4*,
	HDC*&)(0,this,x);	
	
	out = proxy->GetDC(x);

	DDRAW_POP_HACK(DIRECTDRAWSURFACE4_GETDC,IDirectDrawSurface4*,
	HDC*&)(&out,this,x);	

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDrawSurface4::GetFlipStatus(DWORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::GetFlipStatus()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	return proxy->GetFlipStatus(x);
}
HRESULT DDRAW::IDirectDrawSurface4::GetOverlayPosition(LPLONG x, LPLONG y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::GetOverlayPosition()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	return proxy->GetOverlayPosition(x,y);
}
HRESULT DDRAW::IDirectDrawSurface4::GetPalette(DX::LPDIRECTDRAWPALETTE *x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::GetPalette()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	DDRAW_RETURN(proxy->GetPalette(x))
}
HRESULT DDRAW::IDirectDrawSurface4::GetPixelFormat(DX::LPDDPIXELFORMAT x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::GetPixelFormat()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	return proxy->GetPixelFormat(x);
}
HRESULT DDRAW::IDirectDrawSurface4::GetSurfaceDesc(DX::LPDDSURFACEDESC2 x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::GetSurfaceDesc()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	HRESULT out = proxy->GetSurfaceDesc(x);
	
	if(out!=DD_OK) DDRAW_RETURN(out) 

#ifdef _DEBUG
#define OUT(X) if(x->dwFlags&X) DDRAW_LEVEL(7) << ' ' << #X << '\n';

	OUT(DDSD_CAPS)
	OUT(DDSD_HEIGHT)
	OUT(DDSD_WIDTH)
	OUT(DDSD_PITCH)
	OUT(DDSD_BACKBUFFERCOUNT)
	OUT(DDSD_ZBUFFERBITDEPTH)
	OUT(DDSD_ALPHABITDEPTH)
	OUT(DDSD_LPSURFACE)
	OUT(DDSD_PIXELFORMAT)
	OUT(DDSD_CKDESTOVERLAY)
	OUT(DDSD_CKDESTBLT)
	OUT(DDSD_CKSRCOVERLAY)
	OUT(DDSD_CKSRCBLT)
	OUT(DDSD_REFRESHRATE)
	OUT(DDSD_LINEARSIZE)
	OUT(DDSD_TEXTURESTAGE)
	OUT(DDSD_FVF)
	OUT(DDSD_DEPTH)

#define OUT(X) if(x->ddsCaps.dwCaps&X) DDRAW_LEVEL(7) << ' ' << #X << '\n';

	OUT(DDSCAPS_ALPHA)
	OUT(DDSCAPS_BACKBUFFER)
	OUT(DDSCAPS_COMPLEX)
	OUT(DDSCAPS_FLIP)
	OUT(DDSCAPS_FRONTBUFFER)
	OUT(DDSCAPS_OFFSCREENPLAIN)
	OUT(DDSCAPS_OVERLAY)
	OUT(DDSCAPS_PALETTE)
	OUT(DDSCAPS_PRIMARYSURFACE)
	OUT(DDSCAPS_SYSTEMMEMORY)
	OUT(DDSCAPS_TEXTURE)
	OUT(DDSCAPS_VIDEOMEMORY)
	OUT(DDSCAPS_VISIBLE)
	OUT(DDSCAPS_WRITEONLY)
	OUT(DDSCAPS_ZBUFFER)
	OUT(DDSCAPS_OWNDC)
	OUT(DDSCAPS_HWCODEC)
	OUT(DDSCAPS_MIPMAP)	 
	OUT(DDSCAPS_ALLOCONLOAD)
	OUT(DDSCAPS_LOCALVIDMEM)
	OUT(DDSCAPS_NONLOCALVIDMEM)
	OUT(DDSCAPS_STANDARDVGAMODE)
	OUT(DDSCAPS_OPTIMIZED)

#undef OUT
#endif

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDrawSurface4::Initialize(DX::LPDIRECTDRAW x, DX::LPDDSURFACEDESC2 y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::Initialize()\n"; 

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	if(1) DDRAW_LEVEL(7) << "PANIC! Initialize() was called...\n";
						 
	return proxy->Initialize(x,y);
}
HRESULT DDRAW::IDirectDrawSurface4::IsLost()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::IsLost()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	DDRAW_RETURN(proxy->IsLost())
}
HRESULT DDRAW::IDirectDrawSurface4::Lock(LPRECT x, DX::LPDDSURFACEDESC2 y, DWORD z, HANDLE w)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::Lock()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	DDRAW_RETURN(proxy->Lock(x,y,z,w))
}
HRESULT DDRAW::IDirectDrawSurface4::ReleaseDC(HDC x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::ReleaseDC()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK) 
		
	DDRAW_PUSH_HACK(DIRECTDRAWSURFACE4_RELEASEDC,IDirectDrawSurface4*,
	HDC&)(0,this,x);	
	
	out = proxy->ReleaseDC(x);

	DDRAW_POP_HACK(DIRECTDRAWSURFACE4_RELEASEDC,IDirectDrawSurface4*,
	HDC&)(&out,this,x);	

	DDRAW_RETURN(out)
}
HRESULT DDRAW::IDirectDrawSurface4::Restore()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::Restore()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	DDRAW_RETURN(proxy->Restore())
}
HRESULT DDRAW::IDirectDrawSurface4::SetClipper(DX::LPDIRECTDRAWCLIPPER x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::SetClipper()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	DDRAW_RETURN(proxy->SetClipper(x))
}
HRESULT DDRAW::IDirectDrawSurface4::SetColorKey(DWORD x, DX::LPDDCOLORKEY y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::SetColorKey()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)
	
	DDRAW_PUSH_HACK(DIRECTDRAWSURFACE4_SETCOLORKEY,IDirectDrawSurface4*,
	DWORD&,DX::LPDDCOLORKEY&)(0,this,x,y);	
	
	out = proxy->SetColorKey(x,y);

	DDRAW_POP_HACK(DIRECTDRAWSURFACE4_SETCOLORKEY,IDirectDrawSurface4*,
	DWORD&,DX::LPDDCOLORKEY&)(&out,this,x,y);	

	DDRAW_RETURN(out)
}
HRESULT DDRAW::IDirectDrawSurface4::SetOverlayPosition(LONG x, LONG y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::SetOverlayPosition()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	return proxy->SetOverlayPosition(x,y);
}
HRESULT DDRAW::IDirectDrawSurface4::SetPalette(DX::LPDIRECTDRAWPALETTE x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::SetPalette()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)
					   		
	auto p = DDRAW::is_proxy<IDirectDrawPalette>(x);
	
	if(p) x = p->proxy;

	HRESULT out = proxy->SetPalette(x);

	return out; //DDRAW_RETURN(out)
}
HRESULT DDRAW::IDirectDrawSurface4::Unlock(LPRECT x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::Unlock()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	DDRAW_RETURN(proxy->Unlock(x))
}
HRESULT DDRAW::IDirectDrawSurface4::UpdateOverlay(LPRECT x, DX::LPDIRECTDRAWSURFACE4 y, LPRECT z, DWORD w, DX::LPDDOVERLAYFX q)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::UpdateOverlay()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	if(q) DDRAW_LEVEL(7) << "PANIC! LPDDOVERLAYFX was passed...\n";
	
	auto p = DDRAW::is_proxy<IDirectDrawSurface4>(y);
	
	if(p) y = p->proxy;

	return proxy->UpdateOverlay(x,y,z,w,q);
}
HRESULT DDRAW::IDirectDrawSurface4::UpdateOverlayDisplay(DWORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::UpdateOverlayDisplay()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	return proxy->UpdateOverlayDisplay(x);
}
HRESULT DDRAW::IDirectDrawSurface4::UpdateOverlayZOrder(DWORD x, DX::LPDIRECTDRAWSURFACE4 y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::UpdateOverlayZOrder()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)
	
	auto p = DDRAW::is_proxy<IDirectDrawSurface4>(y);
	
	if(p) y = p->proxy;

	return proxy->UpdateOverlayZOrder(x,y);
}									   
HRESULT DDRAW::IDirectDrawSurface4::GetDDInterface(LPVOID *x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::GetDDInterface()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	return proxy->GetDDInterface(x);
}
HRESULT DDRAW::IDirectDrawSurface4::PageLock(DWORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::PageLock()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	return proxy->PageLock(x);
}
HRESULT DDRAW::IDirectDrawSurface4::PageUnlock(DWORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::PageUnlock()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	return proxy->PageUnlock(x);
}										
HRESULT DDRAW::IDirectDrawSurface4::SetSurfaceDesc(DX::LPDDSURFACEDESC2 x, DWORD y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::SetSurfaceDesc()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	assert(0); //unimplemented

	return proxy->SetSurfaceDesc(x,y);
}										
HRESULT DDRAW::IDirectDrawSurface4::SetPrivateData(REFGUID x, LPVOID y, DWORD z, DWORD w)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::SetPrivateData()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)
																		   	
	if(z) DDRAW_LEVEL(7) << "HEADSUP! SetPrivateData was called...\n";

	return proxy->SetPrivateData(x,y,z,w);
}
HRESULT DDRAW::IDirectDrawSurface4::GetPrivateData(REFGUID x, LPVOID y, LPDWORD z)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::GetPrivateData()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	if(z) DDRAW_LEVEL(7) << "HEADSUP! GetPrivateData was called...\n";

	return proxy->GetPrivateData(x,y,z);
}
HRESULT DDRAW::IDirectDrawSurface4::FreePrivateData(REFGUID x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::FreePrivateData()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	if(1) DDRAW_LEVEL(7) << "HEADSUP! FreetPrivateData was called...\n";

	return proxy->FreePrivateData(x);
}
HRESULT DDRAW::IDirectDrawSurface4::GetUniquenessValue(LPDWORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::GetUniquenessValue()\n";
 
	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	return proxy->GetUniquenessValue(x);
}
HRESULT DDRAW::IDirectDrawSurface4::ChangeUniquenessValue()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface4::ChangeUniquenessValue()\n";

	DDRAW_IF_NOT__DX6_RETURN(!DD_OK)

	return proxy->ChangeUniquenessValue();
}										  






DDRAW::IDirectDrawSurface7 *DDRAW::IDirectDrawSurface7::get_head = 0; //static 

HRESULT DDRAW::IDirectDrawSurface7::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::QueryInterface()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	if(7<=DDRAWLOG::debug)
	{
		LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';
	}
	
	if(riid==DX::IID_IDirectDrawGammaControl) //{69C11C3E-B46B-11D1-AD7A-00C04FC29B4E}
	{
		DDRAW_LEVEL(7) << "(IDirectDrawGammaControl)\n";

		if(DDRAW::doIDirectDrawGammaControl)
		{
			if(query->gamma)			
			{
				*ppvObj = query->gamma; query->gamma->AddRef(); 
				
				return S_OK;
			}

			::IDirectDrawGammaControl *q = 0; 

			if(DDRAW::doGamma)
			{
				HRESULT out = proxy->QueryInterface(riid,(LPVOID*)&q); 
				
				if(out!=S_OK) q = 0; //don't panic! assuming not in fullscreen mode
			}
										
			DDRAW::IDirectDrawGammaControl *p = new DDRAW::IDirectDrawGammaControl('dx7a');

			p->proxy7 = q; query->gamma = p; p->surface = query; 
			
			*ppvObj = p; p->AddRef(); 
			
			return S_OK;
		}
	}
	else if(riid==DX::IID_IDirectDrawSurface7) //(06675a80-3b9b-11d2-b92f-00609797ea5b)
	{
		DDRAW_LEVEL(7) << "(IDirectDrawSurface7)\n";		

		AddRef(); return S_OK;
	}
	else if(riid==DX::IID_IDirectDrawSurface //(6C14DB81-A733-11CE-A521-0020AF0BE560)
		  ||riid==DX::IID_IDirectDrawSurface4) //(0B2B8630-AD35-11D0-8EA6-00609797EA5B)
	{
		DDRAW_LEVEL(7) 
		<< " (IDirectDrawSurface" 
		<< dx%(riid==DX::IID_IDirectDrawSurface4?"4":"") 
		<< ")\n";		

		if(riid==DX::IID_IDirectDrawSurface&&query->dxf)
		{
			query->dxf->AddRef(); *ppvObj = query->dxf;	return S_OK;
		}
		else if(riid==DX::IID_IDirectDrawSurface4&&query->dx4)
		{
			query->dx4->AddRef(); *ppvObj = query->dx4;	return S_OK;
		}

		void *q = 0; HRESULT out = 
		proxy->QueryInterface(riid,(LPVOID*)&q); 
		
		if(out!=S_OK)
		{ 
			DDRAW_LEVEL(7) << "QueryInterface FAILED\n"; 
			DDRAW_RETURN(out)  
		}

		if(riid==DX::IID_IDirectDrawSurface&&DDRAW::doIDirectDrawSurface)
		{	
			::IDirectDrawSurface *dds = (::IDirectDrawSurface*)q;
			
			DDRAW::IDirectDrawSurface *p = new DDRAW::IDirectDrawSurface('dx7a',query);

			*ppvObj = query->dxf = p; p->proxy7 = dds; 
		}
		else if(riid==DX::IID_IDirectDrawSurface4&&DDRAW::doIDirectDrawSurface4)
		{	
			::IDirectDrawSurface4 *dds4 = (::IDirectDrawSurface4*)q;
			
			DDRAW::IDirectDrawSurface4 *p = new DDRAW::IDirectDrawSurface4('dx7a',query);

			*ppvObj = query->dx4 = p; p->proxy7 = dds4; 
		}
		else *ppvObj = q;

		DDRAW_RETURN(out) 
	}
	else assert(0);

	HRESULT out = proxy->QueryInterface(riid,ppvObj);
	DDRAW_RETURN(out) 
}
ULONG DDRAW::IDirectDrawSurface7::AddRef()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::AddRef()\n";

	DDRAW_ADDREF_RETURN(0)
}
ULONG DDRAW::IDirectDrawSurface7::Release()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::Release()\n";

	DDRAW_IF_NOT_TARGET_AND_RELEASE(out,0)

	//REMOVE ME?
	//if(out==1&&query->isPrimary)
	//return dx_ddraw_try_primarysurface7_release();

	if(out==0)
	{
		if(DDRAW::TextureSurface7[0]==this)
		{
			DDRAW::TextureSurface7[0] = 0;			
		}
		if(DDRAW::PrimarySurface7==this)
		{			
			if(DDRAW::BackBufferSurface7)
			DDRAW::BackBufferSurface7->Release();

			DDRAW::PrimarySurface7 = 0;	
		}
		else if(DDRAW::BackBufferSurface7==this)
		{
			DDRAW::BackBufferSurface7 = 0;
		}
		else if(DDRAW::DepthStencilSurface7==this)
		{
			DDRAW::DepthStencilSurface7 = 0;
		}
		delete this; 
	}
	return out;
}
HRESULT DDRAW::IDirectDrawSurface7::AddAttachedSurface(DX::LPDIRECTDRAWSURFACE7 x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::AddAttachedSurface()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)
	
	auto p = DDRAW::is_proxy<IDirectDrawSurface7>(x);
	
	if(p) x = p->proxy;
		
	HRESULT out = proxy->AddAttachedSurface(x);	

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDrawSurface7::AddOverlayDirtyRect(LPRECT x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::AddOverlayDirtyRect()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->AddOverlayDirtyRect(x);
}
HRESULT DDRAW::IDirectDrawSurface7::Blt(LPRECT x, DX::LPDIRECTDRAWSURFACE7 y, LPRECT z, DWORD w, DX::LPDDBLTFX q)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::Blt()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	DDRAW_PUSH_HACK(DIRECTDRAWSURFACE7_BLT,IDirectDrawSurface7*,
	LPRECT&,DX::LPDIRECTDRAWSURFACE7&,LPRECT&,DWORD&,DX::LPDDBLTFX&)(0,this,x,y,z,w,q);

	if(x&&IsRectEmpty(x)) DDRAW_FINISH(DD_OK); //hack: allow short circuiting

	DDRAW_LEVEL(7) << "source is " << (UINT)y << "(" << (UINT)z << ")\n";

#ifdef _DEBUG
#define OUT(x) if(w&x) DDRAW_LEVEL(7) << ' ' << #x << '\n';

	OUT(DDBLT_ALPHADEST)
	OUT(DDBLT_ALPHADESTCONSTOVERRIDE)
	OUT(DDBLT_ALPHADESTNEG)
	OUT(DDBLT_ALPHADESTSURFACEOVERRIDE)
	OUT(DDBLT_ALPHAEDGEBLEND)
	OUT(DDBLT_ALPHASRC)
	OUT(DDBLT_ALPHASRCCONSTOVERRIDE)
	OUT(DDBLT_ALPHASRCNEG)
	OUT(DDBLT_ALPHASRCSURFACEOVERRIDE)
	OUT(DDBLT_ASYNC)
	OUT(DDBLT_COLORFILL)
	OUT(DDBLT_DDFX)
	OUT(DDBLT_DDROPS)
	OUT(DDBLT_KEYDEST)
	OUT(DDBLT_KEYDESTOVERRIDE)
	OUT(DDBLT_KEYSRC)
	OUT(DDBLT_KEYSRCOVERRIDE)
	OUT(DDBLT_ROP)
	OUT(DDBLT_ROTATIONANGLE)
	OUT(DDBLT_ZBUFFER)
	OUT(DDBLT_ZBUFFERDESTCONSTOVERRIDE)
	OUT(DDBLT_ZBUFFERDESTOVERRIDE)
	OUT(DDBLT_ZBUFFERSRCCONSTOVERRIDE)
	OUT(DDBLT_ZBUFFERSRCOVERRIDE)
	OUT(DDBLT_WAIT)
	OUT(DDBLT_DEPTHFILL)
	OUT(DDBLT_DONOTWAIT)    

#undef OUT
#endif

	if(q)
	{
		DDRAW_LEVEL(7) << "PANIC! LPDDBLTFX was passed...\n";

		DDRAW::log_surface_bltfx(q);
	}		

	RECT dst;
	if(x&&!DDRAW::doNothing) 
	{
		DX::DDSCAPS2 caps; proxy->GetCaps(&caps);

		if(caps.dwCaps&(DDSCAPS_BACKBUFFER
		 |DDSCAPS_FRONTBUFFER
		 |DDSCAPS_PRIMARYSURFACE
		 |DDSCAPS_3DDEVICE))
		{
			dst = *x; 

			if(DDRAW::doScaling) 
			{
				dst.top = DDRAW::xyScaling[1]*x->top;
				dst.left = DDRAW::xyScaling[0]*x->left;
				dst.right = DDRAW::xyScaling[0]*x->right;
				dst.bottom = DDRAW::xyScaling[1]*x->bottom;
			}
			if(DDRAW::doMapping) 
			{
				dst.top+=DDRAW::xyMapping[1];
				dst.left+=DDRAW::xyMapping[0];
				dst.right+=DDRAW::xyMapping[0];
				dst.bottom+=DDRAW::xyMapping[1];
			}			

			x = &dst;
		}
	}

	bool present = true; DWORD ticks_before_wait;

	//RECT fill
	RECT src; if(query->isPrimary) //assuming "swapping" buffers
	{
		DDRAW_LEVEL(7) << " Destination is primary surface\n";

		if(z&&!DDRAW::doNothing) 
		{
			src = *z;

			if(DDRAW::doScaling) 
			{
				src.top = DDRAW::xyScaling[1]*z->top;
				src.left = DDRAW::xyScaling[0]*z->left;
				src.right = DDRAW::xyScaling[0]*z->right;
				src.bottom = DDRAW::xyScaling[1]*z->bottom;
			}
			if(DDRAW::doMapping) 
			{
				src.top+=DDRAW::xyMapping[1];
				src.left+=DDRAW::xyMapping[0];
				src.right+=DDRAW::xyMapping[0];
				src.bottom+=DDRAW::xyMapping[1];
			}	

			z = &src;
		}

		/*2018: I think was a mistake all along...
		//SetCooperativeLevel receives the top-level
		//window, according to its doc
		if(!DDRAW::fullscreen) //if(DDRAW::inWindow)
		{
			if(GetWindowRect(DDRAW::window,&fill))
			{
				if(x)
				{
					fill.right = x->right-x->left+fill.left;
					fill.bottom = x->bottom-x->top+fill.top;					
				}

				x = &fill;
			}
		}*/

		DDRAW::onEffects();

		if(DDRAW::isPaused) DDRAW::onPause();

		present = DDRAW::onFlip(); 

		ticks_before_wait = DX::tick();
		 		
		if(DDRAW::doWait&&DDRAW::DirectDraw7)
		{
			DDRAW::DirectDraw7->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN,0);
		}
	}

	out = present?!DD_OK:DD_OK;

	if(!y) 
	{
		if(present) out = proxy->Blt(x,y,z,w,q); 
		
		goto out;
	}

	auto p = DDRAW::is_proxy<IDirectDrawSurface7>(y);
	
	if(p&&p->query->isPrimary) 
	DDRAW_LEVEL(7) << " Source is primary surface\n";

	if(p) y = p->proxy; 
	
	if(present) out = proxy->Blt(x,y,z,w,q);

out:if(out!=DD_OK)
	{
		DDRAW_LEVEL(7) << "Blt FAILED\n";	
	}
	else if(present) DDRAW::flipTicks = DX::tick()-ticks_before_wait;

	DDRAW_POP_HACK(DIRECTDRAWSURFACE7_BLT,IDirectDrawSurface7*,
	LPRECT&,DX::LPDIRECTDRAWSURFACE7&,LPRECT&,DWORD&,DX::LPDDBLTFX&)(&out,this,x,y,z,w,q);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDrawSurface7::BltBatch(DX::LPDDBLTBATCH x, DWORD y, DWORD z)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::BltBatch()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	if(x) DDRAW_LEVEL(7) << "PANIC! LPDDBLTBATCH was passed...\n";

	return proxy->BltBatch(x,y,z);
}
HRESULT DDRAW::IDirectDrawSurface7::BltFast(DWORD x, DWORD y, DX::LPDIRECTDRAWSURFACE7 z, LPRECT w, DWORD q)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::BltFast()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)
	
#ifdef _DEBUG
#define OUT(x) if(q&x) DDRAW_LEVEL(7) << ' ' << #x << '\n';

	OUT(DDBLTFAST_DESTCOLORKEY)
	OUT(DDBLTFAST_NOCOLORKEY)
	OUT(DDBLTFAST_SRCCOLORKEY)
	OUT(DDBLTFAST_WAIT)

#undef OUT
#endif

	if(w&&!DDRAW::doNothing
	  &&(DDRAW::xyMapping[0]||DDRAW::xyScaling[0]!=1.0f
	   ||DDRAW::xyMapping[1]||DDRAW::xyScaling[1]!=1.0f)) 
	{
		DX::DDSCAPS2 caps; proxy->GetCaps(&caps);

		if(caps.dwCaps&DDSCAPS_BACKBUFFER) //need to change to normal blt
		{	
			DWORD flags = 0x00000000;

			if(q&DDBLTFAST_WAIT)         flags|=DDBLT_WAIT;
			if(q&DDBLTFAST_DESTCOLORKEY) flags|=DDBLT_KEYDEST;
			if(q&DDBLTFAST_SRCCOLORKEY)  flags|=DDBLT_KEYSRC;

			RECT dst = { x, y, 0, 0 };

			auto p = DDRAW::is_proxy<IDirectDrawSurface7>(z);

			if(!p) DDRAW_RETURN(!DD_OK)

			RECT src; if(!w)   
			{
				DX::DDSURFACEDESC2 desc; desc.dwSize = sizeof(DX::DDSURFACEDESC2);

				if(p->proxy->GetSurfaceDesc(&desc)!=D3D_OK) DDRAW_RETURN(!DD_OK)

				src.left = src.top = 0;
				src.right = desc.dwWidth;
				src.bottom = desc.dwHeight;

				w = &src;
			}

			dst.right = dst.left+(w->right-w->left);
			dst.bottom = dst.top+(w->bottom-w->top);

			return Blt(&dst,z,w,flags,0);
		}
	}

	auto p = DDRAW::is_proxy<IDirectDrawSurface7>(z);
	
	if(p) z = p->proxy;

	HRESULT out = proxy->BltFast(x,y,z,w,q);

	if(out!=DD_OK)
	{
		DDRAW_LEVEL(7) << "BltFast FAILED\n";	
	}

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDrawSurface7::DeleteAttachedSurface(DWORD x, DX::LPDIRECTDRAWSURFACE7 y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::DeleteAttachedSurface()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)
	
	auto p = DDRAW::is_proxy<IDirectDrawSurface7>(y);
	
	if(p) y = p->proxy;

	return proxy->DeleteAttachedSurface(x,y);
}
HRESULT DDRAW::IDirectDrawSurface7::EnumAttachedSurfaces(LPVOID x, DX::LPDDENUMSURFACESCALLBACK7 y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::EnumAttachedSurfaces()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	if(y) DDRAW_LEVEL(7) << "PANIC! LPDDENUMSURFACESCALLBACK7 was passed...\n";

	return proxy->EnumAttachedSurfaces(x,y);
}
HRESULT DDRAW::IDirectDrawSurface7::EnumOverlayZOrders(DWORD x, LPVOID y, DX::LPDDENUMSURFACESCALLBACK7 z)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::EnumOverlayZOrders()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	if(z) DDRAW_LEVEL(7) << "PANIC! LPDDENUMSURFACESCALLBACK7 was passed...\n";

	return proxy->EnumOverlayZOrders(x,y,z);
}
HRESULT DDRAW::IDirectDrawSurface7::Flip(DX::LPDIRECTDRAWSURFACE7 x, DWORD y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::Flip() " << DDRAW::noFlips << '\n';

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)
		
	DDRAW_PUSH_HACK(DIRECTDRAWSURFACE7_FLIP,IDirectDrawSurface7*,
	DX::LPDIRECTDRAWSURFACE7&,DWORD&)(0,this,x,y);
		
	auto p = DDRAW::is_proxy<IDirectDrawSurface7>(x);
	
	if(p) x = p->proxy;

	if(DDRAW::isPaused) DDRAW::onPause();

	bool present = DDRAW::onFlip();

	DWORD ticks_before_wait = DX::tick();

	//Why were their 3 exactly?
	//if(!DDRAW::inWindow&&DDRAW::isExclusive)
	if(DDRAW::fullscreen)
	{	
		if(present)
		{
			DDRAW_FINISH(proxy->Flip(x,y));

			DDRAW_FINISH(out) 
		}
		else DDRAW_FINISH(DD_OK) 
	}

	//C4533
	{
		RECT z = {0,0,640,480};	
		DDRAW::window_rect(&z); 
	
		RECT fill = z;

		GetWindowRect(DDRAW::window,&fill);
		
		if(DDRAW::doWait&&DDRAW::DirectDraw7)
		{
			DDRAW::DirectDraw7->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN,0);
		}

		if(present)
		{
			out = DDRAW::PrimarySurface7->proxy7->
			Blt(&fill,DDRAW::BackBufferSurface7->proxy7,&z,DDBLT_WAIT,0);
		}
		else out = DD_OK;
	}

	switch(out)
	{
	case DD_OK: break;

	case DDERR_SURFACELOST: assert(0); break;

	default: break;
	}	

	if(!out&&present) DDRAW::flipTicks = DX::tick()-ticks_before_wait;
	  	
	DDRAW_POP_HACK(DIRECTDRAWSURFACE7_FLIP,IDirectDrawSurface7*,
	DX::LPDIRECTDRAWSURFACE7&,DWORD&)(&out,this,x,y);
		
	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDrawSurface7::GetAttachedSurface(DX::LPDDSCAPS2 x, DX::LPDIRECTDRAWSURFACE7 *y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::GetAttachedSurface()\n";				 

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

#ifdef _DEBUG	 
#define OUT(X) if(x->dwCaps&X) DDRAW_LEVEL(7) << ' ' << #X << '\n';

	OUT(DDSCAPS_ALPHA)
	OUT(DDSCAPS_BACKBUFFER)
	OUT(DDSCAPS_COMPLEX)
	OUT(DDSCAPS_FLIP)
	OUT(DDSCAPS_FRONTBUFFER)
	OUT(DDSCAPS_OFFSCREENPLAIN)
	OUT(DDSCAPS_OVERLAY)
	OUT(DDSCAPS_PALETTE)
	OUT(DDSCAPS_PRIMARYSURFACE)
	OUT(DDSCAPS_SYSTEMMEMORY)
	OUT(DDSCAPS_TEXTURE)
	OUT(DDSCAPS_VIDEOMEMORY)
	OUT(DDSCAPS_VISIBLE)
	OUT(DDSCAPS_WRITEONLY)
	OUT(DDSCAPS_ZBUFFER)
	OUT(DDSCAPS_OWNDC)
	OUT(DDSCAPS_HWCODEC)
	OUT(DDSCAPS_MIPMAP)	 
	OUT(DDSCAPS_ALLOCONLOAD)
	OUT(DDSCAPS_LOCALVIDMEM)
	OUT(DDSCAPS_NONLOCALVIDMEM)
	OUT(DDSCAPS_STANDARDVGAMODE)
	OUT(DDSCAPS_OPTIMIZED)

#undef OUT
#endif

	HRESULT out = !DD_OK;

	if(x->dwCaps&DDSCAPS_BACKBUFFER)
	{	
		if(y&&DDRAW::BackBufferSurface7) 
		{
			DDRAW::BackBufferSurface7->AddRef();

			*y = DDRAW::BackBufferSurface7; 

			return DD_OK; //DDRAW_FINISH(DD_OK)
		}
		else out = !DD_OK;
	}
	else out = proxy->GetAttachedSurface(x,y);
		
	if(out==DD_OK)
	{
		//if(DDRAW::inWindow) assert(0);

		DDRAW::IDirectDrawSurface7 *p = get(*y);

		if(!p) p = new DDRAW::IDirectDrawSurface7('dx7a'); p->proxy = *y;

		*y = (DX::IDirectDrawSurface7*)p; 
	}
	else
	{
		//2021: previously wasn't using DDRAW_RETURN I think
		bool mip = x->dwCaps==(DDSCAPS_MIPMAP|DDSCAPS_TEXTURE);

		if(!mip||!DDRAW::doMipmaps)
		DDRAW_LEVEL(7) << "GetAttachedSurface FAILED\n";

		if(mip) return DDERR_NOMIPMAPHW;
	}
	
	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDrawSurface7::GetBltStatus(DWORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::GetBltStatus()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->GetBltStatus(x);
}
HRESULT DDRAW::IDirectDrawSurface7::GetCaps(DX::LPDDSCAPS2 x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::GetCaps()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->GetCaps(x);
}
HRESULT DDRAW::IDirectDrawSurface7::GetClipper(DX::LPDIRECTDRAWCLIPPER *x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::GetClipper()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

		assert(0); //2021: DDRAW::IDirectDrawClipper?

	return proxy->GetClipper(x);
}
HRESULT DDRAW::IDirectDrawSurface7::GetColorKey(DWORD x, DX::LPDDCOLORKEY y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::GetColorKey()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->GetColorKey(x,y);
}
HRESULT DDRAW::IDirectDrawSurface7::GetDC(HDC *x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::GetDC()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	HRESULT out = proxy->GetDC(x);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDrawSurface7::GetFlipStatus(DWORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::GetFlipStatus()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->GetFlipStatus(x);
}
HRESULT DDRAW::IDirectDrawSurface7::GetOverlayPosition(LPLONG x, LPLONG y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::GetOverlayPosition()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->GetOverlayPosition(x,y);
}
HRESULT DDRAW::IDirectDrawSurface7::GetPalette(DX::LPDIRECTDRAWPALETTE *x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::GetPalette()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->GetPalette(x);
}
HRESULT DDRAW::IDirectDrawSurface7::GetPixelFormat(DX::LPDDPIXELFORMAT x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::GetPixelFormat()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->GetPixelFormat(x);
}
HRESULT DDRAW::IDirectDrawSurface7::GetSurfaceDesc(DX::LPDDSURFACEDESC2 x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::GetSurfaceDesc()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	HRESULT out = proxy->GetSurfaceDesc(x);
	
	if(out!=DD_OK) DDRAW_RETURN(out) 

#ifdef _DEBUG
#define OUT(X) if(x->dwFlags&X) DDRAW_LEVEL(7) << ' ' << #X << '\n';

	OUT(DDSD_CAPS)
	OUT(DDSD_HEIGHT)
	OUT(DDSD_WIDTH)
	OUT(DDSD_PITCH)
	OUT(DDSD_BACKBUFFERCOUNT)
	OUT(DDSD_ZBUFFERBITDEPTH)
	OUT(DDSD_ALPHABITDEPTH)
	OUT(DDSD_LPSURFACE)
	OUT(DDSD_PIXELFORMAT)
	OUT(DDSD_CKDESTOVERLAY)
	OUT(DDSD_CKDESTBLT)
	OUT(DDSD_CKSRCOVERLAY)
	OUT(DDSD_CKSRCBLT)
	OUT(DDSD_REFRESHRATE)
	OUT(DDSD_LINEARSIZE)
	OUT(DDSD_TEXTURESTAGE)
	OUT(DDSD_FVF)
	OUT(DDSD_DEPTH)

#define OUT(X) if(x->ddsCaps.dwCaps&X) DDRAW_LEVEL(7) << ' ' << #X << '\n';

	OUT(DDSCAPS_ALPHA)
	OUT(DDSCAPS_BACKBUFFER)
	OUT(DDSCAPS_COMPLEX)
	OUT(DDSCAPS_FLIP)
	OUT(DDSCAPS_FRONTBUFFER)
	OUT(DDSCAPS_OFFSCREENPLAIN)
	OUT(DDSCAPS_OVERLAY)
	OUT(DDSCAPS_PALETTE)
	OUT(DDSCAPS_PRIMARYSURFACE)
	OUT(DDSCAPS_SYSTEMMEMORY)
	OUT(DDSCAPS_TEXTURE)
	OUT(DDSCAPS_VIDEOMEMORY)
	OUT(DDSCAPS_VISIBLE)
	OUT(DDSCAPS_WRITEONLY)
	OUT(DDSCAPS_ZBUFFER)
	OUT(DDSCAPS_OWNDC)
	OUT(DDSCAPS_HWCODEC)
	OUT(DDSCAPS_MIPMAP)	 
	OUT(DDSCAPS_ALLOCONLOAD)
	OUT(DDSCAPS_LOCALVIDMEM)
	OUT(DDSCAPS_NONLOCALVIDMEM)
	OUT(DDSCAPS_STANDARDVGAMODE)
	OUT(DDSCAPS_OPTIMIZED)

#undef OUT
#endif

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDrawSurface7::Initialize(DX::LPDIRECTDRAW x, DX::LPDDSURFACEDESC2 y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::Initialize()\n"; 

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	if(1) DDRAW_LEVEL(7) << "PANIC! Initialize() was called...\n";
						 
	return proxy->Initialize(x,y);
}
HRESULT DDRAW::IDirectDrawSurface7::IsLost()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::IsLost()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->IsLost();
}
HRESULT DDRAW::IDirectDrawSurface7::Lock(LPRECT x, DX::LPDDSURFACEDESC2 y, DWORD z, HANDLE w)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::Lock()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->Lock(x,y,z,w);
}
HRESULT DDRAW::IDirectDrawSurface7::ReleaseDC(HDC x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::ReleaseDC()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	HRESULT out = proxy->ReleaseDC(x); DDRAW_RETURN(out)
}
HRESULT DDRAW::IDirectDrawSurface7::Restore()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::Restore()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	HRESULT out = proxy->Restore(); DDRAW_RETURN(out)
}
HRESULT DDRAW::IDirectDrawSurface7::SetClipper(DX::LPDIRECTDRAWCLIPPER x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::SetClipper()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	HRESULT out = proxy->SetClipper(x); DDRAW_RETURN(out)
}
HRESULT DDRAW::IDirectDrawSurface7::SetColorKey(DWORD x, DX::LPDDCOLORKEY y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::SetColorKey()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)
	
	DDRAW_PUSH_HACK(DIRECTDRAWSURFACE7_SETCOLORKEY,IDirectDrawSurface7*,
	DWORD&,DX::LPDDCOLORKEY&)(0,this,x,y);	
	
	out = proxy->SetColorKey(x,y);

	DDRAW_POP_HACK(DIRECTDRAWSURFACE7_SETCOLORKEY,IDirectDrawSurface7*,
	DWORD&,DX::LPDDCOLORKEY&)(&out,this,x,y);	

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirectDrawSurface7::SetOverlayPosition(LONG x, LONG y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::SetOverlayPosition()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->SetOverlayPosition(x,y);
}
HRESULT DDRAW::IDirectDrawSurface7::SetPalette(DX::LPDIRECTDRAWPALETTE x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::SetPalette()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->SetPalette(x);
}
HRESULT DDRAW::IDirectDrawSurface7::Unlock(LPRECT x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::Unlock()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->Unlock(x);
}
HRESULT DDRAW::IDirectDrawSurface7::UpdateOverlay(LPRECT x, DX::LPDIRECTDRAWSURFACE7 y, LPRECT z, DWORD w, DX::LPDDOVERLAYFX q)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::UpdateOverlay()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	if(q) DDRAW_LEVEL(7) << "PANIC! LPDDOVERLAYFX was passed...\n";
	
	auto p = DDRAW::is_proxy<IDirectDrawSurface7>(y);
	
	if(p) y = p->proxy;

	return proxy->UpdateOverlay(x,y,z,w,q);
}
HRESULT DDRAW::IDirectDrawSurface7::UpdateOverlayDisplay(DWORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::UpdateOverlayDisplay()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->UpdateOverlayDisplay(x);
}
HRESULT DDRAW::IDirectDrawSurface7::UpdateOverlayZOrder(DWORD x, DX::LPDIRECTDRAWSURFACE7 y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::UpdateOverlayZOrder()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)
		
	auto p = DDRAW::is_proxy<IDirectDrawSurface7>(y);
	
	if(p) y = p->proxy;

	return proxy->UpdateOverlayZOrder(x,y);
}									   
HRESULT DDRAW::IDirectDrawSurface7::GetDDInterface(LPVOID *x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::GetDDInterface()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->GetDDInterface(x);
}
HRESULT DDRAW::IDirectDrawSurface7::PageLock(DWORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::PageLock()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->PageLock(x);
}
HRESULT DDRAW::IDirectDrawSurface7::PageUnlock(DWORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::PageUnlock()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->PageUnlock(x);
}										
HRESULT DDRAW::IDirectDrawSurface7::SetSurfaceDesc(DX::LPDDSURFACEDESC2 x, DWORD y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::SetSurfaceDesc()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->SetSurfaceDesc(x,y);
}										
HRESULT DDRAW::IDirectDrawSurface7::SetPrivateData(REFGUID x, LPVOID y, DWORD z, DWORD w)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::SetPrivateData()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)
																		   	
	if(z) DDRAW_LEVEL(7) << "HEADSUP! SetPrivateData was called...\n";

	return proxy->SetPrivateData(x,y,z,w);
}
HRESULT DDRAW::IDirectDrawSurface7::GetPrivateData(REFGUID x, LPVOID y, LPDWORD z)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::GetPrivateData()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	if(z) DDRAW_LEVEL(7) << "HEADSUP! GetPrivateData was called...\n";

	return proxy->GetPrivateData(x,y,z);
}
HRESULT DDRAW::IDirectDrawSurface7::FreePrivateData(REFGUID x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::FreePrivateData()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	if(1) DDRAW_LEVEL(7) << "HEADSUP! FreetPrivateData was called...\n";

	return proxy->FreePrivateData(x);
}
HRESULT DDRAW::IDirectDrawSurface7::GetUniquenessValue(LPDWORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::GetUniquenessValue()\n";
 
	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->GetUniquenessValue(x);
}
HRESULT DDRAW::IDirectDrawSurface7::ChangeUniquenessValue()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::ChangeUniquenessValue()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->ChangeUniquenessValue();
}										  
HRESULT DDRAW::IDirectDrawSurface7::SetPriority(DWORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::SetPriority()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->SetPriority(x);
}
HRESULT DDRAW::IDirectDrawSurface7::GetPriority(LPDWORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::GetPriority()\n";
 
	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->GetPriority(x);
}
HRESULT DDRAW::IDirectDrawSurface7::SetLOD(DWORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::SetLOD()\n";
 
	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->SetLOD(x);
}
HRESULT DDRAW::IDirectDrawSurface7::GetLOD(LPDWORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::GetLOD()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!DD_OK)

	return proxy->GetLOD(x);
}





DDRAW::IDirect3DTexture2 *DDRAW::IDirect3DTexture2::get_head = 0; //static

HRESULT DDRAW::IDirect3DTexture2::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DTexture2::QueryInterface()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	if(7<=DDRAWLOG::debug)
	{
		LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';
	}

	assert(0); return !S_OK;
}
ULONG DDRAW::IDirect3DTexture2::AddRef() 
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DTexture2::AddRef()\n";

	DDRAW_ADDREF_RETURN(0)
}
ULONG DDRAW::IDirect3DTexture2::Release()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DTexture2::Release()\n";

	DDRAW_IF_NOT_TARGET_AND_RELEASE(out,0)

	if(out==0)
	{
		if(target=='_dx6')
		{
			assert(surface->dx4);

			if(surface->dx4) surface->dx4->Release();
		}

		delete this; 
	}

	return out;
}
HRESULT DDRAW::IDirect3DTexture2::GetHandle(DX::LPDIRECT3DDEVICE2 x, DX::LPD3DTEXTUREHANDLE y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DTexture2::GetHandle()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_RETURN(proxy6->GetHandle((::LPDIRECT3DDEVICE2)x,(::LPD3DTEXTUREHANDLE)y))
}
HRESULT DDRAW::IDirect3DTexture2::PaletteChanged(DWORD x, DWORD y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DTexture2::PaletteChanged()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_RETURN(proxy6->PaletteChanged(x,y))
}
HRESULT DDRAW::IDirect3DTexture2::Load(DX::LPDIRECT3DTEXTURE2 x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DTexture2::Load()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	auto p = DDRAW::is_proxy<IDirect3DTexture2>(x);

	DDRAW_RETURN(proxy6->Load(p?p->proxy6:(::LPDIRECT3DTEXTURE2)x))
}







HRESULT DDRAW::IDirect3DLight::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DLight::QueryInterface()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	if(7<=DDRAWLOG::debug)
	{
		LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';
	}

	assert(0); return !S_OK;
}
ULONG DDRAW::IDirect3DLight::AddRef() 
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DLight::AddRef()\n";

	DDRAW_ADDREF_RETURN(0)
}
ULONG DDRAW::IDirect3DLight::Release()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DLight::Release()\n";

	DDRAW_IF_NOT_TARGET_AND_RELEASE(out,0)

	if(out==0) delete this; 

	return out;
}
HRESULT DDRAW::IDirect3DLight::Initialize(DX::LPDIRECT3D x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DLight::Initialize()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_RETURN(proxy6->Initialize((::LPDIRECT3D)x))
}
HRESULT DDRAW::IDirect3DLight::SetLight(DX::LPD3DLIGHT x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DLight::SetLight()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_RETURN(proxy6->SetLight((::LPD3DLIGHT)x))
}
HRESULT DDRAW::IDirect3DLight::GetLight(DX::LPD3DLIGHT x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DLight::GetLight()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_RETURN(proxy6->GetLight((::LPD3DLIGHT)x))
}





HRESULT DDRAW::IDirect3DMaterial3::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DMaterial3::QueryInterface()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	if(7<=DDRAWLOG::debug)
	{
		LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';
	}

	assert(0); return !S_OK;
}
ULONG DDRAW::IDirect3DMaterial3::AddRef() 
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DMaterial3::AddRef()\n";

	DDRAW_ADDREF_RETURN(0)
}
ULONG DDRAW::IDirect3DMaterial3::Release()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DMaterial3::Release()\n";

	DDRAW_IF_NOT_TARGET_AND_RELEASE(out,0)

	if(out==0) delete this; 

	return out;
}
HRESULT DDRAW::IDirect3DMaterial3::SetMaterial(DX::LPD3DMATERIAL x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DMaterial3::SetMaterial()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_RETURN(proxy6->SetMaterial((::LPD3DMATERIAL)x))
}
HRESULT DDRAW::IDirect3DMaterial3::GetMaterial(DX::LPD3DMATERIAL x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DMaterial3::GetMaterial()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_RETURN(proxy6->GetMaterial((::LPD3DMATERIAL)x))
}
HRESULT DDRAW::IDirect3DMaterial3::GetHandle(DX::LPDIRECT3DDEVICE3 x, DX::LPD3DMATERIALHANDLE y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DMaterial3::GetHandle()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	auto p = DDRAW::is_proxy<IDirect3DDevice3>(x);

	DDRAW_RETURN(proxy6->GetHandle(p?p->proxy6:(::LPDIRECT3DDEVICE3)x,(::LPD3DMATERIALHANDLE)y))
}






HRESULT DDRAW::IDirect3DViewport3::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DViewport3::QueryInterface()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	if(7<=DDRAWLOG::debug)
	{
		LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';
	}

	assert(0); return !S_OK;
}
ULONG DDRAW::IDirect3DViewport3::AddRef()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DViewport3::AddRef()\n";

	DDRAW_ADDREF_RETURN(0)
}
ULONG DDRAW::IDirect3DViewport3::Release()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DViewport3::Release()\n";

	DDRAW_IF_NOT_TARGET_AND_RELEASE(out,0)

	if(out==0) delete this; 

	return out;
}
HRESULT DDRAW::IDirect3DViewport3::Initialize(DX::LPDIRECT3D)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DViewport3::Initialize()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	assert(0); //unimplemented

	return !S_OK;
}
HRESULT DDRAW::IDirect3DViewport3::GetViewport(DX::LPD3DVIEWPORT x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DViewport3::GetViewport()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	if(target=='_dx6') //???
	{
		HRESULT out = proxy6->GetViewport((::LPD3DVIEWPORT)x);

		if(x&&out==D3D_OK&&!DDRAW::doNothing) //2021: dx_DPI_testing_2021
		{
			if(DDRAW::doMapping) x->dwX = x->dwX-DDRAW::xyMapping[0];
			if(DDRAW::doMapping) x->dwY = x->dwY-DDRAW::xyMapping[1];

			if(DDRAW::doScaling) x->dwWidth = float(x->dwWidth)/DDRAW::xyScaling[0];
			if(DDRAW::doScaling) x->dwHeight = float(x->dwHeight)/DDRAW::xyScaling[1];
		}

		DDRAW_RETURN(out)
	}
		assert(0); //2021: WHAT IS THIS CODE BELOW???

	if(x&&x->dwSize==sizeof(D3DVIEWPORT)) 
	{
		memcpy(x,&dims,sizeof(D3DVIEWPORT)); return S_OK; 
	}
	else assert(0); return !S_OK;
}
HRESULT DDRAW::IDirect3DViewport3::SetViewport(DX::LPD3DVIEWPORT x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DViewport3::SetViewport()\n";
			
	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DX::D3DVIEWPORT temp; if(x&&!DDRAW::doNothing) //2021: dx_DPI_testing_2021
	{
		temp = *x;

		if(DDRAW::doMapping) temp.dwX+=DDRAW::xyMapping[0];
		if(DDRAW::doMapping) temp.dwY+=DDRAW::xyMapping[1];

		if(DDRAW::doScaling) temp.dwWidth*=DDRAW::xyScaling[0];
		if(DDRAW::doScaling) temp.dwHeight*=DDRAW::xyScaling[1];

		x = &temp; 
	}

	if(target=='_dx6') //???
	DDRAW_RETURN(proxy6->SetViewport((::LPD3DVIEWPORT)x))

		assert(0); //2021: WHAT IS THIS CODE BELOW???

	if(x&&x->dwSize==sizeof(D3DVIEWPORT)) 
	{
		memcpy(&dims,x,sizeof(D3DVIEWPORT)); return S_OK; 
	}
	else assert(0); return !S_OK;
}
HRESULT DDRAW::IDirect3DViewport3::TransformVertices(DWORD x, DX::LPD3DTRANSFORMDATA y, DWORD z, LPDWORD w)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DViewport3::TransformVertices()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_RETURN(proxy6->TransformVertices(x,(::LPD3DTRANSFORMDATA)y,z,w))
}
HRESULT DDRAW::IDirect3DViewport3::LightElements(DWORD x, DX::LPD3DLIGHTDATA y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DViewport3::LightElements()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_RETURN(proxy6->LightElements(x,(::LPD3DLIGHTDATA)y))
}
HRESULT DDRAW::IDirect3DViewport3::SetBackground(DX::D3DMATERIALHANDLE x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DViewport3::SetBackground()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_RETURN(proxy6->SetBackground((::D3DMATERIALHANDLE)x))
}
HRESULT DDRAW::IDirect3DViewport3::GetBackground(DX::LPD3DMATERIALHANDLE x, LPBOOL y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DViewport3::GetBackground()\n";
	
	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_RETURN(proxy6->GetBackground((::LPD3DMATERIALHANDLE)x,y))
}
HRESULT DDRAW::IDirect3DViewport3::SetBackgroundDepth(DX::LPDIRECTDRAWSURFACE x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DViewport3::SetBackgroundDepth()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	auto p = DDRAW::is_proxy<IDirectDrawSurface>(x);

	DDRAW_RETURN(proxy6->SetBackgroundDepth(p?p->proxy6:(::LPDIRECTDRAWSURFACE)x))
}
HRESULT DDRAW::IDirect3DViewport3::GetBackgroundDepth(DX::LPDIRECTDRAWSURFACE *x, LPBOOL y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DViewport3::GetBackgroundDepth()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	HRESULT out = proxy6->GetBackgroundDepth((::LPDIRECTDRAWSURFACE*)x,y);

	if(out==S_OK) 
	{
		DDRAW::IDirectDrawSurface *p = DDRAW::IDirectDrawSurface::get_head->get(*x);

		if(p) *x = p;
	}

	DDRAW_RETURN(out)
}
HRESULT DDRAW::IDirect3DViewport3::Clear(DWORD x, DX::LPD3DRECT y, DWORD z)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DViewport3::Clear()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	HRESULT out = S_OK;
	D3DRECT buf[4];
	if(z) for(DWORD i=0,iN=max(x,1);i<iN;i+=_countof(buf))	
	{
		D3DRECT *src;
		if(y&&!DDRAW::doNothing) //2021: dx_DPI_testing_2021
		{
			for(DWORD j=0;i<x&&j<_countof(buf);j++)
			{
				auto &temp = buf[j]; 
				
				temp = ((D3DRECT*)y)[i+j];

				if(DDRAW::doScaling)
				{
					temp.x1*=DDRAW::xyScaling[0]; temp.x2*=DDRAW::xyScaling[0]; 
					temp.y1*=DDRAW::xyScaling[1]; temp.y2*=DDRAW::xyScaling[1]; 
				}
				if(DDRAW::doMapping)
				{
					temp.x1+=DDRAW::xyMapping[0]; temp.y1+=DDRAW::xyMapping[1]; 
					temp.x2+=DDRAW::xyMapping[0]; temp.y2+=DDRAW::xyMapping[1]; 		
				}		
			}		
			src = buf;
		}
		else src = (D3DRECT*)y+i;

		auto yy = y?src:0;

		DWORD xx = y?min(x-i,_countof(buf)):0;

		out = proxy6->Clear(xx,yy,z);
	}

	DDRAW_RETURN(out)
}
HRESULT DDRAW::IDirect3DViewport3::AddLight(DX::LPDIRECT3DLIGHT x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DViewport3::AddLight()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	auto p = DDRAW::is_proxy<IDirect3DLight>(x);

	DDRAW_RETURN(proxy6->AddLight(p?p->proxy6:(::LPDIRECT3DLIGHT)x))
}
HRESULT DDRAW::IDirect3DViewport3::DeleteLight(DX::LPDIRECT3DLIGHT x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DViewport3::DeleteLight()\n";
	
	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	auto p = DDRAW::is_proxy<IDirect3DLight>(x);

	DDRAW_RETURN(proxy6->DeleteLight(p?p->proxy6:(::LPDIRECT3DLIGHT)x))
}
HRESULT DDRAW::IDirect3DViewport3::NextLight(DX::LPDIRECT3DLIGHT x, DX::LPDIRECT3DLIGHT *y, DWORD z)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DViewport3::NextLight()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_RETURN(proxy6->NextLight((::LPDIRECT3DLIGHT)x,(::LPDIRECT3DLIGHT*)y,z))
}
HRESULT DDRAW::IDirect3DViewport3::GetViewport2(DX::LPD3DVIEWPORT2 x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DViewport3::GetViewport2()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	HRESULT out = proxy6->GetViewport2((::LPD3DVIEWPORT2)x);

	if(x&&out==D3D_OK&&!DDRAW::doNothing) //2021: dx_DPI_testing_2021 
	{
		if(DDRAW::doMapping) x->dwX = x->dwX-DDRAW::xyMapping[0];
		if(DDRAW::doMapping) x->dwY = x->dwY-DDRAW::xyMapping[1];

		if(DDRAW::doScaling) x->dwWidth = float(x->dwWidth)/DDRAW::xyScaling[0];
		if(DDRAW::doScaling) x->dwHeight = float(x->dwHeight)/DDRAW::xyScaling[1];
	}

	DDRAW_RETURN(out)
}
HRESULT DDRAW::IDirect3DViewport3::SetViewport2(DX::LPD3DVIEWPORT2 x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DViewport3::SetViewport2()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_PUSH_HACK(DIRECT3DVIEWPORT3_SETVIEWPORT2,IDirect3DViewport3*,
	DX::LPD3DVIEWPORT2&)(0,this,x);
					 
	DX::D3DVIEWPORT2 temp; if(x&&!DDRAW::doNothing) //2021: dx_DPI_testing_2021
	{
		temp = *x;

		if(DDRAW::doMapping) temp.dwX+=DDRAW::xyMapping[0];
		if(DDRAW::doMapping) temp.dwY+=DDRAW::xyMapping[1];

		if(DDRAW::doScaling) temp.dwWidth*=DDRAW::xyScaling[0];
		if(DDRAW::doScaling) temp.dwHeight*=DDRAW::xyScaling[1];
		
		//temp.dvMinZ = x->dvMinZ; temp.dvMaxZ = x->dvMaxZ;

		x = &temp; 
	}

	DDRAW_FINISH(proxy6->SetViewport2((::LPD3DVIEWPORT2)x))

	assert(0); //unimplemented

	DDRAW_POP_HACK(DIRECT3DVIEWPORT3_SETVIEWPORT2,IDirect3DViewport3*,
	DX::LPD3DVIEWPORT2&x)(&out,this,x);

	DDRAW_RETURN(out)
}
HRESULT DDRAW::IDirect3DViewport3::SetBackgroundDepth2(DX::LPDIRECTDRAWSURFACE4 x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DViewport3::SetBackgroundDepth2()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	auto p = DDRAW::is_proxy<IDirectDrawSurface4>(x);

	DDRAW_RETURN(proxy6->SetBackgroundDepth2(p?p->proxy6:(::LPDIRECTDRAWSURFACE4)x))
}
HRESULT DDRAW::IDirect3DViewport3::GetBackgroundDepth2(DX::LPDIRECTDRAWSURFACE4 *x, LPBOOL y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DViewport3::GetBackgroundDepth2()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	HRESULT out = proxy6->GetBackgroundDepth2((::LPDIRECTDRAWSURFACE4*)x,y);

	if(out==S_OK) 
	{
		DDRAW::IDirectDrawSurface4 *p = DDRAW::IDirectDrawSurface4::get_head->get(*x);

		if(p) *x = p;
	}

	DDRAW_RETURN(out)
}
HRESULT DDRAW::IDirect3DViewport3::Clear2(DWORD x, DX::LPD3DRECT y, DWORD z, DX::D3DCOLOR w, DX::D3DVALUE q, DWORD r)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DViewport3::Clear2()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);
									   	
	DDRAW_PUSH_HACK(DIRECT3DVIEWPORT3_CLEAR2,IDirect3DViewport3*,
	DWORD&,DX::LPD3DRECT&,DWORD&,DX::D3DCOLOR&,DX::D3DVALUE&,DWORD&)(0,this,x,y,z,w,q,r);
	if(z==0) return DD_OK; //short-circuited?

	//HRESULT out = S_OK;
	out = S_OK;
	DX::D3DRECT buf[4];
	if(z) for(DWORD i=0,iN=max(x,1);i<iN;i+=_countof(buf))
	{
		DX::D3DRECT *src;
		if(y&&!DDRAW::doNothing) //2021: dx_DPI_testing_2021
		{
			for(DWORD j=0;i<x&&j<_countof(buf);j++)
			{
				auto &temp = buf[j]; 
				
				temp = y[i+j];

				if(DDRAW::doScaling)
				{
					temp.x1*=DDRAW::xyScaling[0]; temp.x2*=DDRAW::xyScaling[0]; 
					temp.y1*=DDRAW::xyScaling[1]; temp.y2*=DDRAW::xyScaling[1]; 
				}
				if(DDRAW::doMapping)
				{
					temp.x1+=DDRAW::xyMapping[0]; temp.y1+=DDRAW::xyMapping[1]; 
					temp.x2+=DDRAW::xyMapping[0]; temp.y2+=DDRAW::xyMapping[1]; 		
				}
			}		
			src = buf;
		}
		else src = y+i;

		auto yy = y?src:0;

		DWORD xx = y?min(x-i,_countof(buf)):0;

		out = proxy6->Clear2(xx,(::LPD3DRECT)yy,z,(::D3DCOLOR)w,(::D3DVALUE)q,r);
	}

	DDRAW_POP_HACK(DIRECT3DVIEWPORT3_CLEAR2,IDirect3DViewport3*,
	DWORD&,DX::LPD3DRECT&,DWORD&,DX::D3DCOLOR&,DX::D3DVALUE&,DWORD&)(&out,this,x,y,z,w,q,r);

	DDRAW_RETURN(out)
}






HRESULT DDRAW::IDirect3D3::QueryInterface(REFIID riid, LPVOID *ppvObj)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirect3D3::QueryInterface()\n";
 
	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	if(7<=DDRAWLOG::debug)
	{
		LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';
	}

	assert(0);

	return proxy->QueryInterface(riid,ppvObj); 
}
ULONG DDRAW::IDirect3D3::AddRef()
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirect3D3::AddRef()\n";
 
	DDRAW_ADDREF_RETURN(0)	
}
ULONG DDRAW::IDirect3D3::Release()
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirect3D3::Release()\n";

	DDRAW_IF_NOT_TARGET_AND_RELEASE(out,0)
	
	if(out==0) delete this; 
	
	return out;
}
HRESULT DDRAW::IDirect3D3::EnumDevices(DX::LPD3DENUMDEVICESCALLBACK x, LPVOID y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3D3::EnumDevices()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	HRESULT out = !D3D_OK; void *passthru = dx_ddraw_reserve_enum_set(x,y);

	//TODO: log dx6 enumeration
	if(target=='_dx6') out = proxy6->EnumDevices(dx_ddraw_enumdevices3cb,passthru);
	if(target=='dx7a') out = proxy->EnumDevices(dx_ddraw_enumdevices723cb,passthru);

	dx_ddraw_return_enum_set(passthru); DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirect3D3::CreateLight(DX::LPDIRECT3DLIGHT *x, LPUNKNOWN y) 
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3D3::CreateLight()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	::LPDIRECT3DLIGHT q = 0;

	HRESULT out = proxy6->CreateLight(&q,y);

	if(out==D3D_OK&&doIDirect3DLight)
	{
		DDRAW::IDirect3DLight *p = new DDRAW::IDirect3DLight('_dx6');

		*x = p; p->proxy6 = q;
	}
	else *x = (DX::LPDIRECT3DLIGHT)q;

	DDRAW_RETURN(out)
}
HRESULT DDRAW::IDirect3D3::CreateMaterial(DX::LPDIRECT3DMATERIAL3 *x, LPUNKNOWN y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3D3::CreateMaterial()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	::LPDIRECT3DMATERIAL3 q = 0;

	HRESULT out = proxy6->CreateMaterial(&q,y);

	if(out==D3D_OK&&doIDirect3DMaterial3)
	{
		DDRAW::IDirect3DMaterial3 *p = new DDRAW::IDirect3DMaterial3('_dx6');

		*x = p; p->proxy6 = q;
	}
	else *x = (DX::LPDIRECT3DMATERIAL3)q;

	DDRAW_RETURN(out)
}
HRESULT DDRAW::IDirect3D3::CreateViewport(DX::LPDIRECT3DVIEWPORT3 *x, LPUNKNOWN y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3D3::CreateViewport()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	HRESULT out = !D3D_OK;

	::LPDIRECT3DVIEWPORT3 q = 0;

	out = proxy6->CreateViewport((::LPDIRECT3DVIEWPORT3*)&q,y);

	if(out==S_OK)
	if(doIDirect3DViewport3)
	{
		DDRAW::IDirect3DViewport3 *p = new IDirect3DViewport3('_dx6');

		p->proxy6 = q; *x = (DX::LPDIRECT3DVIEWPORT3)p;
	}
	else *x = (DX::LPDIRECT3DVIEWPORT3)q;

	DDRAW_RETURN(out)
}
HRESULT DDRAW::IDirect3D3::FindDevice(DX::LPD3DFINDDEVICESEARCH x, DX::LPD3DFINDDEVICERESULT y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3D3::FindDevice()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	assert(0); //unimplemented

	DDRAW_RETURN(proxy6->FindDevice((::LPD3DFINDDEVICESEARCH)x,(::LPD3DFINDDEVICERESULT)y))
}
HRESULT DDRAW::IDirect3D3::CreateDevice(REFCLSID x, DX::LPDIRECTDRAWSURFACE4 y, DX::LPDIRECT3DDEVICE3 *z, LPUNKNOWN w)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3D3::CreateDevice()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	if(!DDRAW::doIDirect3DDevice3) return E_NOINTERFACE;

	DDRAW_PUSH_HACK(DIRECT3D3_CREATEDEVICE,IDirect3D3*,
	REFCLSID,DX::LPDIRECTDRAWSURFACE4&,DX::LPDIRECT3DDEVICE3*&,LPUNKNOWN&)(0,this,x,y,z,w);

	if(7<=DDRAWLOG::debug)
	{
		LPOLESTR o; if(StringFromCLSID(x,&o)==S_OK) DDRAW_LEVEL(7) << ' ' << o << '\n';	
	}
		
	DDRAW_HELLO(1) << "Acquiring Direct3D Device"; 

//	if(x==IID_IDirect3DRampDevice) //supported prior to DirectX7
//	{
//		DDRAW_HELLO(1) << " (IDirect3DRampDevice)\n";
//	}
	if(x==IID_IDirect3DRGBDevice)
	{
		DDRAW_HELLO(1) << " (IDirect3DRGBDevice)\n";
	}
	else if(x==IID_IDirect3DHALDevice)
	{
		DDRAW_HELLO(1) << " (IDirect3DHALDevice)\n";
	}
//	else if(x==IID_IDirect3DMMXDevice) //supported prior to DirectX7
//	{
//		DDRAW_HELLO(7) << " (IDirect3DMMXDevice)\n";
//	}
	else if(x==IID_IDirect3DRefDevice)
	{
		DDRAW_HELLO(1) << " (IDirect3DRefDevice)\n";
	}
	else if(x==IID_IDirect3DNullDevice)
	{
		DDRAW_HELLO(1) << " (IDirect3DNullDevice)\n";
	}
	else if(x==IID_IDirect3DTnLHalDevice)
	{
		DDRAW_HELLO(1) << " (IDirect3DTnLHalDevice)\n";
	}
	else if(x==IID_IDirect3DDevice7)
	{
		DDRAW_HELLO(1) << " (IDirect3DDevice7)\n";

		assert(0); //a valid device??
	}
	else
	{
		assert(!"\nWARNING! Unrecognized device...\n");
	}
	
	out = D3D_OK;

	auto py = DDRAW::is_proxy<IDirectDrawSurface4>(y);

	DX::LPDIRECT3DDEVICE7 q = 0;

	if(target=='dx7a')
	{
		assert(py&&py->query->dx7); //primary surfaces should be all set

		if(!py||!py->query->dx7) DDRAW_FINISH(E_NOINTERFACE); 
		
		out = proxy7->CreateDevice(x,py->query->dx7->proxy7,(::LPDIRECT3DDEVICE7*)&q);
	}
	else if(target!='_dx6')
	{
		assert(0); DDRAW_FINISH(E_NOINTERFACE); 
	}
	else out = proxy6->CreateDevice(x,py->proxy6,(::LPDIRECT3DDEVICE3*)&q,w); 	
	
	bool soft = x==IID_IDirect3DRGBDevice;

	if(out!=D3D_OK)
	{			
		REFCLSID qiid = IID_IDirect3DTnLHalDevice; soft = false;

		if(target=='dx7a')
		out = proxy7->CreateDevice(qiid,py->query->dx7->proxy7,(::LPDIRECT3DDEVICE7*)&q);		
		if(target=='_dx6')
		out = proxy6->CreateDevice(qiid,py->proxy6,(::LPDIRECT3DDEVICE3*)&q,w);		
																					
		if(out==D3D_OK)
		DDRAW_ALERT(1) << "SORRY! Using IDirect3DTnLHalDevice instead\n";		
	}

	if(out!=D3D_OK)
	{ 
		DDRAW_LEVEL(7) << "CreateDevice FAILED\n"; 

		DDRAW_ALERT(0) << "WARNING! Failed to acquire Direct3D device\n";		

		DDRAW_FINISH(out) 
	}
	
	assert(!query->d3ddevice); //assuming singular device

	//Hmm: creating a device is probably different from querying it
	DDRAW::IDirect3DDevice3 *p = new DDRAW::IDirect3DDevice3(target); 

	p->query->d3d = query; query->d3ddevice = p->query; p->proxy = q;
	
	*z = p; p->query->isSoftware = soft;

	if(target=='dx7a')
	{
		assert(!DDRAW::Direct3DDevice7);

		DDRAW::Direct3DDevice7 = new DDRAW::IDirect3DDevice7('dx7a',p->query);

		DDRAW::Direct3DDevice7->proxy = q; q->AddRef();
	}

	if(out==DD_OK) DDRAW::lights = 0; 

	dx_ddraw_level(true); //reset

	DDRAW_POP_HACK(DIRECT3D3_CREATEDEVICE,IDirect3D3*,
	REFCLSID,DX::LPDIRECTDRAWSURFACE4&,DX::LPDIRECT3DDEVICE3*&,LPUNKNOWN&)(&out,this,x,y,z,w);
	
	DDRAW_RETURN(out)
}
HRESULT DDRAW::IDirect3D3::CreateVertexBuffer(DX::LPD3DVERTEXBUFFERDESC x, DX::LPDIRECT3DVERTEXBUFFER *y, DWORD z, LPUNKNOWN w)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3D3::CreateVertexBuffer()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	if(!y) return !D3D_OK;

	HRESULT out = proxy6->CreateVertexBuffer((::LPD3DVERTEXBUFFERDESC)x,(::LPDIRECT3DVERTEXBUFFER*)y,z,w);

	DDRAW_RETURN(out)
}
HRESULT DDRAW::IDirect3D3::EnumZBufferFormats(REFCLSID x, DX::LPD3DENUMPIXELFORMATSCALLBACK y, LPVOID z)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3D3::EnumZBufferFormats()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	LPOLESTR p; if(StringFromCLSID(x,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';

	DDRAW_RETURN(proxy6->EnumZBufferFormats(x,(::LPD3DENUMPIXELFORMATSCALLBACK)y,z))

	return proxy->EnumZBufferFormats(x,y,z);
}
HRESULT DDRAW::IDirect3D3::EvictManagedTextures()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3D3::EvictManagedTextures()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_RETURN(proxy6->EvictManagedTextures())

	return proxy->EvictManagedTextures();
}





HRESULT DDRAW::IDirect3D7::QueryInterface(REFIID riid, LPVOID *ppvObj)
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirect3D7::QueryInterface()\n";
 
	DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK);

	if(7<=DDRAWLOG::debug)
	{
		LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';
	}

	assert(0);

	return proxy->QueryInterface(riid,ppvObj); 
}
ULONG DDRAW::IDirect3D7::AddRef()
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirect3D7::AddRef()\n";
 
	DDRAW_ADDREF_RETURN(0)	
}
ULONG DDRAW::IDirect3D7::Release()
{ 
	DDRAW_LEVEL(7) << "DDRAW::IDirect3D7::Release()\n";

	DDRAW_IF_NOT_TARGET_AND_RELEASE(out,0)

	if(out==0) delete this; 
	
	return out;
}
HRESULT DDRAW::IDirect3D7::EnumDevices(DX::LPD3DENUMDEVICESCALLBACK7 x, LPVOID y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3D7::EnumDevices()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	void *passthru = dx_ddraw_reserve_enum_set(x,y);

	HRESULT out = proxy->EnumDevices(dx_ddraw_enumdevices7cb,passthru);

	dx_ddraw_return_enum_set(passthru); DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirect3D7::CreateDevice(REFCLSID xIn, DX::LPDIRECTDRAWSURFACE7 y, DX::LPDIRECT3DDEVICE7 *z)
{
	const GUID *x = &xIn;

	DDRAW_LEVEL(7) << "DDRAW::IDirect3D7::CreateDevice()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	DDRAW_PUSH_HACK(DIRECT3D7_CREATEDEVICE,IDirect3D7*,
	const GUID*&,DX::LPDIRECTDRAWSURFACE7&,DX::LPDIRECT3DDEVICE7*&)(0,this,x,y,z);

	if(7<=DDRAWLOG::debug)
	{
		LPOLESTR o; if(StringFromCLSID(*x,&o)==S_OK) DDRAW_LEVEL(7) << ' ' << o << '\n';	
	}
						
	DDRAW_HELLO(1) << "Acquiring Direct3D Device";

//	if(*x==IID_IDirect3DRampDevice) //supported prior to DirectX7
//	{
//		DDRAW_HELLO(1) << " (IDirect3DRampDevice)\n";
//	}
	if(*x==IID_IDirect3DRGBDevice)
	{
		DDRAW_HELLO(1) << " (IDirect3DRGBDevice)\n";
	}
	else if(*x==IID_IDirect3DHALDevice)
	{
		DDRAW_HELLO(1) << " (IDirect3DHALDevice)\n";
	}
//	else if(*x==IID_IDirect3DMMXDevice) //supported prior to DirectX7
//	{
//		DDRAW_HELLO(7) << " (IDirect3DMMXDevice)\n";
//	}
	else if(*x==IID_IDirect3DRefDevice)
	{
		DDRAW_HELLO(1) << " (IDirect3DRefDevice)\n";
	}
	else if(*x==IID_IDirect3DNullDevice)
	{
		DDRAW_HELLO(1) << " (IDirect3DNullDevice)\n";
	}
	else if(*x==IID_IDirect3DTnLHalDevice)
	{
		DDRAW_HELLO(1) << " (IDirect3DTnLHalDevice)\n";
	}
	else if(*x==IID_IDirect3DDevice7)
	{
		DDRAW_HELLO(1) << " (IDirect3DDevice7)\n";

		assert(0); //a valid device??
	}
	else
	{
		assert(!"\nWARNING! Unrecognized device...\n");
	}
	
	auto p = DDRAW::is_proxy<IDirectDrawSurface7>(y);

	if(p) y = p->proxy;

	DX::LPDIRECT3DDEVICE7 q = 0; 
	
	out = proxy->CreateDevice(*x,y,(DX::LPDIRECT3DDEVICE7*)&q);

	bool soft = *x==IID_IDirect3DRGBDevice;
		
	if(out!=D3D_OK)
	{			
		REFCLSID qiid = IID_IDirect3DTnLHalDevice; soft = false;
		out = proxy->CreateDevice(qiid,y,(DX::LPDIRECT3DDEVICE7*)&q);

		if(out==D3D_OK)
		DDRAW_ALERT(1) << "SORRY! Using IDirect3DTnLHalDevice instead\n";		
	}

	if(out!=D3D_OK)
	{ 
		DDRAW_LEVEL(7) << "CreateDevice FAILED\n"; 		
		DDRAW_ALERT(0) << "WARNING! Failed to acquire Direct3D device\n";		
		DDRAW_FINISH(out) 
	}

	if(DDRAW::doIDirect3DDevice7) 
	{
		assert(!query->d3ddevice); //assuming singular device

		//Hmm: creating a device is probably different from querying it
		DDRAW::IDirect3DDevice7 *p = new DDRAW::IDirect3DDevice7('dx7a'); //query->d3ddevice

		p->query->d3d = query; query->d3ddevice = p->query; p->proxy = q;
		
		p->query->isSoftware = soft; p->AddRef();
		
		*z = DDRAW::Direct3DDevice7 = p; 
	}
	else *z = q;

	if(out==DD_OK) DDRAW::lights = 0; 

	dx_ddraw_level(true); //reset

	DDRAW_POP_HACK(DIRECT3D7_CREATEDEVICE,IDirect3D7*,
	const GUID*&,DX::LPDIRECTDRAWSURFACE7&,DX::LPDIRECT3DDEVICE7*&)(&out,this,x,y,z);
	
	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirect3D7::CreateVertexBuffer(DX::LPD3DVERTEXBUFFERDESC x, DX::LPDIRECT3DVERTEXBUFFER7 *y, DWORD z)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3D7::CreateVertexBuffer()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	DDRAW_PUSH_HACK(DIRECT3D7_CREATEVERTEXBUFFER,IDirect3D7*,
	DX::LPD3DVERTEXBUFFERDESC&,DX::LPDIRECT3DVERTEXBUFFER7*&,DWORD&)(0,this,x,y,z);						  
	
	if(!y) DDRAW_FINISH(!D3D_OK)

	DX::LPDIRECT3DVERTEXBUFFER7 q = 0; 

	out = proxy->CreateVertexBuffer(x,&q,z);

	if(out==D3D_OK)
	if(DDRAW::doIDirect3DVertexBuffer7)
	{
		DX::LPDIRECT3DVERTEXBUFFER7 d3dvb7 = q;
		
		DDRAW::IDirect3DVertexBuffer7 *p = new DDRAW::IDirect3DVertexBuffer7('dx7a');

		p->proxy = d3dvb7; *y = p;
	}
	else *y = q;
	
	DDRAW_POP_HACK(DIRECT3D7_CREATEVERTEXBUFFER,IDirect3D7*,
	DX::LPD3DVERTEXBUFFERDESC&,DX::LPDIRECT3DVERTEXBUFFER7*&,DWORD&)(&out,this,x,y,z);

	DDRAW_RETURN(out)  
}
HRESULT DDRAW::IDirect3D7::EnumZBufferFormats(REFCLSID x, DX::LPD3DENUMPIXELFORMATSCALLBACK y, LPVOID z)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3D7::EnumZBufferFormats()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	LPOLESTR p; if(StringFromCLSID(x,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';

	return proxy->EnumZBufferFormats(x,y,z);
}
HRESULT DDRAW::IDirect3D7::EvictManagedTextures()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3D7::EvictManagedTextures()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	return proxy->EvictManagedTextures();
}





HRESULT DDRAW::IDirect3DDevice3::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::QueryInterface()\n";

	DDRAW_IF_NOT__DX6_RETURN(!S_OK);

	if(7<=DDRAWLOG::debug)
	{
		LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';
	}

	assert(0);

	return proxy->QueryInterface(riid,ppvObj);
}
ULONG DDRAW::IDirect3DDevice3::AddRef()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::AddRef()\n";

	DDRAW_ADDREF_RETURN(0)	
}
ULONG DDRAW::IDirect3DDevice3::Release()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::Release()\n";

	DDRAW_IF_NOT_TARGET_AND_RELEASE(out,0)

	//REMOVE ME?
	if(out==1) return dx_ddraw_try_direct3ddevice7_release();

	if(out==0) delete this; 

	return out;
}
HRESULT DDRAW::IDirect3DDevice3::GetCaps(DX::LPD3DDEVICEDESC x, DX::LPD3DDEVICEDESC y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::GetCaps()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	assert(0); //should apply same changes as in enumeration

	DDRAW_RETURN(proxy6->GetCaps((::LPD3DDEVICEDESC)x,(::LPD3DDEVICEDESC)y))
}
HRESULT DDRAW::IDirect3DDevice3::GetStats(DX::LPD3DSTATS x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::GetStats()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_RETURN(proxy6->GetStats((::LPD3DSTATS)x))
}
HRESULT DDRAW::IDirect3DDevice3::AddViewport(DX::LPDIRECT3DVIEWPORT3 x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::AddViewport()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK); 

	auto p = DDRAW::is_proxy<IDirect3DViewport3>(x); 

	DDRAW_RETURN(proxy6->AddViewport(p?p->proxy6:(::LPDIRECT3DVIEWPORT3)x))
}
HRESULT DDRAW::IDirect3DDevice3::DeleteViewport(DX::LPDIRECT3DVIEWPORT3 x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::DeleteViewport()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	auto p = DDRAW::is_proxy<IDirect3DViewport3>(x); 

	DDRAW_RETURN(proxy6->DeleteViewport(p?p->proxy6:(::LPDIRECT3DVIEWPORT3)x))
}
HRESULT DDRAW::IDirect3DDevice3::NextViewport(DX::LPDIRECT3DVIEWPORT3 x, DX::LPDIRECT3DVIEWPORT3 *y, DWORD z)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::NextViewport()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	assert(0); //unimplemented

	DDRAW_RETURN(proxy6->NextViewport((::LPDIRECT3DVIEWPORT3)x,(::LPDIRECT3DVIEWPORT3*)y,z))
}
HRESULT DDRAW::IDirect3DDevice3::EnumTextureFormats(DX::LPD3DENUMPIXELFORMATSCALLBACK x, LPVOID y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::EnumTextureFormats()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_RETURN(proxy6->EnumTextureFormats((::LPD3DENUMPIXELFORMATSCALLBACK)x,y))
}
HRESULT DDRAW::IDirect3DDevice3::BeginScene()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::BeginScene()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_PUSH_HACK(DIRECT3DDEVICE3_BEGINSCENE,IDirect3DDevice3*)(0,this);
	
	out = target=='_dx6'?proxy6->BeginScene():proxy->BeginScene(); //???

	if(out==D3D_OK) DDRAW::inScene = true;

	DDRAW_POP_HACK(DIRECT3DDEVICE3_BEGINSCENE,IDirect3DDevice3*)(&out,this);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirect3DDevice3::EndScene()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::EndScene()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_PUSH_HACK(DIRECT3DDEVICE3_ENDSCENE,IDirect3DDevice3*)(0,this);

	out = target=='_dx6'?proxy6->EndScene():proxy->EndScene(); //???
			   	
	if(out==D3D_OK) DDRAW::inScene = false;

	DDRAW_POP_HACK(DIRECT3DDEVICE3_ENDSCENE,IDirect3DDevice3*)(&out,this);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirect3DDevice3::GetDirect3D(DX::LPDIRECT3D3 *x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::GetDirect3D()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_LEVEL(7) << "PANIC! GetDirect3D was called...\n";

	assert(x&&query->d3d&&query->d3d->dx3);
	if(!x||!query->d3d||!query->d3d->dx3) return !S_OK;

	if(query->d3d->dx3)
	{
		*x = query->d3d->dx3; query->d3d->dx3->AddRef();
	}
	else return !S_OK;

	return S_OK;
}
HRESULT DDRAW::IDirect3DDevice3::SetRenderTarget(DX::LPDIRECTDRAWSURFACE4 x, DWORD y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::SetRenderTarget()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);
			
	auto p = DDRAW::is_proxy<IDirectDrawSurface4>(x);
	
	DDRAW_RETURN(proxy6->SetRenderTarget(p?p->proxy6:(::LPDIRECTDRAWSURFACE4)x,y))
}
HRESULT DDRAW::IDirect3DDevice3::GetRenderTarget(DX::LPDIRECTDRAWSURFACE4 *x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::GetRenderTarget()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);
	
	assert(0); //unimplemented

	return !S_OK;
}
HRESULT DDRAW::IDirect3DDevice3::SetCurrentViewport(DX::LPDIRECT3DVIEWPORT3 x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::SetCurrentViewport()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	auto p = DDRAW::is_proxy<IDirect3DViewport3>(x);

	DDRAW_RETURN(proxy6->SetCurrentViewport(p?p->proxy6:(::LPDIRECT3DVIEWPORT3)x))
}

HRESULT DDRAW::IDirect3DDevice3::GetCurrentViewport(DX::LPDIRECT3DVIEWPORT3 *x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::GetCurrentViewport()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_RETURN(proxy6->GetCurrentViewport((::LPDIRECT3DVIEWPORT3*)x))
}
HRESULT DDRAW::IDirect3DDevice3::Begin(DX::D3DPRIMITIVETYPE x, DWORD y, DWORD z)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::Begin()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_RETURN(proxy6->Begin((::D3DPRIMITIVETYPE)x,y,z))
}
HRESULT DDRAW::IDirect3DDevice3::BeginIndexed(DX::D3DPRIMITIVETYPE x, DWORD y, LPVOID z, DWORD w, DWORD q) 
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::BeginIndexed()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_RETURN(proxy6->BeginIndexed((::D3DPRIMITIVETYPE)x,y,z,w,q))
}
HRESULT DDRAW::IDirect3DDevice3::Vertex(LPVOID x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::Vertex()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_RETURN(proxy6->Vertex(x))
}
HRESULT DDRAW::IDirect3DDevice3::Index(WORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::Index()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_RETURN(proxy6->Index(x))
}
HRESULT DDRAW::IDirect3DDevice3::End(DWORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::End()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_RETURN(proxy6->End(x))
}
HRESULT DDRAW::IDirect3DDevice3::GetRenderState(DX::D3DRENDERSTATETYPE x, LPDWORD y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::GetRenderState()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_RETURN(proxy6->GetRenderState((::D3DRENDERSTATETYPE)x,y))
}
HRESULT DDRAW::IDirect3DDevice3::SetRenderState(DX::D3DRENDERSTATETYPE x, DWORD y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::SetRenderState()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_PUSH_HACK(DIRECT3DDEVICE3_SETRENDERSTATE,IDirect3DDevice3*,
	DX::D3DRENDERSTATETYPE&,DWORD&)(0,this,x,y);

	DDRAW_LEVEL(7) << " Render State is " << x << " (setting to " << y << ")\n";
	
	if(x==DX::D3DRENDERSTATE_LIGHTING) DDRAW::isLit = y?true:false;
	if(x==DX::D3DRENDERSTATE_FOGENABLE) DDRAW::inFog = y?true:false;

	if(target=='_dx6') out = proxy6->SetRenderState((::D3DRENDERSTATETYPE)x,y);	
		//???
	if(target=='dx7a') out = proxy->SetRenderState(x,y);	

	DDRAW_POP_HACK(DIRECT3DDEVICE3_SETRENDERSTATE,IDirect3DDevice3*,
	DX::D3DRENDERSTATETYPE&,DWORD&)(&out,this,x,y);

	DDRAW_RETURN(out)
}
HRESULT DDRAW::IDirect3DDevice3::GetLightState(DX::D3DLIGHTSTATETYPE x, LPDWORD y) 
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::GetLightState()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);
	
	DDRAW_RETURN(proxy6->GetLightState((::D3DLIGHTSTATETYPE)x,y))
}
HRESULT DDRAW::IDirect3DDevice3::SetLightState(DX::D3DLIGHTSTATETYPE x, DWORD y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::SetLightState()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);
	
	DDRAW_RETURN(proxy6->SetLightState((::D3DLIGHTSTATETYPE)x,y))
}
HRESULT DDRAW::IDirect3DDevice3::SetTransform(DX::D3DTRANSFORMSTATETYPE x, DX::LPD3DMATRIX y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::SetTransform()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);
 
	DDRAW_PUSH_HACK(DIRECT3DDEVICE3_SETTRANSFORM,IDirect3DDevice3*,
	DX::D3DTRANSFORMSTATETYPE&,DX::LPD3DMATRIX&)(0,this,x,y);

	if(DDRAWLOG::debug>=7) //#ifdef _DEBUG
	{
		DDRAW_LEVEL(7) << " Transform matrix is " << x << '\n';
		
		DX::D3DVALUE *m = (DX::D3DVALUE*)y;

		const char *format = "%6.6f %6.6f %6.6f %6.6f\n";
		char buf[96];
		sprintf(buf,format,*m++,*m++,*m++,*m++); DDRAW_LEVEL(7) << buf;
		sprintf(buf,format,*m++,*m++,*m++,*m++); DDRAW_LEVEL(7) << buf;
		sprintf(buf,format,*m++,*m++,*m++,*m++); DDRAW_LEVEL(7) << buf;
		sprintf(buf,format,*m++,*m++,*m++,*m++); DDRAW_LEVEL(7) << buf;
	}
	
	if(target=='_dx6') out = proxy6->SetTransform((::D3DTRANSFORMSTATETYPE)x,(::LPD3DMATRIX)y);	
		//???
	if(target=='dx7a') out = proxy->SetTransform(x,y);	

	DDRAW_POP_HACK(DIRECT3DDEVICE3_SETTRANSFORM,IDirect3DDevice3*,
	DX::D3DTRANSFORMSTATETYPE&,DX::LPD3DMATRIX&)(&out,this,x,y);

	DDRAW_RETURN(out)
}
HRESULT DDRAW::IDirect3DDevice3::GetTransform(DX::D3DTRANSFORMSTATETYPE x, DX::LPD3DMATRIX y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::GetTransform()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_PUSH_HACK_IF(y!=DDRAW::getD3DMATRIX,
	DIRECT3DDEVICE3_GETTRANSFORM,IDirect3DDevice3*,
	DX::D3DTRANSFORMSTATETYPE&,DX::LPD3DMATRIX&)(0,this,x,y);
						   	
	if(target=='_dx6') out = proxy6->GetTransform((::D3DTRANSFORMSTATETYPE)x,(::LPD3DMATRIX)y);	
		//???
	if(target=='dx7a') out = proxy->GetTransform(x,y);	

	if(y!=DDRAW::getD3DMATRIX)
	DDRAW_POP_HACK(DIRECT3DDEVICE3_GETTRANSFORM,IDirect3DDevice3*,
	DX::D3DTRANSFORMSTATETYPE&,DX::LPD3DMATRIX&)(&out,this,x,y);

	DDRAW_RETURN(out)
}
HRESULT DDRAW::IDirect3DDevice3::MultiplyTransform(DX::D3DTRANSFORMSTATETYPE x, DX::LPD3DMATRIX y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::MultiplyTransform()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_RETURN(proxy6->MultiplyTransform((::D3DTRANSFORMSTATETYPE)x,(::LPD3DMATRIX)y))
}							  
void DDRAW::log_FVF(DWORD y) //2021
{
	if(DDRAWLOG::debug>=7) 
	/*if(y==D3DFVF_TLVERTEX)
	{
		_D3DTLVERTEX *p = (_D3DTLVERTEX*)z;

		if(w<=4) for(int i=0;i<4;i++,p++) //some kind of quad
		{
			auto c = (unsigned char*)&p->color;
			char buf[128];
			sprintf(buf,"v%d(%f,%f)/%f c(#%02x%02x%02x%02x) t(%f,%f)",
			i,p->sx,p->sy,p->rhw,c[0],c[1],c[2],c[3],p->tu,p->tv);
			DDRAW_LEVEL(7) << buf << '\n';			
		}
	}
	else*/
	{
#define OUT(X) if((y&X)==X) DDRAW_LEVEL(7) << ' ' << #X << '\n';
								  
	OUT(D3DFVF_VERTEX)
	OUT(D3DFVF_LVERTEX)                       
	OUT(D3DFVF_TLVERTEX) 
	OUT(D3DFVF_NORMAL)
	OUT(D3DFVF_DIFFUSE)
	OUT(D3DFVF_SPECULAR)
#undef OUT //2021
#define OUT(X) if((y&D3DFVF_POSITION_MASK)==X) DDRAW_LEVEL(7) << ' ' << #X << '\n';
	OUT(D3DFVF_POSITION_MASK)
	OUT(D3DFVF_XYZ)
	OUT(D3DFVF_XYZRHW)
	OUT(D3DFVF_XYZB1)
	OUT(D3DFVF_XYZB2)
	OUT(D3DFVF_XYZB3)
	OUT(D3DFVF_XYZB4)
	OUT(D3DFVF_XYZB5)
#undef OUT //2021
#define OUT(X) if((y&D3DFVF_TEXCOUNT_MASK)==X) DDRAW_LEVEL(7) << ' ' << #X << '\n';	
	OUT(D3DFVF_TEX0)
	OUT(D3DFVF_TEX1)
	OUT(D3DFVF_TEX2)
	OUT(D3DFVF_TEX3)
	OUT(D3DFVF_TEX4)
	OUT(D3DFVF_TEX5)
	OUT(D3DFVF_TEX6)
	OUT(D3DFVF_TEX7)
	OUT(D3DFVF_TEX8)

#undef OUT
	}
}
HRESULT DDRAW::IDirect3DDevice3::DrawPrimitive(DX::D3DPRIMITIVETYPE x, DWORD y, LPVOID z, DWORD w, DWORD q)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::DrawPrimitive()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_PUSH_HACK(DIRECT3DDEVICE3_DRAWPRIMITIVE,IDirect3DDevice3*,
	DX::D3DPRIMITIVETYPE&,DWORD&,LPVOID&,DWORD&,DWORD&)(0,this,x,y,z,w,q);

	assert(q==0); //what is q?

	DDRAW_LEVEL(7) << " Primitive Type is " << x << " (" << w << " vertices)\n";

	if(DDRAWLOG::debug>=7) DDRAW::log_FVF(y); //???

	extern UINT dx_d3d9c_sizeofFVF(DWORD in); //dx.d3d9c.cpp

	UINT sizeofFVF = dx_d3d9c_sizeofFVF(y);

	//WARNING! overwriting input buffer
	if(z&&!DDRAW::doNothing) 
	if(D3DFVF_XYZRHW==(y&D3DFVF_POSITION_MASK))
	{	
		if(DDRAW::doScaling&&DDRAW::xyScaling[0]!=1.0f) for(size_t i=0;i<w;i++)
		{
			*(float*)(((char*)z)+sizeofFVF*i)*=DDRAW::xyScaling[0]; 	
		}
		if(DDRAW::doScaling&&DDRAW::xyScaling[1]!=1.0f) for(size_t i=0;i<w;i++)
		{
			*(float*)(((char*)z)+sizeofFVF*i+sizeof(float))*=DDRAW::xyScaling[1]; 	
		}
		if(DDRAW::doMapping&&DDRAW::xyMapping[0]!=0.0f) for(size_t i=0;i<w;i++)
		{
			*(float*)(((char*)z)+sizeofFVF*i)+=DDRAW::xyMapping[0]; 	
		}
		if(DDRAW::doMapping&&DDRAW::xyMapping[1]!=0.0f) for(size_t i=0;i<w;i++)
		{
			*(float*)(((char*)z)+sizeofFVF*i+sizeof(float))+=DDRAW::xyMapping[1]; 	
		}
	}

	dx_ddraw_level();
		
	if(target=='_dx6') out = proxy6->DrawPrimitive((::D3DPRIMITIVETYPE)x,y,z,w,q);	
		//???
	if(target=='dx7a') out = proxy->DrawPrimitive(x,y,z,w,q);

	DDRAW_POP_HACK(DIRECT3DDEVICE3_DRAWPRIMITIVE,IDirect3DDevice3*,
	DX::D3DPRIMITIVETYPE&,DWORD&,LPVOID&,DWORD&,DWORD&)(&out,this,x,y,z,w,q);	

	DDRAW_RETURN(out)
}
HRESULT DDRAW::IDirect3DDevice3::DrawIndexedPrimitive(DX::D3DPRIMITIVETYPE x, DWORD y, LPVOID z, DWORD w, LPWORD q, DWORD r, DWORD s)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::DrawIndexedPrimitive()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_PUSH_HACK(DIRECT3DDEVICE3_DRAWINDEXEDPRIMITIVE,IDirect3DDevice3*,
	DX::D3DPRIMITIVETYPE&,DWORD&,LPVOID&,DWORD&,LPWORD&,DWORD&,DWORD&)(0,this,x,y,z,w,q,r,s);	
	if(!r) DDRAW_FINISH(D3D_OK) //facilitate cancellation

	dx_ddraw_level();

	if(target=='_dx6') //???
	{			
		DDRAW_FINISH(proxy6->DrawIndexedPrimitive((::D3DPRIMITIVETYPE)x,y,z,w,q,r,s))
/*
		extern UINT dx_d3d9c_sizeofFVF(DWORD in); //dx.d3d9c.cpp

		BYTE *rw = 0; UINT sizeofFVF = dx_d3d9c_sizeofFVF(y); assert(r%3==0);

		s is flags, this version doesn't pass in the number of primitives
		DDRAW_LEVEL(8) << "verts are " << w << " indices are " << r << '\n'; 

		for(DWORD i=0;i<r;i+=3)
		{	
			DDRAW_LEVEL(8) << int(q[i]) << ' ' << int(q[i+1]) << ' ' << int(q[i+2]) << '\n';

			rw = (BYTE*)z+sizeofFVF*q[i];   *rw = *rw;
			rw = (BYTE*)z+sizeofFVF*q[i+1]; *rw = *rw;
			rw = (BYTE*)z+sizeofFVF*q[i+2]; *rw = *rw;
		}		
*/
		return S_OK;
	}

	out = proxy->DrawIndexedPrimitive(x,y,z,w,q,r,s);

	DDRAW_POP_HACK(DIRECT3DDEVICE3_DRAWINDEXEDPRIMITIVE,IDirect3DDevice3*,
	DX::D3DPRIMITIVETYPE&,DWORD&,LPVOID&,DWORD&,LPWORD&,DWORD&,DWORD&)(&out,this,x,y,z,w,q,r,s);	

	DDRAW_RETURN(out)
}
HRESULT DDRAW::IDirect3DDevice3::SetClipStatus(DX::LPD3DCLIPSTATUS x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::SetClipStatus()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);
	
	if(target=='_dx6') //???
	{
		assert(0); //2021

		DDRAW_RETURN(proxy6->SetClipStatus((::LPD3DCLIPSTATUS)x))
	}

	//NOTE: som_db.exe calls this from 00401F8A???
		/*x->
		dwFlags	2	unsigned long
		dwStatus	0	unsigned long
		minx	2048.00000	float
		maxx	0.000000000	float
		miny	2048.00000	float
		maxy	0.000000000	float
		minz	0.000000000	float
		maxz	0.000000000	float
		*/
	//00401F8A FF 52 6C             call        dword ptr [edx+6Ch]

	return proxy->SetClipStatus(x);
}
HRESULT DDRAW::IDirect3DDevice3::GetClipStatus(DX::LPD3DCLIPSTATUS x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::GetClipStatus()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);
	
	if(target=='_dx6') //???
	DDRAW_RETURN(proxy6->GetClipStatus((::LPD3DCLIPSTATUS)x))

	return proxy->GetClipStatus(x);
}
HRESULT DDRAW::IDirect3DDevice3::DrawPrimitiveStrided(DX::D3DPRIMITIVETYPE x, DWORD y, DX::LPD3DDRAWPRIMITIVESTRIDEDDATA z, DWORD w, DWORD q)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::DrawPrimitiveStrided()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);
									
	dx_ddraw_level();

	if(target=='_dx6') //???
	DDRAW_RETURN(proxy6->DrawPrimitiveStrided((::D3DPRIMITIVETYPE)x,y,(::LPD3DDRAWPRIMITIVESTRIDEDDATA)z,w,q))

	return proxy->DrawPrimitiveStrided(x,y,z,w,q);
}
HRESULT DDRAW::IDirect3DDevice3::DrawIndexedPrimitiveStrided(DX::D3DPRIMITIVETYPE x, DWORD y, DX::LPD3DDRAWPRIMITIVESTRIDEDDATA z, DWORD w, LPWORD q, DWORD r, DWORD s)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::DrawIndexedPrimitiveStrided()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);
	
	dx_ddraw_level();

	if(target=='_dx6') //???
	DDRAW_RETURN(proxy6->DrawIndexedPrimitiveStrided((::D3DPRIMITIVETYPE)x,y,(::LPD3DDRAWPRIMITIVESTRIDEDDATA)z,w,q,r,s))

	return proxy->DrawIndexedPrimitiveStrided(x,y,z,w,q,r,s);
}
HRESULT DDRAW::IDirect3DDevice3::DrawPrimitiveVB(DX::D3DPRIMITIVETYPE x, DX::LPDIRECT3DVERTEXBUFFER y, DWORD z, DWORD w, DWORD q)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::DrawPrimitiveVB()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW::IDirect3DVertexBuffer *p = 0; 
//	DDRAW::is_proxy<IDirect3DVertexBuffer>(y);

	dx_ddraw_level();

	DDRAW_RETURN(proxy6->DrawPrimitiveVB((::D3DPRIMITIVETYPE)x,p?p->proxy6:(::LPDIRECT3DVERTEXBUFFER)y,z,w,q))
}
HRESULT DDRAW::IDirect3DDevice3::DrawIndexedPrimitiveVB(DX::D3DPRIMITIVETYPE x, DX::LPDIRECT3DVERTEXBUFFER y, LPWORD z, DWORD w, DWORD q)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::DrawIndexedPrimitiveVB()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW::IDirect3DVertexBuffer *p = 0;
//	DDRAW::is_proxy<IDirect3DVertexBuffer>(y);

	dx_ddraw_level();

	DDRAW_RETURN(proxy6->DrawIndexedPrimitiveVB((::D3DPRIMITIVETYPE)x,p?p->proxy6:(::LPDIRECT3DVERTEXBUFFER)y,z,w,q))
}
HRESULT DDRAW::IDirect3DDevice3::ComputeSphereVisibility(DX::LPD3DVECTOR x, DX::LPD3DVALUE y, DWORD z, DWORD w, LPDWORD q)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::ComputeSphereVisibility()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);
	
	if(target=='_dx6') //???
	DDRAW_RETURN(proxy6->ComputeSphereVisibility((::LPD3DVECTOR)x,(::LPD3DVALUE)y,z,w,q))

	return proxy->ComputeSphereVisibility(x,y,z,w,q);
}
HRESULT DDRAW::IDirect3DDevice3::GetTexture(DWORD x, DX::LPDIRECT3DTEXTURE2 *y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::GetTexture()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	DDRAW_RETURN(proxy6->GetTexture(x,(::LPDIRECT3DTEXTURE2*)y))
}
HRESULT DDRAW::IDirect3DDevice3::SetTexture(DWORD x, DX::LPDIRECT3DTEXTURE2 y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::SetTexture()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK); 
										 
	auto p = DDRAW::is_proxy<IDirect3DTexture2>(y);

	if(!p||!p->surface6->texture)
	DDRAW_RETURN(proxy6->SetTexture(x,p?p->proxy6:(::LPDIRECT3DTEXTURE2)y))

	if(!p->surface6->loaded)
	{
		p->surface6->loaded = 
		p->surface6->texture->Load(p->proxy6)==D3D_OK; assert(p->surface6->loaded);
	}

	DDRAW_RETURN(proxy6->SetTexture(x,p->surface6->texture))		
}
HRESULT DDRAW::IDirect3DDevice3::GetTextureStageState(DWORD x, DX::D3DTEXTURESTAGESTATETYPE y, LPDWORD z)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::GetTextureStageState()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);

	if(target=='_dx6') //???
	DDRAW_RETURN(proxy6->GetTextureStageState(x,(::D3DTEXTURESTAGESTATETYPE)y,z))
	
	return proxy->GetTextureStageState(x,y,z);
}
HRESULT DDRAW::IDirect3DDevice3::SetTextureStageState(DWORD x, DX::D3DTEXTURESTAGESTATETYPE y, DWORD z)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::SetTextureStageState()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);
	
	DDRAW_LEVEL(7) << " Stage is " << x << " (" << y << " set to " << z << ")\n";

	DDRAW_PUSH_HACK(DIRECT3DDEVICE3_SETTEXTURESTAGESTATE,IDirect3DDevice3*,
	DWORD&,DX::D3DTEXTURESTAGESTATETYPE&,DWORD&)(0,this,x,y,z);

	if(target=='_dx6') out = proxy6->SetTextureStageState(x,(::D3DTEXTURESTAGESTATETYPE)y,z);	
	if(target=='dx7a') out = proxy->SetTextureStageState(x,y,z);

	DDRAW_POP_HACK(DIRECT3DDEVICE3_SETTEXTURESTAGESTATE,IDirect3DDevice3*,
	DWORD&,DX::D3DTEXTURESTAGESTATETYPE&,DWORD&)(&out,this,x,y,z);

	DDRAW_RETURN(out)
}
HRESULT DDRAW::IDirect3DDevice3::ValidateDevice(LPDWORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice3::ValidateDevice()\n";

	DDRAW_IF_NOT__DX6_RETURN(!D3D_OK);
	
	if(target=='_dx6') ///???
	DDRAW_RETURN(proxy6->ValidateDevice(x))

	return proxy->ValidateDevice(x);
}





HRESULT DDRAW::IDirect3DDevice7::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::QueryInterface()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!S_OK);

	if(7<=DDRAWLOG::debug)
	{
		LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';
	}

	assert(0);

	return proxy->QueryInterface(riid,ppvObj);
}
ULONG DDRAW::IDirect3DDevice7::AddRef()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::AddRef()\n";

	DDRAW_ADDREF_RETURN(0)
}
ULONG DDRAW::IDirect3DDevice7::Release()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::Release()\n";

	DDRAW_IF_NOT_TARGET_AND_RELEASE(out,0)

	//REMOVE ME?
	if(out==1) return dx_ddraw_try_direct3ddevice7_release();

	if(out==0)
	{
		delete this; DDRAW::Direct3DDevice7 = 0;						

		DDRAW::onReset(); //after DDRAW::Direct3DDevice7 is set 0
	}				 
	return out;
}
HRESULT DDRAW::IDirect3DDevice7::GetCaps(DX::LPD3DDEVICEDESC7 x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::GetCaps()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	HRESULT out = proxy->GetCaps(x);

	if(out==D3D_OK) //hack
	{	
		if(x) x->dwDeviceRenderBitDepth|=DDRAW::force_color_depths; 
	}

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirect3DDevice7::EnumTextureFormats(DX::LPD3DENUMPIXELFORMATSCALLBACK x, LPVOID y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::EnumTextureFormats()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	return proxy->EnumTextureFormats(x,y);
}
HRESULT DDRAW::IDirect3DDevice7::BeginScene()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::BeginScene()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_BEGINSCENE,IDirect3DDevice7*)(0,this);

	out = proxy->BeginScene();

	if(out==D3D_OK) DDRAW::inScene = true;

	DDRAW_POP_HACK(DIRECT3DDEVICE7_BEGINSCENE,IDirect3DDevice7*)(&out,this);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirect3DDevice7::EndScene()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::EndScene()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_ENDSCENE,IDirect3DDevice7*)(0,this);

	out = proxy->EndScene();
			   	
	if(out==D3D_OK) DDRAW::inScene = false;

	DDRAW_POP_HACK(DIRECT3DDEVICE7_ENDSCENE,IDirect3DDevice7*)(&out,this);
							
	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirect3DDevice7::GetDirect3D(DX::LPDIRECT3D7 *x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::GetDirect3D()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	DDRAW_LEVEL(7) << "PANIC! GetDirect3D was called...\n";

	if(query->d3d&&query->d3d->dx7)
	{
		*x = query->d3d->dx7; query->d3d->dx7->AddRef(); return S_OK;
	}
	else return !S_OK;
}
HRESULT DDRAW::IDirect3DDevice7::SetRenderTarget(DX::LPDIRECTDRAWSURFACE7 x, DWORD y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::SetRenderTarget()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	auto p = DDRAW::is_proxy<IDirectDrawSurface7>(x);
	
	if(p) x = p->proxy;

	return proxy->SetRenderTarget(x,y);
}
HRESULT DDRAW::IDirect3DDevice7::GetRenderTarget(DX::LPDIRECTDRAWSURFACE7 *x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::GetRenderTarget()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	HRESULT out = proxy->GetRenderTarget(x); 
	if(out!=DD_OK) DDRAW_RETURN(out) 

	if(DDRAW::doIDirectDrawSurface7)
	{	
		DDRAW::IDirectDrawSurface7 *p = 0; 		
		p = DDRAW::IDirectDrawSurface7::get_head->get(*x); 
		if(!p) p = new DDRAW::IDirectDrawSurface7('dx7a'); p->proxy = *x;
		*x = (DX::IDirectDrawSurface7*)p; 
	}
	
	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirect3DDevice7::Clear(DWORD x, DX::LPD3DRECT y, DWORD z, DX::D3DCOLOR w, DX::D3DVALUE q, DWORD r)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::Clear()\n";
 
	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);
	
	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_CLEAR,IDirect3DDevice7*,
	DWORD&,DX::LPD3DRECT&,DWORD&,DX::D3DCOLOR&,DX::D3DVALUE&,DWORD&)(0,this,x,y,z,w,q,r);

	//HRESULT out = S_OK;
	out = S_OK;
	DX::D3DRECT buf[4];
	if(z) for(DWORD i=0,iN=max(x,1);i<iN;i+=_countof(buf))	
	{
		DX::D3DRECT *src;
		if(y&&!DDRAW::doNothing) //2021: dx_DPI_testing_2021
		{
			for(DWORD j=0;i<x&&j<_countof(buf);j++)
			{
				auto &temp = buf[j]; 
				
				temp = y[i+j];

				if(DDRAW::doScaling)
				{
					temp.x1*=DDRAW::xyScaling[0]; temp.x2*=DDRAW::xyScaling[0]; 
					temp.y1*=DDRAW::xyScaling[1]; temp.y2*=DDRAW::xyScaling[1]; 
				}
				if(DDRAW::doMapping)
				{
					temp.x1+=DDRAW::xyMapping[0]; temp.y1+=DDRAW::xyMapping[1]; 
					temp.x2+=DDRAW::xyMapping[0]; temp.y2+=DDRAW::xyMapping[1]; 		
				}
			}		
			src = buf;
		}
		else src = y+i;

		auto yy = y?src:0;

		DWORD xx = y?min(x-i,_countof(buf)):0;

		out = proxy->Clear(xx,yy,z,w,q,r);
	}

	DDRAW_POP_HACK(DIRECT3DDEVICE7_CLEAR,IDirect3DDevice7*,
	DWORD&,DX::LPD3DRECT&,DWORD&,DX::D3DCOLOR&,DX::D3DVALUE&,DWORD&)(&out,this,x,y,z,w,q,r);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirect3DDevice7::SetTransform(DX::D3DTRANSFORMSTATETYPE x, DX::LPD3DMATRIX y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::SetTransform()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);
 
	//2020: correct? ApplyStateBlock?
	DDRAW_PUSH_HACK_IF(y!=DDRAW::getD3DMATRIX, 
	DIRECT3DDEVICE7_SETTRANSFORM,IDirect3DDevice7*,
	DX::D3DTRANSFORMSTATETYPE&,DX::LPD3DMATRIX&)(0,this,x,y);

	if(DDRAWLOG::debug>=7) //#ifdef _DEBUG
	{
		DDRAW_LEVEL(7) << " Transform matrix is " << x << '\n';
		
		DX::D3DVALUE *m = (DX::D3DVALUE*)y;

		const char *format = "%6.6f %6.6f %6.6f %6.6f\n";
		char buf[96];
		sprintf(buf,format,*m++,*m++,*m++,*m++); DDRAW_LEVEL(7) << buf;
		sprintf(buf,format,*m++,*m++,*m++,*m++); DDRAW_LEVEL(7) << buf;
		sprintf(buf,format,*m++,*m++,*m++,*m++); DDRAW_LEVEL(7) << buf;
		sprintf(buf,format,*m++,*m++,*m++,*m++); DDRAW_LEVEL(7) << buf;
	}

	out = proxy->SetTransform(x,y);

	DDRAW_POP_HACK(DIRECT3DDEVICE7_SETTRANSFORM,IDirect3DDevice7*,
	DX::D3DTRANSFORMSTATETYPE&,DX::LPD3DMATRIX&)(&out,this,x,y);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirect3DDevice7::GetTransform(DX::D3DTRANSFORMSTATETYPE x, DX::LPD3DMATRIX y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::GetTransform()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	DDRAW_PUSH_HACK_IF(y!=DDRAW::getD3DMATRIX,
	DIRECT3DDEVICE7_GETTRANSFORM,IDirect3DDevice7*,
	DX::D3DTRANSFORMSTATETYPE&,DX::LPD3DMATRIX&)(0,this,x,y);

	out = proxy->GetTransform(x,y);

	if(y!=DDRAW::getD3DMATRIX)
	DDRAW_POP_HACK(DIRECT3DDEVICE7_GETTRANSFORM,IDirect3DDevice7*,
	DX::D3DTRANSFORMSTATETYPE&,DX::LPD3DMATRIX&)(&out,this,x,y);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirect3DDevice7::SetViewport(DX::LPD3DVIEWPORT7 x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::SetViewport()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_SETVIEWPORT,IDirect3DDevice7*,
	DX::LPD3DVIEWPORT7&)(0,this,x);

	DX::D3DVIEWPORT7 temp; if(x&&!DDRAW::doNothing) 
	{
		temp = *x;

		if(DDRAW::doMapping) temp.dwX = DDRAW::xyMapping[0]+x->dwX;
		if(DDRAW::doMapping) temp.dwY = DDRAW::xyMapping[1]+x->dwY;

		if(DDRAW::doScaling) temp.dwWidth = DDRAW::xyScaling[0]*float(x->dwWidth);
		if(DDRAW::doScaling) temp.dwHeight = DDRAW::xyScaling[1]*float(x->dwHeight);

		x = &temp; 
	}

	out = proxy->SetViewport(x);

	DDRAW_POP_HACK(DIRECT3DDEVICE7_SETVIEWPORT,IDirect3DDevice7*,
	DX::LPD3DVIEWPORT7&)(&out,this,x);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirect3DDevice7::MultiplyTransform(DX::D3DTRANSFORMSTATETYPE x, DX::LPD3DMATRIX y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::MultiplyTransform()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	return proxy->MultiplyTransform(x,y);
}
HRESULT DDRAW::IDirect3DDevice7::GetViewport(DX::LPD3DVIEWPORT7 x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::GetViewport()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	HRESULT out = proxy->GetViewport(x);

	if(x&&out==D3D_OK&&!DDRAW::doNothing) 
	{
		if(DDRAW::doMapping) x->dwX = x->dwX-DDRAW::xyMapping[0];
		if(DDRAW::doMapping) x->dwY = x->dwY-DDRAW::xyMapping[1];

		if(DDRAW::doScaling) x->dwWidth = float(x->dwWidth)/DDRAW::xyScaling[0];
		if(DDRAW::doScaling) x->dwHeight = float(x->dwHeight)/DDRAW::xyScaling[1];
	}

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirect3DDevice7::SetMaterial(DX::LPD3DMATERIAL7 x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::SetMaterial()\n";
 
	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);
	
	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_SETMATERIAL,IDirect3DDevice7*,
	DX::LPD3DMATERIAL7&)(0,this,x);

	out = proxy->SetMaterial(x);
	
	DDRAW_POP_HACK(DIRECT3DDEVICE7_SETMATERIAL,IDirect3DDevice7*,
	DX::LPD3DMATERIAL7&)(&out,this,x);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirect3DDevice7::GetMaterial(DX::LPD3DMATERIAL7 x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::GetMaterial()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);
	
	return proxy->GetMaterial(x);
}
HRESULT DDRAW::IDirect3DDevice7::SetLight(DWORD x, DX::LPD3DLIGHT7 y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::SetLight()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_SETLIGHT,IDirect3DDevice7*,
	DWORD&,DX::LPD3DLIGHT7&)(0,this,x,y);

	DDRAW_LEVEL(7) << ' ' << x << '\n';

	if(!y) DDRAW_FINISH(proxy->SetLight(x,0))

	//C4533
	{
		DX::D3DLIGHT7 l = *y;

		//Assuming (like D3D9) negative attentuation not cool
		if(l.dvAttenuation0<0.0f) l.dvAttenuation0 = 0.0f;
		if(l.dvAttenuation1<0.0f) l.dvAttenuation1 = 0.0f;
		if(l.dvAttenuation2<0.0f) l.dvAttenuation2 = 0.0f;

		out = proxy->SetLight(x,&l);
	}

	DDRAW_POP_HACK(DIRECT3DDEVICE7_SETLIGHT,IDirect3DDevice7*,
	DWORD&,DX::LPD3DLIGHT7&)(&out,this,x,y);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirect3DDevice7::GetLight(DWORD x, DX::LPD3DLIGHT7 y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::GetLight()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);
	
	return proxy->GetLight(x,y);
}
HRESULT DDRAW::IDirect3DDevice7::SetRenderState(DX::D3DRENDERSTATETYPE x, DWORD y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::SetRenderState()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_SETRENDERSTATE,IDirect3DDevice7*,
	DX::D3DRENDERSTATETYPE&,DWORD&)(0,this,x,y);

	DDRAW_LEVEL(7) << " Render State is " << x << " (setting to " << y << ")\n";
	
	if(x==DX::D3DRENDERSTATE_LIGHTING) DDRAW::isLit = y?true:false;
	if(x==DX::D3DRENDERSTATE_FOGENABLE) DDRAW::inFog = y?true:false;

	out = proxy->SetRenderState(x,y);	

	DDRAW_POP_HACK(DIRECT3DDEVICE7_SETRENDERSTATE,IDirect3DDevice7*,
	DX::D3DRENDERSTATETYPE&,DWORD&)(&out,this,x,y);

	DDRAW_RETURN(out)
}
HRESULT DDRAW::IDirect3DDevice7::GetRenderState(DX::D3DRENDERSTATETYPE x, LPDWORD y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::GetRenderState()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_GETRENDERSTATE,IDirect3DDevice7*,
	DX::D3DRENDERSTATETYPE&,LPDWORD&)(0,this,x,y);
	
	if(x==DX::D3DRENDERSTATE_LIGHTING) 
	{
		//Note: SOM or something seems to request the lighting state
	}

	out = proxy->GetRenderState(x,y);

	DDRAW_POP_HACK(DIRECT3DDEVICE7_GETRENDERSTATE,IDirect3DDevice7*,
	DX::D3DRENDERSTATETYPE&,LPDWORD&)(&out,this,x,y);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirect3DDevice7::BeginStateBlock()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::BeginStateBlock()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);
	
	return proxy->BeginStateBlock(); 
}
HRESULT DDRAW::IDirect3DDevice7::EndStateBlock(LPDWORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::EndStateBlock()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);
	
	return proxy->EndStateBlock(x);
}
HRESULT DDRAW::IDirect3DDevice7::PreLoad(DX::LPDIRECTDRAWSURFACE7 x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::PreLoad()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	auto p = DDRAW::is_proxy<IDirectDrawSurface7>(x);
	
	if(p) x = p->proxy;

	return proxy->PreLoad(x);
}							  
HRESULT DDRAW::IDirect3DDevice7::DrawPrimitive(DX::D3DPRIMITIVETYPE x, DWORD y, LPVOID z, DWORD w, DWORD q)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::DrawPrimitive()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_DRAWPRIMITIVE,IDirect3DDevice7*,
	DX::D3DPRIMITIVETYPE&,DWORD&,LPVOID&,DWORD&,DWORD&)(0,this,x,y,z,w,q);

	DDRAW_LEVEL(7) << " Primitive Type is " << x << " (" << w << " vertices)\n";

	if(DDRAWLOG::debug>=7) DDRAW::log_FVF(y);

	extern UINT dx_d3d9c_sizeofFVF(DWORD); //dx.d3d9c.cpp

	UINT sizeofFVF = dx_d3d9c_sizeofFVF(y);

	//WARNING! overwriting input buffer
	if(z&&!DDRAW::doNothing) 
	if(D3DFVF_XYZRHW==(y&D3DFVF_POSITION_MASK))
	{	
		if(DDRAW::doScaling&&DDRAW::xyScaling[0]!=1.0f) for(size_t i=0;i<w;i++)
		{
			*(float*)(((char*)z)+sizeofFVF*i)*=DDRAW::xyScaling[0]; 	
		}
		if(DDRAW::doScaling&&DDRAW::xyScaling[1]!=1.0f) for(size_t i=0;i<w;i++)
		{
			*(float*)(((char*)z)+sizeofFVF*i+sizeof(float))*=DDRAW::xyScaling[1]; 	
		}
		if(DDRAW::doMapping&&DDRAW::xyMapping[0]!=0.0f) for(size_t i=0;i<w;i++)
		{
			*(float*)(((char*)z)+sizeofFVF*i)+=DDRAW::xyMapping[0]; 	
		}
		if(DDRAW::doMapping&&DDRAW::xyMapping[1]!=0.0f) for(size_t i=0;i<w;i++)
		{
			*(float*)(((char*)z)+sizeofFVF*i+sizeof(float))+=DDRAW::xyMapping[1]; 	
		}
	}

	dx_ddraw_level();
	
	out = proxy->DrawPrimitive(x,y,z,w,q);

	DDRAW_POP_HACK(DIRECT3DDEVICE7_DRAWPRIMITIVE,IDirect3DDevice7*,
	DX::D3DPRIMITIVETYPE&,DWORD&,LPVOID&,DWORD&,DWORD&)(&out,this,x,y,z,w,q);	

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirect3DDevice7::DrawIndexedPrimitive(DX::D3DPRIMITIVETYPE x, DWORD y, LPVOID z, DWORD w, LPWORD q, DWORD r, DWORD s)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::DrawIndexedPrimitive()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);
	
	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_DRAWINDEXEDPRIMITIVE,IDirect3DDevice7*,
	DX::D3DPRIMITIVETYPE&,DWORD&,LPVOID&,DWORD&,LPWORD&,DWORD&,DWORD&)(0,this,x,y,z,w,q,r,s);	

	dx_ddraw_level();

	out = proxy->DrawIndexedPrimitive(x,y,z,w,q,r,s);
	
	DDRAW_POP_HACK(DIRECT3DDEVICE7_DRAWINDEXEDPRIMITIVE,IDirect3DDevice7*,
	DX::D3DPRIMITIVETYPE&,DWORD&,LPVOID&,DWORD&,LPWORD&,DWORD&,DWORD&)(&out,this,x,y,z,w,q,r,s);	

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirect3DDevice7::SetClipStatus(DX::LPD3DCLIPSTATUS x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::SetClipStatus()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);
	
	return proxy->SetClipStatus(x);
}
HRESULT DDRAW::IDirect3DDevice7::GetClipStatus(DX::LPD3DCLIPSTATUS x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::GetClipStatus()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);
	
	return proxy->GetClipStatus(x);
}
HRESULT DDRAW::IDirect3DDevice7::DrawPrimitiveStrided(DX::D3DPRIMITIVETYPE x, DWORD y, DX::LPD3DDRAWPRIMITIVESTRIDEDDATA z, DWORD w, DWORD q)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::DrawPrimitiveStrided()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);
									
	dx_ddraw_level();

	return proxy->DrawPrimitiveStrided(x,y,z,w,q);
}
HRESULT DDRAW::IDirect3DDevice7::DrawIndexedPrimitiveStrided(DX::D3DPRIMITIVETYPE x, DWORD y, DX::LPD3DDRAWPRIMITIVESTRIDEDDATA z, DWORD w, LPWORD q, DWORD r, DWORD s)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::DrawIndexedPrimitiveStrided()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);
	
	dx_ddraw_level();

	return proxy->DrawIndexedPrimitiveStrided(x,y,z,w,q,r,s);
}
HRESULT DDRAW::IDirect3DDevice7::DrawPrimitiveVB(DX::D3DPRIMITIVETYPE x, DX::LPDIRECT3DVERTEXBUFFER7 y, DWORD z, DWORD w, DWORD q)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::DrawPrimitiveVB()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);
		
	auto p = DDRAW::is_proxy<IDirect3DVertexBuffer7>(y);

	if(p) y = p->proxy;

	dx_ddraw_level();

	HRESULT out = proxy->DrawPrimitiveVB(x,y,z,w,q);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirect3DDevice7::DrawIndexedPrimitiveVB(DX::D3DPRIMITIVETYPE x, DX::LPDIRECT3DVERTEXBUFFER7 y, DWORD z, DWORD w, LPWORD q, DWORD r, DWORD s)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::DrawIndexedPrimitiveVB()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_DRAWINDEXEDPRIMITIVEVB,IDirect3DDevice7*,
	DX::D3DPRIMITIVETYPE&,DX::LPDIRECT3DVERTEXBUFFER7&,DWORD&,DWORD&,LPWORD&,DWORD&,DWORD&)(0,this,x,y,z,w,q,r,s);

	auto p = DDRAW::is_proxy<IDirect3DVertexBuffer7>(y);

	if(p) y = p->proxy;

	dx_ddraw_level();

	out = proxy->DrawIndexedPrimitiveVB(x,y,z,w,q,r,s);

	DDRAW_POP_HACK(DIRECT3DDEVICE7_DRAWINDEXEDPRIMITIVEVB,IDirect3DDevice7*,
	DX::D3DPRIMITIVETYPE&,DX::LPDIRECT3DVERTEXBUFFER7&,DWORD&,DWORD&,LPWORD&,DWORD&,DWORD&)(&out,this,x,y,z,w,q,r,s);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirect3DDevice7::ComputeSphereVisibility(DX::LPD3DVECTOR x, LPD3DVALUE y, DWORD z, DWORD w, LPDWORD q)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::ComputeSphereVisibility()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);
	
	return proxy->ComputeSphereVisibility(x,y,z,w,q);
}
HRESULT DDRAW::IDirect3DDevice7::GetTexture(DWORD x, DX::LPDIRECTDRAWSURFACE7 *y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::GetTexture()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);
		
	HRESULT out = proxy->GetTexture(x,y);

	if(out==D3D_OK&&*y&&DDRAW::doIDirectDrawSurface7)
	{
		DDRAW::IDirectDrawSurface7 *p; 
		
		p = DDRAW::IDirectDrawSurface7::get_head->get(*y);

		if(!p) //impossible?
		{
			assert(0);
			p = new DDRAW::IDirectDrawSurface7('dx7a');
			p->proxy = *y; 
		}
		else p->clientrefs++; //NEW 		
		*y = (DX::IDirectDrawSurface7*)p; 
	}
	
	DDRAW_RETURN(out)  
}
HRESULT DDRAW::IDirect3DDevice7::SetTexture(DWORD x, DX::LPDIRECTDRAWSURFACE7 y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::SetTexture()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK); 
	
	if(x>=DDRAW::textures){	assert(!y); return D3D_OK; }

//	DDRAW_LEVEL(7) << " Texture is " << x << " (surface is " << y << ")\n";

	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_SETTEXTURE,IDirect3DDevice7*,
	DWORD&,DX::LPDIRECTDRAWSURFACE7&)(0,this,x,y);
	
	auto p = DDRAW::is_proxy<IDirectDrawSurface7>(y);
	
	if(p) y = p->proxy;
	
	out = proxy->SetTexture(x,y);

	DDRAW_POP_HACK(DIRECT3DDEVICE7_SETTEXTURE,IDirect3DDevice7*,
	DWORD&,DX::LPDIRECTDRAWSURFACE7&)(&out,this,x,y);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirect3DDevice7::GetTextureStageState(DWORD x, DX::D3DTEXTURESTAGESTATETYPE y, LPDWORD z)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::GetTextureStageState()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);
	
	return proxy->GetTextureStageState(x,y,z);
}
HRESULT DDRAW::IDirect3DDevice7::SetTextureStageState(DWORD x, DX::D3DTEXTURESTAGESTATETYPE y, DWORD z)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::SetTextureStageState()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK); 
	
	if(x>=DDRAW::textures){ assert(0); return D3D_OK; }
	
	DDRAW_LEVEL(7) << " Stage is " << x << " (" << y << " set to " << z << ")\n";

	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_SETTEXTURESTAGESTATE,IDirect3DDevice7*,
	DWORD&,DX::D3DTEXTURESTAGESTATETYPE&,DWORD&)(0,this,x,y,z);

	out = proxy->SetTextureStageState(x,y,z);

	DDRAW_POP_HACK(DIRECT3DDEVICE7_SETTEXTURESTAGESTATE,IDirect3DDevice7*,
	DWORD&,DX::D3DTEXTURESTAGESTATETYPE&,DWORD&)(&out,this,x,y,z);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirect3DDevice7::ValidateDevice(LPDWORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::ValidateDevice()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);
	
	return proxy->ValidateDevice(x);
}
HRESULT DDRAW::IDirect3DDevice7::ApplyStateBlock(DWORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::ApplyStateBlock()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);
	
	HRESULT out = proxy->ApplyStateBlock(x);

	DDRAW::ApplyStateBlock();

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirect3DDevice7::CaptureStateBlock(DWORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::CaptureStateBlock()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);
	
	return proxy->CaptureStateBlock(x);
}
HRESULT DDRAW::IDirect3DDevice7::DeleteStateBlock(DWORD x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::DeleteStateBlock()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);
	
	return proxy->DeleteStateBlock(x);
}
HRESULT DDRAW::IDirect3DDevice7::CreateStateBlock(DX::D3DSTATEBLOCKTYPE x, LPDWORD y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::CreateStateBlock()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);
	
	return proxy->CreateStateBlock(x,y);
}
HRESULT DDRAW::IDirect3DDevice7::Load(DX::LPDIRECTDRAWSURFACE7 x, LPPOINT y, DX::LPDIRECTDRAWSURFACE7 z, LPRECT w, DWORD q)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::Load()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	auto p = DDRAW::is_proxy<IDirectDrawSurface7>(x);
	
	if(p) x = p->proxy;

	p = DDRAW::is_proxy<IDirectDrawSurface7>(z);
	
	if(p) z = p->proxy;

	return proxy->Load(x,y,z,w,q);
}
HRESULT DDRAW::IDirect3DDevice7::LightEnable(DWORD x, BOOL y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::LightEnable()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_LIGHTENABLE,IDirect3DDevice7*,
	DWORD&,BOOL&)(0,this,x,y);
	if((long)x<0) DDRAW_FINISH(D3D_OK); //cancelled?

	DDRAW_LEVEL(0) << ' ' << x << ' ' << (y?"on":"off") << '\n';
	
	out = proxy->LightEnable(x,y);

	if(out==D3D_OK)
	{
		DWORD *lost = 0;

		DDRAW::LightEnable(x,y,&lost);

		if(lost)
		{
			proxy->LightEnable(*lost,0); //assuming desired

			if(DDRAW::lightlost) DDRAW::lightlost(*lost,0);
		}
	}

	DDRAW_POP_HACK(DIRECT3DDEVICE7_LIGHTENABLE,IDirect3DDevice7*,
	DWORD&,BOOL&)(&out,this,x,y);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirect3DDevice7::GetLightEnable(DWORD x, BOOL *y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::GetLightEnable()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	return proxy->GetLightEnable(x,y);
}
HRESULT DDRAW::IDirect3DDevice7::SetClipPlane(DWORD x, DX::D3DVALUE *y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::SetClipPlane()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	return proxy->SetClipPlane(x,y); 
}
HRESULT DDRAW::IDirect3DDevice7::GetClipPlane(DWORD x, DX::D3DVALUE *y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::GetClipPlane()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	return proxy->GetClipPlane(x,y);
}
HRESULT DDRAW::IDirect3DDevice7::GetInfo(DWORD x, LPVOID y, DWORD z)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::GetInfo()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	return proxy->GetInfo(x,y,z);
}





HRESULT DDRAW::IDirect3DVertexBuffer7::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DVertexBuffer7::QueryInterface()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	if(7<=DDRAWLOG::debug)
	{
		LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';
	}

	assert(0);

	return proxy->QueryInterface(riid,ppvObj);
}
ULONG DDRAW::IDirect3DVertexBuffer7::AddRef()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DVertexBuffer7::AddRef()\n";

	DDRAW_ADDREF_RETURN(0)
}
ULONG DDRAW::IDirect3DVertexBuffer7::Release()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DVertexBuffer7::Release()\n";

	DDRAW_IF_NOT_TARGET_AND_RELEASE(out,0)

	if(out==0) delete this; 

	return out;
}
HRESULT DDRAW::IDirect3DVertexBuffer7::Lock(DWORD x, LPVOID *y, LPDWORD z)
{
	//x: DDLOCK_DISCARDCONTENTS|DDLOCK_NOSYSLOCK|DDLOCK_WRITEONLY

	DDRAW_LEVEL(7) << "DDRAW::IDirect3DVertexBuffer7::Lock()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	HRESULT out = proxy->Lock(x,y,z);

	DDRAW_RETURN(out) 
}
HRESULT DDRAW::IDirect3DVertexBuffer7::Unlock()
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DVertexBuffer7::Unlock()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	return proxy->Unlock();
}
HRESULT DDRAW::IDirect3DVertexBuffer7::ProcessVertices(DWORD x, DWORD y, DWORD z, DX::LPDIRECT3DVERTEXBUFFER7 w, DWORD q, DX::LPDIRECT3DDEVICE7 r, DWORD s)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DVertexBuffer7::ProcessVertices()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	return proxy->ProcessVertices(x,y,z,w,q,r,s);
}
HRESULT DDRAW::IDirect3DVertexBuffer7::GetVertexBufferDesc(DX::LPD3DVERTEXBUFFERDESC x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DVertexBuffer7::GetVertexBufferDesc()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	return proxy->GetVertexBufferDesc(x);
}
HRESULT DDRAW::IDirect3DVertexBuffer7::Optimize(DX::LPDIRECT3DDEVICE7 x, DWORD y)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DVertexBuffer7::Optimize()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	return proxy->Optimize(x,y);
}
HRESULT DDRAW::IDirect3DVertexBuffer7::ProcessVerticesStrided(DWORD x, DWORD y, DWORD z, DX::LPD3DDRAWPRIMITIVESTRIDEDDATA w, DWORD q, DX::LPDIRECT3DDEVICE7 r, DWORD s)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DVertexBuffer7::ProcessVerticesStrided()\n";

	DDRAW_IF_NOT_DX7A_RETURN(!D3D_OK);

	return proxy->ProcessVerticesStrided(x,y,z,w,q,r,s);
}




#define DDRAW_TABLES(Interface)\
void *DDRAW::Interface::vtables[DDRAW::MAX_TABLES] = {0,0,0,0};\
void *DDRAW::Interface::dtables[DDRAW::MAX_TABLES] = {0,0,0,0};
	
DDRAW_TABLES(IDirectDraw)
DDRAW_TABLES(IDirectDraw4)
DDRAW_TABLES(IDirectDraw7)
DDRAW_TABLES(IDirectDrawClipper)
DDRAW_TABLES(IDirectDrawPalette)
DDRAW_TABLES(IDirectDrawGammaControl)
DDRAW_TABLES(IDirectDrawSurface)
DDRAW_TABLES(IDirectDrawSurface4)
DDRAW_TABLES(IDirectDrawSurface7)
DDRAW_TABLES(IDirect3DTexture2)
DDRAW_TABLES(IDirect3DLight)
DDRAW_TABLES(IDirect3DMaterial3)
DDRAW_TABLES(IDirect3DViewport3)
DDRAW_TABLES(IDirect3D3)
DDRAW_TABLES(IDirect3D7)
DDRAW_TABLES(IDirect3DDevice3) 
DDRAW_TABLES(IDirect3DDevice7) 
//DDRAW_TABLES(IDirect3DVertexBuffer)
DDRAW_TABLES(IDirect3DVertexBuffer7)

#undef DDRAW_TABLES

namespace D3D9C
{
	extern int is_needed_to_initialize();
	extern HRESULT WINAPI DirectDrawCreateEx(GUID*,LPVOID*,REFIID,IUnknown*);
	extern HRESULT WINAPI DirectDrawEnumerateExA(DX::LPDDENUMCALLBACKEXA,LPVOID,DWORD);
}

//REMOVE ME?
extern int DDRAW::is_needed_to_initialize()
{
	static bool one_off = false; 
	if(one_off) return 1; one_off = true;

	DX::is_needed_to_initialize(); 

#define DDRAW_TABLES(Interface) DDRAW::Interface().register_tables('_dx6');

	DDRAW_TABLES(IDirectDraw)
	DDRAW_TABLES(IDirectDraw4)
	DDRAW_TABLES(IDirectDrawClipper)
	DDRAW_TABLES(IDirectDrawPalette)
	DDRAW_TABLES(IDirectDrawGammaControl)
	DDRAW_TABLES(IDirectDrawSurface)
	DDRAW_TABLES(IDirectDrawSurface4)
	DDRAW_TABLES(IDirect3DTexture2)
	DDRAW_TABLES(IDirect3DLight)
	DDRAW_TABLES(IDirect3DMaterial3)
	DDRAW_TABLES(IDirect3DViewport3)
	DDRAW_TABLES(IDirect3D3)
//	DDRAW_TABLES(IDirect3DVertexBuffer)
	DDRAW_TABLES(IDirect3DDevice3) 

#define DDRAW_TABLES(Interface) DDRAW::Interface().register_tables('dx7a');

	DDRAW_TABLES(IDirectDraw)
	DDRAW_TABLES(IDirectDraw4)
	DDRAW_TABLES(IDirectDraw7)
	DDRAW_TABLES(IDirectDrawClipper)
	DDRAW_TABLES(IDirectDrawPalette)
	DDRAW_TABLES(IDirectDrawGammaControl)
	DDRAW_TABLES(IDirectDrawSurface)
	DDRAW_TABLES(IDirectDrawSurface4)
	DDRAW_TABLES(IDirectDrawSurface7)
	DDRAW_TABLES(IDirect3DTexture2)
	DDRAW_TABLES(IDirect3DLight)
	DDRAW_TABLES(IDirect3DMaterial3)
	DDRAW_TABLES(IDirect3DViewport3)
	DDRAW_TABLES(IDirect3D3)
	DDRAW_TABLES(IDirect3D7)
	DDRAW_TABLES(IDirect3DDevice3) 
	DDRAW_TABLES(IDirect3DDevice7) 
//	DDRAW_TABLES(IDirect3DVertexBuffer)
	DDRAW_TABLES(IDirect3DVertexBuffer7)

#undef DDRAW_TABLES

	for(int i=0;i<256;i++) //beats junk
	{
		DDRAW::gammaramp.red[i] = i<<8;
		DDRAW::gammaramp.green[i] = i<<8;
		DDRAW::gammaramp.blue[i] = i<<8;
	}

	//2021: this is zeroed when using the 
	//D3D12 back buffer for interop reasons
	DDRAW::target_backbuffer = DDRAW::target;

	switch(DDRAW::target)
	{
	case 'dx9c':
	case 'dx10': //reserved
	case 'dx95': //ANGLE
	case 'dxGL': //opengl32
		//2021: this tells apps the feature level
		//is compatible with D3D9 (i.e. OpenGL ES 2)
		//in case target is D3D10, etc.
		DDRAW::compat = 'dx9c';
		D3D9C::is_needed_to_initialize();
	}

	DDRAW_HELLO(0) << "DirectDraw initialized\n";

	return 1; //for static thunks //???
}	  

extern bool dx_d3d9c_effectsshader_toggle;
extern void DDRAW::multicasting_dinput_key_engaged(unsigned char keycode)
{
	switch(keycode)
	{
	case 0xC5: case 0xA2: //DIK_PAUSE/DIK_PLAYPAUSE

		if(DDRAW::doPause) DDRAW::isPaused = !DDRAW::isPaused; break;

	case 0x29: //DIK_GRAVE

		if(DX::debug) dx_d3d9c_effectsshader_toggle = !dx_d3d9c_effectsshader_toggle;
		break;
	}
}

namespace DDRAW
{
	static HRESULT (WINAPI *DirectDrawCreateEx)(GUID*,LPVOID*,REFIID,IUnknown*) = 0;  
	static HRESULT (WINAPI *DirectDrawEnumerateExA)(LPDDENUMCALLBACKEXA,LPVOID,DWORD) = 0;
	//For the DirectX 6 pseudo target
	static HRESULT (WINAPI *DirectDrawCreate)(GUID*,LPDIRECTDRAW*,IUnknown*) = 0;
	static HRESULT (WINAPI *DirectDrawEnumerateA)(LPDDENUMCALLBACKA,LPVOID) = 0;
}
static BOOL CALLBACK dx_ddraw_vblankcb(GUID *guid, LPSTR a, LPSTR b, LPVOID passthru, HMONITOR hm)
{
	void **p = dx_ddraw_enum_set(passthru); 

	if(p&&hm==(HMONITOR)p[1]) //assuming any device on the monitor is good as the next
	{
		HRESULT paranoia = 
		DDRAW::DirectDrawCreateEx(guid,(LPVOID*)p[0],IID_IDirectDraw7,0);
		assert(paranoia==S_OK); //TODO: alert message
		return DDENUMRET_CANCEL;
	}

	return DDENUMRET_OK;
}
extern DX::IDirectDraw7 *DDRAW::vblank(HMONITOR mon)
{		
	DX::IDirectDraw7 *out = 0;

	void *passthru = dx_ddraw_reserve_enum_set(&out,(void*)mon);

	//TODO: consider adding DDENUM_DETACHEDSECONDARYDEVICES to the enumeration
	DDRAW::DirectDrawEnumerateExA(dx_ddraw_vblankcb,passthru,DDENUM_ATTACHEDSECONDARYDEVICES);

	dx_ddraw_return_enum_set(passthru); 
	
	return out;
}  
static HMONITOR dx_ddraw_monitorcb_found = 0;
static DWORD dx_ddraw_lastdDirectDrawEnumerateExAwFlags = 0; //hack
static BOOL CALLBACK dx_ddraw_monitorcb(GUID *guid, LPSTR a, LPSTR b, LPVOID lpGuid, HMONITOR hm)
{
	if(!guid) return D3DENUMRET_OK;
	
	if(*(GUID*)lpGuid==*guid)
	{
		dx_ddraw_monitorcb_found = hm; return D3DENUMRET_CANCEL;
	}
	else return D3DENUMRET_OK;	
}
static HMONITOR dx_ddraw_monitor(GUID *lpGuid)
{
	POINT vorigin = {0,0};
	HMONITOR primary = MonitorFromPoint(vorigin,MONITOR_DEFAULTTOPRIMARY);
	if(!lpGuid) return primary;

	dx_ddraw_monitorcb_found = 0;
	DDRAW::DirectDrawEnumerateExA(dx_ddraw_monitorcb,lpGuid,dx_ddraw_lastdDirectDrawEnumerateExAwFlags);
	if(!dx_ddraw_monitorcb_found) dx_ddraw_monitorcb_found = primary;

	return dx_ddraw_monitorcb_found;
} 
HRESULT WINAPI dx_ddraw_DirectDrawCreateEx(GUID *lpGuid, LPVOID *lplpDD, REFIID iid, IUnknown *pUnkouter)
{					
	//assert(0); //DirectShow(wavdest)?

	DDRAW::is_needed_to_initialize();

	DDRAW_LEVEL(7) << "dx_ddraw_DirectDrawCreateEx()\n";

	//experimental: movies?
	if(DDRAW::DirectDraw7&&iid==DX::IID_IDirectDraw7)
	{	
		//2021: if hitting this (unless the device is released) it's
		//a good indicator a movie isn't playing and maybe Release is
		//not letting go of DDRAW::DirectDraw7
		assert(DDRAW::Direct3DDevice7);

		//hack: first come first serve
		return DDRAW::DirectDrawCreateEx(lpGuid,lplpDD,iid,pUnkouter);
		//DDRAW_LEVEL(7) << " Proxy exists. Adding reference...\n"; 	
		//DDRAW::DirectDraw7->AddRef(); *lplpDD = DDRAW::DirectDraw7; 
		//return DD_OK;
	}

	if(7<=DDRAWLOG::debug)
	{
		OLECHAR g[64]; if(lpGuid) StringFromGUID2(*lpGuid,g,64); //,,,
	
		if(lpGuid) DDRAW_LEVEL(7) << ' ' << g << '\n'; //the display device??

		LPOLESTR w; if(StringFromIID(iid,&w)==S_OK) DDRAW_LEVEL(7) << ' ' << w << '\n';
	}

	if(DDRAW::compat=='dx9c') 
	return D3D9C::DirectDrawCreateEx(lpGuid,lplpDD,iid,pUnkouter);

	HRESULT out = !DD_OK; IDirectDraw7 *q = 0; assert(iid==IID_IDirectDraw7);

	//warning! CreateEx will only work with IID_IDirectDraw7
	out = DDRAW::DirectDrawCreateEx(lpGuid,(VOID**)&q,IID_IDirectDraw7,pUnkouter); 
	
	if(out!=DD_OK) DDRAW_ALERT(0) << "ALERT! DirectDrawCreateEx FAILED\n"; 
	if(out!=DD_OK) DDRAW_RETURN(out)  

	DDRAW::IDirectDraw7 *p = new DDRAW::IDirectDraw7('dx7a'); p->proxy7 = q; 

	if(iid==DX::IID_IDirectDraw7) //{15E65EC0-3B9C-11D2-B92F-00609797EA5B}
	{
		DDRAW_LEVEL(7) << " (IDirectDraw7)\n";

		if(DDRAW::doIDirectDraw7) *lplpDD = p; else *lplpDD = q;
	}
	else //this would be really problematic
	{
		DDRAW_ERROR(-1) 
		<< "ERROR: DirectDrawCreateEx() creating non-IDirectDraw7 interface\n"; 
		out = E_NOINTERFACE; 
	}

	if(out!=S_OK)
	{
		if(q) q->Release();		
		if(p) p->proxy = 0; delete p;
	}
	else 
	{
		assert(!DDRAW::DirectDraw7);
		if(iid==DX::IID_IDirectDraw7) p->AddRef();			 
		DDRAW::DirectDraw7 = p;
		if(p->GetMonitorFrequency(&DDRAW::refreshrate)) //2021
		assert(0);
		assert(DDRAW::refreshrate);
	}

	if(out==S_OK) DDRAW::monitor = dx_ddraw_monitor(lpGuid);

	DDRAW_RETURN(out) 
}  
static HRESULT dx_ddraw_directdrawcreate6(GUID *lpGUID, LPDIRECTDRAW *lplpDD, IUnknown *pUnkOuter)
{
	DDRAW::is_needed_to_initialize(); 
		
	LPDIRECTDRAW q = 0; HRESULT out = 
	DDRAW::DirectDrawCreate(lpGUID,&q,pUnkOuter);

	if(out==DD_OK)
	if(DDRAW::doIDirectDraw)
	{
		DDRAW::IDirectDraw *p = new DDRAW::IDirectDraw('_dx6');

		p->proxy = (DX::LPDIRECTDRAW)q; *lplpDD = (LPDIRECTDRAW)p;
	}
	else *lplpDD = q;
		
	DDRAW_RETURN(out) 									   
} 
static HRESULT dx_ddraw_directdrawenumeratea6(LPDDENUMCALLBACKA lpCallback, LPVOID lpContext)
{
	//TODO: some intervention here might be appropriate
	return DDRAW::DirectDrawEnumerateA(lpCallback,lpContext);
}
static HRESULT WINAPI dx_ddraw_DirectDrawCreate(GUID *lpGUID, LPDIRECTDRAW *lplpDD, IUnknown *pUnkOuter)
{
	//assert(0); //DirectShow(wavdest)?

	DDRAW::is_needed_to_initialize();

	DDRAW_LEVEL(7) << "dx_ddraw_DirectDrawCreate()\n";

	if(DDRAW::target=='_dx6') return dx_ddraw_directdrawcreate6(lpGUID,lplpDD,pUnkOuter);

	DX::LPDIRECTDRAW7 q = 0; HRESULT out = 
	DirectDrawCreateEx(lpGUID,(LPVOID*)&q,IID_IDirectDraw7,pUnkOuter);

	if(out!=S_OK) DDRAW_ALERT(0) << "ALERT! DirectDrawCreate FAILED\n"; 
	if(out!=S_OK) DDRAW_RETURN(out) 

	auto p = DDRAW::is_proxy<DDRAW::IDirectDraw7>(q);

	if(!p||!DDRAW::doIDirectDraw) 
	{
		assert(p&&DDRAW::target!='dx9c'); //did someone set doIDirectDraw7 off?!

		if(!p) DDRAW_PANIC(-1) << "PANIC! DirectDrawCreate just passing thru...\n";

		out = q->QueryInterface(::IID_IDirectDraw,(LPVOID*)lplpDD);
	}
	else out = p->QueryInterface(DX::IID_IDirectDraw,(LPVOID*)lplpDD);
	
	if(out!=S_OK) DDRAW_ALERT(0) << "ALERT! DirectDrawCreate FAILED (2)\n"; 

	ULONG paranoia = p?p->Release():1; assert(paranoia==1); //timing matters

	DDRAW_RETURN(out) 
}
static HRESULT WINAPI dx_ddraw_DirectDrawEnumerateExA(LPDDENUMCALLBACKEXA lpCallback, LPVOID lpContext, DWORD dwFlags)
{
	//assert(0); //DirectShow(wavdest)?

	//NEW: require EXE is source
	MEMORY_BASIC_INFORMATION mbi;
	if(!VirtualQuery((void*)0x401000,&mbi,sizeof(mbi))) assert(0);
	if(mbi.AllocationBase!=(void*)0x400000
	||(void*)lpCallback<mbi.BaseAddress||(char*)lpCallback>=(char*)mbi.BaseAddress+mbi.RegionSize)
	{
		DDRAW_ALERT(0) << "(ALERT: Ignoring DirectDrawEnumerateExA originating from DLL)\n"; //2021

		return DDRAW::DirectDrawEnumerateExA(lpCallback,lpContext,dwFlags);
	}
	
	DDRAW::is_needed_to_initialize(); //REMOVE ME?

	DDRAW_LEVEL(7) << "dx_ddraw_DirectDrawEnumerateExA()\n";

	if(DDRAW::compat=='dx9c') 
	return D3D9C::DirectDrawEnumerateExA(lpCallback,lpContext,dwFlags);	

	dx_ddraw_lastdDirectDrawEnumerateExAwFlags = dwFlags;

	void *passthru = dx_ddraw_reserve_enum_set(lpCallback,lpContext);

	HRESULT out = DDRAW::DirectDrawEnumerateExA(dx_ddraw_directdrawenumerateexacb,passthru,dwFlags);

	dx_ddraw_return_enum_set(passthru); DDRAW_RETURN(out) 
}
static HRESULT WINAPI dx_ddraw_DirectDrawEnumerateA(LPDDENUMCALLBACKA lpCallback, LPVOID lpContext)
{
	//assert(0); //DirectShow(wavdest)?

	//NEW: require EXE is source
	MEMORY_BASIC_INFORMATION mbi;
	if(!VirtualQuery((void*)0x401000,&mbi,sizeof(mbi))) assert(0);
	if(mbi.AllocationBase!=(void*)0x400000
	||(void*)lpCallback<mbi.BaseAddress||(char*)lpCallback>=(char*)mbi.BaseAddress+mbi.RegionSize)
	{
		DDRAW_ALERT(0) << "(ALERT: Ignoring DirectDrawEnumerateA originating from DLL)\n"; //2021

		return DDRAW::DirectDrawEnumerateA(lpCallback,lpContext);
	}

	DDRAW::is_needed_to_initialize(); //REMOVE ME?

	DDRAW_LEVEL(7) << "dx_ddraw_DirectDrawEnumerateA()\n"; 

	if(DDRAW::target=='_dx6') return dx_ddraw_directdrawenumeratea6(lpCallback,lpContext);

	LPDDENUMCALLBACKEXA assuming_safe = (LPDDENUMCALLBACKEXA)lpCallback;

	return DirectDrawEnumerateExA(assuming_safe,lpContext,0);
}
static void dx_ddraw_detours(LONG (WINAPI *f)(PVOID*,PVOID))
{	
		//this was not necessary prior to adding the DirectShow (wavdest)
		//static libraries to the mix. it seemed to fail even then, until
		//I noticed the wavdest project was using __stdcall by way of its
		//MSVC project file. could be just a coincidence
		HMODULE dll = GetModuleHandleA("ddraw.dll");
		if(!dll) return; //not needed?
		//#define _(x) x
		#define _(x) GetProcAddress(dll,#x)
	if(!DDRAW::DirectDrawCreateEx)
	{
		(void*&)DDRAW::DirectDrawCreateEx = _(DirectDrawCreateEx);
		(void*&)DDRAW::DirectDrawEnumerateExA = _(DirectDrawEnumerateExA);
		(void*&)DDRAW::DirectDrawCreate = _(DirectDrawCreate);
		(void*&)DDRAW::DirectDrawEnumerateA = _(DirectDrawEnumerateA);
	}
		#undef _
	f(&(PVOID&)DDRAW::DirectDrawCreateEx,dx_ddraw_DirectDrawCreateEx);	
	f(&(PVOID&)DDRAW::DirectDrawEnumerateExA,dx_ddraw_DirectDrawEnumerateExA);	
	f(&(PVOID&)DDRAW::DirectDrawCreate,dx_ddraw_DirectDrawCreate);	
	f(&(PVOID&)DDRAW::DirectDrawEnumerateA,dx_ddraw_DirectDrawEnumerateA);	

}//register dx_ddraw_detours
static int dx_ddraw_detouring = DX::detouring(dx_ddraw_detours);