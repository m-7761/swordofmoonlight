
#include "Ex.h" 
EX_TRANSLATION_UNIT

/*TODO:
This file is scheduled to be removed
by way of moving the bulk of it into
som.game.cpp divided w/ som.tool.cpp
*/
	
//REMOVE ME?
#include <d3dx9math.h>
#include <d3dx9tex.h>

#define DDRAW_OVERLOADS
#include "dx.ddraw.h"
#include "dx.dsound.h"
#include "dx.dinput.h"

#include "Ex.ini.h"
#include "Ex.langs.h" 
#include "Ex.input.h" 
#include "Ex.output.h" 
#include "Ex.movie.h" 
#include "Ex.window.h" 
#include "Ex.cursor.h"
//#include "Ex.dataset.h" //som.keys.h?

#include "som.932.h"
#include "som.932w.h"
#include "som.title.h"
//#include "som.keys.h" //2021
#include "som.shader.h"
#include "som.state.h"
#include "som.game.h"
#include "som.tool.h"
#include "som.status.h"
#include "som.files.h" 
#include "som.extra.h" 

static int som_hacks_ = 0; //margin

//static bool som_hacks_keygen = true;

//REMOVE ME?
extern HDC SOM::HACKS::stretchblt = 0; 
extern int som_hacks_shader_model = 0;

//reminder: picture menus are tinted
extern unsigned som_hacks_tint = 0; 
static unsigned som_hacks_blit = 0;	
static unsigned som_hacks_huds = 0;
static unsigned som_hacks_fade = 0;
static unsigned som_hacks_dark = 0;
extern unsigned som_hacks_fill = 0; //OpenXR

static DX::D3DMATRIX som_hacks_view;

static int som_hacks_lamps = 0;
static DWORD *som_hacks_lamps_pool = 0; 
static float som_hacks_lamps_switch[5] = {0,0,0,0,0}; 
static void som_hacks_ditch_lamps(){ som_hacks_lamps_switch[3] = 0.0f; }
																		
extern bool som_hacks_fab = false; //false alpha blend
static char som_hacks_magfilter = 0; //linearize menus

static D3DCOLOR som_hacks_ambient = 0;
static D3DCOLOR som_hacks_fogcolor = 0;
static D3DCOLOR som_hacks_foglevel = 0xAD000000; //leveled fogcolor
static D3DCOLOR som_hacks_fogalpha = 0xAF000000; //alphafog fogcolor

//float som_hacks_sky[3] = {-66}; //OBSOLETE
extern void som_hacks_skycam(float x);

static bool som_hacks_zenable = false;
static bool som_hacks_fogenable = false;
static bool som_hacks_alphablendenable = false;

static DWORD som_hacks_srcblend = D3DBLEND_ONE; //debugging
static DWORD som_hacks_destblend = D3DBLEND_ZERO; //debugging
static DWORD som_hacks_alphaop = 0; //REMOVE ME

static DWORD som_hacks_texture_addressu = 1; //D3DTADDRESS_WRAP
static DWORD som_hacks_texture_addressv = 1; //D3DTADDRESS_WRAP

//2022: som_hacks_SetTexture also set som_hacks_texture! 
//static DX::IDirectDrawSurface7 *som_hacks_texture = 0;
//static const SOM::KEY::Image *som_hacks_key_image = 0;

//static void *som_hacks_shadow_image = (void*)2; //REMOVE ME?
//static void *som_hacks_correct_image = (void*)1; //REMOVE ME?

static DWORD som_hacks_colorop = 0; //debugging

namespace EX
{
	extern LRESULT CALLBACK WindowProc(HWND,UINT,WPARAM,LPARAM); //Ex.detours.h/cpp
}

#define SOM_HACKS_LIGHTS 128

static DX::D3DLIGHT7 *som_hacks_lightfix = 0, *som_hacks_lightlog = 0;

static bool *som_hacks_lightfix_on = 0; 

static void som_hacks_lightlost(DWORD i, bool on) //DDRAW::lightlost
{
	if(!som_hacks_lightfix_on||i>=SOM_HACKS_LIGHTS) return/* on*/;

	//HACK: Using som_hacks_lightfix_on to mark the map lights 0,1,2!!!
	if(i>2) /*return*/ som_hacks_lightfix_on[i] = on; 
}

static DX::D3DLIGHT7 *som_hacks_light(DWORD i, intptr_t set) //DDRAW::light
{
	if(!som_hacks_lightfix||i>=SOM_HACKS_LIGHTS) return 0;
	
	if(set=='log') return som_hacks_lightlog+i;

	return som_hacks_lightfix+i;
}

static bool som_hacks_lights_detected_map_close = true;

static void som_hacks_level(D3DCOLOR&);

//static void som_hacks_correct(DDRAW::IDirectDrawSurface7*,const SOM::KEY::Image*);

extern float som_hacks_pcstate[6] = {};
static bool som_hacks_pcstated = false;

//REMOVE ME?
static float som_hacks_dot(float u[3], float v[3])
{
	return u[0]*v[0]+u[1]*v[1]+u[2]*v[2];
}
static float som_hacks_radians(float u[3], float v[3], float n[3]) //signed angle
{	
	float d = som_hacks_dot(u,v), r = acosf(d); 
	
	if(_isnan(r)) return d>0?0.0:D3DX_PI;		
	
	return som_hacks_dot(n,v)>0?r:-r; 
}		  
static float som_hacks_length(float a[3])
{
	return sqrtf(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]);
}		  
static float som_hacks_distance(float a[3], float b[3])
{
	float x = a[0]-b[0], y = a[1]-b[1], z = a[2]-b[2];

	return sqrtf(x*x+y*y+z*z);
}
static float som_hacks_lerp(float x, float y, float t)
{
	return x+t*(y-x);
}
static float som_hacks_spin(float a, float b)
{
	return fabsf(b-a)<M_PI?b:b-M_PI*(b>a?2:-2);
}
static float som_hacks_0_to_2pi(float a)
{
	return fmodf(a+M_PI*4,M_PI*2); //4 is arbitrary
}
static float som_hacks_slerp(float &a, float &b, float s)
{
	a = som_hacks_0_to_2pi(a); b = som_hacks_0_to_2pi(b);

	return som_hacks_lerp(a,som_hacks_spin(a,b),s);
}
static float som_hacks_radians(float &a, float &b)
{
	a = som_hacks_0_to_2pi(a); b = som_hacks_0_to_2pi(b);

	return som_hacks_spin(a,b)-a;
}

//// TOOL HACKS ////

static HRESULT som_hacks_EnumDevices_cb3(DX::LPD3DENUMDEVICESCALLBACK,GUID*,LPSTR,LPSTR,DX::LPD3DDEVICEDESC,DX::LPD3DDEVICEDESC,LPVOID);
static void *som_hacks_CreateSurface4(HRESULT*, DDRAW::IDirectDraw4*, DX::LPDDSURFACEDESC2 &x, DX::LPDIRECTDRAWSURFACE4*&, IUnknown *&);
static void *som_hacks_GetDC(HRESULT*, DDRAW::IDirectDrawSurface4*, HDC*&);
static void *som_hacks_ReleaseDC(HRESULT*, DDRAW::IDirectDrawSurface4*, HDC&);
static void *som_hacks_SetViewport2(HRESULT*,DDRAW::IDirect3DViewport3*,DX::LPD3DVIEWPORT2&);
static void *som_hacks_Clear2(HRESULT*,DDRAW::IDirect3DViewport3*,DWORD&,DX::LPD3DRECT&,DWORD&,DX::D3DCOLOR&,DX::D3DVALUE&,DWORD&);


//// GAME HACKS ////

static void *som_hacks_QueryInterface(HRESULT*, DDRAW::IDirectDraw7*, REFIID, LPVOID*&j);
static void *som_hacks_QueryInterface(HRESULT*, DDRAW::IDirectDrawSurface7*, REFIID, LPVOID*&);

static void *som_hacks_SetCooperativeLevel(HRESULT*, DDRAW::IDirectDraw7*, HWND &x, DWORD &y);
extern void *som_hacks_SetDisplayMode(HRESULT*, DDRAW::IDirectDraw7*, DWORD&, DWORD&, DWORD&, DWORD&, DWORD&);

static void *som_hacks_Blt(HRESULT*, DDRAW::IDirectDrawSurface7*, LPRECT&, DX::LPDIRECTDRAWSURFACE7&, LPRECT&, DWORD&, DX::LPDDBLTFX&);
static void *som_hacks_Flip(HRESULT*, DDRAW::IDirectDrawSurface7*, DX::LPDIRECTDRAWSURFACE7&,DWORD&);

static void *som_hacks_SetColorKey(HRESULT*, DDRAW::IDirectDrawSurface7*, DWORD&, DX::LPDDCOLORKEY&);

static void *som_hacks_SetGammaRamp(HRESULT*, DDRAW::IDirectDrawGammaControl*, DWORD&,DX::LPDDGAMMARAMP&);
static void *som_hacks_CreateDevice(HRESULT*, DDRAW::IDirect3D7*, const GUID*&, DX::LPDIRECTDRAWSURFACE7&, DX::LPDIRECT3DDEVICE7*&);

static void *som_hacks_EndScene(HRESULT*,DDRAW::IDirect3DDevice7*);
static void *som_hacks_BeginScene(HRESULT*,DDRAW::IDirect3DDevice7*);

static void *som_hacks_Clear(HRESULT*,DDRAW::IDirect3DDevice7*,DWORD&,DX::LPD3DRECT&,DWORD&,DX::D3DCOLOR&,DX::D3DVALUE&,DWORD&);

static void *som_hacks_SetTransform(HRESULT*,DDRAW::IDirect3DDevice7*,DX::D3DTRANSFORMSTATETYPE&,DX::LPD3DMATRIX&);
static void *som_hacks_GetTransform(HRESULT*,DDRAW::IDirect3DDevice7*,DX::D3DTRANSFORMSTATETYPE&,DX::LPD3DMATRIX&);

static void *som_hacks_SetRenderState(HRESULT*,DDRAW::IDirect3DDevice7*,DX::D3DRENDERSTATETYPE&,DWORD&);
static void *som_hacks_SetRenderState3(HRESULT*,DDRAW::IDirect3DDevice3*,DX::D3DRENDERSTATETYPE&,DWORD&);

static void *som_hacks_DrawPrimitive(HRESULT*,DDRAW::IDirect3DDevice7*,DX::D3DPRIMITIVETYPE&,DWORD&,LPVOID&,DWORD&,DWORD&);
static void *som_hacks_DrawIndexedPrimitive3(HRESULT*,DDRAW::IDirect3DDevice3*,DX::D3DPRIMITIVETYPE&,DWORD&,LPVOID&,DWORD&,LPWORD&,DWORD&,DWORD&);
static void *som_hacks_DrawIndexedPrimitiveVB(HRESULT*,DDRAW::IDirect3DDevice7*,.../*DX::D3DPRIMITIVETYPE&,DDRAW::IDirect3DVertexBuffer7*&,DWORD&,DWORD&,LPWORD&,DWORD&,DWORD&*/);
static void *som_hacks_DrawIndexedPrimitive(HRESULT*,DDRAW::IDirect3DDevice7*,.../*DX::D3DPRIMITIVETYPE&,DWORD&,LPVOID&,DWORD&,LPWORD&,DWORD&,DWORD&*/);

static void *som_hacks_SetTexture(HRESULT*,DDRAW::IDirect3DDevice7*,DWORD&,DX::LPDIRECTDRAWSURFACE7&);
static void *som_hacks_SetTextureStageState(HRESULT*,DDRAW::IDirect3DDevice7*,DWORD&,DX::D3DTEXTURESTAGESTATETYPE&,DWORD&);

static void *som_hacks_LightEnable(HRESULT*,DDRAW::IDirect3DDevice7*,DWORD&,BOOL&);
static void *som_hacks_SetLight(HRESULT*,DDRAW::IDirect3DDevice7*,DWORD&,DX::LPD3DLIGHT7&);

static void *som_hacks_CreateVertexBuffer(HRESULT*,DDRAW::IDirect3D7*,DX::LPD3DVERTEXBUFFERDESC&,DX::LPDIRECT3DVERTEXBUFFER7*&,DWORD&);

//static void *som_hacks_SetVolume(HRESULT*,DSOUND::IDirectSoundBuffer*,LONG&);
static void *som_hacks_SetOrientation(HRESULT*,DSOUND::IDirectSound3DListener*,DX::D3DVALUE&,DX::D3DVALUE&,DX::D3DVALUE&,DX::D3DVALUE&,DX::D3DVALUE&,DX::D3DVALUE&,DWORD&);
static void *som_hacks_SetVelocity(HRESULT*,DSOUND::IDirectSound3DListener*,DX::D3DVALUE&,DX::D3DVALUE&,DX::D3DVALUE&,DWORD&);
static void *som_hacks_SetPosition(HRESULT*,DSOUND::IDirectSound3DListener*,DX::D3DVALUE&,DX::D3DVALUE&,DX::D3DVALUE&,DWORD&);

static void *som_hacks_EnumDevices_diA(HRESULT*,DINPUT::IDirectInputA*,DWORD&,DX::LPDIENUMDEVICESCALLBACKA&,LPVOID&,DWORD&);
static void *som_hacks_CreateDevice_diA(HRESULT*,DINPUT::IDirectInputA*,REFGUID,DX::LPDIRECTINPUTDEVICEA*&,LPUNKNOWN&);
static void *som_hacks_GetDeviceState(HRESULT*,DINPUT::IDirectInputDeviceA*,DWORD&,LPVOID&);

static void *som_hacks_CreateSurface7(HRESULT*,DDRAW::IDirectDraw7*,DX::LPDDSURFACEDESC2&,DX::LPDIRECTDRAWSURFACE7*&,IUnknown*&);

template<typename T> 
inline void *som_hacks_queryinterface_hack(void *(*f)(HRESULT*,T*,REFIID,LPVOID*&)){ return f; }

#define som_hacks_QueryInterface_f(Interface)\
som_hacks_queryinterface_hack<DDRAW::Interface>(som_hacks_QueryInterface)

extern bool Ex_mipmap_point_filter;

//REMOVE ME?
static DWORD WINAPI som_hacks_thread_main(void*);
extern void SOM::kickoff_somhacks_thread()
{
	//static int one_off = 0; if(one_off++) return; //??? 
		
	EX::INI::Window wn; EX::INI::Editor ed;				  
	EX::INI::Option op;	EX::INI::Detail dt;
	EX::INI::Adjust ad;	EX::INI::Bugfix bf;

	if(SOM::game) 
	{
		wchar_t icon[MAX_PATH] = L"";
		//if(!GetEnvironmentVariable(L"ICON",icon,MAX_PATH))
		if(1>=GetEnvironmentVariable(L"ICON",icon,MAX_PATH))
		{
			if(!SOM::retail) //2018: prefer EXE icon if retail?
			{
				wcscpy(icon,L"SomEx.ico"); //resides in tool folder
			}
		}

		//just to be safe (ExtractIconW)
		SetCurrentDirectoryW(SOM::Game.project());
	
		//2018, Nov. Windows is not finding the EXE's icon. I think
		//it is because the taskbar is using the launcher for its pin-
		//to-taskbar logic. I think it will eventualy get fixed. I can't
		//see anything in SOM::initialize_taskbar that groups the launcher
		//with the binary (could Ex_window_class be the cause?)
		icon2:
		{
			SOM::icon = ExtractIcon(0,icon,0);									 
		}
		if(!SOM::icon&&!*icon
		&&GetModuleFileNameEx(GetCurrentProcess(),0,icon,MAX_PATH))
		goto icon2;

		if(*wn->window_title_to_use)
		{
			SOM::caption = wn->window_title_to_use;
		}
		else SOM::caption = SOM::Game::title();
	}
	else if(DDRAW::target=='_dx6') switch(SOM::tool)
	{
	case SOM_PRM.exe: case SOM_MAP.exe:
	
		//assert(DDRAW::target=='_dx6'); //2021: upgrading SOM_MAP to D3D9?

		//YOU DON'T WANT TO DISABLE THIS ONE 
		//function: prevent the tools from trying to create infinitely large textures!!
		DDRAW::intercept_callback(DDRAW::DIRECT3D3_ENUMDEVICES_ENUM,som_hacks_EnumDevices_cb3);
		//funny: if you break here colorkey works?!
		//break;
		
		//colorkey/force texture surfaces to 256x256 and take over texture management
		DDRAW::hack_interface(DDRAW::DIRECTDRAW4_CREATESURFACE_HACK,som_hacks_CreateSurface4);
		DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE3_SETRENDERSTATE_HACK,som_hacks_SetRenderState3);
		
		//get/release DC for StretchBlt
		DDRAW::hack_interface(DDRAW::DIRECTDRAWSURFACE4_GETDC_HACK,som_hacks_GetDC);
		DDRAW::hack_interface(DDRAW::DIRECTDRAWSURFACE4_RELEASEDC_HACK,som_hacks_ReleaseDC);

		if(SOM::tool==SOM_PRM.exe)
		if(ed->clip_volume_multiplier!=1.0f)
		DDRAW::hack_interface(DDRAW::DIRECT3DVIEWPORT3_SETVIEWPORT2_HACK,som_hacks_SetViewport2);

		//if(ed->default_pixel_value!=0xFF000000)
		DDRAW::hack_interface(DDRAW::DIRECT3DVIEWPORT3_CLEAR2_HACK,som_hacks_Clear2);

		if(SOM::tool==SOM_MAP.exe)
		DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE3_DRAWINDEXEDPRIMITIVE_HACK,som_hacks_DrawIndexedPrimitive3);
		break;
	}
	if(!SOM::game) return;

//	DDRAW::hack_interface(DDRAW::DIRECTDRAW7_QUERYINTERFACE_HACK,som_hacks_QueryInterface_f(IDirectDraw7));
//	DDRAW::hack_interface(DDRAW::DIRECTDRAWSURFACE7_QUERYINTERFACE_HACK,som_hacks_QueryInterface_f(IDirectDrawSurface7));

	DDRAW::hack_interface(DDRAW::DIRECTDRAW7_SETCOOPERATIVELEVEL_HACK,som_hacks_SetCooperativeLevel);
	
//	if(wn->do_scale_640x480_modes_to_setting)	
	DDRAW::hack_interface(DDRAW::DIRECTDRAW7_SETDISPLAYMODE_HACK,som_hacks_SetDisplayMode);
	
	//yellow ascii text on black background bug while changing options
	DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_ENDSCENE_HACK,som_hacks_EndScene);
	//NEW: initialize custom shader constants
	DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_BEGINSCENE_HACK,som_hacks_BeginScene);
				 
	if(DDRAW::compat=='dx9c') //mipmaps required
	if(op->do_lights||bf->do_fix_lighting_dropout)
	{												  
		if(op->do_lights)
		{
			if(dt->lights_lamps_limit)
			{
				som_hacks_lamps = min(16-3,dt->lights_lamps_limit);
			}
			else som_hacks_lamps = 16-3; //current maximum
		}		
		else som_hacks_lamps = 3; //SOM emulation (kinda)
	}

	DDRAW::hack_interface(DDRAW::DIRECTDRAWSURFACE7_BLT_HACK,som_hacks_Blt);
	DDRAW::hack_interface(DDRAW::DIRECTDRAWSURFACE7_FLIP_HACK,som_hacks_Flip);

	if(op->do_mipmaps&&DDRAW::compat=='dx9c') //experimental: texture key registration
	DDRAW::hack_interface(DDRAW::DIRECTDRAWSURFACE7_SETCOLORKEY_HACK,som_hacks_SetColorKey);

	DDRAW::hack_interface(DDRAW::DIRECTDRAWGAMMACONTROL_SETGAMMARAMP_HACK,som_hacks_SetGammaRamp);

	//shaders/lighting/reset
	DDRAW::hack_interface(DDRAW::DIRECT3D7_CREATEDEVICE_HACK,som_hacks_CreateDevice);
	
	//clamping/compass/item display clipping
	DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_CLEAR_HACK,som_hacks_Clear);
	
//	if(som_hacks_lamps)
	DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_SETTRANSFORM_HACK,som_hacks_SetTransform);

	DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_GETTRANSFORM_HACK,som_hacks_GetTransform);

	if(som_hacks_lamps)
	DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_SETLIGHT_HACK,som_hacks_SetLight);
	
//	if(op->do_rangefog) //gamma
	DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_SETRENDERSTATE_HACK,som_hacks_SetRenderState);

//	if(bf->do_fix_clipping_item_display||bf->do_fix_oversized_compass_needle)
	DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_DRAWPRIMITIVE_HACK,som_hacks_DrawPrimitive);

//	if(som_hacks_lamps||op->do_alphafog)
	DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_DRAWINDEXEDPRIMITIVEVB_HACK,som_hacks_DrawIndexedPrimitiveVB);
	//2021: this should be identical to VB, it's an alternative path
	//for when drawing without a vbuffer (som.scene.cpp uses this in
	//some experiments and there is vbuffer-less code in som_db.exe)
	DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_DRAWINDEXEDPRIMITIVE_HACK,som_hacks_DrawIndexedPrimitive);

	/*disabling som.keys.h
	if(op->do_mipmaps&&DDRAW::compat=='dx9c') //experimental: texture key registration
	DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_SETTEXTURE_HACK,som_hacks_SetTexture);*/

//	if(op->do_mipmaps&&DDRAW::compat=='dx9c') //experimental: texture key registration
	DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_SETTEXTURESTAGESTATE_HACK,som_hacks_SetTextureStageState);

	if(som_hacks_lamps)
	DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_LIGHTENABLE_HACK,som_hacks_LightEnable);

	//2021: adjusting maximum batch size (mode 3 only for now)
	DDRAW::hack_interface(DDRAW::DIRECT3D7_CREATEVERTEXBUFFER_HACK,som_hacks_CreateVertexBuffer);
		
	//DUPLICATE
	//
	// seems related to som_hacks_SetGammaRamp/do_gamma
	//
	if(!DDRAW::shader //2018: new gamma?
	||!op->do_gamma) //ancient gamma mode
	{
		//HACK: seems to fix weird polar sky cap??
		//BIZARRE: must figure this out sometime???
		//DDRAW::brights[0] = 1; DDRAW::dimmers[0] = 1;

		for(int i=1;i<9+8;i++) 
		{
			DDRAW::brights[i] = 1*i; DDRAW::dimmers[i] = 4*i;
		}
	}

	if(op->do_mipmaps/*&&dt->mipmaps_pixel_art_power_of_two*/)
	{
		DDRAW::hack_interface(DDRAW::DIRECTDRAW7_CREATESURFACE_HACK,som_hacks_CreateSurface7);
	}
	
	//DSOUND::hack_interface(DSOUND::DIRECTSOUNDBUFFER_SETVOLUME_HACK,som_hacks_SetVolume);

	DSOUND::hack_interface(DSOUND::DIRECTSOUND3DLISTENER_SETORIENTATION_HACK,som_hacks_SetOrientation);
	DSOUND::hack_interface(DSOUND::DIRECTSOUND3DLISTENER_SETVELOCITY_HACK,som_hacks_SetVelocity);
	DSOUND::hack_interface(DSOUND::DIRECTSOUND3DLISTENER_SETPOSITION_HACK,som_hacks_SetPosition);
			
	DINPUT::hack_interface(DINPUT::DIRECTINPUTA_ENUMDEVICES_HACK,som_hacks_EnumDevices_diA);

	if(!EX::INI::Joypad(1)&&DINPUT::doIDirectInputDeviceA)
	DINPUT::hack_interface(DINPUT::DIRECTINPUTA_CREATEDEVICE_HACK,som_hacks_CreateDevice_diA);

	DINPUT::hack_interface(DINPUT::DIRECTINPUTDEVICEA_GETDEVICESTATE_HACK,som_hacks_GetDeviceState);

	//2018: read somewhere this gives A/V applications priority
	HMODULE av = LoadLibraryA("Avrt.dll"); if(av)
	{
		//AvRevertMmThreadCharacteristics exists
		//AvSetMmMaxThreadCharacteristics also works?
		DWORD avid = 0;
		if(void*pa=GetProcAddress(av,"AvSetMmThreadCharacteristicsA"))
		{
			HANDLE avh = (HANDLE(WINAPI*)(LPCSTR,LPDWORD))("Games",&avid);
			assert(avh&&avh!=INVALID_HANDLE_VALUE);		
			//should it be freed?
			assert(SOM::game); //FreeLibrary(av);
		}
		else assert(0);
	}

	//EXPERIMENTAL
	//seems to help. Could be imagining things?	seems possible
	if(!SetPriorityClass(GetCurrentProcess(),REALTIME_PRIORITY_CLASS)) assert(0);
	DWORD pri = THREAD_PRIORITY_HIGHEST; //probably a bad idea?
	assert(THREAD_PRIORITY_NORMAL==GetThreadPriority(GetCurrentThread()));
	//pri = THREAD_PRIORITY_ABOVE_NORMAL; //a better idea?
	if(!SetThreadPriority(GetCurrentThread(),pri)) assert(0);	

	PostMessage(0,0,0,0); //AttachThreadInput
	CreateThread(0,0,som_hacks_thread_main,(LPVOID)GetCurrentThreadId(),0,0);
}						
static bool som_hacks_detectXInputDevices = true;
static DWORD WINAPI som_hacks_thread_main(void* attachtid)
{			
	DWORD tid = GetCurrentThreadId();
	SetThreadName(tid,"EX::som_hacks_thread_main");

	PostMessage(0,0,0,0); //AttachThreadInput
	AttachThreadInput(tid,(DWORD)attachtid,1);	

	//ensure same priority just in case changed
	HANDLE ot =	OpenThread(THREAD_QUERY_INFORMATION,0,(DWORD)attachtid);
	if(!SetThreadPriority(GetCurrentThread(),GetThreadPriority(ot))) assert(0);
	CloseHandle(ot);

	DWORD timeout = 250; //quarter of a second

	EX::INI::Window wn;
	EX::INI::Option op; EX::INI::Detail dt;

	if(!op->do_cursor) timeout = 0;
	if(dt->cursor_hourglass_ms_timeout)
	timeout = dt->cursor_hourglass_ms_timeout;

	DWORD fpsref = 0, timer = EX::tick();								   

	bool waspaused = false; 
	
	bool lexed = wcschr(wn->window_title_to_use,'<')!=0; //2022

	int counter = 0; enum{ time_slice=20 }; //2022

	MSG keepalive; //NEW
	//just keep from going "Not Responding"
	while(PeekMessageW(&keepalive,0,0,0,0))
	{
	loop: counter++; //2022

		//2021: the D3D9 dll is hanging inside
		//IDirect3DVertexBuffer7::Release only
		//in release build
		if(SOM::altf&1<<4)
		while(PeekMessage(&keepalive,0,0,0,PM_REMOVE))
		{
			//wait for 'exit' WM_TIMER to kill som_db.exe :(
			//hopefully the INI flushes prior to teardown
			TranslateMessage(&keepalive);
			DispatchMessage(&keepalive); 
		}

		if(lexed&&1==counter%(1000/time_slice))
		if(HWND window=EX::display())
		{
			wchar_t text[MAX_PATH];
			SetWindowTextW(window,SOM::lex(wn->window_title_to_use,text,MAX_PATH));
		}

		if(DDRAW::isPaused)
		{
			waspaused = true;

			EX::sleep(200); continue;
		}
		else EX::sleep(time_slice); //20					

		DWORD fpsclk = EX::tick();

		if(SOM::eticks) //et
		{
			static DWORD diff = 0;

			if(waspaused) SOM::eticks = fpsclk;			

			diff = fpsclk;

			if(diff<SOM::eticks) 
			{
				diff+=~0u-SOM::eticks; SOM::eticks = 0; 
			}

			if(diff-SOM::eticks>=1000)
			{
				SOM::eticks+=1000; 
				SOM::et++;
			}
		}

		//2020: the cursor is popping up after 
		//unpausing, but only the second time?
		//(problem must be elsewhere)
		if(waspaused) timer = fpsclk;
		//if(waspaused) timer = fpsclk+500;

		if(timeout)
		{				
			if(SOM::frame-fpsref)
			{
				fpsref = SOM::frame; 

				if(EX::is_waiting()) //nice
				{
					EX::hiding_cursor('wait');

					EX::following_cursor(); //hack
				}

				timer = fpsclk; //2020
			}
			
			if(fpsclk>timer) //2020							
			if(fpsclk-timer>timeout)
			{	
				//TODO: keep input alive
				EX::showing_cursor('wait');

				EX::following_cursor(); //hack
			}
		}

		waspaused = false;

		//HACK: detectXInputDevices does who knows what that
		//blocks the main thread during start up
		if(som_hacks_detectXInputDevices&&DINPUT::noPolls>0)
		{
			som_hacks_detectXInputDevices = false;
			EX::Joypad::detectXInputDevices();
		}

		if(!waspaused) timer = fpsclk;
	}
	if(EX::debug)
	{
		//2022: PeekMessage doesn't guarantee to keep going!
		assert(SOM::altf&1<<4); 
	}
	goto loop; //return 0; //2022
}

namespace som_hacks_d3d11
{
	const char *ps = nullptr;

	struct 
	{
		//WARNING: this first register is reserved
		//to be filled out by flip() 
		float ps2ndSceneMipmapping2[4]; //skyRegister
	//	float rcpViewport[4];
		float colCorrect[4];
		float colColorize[4];
	}cb;

	void try_DDRAW_fxWGL_NV_DX_interop2()
	{
		if(DDRAW::WGL_NV_DX_interop2)
		DDRAW::fxWGL_NV_DX_interop2(ps,&cb,sizeof(cb));
	}
}
 
template<int N>
static int som_hacks_vs_(int)
{
	int out = N; //new

	SOM::VS<N>::undef(SOM::Define::defined);

	EX::INI::Option op; EX::INI::Detail dt; 
	EX::INI::Stereo vr; EX::INI::Bitmap bm;
	
	bool aa = op->do_aa2;

	if(SOM::tool) goto tool; //!

	if(op->do_alphafog) SOM::VS<N>::define(SOM::Define::alphafog); //UNUSED
	if(op->do_rangefog) SOM::VS<N>::define(SOM::Define::rangefog); //UNUSED	
	if(op->do_hdr)		SOM::VS<N>::define(SOM::Define::hdr);

	if(&bm->bitmap_ambient) SOM::VS<N>::define(SOM::Define::ambient2);
	
	if(N==3&&DDRAW::inStereo) 
	{
		aa = !vr->do_not_aa; SOM::VS<3>::define(SOM::Define::stereo);
	}

	tool:
	if(aa) SOM::VS<N>::define(SOM::Define::aa);	

	const wchar_t *n = 0; //blendN?
	if(op->do_gamma) //EXPERIMENTAL (KF2)
	if(n=dt->gamma_n) if(!*n||!SOM::game) n = 0;
	if(n) SOM::VS<N>::define(SOM::Define::gamma_n,n);

	EXLOG_LEVEL(-1) << "Vertex Shader Model " << N << "\n"; 

	if(SOM::tool) //OPTIMIZING (2021)
	{
		//NOTE: som_shader_compile_vs was rejecting the
		//other shaders before this code was instituted

		if(DDRAW::gl)
		{
			if(N!=3){ assert(0); return 0; } //VS<>?

			DDRAW::vshadersGL_include = SOM::VS<>::header(SOM::Shader::classic);			
			DDRAW::vshadersGL[2] = SOM::VS<>::shader(SOM::Shader::classic_unlit);
			DDRAW::vshadersGL[3] = SOM::VS<>::shader(SOM::Shader::classic_blended);
			DDRAW::vshadersGL[16] = SOM::VS<>::shader(SOM::Shader::effects);
		}
		else
		{
			delete[] DDRAW::vshaders9[2]; 
			delete[] DDRAW::vshaders9[3]; 
			delete[] DDRAW::vshaders9[16];

			//unlit is technically needed for the alteranative FVF layout
			char *unlit = SOM::VS<N>::compile(SOM::Shader::classic_unlit);
			char *blended = SOM::VS<N>::compile(SOM::Shader::classic_blended);
			char *effects = SOM::VS<N>::compile(SOM::Shader::effects);

			int out; if(out=unlit&&blended&&effects?N:0)
			{
				SOM::VS<N>::configure(3,unlit,blended,effects);

				DDRAW::vshaders9[2] = SOM::VS<N>::assemble(unlit);
				DDRAW::vshaders9[3] = SOM::VS<N>::assemble(blended);
				DDRAW::vshaders9[16] = SOM::VS<N>::assemble(effects);
			}
			SOM::VS<N>::release(3,unlit,blended,effects);

			return out;
		}
	}
	else if(DDRAW::gl) //OpenGL
	{
		//2021: unlike D3D9 these are straight GLSL strings

		if(N!=3){ assert(0); return 0; } //VS<>?

		DDRAW::vshadersGL_include = SOM::VS<>::header(SOM::Shader::classic);

		DDRAW::vshadersGL[4] = SOM::VS<>::shader(SOM::Shader::classic_fog);		
		DDRAW::vshadersGL[1] = SOM::VS<>::shader(SOM::Shader::classic_blit);
		DDRAW::vshadersGL[2] = SOM::VS<>::shader(SOM::Shader::classic_unlit);
		DDRAW::vshadersGL[5] = SOM::VS<>::shader(SOM::Shader::classic_sprite);
		if(SOM::game)
		if(op->do_alphafog) //REMINDER: 7/SHADER_INDEX is hardcoded into GLSL
		DDRAW::vshadersGL[7] = SOM::VS<>::shader(SOM::Shader::classic_shadow);	
		else
		DDRAW::vshadersGL[7] = 0; //Exselector?		
		DDRAW::vshadersGL[3] = SOM::VS<>::shader(SOM::Shader::classic_blended);
	//	DDRAW::vshadersGL[8] = SOM::VS<>::shader(SOM::Shader::classic_volume);
		DDRAW::vshadersGL[8] = n?DDRAW::vshadersGL[3]:0;
		DDRAW::vshadersGL[6] = SOM::VS<>::shader(SOM::Shader::classic_backdrop);		
		DDRAW::vshadersGL[16] = SOM::VS<>::shader(SOM::Shader::effects);

		//full screen fill shader (SHADER_INDEX) for VR
		if(DDRAW::xr) DDRAW::vshadersGL[15] = DDRAW::vshadersGL[1];
	}
	else //Direct3D 9
	{
		for(size_t i=0;i<EX_ARRAYSIZEOF(DDRAW::vshaders9);i++) 
		{
			delete[] DDRAW::vshaders9[i]; DDRAW::vshaders9[i] = 0;
		}

		//this is getting ridiculous
		char *fog,*blit,*unlit,*blended,*blendN=0,*sprite,*shadow=0,*backdrop,*effects; 

		//fog is identical to unlit
		fog      = SOM::VS<N>::compile(SOM::Shader::classic_fog);
		blit     = SOM::VS<N>::compile(SOM::Shader::classic_blit);
		unlit    = SOM::VS<N>::compile(SOM::Shader::classic_unlit);
		sprite   = SOM::VS<N>::compile(SOM::Shader::classic_sprite);
		if(SOM::game) //2021
		if(N>2&&op->do_alphafog) //2: ddx/y support isn't there
		shadow   = SOM::VS<N>::compile(SOM::Shader::classic_shadow);		
		blended  = SOM::VS<N>::compile(SOM::Shader::classic_blended);		
		backdrop = SOM::VS<N>::compile(SOM::Shader::classic_backdrop);		
		effects  = SOM::VS<N>::compile(SOM::Shader::effects);
		if(n)	   //2021: #define SHADER_INDEX 8 (DGAMMA_N)
		blendN   = SOM::VS<N>::compile(SOM::Shader::classic_blended,8);

		int out = 0; if(SOM::game) out = 
		fog&&blit&&unlit&&blended&&sprite/*&&shadow*/&&backdrop&&effects?N:0;
		else if(SOM::tool&&blended) out = N;
		if(out)
		{
		SOM::VS<N>::configure(9,fog,blit,unlit,blended,blendN,sprite,shadow,backdrop,effects);

		if(fog) EXLOG_LEVEL(1) << "FOG\n" << fog << "#####\n";
		if(blit) EXLOG_LEVEL(1) << "BLIT\n" << blit << "#####\n";
		if(unlit) EXLOG_LEVEL(1) << "UNLIT\n" << unlit << "#####\n";
		if(sprite) EXLOG_LEVEL(1) << "SPRITE\n" << sprite << "#####\n";
		if(shadow) EXLOG_LEVEL(1) << "SHADOW\n" << shadow << "#####\n";		
		if(blended) EXLOG_LEVEL(1) << "BLENDED\n" << blended << "#####\n";
		if(backdrop) EXLOG_LEVEL(1) << "BACKDROP\n" << backdrop << "#####\n";				
		if(effects) EXLOG_LEVEL(1) << "EFFECTS\n" << effects << "#####\n";

		//what about DDRAW::vshaders9[0]?
		DDRAW::vshaders9[4] = SOM::VS<N>::assemble(fog);
		DDRAW::vshaders9[1] = SOM::VS<N>::assemble(blit);
		DDRAW::vshaders9[2] = SOM::VS<N>::assemble(unlit);
		DDRAW::vshaders9[5] = SOM::VS<N>::assemble(sprite);
		if(shadow)
		DDRAW::vshaders9[7] = SOM::VS<N>::assemble(shadow);
		DDRAW::vshaders9[3] = SOM::VS<N>::assemble(blended);
		if(blendN)
		DDRAW::vshaders9[8] = SOM::VS<N>::assemble(blendN);
		DDRAW::vshaders9[6] = SOM::VS<N>::assemble(backdrop);			
		DDRAW::vshaders9[16] = SOM::VS<N>::assemble(effects);
		}
		SOM::VS<N>::release(9,fog,blit,unlit,blended,blendN,sprite,shadow,backdrop,effects);
		return out;
	}
	return N;
}

