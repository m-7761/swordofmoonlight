#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

#include <algorithm>

#include "dx.dsound.h"

#include "Ex.ini.h"
#include "Ex.input.h"
#include "Ex.output.h"
#include "Ex.window.h"
#include "Ex.cursor.h"

#include "SomEx.h"
#include "som.932.h"
#include "som.932w.h"
#include "som.game.h"
#include "som.tool.h"
#include "som.title.h"
#include "som.status.h"
#include "som.files.h"
#include "som.state.h"
#include "som.extra.h"

#define SOMVECTOR_MATH
#include "../Somplayer/Somvector.h"
#include "../lib/swordofmoonlight.h"
namespace evt = SWORDOFMOONLIGHT::evt;

namespace DINPUT
{
	extern unsigned noPolls;
}
namespace DDRAW
{
	extern float stereo;

	extern BOOL xr;
	extern bool gl,inStereo,isPaused;
	extern bool stereo_toggle_OpenXR(bool=true);

	extern float xyScaling[2],xyMapping[2];

	extern unsigned noFlips,noTicks,noResets;

	extern unsigned dejagrate_divisor_for_OpenXR;
}

extern int //see som.game.cpp & som.tool.cpp
SOM::tool = SOM::tool, SOM::game = SOM::game;
extern bool SOM::retail = false;

extern bool &SOM::f3 = EX::output_overlay_f[3];

extern bool SOM::emu = false;
extern const int &SOM::log = EX::logging_onoff(!'flip');

extern unsigned SOM::eventick = 0;
extern int SOM::event = 0;
extern int SOM::eventapped = 0, SOM::eventype = 0; 
extern unsigned SOM::et = 0, SOM::eticks = 0; 

extern bool SOM::field = false, 
SOM::play = true, SOM::padtest = false,
SOM::player = true;

extern int SOM::recording = 0;
extern SOM::Context SOM::context(0);
extern SOM::Motions SOM::motions(0);
extern SOM::Clipper SOM::clipper(0);
//extern SOM::Climber SOM::climber(0);

extern float SOM::fov[4] = {};
extern FLOAT SOM::xyz[3] = {};
extern FLOAT SOM::uvw[3] = {};
extern FLOAT SOM::hmd[3] = {};
extern FLOAT SOM::pos[3] = {};
extern float SOM::pov[4] = {};
extern float SOM::eye[4] = {};
extern float SOM::cam[4] = {};
extern float SOM::arm[3] = {};
extern float SOM::err[4] = {};

extern float SOM::xyz_past[3] = {};
extern float SOM::xyz_step = 0;

extern float SOM::analogcam[4][4] = {{}};
extern float SOM::steadycam[4][4] = {{}};
extern float SOM::stereo344[3][4][4] = {};

extern float SOM::heading = 0, SOM::incline = 0;
extern float SOM::slope = 0, SOM::arch = 0;

extern bool SOM::ezt[1024] = {};

extern BYTE SOM::mpx = 0, SOM::mpx2 = 0;
extern bool SOM::sky = 0, SOM::skyswap = 0;
extern DWORD SOM::clear = 0;
extern float //SOM::fog[2] = {},
SOM::fogstart = 0, SOM::fogend = 0,
SOM::skystart = 0, SOM::skyend = 0,
SOM::decibels[3] = {}, SOM::doppler[3] = {};
//2022: storing in som_MPX_swap::maps
//extern BYTE *SOM::ambient2[7+1] = {};
extern BYTE **SOM::ambient2 = 0;
extern float SOM::mpx2_blend = 0; //2022
extern float SOM::mpx2_blend_start = 0;
extern int SOM::millibels[3] = {};
extern void SOM::config_bels(int mask)
{
	int v[3] = {SOM::bgmVol,SOM::seVol,max(0,SOM::masterVol)};
	for(int i=3;i-->0;) if(mask&1<<i)
	{
		if(1) //new way? //https://stackoverflow.com/questions/15883611/
		{
			/*NOTE: I don't see why minAtt can't be 0 other than
			//the fact that log(0) is invalid or not finite
			//const float doubleValue = 10; //log10
			//float minimumAttenuation = 1/pow(2,100/doubleValue);
			const float minAtt = 0.0009765625;
			float att = minAtt+(v[i]/255.0f)*(1-minAtt);*/
			//db = doubleValue * ln(attenuation) / ln(2)
			SOM::decibels[i] = v[i]?logf(v[i]/255.0f)/logf(2)*10:-100;
		}
		else SOM::decibels[i] = -100+v[i]/255.0f*100;

		SOM::millibels[i] = (int)(100*SOM::decibels[i]);
	}
}

extern HICON SOM::icon = 0;
extern HWND SOM::window = 0;
extern WNDPROC SOM::WindowProc = 0; 
extern const wchar_t *SOM::caption = 0;

extern int 
SOM::device = 0,
SOM::width = 0, SOM::height = 0,
SOM::bpp = 0, &SOM::filtering = SOM::L.filter, //1
SOM::gamma = 8, SOM::seVol = 255, 
SOM::bgmVol = 255, 
//extended
SOM::opening = 0, SOM::windowX = 0, SOM::windowY = 0, 
SOM::cursorX = 0, SOM::cursorY = 0, SOM::cursorZ = 0,
SOM::capture = 0, 
SOM::analogMode = 0, SOM::thumb1 = 0, SOM::thumb2 = 0,
SOM::buttonSwap = 0,
SOM::map = 0, SOM::mapX = 0, SOM::mapY = 0, SOM::mapZ = 0,
SOM::ipd = -1, SOM::stereo = 0, SOM::stereoMode = 0,
SOM::zoom = 0, //50 is the legacy value
SOM::masterVol = 255, //16
SOM::superMode = 0,
SOM::opengl = 0;
extern BYTE &SOM::bob = SOM::L.on_off[0];
extern BYTE &SOM::showGauge = SOM::L.on_off[1]; 
extern BYTE &SOM::showCompass = SOM::L.on_off[2];

extern HFONT SOM::font = 0;
extern const char *SOM::fontface = som_932_MS_Mincho; 
extern const wchar_t *SOM::wideface = som_932w_MS_Mincho;

extern int SOM::red = 0;
extern float SOM::red2 = 0; //2020
extern void *SOM::hit = 0;
extern unsigned SOM::invincible = 0;

extern bool 
&SOM::paused = DDRAW::isPaused;
extern unsigned 
&SOM::frame = DDRAW::noFlips,
//2017: SomEx.cpp initializes SOM::frame to be more
//than zero to eliminate ambiguity in the beginning
SOM::newmap = 0, SOM::option = 0,
SOM::padcfg = 0, SOM::dialog = 0, 
SOM::taking = 0, SOM::saving = 0,
SOM::hpdown = 0, SOM::pcdown = 0,
SOM::warped = 0, SOM::ladder = 0,
SOM::mapmap = 0, SOM::automap = 0,
SOM::doubleblack = 0, SOM::black = 0,
SOM::altdown = 0,
SOM::alt = 0, SOM::alt2 = 0, SOM::ctrl = 0, SOM::shift = 0, SOM::space = 0,
SOM::altf_mask = 0,
SOM::altf = 0, SOM::limbo = 0, SOM::crouched = 0,
SOM::shoved = 0, SOM::bopped = 0,
SOM::swing = 0, SOM::counteratk = 0;

extern int SOM::tilt = 0; //mouse
extern int SOM::se_looping = 0; //2023
extern int SOM::se_volume = 0;

//extern SOM::TextureAtlasRec SOM::TextureAtlas[1024][2] = {}; //UNUSED

//TODO
//think probably this is 0x19AA978
//004183F8 68 78 A9 9A 01       push        19AA978h
extern DWORD *SOM::menupcs = (DWORD*)0x1A5B4A8;
extern struct SOM::L SOM::L = struct SOM::L();
extern const EX::Section *SOM::memory = 0,
*SOM::text=0,*SOM::rdata=0,*SOM::data = 0;
extern const EX::Program *SOM::disasm = 0;

extern FLOAT SOM::f3state[3] = {};
extern FLOAT *SOM::g = 0, *SOM::u = 0, *SOM::v = 0,
*SOM::cone = 0, *SOM::stool = 0, SOM::u2[3] = {0,0,0};
//4466C0 is som_db.exe's vector normalization subroutine.
void __cdecl som_state_4466C0(FLOAT *x, FLOAT *y, FLOAT *z)
{
	if(0||!EX::INI::Option()->do_u2)
	((void(__cdecl*)(FLOAT*,FLOAT*,FLOAT*))0x4466C0)(x,y,z);
}

//som.mocap.cpp
extern void som_state_pov_vector_from_xyz3_and_pov3()
{
//	EX::dbgmsg("uvw: %.3f %.3f %.3f",SOM::uvw[0],SOM::uvw[1],SOM::uvw[2]);
//	EX::dbgmsg("hmd: %.3f %.3f %.3f",SOM::hmd[0],SOM::hmd[1],SOM::hmd[2]);
	auto &Euler = DDRAW::inStereo?SOM::hmd:SOM::uvw;
	float q[4]; Somvector::map(q).quaternion(Euler);	
	auto &p = Somvector::series(SOM::pov,0,0,1,0).rotate<3>(q);

	SOM::pov[3] = 0; //dot
	for(int i=3;i-->0;)	
	SOM::pov[3]+=SOM::pov[i]*(SOM::cam[i]-SOM::pov[i]*3);
}

