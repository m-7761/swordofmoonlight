
#include "Ex.h"
EX_TRANSLATION_UNIT

//REMOVE ME? 
//win_iconv
#include <errno.h>
#include "iconv.h" 

#include "dx.ddraw.h"

#include "Ex.ini.h" 
#include "Ex.log.h"
#include "Ex.input.h"
#include "Ex.output.h"
#include "Ex.window.h"
#include "Ex.cursor.h" 
#include "Ex.detours.h" 
#include "Ex.langs.h" 
#include "Ex.movie.h" 
#include "Ex.mipmap.h"

#include "som.h"
//extern: SomEx.ini.cpp
extern void SomEx_environ(const wchar_t *kv[2], void*_)
{
	return Som_h_environ(kv,_);
}
#include "som.state.h"
#include "som.title.h"
#include "som.tool.h"
#include "som.game.h"
#include "som.status.h"
#include "som.shader.h"
#include "som.files.h"
#include "som.extra.h"
#include "SomEx.h"
extern int SOMEX_vnumber = SOMEX_VNUMBER;

#include "../Exselector/Exselector.h" //OpenGL font?
extern class Exselector *Exselector = 0;

extern float(*SomEx_output_OpenXR_mats)[4][4] = 0;
extern int SomEx_output_OpenXR_font_callback(int view)
{
	//NOTE: I'm defining this here because it includes
	//Exselector.h and dx.ddraw.h

	//this happens when the device is reset from inside
	//the Options menu while in VR
	if(!DDRAW::xr) return false;
		
	assert(SomEx_output_OpenXR_mats);

	auto &sv = DDRAW::Direct3DDevice7->queryGL->stereo_views;
	auto &s = sv[view];

	if(view>=_countof(sv)||!s.src)
	return false;

	//NOTE: subtitles appear in som_scene_hud
	//0 is menu text
	//1 is menu text on layer 2 and subtitles
	//2 is kf2 style gauge text
	int m = 1;
	extern bool som_scene_hud;
	extern unsigned som_hacks_tint;
	if(som_hacks_tint==SOM::frame)
	if(som_scene_hud)
	m = 2;
	else if(!SOM::Game.taking_item
	&&SOM::doubleblack!=SOM::frame)
	m = 0;
	m = m*_countof(sv)+view;

	Exselector->svg_viewport(s.x,s.y,s.w,s.h);
	Exselector->svg_view(*SomEx_output_OpenXR_mats[m]);

	return true;
}

namespace DSOUND
{
	extern bool doPiano;
	extern bool doDelay;
	extern bool doForward3D,doReverb;
	extern int doReverb_i3dl2[2];
	extern int doReverb_mask;
	extern void Piano(int);
}
namespace DSHOW
{
	extern bool (*opening)(LPCWSTR);
}
namespace DINPUT
{
	extern int Qwerty(unsigned char,bool,bool);

	extern bool doData;
}
namespace EX //REMOVE ME?
{
	extern int Regex_is_needed_to_initialize(); 
}

HMODULE WINAPI Detoured() //detoured.dll
{
    return EX::module; //I think this is how this works???
}

extern wchar_t SomEx_exe[16] = {}; //Som.h

extern const wchar_t *EX::exe() 
{
	if(*SomEx_exe) return SomEx_exe; 
	if(SOM::game) return SOM::Game::title();
	assert(0);
	return L"Ex (untitled)";
}
extern const wchar_t *EX::log()
{	
	enum{ w=8 };
	static wchar_t out[w] = {}; 
	if(*out) return out;		
	const wchar_t *exe = EX::exe();
	if(!wcsnicmp(exe,L"SOM_",4))
	{
		//2021: SomEx(true) sets *EX::log() to '\0' 
		//but doesn't know the length so this must
		//0-terminate
		int i = 0; do
		{			
			out[i] = tolower(exe[4+i]);

		}while(out[i]&&++i<w); out[w-1] = '\0';
	}
	else
	{
		//calling these workshop (ItemEdit, etc.)
		const wchar_t *edit = wcsstr(exe,L"Edit");
		if(edit&&!edit[4]) return wcscpy(out,L"ws"); 
		wcscpy(out,wcsicmp(exe,L"MapComp")?L"rt":L"mpx");	
	}
	return out;
}	  

static void SomEx_Detours(LONG (WINAPI *f)(PVOID*,PVOID))
{
	if(SOM::tool) SOM::Tool.detour(f);
	if(SOM::game) SOM::Game.detour(f);
}

static bool SomEx_numlock = false;

static EX::Joypad::Configuration SomEx_mouse_configuration();
static EX::Joypad::Configuration SomEx_joypad_configuration();
static EX::Pedals::Configuration SomEx_pedals_configuration();
  
extern bool SomEx_movies(LPCWSTR avi)
{	
	if(1==SOM::L.startup) //opening movie?
	{	
		static bool encore = false;
		if(!encore&&SOM::opening
		//DEBUGGING: NEED TO SKIP THE FADE IN/OUT SOMEHOW
		||EX::INI::Window()->do_hide_opening_movie_on_startup)
		{
			encore = true; return false;
		}
		SOM::opening = 1; //skip initial opening movie next session
	}

	//2022: WIndows???
	//EX::AVI::Vmr9::StartPresenting is supposed to do this???
	EX::showing_window();

	if(DDRAW::compat=='dx9c')
	{
		//2021: som_hacks_Flip can't do this
		//NOTE: casting bypasses the 1 frame "limbo" state
		SOM::context = (SOM::Context)SOM::Context::playing_movie;
		extern void som_game_highlight_save_game(int,bool);
		som_game_highlight_save_game(-1,0);

		EX::playing_movie(avi); return false;
	}
	return true;
}

//TODO: make Exselector part of Ex.hpp!
extern bool SomEx_load_Exselector()
{
	if(Exselector) return true;

	if((GetVersion()&0xFF)>=6) //Vista?
	if(SearchPathA(0,"Exselector.dll",0,0,0,0))
	Exselector = Sword_of_Moonlight_Extension_Selector(0);

	return Exselector!=0;
}		

extern const char *SOM_MAIN_generate();
static int EX__INI__Number___MODE = -1;
extern int EX::is_needed_to_initialize()
{		
	//this is where dynamic initialization occurs

		//////REMINDER///////
		//
		// this runs after the INI file is read
		//
		//assert(SOM::bpp);
		assert(!SOM::game||SOM::L.config[3]);

	static bool one_off = false; 
	if(one_off) return 1; one_off = true;

	/////2020/////HOMELESS
	//
	// I crashed today before releasing my 
	// KF2 demo. I've never crashed out of
	// the blue before, so I've added this
	// in case it's useful, either for end
	// user or there's no debugger present
	//
	//I think this is clear of DllMain but it
	//can't catch an earlier crash
	static EX::crash_reporter _cr
	//TODO: can use Win32 API to get 
	//the program name and version #
	(L"%TEMP%\\Swordofmoonlight.net\\SomEx-"
	EX_WCSTRING(SOMEX_VERSION)L"-",EX::exe()); 
	(void)_cr;

	EX::INI::initialize(); //gross initialization

	float f = EX::reevaluating_number(0,L"_MODE"); 
	if(f>=0) EX__INI__Number___MODE = f; //NaN?

	EX::INI::Window wn; EX::INI::Bugfix bf;
	EX::INI::Option op;	EX::INI::Detail dt;	
	EX::INI::Adjust ad;	EX::INI::Player pc;
	EX::INI::Editor ed;	EX::INI::System os;
	EX::INI::Stereo vr; //goto tool?

	if(SOM::tool)
	{
		SOM::superMode = op->do_aa2; //2021

		goto tool; //skip past runtime only stuff
	}

	//HACK: Set the first frame to be greater than various
	//frame dependent variables.
	SOM::frame = DDRAW::noFlips = SOM::frame0;

	//It can change, but should be set once per frame if so.
	EX::central_processing_units = 
	os->central_processing_units;
	if(0==EX::central_processing_units)
	{		
		SYSTEM_INFO si; GetSystemInfo(&si);
		EX::central_processing_units = si.dwNumberOfProcessors;
	}

	//this shows a movie only once
	DSHOW::opening = SomEx_movies;
	SOM::opening = SOM::config("opening",0);
	{
		//2022: opengl? reset some keys on a new user's machine?	

		wchar_t id[64]; DWORD id_s = sizeof(id)-sizeof(wchar_t); //~40
		DWORD type = REG_SZ;		
		//NOTE: Somversion.dll stores CSV timestamps in SOM but it stores other things as well
		if(SHGetValueW(HKEY_CURRENT_USER,L"SOFTWARE\\FROMSOFTWARE\\SOM",L"PCID",&type,id,&id_s))
		{
			//I was thinking of using MachineGuidm but it was kind of a mess and I wasn't sure
			//if it could be used for espionage purposes (not that this value can't be so used)
			id_s = sizeof(wchar_t)*swprintf(id,L"%hs",SOM_MAIN_generate());
			SHSetValueW(HKEY_CURRENT_USER,L"SOFTWARE\\FROMSOFTWARE\\SOM",L"PCID",REG_SZ,id,id_s);
		}		
		id[id_s/sizeof(wchar_t)] = '\0'; //PARANOID?
		wchar_t cmp[sizeof(id)];
		if(!GetPrivateProfileString(L"config",L"pcid",0,cmp,_countof(id),SOM::Game::title('.ini'))
		||wcscmp(cmp,id))
		{
			SOM::Game.write("opening","0"); SOM::opening = 0;
			SOM::Game.write("opengl","-1"); //my main concern
			if(op->do_opengl)
			SOM::Game.write("bpp","32"); SOM::bpp = 32; //NOTE: bpp is already set
			SOM::Game.write("stereo","0");
			SOM::Game.write("stereoMode","0");
			SOM::Game.write("superMode","1"); //default id 4k (with warning title)
			SOM::Game.write("capture","0");
			SOM::Game.write("device","2"); //default display
			SOM::Game.write("gamma","8"); //I guess? //ipd?
			WritePrivateProfileString(L"config",L"pcid",id,SOM::Game::title('.ini'));		
		}
	}

	//REMOVE ME?
	EX::Regex_is_needed_to_initialize();

	//REMOVE ME?
	SOM::initialize_som_menus_cpp();					
	SOM::initialize_som_hacks_cpp();
	SOM::initialize_som_status_cpp();

	EX::Joypad::micecfg = SomEx_mouse_configuration();
	EX::Joypad::padscfg = SomEx_joypad_configuration();
	EX::Pedals::pedscfg = SomEx_pedals_configuration();

	//if(op->do_pedal) //deprecated
	{
		int compile[EX_AFFECTS==3];
		EX::Affects[0].activate(); //rebound
		EX::Affects[1].activate(); //inertia
		EX::Affects[2].activate(); //jumping/falling
	}

	//2020: maybe do_sounds should be Bugfix?
	if(op->do_sounds||op->do_reverb)
	{
		//2020: this value can be set to 1 to prevent unnecessary duplication of 
		//static sound buffers, but not at this point in the process's execution
		//0044AF87 C7 05 8C 9D D6 01 05 00 00 00 mov         dword ptr ds:[1D69D8Ch],5
		DSOUND::doForward3D = true;

		if(DSOUND::doReverb=op->do_reverb)
		{
			DSOUND::doReverb_i3dl2[0] = 3; //testing "room"
		}
	}
	if(bf->do_fix_asynchronous_sound)
	{
		DSOUND::doDelay = true;
	}
	//if(bf->do_fix_asynchronous_input)
	{
		//DINPUT::doData = true; //experimental
	}	
	
	//30 is the new/recommended value
	//(because it minimizes distortion)
	//SOM::zoom = SOM::config("zoom",30);
	//SOM::zoom = /*min(50,*/max(30,SOM::zoom)/*)*/;
	SOM::altf3(0); //initialize SOM::zoom

	bool gl32 = false;

	//note, at this point it's unknown if the shader
	//has the necessary instruction set to do stereo
	switch(wn->directx_version_to_use_for_window)
	{
	case 32: gl32 = true; //WGL?

	case 11: //WGL_NV_DX_interop2? (synonym for now)

		DDRAW::gl = true; //altf2?

	case 12: DDRAW::doD3D9on12 = EX::debug; //break;

	case 9: //TODO: add stereo to OpenGL modes?
			
		if(SOM::stereo=SOM::config("stereo",0)) SOM::altf2();
	}
	if(op->do_opengl&&!DDRAW::gl) //assuming not 11?
	{
		gl32 = DDRAW::gl = true; //disable doFlipEx?
	}

	DDRAW::resolution[0] = SOM::width = SOM::config("width",800); //640
	DDRAW::resolution[1] = SOM::height = SOM::config("height",600); //480
	//2017: prefering SOM::width/height now, but it's best to
	//initialize SOM::fov to avoid problems
	//2022: 401a22 obsoletes this
	//SOM::fov[0] = wn->do_scale_640x480_modes_to_setting?SOM::width:640;
	//SOM::fov[1] = wn->do_scale_640x480_modes_to_setting?SOM::height:480;
	SOM::fov[0] = SOM::width;
	SOM::fov[1] = SOM::height;

	//2020: moving out of EX::arrow
	SOM::cursorX = SOM::config("cursorX",0);
	SOM::cursorY = SOM::config("cursorY",0);
	SOM::cursorZ = SOM::config("cursorZ",0);
	SOM::capture = SOM::config("capture",0);
	int todolist[SOMEX_VNUMBER<=0x1020406UL];
	//2022: no longer working (January) (maybe just
	//a temporary Windows 10 bug?) (is this function
	//only called once?)
//	if(0&&SOM::capture)
	if(SOM::capture) //Aug 2022: trying reenable?
	{
		//TESTING: this shows the up arrow... I wish
		//it would timeout
		int todolist[SOMEX_VNUMBER<=0x1020406UL];
		SOM::f10();
	}

	SOM::map = SOM::rescue(L"map",0);
	SOM::mapX = SOM::rescue(L"mapX",SOM::width/2);
	SOM::mapY = SOM::rescue(L"mapY",SOM::height/2);
	SOM::mapZ = SOM::rescue(L"mapZ",1); 
	
	SOM::escape(0); //hack: SOM::analogMode
	SOM::superMode = SOM::config("superMode",!0); //2021	
	if(SOM::superMode) //remind at startup?
	{
		SOM::superMode = 1;
		SOM::altf|=1<<1; op->initialize_aa(true); //force?
	}

	SOM::stereoMode = SOM::config("stereoMode",0); //2022
	DDRAW::xrSuperSampling = SOM::stereoMode!=0;
	DDRAW::dejagrate_divisor_for_OpenXR = vr->aa_divisor;

	//2020: 1 is a dummy so it can be negated
	SOM::buttonSwap = SOM::config("buttonSwap",1); //0
	//this will work on old files that have 0
	if(!SOM::buttonSwap) SOM::buttonSwap = 1;
	SOM::masterVol = SOM::config("masterVol",255);
	if(SOM::masterVol!=255)
	if(!os->do_hide_master_volume_on_startup)
	SOM::altf|=1<<12;

	if(wn->restrict_aspect_ratio_to)
	DDRAW::aspectratio = wn->restrict_aspect_ratio_to;
	//if(wn->do_scale_640x480_modes_to_setting) //hack
	{
		DDRAW::xyMinimum[0] = 320; //800; //temporary fix
		DDRAW::xyMinimum[1] = 240; //600; //temporary fix
	}	
	if(wn->do_not_compromise_fullscreen_mode)
	DDRAW::fullscreen = true;
	if(wn->do_use_interlaced_scanline_ordering) //forcing exclusive mode
	{
		//2021: fullscreen is not functioning well right now, and it seems
		//it may be that EnumAdapterModesEx can expose higher resolutions
		//if the interlace flag is set
		//DDRAW::isInterlaced = DDRAW::fullscreen = true; 	
		DDRAW::isInterlaced = true; 	
	}
	DDRAW::isStretched = !wn->do_center_picture_for_fixed_display;
	DDRAW::isSynchronized = !wn->do_force_immediate_vertical_refresh;
	DDRAW::doWait = wn->do_force_refresh_on_vertical_blank;	
	
	if(SOM::game) //2021: did Windows Update change this?
	DDRAW::doFlipDiscard = true;
	if(!gl32)
	DDRAW::doFlipEx = true; //Windows 7+ games only
	//this makes som_db run very smooth. I don't understand
	//why that would be. so don't turn it off, but sometimes
	//there is/are spasm(s) so I'm trying to see what happens
	//without averaging the frame rate
	//DDRAW::doTripleBuffering = 1<EX_ARRAYSIZEOF(SOM::onflip_triple_time);
	DDRAW::doTripleBuffering = false;
				  	
	SomEx_numlock = op->do_numlock?dt->numlock_status:false;

	SOM::emu = op->do_2k; //Som2k emulation

tool: //things meaningful to tools starts here	

	//REMOVE ME
	//SOM::initialize_som_keys_cpp();

	//2021: these are secret codes for now
	
	switch(wn->shader_compatibility_level)
	{
	case 0: DDRAW::shader = 'ff'; break; //fixed function
	case 1: DDRAW::shader = 'ps'; break; //pixel shader
	case 2: DDRAW::shader = 'ps'; break; //'vs+' (reserved)

	default: DDRAW::shader = 0; //client model			
	}	

	int gl = 0; 

	//NOTE: this changes the Colors option
	//to Graphics and so forces on 32 bits
	if(SOM::game&&0==DDRAW::shader)
	if(op->do_opengl) 
	{
		if(SOM::game&&0==DDRAW::shader)		
		if(gl=SOM::config("opengl",-1)) //!
		{
			//HACK: try to avoid the bad
			//AMD/Radeon OpenGL experience
			if(gl!=0&&gl!=1)
			DDRAW::doNvidiaOpenGL = true;

			SOM::opengl = gl; gl = 'dxGL';			
		}
	}
	else switch(wn->directx_version_to_use_for_window)
	{
	case 95: gl = 'dx95'; break;
	case 11: //2022
	case 32: gl = 'dxGL'; break;
	}	
	
	if(SOM::game) //tools are _dx6 for now
	{
		switch(EX::directx()) //NEW
		{
		case 9: DDRAW::target = gl?gl:'dx9c'; break;
		case 7: DDRAW::target = EX::debug?'dx7a':'dx9c'; break;
		}
	}
	else if(SOM::tool) 
	{
		int dx = EX::directx();

		if(SOM::tool>=PrtsEdit.exe)
		{
			SOM::initialize_workshop_cpp();

			DDRAW::target = 7==dx?'dx7a':gl?gl:'dx9c'; 
		}
		else if(DDRAW::target=='dx9c') //SOM_MAP.cpp?
		{
			assert(SOM::tool==SOM_MAP.exe); //EXPERIMENTAL (2021)
		}
		else DDRAW::target = '_dx6'; //hack: just for now??

		DDRAW::doSoftware = EX::INI::Editor()->do_hide_direct3d_hardware;
	}
	else assert(0);

	//Exselector?
	//TODO: probably want to do this for other reasons
	if(DDRAW::target==gl)
	{
		if(!SomEx_load_Exselector())
		{
			gl = 0;  assert(0); 

			DDRAW::target = 'dx9c'; //TODO: flash a message?
		}
	}
	else gl = 0; //UNNECESSARY

	//if(9==EX::directx())
	{		
		//HACK: SOM::kickoff_somhacks_thread needs this before
		//DDRAW::is_needed_to_initialize is able to sets it up
		switch(DDRAW::target)
		{
		case 'dx7a': case '_dx6': break;

		default: DDRAW::compat = 'dx9c';  //!!

			DDRAW::doMipmaps = op->do_mipmaps;

			if(op->do_anisotropy) DDRAW::linear = true;

			//NOTE: this doesn't seem to make a difference on my system
			//but is provided in case it helps any drivers not to crash
			/*the Nvidia D3D9 driver is crashing mid background loading
			DDRAW::doMultiThreaded =
			os->do_direct3d_multi_thread&&1!=EX::central_processing_units;*/
			if(SOM::game)
			DDRAW::doMultiThreaded = 1!=EX::central_processing_units;
		}
	}
	
	//DDRAW::doMSAA = op->do_aa2; //OBSOLETE
	//DDRAW::doBlack = op->do_black; //OBSCURE
	DDRAW::do565 = op->do_green;	
	DDRAW::doDither = true; //op->do_dither; //PSVR //???
	DDRAW::doSmooth = op->do_smooth;
	if(SOM::tool&&ed->do_not_dither) DDRAW::doDither = false;
	if(SOM::tool&&ed->do_not_smooth) DDRAW::doSmooth = false;
		
	//TODO: MOVE INTO som.hacks.cpp SETUP
	//2021: no mipmaps is a simple way to
	//minimize blurriness in the tools and
	//keep to the their original esthetic
	//NOTE: in shaders I'm using a -1 bias
	DDRAW::mipmap = SOM::tool&&DDRAW::shader?0:EX::mipmap; 
	DDRAW::colorkey = EX::colorkey;

	if(op->do_alphafog) 
	if(SOM::game&&!DDRAW::shader)
	{
		//NEW: THIS IS FOR SKY/SHADOW EFFECTS
		//REMINDER: could do with one channel 
		//exotic? D3DFMT_G16R16F doesn't work
		//(could fall back to 32-bit instead)
	//	DDRAW::mrtargets9[1] = (D3DFORMAT)114; //D3DFMT_R32F
		DDRAW::mrtargets9[1] = (D3DFORMAT)111; //D3DFMT_R16F
		DDRAW::altargets9[1] = (D3DFORMAT)114; //D3DFMT_R32F
		DDRAW::ClearMRT[1] = "1,0,0,0";	

		//2021: now must be initially set to 1 to use shaders
		//2021: 2 uses Clear instead of shaders. it's a little
		//better at supersampling... maybe more depending on
		//hardware features
		//NOTE: som_hacks_CreateDevice sets this up cautiously
		if(wn->do_not_clear)
		DDRAW::doClearMRT = 1; //2
	}

	//if someone wants to disable this then F1 supersampling
	//either has to be disabled or it must be able to rebuild
	//all mimpaps and rebuild the pixel_art extension textures
	//if(op->do_srgb
	//||dt->mipmaps_pixel_art_power_of_two&&op->do_mipmaps) //F1?
	{
		//2021: this really needs to be turned on/off before
		//CreateDevice in som.hacks.cpp
		if(!DDRAW::shader) //2021
		{
			//DDRAW::sRGB = 1;
			DDRAW::sRGB = op->do_srgb; //probably doesn't matter
		}
	}

	SOM::initialize_shaders(); 
	
	EX::detours = SomEx_Detours;

	EX::setting_up_Detours();

	//install DDRAW::onFlip hook
	//2021: depends on DDRAW::gl
	EX::initialize_output_overlay();

	//hack: scheduled obsolete
	SOM::kickoff_somhacks_thread(); 		

	EXLOG_HELLO(0) << "SomEx initialized\n";

	return 1; //for static thunks
}