template<int N>
static int som_hacks_ps_(int)
{
	SOM::PS<N>::undef(SOM::Define::defined);	

	EX::INI::Option op;	EX::INI::Detail dt;	
	EX::INI::Adjust ad;	EX::INI::Editor ed;
	EX::INI::Stereo vr;

	//HACK: SOM::Define::colorkey tests DDRAW::colorkey
	if(2000!=dt->alphafog_colorkey&&op->do_alphafog) 
	{
		//2020: I'm moving this here from SomEx.cpp to be
		//sure shaders are available

		//want d3d9 with shaders and not 16-bit textures
		//if(!DDRAW::shader)
		if(DDRAW::doMipmaps)
		{
			DDRAW::colorkey = SOM::colorkey;

			//TODO: this requirement can be removed if the
			//colorkey test is ever required with 3
			if(N>=3)
			{
				//2020: can disable management of this state
				//the EX::INI::Volume extensions repurpose it
				//TODO: this is a 2nd general purpose register
				DDRAW::psColorkey = 0;
				SOM::VT::fog_register = DDRAW::psColorkey_nz;
			}
		}
	}
	SOM::PS<N>::define(SOM::Define::colorkey);
	
	if(DDRAW::do565) SOM::PS<N>::define(SOM::Define::green);
	if(DDRAW::sRGB) SOM::PS<N>::define(SOM::Define::sRGB,1); //2020

	if(SOM::game) //SOM::gamma?
	{
		assert(!DDRAW::doGamma);
		//if(!DDRAW::doGamma) 
		SOM::PS<N>::define(SOM::Define::brightness);
		if(op->do_invert)
		SOM::PS<N>::define(SOM::Define::invert);	
	}

	if(op->do_gamma) //EXPERIMENTAL (KF2)	
	if(const wchar_t*y=dt->gamma_y) if(*y)
	{
		SOM::PS<N>::define(SOM::Define::gamma_y,y);

		//EXPERIMENTAL
		int n = dt->gamma_y_stage;
		if(n<1||n>3) n = 2; //default?
		SOM::PS<N>::define(SOM::Define::gamma_y_stage,n);
	}	

	if(DDRAW::xr)
	{
		if(const wchar_t*y=vr->stereo_y) if(*y)
		{
			SOM::PS<N>::define(SOM::Define::stereo_y,y);
		}
		if(const wchar_t*w=vr->stereo_w) if(*w)
		{
			SOM::PS<N>::define(SOM::Define::stereo_w,w);
		}
	}

	if(!op->do_opengl)
	if(SOM::bpp==16||op->do_highcolor)
	if(!DDRAW::inStereo||vr->do_not_force_full_color_depth)
	{
		SOM::PS<N>::define(SOM::Define::highcolor);

		if(DDRAW::doDither)  
		{
			if(DDRAW::inStereo?!vr->do_not_dither:op->do_dither)
			{
				//going to assume 2x2 dither is too much in VR
				int dither = op->do_dither2&&!SOM::stereo?2:1;
				SOM::PS<N>::define(SOM::Define::dither,dither);
			}
			if(DDRAW::inStereo?vr->do_stipple
			:op->do_stipple&&(!SOM::tool||!ed->do_not_stipple))
			{
				//2020: must specify 2 to use do_aa
				SOM::PS<N>::define(SOM::Define::stipple,op->do_stipple2?2:1);

				//HACK: fxStrobe now overrides/conflicts with new texture DAA
				DDRAW::fxStrobe = op->do_stipple2?8:0;
			}
		}
		else assert(SOM::tool); //stereo
	}	
	
	//temporal antialising
	if(DDRAW::do2ndSceneBuffer)
	SOM::PS<N>::define(SOM::Define::dissolve,N>=3&&DDRAW::doMipSceneBuffers?2:1);
	//TODO: for tools to work there'll need to be a 2 point variant
	if(!DDRAW::fxStrobe&&op->do_aa&&SOM::game) 
	SOM::PS<N>::define(SOM::Define::aa);
	if(SOM::superMode&&N>=3) //tex2Dlod
	SOM::PS<N>::define(SOM::Define::ss); //EXPERIMENTAL

	float powers[4] = {}; if(SOM::game) //!SOM::tool?
	{
		//TODO? make a variable number //2022: yes for blending/programming

		powers[0] = ad->fov_sky_and_fog_powers;
		powers[1] = &ad->fov_sky_and_fog_powers2?ad->fov_sky_and_fog_powers2:powers[0];

		//2021: the OpenGL setup can't redefine DFOGPOWERS
		{
			if(&ad->fov_sky_and_fog_powers3) powers[2] = ad->fov_sky_and_fog_powers3;
			else powers[2] = powers[0]*0.7f;
			if(&ad->fov_sky_and_fog_powers4) powers[3] = ad->fov_sky_and_fog_powers4;
			else powers[3] = powers[1]*0.35f;
		}

		//2021: sprite fog is impossible this way, relying on compiler
		//to optimize pow(x,1) away
		//if(powers[0]!=1||powers[1]!=1)
		{
			//NOTE: there was a BUG here up to 1.2.1.8. The shader is reverse
			//of the INI extension. they must be passed in reverse order: 1,0.		
			SOM::PS<N>::define(SOM::Define::fogpowers,powers[1],powers[0],powers[3],powers[2]);
		}

		if(N==3&&DDRAW::inStereo)
		{
			SOM::PS<3>::define(SOM::Define::stereo);	
			float ss = 1; DDRAW::doSuperSamplingMul(ss);
			//if(DDRAW::do2ndSceneBuffer) //???
			//if(float lod=&vr->fuzz_constant?vr->fuzz_constant(ss):ss>1?0.5f:0.75f)
			if(!DDRAW::gl) //PSVR?
			if(float lod=&vr->fuzz_constant?vr->fuzz_constant(ss):0.5f)
			SOM::PS<3>::define(SOM::Define::stereoLOD,lod);	
		}
	}

	EXLOG_LEVEL(-1) << "Pixel Shader Model " << N << "\n"; 
	
	if(SOM::tool) //OPTIMIZING (2021)
	{
		//NOTE: som_shader_compile_ps was rejecting the
		//other shaders before this code was instituted

		if(DDRAW::gl)
		{
			if(N!=3){ assert(0); return 0; } //PS<>?

			DDRAW::pshadersGL_include = SOM::PS<>::header(SOM::Shader::classic);			
			DDRAW::pshadersGL[2] = SOM::PS<>::shader(SOM::Shader::classic_unlit);
			DDRAW::pshadersGL[3] = SOM::PS<>::shader(SOM::Shader::classic_blended);
			DDRAW::pshadersGL[16] = SOM::PS<>::shader(SOM::Shader::effects);
		}
		else
		{
			delete[] DDRAW::pshaders9[2]; 
			delete[] DDRAW::pshaders9[3]; 
			delete[] DDRAW::pshaders9[16];

			char *unlit = SOM::PS<N>::compile(SOM::Shader::classic_unlit);
			char *blended = SOM::PS<N>::compile(SOM::Shader::classic_blended);
			char *effects = SOM::PS<N>::compile(SOM::Shader::effects);

			int out; if(out=unlit&&blended&&effects?N:0)
			{
				SOM::PS<N>::configure(3,unlit,blended,effects);

				DDRAW::pshaders9[2] = SOM::PS<N>::assemble(unlit);
				DDRAW::pshaders9[3] = SOM::PS<N>::assemble(blended);
				DDRAW::pshaders9[16] = SOM::PS<N>::assemble(effects);
			}
			SOM::PS<N>::release(3,unlit,blended,effects);

			return out;
		}
	}
	else if(DDRAW::gl) //OpenGL
	{
		//2021: unlike D3D9 these are straight GLSL strings

		if(N!=3){ assert(0); return 0; } //PS<>?

		DDRAW::pshadersGL_include = SOM::PS<>::header(SOM::Shader::classic);

		DDRAW::pshadersGL[4] = SOM::PS<>::shader(SOM::Shader::classic_fog);
		DDRAW::pshadersGL[1] = SOM::PS<>::shader(SOM::Shader::classic_blit);
		DDRAW::pshadersGL[2] = SOM::PS<>::shader(SOM::Shader::classic_unlit);		
		//if(shadow)
		DDRAW::pshadersGL[7] = SOM::PS<>::shader(SOM::Shader::classic_shadow);
		//if(volume)
		DDRAW::pshadersGL[8] = SOM::PS<>::shader(SOM::Shader::classic_volume);
		DDRAW::pshadersGL[3] = SOM::PS<>::shader(SOM::Shader::classic_blended);		
		DDRAW::pshadersGL[5] = SOM::PS<>::shader(SOM::Shader::classic_sprite);
		DDRAW::pshadersGL[6] = SOM::PS<>::shader(SOM::Shader::classic_backdrop);		
		DDRAW::pshadersGL[16] = SOM::PS<>::shader(SOM::Shader::effects);
		//if(DDRAW::xr)
		som_hacks_d3d11::ps = SOM::PS<>::shader(SOM::Shader::effects_d3d11);
	}
	else //Direct3D 9
	{
		for(size_t i=0;i<EX_ARRAYSIZEOF(DDRAW::pshaders9);i++) 
		{
			delete[] DDRAW::pshaders9[i]; DDRAW::pshaders9[i] = 0;
		}

		//this is getting ridiculous
		char *blit,*unlit,*blended,*fog,*sprite,*shadow=0,*volume=0,*backdrop,*effects; 

		fog	     = SOM::PS<N>::compile(SOM::Shader::classic_fog);
		blit     = SOM::PS<N>::compile(SOM::Shader::classic_blit);
		unlit    = SOM::PS<N>::compile(SOM::Shader::classic_unlit);		
				   //2021: #define SHADER_INDEX 5 (DFOGPOWERS)
		sprite   = SOM::PS<N>::compile(SOM::Shader::classic_sprite,5);
		//2: ddx/ddy support isn't there
		//2: volume is using vpos instead of another input parameter
		if(N>2&&op->do_alphafog&&DDRAW::d3d9cNumSimultaneousRTs>1) 
		{
			if(EX::INI::Volume())
			volume = SOM::PS<N>::compile(SOM::Shader::classic_volume);
			shadow = SOM::PS<N>::compile(SOM::Shader::classic_shadow);			
		}
		blended = SOM::PS<N>::compile(SOM::Shader::classic_blended);
		backdrop = SOM::PS<N>::compile(SOM::Shader::classic_backdrop);
		effects  = SOM::PS<N>::compile(SOM::Shader::effects);

		int out = 0; if(SOM::game) out = 
		blit&&unlit&&blended&&fog&&sprite/*&&shadow&&volume*/&&backdrop&&effects?N:0;
		else if(SOM::tool&&blended) out = N;
		if(out)
		{
		SOM::PS<N>::configure(9,fog,blit,unlit,blended,sprite,shadow,volume,backdrop,effects);

		if(fog) EXLOG_LEVEL(1) << "FOG\n" << fog << "#####\n";
		if(blit) EXLOG_LEVEL(1) << "BLIT\n" << blit << "#####\n";
		if(unlit) EXLOG_LEVEL(1) << "UNLIT\n" << unlit << "#####\n";
		if(sprite) EXLOG_LEVEL(1) << "SPRITE\n" << sprite << "#####\n";
		if(shadow) EXLOG_LEVEL(1) << "SHADOW\n" << shadow << "#####\n";		
		if(volume) EXLOG_LEVEL(1) << "VOLUME\n" << volume << "#####\n";		
		if(blended) EXLOG_LEVEL(1) << "BLENDED\n" << blended << "#####\n";		
		if(backdrop) EXLOG_LEVEL(1) << "BACKDROP\n" << backdrop << "#####\n";				
		if(effects) EXLOG_LEVEL(1) << "EFFECTS\n" << effects << "#####\n";

		DDRAW::pshaders9[4] = SOM::PS<N>::assemble(fog);
		DDRAW::pshaders9[1] = SOM::PS<N>::assemble(blit);
		DDRAW::pshaders9[2] = SOM::PS<N>::assemble(unlit);
		DDRAW::pshaders9[5] = SOM::PS<N>::assemble(sprite);
		if(shadow)
		DDRAW::pshaders9[7] = SOM::PS<N>::assemble(shadow);
		if(volume)
		DDRAW::pshaders9[8] = SOM::PS<N>::assemble(volume);
		DDRAW::pshaders9[3] = SOM::PS<N>::assemble(blended);		
		DDRAW::pshaders9[6] = SOM::PS<N>::assemble(backdrop);		
		DDRAW::pshaders9[16] = SOM::PS<N>::assemble(effects);
		}
		SOM::PS<N>::release(9,fog,blit,unlit,blended,sprite,shadow,volume,backdrop,effects);
		return out;
	}
	return N;
}

//SOM TOOL SPECIFIC HACKS (todo: move to som.tool.cpp)

static HRESULT som_hacks_EnumDevices_cb3(DX::LPD3DENUMDEVICESCALLBACK cb, GUID *x, LPSTR y, LPSTR z, DX::LPD3DDEVICEDESC w, DX::LPD3DDEVICEDESC q, LPVOID p)
{		
	assert(w&&q);

	//assert(q&&q->dwMaxTextureWidth<=256);
	//assert(q&&q->dwMaxTextureHeight<=256);
	//assert(q&&q->dwMaxTextureRepeat<=256);

	if(!w||!q) return cb(x,y,z,w,q,p);

	EX::INI::Editor ed;

	if(ed->do_hide_direct3d_hardware) 
	{
		if(w->dwFlags) return D3DENUMRET_OK;
	}
	
	UINT sx = 256*ed->texture_subsamples;

	DX::D3DDEVICEDESC hal = *w, hel = *q; //polite: don't corrupt driver memory
	
	if(hal.dwMaxTextureWidth) hal.dwMaxTextureWidth = sx;
	if(hel.dwMaxTextureWidth) hel.dwMaxTextureWidth = sx;
	if(hal.dwMaxTextureHeight) hal.dwMaxTextureHeight = sx;
	if(hel.dwMaxTextureHeight) hel.dwMaxTextureHeight = sx;
	if(hal.dwMaxTextureRepeat) hal.dwMaxTextureRepeat = sx;
	if(hel.dwMaxTextureRepeat) hel.dwMaxTextureRepeat = sx;

	if(hal.dwMaxTextureAspectRatio) hal.dwMaxTextureAspectRatio = 0; //256;
	if(hel.dwMaxTextureAspectRatio) hel.dwMaxTextureAspectRatio = 0; //256;

	return cb(x,y,z,&hal,&hel,p);
}

extern UINT som_map_tileviewmask;  
extern DX::IDirect3DTexture2 *som_hacks_edgetexture = 0; //DUPLICATE
static DX::IDirect3DTexture2 *som_hacks_cliptexture = 0; //DUPLICATE
static void *som_hacks_DrawIndexedPrimitive3(HRESULT*hr,DDRAW::IDirect3DDevice3*in,DX::D3DPRIMITIVETYPE&x,DWORD&y,LPVOID&z,DWORD&w,LPWORD&q,DWORD&r,DWORD&s)
{
	assert(SOM::tool==SOM_MAP.exe);	
	bool mhm = 0x100&som_map_tileviewmask;	
	bool part = 0x400&~som_map_tileviewmask;
	if(part)	
	if(0x200&som_map_tileviewmask) //som_map_refresh_model_view?
	{
		r = 0; return 0;
	}
	else if(!mhm) return 0; 	
	
	static DX::IDirect3DTexture2 *mixedmode = 0; if(hr)
	{
		if(mixedmode) in->SetTexture(0,mixedmode);
		in->SetRenderState(DX::D3DRENDERSTATE_FILLMODE,D3DFILL_SOLID);		
		mixedmode = 0; return 0;
	}

	void *out = 0;
	DWORD fm = D3DFILL_SOLID; 
	in->GetRenderState(DX::D3DRENDERSTATE_FILLMODE,&fm);			
	if(!mixedmode) switch(x) 
	{	
	default: assert(0);
	
	case D3DPT_TRIANGLELIST: 

		//2021: SOM_MAP_this::tileview is covering this
		//if(~som_map_tileviewmask&2) 
		//r = 0;
		assert(som_map_tileviewmask&2);
		mhm = false;
		goto edgemode; //break;

	case D3DPT_TRIANGLEFAN: 
		
		//2021: SOM_MAP_this::tileview is covering this
		//if(~som_map_tileviewmask&4&&!part)
		//r = 0;
		assert(som_map_tileviewmask&4||part);
		if(r) edgemode: 
		if(som_map_tileviewmask&0x40||mhm) //edge mode?
		{
			if(som_map_tileviewmask&0x80||mhm) //mixed mode
			{
				in->GetTexture(0,&mixedmode); if(mixedmode)
				{	
					//Reminder: can't seem to change the color with
					//normal render/light states. The vertex format
					//is probably pre-lit color-wise. 
					in->SetTexture(0,!mhm?0:som_hacks_cliptexture); //white or blue?
					in->SetRenderState(DX::D3DRENDERSTATE_FILLMODE,fm=D3DFILL_SOLID);		
					in->DrawIndexedPrimitive(x,y,z,w,q,r,s);										
					in->SetTexture(0,mhm?0:som_hacks_edgetexture); //black or white?
				}
			}
			if(fm!=D3DFILL_WIREFRAME) //illustrating
			in->SetRenderState(DX::D3DRENDERSTATE_FILLMODE,fm=D3DFILL_WIREFRAME);		
			out = som_hacks_DrawIndexedPrimitive3;
		}
		else if(fm!=D3DFILL_SOLID) //illustrating
		in->SetRenderState(DX::D3DRENDERSTATE_FILLMODE,fm=D3DFILL_SOLID);		
		break;
		
	case D3DPT_LINELIST: if(~som_map_tileviewmask&8) r = 0; break; 
	}			
	static bool bias = false;
	/*2021: SOM_MAP_42000_44000 patches 0x10 to show layers
	if(bias||part||som_map_tileviewmask&0x10)*/
	if(bias!=(x!=D3DPT_LINELIST&&fm!=D3DFILL_WIREFRAME))
	{
		DX::D3DVIEWPORT2 vp = {sizeof(vp)};
		DX::IDirect3DViewport3 *i = 0; in->GetCurrentViewport(&i);

		assert(i); if(i)
		{
			if(!i->GetViewport2(&vp))
			{
				bias = !bias;

				if(~som_map_tileviewmask&0x10&&!part)
				{
					vp.dvMaxZ = 1; assert(!bias);
				}
				else vp.dvMaxZ = bias?0.99f:1;
			
				i->SetViewport2(&vp);
			}
			else assert(0);	i->Release();		   
		}
		else assert(0);
	}
	return out;
}

static void *som_hacks_CreateSurface4(HRESULT*hr, DDRAW::IDirectDraw4*in, DX::LPDDSURFACEDESC2 &x,DX::LPDIRECTDRAWSURFACE4*&y,IUnknown*&)
{
	assert(SOM::tool);
	if(!x||!SOM::tool) return 0;

	if(DDSCAPS_PRIMARYSURFACE&x->ddsCaps.dwCaps)
	{
		//just using opportunity to release 
		if(som_hacks_edgetexture)
		{
			som_hacks_edgetexture->Release(); som_hacks_edgetexture = 0; 
			som_hacks_cliptexture->Release(); som_hacks_cliptexture = 0;
		}
		return 0;
	}
	if(DDSCAPS_TEXTURE&~x->ddsCaps.dwCaps) 
	return 0;

	if(!hr) //DSCAPS_TEXTURE
	{	
		if(0==som_hacks_edgetexture) //HACK
		{
			(int&)som_hacks_edgetexture = 1; //RECURSIVE

			DWORD w = x->dwWidth, h = x->dwHeight;
			for(int i=x->dwWidth=x->dwHeight=1;i<=2;i++)
			{
				DX::IDirectDrawSurface4 *s4;
				in->CreateSurface(x,&s4,0);				
				//Intel (Iris Pro) fix (colorkey?)
				DX::DDBLTFX fill = {sizeof(fill)};
				fill.dwFillColor = i==1?0xFF070707:0xFF2787A7;
				s4->Blt(0,0,0,DDBLT_COLORFILL,&fill);				
				s4->QueryInterface(DX::IID_IDirect3DTexture2,
				(LPVOID*)&(i==1?som_hacks_edgetexture:som_hacks_cliptexture));
				s4->Release();
			}
			x->dwWidth = w; x->dwHeight = h;		
		}			
			
		//SOM_MAP sends all kinds of crazy through here
		//one theory is it sends a 255 byte +1 to hit 0

		if(x->dwWidth==0) //this happens
		{
			//Reminder: seen as expanding iconified window

			x->dwHeight = 0; //observed to be nonzero
			
			//return 0; //causes SOM_MAP's single tile view to hang?
		}
		else if(x->dwHeight==0) //assert(x->dwHeight);
		{
			//causes SOM_MAP's single tile view to go black???
			//return 0;
		}
		else if(x->dwWidth!=x->dwHeight) //todo: ensure proportional
		{
			//REMOVE ME?
			//512: began happening on startup one day???
			//if(x->dwHeight<512) assert(x->dwWidth==x->dwHeight);
			if(x->dwHeight<512)	if(x->dwWidth!=x->dwHeight)
			{
				//causing me a headache
				//EX_BREAKPOINT(0)
			}
		}
		if(1!=x->dwWidth*x->dwHeight) //2018: som_hacks_edgetexture?
		{			
			x->dwWidth = x->dwHeight = 256*
			EX::INI::Editor()->texture_subsamples;

			int todolist[SOMEX_VNUMBER<=0x1020406UL];
			//NOTE: this still covers 0~7 for 32-bit textures, it's
			//always been that way... can't think why it should be
			//so. 2021: I've changed the colorkey to 0~0 except for
			//TIM (MDL) textures, but SOM_MAP doesn't reflect that 
			//change here
			x->ddckCKSrcBlt.dwColorSpaceLowValue = 0;
			x->ddckCKSrcBlt.dwColorSpaceHighValue = 0;
			x->dwFlags|=DDSD_CKSRCBLT;
		}
	}
		
	if(DDRAW::target!='_dx6') return 0;

	//should probably be managed automatically by dx.ddraw.cpp

	void *out = 0;

	if(!hr) 
	{
		if(DDRAW::doIDirectDrawSurface4)
		if(x->ddsCaps.dwCaps2&DDSCAPS2_TEXTUREMANAGE)
		if((x->ddsCaps.dwCaps&DDSCAPS_VIDEOMEMORY)==0)
		{
			x->ddsCaps.dwCaps2&=~DDSCAPS2_TEXTUREMANAGE;   
			x->ddsCaps.dwCaps|=DDSCAPS_SYSTEMMEMORY;
			out = som_hacks_CreateSurface4;
		}
	}
	else if(*hr==DD_OK)
	{
		DDRAW::IDirectDrawSurface4 *p = 
		y?DDRAW::is_proxy<DDRAW::IDirectDrawSurface4>(*y):0; 
				
		assert(p&&!p->query6->texture);

		//this would not be acceptable
		if(!p||p->query6->texture) return 0; 
														
		x->ddsCaps.dwCaps&=~DDSCAPS_SYSTEMMEMORY;	
		x->ddsCaps.dwCaps|=DDSCAPS_VIDEOMEMORY|DDSCAPS_ALLOCONLOAD;

		DX::LPDIRECTDRAWSURFACE4 s = 0;

		//should fail for software emulated devices
		if(((DX::IDirectDraw4*)in->proxy6)->CreateSurface(x,&s,0)==DD_OK)
		{
			s->QueryInterface(DX::IID_IDirect3DTexture2,(LPVOID*)&p->query6->texture);
			s->Release();
		}
			
		x->ddsCaps.dwCaps&=~(DDSCAPS_VIDEOMEMORY|DDSCAPS_ALLOCONLOAD);
	}

	return out;
}

static void *som_hacks_SetRenderState3(HRESULT *hr,DDRAW::IDirect3DDevice3*in,DX::D3DRENDERSTATETYPE&x,DWORD&y)
{
	if(!hr) switch(x)
	{		
	case DX::D3DRENDERSTATE_COLORKEYENABLE: 
	
		//tools always set to 0 until done and then to 1???
		/*2021: SOM_MAP_42000_44000 patches 0x20 to show neighbor content
		y = ~som_map_tileviewmask&0x20&&som_map_tileviewmask&0x400?0:1;*/
		y = 1;
		break;
	}
	return 0;
} 

static void *som_hacks_GetDC(HRESULT*hr, DDRAW::IDirectDrawSurface4*, HDC*&x)
{
	if(!hr) return som_hacks_GetDC;

	if(*hr==DD_OK&&x) SOM::HACKS::stretchblt = *x;

	return 0;
}

static void *som_hacks_ReleaseDC(HRESULT*hr, DDRAW::IDirectDrawSurface4*, HDC&)
{
	SOM::HACKS::stretchblt = 0; return 0;
}

static void *som_hacks_SetViewport2(HRESULT*,DDRAW::IDirect3DViewport3*,DX::LPD3DVIEWPORT2&x)
{	
	if(!x) return 0; //paranoia
	
	EX::INI::Editor ed;
	float zoom = ed->clip_volume_multiplier; if(zoom<0.1f) return 0;	
	/*SOM_PRM enlarges by virtue of a larger model viewing window
	//workshop.cpp needs to match it... NOTE that enlarging isn't
	//necessary, but it's easier to match for now than work on it
	extern float som_tool_enlarge;
	zoom/=som_tool_enlarge;
	*/
	
	static DX::D3DVIEWPORT2 cp; cp = *x;
	cp.dvClipX*=zoom; cp.dvClipY*=zoom; cp.dvClipWidth*=zoom; cp.dvClipHeight*=zoom;

	x = &cp; return 0;
}

static void *som_hacks_Clear2(HRESULT*,DDRAW::IDirect3DViewport3*,DWORD&x,DX::LPD3DRECT&,DWORD&z,DX::D3DCOLOR&w,DX::D3DVALUE&,DWORD&r)
{
	if(0x200&som_map_tileviewmask) //som_map_refresh_model_view?
	{
		z = 0; //short-circuit?
	}
	else w = EX::INI::Editor()->default_pixel_value; return 0;
}


//SOM GAME SPECIFIC HACKS (todo: move to som.game.cpp)

static void *som_hacks_QueryInterface(HRESULT *hr, DDRAW::IDirectDraw7*, REFIID riid, LPVOID* &ppvObj)
{
	if(riid!=DX::IID_IDirectDraw) return 0;

	if(!hr) return som_hacks_QueryInterface_f(IDirectDraw7);

	if(*hr==S_OK||!ppvObj) return 0;

//	*(IDirectDraw**)ppvObj = EX::querying_directdraw_movie_interface();

//	if(*ppvObj) *hr = S_OK;

	return 0;
}									

static void *som_hacks_QueryInterface(HRESULT *hr, DDRAW::IDirectDrawSurface7*, REFIID riid, LPVOID* &ppvObj)
{
	if(riid!=DX::IID_IDirectDrawSurface) return 0;

	if(!hr) return som_hacks_QueryInterface_f(IDirectDrawSurface7);

	if(*hr==S_OK||!ppvObj) return 0;

//	*(IDirectDrawSurface**)ppvObj = EX::querying_directdrawsurface_movie_interface();

//	if(*ppvObj) *hr = S_OK;

	return 0;
}									

static void som_hacks_SetCooperativeLevel_icon(HWND sw)
{	
	//REMINDER: not working if started in desktop resolution
	if(SOM::icon) 
	{
		//SetClassLong(sw,GCL_HICON,(LONG)SOM::icon);		
		//SetClassLong(sw,GCL_HICONSM,(LONG)SOM::icon);				
		SendMessage(sw,WM_SETICON,ICON_BIG,(LPARAM)SOM::icon);
		SendMessage(sw,WM_SETICON,ICON_SMALL,(LPARAM)SOM::icon);		
	}			
	else assert(0);
}
static void *som_hacks_SetCooperativeLevel(HRESULT*, DDRAW::IDirectDraw7*, HWND &x, DWORD &y)
{
	MONITORINFO mi = {sizeof(mi)};
	BOOL gmi = GetMonitorInfo(DDRAW::monitor,&mi);

	//BOUND TO BREAK ONE DAY (FIXES MICROSOFT ICON BUG)
	//Can't seem to fix this problem... tried literally
	//everything. seems like a class act Windows 10 bug
	/*if(SOM::window) //return 0; 
	{
		//HACK: destroy the damn window for a Microsoft bug
		//https://social.msdn.microsoft.com/Forums/en-US/7743d566-7360-4394-ac72-02f748479c4e/
		static bool one_off = !EX::fullscreen; if(!one_off)
		{
			one_off = true;

			//NOTE: the icon stays put if out of fullscreen
			//the game just cannot start in fullscreen mode
			//(windowed or not) and have its icon otherwise
			EX::fullscreen = false; goto icon_workaround;
		}
		else return 0;
	}*/
//	EX::fullscreen = gmi
//	&&(SOM::width>=mi.rcMonitor.right-mi.rcMonitor.left)
//	||(SOM::height>=mi.rcMonitor.bottom-mi.rcMonitor.top);

	bool start; if(start=!SOM::window)
	{
		SOM::window = x; //Override main window

		if(SOM::game&&SOM::Game->WindowProc) 
		{
			SOM::WindowProc = 
			(WNDPROC)SetWindowLong(x,GWLP_WNDPROC,(LONG)SOM::Game->WindowProc);	
		}

		if(!SOM::WindowProc) assert(!"PANIC! Windowproc does not exist??");
		
		SOM::windowX = SOM::config("windowX",0); 
		SOM::windowY = SOM::config("windowY",0);
	}
	//else return 0; //icon_workaround:

	EX::INI::Window wn;			
	HWND sw = //2018
	EX::client = SOM::window;	
	if(DINPUT::can_support_window()
	&&wn->do_force_fullscreen_inside_window)		
	{
		//Note: could use EX::screen if handy
		if(SOM::windowX>0) mi.rcMonitor.left+=SOM::windowX;
		if(SOM::windowY>0) mi.rcMonitor.top+=SOM::windowY;

		//2020: this happens a few times at start up... it's confusing
		//the current Windows update... Windows is no longer stable :(
		if(start||SOM::field)
		{
			EX::creating_window(DDRAW::monitor,mi.rcMonitor.left,mi.rcMonitor.top);
		}

		if(EX::window&&!EX::fullscreen)
		{
			SOM::windowX = EX::coords.left-EX::screen.left;
			SOM::windowY = EX::coords.top-EX::screen.top;
		}

		sw = EX::display();
	}
	else 
	{	
		//new default behavior
		if(!wn->do_not_compromise_fullscreen_mode)
		{
			//2021: see above comments about flickering
			if(start||SOM::field) 
			EX::creating_window(DDRAW::monitor,mi.rcMonitor.left,mi.rcMonitor.top);
			
			sw = EX::display();

			//breifly shows smaller white window
			//ShowWindow(sw,SW_SHOWNORMAL); 
			//ShowWindow(sw,SW_SHOW);
		}
		else
		{
			//the launcher now hides the window at runtime
			ShowWindow(sw,SW_SHOWNORMAL); 
			ShowWindow(sw,SW_SHOW);
		}
	}
	SOM::initialize_taskbar(sw);
	//2018: moving here from som_game_WindowProc static bool
	{
		//can't seem to fix this problem... tried literally
		//everything. seems like a class act Windows 10 bug
		//HACK: fails if starting in full screen resolution
		//(even if in windowed mode)
		som_hacks_SetCooperativeLevel_icon(sw);

		if(SOM::caption&&*SOM::caption)	
		{
			const wchar_t *lex = wn->window_title_to_use;
			SetWindowTextW(sw,wcschr(lex,'<')?SOM::lex(lex):SOM::caption);	
		}
		else assert(0);
	}

	if(SOM::capture) //2020: moving out of EX::arrow
	{
		//2022: this is no longer working in January
		//either Windows has a new bug or Microsoft
		//decided users have to click on windows to
		//get catpure... I couldn't fix it no matter
		//what I tried... in the meantime SomEx.cpp 
		//is having SOM::f10 to show the lock cursor
		//but this is pretty annoying

		EX::capturing_cursor();
		EX::clipping_cursor(SOM::cursorX,SOM::cursorY);
	}

	return 0;
}