extern bool SOM::GAME_INI = false;
extern bool SOM::config_ini()
{
	if(SOM::GAME_INI) return false; SOM::GAME_INI = true;

	//HACK: move me 
	if(DDRAW::inStereo) SOM::PSVRToolbox("EnableCinematicMode");

	/*HMM: OVERWRITES ENTIRE SECTION		
	const wchar_t f[] = 	
	L"zoom=%d%lc"
	L"opening=%d%lc"
	L"windowX=%d%lc"L"windowY=%d%lc"
	L"cursorX=%d%lc"L"cursorY=%d%lc"L"cursorZ=%d%lc"L"capture=%d%lc";
	L"%ls=%d%lc"; //analogMode
	wchar_t config[8*33+EX_ARRAYSIZEOF(f)] = L"\0";
	swprintf_s(config,f,
	SOM::zoom,0,
	SOM::opening,0,
	SOM::windowX,0,SOM::windowY,0,
	SOM::cursorX,0,SOM::cursorY,0,SOM::cursorZ,0,SOM::capture,0,
	!EX::INI::Joypad()?L"analogMode":L"",SOM::analogMode,0); //TRICKY
	WritePrivateProfileSection(L"config",config,SOM::Game::title('.ini'));
	return true;*/
	wchar_t w[32];
	const wchar_t *section = L"config";
	const wchar_t *ini = SOM::Game::title('.ini');
#define _(A) WritePrivateProfileStringW\
	(section,L#A,_itow(SOM::##A,w,10),ini);
	_(opengl)
	_(zoom)
	_(stereo)
	_(stereoMode)
	if(SOM::ipd!=-1) //yuck
	_(ipd)
	_(opening)
	_(windowX)
	_(windowY)
	_(cursorX)
	_(cursorY)
	_(cursorZ)
	_(capture)
	_(analogMode)
	_(buttonSwap)
	_(masterVol)
	_(superMode)
	section = L"rescue";
	_(map)
	_(mapX)
	_(mapY)
	_(mapZ)
#undef _
	return true;
}

extern bool SOM::x480() //REMOVE ME
{
	return SOM::height<=480; //SOM::fov[1]/DDRAW::xyScaling[1]<=480+1;
}			 
 		
static int som_state_default_thumb(int thumb, const EX::INI::escape_analog_mode *am)
{
	assert(SOM::analogMode);

	int a = 8, b = 8;	
	for(int i=0;i<8;i++) if(am[i].inplay)
	{
		int k = am[i].motion;
		int v = am[i].axisid;

		if(thumb==1) switch(k)
		{
		case 0: a = v; break;		
		case 2: b = v; break;
		}
		else switch(k) //2
		{
		case 3: a = v; break;
		case 4: b = v; break;		
		}
	}
	//360 marks it as a default
	return 36000+a*10+b;
}
static void som_state_thumbs_boost(int i, float &pa, float &pb, float dza, float dzb)
{
	//static inline float analog_scale(int i) 
	//case 0x1f: return 0.75f;     //3/4 //0.707106f?	
	const float boost = 0.75f/0.707106f;
	
	//HACK: this boosts the diagonals of the cirlce
	//so it hits gait 5
	//the gaits still fluctuate wildly depending on
	//rounding off to bars
	float xy[2] = {pa,pb};
	if(float len=Somvector::map<2>(xy).length<2>())
	{
		pa/=len; pb/=len;

		//SOM::thumbs assumes this approach
		//
		//if(1) //new way? even scale (over 1)
		{
			//TODO? consult escape_analog_gaits
			//TODO? consult escape_analog_gaits
			//TODO? consult escape_analog_gaits
			
			//2020: trying to leave small values
			//alone to minimize backfire jumping
			//len*=boost;
			len*=min(boost,1+pow(boost-1,2-0.05f-len));
		}
		/*else //old way: boost diagonals only
		{	
			float c = fabsf(atan(pb/pa));
			float f = sinf(2*fmodf(c,M_PI_2));
			float g = 0.1f*f; //powf(f,4);
			len*=1+g;
			if(i==1)
			EX::dbgmsg("SOM::thumbs %f (%f)",g,c/M_PI*180);
		}*/
		pa*=len; pb*=len;	
	}

	/*my controller is just unusually sticky lately
	//HACK: enlarge deadzone proportionate to boost
	//dza = (dza*boost-dza); dzb = (dzb*boost-dzb);
	pa+=pa<0?dza:-dza; pb+=pb<0?dzb:-dzb;*/
}
extern bool som_state_square_uv = true; //testing
extern bool som_thumb_boosted()
{
	return !som_state_square_uv;
}
static bool SOM::thumbs(int jp, float p[8])
{	
	if(jp!=0||!SOM::analogMode) //do_escape
	return false; 
	
	bool off = 0&&EX::debug&&GetKeyState(VK_CAPITAL); //SOM::thumbs	
		
	EX::INI::Detail dt;
	const EX::INI::escape_analog_mode *am = 0;
	if(jp==0)
	am = dt->escape_analog_modes[abs(SOM::analogMode)-1];
	else assert(0);

	bool boost = false; //HACK

	const float(&shape)[5] = dt->escape_analog_shape; //2020

	float t = shape[0]; if(t<0) //-1? choose default?
	{
		if(EX::Joypads[jp].isDualShock)		
		{
			t = 0; //DS4 is a known unit circle
		}
		else if(EX::Joypads[jp].isXInputDevice) 
		{
			t = 0; //a likely circle? think so?
		}
		else //DirectInput or DualShock3 only?
		{
			//from reading online maybe the DS3
			//is unique for being square. but I
			//have recollections of controllers
			//being square before XInput. Still
			//my DS3 isn't perfectly square, so
			//this splits the difference

			t = 0.75f; //old style square-ish?
		}
	}

	//xz seems alright at this stage... still 
	//sometimes doing comparison tests for uv
	som_state_square_uv = off;
	if(!off) for(int i=1;i<=2;i++)
	{
		int &thumb = i==1?SOM::thumb1:SOM::thumb2;
		//2018: try to guess at defaults so the axes
		//can be paired for the square->circle mapping
		if(0==thumb) //continue;
		{
			if(0!=jp)
			{
				assert(0); continue;
			}
			else thumb = som_state_default_thumb(i,am);
		}
		int a = thumb%100/10, b = thumb%10;
		int deg = thumb/100;
		float r = deg==360?0:deg*M_PI/180;
		if(r<0) a = -a, b = -b;
		//som_state_default_thumb dummy?
		if(a>7||b>7) continue;

		//HACK: might want to rescale the axes?
		if(i==1) p[a]+=shape[1]; 
		if(i==1) p[b]+=shape[2];
		if(i==2) p[a]+=shape[3]; 
		if(i==2) p[b]+=shape[4];
	
		if(deg==360) //som_state_default_thumb?				
		for(int pass=1;pass<=2;pass++) 
		for(int j=0;j<8;j++)
		{
			int &ab = pass==1?a:b;
			if(am[ab].motion==am[j].motion&&am[j].inplay)
			if(fabs(p[am[j].axisid])>fabs(p[ab]))
			ab = am[j].axisid;
		}

		if(a==b){ assert(a!=b); continue; }
		
		float pa = p[a], pb = p[b], ppa=pa,ppb=pb; 
		//HACK: DS4 is known to be a unit circle
		//if(!EX::Joypads[jp].isDualShock) 
		if(t!=0) 
		{			
			//without this correction, the following
			//rotation can't work... but what if the
			//controller is a circle? Is it ever so?

			float x = pa, y = pb;
			//munge square coordinates to a circle??
			//https://stackoverflow.com/questions/1621831/how-can-i-convert-coordinates-on-a-square-to-coordinates-on-a-circle
			/*if(0) //something's off
			{
				pa = x*sqrt(1-y*y/2);
				pb = y*sqrt(1-x*x/2);
			}
			else*/ if(1&&pa&&pb) //better
			{
				//REMINDER... different powers don't seem
				//to work... or tried reciprocal for sqrt
				//http://squircular.blogspot.com/2015/09/fg-squircle-mapping.html
				float xx=x*x,yy=y*y;
				float s = sqrt(xx+yy-xx*yy)/sqrt(xx+yy);
				//HACK: lerp is a compomise, since the DualShock4's shape is 
				//almost a perfect circle. Even though other controllers are
				//nearly square, their points are still in terms of a square
				//float t = 0.75f;
				pa = x+(x*s-x)*t; //x*t;
				pb = y+(y*s-y)*t; //y*t;
			}
		}

		float c = cosf(r), s = sinf(r); 
		p[a] = pa*c+pb*s;		
		p[b] = pa*-s+pb*c;
		
		//doing both with alternative method
		EX::INI::Joypad &ini = EX::Joypads[jp].ini;
		som_state_thumbs_boost(i,p[a],p[b],*ini->analog_gaits(a),*ini->analog_gaits(b));
		if(jp==0&&!&dt->escape_analog_gaits)
		boost = true;

		//HACK: som_mocap::smooth is not ready for this :(
		if(som_state_square_uv&&deg==360)
		{
			if(am[a].motion>=3) p[a] = ppa;
			if(am[b].motion>=3) p[b] = ppb;
		}
	}

	if(0&&EX::debug) //SOM::thumbs
	{
		//float x = atan(p[1]/p[0])/M_PI*180;
		float x = Somvector::map<2>(p).length<2>();		
		EX::dbgmsg("thumbs %0.4f %0.4f (%f)",p[0],p[1],x);
	}
	else if(off) EX::dbgmsg("SOM::thumbs VK_CAPITAL toggled");

	return boost; //HACK: use 1 for outer deadzone
}

extern bool SOM::escape(int shift2)
{
	EX::INI::Option op;
	EX::INI::Detail dt;
	if(!op->do_escape) return false;

	//incompatible with SOM::analogMode
	if(EX::INI::Joypad()) return false;
					 
	if(SOM::frame==SOM::frame0) //initialize
	{
		assert(0==shift2);
		assert(op->do_escape);
		
		if(op->do_u2) //HACK/temporary fudge
		{
			//REMOVE ME?
			//this is intended for turning with
			//u2_power set to 1.5 in order to be
			//able to smoothly transition gaits 2 
			//and 3
			EX::Joypad::analog_scale_3 = 0.48f; //0.475f;
			EX::Joypad::analog_scale_4 = 0.62f; //0.625f;
		}

		EX::Joypad::thumbs = SOM::thumbs; //2017		
		SOM::thumb1 = SOM::config("thumb1",0); 
		SOM::thumb2 = SOM::config("thumb2",0);

		//2021: 2 is better, it will default to 1 if out-of-range
		if(SOM::analogMode=SOM::config("analogMode",2)) //1
		if(abs(SOM::analogMode)-1>=dt->escape_analog_modesN)			
		SOM::analogMode = min(1,dt->escape_analog_modesN); //out of range			
	}
	else //assuming Esc key was pressed
	{
		assert(1==abs(shift2));

		bool shift = shift2==-1; 

		SOM::altf|=1; //HACK: 1<<1 is F1

		if(SOM::analogMode<0)
		{
			SOM::analogMode = -SOM::analogMode;
			if(!shift) SOM::analogMode++;
		}	 
		else if(SOM::analogMode>0) //see if v is present; if so invert
		{
			if(!shift||--SOM::analogMode) invertible:
			{
				for(int i=0;i<8;i++)
				if(dt->escape_analog_modes[SOM::analogMode-1][i].motion==4) //v
				{
					SOM::analogMode = -SOM::analogMode; break; //invertible
				}
				if(!shift)
				if(SOM::analogMode>0) SOM::analogMode++; //not invertible
			}
		}
		else if(shift)
		{
			SOM::analogMode = dt->escape_analog_modesN;
			goto invertible;
		}
		else SOM::analogMode = 1;

		if(abs(SOM::analogMode)-1>=dt->escape_analog_modesN)
		{
			SOM::analogMode = 0; //ANALOG DISABLED
		}
	}

	//som_state_default_thumb
	if(360==SOM::thumb1/100) SOM::thumb1 = 0;
	if(360==SOM::thumb2/100) SOM::thumb2 = 0;

	EX::Joypad &jp = EX::Joypads[0];

	unsigned short *a = jp.cfg.a(); //hack

	for(int i=0;i<8;i++) 
	{
		jp.cfg.analog_mode[i][0] = 1;

		a[i*EX::contexts] = a[(8+i)*EX::contexts] = 0;
	}
	
	if(!SOM::analogMode) //disabled?
	{
		jp.cfg.analog_mode[0][0] = 1;
		jp.cfg.analog_mode[1][0] = 1;

		jp.cfg.neg_pos[0][0] = 0x4f; //DIK_NUMPAD1;
		jp.cfg.pos_pos[0][0] = 0x51; //DIK_NUMPAD3;		
		jp.cfg.neg_pos[1][0] = 0x4c; //DIK_NUMPAD5;
		jp.cfg.pos_pos[1][0] = 0x50; //DIK_NUMPAD2;

		return true; //classic SOM setup
	}

	assert(SOM::analogMode);
	const EX::INI::escape_analog_mode *mode = 
	dt->escape_analog_modes[abs(SOM::analogMode)-1];

	for(int i=0;i<8;i++) if(mode[i].inplay)
	{			
		int neg = 0, pos = 0;

		switch(mode[i].motion) 
		{
		case 0: neg = 0x4B; pos = 0x4D; break; //x:NUMPAD4/NUMPAD6
		case 1: neg = 0xC9; pos = 0xD2; break; //y:PRIOR/INSERT
		case 2: neg = 0x4C; pos = 0x50; break; //z:NUMPAD5/NUMPAD2
		case 3: neg = 0x4F; pos = 0x51; break; //u:NUMPAD1/NUMPAD3
		case 4: neg = 0x49; pos = 0x47; break; //v:NUMPAD9/NUMPAD7

		default: assert(0); continue; //???
		}
	
		int swap = mode[i].invert; 

		if(SOM::analogMode<0&&mode[i].motion==4) swap = !swap; //v
			
		if(swap) std::swap(pos,neg);

		int axis = mode[i].axisid;

		jp.cfg.analog_mode[axis][0] = 1;

		switch(axis)
		{
		case 0: case 1:	case 2:

			jp.cfg.pos_pos[axis][0] = pos; 
			jp.cfg.neg_pos[axis][0] = neg; break;

		case 3:	case 4:	case 5: axis-=3;

			jp.cfg.pos_rot[axis][0] = pos; 
			jp.cfg.neg_rot[axis][0] = neg; break;

		case 6:	case 7: axis-=6;

			jp.cfg.pos_aux[axis][0] = pos; 
			jp.cfg.neg_aux[axis][0] = neg; break;
		}
	}

	EX::INI::Keymap()->xxtranslate(a,8*2*EX::contexts);

	return true; 
}
extern bool SOM::altf1() //super sampling mode (2021)
{
	SOM::altf|=1<<1; //probably not much else to do?

	return true; //whatever I guess??
}
extern float SOM::ipd_center()
{
	return  0.0630999878f/2;
}
static void DDRAW__stereo___SOM__ipd()
{		
	//this is more like a binocular effect
	//0.001 happens to be 1mm but I don't know
	//right now if DDRAW::stereo is X millimeters
	if(SOM::ipd) //not mono (cyclops) mode?
	{
		DDRAW::stereo = SOM::ipd*0.001f;
		//if(2==EX_INI_STEREO_MODE)
		{
			//float middle = 63/2.0f-15;
			float middle = SOM::ipd_center()-0.015f;

			/*I think it's correct, but lacks depth cues like a 
			//body... standing near an NPC helps to see clearly
			//juding by KF2's green caves, I think
			//think this is far too much
			middle/=2; DDRAW::stereo/=2;
			*/

			DDRAW::stereo+=middle; //*0.001f;
		}
	}
	else DDRAW::stereo = 0.0001f; //nonzero
}
extern int som_hacks_shader_model;
extern void som_mocap_PSVR(SOM::Sixaxis[2],int);
extern void Ex_detours_d3dfonts_release_d3d_interfaces();
extern bool SOM::altf2() //stereo mode
{	
	//this is helpful if sets get stuck in sleep
	//mode and covers controller inputs
	if(DDRAW::isPaused)
	{
		SOM::Unpause(); assert(!DDRAW::isPaused);
	}

	if(SOM::altf_mask&(1<<2)) return false;

	//HACK: trying with different font size
	if(EX::stereo_font)
	Ex_detours_d3dfonts_release_d3d_interfaces();

	if(DDRAW::gl) //OpenXR?
	{
		SOM::altf|=1<<2;

		if(SOM::frame!=SOM::frame0)
		{
			SOM::stereo = !SOM::stereo;
		}
	}
	else if(SOM::frame!=SOM::frame0) 
	{
		SOM::altf|=1<<2;

		//just report not supported
		if(som_hacks_shader_model<3)
		return true;

		SOM::stereo = !SOM::stereo;

		if(SOM::stereo) 
		{
			//HACK: recalibrate while changing modes?
			som_mocap_PSVR(0,2);
		}
	}
	else //YUCK: PSVR at startup?
	{
		//FIX ME
		//I feel like there's a good chance this will be in error
		//if the shader model doesn't allow stereo
		//SHOULDN'T BE HERE IF som_hacks_shader_model ISN'T SETUP
		SOM::PSVRToolbox(SOM::stereo?"EnableVRMode":"EnableCinematicMode");

		if(SOM::ipd==-1) 
		{
			SOM::ipd = SOM::config("ipd",0);
			//DDRAW::stereo = SOM::ipd;
			DDRAW__stereo___SOM__ipd();
		}
	}

	return true;
}
//the best way to test this is to look at a wall and look 
//up and down and see if it seems to draw nearer. perhaps
//94 is not precise enough, but it seems best for current
//settings, without going float
extern float SOM::zoom2()
{
	//68 is subject to change/probably not the right value
	//THERE MAY NOT EVEN BE "a" VALUE but probably the PSVR
	//is symmetrical (though eyes may not)
	//https://developer.oculus.com/blog/tech-note-asymmetric-field-of-view-faq/
	//trying 75, PSVRToolbox's author says 68 is horizontal
	//75 actually feels good... setting a little lower since
	//the matte effect is downsizing the image
	//may not be 100% accurate. People seem a little bit thin
	return DDRAW::inStereo?(float)EX::INI::Stereo()->zoom:SOM::zoom;
}
extern bool SOM::altf3(int shift) //zoom mode
{
	//HACK: initilializing //???
	static char modes[] = { 30,35,40,45,50,0,0,0 };
	if(!SOM::zoom)
	{
		//King's Field II is closer to 62 than 63
		modes[5] = 62; 
		//2020: defaulting to 30? I guess so? 50 and below
		//give the impression of being close to the ground
		//compared to arm size. for that reason I'm making
		//62 (KF2) the new default even though it's skewed
		//SOM::zoom = max(30,SOM::config("zoom",0));
		SOM::zoom = max(30,SOM::config("zoom",62));
		if(!strchr(modes,SOM::zoom))
		{
			modes[6] = SOM::zoom; std::sort(modes,modes+7);
		}
		return true;
	}

	SOM::altf|=1<<3;

	if(DDRAW::xr) 
	{
		auto &d = DDRAW::dejagrate_divisor_for_OpenXR;
		int i = (int)d+10*shift;
		i = i/10*10;
		//6500 should cover an 8k 4320 tall VR display
		//using the same algorithm my HP Reverb G2 has
		d = (unsigned)min(6500,max(0,i)); 
		return true;
	}

	if(DDRAW::inStereo) 
	{
		SOM::ipd+=shift;
		if(SOM::ipd<0) SOM::ipd = 30; //0;
		if(SOM::ipd>30) SOM::ipd = 0; //30;
		//DDRAW::stereo = SOM::ipd;
		DDRAW__stereo___SOM__ipd();

		return true;
	}

	char *p = strchr(modes,SOM::zoom);
	p+=shift;
	int len = strlen(modes);
	if(p<modes) p = modes+len-1;
	if(p==modes+len) p = modes;

	SOM::zoom = *p; return true;
}	
extern SOM::Thread *som_MPX_thread;
extern bool SOM::altf4(int code) //terminate program
{	
	if(!code&&!EX::debug) //2023: matching Alt+F5
	{
		if(!SOM::retail||SOM::alt2==SOM::frame)
		if(IDOK!=MessageBoxW(EX::display(),L"Leave this game?",L"SOM_DB",MB_OKCANCEL))
		{
			return false;
		}
	}

	bool one_off = SOM::altf&1<<4;

	EXLOG_LEVEL(3) << "SOM::altf4(" << one_off << ")\n";

	if(one_off) return false;

	SOM::altf|=1<<4;

	//2018: taskbar doesn't call SOM::config_ini
	SOM::Unpause(); 

	//SOM::config_ini is doing this. Is altf4 not always done?
	//if(SOM::stereo) SOM::PSVRToolbox("EnableCinematicMode");
												
	//mute after window closes
	DSOUND::Stop(); EX::sleep(20);

	//closing from title screen
	if(!SOM::field) SOM::config_ini(); 
	if(!SOM::field) SOM::record_ini();
	
  /////COAX SOM INTO FLUSHING INI FILE////

	//do_escape: see spam(DIK_ESCAPE) below
	SOM::context = SOM::Context::game_over; 
				
	EX::Syspad.send(0x38)+(0x3E); //ALT+F4

	if(!SOM::field)
	{
		//Mute new sounds generated by Escape spam
		DSOUND::knocking_out_delay(EX::tick()+60);
	
		EX::Syspad.spam(0x01); //ESCAPE (for intros)
	}

	EX::Syspad.exit(0);	//dicey

	//permit SOM a second to do its business
	SetTimer(SOM::window,'exit',1000,0);

	EX::abandoning_cursor();
	//EX::destroying_window(); //2021: SOM::window is a child now?
	SetWindowRedraw(SOM::window,0); //2021
	ShowWindow(EX::display(),SW_HIDE); //2021

	//2022: I crashed with invalid iterators in the per map model
	//vectors. I can't remember if I was mid exit or not, but it
	//seems like the "main" teardown could break ref counting
	som_MPX_thread->close();

	if(DDRAW::xr) DDRAW::stereo_toggle_OpenXR(); //avoid crashing

	return true;
}
extern bool SOM::altf5()
{
	if(EX::debug) goto dbg;

	if(!SOM::retail||SOM::alt2==SOM::frame)
	if(IDOK==MessageBoxW(EX::display(),L"Reload this level?",L"SOM_DB",MB_OKCANCEL))
	{	
dbg:	auto &dst = *SOM::L.corridor;

		dst.lock = 1; dst.nosetting = 2; //som_files_wrote_db

		return true;
	}
	return false;
}
extern void EX::play(int w)
{
	//this is because the map reload may be triggered by
	//saving the maps (note, that was developed first)
	if(w==SOM::mpx&&SOM::frame-SOM::newmap<120*10) return; 

	auto &dst = *SOM::L.corridor;

	dst.map = w;
	dst.lock = 1; dst.nosetting = w==SOM::mpx?2:1; //som_files_wrote_db

	SetForegroundWindow(EX::display());
	SetActiveWindow(EX::display());
}
extern bool SOM::f10() //Alt-like system key
{	 	
	if(SOM::capture||EX::is_captive()) //weak capture
	{	
		EX::abandoning_cursor();

		SOM::capture = 0;
	}

	//TESTING
	//helps with weird no-release F10 key
	EX::activating_cursor(); 
	
	EX::showing_cursor('z');	

	EX::following_cursor(); return true;
}

extern bool SOM::z(bool down)
{
	static bool upswing = false;

	if(!upswing)
	{
		if(!SOM::window) return false;

		if(EX::pointer!='z'||!EX::inside) return false;	

		HWND cap = GetCapture();  

		POINT mouse; if(!GetCursorPos(&mouse)) return false;

		RECT trap; if(!GetWindowRect(SOM::window,&trap)) return false;

		bool ok = cap&&cap==EX::window||cap&&cap==SOM::window;

		if(!ok)	ok = GetWindowRect(SOM::window,&trap)&&PtInRect(&trap,mouse);

		if(!ok) return false; if(!down) return true;

		upswing = true; //wait for up

		EX::hiding_cursor();
		EX::holding_cursor();
	}
	else
	{
		upswing = false;

		if(!SOM::capture)
		if('hand'!=EX::pointer) //HACK
		{			
			SOM::cursorX = EX::x;
			SOM::cursorY = EX::y;

			EX::capturing_cursor();	
			EX::clipping_cursor();

			SOM::capture = 1;
		}
		else EX::showing_cursor();
	}

	EX::following_cursor(); return true;
}
 
extern bool SOM::windowed = false;
extern bool SOM::altf11()
{
	if(SOM::windowed)
	{	
		EX::styling_window(!EX::fullscreen,SOM::windowX,SOM::windowY);
	}
	else //NEW: provide a way to minimize. TODO? off screen title bar
	{
		//NOTE: CONTROL SUPPRESSES F11 IN fullscreen MODE SO IT DOESN'T
		//ACCIDENTALLY MINIMIZE
		SendMessage(GetAncestor(SOM::window,GA_ROOT),WM_SYSCOMMAND,SC_MINIMIZE,0);
	}

	return true;
}

extern bool SOM::altf12(int shift)
{
	if(shift==2) shift = 0; //SomEx_fbuf_remember?

	SOM::altf|=1<<12; 

	int mv = SOM::masterVol; //NEW: act like bgmVol/seVol

	//YUCK: the historical limit is 255
	if(mv) mv = (mv+(mv?mv<0?-1:1:0))/16; 

	if(shift) //Plus/Minus
	{
		if(mv<0) mv = -mv;

		EX::INI::System os;
		int lo = max(0,os->master_volume_mute);
		int hi = min(16,os->master_volume_loud);
		if(lo>hi) std::swap(lo,hi);
		
		//Reminder: this is for when F12 set it to 0/16
		if(mv<lo&&shift>0) mv = lo-shift;
		if(mv>hi&&shift<0) mv = hi-shift;

		mv+=shift; 

		//don't wrap... harsh to go from 0 to full
		if(mv<lo) mv = lo; //hi;
		if(mv>hi) mv = hi; //lo;
	}
	else if(mv<0) //F12
	{
		//REMINDER: This is more like a mute button but I worry that
		//when using master_volume_loud it limits the volume to that
		mv = -mv;
		//mv = 16;
	}
	else //F12
	{
		mv = mv?-mv:16;
		//mv = mv?0:16;
	}

	//YUCK: the historical limit is 255
	SOM::masterVol = mv*16+(mv?mv>0?-1:1:0); 

	if(DSOUND::PrimaryBuffer)
	{
		//turns out the BGM buffer is a slave to this buffer
		//DSOUND::PrimaryBuffer->SetVolume((1-float(SOM::bgmVol)/255)*-10000);
		//DSOUND::PrimaryBuffer->SetVolume(-(16-max(0,SOM::masterVol))*10000/16);
		SOM::config_bels(4);
		DSOUND::PrimaryBuffer->SetVolume(SOM::millibels[2]);
	}
	else assert(0); return true;
}

/*moved to som_game_408400
//player was defeated, restarting
static VOID __cdecl som_state_408400() 
{
	//0 resets counters, leafnums, and 128Bs at 555DC4
	((VOID(__cdecl*)())0x408400)(); 

	//SOM::defeated = true;
	som_game_resetting = true;
}*/
static BYTE __cdecl som_state_00408A80_RIP(DWORD A)
{
	//NEW: trying to change how F3 works so that you can
	//still experience the game as intended but not have
	//to think about resets
	if(SOM::f3)
	{
		SOM::L.pcstatus[SOM::PC::hp] = SOM::L.pcstatus[SOM::PC::_hp]/4+1;
		return 1; //or 0? ignored I think
	}
	return ((BYTE(__cdecl*)(DWORD))0x00408A80)(A); 
}
//opening a map for the first time
static VOID __cdecl som_state_4084cd() 
{
	memset(SOM::L.leafnums,0x00,1024); //repairing 4084cd	
	for(int i=0;i<1024;i++) if(SOM::ezt[i]) 
	SOM::L.leafnums[i] = SOM::L.mapsdata_events[SOM::mpx].c[i];
}

static void som_state_404470_strength_and_magic(int pc, float io[2])
{
	if(!EX::INI::Bugfix()->do_fix_damage_calculus) 
	{
		if(pc==12288) //old, ill conceived additive bonus?
		{				
			io[0]/=5; io[1]/=5;
		}
		else assert(io[0]==0&&io[1]==0); //not doing NPCs

		return; 
	}
	else if(pc==12288)
	{
		//TODO? player_character_weight is probably
		//not destined for greatness. should maybe
		//be using 50 instead ever since 1.2.1.12.
		EX::INI::Player pc;
		
		io[0]/=pc->player_character_weight;

		if(&pc->player_character_weight2)
		{
			io[1]/=pc->player_character_weight2;
		}
		else io[1]/=pc->player_character_weight;

		return;
	}

	if(SOMEX_VNUMBER<0x102010EUL) return; //0,0

	//SAME AS SomEx_pc_or_npc
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
		WORD *p = (WORD*)(&SOM::L.obj_prm_file[prm].c+38);
		sm[0] = p[0]; sm[1] = p[8];
		break;
	}}						 	
	io[0] = sm.ryoku(1)/50.0f; io[1] = sm.ryoku(2)/50.0f;
}
static VOID __cdecl som_state_404470(DWORD _1, DWORD _2, LONG *out, BYTE *out2)
{
	extern int SomEx_pc,SomEx_npc;

	//2020: this is now dynamically allocated memory
	bool enemy_2 = _2-(DWORD)SOM::L.ai<SOM::L.ai_size*sizeof(*SOM::L.ai);

	//this value looks bogus
	//SOM::Struct<43> *player = 
	//(SOM::Struct<43>*)0x19C1A18; //Note this is the inventory

	auto &attack = *(SOM::Attack*)_1;	
	switch(attack.attack_mode) //00404239 has many modes?
	{
	case 1: case 8: case 9: case 14: break; default: assert(0);
	}

	//#ifdef _DEBUG
	DWORD dbg = 0, dbg2 = 0;
	if(!EX::debug) dbg = dbg2 = 0; //damage system
	bool invincible = SOM::frame-SOM::invincible<30;
	if(dbg) ((VOID(__cdecl*)(DWORD,DWORD,LONG*,BYTE*))0x404470)(_1,_2,out,out2);		
	if(dbg) dbg = *out, dbg2 = *out2;
	//#endif
	
	//EX::dbgmsg("%x---%x",_1,_2);

	//What was 8a8? 8ec is hitting NPCs/monsters
	//930 was hitting an NPC
//	assert(_1==0x4c28a8||_1==0x004c28ec||_1==0x004c2930); //2018 
	const DWORD ebx = _1; 
	//const DWORD player_1 = 0x4c28a8; //traps too! enemies are below
	const DWORD player_2 = 0x19c1d14; //NPCs are above player, enemies below	
	
	float y = 1;
	if(_2==player_2) //2024: add jump/fall damage?
	{
		float speeds[3]; EX::speedometer(speeds);
		y+=fabsf(speeds[0]-speeds[1]);
	}
	
	EX::INI::Damage hp; EX::INI::Player pc; 
	EX::INI::Option op; EX::INI::Bugfix bf;

	//TODO: allow player to confirm decision to harm
	if(attack.target_type==4&&*(DWORD*)attack.source!=0x19C1A18)
	{
		//2020: it turns out invincible NPCs still go through
		//normal damage processing but it's triggering assert
		//checks in new code
		if(hp->do_not_harm_defenseless_characters
		||!SOM::L.NPC_prm_file[attack.target->s[SOM::AI::npc]].c[59]) //2020
		{
			*out = 0; *out2 = 0; return; //prevent enemy damage to NPC
		}
	}
	/*
	00404470 83 EC 14         sub         esp,14h 
	00404473 53               push        ebx  
	00404474 8B 5C 24 1C      mov         ebx,dword ptr [esp+1Ch] 
	00404478 81 7B 14 18 1A 9C 01 cmp         dword ptr [ebx+14h],19C1A18h 
	0040447F 56               push        esi  
	00404480 C7 44 24 0C 00 00 00 00 mov         dword ptr [esp+0Ch],0 
	00404488 C7 44 24 10 00 00 00 00 mov         dword ptr [esp+10h],0 
	00404490 C7 44 24 14 00 00 00 00 mov         dword ptr [esp+14h],0 
	00404498 C7 44 24 18 00 00 00 00 mov         dword ptr [esp+18h],0 
	004044A0 C7 44 24 08 00 00 00 00 mov         dword ptr [esp+8],0 
	004044A8 75 2F            jne         004044D9 
	*/	
	int src,dst; //pc/npc
	int trap = 0; //AI::xyz3
	float srcStats[2] = {0,0}; 
	float dstStats[2] = {0,0}; 
	//*(DWORD*)(ebx+0x14)==0x19C1A18
	if(attack.source==(void*)0x19C1A18) 
	{				  
		/*
		004044AA 33 C0            xor         eax,eax 
		004044AC 66 8B 03         mov         ax,word ptr [ebx] 
		004044AF 33 C9            xor         ecx,ecx 
		004044B1 66 8B 4B 02      mov         cx,word ptr [ebx+2] 
		004044B5 89 44 24 20      mov         dword ptr [esp+20h],eax 
		004044B9 DB 44 24 20      fild        dword ptr [esp+20h] 
		004044BD 89 4C 24 20      mov         dword ptr [esp+20h],ecx 
		004044C1 D8 0D BC 82 45 00 fmul        dword ptr ds:[4582BCh] 
		004044C7 D9 5C 24 0C      fstp        dword ptr [esp+0Ch] 
		004044CB DB 44 24 20      fild        dword ptr [esp+20h] 
		004044CF D8 0D BC 82 45 00 fmul        dword ptr ds:[4582BCh] 
		004044D5 D9 5C 24 10      fstp        dword ptr [esp+10h] 
		*/
		srcStats[0] = attack.strength; //*(WORD*)(ebx);
		srcStats[1] = attack.magic; //*(WORD*)(ebx+2);

		src = 12288;
	}
	else //enemy or trap?
	{
		//2020: THIS IS TOO COMPLICATED TO USE ELSEWHERE
		//WORD *src2 = (WORD*)&src;
		//ai3 is after ai in memory (not after extending ai)
		if(trap=(size_t)(attack.source_trap-SOM::L.ai3)<512)
		{
			//src2[1] = attack.source_trap-SOM::L.ai3;
			//src2[0] = 2<<12|attack.source_trap->s[SOM::AI::object];		
			src = 2<<12|attack.source_trap-SOM::L.ai3;
		}
		else
		{
			//src2[1] = attack.source_enemy-SOM::L.ai;
			//src2[0] = attack.source_enemy->s[SOM::AI::enemy];		
			src = attack.source_enemy-SOM::L.ai;
		}
	}		
	/*
	004044D9 8B 73 40         mov         esi,dword ptr [ebx+40h] 
	004044DC 81 FE 18 1A 9C 01 cmp         esi,19C1A18h 
	004044E2 75 3A            jne         0040451E 
	*/
	DWORD esi = *(DWORD*)(ebx+0x40);
	BYTE *shield = 0; //2021
	float shield2, shield3;
	SOM::MDL &mdl = *SOM::L.arm_MDL;
	if(esi==0x19C1A18)
	{
		assert(_2==player_2); //19C1C24 is Strength. 19C1C28 Magic
		/*
		004044E4 8B 15 24 1C 9C 01 mov         edx,dword ptr ds:[19C1C24h] 
		004044EA A1 28 1C 9C 01   mov         eax,dword ptr ds:[019C1C28h] 
		004044EF 81 E2 FF FF 00 00 and         edx,0FFFFh 
		004044F5 89 54 24 20      mov         dword ptr [esp+20h],edx 
		004044F9 DB 44 24 20      fild        dword ptr [esp+20h] 
		004044FD 25 FF FF 00 00   and         eax,0FFFFh 
		00404502 89 44 24 20      mov         dword ptr [esp+20h],eax 
		00404506 D8 0D BC 82 45 00 fmul        dword ptr ds:[4582BCh] 
		0040450C D9 5C 24 14      fstp        dword ptr [esp+14h] 
		00404510 DB 44 24 20      fild        dword ptr [esp+20h] 
		00404514 D8 0D BC 82 45 00 fmul        dword ptr ds:[4582BCh] 
		0040451A D9 5C 24 18      fstp        dword ptr [esp+18h] 
		*/		
		dstStats[0] = SOM::L.pcstatus[SOM::PC::str];
		dstStats[1] = SOM::L.pcstatus[SOM::PC::mag];		 

		dst = 12288;

		if(mdl.ext.d2||SOM::motions.swing_move) //shield?
		{
			int pce = !SOM::motions.swing_move?5:0;

			if(auto*mv=SOM::shield_or_glove(pce))
			{
				int d = pce?mdl.ext.d2:mdl.d;

				int e = SOM::L.pcequip[pce];
				if(e>=250) e = SOM::L.pcequip[3];

				//approximately +/-200 ms allowance?
				//int fps = bf->do_fix_animation_sample_rate?2:1;
				int fps = som_MDL::fps;
				if(6*fps>abs(d-mv->uc[86])||3==SOM::motions.swing_move)
				{
					float *xyz = attack.attack_origin;
					auto cmp = atan2(xyz[2]-SOM::xyz[2],xyz[0]-SOM::xyz[0]);
					cmp = ((FLOAT(__cdecl*)(FLOAT))0x44cc20)(cmp-(SOM::uvw[1]+M_PI_2));

					float arc = mv->s[39]*0.008726645f; //pi/180/2

					shield2 = 1;
					shield2+=0.5f*SOM::motions.shield;
					arc*=shield2;
					//this is for bad status negation
					shield3 = shield2*mv->s[39]/90;
					/*assume counter keeps monster
					//at bay (assuming not SFX)
					//assume weapon is less effective
					if(!pce) shield3*=0.5f;*/

					if(cmp>-arc&&cmp<arc) //attack.pie?
					{
						if(pce==0&&!trap&&!attack._source_SFX)
						{
							//TODO? set when enemy is nearby?
							SOM::counteratk = SOM::frame;
						}

						//SOM_PRM_extend_items
						shield = SOM::L.item_prm_file[e].uc+328;

						if(pce==5)
						{
							//NOTE: does se3D use this?
							SomEx_pc = dst; SomEx_npc = src;

							int snd, pitch = 0, muffle = 0;
							EX::INI::Sample se; if(&se->ricochet_identifier)
							{
								float f = se->ricochet_identifier; 
								snd = EX::isNaN(f)?0:(int)f&0xfff;
							}
							else //just an educated guess?
							{								
								//original SOM sounds used as shields are 0055
								//by Lizardman (metal shield) and 0402 used by
								//Death Figher (clinky sword) ... objects have
								//518 (door slam?) and 528 (punching bag) 

								//1 and 96 are interesting sounds
								//427 is another sword like sound
								//450 maybe if higher pitch? 452?

								//SUBJECT TO CHANGE
								//authors shouldn't rely on this
								float metal = (shield[0]+shield[2])/1.5f/shield[1];								
								if(metal>=1)
								{
									//Lizardman uses -7 (Skeleton -6)
									snd = 55; pitch = -16; 
								}
								else
								{
									//would rather stay under 500
									//snd = 528;
									snd = 96; pitch = -10; 
								}

								//randomizing
								1&SOM::frame?pitch--:muffle-=150;
							}
							if(snd)
							{
								//NOTE: footstep_identifier used 30/se3D too
								//(there's currently a bug where sounds can't
								//be both 2d and 3d)
								//NOTE: SOM::xyz should now play at mid height
								//SOM::se3D(SOM::xyz,snd,pitch,volume);
								SOM::se3D(SOM::xyz,snd,pitch,muffle);
							}
						}
					}
				}
			}
		}
	}
	else //NPC or enemy?
	{
		//2020: THIS IS TOO COMPLICATED TO USE ELSEWHERE
		//WORD *dst2 = (WORD*)&dst;		
		if(enemy_2) //_2<player_2
		{
			//dst2[1] = attack.target_enemy-SOM::L.ai;
			//dst2[0] = attack.target_enemy->s[SOM::AI::enemy];
			dst = attack.target_enemy-SOM::L.ai;
		}
		else
		{
			//dst2[1] = attack.target-SOM::L.ai2;
			//dst2[0] = 1<<12|attack.target->s[SOM::AI::npc];
			dst = 1<<12|attack.target-SOM::L.ai2;
		}  		
	}

	//defending? (0xc)
	/*
	0040451E 83 7B 3C 02      cmp         dword ptr [ebx+3Ch],2 
	00404522 55               push        ebp  
	00404523 57               push        edi  
	00404524 75 6A            jne         00404590 
	*/	  	
	//float evasion = 0; //2020: or defense? 
	float defense = 0;
	if(*(DWORD*)(ebx+0x3C)==2) //enemy?
	{
		/*
		00404526 33 C0            xor         eax,eax 
		00404528 66 8B 46 20      mov         ax,word ptr [esi+20h] 
		0040452C 8D 0C 40         lea         ecx,[eax+eax*2] 
		0040452F 8D 0C 89         lea         ecx,[ecx+ecx*4] 
		00404532 8D 14 88         lea         edx,[eax+ecx*4] 
		00404535 8B 4E 68         mov         ecx,dword ptr [esi+68h] 
		00404538 33 C0            xor         eax,eax 
		0040453A 66 8B 04 D5 C8 A1 4D 00 mov         ax,word ptr [edx*8+4DA1C8h] 
		00404542 8D 3C D5 C8 A1 4D 00 lea         edi,[edx*8+4DA1C8h] 
		00404549 51               push        ecx  
		0040454A 8B 2C 85 C8 67 4C 00 mov         ebp,dword ptr [eax*4+4C67C8h] 
		00404551 E8 3A CF 03 00   call        00441490 
		00404556 8B 8E 40 02 00 00 mov         ecx,dword ptr [esi+240h] 
		0040455C 83 C4 04         add         esp,4 
		0040455F 83 F9 0C         cmp         ecx,0Ch 
		00404562 75 2C            jne         00404590 
		*/
		DWORD eax = *(WORD*)(esi+0x20), edx = eax+(eax*3*5*4);
		DWORD ecx = *(DWORD*)(esi+0x68), edi = edx*8+0x4DA1C8;

		eax = *(WORD*)(edi);

		DWORD ebp = *(DWORD*)(eax*4+0x4C67C8); 
		
		//DWORD before = *(DWORD*)(esi+0x240); //ai_state

		//2020: getting current animation frame
		eax = ((DWORD(__cdecl*)(DWORD))0x441490)(ecx);

		DWORD after = *(DWORD*)(esi+0x240); //ai_state

		//assert(before==after);

		//2020: I think c is Defend and not Evade
		//
		if(after==0xC) //cmp ecx,0Ch
		{
			/*
			00404564 33 D2            xor         edx,edx 
			00404566 8A 95 89 00 00 00 mov         dl,byte ptr [ebp+89h] 
			0040456C 3B D0            cmp         edx,eax 
			0040456E 7F 20            jg          00404590 
			00404570 33 C9            xor         ecx,ecx 
			00404572 8A 8D 8A 00 00 00 mov         cl,byte ptr [ebp+8Ah] 
			00404578 3B C1            cmp         eax,ecx 
			0040457A 7F 14            jg          00404590 
			0040457C 33 D2            xor         edx,edx 
			0040457E 8A 97 46 01 00 00 mov         dl,byte ptr [edi+146h] 
			00404584 89 54 24 28      mov         dword ptr [esp+28h],edx 
			00404588 DB 44 24 28      fild        dword ptr [esp+28h] 
			0040458C D9 5C 24 10      fstp        dword ptr [esp+10h] 
			*/	   
			edx = *(BYTE*)(ebp+0x89);

			if(edx<=eax) //cmp edx,eax 
			{
				ecx = *(BYTE*)(ebp+0x8A);

				if(eax<=ecx) //cmp eax,ecx
				{
					edx = *(BYTE*)(edi+0x146);

					defense = edx; 			
				}
			}
		}
	}
	//2018: Defer to hit_point_quantifier?
	auto &hpq2 = &hp->hit_point_quantifier2
	?hp->hit_point_quantifier2:hp->hit_point_quantifier,
	&hoq = &hp->hit_offset_quantifier
	?hp->hit_offset_quantifier:hp->hit_point_quantifier,
	&hoq2 = &hp->hit_offset_quantifier2
	?hp->hit_offset_quantifier2
	:&hp->hit_offset_quantifier?hoq:hpq2; //tricky		
	DWORD mode2 = hp->hit_point_mode==2?~attack.attack_mode&8:3;
	//SWINGING BALL TRAPS USE 9?!	
	if(trap&&mode2==8) mode2 = 0;
	if(!&hpq2) som_state_404470_strength_and_magic(src,srcStats);
	if(!&hpq2) som_state_404470_strength_and_magic(dst,dstStats);
	/*
	00404590 8B 44 24 30      mov         eax,dword ptr [esp+30h] 
	00404594 8B 6C 24 2C      mov         ebp,dword ptr [esp+2Ch] 
	00404598 C7 00 00 00 00 00 mov         dword ptr [eax],0 
	*/
	DWORD ebp = _2; *out = 0;	
	for(esi=0;esi<8;esi++,ebp+=2)
	{
		/*
		0040459E 33 F6            xor         esi,esi 
		004045A0 33 C0            xor         eax,eax 
		004045A2 8A 44 33 04      mov         al,byte ptr [ebx+esi+4] 
		004045A6 84 C0            test        al,al 
		004045A8 74 77            je          00404621 
		004045AA 0F BF 4D 00      movsx       ecx,word ptr [ebp] 
		004045AE 25 FF 00 00 00   and         eax,0FFh 
		004045B3 83 FE 03         cmp         esi,3 
		004045B6 89 44 24 28      mov         dword ptr [esp+28h],eax 
		004045BA DB 44 24 28      fild        dword ptr [esp+28h] 
		004045BE 89 4C 24 28      mov         dword ptr [esp+28h],ecx 
		004045C2 DB 44 24 28      fild        dword ptr [esp+28h] 
		004045C6 D8 44 24 10      fadd        dword ptr [esp+10h] 
		004045CA D9 5C 24 28      fstp        dword ptr [esp+28h] 
		*/
		float offset = *(SHORT*)(ebp);		
		float rating = *(BYTE*)(ebx+esi+4);

		if(shield) offset+=shield[esi];
		rating*=y; //2024
		
		//if(!rating) continue; //todo: extension to bypass this?
		/*		
		004045CE 7D 0E            jge         004045DE 
		004045D0 D8 44 24 14      fadd        dword ptr [esp+14h] 
		004045D4 D9 44 24 28      fld         dword ptr [esp+28h] 
		004045D8 D8 44 24 1C      fadd        dword ptr [esp+1Ch] 
		004045DC EB 0C            jmp         004045EA 
		004045DE D8 44 24 18      fadd        dword ptr [esp+18h] 
		004045E2 D9 44 24 28      fld         dword ptr [esp+28h] 
		004045E6 D8 44 24 20      fadd        dword ptr [esp+20h] 
		*/		

		bool magic = esi>=mode2;

		if(&hpq2)
		{
			SomEx_pc = src; SomEx_npc = dst;

			//indirect attack or magical affinity?			
			rating = (magic?hpq2:hp->hit_point_quantifier)(rating,0,esi);
			offset = (magic?hoq2:hoq)(offset,defense,esi);
		}
		else if(!bf->do_fix_damage_calculus) //emulate Som2k?
		{
			if(!rating) continue;
			rating+=srcStats[magic]; //2023?
			offset+=dstStats[magic]+defense;  
		}
		else //built-in formula
		{	
			offset+=defense;
			//1.2.1.14 changes this.
			if(SOMEX_VNUMBER>=0x102010EUL)
			{
				rating*=srcStats[magic]; 
				offset*=dstStats[magic]; 							
			}
			else
			{
				rating+=rating*srcStats[magic]; 
				offset+=offset*dstStats[magic]; 			
			}			
						
			//if(_1==player_1) //traps use it
			if(attack.source==(void*)0x19C1A18)
			rating*=pc->player_character_scale;
			else rating*=attack.source->f[SOM::AI::scale+trap];
			if(_2==player_2)
			offset*=pc->player_character_scale;
			else offset*=attack.target->f[SOM::AI::scale];
		}
		/*
		004045EA D9 C1            fld         st(1) 
		004045EC D8 D9            fcomp       st(1) 
		004045EE DF E0            fnstsw      ax   
		004045F0 F6 C4 41         test        ah,41h 
		004045F3 75 0D            jne         00404602 
		004045F5 D9 C1            fld         st(1) 
		004045F7 D8 E1            fsub        st,st(1) 		
		004045F9 E8 3A B3 04 00   call        0044F938 
		004045FE 8B F8            mov         edi,eax 
		00404600 EB 02            jmp         00404604 
		00404602 33 FF            xor         edi,edi 
		*/						   
		LONG edi = 0; //max(0,rating-offset); 
		/*								  
		00404604 D9 C1            fld         st(1) 
		00404606 D8 CA            fmul        st,st(2) 
		00404608 D9 C9            fxch        st(1) 
		0040460A DC C0            fadd        st(0),st 
		0040460C DE F9            fdivp       st(1),st 
		//not sure what this does but INF becomes 0//
		0040460E E8 25 B3 04 00   call        0044F938 
		00404613 DD D8            fstp        st(0) 
		00404615 03 F8            add         edi,eax
		*/
		if(&hp->hit_outcome_quantifier)
		{
			//SomEx_pc = SomEx_npc = -1; //NaN? 
			SomEx_pc = dst; SomEx_npc = src; //1.2.1.12

			//until 1.2.1.12 there was a showstopping bug here
			//after 1.2.1.12 negative or positive it is the same
			//(hit-points are subtracted from total HP after all)
			//(as a result, the formulas must clamp their result)
			//*out = hp->hit_outcome_quantifier(rating,offset,esi);
			rating = fabs(hp->hit_outcome_quantifier(rating,offset,esi));						
			edi = min(rating,65535);
		}
		else if(bf->do_fix_damage_calculus) 
		{	
			if(offset>0&&rating>0) //debugging
			{
				if(1||!EX::debug)
				{
					//this is based on the original formula... only
					//problem is the window seems too small and it's
					//not obvious how to increase it

					rating-=max(0,offset-(rating*rating/(offset*2)));
				}
				else //2021: trying to generalize?
				{	
					//IT GETS VERY SIMILAR RESULTS

					//this has more or less double the window of the
					//older (unused) formula while being smooth but not
					//necessarily the same function

					//float r = (rating-offset*0.5f)/offset; //half window
					float r = rating/(offset*2); //full?

					r = max(0,min(1,r));

					if(1) //nonlinear?
					{
						r = (cosf(r*M_PI)+1)*0.5f;
					}
					else //maybe nonlinear? 
					{
						//if offset doesn't equal rating then it
						//seems like this progressively chops more
						//off rating even though it looks linear

						r = 1-r; 
					}

					//NOTE: this will always be more severe than a
					//cosine for ratings less than the offset since
					//the difference between them will be subtracted
					rating-=offset*r;
				}
			}
			rating+=0.5f; //2021
			edi = min((int)rating,65535);
			if(edi<0) continue;			
		}
		else //classic formula with bugs
		{
			//this is max(0,P-Q)+P*P/(Q*2) without
			//acknowledging the divide by 0
			//like so: max(0,P-Q)+inf(P*P/(Q*2),0)			

			edi = max(0,rating-offset); 

			rating*=rating; offset+=offset;

			if(offset) edi+=rating/offset; 
			/*
			00404617 85 FF            test        edi,edi 
			00404619 7E 06            jle         00404621 
			*/
			if(edi<0) continue;
		}
		/*		
		0040461B 8B 44 24 30      mov         eax,dword ptr [esp+30h] 
		0040461F 01 38            add         dword ptr [eax],edi 
		00404621 46               inc         esi 
		00404622 83 C5 02         add         ebp,2 
		00404625 83 FE 08         cmp         esi,8 
		00404628 0F 8C 72 FF FF FF jl          004045A0 
		*/
		*out+=edi; 
	}
	/*
	0040462E 8B 43 14         mov         eax,dword ptr [ebx+14h] 
	00404631 BF 18 1A 9C 01   mov         edi,19C1A18h 
	00404636 3B C7            cmp         eax,edi 
	00404638 75 1B            jne         00404655 
	*/
	DWORD edi = 0x19C1A18;
	if(*(DWORD*)(ebx+0x14)==edi) //0x19C1A18
	{
		//player power gauge
		/*
		0040463A 8B 4C 24 30      mov         ecx,dword ptr [esp+30h] 
		0040463E 33 D2            xor         edx,edx 
		00404640 66 8B 53 0E      mov         dx,word ptr [ebx+0Eh] 
		00404644 B8 59 17 B7 D1   mov         eax,0D1B71759h 
		00404649 0F AF 11         imul        edx,dword ptr [ecx] 
		0040464C F7 E2            mul         eax,edx 
		0040464E C1 EA 0C         shr         edx,0Ch 
		00404651 89 11            mov         dword ptr [ecx],edx 
		00404653 EB 04            jmp         00404659 
		*/

		LONG edx = *out**(WORD*)(ebx+0xE); 

		edx = (0xD1B71759ULL*edx)>>32; //mul edx 
		
		*out = edx>>0xC;
	}
	/*
	00404655 8B 4C 24 30      mov         ecx,dword ptr [esp+30h] 
	00404659 83 39 00         cmp         dword ptr [ecx],0 
	0040465C 75 06            jne         00404664 
	0040465E C7 01 01 00 00 00 mov         dword ptr [ecx],1 
	*/
	if(&hp->hit_handicap_quantifier)
	{
		SomEx_pc = dst; SomEx_npc = src;

		*out = hp->hit_handicap_quantifier(*out);
	}
	else if(*out==0) *out = 1;	
	  
	SomEx_pc = 12288; SomEx_npc = -1;	
	/*
	00404664 8B 74 24 34      mov         esi,dword ptr [esp+34h] 
	00404668 C6 06 00         mov         byte ptr [esi],0 
	0040466B 39 7B 14         cmp         dword ptr [ebx+14h],edi 
	0040466E 5F               pop         edi  
	0040466F 5D               pop         ebp  
	00404670 0F 84 68 01 00 00 je          004047DE 	
	*/
	*out2 = 0; if(!*out) return;

	//NOTE: this says if player
	//is the attacker then skip
	//the attack's side effects 
	//but that is incorrect. it
	//should check the defender
	//if(*(DWORD*)(ebx+0x14)==edi) //0x19C1A18
	if(*(DWORD*)(ebx+0x40)!=edi) //FIX: defender/target
	{
		if(dbg&&!invincible) EX::dbgmsg("%d (%d)",dbg,*out);		

		//2017: King's Field 3 style nonstunned hit?
		//NEW: feed SOM::Versus.hit()
		//TODO: detect invincible NPCs that have HP
		//if(op->do_red&&op->do_hit)
		bool hit = op->do_red&&op->do_hit?false:true;
		{
			float red; //2020
			WORD *hp,_hp;
			SOM::MDL *mdl = nullptr;
			if(enemy_2) //_2<player_2
			{
				//int enemy = (_2-0x4C77C8)/(4*149);
				int enemy = (_2-(DWORD)SOM::L.ai)/(4*149);
				auto &ai = SOM::L.ai[enemy];
				hp = (WORD*)&ai[SOM::AI::hp];
				mdl = (SOM::MDL*)ai[SOM::AI::mdl];
				WORD prm = SOM::L.ai[enemy][SOM::AI::enemy];
				_hp = SOM::L.enemy_prm_file[prm].s[148];
				//red = SOM::Red(*out,_hp);
				red = SOM::Red(*out,src,dst,_hp);
				if(!SOM::Versus.hit(enemy,*out,_hp,red)&&!hit)
				{
					//2020: NORMALLY som_logic_406ab0 WOULD DO
					//THIS. MAYBE IT SHOULD BE DOING THIS ALSO
					SOM::L.ai[enemy][SOM::AI::ai_bits]|=4;

					*out = 0; return; //flashing red, but not stunned?
				}
			}
			else 
			{
				int npc = (_2-0X1A12DF0)/(4*43);
				hp = (WORD*)&SOM::L.ai2[npc][SOM::AI::hp2];
				auto &ai = SOM::L.ai2[npc];
				WORD prm = ai[SOM::AI::npc];
				mdl = (SOM::MDL*)ai[SOM::AI::mdl2];
				_hp = SOM::L.NPC_prm_file[prm].s[1];
				//red = SOM::Red(*out,_hp);
				red = SOM::Red(*out,src,dst,_hp);
				if(!SOM::Versus.hit2(npc,*out,_hp,red)&&!hit)
				{
					*out = 0; return; //flashing red, but not stunned?
				}
			}

			if(*out>=*hp) //2023
			{
				//kf2: fading to white! //white ghost? //EXTENSION?
				if(auto*d=(*mdl)->ext.mdo)				
				for(int i=d->material_count;i-->0;)
				{
					float *m7 = SOM::L.materials[mdl->ext.mdo_materials[i]].f+1;
					//nonzero: som_MDL_440ab0_unanimated manages this
					//emissive ambient
					m7[12+3] = 0.0000001f; 
				}
								
			}
			else if(!hit) if(!SOM::Stun(red))
			{					
				*hp-=*out; *out = 0;
			}
		}

		return; //todo: bad status for monsters???
	}
	/*
	00404676 8A 4B 0C         mov         cl,byte ptr [ebx+0Ch] 
	00404679 84 C9            test        cl,cl 
	0040467B 0F 84 5D 01 00 00 je          004047DE 
	00404681 8A 43 0D         mov         al,byte ptr [ebx+0Dh] 
	00404684 84 C0            test        al,al 
	00404686 0F 86 52 01 00 00 jbe         004047DE 
	*/
	//SOM_PRM allows one bit but if
	//a PRF is edited it would work
	int bits = attack.side_effects; //*(BYTE*)(ebx+0x0C);
	int bite = attack.side_effects_potency; //*(BYTE*)(ebx+0x0D);

	if(bits&&bite) //why jbe and not je???
	for(int i=0;i<5;i++) if(bits&1<<i)
	{
		//this loop is unrolled. omitting ASM of 1~4
		/*		
		0040468C F6 C1 01         test        cl,1 
		0040468F 74 3D            je          004046CE 
		00404691 8A C8            mov         cl,al 
		00404693 A0 1F 1C 9C 01   mov         al,byte ptr ds:[019C1C1Fh] 
		00404698 84 C0            test        al,al 
		*/
		int anti = *(BYTE*)(0x19C1C1F+i); //resistances

		if(anti||shield)
		{	
			/*
			0040469A 88 4C 24 28      mov         byte ptr [esp+28h],cl 
			0040469E 74 11            je          004046B1 
			004046A0 3A C8            cmp         cl,al 
			004046A2 76 08            jbe         004046AC 
			004046A4 2A C8            sub         cl,al 
			004046A6 88 4C 24 28      mov         byte ptr [esp+28h],cl 
			004046AA EB 05            jmp         004046B1 
			004046AC C6 44 24 28 00   mov         byte ptr [esp+28h],0 
			*/
			if(anti>=bite) continue;
			/*
			004046B1 E8 5D B0 04 00   call        0044F713 //SOM::rng 
			004046B6 99               cdq              
			004046B7 B9 64 00 00 00   mov         ecx,64h 
			004046BC F7 F9            idiv        eax,ecx 
			004046BE 8B 44 24 28      mov         eax,dword ptr [esp+28h] 
			004046C2 25 FF 00 00 00   and         eax,0FFh 
			004046C7 3B D0            cmp         edx,eax 
			004046C9 7D 03            jge         004046CE 
			*/

			//randomizer: idiv stores remainder in EDX
			int edx = SOM::rng()%100;

		
			//HACK: just spitballing
			//NOTE: I feel like however this works it should be
			//as-if the poison, etc. is unabled to make contact
		if(shield) anti+=(int)((33+edx/3)*shield3); 


			if(edx>=bite-anti) continue;
		}
		//004046CB 80 0E 01         or          byte ptr [esi],1 
		*out2|=1<<i;
	}

	//2: won't match due to its being random
	if(dbg&&!invincible) EX::dbgmsg("%d (%d) --- %d (%d)",dbg,*out,dbg2,*out2);		
		
	if(_2!=player_2){ assert(0); return; } //paranoia
	
	if(op->do_hit)
	if(SOM::invincible&&!op->do_hit2) //invincibility?
	{
		 *out = *out2 = 0; //prevent damage/effect
	}
	else
	{
		//static int testing = 0;
		//EX::dbgmsg("hit: %d",testing++);
		//EX::dbgmsg("hit: %x",attack.source);

		//bool indirect = attack.attack_mode==9;
		//float *player = SOM::L.pcstate?SOM::L.pcstate:SOM::xyz; //???
		float *player = SOM::L.pcstate;
		//2020: can get the CP I think?
		//float *origin = &attack.source->f[SOM::AI::xyz-trap];
		float *origin = attack.attack_origin;
		origin[1] = player[1]; //2020

		//REMINDER: som_logic_reprogram IS COUNTING ON THIS 
		//TO FINALLY ENABLE SPEAR TRAPS
		float hit[3];
		Somvector::map(hit).copy<3>(player).remove<3>(origin).unit<3>();

		//NOTE: Red is processing critical_hit_point_quantifier
		//and returns 1 for a critical hit even if do_red is no
		SOM::red2 = SOM::Red(*out,src);
		bool crit = SOM::red2==1;

		//2020: factor stun into damage response?
		bool stun2 = SOM::Stun(SOM::red2);
		bool stun = trap||!op->do_red||stun2;

		if(!trap
		//2020: if the trap sweeps through it tends to cancel out
		||SOM::hit!=attack.source
		//this is to include all CPs in a multi CP trap knockback
		||SOM::frame-SOM::invincible<2)
		{
			//2021: critical hit?
			float knockback = crit?4:trap||stun&&stun2?2:1;
			if(shield)
			{
				//REMINDER: what about side_effects_potency?

				//NOTE: holding the shield close here causes
				//more of the impact to transfer
				SOM::motions.shield3 = 0.5f*shield2;

				knockback*=SOM::motions.shield3;
			}				   
			auto &p = EX::Affects[0].position;		
			float prev = Somvector::map(p).length<3>();
			p[0]+=hit[0]*knockback;
			p[2]+=hit[2]*knockback;
			Somvector::map(p).unit<3>(); //renormalize
			knockback+=prev; 
			p[0]*=knockback; 
			p[2]*=knockback;
		}

		if(!SOM::invincible) 	
		{
			//NOTE: do_red is to complement enemy stun behavior when
			//using do_red and do_hit 	
			if(stun)
			{	
				SOM::hit = /*SOM::invincible =*/ attack.source; //true;
			}
			//2017: poison, events, etc.
			SOM::invincible = SOM::frame; //true
		}
	}
}

