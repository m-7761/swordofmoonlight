
#ifndef DX_DDRAW_INCLUDED
#define DX_DDRAW_INCLUDED

#ifdef DDRAW_OVERLOADS
#define D3D_OVERLOADS
#endif

typedef enum _D3DFORMAT D3DFORMAT; //D3D9

namespace DX
{
	//MSV2013 requires these to be defined 
	//separately
	struct _D3DMATRIX;
	typedef _D3DMATRIX D3DMATRIX;
}

struct ID3D12Device;
struct ID3D12Resource;
struct IDirect3DSurface9;

namespace DDRAW
{
	//2022: disable OpenGL
	//unless Nvidia and D3D11
	//feature level
	extern bool doNvidiaOpenGL;

	extern bool doMultiThreaded;

	extern bool midFlip; //mutex

	//2021 (August)
	//I think a Windows Update made
	//Present/PresentEx enforce the
	//docs language that specifying
	//a rectangle requires the COPY
	//mode. so D3DSWAPEFFECT_DISCARD 
	//is opt in (prefer doFlipEx)
	extern bool doFlipDiscard;

	//Windows 7+
	//D3DSWAPEFFECT_FLIPEX
	//turn this on for real-time
	//(don't set after CreateDevice)
	extern bool doFlipEx; 
	extern bool WGL_NV_DX_interop2;
	//EXPERIMENTAL/UNUSED
	//THIS SEEMED TO WORK LIKE A CHARM
	//BUT THEN STOPPED HOLDING TO 60PFS
	//this is tuned for 60fps. If 
	//higher frame rates are needed
	//it'll need to be a variable, or
	//if end-users want to try tweaking
	//it
	enum{ doFlipEX_MaxTicks=15 };

	//2021: OpenXR interop business
	extern bool doD3D12;
	extern bool doD3D9on12;
	extern ID3D12Device *D3D12Device;
	/*REMOVE ME
	extern struct doD3D12_backbuffer
	{
		//EXPERIMENTAL (FAILURE)
		HANDLE nt_handle;
		ID3D12Resource *d3d12;
		IDirect3DSurface9 *d3d9;		

	}doD3D12_backbuffers[2+1];*/

	//constants
	extern HMODULE dll; //ddraw.dll
	static const char *library = "DirectX";	
	//2021: dx95 is ANGLE (OpenGL ES proxy) 
	//via Widgets 95. dxGL is OpenGL32 meant
	//for performance comparison with ANGLE's
	//implementation. these are compatible
	//with 'dx9c' patterns as long as you use
	//queryX instead of query9 and do not use
	//proxy9 (OpenGL uses integer handles)
	extern int 
	target, //'_dx6' 'dx7a' 'dx9c' (dx95 dxGL)
	target_backbuffer, //2021: same or 'dx12'
	compat, //2021: 'dx9c' (dx9c dx95 dxGL)	
	compat_glUniformBlockBinding_1st_of_6,
	compat_moving_target, //2022
	shader, //0 (default) 'ff' 'ps' 'vs+'
	sRGB; //2020: this (1) is textures only
	//EXPERIMENTAL
	//positive X offset in homogenous space
	//TO ACCOMMODATE ASSYMETRICAL EYES THIS
	//HAS TO BE REPLACED WITH TWO X/Y PAIRS
	//AND POSSIBLY OTHER DEGREES OF FREEDOM
	extern float stereo;	
	extern float stereo2;
	extern bool inStereo,
	//CreateDevice calls this once but when
	//"stereo" is changed it must be called
	//(e.g. to change focus or recalibrate)
	stereo_update_SetStreamSource(),	
	//these parameters are required to have
	//xr::effects (Widgets95) take over the
	//effects pass on D3D11's side since it
	//can't be done in OpenGL with OpenXR's
	//design (at least not with efficiency)
	stereo_toggle_OpenXR(bool toggle=true),
	//this is called after building shaders
//	stereo_enable_OpenXR(const char *hlsl, void *c, size_t),
	fxWGL_NV_DX_interop2(const char *hlsl, void *c, size_t),
	//2022: this is called before rendering
	//HACK: io accepts the game character's
	//origin and returns the end-user's. it
	//could cause feedback loop problems if
	//the character moved in response to it
	//a different design might be necessary
	stereo_locate_OpenXR(float nz, float fz, float io[4+3], float post[4]=0);
	//2022: if nonzero stereo_locate_OpenXR
	//fills out this memory with transforms
	extern float(*stereo_locate_OpenXR_mats)[4][4];
	extern unsigned
	textures, //required by client 
	dejagrate, //3D anti-aliasing frames
	dejagrate_divisor_for_OpenXR;
	//vsPresentState IS NOW UPDATED ON FLIP
	//SO THIS IS PROVIDED TO SET UP THE 1ST
	//FRAME, SINCE FLIP OCCURS AFTER RENDER
	extern void dejagrate_update_vsPresentState();	
	extern void dejagrate_update_psPresentState();
	extern DWORD
	aspectratio, //0 '4:3' '13:9'
	bitsperpixel, //32, 16
	resolution[2], //safemode resolution		
	refreshrate; //2021
	extern bool 	
	//2018: I CAN'T REMEMBER WHAT THE DIFFERENCE
	//BETWEEN THESE 3 WAS, BUT TODAY THEY'RE ALL
	//SAYING THE SAME THING!
	//fullscreen, //defaults to false		
	//inWindow,isExclusive,
	fullscreen,
	isStretched,isInterlaced,isSynchronized,
	doSoftware, //software device 
	doMSAA, //antialiasing (removed in dx.d3d9X.cpp)
	doWait, //wait for vertical blank
	do565, //16bit Color mode
	doFog, //block fogenable for shaders
	doWhite, //use white texture instead of none
	//do2nd and doGamma are using the same texture
	//slot in the effects buffer, so pick one for now
	//do2nd alternates between two scene buffer so that 
	//the client's effects shader must blend the two frames 
	doEffects, do2ndSceneBuffer, //doMipSceneBuffers
	doGamma, //experimental (removed in dx.d3d9X.cpp)
	doDither, 
	doMipmaps, 
	doSmooth, doSharp,
	doInvert, //inverted brightness model	
	//NOTE: using do2ndSceneBuffer instead
	doTripleBuffering,
	doSuperSampling; //EXPERIMENTAL (2021)

	extern float xrSuperSampling; //0~1
	extern int stereo_status_OpenXR();

	template<class T>
	inline void doSuperSamplingMul(T &x)
	{
		//NOTE: 3/2 is a problem for values
		//of 1
		//NOTE: 3/2 works best with 50% fuzz
		//(50% mipmap) on PSVR
		//TODO: need a way to opt out of 3/2
		if(doSuperSampling)
		if(DDRAW::inStereo) x = x*3/2; else x*=2; //DUPLICATE
	}
	template<class T>
	inline void doSuperSamplingMul(T &x, T &y)
	{
		doSuperSamplingMul(x); doSuperSamplingMul(y);
	}
	template<class T>
	inline void doSuperSamplingMul(T &x, T &y, T &z, T &w)
	{
		doSuperSamplingMul(x,y); doSuperSamplingMul(z,w);
	}
	inline void doSuperSamplingMul(RECT &x)
	{
		doSuperSamplingMul(x.left,x.top,x.right,x.bottom);
	}
	template<class T>
	inline void doSuperSamplingDiv(T &x)
	{
		if(doSuperSampling)
		if(DDRAW::inStereo) x = x*2/3; else x/=2; //DUPLICATE
	}
	template<class T>
	inline void doSuperSamplingDiv(T &x, T &y)
	{
		doSuperSamplingDiv(x); doSuperSamplingDiv(y);
	}
	template<class T>
	inline void doSuperSamplingDiv(T &x, T &y, T &z, T &w)
	{
		doSuperSamplingDiv(x,y); doSuperSamplingDiv(z,w);
	}
	inline void doSuperSamplingDiv(RECT &x)
	{
		doSuperSamplingDiv(x.left,x.top,x.right,x.bottom);
	}

	extern unsigned doMipSceneBuffers;

	//REMINDER: this is a debugging feature
	//bool: returns previous state (in use/not)
	extern bool toggleEffectsBuffer(int on=-1);

	//REMOVE US //hacks
	extern bool linear; //filter	
	extern int anisotropy;

	//viewports
	extern bool
	doScaling, //should be set false only temporarily 
	doMapping, //should be set false only temporarily
	doNothing; //if true, disable scaling/mapping	
	extern int
	xyMapping2[2]; //DDRAW::ps2ndSceneMipmapping2	
	extern float 
	xyMapping[2],//viewport origin
	xyScaling[2];
	extern DWORD xyMinimum[2]; //display mode enumeration	
		
	extern bool ff; //true if fallback shaders are unavailable
	extern bool gl; //true if DDRAW::target is 'dx9X' or 'dxGL'
	extern BOOL xr; //true if DDRAW::inStereo and using OpenXR

	//this sets GL_ARB_depth_clamp for artificial depth skybox
	extern void xr_depth_clamp(bool);

	//EXPERIMENTAL
	static float xyScalingQuantize(int i, float x)
	{
		 return int(x*xyScaling[i])/xyScaling[i];
	}
	void xyRemap2(RECT&, bool offset=!DDRAW::gl);
	void xyUnmap2(RECT&, bool offset=!DDRAW::gl);

	extern DWORD
	lighting[16+1]; //16 most recently enabled lights	
	extern int 
	lights, //number of enabled lights (up to 16)
	maxlights, //set between 0~16 (mainly for shaders)
	minlights; //ditto: additional lights set to 0

	extern bool isLit; //SetRenderState(LIGHTING)
	extern bool inFog; //true even if doFog is false

	extern DWORD *getDWORD; //get true value from a GetX interface

	extern DX::D3DMATRIX *getD3DMATRIX;

	//FYI: this is system textures I guess
	#define DDRAW_TEX0 (DDRAW::textures+0)
	#define DDRAW_TEX1 (DDRAW::textures+1)

	////////////////////////////////////////////////////////////////
	//bright/dimmer raise the white/black levels respectively without 
	//sacrificing detail. brightening/dimming are applied only if isLit
	//is true/applicable. a nonzero value in brights[bright] adds white
	//A nonzero value in dimmers[dimmer] adds black*
	//
	//*when DDRAW::doInvert everything works oppositely

	//Levels above 8 may or may not be clamped to 8	
	extern unsigned char brights[9+8], dimmers[9+8];	
	extern int bright, dimmer;

	////////////////////////////////////////////////////////////////

	//REMINDER: according to the MSDN documentation calling BeginScene
	//and EndScene more than once before Present is not intended but is
	//allowed... so this practice should be avoided ideally... I didn't
	//know that when I set it up, I assumed it was like glBegin/glEnd()
	extern bool	inScene,
	PushScene(), //safe BeginScene()
	PopScene(); //safe EndScene()

	extern unsigned
	noFlips,noTicks,flipTicks,fps; //ms duration of the last flip

	extern bool (*onFlip)(); //false short-circuits presentation
	extern void (*onEffects)();
									  
	extern bool doPause, isPaused, (*onPause)();
//	extern void (*ifPaused)();

	extern unsigned noResets;
	extern void (*onReset)(); //done after Direct3DDevice7 is set to 0
//	extern void (*ifReset)();

//	extern bool isLost; 
//	extern void (*onLoss)(), (*ifLost)();

	//vertex shader registers//
	//0 to not track the register
	//1 is the first float4 register. 
	// use DDRAW::vsF+n for addressing
	//these will all fit into the minimum
	// number of vs1_1 float registers (96)

	static const int vsF = 1; 

	//reserved registers
	static const int //NEW: dual use
	vsDebugColor = vsF+0, //shader debugging
	vsShaderState = vsF+0; //shader specific

	extern int
	vsPresentState,
	vsProj[4],
	vsWorldViewProj[4],
	vsWorldView[4],
	vsInvWorldView[4],
	vsWorld[4], 
	vsView[4], 
	vsInvView[4], 
	vsTexture0[4], 
	vsFogFactors, //1/(end-start)\end\density\enabled
	vsColorFactors,	//color/alpha ops
	vsMaterialAmbient,
	vsMaterialDiffuse, //D3DRS_COLORVERTEX: zero
	vsMaterialEmitted, 
	vsGlobalAmbient,	//w holds lighting on/off (1/0)
	vsLightsAmbient[16], 
	vsLightsDiffuse[16],
	vsLightsVectors[16], //point (w=1) / directional (w=0)
	vsLightsFactors[16]; //range/attenuation (dir=1,0,0)
		
	//registers <= vsI are floats
	//registers <= vsB are integers	(if vsI!=0)
	//otherwise assumed to be boolean (if vsB!=0)

	//2021: vsI is doubling as the maximum number
	//of float registers to reserve a buffer with
	//OpenGL
	extern int vsI, vsB; //disabled (0) by default

	//more vertex shader registers//	
	extern int vsLights; //number of lights (enabled)

	//pixel shader registers//

	static const int psF = 1; 

	//reserved registers
	static const int //NEW: dual use
	psDebugColor = psF+0, //shader debugging
	psShaderState = psF+0; //shader specific	
	static const int
	psPresentState  = psF+1, //1/w\1/h\fx\fy
	psColorFactors  = psF+2, //color/alpha select
	psColorFunction = psF+3, //texture+color function	
	psCorrections   = psF+7, //bright/dim //psCorrectionsXY	
	//making optional below
	psColorkey_nz   = psF+8, //w=tex0 has ckey?1:0 XYZ unused!		
	psConstants10 = psF+10, //2023: general purpose 10-19			
	psFogFactors = psF+24, //1/(end-start)\end\density\enabled
	psFogColor   = psF+25; //extern

	//NOTE: these aren't const so they can be zeroed
	extern int
	psColorkey; //psColorkey_def
			   
	extern int //psF+26 
	psColorize, //receives fxColorize
	psFarFrustum; //back right corner+1/zFar
	//psTextureState; //w,h,1/w,1/h //UNUSED/EXPERIMENTAL
	enum{ psTextureState=0 };
	
	//EXPERIMENTAL
	//this is loaded with two 1 pixel UV offsets intended
	//to cause mipmap sampling to be off by one pixel on
	//one of the scene buffers so that the 2x2 grid pattern
	//is less apparent
	//NOTE: it must be coordinated with DDRAW::xyMapping2
	//DDRAW::xyMapping is automatically adjusted
	extern int ps2ndSceneMipmapping2;

	extern int psI, psB;
	
	//fx shaders (fullscreen blit) start at 16 and 
	//are pixel shaders, but a corresponding vertex
	//shader is used if available at the same vs index
	extern const DWORD *vshaders9[16+4]; extern int vs;
	extern const DWORD *pshaders9[16+4]; extern int ps, fx;		
	extern const char *vshadersGL_include;
	extern const char *pshadersGL_include;
	static auto &vshadersGL = (const char*(&)[20])vshaders9;
	static auto &pshadersGL = (const char*(&)[20])pshaders9;
	//REMOVE ME
	//REMINDER: this exists because of a seeming bug where
	//the dx_d3d9c_level values stored in shader registers
	//seem to become corrupted. I can't figure out it this
	//is programmer error or if the registers are volatile
	//in some way. ostensibly this patches ApplyStateBlock
	extern void refresh_state_dependent_shader_constants();
	extern void vshaders9_pshaders9_require_reevaluation();
	extern void fx9_effects_buffers_require_reevaluation();

	//NEW: sets vs/psShaderState or overrides a constant c
	extern void vset9(float *f, int registers=1, int c=vsF);
	extern void pset9(float *f, int registers=1, int c=psF);
	//2021: this is also for BOOL types, I'm really not sure
	//how BOOL (SetPixelShaderConstantB) even works. for int
	//it's 4 values that are used to describe a for loop, as
	//that's all they're really for. supposedly there are 16
	//ONE-DIMENSIONAL boolean registers for static branching
	extern void vset9(int i[4], int registers=1, int c=vsI);
	extern void pset9(int i[4], int registers=1, int c=psI);

