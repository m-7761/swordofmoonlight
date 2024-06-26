
#include "directx.h" 
DX_TRANSLATION_UNIT //(C)

	//PREAMBLE: since July 2021 this file is no longer
	//used when DDRAW::shader is 0, indicating shaders
	//are provided by the app. instead dx.d3d9X.cpp is
	//used in that case. it removes all of the ancient
	//fixed-function and build-in shader, multisamples
	//and other experimental features of yore. it adds
	//an OpengGL ES mode for OpenXR/ANGLE side-by-side.

#include <map>
#include <vector>
#include <algorithm>

#define DIRECT3D_VERSION 0x0900

#include <ddraw.h>
#ifdef _DEBUG
//#define D3D_DEBUG_INFO
#endif
#include <d3d9.h>
#include <d3dx9.h> //REMOVE ME?
#pragma comment(lib,"d3dx9.lib")
#ifdef D3D_DEBUG_INFO
#define DDRAW_ASSERT_REFS(x,y) ; //winsanity
#else 
static int DDRAW_ASSERT_REFS_refs;
#define DDRAW_ASSERT_REFS(x,y) if(DX::debug&&x) \
{ int &refs = DDRAW_ASSERT_REFS_refs; \
refs = -1+x->AddRef(); assert(refs y); x->Release(); }
#endif

#define DDRAW_OVERLOADS	  
//long version of dx.d3d9c.h
#define D3D9C_INCLUDE_INTERFACES //REMOVE ME 
#include "dx.d3d9c.h"

static int dx95; //2021

//REMINDER: dx.d3d9c.cpp expects this to be
//SYSTEMMEM. if it's changed back it has to
//be able to override it
extern const D3DPOOL
dx_d3d9c_texture_pool = D3DPOOL_SYSTEMMEM;

//2021: this isn't being applied consistently
//static const bool dx_d3d9c_preload = false; 

//2018: immediate mode had performed so much better
//because of how SOM locks map tiles. But it is not
//good on Intel drivers (at least) because it can't
//be used with stereo, it was discovered that it is
//much better in general to go full buffer VR or no
//som_scene_4137F0_Lock is solving the tile problem
extern const bool dx_d3d9c_immediate_mode; //false;

//NOTE: this currently assumes the app always calls
//SetColorKey on mipmapped textures
extern const bool dx_d3d9c_imitate_colorkey = true;

extern const bool dx_d3d9c_stereo_scissor = true; //2021

extern char *DDRAW::error(HRESULT err)
{
#define CASE_(E) case E: return #E;

	switch(err)
	{
	CASE_(DDRAW_UNIMPLEMENTED)

	CASE_(S_FALSE)
	CASE_(E_FAIL)

	CASE_(D3DERR_WRONGTEXTUREFORMAT)
	CASE_(D3DERR_UNSUPPORTEDCOLOROPERATION)
	CASE_(D3DERR_UNSUPPORTEDCOLORARG)
	CASE_(D3DERR_UNSUPPORTEDALPHAOPERATION)
	CASE_(D3DERR_UNSUPPORTEDALPHAARG)
	CASE_(D3DERR_TOOMANYOPERATIONS)
	CASE_(D3DERR_CONFLICTINGTEXTUREFILTER)
	CASE_(D3DERR_UNSUPPORTEDFACTORVALUE)
	CASE_(D3DERR_CONFLICTINGRENDERSTATE)
	CASE_(D3DERR_UNSUPPORTEDTEXTUREFILTER)
	CASE_(D3DERR_CONFLICTINGTEXTUREPALETTE)
	CASE_(D3DERR_DRIVERINTERNALERROR)
								  
	CASE_(D3DERR_NOTFOUND)
	CASE_(D3DERR_MOREDATA)
	CASE_(D3DERR_DEVICELOST)
	CASE_(D3DERR_DEVICENOTRESET)
	CASE_(D3DERR_NOTAVAILABLE)
	CASE_(D3DERR_OUTOFVIDEOMEMORY)
	CASE_(D3DERR_INVALIDDEVICE)
	CASE_(D3DERR_INVALIDCALL)
	CASE_(D3DERR_DRIVERINVALIDCALL)
	CASE_(D3DERR_WASSTILLDRAWING)
	CASE_(D3DOK_NOAUTOGEN)
	//Ex
	CASE_(D3DERR_DEVICEREMOVED)
	CASE_(S_NOT_RESIDENT)
	CASE_(S_RESIDENT_IN_SHARED_MEMORY)
	CASE_(S_PRESENT_MODE_CHANGED)
	CASE_(S_PRESENT_OCCLUDED)
	CASE_(D3DERR_DEVICEHUNG)
	CASE_(D3DERR_UNSUPPORTEDOVERLAY)
	CASE_(D3DERR_UNSUPPORTEDOVERLAYFORMAT)
	CASE_(D3DERR_CANNOTPROTECTCONTENT)
	CASE_(D3DERR_UNSUPPORTEDCRYPTO)
	CASE_(D3DERR_PRESENT_STATISTICS_DISJOINT)

	default: assert(0); //2021
	}

	return "";

#undef CASE_
}

//NOTE: this is chips/cards, not resolutions
enum{ dx_d3d9c_display_adaptersN=16 }; //ARBITRARY?
extern GUID dx_d3d9c_display_adapters[dx_d3d9c_display_adaptersN] = {};
extern GUID dx_d3d9c_device_x = {'devi','ce','_x','d','x','_','d','3','d','9','c'};

extern D3D9C::Create9Ex D3D9C::DLL::Direct3DCreate9Ex = 0; 
extern D3D9C::Create9 D3D9C::DLL::Direct3DCreate9 = 0; 
extern D3D9C::Create9On12Ex D3D9C::DLL::Direct3DCreate9On12Ex = 0; 

static HMODULE _dx_d3d9c_sw() //Software rasterize
{
	HMODULE out = LoadLibraryA("RGB9Rast_1.dll");

	if(out) DDRAW_HELLO(0) << "Software Device Library RGB9Rast_1.dll loaded succefully\n";

	else DDRAW_ALERT(0) << "Software Device Library RGB9Rast_1.dll failed to load\n";

	return out;
}
static HMODULE dx_d3d9c_sw() //Software rasterizer
{
	if(!DDRAW::doSoftware) return 0;

	static HMODULE out = _dx_d3d9c_sw(); return out;
}

//2022: this needs to be called earlier than
//CreateDevice
extern IDirect3DDevice9 *dx_d3d9c_old_device = 0;
extern int dx_d3d9c_force_release_old_device()
{
	int ret = 0;

	//MEMORY LEAK
	//MEMORY LEAK!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//MEMORY LEAK
//	if(out==E_ACCESSDENIED)
	if(auto*d=dx_d3d9c_old_device)
	{
		dx_d3d9c_old_device = 0;

		//INVESTIGATE
		//can't think of any leaks off hand... tried ex.fonts to
		//no avail
	//	extern int SOMEX_vnumber;
	//	assert(SOMEX_vnumber<=0x1020308UL);
		//MEMORY LEAK?
		//only get here with D3DSWAPEFFECT_FLIPEX 
		//#ifdef NDEBUG
		//#error This is a bad indicator, but a useful one also
		//#endif
		//
		// NOTE: I think Release may return a number that's larger
		// than there were AddRef calls and I don't know which one
		// of them is used by E_ACCESSDENIED  
		//
		//int debug = dx_d3d9c_old_device->Release();
		//retries++; goto retry;
		//if(!debug) dx_d3d9c_old_device = nullptr;
		ret = d->Release();
		for(int i=ret;i-->0;) d->Release();
	
	}
//	else assert(0);

	return ret;
}

static auto*const dx_d3d9c_virtual_group = (IDirect3DTexture9*)8;
static auto*const dx_d3d9c_virtual_surface = (IDirect3DSurface9*)8; //REMOVE ME?

//REMINDER: backbuffer is filled in by GetAttachedSurface
//+1: ItemEdit makes a mystery surface at 402698... I don't
//think it's used but it must have taken the backbuffer slot?
enum{ dx_d3d9c_direct3d_swapN = 3+1 };
extern DX::DDSURFACEDESC2 dx_d3d9c_direct3d_swapchain[dx_d3d9c_direct3d_swapN] = {};
extern DDRAW::IDirectDrawSurface7 *dx_d3d9c_direct3d_swapsurfs[dx_d3d9c_direct3d_swapN] = {};

//2021: using DDRAW::mrtargets9[0] to commuincate this format to
//fx9_effects_buffers_require_reevaluation and no longer spoofing
//the effects buffer when getting the back-buffer descriptor since
//supersampling isn't 1-to-1 with the pitch
//extern INT dx_d3d9c_rendertarget_pitch = 0;
//extern D3DFORMAT dx_d3d9c_rendertarget_format = D3DFMT_UNKNOWN;

//TODO: THESE SHOULD GO INSIDE Query9
extern UINT dx_d3d9c_ibuffers_s[D3D9C::ibuffersN] = {}; 
extern UINT dx_d3d9c_vbuffers_s[D3D9C::ibuffersN] = {}; 
extern IDirect3DIndexBuffer9 *dx_d3d9c_ibuffers[D3D9C::ibuffersN] = {}; 
extern IDirect3DVertexBuffer9 *dx_d3d9c_vbuffers[D3D9C::ibuffersN] = {}; 
extern IDirect3DVertexShader9 *dx_d3d9c_vshaders[16+4] = {}; 
extern IDirect3DPixelShader9 *dx_d3d9c_pshaders[16+4] = {};
extern IDirect3DPixelShader9 *dx_d3d9c_pshaders2[16+4] = {};
extern IDirect3DPixelShader9**dx_d3d9c_pshaders_noclip = dx_d3d9c_pshaders;
extern IDirect3DSurface9 *dx_d3d9c_alphasurface = 0;
extern IDirect3DTexture9 *dx_d3d9c_alphatexture = 0;
extern DWORD dx_d3d9c_anisotropy_max = 0; 
extern DWORD dx_d3d9c_beginscene = 0, dx_d3d9c_endscene = 0;

static const DWORD //getting out of hand
*dx_d3d9c_df = 0, *dx_d3d9c_ck = 0, *dx_d3d9c_fx = 0, *dx_d3d9c_cs[2] = {};
static IDirect3DPixelShader9 
*dx_d3d9c_defaultshader = 0, *dx_d3d9c_colorkeyshader = 0,
*dx_d3d9c_effectsshader = 0, *dx_d3d9c_clearshader = 0;
static IDirect3DVertexShader9 *dx_d3d9c_clearshader_vs = 0;

extern bool dx_d3d9c_effectsshader_enable = false; //dx.d3d9X.cpp
extern bool dx_d3d9c_effectsshader_toggle = false;
extern void dx_d3d9X_enableeffectsshader(bool enable);

static int dx_d3d9c_ps_model = 2;

static void dx_d3d9c_prepshaders()
{	
	//reminder: things break if this is
	//not done, even when DDRAW::shader is 0???
	if(DDRAW::shader=='ff') return;

	//2021: this was never really used for playing
	//games. I think maybe it's used if there's no
	//shader set via DDRAW::ps in order to prevent
	//fixed function behavior

#define HLSL_DEFINE(D) "\n#define " #D "\n"
#define HLSL_IFDEF(D)  "\n#ifdef " #D "\n"
#define HLSL_IFNDEF(D) "\n#ifndef " #D "\n"
#define HLSL_ELSE	   "\n#else\n"
#define HLSL_ENDIF	   "\n#endif\n"

	char *ps_x_0 = "ps_2_0";

	if(dx_d3d9c_ps_model==3) ps_x_0 = "ps_3_0";
		
	HRESULT ok = !D3D_OK;

	DWORD psflags =
	D3DXSHADER_AVOID_FLOW_CONTROL
	|D3DXSHADER_OPTIMIZATION_LEVEL3
	//|D3DXSHADER_USE_LEGACY_D3DX9_31_DLL;
	|D3DXSHADER_IEEE_STRICTNESS; //dither

	LPD3DXBUFFER out = 0, err = 0;

	//COLORKEY SHADER//////////////////////////////////////////

	const char *ps =  
	
	HLSL_DEFINE(DBRIGHTNESS)

	"float2 dimViewport : register(c1);"	
	"float2 colCorrect  : register(c7);"
//	"float4 fogFactors  : register(c24);"
//	"float3 fogColor    : register(c25);"

	"sampler2D sam0:register(s0);"

	HLSL_IFDEF(DDITHER)
//	"sampler2D sam2:register(s2);"
	HLSL_ENDIF

	"struct PS_OUTPUT{ float4 col:COLOR0; };"

	"struct PS_INPUT"
	"{"
	"	float4 col:COLOR0; "
	"	float2 uv0:TEXCOORD0; "
	HLSL_IFDEF(DDITHER)
//	"	float3 uv1:TEXCOORD1; "
//	"	float  fog:FOG;	"
	HLSL_ENDIF
	"};" 

	"PS_OUTPUT f(PS_INPUT In)"
	"{"
	"	PS_OUTPUT Out; "
	
	"	Out.col = tex2D(sam0,In.uv0); "
					   
	HLSL_IFDEF(DCOLORKEY)
	"	clip(Out.col.a-0.3f); "
	"	Out.col.rgb/=Out.col.a; "	
	"	Out.col.a = 1.0f; "
	"   if(In.col.a) Out.col.a = In.col.a; " //hack
	HLSL_ELSE
	"   Out.col.a = In.col.a; "
	HLSL_ENDIF

//	"	Out.col.a = 1.0f; "
	"   Out.col.rgb*=In.col.rgb; "
//	"   if(In.col.a) Out.col.a = In.col.a; " //hack

	HLSL_IFDEF(DBRIGHTNESS)
	"	Out.col.rgb	= saturate(Out.col.rgb); "
	"	float3 inv = float3(1,1,1)-Out.col.rgb; "
	"	Out.col.rgb+=Out.col.rgb*colCorrect.y; "
	"	Out.col.rgb+=inv*colCorrect.x; "
	HLSL_ENDIF

	HLSL_IFDEF(DDITHER)							
//	failed attempt to do fog before dithering
//	"	float far = fogFactors.y; " //hmm??
//	"	float fog = 1.0f- " //fogFactors.w-fogFactors.w* "
//	"		saturate((fogFactors.y-In.uv1.z*far)/  "
//	"		     (fogFactors.y-fogFactors.x)); "
//	"   Out.col.rgb = lerp(Out.col.rgb,fogColor.rgb,fog); "
		
	//"	float2 vpos = In.uv1.xy/In.uv1.z*dimViewport; "
	//"	Out.col.rgb-=tex2D(sam2,vpos/8.0f).r/8.0f; "
	//"	Out.col.rgb+=4.0f/255.0f; "
//	"	Out.col.rgb = vpos.xxx/dimViewport.xxx; "
	HLSL_ENDIF

	"	return Out; "
	"}";

	//compile default shader
	//REMINDER: related to DDRAW::ff
	{			
		D3DXMACRO d[] = 
		{
			{DDRAW::doDither?"DDITHER":"",""},
			{0,0}
		};
		if(D3DXCompileShader(ps,strlen(ps),d,0,"f",ps_x_0,psflags,&out,&err,0))
		{
			const char *debug = err?(const char*)err->GetBufferPointer():0;
			debug = debug; //breakpoint
		}
		else dx_d3d9c_df = (DWORD*)out->GetBufferPointer();
	}							 

	//compile colorkey shader
	{			
		D3DXMACRO d[] = 
		{
			{"DCOLORKEY",""},
			{DDRAW::doDither?"DDITHER":"",""},
			{0,0}
		};
		if(D3DXCompileShader(ps,strlen(ps),d,0,"f",ps_x_0,psflags,&out,&err,0))
		{
			const char *debug = err?(const char*)err->GetBufferPointer():0;
			debug = debug; //breakpoint
		}
		else dx_d3d9c_ck = (DWORD*)out->GetBufferPointer();
	}

	//EFFECTS SHADER//////////////////////////////////////////

	ps =  
	
	"sampler2D sam0:register(s0);"

	HLSL_IFDEF(DGAMMA)
	"sampler1D sam1:register(s1);"
	HLSL_ENDIF

	"struct PS_INPUT{ float2 uv0:TEXCOORD0; };" // float2 pos:VPOS;

	"struct PS_OUTPUT{ float4 col:COLOR0; };"

	"PS_OUTPUT f(PS_INPUT In)"
	"{"
	"	PS_OUTPUT Out; " // Out.col = float4(1,0,0,1); "

	"	Out.col = tex2D(sam0,In.uv0); "

	HLSL_IFDEF(DGAMMA)
	"	Out.col.r = tex1D(sam1,Out.col.r).r; "
	"	Out.col.g = tex1D(sam1,Out.col.g).r; "
	"	Out.col.b = tex1D(sam1,Out.col.b).r; "
	HLSL_ENDIF

	/*HLSL_IFNDEF(DBLACK)
	//"   float3 ntsc = float3(8.0f,8.0f,8.0f)/255.0f; "
	//"	Out.col.rgb = max(Out.col.rgb,ntsc); "
	HLSL_ENDIF*/

	"	return Out; "
	"}";

	//compile fx shader
	{
		D3DXMACRO d[] = 
		{
			{DDRAW::doGamma?"DGAMMA":"",""},
		//	{DDRAW::doBlack?"DBLACK":"",""},
			{0,0}
		};
		if(D3DXCompileShader(ps,strlen(ps),d,0,"f",ps_x_0,psflags,&out,&err,0))
		{
			const char *debug = err?(const char*)err->GetBufferPointer():0;
			debug = debug; //breakpoint
		}
		else dx_d3d9c_fx = (DWORD*)out->GetBufferPointer();
	}

	//CLEAR SHADER/////////////////////////////////
	ps =  	
	"struct PS_INPUT{ float4 col:COLOR0; };"
	"struct PS_OUTPUT{ float4 col[4]:COLOR0; };"
	"PS_OUTPUT f(PS_INPUT In)"
	"{"
	"	PS_OUTPUT Out; " 
	"	Out.col[0] = float4(DCOL0); "
	"	Out.col[1] = float4(DCOL1); "
	"	Out.col[2] = float4(DCOL2); "
	"	Out.col[3] = float4(DCOL3); "
	"	return Out; "			
	"}"
	"struct VS_IO{ float4 pos:POSITION,col:COLOR; }; "
	"VS_IO f_vs(VS_IO In){ return In; }";

	//compile clear shader(s)
	if(1==DDRAW::doClearMRT) //2021 (ignore 0 and 2 and initialize)
	{
		D3DXMACRO d[] = 
		{
			{"DCOL0",DDRAW::ClearMRT[0]?DDRAW::ClearMRT[0]:"In.col"},
			{"DCOL1",DDRAW::ClearMRT[1]?DDRAW::ClearMRT[1]:"In.col"},
			{"DCOL2",DDRAW::ClearMRT[2]?DDRAW::ClearMRT[2]:"In.col"},
			{"DCOL3",DDRAW::ClearMRT[3]?DDRAW::ClearMRT[3]:"In.col"},
			{0,0}
		};
		if(D3DXCompileShader(ps,strlen(ps),d,0,"f",ps_x_0,psflags,&out,&err,0))
		{
			const char *debug = err?(const char*)err->GetBufferPointer():0;
			debug = debug; //breakpoint
		}
		else dx_d3d9c_cs[0] = (DWORD*)out->GetBufferPointer();
		if(D3DXCompileShader(ps,strlen(ps),d,0,"f_vs","vs_2_0",0,&out,&err,0))
		{
			const char *debug = err?(const char*)err->GetBufferPointer():0;	 
			debug = debug; //breakpoint
		}
		else dx_d3d9c_cs[1] = (DWORD*)out->GetBufferPointer();
	}
}

//NOTE: D3DMATRIX has a float* conversion operator, DX::D3DMATRIX does not
extern void dx_d3d9X_vsconstant(int reg, const D3DMATRIX &set, int mode=4);
extern void dx_d3d9X_vsconstant(int reg, const DX::D3DCOLORVALUE &set, int mode=1);
extern void dx_d3d9X_vsconstant(int reg, const DWORD &set, int mode);
extern void dx_d3d9X_vsconstant(int reg, const float &set, int mode);
extern void dx_d3d9X_vsconstant(int reg, const float *set, int mode=1);
extern void dx_d3d9X_psconstant(int reg, const DWORD &set, int mode);
extern void dx_d3d9X_psconstant(int reg, const float &set, int mode);
extern void dx_d3d9X_psconstant(int reg, const float *set, int mode=1);

D3DXMATRIX dx_d3d9c_worldviewproj; //extern
extern void dx_d3d9c_vstransform(int dirty=0) //Ex.output.cpp
{
	if(DDRAW::ff) return;

	static int dirtied = ~0; 	
	if(dirty){ dirtied|=dirty; return; } if(!dirtied) return;		
	IDirect3DDevice9 *d3dd9 = DDRAW::Direct3DDevice7->proxy9;

	static D3DXMATRIX world, view, proj; 
	static bool needworld = true, needview = true, needproj = true;
	if(dirtied&(1<<DX::D3DTRANSFORMSTATE_WORLD)) needworld = true;
	if(dirtied&(1<<DX::D3DTRANSFORMSTATE_VIEW))  needview = true;
	if(dirtied&(1<<DX::D3DTRANSFORMSTATE_PROJECTION)) needproj = true;
							   
//NOTE: can relax these requirements if necessary
#define _SANITY_CHECK1(m) assert(m[1]==m[0]+1&&m[2]==m[1]+1&&m[3]==m[2]+1);
#define _SANITY_CHECK2(m) assert(!DDRAW::vsI||m[1]<DDRAW::vsI&&!DDRAW::vsB||m[1]<DDRAW::vsB);
#define _SANITY_CHECK(m) _SANITY_CHECK1(m) _SANITY_CHECK2(m)

	if(needworld||needview||needproj)
	if(DDRAW::vsWorldViewProj[0]||DDRAW::vsWorldView[0]||DDRAW::vsInvWorldView[0])
	{
		if(needworld) //DUPLICATE
		{
			needworld = false;

			d3dd9->GetTransform(D3DTS_WORLD,&world);

			/*REMOVE ME
			if(DDRAW::gl) //TESTING
			{
				if(0||!DX::debug) 
				{
				}
				else D3DXMatrixIdentity(&world);
			}*/
		}

		if(needview) //DUPLICATE
		{
			needview = false;

			d3dd9->GetTransform(D3DTS_VIEW,&view); 

			/*REMOVE ME
			if(DDRAW::gl) //TESTING
			{
				if(0||!DX::debug) 
				{
				}
				else D3DXMatrixIdentity(&view);
			}*/
		}

		D3DXMATRIX worldview = world*view; 
		if(dirtied&(1<<DX::D3DTRANSFORMSTATE_WORLD|1<<DX::D3DTRANSFORMSTATE_VIEW))
		{
			if(DDRAW::vsWorldView[0])		
			{
				_SANITY_CHECK(DDRAW::vsWorldView)
			
				//d3dd9->SetVertexShaderConstantF(DDRAW::vsWorldView[0]-DDRAW::vsF,(float*)&worldview,4); 
				dx_d3d9X_vsconstant(DDRAW::vsWorldView[0],worldview);
			}
			if(DDRAW::vsInvWorldView[0])
			{
				_SANITY_CHECK(DDRAW::vsInvWorldView)
		
				D3DXMATRIX invworldview;
				if(!D3DXMatrixInverse(&invworldview,0,&worldview))
				{
					//todo: warning (without flooding)
				}
				//d3dd9->SetVertexShaderConstantF(DDRAW::vsInvWorldView[0]-DDRAW::vsF,(float*)&invworldview,4); 
				dx_d3d9X_vsconstant(DDRAW::vsInvWorldView[0],invworldview);
			}
		}
		if(DDRAW::vsWorldViewProj[0])
		{
			_SANITY_CHECK(DDRAW::vsWorldViewProj)
	
			if(needproj) //DUPLICATE
			{
				needproj = false;

				d3dd9->GetTransform(D3DTS_PROJECTION,&proj);

				//DUPLICATE
				/*thinking do this in the shader instead for now
				if(DDRAW::gl)
				{
					if(0||!DX::debug) 
					{
						//assuming perspective matrix
						//basing on Somvector::matrix::frustum
						proj._33*=-2; proj._33-=1;
						proj._43*=-2; proj._43-=1;
						//SOM has 1 here, OpenGL is -1, I think
						//it may need to change as well. I can't
						//find a D3D reference ATM
						assert(proj._34==1);
						proj._34 = -1;
					}
					else D3DXMatrixIdentity(&proj);

					//TESTING: invert Y axis???
				//	proj._22 = -proj._22;
				}*/
			}

			//D3DXMATRIX worldviewproj = worldview*proj; 
			dx_d3d9c_worldviewproj = worldview*proj; 
			//d3dd9->SetVertexShaderConstantF(DDRAW::vsWorldViewProj[0]-DDRAW::vsF,(float*)&dx_d3d9c_worldviewproj,4); 
			dx_d3d9X_vsconstant(DDRAW::vsWorldViewProj[0],dx_d3d9c_worldviewproj);
		}
	}
	if(DDRAW::vsWorld[0]&&dirtied&(1<<DX::D3DTRANSFORMSTATE_WORLD))
	{
		_SANITY_CHECK(DDRAW::vsWorld)

		if(needworld) //DUPLICATE 
		{
			needworld = false;

			d3dd9->GetTransform(D3DTS_WORLD,&world);
		}
		
		//d3dd9->SetVertexShaderConstantF(DDRAW::vsWorld[0]-DDRAW::vsF,(float*)&world,4); 
		dx_d3d9X_vsconstant(DDRAW::vsWorld[0],world);
	}
	if(dirtied&(1<<DX::D3DTRANSFORMSTATE_VIEW))
	{
		if(DDRAW::vsView[0])
		{
			_SANITY_CHECK(DDRAW::vsView)

			if(needview) //DUPLICATE
			{
				needview = false;

				d3dd9->GetTransform(D3DTS_VIEW,&view); 
			}
		
			//d3dd9->SetVertexShaderConstantF(DDRAW::vsView[0]-DDRAW::vsF,(float*)&view,4); 
			dx_d3d9X_vsconstant(DDRAW::vsView[0],view);
		}
		if(DDRAW::vsInvView[0]) //Note: could avoid doing after every world change
		{
			_SANITY_CHECK(DDRAW::vsInvView)

			if(needview) //DUPLICATE
			{
				needview = false;

				d3dd9->GetTransform(D3DTS_VIEW,&view); 
			}

			D3DXMATRIX invview; if(!D3DXMatrixInverse(&invview,0,&view))
			{
				assert(0); //todo: warning (without flooding)
			}

			//d3dd9->SetVertexShaderConstantF(DDRAW::vsInvView[0]-DDRAW::vsF,(float*)&invview,4); 
			dx_d3d9X_vsconstant(DDRAW::vsInvView[0],invview);
		}
	}
	if(DDRAW::vsProj[0]&&dirtied&(1<<DX::D3DTRANSFORMSTATE_PROJECTION))
	{
		_SANITY_CHECK(DDRAW::vsProj)

		if(needproj) //DUPLICATE
		{
			needproj = false;

			d3dd9->GetTransform(D3DTS_PROJECTION,&proj);

			//DUPLICATE
			/*thinking do this in the shader instead for now
			if(DDRAW::gl)
			{
				//assuming perspective matrix
				//basing on Somvector::matrix::frustum
				proj._33*=-2; proj._33-=1;
				proj._43*=-2; proj._43-=1;
				//SOM has 1 here, OpenGL is -1, I think
				//it may need to change as well. I can't
				//find a D3D reference ATM
				assert(proj._34==1);
				proj._34 = -1;
			}*/
		}
		
		//d3dd9->SetVertexShaderConstantF(DDRAW::vsProj[0]-DDRAW::vsF,(float*)&proj,4); 
		dx_d3d9X_vsconstant(DDRAW::vsProj[0],proj);
	}

#undef _SANITY_CHECK1
#undef _SANITY_CHECK2
#undef _SANITY_CHECK

	dirtied = 0;	
}

extern void dx_d3d9c_vslight(bool clean=true)
{
	if(DDRAW::ff) return;

	static bool dirty = true;

	if(!clean)
	{
		dirty = true; return;
	}
	else if(!dirty) return;
															  
	IDirect3DDevice9 *d3dd9 = DDRAW::Direct3DDevice7->proxy9;

	int n = max(DDRAW::lights,DDRAW::minlights);

	for(int i=0;i<n;i++) 
	if(DDRAW::vsLightsAmbient[i]||DDRAW::vsLightsDiffuse[i])
	{
		DX::D3DLIGHT7 lite;
				
		if(i>=DDRAW::lights)
		{
			memset(&lite,0x00,sizeof(lite)); //blank light

			lite.dltType = DX::D3DLIGHT_DIRECTIONAL; //one or the other
		}
		else 
		{
			DX::D3DLIGHT7 *p = 0; 
			if(DDRAW::light)			
			p = DDRAW::light(DDRAW::lighting[i],0);
			if(!p&&d3dd9->GetLight(DDRAW::lighting[i],(D3DLIGHT9*)&lite)!=D3D_OK) 
			continue;
			if(p) lite = *p;
		}

		if(lite.dltType==D3DLIGHT_SPOT)
		{
			assert(0); continue; //unsupported
		}

		if(DDRAW::vsLightsAmbient[i])
		{
			lite.dcvAmbient.a = i; 
			//d3dd9->SetVertexShaderConstantF(DDRAW::vsLightsAmbient[i]-DDRAW::vsF,(float*)&lite.Ambient,1);
			dx_d3d9X_vsconstant(DDRAW::vsLightsAmbient[i],lite.dcvAmbient);
		}
		if(DDRAW::vsLightsDiffuse[i])
		{
			lite.dcvDiffuse.a = i; //???	
			//d3dd9->SetVertexShaderConstantF(DDRAW::vsLightsDiffuse[i]-DDRAW::vsF,(float*)&lite.c,1);
			dx_d3d9X_vsconstant(DDRAW::vsLightsDiffuse[i],lite.dcvDiffuse);
		}
		if(DDRAW::vsLightsVectors[i])
		{
			switch(lite.dltType)
			{
			case D3DLIGHT_POINT:	
			{
				float temp[4] = {lite.dvPosition.x,lite.dvPosition.y,lite.dvPosition.z,1.0};					
				//d3dd9->SetVertexShaderConstantF(DDRAW::vsLightsVectors[i]-DDRAW::vsF,temp,1);
				dx_d3d9X_vsconstant(DDRAW::vsLightsVectors[i],temp);
				break;
			}
			case D3DLIGHT_DIRECTIONAL: 
			{
				float temp[4] = {lite.dvDirection.x,lite.dvDirection.y,lite.dvDirection.z,0.0};					
				//d3dd9->SetVertexShaderConstantF(DDRAW::vsLightsVectors[i]-DDRAW::vsF,temp,1);
				dx_d3d9X_vsconstant(DDRAW::vsLightsVectors[i],temp);
				break;
			}
			default: assert(0);
			}	
		}
		if(DDRAW::vsLightsFactors[i])
		{
			switch(lite.dltType)
			{
			case D3DLIGHT_POINT:	
			{
				float temp[4] = {lite.dvRange,lite.dvAttenuation0,lite.dvAttenuation1,lite.dvAttenuation2};					
				//d3dd9->SetVertexShaderConstantF(DDRAW::vsLightsFactors[i]-DDRAW::vsF,temp,1);
				dx_d3d9X_vsconstant(DDRAW::vsLightsFactors[i],temp);
				break;
			}
			case D3DLIGHT_DIRECTIONAL: 
			{
				float temp[4] = {lite.dvRange,1.0,0.0,0.0};					
				//d3dd9->SetVertexShaderConstantF(DDRAW::vsLightsFactors[i]-DDRAW::vsF,temp,1);
				dx_d3d9X_vsconstant(DDRAW::vsLightsFactors[i],temp);
				break;
			}
			default: assert(0);
			}	
		}
	}

	if(DDRAW::vsLights)
	{
		//WTH: 
		//this is virtually completely undocumented
		//but these registers map to vs-loop/vs-rep under ps3.0 and,
		//x is the loop count
		//y is the initial value
		//z is the amount to increment by
		//[loop] for(int i=0;i<N;i++){} is the syntax in HLSL 
		//(note: this is all gibberish) 
		int temp[4] = {min(DDRAW::lights,DDRAW::maxlights),0,1,0};
		//d3dd9->SetVertexShaderConstantI(DDRAW::vsLights-DDRAW::vsI,temp,1);
		DDRAW::vset9(temp,1,DDRAW::vsLights);
	}

	dirty = false;
}

extern void dx_d3d9c_vstransformandlight()
{
	dx_d3d9c_vstransform(); dx_d3d9c_vslight();
}

static IDirect3DTexture9 *dx_d3d9c_brights[9] = {0,0,0,0,0,0,0,0,0}; 
static IDirect3DTexture9 *dx_d3d9c_badbright = (IDirect3DTexture9*)1;

static int dx_d3d9c_brightsonoff(int on, ::IDirect3DPixelShader9 *ps) 
{
	int clamp = max(min(DDRAW::bright,8),0);

	int lv = !ps&&DDRAW::isLit?DDRAW::brights[clamp]:0;

	if(on!=lv) on = lv; else return on; 

	::IDirect3DDevice9 *d3dd9 = DDRAW::Direct3DDevice7->proxy9;	

	if(on)
	{
		if(!dx_d3d9c_brights[clamp])
		{		
			IDirect3DTexture9 *p = 0; 

			if(d3dd9->CreateTexture
			(1,1,1,D3DUSAGE_DYNAMIC,D3DFMT_A8,D3DPOOL_DEFAULT,&p,0)==D3D_OK)
			{
				D3DLOCKED_RECT lock; 
				
				if(p->LockRect(0,&lock,0,0)==D3D_OK)
				{
					*(BYTE*)lock.pBits = lv&0xFF; p->UnlockRect(0);
				}
				else 
				{
					p->Release(); p = dx_d3d9c_badbright;
				}
			}
			else p = dx_d3d9c_badbright;

			dx_d3d9c_brights[clamp] = p;
		}

		if(dx_d3d9c_brights[clamp]==dx_d3d9c_badbright) return on;
		
		d3dd9->SetTexture(DDRAW_TEX0,dx_d3d9c_brights[clamp]);

		d3dd9->SetTextureStageState(DDRAW_TEX0,D3DTSS_COLOROP,D3DTOP_ADDSMOOTH);
		d3dd9->SetTextureStageState(DDRAW_TEX0,D3DTSS_COLORARG1,D3DTA_CURRENT);
		d3dd9->SetTextureStageState(DDRAW_TEX0,D3DTSS_COLORARG2,D3DTA_TEXTURE);
		d3dd9->SetTextureStageState(DDRAW_TEX0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);
		d3dd9->SetTextureStageState(DDRAW_TEX0,D3DTSS_ALPHAARG1,D3DTA_CURRENT);
	}
	else
	{
		d3dd9->SetTextureStageState(DDRAW_TEX0,D3DTSS_COLOROP,D3DTOP_DISABLE);
		d3dd9->SetTextureStageState(DDRAW_TEX0,D3DTSS_ALPHAOP,D3DTOP_DISABLE);

		d3dd9->SetTexture(DDRAW_TEX0,DDRAW::Direct3DDevice7->query9->black);
	}

	return on;
}

static const DWORD dx_d3d9c_dimmer = 256; //black light
static int dx_d3d9c_dimmingonoff(int on, ::IDirect3DPixelShader9 *ps)
{
	int clamp = max(min(DDRAW::dimmer,8),0);

	int lv = !ps&&DDRAW::isLit?DDRAW::dimmers[clamp]:0;

	if(on!=lv) on = lv; else return on; 

	::IDirect3DDevice9 *d3dd9 = DDRAW::Direct3DDevice7->proxy9;	
	
	if(on)
	{
		::D3DLIGHT9 lit; memset(&lit,0x00,sizeof(lit));

		lit.Ambient.r = lit.Ambient.g = lit.Ambient.b = -float(lv)/256.0f;

		lit.Type = ::D3DLIGHT_POINT; lit.Range = FLT_MAX;

		lit.Attenuation0 = 1.0f; //no attenuation

		d3dd9->SetLight(dx_d3d9c_dimmer,&lit);

		d3dd9->LightEnable(dx_d3d9c_dimmer,1);
	}
	else d3dd9->LightEnable(dx_d3d9c_dimmer,0);

	return on;
}

static int dx_d3d9c_level_constants[2] = {}; //extern
static bool dx_d3d9c_level2(float *c, IDirect3DPixelShader9 *ps, int force)
{
	auto &constants = dx_d3d9c_level_constants; 

	bool islit = DDRAW::isLit;

	if(DDRAW::doInvert&&!DDRAW::shader) 
	{
		if(ps&&ps==dx_d3d9c_pshaders[DDRAW::fx])
		{
			islit = true; //hack: reverse inversion in effects shader
		}
		else islit = !islit;
	}

	int clampb = max(min(DDRAW::bright,8+8),0);
	int clampd = max(min(DDRAW::dimmer,8+8),0);

	if(force||ps) if(force
	||constants[0]!=(islit?DDRAW::brights[clampb]:0)
	||constants[1]!=(islit?DDRAW::dimmers[clampd]:0))
	{
		int b = islit?DDRAW::brights[clampb]:0;
		int d = islit?DDRAW::dimmers[clampd]:0;

		if(force!=2)
		{
			constants[0] = b; constants[1] = d;
		}

		c[0] = b/255.0f; c[1] = -d/255.0f;
		
		return true;
	}
	return false;
}
extern void DDRAW::fxCorrectionsXY(float c[2]) //HACK
{
	auto *ps = dx_d3d9c_pshaders[DDRAW::fx]; 	
	if(ps&&DDRAW::doInvert&&!DDRAW::shader)
	{
		if(dx_d3d9c_level2(c,ps,2)) return;
	}
	c[0] = c[1] = 0.0f;
}
extern void dx_d3d9c_level(IDirect3DPixelShader9 *ps, bool reset=false)
{
	if(!DDRAW::shader)
	{
		float c[4];
		if(dx_d3d9c_level2(c,ps,reset))
		{
			c[2] = c[3] = 0.0f;
			dx_d3d9X_psconstant(DDRAW::psCorrections,c);
		}
		return;
	}

	int clampb = max(min(DDRAW::bright,8+8),0);
	int clampd = max(min(DDRAW::dimmer,8+8),0);

	//brights 
	{
		static int on = 0;

		//2018: isn't this already clamped/to 16?
		//if(clampb>8) clampb = 8;

		if(!reset)
		{
			static bool was_ever_on = false;

			if(was_ever_on)
			{
				on = dx_d3d9c_brightsonoff(on,ps); 
			}
			else if(!ps&&DDRAW::brights[clampb])
			{
				on = dx_d3d9c_brightsonoff(on,ps); 

				was_ever_on = true;
			}
		}
		else on = 0;
	}
	//dimmers 
	{
		static int on = 0;

		//2018: isn't this already clamped/to 16?
		//if(clampd>8) clampd = 8;

		if(!reset)
		{
			static bool was_ever_on = false;

			if(was_ever_on)
			{
				on = dx_d3d9c_dimmingonoff(on,ps); 
			}
			else if(!ps&&DDRAW::dimmers[clampd])
			{
				on = dx_d3d9c_dimmingonoff(on,ps); 

				was_ever_on = true;
			}
		}
		else on = 0;
	}
}

extern const int dx_d3d9c_formats_enumN = 8;
extern const D3DFORMAT dx_d3d9c_formats_enum[dx_d3d9c_formats_enumN] = 
{
	//REMINDER: texture_format_mask EXPECTS THIS ORDER (dx.d3d9X.cpp)
	D3DFMT_A8R8G8B8, D3DFMT_X8R8G8B8, D3DFMT_A1R5G5B5, D3DFMT_X1R5G5B5, D3DFMT_R5G6B5, D3DFMT_P8,
	D3DFMT_A2R10G10B10, D3DFMT_A8
};
static const D3DFORMAT dx_d3d9c_bogus_format = D3DFMT_X8R8G8B8;

const DX::DDPIXELFORMAT &dx_d3d9c_format(D3DFORMAT f) //dx.d3d9X.cpp
{
	static DX::DDPIXELFORMAT ddpfs[dx_d3d9c_formats_enumN] = 
	{
		{	sizeof(DX::DDPIXELFORMAT), //A8R8G8B8
			DDPF_ALPHAPIXELS|DDPF_RGB,0,32,
			0x00ff0000,0x0000ff00,0x000000ff,
			0xFF000000
		},
		{	sizeof(DX::DDPIXELFORMAT), //X8R8G8B8
			DDPF_RGB,0,32,
			0x00ff0000,0x0000ff00,0x000000ff,
			0x00000000
		},
		{	sizeof(DX::DDPIXELFORMAT), //A1R5G5B5
			DDPF_ALPHAPIXELS|DDPF_RGB,0,16,
			0x07c00,0x03e0,0x001f,
			0x8000
		},
		{	sizeof(DX::DDPIXELFORMAT), //X1R5G5B5
			DDPF_RGB,0,16,
			0x7c00,0x03e0,0x001f,
			0x0000
		},
		{	sizeof(DX::DDPIXELFORMAT), //R5G6B5
			DDPF_RGB,0,16,
			0xf800,0x07e0,0x001f, 
			0x0000
		},
		{	sizeof(DX::DDPIXELFORMAT), //P8
			DDPF_PALETTEINDEXED8|DDPF_RGB,0,8,
			0x00,0x00,0x00,
			0x00
		},
		{	sizeof(DX::DDPIXELFORMAT), //D3DFMT_A2R10G10B10
			DDPF_ALPHAPIXELS|DDPF_RGB,0,32,
			0x3FF00000,0x000FFC00,0x000003FF,
			0xC0000000
		},
		{	sizeof(DX::DDPIXELFORMAT), //D3DFMT_A8 (GL_R8)
			DDPF_ALPHA,0,8,
			0x00,0x00,0x00,
			0xff
		},
	};

	for(int i=0;i<dx_d3d9c_formats_enumN;i++) if(f==dx_d3d9c_formats_enum[i]) 
	{
		return ddpfs[i];
	}

	assert(0); //unimplemented
	
	return dx_d3d9c_format(dx_d3d9c_bogus_format); 
}
D3DFORMAT dx_d3d9c_format(DX::DDPIXELFORMAT &f) //dx.d3d9X.cpp
{
	if(f.dwRGBBitCount==8)
	{
		return f.dwFlags&DDPF_PALETTEINDEXED8?D3DFMT_P8:D3DFMT_A8;
	}
	else if(f.dwFlags&DDPF_ALPHAPIXELS)
	{
		return f.dwRGBBitCount==16?D3DFMT_A1R5G5B5:D3DFMT_A8R8G8B8; 
	}
	else return f.dwRGBBitCount==16?D3DFMT_X1R5G5B5:D3DFMT_X8R8G8B8; 

	assert(0); //unimplemented
	
	return dx_d3d9c_bogus_format;
}

extern int DDRAW::d3d9cNumSimultaneousRTs = 1;

//dx_d3d9c_d3ddrivercaps: does not set dwDeviceZBufferBitDepth
extern DX::D3DDEVICEDESC7 &dx_d3d9c_d3ddrivercaps(D3DCAPS9 &in, DX::D3DDEVICEDESC7 &out)
{
	DDRAW::d3d9cNumSimultaneousRTs = in.NumSimultaneousRTs;

	//SEEING IF MY CARDS EVER SUPPORT MRT BLENDING?
	//2021: I thought my card didn't do this but I wrote && by accident
	//still it doesn't seem to work... maybe because the MRT format isn't
	//RGB or is different (it's R32 I think)
	//assert(in.PrimitiveMiscCaps&D3DPMISCCAPS_MRTPOSTPIXELSHADERBLENDING);
	//this does blending in linear space (sRGB) (always I guess???)
	//OR DOES IT? it's called POST and not PRE??? maybe it only applies to
	//an sRGB target?? I have a feeling it only applies to D3DRS_SRGBWRITEENABLE
	//and I doubt if linear sRGB can fit into an 8-bit buffer without problems?
	//assert(in.PrimitiveMiscCaps&D3DPMISCCAPS_POSTBLENDSRGBCONVERT);

//		DWORD            dwDevCaps;              /* Capabilities of device */

	out.dwDevCaps = in.DevCaps|1; //FLOATTLVERTEX

//		D3DPRIMCAPS      dpcLineCaps;
	
	out.dpcLineCaps.dwSize = sizeof(DX::D3DPRIMCAPS);

	//note: assuming mapping is ok (may well not be)
	out.dpcLineCaps.dwMiscCaps = in.PrimitiveMiscCaps;
	out.dpcLineCaps.dwRasterCaps = in.RasterCaps;
	out.dpcLineCaps.dwZCmpCaps = in.ZCmpCaps;
	out.dpcLineCaps.dwSrcBlendCaps = in.SrcBlendCaps;

	out.dpcLineCaps.dwDestBlendCaps = in.DestBlendCaps;
	out.dpcLineCaps.dwAlphaCmpCaps = in.AlphaCmpCaps;
	out.dpcLineCaps.dwShadeCaps = in.ShadeCaps;
	out.dpcLineCaps.dwTextureCaps = in.TextureCaps;
	out.dpcLineCaps.dwTextureFilterCaps = in.TextureFilterCaps;

	//experiment to disable point filtering (failure)
	//out.dpcLineCaps.dwTextureFilterCaps&=~D3DPTFILTERCAPS_MAGFPOINT;
	//out.dpcLineCaps.dwTextureFilterCaps&=~D3DPTFILTERCAPS_MINFPOINT;
	//out.dpcLineCaps.dwTextureFilterCaps&=~D3DPTFILTERCAPS_MIPFPOINT;



	//Hack: Make "trilinear filtering" appear available DirectX 7 //

	out.dpcLineCaps.dwTextureFilterCaps|=D3DPTFILTERCAPS_MIPLINEAR;

	////////////////////////////////////////////////////////////////


	//note: there is no D3D9 analog for this
	out.dpcLineCaps.dwTextureBlendCaps = //attributing all beneficial caps

	 D3DPTBLENDCAPS_MODULATE|D3DPTBLENDCAPS_DECALALPHA
	|D3DPTBLENDCAPS_MODULATEALPHA|D3DPTBLENDCAPS_DECALMASK
	|D3DPTBLENDCAPS_MODULATEMASK|D3DPTBLENDCAPS_COPY
	|D3DPTBLENDCAPS_ADD;

	out.dpcLineCaps.dwTextureAddressCaps = in.TextureAddressCaps;

	out.dpcLineCaps.dwStippleWidth = 32;	//DirectX7 maximum
	out.dpcLineCaps.dwStippleHeight = 32;	//DirectX7 maximum

//		D3DPRIMCAPS      dpcTriCaps;

	out.dpcTriCaps = out.dpcLineCaps;

//		DWORD            dwDeviceRenderBitDepth; /* One of DDBB_8, 16, etc. */
	



//// WARNING: Sword of Moonlight OPTIONS will fail without DDBD_16 ////////

	out.dwDeviceRenderBitDepth = DDBD_32|DDBD_16; //X8R8G8B8

///////////////////////////////////////////////////////////////////////////



//		DWORD            dwDeviceZBufferBitDepth;/* One of DDBD_16, 32, etc. */

	out.dwDeviceZBufferBitDepth = 0x00000000;
	


	
	//note: DOES NOT SET dwDeviceZBufferBitDepth




//		DWORD       dwMinTextureWidth, dwMinTextureHeight;
//		DWORD       dwMaxTextureWidth, dwMaxTextureHeight;
//		DWORD       dwMaxTextureRepeat;
//		DWORD       dwMaxTextureAspectRatio;
//		DWORD       dwMaxAnisotropy;

	//note: assuming 0 is invalid		
	out.dwMinTextureWidth = out.dwMinTextureHeight = 1; 
	out.dwMaxTextureWidth = in.MaxTextureWidth;
	out.dwMaxTextureHeight = in.MaxTextureHeight;
	out.dwMaxTextureRepeat = in.MaxTextureRepeat;
	out.dwMaxTextureAspectRatio = in.MaxTextureAspectRatio;
	out.dwMaxAnisotropy = in.MaxAnisotropy;

//		D3DVALUE    dvGuardBandLeft;
//		D3DVALUE    dvGuardBandTop;
//		D3DVALUE    dvGuardBandRight;
//		D3DVALUE    dvGuardBandBottom;
//		D3DVALUE    dvExtentsAdjust;

	out.dvGuardBandLeft = in.GuardBandLeft;
	out.dvGuardBandTop = in.GuardBandTop;
	out.dvGuardBandRight = in.GuardBandRight;
	out.dvGuardBandBottom = in.GuardBandBottom;
	out.dvExtentsAdjust = in.ExtentsAdjust;	 							  

//		DWORD       dwStencilCaps;

	out.dwStencilCaps = in.StencilCaps;

//		DWORD       dwFVFCaps;

	out.dwFVFCaps = in.FVFCaps;

//		DWORD       dwTextureOpCaps;

	out.dwTextureOpCaps = in.TextureOpCaps;

//		WORD        wMaxTextureBlendStages;
//		WORD        wMaxSimultaneousTextures;
//		DWORD       dwMaxActiveLights;

	out.wMaxTextureBlendStages = in.MaxTextureBlendStages;
	out.wMaxSimultaneousTextures = in.MaxSimultaneousTextures;
	out.dwMaxActiveLights = in.MaxActiveLights;

//		D3DVALUE    dvMaxVertexW;

	out.dvMaxVertexW = in.MaxVertexW;

//		GUID        deviceGUID;

	out.deviceGUID = in.DeviceType==D3DDEVTYPE_HAL?
		DX::IID_IDirect3DTnLHalDevice: //warning: assuming supported
		DX::IID_IDirect3DRGBDevice;

//		WORD        wMaxUserClipPlanes;
//		WORD        wMaxVertexBlendMatrices;

	out.wMaxUserClipPlanes = in.MaxUserClipPlanes;
	out.wMaxVertexBlendMatrices = in.MaxVertexBlendMatrices;

// 		DWORD       dwVertexProcessingCaps;

	out.dwVertexProcessingCaps = in.VertexProcessingCaps;

	return out;
}
 
//TODO? use D3DXGetFVFVertexSize instead
extern UINT dx_d3d9c_sizeofFVF(DWORD in) //dx.ddraw.cpp
{	
	switch(in) //NOTE: with stereo this isn't helpful
	{
	case D3DFVF_VERTEX:	  return sizeof(DX::D3DVERTEX);
	case D3DFVF_TLVERTEX&~0xE|D3DFVF_XYZW: //OPTIMIZING
	case D3DFVF_TLVERTEX: return sizeof(DX::D3DTLVERTEX);
	case D3DFVF_LVERTEX:  return sizeof(DX::D3DLVERTEX);
	}

	UINT out = 0; 
		
	switch(in&D3DFVF_POSITION_MASK)	
	{
	case D3DFVF_XYZ: out+=sizeof(DX::D3DVALUE)*3; break;
	case D3DFVF_XYZW: //0x400E
	case D3DFVF_XYZRHW: out+=sizeof(DX::D3DVALUE)*4; break;
	case D3DFVF_XYZB1: out+=sizeof(DX::D3DVALUE)*4; break;
	case D3DFVF_XYZB2: out+=sizeof(DX::D3DVALUE)*5; break;
	case D3DFVF_XYZB3: out+=sizeof(DX::D3DVALUE)*6; break;
	case D3DFVF_XYZB4: out+=sizeof(DX::D3DVALUE)*7; break;
	case D3DFVF_XYZB5: out+=sizeof(DX::D3DVALUE)*8; break;
	}

	//REMINDER: D3DFVF_LVERTEX actually includes this
	//it's also equal to PSIZE
	//REMINDER: PSIZE is used by dx_d3d9c_stereoVD, I
	//guess there's no conflict?
	if(in&D3DFVF_RESERVED1) out+=sizeof(DWORD);

	if(in&D3DFVF_NORMAL) out+=sizeof(DX::D3DVALUE)*3;
	if(in&D3DFVF_DIFFUSE) out+=sizeof(DX::D3DCOLOR);
	if(in&D3DFVF_SPECULAR) out+=sizeof(DX::D3DCOLOR);

	int n = (in&D3DFVF_TEXCOUNT_MASK)>>D3DFVF_TEXCOUNT_SHIFT;
	for(int i=0;i<n;i++) 
	{
		//1/1/1/1: a two bit mask is used, and 1 covers both bits
		if((in&D3DFVF_TEXCOORDSIZE1(i))==D3DFVF_TEXCOORDSIZE2(i))
		out+=sizeof(DX::D3DVALUE)*2; else
		if((in&D3DFVF_TEXCOORDSIZE1(i))==D3DFVF_TEXCOORDSIZE1(i))
		out+=sizeof(DX::D3DVALUE)*1; else
		if((in&D3DFVF_TEXCOORDSIZE1(i))==D3DFVF_TEXCOORDSIZE3(i))
		out+=sizeof(DX::D3DVALUE)*3; else
		if((in&D3DFVF_TEXCOORDSIZE1(i))==D3DFVF_TEXCOORDSIZE4(i))
		out+=sizeof(DX::D3DVALUE)*4; else
		assert(0);
	}

	return out;
}
extern int dx_d3d9c_ibuffer_i = -1;
extern int dx_d3d9c_vbuffer_i = -1;
extern HRESULT dx_d3d9c_ibuffer(IDirect3DDevice9 *d, IDirect3DIndexBuffer9* &ib, DWORD r, LPWORD q)
{
	HRESULT out = 0; assert(!dx_d3d9c_immediate_mode);

	int &i = dx_d3d9c_ibuffer_i;

	if(r==0) 
	{
		//I think this is the source of crashes, but fixing
		//it here with !dx_d3d9c_ibuffers[i] below doesn't
		//seem to be averting the crashes

		r = r; assert(r!=0); //breakpoint
	}

	i = ++i%D3D9C::ibuffersN;
	UINT &s = dx_d3d9c_ibuffers_s[i]; if(s<r||!dx_d3d9c_ibuffers[i])
	{						
		if(dx_d3d9c_ibuffers[i])
		{
			dx_d3d9c_ibuffers[i]->Release(); dx_d3d9c_ibuffers[i] = 0; //DEBUGGING (MPX)
		}
		s = max(s*3/2,max(r,4096));
		if(out=d->CreateIndexBuffer(2*s,D3DUSAGE_WRITEONLY|D3DUSAGE_DYNAMIC,D3DFMT_INDEX16,D3DPOOL_DEFAULT,dx_d3d9c_ibuffers+i,0))
		{
			assert(0); return out; //DEBUGGING (MPX)
		}
	}	
	ib = dx_d3d9c_ibuffers[i]; void *ibp;
	out = ib->Lock(0,r*2,&ibp,D3DLOCK_DISCARD);
	if(!out)
	{
		if(q) memcpy(ibp,q,r*2);
		//else for(WORD i=r;i-->0;) ((WORD*)ibp)[i] = i;
		else assert(0); //2021
	}
	else assert(0); //DEBUGGING (MPX)
	if(!out) out = ib->Unlock();
	assert(!out); //DEBUGGING (MPX)
	return out;
}
extern HRESULT dx_d3d9c_vbuffer(IDirect3DDevice9 *d, IDirect3DVertexBuffer9* &vb, DWORD qN, LPVOID q)
{
	HRESULT out = 0; assert(!dx_d3d9c_immediate_mode);

	int &i = dx_d3d9c_vbuffer_i;

	i = ++i%D3D9C::ibuffersN;
	UINT &s = dx_d3d9c_vbuffers_s[i]; if(s<qN||!dx_d3d9c_vbuffers[i])
	{						
		if(dx_d3d9c_vbuffers[i]) 
		dx_d3d9c_vbuffers[i]->Release();
		s = max(s*3/2,max(qN,4096));
		if(out=d->CreateVertexBuffer(s,D3DUSAGE_WRITEONLY|D3DUSAGE_DYNAMIC,0,D3DPOOL_DEFAULT,dx_d3d9c_vbuffers+i,0))
		return out;
	}	
	vb = dx_d3d9c_vbuffers[i]; void *vbp;
	out = vb->Lock(0,qN,&vbp,D3DLOCK_DISCARD);
	if(!out) memcpy(vbp,q,qN);
	if(!out) out = vb->Unlock(); return out;
}

struct dx_d3d9c_stereoVD_t dx_d3d9c_stereoVD;
void dx_d3d9c_stereoVD_t::clear_and_Release()
{
	for(map_t::iterator it=map.begin();it!=map.end();it++)
	{
		if(it->second) it->second->Release();
	}
	map.clear(); 
}
dx_d3d9c_stereoVD_t::map_t::mapped_type dx_d3d9c_stereoVD_t::operator()(DWORD fvf)
{
	std::pair<map_t::iterator,bool> ins = 
	map.insert(map_t::value_type(fvf,(map_t::mapped_type)0));
	if(ins.second) ins.first->second = _create(fvf); 
	return ins.first->second;
}
dx_d3d9c_stereoVD_t::map_t::mapped_type dx_d3d9c_stereoVD_t::_create(DWORD fvf)
{
	//EXPERIMENTAL
	//D3DFVF_XYZRHW fails CreateVertexDeclaration including stereo stream
	//this is a different value for D3D9 and 
	//whatever reason, the wrong macro is found
	//assert(0x400E==D3DFVF_POSITION_MASK);
	switch(fvf&0x400E) //D3DFVF_POSITION_MASK)
	{
	case D3DFVF_XYZRHW: //0x004
	case D3DFVF_XYZW: //0x4002

		//HACK: The fourth component must be masked with
		//something
		if(~fvf&(D3DFVF_NORMAL|D3DFVF_PSIZE))
		{
			fvf&=~0x400E; fvf|=D3DFVF_XYZ|D3DFVF_PSIZE;
		}
		else assert(0);
	}

	map_t::mapped_type out = 0;

	D3DVERTEXELEMENT9 vd[MAX_FVF_DECL_SIZE+1] = 
	{
	{1, 0,  D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_DEPTH, 0},
	D3DDECL_END()
	};		
	if(D3DXDeclaratorFromFVF(fvf,vd+1))
	{
			assert(0);
	}
	else if(DDRAW::Direct3DDevice7->proxy9->CreateVertexDeclaration(vd,&out))
	{
		assert(D3DFVF_XYZRHW==(D3DFVF_XYZRHW&fvf)); //WTH? NEEDS WORK
	}
	return out;
}

static int dx_d3d9c_paletteshint = 1;
static bool dx_d3d9c_palettes[256] = {};
extern bool dx_d3d9c_colorkeyenable = true;
extern bool dx_d3d9c_alphablendenable = false; //??? //REMOVE ME

extern IDirect3DStateBlock9 *dx_d3d9c_bltstate = 0;
extern bool dx_d3d9c_prepareblt(bool ck=false, int abe=0)
{
	auto d3dd9 = DDRAW::Direct3DDevice7->proxy9;

	if(!dx_d3d9c_bltstate) 
	{
		if(d3dd9->CreateStateBlock(D3DSBT_ALL,&dx_d3d9c_bltstate)!=D3D_OK) return false;
	}
	else if(dx_d3d9c_bltstate->Capture()!=D3D_OK) return false;

	if(ck) //dx_d3d9c_colorkeyenable
	d3dd9->SetRenderState(D3DRS_ALPHAFUNC,D3DCMP_GREATER);	
	d3dd9->SetRenderState(D3DRS_ALPHATESTENABLE,ck);

	d3dd9->SetRenderState(D3DRS_FOGENABLE,0);
	d3dd9->SetRenderState(D3DRS_ZENABLE,0);
	//2021: I think redundant with ZENABLE off
	//d3dd9->SetRenderState(D3DRS_ZWRITEENABLE,0);	
	d3dd9->SetRenderState(D3DRS_ALPHABLENDENABLE,abe); 
	if(abe) //DDRAW_PUSH_HACK extension
	{		
		d3dd9->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);		
		d3dd9->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
	}
	d3dd9->SetRenderState(D3DRS_LIGHTING,0);
	d3dd9->SetRenderState(D3DRS_SHADEMODE,D3DSHADE_FLAT);
	d3dd9->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);
	d3dd9->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);

	d3dd9->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
	d3dd9->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
	d3dd9->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);
	d3dd9->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);	
	
	d3dd9->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);
	d3dd9->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);

	return true;
}
extern HRESULT dx_d3d9c_backblt(IDirect3DTexture9 *texture, RECT &src, RECT &dst, IDirect3DPixelShader9 *ps, int vs=-1, DWORD white=0xFFFFFF)
{
	auto d3dd9 = DDRAW::Direct3DDevice7->proxy9;

	D3DSURFACE_DESC desc;

	if(texture->GetLevelDesc(0,&desc)!=D3D_OK) return !D3D_OK;

	DWORD fvf = D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX2; //D3DFVF_TEX1

	if(vs>-1&&dx_d3d9c_vshaders[vs]) 
	{
		fvf = D3DFVF_XYZW|D3DFVF_DIFFUSE|D3DFVF_TEX2; //D3DFVF_TEX1
	}

	float l = src.left, r = src.right;	
	float t = src.top, b = src.bottom, c = 0; //hack...
	
	//2018: changing this behavior since I think dx_d3d9c_effectsshader
	//is the built in function only... this could be disasterous
	//if(ps!=dx_d3d9c_effectsshader) 
	bool fx = texture==DDRAW::Direct3DDevice7->query9->effects[0];
	if(fx) //hack
	{
		DWORD filter = D3DTEXF_POINT;
		DWORD mipfilter = //D3DTEXF_NONE
		DDRAW::doMipSceneBuffers?D3DTEXF_POINT:D3DTEXF_NONE;

		if(DDRAW::inStereo)
		{
			//undistortion requires linear
			filter = D3DTEXF_LINEAR; 
			
			if(DDRAW::do2ndSceneBuffer
			 &&DDRAW::doMipSceneBuffers)
			{
				if(!DDRAW::ps2ndSceneMipmapping2)
				mipfilter = D3DTEXF_LINEAR;
			}
			else if(DDRAW::doSmooth) 
			{
				//warning: the .5 pixel offset probably
				//somewhat defeats undistortion filters
				goto smooth;
			}
		}
		else if(DDRAW::doSmooth) //sample in the middle of the pixels
		{
			filter = D3DTEXF_LINEAR; 
			
			//NOTE: I tested this with do_aa disabled... it's hard to
			//say if it's accurate otherwise. fxInflateRect had thrown
			//it off, but it somehow seemed more stable before?
			if(DDRAW::doSuperSampling)
			{
				//WARNING: I've not adjusted fxInflateRect so that this
				//samples a little bit randomly across the entire image
				//this just helps with high-contrast movements. because
				//I guess that keeps the pixels from synchronizing

				//I think this is correct because the shader is using
				//tex2dlod to sample the first mipmap between the four
				//pixels that become one pixel in the mipmap
				if(1){ l+=0.98f; t+=0.98f; r+=0.98f; b+=0.98f; }
				else{ l+=1; t+=1; r+=1; b+=1; }
			}
			else smooth:
			{
				//2020: .49 is for filter2 //???
				if(1){ l+=0.49f; t+=0.49f; r+=0.49f; b+=0.49f; }
				else{ l+=0.5f; t+=0.5f; r+=0.5f; b+=0.5f; }
			}
		}
		//works magic for DDRAW::ps2ndSceneMipmapping2 on my Intel
		//system... I guess it could help nearest neighbor broadly
		//if(1&&DDRAW::ps2ndSceneMipmapping2)
		if(filter==D3DTEXF_POINT)
		{
			//EXPERIMENTAL
			//in theory mipmap effects can benefit from linear 
			//sample by pulling in their neightborhood?
			if(DDRAW::doMipSceneBuffers)
			{
				filter = D3DTEXF_LINEAR; //EXPERIMENTAL
			}

			float x = 0.001f; l+=x; t+=x; r+=x; b+=x; //Intel
		}
		
		DWORD filter2 = filter; //EXPERIMENTAL
		
		if(DDRAW::Direct3DDevice7->query9->effects[1]) 
		{	  			
			if(DDRAW::doSharp) //EXPERIMENTAL
			{
				//CURRENTLY UNUSED
				//It turns out the existing system is plenty sharp if
				//the shader shoots the vertex over half a pixel
				//https://www.reddit.com/r/KingsField/comments/gtcp6t/more_good_news_for_sword_of_moonlight_visuals/
				//TODO: I want to try to vary l,t,r,b and use an even
				//sharper mode that puts the sharp frame in the middle

				if(0&&3==DDRAW::dejagrate)
				{
					//this way is a bit too temporal and can feel
					//weird when jumping sideways. it also makes
					//screenshots unpredictable

					//this applies point filter to the frame that was
					//sampled in the dead center of the pixel. the 
					//third frame shows two half-tones on either side
					//of the dead-center sample				
					int frame = DDRAW::noFlips%DDRAW::dejagrate;
					if(frame<2)
					{
						filter = D3DTEXF_POINT;
						//WHOOPS??? RETRY ME???
						//if(frame=1) 
						if(frame==1) //UNTESTED
						std::swap(filter,filter2);
					}
				}
				else filter = D3DTEXF_POINT; //good enough
			}
		
			//tex2D is unreliable if these are not set (WHY???)
			d3dd9->SetSamplerState(1,D3DSAMP_MIPFILTER,mipfilter);

			d3dd9->SetSamplerState(1,D3DSAMP_MINFILTER,filter2);
			d3dd9->SetSamplerState(1,D3DSAMP_MAGFILTER,filter2);
			//dx_d3d9c_prepareblt
			d3dd9->SetSamplerState(1,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);
			d3dd9->SetSamplerState(1,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);

			//no noticeable benefit (supersampling/nonstereo) when the
			//bulk of the supersampling target can be ignored for this
			//if(mml) d3dd9->SetSamplerState(1,D3DSAMP_MAXMIPLEVEL,1); //TESTING
		}
		//if(mml) d3dd9->SetSamplerState(0,D3DSAMP_MAXMIPLEVEL,1); //TESTING

		d3dd9->SetSamplerState(0,D3DSAMP_MINFILTER,filter);
		d3dd9->SetSamplerState(0,D3DSAMP_MAGFILTER,filter);
		//tex2D is unreliable if these are not set
		d3dd9->SetSamplerState(0,D3DSAMP_MIPFILTER,mipfilter);

		c = -0.5f; //black magic???
	}
	
	FLOAT blt[] = //passing in screenspace coords (tex2) as courtesy to effects shaders
	{
		c+dst.left,  c+dst.top,   0.0f,1.0f,*(float*)&white,l/desc.Width,t/desc.Height,0,0, 
		c+dst.right, c+dst.top,   0.0f,1.0f,*(float*)&white,r/desc.Width,t/desc.Height,0,0, 
		c+dst.right, c+dst.bottom,0.0f,1.0f,*(float*)&white,r/desc.Width,b/desc.Height,0,0, 
		c+dst.left,  c+dst.bottom,0.0f,1.0f,*(float*)&white,l/desc.Width,b/desc.Height,0,0, 
	};
		
	//2018: !fx doesn't work here
	//2020: restoring !fx by factoring in viewport X/Y into flip() 
	if(!DDRAW::doNothing&&!fx&&ps!=dx_d3d9c_effectsshader)
	{
		if(vs!=DDRAW::fx)
		{
			if(DDRAW::doScaling&&DDRAW::xyScaling[0]!=1) 
			{
				for(int i=0;i<4*9;i+=9) blt[i]*=DDRAW::xyScaling[0]; 	
			}		
			if(DDRAW::doScaling&&DDRAW::xyScaling[1]!=1) 
			{
				for(int i=0;i<4*9;i+=9) blt[i+1]*=DDRAW::xyScaling[1];	
			}
		}
		else assert(vs!=DDRAW::fx); //!fx should preclude this?

		if(D3DFVF_XYZRHW==(fvf&D3DFVF_POSITION_MASK))
		{
			if(DDRAW::doMapping&&DDRAW::xyMapping[0]!=0) 
			{
				for(int i=0;i<4*9;i+=9) blt[i]+=DDRAW::xyMapping[0]; 	
			}
			if(DDRAW::doMapping&&DDRAW::xyMapping[1]!=0) 
			{
				for(int i=0;i<4*9;i+=9) blt[i+1]+=DDRAW::xyMapping[1];
			}
		}

		if(DDRAW::doSuperSampling)
		{
			assert(c==0);
			for(int i=0;i<4*9;i+=9)
			DDRAW::doSuperSamplingMul(blt[i],blt[i+1]);
		}
	}
	if(D3DFVF_XYZRHW!=(fvf&D3DFVF_POSITION_MASK))
	{
		//D3DFVF_XYZW: convert to clipping space
		
		D3DVIEWPORT9 vp; 
			
		HRESULT ok = d3dd9->GetViewport(&vp); assert(ok==D3D_OK);

		for(int i=0;i<4*9;i+=9) //clip space
		{
			blt[i+7] = blt[i]; blt[i+8] = blt[i+1]; //tex2

			blt[i] = blt[i]/vp.Width*2-1;

			blt[i+1] = 1-blt[i+1]/vp.Height*2;
		}			
	}	

	if(dx_d3d9c_beginscene==dx_d3d9c_endscene) d3dd9->BeginScene();

	//WARNING: can't call DDRAW::updating_texture from here (2021)
	HRESULT out = d3dd9->SetTexture(0,texture); 
	assert(!out);

	if(ps!=dx_d3d9c_effectsshader) dx_d3d9c_level(ps);

	if(vs>-1) d3dd9->SetVertexShader(dx_d3d9c_vshaders[vs]);

	if(ps) d3dd9->SetPixelShader(ps);

	if(fx)
	{
		out = d3dd9->SetFVF(fvf);	
		out = d3dd9->DrawPrimitiveUP(D3DPT_TRIANGLEFAN,2,blt,9*sizeof(float));	
	}
	else 
	{
		auto d3dd = (D3D9C::IDirect3DDevice7*)DDRAW::Direct3DDevice7;
		//out = d3dd->DrawPrimitive(DX::D3DPT_TRIANGLEFAN,fvf,blt,4,0);
		out = d3dd->drawprims2(DX::D3DPT_TRIANGLEFAN,fvf,blt,4);		
	}

	if(dx_d3d9c_beginscene==dx_d3d9c_endscene) d3dd9->EndScene();

	return out;
}
extern bool dx_d3d9c_cleanupblt()
{
	bool out = true;

	if(!dx_d3d9c_bltstate||dx_d3d9c_bltstate->Apply()!=D3D_OK) out = false;

	//2021: dx_d3d9c_bltstate should include constants??
	//DDRAW::refresh_state_dependent_shader_constants();

	return out;
}

extern LONG dx_d3d9c_pitch(IDirect3DSurface9 *p)
{
	D3DLOCKED_RECT out; if(!p) return 0;

	const DWORD flags = D3DLOCK_NO_DIRTY_UPDATE|D3DLOCK_READONLY|D3DLOCK_NOSYSLOCK; //D3DLOCK_DISCARD
			
	if(p->LockRect(&out,0,flags)!=D3D_OK) 
	{
		assert(0); return 0;
	}
		
	p->UnlockRect(); return out.Pitch;
}
extern LONG dx_d3d9c_pitch(IDirect3DTexture9 *p)
{
	LONG out = 0; if(!p) return 0;

	IDirect3DSurface9 *q = 0;

	if(p->GetSurfaceLevel(0,&q)!=D3D_OK)
	{
		assert(0); return 0;
	}
	else out = dx_d3d9c_pitch(q);

	q->Release(); return out;
}

extern bool dx_d3d9c_colorfill(IDirect3DSurface9 *p, RECT *r, D3DCOLOR c)
{		
	//WARNING: no passing locked surfaces to this routine

	D3DLOCKED_RECT lock; D3DSURFACE_DESC desc; if(!p) return false;

	if(p->LockRect(&lock,r,0)!=D3D_OK) return false;	

	if(p->GetDesc(&desc)!=D3D_OK) return false;

	DX::DDPIXELFORMAT pf = dx_d3d9c_format(desc.Format);

	WORD c16 = 0x0000; BYTE *rgba = (BYTE*)&c; 

	if(pf.dwRGBBitCount==16) switch(desc.Format)
	{
	case D3DFMT_A1R5G5B5: //assuming 1bit color
		
		c16|=rgba[3]>127?0x8000:0; //FALL THRU

	case D3DFMT_X1R5G5B5: 

		c16|=rgba[2]/8<<10|rgba[1]/8<<5|rgba[0]/8; break;

	case D3DFMT_R5G6B5:

		c16 = rgba[2]/8<<11|rgba[1]/4<<5|rgba[0]/8; break;
	}
 
	int w = r?r->right-r->left:desc.Width;	
	int h = r?r->bottom-r->top:desc.Height;

	int compile[sizeof(WORD)==sizeof(wchar_t)];

	if(pf.dwRGBBitCount/8*desc.Width==lock.Pitch) //optimized
	{	
		if(pf.dwRGBBitCount==32)
		{
			if(rgba[0]!=rgba[1]||rgba[0]!=rgba[2]||rgba[0]!=rgba[3])
			{
				D3DCOLOR *q = (D3DCOLOR*)lock.pBits;
				
				for(int i=w*h-1;i<=0;i--) *q++ = c;
			}
			else wmemset((wchar_t*)lock.pBits,*(wchar_t*)&c,w*h*2);
		}
		else if(pf.dwRGBBitCount==16) 
		{
			wmemset((wchar_t*)lock.pBits,c16,w*h);		
		}
		else assert(0);
	}
	else for(int i=0;i<h;i++)
	{
		if(pf.dwRGBBitCount==32)
		{
			D3DCOLOR *p = (D3DCOLOR*)&((BYTE*)lock.pBits)[lock.Pitch*i];

			if(rgba[0]!=rgba[1]||rgba[0]!=rgba[2]||rgba[0]!=rgba[3])
			{
				wmemset((wchar_t*)p,*(WORD*)&c,h*2);
			}
			else for(int j=0;j<w;j++) *p++ = c;				
		}
		else if(pf.dwRGBBitCount==16) //16bit
		{
			WORD *p = (WORD*)&((BYTE*)lock.pBits)[lock.Pitch*i];

			wmemset((wchar_t*)p,c16,w);		
		}		
		else 
		{
			assert(0); break;
		}
	}

	p->UnlockRect(); return true;
}

static bool dx_d3d9c_gammaramp(IDirect3DTexture9 *p, D3DGAMMARAMP *g)
{
	if(!p) return false;

	D3DLOCKED_RECT lock; 

	if(p->LockRect(0,&lock,0,0)!=D3D_OK) return false;

	auto q = (unsigned short*)lock.pBits;

	assert(lock.Pitch==256*2); if(g)
	{
		for(int i=0;i<256;i++) *q++ = g->red[i];
	}
	else for(int i=0;i<256;i++) *q++ = i<<8;
		
	p->UnlockRect(0); return true;
}

extern char dx_d3d9c_dither65[8*8] =
{
	 1,49,13,61, 4,52,16,64,
	33,17,45,29,36,20,48,32,
	 9,57, 5,53,12,60, 8,56,
	41,25,37,21,44,28,40,24,
	 3,51,15,63, 2,50,14,62,
	35,19,47,31,34,18,46,30,
	11,59, 7,55,10,58, 6,54,
	43,27,39,23,42,26,38,22
};
extern bool dx_d3d9c_dither(IDirect3DTexture9 *p, int n, bool dither = true)
{
	D3DSURFACE_DESC desc;

	if(p->GetLevelDesc(0,&desc)!=D3D_OK) return false;
										
	if(dither) switch(n)
	{
	case 65: 

		if(desc.Width!=8||desc.Height!=8||desc.Format!=D3DFMT_A8) 
		{
		default: dither = false;
		}
	}
	
	D3DLOCKED_RECT lock;

	if(p->LockRect(0,&lock,0,0)!=D3D_OK) return false;

	if(dither)
	{
		if(n==65) for(int i=0;i<8;i++)
		{
			void *q = (BYTE*)lock.pBits+lock.Pitch*i;

			memcpy(q,dx_d3d9c_dither65+i*8,8);
		}
		else assert(0);
	}
	else for(size_t i=0;i<desc.Height;i++)
	{
		void *q = (BYTE*)lock.pBits+lock.Pitch*i;

		memset(q,0x00,lock.Pitch);
	}

	p->UnlockRect(0); return true;
}

extern bool dx_d3d9c_autogen_mipmaps = false;
extern void dx_d3d9c_mipmap(DDRAW::IDirectDrawSurface7 *p)
{
	auto g = p->query9->group9;
	if(g&&p->query9->mipmaps>1)
	if(auto f=p->query9->mipmap_f) //DDRAW::mipmap
	{
		UINT lvs = g->GetLevelCount(); assert(lvs>=2);
		
		//2021: even though it has worked with read-only flags
		//som.hacks.cpp is modifying the first mipmap and since
		//this is system memory I think it doesn't really matter
		const DWORD readonly = dx_d3d9c_texture_pool?0:
		D3DLOCK_NO_DIRTY_UPDATE|D3DLOCK_READONLY|D3DLOCK_NOSYSLOCK;

		D3DLOCKED_RECT lock;
		D3DSURFACE_DESC desc;
		if(g->GetLevelDesc(0,&desc)||g->LockRect(0,&lock,0,readonly))
		{
			assert(0); return; 
		}

		DX::DDSURFACEDESC2 src = {sizeof(DX::DDSURFACEDESC2)};
				
		src.dwFlags = DDSD_HEIGHT|DDSD_WIDTH|DDSD_PITCH|DDSD_LPSURFACE|DDSD_PIXELFORMAT;

		src.dwHeight = desc.Height;	src.dwWidth = desc.Width;

		src.lPitch = lock.Pitch; src.lpSurface = lock.pBits;

		src.ddpfPixelFormat = dx_d3d9c_format(desc.Format);

		DX::DDSURFACEDESC2 dst = src;
		
		UINT lv; for(lv=1;lv<lvs;lv++)
		if(!g->GetLevelDesc(lv,&desc)&&!g->LockRect(lv,&lock,0,0))
		{
			dst.dwDepth = lv; //2021
			dst.dwWidth = desc.Width; dst.dwHeight = desc.Height;
			dst.lPitch = lock.Pitch; dst.lpSurface = lock.pBits;

			//bool stop = !DDRAW::mipmap(&src,&dst);
			(void*&)f = f(&src,&dst);

			g->UnlockRect(lv-1);

			if(!f) //probably there was an error
			{
				assert(0); lv++; break; 
			}
			else src = dst;
		}
		else{ assert(0); break; }
	
		g->UnlockRect(lv-1);

		assert(lv==lvs);
	}
	else D3DXFilterTexture(g,0,0,D3DX_FILTER_BOX|D3DX_FILTER_MIRROR);
}

extern bool dx_d3d9c_assume_alpha_ok = true;
extern void dx_d3d9c_assume_alpha_not_ok(DDRAW::IDirectDrawSurface7 *in)
{	
	dx_d3d9c_assume_alpha_ok = false;

	if(!in||!in->proxy9||in->query9->group9==dx_d3d9c_virtual_group) return;

	D3DSURFACE_DESC desc, desc2; 
	if(in->proxy9->GetDesc(&desc)!=D3D_OK) return;

	switch(desc.Format)
	{
	case D3DFMT_A8R8G8B8: desc.Format = D3DFMT_X8R8G8B8; break;
	case D3DFMT_A1R5G5B5: desc.Format = D3DFMT_X1R5G5B5; break;

	default: return;
	}

	if(in->query9->group9)
	{	
		IDirect3DSurface9 *p = in->proxy9;
		IDirect3DTexture9 *q = in->query9->group9;

		IDirect3DDevice9 *d3dd9 = DDRAW::Direct3DDevice7->proxy9;
		
		int lvs = in->query9->group9->GetLevelCount();

		HRESULT ok = d3dd9->CreateTexture
		(desc.Width,desc.Height,lvs,0,desc.Format,D3DPOOL_SYSTEMMEM,&in->query9->group9,0);
				
		if(ok==D3D_OK)
		{
			int dicey = 
			in->clientrefs-1; 
			DDRAW::referee(in,-dicey);
			{
				in->proxy9 = 0;
				in->query9->group9->GetSurfaceLevel(0,&in->proxy9);
			}
			DDRAW::referee(in,+dicey);
			if(in->query9->wasLocked) 
			{
				assert(0); //untested 

				D3DLOCKED_RECT src, dst;		
				for(int lv=0;lv<lvs;lv++)
				{
					if(in->query9->group9->LockRect(lv,&dst,0,0)==D3D_OK)
					{
						if(q->LockRect(lv,&src,0,D3DLOCK_READONLY)==D3D_OK)
						{
							in->query9->group9->GetLevelDesc(lv,&desc);
							q->GetLevelDesc(lv,&desc2);

							assert(dst.Pitch==src.Pitch);
							assert(desc.Height==desc2.Height);

							if(dst.Pitch==src.Pitch
			 				 &&desc.Height==desc2.Height)
							{
								for(size_t i=0;i<desc.Height;i++)
								{
									void *p = (BYTE*)src.pBits+i*src.Pitch;
									void *q = (BYTE*)dst.pBits+i*dst.Pitch;

									memcpy(q,p,src.Pitch);
								}
							}

							q->UnlockRect(lv);
						}
						
						in->query9->group9->UnlockRect(lv);
					}
				}
			}
			DDRAW_ASSERT_REFS(in->query9->group9,>=2)

			p->Release(); q->Release();
		}
		else //woops (not good)
		{
			assert(0); 
			in->proxy9 = 0; 
			in->query9->group9 = 0;
			p->Release(); q->Release();
			return;
		}
	}
	else
	{
		IDirect3DSurface9 *p = in->proxy9;				
		IDirect3DDevice9 *d3dd9 = DDRAW::Direct3DDevice7->proxy9;

		HRESULT ok = d3dd9->
		CreateOffscreenPlainSurface(desc.Width,desc.Height,desc.Format,D3DPOOL_SYSTEMMEM,&in->proxy9,0);

		if(ok==D3D_OK)
		{
			if(in->query9->wasLocked) 
			{	
				assert(0); //untested 

				D3DLOCKED_RECT src, dst;				
				if(in->proxy9->LockRect(&dst,0,0)==D3D_OK)
				{
					if(p->LockRect(&src,0,D3DLOCK_READONLY)==D3D_OK)
					{
						assert(dst.Pitch==src.Pitch);

						if(dst.Pitch==src.Pitch) 
						{
							for(size_t i=0;i<desc.Height;i++)
							{
								void *p = (BYTE*)src.pBits+i*src.Pitch;
								void *q = (BYTE*)dst.pBits+i*dst.Pitch;

								memcpy(q,p,src.Pitch);
							}
						}
						p->UnlockRect();
					}					
					in->proxy9->UnlockRect();
				}  
			}
			p->Release();
		}
		else //woops (not good)
		{
			in->proxy9 = 0; assert(0);
			p->Release();
			return;
		}
	}
}
extern IDirect3DSurface9 *dx_d3d9c_alpha(IDirect3DSurface9 *in)
{
	if(!in) return in; 
	
	bool keep = false;
	IDirect3DSurface9* &out = dx_d3d9c_alphasurface; //!!

	D3DSURFACE_DESC desc;  
	if(in->GetDesc(&desc)) return in;
	switch(desc.Format) 
	{
	case D3DFMT_X8R8G8B8: desc.Format = D3DFMT_A8R8G8B8; break;
	case D3DFMT_X1R5G5B5: desc.Format = D3DFMT_A1R5G5B5; break;

	default: return in;
	}

	assert(!2021); //2021: ENTERED?

	if(out)
	{
		D3DSURFACE_DESC cmp;
		keep = !out->GetDesc(&cmp); if(keep)			
		{				
		if(cmp.Format!=desc.Format) keep = false;
		if(cmp.Height!=desc.Height) keep = false;
		if(cmp.Width !=desc.Width ) keep = false;				
		}	
		if(!keep)
		{
			out->Release(); out = 0;
		}
	}
	if(!out)
	DDRAW::Direct3DDevice7->proxy9->
	CreateOffscreenPlainSurface(desc.Width,desc.Height,desc.Format,D3DPOOL_SYSTEMMEM,&out,0);			
	else assert(keep); if(!out) return in;

	D3DLOCKED_RECT src,dst;
	if(!out->LockRect(&dst,0,0))
	{
		if(in->LockRect(&src,0,D3DLOCK_READONLY)==D3D_OK)
		{
			if(dst.Pitch==src.Pitch) 			
			for(size_t i=0;i<desc.Height;i++)
			memcpy((BYTE*)dst.pBits+i*dst.Pitch,(BYTE*)src.pBits+i*src.Pitch,src.Pitch);
			else assert(0); //todo: relax
			in->UnlockRect();
		}
		out->UnlockRect();
	}
	return out;
}			   
extern IDirect3DTexture9 *dx_d3d9c_alpha(IDirect3DTexture9 *in)
{
	if(!in) return in; 
	
	bool keep = false;
	IDirect3DTexture9* &out = dx_d3d9c_alphatexture; //!!
											
	D3DSURFACE_DESC desc,desc2;
	if(in->GetLevelDesc(0,&desc)) return in;
	switch(desc.Format) 
	{
	case D3DFMT_X8R8G8B8: desc.Format = D3DFMT_A8R8G8B8; break;
	case D3DFMT_X1R5G5B5: desc.Format = D3DFMT_A1R5G5B5; break;

	default: return in;
	}

	assert(!2021); //2021: ENTERED?

	int lvs = in->GetLevelCount();

	if(out)
	{
		if(out->GetLevelCount()==lvs)
		{			
			D3DSURFACE_DESC cmp; 		
			keep = !out->GetLevelDesc(0,&cmp); if(keep)			
			{				
			if(cmp.Format!=desc.Format) keep = false;
			if(cmp.Height!=desc.Height) keep = false;
			if(cmp.Width !=desc.Width ) keep = false;				
			}						
		}
		if(!keep&&out)
		{
			out->Release(); out = 0;
		}
	}	
	if(!out)
	DDRAW::Direct3DDevice7->proxy9->
	CreateTexture(desc.Width,desc.Height,lvs,0,desc.Format,D3DPOOL_SYSTEMMEM,&out,0);			
	else assert(keep); if(!out) return in;

	D3DLOCKED_RECT src, dst;
	for(int lv=0;lv<lvs;lv++)
	{
		if(out->LockRect(lv,&dst,0,0)==D3D_OK)		
		{
			if(in->LockRect(lv,&src,0,D3DLOCK_READONLY)==D3D_OK)
			{
				in->GetLevelDesc(lv,&desc);
				out->GetLevelDesc(lv,&desc2);
				assert(dst.Pitch==src.Pitch);
				assert(desc.Height==desc2.Height);
				if(dst.Pitch==src.Pitch&&desc.Height==desc2.Height)
				{
					for(size_t i=0;i<desc.Height;i++)
					memcpy((BYTE*)dst.pBits+i*dst.Pitch,(BYTE*)src.pBits+i*src.Pitch,src.Pitch);
				}
				else assert(0); //todo: relax
				in->UnlockRect(lv);
			}
			out->UnlockRect(lv);
		}				
	}
	return out;
}

enum{ dx_d3d9c_30fps_test=1 };
extern HRESULT dx_d3d9c_present(IDirect3DDevice9Ex *p)
{		
	DWORD ticks_before_wait = DX::tick(); //EXPEREMENTAL

	HRESULT out = !D3D_OK;

	//note, this is provided for Windows XP systems mainly, so
	//don't bother with the Direct3DDevice9Ex vblack API
	if(DDRAW::doWait&&DDRAW::DirectDraw7)
	DDRAW::DirectDraw7->query9->vblank->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN,0);		
		
		//NOTE: the documentation says if the D3DSWAPEFFECT_COPY 
		//flag is not used, the rectangle is ignored, but that's
		//clearly bullshit!
		//
		// August 2021: D3DSWAPEFFECT_COPY seems to be required
		// now, otherwise D3DERR_INVALIDCALL error is generated
		// Note, 1.2.3.4 isn't having this problem. It doesn't 
		// make any sense unless Microsoft knows to exempt past
		// builds or release builds. I don't think anything has
		// changed
		//
		//2018: needed to support windows that change their size
		//note sure if this is foolproof 100% of the time or not
		HWND win = 0; RECT src; 
		if(!DDRAW::doFlipEx)
		if(!DDRAW::doFlipDiscard) //2021
		if(DDRAW::window_rect(&src))
		win = DDRAW::window;
		
	//EXPERIMENTAL
	//the only way I'm getting the short frame times I'm seeing
	//is if Present(0) is never waiting for the vertical blanks
	//3 back buffers instead of 2 seems to hold to an even 60hz
	int noWait = 0; /*if(0&&DX::debug)
	{
		//Windows 7+ (major improvement)
		//this enables triple buffering. It might be a good idea for
		//non D3DSWAPEFFECT_FLIPEX devices
		//it may be a delicate balance?
		if(!DDRAW::fullscreen		
			//&&DDRAW::noTicks>DDRAW::doFlipEX_MaxTicks
			//&&DDRAW::noTicks<50) //debugging
		&&DDRAW::doFlipEx)
		noWait = D3DPRESENT_DONOTWAIT|D3DPRESENT_FORCEIMMEDIATE;
	}*/

	/*This was an old experiment... I understand sRGB well now
	//I don't believe SOM will benefit from it anytime soon but
	//this might be revived one day. Generally it looks bad for
	//low-poly models with per-vertex lighting
	if(0&&DX::debug) 
	{
	//WARNING: I think the D3DPRESENT_LINEAR_CONTENT cap isn't even defined on 
	//my Intel system that I may have been testing this with at the time

		IDirect3DSwapChain9 *q = 0;
		IDirect3DSwapChain9Ex *qEx = 0;
		if(p->GetSwapChain(0,&q)==D3D_OK)
		{
			if(D3D9C::DLL::Direct3DCreate9Ex)
			if(q->QueryInterface(__uuidof(IDirect3DSwapChain9Ex),(void**)&qEx)==S_OK)
			{
				out = qEx->Present(win?&src:0,0,win,0,D3DPRESENT_LINEAR_CONTENT|noWait);
				qEx->Release();
			}			
			if(out!=D3D_OK)
			out = q->Present(win?&src:0,0,win,0,D3DPRESENT_LINEAR_CONTENT);				
			q->Release();
		}
	}*/
   
	if(out!=D3D_OK)	
	if(D3D9C::DLL::Direct3DCreate9Ex)
	{			   
		/*2021: this goes 100s of fps. it's not just for tearing
		//NOTE: on my system this only works (ironically) if the 
		//window is smaller than the entire screen (I don't know
		//if this is an Intel or Windows policy)
		int sync = DDRAW::isSynchronized?0:D3DPRESENT_FORCEIMMEDIATE;
		out = p->PresentEx(win?&src:0,0,win,0,noWait|sync);*/
		out = p->PresentEx(win?&src:0,win?&src:0,win,0,0);
	}
	else out = p->Present(win?&src:0,0,win,0);

	//EXPEREMENTAL
	//giving multithread solutions (DDRAW::onFlip?)
	//a metric (although just of the earlier frame)
	DDRAW::flipTicks = DX::tick()-ticks_before_wait;

	//2021: Pause unpause is getting stuck in Release build :(
	#if 0 || defined(_DEBUG)
	#define PRESENT_FAILED
	#else
	#define PRESENT_FAILED \
	MessageBoxW(win,L"Present Failed",L"SomEx.dll?",MB_OK);
	#endif	
	   	
	//WARNING: calling assert inside "flip" operations
	//causes an infinite regress. I don't know if this
	//pattern is peculiar to Sword of Moonlight. there
	//needs to be a block to prevent reentry... I have
	//a feeling it's a response to WM_PAINT. I haven't
	//tested it but DDRAW::midFlip is designed to be a
	//block, so these can be put back
	switch(out)
	{
	case E_INVALIDARG:

		//2022: I think my Intel drivers are bugging out
		//since ignoring this went back to normal, but
		//it's probably related to som.MPX.cpp threading
		assert(!win||!E_INVALIDARG);
		if(!win) out = S_OK;
		break;

	case D3DERR_INVALIDCALL: //2021

		//ItemEdit is getting this today... not sure why
		//it's not a legit/expected code
		//
		// NOTE: this was passing "src" in DISCARD mode
		// it had worked for years, I don't know if this
		// is up to the driver. my driver didn't change
		// though that I know of. DDRAW::doFlipDiscard
		// is introduced to resolve this. funnily 1.2.3.4
		// doesn't incur this problem. I wonder what is
		// different? debug builds? I'm losing my mind!
		//
		assert(!D3DERR_INVALIDCALL);
		PRESENT_FAILED //:(
		break;
		
		//GetDeviceSate
	case D3DERR_DEVICEHUNG: //2021

		//I hit this occasionally on my Intel system???
		//have to restart (always at startup?)
		assert(0); break;

	case D3DERR_WASSTILLDRAWING: //doFlipEx?

		assert(noWait); //breakpoint

	case S_PRESENT_OCCLUDED:
	case S_PRESENT_MODE_CHANGED: out = DD_OK; break;	
	case D3DERR_DEVICELOST: out = DDERR_SURFACELOST;

		//TestCooperativeLevel
	case D3DERR_DEVICENOTRESET: //2021
	case D3DERR_DRIVERINTERNALERROR: //2021

	case D3DERR_DEVICEREMOVED: //2021	

		//TODO? the caller is advised to call
		//IDirect3DDevice9Ex::CheckDeviceState (EX)
		//or IDirect3DDevice9::TestCooperativeLevel
		//in response to Present/PresentEx returning
		//DEVICELOST, etc... oddly the return codes
		//are pretty much identical though
		//
		// thing is maybe Reset/ResetEx can be managed 
		// automatically, even though technically it's
		// the callers responsibility. I've never seen
		// this happen though, so I'm not sure if it's
		// a good idea to sink effort into it, and I'd
		// need a way to cause this to occur to try it

	default: 
		
		//WARNING: it seems assert here (why?) generates 
		//a train of assert windows before crashing that
		//there's not time to interact with (why?)
		assert(0); 	
		PRESENT_FAILED //:(
	
	case D3D_OK: break;
	}

	return out;
}

extern void dx_d3d9c_reset() 
{
	const size_t n = dx_d3d9c_direct3d_swapN;
	memset(dx_d3d9c_direct3d_swapchain,0x00,n*sizeof(DX::DDSURFACEDESC2));

	DDRAW_LEVEL(7) << "Deleting swap chain surfaces...\n";

	for(int i=0;i<n;i++) 
	if(dx_d3d9c_direct3d_swapsurfs[i]) dx_d3d9c_direct3d_swapsurfs[i]->Release();
	memset(dx_d3d9c_direct3d_swapsurfs,0x00,n*sizeof(void*));	
	memset(dx_d3d9c_palettes,0x00,sizeof(bool)*256); 
	dx_d3d9c_paletteshint = 1;
}
static void dx_d3d9c_resetdevice()
{
	//2021: dx.d3d9X.cpp does everything shared in common
	extern void dx_d3d9X_resetdevice();
	dx_d3d9X_resetdevice();
	if(!DDRAW::shader) return; //2021

	for(int i=0;i<9;i++) if(dx_d3d9c_brights[i])
	{
		dx_d3d9c_brights[i]->Release(); dx_d3d9c_brights[i] = 0;
	}
	
	if(dx_d3d9c_colorkeyshader==dx_d3d9c_defaultshader)
	{
		dx_d3d9c_colorkeyshader = 0; //...
	}
	
	if(dx_d3d9c_defaultshader) dx_d3d9c_defaultshader->Release();		
	if(dx_d3d9c_colorkeyshader) dx_d3d9c_colorkeyshader->Release();

	dx_d3d9c_defaultshader = dx_d3d9c_colorkeyshader = 0;

	if(dx_d3d9c_effectsshader) dx_d3d9c_effectsshader->Release();

	dx_d3d9c_effectsshader_enable = false;
	dx_d3d9c_effectsshader = 0;

	if(dx_d3d9c_clearshader) dx_d3d9c_clearshader->Release();
	dx_d3d9c_clearshader = 0;
	if(dx_d3d9c_clearshader_vs) dx_d3d9c_clearshader_vs->Release();
	dx_d3d9c_clearshader_vs = 0;
}

extern void dx_d3d9c_discardeffects() //dx.d3d9X.cpp
{	
	if(!DX::debug||!DDRAW::shader) return; //2021

	assert(DDRAW::Direct3DDevice7);

	auto q = DDRAW::Direct3DDevice7->query9;

	if(q->multisample)
	{
		q->multisample->Release(); q->multisample = 0;		
	}								

	for(int i=0;i<=+DDRAW::do2ndSceneBuffer;i++)
	if(q->effects[i])
	{
		q->effects[i]->Release(); q->effects[i] = 0;
	}
}

extern float dx_d3d9c_update_vsPresentState_staircase(int flip=0)
{	
	flip+=(int)DDRAW::noFlips;

	int period = DDRAW::dejagrate;

	//NEW: 8/22/2014 antialising discovery (Wow!)
	float staircase; if(0||!DDRAW::do2ndSceneBuffer) 
	{
		//2nd: assuming half tone motion blur if so
		//(power must be 1 if blurring or knots will appear)
		float power = DDRAW::do2ndSceneBuffer?1.0f:0.5f;

		staircase = (0.5f-float(flip%period)/(period-1))*power;
		//2020: I'm skeptical *2 is correct?
		//2021: I think it is correct for snapping to single pixels over -1,+1
		//float c[4] = {rcpw*2,rcph*2,rcpw+rcpw*staircase,rcph+rcph*staircase};
	}
	else if(1||3!=period) //half pixel version with minimal popping
	{
		staircase = (0.5f-float(flip%period)/(period-1)); ///2;
	}
	else //2020: alternative pattern for do_sharp? (EXPERIMENTAL)
	{
		//this makes every other frame 0 and the odd frames bounce
		//back and forth on either side of 0. this way the sharper
		//frame is never separated by by the jump from 0.5 to -0.5

		staircase = flip%2?0:flip%4?-0.5f:0.5f;			
	}
		
	//if(!DDRAW::doSuperSampling)
	{
		//NEW: ensure slightly less than 1 pixel
		//to prevent very seldom shimmering edges
		//NOTE: this prevents it from being smack
		//on the edge which could be unpredictable
		//in terms of which pixel lights up
		staircase*=0.98f;
	}

	if(DDRAW::doSuperSampling)
	{
		//NOTE: 2 seems to work best (assuming 4x supersampling)
		//NOTE: I think 2 puts the pixel right on the edge of the
		//pixel, which could go either way if not multiplied below
		
		//ATTENTION: what seemed to help 1.5 supersampling a lot 
		//was sampling exactly between the mipmap (50%) ... prior
		//to this the PlayStation VR results were bad at 0% or 75%
		//(I felt 75% was best without supersampling)
		//(by "bad" I mean text was pixelated at 75% splotchy at 0%)

		if(1||!DX::debug) //REMOVE ME
		DDRAW::doSuperSamplingMul(staircase); //1.5 or 2?
		else staircase*=0?1:2; //TESTING
	}

	if(DDRAW::xr) //DICEY
	{
		//EXTENSION
		//what even is the baseline?
		//https://community.khronos.org/t/can-we-retrieve-super-sampling-ratio-to-be-able-to-reason-about-aa-effects/108947

		//can kind of base this on current technology?
		int h = DDRAW::Direct3DDevice7->queryX->effects_h;

		if(int zd=DDRAW::dejagrate_divisor_for_OpenXR) //zero-divide?
		{
			//my set is 4320 x 2160

			//I think I want to tie F3 to this so players can
			//manually focus it
			//staircase*=(float)h/2160;
			//NOTE: h is actually 3092 at 100%
		//	staircase*=(float)h/3100; //ARBITRARY
			staircase*=(float)h/DDRAW::dejagrate_divisor_for_OpenXR;
		}
		else staircase = 0.0f;
	}

	return staircase;
}
extern void dx_d3d9c_update_vsPresentState(D3DVIEWPORT9 &vp)
{				
	//REMINDER
	//
	// I think 1/rcpw is 1/2px, because clipping space
	// range is -1 to +1
	//
	//float rcpw = 1.0f/vp.Width, rcph = 1.0f/vp.Height;
	float rcpw = vp.Width, rcph = vp.Height;
	DDRAW::doSuperSamplingMul(rcpw,rcph);
	rcpw = 1/rcpw; rcph = 1/rcph;

	float staircase = dx_d3d9c_update_vsPresentState_staircase();

	//I think this is 1px clamping but I don't know how it ever
	//ever worked before
	//float c[4] = {rcpw*2,rcph*2,rcpw+rcpw*staircase,rcph+rcph*staircase};
	float c[4] = {rcpw,rcph,rcpw+rcpw*staircase,rcph+rcph*staircase};
	dx_d3d9X_vsconstant(DDRAW::vsPresentState,c); 
}
extern void DDRAW::dejagrate_update_psPresentState()
{
	//som.mocap.cpp calls this blindly
	if(DDRAW::compat!='dx9c') return;

	//NOTE: THIS IS IMPLEMENTING DDRAW::gl AS WELL

	if(!dx_d3d9c_effectsshader_enable) return;

	//OpenGL can't work this way
	//if(!DDRAW::Direct3DDevice7->query9) return;	
	//auto *p = DDRAW::Direct3DDevice7->query9->effects[0];
	//if(!p) return;
	//D3DSURFACE_DESC desc; p->GetLevelDesc(0,&desc); 
	//float r = desc.Width, t = desc.Height; 
	/*this is wrong for OpenXR
	float r,t; if(auto*p=DDRAW::PrimarySurface7)
	{
		r = p->queryX->width;
		t = p->queryX->height;
		DDRAW::doSuperSamplingMul(r,t);
	}
	else{ assert(0); return; }
	*/
	auto *q = DDRAW::Direct3DDevice7->queryX;
	float r = q->effects_w;
	float t = q->effects_h;

	//RENDER-TARGET space (do_stipple/DSTIPPLE)
	//float rcpw = 1.0f/vp.Width, rcph = 1.0f/vp.Height;
	float psc[4] = {1/r,1/t};

	/*2020
			
		this is now about the new texture AA effect that
		takes over these previously marginal zw constant
		values. the fxStrobe effect shouldn't require AA
	*/
	if(DDRAW::fxStrobe) //for stipple/checker effects
	{
		psc[2] = psc[3] = 
		//int checker = //!: zero divide
		/*!DDRAW::fxStrobe||*/DDRAW::noFlips 
		%(2*DDRAW::fxStrobe)>=DDRAW::fxStrobe?0:1;
		//dx_d3d9X_psconstant(DDRAW::psPresentState,checker,'z'); 
		//dx_d3d9X_psconstant(DDRAW::psPresentState,checker,'w'); 	   					
	}
	else if(0) //best (no bias) but side effects
	{
		//the problem with sweeping the diagonal is the large jump
		//from one side to the other is visible in the texture mid
		//movement. however, staggering them seems to do the trick

		//sweep the diagonal? (no shape)
		psc[2] = psc[3] = 0.5f-0.5f*(DDRAW::noFlips%3);

		if(1) //stagger? no side effect, but is it biased?
		{
			//note this change creates a tall right-triangle shape
			//so my thinking is using an equilateral triangle must
			//be better, but it can depend on if bias matters more
			//than equidistance

			//really a tall right-triangle
			psc[3] = DDRAW::noFlips?DDRAW::noFlips==1?-0.5f:0.5f:0;
		}

		float scale = 0.75f; psc[2]*=scale; psc[3]*=scale;
	}
	else if(0) //good but flickery (especially on Continue prompt)
	{
		//note: these don't seem to be workable but they are worth
		//considering and interesting that they don't work because
		//intuitively they have merit

		if(0) psc[2] = psc[3] = (0.5f-(DDRAW::noFlips%2))*0.5f;
		if(1) psc[2] = psc[3] = (DDRAW::noFlips%2?0:DDRAW::noFlips%4?-0.5f:0.5f)*0.66666f;	
	}
	else //equilateral triangle (biased but less side effects)
	{ 
		//this is an equilateral triangle tilted to maximize
		//the slope in x/y so both dimensions move as much as
		//possible. the rationale for this is to avoid the big
		//hop from 0.5 to -0.5 that doesn't make as much since
		//here as it does for jaggy rasterization

		switch(DDRAW::noFlips%3)
		{
		//trying with centered extremes for maximal stability?
		#if 1
		case 0: psc[2] = 0.153; psc[3] = 0.175; break;
		case 1: psc[2] = 0.916; psc[3] = 0.357; break;
		case 2: psc[2] = 0.377; psc[3] = 0.930; break;
		#else 
		//centering seems to bias right side to looking better
		case 0: psc[2] = 0.118; psc[3] = 0.122; break;
		case 1: psc[2] = 0.881; psc[3] = 0.304; break;
		case 2: psc[2] = 0.342; psc[3] = 0.877; break;
		#endif
		}
		//not sure if this helps but it has to be factored into
		//the later scale factor either way
		//note: they're remarkably close together. they weren't
		//always so because I'd accidentally scaled my original
		//equilateral triangle in the art program
		float scale[2] = {1.3106f,1.3245f}; 
		if(0) scale[1] = scale[0];

		//I think there's a sweet spot but this looks pretty
		//good short of doing real-time adjustments sometime
		//0.8 is nice except its side effects are noticeable 
		//for(int i=2;i<=3;i++) psc[i] = (psc[i]-0.5f)*scale[i-2]*0.8f;
		//for(int i=2;i<=3;i++) psc[i] = (psc[i]-0.5f)*scale[i-2]*0.6f;
		//0.6 is still too much side-effect, but going lower
		//seems unacceptable? quality and side effect may be
		//linked?
		/*this can be stroby on plain textures
		//2021: trying something less conservative to see how it interacts
		//with the new edge-detection contrast reduction effects technique
		float power = 0&&DDRAW::sRGB?0.8f:0.66666f;*/
		//TODO: let user increase when not rotating...
		float power = 1-DDRAW::fxCounterMotion;
		assert(power>=0);
		//I think I can sometimes sense 0.78 shifting even on still images
		//power*=0.78f;
		//I'm negating here because in the HUD elements I was only seeing
		//up and down movement... I think maybe it's synchronizing with 
		//the polygon AA effect to somehwate cancel out horizontal motion
		/*April 2021: feel like needs to tone down after changing back to
		//blurry mode
		power*=-0.725f;*/
		power*=0?-0.725f:-0.66666f;
		for(int i=0;i<2;i++) 
		{
			//fxCounterMotion is to try to defeat strobing that's generally caused
			//by turning (have it both ways) but it shouldn't go any lower 
			//since it's possible to hold focus while turning (0.6 looks a
			//lot better, it's hard to hold a focus that low)
			//float power = 0.8f-0.133333f*DDRAW::fxCounterMotion[i];
			/*KF2's chalk white water cave ceilings are the king of strobe
			//on the bright side it highlights a need for a countermeasure
			//NOTE: ~0.78 is balanced against KF2's compass
			float power = 0.78f*(1-min(1,1?fused:DDRAW::fxCounterMotion[i]));*/
			
			psc[2+i] = (psc[2+i]-0.5f)*scale[i]*power; //(DDRAW::sRGB?0.72f:0.66666f);
		}
	}

	dx_d3d9X_psconstant(DDRAW::psPresentState,psc,'xyzw');
}


////////////////////////////////////////////////////////
//              DIRECTX9 INTERFACES                   //
////////////////////////////////////////////////////////


//extern: dx.ddraw.cpp
extern void *dx_ddraw_hacks[DDRAW::TOTAL_HACKS]; 

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

//TEMPORARY
#define DDRAW_IF_NOT_TARGET_RETURN(...) \
	if(target!=DDRAW::target) return(assert(0),__VA_ARGS__);

#define DDRAW_IF_NOT_DX9C_RETURN(...) \
	if(target!='dx9c') return(assert(0),__VA_ARGS__);
		   
#define DDRAW_ADDREF DDRAW::referee(this,+1);

#define DDRAW_ADDREF_RETURN(zero) \
	if(!this) return zero; DDRAW_ADDREF return clientrefs;


#define DDRAW_IF_NOT_TARGET_AND_RELEASE(snapshot,...) \
	DDRAW_IF_NOT_TARGET_RETURN(__VA_ARGS__,0)\
	ULONG snapshot = DDRAW::referee(this,-1);


//REMOVE US (I've a bad feeling about these)
//OBSOLETE?
//what is this even for?
//hack: defined in dx.ddraw.cpp
extern ULONG dx_ddraw_try_directdraw7_release();
//extern ULONG dx_ddraw_try_primarysurface7_release();
extern ULONG dx_ddraw_try_direct3ddevice7_release();

HRESULT D3D9C::IDirectDraw::QueryInterface(REFIID riid, LPVOID FAR *ppvObj)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDraw::QueryInterface()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!S_OK)

	DDRAW_PUSH_HACK(DIRECTDRAW_QUERYINTERFACE,IDirectDraw*,
	REFIID,LPVOID*&)(0,this,riid,ppvObj);

	if(7<=DDRAWLOG::debug)
	{
		LPOLESTR o; if(StringFromIID(riid,&o)==S_OK) DDRAW_LEVEL(7) << ' ' << o << '\n';
	}

	if(riid==DX::IID_IDirectDraw) //(6C14DB80-A733-11CE-A521-0020AF0BE560)
	{
		DDRAW_LEVEL(7) << "(IDirectDraw)\n";

		*ppvObj = this; AddRef();
	}
						 
	DDRAW_POP_HACK(DIRECTDRAW_QUERYINTERFACE,IDirectDraw*,
	REFIID,LPVOID*&)(&out,this,riid,ppvObj);

	if(out!=S_OK){ DDRAW_LEVEL(7) << "QueryInterface FAILED\n"; return out; }

	DDRAW_RETURN(out)
}
ULONG D3D9C::IDirectDraw::AddRef()
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDraw7::AddRef()\n";

	DDRAW_ADDREF_RETURN(0)
}
ULONG D3D9C::IDirectDraw::Release()
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDraw::Release()\n";

	DDRAW_IF_NOT_TARGET_AND_RELEASE(out,0)

	//REMOVE ME?
	if(out==1) return dx_ddraw_try_directdraw7_release();
	
	if(out==0) delete this; 

	return out;
}	







HRESULT D3D9C::IDirectDraw7::QueryInterface(REFIID riid, LPVOID FAR *ppvObj)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDraw7::QueryInterface()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!S_OK)

	DDRAW_PUSH_HACK(DIRECTDRAW7_QUERYINTERFACE,IDirectDraw7*,
	REFIID,LPVOID*&)(0,this,riid,ppvObj);

	if(7<=DDRAWLOG::debug)
	{
		LPOLESTR o; if(StringFromIID(riid,&o)==S_OK) DDRAW_LEVEL(7) << ' ' << o << '\n';
	}

	if(riid==DX::IID_IDirectDraw //(6C14DB80-A733-11CE-A521-0020AF0BE560)
	 ||riid==DX::IID_IDirectDraw4) //(9C59509A-39BD-11D1-8C4A-00C04FD930C5)
	{		
		DDRAW_LEVEL(7) 
		<< " (IDirectDraw" 
		<< dx%(riid==IID_IDirectDraw4?"4":"") 
		<< ")\n";

		if(riid==DX::IID_IDirectDraw)
		{
			DDRAW::IDirectDraw *p = new DDRAW::IDirectDraw(dx95,query);

			*ppvObj = p; p->proxy9 = proxy9; proxy9->AddRef();
		}
		else if(riid==DX::IID_IDirectDraw4)
		{
			DDRAW::IDirectDraw4 *p = new DDRAW::IDirectDraw4(dx95,query);

			*ppvObj = p; p->proxy9 = proxy9; proxy9->AddRef();
		} 
		else DDRAW_FINISH(E_NOINTERFACE)

		out = S_OK;
	}
	else if(riid==DX::IID_IDirect3D3 //(BB223240-E72B-11D0-A9B4-00AA00C0993E)
		  ||riid==DX::IID_IDirect3D7) //(F5049E77-4861-11D2-A407-00A0C90629A8)
	{	
		DDRAW_LEVEL(7) 
		<< " (IDirect3D" 
		<< dx%(riid==DX::IID_IDirect3D3?"3":"7") 
		<< ")\n";
				
		if(riid==DX::IID_IDirect3D3)
		{
			DDRAW::IDirect3D3 *p = new DDRAW::IDirect3D3(dx95,query->d3d);

			query->d3d = p->query; p->query->ddraw = query; 

			p->query9->adapter = query9->adapter;

			*ppvObj = p; p->proxy9 = proxy9; proxy9->AddRef();
		}
		else if(riid==DX::IID_IDirect3D7)
		{
			DDRAW::IDirect3D7 *p = new DDRAW::IDirect3D7(dx95,query->d3d);

			query->d3d = p->query; p->query->ddraw = query;

			p->query9->adapter = query9->adapter;

			*ppvObj = p; p->proxy9 = proxy9; proxy9->AddRef();
		}
		else DDRAW_FINISH(E_NOINTERFACE)

		out = S_OK; 
	}
	else DDRAW_FINISH(!S_OK) //unexpected interface
	
	DDRAW_POP_HACK(DIRECTDRAW7_QUERYINTERFACE,IDirectDraw7*,
	REFIID,LPVOID*&)(&out,this,riid,ppvObj);

	if(out!=S_OK){ DDRAW_LEVEL(7) << "QueryInterface FAILED\n"; return out; }

	DDRAW_RETURN(out)
}
ULONG D3D9C::IDirectDraw7::AddRef()
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDraw7::AddRef()\n";

	DDRAW_ADDREF_RETURN(0)
}
ULONG D3D9C::IDirectDraw7::Release()
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDraw7::Release()\n";

	DDRAW_IF_NOT_TARGET_AND_RELEASE(out,0)

	//see DDRAW::IDirectDraw7::Release for concerns about
	//this pattern
	if(out==1) dx_ddraw_try_directdraw7_release();

	if(out==0) 
	if(DDRAW::DirectDraw7==this)
	{		
		dx_d3d9c_reset();
		//
		// NOTE: in this case dx_d3d9c_reset should've released these
		//
		int i; //NOTE: if assert fails technically it can be a client releases out-of-order
		//2022: this failed... but worked... perhaps because of DDRAW::doMultiThreaded from 
		//recent work in som.MPX.cpp?? (UPDATE: som_MPX_DirectDrawCreateEx is preventing its
		//IDirectDraw7 from being released, so it will be hard to diagnose this going forward)
		for(i=0;DDRAW::PrimarySurface7;){ assert(!i++); DDRAW::PrimarySurface7->Release(); }
		for(i=0;DDRAW::BackBufferSurface7;){ assert(!i++); DDRAW::BackBufferSurface7->Release(); }
		for(i=0;DDRAW::DepthStencilSurface7;){ assert(!i++); DDRAW::DepthStencilSurface7->Release(); }

		DDRAW::DirectDraw7 = 0; 
	}
	else assert(0);

	if(out==0) delete this;

	return out;
}	
HRESULT D3D9C::IDirectDraw7::CreateClipper(DWORD x, DX::LPDIRECTDRAWCLIPPER *y, IUnknown *z)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDraw7::CreateClipper()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

		//REMINDER: ItemEdit and friends use this
		//dx.ddraw.cpp includes 'dx9c' in SetHWnd
	DDRAW::IDirectDrawClipper *p = new DDRAW::IDirectDrawClipper(target);
			    
	p->proxy9 = proxy9; proxy9->AddRef();
	
	*y = p; 
	
	DDRAW_LEVEL(1) << " Created dummy IDirectDrawClipper\n"; return DD_OK;
}
HRESULT D3D9C::IDirectDraw7::CreatePalette(DWORD x, LPPALETTEENTRY y, DX::LPDIRECTDRAWPALETTE FAR *z, IUnknown FAR *w)
{
	assert(0); //SOM::Texture::palette? //2023: not expecting palettes

	DDRAW_LEVEL(7) << "D3D9C::IDirectDraw7::CreatePalette()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	if(!y||!z) DDRAW_RETURN(!DD_OK) assert(!w);

	if(!DDRAW::Direct3DDevice7) return false; //todo: backup palette

	//TODO: track D3DPTEXTURECAPS_ALPHAPALETTE capability

	IDirect3DDevice9 *d3dd9 = DDRAW::Direct3DDevice7->proxy9;

	int palette = -1, paranoia = 0;

	while(dx_d3d9c_palettes[dx_d3d9c_paletteshint])
	{
		if(++dx_d3d9c_paletteshint>=256) 
		{
			dx_d3d9c_paletteshint = 1;	 	

			if(paranoia++>=1) break;
		}
	}

	if(dx_d3d9c_palettes[dx_d3d9c_paletteshint]) DDRAW_RETURN(!DD_OK)	

	palette = dx_d3d9c_paletteshint; assert(palette>0&&palette<256);

	for(int i=0;i<256;i++) //debugging test only
	{
		y[i].peFlags = y[i].peRed<16&&y[i].peGreen<16&&y[i].peBlue<16?0:255;
	}

	if(d3dd9->SetPaletteEntries(palette,y)!=D3D_OK) DDRAW_RETURN(!DD_OK)

	DDRAW::IDirectDrawPalette *p = new DDRAW::IDirectDrawPalette(dx95);

	dx_d3d9c_palettes[dx_d3d9c_paletteshint] = true;

	p->palette9 = palette; p->caps9 = x; 

	*z = p; 

	return DD_OK;
}
HRESULT D3D9C::IDirectDraw7::CreateSurface(DX::LPDDSURFACEDESC2 x, DX::LPDIRECTDRAWSURFACE7 *y, IUnknown *z)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDraw7::CreateSurface()\n";
		
	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK) //DX9X SUBROUTINE
		
	DDRAW_PUSH_HACK(DIRECTDRAW7_CREATESURFACE,IDirectDraw7*,
	DX::LPDDSURFACEDESC2&,DX::LPDIRECTDRAWSURFACE7*&,IUnknown*&)(0,this,x,y,z);
		
	if(!x||!y) DDRAW_FINISH(out)

	if(DDRAWLOG::debug>=8) //#ifdef _DEBUG
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

	out = !DD_OK;

	enum{ swap_surfaces=DDSCAPS_PRIMARYSURFACE|DDSCAPS_ZBUFFER|DDSCAPS_BACKBUFFER };

	//2021: this is useful for detecting errors in new code. technically
	//the following check should ensure this too but I worry it will cause
	//problems in working code (Lock doesn't set DDSD_CAPS though it should)
	assert(x->dwFlags&DDSD_CAPS);
	//if(!DDRAW::Direct3DDevice7) //assuming the swap chain is being setup
	if(int cmp=x->ddsCaps.dwCaps&swap_surfaces) //2021
	{	
		ItemEdit: //???

		//REMINDER: backbuffer is filled in by GetAttachedSurface
reset:	int i;
		for(i=0;i<dx_d3d9c_direct3d_swapN;i++)		
		{
			auto &ea = dx_d3d9c_direct3d_swapchain[i];

			if(!ea.dwSize){ ea = *x; break; }

			if(cmp==(ea.ddsCaps.dwCaps&swap_surfaces)) break;
		}

		if(i<dx_d3d9c_direct3d_swapN)
		{
			DDRAW::IDirectDrawSurface7 *p; 
			if(!dx_d3d9c_direct3d_swapsurfs[i])
			{
				dx_d3d9c_direct3d_swapsurfs[i] = 
				p = new DDRAW::IDirectDrawSurface7(dx95);

				//IDirectDraw7::Release should release them
				{
					//HACK: just preventing AddRef() assert 
					p->proxy9 = dx_d3d9c_virtual_surface+i;
					p->AddRef();
				}

				DDRAW::IDirectDrawSurface7 **pp = 0;
				if(x->ddsCaps.dwCaps&DDSCAPS_PRIMARYSURFACE)
				{
					pp = &DDRAW::PrimarySurface7;
				}
				else if(x->ddsCaps.dwCaps&DDSCAPS_ZBUFFER) //2021
				{
					pp = &DDRAW::DepthStencilSurface7;
				}		
				else if(x->ddsCaps.dwCaps&DDSCAPS_BACKBUFFER) //2021
				{
					//I think only GetAttachedSurface can get the backbuffer
					//but just to cover all the bases
					assert(0);

					pp = &DDRAW::BackBufferSurface7;
				}
				if(pp){ assert(!*pp); *pp = p; }
			}
			p = dx_d3d9c_direct3d_swapsurfs[i];

			//2021: I recall adding this (recently) to be able to have
			//width/height in a standard place but it poses a problem if
			//DDRAW::window isn't set. SOM_MAP.cpp can't use the window's
			//dimensions, and so is passing width/height for the primary
			//surface even though I think that fails if actually passed to
			//ddraw.dll (i.e. if it used Direct3D7)
			if((x->dwFlags&(DDSD_HEIGHT|DDSD_WIDTH))!=(DDSD_HEIGHT|DDSD_WIDTH))
			{
				RECT client; 
				if(DDRAW::window_rect(&client))
				{
					//if(!(x->dwFlags&DDSD_HEIGHT))
					{
						dx_d3d9c_direct3d_swapchain[i].dwFlags|=DDSD_HEIGHT;						
						x->dwHeight = //2021
						dx_d3d9c_direct3d_swapchain[i].dwHeight = client.bottom;
					}
					//if(!(x->dwFlags&DDSD_WIDTH))
					{
						dx_d3d9c_direct3d_swapchain[i].dwFlags|=DDSD_WIDTH;
						x->dwWidth = //2021
						dx_d3d9c_direct3d_swapchain[i].dwWidth = client.right;
					}
				}
				else //UNFINISHED? //SetDisplayMode?
				{
					//client holds junk on failure
					//setting width/height may be
					//required
					assert(DDRAW::window);
				}
			}
			if(x->dwHeight%2) x->dwHeight++; //2023: ItemEdit.exe?
			if(x->dwWidth%2) x->dwWidth++;

			p->query9->group9 = dx_d3d9c_virtual_group;

			assert(p->proxy9<dx_d3d9c_virtual_surface+dx_d3d9c_direct3d_swapN);
			p->proxy9 = dx_d3d9c_virtual_surface+i;
			p->query9->isVirtual = true; //2021

			if(x->dwFlags&DDSD_CAPS)
			if(x->ddsCaps.dwCaps&DDSCAPS_PRIMARYSURFACE)
			{
				if(x->ddpfPixelFormat.dwFlags==0x00000000) //hack
				dx_d3d9c_direct3d_swapchain[i].ddpfPixelFormat = dx_d3d9c_format(D3DFMT_X8R8G8B8);					
				dx_d3d9c_direct3d_swapchain[i].dwFlags|=DDSD_PIXELFORMAT; //2021

				p->query9->isPrimary = true; 

				p->queryX->format = //2021
				p->queryX->nativeformat = D3DFMT_X8R8G8B8;
				
			//	p->AddRef(); //for dx_ddraw_try_primarysurface7_release???

				if(x->ddpfPixelFormat.dwRGBBitCount)
				{				
					DDRAW::bitsperpixel = x->ddpfPixelFormat.dwRGBBitCount;
				
					//dx_d3d9X_enableeffectsshader(DDRAW::bitsperpixel==16);				
				}
			}

			//2021
			p->query9->width = x->dwWidth; 
			p->query9->height = x->dwHeight; 

			*y = (DX::LPDIRECTDRAWSURFACE7)p; 
			
			out = DD_OK; 
		}
		else assert(0);
	}
	else if(!DDRAW::Direct3DDevice7) //2021
	{		
		//ItemEdit? //EneEdit?
		//DDSCAPS_OFFSCREENPLAIN|DDSCAPS_3DDEVICE|DDSCAPS_VIDEOMEMORY

		goto ItemEdit; //TESTING //???

		return !D3D_OK;
	}
	else
	{
		assert(DDRAW::target=='dx9c');

		if(x->dwFlags&DDSD_CAPS&&x->ddsCaps.dwCaps&DDSCAPS_PRIMARYSURFACE) 
		{
			//HACK: assuming the device is thought to be lost / being reset

			dx_d3d9c_resetdevice();	dx_d3d9c_reset(); goto reset;			
		}
					  
		if((x->dwFlags&(DDSD_HEIGHT|DDSD_WIDTH))==0)
		{
			DDRAW_FINISH(DDRAW_UNIMPLEMENTED)
		}

		DDRAW_LEVEL(7) << x->dwWidth << " x " << x->dwHeight << "\n";

		IDirect3DDevice9Ex *d3ddev = DDRAW::Direct3DDevice7->proxy9;
				
		//Note: D3DUSAGE_DYNAMIC required for GDI (GetDC) support
		DWORD usage = D3DUSAGE_DYNAMIC;

		D3DPOOL pool = dx_d3d9c_texture_pool; 

		if(x->ddsCaps.dwCaps&DDSCAPS_SYSTEMMEMORY) pool = D3DPOOL_SYSTEMMEM;

		if(DDRAW::Direct3DDevice7->query9->isSoftware) pool = D3DPOOL_SYSTEMMEM; 
		
		D3DFORMAT client = dx_d3d9c_format(x->ddpfPixelFormat);
		D3DFORMAT format = client;
		
		if(dx_d3d9c_assume_alpha_ok
		&&!DDRAW::Direct3DDevice7->query9->isSoftware)
		{
			if(dx_d3d9c_imitate_colorkey)
			//note: large surfs incur unacceptable delay
			//if(x->dwWidth*x->dwHeight<512*512) 			
			{				 
				//force alpha formats in order to fake colorkey
				//note: docs say alpha formats are incompatible with GDI (GetDC)
				if(format==D3DFMT_R8G8B8)   format = D3DFMT_A8R8G8B8;
				if(format==D3DFMT_X8R8G8B8) format = D3DFMT_A8R8G8B8;
				if(format==D3DFMT_X1R5G5B5) format = D3DFMT_A1R5G5B5;
				if(format==D3DFMT_R5G6B5)   format = D3DFMT_A1R5G5B5;		
			}
		}
		else
		{
			if(format==D3DFMT_R8G8B8)   format = D3DFMT_X8R8G8B8;
			if(format==D3DFMT_A8R8G8B8) format = D3DFMT_X8R8G8B8;
			if(format==D3DFMT_A1R5G5B5) format = D3DFMT_X1R5G5B5;			

			//Not supported for offscreen plain surfaces
			if(format==D3DFMT_P8) format = D3DFMT_X8R8G8B8;
		}

		if(DDRAW::doMipmaps)
		//if(dx_d3d9c_imitate_colorkey)
		if(x->ddsCaps.dwCaps&DDSCAPS_TEXTURE)
		if(!DDRAW::Direct3DDevice7->query9->isSoftware)
		{
			format = D3DFMT_A8R8G8B8;  
		}
		
		LPDIRECT3DSURFACE9 s = 0;
		LPDIRECT3DTEXTURE9 t = 0; 
		
		if(x->ddsCaps.dwCaps&DDSCAPS_OFFSCREENPLAIN
		||DDRAW::Direct3DDevice7->query9->isSoftware)
		{
		  // a true offscreen "plain" surface won't work here //
		  // because d3dd9 style blitting is missing from d3d //

			if(pool!=D3DPOOL_DEFAULT
			||DDRAW::Direct3DDevice7->query9->isSoftware)
			{
				//NOTE: this is just for group9/update9 pattern
				out = d3ddev->CreateOffscreenPlainSurface(x->dwWidth,x->dwHeight,format,pool,&s,0);
			}
			else 
			{					
				//hack: D3D9 just won't map a texture from system memory!!
				//if(!DDRAW::Direct3DDevice7->query9->isSoftware)
				//pool = D3DPOOL_DEFAULT;
				assert(pool==D3DPOOL_DEFAULT);

				out = d3ddev->CreateTexture(x->dwWidth,x->dwHeight,1,usage,format,pool,&t,0);
			}

			if(out==D3D_OK&&t) out = t->GetSurfaceLevel(0,&s);
		}
		else if(x->ddsCaps.dwCaps&DDSCAPS_TEXTURE)
		{				
			int mipmaps = DDRAW::Direct3DDevice7->query9->isSoftware?1:0; 
						
			if(DDRAW::doMipmaps&&dx_d3d9c_autogen_mipmaps)
			{
				if(mipmaps==0) usage|=D3DUSAGE_AUTOGENMIPMAP;
			}
			else if(!DDRAW::doMipmaps)			
			{
				if(x->ddsCaps.dwCaps&DDSCAPS_MIPMAP)
				{
					if(x->dwFlags&DDSD_MIPMAPCOUNT) 
					{
						mipmaps = x->dwMipMapCount; //appropriate??
					}
				}
				else mipmaps = 1;
			}
			else if(!DDRAW::mipmap) mipmaps = 1; //2021

			out = d3ddev->CreateTexture(x->dwWidth,x->dwHeight,mipmaps,usage,format,pool,&t,0);
			DDRAW_ASSERT_REFS(t,==1)

			//if(DX::debug&&usage&D3DUSAGE_AUTOGENMIPMAP) //EXPERIMENTAL
			//t->SetAutoGenFilterType(D3DTEXF_POINT);

			if(out==D3D_OK)
			{
				out|=t->GetSurfaceLevel(0,&s);
			
				DDRAW_ASSERT_REFS(t,==2)
			}
		}
		else DDRAW_FINISH(DDRAW_UNIMPLEMENTED)	

		DDRAW::IDirectDrawSurface7 *p = 0;

		if(s&&out==D3D_OK)
		{
			p = new DDRAW::IDirectDrawSurface7(dx95);

			//note: group9 only set for textures		
			if(x->ddsCaps.dwCaps&DDSCAPS_TEXTURE) //plain surfaces are textures now
			if(s->GetContainer(__uuidof(IDirect3DTexture9),(void**)&p->query9->group9))			
			assert(!p->query9->group9); //paranoia
			
			DDRAW_ASSERT_REFS(p->query9->group9,==3)

			if(dx_d3d9c_texture_pool==D3DPOOL_DEFAULT) 
			{
				p->query9->update9 = t; if(t) t->AddRef(); else assert(0);

				DDRAW_ASSERT_REFS(p->query9->group9,==4)
			}

			s->AddRef(); //WHY? because Release is called below???

			p->proxy9 = s;
			
			p->query9->nativeformat = client; //alpha?

			p->queryX->format = format; //2021

			p->query9->pitch = dx_d3d9c_pitch(p->proxy9);

			//2021
			p->query9->isTextureLike = true;
			p->query9->width = x->dwWidth;
			p->query9->height = x->dwHeight;
			p->query9->mipmaps = t?t->GetLevelCount():1;
			p->query9->mipmap_f = DDRAW::mipmap;
			p->query9->colorkey_f = DDRAW::colorkey;

			*y = (DX::LPDIRECTDRAWSURFACE7)p;

			DDRAW_ASSERT_REFS(p->query9->group9,>=3)
		}
		if(s) s->Release(); if(t) t->Release();

		if(p) DDRAW_ASSERT_REFS(p->query9->group9,==2)
	}

	if(y) DDRAW_LEVEL(7) << "Created Surface (" << (unsigned)*y << ")\n";
		
	DDRAW_POP_HACK(DIRECTDRAW7_CREATESURFACE,IDirectDraw7*,
	DX::LPDDSURFACEDESC2&,DX::LPDIRECTDRAWSURFACE7*&,IUnknown*&)(&out,this,x,y,z);

	if(!x||!y) return out; //hack: assuming short-circuited by hack

		//Known behavior:
	//Sword of Moonlight will request comically large width
	if(x->dwWidth>4096*4) return out;
	
	DDRAW_RETURN(out)
}
static unsigned short dx_d3d9c_rez[] = 
{
	//first is the width followed by heights
	0,1920,1080,1200,1280,1400,1440,
	0,1680,1050,
	0,1600,1200,900,
	0,1440,900,1440,
	0,1366,768,
	0,1360,768,
	0,1280,1024,960,1800,768,720,
	0,1176,664,
	0,1152,864,
	0,1024,768,
	0,800,600,
	0,720,480,576,
	0,640,480,

	//UHD modes (incomplete)
	0,2048,1080,1152,1280,
	0,1792,1344,
	0,1800,1440,
	0,1856,1392,
	0,2160,1200,1440,
	0,2048,1536,	
	0,2256,1504,
	0,2304,1440,1728,
	0,2560,1440,
	0,2576,1450,	
	0,2732,2048,
	0,2736,1824,
	0,2880,1800,1920,	
	0,3200,1800,2400,
};

HRESULT D3D9C::IDirectDraw7::EnumDisplayModes(DWORD x, DX::LPDDSURFACEDESC2 y, LPVOID z, DX::LPDDENUMMODESCALLBACK2 w)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDraw7::EnumDisplayModes()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	if(y) DDRAW_LEVEL(7) << "PANIC! LPDDSURFACEDESC2 was passed...\n";

	DX::DDSURFACEDESC2 desc7; D3DDISPLAYMODE mode9; HRESULT out = DD_OK;

	D3DDISPLAYMODEFILTER filt; filt.Size = sizeof(D3DDISPLAYMODEFILTER);

	filt.Format = D3DFMT_X8R8G8B8; filt.ScanLineOrdering =
	DDRAW::isInterlaced?D3DSCANLINEORDERING_INTERLACED:D3DSCANLINEORDERING_PROGRESSIVE;

	UINT modes = 0; 
	
	if(D3D9C::DLL::Direct3DCreate9Ex) retry_mode_flags:
	{	
		modes = proxy9->GetAdapterModeCountEx(query9->adapter,&filt);

		if(modes==0&&!DDRAW::fullscreen) //2021: maybe desktop is the other way?
		{
			//NOTE: not actually "flags"
			//filt.ScanLineOrdering = (D3DSCANLINEORDERING)3; //PROGRESSIVE|INTERLACED
			//(int&)filt.ScanLineOrdering^=3; //UNKNOWN works too
			filt.ScanLineOrdering = D3DSCANLINEORDERING_UNKNOWN;
			modes = proxy9->GetAdapterModeCountEx(query9->adapter,&filt);
		}
	}
	else modes = proxy9->GetAdapterModeCount(query9->adapter,D3DFMT_X8R8G8B8);

	HMONITOR hm = proxy9->GetAdapterMonitor(query9->adapter);
	MONITORINFO mi = { sizeof(mi) };
	GetMonitorInfoA(hm,&mi);
	//UNIT vw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	//UINT vh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	UINT vw = mi.rcMonitor.right-mi.rcMonitor.left;
	UINT vh = mi.rcMonitor.bottom-mi.rcMonitor.top;	

	//TODO: generic custom table...
	 	
	int sd[3] = {0,0}; int client[2] = {0,0};

	if(D3D9C::doEmulate) //640x480 (required by SOM)
	if(640>=DDRAW::xyMinimum[0]&&480>=DDRAW::xyMinimum[1])
	{
		sd[0] = 640; sd[1] = 480;
	}
	
	if((int)DDRAW::resolution[0]>=DDRAW::xyMinimum[0]
	 &&(int)DDRAW::resolution[1]>=DDRAW::xyMinimum[1])
	{
		client[0] = DDRAW::resolution[0];
		client[1] = DDRAW::resolution[1];
	}

	bool order = client[0]<sd[0]
	||client[0]==sd[0]&&client[1]<sd[1];

	//2021: PLEASE COMMENT HOW THIS WORKS
	UINT customs[2][3] = 
	{
		{order?client[0]:sd[0],order?client[1]:sd[1],-1},
		{order?sd[0]:client[0],order?sd[1]:client[1],-1},
	};
	if(customs[0][0]==customs[1][0]
	 &&customs[0][1]==customs[1][1])
	{
		 customs[1][0] = customs[1][1] = 0;
	}

	typedef std::pair<DWORD,DWORD> v_t; //2023
	std::vector<v_t> v;
	v.reserve(modes+DX_ARRAYSIZEOF(dx_d3d9c_rez));

	DWORD cw,mw = 0, mh = 0;

	int prevw = -1, prevh = -1;
	for(int pass=0;pass<3;pass++) for(UINT i=0;i<modes;i++)
	{					
		int bpp = 8; if(pass>0) bpp+=8; if(pass>1) bpp+=16;
		
		if(D3D9C::DLL::Direct3DCreate9Ex)
		{	
			//WARNING (2022)
			//it seems that the first (0) mode is fullscreen now and then the
			//next one starts at low resolution... this code looks like it is
			//written to assume ascending order
			//(I don't know how I didn't see this because "custom" was making
			//1920x1080 to not display, unless today is the first day it did)

			D3DDISPLAYMODEEX mode9Ex; mode9Ex.Size = sizeof(D3DDISPLAYMODEEX);
					   
			out = proxy9->EnumAdapterModesEx(query9->adapter,&filt,i,&mode9Ex);

			mode9.Width = mode9Ex.Width; mode9.Height = mode9Ex.Height;

			mode9.RefreshRate = mode9Ex.RefreshRate; mode9.Format = mode9Ex.Format;
		}
		else  out = proxy9->EnumAdapterModes(query9->adapter,D3DFMT_X8R8G8B8,i,&mode9);
		
		if(out!=D3D_OK)	DDRAW_RETURN(!DD_OK) else out = DD_OK;

		if(mode9.Format!=D3DFMT_X8R8G8B8
		 ||mode9.Width==prevw&&mode9.Height==prevh)
		{
			if(i+1==modes) i = modes; else continue; //customs
		}

		prevw = mode9.Width; prevh = mode9.Height;
	
		//NOTE: SOM filters these away
		if(prevw>(int)vw||prevh>(int)vh) continue; //weird???

		mw = max(mw,(DWORD)prevw); mh = max(mh,(DWORD)prevh);

		v.push_back(std::make_pair(prevw,prevh)); //2023
	}

	auto *p = dx_d3d9c_rez;	
	for(int i=DX_ARRAYSIZEOF(dx_d3d9c_rez);i-->0;p++) 
	{
		if(!*p){ p++; cw = *p; continue; } 
		if(cw<=mw&&*p<=mh) v.push_back(std::make_pair(cw,*p));
	}

	auto it = v.begin(), itt = v.end();

	std::sort(it,itt); itt = std::unique(it,itt); 
	
	auto iit = it = v.begin(); v.erase(itt,v.end()); itt = v.end();

	for(int i,pass=0;pass<3;pass++) for(i=0,it=iit;it<itt;it++,i++)	
	{
		mw = it->first, mh = it->second;

		int bpp = 8; if(pass>0) bpp+=8; if(pass>1) bpp+=16;

		memset(&desc7,0x00,sizeof(DX::DDSURFACEDESC2)); //paranoia

		desc7.dwSize = sizeof(DX::DDSURFACEDESC2);
		desc7.ddpfPixelFormat.dwSize = sizeof(DX::DDPIXELFORMAT);
		desc7.dwFlags = DDSD_HEIGHT|DDSD_PITCH|DDSD_WIDTH|DDSD_REFRESHRATE|DDSD_PIXELFORMAT;		
		desc7.dwHeight = mh; desc7.dwWidth = mw;
		desc7.lPitch = mw*bpp/8; //likely assumption
		desc7.dwRefreshRate = 0; //mode9.RefreshRate;
		switch(bpp)
		{
		case 8:  desc7.ddpfPixelFormat = dx_d3d9c_format(D3DFMT_P8); break;
		case 16: desc7.ddpfPixelFormat = dx_d3d9c_format(D3DFMT_R5G6B5); break; 
		case 32: desc7.ddpfPixelFormat = dx_d3d9c_format(D3DFMT_X8R8G8B8); break;
		default: assert(0);
		}

		bool keep = false;

		if((int)mw>=DDRAW::xyMinimum[0]&&(int)mh>=DDRAW::xyMinimum[1]) 
		{					
			keep = true; 
			if(DDRAW::aspectratio=='4:3')
			if(fabs(float(mw)/float(mh)-1.333333f)>0.0001f)
			keep = false;
		   			
		    if(!DDRAW::fullscreen) //if(DDRAW::inWindow)
			{
				//WARNING: subject to system DPI setting
				//
				// som.game.cpp calls SetProcessDPIAware
				//
				////////////////////////////////////////
						
				if(vw&&vh) //see resolution fits into virtual screen space
				{
					if(mw>vw||mh>vh) keep = false;
				}
			}
		}

		custom: //hack
		for(int c=0;c<2;c++)
		if(customs[c][2]!=pass)
		if(mw!=customs[c][0]
		 ||mh!=customs[c][1]||!keep)
		{	
			//2022: I think this was for sorting, but currently
			//the results aren't in order anyway since the first
			//is fullscreen and that will cause these to be first
			//(Sword of Moonlight sorts them anyway)
			if(customs[c][0])
			if(mw>=customs[c][0]&&mh>customs[c][1]||i==modes)
			{				
				DX::DDSURFACEDESC2 desc; 

				memcpy(&desc,&desc7,sizeof(DX::DDSURFACEDESC2));
		
				desc.dwWidth = customs[c][0];	
				desc.dwHeight = customs[c][1];

				desc.lPitch = customs[c][0]*bpp/8; //likely assumption
						
				if(w(&desc,z)==DDENUMRET_CANCEL) DDRAW_RETURN(out)
				
				customs[c][2] = pass;
			}
		}
		else customs[c][2] = pass;

		if(keep&&i!=modes) //HACK
		if(w(&desc7,z)==DDENUMRET_CANCEL) 
		DDRAW_RETURN(out)

		if(i+1==modes) //HACK
		for(int c=0;c<2;c++)
		if(customs[c][2]!=pass&&customs[c][0])
		{
			i = modes; goto custom; //force include? 
		}
	}

	DDRAW_RETURN(out)
}
HRESULT D3D9C::IDirectDraw7::GetCaps(DX::LPDDCAPS_DX7 x, DX::LPDDCAPS_DX7 y)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDraw7::GetCaps()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	////////////////////////////////////////////////////////

	if(x) assert(x->dwSize==380); if(y) assert(y->dwSize==380);

	if(x&&x->dwSize!=380) DDRAW_RETURN(!D3D_OK) //todo: warning
	if(y&&y->dwSize!=380) DDRAW_RETURN(!D3D_OK) 
	
	DX::LPDDCAPS_DX7 hal = x; DX::LPDDCAPS_DX7 hel = y;

	if(!dx_d3d9c_sw()&&hel) //no software rasterizer
	{
		memset(hel,0x00,sizeof(DX::LPDDCAPS_DX7));
	}

	//this is a profile generated by DDRAW::log_driver_caps
	//taken from a Nvidia 8600M GT driver under Windows 7.
	//it would be possible to get the caps from DirectDraw
	//assuming the devices names are identical, however that
	//would put a dependency on the DirectDraw drivers, which
	//is probably not a good idea while targeting Direct3D9.
	#include "dx.ddcaps.inl"

	return D3D_OK; //////////////////////////////////////////

	D3DCAPS9 caps9; DWORD memtotal, memavail;

	MEMORYSTATUSEX sysmem;

	sysmem.dwLength = sizeof(MEMORYSTATUSEX);

	if(GlobalMemoryStatusEx(&sysmem)) //total system memory
	{
		memtotal = min(sysmem.ullTotalPageFile,(unsigned long)-1); 
		memavail = min(sysmem.ullAvailPageFile,(unsigned long)-1);
	}
	else memtotal = memavail = (unsigned long)-1; //let's just say infinite

	//warning: most everything seems irrelevant to D3D9 and it's impossible to reliably
	// instantiate a DirectDraw interface via the adapter GUID (because it's not the same)

	for(int pass=0;pass<2;pass++) 
	{
		switch(pass)
		{
		case 0: proxy9->GetDeviceCaps(query9->adapter,D3DDEVTYPE_HAL,&caps9); break;
		case 1: proxy9->GetDeviceCaps(query9->adapter,D3DDEVTYPE_SW,&caps9); break;
		}

		DX::DDCAPS_DX7 &caps7 = pass?*y:*x;

/*  0*/ //DWORD   dwSize;                 // size of the DDDRIVERCAPS structure

		if(caps7.dwSize!=sizeof(DX::DDCAPS_DX7)) DDRAW_RETURN(!DD_OK)
			   		
		memset(&caps7,0x00,sizeof(DX::DDCAPS_DX7));

		caps7.dwSize = sizeof(DX::DDCAPS_DX7);

/*  4*/ //DWORD   dwCaps;                 // driver specific capabilities

		caps7.dwCaps = caps9.Caps;  //from d3d9caps.h: /* Caps from DX7 Draw */

		if(pass) caps7.dwCaps|=DDCAPS_NOHARDWARE;
		if(!pass) caps7.dwCaps|=DDCAPS_COLORKEYHWASSIST; //assuming

		caps7.dwCaps|= //for good measure
		 DDCAPS_3D
		|DDCAPS_BLT
		|DDCAPS_BLTQUEUE
		|DDCAPS_BLTFOURCC
		|DDCAPS_BLTSTRETCH
		|DDCAPS_OVERLAY
		|DDCAPS_OVERLAYFOURCC
		|DDCAPS_OVERLAYSTRETCH
		|DDCAPS_PALETTE
		|DDCAPS_PALETTEVSYNC
		|DDCAPS_READSCANLINE
		|DDCAPS_VBI
		|DDCAPS_ZBLTS
		|DDCAPS_ZOVERLAYS
		|DDCAPS_COLORKEY
		|DDCAPS_ALPHA
		|DDCAPS_BLTCOLORFILL
		|DDCAPS_BLTDEPTHFILL
		|DDCAPS_CANCLIP
		|DDCAPS_CANCLIPSTRETCHED
		|DDCAPS_CANBLTSYSMEM;

/*  8*/ //DWORD   dwCaps2;                // more driver specific capabilites

		caps7.dwCaps2 = 0x00000000;

		if(caps9.Caps2&D3DCAPS2_CANMANAGERESOURCE) caps7.dwCaps2|=DDCAPS2_CANMANAGERESOURCE;

		caps7.dwCaps2 = caps9.Caps2; //should be safe / might provide undocumented flags for now

		caps7.dwCaps2|= //for good measure
		 DDCAPS2_COLORCONTROLPRIMARY
		|DDCAPS2_PRIMARYGAMMA
		|DDCAPS2_CANRENDERWINDOWED
		|DDCAPS2_CANCALIBRATEGAMMA
		|DDCAPS2_FLIPINTERVAL
		|DDCAPS2_FLIPNOVSYNC
		|DDCAPS2_CANMANAGETEXTURE
		|DDCAPS2_CANMANAGERESOURCE;
//		|DDCAPS2_DYNAMICTEXTURES
//		|DDCAPS2_CANAUTOGENMIPMAP  //introduced in DirectX9

/*  c*/ //DWORD   dwCKeyCaps;             // color key capabilities of the surface

		caps7.dwCKeyCaps = //attributing all beneficial caps for now

		 DDCKEYCAPS_DESTBLT|DDCKEYCAPS_DESTBLTCLRSPACE
		|DDCKEYCAPS_DESTBLTCLRSPACEYUV|DDCKEYCAPS_DESTBLTYUV
		|DDCKEYCAPS_DESTOVERLAY|DDCKEYCAPS_DESTOVERLAYCLRSPACE
		|DDCKEYCAPS_DESTOVERLAYCLRSPACEYUV|DDCKEYCAPS_DESTOVERLAYYUV
		|DDCKEYCAPS_SRCBLT|DDCKEYCAPS_SRCBLTCLRSPACE
		|DDCKEYCAPS_SRCBLTCLRSPACEYUV|DDCKEYCAPS_SRCBLTYUV
		|DDCKEYCAPS_SRCOVERLAY|DDCKEYCAPS_SRCOVERLAYCLRSPACE
		|DDCKEYCAPS_SRCOVERLAYCLRSPACEYUV|DDCKEYCAPS_SRCOVERLAYYUV;
//		|DDCKEYCAPS_NOCOSTOVERLAY;		

/* 10*/ //DWORD   dwFXCaps;               // driver specific stretching and effects capabilites

		caps7.dwFXCaps = //attributing all beneficial caps for now

		 DDFXCAPS_BLTMIRRORLEFTRIGHT|DDFXCAPS_BLTMIRRORUPDOWN
		|DDFXCAPS_BLTROTATION
		|DDFXCAPS_BLTSHRINKX|DDFXCAPS_BLTSHRINKY
		|DDFXCAPS_BLTSTRETCHX|DDFXCAPS_BLTSTRETCHY
		|DDFXCAPS_OVERLAYSHRINKX|DDFXCAPS_OVERLAYSHRINKY
		|DDFXCAPS_OVERLAYSTRETCHX|DDFXCAPS_OVERLAYSTRETCHY
		|DDFXCAPS_OVERLAYMIRRORLEFTRIGHT|DDFXCAPS_OVERLAYMIRRORUPDOWN
		|DDFXCAPS_OVERLAYDEINTERLACE
		|DDFXCAPS_BLTALPHA|DDFXCAPS_OVERLAYALPHA;

/* 14*/ //DWORD   dwFXAlphaCaps;          // alpha driver specific capabilities
		
		caps7.dwFXAlphaCaps = //attributing all beneficial caps for now

		 DDFXALPHACAPS_BLTALPHAEDGEBLEND
		|DDFXALPHACAPS_BLTALPHAPIXELS|DDFXALPHACAPS_BLTALPHAPIXELSNEG
		|DDFXALPHACAPS_BLTALPHASURFACES|DDFXALPHACAPS_BLTALPHASURFACESNEG
		|DDFXALPHACAPS_OVERLAYALPHAEDGEBLEND
		|DDFXALPHACAPS_OVERLAYALPHAPIXELS|DDFXALPHACAPS_OVERLAYALPHAPIXELSNEG
		|DDFXALPHACAPS_OVERLAYALPHASURFACES|DDFXALPHACAPS_OVERLAYALPHASURFACESNEG;

/* 18*/ //DWORD   dwPalCaps;              // palette capabilities

		caps7.dwPalCaps = DDPCAPS_8BIT|DDPCAPS_ALLOW256|DDPCAPS_ALPHA;

/* 1c*/ //DWORD   dwSVCaps;               // stereo vision capabilities

		caps7.dwSVCaps = 0x00000000; //3D goggles not included

/* 20*/ //DWORD   dwAlphaBltConstBitDepths;       // DDBD_2,4,8
/* 24*/ //DWORD   dwAlphaBltPixelBitDepths;       // DDBD_1,2,4,8
/* 28*/ //DWORD   dwAlphaBltSurfaceBitDepths;     // DDBD_1,2,4,8
/* 2c*/ //DWORD   dwAlphaOverlayConstBitDepths;   // DDBD_2,4,8
/* 30*/ //DWORD   dwAlphaOverlayPixelBitDepths;   // DDBD_1,2,4,8
/* 34*/ //DWORD   dwAlphaOverlaySurfaceBitDepths; // DDBD_1,2,4,8
/* 38*/ //DWORD   dwZBufferBitDepths;             // DDBD_8,16,24,32

		caps7.dwAlphaBltConstBitDepths = DDBD_16|DDBD_24|DDBD_32;
		caps7.dwAlphaBltPixelBitDepths = DDBD_16|DDBD_24|DDBD_32;
		caps7.dwAlphaBltSurfaceBitDepths = DDBD_16|DDBD_24|DDBD_32;
		caps7.dwAlphaOverlayConstBitDepths = DDBD_16|DDBD_24|DDBD_32;
		caps7.dwAlphaOverlayPixelBitDepths = DDBD_16|DDBD_24|DDBD_32;
		caps7.dwAlphaOverlaySurfaceBitDepths = DDBD_16|DDBD_24|DDBD_32;

		//TODO: actually query this 
		caps7.dwZBufferBitDepths = DDBD_16; //DDBD_32;																	
		if(!pass) caps7.dwZBufferBitDepths|=DDBD_24; //just assuming

/* 3c*/ //DWORD   dwVidMemTotal;          // total amount of video memory
/* 40*/ //DWORD   dwVidMemFree;           // amount of free video memory

		caps7.dwVidMemTotal = memtotal;
		caps7.dwVidMemFree = memavail;

/* 44*/ //DWORD   dwMaxVisibleOverlays;   // maximum number of visible overlays
/* 48*/ //DWORD   dwCurrVisibleOverlays;  // current number of visible overlays
/* 4c*/ //DWORD   dwNumFourCCCodes;       // number of four cc codes
/* 50*/ //DWORD   dwAlignBoundarySrc;     // source rectangle alignment
/* 54*/ //DWORD   dwAlignSizeSrc;         // source rectangle byte size
/* 58*/ //DWORD   dwAlignBoundaryDest;    // dest rectangle alignment
/* 5c*/ //DWORD   dwAlignSizeDest;        // dest rectangle byte size
/* 60*/ //DWORD   dwAlignStrideAlign;     // stride alignment
/* 64*/ //DWORD   dwRops[DD_ROP_SPACE];   // ROPS supported
/* 84*/ //DDSCAPS ddsOldCaps;             // Was DDSCAPS  ddsCaps. ddsCaps is of type DDSCAPS2 for DX6
/* 88*/ //DWORD   dwMinOverlayStretch;    // minimum overlay stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
/* 8c*/ //DWORD   dwMaxOverlayStretch;    // maximum overlay stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
/* 90*/ //DWORD   dwMinLiveVideoStretch;  // minimum live video stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
/* 94*/ //DWORD   dwMaxLiveVideoStretch;  // maximum live video stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
/* 98*/ //DWORD   dwMinHwCodecStretch;    // minimum hardware codec stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
/* 9c*/ //DWORD   dwMaxHwCodecStretch;    // maximum hardware codec stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
/* a0*/ //DWORD   dwReserved1;            // reserved
/* a4*/ //DWORD   dwReserved2;            // reserved
/* a8*/ //DWORD   dwReserved3;            // reserved
/* ac*/ //DWORD   dwSVBCaps;              // driver specific capabilities for System->Vmem blts
/* b0*/ //DWORD   dwSVBCKeyCaps;          // driver color key capabilities for System->Vmem blts
/* b4*/ //DWORD   dwSVBFXCaps;            // driver FX capabilities for System->Vmem blts
/* b8*/ //DWORD   dwSVBRops[DD_ROP_SPACE];// ROPS supported for System->Vmem blts
/* d8*/ //DWORD   dwVSBCaps;              // driver specific capabilities for Vmem->System blts
/* dc*/ //DWORD   dwVSBCKeyCaps;          // driver color key capabilities for Vmem->System blts
/* e0*/ //DWORD   dwVSBFXCaps;            // driver FX capabilities for Vmem->System blts
/* e4*/ //DWORD   dwVSBRops[DD_ROP_SPACE];// ROPS supported for Vmem->System blts
/*104*/ //DWORD   dwSSBCaps;              // driver specific capabilities for System->System blts
/*108*/ //DWORD   dwSSBCKeyCaps;          // driver color key capabilities for System->System blts
/*10c*/ //DWORD   dwSSBFXCaps;            // driver FX capabilities for System->System blts
/*110*/ //DWORD   dwSSBRops[DD_ROP_SPACE];// ROPS supported for System->System blts
/*130*/ //DWORD   dwMaxVideoPorts;        // maximum number of usable video ports
/*134*/ //DWORD   dwCurrVideoPorts;       // current number of video ports used
/*138*/ //DWORD   dwSVBCaps2;             // more driver specific capabilities for System->Vmem blts
/*13c*/ //DWORD   dwNLVBCaps;               // driver specific capabilities for non-local->local vidmem blts
/*140*/ //DWORD   dwNLVBCaps2;              // more driver specific capabilities non-local->local vidmem blts
/*144*/ //DWORD   dwNLVBCKeyCaps;           // driver color key capabilities for non-local->local vidmem blts
/*148*/ //DWORD   dwNLVBFXCaps;             // driver FX capabilities for non-local->local blts
/*14c*/ //DWORD   dwNLVBRops[DD_ROP_SPACE];
	}

	return DD_OK;
}
HRESULT D3D9C::IDirectDraw7::SetCooperativeLevel(HWND x, DWORD y)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDraw7::SetCooperativeLevel()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	DDRAW_PUSH_HACK(DIRECTDRAW7_SETCOOPERATIVELEVEL,IDirectDraw7*,
	HWND&,DWORD&)(0,this,x,y);

	DDRAW_LEVEL(7) << "HWND is " << int(x) << '\n';
	
	DDRAW::window = x;  //capture window	

	out = DD_OK;

	DDRAW_POP_HACK(DIRECTDRAW7_SETCOOPERATIVELEVEL,IDirectDraw7*,
	HWND&,DWORD&)(&out,this,x,y);

	DDRAW_RETURN(out);
}
HRESULT D3D9C::IDirectDraw7::WaitForVerticalBlank(DWORD x, HANDLE y)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDraw7::WaitForVerticalBlank()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	HRESULT out = !DD_OK; if(!query9->vblank) 
	{
		if(x==DDWAITVB_BLOCKBEGIN
		&&D3D9C::DLL::Direct3DCreate9Ex&&DDRAW::Direct3DDevice7)
		{
			out = DDRAW::Direct3DDevice7->proxy9->WaitForVBlank(0);
		}
		else query9->vblank = DDRAW::vblank();
	}
	if(out) out = query9->vblank->WaitForVerticalBlank(x,y);
	
	DDRAW_RETURN(out);
}
HRESULT D3D9C::IDirectDrawPalette::QueryInterface(REFIID riid, LPVOID FAR *ppvObj)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDrawPalette::QueryInterface()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';
	
	DDRAW_RETURN(!S_OK);
}
ULONG D3D9C::IDirectDrawPalette::AddRef()
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDrawPalette::AddRef()\n";

	DDRAW_ADDREF_RETURN(0)
}
ULONG D3D9C::IDirectDrawPalette::Release()
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDrawPalette::Release()\n";

	DDRAW_IF_NOT_TARGET_AND_RELEASE(out,0)

	if(out>0) 
	{
		DDRAW_LEVEL(7) << out; return out;
	}
	
	if(palette9>0&&palette9<256)
	{
		dx_d3d9c_palettes[palette9] = false;
	}
	else assert(0);

	delete this; 
	
	return 0;
}
/*SetEntries: stated implementing this on accident (not actually used)
HRESULT D3D9C::IDirectDrawPalette::SetEntries(DWORD x, DWORD y, DWORD z, LPPALETTEENTRY w)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDrawPalette::SetEntries()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	if(!w||!z) DDRAW_RETURN(!DD_OK) assert(!x); //docs say must be zero

	if(!DDRAW::Direct3DDevice7) return false; //todo: backup palette

	//TODO: track D3DPTEXTURECAPS_ALPHAPALETTE capability

	::IDirect3DDevice9 *d3dd9 = DDRAW::Direct3DDevice7->proxy9;

	if(palette==0||palette9>256||!dx_d3d9c_palettes[palette9]) DDRAW_RETURN(!DD_OK)

	HRESULT out = !D3D_OK;

	if(y>0||z!=256) //complex case
	{
		assert(0);
	}
	else out = d3dd9->SetPaletteEntries(palette9,w);

	DDRAW_RETURN(out==D3D_OK?DD_OK:!DD_OK)
}*/									






HRESULT D3D9C::IDirectDrawGammaControl::GetGammaRamp(DWORD x, DX::LPDDGAMMARAMP y)
{ 
	DDRAW_LEVEL(7) << "D3D9C::IDirectDrawGammaControl::GetGammaRamp()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK) if(!y) return !DD_OK;

	//Why was there 3 exactly? //???
	//if(DDRAW::doGamma&&!DDRAW::inWindow&&DDRAW::isExclusive)
	if(DDRAW::doGamma&&DDRAW::fullscreen
	&&target=='dx9c') //2021
	{
		proxy9->GetGammaRamp(0,(D3DGAMMARAMP*)y);
	}
	else memcpy(y,gammaramp,sizeof(DX::DDGAMMARAMP));

	return DD_OK; 
}
HRESULT D3D9C::IDirectDrawGammaControl::SetGammaRamp(DWORD x, DX::LPDDGAMMARAMP y) 
{ 
	DDRAW_LEVEL(7) << "D3D9C::IDirectDrawGammaControl::SetGammaRamp()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)	if(!y) return !DD_OK;

	DDRAW_PUSH_HACK(DIRECTDRAWGAMMACONTROL_SETGAMMARAMP,IDirectDrawGammaControl*,
	DWORD&,DX::LPDDGAMMARAMP&)(0,this,x,y);

	memcpy(gammaramp,y,sizeof(DX::DDGAMMARAMP));

	y = gammaramp; //for push/pop (just in case)

	if(target=='dx9c') //2021
	//Why was there 3 exactly? //???
	//if(!DDRAW::inWindow&&DDRAW::isExclusive)
	if(DDRAW::doGamma&&DDRAW::fullscreen)
	{
		//warning! this will set the gamma ramp for the entire screen
		//todo: implement documentation's windowed mode suggestions
		proxy9->SetGammaRamp(0,D3DSGR_CALIBRATE,(D3DGAMMARAMP*)y);
	}
	else dx_d3d9c_gammaramp(DDRAW::Direct3DDevice7->query9->gamma[1],(D3DGAMMARAMP*)y);

	out = DD_OK;

	DDRAW_POP_HACK(DIRECTDRAWGAMMACONTROL_SETGAMMARAMP,IDirectDrawGammaControl*,
	DWORD&,DX::LPDDGAMMARAMP&)(&out,this,x,y);

	return out;
}






HRESULT D3D9C::IDirectDrawSurface::QueryInterface(REFIID riid, LPVOID FAR *ppvObj)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDrawSurface::QueryInterface()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';

	DDRAW_PUSH_HACK(DIRECTDRAWSURFACE_QUERYINTERFACE,IDirectDrawSurface*,
	REFIID,LPVOID*&)(0,this,riid,ppvObj);
						   	
	DDRAW_POP_HACK(DIRECTDRAWSURFACE_QUERYINTERFACE,IDirectDrawSurface*,
	REFIID,LPVOID*&)(&out,this,riid,ppvObj);

	if(out!=S_OK) DDRAW_LEVEL(7) << "QueryInterface FAILED\n";

	DDRAW_RETURN(out)
}
ULONG D3D9C::IDirectDrawSurface::AddRef()
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDrawSurface::AddRef()\n";

	DDRAW_IF_NOT_TARGET_RETURN(0)

	if(query9->group9==dx_d3d9c_virtual_group) return 1; //todo: ref counting

	DDRAW_ADDREF_RETURN(0)
}
ULONG D3D9C::IDirectDrawSurface::Release()
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDrawSurface::Release()\n";

	DDRAW_IF_NOT_TARGET_AND_RELEASE(out,0)

	assert(query->dx7); //assuming this is a view of a 7 surface							

  	if(query->isPrimary)
	{
		if(out==0) delete this; 
		//if(out==0) dx_ddraw_try_primarysurface7_release();
	}
	else if(out==0) delete this; 

	return out;
}
HRESULT D3D9C::IDirectDrawSurface::Blt(LPRECT x, DX::LPDIRECTDRAWSURFACE y, LPRECT z, DWORD w, DX::LPDDBLTFX q)
{ 
	DDRAW_LEVEL(2) << "D3D9C::IDirectDrawSurface::Blt()\n";

	DDRAW_IF_NOT_DX9C_RETURN(!DD_OK)
		
	DDRAW_RETURN(DDRAW_UNIMPLEMENTED);
}







HRESULT D3D9C::IDirectDrawSurface7::QueryInterface(REFIID riid, LPVOID FAR *ppvObj)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDrawSurface7::QueryInterface()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';

	DDRAW_PUSH_HACK(DIRECTDRAWSURFACE7_QUERYINTERFACE,IDirectDrawSurface7*,
	REFIID,LPVOID*&)(0,this,riid,ppvObj);
	
	if(!ppvObj) DDRAW_FINISH(!S_OK)

	if(riid==IID_IDirectDrawGammaControl) //{69C11C3E-B46B-11D1-AD7A-00C04FC29B4E}
	{
		DDRAW_LEVEL(7) << "(IDirectDrawGammaControl)\n";

		//todo: save ramp for later (CreateDeviceEx)
		if(!DDRAW::Direct3DDevice7) DDRAW_FINISH(!S_OK); 

		if(query9->gamma)			
		{
			*ppvObj = query9->gamma; query9->gamma->AddRef(); 
			
			DDRAW_FINISH(S_OK);
		}

		DDRAW::IDirectDrawGammaControl *p = new DDRAW::IDirectDrawGammaControl(dx95);

		p->proxy9 = DDRAW::Direct3DDevice7->proxy9; query->gamma = p; p->surface = query;
		
		*ppvObj = p; DDRAW::Direct3DDevice7->proxy9->AddRef();

		DDRAW_FINISH(S_OK);
	}
	else if(riid==IID_IDirectDrawSurface) //(6C14DB81-A733-11CE-A521-0020AF0BE560)
	{
		DDRAW_LEVEL(7) << "(IDirectDrawSurface)\n";

		DDRAW::IDirectDrawSurface *p = new DDRAW::IDirectDrawSurface(dx95,query);

		*ppvObj = p; p->proxy9 = proxy9; AddRef();

		DDRAW_FINISH(S_OK);
	}
	
	DDRAW_POP_HACK(DIRECTDRAWSURFACE7_QUERYINTERFACE,IDirectDrawSurface7*,
	REFIID,LPVOID*&)(&out,this,riid,ppvObj);

	if(out!=S_OK) DDRAW_LEVEL(7) << "QueryInterface FAILED\n";

	DDRAW_RETURN(out)
}
ULONG D3D9C::IDirectDrawSurface7::AddRef()
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDrawSurface7::AddRef()\n";

	DDRAW_IF_NOT_TARGET_RETURN(0)

	DDRAW_ADDREF_RETURN(0)
}
ULONG D3D9C::IDirectDrawSurface7::Release()
{								
	DDRAW_LEVEL(7) << "D3D9C::IDirectDrawSurface7::Release()\n";

	DDRAW_LEVEL(7) << ' ' << (unsigned)this << '\n';

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
		if(query9->group9==dx_d3d9c_virtual_group)		
		{
			if(DDRAW::PrimarySurface7==this)			
			DDRAW::PrimarySurface7 = 0;
			if(DDRAW::BackBufferSurface7==this)
			DDRAW::BackBufferSurface7 = 0;
			if(DDRAW::DepthStencilSurface7==this) //2021
			DDRAW::DepthStencilSurface7 = 0;
		
			for(int i=0;i<dx_d3d9c_direct3d_swapN;i++)
			for(int i=0;i<dx_d3d9c_direct3d_swapN;i++)
			for(int i=0;i<dx_d3d9c_direct3d_swapN;i++)
			{
				if(dx_d3d9c_direct3d_swapsurfs[i]==this) 
				{
					dx_d3d9c_direct3d_swapsurfs[i] = 0;
				}
			}
		}
		else if(query9->group9) query9->group9->Release();

		if(queryX->update)
		if(target=='dx9c')
		{
			 query9->update9->Release();			
		}
		else queryX->_updateGL_delete();

		//2021: operator~ calls assert
		queryX->group = 0; queryX->update = 0; 

		delete this;
	}
	return out;
}
HRESULT D3D9C::IDirectDrawSurface7::AddAttachedSurface(DX::LPDIRECTDRAWSURFACE7 x)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDrawSurface7::AddAttachedSurface()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	auto p = DDRAW::is_proxy<IDirectDrawSurface7>(x);

	if(query9->group9==dx_d3d9c_virtual_group&&p->query9->group9==query9->group9)
	{
		if(!DDRAW::Direct3DDevice7) return DD_OK; else assert(0); //may wanna think about this
	}	

	//unimplemented: just focusing on swapchain setup for now
		
	assert(0); return !DD_OK; /*

	if(x) x = DDRAW::is_proxy<IDirectDrawSurface7>(x)->proxy9;
		
	return proxy9->AddAttachedSurface(x);	
*/
}
HRESULT D3D9C::IDirectDrawSurface7::DeleteAttachedSurface(DWORD x, DX::LPDIRECTDRAWSURFACE7 y)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDrawSurface7::DeleteAttachedSurface()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	assert(0); return !DD_OK; /*

	if(y) y = DDRAW::is_proxy<IDirectDrawSurface7>(y)->proxy9;

	return proxy9->DeleteAttachedSurface(x,y);
*/
}
HRESULT D3D9C::IDirectDrawSurface7::BltFast(DWORD x, DWORD y, DX::LPDIRECTDRAWSURFACE7 z, LPRECT w, DWORD q)
{
	DDRAW_LEVEL(2) << "D3D9C::IDirectDrawSurface7::BltFast()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	//Note: this interface is routed thru Blt therefore it
	//should not implement its own PUSH/POP_HACK framework
	
	DWORD flags = 0x00000000;

	if(q&DDBLTFAST_WAIT)         flags|=DDBLT_WAIT;
	if(q&DDBLTFAST_DESTCOLORKEY) flags|=DDBLT_KEYDEST;
	if(q&DDBLTFAST_SRCCOLORKEY)  flags|=DDBLT_KEYSRC;

	RECT dest = { x, y, 0, 0 };

	auto p = DDRAW::is_proxy<IDirectDrawSurface7>(z);

	if(!p) DDRAW_RETURN(!DD_OK)

	RECT src; if(!w)
	{
		src.left = src.top = 0;
		src.right = p->queryX->width; src.bottom = p->queryX->height;
		w = &src;
	}

	dest.right = dest.left+(w->right-w->left);
	dest.bottom = dest.top+(w->bottom-w->top);

	return Blt(&dest,z,w,flags,0);
}
HRESULT D3D9C::IDirectDrawSurface7::Blt(LPRECT x, DX::LPDIRECTDRAWSURFACE7 y, LPRECT z, DWORD w, DX::LPDDBLTFX q)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDrawSurface7::Blt()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK) //DX9X SUBROUTINE

	if(!DDRAW::Direct3DDevice7) 
	{
		//NOTE: technically a 3d device isn't required to 
		//use DirectDraw's interfaces
		assert(0); return DDRAW_UNIMPLEMENTED;
	}

	DDRAW_PUSH_HACK(DIRECTDRAWSURFACE7_BLT,IDirectDrawSurface7*,
	LPRECT&,DX::LPDIRECTDRAWSURFACE7&,LPRECT&,DWORD&,DX::LPDDBLTFX&)(0,this,x,y,z,w,q);

	if(x&&IsRectEmpty(x)) return DD_OK; //hack: allow short circuiting

	DDRAW_LEVEL(7) << "source is " << (UINT)y << "(" << (UINT)z << ")\n";

	if(DDRAWLOG::debug>=7)
	{
	#define OUT(X) if(w&X) DDRAW_LEVEL(7) << ' ' << #X << '\n';

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
	
		DWORD *dwFx = 0;

	#define DWORD_(x) { DDRAW_LEVEL(7) << #x " is " << *(dwFx=&q->x) << "\n"; }

	#define DWORD_AND(x) if(*dwFx&x){ DDRAW_LEVEL(3) << #x "\n"; }

		if(q)
		{
		DDRAW_LEVEL(7) << "PANIC! LPDDBLTFX was passed...\n";

		#include "log.ddbltfx.inl"
		}

	#undef DWORD_
	#undef DWORD_AND
	}

	IDirect3DDevice9Ex *d3dd9 = DDRAW::Direct3DDevice7->proxy9;

	auto p = DDRAW::is_proxy<IDirectDrawSurface7>(y);
	if(!p) 
	{
		if(y) //is_proxy?
		{
			out = !DD_OK;
		}
		else if(w&DDBLT_COLORFILL)
		{
			if(!q)
			{
				out = !DD_OK;
			}
			else if(query9->group9==dx_d3d9c_virtual_group)
			{
				assert(0); //unimplemented

				out = DDRAW_UNIMPLEMENTED;			
			}
			else //2021: what used this? som_status_automap? 
			{
				D3DSURFACE_DESC desc;
				proxy9->GetDesc(&desc);
				if(desc.Usage&D3DUSAGE_RENDERTARGET) //???
				{
					assert(0); //theoretical?
					assert(!query9->update9);
					out = d3dd9->ColorFill(proxy9,x,q->dwFillColor);					
				}
				else if(!query9->isLocked) //will have to do so manually
				{
					out = !dx_d3d9c_colorfill(proxy9,x,q->dwFillColor);
					dirtying_texture(); //2021
				}
				else assert(0); //2021
			}
		}
		else if(w&(DDBLT_DEPTHFILL|DDBLT_ROP))
		{
			out = DDRAW_UNIMPLEMENTED;
		}
		else out = !DD_OK;

		DDRAW_FINISH(out)
	}
	
	if(p->query9->isPrimary) 
	DDRAW_LEVEL(2) << " Source is primary surface\n"; //???

	RECT dest,src; if(!query9->isPrimary)
	{	
		/*REMOVE ME?
		enum{ colorkey2=DDBLT_KEYDEST|DDBLT_KEYDESTOVERRIDE|DDBLT_KEYSRC|DDBLT_KEYSRCOVERRIDE };
		if(0==(w&colorkey2)) //IDirect3DDevice9::StretchRect does not support colorkey
		{
			//note: this is almost never going to be between compatible surfaces
			if(query9->group9==dx_d3d9c_virtual_group)
			{
				IDirect3DSurface9 *bbuffer = 0;

				if(d3dd9->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&bbuffer)==D3D_OK)
				{
					out = d3dd9->StretchRect(p->proxy9,z,bbuffer,x,D3DTEXF_LINEAR);		

					bbuffer->Release();
				}
			}
			else out = d3dd9->StretchRect(p->proxy9,z,proxy9,x,D3DTEXF_LINEAR);		
		}
		else*/
		{
			if(!x)
			{
				dest.left = dest.top = 0;
				dest.right = queryX->width; dest.bottom = queryX->height;
				x = &dest;
			}
			if(!z) 
			{
				src.left = src.top = 0;
				src.right = p->queryX->width; src.bottom = p->queryX->height;
				z = &src;
			}

			if(query9->group9==dx_d3d9c_virtual_group) //assuming blitting to back buffer
			{
				//DDRAW_PUSH_HACK extension
				int abe = w&DDBLT_ALPHASRCCONSTOVERRIDE?1:0;

				if(dx_d3d9c_prepareblt(p->query9->knockouts,abe)) //colorkey
				{
					D3D9C::updating_texture(p,false);
					IDirect3DTexture9 *texture = p->query9->update9;					
					if(!texture)
					out = p->proxy9->GetContainer(__uuidof(IDirect3DTexture9),(void**)&texture);										
					else out = D3D_OK;
						
					if(out==D3D_OK)
					{
						if(p->query9->palette) 
						d3dd9->SetCurrentTexturePalette(p->query9->palette);

						auto ps = dx_d3d9c_pshaders_noclip[DDRAW::ps];

						if(auto vs=dx_d3d9c_vshaders[DDRAW::vs])
						{
							dx_d3d9c_vstransformandlight();
							d3dd9->SetVertexShader(vs);
						}						
						else if(!ps)
						{
							ps = //colorkey?
							dx_d3d9c_colorkeyenable&&p->query9->knockouts?
							dx_d3d9c_colorkeyshader:dx_d3d9c_defaultshader;
						}

						//can use dx_d3d9c_pshaders_noclip?
						//if(DDRAW::psColorkey)
						//dx_d3d9X_psconstant(DDRAW::psColorkey,p->query9->knockouts?1ul:0ul,'w'); //colorkey
							
						if(abe) //DDRAW_PUSH_HACK extension
						{
							assert(8==q->dwAlphaDestConstBitDepth);							
							abe = q->dwAlphaSrcConst<<24|0xFFFFFF;
						}
						else abe = 0xFFFFFFFF;
						dx_d3d9c_backblt(texture,*z,*x,ps,DDRAW::vs,abe);

						if(!p->query9->update9) texture->Release();
					}

					if(!dx_d3d9c_cleanupblt()) assert(0); //warning: potentially disastrous
				}
				else out = !D3D_OK; //2021
			}
			else DDRAW_FINISH(DDRAW_UNIMPLEMENTED)
		}		
		out = out==D3D_OK?DD_OK:!DD_OK; //hack! //???
	}
	else out = D3D9C::IDirectDrawSurface7::flip(); //a subroutine of Flip

	if(out!=D3D_OK) DDRAW_LEVEL(2) << "Blt FAILED\n";	

	DDRAW_POP_HACK(DIRECTDRAWSURFACE7_BLT,IDirectDrawSurface7*,
	LPRECT&,DX::LPDIRECTDRAWSURFACE7&,LPRECT&,DWORD&,DX::LPDDBLTFX&)(&out,this,x,y,z,w,q);
	
	DDRAW_RETURN(out)
}
HRESULT D3D9C::IDirectDrawSurface7::flip()
{
	//2021: not really okay but WM_PAINT is likely causing
	//a cascade of assert message box windows :(
	if(DDRAW::midFlip) 
	{
		return D3D_OK; //breakpoint
	}
	else DDRAW::midFlip = true; ////POINT-OF-NO-RETURN////

	HRESULT out = !DD_OK;

	assert(DDRAW::Direct3DDevice7);
	assert(DDRAW::Direct3DDevice7->target=='dx9c');

	//detecting fx9_effects_buffers_require_reevaluation??
	auto cmp = DDRAW::Direct3DDevice7->query9->effects[0];

	//WARNING: what if fx9_effects_buffers_require_reevaluation is
	//called inside?
	DDRAW::onEffects();

	auto* &p = DDRAW::Direct3DDevice7->query9->effects[0];

	if(DDRAW::isPaused&&!DDRAW::onPause()||p!=cmp) //2021
	{
		//hack: when unpausing in menus sometimes the lights go awry
		DDRAW::noFlips++; 
		
		//DDRAW::midFlip?
		out = D3D_OK; goto out; //DDRAW_RETURN(D3D_OK);
	}
	
	IDirect3DDevice9Ex *d3dd9 = DDRAW::Direct3DDevice7->proxy9;

	if(!p||!dx_d3d9c_effectsshader_enable)
	{
		if(DDRAW::onFlip())
		out = dx_d3d9c_present(d3dd9);
		else out = S_OK;
	}
	else 
	{
		//2018: THIS IS VERY CONFUSING, BUT WHEN THE VIEWPORT IS SMALLER 
		//THAN THE RENDER-TARGET THE vs/psPresentState NEED TO BE EITHER
		//IN VIEWPORT OR RENDER-TARGET SPACE RESPECTIVELY			
		D3DVIEWPORT9 vp; d3dd9->GetViewport(&vp);
		D3DSURFACE_DESC desc; p->GetLevelDesc(0,&desc); 
		const float r = desc.Width, t = desc.Height;
				
		//VIEWPORT space (do_aa/DAA)
		//REMINDER: THE FIRST FRAME WILL BE GARBAGE
		//(dejagrate_update_vsPresentState is provided in case it needs to
		//be cleaned up)
		if(DDRAW::dejagrate>1) //3D "anti-aliasing"		
		dx_d3d9c_update_vsPresentState(vp);
		DDRAW::dejagrate_update_psPresentState();
  
		IDirect3DSurface9 *rt = 0;
		//nice: do swap in logical order for fx2ndSceneBuffer 
		bool nice = DDRAW::Direct3DDevice7->query9->effects[1]?1:0;
		if(DDRAW::Direct3DDevice7->query9->effects[nice]->GetSurfaceLevel(0,&rt))
		{
			//REMOVE ME? 
			//2021: THIS SEEMS UNPROFESSIONAL SINCE IT DEGRADES THE PRODUCT
			//dx_d3d9c_discardeffects(); DDRAW_RETURN(DDRAW_UNIMPLEMENTED);
			assert(0);
		}

		if(DDRAW::Direct3DDevice7->query9->multisample)
		{
			d3dd9->SetDepthStencilSurface(0); 
			d3dd9->StretchRect(DDRAW::Direct3DDevice7->query9->multisample,0,rt,0,D3DTEXF_POINT);
			rt->Release(); rt = DDRAW::Direct3DDevice7->query9->multisample; 
		}

		IDirect3DSurface9 *bbuffer = 0;
		if(d3dd9->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&bbuffer)==D3D_OK)
		{					
			//bbuffer->GetDesc(&desc);
			//float w = desc.Width, h = desc.Height;
			int w = DDRAW::PrimarySurface7->queryX->width;
			int h = DDRAW::PrimarySurface7->queryX->height;
			assert(w&&h);
			float ww = (float)w, hh = (float)h;
			DDRAW::doSuperSamplingMul(ww,hh);

			for(DWORD i=0;i<3;i++) //MRT: assuming desirable
			if(DDRAW::Direct3DDevice7->query9->mrtargets[i])				
			d3dd9->SetRenderTarget(1+i,0);
				
			//NOTE: SetRenderTarget automatically 
			//resets the viewport
			out = d3dd9->SetRenderTarget(0,bbuffer);

			bool matte = false;
			if(dx_d3d9c_prepareblt())
			{	
				/*2021: trying this for AA corrective effect
				//REMOVE ME: this is for stereo demos for now
				if(DDRAW::doMipSceneBuffers&&DDRAW::inStereo) //NEEDS WORK*/
				for(int i=0;i<(int)DDRAW::doMipSceneBuffers;i++)
				{
					auto pj = p; //caching?

					//Reminder: D3DUSAGE_AUTOGENMIPMAP is glitchy on
					//my Intel Iris Pro workstation... maybe there's
					//not enough memory, or the drivers don't really
					//support it
					IDirect3DSurface9 *lv0 = 0,*lv1 = 0;
					pj->GetSurfaceLevel(i,&lv0);
					pj->GetSurfaceLevel(i+1,&lv1);					
					if(DDRAW::xyMapping2[0]&&i==DDRAW::doMipSceneBuffers-1)
					{
						//FYI: this is a complicated effect to shift the dissolved mipmaps
						//over 1px on the odd frames so that the apparent resolution is 2x
						int rr = r, tt = t;
						if(i){ rr/=2; tt/=2; }; assert(i<2);
						RECT src = {1,1,rr-1,tt-1};
						RECT dst = {0,0,rr/2-1,tt/2-1};
						HRESULT test = d3dd9->StretchRect(lv0,&src,lv1,&dst,D3DTEXF_LINEAR);
						assert(!test);
					}
					else
					{
						HRESULT test = d3dd9->StretchRect(lv0,0,lv1,0,D3DTEXF_LINEAR);
						assert(!test);
					}
					lv0->Release(); lv1->Release();
				}					
				
				//DDRAW::do2ndSceneBuffer?
				if(DDRAW::Direct3DDevice7->query9->effects[1])
				{
					IDirect3DTexture9 *pp =
					DDRAW::Direct3DDevice7->query9->effects[DDRAW::fx2ndSceneBuffer];
					d3dd9->SetTexture(1,pp);	

					if(DDRAW::ps2ndSceneMipmapping2) //EXPERIMENTAL
					if(int i=DDRAW::doMipSceneBuffers)
					{
						//I'm pretty sure rcp should be 2 here but 1.5 or 1.75
						//looks better to me... there are hard to account for 
						//blending/contrast artifacts in the antialiased edges
						//float rcp = i==2?-2:-1;
						float rcp = i==2?-1.66f:-1;
						float c[4] = {rcp/r,rcp/t};

						if(DDRAW::xyMapping2[0!=DDRAW::fx2ndSceneBuffer])
						{
							c[2] = c[0]; c[3] = c[1]; 
						}
						if(!DDRAW::xyMapping2[0]) 
						{
							c[0] = c[1] = 0;
						}
						dx_d3d9X_psconstant(DDRAW::ps2ndSceneMipmapping2,c,'xyzw');						
					}

					DDRAW::fx2ndSceneBuffer = 1;
				}
				else if(DDRAW::doGamma)	
				d3dd9->SetTexture(1,DDRAW::Direct3DDevice7->query9->gamma[!DDRAW::fullscreen/*DDRAW::inWindow*/]);
	
				IDirect3DPixelShader9 *fx = dx_d3d9c_pshaders[DDRAW::fx];

				d3dd9->SetVertexShader(dx_d3d9c_vshaders[DDRAW::fx]);

				if(!fx) fx = dx_d3d9c_effectsshader;

				if(DDRAW::psColorize) 					
				dx_d3d9X_psconstant(DDRAW::psColorize,DDRAW::fxColorize,'rgba'); 

				#if 0 //old way
				RECT src = {0,0,r,t}; RECT dst = {0,0,w,h}; 
				#else //conservative when viewport is smaller, but doesn't scale up/down
				RECT src = {0,0,vp.Width,vp.Height}; RECT dst = {0,0,vp.Width,vp.Height}; 
				DDRAW::doSuperSamplingDiv(dst.right,dst.bottom);
				#endif
				if(r!=ww||t!=hh)
				{
					assert(r==ww&&t==hh); 
					src.right = r; src.bottom = t; dst.right = w; dst.bottom = h;
				}

				if(DDRAW::doSuperSampling)
				{					
					int x = vp.X, y = vp.Y; 
					OffsetRect(&src,x,y);
					DDRAW::doSuperSamplingDiv(x,y);
					OffsetRect(&dst,x,y);					
				}
				else
				{
					OffsetRect(&dst,vp.X,vp.Y); //2020
					OffsetRect(&src,vp.X,vp.Y); //2020
				}

				int l2 = 1;
				int ir = DDRAW::fxInflateRect;
				int ir2 = ir;
				if(1&&DDRAW::doSuperSampling)
				{
					l2*=2; ir2*=2;
				}
				InflateRect(&src,ir2,ir2),
				InflateRect(&dst,ir,ir);
												
				if(DDRAW::inStereo)
				{
					matte = true/*&&DX::debug*/;
					//+2 resolve ambiguity on the edge in the vertex shader
					//+1???
					int sx = (src.right-src.left)/2+l2; //1
					int dx = (dst.right-dst.left)/2+1; 
					src.right-=sx; dst.right-=dx;
					/*DISABLING after changes to PSVR_PROJ_1
					//etc.
					if(1) //circles are oval???
					{
						//NOTE: MIGHT BE INCREASING "CHROMATIC
						//ABERRATION"
						#ifdef NDEBUG
						#error should FOV be asymmetrical?
						#endif
						//THIS IS A NECESSARY CORRECTION...
						//should the barrel shader be changed?							
						int dy2 = ((dst.bottom-dst.top)-dx)/2;
						//After changing som.shader.cpp the oval
						//is wide instead of tall, and no longer
						//matches the square dimensions? THIS IS
						//APPROACHING 0! 
						//dy2 = dy2*0.75f+0.5f; 
						dst.top+=dy2; dst.bottom-=dy2;
					}*/
					dx_d3d9c_backblt(p,src,dst,fx,DDRAW::fx);
					src.right+=sx; src.left+=sx;
					dst.right+=dx; dst.left+=dx;
				}
				dx_d3d9c_backblt(p,src,dst,fx,DDRAW::fx);
				dx_d3d9c_cleanupblt();

				if(fx!=dx_d3d9c_effectsshader)
				if(DDRAW::Direct3DDevice7->query9->effects[1])
				{
					if(DDRAW::ps2ndSceneMipmapping2) //EXPERIMENTAL
					std::swap(DDRAW::xyMapping2[0],DDRAW::xyMapping2[1]);
					std::swap(p,DDRAW::Direct3DDevice7->query9->effects[1]);

					//TODO? retrieve/restore texture #1?
					//2021: CreateDevice sets DDRAW_TEX0 to black instead of 0 
					d3dd9->SetTexture(1,DDRAW::Direct3DDevice7->query9->black); //0
				}						
			}						

			bool ss = false;
			if(DDRAW::doSuperSampling)
			{
				DWORD sw = vp.Width, sh = vp.Height;
				DDRAW::doSuperSamplingDiv(vp.Width,vp.Height);
				std::swap(ss,DDRAW::doSuperSampling);
				d3dd9->SetViewport(&vp);
				vp.Width = sw; vp.Height = sh;
			}
			else d3dd9->SetViewport(&vp);

			if(DDRAW::onFlip())
			{
				if(out==D3D_OK) out = dx_d3d9c_present(d3dd9);
			}

			//WARNING! JUST REALIZED THIS WAS GETTING IN THE 
			//WAY OF AN ASYNC PRESENT EXPERIMENT THAT SHOULD
			//MAYBE BE REVISITED
			if(matte) d3dd9->Clear(0,0,D3DCLEAR_TARGET,0,0,0);

			std::swap(ss,DDRAW::doSuperSampling);
						 
			//Reminder!!! SetRenderTarget sets the viewport 
			if(rt&&d3dd9->SetRenderTarget(0,rt))
			{
				assert(0); //dx_d3d9c_discardeffects(); 
			}

			d3dd9->SetViewport(&vp); //unset SetRenderTarget

			bbuffer->Release();

			for(DWORD i=0,cwe;i<3;i++) //repair MRT targets
			if(DDRAW::Direct3DDevice7->query9->mrtargets[i])
			if(!d3dd9->GetRenderState((D3DRENDERSTATETYPE)(D3DRS_COLORWRITEENABLE1+i),&cwe)&&cwe)
			DDRAW::Direct3DDevice7->SetRenderState((DX::D3DRENDERSTATETYPE)(D3DRS_COLORWRITEENABLE1+i),cwe);				
		}
		else out = DDRAW_UNIMPLEMENTED;
				
		if(rt) if(rt==DDRAW::Direct3DDevice7->query9->multisample)
		{
			d3dd9->SetDepthStencilSurface(DDRAW::Direct3DDevice7->query9->depthstencil);
		}
		else rt->Release(); //effects
	}	

	if(dx_d3d9c_effectsshader_toggle!=dx_d3d9c_effectsshader_enable)
	{
		dx_d3d9X_enableeffectsshader(dx_d3d9c_effectsshader_toggle);
	}

out: DDRAW::midFlip = false;

	DDRAW_RETURN(out)
}
HRESULT D3D9C::IDirectDrawSurface7::Flip(DX::LPDIRECTDRAWSURFACE7 x, DWORD y)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDrawSurface7::Flip() " << DDRAW::noFlips << '\n';

	DDRAW_IF_NOT_DX9C_RETURN(!DD_OK)

	DDRAW_PUSH_HACK(DIRECTDRAWSURFACE7_FLIP,IDirectDrawSurface7*,
	DX::LPDIRECTDRAWSURFACE7&,DWORD&)(0,this,x,y);

	assert(!x&&y<=1); //2021: DDFLIP_WAIT

	out = D3D9C::IDirectDrawSurface7::flip(); 

	DDRAW_POP_HACK(DIRECTDRAWSURFACE7_FLIP,IDirectDrawSurface7*,
	DX::LPDIRECTDRAWSURFACE7&,DWORD&)(&out,this,x,y);
		
	//DDRAW_RETURN(out) //annoying error firing on close
	//return out;
	DDRAW_RETURN(out) //2021: still firing?
}
HRESULT D3D9C::IDirectDrawSurface7::GetAttachedSurface(DX::LPDDSCAPS2 x, DX::LPDIRECTDRAWSURFACE7 *y)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDrawSurface7::GetAttachedSurface()\n";				 

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK) //DX9X SUBROUTINE
		
	if(!x||!y) DDRAW_RETURN(!DD_OK)

	if(DDRAWLOG::debug>=8) //#ifdef _DEBUG
	{
#ifdef _DEBUG
#define OUT(X) if(x->dwCaps&DDSCAPS_##X) DDRAW_LEVEL(8) << ' ' << #X << '\n';
						   
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

	if(query9->group9==dx_d3d9c_virtual_group) 
	{
		int i; DDRAW::IDirectDrawSurface7 *p = 0;
		//warning: matching against dwCaps only (ignoring 2/3/etc.)
		//warning: not checking for ambiguity (or failing if found)
		//warning: returning out if in&out==in (docs are not clear)
		for(i=0;i<dx_d3d9c_direct3d_swapN;i++) 
		{
			auto &ea = dx_d3d9c_direct3d_swapchain[i];

			if(!ea.dwSize) break;

			enum{ swap_surfaces=DDSCAPS_PRIMARYSURFACE|DDSCAPS_ZBUFFER|DDSCAPS_BACKBUFFER };

			if((x->dwCaps&swap_surfaces)==(ea.ddsCaps.dwCaps&swap_surfaces))
			{
				//DDRAW::IDirectDrawSurface7::get_head->get(dx_d3d9c_virtual_surface+i); 			
				p = dx_d3d9c_direct3d_swapsurfs[i];
				break;
			}	
		}

		if(!p&&i>0&&i<dx_d3d9c_direct3d_swapN)
		if(query9->isPrimary)
		{	
			assert(proxy9==dx_d3d9c_virtual_surface);
			assert(query9->group9==dx_d3d9c_virtual_group);

			if(x->dwCaps&DDSCAPS_BACKBUFFER) 
			{
				p = new DDRAW::IDirectDrawSurface7(dx95);

				dx_d3d9c_direct3d_swapsurfs[i] = p;

				p->query9->group9 = dx_d3d9c_virtual_group;

				//TODO: might want to set proxy9 to GetBackBuffer when able
				p->proxy9 = dx_d3d9c_virtual_surface+i;
				p->query9->isVirtual = true; //2021

				DX::DDSURFACEDESC2 &primary = dx_d3d9c_direct3d_swapchain[0];
				DX::DDSURFACEDESC2 &bbuffer = dx_d3d9c_direct3d_swapchain[i];

				bbuffer.dwSize = sizeof(DX::DDSURFACEDESC2);

				bbuffer.dwFlags = DDSD_WIDTH|DDSD_HEIGHT|DDSD_CAPS|DDSD_PIXELFORMAT;

				p->queryX->height = //2021
				bbuffer.dwHeight = primary.dwHeight;
				p->queryX->width = //2021
				bbuffer.dwWidth = primary.dwWidth;

				//note: DDSCAPS_3DDEVICE seems like it should be necessary
				bbuffer.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER|DDSCAPS_3DDEVICE;

				bbuffer.ddpfPixelFormat = primary.ddpfPixelFormat;

				if(!DDRAW::BackBufferSurface7)
				{
					DDRAW::BackBufferSurface7 = p; //2021
					p->query9->format =
					p->query9->nativeformat = dx_d3d9c_format(bbuffer.ddpfPixelFormat); 
				}
				else assert(0); //above code should rule this out
			}
		}
		else assert(proxy9!=dx_d3d9c_virtual_surface); //2021

		if(p){ *y = p; p->AddRef(); }
		
		DDRAW_RETURN(p?DD_OK:!DD_OK)		
	}

	if(x->dwCaps&DDSCAPS_TEXTURE)
	{
		if(~x->dwCaps&DDSCAPS_MIPMAP) DDRAW_RETURN(!DD_OK) //good enough??

		if(DDRAW::doMipmaps||!query9->group9) return DDERR_NOMIPMAPHW;

		int lv = query9->isMipmap+1; IDirect3DSurface9 *s = 0; 

		if(query9->group9->GetSurfaceLevel(lv,&s)==D3D_OK)
		{
			DDRAW::IDirectDrawSurface7 *p = new DDRAW::IDirectDrawSurface7(dx95);
						
			p->query9->isMipmap = lv; 

			p->query9->group9 = query9->group9; 

			p->query9->group9->AddRef();

			p->query9->colorkey2 = query9->colorkey2;

			p->query9->colorkey = query9->colorkey; //shares memory with mipmaps

			p->proxy9 = s; 
				
			p->query9->pitch = dx_d3d9c_pitch(s);

			p->query9->nativeformat = query9->nativeformat; //alpha?

			p->query9->format = query9->format; //2021
			
			D3DSURFACE_DESC desc; //2021
			s->GetDesc(&desc);
			p->query9->width = desc.Width;
			p->query9->height = desc.Height;

			*y = p; return DD_OK;
		}
		else return !DD_OK; //must be the last mipmap
	}
	
	DDRAW_RETURN(DDRAW_UNIMPLEMENTED)
}
HRESULT D3D9C::IDirectDrawSurface7::GetColorKey(DWORD x, DX::LPDDCOLORKEY y)
{ 
	DDRAW_LEVEL(7) << "D3D9C::IDirectDrawSurface7::GetColorKey()\n";	

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	if(!queryX->colorkey) return DDERR_NOCOLORKEY;
			   
	DX::DDCOLORKEY *key = 0;

	switch(x)
	{	
	case DDCKEY_SRCBLT:

		if(queryX->colorkey2&DDSD_CKSRCBLT)			
		key = queryX->colorkey+0; break;

	case DDCKEY_DESTBLT: 

		if(queryX->colorkey2&DDSD_CKDESTBLT)			
		key = queryX->colorkey+1; break;
	
	case DDCKEY_SRCOVERLAY:

		if(queryX->colorkey2&DDSD_CKSRCOVERLAY)
		key = queryX->colorkey+2; break;

	case DDCKEY_DESTOVERLAY:

		if(queryX->colorkey2&DDSD_CKDESTOVERLAY)
		key = queryX->colorkey+3; break;

	default: return DDERR_INVALIDPARAMS;
	}

	if(key&&y) *y = *key;

	return key?DD_OK:DDERR_NOCOLORKEY;
}
HRESULT D3D9C::IDirectDrawSurface7::GetDC(HDC FAR *x)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDrawSurface7::GetDC()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	HRESULT out = !D3D_OK; if(!x) return !D3D_OK;
		
	if(query9->group9==dx_d3d9c_virtual_group)
	{
		DX::DDSURFACEDESC2 desc; desc.dwSize = sizeof(DX::DDSURFACEDESC2);

		if(GetSurfaceDesc(&desc)==DD_OK) 
		{
			if(desc.ddsCaps.dwCaps&DDSCAPS_ZBUFFER) DDRAW_RETURN(!DD_OK)

			//note: direct3d9 can only provide a readonly copy of the front buffer
			if(desc.ddsCaps.dwCaps&DDSCAPS_FRONTBUFFER) DDRAW_RETURN(!DD_OK)

			//note: assuming only one backbuffer exists
			if(desc.ddsCaps.dwCaps&DDSCAPS_3DDEVICE //software backbuffer
			 ||desc.ddsCaps.dwCaps&DDSCAPS_BACKBUFFER)
			{	
				if(!DDRAW::Direct3DDevice7) DDRAW_RETURN(!DD_OK)

				if(1//todo: D3DPRESENTFLAG_LOCKABLE_BACKBUFFER
				||DDRAW::Direct3DDevice7->query9->isSoftware
				||DDRAW::Direct3DDevice7->query9->multisample)
				{
					//// back buffer is not lockable /////

					*x = ::GetDC(DDRAW::window); //bogus??

					DDRAW_RETURN(DD_OK)
				}
				else assert(0);

				//REMINDER: dx_d3d9X_backbuffer WOULD BE REQUIRED
				//BELOW IF THIS CODE WAS NOT UNREACHABLE

				IDirect3DSurface9 *p = 0; 
				out = DDRAW::Direct3DDevice7->proxy9->
				GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&p);
				if(out!=D3D_OK) DDRAW_RETURN(!DD_OK)
				//note: assuming outstanding DC on 0 ref interface ok
				out = p->GetDC(x); p->Release();
			}			
		}
	}
	else 
	{
		out = proxy9->GetDC(x);

		if(out!=D3D_OK)
		{
			dx_d3d9c_assume_alpha_not_ok(this);

			out = proxy9->GetDC(x);

			/*UNTESTED / SPITBALLING
			if(target!='dx9c'&&!out) //OpenGL? (upside down?)
			{
				//HACK: this (probably) works for SOM but there's probably
				//a better way (and it should be done with hack_interface)
				XFORM xf =  {1,0,0,-1,0,queryX->height};
				SetGraphicsMode(*x,GM_ADVANCED); SetWorldTransform(*x,&xf);	
			}*/
		}
	}

	if(out==D3D_OK) query9->wasLocked = query9->isLocked = true;

	DDRAW_LEVEL(7) << ' ' << (unsigned)this << '(' << (out==D3D_OK?*x:0) << ")\n";

	DDRAW_RETURN(out==D3D_OK?DD_OK:!DD_OK)
}
HRESULT D3D9C::IDirectDrawSurface7::GetPixelFormat(DX::LPDDPIXELFORMAT x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirectDrawSurface7::GetPixelFormat()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	DX::DDSURFACEDESC2 desc = {sizeof(DX::DDSURFACEDESC2)};

	HRESULT out = x?GetSurfaceDesc(&desc):!DD_OK;

	if(out==DD_OK&&x) *x = desc.ddpfPixelFormat;

	DDRAW_RETURN(out);
}
HRESULT D3D9C::IDirectDrawSurface7::GetSurfaceDesc(DX::LPDDSURFACEDESC2 x)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDrawSurface7::GetSurfaceDesc()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK) //DX9X SUBROUTINE

	if(!x||x->dwSize!=sizeof(DX::DDSURFACEDESC2)) DDRAW_RETURN(!DD_OK)
	
	if(query9->group9==dx_d3d9c_virtual_group)
	{
		int i = proxy9-dx_d3d9c_virtual_surface; 
		
		if(i<0||i>2) DDRAW_RETURN(!DD_OK)
		
		*x = dx_d3d9c_direct3d_swapchain[i]; 		
		
		if(x->ddsCaps.dwCaps&DDSCAPS_BACKBUFFER)
		{
			//warning: not sure if ok for window mode??
			x->ddsCaps.dwCaps|=DDSCAPS_COMPLEX|DDSCAPS_FLIP; 

			if(!DDRAW::Direct3DDevice7) return DD_OK;
						
			//NECESSARY?
			/*2021: with supersampling the size will be different
			D3DFORMAT format = query9->format;
			if(dx_d3d9c_effectsshader_enable)
			{
				x->lPitch = dx_d3d9c_rendertarget_pitch;

				format = dx_d3d9c_rendertarget_format;
			}
			else*/
			{
				//x->lPitch = dx_d3d9c_backbuffer_pitch;
				x->lPitch = DDRAW::BackBufferSurface7->queryX->pitch;
			}
			//x->ddpfPixelFormat = dx_d3d9c_format(format);
			//if(x->lPitch) //NOTE: this pitch may be just a guess
			x->dwFlags|=DDSD_PITCH;
		}

		return DD_OK;		
	}
	
	D3DSURFACE_DESC desc;
	
	x->dwFlags = DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT;

	x->ddsCaps.dwCaps = 0x00000000;

	if(query9->group9) //mipmapped texture
	{
		if(!query9->isMipmap)
		{
			query9->group9->GetLevelDesc(0,&desc);
			x->dwMipMapCount = query9->group9->GetLevelCount();
			x->dwFlags|=DDSD_MIPMAPCOUNT;
		}
		else query9->group9->GetLevelDesc(query9->isMipmap,&desc);

		x->ddsCaps.dwCaps|=DDSCAPS_COMPLEX;		
		x->ddsCaps.dwCaps|=DDSCAPS_MIPMAP; //ok for first level??		

		x->dwFlags|=DDSD_TEXTURESTAGE;

		x->dwTextureStage = 0; //assuming		
	}
	else proxy9->GetDesc(&desc);

	x->dwHeight = desc.Height;
	x->dwWidth = desc.Width;

	x->ddpfPixelFormat = dx_d3d9c_format(desc.Format);

	//TODO: reconsider methodology for software device textures
	x->ddsCaps.dwCaps|=query9->group9?DDSCAPS_TEXTURE:DDSCAPS_OFFSCREENPLAIN;

	switch(desc.Pool)
	{	
	case D3DPOOL_DEFAULT: x->ddsCaps.dwCaps|=DDSCAPS_VIDEOMEMORY; break;
	case D3DPOOL_SYSTEMMEM: x->ddsCaps.dwCaps|=DDSCAPS_SYSTEMMEMORY; break;

//	case D3DPOOL_MANAGED: if(query9->group9) x->ddsCaps.dwCaps2|=DDSCAPS2_TEXTUREMANAGE; break;

	default: DDRAW_RETURN(!DD_OK)
	}

	if(desc.Usage&D3DUSAGE_WRITEONLY) x->ddsCaps.dwCaps|=DDSCAPS_WRITEONLY;

//	x->ddsCaps.dwCaps|=DDSCAPS_PALETTE; //should work in theory

	if(query9->pitch) x->dwFlags|=DDSD_PITCH; x->lPitch = query9->pitch;

	return DD_OK;
}
HRESULT D3D9C::IDirectDrawSurface7::IsLost()
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDrawSurface7::IsLost()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	DDRAW_RETURN(!D3D_OK) /*

	return proxy9->IsLost();
*/
}
HRESULT D3D9C::IDirectDrawSurface7::Lock(LPRECT x, DX::LPDDSURFACEDESC2 y, DWORD z, HANDLE w)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDrawSurface7::Lock()\n"; assert(!w);

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	DWORD flags = 0; //may not be comprehensive
	if(z&DDLOCK_READONLY)  flags|=D3DLOCK_READONLY;		
	if(z&DDLOCK_WRITEONLY) flags|=D3DLOCK_DISCARD;
	if(z&DDLOCK_NOSYSLOCK) flags|=D3DLOCK_NOSYSLOCK;	
	if(~z&DDLOCK_WAIT)	   flags|=D3DLOCK_DONOTWAIT;

	D3DLOCKED_RECT lock;
	HRESULT out = proxy9->LockRect(&lock,x,flags);
	if(out==D3D_OK) 
	{			
		query9->isLocked = true; 

		//NOTE: ?: segfaulted when converted to bitfield
		if(z&DDLOCK_READONLY) 
		query9->isLockedRO = true; else query9->wasLocked = true;

		//TODO: should add DDSD_CAPS to this list
		y->dwFlags = DDSD_HEIGHT|DDSD_WIDTH|DDSD_PITCH|DDSD_LPSURFACE|DDSD_PIXELFORMAT;

		y->dwHeight = queryX->height; y->dwWidth = queryX->width;

		y->lPitch = lock.Pitch; y->lpSurface = lock.pBits;

		y->ddpfPixelFormat = dx_d3d9c_format(queryX->format);
	}

	DDRAW_RETURN(out)
}
HRESULT D3D9C::IDirectDrawSurface7::ReleaseDC(HDC x)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDrawSurface7::ReleaseDC()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	DDRAW_LEVEL(7) << ' ' << (unsigned)this << '(' << x << ")\n";
				
	HRESULT out = !D3D_OK;
		
	if(query9->group9==dx_d3d9c_virtual_group)
	{
		DX::DDSURFACEDESC2 desc; desc.dwSize = sizeof(DX::DDSURFACEDESC2);

		if(GetSurfaceDesc(&desc)==DD_OK) 
		{
			if(desc.ddsCaps.dwCaps&DDSCAPS_ZBUFFER) DDRAW_RETURN(!DD_OK)

			//note: direct3d9 can only provide a readonly copy of the front buffer
			if(desc.ddsCaps.dwCaps&DDSCAPS_FRONTBUFFER) DDRAW_RETURN(!DD_OK)

			//note: assuming only one backbuffer exists
			if(desc.ddsCaps.dwCaps&DDSCAPS_BACKBUFFER
			 ||desc.ddsCaps.dwCaps&DDSCAPS_3DDEVICE) //software backbuffer
			{
				if(!DDRAW::Direct3DDevice7) DDRAW_RETURN(!DD_OK)

				if(1//todo: D3DPRESENTFLAG_LOCKABLE_BACKBUFFER
				||DDRAW::Direct3DDevice7->query9->isSoftware
				||DDRAW::Direct3DDevice7->query9->multisample)
				{
					//// back buffer is not lockable /////

					::ReleaseDC(DDRAW::window,x); //bogus??

					DDRAW_RETURN(DD_OK)
				}
				else assert(0);

				//REMINDER: dx_d3d9X_backbuffer WOULD BE REQUIRED
				//BELOW IF THIS CODE WAS NOT UNREACHABLE

				IDirect3DSurface9 *p = 0; 
				out = DDRAW::Direct3DDevice7->proxy9->
				GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&p);
				if(out!=D3D_OK) DDRAW_RETURN(!DD_OK)					
				//note: assuming outstanding DC on 0 ref interface ok
				out = p->ReleaseDC(x); p->Release(); 
			}			
		}
	}
	else 
	{
		out = proxy9->ReleaseDC(x);

		if(!out) dirtying_texture();
	}

	if(out==D3D_OK) query9->isLocked = false;
	
	DDRAW_RETURN(out==D3D_OK?DD_OK:!DD_OK);
}
HRESULT D3D9C::IDirectDrawSurface7::Restore()
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDrawSurface7::Restore()\n";
													  
	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	DDRAW_RETURN(!DD_OK) /*

	return proxy9->Restore();
*/
}
HRESULT D3D9C::IDirectDrawSurface7::SetClipper(DX::LPDIRECTDRAWCLIPPER y)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDrawSurface7::SetClipper()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	DDRAW_LEVEL(1) << "Ignoring SetClipper() (ASSUMING UNNECESSARY)\n";

	return DD_OK; //JUST IGNORING... ASSUMING NOT REALLY NECESSARY
}
namespace{ //TEMPORARY
enum{ ddsd_ckunion=DDSD_CKSRCBLT|DDSD_CKDESTBLT|DDSD_CKSRCOVERLAY|DDSD_CKDESTOVERLAY };
enum{ ddckey_union=DDCKEY_SRCBLT|DDCKEY_DESTBLT|DDCKEY_SRCOVERLAY|DDCKEY_DESTOVERLAY };
}
static void dx_d3d9c_colorkey(DDRAW::IDirectDrawSurface7 *pp) //2021
{
	assert(dx_d3d9c_imitate_colorkey);

		//TODO: NEED TO SUPPORT REMOVAL OF COLORKEY

	auto q = pp->query9;
	IDirect3DSurface9 *p = pp->proxy9;

		//EneEdit.exe is triggering this when a TXR
		//skin having PRF is opened after a non TXR
		//having skin PRF is opened (refs is 4)
//		DDRAW_ASSERT_REFS(query9->group9,==2)

	D3DSURFACE_DESC desc; int knockouts = 0;
	
	UINT mipmaps = q->group9?q->group9->GetLevelCount():1;
	for(UINT lv=0;lv<mipmaps;lv++)
	if(!q->group9||!q->group9->GetSurfaceLevel(lv,&p))
	if(!p->GetDesc(&desc)
	&&(desc.Format==D3DFMT_A8R8G8B8||desc.Format==D3DFMT_X8R8G8B8
	||desc.Format==D3DFMT_A1R5G5B5||desc.Format==D3DFMT_X1R5G5B5))
	{
		if(lv==1&&q->group9) //hack: location wise
		if(DDRAW::doMipmaps&&!dx_d3d9c_autogen_mipmaps)
		{				
			//2021: I'm letting this wait for updating_texture to
			//be called by SetTexture since in theory GetDC/Lock
			//may be called in the interim... I'm not sure how it
			//may affect loading/interruption
			//dx_d3d9c_mipmap(this); 								
			p->Release(); 
			break;
		}

		//2021: disable colorkey? this way the second branch
		//will fill in all opaque
		if(!q->colorkey2) switch(desc.Format)
		{
		case D3DFMT_A8R8G8B8: desc.Format = D3DFMT_X8R8G8B8; break;  
		case D3DFMT_A1R5G5B5: desc.Format = D3DFMT_X1R5G5B5; break;
		}

		D3DLOCKED_RECT lock;
		if(q->colorkey2&&!q->isLocked) //uh oh
		if(p->LockRect(&lock,0,0)==D3D_OK)
		{
			if(q->colorkey_f)
			{
				DX::DDSURFACEDESC2 desc2 = {sizeof(DX::DDSURFACEDESC2)};

				desc2.dwFlags = q->colorkey2&ddsd_ckunion|
				DDSD_HEIGHT|DDSD_WIDTH|DDSD_PITCH|DDSD_LPSURFACE|DDSD_PIXELFORMAT;

				desc2.dwHeight = desc.Height;
				desc2.dwWidth = desc.Width;

				desc2.lPitch = lock.Pitch;

				desc2.lpSurface = lock.pBits;

				desc2.ddckCKSrcBlt = q->colorkey[0];
				desc2.ddckCKDestBlt = q->colorkey[1];
				desc2.ddckCKSrcOverlay = q->colorkey[2];
				desc2.ddckCKDestOverlay = q->colorkey[3];

				desc2.ddpfPixelFormat = dx_d3d9c_format(desc.Format);

				//knockouts = DDRAW::colorkey(&desc2,q->nativeformat); //alpha?
				knockouts = q->colorkey_f(&desc2,q->nativeformat); //alpha?
			}
			else //the old way
			{
				/*debugging?
				DWORD colorize = 0;
				if(colorize) switch(lv%3) 
				{
				case 0: colorize = desc.Format==D3DFMT_A8R8G8B8?0xFF0000:0x7c00; break;
				case 1: colorize = desc.Format==D3DFMT_A8R8G8B8?0x00FF00:0x03e0; break;
				case 2: colorize = desc.Format==D3DFMT_A8R8G8B8?0x0000FF:0x001f; break;
				}*/

				//setup alpha channel for pitch black pixels
				for(size_t i=0;i<desc.Height;i++)
				{
					if(desc.Format==D3DFMT_A8R8G8B8
					||desc.Format==D3DFMT_X8R8G8B8)
					{
						DWORD *p = (DWORD*)((BYTE*)lock.pBits+lock.Pitch*i);

						for(size_t j=0;j<desc.Width;j++)
						if((p[j]&0xFF000000)==0
						||desc.Format==D3DFMT_X8R8G8B8)
						{
							if((p[j]&~0xFF070707)==0)
							{
								knockouts++; 
								
								p[j] = 0x00000000; //p[j]&=0x00FFFFFF; 
							}
							else p[j]|=0xFF000000;
						}
					}
					else //16bit
					{
						WORD *p = (WORD*)((BYTE*)lock.pBits+lock.Pitch*i);

						for(size_t j=0;j<desc.Width;j++)
						if((p[j]&0x8000)==0
						||desc.Format==D3DFMT_X1R5G5B5) 
						{
							if((p[j]&~0x8000)==0) 
							{
								knockouts++; 
								
								p[j] = 0x0000; //p[j]&=~0x8000;
							}
							else p[j]|=0x8000;
						}
					}				
				}
			}

			p->UnlockRect();
		}

		if(q->group9) p->Release();				
	}
	else assert(0);

	q->knockouts = knockouts;

		//EneEdit.exe is triggering this when a TXR
		//skin having PRF is opened after a non TXR
		//having skin PRF is opened (refs is 4)
//		DDRAW_ASSERT_REFS(query9->group9,==2)
}
HRESULT D3D9C::IDirectDrawSurface7::SetColorKey(DWORD x, DX::LPDDCOLORKEY y)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDrawSurface7::SetColorKey()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	DDRAW_PUSH_HACK(DIRECTDRAWSURFACE7_SETCOLORKEY,IDirectDrawSurface7*,
	DWORD&,DX::LPDDCOLORKEY&)(0,this,x,y);	

	if((x&ddckey_union)!=x||x==0) DDRAW_FINISH(x?!DD_OK:DD_OK)

	DWORD cmp = query9->colorkey2&(DDSD_CKSRCBLT|DDSD_CKDESTBLT);
	auto cmp2 = query9->colorkey;

	if(!y) //unsetting
	{
		if(x&DDCKEY_SRCBLT) query9->colorkey2&=~DDSD_CKSRCBLT;
		if(x&DDCKEY_DESTBLT) query9->colorkey2&=~DDSD_CKDESTBLT;
		if(x&DDCKEY_SRCOVERLAY) query9->colorkey2&=~DDSD_CKSRCOVERLAY;
		if(x&DDCKEY_DESTOVERLAY) query9->colorkey2&=~DDSD_CKDESTOVERLAY;

		if(query9->colorkey&&!(query9->colorkey2&ddsd_ckunion))
		{
			delete[] query9->colorkey; query9->colorkey = 0;
		}

		if(dx_d3d9c_imitate_colorkey&&(cmp2!=queryX->colorkey
		||cmp!=(query9->colorkey2&(DDSD_CKSRCBLT|DDSD_CKDESTBLT))))
		{
			goto imitate_colorkey; //2021
		}
		else DDRAW_FINISH(DD_OK)
	}

	if(!query9->colorkey) query9->colorkey = new DX::DDCOLORKEY[4];			

	if(x&DDCKEY_SRCBLT) 
	{
		query9->colorkey[0] = *y; 
		query9->colorkey2|=DDSD_CKSRCBLT;
	}
	if(x&DDCKEY_DESTBLT) 
	{
		query9->colorkey[1] = *y; 
		query9->colorkey2|=DDSD_CKDESTBLT;
	}
	if(x&DDCKEY_SRCOVERLAY)
	{
		query9->colorkey[2] = *y; 
		query9->colorkey2|=DDSD_CKSRCOVERLAY;
	}
	if(x&DDCKEY_DESTOVERLAY)
	{
		query9->colorkey[3] = *y; 
		query9->colorkey2|=DDSD_CKDESTOVERLAY;
	}

	query9->knockouts = 0;

	////DEBUGGING/EXPERIMENTAL/////////////////////
	//
	// NOTE: SOM is relying on this to fill out its
	// mipmaps, which works because they always are
	// colorkeyed, but otherwise this is unreliable
	//
	if(dx_d3d9c_imitate_colorkey) imitate_colorkey:
	{
		//NOTE: the code was here was moved to dx_d3d9c_colorkey
		//and is now being deferred until updating_texture		
		if(!DDRAW::doMipmaps) dx_d3d9c_colorkey(this);
				
		//2021: this isn't being applied consistently
		//if(dx_d3d9c_preload&&query9->group9)
		//D3D9C::updating_texture(this,false);
		query9->update_pending|=2;
		dirtying_texture(); //2021
	}

	out = DD_OK;

	DDRAW_POP_HACK(DIRECTDRAWSURFACE7_SETCOLORKEY,IDirectDrawSurface7*,
	DWORD&,DX::LPDDCOLORKEY&)(&out,this,x,y);	

	DDRAW_RETURN(out)
}
HRESULT D3D9C::IDirectDrawSurface7::SetPalette(DX::LPDIRECTDRAWPALETTE x)
{
	assert(!x); //SOM::Texture::palette? //2023: not expecting palettes

	DDRAW_LEVEL(7) << "D3D9C::IDirectDrawSurface7::SetPalette()\n";

	if(x) DDRAW_IF_NOT_DX9C_RETURN(!DD_OK)

	//TODO: find out if this effects palette's ref count
	
	if(!x){	query9->palette = 0; return DD_OK;	} //unsetting

	auto p = DDRAW::is_proxy<IDirectDrawPalette>(x);
	
	if(!p->palette9||p->palette9>256||!dx_d3d9c_palettes[p->palette9])
	DDRAW_RETURN(!DD_OK)

	query9->palette = p->palette9; return DD_OK;
}
HRESULT D3D9C::IDirectDrawSurface7::Unlock(LPRECT x)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirectDrawSurface7::Unlock()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	assert(!x);
	HRESULT out = proxy9->UnlockRect();

	if(!out)
	{
		query9->isLocked = false; //2021

		if(!query9->isLockedRO) //not read-only?
		{
			dirtying_texture(); //2021
		}
		else query9->isLockedRO = false;
	}

	DDRAW_RETURN(out)
}






HRESULT D3D9C::IDirect3D7::QueryInterface(REFIID riid, LPVOID *ppvObj)
{ 
	DDRAW_LEVEL(7) << "D3D9C::IDirect3D7::QueryInterface()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK)

	LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';

	DDRAW_RETURN(!S_OK)
		
	return proxy9->QueryInterface(riid,ppvObj); 
}
ULONG D3D9C::IDirect3D7::AddRef()
{ 
	DDRAW_LEVEL(7) << "D3D9C::IDirect3D7::AddRef()\n";

	DDRAW_ADDREF_RETURN(0)
}
ULONG D3D9C::IDirect3D7::Release()
{ 
	DDRAW_LEVEL(7) << "D3D9C::IDirect3D7::Release()\n";

	DDRAW_IF_NOT_TARGET_AND_RELEASE(out,0)

	if(out==0) delete this;
	
	return out;
}
HRESULT D3D9C::IDirect3D7::EnumDevices(DX::LPD3DENUMDEVICESCALLBACK7 x, LPVOID y)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3D7::EnumDevices()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK)
		
	D3DADAPTER_IDENTIFIER9 id; if(!x) DDRAW_RETURN(!D3D_OK) //paranoia

	if(proxy9->GetAdapterIdentifier(query9->adapter,0,&id)!=D3D_OK) DDRAW_RETURN(!D3D_OK);

	DX::D3DDEVICEDESC7 desc7; ::D3DCAPS9 caps9; D3DFORMAT rgb = D3DFMT_X8R8G8B8; 

	int passes = D3D9C::doEmulate?3:2;

	for(int pass=0;pass<passes;pass++)
	{
		if(!D3D9C::doEmulate&&pass==0&&!dx_d3d9c_sw()) continue; 

		D3DDEVTYPE dev = pass==0?D3DDEVTYPE_SW:D3DDEVTYPE_HAL;

		if(D3D9C::doEmulate&&pass==0&&!dx_d3d9c_sw()) dev = D3DDEVTYPE_HAL;

		if(D3D9C::doEmulate) switch(pass)
		{
		case 1:	case 2: dev = D3DDEVTYPE_HAL;
		}

		if(proxy9->GetDeviceCaps(query9->adapter,dev,&caps9)!=D3D_OK) 
		{
			continue; //todo: log alert message
		}

		dx_d3d9c_d3ddrivercaps(caps9,desc7);

		if(D3D9C::doEmulate) switch(pass) //switch(emulate9)
		{
		case 0: desc7.deviceGUID = DX::IID_IDirect3DRGBDevice; break;
		case 1:	desc7.deviceGUID = DX::IID_IDirect3DHALDevice; break;
		case 2: desc7.deviceGUID = DX::IID_IDirect3DTnLHalDevice; break;
		}
	
		//note: not factoring in stencil bits
		//note: assuming render target is X8R8G8B8
		//note: only X8R8G8B8 reliably permits gamma adjust
		if(proxy9->CheckDepthStencilMatch(query9->adapter,dev,rgb,rgb,D3DFMT_D16)==D3D_OK)
		desc7.dwDeviceZBufferBitDepth|=DDBD_16;
		if(proxy9->CheckDepthStencilMatch(query9->adapter,dev,rgb,rgb,D3DFMT_D24X8)==D3D_OK)
		desc7.dwDeviceZBufferBitDepth|=DDBD_24;
		//if(proxy9->CheckDepthStencilMatch(adapter9,dev,rgb,rgb,D3DFMT_D32)==D3D_OK)
		//desc7.dwDeviceZBufferBitDepth|=DDBD_32;
						
		char *info = id.Description, *name = id.DeviceName;

		if(D3D9C::doEmulate) switch(pass) //switch(emulate9)
		{
		case 0: info = "Microsoft Direct3D RGB Software Emulation";
			
			name = "RGB Emulation"; break;

		case 1:	info = "Microsoft Direct3D Hardware acceleration through Direct3D HAL";
			
			name = "Direct3D HAL"; break;

		case 2: info = "Microsoft Direct3D Hardware Transform and Lighting acceleration capable device";
			
			name = "Direct3D T&L HAL"; break;
		}

		if(x(info,name,&desc7,y)!=DDENUMRET_CANCEL)	
		continue; else break;
	}
	return D3D_OK;
}

//EXPERIMENTAL
extern bool DDRAW::stereo_update_SetStreamSource()
{
	DDRAW::IDirect3DDevice7* &d = DDRAW::Direct3DDevice7;
	if(!d||'dx9c'!=d->target){ assert(0); return false; }
	
	HRESULT out = 0;
	if(!DDRAW::inStereo)
	{
		d->proxy9->SetStreamSourceFreq(0,1);
		d->proxy9->SetStreamSourceFreq(1,1);
		return false;
	}
	else if(!d->query9->stereo)
	{
		D3DPOOL pool = D3DPOOL_DEFAULT;
		if(!D3D9C::DLL::Direct3DCreate9Ex) pool = D3DPOOL_MANAGED;
		DWORD usage = D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY;
		out = d->proxy9->CreateVertexBuffer(16,usage,0,pool,&d->query9->stereo,0);
		if(out) return false;		
	}
	
	union{ void *lock; float *vb; };
	d->query9->stereo->Lock(0,8,&lock,D3DLOCK_DISCARD);
	//TO ACCOMMODATE ASSYMETRICAL EYES THIS
	//HAS TO BE REPLACED WITH TWO X/Y PAIRS
	//AND POSSIBLY OTHER DEGREES OF FREEDOM
	vb[0] = -DDRAW::stereo; vb[1] = -DDRAW::stereo2; 
	vb[2] = +DDRAW::stereo; vb[3] = +DDRAW::stereo2; 
	d->query9->stereo->Unlock();	
	
		//2021: using SCISSOR instead instancing works better because
		//it avoids using clip in the shader and kills more pixels so
		//dx_d3d9c_stereo_scissor just interleaves the extra data (NOTE THE
		//"extra data" IS CONSTANT, SO COULD BE STASHED IN A REGISTER
		//WHEN NOT INSTANCING)

	//"This value is logically combined with the number of instances of 
	//the geometry to draw."
	if(0==dx_d3d9c_stereo_scissor)
	out|=d->proxy9->SetStreamSourceFreq(0,D3DSTREAMSOURCE_INDEXEDDATA|2);
	out|=d->proxy9->SetStreamSource(1,d->query9->stereo,0,8);	
	//"This value is logically combined with 1 since each vertex contains
	//one set of instance data."
	out|=d->proxy9->SetStreamSourceFreq(1,D3DSTREAMSOURCE_INSTANCEDATA|1);
	assert(!out);
	return !out;
}

HRESULT D3D9C::IDirect3D7::CreateDevice(REFCLSID xIn, DX::LPDIRECTDRAWSURFACE7 y, DX::LPDIRECT3DDEVICE7 *z)
{
	const GUID *x = &xIn;

	DDRAW_LEVEL(7) << "D3D9C::IDirect3D7::CreateDevice()\n";
	
	DDRAW_IF_NOT_DX9C_RETURN(!D3D_OK);

	DDRAW_PUSH_HACK(DIRECT3D7_CREATEDEVICE,IDirect3D7*,	
	const GUID*&,DX::LPDIRECTDRAWSURFACE7&,DX::LPDIRECT3DDEVICE7*&)(0,this,x,y,z);

	if(7<=DDRAWLOG::debug)
	{
		LPOLESTR o; if(StringFromCLSID(*x,&o)==S_OK) DDRAW_LEVEL(7) << ' ' << o << '\n';
								
		if(*x==DX::IID_IDirect3DRGBDevice)
		{
			DDRAW_LEVEL(7) << "(IDirect3DRGBDevice)\n";
		}
		else if(*x==DX::IID_IDirect3DHALDevice)
		{
			DDRAW_LEVEL(7) << "(IDirect3DHALDevice)\n";
		}
		else if(*x==DX::IID_IDirect3DTnLHalDevice)
		{
			DDRAW_LEVEL(7) << "(IDirect3DTnLHalDevice)\n";
		}
		else assert(0); //unexpected behavior
	}
		
	auto psurf = DDRAW::is_proxy<IDirectDrawSurface7>(y);

	if(!psurf||psurf->query9->group9!=dx_d3d9c_virtual_group) DDRAW_FINISH(!D3D_OK) //paranoia	  

	DX::DDSURFACEDESC2 zbuffer, primary; //note: primary here can be the backbuffer

	zbuffer.dwSize = primary.dwSize = sizeof(DX::DDSURFACEDESC2);

	//C4533
	{
		DX::LPDIRECTDRAWSURFACE7 zsurf = 0;
		DX::DDSCAPS2 zcaps = { DDSCAPS_ZBUFFER };
		if(psurf->GetAttachedSurface(&zcaps,&zsurf)==D3D_OK)
		{	
			zsurf->GetSurfaceDesc(&zbuffer); zsurf->Release();
			psurf->GetSurfaceDesc(&primary);
		}
		else DDRAW_FINISH(!D3D_OK)

		//2021: this is expected below
		if(!DDRAW::BackBufferSurface7)
		{
			zcaps.dwCaps = DDSCAPS_BACKBUFFER;
			//ItemEdit passes an offscreen-surface to CreateDevice
			//that's not its back-buffer... maybe generating fonts
			//psurf->GetAttachedSurface(&zcaps,&zsurf);
			if(DDRAW::PrimarySurface7)
			if(!DDRAW::PrimarySurface7->GetAttachedSurface(&zcaps,&zsurf))
			zsurf->Release();
			if(!DDRAW::BackBufferSurface7) //no primary?
			{
				assert(0); DDRAW_FINISH(!D3D_OK);
			}
		}		
	}

	const auto w = primary.dwWidth, h = primary.dwHeight; //2021

	D3DDEVTYPE dev = *x==DX::IID_IDirect3DRGBDevice?D3DDEVTYPE_SW:D3DDEVTYPE_HAL; 
	
	if(dev==D3DDEVTYPE_SW&&!dx_d3d9c_sw()) dev = D3DDEVTYPE_HAL;
		
	D3DDISPLAYMODE dm;	  
	proxy9->GetAdapterDisplayMode(query9->adapter,&dm);	
	DDRAW::refreshrate = dm.RefreshRate; //2021
	switch(DDRAW::refreshrate)
	{
	case 59: DDRAW::refreshrate = 60; break;
	}
	D3DFORMAT bpp = DDRAW::fullscreen?dm.Format:D3DFMT_UNKNOWN; 

	/*2021: Seeing if "deep color" helps? I got the idea from ANGLE
	//defaulting to 10:10:10 color... even though it seems wrong to
	//me since the monitor was 8:8:8!
	D3DFORMAT highcolor = DDRAW::do565?D3DFMT_R5G6B5:D3DFMT_X1R5G5B5;
	D3DFORMAT fxbpp = DDRAW::bitsperpixel==16?highcolor:dx_d3d9_fxformat;*/
	D3DFORMAT fxbpp = D3DFMT_A8R8G8B8;
	//I can't really see any difference... better or worse. it might be good for the
	//brightness adjustment, to be less destructive? since it's not known what's the
	//back-buffer format, in theory this should ensure the effects buffer isn't less
	if(0) //UNUSED
	{
		//see dx.d3d9x.cpp for notes on how OpenGL doesn't have facilities
		//for decoding 10/30bit textures into sRGB

		if(/*1&&DX::debug&&*/dev==D3DDEVTYPE_HAL)
		if(!proxy9->CheckDeviceFormat(query9->adapter,dev,D3DFMT_X8R8G8B8,
		//D3DUSAGE_QUERY_SRGBREAD| //???
		D3DUSAGE_RENDERTARGET,D3DRTYPE_TEXTURE,D3DFMT_A2R10G10B10))
		fxbpp = D3DFMT_A2R10G10B10;
	}

	D3DFORMAT dsbpp = D3DFMT_D16;
	switch(zbuffer.ddpfPixelFormat.dwZBufferBitDepth)
	{
	case 24: dsbpp = D3DFMT_D24X8; break; case 32: dsbpp = D3DFMT_D32; break;
	}
	if(DDRAW::DepthStencilSurface7) //2021		
	DDRAW::DepthStencilSurface7->query9->nativeformat = dsbpp;
	else assert(0);

	D3DPRESENT_PARAMETERS pps;											 
		
	//2: triple buffering? //tearing in fullscreen
	//1: not seeing a benefit???
	//2: D3DSWAPEFFECT_FLIPEX wants 2
	//3: 2 doesn't seem to make a difference with FLIPEX
	//so I think that maybe non-FLIPEX might benefit from
	//3, in order to not exclude Vista/XP owners
	//1: must be 1 for non real-time productivity tools
	//ALSO Present with a rectangle fails if it's not 1
	//(DDRAW::?)
	//
	// 2021: maybe because SetMaximumFrameLatency(1) was 
	// called below? NOTE (1) may help for input feel???
	//
	pps.BackBufferCount = 2; //1; //3; //1; //2
	pps.BackBufferWidth = w; //hack
	pps.BackBufferHeight = h; //hack	
	pps.BackBufferFormat = bpp;

	//the backbuffer is not antialiased
	pps.MultiSampleType = D3DMULTISAMPLE_NONE;
	pps.MultiSampleQuality = 0;

	DWORD quality = 0; //not configurable at this time

	D3DMULTISAMPLE_TYPE msaa = D3DMULTISAMPLE_NONE; //0

	if(DDRAW::doMSAA) //REMOVE ME??
	{
		//D3DSWAPEFFECT_FLIPEX may forbid this:
		//"MSAA swapchains are not directly supported in flip model, so the app will need to do an MSAA resolve before issuing the Present."
		assert(0);

		msaa = D3DMULTISAMPLE_NONMASKABLE;
		
		DWORD q = 16, Q = 16;

		for(int i=0;msaa&&i<2;i++)
		{		
			D3DFORMAT test = i==1?fxbpp:dsbpp;

			if(test==D3DFMT_UNKNOWN) test = D3DFMT_X8R8G8B8; //hack

			if(proxy9->CheckDeviceMultiSampleType
			(query9->adapter,dev,test,!DDRAW::fullscreen,msaa,&Q)!=D3D_OK) 
			msaa = D3DMULTISAMPLE_NONE;

			q = min(q,Q);
		}

		quality = 0; //q?q-1:0;
	}
		
	pps.hDeviceWindow = DDRAW::window;
	pps.Windowed = !DDRAW::fullscreen;

	//Disable doFlipEx without Windows 7+
	if(DDRAW::doFlipEx&&!D3D9C::DLL::Direct3DCreate9Ex
	||GetVersion()<0x106) DDRAW::doFlipEx = false;

	//trying to override if D3DSWAPEFFECT_FLIPEX... 
	pps.PresentationInterval = DDRAW::isSynchronized?
	D3DPRESENT_INTERVAL_ONE:D3DPRESENT_INTERVAL_IMMEDIATE; //DEFAULT	

	if(DDRAW::doFlipEx&&pps.Windowed) 
	{
		/*2020: I'm trying to average the timing frame to frame
		3 seems excessive?
		//I think 3 evens things out to be as good as it can be
		//if the timing was managed by DDRAW::doFlipEX_MaxTicks
		pps.BackBufferCount = 3;*/
		pps.BackBufferCount = DDRAW::doTripleBuffering?2:1;
		pps.SwapEffect = (D3DSWAPEFFECT)5; //D3DSWAPEFFECT_FLIPEX;
		//I think this leaves no hope for
		//throttling
		//pps.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	}
	else if(DDRAW::doFlipDiscard||DDRAW::doFlipEx) //August 2021
	{
		//dx_d3d9c_present now fails unless COPY is used like the
		//documentation says to use
		pps.SwapEffect = D3DSWAPEFFECT_DISCARD; //game like
	}
	else pps.SwapEffect = D3DSWAPEFFECT_COPY; //tool like
								
	//now deferring depthbits
	pps.EnableAutoDepthStencil = 0; 
	pps.AutoDepthStencilFormat = dsbpp;

	pps.Flags =			
	(pps.EnableAutoDepthStencil
	?D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL:0)
//	|D3DPRESENTFLAG_DEVICECLIP;	//D3DSWAPEFFECT?
//	|D3DPRESENTFLAG_NOAUTOROTATE; //potential optimization	
;
		
	//note: required to GetDC backbuffer
	/*GetDC returning EX::window HDC for now*/
	//if(!msaa) //hack: does not fly for multisampling
	//pps.Flags|=D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

	pps.FullScreen_RefreshRateInHz =
	DDRAW::fullscreen?60:0; //primary.dwRefreshRate; 
	
	//if(DDRAW::fullscreen) //testing (seems to be ignored)
	//pps.PresentationInterval = D3DPRESENT_INTERVAL_TWO;

	//note: requires Desktop Window Manager (DWM)
	//support Aero and taskbar thumbnails under Vista/7 
//	pps.PresentationInterval|=D3DPRESENT_VIDEO_RESTRICT_TO_MONITOR;

	DWORD behavior = dev==D3DDEVTYPE_HAL?
	D3DCREATE_HARDWARE_VERTEXPROCESSING:D3DCREATE_SOFTWARE_VERTEXPROCESSING;

	if(DDRAW::doMultiThreaded) behavior|=D3DCREATE_MULTITHREADED;

	IDirect3DDevice9 *d = 0;

	HWND wth = DDRAW::fullscreen?0:DDRAW::window; //???
		
	assert(out); do //NEW: triple buffering?
	{
		if(D3D9C::DLL::Direct3DCreate9Ex)
		{
			D3DDISPLAYMODEEX dmEx, *mode = 0; 
		
			if(DDRAW::fullscreen) //docs do not say so 
			{
				dmEx.Size = sizeof(D3DDISPLAYMODEEX);
				dmEx.Width = w;
				dmEx.Height = h;
				dmEx.Format = bpp; 
				
				//0 is erratic
				//trying to match //pps.FullScreen_RefreshRateInHz
				dmEx.RefreshRate = primary.dwRefreshRate;
				pps.FullScreen_RefreshRateInHz = dmEx.RefreshRate;

				dmEx.ScanLineOrdering = DDRAW::isInterlaced?
				D3DSCANLINEORDERING_INTERLACED:D3DSCANLINEORDERING_PROGRESSIVE;

				mode = &dmEx; //but "NULL" must be passed in windowed mode
			}
		
			IDirect3DDevice9Ex* &dEx = *(IDirect3DDevice9Ex**)&d;

			//	int retries = 0; //debugging

retry:		if(out=proxy9->CreateDeviceEx(query9->adapter,dev,wth,behavior,&pps,mode,&dEx))
			pps.FullScreen_RefreshRateInHz = 0;

			if(out!=D3D_OK)
			{
				//FIX ME //DUPLICATE
				if(out==E_ACCESSDENIED&&dx_d3d9c_old_device) //YUCK
				{
					dx_d3d9c_force_release_old_device(); goto retry;
				}

				//NEW: try fewer buffers before giving up on IDirect3DDevice9Ex
				//just in case hardware/memory imposes a limit... in which case
				//the limit is probably among advertised capabilities, but it's
				//easier to find out this way
				if(1==pps.BackBufferCount) 
				{
					D3D9C::DLL::Direct3DCreate9Ex = 0; 
					if(pps.SwapEffect==(D3DSWAPEFFECT)5)
					{
						pps.SwapEffect = D3DSWAPEFFECT_DISCARD;					
						pps.PresentationInterval = DDRAW::isSynchronized?
						D3DPRESENT_INTERVAL_ONE:D3DPRESENT_INTERVAL_IMMEDIATE; //DEFAULT	
					}

					//OBSOLETE? I'm thinking there cannot possibly be any benefit
					//in trying this, but may as well for back-compatibility sake
					goto without_Ex;
				}
			}
			else if(pps.SwapEffect==(D3DSWAPEFFECT)5)
			{
				//dEx->SetMaximumFrameLatency(1); //2021??????????????
			}
			dx_d3d9c_old_device = 0; //2021
		}
		else without_Ex: //should implicate Windows XP (or a legit error)
		{
			out = proxy9->CreateDevice(query9->adapter,dev,wth,behavior,&pps,&d);
			assert(out||(GetVersion()&0xFF)<6);
		}

	}while(out!=D3D_OK&&--pps.BackBufferCount);
					 
	if(out==D3D_OK) d->SetDialogBoxMode(TRUE);

	bpp = pps.BackBufferFormat;
	
	//multisampling must match the render target
	if(!pps.EnableAutoDepthStencil&&out==D3D_OK)
	{
		//2018: adding... unsure if helpful
		//pps.Flags|=D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

		if(dsbpp==D3DFMT_D16) 
		if(proxy9->CheckDepthStencilMatch(query9->adapter,dev,bpp,fxbpp,D3DFMT_D24X8)==D3D_OK) 
		dsbpp = D3DFMT_D24X8;		
		if(dsbpp==D3DFMT_D24X8)
		if(proxy9->CheckDepthStencilMatch(query9->adapter,dev,bpp,fxbpp,D3DFMT_D32)==D3D_OK) 
		dsbpp = D3DFMT_D32;
		
		//2021: fx9_effects_buffers_require_reevaluation does the 
		//actual work to allocate the depth-stencil buffers below

		if(DDRAW::DepthStencilSurface7) //2021		
		DDRAW::DepthStencilSurface7->query9->format = dsbpp;
		else assert(0);
	}
	else assert(0); //2021
	
	DDRAW::IDirect3DDevice7 *p = 0;

	if(out==D3D_OK)
	{	
		assert(!query->d3ddevice); //assuming singular device

		p = new DDRAW::IDirect3DDevice7('dx9c');

		p->query->d3d = query; query->d3ddevice = p->query;		
										 		
		p->proxy9 = (IDirect3DDevice9Ex*)d;		
		
		p->AddRef(); //obsolete? see Release method		

	//	p->query9->depthstencil = depthstencil; 

		p->query9->emulate = 0; 

		if(D3D9C::doEmulate)
		{
			if(*x==DX::IID_IDirect3DRGBDevice) p->query9->emulate = 'rgb';
			if(*x==DX::IID_IDirect3DHALDevice) p->query9->emulate = 'hal';
			if(*x==DX::IID_IDirect3DTnLHalDevice) p->query9->emulate = 'tnl';
		}
		
		bool sw = p->query9->isSoftware = dev==D3DDEVTYPE_SW;	
		
		//REMOVE ME
		dx_d3d9c_defaultshader = 0;
		if(!sw&&dx_d3d9c_df)
		d->CreatePixelShader(dx_d3d9c_df,&dx_d3d9c_defaultshader);
		dx_d3d9c_colorkeyshader = dx_d3d9c_defaultshader;
		if(!sw&&dx_d3d9c_ck&&dx_d3d9c_imitate_colorkey) 
		d->CreatePixelShader(dx_d3d9c_ck,&dx_d3d9c_colorkeyshader);			
		dx_d3d9c_effectsshader = 0;
		if(!sw&&dx_d3d9c_fx) 
		d->CreatePixelShader(dx_d3d9c_fx,&dx_d3d9c_effectsshader);		
		if(!sw&&dx_d3d9c_cs[0])
		{
			d->CreatePixelShader(dx_d3d9c_cs[0],&dx_d3d9c_clearshader);
			d->CreateVertexShader(dx_d3d9c_cs[1],&dx_d3d9c_clearshader_vs);
		}

		DDRAW::ff = !dx_d3d9c_defaultshader; 
	}

	//REMOVE ME
	//NOTE: this just indicates shaders aren't available period.
	if(!dx_d3d9c_effectsshader)
	DDRAW::doSuperSampling = false;
	
	UINT ww = w, hh = h;
	DDRAW::doSuperSamplingMul(ww,hh);

	if(out==D3D_OK)
	{	
		IDirect3DSurface9 *bbuffer = 0;
		if(pps.Flags&D3DPRESENTFLAG_LOCKABLE_BACKBUFFER
		 &&d->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&bbuffer)==D3D_OK)
		{
			//TODO? could override proxy9?
			//dx_d3d9c_backbuffer_pitch = 
			DDRAW::BackBufferSurface7->queryX->pitch = dx_d3d9c_pitch(bbuffer);
			bbuffer->Release();
		}
		else //not ideal //BOGUS?
		{
			//dx_d3d9c_backbuffer_pitch = 
			DDRAW::BackBufferSurface7->queryX->pitch = 
			pps.BackBufferWidth*dx_d3d9c_format(bpp).dwRGBBitCount/8;
		}
		//dx_d3d9c_backbuffer_format = bpp;
		DDRAW::BackBufferSurface7->query9->format = bpp;

		//2021: because of supersampling can't spoof effects 
		//buffer when requesting backbuffer descriptor (GetDesc)
		//dx_d3d9c_rendertarget_pitch = dx_d3d9c_backbuffer_pitch; 
		//dx_d3d9c_rendertarget_format = bpp; //defaults
		assert(fxbpp==bpp);
		if(dev!=D3DDEVTYPE_HAL||!dx_d3d9c_effectsshader)
		fxbpp = D3DFMT_UNKNOWN;
		//dx_d3d9c_rendertarget_format = fxbpp;		
		DDRAW::mrtargets9[0] = fxbpp;
		if(msaa) //REMOVE ME?
		{
			//2021: fx9_effects_buffers_require_reevaluation
			//needs to check query9->multisample to know if 
			//MSAA is enabled... this is a legacy thing that
			//would probably be better ripped out, but that
			//takes work too

			//NOTE: for some reason this required an effects buffer to downsample
			//into (can it work with a back buffer?)
			//if(p->query9->effects[0])
			//if(dx_d3d9c_rendertarget_format)]
			if(DDRAW::mrtargets9[0])
			if(!d->CreateRenderTarget(ww,hh,fxbpp,msaa,quality,0,&p->query9->multisample,0))
			{
				d->SetRenderTarget(0,p->query9->multisample); //2021
			}
			else assert(p->query9->multisample);
		}
		//2021: this now does Z+FX+MRT
		//
		//
		//
		DDRAW::Direct3DDevice7 = p;
		DDRAW::fx9_effects_buffers_require_reevaluation(); //2021
		//
		//
		//
		//MRT WRT D3DRS_COLORWRITEENABLE123
		for(int i=1;i<4;i++)
		{
			//NOTE: fx9_effects_buffers_require_reevaluation should have
			//already allocated these 
			IDirect3DSurface9 *rt = 0;
			IDirect3DTexture9* &t = p->query9->mrtargets[i-1];
			if(t&&!t->GetSurfaceLevel(0,&rt)&&d->SetRenderTarget(i,rt))
			{
				t->Release(); t = 0; assert(0); 
			}
			if(rt) rt->Release(); 
			d->SetRenderState((D3DRENDERSTATETYPE)((int)D3DRS_COLORWRITEENABLE1+i-1),t?0xF:0);
		}	
	  
		if(DDRAW::doGamma) //ANCIENT/EXPERIMENTAL/IMPRACTICAL EMULATION OF SOM'S GAMMA RAMP
		{
			d->CreateTexture(256,1,1,D3DUSAGE_DYNAMIC,D3DFMT_L16,D3DPOOL_DEFAULT,&p->query9->gamma[0],0);
			d->CreateTexture(256,1,1,D3DUSAGE_DYNAMIC,D3DFMT_L16,D3DPOOL_DEFAULT,&p->query9->gamma[1],0);

			dx_d3d9c_gammaramp(p->query9->gamma[0],0); dx_d3d9c_gammaramp(p->query9->gamma[1],0);
		}
						 
		for(int i=DDRAW::doWhite;i>=0;i--)
		{
			auto &bw = i?p->query9->white:p->query9->black;

			if(!d->CreateTexture(1,1,1,D3DUSAGE_DYNAMIC,D3DFMT_A8,D3DPOOL_DEFAULT,&bw,0))
			{
				D3DLOCKED_RECT lock; 			
				if(bw->LockRect(0,&lock,0,0)==D3D_OK)
				{
					*(BYTE*)lock.pBits = i?255:0; bw->UnlockRect(0);
				}
				else //PARANOID??? 
				{
					bw->Release(); bw = 0;
				}
			}
			assert(bw); //2021
		}

		if(DDRAW::doDither)
		if(d->CreateTexture(8,8,1,D3DUSAGE_DYNAMIC,D3DFMT_A8,D3DPOOL_DEFAULT,&p->query9->dither,0)==D3D_OK)
		{	
			if(!dx_d3d9c_dither(p->query9->dither,65))
			{
				p->query9->dither->Release(); p->query9->dither = 0;
			}
			else if(1) //hack: assuming texture 2 unusued
			{
					//REMINDER: TEX0/TEX1 are really 1/2 (very misleading)

				//NOTE: black is the first MRT texture?!
				d->SetTexture(DDRAW_TEX0,p->query9->black); //???
				d->SetTexture(DDRAW_TEX1,p->query9->dither); 
							
					/*2021: DDITHER is knocked out in dx_d3d9c_prepshaders
					//and I really don't want to enter this code
					//today with everything else going on
				//there are limits to wrapping (wrap for uv gen effect)
				d->SetSamplerState(DDRAW_TEX1,D3DSAMP_ADDRESSU,D3DTADDRESS_WRAP);
				d->SetSamplerState(DDRAW_TEX1,D3DSAMP_ADDRESSV,D3DTADDRESS_WRAP);
				d->SetSamplerState(DDRAW_TEX1,D3DSAMP_MAGFILTER,D3DTEXF_POINT);
				d->SetSamplerState(DDRAW_TEX1,D3DSAMP_MINFILTER,D3DTEXF_POINT);

				//uv generation
				int g = DDRAW::bitsperpixel==16&&dx_d3d9c_ck?DDRAW_TEX0:DDRAW_TEX1;
				
				d->SetTextureStageState(g,D3DTSS_TEXCOORDINDEX,D3DTSS_TCI_CAMERASPACEPOSITION|2);
				d->SetTextureStageState(g,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_COUNT3);
				d->SetTextureStageState(g,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
				d->SetTextureStageState(g,D3DTSS_COLORARG1,D3DTA_TEXTURE);
				d->SetTextureStageState(g,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);
				d->SetTextureStageState(g,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);		
				d->SetSamplerState(g,D3DSAMP_ADDRESSU,D3DTADDRESS_WRAP);
				d->SetSamplerState(g,D3DSAMP_ADDRESSV,D3DTADDRESS_WRAP);
				d->SetSamplerState(g,D3DSAMP_MAGFILTER,D3DTEXF_POINT);
				d->SetSamplerState(g,D3DSAMP_MINFILTER,D3DTEXF_POINT);
					*/
			}
		}
		d->SetRenderState(D3DRS_DITHERENABLE,0); //paranoia
		
		D3DCAPS9 caps; d->GetDeviceCaps(&caps);
		{
			/*unset even for proxy9->GetDeviceCaps(adapter)???
			//2021: assuming D3DSAMP_SRGBTEXTURE present if so? 
			if(~caps.Caps3&D3DCAPS3_LINEAR_TO_SRGB_PRESENTATION)
			DDRAW::sRGB = 0;*/

			//using user mipmap routine if provided
			if(DDRAW::mipmap||~caps.Caps2&D3DCAPS2_CANAUTOGENMIPMAP
			||proxy9->CheckDeviceFormat(query9->adapter,dev,bpp,D3DUSAGE_DYNAMIC|D3DUSAGE_AUTOGENMIPMAP,D3DRTYPE_TEXTURE,D3DFMT_A8R8G8B8)!=D3D_OK
			||proxy9->CheckDeviceFormat(query9->adapter,dev,bpp,D3DUSAGE_DYNAMIC|D3DUSAGE_AUTOGENMIPMAP,D3DRTYPE_TEXTURE,D3DFMT_A1R5G5B5)!=D3D_OK)
			dx_d3d9c_autogen_mipmaps = false;

			dx_d3d9c_anisotropy_max = caps.MaxAnisotropy;
			
			//2021: saving caps simplifies dx.d3d9X.cpp
			dx_d3d9c_d3ddrivercaps(caps,p->queryX->caps);
			DWORD zcap = 0; switch(dsbpp)
			{
			case D3DFMT_D16: zcap = DDBD_16; break;
			case D3DFMT_D24X8: zcap = DDBD_24; break;
			case D3DFMT_D32: zcap = DDBD_32; break;
			}
			p->queryX->caps.dwDeviceZBufferBitDepth = zcap;
			DWORD sw = dev==D3DDEVTYPE_SW?D3DUSAGE_SOFTWAREPROCESSING:0;
			for(int i=0;i<dx_d3d9c_formats_enumN;i++)
			{
				switch(dx_d3d9c_formats_enum[i])
				{
				case D3DFMT_P8: 
					
					//Hack: Sword of Moonlight will try to use paletted 
					//textures in software device mode, which are generally
					//incompatible with the GDI routines used to populate them
					if(/*sw*/1) continue; //2021
				
				case D3DFMT_A2R10G10B10: 					
					
					continue; //2021: this isn't a real texture format
				}
				if(!proxy9->CheckDeviceFormat(query9->adapter,dev,D3DFMT_X8R8G8B8,sw,D3DRTYPE_TEXTURE,dx_d3d9c_formats_enum[i]))
				p->queryX->texture_format_mask|=1<<i;
			}
		}
					
		DDRAW::lights = 0; //???
		//DDRAW::refresh_state_dependent_shader_constants(); //too soon?

		if(DDRAW::sRGB&&!DDRAW::shader)
		{
			//going to assume this works if shaders are
			//working? could set DDRAW::sRGB to 0 if it
			//weren't already too late to reset shaders
			d->SetSamplerState(0,D3DSAMP_SRGBTEXTURE,1);
			if(p->query9->effects[1])
			d->SetSamplerState(1,D3DSAMP_SRGBTEXTURE,1);
		}

		out = D3D_OK;
	}
	else DDRAW_LEVEL(7) << "CreateDevice FAILED\n"; 

	DDRAW::mrtargets9[0] = d?fxbpp:D3DFMT_UNKNOWN;	

	*z = DDRAW::Direct3DDevice7 = p; 
	
	//funny, software creates the shaders but 
	//then has other probs later. Should investigate
	if(p&&!p->query9->isSoftware)
	{
		DDRAW::vshaders9_pshaders9_require_reevaluation();		
		DDRAW::refresh_state_dependent_shader_constants();
	}

	//setup scene or MSAA render target
	dx_d3d9X_enableeffectsshader(DDRAW::doEffects); 
	
	//2018: PlayStation VR
	if(DDRAW::inStereo) DDRAW::stereo_update_SetStreamSource();

	dx_d3d9c_pshaders_noclip = dx_d3d9c_pshaders;

	DDRAW_POP_HACK(DIRECT3D7_CREATEDEVICE,IDirect3D7*,
	const GUID*&,DX::LPDIRECTDRAWSURFACE7&,DX::LPDIRECT3DDEVICE7*&)(&out,this,x,y,z);

	DDRAW_RETURN(out) 
}
HRESULT D3D9C::IDirect3D7::CreateVertexBuffer(DX::LPD3DVERTEXBUFFERDESC x, DX::LPDIRECT3DVERTEXBUFFER7 *y, DWORD z)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3D7::CreateVertexBuffer()\n";

	DDRAW_IF_NOT_DX9C_RETURN(!D3D_OK) 
		
	DDRAW_PUSH_HACK(DIRECT3D7_CREATEVERTEXBUFFER,IDirect3D7*,
	DX::LPD3DVERTEXBUFFERDESC&,DX::LPDIRECT3DVERTEXBUFFER7*&,DWORD&)(0,this,x,y,z);						  
	
	if(!x||!y) DDRAW_FINISH(D3D_OK) //short-circuit? //!D3D_OK //2022

	if(z!=0) DDRAW_LEVEL(7) << "PANIC! last argument non-zero\n";

	if(!DDRAW::Direct3DDevice7) 
	{
		DDRAW_FINISH(!D3D_OK) //todo: if necessary save buffer contents for later
	}

	DDRAW::IDirect3DDevice7 *d3dd = DDRAW::Direct3DDevice7;
	
	UINT sizeofFVF = dx_d3d9c_sizeofFVF(x->dwFVF);
	UINT mem = sizeofFVF*x->dwNumVertices;	
																
	D3DPOOL pool = D3DPOOL_DEFAULT; 	
	if(x->dwCaps&D3DVBCAPS_SYSTEMMEMORY) pool = D3DPOOL_SYSTEMMEM;
	
	//required updating buffer (more than once)
	DWORD usage = D3DUSAGE_DYNAMIC; 
	//d3d9d.dll: Direct3D9: (WARN) :
	//Vertexbuffer created with POOL_DEFAULT but WRITEONLY not set. Performance penalty could be severe.
	if(pool==D3DPOOL_DEFAULT) usage|=D3DUSAGE_WRITEONLY;
	if(x->dwCaps&D3DVBCAPS_WRITEONLY) usage|=D3DUSAGE_WRITEONLY;
	if(x->dwCaps&D3DVBCAPS_DONOTCLIP) usage|=D3DUSAGE_DONOTCLIP;	

	IDirect3DVertexBuffer9 *b = 0;	

	//NOTE: mem is used to get the size from the descriptor, but unfilled
	out = d3dd->proxy9->CreateVertexBuffer(mem,usage,x->dwFVF,pool,&b,0);

	if(out==D3D_OK)
	{					   
		DDRAW::IDirect3DVertexBuffer7 *p = new DDRAW::IDirect3DVertexBuffer7('dx9c');

		p->proxy9 = b; 
		p->query9->FVF = x->dwFVF; 
		p->query9->sizeofFVF = sizeofFVF;
		p->query9->mixed_mode_lock = mem;
		p->query9->mixed_mode_size = mem;
		
		if(dx_d3d9c_immediate_mode) 
		{
			p->query9->upbuffer = new char[mem]; assert(mem);
		}
		/*else //if(DDRAW::inStereo) //EXPERIMENTAL
		{
			if(p->query9->stereoVD=dx_d3d9c_stereoVD(x->dwFVF))
			{
				p->query9->stereoVD->AddRef();
			}
		}*/

		*y = (DX::LPDIRECT3DVERTEXBUFFER7)p;
	}

	DDRAW_POP_HACK(DIRECT3D7_CREATEVERTEXBUFFER,IDirect3D7*,
	DX::LPD3DVERTEXBUFFERDESC&,DX::LPDIRECT3DVERTEXBUFFER7*&,DWORD&)(&out,this,x,y,z);

	DDRAW_RETURN(out)
}
HRESULT D3D9C::IDirect3D7::EnumZBufferFormats(REFCLSID x, DX::LPD3DENUMPIXELFORMATSCALLBACK y, LPVOID z)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3D7::EnumZBufferFormats()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK)

	LPOLESTR p; if(StringFromCLSID(x,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';

	D3DDEVTYPE dev = x==DX::IID_IDirect3DRGBDevice?D3DDEVTYPE_SW:D3DDEVTYPE_HAL;

	D3DFORMAT rgb = D3DFMT_X8R8G8B8;		
	
	for(int pass=0;pass<2;pass++)
	{
		D3DFORMAT Z = pass?D3DFMT_D24X8:D3DFMT_D16; //D3DFMT_D32 

		if(proxy9->CheckDepthStencilMatch(query9->adapter,dev,rgb,rgb,Z)==D3D_OK)
		{
			DX::DDPIXELFORMAT ddpf; ddpf.dwSize = sizeof(DX::DDPIXELFORMAT);

			ddpf.dwFlags = DDPF_ZBUFFER; //|DDPF_STENCILBUFFER;

			ddpf.dwZBufferBitDepth = pass?24:16;
			ddpf.dwStencilBitDepth = 0;

			ddpf.dwZBitMask = pass?0xFFFFFF00:0xFFFF0000;
		
			ddpf.dwStencilBitMask = 0x00000000;

			if(y(&ddpf,z)==DDENUMRET_CANCEL) return D3D_OK;
		}
	}		

	return D3D_OK; 
}






HRESULT D3D9C::IDirect3DDevice7::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DDevice7::QueryInterface()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK)

	LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';

	DDRAW_RETURN(!S_OK)
		
	return proxy9->QueryInterface(riid,ppvObj);
}
ULONG D3D9C::IDirect3DDevice7::AddRef()
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DDevice7::AddRef()\n";

	DDRAW_ADDREF_RETURN(0)
}
ULONG D3D9C::IDirect3DDevice7::Release()
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DDevice7::Release()\n";

	ULONG cmp = clientrefs, cmp2 = 0;

	if(clientrefs==1)
	{
		//NOTE: I think SOM may have a bug in it with respect
		//to releasing this object, I'm not sure. a standalone
		//game seems to fully release its device but a "debug"
		//game doesn't... even though som_rt.exe is no longer
		//a factor... I can't imagine what might explain this

		dx_d3d9c_force_release_old_device(); //???

		int a = proxy9->AddRef(); //25?
		int b = proxy9->Release(); //26?
		if(0<b)
		{
			//HACK: ensure dx_d3d9c_force_release_old_device
			//gets a live memory pointer
			proxy9->AddRef();

			//NOTE: Release returns 101 references at this point
			//I assume that means every COM object allocated via
			//the d3ddev has a back-reference to it since there's
			//no other sources of rerefences. dx_d3d9c_old_device
			//has to be released to make a new device on the same
			//adapter when it returns E_ACCESSDENIED

			dx_d3d9c_old_device = proxy9; //TESTING
		}
	}

	DDRAW_IF_NOT_TARGET_AND_RELEASE(out,0)
	
	//REMOVE ME?
	if(out==1) return dx_ddraw_try_direct3ddevice7_release();
	
	if(out==0) 
	{
		delete this; DDRAW::Direct3DDevice7 = 0;

		dx_d3d9c_resetdevice(); DDRAW::onReset();
	}	
	return out;
}
HRESULT D3D9C::IDirect3DDevice7::GetCaps(DX::LPD3DDEVICEDESC7 x)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DDevice7::GetCaps()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK)
	
	if(!x)
	{
		DDRAW_ALERT(2) << "Known misbehavior: Device capabilities requested but descriptor was not provided. Returning error.\n";\

		return !D3D_OK; //DDRAW_RETURN(!D3D_OK)
	}

	#if 1 //NEW WAY (supports dx.d3d9X.cpp)
	
		*x = queryX->caps; //CreateDevice
		assert(x->dwDeviceZBufferBitDepth); //assuming has zbuffer

	#else //OLD WAY (could use p->query9->depthstencil) 
	
		D3DCAPS9 caps9; 
		if(proxy9->GetDeviceCaps(&caps9))
		DDRAW_RETURN(!D3D_OK)	
		dx_d3d9c_d3ddrivercaps(caps9,*x);
		D3DSURFACE_DESC descZ;
		if(p->query9->depthstencil->GetDesc(&descZ))
		assert(0);
		else switch(descZ.Format)
		{
		case D3DFMT_D16: x->dwDeviceZBufferBitDepth = DDBD_16; break;
		case D3DFMT_D24X8: x->dwDeviceZBufferBitDepth = DDBD_24; break;
		case D3DFMT_D32: x->dwDeviceZBufferBitDepth = DDBD_32; break;
		}

	#endif

	return D3D_OK;
}
HRESULT D3D9C::IDirect3DDevice7::EnumTextureFormats(DX::LPD3DENUMPIXELFORMATSCALLBACK x, LPVOID y)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DDevice7::EnumTextureFormats()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK)
	
	if(!x) 
	{
		DDRAW_ALERT(2) << "Known misbehavior: Enumeration requested but callback was not provided. Returning error.\n";\

		return !D3D_OK; //DDRAW_RETURN(!D3D_OK)
	}

	DX::DDPIXELFORMAT ddpf;

	for(int i=0;i<dx_d3d9c_formats_enumN;i++)	
	{
		if((1<<i)&queryX->texture_format_mask)
		{
			ddpf = dx_d3d9c_format(dx_d3d9c_formats_enum[i]);

			if(x(&ddpf,y)==DDENUMRET_CANCEL) break;
		}
	}

	return D3D_OK; //DDRAW_RETURN(out)
}
HRESULT D3D9C::IDirect3DDevice7::BeginScene()
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DDevice7::BeginScene()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK)

	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_BEGINSCENE,IDirect3DDevice7*)(0,this);
								
	if(!DDRAW::inScene)
	if(DDRAW::target=='dx9c')
	out = proxy9->BeginScene(); else out = D3D_OK;

	if(out!=D3D_OK) 
	{
		dx_d3d9c_beginscene = dx_d3d9c_endscene = 0;

		DDRAW_ALERT(0) << "Alert! Begin/EndScene calls out of sync!\n";
	}
	else
	{
		DDRAW::inScene = true; dx_d3d9c_beginscene++;
	}

	if(dx_d3d9c_beginscene!=dx_d3d9c_endscene+1)
	{
		assert(dx_d3d9c_beginscene==dx_d3d9c_endscene+1); //breakpoint
	}

	DDRAW_POP_HACK(DIRECT3DDEVICE7_BEGINSCENE,IDirect3DDevice7*)(&out,this);

	DDRAW_RETURN(out)
}
HRESULT D3D9C::IDirect3DDevice7::EndScene()
{
	//WEIRD: AN ASSERT HERE AMOUNTS TO AN AUTOMATIC EXIT

	DDRAW_LEVEL(7) << "D3D9C::IDirect3DDevice7::EndScene()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK)
	
 	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_ENDSCENE,IDirect3DDevice7*)(0,this);

	if(DDRAW::inScene)
	if(DDRAW::target=='dx9c')
	out = proxy9->EndScene(); else out = D3D_OK;

	if(out!=D3D_OK) 
	{
		dx_d3d9c_beginscene = dx_d3d9c_endscene = 0; 

		DDRAW_ALERT(0) << "Alert! Begin/EndScene calls out of sync!\n";
	}
	else
	{
		DDRAW::inScene = false; dx_d3d9c_endscene++;
	}
		
	if(!/*assert*/(dx_d3d9c_beginscene==dx_d3d9c_endscene))
	{
		out = out; //breakpoint
	}

	DDRAW_POP_HACK(DIRECT3DDEVICE7_ENDSCENE,IDirect3DDevice7*)(&out,this);

	return out; //DDRAW_RETURN(out)
}
HRESULT D3D9C::IDirect3DDevice7::Clear(DWORD x, DX::LPD3DRECT y, DWORD z, DX::D3DCOLOR w, DX::D3DVALUE q, DWORD r)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DDevice7::Clear()\n";
 
	DDRAW_IF_NOT_DX9C_RETURN(!D3D_OK)

 	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_CLEAR,IDirect3DDevice7*,
	DWORD&,DX::LPD3DRECT&,DWORD&,DX::D3DCOLOR&,DX::D3DVALUE&,DWORD&)(0,this,x,y,z,w,q,r);
						  
	if(z==0) DDRAW_FINISH(DD_OK) //!DD_OK

	D3DRECT buf[4];
	for(DWORD i=0,iN=max(x,1);i<iN;i+=_countof(buf))	
	{
		D3DRECT *src;
		if(y&&(!DDRAW::doNothing||DDRAW::doSuperSampling))
		{
			for(DWORD j=0;i<x&&j<_countof(buf);j++)
			{
				auto &temp = buf[j]; 
				
				temp = ((D3DRECT*)y)[i+j];

				if(DDRAW::doNothing) goto ss;
				
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

				ss: DDRAW::doSuperSamplingMul(temp.x1,temp.x2,temp.y1,temp.y2);
			}			
			src = buf;
		}
		else src = (D3DRECT*)y+i;

		auto yy = y?src:0;

		DWORD xx = y?min(x-i,_countof(buf)):0;

		if(~z&D3DCLEAR_TARGET
		||0==DDRAW::doClearMRT
		||1==DDRAW::doClearMRT&&!dx_d3d9c_clearshader)
		{
			out = proxy9->Clear(xx,yy,z,w,q,r);
		}
		else if(2==DDRAW::doClearMRT) //2021
		{
			//I had originally implemented this but
			//commented it out. I think it's safe. it
			//seems to do slightly faster supersampling

			//this is needed to repair the viewport after
			//SetRenderTarget... mainly MinZ/MaxZ are reset
			//to 0,1 and for some reason it broke my KF2 demo
			//on certain screen resolution settings only
			D3DVIEWPORT9 vp; proxy9->GetViewport(&vp);

			IDirect3DSurface9 *mrt[4] = {0,0,0,0}; 			
			for(int i=0;i<4;i++)
			{		
				proxy9->GetRenderTarget(i,&mrt[i]);
				if(mrt[i]&&i!=0)
				proxy9->SetRenderTarget(i,0);
			}
			out = proxy9->Clear(xx,yy,z,w,q,r);
			for(int i=1;i<4;i++) if(mrt[i])
			{
				DWORD rgba; if(DDRAW::ClearMRT[i])
				{
					float r = 0, g = 0, b = 0, a = 0;
					sscanf(DDRAW::ClearMRT[i],"%f,%f,%f,%f",&r,&g,&b,&a);
					rgba = D3DCOLOR_COLORVALUE(r,g,b,a);
				}
				else rgba = w;
				proxy9->SetRenderTarget(0,mrt[i]);
				proxy9->Clear(xx,yy,D3DCLEAR_TARGET,rgba,0,0);
			}
			for(int i=0;i<4;i++) if(mrt[i])
			{
				proxy9->SetRenderTarget(i,mrt[i]); mrt[i]->Release();
			}

			proxy9->SetViewport(&vp);
		}
		else //draw fullscreen quad
		{		
			//UNTESTED (2021)
			auto d3dd9 = DDRAW::Direct3DDevice7->proxy9;
			if(!dx_d3d9c_bltstate) //2021
			d3dd9->CreateStateBlock(D3DSBT_ALL,&dx_d3d9c_bltstate);		
			else dx_d3d9c_bltstate->Capture();
			if(!dx_d3d9c_bltstate) DDRAW_FINISH(!D3D_OK);
			//TODO? CreateStateBlock
			/*DWORD fm,ze,zw,cw,ate,abe,zcmp; //enough?
			proxy9->GetRenderState(D3DRS_FILLMODE,&fm);
			proxy9->GetRenderState(D3DRS_ZENABLE,&ze);
			proxy9->GetRenderState(D3DRS_ZFUNC,&zcmp);
			proxy9->GetRenderState(D3DRS_ZWRITEENABLE,&zw);
			proxy9->GetRenderState(D3DRS_COLORWRITEENABLE,&cw);		
			proxy9->GetRenderState(D3DRS_ALPHATESTENABLE,&ate);		
			proxy9->GetRenderState(D3DRS_ALPHABLENDENABLE,&abe);*/
			proxy9->SetRenderState(D3DRS_FILLMODE,3);
			proxy9->SetRenderState(D3DRS_ZENABLE,1);
			proxy9->SetRenderState(D3DRS_ZFUNC,D3DCMP_ALWAYS);
			proxy9->SetRenderState(D3DRS_ZWRITEENABLE,z&D3DCLEAR_ZBUFFER?1:0);		
			proxy9->SetRenderState(D3DRS_COLORWRITEENABLE,z&D3DCLEAR_TARGET?0xF:0);						
			proxy9->SetRenderState(D3DRS_ALPHABLENDENABLE,0);	
			if(dx_d3d9c_ps_model<=2&&DDRAW::doFog&&DDRAW::inFog)
			proxy9->SetRenderState(D3DRS_FOGENABLE,0);		
			D3DVIEWPORT9 vp; 
			if(!y) proxy9->GetViewport(&vp);
			
			//IDirect3DPixelShader9 *ps = 0;
			//proxy9->GetPixelShader(&ps);
			proxy9->SetPixelShader(dx_d3d9c_clearshader);
			//IDirect3DVertexShader9 *vs = 0; //NEW
			//proxy9->GetVertexShader(&vs);
			proxy9->SetVertexShader(dx_d3d9c_clearshader_vs);		
			DWORD fvf = D3DFVF_XYZRHW|D3DFVF_DIFFUSE;
			float d; *(D3DCOLOR*)&d = w;
			float c = 0; //-0.5f; //black magic
			DDRAW::PushScene(); proxy9->SetFVF(fvf);
			
			for(DWORD k=max(1,xx);k-->0;yy++)
			{
				float l = 0,r,t = 0,b; if(y)
				{
					l = yy->x1; r = yy->x2; t = yy->y1; b = yy->y2;
				}
				else //and what about viewports?
				{
					l = vp.X; r = l+vp.Width; t = vp.Y; b = t+vp.Height;
				}		
				float fan[4*5] = {c+l,c+t,q,1,d, c+r,c+t,q,1,d, c+r,c+b,q,1,d, c+l,c+b,q,1,d};			

				proxy9->DrawPrimitiveUP(D3DPT_TRIANGLEFAN,2,fan,5*sizeof(float));
			}			
			DDRAW::PopScene(); 	
			/*2021
			proxy9->SetPixelShader(ps); 
			proxy9->SetVertexShader(vs);		
			proxy9->SetRenderState(D3DRS_FILLMODE,fm);
			proxy9->SetRenderState(D3DRS_ALPHATESTENABLE,ate);
			proxy9->SetRenderState(D3DRS_ALPHABLENDENABLE,abe);
			proxy9->SetRenderState(D3DRS_COLORWRITEENABLE,cw);
			proxy9->SetRenderState(D3DRS_ZWRITEENABLE,zw);
			proxy9->SetRenderState(D3DRS_ZENABLE,ze);
			proxy9->SetRenderState(D3DRS_ZFUNC,zcmp);	
			if(dx_d3d9c_ps_model<=2&&DDRAW::doFog)
			proxy9->SetRenderState(D3DRS_FOGENABLE,DDRAW::inFog);
			out = D3D_OK; //!!*/
			//NOTE: Apply is supposed to restore shader constants
			out = dx_d3d9c_bltstate->Apply();
		}	
	}

	DDRAW_POP_HACK(DIRECT3DDEVICE7_CLEAR,IDirect3DDevice7*,
	DWORD&,DX::LPD3DRECT&,DWORD&,DX::D3DCOLOR&,DX::D3DVALUE&,DWORD&)(&out,this,x,y,z,w,q,r);

	DDRAW_RETURN(out)
}
HRESULT D3D9C::IDirect3DDevice7::SetTransform(DX::D3DTRANSFORMSTATETYPE x, DX::LPD3DMATRIX y)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DDevice7::SetTransform()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK)

	//2020: correct? ApplyStateBlock?
	DDRAW_PUSH_HACK_IF(y!=DDRAW::getD3DMATRIX, 
	DIRECT3DDEVICE7_SETTRANSFORM,IDirect3DDevice7*,
	DX::D3DTRANSFORMSTATETYPE&,DX::LPD3DMATRIX&)(0,this,x,y);

	if(!y) DDRAW_FINISH(D3D_OK) //short-circuited?

	if(DDRAWLOG::debug>=7)
	{
		DDRAW_LEVEL(7) << " Transform matrix is " << x << '\n';
		
		DX::D3DVALUE *m = (DX::D3DVALUE*)y;

		const char *format = "%6.6f %6.6f %6.6f %6.6f\n";
		char out[96];
		sprintf(out,format,*m++,*m++,*m++,*m++); DDRAW_LEVEL(7) << out;
		sprintf(out,format,*m++,*m++,*m++,*m++); DDRAW_LEVEL(7) << out;
		sprintf(out,format,*m++,*m++,*m++,*m++); DDRAW_LEVEL(7) << out;
		sprintf(out,format,*m++,*m++,*m++,*m++); DDRAW_LEVEL(7) << out;
	}

	D3DTRANSFORMSTATETYPE x9 = (D3DTRANSFORMSTATETYPE)x;

	switch(x)
	{
	case DX::D3DTRANSFORMSTATE_WORLD:  x9 = D3DTS_WORLDMATRIX(0); break;
	case DX::D3DTRANSFORMSTATE_WORLD1: x9 = D3DTS_WORLDMATRIX(1); break;
	case DX::D3DTRANSFORMSTATE_WORLD2: x9 = D3DTS_WORLDMATRIX(2); break;
	case DX::D3DTRANSFORMSTATE_WORLD3: x9 = D3DTS_WORLDMATRIX(3); break;

	case DX::D3DTRANSFORMSTATE_PROJECTION:
	
		if(DDRAW::psFarFrustum) 
		{
			float z = y->_43/(1.0f-y->_33);
			float c[4] = {z/y->_11,z/y->_22,z,1.0f/z};
			dx_d3d9X_psconstant(DDRAW::psFarFrustum,c);
		}							   
		break;
	}

	out = proxy9->SetTransform(x9,(D3DMATRIX*)y);

	dx_d3d9c_vstransform(1<<x);

	DDRAW_POP_HACK(DIRECT3DDEVICE7_SETTRANSFORM,IDirect3DDevice7*,
	DX::D3DTRANSFORMSTATETYPE&,DX::LPD3DMATRIX&)(&out,this,x,y);

	DDRAW_RETURN(out)
}
HRESULT D3D9C::IDirect3DDevice7::GetTransform(DX::D3DTRANSFORMSTATETYPE x, DX::LPD3DMATRIX y)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DDevice7::GetTransform()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK)
	
	DDRAW_PUSH_HACK_IF(y!=DDRAW::getD3DMATRIX,
	DIRECT3DDEVICE7_GETTRANSFORM,IDirect3DDevice7*,
	DX::D3DTRANSFORMSTATETYPE&,DX::LPD3DMATRIX&)(0,this,x,y);

	D3DTRANSFORMSTATETYPE x9 = (D3DTRANSFORMSTATETYPE)x;

	switch(x)
	{
	case DX::D3DTRANSFORMSTATE_WORLD:  x9 = D3DTS_WORLDMATRIX(0); break;
	case DX::D3DTRANSFORMSTATE_WORLD1: x9 = D3DTS_WORLDMATRIX(1); break;
	case DX::D3DTRANSFORMSTATE_WORLD2: x9 = D3DTS_WORLDMATRIX(2); break;
	case DX::D3DTRANSFORMSTATE_WORLD3: x9 = D3DTS_WORLDMATRIX(3); break;
	}

	out = proxy9->GetTransform(x9,(D3DMATRIX*)y);

	DDRAW_POP_HACK(DIRECT3DDEVICE7_GETTRANSFORM,IDirect3DDevice7*,
	DX::D3DTRANSFORMSTATETYPE&,DX::LPD3DMATRIX&)(&out,this,x,y);

	DDRAW_RETURN(out)
}
HRESULT D3D9C::IDirect3DDevice7::GetViewport(DX::LPD3DVIEWPORT7 x)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DDevice7::GetViewport()\n";

	DDRAW_IF_NOT_DX9C_RETURN(!D3D_OK) if(!x) DDRAW_RETURN(!D3D_OK)

	HRESULT out = proxy9->GetViewport((D3DVIEWPORT9*)x);
	
	if(x&&DDRAW::doSuperSampling) //EXPERIMENTAL
	{
		DDRAW::doSuperSamplingDiv(x->dwX,x->dwY,x->dwWidth,x->dwHeight);
	}
	if(x&&out==D3D_OK&&!DDRAW::doNothing)
	{
		//2020: this is actually rather complicated if scaling X/Y

		float s = DDRAW::doScaling?DDRAW::xyScaling[0]:1;
		float t = DDRAW::doScaling?DDRAW::xyScaling[1]:1;
		
		if(DDRAW::doMapping) 
		{
			x->dwX = (x->dwX-DDRAW::xyMapping[0])/s+0.5f;
			x->dwY = (x->dwY-DDRAW::xyMapping[1])/t+0.5f;
		}
		else if(DDRAW::doScaling) 
		{
			x->dwX = x->dwX*s+0.5f; x->dwY = x->dwY*t+0.5f;
		}

		if(DDRAW::doScaling) x->dwWidth = float(x->dwWidth)/s+0.5f;
		if(DDRAW::doScaling) x->dwHeight = float(x->dwHeight)/t+0.5f;
	}
		  
	DDRAW_RETURN(out) 
}
HRESULT D3D9C::IDirect3DDevice7::SetViewport(DX::LPD3DVIEWPORT7 x)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DDevice7::SetViewport()\n";

	DDRAW_IF_NOT_DX9C_RETURN(!D3D_OK) if(!x) DDRAW_RETURN(!D3D_OK)   
	
	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_SETVIEWPORT,IDirect3DDevice7*,
	DX::LPD3DVIEWPORT7&)(0,this,x);

	DX::D3DVIEWPORT7 temp;
	if(x&&(!DDRAW::doNothing||DDRAW::doSuperSampling)) 
	{
		if(DDRAW::doNothing){ temp = *x; goto ss; }

		//2020: this is actually rather complicated if scaling X/Y

		float s = DDRAW::doScaling?DDRAW::xyScaling[0]:1;
		float t = DDRAW::doScaling?DDRAW::xyScaling[1]:1;

		if(DDRAW::doMapping) 
		{
			temp.dwX = DDRAW::xyMapping[0]+x->dwX*s+0.5f;
			temp.dwY = DDRAW::xyMapping[1]+x->dwY*t+0.5f;
		}
		else
		{
			temp.dwX = x->dwX*s+0.5f; temp.dwY = x->dwY*t+0.5f;
		}

		if(DDRAW::doScaling) temp.dwWidth = s*x->dwWidth+0.5f;
		if(DDRAW::doScaling) temp.dwHeight = t*x->dwHeight+0.5f;
		
		temp.dvMinZ = x->dvMinZ; temp.dvMaxZ = x->dvMaxZ;
				
		ss: //EXPERIMENTAL
		{
			DDRAW::doSuperSamplingMul(temp.dwX,temp.dwY,temp.dwWidth,temp.dwHeight);
		}

		x = &temp; 
	}
	
	out = proxy9->SetViewport((D3DVIEWPORT9*)x);
			   
	DDRAW_POP_HACK(DIRECT3DDEVICE7_SETVIEWPORT,IDirect3DDevice7*,
	DX::LPD3DVIEWPORT7&)(&out,this,x);

	DDRAW_RETURN(out)
}
HRESULT D3D9C::IDirect3DDevice7::SetMaterial(DX::LPD3DMATERIAL7 x)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DDevice7::SetMaterial()\n";
 
	DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK)

	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_SETMATERIAL,IDirect3DDevice7*,
	DX::LPD3DMATERIAL7&)(0,this,x);

	assert(x); //2022: I think I finally found this bug at 448158
	//2021
	//2022 //still hitting 44DEA6
	if(!x) //not supported by d3d9 //??? 
	{
		//Note: this is probably an error condition even under DX7
		//return !D3D_OK;
		
//		dx_d3d9c_setmaterial = false; //return D3D_OK;

		static D3DMATERIAL9 temp = 
		{
			{1.0,1.0,1.0,1.0},
			{1.0,1.0,1.0,1.0},
			{0.0,0.0,0.0,0.0},
			{0.0,0.0,0.0,0.0},0.0f	  
		}; 

		x = (DX::D3DMATERIAL7*)&temp; //warning: not thread safe
	}
//	else dx_d3d9c_setmaterial = true;//*/
	//
	// NOTE: 448170 (som_db) returns 0 and 44DEA6 blindly passes
	// it to SetMaterial
	//
	if(!x) DDRAW_RETURN(!D3D_OK)

	assert(x->specular.r+x->specular.b+x->specular.g==0);

	dx_d3d9X_vsconstant(DDRAW::vsMaterialDiffuse,x->diffuse);
	dx_d3d9X_vsconstant(DDRAW::vsMaterialAmbient,x->ambient);
	dx_d3d9X_vsconstant(DDRAW::vsMaterialEmitted,x->emissive);

	out = proxy9->SetMaterial((D3DMATERIAL9*)x); //looks safe
	
	DDRAW_POP_HACK(DIRECT3DDEVICE7_SETMATERIAL,IDirect3DDevice7*,
	DX::LPD3DMATERIAL7&)(&out,this,x);

	DDRAW_RETURN(out) 
}
HRESULT D3D9C::IDirect3DDevice7::GetMaterial(DX::LPD3DMATERIAL7 x)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DDevice7::GetMaterial()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK)
	
	DDRAW_RETURN(proxy9->GetMaterial((D3DMATERIAL9*)x))
}
HRESULT D3D9C::IDirect3DDevice7::SetLight(DWORD x, DX::LPD3DLIGHT7 y)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DDevice7::SetLight()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK)
		
	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_SETLIGHT,IDirect3DDevice7*,
	DWORD&,DX::LPD3DLIGHT7&)(0,this,x,y);

	DDRAW_LEVEL(7) << ' ' << x << '\n';
							  
	D3DLIGHT9 l = *(D3DLIGHT9*)y;

	if(!y) DDRAW_FINISH(proxy9->SetLight(x,0))

	//Direct3D9 does not allow negative attentuation
	if(l.Attenuation0<0.0f) l.Attenuation0 = 0.0f;
	if(l.Attenuation1<0.0f) l.Attenuation1 = 0.0f;
	if(l.Attenuation2<0.0f) l.Attenuation2 = 0.0f;

	out = proxy9->SetLight(x,&l);

	DDRAW_POP_HACK(DIRECT3DDEVICE7_SETLIGHT,IDirect3DDevice7*,
	DWORD&,DX::LPD3DLIGHT7&)(&out,this,x,y);

	DDRAW_RETURN(out) //appears identical
}
HRESULT D3D9C::IDirect3DDevice7::SetRenderState(DX::D3DRENDERSTATETYPE x, DWORD y)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DDevice7::SetRenderState()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK) //DX9X SUBROUTINE

	DDRAW_LEVEL(7) << " Render State is " << x << " (setting to " << y << ")\n";
	
	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_SETRENDERSTATE,IDirect3DDevice7*,
	DX::D3DRENDERSTATETYPE&,DWORD&)(0,this,x,y);

	#define CASE_(x) break; case DX::D3DRENDERSTATE_##x:
							  
	out = D3D_OK; switch(x)					 
	{
	default: assert(0); //break;
		
		DDRAW_PANIC(3) << " PANIC! Unhandled render state type (" << x << ") was passed\n";\

		finish: DDRAW_FINISH(out); //2021

	CASE_(ANTIALIAS) //note: seen (setting to 0)

		//DDRAW_ALERT(0) << " ALERT! ANTIALIAS state set under DirectX9\n";

		goto finish; //todo: enable multisampling
		
	CASE_(TEXTUREPERSPECTIVE) goto finish; //should be unnecessary
	CASE_(ZENABLE)   
    CASE_(FILLMODE)
    CASE_(SHADEMODE)
    CASE_(ZWRITEENABLE) 
    CASE_(ALPHATESTENABLE) 
    CASE_(LASTPIXEL)
	CASE_(SRCBLEND)	 //DDRAW_LEVEL(-1) << "src: " << y << '\n';
    CASE_(DESTBLEND) //DDRAW_LEVEL(-1) << "dst: " << y << '\n';

		//dx_d3d9X_vsconstant(DDRAW::vsDebugColor,DDRAW::DebugPalette[y%12],'rgba');

    CASE_(CULLMODE)	
    CASE_(ZFUNC)		
    CASE_(ALPHAREF)
    CASE_(ALPHAFUNC)
    CASE_(DITHERENABLE) y = 0;
	CASE_(ALPHABLENDENABLE) 

		/*TYPO?
		//2021: WHY IS THIS COMMENTED OUT?!?!?!?!?! //WHAT DOES THIS COMMENT MEAN???
		//dx_d3d9c_alphablendenable = y?true:false; //works differenty under d3d9*/
		dx_d3d9c_alphablendenable = y?true:false; //???

		//return D3D_OK;

	CASE_(FOGENABLE) 
	
		DDRAW::inFog = y?true:false;

		//would like to avoid this but it's something
		//to think about when messing with this stuff
		//if(DDRAW::gl) queryGL->state.fog = DDRAW::inFog; //YUCK

		dx_d3d9X_vsconstant(DDRAW::vsFogFactors,y?1ul:0ul,'w');
		dx_d3d9X_psconstant(DDRAW::psFogFactors,y?1ul:0ul,'w');

		if(!DDRAW::doFog) y = 0; //ultimately disable

    CASE_(SPECULARENABLE)
	CASE_(ZVISIBLE)			out = !D3D_OK; goto finish;
	CASE_(STIPPLEDALPHA)	
		
		//DDRAW_LEVEL(0) << " ALERT! STIPPLEDALPHA state set under DirectX9\n";

		goto finish; 
	
	CASE_(FOGCOLOR)		  dx_d3d9X_psconstant(DDRAW::psFogColor,y,'rgba');
    CASE_(FOGTABLEMODE)
	case DX::D3DRENDERSTATE_FOGSTART: case DX::D3DRENDERSTATE_FOGEND:
	{
		FLOAT start = (FLOAT&)y, end = start;
		proxy9->GetRenderState((D3DRENDERSTATETYPE)(x^1),(DWORD*)&(x&1?start:end));
		start = end-start?1.0f/(end-start):FLT_MAX; //!!
		dx_d3d9X_vsconstant(DDRAW::vsFogFactors,start,'x');
		dx_d3d9X_psconstant(DDRAW::psFogFactors,start,'x');
		dx_d3d9X_vsconstant(DDRAW::vsFogFactors,end,'y');
		dx_d3d9X_psconstant(DDRAW::psFogFactors,end,'y');
	}
	break;
    CASE_(FOGDENSITY)	  dx_d3d9X_vsconstant(DDRAW::vsFogFactors,*(float*)&y,'z');
						  dx_d3d9X_psconstant(DDRAW::psFogFactors,*(float*)&y,'z');
	CASE_(EDGEANTIALIAS) //note: seen (setting to 0)

		//DDRAW_ALERT(0) << " ALERT! EDGEANTIALIAS state set under DirectX9\n";

		goto finish; //todo: enable multisampling

	CASE_(COLORKEYENABLE) //is not reliable??
	
		/*2022: discontinuing this with NOCLIP but colorkey might
		//need to be considered for setting dx_d3d9c_pshaders_noclip
		if(DDRAW::TextureSurface7[0]
		 &&DDRAW::TextureSurface7[0]->query9->knockouts) //colorkey
		{
			dx_d3d9X_psconstant(DDRAW::psColorkey,y?1.0f:0.0f,'w');
		}
		else if(!y) dx_d3d9X_psconstant(DDRAW::psColorkey,0.0f,'w');
		*/

		dx_d3d9c_colorkeyenable = y?true:false; //not part of d3d9

		goto finish; 

	CASE_(ZBIAS) 
	{
		float bias = 0.0000001f*(int)y; //ARBITRARY (0-16)
		y = *(DWORD*)&bias;
		x = (DX::D3DRENDERSTATETYPE)D3DRS_DEPTHBIAS;
		//bias*=5000;
		if(y) bias = bias<0?-1:1;
		proxy9->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS,*(DWORD*)&bias);
	}
    CASE_(RANGEFOGENABLE)
    CASE_(STENCILENABLE)
    CASE_(STENCILFAIL)
    CASE_(STENCILZFAIL)
    CASE_(STENCILPASS)
    CASE_(STENCILFUNC)
    CASE_(STENCILREF)
    CASE_(STENCILMASK)
    CASE_(STENCILWRITEMASK)
    CASE_(TEXTUREFACTOR)
    CASE_(WRAP0)
    CASE_(WRAP1)
    CASE_(WRAP2)
    CASE_(WRAP3)
    CASE_(WRAP4)
    CASE_(WRAP5)
    CASE_(WRAP6)
    CASE_(WRAP7)

    CASE_(CLIPPING) y = 0;

    CASE_(LIGHTING)	DDRAW::isLit = y?true:false;

		dx_d3d9X_vsconstant(DDRAW::vsGlobalAmbient,y?1.0f:0.0f,'w');

	CASE_(EXTENTS) //note: seen	(setting to 0)

		//DDRAW_ALERT(0) << " ALERT! EXTENTS state set under DirectX9\n";

		goto finish; //todo: not sure

    CASE_(AMBIENT)		  
	
		y = y&0x00FFFFFF; y|=DDRAW::isLit?0xFF000000:0;

		dx_d3d9X_vsconstant(DDRAW::vsGlobalAmbient,y,'rgba');

    CASE_(FOGVERTEXMODE)

		//x = (DX::D3DRENDERSTATETYPE)D3DRS_FOGTABLEMODE; //debugging

    CASE_(COLORVERTEX)  assert(y);	 //testing
    CASE_(LOCALVIEWER)
    CASE_(NORMALIZENORMALS)
	CASE_(COLORKEYBLENDENABLE)

		assert(0); //important info

		goto finish; //todo: DDCOLORKEY interfaces

    CASE_(DIFFUSEMATERIALSOURCE)  assert(0); //important info
    CASE_(SPECULARMATERIALSOURCE) assert(0); //important info
    CASE_(AMBIENTMATERIALSOURCE)  //assert(0); //important info
    CASE_(EMISSIVEMATERIALSOURCE) assert(0); //important info
    CASE_(VERTEXBLEND)
    CASE_(CLIPPLANEENABLE) y = 0;break;
	case D3DRS_COLORWRITEENABLE: break;
	case D3DRS_COLORWRITEENABLE1:
	case D3DRS_COLORWRITEENABLE2:		
	case D3DRS_COLORWRITEENABLE3: //MRT
		
		if(query9->mrtargets[(int)x-190])
		{
			int i = (int)x-190; if(y) //enable 
			{
				IDirect3DBaseTexture9 *t = 0;
				proxy9->GetTexture(i+1,&t);
				if(t==query9->mrtargets[(int)x-190]) 
				proxy9->SetTexture(i+1,0);
				IDirect3DSurface9 *s = 0;
				query9->mrtargets[i]->GetSurfaceLevel(0,&s);			
				proxy9->SetRenderTarget((int)x-190+1,s);
				if(s) s->Release(); else assert(0);
				if(t) t->Release(); 
			}
			else //disable/make corresponding texture
			{
				proxy9->SetRenderTarget(i+1,0);
				proxy9->SetTexture(DDRAW_TEX0+i,query9->mrtargets[i]);
			}
		}
		//else assert(0); //quietly ignoring
		
		break;
	}

#undef CASE_

	out = proxy9->SetRenderState((D3DRENDERSTATETYPE)x,y);

	DDRAW_POP_HACK(DIRECT3DDEVICE7_SETRENDERSTATE,IDirect3DDevice7*,
	DX::D3DRENDERSTATETYPE&,DWORD&)(&out,this,x,y);

	DDRAW_RETURN(out)
}
HRESULT D3D9C::IDirect3DDevice7::GetRenderState(DX::D3DRENDERSTATETYPE x, LPDWORD y)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DDevice7::GetRenderState()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK) //DX9X SUBROUTINE
	
	DDRAW_LEVEL(7) << " Render State is " << x << "\n";

	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_GETRENDERSTATE,IDirect3DDevice7*,
	DX::D3DRENDERSTATETYPE&,LPDWORD&)(0,this,x,y);

	#define CASE_(x) break; case DX::D3DRENDERSTATE_##x:

	out = D3D_OK; if(y==DDRAW::getDWORD)
	{
		if(x==DX::D3DRENDERSTATE_COLORKEYENABLE) goto colorkey; //not in d3d9
	}
	else switch(x)					 
	{
	default: assert(0); //break;
		
		DDRAW_PANIC(3) << " PANIC! Unhandled render state type (" << x << ") was passed\n";\

		finish: DDRAW_FINISH(out); //2021

	CASE_(ANTIALIAS) //note: seen (setting to 0)

		//DDRAW_ALERT(0) << " ALERT! ANTIALIAS state set under DirectX9\n";

		goto finish; //todo: enable multisampling
		
	CASE_(TEXTUREPERSPECTIVE) return D3D_OK; //should be unnecessary
	CASE_(ZENABLE)
    CASE_(FILLMODE)
    CASE_(SHADEMODE)
    CASE_(ZWRITEENABLE)
    CASE_(ALPHATESTENABLE)
    CASE_(LASTPIXEL)
    CASE_(SRCBLEND)
    CASE_(DESTBLEND)
    CASE_(CULLMODE)
    CASE_(ZFUNC)
    CASE_(ALPHAREF)
    CASE_(ALPHAFUNC)
    CASE_(DITHERENABLE)
    CASE_(ALPHABLENDENABLE)

		//REMOVE ME
		//2021 NOTE //???
		//REMINDER: dx_d3d9c_alphablendenable was commented out in SetRenderState
		//I don't know if it was a typo or what?!
		if(y) *y = dx_d3d9c_alphablendenable?1:0; //works differenty under d3d9

		out = y?D3D_OK:!D3D_OK; goto finish; //???

    CASE_(FOGENABLE)	    

		if(y) *y = DDRAW::inFog;
	
		out = y?D3D_OK:!D3D_OK; goto finish;

    CASE_(SPECULARENABLE)
	CASE_(ZVISIBLE) out = !D3D_OK; goto finish;
	CASE_(STIPPLEDALPHA)	
		
		//DDRAW_ALERT(0) << " ALERT! STIPPLEDALPHA state queried under DirectX9\n";

		goto finish;
	
    CASE_(FOGCOLOR)
    CASE_(FOGTABLEMODE)
    CASE_(FOGSTART)
    CASE_(FOGEND)
    CASE_(FOGDENSITY)
	CASE_(EDGEANTIALIAS) //note: seen (setting to 0)

		//DDRAW_ALERT(0) << " ALERT! EDGEANTIALIAS state queried under DirectX9\n";		

		goto finish; //todo: enable multisampling

	CASE_(COLORKEYENABLE) colorkey: 

		if(y) *y = dx_d3d9c_colorkeyenable?1:0; //not part of d3d9

		out = y?D3D_OK:!D3D_OK; goto finish;

	CASE_(ZBIAS) x = (DX::D3DRENDERSTATETYPE)D3DRS_DEPTHBIAS;

    CASE_(RANGEFOGENABLE)
    CASE_(STENCILENABLE)
    CASE_(STENCILFAIL)
    CASE_(STENCILZFAIL)
    CASE_(STENCILPASS)
    CASE_(STENCILFUNC)
    CASE_(STENCILREF)
    CASE_(STENCILMASK)
    CASE_(STENCILWRITEMASK)
    CASE_(TEXTUREFACTOR)
    CASE_(WRAP0)
    CASE_(WRAP1)
    CASE_(WRAP2)
    CASE_(WRAP3)
    CASE_(WRAP4)
    CASE_(WRAP5)
    CASE_(WRAP6)
    CASE_(WRAP7)
    CASE_(CLIPPING) if(y) *y = 0; return D3D_OK;
    CASE_(LIGHTING)
		
		//Hmm: SOM or something is requesting the lighting state?!

		//causes trouble??? leaving alone for now
		//if(y) *y = DDRAW::isLit; return y?D3D_OK:!D3D_OK; 

	CASE_(EXTENTS) //note: seen	(setting to 0)

		//DDRAW_ALERT(0) << " ALERT! EXTENTS state queried under DirectX9\n";

		goto finish; //todo: not sure

    CASE_(AMBIENT)
    CASE_(FOGVERTEXMODE)
    CASE_(COLORVERTEX)
    CASE_(LOCALVIEWER)
    CASE_(NORMALIZENORMALS)
	CASE_(COLORKEYBLENDENABLE)

		goto finish; //todo: DDCOLORKEY interfaces

    CASE_(DIFFUSEMATERIALSOURCE)
    CASE_(SPECULARMATERIALSOURCE)
    CASE_(AMBIENTMATERIALSOURCE)
    CASE_(EMISSIVEMATERIALSOURCE)
    CASE_(VERTEXBLEND)
    CASE_(CLIPPLANEENABLE)

		if(y) *y = 0; goto finish;
								
	case D3DRS_COLORWRITEENABLE:
	case D3DRS_COLORWRITEENABLE1:
	case D3DRS_COLORWRITEENABLE2:		
	case D3DRS_COLORWRITEENABLE3: break;
	}

#undef CASE_
	
	out = proxy9->GetRenderState((D3DRENDERSTATETYPE)x,y);

	DDRAW_POP_HACK(DIRECT3DDEVICE7_GETRENDERSTATE,IDirect3DDevice7*,
	DX::D3DRENDERSTATETYPE&,LPDWORD&)(&out,this,x,y);
	
	DDRAW_RETURN(out)
}
HRESULT D3D9C::IDirect3DDevice7::drawprims(DX::D3DPRIMITIVETYPE x, DWORD y, LPVOID z, DWORD w, LPWORD zi, DWORD wi)
{
	//y is fvf, z is verts, w is num verts, zi is indices, wi is num indices

	auto *p = DDRAW::Direct3DDevice7->proxy9;
	
	if(dx_d3d9c_vshaders[DDRAW::vs]&&D3DFVF_XYZRHW==(y&D3DFVF_POSITION_MASK)) 
	{
		y&=~D3DFVF_XYZRHW; y|=D3DFVF_XYZW;
	}
			
	//WARNING! overwriting input buffer
	//WARNING! overwriting input buffer
	//WARNING! overwriting input buffer
	//WARNING! overwriting input buffer
	if(z) switch(y&D3DFVF_POSITION_MASK)
	{
		//TODO: SWITCH TO STATIC BUFFER
		//TODO: SWITCH TO STATIC BUFFER
		//TODO: SWITCH TO STATIC BUFFER
		//TODO: SWITCH TO STATIC BUFFER		
		//for SOM this isn't a problem, but this just needs some work sometime
		assert(w<=4);

	case D3DFVF_XYZRHW: case D3DFVF_XYZW:

		UINT sizeofFVF = dx_d3d9c_sizeofFVF(y);

		if(!DDRAW::doNothing)
		if(DDRAW::doScaling&&DDRAW::xyScaling[0]!=1.0f) for(size_t i=0;i<w;i++)
		{
			*(float*)(((BYTE*)z)+sizeofFVF*i)*=DDRAW::xyScaling[0]; 	
		}
		if(!DDRAW::doNothing)
		if(DDRAW::doScaling&&DDRAW::xyScaling[1]!=1.0f) for(size_t i=0;i<w;i++)
		{
			*(float*)(((BYTE*)z)+sizeofFVF*i+sizeof(float))*=DDRAW::xyScaling[1]; 	
		}

		if(D3DFVF_XYZRHW==(y&D3DFVF_POSITION_MASK))
		{
			if(!DDRAW::doNothing)
			if(DDRAW::doMapping&&DDRAW::xyMapping[0]!=0.0f) for(size_t i=0;i<w;i++)
			{
				*(float*)(((BYTE*)z)+sizeofFVF*i)+=DDRAW::xyMapping[0]; 	
			}			
			if(!DDRAW::doNothing)
			if(DDRAW::doMapping&&DDRAW::xyMapping[1]!=0.0f) for(size_t i=0;i<w;i++)
			{
				*(float*)(((BYTE*)z)+sizeofFVF*i+sizeof(float))+=DDRAW::xyMapping[1]; 	
			}
		}
		else //D3DFVF_XYZW: convert to clipping space
		{
			D3DVIEWPORT9 vp; 			
			if(!p->GetViewport(&vp)) //divide by zero
			for(size_t i=0;i<w;i++) //clip space
			{
				float *xy = (float*)(((BYTE*)z)+sizeofFVF*i);

				int two = 2;
				DDRAW::doSuperSamplingMul(two);

				xy[0] = xy[0]/vp.Width*two-1;
				xy[1] = 1-xy[1]/vp.Height*two;
			}
			else assert(0);
		}
	}

	DWORD op = 0, arg1, arg2; //alphaops

	IDirect3DVertexShader9 *vs = dx_d3d9c_vshaders[DDRAW::vs];
	IDirect3DPixelShader9  *ps = dx_d3d9c_pshaders_noclip[DDRAW::ps];

	if(!vs)
	if(dx_d3d9c_colorkeyenable //hack...
	&&DDRAW::TextureSurface7[0]&&DDRAW::TextureSurface7[0]->query9->knockouts) //colorkey
	{	
		if(!ps) ps = dx_d3d9c_colorkeyshader;

		p->GetTextureStageState(0,D3DTSS_ALPHAARG1,&arg1);
		p->GetTextureStageState(0,D3DTSS_ALPHAARG2,&arg2);

		if(p->GetTextureStageState(0,D3DTSS_ALPHAOP,&op)==D3D_OK&&op!=D3DTOP_DISABLE) 
		{	
			p->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_MODULATE);
			p->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
			p->SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_CURRENT);
		}
		else
		{
			p->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);
			p->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
		}

		p->SetRenderState(D3DRS_ALPHAREF,0); 
		p->SetRenderState(D3DRS_ALPHAFUNC,D3DCMP_GREATER); 		
		p->SetRenderState(D3DRS_ALPHATESTENABLE,dx_d3d9c_imitate_colorkey);
	}
	else p->SetRenderState(D3DRS_ALPHATESTENABLE,0);	  
		
	if(!vs&&!ps) ps = dx_d3d9c_defaultshader;
				  
	if(vs) dx_d3d9c_vstransformandlight(); dx_d3d9c_level(ps);

	p->SetVertexShader(vs); p->SetPixelShader(ps); 
	
	//body is moved out so dx_d3d9c_backblt can make use of it
	HRESULT out = D3D9C::IDirect3DDevice7::drawprims2(x,y,z,w,zi,wi);

	if(!vs)
	if(dx_d3d9c_colorkeyenable&&  //debugging	
	DDRAW::TextureSurface7[0]&&DDRAW::TextureSurface7[0]->query9->knockouts) //colorkey
	{
		if(op)
		{
			p->SetTextureStageState(0,D3DTSS_ALPHAOP,op);
			p->SetTextureStageState(0,D3DTSS_ALPHAARG1,arg1);
			p->SetTextureStageState(0,D3DTSS_ALPHAARG2,arg2);
		}		

		p->SetRenderState(D3DRS_ALPHATESTENABLE,0); 
	}

	return out;
}
HRESULT D3D9C::IDirect3DDevice7::drawprims2(DX::D3DPRIMITIVETYPE x, DWORD y, LPVOID z, DWORD vN, LPWORD zi, DWORD wi)
{	
	//2021: I THINK NEARLY ALL DRAWING CALLS NOW REDIRECT TO THIS SUBROUTINE

	//y is fvf, z is verts, w is num verts, zi is indices, wi is num indices

	enum{ v=0,v0=0,i=0 }; //these aren't used

	//TODO? these probably shouldn't be "this" methods since they call global
	//functions that use DDRAW::IDirect3DDevice7
	//auto *p = this->proxy9; assert(this==DDRAW::IDirect3DDevice7);
	auto *p = DDRAW::Direct3DDevice7->proxy9;

	if(!zi) wi = vN; //non-indexed mode?

	int prims = 0; switch(x)
	{
	case D3DPT_TRIANGLELIST: prims = wi/3; break;
	case D3DPT_TRIANGLESTRIP: prims = wi-2; break;
	case D3DPT_TRIANGLEFAN: prims = wi-2; break;
	case D3DPT_LINELIST: prims = wi/2; break; //2018
	default: assert(0); return !D3D_OK; //lines/points??
	}

	HRESULT out = D3D_OK;

	UINT sizeofFVF; if(z) sizeofFVF = dx_d3d9c_sizeofFVF(y);
	
	if(dx_d3d9c_immediate_mode)
	{	
		assert(z); //2021

		out = p->SetFVF(y);
		if(!out)
		out = zi?p->DrawIndexedPrimitiveUP
		((D3DPRIMITIVETYPE)x,0,vN,prims,zi,D3DFMT_INDEX16,z,sizeofFVF)
		:p->DrawPrimitiveUP((D3DPRIMITIVETYPE)x,prims,z,sizeofFVF);
	}
	else 
	{
		bool stereo = false; if(DDRAW::inStereo)
		{
			if(IDirect3DVertexDeclaration9*vd=dx_d3d9c_stereoVD(y))
			{
				stereo = true; //2021

				p->SetVertexDeclaration(vd);
			}
			else //goto fvf;
			{
				goto fvf; //breakpoint
			}
		}
		else fvf: out = p->SetFVF(y);
					
		IDirect3DIndexBuffer9 *ib = 0;
		IDirect3DVertexBuffer9 *vb = 0;
		if(zi&&!out) out = dx_d3d9c_ibuffer(p,ib,wi,zi);		
		if(z&&!out) out = dx_d3d9c_vbuffer(p,vb,vN*sizeofFVF,z);

		if(out) return out;

		p->SetIndices(ib); //assert(ib);
		if(vb) p->SetStreamSource(0,vb,0,sizeofFVF);
		//z/zi are already built into ib/vb above
		//out = p->DrawIndexedPrimitive((D3DPRIMITIVETYPE)x,z,0,vN,zi,prims);
		//out = p->DrawIndexedPrimitive((D3DPRIMITIVETYPE)x,0,0,vN,0,prims);			
		//out = dx_d3d9c_drawprims2(stereo,(D3DPRIMITIVETYPE)x,vN,prims);
		{
			if(stereo&&dx_d3d9c_stereo_scissor) //TESTING
			{
				//seems to spin with the view some???
				//maybe it requires a transformation:
				//https://stackoverflow.com/questions/3034291/clipplanes-vertex-shaders-and-hardware-vertex-processing-in-direct3d-9
				//float cp[4] = {-1,0,0,0};
				//p->SetRenderState(D3DRS_CLIPPLANEENABLE,1);
				D3DVIEWPORT9 vp; p->GetViewport(&vp);
				RECT sr = {0,0,vp.Width/2,vp.Height};
				p->SetScissorRect(&sr);
				p->SetRenderState(D3DRS_SCISSORTESTENABLE,1);

			//	p->SetStreamSourceFreq(0,D3DSTREAMSOURCE_INDEXEDDATA|1); //2
				out = p->SetStreamSource(1,DDRAW::Direct3DDevice7->query9->stereo,8,8);
				//p->SetClipPlane(0,cp);
				if(ib) out = p->DrawIndexedPrimitive((D3DPRIMITIVETYPE)x,v,v0,vN,i,prims);
				if(!ib) out = p->DrawPrimitive((D3DPRIMITIVETYPE)x,v,prims);
				p->SetStreamSource(1,DDRAW::Direct3DDevice7->query9->stereo,0,8);
				//cp[0] = -cp[0];
				//p->SetClipPlane(0,cp);
				sr.left = sr.right; sr.right = vp.Width;
				p->SetScissorRect(&sr);
				if(ib) out = p->DrawIndexedPrimitive((D3DPRIMITIVETYPE)x,v,v0,vN,i,prims);
				if(!ib) out = p->DrawPrimitive((D3DPRIMITIVETYPE)x,v,prims);
			//	p->SetStreamSourceFreq(0,D3DSTREAMSOURCE_INDEXEDDATA|2); //2

				//p->SetRenderState(D3DRS_CLIPPLANEENABLE,0);
				p->SetRenderState(D3DRS_SCISSORTESTENABLE,0);
			}
			else if(ib)
			{
				out = p->DrawIndexedPrimitive((D3DPRIMITIVETYPE)x,v,v0,vN,i,prims); 
			}
			else out = p->DrawPrimitive((D3DPRIMITIVETYPE)x,v,prims); 
		}
	}
	
	return out;
}
HRESULT D3D9C::IDirect3DDevice7::PreLoad(DX::LPDIRECTDRAWSURFACE7 x)
{
	DDRAW_LEVEL(7) << "DDRAW::IDirect3DDevice7::PreLoad()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK);

	auto p = DDRAW::is_proxy<IDirectDrawSurface7>(x);
	
	if(p) p->proxy9->PreLoad(); else assert(p); return DD_OK;	
}	
HRESULT D3D9C::IDirect3DDevice7::DrawPrimitive(DX::D3DPRIMITIVETYPE x, DWORD y, LPVOID z, DWORD w, DWORD q)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DDevice7::DrawPrimitive()\n";

	DDRAW_IF_NOT_DX9C_RETURN(!D3D_OK)

	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_DRAWPRIMITIVE,IDirect3DDevice7*,
	DX::D3DPRIMITIVETYPE&,DWORD&,LPVOID&,DWORD&,DWORD&)(0,this,x,y,z,w,q);
							 
	if(!w||!z) DDRAW_FINISH(D3D_OK)

	assert(q==0); //what is q?

	DDRAW_LEVEL(7) << " Primitive Type is " << x << " (" << w << " vertices)\n";

	if(DDRAWLOG::debug>=7) //???
	{
		DDRAW::log_FVF(y);

		#define OUT(X) if(x==D3DPT_##X) DDRAW_LEVEL(7) << ' ' << #X << '\n';						   
		OUT(TRIANGLELIST)
		OUT(TRIANGLESTRIP)
		OUT(TRIANGLEFAN)
		#undef OUT
	}

	out = D3D9C::IDirect3DDevice7::drawprims(x,y,z,w);

	DDRAW_POP_HACK(DIRECT3DDEVICE7_DRAWPRIMITIVE,IDirect3DDevice7*,
	DX::D3DPRIMITIVETYPE&,DWORD&,LPVOID&,DWORD&,DWORD&)(&out,this,x,y,z,w,q);	

	DDRAW_RETURN(out)
}
HRESULT D3D9C::IDirect3DDevice7::DrawIndexedPrimitive(DX::D3DPRIMITIVETYPE x, DWORD y, LPVOID z, DWORD w, LPWORD q, DWORD r, DWORD s)
{
	//assuming: y is fvf, z is verts, w is vertex count, q is indices, r is index count, s flags (but not sure)

	DDRAW_LEVEL(7) << "D3D9C::IDirect3DDevice7::DrawIndexedPrimitive()\n";

	DDRAW_IF_NOT_DX9C_RETURN(!D3D_OK)

	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_DRAWINDEXEDPRIMITIVE,IDirect3DDevice7*,
	DX::D3DPRIMITIVETYPE&,DWORD&,LPVOID&,DWORD&,LPWORD&,DWORD&,DWORD&)(0,this,x,y,z,w,q,r,s);	

	//2022: see DrawIndexedPrimitiveVB comment
	//on crashes
	if(!r||!w) DDRAW_FINISH(D3D_OK) //short-circuit this way

	out = D3D9C::IDirect3DDevice7::drawprims(x,y,z,w,q,r);

	DDRAW_POP_HACK(DIRECT3DDEVICE7_DRAWINDEXEDPRIMITIVE,IDirect3DDevice7*,
	DX::D3DPRIMITIVETYPE&,DWORD&,LPVOID&,DWORD&,LPWORD&,DWORD&,DWORD&)(&out,this,x,y,z,w,q,r,s);	

	DDRAW_RETURN(out);
}
HRESULT D3D9C::IDirect3DDevice7::SetClipStatus(DX::LPD3DCLIPSTATUS x)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DDevice7::SetClipStatus()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK)

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
	assert(2==x->dwFlags&&!x->dwStatus);
	assert(!x->maxx&&2048==x->minx);
	assert(!x->maxy&&2048==x->miny);

	//DDRAW_RETURN(!D3D_OK); //not supporting
	DDRAW_RETURN(D3D_OK); //not supporting
} 
HRESULT D3D9C::IDirect3DDevice7::DrawIndexedPrimitiveVB(DX::D3DPRIMITIVETYPE x, DX::LPDIRECT3DVERTEXBUFFER7 y, DWORD z, DWORD w, LPWORD q, DWORD r, DWORD s)
{
	//Note: z is vertex offset, w is vertex count, q is index pointer, r is index count, s is wait flags

	DDRAW_LEVEL(7) << "D3D9C::IDirect3DDevice7::DrawIndexedPrimitiveVB()\n";

	DDRAW_IF_NOT_DX9C_RETURN(!D3D_OK) if(!y) DDRAW_RETURN(!D3D_OK)

	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_DRAWINDEXEDPRIMITIVEVB,IDirect3DDevice7*,
	DX::D3DPRIMITIVETYPE&,DX::LPDIRECT3DVERTEXBUFFER7&,DWORD&,DWORD&,LPWORD&,DWORD&,DWORD&)(0,this,x,y,z,w,q,r,s);
	
	//2022: I think crashes started happening when I removed these
	//checks... dx_d3d9c_ibuffer can't handle 0 r, but even with a
	//fix it still crashes somewhere :(
	#if 0
	if(!w/*||!q||!r*/) DDRAW_FINISH(D3D_OK) //short-circuit this way
	#else
	if(!w||!r) DDRAW_FINISH(D3D_OK) //short-circuit this way
	#endif

	if(DDRAWLOG::debug>=7)
	{
#define OUT(X) if(x==D3DPT_##X) DDRAW_LEVEL(7) << ' ' << #X << '\n';
						   
		OUT(TRIANGLELIST)
		OUT(TRIANGLESTRIP)
		OUT(TRIANGLEFAN)

#undef OUT
	}

	DDRAW_LEVEL(7) << "vertex offset/count is " << z << '/' << w << "\n";
	DDRAW_LEVEL(7) << "index address/count is " << (unsigned)q << '/' << r << "\n";
		
	auto p = DDRAW::is_proxy<IDirect3DVertexBuffer7>(y);
	if(!p) DDRAW_FINISH(!D3D_OK)		   

	if(!dx_d3d9c_immediate_mode)
	{	
		UINT stride = p->query9->sizeofFVF;

		//TODO? It's probably better to just use
		//dx_d3d9c_vbuffer here, except unlike the
		//non-VB APIs indices can be offset into the
		//vbuffer		
		if(p->query9->upbuffer)
		{
			//UINT floor = z*stride;
			UINT limit = (z+w)*stride; 			
			if(0) //2021: this always works, doesn't reuse though
			{
				//NOTE: another downside I can see here is even
				//though dx_d3d9c_vbuffer rotates a few buffers
				//it still starts at 0 everytime, whereas the
				//staggered implementation below might have the
				//benefit of not colliding. although it depends
				//on the app

				//TEST MY FPS SOMETIME
				out = drawprims(x,p->query9->FVF,(char*)p->query9->upbuffer+z*stride,w,q,r);
				DDRAW_FINISH(out)
			}
			else if(p->query9->mixed_mode_lock<limit) //???
			{
				//2021: let me explain, in Direct3D 7 the vertex buffers seem
				//able to be freely locked and unlocked and there is no partial
				//locking. I don't know if it flushes the entire buffer on unlock
				//I hope not because SOM does lots of little locks/unlocks instead
				//of batching, so the rationale here is to flush the buffer as a
				//reponse to a drawing call. I don't think D3D7 vbuffers were VRAM
				//buffers but were DMA buffers. I think they were implemented mainly
				//in software since they came on the scene long before VRAM buffers
				//SOM wouldn't be able to function if they worked differently since
				//it draw calls would just be sending garbage

				assert(limit<=p->query9->mixed_mode_size);
				  
				//2021: this used to be z*stride but this seems better to me
				UINT floor = p->query9->mixed_mode_lock;

				//I think this is envisioning iterating through the buffer making
				//individual drawing calls
				/*2021: I think restarting doesn't make sense since D3D7 can only
				//fill all or nothing? I.e. it will just upload everything again!
				p->query9->mixed_mode_lock = z?limit:0;*/
				p->query9->mixed_mode_lock = limit;
		
				void *b;
				out = p->proxy9->Lock(floor,limit-floor,&b,D3DLOCK_DISCARD);
				if(!out) memcpy(b,(char*)p->query9->upbuffer+floor,limit-floor);
				if(!out) out = p->proxy9->Unlock();
				if(out) DDRAW_FINISH(out)
			}
			else assert(p->query9->mixed_mode_size>=limit); 
		}
		
		//NOTE: before 2021 this hadn't used drawprims... since it is
		//now, SetStreamSource is the only way to offset to z without
		//passing z to drawprims (SOM doesn't use nonzero z)
		proxy9->SetStreamSource(0,p->proxy9,z*stride,stride);	
		out = drawprims(x,p->query9->FVF,0,w,q,r);
		proxy9->SetStreamSource(0,0,0,0); //NICE
	}
	else out = drawprims(x,p->query9->FVF,p->query9->upbuffer,w,q,r);

	DDRAW_POP_HACK(DIRECT3DDEVICE7_DRAWINDEXEDPRIMITIVEVB,IDirect3DDevice7*,
	DX::D3DPRIMITIVETYPE&,DX::LPDIRECT3DVERTEXBUFFER7&,DWORD&,DWORD&,LPWORD&,DWORD&,DWORD&)(&out,this,x,y,z,w,q,r,s);

	DDRAW_RETURN(out)
}
HRESULT D3D9C::IDirect3DDevice7::GetTexture(DWORD x, DX::LPDIRECTDRAWSURFACE7 *y)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DDevice7::GetTexture()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK)
	
	HRESULT out = D3D_OK;

	DDRAW::IDirectDrawSurface7 *p = 0;

	auto t0 = DDRAW::TextureSurface7[0]; //2021

	if(DDRAW::gl) //2021: I'm not sure D3D shouldn't work like this too
	{
		p = t0; *y = 0; //NOTE: OpenGL never calls SetTexture
	}
	else out = proxy9->GetTexture(x,(IDirect3DBaseTexture9**)y);
	
	if(out==D3D_OK&&*y)
	{
		if(t0&&t0->query9->update9==(IDirect3DBaseTexture9*)*y) 
		{
			p = t0; //2021: conservatively leveraging t0 (NEW)
		}
		else if(!t0&&(IDirect3DTexture9*)*y!=query9->white) 
		{
			//*y = 0; return D3D_OK; //2021: memory leak?
		}
		else p = DDRAW::IDirectDrawSurface7::get_head->get(*y);

		if(!(*y)->Release()) assert(0); 
	}
	if(p) DDRAW::referee(p,+1);

	*y = (DX::IDirectDrawSurface7*)p; DDRAW_RETURN(out); 
}
HRESULT D3D9C::IDirect3DDevice7::SetTexture(DWORD x, DX::LPDIRECTDRAWSURFACE7 y)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DDevice7::SetTexture()\n";

	DDRAW_IF_NOT_DX9C_RETURN(!D3D_OK) 
		
	if(x>=DDRAW::textures){	assert(!y); return D3D_OK; }

//	DDRAW_LEVEL(7) << " Texture is " << x << " (surface is " << y << ")\n";		 	

	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_SETTEXTURE,IDirect3DDevice7*,
	DWORD&,DX::LPDIRECTDRAWSURFACE7&)(0,this,x,y);
	
	auto p = DDRAW::is_proxy<IDirectDrawSurface7>(y);
	if(!p)
	{
		DDRAW::TextureSurface7[0] = 0;

		/*discontinuing this in favor of TEXTURE0_NOCLIP
		if(DDRAW::psColorkey) dx_d3d9X_psconstant(DDRAW::psColorkey,0.0f,'w');
		*/

		out = proxy9->SetTexture(x,query9->white);

		DDRAW_FINISH(y?!D3D_OK:out)
	}
	
	if(p->query9->palette)
	{
		proxy9->SetCurrentTexturePalette(p->query9->palette);

		DDRAW_LEVEL(7) << " Palette was set (" << p->query9->palette << ")\n";
	}
	DDRAW_LEVEL(7) << " Format is " << p->queryX->format << "\n";

	D3D9C::updating_texture(p,false);

	if(D3DPOOL_SYSTEMMEM==dx_d3d9c_texture_pool)
	{
		out = proxy9->SetTexture(x,p->query9->update9);
	}
	else out = proxy9->SetTexture(x,p->query9->group9);
			
	DDRAW::TextureSurface7[0] = out==D3D_OK?p:0;

	//I'M DISABLING THIS FOR D3D9 FOR NOW BECAUSE IT'S HARDER TO SET IT UP
	//TODO? dx_d3d9c_colorkeyenable MIGHT BE FACTORED INTO THIS CALCULATION
	//dx_d3d9c_pshaders_noclip = !p->queryX->knockouts?dx_d3d9c_pshaders2:dx_d3d9c_pshaders;

	/*discontinuing this in favor of TEXTURE0_NOCLIP
	float ck = out==D3D_OK?(p->query9->knockouts&&dx_d3d9c_colorkeyenable?1.0f:0.0f):0.0f; //colorkey
	if(DDRAW::psColorkey) dx_d3d9X_psconstant(DDRAW::psColorkey,ck,'w');
	*/

	if(DDRAW::psTextureState) //2022
	{
		float ts[4] = {p->queryX->width,p->queryX->height};
		ts[2] = 1/ts[0]; ts[3] = 1/ts[1];
		dx_d3d9X_psconstant(DDRAW::psTextureState,ts);
	}

	DDRAW_POP_HACK(DIRECT3DDEVICE7_SETTEXTURE,IDirect3DDevice7*,
	DWORD&,DX::LPDIRECTDRAWSURFACE7&)(&out,this,x,y);

	DDRAW_RETURN(out) 
}
HRESULT D3D9C::IDirect3DDevice7::GetTextureStageState(DWORD x, DX::D3DTEXTURESTAGESTATETYPE y, LPDWORD z)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DDevice7::GetTextureStageState()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK) //DX9X SUBROUTINE

	DDRAW_LEVEL(7) << " Stage is " << x << " (" << y << ")\n";

	#define CASE_(y) break; case DX::D3DTSS_##y: out = D3DSAMP_##y;

	HRESULT out = D3D_OK; //2021
		
	//Reminder: observe DDRAW::getDWORD if/when setting up HACK wrapper	

	//if(z!=DDRAW::getDWORD) 
	switch(y) //these have been migrated to D3DSAMPLERSTATETYPE
	{
	default: break;
	CASE_(ADDRESSU)		
    CASE_(ADDRESSV)
    CASE_(BORDERCOLOR) assert(0);	
    CASE_(MAGFILTER)
    CASE_(MINFILTER)
    CASE_(MIPFILTER)
    CASE_(MIPMAPLODBIAS)
    CASE_(MAXMIPLEVEL)
    CASE_(MAXANISOTROPY) break;
	}

	#undef CASE_
	#define CASE_(y) break; case DX::D3DTSS_##y:	
	
	if(!out) if(z!=DDRAW::getDWORD) switch(y)
	{
	CASE_(ADDRESS) assert(0); //2021
	default: 
		
		DDRAW_LEVEL(3) << " PANIC! Unhandled texture stage type (" << y << ") was passed\n";

		DDRAW_RETURN(!D3D_OK)
		
	CASE_(COLOROP)
    CASE_(COLORARG1)
    CASE_(COLORARG2)
    CASE_(ALPHAOP)
    CASE_(ALPHAARG1)
    CASE_(ALPHAARG2)
    CASE_(BUMPENVMAT00)
    CASE_(BUMPENVMAT01)
    CASE_(BUMPENVMAT10)
    CASE_(BUMPENVMAT11)
    CASE_(TEXCOORDINDEX)
	CASE_(BUMPENVLSCALE)
	CASE_(BUMPENVLOFFSET) break;
	}

	#undef CASE_

	if(out) //2021
	{
		out = proxy9->GetSamplerState(x,(D3DSAMPLERSTATETYPE)out,z); 
	}
	else out = proxy9->GetTextureStageState(x,(D3DTEXTURESTAGESTATETYPE)y,z);

	DDRAW_RETURN(out)
}
HRESULT D3D9C::IDirect3DDevice7::SetTextureStageState(DWORD x, DX::D3DTEXTURESTAGESTATETYPE y, DWORD z)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DDevice7::SetTextureStageState()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK) //DX9X SUBROUTINE
		
	if(x>=DDRAW::textures){	assert(0); return D3D_OK; }

	DDRAW_LEVEL(7) << " Stage is " << x << " (" << y << " set to " << z << ")\n";

	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_SETTEXTURESTAGESTATE,IDirect3DDevice7*,
	DWORD&,DX::D3DTEXTURESTAGESTATETYPE&,DWORD&)(0,this,x,y,z);

	assert(x==0); //assuming

	if(0&&DX::debug)
	{
		switch(y)
		{
		case DX::D3DTSS_MINFILTER:
		case DX::D3DTSS_MAGFILTER: z = D3DTEXF_POINT; break; //D3DTEXF_LINEAR
		case DX::D3DTSS_MIPFILTER: z = D3DTEXF_LINEAR; break; //D3DTEXF_NONE
		}
	}
	else
	{
		if(y==DX::D3DTSS_MIPFILTER) //2018
		{
			z--; assert(z>=0&&z<=2); //d3d8/7 differs from d3d9	
		}
		if(DDRAW::doMipmaps)
		if(y==DX::D3DTSS_MINFILTER) //hack 
		{	
			DWORD n = DDRAW::anisotropy;
		
			n = n?min(n,dx_d3d9c_anisotropy_max):dx_d3d9c_anisotropy_max;

			if(n>1) //NEW: 2018
			{
				z = D3DTEXF_ANISOTROPIC;

				proxy9->SetSamplerState(x,D3DSAMP_MAXANISOTROPY,n);
			}
		}
		//forcing linear filter (for now)
		if(y==DX::D3DTSS_MAGFILTER||y==DX::D3DTSS_MIPFILTER)
		{
			if(DDRAW::linear) z = D3DTEXF_LINEAR;
		}
	}
		 	
	#define CASE_(Y) break; case DX::D3DTSS_##Y: out = D3DSAMP_##Y; 

	out = D3D_OK; switch(y) //these have been migrated to D3DSAMPLERSTATETYPE
	{
	default: break;
	CASE_(ADDRESSU)		
    CASE_(ADDRESSV) break;	
	case DX::D3DTSS_ADDRESS:

		out = D3DSAMP_ADDRESSU;
		proxy9->SetSamplerState(x,D3DSAMP_ADDRESSV,z);
		break;

	case DX::D3DTSS_MIPMAPLODBIAS: assert(0);
	
    CASE_(BORDERCOLOR)	
    CASE_(MAGFILTER)
    CASE_(MINFILTER)
    CASE_(MIPFILTER)	
	CASE_(MAXMIPLEVEL)
    CASE_(MAXANISOTROPY)
	}

	#undef CASE_
	#define CASE_(y) break; case DX::D3DTSS_##y:

	if(!out) switch(y)
	{
	default: 
		
		DDRAW_LEVEL(3) << " PANIC! Unhandled texture stage type (" << y << ") was passed\n";
		
		assert(0); DDRAW_FINISH(!D3D_OK)
		
	CASE_(COLOROP)
	{
		const float val[2][3] = {{0,0,0},{1,1,1}};

		switch(z) //HACK: FAIRLY SWORD-OF-MOONLIGHT CENTRIC STUFF
		{
		case D3DTOP_DISABLE:
			
			assert(0); break;
			
		case D3DTOP_SELECTARG1: //select texture (Sword of Moonlight?)
		
			dx_d3d9X_vsconstant(DDRAW::vsColorFactors,val[1],'xyz');			
			dx_d3d9X_psconstant(DDRAW::psColorFactors,val[0],'xyz');
			break;
		
		case D3DTOP_ADD: case D3DTOP_MODULATE:
		
			dx_d3d9X_vsconstant(DDRAW::vsColorFactors,val[0],'xyz');
			dx_d3d9X_psconstant(DDRAW::psColorFunction,val[z==D3DTOP_ADD],'xyz');
			break;
		
		case D3DTOP_SELECTARG2: //select color (Sword of Moonlight?)
		
			dx_d3d9X_vsconstant(DDRAW::vsColorFactors,val[0],'xyz');
			dx_d3d9X_psconstant(DDRAW::psColorFactors,val[z==D3DTOP_SELECTARG2],'xyz');
			break;
				
		default: assert(0);
		}
	}
    CASE_(COLORARG1) assert(z==D3DTA_TEXTURE);
    CASE_(COLORARG2) assert(z==D3DTA_DIFFUSE||z==D3DTA_CURRENT);
    CASE_(ALPHAOP)
	{	
		switch(z) //HACK: FAIRLY SWORD-OF-MOONLIGHT CENTRIC STUFF
		{
		case D3DTOP_DISABLE: //docs say this is undefined (but SOM does it a lot) 
			
			//Note: may be equivalent to disabling the entire TextureStage framework

			//setting alpha to normal (Sword of Moonlight?)
			dx_d3d9X_vsconstant(DDRAW::vsColorFactors,0.0f,'w'); 
			dx_d3d9X_psconstant(DDRAW::psColorFactors,0.0f,'w'); 
			break;

		case D3DTOP_SELECTARG1: //select texture (Sword of Moonlight?)

			dx_d3d9X_vsconstant(DDRAW::vsColorFactors,1.0f,'w'); 
			dx_d3d9X_psconstant(DDRAW::psColorFactors,0.0f,'w'); 			
			break;
	
		case D3DTOP_ADD: case D3DTOP_MODULATE:
		
			dx_d3d9X_vsconstant(DDRAW::vsColorFactors,0.0f,'w');
			dx_d3d9X_psconstant(DDRAW::psColorFunction,z==D3DTOP_ADD?1ul:0ul,'w'); 		
			break;
		
		case D3DTOP_SELECTARG2: //select color (Sword of Moonlight?)

			dx_d3d9X_vsconstant(DDRAW::vsColorFactors,0.0f,'w');
			dx_d3d9X_psconstant(DDRAW::psColorFactors,z==D3DTOP_SELECTARG2?1ul:0ul,'w');
			break;

		default: assert(0);
		}
	}
    CASE_(ALPHAARG1) assert(z==D3DTA_TEXTURE);
    CASE_(ALPHAARG2) assert(z==D3DTA_DIFFUSE||z==D3DTA_CURRENT);

    CASE_(BUMPENVMAT00)
    CASE_(BUMPENVMAT01)
    CASE_(BUMPENVMAT10)
    CASE_(BUMPENVMAT11)
    CASE_(TEXCOORDINDEX)
	CASE_(BUMPENVLSCALE)
	CASE_(BUMPENVLOFFSET) break;
	}

	#undef CASE_
	
	if(out)
	{
		out = proxy9->SetSamplerState(x,(D3DSAMPLERSTATETYPE)out,z);
	}
	else out = proxy9->SetTextureStageState(x,(D3DTEXTURESTAGESTATETYPE)y,z);

	DDRAW_POP_HACK(DIRECT3DDEVICE7_SETTEXTURESTAGESTATE,IDirect3DDevice7*,
	DWORD&,DX::D3DTEXTURESTAGESTATETYPE&,DWORD&)(&out,this,x,y,z);

	DDRAW_RETURN(out)
}
HRESULT D3D9C::IDirect3DDevice7::ApplyStateBlock(DWORD x)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DDevice7::ApplyStateBlock()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK) if(!x) return !D3D_OK;
		
	void **p = (void**)x;
	
	HRESULT out = ((IDirect3DStateBlock9*)p[0])->Apply();

	if(out==D3D_OK)
	{
		//DWORD flags = (DWORD)p[1];

		if(DDRAW::gl) //dx.d3d9X.cpp?
		{
			auto cmp = queryGL->state;
			queryGL->state = (DWORD&)p[1];	
			queryGL->state.apply(cmp);
		}

		if(!DDRAW::doFog&&DDRAW::Direct3DDevice7)
		{
			DWORD fog = (DWORD)p[1]&1;

			//for shader models prior to 3.0
			//tricky: prepare this render state for DDRAW::ApplyStateBlock()
			DDRAW::Direct3DDevice7->proxy9->SetRenderState(D3DRS_FOGENABLE,fog);
		}

		DDRAW::ApplyStateBlock();
	}

	DDRAW_RETURN(out);
}
HRESULT D3D9C::IDirect3DDevice7::CaptureStateBlock(DWORD x)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DDevice7::CaptureStateBlock()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK) if(!x) return !D3D_OK;
		
	void **p = (void**)x;
	
	HRESULT out = ((IDirect3DStateBlock9*)p[0])->Capture();
	
	if(out==D3D_OK)
	{
		DWORD flags; if(DDRAW::gl) //dx.d3d9X.cpp?
		{
			queryGL->state._fog = DDRAW::inFog;

			flags = queryGL->state.dword;
		}
		else flags = DDRAW::inFog;

		(DWORD&)p[1] = flags;
	}

	DDRAW_RETURN(out)
}
HRESULT D3D9C::IDirect3DDevice7::DeleteStateBlock(DWORD x)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DDevice7::DeleteStateBlock()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK) if(!x) return !D3D_OK;
	
	void **p = (void**)x;
	while(((IDirect3DStateBlock9*)p[0])->Release());
	delete[] p; return D3D_OK;
}
HRESULT D3D9C::IDirect3DDevice7::CreateStateBlock(DX::D3DSTATEBLOCKTYPE x, LPDWORD y)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DDevice7::CreateStateBlock()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK) if(!y) return !D3D_OK;
	
	HRESULT out = proxy9->CreateStateBlock((D3DSTATEBLOCKTYPE)x,(IDirect3DStateBlock9**)y);

	if(out==D3D_OK)
	{
		void **p = new void*[2];

		p[0] = *(IDirect3DStateBlock9**)y;

		DWORD flags; if(DDRAW::gl) //dx.d3d9X.cpp?
		{
			queryGL->state._fog = DDRAW::inFog;

			flags = queryGL->state.dword;
		}
		else flags = DDRAW::inFog;

		(DWORD&)p[1] = flags;

		*(void**)y = p;
	}

	DDRAW_RETURN(out)
}
HRESULT D3D9C::IDirect3DDevice7::LightEnable(DWORD x, BOOL y)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DDevice7::LightEnable()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK)

	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_LIGHTENABLE,IDirect3DDevice7*,
	DWORD&,BOOL&)(0,this,x,y);
	if((long)x<0) DDRAW_FINISH(D3D_OK)

	DDRAW_LEVEL(7) << ' ' << x << ' ' << (y?"on":"off") << '\n';

	out = proxy9->LightEnable(x,y);

	if(out==D3D_OK)
	{
		DWORD *lost = 0;

		if(DDRAW::LightEnable(x,y,&lost)) dx_d3d9c_vslight(false);

		if(lost)
		{
			proxy9->LightEnable(*lost,0); //assuming desired

			if(DDRAW::lightlost) DDRAW::lightlost(*lost,0);
		}
	}

	DDRAW_POP_HACK(DIRECT3DDEVICE7_LIGHTENABLE,IDirect3DDevice7*,
	DWORD&,BOOL&)(&out,this,x,y);

	DDRAW_RETURN(out)
}






HRESULT D3D9C::IDirect3DVertexBuffer7::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DVertexBuffer7::QueryInterface()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK)

	LPOLESTR p; if(StringFromIID(riid,&p)==S_OK) DDRAW_LEVEL(7) << ' ' << p << '\n';

	DDRAW_RETURN(!S_OK)
		
	return proxy9->QueryInterface(riid,ppvObj);
}
ULONG D3D9C::IDirect3DVertexBuffer7::AddRef()
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DVertexBuffer7::AddRef()\n";

	DDRAW_ADDREF_RETURN(0)
}
ULONG D3D9C::IDirect3DVertexBuffer7::Release()
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DVertexBuffer7::Release()\n";

	DDRAW_IF_NOT_TARGET_AND_RELEASE(out,0)

	if(out==0)
	{
		if(queryX->vboGL) queryX->_vboGL_delete();

		delete this;
	}
	
	return out;
}
HRESULT D3D9C::IDirect3DVertexBuffer7::Lock(DWORD x, LPVOID *y, LPDWORD z)
{
	//x: DDLOCK_DISCARDCONTENTS|DDLOCK_NOSYSLOCK|DDLOCK_WRITEONLY

	DDRAW_LEVEL(7) << "D3D9C::IDirect3DVertexBuffer7::Lock()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK)
		
	DDRAW_PUSH_HACK(DIRECT3DVERTEXBUFFER7_LOCK,IDirect3DVertexBuffer7*,
	DWORD&,LPVOID*&,LPDWORD&)(0,this,x,y,z);
	
	DWORD sz; if(x&0x80000000L) //2021: partial_Lock?
	{
		sz = *z*query9->sizeofFVF;			
		assert(sz<=query9->mixed_mode_size);
	}
	else sz = query9->mixed_mode_size;

	if(query9->upbuffer) mixed_mode:
	{
		//2021: this was done below? I don't undestand
		//how DrawPrimitiveVB expects to use this???
		//
		// I think the idea is the program will make
		// lots of little locks/unlocks so that this
		// would start over at the begging. whenever
		// DrawIndexedPrimitiveVB draws at vertex #0
		// it rolls this back to 0. but I think that
		// if the buffer is just a DMA receiving bay
		// it probably makes sense to always send it
		// over and never place anything into proxy9
		//
		query9->mixed_mode_lock = 0; //2021

		if(!y) DDRAW_FINISH(!D3D_OK)

		*y = query9->upbuffer; if(z) *z = sz;
		
		DDRAW_FINISH(D3D_OK);
	}	

	DWORD flags = 0; 
	//THIS IS dx_d3d9c_immediate_mode's BOTTLENECK
	//D3DLOCK_DISCARD; //not ok for SOM map pieces
	//(still DDLOCK_DISCARDCONTENTS is sent to it)
	//2021: I think this is bad for SOM since D3D7
	//can't partial lock and it locks on each draw
	//although I'm about to implement a batch path
	//(I think I see a positive performance impact
	//although not a lot)
	if(1&& //2021: TESTING
	proxy9 //2021: OpenGL?
	&&x&DDLOCK_DISCARDCONTENTS)
	{
		//doesn't work with map pieces, but it still
		//has same meaning as DDLOCK_DISCARDCONTENTS
		flags|=D3DLOCK_DISCARD; 
		//flags|=D3DLOCK_NOOVERWRITE;
	}
	else if(1) //som_scene_4137F0_Lock
	{
		query9->mixed_mode_lock = 0; //???

		if(!query9->upbuffer) 
		{
			assert(!dx_d3d9c_immediate_mode);
			query9->upbuffer = new char[query9->mixed_mode_size];
			goto mixed_mode;
		}
	}

//	if((x&DDLOCK_WAIT)==0) flags|=D3DLOCK_DONOTWAIT;

	assert(proxy9); //2021: dx.d3d9X.cpp may not allocate a proxy

	out = proxy9->Lock(0,sz,y,flags); 

	if(!out&&z) *z = sz; 
	
	DDRAW_POP_HACK(DIRECT3DVERTEXBUFFER7_LOCK,IDirect3DVertexBuffer7*,
	DWORD&,LPVOID*&,LPDWORD&)(&out,this,x,y,z);

	DDRAW_RETURN(out)
}
HRESULT D3D9C::IDirect3DVertexBuffer7::Unlock()
{
	DDRAW_LEVEL(7) << "D3D9C::IDirect3DVertexBuffer7::Unlock()\n";

	DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK)

	if(query9->upbuffer) return D3D_OK;

	assert(proxy9); //2021: dx.d3d9X.cpp may not allocate a proxy

	DDRAW_RETURN(proxy9->Unlock())
}




////////////////////////////////////////////////////////
//              PROCEDURAL INTERFACE                  //
////////////////////////////////////////////////////////

extern int dx_d3d9X_supported(HMONITOR);
extern void dx_d3d9X_register();
static void dx_d3d9c_register()
{
	DDRAW::ff = true;
	DDRAW::gl = false;

	assert(dx95==DDRAW::target&&dx95=='dx9c');

	#define DDRAW_TABLES(Interface) \
	D3D9C::Interface().register_tables('dx9c');

	DDRAW_TABLES(IDirectDraw) 
	DDRAW_TABLES(IDirectDraw7)
	DDRAW_TABLES(IDirectDrawClipper) 
	DDRAW_TABLES(IDirectDrawPalette) 
	DDRAW_TABLES(IDirectDrawSurface) 
	DDRAW_TABLES(IDirectDrawSurface7) 
	DDRAW_TABLES(IDirectDrawGammaControl) 
	DDRAW_TABLES(IDirect3D7) 
	DDRAW_TABLES(IDirect3DDevice7)
	DDRAW_TABLES(IDirect3DVertexBuffer7)

#undef DDRAW_TABLES

	if(!dx_d3d9c_df) dx_d3d9c_prepshaders();
}

namespace D3D9X
{
	int is_needed_to_initialize();
	bool updating_texture(DDRAW::IDirectDrawSurface7*,int);
}
extern int D3D9C::is_needed_to_initialize()
{
	static int one_off = 0; if(one_off++) return one_off; //??? 

	DDRAW::compat_moving_target = DDRAW::target;

	char infoBuf[MAX_PATH]; 
	GetSystemDirectoryA(infoBuf,MAX_PATH);
	strcat_s(infoBuf,MAX_PATH,D3D9C::isDebugging?"\\d3d9d.dll":"\\d3d9.dll"); 	
	HMODULE dll = LoadLibraryA(infoBuf);      
	if(!dll) DDRAW_ERROR(0) << "d3d9.dll failed to load\n";
	if(!dll){ assert(0); return 1; }
	
	//fps is etremely slow, screen is extremely bright :(
	//https://github.com/microsoft/D3D9On12/issues/2
	//it was working but after some changes now not:
	//https://github.com/microsoft/D3D9On12/issues/4
	if(0||DDRAW::doD3D9on12)
	D3D9C::DLL::Direct3DCreate9On12Ex = (D3D9C::Create9On12Ex)GetProcAddress(dll,"Direct3DCreate9On12Ex");
	D3D9C::DLL::Direct3DCreate9Ex = (D3D9C::Create9Ex)GetProcAddress(dll,"Direct3DCreate9Ex");
	D3D9C::DLL::Direct3DCreate9 = (D3D9C::Create9)GetProcAddress(dll,"Direct3DCreate9");	

	//2021: I'm working without a net here if 
	//the shaders don't work or can't be used
	if(!DDRAW::shader)
//	if(1&&DX::debug||DDRAW::target!='dx9c')
	{
		dx95 = DDRAW::target;
		return D3D9X::is_needed_to_initialize();
	}
	dx95 = DDRAW::target = 'dx9c';

	dx_d3d9c_register(); //DirectDrawCreateEx 

	DDRAW_HELLO(0) << "Direct3D9 initialized\n";

	return 1; //for static thunks //???
}

void D3D9C::IDirectDrawSurface7::dirtying_texture() //2021
{
	//NOTE: |=2 is reserved for SetColorKey
	query9->update_pending|=1;
	//this could be too much in some cases
	//update could be deferred until drawing
	//if this is a problem
	//if(this==DDRAW::TextureSurface7[0])
	//D3D9C::updating_texture(this);	
	assert(this!=DDRAW::TextureSurface7[0]);
}
bool DDRAW::IDirectDrawSurface7::updating_texture(int force)
{
	assert(target==DDRAW::target);

	//som_status_automap calls this blindly
	if(DDRAW::compat!='dx9c') return false;

	//NOTE: this pattern should be okay even if downgraded
	//via DDRAW::compat_moving_target... there isn't a good
	//system for detecting that ATM
	//if(DDRAW::shader)
	//return D3D9C::updating_texture(this,force);
	return D3D9X::updating_texture(this,force);
}
extern bool D3D9C::updating_texture(DDRAW::IDirectDrawSurface7 *p, int force)
{
	auto q = p->query9;
	auto g = q->group9;

	//YUCK: this is in case "update_all_textures" is used
	//since it just blindly iterates through all surfaces
	if(!q->isTextureLike)
	{
		assert(!q->update_pending); return false;
	}

	if(!force&&!q->update_pending) return false;

	int noupdate = force&4; //testing

	if(1!=DX::central_processing_units) //RAII
	{
		//TODO: this can be implemented without
		//CRITICAL_SECTION with lock primitives

		DX_CRITICAL_SECTION //RAII

		if(force|=q->update_pending)
		{	
			q->update_pending = 8;

			DX_CRITICAL_SECTION //RAII

			//just prevent reentry on same texture
			//trying to eliminate CRITICAL_SECTION
			if(8&force) while(8&q->update_pending) 
			{
				//DX::sleep(); //spin?
			}		
		}
		else return false;
	}
	else force|=q->update_pending;

	if(force&2)
	{
		//TODO: I'd like the dx_d3d9c_imitate_colorkey 
		//logic in SetColorKey to be moved to here so
		//it's delayed. update_pending will have to
		//have be expanded to a bit mask

		if(dx_d3d9c_imitate_colorkey)
		{
			if(DDRAW::doMipmaps)
			dx_d3d9c_colorkey(p);
		}
	}
	if(force&1)
	{
		//2021: I think it's best to defer 
		//dx_d3d9c_mipmap until uploading to
		//avoid generating more than once		
		if(g&&DDRAW::doMipmaps)
		if(dx_d3d9c_autogen_mipmaps)
		{
			g->GenerateMipSubLevels();
		}
		else dx_d3d9c_mipmap(p); //g
	}
	
	//q->update_pending = noupdate;

	//2022: noupdate is a signal to do the mipmaps
	//on another thread
	//NOTE: needs to return false to dx.d3d9X.cpp
	if(noupdate) 
	{
		q->update_pending = noupdate;

		return false;
	}

	auto d3dd9 = DDRAW::Direct3DDevice7->proxy9;

	if(!DDRAW::shader) //dx.d3d9c.cpp?
	{
		assert(D3DPOOL_SYSTEMMEM==dx_d3d9c_texture_pool);

		//HACK: just implement D3DPOOL_SYSTEMMEM logic
		if(p->target!='dx9c') goto finish;
	}
	else switch(dx_d3d9c_texture_pool)
	{
	case D3DPOOL_MANAGED: if(g) g->PreLoad(); goto finish;
	case D3DPOOL_DEFAULT:
	if(g&&!DDRAW::Direct3DDevice7->query9->isSoftware) goto finish;
	}
	 
	//2021: I think it's demonstratively possible to show
	//that dimensions and format of the surface are fixed
	//D3DSURFACE_DESC desc;  
	//if(p->proxy9->GetDesc(&desc)==D3D_OK)
	if(!q->update9) //2021
	{
		//int lvs = g?g->GetLevelCount():1; 
		int lvs = max(1,q->mipmaps);

		D3DFORMAT format = q->format;
		switch(q->format)
		{
		case D3DFMT_X8R8G8B8: format = D3DFMT_A8R8G8B8; break;
		case D3DFMT_X1R5G5B5: format = D3DFMT_A1R5G5B5; break;
		}

		/*2021: SetSurfaceDesc is SYSTEMMEM only
		HRESULT create = !D3D_OK; 
		if(q->update9) //SetSurfaceDesc???
		{
			if(lvs==q->update9->GetLevelCount())
			{
				D3DSURFACE_DESC cmp;
				create = q->update9->GetLevelDesc(0,&cmp);
				if(create==D3D_OK)						
				if(cmp.Format!=format
				||cmp.Height!=q->height				
				||cmp.Width!=q->width) create = !D3D_OK;
			}

			if(create!=D3D_OK)
			{
				q->update9->Release(); q->update9 = 0;
			}
		}
		if(create!=D3D_OK)*/
		d3dd9->CreateTexture(q->width,q->height,lvs,0,format,D3DPOOL_DEFAULT,&q->update9,0);
	}
	
	if(q->update9) if(g)
	{
		IDirect3DTexture9 *a = dx_d3d9c_alpha(g);		
		if(d3dd9->UpdateTexture(a,q->update9)) assert(0);
	}
	else
	{
		IDirect3DSurface9 *pp = 0;
		if(!q->update9->GetSurfaceLevel(0,&pp))
		{
			IDirect3DSurface9 *a = dx_d3d9c_alpha(p->proxy9);
			if(d3dd9->UpdateSurface(a,0,pp,0)) assert(0);
			pp->Release();
		}
		else assert(0);
	}
	else assert(0); //2021

	//EneEdit.exe is triggering this when a TXR
	//skin having PRF is opened after a non TXR
	//having skin PRF is opened (refs is 4)
//	DDRAW_ASSERT_REFS(q->group9,==2)

	if(p==DDRAW::TextureSurface7[0])
	{
		d3dd9->SetTexture(0,q->update9); assert(q->update9);
	}

	finish: q->update_pending = 0; return true;
}

void DDRAW::refresh_state_dependent_shader_constants()
{
	if(DDRAW::compat!='dx9c'||!DDRAW::Direct3DDevice7) return;

	//this should be called if ever
	//the interfaces are left in an 
	//ambiguous state. Eg. after an
	//external ApplyStateBlock call

	//NOTE: this flushes the brightness level adjustment
	//I guess there's nothing else to do after ApplyStateBlock 
	dx_d3d9c_level(0,true);
}

extern bool DDRAW::toggleEffectsBuffer(int on)
{
	bool out = dx_d3d9c_effectsshader_toggle;
	dx_d3d9c_effectsshader_toggle = on!=-1?on:!dx_d3d9c_effectsshader_toggle;
	return out;
}

typedef void ID3D12CommandQueue,ID3D12Fence;

//https://www.magnumdb.com/search?q=IDirect3DDevice9On12
MIDL_INTERFACE("189819f1-1db6-4b57-be54-1821339b85f7") ID3D12Device; //d3d12.h
MIDL_INTERFACE("e7fda234-b589-4049-940d-8878977531c8") IDirect3DDevice9On12 //d3d9.h?
:
public IUnknown
{
public:

    STDMETHOD(GetD3D12Device)(REFIID riid, void **ppvDevice)PURE;

	STDMETHOD(UnwrapUnderlyingResource)
	(IDirect3DResource9*,ID3D12CommandQueue* pCommandQueue, REFIID riid, void **ppvResource12)PURE;

	STDMETHOD(ReturnUnderlyingResource)
	(IDirect3DResource9*, UINT NumSync, UINT64 *pSignalValues, ID3D12Fence **ppFences)PURE;
};
extern bool dx_d3d9X_feature_level_11(LUID);
HRESULT WINAPI D3D9C::DirectDrawCreateEx(GUID *lpGuid, LPVOID *lplpDD, REFIID iid, IUnknown FAR *pUnkouter)
{	
	HRESULT out = !DD_OK; 

	assert(DDRAW::compat=='dx9c');

	static bool autogen = dx_d3d9c_autogen_mipmaps; //hack
	dx_d3d9c_autogen_mipmaps = autogen;
		
	if(iid==IID_IDirectDraw
	 ||iid==IID_IDirectDraw7) //{15E65EC0-3B9C-11D2-B92F-00609797EA5B}
	{
		DDRAW_LEVEL(7) << " (IDirectDraw7)\n";

		IUnknown *d = 0; 

		//2021: found out about this today on Wikipedia's WDDM page
		//https://devblogs.microsoft.com/directx/coming-to-directx-12-d3d9on12-and-d3d11on12-resource-interop-apis/
		if(0&&D3D9C::DLL::Direct3DCreate9On12Ex)
		{
			#ifdef NDEBUG
		//	#error need to figure out if performance is acceptable to be used by default
			extern int SOMEX_vnumber;
			assert(SOMEX_vnumber<=0x1020305UL);
			#endif

			//this shit is extremely cryptic (no examples online)
			//https://github.com/microsoft/DirectX-Specs/blob/master/d3d/TranslationLayerResourceInterop.md
			//this supports that this strategy should work
			//https://github.com/microsoft/D3D12TranslationLayer/issues/8
			D3D9C::D3D9ON12_ARGS I_guess = {1};
			out = D3D9C::DLL::Direct3DCreate9On12Ex(D3D_SDK_VERSION,&I_guess,1,(IDirect3D9Ex**)&d);

			//it still returns okay even though my Intel chipset doesn't support D3D12
			//maybe D3D9ON12_ARGS isn't set up correctly
			/*according the second link this needs to be done on the device instead
			if(d) 
			{				
				IDirect3DDevice9On12 *dd = 0;
				//don't have a device to do this test
				//if(out=d->QueryInterface(__uuidof(IDirect3DDevice9On12),(void**)&dd))
				if(out=dev->QueryInterface(__uuidof(IDirect3DDevice9On12),(void**)&dd))
				{
					d->Release(); d = 0;					
				}				
				else //TESTING
				{
					IUnknown *test = 0;
					if(!dd->GetD3D12Device(__uuidof(ID3D12Device),(void**)&test))
					{
						test->Release();
					}
					dd->Release();
				}
			}*/
			//NOTE: if !out I guess may as well keep this IDirect3D9Ex interface
			if(out!=D3D_OK||!GetModuleHandleA("d3d9on12.dll"))
			{
				assert(!GetModuleHandleA("d3d12.dll"));

				D3D9C::DLL::Direct3DCreate9On12Ex = 0;
			}
		}
		if(out!=D3D_OK) //sans D3D9on12
		if(D3D9C::DLL::Direct3DCreate9Ex)
		{
			out = D3D9C::DLL::Direct3DCreate9Ex(D3D_SDK_VERSION,(IDirect3D9Ex**)&d);

			if(out!=D3D_OK) D3D9C::DLL::Direct3DCreate9Ex = 0;
		}								
		if(out!=D3D_OK) //sans Ex
		if(D3D9C::DLL::Direct3DCreate9)
		{
			d = D3D9C::DLL::Direct3DCreate9(D3D_SDK_VERSION);
			if(d) out = D3D_OK;
		}

		if(out!=D3D_OK)
		{
			DDRAW_LEVEL(7) << "DirectDrawCreateEx FAILED\n"; 
			
			return !DD_OK;
		}
		else out = DD_OK;

		IDirect3D9Ex *d3d9Ex = (IDirect3D9Ex*)d;

		UINT adapter = 0; 
		
		if(lpGuid)
		{
			GUID tmp = *lpGuid;	tmp.Data3 = '_x'; 
			
			if(tmp==dx_d3d9c_device_x)
			{
				adapter = lpGuid->Data3-'0';
			}
		}

		if(adapter>d3d9Ex->GetAdapterCount()) 
		{
			d3d9Ex->Release(); return !DD_OK;
		}

		DDRAW::monitor = d3d9Ex->GetAdapterMonitor(adapter);
		/*2021
		MONITORINFO mon = { sizeof(MONITORINFO) };		
		if(GetMonitorInfo(DDRAW::monitor,&mon)) //appropriate??
		{
			//if(mon.dwFlags&MONITORINFOF_PRIMARY) DDRAW::monitor = 0;
		}*/

		if(dx_d3d9c_sw())
		{
			FARPROC D3D9GetSWInfo = GetProcAddress(dx_d3d9c_sw(),"D3D9GetSWInfo");

			HRESULT ok = d3d9Ex->RegisterSoftwareDevice(D3D9GetSWInfo);

			if(ok!=D3D_OK)
			{
				ok = ok; //breakpoint
			}
		}

		D3DCAPS9 caps9; HRESULT ok =
		d3d9Ex->GetDeviceCaps(adapter,D3DDEVTYPE_HAL,&caps9);

	retarget: //do OpenGL tests again?

		if(DDRAW::target=='dxGL')		
		if(DDRAW::doNvidiaOpenGL) //AMD? (ignoring Intel too)
		{
			//Nvidia: 0x10DE
			//AMD: 0x1002, 0x1022
			//Intel: 0x163C, 0x8086, 0x8087
			D3DADAPTER_IDENTIFIER9 id;
			if(d3d9Ex->GetAdapterIdentifier(adapter,0,&id)
			||id.VendorId!=0x10DE) 
			{
				DDRAW::target = 'dx9c';

				DDRAW_ALERT(0) << "Video adapter isn't Nvidia so OpenGL won't be used by default\n";
			}
		}
		if(DDRAW::target=='dxGL'&&DDRAW::doFlipEx) //WGL_NV_DX_interop2?
		{
			LUID luid;
			d3d9Ex->GetAdapterLUID(adapter,&luid);
			if(!dx_d3d9X_feature_level_11(luid))
			{
				DDRAW::target = 'dx9c';

				DDRAW_ALERT(0) << "Video adapter doesn't have OpenGL+D3D11 (WGL_NV_DX_interop2)\n";
			}
		}
			dx95 = DDRAW::target; //moving target?

			//always calling this is just easier
			dx_d3d9X_register();

		assert(!DDRAW::DirectDraw7);

		//2021: falling back on Shader Model 2 or fixed function
		//if adapater can't support designated target
		int sm = D3DSHADER_VERSION_MAJOR(caps9.VertexShaderVersion);
		if(sm<3&&dx95!='dx9c'||sm<2&&!DDRAW::shader)
		{
			DDRAW::target = dx95 = 'dx9c'; assert(0);

			DDRAW_ALERT(0) << "Video adapter is Shader Model "<< sm << '\n'
			<< " It's necessary to downgrade to D3D9 with or without Shader Model 2\n";

			dx_d3d9c_register();
		}
		else if(sm>=3&&dx95!=DDRAW::compat_moving_target&&dx95!='dx9c') //gl_miss?
		{
			DDRAW::target = dx95 = DDRAW::compat_moving_target;

			DDRAW_ALERT(0) << "Video adapter is Shader Model "<< sm << '\n'
			<< " Now restoring to specified D3D9 or OpenGL/ANGLE with Shader Model 3\n";

			if(dx95=='dxGL') goto retarget; //do OpenGL tests again?
		}
		static DWORD support = 0; //OPTIMIZING?
		if(DDRAW::target!='dx9c')		
		if(~support&(1<<adapter))
		if(!dx_d3d9X_supported(DDRAW::monitor)) //UNIMPLEMENTED
		{
			DDRAW::target = dx95 = 'dx9c'; assert(0);

			DDRAW_ALERT(0) << "Video adapter doesn't meet minimum OpenGL version (4.5 or 3.1 ES)\n"
			<< " It's necessary to fallback to D3D9 with Shader Model 3 just to not quit out now\n";

			dx_d3d9X_register();
		}		
		else support|=1<<adapter;
		//2021: anticipating being unable to support sRGB modes
		if(sm<2||DDRAW::shader) DDRAW::sRGB = 0;

		//HACK! Ex.movie.cpp (dx_d3d9X_d3d12?)
		DDRAW::target_backbuffer = DDRAW::target;

		DDRAW::IDirectDraw7 *p = new DDRAW::IDirectDraw7(dx95);

		p->query9->adapter = adapter; p->proxy9 = d3d9Ex;	

		if(ok==D3D_OK)
		{
			p->query9->vsmodel = sm;
			p->query9->vsprofile = D3DSHADER_VERSION_MINOR(caps9.VertexShaderVersion);
			p->query9->psmodel = D3DSHADER_VERSION_MAJOR(caps9.PixelShaderVersion);
			p->query9->psprofile = D3DSHADER_VERSION_MINOR(caps9.PixelShaderVersion);
		}
		
		//Thoughts: could wait until a flip seems likely to occur
		if(DDRAW::doWait)
		{	
			//could use IDirect3DDevice9Ex::WaitForVBlank here, but 
			//this feature is more for Windows XP systems
			p->query9->vblank = DDRAW::vblank(DDRAW::monitor); 
			assert(p->query9->vblank);
		}
				
		*lplpDD = DDRAW::DirectDraw7 = p; p->AddRef();
	}
	else assert(0); 

	return out;
}
HRESULT WINAPI D3D9C::DirectDrawEnumerateExA(DX::LPDDENUMCALLBACKEXA lpCallback, LPVOID lpContext, DWORD dwFlags)
{
	HRESULT out = !DD_OK; ::IDirect3D9Ex *temp = 0;

	if(D3D9C::DLL::Direct3DCreate9Ex)
	{
		out = D3D9C::DLL::Direct3DCreate9Ex(D3D_SDK_VERSION,(IDirect3D9Ex**)&temp);

		if(out!=D3D_OK) D3D9C::DLL::Direct3DCreate9Ex = 0;
	}

	if(out!=D3D_OK) //sans Ex
	if(D3D9C::DLL::Direct3DCreate9)
	{
		temp = (IDirect3D9Ex*)D3D9C::DLL::Direct3DCreate9(D3D_SDK_VERSION);

		if(temp) out = DD_OK;
	}

	if(out!=D3D_OK) return !DD_OK;

	UINT devs = temp->GetAdapterCount(); D3DADAPTER_IDENTIFIER9 id; 

	devs = min(devs,dx_d3d9c_display_adaptersN);

	if(D3D9C::doEmulate)
	if(!lpCallback(0,"Primary Display Driver","",lpContext,0)) //"display"
	{
		temp->Release(); return DD_OK;
	}

	//static char devnames[dx_d3d9c_display_adaptersN][32]; //???
	char devname[16] = "Adapter ";

	for(UINT i=0;i<devs;i++)
	{
		temp->GetAdapterIdentifier(i,0,&id);

		//MONITORINFO mon = { sizeof(MONITORINFO) }; //UNUSED? //2021

		HMONITOR hm = temp->GetAdapterMonitor(i);
		
		/*UNUSED? //2021
		if(GetMonitorInfo(DDRAW::monitor,&mon)) //appropriate??
		{
			//if(mon.dwFlags&MONITORINFOF_PRIMARY) hm = 0;
		}*/

		dx_d3d9c_display_adapters[i] = dx_d3d9c_device_x;

		dx_d3d9c_display_adapters[i].Data3 = '0'+i;

		//sprintf_s(devnames[i],32,"Adapter %d.",i+1); //id.DeviceName
		sprintf_s(devname+8,7,"%d.",i+1);

		//if(!lpCallback(dx_d3d9c_display_adapters+i,id.Description,devnames[i],lpContext,hm)) 		
		if(!lpCallback(dx_d3d9c_display_adapters+i,id.Description,devname,lpContext,hm))
		break;		
	}

	temp->Release(); return DD_OK;
}