extern bool som_hacks_setup_shaders(DWORD &z) //workshop.cpp
{		
	EX::INI::Window wn; EX::INI::Option op;
	EX::INI::Stereo vr; EX::INI::Detail dt;

	//NEW: simplifying
	//bool dither = false; 
	//if(op->do_highcolor||SOM::bpp==16)
	//if(DDRAW::compat=='dx9c'&&DDRAW::doDither) 
	//dither = true; 	

	//if(wn->shader_compatibility_level>=3)
	if(DDRAW::compat=='dx9c'&&!DDRAW::shader) 	
	{	
		static int bpp = 0; 
		if(bpp!=SOM::bpp||z=='psvr'||op->do_opengl) //YUCK
		{	
			bpp = SOM::bpp;

			bool aa = op->do_aa2;
			bool mip = op->do_lap;
			bool lap = op->do_lap2;			
			if(DDRAW::inStereo)
			{
				aa = !vr->do_not_aa;
				lap = !vr->do_not_lap;
			}
			//AA can't work with idling
			//tools without two buffers
			if(aa&&SOM::tool) lap = true;

			DDRAW::doFog = true;
			DDRAW::doInvert = false; 
			DDRAW::do2ndSceneBuffer = false;
			//TESTING
			//this sets the DDISSOLVE macro
			//REMINDER: SOM_PRM looks bad with DDISSOLVE==2 in high contrast motion
			DDRAW::doMipSceneBuffers = aa&&mip&&!SOM::tool; //VR doesn't matter now
			DDRAW::dejagrate = aa?3:0; //NEW 
			if(DDRAW::dejagrate&&SOM::tool)
			{
				//not continuously refreshing in this case
				//DDRAW::do3rdSceneBuffer = true;
				DDRAW::dejagrate = 2; 
			}
			if(lap)
			if(!DDRAW::xr) //unstable? //REVISIT ME?
			{
				/*2018: doStencil? I think the idea was to try to halve pixel shader 
				//operations. I think this would not work with the new fancy shadows
				if(op->do_highcolor||SOM::bpp==16)
				//WARNING? can't tell if doStencil is vertically foiling mipmapping??
				(!op->do_aa2&&dither?DDRAW::doStencil:DDRAW::do2ndSceneBuffer) = true;			
				else*/ DDRAW::do2ndSceneBuffer = true;
			}

			//2020: som_hacks_ps_ must define this
			//DDRAW::fxStrobe = op->do_stipple2&&bpp==16?8:0;
			DDRAW::fxStrobe = 0;

			bool stereo = DDRAW::gl?DDRAW::xr:SOM::stereo!=0;

			DDRAW::inStereo = stereo;
			DDRAW::doSuperSampling=SOM::superMode!=0&&!DDRAW::xr;

			som_hacks_shader_model = 0;
			int begin = wn->shader_model_to_use;
			if(DDRAW::DirectDraw7)
			{
				int vsm = DDRAW::DirectDraw7->queryX->vsmodel;
				int psm = DDRAW::DirectDraw7->queryX->psmodel;

				if(!begin) begin = min(vsm,psm);

				while(begin>vsm||begin>psm) begin--;
			}
			else assert(0); //2021

			//DirectDrawCreateEx can now upgrade/downgrade modes on
			//a per adapter basis and may force this off
			int swapRGB = DDRAW::sRGB;
			DDRAW::sRGB = 0?1:op->do_srgb; //...

			do switch(begin)
			{
			case 3: som_hacks_shader_model = 
						
				som_hacks_vs_<3>('_0'); som_hacks_ps_<3>('_0'); break;

			case 2:	som_hacks_shader_model = 
						
				som_hacks_vs_<2>('_0'); som_hacks_ps_<2>('_0'); break;

			case 1:	som_hacks_shader_model = 0; //still unable to make this work
						
//				som_hacks_vs_<1>('_0'); som_hacks_ps_<1>('_0'); break;
			
			//TODO: release shaders on fall-through
			}while(!som_hacks_shader_model&&begin--);  

			//UNTESTED: this should even work with switching on F1
			//but I'm just forcing sRGB on to not have to pay this
			//cost on switch
			if(!som_hacks_shader_model) 
			DDRAW::sRGB = 0;
			if(DDRAW::sRGB!=swapRGB) assert(0); //UNEXPECTED
			if(DDRAW::sRGB!=swapRGB)
			DDRAW::IDirectDrawSurface7::updating_all_textures();

			DDRAW::inStereo = false;
			DDRAW::doSuperSampling = false;
			DDRAW::doSmooth = op->do_smooth;			

			if(som_hacks_shader_model)
			{
				//EXPERIMENTAL
				//HACK: this is speculative... requesting a mipmap in
				//case of VR. it'd be better to reallocate the render
				//targets on the fly
				if(som_hacks_shader_model>=3)
				{					
					if(DDRAW::doSuperSampling=SOM::superMode!=0) //EXPERIMENTAL
					{
						//super-sampling is crisp enough and doesn't benefit without
						/*som.shader.cpp had to go back to blurry mode, but on the 
						//plus side it's now good enough to disable do_smooth in
						//supersampling mode, otherwise it looks pretty similar
						//NOTE: disabling the linear filter performs a little better
						//on my Intel chipset
						DDRAW::doSmooth = true;*/
						/*SOM_MAP is too blurry even without doSmooth... I
						//think maybe the problem here was the high contrast
						//effect was being applied by mistake. it looks fine
						//now, at least when not in motion
						DDRAW::doSmooth = 1||!EX::debug?SOM::tool!=0:true; //false*/
						DDRAW::doSmooth = false;

						//CreateDevice isn't called on VR mode switch
						//if(DDRAW::doMipSceneBuffers==1&&!DDRAW::inStereo)
						{
							//DDRAW::doMipSceneBuffers = 2; //first is for downsampling
							DDRAW::doMipSceneBuffers+=1;
						}
					}

					DDRAW::inStereo = stereo&&SOM::stereo!=0;

					if(DDRAW::inStereo) //swap?
					{
						if(DDRAW::xr) //EXPERIMENTAL
						{
							//It seems like supersampling is built-in to OpenXR
							DDRAW::doMipSceneBuffers = 0;
							DDRAW::doSuperSampling = false;
						}
						else
						{
							float ss = 1; DDRAW::doSuperSamplingMul(ss);

							//if(DDRAW::do2ndSceneBuffer) //???
							DDRAW::doMipSceneBuffers = vr->fuzz_constant(ss)!=0;
						}

						//do_smooth is bad for chromatic aberration, so assume that when
						//fuzz_constant is used to smooth, it is not desirable					
						DDRAW::doSmooth = DDRAW::doMipSceneBuffers?false:vr->do_smooth;

						//I'm thinking ultra resolution might not require or benefit from 
						//this effect since it has some downsides
						/*smooth looks better in dx_d3d9c_update_vsPresentState_staircase
						//with h/3000 with max-supersampling mode
						//plus if eyes don't converge there's no possible dissolve effect
						if(DDRAW::xr&&DDRAW::xrSuperSampling) DDRAW::doSmooth = false;*/
					}
					else assert(!DDRAW::xr); //doSuperSampling?
				}


				//DISABLING: 2 does fixed function fog regardless
				//Note: may as well disable for all shader models
				/*if(som_hacks_shader_model<3)*/ DDRAW::doFog = false;
					
				/*VS1 is not supported
				if(som_hacks_shader_model==1) 
				{
					//ensure that all constant registers are initialized
					DDRAW::minlights = EX::INI::Detail()->lights_vs_1_x_unroll_x;
				}
				else*/ DDRAW::minlights = 0;

				if(op->do_invert) DDRAW::doInvert = true;

				DDRAW::doWhite = true; //default to white texture
			}
			else DDRAW::doWhite = false;

			if(som_hacks_shader_model<3)
			{
				DDRAW::doSuperSampling = false;
				DDRAW::doMipSceneBuffers = false; //tex2Dlod?

				//YUCK: this is technically possible because SomEx.cpp
				//puts it into stereo mode before it's known if there 
				//is support in the shader system
				if(SOM::stereo)
				{
					SOM::altf|=1<<2; //trigger message
					SOM::stereo = 0; 
				}
			}

			if(DDRAW::ps2ndSceneMipmapping2) //TESTING
			{
				if(3>som_hacks_shader_model)
				{
					//I think this is a problem for the deferred shading
					//depth buffer
					DDRAW::ps2ndSceneMipmapping2 = 0;
				}
				else
				{
					DDRAW::xyMapping2[0] = false;
					DDRAW::xyMapping2[1] = DDRAW::doMipSceneBuffers;
				}
			}
			
			//2020: som_hacks_ps_ must define this
			//DDRAW::fxStrobe = op->op->do_stipple2&&bpp==16?8:0;
			DDRAW::fxInflateRect = //REMOVE ME (do_st?)
			DDRAW::fxStrobe||DDRAW::doSmooth||DDRAW::xyMapping2[1]?-1:0;
			som_hacks_ = DDRAW::xr?0:2; //-2*DDRAW::fxInflateRect;
		}
	}
	else
	{
		assert(!som_hacks_shader_model);
		return false;
	}

	//now MSAA/obsolete
	//DDRAW::doAA = op->do_aa2;
	if(/*dither&&*/som_hacks_shader_model)
	{
		z = 32; //if(op->do_stipple) DDRAW::doAA = false; //!		
	}

	return som_hacks_shader_model?true:false;
}
extern void *som_hacks_SetDisplayMode(HRESULT*hr,DDRAW::IDirectDraw7*,DWORD&x,DWORD&y,DWORD&z,DWORD&,DWORD&psvr)
{	
	EX::INI::Window wn; EX::INI::Option op;

	//NOTE: I think I may have forgotten SOM seems to use 16bpp
	//in titles screens and movies. I may have removed code for
	//that recently, but now 401A22 and other edits should make
	//changes a thing of the past

	if(op->do_opengl) //DUPLICATE
	{
		//bpp=16 must be edited in the INI file
		//and only works in D3D9 mode until the
		//DDITHER effect is added to D3D11 mode
		z = SOM::bpp==16&&(!SOM::opengl||!DDRAW::doFlipEx)?16:32; 

		//042133f initializes the menu object
		//from vp_bpp
		SOM::L.vp_bpp = 
		SOM::L.config[3] =
		SOM::L.config2[3] = DDRAW::target=='dxGL'?32:16; //DUPLICATE
	}
	else switch(z)
	{
	case 0: case 16: case 32: break;
	//shouldn't be happening
	default: z = 0; assert(0); break; 
	case 555: z = 16; assert(0); break;
	case 565: z = 32; assert(0); break;
	}
	
	int dx = SOM::width; //SOM::fov[0];
	int dy = SOM::height; //SOM::fov[1];
	assert(dx&&dy);

	/*2022: I think knocking out 401A22
	//makes this obsolete
	if(*(WORD*)0x401a22!=0x2deb //REMOVE ME		
	&&x==640&&y==480 
	&&SOM::frame-SOM::option>1 //movie?
	&&wn->do_scale_640x480_modes_to_setting) 
	{
		//2022: what about movies?
		assert(SOM::L.startup!=-1); //TESTING

		if(!SOM::bpp) //ancient???
		{
			assert(0);
			//SOM::bpp = SOM::config("bpp",32);
			SOM::bpp = 32;
		}

		DDRAW::xyScaling[0] = SOM::width/640.0f;
		DDRAW::xyScaling[1] = SOM::height/480.0f;

		x = SOM::width; y = SOM::height; z = SOM::bpp;
	}
	else*/
	{
		DDRAW::xyScaling[0] = DDRAW::xyScaling[1] = 1;

		SOM::fov[0] = x; SOM::fov[1] = y; 
		
		//SOM::bpp = !z?16:z; //do_opengl?
		if(z&&!op->do_opengl) SOM::bpp = z;
	}
	DDRAW::xyMapping[0] = DDRAW::xyMapping[1] = 0; //TESTING

	//2017: letting these track the current resolution as int
	//Note: SOM::fov is the real resolution in 640 x 480 mode
	if(SOM::frame-SOM::option<=1)
	{
		SOM::width = x; SOM::height = y; 
		
		SOM::mapX = SOM::mapX*x/dx; assert(SOM::mapX<(int)x);
		SOM::mapY = SOM::mapY*y/dy; assert(SOM::mapY<(int)y);
	}

	if(!op->do_opengl) //2022
	if(op->do_highcolor) DDRAW::do565 = SOM::bpp!=16; 

		//2020: moving below so DDRAW::xyMapping can be included
		//som_hacks_setup_shaders(z);
	
	//REFERENCE: REMOVING THIS, FOR BETTER OR WORSE...
	//if(wn->do_not_compromise_fullscreen_mode) return 0;

	bool fullscreen = !wn->do_force_fullscreen_inside_window;

		//HACK: need to populate EX::screen somehow or just sync with
		//the device/display selection
		//if(!EX::window) 
		{
			//'psvr': capturing_screen calls Ex_window_move_to_screen
			//that is probably a source of problems at large but when
			//recomputing the back-light bleed border it's moving the
			//top-left corner to 32,32 instead of 0,0
			if('psvr'!=psvr) 
			EX::capturing_screen(DDRAW::monitor); 
		}
		DWORD cx = EX::screen.right-EX::screen.left;
		DWORD cy = EX::screen.bottom-EX::screen.top;

	bool fullscreen_2019 = false; if(!fullscreen)
	{	
		/*REFERENCE
		//2019: standardizing on having the margin
		//be the outer window
		//http://www.swordofmoonlight.com/bbs/index.php?topic=1144.msg13710#msg13710
		if(SOM::fov[0]>=cx||SOM::fov[1]>=cy)		
		{
			x = cx; y = cy;
			fullscreen = true; goto fullscreen;
		}*/
		/*REMINDER: SOM::Game::get prevents resolution
		//from being higher than the starting screen's
		if(SOM::fov[0]>cx||SOM::fov[1]>cy) //NEW
		{
			float a = x>cx?(float)cx/x:1; 
			float b = y>cy?(float)cy/y:1; 
			if(b<a) a = b;

			x = a*cx; if(x&1) x++; DDRAW::xyScaling[0]*=a;
			y = a*cy; if(y&1) y++; DDRAW::xyScaling[1]*=a;

			assert(x==cx||y==cy);
		}*/
		assert(x<=cx&&y<=cy);
		fullscreen_2019 = SOM::width>=(int)cx||SOM::height>=(int)cy;
	}
	else if(!wn->do_not_compromise_fullscreen_mode) //2021
	{
		fullscreen_2019 = true; fullscreen = false;
	}
	else  
	{
		EX::letterbox(DDRAW::monitor,x,y);
		fullscreen:
		DDRAW::xyMapping[0] = x-SOM::fov[0]; //FYI: will 
		DDRAW::xyMapping[1] = y-SOM::fov[1]; //be halved
	}
	
	if(fullscreen)
	{
		//Note: assuming equally spaced letterboxing desired
		DDRAW::xyMapping[0]*=0.5f; DDRAW::xyMapping[1]*=0.5f;
	}
	else DDRAW::xyMapping[0] = DDRAW::xyMapping[1] = 0.0f; 
				
	//new backlight bleed feature
	if(DWORD b=wn->interior_border) if(!SOM::stereo) //fullscreen? 
	{
		BYTE *bb = (BYTE*)&b;

		LONG l = bb[3], r = bb[2], t = bb[1], b = bb[0];

		LONG mx = l+r, my = t+b;

		SOM::fov[0] = mx<0?x+mx:x-max(0,mx-LONG(cx-x));
		SOM::fov[1] = my<0?y+my:y-max(0,my-LONG(cy-y));

		DDRAW::xyScaling[0]*=SOM::fov[0]/x;
		DDRAW::xyScaling[1]*=SOM::fov[1]/y;

		float lr = l/float(r), tb = t/float(b);
		DDRAW::xyMapping[0]+=(x-SOM::fov[0])/2*lr;
		DDRAW::xyMapping[1]+=(y-SOM::fov[1])/2*tb;
	}
	
		//workaround for static check in som_hacks_setup_shaders
		DWORD &yuck = 'psvr'==psvr?psvr:z;
		//DDRAW::xyScaling/xyMapping are fixed at this point
		som_hacks_setup_shaders(yuck); //z
		//HACK: can't rely on bpp!=SOM::bpp for this
		//if(&EX::INI::Bitmap()->bitmap_ambient) 
		{
			extern DWORD som_scene_ambient2;
			if(som_scene_ambient2) som_scene_ambient2 = -1;
		}

	if(fullscreen_2019) fullscreen = true; //YUCK

	SOM::windowed = !fullscreen; //NEW: see SOM::altf11

	if(EX::window) EX::styling_window(fullscreen,SOM::windowX,SOM::windowY); 

	if(wn->do_not_hide_window_on_startup) EX::showing_window(); 
	
	return 0;
}

static void som_hacks_shadow_tower()
{
	RECT box; UINT how;	  	
	float suby = DDRAW::inStereo?0.7f:0.85f;
	box.left = box.right = SOM::width/2;
	box.top = box.bottom = suby*SOM::height;

	//hack: not actually Shadow Tower related but has to be done anyway
	if(auto*st=SOM::subtitle())
	{
		//when crouching it feels like subtitles should be higher
		if(SOM::frame-SOM::crouched<2)
		{
			box.top-=SOM::height-suby*SOM::height;

			SOM::crouched = SOM::frame; //HACK: wait for subtitle
		}
		
		SOM::Print(st,&box,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
	}
	
	if(!EX::INI::Option()->do_st) return;

	//TODO: the walk effect needs to be bumped off in order
	//to decouple the magic and bad status display

	//2020: this has to work differently since the addition
	//of the ability to toggle displays with attack buttons
	//if(SOM::L.on_off[1]) //health
	if(SOM::gauge) 
	{
		if(SOM::showGauge)
	SOM::Print(SOM::Status(&box,&how,'pts',0),&box,0,&how); //HP
		if(SOM::showCompass)
	SOM::Print(SOM::Status(&box,&how,'pts',1),&box,0,&how); //MP
	}
	if(SOM::L.on_off[3]) //damage
	{
	SOM::Print(SOM::Status(&box,&how,'hit',0),&box,0,&how); //ouch! 
	}
	if(SOM::L.on_off[2]) //status
	{
	SOM::Print(SOM::Status(&box,&how,'bad',0),&box,0,&how); //poison
	SOM::Print(SOM::Status(&box,&how,'bad',1),&box,0,&how); //palsy 
	SOM::Print(SOM::Status(&box,&how,'bad',2),&box,0,&how); //dark
	SOM::Print(SOM::Status(&box,&how,'bad',3),&box,0,&how); //curse
	SOM::Print(SOM::Status(&box,&how,'bad',4),&box,0,&how); //slow
	}
}

extern void som_scene_alphaenable(int fab=0xFAB);
extern void som_hacks_fab_alphablendenable()
{
	//in->SetRenderState(DX::D3DRENDERSTATE_ALPHABLENDENABLE,0xFAB);		
	//in->SetRenderState(DX::D3DRENDERSTATE_SRCBLEND,DX::D3DBLEND_SRCALPHA);		
	//in->SetRenderState(DX::D3DRENDERSTATE_DESTBLEND,DX::D3DBLEND_INVSRCALPHA);		
	som_scene_alphaenable(0xFAB);
}

//REFERENCE?
//Ex.output.cpp uses dx_d3d9c_world to extract the world
//transform matrix, so this can't be set to true anymore
enum{ som_hacks_menuxform=true };

//REMOVE ME? 
//TODO? EX_INI_Z_SLICE_1 can hold menus
//2021: this is an effect to draw items over menus so they're not 
//positioned "behind glass" (especially on the main Equip screen)
enum{ som_hacks_items_over_menus=true };

//DEBUGGING?
//this is to to avoid MRT for OpenGL text but I can't
//think off a way to detect picture menus in som_hacks_Clear
//to decide to not turn on MRT
enum{ som_hacks_early_disable_MRT=false };

extern RECT som_hacks_kf2_gauges = {};
extern float *som_hacks_kf2_compass = 0;
extern float som_hacks_hud_fade[2] = {}; //OpenXR //som.mocap.cpp
static bool som_hacks_displays_a[2] = {};
extern float(*SomEx_output_OpenXR_mats)[4][4];
static int som_hacks_Blt_fan = false; //2022
static void *som_hacks_Blt(HRESULT*hr,DDRAW::IDirectDrawSurface7*in,LPRECT&x,DX::LPDIRECTDRAWSURFACE7&y,LPRECT&z,DWORD&w,DX::LPDDBLTFX&q)
{
	if(in!=DDRAW::BackBufferSurface7) //2022
	{
		if(z&&z->right-z->left==3) //automap?
		{
			SOM::automap = SOM::frame; //REMOVE ME
		
			//if(!hr) som_hacks_automap(x,y,z);

			//x = &mote; return 0; //short-circuit
		}

		return 0;
	}
	if(!y) return 0;

	auto *yy = (DDRAW::IDirectDrawSurface7*)y; 

	assert(!in->query->isPrimary); //2022: flip?
	
	static RECT mote = {};
	if(0)short_circuit:{ x = &mote; return 0; } //goto

	int fan = DDRAW::compat!=0; //2022

	//if(!hr) //blitting to center of screen?
	{
		//knocking out 401A22 makes this obsolete
		//int cx = (SOM::field?SOM::width:640)/2;
		//int cy = (SOM::field?SOM::height:480)/2;
		int cx = SOM::width/2;
		int cy = SOM::height/2;
				
		if(!x||x->left<cx&&x->right>cx&&x->top<cy&&x->bottom>cy)
		{									  
			//if(!in->query->isPrimary) //Picture?
			{	
				fan*=2; //HACK

				som_hacks_blit = SOM::frame; 

				//assuming this cannot be harmful
				/*2021: this is causing title screens to disappaer
				//I can't recall why I added this code (comments!)
				if(som_hacks_tint!=SOM::frame)
				y->SetColorKey(DDCKEY_SRCBLT|DDCKEY_DESTBLT,0);*/
														
				//correct aspect ratio?
				DX::DDSURFACEDESC2 desc; //REMOVE ME
				desc.dwSize = sizeof(desc);
				desc.dwFlags = DDSD_HEIGHT|DDSD_WIDTH;
				if(y->GetSurfaceDesc(&desc)==D3D_OK)
				{
					static RECT xx; if(x)
					{
						CopyRect(&xx,x); x = &xx;
					}
					else SetRect(x=&xx,0,0,cx*2,cy*2);

					float w = x->right-x->left;
					float h = x->bottom-x->top;
					float a = (float)desc.dwHeight/desc.dwWidth;
					a*=DDRAW::xyScaling[0]/DDRAW::xyScaling[1];

					float l,t; //lossy?
					if(a<=1)
					{						
						l = w/-2;						
						t = h/-2*(w/h)*a;	
						a = h/t;						
					}
					else 
					{
						l = w/-2*(h/w)/a;
						t = h/-2;
						a = w/l;
					}
					if(a>-2){ a/=-2; l*=a; t*=a; }
					x->left = l+0.5f; x->top = t+0.5f;
					x->right = cx-x->left; x->left+=cx;
					x->bottom = cy-x->top; x->top+=cy;	
				}
			}	
		}
		else if(x) //HUD (either compass or gauge box?)
		{			
			//HUDs are always on for timing/detection purposes
			if(som_hacks_huds!=SOM::frame)
			{
				som_hacks_huds = SOM::frame;

				//ready for the gauges
				som_hacks_fab = false; 

				//this is to invalidate the tint when exiting a 
				//top level menu because often times additional 
				//frames are rendered which never see the light
				//of day (ie. get flip'ed; see major bug above)
				//
				//specifically this is done so to short-circuit
				//the compass needle in som_hacks_DrawPrimitive
				//(when exiting a top-level menu in such a way)
				som_hacks_tint = 0;
				
				if(DDRAW::fxCounterMotion)
				{
					DDRAW::fxCounterMotion = 0; //maximum strength
					DDRAW::dejagrate_update_psPresentState();
				}
									
				if(som_hacks_early_disable_MRT) //DEBUGGING
				{
					//2021: disabling MRT because I worry it interferes
					//with the nanosvg (Exselector) OpenGL text shaders
					//NOTE: som_hacks_Clear must restore prior to clear
					if(DDRAW::doClearMRT) //190: D3DRS_COLORWRITEENABLE1
					DDRAW::Direct3DDevice7->SetRenderState((DX::D3DRENDERSTATETYPE)190,0);
				}

				if(!som_hacks_menuxform&&DDRAW::xr) //TESTING
				{
					//auto *w = (DX::D3DMATRIX*)SOM::menuxform;
					auto *w = (DX::D3DMATRIX*)SOM::stereo344[0];
					DDRAW::Direct3DDevice7->SetTransform(DX::D3DTRANSFORMSTATE_WORLD,w);
				}
			}

			int hud = x->left<cx;

			//NOTE: this used to be done when the gauges are drawn
			//HACK: this also draws the subtitles, should fix that
			if(!hud) som_hacks_shadow_tower();

			EX::INI::Option op;

			if(op->do_st) //Shadow Tower
			{
				goto short_circuit;
			}

			if(som_hacks_displays_a[!hud]) //testing
			{
				//Blt doesn't support this. dx.d3d9c.cpp picks up on it
				w|=DDBLT_ALPHASRCCONSTOVERRIDE;
				q->dwAlphaDestConstBitDepth = 8;
				q->dwAlphaSrcConst = 128;
			}
						
			if(som_hacks_kf2_gauges.left)
			{
				if(hud&&op->do_kf2) goto short_circuit;
			}	

			//King's Field II mode?
			if(!hud) if(op->do_kf2||op->do_nwse) //EXPERIMENTAL
			{
				bool drawing = SOM::showGauge&&op->do_kf2;
				switch((DWORD)SOM::menupcs)
				{
				case 0x19AAA20: case 0x1d11ca8: 
					
					//there's a conflict with the menu system
					//drawing = SOM::fov[0]/SOM::fov[1]>=1.6f;
					drawing = false; 
				}
				if(DDRAW::xr) //text becomes opaque at alpha 0?
				{
					if(som_hacks_hud_fade[0]<=0.0f)
					drawing = false;
				}
				if(drawing)
				{
					//if the compass isn't on screen alpha is turned off
					extern void som_scene_alphaenable(int);
					som_scene_alphaenable(1);
				}

				//2022: for some reason menupcs no longer matches when
				//picking up items so the frame texture isn't detected
				auto swapcs = SOM::menupcs;
				SOM::menupcs = (DWORD*)0x1A5B4A8; //looks hardcoded...

				//HACK: trick som_hacks_DrawPrimitive to think it's in
				//a menu
				//NOTE: tint also triggers SomEx_output_OpenXR_font_callback
				som_hacks_tint = SOM::frame;
				auto men = (float*)0x1a5b400; if(drawing) //i.e. menupcs
				{
					auto &init = (DWORD&)men[42];
					extern BYTE som_game_4212D0(FLOAT*);
					if(!init||init==0xFFFFFFFF)
					som_game_4212D0(men);
					else if(SOM::altf&1<<2&&EX::stereo_font) //VR switch?
					{
						extern void som_game_menucoords_set(bool reset);
						SOM::menupcs = (DWORD*)men+0xA8/4;
						som_game_menucoords_set(true);
					}
				}
				float &a = men[0x28], &b = men[0x29], aa = a, bb = b;
				a = 1.6; b = 1.6; //matches frameG1.bmp
				extern float som_game_menucoords[2];
				float &c = som_game_menucoords[0], &d = som_game_menucoords[1], cc = c, dd = d;				
				c = 3; d = 2.5f;
				{
					EX::INI::Script sc; EX::INI::Adjust ad;
					
					DWORD gb = 0xFFFF&ad->hud_gauge_border;

					RECT box = {}; UINT how;
					const wchar_t *mp = SOM::Status(&box,&how,'pts',1);
					//memset(&box,0x00,sizeof(box));
					const wchar_t *hp = SOM::Status(&box,&how,'pts',0);

					//this is selecting padding appropriate to the frames
					//and font-size too
					int m = box.left;
					//m/2 needs to clear the little decal but just barely
					//int pad = m/2+20-m;
					int pad = m*2/3+20-m;
					OffsetRect(&box,pad,pad*2/3); box.right+=pad;
					
					box.bottom+=2*(gb>>8)-2*(gb&0xff);
					
					int xoff = 0, yoff = 0; 
					
					//wart makes room for the little decal
					//int wart[2] = {xoff+26,yoff+18}; //80x80 square
					int wart[2] = {26,18}; //80x80 square					

					if(DDRAW::inStereo)
					{
						//REMINDER: som.mocap.cpp HAD to compensate
						//for this (OpenXR)

						if(DDRAW::xr) //EXPERIMENTAL (som.mocap.cpp)
						{
							xoff = SOM::width*0.5f-(wart[0]+box.right)/2;
							yoff = SOM::height*0.5f-(wart[1]+box.bottom); //2???	
						}
						else
						{						
							//NOTE: a little higher so compass doesn't touch
							//top of menus
							//xoff = SOM::width*0.26f; yoff = SOM::height*0.03f;	
							//seems to work better in the middle of the screen
							xoff = SOM::width*0.33f; yoff = SOM::height*0.29f;	
						}
						wart[0]+=xoff; wart[1]+=yoff;
					}

					//assuming SOM::Status margins clear the frame?
					//frames are 20x20
					//guages are 110x20
					//int w = 110+box.left*2;
					int w = box.right;
					int h = (15+box.bottom)*2; //20

					w = w/a+0.5f; h = h/a+0.5f;

					w-=24; h-=24; //borders are added on
					
					if(drawing)
					{
						//som_scene_state::push(); //TESTING

						int x = (int)(wart[0]/a+0.5f);
						int y = (int)(wart[1]/a+0.5f);

						x+=12; y+=12; //?? //would like to factor this out

						int layout[6] = {x,y,w,h,0,0}; 

						unsigned hack = SOM::frame;
						std::swap(hack,SOM::doubleblack);
						for(int i=0;i<2;i++,layout[4]=3)
						((void(__cdecl*)(int*,void*,DWORD))0x421F10)(layout,men,men[0x70]);						
						std::swap(hack,SOM::doubleblack);

						//som_scene_state::pop(); //TESTING
					}

					if(DDRAW::xr) c+=4; //FUDGE???

					OffsetRect(&box,c+wart[0]-1,d+wart[1]-1); 
					box.bottom-=wart[1]-yoff;
					
					//HACK: using Start intead of Print because the text is displayed over
					//a panel like in the menu				
					DWORD c1 = sc->system_fonts_contrast; 
					if(c1==0xff464646) 
					c1 = ad->lettering_contrast;	
					DWORD c2 = sc->system_fonts_tint; 				
					if(c2==0xffdcdcdc) 
					c2 = ad->lettering_tint;
					if(float l=ad->hud_label_translucency)
					{
						//TODO? might want to do this for do_st
						((BYTE*)&c1)[3]*=1-l;
						((BYTE*)&c2)[3]*=1-l;
					}
					if(DDRAW::xr)
					{
						float a = som_hacks_hud_fade[0];
						//can't let go to 0
						auto &a1 = ((BYTE*)&c1)[3];
						auto &a2 = ((BYTE*)&c2)[3];
						if(!a1) a1 = 0xff;
						if(!a2) a2 = 0xff;
						a1 = max(1,a1*a); 
						a2 = max(1,a2*a);
					}
					if(drawing)
					{
						OffsetRect(&box,1,1);
						SOM::Start(hp,c1,&box,DT_SINGLELINE);
						OffsetRect(&box,-1,-1);
						SOM::Start(hp,c2,&box,DT_SINGLELINE);
					}
					OffsetRect(&box,0,box.bottom-box.top+31);
					if(drawing)
					{
						OffsetRect(&box,1,1);
						SOM::Start(mp,c1,&box,DT_SINGLELINE);
						OffsetRect(&box,-1,-1);
						SOM::Start(mp,c2,&box,DT_SINGLELINE);
					}

					//HACK
					//the following fudges had som_hacks_
					//built in somehow... I don't really
					//understand it since the box itself
					//is position correctly
					OffsetRect(&box,som_hacks_,som_hacks_);
					//box.top-=17; box.bottom+=14;
					//box.left-=4; box.right-=pad+m+2;		
					box.top-=19; box.bottom+=12;
					box.left-=6; box.right-=pad+m+4;
					som_hacks_kf2_gauges = box;
				}
				a = aa; b = bb; c = cc; d = dd;
				som_hacks_tint = 0;
			
				if(!drawing)
				som_hacks_kf2_gauges.right = som_hacks_kf2_gauges.left-2;				

				//HACK: fix Ex_output_onflip... this is safer than messing with the zbuffer state
				DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_ZFUNC,DX::D3DCMP_LESSEQUAL);

				SOM::menupcs = swapcs;

				if(op->do_nwse) goto short_circuit;				
			}
			
			//if(0==SOM::showGauge) //forcing on
			{
				//gauges are on the left
				if(!(hud?SOM::showGauge:SOM::showCompass)) 
				{
					goto short_circuit;
				}
			}		

			if(!fan&&som_hacks_) //adjust on screen elements 
			{
				x->top+=som_hacks_; x->bottom+=som_hacks_;
				x->left+=x->left<cx?som_hacks_:-som_hacks_;				
				x->right+=x->left<cx?som_hacks_:-som_hacks_;				
			}
		}
	}

	static bool lit = false; //lighting state is unreliable for blits 

	if(!hr&&DDRAW::isLit) 
	{			
		DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_LIGHTING,0); 

		lit = true;
	}
	else if(hr&&lit)
	{
		DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_LIGHTING,1); 

		lit = false;
	}

	if(fan) //2022: OpenXR needs polygons
	{
		float l,r,t,b; if(!z)
		{
			l = t = 0; r = b = 1;
		}
		else
		{
			l = z->left; r = z->right;	
			t = z->top; b = z->bottom;
			
			l/=yy->queryX->width; r/=yy->queryX->width;
			t/=yy->queryX->height; b/=yy->queryX->height;
		}

		DWORD c = 0xFFFFFFFF; 
		if(w&DDBLT_ALPHASRCCONSTOVERRIDE) //UNUSED?
		c = 0x80FFFFFF;
		
		float prims[4*8] =
		{
			x->left,x->top,0,1,*(float*)&c,0,l,t,
			x->right,x->top,0,1,*(float*)&c,0,r,t,
			x->right,x->bottom,0,1,*(float*)&c,0,r,b,
			x->left,x->bottom,0,1,*(float*)&c,0,l,b,
		};

		som_hacks_Blt_fan = fan; //HACK
		
		DDRAW::Direct3DDevice7->SetTexture(0,y);
		DDRAW::Direct3DDevice7->DrawPrimitive(DX::D3DPT_TRIANGLEFAN,D3DFVF_TLVERTEX,prims,4,0);

		som_hacks_Blt_fan = false; //HACK

		goto short_circuit;
	}

	if(som_hacks_shader_model) 
	{
		DDRAW::vs = DDRAW::ps = 1; //blit
	}
	
	return 0;
}

static void *som_hacks_Flip(HRESULT*hr,DDRAW::IDirectDrawSurface7*in, DX::LPDIRECTDRAWSURFACE7&,DWORD&)
{
	/*TODO? add to output
	#ifdef _DEBUG
	static unsigned was;
	if(!hr) was = EX::tick(); if(hr) 	
	{
		static unsigned wait, report = 0;
		if(report<SOM::frame-60*3)
		{	
			wait = EX::tick()-was; 			
			if(wait>50) report = SOM::frame;
		}
		EX::dbgmsg("present: %3d (%d)",wait);
	}
	#endif*/
	if(hr) return 0;
			 
	if(som_hacks_pcstated) //sprinkling around
	{
		som_hacks_pcstated = false;
		memcpy(SOM::L.pcstate,som_hacks_pcstate,sizeof(float)*6);
	}

	if(!hr) //contexts
	if(!SOM::paused&&!SOM::recording) //NEW
	{
		SOM::player = true;

		static int prior = 0;
		
		//2017: This may be out of sync with the input model.
		//som.state.cpp is now changing context in advance of
		//this older approach since having item pick up issues. 

		extern bool som_state_reading_menu;
		extern bool som_state_reading_text;

		//HACK: Trying to recall the load/save game highlight
		//upon reentering the continue and save event screens.
		bool extramenu = false;

		if(som_state_reading_text)
		{
			SOM::context = SOM::Context::reading_text;
		}
		else if(som_state_reading_menu)
		{
			SOM::context = SOM::Context::reading_menu;
		}
		else if(EX::is_playing_movie()) //hack
		{
			//2021: THIS DOESN'T WORK UNDER DIRECT3D 9!
			SOM::context = SOM::Context::playing_movie;		
			extramenu = true;
		}
		else if(som_hacks_tint==SOM::frame)
		{	
			//NEW: true inside Save event loop			
			extern bool som_state_saving_game; 						
			bool saving = SOM::saving==SOM::frame;
			if(!som_state_saving_game) //NEW
			{			
				if(saving) //obsolete/erroneous way
				if(prior!=SOM::Context::saving_game
				 &&prior!=SOM::Context::playing_game)
				{				
					saving = false; //want event save only
				}
			}
			else saving = true;

			if(saving)
			{
				SOM::context = SOM::Context::saving_game;
			}
			else if(SOM::Game.browsing_shop)
			{
				SOM::context = SOM::Context::browsing_shop;
			}			
			else if(SOM::Game.taking_item
			||SOM::taking==SOM::frame) //obsolete
			{
				SOM::context = SOM::Context::taking_item;
			}
			else if(SOM::Game.browsing_menu)
			{
				SOM::context = SOM::Context::browsing_menu;
			}
			else if(SOM::dialog==SOM::frame)
			{
				SOM::context = SOM::Context::reading_text;
			}
			else SOM::context = SOM::Context::browsing_menu;
		}
		else if(SOM::L.startup!=-1) //NEW
		{
			SOM::context = SOM::L.startup<2?
			SOM::Context::viewing_picture:SOM::Context::browsing_menu; 
			switch(SOM::L.startup)
			{
			//4~8 are Load Game also
			//0042C0A8 FF 14 95 54 E5 45 00 call        dword ptr [edx*4+45E554h] 
			case 4: case 5: case 7: case 8: assert(0);
			case 9:	break; //Load Game
			default: extramenu = true;
			}
		}
		else if(som_hacks_blit==SOM::frame)
		{
			SOM::context = SOM::Context::viewing_picture;
			extramenu = SOM::field;
		}
		else if(!SOM::field) //continue screen
		{
			//2022: Moratheia is hitting this today in 
			//entering the intro??? it wasn't earlier
			
			//assert(0); //obsolete, via SOM::L.startup
			//SOM::context = SOM::Context::browsing_menu; 
		}
		else //TODO: consider other kinds of paralysis
		{
			SOM::player = 
			SOM::play&&som_hacks_fade!=SOM::frame;			
			SOM::context = SOM::Context::playing_game;
			extramenu = true;
		}

		if(SOM::context) prior = SOM::context;

		if(extramenu)
		{			
			//HACK: -1 signals leaving event pseudo-menus
			extern void som_game_highlight_save_game(int,bool);
			som_game_highlight_save_game(-1,0);
		}
	}

	EX::INI::Bugfix bf; EX::INI::Option op;

	if(bf->do_fix_asynchronous_sound)
	{				
		static bool sync = false;  
		if(som_hacks_tint!=SOM::frame)
		{					
			if(sync&&DSOUND::noStops==1)
			{
				sync = false; DSOUND::Play(op->do_syncbgm);
			}
		}
		else if(som_hacks_tint==SOM::frame)
		{	
			if(!sync&&DSOUND::noStops==0)
			{
				sync = true; DSOUND::Stop(op->do_syncbgm);
			}
		}
	}
	DSOUND::playing_delays();

	//if(som_hacks_shader_model) DDRAW::fx = 16; 
	assert(16==DDRAW::fx);	

	//HACK: just routinely flushing the system 
	//brightness shader constant tends to get lost in 32bpp mode
	//thoughts: this really should be looked into
	//notes: probably the effects blt prevents this in 16bpp mode
	DDRAW::refresh_state_dependent_shader_constants(); 

	return som_hacks_Flip;
}

static void som_hacks_shadowfog(DDRAW::IDirectDrawSurface7*);
static void *som_hacks_SetColorKey(HRESULT *hr, DDRAW::IDirectDrawSurface7 *p, DWORD &x, DX::LPDDCOLORKEY&y)
{		
	assert(DDRAW::compat=='dx9c'); //REMINDER

	if(!hr)
	{
		//color blended title screen element?
		//HACK: Ex_mipmap_antialias32 is setting the black
		//pixels to their neighbor's color, so it's necessary
		//to not feed them back through, and it's also pointless
		if(p->queryX->colorkey2)
		{
			//2021: this is to find out if SOM ever actually 
			//does this, but either way it might be required
			//even though this prevents SetColorKey from being
			//called twice (unless it's manually unset first)
			assert(0);

			if(y) x = 0; //prevent regeneration!!
		}
		else if(p==*(void**)0x1D3D2F0) //kage.mdl?
		{
			if(EX::INI::Option()->do_alphafog)
			{
				x = 0; som_hacks_shadowfog(p);
			}			
		}

		return x?som_hacks_SetColorKey:0;
	}	
		//REMOVE ME
		//I'm unlikely to implement this for OpenXR
		//since I've been meaning to remove [Keygen]
		//for a very long time

	//concerned with textures only for now
	//if(p->target!='dx9c'||!p->query9->group9) return 0; 

	//D3D9 primary surfaces don't have interface pointers
	//if(DDRAW::is_primary<DDRAW::IDirectDrawSurface7>(p))
	if(p->query9->isVirtual) //2021
	{
		return 0; //pass on pseudo surfaces
	}

	//REMOVE ME
	//if(som_hacks_keygen) //TODO: disable somehow
	{					   
		/*2021

			this came in conflict with deferring
			dx_d3d9c_mipmap/colorkey... I've been
			meaning to remove it for a long time

		const SOM::KEY::Image *q = 0;
		if(!EX::DATA::Get(q,p->query->clientdata,false))
		{
			q = SOM::KEY::image(p->query9->group9);			
			if(q&&EX::DATA::Set(q,p->query->clientdata))
			{
				if(q->correct) som_hacks_correct(p,q);

				p->query->clientdtor = EX::DATA::Nul;
			}
		}*/
	}

	return 0;
}

/*I think maybe this is unnecessary after updating the PSVR firmware
//for the first time
static void som_hacks_PSVR_dim() //EXPERIMENTAL
{
	if(!DDRAW::inStereo) return;
	EX::INI::Stereo vr;
	if(&vr->dim_adjustment)
	DDRAW::dimmer = vr->dim_adjustment(DDRAW::dimmer,0.0f); //0 is PSVR
	else DDRAW::dimmer-=4; 
}*/ 
static void *som_hacks_SetGammaRamp(HRESULT *hr, DDRAW::IDirectDrawGammaControl*, DWORD&,DX::LPDDGAMMARAMP &y)
{
	if(!hr) return som_hacks_SetGammaRamp;

/*	static int log = 0;

	if(log++<2) return 0;

	EXLOG_LEVEL(-1) << "\n\n\n\n\n\n\ngamma log (#" << log-3 << ")\n";

	for(int i=0;i<255;i++)
	{
		EXLOG_LEVEL(-1) << (int)y->red[i];

		if(y->green[i]!=y->red[i]||y->blue[i]!=y->red[i])
		{
			EXLOG_LEVEL(-1) << '(' << (int)y->green[i] << '/' << (int)y->blue[i] <<')';
		}

		EXLOG_LEVEL(-1) << '\n';
	}
*/
	//2018: 8~16 looks like a gamma ramp with increasing power
	//0~7 is bowl shaped, kind of how I imagine King's Field II 
	//(is this similar to tone-mapping?)
	/*if(0&&EX::debug) 
	{
		short test[255]; assert(y);
		for(int i=0;i<255;i++) test[i] = y->red[i]-y->red[i-1];
		int bp = 0; //breakpoint
	}*/

	switch(y?y->red[1]:0)
	{
	case 2:   SOM::gamma =  0; break;
	case 50:  SOM::gamma =  1; break;
	case 98:  SOM::gamma =  2; break;
	case 145: SOM::gamma =  3; break;
	case 193: SOM::gamma =  4; break;
	case 241: SOM::gamma =  5; break;
	case 288: SOM::gamma =  6; break;
	case 336: SOM::gamma =  7; break;
	case 383: SOM::gamma =  8; break;
	case 431: SOM::gamma =  9; break;
	case 479: SOM::gamma = 10; break;
	case 526: SOM::gamma = 11; break;
	case 574: SOM::gamma = 12; break;
	case 622: SOM::gamma = 13; break;
	case 669: SOM::gamma = 14; break;
	case 717: SOM::gamma = 15; break;
	case 765: SOM::gamma = 16; break;

	default: SOM::gamma = 8;
	}

	//ANCIENT DAYS
	//
	// this isn't really coherent since DirectDrawCreateEx
	// now has logic to fall back on fixed-function if the
	// chosen device doesn't have shader model 2 or better
	//
	//REMINDER: this seems to mirror intialization of 
	//DDRAW::brights in kickoff_somhacks_thread. it's
	//more like SOM::shader overrides do_gamma I guess
	EX::INI::Option op;	
	if(!DDRAW::shader ////2018: new gamma?
	||!EX::INI::Option()->do_gamma) //ancient gamma mode?
	{
		if(SOM::gamma>8)
		{
			DDRAW::dimmer = 0; DDRAW::bright = SOM::gamma-8; 
		}
		else if(SOM::gamma<8)
		{
			DDRAW::bright = 0; DDRAW::dimmer = 8-SOM::gamma;
		}
		else DDRAW::bright = DDRAW::dimmer = 0;

	//	som_hacks_PSVR_dim(); 

		DDRAW::Direct3DDevice7-> //reset/level fogcolor
		SetRenderState(DX::D3DRENDERSTATE_FOGCOLOR,som_hacks_fogcolor);	
	}

	return 0; 
}