extern bool som_state_42bca0_door(float(&neg)[3], int obj)
{
	float sep[3]; auto &ai = SOM::L.ai3[obj];

	/*42BD16 8B 02            mov         eax,dword ptr [edx] 
	0042BD18 89 44 24 14      mov         dword ptr [esp+14h],eax 
	0042BD1C 8B 42 04         mov         eax,dword ptr [edx+4] 
	0042BD1F 8B 52 08         mov         edx,dword ptr [edx+8] 
	0042BD22 89 44 24 18      mov         dword ptr [esp+18h],eax 
	0042BD26 33 C0            xor         eax,eax 
	0042BD28 89 54 24 1C      mov         dword ptr [esp+1Ch],edx 
	//0042BD2C 66 8B 47 52      mov         ax,word ptr [edi+52h]
	*/
									  
	//float *xyz = (FLOAT*)(esi-0x1C);
	//WARNING: Looks like this is yxz?
	//float *box = (FLOAT*)(esi+0x08);
	float *xyz = &ai[SOM::AI::xyz3], *box = &ai[SOM::AI::box3];

	typedef float door_mem[40]; //control points?
	//door_mem &debug = *(door_mem*)(esi-64);
	door_mem &debug = *(door_mem*)(ai.c+32);
	
	EX::INI::Player pc;

	//hack: using this just for doors just for now
	//float sep[3], neg[3] = { -SOM::xyz[0],0,-SOM::xyz[2] };

	float radius = pc->player_character_radius, radius_2 = radius*2;
		
	//hack: using this just for doors just for now
	//Somvector::map(sep).copy<3>(xyz).move<3>(neg);
	Somvector::series(sep,xyz[0],0,xyz[2]).move<3>(neg);
	if(fabsf(sep[0])>radius_2||fabsf(sep[2])>radius_2)
	{
		return false; //continue;
	}

	//SOM would do atan2(sep[0],sep[1])-SOM::uvw[1]
	//(the angle gets wrapped around by call 44cc20)
	//458238 is 0.0, so this just tosses if behind us
	//A dot product seems like a better way to go here
	/*00427E15 D8 25 A0 1D 9C 01 fsub        dword ptr ds:[19C1DA0h] 
	00427E1B D9 44 24 08      fld         dword ptr [esp+8] 
	00427E1F D8 25 98 1D 9C 01 fsub        dword ptr ds:[19C1D98h] 
	00427E25 D9 F3            fpatan           
	00427E27 D8 25 A8 1D 9C 01 fsub        dword ptr ds:[19C1DA8h] 
	00427E2D D9 1C 24         fstp        dword ptr [esp] 
	00427E30 E8 EB 4D 02 00   call        0044CC20 
	00427E35 D8 1D 38 82 45 00 fcomp       dword ptr ds:[458238h]
	*/
//	if(1) //2017: TRYING SOMETHING DIFFERENT
//	{
		//doors are weird. to ensure that the same-side test is on the right
		//side of the door, use the door clipper code to figure out its clip
		//shape

		//How to get length of arms? Not sure.
		float reach = pc->player_character_radius2;
		//int obj = tapped?(SOM::eventapped-x1A44460)/0xB8:i;
		float on[3];
		float pos[3]; memcpy(pos,SOM::L.pcstate,3*sizeof(float));
		pos[1]-=reach;
		if(!((BYTE(*)(FLOAT*,FLOAT,FLOAT,DWORD,DWORD,FLOAT*,FLOAT*))0x40dff0)
		(pos,SOM::L.height+reach*2,radius_2,obj,0,sep,on))
		return false; //continue;
		if(Somvector::map(on).dot<3>(Somvector::map(SOM::pov))>0)
		return false; //continue;

		//FIX ME
		//YIKES! separtion below includes elevation??? (2020) 
		sep[1] = 0; assert(!neg[1]);

		Somvector::map<3>(sep).move<3>(neg); 
			
		//goto same_side;
		//Can use arms length from any point on the door.
		float separation = Somvector::map(sep).length<3>();
		
		int pr2 = SOM::L.obj_prm_file[ai[SOM::AI::object]].us[18];
		int clip = SOM::L.obj_pr2_file[pr2].c[80];
		int type = SOM::L.obj_pr2_file[pr2].c[82];

		//DUPLICATE? som_logic_42bb80
		//
		// FIX ME: the doors have very different bbox data
		// the PRF files need editing
		//
		float round = 0.5f+min(box[/*0*/1],box[clip?2:/*0*/1]); 

		//2020: cut double door distance down to a single door 
		if(!clip&&(type&1)==0) round*=0.5f;

		if(separation>reach+round) return false; //continue;

	/*REFERENCE?
	}
	else if(Somvector::map(sep).dot<3>(Somvector::map(SOM::pov))>0) //same_side:
	{	
		float separation = Somvector::map(sep).length<3>();

		float round = 0.5f*sqrtf(box[1]*box[1]+box[2]*box[2]);
			
		//hack: using this just for doors just for now
		if(separation>round+round) continue; 
		//if(separation>radius+pc->player_character_radius2) continue; 
	}
	else continue; //behind the PC (the center that is)*/

	return true;
}
static DWORD __cdecl som_state_42bca0() //42AE60
{	
	//subroutine: returns object to open etc or 0xFFFFFFFF
		
	const bool db = 1; //SOM::image()=='db2'; //hack...

	//REMOVE ME
	DWORD x042BCA0 = db?0x042BCA0:0x042AE60;
	DWORD x1A44460 = db?0x1A44460:0x1A43270;
	DWORD x1A36424 = db?0x1A36424:0x1A35234;
	DWORD x1A1B400 = db?0x1A1B400:0x1A1A210;

		//2021: Note, this looks like a bad
		//algorithm because it doesn't take
		//into account box shaped objects
	if(0) return ((DWORD(__cdecl*)())x042BCA0)(); //testing

	EX::INI::Player pc;

	//hack: using this just for doors just for now
	float neg[] = { -SOM::xyz[0],0,-SOM::xyz[2] };

	//NEW: limited tap/hold determination
	DWORD esi = SOM::eventick?SOM::eventapped:0; 
		
	bool tapped = SOM::eventapped!=0;
	int sz = SOM::L.ai3_size; //2020
	int i = esi?sz-1:0; //511
	if(!esi) esi = x1A44460; //som_db.exe
	for(;i<sz;i++,esi+=0xB8) //184
	{
		//present and not hidden?
		if(!*(BYTE*)(esi+0x19)||!*(BYTE*)(esi+0x1A)) continue;

		/*42BCEF 66 8B 46 A0      mov         ax,word ptr [esi-60h] 
		0042BCF3 8D 14 C5 00 00 00 00 lea         edx,[eax*8] 
		0042BCFA 2B D0            sub         edx,eax 
		0042BCFC 33 C0            xor         eax,eax 
		0042BCFE 66 8B 04 D5 24 64 A3 01 mov         ax,word ptr [edx*8+1A36424h] 
		0042BD06 8D 56 E4         lea         edx,[esi-1Ch] 
		0042BD09 8D 04 40         lea         eax,[eax+eax*2] 
		0042BD0C 8D 04 C0         lea         eax,[eax+eax*8] 
		0042BD0F 8D 3C 85 00 B4 A1 01 lea         edi,[eax*4+1A1B400h]
		*/

		auto &ai = *(SOM::Struct<46>*)(esi-0x60); //2022
		//DWORD prm = *(WORD*)(esi-0x60); assert(prm<1024);
		DWORD prm = ai.us[0]; if(prm>=1024) continue;
		//DWORD pr2 = *(WORD*)(prm*56+x1A36424); //som_db.exe
		DWORD pr2 = SOM::L.obj_prm_file[prm].us[18]; if(prm>=1024) continue;
		////pr2+=pr2*2; pr2+=pr2*8; pr2 = pr2*4+x1A1B400; //som_db.exe
		//pr2 = pr2*108+x1A1B400; //3*9*4
		
		//0042BD2C 66 8B 47 52      mov         ax,word ptr [edi+52h] 

		//WORD *type = (WORD*)(pr2+0x50); if(!type[1]) continue; //interactive?
		WORD *type = SOM::L.obj_pr2_file[pr2].us+40; if(!type[1]) continue; 

		BYTE clip = type[0]&0xFF; //2020
		bool door = type[1]>=0xB&&type[1]<=0xE;
		assert(type[1]!=0xC); //ever seen one or these?

		if(door) //pipe through som_state_408CC0
		{
			int obj = tapped?(SOM::eventapped-x1A44460)/0xB8:i;

			//2022: moved into subroutine so som.logic.cpp
			//can use the same (exact) math for activation
			//events 
			if(!som_state_42bca0_door(neg,obj)) continue;
		}
		else
		{
			static BYTE event[8]; event[1] = 2; //object

			//2021: note, this isn't the direction the PC
			//is facing, it's the direction the object is
			//facing
			*(WORD*)(event+6) = 360; //activation cone 	
			*(WORD*)(event+2) = i; //object number
			if(tapped) //i won't do
			*(WORD*)(event+2) = (SOM::eventapped-x1A44460)/0xB8;

			extern BYTE som_logic_408cc0(som_EVT*); //2021
			//if(som_state_408CC0((DWORD)event-0x1e)) 
			//if(((BYTE(__cdecl*)(DWORD))0x408CC0)((DWORD)event-0x1e)) 
			if(som_logic_408cc0((som_EVT*)((DWORD)event-0x1e)))
			{
				SOM::eventype = type[1]; //2020

				if(tapped)
				{
					SOM::eventapped = 0; 
					SOM::eventype = 0; //2020
					
					return *(WORD*)(event+2); //i
				}
				else if(!SOM::emu&&!EX::context()) 
				{
					//42bca0 detects the closest object, and perhaps inert objects
					//should be given lower priority (this isn't event activation)
					int todolist[SOMEX_VNUMBER<=0x1020602UL];

					switch(type[1])
					{
					case 0x15: //NEW: animated container

						//TODO: SHOULD PROBABLY DO THIS FOR ALL INTERACTIVE OBJECTS

						//2020
					case 0x1E: //spear trap?
					case 0x28: //switch
					case 0x29: //receptacle

						//2021: everything goes through here now to handle box clip
						//shapes correctly
					case 0x16: //corpse
					case 0x14: //unanimated container
					
						if(pc->do_not_dash) return 1; //quick fix

						SOM::eventapped = esi; //wait for tap/hold determination

						//SOM::eventype = type[1]; //2020

						return 0xFFFFFFFF;
					}				
				}
				return i;
			}		  			
			continue; 
		}

		if(tapped)
		{
			DWORD hack = (SOM::eventapped-x1A44460)/0xB8;

			SOM::eventapped = 0;
			SOM::eventype = 0;
		
			return hack; //i
		}
		else if(!SOM::emu&&!EX::context())
		{
			if(!pc->do_not_dash) //quick fix
			{
				SOM::eventapped = esi;
				SOM::eventype = type[1];

				return 0xFFFFFFFF; 
			}
		}

		return i; //testing
	}
	SOM::eventapped = 0;
	SOM::eventype = 0; //2020

	return 0xFFFFFFFF;
}

