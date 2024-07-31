#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

#include "Ex.ini.h"
#include "Ex.input.h"
#include "Ex.output.h"

#include "SomEx.h"
#include "som.state.h"
#include "som.status.h" //PSVR
#include "som.extra.h"
#include "som.files.h" //ITEM.ARM
#include "som.game.h" //SOM::Game //REMOVE ME?

#define SOMVECTOR_MATH
#include "../Somplayer/Somvector.h"

namespace DDRAW
{
	extern DWORD refreshrate;
	extern float fxCounterMotion; 
	extern bool fx2ndSceneBuffer;
	extern void dejagrate_update_psPresentState();

	extern BOOL xr;
	extern bool inStereo;
	extern bool stereo_locate_OpenXR(float,float,float[4+3],float[4]=0);
	extern float(*stereo_locate_OpenXR_mats)[4][4];
}
namespace DSOUND
{
	//REMOVE ME?
	extern void playing_delays(); 
	class IDirectSoundBuffer;
	extern void fade_delay(IDirectSoundBuffer*,int,int); 
}

//HACK: make the C++ type quaternion
extern float Ex_output_f6_head[10-3-3];

extern bool som_clipc_slipping2; //HACK
extern void som_title_further_delay_sub(int);
extern DWORD som_MDL_449d20_swing_mask(bool);

//2020: this is to try to make the squat transition look
//similar to dashing (probably depends on tap_vs_hold)
static const float som_mocap_squat = 0.63f;

//UNDERCOOKED: this is just a silly idea to represent
//the "fast walk" status as if hunched over like in a
//pensive stance
enum{ som_mocap_fastwalker=0&&EX::debug };

//HISTORY LESSON
//
// everything in this file was once 
// spread out everywhere, no OOP at
// all. so it was all moved here in
// one place and refactored so that
// it's possible to reset and so on

//singleton
//MOVED here from som.hacks.cpp
static struct som_mocap 
{	
	static som_mocap &mc; //shorthand

	bool interrupted, landing_2017, suspended;

	float recovering, recovery; //2021

	bool scaling_bouncing;

	/*2020: renaming/formalizing
	float slow; //todo: incorporate slow status*/
	float squat;

	//as seen on the F5 overlay
	float error, speed, speed2;

	float analogbob, touchdown_scorecard; 

	//locomotion
	float landspeed, crabwalk, goround, swivel; 
	float turnspeed, backpedal, bounce, buckle; //UNUSED?			
	//actions
	float dashing, ducking, peaking;
	float jogging, running, fleeing, fleeing2, sliding; 
	float resting, jumping, landing;	
	float leaning, leading, scaling, scaling2;	
	float leading2;
	float squatting; //run->duck->squat?
	//heights
	float falling, ceiling;
	//gesture							   
	float turning; //(fast)		
			 							
	//buttons
	int action,active;
	bool event_let_go;
	int attack,attack2;
	int attack3,magic3,sword_magic3;	
	bool attacked,attacked2,hold_cancel;
	bool hud_cancel;
	unsigned hud_defer,hud_defer2,hud_event;
	
	//squat walking
	bool upsadaisy;	
	bool tunnel_running;
	float inverter,reverter;
	float inverting,reverting;
	float leadingaway;
	float lookahead[2];		
	float pushahead[2];
	float auto_squatting;
	unsigned stealth_squatting;
	unsigned gaits_max;
	unsigned gaits_max2;
	unsigned gaits_max_residual;
	unsigned gaits_max_unboosted;

	//EXPERIMENTAL
	//climbing/entering crawlspace
	bool surmounting;
	//bool scan_for_secret_doors;
	bool surmounting_pullup;
	int surmounting_staging;
		
	//WORK IN PROGRESS
	bool holding;
	float holding2;
	float holding_xyzt[4];
	float holding_pcstate[3];
	float cornering;

	//EXPERIMENTAL
	float bending,bent;
	float bending_soft[3];
	float bending_resist;

	bool arm_clear; //2021

	float nosedive,fastwalk;

	bool hopping;
	unsigned neutral;
	unsigned crouched;
	unsigned lept;
	unsigned temporary_f3;

	float backwards,sideways;

	int gauge_bufs[3] = {};

	typedef struct //engine
	{
		bool rest,push;
		void init()
		{
			Prolog.init();
			Analog.init();
			Epilog.init();
		}
		//entry point
		void operator()(float,BYTE*);		
		static int hold(int &io, bool cmp, int ms=SOM::motions.diff)
		{
			int o = io; if(cmp) io+=ms; else io = 0; return o;
		}
		//subroutines
		struct prolog 
		{
			bool jumping, fallen;
			float diving, freefall,air,airy;
			float falling; //0.1f
			float speeding, alighting, astride;
			void init()
			{
				falling = 0.1f;
			}
			void operator()(float);
		};
		struct analog
		{
			float w, leading, holding;
			int center;	//bool skipping; //UNUSED
			float uturn; bool stopped;
			float windup, phase;			
			float braking, braking_z;
			float shoveoff, horsepower;
			float drag;
			void init()
			{
				w = SOM::uvw[1]; //!
				leading = SOM::err[3]; //!!
				Suspend.init(); 
			}
			void operator()(float,BYTE*);
			
			struct suspend //helper
			{
				float suspended;
				bool resting, bouncing, jumping;
				float xx, zz, touchdown;
				void init(){}
				float operator()(BYTE*,float);

			}Suspend;
		};
		struct epilog
		{
			enum{ waves_s = 4 };
			float waves[waves_s][3];
			void init(){}
			void operator()(float);
		};		

		//subroutines
		prolog Prolog;
		analog Analog;
		epilog Epilog;

	}engine; //typedef

	//these are not stateful, just sharing results
	float antijitter;

	int trigger;

	engine Engine; //singleton

	typedef struct //jumpstart *NEW*
	{
		enum{ start=33*7 };

		bool canceled, squat_walk, squat_jog, in_air;

		char maybe_jumping; //2020

		int started,hop; struct detector
		{
			int g,x,y,dx,dy, residual,ry;

			void operator()(int ms, int gg)
			{	
		//2017: I think there's a bug here, that accidentally jumps, but
		//I see now why -10 is retained to sustain movement upon release
		//		if(gg==-10) gg = 0;

				//I think shortening this may help maybe_jumping to have
				//fewer false positives?
				//
				//longer timeouts seem to produce fewer hiccuping jumps
				//the "residual" should be short as long as input works
				//enum{ to1=33*3, to2=33 }; //100
				//I think this may help hiccups with triggers
				//enum{ to1=33*4, to2=33 }; //100
				enum{ to1=33*3, to2=33 }; //100

				//!=0: account for 1/10 "half-gaits"
				x+=ms; std::swap(g,gg); if(g>gg&&g!=0)
				{								  
					dx = x; dy = g-gg; x = 0; y = g; //g>gg 

					if(dy>10) dy = g-0; //hack: fudge 1/10 gait
				}
				else if(g==-10&&gg==0) //special case: 1/10 gait
				{
					 dx = x; dy = 0; x = 0; y = g; //same as above
				}
				else if(x>to1){ x = dx = dy = 0; y = g; } //timeout

				//NOTE: this was originally added for keyboard input
				//but it seems to be propping up XInput triggers too
				if(y>0){ residual = to2; ry = y; }else residual-=ms; 
			}
			void operator+=(int ms) //smoothing
			{
				assert(y!=-10||dy==0);
				for(x+=ms;x>=dx&&dx;x-=dx) y+=dy; if(y>7) y = 7;
			}

		}dpad[4+4]; void operator()(BYTE *kb)
		{	
			int ms = SOM::motions.diff;
			assert(started||!canceled); 
			
			//float x = EX::Joypads[0].position[0];
			//float y = EX::Joypads[0].position[1];
			//EX::dbgmsg("Jumpstart: %+f %+f",x,y);

			const int bts[8] = //NEW: last 4 do not initiate jumps
			{0x4B,0x4D,0x4C,0x50, 0x4F,0x51,0x47,0x49}; if(started) 
			{				
				//this doesn't work here for some reason? glitches
				//kb[0xF3] = 0x80; //DASH

				//REMINDER: operator+= makes y here keep going up after
				//started
				int dom = 0; 
				for(int i=0;i<4;i++) if(dpad[i].y>dom) dom = dpad[i].y;
								
				//2020: try to keep receiving input from the stick until
				//let go, assuming people let go of both sticks together
				//(I can't NOT let go)
				bool holding = false; 
				if(0)
				if(started>=start-99&&!maybe_jumping)
				for(int i=0;i<4;i++)				
				if(dpad[i].y==EX::Joypad::gaitcode(kb[bts[i]]))
				{
					holding = true;					
					for(int i=0;i<4;i++) if(dpad[i].y>dom) dom = dpad[i].y;
					for(int i=0;i<8;i++)
					dpad[i](ms,EX::Joypad::gaitcode(kb[bts[i]])); break;
				}

				//EXPERIMENTAL
				//the goal of this logic is to differentiate between
				//manually crossing over, versus releasing the stick
				//
				// Unfortunately IT BARELY WORKS depending on sticks
				// because the DS4 backfires up to 33% when it's let
				// go even though it's invisible in the stick itself
				// unfortunately it seems designed to look identical
				// to manually moving the stick... the springs can't
				// be distinguished from purposeful inputs and don't
				// backstop themselves to prevent crossover backfire
				//
				// to deal with this som_state_thumbs_boost is tuned
				// and EX::Joypad::analog_dilation tries to hide the
				// backfire at the expense of diminishing the effect
				//				
				if(!hop) //if(maybe_jumping)
				{
					if(mc.Engine.Prolog.air>0) in_air = true;

					const int cmp[4] = {0x4D,0x4B,0x50,0x4C};
					//2020: too dangerous... assume jumping?
					if(in_air)
					{
						if(maybe_jumping=='n') maybe_jumping = 0;
					}
					else for(int i=4;i-->0;) if(kb[cmp[i]]&&dom==dpad[i].y)
					{
						int g = EX::Joypad::gaitcode(kb[cmp[i]]);
						if(g<0) continue; //-10?

						//EXTENSION
						//HACK: my DS4 controller seems to bounce into
						//the other side when centering so either the
						//dead zone must be extended or a configurable
						//tolerance is called for
						//
						// MAYBE analog_smoothing CAN SMOOTH THIS OUT?
						//
				// having far fewer incidents for the moment???
						//
				//		if(g>=2||dom>=7)
						{
							maybe_jumping = 'n'; //operator=(0); //cancel

							EX::Joypad &bp = EX::Joypads[0]; //breakpoint

							if(EX::debug) 
							{
								MessageBeep(MB_ICONERROR); //MessageBeep(-1);
							}
						}
					}
				}

				for(int i=0;i<8;i++) 
				{
					if(!hop)
					if(!maybe_jumping)
					if(i<4&&started>=66) //33+ms
					{
						if(dom==dpad[i].y)
						if(dom-EX::Joypad::gaitcode(kb[bts[i]])>=dom/2+1)
						if(!canceled&&!squat_jog)
						{
							if(1)
							{
								maybe_jumping = 'y'; //true 

								started = 33+ms; 
							}
							else mc.jumping = 0.000001f; 

							break;
						}
					}
					if(!holding) //sustain movement upon release?
					{
						dpad[i]+=ms; kb[bts[i]] =
						EX::Joypad::analog_dither(EX::Joypad::bytecode(dpad[i].y));
					}
				}								
				if(!mc.jumping)
				{ 	
					if(hop)
					{
						int diff = min(ms,hop);
						
						hop-=diff; ms-=diff;
					}
					started = max(started-ms,0); 
					
					if(!started) 
					{
						if(!canceled) if(maybe_jumping!='y') 
						{
							if(maybe_jumping!='n')
							{
								if(mc.resting) //squat walking
								{
									mc.resting = 0;
							
									if(squat_walk) mc.inverter = 1; 
								}					
								//hack: cancel old style jump (obsolete)
								//note: this is smoothing out inversion somehow
								mc.action = 0; 

								squat_jog = squat_walk = false; //NEW
							}
						}
						else
						{
							mc.jumping = 0.000001f; 

							//move to Prolog?
							SOM::eventick = 0; //cancel hopping subtitle (2020)
						}
					}
					else if(!hop) kb[0xF3] = 0x80; //DASH	
				}
				else started = 0; 
				
				if(!started) canceled = false;

				if(EX::INI::Player()->do_not_jump) mc.jumping = 0;
			
				//why? seems to make operator= think it's standing jumping
				//when double-tapping
				//if(!started) memset(dpad,0x00,sizeof(dpad));	
				
				if(maybe_jumping||in_air) //clawback maybe_jumping sensitivity?
				EX::Joypad::analog_dilation = sqrt(EX::Joypad::analog_dilation);
			}
			else for(int i=0;i<8;i++) dpad[i](ms,EX::Joypad::gaitcode(kb[bts[i]]));

			//2020
			//HACK: eliminate antagonistic inputs... as near as I can
			//tell this isn't being done anywhere... the jump system
			//should be a source of them
			if(1) for(int i=0;i<=8-2;i+=2) if(kb[bts[i]]&&kb[bts[i+1]]) 
			{
				//seems bumpier but jump direction seems more accurate
				int g1 = EX::Joypad::gaitcode(kb[bts[i]]);
				int g2 = EX::Joypad::gaitcode(kb[bts[i+1]]);
				if(g1!=g2) kb[bts[g1<g2?i:i+1]] = 0;
				else kb[bts[i]] = kb[bts[i+1]] = 0;
			}

			assert(started||!canceled);

			if(!started) //don't leave resetting globals to chance
			{
				//TESTING BACKFIRE STRATEGY
				EX::Joypad::analog_dilation = 1;

				maybe_jumping = false; //2021
			}
		}		
		void operator=(BYTE *kb) //starting
		{	
			if(canceled) return; if(!kb) //2020
			{
				if(canceled=started)
				squat_jog = squat_walk = false; hop = 0; return;
			}

			if(mc.scaling||mc.auto_squatting>0.8f) return; //2020

			//2020: this disables intelligent jump prevention logic to 
			//ensure jump occur. it can kick in later as well after it
			//starts. it also applies if not grounded since there's no
			//imperative to eliminate jumps in that case
			in_air = mc.Engine.Prolog.air>0||SOM::motions.tick-mc.lept<=500;
			//it's really frustrating when a crouch jump fails and the
			//imperative to prevent false positives isn't really there
			if(mc.ducking>1) in_air = true;

			EX::INI::Player pc;

			for(int i=0;i<4;i++) if(dpad[i].residual>0) //y
			{
				//REMINDER: the running->crouch/squat-walk transition
				//needs this window to be higher than it really needs
				//to be for jumping, but it seems necessary since the
				//running is propelled forward by this system without
				//stopping
				//started = 33*6;
				//I still get the occassional jerky jump even after a
				//lot of things have been improved upon... using 7 is
				//so far so good but I don't know if it's too long or
				//if it's the right fix... my thinking is input isn't
				//being sustained, but if so why do the jumps happen?
				//note, other sources could be walking-effect or just
				//Windows's inconsistent frame rate
				//this is about 14 frames at 60 fps
				started = start; //33*7;
								
				if(!mc.hopping)
				{
					kb[0xF3] = 0x80; 				
					squat_walk = pc->do_not_squat?false:!mc.holding_xyzt[3];
					squat_jog = pc->do_not_squat?false:1==mc.jogging;
				}
				else hop = 16*3;

				int dom = 0;
				for(int i=0;i<4;i++) 
				{
					//NOTE: "digital input" is really for keyboards but it
					//isn't really possible to differentiate between using
					//a space-bar and a more responsive button and it also
					//now includes XInput triggers since they're extremely
					//unresponsive
					//if(!hop)
					if(dpad[i].residual>0) 
					if(!dpad[i].y) 
					dpad[i].y = dpad[i].ry; //digital input

					dom = max(dom,dpad[i].y);
				}
				//EXTENSION
				//
				// 0.025 is too low for DS4
				// this needs to be as low as possible without preventing
				// jumps due to "backfire" that is when you let go of the
				// stick it crosses onto the other side
				//
				// the DS4 has 33% backfire, which seems very high but it
				// might be other controllers have more, but for now this
				// seems already very conservative
				//
				//TESTING BACKFIRE STRATEGY 
				//EX::Joypad::analog_dilation = 1-max(0,dom-2)*0.06f; //0.05~0.075
				if(!in_air) EX::Joypad::analog_dilation = 1-dom*0.04f;
				
				if(0) //2020: helpful? harmful?
				{
					//"dom" above means that the dominant pad should 
					//prevail, so this doesn't matter in theory, but
					//operator+= can change which pad is dominant in
					//theory if the other's derivative is higher, in
					//which case I'm not sure what is better, but if
					//there is confusion maybe not jumping is safest

					int x = dpad[0].y>dpad[2].y?2:0;
					int y = dpad[1].y>dpad[3].y?3:1;
					memset(dpad+x,0x00,sizeof(*dpad));
					memset(dpad+y,0x00,sizeof(*dpad));
				}
				maybe_jumping = false; return;
			}
			//2020: allow upsadaisy to 
			//be set when not crouching
			//if(mc.ducking<=1) return; //standing
			if(mc.inverting) if(mc.inverting>=0.8f) return; //getting up
			else if(mc.upsadaisy=mc.inverting>0.25f) return; //slow unwinding
			if(mc.ducking<=1) return; //standing

			if(!pc->do_not_jump) //standing jump?
			{
				//2020: this can be annoying
				if(mc.ducking==2)
				{
					//TODO: EX::Joypad::trigger_fast_release(mc.trigger-1)?
					//(anywhere even if resting is full)
					//works but it looks a little funny... maybe stand up faster?
					//return; 
					
					//this lets it stand up when it's sufficiently far out as
					//to look alright, but the threshold is much lower than it
					//is targeting to account for the delay in human perception
					//
					// some code is added to the crouch deceleration that might
					// push this down to 0
					//
					//if(mc.leading2>=0.25f) 
					return;
				}
				mc.jumping = 0.000001f;
			}
		}
		inline operator bool() //jumping?
		{
			//TODO: exclude jumping?
			//return started&&!canceled||mc.jumping; 
			return started&&!canceled||mc.jumping&&mc.jumping<1; 
		}

	}jumpstart; //typedef

	jumpstart Jumpstart; //singleton

	typedef struct //guard
	{
		int operator()(int,int);

		int guard_window;
		int guard_window2;
		int guard_draining;
		int guard_holding;
		int guard_releasing;	
		int guard_bashing;
		bool guard_bashing2;
		bool guard_raising;
		unsigned guard_bashtick;
		unsigned guard_sloshing;
		union //cross communication?
		{
			float guard_sloshing2;			
			int guard_fusion;
		};
		int counter;
		int guard_dropping;

	}guard;
	
	guard Shield,Weapon;
	
	typedef struct //camera
	{
		float diff,diff2;		
		int sway, sign, hack; //1,1,1
		bool stopped; //true
		float rsshock_absorber; //2021
		void init()
		{
			stopped = true;
			sway = sign = hack = 1;
			rsshock_absorber = EX::NaN; //SOM::xyz[1];
		}
		//analog camera, sky camera, and swinging arm model
		float operator()(float(&)[4][4],float(&)[4][4],float[6]);

	}camera; //typedef

	camera Camera; //singleton

	static const float pi,sqrt2;	

	static float signed_root(float x, float e=0.5f)
	{
		return (float)_copysign(powf(fabsf(x),e),x);
	}

	//REMOVE ME?
	//(copied from som.hacks.cpp)
	static float dot(float u[3], float v[3])
	{
		return u[0]*v[0]+u[1]*v[1]+u[2]*v[2];
	}
	static float radians(float u[3], float v[3], float n[3]) 
	{	
		float d = dot(u,v), r = acosf(d); 
		
		if(_isnan(r)) return d>0?0.0:pi;		
		
		return dot(n,v)>0?r:-r; //signed angle
	}		  
	static float length(const float a[3])
	{
		return sqrtf(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]);
	}		  
	static float distance(const float a[3], const float b[3])
	{
		float x = a[0]-b[0], y = a[1]-b[1], z = a[2]-b[2];

		return sqrtf(x*x+y*y+z*z);
	}
	static float lerp(float x, float y, float t)
	{
		return x+t*(y-x); 
	}
	static float spin(float a, float b)
	{
		return fabsf(b-a)<pi?b:b-pi*(b>a?2:-2);
	}
	static float O_to_2pi(float a) //0_to_2pi
	{
		return fmodf(a+pi*4,pi*2); //4 is arbitrary
	}
	static float slerp(float &a, float &b, float s)
	{
		a = O_to_2pi(a); b = O_to_2pi(b);

		return lerp(a,spin(a,b),s);
	}
	static float radians(const float &a, const float &b)
	{
		const_cast<float&>(a) = O_to_2pi(a);
		const_cast<float&>(b) = O_to_2pi(b);
		return spin(a,b)-a;
	}

	struct spring_damper_system //1D
	{
		//60 doesn't feel good for duck jumping anymore. I'm
		//not sure why, I changed up the pedals for turning
		//but they're quickly disabled when ducking. I think
		//the changes to damper/10 is more likely to be the
		//cause
		//2021: trying to untie the different systems so the
		//springstep system can be recalibrated independently
		//enum{ damper=50 }; //60
		float stiffness, memory[2];		
		float reposition(float anchor, float t, float danger=0, int steps=5)
		{
			enum{ step=10 }; int damper = step*steps;

			//F = -kx-vd
			//k is stiffness
			//x is displacement from anchor
			//v is velocity of x
			//d is damper/dampening	constant
			//good enough at 60fps but I have a hunch it's a problem
			//for larger time steps and degrades significantly at 30
			//15 may not be enough either?
			//2021: ironically it feels like 15 (MORE STEPS) may've
			//made it jerky? (I'M NOT SURE IT'S NOT SOMETHING ELSE)
			//5 feels okay... is 15 cursed? (it was 60/20 before 15)
			//const int steps = damper/15;
			//const int steps = damper/10;
			for(int s=0;s<steps;s++)
			{				
				//assert(0==damper%steps);
				float a = memory[1], ago = memory[0];			
				float displacement = a-anchor, velocity = (a-ago)*t;
				memory[0] = memory[1]; 
				float force = -stiffness*displacement-velocity*damper;
				memory[1]+=force/steps;
			}
			//hack: handle mouse after unpausing (and any further emergencies)
			if(danger&&danger<fabsf(memory[1]-memory[0])||!_finite(memory[1]))
			{
				EX::dbgmsg("spring was reset (%d)",SOM::frame);
				reposition(anchor);
			}
			return memory[1];
		}
		inline float reposition(float position)
		{			
			return memory[0] = memory[1] = position; 
		}
		//float position(){ return memory[1]; }
		operator float(){ return memory[1]; }

	}springs[5],leadspring,duckspring;

	//EXPRIMENTAL
	bool smoothing;
	char nudging[5]; 
	float smoothed[5]; 
	float time_lapse; //duckspring
	float springstep(int i, float gait, float t, bool unsnap)
	{
		if(gait||mc.hopping) unsnap = true;

		enum{ ratio=2, fudge=6, fudge2 = fudge*ratio };

		//NOTE: there's a problem with EX::Affects[2] when
		//turning since the resolution on the rotated gait
		//isn't so good
		float stiffen; if(mc.suspended&&i!=4)
		{
			if(0) //old way?
			{
				//NEW: ensure jumps are uniform
				return springs[i].reposition(gait);
			}
			else
			{
				stiffen = ratio; //snap2/fudge?
				goto suspend;
			}
		}
		bool resting = i<3&&mc.resting&&!mc.suspended;

		//2018: trying to smooth the snap as much as possible
		//without feeling overly drawn out (1 seems too much)
		//(1.5 feels floaty when maneuvering around monsters)
//		float fudge = resting?4:2, snap2 = resting?1:/*4*/1.75f;
		//trying new values after changing som_state_thumbs_boost
		//(trying to lessen runaway train effect)
		//going to 1 to improve experience in lowest/half gaits 
		//when nudging... perhaps this is too drastic, but it's
		//worse to leave the bad experience in place
		//
		//NOTE: 3 can be tuned to match the response of a thumbpad
		//however if it's too snappy it can seem like it's pulling
		//the controls away sometimes. it controls what happens if
		//there is not pressure on the pad. 3 seems like the right
		//balance... but drifty. 4 seems like too far. maybe 3.25?
		#if 0
//		float fudge = resting?4:2.5f, snap2 = resting?1:nudging[i]<20?0.5f:3.25f;
		//
		//SMOOTH TRANSITION
		//trying to make the transtion between gaits less obvious
		//trying to unify resting (ducked) and nonresting setting
		#else
		//float fudge = resting?4:4, snap2 = resting?1:nudging[i]<20?1:7;
		//converging/lessening stiffness in order to be able to increase
		//u2_power up to 1.75 in order to try to approximate the slowest
		//moving speed with the slowest turning speed
		//TODO/EXTENSION
		//fudge, etc. needs to be an extension at this point, as it will
		//depend on u2_power by a lot, but is also a point of preference
		//2020: trying different values since changing Affects[1] "lead"
	//	float fudge = 6, snap2 = resting?1:nudging[i]<20?1:12;
		//0.5 is good for jumping but a little sluggish? maybe movement
		//can be sped up elsewhere? m_s? (no doesn't work)
		//maybe this can go higher if if Affects[1] uses 0
		//(seems so, .6 is pretty comfy then)
		//float fudge = 6, snap2 = resting?0.5f:nudging[i]<20?1:12;
		//I mean to use ducking here instead, but this actually works
		//and it covers ducking also
		//2021: below max(0,2-rate)/2 only works because the maximum
		//of snap2/fudge is 2 and dividing by 2 yields values that
		//are close to 1-rate when not snapping (it's had to think of
		//a continuous function if the limit didn't happen to be 2)		
		float snap2 = resting?1-mc.resting/2:nudging[i]<20?1:fudge2;
		#endif
		stiffen = (unsnap?fabsf(gait):snap2)/fudge;
		//2021: prevent phantom movement when co-axis forces unsnap
		if(unsnap) stiffen = max(0.1f/fudge,stiffen); //0.1f/fudge
		suspend:
		bool rated = !EX::debug||~GetKeyState(VK_CAPITAL)&1; //2021
		//2021
		//when turning slowly but unsteadily the spring will
		//kink. modulating stiffness on the fly isn't really
		//how springs are supposed to work I think. I really
		//didn't expect this to work but
		//AS HAREBRAINED AS THIS IS IT SEEMS TO DO THE TRICK
		float rate = 1; if(rated) //if(unsnap)
		{
			//TODO: 0~3 should use this too but it feels a little
			//too floaty in the lower gaits so I want to see what
			//can be done first

			rate = fabsf(stiffen-springs[i].stiffness);
			assert(stiffen<=ratio); //12/6 (snap2/fudge)
			
			//this just doesn't work for fudge2 but the function 
			//needs to be continuous somehow???
			rate = (1-sqrtf(1-rate/ratio))*ratio;

			//6 is better for steady transitions but I worry it doesn't
			//handle snapping well since in the extreme it's -2 which is
			//clipped to 0... 5 shouldn't require max
			enum{ mode=5 };
			if(mode==6) rate = max(0,ratio-rate*ratio)/ratio;			
			if(mode==5) rate = (ratio-rate)/ratio; //also works
			assert(rate>=0);
			//if(i==3) EX::dbgmsg("3: %f (%f) %d",rate,gait,unsnap);
		}
		//EX::dbgmsg("smoothed[%d]: %f %f %d %f",i,mc.smoothed[i],gait,unsnap,rate);		
		//NOTE: t*5 here makes a runaway train effect when circling the 
		//right thumbstick. this is just to smooth transition stiffness
		//the effect wasn't noticed until smooth started to couple axes
		//I REALLY DON'T UNDERSTAND why but anything in addition to "t" 
		//makes everything go south								
		springs[i].stiffness = lerp(springs[i].stiffness,stiffen,t*rate); //t/**5*/		
		//6 is too stiff sometimes. 5 can feel loose but is better all
		//around, especially cornering
		return springs[i].reposition(gait,t,0.5f,/*rated?mode:*/5);
	}
	void smooth(float &x, float &y, float &z, float &u, float &v, float power, float t, bool unsnap[5])
	{
		//REWRITING
		float len, *s[5] = {&x,&y,&z,&u,&v}; if(1)
		{
			for(int i=0;i<3;i++) if(unsnap[i]||*s[i])
			{
				for(int i=0;i<3;i++)
				{
					//NOTE: this test may be a source of wobbles
					unsnap[i] = true; 
				}
				break;
			}
			for(int i=3;i<5;i++) if(unsnap[i]||*s[i])
			{
				//I'm adding nudging here because it's otherwise
				//possible to keep an unpressed axis moving that
				//is especially noticeable for the U/V axis pair
				//I don't know if there's a side effect, I don't
				//trust my senses rotating the right thumb stick
				//logically I think it's sound
				for(int i=3;i<5;i++) 
				{
					//NOTE: this test may be a source of wobbles
					unsnap[i] = true;
				}
				break;
			}
		}
		bool before; if(before=false) for(int i=0;i<5;i++) //2021
		{
			*s[i] = smoothed[i] = springstep(i,*s[i],t,unsnap[i]);	
		}
		if(0) //this is messing with resting/leaning diagonally
		{	
			//hack: constrain cheating analog game pads to 1
			len = sqrt(x*x+z*z); if(len>1){ x/=len; z/=len; }
			len = sqrt(u*u+v*v); if(len>1){ u/=len; v/=len; }
			//0: slower takeoffs are less jarring (sneaking?)
			for(int i=mc.resting?0:3;i<5;i++) //3 //non-linear turning rate?
			*s[i] = smoothed[i] = signed_root(*s[i],power);
		}
		else //rewritten
		{
			//TODO? this might need to be blended according
			//to ducking (1~2.) it's possible that stopping
			//to duck is sufficient to wipe the slate clean
			if(mc.resting)
			{
				len = sqrtf(x*x+z*z+y*y); //y???
				float rcp = len?1/len:0; x*=rcp; z*=rcp; y*=rcp; 
				if(len>1) len = 1;
				len = powf(len,power); //non-linear leaning rate?
				x*=len; y*=len; z*=len;
			}
			len = sqrtf(u*u+v*v); 
			float rcp; if(len)
			{
				rcp = 1/len;
			}
			else rcp = 0; u*=rcp; v*=rcp; 
			if(len>1) len = 1;
			extern bool som_state_square_uv;
			if(som_state_square_uv) //non-linear turning rate? (old way!)
			{
				//even while I know this is wrong, it still feels
				//less unwieldy
				//I think it doesn't suffer from round off errors
				u*=len; v*=len;
		
				u = signed_root(u,power);
				v = signed_root(v,power);
			}
			else //non-linear turning rate? 
			{
				len = powf(len,power); 
				u*=len; v*=len;
				//EX::dbgmsg("unsnap %f %f",u,v);
				//EX::dbgmsg("nudge %d",(int)nudging[3]);
			}
		}
		if(!before) for(int i=0;i<5;i++) 
		{
			*s[i] = smoothed[i] = springstep(i,*s[i],t,unsnap[i]);	
		}
	}
	void nudge(int i, int g1, int g2)
	{
		if(!jumping&&!hopping)
		{
			//int g = EX::Joypad::gaitcode(g1|g2);
			g1 = EX::Joypad::gaitcode(g1);
			g2 = EX::Joypad::gaitcode(g2);
			int g = max(g1,g2); //not canceled out?
			if(g>1&&nudging[i]<30) nudging[i]+=g-1;
			if(!g&&smoothed[i]<0.1f&&nudging[i]) nudging[i]--;
		}
		else nudging[i] = 90;
	}

	void reset() //NEW
	{
		//this is the most important bit
		memset(this,0x00,sizeof(*this));
		inverter = -1;
		squatting = 1;
		Engine.init(); Camera.init();
	} 

	void dbgmsg();

	static bool f4__1;
	static const float idle;

}som_mocap; //singleton

extern void som_mocap_dy(float d) //2022
{
	som_mocap.Camera.rsshock_absorber+=d; //YUCK
}
	  
bool som_mocap::f4__1;
const float som_mocap::idle = 0.1f;
const float som_mocap::pi = SOMVECTOR_PI;
const float som_mocap::sqrt2 = 1.414213f;
struct som_mocap &som_mocap::mc = ::som_mocap;

//REMOVE ME?
//EX::speedometer handler //Ex.output.h
extern void som_mocap_speedometer(float speeds[3])
{
	speeds[0] = som_mocap.speed;
	speeds[1] = som_mocap.speed2;
	speeds[2] = som_mocap.error;
}
static DWORD som_mocap_footstep_tick = 0; //2017
//static float som_mocap_footstep_volume = 0; //2020
static void som_mocap_footstep_soundeffect()
{
	//if(EX::debug) return;

	#ifdef NDEBUG
	//do this with frequency and left Ex.ini define
	//desired range
	int todolist[SOMEX_VNUMBER<=0x1020602UL];
	#endif

	//2017: this plays at very fast stutters under
	//certain conditions. hadn't noticed it before
	//NOTE: In general it's best to limit playback
	if(SOM::motions.tick-som_mocap_footstep_tick<200)
	return;
	else som_mocap_footstep_tick = SOM::motions.tick;

	int snd;
	EX::INI::Sample se; if(&se->footstep_identifier)
	{
		float f = se->footstep_identifier(SOM::motions.floor_object); 
		//todo: check high 12 bits for heel+toe model
		//2020: going ahead and masking this for now
		snd = EX::isNaN(f)?0:(int)f&0xfff; 
	}
	else snd = 30; if(!snd) return;

	int volume = 0, pitch = 0;

	float sneaking = 0, louder = //2020
	1-fabsf(som_mocap.crabwalk)*0.75f-sqrtf(som_mocap.backwards);
	louder = max(0,louder);

	//just roughing out. doesn't have to be continuous
	if(!som_mocap.bounce||som_mocap.touchdown_scorecard==10)	
	if(som_mocap.dashing>=0.85f)
	{
		float r2 = som_mocap.running*som_mocap.running;
		float j2 = som_mocap.jogging*som_mocap.jogging;

		sneaking = 1- //quietness	
		//2020: want fleeing to be distinctly audible
		//in order to hear that all 3 buttons are down
		//BUT I also want walking to sound like NPCs so
		//it makes walking louder if running is so quiet
		//as to be the same on the scale (EXTENSION?)
		//(a power function can be used in the Ex.ini)
		(0.3f*j2+0.5f*r2+0.5f*som_mocap.fleeing);
		float t = 1-som_mocap.running-som_mocap.jogging;
		//2020: there's a hole between sneaking and jogging
		//that's louder than either?
		//if(sneaking==1) //sideways or backward?
		{
			//might need to rethink this?
			//sneaking-=t*min(0.85f,powf(2*som_mocap.landspeed*som_mocap.dashing,2));			
			sneaking-=t*louder*powf(som_mocap.landspeed*som_mocap.dashing*1.5f,1.5f);
			pitch = t*-8*sneaking;
		}
		//else pitch = -8+8*som_mocap.running+4*som_mocap.jogging;
		pitch+=(1-t)*(-8+6*som_mocap.running+4*som_mocap.jogging+som_mocap.fleeing);
		volume = -250*sneaking*sneaking; 			

		//HACK: since adding "louder" there's a high pitch
		//donut hole that can happen in the dash transition
		//going forward
		if(som_mocap.dashing<1) pitch = min(-3,pitch);
	}
	else 
	{
		//NOTE: just trying to match the NPC volume level		
		volume = -150+50*som_mocap.landspeed; 
		pitch = -8+5*som_mocap.landspeed;
	}
	else pitch = -8; //hard landing
	
	//NOTE: pitch drops sound like decreased volume
	volume-=20*som_mocap.squat; pitch-=2*som_mocap.squat;

	if(!som_mocap.bounce) volume*=3; else pitch+=3; //2
		
	int r = rand(); //2017: less monotony?
	int x_factor = 10, x = 2+(x_factor-2)*som_mocap.dashing; 
	//TODO: SHOULD DO THIS IN TERMS OF FREQUENCY INSTEAD
	//FOR MORE GRANULARITY (SOM::se_frequency?)
	if(!som_mocap.bounce)
	pitch-=r%x?0:1;
	//+2 for zero-divide and to ensure some randomness.
	volume-=min(-volume,100)*(1+r%(x_factor+2-x)/6.0f);

	//2020: not sure about this
	if(float r=som_mocap.resting)
	{
		volume*=2-r; pitch-=2;
	}

	//0030.SND sounds funky below this level
	pitch = max(pitch,-10); 

	const float swap = SOM::cam[1];
	//NOTE: som_hacks_SetPosition is changing the center
	//from foot level to mid height
	SOM::cam[1] = SOM::xyz[1]-(SOM::cam[1]-SOM::xyz[1])/2;
	{	
		//with changes to snd_min_dist and moving it to be
		//below in 3D space the volume needs to be increased
		//somehow
		//volume/=2;
		//volume*=2; //quieter? //EXTENSION?

		//because NPC sounds are at their feet that is ear
		//level for all intents and purposes, so footsteps
		//need to be underground to sound best with reverb
		//SOM::se3D(SOM::xyz,snd,pitch,volume);	
		SOM::se3D(SOM::cam,snd,pitch,volume);	
	}
	SOM::cam[1] = swap;

	//som_mocap_footstep_volume = louder; //volume; //debug
}
void SOM::Motions::ready_config(DWORD ms, BYTE *keys)
{	
	//EX::dbgmsg("footstep: %f",som_mocap_footstep_volume);	

	float step = ms/1000.0f;
	
	//EXPERIMENTAL: this is averaged to smooth out
	//chaotic triple buffer timing
	/*if(1==EX_ARRAYSIZEOF(SOM::onflip_triple_time)) 
	{
		SOM::motions.step = step; //2020
		SOM::motions.diff = ms; //2020
	}
	else*/ step = SOM::motions.step; //2020

	if(!step) return; //2020: happened once?

	som_mocap.time_lapse+=step; //YUCK

	//hack: first-chance initialization
	EX::speedometer = som_mocap_speedometer;
	{
		som_mocap.Engine(step,keys); 
	}	
	
	//REMOVE ME? (communicating with clipper)
	if(SOM::motions.aloft>0) SOM::motions.aloft--;
	else assert(!SOM::motions.aloft);

	//NOTE: clinging is purely temporal... it may be 
	//tempting to factor leading2 into it, but I worry
	//the systemic consequences of that at this stage
	SOM::motions.clinging = som_mocap.lerp
	(SOM::motions.clinging,SOM::motions.cling,som_mocap.antijitter);
	SOM::motions.clinging = min(1,SOM::motions.clinging);
	if(SOM::motions.clinging<0.001f)
	SOM::motions.clinging = 0; //DEN
	SOM::motions.clinging2 = //som.state.cpp
	SOM::motions.clinging*
	SOM::motions.clinging*som_mocap.leading2;
	//2021: this is needed to slow walk steps with shield held out
	SOM::motions.arm_clear = 1-som_mocap.lerp
	(1-SOM::motions.arm_clear,som_mocap.arm_clear,som_mocap.antijitter);
	//this might help but it's not going to do anything
	//because mc.scaling doesn't start going down until
	//at the top. the built-in friction is good for now
	//(it could track pcstepladder or it could build up
	//over time while scaling)
	//*(2+cosf(M_PI*(som_mocap.scaling-0.5f)/0.5f));
	SOM::motions.leanness = 1; //ceiling test
	if(som_mocap.resting)
	SOM::motions.leanness-=min(som_mocap.leading,0.995f);
	//Number pc[12]
	SOM::motions.dash = som_mocap.dashing;
	assert(SOM::motions.dash<=1);
	//HACK: this is used to not "smooth" complex slopes
	SOM::motions.freefall = som_mocap.Engine.Prolog.freefall!=0;

	//if(1) som_mocap.dbgmsg();
}		  
float SOM::Motions::place_camera(float (&a)[4][4], float (&b)[4][4], float c[6])
{
	return som_mocap.Camera(a,b,c);
}
float *SOM::Motions::place_shadow(float tmp[3])
{
	if(0==som_mocap.holding_xyzt[3]) //return SOM::xyz;
	{
		if(1&&som_mocap.ducking>1)
		{
			//WARNING: FOR SOME REASON pcstate2 (not pcstate) HOLDS THE CORRECT
			//VALUES AT THIS STAGE OF THE PROGRAM (som_scene_shadows::draw) ???
			//OR SOMETHING ELSE
			float t = (som_mocap.ducking-1)*0.33333f;			
			tmp[0] = som_mocap.lerp(SOM::xyz[0],SOM::L.pcstate2[0],t); //pcstate
			tmp[1] = SOM::xyz[1];
			tmp[2] = som_mocap.lerp(SOM::xyz[2],SOM::L.pcstate2[2],t); //pcstate
			return tmp;
		}
		else return SOM::xyz;
	}
	if(1==som_mocap.holding_xyzt[3]) return som_mocap.holding_xyzt;
	for(int i=3;i-->0;) tmp[i] = 
	som_mocap.lerp(SOM::xyz[i],som_mocap.holding_xyzt[i],som_mocap.holding_xyzt[3]);
	return tmp;
}
void SOM::Motions::reset_config()
{
	som_mocap.reset(); //New Game / Game Over

	//2021: "sizeof(this)" fix (may break something)
	auto swap = tick;		
	memset(this,0x00,sizeof(*this)); leanness = 1;
	tick = swap;

	//2021: for some reason this isn't resetting
	//by itself anymore and is freezing (maybe I
	//just never died mid swing before)
	auto &mdl = *SOM::L.arm_MDL;
	SOM::L.swinging = 0;
	mdl.d = 1; 
	mdl.ext.d2 = 0;
	mdl.rewind(); //mdl.f = -1;
	mdl.rewind2();
}