static void *som_hacks_CreateDevice(HRESULT *hr, DDRAW::IDirect3D7*p, const GUID*&, DX::LPDIRECTDRAWSURFACE7&, DX::LPDIRECT3DDEVICE7*&z)
{
	EX::INI::Window wn;	EX::INI::Option op;	EX::INI::Bugfix bf;
	
	if(hr&&*hr==D3D_OK) //defaults
	{
		//install D3D11 effects shaders?
		som_hacks_d3d11::try_DDRAW_fxWGL_NV_DX_interop2();

		som_hacks_texture_addressu = 1; //D3DTADDRESS_WRAP
		som_hacks_texture_addressv = 1; //D3DTADDRESS_WRAP

	//	som_hacks_key_image = 0;

		int clear = EX::INI::Window()->do_not_clear?1:2; //2021

		//do_alphafog
		DWORD cwe1 = 0; //D3DRS_COLORWRITEENABLE1
		DDRAW::Direct3DDevice7->GetRenderState((DX::D3DRENDERSTATETYPE)190,&cwe1);
		DDRAW::doClearMRT = cwe1?clear:0; 
		if(som_hacks_early_disable_MRT) //2021
		{		
			DDRAW::Direct3DDevice7->SetRenderState((DX::D3DRENDERSTATETYPE)190,0);
		}
	}

	if(hr) if(som_hacks_shader_model)
	{
		switch(DDRAW::target) //2021
		{
		case 'dxGL': EX::shadermodel = L"OpenGL 4.5"; break;
		case 'dx95': EX::shadermodel = L"OpenGL ES 3.1"; break;
		case 'dx9c': default:
		assert('dx9c'==DDRAW::target);
		switch(som_hacks_shader_model)
		{
	//	case 1: EX::shadermodel = L"Shader Model 1.1"; break;
		case 2: EX::shadermodel = L"Shader Model 2.0"; break;
		case 3: EX::shadermodel = L"Shader Model 3.0"; break;

		default: EX::shadermodel = L"unrecognized shader mode";
		}}
	}
	else if(!DDRAW::ff) switch(DDRAW::shader)
	{
	default: EX::shadermodel = L"unrecognized shader mode";

	case 'ps': case 0:
		
		EX::shadermodel = L"pixel shader enabled"; break;
	}
	else EX::shadermodel = L"fixed function mode";
		
	if(som_hacks_lamps)
	{
		if(!som_hacks_lightfix)
		{
			som_hacks_lightfix = new DX::D3DLIGHT7[SOM_HACKS_LIGHTS];
			memset(som_hacks_lightfix,0x00,sizeof(DX::D3DLIGHT7)*SOM_HACKS_LIGHTS);

			som_hacks_lightlog = new DX::D3DLIGHT7[SOM_HACKS_LIGHTS];
			memset(som_hacks_lightlog,0x00,sizeof(DX::D3DLIGHT7)*SOM_HACKS_LIGHTS);

			som_hacks_lightfix_on = new bool[SOM_HACKS_LIGHTS];		 
			memset(som_hacks_lightfix_on,0x00,sizeof(bool)*SOM_HACKS_LIGHTS);

			som_hacks_lamps_pool = new DWORD[SOM_HACKS_LIGHTS];		 
			memset(som_hacks_lamps_switch,0x00,sizeof(som_hacks_lamps_switch));

			DDRAW::lightlost = som_hacks_lightlost;

			DDRAW::light = som_hacks_light;

			*som_hacks_lamps_pool = 0;
		}

		if(hr) 		
		if(som_hacks_lights_detected_map_close) //recover from false alarm
		{
			DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_SETLIGHT_HACK,0);
			DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_LIGHTENABLE_HACK,0);

			for(int i=3;i<SOM_HACKS_LIGHTS;i++) 
			if(som_hacks_lightfix[i].dltType) 
			DDRAW::Direct3DDevice7->SetLight(i,som_hacks_lightfix+i);
			for(int i=3;i<SOM_HACKS_LIGHTS;i++) 
			if(som_hacks_lightfix_on[i])
			DDRAW::Direct3DDevice7->LightEnable(i,true);

			DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_LIGHTENABLE_HACK,som_hacks_LightEnable);
			DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_SETLIGHT_HACK,som_hacks_SetLight);

			som_hacks_lights_detected_map_close = false;
		}
	}

	return som_hacks_CreateDevice;
}
 
extern float SOM::reset_projection(float zoom2, float zf)
{					
	//zoom2 is being set to 50 for the item view, otherwise the item
	//doesn't center. For VR 50 doesn't work, so the model must move
	//(a way to control the model would be useful/good way to do it)
	if(DDRAW::inStereo) zoom2 = SOM::zoom2();
	
	D3DXMATRIX proj; float zoom = M_PI/180*zoom2;
	float aspect2 = DDRAW::inStereo?1920/1080.0f:SOM::fov[0]/SOM::fov[1];

	float yScale = 1/tan(zoom/2);
	float xScale = yScale/aspect2; //division???
	float zn = SOM::fov[2];
	//float zf = SOM::fov[3]; //or SOM::skyend?
	float r=zn/xScale;
	float l=-r;
	float t=zn/yScale;
	float b=-t;

	/*UNUSED

		//NO CLUE FOR SOM::ipd_center
		//maybe this is the "virtual screen" that is different
		//for different products
		//https://web.archive.org/web/20170919095247/http://www.orthostereo.com:80/geometryopengl.html
		float screen = 1.5f; //zn~zf

	//float vr = DDRAW::stereo*zn/screen; //skew for VR	
	float vr = DDRAW::stereo; 
	//Reddit discussion suggests skewing 
	//from the center of the eyes...
	//but by how much/in what direction?
	//https://www.reddit.com/r/vrdev/comments/a418sy/skew_matrix_beneficial_or_detrimental_projection/
	if(SOM::ipd) 
	vr-=SOM::ipd_center();		
	vr*=zn/screen;

	if(DDRAW::inStereo)
	{	
		if(0&&EX::debug)
		{
			assert(SOMEX_VNUMBER==0x1020209UL);

			//computing asymmetric matrix
			//https://web.archive.org/web/20170924184212/http://doc-ok.org/?p=77
			//stereo2 is equal to D3DXMATRIX._31			
			//it should be positive for the right eye			
			float stereo2 = vr*2/(r-l); //(l+r)/0.381286681			
			if(stereo2!=DDRAW::stereo2) //map change?
			{
				//not appropriate with SOM::ipd_center
				//assert(stereo2>=0);
				DDRAW::stereo2 = stereo2; 
				DDRAW::stereo_update_SetStreamSource();
			}
		}
	
		//testing equivalence
		//if(SOM::frame%1&&EX::debug) 
		//D3DXMatrixPerspectiveOffCenterLH(&proj,l,r,b,t,zn,zf);
		//else goto testing2;
	}*/
	//else testing2:
	{	
		//2021: I thought I made a mistake to not pass in the far plane
		//(I had removed some code that did) however it really seems not
		//to matter as far as z-fighting is concerned. I'm not sure why
		//that should be... other than the coordinates are nonlinear
		//IOW (at least for KF2's draw distance) SOM::skyend is just as
		//good as passing in a short far plane for the auxiliary planes

		D3DXMatrixPerspectiveFovLH(&proj,zoom,aspect2,SOM::fov[2],zf); //SOM::skyend
	}
	DDRAW::Direct3DDevice7->SetTransform(DX::D3DTRANSFORMSTATE_PROJECTION,(DX::D3DMATRIX*)&proj);	

	/*//debugging, comparing matrixes	
	#if 0 && SOMEX_VNUMBER==0x1020209UL && defined(_DEBUG)
	{	
		D3DXMATRIX left,right;	
		D3DXMatrixPerspectiveOffCenterLH(&left,l+vr,r+vr,b,t,zn,zf);
		D3DXMatrixPerspectiveOffCenterLH(&right,l-vr,r-vr,b,t,zn,zf);
		b = b; //breakpoint
	}
	#endif*/

	return zoom2; //stereo?
}
extern float som_hacks_skyconstants[4]={}; //som.scene.cpp
extern float som_hacks_volconstants[4]={}; //2023
static void *som_hacks_BeginScene(HRESULT*,DDRAW::IDirect3DDevice7*)
{	
	if(!SOM::field) return 0; //2022

	int todolist[SOMEX_VNUMBER<=0x1020406UL];
	/*2022: this is causing a glitch on the first frame after 
	//changing maps???
	//NOTE: som_hacks_onflip had some code to ignore the first
	//frame(s) because they were glitched out
	static unsigned one_off = 0;
	if(one_off==SOM::frame) return 0;	
	one_off = SOM::frame;*/
	//EX::dbgmsg("begin? %d",SOM::frame); //should print more than once
//	EX::dbgmsg("begin?"); //should print more than once

	//simulate sky at top of scene (in case late)
	{
		DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_LIGHTENABLE_HACK,0);
		DDRAW::killing_lights(som_hacks_lightfix_on); 
		DDRAW::reviving_lights(som_hacks_lightfix_on); 			
		DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_LIGHTENABLE_HACK,som_hacks_LightEnable);
		som_hacks_ditch_lamps(); 
	}
	
	EX::INI::Adjust ad;
	EX::INI::Option op; EX::INI::Detail dt;

	//2022: fog blending logic
	float t2; if(t2=SOM::mpx2_blend)
	{
		auto &defs = SOM::mpx_defs(SOM::mpx);

		if(0) //time based?
		{
			if(SOM::motions.frame==SOM::frame)
			t2-=0.5f/DDRAW::refreshrate;
		}
		else //distance based?
		{
			//som.MPX.cpp overwrites the starting
			//point with a transfer landing point
			//for this purpose
			som_MPX &mpx = *SOM::L.mpx->pointer;
			float x = SOM::xyz[0]-mpx.f[121];
			float z = SOM::xyz[2]-mpx.f[123];
			float d = sqrtf(x*x+z*z)-1;

			//ARBITRARY
			//SOM_MAP defaults to 30. I fear that
			//if the sky doesn't transition until
			//deep inside the map it may ruin the
			//map's character
			enum{ zmost=30 };

			//EXTENSION
			//min is mainly to avoid asymmetries 
			//in going in and out of a threshold
			//NOTE: asymmetries wouldn't be able
			//to be detected as a visible glitch
			//(they would be temporal in nature)
			float zf2 = SOM::mpx_defs(SOM::mpx2).zfar;
			zf2 = min(zf2,defs.zfar);
			d*=1/min(zf2,zmost);

			//NOTE: EX::Affects[1] is used to 
			//bias the circle when using the
			//new relative transfer option
			t2 = 1-max(d,0)-SOM::mpx2_blend_start;			
		}
		if(t2<=0)
		{
			t2 = 0;

			if(!defs.sky) SOM::sky = false;
		}
		SOM::mpx2_blend = t2;		
	}
	auto &dst = *SOM::L.corridor;
	int n = dst.fade[0]==17&&t2?2:1; //blend fog?
	//float fog = 0;	
	DWORD fogcolor[2];	
	SOM::fov[3] = 0;
	SOM::fogend = SOM::fogstart = 0;
	SOM::skyend = SOM::skystart = 0;
	float skyfloor[2];
	float skyflood[2];
	float fogmin[2] = {}; for(int i=n;i-->0;)	
	{
		auto &defs = SOM::mpx_defs(i?SOM::mpx2:SOM::mpx);

		//only running extensions on one map at a time so events
		//can manage their own counters
		if(i==0) 
		{			
			t2 = 1-t2; //!

			//HACK: sky_instance lags behind sky and I'm not sure
			//I want to commit to passing sky to extensions as is
			//done below with skyfloor/skyflood but I need it for
			//my tests/project
			int sky = defs.sky;

			//43:draw distance //44:fog start (as percentage of 43)
			//SOM subtracts 2 from the draw distance for the fogend
			float fog_1 = defs.zfar-2; //SOM::fog[1]
			float fog_0 = defs.fog*fog_1; //SOM::fog[0]
			float min_0 = {ad->fov_sky_and_fog_minima};
			float min_1 = &ad->fov_sky_and_fog_minima2?ad->fov_sky_and_fog_minima2:min_0;
			float fogend = fog_1; ad->fov_fogline_adjustment2(&fogend);	
			float fogstart = fog_0; ad->fov_fogline_adjustment(&fogstart);	
			if(fogstart>fogend) fogend = fogstart;
			float skyend = sky?defs.zfar:fogend; //SOM::sky
			float skystart = skyend;
			if(sky) //SOM::sky
			{
				ad->fov_skyline_adjustment2(&skyend); 	
				ad->fov_skyline_adjustment(&skystart);
				if(skystart>skyend) skyend = skystart;
			}
			
			if(sky&&op->do_alphafog) //SOM::sky
			{
				//HACK: I haven't decided if to pass "sky" to Adjust
				//extensions, but these are Detail extensions and it
				//seems like these should be properties of the model
				defs._skyfloor = dt->alphafog_skyfloor_constant(sky);
				defs._skyflood = dt->alphafog_skyflood_constant(sky);
			}
			else defs._skyflood = 0;

			defs._fogmin[0] = min_0; 
			defs._fogmin[1] = min_1; 

			defs._fogstart = fogstart; defs._fogend = fogend;
			defs._skystart = skystart; defs._skyend = skyend;

			if(&ad->fov_fogline_saturation)
			defs._fogcolor = ad->fov_fogline_saturation;
			else defs._fogcolor = defs.bg;
		}
		fogcolor[i] = defs._fogcolor;
	
		fogmin[0]+=t2*defs._fogmin[0];
		fogmin[1]+=t2*defs._fogmin[1];

		SOM::fov[3]+=defs.zfar*t2; //UNUSED (prefer skyend?)

		SOM::fogend+=defs._fogend*t2; SOM::fogstart+=defs._fogstart*t2;
		SOM::skyend+=defs._skyend*t2; SOM::skystart+=defs._skystart*t2;

		skyfloor[i] = defs._skyfloor;
		skyflood[i] = defs._skyflood;

		//fog+=defs.fog*t2; //fogstart? //D3DRENDERSTATE_FOGSTART?
	}
	if(n==2) 
	{
		float tb = SOM::mpx2_blend;
		if(skyflood[0]>0&&skyflood[1]>0)
		{
			skyflood[0] = skyflood[0]*t2+skyflood[1]*tb;
			skyfloor[0] = skyfloor[0]*t2+skyfloor[1]*tb;
		}
		else if(skyflood[1]>0)
		{
			skyflood[0] = skyflood[1]; skyfloor[0] = skyfloor[1];
		}
		
		BYTE *a = (BYTE*)&fogcolor[0];
		BYTE *b = (BYTE*)&fogcolor[1];		
		for(int i=3;i-->0;)
		a[i] = (BYTE)(t2*a[i]+tb*b[i]);
	}
	SOM::clear = fogcolor[0]|0xff<<24; 
	//TODO? update som_db's variables?
	//((DWORD(__cdecl*)(float,float,float))0x403ac0)(0.8726646f,map_znear,x->zfar);
	//((BYTE(__cdecl*)(float,DWORD))0x4115f0)(fog,fogcolor[0]);
	DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_FOGCOLOR,fogcolor[0]);

	if(SOM::fogend>SOM::skyend) SOM::fogend = SOM::skyend;
	if(SOM::fogend-SOM::fogstart<SOM::fogend*fogmin[1]) 
	SOM::fogstart = SOM::fogend-SOM::fogend*fogmin[1];	
	DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_FOGSTART,(DWORD&)SOM::fogstart);
	DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_FOGEND,(DWORD&)SOM::fogend);		
	SOM::L.frustum[1] = SOM::skyend; 
	float c[4] = {FLT_MAX,SOM::skyend,-100,1}; 	
	if(SOM::sky&&op->do_alphafog)
	{		
		if(SOM::skyend-SOM::skystart<SOM::skyend*fogmin[0])
		SOM::skystart = SOM::skyend-SOM::skyend*fogmin[0];			

		c[0] = 1/(SOM::skyend-SOM::skystart); c[1] = SOM::skyend; 

		if(skyflood[0]>0){ c[2] = skyfloor[0]; c[3] = skyflood[0]; }
	}	
	//1: z is marking NPCs in the pixel shaders
	DDRAW::vset9(c); c[2] = 1; DDRAW::pset9(c);			
	memcpy(som_hacks_skyconstants,c,sizeof(c)); 	

	//NEW: update for zoom/sky extensions
	SOM::reset_projection();

	return 0;
}
static void *som_hacks_EndScene(HRESULT*,DDRAW::IDirect3DDevice7 *in)
{		
	if(!DDRAW::inScene)	if(in) 
	{
		//avoid buggy frame while changing options such as resolution
		in->BeginScene(); in->Clear(0,0,D3DCLEAR_TARGET,0x00000000,0,0);
	}
	return 0;										  
}

static void som_hacks_clear_letterbox(bool onflip)
{
	RECT box = {DDRAW::xyMapping[0],DDRAW::xyMapping[1]}; 

	box.right = box.left+SOM::fov[0]; box.bottom = box.top+SOM::fov[1];	

	//XP: frame is dirty
	InflateRect(&box,DDRAW::fxInflateRect,DDRAW::fxInflateRect); 

	RECT client; 
	if(!DDRAW::window_rect(&client)||EqualRect(&box,&client))
	return; 

	DDRAW::doNothing = true; //// POINT OF NO RETURN ////

	//NOTE: doSuperSampling overrides doNothing
	//and OpenGL depends on doSuperSampling to
	//invert the Y axis (maybe it shouldn't)
	//if(!onflip) DDRAW::doSuperSamplingMul(box);
	//if(!onflip) DDRAW::doSuperSamplingMul(client);
	//NOTE: OpenGL needs doSuperSampling to invert Y axis 
	bool ss = false;
	if(onflip) std::swap(ss,DDRAW::doSuperSampling); 

	DX::D3DVIEWPORT7 vp; DDRAW::Direct3DDevice7->GetViewport(&vp);

	DX::D3DVIEWPORT7 full = { 0,0,client.right,client.bottom,0.0,1.0 };

	DDRAW::Direct3DDevice7->SetViewport(&full);

	//2022: SOM always passes 0,0 so som_hacks_Clear can ignore other values
	//DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_CLEAR_HACK,0);

	DX::D3DRECT buf[4]; DWORD i = 0;

	if(box.left) 
	{																	  
		DX::D3DRECT clear;

		clear.x1 = client.left; clear.y1 = client.top; 
		clear.y2 = client.bottom; clear.x2 = client.left+box.left; //left+1 //2020

		//DDRAW::Direct3DDevice7->Clear(1,&clear,D3DCLEAR_TARGET,0x00000000,0.0,0);	
		buf[i++] = clear;

		clear.x1 = client.left+box.right; clear.x2 = client.right;

		//DDRAW::Direct3DDevice7->Clear(1,&clear,D3DCLEAR_TARGET,0x00000000,0.0,0);	
		buf[i++] = clear;

		/*buf won't work
		if(box.top) //optimization
		{
			DX::D3DVIEWPORT7 half = full; //avoid double clearing corners
			
			half.dwX = box.left; half.dwWidth = box.right-box.left;

			DDRAW::Direct3DDevice7->SetViewport(&half);
		}*/
	}
	if(box.top) 
	{
		DX::D3DRECT clear;

		clear.x1 = client.left; clear.y1 = client.top;		
		clear.x2 = client.right; clear.y2 = client.top+box.top; //top+1 //2020

		//DDRAW::Direct3DDevice7->Clear(1,&clear,D3DCLEAR_TARGET,0x00000000,0.0,0);	
		buf[i++] = clear;

		clear.y1 = client.top+box.bottom; clear.y2 = client.bottom;

		//DDRAW::Direct3DDevice7->Clear(1,&clear,D3DCLEAR_TARGET,0x00000000,0.0,0);	
		buf[i++] = clear;
	}
	if(i) DDRAW::Direct3DDevice7->Clear(i,buf,D3DCLEAR_TARGET,0x00000000,0.0,0);

	//2022: SOM always passes 0,0 so som_hacks_Clear can ignore other values
	//DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_CLEAR_HACK,som_hacks_Clear);
	
	DDRAW::Direct3DDevice7->SetViewport(&vp);

	if(onflip) std::swap(ss,DDRAW::doSuperSampling);

	DDRAW::doNothing = false;
}
static void *som_hacks_Clear(HRESULT *hr,DDRAW::IDirect3DDevice7*p,DWORD&x,DX::LPD3DRECT&y,DWORD&z,DX::D3DCOLOR&w,DX::D3DVALUE&q,DWORD&)
{
	if(!hr)
	{
		//for some reason menus pass a RECT and 1
		//and depth is 1.0 even though z is color
		if(x==1&&q==1) 
		{
			//actually it calls Clear twice after
			//drawing the picture background. but
			//I hope depth-write is disabled
			//assert(z==1);
			assert(!y->x1&&y->x2==*(DWORD*)0x4c0974);
			assert(!y->y1&&y->y2==*(DWORD*)0x4c0978);

			x = 0; y = 0;
		}
		else assert(x!=1);

		//HACK: SOM doesn't do multi-rect clears (must be internal)
		if(x) return 0;
	}

	auto &mdl = *SOM::L.arm_MDL;

	if(hr) //character model
	{
		if(SOM::picturemenu)		
		if(SOM::motions.frame!=SOM::frame)
		return 0;

		bool swinging2 = SOM::L.swinging||mdl.d>1||mdl.ext.d2;

		//2021: draw arms first
		if(1&&z==3&&swinging2)
		if(EX::INI::Bugfix()->do_fix_zbuffer_abuse)
		{
			//assert(SOM::L.swinging==1);

			SOM::L.swinging|=2; //signal was drawn
			
			DX::D3DVIEWPORT7 vp; p->GetViewport(&vp);
			float swap = vp.dvMinZ;
			float swap2 = vp.dvMaxZ;
			vp.dvMinZ = EX_INI_Z_SLICE_1;
			vp.dvMaxZ = EX_INI_Z_SLICE_2;	
			p->SetViewport(&vp);
			//NOTE: to accomplish this som_scene_reprogram has
			//swapped the Clear and BeginScene calls...I think
			//this new order is more correct reading MSDN docs
			extern void som_scene_swing(bool,float);
			som_scene_swing(true,1);
			vp.dvMinZ = swap; 
			vp.dvMaxZ = swap2;			
			p->SetViewport(&vp);
		}

		return 0;
	}
	else if(SOM::motions.frame!=SOM::frame) //menus?
	{
		//som_hacks_items_over_menus?
		//2021: clear zbuffer if arm is visible on menus so it
		//doesn't cover them up
		if(mdl.d>1||mdl.ext.d2)
		if(z==D3DCLEAR_ZBUFFER)
		if(som_hacks_items_over_menus&&!DDRAW::xr)
		if(som_hacks_tint!=SOM::frame)
		if(som_hacks_huds==SOM::frame)
		{
			//TODO: this wouldn't be required if the menus 
			//shared the 3D compass's partition?

			assert(!SOM::picturemenu);

			return 0;
		}
	}

	if(DDRAW::doNothing) assert(0);
	if(EX::INI::Bugfix()->do_fix_zbuffer_abuse) //documentation
	{
		if(z==1||z==3) //picture background fix
		{	 
			static unsigned once_per_frame = 0;

			//SOM is double clearing if talking 
			extern bool som_state_reading_text;
			extern bool som_state_reading_menu;
			extern bool som_state_viewing_show;				
			if(z==1&&!som_state_viewing_show) 												
			if(!SOM::picturemenu&&SOM::field
			||som_state_reading_text||som_state_reading_menu) 
			{
				z = 0; return 0;
			}				  

			if(once_per_frame==SOM::frame)
			{
				z = 0; return 0;
			}
			else z = 3; q = 1; once_per_frame = SOM::frame; 
		}

		DX::D3DVIEWPORT7 vp; p->GetViewport(&vp);

		if(z==D3DCLEAR_ZBUFFER) 
		{
			//skysync: hmmm? originally seemed
			//like the zbuffer was always cleared
			//if there was a sky or not, but no more

			z = 0; static unsigned skysync = -1;

			if(skysync!=SOM::frame
			&&SOM::field&&SOM::sky) //NEW
			{		
				skysync = SOM::frame; //scene
				vp.dvMinZ = EX_INI_Z_SLICE_2; vp.dvMaxZ = 1; 
			}
			else //menu/swing model
			{
				//2020: som_scene_42DC20 effects and KF2
				//style compass use 0~0.001f
				//vp.dvMinZ = 0;
				vp.dvMinZ = EX_INI_Z_SLICE_1; //0.001f; 
				vp.dvMaxZ = EX_INI_Z_SLICE_2;	
			}			
		}
		else //fog/sky model or scene
		{
			vp.dvMinZ = EX_INI_Z_SLICE_2; vp.dvMaxZ = 1;
		}
		p->SetViewport(&vp);
	}

	if(z||!som_hacks_early_disable_MRT) //2021
	{
		//2022: som_MPX_411a20 sets the background/fog to
		//0xffFFFFFF (this old code was undocumented)

		if(z&D3DCLEAR_TARGET)
		{
			//if(w!=0xFFFFFF) //??? //REMOVE ME
			if(w!=0xffFFFFFF) //??? //REMOVE ME
			{
				//what does this mean? what other case Clear?
				//picture menu?
				//assert(w==0xFFFFFF||!SOM::field);
			}
			else if(SOM::clear) //paranoia
			{
				w = SOM::clear;
				EX::INI::Adjust ad;
				if(&ad->fov_fogline_saturation)
				w = ad->fov_fogline_saturation;
			}

			if(!DDRAW::doInvert) som_hacks_level(w);

			if(DDRAW::WGL_NV_DX_interop2&&!DDRAW::inStereo)
			{
				int swap = 0; //OPTIMIZING
				std::swap(swap,DDRAW::doClearMRT);
				{
					//can't do this in som_hacks_onflip
					som_hacks_clear_letterbox(false);
				}
				std::swap(swap,DDRAW::doClearMRT);
			}
		}

		if(som_hacks_early_disable_MRT) //DEBUGGING
//		if(SOM::field&&!SOM::picturemenu) //TESTING: how to detect a menu?
		{
			//2021: som_hacks_Blt is turning off MRT in case it interferes
			//with text on OpenGL	
			if(DDRAW::doClearMRT) //190: D3DRS_COLORWRITEENABLE1
			if(z!=D3DCLEAR_ZBUFFER)
			DDRAW::Direct3DDevice7->SetRenderState((DX::D3DRENDERSTATETYPE)190,0xF);	 
		}
	}

	//HACK: capture viewport for Exselector::svg_view?
	if(z&D3DCLEAR_TARGET)
	{
		extern int SomEx_output_OpenXR_font_callback(int);
		EX::beginning_output_font(true,SomEx_output_OpenXR_font_callback);
	}

	return som_hacks_Clear;
}

extern int som_hacks_inventory_item = 0;
extern float som_hacks_inventory[4][4] = 
{	{1,0,0,0},{0,1,0,0},{0,0,1,0},

	//NOTE: having player_character_radius2 override viewing distance

	//{0,0,2,1} //same values som_db uses
	//good for 50 zoom, but using 62 instead (I think VR has its own FOV)	
	{0.00375f,0.045f,1,1} //{0.00225f,0.04f,1,1}
};
//0044D30E 68 20 F5 45 00       push        45F520h 
static const DX::LPD3DMATRIX som_hacks_identity = (DX::LPD3DMATRIX)0x45F520;
extern void som_hacks_recenter_item(DX::LPD3DMATRIX y)
{
	if(DDRAW::inStereo) return; //test

	//the profile center/tilt (menu/store only)
	//004231C3 D9 47 40             fld         dword ptr [edi+40h]
	//004231D6 66 8B 47 44          mov         ax,word ptr [edi+44h]
	//initial rotation (-0.00349065871)
	//004231F0 C7 42 50 89 C3 64 BB mov         dword ptr [edx+50h],0BB64C389h
	//float recenter = (som_hacks_inventory[0][0]-1);
	//0.75 is good for 50 zoom, but using 62 instead			
	float recenter = 0.5; //0.75;			
	//Equipment menu only?
	if(3==SOM::L.main_menu_selector&&SOM::Game.browsing_menu)
	{
		extern float som_game_menucoords[2];
		y->_41-=som_game_menucoords[0]/SOM::fov[0];
		float aspect = SOM::fov[0]/SOM::fov[1];
		//experimentally determined values, based on KF2 breastplate equipment
		float x = som_hacks_lerp(0.54f,0.24f,(aspect-1.25f)/(1.77777f-1.25f));
		y->_41-=x*recenter; 
		if(y->_42>-0.6f)
		y->_42 = -0.65f;
		y->_42+=0.15f;				
	}
	else if(y->_42>-0.3f)
	{
		y->_42 = -0.35f;
	}
	y->_42+=0.35f/2*recenter; 

	//TODO: go higher if name/menu is not shown
	if(SOM::Game.taking_item)
	y->_42+=0.0325f;
}
static void *som_hacks_SetTransform(HRESULT*,DDRAW::IDirect3DDevice7 *p,DX::D3DTRANSFORMSTATETYPE &x,DX::LPD3DMATRIX &y)
{		
	if(!y) assert(0); else switch(x)
	{
	case DX::D3DTRANSFORMSTATE_WORLD:
	
		if(y==som_hacks_identity)
		{
			if(som_hacks_huds==SOM::frame)
			{
				if(DDRAW::xr) //OpenXR?
				{
					if(!som_hacks_menuxform)
					{
						//NOTE: som_hacks_Blt sets this too, so I don't
						//know if it needs to be set again... (probably
						//not after a testing period)
						
						//REMOVE ME?
						//(void*&)y = SOM::menuxform; return 0;
						(void*&)y = SOM::stereo344[0]; return 0;

						y = nullptr; return 0; //short-circuit?
					}
				}
			}
		}
		else if(som_hacks_tint==SOM::frame) //KF2 (items)
		{
			if(DDRAW::xr) //OpenXR?
			{
				break; //som_game_410830? //som_game_4230C0?
			}

			som_hacks_recenter_item(y); //REFACTOR
		}
		//som_hacks_world = *y; //OBSOLETE			
		break;

	case DX::D3DTRANSFORMSTATE_VIEW:
	{	
		if(y->_11==1&&!memcmp(y,&DDRAW::Identity,sizeof(DX::D3DMATRIX)))
		{
			//NOTE: ApplyStateBlock was passing DDRAW::getD3DMATRIX
			//I've changed SetTransform to not call the hook in its
			//case (or any getD3DMATRIX case)
			assert(y==(DX::LPD3DMATRIX)0x45F520);

			return 0; //don't want to change compass in VR right now
		}

		//this is getting ridiculous
		if(y->m!=som_hacks_inventory) //blacklist
		if(y->m!=SOM::analogcam&&y->m!=SOM::steadycam)
		if(y->m!=som_hacks_view.m&&!DDRAW::ofApplyStateBlock)
		{	
			if(1) //if(SOM::L.view_matrix_xyzuvw)
			{	
				y = (DX::LPD3DMATRIX)SOM::analogcam;

				if(som_hacks_pcstated) //occasionally necessary
				{
					som_hacks_pcstated = false; //paranoia (see below) 
					memcpy(SOM::L.pcstate,som_hacks_pcstate,sizeof(float)*6);
				}
				
				const float gap = 1;
				if(SOM::newmap!=SOM::frame) //2022
				if(SOM::newmap==SOM::frame||gap/*<delta*/ //human scale
				//NOTE: this is not the reason _pcstate/d exists in the 
				//first place, but they are convenient here nonetheless
				//NOTE: while we could try to catch warp events as they
				//happen it might be useful to authors to nudge players
				 <=som_hacks_distance(SOM::L.pcstate,som_hacks_pcstate)
				//don't warp if traveling more than gap at a min 15 FPS 
				 &&gap>som_hacks_length(SOM::doppler)/15)
				{
					if(SOM::newmap!=SOM::frame) 
					{	
						//EXPERIMENTING
						//2017: It's come time to not expect the clipper to kick the
						//player around for legit reasons. This is to solve problems
						//with two objects sucking the player into their middle zone
						//and vomitting them back up on the other side. Mainly doors
						//can do this. And yes, it would be better to fix the object
						//code. But this is better than nothing.

				//		/*2020: this is becoming a liability for vertical doors when
						//shutting on your head. the original clipping issues may've
						//been fixed already by subsequent fixes
						if(1&&SOM::warped!=SOM::frame&&!SOM::emu)
						{
							if(SOM::frame-SOM::warped>5) //being suppressed
							{
								if(EX::debug) MessageBeep(MB_ICONWARNING);
							}

							SOM::warped = SOM::frame;
				//			memcpy(SOM::L.pcstate,som_hacks_pcstate,sizeof(float)*3);
				//			goto put_back;
						}
					}
					SOM::warp(SOM::L.pcstate,som_hacks_pcstate);
				//	put_back:;
				}				
				memcpy(som_hacks_pcstate,SOM::L.pcstate,sizeof(float)*6);
				som_hacks_pcstated = true;

				//where SOM believes the center of the sky to be
				float swing[6], skyanchor = 
				SOM::motions.place_camera(SOM::analogcam,SOM::steadycam,swing);				
				/*OBSOLETE?
				som_hacks_sky[0] = SOM::L.pcstate[0];
				som_hacks_sky[1] = SOM::L.pcstate[1]+skyanchor;
				som_hacks_sky[2] = SOM::L.pcstate[2];				
				*/

				//SOM will use this to construct its arm
				memcpy(SOM::L.pcstate,swing,sizeof(swing));
			}								 
			else //obsolete: 2 4x4 inverses is gratuitous
			{
				assert(0);

				D3DXMATRIX inv; 
				//the inverse of the view is the view itself
				if(D3DXMatrixInverse(&inv,0,(D3DXMATRIX*)y))
				{				
					float bob = inv._42-SOM::xyz[1]-1.5f; 					
					float pov = -1.5f+EX::INI::Player()->player_character_stature;

					SOM::eye[3] = bob;											
					SOM::steadycam[3][1] = -bob+SOM::eye[3]+pov; //_baseline_adjustment;
					inv._42 = inv._42+SOM::steadycam[3][1];

					SOM::eye[0] = inv._41; 
					SOM::eye[1] = inv._42-SOM::eye[3]; 
					SOM::eye[2] = inv._43;
					
					for(int i=0;i<3;i++) SOM::eye[i]-=SOM::xyz[i];
									
					/*//obsolete: but can be useful for visualizing the frustum
					if(SOM::L.frustum&&SOM::context==SOM::Context::playing_game)
					{
						EX::INI::Option op;	EX::INI::Bugfix bf;
						if(op->do_frustum&&bf->do_fix_frustum_in_memory)
						{							
							p->GetTransform(DX::D3DTRANSFORMSTATE_PROJECTION,DDRAW::getD3DMATRIX);
							som_hacks_updatefrustum(&inv,(D3DXMATRIX*)DDRAW::getD3DMATRIX); 
						}
					}*/

					som_hacks_view = *(DX::D3DMATRIX*)
					D3DXMatrixInverse((D3DXMATRIX*)y,0,&inv);

					y = &som_hacks_view;
				}
			}
		}
		if(DDRAW::inStereo) if(!DDRAW::xr) //PSVR?
		{
			extern bool som_mocap_PSVR_view(float(&out)[4][4],float(&in)[4][4]);
			if(som_mocap_PSVR_view(som_hacks_view.m,y->m))
			y = &som_hacks_view;
		}
		break;
	}
	case DX::D3DTRANSFORMSTATE_PROJECTION: 
	{
		/*
		float zerodiv1 = y->_33, zerodiv2 = y->_33-y->_34; //paranoia

		if(zerodiv1&&zerodiv2)
		{
			//reconstruct near/far planes from projection matrix
//			SOM::fov[2] = y->_44-y->_43/zerodiv1*y->_34;
//			SOM::fov[3] = (y->_44-y->_43)/zerodiv2*y->_34+y->_44;
							
			//SOM::fov[2]*=1.5f;
			//TODO: might want to adjust znear/far
			//y->_33 = SOM::fov[3]/(SOM::fov[3]-SOM::fov[2]);
			//y->_43 = -SOM::fov[2]*SOM::fov[3]/(SOM::fov[3]-SOM::fov[2]);
		
			if(0) //SOM seems to provide correct fov values
			{
				//arccotangent = atan(1/x)
				float fovh = atan(1.0f/y->_11)*2;
				float fovv = atan(1.0f/y->_22)*2;

				//if(op->do_fov)
				{
					fovv*=1.0f; //op->fov_vertical_multiplier

					fovh = 2.0f*atan(tan(fovv*0.5)*SOM::fov[0]/SOM::fov[1]);

					fovh*=1.0f; //op->fov_horizontal_multiplier
				}

				//cotangent = 1/tan(x)
				y->_11 = 1.0f/tan(fovh*0.5); 
				y->_22 = 1.0f/tan(fovv*0.5);
			}
		}*/
		
		break;
	}}

	return 0;
}

static void *som_hacks_GetTransform(HRESULT*,DDRAW::IDirect3DDevice7 *p,DX::D3DTRANSFORMSTATETYPE &x,DX::LPD3DMATRIX &y)
{
	return 0; //turns out unnecessary

	return x==DX::D3DTRANSFORMSTATE_VIEW?som_hacks_GetTransform:0;

	if(!DDRAW::ofApplyStateBlock)
	if(y&&memcmp(y,&DDRAW::Identity,sizeof(DX::D3DMATRIX)))
	{
		y->_42+=-1.5f+EX::INI::Player()->player_character_stature;
	}

	return 0;
}