//REMOVE ME?
extern const wchar_t *EX::Workpath(const wchar_t *cd)
{
	static wchar_t out[MAX_PATH] = {}; if(!cd)
	{
		if(!*out) GetCurrentDirectoryW(MAX_PATH,out); return out;
	}

	if(wcsnlen(cd,MAX_PATH)==MAX_PATH) return 0;

	if(PathIsRelativeW(cd))
	{
		if(!*out) GetCurrentDirectoryW(MAX_PATH,out);

		if(wcsnlen(cd,MAX_PATH-wcslen(out)-1)==MAX_PATH) return 0;	

		wcscat_s(out,MAX_PATH,L"\\"); wcscat_s(out,MAX_PATH,cd); 
	}
	else wcscpy_s(out,MAX_PATH,cd); return out;
}
						 
static void SomEx_langs()
{
	const wchar_t *messages = 0; 

	EX::INI::Script lc;	EX::INI::Author au;

	if(*lc->translation_to_use_for_play)
	messages = lc->translation_to_use_for_play;	
	else if(*au->international_translation_title)
	messages = au->international_translation_title;	
	else if(*au->international_production_title)
	messages = au->international_production_title; 
	else messages = L"script"; 
	
	EX::Locale &applocale = EX::Locales[EX::Locale::APP];
	EX::Locale &usrlocale = EX::Locales[EX::Locale::USR];

	applocale.messages = messages;
	applocale.languages = EX::lang;	
		
	if(*lc->locale_to_use_for_play)
	applocale->desired = lc->locale_to_use_for_play;
	if(*lc->locale_to_use_for_play_if_not_found)
	applocale->missing = lc->locale_to_use_for_play_if_not_found;
	if(lc->do_use_builtin_english_by_default)
	applocale->instead = "en_US";	
	applocale->program = "ja_JP";
	if(*lc->locale_to_use_for_dates_and_figures)
	usrlocale->desired = lc->locale_to_use_for_dates_and_figures;

	applocale.ready(); 
	
	//NEW: default script	
	if(!*applocale.catalog) 
	if(0<swprintf_s(applocale.catalog,L"%ls\\script.mo",EX::cd()))
	if(SOM::game&&!PathFileExistsW(applocale.catalog))
	*applocale.catalog = '\0'; 
}

static bool SomEx_fonts(const wchar_t *p)
{	
	DWORD flags = FR_PRIVATE|FR_NOT_ENUM;

	if(!*p||!PathIsDirectoryW(p))
	{				
		if(*p) AddFontResourceExW(p,flags,0);
		return *p; 
	}

	wchar_t font[MAX_PATH];		
	wchar_t *file = font+swprintf(font,L"%ls\\*",p)-1;				

	WIN32_FIND_DATAW found; assert(file>font);

	HANDLE glob = FindFirstFileW(font,&found);

	if(glob!=INVALID_HANDLE_VALUE)
	{
		for(int safety=0;1;safety++)
		{
			if(*found.cFileName!='.')
			if(~found.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN)
			{
				wcscpy(file,found.cFileName);				
				AddFontResourceExW(font,flags,0);				
			}
			
			//TODO: safety dialog
			if(!FindNextFileW(glob,&found)||safety>32)
			{				
				assert(GetLastError()==ERROR_NO_MORE_FILES);
						
				FindClose(glob); break;
			}		
		}
	}

	return true;
}

static bool SomEx_tool(wchar_t *exe, wchar_t *ext) 
{
	const wchar_t *guess = 0;

	while(ext!=exe&&*ext!='\\'&&*ext!='/') ext--;

	if(ext!=exe) exe = ext+1; ext = 0; //!! 

	if(!wcsnicmp(exe,L"SOM_",4))
	{
		switch(toupper(exe[4]))
		{	
		case 'M': guess = L"SOM_MAIN"; 

			if(toupper(exe[6])=='P') guess = L"SOM_MAP"; break;

		case 'E': guess = L"SOM_EDIT";
			
			if(toupper(exe[5])=='X') guess = L"SOM_EX"; break;

		case 'R': guess = L"SOM_RUN"; break;
		case 'P': guess = L"SOM_PRM"; break;
		case 'S': guess = L"SOM_SYS"; break;
		case 'D': guess = L"SOM_DB"; break;
		}
	}
	else switch(toupper(exe[0]))		
	{
	case 'M': guess = L"MapComp"; break;
	case 'P': guess = L"PrtsEdit"; break;
	case 'I': guess = L"ItemEdit"; break;
	case 'O': guess = L"ObjEdit"; break;
	case 'E': guess = L"EneEdit"; break;
	case 'N': guess = L"NpcEdit"; break;
	case 'S': guess = L"SfxEdit"; break;
	}

	if(!guess) return false;

	size_t n = wcslen(guess);

	if(wcsnicmp(exe,guess,n)) return false;

	if(exe[n]&&wcsicmp(exe+n,L".exe")) return false;
											 
	wcscpy(SomEx_exe,guess); return true;
}

static void SomEx_assert_IsDebuggerPresent() 
{
	//NEW: pierce topmost splash screen
	//OLD: assert(IsDebuggerPresent());	
	//if(EX::debug&&!IsDebuggerPresent()) 
	//#ifdef _DEBUG //RelWithDebInfo //2021
	#if 0 || !defined(NDEBUG) || defined(RELWITHDEBINFO) //2022
	if(!IsDebuggerPresent())
	{
		//memos: CreateThread and DebugBreak are
		//for ? reason not viable in this context
		//vsjitdebugger is actually an improvement
		HWND splash = SOM::splash(); struct{static 
		VOID CALLBACK timer(HWND,UINT,UINT_PTR id,DWORD)
		{KillTimer(0,id),SendMessage(SOM::splash(),WM_CLOSE,0,0);}
		}splashdown; if(IsWindow(splash)&&SetTimer(0,0,1500,splashdown.timer))
		/*do{*/ EX::sleep(500); /*}while(!IsWindowVisible(splash));*/
		switch(MessageBoxA(0,"     " 
		"You may attach a debugger now or Retry to run vsjitdebugger.",
		"SomEx.dll - Asserting IsDebuggerPresent",
		MB_ABORTRETRYIGNORE|MB_SYSTEMMODAL|MB_SETFOREGROUND|MB_DEFBUTTON3))
		{
		case IDABORT: EX::is_needed_to_shutdown_immediately(0,"SomEx_assert_IsDebuggerPresent");
		case IDRETRY: if(!IsDebuggerPresent())
		{	
			//RISKY BUSINESS (DllMain)
			//ShellExecuteEx deadlocked on Windows 10 
			//even when som.exe.cpp preloaded Ole32.dll 
			//(CoInitializeEx was even called beforehand)			
			PROCESS_INFORMATION pi; STARTUPINFO si; 
			wchar_t cmd[60]; memset(&si,0x00,sizeof(si)); si.cb = sizeof(si); 
			swprintf_s(cmd,L"vsjitdebugger.exe -p %d",GetCurrentProcessId());
			if(CreateProcess(0,cmd,0,0,1,CREATE_SUSPENDED,0,0,&si,&pi))
			{
				AllowSetForegroundWindow(pi.dwProcessId);
				ResumeThread(pi.hThread); CloseHandle(pi.hThread);
				for(MSG msg;1;)
				switch(MsgWaitForMultipleObjects(1,&pi.hProcess,0,INFINITE,QS_ALLINPUT))
				{
				case 0: CloseHandle(pi.hProcess); /*__asm int 3*/ return;
				case 1: while(PeekMessageW(&msg,0,0,0,PM_REMOVE)) DispatchMessageW(&msg);
				}
			}
		}}	
	}
	#endif
}