extern BYTE *som_state_43FE70_lore = 0;
static void __cdecl som_state_43FE70(BYTE *name, char *lore) 
{
	SOM::frame_is_missing();	
	som_state_43FE70_lore = (BYTE*)lore;
	int c = lore>SOM::L.NPC_prm_file->c?'PRM5':'PRM4';
	((void(__cdecl*)(void*,const char*))0x43FE70)(name,SOM::title(c,lore));			
	som_state_43FE70_lore = 0;
}
static void __cdecl som_state_422BD0(DWORD a, DWORD b, char *lore, DWORD d, DWORD e)
{
	((void(__cdecl*)(DWORD,DWORD,const char*,DWORD,DWORD))0x422BD0)
	(a,b,SOM::title(lore>=SOM::L.magic_prm_file->c?'PRM2':'PRM0',lore),d,e);			
}
static void __cdecl som_state_slides422AB0(HDC dc, char *line, int x, int y)
{	
	if(0) return ((void(__cdecl*)(HDC,char*,int,int))0x422AB0)(dc,line,x,y);

	static int i = 0;
	static std::string lines; 
	if(!i) lines.reserve(16*48+16);	
	if(line[0]!=' '||line[1]) lines+=line;
	if(++i!=16) lines+='\n'; //expecting 16 lines
	else for(SetTextAlign(dc,0);i;i=0,lines.clear())
	{
		//2022: adjust like som_game_640x480_4487f0/44a160 
		int xx = 60*SOM::width/640.0f;
		int yy = 48*SOM::height/480.0f;

		//TextOutA: expecting Ex_detours_TextOutA
		//todo? default to line-by-line translation
		const char *x = SOM::title('SYS2',lines.c_str());
		int x_s = x!=lines.c_str()?strlen(x):lines.size();		
		SetTextColor(dc,0x464646);TextOutA(dc,xx+1,yy+1,x,x_s); //61,49
		SetTextColor(dc,0xdcdcdc);TextOutA(dc,xx,yy,x,x_s); //60,48
	}
}
extern int som_state_viewing_show = false;
static int __cdecl som_state_4051F0_strcmp(char*,char*)
{
	return !(2==som_state_viewing_show); //2023: allow non-AVI video
}
static void __cdecl som_state_4051F0_intro(char *a)
{
	//NOTE: NOT UTILIZED BY OPENING SHOW (som_state_slides422AB0 is)

	/*bool movie = true; switch((DWORD)a)
	{
	case 0x45a11c: case 0x45a380: case 0x45a38c: case 0x45a398:
	case 0x45a374: case 0x45e57c:
	}*/
	bool movie = (DWORD)a>0x45ffff;

	som_state_viewing_show = 1+movie; //true
	((void(__cdecl*)(char*))0x4051F0)(a);
	som_state_viewing_show = false;
}
static void __cdecl som_state_4051F0_intro_2022(char *a) //2022
{
	//2022: start loading first map?
	SOM::queue_up_game_map(SOM::L.debug_map);

	som_state_4051F0_intro(a);
}
static BYTE __cdecl som_state_4053b0(DWORD s) //2022: slideshow textures
{
	EX::section raii(som_MPX_thread->cs);

	return ((BYTE(__cdecl*)(DWORD s))0x4053b0)(s);
}
static BYTE __cdecl som_state_42C6D0() //2022: title screen textures
{
	EX::section raii(som_MPX_thread->cs);

	return ((BYTE(__cdecl*)())0x42C6D0)();
}
static BYTE __cdecl som_state_42C6D0_start_1() //2022
{
	som_state_42C6D0();

	//this is mainly for demos since it starts in KF mode
	//NOTE: som_state_reprogram tests do_start/start_mode		
	//403715 plays intros and quits for SOM_SYS's preview
	//so in that case, don't load the test map to be nice
	DWORD &c = SOM::L.argc;
	char **v = SOM::L.argv;
	if(c<4||*(WORD*)v[3]!=0x742f) //"/t"
	SOM::queue_up_game_map(SOM::L.debug_map); return 1;
}