static void *som_hacks_SetLight(HRESULT*hr,DDRAW::IDirect3DDevice7*in,DWORD &x,DX::LPD3DLIGHT7 &y)
{	
	void *out = 0; bool on = false; if(!y) return 0;
	 
	if(!hr) if(som_hacks_lightlog)
	{
		if(x<SOM_HACKS_LIGHTS) som_hacks_lightlog[x] = *y; //backup
	}

	if(som_hacks_lights_detected_map_close) //new map
	{			
		DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_LIGHTENABLE_HACK,0);
		
		//turn off all lights: doing all just for good measure
		for(int i=3;i<SOM_HACKS_LIGHTS;i++) in->LightEnable(i,0);

		DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_LIGHTENABLE_HACK,som_hacks_LightEnable);

		memset(som_hacks_lightfix,0x00,sizeof(DX::D3DLIGHT7)*SOM_HACKS_LIGHTS);
		memset(som_hacks_lightlog,0x00,sizeof(DX::D3DLIGHT7)*SOM_HACKS_LIGHTS);

		memset(som_hacks_lightfix_on,0x00,sizeof(bool)*SOM_HACKS_LIGHTS);
		memset(som_hacks_lamps_switch,0x00,sizeof(som_hacks_lamps_switch));

		*som_hacks_lamps_pool = 0;

		som_hacks_lights_detected_map_close = false;
	}

	if(!hr) //Note: assuming one of Sword of Moonlight's lights
	{	
#ifdef _DEBUG

		if(0) //was using som_db 1.1
		if(y&&y->dltType==DX::D3DLIGHT_POINT) 
		if(!y->dvAttenuation0&&!y->dvAttenuation2) //som_rt 
		{
			//assert(!SOMDB::detected()); //v1.2 

			//expected behavior
			assert(y->dvAttenuation1==0.4f); 
		}
		else if(!y->dvAttenuation1) //som_db model
		{
			//assert(SOMDB::detected()); //v1.1

			//expected behavior
			assert(y->dvAttenuation0==1.0f&&y->dvAttenuation2==1.0f); 
		}
		else assert(0); //non-SOM traffic??
#endif				
		if(1)
	  //if(!dt||dt->lights_calibration_factor) //deprecated behavior
		if(y&&y->dltType==DX::D3DLIGHT_POINT) 
		{	
			static DX::D3DLIGHT7 calibrate;	calibrate = *y;

			static const DX::D3DCOLORVALUE colortest[3] = 
			{
				{1.0f,0.0f,0.0f,1.0f},{0.0f,1.0f,0.0f,1.0f},{0.0f,0.0f,1.0f,1.0f},
			};

			static int testing = 0;

			//calibrate.dcvDiffuse = colortest[testing++%3];

			EX::INI::Detail dt;

			calibrate.dvAttenuation0 = dt->lights_constant_attenuation; 
			calibrate.dvAttenuation1 = dt->lights_linear_attenuation; 
			calibrate.dvAttenuation2 = dt->lights_quadratic_attenuation; 

			float i = fabsf(calibrate.dcvDiffuse.r);
			i = max(i,fabsf(calibrate.dcvDiffuse.g));
			i = max(i,fabsf(calibrate.dcvDiffuse.b));
			
			float a = 0.0f, d = 1.0f;

			EX::INI::Option op;

			if(op->do_ambient) //UNUSED/EXPERIMENTAL
			{						  
				a = dt->ambient_contribution; d = 1.0f-a;
			}

			a*=dt->lights_ambient_multiplier;
			d*=dt->lights_diffuse_multiplier;

			calibrate.dcvAmbient.r = calibrate.dcvDiffuse.r*a;
			calibrate.dcvAmbient.g = calibrate.dcvDiffuse.g*a;
			calibrate.dcvAmbient.b = calibrate.dcvDiffuse.b*a; 
			calibrate.dcvDiffuse.r = calibrate.dcvDiffuse.r*d;  
			calibrate.dcvDiffuse.g = calibrate.dcvDiffuse.g*d;
			calibrate.dcvDiffuse.b = calibrate.dcvDiffuse.b*d;			
								  
			float C = i?1.0f/255.0f/*/i*/:0; //effective attenuation
			
			float &X = calibrate.dvAttenuation0;
			float &Y = calibrate.dvAttenuation1;
			float &Z = calibrate.dvAttenuation2;
			float &M = calibrate.dvRange;

//			if(dt) calibrate.dvAttenuation1*=dt->lights_exponent_multiplier;

			M = C?M*dt->lights_distance_multiplier:0;
					
			//2017: SOM seems to approximate this, at least in one case
			//involving a Moratheia 2.1 map with more than 16 50m light
			//sources.
			if(0)
			if(M&&C)
			{	
				//Y = -(C*Z*M*M+X*C-1)/(C*M);

				Z = -(C*M*Y+C*X-1)/(C*M*M);

				//solution to c=1/(x+ym+zmm) (where range is m)
				//(sqrtf((C*4-C*C*4*X)*Z+C*C*Y*Y)-C*Y)/(C*2*Z);				

				if(0)
				if(Z) //avoiding divide by zero (only way I know how)
				{
					//Note: may as well factor out x (see above)
					M = (sqrtf((C*4-C*C*4*X)*Z+C*C*Y*Y)-C*Y)/(C*2*Z);				

					float proof = 1.0f; proof/=X+Y*M+Z*M*M; 

					assert(fabsf(proof-C)<0.00003);
				}
				else if(Y) //same deal (zero divide)
				{
					M = -(C*X-1)/(C*Y);
				}
			}

/*			//deprecated behavior
			if(dt->lights_calibration_factor!=1.0f) //partial calibration
			{
				D3DXVECTOR3 temp; float t = dt->lights_calibration_factor;
				
				temp = *(D3DXVECTOR3*)&calibrate.dcvAmbient;

				D3DXVec3Lerp((D3DXVECTOR3*)&calibrate.dcvAmbient,
							 (D3DXVECTOR3*)&y->dcvAmbient,&temp,t);

				temp = *(D3DXVECTOR3*)&calibrate.dcvDiffuse;

				D3DXVec3Lerp((D3DXVECTOR3*)&calibrate.dcvDiffuse,
							 (D3DXVECTOR3*)&y->dcvDiffuse,&temp,t);

				temp = *(D3DXVECTOR3*)&calibrate.dvAttenuation0;

				D3DXVec3Lerp((D3DXVECTOR3*)&calibrate.dvAttenuation0,
							 (D3DXVECTOR3*)&y->dvAttenuation0,&temp,t);

				calibrate.dvRange = y->dvRange+
				(calibrate.dvRange-y->dvRange)*t;
			}	
*/
			y = &calibrate;
		}

		return som_hacks_SetLight;
	}
	else if(*hr!=D3D_OK) return 0;

	if(som_hacks_lamps)
	if(x>2&&x<SOM_HACKS_LIGHTS)
	if(!som_hacks_lightfix[x].dltType)
	{
		assert(som_hacks_lamps_pool&&*som_hacks_lamps_pool<SOM_HACKS_LIGHTS);

		if(som_hacks_lamps_pool&&*som_hacks_lamps_pool<SOM_HACKS_LIGHTS)
		{	
			som_hacks_lamps_pool[++*som_hacks_lamps_pool] = x;
		}
	}

	if(som_hacks_lightfix)
	{
		if(x<SOM_HACKS_LIGHTS) som_hacks_lightfix[x] = *y; 					  
	}

	return 0;
}

static void *som_hacks_SetRenderState(HRESULT *hr,DDRAW::IDirect3DDevice7*in,DX::D3DRENDERSTATETYPE&x,DWORD&y)
{
	if(!hr) switch(x)
	{
	case DX::D3DRENDERSTATE_ZENABLE:

		som_hacks_zenable = y!=0; //2021

		assert(y<=1); break; //debugging

	case DX::D3DRENDERSTATE_SRCBLEND: 

		som_hacks_srcblend = y; break; //debugging

	case DX::D3DRENDERSTATE_DESTBLEND:

		som_hacks_destblend = y; break; //debugging

	case DX::D3DRENDERSTATE_ALPHABLENDENABLE: 

		som_hacks_alphablendenable = y?true:false; 

		som_hacks_fab = y==0xFAB; break;

	case DX::D3DRENDERSTATE_FOGENABLE: 
		
		som_hacks_fogenable = y?true:false; 
		
		if(EX::INI::Option()->do_rangefog)	
		in->SetRenderState(DX::D3DRENDERSTATE_RANGEFOGENABLE,y); break;
	
	case DX::D3DRENDERSTATE_FOGTABLEMODE: 
		
		if(EX::INI::Option()->do_rangefog)	
		in->SetRenderState(DX::D3DRENDERSTATE_FOGVERTEXMODE,y); break;	
	
//	case DX::D3DRENDERSTATE_FOGEND: (float&)y = SOM::fogend; break;	

//	case DX::D3DRENDERSTATE_FOGSTART: (float&)y = SOM::fogstart; break;	

	case DX::D3DRENDERSTATE_FOGCOLOR: 
		
		if(y!=som_hacks_fogalpha&&y!=som_hacks_foglevel) 
		{
			som_hacks_fogcolor = y&0x00FFFFFF;
		
			if(!som_hacks_shader_model) som_hacks_level(y); 

			som_hacks_foglevel = y|0xAD000000; 
		}		
		break;

	case DX::D3DRENDERSTATE_AMBIENT: 

		if((y&0xFF000000)!=0xFA000000) //false ambient
		som_hacks_ambient = y;		
		break;		
	}

	return 0;
}

extern bool som_scene_hud; 
static void som_hacks_stereo_z(DX::D3DTLVERTEX p[4])
{
	extern float som_game_menucoords[2];
	//EXPERIMENTAL: trying to push backward or something???
	//if(DDRAW::inStereo) for(int i=0;i<4;i++) p[i].sz = -2;
	for(int i=0;i<4;i++) 
	{
		p[i].sx+=som_game_menucoords[0];
		p[i].sy+=som_game_menucoords[1];
	}
}
static void *som_hacks_DrawPrimitive(HRESULT*hr,DDRAW::IDirect3DDevice7*in,DX::D3DPRIMITIVETYPE&x,DWORD&y,LPVOID&z,DWORD&w,DWORD&q)
{
	//fairy map?
	//TESTING A DISABLED FEATURE
	//what is 4144E0 (4C2351) up to? som_game_reprogram enables this
	#if 0 && defined(_DEBUG)
	if(y==(D3DFVF_DIFFUSE|D3DFVF_XYZRHW)) //transparent triangles???
	{
		//I can't make these appear on screen no matter what I do... I 
		//will have to try again someday
	//	assert(0);

		in->SetRenderState(DX::D3DRENDERSTATE_CULLMODE,DX::D3DCULL_NONE);

		typedef struct fvf
		{
			float x,y,z,w; D3DCOLOR dif;

		}fvf4[40];

		fvf4 &peek = *(fvf4*)z;

		for(int i=w;i-->0;)
		{
			//peek[i].x/=2; peek[i].y/=2;
			//peek[i].z = 1; peek[i].w = 1;
		}

		if(som_hacks_shader_model) //testing
		{
			//1:blit 5:sprite
			DDRAW::vs = DDRAW::ps = 5; //5? 
		}

		//auto *identity = (DX::LPD3DMATRIX)0x45F520;
		//DDRAW::Direct3DDevice7->SetTransform(DX::D3DTRANSFORMSTATE_VIEW,identity);
		//DDRAW::Direct3DDevice7->SetTransform(DX::D3DTRANSFORMSTATE_PROJECTION,identity);

		return 0;
	}
	#endif

	void *out = 0;

	static DWORD Y; if(!hr) Y = y; 

	if(som_hacks_shader_model) 
	{
		if(!hr) 
		{
			//1:blit 5:sprite
			DDRAW::vs = DDRAW::ps = Y==D3DFVF_TLVERTEX?1:5;
			
			out = som_hacks_DrawPrimitive;
		}
//		else DDRAW::vs = DDRAW::ps = 0;
	}	
					 
	//2017: trying to simplify clamping of menu elements.
	//this had saved the current address mode, and restored
	//it. but it failed with SOM::Map because it is recursive
	//That will change, but still, this was very work intensive
	if(!hr) if(Y==D3DFVF_TLVERTEX) //menu elements 
	{
		if(som_hacks_texture_addressu!=DX::D3DTADDRESS_CLAMP)
		{
			in->SetTextureStageState(0,DX::D3DTSS_ADDRESS,DX::D3DTADDRESS_CLAMP);
		}
	}
	else //TODO? do_spritefog
	{
		assert(y==D3DFVF_LVERTEX); 

		if(som_hacks_texture_addressu!=DX::D3DTADDRESS_WRAP)
		{
			in->SetTextureStageState(0,DX::D3DTSS_ADDRESS,DX::D3DTADDRESS_WRAP);
		}
	}

  /*////*/ if(hr) return 0; /*/// CAREFUL ///*/
	
	if(x!=DX::D3DPT_TRIANGLEFAN||y!=D3DFVF_TLVERTEX||w!=4) 
	{
		return out; //???
	}

	//NEW: reserve point filter for 3D things
	if(som_hacks_magfilter!=DX::D3DTFG_LINEAR)
	{
//		assert(!SOM::filtering||SOM::emu); //TESTING (2020)

		in->SetTextureStageState(0,DX::D3DTSS_MINFILTER,DX::D3DTFN_LINEAR);
		in->SetTextureStageState(0,DX::D3DTSS_MAGFILTER,DX::D3DTFG_LINEAR);
	}
	
	//auto *ts7 = som_hacks_texture;
	auto *ts7 = DDRAW::TextureSurface7[0]; //2022

	EX::INI::Bugfix bf; EX::INI::Option op; EX::INI::Adjust ad;
	
	static DX::D3DTLVERTEX p[4]; //paranoia?
	
	memcpy(p,z,sizeof(p)); z = p; //!
		
	DWORD c = p[0].color; BYTE *cb = (BYTE*)&c; 

	//2022: going to assume these are square?
	//if(p[0].sx>0||p[0].sy>0||p[1].sy>0||p[3].sx>0) 
	if(p[0].sx>0||p[0].sy>0||som_hacks_Blt_fan) 
	{	
		//OpenXR moves KF2's gauges to center
		//bool hud = p[0].sx<SOM::width*0.5f;
		bool hud = p[0].sx<SOM::width*0.75f;

		int cx = 0, cy = 0;
		if(DDRAW::xr&&som_scene_hud) //center pivot?
		if(!(hud?op->do_kf2:op->do_nwse)) //classic?
		{
			cx = (int)SOM::fov[0]/2;
			cy = (int)SOM::fov[1]/2;

			if(hud)
			{
				cx-=122; cy-=33;						
			}
			else //compass 
			{
				cx = -cx+61; cy-=61;
			}
		}

		RECT box = {p[0].sx,p[0].sy,p[2].sx,p[2].sy};

		if(SOM::mapmap==SOM::frame) //YUCK
		{
			if(0!=EX::context()) //SOM::map
			if(box.right-box.left<SOM::width/199.0f)
			{
				//HACK: This is recursive now :(
				//Reminder: p is a static variable.
				DX::D3DTLVERTEX q[4];
				memcpy(q,z,sizeof(p));
				SOM::Map(q/*p*/);
				memcpy(p,q,sizeof(p));
			}
			if(DDRAW::xr) goto menuxform;

			return out; 
		}

		if(2==som_hacks_Blt_fan)
		{
			//2022: drawing picture without Blt (OpenXR?)
		}
		else if(!SOM::field //start menus
		&&box.right-box.left==256&&box.bottom-box.top==64)
		{
			//401A22 knocks out 640x480
			float sx = SOM::width/640.0f;
			float sy = SOM::height/480.0f;
			for(int i=4;i-->0;)
			{
				//NOTE: 42c460 draws these with hardcoded sizes
				//(it's easier to scale here)
				p[i].sx*=sx; p[i].sy*=sy;
			}

			if(cb[3]!=0xFF&&ad->start_blackout_factor)
			{
				int bo = ad->start_blackout_factor*(0xFF-cb[3]);

				cb[0]-=bo; cb[1]-=bo; cb[2]-=bo; cb[3] = max(bo+cb[3],0xFF);

				for(int i=0;i<4;i++) p[i].color = c; 
			}		

			const DWORD tint = ad->start_tint; const BYTE *ct = (BYTE*)&tint;
							
			for(int i=0;i<4;i++){ int t = int(cb[i])-(0xFF-ct[i]); cb[i] = max(0,t); }

			//TODO: dx7a cannot do transparency?
			//2021: like alpha in the text color?
			if(op->do_start&&DDRAW::target!='dx7a')
			{
				//TODO: aspect correct these graphics!
				//TODO: aspect correct these graphics!
				//TODO: aspect correct these graphics!

				int text = 0; wchar_t x[32] = L"";					
				box.left = 0; 
				//2022: 401A22 knocks out 640x480
				//box.right = 640;
				box.right = SOM::width;
				//swprintf(x,L"%d , %d --- %d x %d",box.left,box.top,box.right-box.left,box.bottom-box.top);															
				switch(box.top)
				{
				case 360: text++; //Continue: 206 , 360 --- 256 x 64
				case 315: text++; //New Game: 206 , 315 --- 256 x 64
				case 330:         //Push Key: 205 , 330 --- 256 x 64
					
					EX::INI::Detail dt;
					int mode = dt->start_mode;
					//todo: do_som, do_kf, do_kf2, and do_kf3
					if(op->do_st&&!&dt->start_mode) mode = 4;
					/*2021: I think SOM::Start sees to this now 
					box.top = DDRAW::xyScaling[1]*box.top+0.5f;
					//box.left = DDRAW::xyScaling[0]*box.left+0.5f;
					//box.right = DDRAW::xyScaling[0]*box.right+0.5f;				
					box.bottom = DDRAW::xyScaling[1]*box.bottom+0.5f;
					OffsetRect(&box,DDRAW::xyMapping[0],DDRAW::xyMapping[1]);*/
					const char kf2[3][16] = {"START","Begin","Load Game"};				
					const char *en = mode==2?kf2[text]:som_932_Start[mode][text];
					if(!SOM::translate(x,som_932_Start[mode][text],en))
					{
						//REMOVE ME
						//static unsigned limit = 0;
						//if(2+limit<DINPUT::noPolls) //skip this screen
						{
							SOM::se_volume = -10000; //2021

							//this is consistently failing April 2021?
							DSOUND::knocking_out_delay(EX::tick()+30);
								
							//limit = DINPUT::noPolls; //drawing is asynchronous

							//2022: hitting assert in repeat?
							//(turns out this was jamming and
							//that assert dropping the buffer
							//was the only reason this worked)
							if(0==EX::Syspad._fifo||SOM::frame%5==1) 
							EX::Syspad.send(0x39); //SPACE 
						}
					}
					else SOM::Start(x,c,&box,DT_CENTER|DT_SINGLELINE); 

					w = 0; return out; //short-circuit!!				
				}
				assert(0);
			}

			//auto *ts7 = som_hacks_texture;
			auto *ts7 = DDRAW::TextureSurface7[0]; //2022

			DX::DDCOLORKEY black = {}; //allow black knockout 
			if(ts7&&ts7->GetColorKey(DDCKEY_SRCBLT,&black))
			{
				assert(0); //DDERR_NOCOLORKEY?
				ts7->SetColorKey(DDCKEY_SRCBLT,&black);
			}

			in->SetRenderState(DX::D3DRENDERSTATE_COLORKEYENABLE,1);					   							
		}
		else if(!SOM::field||som_hacks_tint==SOM::frame)
		{	
			extern bool som_game_continuing;

			if(!som_hacks_items_over_menus)
			{
				assert(0); //UNUSED

				//som_scene_translucent_frame zbuffer
				if(SOM::doubleblack!=SOM::frame)
				for(int i=0;i<4;i++) p[i].sz = 0.01f;
			}
			else //2021
			{
				//2021: need room to increase icons/frames z
				//2022: for some reason the HUD fakes doubleblack and this
				//causes the depth to come out wrong in OpenXR
				float sz; if(DDRAW::xr)
				{
					if(!som_scene_hud
					&&(SOM::Game.taking_item||SOM::doubleblack==SOM::frame))
					{
						sz = -SOM_STEREO_SUBMENU; //0.3f;
					}
					else sz = 0.1f;
				}
				else if(som_scene_hud||SOM::doubleblack!=SOM::frame)
				{
					sz = 1.0f;
				}
				else sz = 0.1f;

				assert(0==p[0].sz);				
				/*2021: trying to let 3d models go in front
				//of panels				
				for(int i=0;i<4;i++) p[i].sz = 0.01f;*/
				for(int i=0;i<4;i++) p[i].sz = sz; //1.00f;
			}			
				//NOTE: moved outside to cover auxiliary equipment
				//stats panel
				if(som_hacks_items_over_menus) //2021
				{
					//2021: enabling z-test so icons can be drawn
					//behind 3d models
					if(!som_hacks_zenable&&SOM::field) //2021
					in->SetRenderState(DX::D3DRENDERSTATE_ZENABLE,1);
				//	for(int i=0;i<4;i++) p[i].sz-=0.0001f;
				}

			if(som_hacks_alphablendenable&&!som_hacks_fab) 
			{	
				//2018: this is menu elements only
				//right? (som_hacks_stereo_z)
				////hmm: true for screen/support magics
				assert(cb[3]!=0xFF);
				
				static DX::IDirectDrawSurface7 *flash = 0;

				if(c!=0x7fffffff) flash = ts7;

				float o = float(cb[3])/0xFF; if(flash==ts7)
				{
					if(som_hacks_items_over_menus&&!DDRAW::xr)
					{
						auto &mdl = *SOM::L.arm_MDL;
						if(mdl.d>1||mdl.ext.d2) //emergency only?
						{
							//2021: undo above code?
							//WARNING: this casts a glow on the frame
							//because the highlight is drawn first and
							//may overlap but it doesn't write to the
							//z buffer I think (it seems not to anyway)
							if(SOM::doubleblack!=SOM::frame)
							for(int i=0;i<4;i++) p[i].sz = 0.5f;
						}
					}

					if(&ad->highlight_opacity_adjustment)					
					o = ad->highlight_opacity_adjustment(o);											
					extern char *som_state_40a8c0_menu_text;
					extern RECT som_state_40a8c0_menu_yesno[2];					
					if(som_state_40a8c0_menu_text)
					{
						if(som_state_40a8c0_menu_yesno[0].left)
						{
							//todo? replicate margins
							RECT &box = som_state_40a8c0_menu_yesno
							[p[0].sy-10>som_state_40a8c0_menu_yesno[0].top];						
							p[0].sx = box.left;  p[0].sy = box.top;
							p[1].sx = box.right; p[1].sy = box.top;
							p[2].sx = box.right; p[2].sy = box.bottom;
							p[3].sx = box.left;  p[3].sy = box.bottom;
						}
						else w = 0; //short-circuit
					}
					else som_hacks_stereo_z(p);
				}	
				else 
				{
					som_hacks_stereo_z(p);
					if(&ad->paneling_opacity_adjustment)
					o = ad->paneling_opacity_adjustment(o,SOM::doubleblack==SOM::frame);
				}
				if(!o) w = 0; //short-circuit?

				c = int(o*0xFF)<<24|0xFFFFFF&c; 				
				for(int i=0;i<4;i++) p[i].color = c; 

				//pad config and options screen fix
				static float bottoms[32], tops[32];
				static int n = 0; static float sx = 0; 
				static unsigned optimizing = SOM::frame;
				//highlight is 1px off
				if(fabsf(p[0].sx-sx)>4||optimizing!=SOM::frame)
				{
					tops[0] = p[0].sy; optimizing = SOM::frame;

					bottoms[0] = p[3].sy; n = 1; 
				}
				else if(n<EX_ARRAYSIZEOF(bottoms)-1)
				{
					for(int i=0;i<n;i++) 
					{
						if(fabsf(p[3].sy-tops[i])<4) p[2].sy = p[3].sy = tops[i]; 
						if(fabsf(p[0].sy-bottoms[i])<4)	p[0].sy = p[1].sy = bottoms[i];
					}
					bottoms[n] = p[3].sy; tops[n] = p[0].sy; n++;
				}
				else //assert(0);
				{
					//2021: this is preventing release mode debugging diagnosis :(

					EX_BREAKPOINT(0);
					//SOM::frame_is_missing(); //BeginScene problem
					optimizing = ~0;
				}
				
				sx = p[0].sx;

				//King's Field 2, making square to fill in gap
				//NOTE: vertically there is no gap
				p[1].sx+=3; p[2].sx+=3; 
				p[0].sx-=3; p[3].sx-=3; 
			}
			else if(som_hacks_tint==SOM::frame||som_game_continuing) 
			{
				//NOTE: moved outside to cover auxiliary equipment
				//stats panel
				if(som_hacks_items_over_menus) //2021
				{
					//2021: enabling z-test so icons can be drawn
					//behind 3d models
				//	if(!som_hacks_zenable&&SOM::field) //2021
				//	in->SetRenderState(DX::D3DRENDERSTATE_ZENABLE,1);
					for(int i=0;i<4;i++)
					{
						//OpenXR needs a bit more
						//p[i].sz-=0.0001f;
						p[i].sz-=0.005f;
					}
				}

				//it's possible to detect which texture is used 
				//like so
				DWORD &tex = *(DWORD*)0x1D6A280;	
				//0xB is the sword/decals (I think it's square)
				//0xC-0x0D are the arrows (horizontal is unused)
				//0xE-0x16 are item icons
				for(int i=0xB;i<=0x16;i++) //icons/som_game_421F10
				if(tex==SOM::menupcs[i]) 
				{
					if(som_hacks_items_over_menus) //2021
					{
						//2021: enabling z-test so icons can be drawn
						//behind 3d models
						for(int i=0;i<4;i++) 
						{
							//OpenXR needs a bit more
							//p[i].sz-=0.0001f;
							p[i].sz-=0.005f;
						}
					}

					//int debug_icon = i;
					//TODO: do this in som_game_421F10 if possible
				//	if(0&&EX::debug)
				//	for(int i=0;i<4;i++) p[i].color = 0xFF00FF00;
					//assert(0xB!=i);
					int square = box.bottom-box.top; //int
					if(i==0xC||i==0xD)
					{	 
						float half = 20/DDRAW::xyScaling[0];
						if(float sf=EX::stereo_font) half*=sf;
						p[2].sx = (box.left+box.right)/2+half;
						square = box.top+square/2;
						p[1].sy = square-half;						
						square = (int)(half*2);
					}					

					if(i>=0xE) //overlapping?
					{
						square-=1; p[1].sy+=1; //2021
					}

					//NEW: 0.5 offset is pixel perfect //???
					p[1].sx = p[2].sx-=0.5f;
					p[0].sy = p[1].sy+=0.5f;
					p[2].sy = p[3].sy = p[1].sy+square;
					//make square
					p[0].sx = p[3].sx = p[2].sx-square;
				}

				//0: top-left
				//1: top horizontal?
				//2: top-right?
				//3: left vertical?
				//4: right vertical?
				//5: bottom-left?
				//6: bottom horizontal?
				//7: bottom-right?
				for(int i=0x0;i<=0x7;i++)
				if(tex==SOM::menupcs[i]) //top, middle, bottom 
				{
					//need to do better than this for do_aa2 to work at
					//all resolutions.
					/*6 CONFLICTS WITH BELOW APPROACH
					if(i==1||i==6) //horizontal?					 
					{
						//these don't quite connect at some resolutions
						//on some menu screens
						//(Use Item?) (Buy, etc.)
						p[1].sx+=1; p[2].sx+=1; 

						//1280x800 left corners?
						p[0].sx-=1; p[3].sx-=1; 
					}
					if(1) //do_aa2 fix?*/
					{
						//I guess this can't hurt? but I'm not sure it 
						//helps either. the problem turned out to be to
						//do with either clamping or colorkey effects???
						//Note: I think only 1152 x 864 has this problem?

						static float y0,y1,y2,y3; switch(i)
						{
						case 0: y0 = p[0].sy;

							y1 = p[2].sy; break;

						case 3: y2 = max(y1,p[2].sy); //-1

							p[0].sy = p[1].sy = y1; break;

						case 5: y3 = p[2].sy;

							//there's overlap here that makes the bottom
							//vertically thinner (this could depend on the
							//the resolution setting) that's noticeable 
							//in VR on the HP display
							/*this makes a gap at the bottom of the inner
							//panels
							y3+=y2-p[1].sy; p[3].sy = p[2].sy = y3;
							*/

							p[0].sy = p[1].sy = y2; break;

						case 1: case 2: 
						
							p[0].sy = p[1].sy = y0; 
							p[3].sy = p[2].sy = y1; break;

						case 4:
							
							p[0].sy = p[1].sy = y1;
							p[3].sy = p[2].sy = y2; break;

						case 6: case 7:
							
							p[0].sy = p[1].sy = y2;
							p[3].sy = p[2].sy = y3; break;
						}
						static float x0,x1,x2,x3; switch(i)
						{
						case 0: x0 = p[0].sx;

							x1 = p[1].sx; break;

						case 1: x2 = p[1].sx;

							p[0].sx = p[3].sx = x1; break;

						case 2: x3 = p[1].sx;

							p[0].sx = p[3].sx = x2; break;

						case 3: case 5:

							p[0].sx = p[3].sx = x0;
							p[1].sx = p[2].sx = x1; break;

						case 6:

							p[0].sx = p[3].sx = x1;
							p[1].sx = p[2].sx = x2; break;
						
						case 4: case 7:

							p[0].sx = p[3].sx = x2;
							p[1].sx = p[2].sx = x3; break;
						}
					}

					//EXPERIMENTAL
					if(float l=ad->paneling_frame_translucency)
					{
						cb[3] = (1-l)*cb[3]; 

						if(!som_hacks_alphablendenable) 
						{
							//som_hacks_fab_alphablendenable();
							extern void som_scene_translucent_frame();
							som_scene_translucent_frame();
						}

						if(cb[3]) for(int i=0;i<4;i++) p[i].color = c;
				
						if(!cb[3]) w = 0; //short-circuit
					}

					if(p[0].sy>=p[2].sy) //double-blend artifact?
					{
						w = 0; //short-circuit?
					}

					//EXPERIMENTAL (VR)
					EX::INI::Stereo vr;
					//this is for an early demo
					if(DDRAW::xr&&&vr->kf2_demo_bevel) //bevel?
					{
						//this is a very basic KF2 like beveling effect
						//to make VR more 3D but it has some distortion
						//I don't fully understand where the alpha mask
						//doesn't meet and the sliver like side panels
						//become more visible as double blend artifacts

						const float *b = vr->kf2_demo_bevel; //0.02,0.01,0.225

						//float b0 = p[0].sz+0.04f;
						float b0 = b[0]+p[0].sz;
						//NOTE: this is pulling the alpha masked part into
						//the foreground to line up with the inner panel
						float b1 = b[0]+b[1];
						for(int j=4;j-->0;) p[j].sz-=b1;

						//0--1
						//|  |
						//3--2

						if(1) switch(i)
						{
						break; case 0: p[0].sz = p[1].sz = p[3].sz = b0; //tl
						break; case 1: p[0].sz = p[1].sz = b0; //th
						break; case 2: p[0].sz = p[1].sz = p[2].sz = b0; //tr
						break; case 3: p[0].sz = p[3].sz = b0; //lv
						break; case 4: p[1].sz = p[2].sz = b0; //rv
						break; case 5: p[0].sz = p[3].sz = p[2].sz = b0; //bl
						break; case 6: p[3].sz = p[2].sz = b0; //bh
						break; case 7: p[1].sz = p[3].sz = p[2].sz = b0; //br
						}

						//HACK: KF2 diagonal isn't symmetric
						//which causes distortion in the texture
						//map (fixing it this way blurs the texture)
						if(b[2]) 
						{
							//float lu = 0.225f, ru = 1-lu;
							float lu = b[2], ru = 1-lu;

							switch(i)
							{
							case 0: case 3: case 5:

								p[1].tu = p[2].tu = ru; break;

							case 2: case 4: case 7:

								p[0].tu = p[3].tu = lu; break;
							}
						}
						
						if(i==2||i==5) //rotate crease?
						{
							auto swap = p[3];
							memmove(p+1,p+0,sizeof(swap)*3);
							p[0] = swap;
						}
					}
				}

				som_hacks_stereo_z(p);
			}
			else if(!SOM::field) //sans tint
			{
				//opaque start/save screen elements
			}
		}
		else if(som_hacks_huds==SOM::frame)
		{			
			//2020: for some reason half the screen fx
			//sparkles are getting cut as compass needles
			//(only when the compass is hidden)			
			if(!som_scene_hud||som_hacks_Blt_fan) goto return_hud;

			if(hud) if(p[0].sx==125) //gauge (HUD)			  
			{
				//p[1].sx is 229 when at full capacity
				int gauge = p[0].sy==16?0:1;

				//gauges are always on for do_st etc (ACTUALLY
				//NOT ANY LONGER BECAUSE the attack buttons can
				//now be used to hide/show the gauges so for now
				//do_st ties the MP and status displays together)
				if(!SOM::gauge //2020
				||!(!gauge||!op->do_st?SOM::showGauge:SOM::showCompass)) 
				{
					w = 0; return out; //short-circuit 
				}

				float t = (p[1].sx-125)/(229-125);

				//Shadow Tower? King's Field II?
				if(op->do_st||op->do_kf2&&som_hacks_kf2_gauges.left)
				{	
					RECT box; if(som_hacks_kf2_gauges.left)
					{
						box = som_hacks_kf2_gauges;
						box.top = gauge?box.bottom:box.top;
						box.bottom = box.top+16; goto bar;
					}
					else if(SOM::Status(&box,0,'bar',gauge)) bar:
					{
						if(DWORD bb=ad->hud_gauge_border)
						{
							if(box.left==box.right)
							{
								w = 0; //do_kf2?
							}

							auto b = (BYTE*)&bb;
							int lr = -(int)b[3]+b[2];
							int tb = -(int)b[1]+b[0];
							if(som_hacks_kf2_gauges.left)
							{
								OffsetRect(&box,0,tb);
								tb+=min(b[1],b[0]);
							}
							box.left+=lr; box.right-=lr;
							box.top+=tb; box.bottom-=tb;
						}

						//interior_border?
						float bot,top = box.top; if(1!=DDRAW::xyScaling[1])
						{
							//YUCK: ensure guages are same height
							//2021: unfortunately I don't think it can guarantee
							//same distance from som_hacks_kf2_gauges's baseline 
							//(som_hacks_Blt could detect/scoot bottom text 1px)
							bot = DDRAW::xyScalingQuantize(1,box.bottom-top);
							top = DDRAW::xyScalingQuantize(1,top+0.5f);
							bot+=top;
						}
						else bot = box.bottom;

						p[0].sx = box.left;  p[0].sy = top;
						p[1].sx = box.right; p[1].sy = top;
						p[2].sx = box.right; p[2].sy = bot;
						p[3].sx = box.left;  p[3].sy = bot;
					}
					else w = 0; //short-circuit
				}
				else p[1].sx = p[2].sx = 229; //gauge

				if(op->do_kf2) //2022: depth for OpenXR?
				{
					float sz = DDRAW::xr?0.1f:1.0f; //DUPLICATE?

					sz-=0.0075f;

					for(int i=4;i-->0;) p[i].sz = sz;
				}

				if(t!=1) //draw depleted portion of the guage
				{		
					DX::D3DTLVERTEX _[4]; memcpy(_,p,sizeof(_));

					_[0].sx = _[3].sx = //! restore bar to original size
					p[1].sx = p[2].sx = som_hacks_lerp(p[0].sx,p[1].sx,t);
					_[0].tu = _[3].tu = //a courtesy not provided by SOM
					p[1].tu = p[2].tu = som_hacks_lerp(p[0].tu,p[1].tu,t);
										
					for(int i=4;i-->0;)
					{
						//hmmm: should ad->hud_gauge_opacity_adjustment be in play here ???
						_[i].color = ad->hud_gauge_depleted_half_tint;					

						_[i].sx+=cx; _[i].sy+=cy; //center pivot? //DDRAW::xr?
					}
					//if(som_hacks_)
					{
						for(int i=0;i<4;_[i].sy+=som_hacks_,i++)
						_[i].sx+=hud?som_hacks_:-som_hacks_;
					}
										
					if(DDRAW::xr) //DUPLICATE
					{
						y&=~D3DFVF_XYZRHW;
						y|=D3DFVF_XYZ|D3DFVF_RESERVED1; //RESERVED1 plugs the w hole

						auto *xform = &SOM::stereo344[1];
						if(som_hacks_menuxform)
						for(int i=4;i-->0;)
						{
							D3DXVECTOR3 ps(*(D3DXVECTOR3*)&_[i].sx);
							D3DXVec3TransformCoord((D3DXVECTOR3*)&_[i].sx,&ps,(D3DXMATRIX*)*xform); 
						}

						float a = som_hacks_hud_fade[0];

						if(a<=0)
						{
							w = 0; return out; //short-circuit?
						}
						for(int i=4;i-->0;)
						{
							auto &c = _[i].color; ((BYTE*)&c)[3]*=a;
						}
					}

					DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_DRAWPRIMITIVE_HACK,0); //hack
					in->DrawPrimitive(x,y,_,w,q); 
					DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_DRAWPRIMITIVE_HACK,som_hacks_DrawPrimitive);					
				}	
				if(&ad->hud_gauge_opacity_adjustment)				
				cb[3] = 255.0f*ad->hud_gauge_opacity_adjustment(cb[3]/255.0f);
				if(cb[3]) for(int i=0;i<4;i++) p[i].color = c; 
				if(!cb[3]) w = 0; //short-circuit

				goto return_hud;
			}				
			else //screen/support magics? SOM::Map
			{
				w = w; //breakpoint //assert(0);

				//EX::dbgmsg("sprite: %f %f",p->sx,p->sy);
			}

			if(!hud&&som_scene_hud) //compass needle?
			{					
				//2022: alpha is 11? //must be disabled?
				//(but what about the older code below?)
				for(int i=0;i<4;i++) p[i].color|=0xff000000; 

				if(som_hacks_displays_a[1])
				{
					if(!som_hacks_fab) 
					som_hacks_fab_alphablendenable();
					for(int i=0;i<4;i++) p[i].color|=0x80000000; 
				}
								
				if(op->do_st||0==SOM::showCompass)
				{
					w = 0; //short-circuit compass needle
				}
				else if(som_hacks_kf2_compass)
				{
					extern void som_scene_compass(float*);
					som_scene_compass(som_hacks_kf2_compass);

					w = 0; //short-circuit compass needle
				}
				else if(bf->do_fix_oversized_compass_needle) 
				{	
					//(2018: needed or not, it better centers the
					//needle)
					//maybe unneeded when som_rt/db is patched???
					//[unpatched binaries are no longer supported]
					//Reminder: I think the centering may be better
					//than the original.

					if(SOM::x480()) goto return_hud; //looks better
							
					//SOM scales horizontally with resolution
					//float scaledown = 640/SOM::fov[0]; 
					float scaledown = 640.0f/SOM::width; 

					float centerx = 0.0, centery = 0.0;						

					for(int i=0;i<4;i++)
					{
						centerx+=p[i].sx; centery+=p[i].sy;				
					}

					centerx*=0.25; centery*=0.25;

					float vx = p[2].sx-p[1].sx, vy = p[2].sy-p[1].sy;

					float magnitude = sqrtf(vx*vx+vy*vy);
										
					if(magnitude>=63&&magnitude<=66) //64~65px+/-1
					{
						//close enough to correct scale
						if(0) scaledown = 1; else goto return_hud; 
					}												

					float fudgefactor = 0.1f/scaledown; //arbitrary

					float newoffsetx = vx/2*fudgefactor;
					float newoffsety = vy/2*fudgefactor;
				
					for(int i=0;i<4;i++)
					{
						p[i].sx = scaledown*(p[i].sx-centerx)+centerx+newoffsetx;
						p[i].sy = scaledown*(p[i].sy-centery)+centery+newoffsety;
					}
				}
			}

			return_hud:

			//if(som_hacks_) //!DDRAW::xr?
			{
				for(int i=0;i<4;p[i].sy+=som_hacks_,i++)
				{
					p[i].sx+=hud?som_hacks_:-som_hacks_;
				}
			}
			for(int i=4;i-->0;) //DUPLICATE //DDRAW::xr?
			{
				p[i].sx+=cx; p[i].sy+=cy; //center pivot?
			}
		}
		else //assert(0);
		{
			//2021: this is preventing release mode debugging diagnosis :(

			EX_BREAKPOINT(0)
		}
		
		if(som_scene_hud)		
		if(!hud||som_hacks_Blt_fan) //need to include needle
		{			
			//NOTE: the base of the compass is subject to whatever
			//state the last thing rendered had
			bool abe = DDRAW::xr!=0;
			if(abe!=som_hacks_alphablendenable)
			som_scene_alphaenable(abe?0xFAB:0);
			extern void som_scene_ops(DWORD,DWORD);
			som_scene_ops(D3DTOP_SELECTARG2,D3DTOP_DISABLE);
		}		

		if(DDRAW::xr) menuxform: //DUPLICATE
		{
			//YUCK: it would be nice if dx_d3d9x_drawprims_static worked
			//to do the 2D transform but I can't get a visual
			
			y&=~D3DFVF_XYZRHW; 
			y|=D3DFVF_XYZ|D3DFVF_RESERVED1; //RESERVED1 plugs the w hole

			auto *xform = &SOM::stereo344[0];
			if(som_scene_hud)
			{
				//OpenXR moves KF2's gauges to center
				//int hud = p[0].sx>SOM::width*0.5f; 
				//int compass = p[0].sx>SOM::width*0.75f; 
				int compass = !hud;

				xform = &SOM::stereo344[1+compass]; 

				float a = som_hacks_hud_fade[compass];

				if(a<=0)
				{
					w = 0; return out; //short-circuit?
				}
				
				if(compass||som_hacks_Blt_fan)
				{
					if(!som_hacks_fab) 
					som_hacks_fab_alphablendenable();
				}

				for(int i=4;i-->0;)
				{
					auto &c = p[i].color; ((BYTE*)&c)[3]*=a;
				}
			}

			//REMINDER
			// 
			// currently this is preferred over shader mainly since
			// it's hard to repair D3DTRANSFORMSTATE_WORLD after an
			// item is displayed
			// 
			//if not done here then it's done in the som.shader.hpp
			if(som_hacks_menuxform)
			for(int i=4;i-->0;)
			{
				//#include Somvector.h?
			//	auto *ps = &p[i].sx; //C++
			//	Somvector::map<3>(ps).premultiply<4>(*xform);
				D3DXVECTOR3 ps(*(D3DXVECTOR3*)&p[i].sx);
				D3DXVec3TransformCoord((D3DXVECTOR3*)&p[i].sx,&ps,(D3DXMATRIX*)*xform); 
			}
		}

		return out; //dead end
	}

	if(ts7 //som_hacks_texture
	||!som_hacks_alphablendenable
	||p[1].sx!=SOM::width||p[2].sy!=SOM::height
	||p[2].sx!=SOM::width||p[3].sy!=SOM::height)
	{
		//what gets here?
		if(DDRAW::xr){ assert(0); goto menuxform; };

		return out; //???
	}
		
	//HACK? do the real-time map before any b/w fades
	//NOTE: it seems that color flashes go underneath
	//in which case, they are not processed correctly
	//above :(
	if(SOM::map&&c>>24) SOM::Map2(p);

	//NEW: covers the edges
	p[0].sx-=1; p[0].sy-=1; p[1].sx+=1; p[1].sy-=1;
	p[3].sx-=1; p[3].sy+=1; p[2].sx+=1; p[2].sy+=1;
	
	//filter out redundant fills
	//not ready for release builds
	//const int bug = EX::debug?0:4; 
	const int bug = 0; //seems ok now
 	
	if((c&0x00FFFFFF)==0) //black tint/fade/dark
	{	
		if(cb[3]==0) w = 0; //WTH: title screen

		bool fade = false; //will get set below

		//hack: HUDs must be shown for this to work
		bool dark = som_hacks_huds!=SOM::frame;

		//inside of picture menu (no HUDs)
		if(som_hacks_blit==SOM::frame) dark = false;

		if(!dark) //then fade/tint						  
		{	
			static int prev = 0; //moved up...

			if(som_hacks_fade!=SOM::frame)
			{
				//static int prev = 0; //alpha component

				if(SOM::pcdown==SOM::frame
				//NEW: resetting to head off false positives			
				||som_hacks_fade<SOM::frame-1) prev = 0;

				//note: will paralyze the player
				//NEW: undoing below if tint is raised
				som_hacks_fade = SOM::frame;				
								
				//The change is 2 to 3 shades at 60 fps
				fade = prev!=cb[3]&&abs(prev-int(cb[3]))<16; //4 				
				prev = cb[3];
			}
			//Reminder: won't enter menus if fading
			else if(som_hacks_tint!=SOM::frame)
			{							   
				extern bool som_state_viewing_show;

				if(!som_state_viewing_show) //NEW
				//can also be c==0x64000000 when pausing
				//NEW: catching tint generated by an event
				//in the midst of an apparent fade (map open)
				//if(prev!=cb[3]){ assert(c==0x80000000); }else
				//black magic: patch for slideshows transitions
				if(SOM::context!=SOM::Context::viewing_picture)
				{
					if(prev!=cb[3]) //NEW: moving inside (bad idea?)
					{ 
						assert(c==0x80000000||c==0x64000000);
					}
					else if(w) w = bug; //bug: short-circuit
				}
			}
		}
		else if(som_hacks_dark!=SOM::frame)
		{
			if(SOM::field) //continue screen?
			{
				som_hacks_dark = SOM::frame; 

				if(DDRAW::psColorize)
				{	
					DDRAW::fxColorize|=c&0xFF000000;
					w = 0; //short-circuit
				}
				else //full screen fill
				{
					som_hacks_srcblend; //5
					som_hacks_destblend; //6
					
				}
			}
		}
		else if(w) w = bug; //bug: short-circuit

		bool tint = !dark&&!fade&&w;

		if(tint&&c==0x80000000) //menu tint		 
		{	
			SOM::black = SOM::frame;

			som_hacks_tint = SOM::frame; 
			som_hacks_fade = SOM::frame-6; //NEW

			if(bf->do_fix_clipping_item_display)
			{		
				//do_fix_zbuffer_abuse
				//DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_CLEAR_HACK,0);
				in->Clear(0,0,D3DCLEAR_ZBUFFER,0,1.0f,0);
				//DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_CLEAR_HACK,som_hacks_Clear);
			}

			DWORD bt = ad->blackened_tint;

			extern bool som_state_reading_text;
			if(0==(bt&0xFF000000)&&som_state_reading_text) //2023
			{
				bt = ad->blackened_tint2;
			}

			if(bt&0xFF000000) for(int i=0;i<4;i++) p[i].color = bt; 
			else w = 0; //short-circuit
		}
		else if(tint&&c==0x64000000) //tint tint
		{
			SOM::doubleblack = SOM::frame;
			som_hacks_fade = SOM::frame-6; //NEW
			DWORD bt2 = ad->blackened_tint2;
			//xr: it doesn't make of a difference in VR
			//and is expensive to draw
			if(bt2&0xFF000000&&!DDRAW::xr)
			for(int i=0;i<4;i++) p[i].color = bt2; 
			else w = 0; //short-circuit
		}

		goto fill; //return out;
	}
	else if((c&0x00FFFFFF)==0xFFFFFF) //white
	{
		//WTH: title screen
		if(cb[3]==0) w = 0; 

		//WTH: SOM applies these twice???
		if(som_hacks_fade!=SOM::frame)
		{
			som_hacks_fade = SOM::frame;
		}
		else if(w) w = bug; //bug: short-circuit

		goto fill; //return out;
	}
		
	int a = 0xFF; //color flashes
	
	if(!(c&0xFFFFFF00))
	{
		a = cb[0]; c = EX::INI::Adjust()->blue_flash_tint; 
	}
	else if(!(c&0xFFFF00FF)) 
	{
		a = cb[1]; c = EX::INI::Adjust()->green_flash_tint; 
	}
	else if(!(c&0xFF00FFFF)) 
	{
		a = cb[2]; c = EX::INI::Adjust()->red_flash_tint; 

		//This number tends to vary
		//It's probably time sensitive
		static const int flashes = 20; //18
			
		//NOTE: SOM::red wasn't reset until death???
		if(SOM::red&&SOM::frame-SOM::hpdown<=flashes) 			
		if(op->do_red) 
		{
			//2020: make poison relatively visible?
			WORD red = SOM::red;
			WORD hp = SOM::L.pcstatus[SOM::PC::_hp];
			if(SOM::red==1&&1&SOM::L.pcstatus[SOM::PC::xxx])
			{
				red = hp/5;
			}

			//2020: I have no idea how to properly integrate
			//critical_hit_point_quantifier at the moment :(
			//(needs an overhaul)
			//ad->red_saturation?
			//float o = SOM::Red(red,hp); //REMOVE ME
			float o = SOM::red2?SOM::red2:SOM::Red(red);
			if(o<1) cb[3]*=o;
		}
	}
	else //are flashes blended?
	{
		//No!		
		//Note: assert here crashes
		//assert(0); //heads up!
		//screen/support magic
		//2020: includes 0x64010101
		goto fill; //return out; 
	}

	float alpha = float(a)/0xFF*cb[3]/0xFF;

	if(alpha!=1) for(int i=0;i<3;i++) cb[i] = alpha*cb[i];

	if(c&0xFF000000&&c&0x00FFFFFF)
	{
		if(DDRAW::psColorize)
		{	
			BYTE *fx = (BYTE*)&DDRAW::fxColorize;

			for(int i=0;i<3;i++) fx[i] = min(int(fx[i])+cb[i],0xFF);

			w = 0; //short-circuit
		}
		else //full screen fill
		{
			som_hacks_srcblend; //1
			som_hacks_destblend; //1
			for(int i=0;i<4;i++) p[i].color = c;
		}
	}
	else w = 0; //short-circuit

	//fillrate optimization
	fill: if(w)
	{
		if(DDRAW::inStereo) if(DDRAW::xr)
		{
			DDRAW::vs = 15; //VR only

			auto &s = in->queryGL->stereo_views[0];

			p[0].sx = p[3].sx = s.x;
			p[0].sy = p[1].sy = s.y;
			p[1].sx = p[2].sx = s.x+s.w;
			p[3].sy = p[2].sy = s.y+s.h; //UNTESTED

			som_hacks_fill = SOM::frame+1;
		}
		else
		{
			//might be bad for the full screen effects
			//(will need to adjust UVs then, but how?)
			float clip = SOM::width/4-DDRAW::stereo*EX_INI_MENUDISTANCE*SOM::width/2;
			for(int i=0;i<4;i++) p[i].sx-=clip;
		}
	}

	return out;
}