static bool SomEx(bool reload=false) 
{
	//hack: let SOM_EX.exe override this
	LPWSTR (__stdcall *GotCommandLine)(); //@0: means no arguments?
	(void*&)GotCommandLine = GetProcAddress(GetModuleHandle(0),"_CommandLine@0");
	if(!GotCommandLine) GotCommandLine = GetCommandLineW;

	bool out = false;
	int argc = 0; wchar_t **argv =
	CommandLineToArgvW(GotCommandLine(),&argc);
	wchar_t *ext = argc?PathFindExtensionW(argv[0]):0;
	if(1&&SOM::game||1&&SOM::tool) SomEx_assert_IsDebuggerPresent();
	
	if(SOM::image()) //early out 
	{			
		//SomEx_tool here includes som_db.exe
		//SOM::retail = argc<=1||!SomEx_tool(argv[0],ext);
		SOM::retail = !SomEx_tool(argv[0],ext);

		/*KEEP 
		// 
		// _OUT_TO_STDERR calls abort on assert)
		// ANGLE doesn't use glDebugMessageCallback
		// in this mode, but outputs things that 
		// it doesn't otherwise
		//
		//2021: trying to catch ANGLE's stderr output
		//if(!AttachConsole(ATTACH_PARENT_PROCESS))
		if(1&&EX::debug&&SOM::game)
		{
			bool a = AttachConsole(ATTACH_PARENT_PROCESS);
			if(!a) AllocConsole();
			HWND c = GetConsoleWindow(); assert(c);
			ShowWindow(c,1);
			FILE *C2143;
			freopen_s(&C2143,"CONOUT$","w",stdout); clearerr(stdout);
			freopen_s(&C2143,"CONOUT$","w",stderr); clearerr(stderr); 
			//ANGLE doesn't make a peep with
			//_OUT_TO_MSGBOX
			_set_error_mode(_OUT_TO_STDERR);
			
			//TODO: Ex.ini configuration to turn this on by nonzero buffer size?
		//	CONSOLE_SCREEN_BUFFER_INFOEX csbi = {sizeof(csbi)};
		//	if(GetConsoleScreenBufferInfoEx(GetStdHandle(STD_ERROR_HANDLE),&csbi))
			{
		//		csbi.dwSize.Y = y;
		//		SetConsoleScreenBufferInfoEx(GetStdHandle(STD_ERROR_HANDLE),&csbi);
			}

			if(!a) CloseWindow(c); //minimize out of way of debugger
		}*/
	}
	else //flexible argument model reserves '-'	
	{	
		bool som = false;

		if(reload) //REMOVE ME??
		{
			//2018
			//HACK! (Maybe SOM_EX should unload SomEx.dll?)
			EX::closing_log();
			*const_cast<wchar_t*>(EX::log()) = '\0';
			*const_cast<wchar_t*>(EX::Workpath()) = '\0';
			extern void Ex_2018_reset_SOM_EX_or_SOM_EDIT_tool();
			Ex_2018_reset_SOM_EX_or_SOM_EDIT_tool();

			wchar_t *reset[] = {L".MO",L".SOM",L".MAP",L".NET",L".WS"};
			for(size_t i=0;i<EX_ARRAYSIZEOF(reset);i++) Sompaste->set(reset[i],L"");
		}		
		for(int i=0;i<argc;i++) if(*argv[i]!='-')
		{
			if(i) ext = PathFindExtensionW(argv[i]);
		
			if(SomEx_tool(argv[i],ext)) continue;

			//2017: these should probably be using Sompaste->set
			if(!wcsicmp(ext,L".MO"))
			{	
				SetEnvironmentVariableW(L".MO",argv[i]);
			}
			else if(!wcsicmp(ext,L".SOM"))
			{
				som = true;
				SetEnvironmentVariableW(L".SOM",argv[i]);
			}
			else 
			{
				const char *msg = 0; 
				const wchar_t *exe = 0;
				WIN32_FILE_ATTRIBUTE_DATA fad = {};
				GetFileAttributesEx(argv[i],GetFileExInfoStandard,&fad);
				
				if(fad.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
				{
					SetEnvironmentVariableW(L".NET",argv[i]);
				}				
				if(fad.nFileSizeLow>=98308) //save game?
				{			
					//NOTE: the player_file_extension extension only
					//governs writing files. it may be prohibitive to
					//load the config files at this stage, but they're
					//not yet loaded either way
					if(!som) if(FILE*f=_wfopen(argv[i],L"rb"))
					{
						char test2[4];
						if(fread(test2,4,1,f)&&!memcmp(test2,"SOM=",4))
						SetEnvironmentVariableW(L".SOM",argv[i]);
						fclose(f);
					}
				}
				else if(!*ext) //test map?
				{
					//if(!wcspbrk(argv[i],L"\\/"))
					{
						//For now this must be two digits.
						if(isdigit(argv[i][0])
						 &&isdigit(argv[i][1])&&!argv[i][2])
						SetEnvironmentVariableW(L".MAP",argv[i]); 
					}
				}
				else if(!wcsicmp(ext,L".PRT"))
				{
					//228 bytes
					wcscpy(SomEx_exe,L"PrtsEdit"); goto ws;
				}
				else if(!wcsicmp(ext,L".PRF"))
				{
					switch(fad.nFileSizeLow)
					{
					case 88: case 88+97: exe = L"ItemEdit"; break;
						//todo? have SfxEdit convert it?
					case 40: case 40+97:
						/*msg = "     "
						"Legacy personal magic PRF file doesn't have workshop tool.";
						goto msg;*/
						exe = L"SfxEdit"; break;
					case 108: case 108+97: exe = L"ObjEdit"; break;
					default: if(fad.nFileSizeLow>=384) 
					{
						//this is a new scheme introduced by Ene/NpcEdit to be able
						//to tell a large NPC profile apart from an enemy profile
						//NPC files with histories (written by NpcEdit) place an F
						//before the 97B history field, though any will do... and 
						//this could be used to further classify NPCs. likewise FX
						//is reserved for a coming SFX format that may or may not 
						//use PRF for its exension
						switch(fad.nFileSizeLow%4)
						{					
						//the magic words are lowercase to reduce the possibility
						//of the "history" beginning with
						//X and so appearing to be an FX
						case 2: exe = L"NpcEdit"; break; //f (one letters)
						case 3: exe = L"SfxEdit"; break; //fx (two letters)
						//0 is without a history field
						//1 is with... checking 564 is the old way that can fail if
						//NPCs have many SFX or SND references
						default: exe = fad.nFileSizeLow>=564?L"EneEdit":L"NpcEdit";
						}					
						break;
					}
					case 0: msg = "     "
					"PRF file does not exist or couldn't be classified by its size.";
					msg:MessageBoxA(0,msg,"SomEx.dll - Asserting IsDebuggerPresent",MB_OK);
					}
					if(!exe) continue;
					wcscpy(SomEx_exe,exe); ws: 
					//HACK: Can't fix this in workshop_CreateFileA
					wchar_t ws[MAX_PATH]; if(PathIsRelative(argv[i]))  
					wcscpy(ws+GetCurrentDirectory(MAX_PATH,ws)+1,argv[i])[-1] = '\\';				
					else *ws = '\0';
					SetEnvironmentVariableW(L".WS",*ws?ws:argv[i]);
				}
			}
		}			

		if(GetEnvironmentVariableW(L".NET",0,0)<=1
		//New Years 2018 note: Adding SOM_EX test so standalone
		//game setups cannot draw from InstDir.
		&&!wcsicmp(L"SOM_EX.exe",PathFindFileNameW(argv[0]))) //NEW
		{			
			wchar_t InstDir[MAX_PATH]; DWORD InstDir_s = sizeof(InstDir);

			if(!SHGetValueW(HKEY_CURRENT_USER,
			L"SOFTWARE\\FROMSOFTWARE\\SOM\\INSTALL",L"InstDir",0,InstDir,&InstDir_s))
			SetEnvironmentVariableW(L".NET",InstDir);
		}
		
		//command line oriented defaults
		if(!wcscmp(SomEx_exe,L"SOM_EX"))
		if(GetEnvironmentVariableW(L".SOM",0,0)<=1)
		{
			wcscpy(SomEx_exe,L"SOM_MAIN");
		}
		else if(GetEnvironmentVariableW(L".MAP",0,0)<=1)
		{
			//HACK: Som_h currently overrides this in
			//case a save file is detected
			wcscpy(SomEx_exe,L"SOM_EDIT"); 
		}
		else wcscpy(SomEx_exe,L"SOM_DB");

		out = true;
	}	

	LocalFree(argv); return out;
}

//static WNDPROC SomEx_OpenProc = 0;
static LRESULT CALLBACK SomEx_OpenCentered_etc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scID, DWORD_PTR ofn)
{
	OPENFILENAME* &p = (OPENFILENAME*&)ofn; 

	switch(uMsg)
	{
	case WM_SHOWWINDOW:	//center
	{			
		if(p->hwndOwner)
		{
			extern void som_tool_recenter(HWND,HWND,int);
			som_tool_recenter(hWnd,p->hwndOwner,0);
		}
		else
		{
			RECT wr; MONITORINFO mi = {sizeof(mi)};
			HMONITOR mon = MonitorFromRect(&wr,MONITOR_DEFAULTTONEAREST);
			if(wParam&&GetWindowRect(hWnd,&wr)&&GetMonitorInfo(mon,&mi))
			{
				int w = wr.right-wr.left, h = wr.bottom-wr.top;
				wr.left = mi.rcWork.left+(mi.rcWork.right-mi.rcWork.left-w)/2;
				wr.top = mi.rcWork.top+(mi.rcWork.bottom-mi.rcWork.top-h)/2;
				MoveWindow(hWnd,wr.left,wr.top,w,h,0);			
			}
		}

		//WINSANITY... GetOpenFileName no longer works for files located
		//in more than one place (2021: more than one place?)
		if(p->lpstrInitialDir&&*p->lpstrInitialDir)
		if(!*p->lpstrFile
		//2021: som_tool_SHBrowseForFolderA
		||p->lpstrFile==p->lpstrInitialDir) 
		{
			//assert(SOM::tool>=PrtsEdit.exe); //som_art_2021?
			if(wParam&&GetVersion()>=0x106) //Windows 7?
			if(HWND ec=(HWND)SendMessage(GetDlgItem(hWnd,1148),CBEM_GETEDITCONTROL,0,0))
			{
				SetWindowText(ec,p->lpstrInitialDir);
				SendMessage(hWnd,WM_COMMAND,IDOK,0);
				//if the directory doesn't change, the
				//text remains in the box, so it's this
				//or check if the directories are equal?
				SetWindowText(ec,L"");
			}
		}

		SetTimer(hWnd,'yuck',250*2,0);
		goto yuck; //break;
	}
	case WM_TIMER:

		if(wParam=='yuck')
		{
			KillTimer(hWnd,wParam); yuck:
				   		
			HWND SHELLDLL_DefView = GetDlgItem(hWnd,1121); 
			HWND lv = GetDlgItem(SHELLDLL_DefView,1);
			SetWindowTheme(lv,L"",L"");	

			//NEW: doing for ItemEdit and friends
			LVFINDINFOW lvfi = {LVFI_STRING,PathFindFileName(p->lpstrFile)};
			if(lvfi.psz&&*lvfi.psz)
			{
				//int test = ListView_GetItemCount(lv);
				int fi = ListView_FindItem(lv,-1,&lvfi);
				if(-1!=fi)
				{
					ListView_SetItemState(lv,fi,~0,LVIS_SELECTED|LVIS_FOCUSED);
					ListView_EnsureVisible(lv,fi,0);
					//PostMessage works here but fails ListView_EnsureVisible??
					SetTimer(hWnd,'nice',250/*50*/,0);					
				}
			}
			return 0;
		}
		if(wParam=='nice') //restore highlight
		{
			KillTimer(hWnd,wParam); 
			HWND cb = GetDlgItem(hWnd,1148);
			cb = (HWND)SendMessage(cb,CBEM_GETEDITCONTROL,0,0);
			PostMessage(cb,EM_SETSEL,0,MAKELPARAM(0,-1));
		}
		break;

	case WM_COMMAND:

		switch(wParam) 
		{
			//ComboBoxEx
		case MAKEWPARAM(1148,CBN_DROPDOWN):
			lParam = SendMessage((HWND)lParam,CBEM_GETCOMBOCONTROL,0,0);
			//break;
		case MAKEWPARAM(1136,CBN_DROPDOWN): 
		case MAKEWPARAM(1137,CBN_DROPDOWN): 		
		{
			COMBOBOXINFO cbi = {sizeof(cbi)};
			if(GetComboBoxInfo((HWND)lParam,&cbi))
			SetWindowTheme(cbi.hwndList,L"",L""); else assert(0);
			break;
		}}
		break;
			  	
	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,SomEx_OpenCentered_etc,scID);		
		break;
	}	
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
} 
extern UINT_PTR CALLBACK SomEx_OpenHook(HWND hwnd,UINT msg,WPARAM,LPARAM lp)
{	
	//Reminder: effects workshop_GetSaveFileNameA
	if(msg==WM_NOTIFY)
	{
		union
		{
			OFNOTIFYW *p;
			OFNOTIFYEX *ex; //CDN_INCLUDEITEM
		};
		p = (OFNOTIFYW*)lp; 
				
		//REMINDER: som_tool_openhook/SOM_MAIN_openhook defer to
		//SomEx_OpenHook for the following basic functionalities

		switch(p->hdr.code)
		{			
		case CDN_INITDONE:

			//SomEx_OpenProc = (WNDPROC)			
			//SetWindowLongPtrW(p->hdr.hwndFrom,GWL_WNDPROC,(LONG_PTR)Somtext_OpenCentered);			
			SetWindowSubclass(p->hdr.hwndFrom,SomEx_OpenCentered_etc,0,(DWORD_PTR)p->lpOFN);
			break;
						
	//	case CDN_INCLUDEITEM: //TESTING

	//ignored for files (SFGAO_FILESYSTEM)
			
			//LPVOID???
			//ex->pidl is LPITEMIDLIST
			//ex->psf is ???
	//		return 0;
		}
	}
	return 0;
}