//EXPERIMENTAL
static void som_mocap_ceiling(BYTE *kb) 
{	
	EX::INI::Player pc;
	float hit = SOM::motions.ceiling;	
	struct som_mocap &mc = ::som_mocap.mc; 				
	float height = pc->player_character_height+0.01f;
	if(!hit&&pc->tap_or_hold_ms_timeout<mc.action //auto-squatting?
	||som_mocap.surmounting_staging==2/*&&!SOM::L.pcstep*/) //HACK
	if(!mc.resting)
	{
		float dist = som_mocap.speed2*pc->tap_or_hold_ms_timeout/1000;
		//2020: shape2 is now for combat/circling
		//dist = max(dist,pc->player_character_shape+pc->player_character_shape2);
		dist = max(dist,pc->player_character_shape+pc->player_character_shape/2);
		float futurepos[3] = {mc.lookahead[0],0,mc.lookahead[1]};
		Somvector::map(futurepos).unit<3>().se_scale<3>(dist).move<3>(SOM::xyz);		
		futurepos[1]+=SOM::doppler[1]*(mc.speed-mc.speed2);			
		//aloft: this routine also has to scan for walls in advance
		if(SOM::enteringcrawlspace(futurepos)&&!SOM::motions.aloft)							
		{
			hit = SOM::motions.ceiling = pc->player_character_height2;
			//needed to get past short crawlspaces
			mc.auto_squatting = 1-mc.inverting; 	
		}
	}

	mc.ceiling = hit&&hit<height&&mc.f4__1?hit:0;
	//do thud sound effect
	if(hit&&hit<=SOM::L.height&&mc.jumping>=1) 
	{
		SOM::bopped = SOM::frame;

		int snd = -1;
		EX::INI::Sample se;
		if(&se->headbutt_identifier)
		{
			float f = se->headbutt_identifier(SOM::motions.ceiling_object); 
			if(!EX::isNaN(f)) snd = f;
		}
		//2017: Using se3D instead of se because it now loads unused SNDs
		if(snd>=0||EX::debug)
		{			
			snd = snd>=0?snd:1009; //1009 is just beep test		
			//REMOVE ME
			//there's a bug where you can't mix 2D and 3D sounds
			if(snd>=1000) SOM::se(snd); else SOM::se3D(SOM::xyz,snd); 
		}
	}
	//CONTINUED BELOW...
}
static void som_mocap_ceiling2(BYTE *kb) 
{
	//CAN'T REMEMBER WHY THIS MUST BE DONE BEFORE AND AFTER
	//A BUG APPEARED SHORTLY AFTER REMOVING THE LATTER CASE

	//2017: I don't know if this system even contemplated 
	//arches when it was designed. But they kind of count
	//as crawlspaces in and of themself. I can't think of
	//a way to designate certain arches as crawlspaces so
	//if their angle is less than X you just have to duck
	//for the time being.	
	if(SOM::arch>=M_PI/6) //Is 60 degrees enough?
	{
		//if(!som_mocap.inverting&&!som_mocap.scaling)
		if(!som_mocap.inverting&&!som_mocap.surmounting_staging&&!som_mocap.scaling)
		return;
	}

	if(som_mocap.ceiling) 		
	if(som_mocap.inverting&&som_mocap.inverter==-1)
	{  	
		som_mocap.inverter = 1;
		if(!som_mocap.upsadaisy&&kb[0xF3])
		{
			som_mocap.reverting = 0.000001f; 
			som_mocap.reverter = 1/som_mocap.inverting;
		}
	}
	else if(!som_mocap.jumping) 
	{		
		som_mocap.inverter = 1; //hack: edge cases
	}
}
extern float som_mocap_climb_y(float platform)				
{
	struct som_mocap &mc = ::som_mocap.mc;
	float inv = mc.inverting-mc.inverting*mc.reverting;

	//this is trying to climb stealthily when inverted
	return mc.lerp(SOM::clipper.ceiling-1,platform+0.25f,inv);
}
static void som_mocap_surmount(BYTE *kb)
{
	struct som_mocap &mc = ::som_mocap.mc; 
		
	//EXPERIMENTAL
	mc.cornering = mc.lerp(mc.cornering,
	SOM::motions.cornering,SOM::motions.step*30);

	//2020: the new input model is constantly 
	//resetting back to 1
	if(mc.scaling>0.8f||mc.surmounting_staging)
	{
		return; 
	}

	if(mc.resting&&!SOM::motions.aloft) //2020
	{
		assert(!mc.surmounting/*&&!mc.scaling*/);

		return;
	}

	//2020: letting holding activate objects 
	//so it needs to not chain into climbing
	if(1==mc.holding_xyzt[3]&&!mc.holding)
	{
		return;
	}

	//mc.scan_for_secret_doors = false;	

	EX::INI::Player pc;		
	
	bool dash = pc->do_not_dash?kb[0xF3]:kb[0x39];	

	const int tvh_ms = pc->tap_or_hold_ms_timeout;

	/*2020: I think the stairs thing may no longer apply
	//I did a lot of work for KF2 to improve stair stuff
	//I'm lowering it to capitalize on delay_surmounting
	//
	//REMINDER: some is required so stairs do not impede
	//movement when the Action button is pressed. however
	//it feels right that some push against the surface is
	//required*/
	//WARNING: switching from leading to leading2 (2020)
	bool pressure_requirement = mc.f4__1&&mc.leading2>0.1f; //0.2; //loosening
	bool pressure_requirement2 = mc.f4__1&&mc.leading2>0.5f;

	//EX::dbgmsg("leading %f (%d)",mc.leading,SOM::frame);
	//EX::dbgmsg("holding: %f %f (%.2f)",mc.leading2,SOM::motions.clinging,(int)(100*(mc.cornering-0.001))/100.0);

	/*having problem climbing onto convex corners, but
	//also there is another problem that later defeats
	//the climbing attempt?
	const float tol = 0.01f; //0.004f; //ARBITRARY
	*/const float tol = 0.2f; //0.1f; //loosening
	float futurepos[3] = {mc.lookahead[0],0,mc.lookahead[1]};
	Somvector::map(futurepos).unit<3>().se_scale<3>(tol).move<3>(&SOM::L.pcstate);		
	
	//REMINDER: THIS INCLUDED mc.action ZEROED
	//HACK: relaxing timing and experimenting a little
	//bool delay_surmounting = !dash&&mc.action<tvh_ms
	bool delay_surmounting = !dash;

	if(SOM::event>=tvh_ms/*&&mc.surmounting*/)
	{
		delay_surmounting = false;

		mc.surmounting = false; //2020 impede?

		//PIGGYBACK holding?
		if(SOM::event<=tvh_ms+(int)SOM::motions.diff
		&&pressure_requirement2&&mc.Engine.push
		//warning: reducing this coincides with changing how
		//mc.ducking is considered
		//&&SOM::motions.clinging*SOM::motions.cornering>=0.75f
		&&SOM::motions.clinging*mc.cornering>=0.25f //0.75f
		//HACK? what I'm trying to (hastily)
		//accomplish is to not grab the wall
		//if it looks like you're running on
		//the edge of a wall (to the player)
		//NOTE: grabbing after running can't
		//really work because it breaks away
		//immediately as if pushed by an NPC
	//&&mc.ducking<=1) //0.75?
	&&mc.ducking<=1-mc.leading2/2*!mc.running) //0.5f?
		{
			//this is an extensions of crouching
			if(!pc->do_not_duck&&!mc.interrupted
			&&!SOM::motions.aloft
			&&!mc.bounce //YUCK
			&&!mc.resting //can't hurt
			//&&!mc.Engine.rest //wrong I think?
			&&(!mc.attack||!mc.attack2)) //mc.fleeing?
			{
				assert(!mc.holding);
				for(int i=3;i-->0;)
				mc.holding_pcstate[i] = SOM::L.pcstate[i];
				if(!mc.holding_xyzt[3])
				{
					for(int i=3;i-->0;)
					mc.holding_xyzt[i] = SOM::xyz[i];
					mc.holding_xyzt[3] = 1;
				}
				else //possible?
				{
					assert(1!=mc.holding_xyzt[3]);
					SOM::motions.place_shadow(mc.holding_xyzt); //HACK
					mc.holding_xyzt[3] = 1;
				}
				mc.leadingaway = 0;
				mc.leadspring.reposition(0);
				mc.holding = true; 

				mc.Jumpstart = 0; //cancel
			}
		}
		if(mc.holding) return; //surmountedcrawlspace?
	}
	if(delay_surmounting&&mc.action&&!mc.surmounting)
	{
		if(pressure_requirement&&mc.Engine.push)
		if(SOM::frame-SOM::black>(unsigned)SOM::event)
		goto maybe_surmounting;
	}

	if(!mc.surmounting) 
	//some push is required even while jumping
	//REMINDER: SOM::motions.cling is climbing back onto a
	//ledge while walking off of it at full speed.
	if(mc.Engine.push&&SOM::motions./*cling*/clinging>=0.75f||mc.landing_2017)
	if(SOM::motions.aloft) 
	{
		//this is feeding landing_2017 until it resolves		
		if(mc.scaling) return;
		SOM::motions.aloft+=2;

		//TODO? look ahead just like below

		//TODO: figure out the climbing surface and do pull up animation if
		//it's high enough
		mc.surmounting_pullup = false;

		//if on rise climb to fence2, else fall onto fence
		//(1: fence just isn't enough to feel fair for now)
		goto mc_scaling_1; //mc.jumping&&mc.jumping<1?1:0.000001f;
	}	
	else if(dash&&!mc.action&&pressure_requirement) 
	{
		maybe_surmounting:
		mc.surmounting = SOM::surmountableobstacle(futurepos);		
		mc.surmounting_pullup = true;
		//if(dash)
		//mc.scan_for_secret_doors = !mc.surmounting;
	}

	if(mc.surmounting)
	if(delay_surmounting) //!dash&&mc.action<tvh_ms
	{
		SOM::eventick = 0; //cancel subtitle
		mc.surmounting = false; 
		if(!mc.interrupted //2020
		&&(!mc.holding_xyzt[3]||!mc.Jumpstart))
		{
			//EXPERIMENTAL
			//HOW? I'm pretty sure climbing should invert somehow
			//mc.inverter = 1;

			mc_scaling_1: 		
			//mc.scaling = 1;
			mc.surmounting_staging = 4;
			if(SOM::stool) *SOM::stool = pc->player_character_shape; 	
		}
	}
	/*2020: WHAT DID THIS DO? 
	//the new "holding" feature makes it impossible to input this
	//scenario since it works the exact same way... I can't recall
	//why this was necessary... perhaps it's obsolete?
	else if(dash&&mc.action+SOM::motions.diff>=tvh_ms)
	{
		mc.surmounting = false; 		
		if(!mc.interrupted) //2020
		{
			futurepos[1]+=0.001f;
			if(SOM::surmountedcrawlspace(futurepos))
			mc.inverter = 1;
		//	else goto mc_scaling_1; //experimenting
		}
	}*/
	else mc.surmounting = dash;
}

enum{ som_mocap_circle_before=0 }; //TESTING
static void som_mocap_circle(int &a, int &b)
{
	if(a<5&&b<5) return; //paranoia

	//just worked this out on graph paper
	if(a==7&&b==5){ a = 6; b = 5; return; }
	if(a==5&&b==7){ a = 5; b = 6; return; }
	if(a==7&&b==4){ a = 6; b = 3; return; }
	if(a==4&&b==7){ a = 3; b = 6; return; }

	a = b = 5; //take a straight diagonal
}	 
static void som_mocap_circle_2020(BYTE *kb)
{
	//restrict movement along diagonals
	int x4C = max(0,EX::Joypad::gaitcode(kb[0x4C]));
	int x50 = max(0,EX::Joypad::gaitcode(kb[0x50]));		
	int x4B = max(0,EX::Joypad::gaitcode(kb[0x4B]));
	int x4D = max(0,EX::Joypad::gaitcode(kb[0x4D]));

	//2017: antagonistic inputs are triggering som_mocap_circle
	//and causing it to generate nonexistant orthogonal outputs
	//NOTE: this is because things like Jumpstart alter the key
	//codes. why isn't it done at the top of engine::operator()?
	//if(x4C+x50+x4B+x4D>10) 
	if(abs(x4C-x50)+abs(x4B-x4D)>10) //restrict movement
	{	
		//EX::dbgmsg("circle %d ---%x %x %x %x",SOM::frame,x4C,x50,x4B,x4D);
		som_mocap_circle(x4C?x4C:x50,x4B?x4B:x4D);			
		kb[0x4C] = EX::Joypad::analog_dither(EX::Joypad::bytecode(x4C));
		kb[0x50] = EX::Joypad::analog_dither(EX::Joypad::bytecode(x50));
		kb[0x4B] = EX::Joypad::analog_dither(EX::Joypad::bytecode(x4B));
		kb[0x4D] = EX::Joypad::analog_dither(EX::Joypad::bytecode(x4D));
	}
}

enum{ som_mocap_guard=SOMEX_VNUMBER>0x1020304UL };
extern bool som_mocap_allow_modifier() //2021
{
	int tvh = EX::INI::Player()->tap_or_hold_ms_timeout;
	if(som_mocap_guard&&!som_mocap.hold_cancel)
	return som_mocap.mc.attack2<tvh&&som_mocap.mc.attack<tvh;
	return true;
}
extern void som_mocap_Ctrl_Shift_cancel_attacks()
{
	if(som_mocap_allow_modifier())
	{
		som_mocap.attacked = som_mocap.attacked2 = true;

		//2021: this is just to let macros be inputted
		//quickly without raising a shield, etc.
		som_mocap.hold_cancel = true; 
	}
}
extern bool som_mocap_attacks3() //pcequip2?
{
	return som_mocap.attack3||som_mocap.magic3||som_mocap.sword_magic3;
}

static int som_mocap_windup2()
{
	//debugging/assuming 500 is maximum
	int cmp = SOM::motions.tick-SOM::motions.swung_tick;
	if(cmp>=500) return 0;

	int id; if(auto*mv=SOM::movesets[0][0]) 
	{
		id = mv->us[0];
	}
	else //YUCK
	{					
		id = SOM::L.pcequip[0];
		id = id>=250?-1:SOM::L.item_prm_file[id].s[0];
		if(id>=0) id = SOM::L.item_pr2_file[id].uc[72];
	}

	EX::INI::Player pc;
	int w2 = (int)pc->arm_ms_windup2((float)SOM::motions.swung_id,(float)id);			
	return w2>cmp?w2-cmp:0;
}
				