extern bool som_scene_sky;
static void som_hacks_debug_lights();
static void som_hacks_debug_aabb(float*,float,float,bool debug=0);
static void som_hacks_switch_lamps(const float *center, float radius, float height)
{									   
	if(!som_hacks_lamps) return;

	if(!som_hacks_lamps_pool||!*som_hacks_lamps_pool) return;

	assert(DDRAW::Direct3DDevice7);

	if(!DDRAW::Direct3DDevice7) return;

	float sphere = max(radius,height); 

	D3DXVECTOR3 difference = 
	*(D3DXVECTOR3*)center-*(D3DXVECTOR3*)som_hacks_lamps_switch;

	float distance = D3DXVec3Length(&difference); 

	//completely inside current sphere if true
	if(distance+sphere<=som_hacks_lamps_switch[3]) 
	{	
		//is the current sphere swallowing up smaller ones?
		if(som_hacks_lamps_switch[3]>1.0f)
		{
			sphere = max(sphere,1.0f); //1m radius minimum

			//if current sphere is more than twice as large continue
			if(fabsf(som_hacks_lamps_switch[3]-sphere)<sphere)
			return;
		}
		else return;
	}
	else sphere = max(sphere,1.0f); //1m radius minimum
								   	
	assert(*som_hacks_lamps_pool<SOM_HACKS_LIGHTS-3);
	
	static DWORD ons[SOM_HACKS_LIGHTS]; *ons = 0;
	static DWORD offs[SOM_HACKS_LIGHTS]; *offs = 0;
	static float dists[SOM_HACKS_LIGHTS] = {-1,-1,-1}; //2017
		
	for(DWORD i=1;i<=*som_hacks_lamps_pool;i++) 
	{			   
		DWORD lamp = som_hacks_lamps_pool[i];	
		assert(som_hacks_lightfix[lamp].dltType==DX::D3DLIGHT_POINT);
		
		difference = *(D3DXVECTOR3*)center
		-*(D3DXVECTOR3*)&som_hacks_lightfix[lamp].dvPosition;
		distance = D3DXVec3Length(&difference); 

		if(distance-sphere<som_hacks_lightfix[lamp].dvRange)
		{
			dists[lamp] = distance; //2017

			if(!som_hacks_lightfix_on[lamp])
			{
				ons[++*ons] = lamp;					
			}
		}
		else if(som_hacks_lightfix_on[lamp])
		{
			offs[++*offs] = lamp;
		}
	}

	DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_LIGHTENABLE_HACK,0);

	//Note: better for DDRAW::lighting to turn lights off first

	for(size_t i=1;i<=*offs;i++)
	if(DDRAW::Direct3DDevice7->LightEnable(offs[i],0)==D3D_OK)
	{
		som_hacks_lightfix_on[offs[i]] = false;
	}
	//OVERFLOW? IMPORTANT COMES AFTER LIGHTS OUT	
	{
		struct _ //2017: Trying to handle overflow.
		{
			static int f(void *c, const void *a, const void *b)
			{
				return ((float*)c)[*(DWORD*)a]<((float*)c)[*(DWORD*)b]?1:-1;
			}
		};
		if((int)*ons+DDRAW::lights>DDRAW::maxlights)
		{
			//Must include existing lights in sort.
			//The map lights should already be among them.
			//Their distances should be -1, so at the very top.
			for(int i=0;i<DDRAW::lights;i++)
			ons[++*ons] = DDRAW::lighting[i];
			qsort_s(ons+1,*ons,sizeof(*ons),_::f,dists);
		}
	}
	for(size_t i=1;i<=*ons;i++) 
	if(DDRAW::Direct3DDevice7->LightEnable(ons[i],1)==D3D_OK)
	{			
		som_hacks_lightfix_on[ons[i]] = true;
	}

	DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_LIGHTENABLE_HACK,som_hacks_LightEnable);

	som_hacks_lamps_switch[0] = center[0];
	som_hacks_lamps_switch[1] = center[1];
	som_hacks_lamps_switch[2] = center[2];
	som_hacks_lamps_switch[3] = sphere;
	som_hacks_lamps_switch[4] = sphere;
}
extern DWORD som_hacks_primode = 0;
static void *som_hacks_DrawIndexedPrimitiveVA(HRESULT*hr, DDRAW::IDirect3DDevice7*&in, va_list va, void *vb)
{
	void *out = 0;
	
	//2022: debugging 413F10 crashes... I thought I found the
	//source in _transparent_indicesN but Moratheia is still
	//doing it... I wonder if its MSM data doesn't fit inside
	if(EX::debug&&*((DWORD**)va)[4]==0x5A7F68)
	{
		assert(*((DWORD**)va)[3]<=896);
		assert(*((DWORD**)va)[5]<=2688);				
	} 

	static unsigned inventory = 0;
	if(som_hacks_tint==SOM::frame&&inventory!=SOM::frame) //hack
	{
		inventory = SOM::frame; //todo: still necessary?? (YES? See note below)

		EX::INI::Player pc;

		//TODO
		//instead of 50, raise the item up higher, and pull it forward as necessary
		//to approximate 50, both WRT SOM::zoom and VR mode
		int todolist[SOMEX_VNUMBER<=0x1020406UL];
		//NOTE: 50 is the original value, but using 62 (kf2) just to scale down some
		//since 1m is a little bit in your face... in VR mode I think the value gets
		//overriden... 1m is chosen for VR. this branch is in case an extension puts
		//it in legacy mode		
		//
		//Not doing this since distance is made programmable
		//int zoom = 2==som_hacks_inventory[3][3]?50:62;
		int zoom = 62; //max(50,SOM::zoom); 
		float z = pc->player_character_radius3((float)som_hacks_inventory_item);
		som_hacks_inventory[3][3] = z?z:1;
		SOM::reset_projection(/*50*/zoom,3);
				
		//REMINDER: som_scene_4137F0 is unable to do this with picture menus
		{
			extern void som_scene_zwritenable_text();
			som_scene_zwritenable_text();
			//assert(som_hacks_zenable); //2021
			in->SetRenderState(DX::D3DRENDERSTATE_ZFUNC,DX::D3DCMP_LESSEQUAL);			
		}
		if(!SOM::filtering) //NEW: reserve point filter for 3D things
		{
			in->SetTextureStageState(0,DX::D3DTSS_MINFILTER,DX::D3DTFG_POINT);
			in->SetTextureStageState(0,DX::D3DTSS_MAGFILTER,DX::D3DTFG_POINT);
		}

		in->SetTransform(DX::D3DTRANSFORMSTATE_VIEW,(DX::D3DMATRIX*)som_hacks_inventory);						
	}

	//2017: trying to simplify clamping of menu elements
	if(som_hacks_texture_addressu!=DX::D3DTADDRESS_WRAP)
	in->SetTextureStageState(0,DX::D3DTSS_ADDRESS,DX::D3DTADDRESS_WRAP);
	
	static DWORD cachedop = 0; //REMOVE ME

	//2019: doing earlier for short-circuiting
	if(!hr) if(som_hacks_shader_model) 
	{
		//Moving this to below
		//if(sky) //NEW: backdrop
		{
			//DDRAW::vs = DDRAW::ps = 6;	
		}
		//else //if(fog<1.0f)
		{
			if(som_hacks_primode)
			{
				//DDRAW::vs = 4; //2023: fog?
				//DDRAW::vs = 5; //2023: sprite?
				DDRAW::vs = som_hacks_primode; 				
			}
			else if(som_hacks_alphaop!=DX::D3DTOP_DISABLE) 
			{	
				DDRAW::vs = 3;
			}				
			else //unlit
			{
				DDRAW::vs = 2;
			}

			extern int som_scene_volume; if(!som_scene_volume)
			{
				DDRAW::ps = DDRAW::vs;
			}
			else if(2<=som_scene_volume) //== (som.bsp.cpp)
			{
				//FIX ME: ps may happen to be 4 now but it shouldn't be
				//(som.bsp.cpp)

				bool q; if(DDRAW::ps!=4) //depth-write or volume render?
				{
					DDRAW::ps = 4; //depth-write //fog shader

					//RECURSIVE
					//draw back-faces to depth-buffer. this enables underwater
					//view (via full-screen effect) and self-contained volumes
					//NOTE: self-contained volumes must use a single draw call
					//or else this can't always be done back-to-back like this
					#define _(x) *va_arg(va,x*)
					if(vb==som_hacks_DrawIndexedPrimitiveVB)
					{
						//2022: untested? I think this is because "sequence-points"
						//oddly the arguments are all zeroes
						//in->DrawIndexedPrimitiveVB(x,y,z,w,q,r,s); 
						//2022: passing all zeroes??? (2-sided volume texture??)
					//	in->DrawIndexedPrimitiveVB(_(DX::D3DPRIMITIVETYPE),_(DDRAW::IDirect3DVertexBuffer7*),_(DWORD),_(DWORD),_(LPWORD),_(DWORD),_(DWORD));
						auto &_1 = _(DX::D3DPRIMITIVETYPE);
						auto &_2 = _(DDRAW::IDirect3DVertexBuffer7*);
						auto &_3 = _(DWORD);
						auto &_4 = _(DWORD);
						auto &_5 = _(LPWORD);
						auto &_6 = _(DWORD);
						auto &_7 = _(DWORD);
						in->DrawIndexedPrimitiveVB(_1,_2,_3,_4,_5,_6,_7);
					}
					else
					{
						//2022: untested? I think this is because "sequence-points"
						//oddly the arguments are all zeroes
						//in->DrawIndexedPrimitive(x,y,z,w,q,r,s);
					//	in->DrawIndexedPrimitive(_(DX::D3DPRIMITIVETYPE),_(DWORD),_(LPVOID),_(DWORD),_(LPWORD),_(DWORD),_(DWORD));
						auto &_1 = _(DX::D3DPRIMITIVETYPE);
						auto &_2 = _(DWORD);
						auto &_3 = _(LPVOID);
						auto &_4 = _(DWORD);
						auto &_5 = _(LPWORD);
						auto &_6 = _(DWORD);
						auto &_7 = _(DWORD);
						in->DrawIndexedPrimitive(_1,_2,_3,_4,_5,_6,_7);
					}
					#undef _
					va_start(va,in);

					DDRAW::ps = 8; //volume-texture

					/*this was the LPWORD parameter, but it's used as boolean
					q = 0; //back-door request to d3d9c to reuse index buffer*/
					q = false;
				}
				else q = true;

				if(2==som_scene_volume)
				{
					//190: D3DRS_COLORWRITEENABLE1 (reenable depth-texture write)
					//168: D3DRS_COLORWRITEENABLE
					in->SetRenderState((DX::D3DRENDERSTATETYPE)190,q?0xF:0);
					in->SetRenderState((DX::D3DRENDERSTATETYPE)168,q?0:0xF);
					//NOTE: SEEMS RELATED TO som_hacks_alphablendenable CODE FURTHER DOWN
					//I've disabled it since this effect requires shaders
					//I don't know if FAB is correct here but 1 sometimes doesn't enable transparency
					in->SetRenderState(DX::D3DRENDERSTATE_ALPHABLENDENABLE,q?0:1/*0xFAB*/);
					in->SetRenderState(DX::D3DRENDERSTATE_CULLMODE,q?DX::D3DCULL_CW:DX::D3DCULL_CCW);
				}
				else //som.bsp.cpp
				{
					if(!q) //short-circuit
					{
						DWORD &r = *((DWORD**)va)[5]; //HACK (index count)

						r = 0;
					}
				}
			}
			else DDRAW::ps = 8; //volume-texture
				
			extern bool som_scene_gamma_n; if(som_scene_gamma_n)
			{
				if(DDRAW::vshaders9[8]) DDRAW::vs = 8; //fixed lighting
			}
		}
		//else DDRAW::vs = DDRAW::ps = 4; //fog saturated		 			
			
		out = vb; //out = som_hacks_DrawIndexedPrimitiveVB;

		if(!DDRAW::doClearMRT)
		if(SOM::sky&&!som_hacks_alphablendenable) //legacy
		{			
			if(SOM::skystart<SOM::skyend) 
			som_hacks_fab_alphablendenable();
		}
	}	
	else if(som_hacks_alphaop!=cachedop) //!som_hacks_shader_model (legacy)
	{
		//REMINDER: this used to happen once per frame, until the addition of
		//transparent map elements (that som_scene_44D7D0 is responsible for) 

		cachedop = som_hacks_alphaop; //REMOVE ME?

		DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_SETRENDERSTATE_HACK,0);
			
		//enable lighting of map tiles
		if(som_hacks_alphaop==DX::D3DTOP_DISABLE) 
		{
			//REMOVE ME?
			//why is this desirable? //this is ancient code!!
			in->SetRenderState(DX::D3DRENDERSTATE_LIGHTING,1); 		
			in->SetRenderState(DX::D3DRENDERSTATE_AMBIENTMATERIALSOURCE,D3DMCS_COLOR1); 		
						
			D3DCOLOR cheat = 0xFAFFFFFF; //mark as false ambient

		//	if(!som_hacks_shader_model)
			{			
				if(SOM::gamma<8) som_hacks_level(cheat); //seems to help a lot
			}

			in->SetRenderState(DX::D3DRENDERSTATE_AMBIENT,cheat); //0xFAFFFFFF
			DX::D3DMATERIAL7 mat = {{0,0,0,1},{0,0,0,1},{0,0,0,0},{0,0,0,0},0};
			in->SetMaterial(&mat);		

			//and why is this being done again?
			/*this interferes with legacy shadows and calling som_scene_alphaenable(0)
			//prevents som_scene_413F10_maybe_flush from displaying the new transparent
			//map geometry
			som_hacks_alphablendenable = false;*/
		}
		else //return things to normal
		{
			in->SetRenderState(DX::D3DRENDERSTATE_AMBIENTMATERIALSOURCE,D3DMCS_MATERIAL); 		
			in->SetRenderState(DX::D3DRENDERSTATE_AMBIENT,som_hacks_ambient);
		}

		DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_SETRENDERSTATE_HACK,som_hacks_SetRenderState);
	}

	if(som_hacks_alphaop==DX::D3DTOP_DISABLE) //tile
	{
		//2022: trying to not set DDRAW::isLit above
	}
	else if(!DDRAW::isLit) //skies & shadows 
	{	
		//detect sky & fix swing model lighting bug

		DWORD w = *((DWORD**)va)[3]; //HACK	(vertex count)

		//KF2's moon has 4 vertex halo
		if(w!=4||som_scene_sky) //w!=4: filter out shadows
		{
			if(som_scene_sky) //NEW: unambiguous
			//if(som_hacks_distance(som_hacks_sky,y->m[3])<0.05f)
			{				
				DDRAW::vs = DDRAW::ps = 6; //overriding earlier setting

				//som_hacks_skycam(hr?-1:+1); //2022: moving to som.scene.cpp

				if(!hr&&som_hacks_shader_model //legacy
				 &&!som_hacks_alphablendenable) //skyfloor
				{
					int todolist[SOMEX_VNUMBER<=0x1020406UL]; //and fog powers?
					if(EX::INI::Option()->do_alphafog)
				//	if(EX::INI::Detail()->alphafog_skyflood_constant>0) //blend?
					som_hacks_fab_alphablendenable();	
				}	
			}
			else if(!hr) //assuming unlit swing model (bug)
			{
				//assert(0); //2022: should be obsolete
				int todolist[SOMEX_VNUMBER<=0x1020406UL];

				in->SetRenderState(DX::D3DRENDERSTATE_LIGHTING,1); 

				goto lit; //2022
			}
		}													 
	
		if(som_hacks_lamps&&DDRAW::lights+DDRAW::lights_killed) 
		{			
			if(!hr) DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_LIGHTENABLE_HACK,0);

			if(!hr) DDRAW::killing_lights(som_hacks_lightfix_on); 

			out = vb; //som_hacks_DrawIndexedPrimitiveVB;		

			if(hr) DDRAW::reviving_lights(som_hacks_lightfix_on); 
			
			if(hr) DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_LIGHTENABLE_HACK,som_hacks_LightEnable);

			som_hacks_ditch_lamps(); 
		}
	}
	else lit: //DDRAW::isLit
	{
		assert(!som_scene_sky);
		
		extern float *som_scene_lit; //xyz,radius,height	

		if(!hr&&som_scene_lit)
		som_hacks_switch_lamps(som_scene_lit,som_scene_lit[3],som_scene_lit[4]);
	}
			
	//STUDY ME
	//2020: I think som.shader.cpp uses the fog's color
	//som_scene_volume's recursive use of this function
	//interferes with "static bool enable". I've made a
	//note to see if som.shader.cpp blends colors based
	//functions correctly with respect to fog
	//if(!som_hacks_shader_model||!DDRAW::pshaders9[8]) 
	if(!DDRAW::pshaders9[8]) //2022
	if(!som_scene_sky&&EX::INI::Option()->do_alphafog) //OVERKILL
	{
		static bool enable = false; //REMOVE ME

		if(!hr) //enable alpha fog			
		{
			//REMINDER: this seems to cause problems for som_scene_volume 
			if(!som_hacks_fogenable)
			if(som_hacks_alphablendenable&&!som_hacks_fab) //NEW: fogless maps?
			{	
				in->SetRenderState(DX::D3DRENDERSTATE_FOGENABLE,1);

				//2021: disabling som.key.h
				//if(som_hacks_key_image //shadowfog
				//&&som_hacks_key_image->corrected==som_hacks_shadow_image)
				if(*(void**)0x1D3D2F0==DDRAW::TextureSurface7[0])
				{										
					//2020: "fab" seems to be at odds with
					//the text for !som_hacks_fab above! 
					//som_hacks_fab_alphablendenable(); //shadow with alpha channel											
					//som_hacks_fab = false; //HACK!
					//NOTE: this blends with spawning (momentarily transparent) monsters
   					som_scene_alphaenable(1);

					in->SetRenderState(DX::D3DRENDERSTATE_FOGCOLOR,som_hacks_foglevel); 
				}
				else in->SetRenderState(DX::D3DRENDERSTATE_FOGCOLOR,som_hacks_fogalpha); 

				enable = true; out = vb; //som_hacks_DrawIndexedPrimitiveVB;				
			}			
		}
		else if(enable) //alpha fog is enabled
		{
			in->SetRenderState(DX::D3DRENDERSTATE_FOGENABLE,0);
			in->SetRenderState(DX::D3DRENDERSTATE_FOGCOLOR,som_hacks_foglevel);

			enable = false;
		}
	}

	return out;
}
//static void *som_hacks_DrawIndexedPrimitive(HRESULT*hr,DDRAW::IDirect3DDevice7*in,DX::D3DPRIMITIVETYPE&x,DWORD&y,LPVOID&z,DWORD&w,LPWORD&q,DWORD&r,DWORD&s)
static void *som_hacks_DrawIndexedPrimitive(HRESULT*hr,DDRAW::IDirect3DDevice7*in,...)
{
	va_list va; va_start(va,in);
	return som_hacks_DrawIndexedPrimitiveVA(hr,in,va,som_hacks_DrawIndexedPrimitive);
}
//static void *som_hacks_DrawIndexedPrimitiveVB(HRESULT*hr,DDRAW::IDirect3DDevice7*in,DX::D3DPRIMITIVETYPE&x,DDRAW::IDirect3DVertexBuffer7*&y,DWORD&z,DWORD&w,LPWORD&q,DWORD&r,DWORD&s)
static void *som_hacks_DrawIndexedPrimitiveVB(HRESULT*hr,DDRAW::IDirect3DDevice7*in,...)
{
	va_list va; va_start(va,in);
	return som_hacks_DrawIndexedPrimitiveVA(hr,in,va,som_hacks_DrawIndexedPrimitiveVB);
}

/*disabling som.keys.h
static void *som_hacks_SetTexture(HRESULT*,DDRAW::IDirect3DDevice7*in,DWORD&,DX::LPDIRECTDRAWSURFACE7&y)
{	
	som_hacks_texture = y; //2022: OOPS... LOOKS LIKE THIS WAS IMPORTANT :(

	DDRAW::IDirectDrawSurface7 *p = DDRAW::is_proxy<DDRAW::IDirectDrawSurface7>(y);

	const SOM::KEY::Image *old = som_hacks_key_image; 
	const SOM::KEY::Image *img = som_hacks_key_image = 0;

	if(p&&p->query->clientdata)
	EX::DATA::Get(img,p->query->clientdata,false);

	if(!img)
	{
		if(old)
		{
			if(old->addressu)
			in->SetTextureStageState(0,DX::D3DTSS_ADDRESSU,som_hacks_texture_addressu);
			if(old->addressv)
			in->SetTextureStageState(0,DX::D3DTSS_ADDRESSV,som_hacks_texture_addressv);
			//REMINDER: PRETTY SURE THIS DOESN'T WORK WITH SHADERS ENABLED
			if(old->lodbias)
			in->SetTextureStageState(0,DX::D3DTSS_MIPMAPLODBIAS,0);
		}
		
		return 0;
	}

	//REMINDER: PRETTY SURE THIS DOESN'T WORK WITH SHADERS ENABLED
	if(img->lodbias||old&&old->lodbias)
	{
		in->SetTextureStageState(0,DX::D3DTSS_MIPMAPLODBIAS,img->lodbias);
	}

	while(old) //optimization
	{
		if(old==img) return 0; 

		if(img->addressu!=old->addressu) break;
		if(img->addressv!=old->addressv) break;

		som_hacks_key_image = img;

		return 0; //equivalent
	}
	while(!old) //optimization
	{
		if(img->addressu&&img->addressu!=som_hacks_texture_addressu) break;
		if(img->addressv&&img->addressv!=som_hacks_texture_addressv) break;

		som_hacks_key_image = img;

		return 0; //equivalent
	}

	DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_SETTEXTURESTAGESTATE_HACK,0);

	if(!old&&img->addressu||old&&img->addressu!=old->addressu)
	in->SetTextureStageState(0,DX::D3DTSS_ADDRESSU,!img->addressu?som_hacks_texture_addressu:img->addressu);
	if(!old&&img->addressv||old&&img->addressv!=old->addressv)
	in->SetTextureStageState(0,DX::D3DTSS_ADDRESSV,!img->addressv?som_hacks_texture_addressv:img->addressv);
	
	DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_SETTEXTURESTAGESTATE_HACK,som_hacks_SetTextureStageState);

	som_hacks_key_image = img;

	return 0;
}*/

static void *som_hacks_SetTextureStageState(HRESULT*,DDRAW::IDirect3DDevice7 *in,DWORD &x,DX::D3DTEXTURESTAGESTATETYPE &y, DWORD &z)
{
	//static int magfilter = 0; 	

	EX::INI::Option op;

	//TESTING (2020)
	//Since "Bilinear" is basically a useless option
	//I'd like it if it mixed a point mag filter with
	//linear min/mip filter with anisotropic filtering
	//or a lightweight shader solution
	bool midfilter = 0&&EX::debug&&SOM::filtering==1&&!op->do_anisotropy; 
							   
	switch(y)
	{
	case DX::D3DTSS_COLOROP: 

		som_hacks_colorop = z; break;

	case DX::D3DTSS_MIPFILTER:
		/*THIS CODE ALWAYS SETS TO POINT/POINT/D3DTFP_POINT (after 402C00)
		0042341D 6A 01                push        1  
		0042341F 6A 10                push        10h  
		00423421 6A 00                push        0  
		00423423 50                   push        eax  
		00423424 FF 91 94 00 00 00    call        dword ptr [ecx+94h]  
		0042342A 8B 06                mov         eax,dword ptr [esi]  
		0042342C 8B 10                mov         edx,dword ptr [eax]  
		0042342E 6A 01                push        1  
		00423430 6A 11                push        11h  
		00423432 6A 00                push        0  
		00423434 50                   push        eax  
		00423435 FF 92 94 00 00 00    call        dword ptr [edx+94h]
		0042343B 8B 36                mov         esi,dword ptr [esi]  
		0042343D 8B 06                mov         eax,dword ptr [esi]  
		0042343F 6A 02                push        2  
		00423441 6A 12                push        12h  
		00423443 6A 00                push        0  
		00423445 56                   push        esi  
		00423446 FF 90 94 00 00 00    call        dword ptr [eax+94h] 
		*/
		/*switch(z)
		{
		default:
		case DX::D3DTFP_NONE:
		case DX::D3DTFP_POINT: 
			
			SOM::filtering = magfilter; break; 

		case DX::D3DTFP_LINEAR: SOM::filtering = 2; break;
		}*/

		//2018: not working?? or 402C00?
		//I don't understand this... SOM specifically has mode 3 for mipmaps		
		if(SOM::filtering!=2)
		{
			z = DX::D3DTFP_NONE; //assert(z==DX::D3DTFP_NONE); //D3DTFP_POINT?

			if(midfilter) z = DX::D3DTFP_LINEAR; //TESTING (2020)
		}
		else
		{
			//mode 1 looks just like mipmapping, because of D3DTEXF_ANISOTROPIC?

			z = DX::D3DTFP_LINEAR; //assert(z==DX::D3DTFP_LINEAR); //D3DTFP_POINT?			
		}		
		break;
	
	case DX::D3DTSS_MINFILTER: assert(z<=2);

		if(SOM::filtering!=0) //improves menus without do_mipmaps
		{
			z = DX::D3DTFN_LINEAR; //assert(z==DX::D3DTFN_LINEAR);
		}

		if(DDRAW::xr)
		{
			//stereoMode is mega-supersampling, and I figure
			//since VR is high-resolution it doesn't need 16
			DDRAW::anisotropy = SOM::stereoMode?4:8;
		}
		else if(DDRAW::doSuperSampling) 
		{
			//this is hard coded since this helps performance 
			//a lot and I can't see any difference personally
			DDRAW::anisotropy = //4
			0&&EX::debug?16:EX::INI::Detail()->aa_supersample_anisotropy;

			//2022: I haven't tested this, but I figure that
			//the 1.5x supersampling may not be enough for VR
			//WRT anisotropic filtering... might even want 16x
			if(DDRAW::inStereo&&4==DDRAW::anisotropy)
			{
				int test = 1; DDRAW::doSuperSamplingMul(test);
				if(test<2) DDRAW::anisotropy = 8;
			}
		}
		else if(op->do_anisotropy)
		{
			switch(SOM::filtering)
			{
			case 0: DDRAW::anisotropy =  4; break;
			case 1: DDRAW::anisotropy =  8;	break;
			case 2: DDRAW::anisotropy = 16; break;
			}	

			//set anisotropy in case min filter is already set
			//in->SetTextureStageState(0,DX::D3DTSS_MAXANISOTROPY,DDRAW::anisotropy);
		}
		else if(midfilter) //TESTING (2020)
		{
			DDRAW::anisotropy = 1; //0
			 
			z = DX::D3DTFN_LINEAR; //DX::D3DTFN_POINT;
		}
		else DDRAW::anisotropy = 2!=SOM::filtering; //2018 (1 disables)

		break;

	case DX::D3DTSS_MAGFILTER: assert(z<=2);

		//menu color seems degraded without do_mipmaps????
		if(0!=SOM::filtering) 
		{			
			//NOT FIXING THE PROBLEM
			//2018: enable linear filter in menus?
			//assert(z==DX::D3DTFG_LINEAR); //D3DTFG_POINT
			if(!SOM::emu) z = DX::D3DTFG_LINEAR; 
		}

		if(midfilter) z = DX::D3DTFG_POINT; //TESTING (2020)

		//HACK: signal to menus need to switch filter to linear
		//while in point filter mode
		som_hacks_magfilter = 0xf&z; 

		break;

	case DX::D3DTSS_ADDRESSU: 

		som_hacks_texture_addressu = z;	
		/*disabling som.keys.h
		if(som_hacks_key_image&&som_hacks_key_image->addressu) 
		z = som_hacks_key_image->addressu;*/
		break;

	case DX::D3DTSS_ADDRESSV: 

		som_hacks_texture_addressv = z; 		
		/*disabling som.keys.h
		if(som_hacks_key_image&&som_hacks_key_image->addressv) 			
		z = som_hacks_key_image->addressv;*/
		break;

	case DX::D3DTSS_ADDRESS: 

		som_hacks_texture_addressu = som_hacks_texture_addressv = z; 		
		
		/*disabling som.keys.h
		if(som_hacks_key_image) 
		{
			DWORD u = som_hacks_key_image->addressu;
			DWORD v = som_hacks_key_image->addressv;

			if(!u) u = som_hacks_texture_addressu;
			if(!v) v = som_hacks_texture_addressv;
			
			if(u!=v) //tricky
			{
				DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_SETTEXTURESTAGESTATE_HACK,0);

				in->SetTextureStageState(0,DX::D3DTSS_ADDRESSU,u);

				DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_SETTEXTURESTAGESTATE_HACK,som_hacks_SetTextureStageState);

				y = DX::D3DTSS_ADDRESSV; z = v;
			}
			else z = u;
		}*/

		break;

	case DX::D3DTSS_ALPHAOP: //2022

		som_hacks_alphaop = z; break;
	}	

	return 0;
}