	//REMOVE ME??
	//Sword of Moonlight flash
	//rgb is additive whitening, alpha is blackening effect	
	extern DWORD fxColorize;	
	//negative margin for InflateRect
	extern int fxInflateRect; 
	extern unsigned fxStrobe; //psPresentState.zw
	extern bool fx2ndSceneBuffer;
	extern float fxCounterMotion; //0~1
	
	//HACK: get psCorrections values for OpenXR path
	extern void fxCorrectionsXY(float[2]);
	
	//dx_d3d9c_d3ddrivercaps will set this capped at 4
	extern int d3d9cNumSimultaneousRTs;

	//MRT (Multiple Render Targets)
	//mrtargets9[0] is the effects/backbuffer format
	//if [1] through [3] are nonzero textures are managed
	//and can be enabled with D3DRS_COLORWRITEENABLE1,2&3
	//provided index 0 is bound to the effects/backbuffer
	//targets 1 through 3 must be set before CreateDevice
	extern D3DFORMAT mrtargets9[4],altargets9[4];
	
	//has Clear draw a quad
	//the color argument is used if ClearMRT is are 0
	//(ClearMRT MUST BE SET BEFORE dx_d3d9c_prepshaders IS CALLED)
	//
	// 2021: 1 must now be set before initializing, so
	// that shaders won't be generated if using mode 2
	// 2021: 2 uses Clear instead of shaders. it can't
	// set precisely to floating point values since it
	// has to use D3DCOLOR
	extern int doClearMRT; extern const char *ClearMRT[4];
}

#ifdef DIRECTX_INCLUDED

namespace DDRAWLOG
{
	static int debug = 0; //debugging
	static int error = 7; //serious errors
	static int panic = 7; //undefined
	static int alert = 7; //warning
	static int hello = 7; //fun stuff
	static int sorry = 7; //woops
	
	static int *master = DX_LOG(DDraw);
}

#define DDRAW_LEVEL(lv) if(lv<=DDRAWLOG::debug&&lv<=*DDRAWLOG::master) dx.log
#define DDRAW_ERROR(lv) if(lv<=DDRAWLOG::error&&lv<=*DDRAWLOG::master) dx.log
#define DDRAW_PANIC(lv) if(lv<=DDRAWLOG::panic&&lv<=*DDRAWLOG::master) dx.log
#define DDRAW_ALERT(lv) if(lv<=DDRAWLOG::alert&&lv<=*DDRAWLOG::master) dx.log
#define DDRAW_HELLO(lv) if(lv<=DDRAWLOG::hello&&lv<=*DDRAWLOG::master) dx.log
#define DDRAW_SORRY(lv) if(lv<=DDRAWLOG::sorry&&lv<=*DDRAWLOG::master) dx.log

static const HRESULT DDRAW_UNIMPLEMENTED = 2;

namespace DDRAW{ extern char *error(HRESULT); }

#ifdef _DEBUG
#define DDRAW_RETURN(statement){ HRESULT __ = (statement);\
	if(__) DDRAW_ALERT(2) << "DirectX interface returned error " << dx%DDRAW::error(__) << '(' << std::hex << __ << ')' << " in " << dx%__FILE__ << " at " << std::dec << __LINE__ << " (" << dx%__FUNCTION__ ")\n";\
		assert(!__||DDRAW::fullscreen); if(__==DDRAW_UNIMPLEMENTED) __ = S_OK; return __; }
#else 
#define DDRAW_RETURN(statement)\
{ HRESULT __ = (statement); if(__==DDRAW_UNIMPLEMENTED) __ = S_OK; return __; }
#endif
#define DDRAW_FINISH_(out,statement)\
{ out = (statement); if(out==DDRAW_UNIMPLEMENTED) out = S_OK; goto pophack; }
#define DDRAW_FINISH(statement) DDRAW_FINISH_(out,statement)

#endif //DIRECTX_INCLUDED

namespace DX
{
	#undef __DDRAW_INCLUDED__
	#undef _D3D_H_
	#undef _D3DTYPES_H_
	#undef _D3DCAPS_H

	#undef D3DMATRIX_DEFINED

	//see dx.dsound.h etc.
	#ifndef DX__DX_DEFINED
	#undef D3DCOLOR_DEFINED
	#undef D3DCOLORVALUE_DEFINED
	#undef D3DVALUE_DEFINED
	#undef D3DVECTOR_DEFINED
	#undef D3DRECT_DEFINED
	#undef DX_SHARED_DEFINES
	#define DX__DX_DEFINED
	#endif	

	#define DIRECT3D_VERSION 0x0700	//Sword of Moonlight

	struct IDirectDraw;
//	struct IDirectDraw2;
	struct IDirectDraw4;
	struct IDirectDraw7;
	struct IDirectDrawSurface;
//	struct IDirectDrawSurface2;
//	struct IDirectDrawSurface3;
	struct IDirectDrawSurface4;
	struct IDirectDrawSurface7;
	struct IDirectDrawPalette;
	struct IDirectDrawClipper;
	struct IDirectDrawColorControl;
	struct IDirectDrawGammaControl;

	struct _DDFXROP;
	struct _DDSURFACEDESC;
	struct _DDSURFACEDESC2;
	struct _DDCOLORCONTROL;

	#include "dx8.1/ddraw.h"
	#include "dx8.1/d3d.h" 

	#undef DIRECT3D_VERSION
}

struct IDirect3D9;
struct IDirect3D9Ex;

typedef struct _D3DMATRIX D3DMATRIX;
										 
namespace DDRAW
{	
	static const bool doIDirectDraw = 1;
	static const bool doIDirectDraw4 = 1;
	static const bool doIDirectDraw7 = 1;
	static const bool doIDirectDrawGammaControl = 1;
	static const bool doIDirectDrawSurface = 1;
	static const bool doIDirectDrawSurface4 = 1;
	static const bool doIDirectDrawSurface7 = 1;
	static const bool doIDirect3DTexture2 = 1;
	static const bool doIDirect3DLight = 1;
	static const bool doIDirect3DMaterial3 = 1;
	static const bool doIDirect3DViewport3 = 1;
	static const bool doIDirect3D3 = 1;
	static const bool doIDirect3D7 = 1;
	static const bool doIDirect3DDevice3 = 1;
	static const bool doIDirect3DDevice7 = 1;
	static const bool doIDirect3DVertexBuffer7 = 1;
						
	//REMOVE ME?
	extern int is_needed_to_initialize();

	extern void log_display_mode();
	extern void log_surface_desc(DX::DDSURFACEDESC2*);
	extern void log_surface_bltfx(DX::DDBLTFX*);
	extern void log_driver_caps(DX::DDCAPS_DX7 *hal, DX::DDCAPS_DX7 *hel);
	extern void log_device_desc(DX::D3DDEVICEDESC7*, const char *pre=0);
	extern void log_device_desc(DX::D3DDEVICEDESC*, const char *pre=0);
	extern void log_FVF(DWORD); //2021

	extern void multicasting_dinput_key_engaged(unsigned char keycode);

	extern HMONITOR monitor; //via DirectDrawCreateEx()

	extern HWND window; //via IDirectDraw7::SetCooperativeLevel()
	extern BOOL window_rect(RECT*,HWND=window);

	extern DX::DDGAMMARAMP gammaramp; //singular ramp for DirectX windows

	//user handler: returns the number of knockouts
	//2021: now every surface has a matching setting to delay loading
	extern int(*colorkey)(DX::DDSURFACEDESC2*,D3DFORMAT nativeformat); //alpha?

	//user handler: return false to stop mipmapping
	//2021: now every surface has a matching setting to delay loading
	//2021: the return type is now a new filter function or 0 to stop
	extern void*(*mipmap)(const DX::DDSURFACEDESC2*,DX::DDSURFACEDESC2*);
	
	//user handler: number of lights exceeded 16 notifier
	extern void(*lightlost)(DWORD iL, bool assumed_d3d_state);

	//user handler: light retrieval for shaders
	extern DX::D3DLIGHT7 *(*light)(DWORD iL, intptr_t clientdata);

	class IDirectDraw7; //DDRAW::

	extern DDRAW::IDirectDraw7 *DirectDraw7;

	class IDirect3DDevice7; 
	class IDirectDrawSurface7;

	extern DDRAW::IDirectDrawSurface7 *PrimarySurface7;
	extern DDRAW::IDirectDrawSurface7 *BackBufferSurface7; //2021
	extern DDRAW::IDirectDrawSurface7 *DepthStencilSurface7; //2021
	extern DDRAW::IDirectDrawSurface7 *TextureSurface7[8];

	extern DDRAW::IDirect3DDevice7 *Direct3DDevice7;
	
	#ifdef DDRAW_OVERLOADS
	static DX::D3DMATRIX Identity(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);
	#else
	static DX::D3DMATRIX Identity = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
	#endif

	static DX::D3DCOLOR DebugPalette[] = 
	{
		0xFF000000, 0xFFFFFFFF, 0x80808080, //black/white/grey		
		0xFFFF0000, 0xFF00FF00, 0xFF0000FF, //red/green/blue
		0xFF00FFFF, 0xFFFF00FF, 0xFFFFFF00, //cyan/magenta/yellow
		0xFF8080FF, 0xFFFF8080, 0xFF80FF80, //babyblue/pink/mint
	};

	extern bool SetDisplayMode(DWORD,DWORD,DWORD,DWORD,DWORD);

	extern bool RestoreDisplayMode();

	//LightEnable: track lighting via DDRAW::lighting/lights
	//lost is set if turning on a light causes a light to be
	//lost track of (because the light maximum was exceeded)
	extern bool LightEnable(DWORD,bool,DWORD**lost=0); 

	static int lights_killed = 0; //reviving_lights

	template<typename T>
	inline void killing_lights(T *switches=0)
	{
		lights_killed = 0;

		if(DDRAW::Direct3DDevice7)
		for(int i=DDRAW::lights-1;i>=0;i--)
		{
			int iL = DDRAW::lighting[i];

			if(DDRAW::Direct3DDevice7->LightEnable(iL,0)==D3D_OK)
			{
				if(switches) switches[iL] = false;

				lights_killed++;
			}
		}
	}

	template<typename T>
	inline void reviving_lights(T *switches=0)
	{
		int iL = DDRAW::lighting[0];

		if(DDRAW::Direct3DDevice7)
		for(int i=0;i<lights_killed;i++)
		{
			int iN = DDRAW::lighting[i+1];

			if(DDRAW::Direct3DDevice7->LightEnable(iL,1)==D3D_OK)
			{
				if(switches) switches[iL] = true;
			}

			iL = iN;
		}
	}

	//ofApplyStateBlock: true if in ApplyStateBlock (below)	
	extern bool ofApplyStateBlock;

	//ApplyStateBlock: see definition for list of managed states
	extern void ApplyStateBlock(); 

	extern void simulating_lost_device();

	//you want to Release() this interface when finished
	extern DX::IDirectDraw7 *vblank(HMONITOR=DDRAW::monitor);
	
	//2023: these create a large texture to hold smaller textures
	//they're developed to reduce draw calls for sorting triangles
	extern DDRAW::IDirectDrawSurface7 *CreateAtlasTexture(int=8192);
	extern void BltAtlasTexture(DDRAW::IDirectDrawSurface7*,DDRAW::IDirectDrawSurface7*,int border,int,int);

	//DDRAW_LEVEL(7) << "~I()\n";
	extern void Yelp(const char *I);

#ifdef _DEBUG

		//DDRAW_LEVEL(7) << This << '\n';
		extern void Here(void *This);
#else
		inline void Here(void*){}
#endif

	enum Enum
	{			
	DIRECT3D3_ENUMDEVICES_ENUM = 0,
	TOTAL_ENUMS
	};

	//intercept_callback: 
	//'f' is a pointer to a function which receives the callbacks
	//originating from 'e' followed by the arguments being passed like so
	//
	// HRESULT EnumDevicesCallback(LPD3DENUMDEVICESCALLBACK, 
	//			GUID*,LPSTR,LPSTR,LPD3DDEVICEDESC,LPD3DDEVICEDESC,LPVOID);
	//
	//'g' is purely hypothetical (in case any interface involves more than one callback)
	//
	//It is up to the intercepting routine to pass the callback thru to the original recipient
	//The returned value will be returned to the original caller	
	extern bool	intercept_callback(DDRAW::Enum e, void *f, void *g=0);

	enum Hack
	{			
	DIRECTDRAW_QUERYINTERFACE_HACK = 0,
	DIRECTDRAW_CREATESURFACE_HACK,
	DIRECTDRAW_SETDISPLAYMODE_HACK,
	DIRECTDRAW_RESTOREDISPLAYMODE_HACK,
	DIRECTDRAW4_QUERYINTERFACE_HACK,
	DIRECTDRAW4_CREATESURFACE_HACK,
	DIRECTDRAW4_SETCOOPERATIVELEVEL_HACK,
	DIRECTDRAW4_SETDISPLAYMODE_HACK,
	DIRECTDRAW4_RESTOREDISPLAYMODE_HACK,
	DIRECTDRAW7_QUERYINTERFACE_HACK,
	DIRECTDRAW7_CREATESURFACE_HACK,
	DIRECTDRAW7_SETCOOPERATIVELEVEL_HACK,
	DIRECTDRAW7_SETDISPLAYMODE_HACK,
	DIRECTDRAW7_RESTOREDISPLAYMODE_HACK,
	DIRECTDRAWGAMMACONTROL_SETGAMMARAMP_HACK,
	DIRECTDRAWSURFACE_QUERYINTERFACE_HACK,
	DIRECTDRAWSURFACE4_QUERYINTERFACE_HACK,
	DIRECTDRAWSURFACE4_BLT_HACK,
    DIRECTDRAWSURFACE4_BLTFAST_HACK = 
	DIRECTDRAWSURFACE4_BLT_HACK, 
	DIRECTDRAWSURFACE4_FLIP_HACK,
	DIRECTDRAWSURFACE4_GETDC_HACK,
	DIRECTDRAWSURFACE4_RELEASEDC_HACK,
	DIRECTDRAWSURFACE4_SETCOLORKEY_HACK,
	DIRECTDRAWSURFACE7_QUERYINTERFACE_HACK,
	DIRECTDRAWSURFACE7_BLT_HACK,
    DIRECTDRAWSURFACE7_BLTFAST_HACK = 
	DIRECTDRAWSURFACE7_BLT_HACK, 
	DIRECTDRAWSURFACE7_FLIP_HACK,
	DIRECTDRAWSURFACE7_SETCOLORKEY_HACK,
	DIRECT3DVIEWPORT3_SETVIEWPORT2_HACK,
	DIRECT3DVIEWPORT3_CLEAR2_HACK,
	DIRECT3D3_CREATEDEVICE_HACK,		
	DIRECT3D7_CREATEDEVICE_HACK,		
	DIRECT3D7_CREATEVERTEXBUFFER_HACK,
	DIRECT3DDEVICE3_BEGINSCENE_HACK,
    DIRECT3DDEVICE3_ENDSCENE_HACK,
	DIRECT3DDEVICE3_SETTRANSFORM_HACK,   
	DIRECT3DDEVICE3_GETTRANSFORM_HACK,
	DIRECT3DDEVICE3_SETRENDERSTATE_HACK,
	DIRECT3DDEVICE3_DRAWPRIMITIVE_HACK,
	DIRECT3DDEVICE3_DRAWINDEXEDPRIMITIVE_HACK,
	DIRECT3DDEVICE3_SETTEXTURESTAGESTATE_HACK,
	DIRECT3DDEVICE7_BEGINSCENE_HACK,
    DIRECT3DDEVICE7_ENDSCENE_HACK,
	DIRECT3DDEVICE7_CLEAR_HACK,
	DIRECT3DDEVICE7_SETTRANSFORM_HACK, 
	DIRECT3DDEVICE7_GETTRANSFORM_HACK,
	DIRECT3DDEVICE7_SETVIEWPORT_HACK,   
	DIRECT3DDEVICE7_SETLIGHT_HACK,   
	DIRECT3DDEVICE7_SETRENDERSTATE_HACK,
	DIRECT3DDEVICE7_GETRENDERSTATE_HACK,
	DIRECT3DDEVICE7_DRAWPRIMITIVE_HACK,
	DIRECT3DDEVICE7_DRAWINDEXEDPRIMITIVE_HACK,
	DIRECT3DDEVICE7_DRAWINDEXEDPRIMITIVEVB_HACK,
	DIRECT3DDEVICE7_SETTEXTURE_HACK,
	DIRECT3DDEVICE7_SETTEXTURESTAGESTATE_HACK,
	DIRECT3DDEVICE7_LIGHTENABLE_HACK,   
	DIRECT3DDEVICE7_SETMATERIAL_HACK, 
	DIRECT3DVERTEXBUFFER7_LOCK_HACK,	
	TOTAL_HACKS
	};