#ifdef _DEBUG //MSVC2010 debugger bug
static struct som_mocap &mc = ::som_mocap.mc; 
#endif
void som_mocap::engine::operator()(float step, BYTE *kb)
{
	auto &mdl = *SOM::L.arm_MDL; //2021

	EX::INI::Option op; EX::INI::Player pc;
	EX::INI::Detail dt;	

	//hack: reconstructing
	//const int diff = step*1000; //s->ms
	//const int diff = SOM::motions.diff;
	const int tvh_ms = pc->tap_or_hold_ms_timeout;
	const float tap_vs_hold = tvh_ms;
	//const float t = diff/tap_vs_hold;
	const float t = step*1000/tap_vs_hold;
	
	//REMOVE ME???
	//1: extrapolation is probably not good?
	mc.antijitter = min(1,step*15);
	
	//NOTE: this really cancels arm movement
	{
		float swing = SOM::L.swinging!=0;
		SOM::motions.swing = lerp(SOM::motions.swing,swing,mc.antijitter*0.5f);
	}

	//2022: save the real state of the action
	//mainly for fast running
	hold(mc.active,kb[0x39]);

	mc.trigger = 0; switch(kb[0x39])
	{
	case 0xFC: mc.trigger = 1; break;
	case 0xFE: mc.trigger = 2; break;
	}
   
	//f4__1 = SOM::L.f4?*SOM::L.f4:1;  //f4==1
	f4__1 = SOM::L.f4; //f4==1

	//2020: make it so scaling player_character_stride
	//is adequate to speed up or slow down
	SOM::walk = SOM::_walk*pc->player_character_stride;
	SOM::dash = SOM::_dash*pc->player_character_stride;
	
	//2020: moving this here from analog::operator() because
	//gaits_max is distorted diagonally (som_state_thumbs_boost?)
	//but I don't know if this will help with gaits below the highest 
	//gaits but it seems like it should be done before Jumpstart
	{	
		//something feels wrong since moving this to the top
		//I'm using som_mocap_circle_before to try both ways 
		if(som_mocap_circle_before) som_mocap_circle_2020(kb);
	}
	//need to collect before Jumpstart
	const int gaits_1 = mc.gaits_max;
	int gaits_z = EX::Joypad::gaitcode(kb[0x4C]?kb[0x4C]:kb[0x50]);
	gaits_z = max(0,gaits_z); //half gait?
	int gaits_x = EX::Joypad::gaitcode(kb[0x4D]?kb[0x4D]:kb[0x4B]); 
	gaits_x = max(0,gaits_x); //half gait?		
	mc.gaits_max = gaits_x+gaits_z;
	//HACK: trying to remove bias on diagonal... maybe 
	//som_mocap_circle should do this to the input itself
	//som_state_thumbs_boost is probably responsible for this
	//diagonals definitely need to hit 7, but using this to test
	//for lower gaits (like some summitting code I just added) will
	//be uneven on diagonals if this isn't done
	//
	// TODO: when Jumpstart accumulates it's probably converting
	// off diagonal inputs into diagonal inputs
	//
	//if(mc.gaits_max<8) //ignore if maybe full press (being cautious)
	{	
		extern bool som_thumb_boosted();
		if(som_thumb_boosted())
		{
			float diagonal = 1-fabsf(gaits_x-gaits_z)/7;
			mc.gaits_max_unboosted = (mc.gaits_max-mc.gaits_max*diagonal*0.25f)+0.5f;
		}
		else mc.gaits_max_unboosted = mc.gaits_max;
		mc.gaits_max_unboosted = min(7,mc.gaits_max_unboosted);
	}
	switch(mc.gaits_max2-mc.gaits_max)
	{
	default: mc.gaits_max2 = gaits_1;
	case 1: break;
	case 2: mc.gaits_max2 = mc.gaits_max; 
		goto gaits_max;
	}
	if(mc.gaits_max>=7) gaits_max:
	{
		mc.gaits_max_residual = 33*6;
		
		//NECESSARY?
		//give stealth squat a technical window so it isn't inputted
		//just because of mechanical timing
		if(mc.stealth_squatting)
		if(mc.stealth_squatting<=mc.gaits_max_residual)
		{
			//if(EX::debug) MessageBeep(-1);

			mc.inverter = -1; //mc.stealth_squatting = 0;
		}
	}
	else mc.gaits_max_residual-=min(SOM::motions.diff,mc.gaits_max_residual);
	//EX::dbgmsg("gaits_max: %d (%d) %d",mc.gaits_max_residual,mc.gaits_max,mc.gaits_max2);

	//NEW: high fidelity analog model
	mc.smoothing = op->do_u2&&SOM::u;
	 
	if(mc.sliding) kb[0xF3] = 0x80;

	//2020: moving up so mc.interrupted is
	//available from top
	if(!SOM::emu)
	if(SOM::hit||SOM::invincible) //receiving damage?
	{			
		//NOTE: I forget how this works but something
		//is holding you down longer depending on the
		//impact. I'd like to document what. it works
		//great for mc.recovering

		mc.resting = 0; //NEW: got stuck mid duck?

		if(SOM::hit)
		if((size_t)SOM::hit>=1000)
		mc.recovery = 1;
		else mc.recovery = //poison/falling damage
		(size_t)SOM::hit*10/(float)SOM::L.pcstatus[SOM::PC::_hp];
		//2022: this somehow 24.8550720 today when
		//loading a save file in Moratheia
		if(mc.recovery>1) mc.recovery = 1;

		//2020: sticking on poison effect?
		if(SOM::hit)
		if(mc.ducking</*1*/0.95f)
		{
			kb[0xF3] = 0x80; //ducking
		}
		else //SOM::hit = 0;
		{
			//2021: som.hacks.cpp is settinng SOM::red
			//to 0 after the red flash window
			if(!SOM::red&&SOM::invincible<SOM::frame-15)
			{
				SOM::hit = 0;
			}
		}
		if(SOM::invincible&&!SOM::hit)
		if(mc.ducking) 
		{
			kb[0xF3] = 0; //rising
		}
		else SOM::invincible = 0;

		mc.interrupted = true; kb[0x39] = 0;
	}	
	else if(mc.interrupted)
	{
		if(mc.interrupted=kb[0x39]) 
		{	
			kb[0xF3] = 0x80; kb[0x39] = 0;
		}
	}
	if(!mc.interrupted) //2021
	{
		mc.recovering-=min(t,mc.recovering);
	}
	else mc.recovering = max(mc.recovery,mc.recovering);

	//thinking about reversing the bounce
	//animation for landing_2017 but it's not
	//working out
	if(pc->player_character_shape<SOM::L.shape)
	{
		mc.landing_2017 = true; 	
		kb[0x4B] = kb[0x4D] = kb[0x4C] = kb[0x50] = 0;
	}
	else mc.landing_2017 = false;
	push = kb[0x4B]|kb[0x4D]|kb[0x4C]|kb[0x50];

	if(!push) mc.neutral = SOM::motions.tick;

	mc.event_let_go = SOM::event;
	int event_before = SOM::event;
	hold(SOM::event,kb[0x39]);
	if(SOM::event||mc.interrupted)
	{
		mc.event_let_go = false;
	}
	if(mc.event_let_go)
	{
		mc.hud_event = SOM::motions.tick;
	}
	if(mc.holding_xyzt[3])
	{	
		if(!mc.holding)
		{
			float rate = 1+sin(pi*mc.holding_xyzt[3]);
			mc.holding_xyzt[3]-=min(t*rate/2,mc.holding_xyzt[3]);
		}
		else if(!SOM::event //let go?
		//NOTE: has side effect of walking only working
		//||memcmp(mc.holding_pcstate,SOM::L.pcstate,sizeof(float)*3))
		||0.1f<Somvector::measure<3>(mc.holding_pcstate,&SOM::L.pcstate))
		{
			mc.holding = false; mc.resting = 0;

			switch(SOM::eventype)
			{
			case 0x29: //receptacle (I guess?)
			case 0x1E: //spear trap?
			case 0x15: case 0x28: //treasure/switch
			case 0xb: case 0xc: case 0xd: case 0xe: //doors

				if(SOM::eventapped)
				{
					SOM::L.controls[-4]|=0x8; //complete activation
				}
				break;

			case 0x14: case 0x16: //simple containers

				som_title_further_delay_sub(-300); //subtitle
			}
		}
	}
	mc.Jumpstart(kb); //wants pristine inputs
	{
		//landing_2017 surmount is
		//overriding push
		som_mocap_surmount(kb); //EXPERIMENTAL		
		if(mc.surmounting_pullup)
		if(mc.surmounting&&!mc.ducking
		&&SOM::motions.clinging>=0.75f)
		{
			//2020: defer dash until scaling
			//NOTE: this somehow (magically)
			//lets tap replace hold to enter
			//crawlspaces!
			SOM::L.dashing = 0; 
		}		
		//reminder: this covers tapping crawlspaces as well
		//if(mc.surmounting_pullup&&mc.scaling)
		if(mc.scaling2==1&&!mc.peaking)
		{		
			//REMINDER: if fast climbing this forces to duck
			//duck down. if there is a ceiling then climbing
			//is slow and the ceiling forces to duck instead

			if(mc.inverter!=1) kb[0xF3] = 0x80;
		}
		else if(!mc.ducking)
		{
			//REMINDER: this causes the gauge to be full up
			if(mc.leading>0.5f&&!mc.surmounting&&!mc.holding
			//YUCK: something is driving a 1.5 deep crouch?
			||mc.Jumpstart&&!mc.Jumpstart.squat_walk) 
			{	
				SOM::L.dashing = 0; 

				kb[0xF3] = kb[0x39] = 0; //NEW: was twitching
			}
		}
	}		

	//float dashing = SOM::L.dashing
	//?float(*SOM::L.dashing)/tap_vs_hold:0; //SOM::L.hold
	float dashing = SOM::L.dashing/tap_vs_hold;
	if(mc.holding) dashing = 0;

	bool standing = //recharging
	//NEW: -F3- action added as jogging is sticking?!
	1==mc.jogging&&mc.action&&!mc.resting
	||SOM::motions.aloft; //mc.jumping||mc.falling;
	//NEW: somehow able to jog while squatting?!
	if(mc.inverter==1) standing = false;
							 
	if(kb[0x39]&&!pc->do_not_dash)
	{
		kb[0xF3] = kb[0x39]; //DASH
	}
	//NOTE: doing this earlier disables jumping
	//hold(mc.action,kb[0xF3]);
										 
	/*MOVED into "this"*/
	//static bool rest = false; //hack			

	//technically this lets the
	//player go back down before 
	//they come all of the way up
	if(dashing>mc.dashing)
	{
		//HACK: adding bent is a last ditch effort to patch
		//a hard clipping case. it doesn't appear to glitch
		//bool impede = mc.resting||mc.scaling||mc.bent>=0.2f;
		float impede = (mc.resting?1:0)+mc.scaling/2;
		/*
		//assuming climbing with DASH!
		if(!impede) //ducking/climbing
		{
			//mc.dashing = min(1,dashing);
			//this lets being forced to look up by dashing be a smoother transition
			//NOTE: 0.25f is pretty good with scaling but sometimes has a big burst
			//if(mc.bent>=0.25f)
			if(mc.bent>0.001f) //looking down?
			mc.dashing = lerp(min(1,dashing),mc.dashing,sqrt(min(0.5f,mc.bent)*2));
			else 
			mc.dashing = min(1,dashing);
		}*/
		if(mc.bent>0.001f) //looking down?		
		impede+=sqrtf(min(0.5f,mc.bent)*2);
		mc.dashing = lerp(min(1,dashing),mc.dashing,min(1,impede));
		
		if(!mc.stealth_squatting) //NEW
		{
			//NEW: limit crouch->squat transition to walk
			//FOR SOME REASON THIS WON'T WORK IN JUMPSTART
			if(mc.inverter==1&&mc.inverting<1&&!kb[0xF3])		
			mc.dashing-=min(1-mc.inverting,mc.dashing);
			if(mc.reverting&&!kb[0xF3]) //in crawlspace
			mc.dashing-=min(mc.reverting,mc.dashing);
		}
		else goto stealth_squatting;
	}
	if(mc.stealth_squatting) stealth_squatting:
	{
		if(mc.inverting==1||mc.inverter!=1)
		{
			mc.stealth_squatting = 0;
		}
		else mc.stealth_squatting+=SOM::motions.diff;
	}
			
	//if(dashing>=1) //2020: hoisting out of next block
	{
		//I'm trying here to let crouching be a valid 
		//way of interacting with objects at your feet

		if(SOM::event>=tvh_ms)
		{
			//SOM::eventick = 0;

			if(event_before<=tvh_ms||mc.holding) //2020: only once
			{				
				switch(SOM::eventype)
				{
				case 0x29: //receptacle (I guess?)
				case 0x1E: //spear trap?

				case 0xb: case 0xc: case 0xd: case 0xe: //doors

					if(mc.holding)
					{
				case 0x14: case 0x16:
				case 0x15: case 0x28: 
					
					if(mc.holding) //2020
					{
						int delay = SOM::event-event_before;
						SOM::eventick+=delay; //delay opening
						som_title_further_delay_sub(delay);
						break;
					}
					else if(!push) //2020 
					{
						//2020: exempt these in case players want to
						//crouch activate them
						//(want to do something more complex one day)

						//want it to feel the same crouching as
						//ducking
						int delay = tap_vs_hold*0.85f;
						SOM::eventick+=delay; //delay opening
						som_title_further_delay_sub(delay);
						break;
					}}							
				default: SOM::eventick = 0; //cancel subtitle
				}
			}
		}
	}

	if(dashing<1)
	{
		if(dashing>mc.ducking) mc.ducking = dashing;
	}
	else //standardize
	{
		if(mc.ducking<1)
		{
			mc.ducking = 1;

			//assert(!mc.peaking); //NEW
			mc.peaking = max(mc.peaking,0.000001f);

			//2020: walk away from dashing faster?
			//NOTE: this makes fighting monsters and
			//dashing sideways "feel" a lot better
			if(!mc.scaling) 			
			if(mc.fastwalk<1) //mc.fastwalk = 1; //sliding?
			{
				mc.fastwalk = max(mc.fastwalk,1-mc.squat/2); //HACK
			}

			/*2020: moving outside
			if(SOM::event>=tap_vs_hold)
			{
				SOM::eventick = 0; //cancel subtitle
			}*/
		}
		else if(mc.resting>mc.ducking-1&&!mc.holding_xyzt[3])
		{
			if(mc.ducking<2)
			{
				mc.ducking = mc.resting+1;

				if(mc.ducking>=2)
				{
					mc.crouched = SOM::motions.tick;
				}
			}			
		}

		//moving outside so it rises up when leaving
		//a tunnel. I think it used to do so
		if(mc.inverter==1&&mc.peaking&&!mc.reverting)
		{
			//2020: don't stand up if trying to do a 
			//new stealth mode to squat mode transition
			if(!mc.Jumpstart)
			{
				if(kb[0xF3]||SOM::hit) mc.inverter = -1;
			}
		}
		
		goto resting;
	}			
	if(mc.holding
	||mc.Jumpstart.squat_jog&&mc.leading2<0.1f) resting:
	{	
		if(!pc->do_not_duck) //2020: request crouch/hold?
		{
			bool arrest;
			if(!mc.holding)
			if(SOM::event==SOM::motions.diff)
			if(mc.Jumpstart.squat_jog
			||mc.dashing==1
			&&mc.ducking<=1
			&&mc.peaking==1) //EXPERIMENTAL
			{
				//NOTE: this entering crouch/squat by pressing the action button
				//immediately upon leaving dash mode inside the auto-jump window
				//it's disabled when circle-strafing at non-stealth speeds since
				//it's unwelcome in that case
				if(!rest&&!SOM::motions.aloft)				
				{	
					//HACK: this should be the stealth gait when in stealth mode
					//I'm assuming it's a good gait for running/jogging although
					//if it's too high or too low it will change the behavior of
					//this feature
					int gait = op->do_dash?dt->dash_jogging_gait-1:3;
					//want to work backward too!
					//if(mc.jogging||mc.running) //TODO? beyond some percentage?
					if(gait>=gaits_x
					||mc.jogging+mc.running>0.7f) //SKETCHY? is this possible???
					{
						if(unsigned time=EX::Joypad::trigger_full_release(mc.trigger-1))
						if(time>=50)
						{
							mc.Jumpstart = 0; //cancel

							mc.squatting = 0.000001f;

							arrest = true; goto duck_and_run;
						}
					}
					else if(!mc.sliding&&!pc->do_not_slide)
					{
						mc.sliding = 0.000001f;
					}
				}				
			}
			arrest = mc.holding||kb[0xF3]
			&&!pc->do_not_duck&&!pc->do_not_jump; //DASH

			//REMINDER: rest SEEMS TO TRACK FOR RUNNING TOO?!
			if(arrest&&!rest)		
			if(!push&&!standing&&!mc.interrupted||mc.holding)
			{
				duck_and_run: mc.resting = 0.000001f;
			}
					
			rest = arrest;
		}
	}
	if(mc.ducking>=2) SOM::crouched = SOM::frame;

	//2020: stealth->squat has a huge speed drop off. I'm
	//not sure what to do about the big differential from
	//walking to dashing?
	//
	//2020: I'm thinking maybe fastwalk smooths this over

	//mc.squat = 0; //NEW
	//2017: This seems like a typo. Don't know?
	//if(mc.inverter>-1)	
	//mc.squat = (1-mc.dashing)*(1+mc.inverter);
	/*2020: cutting in half (keeping continuous) (peaking???)
	if(mc.inverter==1)
	mc.squat = (1-max(mc.dashing,mc.peaking))*(1+mc.inverting);*/
	mc.squat = (1-mc.dashing)*mc.inverting;

	/////////////////////////////////////

	//Reminder: this problem has gone away
	//since I noticed the two frame boost
	//when jumping falling to overcome the
	//original gravity system is no longer
	//required (see Prolog) but it may be
	//the problem still exists for certain
	//ceiling heights
	//FIX ME?
	//FIX ME?
	//FIX ME?
	//2020: Happens jumping between barrels	r
	//in KF2's lobby. Jumping clips ceiling.
	//HACK: Need to find the source of this.	
	float *debug = &SOM::L.pcstate;
	float *debug2 = &SOM::L.pcstate2;
	if(!_finite(debug[0]))
	{
		if(EX::debug) MessageBeep(-1); //_finite?
		debug[0] = debug2[0];
		assert(_finite(debug[1]));
		debug[2] = debug2[2];
		EX::dbgmsg("Reset NaN xz: %d",SOM::frame);
	}

	mc.suspended = Analog.Suspend.suspended; 

	Prolog(step);
	Analog(step,kb);
	Epilog(step);	

	mc.suspended|=(bool)Analog.Suspend.suspended;

	/////////////////////////////////////
			
	//limit drain to leaning away from contact point
	{
		float y = mc.leadingaway; //*mc.leadingaway;
		mc.holding2 = lerp(mc.holding2,y*mc.holding*sqrt(mc.leading2),step*5);
		//EX::dbgmsg("holding2: %f (%f)",mc.holding2,y);
	}

	//EX::dbgmsg("scaling: %f (%f) %f",mc.scaling,+SOM::L.pcstepladder,+SOM::L.fence);

	if(!SOM::emu)
	{
		//2020: the scaling part is pretty weird... it just
		//disables the dash button at the very end top that
		//for some reason makes the exit less violent while
		//not sacrificing the pleasing tension from dashing
		//if(!standing&&(!mc.scaling||!SOM::L.pcstepladder))
		if(!standing&&(mc.scaling!=1||SOM::L.pcstepladder))
		{	
			bool dash = kb[0xF3];
			bool rush = dashing&&dashing<1&&!pc->do_not_rush;			
	
			if(dash||rush) SOM::L.controls[-3]|=0x20; 
			if(mc.holding) SOM::L.controls[-3]|=0x20;
			
			//trying to increase refill to encourage cover
			//based play option
			//if(mc.resting==1) SOM::L.controls[-3]&=~0x20; 
			if(mc.holding)
			{
				if(mc.holding2<0.7f) SOM::L.controls[-3]&=~0x20; 
			}
			else if(mc.ducking==2&&mc.resting>0.25f) SOM::L.controls[-3]&=~0x20; 
		}
		else SOM::L.controls[-3]&=~0x20; 		
		
		if(SOM::eventapped&&SOM::eventick)
		if(SOM::eventick<=SOM::motions.tick) //2020
		if(SOM::motions.tick-SOM::eventick>=tvh_ms+SOM::motions.diff)
		{
			SOM::L.controls[-4]|=0x8; //complete activation
		}
	}
	else if(kb[0x39]) SOM::L.controls[-3]|=0x20;
	
	float decel = t;
	{
		//TODO: soften camel hump on squat->stealth gaits 1/2 (note: this is stealth->squat)
		static float test = 1;
		//this is supposed to slow down gaits 1/2 while being symmetrical with squat->stealth
		int gm = 7-min(3,mc.gaits_max);
		//test = lerp(test,mc.stealth_squatting?2:1,mc.antijitter);
		test = lerp(test,mc.stealth_squatting?powf(gm,0.75f-mc.inverting/2):1,mc.antijitter);
		//EX::dbgmsg("gaits_max: %d (%d) %d",gm,mc.gaits_max,mc.stealth_squatting);
		decel/=test; //testing
	}
	if(mc.fastwalk>1) decel*=1/mc.fastwalk; //sliding?
	
	if(mc.landing)
	{
		if(mc.landing-=min(t,mc.landing))
		{
			//hit the ground running / skipping
			if(!kb[0xF3]) mc.landing = 0.000001f;				
		}
	}
	else if(!standing)
	{
		//2018: trying to slow down run->walk 
		//deceleration so it is a less jarring experience
		if(mc.ducking<=1) //2020: crouch->run
		if(mc.running+mc.jogging&&!SOM::hit)
		{
			//could differentiate these between jog/run
			//decel = 0.5f; //works well
			//decel = 0.75f; //better?
			//1 helps jogging (can simplify in that case)
			//decel*=(1-decel)+decel*(1-max(mc.running,mc.jogging));
			decel*=1-max(mc.running,mc.jogging);
		}

		//2020: dashing sets m_s on next poll!!
		//(SOM::L.dashing is 0)
		if(!mc.suspended)
		mc.dashing-=min(/*t*/decel,mc.dashing); //decel
	}		
	if(!mc.resting||standing||mc.holding_xyzt[3])
	{
		//FIX ME
		//som_mocap_squat changes the "upsadaisy" dynamic
		//from how I originally designed it. for the time
		//being it looks fine to me without the effect so
		//the state is managed separately for input needs
		if(1||!mc.upsadaisy)
		{
			//EXPERIMENTAL
			//2020: standing up faster when leaned out?
			//
			// this seems more stable since you're 
			// already up higher it makes sense to take
			// less time to get up
			//
			//float ll = mc.leadingaway;
			float ll = 1; if(1)
			{
				//lerp has to bridge squat-walk since
				//this isn't appropriate in that case
				//it also bleeds into jumps but since
				//rising up faster is like jumping it
				//could even be a good thing
				static float test = 1; if(!mc.inverting)
				{
					//ll = mc.smoothing?mc.leadspring:mc.leadingaway;
					ll = mc.leadingaway;
					//the effect should not be too dramatic since 
					//the squat height is only about 1/3 the way
					//lerp below sucks some of the power out of it
					ll = 1+powf(ll,2);
				}
				ll = test = lerp(test,ll,mc.antijitter);
			}
						
			mc.ducking-=min(decel*ll,mc.ducking);
		}
		else if(mc.upsadaisy) //UNUSED (DISABLED)
		{
			//ROOM FOR IMPROVEMENT
			//TODO: modulate via upsadaisy? 
			//TODO: do this if jumping is disabled?
			//smooth non-jumping transitio/n while getting back up			
			float pow = powf(1-mc.inverting,2); 
			mc.ducking-=min(t*pow,mc.ducking);			
		}
		if(mc.upsadaisy) mc.upsadaisy = 
		mc.inverting&&mc.ducking&&mc.inverter==-1;
	}

	if(mc.peaking) 
	if(!mc.jumping&&!mc.falling) //suspend
	if(mc.action||mc.scaling||mc.holding) //scaling??
	mc.peaking = min(mc.peaking+t,1);
	else mc.peaking = max(mc.peaking-t,0);

	//2020: walk away from dashing faster?
	if(!SOM::L.swinging)
	{	
		if(!mc.dashing&&!mc.suspended&&!mc.scaling) 
		{
			//HACK: want speed of over one but drain
			//out faster than normal... not so much
			//for the transition but the time needed
			//to end the walking effect is too much
			float x = max(1,pow(mc.fastwalk,0.75f)); //sliding?

			if(som_mocap_fastwalker) //positive?
			{
				static float test = 0;
				test = mc.lerp(test,mc.Engine.push?0:2,mc.antijitter);				
				x*=1+test*mc.landspeed;
			}

			if(!som_mocap_fastwalker //negative?
			||mc.speed>0.1f||mc.resting||mc.dashing)
			{
				mc.fastwalk-=min(decel*0.2f*x,mc.fastwalk);
			}
		}
	}
	else if(mc.fastwalk<1) mc.fastwalk+=decel*0.2f;
	//EX::dbgmsg("fastwalk: %f (%f)",mc.fastwalk,mc.peaking);

	if(mc.resting) 
	if(mc.jumping||push&&(mc.ducking==2||mc.holding))
	{					
		mc.resting = max(mc.resting-t,0.000001f);
	}
	else if(rest) //NEW
	{
		//trying to smooth the new running->squatting transition
		//mc.resting = min(mc.resting+t,1);
		mc.resting = min(mc.resting+t*powf(mc.squatting,2.0f),1);
	}
			
	if(f4__1) som_mocap_ceiling(kb);
	if(f4__1) som_mocap_ceiling2(kb); //REMOVE ME?
	mc.auto_squatting = max(0,mc.auto_squatting-t); 
	if(mc.tunnel_running)	
	if(!kb[0xF3]||!mc.ceiling&&!mc.auto_squatting||!mc.inverting||mc.falling)
	{
		mc.tunnel_running = false;			
		mc.inverting = mc.inverter = 1;
	}	

	//trying to smooth the new running->squatting transition
	//mc.inverting+=t*mc.inverter;
	mc.inverting+=t*som_mocap_squat*powf(mc.squatting,0.5f)*mc.inverter;
	mc.squatting = min(mc.squatting+t*som_mocap_squat,1);
	if(mc.inverting>1) mc.inverting = 1;
	if(mc.inverting<0) mc.inverting = 0;
	if(mc.reverting) //hard to explain
	{	
		if(dashing>=1&&!mc.resting)		 
		{
			if(mc.inverting==1)
			if(mc.tunnel_running=kb[0xF3])
			mc.reverting = mc.reverter = 0;			
		}
		else //crouching within crawlspace
		{
			if(kb[0xF3])
			mc.reverting+=t*mc.reverter;
			else mc.reverting-=t*mc.reverter;		
			if(mc.reverting<=0) mc.reverting = 0;	
			if(mc.reverting>=1) mc.reverting = mc.reverter = 1;
		}		
	}
	som_mocap_ceiling2(kb); //REMOVE ME?
	//important: do after ceiling business above
	{
		//if(SOM::L.duck&&SOM::L.height) //clearance
		{
			SOM::L.height = pc->player_character_height;

			//2024: I can't believe I've never worked on this!?!
			SOM::L.height-=pc->player_character_stature-SOM::eye[1];

			if(mc.ducking>1&&!SOM::emu) //hit detection
			{
				float d3 = pc->player_character_duck3;
		
				d3*=1-mc.leading/2; d3*=(mc.ducking-1);

				SOM::L.duck = d3+SOM::L.height;	
			}
			else SOM::L.duck = SOM::L.height;

			if(pc->player_character_height2)
			SOM::L.height = //pc->player_character_height2;
			lerp(SOM::L.height,pc->player_character_height2,mc.inverting); 		
			if(SOM::L.duck>SOM::L.height) //paranoia? 
			SOM::L.duck = SOM::L.height; 
			SOM::L.height+=0.01f;

			EX::dbgmsg("duck: %f (%f)",(float)SOM::L.duck,(float)SOM::L.height);
		}
	}
	//if(SOM::L.fence) //scaling?
	{
		int look_down = EX::Joypad::gaitcode(kb[0x47]);

		//don't remove before surmountableobstacle is fixed
		//EX::dbgmsg("scaling: %f %f (%d)",mc.scaling,mc.scaling2,mc.surmounting_staging);
		//EX::dbgmsg("scaling: %f %f (%f)",mc.scaling,mc.scaling2,(float)SOM::L.pcstepladder);

		SOM::L.fence = pc->player_character_fence;
		//if(mc.scaling/*==1*/&&pc->player_character_fence2)
		if(mc.surmounting_staging&&pc->player_character_fence2)
		{
			//2017: this is temporary for a more professional
			//presentation. It should go away as the various
			//event activations become able to be cancelled
			//(eventually the Action and Activation buttons
			//should become uncoupled)
			//if(mc.scaling==1)
			if(mc.surmounting_staging==4)
			{
				//HACK: wait for one frame to see if a black
				//screen event (stopping time) occurred
				//mc.scaling = 0.99999f;
				mc.surmounting_staging = 3;
				if(!SOM::motions.aloft)
				goto HACK_mc_scaling_delayed;
			}
			//if(mc.scaling==0.99999f)
			if(mc.surmounting_staging==3)
			{	
				if(SOM::black<SOM::frame-8 //Black screen?
				//2020: creating artificial climbing effect?
				/*&&(mc.ducking+t>0.5f||mc.peaking||SOM::motions.aloft)*/)
				{
					mc.surmounting_staging = 2;

			SOM::L.fence = pc->player_character_fence2;

			//NOTE: this is needed to jump over Moratheia's fences
			//HACK: this height is subtracted from jumping because 
			//it started to feel weird after some upgrade I forget
			if(SOM::motions.aloft)
			{
				SOM::L.fence+=EX_INI_NICE_JUMP;
			}

					//NEW: having problems with convex walls...
					//I think the clinging function is keeping
					//fully dilated radius from reaching beyond
					//the walls
					//COULD MODIFY THIS TO INCREASE THE RISK OF 
					//FALLING should a climbing attempt fail
					SOM::motions.clinging*=0.9f; //0.95
				}
				else mc.surmounting_staging = 0;
			}
			else if(mc.surmounting_staging==2)
			{
				if(SOM::L.pcstep||mc.ceiling)
				{
					mc.surmounting_staging = 1;
				
					float summit = SOM::L.pcstepladder;
					bool tall = summit>pc->player_character_inseam+0.01f;
					//float upper = pc->player_character_stature-pc->player_character_fence2;
					//bool full = summit>pc->player_character_fence2-upper;

					if(SOM::L.pcstep)
					{
						mc.scaling = 1;
					
						if(!mc.bounce) //2020
						{
							mc.bounce = 0.000001f;
						}

						//2020: this needs better detection/cancelation logic
						if(mc.gaits_max_unboosted<3
						||mc.bent>=0.01f //do throughout climbing? (below)
						||mc.gaits_max_unboosted<=4
						&&mc.gaits_max_residual<100&&mc.gaits_max==mc.gaits_max2)
						{
							//I think this system can satisfy a need for a way
							//to sit down on a chair or anything, so it needs 
							//to work even with small things like a bucket
							//(a body double would have to deduce the sitting
							//behavior somehow, unless a manual system appaers)
							//(organic systems seem more elegant/natural to me)
							//if(full)
							if(!SOM::motions.aloft) //ignoring jumping/falling
							mc.inverter = 1; //2020
						}
					}
					
					//REMINDER: basically how this works is climbing is
					//slow unless it needs to be fast. fast climbing is
					//bumpy over small platforms

					if(SOM::motions.aloft
					||tall&&!mc.ceiling&&SOM::L.pcstep)
					{
						//TODO: variable speed scaling?
						//if(mc.gaits_max_unboosted>4)
						mc.scaling2 = mc.scaling; //1
					}

					goto HACK_mc_scaling_delayed;
				}
				else //mc.surmounting_staging = 0;
				{
					//HACK: got stuck bouncing between
					//2 and 4 if jumping on crawlspace
					if(!mc.landing_2017)
					mc.surmounting_staging = 0;
				}
			}				
			else if(mc.surmounting_staging==1&&!mc.scaling)
			{
				mc.surmounting_staging = 0;
			}
		}
		//SOM::L.fence+=0.01f; //0.51 //delayed...			

		if(/*mc.scaling&&*/!SOM::L.pcstep) //GETTING STUCK?
		{
			mc.scaling = max(0,mc.scaling-t); //impede

			if(mc.scaling2) mc.scaling2 = mc.scaling; //HACK
		}
			HACK_mc_scaling_delayed:

			SOM::L.fence+=0.01f; //0.51		


		//there's currently a glitch if inversion doesn't 
		//start at the bottom (does it affect jumping?)
		if(SOM::L.pcstep&&mc.scaling&&mc.bent&&look_down>1)
		{
			//TODO: variable speed scaling?
			//if(!mc.scaling2)
			if(SOM::L.pcstepladder>pc->player_character_fence)
			mc.inverter = 1; //2020
		}
	}
	
	//2020: can't be left crouching after leaving menus?
	//if(mc.Jumpstart||mc.action&&!kb[0xF3]&&!mc.holding)
	if(mc.Jumpstart||!mc.action||!kb[0xF3]&&!mc.holding)
	{	
		rest = false; if(!mc.Jumpstart) mc.resting = 0;		
	}
	else if(mc.ducking>1&&mc.action||mc.holding) //arrest
	{
		if(!mc.Engine.Analog.braking) //2020
		{
			if(mc.squatting<1)
			{
				//NOTE: I guess this only makes sense if "resting"
				//is nonzero below so it's only pushing the anchor

				float div = 8*mc.squatting;
				kb[0x4B] = EX::Joypad::analog_divide(kb[0x4B],div);
				kb[0x4D] = EX::Joypad::analog_divide(kb[0x4D],div);
				kb[0x4C] = EX::Joypad::analog_divide(kb[0x4C],div);
				kb[0x50] = EX::Joypad::analog_divide(kb[0x50],div);
			}
			else kb[0x4B] = kb[0x4D] = kb[0x4C] = kb[0x50] = 0;
		}
	}

	int tvh = tvh_ms-33*2; //match jump leeway?
	extern float som_MDL_arm_fps; //0.06f

	int turbo = mc.attack;
	int turbo2 = mc.attack2;
	//2020: swapping so shift
	//is left button
	hold(mc.attack2,kb[0x2A]); //DIK_LSHIFT
	hold(mc.attack,kb[0x1D]); //DIK_LCONTROL
	if(mc.attack||mc.attack2||mc.hud_defer||mc.hud_defer2)
	{
		if(SOM::event //fleeing?
		||SOM::motions.tick-mc.hud_event<150)
		{
			//if(mc.action-max(mc.attack,mc.attack2)<tvh_ms) //2021
			if(SOM::event<tvh_ms) //2021
			mc.hud_cancel = true; 
		}		
		else for(int i=1;i<256;i++) if(kb[i]) switch(i)
		{
		default: mc.hud_cancel = true; 
		som_mocap_Ctrl_Shift_cancel_attacks(); break;
		case 0x39: case 0xF3: //action?
		case 0x2A: case 0x1D: //attack
		//movement
		case 0x47: case 0x49: case 0x4B: 
		case 0x4C: case 0x4D: case 0x50: 
		case 0x4F: case 0x51: 
		//missing?
		case 0x48: continue; //END

		//WTH? stuck unregistered DIK code???
		//Control+Pause (Break?)
		case 0xC6: continue; 
		}
		if(mc.hud_cancel) mc.hud_defer = mc.hud_defer2 = 0;
	}
	else mc.hud_cancel = false; if(!mc.hud_cancel)
	{
		if(turbo&&!mc.attack&&turbo<tvh) //same as below
		if(op->do_st?SOM::gauge:SOM::compass)
		{
			//if(!mc.hud_defer)
			if(mc.hud_defer2)
			{
				//to reset or not to reset?
				mc.hud_defer2 = max(SOM::motions.tick-50,mc.hud_defer2);
				mc.hud_defer = mc.hud_defer2;
			}
			else if(mc.attack2||turbo2)
			{
				mc.hud_defer = SOM::motions.tick;

				som_mocap_Ctrl_Shift_cancel_attacks();
			}
		}		
		if(turbo2&&!mc.attack2&&turbo2<tvh) //same as above
		if(SOM::gauge)
		{	
			//if(!mc.hud_defer2)
			if(mc.hud_defer)
			{
				//to reset or not to reset?
				mc.hud_defer = max(SOM::motions.tick-50,mc.hud_defer);
				mc.hud_defer2 = mc.hud_defer;
			}
			else if(mc.attack||turbo)
			{
				mc.hud_defer2 = SOM::motions.tick; 

				som_mocap_Ctrl_Shift_cancel_attacks();
			}
		}	
	}
	//2021: have to delay this to prevent false-positives
	//when the damn trigger inputs take longer to kick in
	if(mc.hud_defer||mc.hud_defer2)
	if(!kb[0xF3]&&!kb[0x39]||SOM::event>=tvh)
	{
		if(SOM::motions.tick-(mc.hud_defer|mc.hud_defer2)>=99)
		{
			mc.hold_cancel = true;

			if(mc.hud_defer&&mc.hud_defer2)
			{
				SOM::showCompass = SOM::showGauge = 
				SOM::showCompass&&SOM::showGauge?false:true;
			}
			else if(mc.hud_defer)
			{
				SOM::showCompass = !SOM::showCompass;
			}
			else if(mc.hud_defer2)
			{
				SOM::showGauge = !SOM::showGauge;
			}
			mc.hud_defer = mc.hud_defer2 = 0;
		}
	}
	else mc.hud_defer = mc.hud_defer2 = 0; //cancel

	int guard_lock = 0; //goto
		
	if(mc.attack3)
	{
		turbo = mc.attack = 0;

		mc.attack3-=(int)SOM::motions.diff;

		if(mc.attack3<=0)
		{
			//HACK: plug leak since goto bypasses
			//magic button's proccessing
			kb[0x2A] = 0;

			mc.attack3 = 0; goto attack3;
		}
	}
	if(mc.magic3)
	{
		turbo2 = mc.attack2 = 0;

		mc.magic3-=(int)SOM::motions.diff;

		if(mc.magic3<=0)
		{
			mc.magic3 = 0; goto magic3;
		}
	}

	//int guard_lock; //attack3?
	{
		int a = mc.Weapon(turbo2,turbo);
		int b = mc.Shield(turbo,turbo2);		
		guard_lock = max(a,b);
	}
	
	if(!mc.attack&&!mc.attack2)
	mc.hold_cancel = false;
	if(3==SOM::motions.swing_move) //counter-attack?
	{
		//this lets you "play along" by pressing the button and
		//pressing again to hold as-if you initiated the attack
		//(which may be an involunatary reaction)
		/*SAME AS BELOW/LATER
		mc.attacked = true;*/		
		if(auto*mv=SOM::shield_or_glove(0))
		{
			//HACK: prevent chaining until beyond damage frame?
			//if((mdl.d-mv->uc[76])/som_MDL_arm_fps<-66)
			if(mdl.d<mv->uc[77])
			mc.attacked = true;
		}
		else assert(0);
	}
	else if(!mc.attack&&!turbo&&(mdl.ext.d2||!mc.attack2))
	{
		mc.attacked = false;
	}
	if(!mc.attack2&&!turbo2&&(!mc.attack||SOM::motions.swing_move))
	{
		mc.attacked2 = false;	
	}
	if(!SOM::emu)
	{		
		if(!mc.attack2&&turbo2&&!mc.magic3)
		{
			//this test is to protect against a habit of
			//pressing the button twice formed by bashing
			//after the shield is raised
			//it's also a little bit justified since the
			//magic gauge is drained
			if(2==mc.Shield.guard_bashing
			&&SOM::motions.tick<mc.Shield.guard_bashtick+tvh+100)
			{
				mc.attacked2 = true;
			}

			if(!mc.attacked2) //SAME AS BELOW
			if(guard_lock||SOM::L.swinging||mdl.d>1)
			{
				//TODO: SOM::movesets?
				int prm = SOM::L.pcequip[0];
				float ss = 0; 
				if(0==SOM::motions.swing_move)
				{
					if(prm<250&&255!=SOM::L.item_prm_file[prm].uc[136])
					{
						//sword magic?
						int prf = SOM::L.item_prm_file[prm].us[0];
						if(mdl.d>=SOM::L.item_pr2_file[prf].uc[86])
						if(mdl.d<=SOM::L.item_pr2_file[prf].uc[87])
						{
							//goto magic3;
							//2021: synchronize sword magic animation
							//NOTE: also useful to give gauge time to
							//recharge
							mc.sword_magic3 = 
							(SOM::L.item_pr2_file[prf].uc[87]-mdl.f)/2;
							assert(mc.sword_magic3>=-2);
							mc.sword_magic3 = max(1,mc.sword_magic3);
							mc.attacked2 = true;
						}
					}
					if(SOM::L.swinging) //2021
					{
						int rem = mdl.running_time(mdl.c)-mdl.d;
						ss = rem/som_MDL_arm_fps-150;
					}
				}
				float tt = guard_lock/som_MDL_arm_fps;
				
				//FIX ME: these will depend on the 
				//length of attack animation tails
				tt+=SOM::motions.swing_move?0:-150;
				//counter-attacks need long delays
				if(2==mc.Weapon.guard_bashing) tt+=250;

				if((tt=max(ss,tt))>=1)
				{
					mc.magic3 = (int)tt;
					mc.attacked2 = true; //wait a while yet
				}
			}
			if(!mc.attacked2) magic3:
			{
				kb[0x2A] = 0x80;
			}
		}
		else kb[0x2A] = 0;

		if(!mc.attack&&turbo&&!mc.attack3)
		{
			//same deal as magic3, let players play along
			//
			// +100 seems too slow for slower weapons but
			// it's what shield/magic uses... really this
			// should align with the counter-attack model
			//
			if(2==mc.Weapon.guard_bashing)
			{
				//SEEMS UNNECESSARY
				//if(SOM::motions.tick<=mc.Weapon.guard_bashtick+tvh+100)
				//mc.attacked = true;
				if(auto*mv=SOM::shield_or_glove(0))
				{
					//HACK: prevent chaining until beyond damage frame?
					//if((mdl.d-mv->uc[76])/som_MDL_arm_fps<-66)
					if(mdl.d<mv->uc[77]) mc.attacked = true;
				}
				else assert(0);
			}

			if(guard_lock&&!mc.attacked) //SAME AS ABOVE
			{
				float tt = guard_lock/som_MDL_arm_fps+150;
				if(tt>=1&&som_MDL_449d20_swing_mask(1)&1<<30)
				{
					mc.attack3 = (int)tt;
					mc.attacked = true; //wait a while yet
				}
			}
			if(!mc.attacked) //arm_ms_windup2?
			{
				if(SOM::L.swinging)
				{
					int rem = mdl.running_time(mdl.c)-mdl.d;
					mc.attack3 = (int)(rem/som_MDL_arm_fps);
				}					
				if(mc.attack3+=som_mocap_windup2())
				{				
					mc.attacked = true; //wait a while yet				
				}
			}
			if(!mc.attacked) attack3:
			{
				//REMINDER: this is required to not attack
				//while running
				if(SOM::L.pcstatus[SOM::PC::p])
				kb[0x1D] = 0x80;
				else mc.attacked = true;
			}
		}
		else kb[0x1D] = 0;		

		if(mc.attacked2) kb[0x2A] = 0;
		if(kb[0x2A]) mc.attacked2 = true;
		if(mc.attacked) kb[0x1D] = 0;
		if(kb[0x1D]) mc.attacked = true;

		if(mc.sword_magic3)
		if(mc.sword_magic3--==1)
		{
			kb[0x2A] = 0x80; //TODO: heavy sword magic?
		}
	}
	
	if(mc.running&&!mc.tunnel_running //and sneaking?
	&&mc.attack>=tvh_ms&&mc.attack2>=tvh_ms) 
	{
		mc.hold_cancel = false;

		mc.attacked = mc.attacked2 = true; //2021

		//todo: this should drain stamina
		mc.fleeing = min(1,mc.fleeing+t);
	}
	else mc.fleeing = max(0,mc.fleeing-t);

	//2021: this is just a longer tail
	if(mc.fleeing!=1)
	{
		//2022: taking damage while fast running causes
		//the shield to raise up
		if(!mc.active||!mc.attack||!mc.attack2)
		mc.fleeing2 = max(0,mc.fleeing2-t);
	}
	else mc.fleeing2 = 1;

	//EX::dbgmsg("fleeing: %f %f",mc.fleeing,mc.fleeing2);

	if(mc.sliding) //TESTING
	{
		mc.sliding = min(2,mc.sliding+t);
		if(mc.sliding==2) mc.sliding = 0;

		if(mc.sliding) mc.fastwalk = 1.75f; //letting go?
		else 
		if(SOM::event) mc.fastwalk = 1; //not letting go?
	}
	//EX::dbgmsg("fastwalk: %f %d",mc.fastwalk,SOM::event);
		
	/*if(mc.scan_for_secret_doors)
	{
		mc.action = 0; kb[0x39] = 0x80; //NEW
	}	
	else*/ if(mc.scaling||mc.action&&kb[0xF3])
	{	
		if(!pc->do_not_dash) kb[0x39] = 0;
	}		
	//EXPERIMENTAL
	{
		WORD *p = &SOM::L.pcstatus[SOM::PC::p];

		if(mc.scaling //2018: deplete gauges?
		 ||mc.recovering) //2021
		{
			//HACK: without this bias normal hits don't
			//warrant concern. I need to make very weak
			//hits do nothing if (size_t)SOM::hit>=1000
			float r = 0.5f+*p/2000.0f;

			//5x: is this the right formula?
			//thinking that it should be more
			//than crouching
			//t*tap_vs_hold is step*1000
			//int x = (int)(t*tap_vs_hold*5*max(mc.scaling,mc.recovering*r));
			/*42523d defines this as ms*3333/1000
			int x = (int)(step*5000*max(mc.scaling,mc.recovering*r));*/
			int x = SOM::motions.diff*3333/1000;
			x = (int)(x*max(mc.scaling,mc.recovering*r));
		
			//TODO: it would be nice if poison rebounded more quickly
			//can't seem to do that with p[i+2]
			for(int i=0;i<2;i++)
			{
				int defeat = p[i]-x;

				p[i]-=(WORD)min(p[i],x); p[i+2] = 0; //suppresses refill

				if(defeat<=0) (&mc.Shield)[!i].guard_dropping = -defeat;
			}
		}
		//EX::dbgmsg("p: %d (%d)",p[0],p[2]);
	}

	if(kb[0xF3]&&!mc.action)
	{
		//squat for light push?
		//if(mc.scaling) mc.inverter = -1; //EXPERIMENTAL
	}
	//NOTE: doing this earlier disables jumping
	hold(mc.action,kb[0xF3]);
	
	//if(diff) //triggers events on game start
	//if(hold(SOM::event,kb[0x39],diff)==diff)
	if(SOM::event==SOM::motions.diff&&SOM::motions.diff)
	{
		SOM::eventapped = 0; //NEW

		SOM::eventick = SOM::motions.tick; //activate

		//REFERENCE
		//waiting for tap_vs_hold to open so "holding"
		//works. there could be a good reason for this
		//if(mc.scan_for_secret_doors) SOM::event = -1; //hack

		SOM::L.controls[-4]|=0x8; kb[0x39] = 0;
	}
	else kb[0x39] = 0; //SPACE
	 
	//////////////////////////////////

	if(mc.smoothing&&!SOM::emu)	
	{	
		//2020: resting doesn't jump to zero when suspeneded
		bool resting = mc.resting&&!mc.suspended;

		struct{ float per, dir; }pad[5]; for(int i=0;i<5;i++)
		{
			//0: diagonal jumps are foreshortened without this
			pad[i].per = resting&&i<3?0:fabsf(mc.smoothed[i]);
			if(pad[i].per<0.001f){ pad[i].per = 1; pad[i].dir = 0; }
			else pad[i].dir = mc.smoothed[i]<0?-1:+1;
		}		
		kb[0x4F] = pad[3].dir<0?0x80:0; kb[0x51] = pad[3].dir>0?0x80:0;		
		kb[0x47] = pad[4].dir>0?0x80:0; kb[0x49] = pad[4].dir<0?0x80:0;
		*SOM::u*=pad[3].per;
		*SOM::v*=pad[4].per/pad[3].per;	  		
		if(!resting)
		{
		//TODO: FIND som_state_4466C0 CALL FOR Y/1 (normalization)
		kb[0x4B] = pad[0].dir<0?0x80:0; kb[0x4D] = pad[0].dir>0?0x80:0;
		//kb[0x2C] = pad[1].dir<0?0x80:0; kb[0x1E] = pad[1].dir>0?0x80:0;
		kb[0x50] = pad[2].dir<0?0x80:0; kb[0x4C] = pad[2].dir>0?0x80:0;			
		}
		SOM::u2[0] = pad[0].per/pad[2].per;
		//SOM::u2[1] = pad[1].per/pad[2].per;
		SOM::u2[2] = 1;
		
		SOM::L.dash*=pad[2].per;
		SOM::L.walk*=pad[2].per;
	}
}

int som_mocap::guard::operator()(int turbo, int turbo2)
{
	if(!som_mocap_guard) return 0; //TESTING

	auto &mdl = *SOM::L.arm_MDL;

	bool shield = this==&mc.Shield;
	int &mc_attack = shield?mc.attack:mc.attack2;
	int &mc_attack2 = shield?mc.attack2:mc.attack;
	bool&mc_attacked = shield?mc.attacked2:mc.attacked;
	int &mdl_d = shield?mdl.ext.d2:mdl.d;
	int &mdl_c = shield?mdl.ext.c2:mdl.c;
	int &mdl_f = shield?mdl.ext.f2:mdl.f;

	WORD *ps = SOM::L.pcstatus;
	WORD &m = ps[shield?SOM::PC::m:SOM::PC::p];

	int ret = 0; //guard_lock

	const int tvh_ms = EX::INI::Player()->tap_or_hold_ms_timeout;
	int tvh = tvh_ms-33*2; //match jump leeway?
	extern float som_MDL_arm_fps; //0.06f

	int _ms = tvh_ms-tvh;
	if(mc.hud_defer||mc.hud_defer2) 
	tvh+=_ms;		

	auto *mv = SOM::shield_or_glove(shield?5:0);
	
	/*static int test = 0; if(!shield)
	{
		EX::dbgmsg("test: %d",mdl_d-test);
		test = mdl_d;
	}
	EX::dbgmsg("guard: %d (%d)",mdl_d,mdl_f);*/

	if(!shield) //weapon? 
	{
		if(mdl_d<=1)
		{	
			#ifdef NDEBUG
			//do this with frequency and let Ex.ini define
			//desired range
			int todolist[SOMEX_VNUMBER<=0x1020602UL];
			#endif
			//HACK: historically the original arm resets to 1
			//somehow/somewhere it's being set to 0 (this is
			//just normalizing it so it's handled correctly)
			//NOTE: normally f seems to end up set to the last
			//frame... I wonder if standardizing it can help 
			//with a bug I can't seem to place:
			//
			// WARNING 
			// setting f to -1 does appear to solve this bug
			// but I worry this isn't the best place for it
			// to be reset... the bug has to do with the shield
			// arm animation running past its end and so it
			// crashes. it happens when swinging mid jump and
			// then executing a "bash" input mid air.. although
			// it sometimes happens under normal conditions too
			//
			mdl_d = 1; mdl_f = -1;

			if(mc.magic3&&mc.attack)
			{
				mc.attack = 1; 
					
				guard_bashing = 0; //preventing wraparound
			}
			if(3==SOM::motions.swing_move) //counter?
			{
				//this is to reward/encourage playing along with
				//the counter system by releasing the button...
				//if held down the whole time the guard will take
				//longer to come back up

				if(mc.attack>=tvh*3/2) //HACK: need a variable?
				{
					mc.attack = 1; 
					
					guard_bashing = 0; //preventing wraparound
				}
			}

			SOM::motions.swing_move = 0;
		}

		//assuming shield is processes second
		if(!mv) return 0; 
	
		if(mdl_d>1) //swinging?
		{
			if(mv->s[0]!=mdl.animation_id()) 
			{
				//YUCK: seems to work alright
				//assert(!mdl.ext.reverse_d);
				mdl.ext.reverse_d = false;
				SOM::motions.swing_move = 0; //HACK
				return 0;
			}
		}
	}
	else //shield?
	{
		auto *mv2 = SOM::shield_or_glove(0);

		//LEGACY MODE?
		//
		// TODO: this is probably undesirable if a game has any
		// move-set based equipment whatsoever... should scan 
		// item.arm I guess... but right now any dummy behavior
		// isn't in place (what would said behavior look like?)
		//
		if(!mv&&!mv2) return 0;

		//REMOVE ME
		//both are needed to not swing when pulling the shield
		//up close		
		//if(mv&&!mv2&&mc.attack>=tvh)		
		if(mv&&mc.attack>=tvh&&!SOM::motions.swing_move)
		{
			if(2!=guard_bashing)
			mc.attacked = true;
			if(!mc.hold_cancel)
			mc.hud_cancel = true;
			/*the gauge no longer works like this..
			if(!mc.hold_cancel)
			{
				//42523d defines this as ms*3333/1000
				WORD &p = SOM::L.pcstatus[SOM::PC::p];
				//int x = (int)p-step*5000;
				int x = (int)p-SOM::motions.diff*3333/1000;
				p = (WORD)max(0,x);
				*(&p+2) = 0; //suppresses refill
			}*/
		}
	}

	enum{ wraparound=3, full_release=2 };
	
	//TODO: the Menu buttons should cancel the shield probably
	//but this can wait for the quick-select system's addition
	bool guard = false;
	bool cancel = mc.attack3||SOM::L.swinging;
	if(SOM::hit&&SOM::red2==1) //critical hit?
	if(SOM::motions.shield2[1]<-0.2f) //ARBITRARY (not immediately)
	{
		cancel = true; mc.fleeing2 = 1.5f; //HACK: hold down shield
	}
	//EX::dbgmsg("hit: %f %d",SOM::motions.shield2[1],SOM::hit!=0);

	auto mdl_advance = [&](int dir)
	{
		if((shield?mdl.ext.dir2:mdl.ext.dir)!=dir)
		{
			if(shield) mdl.advance2(dir);
			if(!shield) mdl.advance(dir);
		}
	};

	if(mv&&mc_attack2>=tvh
	||mv&&guard_holding&&guard_bashing<=1)
	{
		bool atk = mc_attack2>=tvh;
		//if(atk) mc_attacked = true; //bash?
		if(atk&&!mc.hold_cancel) mc.hud_cancel = true;

		int mc_action = SOM::event; //mc.action //fleeing?

		bool up = mdl_d>1;

		cancel = true;

		//2022: I may have at one point decided running is
		//allowed but I recall planning to not allow it...
		//in which case this disables it on an empty gauge
		//(it seems reasonable to not allow it when empty)
		if(m||mc.running<1.0f)
		if(!mc.fleeing2&&!guard_dropping)
		if(!mc.hold_cancel)			
		if(!mc_action||!mc_attack||mc.resting
		||mc_action<tvh_ms+_ms&&!mc.Engine.push
		||mc_attack<tvh&&mc_attack2-mc_attack>200
		||mc_action<tvh&&mc_attack2-mc_action>200)
		if(up||!mc.Jumpstart)
		if((!mc.jumping||mc.hopping)&&!mc.scaling)
		if(!(mc_attack&&!mc_attack2))
		if(!shield||!SOM::L.swinging&&mdl.d<=1&&!mc.attack3)
		if(!shield||!SOM::motions.swing_move)
		if(shield||!mdl.ext.d2&&!mc_attack)
		if(!SOM::L.pcequip2) //equipping?
		{
			if(atk) mc_attacked = true; //bash?

			guard = true; cancel = false;			

			/*if(up) //TESTING
			{
				//300ms or maybe 150ms since arm models are sped
				//up?
				if(mdl.ending_soon2(9)) //cancel?
				{
					if(!mc.bounce&&!mc.fleeing) //extend?
					{
						mdl_d = !shield; //...
					}
				}
			}*/
			if(atk) if(!shield==mdl_d)
			{
				//REMINDER
				//just as I noticed 34 was hardcoded I looked up and saw
				//this (https://www.youtube.com/watch?v=tIj1k9fykg4) song
				//titled 34 queued up waiting to press play
				//int id = mdl.animation(34);
				int id = mdl.animation(mv->s[0]);
				if(id==-1) return 0;
				mdl_c = id;
				shield?mdl.rewind2():mdl.rewind(); //mdl_f = -1;
				mdl_d = full_release;					
				int w = 0, ww = 0;				
				if(!mc.bounce&&!mc.fleeing&&guard_bashing<=0) //extend?
				ww = 150;
				w+=(int)(som_MDL_arm_fps*(ww));
				//this is the time until the shield will be fully raised
				//guard_window = tvh+(mv->uc[86]-w)/som_MDL_arm_fps;
				guard_draining = 0;				
				//guard_window2 = 0; //???
				if(guard_bashing>0)
				{
					w = mv->uc[87];
					int rt = mdl.running_time(mdl_c);					
					ww = tvh/2*wraparound-guard_bashing;
					ww = min(150,ww);
					w+=(int)(som_MDL_arm_fps*(ww));
					//why did I subtract ww from rt???
					//this is not working with slower weapons
					//guard_window2 = tvh+(rt-w)/som_MDL_arm_fps+33;
					guard_window = tvh+(rt-w)/som_MDL_arm_fps;
				}				
				else guard_window = tvh+(mv->uc[86]-w)/som_MDL_arm_fps;
				mdl_d+=w;
				guard_raising = true;
				guard_bashing = 0;
				guard_bashing2 = false;

				//these all need to be reset here because there's a chance
				//that the shield is canceled at the same time it's raised
				guard_holding = 0;
				guard_releasing = 0;
				counter = false;

				//HACK: close gap to simplify the
				//som_hacks_Clear afterimage logic
				if(mdl_d>1) mdl.update_animation();

				if(!shield) SOM::motions.swing_move = 2; 
			}			
		}
	}
	if(guard_dropping>0) //2024
	{
		cancel = true;

		int x = SOM::motions.diff*3333/2000; //1000

		guard_dropping = max(0,guard_dropping-x);
	}

	if(mdl_d>(int)!shield) //shield?
	{
		if(cancel&&!guard_raising) 
		guard_releasing = 0;
		guard_releasing-=min(guard_releasing,(int)SOM::motions.diff);

		////wraparound?
		int rt = mdl.running_time(mdl_c)-1;
		int lo = mv->uc[86]-1, hi = mv->uc[87];
		
		//wraparound?
		//ret = mdl_d<=lo+1?lo:rt-mdl_d;
		ret = mdl_d<=lo+1||mdl_d>hi+1?lo:hi-mdl_d; //good enough???

		//NOTE: afterimage drawing can stop on +1
		//as well as 30fps mode
		//2023: speed can skip frames +1
		//bool top = mdl_d>=lo&&mdl_d<=lo+1; if(top)
		bool top = mdl_d>=lo-1&&mdl_d<=lo+1; if(top)
		{
			mdl.ext.dir = mdl.ext.dir2 = 0; //2023: advance?

			guard_raising = false;

			if(!guard_releasing) guard_draining = 0;

			if(guard_bashing2)
			{
				guard_bashing = 2; guard_bashing2 = false; 

				guard_bashtick = SOM::motions.tick;
			}
		}

		if(guard_raising||guard&&mc_attack2&&guard_releasing)
		{
			if(guard_raising)
			{
				guard_holding+=SOM::motions.diff;
			}
			if(guard_raising||guard_releasing)
			{
				guard_releasing = tvh_ms*3-guard_holding;
				//2023: this is getting stuck with speed (sometimes)
				//assert(guard_releasing>0);
				if(guard_releasing<0) guard_releasing = 0; //UNTESTED
			}
		}
			
		if(turbo2&&!mc_attack2) //releasing button?
		if(m) //running?
		{
			//sometimes it's not set???
			mc_attacked = true;				

			if(mdl_d>hi+1) guard_bashing2 = true;

			if(0==guard_bashing) //initiating press?
			{
				guard_bashing = 1;

				if(turbo2<guard_window+33)
				if(guard_raising||guard_releasing)
				{
					guard_bashing = 2;
					guard_bashtick = SOM::motions.tick;
				}
			}
			else if(mdl_d>lo-som_MDL::fps*4) //close shave
			{
				//if(1==guard_bashing) //one shot?
				assert(1==guard_bashing||2==guard_bashing);
				//HACK: just for ambiguity
				if(2!=guard_bashing)
				guard_bashtick = SOM::motions.tick;
				guard_bashing = 2;
			}

			counter = guard_bashing==2;
		}
		if(counter==1&&!shield) //save attack power?
		{
			if(guard_bashing!=2) //counter attacking?
			{
				guard_bashtick = SOM::motions.tick;
				guard_bashing = 2;

				SOM::motions.swing_move = 3; //2|1

				//mc.magic3 = 0; mc.attacked2 = false; //TESTING
			}
		}
		if(guard_bashtick==SOM::motions.tick)
		{
			ps[SOM::PC::attack_power] = m;

			if(shield) mc.gauge_bufs[2]+=m; //2024
		}

		//EX::dbgmsg("guard %d %d %d",guard_raising,guard_bashing,guard_releasing);

		if(guard_raising) //raising?
		{
			mdl_advance(1); //mdl_d+=1;

			ps[SOM::PC::attack_power] = m; //HACK: som_scene_swing
		}
		else if(guard_bashing>1)
		{
			int dir = 1; 
			if(top&&guard_bashing2)
			{
				dir = 0;
				guard_bashing = 1;
				guard_bashing2 = false;
			}

			//order is important
			if(mdl_d>lo+1) mc_attacked = false; //attack3?

			if(dir)
			{
				//mdl_d+=dir>0?1:-1; //-1??? 
				mdl_advance(1);
			}
			
			if(mdl_d>=hi-1) //order is important?
			{				
				guard_bashing = tvh/2*wraparound;
			}

			if(mdl_d>=mv->uc[76]&&mdl_d<=mv->uc[77]+4)
			{
				SOM::Attack a = SOM::L.pcattack[0];
					//ANNOYINGLY 427780 leaves a lot undefined
					a.strength = ps[SOM::PC::str];
					a.magic = ps[SOM::PC::mag];
					a.power_gauge = min(5000,ps[SOM::PC::attack_power]);
					memcpy(a.attack_origin,SOM::L.pcstate,3*sizeof(float));
					a.aim = SOM::L.pcstate[4]+pi/2;					
				//this feels wrong... probably missed some division code
				//note: 0.008726645f equals a.pie
				//
				// still, something's wrong... everything is hitting the
				// monster from everywhere (it was setting a.unknown_pc1
				// to 2???)
				//
				//a.pie = mv->s[39]*0.01745329f;
				a.pie = mv->s[39]*0.008726645f; //pi/180/2

				//FIX ME: som_game_measure_weapon?
				float len = 0; if(!shield)
				{
					auto &prm = SOM::L.item_prm_file[SOM::L.pcequip[0]];
					auto &pr2 = SOM::L.item_pr2_file[prm.s[0]];
					len = pr2.f[20];
				}
				len = max(0.25f,len);
				//EXTENSION?
				//0.75 is about the length of arm.mdl 
				//a.radius = mv->f[20]+SOM::L.hitbox2; //som.logic.cpp
				a.radius = (len+0.75f)*mv->f[20];
				int e = SOM::L.pcequip[shield?5:0]; //SOM_PRM_extend_items
				if(e>=250) e = SOM::L.pcequip[3];
				BYTE *dmg = SOM::L.item_prm_file[e].uc+300;
				BYTE *dmg3 = dmg+20;
				if(e!=SOM::L.pcequip[0]) dmg+=20;
				
				float bonus3 = 1;
				if(dmg!=dmg3&&!dmg3[0]&&!dmg3[1]&&!dmg3[2])
				{
					//heavy attack bonus? a little goes a long
					//way with the damage formula
					//memcpy(a.damage,dmg,8);
					bonus3 = 112/100.0f; dmg3 = dmg;
				}				
				
				for(int i=8;i-->0;)	
				a.damage[i] = (BYTE)min(255,i<3?dmg3[i]*bonus3:dmg[i]);
					
				//a.unknown_pc1 = 2; //HACK: normally 1
				assert(a.unknown_pc1==1);
				((void(__cdecl*)(void*))0x4041d0)(&a);
			}
			else if(mdl_d>hi)
			{
				ps[SOM::PC::attack_power] = m; //HACK: som_scene_swing
			}
		}			
		else if(!guard_releasing) //lowering?
		{
			guard_holding = guard_bashing = 0;

			//HACK: it'd be better to unset mc_attacked
			//but it isn't working
			if(turbo2&&!mc_attack2) //releasing button?
			{
				if(!cancel) //FIX ME? when was first release?
				{
					mc_attacked = true;

					//NOTE: this is just inputting a normal
					//attack as the shield is being lowered
					int &atk3 = shield?mc.magic3:mc.attack3;			
					if(!atk3) atk3 = mdl_d/som_MDL_arm_fps;
					if(!shield) atk3+=som_mocap_windup2();
				}
			}

			//mdl_d-=1; assert(mdl_d<=lo);
			mdl_advance(-1);

			ps[SOM::PC::attack_power] = m; //HACK: som_scene_swing

			//keep CP adjustment alive?
			if(mdl_d<=full_release+1)
			if(shield?mdl.d>1:mdl.ext.d2)
			{
				//assert(mdl_d+1>=mdl_f); 
				
				mdl_d = mdl_f; 

				//I've seen his fail twice. that's a problem
				//assert(mdl_d>full_release);
			}			
		}
			
		//if(mdl_d==hi||mdl_d==hi+1)
		if(mdl_d>=hi-1&&mdl_d<=hi+1)
		{
			mdl_d = !shield; //finish up?

			//delay shield bashing?
			if(som_MDL_449d20_swing_mask(1)&1<<30)
			mc.fleeing2 = 1; //HACK 
		}
		if(mdl_d>=rt||mdl_d<=full_release+1) 
		{
			mdl_d = mdl_d>=rt?lo:!shield;
			
			shield?mdl.rewind2():mdl.rewind(); //mdl_f = -1; 

			//HACK: close gap to simplify the
			//som_hacks_Clear afterimage logic
			mdl.update_animation();
		}

		if(SOM::frame-SOM::swing>15) //2023
		{
			int snd = mv->uc[73];
			//if(mdl_d==snd||mdl_d+1==snd)
			if(mdl_d>=snd-1&&mdl_d<=snd+4)
			{
				SOM::swing = SOM::frame;

				extern int SomEx_npc; //PC
				SomEx_npc = 12288;
				//I think 0 is silence or ignored 
				//but this is for som_game_nothing
				//if(snd=mv->us[37]) SOM::se(snd,mv->c[85]);
				DWORD sp = SOM::shield_or_glove_sound_and_pitch(shield?5:0);
				if(sp>>16) SOM::se(sp>>16,(char&)sp);
				SomEx_npc = -1;
			}
		}
	
		//NOTE: must be done after counter-attacking
		if(guard_draining<guard_window-tvh+33)
		{
			guard_draining+=SOM::motions.diff;

			heavy: //HACK

			//42523d defines this as ms*3333/1000			
			//int x = (int)m-step*5000;
			int x = (int)m-SOM::motions.diff*3333/1000;
			m = (WORD)max(0,x);
			*(&m+2) = 0; //suppresses refill
		}
		else if(!shield&&guard_bashing>1)
		{
			goto heavy; //HACK: makeshift heavy penalty
		}
		
		if(mdl_d>=lo+1&&mdl_d<=hi)
		{
			m = *(&m+2) = 0; //suppresses refill
		}
	}
	else if(guard_bashing>0)
	{
		guard_bashing-=(int)SOM::motions.diff;
	
		if(guard_bashing<0) guard_bashing = 0;
	}
	if(counter) counter--;

	//atk2 is for pressing both buttons... for single-arm weapons
	//it makes sense to briefly raise the shield before attacking
	//however for two-handed the shield must be lowered before it
	//can begin its animation... either way masking it some helps
	if(!mc_attack!=!turbo)
	guard_sloshing = SOM::motions.tick;
	//NOTE: how this looks can depend a lot on how fast shields 
	//pulls back... this setting is very high, which is good I
	//think but works because the pull back is very slow, which
	//I guess wasn't always the case
	int atk = 33*8;
	atk = atk-min((unsigned)atk,SOM::motions.tick-guard_sloshing);
	if(mc_attack) atk = max(0,mc_attack-atk);
	bool atk2 = guard&&atk&&guard_bashing!=2;
	if(!shield) 
	{		
		//HACK: prevent animation from resetting?
		//mdl.ext.reverse_d = mdl.f>mdl.d;
		mdl.ext.reverse_d = mdl.ext.dir<0; //2023

		guard_fusion = atk2?2:guard; //fusing...
	}
	else //there can be only one
	{
		switch(mc.Weapon.guard_fusion) //fusing?
		{
		case 2: atk2 = true; case 1: guard = true;
		}

		guard_sloshing2 = lerp(guard_sloshing2,atk2,mc.antijitter*0.25f);
		float slosh = atk2?guard_sloshing2:1-guard_sloshing2;
		//float cling = SOM::motions.clinging2;
		float cling;
		{
			enum{ test=1 };
			float clead = sqrtf(mc.leading2);
			float clear = SOM::motions.arm_clear;
			float rate = 0.5f;
			float pz = mc.pushahead[1]; if(pz>0)
			{
				//saturate?
				if(0) pz*=1.2f;
				//you'd think this cancels out, but it seems to work??
				else pz+=1-fabsf(mc.pushahead[0]);
				pz = min(1,pz);
			}
			else
			{
				//NOTE: arm_clear makes this useless... it used to give
				//some tactile feedback when backing into things but it
				//would do so walking up stairs (backward) if arm_clear
				//was negated
				pz*=-0.5f; //...				

				//NOTE: rate is mainly to deal with stairs (walking up
				//backwards)
				rate*=sqrtf(mc.landspeed); 
				clear+=(1-clear)*-min(0,mc.pushahead[1]);
			}
			cling = pz*clead*SOM::motions.clinging*sqrtf(clear);
				static float cling2 = 0; //cornering?
				cling2 = lerp(cling2,cling,mc.antijitter*(0.1f+rate*clead));
			cling = mc.dashing*cling+(1-mc.dashing)*cling2;
			//EX::dbgmsg("cling: %f (%f) %f",cling,SOM::motions.shield,rate);
		}
		float rate = guard_bashing==2?2:0.25f;
		rate*=slosh*slosh+2*powf(cling,0.25f);
		//NOTE: 1 is too much in VR where it clips the near plane unless you
		//lean back a little bit... it may be because the plane is fixed for
		//the entire frame
		float l = DDRAW::xr?0.8f:1;
		float h = guard?atk2?l:0.125f:0;
		float guard2 = lerp(h,l,cling);
		SOM::motions.shield = lerp(SOM::motions.shield,guard2,mc.antijitter*rate);
		//EX::dbgmsg("shield: %f",SOM::motions.shield);
	}
	return ret; //guard_lock
}
extern void som_mocap_counter_attack()
{
	if(som_mocap.Weapon.guard_releasing) 
	if(som_mocap.Weapon.guard_bashing<=1)
	{
		som_mocap.Weapon.counter = 8+SOM::rng()%12; //frames
	}
}