static void *som_hacks_LightEnable(HRESULT*,DDRAW::IDirect3DDevice7 *p, DWORD &x, BOOL &y)
{	
	if(som_hacks_lamps)
	{
		if(x>=127) //closing map or reseting device one
		{	
			som_hacks_lights_detected_map_close = true;			
		}

		if(x>2) //leave all lamps on for lifetime of map
		{
			x = -1; y = 0; //ignore SOM's pleas
		}
	}
	
	if(som_hacks_lightfix_on)
	{
		if(x<SOM_HACKS_LIGHTS) som_hacks_lightfix_on[x] = y;
	}

	return 0;
}

static void *som_hacks_CreateVertexBuffer(HRESULT*hr,DDRAW::IDirect3D7*in,DX::LPD3DVERTEXBUFFERDESC&x,DX::LPDIRECT3DVERTEXBUFFER7*&y,DWORD&)
{
	//2022: these are menu/sprite vbuffers that are
	//never used, but still allocate 4096 vertices
	//(which is too much for menus/sprites anyway)
	if(y==(void*)0x1d6a28c||y==(void*)0x1d6a22c) 
	{
		assert(!*y); y = 0; return 0; //short-circuit
	}

	bool mpx = y==(void*)0x59892C;

	if(!hr)
	{
		//som.scene.cpp has to remove DDLOCK_DISCARDCONTENTS
		//from the MSM buffer because it's partially locked
		if(!mpx) x->dwCaps|=D3DVBCAPS_WRITEONLY;
		else x->dwCaps = 0; //YUCK: restore
				
		//YUCK: restore 4096 for remaining buffers
		if(x->dwFVF!=274)
		{
			x->dwNumVertices = 4096; 
			
			return mpx?som_hacks_CreateVertexBuffer:0;
		}
	}
	else if(mpx) //2023
	{
		som_scene::ubuffers[0] = *y;
		for(int i=som_scene::vbuffersN;i-->1;)
		in->CreateVertexBuffer(x,som_scene::ubuffers+i,0);

		return 0;
	}

	//TODO: make this 4096 even if these extensions
	//aren't in order
//	if(!som_scene::vbuffer_size) return 0;

	if(!hr)
	{
		x->dwNumVertices = som_scene::vbuffer_size;
	}
	else if(!*hr&&!som_scene::vbuffers[0]) //RECURSIVE
	{
		som_scene::vbuffers[0] = *y;
		for(int i=som_scene::vbuffersN;i-->1;)
		in->CreateVertexBuffer(x,som_scene::vbuffers+i,0);
	}

	return som_hacks_CreateVertexBuffer;
}

static void *som_hacks_mipmaps_saturate(const DX::DDSURFACEDESC2 *in, DX::DDSURFACEDESC2 *out)
{
	enum{ bytes=4 };
	
	const float sat = EX::INI::Detail()->mipmaps_saturate;

	//I'm not sure this is the best formula
	//const float magic[3] = {0.222f*255,0.707f*255,0.071f*255};
	const float magic[3] = {0.222f,0.707f,0.071f};
	//I wonder about precision loss
	const float magic255[3] = { 0.00087058632f,0.00277254292f,0.00027843076f};

	//NOTE: this is modifying "in" even though technically it's
	//expected to be treated as a "const" input (source) image
	//(I've even removed the D3DLOCK_READONLY flag even though
	//it seems to work fine either way being D3DPOOL_SYSTEMMEM)
	auto n = (BYTE*)in->lpSurface;
	int npitch = in->lPitch;	
	int nwidth = in->dwWidth;
	int nheight = in->dwHeight;
	for(int row=0;row<nheight;row++)
	{
		BYTE *sample = (BYTE*)n+npitch*row; 

		for(int col=0;col<nwidth;col++,sample+=bytes)
		{
			float linear[3]; //2022
			extern const WORD Ex_mipmap_s8l16[256];
			for(int i=3;i-->0;) linear[i] =
			Ex_mipmap_s8l16[sample[i]]*0.000015259021896696f; //1/65545.0f

			float dp = 0;
			for(int i=0;i<bytes-1;i++) //dot product
			{
				//dp+=sample[i]/255.0f*magic[i];
			//	dp+=sample[i]*magic255[i];
				dp+=linear[i]*magic[i];
			}
			for(int i=0;i<bytes-1;i++) //lerp
			{
				//float lerp = dp+(sample[i]/255.0f-dp)*sat;
			//	float lerp = dp+(sample[i]*0.0039215627f-dp)*sat;
				float lerp = dp+(linear[i]-dp)*sat;
			//	sample[i] = max(0,min(255,(int)(lerp*255+0.5f)));
				float l = max(0.0f,min(1.0f,lerp));
				extern BYTE Ex_mipmap_LinearToSRGB8(WORD);
				sample[i] = Ex_mipmap_LinearToSRGB8((WORD)(l*65535));
			}
		}
	}

	if(out) //som_hacks_mipmaps_saturate2 short-circuit?
	{
		if(!EX::mipmap(in,out)) return 0;
	}
	return EX::mipmap;
}
extern void *Ex_mipmap_pointfilter(const DX::DDSURFACEDESC2 *in, DX::DDSURFACEDESC2 *out);
static void *som_hacks_mipmaps_pixel_art_power_of_two(const DX::DDSURFACEDESC2 *in, DX::DDSURFACEDESC2 *out)
{
	enum{ bytes=4 }; assert(in->dwDepth==0); 

	/*currently forcing point filter 
	extern int Ex_mipmap_kaiser_sinc_levels; //WIP
	if(Ex_mipmap_kaiser_sinc_levels)
	{
		if(!EX::mipmap(in,out)) return 0;
	}
	else*/ if(!Ex_mipmap_pointfilter(in,out)){ assert(0); return 0; }

	if(0x0000ff00!=in->ddpfPixelFormat.dwGBitMask) return EX::mipmap;

	//parameters to Ex_mipmap_mipmap (reversed)
	auto n = (BYTE*)in->lpSurface;
	int npitch = in->lPitch;	
	int nwidth = in->dwWidth;
	int nheight = in->dwHeight;
    auto m = (BYTE*)out->lpSurface;
	int mpitch = out->lPitch;
	int mwidth = out->dwWidth;	
	int mheight = out->dwHeight;

	const float l_w = EX::INI::Detail()->mipmaps_pixel_art_power_of_two;
	const float w = 1-l_w, w_4 = w/4;

	//something should be optimal for alpha
	//0.66666 and 0.75 look very good... 0.75 is a little choppier, 0.5
	//is not as good. 0.66666f is good enough unless a problem comes up
	const float l_a = 0.66666f;
	const float a = 1-l_a, a_4 = a/4;

	BYTE *end = m+mpitch*mheight;
	BYTE *above = end-mpitch;
	BYTE *below = m;

	int leftmost = bytes*mwidth-bytes;

	int sRGB = DDRAW::sRGB&&som_hacks_shader_model; //2020
	if(sRGB&&l_w!=1) sRGB = 2;

	//this needs a lot more work, but it should 
	//be getting moved into x2mdl.dll shortly
	int todolist[SOMEX_VNUMBER<=0x1020406UL];
	//
	// 2022: this is mode 3 below that's designed
	// to deemphasize diagonal pixels
	// 	 
	//if(1&&EX::debug&&sRGB==2) sRGB = 3; //TESTING
	if((1||!EX::debug)&&sRGB==2) sRGB = 3; //TESTING
	//0.4 eliminates the problem patterns but makes
	//everything else too square, which I think may
	//be too great a sacrifice
	//0.45 is a good starting point
	//0.5 may be pushing it
	//
	// NOTE: Merrel Ur's armor's decal in KF2 is an
	// example where diagonal pixels are unbearable
	//
	const float bias = 0.475f; //0.45f; //ARBITRARY
	const float w1 = w_4*bias, w3 = w_4*(1+(1.0f-bias)/3);
	const float a1 = a_4*bias, a3 = a_4*(1+(1.0f-bias)/3);

	for(int row=0;row<nheight;row++)
	{
		BYTE *sample = (BYTE*)n+npitch*row; 

		int left = leftmost, right = 0;

		for(int col=0;col<nwidth;col++)
		{
			BYTE *kernel[4] = 
			{
				above+left,above+right,
				below+left,below+right
			};

			extern BYTE Ex_mipmap_sRGB(DWORD);
			extern const WORD Ex_mipmap_s8l16[256];
			extern BYTE Ex_mipmap_LinearToSRGB8(WORD lin);

			switch(sRGB)
			{
			case 0:

				for(int i=0;i<3;i++)
				{
					float sum = 0;

					sum+=kernel[0][i];
					sum+=kernel[1][i];
					sum+=kernel[2][i];
					sum+=kernel[3][i];
				
					sum*=w_4;
					sum+=l_w*sample[i];
					
					assert(sum>=0&&sum<256);

					sample[i] = 0xFF&int(sum);
				}
				break;

			case 1:
			
				for(int i=0;i<3;i++)
				{
					union{ DWORD dw; BYTE bt[4]; };
					bt[0] = kernel[0][i];
					bt[1] = kernel[1][i];
					bt[2] = kernel[2][i];
					bt[3] = kernel[3][i];
					sample[i] = Ex_mipmap_sRGB(dw);
				}
				break;
			
			case 2:
			
				for(int i=0;i<3;i++)
				{
					//NOTE: one of these should be the same
					//as sample[i]
					DWORD suw = 0;
					suw+=Ex_mipmap_s8l16[kernel[0][i]];
					suw+=Ex_mipmap_s8l16[kernel[1][i]];
					suw+=Ex_mipmap_s8l16[kernel[2][i]];
					suw+=Ex_mipmap_s8l16[kernel[3][i]];
				
					float sum = suw*w_4+l_w*Ex_mipmap_s8l16[sample[i]];

					sample[i] = Ex_mipmap_LinearToSRGB8((WORD)sum);
				}
				break;
		
			case 3: //TESTING (2022)
			{
				BYTE *k1,*k2,*k3;	

				switch((row&1)<<1|col&1)
				{
				break; case 0:
					
					k1 = kernel[0]; k2 = kernel[1];					
					k3 = kernel[2]; 

				break; case 1:

					k2 = kernel[0]; k1 = kernel[1];					
								    k3 = kernel[3]; 
				break; case 2:

					k2 = kernel[0]; 
					k1 = kernel[2]; k3 = kernel[3]; 

				break; case 3:
								    k2 = kernel[1]; 
					k3 = kernel[2]; k1 = kernel[3]; 
				}			

				for(int i=0;i<3;i++)
				{					
					//here I'm trying to reduce sampling on the 
					//diagonal across pixel to try to eliminate
					//artifacts

					DWORD sam = Ex_mipmap_s8l16[sample[i]];

					float sum = w3*sam+l_w*sam; 					
					sum+=w1*Ex_mipmap_s8l16[k1[i]];
					sum+=w3*Ex_mipmap_s8l16[k2[i]];
					sum+=w3*Ex_mipmap_s8l16[k3[i]];

					sample[i] = Ex_mipmap_LinearToSRGB8((WORD)sum);
				}

				if(bytes==4) //alpha?
				{
					float sum = a3*sample[3]+l_a*sample[3];

					sum+=a1*k1[3]+a3*k2[3]+a3*k3[3];
					
					assert(sum>=0&&sum<256);

					sample[3] = 0xFF&int(sum);
				}
				goto post_alpha;
			}}
			
			//if(i==3) //TESTING
			if(bytes==4) //alpha?
			{
				float sum = 0;

				sum+=kernel[0][3];
				sum+=kernel[1][3];
				sum+=kernel[2][3];
				sum+=kernel[3][3];
				
				sum*=a_4;
				sum+=l_a*sample[3];
					
				assert(sum>=0&&sum<256);

				sample[3] = 0xFF&int(sum);

			}post_alpha:;

			sample+=bytes;

			if(col%2==0)
			{
				left+=bytes;
				if(left>=mpitch) left = 0;
				right+=bytes;
				if(right>=mpitch) right = 0;
			}
		}

		if(row%2==0)
		{
			above+=mpitch;
			if(above>=end) above = m;
			below+=mpitch;
			if(below>=end) below = m;
		}
    }

	return EX::mipmap;
}
static void *som_hacks_mipmaps_saturate2(const DX::DDSURFACEDESC2 *in, DX::DDSURFACEDESC2 *out)
{
	//HACK: 0 is short-circuiting the mipmapping function 
	//since this is only "saturating" in (via const-cast)
	som_hacks_mipmaps_saturate(in,0); 
	return som_hacks_mipmaps_pixel_art_power_of_two(in,out);
}
static void *som_hacks_CreateSurface7(HRESULT *hr,DDRAW::IDirectDraw7*,DX::LPDDSURFACEDESC2&x,DX::LPDIRECTDRAWSURFACE7*&,IUnknown*&)
{		
	if(hr) //2021: mipmap_f remembers mipmap
	{
		//NOTE: instead of this could just set mipmap_f directly
		//at this point

		DDRAW::mipmap = EX::mipmap; return 0;
	}
	if(!DDRAW::mipmap) //som_status_automap? //2021
	{
		//HACK: in som_status_automap I had to turn off mipmaps
		//because Ex_mipmap can't handle odd sized mipmaps, and
		//the automap doesn't require minification. 
		//
		//A DOWNSIDE IS IT CAN'T BE SUBJECT TO mipmaps_saturate  
		//BUT I DON'T THINK IT NEEDS IT PROBABLY. I'M NOT SURE
		//INTERFACE ELEMENTS SHOULD BE SATURATED IN THE FIRST
		//PLACE
		return 0; 
	}
	assert(DDRAW::mipmap==EX::mipmap||!DDRAW::mipmap);

	extern char *som_game_449530_cmp;
	extern char *som_game_trim_a(char*);
	
	if(~x->ddsCaps.dwCaps&DDSCAPS_TEXTURE) return 0; //???

	EX::INI::Option op; EX::INI::Detail dt;

	//sRGB and filtering in general bleeds 
	//the life out of color
	bool saturate = 1!=dt->mipmaps_saturate;
	if(saturate)
	DDRAW::mipmap = som_hacks_mipmaps_saturate;
	
	int w = x->dwWidth, h = x->dwHeight; 

	//avoid map_icon.bmp and upscaling menu elements doesn't matter because of
	//mipmapping, although it might on higher than 1080 displays, in that case
	//it's undesirable
	auto cmp = som_game_449530_cmp;

	extern int Ex_mipmap_kaiser_sinc_levels; //WIP
	
	if(!cmp)
	{
		assert(!Ex_mipmap_kaiser_sinc_levels);

		return som_hacks_CreateSurface7; //som_status_automap? (map_icon.bmp?)
	}

	cmp = som_game_trim_a(cmp);

	//2021: point filtering the mipmaps is slightly less blurry
	//and is stable unlike doing point-filtering in the sampler
	//TODO: maybe expose control over this via another extension
	Ex_mipmap_point_filter = 0!=dt->mipmaps_pixel_art_power_of_two; //EXTENSION?

	if(w>512||h>512 //picture?
	 ||!strnicmp(cmp,"my/texture/",11)
	 ||!strnicmp(cmp,"data\\menu\\",10))
	//the art system now locates map textures in here (can't tell them apart)
	//why was sky disabled anyway?
//	 ||!strnicmp(cmp,"data\\map\\model\\",15)) //sky?
	{
		//itemicon.bmp and arrows need filtering since
		//you will often see them as mipmaps
		if(w<512&&h<512) Ex_mipmap_point_filter = 0;

		Ex_mipmap_kaiser_sinc_levels = 0;
		
		return som_hacks_CreateSurface7; 
	}

	if(int x=dt->mipmaps_kaiser_sinc_levels)
	if(DDRAW::sRGB&&!som_hacks_shader_model)		
	Ex_mipmap_kaiser_sinc_levels = x;
	
	double f = dt->mipmaps_pixel_art_power_of_two;
	int p = (int)f;
	if(f!=p)
	{
		p++;

		if(p==1) 
		if(DDRAW::mipmap==som_hacks_mipmaps_saturate)
		{
			DDRAW::mipmap = som_hacks_mipmaps_saturate2;
		}
		else DDRAW::mipmap = som_hacks_mipmaps_pixel_art_power_of_two;
	}
	if(p>2) p = 2;

	p = 1<<p;

	//NOTE: the relevant code starts at 449b8f. SetStretchBltMode
	//can be manipulated but I think either HALFTONE doesn't work
	//or it does perfect nearest-neighbor sampling if the upscale
	//dimensions are a multiple of the original's. And then there
	//is colorkey to consider and that filtering might defeat the
	//purpose of enlargement altogether!
		
	x->dwWidth = p*w; x->dwHeight = p*h;

	return som_hacks_CreateSurface7;
}

/*2020: som_game_44c100/som_game_11_menu41e100 implement this
static void *som_hacks_SetVolume(HRESULT*,DSOUND::IDirectSoundBuffer*in,LONG&x)
{
	if(in==DSOUND::PrimaryBuffer) //SOM::altf12?
	{
		return 0;
	}
	else if(SOM::se_volume) //se/se3D?
	{
		x+=SOM::se_volume; SOM::se_volume = 0; //HACK

		return 0;
	}

	//Options menu
	int vol = 16-int(-x/10000.0f*16); 

	if(in->in3D) //REMOVE US
	{	
		#error this no longer holds (2020)

		//NOTE: the volume level is always
		//the same. I'm not sure if the 3D
		//samples have their own volume or
		//not

		SOM::seVol = vol*255/16.0f; //HACK 	

		static LONG dx = 1;
		if(dx<=0) DSOUND::Sync(0,x-dx); 		
		dx = x;
	}
	else
	{
		SOM::bgmVol = vol*255/16.0f; //HACK

		static LONG dx = 1;
		if(dx<=0) DSOUND::Sync(x-dx,0); 		
		dx = x;
	}

	//Ex.output.cpp
	SOM::decibels[in->in3D?1:0] = float(x)/100;
	//SOM::volume[in->in3D?1:0] = vol; //UNUSED

	return 0;
}*/

static void *som_hacks_SetOrientation(HRESULT*,DSOUND::IDirectSound3DListener*,DX::D3DVALUE&x,DX::D3DVALUE&y,DX::D3DVALUE&z,DX::D3DVALUE&t,DX::D3DVALUE&o,DX::D3DVALUE&p,DWORD&)
{
	return 0;

//	if(SOM::L.pcstate) 
	{
		t = 0; o = 1; p = 0; //let Direct Sound do this

		x = SOM::pov[0]; y = SOM::pov[1]; z = SOM::pov[2]; 
	}
/*	else //old way (won't work if Direct Sound fails)
	{
		//REMINDER: som_db/rt.exe report a max/min up/down
		//view angle of 60 degrees where elsewhere it is 45

		SOM::pov[0] = x; SOM::pov[1] = y; SOM::pov[2] = z;
		
		float x_z[] = {x,0,z};
		float north[] = {0,0,1}, east[] = {1,0,0}, up[] = {0,1,0}; 		
		float testing = SOM::uvw[0];

		SOM::uvw[1] = som_hacks_radians(north,x_z,east); //compass 	
		SOM::uvw[0] = som_hacks_radians(x_z,SOM::pov,up); //incline
	}
*/
	return 0;
}				

static void *som_hacks_SetVelocity(HRESULT*,DSOUND::IDirectSound3DListener*,DX::D3DVALUE&x,DX::D3DVALUE&y,DX::D3DVALUE&z,DWORD&)
{
	//REMINDER: som_db/rt.exe report unreliable values
	//Also their velocities are unaffected by obstacles

	x = SOM::doppler[0]; y = SOM::doppler[1]; z = SOM::doppler[2];

	return 0;
}

static void *som_hacks_SetPosition(HRESULT*,DSOUND::IDirectSound3DListener*,DX::D3DVALUE&x,DX::D3DVALUE&y,DX::D3DVALUE&z,DWORD&)
{		
//	if(SOM::L.pcstate)
	{
		/*I think it's still too soon for this and footsteps should
		//be adjusted instead, maybe weapon sounds too
		//2020: shouldn't this include height?		
		x = SOM::xyz[0]; y = SOM::xyz[1]; z = SOM::xyz[2];
		x = SOM::cam[0]; y = SOM::cam[1]; z = SOM::cam[2];*/
		x = SOM::cam[0]; y = SOM::xyz[1]; z = SOM::cam[2];

		//push NPC sounds up half the PC's height to be more centered
		//this is splitting the difference between typical footsteps
		//and sounds originating from head height
		//NOTE: som_mocap_footstep_soundeffect and maybe arm sounds
		//will need to compensate for this change
		y+=(SOM::cam[1]-y)/2;
		
	}
/*	else //old way (won't work if Direct Sound fails)
	{
		SOM::xyz[0] = x; SOM::xyz[1] = y; SOM::xyz[2] = z;		
	}
*/
	return 0;
}

static BOOL CALLBACK som_hacks_enumjoysticks_cb(DX::LPCDIDEVICEINSTANCEA lpddi, LPVOID pvRef)
{
	DX::DIDEVICEINSTANCEA cp; memcpy(&cp,lpddi,lpddi->dwSize);

	//TOODO: use Unicode name converted to UTF-8
	sprintf_s(cp.tszProductName,"PadID: %s",lpddi->tszInstanceName);

	HRESULT out = ((DX::LPDIENUMDEVICESCALLBACKA)pvRef)(&cp,0);

	return out;
}
									
static void *som_hacks_EnumDevices_diA(HRESULT*,DINPUT::IDirectInputA*in,DWORD&x,DX::LPDIENUMDEVICESCALLBACKA&y,LPVOID&z,DWORD&w)
{
	assert(!z); //Reminder: the Controls menu now depends upon this one

	if(!z&&x==DIDEVTYPE_JOYSTICK)
	{
		if(!EX::INI::Joypad())
		{
			z = y; y = som_hacks_enumjoysticks_cb; 
		}
		else y = 0; //short circiut
	}

	return 0;
}

static const size_t som_hacks_controls_s = 32;

static DINPUT::IDirectInputDeviceA *som_hacks_controls[som_hacks_controls_s] = 
{
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, //paranoia
};

static void *som_hacks_CreateDevice_diA(HRESULT*hr,DINPUT::IDirectInputA*in,REFGUID x,DX::LPDIRECTINPUTDEVICEA*&y,LPUNKNOWN&)
{
	if(!hr) 
	{
		assert(*y==0); //2017 (testing what?)
		*y = 0; //testing
		return som_hacks_CreateDevice_diA; 
	}
	
	if(*hr||!y||!*y) return 0;
	
	DINPUT::IDirectInputDeviceA *p = DINPUT::is_proxy<DINPUT::IDirectInputDeviceA>(*y);

	if(!p||!strcmp(p->product,"Keyboard")) return 0; //GUID_SysKeyboard

	static int i = 0; //hack
	if(i<som_hacks_controls_s) 
	{		
		//p->AddRef();		
		//hack: assuming instantiated once and only once
		som_hacks_controls[i++] = p; 
	}

	return 0;
}

//2020: Probably a better way to do this?	
static bool som_hacks_compare(int bit, bool st)
{
	static int cmp; bit = 1<<bit;

	bool ret = false; if(st)
	{
		if(~cmp&bit) ret = true; cmp|=bit;
	}
	else cmp&=~bit; return ret;
}
static void *som_hacks_GetDeviceState(HRESULT*hr,DINPUT::IDirectInputDeviceA*in,DWORD&x,LPVOID&y)
{		
	static void *Y = 0;

	static BYTE kb[256]; 

	static DX::DIJOYSTATE js; 
	
	if(!hr)
	{		
		Y = y;

		if(in==DINPUT::Keyboard) y = kb; 
		if(in==DINPUT::Joystick) y = &js; 

		return som_hacks_GetDeviceState;
	}
	else if(*hr)
	{
		//disrupts hot plugging (unplugging)
		//assert(hr&&!*hr);
		//assert(*hr==0x8007001e);
		//2020: saw 0x8007000c (INVALID_ACCESS) unplugging controller
		assert(*hr==0x8007001e||*hr==0x8007000c);
	}
	
	if(som_hacks_pcstated)	
	{
		som_hacks_pcstated = false;
		memcpy(SOM::L.pcstate,som_hacks_pcstate,sizeof(float)*6);
	}
		
	EX::INI::Bugfix bf;

	if(y==kb)	
	if(bf->do_fix_asynchronous_input)		
	{
		static unsigned sync = 0;

		switch(SOM::context) //bugs
		{
		case SOM::Context::saving_game:
		case SOM::Context::taking_item:
		case SOM::Context::reading_menu:		
		//there is a pause when leaving the
		//shop, but this doesn't seem to help
		//case SOM::Context::browsing_shop:

			if(sync>=SOM::frame) break; //==

		default: //falling thru

			((DWORD*)&SOM::L.controls)[-1] = 0x00000000;
		}

		sync = SOM::frame;
	}
	
	//note: this isn't especially precise
	const int EX_context = EX::context(); //???
	bool playing_game = 0==EX_context;	
	if(som_hacks_fade>=SOM::frame-2)
	playing_game = false;
	if(kb[0xC5]||kb[0x0F]||SOM::paused) //2020: unpausing? menu?
	playing_game = false;
	if(!playing_game&&SOM::g) *SOM::g = 0; //2020: unpausing? menu?

	if(y==&js)
	{
		if(playing_game)
		{	
			//if(SOM::analogMode) 
			{
				js.lX = js.lY = 0; //paralyze
			}
		}
	}
										  
	//if(SOM::L.control_) //nice
	{
		int i = *SOM::L.control_;

		if(i>=0&&i<som_hacks_controls_s)
		{
			//allow config without being required to 
			//exit the menu after changing controllers
					   
			if(som_hacks_controls[i])
			if(som_hacks_controls[i]!=DINPUT::Joystick2)
			{			
				//assuming these will never be released!
				DINPUT::Joystick2 = som_hacks_controls[i];
			}
		}
	}

	//hack: just configuration now
	if(y==&js&&!EX::INI::Joypad()) 
	{
		EX::Joypad &jp = EX::Joypads[0];
				
		//todo: this some place else
		if(in->instance!=jp.instance) 
		{	
			jp.activate(0,in->instance,in->productID);			
			//jp.detectXInputDevices();
			som_hacks_detectXInputDevices = true;

			EX::INI::Detail dt; //todo: this some other way
			jp.assign_gaits_if_default_joypad(dt->escape_analog_gaits);			
		}
		
		if(SOM::paused)
		{
			//HACK: take screenshot using 
			//controller Select/Back button 		
			//REMINDER: BUTTONS DISABLED 2 BLOCKS DOWN
			int select = 9;
			if(jp.isDualShock) select = 8;
			if(jp.isXInputDevice) select = 6;
			if(js.rgbButtons[select]&&!SOM::recording)
			{
				//reminder: scroll+control=pause
				if(~GetKeyState(VK_CONTROL)>>15) 			
				{
					keybd_event(VK_SCROLL,0,0,0);
					keybd_event(VK_SCROLL,0,KEYEVENTF_KEYUP,0);
				}		
			}
		}
		else if(1==EX_context) //2017
		{
			//2022: this condition is unrelated but is needed
			//to prevent up/down input in the minimal KF mode
			if(!SOM::Game.taking_item&&!EX::is_playing_movie())
			{
				//HACK: handle Up/Down wraparound inside menus
				//EX::dbgmsg("js.lY %d",js.lY);
				enum{ mid=5120/2 }; //or 2500?
				if(js.lY>=+mid) EX::Syspad.send(0x50);
				if(js.lY<=-mid) EX::Syspad.send(0x4C);			
				js.lY = 0;
			}
		}
		
		//MUST WE KEEP DOING THIS EACH AND EVERY POLL???
		memset(jp.cfg.buttons,0x00,sizeof(jp.cfg.buttons));

		if(SOM::padtest&&SOM::padcfg>=SOM::frame-1)
		{			
			//disable pause/escape when discovering buttons
			
			/*for(int i=0;i<16;i++) //assuming zeroed out
			{
				if(jp.cfg.buttons[i][1]==0xC5 //PAUSE
				 ||jp.cfg.buttons[i][1]==0x01) //ESCAPE
				{
					jp.cfg.buttons[i][1] = 0; 
				}
			}*/

			return 0;
		}

		//allow movie cancel	
		jp.cfg.buttons[0][2] = 0x01; //ESCAPE
		

		//2021: trying to let action/menu buttons be used
		//inside menus
		int o = SOM::L.controls[3]%8;
		int x = SOM::L.controls[2]%8;

		if(playing_game
		//FIX: paused is for X-Box controllers
		//they use button 7 for a Start button
		||SOM::paused||SOM::recording) for(int i=0;i<8;i++)
		{
			js.rgbButtons[i] = 0; //paralyze

			//%8: part of a unique binding scheme??

			int j = SOM::L.controls[i]%8; //padCfg[i]
		
			switch(i) //OVERKILL: configure the buttons
			{				
			//for menu key prediction down below
			case 2: jp.cfg.buttons[j][0] = 0x0F; break; //TAB

			case 4: jp.cfg.buttons[j][0] = 0x4B; break; //NUMPAD4
			case 5: jp.cfg.buttons[j][0] = 0x4D; break; //NUMPAD6
			case 6: jp.cfg.buttons[j][0] = 0x49; break;	//NUMPAD9
			case 7: jp.cfg.buttons[j][0] = 0x47; break; //NUMPAD7

			//may as well do all of these really
			case!0: jp.cfg.buttons[j][0] = 0x2A; break; //LSHIFT
			case!1: jp.cfg.buttons[j][0] = 0x1D; break; //LCONTROL
			case 3: jp.cfg.buttons[j][0] = 0x39; break; //SPACE

			default: jp.cfg.buttons[j][0] = 0;
			}
		}
		else //HACK: menu inputs
		{
			if(jp.isDualShock)
			{
				std::swap(js.rgbButtons[0],js.rgbButtons[2]);
				std::swap(js.rgbButtons[0],js.rgbButtons[1]);
			}

			//2020: standardize menu inputs
			for(int i=1;i<EX::contexts-1;i++)
			{
				//for(int j=4;j-->0;) //already done above?
				//jp.cfg.buttons[j][i] = 0; //Control Panel processing
				jp.cfg.pov_hat[0][i] = 0x4C; //UP
				jp.cfg.pov_hat[1][i] = 0x51; //RIGHT
				jp.cfg.pov_hat[2][i] = 0x50; //DOWN
				jp.cfg.pov_hat[3][i] = 0x4F; //LEFT
				jp.cfg.pov_hat[4][i] =				
				jp.cfg.pov_hat[5][i] =
				jp.cfg.pov_hat[6][i] =
				jp.cfg.pov_hat[7][i] = 0; //diagonal

				if(o>3&&x>3) //2021
				{
					jp.cfg.buttons[o][i] = 0x39;
					jp.cfg.buttons[x][i] = 0x01;
				}
			}
		}
		EX::INI::Button bt;
		for(int i=1;i<=16;i++) //if(bt) //Pause/Select
		{
			//2020: I don't think these can be pressed
			//but this is what the code above requires
			//NOTE: 6 covers buttonSwap
			int n = i>6?EX::contexts-1:1; 
			for(int j=0;j</*EX::contexts-1*/n;j++)
			{
				if(bt->buttons[i][j]) jp.cfg.buttons[i-1][j] = bt->buttons[i][j];
			}
		}
		if(bt) for(int i=0;i<4;i++)
		{
			int d = bt->pseudo_button(i*9000);

			if(d<bt->buttons_s) for(int j=0;j</*EX::contexts-1*/1;j++)
			{
				if(bt->buttons[d][j]) jp.cfg.pov_hat[i][j] = bt->buttons[d][j];
			}
		}

		//2020: negating disables just in case it annoys somebody
		//(Ex.input.cpp ISN'T MANAGING js FOR CONTEXT SWITCHING PURPOSES)
		int bswap = SOM::buttonSwap; if(bswap>=0)
		{
			//2020: Not sure where to sequence this?
			//TODO: Is context-switching managed at all for the joypad?			
			bool limbo = SOM::frame-SOM::limbo<=10; //HACK

			//2021: try to use the buttons not mapped to action/menu?
			int bt0 = 4, bt1 = 5; if(o>3&&x>3)
			{	
				int mask = 1<<o|1<<x;
				switch(mask)
				{
				case 1<<4|1<<5: bt0 = 6; break;
				case 1<<6|1<<7: bt0 = 4; break;
				default:
				for(int i=4;i<=7;i++) if(~mask&1<<i)
				{
					bt0 = i; break;
				}}
				mask|=1<<bt0;
				bt1 = ~mask>>4&0xF;
				bt1 = bt1==8?7:bt1/2+4;
			}
			
			for(int i=0;i<=1;i++)
			if(som_hacks_compare(i,js.rgbButtons[i?bt1:bt0]||limbo))
			{
				//doesn't always work???
				//if(!playing_game&&SOM::frame-SOM::limbo>10) //HACK
				if(!playing_game&&!limbo) //HACK
				switch(EX::context()) //NEW: pressing in fade in effect
				{
				case 1: case 2:

					if(SOM::player) //HACK: "watching game"
					{
						//bswap^=1+i; //see below comments
						bswap^=2<<i; SOM::altf|=1<<16+i;

						SOM::buttonSwap = bswap; //2020
					}
				}
			}
		}
		else bswap = -abs(bswap); if(!playing_game) 
		{
			//2020: &1 is skipped because -0 isn't a negative number
			if(bswap&2) std::swap(js.rgbButtons[0],js.rgbButtons[1]);
			if(bswap&4) std::swap(js.rgbButtons[2],js.rgbButtons[3]);
		}
	}
			
	if(y==kb) //keyboard (called first by 40f250)
	{	
		if(DDRAW::isPaused) //PAUSED?
		if(DDRAW::inStereo)
		{		
			//TESTING: Not sure when/where to do this???
			SOM::PSVRToolbox();
		}

		unsigned now = EX::tick();
		static unsigned time = now; //!
		unsigned diff = now-time; time = now; 				
		/*if(1==EX_ARRAYSIZEOF(SOM::onflip_triple_time)) 
		{
			SOM::motions.diff = diff;
			SOM::motions.tick = now;			
		}*/
				
		static unsigned flip = 0, ppf = 0;		 

		if(flip!=SOM::frame)
		{	
			flip = SOM::frame; ppf = 0;		  

			int i, j;
			for(i=1,j=0;i<256;i++)
			{
				if(kb[i]) EX::keys[j++] = i;
			}

			EX::keys[j] = 0;
		}																		 

		if(ppf<256)
		{
			EX::polls[ppf++] = max(diff,1); //REFACTOR ME
			EX::polls[ppf] = 0;
		}	
				
		//standardize input
		const unsigned char or[] = 
		{
			0x4C,0xC8, //5|=UP
			0x50,0xD0, //2|=DOWN
			0x51,0xCD, //3|=RIGHT
			0x4F,0xCB, //1|=LEFT
			0x47,0xD3, //7|=DELETE
			0x49,0xD1, //9|=NEXT	
			0x48,0xCF, //8|=END			
			0x4B,0,0x4D,0 //2021: lateral triggers?
		};assert(!kb[0]); //dx.dinput.cpp zeroes it
		//crosstalk between controllers/mouse
		//makes half-gaits a complicating factor		
		for(int i=0;i<sizeof(or);i+=2)
		{
			//kb[0x4C]|=kb[0xC8]; kb[0xC8]=0; //5|=UP
			unsigned char &a = kb[or[i]], &b = kb[or[i+1]];		
			a = EX::Joypad::analog_select(a,b); b = 0; 
		}

		if(!SOM::analogMode) //routed through keyboard
		{
		kb[0x4C] = EX::Joypad::analog_dpad(kb[0x4C]);
		kb[0x50] = EX::Joypad::analog_dpad(kb[0x50]);
		kb[0x51] = EX::Joypad::analog_dpad(kb[0x51]);
		kb[0x4F] = EX::Joypad::analog_dpad(kb[0x4F]);
		}
					
		//eliminate half-gaits if there's not complementary gaits
		//Ex.input.cpp doesn't have enough information to do this
		{
			const char bts[8] = {0x4B,0x4C,0x4D,0x50, 0x4F,0x51,0x47,0x49};
			int max_gait = INT_MIN;
			for(int i=0;i<4;i++) if(kb[bts[i]]!=0)
			max_gait = max(max_gait,EX::Joypad::gaitcode(kb[bts[i]]));
			if(-10==max_gait)
			for(int i=0;i<4;i++) kb[bts[i]] = 0;
			max_gait = INT_MIN;
			for(int i=4;i<8;i++) if(kb[bts[i]]!=0)
			max_gait = max(max_gait,EX::Joypad::gaitcode(kb[bts[i]]));
			if(-10==max_gait)
			for(int i=4;i<8;i++) kb[bts[i]] = 0;
		}

		static DWORD combo4c50 = 0, combo4749 = 0;
		static DWORD combo4f51 = 0, combo4b4d = 0;

		if(kb[0x4C]==0x80&&kb[0x50]==0x80) //NUMPAD5+NUMPAD2
		{
			combo4c50 = now; kb[0x48]|=0x80; 
		}
		if(kb[0x47]==0x80&&kb[0x49]==0x80) //NUMPAD7+NUMPAD9
		{
			combo4749 = now; kb[0x48]|=0x80; 
		}
		if(kb[0x48]&0x80) //NUMPAD8
		{
			EX::Mouse.absolute[1] = 0.0f; //center vertically		
		}
		if(kb[0x4F]==0x80&&kb[0x51]==0x80) //NUMPAD1+NUMPAD3
		{
			combo4f51 = now;
			EX::Mouse.absolute[0] = 0.0f; //center horizontally
		}
		if(kb[0x4B]==0x80&&kb[0x4D]==0x80) //NUMPAD4+NUMPAD6
		{
			combo4b4d = now;
			EX::Mouse.absolute[0] = 0.0f; //center horizontally
		}		
		if(combo4c50>now-100)
		if(kb[0x4C]==0x80||kb[0x50]==0x80)
		{
			kb[0x4C]&=~0x80; kb[0x50]&=~0x80; //down/up
		}
		if(combo4749>now-100)
		if(kb[0x47]==0x80||kb[0x49]==0x80)
		{
			kb[0x47]&=~0x80; kb[0x49]&=~0x80; //delete/next
		}
		if(combo4f51>now-100)
		if(kb[0x4F]==0x80||kb[0x51]==0x80)
		{
			kb[0x4F]&=~0x80; kb[0x51]&=~0x80; //left/right
		}
		if(combo4b4d>now-100)
		if(kb[0x4B]==0x80||kb[0x4D]==0x80)
		{
			kb[0x4B]&=~0x80; kb[0x4D]&=~0x80; //F4 mode???
		}
		
		//SCHEDULED OSBOLETE
		if(SOM::u) *SOM::u = 1;
		if(SOM::v) *SOM::v = 1;
		memset(SOM::u2,0x00,sizeof(SOM::u2));
		if(playing_game) 
		{
			//TAB: entering menu?
			bool menu = kb[0x0F]; 
			//won't work: but the Menu button is rerouted
			//through EX::Joypad[0] when using analogMode
			//if(!menu&&DINPUT::Joystick&&SOM::L.controls)
			//{
			//	menu = js.rgbButtons[SOM::L.controls[2]%8];
			//}		
			//0: no input during the fading effect
	//		if(SOM::frame-som_hacks_fade<2) diff = 0;
			//60: there is a long disruptive pause 
			//when entering into some of the menus
			/*2022: this freezes if frame rate is actually
			//low (it blocks input) (I'm hoping what
			//this is meant to fix has resolved itself)
			if(!menu&&diff<60)*/
			if(!menu)
			{	
				//drives the player character model
				SOM::motions.ready_config(diff,kb); 
			}
			else 
			{
				kb[0x1D] = kb[0x2A] = 0; //2021

				/*if(0&&!menu)
				if(SOM::g&&!*SOM::g) //2022: freezing?
				{
					*SOM::g = 0.000001f;  //gzero
				}*/
			}
		}
		else kb[0x1D] = kb[0x2A] = 0; //2021

		//REMOVE ME?		
		float u2[3] = //not pretty...
		{
		kb[0x4D]&0x80?1:kb[0x4B]&0x80?-1:0,
		kb[0x1E]&0x80?1:kb[0x2C]&0x80?-1:0,
		kb[0x4C]&0x80?1:kb[0x50]&0x80?-1:0,
		};
		for(int i=0;i<3;i++) SOM::u2[i] = 
		SOM::u2[i]?SOM::u2[i]*u2[i]:u2[i];
													
		//hiding 
		if(!kb[0x3E] //F4
		||~SOM::altf&1<<4) //2018: better
		kb[0x38] = kb[0xB8] = 0; //LMENU/RMENU
		kb[0x3B] = kb[0x3C] = 0; //F1/F2	
		kb[0x3D] = 0; //F3 (som_state_00408A80_RIP)

		//2020: swapping so shift is left button
		std::swap(kb[0x1D],kb[0x2A]);
	}
	if(SOM::Game.taking_item)
	{
		//2022: disable up/down if KF style examine
		//option is on (SOM examine display is off)
		bool kf_style = !SOM::L.on_off[3];
		if(y==kb)
		if(kf_style) kb[0x47] = kb[0x49] = 0;
		if(y==&js)
		{
			if(kf_style) js.lY = 0;

			//toggle text? (left button)
			//toggle menu? (right button)
			if(js.rgbButtons[2]&0x80) EX::Syspad.send(0x22);
			if(js.rgbButtons[3]&0x80) EX::Syspad.send(0x24);
		}
		if(kf_style) *(DWORD*)0x19AB328 = 0; //Yes? //422d80
	}

	if(y==kb) 
	{
		memcpy(Y,kb,256);
	}
	else if(y==&js) 
	{			
		//REMOVE ME
		if(!SOM::context) //limbo?
		{
			//HACK: THIS SEEMS LIKE A BANDAID ... Ex.input.cpp
			//DOESN'T WORK WITH js DIRECTLY, AND DOESN'T HAVE A
			//CONFIGURATION FOR MENU INPUTS AS NEAR AS I CAN SEE

			//2020: this fixes a longtime bug with leaking 
			//the cancel button out of the menu
			memset(js.rgbButtons,0x00,sizeof(js.rgbButtons));
		}
		else if(0==EX::context()) //menus, etc?
		{
			//2021: this prevents attacking/firing magic when
			//entering menus while holding these buttons
			memset(js.rgbButtons,0x00,sizeof(js.rgbButtons));
		}

		memcpy(Y,&js,sizeof(js));
	}	
	else assert(0); return 0;
}
			   