//LNK1120 inside Sword_of_Moonlight???
extern void som_state_reprogram_image(); //som.state.cpp
SOMEX_API HMODULE Sword_of_Moonlight(const wchar_t *cwd) 
{		
	static bool reload = false;

	//2017: SomEx(reload) was called unconditionally, but
	//it it seems counterproductive and it was triggering
	//SomEx_assert_IsDebuggerPresent twice via DllMain
	if(!reload) 
	{
		reload = true; //NEW: SOM_EX.exe
	}
	else
	{
		//2018: moving up top from below so PRF/PRT files
		//can be found. hopefully doesn't break anything!
		if(cwd) SetCurrentDirectory(cwd);
		else assert(cwd);
		SomEx(reload); 
	}

	wchar_t som[MAX_PATH]; *som = '\0';
	GetEnvironmentVariableW(L".SOM",som,MAX_PATH);
	wchar_t net[MAX_PATH]; *net = '\0';
	GetEnvironmentVariableW(L".NET",net,MAX_PATH);
	const wchar_t *mo = Sompaste->get(L".MO",0);
	
	if(cwd) //0 implicates DllMain
	{
		wchar_t swap[MAX_PATH]; *swap = '\0';
		if(*som&&PathIsRelativeW(som))
		{		
			swprintf_s(swap,L"%ls\\%ls",cwd,som);
			wcscpy(som,swap);
		}
		if(mo&&*mo&&PathIsRelativeW(mo))
		{
			swprintf_s(swap,L"%ls\\%ls",cwd,mo);
			mo = Sompaste->set(L".MO",swap);
		}
	}

	if(!wcscmp(L"SOM_MAIN",SomEx_exe))
	{
		//allowing a manual SOM file
		//*som = '\0'; //projectless
	}
	else if(!wcscmp(L"SOM_RUN",SomEx_exe))
	{
		//automatically fillout form
		//*som = '\0'; //projectless
	}
	else if(cwd) //0 implicates DllMain
	{
		if(!*som) //FindFirstFile?
		{			
			WIN32_FIND_DATAW found; 

			int cat = -6+swprintf_s(som,L"%ls\\*.som",cwd);	
			
			HANDLE glob = FindFirstFileW(som,&found);

			if(glob!=INVALID_HANDLE_VALUE) 
			{
				wcscpy_s(som+cat+1,MAX_PATH-cat-1,found.cFileName);

				if(FindNextFileW(glob,&found)) som[cat] = '\0'; //force dialog

				FindClose(glob);
			}
			else som[cat] = '\0'; //force dialog
		}
		if(*som) //GetOpenFileName?
		{
			DWORD fa = 
			GetFileAttributesW(som);
			if(fa==INVALID_FILE_ATTRIBUTES
			 ||fa&FILE_ATTRIBUTE_DIRECTORY)
			if(wcsnicmp(L"SOM_",SomEx_exe,4)
			 &&wcsicmp(L"MapComp",SomEx_exe))
			{
				//2018: This excludes standalone projects
				//in order to not deal with ItemEdit, etc.
			}
			else //GetOpenFileName?
			{
				OPENFILENAMEW open = 
				{
					sizeof(open),0,0,
					L"Sword of Moonlight (*.som)\0*.som\0",	
					0, 0, 0, som, MAX_PATH, 0, 0, 0,
					L"Prefer Sword of Moonlight project", 
					//this hook is just to change the look
					OFN_ENABLEHOOK|OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_FILEMUSTEXIST,
					0, 0, 0, 0, SomEx_OpenHook, 0, 0, 0
				};
				if(PathIsDirectoryW(som)) wcscat_s(som,L"\\*.som");

				if(!GetOpenFileNameW(&open)) *som = '\0';			
			}
		}
		if(!*som) swprintf_s(som,L"%ls\\.som",cwd);
	}			
	
	//NOTE: Som_h returns immediately if "som" matches 
	//the last loaded SOM file
	//if(!SOM::image()) 
	{
		//setup environment variables
		Som_h(Sompaste->path(som),Sompaste->path(net));
		//NEW: SOM isn't necessarily the SOM file path
		//NEW: this can be redirected SOM file in 2020
		Sompaste->set(L".SOM",som);
	}

	if(SOM::tool) //obsolete?
	{		
		//REMOVE ME?
		EX::Workpath(SOM::Tool::install());
		EX::Workpath(L"tool");
	}
	else //REMOVE ME?
	{
		EX::Workpath(EX::cd()); 
	}	
		
	//REMOVE ME?
	if(EX::INI::Option()->do_log) EX::logging_on(); 

	int v[4] = {SOMEX_VERSION};

	/*NO LONGER APPROPRIATE*/
	EXLOG_HELLO(0) << "SomEx.dll " 
	<< v[0] << '.' << v[1] << '.' << v[2] << '.' << v[3]
	<< " attached\n";

	if(SOM::image()) //initialize
	{						
		//hack: detect new project
		//>: the virtual project folder
		if('>'==*Sompaste->get(L"GAME"))
		{
			//todo: open up Project Settings
			assert(SOM::tool==SOM_EDIT.exe); 

			FILE *new_som = _wfopen(som,L"wb");

			if(new_som) //hack
			{
				const char *game = 
				EX::need_ansi_equivalent(65001,
				PathFindFileNameW(SOM::Tool::project()));
				fputs(game,new_som);
				fputs("\r\n0\r\n\r\nEX=%EX%; Ex.ini",new_som);
				SOM::xcopy(L"Ex.ini",L"data\\my\\prof\\Ex.ini");
				fclose(new_som);
			}
			else assert(0);
	
			Som_h(0,net); //!!
		}
	
		//multi-byte charset for C runtimes
		//_configthreadlocale(_DISABLE_PER_THREAD_LOCALE);
		setlocale(LC_ALL,"Japanese_Japan.932");

		EXLOG_HELLO(0) << "Initializing off back of launcher\n";

		EX::INI::initialize(); //EX_CHECKPOINT(0)

		EX::detours = SomEx_Detours;
		
		EX::setting_up_Detours(EX::threadmain);			

		//REMOVE ME???
		//LNK1120 because SOMEX_API???
		//extern void //som.state.cpp
		//som_state_reprogram_image(); 
		som_state_reprogram_image();
		
		switch(SOM::image()) //NEW		
		{
		case 'edit': case 'prm': case 'map': case 'sys':
		
			if(!*Sompaste->get(L".MO")) 
			{
				SomEx_langs();
				Sompaste->set(L".MO",EX::Locales[0].catalog);
			}
		}
		if(SOM::game) SomEx_langs();
		if(SOM::game)
		for(int i=0;SomEx_fonts(EX::font(i));i++);
		if(SOM::tool)
		{
			wchar_t font[MAX_PATH]; *font = '\0';
			swprintf_s(font,L"%ls\\font",net);
			SomEx_fonts(font);
		}

		assert(cwd==0);
		//assuming DllMain called Sword_of_Moonlight(0)
		EXLOG_HELLO(0) << "Exiting DllMain entrypoint\n";
	}
	else //SOM::launch_legacy_exe(); 
	{
		assert(cwd!=0);

		SOM::launch_legacy_exe(); 

		//2018: If SOM_EX is always on, EX::log change with each new program.
		//It seems best to detach the log now, instead of when it is reopend.
		//It doesn't currently write to the log and doesn't have its own log.
		EXLOG_HELLO(0) << "Launcher closing log; anticipating next launch date...\n";
		EX::closing_log();
	}

	return EX::module;
}						   

extern HMODULE EX::module = 0; 
extern int EX::central_processing_units = 1;
extern DWORD EX::threadmain = SOM::thread();
extern bool EX::attached = false, EX::detached = false;
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch(ul_reason_for_call)
	{	
	case DLL_PROCESS_ATTACH: 

				_set_error_mode(_OUT_TO_MSGBOX);

#ifdef _DEBUG //compiler

		//_CRTDBG_LEAK_CHECK_DF floods output on debugger termination
		_crtDbgFlag = _CRTDBG_ALLOC_MEM_DF; //|_CRTDBG_LEAK_CHECK_DF;
		//_crtDbgFlag|=_CRTDBG_CHECK_ALWAYS_DF; //careful
#endif	
		EX::attached = true;

		EX::module = hModule;  
		
		//HACK: som.state.cpp is using this, and som_db.exe invokes
		//the clipper before receiving input.
		extern void som_mocap_speedometer(float[3]);
		EX::speedometer = som_mocap_speedometer;

		//breakpoints/asserts aren't working
		//here anymore???
		if(!SomEx()) Sword_of_Moonlight(0);

		break;

	case DLL_PROCESS_DETACH: 
				
		/////////////////////////////////////////
		//
		// TODO: free up EX::tls memory here
		//
		///////////////////////////////////////

		EXLOG_HELLO(0) << "SomEx.dll detached\n";
		
		EX::cleaning_up_Detours(0); //EX::threadmain

		EXLOG_HELLO(0) << "Have a nice day...\n";
					
		EX::abandoning_cursor(); //hacK??		

		EX::detached = true; break;
	}
    return TRUE;
} 

//REMOVE ME? (ALONG WITH win_iconv.c)
static size_t Ex_iconv_error(char *u=0, wchar_t *w=0)
{
	const char *err = 0; switch(errno)
	{
	case EILSEQ: if(u) strcpy(u,"EILSEQ"); if(w) wcscpy(w,L"EILSEQ");
	err = "FAILURE: Iconv encountered invalid multibyte sequence\n"; break;
	case EINVAL: if(u) strcpy(u,"EINVAL"); if(w) wcscpy(w,L"EINVAL");
	err = "FAILURE: Iconv encountered incomplete multibyte sequence\n"; break;
	case E2BIG: if(u) strcpy(u,"E2BIG"); if(w) wcscpy(w,L"E2BIG");
	err = "PANIC! Iconv ran out of room for conversion (woops!) \n"; break;		
	default: err = "FAILURE: Iconv returned undocumented error\n";
	}
	if(err) EXLOG_ALERT(1) << err; //std::cerr //2021

	return u?strlen(u):wcslen(w);
}

//REMOVE ME? (ALONG WITH win_iconv.c)
static iconv_t Ex_sjis2utf8 = 0, Ex_utf8tow = 0, Ex_wtoutf8 = 0;

extern char SomEx_utf_xbuffer[EX_MAX_CONVERT] = ""; //som_MDL_skin
extern int EX::Convert(int cp, const char **inout, int *inout_s, const wchar_t **wout)
{
	if(!inout||!*inout||!inout_s||*inout_s<0) return wout?0:cp;

	//REMINDER: old code probably depends on this not being shared
	//with the other Convert/Decode procs
	//static char utf_xbuffer[EX_MAX_CONVERT];
	auto &utf_xbuffer = SomEx_utf_xbuffer;

	const char *in = *inout; char *out = utf_xbuffer; 
	
	unsigned in_s = *inout_s, out_s = EX_MAX_CONVERT; 

	switch(cp)
	{
	default: return cp;

	case 65001: //UFT8 (already converted)
		
		if(!wout) return cp; out_s = EX_MAX_CONVERT-*inout_s; goto w; 

	case 932: //Microsoft Shift_JIS (Japanese)
		
		if(!Ex_sjis2utf8) Ex_sjis2utf8 = iconv_open("UTF-8","CP932"); 

		iconv(Ex_sjis2utf8,0,0,&out,&out_s); //initialize just in case

		while(in_s&&iconv(Ex_sjis2utf8,&in,&in_s,&out,&out_s)!=size_t(-1)); 
		
		if(in_s) goto err; //error	(conversion was not complete)
	}

	utf_xbuffer[EX_MAX_CONVERT-out_s] = '\0'; //paranoia??

	if(!wout) //wide conversion not necessary
	{
		*inout = utf_xbuffer; *inout_s = EX_MAX_CONVERT-out_s; 
		return 65001; //utf8 (Unicode)
err:	*inout_s = Ex_iconv_error((char*&)*inout=utf_xbuffer,0); 
		return 65001; //cp;
	}

	in = utf_xbuffer;

w:	if(!Ex_utf8tow) Ex_utf8tow = iconv_open("UTF-16LE","UTF-8"); 

	int utf8_s = EX_MAX_CONVERT-out_s; 

	in_s = utf8_s; out_s = EX_MAX_CONVERT*2; 

	static wchar_t utf_wbuffer[EX_MAX_CONVERT]; 
	
	out = (char*)utf_wbuffer; *utf_wbuffer = 0; //paranoia

	iconv(Ex_utf8tow,0,0,&out,&out_s); //initialize just in case

	while(in_s&&iconv(Ex_utf8tow,&in,&in_s,&out,&out_s)!=size_t(-1)); 
	
	if(in_s) goto werr; //error (conversion was not complete)
	
	utf_wbuffer[EX_MAX_CONVERT-out_s/2] = L'\0'; //paranoia??

	if(cp!=65001) //utf8 conversion was performed
	{
		*inout = utf_xbuffer; *inout_s = utf8_s; 
	}

	*wout = utf_wbuffer; return EX_MAX_CONVERT-out_s/2; 
	werr: return Ex_iconv_error
	((char*&)*inout=utf_xbuffer,(wchar_t*&)*wout=utf_wbuffer); 
}

extern int EX::Decode(int cp, const char **inout, int *inout_s) 
{
	if(cp==65001||inout_s&&*inout_s<0) return cp; //no decoding

	if(!inout) return !cp||cp==932?65001:cp;

	//REMINDER: old code probably depends on this not being shared
	//with the other Convert/Decode procs
	static char utf_xbuffer[EX_MAX_CONVERT]; 
	
	if(!*inout) *inout = ""; //best practice??

	int out_x = 0; if(!inout_s)
	{
		inout_s = &out_x; out_x = strlen(*inout);
	}

	const char *in = *inout; char *out = utf_xbuffer; 
	
	unsigned in_s = *inout_s, out_s = EX_MAX_CONVERT; 

	switch(cp)
	{
	default: return cp;

	case 65001: return 65001; //redundant reminder
		
	case 932: //Microsoft Shift_JIS (Japanese)
		
		if(!Ex_sjis2utf8) Ex_sjis2utf8 = iconv_open("UTF-8","CP932"); 

		iconv(Ex_sjis2utf8,0,0,&out,&out_s); //initialize just in case

		while(in_s&&iconv(Ex_sjis2utf8,&in,&in_s,&out,&out_s)!=size_t(-1)); 
		
		if(in_s) goto err; //error (conversion was not complete)
	}

	utf_xbuffer[EX_MAX_CONVERT-out_s] = '\0'; //paranoia??
	*inout = utf_xbuffer; *inout_s = EX_MAX_CONVERT-out_s;	  	
	return 65001; //utf8 (Unicode)
err: *inout_s = Ex_iconv_error((char*&)*inout=utf_xbuffer,0); 	
	return 65001; //cp;
}

extern int EX::Decode(const wchar_t *win, const char **xout, int *inout_s) 
{
	if(!win||!xout||inout_s&&*inout_s<0) return 0; //no decoding

	//REMINDER: old code probably depends on this not being shared
	//with the other Convert/Decode procs
	static char utf_xbuffer[EX_MAX_CONVERT];

	for(int out_x=0;!inout_s;inout_s=&out_x) out_x = wcslen(win);

	const char *in = (char*)win; char *out = utf_xbuffer; 
	
	unsigned in_s = *inout_s*2, out_s = EX_MAX_CONVERT;	 

	if(!Ex_wtoutf8) Ex_wtoutf8 = iconv_open("UTF-8","UTF-16LE"); 

	iconv(Ex_wtoutf8,0,0,&out,&out_s); //initialize just in case
	while(in_s&&iconv(Ex_wtoutf8,&in,&in_s,&out,&out_s)!=size_t(-1)); 	
	if(in_s) goto err; 
							 
	utf_xbuffer[EX_MAX_CONVERT-out_s] = '\0'; //paranoia??
	*xout = utf_xbuffer; *inout_s = EX_MAX_CONVERT-out_s; 	
	return 65001; //utf8 (Unicode)
err: *inout_s = Ex_iconv_error((char*&)*xout=utf_xbuffer,0); 	
	return 65001; //0
}					

extern BYTE *som_state_43FE70_lore;
extern char *som_state_40a8c0_menu_text;		
extern RECT som_state_40a8c0_menu_yesno[2]; 
static int SomEx_trans(void *procA, HDC hdc, LPCSTR *txt, int *len)
{
	//if(1) //EXPERIMENTAL
	{
		//HACK: what this is doing is restoring zwrite for after
		//the menu elements, while also disabling depth test for
		//text, so the below the line text draws over the frames 
		extern void som_scene_zwritenable_text();
		if(!*(DWORD*)0x1D6A248) //som_scene_state::&writeenable
		som_scene_zwritenable_text();
	}

	static bool shading = false;
	DWORD c = GetTextColor(hdc);
	DWORD c2; switch(c2=c|=0xFF000000) //!
	{
	case 0xff00ffff: //yellow
		
		//this is junk text SOM mysteriously renders to a texture
		//that is never used for anything. it would appear on the
		//screen (the whole texture) in the form of an odd glitch
		return *len = 0;

	case 0xff464646: //grey
		
		shading = true; //knockout next round
	
		if(SOM::dialog!=SOM::frame)
		c2 = EX::INI::Script()->system_fonts_contrast; 
		if(c2==0xff464646) 
		c2 = EX::INI::Adjust()->lettering_contrast; break;

	case 0xffdcdcdc: //white
		
		if(shading) procA = 0; //signal knockout

		if(SOM::dialog!=SOM::frame)		
		c2 = EX::INI::Script()->system_fonts_tint; 				
		if(c2==0xffdcdcdc) 
		c2 = EX::INI::Adjust()->lettering_tint; //break;	
		
	default: shading = false;		
	}	
	if(procA) //block translation?
	if(som_state_40a8c0_menu_text) //Yes or No?
	{
		if(**txt==*SOM::title()) //'\a' 
		*txt = som_state_40a8c0_menu_text;
		if(*txt!=SOM::title('MAP2',*txt)) procA = 0;
	}
	else if(som_state_43FE70_lore) //Truth Glass
	{	
		if(**txt==*SOM::title()) //'\a'
		*txt = (char*)som_state_43FE70_lore;
		bool npc = som_state_43FE70_lore>SOM::L.NPC_prm_file->uc;
		if(*txt!=SOM::title(npc?'PRM5':'PRM4',*txt)) procA = 0;
	}
	//side-effect: track translation state
	int out = SOM::title(procA,*txt,*len);
	if(c2!=c&&c2>>24) //todo: transparency 
	SetTextColor(hdc,c2<<16&0xFF0000|c2&0xFF00FF00|c2>>16&0xFF); 
	return c2>>24?out:*len=0; //!
} 
extern unsigned som_hacks_tint;
extern bool som_game_continuing;
extern float som_game_menucoords[2];
extern int EX::Translate(void *procA, HDC hdc, LPCSTR *txt, int *len, LPRECT *box, UINT *how, int*)
{
	int out = SomEx_trans(procA,hdc,txt,len);
	if(!*len) return out; //illustrating
			
	if(**txt=='<'||**txt=='>') //shifting
	{	
		int bs = 0, sh = **txt;
		if(GetCharWidthI(hdc,sh,1,0,&bs)&&sh=='>') 
		bs = -bs; 
		DDRAW::doSuperSamplingDiv(bs);
		while(**txt==sh)
		{
			if(*how&DT_RIGHT)
			(*box)->right-=bs; else (*box)->left-=bs;			
			(*txt)++; (*len)--; 
		}
	}	
	else if(**txt=='$') //2018: were did this go to?
	{
		(*txt)++; (*len)--; 
		*how|=DT_CENTER|DT_SINGLELINE;
	}
		
	if(som_state_40a8c0_menu_text) //choice?
	{
		RECT cr = **box; //todo? non-single-line 
		if(DrawTextW(hdc,EX::need_unicode_equivalent(out,*txt),-1,&cr,*how|DT_CALCRECT))
		if(som_state_40a8c0_menu_yesno[0].left)
		som_state_40a8c0_menu_yesno[cr.top>som_state_40a8c0_menu_yesno[0].top] = cr;
		else som_state_40a8c0_menu_yesno[0] = cr;
	}
	else if(som_hacks_tint==SOM::frame||som_game_continuing)
	{
		//2018: squaring menus for VR
		//and so frames are identical
		OffsetRect(*box,som_game_menucoords[0],som_game_menucoords[1]);		
	}

	return out;
}
extern int EX::Translate(void *procA, HDC hdc, int *x, int *y, LPCSTR *txt, int *len, int *clipx) 
{	
	int out = SomEx_trans(procA,hdc,txt,len);
	if(!*len) return out; //illustrating

	//FIX ME
	//
	// Ex_detours_TextOutA is implementing this
	// logic until I can refactor it (which may
	// well be never)
	//
	/*2022: TA_CENTER (6) includes TA_RIGHT (2)
	static int uncenter = 0; if(uncenter) 
	{
		int ta = GetTextAlign(hdc)&uncenter;

		SetTextAlign(hdc,ta); uncenter = 0;
	}
	if(SOM::title_pixels<0||**txt=='$') //centering
	{
		if(**txt=='$'){ (*txt)++; (*len)--; } 

		if(!(GetTextAlign(hdc)&TA_CENTER))
		{	
			SetTextAlign(hdc,TA_CENTER);
			*x = SOM::width/2;
			uncenter = ~TA_CENTER;	
		}
	}
	else *clipx = SOM::title_pixels;*/

	if(**txt=='<'||**txt=='>') //shifting
	{	
		int bs = 0, sh = **txt;
		if(GetCharWidthI(hdc,sh,1,0,&bs)&&sh=='>')
		bs = -bs;
		DDRAW::doSuperSamplingDiv(bs);
		while(**txt==sh)
		{
			*x-=bs; (*txt)++; (*len)--; 
		}
	}
			
	//2018: squaring menus for VR
	//and so frames are identical
	if(som_hacks_tint==SOM::frame||som_game_continuing)
	{
		*x+=som_game_menucoords[0]; *y+=som_game_menucoords[1];		
	}

	return out; 
}