extern bool som_state_reading_text = false;
static void __cdecl som_state_409360(DWORD event, DWORD *stack) //Text
{		
	const evt::code00_t &eax = *(evt::code00_t*)*stack;
	
	*stack+=eax.next;  

	if('\1'==*eax.text) return; //2022: comment? empty?
	
	//2017: items are being taken without displaying a screen
	//assert(SOM::context._2017_delay>1);
	SOM::context = SOM::Context::reading_text;

	SOM::frame_is_missing();	
	som_state_reading_text = true;
	if(0) return ((void(__cdecl*)(DWORD,DWORD*))0x409360)(event,stack);			

	const char *title = SOM::title('MAP2',eax.text);
	if(*title) ((void(__cdecl*)(const void*,DWORD,DWORD,DWORD))0x423E50)
	(title,0xdcdcdc,1000,0x45835C);
	som_state_reading_text = false;
}
static void __cdecl som_state_409390(DWORD event, DWORD *stack) //Text+Font
{		
	//2017: items are being taken without displaying a screen
	//assert(SOM::context._2017_delay>1);
	SOM::context = SOM::Context::reading_text;

	SOM::frame_is_missing();	
	som_state_reading_text = true;
	const bool puremarkup = false; //todo: see if so
	const evt::code01_t &esi = *(evt::code01_t*)*stack;
	if(0) return ((void(__cdecl*)(DWORD,DWORD*))0x409390)(event,stack);		
	*stack+=esi.next; 
	const char *title = SOM::title('MAP2',esi.text_0_logfontface);	
	int blackness = esi.logfontweight; if(!blackness) blackness = 1000; //NEW
	if(*title) ((void(__cdecl*)(const void*,DWORD,DWORD,const void*))0x423E50)
	(title,esi.textcolorref,blackness,esi.text_0_logfontface+strlen(esi.text_0_logfontface)+1);	
	som_state_reading_text = false;
}
//static void __cdecl som_state_409BE0(DWORD event, DWORD *stack) //Warp
static void __cdecl som_state_409af0(DWORD event, DWORD *stack) //Map
{
	//HACK: this services 3 events (3b,3c,3d) (59-61)
	auto warp = (evt::code3d_t*)*stack;
	
	int f = warp->x3d;

	SOM::warped = SOM::frame;

	if(f==0x3c) (char*&)warp+=4; //3d is a subset of 3c

	som_MPX &mpx = *SOM::L.mpx->pointer;
	auto ll = (SOM::MPX::Layer*)&mpx[SOM::MPX::layer_0];

	//EXTENSION
	//NOTE: rel is "relative to subject" but it's
	//really just the subject's starting tile and
	//not even its relative offset
	const BYTE *rel = 0;	
	int swap2 = warp->settingmask; 
	float swap[3];
	auto &e = (evt::event_t&)SOM::L.events[event];
	//16 is a new checkbox so events
	//can cover a large are of tiles
	if(0!=(16&swap2)) switch(e.type) //EXTENSION
	{
		//EXTENSION
		//MapComp is extended to include this
		//data in MPX records
	case 0: rel = SOM::L.ai2[e.subject].uc+1; break;
	case 1: rel = SOM::L.ai[e.subject].uc+1; break;
	case 2: rel = SOM::L.ai3[e.subject].uc+5; break;
	}
	if(rel) //relative to subject w/ subject?
	{
		float xyz[3] = {rel[1]*2,0,rel[2]*2};
		int zi = -rel[0];
		if(zi>=-6&&ll[zi].tiles)
		{				 
			int xz = rel[2]*100+rel[1];
			if(xz>=0&&xz<100*100)
			{
				xyz[1] = ll[zi].tiles[xz].elevation;
			}
		}		

		float *os = warp->offsetting;		
		memcpy(swap,os,sizeof(swap));
		for(int i=3;i-->0;)
		{
			warp->settingmask|=2<<i;
			if(~swap2&(2<<i))
			os[i] = 0;
			os[i]+=SOM::L.pcstate[i]-xyz[i];
		}
	}

	auto &dst = *SOM::L.corridor; //debugging

	if(f==0x3c) //map change? 
	{
		//handoff to som_MPX_411a20?
		dst.zindex = warp->zindex; //EXTENSION

		//REMINDER: maps are read/changed only after 409af0 returns
		((void(__cdecl*)(DWORD,DWORD*))0x409af0)(event,stack);

		//EXTENSION
		if(~dst.settingmask&1)
		dst.heading[1] = SOM::L.pcstate[4];
		dst.heading[0] = SOM::L.pcstate[3]; //EXTENSION
		dst.heading[2] = SOM::L.pcstate[5]; //roll?
		dst.settingmask|=1;
	}
	else if(1) //implement 3d (warp) from scratch?
	{
		const evt::code3d_t &eax = *(evt::code3d_t*)*stack;
	
		*stack+=eax.x18; //2023  

		int mask = warp->settingmask;

		//NOTE: map changes update SOM::L.pcstate2 to the 
		//same value, but this event does not. in the past
		//that might have been a bug, but with the analog
		//system it's kind of a feature since short warps
		//get smoothed out (hopefully this feels natural)
		float *p = SOM::L.pcstate;

		float src[3]; if(14!=(mask&14))
		{
			//NOTE: the reason this has to be implemented
			//is two-fold. 1) it doesn't use 417540 so it
			//can't be tricked to ignore elevation. 2) it
			//is not clear what the source layer ought to
			//be, so some BLACK MAGIC heuristic is needed

			int x,z; assert(!rel);
			((BYTE(__cdecl*)(FLOAT,FLOAT,int*,int*))0x415bc0)(p[0],p[2],&x,&z);
			src[0] = x*2;
			src[2] = z*2; 
			src[1] = p[1]; if(~mask&4) //LEGACY?
			{
				//WHAT IS THE REFERENCE LAYER?
				//this should only be used if the source and 
				//destination are identical teleporater pods
				//and the event is isolated to a single tile
				int xz = z*100+x; if(xz>=0&&xz<100*100)
				{
					float closest = FLT_MAX;					
					for(int i=0;i>=-6;i--) if(ll[i].tiles) 
					{
						auto &tile = ll[i].tiles[xz];
						if(fabsf(p[1]-tile.elevation)<closest)
						{
							closest = tile.elevation;
						}
					}
					if(fabsf(p[1]-closest)<SOM::L.height*0.4f) //???
					{
						src[1] = closest;
					}
				}
			}
		}

		float dst[3] = {2*warp->setting[0],0,2*warp->setting[1]};
		{
			int zi = -warp->zindex;
			if(zi>=-6&&ll[zi].tiles)
			{				 
				int xz = warp->setting[1]*100+warp->setting[0];
				if(xz>=0&&xz<100*100)
				{
					dst[1] = ll[zi].tiles[xz].elevation;
				}
			}
		}

		for(int i=3;i-->0;) if(mask&(1<<i))
		{
			p[i] = dst[i]+warp->offsetting[i];
		}
		else p[i] = dst[i]+p[i]-src[i]; //LEGACY?

		if(mask&1) p[4] = warp->heading*0.01745329f;

		((void(__cdecl*)())0x402c10)(); //update tile visibility?
	}
	else ((void(__cdecl*)(DWORD,DWORD*))0x409be0)(event,stack);

	//EXTENSION
	if(rel) warp->settingmask = (BYTE)swap2;
	if(rel) memcpy(warp->offsetting,swap,sizeof(swap));
}
//static void __cdecl som_state_409570(DWORD event, DWORD *stack) //Warp Enemy
static void __cdecl som_state_4094f0(DWORD event, DWORD *stack) //Warp NPC
{
	auto warp = (evt::code19_t*)*stack;

	int f = warp->x19; //or warp->x20?

	som_MPX *swap = SOM::L.mpx->pointer;
	int &swap2 = (*swap)[SOM::MPX::layer_selector];

	if(warp->zindex) //UNFINISHED 
	{		
		int zi = -warp->zindex;
		int *p = &(*swap)[SOM::MPX::layer_0];
		if(zi>=-6&&p[10*zi])
		{
			swap2 = zi;
		}
		else //treat as-if zindex is 7
		{
			swap = 0; //this causes 417540 to return 0
		}
	}

	//PLACEHOLDER?
	//
	// 415c80 (subroutine) should be implemented from scratch to avoid
	// using the offset in getting the base elevation (if limited to 1
	// then it could be shaved off by a hair to keep it within bounds)
	//
	((void(__cdecl*)(DWORD,DWORD*))(f==0x19?0x4094f0:0x409570))(event,stack);

	SOM::L.mpx->pointer = swap; swap2 = 0;
}
static void __cdecl som_state_40a3e0(DWORD event, DWORD *stack) //Engage Object
{
	auto e = (evt::code64_t*)*stack;

	auto &ai = SOM::L.ai3[e->object];

	auto &evt = (evt::event_t&)SOM::L.events[event];
	if(evt.protocol<=2)
	{
		event = event; //breakpoint
	}

	if(e->object<SOM::L.ai3_size)
	if(auto*mdl=(SOM::MDL*)ai[SOM::AI::mdl3])
	if(int cmp=mdl->animation_id())
	{
		//HACK: the first opening frame is essentially closed
		if(cmp==4&&mdl->d==1) cmp = 5;

		auto &st = ai[SOM::AI::stage3]; //debbuging/testing

		//after a door finishes opening it's set to the closing
		//animation and held on the first frame
		auto &held = ai.c[0x7e];
		
		if(held) if(e->engage)
		{
			cmp = 4; //really being held on first closing frame
			
			//HACK: chests are held for 8, doors for 120... but
			//I don't want to look up the object's type because
			//it takes lots of code (it could use a subroutine)
			held = held<=8?8:120;
		}
		else held = 0;

		//NOTE: this is to implement KF2's drawbridges
		//(without involving synchronizing counters/timers that
		//likely cannot be implemented without issues)
		if(cmp==(e->engage?4:5))
		{
			//TESTING (necessary?)
			//I haven't looked into how this is managed... note
			//the id/state values happen ot be the same
			//st = cmp;

			*stack+=e->x08; return;
		}
	}
	((void(__cdecl*)(DWORD,DWORD*))0x40a3e0)(event,stack);
}
static void __cdecl som_state_40a480(DWORD event, DWORD *stack) //Warp Object
{
	auto warp = (evt::code66_t*)*stack;

	som_MPX *swap = SOM::L.mpx->pointer;
	int &swap2 = (*swap)[SOM::MPX::layer_selector];

	//WARNING
	//this extension extends the data
	//beyond the original size by 4Bs 
	if(warp->x1c>=0x20&&warp->zindex) //UNFINISHED 
	{
		int zi = -warp->zindex;
		int *p = &(*swap)[SOM::MPX::layer_0];
		if(zi>=-6&&p[10*zi])
		{
			swap2 = zi;
		}
		else //treat as-if zindex is 7
		{
			swap = 0; //this causes 417540 to return 0
		}
	}

	//PLACEHOLDER
	//
	// I think this needs to be implemented from scratch
	// since it uses the offset to get the base elevation
	// (assuming SOM_MAP doesn't convert 1 into 0.0998f)
	//
	// NOTE: animation stepping code may start at 42b0a1
	// (but it looks like it's float based by this point)
	//
	// NOTE: 40a480 includes code to extend the animation
	// to 60 frames-per-second (som_MDL_reprogram)
	((void(__cdecl*)(DWORD,DWORD*))0x40a480)(event,stack);

	SOM::L.mpx->pointer = swap; swap2 = 0;
}

//REMOVE ME
extern int som_state_sellout423480_beep = 0;
extern bool som_state_sellout423480_beeped = false;
static void __cdecl som_state_sellout423480(DWORD se) //Selling 
{
	//NOTE: 423480 PROBABLY PLAYS ALL NON-3D SOUNDS
	//2022: no it only plays menu sounds (with mute)

	//incessant beeping bug
//	if(0) *(BYTE*)0x1D1AE6B = 0; //always 1; menu sound support
	/*
	00423480 A0 6B AE D1 01   mov       al,byte ptr ds:[01D1AE6Bh] 
	00423485 84 C0            test        al,al 
	00423487 74 0F            je          00423498 
	00423489 8B 44 24 04      mov         eax,dword ptr [esp+4] 
	0042348D 6A 00            push        0    
	0042348F 50               push        eax  
	00423490 E8 AB C0 01 00   call        0043F540 
	00423495 83 C4 08         add         esp,8 
	00423498 C3               ret		  */
	//detected/set by som.menus.cpp
	if(!som_state_sellout423480_beeped) 
	{
		//Reminder: This depends on the SOM_SYS sound-suite selection.
		som_state_sellout423480_beeped = true;
		((void(__cdecl*)(DWORD))0x423480)(som_state_sellout423480_beep=se);
	}
}

static void __cdecl som_state_409d50(DWORD event, DWORD *stack) //Status
{
	//ecx: standard prolog
	const evt::code50_t &ecx = *(evt::code50_t*)*stack;
	assert(ecx.x0c==0x0c);
	bool items = 4==ecx.status;
	assert(ecx.status2!=0xFF);
	bool simple = 0==ecx.status2||250>=ecx.status2;
	if(ecx.operation<4) 
	if(items||simple&&ecx.status<6) 
	return ((void(__cdecl*)(DWORD,DWORD*))0x409d50)(event,stack);
	*stack+=ecx.x0c;	
	
	BYTE *out = 0, *pce = SOM::L.pcequip;
		
	//11-18: change equipment
	if(ecx.status>=11&&ecx.status<=18)
	{
		if(0==ecx.status2) switch(ecx.status)
		{
		case 11: out = pce+1; break; //helmet
		case 12: out = pce+0; break; //weapon
		case 13: out = pce+5; break; //shield
		case 14: out = pce+3; break; //arms
		case 15: out = pce+2; break; //upperbody			
		case 16: out = pce+4; break; //legs
		case 17: out = pce+6; break; //accessory
		case 18: out = pce+7; break; //magic
		}
	}

	if(out-pce<8&&SOM::L.pcequip2) //animating?
	{
		memcpy(pce,SOM::L.pcequip2_in_waiting,8);
	}

	int i = ecx.operand;
		
	if(out) switch(ecx.operation)
	{
	case 0: *out = min(i,0xFF); break; 
	case 1: *out = min(i+*out,0xFF); break; 
	case 2: *out = max((int)*out-i,0); break; 
	case 3: *out = min(SOM::L.counters[i],0xFF); break;
	}

	if(out-pce<8) //update swing model / naked equip
	{
		extern void som_game_equip(); som_game_equip();
	}
}
static void __cdecl som_state_40a330(DWORD event, DWORD *stack) //Counter+Status
{	
	//eax: standard prolog
	const evt::code54_t &eax = *(evt::code54_t*)*stack;
	assert(eax.x08==8); 
	bool items = 4==eax.status;
	assert(eax.status2<250);
	bool simple = 0==eax.status2||250>=eax.status2;
	if(items||simple&&eax.status<7)
	return ((void(__cdecl*)(DWORD,DWORD*))0x40a330)(event,stack);
	*stack+=eax.x08; 		
	
	WORD out = -1;

	switch(eax.status)
	{
	case 0: //speed

		switch(eax.status2) 
		{
		case 4: break; //speed
		}
		break;

	case 6: //level table
	{
		static int cache[5] = {-1};

		int lv = SOM::L.pcstatus[SOM::PC::lv];

		if(cache[0]!=lv)
		{
			cache[0] = lv;
			cache[1] = SOM::L.setup->hp;
			cache[2] = SOM::L.setup->mp;
			cache[3] = SOM::L.setup->str;
			cache[4] = SOM::L.setup->mag;

			BYTE *abcd = (BYTE*)&SOM::L.lvtable+4;

			if(lv<99) for(int i=0;i<lv;i++,abcd+=8)
			{
				for(int i=0;i<4;i++) cache[i]+=abcd[i];
			}
		}

		if(lv>=0&&lv<100) switch(eax.status2) 
		{
		case 1: //next: 0-99 or 100 at level 99

			if(lv<99) 
			{					
				DWORD next = SOM::L.lvtable[lv];
				DWORD prev = !lv?0:SOM::L.lvtable[lv-1];
				DWORD t = (DWORD&)SOM::L.pcstatus[SOM::PC::pts];

				if(t<next) if(t>=prev)
				{	
					float x = float(t-prev)/float(next-prev);

					if(x>=0&&x<100) out = x;
				} 
			}
			else 0; break;

		case 2: out = cache[1]; break; //max hp
		case 3: out = cache[2]; break; //max mp
		case 4: out = cache[3]; break; //max strength
		case 5: out = cache[4]; break; //max magic
		case 6: break; //max speed
		}
		break;
	}
	case 7: //magic table

		//2018: looks like a nasty bug... WTH was I
		//thinking???
		//out = SOM::L.pcmagic[1<<eax.status2]?1:0;
		out = SOM::L.pcmagic&(1<<eax.status2)?1:0;
		break;

	case 8: //status mode
	{
		int st = eax.status2-1; //-normal

		out = SOM::L.pcstatus[SOM::PC::xxx];
		
		if(0==eax.status2)
		{
			out = !out; //normal
		}
		else if(st<5) //timers 0~1000 (10s)
		{
			WORD &timer = SOM::L.pcstatus[2*st+SOM::PC::xxx_timers];

			out = out&1<<st?timer:0; assert(out==timer);
		}
		else out = 0;
		break;
	}
	case 9: //system mode

		switch(eax.status2)
		{
		case 0: out = SOM::mpx; break;
		case 1: out = SOM::setup(); break;
		case 2:	out = SOM::L.save; break;
		case 3:	out = SOM::L.mode; break;
		}
		break;

	case 10: //fall impact

		switch(eax.status2) //testing
		{
		case 0:	out = SOM::motions.fallheight;

			SOM::motions.fallheight = 0; break;

		case 1: out = SOM::motions.fallimpact;

			SOM::motions.fallimpact = 0; break;
		}
		break;

	default: //11-18 (equipment?)
		
		int e = -1;	switch(eax.status)
		{
		case 11: e = 1; break; //helmet
		case 12: e = 0; break; //weapon
		case 13: e = 5; break; //shield
		case 14: e = 3; break; //arms
		case 15: e = 2; break; //upperbody			
		case 16: e = 4; break; //legs
		case 17: e = 6; break; //accessory
		case 18: e = 7; break; //magic
		}
		if(e<0||e>7) break; //7: paranoia

		//int e2 = SOM::L.pcequip[e];
		int e2 = SOM::L.pcequip2_in_waiting[e];
		bool nothing = e2>=250||e==7&&e2>=32;
		extern int *som_game_nothing();
		if(!nothing) nothing = e2==som_game_nothing()[e];

		if(0==eax.status2) //equipement
		{
			out = nothing?65535:e2; 
		}
		else if(eax.status2<9) //coefficients
		{	
			BYTE *c8 = 0;		
			if(!nothing) if(e==7) //magic
			{
				c8 = SOM::L.magic_prm_file->uc;
				c8+=int(SOM::L.magic32[e2])*320+48;
			}
			else //equipment (assuming)
			{
				c8 = SOM::L.item_prm_file->uc;
				c8+=e2*336+300;
			}
			out = nothing?0:c8[eax.status2-1];
		}		
		break;
	}

	SOM::L.counters[eax.counter] = out;
}
extern bool som_state_saving_game = false;
static void __cdecl som_state_40a5f0(DWORD event, DWORD *stack) //Save
{
	//2017: Items are being taken without displaying a screen.
	//assert(SOM::context._2017_delay>1);
	SOM::context = SOM::Context::saving_game;

	//REMINDER: indicates EVENT BASED saving only
	//REMINDER: indicates EVENT BASED saving only
	//REMINDER: indicates EVENT BASED saving only
	som_state_saving_game = true;
	//HMM??? 0x219F9EC may not be a fixed address... maybe it
	//is general purpose event memory?? Debug builds appear to
	//be reliable, but not Release builds?!	(it's on the stack)
	//som_game_4212D0 is overriding it
	//NOTE: 1e9b000 is the end of the module
	SOM::menupcs = (DWORD*)0x219f9ec;
	((void(__cdecl*)(DWORD,DWORD*))0x40a5f0)(event,stack);
	SOM::menupcs = (DWORD*)0x1A5B4A8;
	som_state_saving_game = false;

	//eliminates glitch after picture menu
	SOM::frame_is_missing(); 
}
extern bool som_state_reading_menu = false;
extern char *som_state_40a8c0_menu_text = 0;
extern RECT som_state_40a8c0_menu_yesno[2] = {};
static void __cdecl som_state_40a8c0(DWORD event, DWORD *stack) //Menu
{
	assert(!som_state_reading_menu);
		
	//2017: Items are being taken without displaying a screen.
	//assert(SOM::context._2017_delay>1);
	SOM::context = SOM::Context::reading_menu;

	SOM::frame_is_missing(); 
	som_state_reading_menu = true;
	evt::code8D_t &x8D = *(evt::code8D_t*)*stack; if(1)
	{		
		char swap[8+40*7+1+40+1+40+1+4];
		som_state_40a8c0_menu_text = (char*)memcpy(swap,&x8D,x8D.next)+4;
		char *yes = som_state_40a8c0_menu_text+strlen(x8D.text_0_yes_0_no)+1;
		sprintf(x8D.text_0_yes_0_no,"%s%c%s%c%s",SOM::title('MAP2',swap+4),0,yes,0,yes+strlen(yes)+1);	
		((void(__cdecl*)(DWORD,DWORD*))0x40a8c0)(event,stack);	
		som_state_40a8c0_menu_text = 0; memcpy(&x8D,swap,x8D.next);
		som_state_40a8c0_menu_yesno[0].left = 0;
	}
	else ((void(__cdecl*)(DWORD,DWORD*))0x40a8c0)(event,stack);		
	som_state_reading_menu = false;
}
static void __cdecl som_state_40adc0(DWORD event, DWORD *stack) //Counter
{		
	//eax: standard prolog
	const evt::code90_t &eax = *(evt::code90_t*)*stack;
	assert(eax.x0c==0xc); 
	if(eax.operation<3&&eax.source<2)
	return ((void(__cdecl*)(DWORD,DWORD*))0x40adc0)(event,stack);
	*stack+=eax.x0c; 	
						
	float lv = 
	SOM::L.counters[eax.counter];
	float rv = eax.source?
	SOM::L.counters[eax.operand]:eax.operand;
	switch(eax.operation)
	{
	case 3: lv = rv-lv; break;
	case 4: lv = lv*rv; break;
	case 7: lv*=100; case 5: lv = lv/rv; break;
	case 8: lv*=100; case 6: lv = rv/lv; break;
	case 9: lv = rv<1?0:int(lv)%int(rv); break;
	case 10: lv = lv<1?0:int(rv)%int(lv); break;
	}
	SOM::L.counters[eax.counter] = 
	_finite(lv)?max(0,min(lv,65535)):65535;	
}
static void __cdecl som_state_40a5a0(DWORD event, DWORD *stack) //System
{	
	//THERE SEEMS TO BE A BUG HERE...
	//1D12D62 concerns saving in the menu. If that's not on
	//then it's impossible to change the save mode state on/off
	//But the save mode state in actuality only affects the Saving
	//event instruction (19C1DDF) and has no effect menu saving wise!
	//return ((void(__cdecl*)(DWORD,DWORD*))0x40a5a0)(event,stack);
	/*
	0040A5B0 33 C9            xor         ecx,ecx 
	0040A5B2 8A 48 04         mov         cl,byte ptr [eax+4] 
	0040A5B5 83 E9 00         sub         ecx,0 
	0040A5B8 74 1B            je          0040A5D5 
	0040A5BA 49               dec         ecx  
	0040A5BB 75 30            jne         0040A5ED 
	0040A5BD 8A 0D 8E 2B D1 01 mov         cl,byte ptr ds:[1D12B8Eh] 
	0040A5C3 84 C9            test        cl,cl 
	0040A5C5 74 26            je          0040A5ED 
	0040A5C7 8A 48 05         mov         cl,byte ptr [eax+5] 
	0040A5CA 84 C9            test        cl,cl 
	0040A5CC 0F 95 C0         setne       al   
	0040A5CF A2 DE 1D 9C 01   mov         byte ptr ds:[019C1DDEh],al 
	0040A5D4 C3               ret              
	0040A5D5 8A 0D 62 2D D1 01 mov         cl,byte ptr ds:[1D12D62h] 
	0040A5DB 84 C9            test        cl,cl 
	0040A5DD 74 0E            je          0040A5ED 
	0040A5DF 8A 48 05         mov         cl,byte ptr [eax+5] 
	0040A5E2 84 C9            test        cl,cl 
	0040A5E4 0F 95 C1         setne       cl   
	0040A5E7 88 0D DF 1D 9C 01 mov         byte ptr ds:[19C1DDFh],cl 
	0040A5ED C3               ret              
	*/
	//eax: standard prolog
	const evt::code78_t &eax = *(evt::code78_t*)*stack;
	*stack+=eax.x08; assert(eax.x08==0x8); 

	switch(eax.sysmode)
	{
	case 0: SOM::L.save/*2*/ = eax.newflag; break;
	case 1: SOM::L.mode/*2*/ = eax.newflag; break;

	case 2: SOM::play = eax.newflag; break;
	}
}