static float som_mocap_d() //initial
{
	float d = //vertical jumping height
	EX::INI::Player()->player_character_fence4
	-EX::INI::Player()->player_character_fence3;

	//2021: hack since jumps feel floaty since
	//I last modified som_game_402070_timeGetTime
	//I have no idea why this makes such a difference
	//or if it generalizes to settings besides 2.3->2.28
	d-=EX_INI_NICE_JUMP;

	//jumping without do_g would look like a spike
	if(d<=0||!EX::INI::Option()->do_g) return 0; 

	//http://en.wikipedia.org/wik/Trajectory

	return d; //sqrtf(d*2*EX::INI::Detail()->g);
}
static float som_mocap_v() //initial
{
	//http://en.wikipedia.org/wik/Trajectory
	return sqrtf(som_mocap_d()*2*EX::INI::Detail()->g);
}

extern void som_game_volume_level(int, int &_2);
extern void som_game_pitch_to_frequency(int, int &_3);
extern void som_mocap_headwind(bool on)
{
	EX::INI::Sample se;

	int snd; if(&se->headwind_identifier)
	{
		float f = se->headwind_identifier();
		snd = EX::isNaN(f)?0:(int)f; 
	}
	else snd = 1000; if(snd)
	{
		auto &mc = som_mocap;

		auto &sb = SOM::L.snd_bank[snd];
		auto **i = (DSOUND::IDirectSoundBuffer**)sb.sb;
		if(!i||!*i)
		{
			if(!on) return;

			//2023: init/play wind sound effect (0 volume)
			SOM::se_looping = 1; SOM::se(snd,0,0); //-10000
			SOM::se_looping = 0;			
		}
		if(i&&*i) 
		{
			//float speed = mc.speed+powf(mc.speed-mc.speed2,2);
			float speed = max(mc.speed2,mc.speed-mc.speed2);				

			float f = speed/8;
		//	float g = 1-powf(max(0,1-f),2); //2
			float g = powf(min(1,f),0.333f);

			float aw = se->headwind_ambient_wind_effect;
			static float aw2 = aw; //just in case
			aw = aw2 = mc.lerp(aw2,aw,mc.antijitter);

			//YUCK: reshaping to boost to footstep levels
			float h = 0.75f-aw*0.25f;
			g = (g*g)*h+(1-g*g)*(1-h); g*=1/h;

			//som_game_pitch_to_frequency returns 2 always???
		//	int _3 = mc.analogbob*3;
			int _3 = 0;
			int _2 = (on?1-g:1)*-10000;

			som_game_volume_level(snd,_2);
			som_game_pitch_to_frequency(snd,_3);

			_3 = (int)(mc.analogbob*3*(_3?_3:1));

		//	EX::dbgmsg("wind: %f=>%f/%f (%d/%d)",f,g,mc.analogbob,_2,(int)(mc.analogbob*4));

			DSOUND::fade_delay(*i,_2,_3);
		}
	}
}

//REMOVE ME? 
static float som_mocap_elevator = 0;
void som_mocap::engine::prolog::operator()(float step)
{
	assert(step); //if(!step) return; //hack??
	
	EX::INI::Player pc; EX::INI::Adjust ad;
	EX::INI::Option op; EX::INI::Detail dt;

	bool do_g = op->do_g; assert(!SOM::g==!do_g);

	//trying to include a vexing jumping glitch
	//that occurs right on the edge of MHM polygons
	//maybe not helping but rules out a possibility
	static const float gzero = 0.000001f;
	//bool was_paused = SOM::g&&!*SOM::g; //testing
	bool was_paused = SOM::g&&*SOM::g<=gzero;

	//assuming falling off map
	//if(freefall>(SOM::f3?3:10)) return SOM::die(); 
	{
		//2020: try to recover from badly timed or
		//glitch jump to be both like a life-like
		//scramble to recover and to make jumping
		//less stressful... but Jumpstart needs to
		//register a jump input for it to kick in

		int f3 = 10;
		if(mc.temporary_f3 //catches at start :(
		&&SOM::motions.tick-mc.temporary_f3<=1500
		&&SOM::L.pcstate[1]-SOM::f3state[1]>0.25f)
		f3 = 1;
		else if(SOM::f3) f3 = 3;

		if(freefall>f3||f3==1) return SOM::die(f3!=10); 
	}

	/*MOVED into "this"*/
	//we measure the step before last
	//static float SOM::xyz_past[4] = {0,0,0,0}; //xyz

	//// in the past ///////
			
	float velocity[3]; 

	//if(SOM::warped<SOM::frame-1)   
	if(SOM::xyz_step) for(int i=0;i<3;i++) //zero divide
	{
		velocity[i] = (SOM::xyz[i]-SOM::xyz_past[i])/SOM::xyz_step;
	}

	//hack: imposing speed limit of 300m/s
	if(Somvector::map(velocity).length<3>()<300)
	Somvector::map(SOM::doppler).copy<3>(velocity);
	
	mc.speed2=SOM::doppler[0]*SOM::doppler[0];
	mc.speed2+=SOM::doppler[2]*SOM::doppler[2];

	if(SOM::doppler[1])
	{
		mc.speed = sqrtf(mc.speed2+SOM::doppler[1]*SOM::doppler[1]);
		mc.speed2 = sqrtf(mc.speed2);
	}
	else mc.speed = mc.speed2 = sqrtf(mc.speed2); 

	som_mocap_headwind(true); //2023: do_wind //how fast?
	
	//0.33333 is SOM's value for SOM::g
	float g = do_g?*SOM::g:0.333333f;		
	//2017: make jumping off ledges much easier
	if(mc.jumping==0.000001f) if(air>0&&do_g)
	{
		g = *SOM::g = 0.333333f; 

		air = freefall = 0; 
	}
	else if(freefall)
	{
		mc.jumping = 0; //fail (elegantly)

		//2020: some jumps off ledges are being cut short, not sure
		//why... so far not this
		//if(EX::debug) MessageBeep(-1);
	}

	/*MOVED into "this"*/
	//static bool jumping = false; //hack

	float v = som_mocap_v(); //sqrt(d*g)
	
	static float vtheta = 0; //2020: locking in

	if(mc.jumping<=0.000001f) vtheta = 0; if(!vtheta)
	{	
		vtheta = v*1; //1: sin(90deg)		

		//http://en.wikipedia.org/wiki/Range_of_a_projectile
									  
		///*2020
		//
		// this is resulting in regular jumps that are 25% 
		// of the height. I'm sure it's to simulate longer
		// jumps, but it needs a review
		//
		// resting? I think the stealth jump must be setting
		// resting?
		//
		if(!mc.resting //vtheta*=0.707106f; //0.7: sin(45deg)
		 &&!mc.holding_xyzt[3])
		{	
			//support digital/discretize jumps
			//vtheta*=sinf(lerp(pi/4,pi/2,mc.running));
		//	vtheta*=sinf(pi/(mc.running?2:4));

			//2020: 4 is the short jump... there's an element
			//of chance (vtheta was varying for the duration)
			//hopping is covered by jump_distance to slow it
			//down
			if(!mc.running) vtheta*=mc.hopping?0.75f:0.9f;
		}//*/		

		//2020: shape sidways/backward jumps?
		//
		// conceptually jumping diagonally is like 
		// turning in that direction since that's 
		// how the walk effect is programmed, so it
		// covers the full distance... but I'm not 
		// sure backward jump should be the same
		//
		// note: jumping backward feels overly dramatic
		// this makes it feel the same as going forward
		// while it looks and feels nice it gives the
		// impression it's going the same distance, but
		// it's not; which is natural since jumping 
		// is easier to do going forward; and looking 
		// nice is good too. it's just a bit deceptive
		//
		//I designed this for limiting on all directions but
		//ultimately felt this sucked the freedom out of 360
		//jumping so now it just applies to back-jumps and 
		//smoothly spreads that across back+sideways jumps
		if(!mc.hopping&&mc.backwards>0.1f)
		if(op->do_dash&&mc.jumping==0.000001f)
		{
			float f = 0.05f; //doubled below

			int z = mc.Jumpstart.dpad[3].y;
			int x = max(mc.Jumpstart.dpad[0].y,mc.Jumpstart.dpad[1].y);			
			//assert(mc.Jumpstart.dpad[2].y>z);

			float dist; if(x)
			{
				float len = sqrtf(x*x+z*z);

				dist = x/len-z/len;		
				dist = f-dist*f;
			}
			else dist = f+f; if(dist) 
			{
				vtheta*=1-dist; //ARBITRARY
			}
		}
	}
	//EX::dbgmsg("vtheta: %f (%f)",vtheta,mc.backwards);
		
	//float t = vtheta?vtheta/dt->g:0.5f; //time to apex
	float t = 0.5f; if(vtheta)
	{
		t = vtheta/dt->g;
		
		//HACK? the old way of canceling 
		//twice on the first frame is too bumpy now so this
		//is meant to add a little extra height to compensate
		//for the first frame's loss, and I'm doubling it too
		//just to ring the bell for the set jumping height
		//since it tends to get lost in the apex or something
		t+=18*0.333333f*step/dt->g;
	}
	
	if(!v) //&&mc.jumping) //disabled
	{
		//hack: this is being used to disable
		//jumping and ducking together for now

		mc.jumping = 0; //hack: suppress
		mc.resting = 0; //important: yikes
	}

	float y = 0; //moved to outer scope

	//0.01f: this is an arbitrarily small value
	if(jumping&&SOM::xyz_past[1]>SOM::xyz[1]+0.01f)
	{
		mc.jumping = 1; //assuming hit ceiling 

		//if(EX::debug) MessageBeep(-1);
	}
	else if(jumping&&mc.scaling&&SOM::ladder>SOM::frame-2) 
	{
		mc.jumping = 1; //don't want super jumps

		//if(EX::debug) MessageBeep(-1);
	}
	else if(mc.jumping&&mc.jumping<1)
	{	 				
		float t1 = mc.jumping*t, t2 = t1+step;

		mc.jumping+=step/t; 
				
		SOM::motions.aloft+=!jumping?2:1;

		float y1 = vtheta*t1-t1*t1*dt->g/2;		
		float y2 = vtheta*t2-t2*t2*dt->g/2;		
										
		/*float*/ y = y2-y1; //jumping velocity
	
		//minimize error (moved below)
		//SOM::xyz[1]+=y; SOM::err[1]+=y;
	
		//bumpy???
		//2020: FOR SOME REASON THIS IS NO LONGER SO
		//(MAY DEPEND ON EXTENSION SETTINGS?)
		if(0) 
		{
			//2020: I'm not seeing the glitch any longer
			//what was it exactly?

			//cancel SOM's pseudo gravity
			//first frame is different 
			som_mocap_elevator = y+g*18*step*(jumping?1:2);
		}
		else //REMINDER: 0.001f seems unnecessary 
		{
			som_mocap_elevator = y+g*18*step; //+0.001f;
		}

			///FIX ME
			//
			// this is an ongoing problem... it happens
			// only when jumping off platforms that I know 
			// of
			//
			// NOTE: the glitch happens when pausing in
			// midair, meaning it may be a timing thing
			//			
			static float pred = 0;

			float dy = 0; if(jumping)
			{
				if(SOM::L.pcstate[1]<pred)
				{	
					float dy = pred-SOM::L.pcstate[1];
				
					if(dy>y&&mc.jumping<0.9f)
					if(SOM::frame-SOM::bopped<10) //ceiling?
					{
						y1 = SOM::L.pcstate[1];
						y2 = SOM::L.pcstate2[1];

						//this is a problem jump... I guess the 
						//clipper is probably holding it down...
						//not sure how to proceed


						//TODO? THIS IS A GOOD INDICATOR OF A 
						//FAILED JUMP... IT MIGHT BE AN IDEA TO
						//INTELLIGENTLY HELP PLAYERS TO RECOVER
						//WITHOUT RUNNING OFF THE EDGE OF A CLIFF
						//(Note: F3 already has recovery code here
						//somewhere)
						mc.temporary_f3 = SOM::motions.tick; //TESTING

						//if(EX::debug) //sorry :( 
						MessageBeep(-1); 
					}

					//NOTE: this has a minor contribution that
					//might result in fuller jump heights when
					//leaking occurs
					//(it feels lumpier to me, jumps in general
					//feel like they're going to high up, but 
					//if the height is reduced they won't go as
					//far, so it's a catch-22. part of that may
					//be dashing effect draining out in midair)
					//NOTE: lumpiness goes away if you reengage
					//the direction inputs on landing, so most
					//of it seems to be "error"
					if(1) som_mocap_elevator+=dy; //y+=dy;
					else dy = 0; //...
				}
			}

			//NOTE: might want to keep this even after the bug
			//is fixed... it produces numbers, even though it's
			//hard to say jumps are smoother or higher with this
			pred = SOM::L.pcstate[1]+y+dy;

		if(!jumping)
		{
			jumping = true;

			//2020: TESTING MORE GLITCH THEORIES
			//assert(!SOM::L.pcstep);
			//SOM::L.pcstep = 0; SOM::L.pcstepladder = 0;
		}		
	}

	/*MOVED into "this"*/
	//static float diving = 0;
	//static float freefall = 0;
	
	float dive = SOM::L.pcstate[1]-diving;

	diving = SOM::L.pcstate[1];		
	
	//the first time is different??
	//Reminder: this is derived from som_db/rt constants
	float tol = freefall?0.6f:0.9f;
	
	bool landing = jumping&&mc.jumping>=1;

	if(mc.scaling) air = 0; //2020

	if(!was_paused) //2020
	if(SOM::xyz_step) //zero divide 
	if(landing||-dive/SOM::xyz_step>g*18*tol) //18: convert from SOM units to m/s
	{
		//2020: trying to prevent/narrow jump glitch
		//(so far none with 0.1f. maybe go lower?)
		//jumping = false;
		if(mc.jumping>0.1f||freefall||!mc.jumping) 
		jumping = false;
		if(!jumping) if(SOM::L.f4) //F4?
		{			   
			if(!freefall) //obsolete?
			{					
				if(dive>-0.5f) //warping?
				{
					//hack: rewind first time slice
					//todo: correct for our velocity

					diving-=dive;

					if(do_g&&!SOM::emu) //!
					{	
						//SOM::L.pcstate[1]-=dive;
						//SOM::L.pcstate2[1]-=dive;
						som_mocap_elevator = -dive;
					}					

					/*2020: testing using this on regular platforms
					//REMOVE ME?
					//Restricting this to using slopes as ramps, because the way
					//the clipper works, the slope ends at its summit because it
					//is below the PC's position, but that's not so for downhill.
					if(SOM::doppler[1]>0)*/
					{
						//REMOVE ME?? air could be a useful jumping aid??
						//NEW/EXPERIMENTAL
						//2017: Trying to give more time to jump off edges of things

						//if(mc.speed2>0.01f) //large values here glitch on climbing
						if(mc.speed2>0.00001f) //zero divide 
						{
							//2020: there is a problem with occasionally getting stuck on
							//the lips of platforms I don't understand
							//(this tends to be around 0.6 without *2)
							//2020: not dashing could use some air? (no)
							#if 1
							//2 looks funny. 1.5 is plenty of time for
							//a blind jump. I still get bad jumps on 2
							float x = sqrt(mc.landspeed*mc.dashing)*1.5f; //2
							#else //NOPE
							//can't use this going down stairs without having some idea 
							//of the geometry underneath
							float x = sqrt(mc.landspeed*(0.5f+mc.dashing));
							#endif
							//REMOVE ME? (NO, SEEMS SIGNIFICANT)							
							//if(air<=0) airy = SOM::L.pcstate[1]+0.001f;
							//+0.001f sometimes has an unpleasant speed change effect?
							if(air<=0) airy = max(SOM::L.pcstate[1],SOM::L.pcstate2[1])+0.0001f;

							//2020: seems this needs to relax somehow, did something change?
							//air = !mc.jumping?*SOM::stool/mc.speed2*(SOM::slope?1:0):0;
							air = *SOM::stool/mc.speed2*x;
						}
						else air = 0;
					}
					/*2020: haven't got stuck on a platform lip since changing
					//to 0.000001f, knock on wood... going to relax the jump
					//defeat logic in Jumpstart when in "air"...
					//FIX ME: the clipper needs to summit slopes (?)
					//air = mc.scaling||mc.jumping?0:min(0.25f,air);*/
					air = mc.scaling||mc.jumping>0.000001f?0:min(0.25f,air);
					//make sure nonzero? why are jumps failing?
					if(air>0) air+=step; 
				}												   
				else SOM::xyz_step = 0; //warp
			}			
			freefall+=SOM::xyz_step;
		}
		else freefall = 0;		
	}
	else if(!som_clipc_slipping2) //2020: unpausing? f4?
	if(freefall||mc.jumping>=1) if(air<=0) //>=1: warped???
	{
		//freefall = 0;		
		float t = freefall; freefall = 0; //SHADOWING		
		{
			if(do_g) //gravity?
			{
				//http://en.wikipedia.org/wiki/Equations_for_a_falling_body
				astride = 0.5f*dt->g*(t*t);
				SOM::motions.fallimpact = dt->g*t;
			}
			else //classical contants (g=1/3)
			{
				astride = t*g*18; //t*6
				SOM::motions.fallimpact = g*18;	//6
			}
			//2020: some "save" logic is necessary so events don't play
			//a plod sound effect when scaling back onto platforms, but
			//should it be gravity based? and what's a good metric? 1.5
			//works well in my KF2 project, but the drop doesn't really
			//feel like 1 meter, wich is needed to make a landing sound
			//(ASSUMING CHARACTER HEIGHT, ETC!)
			if(!SOM::L.pcstep||astride-alighting>=1)
			{
				//TODO: SUBTRACT alighting FROM fallimpact

				SOM::motions.fallheight = astride-alighting; //WORD
			}
			else SOM::motions.fallimpact = 0; //HACK: just undoing work

			if(SOM::motions.fallimpact>3||SOM::motions.fallheight>1)
			{
				SOM::rumble();
			}
		}

		if(mc.jumping>=1) mc.jumping = mc.resting = 0;

		//landing_2017
		//this is an edge case, where falling out of a crawlspace, into
		//another crawlspace, mid stretching out.
		//Reminder: "reverter" is getting stuck. maybe it's not correct
		if(SOM::L.pcstep)
		if(mc.inverting&&mc.inverter==-1)
		{
			mc.inverter = 1;
		}

		//som_mocap_epilog
		//mc.falling = 0;		
		mc.bounce = 0.000001f; //transfer to epilog
	}

	//EX::dbgmsg("air: %f %f",air,mc.jumping);

	if(do_g) 
	if(!SOM::emu&&(freefall
	//2020: trying things... I really feel this extra test solves 
	//the main problem (airy, etc.) jumping off edges of platforms
	//but I did hit one bad jump that felt like it should have gone
	//through... but I hit many sloppy jumps too, so I'd say this is
	//more resilient (might be the 0 divide makes a difference below?)
	||air>=step&&mc.jumping<=0.000001f)) //test
	{	
		if((air-=step)>=0) //2017: zero gravity!
		{
			//TODO: MAYBE GRADUALLY BUILD GRAVITY LIKE BELOW? 
			//(it makes implicit g/t/v calculation impossible)

			//this is simulating a disc like flat platforms have but
			//for slopes. slopes go up,up,up, until they don't touch
			//*SOM::g = 0;
			*SOM::g = gzero; //maybe can zero divide?
			freefall-=SOM::xyz_step;
			freefall = max(freefall,0.001f*18); //0.000001f;
		}
		else if(0) //if(freefall<1&&!mc.jumping) //variable g?
		{
			//NOTE: "alighting" was the source of the glitch
			//but maybe this is useful if it models how people
			//actually walk versus just plummeting in a vacuum

			//2020: I feel like it's hitting the ground like
			//a sack of potatoes today... I can't think of
			//anything that might have changed... this is
			//designed to start the fall more gradually
			//(maybe "air" has altered my expectations
			//but g is too abrupt on short platforms if so)
			*SOM::g = dt->g*powf(freefall,1.25f)/18; 
		}
		else *SOM::g = dt->g*freefall/18; 

		if(*SOM::g<gzero) //2022
		{
			*SOM::g = gzero; assert(0); //DEBUGGING
		}
	}
	//else if(SOM::emu||!mc.scaling||!SOM::L.pcstep)
	else if(SOM::emu||!(mc.scaling2||SOM::L.pcstep)||!f4__1
	||mc.jumping) //new
	{
		//REMINDER: lessening is reducing the bounce effect
		//(upon landing)
		*SOM::g = 0.333333f; 
	}
	else //*SOM::g = 0.333333f/1.5f; //trying out (.222222)
	{
		//trying slower climbing when moving gradually
		float up = 0.222222f;
		if(!mc.scaling2&&mc.landspeed<0.5f)
		up/=3; //2 
		*SOM::g = up; //lerp(*SOM::g,up,step);
	}

	//// in the present ////
	
	/*if(step)*/ SOM::xyz_step = step;	
	/*if(step)*/ Somvector::map(SOM::xyz_past).copy<3>(SOM::xyz);	

	//if(1||!EX::debug) //moved: minimize error
	{
		//NOTE: this is 0 when falling?

		SOM::xyz[1]+=y; SOM::err[1]+=y;

		static float max = -1000;
		if(SOM::err[1]>max) max = SOM::err[1];
		EX::dbgmsg("y: %f (%f)",y,max); //12.77091 //12.775837 
	}
	//else SOM::xyz[1] = SOM::L.pcstate[1];

	if(!y&&!freefall&&!mc.bounce) 
	if(fabsf(SOM::L.pcstate[1]-SOM::L.pcstate2[1])<0.000001f)	
	if(SOM::motions.tick-mc.temporary_f3>1500)
	memcpy(SOM::f3state,SOM::L.pcstate2,sizeof(SOM::f3state));	
	
	//todo: player_character_stride?
	const float stride = pc->player_character_shape;		
	
	/*MOVED into "this"*/
	//.1f: SOM clipping constant
	//static float falling = 0.1f;	
	//geez: the plot thickens...
	//static float speeding = 0, alighting = 0; 		
	//static bool fallen = false;

	float catchup = 0; //dbgmsg

	if(!freefall) 
	{
		falling = 0.1f;
		speeding = alighting = air = 0;
	}
	else if(air<=0) //down you go
	{
		catchup = dive;

		if(falling>=0) //hack: 0.1f???
		{
			catchup = min(falling+dive,0);						
			fallen = false; 
		}
		
		mc.falling = falling+=dive;

		//EX::dbgmsg("falling %f (%f)",mc.falling,vtheta);

		if(!mc.jumping) //alighting? (requires analog?)
		if(!mc.dashing) //2020
		{
			//increasing from .001 to .01 to .05 for slopes
			float squat = pc->player_character_inseam+0.05f;

			//2017: inseam defaults to 0.5 now. Plus +0.01f?
			//if(!squat) squat = pc->player_character_fence+0.01f;

			if(alighting<squat)
			{
				float t = !do_g?squat/6:sqrtf(squat*2/dt->g);

				speeding = max(speeding,mc.speed2);

				if(t*speeding<stride) alighting = min(-falling,squat);
			} 
			
			mc.falling+=alighting;

			//2020: this is causing a very bad "sack of potatoes" glitch
			//on higher platforms but is needed for stairs... I guess it
			//is causing error build ubep between the clipper and analog
			//world positions, but it's really hard to tell. the effect
			//is counterintuitive since this is resisting being pulled
			//down, but the glitch pulls down instantly
			//if(mc.falling>=0) catchup = 0;
			{
				//REMINDER: "air" can't apply to walking for this to work
				//otherwise it's pretty flexible

				//WARNING! this actually works very well in KF2 for walking
				//down stairs at full speed. it looks like fast walking down
				//stairs
				float t = min(1,max(0,-falling-0.1f)/squat);
				catchup*=powf(t,2);
			}
		}
		//2020: causing glitches
		//if(mc.falling>-0.01f) mc.falling = 0;
		//assert(mc.falling<=0);
		mc.falling = min(0,mc.falling);
		
		//REMINDER: fallen remains after falling
		if(mc.falling) 
		{
			fallen = true; mc.lept = SOM::motions.tick;

			SOM::eventick = 0; //cancel object subtitle (a la jumping)
		}

		if(0==SOM::motions.aloft) 
		{
			if(fallen) SOM::motions.aloft = 2;
		}
		else SOM::motions.aloft++;

		//NEW: stretch out as falling		
		//if(fallen) mc.inverter = -1; 
		//NEW: glitches if under a ceiling
		//but also if about to climb into a ceiling
		//(which cannot be detected but does require a push)
		//(ALSO: GIVING THE PLAYER DIRECT CONTROL OVER THIS STRECHING OUT)
		if(fallen)
		if(!mc.ceiling&&!mc.Engine.push
		//HACK: landing_2017. would prefer SOM::motions.clinging here, but
		//it doesn't currently test if the wall is being pushed up against 
		//or if it's behind, doing the pushing
		||mc.landing_2017
		||mc.dashing) //2020: getting out of seat?
		{
			if(!mc.hopping) mc.inverter = -1; //testing
		}
		
	//NOTE: y is 0 above when falling? is this
	//the negative version of y?

		//important: for propper effect
		SOM::xyz[1]+=catchup; SOM::err[1]+=catchup;	
	}
	//EX::dbgmsg("catchup: %f (%f)",catchup,y);

	//static float test = 0; 
	//if(mc.falling) test = mc.falling;
	//EX::dbgmsg("g: %f (%f)",*SOM::g,test);	

	if(SOM::stool) if(SOM::emu) 
	{
		SOM::L.shape = *SOM::stool = stride;
	}
	else if(!freefall||mc.falling/*fallen is past tense*/) if(air<=0)
	{	
		//assert(air<=0); //is stool/shape changing midair? //hitting?

		int careful = 30;
		if(mc.landspeed<=0.5f&&mc.landspeed>0)
		{
			careful = 2*75*(0.5f-mc.landspeed); //HACK! slow climbing?
		}
		bool climbing = SOM::L.pcstep||SOM::ladder>SOM::frame-careful;	
		
		//2017: the stool/shape is no longer returning to the original
		//size when walking. probably mc.leading was originally an error
		//between the digital and analog cameras, which was once very high
		//
		//there is now a delicate balance here between regular stairs and 
		//SOM's cyclopean stairs, or needing to climb back up in general. it
		//also affects recovering from falls. this is all a bit hocus-pocus now
		//EX::dbgmsg("tripping??? %f %f %f",*SOM::L.shape,*SOM::stool,SOM::motions.clinging);

		if(SOM::L.shape<stride
		&&!mc.leaning&&!climbing
		&&!mc.falling //Or !freefall?
		&&mc.landspeed>(astride<SOM::L.fence?0.1f:0))
		{
			//Reminder: the ultimate goal of this is to not let the shape be 
			//stuck for long (or rather far) in the reduced version of itself
			SOM::L.shape = min(stride,SOM::L.shape+stride*mc.landspeed*step*2);			
		}
		else //This "else" is special...
		{		
			//not sure why, but "else" is making it so after placing one leg
			//off a ledge, it's possible to get back up onto it without ugly
			//bumping away from the ledge
			//WOOPS: this seems to be smoothing the start of the fall, which
			//works wonders for 3D platforming and seems more in keeping with
			//the trend of smoothing out the clipping system

			if(!mc.scaling&&!mc.surmounting_staging)
			if(float slip=pc->player_character_slip) 
			{
				//HACK/2018: enable lower gaits to climb stairs... adding
				//cleaned up version of mc.leading
				float l = mc.leading2;
				if(l>0.0005f) l = powf(l,0.333333f);
				//EX::dbgmsg("shape %f (%f,%f)",l,mc.leading2,*SOM::stool);

				//Reminder: mc.leading is quite different from mc.leaning
				//increasing 2 (or 2.x) decreases resistance on stairways
				//maybe it should be separated from player_character_slip? 
				//
				//OBSOLETE? mc.falling may have no effect since the next
				//block's code overrides the stool radius to help slopes
				float grow = mc.falling?1.0f:l*2+mc.speed2/slip;				

				//2020: squatting is also interpreted as sitting
				//so it's more stable on the edge of the ledge
				//TODO: it'd be nice if no pressure was required 
				//to keep from falling while climbing... it'd 
				//have to be stateful (using inv*inv decreases it
				//for short climbs)
				//REMINDER: a much larger size is needed than I'd
				//imagined. smaller values aren't even noticeable
				//(one interesting thing is doing the getting up 
				//maneuver gets out of the seat when on the edge)
				float seat = max(0.01f,SOM::L.hitbox)*mc.inverting*mc.inverting;
				seat = powf(seat,1+mc.landspeed*mc.landspeed);

				float before = *SOM::stool;
				//2021: seconds_per_frame is pushing tt over 1 causing
				//a negative final radius
				const float yy = max(seat,min(grow,1)*stride); //2021
				const float tt = min(1,step*16); //2021
				//taper as climbing onto the summit
				float after = lerp(*SOM::stool,yy,tt)+0.001f;
																			
				//2017: don't stumble back down while climbing up stairs
				if(before<after||!climbing) 
				*SOM::stool = after;

				if(*SOM::stool>stride) *SOM::stool = stride; 
			}	
			else *SOM::stool = stride; //NEW
			else assert(*SOM::stool==stride);
		}
		
		if(fallen&&SOM::L.shape<stride) //ouch!
		{
			SOM::L.shape = //tumble over as best as we can
			lerp(SOM::L.shape,stride,step*16)+0.001f; 
			if(SOM::L.shape>stride) SOM::L.shape = stride;

			//2017: THIS IS for falling off slopes down the
			//side of cliffs. the slope doesn't have a depth
			//so if the "stool" grows it can cut into a space
			//below the slope, and so appear to have fallen on
			//the slope 
			//making it half the size is probably not necessary
			//(actually it is. or at least less than 0.75f)
			//it kind of simulates poor footing when falling. it
			//means you must land on a large foothold...
			//BUT NOT REALLY, because this only applies until the 
			//shape returns to full size (this block requires it)
			//and climbing while falling is still in the cards
			//
			//Reminder: this is contradicting the above blocks
			//use of fallen to grow the "stool" shape
			if(!mc.scaling&&!mc.surmounting_staging)
			*SOM::stool = max(0.001f,SOM::L.shape/2); //extension?
		}

		SOM::L.shape = max(SOM::L.shape,*SOM::stool);
	}

	astride-=mc.speed2*step/2;

	if(!SOM::emu) //limiting F4 look to +/- 90 degrees
	if(fabsf(SOM::L.pcstate[3])>pi/2) 
	{
		SOM::L.pcstate[3] = SOM::L.pcstate[3]<0?-pi/2:pi/2; 
	}
	
	if(1) //if(1&&EX::debug) //EXPERIMENTAL
	{
		//was going to use 1 but for now that's too bumpy on 0.25m staircases
		//and anyway 2 is probably a better, more conservative, value. I wonder
		//if a sinusoid function could work to stretch the effect out over meters
		//"lerp" IS A SHORT-ORDER HACK
		const float dist = 1.5f;

		//HACK: because falling instantly becomes 0 antijitter/2 is jarring at 
		//the extreme end (i.e. falling a significant distance)
		static float hack = 0; 
		hack = lerp(hack,mc.nosedive<-0.75f?0.2f:0.5f,mc.antijitter*0.75f);

		//modulating by 1-mc.bounce works surprisingly well for blending with bounce
		//FIX ME (HACK)
		//pow(2) is simply to not linger at the tail end since it's noticibly stilted 
		mc.nosedive = //2020
		//lerp(mc.nosedive,min(0,max(-1,mc.falling)),mc.antijitter/2);		
		//lerp(mc.nosedive,min(0,max(-1,falling)),mc.antijitter/2*(1-mc.bounce));		
		lerp(mc.nosedive,-pow(max(0,min(1,-falling/dist)),2),mc.antijitter*hack*(1-mc.bounce));
		//EX::dbgmsg("nosedive: %f (%f)",mc.nosedive,hack);
	}
}