extern HDC EX::hdc(HDC *release)
{
	if(release)
	{
		if(*release)
		{
			int ok = 0;

			if(EX::window)
			{
				ok = ReleaseDC(EX::window,*release);
			}
			else ok = ReleaseDC(DDRAW::window,*release);

			if(!ok) ReleaseDC(0,*release);
		}

		return 0;
	}
	else
	{
		HDC out = 0;

		if(EX::window)
		{
			out = GetDC(EX::client);
		}
		else out = GetDC(DDRAW::window);

		if(!out) //don't understand this stuff
		{
			out = GetDC(0);
		}

		return out;
	}
}

extern unsigned EX::tick()
{
	//2020: som_db.exe actually uses timeGetTime
	//it's DirectX that's calling GetTickCount?!
	//004020BB FF 15 14 82 45 00    call        dword ptr ds:[458214h]
	int compile[sizeof(unsigned)==sizeof(DWORD)];
	return timeGetTime(); //GetTickCount();
}

extern int som_tool_initializing;
extern void EX::vblank()
{	
	if(som_tool_initializing) return; //HACK: open windows without delay?

	//HACK: not bothering with vb->Release()
	static DX::IDirectDraw7 *vb = DDRAW::DirectDraw7?DDRAW::DirectDraw7:DDRAW::vblank();
	if(vb) vb->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN,0);
}

static EX::_sysenum SomEx_editor_no
(EX::Calculator*, const wchar_t *_, size_t, float *out)
{	
	*out = EX::NaN; return EX::constant;
}

static EX::_sysenum SomEx_system_no
(EX::Calculator*, const wchar_t *_, size_t, float *out)
{	
	*out = EX::NaN; 
	if(*_!='_') return EX::constant;
	switch(_[1]) //assuming L"_WALK" etc.
	{
	case 'W': *out = SOM::PARAM::Sys.dat->walk; break;
	case 'D': *out = SOM::PARAM::Sys.dat->dash; break;
	case 'T': *out = SOM::PARAM::Sys.dat->turn; break;
	}
	return EX::mismatch; //error
}

extern int SomEx_pc = 12288,SomEx_npc = -1; //NaN
static EX::_sysenum SomEx_pc_or_npc(int pc, size_t i, float *out)
{	
	if(pc==-1) return EX::variable;
		
	switch(i) 
	{				
	case 0: 
	{
		if(EX__INI__Number___MODE<0) //REMOVE ME
		{
			i = 16; goto _1_2_1_12_scale;
		}

		/*2020: this was the old encoding (som_state_404470)
		*out = pc&0xFFFF;
		EX::INI::Adjust ad;
		if(&ad->character_identifier)
		*out = ad->character_identifier(*out,(float)(pc>>16));
		*/
		int ai = pc&4095; switch(pc&3<<12)
		{
		case 0x0000: *out = SOM::L.ai[ai][SOM::AI::enemy]; break;
		case 0x1000: *out = SOM::L.ai2[ai][SOM::AI::npc]; break;
		case 0x2000: *out = SOM::L.ai3[ai][SOM::AI::object]; break;
		default: assert(0);
		}
		if(!EX::isNaN(*out))
		{
			EX::INI::Adjust ad;
			if(&ad->character_identifier)
			*out = ad->character_identifier(*out,(float)ai);
		}
		else assert(0); break;
	}
	case 1: case 2: //case 3:
	
		if(12288!=pc)
		{	
			if(EX__INI__Number___MODE<0) //REMOVE ME
			{
				*out = 0; break;
			}

			//SAME AS som_state_404470_strength_and_magic
			SOM::xxiix sm;
			//int prm = pc&4095; assert(prm<1024); 
			int prm,ai = pc&4095; 
			switch(pc&3<<12) //YUCK
			{
			case 0x0000: //enemy

				prm = SOM::L.ai[ai][SOM::AI::enemy];
				//WARNING: 282 isn't 4B aligned (assuming correct)
				sm = *(int*)(SOM::L.enemy_prm_file[prm].c+282);
				break;

			case 0x1000: //NPC

				prm = SOM::L.ai2[ai][SOM::AI::npc];
				//WARNING: 302 isn't 4B aligned (assuming correct)
				sm = *(int*)(SOM::L.NPC_prm_file[prm].c+302);			
				break;

			case 0x2000: //trap
			{
				prm = SOM::L.ai3[ai][SOM::AI::object];
				WORD *p = (WORD*)(SOM::L.obj_prm_file[prm].c+38);
				sm[0] = p[0]; sm[1] = p[8];
				break;
			}}
			*out = sm.ryoku(i);
		}	
		else *out = SOM::L.pcstatus[i==1?SOM::PC::str:SOM::PC::mag]; 
			
	case 3: break; //Speed (or?) (reserved)

	default: //Equipment?
			
		if(i>4+7) return EX::mismatch;

		if(12288==pc) *out = SOM::L.pcequip[i-4]; 
		
		break;

	case 12: //NEW: player_character_bob provision

		//TODO? detect #2 animation?
		*out = 12288!=pc?0:SOM::motions.dash; 
		
		break;

		_1_2_1_12_scale: //REMOVE ME

	case 13: case 14: case 15: //xyz
	case 16: //scale

		if(12288!=pc) 
		{
			//NOTE: this was broken before 1.3.2.4
			//(ai was the PRM index)
			int ai = pc&4095;
			int aj = i==16?
			SOM::AI::_scale:SOM::AI::xyz+(i-13);			
			switch(pc&3<<12)
			{
			case 0x0000: //enemy

				*out = SOM::L.ai[ai].f[aj];
				break;

			case 0x1000: //NPC

				//NEW: looks like SOM::AI::xyz2 
				//is 2 less than "SOM::AI::xyz" 
				if(aj!=SOM::AI::_scale) aj-=2;
				*out = SOM::L.ai2[ai].f[aj];
				break;

			case 0x2000: //trap

				//HACK: SOM::AI::xyz3 is 1 less
				//HACK: SOM::AI::scale3 is 1 more
				aj+=aj==SOM::AI::_scale?1:-1;
				*out = SOM::L.ai3[ai].f[aj];
				break;
			}
		}
		else if(i==16) 
		{
			*out = EX::INI::Player()->player_character_scale; 
		}
		else *out = SOM::xyz[i-13]; break;

	case 17: //2020: compass?

		break; //RESERVING;

	case 18: case 19: //2020: hp/max
	case 20: case 21: //2020: mp/max 

		if(12288!=pc) 
		{
			int ai = pc&4095; switch(pc&3<<12)
			{
			case 0x0000: //enemy

				if(i&1)
				{
					int j = SOM::L.ai[ai][SOM::AI::enemy];
					*out = SOM::L.enemy_prm_file[j].s[148+(i==21)];
				}
				else *out = (WORD)SOM::L.ai[ai].s[SOM::AI::hp+(i==20)];
				break;

			case 0x1000: //NPC

				if(i<=19) if(i&1) //MP?
				{
					int j = SOM::L.ai2[ai][SOM::AI::npc];
					*out = SOM::L.NPC_prm_file[j].s[1];
				}
				else *out = SOM::L.ai2[ai][SOM::AI::hp2];
				break;
			}
		}
		else *out = SOM::L.pcstatus[i-14]; break;

	case 22: //2020

		if(12288==pc) *out = SOM::L.pcstatus[SOM::PC::xxx];

		break;
	}
	return EX::variable; 
}
static EX::_sysenum SomEx_builtin_no
(EX::Calculator*, const wchar_t *c, size_t num, float *out)
{
	*out = EX::NaN; //lazy

	//REMEMBER TO UPDATE EX::assigning_number(0,L"pc",_no,13);

	switch(c[0])
	{
	case 'c': if(c[1]) return EX::mismatch; 
		
		//if(SOM::L.counters)
		{
			if(num<1024) *out = SOM::L.counters[num]; 
		}

	break;	
	case 'n': 
		
		if(c[1]!='p'||c[2]!='c'||c[3]) 
		return EX::mismatch; 
		return SomEx_pc_or_npc(SomEx_npc,num,out);

	case 'p': 
		
		if(c[1]!='c'||c[2]) 
		return EX::mismatch; 
		return SomEx_pc_or_npc(SomEx_pc,num,out);

	break;
	default: return EX::mismatch; 
	}
	return EX::variable; 
}

extern void EX::numbers()
{
	EX::resetting_numbers_to_scratch(0);
	
	//CRASHING TOOLS
	//if(!SOM::game) return;
	EX::including_number_r(0);
	EX::including_number_id(0);
	if(SOMEX_VNUMBER>=0x102010EUL) 
	EX::assigning_number(0,L"_MODE",L"0");
	//SomEx_editor_no is always NaN
	EX::_sysnum _no = SomEx_editor_no;
	if(SOM::game) _no = SomEx_builtin_no;
	//if(SOM::L.pcstatus)
	EX::assigning_number(0,L"pc",_no,17);
	//2020: guess I forgot to add npc???
	EX::assigning_number(0,L"npc",_no,17); 
	//if(SOM::L.counters)
	EX::assigning_number(0,L"c",_no,1024);
	//FIX ME?
	//the [Number] section is initialized
	//by tools too :(
	if(SOM::game
	||SOM::Tool::project()) //YUCK: crashes SOM_FILES_FOPEN
	if(SOM::PARAM::Sys.dat->open())
	for(int i=0;i<1024;i++) if(*SOM::PARAM::Sys.dat->counters[i])
	{
		wchar_t c[] = L"c_0000_"; const wchar_t *c_label = 
		EX::need_unicode_equivalent(932,SOM::PARAM::Sys.dat->counters[i]);		
		if(swprintf(c,L"c_%d_",i)>0) EX::assigning_number(0,c,c_label);
	}			   
	if(SOM::game) _no = SomEx_system_no;
	if(SOM::game) SOM::PARAM::Sys.dat->open();	
	EX::assigning_number(0,L"_WALK",_no,1);
	EX::assigning_number(0,L"_DASH",_no,1);
	EX::assigning_number(0,L"_TURN",_no,1);

#ifdef _DEBUG //testing

	//EX::debugging_number(0,L"test_1",L"1+ 2");
	//EX::debugging_number(0,L"test_2",L"2 +3");
	//EX::debugging_number(0,L"test_3",L"3 + 4");
	//EX::debugging_number(0,L"test_4",L"4 + -5");
	//EX::debugging_number(0,L"test_5",L"5 +-6");
	//EX::debugging_number(0,L"test_6",L"-6+7");
	//EX::debugging_number(0,L"test_7",L"-7--8");
	//EX::debugging_number(0,L"test_8",L"-8 --9");
	//EX::debugging_number(0,L"test_9_a",L"8(c)");
	//EX::debugging_number(0,L"test_10",L"c[a]");
	//EX::debugging_number(0,L"test_12",L"pow(2,2)");
	//EX::debugging_number(0,L"test_13",L"(2,2)-|1,1|");
	//EX::debugging_number(0,L"test_14",L"()-((3,3)*(4,4))");
	//EX::debugging_number(0,L"test_15_label",L"test[1+2][label](3+4)");
	
	//EX::debugging_number(0,L"test_0",L"abs(2)c");
	//EX::debugging_number(0,L"test_1",L"abs(2)c[1+2](3+4)");
		
	//EX::debugging_number(0,L"add_1_x",L"%n(1_)");
	//EX::debugging_number(0,L"add_2_y",L"%n(1_)+3");
	//EX::debugging_number(0,L"add_0_f",L"_[x]+_[y]");	
	//EX::debugging_number(0,L"test_0",L"add");
	
	//EX::debugging_number(0,L"test_0",L"n(abs(1)c[1])");

	//EX::debugging_number(0,L"test_0",L"pow(2,2)");

	//EX::debugging_number(0,L"_WALK",L"_$*2");

	//EX_INI_NUMBER(-100000,0,100000) test, test2, test3, test4;
								
	//test4 = test3 <<= test2 <<= test <<= L"2 4 2+4 1_S*2_S";

	//float _S3 = test4; //breakpoint
													 													 
	//EX_INI_SERIES(-100000,test_numbering,100000) test5;

	//const float (&_5678)[4] = test5 = L"5 6r 7 8"; 	

	//test = L"1_+2_"; float A = test(1,0.5);

#endif
}
						
extern int EX::directx()
{
	return 7==EX::INI::Window()->directx_version_to_use_for_window?7:9;
}
extern int EX::context()
{
	if(!SOM::paused)
	if(!SOM::recording)
	switch(SOM::context)
	{
	case SOM::Context::limbo: return 3; 

	case SOM::Context::playing_game: return 0; 

	//NEW: assume 2 is a prompt
	//case SOM::Context::saving_game: 
	//case SOM::Context::browsing_menu:	
	//case SOM::Context::browsing_shop: //NEW
	default:

		//can't remember why 2 exists but
		//returning 1 while saving_game on
		//the Yes/No cancels it immediately
		//(SEEMS SO BUTTONS DON'T CARRY OVER)	
		return SOM::doubleblack>=SOM::frame-1?2:1;

	case SOM::Context::reading_menu: return 2;
	}
	return 2;
}

extern bool EX::alt()
{	
	//return SOM::alt==SOM::frame;
	bool alt = SOM::alt==SOM::frame;

	//don't do this if triple button running
	if(SOM::space==SOM::frame) return alt;

	//2020: ctrl and shift imply alt combinations since the
	//function overlay keys don't require ctrl or shift and
	//would otherwise require alt
	return alt||SOM::ctrl==SOM::frame||SOM::shift==SOM::frame;
}