//REMOVE ME?
//REMOVE ME?
//REMOVE ME?
static __int64 __cdecl som_state_automap44F938()
{
	//force origin to be 0,0
	//reminder: return is not a double
	//(in fact, the point of this routine is 
	//probably to convert it into an integer pixel.)
	((__int64(__cdecl*)())0x44F938)(); //fstp	
	return 0;
}
static float som_state_automap_dimens[2];
static DWORD __cdecl som_state_automap423450(float in)
{
	static int alt = 0; return som_state_automap_dimens[alt=!alt] = in; 
}
static BYTE __cdecl som_state_automap448930(DWORD h,DWORD i,DWORD j,DWORD di,DWORD dj,DWORD x,DWORD y,DWORD dx, DWORD dy)
{
	if(0) return ((BYTE(__cdecl*)(DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD))0x448930)(h,i,j,di,dj,x,y,dx,dy);

	//448930 blts to the screen

	extern SOM::Texture *som_status_mapmap;
	extern void som_status_automap(int,int,int,int);
	som_status_mapmap =	SOM::L.textures+h;
	
	static unsigned frame0 = 0;
	static unsigned noauto = -1; //optimization
	if(SOM::automap<SOM::frame-1)	
	{
		//noauto assumes the entire map will always be visible
		unsigned hash = DDRAW::noResets<<6|SOM::mpx;
		if(noauto!=hash) 
		{
			noauto = hash; frame0 = SOM::frame;
		}		
	}
	SOM::automap = SOM::mapmap = SOM::frame;

	if(frame0==SOM::frame)
	{
		x = 0.5f+(float)x/som_state_automap_dimens[0];
		y = 0.5f+(float)y/som_state_automap_dimens[1];
		
		som_status_automap(i,j,3*x,3*y); //fill out		
	}

	return 0; //failure? automap ignores the result
}
static BYTE __cdecl som_state_automap4487F0(DWORD h,DWORD i, DWORD x,DWORD y,DWORD dx, DWORD dy)
{		
	//2017: WHAT DOES 4487F0 DO? IT'S NOT RENDERING.

	if(0) return ((BYTE(__cdecl*)(DWORD,DWORD,DWORD,DWORD,DWORD,DWORD))0x4487F0)(h,i,x,y,dx,dy);

	extern void som_status_mapmap__free(); //HACK!!
	if(SOM::mapmap<SOM::frame-1) 
	som_status_mapmap__free();

	assert(i==1); //???

	extern SOM::Texture *som_status_mapmap;
	som_status_mapmap =	SOM::L.textures+h;
	SOM::mapmap = SOM::frame;

	return 0; //failure? automap ignores the result
}

//EXPERIMENTAL (PlayStation VR)
static void __cdecl som_state_403BB0()
{
	//this subroutine sets up the frustum
//	(float&&)DWORD(0x3f490fe0)
	//this inner routine is particularly interesting, because it seems
	//to be using a __thiscall convention, which the player never uses
	//("this" is SOM::L.view_matrix_xyzuvw)
	//00403BC7 E8 A4 D4 FF FF       call        00401070 
	{
		/*
		00401070 56                   push        esi  
		00401071 8B 74 24 08          mov         esi,dword ptr [esp+8]  
		00401075 85 F6                test        esi,esi  
		00401077 74 26                je          0040109F  
		00401079 8B 54 24 0C          mov         edx,dword ptr [esp+0Ch]  
		0040107D 85 D2                test        edx,edx  
		0040107F 74 1E                je          0040109F  
		00401081 8B 44 24 10          mov         eax,dword ptr [esp+10h]  
		00401085 85 C0                test        eax,eax  
		00401087 74 16                je          0040109F  
		00401089 57                   push        edi  
		0040108A 8B 39                mov         edi,dword ptr [ecx]  
		0040108C 89 3E                mov         dword ptr [esi],edi  
		0040108E 8B 71 04             mov         esi,dword ptr [ecx+4]  
		00401091 89 32                mov         dword ptr [edx],esi  
		00401093 8B 49 08             mov         ecx,dword ptr [ecx+8]  
		00401096 5F                   pop         edi  
		00401097 89 08                mov         dword ptr [eax],ecx  
		00401099 B0 01                mov         al,1  
		0040109B 5E                   pop         esi  
		0040109C C2 0C 00             ret         0Ch  
		0040109F 32 C0                xor         al,al  
		004010A1 5E                   pop         esi  
		004010A2 C2 0C 00             ret         0Ch 
		*/
	}

	//TODO: CAN USE THIS FOR MORE THAN JUST HEAD-TRACKING!
	//TODO: CAN USE THIS FOR MORE THAN JUST HEAD-TRACKING!
	//TODO: CAN USE THIS FOR MORE THAN JUST HEAD-TRACKING!
	if(1||DDRAW::inStereo)
	{
		//403BB0 calls 2 subroutines prior to setting
		//up the frustum, but this seems to work with 
		//or without them
		//((void(__cdecl*)())0x403BB0)();
		extern void som_mocap_403BB0();
		som_mocap_403BB0();
		//this code is entered when tiles are updated
		//EX::dbgmsg("som_state_403BB0 %d",SOM::frame);
		//00403D19 A1 FC 23 4C 00       mov         eax,dword ptr ds:[004C23FCh]  
		//00403D1E 8B 0D F8 23 4C 00    mov         ecx,dword ptr ds:[4C23F8h]
		
		//there's code here that accesses the frustum
		//however it's never entered?
		//417ea0 417ece 417ef8 417f1c
	}
	else ((void(__cdecl*)())0x403BB0)();
}
static void __cdecl som_state_403BB0_alt() //REMOVE ME?
{
	//2022: this can't be good. this routine is called whenever tile
	//visilibity is refreshed, including after map change and warps
	//and some odd ones that don't make sense
	//SOM::warp(SOM::L.pcstate,SOM::L.pcstate);
	
	//is this still a good idea? I think som_state_403BB0_alt is called
	//at start up only (2022: NO IT'S NOT)
	if(DDRAW::inStereo) if(DDRAW::xr)
	{
		#ifdef NDEBUG
//		#error refactor me (separate VR pose)
		#endif
		float swing[6], skyanchor = 
		SOM::motions.place_camera(SOM::analogcam,SOM::steadycam,swing);
	}
	else SOM::PSVRToolbox(); //??? //frustum parameters?

	som_state_403BB0();
}

extern int som_game_hp_2021;
extern void som_clipc_reset();
extern void SOM::reset()
{		
	SOM::pcdown = SOM::frame;
	//REMOVE ME
	som_game_hp_2021 = 0; 
		
	//overwrite save file with defaults	
	SOM::L.save2 = 1; //bug
	SOM::L.mode2 = 1; //???
	SOM::L.mode = SOM::mode;
	SOM::L.save = SOM::save;		

	SOM::play = true; //third system mode

	for(int i=0;i<EX_AFFECTS;i++) 
	{
		EX::Affects[i].activate(); //hack
	}

	//2021
	//som_state_slipping = false;
	//som_state_slipping2 = false;
	//som_state_haircut2[0] = 0;
	//som_state_haircut2[1] = 0;
	som_clipc_reset();

	SOM::motions.reset_config();
}
extern void som_mocap_dy(float d);
extern void SOM::warp(FLOAT dst[6], FLOAT src[6])
{
	float d[6]; if(!src) //2022: KF2 map change?
	{
		float(&p)[6+6] = SOM::L.pcstate;
		float(&pp)[6] = SOM::L.pcstate2;
		
		float dd[6];
		Somvector::map(dd).copy<6>(pp).remove<6>(p);
		Somvector::map(d).copy<6>(dst).remove<6>(p);

		memcpy(p,dst,sizeof(d));

		for(int i=3;i-->0;)
		{
			SOM::err[i]+=d[i];			
			SOM::xyz[i]+=d[i];			
			SOM::xyz_past[i]+=d[i];
			SOM::f3state[i] = p[i];			
			SOM::cam[i]+=d[i]; //SFX (billboards)
		}
		som_mocap_dy(d[1]); //YUCK (som.mocap.cpp)

		Somvector::map(pp).copy<6>(p).move<6>(dd);

		//update 3D sound coordinates? (footsteps)
		{
			//TODO: som.MPX.cpp SHOULD SIGNAL TO
			//CANCEL SOUNDS OTHERWISE, WHICH THIS
			//CODE IS WELL POSITIONED TO IMPLEMENT
			
			//n will be 5 if do_sounds is off. in
			//which case it's not worth bothering
			int n = SOM::L.snd_dups_limit;
			if(n==1)
			for(int i=SOM::L.snd_bank_size;i-->0;)
			{
				auto &se = SOM::L.snd_bank[i];

				//emulating 44c100
				if(!se.unknown6||!se.sb3d) continue;

				DSOUND::IDirectSoundBuffer*(*p)[5];
				(void*&)p = (void*)se.sb;

				enum{ j=0 };
				//for(int j=n;j-->0;)
				if((*p)[j])
				{
					//assert(!j); //do_sounds?
					auto *m = (*p)[j]->master;
					if(m->forwarding)
					(*p)[j]->master->movefwd7(d);
				}
			}
		}

		goto d;
	}
	else SOM::warped = SOM::frame;

	if(SOM::pcdown!=SOM::frame)
	{
		Somvector::map(d).copy<6>(dst).remove<6>(src);

		//// translation /////////////////

		Somvector::map(SOM::err).move<3>(d);

		//paranoia: worst case scenario
		if(Somvector::measure<3>(SOM::err,dst)>1) //1: human scale
		{
			for(int i=0;i<3;i++)
			SOM::f3state[i] =
			SOM::xyz_past[i] = SOM::xyz[i] = SOM::err[i] = dst[i];
			SOM::xyz_step = 0;
			//2022: fix SFX billboards (DUPLICATE)
			float Y = SOM::xyz[1]+SOM::eye[1];
			SOM::cam[0] = SOM::xyz[0];
			SOM::cam[1] = Y;
			SOM::cam[2] = SOM::xyz[2];
			SOM::cam[3] = Y-0.3f-SOM::xyz[1]; //1.2
		}	
		else //compensate for the difference
		{
			Somvector::map(SOM::xyz_past).move<3>(d);
			Somvector::map(SOM::xyz).move<3>(d);			
			Somvector::map(SOM::cam).move<3>(d);
		}
			
		//// rotation ////////////////////

	d:	SOM::arm[1]+=d[4]; SOM::arm[0]+=d[3];
		SOM::uvw[1]+=d[4]; SOM::uvw[0]+=d[3];

		float q[4];	Somvector::map(q).quaternion<0,1,0>(d[4]);
		for(int i=0;i<EX_AFFECTS;i++) 
		Somvector::map(EX::Affects[i].position).rotate<3>(q);
	}
	else //resetting
	{
		for(int i=0;i<3;i++) 
		SOM::f3state[i] =
		SOM::xyz_past[i] = SOM::xyz[i] = SOM::err[i] = dst[i];
		SOM::xyz_step = 0;
						
		SOM::arm[1] = SOM::uvw[1] = dst[4]; SOM::err[3] = 0;
		SOM::arm[0] = SOM::uvw[0] = dst[3]; 	

		//2022: fix SFX billboards (DUPLICATE)
		SOM::eye[1] = EX::INI::Player()->player_character_stature;
		float Y = SOM::xyz[1]+SOM::eye[1];
		SOM::cam[0] = SOM::xyz[0];
		SOM::cam[1] = Y;
		SOM::cam[2] = SOM::xyz[2];
		SOM::cam[3] = Y-0.3f-SOM::xyz[1]; //1.2
	}

	som_state_pov_vector_from_xyz3_and_pov3();
}

extern int SOM::config(const char *keyname, int def)
{
	return SOM::Game::get(keyname,def);
}				
extern int SOM::rescue(const wchar_t *keyname, int def)
{
	return GetPrivateProfileIntW(L"rescue",keyname,def,SOM::Game::title('.ini'));
}

extern DWORD SOM::thread()
{
	static DWORD out = 0; if(out) return out;

	if(!SOM::image()) return out = GetCurrentThreadId();

	char *paranoia;		
	out = strtoul((char*)0x40004E,&paranoia,16);

	if(paranoia==(char*)0x40004E+8)
	{
		SetThreadName(out,"EX::threadmain"); return out;
	}
	else assert(0);
	
	return out = GetCurrentThreadId();
}

extern int SOM::image()
{			
	static int out = 0; 
				
	/*or unrecognized if out is 0*/
	static bool recognized = false;

	if(recognized) return out; 

	bool unmarked = //assuming this string is ubiquitous to win32 PE files
	!memcmp((void*)0x40004E,"This program cannot be run in DOS mode.",39);

	if(unmarked)
	{
		recognized = true; return out = 0;
	}

  //// code purloined from som_ex.exe (of SOM_EX.exe) 
  //
  // NEW: code now resides in som.exe.cpp (of SomEx.dll)

	DWORD rdata = 0, data = 0; //hack
		
	LPCVOID guess = (LPCVOID*)0x400000; //hack

	MEMORY_BASIC_INFORMATION info; 
	
	bool som_run = 0;

	HANDLE cp = GetCurrentProcess(); //...

	while(VirtualQueryEx(cp,guess,&info,sizeof(info)))
	{
		DWORD i = (DWORD)info.BaseAddress;
		if(info.AllocationProtect&PAGE_EXECUTE_WRITECOPY)
		{				
			switch(i) 
			{
			case 0x00414000: //MPX rdata //EneEdit/NpcEdit rdata

				rdata = i; break;
			
			case 0x00415000: //MPX data
				
				data = i; out = 'mpx'; break;

			case 0x00416000: //EneEdit/NpcEdit data

				//from workshop_reprogram
				//0040329F 80 7A 03 01          cmp         byte ptr [edx+3],1  
				//004032A3 74 25                je          004032CA  
				out = *(DWORD*)0x40329F==0x01037A80?'ene':'npc';

				data = i; break;

			case 0x00420000: //ItemEdit rdata							
			case 0x00425000: //ObjEdit rdata

				rdata = i; break;

			case 0x00426000: //ItemEdit data //PrtsEdit rdata

				if(rdata) //ObjEdit
				{
					data = i; out = 'item';
				}
				else rdata = i; break;

			case 0x0042B000: //MAIN rdata //ObjEdit data 
				
				if(rdata) //ObjEdit
				{
					data = i; out = 'obj'; break;
				}

				rdata = i; break;
							
			case 0x00430000: //PrtsEdit data

				data = i; out = 'prts'; break;

			case 0x00436000: //MAIN data

				data = i; out = 'main'; break;

			case 0x0042D000: //RUN rdata (JDO unlocked)

				rdata = i; //FALL THRU...

			case 0x00428000: //RUN rdata
			
				if(!rdata) rdata = i;

				som_run = true;	break;

			case 0x00429000: //RUN data / EDIT rdata

				if(!som_run) //then assume EDIT
				{
					rdata = i; break;
				}
				else; //FALL THRU...

			case 0x00437000: //RUN data (JDO unlocked)

				data = i; assert(som_run);
				
				out = 'run'; break;

			case 0x00433000: //EDIT data

				data = i; out = 'edit'; break;

			case 0x00464000: //PRM rdata

				rdata = i; break;

			case 0x00476000: //PRM data

				data = i; out = 'prm'; break;

			case 0x00478000: //MAP rdata

				rdata = i; break;

			case 0x0048E000: //MAP data

				data = i; out = 'map'; break;

			case 0x0043C000: //SYS rdata

				rdata = i; break;

			case 0x0044B000: //SYS data
				
				data = i; out = 'sys'; break;

			//original som_db.exe/patched som_rt.exe...
			case 0x00457000: rdata = i; break; 
			case 0x00459000: data = i; break;

			//original som_rt.exe...
			case 0x00456000: rdata = i; break; 

			case 0x00458000: //ambiguous: can be patched som_db.exe...
				
				if(rdata) data = i; //then original som_rt.exe
				
				//patched som_db.exe...

					else rdata = i; break; //...				

		  //case 0x00458000: rdata = i; break; 

			case 0x0045A000: data = i; break;
			}
		}

		guess = (LPCVOID)(i+info.RegionSize);

		if(data&&rdata) break; //early out
	}

	if(!out)
	/*//hack: db1&rt2 are ambiguous
	switch((int)rdata+!SOMDB::detected()) 
	{	
	case 0x00456001: out = 'rt1'; break; //original som_rt.exe
	case 0x00457001: out = 'rt2'; break; //patched som_rt.exe
    case 0x00457000: out = 'db1'; break; //original som_db.exe
	case 0x00458000: out = 'db2'; break; //patched som_db.exe
	}*/
	switch((int)rdata)
	{	
	case 0x00458000: out = 'db2'; break; //patched som_db.exe
	case 0x00457000: out = 'rt2'; //break; //patched som_rt.exe
		
		/*som_state_reprogram_image reports much the same message
		if(SOMEX_VNUMBER>=0x01010202)
		{
			EX::messagebox(MB_OK|MB_ICONERROR
			"You are seeing this message because support for som_rt.exe has been deprecated.");
		}
		break;*/

	default:

		if(EX::ok_generic_failure(L"Unsupported PE image"))
		{
			EX::is_needed_to_shutdown_immediately();
		}
	}

	if(out) //NEW
	{
		SOM::image_rdata = rdata;
		SOM::image_data = data; 
	}

	recognized = true; return out;
}
extern DWORD SOM::image_rdata = 0;
extern DWORD SOM::image_data = 0;

//REMOVE US (these are used by ancient 10yo+ code)
static void som_state_route(int op, int at, DWORD set)
{																			   
	DWORD *mem = (DWORD*)(0x401000+op+at); *mem = set;
}
extern void som_state_word(DWORD at, WORD safe, WORD set)
{
	WORD *mem = (WORD*)(0x401000+at); assert(*mem==safe);

	if(*mem==safe) *mem = set;
}
extern void som_state_dword(DWORD at, DWORD safe, DWORD set)
{
	DWORD *mem = (DWORD*)(0x401000+at); assert(*mem==safe);

	if(*mem==safe) *mem = set;
}
template<typename T, typename U>
inline void som_state_dword(DWORD at, const T &safe, const U &set)
{
	som_state_dword(at,(DWORD)safe,(DWORD)set);
}

//SOM_DB's implements this with an infinite loop
extern float __cdecl som_state_clampi(DWORD call_ret, float a) 
{
	if(_finite(a)) return fmodf(a+M_PI*5,M_PI*2)-M_PI; assert(!"finite"); 
	return 0; //todo? restart som.mocap.cpp
}			   

static FLOAT som_state_cone = 0;
static FLOAT som_state_uv[2] = {1,1};

