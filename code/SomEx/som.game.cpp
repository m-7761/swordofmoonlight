
#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

#include <algorithm>

#include "SomEx.h"

//todo: check for Sompaste.dll
#include "../Sompaste/Sompaste.h"

extern SOMPASTE Sompaste;

#include "Ex.ini.h"
#include "Ex.input.h"
#include "Ex.output.h"
#include "Ex.cursor.h"
#include "Ex.window.h"

#include "dx.ddraw.h"

#include "som.state.h"
#include "som.status.h"
#include "som.extra.h"
#include "som.files.h"
#include "som.game.h"

#define SOMVECTOR_MATH
#include "../Somplayer/Somvector.h"
#include "../lib/swordofmoonlight.h"

namespace DSOUND
{
	extern void playing_delays(); 
	extern bool doForward3D;
	extern void Sync(LONG,LONG);
	class IDirectSoundBuffer;
}	   
namespace EX 
{		
	extern bool vk_pause_was_pressed;				 
}
namespace DDRAW
{
	extern bool inStereo;
	
	extern float xyScaling[2];

	extern const DWORD *pshaders9[];

	//REMOVE ME?
	extern int psColorkey;
	extern int(*colorkey)(DX::DDSURFACEDESC2*,D3DFORMAT); 

	extern DWORD refreshrate;
}

static void som_game_menu_init();
extern void som_game_field_init()
{
	if(SOM::field){ assert(0); return; }
	
	//SOM::field = true; //???

	//TODO: MOVE TO INITIALIZATION POINT
	extern float *som_hacks_kf2_compass;
	if(EX::INI::Option()->do_nwse)
	{
		assert(!som_hacks_kf2_compass);

		//NOTE: 445870/445ad0 unload this memory
		char writeable[64] = "data\\menu\\NWSE1.mdo"; //art? som_MPX_64?
		extern SOM::MDO::data *som_MDO_445660(char*);
		if(void*mdo=som_MDO_445660(writeable))
		{
			som_hacks_kf2_compass = ((float*(__cdecl*)(void*))0x4458d0)(mdo);
			assert(som_hacks_kf2_compass);					
			EX::INI::Detail dt; if(&dt->nwse_saturation)
			{
				bool constant; DWORD s =
				(DWORD)dt->nwse_saturation(&constant);

				auto mat = *(float**)som_hacks_kf2_compass;
				int m = 0, n = *(int*)(mat+3);
				extern float *som_scene_nwse; if(!constant)
				{
					som_scene_nwse = new float(n*4);
				}						
				for(mat=*(float**)(mat+4);n-->0;mat+=8,m+=4)
				{
					if(som_scene_nwse)
					memcpy(som_scene_nwse+m,mat,4*sizeof(float));

					if(mat[3]!=1)
					{
						float v = (s>>8&0xff)/255.0f;
						for(int i=3;i-->0;) mat[i]*=v; //0.175f; 								
						mat[3] = (s&0xff)/255.0f; //0.55f;
					}
					else 
					{
						float v = (s>>16&0xff)/255.0f;
						for(int i=3;i-->0;) mat[i]*=v; //0.4f
					}
				}
			}
		}
	}

	SOM::field = true;	
	SOM::L.startup = -1; //NEW
			
	//expanding name cutoffs
	//if(SOM::L.item_prm_file)
	{
		//don't wait until in demand
		SOM::PARAM::Item.prm->open();
		SOM::PARAM::Magic.prm->open();
		for(int i=250;i-->0;)
		{
			SOM::L.item_prm_file[i].c[2] = '@';
			itoa(i,SOM::L.item_prm_file[i].c+3,10);
			SOM::L.magic_prm_file[i].c[2] = '@';
			SOM::L.magic_prm_file[i].c[3] = '-';
			itoa(i,SOM::L.magic_prm_file[i].c+4,10);
		}
	}		

	////NOTE: do this unconditionally
	//2020: do_fix_animation_sample_rate?
	//2024: moving to som_game_4245e0_init_pc_arm_and_hud_etc
	//below 
//	extern void som_game_60fps(); som_game_60fps(); //workshop.cpp

	//HACK: popuplate SOM::material_textures?
	SOM::MT::lookup("");

	//2022: formerly som_game_ReadFile did this if
	//the first map, but SOM::field is now set after
	//som_MPX_411a20 reads the first map
	som_game_menu_init();

	extern void som_CS_init();
	som_CS_init();
}
static void __cdecl som_game_4245e0_init_pc_arm_and_hud_etc()
{
	//2024: this needs to run before background loading
	//in order to support som_SFX_sounds 
	
	//NOTE: do this unconditionally
	//2020: do_fix_animation_sample_rate?
	extern void som_game_60fps(); som_game_60fps(); //workshop.cpp

	((void(*)())0x4245e0)(); //PIGGYBACKING
}

//REMOVE US
//2021: som.hacks.cpp refactor
//TODO: this should be executed inside the
//event processing logic
extern int som_game_hp_2021 = 0;
extern bool som_mocap_attacks3();
extern bool som_game_reset_model_speeds = false; //2023
static void som_game_reload_enemy_npc_pr2_files(); //2024
static void som_game_once_per_frame() //STAGE 1
{
	//2024: som.files.cpp signals these
	EX::INI::Bugfix bf;
	auto fasr = bf->do_fix_animation_sample_rate;
	if(fasr.enemy||fasr.npc)
	som_game_reload_enemy_npc_pr2_files();

	//2022: unloads standby maps
	extern void som_MPX_once_per_frame();
	som_MPX_once_per_frame();

	//TODO: equip menu?
	//TODO: before or after the logical frame?
	/*2021: this is bad unless events only happen every
	//third frame?
	if(SOM::frame%3==0)*/
	EX::reevaluating_numbers(EX::need_calculator()); 
	
	//THESE MIGHT BE BETTER PUT IN som_game_once_per_scene
	//BUT ARE PROBABLY NOT CRUCIAL

	bool swing = SOM::L.arm_MDL->f>1; if(swing) //arm_ms_windup2?
	{
		SOM::motions.swung_tick = SOM::motions.tick;
		SOM::motions.swung_id = SOM::L.arm_MDL->animation_id();
	}
	//2021: trying to change equipment while arm animations
	//is happening is a problem area this is meant to solve
	if(SOM::L.pcequip2&&!swing
	&&!SOM::L.arm_MDL->ext.d2&&!som_mocap_attacks3())
	{
		//TODO: might decouple attacks?
		memcpy(&SOM::L.pcequip,SOM::L.pcequip2_in_waiting,8);
		extern void som_game_equip(); som_game_equip();
	}

	if(som_game_reset_model_speeds) //2023
	{
		extern void som_MPX_reset_model_speeds();
		som_MPX_reset_model_speeds();
	}

	//2024: pcmagic_shield_timers should be 16-bit (not 8)
	if(!SOM::L.pcmagic_support_timer)
	{
		memset(SOM::L.pcmagic_shield_timers,0x00,8);
		memset(SOM::L.pcmagic_shield_ratings,0x00,8);
	}
	else memset(SOM::L.pcmagic_shield_timers,0xff,8); 
}
extern void som_game_once_per_scene() //STAGE 2
{
	//this code needs to execute after the business logic
	//but before the world is drawn so it's delegated to
	//som_scene_4137F0 for the time being (if not the first
	//frame of the red flash doesn't register as damage)

	//static int hp = 0; //HACK
	int &hp = som_game_hp_2021; //REMOVE ME		
	int cur = SOM::L.pcstatus[SOM::PC::hp];
	if(!hp||!cur) //dead?
	{		
		SOM::red = 0; //do_red
	}
	else if(hp>cur)	//we've been hit!
	{
		//fyi: knockback has been moved to som.state.cpp
		SOM::hpdown = SOM::frame; SOM::red = hp-cur;

		//hack: support non-combat damage sources
		if(SOM::red&&!SOM::hit&&!SOM::invincible)
		{
			if(EX::INI::Option()->do_hit)
			{
				SOM::hit = (void*)SOM::red;
				SOM::invincible = SOM::frame; //appropriate?
			}
		}
	}	 						
	else //todo: do away with 22 (flashes are time sensitive)
	{
		//does 60 fps not require an adjustment?
			
		if(SOM::red&&SOM::hpdown>=SOM::frame-22)
		{
			if(!SOM::context||SOM::context!=SOM::Context::playing_game)
			{
				SOM::hpdown++; //keep alive thru menus
			}
		}
		else //2020: testing resetting and red2 hack
		{
			//2021: I'm made som.mocap.cpp wait for SOM::red to be
			//cleared to reset SOM::hit to support the gauge drain
			//feature
			SOM::red = 0; SOM::red2 = 0;
		}
	}
		
	hp = cur;
}

extern int som_game_interframe = false;

//puts SOM on the same clock as SomEx
//2020: som_db.exe actually uses timeGetTime
//it's DirectX that's calling GetTickCount?!
//004020BB FF 15 14 82 45 00    call        dword ptr ds:[458214h]
static DWORD WINAPI som_game_GetTickCount()
{
	if(0) return SOM::Game.GetTickCount();

	return timeGetTime(); //should be same as EX::tick();
}
static DWORD som_game_4023c0_ms = 0;
static void __cdecl som_game_4023c0() //2020
{
	extern void som_CS_commit();

	if(SOM::newmap==SOM::frame) 
	{
		som_game_4023c0_ms = 33; som_game_interframe = 0;
	}

	//EXPERIMENTAL (2024)
	//I think maybe this messes up animations some
	//maybe just around looping at variable speeds
	float fps = 30.0f/DDRAW::refreshrate;
	SOM::L.fade = 0.01666667*fps;
	SOM::L.rate = 0.03333334*fps;
    SOM::L.rate2 = 0.06666667*fps;

	//NEW: this helps with subtitles
//	menu name position on some screens :(
//	SOM::L.fps = DDRAW::refreshrate;

	//note: the animations are rough because they don't update
	//every frame. they'll have to be overhauled at some point

	if(EX::INI::Bugfix()->do_fix_animation_sample_rate)
	{
		//SOM's animation uses fixed time step logic. It could
		//be improved by reimplementing the animation routines
		//but there's no time for that in the immediate future

		//this test is designed to treat 72fps like 60fps and to
		//skip the third frames at 90fps since I suspect Direc3D
		//will flip at native frame rate
		if(som_game_4023c0_ms>=13||DDRAW::refreshrate>60) 
		{
			if(1!=SOM::recording) //2020: som.record.cpp still image?
			{
				//DICEY: I really don't want to rollover but it needs
				//to meet 30fps somehow				
				for(int i=som_game_4023c0_ms>=30?2:1;i-->0;) //~30fps?
				{	
					DWORD flash = SOM::L.damage_flash;

					DWORD cmp[5]; //easier here than spot fix
					memcpy(cmp,&SOM::L.status_timers,sizeof(cmp));

					//4023c0 is the top world step subroutine
					((void(__cdecl*)())0x4023c0)(); som_CS_commit();

					if(++som_game_interframe>=(int)DDRAW::refreshrate/30) //2024
					som_game_interframe = 0;
					//if(som_game_interframe=!som_game_interframe)
					if(som_game_interframe)
					{
						//2021: set these back so they update
						//every other frame. world_counter is
						//related to a few status things, the
						//main one being poison timing. flash
						//prolongs damage flashes but retains
						//the same gradiant spread across two
						//frames (with the dissolve effect it
						//may not be much different)

						//causes poison to hit for 2 HP... in
						//som_MDL_reprogram the tests are set
						//to twice as many frames
						//SOM::L.world_counter--; //30fps

						//NOTE: this can be set to 10 and its
						//sources can also be poison or tiles
						if(flash-1==SOM::L.damage_flash)
						{
							SOM::L.damage_flash = flash; //30fps
						}
						//NOTE: 425940 DECREMENTS THESE TWICE
						//IT CERTAINLY LOOKS BIZARRE CODEWISE
						for(int i=5;i-->0;)
						if(cmp[i]-2==SOM::L.status_timers[i]) //1
						{
							SOM::L.status_timers[i] = cmp[i]; //30fps
						}
					}
				}
			}
			som_game_4023c0_ms = 0;
		}
	}
	else while(som_game_4023c0_ms>=33) //advance animations one frame?
	{
		som_game_4023c0_ms-=33; assert(som_game_4023c0_ms<33);

		//REMINDER: som_MPX_42dd40 may call 4023c0 once after map
		//loading
		((void(__cdecl*)())0x4023c0)(); som_CS_commit();
	}
}
static DWORD __stdcall som_game_402070_timeGetTime() //2020
{
	//REMOVE ME
	//2021: som.hacks.cpp refactor
	som_game_once_per_frame(); //REMOVE ME
	//som_game_once_per_scene(); //som.scene.cpp

	//I'm not so sure this is a good idea for som_db?
	//it's normally a constant probably fixed at 33 (~30fps) that is 
	//used to subdivide steps
	//but it poses a problem because it can have a sliver left over
	//or if you accumulate/carry the slivers (as below) it injects a 
	//an extra 33 boost at a random moment which is obviously not
	//good either
	//so probably what's best for now is to just cancel it out? as
	//is done in the final code before th return
	DWORD &denom = *(DWORD*)0x4C2240;

	//this removes the potential for using very small time steps if the
	//remainder is small since som_db's main loop just uses whatever is
	//leftover and that's also not a good use of CPU time
//	static DWORD carry = 0;

	enum{ r=0 }; //round? causing visual turbulence

	//WINSANITY: for the time being fixed is what's best
	float s = EX::INI::Window()->seconds_per_frame*1000;
	if(s<=0&&DDRAW::refreshrate)
	s = 1000.0f/DDRAW::refreshrate;
	DWORD t = (unsigned)(r?s+0.5f:s);	
	if(!t) //REFERENCE
	{
		//I don't expect to enter this code any longer
		assert(DDRAW::refreshrate);

		//TESTING
		//this system is designed to deal with the chaos of triple/quadruple
		//buffering timing by averaging out the frame rate to smooth hiccups
		for(int i=EX_ARRAYSIZEOF(SOM::onflip_triple_time);i-->0;)
		t+=SOM::onflip_triple_time[i];
		s = float(t)/EX_ARRAYSIZEOF(SOM::onflip_triple_time); //TESTING		
		//if(1&&EX::debug)
		{
			t = (unsigned)(1000/s+0.5f);
			t = t<45?30:t<54?50:t<67?60:t<81?72:t<110?90:t<132?120:t<170?144:240;
			s =  1000.0f/t;
		}
		t = (unsigned)(r?s+0.5f:s);

		if(EX::debug)
		{
			static DWORD cmp = t; if(cmp!=t) 
			{
				MessageBeep(t>cmp?MB_ICONQUESTION:MB_ICONASTERISK); 
			
				cmp = t;
			}
		}
	}
	if(r) //round? causing visual turbulence
	{		
		assert(0);
		static float overflow = 0;
		if((overflow+=s-t)>1){ t++; overflow-=1; }
	}
	else s = (float)t;

//	carry+=t%denom; t-=carry; 
//	if(carry>=denom){ t+=denom; carry-=denom; }
//	if(!t){ t = min(denom,carry); carry-=t; }		
	//JUST CANCEL OUT THIS FACILITY? Just do old school slow-motion below
	//30fps? 4020ef
	
	if(t>33+1) //t = 33; //TODO: adaptive sync monitor?
	{		
		//impossible if fixing to DDRAW::refreshrate?
		t = 33;
	//	assert(0);
	}

	som_game_4023c0_ms+=t; if(t) denom = t; //zero divide

	//REMINDER: this is frozen when in menus, etc.
	SOM::motions.tick+=t;
	SOM::motions.diff = t; 
	//SOM::motions.step = t/1000.0f;
	SOM::motions.step = s/1000;
	SOM::motions.frame = SOM::frame;

	//0040219B 89 15 D0 0E 4C 00    mov         dword ptr ds:[4C0ED0h],edx
	return t+*(DWORD*)0x4C0ED4;
}

 //UNUSED
//REMINDER: may want to use these for movies
//catching these might help, but not by much
static LSTATUS APIENTRY som_game_RegOpenKeyExW(HKEY A, LPCWSTR B, DWORD C, REGSAM D, PHKEY E)
{
	//fix IDirectSound3DListener::CommitDeferredSettings?
	if(B&&!wcscmp(B,L"Software\\Microsoft\\Multimedia\\DirectSound\\"))
	{
		EXLOG_LEVEL(0) << "som_game_RegOpenKeyExW()\n"
		"Ignoring "<< B << "believing it negatively impacts IDirectSound3DListener::CommitDeferredSettings";
		return 2; //ERROR_FILE_NOT_FOUND
	}
	//this is queried when text is displayed?
	//SOFTWARE\Microsoft\Windows NT\CurrentVersion\LanguagePack\SurrogateFallback
	return SOM::Game.RegOpenKeyExW(A,B,C,D,E);
}//UNUSED
static LSTATUS APIENTRY som_game_RegOpenKeyExA(HKEY A, LPCSTR B, DWORD C, REGSAM D, PHKEY E)
{
	//fix IDirectSound3DListener::CommitDeferredSettings?	
	if(B&&!strcmp(B,"SOFTWARE\\Microsoft\\APL")) 
	{
		EXLOG_LEVEL(0) << "som_game_RegOpenKeyExA()\n"
		"Ignoring "<< B << "believing it negatively impacts IDirectSound3DListener::CommitDeferredSettings";
		return 2; //ERROR_FILE_NOT_FOUND
	}
	return SOM::Game.RegOpenKeyExA(A,B,C,D,E);
}

static LRESULT CALLBACK som_game_WindowProc(HWND,UINT,WPARAM,LPARAM);

static BOOL WINAPI som_game_SetCurrentDirectoryA(LPCSTR); 

static BOOL WINAPI som_game_WritePrivateProfileStringA(LPCSTR,LPCSTR,LPCSTR,LPCSTR);
static UINT WINAPI som_game_GetPrivateProfileIntA(LPCSTR,LPCSTR,INT,LPCSTR);

static HANDLE WINAPI som_game_FindFirstFileA(LPCSTR,LPWIN32_FIND_DATAA);

static BOOL WINAPI som_game_FindClose(HANDLE hObject);

static HANDLE WINAPI som_game_CreateFileA(LPCSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,DWORD,DWORD,HANDLE);
static HANDLE WINAPI som_game_LoadImageA(HINSTANCE,LPCSTR,UINT,int,int,UINT);	

static BOOL WINAPI som_game_ReadFile(HANDLE,LPVOID,DWORD,LPDWORD,LPOVERLAPPED);
static BOOL WINAPI som_game_WriteFile(HANDLE,LPCVOID,DWORD,LPDWORD,LPOVERLAPPED); //UNUSED 

static BOOL WINAPI som_game_CloseHandle(HANDLE hObject);

//// BGM //////////	
extern HMMIO WINAPI som_game_mmioOpenA(LPSTR,LPMMIOINFO,DWORD);
static LONG WINAPI som_game_mmioRead(HMMIO,HPSTR,LONG);
//static LONG WINAPI som_game_mmioSeek(HMMIO,LONG,int); 
static MMRESULT WINAPI som_game_mmioDescend(HMMIO,LPMMCKINFO,const MMCKINFO*,UINT);
static MMRESULT WINAPI som_game_mmioAscend(HMMIO,LPMMCKINFO,UINT);
static MMRESULT WINAPI som_game_mmioClose(HMMIO,UINT);	
static MCIERROR WINAPI som_game_mciSendCommandA(MCIDEVICEID,UINT,DWORD_PTR,DWORD_PTR);

//// Removing WS_EX_TOPMOST /////////////
static HWND WINAPI som_game_CreateWindowExA(DWORD,LPCSTR,LPCSTR,DWORD,int,int,int,int,HWND,HMENU,HINSTANCE,LPVOID);
static HWND WINAPI som_game_CreateWindowExW(DWORD,LPCWSTR,LPCWSTR,DWORD,int,int,int,int,HWND,HMENU,HINSTANCE,LPVOID);

static VOID WINAPI som_game_OutputDebugStringA(LPCSTR lpOutputString)
{	
	EXLOG_LEVEL(0) << "som_game_OutputDebugStringA()\n";
	/*if('>'!=*lpOutputString)
	{
		wchar_t buf[512];
		EX::need_unicode_equivalent(932,lpOutputString,buf,sizeof(buf));
		EXLOG_LEVEL(0) << "L\""+1 << lpOutputString << "\"\n";
		EXLOG_LEVEL(0) << "L\"" << buf << "\"\n";
	}
	else*/ EXLOG_LEVEL(0) << '>' << lpOutputString;

	//docs say this calls OutputDebugStringA... the game fails to start
	//if a debugger isn't attached, but for some reason works if one is
	//OutputDebugStringW(buf);
	SOM::Game.OutputDebugStringA(lpOutputString);
}

extern void*__cdecl som_MPX_operator_new(size_t sz);
extern void __cdecl som_MPX_operator_delete(void*p);

static BYTE som_game_401300(char *a, char *ext)
{
	//hopefully these don't allocate 2 buffers like 401300???
	return (a=strrchr(a,'.'))&&!stricmp(a+1,ext);
}
static BYTE som_game_401410(char *a, char *d, char *ext)
{
	int i,len = strlen(a);
	for(i=len;i-->0;) if(a[i]=='.') break;
	if(i<=0) i = len;			
	if(d!=a) memcpy(d,a,i); //same for all but 44037f's case
	d[i] = '.';
	memcpy(d+i+1,ext,4); assert(strlen(ext)<=3);
	return 1;
}

static BYTE som_game_441510_MDL(SOM::MDL &m)
{	
	return *m->file_head&&m.advance();
}
static BYTE som_game_4414c0_MDL(SOM::MDL &m, int i)
{	
	int rt = m.running_time(m.c);

	int mode = *m->file_head;

	if(mode&7&&i<rt)
	{
		m.rewind(); m.d = i; return 1;
	}
	else assert(!*m->file_head); return 0;
}

const struct SOM::Game SOM::Game; //extern

SOM::Game::Game() 
{		
	static int twice = 1; //paranoia	
					 
	if(this==&SOM::Game&&twice++==1)
	{	
		int rt = 0, db = 0;

		switch(SOM::image())
		{
		case 'db2': db = 3; break;
		case 'db1': db = 2; break;
		case 'rt2': rt = 3; break;
		case 'rt1': rt = 2; break;
		case 'kf1': rt = 1; break;
		}

		if(rt||db)
		{
			assert(!rt!=!db);

			if(rt) SOM::game = som_rt.exe;
			if(db) SOM::game = som_db.exe;

		#ifdef NDEBUG
		//#error test me
		int todolist[SOMEX_VNUMBER<=0x1020704UL];
		#endif

			if(1||!EX::debug) //2021
			{
				//TODO: might try to improve tools to not be blurry on 
				//Windows 10?
				HMODULE user32 = LoadLibraryA("User32.dll");
				//NOTE: IsProcessDPIAware exists
				if(void*proc=GetProcAddress(user32,"SetProcessDPIAware"))
				((BOOL(WINAPI*)())proc)();
			}
		}

		memset(this,0x00,sizeof(SOM::Game));
		
		//EXPERIMENTAL (Ex.detours.h?)
		RegOpenKeyExW = ::RegOpenKeyExW;
		RegOpenKeyExA = ::RegOpenKeyExA;

		GetTickCount = ::GetTickCount; 
		SetCurrentDirectoryA = ::SetCurrentDirectoryA;
		WritePrivateProfileStringA  = ::WritePrivateProfileStringA;   
		GetPrivateProfileIntA = ::GetPrivateProfileIntA;
		FindFirstFileA = ::FindFirstFileA;
		FindClose = ::FindClose;
		LoadImageA = ::LoadImageA;
		CreateFileA = ::CreateFileA;
		ReadFile = ::ReadFile;
		WriteFile = ::WriteFile;
		CloseHandle = ::CloseHandle;	
	
		//// ADPCM business ////
		mmioOpenA = ::mmioOpenA;
		mmioRead = ::mmioRead; 		
		//mmioSeek = ::mmioSeek;
		mmioDescend = ::mmioDescend;
		mmioAscend = ::mmioAscend;
		mmioClose = ::mmioClose;		
		//// MIDI ////////////////////
		mciSendCommandA = ::mciSendCommandA;

		CreateWindowExA = ::CreateWindowExA;
		CreateWindowExW = ::CreateWindowExW;

		OutputDebugStringA = ::OutputDebugStringA;

		(DWORD&)_new_401500 = 0x401500;
		(DWORD&)_delete_401580 = 0x401580;

		(DWORD&)_ext_401300 = 0x401300;
		(DWORD&)_ext_401410 = 0x401410;

		(DWORD&)_ext_441510_advance_MDL = 0x441510;
		(DWORD&)_ext_4414c0_set_MDL = 0x4414c0;

		detours = new struct SOM::Game;
	}
	else if(!SOM::Game.detours&&twice++==2) 
	{
		//assuming SOM::Game.detours

		memset(this,0x00,sizeof(SOM::Game));
				
		if(!SOM::game) return;

		WindowProc = som_game_WindowProc; 

		if(0) //EXPERIMENTAL (Ex.detours.h?)
		{
			RegOpenKeyExW = som_game_RegOpenKeyExW;
			RegOpenKeyExA = som_game_RegOpenKeyExA;
		}

		GetTickCount = som_game_GetTickCount;
		SetCurrentDirectoryA = som_game_SetCurrentDirectoryA;
		WritePrivateProfileStringA  = som_game_WritePrivateProfileStringA;   
		GetPrivateProfileIntA = som_game_GetPrivateProfileIntA;

		FindFirstFileA = som_game_FindFirstFileA;
		FindClose = som_game_FindClose; 		
		LoadImageA = som_game_LoadImageA;
		CreateFileA = som_game_CreateFileA;
		ReadFile = som_game_ReadFile;
		//WriteFile = som_game_WriteFile; //UNUSED
		CloseHandle = som_game_CloseHandle; 		

		//// ADPCM business //////////
		mmioOpenA = som_game_mmioOpenA;
		mmioRead = som_game_mmioRead; 		
		//mmioSeek = som_game_mmioSeek;
		mmioDescend = som_game_mmioDescend;
		mmioAscend = som_game_mmioAscend;
		mmioClose = som_game_mmioClose;
		//// MIDI ////////////////////
		mciSendCommandA = som_game_mciSendCommandA;

		//// WS_EX_TOPMOST??? DEBUGGING /////////////

		CreateWindowExA = som_game_CreateWindowExA;
		CreateWindowExW = som_game_CreateWindowExW;

		OutputDebugStringA = som_game_OutputDebugStringA;;

		//2022: testing performance
		//in my test switching heaps changed transfers
		//to zone 2 in KF2 to go from 135ms to 50ms in
		//no disk IO scenario. still not enough to not
		//hiccup but all around it's better
		if(1) _new_401500 = som_MPX_operator_new;
		if(1) _delete_401580 = som_MPX_operator_delete;

		_ext_401300 = som_game_401300;
		_ext_401410 = som_game_401410;

		(void*&)_ext_441510_advance_MDL = som_game_441510_MDL;
		(void*&)_ext_4414c0_set_MDL = som_game_4414c0_MDL;
	}
	else assert(0); // should not occur
}

inline bool som_game_data(const char *p)
{
	DWORD d = *(DWORD*)p;
	return d==*(DWORD*)"DATA"||d==*(DWORD*)"data"; //Data
}

const wchar_t *SOM::Game::data(const char *in, bool ok) 
{	
	static char a[MAX_PATH] = "";
	static wchar_t w[MAX_PATH] = L"";
	static size_t a_s = 0, w_s = 0; 	

	if(!*w)  
	{	
		//NOTE: SOMEX_(A) is really the install
		//path for som_db (not som_rt)
		w_s = wcslen(wcscpy(w,L"DATA"));
		a_s = strlen(strcpy(a,SOMEX_(A)"\\data"));		
	}

	w[w_s] = '\0'; if(!in) return w;
		
	if(!som_game_data(in)||in[4]!='\\') 
	if(strnicmp(in,a,a_s)||in[a_s]!='\\')
	{
		//Reminder: in may not come from SOM

		assert(!ok); return 0; 
	}
	else in+=a_s;	
	else in+=4;

	int i, j;
	for(i=0,j=w_s;in[i];i++,j++) if(in[i]>127)
	{
		//Reminder: in may not come from SOM

		assert(0); return 0; 
	}
	else w[j] = in[i]; w[j] = '\0'; 

	return w;
}

const wchar_t *SOM::Game::project(const char *in, bool ok) 
{	
	static char a[MAX_PATH] = "";
	static wchar_t w[MAX_PATH] = L"", x[MAX_PATH] = L"";	

	static size_t w_s,x_s,a_s = 0; if(!a_s)
	{
	  	w_s = wcslen(wcscpy(w,EX::cd()));
		a_s = strlen(strcpy(a,SOMEX_(B))); //B: Project
							
		if(*EX::user(0)&&wcscmp(w,EX::user(0))) //NEW
		{
			x_s = wcslen(wcscpy(x,EX::user(0)));
		}
	}	
	if(!w_s) return 0; //???

	w[w_s] = '\0'; if(!in) return w;

	if(!PathIsRelativeA(in))
	{
		if(strnicmp(in,a,a_s)
		||(in[a_s]!='\\'&&in[a_s]!='/'))
		{
			if(!in[a_s]) return w;
			assert(!ok); return 0; //TODO: red alert!!
		}
		else in+=a_s+1; 
	}
  
	if(*in) w[w_s] = '\\'; 

	int i, j; int allcaps = w_s;
	for(i=0,j=w_s+1;in[i];i++,j++) if(in[i]>127)
	{
		//NOTE: in may not come from SOM

		assert(!ok); return 0; //TODO: red alert!!
	}
	else if((w[j]=in[i])=='.')
	{
		int e = tolower(in[i+1]);

		if(e=='d')
		{			
			if(!strnicmp(in+i,".dat",5))
			{
				allcaps = j+4;
			}
		}
		else if(e=='p')
		{
			if(!strnicmp(in+i,".PR2",5))
			{
				wcscpy_s(w+j,MAX_PATH-j,L".PRO"); 
				allcaps = j+=4; break;
			}
			else if(!strnicmp(in+i,".PRM",5))
			{
				allcaps = j+4;
			}
		}
	}
	else if(w[j]=='\\') allcaps = j;

	w[j] = '\0';

	//normalize for case sensitive file systems			
	for(j=w_s;j<allcaps;j++) w[j] = toupper(w[j]);

	//NEW: prefer copy of file in user's folder
	if(x_s&&PathFileExistsW(wcscpy(x+x_s,w+w_s))) return x;

	return w;
}

static struct som_game_saves //cache
{
	struct datum : WIN32_FIND_DATAW
	{
		HANDLE h;

		operator HANDLE&(){ return h; }

		void operator=(HANDLE hh){ h = hh; }

		datum():h(){ _close(); }

		void _close()
		{
			if(h) CloseHandle(h); h = 0;

			cFileName[0] = '\0';
			ftLastWriteTime.dwLowDateTime = 0;
			ftLastWriteTime.dwHighDateTime = 0;			
		}

	}*data; bool populated;

	datum &operator[](int i)
	{
		assert(data&&i>=0&&i<100); return data[i]; 
	}

	void init(){ if(!data) data = new datum[100]; }

	void close()
	{
		if(populated)
		{
			populated = false;
			for(int i=100;i-->0;) data[i]._close();
		}
	}

	som_game_saves():data(),populated(){} 
	
	~som_game_saves(){ close(); }

}som_game_saves; 

const wchar_t *SOM::Game::save(const char *in, bool reading)
{
	static wchar_t*save_glob = 0;
	static wchar_t*save_path = 0;
	if(!in) return save_path;
	static wchar_t*save_file; if(!save_path) 
	{
		som_game_saves.init();

		save_path = new wchar_t[MAX_PATH+16];
		save_glob = new wchar_t[MAX_PATH+16];
		save_file = save_path +
		swprintf(save_path,L"%ls\\save\\",EX::user(1));

		//NOTE: this API refuses / but this case should be clean
		SHCreateDirectoryExW(0,save_path,0);

		EXLOG_ALERT(0) << "Saving games to " << save_path << '\n'; 		
	
		//need to find "som" files and "dat" also
		//note, * now includes elapsed-time, etc!
		//swprintf(save_glob,L"%s??.dat",save_path);
		swprintf(save_glob,L"%s??.*",save_path);
	}	
	//assume relative (e.g. save\00.dat)
	//if(!in||strnicmp(in,"save",4)||in[4]!='\\')
	{
	//	assert(0); return 0;
	}
	assert(in&&!strnicmp(in,"save",4)&&in[4]=='\\');

	auto &ss = som_game_saves[atoi(in+5)];

	//TODO: ok this is mandatory now
	//The last straw was not closing FindFirstFile HANDLES!!
	//Although if it turns out SOM does that for all files--oh nevermind
	//if(bf->do_fix_slowdown_on_save_data_select)
	{
		if(!som_game_saves.populated)
		{
			som_game_saves.populated = true;

			const wchar_t *ext2 = L"som";
			if(SOM::retail)
			{
				const wchar_t *ext = EX::INI::Player()->player_file_extension;
				if(*ext) ext2 = ext;
			}

//			load_mask[0]&=1; //SOM may not save without at least one (00.dat)

			HANDLE glob; WIN32_FIND_DATAW find; 
			
			wchar_t *name = find.cFileName;

			glob = FindFirstFileW(save_glob,&find);

			if(glob==INVALID_HANDLE_VALUE) 
			{
				//warning! assuming no save files
			}
			else for(int safety=0;;safety++)
			{	
				if(isdigit(name[0])&&isdigit(name[1])) //"??.*"
				{
					auto &cmp = som_game_saves[_wtoi(name)];
					if(CompareFileTime(&cmp.ftLastWriteTime,&find.ftLastWriteTime)<0)
					{
						wchar_t *ext = PathFindExtension(name);										
						if(*ext++&&(!wcsicmp(ext,ext2)||!wcsicmp(ext,L"dat")))
						{
							static_cast<WIN32_FIND_DATAW&>(cmp) = find;
						}
					}
				}
				
				//TODO: safety dialog
				if(!FindNextFileW(glob,&find)||safety>100)
				{				
					assert(GetLastError()==ERROR_NO_MORE_FILES);
							
					::FindClose(glob); break;
				}		
			}
		}

		if(reading&&!*ss.cFileName) return 0;
	}
	
	if(!*ss.cFileName) //writing?
	{
		for(int i=0;i<7;i++) save_file[i] = in[5+i];
	}
	else wcscpy(save_file,ss.cFileName);
		
	if(!reading) //conservative	policy
	{
		//2020: now that save file names can come in different
		//combinations caching needs to be restricted to menus
		som_game_saves.close();
	}	
	
	return save_path;
}

bool SOM::Game::ini(const char *test, const char *section)
{
	if(!test||!section||stricmp(section,"config")) return false;

	const char *cmp = image('.ini'); 

	if(!cmp||stricmp(cmp,test)) return false;

	return true;
}

static wchar_t *som_game_rewrite(wchar_t *inout, size_t inout_s)
{
	if(!inout||!inout_s) return inout;

	EX::INI::Bugfix bf;
		
	bool padCfgX = bf->do_fix_controller_configuration;
	bool engrish = bf->do_fix_spelling_of_english_words;

	switch(*inout)
	{
	case 'p':

		if(!padCfgX) return inout;
		if(!wcsncmp(inout,L"pacCfg7",inout_s))
		return wcscpy_s(inout,inout_s,L"padCfg7"),inout; 
		if(!wcsncmp(inout,L"paddfg6",inout_s))
		return wcscpy_s(inout,inout_s,L"padCfg6"),inout;
		break;

	case 's':

		if(EX::INI::Option()->do_st)
		{
			if(!wcsncmp(inout,L"showGage",inout_s))
			return wcscpy_s(inout,inout_s,L"showHealth"),inout;	
			if(!wcsncmp(inout,L"showCommpass",inout_s))
			return wcscpy_s(inout,inout_s,L"showStatus"),inout;	
			if(!wcsncmp(inout,L"showItem",inout_s))
			return wcscpy_s(inout,inout_s,L"showDamage"),inout;	
			return inout;
		}
		else if(!engrish) return inout;
		if(!wcsncmp(inout,L"showGage",inout_s))
		return wcscpy_s(inout,inout_s,L"showGauge"),inout;	
		if(!wcsncmp(inout,L"showCommpass",inout_s))
		return wcscpy_s(inout,inout_s,L"showCompass"),inout; 
		break;
	}

	return inout;
}

static UINT som_game_clamp(const char *key, UINT val)
{		
	bool fix = 
	EX::INI::Bugfix()->do_fix_out_of_range_config_values;
	if(!fix||!key) return val;

	switch(*key)
	{
	case 'b': //"bpp=16\r\n"
		     //"bob=1\r\n"
		    //"bgmVol=255\r\n"

		switch(key[1])
		{
		case 'p': if(!strnicmp(key,"bpp",4)) return val==32?32:16;
				  break;
		case 'o': if(!strnicmp(key,"bob",4)) return val?1:0;
				  break;
		case 'g': if(!strnicmp(key,"bgmVol",7)) return max(min(val,255),0);
				  break;
		}
		break;

	case 'd': //"device=1\r\n"

		//TODO: count devices??
		if(!strnicmp(key,"device",7)) return val; 
		break;

	case 'f': //"filtering=1\r\n"
		
		if(!strnicmp(key,"filtering",10)) return max(min(val,2),0);
		break;

	case 'g': //"gamma=8\r\n"

		if(!strnicmp(key,"gamma",6)) return max(min(val,16),0);
		break;

	case 'h': //"height=600\r\n"

		//NOTE: power gauges are malfunctioning below 480
		if(!strnicmp(key,"height",7)) return max(val,480); 
		break;

	case 's': //"seVol=255\r\n"
			 //"showGage=1\r\n"
			//"showGauge=1\r\n"
		   //"showCommpass=1\r\n"
		  //"showCompass=1\r\n"
		 //"showItem=1\r\n"

		if(!strnicmp(key,"show",4))
		{
			return val?1:0; //assuming
		}
		if(!strnicmp(key,"seVol",6))
		{
			return max(min(val,255),0);
		}
		break;

	case 'p': //"padDev=0\r\n"
			 //"padCfg0=4\r\n"
			//"padCfg1=3\r\n"
		   //"padCfg2=0\r\n"
		  //"padCg3=1\r\n"
	     //"padCfg4=6\r\n"
		//"padCfg5=7\r\n"
	   //"padCfg6=2\r\n"
	  //"padCfg7=5\r\n";

		if(strnlen(key,8)==7) //sometimes 8 is written instead of 0
		{
			if(val==8) val = 0; //val = val%8

			if(!strnicmp(key,"padCfg",6)) return max(min(val,7),0); 
			if(!strnicmp(key,"pacCfg",6)) return max(min(val,7),0); 
			if(!strnicmp(key,"paddfg",6)) return max(min(val,7),0); 
		}

		//TODO: count controllers??
		if(!strnicmp(key,"padDev",7)) return val;
		break;

	case 'w': //"width=800\r\n"

		//NOTE: power gauges are malfunctioning below 640
		if(!strnicmp(key,"width",6)) return max(val,640); 
		break;
	}

	return val;
}

BOOL SOM::Game::write(const char *lpKeyName, const char *lpString, const char *lpAppName)
{	
	assert(lpKeyName&&lpString);
	assert(lpAppName&&!stricmp("config",lpAppName));

	if(!lpKeyName||!lpString||!lpAppName) return 0;
		
	//hack: if SOM is writing to config file on its own. Flush ini
	//2022: SOM::field avoids writes when resetting new pcid value
	static bool flushed = false; if(!flushed&&SOM::field)
	{
		flushed = true; SOM::config_ini(); SOM::record_ini(); 
	}

	if(*lpKeyName=='2') //this is a bug
	{
		assert(0); //what's going on here???

		return TRUE; //yeah, whatever
	}

	wchar_t wKeyName[MAX_PATH+1] = L"", wString[MAX_PATH+1] = L"";

	int i, j;
	for(i=0;i<MAX_PATH&&lpKeyName[i];i++) wKeyName[i] = lpKeyName[i]; 
	for(j=0;j<MAX_PATH&&lpString[j];j++) wString[j] = lpString[j]; 

	if(i>=MAX_PATH) return 0; wKeyName[i] = '\0';
	if(j>=MAX_PATH) return 0; wString[j] = '\0';
		
	som_game_rewrite(wKeyName,MAX_PATH);

	//Sword of Moonlight does not overwrite seVol this once 0

	switch(*lpKeyName)
	{
	case 's':

		if(!strcmp(lpKeyName,"seVol")) //so we're doing that on its behalf
		{
			_itow_s(SOM::seVol,wString,MAX_PATH,10);
		}
		break;

	case 'b':

		if(!strcmp(lpKeyName,"bpp")) //TODO: out source custom stuff
		{
			EX::INI::Option op;

			if(op->do_opengl)
			{
				wcscpy(wString,SOM::bpp==16?L"16":L"32");				
			}
			else if(op->do_highcolor) switch(atoi(lpString))
			{
			case 16: wcscpy(wString,L"555"); break;
			case 32: wcscpy(wString,L"565"); break;
			}
		}
		break;

	case 'd':

		if(!strcmp(lpKeyName,"device")) //avoid software device on shutdown
		{
			if(!strcmp(lpString,"0")) wcscpy(wString,L"2");
		}
		break;

		//in case of DDRAW::inStreo experiments, or maybe 640x480 mode exit
	case 'h':
		assert(SOM::height);
		if(!strcmp(lpKeyName,"height")) _itow(SOM::height,wString,10);
		break;
	case 'w':
		assert(SOM::width);
		if(!strcmp(lpKeyName,"width")) _itow(SOM::width,wString,10);
		break;
	}

	EXLOG_LEVEL(5) << "Writing " << lpKeyName << " to ini (" << lpString << ")\n";

	const wchar_t *ini = title('.ini'); assert(ini&&*ini);

	return WritePrivateProfileStringW(L"config",wKeyName,wString,ini);
}

UINT SOM::Game::get(const char *lpKeyName, int nDefault, const char *lpAppName)
{
		//////REMINDER///////
		//
		// this runs before EX::is_needed_to_initialize
		//

	if(!lpKeyName||!lpAppName) return nDefault;	   
							
	assert(lpKeyName&&lpAppName);
	assert(!stricmp("config",lpAppName)); //hard coded for now

	wchar_t wKeyName[MAX_PATH+1] = L"";
	int i;
	for(i=0;i<MAX_PATH&&lpKeyName[i];i++) 
	wKeyName[i] = lpKeyName[i]; 

	if(i>=MAX_PATH) return nDefault; wKeyName[i] = '\0';

	som_game_rewrite(wKeyName,MAX_PATH);

	const wchar_t *ini = SOM::Game::title('.ini');
	assert(ini&&*ini);
		
	if(!strcmp(lpKeyName,"bpp")) //TODO: out source custom stuff
	{
		nDefault = 32; //2021

		if(EX::INI::Option()->do_opengl) //2022
		{
			SOM::bpp = GetPrivateProfileInt(L"config",wKeyName,nDefault,ini);
			
			return 32; //I think it shouldn't matter... it's too soon to say
		}
		int bpp = GetPrivateProfileInt(L"config",wKeyName,nDefault,ini);

		switch(bpp) //if(op->do_highcolor) 
		{
		case 555: return 16; case 565: return 32;
		}
		return som_game_clamp("bpp",bpp);
	}
	else if(!strcmp(lpKeyName,"seVol"))
	{
		SOM::seVol = GetPrivateProfileInt(L"config",wKeyName,nDefault,ini);
		SOM::seVol = som_game_clamp("seVol",SOM::seVol);

		return SOM::seVol; //SOM::config_bels();
	}
	else if(!strcmp(lpKeyName,"bgmVol")) //HACK
	{
		//2020: I'm not sure why seVol is clamped above, but may as well do
		//it for the BGM too
		SOM::bgmVol = GetPrivateProfileInt(L"config",wKeyName,nDefault,ini);
		SOM::bgmVol = som_game_clamp("bgmVol",SOM::bgmVol);

		//2020: taking over volume levels requires initalization, just not
		//sure when is the best time to?
		SOM::config_bels();

		return SOM::bgmVol;
	}
	else if(!strcmp(lpKeyName,"device")) //avoid software device on startup
	{
		nDefault = 2; //2021

		int device = GetPrivateProfileInt(L"config",wKeyName,nDefault,ini);

		return som_game_clamp("device",device==0?2:device);
	}	
	else if(!strcmp(lpKeyName,"width")||!strcmp(lpKeyName,"height"))
	{
		EX::INI::Window wn;

		//if(wn->do_scale_640x480_modes_to_setting) //2022: retiring
		{
			nDefault = *lpKeyName=='w'?800:600; //2021
		}		

		if(!wn||!wn->do_force_fullscreen_inside_window)
		{
			return GetPrivateProfileInt(L"config",wKeyName,nDefault,ini);
		}

		int width = GetPrivateProfileInt(L"config",L"width",nDefault,ini);
		int height = GetPrivateProfileInt(L"config",L"height",nDefault,ini);

		int vw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
		int vh = GetSystemMetrics(SM_CYVIRTUALSCREEN);

		if(vw&&vh) //see width/height fit into virtual screen space
		{
			vw-=GetSystemMetrics(SM_XVIRTUALSCREEN);
			vh-=GetSystemMetrics(SM_YVIRTUALSCREEN);			

			if(width>vw||height>vh)
			{
				float aspect = float(width)/float(height);

				if(width>vw)
				{
					width = vw; height = 1.0f/aspect*width;
				}
				if(height>vh)
				{
					height = vh; width = aspect*height;
				}

				if(width&1) width++; if(height&1) height++;
			}
		}

		return !strcmp(lpKeyName,"width")?width:height;
	}
	else 
	{
		//2021: default to shoulder button layout
		//NOTE: lpKeyName has pacCfg7 and paddfg6
		if('p'==*wKeyName&&!wcsncmp(wKeyName,L"padCfg",6))
		{
			nDefault = 0x02137645>>4*(wKeyName[6]-'0')&0xf;
		}

		int got = GetPrivateProfileInt(L"config",wKeyName,nDefault,ini);

		return som_game_clamp(lpKeyName,got);
	}
}

#define SOM_GAME_DETOUR_THREADMAIN(f,...) \
	if(GetCurrentThreadId()!=EX::threadmain)\
	{\
		return SOM::Game.f(__VA_ARGS__);\
	}

extern DWORD som_MPX_thread_id;
#define SOM_GAME_DETOUR_MULTITHREAD(f,...) \
	{\
		DWORD tid = GetCurrentThreadId();\
		if(tid!=EX::threadmain\
		 &&tid!=som_MPX_thread_id)\
		return SOM::Game.f(__VA_ARGS__);\
	}

static BOOL WINAPI som_game_SetCurrentDirectoryA(LPCSTR lpPathName)
{
	SOM_GAME_DETOUR_THREADMAIN(SetCurrentDirectoryA,lpPathName)

	EXLOG_LEVEL(7) << "som_game_SetCurrentDirectoryA()\n"; 

	EXLOG_LEVEL(3) << "Changing to "<< lpPathName << "\n";

	//assert(0); //set to the project folder argument early on

	const wchar_t *w = SOM::Game.project(lpPathName); 
			
	if(w) return SetCurrentDirectoryW(w); else assert(0); 
	
	return SOM::Game.SetCurrentDirectoryA(lpPathName); 
}

static BOOL WINAPI som_game_WritePrivateProfileStringA(LPCSTR A, LPCSTR B, LPCSTR C, LPCSTR D)
{
	SOM_GAME_DETOUR_THREADMAIN(WritePrivateProfileStringA,A,B,C,D)

	EXLOG_LEVEL(7) << "som_game_WritePrivateProfileStringA()\n";

	if(!SOM::Game.ini(D,A)) return SOM::Game.WritePrivateProfileStringA(A,B,C,D);
	
	return SOM::Game::write(B,C,A);
}
					
static UINT WINAPI som_game_GetPrivateProfileIntA(LPCSTR A, LPCSTR B, INT C, LPCSTR D) 
{
	SOM_GAME_DETOUR_THREADMAIN(GetPrivateProfileIntA,A,B,C,D)

	EXLOG_LEVEL(7) << "som_game_GetPrivateProfileIntA()\n";	

	if(!SOM::Game.ini(D,A)) return SOM::Game.GetPrivateProfileIntA(A,B,C,D);
	
	return SOM::Game::get(B,C,A);
}

static HANDLE som_game_find_xx_dat = 0;

static bool som_game_find_xx_dat_paranoia = false;

static HANDLE WINAPI som_game_FindFirstFileA(LPCSTR A, LPWIN32_FIND_DATAA B)
{
	SOM_GAME_DETOUR_THREADMAIN(FindFirstFileA,A,B)

	EXLOG_LEVEL(7) << "som_game_FindFirstFileA()\n";

	const wchar_t *w = 0; assert(A);

	if(!strnicmp(A,"save\\",5)) 
	{
		w = SOM::Game::save(A,true);

		if(!w) return INVALID_HANDLE_VALUE; 
		
		unsigned i = atoi(A+5); //should be 00 thru 99

		if(i>99) return INVALID_HANDLE_VALUE; 

	   //// SOM does this every frame for all files ////
	   //// so speed this up for network access etc ////

		if(B) memset(B,0x00,sizeof(*B));
		
		FILETIME wr = som_game_saves[i].ftLastWriteTime;

		if(B) B->ftCreationTime = B->ftLastAccessTime = B->ftLastWriteTime = wr; 

		if(B) memcpy(B->cAlternateFileName,memcpy(B->cFileName,A+5,6),6);

		if(B) B->nFileSizeLow = 98308; //size of a SOM save file

		som_game_find_xx_dat_paranoia = true; //just in case

		//assuming a safe dummy handle
		//Actually it turns out SOM never closes these handles!!
		return som_game_find_xx_dat = 0; 
	}
	else //MONITORING
	{
		w = SOM::Game::file(A); //assert(0);
		
		#ifdef _DEBUG
		assert(EX::INI::Output()->missing_whitelist(A));
		#endif
	}

	if(!w) return SOM::Game.FindFirstFileA(A,B);
							 
	WIN32_FIND_DATAW dw; HANDLE out = w?::FindFirstFileW(w,&dw):0;

	memcpy(B,&dw,sizeof(*B)); if(!out) return 0; 

	size_t conv = B->cAlternateFileName+sizeof(B->cAlternateFileName)-B->cFileName;

	for(size_t i=0;i<conv;i++) B->cFileName[i] = dw.cFileName[i];
	
	return out;
}

static BOOL WINAPI som_game_FindClose(HANDLE in)
{	
	SOM_GAME_DETOUR_THREADMAIN(FindClose,in)

	EXLOG_LEVEL(7) << "som_game_FindClose()\n";

	if(in&&in==som_game_find_xx_dat)
	{
		assert(som_game_find_xx_dat_paranoia);

		som_game_find_xx_dat_paranoia = false;

		som_game_find_xx_dat = 0; return TRUE;
	}

	return SOM::Game.FindClose(in);
}

static size_t som_game_reading = 0;

static const size_t som_game_readmax = 8;

static HANDLE som_game_reading_list[som_game_readmax];

static char som_game_reading_A[som_game_readmax][MAX_PATH];

static char *som_game_reading_a[som_game_readmax] =
{
	som_game_reading_A[0],som_game_reading_A[1],som_game_reading_A[2],som_game_reading_A[3],
	som_game_reading_A[4],som_game_reading_A[5],som_game_reading_A[6],som_game_reading_A[7],
};

static int som_game_reading_mode[som_game_readmax];

static void som_game_reading_x(size_t i)
{
	assert(i<som_game_reading);

	if(i>som_game_reading) return;

	char *a = som_game_reading_a[i];

	while(++i<som_game_reading)
	{
		som_game_reading_list[i-1] = som_game_reading_list[i];
		som_game_reading_mode[i-1] = som_game_reading_mode[i];
		som_game_reading_a[i-1] = som_game_reading_a[i];
	}
	
	som_game_reading_a[i-1] = a;		
	som_game_reading--;
}

static int som_game_save_xx = 0;
static HANDLE som_game_save_xx_dat = 0;
static bool som_game_resetting = true;
static HANDLE WINAPI som_game_CreateFileA(LPCSTR fname, DWORD faccess, DWORD C, LPSECURITY_ATTRIBUTES D, DWORD E, DWORD F, HANDLE G)
{	
	if(!strncmp(fname,"\\\\?\\",4)) //device file
	return SOM::Game.CreateFileA(fname,faccess,C,D,E,F,G);

	//SOM_GAME_DETOUR_THREADMAIN
	SOM_GAME_DETOUR_MULTITHREAD(CreateFileA,fname,faccess,C,D,E,F,G)

	EXLOG_LEVEL(7) << "som_game_CreateFileA()\n";

	EXLOG_LEVEL(3) << "Creating "<< fname << "\n";
		
	if(!strnicmp(fname,"save\\",5)) //early out
	{
		const wchar_t *wfname = 
		SOM::Game::save(fname,faccess==GENERIC_READ);

		//REMINDER: SOM::Game::save above is required
		//for som_game_highlight_load_game to succeed
		if(!SOM::field&&!SOM::L.startup)
		if(GetEnvironmentVariableW(L"LOAD",0,0)>1)
		{
			wchar_t buf[MAX_PATH]; 
			if(PathIsRelativeW(wfname=Sompaste->get(L"LOAD")))
			{
				swprintf_s(buf,L"%ls\\%ls",EX::user(1),wfname);
				wfname = buf;
			}
			return CreateFileW(wfname,faccess,C,D,E,F,G);
		}
		else assert(0);

		if(!wfname) return INVALID_HANDLE_VALUE;
		
		EXLOG_LEVEL(3) << "Widened to "<< wfname << '\n';

		som_game_save_xx = atoi(fname+5); //should be 00 thru 99

		if(faccess==GENERIC_WRITE)
		{
			assert(!som_game_save_xx_dat);
			som_game_save_xx_dat = 0;
			return CreateFileW(wfname,faccess,C,D,E,F,G);
		}
		else if(faccess==GENERIC_READ) //caching: see CloseHandle for details
		{
			auto &h = som_game_saves[som_game_save_xx];
			if(!h||SetFilePointer(h,0,0,FILE_BEGIN)==INVALID_SET_FILE_POINTER)
			{
				if(h) CloseHandle(h);

				C|=FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE; //share everything
				h = ::CreateFileW(wfname,faccess,C,D,E,F,G);
			}

			return som_game_save_xx_dat = som_game_saves[som_game_save_xx];
		}
		else assert(0);

		SetLastError(ERROR_FILE_NOT_FOUND); return INVALID_HANDLE_VALUE;
	}
	
	int _ext = fname?strlen(fname):0;
	while(_ext&&fname[--_ext]!='.');
	int e = fname[_ext]!='.'?0:tolower(fname[_ext+1]);
	const char *ext = fname+_ext+1;
	
	const wchar_t *wname; 
	wchar_t vname[MAX_PATH];
	extern wchar_t *som_art_CreateFile_w; //2021
	extern wchar_t *som_art_CreateFile_w_cat(wchar_t[],const char*);
	if(som_art_CreateFile_w)
	{
		wname = som_art_CreateFile_w_cat(vname,fname);		
	}
	else 
	{
		wname = SOM::Game::file(fname);
	
		//NEW: virtual data directory
		/*static*/ //wchar_t vname[MAX_PATH]; 
		if(wname&&!wcsnicmp(wname,L"DATA\\",5)) 
		{	
			//2018: assuming not returning this
			//static wchar_t v[MAX_PATH] = L"";
			if(EX::data(wname+5,vname)) wname = vname;
		}
		else if(!strnicmp(fname,"DATA\\",5)) //2023
		{
			assert(0); //TESTING

			//files for KING'S FIELD sample projects?
			if(EX::data(fname+5,vname)) wname = vname;
		}
	}

	//2022: PathFileExistsA returns true now
	//with a SOMEX_ path. I don't think it's
	//because of changes. it shouldn't be so
	//at this point though
	//2022: som_art_CreateFile_w_cat may set
	//wname to 0 if third-party code tries to
	//open files
	assert(wname||fname[3]!='>'); //SOMEX_(A)?
	//note, this first block duplicates
	//som_tool_file_appears_to_be_missing
	if(wname&&!PathFileExistsW(wname)					 
	||!wname&&!PathFileExistsA(fname))
	if(wname&&e=='p'&&!strnicmp(ext,"pr2",5)) //NEW
	{
		if(!PathFileExistsW(wname)) //PRO
		{
			int _ = wcslen(wname)-1; 

			(wchar_t&)wname[_] = '~'; //PR~

			if(!PathFileExistsW(wname)) 
			{
				(wchar_t&)wname[_] = '2'; //PR2

				if(!PathFileExistsW(wname)) 
				{
					wcscpy((wchar_t*)wname+_-2,L"PRO");
					goto missing; 
				}
			}
		}
	}
	else if(E==OPEN_EXISTING||E==TRUNCATE_EXISTING) //OLD
	{
		if(SOM::game==som_rt.exe) //???
		if(!strcmp(fname,"project.dat")) //hide project.dat in EX folder
		{
			if(PathFileExistsA("EX\\project.dat"))
			return SOM::Game.CreateFileA("EX\\project.dat",faccess,C,D,E,F,G);
		}
		if(!stricmp(fname,"PARAM\\OPENNING.DAT")) //try correct English
		{
			if(PathFileExistsA("PARAM\\OPENING.DAT"))
			return SOM::Game.CreateFileA("PARAM\\OPENING.DAT",faccess,C,D,E,F,G);
		}

		if(e=='c'&&!strncmp(ext,"cp",4)
		 ||e=='m'&&!strnicmp(ext,"mo",4)) goto pass;

		//2020: this got me on my KF2 demo since it short
		//circuited do_missing_file_dialog_ok
		bool sfx = wname&&!wcsncmp(wname+4,L"\\sfx\\",5);
		//SFX is searching for TXR or MDL (can use work)
		//CP is often not provided and could be generated
		if(sfx&&e=='m') goto pass; //mdl is first?

					missing:; //hacks

		bool log_only = false; //2021

#ifdef _DEBUG //then we might wanna know what's up

		if(!wname)
		{
			log_only = EX::INI::Output()->missing_whitelist(fname);

			wname = EX::need_unicode_path_to_file(fname);
		}
#else
		//then it should not be a file of interest to sword of moonlight 
		if(!wname) return SOM::Game.CreateFileA(fname,faccess,C,D,E,F,G);
#endif		
		wchar_t pretty[MAX_PATH] = L""; Sompaste->path(wcscpy(pretty,wname));
				
		EXLOG_ALERT(0) << "ALERT!: A file appears to be missing: " << Ex%pretty << "\n";
		if(sfx) EXLOG_ALERT(0) << "	(file may be MDL based instead of TXR since SFX)\n";

		static bool suppress = false; EX::INI::Output tt;

		if(!log_only)
		if(!suppress) if(!tt||!tt->do_missing_file_dialog_ok)
		{
			SOM::splashed(0); //HACK

			if(sfx) wcscat_s(pretty,L"\n(SFX can be a MDL file)");

			if(IDOK!=EX::messagebox(MB_OKCANCEL|MB_ICONERROR,
			"A file appears to be missing: \n\n%ls\n\n"					   
			"Press CANCEL to disable this message box.",pretty))
			{
				suppress = true; //pass:;
			}
		}
	}

pass: if(!wname) return SOM::Game.CreateFileA(fname,faccess,C,D,E,F,G);

	EXLOG_LEVEL(3) << "Widened to "<< wname << '\n';

	HANDLE out = 0; int read = 0; //NEW

	if(e=='e'&&!stricmp(ext,"evt")) //0: testing
	{
		//2022: ezt was initliazed here, but this caused 
		//an ownership conflict over the SOM::Game.project
		//buffer when working on som_MPX_411a20_ltd 
		extern HANDLE som_zentai_ezt;
		HANDLE &ezt = som_zentai_ezt;
		if(ezt&&ezt!=INVALID_HANDLE_VALUE) //portable events
		{
			read = 'evt'; assert(wname);

			static EX::temporary evt;
			if(CopyFile(wname,evt,0)) 
			{
				HANDLE gcp = GetCurrentProcess(); //don't CloseHandle on evt
				DuplicateHandle(gcp,evt,gcp,&out,0,0,DUPLICATE_SAME_ACCESS);
				SOM::zentai_splice(out);
			}
			else assert(0);
		}
	}
	else if(e=='d'&&!stricmp(fname,"PARAM\\sys.dat"))
	{
		read = 'sys';
	}

	if(!out) out = CreateFileW(wname,faccess,C,D,E,F,G);
	
	if(read&&out!=INVALID_HANDLE_VALUE)
	if(som_game_reading<som_game_readmax)
	{	
		som_game_reading_list[som_game_reading] = out;
		som_game_reading_mode[som_game_reading] = read;	

		strcpy(som_game_reading_a[som_game_reading],fname);

		som_game_reading++;
	}
	else assert(0);
		
	return out;
}

static HANDLE WINAPI som_game_LoadImageA(HINSTANCE hi, LPCSTR in, UINT uType, int cx, int cy, UINT uFlags)
{
	SOM_GAME_DETOUR_THREADMAIN(LoadImageA,hi,in,uType,cx,cy,uFlags)
			
	EXLOG_LEVEL(7) << "som_game_LoadImageA()\n";

	//XP: MSCTF.dll (Microsoft Text Service Module) extracts various (cursors?) every frame that are 16x16
	if(hi&&(int)hi!=0x400000) //assert(cx==0&&cy==0); 
	{
		assert(hi!=EX::module);
		return SOM::Game.LoadImageA(hi,in,uType,cx,cy,uFlags);
	}
	else assert(cx==0&&cy==0); 

	cx = cy = 0; //Stretch mode?

	if(!(uFlags&LR_LOADFROMFILE))
	{	
		if(uintptr_t(in)<=0xFFFF) //MAKEINTRESOURCE
		{
			//XP requesting something?
			//assert(in==(char*)0x68); //splash screen?
			HANDLE out = SOM::Game.LoadImageA(hi,in,uType,cx,cy,uFlags);			
			if(!out&&in==(char*)0x68) //normal for som_db.exe?
			{
				//REMINDER: this is for EXEs that have removed 0x68
				//should probably just return a rudimentary HBITMAP
				//0042C7FD FF 15 D8 81 45 00    call        dword ptr ds:[4581D8h]
				//assert(0); //2017

				//load test pattern
				uFlags|=LR_LOADFROMFILE;
				in = "DATA\\MENU\\map_icon.bmp";
				out = SOM::Game.LoadImageA(hi,in,uType,cx,cy,uFlags);
			}

			return out;
		}
		else if(1) 
		{
			//are these looking inside the EXE by resource string names?
			//A:\>\Data\Menu\pushstart.bmp //newgame.bmp //continue.bmp
			//0044940F FF D3                call        ebx			
			//assert(0); //2017
			return 0; //som_db.exe requests file names???
		}		

		return SOM::Game.LoadImageA(hi,in,uType,cx,cy,uFlags);
	}
	
	EXLOG_LEVEL(3) << "Loading "<< in << '\n';
	 
	const wchar_t *w = SOM::Game::file(in);

	//NEW: virtual data directory
	/*static*/ wchar_t vname[MAX_PATH];
	if(w&&!wcsnicmp(w,L"DATA\\",5)) 
	{
		//2018: assuming not returning this
		//static wchar_t v[MAX_PATH] = L"";
		if(EX::data(w+5,vname)) w = vname;
	}
	else //2023: lookup project files in alternative data?
	{
		if(!w||!PathFileExistsW(w)) //King's Field sample?
		if(!strnicmp(in,"DATA\\",5))
		{
			if(EX::data(in+5,vname)) w = vname;
		}
	}

	if(!w) return SOM::Game.LoadImageA(hi,in,uType,cx,cy,uFlags);

	EXLOG_LEVEL(3) << "Widened to "<< w << '\n';

	HANDLE out = LoadImageW(hi,w,uType,cx,cy,uFlags);

	if(strstr(in,"itemicon.bmp"))
	{
		//create the new/9th magic item icon
		//som_game_421f10 selects this texture
		DWORD &nine = SOM::menupcs[0x16]; 
		if(nine!=0xffFFffFF) return out;
		/*
		00421CF3 6A 28                push        28h  
		00421CF5 6A 28                push        28h  
		00421CF7 6A 00                push        0  
		00421CF9 8B F8                mov         edi,eax  
		00421CFB 6A 00                push        0  
		00421CFD 57                   push        edi  
		00421CFE E8 AD 4A 02 00       call        004467B0  
		00421D03 68 14 04 4C 00       push        4C0414h  
		00421D08 8D 4C 24 3C          lea         ecx,[esp+3Ch]  
		00421D0C 68 1C E2 45 00       push        45E21Ch  
		00421D11 51                   push        ecx  
		00421D12 8B D8                mov         ebx,eax  
		00421D14 E8 F4 DA 02 00       call        0044F80D  
		00421D19 53                   push        ebx  
		00421D1A 6A 00                push        0  
		00421D1C 6A 00                push        0  
		00421D1E 8D 54 24 50          lea         edx,[esp+50h]  
		00421D22 6A 00                push        0  
		00421D24 52                   push        edx  
		00421D25 E8 36 69 02 00       call        00448660  
		//00421D2A 6A 28                push        28h  
		//00421D2C 6A 28                push        28h  
		//00421D2E 6A 00                push        0  
		//00421D30 6A 28                push        28h  
		//00421D32 57                   push        edi  
		00421D33 89 46 38             mov         dword ptr [esi+38h],eax  
		*/
		HANDLE eax = ((HANDLE(__cdecl*)(HANDLE,DWORD,DWORD,DWORD,DWORD))0x4467B0)(out,80,80,40,40);
		//there are various dummy strings at 0x45E190
		//%sData\Menu\21.bmp and so on down to 0
		//really any unique string shoudl do
		const char *db_string = SOMEX_(A)"\\Data\\Menu\\22.bmp";			
		nine = ((DWORD(*)(const char*,DWORD,DWORD,DWORD,HANDLE))0x448660)(db_string,0,0,0,eax);	
	}
	else if(strstr(in,"Menu\\Frame")) //2024
	{
		HANDLE l = ((HANDLE(__cdecl*)(HANDLE,DWORD,DWORD,DWORD,DWORD))0x4467B0)(out,80,120,40,40);
		HANDLE r = ((HANDLE(__cdecl*)(HANDLE,DWORD,DWORD,DWORD,DWORD))0x4467B0)(out,120,120,40,40);
		
		const char *db_string = SOMEX_(A)"\\Data\\Menu\\23.bmp";			
		SOM::menupcs[0x17] = ((DWORD(*)(const char*,DWORD,DWORD,DWORD,HANDLE))0x448660)(db_string,0,0,0,l);	
		db_string = SOMEX_(A)"\\Data\\Menu\\24.bmp";			
		SOM::menupcs[0x18] = ((DWORD(*)(const char*,DWORD,DWORD,DWORD,HANDLE))0x448660)(db_string,0,0,0,r);	
	}

	return out;
}
static void __cdecl som_game_421f10_itemicon(DWORD *in, DWORD edx, DWORD ebp) //itemicon.bmp?
{
	//TODO: square these somehow? som_hacks_DrawPrimitive is doing so.

	//0041D047 55                   push        ebp  
	//0041D048 52                   push        edx  
	//0041D049 89 74 24 58          mov         dword ptr [esp+58h],esi  
	//0041D04D 89 5C 24 5C          mov         dword ptr [esp+5Ch],ebx  
	//0041D051 89 44 24 68          mov         dword ptr [esp+68h],eax
	//DWORD esi = in[0];
	//DWORD ebx = in[1];
	DWORD eax = in[4];	

	//421F1f0 has a jump table that is identical for the 8 item
	//icons. it's not worth adding another identical entry that
	//would go at 4229C4
	if(eax>=0xe) 
	{
		in[4] = 0xd;
		std::swap(SOM::menupcs[0xd+8],SOM::menupcs[eax+8]);
	}

	((void(__cdecl*)(DWORD*,DWORD,DWORD))(0x421F10))(in,edx,ebp);

	if(eax>=0xe) 
	{
		in[4] = eax;
		std::swap(SOM::menupcs[0xd+8],SOM::menupcs[eax+8]);
	}	
}
static void __cdecl som_game_421f10_lr_arrow(DWORD layout[6], float *menu, float menu_70) //2024
{
	//0041D047 55                   push        ebp  
	//0041D048 52                   push        edx  
	//0041D049 89 74 24 58          mov         dword ptr [esp+58h],esi  
	//0041D04D 89 5C 24 5C          mov         dword ptr [esp+5Ch],ebx  
	//0041D051 89 44 24 68          mov         dword ptr [esp+68h],eax
	//DWORD esi = layout[0];
	//DWORD ebx = layout[1];
	DWORD eax = layout[4];	

	if(eax==4) std::swap(SOM::menupcs[0xc],SOM::menupcs[23]); //left
	if(eax==5) std::swap(SOM::menupcs[0xd],SOM::menupcs[24]); //right

	((void(__cdecl*)(DWORD*,float*,float))(0x421F10))(layout,menu,menu_70);

	if(eax==4) std::swap(SOM::menupcs[0xc],SOM::menupcs[23]); //left
	if(eax==5) std::swap(SOM::menupcs[0xd],SOM::menupcs[24]); //right
}
int SOM::VT::fog_register = 0; //TEMPORARY
extern SOM::VT*(*SOM::volume_textures)[1024] = 0;
extern SOM::MT*(*SOM::material_textures)[1024] = 0;
extern char *som_game_449530_cmp = 0;
extern int som_kage_colorkey(DX::DDSURFACEDESC2*_=0,D3DFORMAT=(D3DFORMAT)0){ return 0; }
extern BYTE __cdecl som_game_449530(SOM::Texture *_1, char *_2, DWORD *bpp, DWORD surfcaps, HGDIOBJ *palettes, DWORD mipmaps_s, HGDIOBJ *mipmaps)
{
	if(_1->ref_counter||_1->texture) //what's happening???
	{
		//2022: Moratheia called this while being closed in the start menu?!
		//mini map change has this set to 0xffffffff??? (som_status_mapmap2)
		if(_1->texture!=(void*)~0) assert(0);
	}

	//WARNING: Ghidra messes this subroutine up somehow. it definitely has 7 parameters
	//it seems like the stack might be off for Ghidra...

	extern char *som_game_trim_a(char*);
	som_game_449530_cmp = _2?som_game_trim_a(_2):0; //som_hacks_CreateSurface7?

	bool kage = false;
	if(char*ext=PathFindExtensionA(som_game_449530_cmp))
	{
		if(kage=ext[0]&&'M'==toupper(ext[1])) //mdl?
		{
			DDRAW::colorkey = som_kage_colorkey; //som_hacks_SetColorKey 
		}
	}

	_1->texture = 0; _1->_palette = 0; //4484B7/449571

	//NOTE: in at least one case (KAGE) the end of the path is added " TIM 000"
	//so these are actually just unique IDs. 004262c0 decomposes the hud images
	BYTE ret = ((BYTE(__cdecl*)(SOM::Texture*,char*,DWORD*,DWORD,HGDIOBJ*,DWORD,HGDIOBJ*))0x449530)(_1,_2,bpp,surfcaps,palettes,mipmaps_s,mipmaps);

	if(kage)
	{
		DDRAW::colorkey = SOM::colorkey; //2023: som.kage.cpp?

		extern void som_hacks_shadowfog(DDRAW::IDirectDrawSurface7*);
		if(ret) som_hacks_shadowfog((DDRAW::IDirectDrawSurface7*)_1->texture);
	}

	som_game_449530_cmp = 0; 

	if(!ret) return 0;

	//all textures go through this subroutine (loading)

	if(0!=_1->ref_counter) return 1;
	
	/*WORK IN PROGRESS
	//doesn't survive device reset (will need to bypass)
	//004484DD E8 4E 10 00 00       call        00449530	
	//OPTIMIZATION
	//what is _3? I think maybe it relates to using "blt" or texture
	//to render to DirectDraw. perhaps 2 means neither? map_icon.bmp
	//uses 2. maybe other maps do also
	if(SOM::newmap==SOM::frame
	&&2!=_3) //HACK: som.status.cpp is relying on the bitmap handles
	{
		//2019: I think this is just wasting memory since. There are
		//already system memory textures backing up the video memory
		//these may not take from the address space, but the process
		//must still back this memory
		for(HANDLE *p=_1->mipmaps;*p;p++){ DeleteObject(*p); *p = 0; }
	}*/

	if(DDRAW::pshaders9[8] //[Volume]
	||SOM::material_textures) //UV animation?
	{
		//HACK: allow for subdirectories in texture references
		//assuming using / and not \ as separator (x2mdl does)
		//DUPLICATE: map_442830_407470
		//char *fn = PathFindFileNameA(_2);
		char *fn = strrchr(_2,'\\'); if(!fn++)
		{
			//4262c0 has various my/texture/x made up names
			if(_2[2]=='/'||!strcmp(_2,"logo.bmp")) goto my_texture_x;
		}
		int len = PathFindExtensionA(fn)-fn;

		EX::INI::Volume vt;
		auto *v = vt->textures.data();
		for(size_t i=vt->textures.size();i-->0;)		
		if(v[i].match(fn,len))
		{
			if(!SOM::volume_textures)
			(void*&)SOM::volume_textures = new SOM::VT*[1024](); //C++ 
			auto t = _1-SOM::L.textures;
			SOM::VT cp = { v[i].group };
			//assert(!(*SOM::volume_textures)[t]);
			if((*SOM::volume_textures)[t])
			{
				//reset? indicates som_game_4498d0 is not called???
				assert(cp.group==((*SOM::volume_textures)[t])->group);
			}
			else (*SOM::volume_textures)[t] = new SOM::VT(cp);
			break;				
		}

		if(SOM::material_textures)
		{
			SOM::MT *mt = SOM::MT::lookup(fn); if(mt) 
			{		
				//TODO: for now just UV animation is used by games
				//in the future Dark Slayer will need the material
				//colors
				if(mt->mode&(8|16)==0
				||!mt->data[7]&&!mt->data[8]) mt = 0;
			}
			(*SOM::material_textures)[_1-SOM::L.textures] = mt;
		}

	}my_texture_x:;

	/*REMOVE ME
	//last minute fix prior to publishing 1.2.3.6
	//this ensures the texture is processed while
	//the map is loading and not inside which can
	//cause hiccups if a lot hit at once. this is
	//the same as 1.2.3.4 (i.e. previous release)
	if(SOMEX_VNUMBER<=0x1020308UL)
	{
		//in theory the first frame should include
		//all visible textures, but it doesn't seem
		//to, so I can't disable this until I figure
		//it out
		if(0||!EX::debug)
		if(_1->texture) _1->texture->updating_texture(0);
		else assert(0);
	}
	else assert(0);*/
	if(SOM::Game.item_lock //changing resolution, etc?
	||1==EX::central_processing_units)
	{
		if(_1->texture) _1->texture->updating_texture(0);
	}

	return 1;
}
bool SOM::Texture::update_texture(int force) //som.MPX.cpp
{
	assert(DDRAW::compat);

	bool ret;

	int cmp = force==4?3:-1;
	if(!texture||0==(cmp&texture->query9->update_pending))
	{
		ret = false; //return false;
	}
	else
	{
		assert(ref_counter);
		//NOTE: 4 always returns false for dx.d3d9X.cpp/OpenGL
		ret = texture->updating_texture(force)||force&4;
	}
	/*2023 //REMOVE ME?
	{
		DWORD t = this-SOM::L.textures; assert(t<1024); //HACK
	
		//2nd pass
		auto &ta = SOM::TextureAtlas[t];
		bool pending = ta[0].pending||ta[1].pending;
		extern void som_MPX_atlas2(DWORD);
		if(pending) som_MPX_atlas2(t); //2023
	}*/

	return ret;
}
bool SOM::Texture::uptodate() //som.MPX.cpp
{
	assert(DDRAW::compat);

	return !texture||!texture->query9->update_pending;
}
extern int Ex_mipmap_colorkey;
BYTE __cdecl som_game_444b20(SOM::MDL *_1)
{
	assert(0==Ex_mipmap_colorkey);
	//2021: this subroutine loads TIM (MDL) textures that are 16-bit and so 
	//require the old 0~7 colorkey	
	Ex_mipmap_colorkey = 7;
	BYTE ret = ((BYTE(__cdecl*)(SOM::MDL*_1))0x444b20)(_1);
	Ex_mipmap_colorkey = 0;
	return ret;
}

extern bool SOM::picturemenu = false;

static BYTE _som_game_mode = 1; //const
static BYTE _som_game_save = 1; //const

extern const BYTE &SOM::mode = _som_game_mode;
extern const BYTE &SOM::save = _som_game_save;

static float _som_game_walk = 0; //const
static float _som_game_dash = 0; //const
static float _som_game_turn = 0; //const 
						
extern const float &SOM::_walk = _som_game_walk;
extern const float &SOM::_dash = _som_game_dash;
extern const float &SOM::_turn = _som_game_turn;

extern float SOM::walk = 0, SOM::dash = 0; //2020

static BYTE _som_game_gauge = 0; //const
static BYTE _som_game_compass = 0; //const
static BYTE _som_game_capacity = 0; //const

extern const BYTE &SOM::gauge = _som_game_gauge;
extern const BYTE &SOM::compass = _som_game_compass;
extern const BYTE &SOM::capacity = _som_game_capacity;

static int _som_game_setup = 1; 
static bool _som_game_allset = false;

extern int SOM::setup()
{
	assert(_som_game_allset); return _som_game_setup;
}

//REFACTOR
//2022: this code might better go in som.MPX.cpp but it has
//a lot variables local to som.game.cpp that would be a bit
//much to pull into som.MPX.cpp
extern void som_game_on_map_change(int m)
{
	/////// formerly som_game_CreateFile /////////

	if(som_game_resetting) 
	{
		som_game_resetting = false;

		assert(_som_game_allset);
	//	SOM::et = savetime;	savetime = 0;
		SOM::eticks = EX::tick(); 			
		SOM::reset(); //hack			
	}
	
	//entering loading screen
	DSOUND::playing_delays(); //??? 

	memset(&SOM::Versus,0x00,sizeof(SOM::Versus)); //2017
}
extern void som_game_silence_wav(char *bgm)
{
	if(*bgm&&strcmp(bgm,".silence.wav")) return;

	//2020: DirectX 9's dsound.lib didn't seem to freeze if
	//a MIDI file is BGM but still up without a BGM 
	//REMINDER: 000.snd looks like it might be a silent BGM
	
	if(!*bgm) strcpy(bgm,".silence.wav");

	//2022: only first map for map streaming
	static HANDLE wav = 0; if(wav) return; 
	
	//2022: don't rewrite if opened
	wchar_t _silence_wav[MAX_PATH];
	swprintf_s(_silence_wav,L"%ls\\data\\BGM\\.silence.wav",EX::user(1));
	wav = CreateFileW(_silence_wav,GENERIC_WRITE,0,0,OPEN_EXISTING,0,0);
	if(wav!=INVALID_HANDLE_VALUE)
	{
		SOM::Game.CloseHandle(wav); return;
	}
	
	EX::clearing_a_path_to_file(_silence_wav);
	wav = CreateFileW(_silence_wav,GENERIC_WRITE,0,0,CREATE_NEW,FILE_ATTRIBUTE_HIDDEN,0);

	//there are not well understood yet serious issues 
	//whenever an MPX file is compiled without any BGM
			
	#define SOM_GAME_DATA_BYTES 0x20,0x00,0x00,0x00 //32
	#define SOM_GAME_RIFF_BYTES 0x44,0x00,0x00,0x00 //32+36

	BYTE som_game_0_wav[8+36+32] = //32 zeroes of silence //const
	{
	//	8 bits per sample was popping no matter what (so 16 bits are being used instead)
		0x52,0x49,0x46,0x46,SOM_GAME_RIFF_BYTES,0x57,0x41,0x56,0x45,0x66,0x6D,0x74,0x20,
	//	0x10,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x40,0x1F,0x00,0x00,0x40,0x1F,0x00,0x00,
		0x10,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x40,0x1F,0x00,0x00,0x80,0x3E,0x00,0x00,
	//	0x01,0x00,0x08,0x00,0x64,0x61,0x74,0x61,SOM_GAME_DATA_BYTES,0x00,0x00,0x00,0x00,
		0x02,0x00,0x10,0x00,0x64,0x61,0x74,0x61,SOM_GAME_DATA_BYTES,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
	};
						
	//som_game_0_wav is (or was) popping 
	//(but 16-bit samples magically made that go away???)
	//1: didn't help, but extend the buffer out to 1024 just in
	//case this improves buffering coroutine performance anyway
	DWORD writ; if(1&&wav!=INVALID_HANDLE_VALUE)
	{
		//2018: increasing to 4096 to limit looping/mmioRead calls
		//const int zero_extend = 31*32; //1024 0s				
		const int zero_extend = 4096-32;
		assert(32==*(DWORD*)(som_game_0_wav+40));
		char zeroes[sizeof(som_game_0_wav)+zero_extend];
		memset(zeroes,0x00,sizeof(zeroes));

		SOM::Game.WriteFile(wav,zeroes,sizeof(zeroes),&writ,0);			
				
		SetFilePointer(wav,0,0,FILE_BEGIN);
		*(DWORD*)(som_game_0_wav+4)+=zero_extend; //SOM_GAME_DATA_BYTES
		*(DWORD*)(som_game_0_wav+40)+=zero_extend; //SOM_GAME_DATA_BYTES
	}
	if(wav!=INVALID_HANDLE_VALUE)
	SOM::Game.WriteFile(wav,som_game_0_wav,sizeof(som_game_0_wav),&writ,0);	
	SOM::Game.CloseHandle(wav);
}
extern void som_game_highlight_save_game(int,bool);
static BOOL WINAPI som_game_ReadFile(HANDLE in, LPVOID to, DWORD sz, LPDWORD rd, LPOVERLAPPED o)
{	
	//SOM_GAME_DETOUR_THREADMAIN
	SOM_GAME_DETOUR_MULTITHREAD(ReadFile,in,to,sz,rd,o)

	EXLOG_LEVEL(7) << "som_game_ReadFile()\n";

	//static DWORD savetime = 0; //when loading games 

	if(in) /*OBSOLETE (2020)
	if(in==som_game_save_xx_dat)
	{	
		DWORD fp = SetFilePointer(in,0,0,FILE_CURRENT);
		
		BOOL out = SOM::Game.ReadFile(in,to,sz,rd,o);

		static DWORD last_seen_savetime = 0;

		if(out)
		if(fp>0) //more than just peeking at time
		{
			if(fp==8)
			{
				//2017: highlight xx in the save game menu
				som_game_highlight_save_game(som_game_save_xx,?);	

				savetime = last_seen_savetime; //see mpx read below
			}
		}
		else last_seen_savetime = *(DWORD*)to;	

		return out;
	}
	else*/ for(size_t i=0;i<som_game_reading;i++) 
	if(in==som_game_reading_list[i]) switch(som_game_reading_mode[i])
	{
	case 'evt':
	{
		som_game_reading_x(i); //get this out of the way

		//DWORD debug = SetFilePointer(in,0,0,FILE_CURRENT);

		//Reminder: oddly 258048 is 4 shy of the records table
		//SOM_MAP does the same thing. It's probably a bug but
		//it doesn't have any effect on SOM_MAP. No tests have
		//been run on som_db.exe to ensure that it is bug free
		//if(SetFilePointer(in,0,0,FILE_CURRENT)==0&&sz==258048)
		//The King's Field sample is reading in 270336 today???
		if(SetFilePointer(in,0,0,FILE_CURRENT)==0&&sz>=258048)
		{
			if(!SOM::Game.ReadFile(in,to,sz,rd,o)) return FALSE;

			int n = *(int*)to; //2022

			BYTE *p = (BYTE*)to+4; assert(*(DWORD*)to==1024);

			//DUPLICATE som_MPX_411a20
			for(int i=0;i<n;i++,p+=252) switch(p[31])
			{																	   
			case 0xFE: //System

				//2020: this is a problem for the Play Sound event, which seems
				//to be the only event that refers back to the subject outside 
				//of predicates
				//
				// I mapped out all of the events and it looks like 0x40 doesn't 
				// test the subject (I think maybe I just assumed it did, there's
				// actually no way for it to do so
				//
				//if(i>9) p[31] = 0; //NPC (needed for 0x40) (2020: actually not)

			default: SOM::ezt[i] = false; break; 

			case 0xFD: //Systemwide
			case 0xFC: //conflicted (shouldn't be seen but)

				if(i<10) p[31] = 0xFE; //System

				//2020: see above
				//if(i>9) p[31] = 0; //NPC (needed for 0x40) (2020: actually not)

				SOM::ezt[i] = true; break;
			}
			return TRUE;
		}
		else assert(0); break;
	}
	case 'sys': //PARAM\SYS.DAT
	{	
		DWORD fp = SetFilePointer(in,0,0,FILE_CURRENT);

		if(!SOM::Game.ReadFile(in,to,sz,rd,o)) return FALSE;

		EX::INI::Option op;

		if(fp==0&&sz>=0x8000)
		{
			if(op->do_dash) //2017: assuming dashing is desired
			*((BYTE*)to+254) = 1;
			_som_game_mode = *((BYTE*)to+254);
			auto *c = EX::need_calculator(); //2024
			SOM::walk = //2020
			_som_game_walk = EX::reevaluating_number(c,L"_WALK"); //*((FLOAT*)to+64);
			SOM::dash = //2020
			_som_game_dash = EX::reevaluating_number(c,L"_DASH"); //*((FLOAT*)to+65);
			_som_game_turn = EX::reevaluating_number(c,L"_TURN"); //*((SHORT*)to+132);
			if(op->do_save)
			*((BYTE*)to+722) = 1;
			_som_game_save = *((BYTE*)to+722); 
			_som_game_gauge = *((BYTE*)to+725);
			_som_game_compass = *((BYTE*)to+724);
			_som_game_capacity = *((BYTE*)to+723);
												  
			/*//force gauges on
			2020: the compass goes first, so using it
			if(SOM::L.on_off)*/ *((BYTE*)to+724) = 1; //725
			//turns out do_st needs to show both gauges
								*((BYTE*)to+725) = 1; //725
			
			wchar_t setup[2] = L"1";
			GetEnvironmentVariableW(L"SETUP",setup,2);
			_som_game_setup = *setup=='2'?2:1;			
			_som_game_allset = true;

			if(_som_game_setup==1&&SOM::game==som_db.exe)
			{
				//copy player setup over som_db player setup
				memcpy((BYTE*)to+0x604,(BYTE*)to+0x4F0,276);
			}
			else if(_som_game_setup==2&&SOM::game==som_rt.exe)
			{
				memcpy((BYTE*)to+0x4F0,(BYTE*)to+0x604,276);
			}			
		}
		//NEW: do_fix_zbuffer_abuse
		if(fp<=33594&&fp+sz>=33595) 
		{
			//if(EX::debug) *((BYTE*)to+33594-fp) = false;

			//first letter of the picture filename
			SOM::picturemenu = *((BYTE*)to+33594-fp);
			
			som_game_reading_x(i); //!!
		}		
		return TRUE; 
	}}

	return SOM::Game.ReadFile(in,to,sz,rd,o);
}

static BOOL WINAPI som_game_WriteFile(HANDLE in, LPCVOID at, DWORD sz, LPDWORD wr, LPOVERLAPPED o)
{
	//Then this is logging: so lets not log logging!!
	if(in==EX::monolog::handle) return SOM::Game.WriteFile(in,at,sz,wr,o);

	SOM_GAME_DETOUR_THREADMAIN(WriteFile,in,at,sz,wr,o)
	
	EXLOG_LEVEL(7) << "som_game_WriteFile()\n"; 

	if(in) /*OBSOLETE (2020)
	if(in==som_game_save_xx_dat)
	{	
		DWORD fp = SetFilePointer(in,0,0,FILE_CURRENT);
		
		if(fp==0&&sz>=4) //repair elapsed time in save file
		{
			EX::INI::Bugfix bf;

			if(bf->do_fix_elapsed_time) *(DWORD*)at = SOM::et;
		}
	}*/

	return SOM::Game.WriteFile(in,at,sz,wr,o);
}

static BOOL WINAPI som_game_CloseHandle(HANDLE in)
{	
	//SOM_GAME_DETOUR_THREADMAIN
	SOM_GAME_DETOUR_MULTITHREAD(CloseHandle,in)

	EXLOG_LEVEL(7) << "som_game_CloseHandle()\n";

	if(!in) return SOM::Game.CloseHandle(0); //loopy

	if(in==som_game_save_xx_dat)
	{
	  //don't close save files since SOM opens//
	  //and closes them on every frame on save//
	  //and load screens. Which is fairly nuts//
	  //(especially for access over a network)//
	
		if(som_game_saves[som_game_save_xx]==som_game_save_xx_dat)
		{
			som_game_save_xx_dat = 0; return TRUE; //cached file
		}
		else som_game_save_xx_dat = 0; //writing?
	}
	else for(size_t i=0;i<som_game_reading;i++) 
	{
		if(in==som_game_reading_list[i]) 
		{	
			som_game_reading_x(i); break;
		}
	}

	return SOM::Game.CloseHandle(in);
}

static void som_game_timeout()
{	
	if(!SOM::capture) SOM::cursorZ = 0;
}

static void som_game_map_xy(int dx=0, int dy=0)
{
	if(SOM::mapmap>=SOM::frame-1&&0==EX::context())
	{
		extern void som_status_map_xy(int,int);
		som_status_map_xy(dx,dy);
	}
}

//REMOVE ME?
VOID CALLBACK som_game_emergencyexit(HWND,UINT,UINT,DWORD)
{
	EX::is_needed_to_shutdown_immediately(-1,"som_game_emergencyexit");
}
static LRESULT CALLBACK som_game_WindowProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l)
{
	assert(!SOM::tool);

	if(hwnd!=SOM::window) 
	{
		return CallWindowProc(SOM::WindowProc,hwnd,msg,w,l);
	}

	assert(SOM::WindowProc); //paranoia
				
	switch(msg) 
	{
	case WM_DEVICECHANGE: //2019: hot plug controllers

		if(w==7) //DBT_DEVNODES_CHANGED 
		{
			//2021: this is needed if there's another controller
			//already plugged in, otherwise 44aa00 returns FALSE
			SOM::L.controller_count = 0;

			//00401962 E8 49 F6 00 00       call        00410FB0
			//0040F1F6 E8 05 B8 03 00       call        0044AA00
			if(SOM::field) ((void(*)())0x44aa00)();
		}
		break;

	case WM_ACTIVATE: 
	
		if(EX::window)
		{
			SetActiveWindow(EX::window); 

			//2018: enabling via taskbar
			if(w!=WA_CLICKACTIVE) 
			{
				EX::activating_cursor();
				//EX::unclipping_cursor();
			}
		}					
		break;

	case WM_SETFOCUS: 
	
		if(EX::window) SetForegroundWindow(EX::window); 
		break;
	
	//case WM_GETTEXT: //window caption
		
		//if(SOM::caption&&*SOM::caption) 
		//SetWindowTextW(SOM::window,SOM::caption);  		
		//goto def;
		
	//case WM_PAINT: 
		
		//EXLOG_LEVEL(1) << "WindowProc: WM_PAINT\n"; 
		//break;
	
	case WM_MOVE:
		
		if(EX::window&&!EX::fullscreen)
		{
			SOM::windowX = EX::coords.left-EX::screen.left;
			SOM::windowY = EX::coords.top-EX::screen.top;
		} 		
		break;

	case WM_SETCURSOR: EX::inside = true; 
		
		//2020: testing: cursor is hidden even if
		//Ex_cursor_ShowCursor rejects all requests
		//to hide it???
	//	if(!EX::is_captive()) EX::showing_cursor();

		return 1; //break;
		
	case WM_MOUSEMOVE: //FALLING THROUGH
		
		if(EX::window)
		{
			//Note: cursor cannot be negative

			POINT _ = {GET_X_LPARAM(l),GET_Y_LPARAM(l)};

			ClientToScreen(SOM::window,&_);

			_.x-=GetSystemMetrics(SM_XVIRTUALSCREEN);
			_.y-=GetSystemMetrics(SM_YVIRTUALSCREEN);

			assert(_.x>=0&&_.y>=0);

			if(_.x<0) _.x = 0; if(_.y<0) _.y = 0;

			int dx = EX::x, dy = EX::y;
			EX::following_cursor(_.x,_.y);
			dx-=EX::x; dy-=EX::y;

			//NOTE: dx/dy are 1/1 for some reason
			//even when no movement occurs			
			if(!SOM::capture&&(dx|dy)>2)
			{
				EX::showing_cursor();
			}

			som_game_map_xy(dx,dy);

			SetTimer(SOM::window,'wmmm',300,0);
			
			return 0;
		}
		break;

	case 0x020E: //WM_MOUSEHWHEEL:

		if(EX::window&&EX::active)
		{
			static int wheel = 0;			

			int fpm = 1; //frames per messages

			static unsigned delta = 0;

			if(delta!=SOM::frame)
			{
				fpm = SOM::frame-delta;
			}

			wheel+=GET_WHEEL_DELTA_WPARAM(w);

			if(wheel>=WHEEL_DELTA)
			{
				wheel-=WHEEL_DELTA;

				SOM::tilt = +fpm;
			}
			else if(wheel<=-WHEEL_DELTA)
			{
				wheel+=WHEEL_DELTA;

				SOM::tilt = -fpm; 
			}			   
			return 1; //return 0; //Logitech bug.
		}		 
		break;

	case WM_MOUSEWHEEL:

		if(EX::pointer=='hand') //2024
		{
			som_MPX &mpx = *SOM::L.mpx->pointer;

			int curl = SOM::mapL; 
			int del = GET_WHEEL_DELTA_WPARAM(w);
			if(del>0)
			{
				while(curl)
				{
					auto &ll = ((SOM::MPX::Layer*)&mpx[SOM::MPX::layer_0])[++curl];
					if(ll.tiles) break;
				}
			}
			else while(curl>-6)
			{
				auto &ll = ((SOM::MPX::Layer*)&mpx[SOM::MPX::layer_0])[--curl];
				if(ll.tiles) break;
			}

			auto &ll = ((SOM::MPX::Layer*)&mpx[SOM::MPX::layer_0])[curl];
			if(!ll.tiles) curl = 0;

			if(SOM::mapL!=curl)
			{
				SOM::mapL = curl;

				extern void som_status_mapmap2__automap();
				som_status_mapmap2__automap();
			}
			
			return 1;
		}

		if(EX::window/*&&EX::active*/)
		if(EX::INI::Option()->do_mouse)
		{
			static int wheel = 0;			

			wheel+=GET_WHEEL_DELTA_WPARAM(w);

			int delta = SOM::cursorZ;

			if(wheel>=WHEEL_DELTA)
			{
				wheel-=WHEEL_DELTA;

				SOM::cursorZ = min(8,SOM::cursorZ+1);
			}
			else if(wheel<=-WHEEL_DELTA)
			{
				wheel+=WHEEL_DELTA;

				SOM::cursorZ = max(-8,SOM::cursorZ-1);
			}
			else return 1;

			if(EX::pointer!='z')			
			if(delta<=0&&SOM::cursorZ>0
			 ||delta>=0&&SOM::cursorZ<0)
			{
				SOM::f10(); return 1;									
			}

			//if(!EX::timeout) //NEW 12/3/2012
			{
				EX::timeout = som_game_timeout;
			}

			return 1;
		}	   

		//2018: The default procedure is overflowing on
		//WM_MOUSEWHEEL when debugging. Just returning
		//since som_db doesn't consider mouse wheels
		//(Ex.window.cpp just passes the message)
		//break;									
		return 1;

	case WM_RBUTTONDOWN:

		//2022: capture is being lost??
		if(SOM::capture&&!GetCapture())
		{
			//HACK? this shouldn't be done when the cursor
			//is floating over the Alt+M+M map for instance
			if(!EX::pointer) EX::capturing_cursor(); 
		}

		//2020: let weak mouse advance text/menus?
		if(0!=EX::context()&&EX::is_cursor_hidden()) //???
		{
			dik_space: //WM_LBUTTONDOWN
			if(!SOM::capture)
			{
				EX::Syspad.send(msg==WM_LBUTTONDOWN?0x39:1); //SPACE/ESCAPE

				//2022: there was some code here (pretty recent) entering
				//into capture mode, but that's too out of left field, so
				//this code forces weak capture when exiting out of menus
				//(so players don't have to think about the capture mode)
				extern DWORD SomEx_recapture_tick;
				SomEx_recapture_tick = EX::tick();
			}
		}		
		else if(EX::window)
		{
			if(EX::arrow()) EX::hiding_cursor(); //hide z
		}

		break;

	case WM_RBUTTONDBLCLK:

		if('hand'==EX::pointer)
		SOM::mapZ = SOM::mapZ==0?-1:-SOM::mapZ;
		break;
		
	case WM_LBUTTONDBLCLK:

		EX::crosshair = true; break;

	case WM_LBUTTONDOWN:

		EX::crosshair = false;	

		//2022: capture is being lost??
		if(SOM::capture&&!GetCapture())
		{
			//HACK? this shouldn't be done when the cursor
			//is floating over the Alt+M+M map for instance
			if(!EX::pointer) EX::capturing_cursor(); 
		}

		if(EX::is_cursor_hidden())		
		if(0!=EX::context())
		{
			goto dik_space; //2020
		}
		else
		{
			//2020: show briefly for weak capture feedback
			//(keeping hidden)
			if(!SOM::capture&&!EX::is_captive())
			{
				EX::showing_cursor();
			}
		}
		//break;

	case WM_LBUTTONUP:

		som_game_map_xy(); //DOWN/UP
		
		//2020: inverting logic to be able to
		//double-click
		if(msg==WM_LBUTTONUP) if(EX::window) //?
		{
			//bool alt = EX::alt();
			bool alt = GetKeyState(VK_MENU)>>15;
			bool weak = !SOM::capture&&EX::arrow()&&!alt;
			if(!weak&&'z'!=EX::pointer) break;

			//2020: this is hiding the crosshair when activated
			if(weak&&EX::is_captive()) break;

			if(!EX::crosshair)
			{
				//wait for WM_LBUTTONDBLCLK (assuming double click) 			
				//STUPID... returns immediately for this message???
				//EX::crosshair = !MsgWaitForMultipleObjects(0,0,0,GetDoubleClickTime()/2,QS_MOUSEBUTTON);
				SetTimer(hwnd,'lbtn',GetDoubleClickTime()/2,0);
				return 0;
			}
			else goto dblclickup;
		}
		break;

	case WM_TIMER: //keep cursor alive while mouse is stationary

		//I THINK THIS TIMER IS MEANT TO REPEAT INDEFINITELY
		//BECAUSE THE CURSOR ISN'T HIDING ITSELF ONCE KILLED
		//2018: without this the timer should repeat! Don't know
		//how this will change things (or how they ever worked?)
		//KillTimer(hwnd,w); 

		switch(w)
		{
		case 'wmmm': EX::following_cursor(); break;			
		case 'exit': 
			//2020: Windows is leaking into underneath application!?
			while(GetAsyncKeyState(VK_F4)) EX::sleep(0);
			//2021: som_hacks_thread_main now keeps the message pump
			//alive in case a (direct3d9) DLL hangs
			EX::is_needed_to_shutdown_immediately(0,"WM_TIMER"); break;
		case 'lbtn': //CONTINUED
			
			KillTimer(hwnd,w);

			if(EX::crosshair) return 0; //wait for second LBUTTONUP?

		dblclickup:

			//used to be split over DOWN/UP
			if('z'==EX::pointer)
			{	
				SOM::z(true); //DOWN
				SOM::z(false); //UP
			}
			else //weak capture
			{
				//for good measure				
				EX::hiding_cursor(); //DOWN
				EX::holding_cursor(); //DOWN
				
				EX::capturing_cursor(); //UP
				EX::clipping_cursor(); //UP
			}
			break;		
		}
		return 0; //wait for UP		

	case WM_MOUSEACTIVATE:

		if(EX::window) 
		{	
			//2017: Let Alt/F10 clicks work with som_game_map_xy().
			EX::inside = true;

			int disposition = MA_NOACTIVATE;
			
			if(!EX::active&&!EX::arrow()) 				
			disposition = MA_NOACTIVATEANDEAT;

			//hack: EX.window.cpp must hook EX::client WindowProc
			SetActiveWindow(EX::window); EX::activating_cursor();

			if(EX::pointer!='z'&&EX::pointer!='hand')
			{
				EX::hiding_cursor();
			}
			
			return disposition;
		}			 											
		break;

	case WM_KEYDOWN: 
	//case WM_SYSKEYDOWN: //alt+pause?

		switch(w)
		{
		case VK_PAUSE: //a pause is an event 		
		case VK_CANCEL: //ctrl+pause (break)

			EX::vk_pause_was_pressed = true; break;
		}		
		break;

	case WM_KEYUP: break; 

	/*case WM_SYSKEYUP: case WM_SYSKEYDOWN:
		
		if(w==VK_F10) //Ex_window_proc 
		{
			//doesn't send WM_SYSKEYUP?!
			if(msg==WM_SYSKEYDOWN) return 0;
			
			//2020: Windows is hiding the cursor on release???
			SOM::f10(); break;
		}		
		break;*/

	case WM_CLOSE: 
		
		EXLOG_HELLO(3) << "WM_CLOSE\n";

		//simulate ALT+F4 so that SOM will flush ini file

		if(!SOM::altf4(1)) //see: altf4 (som.state.cpp)
		{
			//paranoia: should not get here
			EX::is_needed_to_shutdown_immediately(0,"WM_CLOSE"); 
		}				
		return 0; 
		
	case WM_DESTROY: 
		
		EXLOG_HELLO(3) << "WM_DESTROY\n";

		//Now waiting for SOM to write to ini file
		//EX::is_needed_to_shutdown_immediately(0);
																	
		//TODO: fix assert dialog close on exit		 

		if(SOM::altf4(1)) //abnormal exit
		{
			EX::is_needed_to_shutdown_immediately(0,"Alt+F4");
		}	

		//NEW: process is hanging around today (10/25/2015)
		SetTimer(0,0,EX::debug?1000:5000,som_game_emergencyexit);

		goto def;
	}

som:return CallWindowProc(SOM::WindowProc,hwnd,msg,w,l); //just passing thru for now

def:return DefWindowProc(hwnd,msg,w,l);
}

static BYTE __cdecl som_game_BGM_401300(const char *p, DWORD ext)
{
	p = strrchr(p,'.'); if(!p) return 0;

	if(ext==0x45F338) //wav (43F749 and 44C231)
	{
		switch(tolower(p[1]))
		{
		default: return 1; 
		case 'm': ext = 0x45F330; break; //mid
		case 'r': ext = 0x45F32C; break; //rmi
		case 's': ext = p[2]&1?0x45F334:0x45F328; //snd/smf
		}
		//NOTE: rejecting if MIDI or SND
		return 0!=stricmp((char*)ext,p+1);
	}
	else //2020
	{
		assert(ext!=0x45F334); //SND

		//for some reason this routine is INTENSE so might want to
		//reimplement it (and investigate it)
		//45e54c //mdo (42a72f)
		//45e550 //mdl (42a703)
		//45aae8 //txr (448618)
		//45f490 //dib (448606)
		//45f494 //bmp (4485f4)
		//45F338 //wav (44b496)
		//45F334 //snd (44b4ba)
		//(some more are mid/rmi/smf but I think they're covered
		//above)
	}
	return !stricmp((char*)ext,p+1);
}
extern char *som_game_trim_a(char *a) //2020
{
	enum{ len = sizeof(SOMEX_(A)) };
	return (char*)a+len*!memcmp(a,SOMEX_(A)"\\",len);
}
extern DWORD __cdecl som_game_44b450(char *snd, DWORD _2, DWORD _3)
{
	char *a = som_game_trim_a(snd); 

	if(!PathFileExistsA(a)) //optimizing?
	if(const wchar_t*wname=SOM::Game::file(a))
	{	
		wchar_t vname[MAX_PATH]; 
		if(!EX::data(wname+5,vname))
		{
			//TODO: FindFirstFile? how to support more aliases?
			memcpy(a+strlen(a)-3,"wav",3);
		}
	}

	return ((DWORD(__cdecl*)(char*,DWORD,DWORD))0x44b450)(snd,_2,_3);
}
static char *som_game_sound_bgm(char sound[], const char *a)
{
	if(strncmp(a,"data\\bgm\\",9)) return 0;
	memcpy(sound,"data\\sound\\bgm\\",15);
	strcpy(sound+15,(char*)a+9); return sound;
}
static MCIERROR WINAPI som_game_mciSendCommandA(MCIDEVICEID A, UINT B, DWORD_PTR C, DWORD_PTR D)
{
	MCIERROR ret = SOM::Game.mciSendCommandA(A,B,C,D);

	//NOTE: this is for MIDI based BGM but it freezes up on loop and there is
	//a problem with Direct Sound freezing when it has not music to play until
	//a sound effect plays that is cause by MIDI so I think MIDI has to be done
	//another way
	//(Doing it on another thread could solve the loop problem)
	if(B==MCI_OPEN)
	{
		//SOM_GAME_DETOUR_THREADMAIN
		SOM_GAME_DETOUR_MULTITHREAD(mciSendCommandA,A,B,C,D)

		EXLOG_LEVEL(7) << "mciSendCommandA(MCI_OPEN)\n";

		//MCI_OPEN happens at the following address/call
		//(it might make more sense to just commandeer that call, but it's not 
		//a normal call)
		//0042362E FF D7                call        edi

		LPMCI_OPEN_PARMSA p = (LPMCI_OPEN_PARMSA)D;

		if(ret) //&&SOM::game==som_db.exe)
		{
			char sound[64]; 
			const char *swap = p->lpstrElementName;
			if(p->lpstrElementName=som_game_sound_bgm(sound,swap))
			{
				ret = SOM::Game.mciSendCommandA(A,B,C,D);			
				p->lpstrElementName = swap;
			}
		}
	}
	return ret;
}
extern HMMIO som_game_mmioOpenA_BGM = 0; //2020
extern HMMIO WINAPI som_game_mmioOpenA(LPSTR A, LPMMIOINFO B, DWORD C)
{
	//SOM_GAME_DETOUR_THREADMAIN
	SOM_GAME_DETOUR_MULTITHREAD(mmioOpenA,A,B,C)

	EXLOG_LEVEL(7) << "som_game_mmioOpenA()\n";
	
	EXLOG_LEVEL(3) << "Opening " << A << '\n';

	//2020: PathFileExistsA?
	A = som_game_trim_a(A);

	bool bgm = !strncmp(A,"data\\bgm\\",9); //2020

	if(bgm) som_game_mmioOpenA_BGM = 0;
	
	//EXPERIMENTAL
	/*
	0x0045F324  snd.
	//0043F7A3 68 28 F3 45 00       push        45F328h
	0x0045F328  smf. //MIDI
	//0043F78D 68 2C F3 45 00       push        45F32Ch 
	0x0045F32C  rmi. //MIDI
	//0043F777 68 30 F3 45 00       push        45F330h
	0x0045F330  mid. //MIDI	
	//0043F73B 68 38 F3 45 00       push        45F338h
	//0043F75D 68 34 F3 45 00       push        45F334h 
	///44C210 seems to only deal with WAV/SND
	//0044C248 68 34 F3 45 00       push        45F334h 
	0x0045F334  snd.
	//0044C22B 68 38 F3 45 00       push        45F338h
	0x0045F338  wav.
	*/
	bool wav = true;
	const char *ext = PathFindExtensionA(A);
	wav = !ext||!stricmp(ext+1,"wav");

	char sound[64]; while(A)
	{		
		wchar_t *w = 0,  vname[MAX_PATH]; //2023
		if(!strnicmp(A,"DATA\\",5)) 
		{
			//2018: assuming not returning this
			//static wchar_t v[MAX_PATH] = L"";
			if(EX::data(A+5,vname)) w = vname;
		}
		else assert(0);

		if(w) if(!wav) //TODO? can cache maybe?
		{
			if(PathFileExistsA(A)) nonPCM:
			{
				//these fill up the TEMP folder if the program is 
				//terminated
				//static EX::temporary bgm;
				static EX::temporary tmp[2];
				//auto &tmp = tmp2[bgm];
				//if(!tmp) tmp = new EX::temporary;
				//auto &buf = tmp[bgm];
				auto &buf = tmp[bgm];
				SetFilePointer(buf,0,0,FILE_BEGIN);				
				SetEndOfFile(buf);
	
				//EXPERIMENTAL
				extern bool dx_dshow_begin_conversion(const wchar_t*[2],const wchar_t*[2],HANDLE,bool);
				{
					const wchar_t *src[2] = {SOM::Game::project(A)};
					const wchar_t *dst[2] = {buf,L"wav"};
					if(!dx_dshow_begin_conversion(src,dst,buf,!bgm))
					return 0;
				}

				HMMIO out = mmioOpenW((LPWSTR)(LPCWSTR)buf,B,C);

				if(bgm) som_game_mmioOpenA_BGM = out;

				return out;
			}
		}
		else if(HMMIO out=mmioOpenW(w,B,C)) //PCM?
		{
			MMCKINFO fmt;
			if(!SOM::Game.mmioDescend(out,&fmt,0,0)) //parent/RIFF 
			{
				if(8==fmt.dwDataOffset&&fmt.fccType==0x45564157) //WAVE
				{
					//HACK: enforcing a 44B header, otherwise processing
					//the file through DirectShow so that it will be 44B
					//can't seem to make this work					
					if(SOM::Game.mmioDescend(out,&fmt,0,0)!=MMIOERR_CHUNKNOTFOUND
					&&fmt.cksize!=16&&fmt.ckid==0x20746D66) //(fmt )
					{
						mmioClose(out,0); goto nonPCM; //EXPERIMENTAL
					}
					else assert(fmt.cksize==16);
				}
				else assert(0);
			}
			mmioSeek(out,0,SEEK_SET);
			
			if(bgm) som_game_mmioOpenA_BGM = out;

			return out;
		}
		A = A==sound?0:som_game_sound_bgm(sound,A);
	}

	return 0;
}
static LONG WINAPI som_game_mmioRead(HMMIO A, HPSTR B, LONG C)
{
	//SOM_GAME_DETOUR_THREADMAIN
	SOM_GAME_DETOUR_MULTITHREAD(mmioRead,A,B,C)

	EXLOG_LEVEL(7) << "som_game_mmioRead()\n";

	//EXPERIMENTAL
	//streaming non PCM audio? see if file has
	//grown
	//if(C==SOM::L.bgm->fmt.nSamplesPerSec)
	extern bool dx_dshow_media_release(bool=true);
	if(C>=40) //assuming header read if not
	if(A==som_game_mmioOpenA_BGM) //2020
	{
		enum{ data0=44 }; //dx_dshow_media_release

		LONG fp = 0; buffering: 		
		if(dx_dshow_media_release())
		{
			if(!fp)
			{
				fp = mmioSeek(A,0,SEEK_CUR);
				assert(SOM::L.bgm->head==fp-data0); 
				//I think som_game_mmioDescend's hack resolves
				//this issue, but head is repaired anyway just
				//in case it doesn't estimate enough, or it is
				//dependent on load times
				if(fp<0)
				{
					//this will probably skip, but it's not worth
					//programming a better repair in the unlikely
					//event that it occurs
					fp = mmioSeek(A,44+max(0,SOM::L.bgm->head),SEEK_SET); 
					//doing this below regardless
					//SOM::L.bgm->head = fp-data0;
				}				
				//I think som_game_mmioDescend's hack resolvse
				//this issue, but head is repaired anyway just
				//in case it doesn't estimate enough, or it is
				//dependent on load times
				assert(SOM::L.bgm->head==fp-data0); 
				//BLACK MAGIC
				//somehow the read head is getting out of sync
				//but this magically fixes it					
				SOM::L.bgm->head = fp-data0; //patching?
			}

			//ASSUMING NO CHUNKS FOLLOW DATA!
			LONG sz = mmioSeek(A,0,SEEK_END)-data0;
			LONG cmp = sz-SOM::L.bgm->size;
			if(cmp>0)
			{
				SOM::L.bgm->size = sz;					
			}
			else if(cmp==0) //maybe finished
			{
				//dx_dshow_media_release()
			}
			else //assert(0); 
			{
				//in case som_game_mmioDescend's hack gets ahead
				//dx_dshow_media_release
				//NOTE: assuming BGM is at least several seconds
				//long!
				goto buffering2;
			}

			mmioSeek(A,fp,SEEK_SET);
			
			//= is prevent loops
			if(data0+sz<=fp+C) buffering2:
			{
				EX::sleep(); goto buffering; 
			}
		}
	}

	return SOM::Game.mmioRead(A,B,C);
}

static MMRESULT WINAPI som_game_mmioDescend(HMMIO A, LPMMCKINFO B, const MMCKINFO *C, UINT D)
{
	//SOM_GAME_DETOUR_THREADMAIN
	SOM_GAME_DETOUR_MULTITHREAD(mmioDescend,A,B,C,D)

	EXLOG_LEVEL(7) << "som_game_mmioDescend()\n";

	MMRESULT ret = SOM::Game.mmioDescend(A,B,C,D);
	
	//EXERIMENTAL (dx_dshow_begin_conversion)
	if(A==som_game_mmioOpenA_BGM) //2020
	if(!ret&&B&&!B->cksize&&B->ckid==0x61746164) //data
	{
		WAVEFORMATEX *fmt = (WAVEFORMATEX*)&SOM::L.bgm;
		//speculate large as CreateSoundBuffer descriptor 
		//B->cksize = SOM::L.bgm->fmt.nSamplesPerSec;
		B->cksize = 2*fmt->nBlockAlign*fmt->nSamplesPerSec;
		//HACK: this is meant to avoid artifical loops at
		//the top because the EBX register is loaded with
		//the remainder, and it isn't practical to change
		//it inside of mmioRead
		B->cksize*=2; //4 seconds?
		B->cksize*=2; //8 seconds? 
	}

	return ret;
}

static MMRESULT WINAPI som_game_mmioAscend(HMMIO A, LPMMCKINFO B, UINT C)
{
	//SOM_GAME_DETOUR_THREADMAIN
	SOM_GAME_DETOUR_MULTITHREAD(mmioAscend,A,B,C)

	EXLOG_LEVEL(7) << "som_game_mmioAscend()\n";

	return SOM::Game.mmioAscend(A,B,C);
}

static MMRESULT WINAPI som_game_mmioClose(HMMIO A, UINT B)
{
	//SOM_GAME_DETOUR_THREADMAIN
	SOM_GAME_DETOUR_MULTITHREAD(mmioClose,A,B)

	EXLOG_LEVEL(7) << "som_game_mmioClose()\n";

	//2017: found this commented out! Doesn't seem like it should be???
	//USED? added assert(0);
	//som_game_hmmios_x(A); //free it up

	extern HMMIO som_MPX_mmioOpenA_BGM; 
	if(A==som_MPX_mmioOpenA_BGM) return 0; //som_MPX_mmioOpenA_BGM_close

	return SOM::Game.mmioClose(A,B);
}

static INT32 som_game_convert_note_to_freq(INT32 _1, INT32 _2) //43fa30
{
	//int cmp = ((INT32(__cdecl*)(INT32,INT32))0x43fa30)(_1,_2);
	//if(0) return cmp;

	//2024: note, this is unaltered (it's PSX like I think)

	if(_2==0) return _1;

	if(12<_2&&_2<=24) return _1*2+((_2-12)*_1*2)/12;

	if(0<(char)_2&&_2<13) return _1+(_2*_1)/12;

	INT32 i1,i2,i3;
	if(_2<-12||(BYTE&)_2<128)
	{
		if(_2<-24||_2>-13) return _1; //?

		i2 = _1/2; i3 = _2+12; i1 = _1/4;
	}
	else
	{
		i3 = _1/2; i1 = _2; i2 = _1;
	}
	return i2-(i3*i1)/-12;
}
extern DWORD som_game_43fa30_freq = 0;
extern WORD som_SFX_SND_to_sound(WORD);
static DWORD __cdecl som_game_43fa30(DWORD _1, char _2c, const DWORD snd) //stack
{	
	if(snd>=1024) //65535?
	return som_game_convert_note_to_freq(_1,_2c); //2024

	auto &wav = SOM::L.snd_bank[snd];

	if(_1==0) //2024: trying to load sound just-in-time
	{
		assert(!*wav.sb);

		extern BYTE __cdecl som_MPX_43f420(DWORD,BYTE);
		som_MPX_43f420(snd,1);

		_1 = wav.fmt.nSamplesPerSec;
	}
	//assert(_1==wav.fmt.nSamplesPerSec); //this should hold but doesn't

	INT32 _2 = _2c;
	EX::INI::Sample se;
	if(&se->sample_pitch_adjustment) //snd is on the stack	
	{
		//there are at least two calls that are passing bogus pitch
		//values (42eae9 & 42f2c9) one is fireball, the other may be
		//be flames
		if(_2<-24||_2>20) _2 = 0;

		//TODO? what about pre-converting to frequency?
		//2021: SOM's pitch range doesn't make a big difference
		//to adjust the pitch for monster scales (KF2 does this) the 
		//built-in "npc" number is being set by som.logic.cpp
		_2 = (int)se->sample_pitch_adjustment(som_SFX_SND_to_sound(snd),_2c,_1,_2);

		//this allows for values outside the -24,20 range (which could
		//be extended to -128,127)
		if(_2>127) goto npc;
	}
	_2 = som_game_convert_note_to_freq(_1,_2);

	extern int SomEx_npc; npc:

	if(SomEx_npc!=-1) //2024: som_MPX_reset_model_speeds?
	{
		DWORD m = 0;
		int ai = SomEx_npc&0xfff; switch(SomEx_npc&3<<12)
		{
		case 0x0000: m = SOM::L.ai[ai][SOM::AI::mdl]; break;
		case 0x1000: m = SOM::L.ai2[ai][SOM::AI::mdl2]; break;
		case 0x2000: m = SOM::L.ai3[ai][SOM::AI::mdl3]; break;
		case 0x3000: m = (DWORD)*&SOM::L.arm_MDL; break;
	//	default: assert(0);
		}
		if(auto*l=(SOM::MDL*)m)
		{
			float fps = 30.0f*som_MDL::fps/DDRAW::refreshrate;

			float x = l->ext.speed;

			if(*(*l)->file_head&16) x/=2; //60fps? 

			if(SomEx_npc==12288)
			{
				if(l->d<1) x = l->ext.speed2; //shield

				fps*=2; //2 is because arm.mdl is twice as long
			}

			if(x) _2/=fps/x; //zero divide
		}
	}

	som_game_43fa30_freq = _2; //2024

	return _2;
}
extern void som_game_volume_level(int snd, int &_2)
{	
	EX::INI::Sample se;

	if(&se->sample_volume_adjustment)
	{
		_2 = min(0,max(-10000,_2));

		//TODO? can this be logarithmic?
		_2 = se->sample_volume_adjustment(som_SFX_SND_to_sound(snd),_2/100.0f)*100;
		_2 = min(0,_2);
	}
	_2+=SOM::millibels[1]; _2 = min(0,max(-10000,_2));
}
extern void som_game_pitch_to_frequency(int snd, int &_3)
{	
	SOM::L::WAV &sb = SOM::L.snd_bank[snd]; 

	_3 = som_game_43fa30(sb.fmt.nAvgBytesPerSec,_3,snd);
}
static void __cdecl som_game_44c100(DWORD _1, INT32 _2, DWORD _3, DWORD _4, FLOAT _5, FLOAT _6, FLOAT _7)
{
	//NOTE: max_dist is fixed to 15. if long distance buffers don't survive
	//map changes it should be pegged to the draw distance, but even that 
	//can be dynamic with Ex.ini (I'm increasing it in som_game_reprogram)
	float d2 = powf(SOM::xyz[0]-_5,2)+powf(SOM::xyz[1]-_6,2)+powf(SOM::xyz[2]-_7,2);
	d2 = sqrt(d2)/SOM::L.snd_max_dist;
	//if(d2>=powf(SOM::L.snd_max_dist,2)) return;
	if(d2>1)
	{
		som_game_43fa30_freq = 0; return;
	}

	//SOM plays sounds for anything that's active. objects seem to be active
	//for the entire map at all times
	//EX::dbgmsg("3d: %d (%d)",_1,SOM::frame);

	//TODO: this can take into account the MPX BSP structure or the scale of
	//the sound source
	int mute = d2>0.5f?pow((d2-0.5f)*2,5)*5000:0;
	//EX::dbgmsg("3d: %d (%d)",_1,mute);

	//previously som_hacks_SetVolume
	{
		if(SOM::se_volume) //HACK (footsteps)
		{
			_2 = SOM::se_volume; SOM::se_volume = 0; //+=
		}
		else _2 = 0;
	}

	som_game_volume_level(_1,_2); _2-=mute;

	if(_2>-10000)
	((void(__cdecl*)(DWORD,INT32,DWORD,DWORD,FLOAT,FLOAT,FLOAT))0x44c100)(_1,_2,_3,_4,_5,_6,_7);
}
static void __cdecl som_game_44b6f0(DWORD _1, INT32 _2, DWORD _3, DWORD _4)
{
	//previously som_hacks_SetVolume
	{
		if(SOM::se_volume) //HACK
		{
			_2 = SOM::se_volume; SOM::se_volume = 0; //+=

			//HACK: do_start mute isn't taking
			if(-10000==SOM::se_volume) return;
		}
		else _2 = 0;
	}
	//2023
	{
		if(SOM::se_looping) _4 = 1;
	}
	
	//EXTENSION?
	//menu sound effects are very grating
	if(_1>=1008) _2-=1000;
	
	EX::INI::Sample se;
	if(&se->sample_volume_adjustment)
	{
		//TODO? can this be logarithmic?
		_2 = se->sample_volume_adjustment(som_SFX_SND_to_sound(_1),_2/100.0f)*100;
		_2 = min(0,_2);
	}
	_2+=SOM::millibels[1];

	if(_2>-10000)
	((void(__cdecl*)(DWORD,INT32,DWORD,DWORD))0x44b6f0)(_1,_2,_3,_4);
}
static void __cdecl som_game_43f540_fullscreen_snd(int snd, int pitch)
{
	bool heal = snd==890, wall = snd==891; //EXTENSION?

	assert(heal||wall);

	snd = SOM::SND((WORD)snd); //healing and wall magic

	((void(__cdecl*)(DWORD,INT32))0x43f540)(snd,pitch);
}
extern DWORD som_game_44c4b0_fade = 0;
static void som_game_44c270() //close BGM
{
	//this is in case events takeover BGM
	som_game_44c4b0_fade = 0;
	((void(__cdecl*)())0x44c270)();
}
static DWORD som_game_44c4b0(DWORD) //BGM
{
	int vol = SOM::millibels[0];

	if(DWORD&fade=som_game_44c4b0_fade)
	{
		//I think this goes quiet well before
		//-10000, but that's theoretical mute
		//so I don't know what else to do
		DWORD rr = 3*DDRAW::refreshrate;

		if(rr<=++fade) 
		{
			//NOTE: the string is overwritten by
			//the BGM close routine
			char swap[32];
			memcpy(swap,SOM::L.bgm_file_name,32);
			SOM::bgm(swap,1);

			assert(!fade); fade = 0; //PARANOID
		}
		else 
		{
			float mute = fade/(float)rr;

			//experimenting
			//mute = sqrtf(mute); //not good
			//mute = powf(mute,2); //alright //2
			mute = 1-powf(1-mute,2); //better? //1.5f

			if(1) //sounds off? (fade/rr) (pow 2 is better)
			{
				float v = SOM::bgmVol/255.0f;

				v*=1-mute;		

				//NOTE: SOM::config_bels has the original code for this
				//
				// to smoothly fade requires logarithm logic... I think
				// it might be an improvement if se_volume did that too
				//
				vol = v?(int)(logf(v)/logf(2)*1000):-10000;
			}
			else
			{
				//this isn't bad but it takes a long time to make it to
				//-10000 and I couldn't find a power function I liked to
				//speed out of the quiet time

				vol+=(int)(mute*(-10000-vol));
			}
		}
	}

	//just calls IDirectSoundBuffer::SetVolume
	return ((DWORD(__cdecl*)(int))0x44c4b0)(vol);
}

static HWND WINAPI som_game_CreateWindowExW(DWORD A,LPCWSTR B,LPCWSTR C,DWORD D,int E,int F,int G,int H,HWND I,HMENU J,HINSTANCE K,LPVOID L)
{
	if(!I&&GetCurrentThreadId()!=EX::threadmain)
	{
		if((DWORD)C>65535&&!wcscmp(C,L"Watch")
		 &&(DWORD)B>65535&&!wcscmp(B,L"wxWindowNR")) //Exselector.cpp?
		{
			I = EX::display();

			//WS_EX_TOOLWINDOW removes cpanel style icon (Exselector?)
			//WS_EX_TOPMOST is required for fullscreen, otherwise it
			//closes the window, and will freeze up during startup 

			A|=WS_EX_TOPMOST; //WS_EX_TOOLWINDOW|WS_EX_TOPMOST

			HINSTANCE h = GetModuleHandleA("Exselector.dll");			
			HICON icon = LoadIconA(h,"IDI_ICON");
			
			HWND hw = SOM::Game.CreateWindowExW(A,B,C,D,E,F,G,H,I,J,K,L);

			SendMessage(hw,WM_SETICON,ICON_SMALL,(LPARAM)icon);
			SendMessage(hw,WM_SETICON,ICON_BIG,(LPARAM)icon);

			return hw;
		}
	}

	return SOM::Game.CreateWindowExW(A,B,C,D,E,F,G,H,I,J,K,L);
}

//// Removing WS_EX_TOPMOST /////////////
static HWND WINAPI som_game_CreateWindowExA(DWORD A,LPCSTR B,LPCSTR C,DWORD D,int E,int F,int G,int H,HWND I,HMENU J,HINSTANCE K,LPVOID L)
{
	SOM_GAME_DETOUR_THREADMAIN(CreateWindowExA,A,B,C,D,E,F,G,H,I,J,K,L)

	EXLOG_LEVEL(7) << "som_game_CreateWindowExA()\n";

	//2017: Thought this might be the cause of a problem that begins in 
	//fullscreen mode after the Windows 10 Anniversary or Creators Update
	//one. There doesn't seem to be a way to remove this flag, even with the
	//SetWindowPos API. But it's not the problem after all.
	A&=~WS_EX_TOPMOST; //assert(0==(WS_EX_TOPMOST&A));
	
	//2021: testing WS_EX_NOREDIRECTIONBITMAP 
	//https://docs.microsoft.com/en-us/archive/msdn-magazine/2014/june/windows-with-c-high-performance-window-layering-using-the-windows-composition-engine
	A|=0x00200000L;

	HWND out = SOM::Game.CreateWindowExA(A,B,C,D,E,F,G,H,I,J,K,L);

	//For Alt+Alt+M. Reminder: The game's mouse inputs use DirectInput.
	SetClassLong(out,GCL_STYLE,CS_DBLCLKS|GetClassLong(out,GCL_STYLE));

	return out;
}

const char *SOM::Game::image(int ext)
{	
	static char bin[MAX_PATH] = "";
	static char ini[MAX_PATH+4] = "";

	if(ext=='.ini')
	{	
		if(!*ini)	
		{	
			//seems like the best we can do
			SetCurrentDirectoryW(project());
			int sep = GetCurrentDirectoryA(MAX_PATH,ini);
			strcpy(ini+sep,PathFindFileNameA(image())-1);
			strcpy(PathFindExtensionA(ini+sep+1),".ini");
		}
		return ini;
	}
	
	if(!*bin)
	{
		const char *cli = GetCommandLineA();

		bool quote = *cli=='"'; if(quote) cli++;

		const char *sep = strchr(cli,quote?'"':' ');

		if(sep) strncpy_s(bin,cli,sep-cli);
		if(!sep) strcpy_s(bin,cli);

		bin[MAX_PATH-1] = '\0';
	}
	return bin;
}

const wchar_t *SOM::Game::title(int ext)
{
	if(ext=='.ini')
	{	
		static wchar_t out[MAX_PATH] = L""; 
		
		if(*out) return out;

		const wchar_t *ini = Sompaste->get(L"INI");

		if(PathIsRelativeW(ini))
		{		
			swprintf_s(out,L"%ls\\%ls",EX::user(1),ini);
		}
		else wcscpy_s(out,ini);
		
		EXLOG_HELLO(1) << L"Configuration file is " << Ex%ini << '\n';
		
		return out;
	}
	else assert(!ext);

	EX::INI::Launch go;
	
	if(!*go->launch_title_to_use) return Sompaste->get(L"TITLE");

	return go->launch_title_to_use;
}




//2017 //MENUS //2017 //MENUS //2017 //MENUS //2017 //MENUS //2017 //MENUS 
//2017 //MENUS //2017 //MENUS //2017 //MENUS //2017 //MENUS //2017 //MENUS 
//2017 //MENUS //2017 //MENUS //2017 //MENUS //2017 //MENUS //2017 //MENUS 
//2017 //MENUS //2017 //MENUS //2017 //MENUS //2017 //MENUS //2017 //MENUS 
//2017 //MENUS //2017 //MENUS //2017 //MENUS //2017 //MENUS //2017 //MENUS 





static SOM::Shop *som_game_empty_shop = 0;

namespace som_game_menu //2017: menu memory
{	
	//all unused slots are factored out of N
	//enum{ N=250+32*EX::debug };
	static int N = 250;

	static BYTE itemenu = 255;
	static WORD itemenuN[2]={};
	static WORD items[15]={};
	static WORD itemsel[15]={};
	static BYTE itemsel2[15]={};
	static BYTE itemsel3 = 0;

	static BYTE nonitemenu = 255;
	static BYTE nonitemenusel[7] = {};
	static BYTE nonitemenusel2[3] = {};
	
	static BYTE pcequip[8];	
	static void swap_equip() 
	{		
		//the goal of this is to use the real item numbers 
		//for the tinted menu model	
		std::swap(*(long long*)&pcequip,*(long long*)&SOM::L.pcequip);
	}	
	template<int how> void _sort(int kind);
	static WORD *ini_browse = 0;
	static BYTE *prm_sorted = (BYTE*)0x566ff0;
	static WORD *inv_sorted = (WORD*)0x19C1A18;	
	static BYTE *m32_sorted = (BYTE*)0x1D12D22;
	static DWORD pcm_masked = 0;
	static DWORD pcm_sorted = 0;
	static void _prm_sort(int kind)
	{
		_sort<1>(kind);
	}
	static void _inv_sort(int kind)
	{
		_sort<2>(kind);
	}
	template<int how> void _sort(int kind)
	{
		enum{ store=how&2, param=how&1 };

		SOM::Shop *unused_shop = 0;
		bool include_inventory = N<=250||kind==8;		

		if(!ini_browse)
		{
			if(store) pcm_sorted = SOM::L.pcmagic; 
			return;
		}
		else if(store) pcm_sorted = 0;
		
		BYTE *prm = prm_sorted;
		WORD *inv = inv_sorted;		
		
		for(int i=0,m=-1;i<N;i++)
		{
			int j = ini_browse[i]; if(j>=250)
			{
				//SAME AS som_game_5_menu41cbd0 (DUPLICATION)

				j-=250; m++; 

				BYTE m32 = SOM::L.magic32[j];
				
				if(store)
				{
					m32_sorted[m] = m32;
				}
				if(param) if(include_inventory) 
				{
					*(WORD*)prm = 250;
					memcpy(prm+2,SOM::L.magic_prm_file[m32].c+2,8); //sizeof("@-249");						
					prm[33] = '\0'; //keep out of appraisers menu
					prm+=336; 
				}				

				if(store)
				{
					j = 1<<j&SOM::L.pcmagic;
			
					if(include_inventory)
					{
						*inv = j&pcm_masked?0x100:j?0x180:0; //and 0x80? 
					}
					//2020: Moratheia had N==254 so this happened to work
					//for it (sorry, can't remember what N<=250 is about)
					//I think this is indequate because 250???
					//else //2019: did I delete by accident???
					{
						if(j&pcm_masked) j = 0;
					}

					if(j) pcm_sorted|=1<<m; 
				}
				if(include_inventory) inv++;
			}
			else 
			{	
				if(store) *inv = SOM::L.pcstore[j]; inv++; //!!
				if(param)  
				{
					memcpy(prm,SOM::L.item_prm_file->c+j*336,336);
					prm+=336;
				}
			}		
		}	

		int rem = inv_sorted+N-inv; if(store) 
		{
			memset(inv,0x00,rem*2);
			//HACK: main-menu won't open the inventory-menu
			//if there is not an item in the inventory
			if(!include_inventory&&rem!=0&&0!=pcm_sorted)
			*inv = 0x100;
		}
		if(param) for(;rem-->0;prm+=336) *(WORD*)prm = 0xFFFF;
	}
	static BYTE find_item(int item)
	{
		if(!ini_browse||item>=250) return 0xFF&item;

		if(N>250) //Filter magic?
		{
			assert(10!=itemenu);

			for(int i=0,m=0;i<N;i++) 
			{
				int j = ini_browse[i]; 

				if(item==j) return 0xFF&(i-m); if(j>=250) m++;
			}
		}
		else item = std::find(ini_browse,ini_browse+N,item)-ini_browse;
		return 0xFF&item;
	}
	static BYTE find_magic(BYTE magic, BYTE *m32=m32_sorted)
	{
		if(magic<32)
		{
			magic = (m32==m32_sorted?SOM::L.magic32:m32_sorted)[magic];
			for(int i=0;i<32;i++) if(m32[i]==magic) 
			return 0xFF&i;
		}
		return magic;
	}	
	
	static bool _appraiser_filter(int i)
	{
		if(inv_sorted[i]>>8)				
		if('\0'!=prm_sorted[i*336+33])
		{
			//REMINDER: there are plans to make this 
			//considerably more complex. (by looking
			//into the shops' quantities
			return true;
		}
		return false;
	}	
	static bool _shop_item_is_(int kind, int i)
	{
		SOM::Shop *s = SOM::L.shops+nonitemenusel2[2];

		switch(kind) //CODE? NOT 100% SURE ABOUT THIS.
		{
		case 10: return 0!=s->stock[i];
		case 12: return _appraiser_filter(i); 
		}
		assert(kind==11); //SELL LOGIC IS COMPLICATED.
		if(0==s->price2[i])
		return false;
		int n = inv_sorted[i]>>8; if(n==1)
		{
			//equipped items are excluded from totals
			for(int j=0;j<7;j++)
			if((0xFF&i)==SOM::L.pcequip[j])
			return false;
		}
		return n!=0;
	}		
	static bool _some_item_was_(bool(_is_)(int,int), int kind, WORD &item)
	{
		//This is based on _inventory_item_was_.
		int i = (int)item;
		if(_is_(kind,i)) 
		return false;
		int n = min(N,250); assert(i<n);
		while(++i<n&&!_is_(kind,i)) 
		;
		itemenuN[0]--; 
		item = 0xFFFF&i; return true; //i<n;
	}
	static bool _shop_item_was_sold_out_(int kind, WORD &item)
	{
		if(!_some_item_was_(_shop_item_is_,kind,item))
		return false;
		//HACK/CLASSIC-BUG: the old item's graphic remains
		//for this to work the following code must be knocked out
		//don't know if it's safe to do so or not???
		//0043E378 76 3E                jbe         0043E3B8
		*(BYTE*)0x1D11E21 = 0;		
		return true;
	}	
	static bool _inventory_item_is_(int kind, int i)
	{
		int inv = inv_sorted[i]; switch(inv>>8)
		{
		case 0: case 0xFF: return false; //Why FF?!? 
		}
		return (kind==9)==(0==(0x80&inv));
	}
	static bool _inventory_item_was_(int kind, WORD &item)
	{
		int i = (int)item;
		while(i<N&&!_inventory_item_is_(kind,i)) 
		i++;
		if(item==i) return false;
		
		//DELICATE MACHINERY
		//Detect/handle inventory's swap?		
		if(!_inventory_item_is_(kind,item)
		&&_inventory_item_is_(kind^1,item))
		{
			itemenuN[kind&1]--; 
			if(itemenuN[~kind&1]++==0) 								
			items[2+(kind^1)] = item;
		}		

		item = 0xFFFF&i; return true; //i!=N;
	}
	static bool _nonmagical_item_is_(int kind, int i)
	{
		/*419020 just does process of elimination...
		//EBP is pcstore+1
		004190ED BD 19 1A 9C 01       mov         ebp,19C1A19h  
		//EDI is item_prm_file
		004190F2 BF F0 6F 56 00       mov         edi,566FF0h  
		004190F7 66 8B 07             mov         ax,word ptr [edi]  
		004190FA 66 3D FF FF          cmp         ax,0FFFFh  
		004190FE 0F 84 8D 01 00 00    je          00419291  
		00419104 80 7D 00 00          cmp         byte ptr [ebp],0  
		00419108 0F 84 83 01 00 00    je          00419291  
		0041910E F6 45 FF 80          test        byte ptr [ebp-1],80h  
		00419112 0F 84 79 01 00 00    je          00419291  
		00419118 25 FF FF 00 00       and         eax,0FFFFh  
		0041911D 8D 14 80             lea         edx,[eax+eax*4]  
		00419120 8D 04 50             lea         eax,[eax+edx*2]  
		//58047Eh is item_pr2_file+66
		00419123 8A 14 C5 7E 04 58 00 mov         dl,byte ptr [eax*8+58047Eh]  
		0041912A 84 D2                test        dl,dl  
		0041912C 0F 85 5F 01 00 00    jne         00419291  
		00419132 8A 87 29 01 00 00    mov         al,byte ptr [edi+129h]  
		00419138 84 C0                test        al,al  
		0041913A 8D 5F 02             lea         ebx,[edi+2]  
		0041913D 0F 85 4E 01 00 00    jne         00419291  
		00419143 8B 44 24 1C          mov         eax,dword ptr [esp+1Ch]  
		00419147 3B C1                cmp         eax,ecx  
		00419149 0F 8C 3D 01 00 00    jl          0041928C  
		0041914F 8D 51 04             lea         edx,[ecx+4]  
		00419152 3B C2                cmp         eax,edx  
		00419154 0F 8D 32 01 00 00    jge         0041928C  
		0041915A 83 44 24 18 14       add         dword ptr [esp+18h],14h  
		*/
		WORD inv = inv_sorted[i];
		if(0==(0x80&inv)||0==inv>>8) //||0xFF==inv>>8)
		return false;
		BYTE *prm = prm_sorted+i*336;
		WORD &pr2 = *(WORD*)prm; 
		if(0xFFFF==pr2)
		return false;
		assert(pr2<=250);
		BYTE a = kind>1?2:kind;
		if(a!=SOM::L.item_pr2_file[pr2].c[62])
		return false;		
		if(a==0) //Use item?
		{
			if(0!=prm[0x129])
			return false; //Usable?
		}
		if(a==2) //Armor?
		{
			//4 is body suit stuff. 
			BYTE b = kind-2; if(b>=4) b+=1;
			if(b!=SOM::L.item_pr2_file[pr2].c[72])			
			if(b!=1||4!=SOM::L.item_pr2_file[pr2].c[72])
			return false;
		}
		return true;
	}
	static bool _map_item_was_used_up_(WORD &item)
	{
		return _some_item_was_(_nonmagical_item_is_,0,item);
	}
	static bool _magical_item_is_(int kind, int i)
	{
		/*
		//1D12D22 is magic32
		0041BCC3 33 C0                xor         eax,eax  
		0041BCC5 8A 85 22 2D D1 01    mov         al,byte ptr [ebp+1D12D22h]  
		0041BCCB 8D 3C 80             lea         edi,[eax+eax*4]  
		0041BCCE C1 E7 06             shl         edi,6  
		//19AB950 is magic_prm_file
		0041BCD1 81 C7 50 B9 9A 01    add         edi,19AB950h  
		0041BCD7 33 C0                xor         eax,eax  
		0041BCD9 66 8B 07             mov         ax,word ptr [edi]  
		0041BCDC 66 3D FF FF          cmp         ax,0FFFFh  
		0041BCE0 0F 84 F1 00 00 00    je          0041BDD7  
		//19C1C40 is pcmagic
		0041BCE6 85 1D 40 1C 9C 01    test        dword ptr ds:[19C1C40h],ebx  
		0041BCEC 0F 84 E5 00 00 00    je          0041BDD7  
		0041BCF2 25 FF FF 00 00       and         eax,0FFFFh  
		0041BCF7 8D 04 80             lea         eax,[eax+eax*4]  
		//19BF1EF is magic_pr2_file+35
		0041BCFA 80 3C C5 EF F1 9B 01 00 cmp         byte ptr [eax*8+19BF1EFh],0  
		0041BD02 0F 85 CF 00 00 00    jne         0041BDD7  
		*/
		BYTE *prm = SOM::L.magic_prm_file[m32_sorted[i]].uc;
		WORD &pr2 = *(WORD*)prm;
		if(0xFFFFF==pr2) 
		return false;
		if(0==(1<<i&pcm_sorted))
		return false;
		if((kind==-2)!=(0==SOM::L.magic_pr2_file[pr2].c[31]))
		return false;
		return true;
	}	
		
	static void _scroll_removal(int &hl_0, int &hl_1, int lim=0)
	{	
		//SOM's menu's handle item removal by scrolling the items
		//above the removal; which is just weird.
		if(hl_1) if(hl_0+hl_1<itemenuN[lim])
		hl_1++; 
		else if(hl_0){ hl_0--; hl_1++; }
	}

	static void _item_visual(int itemenu)
	{
		//this needs to trigger a call to 004230C0
		//in the right context to update the item
		//visual
		/*for example (ITEM MENU)
		0041918D A0 E9 B1 9A 01       mov         al,byte ptr ds:[019AB1E9h]  
		00419192 84 C0                test        al,al  
		00419194 75 51                jne         004191E7  
		00419196 8B 54 24 20          mov         edx,dword ptr [esp+20h]  
		0041919A A1 38 DB 45 00       mov         eax,dword ptr ds:[0045DB38h]  
		0041919F 81 C2 00 FF FF FF    add         edx,0FFFFFF00h  
		004191A5 3B D0                cmp         edx,eax  
		004191A7 76 3E                jbe         004191E7
		*/
		DWORD viz2,viz = 0; switch(itemenu) //MEMORY LEAK?
		{
		case 2: viz = 0x45DB38; viz2 = 0x019AB1E9; break; //USE

		default: if(itemenu<3||itemenu>9) break; //EQUIP 
			
			//where was this from???
			//viz = 0x45DB3C; viz2 = 0x19AB211; break;
			viz = 0x45DB40; viz2 = 0x19AB231; break;

		//here viz2 seems to be part of "SOM::L.menupcs"
		//as does the second argument to 004230C0
		case 12: viz = 0x45F2B8; viz2 = 0x1D11E01; break; //BUY 
		case 13: viz = 0x45F2C4; viz2 = 0x1D11E21; break; //SELL
		case 14: viz = 0x45F2C8; viz2 = 0x1D11E41; break; //SHOW
		}
		if(viz)
		{
			//*(BYTE*)0x019AB1E9 = 0; //*(BYTE*)0x45DB38 = 0;
			*(BYTE*)viz2 = 0; *(DWORD*)viz = 0; //0xFFFFFFFF; 				
		}
	}

	static int *_highlight_item(int &most, int kind)
	{
		most = kind==8||kind==9?15:3;
		switch(kind)
		{
		case -2: return (INT*)0x19AB23C; //equip magic
		case -1: return (INT*)0x19AB1F4; //work magic
		case 0: return (INT*)0x19AB1D8; //use item
		default: return (INT*)0x19AB21C; //equip item
		case 8: return (INT*)0x19AB24C; //left inventory
		case 9: return (INT*)0x19AB254; //right inventory
		case 10: return (INT*)0x1D11DF4; //buy
		case 11: return (INT*)0x1D11E10; //sell
		case 12: return (INT*)0x1D11E30; //show
		}
	}

	static BYTE norepeat_updown = 0;
	extern bool translate_dik(BYTE x)
	{
		if(itemenu>14) return false;	

		int hl_most,*hl =
		_highlight_item(hl_most,itemenu-2);
		if(itemenu==10)
		{
			std::swap(hl[1],hl[2]);
			if(itemsel3) hl+=2;
		}

		//EX::dbgmsg("controls-4 %x",*(DWORD*)(SOM::L.controls-4));
		
		int sel = hl[0]+hl[1];
		int n = itemenuN[itemenu<10?0:itemsel3];
		BYTE _c_4 = ~SOM::L.controls[-4];
		if(n>1) //NEW: keep looping on 1 item from resetting model?
		switch(x)
		{
		case 0xC9: //PRIOR
					
			//Reminder: for some reason PageUP is treated
			//as if having pressed Up???
			break; 

		case 0x4C: case 0xC8: //UP

			if(norepeat_updown) goto repeating;

			if(0x10&_c_4) 
			{
				//UNRELATED: trying to make keyboard responsive :(
				//EX::Syspad.send(x,4);
				
				if(0==sel)
				{	
					hl[0] = min(n-1,hl_most);
					hl[1] = n-hl[0]-1;	
					goto norepeat;
				}
			}
			x = 0; break;

		case 0x50: case 0xD0: //DOWN

			if(norepeat_updown) repeating:
			{
				//if(0x30&SOM::L.controls[-4])
				{
					SOM::L.controls[-4]|=0x30;

					norepeat_updown--; break;
				}
				//else x = norepeat_updown = 0;
			}
			else if(0x20&_c_4) 
			{					
				//UNRELATED: Trying to make keyboard responsive :(
				//EX::Syspad.send(x,4);

				if(sel==n-1)
				{
					hl[0] = hl[1] = 0;												
					norepeat: norepeat_updown = 5;
					SOM::L.controls[-4]|=0x30;
					SOM::menuse(3);
					_item_visual(itemenu);
					break;
				}				
			}
			x = 0; break;

		default: x = 0; break;
		}		
		
		if(itemenu==10)
		{
			if(itemsel3) hl-=2;
			std::swap(hl[1],hl[2]);			
		}
		return x?true:false;
	}
		
	static void recall_item(int kind)
	{	 
		const int im = kind+2;
		const int i_shop = kind<10?N:min(N,250);
		const int i_most = kind<0?31:i_shop-1;
		bool(*item_is_)(int,int) = _shop_item_is_;
		if(im<12) item_is_ = _inventory_item_is_;
		if(im<10) item_is_ = _nonmagical_item_is_;
		if(im<2) item_is_ = _magical_item_is_;

		int hl_most,*hl =
		_highlight_item(hl_most,kind);		
		int sel = hl[0]+hl[1];		
		if(sel<=i_most)
		if(itemenu!=im)
		{
			//HACK: Entering/leaving inventory?
			if(N>250&&(kind==8||itemenu==10))
			_sort<1|2>(kind);

			if(kind==8) 
			{
				recall_item(9);
				*(BYTE*)0x45DB44 = itemsel3^1;
			}

			itemenu = im; assert(sel==0);

			int i=-1,j=-1,is=-1;
			const int k = items[im];
			while((i<k||j==-1)&&i<i_most)			
			if(item_is_(kind,++i))
			{
				//is is in case i overflows
				j++; is = i;
			}
			int n = j+1;
			if(j==-1) j = 0;
			if(is!=-1) i = is;
			items[im] = 0xFFFF&i;
			itemsel[im] = 0xFFFF&j;
			if(i!=k) 
			itemsel2[im] = 1;			
		
			while(i<i_most)
			if(item_is_(kind,++i))
			n++;
			//2020: this fixes a problem when taking an
			//item increases the count to most+1, could
			//be a big mistake, not sure
			//while(j>0&&n-j<hl_most)
			while(j>0&&n-j<=hl_most)
			{
				j--; hl[0]+=1;
			}
			while(j>0&&hl[0]<itemsel2[im]) 
			{
				j--; hl[0]+=1; 
			}
			hl[1] = j;			
			itemenuN[kind==9] = 0xFFFF&n;
		}
		else
		{	
			if(kind==8)
			{
				//did items change sides?
				for(int i=8,j=0;i<=9;i++,j++)
				if(_inventory_item_was_(i,items[i+2]))				
				_scroll_removal(hl[j],hl[j+2],j);

				std::swap(hl[1],hl[2]); 				
				itemenu++; recall_item(9);
				itemenu--;
				sel = hl[0]+hl[1];
				itemsel3 = *(BYTE*)0x45DB44^1;
			}	
			else if(kind==10||kind==11)
			{
				if(_shop_item_was_sold_out_(kind,items[im]))
				_scroll_removal(hl[0],hl[1]);
			}
			
			int i = items[im];
			int j = itemsel[im];
			while(sel<j&&i>0)
			if(item_is_(kind,--i))
			j--;
			while(sel>j&&i<i_most)
			if(item_is_(kind,++i)) 
			j++;
			items[im] = 0xFFFF&i;
			itemsel[im] = 0xFFFF&j;
			itemsel2[im] = 0xFF&hl[0];
		}

		if(kind==8) std::swap(hl[1],hl[2]);

		nonitemenu = 255;
	}
	
	static void recall(int kind)
	{	
		//HACK: Leaving inventory?
		if(10==itemenu&&N>250)
		_sort<1|2>(0);

		itemenu = 255;
					 
		int *hl; switch(kind)
		{
		case -1: goto _1;
		case 0: hl = (INT*)0x19AB204; break; //equip
		case 1: hl = (INT*)0x19AB2F4; break; //system
		case 2: hl = (INT*)0x19AB304; break; //load
		case 3: hl = (INT*)0x19AB310; break; //save
		case 4: hl = (INT*)0x19AB2FC; break; //options
		case 5: hl = (INT*)0x19AB31C; break; //controls
		case 6: hl = (INT*)0x1D11DEC; break; //shop
		}

		if(nonitemenu!=kind)
		{		
			hl[0] = nonitemenusel[kind];
			if(kind==2||kind==3) //save/load?
			hl[1] = nonitemenusel2[kind%2];

		_1: nonitemenu = kind;
		}
		else
		{
			nonitemenusel[kind] = hl[0];
			if(kind==2||kind==3) //save/load?
			nonitemenusel2[kind%2] = hl[1];
		}
	}
	static void recall_shop(BYTE shop)
	{
		if(shop!=nonitemenusel2[2])
		{
			nonitemenusel2[2] = shop;
			for(int i=10;i<=12;i++)
			items[i] = itemsel[i] = itemsel2[i] = 0;
		}
		recall(6);
	}

	static void browsing_items_menu()
	{
		if(som_game_empty_shop)
		pcm_masked = *(DWORD*)som_game_empty_shop->stock;
		_inv_sort(0); 

		memcpy(pcequip,SOM::L.pcequip,8); 		
		for(int i=0;i<7;i++)
		SOM::L.pcequip[i] = find_item(pcequip[i]);	
		SOM::L.pcequip[7] = find_magic(pcequip[7]);	
		swap_equip();		
	}
}

extern char SOM::move = 0; //2024
extern DWORD (*SOM::movesnds)[4] = {}; //2023
extern const SOM::Struct<22>*(*SOM::movesets)[4] = 0; //2021

extern const SOM::Struct<22> *SOM::shield_or_glove(int e)
{
	assert(SOM::movesets);
	auto **ms = SOM::movesets[e];
	return !e||ms[2]?ms[2]:ms[0];
}
extern DWORD SOM::shield_or_glove_sound_and_pitch(int e)
{
	DWORD *ms = SOM::movesnds[e];
	return !e||ms[2]?ms[2]:ms[0];
}
extern BYTE *SOM::move_damage_ratings(int mv) //2024
{
	int pce = SOM::L.pcequip[0];
	BYTE *dmg = SOM::L.item_prm_file[pce].uc+300;
	if(mv) //SOM_PRM_extend_items
	{
		struct equip //36B
		{		
			//+300
			BYTE hp[8],fx,fx2,sm;
			//EXTENDED
			BYTE assist[5];
			BYTE attack2[4];
			union
			{
				BYTE attack34[2][4];
				BYTE attack1[8];
			};
			BYTE shield[8]; //328		
		};
		equip *eq = (equip*)dmg;

		switch(mv)
		{
		case 1: dmg = eq->attack2; break;
		case 2: dmg = eq->attack34[0]; break;
		case 3: dmg = eq->attack34[1]; break;
		default: assert(0); return nullptr;
		}
	}
	return dmg[0]||dmg[1]||dmg[2]?dmg:nullptr;
}

namespace som_MPX_swap
{
	extern void models_free();
	extern int &models_refs(void*);
}
extern float som_game_measure_weapon(char *model)
{
	if(!*model) return 0;

	char a[64];
	memcpy(a,"data\\item\\model\\",17); 
	memcpy(strrchr(strcat(a,model)+16,'.'),"_0.mdo",7);

	float len = 0;
	extern void *som_MDL_401300_maybe_mdo(char*,int*);
	if(auto*o=(SOM::MDO::data*)som_MDL_401300_maybe_mdo(a,0))
	{
		som_MPX_swap::models_refs(o)--;

		auto &cps = o->cpoints_ptr;

		float _[3] = {};

		if(memcmp(_,cps[1],sizeof(_))/*&&memcmp(_,cps[0],sizeof(_))*/)
		{
			//len = cps[1][2]-cps[0][2];
			len = Somvector::measure<3>(cps[0],cps[1]);
		}
		else for(int i=o->chunk_count;i-->0;)
		{
			int c = o->chunks_ptr[i].vertcount;
			auto *p = o->chunks_ptr[i].vb_ptr;

			while(c-->0) len = max(len,-p[c].pos[2]);
		}
	}	
	return len;
}

extern int *som_game_nothing()
{
	//NOTE: 427868 zeroes power/magic guages when
	//there's no weapon equipped

	static int out[8] = {-1};
	
	if(out[0]!=-1) return out; 

	for(int i=0;i<8;i++) out[i] = 0xFF;

	BYTE *prm = (BYTE*)SOM::L.item_prm_file->c;

	//2021: there's extra room in item_pr2_file
	//this has the added benefit of easily detecting
	//"Nothing" equipment
	//DWORD *pr2 = (DWORD*)0x580440-2;	

	int priority[5] = {4,2,3,0,1};
	for(int i=0,k=0;i<250&&k<5;i++)		
	if(*(WORD*)(prm+i*336)==0xFFFF)
	{
		assert(!SOM::L.item_MDO_files[i]);

		int j = priority[k++];

		/*2021
		if(*pr2>=250) break; //out of room												
		//reminder +72 is armor type
		*(BYTE*)(0x580440+*pr2*88+62) = ++j; //1/2	 
		*(WORD*)(prm+i*336) = (*pr2)++;*/
		//NOTE: #250 was already used years ago
		//for adding magic to the inventory menu
		auto &prf = SOM::L.item_pr2_file[251+j];
		prf.uc[62] = j==4?1:2;
		prf.uc[72] = j==4?2:j==2?5:j;
		SOM::L.item_prm_file[i].s[0] = 251+j;

		char *title = (char*)(prm+i*336)+2;

		memset(title,0x00,336-2); 
		//strcpy(title,som_932_Nothing);
		title[0] = '@'; itoa(i,title+1,10); //2017

		//this works out well... the shield doesn't
		//need its own slot because it will default
		//to a glove, as will weapons, but in their
		//case the item type is 1 instead of 2
		switch(j) 
		{
		case 0: out[1] = i; break; //head
		case 1: out[2] = i; //body
				out[6] = i; break; //accessory
		case 2: out[3] = i; //arms
				out[5] = i; break; //shield
		case 3: out[4] = i; break; //legs
		case 4: out[0] = i; break; //weapon
		}
		//HACK: this is how movesets are detected. it
		//just has to have a nozero pitch... 127 is
		//outside the pitch range and so will default
		//to the moveset's
		for(int k=4;k-->0;)
		{
			//this sets the SND and pitch to defer to 
			//the jth moveset's
			if(j!=4) //HACK: -1cm weapon length?
			prf.s[38+k] = -1; prf.c[84+k] = 127;

			//som_game_moveset needs this to default
			//to the equipment types
			prf.c[k==0?63:72+k] = 255;
		}
	}
	//2021: in case there aren't enough slots use
	//any armor slot (som_game_moveset may need to 
	//consider this possibility)
	for(int i=1;i<7;i++) if(out[i]==255)	
	for(int k=1;k<5;k++) 
	{
		//+1 just happened to line up
		int j = priority[k]+1;
		if(out[j]!=255) for(;i<7;i++) 
		{
			if(out[i]==255) out[i] = out[j];
		}
	}

	int magic = 0xFF; //magic (profile)
	{
		WORD pr2 = 0; //no record count this time

		for(int i=0;i<250;i++)
		{
			WORD profile = *SOM::L.magic_prm_file[i].us;

			if(profile==0xFFFF) //again, no record count
			{
				if(magic==0xFF) magic = i;
			}
			else if(profile>pr2) pr2 = profile;
		}
		if(magic!=0xFF&&pr2<249) //paranoia
		{
			WORD *record = SOM::L.magic_prm_file[magic].us;
			
			memset(record,0x00,320); *record = pr2+1; 
			//strcpy((char*)record+2,som_932_Nothing);
			record[1] = '@'; itoa(-magic,(char*)record+3,10); //2017  
		}		
		else magic = 0xFF; //pr2<249
	}
	if(magic!=0xFF)
	for(int i=0;i<32;i++) if(0xFF==SOM::L.magic32[i]) 
	{	
		SOM::L.magic32[i] = magic;
		SOM::L.magic32[i+32] = 0; //event based acquisition
		out[7] = i; break;
	}
	return out;
}
static int som_game_weapon_to_glove(int e)
{
	//NOTE: this is its own subroutine just so the
	//equip menu is updated without having to exit
	//out and enter again

	int *nothing = som_game_nothing();
	int pce2 = nothing[0], pce = pce2;
	BYTE *dmg = SOM::L.item_prm_file[pce2].uc+300;
	//int e = SOM::L.pcequip[3];
	if(e<250) e = SOM::L.item_prm_file[e].us[0];
	if(e<250)
	{
		pce = SOM::L.pcequip[3];
		auto &prm = SOM::L.item_prm_file[pce];
		auto &pr2 = SOM::L.item_pr2_file[prm.s[0]];
		memcpy(dmg,SOM::L.item_prm_file[pce].uc+320,8);
	}
	else memset(dmg,0x00,8);
	int fi = som_game_menu::find_item(pce2);	
	memcpy(som_game_menu::prm_sorted+fi*336+300,dmg,8);
	return pce;	
}
#pragma optimize("",off)
extern void som_game_moveset(int i, int j)
{	
	auto *item_arm = SOM::PARAM::Item.arm->records; 

	if(!item_arm) return;

	int pce = SOM::L.pcequip[i];

	if(pce==255){ assert(0); return; }

	//weapon->glove?
	int *nothing = som_game_nothing();
	if(i==0&&pce==nothing[0])
	pce = som_game_weapon_to_glove(SOM::L.pcequip[3]);

	auto &prm = SOM::L.item_prm_file[pce];
	auto &pr2 = SOM::L.item_pr2_file[prm.s[0]];

	if(!pr2.c[84]) return; //moveset?

  //WARNING: SOMETHING IS WRONG HERE WHEN
  //ENABLING OPTIMIZATION (MSVC2010-2019)
	//#pragma optimize("",off)
	if(prm.s[0]>=250) //som_game_nothing?
	for(int ii=0;ii<7;ii++) if(nothing[ii]==pce)
	{
		//indicates there wasn't enough unused memory
		//to inject a Nothing equipment for this type
		if(ii!=i) //return;
		{
			//shield/accessory share with arm/body
			if(i==3&&ii==5||i==5&&ii==3) continue;
			if(i==2&&ii==6||i==6&&ii==2) continue;

			return; 
		}
	}
	//#pragma optimize("",on)

	//HACK: just avoiding #include swordofmoonlight.h
	auto *prf = item_arm; (void*&)prf = &pr2;

	//2024: this is code for switching the animation
	//depending on what inputs you want to work with
	enum
	{
		random=1 //this just means using r...
	}; 
	int r = j==2?2:0;
	int x = pr2.uc[14+1];
	int z = pr2.uc[14+3];
	if(j!=2&&z&&z!=255&&x&&x!=255) //unlock it?
	{
		float dx = fabsf(SOM::motions.dx);
		float dz = fabsf(SOM::motions.dz);
		
		if(dx>0.5f)	
		{
			if(dz<0.3f) r = 1; //slice
		}
		else if(SOM::motions.dz>0.5f) //dz
		{
			if(dx<0.3f) r = 3; //thrust
		}

		auto rt = SOM::move_damage_ratings(r);
		if(!rt) rt = SOM::move_damage_ratings(r=0);
		if(!rt) rt = SOM::move_damage_ratings(r=1);
		if(!rt) rt = SOM::move_damage_ratings(r=3);
	}

	//HACK: for compatibility with older code weapons
	//overwrite the PR2 table
	int mv; if(i==0&&pce==SOM::L.pcequip[0])
	{
	//	if(0&&random) //fully random (testing)
		{
	//		r = SOM::frame%4; if(r==2) r = 0;			
		}

		int jj = random?r:j;

		if(jj!=2) SOM::move = jj;

		//14+j is prepared by som_game_equip
		mv = pr2.uc[14+jj];

		if(!mv) mv = 255; //I guess???
	}
	else //non-weapon?
	{
		SOM::move = 0; //note sure???

		//TODO: som.files.cpp could rearrange this in advance
		//YUCK: for better or worse this is how I programmed
		//it... but I think I had a different model in mind
		//where the item type would be the standard animation
		//ID and I hadn't conceived of ITEM.ARM
 		mv = prf->my; if(mv==255)
		{
			//NOTE: I think nonweapon PRM records don't have
			//room for 4 attack stats
			if(j<2) j+=2; else return;	
		}
		if(j||mv==255) mv = (&prf->_0a)[j];
		
		//this is a little bit ambiguous as is? this is how I
		//originally designed it (years earlier) but I thought
		//it wasn't practical... until I realized the ability
		//to remap the animation ID made it quite feasible, so
		//this is how som_game_nothing works for right now
		if(mv==255) mv = prf->_0a; 
	}
	SOM::movesets[i][j] = 0;
	SOM::movesnds[i][j] = 0;
	
	//9+j is prepared by som_game_equip
	auto snd = pr2.us[9+j]; auto pitch = pr2.uc[26+j]; //2023

	extern int som_logic_4041d0_move;
	if(i==0) som_logic_4041d0_move = -1;

//	if(i==0&&mv!=255) //weapon?
	if(!i&&!j&&mv!=255) //weapon? primary?
	{
		//HACK: write glove into *som_game_nothing()?
		int pce2 = SOM::L.pcequip[0];
		auto &prf2 = SOM::L.item_prm_file[pce2].s[0];
		auto &pr22 = pce==pce2?pr2:SOM::L.item_pr2_file[prf2];

		//this is prepared by som_game_equip
		int j2 = pce==pce2?0:72-14; //glove?

		//som_scene_4412E0_swing sets up the animation ID
		//since it may not fit in 1B 
		float swap = pr22.f[20];
		memcpy(pr22.c+72,&item_arm[mv]._0a,16);
		//2023: preserve radius stats
		pr22.f[20] = swap; 
		som_logic_4041d0_move = mv;

		//this is prepared by som_game_equip
		if(WORD snd=pr2.us[9+j+j2/2])
		if(snd!=(WORD)-1)
		pr22.us[37] = snd; //SND
		if(pr2.c[26+j+j2]!=127)
		pr22.c[85] = pr2.c[26+j+j2]; //SND_pitch

		pr2.c[84] = 1; //HACK: continue to mark as moveset

		if(pce2==nothing[0])
		{
			BYTE *dmg = SOM::L.item_prm_file[pce2].uc+300;
			if(j2)
			memcpy(dmg,SOM::L.item_prm_file[pce].uc+320,8);
			else memset(dmg,0x00,8);

			//STILL ISN'T UPDATED UNTIL LEAVING MENU
			int fi = som_game_menu::find_item(pce2);	
			memcpy(som_game_menu::prm_sorted+fi*336+300,dmg,8);
		}
		
		snd = pr22.us[37]; pitch = pr22.c[85]; //2023
	}
	else if(i==3&&SOM::L.pcequip[0]==nothing[0]) //empty glove?
	{
		//HACK: it's simplest to just force update the weapon
		som_game_moveset(0,j);
	}

	prf = item_arm+mv;
	
	if(!i&&j&&pce==SOM::L.pcequip[0])
	{
		if((mv==0||mv==255)||random&&j!=2) 
		{
			prf = item_arm+pr2.uc[14];

			mv = prf->mvs[j]; prf = item_arm+mv;
		}
	}

	if(mv==255||!prf->my) return; //valid record?
	
	auto *prf22 = (SOM::Struct<22>*)prf;

	if(snd==0||snd>=0xFFFF)
	{
		snd = prf22->us[37]; pitch = prf22->uc[85];
	}

	SOM::movesnds[i][j] = snd<0xffff?snd<<16:0;
	SOM::movesnds[i][j]|=pitch==127?0:pitch;
	SOM::movesets[i][j] = prf22;
}
#pragma optimize("",on)
extern void __cdecl som_game_equip() //427170
{	
	//2022: I'm rewriting this in full, including 
	//rewriting the 4275e0 and 427170 subroutines
	//the ostensible reason is to remove patterns
	//that cause SND and SFX ref-counts to become
	//unreliable, but too the old code was really
	//top heavy with crusty hacks on top of hacks

	//Status event? pcequip2_in_waiting?
	SOM::Game::item_lock_section raii;

	BYTE *pce = SOM::L.pcequip;
	static BYTE old[8] = {255,255,255,255,255,255,255,255};

	//REMOVE ME?
	int *nothing = som_game_nothing();	
	for(int i=0;i<8;i++) if(pce[i]>=250)
	{
		assert(pce[i]==0xFF); pce[i] = nothing[i];
	}
	//2021: can't update equipment until animation completes	
	memcpy(SOM::L.pcequip2_in_waiting,pce,8);

	auto &mdl = *SOM::L.arm_MDL;	
	bool reset = SOM::L.corridor->dbsetting;
	if(!reset&&(mdl.d>1||mdl.ext.d2))
	{
		memcpy(pce,old,8); //pcequip2_in_waiting?

		SOM::L.pcequip2 += 1; return; //!!
	}
	else SOM::L.pcequip2 = 0;

	if(!SOM::movesets) //one_off
	{
		(void*&)SOM::movesnds = new DWORD[4*7]();
		(void*&)SOM::movesets = new void*[4*7](); //C++
	}
	if(!SOM::PARAM::Item.arm)
	{
		SOM::PARAM::Item.arm->open();
		if(auto*item_arm=SOM::PARAM::Item.arm)
		for(int i=256;i-->0;)
		{			
			auto &prf = SOM::L.item_pr2_file[i];
			auto &it = (swordofmoonlight_prf_item_t&)prf.c[62];
			auto &mv = (swordofmoonlight_prf_move_t&)prf.c[72];

			if(0==it.equip) continue; //2024: plain item?

			if(1==it.equip&&0==it.my) mv.SND = SOM::SND(mv.SND); //2024

			//this memory isn't used by the player
			//(backing up SND/pitch fields for som_game_moveset)
			memcpy(prf.c+14,&mv,16);
			//this is just for printing the radius
			//in equip/shop menus... assuming the
			//first radius is best?
			mv.hit_radius = som_game_measure_weapon(prf.c+31); //2023		
		}
	}
	for(int i=0;i<7;i++) if(old[i]!=pce[i]||old[i]>=250)
	{
		//2023: Battle Axe needs to override h. attacks
		//som_game_moveset(i,0); //reset to primary move?
		for(int j=0;j<4;j++) som_game_moveset(i,j);
	}

	if(pce[0]<250) //animation glitches fixes?
	{
		auto &prm = SOM::L.item_prm_file[pce[0]];
		auto &pr2 = SOM::L.item_pr2_file[prm.s[0]];

		//HACK: clear som_MDL_449d20_swing_mask cache?
		auto *mv = SOM::movesets[0][0];
		mdl.c = mdl.animation(mv?mv->us[0]:pr2.uc[72]);
	}
	else mdl.c = -1; 
	
	mdl.d = 1; mdl.rewind(); //mdl.f = -1;

	if(pce[0]!=old[0])
	for(int pass=1;pass<=2;pass++) //weapon?
	{
		int e = (pass==1?old:pce)[0];
		if(e>=250) continue;

		auto &prm = SOM::L.item_prm_file[e];
		auto &pr2 = SOM::L.item_pr2_file[prm.s[0]];
		
		//sound effect?
		extern BYTE __cdecl som_MPX_43f4f0(DWORD);
		extern BYTE __cdecl som_MPX_43f420(DWORD,BYTE);
		if(pass==1) som_MPX_43f4f0(pr2.us[37]);
		if(pass==2) som_MPX_43f420(pr2.us[37],1);

		//sword magic?
		for(int sm=310;sm<=310;sm++) //TODO: KF2 sword magic
		{
			int m = prm.c[sm];		
			WORD &prf = SOM::L.magic_prm_file[m].us[0]; //!
			if(m>=250||prf>=256) continue;
			WORD sfx = SOM::L.magic_pr2_file[prf].us[16];
			//TODO: defer this until magic finishes up
			//NOTE: 42eb00 seems to remove instances of the SFX
			if(pass==1) ((BYTE(__cdecl*)(WORD))0x42eb00)(sfx);
			if(pass==1) ((BYTE(__cdecl*)(WORD))0x42e7e0)(sfx);
			if(pass==2) ((BYTE(__cdecl*)(WORD))0x42e5c0)(sfx);
		}
	}	
	if(pce[7]!=old[7])
	for(int pass=1;pass<=2;pass++) //magic?
	{
		int e = (pass==1?old:pce)[7];
		if(e>=32) continue;

		int m = SOM::L.magic32[e];
		WORD &prf = SOM::L.magic_prm_file[m].us[0]; //!
		if(m>=250||prf>=256) continue;
		WORD sfx = SOM::L.magic_pr2_file[prf].us[16];
		//TODO: defer this until magic finishes up
		//NOTE: 42eb00 seems to remove instances of the SFX
		if(pass==1) ((BYTE(__cdecl*)(WORD))0x42eb00)(sfx);
		if(pass==1) ((BYTE(__cdecl*)(WORD))0x42e7e0)(sfx);
		if(pass==2) ((BYTE(__cdecl*)(WORD))0x42e5c0)(sfx);
	}

	char m = 3, mm = 3;
	for(int pass=1;pass<=2;pass++) //suit of armor?
	{
		int a = (pass==1?pce:old)[2]; if(a>=250) continue;

		int prm = SOM::L.item_prm_file[a].us[0];

		//RFC? 250??? this is copying the behavior below...
		if(4==SOM::L.item_pr2_file[prm>=256?250:prm].c[72])
		{
			(pass==1?m:mm) = 2;
		}
	}
	if(m!=mm) //apples and oranges?
	{
		old[m] = ~pce[m]; //ensure unequal in below block...
	}

	char eq[8] = {0,m,m,m,5,m,m,m};

	char eqf[8] = {0,1,2,3,-1,1,2,3};

	//som_MDL_449d20_swing_mask insists on exactly 9 channels
	int n = mdl->skeleton_size==9?8:4;

	SOM::Struct<> **files = SOM::L.arm_MDO_files; 

	SOM::MDO **oo = SOM::L.arm_MDO; for(int i=0;i<n;i++,oo++) 
	{
		auto* &o = *oo; if(i==3) oo = SOM::L.arm2_MDO-1; 
		
		int a = pce[eq[i]]; if(a==old[eq[i]]) continue;

		extern void __cdecl som_MDO_445870(void*);

		auto **f = files+eqf[i]; if(i<5) if(i<4) //free file?
		{
			if(*f) som_MDO_445870(*f); *f = 0;
		}
		else if(o) som_MDO_445870(o->mdo_data()); //shield file?

		((BYTE(__cdecl*)(void*))0x445ad0)(o); o = 0; //free instance?

		if(a>=250) continue;

		SOM::MDO::data *file = 0; if(i<5) //mirroring arm on left side
		{
			int prm = SOM::L.item_prm_file[a].us[0]; //RFC? 250???
			char *fname = SOM::L.item_pr2_file[prm>=256?250:prm].c+31;
			char path[64]; if(*fname) //Nothing?
			{
				//4c1d1c is "A:\\>\\data\\item\\model\\"
				int cat = sprintf(path,"%s%s",(char*)0x4c1d1c,fname);
				//if('.'==path[cat-4]) //art?
				{
					char _i[4] = "_0"; if(i>1&&i<4) _i[1]+=i-1;

					//memcpy(path+cat-4,"_0.mdo",8);
					int ext = PathFindExtensionA(path)-path;
					memcpy((char*)memmove(path+ext+2,path+ext,cat-ext+1)-2,_i,2);
					extern SOM::MDO::data *som_MDO_445660(char*);
					file = som_MDO_445660(path);
				}
			}
		}
		else (void*&)file = files[i-4]; 
		
		if(i<4) (void*&)*f = file; if(!file) continue; 

		(void*&)o = ((void*(__cdecl*)(void*))0x4458d0)(file); //make instance

		DWORD *ui = o->ui; //427487 does this		
		 
		//I wasn't getting a picture without this part
		//they're all 0 except 1,1,1 but the destination
		//seems to be junk (require zeroing)

		ui[0x10/4] = ui[0x4/4];
		ui[0x14/4] = ui[0x8/4];
		ui[0x18/4] = ui[0xc/4];

		ui[0x28/4] = ui[0x1c/4];
		ui[0x2c/4] = ui[0x20/4];
		ui[0x30/4] = ui[0x24/4];

		ui[0x40/4] = ui[0x34/4];
		ui[0x44/4] = ui[0x38/4];
		ui[0x48/4] = ui[0x3c/4];

		ui[0x58/4] = ui[0x4C/4];
		ui[0x5C/4] = ui[0x50/4];
		ui[0x60/4] = ui[0x54/4];

		ui[0x70/4] = ui[0x64/4]; //1.0
		ui[0x74/4] = ui[0x68/4]; //1.0
		ui[0x78/4] = ui[0x6c/4]; //1.0
	}

	memcpy(old,pce,sizeof(old));
}

extern int som_hacks_inventory_item;
namespace som_menus_x
{
	namespace eq
	{
		extern int &item = som_hacks_inventory_item; //2024
		extern bool info;
		extern int kind;
		extern int pages[8];		
	}
}
extern int *som_game_nothing();
extern float som_hacks_inventory_accum[3];
static DWORD som_game_0_shop43d0c0(BYTE shop)
{
	som_game_menu::swap_equip(); //Shop

	som_hacks_inventory_item = 255; //disable?

	//DWORD *hl = (DWORD*)0x1D11DEC;
	//EX::dbgmsg("menu: Shop (%d %d)",hl[0],hl[1]);	
	som_game_menu::recall_shop(shop);
	DWORD ret = ((DWORD(__cdecl*)(BYTE))0x43d0c0)(shop);

	som_game_menu::swap_equip(); return ret;
}
static DWORD som_game_1_shop43d3b0(BYTE shop)
{
	//EX::dbgmsg("menu: Buy");	
	som_game_menu::recall_item(10);
	return ((DWORD(__cdecl*)(BYTE))0x43d3b0)(shop);
}
static DWORD som_game_2_shop43e150(DWORD shop)
{
	som_game_menu::swap_equip(); //SELL

	//DWORD *hl = (DWORD*)0x1D11DC0; //flashing pulse?
	DWORD *hl = (DWORD*)0x1D11E10;
	//EX::dbgmsg("menu: Sell %d %d",hl[0],hl[1]);	
	som_game_menu::recall_item(11);
	DWORD ret = ((DWORD(__cdecl*)(DWORD))0x43e150)(shop);

	som_game_menu::swap_equip(); return ret;
}
static DWORD som_game_3_shop43ea90(BYTE shop)
{
	EX::dbgmsg("menu: Show");	
	som_game_menu::recall_item(12);
	return ((DWORD(__cdecl*)(BYTE))0x43ea90)(shop);
}
static DWORD som_game_0_menu418700(DWORD A)
{
	som_hacks_inventory_item = 255; //disable?

	som_game_menu::swap_equip();

	//EX::dbgmsg("menu: Menu");	
	som_game_menu::recall(-1);
	DWORD ret = ((DWORD(__cdecl*)(DWORD))0x418700)(A);

	som_game_menu::swap_equip(); return ret;
}
static DWORD som_game_1_menu419020(DWORD A)
{
	//EX::dbgmsg("menu: Item");		
	som_game_menu::recall_item(0);
	DWORD ret = ((DWORD(__cdecl*)(DWORD))0x419020)(A);		
	//if(ret==0xFFffFFff) //maps
	{
		using namespace som_game_menu;
		/*
		004198A5 88 1D 60 AB 9A 01    mov         byte ptr ds:[19AAB60h],bl  
		004198AB E8 90 99 00 00       call        00423240  
		004198B0 83 C4 08             add         esp,8  
		004198B3 83 C8 FF             or          eax,0FFFFFFFFh
		*/		
		//if(ini_browse) //maps
		{
			//3: Read past @0, @1, etc.
			int i = items[2];
			int j = atoi((char*)prm_sorted+336*i+3); 			

			//Maps only?
			WORD &map = SOM::L.pcstore[j] = inv_sorted[i];
			if(0==map>>8)
			if(_map_item_was_used_up_(items[2]))
			{	
				//HACK: CLASSIC BUG FIX
				//The highlight hangs off the end of the menu.
				int *hl = (INT*)0x19AB1D8;
				if(hl[0]+hl[1]>=itemenuN[0])
				{
					if(hl[0]) hl[0]--; else if(hl[1]) hl[1]--;
				}

				_scroll_removal(hl[0],hl[1]);
			}
			else assert(0);

			if(ret==0xFFffFFff)
			{
				//NOTE: this code picks up on the item (and uses it?)
				//00418690 A0 60 AB 9A 01       mov         al,byte ptr ds:[019AAB60h]  
				//00418695 8B 54 24 38          mov         edx,dword ptr [esp+38h]  
				//00418699 8B 4C 24 3C          mov         ecx,dword ptr [esp+3Ch]  
				//0041869D 88 02                mov         byte ptr [edx],al 
				BYTE &o = *(BYTE*)0x19AAB60;
				assert(o==(i&0xFF));
				o = j&0xFF;
			}
		}
	}
	return ret;
}
static DWORD som_game_2_menu419b60(DWORD A)
{	
	//EX::dbgmsg("menu: Magic");
	som_game_menu::recall_item(-1);	
	DWORD ret = ((DWORD(__cdecl*)(DWORD))0x419b60)(A);

	using namespace som_game_menu;
	int i = find_magic(items[1],SOM::L.magic32);	
	som_hacks_inventory_item = -i-1;

	if(ret==0xFFffFFff)
	{
		/*
		00419F4A A2 C8 B1 9A 01       mov         byte ptr ds:[019AB1C8h],al  
		00419F4F 5B                   pop         ebx  
		00419F50 83 C8 FF             or          eax,0FFFFFFFFh  
		00419F53 83 C4 64             add         esp,64h  
		00419F56 C3                   ret  
		*/
		//NOTE: this code picks up on the item (and uses it?)
		//0041869F 8A 15 C8 B1 9A 01    mov         dl,byte ptr ds:[19AB1C8h]  
		//004186A5 8B 44 24 30          mov         eax,dword ptr [esp+30h]  
		//004186A9 88 11                mov         byte ptr [ecx],dl  
		//and what are these two ???
		//004186AB 8A 0D 98 B1 9A 01    mov         cl,byte ptr ds:[19AB198h]  
		//004186B1 8B 54 24 34          mov         edx,dword ptr [esp+34h]  
		//004186B5 88 08                mov         byte ptr [eax],cl  
		//004186B7 A0 70 A9 9A 01       mov         al,byte ptr ds:[019AA970h]  
		//004186BC 88 02                mov         byte ptr [edx],al  
		/*428220 sets up the screen effect (ax holds the SFX number)
		004256A8 8B 4C 24 48          mov         ecx,dword ptr [esp+48h]  
		004256AC 8D 04 40             lea         eax,[eax+eax*2]  
		004256AF 8D 14 80             lea         edx,[eax+eax*4]  
		004256B2 33 C0                xor         eax,eax  
		004256B4 66 8B 41 20          mov         ax,word ptr [ecx+20h]  
		004256B8 D1 E2                shl         edx,1  
		004256BA 52                   push        edx  
		004256BB 50                   push        eax  
		004256BC E8 5F 2B 00 00       call        00428220 
		*/
		BYTE &o = *(BYTE*)0x19AB1C8;
	//	o = find_magic(items[1],SOM::L.magic32);
		o = 0xFF&i;
	}

	return ret;
}
static DWORD som_game_3_menu41a1e0(DWORD A)
{	
	som_game_menu::swap_equip();

	int i = som_game_menu::nonitemenusel[0]; 
	BYTE &e = SOM::L.pcequip[i];

	som_menus_x::eq::pages[i] = 0;
	som_menus_x::eq::kind = i; //2024
	som_menus_x::eq::info = false; //2024

	//EX::dbgmsg("menu: Equip");
	som_game_menu::recall(0);
	DWORD ret = ((DWORD(__cdecl*)(DWORD))0x41a1e0)(A);	
	//EX::dbgmsg("Equip? %d",ret);
	
	som_game_menu::swap_equip(); 
	
	i = som_game_menu::nonitemenusel[0]; 
	i = e==som_game_nothing()[i]?250:7==i?-1-e:e; 
	som_hacks_inventory_item = i;

	som_menus_x::eq::info = false;
	
	return ret;
}
static DWORD som_game_4_menu41c370(DWORD A)
{
	som_game_menu::swap_equip();

	//EX::dbgmsg("menu: Summary");	
	som_game_menu::recall(-1);
	DWORD ret = ((DWORD(__cdecl*)(DWORD))0x41c370)(A);

	som_game_menu::swap_equip(); return ret;
}
static DWORD som_game_5_menu41cbd0(DWORD A)
{
	//EX::dbgmsg("menu: Inventory");	
	som_game_menu::recall_item(8);
	DWORD ret = ((DWORD(__cdecl*)(DWORD))0x41cbd0)(A);	
	if(0==ret) //5 is idle
	{
		using namespace som_game_menu;
		if(ini_browse)
		{
			//2020: _sort can't always do this because
			//of the 250 optimization I don't remember
			int m = -1; pcm_sorted = 0;

			for(int i=0,m=-1;i<N;i++)
			{
				int j = ini_browse[i]; if(j<250)
				{
					SOM::L.pcstore[j] = inv_sorted[i];
				}
				else //SAME AS _sort (DUPLICATION)
				{
					m++; //2020

					j = 1<<j-250;

					if(inv_sorted[i]&0x80)
					{
						pcm_masked&=~j;
					}
					else pcm_masked|=j;
						 
					//2020: exclude "Nothing" (som_game_nothing)
					//and masked from pcm_sorted 
					j&=SOM::L.pcmagic&~pcm_masked;

					//2020: _sort can't always do this because
					//of the 250 optimization I don't remember
					if(j) pcm_sorted|=1<<m;
				}
			}
		}
		if(som_game_empty_shop)
		*(DWORD*)som_game_empty_shop->stock = pcm_masked;
	}
	return ret;
}
static DWORD som_game_6_menu41d760(DWORD A)
{
	//EX::dbgmsg("menu: System");	
	som_game_menu::recall(1);
	return ((DWORD(__cdecl*)(DWORD))0x41d760)(A);
}
static DWORD som_game_7_menu41adf0(DWORD *A)
{
	som_game_menu::swap_equip();

	//EX::dbgmsg("menu: Equip Item");
	int type = *(INT*)0x19AB204;
	som_game_menu::recall_item(1+type);	

	som_menus_x::eq::kind = type; //2024
	som_menus_x::eq::info = *(BYTE*)0x19ab228!=0;

	BYTE cmp[8]; memcpy(cmp,SOM::L.pcequip,8);
	DWORD ret = ((DWORD(__cdecl*)(DWORD*))0x41adf0)(A);

	if(EX::debug||EX::INI::Launch()->do_ask_to_attach_debugger) //shop?
	if(*(BYTE*)0x19ab228) //2024: horizontal arrows
	{		
		static unsigned wait = 0;
		if(0x40000000==*(DWORD*)0x19aab6c) //4212a7
		{
			if(EX::tick()-wait>180)
			{
				wait = EX::tick();

				som_menus_x::eq::pages[type]--; SOM::se(A[0x1a0/4]);
			}
		}
		else if(0x80000000==*(DWORD*)0x19aab6c) //4212b9
		{
			if(EX::tick()-wait>180)
			{
				wait = EX::tick();

				som_menus_x::eq::pages[type]++; SOM::se(A[0x1a0/4]);
			}
		}		

		float *menu = (float*)(SOM::menupcs-0xA8/4);
		assert((DWORD*)menu==A);

		float &a = menu[0x28];
		float &b = menu[0x29];
		//float &c = menu[0x70] = 1.0f; //menu is flashing

		//static int y = 145, x = 125; //55 560

		if(Exselector)
		{
		//	Exselector->watch("x",&x); 
		//	Exselector->watch("y",&y);
		}

		//NOTE: D3DRENDERSTATE_ZENABLE doesn't work here???
		//how will this look in VR? probably wrong place???
		DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_ZFUNC,DX::D3DCMP_ALWAYS);
		{
			int layout1[6] = {125,175,(int)(40/a+0.5f),(int)(40/b+0.5f),4,0}; //180
			som_game_421f10_lr_arrow((DWORD*)layout1,menu,1.0f); //menu[0x70]
			int layout2[6] = {630-125,175,(int)(40/a+0.5f),(int)(40/b+0.5f),5,0}; 
			som_game_421f10_lr_arrow((DWORD*)layout2,menu,1.0f); //menu[0x70]
		}
		DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_ZFUNC,DX::D3DCMP_LESSEQUAL);
	}

	//Returns 7:idle, 3:cancel/confirm (useless)
	//EX::dbgmsg("Equip Item? %d",out);
	assert(ret==7||ret==3);
	//REMINDER: the unequip function doesn't leave
	//the equip submenu; testing out is not useful
	//bool diff = false;
	if(/*out==3&&*/0!=memcmp(cmp,SOM::L.pcequip,8))
	{
		//FYI: this isn't done on leaving the Equip or Main menu
		//screens because som_game_nothing needs to convert 255
		//into the nothing-equipment. IT'S ACTUALLY A PROBLEM IF
		//EVENTS CAN'T RESPOND TO IT

		//full body armor logic requires checking arms/legs/body
		//Note: Helmets are not included. Even in the other full
		//body piece that isn't headless
		for(type=0;type<7;type++) 
		if(cmp[type]!=SOM::L.pcequip[type]) 
		{
			using namespace som_game_menu;
			BYTE &e = SOM::L.pcequip[type];
			assert(e<0xFA||e==0xFF);
//			if(0||!EX::debug) //TESTING 255
			if(e>=250) e = find_item(som_game_nothing()[type]);	
			//REMINDER: The item names should be @0, @1, etc.
			pcequip[type] = e>=250?e:atoi((char*)prm_sorted+e*336+3);
	//		diff = true;

			//HACK: set Nothing attack damage to use
			//the glove?
			if(type==0||type==3)
			if(pcequip[0]==som_game_nothing()[0])
			som_game_weapon_to_glove(pcequip[3]);
		}
	}

	som_game_menu::swap_equip();
	{	
		/*2021: I think maybe this is interfering with pcequip2_in_waiting
		//(anyway, the way it's described it should be obsolete)
		//trying to get legacy swing model change behavior
		//although, it's buggy since the weapon changes but
		//the animation doesn't
		//NOTE: this way players can preview the swing model
		//but if it's not done it will change after the menu
		//is exited (anyway)
		if(diff) som_game_equip();
		*/
	}
	return ret;
}
static DWORD som_game_8_menu41bc00(DWORD A)
{
	som_game_menu::swap_equip();

	//EX::dbgmsg("menu: Equip Magic");	
	som_game_menu::recall_item(-2);
	BYTE cmp = SOM::L.pcequip[7];
	DWORD ret = ((DWORD(__cdecl*)(DWORD))0x41bc00)(A);

	using namespace som_game_menu;
	int i = find_magic(items[0],SOM::L.magic32); 
	som_hacks_inventory_item = -i-1;

	//REMINDER: The unequip function doesn't leave
	//the equip submenu; testing out is not useful.
	if(/*ret==3*/cmp!=SOM::L.pcequip[7]) 
	{
		//FYI: This isn't done on leaving the Equip or Main menu
		//screens because som_game_nothing needs to convert 255
		//into the nothing-equipment. IT'S ACTUALLY A PROBLEM IF
		//EVENTS CAN'T RESPOND TO IT.

		BYTE &e = SOM::L.pcequip[7];
		assert(e<32||e==0xFF);
//		if(0||!EX::debug) //TESTING 255
		if(e>=32) e = find_magic(som_game_nothing()[7]);
		pcequip[7] = find_magic(e,SOM::L.magic32);
	}

	som_game_menu::swap_equip();
	{
		//2021: I think this should be calling som_game_equip?
		//(is 427170 not required?)
		//NOTE: I'm changing it to be called on main menu exit
	}
	return ret;
}
extern bool som_game_continuing = false;
static DWORD som_game_9_menu41f880(DWORD A)
{
	//EX::dbgmsg("menu: Load");
	som_game_menu::recall(2);
	som_game_continuing = !SOM::field;
	assert(A==(SOM::field?0x019aa978:0x1A5B400));
	assert(SOM::menupcs==(DWORD*)(SOM::field?0x19AAA20:0x1A5B4A8));
	DWORD ret = ((DWORD(__cdecl*)(DWORD))0x41f880)(A);
	som_game_continuing = false;
	return ret;
}
static DWORD som_game_10_menu4200a0(DWORD A)
{
	//EX::dbgmsg("menu: Save (%d/%d)",*(DWORD*)0x019AB310,*(DWORD*)0x019AB314);	
	som_game_menu::recall(3);
	return ((DWORD(__cdecl*)(DWORD))0x4200a0)(A);	
}
static DWORD som_game_11_menu41e100(DWORD A)
{
	EX::INI::Option op;

	//EX::dbgmsg("menu: Options");
	som_game_menu::recall(4);
	UINT x = SOM::L.config[1], y = SOM::L.config[2];
	int vol2[2] = { SOM::L.bgm_volume,SOM::L.snd_volume };
	int bels[2] = { SOM::millibels[0],SOM::millibels[1] };
	DWORD ret = ((DWORD(__cdecl*)(DWORD))0x41e100)(A);
	//if(6==ret) //these change on cancel as well
	if(x!=SOM::L.config[1]||y!=SOM::L.config[2])
	{
		//HACK: handle resolution change by reinitializing the menu
		//0x19AAA18 is SOM::L.menupcs[-2,-1] or [40,41] if moved A8
		extern void som_game_menucoords_set(bool);
		//SOM::fov[0] = SOM::L.config[1];
		//SOM::fov[1] = SOM::L.config[2];
		SOM::width = SOM::L.config[1];
		SOM::height = SOM::L.config[2];
		SOM::fov[0] = SOM::width*DDRAW::xyScaling[0];
		SOM::fov[1] = SOM::height*DDRAW::xyScaling[1];
		som_game_menucoords_set(true);
	}
	if(11!=ret) if(op->do_opengl) //DICEY
	{
		if(SOM::opengl&&!DDRAW::gl)
		{
			SOM::opengl = 0;
		}
		//042133f initializes the menu object
		//from vp_bpp
		SOM::L.vp_bpp = 
		SOM::L.config[3] =
		SOM::L.config2[3] = SOM::opengl?32:16; //DUPLICATE
	}
	//previously som_hacks_SetVolume	
	if(vol2[0]!=SOM::L.bgm_volume||vol2[1]!=SOM::L.snd_volume)
	{
		//may want to replace these?
		SOM::bgmVol = SOM::L.bgm_volume;
		SOM::seVol = SOM::L.snd_volume;
		SOM::config_bels(3);
		DSOUND::Sync(SOM::millibels[0]-bels[0],SOM::millibels[1]-bels[1]);
	}
	return ret;
}
static DWORD som_game_447400(DWORD dev, DWORD dm, DWORD *w, DWORD *h, DWORD *bpp)
{
	if(!((DWORD(__cdecl*)(DWORD,DWORD,DWORD*,DWORD*,DWORD*))0x447400)(dev,dm,w,h,bpp))
	{
		assert(0); return 0;
	}

	//HACK: call (knocked out) release function before
	//moving_target
	((void(__cdecl*)())0x401fb0)();

	//HACK: these overlap in memory and are user managed
	for(size_t i=EX_ARRAYSIZEOF(DDRAW::vshaders9);i-->0;) 
	{
		if(!DDRAW::gl)
		delete[] DDRAW::vshaders9[i]; DDRAW::vshaders9[i] = 0;
	}
	for(size_t i=EX_ARRAYSIZEOF(DDRAW::pshaders9);i-->0;) 
	{
		if(!DDRAW::gl)
		delete[] DDRAW::pshaders9[i]; DDRAW::pshaders9[i] = 0;
	}
	
	//if(op->do_opengl)
	assert(EX::INI::Option()->do_opengl);
	{
	//	auto &bpp = SOM::L.config[3];

		DDRAW::doNvidiaOpenGL = false;

		//Exselector is needed for fonts
		extern bool SomEx_load_Exselector();
		if(*bpp==32&&DDRAW::compat&&SomEx_load_Exselector())
		{
			SOM::opengl = 1;
			DDRAW::target = 'dxGL';
		}
		else 
		{
			SOM::opengl = 0;
			DDRAW::target = 'dx9c';
		}

		//HACK: DDRAW::is_needed_to_initialize sets this
		DDRAW::compat_moving_target = DDRAW::target;

		*bpp = SOM::bpp==16?16:32; //DICEY
	}

	return 1;
}
static DWORD som_game_12_menu420910(DWORD A)
{
	//EX::dbgmsg("menu: Controls");
	som_game_menu::recall(5);
	return ((DWORD(__cdecl*)(DWORD))0x420910)(A);
}
static DWORD som_game_13_menu421010(DWORD A)
{
	//EX::dbgmsg("menu: Quit");
	som_game_menu::recall(-1);
	//Returns 13:idle, 6:cancel, -1:confirm.
	return ((DWORD(__cdecl*)(DWORD))0x421010)(A);
}
static DWORD som_game_14_menuUNUSED(DWORD)
{
	assert(0); return 0xFFFFFFFF;
}
static DWORD som_game_15_menuUNUSED(DWORD)
{
	assert(0); return 0xFFFFFFFF;
}
static DWORD som_game_16_menuUNUSED(DWORD)
{
	assert(0); return 0xFFFFFFFF;
}

static void som_game_highlight_load_game(int xx, bool saving)
{
	BYTE &sel = som_game_menu::nonitemenusel[2];
	BYTE &sel2 = som_game_menu::nonitemenusel2[0];
	
	if(!saving) sel = sel2 = 0; //Continuing?

	int m = 0;
	for(int i=0;i<xx;i++) 
	if(*som_game_saves[i].cFileName) //h is 0 on Continue
	{
		m++;
	}
	if(!saving)
	{
		sel = m%3; sel2 = m-m%3;
	}

	//HACK: this just advances the the load slot in case a
	//new save file is created
	if(saving) if(m<=sel+sel2+1) sel++;
}
extern void som_game_highlight_save_game(int xx, bool saving)
{
	if(xx>=0) //som.hacks.cpp???
	{
		//HACK: this just advances the the load slot in case a
		//new save file is created
		som_game_highlight_load_game(xx,saving);

		som_game_menu::nonitemenusel[3] = xx%3;	
		som_game_menu::nonitemenusel2[1] = xx-xx%3;
	}
	som_game_menu::nonitemenu = 255; //HACK: leaving pseudo menu
}

extern SOM::Thread *som_MPX_thread;
SOM::Game::item_lock_section::item_lock_section(int c)	
:EX::section(som_MPX_thread->cs) //RAII
{
	//NOTE: the original idea was to use these bits but there's
	//a need for events and delayed equip and nesting locks too
	SOM::Game.item_lock++; if(!c) return;
	auto e = (SOM::Context::belief)c; SOM::context = e; switch(e)
	{
	case SOM::Context::browsing_menu: SOM::Game.browsing_menu = 1; break;
	case SOM::Context::browsing_shop: SOM::Game.browsing_shop = 1; break;
	case SOM::Context::taking_item: SOM::Game.taking_item = 1; break;
	default: assert(0);
	}	
}
SOM::Game::item_lock_section::~item_lock_section() //RAII
{
	if(0xff&(--SOM::Game.item_lock)) return; //nested?

	som_MPX_swap::models_free(); SOM::Game.item_lock = 0; //UNION
};
static void __cdecl som_game_4183F0(DWORD A, DWORD B, DWORD C, DWORD D)
{
	//2022: take control of resources?
	SOM::Game::item_lock_section raii(SOM::Context::browsing_menu);

	SOM::menupcs = (DWORD*)0x19AAA20;
	{
		//2021: waiting for swing model to clear?
		int pce2 = SOM::L.pcequip2;
		if(pce2) for(int i=8;i-->0;) 
		std::swap(SOM::L.pcequip[i],SOM::L.pcequip2_in_waiting[i]);
		{
			//NOTE: this subroutine isn't specific to items menus
			som_game_menu::browsing_items_menu();
			((void(__cdecl*)(DWORD,DWORD,DWORD,DWORD))0x4183F0)(A,B,C,D);
		}
		//if som_game_equip was called the new settings should be
		//used instead
		if(pce2&&pce2==SOM::L.pcequip2)
		for(int i=8;i-->0;) 
		std::swap(SOM::L.pcequip[i],SOM::L.pcequip2_in_waiting[i]);
		//2021: calling this inside the submenus is no longer required
		//and is making pcequip2_in_waiting fail for reasons I haven't
		//been able to figure out
		som_game_equip(); 
		som_game_menu::recall(-1); //translate_dik

		//2020: closing save files cache on exiting main menu
		som_game_saves.close(); 
	}
	SOM::menupcs = (DWORD*)0x1A5B4A8;

	SOM::frame_is_missing();
}
extern float som_game_menucoords[2] = {};
extern void som_game_menucoords_set(bool reset) 
{
	float *xy = (FLOAT*)SOM::menupcs-2;
	float &x = xy[0], &y = xy[1];

	if(reset)
	{
		//do_scale_640x480_modes_to_setting/resolution Option change
		x = SOM::fov[0]/640; y = SOM::fov[1]/480;
	}

	float yy = 0.9f;
	if(y>x) //e.g. 1280 x 1024 
	{
		som_game_menucoords[0] = 0;
		som_game_menucoords[1] = y-x*yy;
		y = x; 
	}
	else //NOTE: also includes 1:1 (640x480)
	{
		//this is both to make it a little wider (there isn't
		//(enough room really, and to center vertically more in 
		//VR mode, which also can't be too wide... not now anyway)		
		som_game_menucoords[0] = x-y;
		som_game_menucoords[1] = y-y*yy;
		x = y; 
	}
	y*=yy;

	som_game_menucoords[0]*=640/2; 
	som_game_menucoords[1]*=480/2; 
	//HACK: in VR move left 2 pixels, so it's dead center, since this
	//might create an ideal optical illusion for determining your IPD
	//setting on the Inventory screen with faux 3D styled menu frames 
	int _350 = DDRAW::inStereo?-350:350;
	som_game_menucoords[0]+=SOM::fov[0]/_350; //2: nudging over a bit
	som_game_menucoords[1]+=SOM::fov[1]/70; //2: pushing down a ways

	if(reset) //2020: som_game_4212D0 did this just for 640x480 mode
	{
		x/=DDRAW::xyScaling[0];
		y/=DDRAW::xyScaling[1];
		som_game_menucoords[0]/=DDRAW::xyScaling[0];
		som_game_menucoords[1]/=DDRAW::xyScaling[1];
	}

	//UNFINISHED
	//this shrinks down so the entire menu is visible
	//but I don't know what to do with the font right
	//now
	if(DDRAW::inStereo) //testing
	if(EX::stereo_font)
//	if(EX::stereo_font<1.0f) //PSVR?
	{
		float o7 = EX::stereo_font; //0.7

		float aspect = 1; if(!DDRAW::xr) //PSVR?
		{
			//2021: trying to make this look nice even if
			//not meeting the PSVR 1920/1080 aspect
			aspect = SOM::fov[0]/SOM::fov[1];
			aspect = aspect/1.77777f; //1920/1080
		}

		//off center?
		//float vry = 0.6666666f, vr2y = 1.5f;
		//float vry = 0.7f, vr2y = 1.72f; (how to derive this?)
	//	float vry = o7, vr2y = 1.72f;
		float vry = o7;
		//float vrx = 0.7f*aspect; //vr2x = 1.36f;
		float vrx = o7*aspect; //vr2x = 1.36f;
		float xx = x, yy = y;
		x*=vrx; y*=vry;
		som_game_menucoords[0]/=640/2; 
		som_game_menucoords[0]+=xx-x;
		som_game_menucoords[0]*=640/2; 
		//som_game_menucoords[0]/=vrx/vr2x;
		//2022: this seems to work, but why wouldn't I have tried
		//this? it seems too simple
		//som_game_menucoords[1]/=vry/vr2y;		
		{
			//NOTE: this looked okay in PSVR mode to me, but I only
			//tried one resolution and I wasn't wearing a PSVR then
			//(I didn't try 1920x1080)

			som_game_menucoords[1]/=480/2; 
			som_game_menucoords[1]+=yy-y;
			som_game_menucoords[1]*=480/2; 
		}
	}

	//SUBPIXEL EFFECTS
	//text sits on the baseline differently for integer values. I'm
	//trying to make supersampled text more even. unfortunately what
	//seems to work makes it less crisp but more even... sitting on
	//every other pixel doesn't seem to matter for downsampling
	if(1) 
	{
		//NOTE: I think this fixes a gap glitch on the Continue
		//screen transparent background texture
		for(int i=0;i<2;i++)
		som_game_menucoords[i] = (int)som_game_menucoords[i];
		//WEIRD? integer values cause the menus frames to be
		//sampled in the middle of pixels only on the X axis
		//is less even supersampling 
		//NOTE: You'd think this will counteract the goal of
		//evening out the text, but it doesn't seem to alter
		//it (ID3DFont quirk?)
		//DOES som_db shift X over???
		som_game_menucoords[0]-=0.5f; 
	}
}
static void som_game_equip_menu_text_422ab0(HDC dc, char *p, int xx, int yy)
{
	((void(__cdecl*)(HDC,char*,int,int))0x422ab0)(dc,p,xx,yy); 

	//add x count to Equip menus
			
	int item = atoi(p+1); assert(*p=='@');
	int inv = ((BYTE*)(&SOM::L.pcstore[item]))[1];
	char c[8]; sprintf(c,"x%3d",inv);

	float *xy = (FLOAT*)SOM::menupcs-2;
	float &x = xy[0], &y = xy[1];

	xx = (int)(550*x);		

	((void(__cdecl*)(HDC,char*,int,int,int))0x422b20)(dc,c,xx,yy,DT_RIGHT);
}
extern BYTE __cdecl som_game_4212D0(FLOAT *_19AA978) //static 
{
	//2022: 4212D0 initializes textures and sounds
	//in some unexpected contexts
	EX::section raii(som_MPX_thread->cs);

	//som_state_40a5f0 has its "menu" on the stack
	//assert(_19AA978+0xA8/4==(FLOAT*)SOM::menupcs);
	
	float &x = _19AA978[40];
	float &y = _19AA978[41];
	//NOTE: this initializes the menu object so it would
	//be wrong to assign SOM::menupcs before calling it
	BYTE al = ((BYTE(__cdecl*)(FLOAT*))0x4212D0)(_19AA978);			
	//if(1)
	{
		//REMINDER: indicates EVENT BASED saving only
		extern bool som_state_saving_game;
		//0x219F9eC not set in Release builds????
		//is 0x219F9eC not a fixed address... maybe it
		//is general purpose event memory?? Debug builds appear to
		//be reliable, but not Release builds?!		
		if(SOM::menupcs==(DWORD*)0x219F9eC)
		{
			//REMINDER: som_state_40a5f0 sets this to 0x219F9eC
			//this test is to verify that it changes. and that I
			//didn't initially set it to a typo (it'on the stack)
			auto test = (DWORD*)_19AA978+0xA8/4;
			//assert(test==SOM::menupcs);
			SOM::menupcs = test;
			assert(som_state_saving_game);

			//2020: for some reason the save menu doesn't play a 
			//sound effect when opened like every other menu has
			//(except the main menu)
			if(som_state_saving_game) SOM::menuse(1);
		}
		else
		{
			assert(!som_state_saving_game);
			assert(_19AA978+0xA8/4==(FLOAT*)SOM::menupcs);	
		}

		som_game_menucoords_set(!SOM::field); //Continue?				
		
	//	if(!SOM::field) //do_scale_640x480_modes_to_setting
		{
			/*2020: moving into som_game_menucoords_set
			x/=DDRAW::xyScaling[0];
			y/=DDRAW::xyScaling[1];
			som_game_menucoords[0]/=DDRAW::xyScaling[0];
			som_game_menucoords[1]/=DDRAW::xyScaling[1];*/
		}
						
		//2020: for some reason the main menu doesn't play a 
		//sound effect when opened like every other menu has
		//(except the save menu)
		if(SOM::Game.browsing_menu) SOM::menuse(1);
	}
	return al;
}

static void som_game_iteminit(float axes[3], float *turntable)
{
	//turn table starting point
	//could store in y if it doesn't interfer with tilt transform
	//OR NOT? som_game_4230C0 stores turn in Y axis? or something
	//like it:
	//004231F0 C7 42 50 C2 B8 32 3E mov         dword ptr [edx+50h],3E32B8C2h
	//seems to confirm:
	//00418566 D9 40 50             fld         dword ptr [eax+50h]
	int x = axes[0]/(2*M_PI)+0.001f;
	float f = M_PI/18; //10deg (counterclockwise)
	if(x&1) f+=M_PI/2; //90deg (view of item from side)
	if(x&2) f+=M_PI; //180deg (typically reverse side blade is on)
	*turntable = f;
	//tilt x or z?		
	f = fmodf(axes[0],M_PI*2);
	//if(SOMEX_VNUMBER==0x1020209UL) //demo
	//x|=4;
	axes[x&4?0:2] = 0;
	axes[x&4?2:0] = f;
}
static void __cdecl som_game_410830(DWORD _1, DWORD _2)
{
	//seems _1 is 0 if the item isn't on the ground
	if(_1) assert(_2==((SOM::Item*)_1)->prm);

	som_hacks_inventory_item = (int)_2;
	//EX::dbgmsg("item is %d",som_hacks_inventory_item);

	((void(__cdecl*)(DWORD,DWORD))0x410830)(_1,_2);
	{
		//_1/esi is SOM::L.items record
		//_2/eax is the ITEM.PRM record
		//0041086B 8B 84 24 1C 01 00 00 mov         eax,dword ptr [esp+11Ch]  
		//00410872 8B B4 24 18 01 00 00 mov         esi,dword ptr [esp+118h]

		//per profile instance table (64*4)
		//seems like it may be saved for the duration of the menu
		//004108B5 8B 04 8D D0 6F 55 00 mov         eax,dword ptr [ecx*4+556FD0h]
		//004108BC A3 60 60 58 00       mov         dword ptr ds:[00586060h],eax 
		if(auto*ecx=*(SOM::MDO**)0x586060) //SOM::L.take_MDO
		{
			if(int did=EX::Joypads[0].JslDeviceID) //EXPERIMENTAL
			{
				SOM::motions.sixaxis_calibration = JslGetMotionState(~did);
				memset(som_hacks_inventory_accum,0x00,3*sizeof(float));
			}

			//rotation
			//004109FC C7 40 50 C2 B8 32 3E mov         dword ptr [eax+50h],3E32B8C2h
			//ecx[14] //rotation
			//00410A66 D9 59 24             fstp        dword ptr [ecx+24h]  	
			//ecx[7] //x axis (8/9 are y/z)	
			som_game_iteminit(ecx->f+7,ecx->f+14);		

 			if(DDRAW::xr) //OpenXR?
			{
				ecx->f[8]+=SOM::uvw[1]; //face forward?

				//TODO: might want to lift up some?

				for(int i=3;i-->0;)
				{
					//NOTE: som_hacks_SetTransform has to skirt around some
					//code added to better center items since it makes them
					//completely invisible

					//WARNING
					//for some reason the items appear higher up than right
					//in front of your face, the shorter distance the higher

					ecx->f[13+i] = SOM::cam[i]+0.66666f*SOM::pov[i]; //world space?			
				}
				ecx->f[13+1]-=0.1f;
			}
		}
	}
}
static BYTE __cdecl som_game_4230C0(DWORD _1, FLOAT _2, FLOAT _3, FLOAT _4, DWORD _5)
{		
	//som_hacks_inventory_item = _1;
	WORD *ib = som_game_menu::ini_browse;
	som_hacks_inventory_item = ib?ib[_1]:_1;
	//EX::dbgmsg("item is %d",som_hacks_inventory_item);

	/*
	004191AB 56                   push        esi  
	004191AC 83 EC 0C             sub         esp,0Ch  
	004191AF 89 44 24 44          mov         dword ptr [esp+44h],eax  
	004191B3 8B CC                mov         ecx,esp  
	004191B5 89 01                mov         dword ptr [ecx],eax  
	004191B7 C7 44 24 44 0A D7 A3 BE mov         dword ptr [esp+44h],0BEA3D70Ah  
	004191BF 8B 54 24 44          mov         edx,dword ptr [esp+44h]  
	004191C3 89 51 04             mov         dword ptr [ecx+4],edx  
	004191C6 89 41 08             mov         dword ptr [ecx+8],eax  
	004191C9 8B 44 24 24          mov         eax,dword ptr [esp+24h]  
	004191CD 50                   push        eax  
	004191CE C7 05 38 DB 45 00 FF FF FF FF mov         dword ptr ds:[45DB38h],0FFFFFFFFh  
	004191D8 C6 05 E9 B1 9A 01 01 mov         byte ptr ds:[19AB1E9h],1  
	004191DF E8 3C 68 C7 69       call        som_game_4230C0 (6A08FA20h)  
	*/
	BYTE ret = ((BYTE(__cdecl*)(DWORD,FLOAT,FLOAT,FLOAT,DWORD))0x4230C0)(_1,_2,_3,_4,_5);
	{
		//_1 is the som_game_menu modified record
		//_2,_3,_4 maybe for equip screen offset?
		//_5 is esi
		//assert(_5==0x019aa978||_5==01d11c00);
		assert(_5==(DWORD)SOM::menupcs-0xA8);
		/*code likely deals with _2,_3,_4
		004231A3 8B 94 24 20 01 00 00 mov         edx,dword ptr [esp+120h]  
		004231AA 89 46 70             mov         dword ptr [esi+70h],eax  
		004231AD 8B 84 24 24 01 00 00 mov         eax,dword ptr [esp+124h]  
		004231B4 89 11                mov         dword ptr [ecx],edx  
		004231B6 8B 94 24 28 01 00 00 mov         edx,dword ptr [esp+128h]
		*/

		//00423126 8B B4 24 38 01 00 00 mov         esi,dword ptr [esp+138h]
		//...
		//004231D1 8B 4E 3C             mov         ecx,dword ptr [esi+3Ch] 
		//FLOAT *ecx = *(SOM::MDO**)(_5+0x3c);
		if(auto*ecx=*(SOM::MDO**)(_5+0x3c))
		{
			if(int did=EX::Joypads[0].JslDeviceID) //EXPERIMENTAL
			{
				SOM::motions.sixaxis_calibration = JslGetMotionState(~did);
				memset(som_hacks_inventory_accum,0x00,3*sizeof(float));
			}

			//50 is y axis?
			//004231EA D9 59 54             fstp        dword ptr [ecx+54h]  
			//004231ED 8B 56 3C             mov         edx,dword ptr [esi+3Ch]  
			//004231F0 C7 42 50 C2 B8 32 3E mov         dword ptr [edx+50h],3E32B8C2h  
			//NOTE: these overlap... maybe can be zeroed?
			som_game_iteminit(ecx->f+19,ecx->f+20);

			if(DDRAW::xr) //OpenXR?
			{
				ecx->f[8]+=SOM::uvw[1]; //face forward?

				//TODO: might want to lift up some?
				//TODO: scoot equip screen items over to the side

				for(int i=3;i-->0;)
				{
					//NOTE: som_hacks_SetTransform has to skirt around some
					//code added to better center items since it makes them
					//completely invisible

					ecx->f[13+i] = SOM::cam[i]+0.666f*SOM::pov[i]; //world space?			
				}
				ecx->f[13+1]-=0.1f;
			}
		}
	}

	return ret;
}

extern void __cdecl som_game_409440(DWORD event, DWORD *stack) //Shop
{
	//2022: take control of resources?
	SOM::Game::item_lock_section raii(SOM::Context::browsing_shop);
	
	//1CE1D02 is shop memory. Back to back.
	//1CE1EF6 is shop prices. Back to back.
	//1CE20EA is sell prices. Back to back.	
	//(1534B) (1500+name+kind+padding)
	//1ce26ea is sell prices Shop number 1. (e.g. Shop.Dat)
	//
	/*
	0043E2C5 8B 84 24 78 01 00 00 mov         eax,dword ptr [esp+178h]  
	0043E2CC 8D 0C 40             lea         ecx,[eax+eax*2]  
	0043E2CF C1 E1 08             shl         ecx,8  
	0043E2D2 2B C8                sub         ecx,eax  
	0043E2D4 03 CE                add         ecx,esi  
	0043E2D6 66 83 3C 4D EA 20 CE 01 00 cmp         word ptr [ecx*2+1CE20EAh],0  
	0043E2DF 8D 2C 4D EA 20 CE 01 lea         ebp,[ecx*2+1CE20EAh]  
	*/		
	//NOTE: [4] is 0xc. probably this code is shared
	SOM::Shop swap,&shop = SOM::L.shops[((BYTE*)*stack)[4]]; 
	using namespace som_game_menu;			
	enum{ cp=2*250*3 };
	if(ini_browse)
	{
		memcpy(&swap,&shop,cp);
		memset(&shop,0x00,cp);
		for(int i=0,j=0,k;i<N;i++)
		{
			k = ini_browse[i]; if(k<250) 
			{
				shop.stock[j] = swap.stock[k];
				shop.price[j] = swap.price[k];
				shop.price2[j] = swap.price2[k];
				j++;
			}
			else if(N<=250) j++;
		}			
	}

	//2021: waiting for swing model to clear?
	if(SOM::L.pcequip2) for(int i=8;i-->0;) 
	std::swap(SOM::L.pcequip[i],SOM::L.pcequip2_in_waiting[i]);
	{
		SOM::menupcs = (DWORD*)0x1d11ca8;
		som_game_menu::browsing_items_menu();
		((void(__cdecl*)(DWORD,DWORD*))0x409440)(event,stack);	
		SOM::menupcs = (DWORD*)0x1A5B4A8;
	}
	if(SOM::L.pcequip2) for(int i=8;i-->0;) 
	std::swap(SOM::L.pcequip[i],SOM::L.pcequip2_in_waiting[i]);

	if(ini_browse)		
	{
		for(int i=0,j=0,k;i<N;i++)
		{
			k = ini_browse[i]; if(k<250)
			{
				swap.stock[k] = shop.stock[j];
				j++;
			}
			else if(N<=250) j++;
		}
		memcpy(&shop,&swap,cp);
	}
	
	
	SOM::frame_is_missing();
}

extern bool som_game_410620_item_detected = false;
static BYTE __cdecl som_game_410620(SOM::Item *_1, DWORD _2)
{
	if(_1-SOM::L.items>=250) return 0; //2024

	som_game_410620_item_detected = false;

	if(_1) //2020: KF2 style container?	
	for(int i=SOM::L.ai3_size;i-->0;)
	{
		//SKETCHY
		//man this nesting is really kind of crazy
		auto &ai = SOM::L.ai3[i];		
		//what does this mean? opened? (not really)
		int status = ai.c[0x7c];
		
		//can animation be checked?
		//have to hold open I guess
		auto mdl = (SOM::MDL*)ai[SOM::AI::mdl3];		
		if(!mdl) continue;
		//assuming not in unopened frame if so
		if(mdl->d>2) switch(status)
		{
		default: continue;

		case 4: //finished opening?

			//let item be taken before fully opened?
			//if(mdl->d<mdl->running_time(mdl->c)-1)
			//break;
			continue;

		case 5: //waiting to close?
			
			//if(ai.c[0x7e]) continue;

			if(5!=mdl->animation_id()) continue;
			
			break;
		}

		//deja vu (where have I done this before?)
		//don't actually need this, note fe is for
		//event counter based locks
		//BYTE key = ai.c[0x24];
		//if(key<0xfa) continue;
		int prm = ai[SOM::AI::object];
		WORD pr2 = SOM::L.obj_prm_file[prm].s[18];
		auto &prf = SOM::L.obj_pr2_file[pr2];		
		//I don't think the other container types
		//can be done here
		if(prf.s[41]!=0x15) continue;

		//EXTENSION? 
		//almost like player_character_radius3?
		static const float tol = 0.01f;

		//note: negative radius/height doesn't work
		float hit[3],_[3];
		if(((BYTE(*)(FLOAT*,FLOAT,FLOAT,DWORD,DWORD,FLOAT*,FLOAT*))0x40DFF0)
		(&_1->x,tol,tol,i,0,hit,_))
		{
			//what about vertical? what is item height?
			//assuming object is upright?
			 
			float y = ai[SOM::AI::y3];
			float h = ai[SOM::AI::height3];					
			if(h<0) y-=h=-h;
			//NOTE: item may sit exactly on container bottom
			//like KF2's secret comparments
			y-=tol; h-=tol;			
			if(_1->y<y-tol||_1->y>y+h-tol) continue;

			//THIS GENERATES A LOT OF NONSENSE ASSEMBLY
			if(som_game_dist2(hit[0],hit[2],_1->x,_1->z)>tol)
			{
				som_game_410620_item_detected = true; //HACK

				//41060b has 2 bytes that must be knocked out
				//to forward the result
				return 0;		
			}
		}
	}

	//2022: hide menu a la KF? more notes below
	BYTE &mute = SOM::L.sys_data_menu_sounds_suite;
	//YUCK: buttons need to retrieve this value
	char &swap = SOM::L.bgm_file_name[MAX_PATH-1];
	swap = mute;
	if(!SOM::L.on_off[3]) mute = 0;

	//2022: take control of resources?
	SOM::Game::item_lock_section raii(SOM::Context::taking_item);
	
	SOM::menupcs = (DWORD*)0x556690;	
	BYTE ret = ((BYTE(__cdecl*)(SOM::Item*,DWORD))0x410620)(_1,_2);
	SOM::menupcs = (DWORD*)0x1A5B4A8;

	SOM::frame_is_missing();

	//som_game_examine_421f10/421f10 below disable menus and the
	//up/down buttons are disabled with som_hacks_GetDeviceState 
	mute = swap;

	//2020: som_game_reprogram knocks out the caller code so the 
	//super caller above it can receive the KF2 item result when
	//the item is inside of a container
	//return ret;
	{
				return _1?1:ret;
	}
}
void __cdecl som_game_examine_421f10(int _1, int _2, int _3)
{
	if(SOM::L.on_off[3]) //2022: take item menu?
	((void(__cdecl*)(int,int,int))0x421f10)(_1,_2,_3);
}
void __cdecl som_game_examine_422ab0(HDC _1, char *_2, int _3, int _4)
{
	if(SOM::L.on_off[3]) //2022: take item menu?
	((void(__cdecl*)(HDC,char*,int,int))0x422ab0)(_1,_2,_3,_4);
}

#pragma runtime_checks("",off) //mov eax,0CCCCCCCCh  
static void som_game_41B7A8() //BUG-FIX
{
	DWORD _ecx;
	//This is checking if the body-equipment piece covers the arms/legs.
	//Problem is it doesn't check for if there is no equipment available.
	//And to a lesser extent, it takes the PR2 index validity for granted.
	/*
	0041B7A8 66 8B 81 F0 6F 56 00 mov         ax,word ptr [ecx+566FF0h]  
	0041B7AF 8D 14 80             lea         edx,[eax+eax*4]  
	0041B7B2 8D 04 50             lea         eax,[eax+edx*2]  
	0041B7B5 80 3C C5 88 04 58 00 04 cmp         byte ptr [eax*8+580488h],4
	0041B7BD 75 06                jne         0041B7C5
	->
	call som_game_41B7A8
	0041B7BD 75 06                jne         0041B7C5
	*/
	__asm
	{
		mov _ecx,ecx				
	}

	if(_ecx<250*336)
	{
		WORD pr2 = *(WORD*)(som_game_menu::prm_sorted+_ecx);
		if(pr2<250)
		{
			BYTE *body = SOM::L.item_pr2_file[pr2].uc;

			if(2==body[62]&&4==body[72]) 
			{
				//TODO? Explain to the player that the body armor
				//needs to be removed in order to equip arms/legs.

				//Set up "inc ecx" below to rollover from -1 to 0.
				_ecx = 0xFFFFFFFF; assert(0==_ecx+1);
			}
		}
	}

	__asm
	{	
		//Set the zero flag ahead of jne?
		mov ecx,_ecx
		inc ecx 		
	}				
}
static void som_game_43D229()
{
	DWORD _eax,_ecx;
	/*
	0043D229 80 38 00             cmp         byte ptr [eax],0  
	0043D22C 74 01                je          0043D22F  
	0043D22E 41                   inc         ecx 
	->
	call som_game_43D229	
	*/
	__asm
	{
		mov _eax,eax
		mov _ecx,ecx				
	}
	int i = (_eax-(DWORD)som_game_menu::inv_sorted)/2;
	_ecx+=som_game_menu::_appraiser_filter(i);
	__asm
	{	
		mov eax,_eax
		mov ecx,_ecx
		mov edx,0		
	}
}
static void som_game_43EBC7()
{	
	DWORD _eax,_ecx,_edx;
	/*
	0043EBC7 80 3A 00             cmp         byte ptr [edx],0  
	0043EBCA 0F 84 39 01 00 00    je          0043ED09  
	0043EBD0 3B C8                cmp         ecx,eax  
	0043EBD2 0F 8C 28 01 00 00    jl          0043ED00  
	0043EBD8 8D 50 04             lea         edx,[eax+4]  
	0043EBDB 3B CA                cmp         ecx,edx
	0043EBDD 0F 8D 1D 01 00 00    jge         0043ED00
	->
	call som_game_43EBC7
	je          0043ED09  
	jge         0043ED00
	*/
	__asm
	{
		mov _eax,eax
		mov _ecx,ecx
		mov _edx,edx
	}
	int i = (_edx-(DWORD)som_game_menu::inv_sorted)/2;
	int a = som_game_menu::_appraiser_filter(i);
	int b = a;
	if(a!=0) 
	b+=_ecx<_eax||_ecx>=_eax+4?-1:+1;
	__asm
	{	
		//cmp a,b
		mov eax,a
		mov ecx,b
		cmp eax,ecx 

		mov eax,_eax
		mov ecx,_ecx
		mov edx,_edx					
	}				
}
#pragma runtime_checks("",restore) 

static DWORD __cdecl som_game_44fb13_60fps_etc(DWORD *p, DWORD s, DWORD, FILE *f)
{
	som_game_fread(p,s,1,f);

	bool enemy = (BYTE*)p==SOM::L.enemy_pr2_file;

	BYTE *e = (BYTE*)p+s; if(p+1024>(void*)e) 
	{	
		if(EX::ok_generic_failure(L"%s param file is too small",enemy?"enemy":"npc"))
		{
			EX::is_needed_to_shutdown_immediately(1);
		}
	}

	BYTE **edata = SOM::L.enemy_pr2_data; //2024
	BYTE **ndata = SOM::L.NPC_pr2_data;
	BYTE **data = enemy?edata:ndata; 
	
	std::vector<BYTE*> v; //REMOVE ME (IN FAVOR OF data) (2024)
	
	v.push_back(e); e-=enemy?564:384;
	
	for(int i=1024;i-->0;) if(p[i])
	{
		BYTE *q = (BYTE*)p+p[i];

		if(q>e) //todo: log errors
		{
			p[i] = 0; continue;
		}

		v.push_back(q);

		data[i] = q; //2024: saving for som_game_reload_enemy_npc_pr2_files 
	}
	else data[i] = 0;
	
	//NOTE: do this unconditionally
	extern void som_game_60fps_npc_etc(std::vector<BYTE*>&); //workshop.cpp
	som_game_60fps_npc_etc(v);
	return 1;
}
static void som_game_reload_enemy_npc_pr2_files()
{
	//HACK: som_game_60fps_npc_etc assumes these are done in order
	delete[] (BYTE*)SOM::L.enemy_pr2_file; SOM::L.enemy_pr2_file = 0;	
	delete[] (BYTE*)SOM::L.NPC_pr2_file; SOM::L.NPC_pr2_file = 0;
	if(FILE *f=som_game_fopen(SOMEX_(B)"\\param\\enemy.pr2","rb"))
	{
		som_game_fseek(f,0,SEEK_END); int sz = som_game_ftell(f);
		som_game_fseek(f,0,SEEK_SET);
		SOM::L.enemy_pr2_file = new BYTE[sz];
		som_game_44fb13_60fps_etc((DWORD*)(BYTE*)SOM::L.enemy_pr2_file,sz,1,f);
		som_game_fclose(f);
	}
	if(FILE *f=som_game_fopen(SOMEX_(B)"\\param\\npc.pr2","rb"))
	{
		som_game_fseek(f,0,SEEK_END); int sz = som_game_ftell(f);
		som_game_fseek(f,0,SEEK_SET);

		SOM::L.NPC_pr2_file = new BYTE[sz];
		som_game_44fb13_60fps_etc((DWORD*)(BYTE*)SOM::L.NPC_pr2_file,sz,1,f);
		som_game_fclose(f);
	}
}

//2020: this code extends the enemy limit and opens the door to future
//modifications to save files
struct som_game_x
{
	//this is same as the original enemy data in excess of (minus) 128
	std::vector<WORD> x_enemy;
};
static som_game_x(*som_game)[64] = 0;
static void __cdecl som_game_42c8f0()
{
	//this subroutine clears/defaults the save games data

	((void(__cdecl*)())0x42c8f0)(); 
	
	delete[] som_game; som_game = 0;
}
extern bool __cdecl som_game_42cf40()
{	
	//this routine loads the save state
	//or does nothing if beginning anew

	//if(1&&EX::debug) //PIGGYBACKING
	{
			//TODO: more sanitization

		//2020: cut down on iteration?
		//2022: MapComp_407c80 now clips these arrays
		//in the MPX file to reduce memory on loading
		//but this can stay since these reverse loops
		//should early out on new MPX files
		int i; for(i=SOM::L.ai_size;i-->0;)
		{
			if(SOM::L.ai[i].i[0x8F]) break;
		}
		SOM::L.ai_size = i+1;
		for(i=SOM::L.ai2_size;i-->0;)
		{
			if(SOM::L.ai2[i].c[0x79]) break;
		}
		SOM::L.ai2_size = i+1;
		for(i=SOM::L.ai3_size;i-->0;)
		{
			if(SOM::L.ai3[i].c[0x79]) break;
		}
		SOM::L.ai3_size = ++i; //i+1
		{
			extern int *som_clipc_objectstack;
			int *e = som_clipc_objectstack+i;
			while(i-->0) som_clipc_objectstack[i] = i;
			auto *ai3 = &SOM::L.ai3[0];
			std::sort(som_clipc_objectstack,e,[ai3](int a, int b)->bool 
			{
				float ay = ai3[a].f[SOM::AI::_xyz3+1]+ai3[a][SOM::AI::height3];
				float by = ai3[b].f[SOM::AI::_xyz3+1]+ai3[b][SOM::AI::height3];
				return ay<by;
			});
		}
	}

	BYTE ret; if(som_game)
	{
		//this subroutine initializes the map state vis-a-vis
		//save games data

		som_game_x &g = (*som_game)[SOM::mpx];
	
		DWORD &sz = SOM::L.ai_size, swap = sz;
		sz = min(128,sz);
	
		ret = ((BYTE(__cdecl*)())0x42cf40)();

		if(!g.x_enemy.empty()) //extra monsters?
		{
			int i = sz, j = g.x_enemy.size();

			sz = swap; for(;swap>DWORD(i+j);swap--)
			{
				SOM::L.ai[swap][SOM::AI::instance] = 0;
			}
			j = min((int)swap-i,j); while(j-->0)
			{
				SOM::Struct<149> &ai = SOM::L.ai[i+j];

				//note: &1 isn't be required but it's how
				//42cf40 does it for sake of illustration
				if(g.x_enemy[j]&1)
				ai[SOM::AI::instance] = g.x_enemy[j]>>8&0xff;
			}
		}
	}
	else ret = ((BYTE(__cdecl*)())0x42cf40)();
	
	if(ret)
	{
		//TRYING TO MAKE MORE ROBUST FOR DEVELOPMENT//

		//410750 crashes if there's no MDO for an item
		auto &items = 
		*(SOM::Items*)&SOM::L.mapsdata_items[SOM::mpx];
		for(int i=256;i-->0;)
		{
			auto &it = items[i];
			if(it.nonempty&&!SOM::L.items_MDO_table[it.mdo][0])
			{
				//ISSUE WARNING
				int todolist[SOMEX_VNUMBER<=0x1020704UL];

				if(it.item<256) //try to repair?
				{
					WORD prm = SOM::L.MPX_items[it.item].s[14];
					if(prm<256)
					{
						it.prm = (BYTE)prm;
						it.mdo = (BYTE)SOM::L.item_pr2_file[prm].s[0];
						if(SOM::L.items_MDO_table[it.mdo][0])
						continue;
					}
				}				
				it.nonempty = false;
			}
		}
	}

	return ret!=0;
}
static bool __cdecl som_game_42d300()
{
	//this subroutine flushes the current map data to the
	//save game record prior to saving or unloading a map

 	assert((unsigned)SOM::mpx<64);
	if(!som_game) 
	(void*&)som_game = new som_game_x[64](); //C++
	som_game_x &g = (*som_game)[SOM::mpx];

	DWORD &sz = SOM::L.ai_size;
	g.x_enemy.assign(max(0,(int)sz-128),0);
	sz = min(128,sz);

	bool ret = ((bool(__cdecl*)())0x42d300)();

	if(!g.x_enemy.empty()) //extra monsters?
	{
		int i = sz, j = g.x_enemy.size();

		sz+=j; while(j-->0)
		{
			SOM::Struct<149> &ai = SOM::L.ai[i+j];
			
			//note: this is a floating point value but for
			//this subroutine it's a boolean
			if(0!=(BOOL&)ai[SOM::AI::_save])			
			g.x_enemy[j] = (WORD)ai[SOM::AI::instance]<<8|1;
		}
	}

	return ret;
}
extern DWORD som_game_version = 0; //up to 255
static DWORD __cdecl som_game_44fb13_y(DWORD *_1, DWORD _2, DWORD _3, FILE *_4)
{
	//this is just probing for the elapsed-time to show in the
	//game selection menus

	DWORD ret = som_game_fread(_1,4,1,_4);
	if(!ret||*_1!=*(DWORD*)"SOM=") //old style save file?
	return ret;
	char buf[MAX_PATH*4];
	som_game_fgets(buf,sizeof(buf),_4); 
	DWORD rem = 4-som_game_ftell(_4)%4;
	som_game_fread(&rem,rem,1,_4);
	return som_game_fread(_1,4,1,_4);
}
static DWORD __cdecl som_game_44fb13_x(DWORD *_1, DWORD _2, DWORD _3, FILE *_4)
{
	//som_game_fread
	auto f = (DWORD(__cdecl*)(void*,DWORD,DWORD,FILE*))0x44fb13;

	char buf[MAX_PATH*4];

	if(_2==4) //reading elapsed-time at top of file
	{		
		som_game_resetting = true;

		//2021: keeping these synchronized seems to make sense
		//som_game_highlight_load_game(som_game_save_xx,false);	
		som_game_highlight_save_game(som_game_save_xx,false);	

		//assert(!som_game); //not cleared?
		delete[] som_game; som_game = 0;

		som_game_version = 2000;

		DWORD ret = f(_1,4,1,_4);

		if(!ret||*_1!=*(DWORD*)"SOM=") //old style save file?
		{
			SOM::et = *_1; //2020: SHOULDN'T REQUIRE THIS NOW

			return ret;
		}

		som_game_fgets(buf,sizeof(buf),_4);

		(void*&)som_game = new som_game_x[64](); //C++

		//align file on version field
		//note: this version refers more to map data than other
		//kinds of data
		DWORD rem = 4-som_game_ftell(_4)%4;
		som_game_version = 0;
		f(&som_game_version,rem,1,_4); //LITTLE-ENDIAN

		if((unsigned)som_game_version>=0xff)
		{
			assert(0xff==som_game_version);

			som_game_version = 2000;
		}

		ret = f(_1,4,1,_4);

		SOM::et = *_1; //2020: SHOULDN'T REQUIRE THIS NOW

		return ret;
	}		
	if(som_game_version==2000) //old style save file? 
	{
		return f(_1,_2,_3,_4);
	}

	//it uses _3 instead of _2
	assert(_2==1&&_3==0x7a20);

	DWORD cmp, ver = som_game_version;

	//scan as if ver is 0
	*buf = '?'; do
	{
		cmp = som_game_ftell(_4);
		som_game_fgets(buf,sizeof(buf),_4);
		cmp = som_game_ftell(_4)-cmp;

		if(cmp)
		{
			f(&cmp,4,1,_4);

			if(*buf=='?')
			som_game_fseek(_4,cmp,SEEK_CUR);
		}
		else break;

	}while(*buf=='?');

	const char *er = 0;

	if(cmp<0x7a20)
	{
		if(cmp) goto error; return 0;
	}

	f(_1,0x7a20,1,_4);
	cmp-=0x7a20;

	auto map = *(unsigned char*)_1;

	if(map>=64) //prevent overflow bug?
	{
		er = "map index exceeds 64";
		goto error;
	}

	auto &g = (*som_game)[map];

	#define _0 0 //current/highest version

	//each version's data is appended to
	//the back of the map data
	for(DWORD v=0;cmp;v++)
	{
		if(v>ver||cmp<4)
		{
			assert(v<=ver&&cmp>=4);
			goto error;
		}

		DWORD chunk;
		f(&chunk,4,1,_4);
		cmp-=4;
		if(!chunk) continue;

		DWORD os = som_game_ftell(_4);

		assert(os%4==0); //true?

		switch(v)
		{	
		case _0: //ver 0

			g.x_enemy.assign(chunk/2,0);
			f(g.x_enemy.data(),chunk,1,_4);
			break;

		default: //incompatible?

			er = "exceeds map data version " EX_CSTRING(_0);
			goto error;
		}		

		if(chunk%4) chunk+=4-chunk%4;
		som_game_fseek(_4,os+chunk,SEEK_SET);
		if(chunk>cmp)
		{
			assert(chunk<=cmp);
			goto error;
		}
		else cmp-=chunk;
	}
	if(cmp)
	{
		er = "extra data may be lost";
		error:
		if(EX::ok_generic_failure(er?
		L"Save file version is newer than SomEx.dll (%hs)":L"Save file corrupt",er))
		{
			//note: this is forced on release builds
			EX::is_needed_to_shutdown_immediately();
		}
		som_game_fseek(_4,cmp,SEEK_CUR);
	}

	#undef _0

	return 0x7a20;
}
static DWORD __cdecl som_game_44ff9f_x(DWORD *_1, DWORD _2, DWORD _3, FILE *_4)
{
	//som_game_write
	auto f = (DWORD(__cdecl*)(const void*,DWORD,DWORD,FILE*))0x44ff9f;

	if(_2==4) //writing elapsed-time at top of file
	{
		som_game_highlight_save_game(som_game_save_xx,true);	

		//if(EX::INI::Bugfix()->do_fix_elapsed_time)
		SOM::L.et = SOM::et;

		//this is designed to be conservative so that save games
		//are compatible as long as they don't have any features
		//that can only be represented by a later version format

		int v = -1;
		if(som_game) for(int i=64;i-->0;)
		{
			auto &g = (*som_game)[i];
			if(!g.x_enemy.empty()) v = max(0,v);
		}

		som_game_version = v==-1?2000:(unsigned)v;

		//this extension is just in case anyone insists on being
		//able to use their save files with unmodified SOM games
		if(som_game_version!=2000
		||!EX::INI::Player()->do_2000_compatible_player_file)
		{
			const wchar_t *som = Sompaste->get(L".SOM");
			const char *utf8; int len = wcslen(som);
			EX::Decode(som,&utf8,&len);

			//this makes the save file look like a SOM file that
			//redirects to the SOM file in use when it was saved
			//it's also the way it's detected as a modified save
			//file
			f("SOM=",4,1,_4); 
			f(utf8,len+1,1,_4);
			//this is just so fgets can be used without reading
			//past it
			som_game_fputc('\n',_4);

			//align file on version field
			//note: this version refers more to map data than other
			//kinds of data
			DWORD rem = 4-som_game_ftell(_4)%4;
			//don't write/truncate 2000
			//f(&som_game_version,rem,1,_4);
			if(v<0) v = 0xFF;f(&v,rem,1,_4); //LITTLE-ENDIAN

			assert(0==som_game_ftell(_4)%4);
		}
		return f(_1,4,1,_4);
	}
	if(som_game_version==2000) //old style save file? 
	{
		return f(_1,_2,_3,_4);
	}

	//it uses _3 instead of _2
	assert(_2==0x7a20&&_3==1);

	DWORD ver = som_game_version;
		
	auto map = *(unsigned char*)_1;
	assert(map<64);
	auto &g = (*som_game)[map];

	//NOTE: all sections should begin like
	//this, but for now there is just this
	//one section. since it uses filenames
	//future section names cannot be legal
	//filenames
	char buf[16];
	int len = sprintf(buf,"%02d.mpx",map)+1; 
	int rem = 4-len%4;
	while(rem-->1)
	buf[len++] = '\0';
	buf[len++] = '\n';
	f(buf,len,1,_4);

	auto top = som_game_ftell(_4);
	{
		assert(0==top%4);

		//this will be filled in below via fseek
		f(&(len=0),4,1,_4);

		f(_1,_2,_3,_4); //0x7a20

		//this is illustrative of how to write more 
		//than one version for in the future
		for(int w=-1,v=0;v<=0;v++)
		{
			switch(v)
			{
			default: len = 0; break;
			case 0: len = 2*g.x_enemy.size(); break;
			}
			if(!len) continue;

			//YUCK: output zero placeholders
			while(++w<v) f(&(len=0),rem,1,_4); 

			f(&len,4,1,_4);

			switch(v)
			{
			case 0: f(g.x_enemy.data(),len,1,_4); break;
			}

			if(4!=(rem=4-len%4)) f(&(len=0),rem,1,_4); 

			assert(0==som_game_ftell(_4)%4);
		}
	}
	auto end = som_game_ftell(_4);

	som_game_fseek(_4,top,SEEK_SET);
	len = end-top-4;
	f(&len,4,1,_4);
	som_game_fseek(_4,end,SEEK_SET);

	assert(0==som_game_ftell(_4)%4);

	return 0x7a20;
}
static int __cdecl som_game_44f5b1(FILE *f)
{
	if(int ret=((int(__cdecl*)(FILE*))0x44f5b1)(f))
	return ret;

	EX::INI::Player pc;

	const wchar_t *name = SOM::Game::save(0,0);
	const wchar_t *path = PathFindFileName(name);
	long cat = path-name;
	wchar_t move[MAX_PATH]; wmemcpy(move,name,cat);
	wchar_t *rename = move+cat;

	const wchar_t *ext = L"som";
	if(som_game_version!=2000||!pc->do_2000_compatible_player_file)
	{
		const wchar_t *ext2 = pc->player_file_extension;
		if(SOM::retail&&*ext2) ext = ext2;

		int h = SOM::et/3600, m = SOM::et%3600/60;

		int e = SOM::L.pcstatus[SOM::PC::pts];

		const wchar_t *title = 
		EX::need_unicode_equivalent(932,&SOM::L.mpx->pointer->c[SOM::MPX::title]);

		cat = _snwprintf(rename,MAX_PATH-cat,L"%02d.%dh%02dm.%de.%s\b",som_game_save_xx,h,m,e,title);

		//Sompaste->path
		for(int i=0;i<cat;i++) switch(rename[i])
		{
		case '/': case '\\': 
		case '*': case '?':
		case '<': case '>':
		case '|': case ':': case '"': //what else?

			rename[i] = '~';
		}
	}
	else
	{
		ext =  L"dat"; cat = _snwprintf(rename,MAX_PATH-cat,L"%02d\b",som_game_save_xx);
	}

	bool cut = '\b'!=rename[--cat];	
	long len = wcslen(ext);
	long trunc = 1+len-(MAX_PATH-(path-name+cat));
	cat-=max(0,trunc); if(cut)
	{
		cat-=3;
		for(int i=3;i-->0;) rename[cat++] = '.';
	}
	rename[cat++] = '.';
	for(int i=0;i<=len;i++) rename[cat++] = ext[i];

	Sompaste->path(move);
	if(wcscmp(name,move)) while(!MoveFile(name,move)) 
	if(!DeleteFile(move)) break; return 0;
}
static BYTE __cdecl som_game_403670_params(char *buf)
{
	//NOTE: I think SOM_EX.exe is adding the debug map
	//to the commandline when loading "SOM" save files
	if(GetEnvironmentVariableW(L"LOAD",0,0)>1)
	{
		//SOM::L.startup = -1; //TESTING
		//SOM::L.startup = 9; //DITTO

		*buf = '\0'; return 0;
	}
	//NOTE: 0 is success 1 shuts down instantly, but I
	//guess that might make sense for SOM_SYS... maybe
	//1 is returned if the commandline can't be parsed
	return ((BYTE(__cdecl*)(char*))0x403670)(buf);
}
static DWORD __cdecl som_game_42bfc0_intros()
{
	DWORD ret; if(GetEnvironmentVariableW(L"LOAD",0,0)>1)
	{
		//the returned value here is interpreted
		//as selected from the "Continue" screen
		
		//this is just for highlighting the slot
		//after returning to the load/save menus
		ret = _wtoi(PathFindFileName(Sompaste->get(L"LOAD")));
	}
	else //ret = ((DWORD(__cdecl*)())0x42bfc0)();
	{
		if(SOM::retail) //preload starter map?
		{
			EX::INI::Option op;
			EX::INI::Detail dt;
			if(dt->start_mode==1&&op->do_start)
			{
				//2022: start loading first map?
				SOM::queue_up_game_map(SOM::L.debug_map);
			}			
		}

		ret = ((DWORD(__cdecl*)())0x42bfc0)();
	}

	/*2021: I've made loading change the save
	//slot... this was crashing on "LOAD" too
		//
	//it seems loading a game doesn't change
	//the save game menu. but at start up it
	//should?
	if(ret<100) som_game_highlight_save_game(ret,false);*/

	//HACK: covering Continue screen?
	som_game_saves.close(); return ret;
}
static BYTE __cdecl som_game_640x480_4487f0(DWORD _1, DWORD _2, DWORD _3, DWORD _4, DWORD, DWORD)
{
	//NOTE: som_state_slides422AB0 adjusts the text
	return ((BYTE(__cdecl*)(DWORD,DWORD,DWORD,DWORD,DWORD,DWORD))0x4487f0) //2022
	(_1,_2,_3,_4,SOM::width,SOM::height);
}
static BYTE __cdecl som_game_640x480_44a160(DWORD *_1, DWORD _2, DWORD _3) //2022
{
	_1[2] = SOM::width; _1[3] = SOM::height;
	return ((BYTE(__cdecl*)(DWORD*,DWORD,DWORD))0x44a160)(_1,_2,_3);
}
extern void som_game_ai_resize(DWORD lim) //som.MPX.cpp
{
	//NOTE: at this stage SOM::L.ai_size is 0 so that it
	//can count up with each enemy read
	assert(!SOM::L.ai_size);
	void* &ptr = (void*&)SOM::L.ai;	

	if((DWORD)ptr==0x4C77C8)
	{
		if(lim<=128) return;

		ptr = malloc(lim*0x254);
	}
	else ptr = realloc(ptr,lim*0x254); /*return 1;
}
extern void som_game_kfii_reprogram() //EXPERIMENTAL
{
	//BACKGROUND: I wouldn't normally go to this length other
	//than porting KING'S FIELD II requires more spawn points
	//maybe though in lieu of layers increasing these numbers
	//is worth the effort
	
	DWORD &lim = SOM::L.ai_size; 	
	void* &ptr = (void*&)SOM::L.ai;
	if(lim<=128) return;

	if((DWORD)ptr==0x4C77C8)
	ptr = malloc(lim*0x254);
	else ptr = realloc(ptr,lim*0x254);*/

	DWORD mem = (DWORD)ptr;

	//HACK: can just use 512 (lim) permanently? but this lets
	//it be fit to the MPX numbers
	DWORD old = 0;
	DWORD text = 0x401000, text_s = SOM::text->info.RegionSize; 
	VirtualProtect((void*)text,text_s,PAGE_EXECUTE_READWRITE,&old); 
	//assert(old!=PAGE_EXECUTE_READWRITE);

	//bool one_off = !ptr; if(!ptr) 
	{
		//00405DEB 3D 80 00 00 00       cmp         eax,80h
		*(DWORD*)0x0405DEC = lim;

	//	if(!ptr) ptr = new SOM::Struct<149>[lim];
	}

	//if(one_off)
	{
		//2020: increase enemy limit for KF2
		//NOTE: member addressing may be offset from 4C77C8
		//ai_size (5541CC) is accessed by the following
		//these will all need to be checked out
		//0x405c4a zeroes?
		//0x405de0
		//0x405e37
		//0x405fdb
		//0x4064e1
		//0x4064e7
		//0x40659e zeroes?
		//0x4065b1
		//0x40669d
		//0x4082c8
		//0x408318 //!		
		//0x408378
		//0x408e70
		//0x4090fe
		//0x40927c
		//0x426fdc
		//0x42706d
		//0x42a20f
		//0x42a28f
		//0x42cf85
		//0x42d36a
		//0x42d393
		//0x43fd6c
		//0x43fdc1
		//
		//4C77C8 by the following
		//00405BBB B9 80 4A 00 00       mov         ecx,4A80h		 
		//00405BC0 BF C8 77 4C 00       mov         edi,4C77C8h
		*(DWORD*)0x405BBC = 0x95*lim;
		*(DWORD*)0x405BC1 = mem;
		/*testing enemy_pr2_data overflow
		//00405CC5 3D C8 77 4C 00       cmp         eax,4C77C8h
		*(DWORD*)0x405CC6 = mem*/;
		//00405E2F 8D 14 8D C8 77 4C 00 lea         edx,[ecx*4+4C77C8h]	 	 
		*(DWORD*)0x405E32 = mem;
		//00405FF6 8D 2C 85 C8 77 4C 00 lea         ebp,[eax*4+4C77C8h]
		*(DWORD*)0x405FF9 = mem;
		//00406532 BE 34 78 4C 00       mov         esi,4C7834h 
		*(DWORD*)0x406533 = mem+(0x834-0x7c8);
		//00406585 B9 80 4A 00 00       mov         ecx,4A80h
		//0040658A BF C8 77 4C 00       mov         edi,4C77C8h
		*(DWORD*)0x406586 = 0x95*lim;
		*(DWORD*)0x40658B = mem;
		//004066DF 8D 34 85 C8 77 4C 00 lea         esi,[eax*4+4C77C8h]
		*(DWORD*)0x4066E2 = mem;
		//004068B1 8D 34 85 C8 77 4C 00 lea         esi,[eax*4+4C77C8h]
		*(DWORD*)0x4068B4 = mem;
		//00406AC2 8D 2C 85 C8 77 4C 00 lea         ebp,[eax*4+4C77C8h]
		*(DWORD*)0x406AC5 = mem;
		//004080AC BE 34 78 4C 00       mov         esi,4C7834h
		*(DWORD*)0x4080AD = mem+(0x834-0x7c8);
		//004082D9 8D 0C 85 C8 77 4C 00 lea         ecx,[eax*4+4C77C8h]
		*(DWORD*)0x4082DC = mem;
		//00408393 8D 04 85 C8 77 4C 00 lea         eax,[eax*4+4C77C8h]
		*(DWORD*)0x408396 = mem;
		//00408E8D 8D 34 85 C8 77 4C 00 lea         esi,[eax*4+4C77C8h]
		*(DWORD*)0x408E90 = mem;
		//00409125 8D 04 85 C8 77 4C 00 lea         eax,[eax*4+4C77C8h]
		*(DWORD*)0x409128 = mem;
		//0040929A 8D 3C 85 C8 77 4C 00 lea         edi,[eax*4+4C77C8h]
		*(DWORD*)0x40929D = mem;

		//MORE IRREGULARS
		//004065D7 BF 10 78 4C 00       mov         edi,4C7810h 
		*(DWORD*)0x4065D8 = mem+(0x810-0x7c8);
		//004068A0 8B 14 85 30 78 4C 00 mov         edx,dword ptr [eax*4+4C7830h]
		*(DWORD*)0x4068A3 = mem+(0x830-0x7c8);
		//004099FB 8D 0C 85 10 78 4C 00 lea         ecx,[eax*4+4C7810h]
		*(DWORD*)0x4099FE = mem+(0x810-0x7c8);
		//0042CFA6 B9 FC 77 4C 00       mov         ecx,4C77FCh 
		*(DWORD*)0x42CFA7 = mem+(0x7fc-0x7c8);
		//0042D37C BE FC 77 4C 00       mov         esi,4C77FCh
		*(DWORD*)0x42d37d = mem+(0x7fc-0x7c8);
		//00431A27 B9 10 78 4C 00       mov         ecx,4C7810h
		*(DWORD*)0x431a28 = mem+(0x810-0x7c8);
		//00431CB3 D9 80 10 78 4C 00    fld         dword ptr [eax+4C7810h]
		//00431CC8 D8 80 14 78 4C 00    fadd        dword ptr [eax+4C7814h]
		//00431CD5 D9 80 18 78 4C 00    fld         dword ptr [eax+4C7818h]
		*(DWORD*)0x431cb5 = mem+(0x810-0x7c8);
		*(DWORD*)0x431cca = mem+(0x814-0x7c8);
		*(DWORD*)0x431cd7 = mem+(0x818-0x7c8);
		//0043FD77 BF 18 78 4C 00       mov         edi,4C7818h
		*(DWORD*)0x43fd78 = mem+(0x818-0x7c8);
		//0043FE22 66 8B 04 95 E8 77 4C 00 mov         ax,word ptr [edx*4+4C77E8h]  
		*(DWORD*)0x43fe26 = mem+(0x7e8-0x7c8);

		//0040687E C7 04 85 04 7A 4C 00 01 00 00 00 mov         dword ptr [eax*4+4C7A04h],1  
		*(DWORD*)0x406881 = mem+(0xa04-0x7c8);
		//00407B4F BD 00 7A 4C 00       mov         ebp,4C7A00h  
		*(DWORD*)0x407b50 = mem+(0xa00-0x7c8);

		//warping enemy (having issues??)
		//
		// NOTE: it works with more complete enemy profiles/models than 
		// my KF2 project currently has. I guess I will find out why???
		//
		//00408211 BE 08 7A 4C 00       mov         esi,4C7A08h  
		*(DWORD*)0x408212 = mem+(0xa08-0x7c8);
		//3? (offscreen entry)
		//00408329 8B 0C 85 04 7A 4C 0040454900 mov         ecx,dword ptr [eax*4+4C7A04h]  
		//00408330 83 F9 03             cmp         ecx,3
		//00408333 8D 04 85 C8 77 4C 00 lea         eax,[eax*4+4C77C8h]
		*(DWORD*)0x40832c = mem+(0xa04-0x7c8);		
		*(DWORD*)0x408336 = mem;
		//00408389 8B 0C 85 04 7A 4C 00 mov         ecx,dword ptr [eax*4+4C7A04h]  
		*(DWORD*)0x40838c = mem+(0xa04-0x7c8);

		//0040911B 8B 14 85 04 7A 4C 00 mov         edx,dword ptr [eax*4+4C7A04h]  
		*(DWORD*)0x40911e = mem+(0xa04-0x7c8);
		//0040B2B5 BB F8 79 4C 00       mov         ebx,4C79F8h  
		*(DWORD*)0x40b2b6 = mem+(0x9f8-0x7c8);
		//0040B6EF BB F4 79 4C 00       mov         ebx,4C79F4h  
		*(DWORD*)0x40b6f0 = mem+(0x9f4-0x7c8);
		//0040BC36 BB 00 7A 4C 00       mov         ebx,4C7A00h  
		*(DWORD*)0x40bc37 = mem+(0xa00-0x7c8); 
		//0040C07E BB 00 7A 4C 00       mov         ebx,4C7A00h
		*(DWORD*)0x40c07f = mem+(0xa00-0x7c8);		
		//00426FEB BE 00 7A 4C 00       mov         esi,4C7A00h  
		*(DWORD*)0x426fec = mem+(0xa00-0x7c8);
		//0042A21E BD 00 7A 4C 00       mov         ebp,4C7A00h  
		*(DWORD*)0x42a21f = mem+(0xa00-0x7c8);
		//00431CBC D9 80 F4 79 4C 00    fld         dword ptr [eax+4C79F4h]  
		*(DWORD*)0x431cbe = mem+(0x9f4-0x7c8);
	}

	DWORD end = mem+lim*0x254; //4C77C8+12a00=4DA1C8

	//if(1||one_off)
	{
		//STATIC LOOP TERMINATORS
		//disregarding ai_size?
		//00406556 81 C6 54 02 00 00    add         esi,254h  
		//0040655C 81 FE 34 A2 4D 00    cmp         esi,4DA234h
		*(DWORD*)0x40655e = end+(0x234-0x1C8);
		//00407BC9 81 C5 54 02 00 00    add         ebp,254h  
		//00407BCF 81 FD 00 A4 4D 00    cmp         ebp,4DA400h
		*(DWORD*)0x407bd1 = end+(0x400-0x1C8);
		//004081EA 81 C6 54 02 00 00    add         esi,254h  
		//004081F0 81 FE 34 A2 4D 00    cmp         esi,4DA234h
		*(DWORD*)0x4081f2 = end+(0x234-0x1C8);
		//004082AB 81 C6 54 02 00 00    add         esi,254h  
		//004082B1 81 FE 08 A4 4D 00    cmp         esi,4DA408h
		*(DWORD*)0x4082b3 = end+(0x408-0x1C8);
		//0040B340 81 C3 54 02 00 00    add         ebx,254h  
		//0040B346 81 FB F8 A3 4D 00    cmp         ebx,4DA3F8h  
		*(DWORD*)0x40b348 = end+(0x3F8-0x1C8);
		//0040B77A 81 C3 54 02 00 00    add         ebx,254h  
		//0040B780 81 FB F4 A3 4D 00    cmp         ebx,4DA3F4h 
		*(DWORD*)0x40b782 = end+(0x3F4-0x1C8);
		//0040BCC8 81 C3 54 02 00 00    add         ebx,254h  
		//0040BCCE 81 FB 00 A4 4D 00    cmp         ebx,4DA400h  
		*(DWORD*)0x40bcd0 = end+(0x400-0x1C8);
		//0040C120 81 C3 54 02 00 00    add         ebx,254h  
		//0040C126 81 FB 00 A4 4D 00    cmp         ebx,4DA400h
		*(DWORD*)0x40c128 = end+(0x400-0x1C8);
		//00431B11 81 C1 54 02 00 00    add         ecx,254h  
		//00431B1A 81 F9 10 A2 4D 00    cmp         ecx,4DA210h  
		*(DWORD*)0x431b1c = end+(0x210-0x1C8);

		/*CRASHING IN HERE ON EXIT
		00406532 BE 74 90 4F 05       mov         esi,54F9074h  
		00406537 8B 46 FC             mov         eax,dword ptr [esi-4]  
		0040653A 85 C0                test        eax,eax  
		0040653C 74 09                je          00406547  
		0040653E 50                   push        eax  
		0040653F E8 8C A4 03 00       call        004409D0
		*/
	}

	VirtualProtect((void*)text,text_s,old,&old);
}
static void __cdecl som_game_408400() //som_state_408400
{
	//0 resets counters, leafnums, and 128Bs at 555DC4
	((void(__cdecl*)())0x408400)(); 

	som_game_resetting = true;
}

extern void som_game_reprogram()
{	
	//NOTE: the following files also pertain to som_db.exe:
	//graphics: som.scene.cpp
	//gameplay: som.logic.cpp (2020) 
	//original: som.state.cpp (2010)
	//file formats specifics: various (MHM/MSM,LYT/MPX,MDL)
 
	EX::INI::Option op;

	//2020: sound optimization and reverb effect support
	if(op->do_sounds);
	{
		//this is an optimazation disabling som_db's own duplication system
		//0044AF87 C7 05 8C 9D D6 01 05 00 00 00 mov         dword ptr ds:[1D69D8Ch],5
		*(BYTE*)0x44AF8d = 1;
		//the new reverb effect is causing buffers to be dropped with or without
		//setting 44AF8d = 1;
		//this is designed to always play new sounds and forget about them after
		//(I don't know how this was never a problem  before... somehow more than
		//4 sounds were possible even though a memory of played sounds is stored
		//here... maybe it's let go before they finish playing?)
		//0044C1E7 89 04 16             mov         dword ptr [esi+edx],eax
		//0044B7E4 89 2C 16             mov         dword ptr [esi+edx],ebp 
		memset((void*)0x44C1E7,0x90,3);
		memset((void*)0x44B7E4,0x90,3);

		//WARNING
		//remove dependence on DSBCAPS_MUTE3DATMAXDISTANCE to limit number of 
		//duplicate sounds (8) to audible
		//NOTE: historically sounds are fixed in 3D space where they begin to
		//play. if that changes this will become a liability
		//0043F617 E8 E4 CA 00 00       call        0044C100		
		//0043F598 E8 53 C1 00 00       call        0044B6F0
		*(DWORD*)0x43F618 = (DWORD)som_game_44c100-0x43F61c;
		*(DWORD*)0x43F599 = (DWORD)som_game_44b6f0-0x43F59d;
		//NOTE: MIDI volume is unaffected by bgmVol
		//0043F6A3 E8 08 CE 00 00       call        0044C4B0
		//0043F8AB E8 00 CC 00 00       call        0044C4B0
		*(DWORD*)0x43F6A4 = (DWORD)som_game_44c4b0-0x43F6A8;
		*(DWORD*)0x43F8AC = (DWORD)som_game_44c4b0-0x43F8B0;
		//2022: closing BGM? (wav only) (cut fade effect)
		//0043F8FB E8 70 C9 00 00       call        0044C270		
		*(DWORD*)0x43F8Fc = (DWORD)som_game_44c270-0x43F900;

		//EXTENSION?
		//increase SetMaxDistance from 15?
		//"The SetMaxDistance method sets the maximum distance, which is the 
		//distance from the listener beyond which sounds in this buffer are 
		//no longer attenuated. "
		//
		// som_game_44c100 id using this distance to manually blend out the 
		// sounds since games aren't like real life, similar to how elements
		// fade in and out
		//
		//0043F312 68 00 00 70 41       push        41700000h
		//20 matches KF2 better, hearing sound effects far away can actually
		//be annoying
		//*(FLOAT*)0x43F313 = 15+10;
		*(FLOAT*)0x43F313 = 15+2.5;
		//it's kind of weird for large objects since the sound isn't
		//necessarily in their center?
		//decrease SetMinDistance from 2.5 (DirectSound default is 1)
		//0043F317 68 00 00 20 40       push        40200000h
		//*(FLOAT*)0x43F318 = 1;

		//sample_pitch_adjustment extension
		//0043F55E E8 CD 04 00 00       call        0043FA30
		//0043F5CE E8 5D 04 00 00       call        0043FA30
		*(DWORD*)0x43F55f = (DWORD)som_game_43fa30-0x43F563;
		*(DWORD*)0x43F5Cf = (DWORD)som_game_43fa30-0x43F5D3;

		//increase SetRolloffFactor from 1?
		//DirectSound seems unrealistic with unless min/max
		//distance are have appropriate values
		//
		// NOTE: I've gone with 5 only because sounds tend to
		// just cut out otherwise. I don't really understand 
		// it since SetMaxDistance is supposed to go down to
		// silence, or so I thought (the documentation may be
		// unclear or inaccurate)
		//
		// this could be changed to simulate open-air versus
		// a dungeon like space, but it's not on a per sound
		// basis. probably it should be 1
		//
		//0044B128 68 00 00 80 3F       push        3F800000h
		//*(FLOAT*)0x44B129/=2;
		//*(FLOAT*)0x44B129 = 2; //5 times attentuation? fake???
		*(FLOAT*)0x44B129 = 1; //1.25

		//BREAKING CHANGE
		//sound event?
		//sounds aren't played 3D for always on/use item on map events
		//but how else to play looping sounds? there's now system events
		//so events don't have to be bound to objects
		//
		// note: unbound sounds always played every frame so that the 
		// result is a cacophony, especially with reverb
		//
		//004099AE 8A 41 22             mov         al,byte ptr [ecx+22h]  		
		//004099B1 3C 20                cmp         al,20h  
		//004099B3 0F 84 8E 00 00 00    je          00409A47  
		//004099B9 3C 40                cmp         al,40h 
		*(BYTE*)0x4099b0 = 0x1f; //reject by class instead
		*(BYTE*)0x4099B2 = 0xfd; //systemwide
		*(BYTE*)0x4099Ba = 0xfe; //system

		//VARIOUS PITCH CODE IS BROKEN
		//object open/close pitch
		//0042BA52 33 D2                xor         edx,edx  
		//0042BA54 8A 57 62             mov         dl,byte ptr [edi+62h]
		//movsx edx,byte ptr [edi+62h]
		memcpy((void*)0x42BA52,"\x0f\xbe\x57\x62\x90",5);
		//0042BC56 33 C9                xor         ecx,ecx  
		//0042BC58 8A 4B 63             mov         cl,byte ptr [ebx+63h]  
		//movsx ecx,byte ptr [ebx+63h]  
		memcpy((void*)0x42BC56,"\x0f\xbe\x4b\x63\x90",5);
	}

	//extern void som_game_kfii_reprogram(); //OBSOLETE (2022)
	{
		//som_game_kfii_reprogram();
		//00411EBE E8 50 DC 03 00       call        0044FB13
	//	*(DWORD*)0x411EBF = (DWORD)som_game_44fb13_ai_resize-0x411EC3;
	}

	//TESTING: triple buffering? (really quadruple)
	//if(1!=EX_ARRAYSIZEOF(SOM::onflip_triple_time)) 
	{
		//this system is designed to deal with the chaos of triple/quadruple
		//buffering timing by averaging out the frame rate to smooth hiccups
		//004020BB FF 15 14 82 45 00    call        dword ptr ds:[458214h]  
		*(BYTE*)0x4020BB = 0xe8;
		*(DWORD*)0x4020BC = (DWORD)som_game_402070_timeGetTime-0x4020c0;
		*(BYTE*)0x4020c0 = 0x90;

		//004021A5 E8 16 02 00 00       call        004023C0
		*(DWORD*)0x4021A6 = (DWORD)som_game_4023c0-0x4021AA;
	}
	
	if(0&&EX::debug) //SEEMINGLY UNRELATED: not texture mapped
	{
		//this enables drawing transparent triangles at the end of
		//the main loop... they aren't texture mapped, so they can't
		//be text from above unless that comes after

		SOM::L.maybe_fairy_map_feature = 1;
		//disable alphablend?
		//004145B7 6A 01                push        1 
		*(BYTE*)0x4145B8 = 0; //testing

		//disable function call?
		//NOTE: this function appears to overwrite some MDO memory
		//that causes the unload routine to segfault on map change
		if(0) memset((void*)0x402563,0x9090,5); 
	}
	//else //YELLOW DEBUG TEXT???
	{
		//2020
		//
		// I think this is the code that generates the yellow bitmap font
		// that shows up as garbage at start up. I think it must be defuct
		// code that should've been removed. It requests a -16 font that is
		// hitting an assert in Ex.detours.cpp that doesn't know how to deal
		// with negative fonts right now. I think it means ignore line height
		// NOTE: it could be a secret "debug" system, but it's not required now
		//
		//0044A34D E8 8E 01 00 00       call        0044A4E0
		memset((void*)0x44A34D,0x90,5);
		//0044A3ED E8 EE 00 00 00       call        0044A4E0
		memset((void*)0x44A3ED,0x90,5);
	}

	/*BGM
	reading WAVE data size... ecx/1D69D60 is size...
	WAVEFORMATEX
	{
	1D69D4C is format/channels
	1D69D50 is sample rate
	1D69D58 is alignment/sample bits
	}
	1D69D60 is data size
	1D69D68 is 2*alignment*sample rate 
	0044C5B2 FF D7                call        edi  
	0044C5B4 3D 09 01 00 00       cmp         eax,109h
	0044C5BB 8B 15 58 9D D6 01    mov         edx,dword ptr ds:[1D69D58h]  
	0044C5C1 8B 4C 24 0C          mov         ecx,dword ptr [esp+0Ch]
	//seeking on loop
	0044C928 FF 15 F8 81 45 00    call        dword ptr ds:[4581F8h] 
	*/

	//SQUARING/STANDARDIZING MENUS (originally for VR)
	//Menu button loop
	//004253A2 E8 49 30 FF FF       call        004183F0
	*(DWORD*)0x4253A3 = (DWORD)som_game_4183F0-0x4253A7;
	//initialize main menu...
	//00418412 E8 B9 8E 00 00       call        004212D0
	*(DWORD*)0x418413 = (DWORD)som_game_4212D0-0x418417;
	//initialize continue game (load) menu
	//0042BFC8 E8 03 53 FF FF       call        004212D0
	*(DWORD*)0x42BFC9 = (DWORD)som_game_4212D0-0x42BFCD;
	//text event?
	//00423E5B E8 70 D4 FF FF       call        004212D0
	//shop? (shops?)
	//seen this for yes/no too???	
	//0043CE88 E8 43 44 FE FF       call        004212D0
	*(DWORD*)0x43CE89 = (DWORD)som_game_4212D0-0x43CE8D;
	//yes/no event? (yes/no text shadow is off)	
	//seen this for shops too???	
	//0040A8C9 E8 02 6A 01 00       call        004212D0
//	*(DWORD*)0x40A8CA = (DWORD)som_game_4212D0-0x40A8CE;
	//save event
	//0040A61A E8 B1 6C 01 00       call        004212D0  
	*(DWORD*)0x40A61B = (DWORD)som_game_4212D0-0x40A61F;
	//taking item
	//0041065B E8 70 0C 01 00       call        004212D0
	*(DWORD*)0x41065C = (DWORD)som_game_4212D0-0x410660;
	//this resets the menu resolution when canceling out
	//0041ECFB D9 9D A0 00 00 00    fstp        dword ptr [ebp+0A0h] 
	//0041ED09 D9 95 A4 00 00 00    fst         dword ptr [ebp+0A4h]
	{
		//copy 0041ED37 DD D8                fstp        st(0)
		*(WORD*)0x41ECFB = 0xD8DD;
		*(DWORD*)0x41ECFD = 0x90909090;
		memset((void*)(0x41ED09),0x90,6);
	}
	//this resets the menu resolution when changing BPP??
	//0041EA14 D9 9D A0 00 00 00    fstp        dword ptr [ebp+0A0h]
	//0041EA22 D9 95 A4 00 00 00    fst         dword ptr [ebp+0A4h]
	{
		//copy 0041ED37 DD D8                fstp        st(0)
		*(WORD*)0x41EA14 = 0xD8DD;
		*(DWORD*)0x41EA16 = 0x90909090;
		memset((void*)(0x41EA22),0x90,6);
	}

	/*
	0043D008 8B 0D E8 1D D1 01    mov         ecx,dword ptr ds:[1D11DE8h]  
	0043D00E 56                   push        esi  
	0043D00F FF 14 8D 68 EE 45 00 call        dword ptr [ecx*4+45EE68h]
	*/
	void **menus = (void**)0x045EE68;
	menus[0] = som_game_0_shop43d0c0;
	menus[1] = som_game_1_shop43d3b0;
	menus[2] = som_game_2_shop43e150; 
	menus[3] = som_game_3_shop43ea90; 
	/*
	004185F3 A1 64 AB 9A 01       mov         eax,dword ptr ds:[019AAB64h]  
	004185F8 C1 E2 02             shl         edx,2  
	004185FB 89 0D 48 AB 9A 01    mov         dword ptr ds:[19AAB48h],ecx  
	00418601 2B CA                sub         ecx,edx  
	00418603 68 78 A9 9A 01       push        19AA978h  
	00418608 89 35 4C AB 9A 01    mov         dword ptr ds:[19AAB4Ch],esi  
	0041860E 89 0D 54 AB 9A 01    mov         dword ptr ds:[19AAB54h],ecx  
	00418614 FF 14 85 00 DB 45 00 call        dword ptr [eax*4+45DB00h]  
	0041861B 83 C4 0C             add         esp,0Ch  
	*/
	menus = (void**)0x045DB00;
	menus[0] = som_game_0_menu418700;
	menus[1] = som_game_1_menu419020;
	menus[2] = som_game_2_menu419b60; 
	menus[3] = som_game_3_menu41a1e0; 
	menus[4] = som_game_4_menu41c370; 
	menus[5] = som_game_5_menu41cbd0; 
	menus[6] = som_game_6_menu41d760;
	menus[7] = som_game_7_menu41adf0;
	menus[8] = som_game_8_menu41bc00;
	menus[9] = som_game_9_menu41f880;
	menus[10] = som_game_10_menu4200a0;
	menus[11] = som_game_11_menu41e100;
	menus[12] = som_game_12_menu420910;
	menus[13] = som_game_13_menu421010;
	menus[14] = som_game_14_menuUNUSED;
	menus[15] = som_game_15_menuUNUSED;
	menus[16] = som_game_16_menuUNUSED;
	/*
	//this is to cover the starting Load game screen
	//no idea what's going on with this jump-table
	//why there are so many duplicates? games use #9
	1A5B5EC is SOM::L.startup
	0042C09D 8B 15 EC B5 A5 01    mov         edx,dword ptr ds:[1A5B5ECh]  
	0042C0A3 68 00 B4 A5 01       push        1A5B400h  
	0042C0A8 FF 14 95 54 E5 45 00 call        dword ptr [edx*4+45E554h] 
	*/
	menus = (void**)0x045E554;
	//menus[0] = som_game_startup_42c120;
	//menus[1] = som_game_startup_42c230;
	//menus[2] = som_game_startup_42c280; 
	//menus[3] = som_game_startup_42c460; 
	menus[4] = som_game_9_menu41f880;
	menus[5] = som_game_9_menu41f880;
	//menus[6] = som_game_startup_42c460; 
	menus[7] = som_game_9_menu41f880;
	menus[8] = som_game_9_menu41f880;
	menus[9] = som_game_9_menu41f880; //standalone game?
	//event based save game
	//0040A70C E8 8F 59 01 00       call        004200A0  
	//0040A711 83 C4 04             add         esp,4  
	*(DWORD*)0x40A70D = (DWORD)som_game_10_menu4200a0-0x40A711;

	//the "show" or appraisal shop has no filter
	//it only tests if an item is in the pcstore
	//this installs _appraiser_filter_43EBC7 which is
	//used at a minimum take magics off the menu
	/*
	0043D229 80 38 00             cmp         byte ptr [eax],0  
	0043D22C 74 01                je          0043D22F  
	0043D22E 41                   inc         ecx 
	->
	call som_game_43D229	
	*/
	*(BYTE*)0x43D229 = 0xe8; //call
	*(DWORD*)0x43D22A = (DWORD)som_game_43D229-0x43D22E;
	*(BYTE*)0x43D22E = 0x90;	
	/*
	0043EBC7 80 3A 00             cmp         byte ptr [edx],0  
	0043EBCA 0F 84 39 01 00 00    je          0043ED09  
	0043EBD0 3B C8                cmp         ecx,eax  
	0043EBD2 0F 8C 28 01 00 00    jl          0043ED00  
	0043EBD8 8D 50 04             lea         edx,[eax+4]  
	0043EBDB 3B CA                cmp         ecx,edx
	0043EBDD 0F 8D 1D 01 00 00    jge         0043ED00
	->
	call som_game_menu::_appraiser_filter_43EBC7
	je          0043ED09  
	jge   
	*/	
	memset((void*)0x43EBC7,0x90,0x43EBDD-0x43EBC7);
	*(BYTE*)0x43EBC7 = 0xe8; //call
	*(DWORD*)0x43EBC8 = (DWORD)som_game_43EBC7-0x43EBCC;
	*(WORD*)(0x43EBDD-6) = 0x840F; //je
	*(DWORD*)(0x43EBDD-4) = 0x43ED09-0x43EBDD;

	//BUG-FIX
	//this is checking if the body-equipment piece covers the arms/legs
	//problem is it doesn't check for if there is no equipment available
	//and to a lesser extent, it takes the PR2 index validity for granted
	/*
	0041B7A8 66 8B 81 F0 6F 56 00 mov         ax,word ptr [ecx+566FF0h]  
	0041B7AF 8D 14 80             lea         edx,[eax+eax*4]  
	0041B7B2 8D 04 50             lea         eax,[eax+edx*2]  
	0041B7B5 80 3C C5 88 04 58 00 04 cmp         byte ptr [eax*8+580488h],4
	0041B7BD 75 06                jne         0041B7C5
	->
	call som_game_41B7A8
	0041B7BD 75 06                jne         0041B7C5
	*/
	memset((void*)0x41B7A8,0x90,0x41B7BD-0x41B7A8);
	*(BYTE*)0x41B7A8 = 0xe8; //call
	*(DWORD*)0x41B7A9 = (DWORD)som_game_41B7A8-0x41B7AD;


	//SELL BUG-FIX
	//SAFE????? Sold-out items' graphic is unchanged.
	//NOTE: _shop_item_was_sold_out_ zeroes 0x1D11E21.
	//0043E378 76 3E                jbe         0043E3B8
	memset((void*)0x43E378,0x90,2);


	//OBSCURE BUG-FIXES	
	//prevent infinite-recursion (crash) if file is missing?
	//0042E5C0...
	//0042E628 E8 93 FF FF FF       call        0042E5C0
	//0042E649 E8 72 FF FF FF       call        0042E5C0
	*(WORD*)&SOM::L.SFX_dat_file = 0xFFFF;
	//this zeroes out the SFX.dat region before it's read in
	//0042E4A5 F3 AB                rep stos    dword ptr es:[edi]
	*(WORD*)0x42E4A5 = 0x9090;



	//2018: mp3, etc.
	// 	   
	// 	REMINDER: 401300 is detoured to _ext_401300 (2022)
	// 
	//0043F749 E8 B2 1B FC FF       call        00401300
	*(DWORD*)0x43F74A = (DWORD)som_game_BGM_401300-0x43F74E;
	//0044C231 E8 CA 50 FB FF       call        00401300
	*(DWORD*)0x44C232 = (DWORD)som_game_BGM_401300-0x44C236;
	//2020: snd->wav? 
	//0043F4A1 E8 AA BF 00 00       call        0044B450
	*(DWORD*)0x43F4A2 = (DWORD)som_game_44b450-0x43F4A6;
	
	//2019: load/unload textures
	//NOTE: THIS ONE IS USED TO RESET THE RENDER DEVICE!
	//004484DD E8 4E 10 00 00       call        00449530  	
	*(DWORD*)0x4484DE = (DWORD)som_game_449530-0x4484E2;
	//00448725 E8 06 0E 00 00       call        00449530  
	*(DWORD*)0x448726 = (DWORD)som_game_449530-0x44872A;
	//004492E4 E8 47 02 00 00       call        00449530  
	*(DWORD*)0x4492E5 = (DWORD)som_game_449530-0x4492E9;
	//004494E4 E8 47 00 00 00       call        00449530
	*(DWORD*)0x4494E5 = (DWORD)som_game_449530-0x4494E9;
	//2021: TIM colorkey? (16-bit) 
	if(EX::INI::Bugfix()->do_fix_colorkey)
	{
		//0044489A E8 81 02 00 00       call        00444B20
		*(DWORD*)0x44489b = (DWORD)som_game_444b20-0x44489f;
	}
	else Ex_mipmap_colorkey = 7; //Moratheia?	

	if(1) //EXPERIMENTAL
	{
		//dying? (moving from som_state.cpp)
		//004258DB E8 20 2B FE FF   call        00408400
		*(DWORD*)0x4258DC = (DWORD)som_game_408400-0x4258E0;		
		
		//2020: save game (enemy extension)
		//loading MPX
		//413140 41315f (playing BGM, i.e. SOM::bgm)
		//clearing
		//42c8f8 42c8ff 42c90d
		//0040194E E8 9D AF 02 00       call        0042C8F0
		*(DWORD*)0x40194F = (DWORD)som_game_42c8f0-0x401953;
		//004258D1 E8 1A 70 00 00       call        0042C8F0
		*(DWORD*)0x4258D2 = (DWORD)som_game_42c8f0-0x4258D6;
		//0042C97F E8 6C FF FF FF       call        0042C8F0
		*(DWORD*)0x42C980 = (DWORD)som_game_42c8f0-0x42C984;
		//reading save file
		//42ca57
		//first read
		//0042C98E E8 80 31 02 00       call        0044FB13
		*(DWORD*)0x42C98f = (DWORD)som_game_44fb13_x-0x42C993;
		//read maps
		//0042CA2B E8 E3 30 02 00       call        0044FB13
		*(DWORD*)0x42CA2C = (DWORD)som_game_44fb13_x-0x42CA30;
		//writing save file
		//42cef4 42cf15
		//first write
		//0042CE4A E8 50 31 02 00       call        0044FF9F
		*(DWORD*)0x42CE4b = (DWORD)som_game_44ff9f_x-0x42CE4f;
		//write maps
		//0042CF07 E8 93 30 02 00       call        0044FF9F
		*(DWORD*)0x42CF08 = (DWORD)som_game_44ff9f_x-0x42CF0c;
		//loading MPX subroutine (initialization)
		//42cf71 (42cf40)
		//00412F32 E8 09 A0 01 00       call        0042CF40
		*(DWORD*)0x412F33 = (DWORD)som_game_42cf40-0x412F37;
		//writing/unloading subroutine (flushing)
		//42d332 (42d300)
		//004131A8 E8 53 A1 01 00       call        0042D300
		*(DWORD*)0x4131A9 = (DWORD)som_game_42d300-0x4131Ad;
		//0042CDD5 E8 26 05 00 00       call        0042D300
		*(DWORD*)0x42CDD6 = (DWORD)som_game_42d300-0x42CDDa;
		//probing save files
		//(41deb0)
		//testing on load (save?) just by calling fopen/fclose
		//0041DEB1 E8 E8 16 03 00       call        0044F59E
		//(41f880)
		//again? what's the difference? this time only 3 files
		//are tested for the real-time selection display
		//0041F8B4 E8 E5 FC 02 00       call        0044F59E
		//(fread)
		//reads just the time/exp (first two dwords)
		//0041FA63 E8 AB 00 03 00       call        0044FB13
		*(DWORD*)0x41FA64 = (DWORD)som_game_44fb13_y-0x41FA68;
		//for some reason Load is different from Save?
		//0042021F E8 EF F8 02 00       call        0044FB13
		*(DWORD*)0x420220 = (DWORD)som_game_44fb13_y-0x420224;
		//closing after writing (renaming)
		//0042CF1E E8 8E 26 02 00       call        0044F5B1
		*(DWORD*)0x42CF1F = (DWORD)som_game_44f5b1-0x42CF23;
		//menu system, returns save game or 0xff ... second
		//is SOM_SYS called from inside som_game_403670_params
		//00401A51 E8 6A A5 02 00       call        0042BFC0
		//00403715 E8 A6 88 02 00       call        0042BFC0  
		*(DWORD*)0x401A52 = (DWORD)som_game_42bfc0_intros-0x401A56;		
		//*(DWORD*)0x403716 = (DWORD)som_game_42bfc0_intros-0x40371a;
		//April 2021
		//this seems to parse the commandline? when "LOAD" is set
		//to load a SOM save file it has to ignore the commandline
		//provided by SOM_EX.exe
		//004019DC E8 8F 1C 00 00       call        00403670 
		*(DWORD*)0x4019DD = (DWORD)som_game_403670_params-0x4019E1;
	}

	//LOADING PR2 (PRO) FILE?
	//00405C9A E8 74 9E 04 00       call        0044FB13
	*(DWORD*)0x405C9B = (DWORD)som_game_44fb13_60fps_etc-0x405C9F;
	//00428830 E8 DE 72 02 00       call        0044FB13
	*(DWORD*)0x428831 = (DWORD)som_game_44fb13_60fps_etc-0x428835;

	//2020: EQUIP ARM.MDL PIECE #2 OF FULL BODY ARMOR
	//00427388 81 FE 10 1A 9C 01    cmp         esi,19C1A10h
	*(DWORD*)0x42738a = 0x19C1A14;
	//these treat 0 as a special item ID for unloading swing MDO
	//data. I guess it's supposed to be the unequip ID
	//00427601 75 07                jne         0042760A
	//004276AF 74 04                je          004276B5
	*(BYTE*)0x427601 = 0xeb; //jmp
	*(WORD*)0x4276AF = 0x9090; //nop*/

	//2020: moving this older code out from som.state.cpp
	{
		//Take Item? loop
		//00410600 E8 1B 00 00 00       call        00410620
		*(DWORD*)0x410601 = (DWORD)som_game_410620-0x410605;
		//from inanimate container
		//0042AC84 E8 97 59 FE FF       call        00410620
		*(DWORD*)0x42AC85 = (DWORD)som_game_410620-0x42AC89;
		//from animated containers
		//0042BAF9 E8 22 4B FE FF       call        00410620
		*(DWORD*)0x42BAFA = (DWORD)som_game_410620-0x42BAFE;

		//2020: knockout unused return code to forward return
		//from som_game_410620 to super routine caller's
		//0041060B B0 01                mov         al,1
		*(WORD*)0x41060B = 0x9090;
	}

	//2021: arm equipment load/unload (for left arm mainly)
	{
		//TODO? som_MPX_413190 COULD CALL THIS WITH LESS
		//CODE? som_MPX_401500 TOO? WOULD JUST HAVE TO
		//KNOCKOUT THESE CALLS THEN

		//unloading equipment?
		//004131B7 E8 24 44 01 00       call        004275E0
		/*som_MPX_413190 (layers) does this more than once
		*(DWORD*)0x4131B8 = (DWORD)som_game_4275e0-0x4131BC;*/
		memset((void*)0x4131B7,0x90,5);
		//004253D9 E8 02 22 00 00       call        004275E0 
		memset((void*)0x4253D9,0x90,5);
		//forced unequip?
		//this routine removes equipment not in the inventory
		//it's only used in the Sell menu and actually isn't
		//needed because equipped items aren't on the table
		//0043D0A5 E8 76 A0 FE FF       call        00427120 
		memset((void*)0x43D0A5,0x90,5);
		/*2022: this is duplicated in the INVENTORY section
		//below
		//2021: likewise the inventory screen unequips items
		//while this is a logical feature to have, it would
		//remove event based equipment as a side effect, and
		//isn't required
		//0041D568 74 39                je          0041D5A3
		*(BYTE*)0x41D568 = 0xeb; //jmp*/

		//loading equipment?
		//NOTE: som_game_menu_reprogram disables the menus
		//0041311A E8 51 40 01 00       call        00427170
		//
		// REMINDER: som_MPX_411a20 reproduces this change
		//
		*(DWORD*)0x41311b = (DWORD)som_game_equip-0x41311F;
	}

	//2021: vbuffer released after d3d/ddraw?
	{
		//KEEPING? Direct3DCreate9On12Ex (dx.d3d9c.cpp)
		//has wording suggesting the "device" needs to 
		//be released last. I don't know why it would
		//mention that unless it has some limitation
		//https://github.com/microsoft/DirectX-Specs/blob/master/d3d/TranslationLayerResourceInterop.md

		//NOTE: I investigated this because of a bug I
		//introduced into dx.d3d9X.cpp (X) but there are
		//some notes of freezes on releasing a vbuffer
		//in Ghidra that could be because of the unusual
		//release order? I think this happens in release
		//builds. I think there's some kill code somewhere
		//to avoid it (it's better to not deallocate on
		//close anyway, it just wastes time)
		#ifdef NDEBUG
//		#error test me
		int todolist[SOMEX_VNUMBER<=0x1020704UL];
		#endif

		//this routine tears down d3d/ddraw and zeroes a
		//large chunk of memory. hoping that memory won't
		//change the behavior of the subsequent calls that
		//also tear down d3d/ddraw (after vbuffers, etc.)
		memset((char*)0x401b97,0x90,5);
	}

	//2022: hide "take item" menu?
	{
		//00410D8B E8 80 11 01 00       call        00421F10 
		*(DWORD*)0x410D8c = (DWORD)som_game_examine_421f10-0x410D90;
		//00410DF0 E8 BB 1C 01 00       call        00422AB0
		//00410E1F E8 8C 1C 01 00       call        00422AB0
		//00410E4E E8 5D 1C 01 00       call        00422AB0
		*(DWORD*)0x410DF1 = (DWORD)som_game_examine_422ab0-0x410DF5;
		*(DWORD*)0x410E20 = (DWORD)som_game_examine_422ab0-0x410E24;
		*(DWORD*)0x410E4f = (DWORD)som_game_examine_422ab0-0x410E53;

		//THE WEIRDEST THING
		//these hide items in menus, but the MDO files are still
		//loaded, so it's not a performance optimization... and
		//on the field the items on the ground disappear!!!
		//0043CFB4 74 3B                je          0043CFF1 
		//00418549 74 3B                je          00418586
		//00410D08 74 3B                je          00410D45
		*(WORD*)0x43CFB4 = 0x9090;
		*(WORD*)0x418549 = 0x9090;
		*(WORD*)0x410D08 = 0x9090;
	}

	if(EX::INI::Option()->do_opengl) //Options
	{
		//447400 chooses D3D/OpenGL but it's called before
		//401FB0 so the order needs to change... it's best
		//to just knock them out and have 447400 call them
		
		//0041EC25  call        00447400		
		//0041E98C  call        00447400
		*(DWORD*)0x41EC26 = (DWORD)som_game_447400-0x41EC2a;
		*(DWORD*)0x41E98d = (DWORD)som_game_447400-0x41E991;
		//0041EC2D  call        00401FB0
		//0041E99D  call        00401FB0
		memset((void*)0x41EC2D,0x90,5);
		memset((void*)0x41E99D,0x90,5);

		//NOTE: disabling this is probably a good idea
		//with or without do_opengl
		//playing movies forces 16BPP and 640x480 which
		//won't work with do_opengl since 16BPP is D3D9
		//00423782 75 10                jne         00423794 
		//0042378A 75 08                jne         00423794
		//00423792 74 1D                je          004237B1
		*(WORD*)0x423782 = 0x2deb; //jmp 4237B1
	}
	if(1)
	{
		*(WORD*)0x423782 = 0x2deb; //jmp 4237B1
		//SAME (game over) (test is inverted?)
		//00402D6C EB 10           jmp        00402D7E
		//00402D74 75 08           jne        00402D7E
		//00402D7C 74 1B           je         00402D99 //game over	
		*(WORD*)0x402D6C = 0x2beb; //jmp 402D99
		//00401A22 75 10           jne        00401A34 //
		//00401A2A 75 08           jne        00401A34 //
		//00401a32 74 1d           je         00401a51 //titles (pointless)
		*(WORD*)0x401a22 = 0x2deb; //jmp 401a51
		//004036E8 75 10           jne        004036FA
		//004036F0 75 08           jne        004036FA
		//004036F8 74 1B           je         00403715 //SOM_SYS (preview)
		*(WORD*)0x4036E8 = 0x2beb; //jmp 403715
		//00403967 75 10           jne        00403979  
		//0040396F 75 08           jne        00403979  
		//00403977 EB 1B           jmp        00403994 //SOM_SYS (preview)	
		*(WORD*)0x403967 = 0x2beb; //jmp 40399
		//
		// NOTE: there's some more in the F1 and F2 debug screens
		//
		//now, fix some hard calls that hardcode 640x480
		//NOTE: 42c460 draws start text but som.hacks.cpp upscales them
		//NOTE: these really could go one level up, but I think this is
		//less work this way
		// 
		// NOTE: som_state_slides422AB0 adjusts the text
		// 
		//00405B70 E8 7B 2C 04 00       call        004487F0
		//0042C8A4 E8 47 BF 01 00       call        004487F0
		//0042C8DB E8 80 D8 01 00       call        0044A160
		//00405BA4 E8 B7 45 04 00       call        0044A160
		*(DWORD*)0x405B71 = (DWORD)som_game_640x480_4487f0-0x405B75;
		*(DWORD*)0x42C8A5 = (DWORD)som_game_640x480_4487f0-0x42C8A9;
		*(DWORD*)0x42C8Dc = (DWORD)som_game_640x480_44a160-0x42C8e0;
		*(DWORD*)0x405BA5 = (DWORD)som_game_640x480_44a160-0x405BA9;
		//this black fade out is hardwired after the show ends
		//0040531E E8 3D 4E 04 00       call        0044A160
		*(DWORD*)0x40531f = (DWORD)som_game_640x480_44a160-0x405323;
		//movies: this one too probably (it's hard to tell)
		//00423C0B E8 50 65 02 00       call        0044A160
		*(DWORD*)0x423C0c = (DWORD)som_game_640x480_44a160-0x423C10;
	}

	//2024: change bad status negation from F3 to F4?
	{
		//004259C4 8A 0D 5F 08 4C 00    mov         cl,byte ptr ds:[4C085Fh] 
		*(BYTE*)0x4259C6 = 0x60;
		//004259D2 0F 85 9A 00 00 00    jne         00425A72
		*(BYTE*)0x4259D3 = 0x84; //je
	}

	//removing redundant subroutine?
	memset((void*)0x0424669,0x90,5); //call 427780 (reset pc atk/def params)

	if(1) //2024: speeding up running gauge depletion
	{
		//00425247 c1 ea 06        SHR        EDX,0x6
		*(BYTE*)0x425249 = 5;
	}

	if(1) //2024: SOM::SND support
	{
		//this needs to run before background loading
		//in order to support som_SFX_sounds 
		//0040199e e8 3d 2c 02 00	CALL	FUN_004245e0_init_pc_arm_and_stats_and_hud_etc?
		*(DWORD*)0x40199f = (DWORD)som_game_4245e0_init_pc_arm_and_hud_etc-0x4019a3;

		//healing and wall magic
		//004283d8 e8 63 71 01 00  CALL  FUN_0043f540_play_2D_sound_w_pitch?
		//004283f8 e8 43 71 01 00  CALL  FUN_0043f540_play_2D_sound_w_pitch?
		*(DWORD*)0x4283d9 = (DWORD)som_game_43f540_fullscreen_snd-0x4283dd;		
		*(DWORD*)0x4283f9 = (DWORD)som_game_43f540_fullscreen_snd-0x4283fd;
	}

	if(1) //2024: add inventory count (for equipment breaking)
	{
		//0041b081 e8 2a 7a  00 00  CALL FUN_00422ab0_TextOutA_with_contrast?
		*(DWORD*)0x41b082 = (DWORD)som_game_equip_menu_text_422ab0-0x41b086;
	}
}
static void som_game_menu_reprogram()
{			
	using namespace som_game_menu;
	
	//this reprogramming is done only after Item.Prm & Sys.Dat
	//are loaded into memory
	DWORD old = 0;
	DWORD text = 0x401000, text_s = SOM::text->info.RegionSize; 
	VirtualProtect((void*)text,text_s,PAGE_EXECUTE_READWRITE,&old); 
	assert(old!=PAGE_EXECUTE_READWRITE);

	assert(250==N);
	N = 0;
	for(int i=0;i<250*336;i+=336)
	if(0xFFFF!=SOM::L.item_prm_file->s[i])
	N++;
	//this is not quite working
	//inventory wrapping breaks with it. Equip Magic shows
	//Nothing after leaving inventory
	//int _7 = som_game_nothing()[7]; //2020: not sure about this
	int M = N;
	for(int i=0;i<32;i++) 
	if(SOM::L.magic32[i]<250) //pretty sure this shouldn't be in here
	//if(_7!=SOM::L.magic32[i]) //include "Nothing"? something's wrong
	N++;

	BYTE *mem = new BYTE[2*N+2*N+N*336+32];
	ini_browse = (WORD*)mem; mem+=2*N;
	inv_sorted = (WORD*)mem; mem+=2*N;		
	prm_sorted = (BYTE*)mem; mem+=N*336; 
	m32_sorted = mem; mem+=32;

	for(int i=0,j=0;i<32;i++) 
	if(SOM::L.magic32[i]<250)
	//if(_7!=SOM::L.magic32[i]) //2020
	ini_browse[M+j++] = 250+i;		
	for(int i=0,j=0,k=0;i<250*336;i+=336,k++)
	if(0xFFFF!=SOM::L.item_prm_file->s[i])
	ini_browse[j++] = k;
	
	wchar_t buf[(250+32)*sizeof("000=000")+1];
	if(0!=GetPrivateProfileSection
	(L"config/itemCfg",buf,sizeof(buf),SOM::Game::title('.ini')))
	for(wchar_t *p=buf;*p!='\0';)
	{
		int src2 = 0;
		int src = _wtoi(p); while('='!=*p++)
		if('^'==p[-1]) src2 = _wtoi(p);
		int dst = _wtoi(p); while('\0'!=*p++);		
		if(src<0) //Magic is -1 to -32.
		{
			src = -src+249;
			//abs: second minus-sign is optional
			if(src2) src2 = abs(src2)+249; 
		}
		if(!src2) src2 = src;
		else if(src2<src) continue; //Log error?				
		if(dst<0) dst = -dst+249;

		int swap = src, cp = src2-src+1;
		if(src!=ini_browse[src])
		src = std::find(ini_browse,ini_browse+N,src)-ini_browse;
		if(dst!=ini_browse[dst])
		dst = std::find(ini_browse,ini_browse+N,dst)-ini_browse;
		if(src==N||dst==N) continue; //Log error?		

		if(cp==1) cp__1: //model algorithm
		{
			if(src==dst||dst==src+1) continue; //Log error?

			for(int i=src-1;i>=dst;i--)
			ini_browse[i+1] = ini_browse[i];
			for(int i=src+1;i<dst;i++)
			ini_browse[i-1] = ini_browse[i];
			ini_browse[dst<src?dst:dst-1] = swap;
		}
		else cp__n: //block transfer mode
		{
			//PROLOG
			int n = 1;
			while(n<cp&&swap+n==ini_browse[src+n])
			n++;
			if(dst>=swap&&dst<=swap+n) continue; //Log error? 
			
			//ALGORITHM
			if(src>dst)
			memmove(ini_browse+dst+n,ini_browse+dst,(src-dst)*sizeof(wchar_t));
			else //src<dst
			memmove(ini_browse+dst-n,ini_browse+dst,(dst-src)*sizeof(wchar_t));
			for(int i=0;i<n;i++) 
			ini_browse[dst+i] = swap+i;

			//EPILOG
			if(cp!=n)
			{
				swap+=n; cp-=n; dst+=n;
				src = std::find(ini_browse,ini_browse+N,swap)-ini_browse;
				if(src==N) continue; //Log error?		
				if(cp==1) goto cp__1; goto cp__n;
			}
		}
	}	
		
	//ILLUSTRATING: this is initialized to 0...
	//som_game_menu_init sets it for real, later on
	//NOTE: 40FBB7 demonstrates this should be safe to do 
	//because (for some reason) there are 256 PR2 records
	SOM::L.item_pr2_file[250].c[62] = 8; //585A6E
	//add 9th icon to itemicon.bmp...
	//does 421F10 only do the item icons?
	//00421F1D 83 F8 0D             cmp         eax,0Dh
	//*(BYTE*)0x421F1f = 0xE; //won't do...
	//0041D055 E8 B6 4E 00 00       call        00421F10
	*(DWORD*)0x41D056 = (DWORD)som_game_421f10_itemicon-0x41D05A;
	
	//EXPERIMENTAL //EXPERIMENTAL //EXPERIMENTAL //EXPERIMENTAL

	//(4230C0)
	//THIS UPDATES THE ITEM MODEL (shops AND menus)
	//004230D3 05 F0 6F 56 00       add         eax,566FF0h  
	*(DWORD*)0x4230D4 = (DWORD)prm_sorted;	
	if(1) //UNRELATED: change item display to better match KF2
	{
 		//hack: change from X axis tilt to Z axis
		//004231EA D9 59 4C             fstp        dword ptr [ecx+4Ch]
		//004231FA 89 41 54             mov         dword ptr [ecx+54h],eax
	//	std::swap(*(BYTE*)0x4231EC,*(BYTE*)0x4231FC);
		//same: prize menu
		//00410A66 D9 59 1C             fstp        dword ptr [ecx+1Ch]    
		//00410A78 89 69 24             mov         dword ptr [ecx+24h],ebp 
	//	std::swap(*(BYTE*)0x410A68,*(BYTE*)0x410A7a);
		//hack: initial rotation (-0.00349065871)
		//004231F0 C7 42 50 89 C3 64 BB mov         dword ptr [edx+50h],0BB64C389h	
	//	*(FLOAT*)0x4231F3 = M_PI/18; //10deg	
		//same: how to do this? (0)
		//fortunately the ASM here is extremely redundant???
		//004109FC 8B 0D 60 60 58 00    mov         ecx,dword ptr ds:[586060h]  
		//00410A02 89 69 50             mov         dword ptr [ecx+50h],ebp 	
	//	memcpy((void*)0x4109FC,(void*)0x4231F0,7);
	//	*(BYTE*)0x4109FD = 0x40; //eax+50h
	//	*(WORD*)0x410A03 = 0x9090;
		//hack: reversing rotation (-0.00872664619)
		//00410D2B D8 05 B0 83 45 00    fadd        dword ptr ds:[4583B0h] //take menu
		//0041856C D8 05 B0 83 45 00    fadd        dword ptr ds:[4583B0h] //main menu
		//0043CFD7 D8 05 B0 83 45 00    fadd        dword ptr ds:[4583B0h] //shop menu
		*(FLOAT*)0x4583B0*=-1;

		//trying to do programmatically
		//using item
		//004191DF E8 DC 9E 00 00       call        004230C0
		*(DWORD*)0x4191e0 = (DWORD)som_game_4230C0-0x4191e4;
		//equip menu
		//0041AAF9 E8 C2 85 00 00       call        004230C0
		*(DWORD*)0x41AAFa = (DWORD)som_game_4230C0-0x41AAFe;
		//equip item
		//0041B03F E8 7C 80 00 00       call        004230C0
		*(DWORD*)0x41B040 = (DWORD)som_game_4230C0-0x41B044;
		//buy/show
		//0043D58D E8 2E 5B FE FF       call        004230C0
		*(DWORD*)0x43D58e = (DWORD)som_game_4230C0-0x43D592;
		//sell
		//0043E3B0 E8 0B 4D FE FF       call        004230C0
		*(DWORD*)0x43E3B1 = (DWORD)som_game_4230C0-0x43E3B5;
		//taking item
		//00410651 E8 DA 01 00 00       call        00410830
		*(DWORD*)0x410652 = (DWORD)som_game_410830-0x410656;
	}

	//MAIN (418700)
	//Items?
	//00418E0E BA 19 71 56 00       mov         edx,567119h  
	*(DWORD*)0x418E0F = (DWORD)prm_sorted+297;
	//00418E13 B9 18 1A 9C 01       mov         ecx,19C1A18h 
	*(DWORD*)0x418E14 = (DWORD)inv_sorted;
	//00418E4D 81 F9 0C 1C 9C 01    cmp         ecx,19C1C0Ch
	*(DWORD*)0x418E4F = (DWORD)inv_sorted+min(N,250)*2;
	//Magic?
	//00418E5C 8B 3D 40 1C 9C 01    mov         edi,dword ptr ds:[19C1C40h]
	*(DWORD*)0x418E5E = (DWORD)&pcm_sorted;
	//00418E71 8A 81 22 2D D1 01    mov         al,byte ptr [ecx+1D12D22h]
	*(DWORD*)0x418E73 = (DWORD)m32_sorted;
	//Equip?
	//00418EA3 BA F0 6F 56 00       mov         edx,566FF0h  
	*(DWORD*)0x418EA4 = (DWORD)prm_sorted;
	//00418EA8 B9 18 1A 9C 01       mov         ecx,19C1A18h	
	*(DWORD*)0x418EA9 = (DWORD)inv_sorted;
	//00418EDD 81 F9 0C 1C 9C 01    cmp         ecx,19C1C0Ch
	*(DWORD*)0x418EDF = (DWORD)inv_sorted+min(N,250)*2;
	//Inventory?
	//00418F46 B8 19 1A 9C 01       mov         eax,19C1A19h 
	*(DWORD*)0x418F47 = (DWORD)inv_sorted+1;
//	00418F4B 80 38 00             cmp         byte ptr [eax],0  
//	00418F4E 74 01                je          00418F51  
//	00418F50 41                   inc         ecx  
//	00418F51 83 C0 02             add         eax,2  
	//00418F54 3D 0D 1C 9C 01       cmp         eax,19C1C0Dh	
	*(DWORD*)0x418F55 = (DWORD)inv_sorted+1+N*336;		
	//
	// 2020: did I miss this one? it mostly works in Moratheia, but it does
	// enter the menu when it shouldn't and doesn't work in my KF2 project?
	// STILL NOT WORKING???
	//
	//0041AC57 8B 2D 40 1C 9C 01    mov         ebp,dword ptr ds:[19C1C40h]
	*(DWORD*)0x41AC59 = (DWORD)&pcm_sorted;

	//ITEM (419020)
	//004190ED BD 19 1A 9C 01       mov         ebp,19C1A19h  
	*(DWORD*)0x4190EE = (DWORD)inv_sorted+1;
	//004190F2 BF F0 6F 56 00       mov         edi,566FF0h  
	*(DWORD*)0x4190F3 = (DWORD)prm_sorted;
	//0041929F 81 FF 10 B8 57 00    cmp         edi,57B810h 
	*(DWORD*)0x4192A1 = (DWORD)prm_sorted+min(N,250)*336;		
	//004193AE 8A 83 20 71 56 00    mov         al,byte ptr [ebx+567120h]
	*(DWORD*)0x4193B0 = (DWORD)prm_sorted+304;
	//0041963B 8A 83 1A 71 56 00    mov         al,byte ptr [ebx+56711Ah]  
	*(DWORD*)0x41963D = (DWORD)prm_sorted+298;
		//Map that is Consumed only?
		//00419650 8A 0C 45 19 1A 9C 01 mov         cl,byte ptr [eax*2+19C1A19h]  
		*(DWORD*)0x419653 = (DWORD)inv_sorted+1;
		//0041965F 88 0C 45 19 1A 9C 01 mov         byte ptr [eax*2+19C1A19h],cl
		*(DWORD*)0x419662 = (DWORD)inv_sorted+1;
	//004197A3 80 BF 18 71 56 00 02 cmp         byte ptr [edi+567118h],2
	*(DWORD*)0x4197A5 = (DWORD)prm_sorted+296;
	//004197C3 8A 87 20 71 56 00    mov         al,byte ptr [edi+567120h]
	*(DWORD*)0x4197C5 = (DWORD)prm_sorted+304;
	//004197E8 8D 8F 21 71 56 00    lea         ecx,[edi+567121h] 
	*(DWORD*)0x4197EA = (DWORD)prm_sorted+305;
	//00419909 81 C1 F2 6F 56 00    add         ecx,566FF2h 
	*(DWORD*)0x41990B = (DWORD)prm_sorted+2;

	//MAGIC (419b60)
	//00419C28 8A 85 22 2D D1 01    mov         al,byte ptr [ebp+1D12D22h]
	*(DWORD*)0x419C2A = (DWORD)m32_sorted;
	//00419C63 85 05 40 1C 9C 01    test        dword ptr ds:[19C1C40h],eax
	*(DWORD*)0x419C65 = (DWORD)&pcm_sorted;
	//00419E4A 8A 81 22 2D D1 01    mov         al,byte ptr [ecx+1D12D22h]
	*(DWORD*)0x419E4C = (DWORD)m32_sorted;
	//00419F80 8A 81 22 2D D1 01    mov         al,byte ptr [ecx+1D12D22h]
	*(DWORD*)0x419F82 = (DWORD)m32_sorted;

	//EQUIPMENT (41a1e0)
	//0041A31A D9 80 18 71 56 00    fld         dword ptr [eax+567118h]	   
	*(DWORD*)0x41A31C = (DWORD)prm_sorted+296;
	//0041A33C D8 82 18 71 56 00    fadd        dword ptr [edx+567118h] 
	*(DWORD*)0x41A33E = (DWORD)prm_sorted+296;
	//0041A4B6 8A 88 22 2D D1 01    mov         cl,byte ptr [eax+1D12D22h]
	*(DWORD*)0x41A4B8 = (DWORD)m32_sorted;
	//0041A595 81 C7 F0 6F 56 00    add         edi,566FF0h		 41CE32
	*(DWORD*)0x41A597 = (DWORD)prm_sorted;
	//0041A8FA D9 82 18 71 56 00    fld         dword ptr [edx+567118h] 
	*(DWORD*)0x41A8FC = (DWORD)prm_sorted+296;
	//0041A91C D8 80 18 71 56 00    fadd        dword ptr [eax+567118h] 
	*(DWORD*)0x41A91E = (DWORD)prm_sorted+296;
		//this sets up the equipment based status effects like darkness
		//it may do additional "upkeep" routines, but it's used also by
		//the normal play context, and so shouldn't be modified. it can
		//be called after 41a1e0 returns
		//NOTE: the shops also do this for some reason. it's done after
		//leaving the screen; so that darkness, etc. kicks in only once
		//the "Main Menu" screen appears
		//0041AB71 E8 0A CC 00 00       call        00427780
		memset((void*)0x41AB71,0x90,5);		
	//0041ABE3 BA 18 1A 9C 01       mov         edx,19C1A18h 
	*(DWORD*)0x41ABE4 = (DWORD)inv_sorted;
	//0041ABE8 BF F0 6F 56 00       mov         edi,566FF0h 
	*(DWORD*)0x41ABE9 = (DWORD)prm_sorted;
	//0041AC48 81 FF 10 B8 57 00    cmp         edi,57B810h 
	*(DWORD*)0x41AC4A = (DWORD)prm_sorted+min(N,250)*336;
	//0041AC68 8A 81 22 2D D1 01    mov         al,byte ptr [ecx+1D12D22h] 
	*(DWORD*)0x41AC6A = (DWORD)m32_sorted;
	//0041ACE0 BA F0 6F 56 00       mov         edx,566FF0h
	*(DWORD*)0x41ACE1 = (DWORD)prm_sorted;
	//0041ACE5 B9 18 1A 9C 01       mov         ecx,19C1A18h 
	*(DWORD*)0x41ACE6 = (DWORD)inv_sorted;
	//0041AD1E 81 F9 0C 1C 9C 01    cmp         ecx,19C1C0Ch (pcequip)
	*(DWORD*)0x41AD20 = (DWORD)inv_sorted+min(N,250)*2;

	//EQUIP ITEM (41adf0)
	//0041AEF4 BA 18 1A 9C 01       mov         edx,19C1A18h
	*(DWORD*)0x41AEF5 = (DWORD)inv_sorted;
	//0041AF09 BF F0 6F 56 00       mov         edi,566FF0h
	*(DWORD*)0x41AF0A = (DWORD)prm_sorted;
	//0041B09C 81 FF 10 B8 57 00    cmp         edi,57B810h 
	*(DWORD*)0x41B09E = (DWORD)prm_sorted+min(N,250)*336;
	//0041B1F4 66 8B 87 F0 6F 56 00 mov         ax,word ptr [edi+566FF0h]  
	*(DWORD*)0x41B1F7 = (DWORD)prm_sorted;
	//WTH???
	//0041B28B D9 87 18 71 56 00    fld         dword ptr [edi+567118h]
	*(DWORD*)0x41B28D = (DWORD)prm_sorted+296;
	//0041B2DB 8D AF 1C 71 56 00    lea         ebp,[edi+56711Ch] 
	*(DWORD*)0x41B2DD = (DWORD)prm_sorted+300;
	//0041B40D D9 87 18 71 56 00    fld         dword ptr [edi+567118h]
	*(DWORD*)0x41B40F = (DWORD)prm_sorted+296;
	//0041B463 8D AF 1C 71 56 00    lea         ebp,[edi+56711Ch]
	*(DWORD*)0x41B465 = (DWORD)prm_sorted+300;
	//0041B627 D9 82 18 71 56 00    fld         dword ptr [edx+567118h]
	*(DWORD*)0x41B629 = (DWORD)prm_sorted+296;
	//0041B64E D9 80 18 71 56 00    fld         dword ptr [eax+567118h]
	*(DWORD*)0x41B650 = (DWORD)prm_sorted+296;
	//0041B67F D8 81 18 71 56 00    fadd        dword ptr [ecx+567118h]
	*(DWORD*)0x41B681 = (DWORD)prm_sorted+296;
	//0041B69E D8 80 18 71 56 00    fadd        dword ptr [eax+567118h]
	*(DWORD*)0x41B6A0 = (DWORD)prm_sorted+296;
		//THIS MAY NOT BE NECESSARY, HOWEVER THIS IS CHANGING THE MODEL
		//AND som_game_equip SHOULD DO THAT REGARDLESS, AND ITS NOT USING
		//THE CORRECT ITEM NUMBERS TO DO SO
		//0041B700 E8 DB BE 00 00       call        004275E0
		memset((void*)0x41B700,0x90,5);		
		//HELMET? DOES THIS HAVE ANY EFFECT?
		//0041B805 E8 D6 BD 00 00       call        004275E0  		
		memset((void*)0x41B805,0x90,5);
	//0041B733 66 8B 81 F0 6F 56 00 mov         ax,word ptr [ecx+566FF0h] 
	*(DWORD*)0x41B736 = (DWORD)prm_sorted;
	//som_game_41B7A8 has this covered
	//0041B7A8 66 8B 81 F0 6F 56 00 mov         ax,word ptr [ecx+566FF0h]
	//*(DWORD*)0x41B7AB = (DWORD)prm_sorted;	
		//0041B831 E8 3A B9 00 00       call        00427170
		memset((void*)0x41B831,0x90,5);
	//0041B8EE 05 F2 6F 56 00       add         eax,566FF2h
	*(DWORD*)0x41B8EF = (DWORD)prm_sorted+2;
		//WEAPON--UNEQUIPPING		
		//0041B8A9 E8 32 BD 00 00       call        004275E0
		memset((void*)0x41B8A9,0x90,5);	
		//0041B8B7 E8 B4 B8 00 00       call        00427170
		memset((void*)0x41B8B7,0x90,5);			
		//HELMET--UNEQUIPPING
		//0041BA44 E8 97 BB 00 00       call        004275E0
		memset((void*)0x41BA44,0x90,5);	
		//0041BA5C E8 0F B7 00 00       call        00427170
		memset((void*)0x41BA5C,0x90,5);	
		//41b76d //2022: missed one? call        004275E0		
		//FEET!? HOW MANY OF THESE ARE THERE? UNEQUIPPING?
		//0041B7D7 E8 04 BE 00 00       call        004275E0
		memset((void*)0x41B7D7,0x90,5);	
		memset((void*)0x41b76d,0x90,5); //2022		
		//ACCESSORY--UNEQUIPPING :(
		//0041BA01 E8 DA BB 00 00       call        004275E0
		memset((void*)0x41BA01,0x90,5);	
		//SHIELD--UNEQUIPPING :(
		//0041BA19 E8 52 B7 00 00       call        00427170
		memset((void*)0x41BA19,0x90,5);	

	//EQUIP MAGIC (41bc00)
	//0041BCC5 8A 85 22 2D D1 01    mov         al,byte ptr [ebp+1D12D22h]
	*(DWORD*)0x41BCC7 = (DWORD)m32_sorted;
	//0041BCE6 85 1D 40 1C 9C 01    test        dword ptr ds:[19C1C40h],ebx 
	*(DWORD*)0x41BCE8 = (DWORD)&pcm_sorted;
	//0041BE61 8A 81 22 2D D1 01    mov         al,byte ptr [ecx+1D12D22h]
	*(DWORD*)0x41BE63 = (DWORD)m32_sorted;
	//0041BF07 8A 83 22 2D D1 01    mov         al,byte ptr [ebx+1D12D22h]
	*(DWORD*)0x41BF09 = (DWORD)m32_sorted;
		//THIS MAY NOT BE NECESSARY, HOWEVER THIS IS CHANGING THE MODEL
		//AND SOM::som_game_equip SHOULD DO THAT REGARDLESS, AND ITS NOT USING
		//THE CORRECT ITEM NUMBERS TO DO SO
		//0041C045 E8 96 B5 00 00       call        004275E0 (unequipping)
		memset((void*)0x41C045,0x90,5);
		//0041C053 E8 18 B1 00 00       call        00427170
		memset((void*)0x41C053,0x90,5);
		//0041C09E E8 3D B5 00 00       call        004275E0 (equipping)
		memset((void*)0x41C09E,0x90,5);
		//0041C0AD E8 BE B0 00 00       call        00427170 (equipping)
		memset((void*)0x41C0AD,0x90,5);
	//0041C0D3 8A 83 22 2D D1 01    mov         al,byte ptr [ebx+1D12D22h] 
	*(DWORD*)0x41C0D5 = (DWORD)m32_sorted;

	//INVENTORY (41cbd0)
	//0041CBD8 B8 18 1A 9C 01       mov         eax,19C1A18h
	*(DWORD*)0x41CBD9 = (DWORD)inv_sorted;
	//0041CBED 3D 0C 1C 9C 01       cmp         eax,19C1C0Ch (pcequip)
	*(DWORD*)0x41CBEE = (DWORD)inv_sorted+N*2;
	//0041CCEC BE F0 6F 56 00       mov         esi,566FF0h
	*(DWORD*)0x41CCED = (DWORD)prm_sorted;
	//0041CCF5 C7 44 24 1C 19 1A 9C 01 mov         dword ptr [esp+1Ch],19C1A19h
	*(DWORD*)0x41CCF9 = (DWORD)inv_sorted+1;
	//0041CFE7 3D 0D 1C 9C 01       cmp         eax,19C1C0Dh
	*(DWORD*)0x41CFE8 = (DWORD)inv_sorted+1+N*2;
	//0041D1C5 C6 04 45 18 1A 9C 01 00 mov         byte ptr [eax*2+19C1A18h],0 
	*(DWORD*)0x41D1C8 = (DWORD)inv_sorted;
	//0041D236 8A 14 45 18 1A 9C 01 mov         dl,byte ptr [eax*2+19C1A18h]
	*(DWORD*)0x41D239 = (DWORD)inv_sorted;
	//0041D244 88 14 45 18 1A 9C 01 mov         byte ptr [eax*2+19C1A18h],dl
	*(DWORD*)0x41D247 = (DWORD)inv_sorted;
//		0041D2C3 8B 4C 24 2C          mov         ecx,dword ptr [esp+2Ch]  
//		0041D2C7 8D 04 CD 00 00 00 00 lea         eax,[ecx*8]  
//		0041D2CE 2B C1                sub         eax,ecx  
//		0041D2D0 8D 0C 40             lea         ecx,[eax+eax*2]  
//		0041D2D3 C1 E1 04             shl         ecx,4
	//0041D2D6 81 C1 F2 6F 56 00    add         ecx,566FF2h
	*(DWORD*)0x41D2D8 = (DWORD)prm_sorted+2;
	//2022: preventing equip/unequip? (I think this may be done elsewhere)
	//0041D568 EB 39                jmp         0041D5A3
	*(WORD*)0x41D568 = 0x3feb; //jmp 41D5A9
	//41d58f call        004275E0 //notational (unequip)
	memset((void*)0x41d58f,0x90,5);

	//SHOP (43CE80)
	//0043D00F FF 14 8D 68 EE 45 00 call        dword ptr [ecx*4+45EE68h] 
	//Jump table: 0043d0c0 0043d3b0 0043e150 0043ea90
	//
	//TOP (43d0c0)
	//Showing?
	//0043D224 B8 19 1A 9C 01       mov         eax,19C1A19h  
	*(DWORD*)0x43D225 = (DWORD)inv_sorted+1;
	//0043D232 3D 0D 1C 9C 01       cmp         eax,19C1C0Dh  (pcequip+1) 
	*(DWORD*)0x43D233 = (DWORD)inv_sorted+1+min(N,250)*2;		
	//Selling?
	//0043D2B8 BF 19 1A 9C 01       mov         edi,19C1A19h 
	*(DWORD*)0x43D2B9 = (DWORD)inv_sorted+1;
	//UNUSED? Why care about equipment??? 
//	0043D2DF 8A 98 0C 1C 9C 01    mov         bl,byte ptr [eax+19C1C0Ch] (pcequip)	
	//0043D301 81 FF 0D 1C 9C 01    cmp         edi,19C1C0Dh  (pcequip+1)
	*(DWORD*)0x43D303 = (DWORD)inv_sorted+min(N,250)*2;
	//
	//BUY (43D3B0)	
	//0043D4E9 BB F2 6F 56 00       mov         ebx,566FF2h
	*(DWORD*)0x43D4EA = (DWORD)prm_sorted+2;
	//0043D69A 81 FB 12 B8 57 00    cmp         ebx,57B812h
	*(DWORD*)0x43D69C = (DWORD)prm_sorted+2+min(N,250)*336;
	//0043D6DB 66 8B 82 F0 6F 56 00 mov         ax,word ptr [edx+566FF0h]  
	*(DWORD*)0x43D6DE = (DWORD)prm_sorted;
	//0043D6E2 8D BA F0 6F 56 00    lea         edi,[edx+566FF0h]
	*(DWORD*)0x43D6E4 = (DWORD)prm_sorted;
	//0043DBCC 8A 04 75 19 1A 9C 01 mov         al,byte ptr [esi*2+19C1A19h]  
	*(DWORD*)0x43DBCF = (DWORD)inv_sorted+1;
	//0043DE3B 00 0C 45 19 1A 9C 01 add         byte ptr [eax*2+19C1A19h],cl  
	*(DWORD*)0x43DE3E = (DWORD)inv_sorted+1;
	//NOTE: CHOOSING TO PURCHASE AN ITEM IN PARTICULAR?
	//0043DFA2 80 3C 5D 19 1A 9C 01 FF cmp         byte ptr [ebx*2+19C1A19h],0FFh
	*(DWORD*)0x43DFA5 = (DWORD)inv_sorted+1;
	//
	//SELL (43E150)
	//0043E1BB B8 19 1A 9C 01       mov         eax,19C1A19h
	*(DWORD*)0x43E1BC = (DWORD)inv_sorted+1;
	//0043E1CA 3D 0D 1C 9C 01       cmp         eax,19C1C0Dh
	*(DWORD*)0x43E1CB = (DWORD)inv_sorted+1+min(N,250)*2;
	//0043E29F B8 19 1A 9C 01       mov         eax,19C1A19h
	*(DWORD*)0x43E2A0 = (DWORD)inv_sorted+1;
	//0043E2B3 BB F2 6F 56 00       mov         ebx,566FF2h
	*(DWORD*)0x43E2B4 = (DWORD)prm_sorted+2;
	//0043E494 3D 0D 1C 9C 01       cmp         eax,19C1C0Dh
	*(DWORD*)0x43E495 = (DWORD)inv_sorted+1+min(N,250)*2;
	//0043E846 28 0C 5D 19 1A 9C 01 sub         byte ptr [ebx*2+19C1A19h],cl
	*(DWORD*)0x43E849 = (DWORD)inv_sorted+1;
	//
	//SHOW (43EA90)
	//0043EBAD BA 19 1A 9C 01       mov         edx,19C1A19h  
	*(DWORD*)0x43EBAE = (DWORD)inv_sorted+1;
	//0043EBBE BB F2 6F 56 00       mov         ebx,566FF2h  
	*(DWORD*)0x43EBBF = (DWORD)prm_sorted+2;
	//0043ED13 81 FA 0D 1C 9C 01    cmp         edx,19C1C0Dh
	*(DWORD*)0x43ED15 = (DWORD)inv_sorted+1+min(N,250)*2;
	//0043EE0B 81 C1 11 70 56 00    add         ecx,567011h
	*(DWORD*)0x43EE0D = (DWORD)prm_sorted+33;

	//OBSOLETE SUBROUTINE?
	//this subroutine unequips anything that's not in the inventory
	//and calls 427780 afterward. I don't believe it's called other
	//than here. aside from 427780 selling equipment is disallowed
	//and it should be possible to equip items not in the inventory
	//NOTE: 427780 is (also) called by the equip screen on its exit
	//seems like it cannot possibly do anything that cannot be done
	//by the frame immediately after the shop is exited. if so then
	//it can be called after 43ce80 exits
	if(1)
	{
		//TODO: 43D0A5 CALLS THIS (GHIDRA FINDS NO OTHER SOURCES)

		//the SELL option won't sell equipped items, and events do
		//not remove equipment
		/*2021: som_game_reprogram knocks this out with 4275e0
		memset((void*)0x427120,0x90,0x427164-0x427120);*/
		assert(0x90==*(BYTE*)0x43D0A5);
	}
	else
	{
	/*
	00427120 53                   push        ebx  
	00427121 33 C9                xor         ecx,ecx  
	00427123 BA FF 00 00 00       mov         edx,0FFh  
	00427128 33 C0                xor         eax,eax  
	0042712A 8A 81 0C 1C 9C 01    mov         al,byte ptr [ecx+19C1C0Ch]  
	00427130 3A C2                cmp         al,dl  
	00427132 74 13                je          00427147  
	00427134 23 C2                and         eax,edx  
	00427136 8A 1C 45 19 1A 9C 01 mov         bl,byte ptr [eax*2+19C1A19h]  
	*/
		*(DWORD*)0x427139 = (DWORD)inv_sorted+1;
	/*
	0042713D 84 DB                test        bl,bl  
	0042713F 75 06                jne         00427147  
	00427141 88 91 0C 1C 9C 01    mov         byte ptr [ecx+19C1C0Ch],dl 
	00427147 41                   inc         ecx  
	00427148 83 F9 07             cmp         ecx,7  
	0042714B 7C DB                jl          00427128

			2021: 0,0 should be 255 here

	0042714D 6A 00                push        0  
	0042714F 6A 00                push        0  
	00427151 E8 8A 04 00 00       call        004275E0  
	*/
		memset((void*)0x427151,0x90,5);
	/*
	00427156 83 C4 08             add         esp,8  
	00427159 E8 12 00 00 00       call        00427170  
	*/
		memset((void*)0x427159,0x90,5);
	/*
	0042715E E8 1D 06 00 00       call        00427780  
	00427163 5B                   pop         ebx  
	00427164 C3                   ret  
	*/
	}

	VirtualProtect((void*)text,text_s,old,&old);
}	
static void som_game_menu_init()
{		
	using namespace som_game_menu;

	//these don't really belong here, but there's no use for
	//another subroutine
	BYTE *p = (BYTE*)SOM::L.item_prm_file->c;
	for(int i=0;i<250;i++,p+=336) 
	if(0xFFFF==*(WORD*)p)
	{
		//HACK: SOM_SYS doesn't 0 out items that are removed with 
		//SOM_PRM. technically SOM_SYS isn't required to be saved
		SOM::L.pcstore[i] = 0;
	}
	else //normalize non-description for _appraiser_filter?
	{	
		for(int i=33;i<33+241;i++) switch(p[i])
		{
		//Shift-JIS
		case '\x81': if('\x40'!=p[i+1]) //'\x81\x40'
		{
			default: goto nonempty;
		}
		else i++; //U+3000
		case '\x80': case '\xA0': //U+0080 //U+00A0		
		//ASCII
		case '\t': case ' ': case '\r': case '\n': 
		continue; 
		case '\0': goto empty;
		}
		empty: p[33] = '\0'; nonempty:; 
	}
	//HACK: this memory doesn't appear to ever be used, but is
	//set to 0 at some point???
	SOM::L.item_pr2_file[250].c[62] = 8; //585A6E	
	//HACK: initialize som_game_nothing, etc.
	//REMINDER: FOR SOME REASON SOM::equip IS
	//VERY VOLATILE HERE. IT CANNOT BE CALLED
	//FROM INSIDE CreateFileA. NO CLUE WHY???		
	//som_game_equip(); 
	som_game_nothing(); //2022

	//#ifdef _DEBUG
	if(1) som_game_menu_reprogram();
	//#endif
	_prm_sort(0);	
	for(int i=1;i<=7;i++)
	items[i+2] = find_item(SOM::L.pcequip[i-1]);	
	items[0] = find_magic(SOM::L.pcequip[7]);

	SOM::Shops &s = SOM::L.shops;
	for(int i=128;i-->0;) 
	if('\0'==*s[i].title) 
	{
		som_game_empty_shop = s+i; strcpy(s[i].title,"EX");
		break;
	}							
}