extern bool EX::numlock()
{
	if(!EX::INI::Option()->do_numlock)
	{
		return GetKeyState(VK_NUMLOCK)&1;
	}
	else return SomEx_numlock;	
}

extern DWORD SomEx_recapture_tick = 0; //2022
extern bool EX::arrow()
{
	//FYI: a lot of hacks are crammed 
	//in here for sake of convenience
	//(there used to be a lot more!!) 
	//return EX::context()==0;

	//// weak capture semantics ////

	static int previous = 0;

	static bool recapture = false;

	int context = EX::context();
	
	bool paused = DDRAW::isPaused;

	if(!SOM::capture)
	if(paused||previous!=context)
	{	
		//2020: alternating between captured
		//when pausing???
		//if(paused||EX::is_captive())
		if(EX::is_captive())
		{				
			//game >> menu/picture
			//if(context&&!previous) //suspend
			if(paused||context&&!previous) //2020
			{	
				EX::releasing_cursor();
				if(!EX::crosshair)
				EX::unclipping_cursor();

				recapture = true;
			}
		}
		else if(recapture //restore?
		||EX::tick()-SomEx_recapture_tick<=250) //2022
		{
			//menu/picture >> game 
			if(!context&&previous)
			{
				EX::capturing_cursor();
				EX::clipping_cursor();
			}	
		}
	}

	if(recapture) //restore conditions
	{
		//probably should not be possible
		if(SOM::capture) recapture = false; 

		if(EX::inside&&EX::active&&!EX::crosshair)
		{
			//cursor was moved basically
			//TODO: movement threshold??
			if(EX::cursor) recapture = false; 
		}	
	}
	
	previous = context; 

	return context==0&&'hand'!=EX::pointer;
}

extern int EX::tilt()
{
	static int out = 0;

	static unsigned perframe = 0;

	if(perframe==SOM::frame) return out;
		
	if(abs(SOM::tilt)<45) //hump
	{		
		if(out<0&&SOM::tilt>0) out = 0;
		if(out>0&&SOM::tilt<0) out = 0;

		if(out)
		{
			float per = min(60.0f/EX::fps+0.5f,1.0f);

			out = out>0?max(out-per,0):min(out+per,0);			
		}

		if(SOM::tilt&&abs(out)>15)
		{
			out = out>0?15:-15;
		}

		out+=SOM::tilt; 				
	}
	else out = SOM::tilt>0?45:-45; 
	
	SOM::tilt = 0;

	perframe = SOM::frame;

	return out;
}

extern void EX::pause(int ch)
{
	return SOM::Pause(ch);
}

extern void EX::unpause(int ch)
{
	return SOM::Unpause(ch);
}

//Fyi: Sword of Moonlight 
//has a 2 frame input window at 60fps
static const size_t Ex_char_input_rrepeat = 2;
static const size_t Ex_char_input_repeat = 24; //12
static const size_t Ex_char_input_limit = 128;
static const size_t Ex_wide_input_limit = 256; //todo: unlimited

static char Ex_char_input_[128+1+2];
static wchar_t Ex_wide_input_[256+1+2];

static char *Ex_char_input = Ex_char_input_+2;
static wchar_t *Ex_wide_input = Ex_wide_input_+2;

static int Ex_char_input_lead = 0;
static int Ex_char_input_term = 0;

static size_t Ex_char_input_width = 0;
static size_t Ex_char_input_length = 0;

static bool Ex_char_input_return = 0;
static bool Ex_char_input_escape = 0;

static unsigned Ex_char_input_escaped = 0; //hack

static int Ex_char_input_mode = 0;

//-1: backwards tabbing code
static char Ex_char_inputs[0x39+1] = 
{
0,
'\x1b', //ESCAPE
'1','2','3','4','5','6','7','8','9','0',
'-', //MINUS
'+', //EQUALS
'\x08', //BACK   
'\t', //TAB    
'q','w','e','r','t','y','u','i','o','p',
0,0, //LBRACKET/RBRACKET
'\r', //RETURN
0, //LCONTROL
'a','s','d','f','g','h','j','k','l',
0, //SEMICOLON
0, //APOSTROPHE
-1, //GRAVE 
-1, //LSHIFT
'\t', //BACKSLASH
'z','x','c','v','b','n','m',
0, //COMMA
'.', //PERIOD
'/', //SLASH
-1, //RSHIFT
0, //MULTIPLY
0, //LMENU
' ' //SPACE
};
static const size_t Ex_char_inputs_s = sizeof(Ex_char_inputs);

static unsigned Ex_char_input_states[Ex_char_inputs_s];
static unsigned Ex_char_input_frames[Ex_char_inputs_s] = {1}; 
//0x1C: DIK_RETURN (switching over to using DIK codes)
static unsigned &Ex_char_input_frames_r = Ex_char_input_frames[0x1C];

static bool Ex_can_input(unsigned char unx)
{
	if(!Ex_char_input_mode) return false;

	char in = unx<Ex_char_inputs_s?Ex_char_inputs[unx]:0;

	switch(in) //control characters
	{			
	case '\t': case -1: //-1: backwards tabbing

	case '\x1b': case '\x08': case '\r': return true;
	}
	
	switch(Ex_char_input_mode)
	{
	case 'tab': return in;
	case 'pos': return in>='0'&&in<='9';
	case 'int':	return in>='0'&&in<='9'||in=='-'||in=='+';
	case 'num':	return in>='0'&&in<='9'||in=='-'||in=='+'||in=='.';
	case 'hex':	return in>='a'&&in<='f'||in>='0'&&in<='9';

	default: assert(0); return false;
	}
}

static char Ex_input(unsigned char unx)
{
	if(unx>=Ex_char_inputs_s) return 0;
	
	if(Ex_char_input_frames[0]==1) //hack
	{
		memset(Ex_char_input_states,0,sizeof(Ex_char_input_states));
		memset(Ex_char_input_frames,0,sizeof(Ex_char_input_frames));

		Ex_char_input_frames[Ex_char_input_lead] = DDRAW::noFlips;
	}
		
	char in = Ex_char_inputs[unx];

	if(!in||Ex_char_input_frames[unx]==DDRAW::noFlips) return 0;

	if(Ex_char_input_escaped>=DDRAW::noFlips-2) //hack
	{
		if(in=='\x08'||in=='\x1b'||unx==Ex_char_input_term) 
		{
			Ex_char_input_escaped = DDRAW::noFlips;
		} 

		return 0; //prevent escape key spillage
	}
	else if(Ex_char_input_frames_r>=DDRAW::noFlips-Ex_char_input_rrepeat)
	{
		if(in!='\r') return 0; //block input on return
	}

	Ex_char_input_term = 0; //manual termination timeout

	if(unx==Ex_char_input_lead)
	{
		if(Ex_char_input_frames[unx]>=DDRAW::noFlips-2) 
		{
			Ex_char_input_frames[unx] = DDRAW::noFlips; return 0;
		}
		else Ex_char_input_lead = 0; //suppression satisified
	}
	else if(in=='\r'&&Ex_char_input_frames[unx]>DDRAW::noFlips-Ex_char_input_rrepeat)
	{
		Ex_char_input_frames_r = DDRAW::noFlips; return 0;		
	}
	else if(Ex_char_input_states[unx]>=DDRAW::noFlips-2)
	{
		if(Ex_char_input_frames[unx]>DDRAW::noFlips-Ex_char_input_repeat)
		{
			Ex_char_input_states[unx] = DDRAW::noFlips; return 0; //repeating
		}
	}
	
	if(Ex_char_input_states[unx]<DDRAW::noFlips-2)
	{
		Ex_char_input_frames[unx] = DDRAW::noFlips;
	}

	Ex_char_input_states[unx] = DDRAW::noFlips;

	if(Ex_char_input_return||Ex_char_input_escape) return 0;

	static unsigned backspace = 0; 

	switch(in) //control characters
	{
	case '\t': case -1: //tabs
		
		return in;

	case '\x08': //backspace
		
		if(Ex_char_input_length==0
		  &&backspace<DDRAW::noFlips-Ex_char_input_repeat-1) 
		{								   
			Ex_char_input_escape = true; 			
			Ex_char_input_escaped = DDRAW::noFlips; //hack
			return '\x1b'; //soft escape sequence
		}
		else if(Ex_char_input_length)
		{
			Ex_char_input_length--; 
		}

		backspace = DDRAW::noFlips;
		return '\x08';

	case '\x1b': //escape
	
		Ex_char_input_escape = true; 		
		Ex_char_input_escaped = DDRAW::noFlips; //hack
		return '\x1b';
	
	case '\r': //return
	
		Ex_char_input_return = true; 
		return '\r';
	}

	if(!Ex_can_input(unx)) return 0;

	if(Ex_char_input_length<Ex_char_input_limit)
	{
		Ex_char_input[Ex_char_input_length] = in;
		Ex_wide_input[Ex_char_input_length] = in;

		Ex_char_input_length++;	
	}

	return in;
}

extern bool EX::validating_direct_input_key(unsigned char unx, const char unxaltetc[256])
{
	//ALT: override direct input modes
	if(unxaltetc[0x38]||unxaltetc[0xB8]) return true; 

	if(Ex_char_input_escaped>=DDRAW::noFlips-2) //hack
	{
		return unx>=Ex_char_inputs_s||!Ex_char_inputs[unx];
	}
		
	if(Ex_char_input_mode&&!Ex_char_input_escape) return !Ex_can_input(unx);	

	return true;
}