static void som_hacks_level(D3DCOLOR &c)
{
	if(DDRAW::doGamma) return;

	float black = 0.0f;

	D3DCOLORVALUE cv = { 0xFF&(c>>16), 0xFF&(c>>8), 0xFF&c, 0xFF&(c>>24) };

	if(SOM::gamma>8)
	{
		float lv = DDRAW::brights[DDRAW::bright];

		cv.r = max(min(cv.r+(1.0f-cv.r/255.0f)*lv,255.0f),black);
		cv.g = max(min(cv.g+(1.0f-cv.g/255.0f)*lv,255.0f),black);
		cv.b = max(min(cv.b+(1.0f-cv.b/255.0f)*lv,255.0f),black);
	}
	else if(SOM::gamma<8)
	{
		float lv = float(DDRAW::dimmers[DDRAW::dimmer])/255.0f;

		cv.r = max(cv.r+cv.r*-lv,black);
		cv.g = max(cv.g+cv.g*-lv,black);
		cv.b = max(cv.b+cv.b*-lv,black);
	}
	else
	{
		cv.r = max(cv.r,black);
		cv.g = max(cv.g,black);
		cv.b = max(cv.b,black);
	}

	c = DWORD(cv.a)<<24|DWORD(cv.r)<<16|DWORD(cv.g)<<8|DWORD(cv.b);
}

extern void som_hacks_skycam(float x)
{
//	if(SOM::L.view_matrix_xyzuvw) //&&EX::output_overlay_f[12]) //testing
	{		
		DX::D3DMATRIX &cam = (DX::D3DMATRIX&)(x>0?SOM::steadycam:SOM::analogcam);

		DDRAW::Direct3DDevice7->SetTransform(DX::D3DTRANSFORMSTATE_VIEW,&cam);
	}
/*	else if(SOM::steadycam[3][1]) //bob (old way)
	{
		DX::D3DMATRIX world;

		if(DDRAW::Direct3DDevice7->GetTransform(DX::D3DTRANSFORMSTATE_WORLD,&world)==D3D_OK)
		{
			world._42+=x*SOM::steadycam[3][1]; 			

			DDRAW::Direct3DDevice7->SetTransform(DX::D3DTRANSFORMSTATE_WORLD,&world);
		}
	}
*/
}

static void som_hacks_debug_aabb(float *center, float radius, float height, bool debug)
{
	if(DDRAW::target!='dx9c'||!DDRAW::Direct3DDevice7->proxy9) return;
	
	::IDirect3DDevice9 *d3dd9 = DDRAW::Direct3DDevice7->proxy9;

	d3dd9->SetVertexShader(0); d3dd9->SetPixelShader(0);

	d3dd9->SetRenderState(D3DRS_LIGHTING,0);
//	d3dd9->SetRenderState(D3DRS_AMBIENT,0xFFFFFFFF);
	d3dd9->SetRenderState(D3DRS_FILLMODE,D3DFILL_WIREFRAME);
	d3dd9->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG2);
	
	D3DCOLOR red = 0xFFFF0000, green = 0xFF00FF00, blue = 0xFF0000FF; 

	if(debug) red = green = blue = 0xFFDD7700;

	D3DXVECTOR3 x = *(D3DXVECTOR3*)center;
	
	D3DXVECTOR3 a, b, c, d, i, j, k, l; //caps of a cube

	a = i = x+D3DXVECTOR3(+radius,height,+radius); i.y-=height*2;
	b = j = x+D3DXVECTOR3(-radius,height,+radius); j.y-=height*2;
	c = k = x+D3DXVECTOR3(-radius,height,-radius); k.y-=height*2;
	d = l = x+D3DXVECTOR3(+radius,height,-radius); l.y-=height*2;
	
	float lines[12*2*4] = 
	{	//top cap
		a.x,a.y,a.z,*(float*)&red, b.x,b.y,b.z,*(float*)&red,
		b.x,b.y,b.z,*(float*)&red, c.x,c.y,c.z,*(float*)&red,
		c.x,c.y,c.z,*(float*)&red, d.x,d.y,d.z,*(float*)&red,
		d.x,d.y,d.z,*(float*)&red, a.x,a.y,a.z,*(float*)&red,
	 	//legs (cap to cap)
		a.x,a.y,a.z,*(float*)&green, i.x,i.y,i.z,*(float*)&green,
		b.x,b.y,b.z,*(float*)&green, j.x,j.y,j.z,*(float*)&green,
		c.x,c.y,c.z,*(float*)&green, k.x,k.y,k.z,*(float*)&green,
		d.x,d.y,d.z,*(float*)&green, l.x,l.y,l.z,*(float*)&green,
	 	//bottom cap
		i.x,i.y,i.z,*(float*)&blue, j.x,j.y,j.z,*(float*)&blue,
		j.x,j.y,j.z,*(float*)&blue, k.x,k.y,k.z,*(float*)&blue,
		k.x,k.y,k.z,*(float*)&blue, l.x,l.y,l.z,*(float*)&blue,
		l.x,l.y,l.z,*(float*)&blue, i.x,i.y,i.z,*(float*)&blue,
	};

	d3dd9->SetFVF(D3DFVF_XYZ|D3DFVF_DIFFUSE);
	d3dd9->DrawPrimitiveUP(D3DPT_LINELIST,12,lines,4*sizeof(float));

	d3dd9->SetTextureStageState(0,D3DTSS_COLOROP,som_hacks_colorop);
	d3dd9->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);	
//	d3dd9->SetRenderState(D3DRS_AMBIENT,som_hacks_ambient);
	d3dd9->SetRenderState(D3DRS_LIGHTING,DDRAW::isLit);
}

void som_hacks_debug_lights()
{
	if(!som_hacks_lamps_pool) return;

	for(size_t i=1;i<=*som_hacks_lamps_pool;i++)
	{
		DX::D3DLIGHT7 &l = som_hacks_lightfix[som_hacks_lamps_pool[i]];

		float *p = (float*)&l.dvPosition; float r = l.dvRange*1.5;

		som_hacks_debug_aabb(p,0.1,0.1);
		som_hacks_debug_aabb(p,r,r);
	}
}

extern void som_hacks_shadowfog(DDRAW::IDirectDrawSurface7*);
/*2021: disabling along with som.keys.h (see notes in its file)
static bool som_hacks_fix(DDRAW::IDirectDrawSurface7*,const SOM::KEY::Image*,const wchar_t*);
static void som_hacks_correct(DDRAW::IDirectDrawSurface7 *in, const SOM::KEY::Image *image)
{
	if(!in||!image||!image->correct) return;

	EXLOG_LEVEL(3) << "CORRECTING: " << image->correct << '\n';

	image->corrected = 0; int depth = 0; //parenthesis

	for(const wchar_t *p=image->correct;*p;p++) switch(*p)
	{
	case 's': //shadow()

		if(*p=='s')
		if(!memcmp(p,L"shadow",6*sizeof(wchar_t)))
		if(!p[6]||p[6]==' '||p[6]=='\t'||p[6]=='(')
		{
			image->corrected = som_hacks_shadow_image; //bogus

			if(DDRAW::compat=='dx9c') //hack
			{
				//2018 New Years: standalone_game_shadowfog_hack is triggering this???
				//assert(!in->queryX->knockouts); //colorkey

				in->queryX->knockouts = 0; //colorkey = 0;
			}
		}

	case 'f': //fix(<filename of image to substitute>)

		if(*p=='f')
		if(!memcmp(p,L"fix",3*sizeof(wchar_t)))
		if(p[3]=='(')
		{
			const wchar_t *q = p+4, *d = q;
			for(depth=0;*d&&(*d!=')'||depth);d++) 
			if(*d=='(') depth++; else if(*d==')') depth--;			
			if(depth==0)
			if(!som_hacks_fix(in,image,SOM::KEY::genpath(q,d)))
			som_hacks_fix(in,image,SOM::KEY::genpath2(q,d));			
		}

	case 'c': //colorkey(0)

		if(*p=='c')
		if(!memcmp(p,L"colorkey",8*sizeof(wchar_t)))
		if(p[8]=='(')
		{
			const wchar_t *q = p+9, *d = q;

			for(depth=0;*d&&(*d!=')'||depth);d++) 
			{
				if(*d=='(') depth++; else if(*d==')') depth--;
			}

			if(*q=='0'&&d-q==1)
			{
				//wipe colorkey flags
				in->queryX->descriptorflags = 0; //overkill	   
				delete[] in->queryX->colorkey;
				in->queryX->colorkey = 0;
			}
		}

	default: //skip forward to next function 

		while(*p==' '&&*p=='\t') p++;

		for(depth=1;*p&&depth;p++) switch(*p)
		{
		default: break;

		case '(': depth++; break;
		case ')': depth--; break;

		case ' ': case '\t':

			if(depth==1) 
			{
				while(*p==' '&&*p=='\t') p++; p--; depth--; 
			}
		}

		if(!*p) p--;
	}		

	if(!image->corrected) 
	{
		image->corrected = som_hacks_correct_image; //bogus
	}
	else if(image->corrected==som_hacks_shadow_image) //equally bogus
	{  
		if(EX::INI::Option()->do_alphafog) som_hacks_shadowfog(in);		
	}
}
static bool som_hacks_fix
(DDRAW::IDirectDrawSurface7 *in, const SOM::KEY::Image *image, const wchar_t *file)
{	
	if(!in||!file) return false;

	if(!DDRAW::Direct3DDevice7) return false;

	const DWORD lockflags = D3DLOCK_NO_DIRTY_UPDATE|D3DLOCK_READONLY|D3DLOCK_NOSYSLOCK; //D3DLOCK_DISCARD

	DWORD U = D3DX_FILTER_MIRROR_U, V = D3DX_FILTER_MIRROR_V;
	
	if(!image||!image->addressu||image->addressu==D3DTADDRESS_WRAP) U = 0;
	if(!image||!image->addressv||image->addressv==D3DTADDRESS_WRAP) V = 0;

	HRESULT ok = !D3D_OK; IDirect3DTexture9 *out9 = 0;

	switch(DDRAW::target)		   
	{
	case 'dxGL':
	case 'dx95': assert(!'dx95'); //UNFINISHED?

	default: return false;

	case 'dx9c':
		
		ok = D3DXCreateTextureFromFileExW
		(
			DDRAW::Direct3DDevice7->proxy9,
			file,
			D3DX_DEFAULT, //width
			D3DX_DEFAULT, //height
			0, //full mipmapping
			0, //usage
			D3DFMT_A8R8G8B8, //D3DFMT_UNKNOWN, 
			D3DPOOL_SYSTEMMEM, //D3DPOOL_DEFAULT
			//gen quality mipmaps if needed
			D3DX_FILTER_TRIANGLE|U|V, 
			D3DX_FILTER_TRIANGLE|U|V,
			0, //no colorkey
			0, //no feedback
			0, //no palette
			&out9
		);
		break;
	}

	if(ok==D3D_OK)
	{
		if(out9)
		{
			IDirect3DSurface9 *p = 0;				
			if(out9->GetSurfaceLevel(0,&p)==D3D_OK)
			{
				//int dicey = 
				//in->clientrefs-1; 
				//DDRAW::referee(in,-dicey);
				assert(in->clientrefs==1);
				{
					//ensure device is released
					in->proxy9->Release(); in->proxy9 = p;
					if(in->query9->group9) in->query9->group9->Release(); 
					else assert(0); in->query9->group9 = out9;					
					in->query9->palette = 0;
				}
				//DDRAW::referee(in,+dicey);				

				D3DLOCKED_RECT lock; 	 						
				if(!p->LockRect(&lock,0,lockflags)) 
				in->query9->pitch = lock.Pitch;					
				else in->query9->pitch = 0;					
				p->UnlockRect();

				if(in->query9->update9) //forcing
				{
					//NOTE: 1|2 should be passed if
					//SetColorKey is involved
					//D3D9C::updating_texture(in,true);
					in->updating_texture();
				}
				if(in==DDRAW::TextureSurface7[0])
				DDRAW::Direct3DDevice7->SetTexture(0,in);					
			}
			else
			{
				out9->Release(); return false;
			}
		}
	}
	return true;
}*/
static void som_hacks_shadowfog(DDRAW::IDirectDrawSurface7 *in)
{
	if(DDRAW::compat!='dx9c'||in->queryX->format!=D3DFMT_A8R8G8B8)
	{
		assert(0); return; //what was the orignal blending mode???
	}

	DX::DDSURFACEDESC2 lock;
	if(!in->Lock(0,&lock,0,0))
	{
		for(size_t i=0;i<lock.dwHeight;i++)
		{
			DWORD *p = (DWORD*)((BYTE*)lock.lpSurface+lock.lPitch*i);
			for(size_t j=0;j<lock.dwWidth;j++) p[j] = 
			(255-max(max(p[j]>>16&0xFF,p[j]>>8&0xFF),p[j]&0xFF))<<24;			
		}	
		//NEW: the original kage.mdl files are mess up
		//seems they are meant to use a white colorkey
		//(more likely it's just a 16-bit color image)
		//(this just removes the box; it's still ugly)
		int shift = *(DWORD*)lock.lpSurface>>24; if(shift)
		{
			for(size_t i=0;i<lock.dwHeight;i++)
			{
				DWORD *p = (DWORD*)((BYTE*)lock.lpSurface+lock.lPitch*i);
				for(size_t j=0;j<lock.dwWidth;j++) p[j] = max(0,-shift+(p[j]>>24))<<24;
			}			
		}				
		in->Unlock(0);
	}
	else assert(0); 
}

static void som_hacks_device_reset(bool effects_buffers)
{
	//HACK: this change is just to refresh the unpublished black
	//border for protecting against back-light bleed
	//'psvr' is a special signal to avoid moving the window since
	//Windows 10 is bug riddled around fullscreen windows and the
	//taskbar
	{
		//DWORD z = SOM::bpp; //do_opengl?
		DWORD z = 0; 
		DWORD _ = 'psvr'; //0
		DX::D3DVIEWPORT7 vp = {0,0,SOM::width,SOM::height,EX_INI_Z_SLICE_2,1};
		//401a22 obsoletes this
		//DWORD x = SOM::field?vp.dwWidth:640;
		//DWORD y = SOM::field?vp.dwHeight:480;
		DWORD x = vp.dwWidth;
		DWORD y = vp.dwHeight;
		som_hacks_SetDisplayMode(0,0,x,y,z,_,_);
		DDRAW::Direct3DDevice7->SetViewport(&vp);				
	}
	if(effects_buffers) 
	{
		DDRAW::fx9_effects_buffers_require_reevaluation();
		//install D3D11 effects shaders?
		som_hacks_d3d11::try_DDRAW_fxWGL_NV_DX_interop2();
	}	
	//NOTE: SetDisplayMode doesn't do this unless bpp is changed
	{
		som_hacks_vs_<3>('_0'); som_hacks_ps_<3>('_0');
		DDRAW::vshaders9_pshaders9_require_reevaluation();
	}
	DeleteObject(SOM::font); SOM::font = 0; //resets SOM::Print
}

extern DWORD SOM::onflip_triple_time
[EX_ARRAYSIZEOF(SOM::onflip_triple_time)] = {};
//extern bool (*som_hacks_onflip_passthru)() = 0; //2021
extern bool (*som_exe_onflip_passthru)() = 0;
static bool som_hacks_onflip()
{
	//2022: upload one texture per frame?
	//these have already been updated by
	//som_MPX_job_3, however it's unable
	//to upload them
	//
	// NOTE: I tried som_game_once_per_scene
	// (before Begin) but I seem to detect a
	// second pause/stutter after a map load
	//
	extern SOM::TextureFIFO *som_MPX_textures;
	for(;;)
	{
		DWORD t = som_MPX_textures->remove_front();
		if(t>=1024||SOM::L.textures[t].update_texture(0))
		break;
	}

	//EX::dbgmsg("wait: %2d",DDRAW::flipTicks); //TESTING

	DDRAW::fxColorize = 0x00000000;

	bool out = som_exe_onflip_passthru();

	//this is for som_game_timeGetTime to consume
	//note, 500 seems wrong but it's what the main
	//loop has historically considered as the limit
	if(SOM::frame<120||0==EX::context())
	if(DDRAW::noTicks<100)
	if(SOM::frame-SOM::newmap>1) //2022
	SOM::onflip_triple_time[DDRAW::noFlips
	%EX_ARRAYSIZEOF(SOM::onflip_triple_time)] = DDRAW::noTicks;

	/*maybe just my imagination, but it seems less blurry
	//to do this in som_mocap_PSVR_view
	if(DDRAW::inStereo)
	{		
		//TESTING: not sure when/where to do this???
		SOM::PSVRToolbox();
	}*/

	//if(SOM::L.on_off)
	{	
		static int g = 0, j = 0;
		som_hacks_displays_a[0] = g!=SOM::showGauge;
		som_hacks_displays_a[1] = j!=SOM::showCompass;		
		g = SOM::showGauge; j = SOM::showCompass;
	}

	if(SOM::cone) //do_fix_fov_in_memory
	{
		//get ready for the next frame
		float zoom = M_PI/180*SOM::zoom2();
		float aspect = SOM::fov[0]/SOM::fov[1];	
		*SOM::cone = atanf(tanf(zoom/2)*aspect)*2;
		*SOM::cone*=EX::INI::Adjust()->fov_frustum_multiplier;	
		if(DDRAW::inStereo) *SOM::cone/=2;
	}
		
	#ifdef NDEBUG
	//#error fixed? investigated?
	int todolist[SOMEX_VNUMBER<=0x1020406UL];
	#endif
//2018: ASSUMING matte is not required here
//2017: something like this was done before, but
//I'm seeing garbage on maps that don't fade in/out
//E.g. Moratheia
/*2022: I think maybe this is hiding a bug that's 
//causing hiccups for trying to do seamless transfer
//update: som_hacks_BeginScene has/had some code for
//preventing it from being entered more than once
//for a given frame... I think this indicates larger
//problems... but removing it solves this one
if(SOM::newmap>=SOM::frame-2) return false; 
*/
	
			//////// DRAWING BLACK MATTE? /////////

	//note: these days the matte is limited to masking effects
	//since the parent window has a black background that will
	//serve as a primary matte in when the screen is maximized

	//stereo has a black matte at present
	//ASSUMING !dx_d3d9c_effectsshader_enable can do without a
	//matte
	#ifdef NDEBUG
	//#error WGL_NV_DX_interop2 can't draw in onFlip
	int todolist2[SOMEX_VNUMBER<=0x1020406UL];
	#endif
	if(!DDRAW::inStereo&&!DDRAW::WGL_NV_DX_interop2)
	{
		//NOTE: som_hacks_Clear is drawing WGL_NV_DX_interop2
		//since it's hard to guarantee som_hacks_oneffects is
		//drawn before Ex_output_oneffects 

		som_hacks_clear_letterbox(true);
	}

	/*this was never actually called because there was this
	//return statement here where it didn't belong

		return out;
	
	if(som_hacks_items_over_menus) //HACK #2
	{
		//2021: having problems when ZENABLE is turned on for 
		//icons
	//	*(DWORD*)0x1D6A248 = 1; //CAUSES PROBLEMS???
	//	DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_ZWRITEENABLE,1);
		//if(som_hacks_zenable)
		DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_ZENABLE,1);
	}*/

	return out;
}

static void (*som_hacks_oneffects_passthru)() = 0;

static void som_hacks_oneffects()
{
	if(DDRAW::WGL_NV_DX_interop2) //HACK: improvising :(
	{
		//D3D9X::IDirectDrawSurface7::flip fills this
		//som_hacks_d3d11::cb.ps2ndSceneMipmapping2;
		
		float *cc = som_hacks_d3d11::cb.colCorrect;
		DDRAW::fxCorrectionsXY(cc);

		cc = som_hacks_d3d11::cb.colColorize;
		for(int i=3;i-->0;)
		cc[i] = ((DDRAW::fxColorize>>i*8)&0xff)/255.0f;
		std::swap(cc[0],cc[2]);
	}

	if(som_hacks_items_over_menus) //HACK #1
	{
		//2021: having problems when ZENABLE is turned on for 
		//icons
	//	*(DWORD*)0x1D6A248 = 0; //CAUSES PROBLEMS???
	//	DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_ZWRITEENABLE,0);
	//	if(som_hacks_zenable)
		DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_ZENABLE,0);
	}

	som_hacks_oneffects_passthru();
		 	
	static int time = 0;	
	static int openxr = 0;
	static wchar_t text[64] = L""; 
	//2021: queue startup reminders
	static int startup = SOM::altf; 
	if(startup)
	if((SOM::altf||time)&&SOM::altf==(SOM::altf&startup))
	{
		if(time) goto startup;

		startup|=1<<31; //keep nonzero for background
	}
	else if(!time) startup = false;
		
	if(SOM::altf&(1<<12)) //F12 (startup reminder order)
	{
		SOM::altf&=~(1<<12);

		int mv = max(0,SOM::masterVol);
		if(mv) mv = (mv+1)/16;
		swprintf_s(text,SOM::translate(som_932_Master_Volume,"MASTER VOLUME %d"),mv);

		time = 1000;

		startup&=~(1<<12); goto startup;
	}
	if(SOM::altf&(1<<1)) //F1 (startup reminder order)
	{
		SOM::altf&=~(1<<1);

		if(DDRAW::xr)
		{
			int &sm = SOM::stereoMode; sm = !sm;

			SOM::translate(text,som_932_Super_Modes[sm],sm?"MAX SUPERSAMPLING":"MIN SUPERSAMPLING");

			DDRAW::xrSuperSampling = sm!=0;

			DDRAW::stereo_toggle_OpenXR(false);

			som_hacks_device_reset(true); 

			//DUPLICATE
			float s = DDRAW::Direct3DDevice7->queryX->effects_w/SOM::fov[0]; 
			s*=SOM::stereoMode?0.4f:0.6666f;
			EX::stereo_font = s;
		}
		else
		{
			int &sm = SOM::superMode;
			
			if(!startup)
			{
				//HACK: supersampling is for use with do_aa
				if(!EX::INI::Option()->initialize_aa(true)) //force?	
				{
					//FIX ME: rebuild mipmaps??? mipmaps_pixel_art_power_of_two?
					//if(!DDRAW::sRGB) DDRAW::sRGB = 1;
					assert(DDRAW::sRGB);

					sm = true;
				}
				else sm = !sm;
			}
			if(som_hacks_shader_model<3)
			{
				swprintf_s(text,SOM::translate
				(som_932_Super_Modes[2],"SUPERSAMPLING UNAVAILABLE (%d)"),som_hacks_shader_model);
			}
			else //TODO: it'd be nice to revert back to do_aa
			{
				SOM::translate(text,som_932_Super_Modes[sm],sm?"4X SUPERSAMPLING":"3D ANTI-ALIASING");
		
				if(!startup) som_hacks_device_reset(true);
			}
		}

		time = DDRAW::xr?3000:startup?1000:2000;

		startup&=~(1<<1); goto startup;
	}	
	
	if(SOM::altf&1) //HACK: 1<<1 is F1
	{	
		if(SOM::analogMode)
		{
			int d = abs(SOM::analogMode), c = SOM::analogMode<0?'B':'A';
			swprintf_s(text,SOM::translate(som_932_Analog_Mode,"ANALOG MODE %d-%c"),d,c);
		}
		else SOM::translate(text,som_932_Analog_Disabled,"ANALOG DISABLED");	

		time = 1000; SOM::altf&=~1;
	}

	if(openxr&&!time) //HACK
	{
		assert(SOM::altf_mask&(1<<2));

		SOM::altf_mask&=~(1<<2);

		SOM::stereo = true; 
		DDRAW::inStereo = true;
		DDRAW::xr = openxr; openxr = 0; //!!

		//HACK: reset view incline because it starts out looking 
		//slightly down as it's more natural, but it's not center
		//in VR since your real head will do that
		SOM::L.pcstate[3] = SOM::uvw[0] = 0;

		som_hacks_device_reset(true); //DDRAW::doSuperSampling

		assert(DDRAW::inStereo);

		//if(DDRAW::inStereo)
		{	
			//NOTE: might help to turn off GL_SHADING_RATE_IMAGE_NV
			//since it seems to interact badly with alpha channels
			//featuring fine b/w type details (bilinear sampling)
			/*weirdly SOM's font size is (still) based on horizontal
			//resolution
			float s = DDRAW::Direct3DDevice7->queryX->effects_h/SOM::fov[1];*/
			float s = DDRAW::Direct3DDevice7->queryX->effects_w/SOM::fov[0]; 

			//NOTE: the text can't be mipmapped, so if it's too big
			//it will show shimmering artifacts
			//TODO? might try toggling off GL_SHADING_RATE_IMAGE_NV
			//NOTE: 0.5 is too much but the guages are double sized
			//NOTE: 0 is 1.0f (disabled)
			s*=SOM::stereoMode?0.4f:0.6666f;

			EX::stereo_font = s; //super sample? //DUPLICATE
			
			if(!SomEx_output_OpenXR_mats)
			{
				auto &sv = DDRAW::Direct3DDevice7->queryGL->stereo_views;
				typedef float mat[4][4]; //C++
				SomEx_output_OpenXR_mats = new mat[3*_countof(sv)]();
				DDRAW::stereo_locate_OpenXR_mats = new mat[_countof(sv)];
			}

			//HACK: initialize QueryGL::stereo before drawprims2
			//if(EX::context())
			{
				float swing[6], skyanchor = 
				SOM::motions.place_camera(SOM::analogcam,SOM::steadycam,swing);
			}
		}
	}
	else if(SOM::altf&(1<<2) //VR?
	&&(!DDRAW::gl||SOM::field)) //HACK: must test titles and classic HUD elements
	{			
		if(DDRAW::gl) //OpenXR?
		{
			//NOTE: it wouldn't be bad to default
			//to PSVR mode stereo, but the OpenGL
			//path doesn't support it
			if(!DDRAW::stereo_toggle_OpenXR()) goto mono;

			if(DDRAW::inStereo)
			{
				//HACK: wait one more frame to draw message

				openxr = DDRAW::xr; SOM::altf_mask|=1<<2; //prevent pressing F2

				SOM::stereo = 0; DDRAW::xr = 0; DDRAW::inStereo = false; //HACK!!!

				auto alt = EX::INI::Output()->function_overlay_tint[2]>>24;

				swprintf_s(text,SOM::translate(som_932_Stereo,"VR IN USE (Win+Y, %hsF2)"),alt?"Alt+":"");
			}
			else 
			{
				EX::stereo_font = 0.0f;

				som_hacks_device_reset(true); //DDRAW::doSuperSampling
			}
		}
		else  if(som_hacks_shader_model>=3)
		{		
			bool mono = 0!=SOM::stereo; //2022

			if(3>=som_hacks_shader_model)
			{	
				//NOTE: can be false if using 2x supersampling
				//however I'm trying 3/2 since the PlayStation
				//VR doesn't really benefit from 2x regardless
				som_hacks_device_reset(DDRAW::doSuperSampling);

				/*I think maybe this is unnecessary after updating the PSVR firmware
				//for the first time
				//EXTENSION: ASSUMING PlayStation VR			
				DDRAW::dimmer = SOM::gamma<0?8-SOM::gamma:0;
				som_hacks_PSVR_dim();*/
			}

			//2022: I'm adding a lot to harden this in case of any failures.
			if(mono&&(!SOM::stereo||!DDRAW::stereo_update_SetStreamSource()))
			{
				assert(!SOM::stereo&&!DDRAW::inStereo); //2022

				SOM::stereo = 0;

				if(DDRAW::inStereo) //2022: stereo_update_SetStreamSource?
				{
					DDRAW::inStereo = false; //UNNECESSARY

					som_hacks_device_reset(DDRAW::doSuperSampling);
				}

				EX::stereo_font = 0.0f;

				goto mono;				
			}
			else //PSVR?
			{
				EX::stereo_font = 0.7f; //EX::small_stereo_font?
			}
		}		
		else mono:
		{
			SOM::stereo = 0; assert(!DDRAW::inStereo);

			swprintf_s(text,SOM::translate(som_932_Mono,"STEREO UNAVAILABLE (%d)"),som_hacks_shader_model);
		}		

		EX::beginning_output_font(); //Ex_output_Exselector_xr_callback?

		time = 1000; SOM::altf&=~(1<<2);
	}

	if(SOM::altf&(1<<3))
	{
		if(DDRAW::xr) //OpenXR?
		{
			auto d = DDRAW::dejagrate_divisor_for_OpenXR;
			auto s = DDRAW::Direct3DDevice7->queryGL->stereo_views[0];			
			swprintf_s(text,SOM::translate(som_932_Zoom,"%d AA TUNE (%d SS HEIGHT)"),d,s.h);
		}
		else if(DDRAW::inStereo) //PSVR?
		{	
			swprintf_s(text,SOM::translate(som_932_IPD,"%d BINOCULAR GAP"),SOM::ipd);			
		}
		else swprintf_s(text,SOM::translate(som_932_Zoom,"%d DEGREE ZOOM"),SOM::zoom);

		time = 1000; SOM::altf&=~(1<<3);
	}

	if(SOM::altf&(1<<16|1<<17))
	{
		int mask = SOM::altf>>16;

		if(mask==3) mask = 1;

		time = 1000; SOM::altf&=~(1<<16|1<<17);

		int i = mask==1?1:3, j = mask==1?2:4; 
		
		//2020: negating disables just in case it annoys somebody
		assert(SOM::buttonSwap>=0);

		//2020: reserving bit 1 for negating since -0 isn't an int
		//if(SOM::buttonSwap&mask) std::swap(i,j);
		if(SOM::buttonSwap>>1&mask) std::swap(i,j);

		//HACK: DS4 inputs are unconventional.
		wchar_t pf[3] = L"("; const wchar_t*p;
		pf[1] = '0'+i; if(p=SOM::Joypad(i-1,pf)) i = p[1]-'0'; 
		pf[1] = '0'+j; if(p=SOM::Joypad(j-1,pf)) j = p[1]-'0';

		swprintf_s(text,SOM::translate(som_932_Button_Swap,"BUTTON SWAP %d-%d"),i,j);
	}

	if(time!=0) startup:
	{
		/*no use... I get VISIBLE/FOCUSED while in the
		//cliff house and animation
		if(DDRAW::xr)
		switch(DDRAW::stereo_status_OpenXR())
		{
		default: return; //wait until text can be seen?

		case 4: case 5: break; //VISIBLE //FOCUSED
		}*/

		bool playing_game = 
		SOM::context==SOM::Context::playing_game;

	//	if(context==playing_game)
		if(!SOM::paused)
		{						
			//black here is interpreted as a context
			//switch somewhere
			//NOTE: 0x64010101 bypasses blackened_tint2
			//(probably for the better)
			//if(!playing_game) SOM::Black();
			if(!playing_game&&!startup) SOM::Clear(0x64010101);			

			//2020: assume loading or pause delayed the time
			//so don't count it against it
			if(DDRAW::noTicks<=66) time-=DDRAW::noTicks; 
			else if(!openxr)
			time = 1000; //go aheads and reset
			
			SOM::Print(text);

			if(time<=0) *text = '\0'; //NEW: alt+f2
		}

		if(time<0) time = 0; //2021
	}

	static bool splashed = SOM::splashed(SOM::window); 
}

extern void SOM::initialize_som_hacks_cpp()
{
	//REMINDER: workshop.cpp/SOM_MAP.cpp are reusing
	//som_exe_onflip_passthru (probably shouldn't)
	assert(SOM::game);

	if(som_exe_onflip_passthru) return;

	som_exe_onflip_passthru = DDRAW::onFlip;
	som_hacks_oneffects_passthru = DDRAW::onEffects; 

	DDRAW::onFlip = som_hacks_onflip;
	DDRAW::onEffects = som_hacks_oneffects;
}