	//hack_interface: 
	//'f' is a pointer to a function which receives the interface
	//and all of the arguments for 'h' by address like so
	//
	// void *DrawPrimitive(HRESULT*,DDRAW::IDirect3DDevice7*, 
	//			D3DPRIMITIVETYPE&,DWORD&,LPVOID&,DWORD&,DWORD&);
	//
	//if another function pointer is returned it will be called after the interface
	extern void *hack_interface(DDRAW::Hack h, void *f);

	static const int MAX_TABLES = (1+3); //DX6, 7, 9/dx.d3dXr.cpp

	static inline int v(int t) //vtable index
	{
		switch(t) 
		{
		//optimized for is_proxy (below)

		case 'dxGL': //2021
		case 'dx95': //2021
		case 'dx9c': return 1; //DirectX 9 (dx.d3d9X.cpp)
		case 'dx7a': return 2; //DirectX 7
		case '_dx6': return 3; //DirectX 6

		default: //reserved
			
			assert(0); return 0; 
		}
	}

	template<class T> 
	T *is_proxy(void *p)
	{
		if(!p) return 0;
		void *v = *(void**)p; 
		for(int t=0;t<DDRAW::MAX_TABLES;t++)
		{
			if(T::vtables[t]==v&&v) return (T*)p;
		}		
		return 0; 		
	}

	template<class T> inline int referee(T *t, int i=0)
	{		
		if(t->clientrefs>0)
		{
			if(uintptr_t(t->proxy)>0xFF)
			{
				ULONG safe = t->clientrefs;

				if(i<0) //TODO: there's no reason to call proxy->Release/AddRef
				{	
					do safe=t->proxy->Release(); while(--t->clientrefs&&++i&&safe);							
				}
				else if(i>0) do safe=t->proxy->AddRef(); while(t->clientrefs++&&--i);			

				#ifndef D3D_DEBUG_INFO //winsanity
				assert(safe>=t->clientrefs); 
				#endif
			}
			else if(t->proxy) 
			{
				if(i<0) 
				{
					t->clientrefs-=min(int(t->clientrefs),-i);
				}
				else t->clientrefs+=i;
			}
			else //assert(t->proxy||DDRAW::gl); //bug zapper
			{
				//IDirectDrawGammaControl is 0 if doGamma is false
				//under 'dx7a'
			}
		}
		else assert(t->clientrefs==0);

		return t->clientrefs;
	}

	//Note: in process of phasing out dlist stuff
	template<class X, class Y> void u_dlists(X *x, Y *y)
	{
		if(!x||!x->dlist||!y||!y->dlist) return;

		if(y->dlist==(void**)y) //y is singleton
		{
			void **swap = x->dlist; 
			
			x->dlist = (void**)y; y->dlist = swap;
		}
		else assert(0); //unimplemented
	}			 
	template<class T> void x_dlist(T *p)
	{
		if(!p||!p->dlist) return;  

		typedef void**(*dtor)(void*);

		void **d = p->dlist;
		while(d!=(void**)p)	d = ((dtor)d[1])(d);
	}

	//destructible_t:
	//useful for notification of release of an interface
	template<typename T> struct destructible_t
	{			
		T *tptr;		
		void **(*dtor)(destructible_t<T>*);
		void **dlist;

		inline operator void**(){ return (void**)this; }

		inline destructible_t(void **(*d)(destructible_t<T>*), T *t=0)
		{
			dtor = d; tptr = t; dlist = *this;
		}		

		static void **destruct(destructible_t<T> *d)
		{
			void **out = d->dlist; d->dlist = 0; delete d; return out;
		}
	};
}

#pragma push_macro("PURE") 
#pragma push_macro("STDMETHOD")
#pragma push_macro("STDMETHOD_") //for DirectX headers compatability

#define PURE
#define STDMETHOD(method) HRESULT STDMETHODCALLTYPE method //virtual HRESULT STDMETHODCALLTYPE method
#define	STDMETHOD_(type,method) type STDMETHODCALLTYPE method //virtual type STDMETHODCALLTYPE method

#define DDRAW_INTRAFACE(Intraface)\
public:\
\
	void register_tables(int t)\
	{\
		vtables[DDRAW::v(t)] = *(void**)this;\
		dtables[DDRAW::v(t)] = Intraface::destruct;\
	}\
\
	static void **destruct(Intraface *d)\
	{\
		void **out = d->dlist; d->dlist = 0; delete d; return out;\
	}\

#define DDRAW_INTERFACE(Interface)\
public:\
\
	DDRAW_INTRAFACE(Interface)\
\
	static void *vtables[DDRAW::MAX_TABLES];\
	static void *dtables[DDRAW::MAX_TABLES];\
\
	void **(*dtor)(void*); /*important! expected to be first member*/\
	void **dlist;\
\
	const int target; /*const for now*/\
\
	ULONG clientrefs; /*not for client use*/\
\
public: Interface():target(0),dtor(0),dlist(0){}

#define DDRAW_CONSTRUCT(Interface)\
\
		assert(target==DDRAW::target);\
\
		*(void**)this = vtables[DDRAW::v(t)];\
\
		dtor = (void**(*)(void*))dtables[DDRAW::v(t)];\
		dlist = (void**)this; clientrefs = 1;\
\
		DDRAW::Here(this); //_DEBUG only