void som_mocap::engine::epilog::operator()(float step)
{	
	assert(step); //if(!step) return; //hack??

	EX::INI::Player pc;

	//// elevation ///////

	if(!SOM::emu) //paranoia
	{
		SOM::L.pcstate[1]+=som_mocap_elevator;
		SOM::L.pcstate2[1]+=som_mocap_elevator;			
		som_mocap_elevator = 0; //!

		//2020: desparate patching //REMOVE ME? (NO, SEEMS SIGNIFICANT)
		if(mc.Engine.Prolog.air>0)
		{
			SOM::L.pcstate[1] = max(SOM::L.pcstate[1],mc.Engine.Prolog.airy);
			SOM::L.pcstate2[1] = max(SOM::L.pcstate2[1],mc.Engine.Prolog.airy);
		}
	}

	//// bouncing ////////
	
	/*MOVED into "this"*/
	//const int waves_s = 4;
	//static float waves[waves_s][2] = {{0},{0},{0},{0}};
														 
	if(mc.bounce==0.000001f) //hack
	{	
		bool scaling = mc.scaling==1; //2020

		float d = som_mocap_d(); //jump height		
		if(!d) d = pc->player_character_fence; //hack (fallback)

		//NEW/HACK? player_character_dip isn't bottoming out 
		//I think maybe it's because Prolog::falling is from
		//0.1???
		d-=0.1f; //or vtheta?

		if(mc.touchdown_scorecard>1 //hmmm: I guess so?
		 &&mc.falling<-max(d,pc->player_character_inseam)-0.01f)
		{			
			mc.touchdown_scorecard = 1; //hit the ground hard
		}
		if(!scaling)
		som_mocap_footstep_soundeffect(); //NEW

		/*2020: scaling affects duck animation now
		//LOOKS AWFULLY SUSPICIOUS?!
		if(SOM::ladder<SOM::frame-8) //slowing down
		if(mc.touchdown_scorecard!=1) mc.scaling = 0; //???	
		*/

		/*2020: decreasing for variable player_character_dip/dip2?
		//increasing for 30 zoom
		float dab = 0.15f; //0.125f; //minimum dip*/
		float dab = 0.125f;

			int bounce_path = 0; //one is unpleasant?

		float x = 1, y = 1;	
		//2017: try to reduce bounce on human scale stairs
		//NOTE: small bounces can be unpleasant without AA
		//REMINDER: WALKING AT TOP SPEED TYPICALLY TUMBLES
		//DOWN STAIRS, AND SO WILL FAIL SOM::motions.aloft
		//#if 0 //ALTERNATIVE?
		//float speed_filtered = EX::kmph*1000/60/60;
		//if(speed_filtered<SOM::walk*1.1f&&mc.speed2>mc.speed/2)
		//#else

		if(scaling) //2020
		{
			/*is x a height? don't think so
			//NOTE: pcstepladder doesn't quite make it to the top 
			//x = pow(SOM::L.pcstepladder+pc->player_character_fence,3);*/
			x = pow(SOM::L.pcstepladder/pc->player_character_fence2,3);
			x = min(1,x);
			y = 0.2f/(1+x);
			dab = 0.5f; mc.touchdown_scorecard = 1;

			bounce_path = 1;

			mc.scaling_bouncing = true;
			for(int i=waves_s;i-->0;) if(waves[i][0])
			mc.scaling_bouncing = false;
		}
		else if(!SOM::motions.aloft&&mc.speed2<=SOM::walk)
		//#endif
		{			
			//EX::dbgmsg("bounce %f %f (%d)",mc.speed2,SOM::walk,SOM::frame);

			//HACK: problem with slow climbing stairs
			//(should probably not be here)
			if(SOM::ladder>SOM::frame-60) //x = 0.25f;
			x = 0;				
			else x = 0.25f;
			//this is a problem for walking sideways... the landing
			//is hard as if falling a shorter distance
			//y/=max(1,5*(1-mc.landspeed*mc.landspeed));
			y/=max(1,5*(1-powf(mc.gaits_max_unboosted/7.0f,2)));

			bounce_path = 2;
		}
		else //HACK: landing feels flat-footed 
		{			
			//I think this stuff needs to be a percentage of FOV
			//(NOTE: it is kind of now, but this is a percentage too?)
			//dab*=1.2f;

			//2020: increasing for variable player_character_dip/dip2
			//REMINDER: landing is 0.999998 unless jumping
			if(1==mc.landing&&mc.touchdown_scorecard>1||mc.hopping)
			{
				float t = min(SOM::zoom2()-30,32)/32.0f; //VR?
				dab = lerp(0.275f,0.375f,t); 
				//values under 0.5 glitch on small amplitudes, so the
				//"dab" values must be increased to compensate
				y = 0.4f;

				//2020: it's jarring if landing into a walk
				if(!mc.action)
				{
					y*=1.5f; dab*=2;

					bounce_path = 3;
				}
				else bounce_path = 4;
			}	 
			else //bounce_path = 5;
			{
				bounce_path = 5; //sometimes falling is 25% ???
			}
		}

		//EX::dbgmsg("bounce path: %d (%f %f)",bounce_path,mc.falling,d);

		if(x&&d>0) for(int i=0;i<waves_s;i++) if(!waves[i][0])
		{
			//2020: decreasing for variable player_character_dip/dip2
			x*=mc.touchdown_scorecard>1?dab:max(dab,min(-mc.falling/d,1));

			//2021: prolong raising shield
			if(x>0.125f&&!mc.hopping) //x>dab
			{
				float time = SOM::L.arm_MDL->ending_soon2(9)?0.66666f:1.33333f;
				mc.fleeing2 = max(time,mc.fleeing2); //HACK 
			}

			waves[i][0] = 1;		
			waves[i][1] = x;
			waves[i][2] = y;
			break;
		}

		mc.falling = 0; //!
	}	

	float hang10 = 0; //totally tubular

	for(int i=0;i<waves_s;i++) if(waves[i][0])
	{
		float amp = waves[i][1];       					
		float rate = waves[i][2]; //1
		rate/=(pc->tap_or_hold_ms_timeout/500);
		rate = sqrtf(rate); //NEW: 1.3333 becomes unpleasant
		//caution: this formula is sensitive to refresh rate limits
		float time = waves[i][0]-=step*min(2.75f,rate*(3.0f-amp*2));

		//TODO? SHOULD THE WAVES INTERFERE? THEY'RE JUST SUMMED
		//RIGHT NOW. I'M NOT SURE WHAT'S CORRECT OR IS PLEASANT

		if(waves[i][0]>0) //Note: time*time below
		{
			//using the trough of the sine wave			
			//hang10+=amp*cosf(time*time*2.0f*pi)-amp;
			hang10+=amp*sinf(pi+time*time*pi);
		}
		else waves[i][0] = 0;
	}

	mc.bounce = -hang10;	

	if(mc.bounce<=0.000001f) //hack
	{
		mc.bounce = 0; //touchdown_scorecard = 1; //reset

		mc.scaling_bouncing = false;
	}

	///// miscellaneous ////////

	extern void //hack: keep SOM::pov uptodate
	som_state_pov_vector_from_xyz3_and_pov3();
	som_state_pov_vector_from_xyz3_and_pov3();
}

float som_mocap::engine::analog::suspend::operator()(BYTE *kb, float out)
{	
	/*MOVED into "this"*/
	//static float suspended = 0; 

	if(!kb||!mc.falling&&!mc.bounce&&!mc.jumping)
	{
		Somvector::series(EX::Affects[2].analog,0,0,0,0); //F5

		suspended = 0; 
		
		bouncing = false; //2020: unstick, preventing bending???

		return out;
	}
	
	const BYTE bts[] = {0x4B,0x4C,0x4D,0x50,0x4F,0x51}; //4562 13
	
	/*MOVED into "this"*/
	//static bool resting = false;
	//static bool bouncing = false;

	//experimentally determined
	const float stick = 0.25001f;

	/*MOVED into "this"*/
	//static float xx = 0, zz = 0, touchdown;
		
	if(mc.bounce==0.000001f||!suspended
	 ||mc.jumping-mc.falling<suspended)
	{	
		bouncing = mc.bounce==0.000001f;

		float x = 0, z = 0, u = 0;

		//2017: these are very sensitive since they
		//determine the direction of jumps
		//would rather fix this in Jumpstart
		/*if(0) for(int i=0;i<sizeof(bts);i++) 
		{
			float gait = 
			EX::Joypad::analog_scale(kb[bts[i]]);

			switch(bts[i])
			{
			case 0x4D: x+=gait; break; //6
			case 0x4B: x-=gait; break; //4
			case 0x4C: z+=gait; break; //5
			case 0x50: z-=gait; break; //2
			case 0x51: u+=gait; break; //3
			case 0x4F: u-=gait; break; //1
			}
		}
		else //EXPERIMENTAL*/
		{
			x = EX::Joypad::analog_scale(kb[0x4D]);
			float _x = EX::Joypad::analog_scale(kb[0x4B]);
			z = EX::Joypad::analog_scale(kb[0x4C]);
			float _z = EX::Joypad::analog_scale(kb[0x50]);
			u = EX::Joypad::analog_scale(kb[0x51]);
			u-=EX::Joypad::analog_scale(kb[0x4F]);				
			if(!mc.jumping)
			{
				x-=_x; z-=_z;
			}
			else //Jumping/canceling out?
			{
				//HACK: don't let the lesser direction dominate
				//NOTE: I think Jumpstart may be the only source
				//of antagonistic inputs
				if(x<_x) x = -_x; if(z<_z) z = -_z;
			}
		}
		
		if(!bouncing)
		{
			//SOM wants to invert these
			//TODO: standardize on som_logic_406ab0 
			float c = cosf(-SOM::uvw[1]);
			float s = sinf(-SOM::uvw[1]); 

			//rotate x/z about the left handed Y axis
			xx = x; zz = z; x = x*c+z*s; z = xx*-s+z*c;	
		}
		else //score the landing
		{
			xx-=x; zz-=z; touchdown = sqrtf(xx*xx+zz*zz); 
		}
		
		if(resting=!x&&!z)
		{
			//EX::Affects[1].position[0] = 0;
			//EX::Affects[1].position[2] = 0;		
		}		  
		else if(mc.jumping)
		{		
			jumping = true; //2020

			//hack: leaning jump
			if(mc.resting||mc.holding_xyzt[3]||mc.hopping)
			{
				mc.dashing = 1; 
			}
			  
			float len = sqrtf(x*x+z*z); assert(len);

			//http://en.wikipedia.org/wiki/Trajectory_of_a_projectile

		//	float v = som_mocap_v()*0.707106f; //cos(45deg)
			
			//jumps sometime go the wrong way???
			//EX::dbgmsg("len %f (%f---%f) %x",len,xx,zz,kb[0x4D]|kb[0x4B]);

			if(1) //m_s here is no longer maintainable
			{
				x/=len; z/=len; //just normalize
			}
			/*m_s was passed 
			else if(v>len*m_s) //jump or dash velocity?
			{
				//Reminder: long jumpers' speed	  
				//does not increase upon take off				
				v = 1/len*(v/(len*m_s)); x*=v; z*=v;				
			}*/
		}
		
		if(bouncing) //if(kb[0xF3]) //DASH				
		{
			mc.landing = 0;

			if(touchdown<stick&&!resting)
			{
				//2020: try to distinguish between jumping in epilog?
				//mc.landing = 1; //hit the ground running
				mc.landing = jumping?1:0.9999998f;

				mc.touchdown_scorecard = 10; //hack
			}
			else mc.touchdown_scorecard = 1; //ditto

			jumping = false;
		}

		if(0&&bouncing) //testing
		{
			static int test = 0; 			
			//results: seems to be an unexplained sample error of exactly 0.125
			EX::dbgmsg("landing %d: %f (%d)",test++,touchdown,(int)resting);
		}		

		EX::Affects[2].position[0] = x;
		EX::Affects[2].position[2] = z;
		EX::Affects[2].position[3] = u;		
	}
		
	//0: land with feet planted
	enum{ conserve_momentum=0 };

	if(bouncing) 
	{
		float trip = 0;

		if(resting) //stick it
		{
			trip = mc.bounce*7;
		}
		else if(touchdown>stick)
		{				
			float stumble = 
			1-fabsf(mc.bounce-0.5f)*2;										   
			trip = stumble*7+2;
			out*=stumble;
		}

		//hack: player disadvantage
		if(trip) for(size_t i=0;i<sizeof(bts);i++) 
		{
			kb[bts[i]] = EX::Joypad::analog_divide(kb[bts[i]],trip);
		}
	}		
	else //suspending
	{			
		//NOTE: is falling not negative?
		assert(mc.falling<=0); 
		suspended = mc.jumping-mc.falling; 
		//INVESTIGATE ME
		//assert(suspended);
		if(!suspended) 
		{
			//this is a false positive when jumping (why?)
			//need to test falling
			//suspended = 0.000001f;
			return out;
		}

		for(size_t i=sizeof(bts);i-->0;) kb[bts[i]] = 0;

		//NOTE: there's a problem with EX::Affects[2] when
		//turning since the resolution on the rotated gait
		//isn't so good
		EX::Affects[2].simulate(kb,256); 

		if(conserve_momentum) return 0; 
	}
	return out;
}

static float som_mocap_am_relax(float step)
{
	return step*EX::INI::Analog()->error_impedance;
}
static float som_mocap_am_relax2(float &a, float &b, float step)
{
	//NOTE: Ex_input_ratchet_pedals=0 SEEMED TO BE THE MAIN
	//SOURCE OF PROBLEMS BUT KEEPING THIS SEEMS TO HELP TOO
	if(1)
	//2021: I'm worried this could be a source of lumpiness
	//when slowly turning at first gait (note: I feel like 
	//it's probably an error that there's any discrepency
	//in the first place and I'm not 100% positive this is
	//helping... it's snappy but not when fast turning IT 
	//ADDS SOME DRUNKENESS WHICH MAY EVEN BE KIND OF COOL?
	if(som_mocap.smoothing)
	{
		static const float flr = 0.05f;
		float diff = fabsf(som_mocap.radians(a,b));
		float diff2 = min(1,diff*10);
		step*=flr+(1-flr)*diff2;
		//EX::dbgmsg("relax2: %f (%f)",diff,diff2);
	}
	return som_mocap.slerp(a,b,EX::INI::Analog()->error_impedance2*step);
}
static float som_mocap_am_relax3(float a, float b, float step)
{
	/*End key has to work :(
	//(I haven't noticed a problem on this axis in any case)
	//2021: see som_mocap_am_relax2 note
	if(som_mocap.smoothing)
	{
		static const float flr = 0.05f;
		float diff = fabsf(a-b);
		diff = min(1,diff*10);
		step*=flr+(1-flr)*diff;
		EX::dbgmsg("relax3: %f (%f)",diff,(1-flr)*diff-flr);
	}*/
	return som_mocap.lerp(a,b,EX::INI::Analog()->error_impedance2*step);
}										
static float som_mocap_am_reshape(float clearance)
{
	//1.75f: account for camera lens
	//float znear = SOM::fov[2]*1.75f; 
	//2020: this is no longer cutting it. is it the depth-buffer
	//partition? or the soft-clip logic??
	//2.1 works with 0.1 z-near plane, but that's a large amount
	//of the clip shape (0.21) so I've had som_game_ReadFile cut
	//the clip plane down to 0.01. it seems to help so that 0.15
	//works
	//NOTE: the problem is more from pushing into things sideways
	//(especially when dashing)
	float z = 0.15f; assert(SOM::fov[2]==0.01f);

	float den = EX::INI::Player()->player_character_shape-z; 

	return den>0?clearance/den:1; //zero divide 
}

//todo: solve for error algebraically somehow??
extern float som_mocap_am_calibrate(float walk, float step, float cal=som_mocap_am_relax(1))
{
	//int fps = DDRAW::refreshrate; //2021
	//const float step = 1.0f/(fps?fps:60); //fps
	const float m_s_step = walk*step; 
	
	float out = 0, err = 0; do //simulation of lean into a wall
	{	
		//out = err; err-=err*som_mocap_am_relax(step); err+=m_s_step;
		out = err; err-=err*cal*step; err+=m_s_step;

	}while(fabsf(out-err)>0.00003f); return out;	
}
			