namespace som_game_menu
{
	extern bool translate_dik(BYTE);
}
extern void som_mocap_Ctrl_Shift_cancel_attacks();
static bool SomEx_alt()
{
	//don't do this if triple button running
	if(SOM::space==SOM::frame) return false;

	return SOM::alt==SOM::frame||SOM::alt2==SOM::frame; 
}
extern int SomEx_shift()
{
	//NOTE: Ctrl/Shift now imply Alt is pressed also
	if(SOM::shift==SOM::frame||SOM::ctrl==SOM::frame)
	{
		//HACK: doing here so to avoid doing individually
		som_mocap_Ctrl_Shift_cancel_attacks();

		return SOM::shift==SOM::frame?-1:1;
	}
	return GetAsyncKeyState(VK_SHIFT)>>15?-1:1;
}
extern bool som_mocap_allow_modifier();
extern unsigned char EX::translating_direct_input_key(unsigned char x, const char xaltetc[256])
{	
	//2021: WinKey+G opens XBox overlay?
	if(!x&&xaltetc[0xdb]) 
	{
		assert(0x80==(BYTE)xaltetc[0xdb]);

		SOM::context = SOM::Context::limbo;
	}

	bool alt = xaltetc[0x38]||xaltetc[0xB8];

	switch(x)
	{
	case 0x4C: case 0xC8: //UP
	case 0x50: case 0xD0: //DOWN
	case 0x4F: case 0xCB: //LEFT
	case 0x51: case 0xCD: //RIGHT
	case 0xD1: case 0xC9: //NEXT/PRIOR 	 
	case 0xC7: case 0xCF: //HOME/END

		if(1==EX::context())
		{
			if(som_game_menu::translate_dik(x))	
			return 0;
		}

	case 0: //NEW: canceling SomEx_alt
		
		if(xaltetc[0x39]) SOM::space = SOM::frame; break;
	}

	switch(x)
	{		   
	case 0x38: case 0xB8: //LMENU/RMENU 
			
		//&&: toggle emu/do_2000 mode
		if(xaltetc[0x38]&&xaltetc[0xB8])
		{
			SOM::alt2 = SOM::frame; 

			//2017: need to work with Alt+Alt+M
			if(GetKeyState(VK_MENU)>>15) //2020: macro?
			EX::activating_cursor();
		}
		SOM::alt = SOM::frame; return x; //hack
			
	case 0x57: //F11

		//HACK: having problem with minimizing
		//in VR, so let macros specify CONTROL
		//to prevent this
		if(xaltetc[0x1D]||xaltetc[0x9D])
		{
			if(!SOM::windowed) return 0;
		}
		break;

	case 0xC5: //PAUSE

		if(alt) //Alt+Ctrl+Pause/Break
		{
			SOM::play = !SOM::play;	return 0;
		}
		break;

	case 0x39: //SPACE

		//2018: WHY alt??? It's forcing to stand
		//up while ducking. Probably System Menu
		//Will suppress it...
		if(/*alt||*/!SOM::player)
		return 0; 
		break;

	case 0x58: //F12 //2021: translate to +/- for repeat?

		//int mute = x==0x58?0:x==0x0C?-1:+1;
		//if(!mute)
		{
			bool ctrl = SOM::ctrl==SOM::frame;
			bool shift = SOM::shift==SOM::frame;
			if(ctrl==shift)
			{
				//mute = 0; //mute function
			}
			else //mute = sh;
			{
				return SomEx_shift()==-1?0x0C:0x0D; //DIK_MINUS/EQUALS
			}
		}
		break;

	//case 0x2a: case 0x36: //LSHIFT/RSHIFT (Attack)
	case 0x2a: case 0x36: //LSHIFT/RSHIFT (Magic)

		//NEW: Ctrl+Shift facility		
		if(som_mocap_allow_modifier())
		{
			SOM::shift = SOM::frame; //NEW
			if(SOM::ctrl==SOM::frame)
			{
				SOM::alt2 = SOM::frame; //Ctrl+Shift=Alt+Alt
				//simplifies Alt+Alt combos
			//	som_mocap_Ctrl_Shift_cancel_attacks();
			}
		}

		if(!SOM::player
		||alt&&!SOM::L.arm_MDL->ext.d2) return 0; 

		//2020: woops, I don't if som.mocap.cpp even
		//knows what to do with 0x36?
		x = 0x2a; break;

	//case 0x1d: case 0x9d: //LCONTROL/RCONTROL (Magic)
	case 0x1d: case 0x9d: //LCONTROL/RCONTROL (Attack)

		//NEW: Ctrl+Shift facility
		if(som_mocap_allow_modifier())
		{
			SOM::ctrl = SOM::frame; 
			if(SOM::shift==SOM::frame) 
			{
				SOM::alt2 = SOM::frame; //Ctrl+Shift=Alt+Alt
				//simplifies Alt+Alt combos
			//	som_mocap_Ctrl_Shift_cancel_attacks();
			}
		}

		if(!SOM::player
		||alt&&SOM::L.arm_MDL->d<=1) return 0; 

		//2020: woops, I don't if som.mocap.cpp even
		//knows what to do with 0x9d?
		x = 0x1d; break;

	case 0x2C: //case 0x1E: //Z/A (see WASD)
		
		return 0; //free up alphabet for input
	
	case 0xD2: //INSERT->Z
		
		if(alt||!SOM::player) return 0;

		return EX::context()?0xD0:0x2C; //DOWN/Z
		
	case 0xC9: //PRIOR->A (page up)

		if(alt||!SOM::player) return 0;

		return EX::context()?0xC8:0x1E; //UP/A

	case 0x4B: case 0x4D: //NUMPAD4/6

		if(!SOM::player) return 0;

		if(EX::context()) 
		{
			return x==0x4B?0xCB:0xCD; //LEFT/RIGHT
		}
		break;

	case 0x47: case 0x49:            //NUMPAD7/9	
	case 0xD3: case 0xD1: case 0xCF: //DELETE/NEXT/END

		return SOM::player?x:0;

	case 0xC8: case 0xD0: //UP/DOWN (+ALT)
	case 0x4C: case 0x50: //NUMPAD5/NUMPAD2 (+ALT)

		if(!SOM::player) return 0;

		//2020: making it so pressing both buttons on the controller
		//can switch to vertical mode. for some reason "alt" must be
		//tested to get the keys
		//if(alt) //L/RMENU
		//if(SOM::L.f4?!*SOM::L.f4:SOM::game==som_db.exe&&EX::f4)
		if(!SOM::L.f4&&(alt||SomEx_alt())) 
		{
			som_mocap_Ctrl_Shift_cancel_attacks();

			return x==0x4C||x==0xC8?0x1E:0x2C; //A/Z
		}	   
		break;
		
	case 0xCB: case 0xCD: //LEFT/RIGHT (+ALT)
	case 0x4F: case 0x51: //NUMPAD1/NUMPAD3 (+ALT)

		if(!SOM::player) return 0;

		if(alt)
		{
			return x==0x04F||x==0xCB?0x4B:0x4D; //NUMPAD4/NUMPAD6
		}	   
		break;

	case 0x4A: case 0x4E: //NUMPADMINUS/NUMPADPLUS

		if(alt||!SOM::player) return 0;

		return x==0x4A?0x1E:0x2C; //A/Z
								
	case 0x0E: case 0x01: //left handed Esc
		
		//letting Alt+Back restore volume
		//return !alt&&SOM::player?0x01:0; //BACK/ESCAPE
		if(!alt) return SOM::player?0x01:0; //BACK/ESCAPE
		break;

	case 0x1C: case 0x0F: //left handed menu
		
		if(alt||!SOM::player) return 0; //RETURN
		
		return EX::context()<2?0x0F:0x1C; //TAB/RETURN

	case 0x11: case 0x19: case 0x1E: case 0x26:
	case 0x1F: case 0x27: case 0x20: case 0x28: //WASD
	{	
		if(!SOM::player) return 0;  

		EX::INI::Option op;	if(!op->do_wasd) return 0;

		unsigned char (*out)(unsigned char,const char[256]);
		
		out = EX::translating_direct_input_key; //hack: alt combos

		if(x==0x11||x==0x19) return out(0x4C,xaltetc); //W/P/NUMPAD5
		if(x==0x1F||x==0x27) return out(0x50,xaltetc); //S/SEMICOLON/NUMPAD2

		EX::INI::Detail dt;

		int mode = EX::context()==1?1:0; //menus left/right
		
		if(!mode) mode = dt->wasd_and_mouse_mode; 
		if(!mode) mode = op->do_mouse?2:1; mode = mode%2?1:2; //one or two handed

		if(EX::context()==1) mode = 1; 

		//A/L/NUMPAD4/NUMPAD1
		if(x==0x1E||x==0x26) return out(mode==1?0x4F:0x4B,xaltetc); 
		//D/APOSTROPHE/NUMPAD6/NUMPAD3
		if(x==0x20||x==0x28) return out(mode==1?0x51:0x4D,xaltetc); 

		break;
	}}

	return x;
}
extern int Ex_output_overlay_focus; //HACK
extern void EX::broadcasting_direct_input_key(unsigned char x, unsigned char unx)
{	
	static bool keypressed = 0;
	static int alt2_inputs = 0;	
	
	if(!x&&!unx) //synchronization
	{	
		if(SOM::alt==SOM::frame) 
		{
			if(SOM::altdown<SOM::alt-1) 
			if(GetKeyState(VK_MENU)>>15) //2020: macro?
			{
				if(EX::is_captive()&&!SOM::capture) //weak hold
				{
					EX::abandoning_cursor(); 
				}

				EX::alternating_cursor(true); //hold

				//it seems intuitive to switch to weak-capture mode in this case 
				//note that more ways are to wait for the timeout or right click
				if(!EX::active&&EX::pointer=='z') SOM::capture = EX::pointer = 0;
			}
		}
		else if(keypressed)
		{
			//get cursor out of the way
			EX::activating_cursor();
			EX::hiding_cursor();
			EX::following_cursor();
									
			keypressed = false;
		}

		SOM::altdown = SOM::alt; 
				
		if(alt2_inputs!=0&&SOM::alt2<SOM::frame)
		{
			//REMOVE ME?
			//Alt+Alt? This can probably use a third input?
			if(1==alt2_inputs) SOM::emu = !SOM::emu; 

			alt2_inputs = 0;
		}

		return;
	}

	if(SOM::context==SOM::Context::limbo) //2021: Win key?
	{
		return;
	}
		
	//NOTE: Ctrl+Shift now triggers alt2 in addition to Alt+Alt
	if(SOM::alt2==SOM::frame) 
	{
		//alt2_inputs|=1; //SOM::emu is now done upon release!

		if(x==0x32&&!EX::context()) //Alt+Alt+M (map)
		{			
			int sh = SomEx_shift(); //cancels shield

			if(0==(4&alt2_inputs))
			{
				alt2_inputs|=4;
				SOM::map = 0==SOM::map?sh:sh?SOM::map+sh:0;
			}
			else SOM::map+=sh; 			
		}
		else if(x==0x2F) //2020: Alt+Alt+V (vector mode)
		{
			//set D3DRENDERSTATE_FILLMODE
			//note som_hacks_Clear must assist this
			//0040249E 8B 0D 0C A1 45 00    mov         ecx,dword ptr ds:[45A10Ch]
			DWORD &rs = SOM::L.fill; rs = rs==2?3:2;
			alt2_inputs|=2;
		}
		else if(x==0x2D) //2020: X?
		{
			int &i = DSOUND::doReverb_i3dl2[0];

			if(SOM::ctrl==SOM::frame)
			{
				i = i==29?0:i+1;
			}
			else if(SOM::shift==SOM::frame)
			{
				i = i==0?29:i-1;
			}
			else 
			{
				DSOUND::doReverb_mask = (1+DSOUND::doReverb_mask)%3;
			}

			alt2_inputs|=2;
		}
		else //if(x!=0x38&&x!=0xB8) //("emulation"?)
		{
			/*2020: E? //making "emu" harder to input
			//prevent SOM::emu if there are any inputs at all
			alt2_inputs|=2;*/
			if(x==0x12) //E
			if(GetKeyState(VK_MENU)>>15) //require keyboard input
			{
				alt2_inputs|=1; //emu
			}
		}
	}//hack: alt combinations
	else if(SOM::alt==SOM::frame) 
	//if(x>=0x3B&&x<=0x43||x>=0x57&&x<=0x66  //F1~9, 11, 12
	 //||x>=0x47&&x<=0x53||x>=0xC7&&x<=0xD3) //numpad, arrows etc
	{			
		//Note: Alt+Alt handled below
		if(x!=0x38&&x!=0xB8&&x!=0x44) 
		if(GetKeyState(VK_MENU)>>15||GetKeyState(VK_F10)>>15) //2020: macro?
		{
			keypressed = true; //Alt/F10
		}

		if(unx==0x3E) x = 0x3E; //hack: ensure Alt+F4 combo
	}	
	else if(unx==0x44) x = 0x44; //hack: ensure F10 function

	if(!SomEx_alt())
	if(Ex_char_input_mode&&!Ex_char_input_escape
	 ||Ex_char_input_escaped>=DDRAW::noFlips-2) //hack
	{													   
		if(!Ex_input(unx)) switch(unx) 
		{
		//tab	   backslash  lshift     //rshift   //grave
		case 0x0F: case 0x2B: case 0x2A: case 0x36: case 0x29:
			
			unx = 0; //hack: throttle tabs
		}
	}

	EX::INI::Output tt;
	EX::INI::Option op;	EX::INI::Detail dt;	
	
	//force Alt+F4 regardless of keymap
	if(unx==0x3E&&SOM::alt==SOM::frame) x = unx; 

	//hack: should track the up/down state here
	if(Ex_char_input_escaped>DDRAW::noFlips-10) switch(unx)
	{
	case 0x01: case 0x08: return; //hack: some problem keys
	}

	switch(x)
	{
	case 0x01: //ESCAPE
		
		if(0==EX::context())
		SOM::escape(SomEx_shift()); //analog
		break;
			  
	case 0x45: //NUMLOCK 
	
		if(op->do_numlock) SomEx_numlock = !SomEx_numlock;
		break;	

	case 0x22: //G (toggle gauge)

		if(SOM::Game.taking_item) //2022 (take item menu?)
		{
			if(!SOM::L.on_off[3])
			{
				//TODO: toggle lower-third text?
				//maybe even an item stats view?
			}			
		}
		else SOM::L.on_off[1] = !SOM::L.on_off[1]; //gauge
		break;
	case 0x23: //H

		//NEW: let PS button hide stubborn cursor?
		if(!EX::crosshair)
		if(!EX::is_cursor_hidden())
		return EX::hiding_cursor();

		//simulate double-click to show crosshair?
		EX::crosshair = !EX::crosshair;
		SOM::f10(); //HACK
		SendMessage(SOM::window,WM_LBUTTONUP,0,0); //HACK
		break;
	case 0x24: //J (toggle compass?)

		if(SOM::Game.taking_item) //2022 (take item menu?)
		{
			//HACK: som_game_410620 
			auto swap = (char*)&SOM::L.bgm_file_name[MAX_PATH-1];
			auto mute = (char*)&SOM::L.sys_data_menu_sounds_suite;
			std::swap(*swap,*mute);
			SOM::L.on_off[3] = !SOM::L.on_off[3];
		}
		else SOM::L.on_off[2] = !SOM::L.on_off[2]; //compass
		break;

	case 0x3B: //F1

		if(SomEx_alt()||!tt->f(1))
		{
			//2021: this was going to be a game manual system but
			//super-sampling takes priority. the help system can be
			//rolled into the system menu, it can remember its page
			//and be toggled with Esc

			/*Help?
			//2020: quick and dirty solution
			if(const wchar_t*url=EX::INI::Author()->online_website_to_visit)
			if(*url&&(HINSTANCE)33>ShellExecuteW(EX::display(),L"openas",url,0,0,1))
			ShellExecuteW(EX::display(),L"open",url,0,0,1);
			*/
			SOM::altf1(); //super sampling mode
		}
		break;

	case 0x3C: //F2

		if(SomEx_alt()||!tt->f(2))
		{
			SOM::altf2(); //PSVR mode
		}
		break;

	case 0x3D: //F3

		if(EX::alt()||!tt->f(3)) //SomEx_alt
		{
			SOM::altf3(SomEx_shift()); //zoom mode
		}
		break;

	case 0x3E: //Alt+F4 (close!)
	
		if(SOM::alt==SOM::frame) //||!tt->f(4) //SomEx_alt
		{
			//2018: forcing alt+f4 to use the keyboard because 
			//it's possible to press a button that uses an alt
			//macro and an f4 (no-clip) macro at the same time
			if(GetKeyState(VK_MENU)>>15&&GetKeyState(VK_F4)>>15)
			{
				SOM::altf4(); //cleaner than SOM's ALT+F4

				return; //2020
			}
		}
		break;

	case 0x44: //F10 

		//WARNING: Windows doesn't send an up event for this 
		//and restores the cursor to its prior state instead
		//so dx.dinput.cpp is sending two extra events after
		//it detects a release. that's just a short term fix
		SOM::f10(); break;

	case 0x57: //F11

		if(SomEx_alt()||!tt->f(11))
		{
			//NOTE: CONTROL SUPPRESSES F11
			//IN fullscreen MODE SO IT DOESN'T
			//ACCIDENTALLY MINIMIZE
			SOM::altf11(); //fullscreen
		}
		break;

	/*these change the system volume level
	case 0xA0: //DIK_MUTE
	case 0xB0: //DIK_VOLUMEUP
	case 0xAE: //DIK_VOLUMEDOWN
	SOM::altf12(x==0xA0?0:x==0xB0?1:-1); break;*/
	case 0x58: //F12
	case 0x0C: //DIK_MINUS
	case 0x0D: //DIK_EQUALS
	case 0x0E: //DIK_BACKSPACE

		//triple button running?
		if(SOM::space==SOM::frame) break;

		if(EX::alt()||!tt->f(12) //SomEx_alt
		||!Ex_output_overlay_focus&&(x==0x0C||x==0x0D))
		{
			int sh = SomEx_shift();

			//HACK: have Back restore the max volume
			//F12 had done this when unmuting but it
			//now restores the previous muted volume
			if(0x0E==x)
			{
				//HACK: backspace mirrors Esc for the menus
				//this is more of an emergency fix strategy
				if(!SomEx_alt()||~GetKeyState(VK_MENU)>>15) 
				break;

				x = 0x58; sh = 0;

				SOM::masterVol = 0;
			}

			//F12 mutes/unmutes
			//- or Shift+F12 lowers volume
			//+/= or Ctrl+F12 raises volume
			int mute = x==0x58?0:x==0x0C?-1:+1;
			if(!mute)
			{
				bool ctrl = SOM::ctrl==SOM::frame;
				bool shift = SOM::shift==SOM::frame;
				if(ctrl==shift)
				{
					mute = 0; //mute function
				}
				else mute = sh;
			}
			else //if(sh==-1) mute = -mute; //???
			{
				//2021: translating_direct_input_key
				//is not converting F12 into -/+ for
				//a simple way to add repeat support
				//NOTE: I'm not sure if shift on +/-
				//was desired for some reason but it
				//doesn't work with this translation
				//pattern (some may shift to type +)
			}

			SOM::altf12(mute);
		}
		break;
		case 0x0B: //0 (close to +/-
		if(!Ex_output_overlay_focus)
		{
			SOM::altf12(0); //mute
		}
		break;

		//HACK	
	case 0xC5: case 0xA2: //DIK_PAUSE/DIK_PLAYPAUSE
	
		if(DDRAW::isPaused) //center VR on unpause?
		{
			//REMINDER: it takes a while for the "head" extension
			//to get settled in, and so it would be nice to be able
			//to pause without recentering. but I don't know what the
			//toll of pausing is on VR, and generally, centering is the
			//first thing you do when playing with VR, and so needs to be
			//as convenient as it can
			SOM::PSVRToolbox("Recenter");
		}
		break;

	case 0x29: //GRAVE

		if(SomEx_alt()) 
		{
			EX::logging_onoff(); //Alt+`
		}
		break;
	}

	static bool doing_piano = false;

	if(x==0x29&&DSOUND::doPiano) //DIK_GRAVE
	{
		doing_piano = !doing_piano;
		return;
	}
	if(doing_piano)
	{
		int key = DINPUT::Qwerty(x,0,0);
		if(key)	return DSOUND::Piano(key-1);
	}

	if(!SomEx_alt()||x==0x3D||x==0x3E) //F4 F3
	{
		//REMOVE ME? (doing pause)
		DDRAW::multicasting_dinput_key_engaged(x);

		if(!SOM::paused||x==0x3E||x==0x3D) //NEW
		EX::simulcasting_output_overlay_dik(x,unx);
	}
	else if(DDRAW::isPaused&&x==0xC5) //PAUSE
	{
		//got stuck when paused twice on 5/25/2015
		DDRAW::multicasting_dinput_key_engaged(x);
		assert(0||!DDRAW::isPaused);
	}

	if(EX::is_playing_movie())
	{
		switch(x)
		{
		case 0x01: //DIK_ESCAPE

			EX::stop_playing_movie(); break;
		}
	}
}
 
bool EX::requesting_ascii_input(int mode, int lead)
{		
	if(Ex_char_input_mode) 
	{
		if(mode!='new') return false;

		mode = Ex_char_input_mode;
		
		Ex_char_input_length = 0;
	}
						   
	if(Ex_char_input_width>1) 
	{
		//was suspended in unicode mode
		EX::cancelling_requested_input();
	}

	if(mode<128)
	{
		lead = mode; mode = 'tab';
	}

	if(lead<0||lead>=Ex_char_inputs_s) lead = 0;

	Ex_char_input_mode = mode; 
	Ex_char_input_lead = lead;

	Ex_char_input_frames[lead] = DDRAW::noFlips;
		
	Ex_char_input_return = 0;
	Ex_char_input_escape = 0;
	
	Ex_char_input_width = 1; //ascii

	return true;
}

extern const char EX_returning_ascii[] = "\0\0<";
extern const char *EX::returning_ascii = EX_returning_ascii+2;