#define DDRAW_DESTRUCT(Interface)\
\
		DDRAW::Yelp(#Interface);\
\
		DDRAW::x_dlist(this);

namespace DDRAW{
template<class T> union qtable_t
{
	typename T::Query qbase;
	typename T::Query6 _dx6;
	typename T::Query7 dx7a;
	typename T::Query9 dx9c;
	typename T::QueryX dx9X; //2021

	template<class U> 
	inline qtable_t(U *in)
	{ 
		memset(this,0x00,sizeof(*this)); 
		assert(!qbase._target||qbase._target==in->target);		
		qbase._target = in->target;		
		qbase+=in; 
	}
	inline ~qtable_t()
	{				
		switch(qbase._target)
		{
		case '_dx6': ~_dx6; break; 
		case 'dx7a': ~dx7a; break; 
			default: //2021
		case 'dx9c': ~dx9X; break; //~dx9c
		}
	}
};}

namespace DDRAW
{		
	//see constructors
	template<class T, class U> 
	inline DDRAW::qtable_t<T> *new_qtable(U *p, void *q=0)
	{			
		if(q) //qtable already exist in q, so add p to it
		{		
			assert(p->target==DDRAW::target);

			*(T::Query*)q+=p; return (DDRAW::qtable_t<T>*)q;
		}
		else return new DDRAW::qtable_t<T>(p);
	}

	//see deconstructors
	template<class T, class U> 
	inline void delete_qtable(U *p, void *q=0)
	{			
		if(p&&!(*(T::Query*)q-=p)) delete (DDRAW::qtable_t<T>*)q;
	}		 

	//messy internals
	struct IDirectDraw__Query;
	struct IDirectDrawSurface__Query;
	struct IDirect3D__Query;
	struct IDirect3DDevice__Query;

	class IDirectDraw;
	class IDirectDraw4;
	class IDirectDraw7;	
	struct IDirectDraw__Query
	{
		int _target;

		DDRAW::IDirectDraw  *dxf; 
		DDRAW::IDirectDraw4 *dx4;
		DDRAW::IDirectDraw7 *dx7;

		DDRAW::IDirect3D__Query *d3d; 

		inline void operator~()
		{
			assert(!dxf&&!dx4&&!dx7&&!d3d);
		}
	};

	class IDirectDrawSurface;
	class IDirectDrawSurface4;
	class IDirectDrawSurface7;	
	class IDirectDrawGammaControl;
	struct IDirectDrawSurface__Query
	{
		int _target;

		DDRAW::IDirectDrawSurface  *dxf; 
		DDRAW::IDirectDrawSurface4 *dx4;
		DDRAW::IDirectDrawSurface7 *dx7;

		DDRAW::IDirectDrawGammaControl *gamma;

		inline void operator~()
		{
			if(gamma) //temporary fix
			{
				//Sword of Moonlight is not
				//always releasing the control
				//TODO: utilize hack_interface()
				delete gamma; gamma = 0;
			}

			assert(!dxf&&!dx4&&!dx7&&!gamma);
		}
	};

	class IDirect3D3;
	class IDirect3D7;
	struct IDirect3D__Query
	{
		int _target;

		DDRAW::IDirect3D3 *dx3;
		DDRAW::IDirect3D7 *dx7;
		
		DDRAW::IDirect3DDevice__Query *d3ddevice; 

		DDRAW::IDirectDraw__Query *ddraw; 

		inline void operator~()
		{
			assert(!dx3&&!dx7&&!d3ddevice&&ddraw);

			if(ddraw) ddraw->d3d = 0;
		}
	};

	class IDirect3DDevice3;
	class IDirect3DDevice7;
	struct IDirect3DDevice__Query
	{	
		int _target;

		DDRAW::IDirect3DDevice3 *dx3;
		DDRAW::IDirect3DDevice7 *dx7; 
		
		DDRAW::IDirect3D__Query *d3d; 

		inline void operator~()
		{
			assert(!dx3&&!dx7&&d3d);

			if(d3d) d3d->d3ddevice = 0;
		}
	};

	class IDirect3DVertexBuffer;
	class IDirect3DVertexBuffer7;
	struct IDirect3DVertexBuffer__Query
	{
		int _target;

		DDRAW::IDirect3DVertexBuffer *dxf;
		DDRAW::IDirect3DVertexBuffer *dx7;

		inline void operator~()
		{
			assert(!dxf&&!dx7);
		}
	};
}

struct IDirectDraw;

namespace DDRAW{
class IDirectDraw : public DX::IDirectDraw
{
DDRAW_INTERFACE(IDirectDraw) //public

	static const int source = 'dxf';
			
	typedef struct:DDRAW::IDirectDraw__Query
	{
		template<class T> inline void operator+=(T *in);
		template<class T> inline bool operator-=(T *in);

	}Query, Query6, Query7;

	typedef struct : public Query //9
	{		
		UINT adapter;
	
		int vsmodel, vsprofile; //D3DVS_VERSION (hardware only)
		int psmodel, psprofile; //D3DPS_VERSION (hardware only)

		DX::IDirectDraw7 *vblank;

		inline void operator~();

	}Query9,QueryX;

	union
	{
	DDRAW::IDirectDraw::Query *query;
	DDRAW::IDirectDraw::Query6 *query6;
	DDRAW::IDirectDraw::Query7 *query7;
	DDRAW::IDirectDraw::Query9 *query9;
	DDRAW::IDirectDraw::QueryX *queryX;
	DDRAW::qtable_t<IDirectDraw> *qtable;
	};
	
	union
	{
	::IDirectDraw *proxy6;
	DX::IDirectDraw *proxy;
	::IDirectDraw *proxy7;
	::IDirect3D9Ex *proxy9;
	::IDirect3D9Ex *proxyX; //just for adaptor stuff
	};

	IDirectDraw(int t, void *q=0):target(t),proxy(0)
	{ 
		DDRAW_CONSTRUCT(IDirectDraw)   		

		qtable = DDRAW::new_qtable<IDirectDraw>(this,q);
	}
		
	~IDirectDraw()
	{
		if(!target) return;
		
		DDRAW::delete_qtable<IDirectDraw>(this,qtable);

		DDRAW_DESTRUCT(IDirectDraw)
	}

	 /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDraw methods ***/
    STDMETHOD(Compact)(THIS) PURE;
    STDMETHOD(CreateClipper)(THIS_ DWORD, DX::LPDIRECTDRAWCLIPPER FAR*, IUnknown FAR * ) PURE;
    STDMETHOD(CreatePalette)(THIS_ DWORD, LPPALETTEENTRY, DX::LPDIRECTDRAWPALETTE FAR*, IUnknown FAR * ) PURE;
    STDMETHOD(CreateSurface)(THIS_  DX::LPDDSURFACEDESC, DX::LPDIRECTDRAWSURFACE FAR *, IUnknown FAR *) PURE;
    STDMETHOD(DuplicateSurface)( THIS_ DX::LPDIRECTDRAWSURFACE, DX::LPDIRECTDRAWSURFACE FAR * ) PURE;
    STDMETHOD(EnumDisplayModes)( THIS_ DWORD, DX::LPDDSURFACEDESC, LPVOID, DX::LPDDENUMMODESCALLBACK ) PURE;
    STDMETHOD(EnumSurfaces)(THIS_ DWORD, DX::LPDDSURFACEDESC, LPVOID, DX::LPDDENUMSURFACESCALLBACK ) PURE;
    STDMETHOD(FlipToGDISurface)(THIS) PURE;
	STDMETHOD(GetCaps)( THIS_ DX::LPDDCAPS, DX::LPDDCAPS) PURE;
    STDMETHOD(GetDisplayMode)( THIS_ DX::LPDDSURFACEDESC) PURE;
    STDMETHOD(GetFourCCCodes)(THIS_  LPDWORD, LPDWORD ) PURE;
    STDMETHOD(GetGDISurface)(THIS_ DX::LPDIRECTDRAWSURFACE FAR *) PURE;
    STDMETHOD(GetMonitorFrequency)(THIS_ LPDWORD) PURE;
    STDMETHOD(GetScanLine)(THIS_ LPDWORD) PURE;
    STDMETHOD(GetVerticalBlankStatus)(THIS_ LPBOOL ) PURE;
    STDMETHOD(Initialize)(THIS_ GUID FAR *) PURE;
    STDMETHOD(RestoreDisplayMode)(THIS) PURE;
    STDMETHOD(SetCooperativeLevel)(THIS_ HWND, DWORD) PURE;
    STDMETHOD(SetDisplayMode)(THIS_ DWORD, DWORD,DWORD) PURE;
    STDMETHOD(WaitForVerticalBlank)(THIS_ DWORD, HANDLE ) PURE;
};}

struct IDirectDraw4;

namespace DDRAW{
class IDirectDraw4 : public DX::IDirectDraw4
{
DDRAW_INTERFACE(IDirectDraw4) //public
	
	static const int source = 'dx4';
	
	union
	{
	DDRAW::IDirectDraw::Query *query;
	DDRAW::IDirectDraw::Query6 *query6;
	DDRAW::IDirectDraw::Query7 *query7;
	DDRAW::IDirectDraw::Query9 *query9;
	DDRAW::IDirectDraw::QueryX *queryX;
	DDRAW::qtable_t<IDirectDraw> *qtable;
	};
	
	union 
	{
	::IDirectDraw4 *proxy6;
	DX::IDirectDraw4 *proxy;
	::IDirectDraw4 *proxy7;
	::IDirect3D9Ex *proxy9;
	};

	typedef DDRAW::IDirectDraw::Query Query;

	IDirectDraw4(int t, void *q=0):target(t),proxy(0)
	{ 		
		DDRAW_CONSTRUCT(IDirectDraw7)   
		
		qtable = DDRAW::new_qtable<IDirectDraw>(this,q);
	}

	~IDirectDraw4()
	{ 
		if(!target) return;
		
		DDRAW::delete_qtable<IDirectDraw>(this,qtable);

		DDRAW_DESTRUCT(IDirectDraw4)
	}

	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDraw methods ***/
    STDMETHOD(Compact)(THIS) PURE;
    STDMETHOD(CreateClipper)(THIS_ DWORD, DX::LPDIRECTDRAWCLIPPER FAR*, IUnknown FAR * ) PURE;
    STDMETHOD(CreatePalette)(THIS_ DWORD, LPPALETTEENTRY, DX::LPDIRECTDRAWPALETTE FAR*, IUnknown FAR * ) PURE;
    STDMETHOD(CreateSurface)(THIS_  DX::LPDDSURFACEDESC2, DX::LPDIRECTDRAWSURFACE4 FAR *, IUnknown FAR *) PURE;
    STDMETHOD(DuplicateSurface)( THIS_ DX::LPDIRECTDRAWSURFACE4, DX::LPDIRECTDRAWSURFACE4 FAR * ) PURE;
    STDMETHOD(EnumDisplayModes)( THIS_ DWORD, DX::LPDDSURFACEDESC2, LPVOID, DX::LPDDENUMMODESCALLBACK2 ) PURE;
    STDMETHOD(EnumSurfaces)(THIS_ DWORD, DX::LPDDSURFACEDESC2, LPVOID,DX::LPDDENUMSURFACESCALLBACK2 ) PURE;
    STDMETHOD(FlipToGDISurface)(THIS) PURE;
    STDMETHOD(GetCaps)( THIS_ DX::LPDDCAPS, DX::LPDDCAPS) PURE;
    STDMETHOD(GetDisplayMode)( THIS_ DX::LPDDSURFACEDESC2) PURE;
    STDMETHOD(GetFourCCCodes)(THIS_  LPDWORD, LPDWORD ) PURE;
    STDMETHOD(GetGDISurface)(THIS_ DX::LPDIRECTDRAWSURFACE4 FAR *) PURE;
    STDMETHOD(GetMonitorFrequency)(THIS_ LPDWORD) PURE;
    STDMETHOD(GetScanLine)(THIS_ LPDWORD) PURE;
    STDMETHOD(GetVerticalBlankStatus)(THIS_ LPBOOL ) PURE;
    STDMETHOD(Initialize)(THIS_ GUID FAR *) PURE;
    STDMETHOD(RestoreDisplayMode)(THIS) PURE;
    STDMETHOD(SetCooperativeLevel)(THIS_ HWND, DWORD) PURE;
    STDMETHOD(SetDisplayMode)(THIS_ DWORD, DWORD,DWORD, DWORD, DWORD) PURE;
    STDMETHOD(WaitForVerticalBlank)(THIS_ DWORD, HANDLE ) PURE;
	/*** Added in the v2 interface ***/
    STDMETHOD(GetAvailableVidMem)(THIS_ DX::LPDDSCAPS2, LPDWORD, LPDWORD) PURE;
    /*** Added in the V4 Interface ***/
    STDMETHOD(GetSurfaceFromDC) (THIS_ HDC, DX::LPDIRECTDRAWSURFACE4 *) PURE;
    STDMETHOD(RestoreAllSurfaces)(THIS) PURE;
    STDMETHOD(TestCooperativeLevel)(THIS) PURE;
    STDMETHOD(GetDeviceIdentifier)(THIS_ DX::LPDDDEVICEIDENTIFIER, DWORD ) PURE;    
};}

struct IDirectDraw7;

namespace DDRAW{
class IDirectDraw7 : public DX::IDirectDraw7
{
DDRAW_INTERFACE(IDirectDraw7) //public
	
	static const int source = 'dx7';

	union
	{
	DDRAW::IDirectDraw::Query *query;
	DDRAW::IDirectDraw::Query7 *query7;
	DDRAW::IDirectDraw::Query9 *query9;
	DDRAW::IDirectDraw::QueryX *queryX;
	DDRAW::qtable_t<IDirectDraw> *qtable;
	};
	
	union 
	{
	DX::IDirectDraw7 *proxy;
	::IDirectDraw7 *proxy7;
	::IDirect3D9Ex *proxy9;
	::IDirect3D9Ex *proxyX; //just for adaptor stuff
	};

	typedef DDRAW::IDirectDraw::Query Query;

	IDirectDraw7(int t, void *q=0):target(t),proxy(0)
	{ 		
		DDRAW_CONSTRUCT(IDirectDraw7)   
				
		qtable = DDRAW::new_qtable<IDirectDraw>(this,q);
	}

	~IDirectDraw7()
	{ 
		if(!target) return;
		
		DDRAW::delete_qtable<IDirectDraw>(this,qtable);

		DDRAW_DESTRUCT(IDirectDraw7)
	}

	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDraw methods ***/
    STDMETHOD(Compact)(THIS) PURE;
    STDMETHOD(CreateClipper)(THIS_ DWORD, DX::LPDIRECTDRAWCLIPPER FAR*, IUnknown FAR * ) PURE;
    STDMETHOD(CreatePalette)(THIS_ DWORD, LPPALETTEENTRY, DX::LPDIRECTDRAWPALETTE FAR*, IUnknown FAR * ) PURE;
    STDMETHOD(CreateSurface)(THIS_  DX::LPDDSURFACEDESC2, DX::LPDIRECTDRAWSURFACE7 FAR *, IUnknown FAR *) PURE;
    STDMETHOD(DuplicateSurface)( THIS_ DX::LPDIRECTDRAWSURFACE7, DX::LPDIRECTDRAWSURFACE7 FAR * ) PURE;
    STDMETHOD(EnumDisplayModes)( THIS_ DWORD, DX::LPDDSURFACEDESC2, LPVOID, DX::LPDDENUMMODESCALLBACK2 ) PURE;
    STDMETHOD(EnumSurfaces)(THIS_ DWORD, DX::LPDDSURFACEDESC2, LPVOID,DX::LPDDENUMSURFACESCALLBACK7 ) PURE;
    STDMETHOD(FlipToGDISurface)(THIS) PURE;
    STDMETHOD(GetCaps)( THIS_ DX::LPDDCAPS_DX7, DX::LPDDCAPS_DX7) PURE;
    STDMETHOD(GetDisplayMode)( THIS_ DX::LPDDSURFACEDESC2) PURE;
    STDMETHOD(GetFourCCCodes)(THIS_  LPDWORD, LPDWORD ) PURE;
    STDMETHOD(GetGDISurface)(THIS_ DX::LPDIRECTDRAWSURFACE7 FAR *) PURE;
    STDMETHOD(GetMonitorFrequency)(THIS_ LPDWORD) PURE;
    STDMETHOD(GetScanLine)(THIS_ LPDWORD) PURE;
    STDMETHOD(GetVerticalBlankStatus)(THIS_ LPBOOL ) PURE;
    STDMETHOD(Initialize)(THIS_ GUID FAR *) PURE;
    STDMETHOD(RestoreDisplayMode)(THIS) PURE;
    STDMETHOD(SetCooperativeLevel)(THIS_ HWND, DWORD) PURE;
    STDMETHOD(SetDisplayMode)(THIS_ DWORD, DWORD,DWORD, DWORD, DWORD) PURE;
    STDMETHOD(WaitForVerticalBlank)(THIS_ DWORD, HANDLE ) PURE;
    /*** Added in the v2 interface ***/
    STDMETHOD(GetAvailableVidMem)(THIS_ DX::LPDDSCAPS2, LPDWORD, LPDWORD) PURE;
    /*** Added in the V4 Interface ***/
    STDMETHOD(GetSurfaceFromDC) (THIS_ HDC, DX::LPDIRECTDRAWSURFACE7 *) PURE;
    STDMETHOD(RestoreAllSurfaces)(THIS) PURE;
    STDMETHOD(TestCooperativeLevel)(THIS) PURE;
    STDMETHOD(GetDeviceIdentifier)(THIS_ DX::LPDDDEVICEIDENTIFIER2, DWORD ) PURE;
    STDMETHOD(StartModeTest)(THIS_ LPSIZE, DWORD, DWORD ) PURE;
    STDMETHOD(EvaluateMode)(THIS_ DWORD, DWORD * ) PURE;
};}

template<class T> 
inline void DDRAW::IDirectDraw::Query::operator+=(T *in)
{ 
	if(in->source==dxf->source) *(void**)&dxf = in;
	if(in->source==dx4->source) *(void**)&dx4 = in;
	if(in->source==dx7->source) *(void**)&dx7 = in;
}
template<class T> 
inline bool DDRAW::IDirectDraw::Query::operator-=(T *in)
{ 
	if(in->source==dxf->source) dxf = 0;
	if(in->source==dx4->source) dx4 = 0;
	if(in->source==dx7->source) dx7 = 0; 

	return dxf||dx4||dx7;
}

inline void DDRAW::IDirectDraw::Query9::operator~() 
{
	DDRAW::IDirectDraw::Query::operator~(); 

	if(vblank) vblank->Release();
}

struct IDirectDrawClipper;

namespace DDRAW{
class IDirectDrawClipper : public DX::IDirectDrawClipper
{
DDRAW_INTERFACE(IDirectDrawClipper) //public

	static const int source = 'dxf';

	union
	{
	::IDirectDrawClipper *proxy6;
	DX::IDirectDrawClipper *proxy;
	::IDirectDrawClipper *proxy7;
	::IDirect3D9Ex *proxy9;
	::IDirect3D9Ex *proxyX; //just for adaptor stuff
	};

	IDirectDrawClipper(int t):proxy(0),target(t)
	{ 
		DDRAW_CONSTRUCT(IDirectDrawClipper)   
	}

	~IDirectDrawClipper()
	{
		if(!target) return;

		DDRAW_DESTRUCT(IDirectDrawClipper)
	}

    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDrawClipper methods ***/
    STDMETHOD(GetClipList)(THIS_ LPRECT, LPRGNDATA, LPDWORD) PURE;
    STDMETHOD(GetHWnd)(THIS_ HWND FAR *) PURE;
    STDMETHOD(Initialize)(THIS_ DX::LPDIRECTDRAW, DWORD) PURE;
    STDMETHOD(IsClipListChanged)(THIS_ BOOL FAR *) PURE;
    STDMETHOD(SetClipList)(THIS_ LPRGNDATA,DWORD) PURE;
    STDMETHOD(SetHWnd)(THIS_ DWORD, HWND ) PURE;
};}

struct IDirectDrawPalette;

namespace DDRAW{
class IDirectDrawPalette : public DX::IDirectDrawPalette
{
DDRAW_INTERFACE(IDirectDrawPalette) //public

	static const int source = 'dxf';

	union
	{
	::IDirectDrawPalette *proxy6;
	DX::IDirectDrawPalette *proxy;
	::IDirectDrawPalette *proxy7;
	};

	UINT palette9; //palette number

	DWORD caps9; //DDPCAPS

	IDirectDrawPalette(int t):proxy(0),target(t)
	{ 
		DDRAW_CONSTRUCT(IDirectDrawPalette)   
	}

	~IDirectDrawPalette()
	{
		if(!target) return;

		DDRAW_DESTRUCT(IDirectDrawPalette)
	}

	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDrawPalette methods ***/
    STDMETHOD(GetCaps)(THIS_ LPDWORD) PURE;
    STDMETHOD(GetEntries)(THIS_ DWORD,DWORD,DWORD,LPPALETTEENTRY) PURE;
    STDMETHOD(Initialize)(THIS_ DX::LPDIRECTDRAW, DWORD, LPPALETTEENTRY) PURE;
    STDMETHOD(SetEntries)(THIS_ DWORD,DWORD,DWORD,LPPALETTEENTRY) PURE;
};}

struct IDirectDrawGammaControl;
struct IDirect3DDevice9Ex;

namespace DDRAW{
class IDirectDrawGammaControl : public DX::IDirectDrawGammaControl
{
DDRAW_INTERFACE(IDirectDrawGammaControl) //public
	
	static const int source = 'dxf';

	union 
	{
	::IDirectDrawGammaControl *proxy6;
	DX::IDirectDrawGammaControl *proxy;
	::IDirectDrawGammaControl *proxy7;
	::IDirect3DDevice9Ex *proxy9;
	};

	DX::DDGAMMARAMP *gammaramp; //DDRAW::gammaramp

	DDRAW::IDirectDrawSurface__Query *surface;

	IDirectDrawGammaControl(int t):proxy(0),target(t)
	{ 
		DDRAW_CONSTRUCT(IDirectDrawGammaControl)   

		gammaramp = &DDRAW::gammaramp; //assuming

		surface = 0; 
	}

	~IDirectDrawGammaControl()
	{
		if(!target) return;
		
		if(surface) surface->gamma = 0;

		DDRAW_DESTRUCT(IDirectDrawGammaControl)
	}

	/*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDrawGammaControl methods ***/
    STDMETHOD(GetGammaRamp)(THIS_ DWORD, DX::LPDDGAMMARAMP) PURE;
	STDMETHOD(SetGammaRamp)(THIS_ DWORD, DX::LPDDGAMMARAMP) PURE;
};}

struct IDirect3DTexture2;
struct IDirect3DSurface9;
struct IDirect3DTexture9;

struct IDirectDrawSurface;

namespace DDRAW{
class IDirectDrawSurface : public DX::IDirectDrawSurface
{
DDRAW_INTERFACE(IDirectDrawSurface) //public

	static const int source = 'dxf';

	typedef struct:IDirectDrawSurface__Query
	{	
		template<class T> inline void operator+=(T *in);
		template<class T> inline bool operator-=(T *in);

		bool isPrimary;		
			
		//for more sophisticated client data
		//management, see Get/Set/FreePrivateData
		//2021: as a reminder this was being used with
		//Ex.dataset.h (EX::DATA::Get/Set)
		void *clientdata; //see: clientdtor()

		static void (*clientdtor)(void*&); 

		inline void operator~()
		{
			DDRAW::IDirectDrawSurface__Query::operator~();

			if(clientdata&&clientdtor) 
			{
				clientdtor(clientdata); clientdata = 0;
			}
		}

	}Query;
	
	typedef struct : public Query //6
	{	
		::IDirect3DTexture2 *texture; //in video memory

		bool loaded; //is texture loaded?

		inline void operator~(); 

	}Query6;

	typedef Query Query7;

	typedef struct : public Query
	{			
		//NOTE: MSVC2010 segfaulted on ?: operator with bitfield
		//so keeping with bool to avoid trouble in existing code
		bool isVirtual; //2021
		bool isLocked; //not 100% supported
		bool wasLocked; //ie. has image data
		bool isLockedRO; //2021
		bool isTextureLike; //2021: for updating_all_textures

		int isMipmap;

		//2021: these override DDRAW::colorkey and DDRAW::mipmap
		//this is to remember which ones to use since now colorkey
		//and mipmap generation is delayed until updating_texture is
		//called, which SetTexture will do
		int(*colorkey_f)(DX::DDSURFACEDESC2*,D3DFORMAT nativeformat);
		void*(*mipmap_f)(const DX::DDSURFACEDESC2*,DX::DDSURFACEDESC2*);

		//this just tells if the source was 16bpp
		//or had had alpha data
		D3DFORMAT nativeformat;

		D3DFORMAT format; //2021

		//::IDirect3DTexture9 *group,*update;

		INT pitch; UINT palette; 

		//valid for each colorkey2 set (four total)
		//DDSD_CKSRCBLT/CKDESTBLT/CKSRCOVERLAY/CKDESTOVERLAY
		DWORD colorkey2; DX::DDCOLORKEY *colorkey;

		int update_pending, knockouts; //colorkey

		//OpenGL can't query mipmaps, so this has to be
		//stored or recalculated. may as well store the
		//width and height as well
		//ceil(log2(max(width,height)))+1
		//
		// UPDATE: I decided to use Direct3DDevice9 to
		// manage the D3DPOOL_SYSTEMEM surface and just
		// push it to OpenGL in D3D9X::updating_texture
		// so now these are just used to write briefer
		// code than GetDesc, since I don't want to rip
		// them out and put back GetDesc calls... maybe
		// later I will try to mess with PBOs but that
		// doesn't seem very practical (plus PBOs look 
		// like an unreliable mess API/performance wise)
		//
		unsigned int mipmaps,width,height;

		union
		{
			IUnknown *group; //QueryX

			IDirectDrawSurface *groupGL;
			::IDirect3DTexture9 *group9; //Query9
		};
		union
		{
			void *update; //QueryX
			unsigned updateGL; //GLuint
			::IDirect3DTexture9 *update9; //Query9			
		};
		void _updateGL_delete();

		inline void operator~(); 

	}Query9,QueryX;

	union
	{
	DDRAW::IDirectDrawSurface::Query *query;
	DDRAW::IDirectDrawSurface::Query6 *query6;
	DDRAW::IDirectDrawSurface::Query7 *query7;
	DDRAW::IDirectDrawSurface::Query9 *query9;
	DDRAW::IDirectDrawSurface::QueryX *queryX;
	DDRAW::qtable_t<IDirectDrawSurface> *qtable;
	};
	
	union
	{
	::IDirectDrawSurface *proxy6;
	DX::IDirectDrawSurface *proxy;
	::IDirectDrawSurface *proxy7;
	::IDirect3DSurface9 *proxy9;
	};

	//TODO: consider handling inside Query
	IDirectDrawSurface *get_next, *get_prev;

	static IDirectDrawSurface *get_head; 

	typedef DDRAW::IDirectDrawSurface::Query Query;
	
	IDirectDrawSurface(int t, void *q=0):target(t),proxy(0)
	{ 
		DDRAW_CONSTRUCT(IDirectDrawSurface)   		

		qtable = DDRAW::new_qtable<IDirectDrawSurface>(this,q);

		get_next = get_head?get_head:this; 	
		get_prev = get_head?get_head->get_prev:this; 

		get_next->get_prev = this;
		get_prev->get_next = this;

		get_head = this;
	}

	~IDirectDrawSurface()
	{
		if(!target) return;
		
		DDRAW::delete_qtable<IDirectDrawSurface>(this,qtable);

		get_prev->get_next = get_next; get_next->get_prev = get_prev;

		if(get_head==this) get_head = get_next==this?0:get_next;		

		get_prev = get_next = 0;

		DDRAW_DESTRUCT(IDirectDrawSurface)
	}

	DDRAW::IDirectDrawSurface *get(void*); //proxy pointer

    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDrawSurface methods ***/
    STDMETHOD(AddAttachedSurface)(THIS_ DX::LPDIRECTDRAWSURFACE) PURE;
    STDMETHOD(AddOverlayDirtyRect)(THIS_ LPRECT) PURE;
    STDMETHOD(Blt)(THIS_ LPRECT,DX::LPDIRECTDRAWSURFACE, LPRECT,DWORD, DX::LPDDBLTFX) PURE;
    STDMETHOD(BltBatch)(THIS_ DX::LPDDBLTBATCH, DWORD, DWORD ) PURE;
    STDMETHOD(BltFast)(THIS_ DWORD,DWORD,DX::LPDIRECTDRAWSURFACE, LPRECT,DWORD) PURE;
    STDMETHOD(DeleteAttachedSurface)(THIS_ DWORD,DX::LPDIRECTDRAWSURFACE) PURE;
    STDMETHOD(EnumAttachedSurfaces)(THIS_ LPVOID,DX::LPDDENUMSURFACESCALLBACK) PURE;
    STDMETHOD(EnumOverlayZOrders)(THIS_ DWORD,LPVOID,DX::LPDDENUMSURFACESCALLBACK) PURE;
    STDMETHOD(Flip)(THIS_ DX::LPDIRECTDRAWSURFACE, DWORD) PURE;
    STDMETHOD(GetAttachedSurface)(THIS_ DX::LPDDSCAPS, DX::LPDIRECTDRAWSURFACE FAR *) PURE;
    STDMETHOD(GetBltStatus)(THIS_ DWORD) PURE;
    STDMETHOD(GetCaps)(THIS_ DX::LPDDSCAPS) PURE;
    STDMETHOD(GetClipper)(THIS_ DX::LPDIRECTDRAWCLIPPER FAR*) PURE;
    STDMETHOD(GetColorKey)(THIS_ DWORD, DX::LPDDCOLORKEY) PURE;
    STDMETHOD(GetDC)(THIS_ HDC FAR *) PURE;
    STDMETHOD(GetFlipStatus)(THIS_ DWORD) PURE;
    STDMETHOD(GetOverlayPosition)(THIS_ LPLONG, LPLONG ) PURE;
    STDMETHOD(GetPalette)(THIS_ DX::LPDIRECTDRAWPALETTE FAR*) PURE;
    STDMETHOD(GetPixelFormat)(THIS_ DX::LPDDPIXELFORMAT) PURE;
    STDMETHOD(GetSurfaceDesc)(THIS_ DX::LPDDSURFACEDESC) PURE;
    STDMETHOD(Initialize)(THIS_ DX::LPDIRECTDRAW, DX::LPDDSURFACEDESC) PURE;
    STDMETHOD(IsLost)(THIS) PURE;
    STDMETHOD(Lock)(THIS_ LPRECT,DX::LPDDSURFACEDESC,DWORD,HANDLE) PURE;
    STDMETHOD(ReleaseDC)(THIS_ HDC) PURE;
    STDMETHOD(Restore)(THIS) PURE;
    STDMETHOD(SetClipper)(THIS_ DX::LPDIRECTDRAWCLIPPER) PURE;
    STDMETHOD(SetColorKey)(THIS_ DWORD, DX::LPDDCOLORKEY) PURE;
    STDMETHOD(SetOverlayPosition)(THIS_ LONG, LONG ) PURE;
    STDMETHOD(SetPalette)(THIS_ DX::LPDIRECTDRAWPALETTE) PURE;
    STDMETHOD(Unlock)(THIS_ LPVOID) PURE;
    STDMETHOD(UpdateOverlay)(THIS_ LPRECT, DX::LPDIRECTDRAWSURFACE,LPRECT,DWORD, DX::LPDDOVERLAYFX) PURE;
    STDMETHOD(UpdateOverlayDisplay)(THIS_ DWORD) PURE;
    STDMETHOD(UpdateOverlayZOrder)(THIS_ DWORD, DX::LPDIRECTDRAWSURFACE) PURE;
};}

struct IDirectDrawSurface4;

namespace DDRAW{
class IDirectDrawSurface4 : public DX::IDirectDrawSurface4
{
DDRAW_INTERFACE(IDirectDrawSurface4) //public

	static const int source = 'dx4';

	union
	{
	DDRAW::IDirectDrawSurface::Query *query;
	DDRAW::IDirectDrawSurface::Query6 *query6;
	DDRAW::IDirectDrawSurface::Query7 *query7;
	DDRAW::IDirectDrawSurface::Query9 *query9;
	DDRAW::IDirectDrawSurface::QueryX *queryX;
	DDRAW::qtable_t<IDirectDrawSurface> *qtable;
	};

	union
	{
	::IDirectDrawSurface4 *proxy6;
	DX::IDirectDrawSurface4 *proxy;
	::IDirectDrawSurface4 *proxy7;
	::IDirect3DSurface9 *proxy9;
	};

	//TODO: consider handling inside Query
	IDirectDrawSurface4 *get_next, *get_prev;

	static IDirectDrawSurface4 *get_head; 

	typedef DDRAW::IDirectDrawSurface::Query Query;

	IDirectDrawSurface4(int t, void *q=0):target(t),proxy(0)
	{ 
		DDRAW_CONSTRUCT(IDirectDrawSurface4)   		

		qtable = DDRAW::new_qtable<IDirectDrawSurface>(this,q);

		get_next = get_head?get_head:this; 	
		get_prev = get_head?get_head->get_prev:this; 

		get_next->get_prev = this;
		get_prev->get_next = this;

		get_head = this;
	}

	~IDirectDrawSurface4()
	{
		if(!target) return;

		DDRAW::delete_qtable<IDirectDrawSurface>(this,qtable);

		assert(get_next&&get_prev);

		get_prev->get_next = get_next; get_next->get_prev = get_prev;

		if(get_head==this) get_head = get_next==this?0:get_next; 

		get_prev = get_next = 0; 

		DDRAW_DESTRUCT(IDirectDrawSurface4)
	}

	DDRAW::IDirectDrawSurface4 *get(void*); //proxy pointer

	/*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDrawSurface methods ***/
    STDMETHOD(AddAttachedSurface)(THIS_ DX::LPDIRECTDRAWSURFACE4) PURE;
    STDMETHOD(AddOverlayDirtyRect)(THIS_ LPRECT) PURE;
	STDMETHOD(Blt)(THIS_ LPRECT,DX::LPDIRECTDRAWSURFACE4, LPRECT,DWORD, DX::LPDDBLTFX) PURE;
    STDMETHOD(BltBatch)(THIS_ DX::LPDDBLTBATCH, DWORD, DWORD ) PURE;
    STDMETHOD(BltFast)(THIS_ DWORD,DWORD,DX::LPDIRECTDRAWSURFACE4, LPRECT,DWORD) PURE;
    STDMETHOD(DeleteAttachedSurface)(THIS_ DWORD,DX::LPDIRECTDRAWSURFACE4) PURE;
    STDMETHOD(EnumAttachedSurfaces)(THIS_ LPVOID,DX::LPDDENUMSURFACESCALLBACK2) PURE;
    STDMETHOD(EnumOverlayZOrders)(THIS_ DWORD,LPVOID,DX::LPDDENUMSURFACESCALLBACK2) PURE;
    STDMETHOD(Flip)(THIS_ DX::LPDIRECTDRAWSURFACE4, DWORD) PURE;
    STDMETHOD(GetAttachedSurface)(THIS_ DX::LPDDSCAPS2, DX::LPDIRECTDRAWSURFACE4 FAR *) PURE;
    STDMETHOD(GetBltStatus)(THIS_ DWORD) PURE;
    STDMETHOD(GetCaps)(THIS_ DX::LPDDSCAPS2) PURE;
    STDMETHOD(GetClipper)(THIS_ DX::LPDIRECTDRAWCLIPPER FAR*) PURE;
    STDMETHOD(GetColorKey)(THIS_ DWORD, DX::LPDDCOLORKEY) PURE;
    STDMETHOD(GetDC)(THIS_ HDC FAR *) PURE;
    STDMETHOD(GetFlipStatus)(THIS_ DWORD) PURE;
    STDMETHOD(GetOverlayPosition)(THIS_ LPLONG, LPLONG ) PURE;
    STDMETHOD(GetPalette)(THIS_ DX::LPDIRECTDRAWPALETTE FAR*) PURE;
    STDMETHOD(GetPixelFormat)(THIS_ DX::LPDDPIXELFORMAT) PURE;
    STDMETHOD(GetSurfaceDesc)(THIS_ DX::LPDDSURFACEDESC2) PURE;
    STDMETHOD(Initialize)(THIS_ DX::LPDIRECTDRAW, DX::LPDDSURFACEDESC2) PURE;
    STDMETHOD(IsLost)(THIS) PURE;
    STDMETHOD(Lock)(THIS_ LPRECT,DX::LPDDSURFACEDESC2,DWORD,HANDLE) PURE;
    STDMETHOD(ReleaseDC)(THIS_ HDC) PURE;
    STDMETHOD(Restore)(THIS) PURE;
    STDMETHOD(SetClipper)(THIS_ DX::LPDIRECTDRAWCLIPPER) PURE;
    STDMETHOD(SetColorKey)(THIS_ DWORD, DX::LPDDCOLORKEY) PURE;
    STDMETHOD(SetOverlayPosition)(THIS_ LONG, LONG ) PURE;
    STDMETHOD(SetPalette)(THIS_ DX::LPDIRECTDRAWPALETTE) PURE;
    STDMETHOD(Unlock)(THIS_ LPRECT) PURE;
    STDMETHOD(UpdateOverlay)(THIS_ LPRECT, DX::LPDIRECTDRAWSURFACE4,LPRECT,DWORD, DX::LPDDOVERLAYFX) PURE;
    STDMETHOD(UpdateOverlayDisplay)(THIS_ DWORD) PURE;
    STDMETHOD(UpdateOverlayZOrder)(THIS_ DWORD, DX::LPDIRECTDRAWSURFACE4) PURE;
    /*** Added in the v2 interface ***/
    STDMETHOD(GetDDInterface)(THIS_ LPVOID FAR *) PURE;
    STDMETHOD(PageLock)(THIS_ DWORD) PURE;
    STDMETHOD(PageUnlock)(THIS_ DWORD) PURE;
    /*** Added in the v3 interface ***/
    STDMETHOD(SetSurfaceDesc)(THIS_ DX::LPDDSURFACEDESC2, DWORD) PURE;
    /*** Added in the v4 interface ***/
    STDMETHOD(SetPrivateData)(THIS_ REFGUID, LPVOID, DWORD, DWORD) PURE;
    STDMETHOD(GetPrivateData)(THIS_ REFGUID, LPVOID, LPDWORD) PURE;
    STDMETHOD(FreePrivateData)(THIS_ REFGUID) PURE;
    STDMETHOD(GetUniquenessValue)(THIS_ LPDWORD) PURE;
    STDMETHOD(ChangeUniquenessValue)(THIS) PURE;
};}

struct IDirectDrawSurface7;

namespace DDRAW{
class IDirectDrawSurface7 : public DX::IDirectDrawSurface7
{
DDRAW_INTERFACE(IDirectDrawSurface7) //public

	static const int source = 'dx7';

	union
	{
	DDRAW::IDirectDrawSurface::Query *query;
	DDRAW::IDirectDrawSurface::Query7 *query7;
	DDRAW::IDirectDrawSurface::Query9 *query9;
	DDRAW::IDirectDrawSurface::QueryX *queryX;
	DDRAW::qtable_t<IDirectDrawSurface> *qtable;
	};

	union
	{
	DX::IDirectDrawSurface7 *proxy;
	::IDirectDrawSurface7 *proxy7;
	::IDirect3DSurface9 *proxy9;
	};
	
	//2021: this is a quick-and-dirty way to
	//update mipmaps after turning DDRAW::sRGB
	//on/off
	static void updating_all_textures(int force=1)
	{
		auto pp = get_head, p = pp; 
		if(p) do p->updating_texture(force);
		while(pp!=(p=p->get_next));
	}
	//2022: 4 is an experimental mode that's
	//used to signal a multithread worker to
	//skip UpdateTexture since it freezes up
	//2021: replaces D3D9C::updating_texture
	//NOTE: if force is set to 2 or 1|2 then
	//the colorkey will be reevaluated, else
	//DDRAW::doMipmaps will generate mipmaps
	//if force&1 and the texture will copied
	//to GPU memory. if force==0 any pending
	//operations are performed and copy will
	//be performed if anything was done
	bool updating_texture(int force=1);

	//TODO: consider handling inside Query
	IDirectDrawSurface7 *get_next, *get_prev;

	static IDirectDrawSurface7 *get_head; 

	typedef DDRAW::IDirectDrawSurface::Query Query;

	IDirectDrawSurface7(int t, void *q=0):target(t),proxy(0)
	{ 
		DDRAW_CONSTRUCT(IDirectDrawSurface7)   		

		qtable = DDRAW::new_qtable<IDirectDrawSurface>(this,q);

		get_next = get_head?get_head:this; 	
		get_prev = get_head?get_head->get_prev:this; 

		get_next->get_prev = this;
		get_prev->get_next = this;

		get_head = this;
	}

	~IDirectDrawSurface7()
	{
		if(!target) return;

		DDRAW::delete_qtable<IDirectDrawSurface>(this,qtable);

		assert(get_next&&get_prev);

		get_prev->get_next = get_next; get_next->get_prev = get_prev;

		if(get_head==this) get_head = get_next==this?0:get_next; 

		get_prev = get_next = 0; 

		DDRAW_DESTRUCT(IDirectDrawSurface7)
	}

	DDRAW::IDirectDrawSurface7 *get(void*); //proxy pointer

	/*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDrawSurface methods ***/
    STDMETHOD(AddAttachedSurface)(THIS_ DX::LPDIRECTDRAWSURFACE7) PURE;
    STDMETHOD(AddOverlayDirtyRect)(THIS_ LPRECT) PURE;
	STDMETHOD(Blt)(THIS_ LPRECT,DX::LPDIRECTDRAWSURFACE7, LPRECT,DWORD, DX::LPDDBLTFX) PURE;
    STDMETHOD(BltBatch)(THIS_ DX::LPDDBLTBATCH, DWORD, DWORD ) PURE;
    STDMETHOD(BltFast)(THIS_ DWORD,DWORD,DX::LPDIRECTDRAWSURFACE7, LPRECT,DWORD) PURE;
    STDMETHOD(DeleteAttachedSurface)(THIS_ DWORD,DX::LPDIRECTDRAWSURFACE7) PURE;
    STDMETHOD(EnumAttachedSurfaces)(THIS_ LPVOID,DX::LPDDENUMSURFACESCALLBACK7) PURE;
    STDMETHOD(EnumOverlayZOrders)(THIS_ DWORD,LPVOID,DX::LPDDENUMSURFACESCALLBACK7) PURE;
    STDMETHOD(Flip)(THIS_ DX::LPDIRECTDRAWSURFACE7, DWORD) PURE;
    STDMETHOD(GetAttachedSurface)(THIS_ DX::LPDDSCAPS2, DX::LPDIRECTDRAWSURFACE7 FAR *) PURE;
    STDMETHOD(GetBltStatus)(THIS_ DWORD) PURE;
    STDMETHOD(GetCaps)(THIS_ DX::LPDDSCAPS2) PURE;
    STDMETHOD(GetClipper)(THIS_ DX::LPDIRECTDRAWCLIPPER FAR*) PURE;
    STDMETHOD(GetColorKey)(THIS_ DWORD, DX::LPDDCOLORKEY) PURE;
    STDMETHOD(GetDC)(THIS_ HDC FAR *) PURE;
    STDMETHOD(GetFlipStatus)(THIS_ DWORD) PURE;
    STDMETHOD(GetOverlayPosition)(THIS_ LPLONG, LPLONG ) PURE;
    STDMETHOD(GetPalette)(THIS_ DX::LPDIRECTDRAWPALETTE FAR*) PURE;
    STDMETHOD(GetPixelFormat)(THIS_ DX::LPDDPIXELFORMAT) PURE;
    STDMETHOD(GetSurfaceDesc)(THIS_ DX::LPDDSURFACEDESC2) PURE;
    STDMETHOD(Initialize)(THIS_ DX::LPDIRECTDRAW, DX::LPDDSURFACEDESC2) PURE;
    STDMETHOD(IsLost)(THIS) PURE;
    STDMETHOD(Lock)(THIS_ LPRECT,DX::LPDDSURFACEDESC2,DWORD,HANDLE) PURE;
    STDMETHOD(ReleaseDC)(THIS_ HDC) PURE;
    STDMETHOD(Restore)(THIS) PURE;
    STDMETHOD(SetClipper)(THIS_ DX::LPDIRECTDRAWCLIPPER) PURE;
    STDMETHOD(SetColorKey)(THIS_ DWORD, DX::LPDDCOLORKEY) PURE;
    STDMETHOD(SetOverlayPosition)(THIS_ LONG, LONG ) PURE;
    STDMETHOD(SetPalette)(THIS_ DX::LPDIRECTDRAWPALETTE) PURE;
    STDMETHOD(Unlock)(THIS_ LPRECT) PURE;
    STDMETHOD(UpdateOverlay)(THIS_ LPRECT, DX::LPDIRECTDRAWSURFACE7,LPRECT,DWORD, DX::LPDDOVERLAYFX) PURE;
    STDMETHOD(UpdateOverlayDisplay)(THIS_ DWORD) PURE;
    STDMETHOD(UpdateOverlayZOrder)(THIS_ DWORD, DX::LPDIRECTDRAWSURFACE7) PURE;
    /*** Added in the v2 interface ***/
    STDMETHOD(GetDDInterface)(THIS_ LPVOID FAR *) PURE;
    STDMETHOD(PageLock)(THIS_ DWORD) PURE;
    STDMETHOD(PageUnlock)(THIS_ DWORD) PURE;
    /*** Added in the v3 interface ***/
    STDMETHOD(SetSurfaceDesc)(THIS_ DX::LPDDSURFACEDESC2, DWORD) PURE;
    /*** Added in the v4 interface ***/
    STDMETHOD(SetPrivateData)(THIS_ REFGUID, LPVOID, DWORD, DWORD) PURE;
    STDMETHOD(GetPrivateData)(THIS_ REFGUID, LPVOID, LPDWORD) PURE;
    STDMETHOD(FreePrivateData)(THIS_ REFGUID) PURE;
    STDMETHOD(GetUniquenessValue)(THIS_ LPDWORD) PURE;
    STDMETHOD(ChangeUniquenessValue)(THIS) PURE;
    /*** Moved Texture7 methods here ***/
    STDMETHOD(SetPriority)(THIS_ DWORD) PURE;
    STDMETHOD(GetPriority)(THIS_ LPDWORD) PURE;
    STDMETHOD(SetLOD)(THIS_ DWORD) PURE;
    STDMETHOD(GetLOD)(THIS_ LPDWORD) PURE;
};}

template<class T> 
inline void DDRAW::IDirectDrawSurface::Query::operator+=(T *in)
{ 
	if(in->source==dxf->source) *(void**)&dxf = in;
	if(in->source==dx4->source) *(void**)&dx4 = in;
	if(in->source==dx7->source) *(void**)&dx7 = in;
}
template<class T> 
inline bool DDRAW::IDirectDrawSurface::Query::operator-=(T *in)
{ 
	if(in->source==dxf->source) dxf = 0;
	if(in->source==dx4->source) dx4 = 0;
	if(in->source==dx7->source) dx7 = 0; return dxf||dx4||dx7;
}
inline void DDRAW::IDirectDrawSurface::Query6::operator~() 
{
	DDRAW::IDirectDrawSurface::Query::operator~(); 

	if(texture) ((IUnknown*)texture)->Release();
}
inline void DDRAW::IDirectDrawSurface::QueryX::operator~() 
{
	DDRAW::IDirectDrawSurface::Query::operator~(); 

	if(!isMipmap) delete colorkey;

	assert(!group&&!update); //2021
}

struct IDirect3DTexture2;
struct IDirect3DTexture9;

namespace DDRAW{
class IDirect3DTexture2 : public DX::IDirect3DTexture2
{
DDRAW_INTERFACE(IDirect3DTexture2) //public

	static const int source = 'dx2';
	
	union
	{
	::IDirect3DTexture2 *proxy6;
	DX::IDirectDrawSurface4 *proxy;
	::IDirectDrawSurface4 *proxy7;
	::IDirect3DTexture9 *proxy9;
	};

	union
	{
	DDRAW::IDirectDrawSurface::Query *surface;
	DDRAW::IDirectDrawSurface::Query6 *surface6;
	DDRAW::IDirectDrawSurface::Query7 *surface7;
	DDRAW::IDirectDrawSurface::QueryX *surface9;
	};

	IDirect3DTexture2 *get_next, *get_prev;

	static IDirect3DTexture2 *get_head; 

	IDirect3DTexture2(int t):target(t),proxy(0)
	{ 
		DDRAW_CONSTRUCT(IDirect3DTexture2)   

		get_next = get_head?get_head:this; 	
		get_prev = get_head?get_head->get_prev:this; 

		get_next->get_prev = this;
		get_prev->get_next = this;

		get_head = this;

		surface = 0;
	}

	~IDirect3DTexture2()
	{
		if(!target) return;
				
		get_prev->get_next = get_next; get_next->get_prev = get_prev;

		if(get_head==this) get_head = get_next==this?0:get_next;		

		get_prev = get_next = 0;

		DDRAW_DESTRUCT(IDirect3DTexture2)
	}

	/*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IDirect3DTexture2 methods ***/
	STDMETHOD(GetHandle)(THIS_ DX::LPDIRECT3DDEVICE2,DX::LPD3DTEXTUREHANDLE) PURE;
    STDMETHOD(PaletteChanged)(THIS_ DWORD,DWORD) PURE;
    STDMETHOD(Load)(THIS_ DX::LPDIRECT3DTEXTURE2) PURE;
};}

struct IDirect3DLight;

namespace DDRAW{
class IDirect3DLight : public DX::IDirect3DLight
{
DDRAW_INTERFACE(IDirect3DLight) //public

	static const int source = 'dxf';
	
	union
	{
	::IDirect3DLight *proxy6;

	IUnknown *proxy; //should remain "null"
	};

	IDirect3DLight(int t):target(t),proxy(0)
	{ 
		DDRAW_CONSTRUCT(IDirect3DLight)   
	}

	~IDirect3DLight()
	{
		if(!target) return;
		
		DDRAW_DESTRUCT(IDirect3DLight)
	}

	/*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
	/*** IDirect3DLight methods ***/
    STDMETHOD(Initialize)(THIS_ DX::LPDIRECT3D) PURE;
    STDMETHOD(SetLight)(THIS_ DX::LPD3DLIGHT) PURE;
    STDMETHOD(GetLight)(THIS_ DX::LPD3DLIGHT) PURE;
};}

struct IDirect3DMaterial3;

namespace DDRAW{
class IDirect3DMaterial3 : public DX::IDirect3DMaterial3
{
DDRAW_INTERFACE(IDirect3DMaterial3) //public

	static const int source = 'dx3';
	
	union
	{
	::IDirect3DMaterial3 *proxy6;

	IUnknown *proxy; //should remain "null"
	};

	IDirect3DMaterial3(int t):target(t),proxy(0)
	{ 
		DDRAW_CONSTRUCT(IDirect3DMaterial3)   
	}

	~IDirect3DMaterial3()
	{
		if(!target) return;
		
		DDRAW_DESTRUCT(IDirect3DMaterial3)
	}

	/*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IDirect3DMaterial3 methods ***/
    STDMETHOD(SetMaterial)(THIS_ DX::LPD3DMATERIAL) PURE;
    STDMETHOD(GetMaterial)(THIS_ DX::LPD3DMATERIAL) PURE;
    STDMETHOD(GetHandle)(THIS_ DX::LPDIRECT3DDEVICE3,DX::LPD3DMATERIALHANDLE) PURE;
};}

struct IDirect3DViewport3;

namespace DDRAW{
class IDirect3DViewport3 : public DX::IDirect3DViewport3
{
DDRAW_INTERFACE(IDirect3DViewport3) //public

	static const int source = 'dx3';
	
	union
	{
	::IDirect3DViewport3 *proxy6;

	IUnknown *proxy; //should remain "null"
	};

	//assuming belongs to single device
	DDRAW::IDirect3DViewport3 *prev, *next;

	DX::D3DVIEWPORT dims;
		
	IDirect3DViewport3(int t, void *q=0):target(t),proxy(0)
	{ 
		DDRAW_CONSTRUCT(IDirect3DViewport3)					

		dims.dwSize = sizeof(DX::D3DVIEWPORT);

		prev = next = 0;
	}

	~IDirect3DViewport3()
	{ 
		if(!target) return;
		
		DDRAW_DESTRUCT(IDirect3DViewport3)
	}

    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IDirect3DViewport2 methods ***/
    STDMETHOD(Initialize)(THIS_ DX::LPDIRECT3D) PURE;
    STDMETHOD(GetViewport)(THIS_ DX::LPD3DVIEWPORT) PURE;
    STDMETHOD(SetViewport)(THIS_ DX::LPD3DVIEWPORT) PURE;
    STDMETHOD(TransformVertices)(THIS_ DWORD,DX::LPD3DTRANSFORMDATA,DWORD,LPDWORD) PURE;
    STDMETHOD(LightElements)(THIS_ DWORD,DX::LPD3DLIGHTDATA) PURE;
    STDMETHOD(SetBackground)(THIS_ DX::D3DMATERIALHANDLE) PURE;
    STDMETHOD(GetBackground)(THIS_ DX::LPD3DMATERIALHANDLE,LPBOOL) PURE;
    STDMETHOD(SetBackgroundDepth)(THIS_ DX::LPDIRECTDRAWSURFACE) PURE;
    STDMETHOD(GetBackgroundDepth)(THIS_ DX::LPDIRECTDRAWSURFACE*,LPBOOL) PURE;
    STDMETHOD(Clear)(THIS_ DWORD,DX::LPD3DRECT,DWORD) PURE;
    STDMETHOD(AddLight)(THIS_ DX::LPDIRECT3DLIGHT) PURE;
    STDMETHOD(DeleteLight)(THIS_ DX::LPDIRECT3DLIGHT) PURE;
    STDMETHOD(NextLight)(THIS_ DX::LPDIRECT3DLIGHT,DX::LPDIRECT3DLIGHT*,DWORD) PURE;
    STDMETHOD(GetViewport2)(THIS_ DX::LPD3DVIEWPORT2) PURE;
    STDMETHOD(SetViewport2)(THIS_ DX::LPD3DVIEWPORT2) PURE;
    STDMETHOD(SetBackgroundDepth2)(THIS_ DX::LPDIRECTDRAWSURFACE4) PURE;
    STDMETHOD(GetBackgroundDepth2)(THIS_ DX::LPDIRECTDRAWSURFACE4*,LPBOOL) PURE;
    STDMETHOD(Clear2)(THIS_ DWORD,DX::LPD3DRECT,DWORD,DX::D3DCOLOR,DX::D3DVALUE,DWORD) PURE;
};}

struct IDirect3D;

namespace DDRAW{
class IDirect3D3;
class IDirect3D7;
class IDirect3D : public DX::IDirect3D
{
public:
//There's not yet been a reason to implement DDRAW::IDirect3D 
//So this class is only a namespace for the Query structures below

	static const int source = 'dxf';
			
	typedef struct:DDRAW::IDirect3D__Query 
	{
		template<class T> inline void operator+=(T *in);
		template<class T> inline bool operator-=(T *in);

	}Query, Query6, Query7;

	typedef struct : public Query //9
	{		
		UINT adapter; //could use ddraw->adapter instead

	}Query9,QueryX;
};}

struct IDirect3D3;
struct IDirect3D7;
struct IDirect3D9Ex;

namespace DDRAW{
class IDirect3D3 : public DX::IDirect3D3
{
DDRAW_INTERFACE(IDirect3D3) //public
	
	static const int source = 'dx3';
			
	union
	{
	DDRAW::IDirect3D::Query *query;
	DDRAW::IDirect3D::Query6 *query6;
	DDRAW::IDirect3D::Query7 *query7;
	DDRAW::IDirect3D::Query9 *query9;
	DDRAW::IDirect3D::QueryX *queryX;
	DDRAW::qtable_t<IDirect3D> *qtable;
	};
		
	union 
	{
	::IDirect3D3 *proxy6;
	DX::IDirect3D7 *proxy;
	::IDirect3D7 *proxy7;
	::IDirect3D9Ex *proxy9;
	};
		
	typedef DDRAW::IDirect3D::Query Query;

	IDirect3D3(int t, void *q=0):target(t),proxy(0)
	{ 
		DDRAW_CONSTRUCT(IDirect3D3)   

		qtable = DDRAW::new_qtable<IDirect3D>(this,q);
	}						   

	~IDirect3D3()
	{ 
		if(!target) return;
		
		DDRAW::delete_qtable<IDirect3D>(this,qtable);

		DDRAW_DESTRUCT(IDirect3D3)
	}

    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IDirect3D3 methods ***/
	STDMETHOD(EnumDevices)(THIS_ DX::LPD3DENUMDEVICESCALLBACK,LPVOID) PURE;
    STDMETHOD(CreateLight)(THIS_ DX::LPDIRECT3DLIGHT*,LPUNKNOWN) PURE;
    STDMETHOD(CreateMaterial)(THIS_ DX::LPDIRECT3DMATERIAL3*,LPUNKNOWN) PURE;
    STDMETHOD(CreateViewport)(THIS_ DX::LPDIRECT3DVIEWPORT3*,LPUNKNOWN) PURE;
    STDMETHOD(FindDevice)(THIS_ DX::LPD3DFINDDEVICESEARCH,DX::LPD3DFINDDEVICERESULT) PURE;
    STDMETHOD(CreateDevice)(THIS_ REFCLSID,DX::LPDIRECTDRAWSURFACE4,DX::LPDIRECT3DDEVICE3*,LPUNKNOWN) PURE;
    STDMETHOD(CreateVertexBuffer)(THIS_ DX::LPD3DVERTEXBUFFERDESC,DX::LPDIRECT3DVERTEXBUFFER*,DWORD,LPUNKNOWN) PURE;
    STDMETHOD(EnumZBufferFormats)(THIS_ REFCLSID,DX::LPD3DENUMPIXELFORMATSCALLBACK,LPVOID) PURE;
    STDMETHOD(EvictManagedTextures)(THIS) PURE;
};}

namespace DDRAW{
class IDirect3D7 : public DX::IDirect3D7
{
DDRAW_INTERFACE(IDirect3D7) //public
	
	static const int source = 'dx7';
			
	union
	{
	DDRAW::IDirect3D::Query *query;
	DDRAW::IDirect3D::Query7 *query7;
	DDRAW::IDirect3D::Query9 *query9;
	DDRAW::IDirect3D::QueryX *queryX;
	DDRAW::qtable_t<IDirect3D> *qtable;
	};
		
	union 
	{
	DX::IDirect3D7 *proxy;
	::IDirect3D7 *proxy7;
	::IDirect3D9Ex *proxy9;
	};

	typedef DDRAW::IDirect3D::Query Query;

	IDirect3D7(int t, void *q=0):target(t),proxy(0)
	{ 
		DDRAW_CONSTRUCT(IDirect3D7)   

		qtable = DDRAW::new_qtable<IDirect3D>(this,q);
	}						   

	~IDirect3D7()
	{ 
		if(!target) return;
		
		DDRAW::delete_qtable<IDirect3D>(this,qtable);

		DDRAW_DESTRUCT(IDirect3D7)
	}

    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IDirect3D7 methods ***/
    STDMETHOD(EnumDevices)(THIS_ DX::LPD3DENUMDEVICESCALLBACK7,LPVOID) PURE;
    STDMETHOD(CreateDevice)(THIS_ REFCLSID,DX::LPDIRECTDRAWSURFACE7,DX::LPDIRECT3DDEVICE7*) PURE;
    STDMETHOD(CreateVertexBuffer)(THIS_ DX::LPD3DVERTEXBUFFERDESC,DX::LPDIRECT3DVERTEXBUFFER7*,DWORD) PURE;
    STDMETHOD(EnumZBufferFormats)(THIS_ REFCLSID,DX::LPD3DENUMPIXELFORMATSCALLBACK,LPVOID) PURE;
    STDMETHOD(EvictManagedTextures)(THIS) PURE;
};}

template<class T> 
inline void DDRAW::IDirect3D::Query::operator+=(T *in)
{ 
	if(in->source==dx3->source) *(void**)&dx3 = in;
	if(in->source==dx7->source) *(void**)&dx7 = in;
}									 
template<class T> 
inline bool DDRAW::IDirect3D::Query::operator-=(T *in)
{ 
	if(in->source==dx3->source) dx3 = 0;
	if(in->source==dx7->source) dx7 = 0; return dx3||dx7;
}

struct IDirect3DDevice;
struct IDirect3DVertexBuffer9;
struct IDirect3DVertexDeclaration9;
namespace Widgets95{ class xr; }

namespace DDRAW{
class IDirect3DDevice : public DX::IDirect3DDevice
{
public:
//There's not yet been a reason to implement DDRAW::IDirect3DDevice 
//So this class is only a namespace for the Query structures below

	static const int source = 'dxf';
			
	typedef struct:DDRAW::IDirect3DDevice__Query
	{
		template<class T> inline void operator+=(T *in);
		template<class T> inline bool operator-=(T *in);

		bool isSoftware;

	}Query, Query6;

	typedef struct : public Query //7
	{	
		//::IDirectDrawSurface7 *effects; //unused
	
	}Query7;

	typedef struct : public Query //X
	{
		DX::D3DDEVICEDESC7 caps;

		int texture_format_mask;

		int emulate; //if D3D9C::doEmulate: one of 'rgb' 'hal' 'tnl'

		int effects_w,effects_h; //2022

	}Query9X;

	typedef struct : public Query9X //9 //DUPLICATE
	{
		inline void operator~(); union
		{IUnknown *rel_ease[13]; struct //DUPLICATE
		{
		//[2]: for two frame blending
		::IDirect3DTexture9 *effects[2]; //render target/texture		
		::IDirect3DTexture9 *mrtargets[3];		
		::IDirect3DTexture9 *black; //blank texture
		::IDirect3DTexture9 *white; //default for pixel shaders
		::IDirect3DTexture9 *dither;
		::IDirect3DSurface9 *depthstencil; //DUPLICATE
		::IDirect3DVertexBuffer9 *stereo;
		::IDirect3DSurface9 *multisample; //MSAA render target //UNUSED
		::IDirect3DTexture9 *gamma[2]; //gamma9[DDRAW::inWindow] //UNUSED
		};};
	}Query9; //same layouts
	typedef struct : public Query9X //GL //DUPLICATE
	{
		//FIRST
		union{
		unsigned textures[8]; struct //DUPLICATE
		{
		unsigned effects[2];		
		unsigned mrtargets[3];		
		unsigned black,white,dither; 
		};};
		unsigned depthstencil; //DUPLICATE
		unsigned framebuffers[2];
		unsigned uniforms[2];
		unsigned samplers[3];

	  //TODO: wgl CAN SAFELY BE REMOVED//

		Widgets95::xr *xr; HGLRC wgl; //dx95/dxGL

		const void *effects_hlsl; //xr::effects
		const void *effects_cbuf;
		size_t effects_cbuf_size;

		unsigned stereo; //VBO

		struct StereoView
		{
			const void *src; //xr::section //P0289R0
			
			int x,y,w,h; //glViewport

		}stereo_views[2]; //4?

		bool stereo_ultrawide;
		unsigned stereo_fovea;
		float stereo_fovea_cmp[4]; //fz?

		struct UniformBlock;
		struct UniformBlock *uniforms2;

		union StateBlock
		{
			void apply_nanovg(); //HACK

			DWORD dword; struct //32 bits
			{
				//LSB for CreateStateBlock
			unsigned _fog:1; //LSB			
			unsigned cullmode:2; 
			unsigned fillmode:1; //solid
			unsigned shademode:1; //Gouraud //UNUSED
			unsigned colorwriteenable:4; //MRT
			unsigned zwriteenable:1;
			unsigned zfunc:4;
			unsigned zenable:1;
			unsigned alphablendenable:1; 
			unsigned srcblend:4;
			unsigned destblend:4;			
				//24 bits
				//these are TextureStageState
			unsigned addressu:2;
			unsigned addressv:2;
			unsigned minfilter:1; //linear
			unsigned magfilter:1; //linear
			unsigned mipfilter:2;
			};
			struct
			{
			unsigned rare_mask:9;
			unsigned z_mask:6;
			unsigned blend_mask:9;
			unsigned sampler_mask:8;
			};

			//TODO? because the blend states (and
			//min/mipfilter states) are tied together
			//under OpenGL deferring changes makes sense
			void apply(const StateBlock &cmp)const;

			void init()
			{
				//D3D's defaults/enums
				dword = 0;
				zwriteenable = 1;
				colorwriteenable = 1<<3;
				zfunc = 4; //LESSEQUAL
				cullmode = 3; //CCW
				fillmode = 1; //3 //SOLID
				shademode = 1; //2 //GOURAUD
				srcblend = 2; //ONE
				destblend = 1; //ZERO
				addressu = 1; //WRAP
				addressv = 1; //WRAP
				minfilter = 0; //POINT
				magfilter = 0; //POINT
			}

			StateBlock(DWORD cp):dword(cp){}

		}state;

	}QueryGL;

	typedef struct : public Query9X
	{
		void _delete_operatorGL();

		inline void operator~()
		{
			switch(_target)
			{
			case 'dx9c':
			((Query9*)this)->operator~(); break;
			case 'dx95': case 'dxGL':
			_delete_operatorGL();
			((Query9X*)this)->operator~(); break;
			default: assert(0);
			}			
		}

		char _9x_pad[(int)sizeof(Query9)-(int)sizeof(Query9X)];
		char _gl_pad[(int)sizeof(QueryGL)-(int)sizeof(Query9)];

	}QueryX;

};}

struct IDirect3DDevice3;
struct IDirect3DDevice7;
struct IDirect3DDevice9Ex;

namespace DDRAW{
class IDirect3DDevice3 : public DX::IDirect3DDevice3
{
DDRAW_INTERFACE(IDirect3DDevice3) //public
	
	static const int source = 'dx3';
	
	union
	{
	DDRAW::IDirect3DDevice::Query *query;
	DDRAW::IDirect3DDevice::Query6 *query6;
	DDRAW::IDirect3DDevice::Query7 *query7;
	DDRAW::IDirect3DDevice::Query9 *query9;
	DDRAW::IDirect3DDevice::QueryX *queryX;
	DDRAW::qtable_t<IDirect3DDevice> *qtable;
	};

	union
	{
	::IDirect3DDevice3 *proxy6;
	DX::IDirect3DDevice7 *proxy;
	::IDirect3DDevice7 *proxy7;
	::IDirect3DDevice9Ex *proxy9;
	};
	
	DDRAW::IDirect3DViewport3 *views;

	typedef DDRAW::IDirect3DDevice::Query Query;

	IDirect3DDevice3(int t, void *q=0):target(t),proxy(0)
	{ 
		DDRAW_CONSTRUCT(IDirect3DDevice3)			

		qtable = DDRAW::new_qtable<IDirect3DDevice>(this,q);

		views = 0;
	}

	~IDirect3DDevice3()
	{
		if(!target) return;
		
		DDRAW::delete_qtable<IDirect3DDevice>(this,qtable);

		DDRAW_DESTRUCT(IDirect3DDevice3)
	}

    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
	/*** IDirect3DDevice3 methods ***/
	STDMETHOD(GetCaps)(THIS_ DX::LPD3DDEVICEDESC,DX::LPD3DDEVICEDESC) PURE;
    STDMETHOD(GetStats)(THIS_ DX::LPD3DSTATS) PURE;
    STDMETHOD(AddViewport)(THIS_ DX::LPDIRECT3DVIEWPORT3) PURE;
    STDMETHOD(DeleteViewport)(THIS_ DX::LPDIRECT3DVIEWPORT3) PURE;
    STDMETHOD(NextViewport)(THIS_ DX::LPDIRECT3DVIEWPORT3,DX::LPDIRECT3DVIEWPORT3*,DWORD) PURE;
    STDMETHOD(EnumTextureFormats)(THIS_ DX::LPD3DENUMPIXELFORMATSCALLBACK,LPVOID) PURE;
    STDMETHOD(BeginScene)(THIS) PURE;
    STDMETHOD(EndScene)(THIS) PURE;
    STDMETHOD(GetDirect3D)(THIS_ DX::LPDIRECT3D3*) PURE;
    STDMETHOD(SetCurrentViewport)(THIS_ DX::LPDIRECT3DVIEWPORT3) PURE;
    STDMETHOD(GetCurrentViewport)(THIS_ DX::LPDIRECT3DVIEWPORT3 *) PURE;
    STDMETHOD(SetRenderTarget)(THIS_ DX::LPDIRECTDRAWSURFACE4,DWORD) PURE;
    STDMETHOD(GetRenderTarget)(THIS_ DX::LPDIRECTDRAWSURFACE4 *) PURE;
    STDMETHOD(Begin)(THIS_ DX::D3DPRIMITIVETYPE,DWORD,DWORD) PURE;
    STDMETHOD(BeginIndexed)(THIS_ DX::D3DPRIMITIVETYPE,DWORD,LPVOID,DWORD,DWORD) PURE;
    STDMETHOD(Vertex)(THIS_ LPVOID) PURE;
    STDMETHOD(Index)(THIS_ WORD) PURE;
    STDMETHOD(End)(THIS_ DWORD) PURE;
    STDMETHOD(GetRenderState)(THIS_ DX::D3DRENDERSTATETYPE,LPDWORD) PURE;
    STDMETHOD(SetRenderState)(THIS_ DX::D3DRENDERSTATETYPE,DWORD) PURE;
    STDMETHOD(GetLightState)(THIS_ DX::D3DLIGHTSTATETYPE,LPDWORD) PURE;
    STDMETHOD(SetLightState)(THIS_ DX::D3DLIGHTSTATETYPE,DWORD) PURE;
    STDMETHOD(SetTransform)(THIS_ DX::D3DTRANSFORMSTATETYPE,DX::LPD3DMATRIX) PURE;
    STDMETHOD(GetTransform)(THIS_ DX::D3DTRANSFORMSTATETYPE,DX::LPD3DMATRIX) PURE;
    STDMETHOD(MultiplyTransform)(THIS_ DX::D3DTRANSFORMSTATETYPE,DX::LPD3DMATRIX) PURE;
    STDMETHOD(DrawPrimitive)(THIS_ DX::D3DPRIMITIVETYPE,DWORD,LPVOID,DWORD,DWORD) PURE;
    STDMETHOD(DrawIndexedPrimitive)(THIS_ DX::D3DPRIMITIVETYPE,DWORD,LPVOID,DWORD,LPWORD,DWORD,DWORD) PURE;
    STDMETHOD(SetClipStatus)(THIS_ DX::LPD3DCLIPSTATUS) PURE;
    STDMETHOD(GetClipStatus)(THIS_ DX::LPD3DCLIPSTATUS) PURE;
    STDMETHOD(DrawPrimitiveStrided)(THIS_ DX::D3DPRIMITIVETYPE,DWORD,DX::LPD3DDRAWPRIMITIVESTRIDEDDATA,DWORD,DWORD) PURE;
	STDMETHOD(DrawIndexedPrimitiveStrided)(THIS_ DX::D3DPRIMITIVETYPE,DWORD,DX::LPD3DDRAWPRIMITIVESTRIDEDDATA,DWORD,LPWORD,DWORD,DWORD) PURE;
    STDMETHOD(DrawPrimitiveVB)(THIS_ DX::D3DPRIMITIVETYPE,DX::LPDIRECT3DVERTEXBUFFER,DWORD,DWORD,DWORD) PURE;
    STDMETHOD(DrawIndexedPrimitiveVB)(THIS_ DX::D3DPRIMITIVETYPE,DX::LPDIRECT3DVERTEXBUFFER,LPWORD,DWORD,DWORD) PURE;
    STDMETHOD(ComputeSphereVisibility)(THIS_ DX::LPD3DVECTOR,DX::LPD3DVALUE,DWORD,DWORD,LPDWORD) PURE;
    STDMETHOD(GetTexture)(THIS_ DWORD,DX::LPDIRECT3DTEXTURE2 *) PURE;
    STDMETHOD(SetTexture)(THIS_ DWORD,DX::LPDIRECT3DTEXTURE2) PURE;
    STDMETHOD(GetTextureStageState)(THIS_ DWORD,DX::D3DTEXTURESTAGESTATETYPE,LPDWORD) PURE;
    STDMETHOD(SetTextureStageState)(THIS_ DWORD,DX::D3DTEXTURESTAGESTATETYPE,DWORD) PURE;
    STDMETHOD(ValidateDevice)(THIS_ LPDWORD) PURE;
};}

namespace DDRAW{
class IDirect3DDevice7 : public DX::IDirect3DDevice7
{
DDRAW_INTERFACE(IDirect3DDevice7) //public
	
	static const int source = 'dx7';
	
	union
	{
	DDRAW::IDirect3DDevice::Query *query;
	DDRAW::IDirect3DDevice::Query7 *query7;
	DDRAW::IDirect3DDevice::Query9 *query9;
	DDRAW::IDirect3DDevice::QueryX *queryX;
	DDRAW::IDirect3DDevice::QueryGL *queryGL;
	DDRAW::qtable_t<IDirect3DDevice> *qtable;
	};

	union
	{
	DX::IDirect3DDevice7 *proxy;
	::IDirect3DDevice7 *proxy7;
	::IDirect3DDevice9Ex *proxy9;
	};
	
	typedef DDRAW::IDirect3DDevice::Query Query;
	typedef DDRAW::IDirect3DDevice::Query7 Query7;
	typedef DDRAW::IDirect3DDevice::Query9 Query9;
	typedef DDRAW::IDirect3DDevice::QueryX QueryX;
	typedef DDRAW::IDirect3DDevice::QueryGL QueryGL;

	IDirect3DDevice7(int t, void *q=0):target(t),proxy(0)
	{ 
		DDRAW_CONSTRUCT(IDirect3DDevice7)			

		qtable = DDRAW::new_qtable<IDirect3DDevice>(this,q);
	}

	~IDirect3DDevice7()
	{
		if(!target) return;
		
		DDRAW::delete_qtable<IDirect3DDevice>(this,qtable);

		DDRAW_DESTRUCT(IDirect3DDevice7)
	}

    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IDirect3DDevice7 methods ***/
    STDMETHOD(GetCaps)(THIS_ DX::LPD3DDEVICEDESC7) PURE;
    STDMETHOD(EnumTextureFormats)(THIS_ DX::LPD3DENUMPIXELFORMATSCALLBACK,LPVOID) PURE;
    STDMETHOD(BeginScene)(THIS) PURE;
    STDMETHOD(EndScene)(THIS) PURE;
    STDMETHOD(GetDirect3D)(THIS_ DX::LPDIRECT3D7*) PURE;
    STDMETHOD(SetRenderTarget)(THIS_ DX::LPDIRECTDRAWSURFACE7,DWORD) PURE;
    STDMETHOD(GetRenderTarget)(THIS_ DX::LPDIRECTDRAWSURFACE7 *) PURE;
    STDMETHOD(Clear)(THIS_ DWORD,DX::LPD3DRECT,DWORD,DX::D3DCOLOR,DX::D3DVALUE,DWORD) PURE;
    STDMETHOD(SetTransform)(THIS_ DX::D3DTRANSFORMSTATETYPE,DX::LPD3DMATRIX) PURE;
    STDMETHOD(GetTransform)(THIS_ DX::D3DTRANSFORMSTATETYPE,DX::LPD3DMATRIX) PURE;
    STDMETHOD(SetViewport)(THIS_ DX::LPD3DVIEWPORT7) PURE;
    STDMETHOD(MultiplyTransform)(THIS_ DX::D3DTRANSFORMSTATETYPE,DX::LPD3DMATRIX) PURE;
    STDMETHOD(GetViewport)(THIS_ DX::LPD3DVIEWPORT7) PURE;
    STDMETHOD(SetMaterial)(THIS_ DX::LPD3DMATERIAL7) PURE;
    STDMETHOD(GetMaterial)(THIS_ DX::LPD3DMATERIAL7) PURE;
    STDMETHOD(SetLight)(THIS_ DWORD,DX::LPD3DLIGHT7) PURE;
    STDMETHOD(GetLight)(THIS_ DWORD,DX::LPD3DLIGHT7) PURE;
    STDMETHOD(SetRenderState)(THIS_ DX::D3DRENDERSTATETYPE,DWORD) PURE;
    STDMETHOD(GetRenderState)(THIS_ DX::D3DRENDERSTATETYPE,LPDWORD) PURE;
    STDMETHOD(BeginStateBlock)(THIS) PURE;
    STDMETHOD(EndStateBlock)(THIS_ LPDWORD) PURE;
    STDMETHOD(PreLoad)(THIS_ DX::LPDIRECTDRAWSURFACE7) PURE;
    STDMETHOD(DrawPrimitive)(THIS_ DX::D3DPRIMITIVETYPE,DWORD,LPVOID,DWORD,DWORD) PURE;
    STDMETHOD(DrawIndexedPrimitive)(THIS_ DX::D3DPRIMITIVETYPE,DWORD,LPVOID,DWORD,LPWORD,DWORD,DWORD) PURE;
    STDMETHOD(SetClipStatus)(THIS_ DX::LPD3DCLIPSTATUS) PURE;
    STDMETHOD(GetClipStatus)(THIS_ DX::LPD3DCLIPSTATUS) PURE;
    STDMETHOD(DrawPrimitiveStrided)(THIS_ DX::D3DPRIMITIVETYPE,DWORD,DX::LPD3DDRAWPRIMITIVESTRIDEDDATA,DWORD,DWORD) PURE;
    STDMETHOD(DrawIndexedPrimitiveStrided)(THIS_ DX::D3DPRIMITIVETYPE,DWORD,DX::LPD3DDRAWPRIMITIVESTRIDEDDATA,DWORD,LPWORD,DWORD,DWORD) PURE;
    STDMETHOD(DrawPrimitiveVB)(THIS_ DX::D3DPRIMITIVETYPE,DX::LPDIRECT3DVERTEXBUFFER7,DWORD,DWORD,DWORD) PURE;
    STDMETHOD(DrawIndexedPrimitiveVB)(THIS_ DX::D3DPRIMITIVETYPE,DX::LPDIRECT3DVERTEXBUFFER7,DWORD,DWORD,LPWORD,DWORD,DWORD) PURE;
    STDMETHOD(ComputeSphereVisibility)(THIS_ DX::LPD3DVECTOR,DX::LPD3DVALUE,DWORD,DWORD,LPDWORD) PURE;
    STDMETHOD(GetTexture)(THIS_ DWORD,DX::LPDIRECTDRAWSURFACE7 *) PURE;
    STDMETHOD(SetTexture)(THIS_ DWORD,DX::LPDIRECTDRAWSURFACE7) PURE;
    STDMETHOD(GetTextureStageState)(THIS_ DWORD,DX::D3DTEXTURESTAGESTATETYPE,LPDWORD) PURE;
    STDMETHOD(SetTextureStageState)(THIS_ DWORD,DX::D3DTEXTURESTAGESTATETYPE,DWORD) PURE;
    STDMETHOD(ValidateDevice)(THIS_ LPDWORD) PURE;
    STDMETHOD(ApplyStateBlock)(THIS_ DWORD) PURE;
    STDMETHOD(CaptureStateBlock)(THIS_ DWORD) PURE;
    STDMETHOD(DeleteStateBlock)(THIS_ DWORD) PURE;
    STDMETHOD(CreateStateBlock)(THIS_ DX::D3DSTATEBLOCKTYPE,LPDWORD) PURE;
    STDMETHOD(Load)(THIS_ DX::LPDIRECTDRAWSURFACE7,LPPOINT,DX::LPDIRECTDRAWSURFACE7,LPRECT,DWORD) PURE;
    STDMETHOD(LightEnable)(THIS_ DWORD,BOOL) PURE;
    STDMETHOD(GetLightEnable)(THIS_ DWORD,BOOL*) PURE;
	STDMETHOD(SetClipPlane)(THIS_ DWORD,DX::D3DVALUE*) PURE;
    STDMETHOD(GetClipPlane)(THIS_ DWORD,DX::D3DVALUE*) PURE;
    STDMETHOD(GetInfo)(THIS_ DWORD,LPVOID,DWORD) PURE;
};}

template<class T> 
inline void DDRAW::IDirect3DDevice::Query::operator+=(T *in)
{ 
	if(in->source==dx3->source) *(void**)&dx3 = in;
	if(in->source==dx7->source) *(void**)&dx7 = in;
}
template<class T> 
inline bool DDRAW::IDirect3DDevice::Query::operator-=(T *in)
{ 
	if(in->source==dx3->source) dx3 = 0;
	if(in->source==dx7->source) dx7 = 0; return dx3||dx7;
}
inline void DDRAW::IDirect3DDevice::Query9::operator~()
{
	DDRAW::IDirect3DDevice::Query::operator~();

	for(size_t i=0;i<sizeof(rel_ease)/sizeof(*rel_ease);i++)
	if(rel_ease[i]) rel_ease[i]->Release();	
}

struct IDirect3DVertexBuffer;
struct IDirect3DVertexBuffer7;
struct IDirect3DVertexBuffer9;

namespace DDRAW{
class IDirect3DVertexBuffer : public DX::IDirect3DVertexBuffer
{
DDRAW_INTERFACE(IDirect3DVertexBuffer) //public

	static const int source = 'dxf';

	typedef struct:IDirect3DVertexBuffer__Query
	{	
		template<class T> inline void operator+=(T *in);
		template<class T> inline bool operator-=(T *in);

	}Query, Query6, Query7;

	typedef struct : public Query //9
	{	
		void *upbuffer; //DrawIndexedPrimitivesUP

		UINT mixed_mode_lock, mixed_mode_size;

		DWORD FVF; UINT sizeofFVF;

		unsigned int vaoGL,vboGL; //GLuint

		void _vboGL_delete();

		inline void operator~(); 

	}Query9,QueryX;

	union
	{
	DDRAW::IDirect3DVertexBuffer::Query *query;
	DDRAW::IDirect3DVertexBuffer::Query6 *query6;
	DDRAW::IDirect3DVertexBuffer::Query7 *query7;
	DDRAW::IDirect3DVertexBuffer::Query9 *query9;
	DDRAW::IDirect3DVertexBuffer::QueryX *queryX;
	DDRAW::qtable_t<IDirect3DVertexBuffer> *qtable;
	};
	
	union
	{
	::IDirect3DVertexBuffer *proxy6;
	DX::IDirect3DVertexBuffer7 *proxy;
	::IDirect3DVertexBuffer7 *proxy7;
	::IDirect3DVertexBuffer9 *proxy9;
	};

	typedef DDRAW::IDirect3DVertexBuffer::Query Query;
	
	IDirect3DVertexBuffer(int t, void *q=0):target(t),proxy(0)
	{ 
		DDRAW_CONSTRUCT(IDirect3DVertexBuffer)   		

		qtable = DDRAW::new_qtable<IDirect3DVertexBuffer>(this,q);
	}

	~IDirect3DVertexBuffer()
	{
		if(!target) return;
		
		DDRAW::delete_qtable<IDirect3DVertexBuffer>(this,qtable);

		DDRAW_DESTRUCT(IDirect3DVertexBuffer)
	}

    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirect3DVertexBuffer methods ***/
    STDMETHOD(Lock)(THIS_ DWORD,LPVOID*,LPDWORD) PURE;
    STDMETHOD(Unlock)(THIS) PURE;
    STDMETHOD(ProcessVertices)(THIS_ DWORD,DWORD,DWORD,DX::LPDIRECT3DVERTEXBUFFER,DWORD,DX::LPDIRECT3DDEVICE3,DWORD) PURE;
    STDMETHOD(GetVertexBufferDesc)(THIS_ DX::LPD3DVERTEXBUFFERDESC) PURE;
	STDMETHOD(Optimize)(THIS_ DX::LPDIRECT3DDEVICE3,DWORD) PURE;
};}
 
namespace DDRAW{
class IDirect3DVertexBuffer7 : public DX::IDirect3DVertexBuffer7
{
DDRAW_INTERFACE(IDirect3DVertexBuffer7) //public

	static const int source = 'dx7';
	
	union
	{
	DDRAW::IDirect3DVertexBuffer::Query *query;
	DDRAW::IDirect3DVertexBuffer::Query7 *query7;
	DDRAW::IDirect3DVertexBuffer::Query9 *query9;
	DDRAW::IDirect3DVertexBuffer::QueryX *queryX;
	DDRAW::qtable_t<IDirect3DVertexBuffer> *qtable;
	};

	union
	{
	DX::IDirect3DVertexBuffer7 *proxy;
	::IDirect3DVertexBuffer7 *proxy7;
	::IDirect3DVertexBuffer9 *proxy9;
	};
		
	IDirect3DVertexBuffer7(int t, void *q=0):target(t),proxy(0)
	{ 
		DDRAW_CONSTRUCT(IDirect3DVertexBuffer7)			

		qtable = DDRAW::new_qtable<IDirect3DVertexBuffer>(this,q);
	}

	~IDirect3DVertexBuffer7()
	{ 
		if(!target) return;
		
		DDRAW::delete_qtable<IDirect3DVertexBuffer>(this,qtable);
				
		DDRAW_DESTRUCT(IDirect3DVertexBuffer7)
	}

    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IDirect3DVertexBuffer7 methods ***/
    STDMETHOD(Lock)(THIS_ DWORD,LPVOID*,LPDWORD) PURE;	
	//2021: this is an OPTIMIZATION to reduce the amount of memory
	//uploaded since D3D7 could only do a full lock. without this
	//it's necessary to keep vbuffers small to avoid uploading junk
	//when the buffer can't be filled. size is passed via cIO. an
	//offset could be passed via bIO if required (not implemented)
	inline HRESULT partial_Lock(DWORD a, LPVOID *bIO, LPDWORD cIO)
	{
		assert(*bIO==0); //TODO? allow offset if DDRAW::compat=='d3d9c'
		assert(*cIO!=0);			
		return Lock(a|0x80000000L,bIO,cIO);
	}
    STDMETHOD(Unlock)(THIS) PURE;
	STDMETHOD(ProcessVertices)(THIS_ DWORD,DWORD,DWORD,DX::LPDIRECT3DVERTEXBUFFER7,DWORD,DX::LPDIRECT3DDEVICE7,DWORD) PURE;
    STDMETHOD(GetVertexBufferDesc)(THIS_ DX::LPD3DVERTEXBUFFERDESC) PURE;
    STDMETHOD(Optimize)(THIS_ DX::LPDIRECT3DDEVICE7,DWORD) PURE;
    STDMETHOD(ProcessVerticesStrided)(THIS_ DWORD,DWORD,DWORD,DX::LPD3DDRAWPRIMITIVESTRIDEDDATA,DWORD,DX::LPDIRECT3DDEVICE7,DWORD) PURE;
};}

template<class T> 
inline void DDRAW::IDirect3DVertexBuffer::Query::operator+=(T *in)
{ 
	if(in->source==dxf->source) *(void**)&dxf = in;
	if(in->source==dx7->source) *(void**)&dx7 = in;
}						
template<class T> 
inline bool DDRAW::IDirect3DVertexBuffer::Query::operator-=(T *in)
{ 
	if(in->source==dxf->source) dxf = 0;
	if(in->source==dx7->source) dx7 = 0; return dxf||dx7;
}
inline void DDRAW::IDirect3DVertexBuffer::Query9::operator~()
{
	DDRAW::IDirect3DVertexBuffer::Query::operator~();

	delete[] upbuffer;
}

#undef STDMETHOD_
#undef STDMETHOD
#undef PURE

//see: push_macro("PURE")
#pragma pop_macro("PURE") 
#pragma pop_macro("STDMETHOD")
#pragma pop_macro("STDMETHOD_")

#endif //DX_DDRAW_INCLUDED