//NEW: setup SOM::u, including extra precision beyond SHORT
static void som_mocap_seturn(SHORT &t, float to, float u=1)
{
	t = to+0.5f; if(!SOM::u) return;
	*SOM::u = float(t)/to*u; *SOM::v = 1/u; //NEW
}
static void som_mocap_refill(WORD &p, int loss)
{	
	//00425162 66 01 15 24 1D 9C 01 add         word ptr ds:[19C1D24h],dx
	int d = SOM::motions.diff;
	//int o = SOM::L.pcstatus[SOM::PC::o]; //weight ???			
	int oa = *(&p+2);

	bool m = &p==SOM::L.pcstatus+SOM::PC::m; //2024

	DWORD sm = som_MDL_449d20_swing_mask(0);

	bool dbl = SOM::motions.swing_move&&3==3&sm>>29;

	int gain2 = 0; if(m) //shield?
	{
		//like som_scene_425d50?
		int r = SOM::L.pcequip[0];
		int h = SOM::L.pcequip[3]; //TODO: add h/2 to oa (0x427855) ////
		int l = SOM::L.pcequip[5];
		auto *prm = &SOM::L.item_prm_file;
		float wt = h!=0xff?prm[h].f[74]/2:0; 
		float wt2 = wt;
	//	if(r!=h&&r!=0xff) wt+=prm[r].f[74];
		if(l!=h&&l!=0xff) wt2+=prm[l].f[74];

		//0042784F D9 81 28 01 00 00    fld         dword ptr [ecx+128h]  
		//00427855 D8 0D 68 84 45 00    fmul        dword ptr ds:[458468h]  
		//0042785B D8 05 B4 82 45 00    fadd        dword ptr ds:[4582B4h]  
		wt2 = wt2*10+1;

		gain2 = 42*SOM::L.pcstatus[SOM::PC::str]/wt2*d*30/1000;
	}
	int gain = 42*SOM::L.pcstatus[m?SOM::PC::mag:SOM::PC::str]/oa*d*30/1000;

	if(loss) //sliding?
	{
		//00425253 66 29 15 24 1D 9C 01 sub         word ptr ds:[19C1D24h],dx 
		//int loss = d*3333/1000;
		loss*=d*3333/1000;

		//HACK: the loss will be subtracted
		p = max(0,min(p+gain+loss,5000+loss));
	}
	else //2024: recover from running or bashing?
	{
		loss = gain; gain = d*3333/1000;

		gain = min(gain,som_mocap.gauge_bufs[m]);
		gain2 = min(gain2,som_mocap.gauge_bufs[2]);

		som_mocap.gauge_bufs[m]-=gain;
		som_mocap.gauge_bufs[2]-=gain2;		

		gain+=gain2;

		if(loss>gain) 
		{
			loss = gain; gain = 0;
		}
			
		//HACK: the loss will be added
		p = max(0,min(p+gain-loss,5000));
	}
}
void som_mocap::engine::analog::operator()(float step, BYTE *kb)
{	
	EX::INI::Player pc; EX::INI::Option op; 
	EX::INI::Adjust ad; EX::INI::Detail dt; EX::INI::Analog am;	

	const int tvh_ms = pc->tap_or_hold_ms_timeout;
	const float tap_vs_hold = tvh_ms;
	const float t = step*1000/tap_vs_hold;

	//1: extrapolation is probably not good?
	//const float antijitter = min(1,step*15);
	const float antijitter = mc.antijitter;

	float f4 = f4__1?1:3, f4_2 = f4__1?1:1.5;  //f4/2
	
	if(SOM::emu)
	{	
		SOM::L.walk = SOM::walk; SOM::L.dash = SOM::dash; 
		som_mocap_seturn(SOM::L.turn,SOM::_turn/pi*180); //NEW
	}	
	if(SOM::u) *SOM::u = 1; //u-turn
	
	if(SOM::emu) return; //! (emulation) /////////////////////
	
	float power = horsepower;
	if(!SOM::motions.aloft)
	{
		WORD &p = SOM::L.pcstatus[SOM::PC::p];
		WORD &m = SOM::L.pcstatus[SOM::PC::m];
						
		//restore power when sliding
		//NOTE: code for subtracting power is at 425253
		//code for adding power is at 425162
		//if(mc.sliding&&!SOM::L.swinging)
		int slid = mc.sliding&&!SOM::L.swinging;
		{
			//it looks weird to not refill the magic gauge
			if(slid||mc.dashing<1&&p) som_mocap_refill(p,slid);
			if(slid||mc.dashing<1&&m) som_mocap_refill(m,slid);
		}
		if(mc.dashing>=1)
		{
			int regain = SOM::motions.diff*3333/1000;
			mc.gauge_bufs[0]+=min(p,regain);
			mc.gauge_bufs[1]+=min(m,regain);
		}

		power = mc.fleeing*5000; 
		power+=p;	
		if(power>5000) power = 5000;		

		if(mc.landing||mc.scaling) //NEW: fleeing
		{
			//power = lerp(power,horsepower,mc.landing);
			power = lerp(power,horsepower,min(1,mc.landing+mc.scaling));
		}
		else horsepower = power; 
	}
	else //2024: refill running/shield bash if jumping?
	{
		som_mocap_refill(SOM::L.pcstatus[SOM::PC::m],0);
		som_mocap_refill(SOM::L.pcstatus[SOM::PC::p],0);
	}
	EX::dbgmsg("dashing: %f",mc.dashing);
	
	const float running2 = 1-powf(power/5000,2);
	float dashing2 = mc.dashing*mc.dashing, walking2 = 1-dashing2;
	if(SOM::hit){ dashing2 = 1; walking2 = 0; }
			
	//2020: "sliding" goes off on a tangent in circling
	//maybe only fast turning should be restricted this
	//way? the dash speed has to be slowed down too. it
	//makes it fun when circling
	float slide = cosf(pi*mc.sliding);
	//float slide1 = slide/2+0.5f;
	//2022: slide2 doesn't sum to 1???
	//
	// WARNING: this makes _DASH=4.5 wrong but it works
	// to synchronize circling with in use Ex.ini files
	//
	float slide2 = slide/4+0.5f; //dip to 50% in middle
	//2022: fix?
	//slide2*=1+0.3333f*(mc.jogging+mc.running); 
	slide2*=1+0.3333f*(mc.jogging+(mc.running+mc.fleeing)*0.5f); 

	float fast = SOM::walk*0.3f;
	const float walk = SOM::walk+fast*mc.fastwalk; //2020
	const float dash_walk = SOM::dash/walk; //SOM::walk
	float m_s = lerp(walk,SOM::dash,dashing2*slide2); //SOM::walk
	float r_s = SOM::_turn*walking2+dash_walk*SOM::_turn*dashing2;
		
	//NEW: slowdown 20% if squat walking and not dashing
	float slowdown = 1-mc.squat*0.2f;
	
	m_s*=slowdown; r_s*=slowdown;

	SOM::L.dash = m_s; //hack
	SOM::L.walk = m_s; m_s*=f4;   		
	if(op->do_dash) //NEW: slow turning
	{
		//soon slow to regular turn speed (soon?)
		r_s = lerp(r_s,SOM::_turn,mc.peaking*slide2); 	
		//NEW: 150% accelerated turn rate (KF3 does "100%")
		if(r_s>SOM::_turn) r_s = SOM::_turn+(r_s-SOM::_turn)/2;
	}
	som_mocap_seturn(SOM::L.turn,r_s/pi*180,
	f4__1&&mc.turning?lerp(1,pi*2/r_s,mc.turning):1);
	r_s*=f4_2;

	//restrict movement along diagonals
	{
		//NOTE: som_mocap_circle had been here for a
		//long time

		//something feels wrong since moving this to the top
		//I'm using som_mocap_circle_before to try both ways 
		if(!som_mocap_circle_before) som_mocap_circle_2020(kb);
	}
	if(SOM::hit||SOM::invincible) if(SOM::L.f4)
	{
		float div = //hack: clamp down on player input
		(SOM::hit?8:mc.ducking*8)-2*(1-mc.recovery);
		const BYTE bts[]={0x47,0x49,0x4B,0x4C,0x4D,0x50,0x4F,0x51};		
		if(div>0) for(size_t i=0;i<sizeof(bts);i++) 	
		kb[bts[i]] = EX::Joypad::analog_divide(kb[bts[i]],min(8,div));
	}

	///// deviation /////////	
		
	float _y_ = SOM::err[1];

	if(SOM::err[1]<SOM::L.pcstate[1]) //going up
	{
		SOM::err[1]+=am->error_parachute;

		if(SOM::err[1]>SOM::L.pcstate[1]) SOM::err[1] = SOM::L.pcstate[1];
	}
	else if(SOM::err[1]>SOM::L.pcstate[1]) //going down
	{
		SOM::err[1]-=am->error_parachute;

		if(SOM::err[1]<SOM::L.pcstate[1]) SOM::err[1] = SOM::L.pcstate[1];
	}	
	
	#if 1 
	float *pc3 = SOM::L.pcstate;
	#else
	float pc3[3]; 
	memcpy(pc3,SOM::L.pcstate,sizeof(pc3));
	for(int i=0;i<=2;i+=2)
	pc3[i] = (pc3[i]+SOM::L.pcstate2[i])*0.5f;
	#endif

	float lasso[3]; mc.error = //ATTN: lasso IS REUSED DOWN BELOW	
	Somvector::map(lasso).copy<3>(pc3).remove<3>(SOM::err).length<3>();
				
	SOM::err[1] = _y_; _y_ = lasso[1]; lasso[1] = 0; //hacks//
	SOM::err[3] = Somvector::map(lasso).length<3>(); lasso[1] = _y_;
			
	float clearance = am->error_clearance;
	float tolerance = am->error_tolerance;

	if(!clearance) //todo: algebra
	{
		//moving out of som_mocap_am_calibrate
		int fps = DDRAW::refreshrate; //2021
		const float step = 1.0f/(fps?fps:60); //fps

		static float cached = 0, hit = 0;
		if(hit!=SOM::walk) cached = som_mocap_am_calibrate(hit=SOM::walk,step);
		clearance = cached;
	}
	if(!tolerance) //safe at any speed??
	{	
		tolerance = clearance/2;
	}							  

	if(mc.error>tolerance)
	{			
		//todo: make leaning like leading
		//todo: why not lose leaning altogether?

		if(mc.error<clearance)
		{
			float den = clearance-tolerance; //zero divide
			mc.leaning = (mc.error-tolerance)/(den?den:1);
			if(mc.leaning>1) mc.leaning = 1;			
		}
		else mc.leaning = 1;
	}		
	else mc.leaning = 0; 
			
	/*MOVED into "this"*/
	//static float leading = SOM::err[3]; 
	{	
		//hack: smoothing crouching jumps at hard diagonals/in general		
		float rate = lerp(0.75f,0.5f,tap_vs_hold/500-1); //NEW	
		if(mc.smoothing)
		{
			float height = mc.leadingaway; //YUCK
			mc.leadspring.stiffness = rate*lerp(0.5f,1,height*height);
			mc.leadspring.reposition(height,step);
			leading = SOM::err[3]*(1-mc.jumping); //smoothed in camera
		}
		else leading = lerp(leading,SOM::err[3]*(1-mc.jumping),antijitter*rate);				
		
		//unlike leaning ranges from 0~clearance
		mc.leading = min(1,leading/clearance); 
		//mc.leading2 = SOM::err[3]/clearance; //NEW: stairs
		mc.leading2 = min(1,SOM::err[3]/clearance); //it's going over 1, assuming unwanted

		if(mc.holding_xyzt[3]) //2020
		{
			//NOTE: same as with leading but different length pair
			float xz[2] = {SOM::xyz[0]-mc.holding_xyzt[0],SOM::xyz[2]-mc.holding_xyzt[2]};
			float lead = Somvector::map(xz).length<2>();
			lead*=mc.holding_xyzt[3]; //simulate place_shadow?
			if(!mc.smoothing) 
			lead = holding = lerp(holding,lead,antijitter*rate);
			mc.leadingaway = min(1,lead/clearance);
		}
		else if(mc.action&&!mc.Jumpstart)
		{
			//NEW: this is for entering squatting mode
			//unfortunately standing jumps are roped in
			mc.leadingaway = mc.leading;
		}
	}
					   
	float lean = 1-mc.leaning;  
	
	////// modulation /////////

	const int gaits_z = EX::Joypad::gaitcode(kb[0x4C]);
	const int gait_z2 = EX::Joypad::gaitcode(kb[0x50]); //2020
	const int gaits_x = EX::Joypad::gaitcode(kb[0x4D]?kb[0x4D]:kb[0x4B]); 
	//REMOVE ME?
	float crabgain = EX::Joypad::analog_scale(kb[0x4D]?kb[0x4D]:kb[0x4B]);

	//includes sneak/jog/sprint
	/*2021: need fleeing state for attack inputs
	if(!f4__1||SOM::hit||mc.scaling)*/
	if(SOM::hit||mc.scaling) 
	{
		//2021: break "standing" on SOM::hit to unstick
		mc.running-=min(mc.running,t);
		mc.jogging-=min(mc.jogging,t);
	}
	else //business as usual
	{	
		int j = dt->dash_jogging_gait; 
		int k = 7-dt->dash_running_gait; 
		int l = j-1-dt->dash_stealth_gait; 

		int gait = gaits_z;

		if(!op->do_dash)
		{
			assert(!mc.running);
		}
		else if(dashing2&&!mc.resting) //forward (do_dash)
		{	
			int i = 7-dt->dash_stealth_gait;			

			//2018: problem for SOM::thumbs analog model?
			//#if 0 //QUICK-FIX
			//if(gait>=j&&gait<7) gait = j+gaits_x; //flanking?
			//#else
			if(gait>=j) gait = min(7,gait+gaits_x);
			//#endif

			//moving this outside of run/jog block so the
			//speed is faster going forward after clipped
			//by the "sliding" consideration below
			float n = gait<j?l:k;

			//HACK: masking an inappropriate dip in speed
			if(mc.stealth_squatting) n*=1-mc.squatting;

			//2020: trying to restrict jogging/running to
			//around 45 degrees for new "sliding" feature
			if(gaits_x>j-1) gait = min(gait,j-1);
		
			if(gait>6||gait<j) 
			{
				float slow = running2*n*lean;

				kb[0x4C] = EX::Joypad::analog_divide(kb[0x4C],slow);

				//EX::dbgmsg("slow %f (%f)", slow,running2);

				if((mc.jogging-=t)<0) mc.jogging = 0;
							
				bool hack = power<5000||mc.fleeing; assert(t||!step);

				if(gait>6&&hack/*NEW*/&&mc.action)
				{
					if((mc.running+=t)>1) mc.running = 1; 
				}
				else if((mc.running-=t)<0) mc.running = 0;
			}
			else //jogging gaits (j~6)
			{	
				if(!mc.action) /*NEW (deceleration)*/
				{
					mc.jogging-=min(mc.jogging,t);
				}
				else if(mc.dashing==1)
				if((mc.jogging+=t)>1) mc.jogging = 1;
				if((mc.running-=t)<0) mc.running = 0;

				float jogging = mc.jogging;	
				jogging*=EX::Joypad::gaitcode(kb[0x4C])-j;
				
				kb[0x4C] = EX::Joypad::analog_divide(kb[0x4C],jogging*lean); 		
			}
		}	
		else //mc.running = mc.dashing; 
		{
			//seems safer considering the new deceleration hack
			if(mc.dashing<mc.running) mc.running = mc.dashing;		

			mc.running-=min(mc.running,t);
			mc.jogging-=min(mc.jogging,t);
		}

		float fast = min(1,2-mc.ducking); //2018
		fast*=1-mc.holding_xyzt[3]; //2020
		if(walking2&&op->do_walk&&f4__1) //backwards/sideways
		{			
			float slow = lean*fast;
			slow*=1-mc.recovering; //2021: poison?
			//2021: sqrtf fixes a bad, delayed effect on SOM::motions.shield
			//watch the map coordinate suddenly switch gears at the very end
			//almost any power function seems to do the trick pow(2) is both
			//least severe (to changing directions) and smooth
			kb[0x50] = EX::Joypad::analog_divide(kb[0x50],walking2*2*slow*slow); 
			kb[0x4B] = EX::Joypad::analog_divide(kb[0x4B],walking2*1*slow); 
			kb[0x4D] = EX::Joypad::analog_divide(kb[0x4D],walking2*1*slow); 
		}		
		if(!mc.jumping) //2020 (TESTING)
		if(dashing2&&op->do_dash&&f4__1) //backwards/sideways
		{
			//walking2 should be 0 if sliding
			fast*=cosf(pi*mc.sliding)/2+0.5f; //2020

			float slow = lean*dashing2*fast; //*dash_walk/2

			//2018: problem for SOM::thumbs analog model?
			int j2 = 0?j:dt->dash_running_gait-(gait>6?0:2);

			int _4 = 7-j2+l, _3 = 7-j2; //want to end up jogging sideways

			//note: running2 isn't mc.running^2
			float rugging2 = running2+mc.jogging/2; //2020

			//CAUTION: 50 is for sneaking backwards. walking may be faster
			kb[0x50] = EX::Joypad::analog_divide(kb[0x50],rugging2*_4*slow); 
			kb[0x4B] = EX::Joypad::analog_divide(kb[0x4B],running2*_3*slow); 
			kb[0x4D] = EX::Joypad::analog_divide(kb[0x4D],running2*_3*slow); 						
		}
	}

	//paradox: because jogging/sprinting gaits
	//if(!mc.stealth_squatting)
	{
		/*MOVED into "this"*/
		//static float braking, braking_z;

		//fyi: braking here means transitioning from 
		//dashing to walking modes via releasing DASH 

		if(!mc.hopping) //shield?
		if(mc.dashing&&f4__1&&!SOM::hit) //braking?
		{
			if(braking_z&&mc.action&&!kb[0xF3]) //DASH 
			{
				braking = mc.dashing; //begin braking				
			}
			else if(mc.dashing>=braking) braking = 0; //cancel
		}
		else braking = 0; for(;;) //rewriting to calculate z once
		{
			float z = m_s*EX::Joypad::analog_scale(kb[0x4C]);

			//remember the highspeed
			//REMINDER: USING PRIOR FRAME IMPACTS AUTOMATIC JUMP
			if(!braking) braking_z = z; 

			if(mc.stealth_squatting) break;

			//ensure braking speeds do not exceed the highspeed
			if(z<=braking_z) break;
			int paranoia = kb[0x4C];
			kb[0x4C] = EX::Joypad::analog_divide(kb[0x4C],1); 
			if(paranoia==kb[0x4C]) break;
		}
	}
	if(op->do_dash) //NEW: crouch turning speed
	{
		float slow = fabsf(mc.goround)*mc.resting;
		const int bts[2] = {0x51,0x4F};	for(int i=0;i<2;i++)
		{
			//cannot be too slow or the up/down speed is too different
			const int maxgait = 5; //75%
			float divide = EX::Joypad::gaitcode(kb[bts[i]])-maxgait;
			if(divide>0)
			kb[bts[i]] = EX::Joypad::analog_divide(kb[bts[i]],slow*divide);
		}
	}
		
	if(crabgain) //REMOVE ME?
	crabgain/=EX::Joypad::analog_scale(kb[0x4B]?kb[0x4B]:kb[0x4D]); //-gaits_x	
	if(!crabgain) crabgain = 1; assert(_finite(crabgain));
	 	
	////// suspension //////////
					
	float leap = Suspend(f4__1?kb:0,step);							
	if(mc.hopping&&!mc.Jumpstart&&!mc.suspended||mc.falling<-0.25f)
	mc.hopping = false;

	if(!pc->do_not_jump&&f4__1) 
	{
		/*MOVED into "this"*/
		//static int center = 0; //hack?? 	
		//static bool skipping = false; //NEW

		if(mc.event_let_go)
		if(1==mc.holding_xyzt[3])
		{
			//DICEY: leadingaway is keeping from jumping
			//when letting go around the original position
			if(mc.leadingaway>=0.8f&&mc.leading>0.7f)
			{
				assert(!mc.jumping); //???
				if(!mc.interrupted&&!mc.jumping)
				{
					mc.Jumpstart = kb;					
				}
			}
		}
		else if(mc.action&&!kb[0xF3]) //does event_let_go cover this?
		{
			//for some reason mc.action is larger coming off of jumps
			//and swinging???
			int hop = SOM::motions.tick-mc.neutral>=(unsigned)tvh_ms?
			0:SOM::L.swinging?450:mc.falling||mc.bounce?300:200; //150

			if(mc.landing||mc.jogging&&!mc.running)
			{
				mc.Jumpstart = kb;
			}
			else if(mc.action<=hop&&mc.ducking<1) //2020: hop?
			{
				if(!mc.Jumpstart)
				{
					mc.hopping = true;
					mc.Jumpstart = kb;
				}
			}
			else if(!mc.jumping) //skipping?
			{				
				//FIX ME: what about frame rate?

				bool crouching = //-15: offer some leeway
				//mc.ducking<2||SOM::crouch>=SOM::frame-10;
				mc.ducking<2||SOM::motions.tick-mc.crouched<=150;
				if(crouching||mc.resting!=1)		 
				{	
					//if(!mc.leaning||mc.resting) 
					if(mc.resting||mc.leading<0.5f) 
					{	
						if(mc.ducking>=1 //HACK
						//2020: mc.ducking>=1 is a source of 
						//misfires... this may be too lax by
						//itself
						//||mc.action>=tap_vs_hold) 
						||mc.action>=tvh_ms-33*2) //add some perceptual leeway
						{
							//would like to get to the bottom of this (but -33*2 is
							//making this a pretty gray area)
							//if(mc.ducking<1)
							//EX::dbgmsg("jump duck/hold discrepancy %f",mc.ducking);

							if(!mc.interrupted)
							{
								//EXPERIMENTAL
								//2020: stealth->squat transition?
								if(!pc->do_not_squat)
								if(mc.inverting<0.25f) //upsadaisy?								
								if(!mc.resting&&!mc.jogging&&!mc.running)								
								{
									//KITCHEN SINK
									//stealth? I keep doing this standing still
									//when beginning to crouch and I'm not sure
									//why, but I'm pretty sure it is this code
									if(mc.ducking==1) 
									if(mc.Engine.push||mc.action>=tvh_ms+66)
									{
										if(!mc.gaits_max_residual)
										{
											//speculative inversion?
											mc.inverter = 1;
											mc.squatting = 0.000001f;
											mc.stealth_squatting = 1;
										}
										else //mc.fastwalk = 0; //better
										{
											//ideally this can be tweaked to make transitions
											//seemless as can be (right now very little thought
											//time or testing has been put into this formula)
											mc.fastwalk = max(0,(int)mc.gaits_max_unboosted-4)/3.0f;
										}
									}
								}

								mc.Jumpstart = kb; //mc.jumping = 0.000001f; 

								SOM::eventick = 0; //cancel crouching subtitle (2020)
							}
						}
					}
				}
			}
			//else skipping = true; //2020
		}
	}
	
	////// orientation ///////
	
	//SOM wants to invert these
	//TODO: standardize on som_logic_406ab0
	const float c = cosf(-SOM::uvw[1]);
	const float s = sinf(-SOM::uvw[1]); 

	float x = 0, y = 0, z = 0, u = 0, v = 0;
	 	
	if(kb[0x4D]) x+=EX::Joypad::analog_scale(kb[0x4D]);
	if(kb[0x51]) u+=EX::Joypad::analog_scale(kb[0x51]);
	if(kb[0x4B]) x-=EX::Joypad::analog_scale(kb[0x4B]);
	if(kb[0x4F]) u-=EX::Joypad::analog_scale(kb[0x4F]);

	float corkscrew = 0;
	if(x*u>0&&!pc->do_not_corkscrew) //same sign?
	corkscrew = x*crabgain*u;
	corkscrew = powf(corkscrew,pc->corkscrew_power);
	if(u<0) corkscrew*=-1;

	if(!mc.jumping&&!mc.falling) //NEW: good enough?
	{
		/*MOVED into "this"*/
		//static float uturn = 0; 
		float uwalk = lerp(0.5f,0.25f,walking2);	
		uturn = lerp(uturn,corkscrew,antijitter*uwalk);
		mc.turning = fabsf(uturn);
	}
	
	if(op->do_hit) 
	{
		if(som_mocap_guard) //2021: shield?
		{
			const float c = cosf(+SOM::uvw[1]);
			const float s = sinf(+SOM::uvw[1]); 

			float xz[2]; 
			float xx = EX::Affects[0].position[0];
			xz[0] = xx;
			xz[1] = EX::Affects[0].position[2];
			//rotate x/z about the left handed Y axis
			xz[0] = xx*c+xz[1]*s; xz[1] = xx*-s+xz[1]*c;
			//I'm not sure this was meant to go over 1
			float hit = sqrtf(SOM::motions.shield3);
			xz[0]*=0.2f+0.2f*hit;
			xz[1]*=0.2f+0.4f*hit;
			//can't turn into a third-person style game
			//if less than 1 give it more push, else it
			//has to be reeled in (note, this needed to
			//be added because the knockback effect was
			//increased dramatically)
			if(xz[1]>0.0001f) //from behind?
			{
				//no clue... just has to be within some
				//tolerance to see the back of the head
				//(TODO: the camera could look up some)
				float t = 1/powf(lerp(1,2.5f,xz[1]),2); //ARBITRARY
				xz[1]*=t;
				xz[0]*=t; //can't be disproportionate
			}
			for(int i=2;i-->0;)
			SOM::motions.shield2[i] =
			lerp(SOM::motions.shield2[i],xz[i],antijitter*0.5f);
			//SHOULD MATCH leap*4 BELOW
			//float step_4 = step*4;
			float l_step_4 = max(1-step*4,0); //simulate
			SOM::motions.shield3*=l_step_4;
		}

		//2020: I'm increasing this to compensate for removing the
		//traps clip test... I'm using the same value for monsters
			if(f4__1)
		EX::Affects[0].simulate(kb,256,leap*4,1); //0.5f
	}

	const BYTE bts[] = 
	{	
		0x47,       0x49, //7 9
		0x4B, 0x4C, 0x4D, //456
		0x4F, 0x50, 0x51, //123
		0x1E,       0x2C, //A Z
	};

	x = y = z = u = v = 0;

	for(size_t i=0;i<sizeof(bts);i++) 
	{
		float gait = 
		EX::Joypad::analog_scale(kb[bts[i]]);
		switch(bts[i])
		{
		case 0x4D: x+=gait; break; //6
		case 0x4B: x-=gait; break; //4
		case 0x1E: y+=gait; break; //A
		case 0x2C: y-=gait; break; //Z
		case 0x4C: z+=gait; break; //5
		case 0x50: z-=gait; break; //2
		case 0x51: u+=gait; break; //3
		case 0x4F: u-=gait; break; //1
		case 0x47: v+=gait; break; //7
		case 0x49: v-=gait; break; //9
		}
	}
	
	//saving for later
	float sidestep = x, backstep = z;
	float xz = x+z?1/sqrt(x*x+z*z):0;

	//rotate x/z about the left handed Y axis
	float xx = x; x = x*c+z*s; z = xx*-s+z*c;	
	//NEW: reconstruct thumbstick's direction
	mc.lookahead[0] = x; mc.lookahead[1] = z;

	//if(op->do_pedal) //deprecated
	{	
		//2018 
		//Affects[1] can cancel these out, so
		//smooth() believes they're depressed
		bool unsnap[5] = {x,y,z,u,v};

		/*increasing because dodging sideways
		//drifts too far
		const float damp = 4; //position*/
		//REMINDER: can't go over 1 below
		//
		// don't change this without checking l2
		// WRT mc.fleeing
		//
		float damp = 4.8f; //5;				
		//const float whip = 2; //rotation
		//const float whip = 2.25;
		//should be in terms of damp I think
		//float whip = damp/2;
		//this feels way better when coming off circling
		//(but not so much for looking around?)
		//float whip = damp/(2-fabsf(sidestep));		
		float sq = mc.squat/2;		
		float whip = damp/(2-sq*1.333f); //2020
		float whip2 = damp/2;
		
			damp+=sq; //NOTE: after "whip"
		
		//EX::dbgmsg("damp: %f (%f)",damp,ss);
		
		//usually wanna relax before accumulation
		EX::Affects[1].simulate(kb,256,damp*leap);
		
		//2020: something feels too stiff these days
		//I'm just trying things out... it seems the
		//values only change the experience when the
		//gait is increased. 
		//
		// HISTORY LESSON: values over 1 add increase real speed (not drift speed)
		// this cancels out speed reduction effects like mc.fleeing
		//		
		//HACK: cancel out mc.fleeing (a little)?
		//maybe it's not possible... speed doesn't really change until the gait is
		//changed, then it goes from 12 to 14 km/h		
		//REMINDER: 1.3 SEEMS TO INFLUENCE RUNNING SPEED (2021)
		float g = walking2*damp*1.1f; //HACK: 1 has gait 2 go to 0.19999 (not 0.2)
		float g2 = dashing2*damp*1.3f; //notice 1.3 note (2021)
		float f = walking2*whip;
		float f2 = dashing2*whip;
		float f3 = dashing2*whip2; //2020

		float lead = leap;
		
		//SUSPECT? (NOT ANYMORE)
		//2020: I DON'T FOLLOW THIS... I WORRY IT MAY
		//BE DOING MORE HARM THAN GOOD THESE DAYS?
		//(I think r is smoothing takeoffs when going
		//from partially leaned out to jumping, but
		//"leading" should be very small these days?)
		//if(1)
		{
			//NEW: resisting release for Jumpstart
			//float r = min(1,2.0f-mc.ducking+0.5f);
			//NOTE: r modifies this. it's older code
			//lead*=1-mc.leading*r;
			
			//2020: this is pretty sensitive... there's "two 
			//masters" to serve here. one is to ideally push
			//out so it doesn't withdraw when jumping, two is
			//to not hook when letting go of the stick... you
			//can't have it both ways just by this. hooking is
			//unacceptable
			lead*=(1-mc.leading)*min(1,2-mc.ducking);
		}
				
		EX::Affects[1].position[0]+=x*(g+g2)*lead;
		EX::Affects[1].position[1]+=y*(g+g2)*step;
		EX::Affects[1].position[2]+=z*(g+g2)*lead;		
		EX::Affects[1].position[3]+=u*(f+f2)*leap;
		EX::Affects[1].position[4]+=v*(f+f3)*step;

		if(leap!=step) //hack: free up look up/down
		{
			//todo: EX::Affects[1].relax()
			EX::Affects[1].position[4]*=(1-damp*(step-leap));			
		}
		
		//REMINDER: these are in world coordinates
		//EX::dbgmsg("pedal: %f",EX::Affects[1].position[0]);

		x = y = z = u = v = 0; 

		for(size_t i=0;i<sizeof(bts);i++) 
		{
			float gait = 
			EX::Joypad::analog_scale(kb[bts[i]]);	
			switch(bts[i])
			{
			case 0x4D: x+=gait; break; //6
			case 0x4B: x-=gait; break; //4
			case 0x1E: y+=gait; break; //A
			case 0x2C: y-=gait; break; //Z
			case 0x4C: z+=gait; break; //5
			case 0x50: z-=gait; break; //2
			case 0x51: u+=gait; break; //3
			case 0x4F: u-=gait; break; //1
			case 0x47: v+=gait; break; //7
			case 0x49: v-=gait; break; //9
			}
		}		
	
		sidestep = x; backstep = z;

		if(mc.smoothing) //NEW
		{
			if(drag>0.001f) //EXPERIMENTAL
			{
				//x*=0.9f; z*=0.9f;
				float slow = 1-0.1f*powf(drag,0.25f);
				x*=slow; z*=slow;
			}

			//NEWER
			mc.nudge(0,kb[0x4D],kb[0x4B]);
			mc.nudge(1,kb[0x1E],kb[0x2C]);
			mc.nudge(2,kb[0x4C],kb[0x50]);
			mc.nudge(3,kb[0x51],kb[0x4F]);
			mc.nudge(4,kb[0x47],kb[0x49]);
			
			mc.smooth(x,y,z,u,v,dt->u2_power,step,unsnap);			
		}
									   	
		//rotate x/z about the left handed Y axis
		float xx = x; x = x*c+z*s; z = xx*-s+z*c;
	}	

	///// diagonals /////////
	
	if(!mc.smoothing) 
	if(((kb[0x50]|kb[0x4C])&0x80)
	 ==((kb[0x4B]|kb[0x4D])&0x80)) 
	{	
		//SOM slows down diagonals so that
		//the same distance is covered. So
		//we are compensating for that now

		//assuming turning is not affected
		SOM::L.walk*=sqrt2;
		SOM::L.dash*=sqrt2;
	} 	

	///// nightmares ////////

	float bendover = 0; //if(EX::debug) //EXPERIMENTAL
	{
		//note: this includes looking upward a little
		float cmp = fabsf(SOM::uvw[0]);
		float soft = som_clipc_slipping2;
		float bend = pc->player_character_nod;
		float duck = max(0,mc.ducking-1);		
		float duck2 = 1-sqrtf(duck);		
		bool headsup = mc.jogging||mc.running;
		if(mc.gaits_max_unboosted>=5&&SOM::uvw[0]<0)
		headsup = true;

		if(duck||!headsup)
		if(!soft&&cmp>bend&&f4__1&&!mc.scaling)
		if(!SOM::motions.aloft&&!SOM::L.pcstep)
		{
			bendover = 1-mc.leading2*mc.leading2*duck2; //1
			
			//this is mainly to not put your head through
			//the floor but it makes physical sense too
			bendover-=duck*0.2f*(1-mc.leading2);
		}
		
		//2021: always testing for shield/arm
		bool bending = false; float dist = FLT_MAX;

		if(SOM::uvw[0]>=0)
		{
			bendover*=0.5f; //looking up is simple :)
		}
		else if(bendover&&SOM::uvw[0]<pi/-6)
		{
			bending = true;
		}
		//else if(bendover)
		{			
			float flr;
			auto &mdl = *SOM::L.arm_MDL;
			if(bending&&duck&&(mdl.d>1||mdl.ext.d2))
			{
				//flr = 0.25f; //ARBITRARY
				flr = pc->player_character_inseam/2;
			}
			else flr = 0.7f*SOM::L.duck;

			float center[3],_[3];
			float radius = pc->player_character_shape;
			float xyz[3] = {SOM::L.pcstate[0],0,SOM::L.pcstate[2]};
			float s = -sinf(SOM::uvw[1]);
			float c = +cosf(SOM::uvw[1]);
			center[0] = xyz[0]+s*radius; xyz[0]+=s*0.01f;
			center[2] = xyz[2]+c*radius; xyz[2]+=c*0.01f;
			center[1] = xyz[1]=SOM::xyz[1]+flr-radius/2;			
			float dist3[3]; 
			auto fdist3 = [&]() //2021
			{
				float x = dist3[0]-xyz[0];
				float z = dist3[2]-xyz[2];
				float d = sqrtf(x*x+z*z);
				if(d<dist) dist = d;
			};
							
			//EX::dbgmsg("bent over %f %d (%f)",bendover,SOM::frame,*mc.bending_soft);

			//todo: test at floor level to better activate 20/22 type objects
			for(int sz=SOM::L.ai3_size,i=0;i<sz;i++)			
			if(SOM::clipper_40dff0(center,radius,radius,i,5,dist3,_))
			{
				fdist3(); //2021

				//EX::dbgmsg("hit object %d %d",i,SOM::frame);
			}

			//if(SOM::clipper.clip2(center,radius,radius,5,0,dist3)) 
			{
				DWORD pt;
				extern BYTE _cdecl som_MHM_415450(FLOAT[3],FLOAT,FLOAT,DWORD,FLOAT[3],FLOAT[3],DWORD*);
				//REMOVE ME
				//because som_MHM_415450 stops on the first hit polygon the
				//clip shape needs to be reduced... need to implement a version
				//that can accumulate the closest hit?
				if(som_MHM_415450(xyz,radius,radius,5,dist3,_,&pt))
				{
					fdist3(); //2021
				}
				if(dist==FLT_MAX)
				if(som_MHM_415450(center,radius,radius,5,dist3,_,&pt))
				{
					fdist3(); //2021
				}
			}
			//else //NPCs
			{
				radius/=2; //pc->player_character_shape2;

				//NOTE: this is sphere-vs-cylinder test used by projectiles
				//since the cylinder-vs-cylinder (40c2e0) has not seen work
				extern BYTE __cdecl som_logic_40C8E0
				(FLOAT[3],FLOAT,FLOAT[3],FLOAT,FLOAT,FLOAT[3],FLOAT[3]);
				float *npc = &SOM::L.ai2[0][SOM::AI::xyz2]; 
				float *enemy = &SOM::L.ai[0][SOM::AI::xyz];
				for(int sz=SOM::L.ai_size,i=0;i<sz;i++,enemy+=149)
				if(3==SOM::L.ai[i][SOM::AI::stage])
				if(som_logic_40C8E0(center,radius,enemy,enemy[123],enemy[121],dist3,_))
				{
					fdist3(); //2021
				}
				for(int sz=SOM::L.ai2_size,i=0;i<sz;i++,npc+=43)				
				if(3==SOM::L.ai2[i][SOM::AI::stage2])
				if(som_logic_40C8E0(center,radius,npc,npc[12],npc[10],dist3,_))
				{
					fdist3(); //2021
				}
			}
		}

		mc.arm_clear = dist>=pc->player_character_shape+0.05f; //2021

		//EX::dbgmsg("arm clear: %f (%f) %d",SOM::motions.arm_clear,dist==FLT_MAX?-1.0f:dist,mc.arm_clear);

		if(bending&&dist!=FLT_MAX) //hit?
		{
			//EX::dbgmsg("bent over hit %d (%f)",SOM::frame,*mc.bending_soft);

			//bendover = -0.3333f; //prevents bending over
			bendover = 0;

			soft = 1; //hack = 1;

			mc.bending_resist = min(1,mc.bending_resist+step*2);
		}

		float rate = 1;
		if(!SOM::motions.aloft) 
		rate+=3*(1-sqrtf(mc.landspeed));		
		mc.bending = lerp(mc.bending,bendover,antijitter/rate); //4
		bendover = mc.bending;
		if(mc.falling||mc.scaling||SOM::motions.aloft||Suspend.bouncing) 
		soft = 1;	

		if(!bending||dist==FLT_MAX)
		{
			//slow things down when coming off the geometry
			mc.bending_resist-=min(step*2,mc.bending_resist);
			bendover*=1-mc.bending_resist;
		
			//FIX ME: this should be unconditional but it seems to better behaved
			//if blocked when hit
			//if(bendover) //WORKS BUT REALLY DOESN'T BELONG HERE?		
			if(1) //warning: this was from before leading2 was factored into bendover
			{
				mc.bending_soft[1] = lerp(mc.bending_soft[1],soft,antijitter/rate); //4

				duck2 = lerp(duck2,1,mc.bending_soft[1]); //NEW: cancel duck2 on hit?

				//mc.leading has a problem with turning corners
				//[4] here just holds past value of mc.leading2
				//2020: I tried to lessen this but it came out lumpy no matter what I did			
				mc.bending_soft[2] = lerp(mc.bending_soft[2],mc.leading2*duck2,antijitter/(rate*2)); //8

				/*this is glitching while climbing up with the new bounce effect
				//maybe without it as well? I've incorporated mc.leading2 above
				//NOTE: was doing this in analogcam but it is
				//not meant to modify values and so shouldn't
				//smooth/soften
				soft = 0;
				if(!SOM::motions.aloft&&!Suspend.bouncing)
				if(!mc.resting)*/
				soft = max(mc.bending_soft[2],powf(mc.bending_soft[1],2));
				*mc.bending_soft = lerp(*mc.bending_soft,soft,antijitter);
			}
			else //2020: works fine except clips through walls when pressed against
			{		
				soft = lerp(*mc.bending_soft,soft,antijitter/4);  
				//if(soft<0.001) soft = 0;
				*mc.bending_soft = soft*soft;
			}
		}
	}

	///// relaxation ////////	

	float r_s_step[2] = {u,v};
	r_s_step[1]*=1+bendover*1.5f;
	float m_s_step[3] = {x,f4__1?0:y,z};
	if(SOM::u) r_s_step[0]*=*SOM::u;
	r_s_step[0]*=r_s*step; r_s_step[1]*=r_s*step;	
	Somvector::map(m_s_step).se_scale<3>(m_s*step);	
	/*2021: this cancels out below, although note that
	//som_mocap_am_calibrate works like this
	float relax = som_mocap_am_relax(step)*mc.error;*/
	float relax = som_mocap_am_relax(step);

	if(mc.error) //zero divide
	{
		Somvector::map(SOM::err).move<3>(m_s_step)
		/*2021: see above change to relax... I'm changing this so the
		//math doesn't appear more complex than it actually is
		.move<3>(Somvector::map(lasso).se_scale<3>(relax/mc.error));*/
		.move<3>(Somvector::map(lasso).se_scale<3>(relax));
	}
	else //NEW: SOM::err is 0 at top of game, requiring some initial movement???
	{
		//I'M 90% SURE THIS IS CORRECT (10% NOT!)
		//EX::dbgmsg("ducking bug at top of game %d",SOM::frame);
		//without this ducking before ever moving makes leaning out stay flush
		//to the ground
		memcpy(SOM::err,SOM::xyz,3*sizeof(float));
	}
	//EX::dbgmsg("ducking bug at top of game %f",SOM::err[3]);	
	
	float lasso_length = //ATTN! overwriting lasso with xyz
	Somvector::map(lasso).copy<3>(pc3).remove<3>(SOM::xyz).length<3>();

	if(lasso_length) //EXPERIMENTAL
	{
		//don't want VR contibution
		//float xz[2] = {SOM::pov[0],SOM::pov[2]};
		//Somvector::map(xz).unit<2>();
		float xz[3] = {0,0,1};
		SOM::rotate(xz,0,SOM::uvw[1]); //xz[1] = xz[2];
		float xz2[3] = {xz[0],0,xz[1]};
		SOM::rotate(xz2,0,pi/2); //xz2[1] = xz2[2]; //YUCK
		float cmp[3] = 
		{-lasso[0]/lasso_length,0,-lasso[2]/lasso_length};
		mc.pushahead[1] = xz[0]*cmp[0]+xz[2]*cmp[2];
		mc.pushahead[0] = xz2[0]*cmp[0]+xz2[2]*cmp[2];
		//EX::dbgmsg("push: %f,%f",mc.pushahead[0],mc.pushahead[1]);
	}
	else mc.pushahead[0] = mc.pushahead[1] = 1;
		
	float reshape = som_mocap_am_reshape(clearance); 
	relax = reshape*lasso_length*som_mocap_am_relax(step); 
	
	//NEW: in support of do_u2. should be higher up, but isn't
	float relax3D[3] = {};
	//2020: prevent crawling do_aa2 artifacts? 
	if(lasso_length>0.0005f)
	Somvector::map(relax3D).se_copy<3>(relax);
	if(1) //back away slowly (best to test with keyboard)
	{
		/*MOVED into "this"*/
		//static float shoveoff = 0;									   
		//TODO: determine if counter to lean direction or not		
		shoveoff = lerp(shoveoff,!mc.Engine.push,antijitter*(1+mc.leading));				
		float half = 0.5f, rate = half/(1+(dash_walk-1)*0.5f*dashing2);		
		float factor = lerp(1,1-mc.leading*rate,shoveoff);
		relax3D[0]*=lerp(1,factor,1-fabsf(x));
		relax3D[2]*=lerp(1,factor,1-fabsf(z));
	}	

	//EXPERIMENTAL (running on stairs)
	//meant to act like a release valve so that reaching
	//the top of the stairs the error differential is not
	//so great as to create a slingshot
	drag = lerp(drag,mc.running*SOM::L.pcstep,antijitter);
	{
		//ramping up first contact with stairs since drag
		//itself usually doesn't make it very far past .5
		float drag2 = powf(drag,0.25f);
		//this term can't really be negative because if the
		//negative part is hit at the top of the stairs the
		//original glitch happens. analogbob is factored in
		//to create the appearance of skipping steps at the
		//running clip
		//TODO: WOULD FEEL BETTER IF STEPS ARE SYNCHRONIZED
		//WITH THE TOPS OF THE CLIMBED SURFACE
		float close_distance = drag2*(2-mc.analogbob)*0.25f;
		relax3D[0]*=1+close_distance;
		relax3D[2]*=1+close_distance;
		//this last part gives it a bounding motion that is
		//like running up stairs...
		//ALSO: it does wonders to smooth out the look/feel
		relax3D[1]*=1+close_distance*1.2f-drag2*0.8f;
	}

	//try to make pulling back less dramatic and maybe feel
	//like pulling on a heavy door
	if(float t2=mc.holding2) 
	{
		//t2*=1-mc.sideways;
		t2*=1-(1-mc.backwards)/2-mc.sideways/2;
		for(int i=0;i<=2;i+=2)
		relax3D[i] = lerp(relax3D[i],lasso_length,t2*step*2);
	}
	
	//REMOVE ME?
	bool ok = mc.smoothing;
	bool x_ok = ok||x||fabsf(lasso[0])>am->error_allowance;
	bool y_ok = ok||1||fabsf(lasso[1])>0.1f; //_allowance3?
	bool z_ok = ok||z||fabsf(lasso[2])>am->error_allowance;
	bool u_ok = ok||u||fabsf(SOM::uvw[1]-SOM::L.pcstate[4])>am->error_allowance2;
	bool v_ok = ok||v||fabsf(SOM::uvw[0]-SOM::L.pcstate[3])>am->error_allowance2;

	EX::dbgmsg("d: %f (%f,%f)",SOM::pov[3],SOM::cam[0],SOM::cam[2]);

	if(!SOM::L.pcstate[3]) v_ok = true; //hack: recentering		

	float spin_delta[2] = {SOM::uvw[0],SOM::uvw[1]};

	//float lasso_length_rcp_x_relax = 1;
	float lasso_length_rcp_x_relax[3] = {1,1,1};
	for(int i=0;i<3;i++)
	{
		if(lasso_length>relax3D[i]) //don't wanna overshoot
		lasso_length_rcp_x_relax[i] = relax3D[i]/lasso_length;
	}
	if(x_ok) SOM::xyz[0]+=lasso[0]*lasso_length_rcp_x_relax[0];
	if(y_ok) SOM::xyz[1]+=lasso[1]*lasso_length_rcp_x_relax[1];
	if(z_ok) SOM::xyz[2]+=lasso[2]*lasso_length_rcp_x_relax[2];
	if(u_ok) SOM::uvw[1] = som_mocap_am_relax2(SOM::uvw[1],SOM::L.pcstate[4],step);
	if(v_ok) SOM::uvw[0] = som_mocap_am_relax3(SOM::uvw[0],SOM::L.pcstate[3],step);
		
//	if(0&&EX::debug) //TESTING
//	for(int i=0;i<=2;i+=2) SOM::xyz[i] = SOM::L.pcstate[i];

	if(lean<1) //hack: slow to walking speed when leaned into wall
	Somvector::map(m_s_step).se_scale<3>(lerp(walk,m_s,lean)/m_s); //SOM::walk

	//Reminder: it's important to timing to relax before accumulating
	SOM::uvw[1]-=r_s_step[0]; Somvector::map(SOM::xyz).move<3>(m_s_step);
	SOM::uvw[0]-=r_s_step[1];
	
	//2021: suppress texture AA effect? (NOTE: not the ideal place to do this)
	{
		float fx[2],uu;

		//TODO: would like to factor in dip effects
		for(int i=0;i<2;i++) 
		{		
			float t2 = SOM::uvw[i];
			if(i) t2 = uu = radians(t2,spin_delta[i]);
			else t2-=spin_delta[i];
			//NOTE: pow(2) is masking some funky halting behavior in the 
			//lower range (that might be indicative of a larger problem)
			//NOTE: dx.d3d9c.cpp IS APPLYING A "FLOOR" THRESHOLD TO THIS
			//SO MAYBE THE HALTING IS BELOW THE FLOOR
			fx[i] = pow(fabsf(t2/step),1.5f)*4;
		}
		//looking up/down rolls around the view axis
		fx[1]+=fx[0]*fabsf(SOM::pov[1]);
		if(1)
		{
			const float c = cosf(+SOM::uvw[1]);
			const float s = sinf(+SOM::uvw[1]);
			float x = SOM::doppler[0], z = SOM::doppler[2];
			float xx = x; x = x*c+z*s; z = xx*-s+z*c;
			fx[1]+=fabs(SOM::doppler[1])*2;
			fx[0]+=fabs(x)*2;
			fx[0]+=fabs(z);
			fx[1]+=fabs(z);
		}
		//EX::dbgmsg("fx: %+.6f %+.6f",fx[0],fx[1]);
		//fused was determined to be best for texture AA but maybe if
		//future effects need both this should be a 2D vector
		float cm = Somvector::map(fx).length<2>();
		cm = min(1,cm);
		const float flr = 0.005f; //floor
		cm = (max(cm,flr)-flr)*(1+flr); //ensure won't linger
		DDRAW::fxCounterMotion = cm;
		DDRAW::dejagrate_update_psPresentState();

		if(1&&!mc.suspended) //HACK: unrelated
		{
			//ATTENTION
			//THIS IS GREAT IN THEORY HOWEVER I THINK
			//IT'S CONTRIBUTING TO BUMPINESS IN smooth
			//BECAUSE IT'S ROTATING THE DIGITAL GAITS
			//DISABLING Ex_input_ratchet SEEMS TO HELP
			//IMPROVING springstep WOULD BE CAUSE TO 
			//SEE IF THE RATCHET CAN BE REANABLED. IT'S
			//PRETTY smooth SINCE RECENTLY SO MAYBE THE
			//RATCHET ISN'T NEEDED ANY LONGER

			if(0) uu*=0.9f; //a little leaky?

			const float c = cosf(uu);
			const float s = sinf(uu);
			float &x = EX::Affects[1].position[0];
			float &z = EX::Affects[1].position[2];
			float xx = x; x = x*c+z*s; z = xx*-s+z*c;
		}
	}

	//dash+look gets out of hand
	float nod2 = f4__1?pc->player_character_nod2:pi/2;
	nod2*=1+bendover*1.5f; //EXPERIMENTAL
	
	if(fabsf(SOM::uvw[0])>nod2) SOM::uvw[0] = SOM::uvw[0]<0?-nod2:nod2; 
		
	if(pc->player_character_arm) if(!f4__1)
	{
		SOM::arm[0] = SOM::uvw[0]; 
		SOM::arm[1] = SOM::uvw[1];
		SOM::arm[2] = 0;
	}
	else //TODO: player_character_arm WAS AN EXAGGERATION FACTOR
	{
		//2021: this is to reduce crawling artifacts when the 
		//shield is raised up since it doesn't flash like the
		//weapon is presumed to
		//TODO: would help to close differences about 1 pixel 
		float s = 1+1.5f*(1-SOM::motions.swing)*(1-mc.landspeed);
		float ss = -0.25f+1.25f*SOM::motions.swing;

		SOM::arm[0] = lerp(SOM::arm[0],SOM::uvw[0],antijitter/8*s);
		SOM::arm[1] = slerp(SOM::arm[1],SOM::uvw[1],antijitter);
		//2020: adding roll... this was very simple before 
		//I wanted to combine strafing into turning
		//I developed it with KF2's Bastard Sword since 
		//it swings straight down the middle
		float r = 30-max(fabsf(u),fabsf(sidestep))*20;
		float c = sqrt(fabsf(corkscrew));
		float w = r_s_step[0]/step*2*(1-c);
		w+=sidestep*(1+dashing2*0.5f)*r_s*(3-c*1.5f);
		SOM::arm[2] = lerp(SOM::arm[2],w*ss,antijitter/r*s);
	}

	float q[4]; 		
	Somvector::map(q).quaternion<0,1,0>(-SOM::uvw[1]);

	//NOTE: since Feb. 2021 Affects[1] is incemented by
	//the rotation delta (elsewhere) as if the momentum
	//stays in your body when not airborne
	for(int i=0;i<EX_AFFECTS;i++) 
	Somvector::map(EX::Affects[i].correction).copy<4>(q);

	if(!f4__1) //F4: disable walking effects
	{
		mc.analogbob = 0; return; //!
	}			

	///// locomotion /////////
	
	/*MOVED into "this"*/
	//static bool stopped = true; 

	float landspeed = min(1,mc.speed2/m_s);

	EX::dbgmsg("ls: %f",landspeed);
	
	//float brake = !mc.suspended&&!mc.jumping&&!mc.hopping;
	float brake = !mc.suspended&&!mc.jumping;
	//WARNING: hopping HERE INCLUDES ALL TAP-DASH MOVEMENT
	//NOTE: I should add including it is for the shield to
	//not jerk when hopping. the jerk may be a glitch that
	//will resolve itself later. I've revised this bobbing
	//code a lot today making some strides but there needs
	//to be more innovation I feel
	if(mc.hopping&&!mc.Engine.push)
	{
		//below "brake?0.25f:1" glitches when not jumping
		//whereas 0.25 by itself glitches when jumping :(
		brake = 0;
	}
	float accel = brake?0.25f:1; //0.25f
		
	//2020: prevent crawling do_aa2 artifacts?	
	if(1&&brake)
	if(!mc.Engine.push&&!mc.ducking&&landspeed<0.3f)
	{
		//NOTE: by this point all visible movement should be
		//stopped (if assumptions hold)
		float p = pow(landspeed*3.33333f,1.5f);
		//NOTE: accel was never intended to be used like this
		landspeed*=p; accel*=4*(1-p);
	}

	mc.turnspeed = lerp(mc.turnspeed,u*brake,antijitter);	
	mc.landspeed = lerp(mc.landspeed,landspeed*brake,antijitter*accel);
	//EX::dbgmsg("ls: %f (%f) %f",mc.landspeed,landspeed*brake,mc.ducking);

	if(mc.landspeed>=0.0003f) 
	{	
		//normalize (0: zero divide)	
		float stride = sqrtf(x*x+z*z);
 		float crabwalk = stride?sidestep/stride:0; 

		float pow = powf(stride*crabgain,1.5f); //extension?

		//hack: deemphasize at lower overall speeds
		crabwalk = lerp(crabwalk,0,(1-pow));
		crabwalk = min(max(crabwalk,-1),1); 

		mc.crabwalk = lerp(mc.crabwalk,crabwalk,antijitter);
	}
	else mc.landspeed = mc.crabwalk = 0; 

	float lateral = sidestep*crabgain;

	//0.8f: swing back bias towards center
	float bias = 1-fabsf(mc.swivel)*0.8f, rate = 8; //extension?

	if(bias<0) bias = 0; //paranoia? don't think so???

	const float _75 = .75f, _1_75 = 1/_75;
	//.75: swivel legs 75% of 90 degrees relative to upperbody/kneck
	mc.swivel = _75*lerp(mc.swivel*_1_75,lateral,step*bias*rate); 			  	
	assert(_finite(mc.swivel));

	float reverse = 
	sidestep*sinf(mc.swivel*pi/2)+
	backstep*cosf(mc.swivel*pi/2)<0?-1:1;

	//EX::dbgmsg("backpedal: %f (%f) %f (%f)",mc.backpedal,sidestep,backstep,mc.swivel);

	if(!stopped) //hack
	{
		//inertia: note that movement is not impeded by awkward footing
		mc.backpedal = lerp(mc.backpedal,reverse,step*8); 
	}
	else mc.backpedal = reverse;

	//2020: for footsteps? 
	//this is only effective over about 90 degrees 
	//because of how backpedal works
	//mc.backwards = max(0,-mc.backpedal)*mc.landspeed;
	mc.backwards = lerp(mc.backwards,max(0,-backstep),step*8);
	mc.sideways = lerp(mc.sideways,fabsf(sidestep),step*8);

	//sprintf(EX::dbgmsg,"crab: %f, swivel: %f",mc.crabwalk,mc.swivel);
	//sprintf(EX::dbgmsg,"backpedal: %f, swivel: %f",mc.backpedal,mc.swivel);

	///// analogbob ///////////

	//HACK: trying to account for climbing stairs/hills
	//not by slowing down but speeding up leg movements
	//TODO? sync footsteps & motions with SOM::L.pcstep
	//float speedup = mc.landspeed;	
	float speedup = landspeed; //2021
	if(SOM::doppler[1]>0&&!mc.scaling)
	speedup+=(mc.speed-mc.speed2)/m_s*3; //leg movement?
	
	//2020: need to speed up so footstep sound delay is shorter
	if(op->do_dash)
	if(dashing2&&backstep<0) speedup+=(1-dashing2)*-backstep;

	if(mc.running) //do_dash?
	{		
		//HACK: boosting endurance dash so it gets up to
		//3.333333f down below
		int delta = EX::Joypad::analog_divide(0x7f,7-dt->dash_running_gait);
		speedup+=(1-EX::Joypad::analog_scale(delta))*mc.running;
	}

	bool afoot = speedup>mc.idle; 	

	//dashing is factored in below in the final step
	float freq = SOM::walk*(afoot?speedup:mc.idle);
	
	//2020: adding slow for squat-walking pacing
	freq/=pc->player_character_stride*(1+mc.squat/10);
		
	/*MOVED into "this"*/
	//static float windup = 0;  
	//static float w = SOM::uvw[1]; 		
	float spun = spin(w,SOM::uvw[1])-w;
	if(SOM::warped==SOM::frame) 
	spun = 0; //hack
	windup = afoot?0:windup+spun; w = SOM::uvw[1]; 
	
	if(!afoot&&(!mc.resting||1==mc.resting)) 
	{
		const float dz = pi/2, sa = pi*0.2;				
		mc.goround = min(max(fabsf(windup)-dz,0),sa)/sa;		
	}
	else mc.goround-=mc.goround*step*8;
		
	float freq2 = SOM::_turn/M_PI_2/*90*/*fabsf(mc.turnspeed);
	
	if(freq2>0.0003f) //lazy: smooth transition
	{
		freq2 = lerp(freq,freq2,mc.goround);

		if(!afoot&&!mc.suspended) afoot = freq2>freq; freq = freq2; 
	}
	else windup = 0; //reset

	//NOTE, these are contants
	if(SOM::dash>SOM::walk)
	freq = freq*(1-dashing2)+freq*dash_walk*dashing2;						 
	//TODO? player_character_stride2
	//according to PBS Nova "Making Stuff Faster"
	//~0.3s is the maximum human footstep frequency for all ages
	/*2021: discontinuous??
	if(mc.action>=tvh_ms)*/
	if(op->do_dash)
	freq = min(freq,3.333333f); //EXTENSION
	//EX::dbgmsg("freq: %f (%f)",freq,dashing2);

	if(mc.suspended) afoot = false; //2021

	/*MOVED into "this"*/
	//static float phase = 0;
	if(afoot&&SOM::bob||mc.analogbob) 
	{	
		float before = phase; //NEW
		
		//effectively inverts SOM's sine wave
		float y = cosf(phase+=2*pi*freq*step*mc.backpedal)-1;
		
		//2017: keeping from drifting and normalize for easier testing
		phase = O_to_2pi(phase); 

		if(!afoot||!SOM::bob) //coming to a stop?	
		//if(before*mc.backpedal>=phase*mc.backpedal)
		if(mc.backpedal<0?phase>=before:before>=phase)
		{
			y = phase = 0;
		}

		phase = O_to_2pi(phase); //2021

		mc.analogbob = y;
	}
	else mc.analogbob = phase = 0; 
	
	//EX::dbgmsg("bob: %f (%f)",mc.analogbob,phase);

	/*2021: this helps a lot with hopping 
	//(was speedup ever 0?)
	stopped = !speedup;*/
	stopped = !afoot&&!mc.analogbob; //2021
												 
	//EX::dbgmsg("y: %f, phase: %f, z: %f",mc.analogbob,phase,mc.backpedal);
	//sprintf(EX::dbgmsg,"goround: %f",mc.goround);	

/*#ifdef _DEBUG
#define CHECK(f) \
if(!_isnan(SOM::f)&&!_finite(SOM::f)) strcpy(EX::dbgmsg,#f " is NaN");\
if(!_finite(SOM::f)&&!_finite(SOM::f)) strcpy(EX::dbgmsg,#f " is infinite");

	//these hidden variables can spoil the view matrix
	CHECK(landspeed) CHECK(crabwalk) CHECK(goround) CHECK(swivel) CHECK(error)
	CHECK(turnspeed) CHECK(backpedal) CHECK(bounce) 

#undef CHECK
#endif*/	
}