extern const char *EX::displaying_ascii_input(const char *cursor, size_t cutoff)
{
	assert(Ex_char_input_width==1);

	if(Ex_char_input_escape
	 ||Ex_char_input_width!=1) return "\0\0"+2; //hack

	if(Ex_char_input_return
	  ||Ex_char_input_frames_r
	  >=DDRAW::noFlips-Ex_char_input_rrepeat)
	{
		return EX::returning_ascii;
	}

	if(!cutoff)
	{
		cutoff = Ex_char_input_limit;
	}
	else if(cutoff<Ex_char_input_length)
	{
		Ex_char_input_length = cutoff;
	}

	Ex_char_input[Ex_char_input_length] = '\0';
	Ex_wide_input[Ex_char_input_length] = '\0';

	if(cursor) 
	if(cutoff!=Ex_char_input_length)
	{	
		size_t m = Ex_char_input_length, n = cutoff-m;

		if(DDRAW::noFlips%16>8) 
		{
			char *ws = "                          "; //hack

			strncpy(Ex_char_input+m,ws,n);
		}
		else strncpy(Ex_char_input+m,cursor,n=min(n,strlen(cursor)));
		
		Ex_char_input[Ex_char_input_length+n] = '\0';		
	}
	
	Ex_char_input[-1] = Ex_char_input_length;
	Ex_char_input[-2] = Ex_char_input_length;

	return Ex_char_input;
}

extern const wchar_t EX_returning_unicode[] = L"\0\x0\u21b2";
extern const wchar_t *EX::returning_unicode = EX_returning_unicode+2;

extern const wchar_t *EX::displaying_unicode_input(const wchar_t *cursor, size_t cutoff)
{
	assert(Ex_char_input_width==1);

	if(Ex_char_input_escape
	  ||Ex_char_input_width!=1) return L"\0\0"+2; //hack

	if(Ex_char_input_return	
	  ||Ex_char_input_frames_r
	  >=DDRAW::noFlips-Ex_char_input_rrepeat)							   
	{
		return EX::returning_unicode;
	}

	if(!cutoff)
	{
		cutoff = Ex_char_input_limit;
	}
	else if(cutoff<Ex_char_input_length)
	{
		Ex_char_input_length = cutoff;
	}

	Ex_char_input[Ex_char_input_length] = '\0';
	Ex_wide_input[Ex_char_input_length] = '\0';

	if(cursor) 
	if(cutoff!=Ex_char_input_length)
	{
		size_t m = Ex_char_input_length, n = cutoff-m;

		if(DDRAW::noFlips%16>8) 
		{
			wchar_t *ws = L"                          "; //hack

			wcsncpy(Ex_wide_input+m,ws,n);
		}
		else wcsncpy(Ex_wide_input+m,cursor,n=min(n,wcslen(cursor)));
		
		Ex_wide_input[Ex_char_input_length+n] = '\0';	
	}
	
	Ex_wide_input[-1] = Ex_char_input_length;
	Ex_wide_input[-2] = Ex_char_input_length;

	return Ex_wide_input;
}

extern int EX::inquiring_regarding_input(bool complete)
{
	if(!Ex_char_input_mode) return 0;

	if(complete)
	{
		if(!Ex_char_input_escape&&!Ex_char_input_return) return 0;
	}

	return Ex_char_input_mode;
}

extern const char EX_inputting_ascii[] = "\x0\x0";
extern const char *EX::inputting_ascii = EX_inputting_ascii+2;

extern const char *EX::retrieving_ascii_input(const char *esc)
{
	if(!Ex_char_input_mode
	  ||Ex_char_input_width!=1)	return esc;

	const char *out = 0;

	if(!Ex_char_input_escape) 
	{		
		if(!Ex_char_input_return) return EX::inputting_ascii;

		Ex_char_input[Ex_char_input_length] = '\0';

		Ex_char_input[-1] = Ex_char_input_length;
		Ex_char_input[-2] = Ex_char_input_length;

		out = Ex_char_input;
	}
	else 
	{
		//assuming should be inforced
		EX::cancelling_requested_input();

		out = esc;
	}

    Ex_char_input_return = 0;
	Ex_char_input_escape = 0;

	return out;
}

double Ex_char_input_numerical_default = -0.0;

extern double EX::retrieving_numerical_input(double esc)
{			
	if(!Ex_char_input_mode
	  ||Ex_char_input_width!=1)	return esc;

	double out = Ex_char_input_numerical_default;

	if(!Ex_char_input_escape) 
	{		
		if(!Ex_char_input_return) return out;

		Ex_char_input[Ex_char_input_length] = '\0';

		out = strtod(Ex_char_input,0);

		switch(Ex_char_input_mode)
		{		
		case 'pos': //NEW
		case 'int': out = (long long)out; case 'num': break;

		default: return Ex_char_input_numerical_default;
		}
	}
	else 
	{
		//assuming should be inforced
		EX::cancelling_requested_input();

		out = esc;
	}

    Ex_char_input_return = 0;
	Ex_char_input_escape = 0;

	return out;
}

extern void EX::suspending_requested_input(int term)
{
	Ex_char_input_mode = 0; 

	if(Ex_char_input_term=term) 
	{
		Ex_char_input_escaped = DDRAW::noFlips; //hack
	}
}

extern void EX::cancelling_requested_input(int term)
{
	Ex_char_input_mode = 0; 
	
	if(Ex_char_input_term=term) 
	{
		Ex_char_input_escaped = DDRAW::noFlips; //hack
	}

	Ex_char_input_length = 0;
	Ex_char_input_return = 0;
	Ex_char_input_escape = 0;
	Ex_char_input_width = 0; 
}

#define SOMEX_CONFIGURE(am,ppx,ppy,ppz,npx,npy,npz,prx,pry,prz,nrx,nry,nrz,psx,psy,nsx,nsy,du,dr,dd,dl,b1,b2,b3,b4,b5,b6,b7,b8,b9,dz,sa,menu) \
{\
	/*int analog_mode[8][EX::contexts]*/\
	{\
	{am,0,0,0},{am,0,0,0},{am,0,0,0},{am,0,0,0},\
	{am,0,0,0},{am,0,0,0},{am,0,0,0},{am,0,0,0},\
	},\
	/*unsigned short pos_pos[3][EX::contexts]*/\
	{{ppx,ppx,ppx,0},{ppy,ppy,ppy,0},{ppz,ppz,ppz,0}},\
	/*unsigned short neg_pos[3][EX::contexts]*/\
	{{npx,npx,npx,0},{npy,npy,npy,0},{npz,npz,npz,0}},\
	/*unsigned short pos_rot[3][EX::contexts]*/\
	{{prx,prx,prx,0},{pry,pry,pry,0},{prz,prz,prz,0}},\
	/*unsigned short neg_rot[3][EX::contexts]*/\
	{{nrx,nrx,nrx,0},{nry,nry,nry,0},{nrz,nrz,nrz,0}},\
	/*unsigned short pos_aux[2][EX::contexts]*/\
	{{psx,psx,psx,0},{psy,psy,psy,0}},\
	/*unsigned short neg_aux[2][EX::contexts]*/\
	{{nsx,nsx,nsx,0},{nsy,nsy,nsy,0}},\
	/*unsigned short pov_hat[8][EX::contexts]*/\
	{{du,du,du,0},{dr,dr,dr,0},{dd,dd,dd,0},{dl,dl,dl,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}},\
	/*unsigned short buttons[32][EX::contexts]*/\
	{\
	{b1,b1,b1,0},{b2,b2,b2,0},{b3,b3,b3,0},{b4,b4,b4,0},\
	{b5,b5,b5,0},{b6,b6,b6,0},{b7,b7,b7,0},{b8,b8,b8,0},\
	{b9,b9,b9,0},{ 0, 0, 0,0},{ 0, 0, 0,0},{ 0, 0, 0,0},\
	{ 0, 0, 0,0},{ 0, 0, 0,0},{ 0, 0, 0,0},{ 0, 0, 0,0},\
	{ 0, 0, 0,0},{ 0, 0, 0,0},{ 0, 0, 0,0},{ 0, 0, 0,0},\
	{ 0, 0, 0,0},{ 0, 0, 0,0},{ 0, 0, 0,0},{ 0, 0, 0,0},\
	{ 0, 0, 0,0},{ 0, 0, 0,0},{ 0, 0, 0,0},{ 0, 0, 0,0},\
	{ 0, 0, 0,0},{ 0, 0, 0,0},{ 0, 0, 0,0},{ 0, 0, 0,0},\
	},\
	/*unsigned short menu[EX::contexts]*/\
	{menu,menu,menu,0},\
	/*unsigned short deadzone[8]*/\
	{\
	{dz,dz,dz,0},{dz,dz,dz,0},{dz,dz,dz,0},{dz,dz,dz,0},\
	{dz,dz,dz,0},{dz,dz,dz,0},{dz,dz,dz,0},{dz,dz,dz,0},\
	},\
	/*unsigned short saturation[8]*/\
	{\
	{sa,sa,sa,0},{sa,sa,sa,0},{sa,sa,sa,0},{sa,sa,sa,0},\
	{sa,sa,sa,0},{sa,sa,sa,0},{sa,sa,sa,0},{sa,sa,sa,0},\
	},\
};

static EX::Joypad::Configuration SomEx_mouse_configuration()
{
	EX::INI::Option op; EX::INI::Detail dt;

	int mode = dt->wasd_and_mouse_mode;

	if(!mode) mode = op->do_wasd&&op->do_mouse?2:1;

	switch(mode) //left/right buttons 
	{
	case 2:	case 3: mode = 0; break; //event/attack

	default: mode = 1; break; //walk forward/back
	}

	int compile[EX::contexts==4];
	EX::Joypad::Configuration out = SOMEX_CONFIGURE
	(
	1,//am
	0x51,//ppx Turn right (DIK_NUMPAD3)
	0x47,//ppy Look up (DIK_NUMPAD9)
	0,//wheel,//ppz
	0x4F,//npx Turn left (DIK_NUMPAD1)
	0x49,//npy Look down (DIK_NUMPAD7)
	0,//wheel,//npz
	0x4D,//prx (tilt) Move right (DIK_NUMPAD6)
	0,//pry 
	0,//prz
	0x4B,//nrx (tilt) Move left (DIK_NUMPAD4)
	0,//nry
	0,//nrz
	0,//psx
	0,//psy
	0,//nsx
	0,//nsy
	0,//du
	0,//dr
	0,//dd
	0,//dl
	//left mode1:forward mode2:event
	mode?0x4c:0x39, //b1 DIK_NUMPAD5/SPACE
	//right mode1:backward mode2:attack
//	mode?0x50:0x2a, //b2 DIK_NUMPAD2/LSHIFT
	mode?0x50:0x1D, //b2 DIK_NUMPAD2/LSHIFT //2020
	//middle
	0x48,//b3 Look center (DIK_NUMPAD8)	
	0x0F,//b4 bonus: TAB
	mode?0x39:0x1D,//b5 //bonus: SPACE/MAGIC
	0,//b6
	0,//b7
	0,//b8
	0,//b9 
	5,//dz
	0,//sa
	0 //menu
	)	

	//2020: I don't know if this is best but
	//I can't get this to work in Ex.input.cpp
	//with digital???
	//out.analog_mode[0][1] = 1; //menu navigation?
	//out.analog_mode[1][1] = 1; //menu navigation?

	out.pos_pos[0][1] = 0x51;
	out.neg_pos[0][1] = 0x4F;	
	out.pos_pos[1][1] = 0x50;
	out.neg_pos[1][1] = 0x4C;	
	out.buttons[0][1] = 0x39; //SPACE
	out.buttons[1][1] = 0x01; //ESCAPE
	out.buttons[3][1] = 0x01; //ESCAPE
	out.buttons[4][1] = 0x0F; //TAB
	out.pos_pos[0][2] = 0;
	out.neg_pos[0][2] = 0;
	out.pos_pos[1][2] = 0x50; //Yes
	out.neg_pos[1][2] = 0x4C; //No
	out.buttons[0][2] = 0x39; //SPACE
	out.buttons[1][2] = 0x01; //ESCAPE	

	out.deadzone[0][0] = 0; //warning!
	out.deadzone[1][0] = 0; //not safe

	out.menu[0] = mode?0x44:0; //F10

	unsigned short *a = out.a(); size_t z = out.z();

	EX::INI::Keymap()->xxtranslate(out.a(),out.x());
	
	return out;
}

static EX::Joypad::Configuration SomEx_joypad_configuration()
{
	//TODO: swap ppz/npz and prx/nrx (per joypad)

	EX::Joypad::Configuration out = SOMEX_CONFIGURE
	(
	1,//am
	0x4D,//ppx Move right (DIK_NUMPAD6)
	0x50,//npy Move backward (DIK_NUMPAD2)	
	0x51,//ppz Turn right (DIK_NUMPAD3)
	0x4B,//npx Move left (DIK_NUMPAD4)
	0x4C,//ppy Move forward (DIK_NUMPAD5)
	0x4F,//npz Turn left (DIK_NUMPAD1)
	0x47,//nrx Look down (DIK_NUMPAD7)
	0x47,//nry Look down (DIK_NUMPAD7)	
	0,//prz
	0x49,//prx Look up (DIK_NUMPAD9)
	0x49,//pry Look up (DIK_NUMPAD9)
	0,//nrz
	0,//psx
	0,//psy
	0,//nsx
	0,//nsy
	0x4C,//du Move forward (DIK_NUMPAD5)
	0x51,//dr Turn right (DIK_NUMPAD3)
	0x50,//dd Move backward (DIK_NUMPAD2)
	0x4F,//dl Turn left (DIK_NUMPAD1)
	0x0F,//b1 Menu	(DIK_TAB)
	0x39,//b2 Event (DIK_SPACE)
	0x49,//b3 Look up (DIK_NUMPAD9)
	0x2A,//0x1D,//b4 Magic (DIK_LCONTROL) //2020
	0x1D,//0x2A,//b5 Attack (DIK_LSHIFT) //2020
	0x47,//b6 Look down (DIK_NUMPAD7)
	0x4B,//b7 Move left (DIK_NUMPAD4)
	0x4D,//b8 Move right (DIK_NUMPAD6)
	0xC5,//b9 Pause (DIK_PAUSE)
	0,//dz
	0,//sa
	0 //menu
	)

	unsigned short *a = out.a(); size_t z = out.z();

	EX::INI::Keymap()->xxtranslate(out.a(),out.x());
	
	return out;
}

static EX::Pedals::Configuration SomEx_pedals_configuration()
{
	EX::Pedals::Configuration out = 
	{
	//int analog_mode[6][1]
	{{1},{1},{1},{1},{1},{0}},
	//unsigned char pos_pos[3][1]
	{{0x4D},{0x1E},{0x4C}},
	//unsigned char neg_pos[3][1]
	{{0x4B},{0x2C},{0x50}},
	//unsigned char pos_rot[3][1]
	{{0x51},{0x47},{0}},
	//unsigned char neg_rot[3][1]
	{{0x4F},{0x49},{0}},
	};

	//EX::INI::Keymap::xxtranslate(out.a(),out.x());

	return out;
}

extern const char *EX::need_ansi_equivalent(int cp, const wchar_t *in, char *out, int out_s)
{
	static char a[MAX_PATH]; //! not thread safe
	
	if(!in){ assert(0); return 0; } //undefined

	if(!out){ out = a; out_s = MAX_PATH; }else if(out_s<1) return ""; //!!

	int dwFlags = cp==65001/*CP_UTF8*/?0:WC_NO_BEST_FIT_CHARS; 

	//2018: desiring truncation (workshop.cpp)
	if(!WideCharToMultiByte(cp,dwFlags,in,-1,out,out_s,0,0)) //*out = '\0';
	{
		DWORD err;
		switch(err=GetLastError())
		{
		case ERROR_INSUFFICIENT_BUFFER:
		case ERROR_NO_UNICODE_TRANSLATION: assert(!SOM::game); break;
		default: assert(0);
		}		
	}

	return out;
}

extern const wchar_t *EX::need_unicode_equivalent(int cp, const char *in, wchar_t *out, int out_s)
{
	static wchar_t w[MAX_PATH]; //! not thread safe
	
	if(!in){ assert(0); return 0; } //undefined

	if(!out){ out = w; out_s = MAX_PATH; }else if(out_s<1) return L""; //!!

	//2018: desiring truncation (workshop.cpp)
	if(!MultiByteToWideChar(cp,0,in,-1,out,out_s)) //*out = '\0';
	{
		DWORD err; 
		switch(err=GetLastError())
		{
		case ERROR_INSUFFICIENT_BUFFER:
		case ERROR_NO_UNICODE_TRANSLATION: assert(!SOM::game); break;
		default: assert(0);
		}		
	}

	return out;
}