//REMOVE ME (eventually)
extern void som_state_reprogram_image() //SomEx.cpp
{
	int image = SOM::image(); assert(image);

	if(SOM::tool) //NEW: now with tools!
	{
		//SHOULD PROBABLY JUST REMOVE THIS?
		if(SOM::tool!=SOM_MAIN.exe
		 &&SOM::tool!=SOM_PRM.exe
		 &&SOM::tool!=SOM_MAP.exe&&SOM::tool!=MapComp.exe
		 &&SOM::tool!=SOM_SYS.exe
		 &&SOM::tool<PrtsEdit.exe) return; //not on the list
	}

	EX::INI::Bugfix bf; EX::INI::Window wn;
	EX::INI::Option op;	EX::INI::Detail dt;

	EXLOG_LEVEL(5) << "Initializing virtual memory state...\n";
	 	
	SOM::memory = EX::memory(); 

	if(SOM::memory)
	if(SOM::memory->info.BaseAddress==(void*)0x400000)
	if(SOM::memory->next->info.BaseAddress==(void*)0x401000)
	{		
		SOM::text = SOM::memory->next;		
		SOM::rdata = SOM::text->next; //assuming
		SOM::data = SOM::rdata->next; //assuming		
	}
	assert(SOM::text);
	assert(SOM::text->info.BaseAddress==(void*)0x401000);

	//REMOVE ME?
	const char *saveface = 0;	
	
	switch(image) 
	{		   	
	case 'rt1': //r456000/458000 //VERY OBSOLETE

//		saveface = (CHAR*)0x45633C; 

		//// NO LONGER SUPPORTING ////
		//EX::messagebox(MB_OK|MB_ICONERROR
		//"This version of som_rt.exe is no longer supported by SomEx.dll");

		//break;
	case 'rt2': //r457000/459000 //OBSOLETE

		EX::messagebox(MB_OK|MB_ICONERROR,
		"som_rt.exe is no longer supported by SomEx.dll");
		break; 
		
//		saveface = (CHAR*)0x4574CC; //rdata
/*
		SOM::L.hold = (DWORD*)0x423A23; //text
		
		SOM::L.duck   = (FLOAT*)0x45735C; 
		SOM::L.hitbox = (FLOAT*)0x457360;
		SOM::L.height = (FLOAT*)0x4574DC;
		SOM::L.shape  = (FLOAT*)0x4574E0;		
		SOM::L.bob    = (FLOAT*)0x4574FC;		
		SOM::L.nod    = (FLOAT*)0x457500;
		//Note: this address looks the same
		//SOM::L.nod    = (FLOAT*)0x457540;
		SOM::L.abyss  = (FLOAT*)0x457508;
		SOM::L.fence  = (FLOAT*)0x457534;				
									  
		SOM::L.on_off = (BYTE*)0x4BF662;
		SOM::L.f3 = &SOM::L.on_off[13]; 
		SOM::L.f4 = &SOM::L.on_off[14];
		SOM::L.config = (UINT*)0x4BF780; 
		SOM::L.view_matrix_xyzuvw = (FLOAT*)0x4C1168;
		SOM::L.frustum = (FLOAT*)0x4C1200;

		SOM::L.ai = (SOM::Struct<149>*)0x4C65D8;

		SOM::L.counters = (WORD*)0x5543D4; 
		SOM::L.leafnums = (BYTE*)0x554C54;
		SOM::L.controls = (BYTE*)0X5553D8;
				
		SOM::L.control_ = (BYTE*)0x19A9984;

		SOM::L.pcequip = (BYTE*)0x19C0A1C;
		SOM::L.pcstatus = (WORD*)0x19C0A34;		
		SOM::L.pcstate = (FLOAT*)0X19C0BA8;		
		SOM::L.pcstate2 = SOM::L.pcstate+6;		
		SOM::L.dashing = (DWORD*)0x19C0BE8;
						   		
		SOM::L.mode = (BYTE*)0x1D1199E;
		SOM::L.walk = (FLOAT*)0x1D119A0;
		SOM::L.dash = (FLOAT*)0x1D119A4;
		SOM::L.turn = (SHORT*)0x1D119A8;
*/
		break; 
			
	case 'db1': //r457000/459000 //VERY OBSOLETE
		
		//// NO LONGER SUPPORTING ////

		MessageBoxW(EX::client,
		L"This version of som_db.exe is no longer supported by SomEx.dll",
		EX::exe(),MB_OK|MB_ICONERROR);		
		break; 

//		saveface = (CHAR*)0x45735c; 
/*		
		SOM::L.counters = (WORD*)0x554874;
		SOM::L.leafnums = (BYTE*)0x5550F4;
		SOM::L.pcstatus = (WORD*)0x19c0e9c;
*/
	case 'db2': //r458000/45A000

		saveface = (CHAR*)0x45835C; //rdata

/*		SOM::L.hold = (DWORD*)0x42486B; //text
															
		SOM::L.duck   = (FLOAT*)0x45837C; //1.8
		SOM::L.hitbox = (FLOAT*)0x458380; //.25
		SOM::L.height = (FLOAT*)0x4584FC; //1.8		
		SOM::L.shape  = (FLOAT*)0x458500; //.25
		SOM::L.bob    = (FLOAT*)0x45851C;		
		SOM::L.nod    = (FLOAT*)0x458520;
		//Note: this address looks the same 
		//SOM::L.nod    = (FLOAT*)0x458560;
		SOM::L.abyss  = (FLOAT*)0x458528;
		SOM::L.fence  = (FLOAT*)0x458554;				

		SOM::L.on_off = (BYTE*)0x4C0852;
		SOM::L.f3 = &SOM::L.on_off[13];
		SOM::L.f4 = &SOM::L.on_off[14];
		//SOM::L.config2 = (UINT*)0x4C0830; //UNUSED
		SOM::L.config = (UINT*)0x4C0970;
		(BYTE*&)SOM::L.corridor = (BYTE*)0x4C0B98;
		(BYTE*&)SOM::L.entryway = (BYTE*)0x4C0CA0;
		//4c223c //som_scene_transparentelements
		//4C2358 is __thiscall associated
		SOM::L.view_matrix_xyzuvw = (FLOAT*)0X4C2358;	
		SOM::L.frustum = (FLOAT*)0x4C23f0; 
*/		
		//4C28A0 is a 4B flag that if 0 skips fight
		//stuff. 
		//0x4c28a8 is the player's attack 
		//memory passed to som_state_404470
		//4C28B8 has many states:
		/*
		00404239 F6 C1 01             test        cl,1  
		0040423C 74 05                je          00404243  
		0040423E B8 01 00 00 00       mov         eax,1  
		00404243 F6 C1 02             test        cl,2  
		00404246 74 03                je          0040424B  
		00404248 83 C8 02             or          eax,2  
		0040424B F6 C1 04             test        cl,4  
		0040424E 74 03                je          00404253  
		00404250 83 C8 04             or          eax,4  
		00404253 F6 C1 08             test        cl,8  
		00404256 74 03                je          0040425B  
		00404258 83 C8 08             or          eax,8  
		0040425B 8B 4E 0C             mov         ecx,dword ptr [esi+0Ch]  
		0040425E 49                   dec         ecx  
		0040425F 83 F9 03             cmp         ecx,3  
		00404262 0F 87 A5 00 00 00    ja          0040430D 
		*/
		
/*		SOM::L.enemy_pr2_data = (BYTE**)0x4C67C8;
		SOM::L.ai = (SOM::Struct<149>*)(0x4C77C8);
		SOM::L.enemy_prm_file = (BYTE*)0x4DA1C8;		
		
		SOM::L.counters = (WORD*)0x5555C4;		
		SOM::L.leafnums = (BYTE*)0x555E44; 
		SOM::L.controls = (BYTE*)0X5565C8;

		SOM::L.item_prm_file = (BYTE*)0x566ff0; //250
		SOM::L.items = (SOM::Items*)0x57E038;
		SOM::L.item_pr2_file = (BYTE*)0x58043C; //256 (see 40FBB7)

		//2017: Guessing at what this is.
		(DWORD&)SOM::L.mpx = 0x59893C;
		
		//TODO
		//think probably this is 0x19AA978
		//004183F8 68 78 A9 9A 01       push        19AA978h
		SOM::L.menupcs = (DWORD*)0x1A5B4A8; //starting/continue;

		SOM::L.control_ = (BYTE*)0x19AAB74;
		
		SOM::L.magic_prm_file = (BYTE*)0x19AB950;
		SOM::L.magic_pr2_file = (BYTE*)0x19BF1CC;
		
		//19C1A18 is code for PC
		SOM::L.pcstore = (WORD*)0x19C1A18;
		SOM::L.pcequip = (BYTE*)0x19C1C0C;		
		SOM::L.pcstatus = (WORD*)0x19C1C24;
		SOM::L.pcmagic = (DWORD*)0x19C1C40;
		//bad status is at 19C1D4c
		//hp loss deferred 19C1D78 (32bit)
		//bad status deferred 19C1D7C
		SOM::L.pcstate = (FLOAT*)0x19C1D98;
		SOM::L.pcstate2 = SOM::L.pcstate+6;
		SOM::L.pcstep = (BYTE*)0x19C1DC8; 
		SOM::L.pcstepladder = (FLOAT*)0x19C1DCC;		
		SOM::L.dashing = (DWORD*)0x19C1DD8;
		SOM::L.mode2 = (BYTE*)0x19C1DDE;
		SOM::L.save2 = (BYTE*)0x19C1DDF;		

		SOM::L.NPC_prm_file = (BYTE*)0x19C2DE8; 				
		SOM::L.ai2 = (SOM::Struct<43>*)0x1A12DF0;
		SOM::L.obj_pr2_file = (BYTE*)0x1A1B3FC;
		SOM::L.NPC_pr2_data = (BYTE**)0x1A183F0;
		SOM::L.obj_prm_file = (BYTE*)0x1A36400;
		SOM::L.ai3 = (SOM::Struct<46>*)0x1A44400; 
										  		
		SOM::L.startup = (LONG*)0x1A5B5EC;

		//MAP MEMORY (of approximately 2MB)
		//separated by 0x7A20 (31264) bytes
		SOM::L.leafmaps = (BYTE*)0x1A62C68;//~1C4B468

		//TODO: are there 256 or 1024??
		SOM::L.SFX_dat_file = (BYTE*)0x1C91D30; //48B apiece

		SOM::L.SFX_ref_counted_models = (SOM::Struct<20>*)(0x1CDCD38);

		SOM::L.shadows = (BYTE*)0x1CE1CF8; //???

		SOM::L.shops = (SOM::Shops*)0x1CE1D02;

		SOM::L.SND_ref_counts = (WORD*)0x1D11E54; //1024

		SOM::L.lvtable = (DWORD*)0x1D12778;
		
		//SYS.DAT
		SOM::L.mode = (BYTE*)0x1D12B8E;
		SOM::L.walk = (FLOAT*)0x1D12B90;
		SOM::L.dash = (FLOAT*)0x1D12B94;
		SOM::L.turn = (SHORT*)0x1D12B98;
		SOM::L.magic32 = (BYTE*)0x1D12D22;
		SOM::L.save = (BYTE*)0x1D12D62;						

		//+1; som_db.exe uses setup 2 of 2
		SOM::L.setup = (SOM::L::Setup*)0x1D12F80+1;

		//NOTE: 1e9b000 is the end of the module
*/
		break;
	}

	if(SOM::game) //saveface/not db2?
	{
		if(!saveface) 
		EX::is_needed_to_shutdown_immediately(-1,"!saveface");

		//debugging: simple sanity check...
		assert(saveface&&!strcmp(saveface,som_932_MS_Mincho));

		int paranoia = saveface?strnlen(saveface,32):0; 

		if(paranoia>6&&paranoia<LF_FACESIZE) //6 arbitrary
		{	
			SOM::fontface = saveface;

			if(EX::Convert(932,&saveface,&paranoia,&SOM::wideface))
			{
				SOM::wideface = wcsdup(SOM::wideface); //permanent copy
			}
		}
	}	

	const char nop = 0x90; DWORD old = 0;

	DWORD text = 0x401000, text_s = SOM::text->info.RegionSize; 

	//make .text temporarily writable 
	VirtualProtect((void*)text,text_s,PAGE_EXECUTE_READWRITE,&old); //!

	if(1||SOM::game) //make .rdata permanently writable
	{
		DWORD _throwaway; //if(1&&EX::debug) //testing
		VirtualProtect((void*)SOM::rdata->info.BaseAddress,
		SOM::rdata->info.RegionSize,PAGE_READWRITE,&_throwaway);
	}
	if(SOM::tool) //NEW: tools (early out) 
	{
		//REMINDER: There's a blacklist at the top... though
		//it's basically pointless!
		extern void 
		SOM_MAIN_reprogram(),
		SOM_PRM_reprogram(),
		SOM_MAP_reprogram(), MapComp_reprogram(),
		SOM_SYS_reprogram(),
		workshop_reprogram(),som_MDL_reprogram();
		if(SOM::tool==SOM_MAIN.exe) SOM_MAIN_reprogram();
		if(SOM::tool==SOM_PRM.exe) SOM_PRM_reprogram();
		if(SOM::tool==SOM_MAP.exe) SOM_MAP_reprogram();
		if(SOM::tool==MapComp.exe) MapComp_reprogram();
		if(SOM::tool==SOM_SYS.exe) SOM_SYS_reprogram();		
		if(SOM::tool>=PrtsEdit.exe) workshop_reprogram();
		som_MDL_reprogram();
		VirtualProtect((void*)text,text_s,old,&old); //!
		return; //!!
	}
	//todo: som_state_reprogram_SOM_DB_exe
	assert(SOM::game); 	
	enum{ image__db2 = 1 }; //2017
	if(image!='db2') //2017
	return EX::is_needed_to_shutdown_immediately(-1,"!'db2'");

	if(image__db2) 
	{	
		//Allow firing of unacquired magic
		memset((BYTE*)text+0x237F1,nop,2);	
		//Allow gauge to fill up with 0 MP
		memset((BYTE*)text+0x241AF,nop,6);
	}
	else if(image=='rt2')					   
	{
		memset((BYTE*)text+0x229a9,nop,2);
		memset((BYTE*)text+0x23367,nop,6);
	}

	EX::INI::Player pc; EX::INI::Adjust ad;	

	//TODO: these could change dynamically so
	//that they should be reset on the event
	//cycle
	int todolist[SOMEX_VNUMBER<=0x1020602UL];
	SOM::L.shape = pc->player_character_shape; //0.25			
	SOM::L.hitbox = pc->player_character_shape2; //0.25
	SOM::L.hitbox2 = pc->player_character_shape3; //0.25
	SOM::L.bob = pc->player_character_bob; //0.075
	SOM::L.nod[0] = -(SOM::L.nod[1]=pc->player_character_nod); //pi/4
	SOM::L.height = pc->player_character_height; //1.8	
	SOM::L.duck = pc->player_character_height; //1.8		
	SOM::L.fence = pc->player_character_fence+0.01f; //0.5
	//.text 
	//NOTE: technically this should be able to change 
	//on the fly, but it seems like that won't work as
	//long as this .text section variable is to be used
	*(DWORD*)&SOM::L.hold = pc->tap_or_hold_ms_timeout; //750

	SOM::L.abyss = ad->abyss_depth; //-10
	//ALLOW F3 TO RECOVER FROM ABYSS (to avoid reload)
	//(note: this code is zeroing HP, not calling a subroutine)
	//004257D6 66 89 1D 2C 1C 9C 01 mov         word ptr ds:[19C1C2Ch],bx
	*(BYTE*)0x4257D6 = 0xE8; *(DWORD*)0x4257D7 = (DWORD)SOM::die_4257DB-0x4257DB;  
	*(BYTE*)0x4257DB = 0x90; *(BYTE*)0x4257DC = 0x90;
	
	if(image__db2) //44CC20 loops infinitely
	{	
		*(BYTE*)0x44CC20 = 0xE8; //call
		//NOTE: SHOULD BE UNNECESSARY UNDER NORMAL CONDITIONS
		*(DWORD*)0x44CC21 = (DWORD)som_state_clampi-0x44CC25;
		*(BYTE*)0x44CC25 = 0xC3; //ret
	}

	if(op->do_u) //decouple turn axes
	{	
		//TODO: IT TURNS OUT THE x/z PART 
		//IS POINTLESS AND NEEDS TO BE REMOVED

		SOM::u = som_state_uv+0;
		BYTE u[4]; *(DWORD*)u = (DWORD)SOM::u;
		//NEW: does SHORT->FLOAT
		SOM::v = som_state_uv+1; 		
		BYTE v[4]; *(DWORD*)v = (DWORD)SOM::v;
		//REMOVE ME?
		BYTE x[4]; *(DWORD*)x = (DWORD)&SOM::u2[0];
		BYTE z[4]; *(DWORD*)z = (DWORD)&SOM::u2[2];

		//https://defuse.ca/online-x86-assembler.htm

		BYTE x4249D7[] = { 
		//mov ecx, dword ptr [z]
		0x8B,0x0D,z[0],z[1],z[2],z[3], 
		//mov dword ptr[esp+38h], ecx
		0x89,0x4C,0x24,0x38,
		//fld dword ptr [esp+18h]
		0xD9, 0x44, 0x24, 0x18, 		
		//fmul dword ptr [u]
		0xD8, 0x0D, u[0],u[1],u[2],u[3], 
		//fstp dword ptr [esp+18h]
		0xD9, 0x5C, 0x24, 0x18 };

		BYTE x424A47[] = { 
		//mov ecx, dword ptr [x]
		0x8B,0x0D,x[0],x[1],x[2],x[3], 
		//mov dword ptr[esp+30h], ecx
		0x89,0x4C,0x24,0x30,
		//fld dword ptr [esp+18h]
		0xD9, 0x44, 0x24, 0x18, 		
		//fmul dword ptr [v]
		0xD8, 0x0D, v[0],v[1],v[2],v[3], 
		//fstp dword ptr [esp+18h]
		0xD9, 0x5C, 0x24, 0x18 };
				
		if(image__db2)
		{	
			memset((BYTE*)0x4249D7,nop,36);
			memcpy((BYTE*)0x4249D7,x4249D7,sizeof(x4249D7));
			memset((BYTE*)0x424A47,nop,36);
			memcpy((BYTE*)0x424A47,x424A47,sizeof(x424A47));
			//NEW: normalization of xyz vector
			//00424AB3 E8 08 1C 02 00       call        004466C0 
			*(DWORD*)0x424AB4 = (DWORD)som_state_4466C0-0x424AB8;
		}			
		/*else if(image=='rt2'&&!op->do_u2)
		{	
			memset((BYTE*)0x423B8F,nop,36);
			memcpy((BYTE*)0x423B8F,x4249D7,sizeof(x4249D7));
			memset((BYTE*)0x423BFF,nop,36);
			memcpy((BYTE*)0x423BFF,x424A47,sizeof(x424A47));
		}
		else SOM::u = 0;*/	
	}

	//widescreen frustum fix
	//PlayStation VR?
	//if(bf->do_fix_fov_in_memory) if(SOM::L.config) 
	{	
		//fixed depth (this is not used)
		//*(double*)0x458298 = 300 //rdata

		if(image__db2) //PlayStation VR
		{
			//CURIOSITY
			//WTH __thiscall??? som_db almost never does this???
			//00402308 B9 58 23 4C 00       mov         ecx,4C2358h  
			//0040230D E8 FE ED FF FF       call        00401110 

			//TIMING-SENSITIVE
			//403BB0 sets up SOM::L.frustum[2~9]
			//00402312 E8 99 18 00 00       call        00403BB0
			*(DWORD*)0x402313 = (DWORD)som_state_403BB0-0x402317;		
			//REMOVE ME? (2022)
			//also does so? 
			//00402C6F E8 3C 0F 00 00       call        00403BB0
			*(DWORD*)0x402C70 = (DWORD)som_state_403BB0_alt-0x402C74;
		
			//this code keeps tiles from being updated every frame
			/*
			00402322 DF E0                fnstsw      ax  
			00402324 F6 C4 40             test        ah,40h  
			00402327 74 2D                je          00402356  
			00402329 D9 44 24 08          fld         dword ptr [esp+8]  
			0040232D D8 5C 24 14          fcomp       dword ptr [esp+14h]  
			00402331 DF E0                fnstsw      ax  
			00402333 F6 C4 40             test        ah,40h  
			00402336 74 1E                je          00402356  
			00402338 D9 44 24 0C          fld         dword ptr [esp+0Ch]  
			0040233C D8 5C 24 18          fcomp       dword ptr [esp+18h]  
			00402340 DF E0                fnstsw      ax  
			00402342 F6 C4 40             test        ah,40h  
			00402345 74 0F                je          00402356  
			00402347 D9 44 24 38          fld         dword ptr [esp+38h]  
			0040234B D8 5C 24 20          fcomp       dword ptr [esp+20h]  
			0040234F DF E0                fnstsw      ax  
			00402351 F6 C4 40             test        ah,40h  
			00402354 75 05                jne         0040235B 
			//som_scene_unused_4133E0/som_scene_reverse_4133E0
			00402356 E8 85 10 01 00       call        004133E0  
			*/
			//could knock it all out, but why bother?
			memset((void*)0x402354,0x90,2);

			//2022: turns out the display isn't sorted if the
			//the tile elimination feature is disabled
			//00402356 E8 85 10 01 00       call        004133E0
			//00402c74 e8 67 07 01 00       call        004133E0
			extern void __cdecl som_scene_4133E0();
			*(DWORD*)0x402357 = (DWORD)som_scene_4133E0-0x40235b;
			*(DWORD*)0x402c75 = (DWORD)som_scene_4133E0-0x402c79;			
		}

		/*if(image=='rt2')
		{
			//fld dword ptr [004C11FC] : [3F5F66F3]
			som_state_route(0x2A6D,2,(DWORD)&som_state_cone); 
		}*/
		if(image__db2)  
		{					
			//00403C3D D9 05 E4 18 52 6B fld dword ptr [004C23EC]
			som_state_route(0x2C3D,2,(DWORD)&som_state_cone);

			//2017: fix for SOM_DB loading with flash of light
			//The frustrum isn't updated during the flash, and
			//since the zoom function was added, onflip() does
			//the frustrum update
			som_state_cone = 3.14f/2;	
		}		
		SOM::cone = &som_state_cone; 
	}

	//if(SOM::L.on_off)
	{
		bool st = op->do_st;

		//no relation to "always on" events
		static BYTE always_on = 1;

		//The gauge is kept always on to know 
		//when to display the onscreen displays
		//Reminder: we may want to always to this
		//in order to detect the runaway menus bug
		//and to know when to best display any text

		if(image__db2) //4C0852 
		{
			/*2020: switching to compass
			//showGage [sic]
			som_state_dword(0x25667,&SOM::L.on_off[1],&always_on);
			*/
			//00426554 A0 54 08 4C 00       mov         al,byte ptr ds:[004C0854h]
			som_state_dword(0x25555,&SOM::L.on_off[2],&always_on);
				//do_st actually needs this as well
			som_state_dword(0x25667,&SOM::L.on_off[1],&always_on);
			if(st) //showItem (take)
			som_state_dword(0xfcff,&SOM::L.on_off[3],&always_on);
			if(st) //showItem (use/equip)
			som_state_dword(0x17540,&SOM::L.on_off[3],&always_on);
		}
		/*if(image=='rt2') //4BF662
		{
			//showGage [sic]
			som_state_dword(0X24817,&SOM::L.on_off[1],&always_on);
			if(st) //showItem (take)
			som_state_dword(0Xf12f,&SOM::L.on_off[3],&always_on);
			if(st) //showItem (use/equip)
			som_state_dword(0X16970,&SOM::L.on_off[3],&always_on);
		}*/
	}
	
	//event activation (do_fix_boxed_events)
	{		
		/*4256D6 A8 08            test        al,8 
		004256D8 74 13            je          004256ED 
		//items		
		004256DA E8 21 AE FE FF   call        00410500 
		004256DF 84 C0            test        al,al 
		004256E1 75 05            jne         004256E8 
		//props, events
		004256E3 E8 58 2E FE FF   call        00408540		 
		//money
		004256E8 E8 C3 E9 FD FF   call        004040B0
		004256ED*/

		/*408540 //props, events
		//event activation
		00408566 E8 55 07 00 00   call        00408CC0
		0040856E 84 C0            test        al,al 
		00408570 74 34            je          004085A6
		//event filtering? (clip test)
		0040858D E8 3E 06 00 00   call        00408BD0
		00408595 84 C0            test        al,al 
		00408597 74 0D            je          004085A6 
		//event processing
		0040859A E8 41 05 00 00   call        00408AE0
		0040859F 83 C4 04         add         esp,4 
		004085A2 84 C0            test        al,al 
		004085A4 75 1D            jne         004085C3 
		//default: doors, chests, etc...
		004085B6 E8 B5 25 02 00   call        0042AB70 		
		*/
				
		//do_fix_boxed_events...
		//turns out the trouble here is mainly a clerical one

		if(image__db2) //408CC0/42BCA0
		{
			if(bf->do_fix_boxed_events)
			{				
				*(BYTE*)0x408E4A = 0x70; //00408E48 D8 46 68

					//REMOVE ME
					/*2021: I don't know what I was thinking here, it
					//looks like this is unrelated to event activation.
					//I swear I had Ghidra when I added this, maybe not
				//2020
				//disable distance to center test (prevents activation
				//of receptacles from front)
				//note: this utilizes 458574 (1.5) and 458570 which is
				//set by player_character_radius2 below doubled to 0.5 
				//by default
				//0042BE1A 74 13                je          0042BE2F 
				*(WORD*)0x42BE1A = 0x9090;
					//I try to solve this below by enabling the second
					//instance of som_state_42bca0*/
			}			
	
			//00408566 E8 55 07 00 00   call        00408CC0
		//	*(DWORD*)0x408567 = (DWORD)som_state_408CC0-0x40856B; //som.logic.cpp 
			*(DWORD*)0x42AB71 = (DWORD)som_state_42bca0-0x42AB75;
					/*2021: I think 42bca0 is poison... I'm not
					//sure if this will have side effects but I
					//think it routes things through the better
					//event check subroutine
			//2020: this is a second instance? it seems to take a
			//key item id. the other doesn't apply to item pickup
			//propts and neither does this one*/
			//0042ADA1 E8 FA 0E 00 00       call        0042BCA0
			*(DWORD*)0x42ADA2 = (DWORD)som_state_42bca0-0x42ADA6;
		}
		/*else if(image=='rt2') //4080F0/42AE61
		{
			if(bf->do_fix_boxed_events)
			{				
				*(BYTE*)0x40827A = 0x70; //00408278 D8 46 68
			}

			*(DWORD*)0x407997 = (DWORD)som_state_408CC0-0x40799B; //som.logic.cpp
			*(DWORD*)0x429D21 = (DWORD)som_state_42bca0-0x429D25;
		}*/

		//memset((BYTE*)0x408566,nop,5);
		//memset((BYTE*)0x40858D,nop,5);
		//memset((BYTE*)0x40859A,nop,5);
	}
		
	FLOAT *range = 0, *props, *items, *money; 

	if(image__db2)
	{
		range = (FLOAT*)0x458340; //0.25
		som_state_route(0x7EA8,2,(DWORD)(SOM::xyz+0)); 
		som_state_route(0x7EB6,2,(DWORD)(SOM::xyz+2));
		som_state_route(0x7FA1,2,(DWORD)(SOM::xyz+0)); 
		som_state_route(0x7FAF,2,(DWORD)(SOM::xyz+2)); 		 
		//2020: it's possible this is doing nothing
		//more thoughts below
		props = (FLOAT*)0x458570; //0.25
		som_state_route(0x2ADEB,1,(DWORD)(SOM::xyz+2)); 
		som_state_route(0x2ADF1,2,(DWORD)(SOM::xyz+0));
		som_state_route(0x7E0E,2,(DWORD)props); 
		som_state_route(0x7E33,2,(DWORD)props); 
		som_state_route(0x7D23,2,(DWORD)(SOM::xyz+0));
		som_state_route(0x7D36,2,(DWORD)(SOM::xyz+2));
		som_state_route(0x7E3D,2,(DWORD)(SOM::xyz+2)); 
		som_state_route(0x7E59,1,(DWORD)(SOM::xyz+0));
		money = (FLOAT*)0x4582B0; //2		
		som_state_route(0x30E2,2,(DWORD)(SOM::xyz+2)); 
		som_state_route(0x30E9,1,(DWORD)(SOM::xyz+0));	
		items = (FLOAT*)0x458394; //2
		som_state_route(0xF57E,2,(DWORD)(SOM::xyz+2)); 
		som_state_route(0xF585,2,(DWORD)(SOM::xyz+0)); 
	}
	/*else if(image=='rt2')
	{
		range = (FLOAT*)0x457320; //0.25
		som_state_route(0x72D8,2,(DWORD)(SOM::xyz+0)); 
		som_state_route(0x72E6,2,(DWORD)(SOM::xyz+2)); 
		som_state_route(0x73D1,2,(DWORD)(SOM::xyz+0)); 
		som_state_route(0x73DF,2,(DWORD)(SOM::xyz+2)); 
		props = (FLOAT*)0x457550; //0.25
		som_state_route(0x29FAB,1,(DWORD)(SOM::xyz+2)); 
		som_state_route(0x29FB1,2,(DWORD)(SOM::xyz+0));		
		som_state_route(0x723E,2,(DWORD)props); 				
		som_state_route(0x7263,2,(DWORD)props); 
		som_state_route(0x7153,2,(DWORD)(SOM::xyz+0));
		som_state_route(0x7166,2,(DWORD)(SOM::xyz+2));		
		som_state_route(0x726D,2,(DWORD)(SOM::xyz+2)); 
		som_state_route(0x7289,1,(DWORD)(SOM::xyz+0)); 
		money = (FLOAT*)0x457290; //2
		som_state_route(0x2F12,2,(DWORD)(SOM::xyz+2)); 
		som_state_route(0x2F19,1,(DWORD)(SOM::xyz+0)); 
		items = (FLOAT*)0x457374; //2
		som_state_route(0xE9AE,2,(DWORD)(SOM::xyz+2)); 
		som_state_route(0xE9B5,2,(DWORD)(SOM::xyz+0)); 
	}*/

	if(range)
	{
		*range = pc->player_character_radius-1.5f;
		//44ce30 does event test (return to this)
		int todolist[SOMEX_VNUMBER<=0x1020602UL];
		//2020: it's possible this is doing nothing
		//since I changed do_fix_boxed_events after
		//learning 42bca0 tests from  the center of
		//the object (an edge test may be required)
		//NOTE: I think events perform an edge test		
		*props = pc->player_character_radius2-1.5f;

		*items = *money = pc->player_character_radius2; 

		if(pc->player_character_radius3) //items
		{
			*items = *money = pc->player_character_radius3;
		}
		if(pc->player_character_radius4) //money
		{
			*money = pc->player_character_radius4;
		}
	}
		
	if(image__db2)
	if(SOMEX_VNUMBER>=0x010101FF)//1.1.2.1 demonstration
	{	
		/*Is this the damage formula?
		00404352 8D 4C 24 0F      lea         ecx,[esp+0Fh] 
		00404356 51               push        ecx  
		00404357 8D 54 24 14      lea         edx,[esp+14h] 
		0040435B 52               push        edx  
		0040435C 8D 46 F0         lea         eax,[esi-10h] 
		0040435F 68 14 1D 9C 01   push        19C1D14h 
		00404364 50               push        eax  
		00404365 E8 06 01 00 00   call        00404470 
		*/
		//00404365 E8 06 01 00 00   call        00404470 
		*(DWORD*)0x404366 = (DWORD)som_state_404470-0x40436A;
		//004043AD E8 06 01 00 00   call        00404470 
		*(DWORD*)0x4043AE = (DWORD)som_state_404470-0x4043B2;
		//004043FA E8 06 01 00 00   call        00404470 
		*(DWORD*)0x4043FB = (DWORD)som_state_404470-0x4043FF;		
	}

	if(image__db2) //zentai / defeat (2018: F3 policy change)
	{

		//2018: trying to change how F3 works
		//NOTE: 00408A80 may be the way events are processed
		//but here it is in the SYSTEM's death event context
		//004258C0 E8 BB 31 FE FF       call        00408A80
		*(DWORD*)0x4258C1 = (DWORD)som_state_00408A80_RIP-0x4258C5;
	


		//REMINDER: 3 DWORDs could be the map, clock, and ?

		/*
		ON/OFF:
		//4083B0 (called at +97b startup battery and jmp'd to at +73f5)
		//401000+73b0 (same as below but also the 3 555-dwords. Returns 01)
		DEATH:
		//408400 (called at +248db)
		//401000+7400 (subroutine reset counters, leafnums, and 128 bytes at 555DC4)
		OPEN(2):
		//+74cb resets leafnums and 555dc4 (128Bs)
		Redundant
		OPEN(1)/CLOSE:
		//-+7502 resets leafnums and 3 555-dwords to 0
		*/

		/*moving to som_game_reprogram
		//004258DB E8 20 2B FE FF   call        00408400
		*(DWORD*)0x4258DC = (DWORD)som_state_408400-0x4258E0;*/

		//Replacing 4084CD through 4084D8 with custom call
		/*
		004084CB 33 C0            xor         eax,eax 
		004084CD B9 00 01 00 00   mov         ecx,100h 
		004084D2 BF 44 5E 55 00   mov         edi,555E44h 
		004084D7 F3 AB            rep stos    dword ptr es:[edi] 
		004084D9 B9 20 00 00 00   mov         ecx,20h 
		004084DE BF C4 5D 55 00   mov         edi,555DC4h 
		004084E3 F3 AB            rep stos    dword ptr es:[edi] 
		004084E5 5F               pop         edi  
		004084E6 B0 01            mov         al,1 
		004084E8 5E               pop         esi  
		004084E9 81 C4 04 01 00 00 add         esp,104h 
		004084EF C3               ret           
		*/
		*(BYTE*)0x4084CD = 0xE8; //call
		*(DWORD*)0x4084CE = (DWORD)som_state_4084cd-0x4084D2;
		memset((VOID*)0x4084D2,0x90,7); //NOP
	}
	 	
	if(image__db2) //lore/magic description
	{
		//0043FE44 E8 27 00 00 00   call        0043FE70
		*(DWORD*)0x43FE45 = (DWORD)som_state_43FE70-0x43FE49; //enemy
		//0043FE03 E8 68 00 00 00   call        0043FE70 
		*(DWORD*)0x43FE04 = (DWORD)som_state_43FE70-0x43FE08; //NPC
		//0043EE18 E8 B3 3D FE FF   call        00422BD0
		*(DWORD*)0x43EE19 = (DWORD)som_state_422BD0-0x43EE1D; //Item
		//00419E66 E8 65 8D 00 00   call        00422BD0
		*(DWORD*)0x419E67 = (DWORD)som_state_422BD0-0x419E6B; //Magic
		//0041BF20 E8 AB 6C 00 00   call        00422BD0
		*(DWORD*)0x41BF21 = (DWORD)som_state_422BD0-0x41BF25; //Equip: Magic
	}
	if(image__db2) //presentations
	{
		//opening slides (45a11c is opening.dat)
		//NOTE: this is independent of SOM_SYS's commandline arguments
		//00401A79 E8 72 37 00 00       call        004051F0 		
		*(DWORD*)0x401A7A = (DWORD)som_state_4051F0_intro_2022-0x401A7E;
		//game over (1,2 &3)	(45a380, 45a38c & 45a398)
		//00402DF5 E8 F6 23 00 00       call        004051F0 
		*(DWORD*)0x402DF6 = (DWORD)som_state_4051F0_intro-0x402DFA;
		//ending credits (immediately below game over) (45a374)
		//00402E16 E8 D5 23 00 00       call        004051F0
		*(DWORD*)0x402E17 = (DWORD)som_state_4051F0_intro-0x402E1B;
		//SOM_SYS test
		//00403999 E8 52 18 00 00       call        004051F0
		*(DWORD*)0x40399a = (DWORD)som_state_4051F0_intro-0x40399e;
		//function pointer in Startup table (45e57c is title.dat)
		//0042C259 E8 92 8F FD FF       call        004051F0
		*(DWORD*)0x42C25a = (DWORD)som_state_4051F0_intro-0x42C25e;
		//00405221 E8 3A A7 04 00       call        0044F960
		*(DWORD*)0x405222 = (DWORD)som_state_4051F0_strcmp-0x405226;
		
		//00405727 E8 84 D3 01 00   call        00422AB0
		*(DWORD*)0x405728 = (DWORD)som_state_slides422AB0-0x40572C;

	}
	/*Calculate jump into Event instruction table
	00408B82 8B 44 24 10      mov         eax,dword ptr [esp+10h] 
	00408B86 33 D2            xor         edx,edx 
	00408B88 66 8B 16         mov         dx,word ptr [esi] 
	00408B8B 8B 0C 95 00 A7 45 00 mov         ecx,dword ptr [edx*4+45A700h] 
	00408B92 85 C9            test        ecx,ecx 
	00408B94 74 11            je          00408BA7 
	00408B96 8D 44 24 10      lea         eax,[esp+10h] 
	00408B9A 50               push        eax  
	00408B9B 53               push        ebx  
	00408B9C FF D1            call        ecx  
	00408B9E 8B 44 24 18      mov         eax,dword ptr [esp+18h] 
	00408BA2 83 C4 08         add         esp,8 
	00408BA5 EB 0C            jmp         00408BB3 
	00408BA7 33 C9            xor         ecx,ecx 
	00408BA9 66 8B 4E 02      mov         cx,word ptr [esi+2] 
	00408BAD 03 C1            add         eax,ecx 
	00408BAF 89 44 24 10      mov         dword ptr [esp+10h],eax 
	00408BB3 8B F0            mov         esi,eax */
	//THESE ADDRESSES ARE SIDE BY SIDE^v
	//save/take menus+always on event fix
	if(bf->do_fix_asynchronous_input)
	{
		//REMINDER: shop exit is still sticky 
		//EXTENSIONS MAY RELY ON THIS WORKING
		//(which is really kind of a problem)

		if(image__db2) //40f26d/16d
		{
			//yikes: always on events calling
			//the input processing subroutine
			//(just tacking this one on here)
			memset((BYTE*)0x408BB5,nop,5);

			//zero SOM's input bitmask for it
			//unsure 0x40f16d is necessary???
			//reminder: tried with browsing_shop
			//memset((BYTE*)0x40f16d,nop,10);
			memset((BYTE*)0x40f26d,nop,10); 			

			//seems likely related to the changes above
			//but can't test because the event misfires
			//if do_fix_asynchronous_input is not on???
			//NEW: runaway Sell more-than-1 beep bug???
			//0043E5C2 50               push        eax  
			//0043E5C3 E8 B8 4E FE FF   call        00423480
			*(DWORD*)0x43E5C4 = (DWORD)som_state_sellout423480-0x43E5C8;
		}
		else if(image=='rt2') //40e69d/59d
		{	
			//yikes: always on events calling
			//the input processing subroutine
			//(just tacking this one on here)
			memset((BYTE*)0x407fe5,nop,5);

			//zero SOM's input bitmask for it
			//unsure 0x40e59d is necessary???
			//memset((BYTE*)0x40e59d,nop,10); 
			memset((BYTE*)0x40e69d,nop,10); 
		}
	}
	if(image__db2) //event jump table
	{
		//00408B8B 8B 0C 95 00 A7 45 00 mov         ecx,dword ptr [edx*4+45A700h] 		
		//MSVC2010: error C2101: '&' on constant
		//DWORD(&f)[160] = *(DWORD(*)[160])0x45A700;
		auto fp = (DWORD(*)[160])0x45A700;
		auto &f = *fp;
		f[0x00] = (DWORD)som_state_409360; //Text
		f[0x01] = (DWORD)som_state_409390; //Text+Font 
		extern void __cdecl som_game_409440(DWORD,DWORD*);
		f[0x17] = (DWORD)som_game_409440; //Shop
		f[0x19] = (DWORD)som_state_4094f0; //Warp NPC (layer)
		f[0x1a] = (DWORD)som_state_4094f0; //Warp Enemy (409570) (layer)
		extern void som_MPX_corridor(DWORD,DWORD*);
		f[0x3b] = (DWORD)som_MPX_corridor; //Swap Map //EXTENSION
		f[0x3c] = (DWORD)som_state_409af0; //Map (layer)
		f[0x3d] = (DWORD)som_state_409af0; //Warp (409be0) (layer)
		f[0x50] = (DWORD)som_state_409d50; //Status 		
		f[0x54] = (DWORD)som_state_40a330; //Counter+Status
		f[0x64] = (DWORD)som_state_40a3e0; //Engage Object (2022)
		f[0x66] = (DWORD)som_state_40a480; //Move/Warp Object (layer)
		f[0x78] = (DWORD)som_state_40a5a0; //System
		f[0x79] = (DWORD)som_state_40a5f0; //Saving
		f[0x8D] = (DWORD)som_state_40a8c0; //Text+Menu
		f[0x90] = (DWORD)som_state_40adc0; //Counter
	}
		
	if(bf->do_fix_automap_graphic) if(image__db2)
	{			
		//automap44F938 sets origin at 0,0
		//(44F938 seems to convert a double to an integer)
		//0041936F E8 C4 65 03 00   call        0044F938
		*(DWORD*)0x419370 = (DWORD)som_state_automap44F938-0x419374;
		//00419394 E8 9F 65 03 00   call        0044F938 
		*(DWORD*)0x419395 = (DWORD)som_state_automap44F938-0x419399;
		//004194CD E8 7E 9F 00 00   call        00423450
		//(423450 is used to get the dimensions as floats)
		*(DWORD*)0x4194CE = (DWORD)som_state_automap423450-0x4194D2;
		//004194E6 E8 65 9F 00 00   call        00423450
		*(DWORD*)0x4194E7 = (DWORD)som_state_automap423450-0x4194EB;
		//00419524 E8 07 F4 02 00   call        00448930
		//(448930 seems to be a general purpose screen blt)
		*(DWORD*)0x419525 = (DWORD)som_state_automap448930-0x419529;

		//regular picture map
		//00419586 E8 65 F2 02 00   call        004487F0
		*(DWORD*)0x419587 = (DWORD)som_state_automap4487F0-0x41958B;
	}
	
	if(image__db2)
	{
		//start sequence. returns save game or FF?
		//00401A51 E8 6A A5 02 00       call        0042BFC0 
		//00403715 E8 a6 88 02 00       call        0042BFC0 //SOM_SYS
		//pre-game texture initialization sequence?
		//0042BFDD E8 EE 06 00 00       call        0042C6D0
		if(dt->start_mode==1&&op->do_start)
		{
			//2022: just tossing this in here... can't start
			//loading the map until the textures are cleared
			*(DWORD*)0x42BFDe = (DWORD)som_state_42C6D0_start_1-0x42BFe2;		
		}		
		else *(DWORD*)0x42BFDe = (DWORD)som_state_42C6D0-0x42BFe2; //2022
		//00405257 E8 54 01 00 00       call        004053B0
		*(DWORD*)0x405258 = (DWORD)som_state_4053b0-0x40525c; //2022		

		//BeginScene
		//backs up RenderStates then calls SetTransform?
		//0042C098 E8 13 10 02 00       call        0044D0B0
		//draws start screens, returns -1 once entered field
		//1A5B5ECh is 0,1,2,3,9 for jumping into 45E554h 
		//2 is "press start" and 3 is the menu, 9 is continue
		//(the entire table is padded like 0,1,2,3,9,9,3,9,9,9,???)
		//0042C09D 8B 15 EC B5 A5 01    mov         edx,dword ptr ds:[1A5B5ECh]  
		//0042C0A3 68 00 B4 A5 01       push        1A5B400h
		//0042C0A8 FF 14 95 54 E5 45 00 call        dword ptr [edx*4+45E554h]
		//NOTE: these subroutines return the new 1A5B5ECh code here
		//if(wn->do_hide_splash_screen_on_startup)
		if(wn->do_hide_company_title_on_startup) 
		{
			SOM::L.startup = 1; memset((BYTE*)text+0x2b6e1,0x90,6);		

			//DEBUGGING: NEED TO SKIP THE FADE IN/OUT SOMEHOW
			if(wn->do_hide_opening_movie_on_startup) SOM::L.startup = 2;
		}
	}	

	//This bug tries to use a PRM no. to look up a PR2 record and equips
	//the 0th magic if it's not an equip-friendly magic (which of course
	//the 0th magic may not be either!) and so since it's being paranoid
	//the 0th magic assignment is NOP'ed out.
	if(image__db2)
	{	
		/*
		0042CC6B A0 13 1C 9C 01       mov         al,byte ptr ds:[019C1C13h]  
		0042CC70 3C 20                cmp         al,20h  
		0042CC72 73 41                jae         0042CCB5  
		0042CC74 33 C9                xor         ecx,ecx  
		0042CC76 8A C8                mov         cl,al  
		0042CC78 8A 81 22 2D D1 01    mov         al,byte ptr [ecx+1D12D22h]  
		0042CC7E 3C FA                cmp         al,0FAh  
		0042CC80 88 44 24 14          mov         byte ptr [esp+14h],al  
		0042CC84 73 28                jae         0042CCAE  
		0042CC86 8B 44 24 14          mov         eax,dword ptr [esp+14h]  
		0042CC8A 25 FF 00 00 00       and         eax,0FFh  
		0042CC8F 8D 14 80             lea         edx,[eax+eax*4]  
		0042CC92 C1 E2 06             shl         edx,6  
		//See, EAX should store this WORD.
		0042CC95 66 81 BA 50 B9 9A 01 FF FF cmp         word ptr [edx+19AB950h],0FFFFh  
		0042CC9E 74 0E                je          0042CCAE  
		//EAX has the wrong value here. It's not worth reprogramming this ASM.
		0042CCA0 8D 04 80             lea         eax,[eax+eax*4]  
		0042CCA3 8A 0C C5 EF F1 9B 01 mov         cl,byte ptr [eax*8+19BF1EFh]  
		0042CCAA 84 C9                test        cl,cl  
		0042CCAC 74 07                je          0042CCB5  
		0042CCAE C6 05 13 1C 9C 01 00 mov         byte ptr ds:[19C1C13h],0
		*/
		memset((void*)0x42CCAE,0x90,7);
	}

	//TODO: HANDLE MISSING CP FILE GRACEFULLY
	//
	/*if(image__db2) if(EX::debug) //!!!
	{
		//NOTE: this is called in real-time (it's CP lookup code)

		//004418D6 8A 4D 54             mov         cl,byte ptr [ebp+54h]  
		//004418D9 84 C9                test        cl,cl
		memcpy((void*)0x4418D6,"\xb1\x01\x90",3); //mov cl,1 //nop
	}*/

	if(image__db2) //2017: menu table?
	{
		//som.game.cpp was a first attempt to move
		//some game code out of som.state.cpp, and
		//to stop adding new code to it. then came
		//further specialization. som.logic.cpp is
		//new in later 2020 and maybe it's an idea
		//to try to move the rest of som.state.cpp
		//into it
		extern void 
		som_game_reprogram();som_game_reprogram();	
		extern void 
		som_scene_reprogram(),som_MHM_reprogram(),
		som_logic_reprogram(),som_MDL_reprogram(),som_MPX_reprogram(),
		som_clipc_reprogram();
		som_scene_reprogram();som_MHM_reprogram();
		som_logic_reprogram();som_MDL_reprogram();som_MPX_reprogram();
		som_clipc_reprogram();
	}

/////////////////////////////////////////////////////////////////////////

	VirtualProtect((void*)text,text_s,old,&old);

	//2022: something I found (probably don't matter)
	FlushInstructionCache(GetCurrentProcess(),(VOID*)text,text_s);
						
  //*disassemble last so to incorporate changes*//
	
	if(SOM::game) //todo: tool disassembler?
	{
		if(op->do_disasm)
		if(EX::INI::Output()->f(11)) 
		SOM::disasm = EX::program(SOM::text);
		if(SOM::disasm)	SOM::disasm->scratchmem();		
		//hack: display F11 .text output as asm
		EX::code_view_of_memory(SOM::disasm); 
	}

  //////////////////////////////////////////////
}