template<class No>
static float som_mocap_bob(No &ext, float dash=1)
{
	//2017: if player_character_bob is unspecified, provide
	//a 0.04+pc[12]*0.035 style formula. the classic bob is
	//just too much for a walk animation. but OK for dashes
	//This is 0.075/(2-pc[12]). it's better with the change
	//to the walk effect in 1.2.1.8. it was not good before
	float o = ext;
	//if(!&ext) o/=2-SOM::motions.dash;
	if(!&ext) o*=0.3f+0.7f*SOM::motions.dash*dash;
	return o;
}

static float som_mocap_soft_nod(float u)
{
	//this is designed to be imperceptible just to take
	//some of the edge off the stop so it's not noticed

	float nod = EX::INI::Player()->player_character_nod;

	float soft = fabsf(u/nod);

	nod = min(max(u,-nod),nod);

	return nod-0.1f*_copysign(powf(min(1,soft),8),nod);
}

extern float(*SomEx_output_OpenXR_mats)[4][4];
float som_mocap::camera::operator()
(float (&analogcam)[4][4], float (&steadycam)[4][4], float swing[6])
{		
	if(SOM::black==SOM::frame) //HACK
	{
		//2020: this is intended to cancel residual movement leaving 
		//item or NPC screens that are activated by the action input
		mc.action = 0; 
	}

	float step = 0; if(!EX::context()) //YUCK
	{
		step = mc.time_lapse; mc.time_lapse = 0; 
	}
	float antijitter = min(1,step*15); 

	if(EX::isNaN(rsshock_absorber))
	rsshock_absorber = SOM::xyz[1];
	rsshock_absorber = //2021
	lerp(rsshock_absorber,SOM::xyz[1],antijitter);
	
	extern float *som_hacks_kf2_compass;
	if(float*x=som_hacks_kf2_compass) if(step)
	{		
		//note: there are two sets of rotation/translation that apply before
		//and after scaling and lets you change rotation order. it's kind of
		//clever actually
		float &aa = x[8]; //x[8] = -SOM::uvw[1]; //20
		float &bb = x[19]; //x[19] = SOM::uvw[0]; //7

		enum{ delay=20*2 };
		static float *buf = new float[delay]();
		float a = buf[0]; //8
		float b = buf[1]; //19
		memmove(buf,buf+2,sizeof(float)*(delay-2));
		buf[delay-2] = -SOM::uvw[1]; 
		//buf[delay-1] = -SOM::uvw[0];
		float ext = pi/10; //25

		if(DDRAW::xr)
		{
			buf[delay-1] = som_mocap_soft_nod(SOM::uvw[0]);
		}		
		else buf[delay-1] = min(max(SOM::uvw[0]/5,-pi/5),pi/5-ext)/2;
		
		//HACK: try to keep out of view if hidden
		//float fade = x[0x1f]; fade = 5-3.0f*fade;		

		float ss = radians(aa,a);
		float tt = b-bb;
		static float s = 0, t = 0; //REMOVE ME	
		//if(DDRAW::xr)
		//t = lerp(bb,b,step*max(1,fade*fabs(tt)/step))-bb;
		//else
		t = lerp(t,tt,step*max(1,4*pi-powf(fabs(tt)/step,1.5)));
		s = lerp(s,ss,step*max(1,4*pi-powf(fabs(ss)/step,1.5)));

		aa+=s; bb+=t;

		//MATCH GAUGE BEHAVIOR FOR NOW?
		if(DDRAW::xr) bb = buf[delay-1];
	}

	const float *pcstate = SOM::L.pcstate;			
	
	//REMOVE ME?
	typedef Somvector::vector<float,4> float4;
	typedef Somvector::matrix<float,4,4> float4x4;

	//radians: this mainly just normalizes the turn angle now
	if(SOM::emu||fabsf(radians(SOM::uvw[1],pcstate[4]))>pi/6)
	{	
		if(SOM::emu)
		Somvector::map(SOM::xyz).copy<3>(pcstate).set<3>(SOM::err);
		if(SOM::emu||!pcstate[3]) 
		SOM::uvw[0] = SOM::arm[0] = pcstate[3];
		SOM::uvw[1] = SOM::arm[1] = pcstate[4];
	}
														   
	float nod = SOM::uvw[0];
	float bob = SOM::L.view_matrix_xyzuvw[1]-pcstate[1]-1.5f; 

	EX::INI::Player pc; EX::INI::Adjust ad;		
	
	//2017: if player_character_bob is unspecified, provide
	//a 0.04+pc[12]*0.035 style formula. the classic bob is
	//just too much for a walk animation. but OK for dashes
	//This is 0.075/(2-pc[1landspeed2]). it's better with the change
	//to the walk effect in 1.2.1.8. it was not good before
	float player_character_bob = 
	som_mocap_bob(pc->player_character_bob);
	
	if(&pc->player_character_bob3) //EXPERIMENTAL (2020)
	{
		player_character_bob = 
		pc->player_character_bob3(player_character_bob,0);
	}		
	//2020: assuming won't go over running height?
	//player_character_bob*=1+mc.squat/3;
	float squat = 1+mc.squat/3;
	
	//2018: IS THIS EVEN THE RIGHT PLACE FOR THIS?
	//HACK: this isn't a perfectly smooth function
	//and may be noticeable with higher amplitudes
	//because it's directly inputted into SOM::eye
	//(it looks alright with the higher/older bob)
	//NOTE: approaching 0 must be perfectly smooth
	float ls = mc.landspeed; 

	//*2020: to eliminate do_aa2 crawling artifacts I've made
	//landspeed to cut itself off so that I don't know if 
	//this is still necessary, but I think having two layers
	//of fudging is probably not right... still it feels 
	//better like this?
	if(1) if(ls<0.5f) //2018: prevent robotic gliding 
	{
		//discontinuous approaching 0 :(
		//ls = powf(ls,0.1f)/2;
		//works, but can it be better?
		//ls = powf(ls,0.5f)/1.4f;
		ls = powf(ls,0.4f)/1.5f; //better. limit?
	}//*/

	if(!SOM::emu) 
	{
		float amp = squat*player_character_bob;
		SOM::eye[3] = amp/2*ls*mc.analogbob;
	}
	else SOM::eye[3] = bob;
		
	//TODO: move "sway" logic elsewhere as
	//it is no longer part of SOM emulation

	float sway_x = 0, sway_y = bob, sway_z = 0;
	
	if(!SOM::emu&&player_character_bob)
	{	
		sway_y = -mc.analogbob; 		

		//hack: working backward
		//float sway_w = -SOM::eye[3]/som_mocap_bob;		
		float amp = ls; //mc.landspeed;

		/*MOVED into "this"*/
		//static float diff = 0;		
		//static int sway = 1, sign = 1, hack = 1;
		//static bool stopped = true;
		 					
		if(stopped) //leading foot 
		{
			//todo: support leading with left foot
			sway = mc.backpedal<0?-1:1; sign = 1;

			//Reminder: foot switching differs for a 
			//crabwalk maintaining a defensive stance

			//this assumes the buttons are pressed 
			//simultaneously. There has to be a way
			//to saturate the input to do this right
			if(sway>0&&mc.crabwalk>0) sway = -sway;
			if(sway<0&&mc.crabwalk<0) sway = -sway;
		}

		stopped = mc.landspeed+fabsf(mc.goround)<0.0003f;

		int z = mc.backpedal<0?-1:1;

		bool reversing = z!=hack; hack = z;	//hack?

		float phase = mc.Engine.Analog.phase; //NEW		

		//these make the shield jerk on jumps?
		//turning them off forces sign to be 1
		//when entering menus
		//if(fabsf(sway_y)>0.0003f)
		//if(sway_y!=diff) //maybe harmless?		
		if(!EX::context()) //try this instead?
		switch(sway_y>=diff?1:-1)
		{
		case +1: climbing: //climbing
				
			if(sign!=1&&!reversing) //trough (sin)
			{
				sway = -sway; //hack: inverting state
			}			
			sign = 1; break;

		case -1: //falling
				
			//2017: edge case. 
			if(phase>pi&&diff2<pi||phase<pi&&diff2>pi)
			goto climbing;

			if(sign!=-1&&!stopped) //NEW/testing
			{
				//was playing footstep sound here, but it needs to begin earlier
			}	
			sign = -1; //falling
		} 				
		if(diff<1&&sway_y>=1&&!stopped)
		if(!SOM::motions.aloft&&!mc.bounce)
		{
			som_mocap_footstep_soundeffect();
		}
		diff = sway_y, diff2 = phase;
		
		//2017: I'm pretty sure this was adding an extra 
		//little curve where the sign changed. but there
		//is still some new noise from somewhere I think
		//float x = sway_w*sway; 		
		float x = amp*sinf(acosf(mc.analogbob+1)/2)*(float)sway;
		
		//WARNING: THIS INTRODUCES MOVEMENT FOR ZERO analogbob
		/*2021: this is causing the shield to glitch sometimes
		//need to figure out if necessary and solve the glitch
		//(and tie it to analogbob. how??)
		//NOTE: I still see a similar glitch (maybe this isn't 
		//it afterall?) but it seems very infrequent with this
		//disabled		
		x+=amp*(mc.crabwalk-mc.swivel);*/
		
			float bob2;

		//NEW: decoupling SomEx.ini.h bob2 setting
		if(&pc->player_character_bob2||!&pc->player_character_bob)
		{
			bob2 = som_mocap_bob(pc->player_character_bob2);
		}
		else bob2 = player_character_bob;

		if(&pc->player_character_bob3) //EXPERIMENTAL (2020)
		{
			bob2 = pc->player_character_bob3(0,bob2);
		}
			x*=squat*bob2;

		//2017: moving this out of player_character_bob2
		//can't recall if it was this way before. but it's
		//not contributing anything. should it be _bob3?
		//is 0.0375 the classic bob height? Or arbitrary?
		x-=mc.goround*mc.turnspeed*0.0375f*mc.analogbob*sway;
		
		//FYI: SOM reverses these for no real reason ???
		sway_x = x*cosf(-SOM::uvw[1]+mc.swivel*pi/2); 
		sway_z = x*-sinf(-SOM::uvw[1]+mc.swivel*pi/2);
		
		//these are for the F6 function overaly
		SOM::eye[0] = x*cosf(mc.swivel*pi/2); 
		SOM::eye[2] = x*-sinf(mc.swivel*pi/2);
	}

	//REMINDER: squat-transitions now look like dashing 
	//but it's just a decision so to not draw attention
	SOM::eye[1] = pc->player_character_stature;
	float shrinkage = (SOM::eye[1]-pc->player_character_duck2)/SOM::eye[1];	
	float amp = 1-shrinkage, inv = mc.inverting-mc.inverting*mc.reverting;	
	//inv*inv: blends y/y2 when getting back up
	float shrinkray = sinf(pi+inv*inv/2*pi)*amp+1;
	//sprintf(EX::dbgmsg,"inverting: %f (%f)",mc.inverting,mc.ducking);
	//mc.turning also for walking
	if(/*mc.ducking&&*/!SOM::emu) 
	{	
		//EX::dbgmsg("dashing: %f %f (%d)",mc.dashing,mc.ducking,SOM::frame);

		amp = pc->player_character_duck/2;		
		SOM::eye[1]-=-cosf(min(1,mc.turning)*pi)*amp+amp; //NEW						
		
		float duck = mc.ducking;		
		static const float sfactor = 0.30f; //0 ~ 0.5f
		//duck*=cosf(pi*mc.sliding)/2+0.5f;
		duck*=cosf(pi*mc.sliding)*sfactor+(1-sfactor);
		//EX::dbgmsg("slide %f (%f)",duck,mc.ducking);

		//float _ = lerp(-1,1,mc.inverting);		
		float y = -cosf(min(1,duck)*pi)*amp+amp; //mc.ducking
		float _ = -cosf(inv*inv*pi);
	//	if(mc.scaling) _ = -1; //TESTING
		float _y = _*y/shrinkray;
		float y2 = 0;
		float ll = mc.smoothing?mc.leadspring:mc.leadingaway;
		if(mc.holding_xyzt[3])
		{
			if(mc.ducking>1)
			{
				assert(0); //if(EX::debug) MessageBeep(-1);

				goto crouch;
			}

			//EX::dbgmsg("holding: %f %f (%d)",mc.holding_xyzt[3],mc.ducking,SOM::frame);
			
			float t = 1-mc.ducking;

			t*=ll; //similar to earlier code below
			//t*=1-mc.backwards; //TESTING: opening door?			
			//trying different combinations (this one feels good)
			t = sqrtf(t);
			//t*=sqrtf(0.5f+0.25f*mc.sideways+0.25f*mc.backwards);
			t*=0.5f+0.25f*mc.sideways+0.25f*mc.backwards;
			//t = sqrtf(t);			

			//same as dashing above
			y2+=(1-cosf(t*pi))*amp*mc.holding_xyzt[3];
		}
		else if(mc.ducking>1) crouch:
		{	
			//todo? pc->player_character_duck2
			amp = (pc->player_character_duck3+_y)/2;
			//if(mc.inverter<0) //getting up?
			amp = lerp(amp,0,inv); //better 
						
			float t = mc.ducking-1;
			y2 = -cosf(t*pi)*amp+amp;

			//2020: raise up to squat height?
			//test: this is a smooth transition to squat but is way too high up
			//(might want to change the squat-transition)
			//amp = ((pc->player_character_duck2-pc->player_character_duck)+_y)/2;
			amp = (pc->player_character_duck2+_y)/2;
			amp = lerp(amp,0,inv);
			float y3 = -cosf(t*pi)*amp+amp;

			t*=ll; //away is for squat walking

			//REMINDER: this needs to end on a flat line because the squat
			//transition is the inverse of the duck's (might change that?)
			//y2-=sinf(t*pi/2)*y2/2;
			y2-=sinf(t*pi/2)*(y2-y3); //2020
			//TESTING
			//const float cfactor = 7/8.0f, cf2 = 1/cfactor;
			//const float cfactor = 3/4.0f, cf2 = 1;
			//y2-=(cosf(pi+powf(t,1)*cfactor*pi)/2+0.5f)*cf2*(y2-y3); //2020
		}

		float _y_y2 = _y-y2; if(1)
		{				
			if(som_mocap_fastwalker) //EXPERIMENTAL
			{
				float fw = min(1,max(0,mc.fastwalk-mc.ducking));
				_y_y2-=fw*pc->player_character_duck;
			}

			//TODO: general purpose shock absorbers could help stairs

			//I set this up to try to take the edge off the squatting
			//transition without changing its simple set up but could
			//not get it to look nice... however it seems like a good
			//idea to damper all ducking heights... and that seems to
			//do the trick for the squat transition. it will mask any
			//discontinuities in transitions. ideally there shouldn't 
			//be any but there are some difficult cases that might be
			//best left to smoothing out

			if(step) //reposition does memory[0] = memory[1]
			{
				mc.duckspring.stiffness = 0.5f;
				mc.duckspring.reposition(_y_y2,step); //YUCK
			}
			_y_y2 = mc.duckspring;
		}
		SOM::eye[1]+=_y_y2;
	}
	SOM::eye[1]*=shrinkray;	
		
	float bent = 0; if(1) //looking out over ledge?
	{
		//WARNING: swing IS VERY DELICATELY 
		//BALANCED ACCORDING TO MAXIMUM bent (~0.69)
		//FURTHER DOWN BELOW (bemt)

		bent = fabsf(nod)-pc->player_character_nod;
		float duck = max(0,mc.ducking-1);
	//	float hack = bent;
		//0.0001f ensures nonzero for normalizing
		bent*=1-*mc.bending_soft;
		//sin is not very smooth but looks better
		//running into a wall and has a cool thing
		//it does when ducking
		//bent = sinf(max(0,bent));
		//bent = 1-1*cosf(max(0,bent));
		bent = 0.5f+cosf(max(0,bent*2))/-2;
		//the animation fights when the collision shape
		//is lowered down on top of it, so I guess for
		//now just slide it out and hope there is enough
		//tolerance to not see-through the object?		
	//	mc.bending_soft[0] = SOM::eye[1]-hack/4;
		{
			float t = 0.25f; //1/4 is arbitrary		

			//UNSTABLE STANDING UP
			//how to factor in duck2? ideally this makes
			//moving the dpad hang around the same height
			//t-=t*duck*mc.resting*0.5f;

			SOM::eye[1]-=bent*t; 
		}
		//mc.bending_soft[0] = SOM::eye[1];

		if(nod>0) bent = -bent;	

		mc.bent = bent; //2020

		//reel in when ducking? I don't quite understand this...
		//it seems like the clipper isn't working more than anything
		//but this pulls the view back since crouching extends the 
		//view much further (until it clips some geometry)
		duck*=sqrtf(mc.leading2); //*(1-*mc.bending_soft);
		bent*=1-0.1f*duck; //0.05f
		//EX::dbgmsg("bent: %f (%f) %f",bent,duck,mc.resting);

		//HACK: offset analogcam
		sway_x+=bent*-sinf(SOM::uvw[1]); 
		sway_z+=bent*cosf(SOM::uvw[1]);
	}

	if(!SOM::emu) //landing and buckle effects 
	{
		//really-simple-shock-absorber
		//if(!EX::debug||~GetKeyState(VK_CAPITAL)&1)
		SOM::eye[1]+=rsshock_absorber-SOM::xyz[1]; //2021

		//if(nod>-pi/2) //bending?
		float floor = min(nod,-pi/2);
		{
			//WARNING: these effects are subjective
			//rather than being realistic since the
			//FOV is nothing like IRL (what of VR?)

			float zoom = SOM::zoom2();
			
			float center = pi/7;
			float bias = lerp(pi,pi/4,(nod-center)/(pi/4));
			bias = max(pi/4,min(pi,bias));
												
			//NEW: this depends on the FOV //VR?
			//can go higher with the reduction below
			//float dip2 = pi/8; //pi/9
			float dip2 = pi/7; 
			//FIX ME
			// 
			// pi/9 is too little in VR but jumping
			// seems too much or too fast
			// 
			int todolist[SOMEX_VNUMBER<=0x1020602UL];
		//	if(zoom<70||EX::debug) //VR? //???
			if(zoom<70||1) //2022
			dip2 = lerp(pi/9,dip2,(zoom-30)/32);
			else dip2 = pi/9; //VR?
			
			if(&pc->player_character_dip2) //1.2.3.2
			dip2 = pc->player_character_dip2(dip2,zoom);
			dip2*=min(1,max(0,1-fabsf(center-nod)/bias)); //!!!

			//TESTING			
			//NOTE: using cos/acos here might suggest nosedive is a strictly
			//linear input. not much thought has been put into generating it 
			//TODO: PROBABLY THIS SHOULD DEPEND SOME ON zoom (FIELD-OF-VIEW)
			if(0)
			{
				//nod = min(nod,mc.nosedive*bias/6); //discontinous
				nod+=mc.nosedive*bias/6;	
			}
			else //cos?
			{
				//tail or no tail?
				nod-=acos(mc.nosedive*2+1)*bias/pi/6;
				//nod-=acos(mc.nosedive+1)*2*bias/pi/6;	
			}
			nod-=dip2*mc.bounce;			
			assert(!mc.buckle); //UNUSED?
		//	nod = lerp(nod,floor,mc.buckle);	
			nod = max(nod,floor);

			float b2 = mc.bounce; 
			if(mc.scaling_bouncing)
			{
				//2020: scaling has a bounce effect now 
				//but it doesn't duck so it needs to be
				//canceled out somehow
				b2 = pow(1-mc.scaling,1.5f)*mc.bounce;
			}

			//NEW: this depends on the FOV //VR?
			float dip = 0.45f; //62?
			if(zoom<40) //guessing
			dip = lerp(0.2f,0.3f,(zoom-30)/10);
			else if(zoom<60) //guessing
			dip = lerp(0.3f,dip,(zoom-40)/20);			

			if(&pc->player_character_dip) //1.2.3.2			
			dip = pc->player_character_dip(dip,zoom);

			//2002: don't go as far when squatting
			//(makes sense but I just want to keep
			//Aleph's head above sea-level in KF2)
			dip*=1-mc.inverting*0.5f;

			SOM::eye[1]-=dip*b2; 
		//	SOM::eye[1]*=1-mc.buckle; //UNUSED?
			//SOM::eye[1]+=mc.nosedive*pc->player_character_duck; 
			SOM::eye[1]+=(cosf(mc.nosedive*pi)-1)/2*pc->player_character_duck;
		//	EX::dbgmsg("buckle: %f",mc.buckle);
		}
		
		//ARBITRARY: avoid going through floor
		if(SOM::eye[1]<0.3f) SOM::eye[1] = 0.3f;	
	}	
		
	//hack: F6 overlay	
	SOM::incline = nod; SOM::heading = SOM::uvw[1];

	float difference = SOM::eye[1]-1.5f+SOM::eye[3]-bob;	
	
	//// analog /////////////////////////////////////

	float q[4], Euler[3] = {nod,SOM::uvw[1],0};		 

	float4x4 &cam = float4x4::map(analogcam); //!

	cam.copy_quaternion<3,4>(float4::map(q).quaternion(Euler).flip<3>().unit<4>());
									 
	float Y = SOM::xyz[1]+SOM::eye[1]+SOM::eye[3];

	//NOTE: SOM::cam is further altered down below
	//it's mainly the target for billboard effects
	SOM::cam[0] = SOM::xyz[0]+sway_x;
	SOM::cam[1] = Y;
	SOM::cam[2] = SOM::xyz[2]+sway_z;
	SOM::cam[3] = Y-0.3f-SOM::xyz[1]; //1.2
	float4 &pos = Somvector::series(analogcam[3],-SOM::xyz[0]-sway_x,-Y,-SOM::xyz[2]-sway_z,1);

	if(DDRAW::xr) //OpenXR? //2022
	{
		EX::INI::Stereo vr;

		float pose[4+3] = 
		{
			-q[0],-q[1],-q[2],q[3], //flip<3>?
			SOM::cam[0],SOM::cam[1],SOM::cam[2]
		};
		//TODO: probably need to change to OpenGL like convention
		{
			pose[0] = -pose[0];
			pose[1] = -pose[1];
			pose[6] = -pose[6];
			DDRAW::stereo_locate_OpenXR(SOM::fov[2],SOM::skyend,pose);
			pose[0] = -pose[0];
			pose[1] = -pose[1];
			pose[6] = -pose[6];
		}
		
			//stereo_locate_OpenXR bakes uvw[0] into its pose because
			//it uses OpenXR to computet the correct roll for the set
		//	float _[4],Euler[3] = {SOM::uvw[0],SOM::uvw[1],0};
		//	float _[4],Euler[3] = {0,SOM::uvw[1],0};
			float v[4],Euler[3] = {0,-SOM::uvw[1],0};
			Somvector::map(v).quaternion(Euler).unit<4>();
			memcpy(SOM::pos,pose+4,sizeof(SOM::pos));
			Somvector::map(SOM::pos).rotate<3>(v);
			for(int i=3;i-->0;) 
			SOM::cam[i]+=SOM::pos[i]; SOM::cam[3]+=SOM::pos[1];
		
		//TODO: probably need to change FROM OpenGL like convention
		{
				//CLEAN ME UP
				
			//THIS CODE IS COPIED AS-IS FROM som_mocap_BMI055Integrator
			//TODO: IT WILL SURELY REQUIRE SOME MODIFICATIONS!

			//for X to work looking up/down the POV
			//must be included inside the quaternion		
			float (&Q)[4] = *(float(*)[4])pose;
			//is PSVR inverted? inverting makes pitch and roll oscillate
			//WRT yaw
		//	float Qinv[4] = {-Q[0],-Q[1],-Q[2],Q[3]};
			float Qdup[4] = {Q[0],Q[1],Q[2],Q[3]}; //Qinv
			Somvector::multiply(v,Qdup,Q);
	
			//the pitch and roll trade places
		//	float v[3] = {0,0,1};
		//	Somvector::map(v).rotate<3>(Q);

			//NOTE, THE ANGULAR COMPONENTS HAVE BEEEN SWAPPED
			//the SIXAXIS may be sideways, or just convention
			float Qw = Q[3], Qx = Q[0], Qy = Q[1], Qz = Q[2];
			float Y = -atan2(2*Qy*Qw+2*Qx*Qz,1-2*(Qz*Qz+Qw*Qw)); // Yaw 
			float X = -asin(2*(Qy*Qz-Qw*Qx)); // Pitch 
			float Z = atan2(2*Qy*Qx+2*Qz*Qw,1-2*(Qx*Qx+Qz*Qz)); // Roll 

		//	float X = acosf(v[1]);
		//	EX::dbgmsg("v: %.3f %.3f %.3f",v[0],v[1],v[2]);

			//som_mocap_403BB0 needs this adjusted
			Ex_output_f6_head[4] = X;
		//	Ex_output_f6_head[5] = -Y+M_PI; //Qinv?
			Ex_output_f6_head[5] = Y+M_PI;
			
			Ex_output_f6_head[6] = Z; 

			//som_state_pov_vector_from_xyz3_and_pov3?
			SOM::hmd[0] = -X;
			SOM::hmd[1] = -Y+M_PI;
			//somehow this causes som_scene_compass to miscalculate
			//the angle that is used to fade out (hide) the compass 
		//	SOM::hmd[2] = -Z; //or +Z?
			SOM::hmd[2] = 0;
		}
		memcpy(Ex_output_f6_head,pose,4*sizeof(float));
	//	memcpy(Ex_output_f6_head+4,Euler,3*sizeof(float));
		memcpy(Ex_output_f6_head+7,pose+4,3*sizeof(float));

		//if(browsing_menu)
		{
			extern void //hack: keep SOM::pov uptodate
			som_state_pov_vector_from_xyz3_and_pov3();
			som_state_pov_vector_from_xyz3_and_pov3();

			float s = EX::stereo_font; s = s?1/s:1; s*=0.5f;

		  //REMOVE ME? this doesn't depend on VR factors
			
			//screen to viewport (2D)
			float m[4][4] = {};	
			m[0][0] = -1/(SOM::fov[0]*240/320/2)*(320/240.0f);
			m[1][1] = -1/(SOM::fov[1]/2);
			m[2][2] = -1/s; //HACK...
			m[3][0] = 1+0.8f; //ARBITRARY //DUPLICATE
			m[3][1] = 1;
			m[3][2] = 0;			
			m[3][3] = 1;
			for(int i=15;i-->0;) m[0][i]*=s;

			//world space transform (3D)
			float n[4][4];				
			float Euler[3] = {-SOM::uvw[0],SOM::uvw[1]+M_PI};
			Somvector::map(n).rotation<4,4,'yxz'>(Euler);

			//1 seems to far away, but anything less
			//makes kf2_demo_bevel to show the sides
			//at some oblique angle that's invisible
			float z[3] = {0,0,1.1f};
			SOM::rotate(z,SOM::uvw[0],SOM::uvw[1]);	

			for(int j=3;j-->0;)
			n[3][j] = SOM::cam[j]-SOM::pos[j]+z[j];

			auto &menuxform = SOM::stereo344[0];

			if(1)
			Somvector::multiply<4,4>(m,n,menuxform);
			else
			memcpy(menuxform,n,sizeof(n)); //TESTING

			m[2][2] = -0.09f;
			m[3][0] = 2.69f*s;
			float mn[4][4];
			Somvector::multiply<4,4>(m,n,mn);
			for(int v=DDRAW::xr;v-->0;)
			{
				auto &o = SomEx_output_OpenXR_mats[v];
				auto &p = DDRAW::stereo_locate_OpenXR_mats[v];
				Somvector::multiply<4,4>(mn,p,o);
			}
			//second menu layer?
			float submenu = SOM_STEREO_SUBMENU+0.1f;
			m[3][2]+=submenu;
			Somvector::multiply<4,4>(m,n,mn);
			for(int v=DDRAW::xr;v-->0;)
			{
				auto &o = SomEx_output_OpenXR_mats[DDRAW::xr+v];
				auto &p = DDRAW::stereo_locate_OpenXR_mats[v];
				Somvector::multiply<4,4>(mn,p,o);
			}
			m[3][2]-=SOM_STEREO_SUBMENU; //restore

		  //ONSCREEN ELEMENTS//
			
			EX::INI::Option op;
			
			//TODO: make floaty like compass?
			//float nod = EX::INI::Player()->player_character_nod;
			//float x19 = min(max(SOM::uvw[0],-nod),nod);
			float x19 = som_mocap_soft_nod(SOM::uvw[0]);

			float m11 = m[1][1], m22 = m[2][2], m30 = m[3][0];
							
			//double size gauge and compass?
			float xx,l_xx; 
			//try to copy som_scene_compass?
			for(int ii=op->do_nwse?1:2;ii-->0;)
			{
				xx = !ii&&op->do_kf2?3:1/s/2; l_xx = 1/xx; 

				float side = ii==1?-1:1;

				//NOTE: the only difference on this path is the
				//addition of x[8] that doesn't fit into 3 Euler
				
				//HACK: just using this build the transformation
				float x[sizeof(SOM::MDO)/4] = {};
								
				float ext = M_PI/30;
				x[19] = x19+ext;
				//doesn't need to act like a compass?
			//	x[8] = -SOM::uvw[1];
				x[8] = 0;

				x[13] = -1.025f*side;
				x[14] = +1.025f;
				x[15] = +1.025f;
				SOM::rotate(x+13,x19,SOM::uvw[1]);	

				//NOTE: could be done below since this
				//isn't doing the fade test
				for(int i=3;i-->0;) x[13+i]-=SOM::pos[i];

				if(1)
				{
					float extra1 = 0, extra2 = 0;
					if(x19>0)
					{
						extra2 = x19*0.25f; //rolls???			
						//x[8]-=x19*0.7f;
						x[8]+=x19*0.7f*side;
					}
					else
					{
						extra1 = x19*-0.3f;
						extra2 = x19*-0.2f;
					}
					x[19]+=ext*4.0f+extra1;
					//x[20] = SOM::uvw[1]-ext*7.0f+extra2;
					x[20] = SOM::uvw[1]+ext*7.0f*side-extra2*side;
				}

				for(int i=3;i-->0;) x[13+i]+=SOM::cam[i]; //-SOM::pos[i]

					//for some reason it's bacward from the compass
					x[19] = -x[19]; 
					//x[20]+=M_PI*1.25f;
					x[20]+=M_PI;

				//x[25] = x[26] = x[27] = xx;
				//TODO: 445cd0 is very inefficient... this code can implement
				//it 
				//this won't work because it doesn't store a matrix internally
				//((void(__cdecl*)(void*))0x445cd0)(x);
				{
					auto &Euler1 = *(float(*)[3])(x+7); //x[8] only?
					auto &Euler2 = *(float(*)[3])(x+19);

					//NOTE: if not for scaling these could be quaternions
					//but I don't know if 'yxz' can be done by reordering
					//Euler angles either

					//NOTE: x[25], x[26], x[27] should scale non-uniformly
					Somvector::map(mn).rotation<4,4,'yxz'>(Euler1).scale<3,3>(xx);

					//TODO: here som_MDO::trans1 needs to be multiplied by
					//this matrix and added to it (trans1 isn't used here)

					float nn[4][4];
					Somvector::map(nn).rotation<4,4,'yxz'>(Euler2); //rot2

					for(int i=3;i-->0;) nn[3][i]+=x[13+i]; //trans2

					Somvector::multiply<4,4>(mn,nn,n);
				}

				//NOTE: I don't quite understand why l_xx
				m[2][2] = -1*l_xx;
				m[3][0] = s*(1+0.8f); //ARBITRARY //DUPLICATE

				m[1][1] = ii==1||!op->do_kf2?m[0][0]:m11;

				Somvector::multiply<4,4>(m,n,SOM::stereo344[1+ii]);
			}

			if(op->do_kf2)
			{		
				//NOTE: I don't quite understand why l_xx
				m[1][1] = m11;
				m[2][2] = -0.09*l_xx;
				m[3][0] = m30;
				Somvector::multiply<4,4>(m,n,mn);
				for(int v=DDRAW::xr;v-->0;)
				{
					auto &o = SomEx_output_OpenXR_mats[2*DDRAW::xr+v];
					auto &p = DDRAW::stereo_locate_OpenXR_mats[v];
					Somvector::multiply<4,4>(mn,p,o);
				}
			}
			
			//fade like som_scene_compass?
			extern float som_hacks_hud_fade[2]; 
			for(int i=op->do_nwse?1:2;i-->0;)
			if(SOM::stereo344[1+i][3][3])
			{
				float v[3] = {};
				v[0]+=SOM::fov[0]*0.5f; //0.33f;
				v[1]+=SOM::fov[1]*0.5f; //0.29f;
				Somvector::map(v).premultiply<4>(SOM::stereo344[1+i]);
				for(int j=3;j-->0;) v[j]-=SOM::cam[j];

				float d = Somvector::map(v).unit<3>().dot<3>(Somvector::map(SOM::pov));
				float r = acosf(d);
				float rr = M_PI_4*0.5f;
				float fade = 6*powf(max(0,rr-r)/rr,2.0f);
				fade = min(1.0f,fade);

				//REMOVE ME?				
				som_hacks_hud_fade[i] = fade;
			}
		}
	}

	float back[3] = {};
	if(!DDRAW::inStereo)
	if(SOM::zoom<50&&SOM::pov[1]) //1: pow issue 
	{
		//was normalizing SOM::pov[0] and pov[2], but that is a
		//problem in F4 mode, when looking straight up/down.
		back[0] = sinf(-SOM::uvw[1])*sqrt2; 
		back[2] = cosf(-SOM::uvw[1])*sqrt2;	

		//2020: I feel very short when looking down using 50 and
		//below (note: back only applies to 50 and below)
		back[1] = -2.5; 

		//back up so you can see above and below
		//20: this is just 50-30 covering the zoom modes in full
		//4: works surprisingly well for 1.5m tall player character
		assert(SOM::zoom>=30); 
		//reminder: pow can't help negative numbers
		float up = powf(fabsf(SOM::pov[1]),1.5f)/4;		
		Somvector::map(back).se_scale<3>(float(50-SOM::zoom)/20*up);		
		pos.move<3>(back);
	}	
	Somvector::map(SOM::cam).remove<3>(back);
		
	pos.s<3>().premultiply<3>(cam.s<3,3>()); 
					
	//// sky model //////////////////////////////////

	//REMOVE ME?
	//where SOM believes the center of the sky to be
	float out = 1.5f+bob;

	float4x4 &cam2 = cam2.map(steadycam).identity<3,4>(); //! 

	float4::map(steadycam[3]).copy<3>(pcstate).flip<3>().se<3>() = 1;
	float4::map(steadycam[3]).move<3>(SOM::xyz).se<1>()+=difference;
	
	steadycam[3][0]+=sway_x; steadycam[3][2]+=sway_z;
	
	cam2.premultiply<4,4>(cam);

	//// swing model ////////////////////////////////

	if(!swing) return out;
	
	memcpy(swing,SOM::xyz,sizeof(float)*3);
	if(!SOM::emu&&pc->player_character_arm
	 &&SOM::L.f4) //2024: there's a bug
	{
		//2021: apply motion to shield?
		float l = SOM::motions.swing;
		l+=(1-l)*mc.bounce; //helpful?
			//BLACK MAGIC
			//after 0.5f the arm floats to the center
			//of the screen???
			//it's very strange... just scaling it is
			//unwieldy... between 0.5 and 0.7 there's
			//a big change along the vertical, but by
			//sway_x/z, not swing[1]
			float bemt = bent; if(bemt>0)
			{
				//EX::dbgmsg("bemt: %f",bemt);
				bemt = min(1,bemt/0.69f); //max bent?
				bemt = 0.5f*sqrtf(bemt);
			}
			else bemt = -bemt;
		l = bemt*(1-l)+(0.25f+0.75f*l); //bent
		swing[0]+=l*sway_x;
		swing[1]+=difference-(1-l)*SOM::eye[3];
		swing[2]+=l*sway_z;
		//EX::dbgmsg("sway: %f,%f",sway_x,sway_z);

		//1 locks the shield with the head animation
		//but I think it represents neck movement more
		//than upperbody movement
		float shield = 0.5f*(SOM::incline-SOM::uvw[0]); //1
		assert(shield<=0);
		//shield = max(-pi/6,shield);
		shield+=SOM::arm[0];
		float nod2 = pc->player_character_nod2;
		shield = max(-nod2,shield);

		assert(!mc.buckle); //UNUSED?
	//	swing[3] = lerp(SOM::arm[0],nod,mc.buckle*0.5f);
		swing[3] = 1?shield:SOM::arm[0]; 
		swing[4] = SOM::arm[1]; 
		float w = SOM::arm[2]/SOM::_turn;
		swing[5] = w*M_PI/60;
		swing[1]-=fabsf(w)/60;
	}
	else
	{ 
		swing[0]+=sway_x;
		swing[1]+=difference; //hack
		swing[2]+=sway_z;
		swing[4] = SOM::uvw[1]; 
		swing[3] = SOM::uvw[0];
		swing[5] = 0;
	}

	//OBSOLETE?
	//som_scene_449d20_swing is experimenting with a fixed field-of-view
	/*if(0||!EX::debug)
	if(!DDRAW::inStereo)
	if(SOM::zoom<50) //push arm up front
	{
		//4: derived from look up/down adjustment above
		//20: this is just 50-30 covering the zoom modes in full

		const float fudge = 1.25f; //science!!
		float up = float(50-SOM::zoom)/20/4*fudge;
		float push[3]; Somvector::map(push).copy<3>(SOM::pov).se_scale<3>(up);
		//Somvector::map<3>(swing).move<3>(push).remove<3>(back);
		Somvector::map<3>(swing).move<3>(push);
	}*/			
	Somvector::map<3>(swing).remove<3>(back);

	return out;	//REMOVE ME?
}
 
void som_mocap::dbgmsg()
{		
	EX::dbgmsg("\
	1 interrupted %d\n\
	2 landing_2017 %d\n\
	3 squat %f\n\
	4 analogbob %f\n\
	5 landspeed %f\n\
	6 crabwalk %f\n\
	7 goround %f\n\
	8 swivel %f\n\
	9 turnspeed %f\n\
	10 backpedal %f\n\
	11 bounce %f\n\
	12 buckle %f\n\
	13 dashing %f\n\
	14 ducking %f\n\
	15 peaking %f\n\
	16 jogging %f\n\
	17 running %f\n\
	18 fleeing %f\n\
	19 resting %f\n\
	20 jumping %f\n\
	21 landing %f\n\
	22 leaning %f\n\
	23 leading %f\n\
	24 scaling %f\n\
	25 falling %f\n\
	26 ceiling %f\n\
	27 turning %f\n\
	28 upsadaisy %d\n\
	29 tunnel_running %d\n\
	30 inverter %f\n\
	31 reverter %f\n\
	32 inverting %f\n\
	33 reverting %f\n\
	34 leadingaway %f\n\
	35 auto_squatting %f\n\
	Camera\n\
	0 sway %d\n\
	1 sign %d\n\
	2 stopped %d", 
	/*1*/interrupted,
	/*2*/landing_2017,
	/*3*/squat,
	/*4*/analogbob,
	/*5*/landspeed,
	/*6*/crabwalk,
	/*7*/goround,
	/*8*/swivel,
	/*9*/turnspeed,
	/*10*/backpedal,
	/*11*/bounce,
	/*12*/buckle,
	/*13*/dashing,
	/*14*/ducking,
	/*15*/peaking,
	/*16*/jogging,
	/*17*/running,
	/*18*/fleeing,
	/*19*/resting,
	/*20*/jumping,
	/*21*/landing,
	/*22*/leaning,
	/*23*/leading,
	/*24*/scaling,
	/*25*/falling,
	/*26*/ceiling,
	/*27*/turning,
	/*28*/upsadaisy,
	/*29*/tunnel_running,
	/*30*/inverter,
	/*31*/reverter,
	/*32*/inverting,
	/*33*/reverting,
	/*34*/leadingaway,
	/*35*/auto_squatting,
	//camera
	/*0*/Camera.sway,
	/*1*/Camera.sign,
	/*2*/Camera.stopped);
}

/*float SOM::Climber::reach(float y)
{
	if(!SOM::L.f4||SOM::motions.aloft||SOM::ladder>SOM::frame-8)
	starting_point[2] = FLT_MAX;
									   
	if(SOM::slope<=0.667458f) //0x4583F0 in radians.
	memcpy(starting_point,SOM::L.pcstate,3*sizeof(float));

	float d = Somvector::measure<3>(starting_point,&SOM::L.pcstate);
	d/=EX::INI::Player()->player_character_shape*som_mocap::sqrt2;
	//I don't see anyway to find a leaning/climbing equilibrium so
	//it is serving to end/frustrate the climb. It works very well.
	//The only alternative vision is to lean in like a normal wall.
	//float l = powf(mc.leaning,2);
	return min(1,d);//max(l,min(1,d)); 
}*/

  ////Nolo VR ////Nolo VR ////Nolo VR ////Nolo VR ////Nolo VR ////Nolo VR
  ////Nolo VR ////Nolo VR ////Nolo VR ////Nolo VR ////Nolo VR ////Nolo VR
  ////Nolo VR ////Nolo VR ////Nolo VR ////Nolo VR ////Nolo VR ////Nolo VR

namespace NOLOVR
{
	//#define NOLO_VERSION 113

	//NoloMath.h
	typedef float NVector2[2];
	typedef float NVector3[3];
	typedef float NQuaternion[4]; //xyzw
	typedef unsigned int UINT;
	typedef unsigned char byte;

	//Nolo_DeviceType.h
	enum ENoloDeviceType
	{
		eHmd = 0,
		eLeftController,
		eRightController,
		eBaseStation
	};
	enum EControlerButtonType 
	{
		ePadBtn     = 0x01,
		eTriggerBtn = 0x02,
		eMenuBtn    = 0x04,
		eSystemBtn  = 0x08,
		eGripBtn    = 0x10,
		ePadTouch   = 0x20
	};
	enum ERotationType //NEW
	{
	eNoloRotation = 0x01,
	eHmdRotation
	};

	#pragma pack(push,1) //FixedEyePosition is unaligned???

	struct Controller
	{
		int VersionID;
		NVector3 Position;
		NQuaternion Rotation;
		UINT Buttons;
		int Touched;
		NVector2 TouchAxis;
		int Battery;
		int State;
		Controller() //x64?
		{
			//#pragma pack(push,1)
			//4+12+16+4+4+8+4+4 = 56
			assert(sizeof(*this)==56);
		}
	};
	struct HMD
	{
		int HMDVersionID;
		NVector3 HMDPosition;
		NVector3 HMDInitPostion;
		UINT HMDTwoPointDriftAngle;
		NQuaternion HMDRotation;
		int HMDState;

		HMD() //x64?
		{
			//#pragma pack(push,1)
			//4+12+12+4+16+4 = 52
			assert(sizeof(*this)==52);
		}
	};
	struct BaseStation
	{
		int BaseStationVersionID;
		int BaseStationPower;
	};
	struct NoloSensorData
	{
		NVector3 vecLVelocity;
		NVector3 vecLAngularVelocity;
		NVector3 vecRVelocity;
		NVector3 vecRAngularVelocity;
		NVector3 vecHVelocity;
		NVector3 vecHAngularVelocity;
	};
	struct NOLOData
	{
		Controller leftData;
		Controller rightData;
		HMD hmdData;
		BaseStation bsData;
		byte expandData[64];
		NoloSensorData NoloSensorData;

		//NEW
		UCHAR leftPackNumber;
		UCHAR rightPackNumber;
		NVector3 FixedEyePosition;
	};
	#pragma pack(pop) //FixedEyePosition is unaligned???

	class INOLOZQMEvent
	{
	public:
		virtual void OnZMQConnected() = 0;
		virtual void OnZMQDisConnected() = 0;	
		virtual void OnKeyDoubleClicked(ENoloDeviceType DevType, UCHAR Keys) = 0;		
		virtual void OnNewData(const NOLOData &_noloData) = 0;
		virtual void OnNoloDevNeedUpdate(int Versions) = 0;		
	};
	/*C support I think.
	typedef void(__cdecl *pfnKeyEvent)(ENoloDeviceType DevType, UCHAR Keys);
	typedef void(__cdecl *pfnVoidCallBack)();
	typedef void(__cdecl *pfnDataCallBack)(const NOLOData &leftData);
	typedef void(__cdecl *pfnVoidIntCallBack)(int Versions);
	enum EClientCallBackTypes
	{
		eOnZMQConnected = 0,  
		eOnZMQDisConnected,       
		eOnButtonDoubleClicked, 
		eOnNewData,        // pfnDataCallBack
		eOnNoloDevVersion, // pfnVoidIntCallBack
		eCallBackCount
	};*/

	//NoloClientLib.h
	bool (__cdecl*StartNoloServer)(const wchar_t *StrServerPath);
	void (__cdecl*SetEventListener)(INOLOZQMEvent *Listener);
	void (__cdecl*SetHmdCenter)(const NVector3 &hmdCenter); //0.00f,0.08f,0.08f
	bool (__cdecl*OpenNoloZeroMQ)();
	void (__cdecl*CloseNoloZeroMQ)();
	void (__cdecl*TriggerHapticPulse)(ENoloDeviceType deviceType, int intensity); //50-100
	//OnNewData covers this I think?
	//NOLOData (__cdecl*GetNoloData)();
	//Controller (__cdecl*GetLeftControllerData)();
	//Controller (__cdecl*GetRightControllerData)();
	//HMD (__cdecl*GetHMDData)();

	//C interface
	//Older or newer version?
	//https://github.com/NOLOVR/NoloDeviceSDK
	//void (__cdecl*RegisterCallBack)(EClientCallBackTypes callBackType, void *pCallBackFun);
}

//EXPERIMENTAL
extern SOM::NoloClientLib *SOM::NoloVR = 0;
static struct som_mocap_Nolo : NOLOVR::INOLOZQMEvent, SOM::NoloClientLib
{		
	struct Status
	{
		float rotation[4], location[3];

	}status[3],center[3]; 
		
	unsigned frame;

	//EXPERIMENTAL
	//Nolo operates at 60hz, so I think there's a possiblity
	//that two frames are using the same position
	EX::critical _cs;
	Status current;
	std::vector<Status> queue; //HACK
	Status pop()
	{
		EX::section cs(_cs);
		if(queue.empty()) return current;
		Status o = queue.back(); queue.pop_back(); return o;
	}

	void recenter()
	{
		for(int i=0;i<3;i++)
		{
			Status &s = status[i], &t = center[i];
			Somvector::map(t.rotation).copy<4>(s.rotation).flip<3>().unit<4>();
			Somvector::map(t.location).copy<3>(s.location);			
		}
	}

	virtual void OnZMQConnected()
	{
		thread = GetCurrentThread();

		assert(THREAD_PRIORITY_NORMAL==GetThreadPriority(thread));
		SetThreadPriority(thread,THREAD_PRIORITY_ABOVE_NORMAL);
	}
	virtual void OnZMQDisConnected()
	{
	}	
	virtual void OnKeyDoubleClicked(NOLOVR::ENoloDeviceType DevType, UCHAR Keys)	
	{
	}
	virtual void OnNewData(const NOLOVR::NOLOData &noloData)
	{
		//CRITICAL-SECTION?
		//This code is currently executing in the Nolo API thread. I think the 
		//32-bit components are atomic writes, but it may be that Z for example
		//ends up using the previous sample. GetNoloData is another option that
		//would not necessitate a critical-section based access pattern.

		Somvector::map(status[0].rotation).copy<4>(noloData.hmdData.HMDRotation);
		Somvector::map(status[0].location).copy<3>(noloData.hmdData.HMDPosition);

		if(started==1){ started = 2; recenter(); }

		//I think the API thread may be starved?
		EX::dbgmsg("Nolo: %d",SOM::frame-frame); //Nolo.com says 60hz frequency

		frame = SOM::frame;		

		//ALGORITHM
		//Since Nolo is running at 60hz, and since the game is too, this method
		//may be entered between 0 and 2 times each frame. The animation can be
		//choppy if the samples are doubled up.
		EX::section cs(_cs);
		Status *s = status, *t = center;
		Somvector::map<3>(current.location).copy<3>(s[0].location).remove<3>(t[0].location);
		Somvector::multiply(t[0].rotation,s[0].rotation,current.rotation);
		if(!queue.empty())
		{
			queue.resize(2,queue.back()); queue[0] = current;
		}
		else queue.push_back(current);		
	}	
	virtual void OnNoloDevNeedUpdate(int Versions)
	{
		//???
	}

	HANDLE thread;

	som_mocap_Nolo():thread(),frame()
	{
		SOM::NoloVR = this;
				
		EX::INI::Stereo vr;
		const wchar_t *dir = vr->StartNoloServer_folder;
		if(!*dir) dir = L"VR";

		int llN = 0;
		wchar_t ll[MAX_PATH+1];
		if(PathIsRelative(dir))
		{
			GetModuleFileNameW(EX::module,ll,MAX_PATH);
			llN = PathFindFileName(ll)-ll;
			wcscpy_s(ll+llN,MAX_PATH-llN,dir);
			llN+=wcslen(ll+llN);
		}
		else wmemcpy(ll,dir,llN=wcslen(dir));
		ll[llN++] = '\\';
		//NoloClientLib loads this, but VR directory
		//is not in the PATH environment variable :(
		wcscpy_s(ll+llN,MAX_PATH-llN,L"libzmq-32.dll");
		LoadLibraryW(ll);
		wcscpy_s(ll+llN,MAX_PATH-llN,L"NoloClientLib.dll");		
		dll = LoadLibraryW(ll);
		DWORD err = GetLastError();		
	
		if(!dll) return;		
		//These PEiD ordinals return incorrect entrypoints... maybe they
		//are off by 1???
		#define _(x,y) (void*&)NOLOVR::y = \
		GetProcAddress(dll,#y/*MAKEINTRESOURCEA(x)*/); assert(NOLOVR::y);
		/*1.1.3
		_(0x1C,StartNoloServer)
		_(0x1A,SetEventListener)
		_(0x1B,SetHmdCenter)
		_(0x19,OpenNoloZeroMQ)
		_(0x11,CloseNoloZeroMQ)
		_(0x1D,TriggerHapticPulse)
		_(0x17,GetNoloData)
		*/
		//UNVERSIONED (apparently newer SDK)
		//TODO: later SDK uses extern "C" linkage
		_(0x23,StartNoloServer)
		_(0x21,SetEventListener)
		_(0x22,SetHmdCenter)
		_(0x1F,OpenNoloZeroMQ)
		_(0x1A,CloseNoloZeroMQ)
		_(0x24,TriggerHapticPulse)
		//_(0x1D,GetNoloData)
		//_(0x20,RegisterCallBack) //NEW (or old?)
		#undef _	

		//Not directory then?
		//ll[--llN] = '\0';
		wcscpy_s(ll+llN,MAX_PATH-llN,L"NoloServer.exe");

		if(NOLOVR::StartNoloServer)
		if(started=NOLOVR::StartNoloServer(ll))
		{
			if(NOLOVR::SetEventListener)
			NOLOVR::SetEventListener(this);

			//documentation suggests this. I think it's a vector from the
			//center (of?) to the unicorn component's ball piece
			float center[3] = {0.00f,0.08f,0.08f};
			if(NOLOVR::SetHmdCenter)
			NOLOVR::SetHmdCenter(center);

			if(NOLOVR::OpenNoloZeroMQ)
			NOLOVR::OpenNoloZeroMQ();
		}
	}
}*som_mocap_Nolo = 0;

extern void som_mocap_NOLO(int center)
{
	//relaxing this since I messed up and made a patch with it disabled
	//#if 0 || defined(NDEBUG)
	//return; //DON'T FORGET ME
	//#endif
	assert(SOM::stereo||DDRAW::inStereo);	
	if(!SOM::stereo&&!som_mocap_Nolo) return;

	if(!som_mocap_Nolo) 
	{
		som_mocap_Nolo = new struct som_mocap_Nolo;
	}
	if(!SOM::NoloVR->started) return;

	if(center) som_mocap_Nolo->recenter();
}


  ////PlayStation VR ////PlayStation VR ////PlayStation VR ////PlayStation VR
  ////PlayStation VR ////PlayStation VR ////PlayStation VR ////PlayStation VR
  ////PlayStation VR ////PlayStation VR ////PlayStation VR ////PlayStation VR



  //BMI055 is Sony's gyro/accelerometer
//https://github.com/gusmanb/PSVRFramework/blob/master/PSVRFramework/BMI055Integrator.cs
static struct som_mocap_BMI055Integrator
{
	typedef double scalar2; //float;

	typedef struct som_mocap_Nolo Nolo; 
	
	static bool nolo() //temporary
	{
		return SOM::NoloVR&&SOM::frame-som_mocap_Nolo->frame<5; 
	}

	template<typename scalar> struct Madgwick
	{
		scalar beta;
		scalar out[4]; //MadgwickAHRS.Quaternion
		scalar ibp[4]; //ZeroPose (inverse bind pose)

		Madgwick(scalar b):beta(b){}
		
		void MadgwickAHRS(scalar2 g[3], scalar2 a[3], scalar2 Beta, scalar2 SamplePeriod)
		{
			AHRS(g[0],g[1],g[2],a[0],a[1],a[2],Beta,SamplePeriod);
		}

		//https://github.com/gusmanb/PSVRFramework/blob/master/PSVRFramework/Madgwick.cs
		void AHRS(scalar gx, scalar gy, scalar gz, scalar ax, scalar ay, scalar az, scalar Beta, scalar SamplePeriod)
		{
			// short name local variable for readability
			scalar qw = out[3], qx = out[0], qy = out[1], qz = out[2];   
			scalar norm;
			scalar s1, s2, s3, s4;
			scalar qDot1, qDot2, qDot3, qDot4;

			// Auxiliary variables to avoid repeated arithmetic
			scalar _2qw = 2 * qw;
			scalar _2qx = 2 * qx;
			scalar _2qy = 2 * qy;
			scalar _2qz = 2 * qz;
			scalar _4qw = 4 * qw;
			scalar _4qx = 4 * qx;
			scalar _4qy = 4 * qy;
			scalar _8qx = 8 * qx;
			scalar _8qy = 8 * qy;
			scalar qwqw = qw * qw;
			scalar qxqx = qx * qx;
			scalar qyqy = qy * qy;
			scalar qzqz = qz * qz;

			// Normalise accelerometer measurement
			norm = sqrt(ax * ax + ay * ay + az * az);
			if (norm == 0) return; // handle NaN
			norm = 1 / norm;  // use reciprocal for division
			ax *= norm;
			ay *= norm;
			az *= norm;

			// Gradient decent algorithm corrective step
			s1 = _4qw * qyqy + _2qy * ax + _4qw * qxqx - _2qx * ay;
			s2 = _4qx * qzqz - _2qz * ax + 4 * qwqw * qx - _2qw * ay - _4qx + _8qx * qxqx + _8qx * qyqy + _4qx * az;
			s3 = 4 * qwqw * qy + _2qw * ax + _4qy * qzqz - _2qz * ay - _4qy + _8qy * qxqx + _8qy * qyqy + _4qy * az;
			s4 = 4 * qxqx * qz - _2qx * ax + 4 * qyqy * qz - _2qy * ay;		
			norm = 1 / sqrt(s1 * s1 + s2 * s2 + s3 * s3 + s4 * s4); // normalise step magnitude
			s1 *= norm;
			s2 *= norm;
			s3 *= norm;
			s4 *= norm;

			// Compute rate of change of quaternion
			qDot1 = 0.5f * (-qx * gx - qy * gy - qz * gz) - Beta * s1;
			qDot2 = 0.5f * (qw * gx + qy * gz - qz * gy) - Beta * s2;
			qDot3 = 0.5f * (qw * gy - qx * gz + qz * gx) - Beta * s3;
			qDot4 = 0.5f * (qw * gz + qx * gy - qy * gx) - Beta * s4;

			// Integrate to yield quaternion
			qw += qDot1 * SamplePeriod;
			qx += qDot2 * SamplePeriod;
			qy += qDot3 * SamplePeriod;
			qz += qDot4 * SamplePeriod;
			norm = 1 / sqrt(qw * qw + qx * qx + qy * qy + qz * qz); // normalise quaternion
			out[3] = qw * norm;
			out[0] = qx * norm;
			out[1] = qy * norm;
			out[2] = qz * norm;
		}
	};
	
	/*
	enum AScale
	{
		AFS_2G = 0x03,
		AFS_4G = 0x05,
		AFS_8G = 0x08,
		AFS_16G = 0x0C
	};
	float GetAres(AScale Scale)
	{
		switch(Scale)
		{
			case AScale.AFS_2G: return 2/2048.0f;
			case AScale.AFS_4G: return 4/2048.0f;
			case AScale.AFS_8G: return 8/2048.0f;
			case AScale.AFS_16G: return 16/2048.0f;
		}
		return 0;
	}
	enum Gscale
	{
		GFS_2000DPS = 0,
		GFS_1000DPS,
		GFS_500DPS,
		GFS_250DPS,
		GFS_125DPS
	};
	float GetGres(Gscale Scale)
	{
		float factor = 0;
		switch (Scale)
		{
			case Gscale.GFS_125DPS: factor = 0.00381f; break;
			case Gscale.GFS_250DPS: factor = 0.007622f; break;
			case Gscale.GFS_500DPS: factor = 0.01524f; break;
			case Gscale.GFS_1000DPS: factor = 0.03048f; break;
			case Gscale.GFS_2000DPS: factor = 0.06097f; break;
		}
		return factor*M_PI/180;
	}	
	void Init(AScale AccelerometerScale, Gscale GyroscopeScale)
	{
		aRes = GetAres(AccelerometerScale);
		gRes = GetGres(GyroscopeScale);   
	}
	*/

	//TODO: TRY <double> BUT FLOAT SEEMS MORE STABLE, DRIFTWISE
	//IT'S HARD TO SAY THOUGH, AND AS TO WHY IS A MYSTERY IF SO
	Madgwick<float> antidrift;
	Madgwick<scalar2> steadycam;
	som_mocap_BMI055Integrator()
	:antidrift(0.1f/*0.125f*/) //RATIONAL FRACTION
	,steadycam(/*0.05*/0.035)
	,recalibrate(true)
	,calibrating(true)
	,recenter()
	,samplesLeft(samplesN) //2000	
	,bt()//,id(true)
	{
		//BMI055Integrator.Init(BMI055Integrator.AScale.AFS_2G, BMI055Integrator.Gscale.GFS_2000DPS);
		/*
		(A):
		2 g 1024 LSB/g
		4 g 512 LSB/g
		8 g 256 LSB/g
		16 g 128 LSB/g
		*/
		aRes = 2/2048.0; 
		
		/*
		(G):
		125d/s 262.4 LSB/d/s
		250d/s: 131.2 LSB/d/s
		500d/s: 65.6 LSB/d/s
		1000d/s: 32.8 LSB/d/s
		2000d/s: 16.4 LSB/d/s 
		*/
		//gRes = 0.06097f*M_PI/180; 
		//gres = 0.06097560975609756097560975609756*M_PI/180;
		gRes = scalar2(0.00106422515365507901031932363932);

		Ex_output_f6_head[3] = 1;
	} 

	scalar2 aRes,gRes;
		
	scalar2 accelOffset[3],gyroOffset[3];
		
	int prevTimestamp; //uint prevTimestamp;

	//SOM: related to GFS_2000DPS? 2000 degrees/s?
	//2000 is 1s, but if the beta parameters are changed more time may
	//be added to finish converging the calibration. Otherwise it will
	//move after the calibration phase and may be inadequate (although
	//it looks alright?)
	enum{ samplesN=2000 };

	int samplesLeft; 

	bool recalibrate,calibrating;   
	void Recalibrate(){ calibrating = recalibrate = true; Recenter(); }

	bool recenter;
	void Recenter()
	{
		Ex_output_f6_head[4] = Ex_output_f6_head[6] = 0; recenter = true;
	}
	
	int bt; //bool id; 
	void Position(SOM::Sixaxis sa[2])
	{
		if(recalibrate)
		{
			samplesLeft = samplesN;
			Somvector::series(accelOffset,0,0,0);
			Somvector::series(gyroOffset,0,0,0);
			recalibrate = false;
		}

		bool id = true;
		scalar2 la[2][3],aa[2][3];
		if(sa) 
		{	  
			for(int i=0;i<2;i++)
			{
				Somvector::map(la[i]).copy<3>(sa[i].motion).se_scale<3>(aRes);
				Somvector::map(aa[i]).copy<3>(sa[i].gyro).se_scale<3>(gRes);
				id = Integrate(la[i],aa[i],sa[i].time);
			}
			bt|=sa->button;
		}			

		//ASSUMING BETTER TO DO THIS WITH EACH SENSOR REPORT
		//NOT ONCE-PER-FRAME
		if(!id) if(1||!EX::debug) ////Q = Quaternion.Inverse(ibp*out);
		{
			//slerp/attractor: should be aggressive as long as head is not still					
			//http://www.euclideanspace.com/maths/algebra/realNormedAlgebra/quaternions/slerp/
			scalar2 cosHalfTheta = 
			Somvector::map(steadycam.out).dot<4>(Somvector::map(antidrift.out));
			if(abs(cosHalfTheta)<1)
			{						
				//Calculate temporary values.
				scalar2 halfTheta = acos(cosHalfTheta);
				scalar2 sinHalfTheta = sqrt(1-cosHalfTheta*cosHalfTheta);
				//if theta = 180 degrees then result is not fully defined
				//we could rotate around any axis normal to qa or qb
				scalar2 a,b;
				//might want to play with cutoff?
				const scalar2 exact = 0.1;
				const scalar2 cutoff = 0.0025;
				if(abs(sinHalfTheta)>=cutoff) //0.001; //zero divide?
				{
					scalar2 t = halfTheta*M_2_PI; 
					//SHOULD NOT ENTER UNLESS HEAD IS MOVED
					//would like to be able to see this... in the headset
					//EX::dbgmsg("t %f (%d)",t,SOM::frame);
					if(1) 
					{
						t = abs(t);
								
						if(t<exact) //worried about roll
						{
							//t = 1-t;
							//steadycam.out[2] = antidrift.out[2];
							t = pow(t,0.25);
						}
					}
					else t*=t; //absolute value/power2

					assert(t>0&&t<=1);

					a = sin((1-t)*halfTheta)/sinHalfTheta;
					b = sin(t*halfTheta)/sinHalfTheta; 
				}
				else goto eq; //a = b = 0.5f; 
						
				for(int i=0;i<4;i++) //calculate Quaternion.
				{
					steadycam.out[i] = a*steadycam.out[i]+b*antidrift.out[i];
					//steadycam.ibp[i] = a*steadycam.ibp[i]+b*antidrift.ibp[i];		
				}
				Somvector::map(steadycam.out).unit<4>();
				//Somvector::map(steadycam.ibp).unit<4>();
			}
			else goto eq; //assert(0);

		eq:; //Somvector::multiply(steadycam.ibp,steadycam.out,Q);
		}
		//else Somvector::multiply(antidrift.ibp,antidrift.out,Q);
		////these are inverted for VIEW multiply
		////Somvector::map(Q).flip<3>();
	}
	void Position_once_per_frame()
	{
		if(samplesLeft>=-samplesN)
		{
			Ex_output_f6_head[5] = -SOM::uvw[1];			
					
			return;
		}

		//which routine calls this first?
		static unsigned was = SOM::frame;
		if(was==SOM::frame) return;
		was = SOM::frame;

		scalar2 Q[4] = {0,0,0,1}; //if(!id) 
		{
			scalar2 q[4];
			//scalar2 sign = -1;
			switch(bt) //switch(sa->button)
			{
			case 8:
				extern int SomEx_shift();
				if(-1==SomEx_shift()) Recalibrate(); 
				else Recenter(); break;
				/*
			case 2: sign = 1; case 4:		
				Somvector::map(q).quaternion<1,0,0>(sign*M_PI/20000);
				Somvector::map(ibp).postmultiply?(q).unit<4>();
				*/
				//using this for setting IPD since 
				//going backward is so complicated
			case 2: case 4: 		
				if(0==EX::context())
				if(SOM::frame%8==0&&~SOM::altf&(1<<3)) 
				{			
					bool shift = 4==bt;
					if(SOM::ipd!=(shift?0:30))
					SOM::altf3(shift); 
					else SOM::altf|=1<<3;
				}
			}
			bt = 0;

			if(1&&nolo()) //NEW
			{				
				//60hz seems to step on its own feet
				Nolo::Status st = som_mocap_Nolo->pop();

				float *p = Ex_output_f6_head+7; //C++
				Somvector::map<3>(p).copy<3>(st.location);

				Somvector::map(Q).copy<4>(st.rotation);

				//Invert just angle works? Or seems to.
				//Q[3] = -Q[3];
				//Invert works too. Is it more correct?
				Somvector::map(Q).flip<3>().unit<4>();				
			}
			else 
			{
				if(1||EX::debug)
				{
					Somvector::multiply(steadycam.ibp,steadycam.out,Q);
				}
				else Somvector::multiply(antidrift.ibp,antidrift.out,Q);
				//these are inverted for VIEW multiply
				//Somvector::map(Q).flip<3>();

				//can't do this before MadgwickAHRS (without rewriting)
				std::swap(Q[0],Q[1]);
				
				//can't hurt
				Somvector::map(Q).unit<4>();
			}			
								
			//0~4 is the raw view vector
			//4~6 is the frustum's shape
			Ex_output_f6_head[0] = Q[0];
			Ex_output_f6_head[1] = Q[1];
			Ex_output_f6_head[2] = Q[2];
			Ex_output_f6_head[3] = Q[3];									  			
			//Report.Orientation = ToEuler(Report.Pose);
			//Vector3 ToEuler(Quaternion Q)
			{			
				//Vector3 pitchYawRoll = new Vector3();
				
				//for X to work looking up/down the POV
				//must be included inside the quaternion		
				scalar2 Qinv[4] = {-Q[0],-Q[1],-Q[2],Q[3]};
				scalar2 Euler[3] = {SOM::uvw[0],SOM::uvw[1],0};
				Somvector::multiply(Somvector::map(q).quaternion(Euler),Qinv,Q);
								
				//NOTE, THE ANGULAR COMPONENTS HAVE BEEEN SWAPPED
				//the SIXAXIS may be sideways, or just convention
				scalar2 Qw = Q[3], Qx = Q[0], Qy = Q[1], Qz = Q[2];								
				scalar2 X,Y,Z;
				/*Report.Orientation.*/X = -atan2(2*Qy*Qw+2*Qx*Qz,1-2*(Qz*Qz+Qw*Qw)); // Yaw 
				/*Report.Orientation.*/Y = -asin(2*(Qy*Qz-Qw*Qx)); // Pitch 
				/*Report.Orientation.*/Z = atan2(2*Qy*Qx+2*Qz*Qw,1-2*(Qx*Qx+Qz*Qz)); // Roll 				
				Ex_output_f6_head[4] = Y; 
				Ex_output_f6_head[5] = X-M_PI;
				Ex_output_f6_head[6] = Z; 

				//EX::dbgmsg("Yaw %f\nPitch %f\nRoll %f",X,Y,Z); 
				//return pitchYawRoll;	
			}
		}
	}

	bool Integrate(scalar2 (&linearAcceleration)[3], scalar2 (&angularAcceleration)[3], int Timestamp)
	{
		int samples = samplesLeft--;

		Somvector::vector<scalar2,3> 
		&ao = Somvector::map(accelOffset),
		&go = Somvector::map(gyroOffset),
		&la = Somvector::map(linearAcceleration),
		&aa = Somvector::map(angularAcceleration);

		//DISABLING
		enum{ use1G=0, gyro_if_not=0 };

		if(samples>0)
		{
			//skip first half second
			if(!use1G&&!gyro_if_not)
			{
				samples = 0; goto disabled;
			}

			ao.move<3>(la); go.move<3>(aa);   
			return true;
		}
		else if(samples==0) disabled:
		{	
			if(use1G)  
			ao.se_scale<3>(1.0f/samplesN);
			if(use1G||gyro_if_not)
			go.se_scale<3>(1.0f/samplesN);
			
			//I don't want to use this, since it seems to work alright
			//without it, and I fear it will introduce error if the set
			//is not upright when calibrated
			{				
				//gravity????
				//is the unit of acceleration 1G? maybe that's
				//why that normalization is all there is to it					
				scalar2 gravityVector[3];
				Somvector::map(gravityVector).copy<3>(ao).unit<use1G?3:0>();
				//does it matter if the set is upright? seems to
				//approximately 1,0,0 if upright
				//EX::dbgmsg("G %f,%f,%f",gravityVector[0],gravityVector[1],gravityVector[2]);

				if(use1G) 
				{
					ao.remove<3>(gravityVector);
				}	
				else 
				{
					memset(accelOffset,0x00,sizeof(accelOffset));
					if(!gyro_if_not)
					memset(gyroOffset,0x00,sizeof(gyroOffset));
				}
			}
				
			//fusion = new PSVRFramework.MadgwickAHRS(Quaternion.Identity);
			Somvector::series(steadycam.out,0,0,0,1).set<4>(antidrift.out);

			prevTimestamp = Timestamp;

			return true;
		}

		la.remove<3>(ao).unit<3>();
		aa.remove<3>(go);

		scalar2 interval;		
		if(prevTimestamp>Timestamp)
		interval = (Timestamp+(0xFFFFFF-prevTimestamp))/1000000.0;
		else
		interval = (Timestamp-prevTimestamp)/1000000.0;
		prevTimestamp = Timestamp;

		/*
		if(interval>0.002)
		{
			//HACK: throw out anomalous samples... expecting the
			//interval to be an almost constant 0.0005f (2000/s)
			samplesLeft++; return samples>=-samplesN;
		}*/

		EX::INI::Stereo vr;
		//static const float best = 0.1f; //0.05f;
		scalar2 beta = steadycam.beta;
		if(&vr->beta_constant) beta = vr->beta_constant;
		scalar2 beta2 = antidrift.beta; //beta_constant2?

		//scalar2 beta; 
		if(samples>-samplesN/4*3) //-1500
		{
			beta = beta2 = 1.5f; //calibrating
		}
		else if(samples>-samplesN)
		{
		//	beta = best; //0.05f; //calibrating
		}
		//just eliminating branch
		//else //... //beta = best; //beta = 0.035f;

		//MadgwickAHRS(angularAcceleration,linearAcceleration,beta,interval);		
		steadycam.MadgwickAHRS(angularAcceleration,linearAcceleration,beta,interval);
		antidrift.MadgwickAHRS(angularAcceleration,linearAcceleration,beta2,interval);
		
		if(samples==-samplesN) identity:	
		{
			//WTH? what's * trying to express???
			//ibp = Quaternion.Identity * Quaternion.Inverse(fusion.Quaternion);
			//conjugate/normalize to get inverse
			//Somvector::map(ibp).copy<4>(out).flip<3>().unit<4>();			
			//Somvector::map(steadycam.ibp).copy<4>(steadycam.out).flip<3>().unit<4>();
			Somvector::map(antidrift.ibp).copy<4>(antidrift.out).flip<3>().unit<4>()
			.set<4>(steadycam.ibp);

			//PRETTY? less obnoxious than reeling it in? maybe not
			//memcpy(steadycam.out,antidrift.out,sizeof(steadycam.out));

			calibrating = false;
		}
		else if(samples<-samplesN) 
		{
			//paranoia: don't rollover to INTMAX
			samplesLeft++;

			if(recenter)
			{
				assert(!calibrating);

				recenter = false; goto identity; 
			}

			return false; //return Quaternion.Inverse(ibp*out);
		}

		return true; //return Quaternion.Indentity;
	}

}*som_mocap_head = 0;

extern void som_mocap_NOLO(int); //EXPERIMENTAL
extern void som_mocap_PSVR(SOM::Sixaxis sa[2], int recenter)
{	 	
	if(DDRAW::xr) //OpenXR?
	{
		assert(0); return; //FIX ME?
	}

	if(!som_mocap_head)
	som_mocap_head = new som_mocap_BMI055Integrator;
	if(1==recenter)
	som_mocap_head->Recenter();
	if(2==recenter)
	som_mocap_head->Recalibrate();
	//if(sa)
	som_mocap_head->Position(sa);

	//2020: prevent a long pause when switching into F2
	//mode without a PSVR system
	if(sa||som_mocap_Nolo)
	{
		//Nolo tracking/controller kit?
		som_mocap_NOLO(recenter); 
	}
}

//TODO: CAN USE THIS FOR MORE THAN JUST HEAD-TRACKING!
extern void som_mocap_403BB0()
{		
	//NEW: don't need to do this for every sensor report
	if(som_mocap_head) som_mocap_head->Position_once_per_frame();

	//using without VR to pull back
	//assert(DDRAW::inStereo);	
	
	EX::INI::Stereo vr;

	float fov; if(1||!DDRAW::inStereo) //NEW
	{
		fov = *SOM::cone/2;
	}
	else //increase FOV when head is rolled? //FIX ME?
	{	
		/*2022: was this because PSVR is taller than wide?
		//NOTE: SOM::cone is already divided by 2. shouldn't
		//fovy be prior to lerp? (Note furthe divide above)
		float w = sin(Ex_output_f6_head[6]);			
		float fovy = M_PI/180*vr->zoom;
		fov = som_mocap::lerp(*SOM::cone,fovy,fabsf(w))/2;*/
		fov = *SOM::cone/2;
	}
	
	//SOM likes 300 N to S (300 is basically infinite)
	float s = sinf(fov)*300; 
	float c = cosf(fov)*300; 		
	float frustum_shape[4][2] = 
	{
	// 1------S------3
	//  \	  		/
	//	 \	  	   /
	//	  \	  	  /
	//	   \  	 /
	//		0-N-2
		{   -6,    }, //0
		{ -s-6, +c }, //1
		{   +6,    }, //2
		{ +s+6, +c }, //3
	};

	//BUG-FIX: fix tiles disappearing along vertical
	//(on maps with high/low features)
	{
		float u = SOM::uvw[0];
		u-=Ex_output_f6_head[4];
		u = fabsf(u);
		//better results for now... this decision is
		//more to do with the lingering turn axis issue
		//than the vertical axis
		if(u>M_PI_2) u = M_PI_2; //M_PI_2-u;
		//Extension? or function of draw distance?
		//float ext = 12; //KF2 lighthouse
		//float ext = 20; //KF2 lighthouse (nonlinaer)
		float ext = 25; //Moratheia 2.1
		//I don't understand how 25 doesn't pull back to
		//the middle tile looking straight down with the
		//draw distance set to less than 25
		ext = min(ext,SOM::L.frustum[1]);
		//TODO? may need to extend draw distance somehow
		//(better not to so performance profile is same)	
		float pullback = u*M_2_PI;
		pullback = pullback*pullback*ext;
		//pulling back all 4 so the angle doesn't narrow
		for(int i=0;i<4;i++)
		frustum_shape[i][1]-=pullback;
	}

	if(!DDRAW::inStereo) //NEW
	{
		s = sinf(-SOM::uvw[1]);
		c = cosf(-SOM::uvw[1]);
	}
	else
	{
		s = sinf(Ex_output_f6_head[5]);	
		c = cosf(Ex_output_f6_head[5]);
	}
	for(int i=0;i<4;i++)
	{
		float &x = SOM::L.frustum[2+2*i+0];
		float &z = SOM::L.frustum[2+2*i+1];
		x = frustum_shape[i][0]; 
		z = frustum_shape[i][1];
		float xx = x, zz = z; x = x*c+z*s; z = xx*-s+z*c;	
		x+=SOM::xyz[0];
		z+=SOM::xyz[2];
	}
}

extern bool som_mocap_PSVR_view(float (&out)[4][4], float (&in)[4][4])
{
	if(DDRAW::xr) 
	{
		//I think the OpenXR path keeps the matrix undisturbed
		assert(0); 
	}
	else
	{
		if(1) //RENAME ME
		{
			static unsigned was, print; 
			
			if(was!=SOM::frame)
			{
			//	print = SOM::frame-was;
				was = SOM::frame;		
				//Maybe just my imagination, but it's blurrier
				//to do this in som_hacks_onflip
				SOM::PSVRToolbox();
			}
			//else assert(0); //2020 //this happens normally
			
			/*(2020: or 2000? Microseconds or milliseconds?)
			//interval is like 200 fps (steady) 
			//EX::dbgmsg("interval: %d",print); */
		}
		else SOM::PSVRToolbox();

		//NEW: don't need to do this for every sensor report
		if(som_mocap_head) som_mocap_head->Position_once_per_frame();
		else return false;
	}

	float m[4][4]; assert(DDRAW::inStereo);	
	Somvector::map(m).copy_quaternion<4,4>(Ex_output_f6_head);
	Somvector::map(m[3]).remove<3>(Ex_output_f6_head+7);
	Somvector::multiply<4,4>(in,m,out);
	return true;
}
