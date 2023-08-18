#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

#include "Ex.ini.h"
#include "Ex.output.h"

#include "SomEx.h"
#include "som.state.h"

#define SOMVECTOR_MATH
#include "../Somplayer/Somvector.h"

namespace //som_db constants
{
	const float o1 = 0.01f;
	const float oo1 = 0.001f; //4583E8
	const float oo2 = 0.002f;
}

static float som_clipc_haircut;
static float som_clipc_haircut2[2];
static float som_clipc_climbing;
//this is the master subroutine
//of all of the clipper routines
static float som_clipc_426D60_y;
static float som_clipc_426D60_1 = 0;
static bool som_clipc_slipping = false;
extern bool som_clipc_slipping2 = false; //2020
static bool som_clipc_slipping3 = false; //2021
extern void som_clipc_reset()
{
	som_clipc_slipping = false;
	som_clipc_slipping2 = false;
	som_clipc_haircut2[0] = 0;
	som_clipc_haircut2[1] = 0;	
}
static void __cdecl som_clipc_426D60(FLOAT _1) 
{		
	float *p = SOM::L.pcstate, *p2 = SOM::L.pcstate2;

	//assert(_1); //2018: bug?
	if(!_1) //indicates no time has passed???
	{
		//2020: prevent lateral movement in cases
		//where SOM::g is set to 0 around pausing
		memcpy(p,p2,6*sizeof(float));

		return; //assert(*SOM::g>0.001f);		
	}
	
	const float ddx = p[0]-p2[0], xx = p[0];
	const float ddz = p[2]-p2[2], zz = p[2];

	som_clipc_426D60_y = p[1];
	
	EX::INI::Player pc;
	const float _3 = SOM::L.shape;
	const float radius = pc->player_character_shape;
	const bool landing_2017 = _3>radius;
	
	static BYTE step = 0; //EXPERIMENTAL

	//for some crazy reason SOM does this
	//three times in a row before polling input
	static int pass = 3; if(pass++==3)
	{	
		pass = 1; 
		
		//initializing
		SOM::motions.ceiling = 0; 
		SOM::motions.floor_object = 
		SOM::motions.ceiling_object = EX::NaN; //sound effects
		SOM::motions.cling = false;		 
		float *hc = som_clipc_haircut2;		
		som_clipc_haircut = fabsf(SOM::L.height-hc[0]);
		hc[0] = SOM::L.height;
		som_clipc_haircut+=max(0,p[1]-hc[1]);
		hc[1] = p[1];
		if(som_clipc_climbing) //*SOM::L.pcstep
		som_clipc_haircut+=som_clipc_426D60_1;
		if(som_clipc_haircut) //kind of like a margin-of-error
		som_clipc_haircut+=0.001f; //4583E8 		

		som_clipc_slipping3 = som_clipc_slipping2; //2021
		som_clipc_slipping2 = false; //2020

	}				//F3???? (I think F3 was just for testing)
	if(!SOM::emu/*&&!SOM::f3*/) //skip 1st/2nd pass? 
	{		
		if(0||pass!=3) return; _1*=3;
	}
	if(!SOM::emu) *(float*)0x458334 = 1; //???
	{	
		SOM::L.pcstep = step;

		som_clipc_426D60_1 = _1; //pseudo gravity constant
		//grab beforehand in case running into the ceiling
		som_clipc_climbing = SOM::L.pcstep?SOM::L.pcstepladder:0;		
		((void(__cdecl*)(FLOAT))0x426D60)(_1);
		if(SOM::L.pcstep) SOM::ladder = SOM::frame;
		else SOM::L.pcstepladder = 0; //courtesy				

		//2018: trying to delay climb, but not if falling/recovering
		//(this way the speed can be slowed for the 1st climb frame)
		if(!step&&SOM::L.pcstep)		
		if(!SOM::motions.aloft&&!SOM::emu)
		{
			float &rem = SOM::L.pcstepladder;
			rem+=_1; p[1]-=_1;
		}

		step = SOM::L.pcstep; //EXPERIMENTAL
		
		//WHAT IS THIS??
		if(0&&!SOM::emu) SOM::L.pcstep = 0;	
	}
	*(float*)0x458334 = 0.9f; //???	
	
	if(0&&EX::debug) return;

		//UNTESTED: try to accommodate 30fps?
		//(2020: I'm unsure how to lock at 30fps
		//in windowed mode?)
		float fps = 0.01666666f/SOM::motions.step;

	if(1) //2017: filtering?
	{	
		//2017: EXPERIMENTAL
		//this makes the vertical edges of MHM seem as if they are
		//rounded off when moving around them. otherwise the clipper
		//can act up when switching from one face to the next making 
		//it noticeably kick out in the new direction

		//TODO? frame-rate might affect this
		//closer to 1 is smoother. not too smooth
		//(2020: the math looks correct but I've not tried
		//it this way since I'm unsure how to lock at 30fps
		//in windowed mode?)
		//const float power = 1.125f;
		const float power = powf(1.05f,fps); //1 goes through objects

		float dx = p[0]-xx;
		float dz = p[2]-zz;
		float mag = sqrtf(dx*dx+dz*dz);
		
		//0.025 was based on a 0.5 radius
		//REMINDER: som.MHM.cpp pushes away according the 
		//radius and stem is always 2
		//float lim = 0.025f/2*radius*2; //LOOKS WRONG???
		float lim = 0.025f*radius;
		static float stem = 2; //power of 2

		//REMOVE ME?
		//HACK: extract som.mocap::speed/*2*/.
		float speeds[3]; EX::speedometer(speeds);
			
		//the effect must be disabled at high speeds because it
		//sees through the wall, even if it never falls through
		//THIS MIGHT NOT WORK FOR INHUMAN SHAPES/WALKING SPEEDS
		float impact = speeds[0]/SOM::dash-SOM::walk/SOM::dash;
//		float impact2 = 2-max(0,impact); //2022
		impact = pow(1+max(0,impact),2);
		lim*=impact;
		//EX::dbgmsg("%f --- %f",impact,lim*stem);		
			
		bool test = false;

//		if(out)
		if(mag>lim*stem //stem is at least 2
		&&!landing_2017
		&&SOM::frame-SOM::shoved>15) //2020: testing
		{	
			test = true; if(0)
			{
				test: test = test; //breakpoint
			}

			//WARNING: 1 must be 1 m!
			float x = 1/mag*lim*stem;
				
			//I think this is when
			//falling or something
			x = 1+_3/radius*(x-1); //lerp(1,x,_3/radius)

			//EX::dbgmsg("x: %f (%f)",mag,stem);

			if(x<1) //assert(x<=1); //2022 //test?
			{				
				p[0] = xx+dx*x; //*1/mag*lim*stem;
				p[2] = zz+dz*x; //*1/mag*lim*stem;				

				//increase the limit in case the violent movement is
				//appropriate: e.g. a head-on collision
				if(test) stem = pow(stem,power);
				//WARNING: it's not a mistake that this is reapplied 
				//and not commented out... it actually helps. but it
				//is an accident and I don't understand how it works
			//	if(test) stem = pow(stem,1+(power-1)*x);
				if(test) stem = pow(stem,1+(power-1)*x*impact*impact);

				//EXPERIMENTAL
				SOM::motions.cornering = x;
			}
		}
		else 
		{
			stem = max(2,pow(stem,1/power)); 

			SOM::motions.cornering = 1; //???

			//2022: trying to make "cornering" continuous?
			if(1&&mag&&stem>2) goto test;
		}

		//EX::dbgmsg("impact: %f (%f)",impact,SOM::motions.cornering);
	}

	//2020: friction?
	if(float t=pow(SOM::motions.clinging2,0.8f)) //0.5~1
	{
		//2022: trying to let go if on the lip
		//t*=powf(SOM::motions.cornering,2.0f);
		t*=SOM::motions.cornering;			

		float d2[2] = {p[0]-p2[0],p[2]-p2[2]};
		float d = sqrt(d2[0]*d2[0]+d2[1]*d2[1]);
		float dd = sqrt(ddx*ddx+ddz*ddz);
		//trying to distinguish between desired movement
		//and clipping movement
		/*I FEEL LIKE THESE SHOULD BE DECOUPLED SOMEHOW?		
		if(0)
		{		
			//NEEDS READJUSTMENT ONCE USING NEW "clinging2"
			//add pseudo friction over short distance so it
			//is less slippery when pressing against things		
			const float len = oo2;

			//2022: I think this is a typo since this isn't
			//in local space
			//float dot = ddx/d*d2[0]/d+ddz/dd*d2[1]/d;
			float dot = ddx/dd*d2[0]/d+ddz/dd*d2[1]/d;
			d/=fabs(dot);

			float exp = d/(len*t);
			exp = 2-pow(exp,0.5f);
			if(exp>1) for(int i=0;i<=2;i+=2)
			{
				float di = d2[i?1:0];
				p[i] = p2[i]+_copysign(pow(fabs(di),exp),di);
			}
		}
		else*/ //feels pretty good
		if(SOM::frame-SOM::shoved>15) //TESTING
		{
			const float len = 0.0275f*fps;

			//2022: I think this is a typo since this isn't
			//in local space
			//float xy[2] = {ddx/d*d2[0]/d,ddz/dd*d2[1]/d};
			float xy[2] = {ddx/dd*d2[0]/d,ddz/dd*d2[1]/d};
			for(int i=0,j=0;i<=2;i+=2,j++)
			{
				float d = d2[j]/fabs(xy[j]);
				float exp = d/(len*t);
				if((exp=2-pow(exp,0.5f))>1)
				p[i] = p2[i]+_copysign(pow(fabs(d2[j]),exp),d2[j]);
			}
		}
	}
}

typedef BYTE __cdecl som_clipc_40dff0_t(FLOAT*,FLOAT,FLOAT,DWORD,DWORD,FLOAT*,FLOAT*);
static const auto &som_clipc_x40dff0 = *(som_clipc_40dff0_t*)0x40dff0;
bool SOM::clipper_40dff0(float _1[3],float _2,float _3, int obj, int _5, float _6[3], float _7[3])
{
	auto &ai = SOM::L.ai3[obj];	
	auto *mdl = (SOM::MDL*)ai[SOM::AI::mdl3];
	auto *mdo = (SOM::MDO*)ai[SOM::AI::mdo3]; //assert(mdl||mdo);
	auto *mhm = mdo?mdo->mdo_data()->ext.mhm:mdl?mdl->mdl_data->ext.mhm:0;

	float _[3]; assert(_6);
	float h2 = mhm&&_5&2?ai[SOM::AI::height3]:_2;
	if(!som_clipc_x40dff0(_1,h2,_3,obj,_5&2?1:0,_6,_7?_7:_))
	return false;

	if(mhm)
	{
		//HACK: return results in SOM::clipper
		//SOM::Clipper pclip(_1,_2,_3,_5);
		//if(!pclip.clip(mhm,mdo,mdl,_6,_7?_7:_))
		if(!SOM::clipper.clip(mhm,mdo,mdl,_6,_7?_7:_));
		return false;

		if(!_7&&!_5)
		memcpy(_6,SOM::clipper.pclipos,3*sizeof(float));
	}	
	else 
	{
		SOM::clipper.obj_had_mhm = false;

		if(!_7&&!_5)
		{
			_6[0]+=_3*_[0]; _6[2]+=_3*_[2];
		}
	}

	return true;
}

static void som_clipc_cling(float *xyz) //lerp
{		
	if(EX::INI::Player()->player_character_slip)
	{
		float t = 0&&EX::debug?1: //som_clipc_cling
		SOM::motions.clinging*SOM::motions.clinging;
		xyz[0] = SOM::xyz[0]+t*(xyz[0]-SOM::xyz[0]); 
		xyz[2] = SOM::xyz[2]+t*(xyz[2]-SOM::xyz[2]); 
	}
}
static struct som_clipc_climb_t 
{
	float y,xz[2]; bool operator()(float *xyz)
	{
		if(SOM::motions.clinging<0.5f) return false;
		
		float xz2[2] = {xyz[0],xyz[2]}; 
		return Somvector::measure<2>(xz,xz2)
		<EX::INI::Player()->player_character_shape;
	}	

}som_clipc_climb;
	
static float som_clipc_40D750_4[3];
static BYTE __cdecl som_clipc_40D750(FLOAT *_1,FLOAT _2,FLOAT _3,FLOAT *_4,
FLOAT _5,FLOAT _6,FLOAT _7,FLOAT _8, DWORD _9, FLOAT *_10, FLOAT *_11)
{
	if(!((BYTE(__cdecl*)(FLOAT*,FLOAT,FLOAT,FLOAT*,FLOAT,FLOAT,FLOAT,FLOAT,DWORD,FLOAT*,FLOAT*))
	0x40D750)(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11))
	return 0;

	if(EX::debug) //hinge? (som_clipc_40D750)
	som_clipc_40D750_4[1] = 1; 
	som_clipc_40D750_4[0] = _4[0]; //HACK: return the door's hinge?
	som_clipc_40D750_4[2] = _4[2]; return 1;
}

static float som_clipc_opposite(float adjacent, float clip_y)
{
	float angle = M_PI_2-asinf(clip_y); return tanf(angle)*adjacent;
}

//substitute analog xz for elevation
//NOW also doing ceiling/not doing rt2
extern int *som_clipc_objectstack = 0; 
static float som_clipc_objectelevator; //REMOVE ME
static BYTE __cdecl som_clipc_40dff0 //40D420 (objects)
(FLOAT*_1, FLOAT _2, FLOAT _3, DWORD _4, DWORD _5, FLOAT*_6, FLOAT*_7)
{	
	//TODO: this should be merged into som_MHM_4159A0 soon 

	assert(_1==SOM::L.pcstate);
	
	if(SOM::emu) return som_clipc_x40dff0(_1,_2,_3,_4,_5,_6,_7);
	
	//2022: lateral test order shouldn't matter
	//auto obj = _5?(DWORD)som_clipc_objectstack[_4]:_4;
	auto obj = _5?som_clipc_objectstack[_4]:SOM::frame&1?_4:SOM::L.ai3_size-1-_4;
	auto &ai = SOM::L.ai3[obj];	
	auto *mdl = (SOM::MDL*)ai[SOM::AI::mdl3];
	auto *mdo = (SOM::MDO*)ai[SOM::AI::mdo3]; //assert(mdl||mdo);
	auto *mhm = mdo?mdo->mdo_data()->ext.mhm:mdl?mdl->mdl_data->ext.mhm:0;

	if(_5==0) //lateral?	
	{
		bool open,door;

		//2017: doors are the only object type that can
		//move the PC. for some reason the test below is
		//making one side of the doors permeable
		WORD pr2 = SOM::L.obj_prm_file[ai[SOM::AI::object]].s[18];
		SHORT *type = &SOM::L.obj_pr2_file[pr2].s[40];
		if(door=mdl&&type[1]>=0xB&&type[1]<=0xE)
		{
			open = false;

			switch(ai.c[0x7c])
			{
			case 4: case 5: 
				
				open = true;

				DWORD bf = mdl->f;

				//improve clipping for doors?
				//the animations are advanced in the drawing phase
				//instead of clipping, so the door's CPs are always
				//1 frame behind
				//((void(__cdecl*)(void*))0x440f30)(mdl); //advance animation?
				extern void som_MDL_440f30(SOM::MDL&);
				extern BYTE som_MDL_4416c0(SOM::MDL&);
				som_MDL_440f30(*mdl);
				
				if(bf!=mdl->f) som_MDL_4416c0(*mdl); //store xform/CPs
			}
					
			if(open) SOM::shoved = SOM::frame; //testing
		}				

		if(obj==63)
		{
			obj = obj; //breakpoint
		}
		if(!som_clipc_x40dff0
		(_1,_2-som_clipc_haircut,_3,obj,_5,_6,_7))
		return 0;
		if(mhm) //2022: refine?
		{
			float yy = _1[1];

			SOM::Clipper pclip(_1,SOM::L.fence,_3,/*14*/12); //5
			if(mhm->types124[2]
			&&pclip.clip(mhm,mdo,mdl,_6,_7)
			&&pclip.elevator!=1)
			{
				float e = som_clipc_opposite(_3/*+oo1*2*/,pclip.elevator);	
				_1[1]+=e;
				pclip.height = max(0.001f,_2-max(e,som_clipc_haircut));				
			}
			else pclip.height = _2-som_clipc_haircut;

			pclip.mask = 5;

			bool out = pclip.clip(mhm,mdo,mdl,_6,_7);

			_1[1] = yy; if(out)
			{
				//YUCK: just return a position like MHM
				//since slopes can't shift the position
				memcpy(_6,pclip.pclipos,3*sizeof(float));
				memset(_7,0x00,3*sizeof(float));

				if(pclip.cling)
				SOM::motions.cling = true;
			}
			else return 0;
		}
		else
		{
			//if(door||type[0]!=0) //box? double-doors are circular?
			if(type[0]!=0) //2020: woops?
			{
				//REMOVE ME?
				//extension? fix around corners of boxes
				float *pos = &ai[SOM::AI::xyz3]; if(door) //hinge?
				{
					pos = som_clipc_40D750_4; 
				
					/*becoming a nuissance (trap door?)
					//REMINDER: this is triggered when touching the hinge (not sure
					//why) which is usually difficult
					assert(som_clipc_40D750_4[1]==1);*/
				}			
				float a[2] = {_1[0]-pos[0],_1[2]-pos[2]};
				float b[2] = {_6[0]+(_3+oo2)*_7[0]-pos[0],_6[2]+(_3+oo2)*_7[2]-pos[2]};
				if(Somvector::map(a).length<2>()>=Somvector::map(b).length<2>())
				{
					return 0; //all or nothing???
				}
			}
		//	if(EX::debug) som_clipc_40D750_4[1] = 0; //hinge? (som_clipc_40D750)

			SOM::motions.cling = true;

			//this fix is needed to stop clinging
			//(it just makes objects behave like MHM walls)
			_6[0]+=oo2*_7[0]; _6[2]+=oo2*_7[2];
		}

		return 1;
	}
	else //1
	{		
		/*REFERENCE (prolog?)
		if(_4==0) //looping
		{	
			if(EX::debug) som_clipc_40D750_4[1] = 0; //hinge? (som_clipc_40D750)

			som_clipc_slipping = true; //assuming

			som_clipc_cling(_1); //_1[0] = SOM::xyz[0]; _1[2] = SOM::xyz[2];
		}*/		

		if(!som_clipc_slipping) _2-=som_clipc_426D60_1; //naive bugfix

		float h2 = mhm?ai[SOM::AI::height3]+som_clipc_426D60_1+oo2:_2;

		BYTE out = som_clipc_x40dff0(_1,h2,_3,obj,_5,_6,_7); //_5
		if(out&&mhm) //2022: refine?
		{
			assert(out<=1); //layers?

			//HACK: just copying som_clipc_4159A0
			float up = !som_clipc_slipping
			?0.000001f:som_clipc_426D60_1;
			SOM::Clipper pclip(_1,_2,_3,14,up);
			if(pclip.clip(mhm,mdo,mdl,_6,_7))
			{
				_6[1] = pclip.pclipos[1]; //YUCK

				float slope = M_PI_2-asinf(pclip.slopenormal[1]);	
				if(slope<0) slope = 0; //-0? 
				if(pclip.slopefloor<pclip.floor)
				_6[1] = pclip.floor+oo1; 
		
				//enable jumping on slope polygons
				if(SOM::motions.aloft) 	
				if(slope&&som_clipc_426D60_y>pclip.slopefloor)
				out = som_clipc_426D60_y<=pclip.floor; //0   

				SOM::slope = slope;

				float e = pclip.elevator; if(e!=1)
				{
					float &e2 = som_clipc_objectelevator;
					e2 = min(e2,e);
				}
			}
			else out = false;
		}
		if(out)
		{
			som_clipc_slipping = false;
			SOM::motions.floor_object = ai[SOM::AI::object];
		}

		/*REFERENCE (epilog?)
		//YUCK (2020) 
		//som_MPX_411a20 is shrinkwrapping sz	
		//if(_4==511) //512 count
		if(_4==SOM::L.ai3_size-1) //512 count?
		{				
			_1[0] = som_clipc_426D60_x;	//som_clipc_cling 
			_1[2] = som_clipc_426D60_z; //som_clipc_cling 
		}*/

		return out; 
	}

	return 0; //compiler //UNREACHABLE
}

extern BYTE __cdecl som_MHM_4159A0(FLOAT*,FLOAT,FLOAT,DWORD,FLOAT*);
//static BYTE __cdecl som_clipc_4159A0 //414dd0 (tiles)
static BYTE __cdecl som_clipc_4159A0_w_40dff0 //2022
(FLOAT _1[6+6], FLOAT _2, FLOAT _3, DWORD _4, FLOAT *_5)
{					
	DWORD x4159A0 = //REMOVE ME?
	/*SOM::image()=='db2'?*/0x4159A0/*:0x414dd0*/; //rt2	

	assert(_1==SOM::L.pcstate);

	if(SOM::emu) return som_MHM_4159A0(_1,_2,_3,_4,_5);

	EX::INI::Player pc;

	//NOTE: out was a bool, technically Clipper is returning
	//a layer mask now
	int out = 0;

	const float y = _1[1], x = _1[0], z = _1[2];
	
	float _6[3];
	
	//don't know if extra should be additive or multiplicative
	//in order to generalize to any radius. smaller values are
	//self-defeating. Reminder: maybe some whiplash is not bad
	const float extra = 0.1f;
	const float radius = pc->player_character_shape;	
	bool landing_2017 = SOM::L.shape>radius; 

	assert(_4==5||_4==14); //32?

	if(_4==5) //wall test
	{ 	
		if(landing_2017) //EXPERIMENTAL
		{		
			//som_mocap_surmount keeps aloft up
			if(!SOM::motions.aloft)
			{
				SOM::L.shape-=extra*2;
			}
		}

		const float _22=_2; if(SOM::L.pcstep)
		{				
			//KF2's stairs are just too choppy
			//this is intended to step over the
			//basic wall of the climbing surface
			if(1)
			{
				//2 works wonders for attacking stairs where the angle is about 30
				//degrees. higher angles climb fast enough to avoid the steps, and
				//lower angles don't bunch up (1/4m steps)
				//3 helps with faster running... it should maybe be velocity based
				//NOTE: JUST player_character_fence+o1 ITSELF ISN'T AN IMPROVEMENT
				float f = min(SOM::L.pcstepladder*3,pc->player_character_fence+o1);
				_1[1]+=f;
				f = min(f,_2-pc->player_character_height2);					
				_2-=max(f,0);
			}

			//2022: rolling 40dff0 into 4159A0
			obj_wall_2022:

			//WARNING: THIS IS BECAUSE OBJECTS
			//ARE NORMALLY IGNORED IF CLIMBING

			//fix: include objects
			//REMINDER: ORDER-IS-IMPORTANT for som_clipc_40dff0
			for(int sz=SOM::L.ai3_size,i=0;i<sz;i++)
			if(som_clipc_40dff0(_1,_2,_3,i,0,_5,_6))
			{
				out = 1; 
				//weird? the calling routine does this for 40DFF0
				_1[0] = _5[0]+_3*_6[0]; _1[2] = _5[2]+_3*_6[2];
			}
			_5[0] = _1[0]; _5[2] = _1[2]; _1[0] = x; _1[2] = z;
		}
		else //2022: rolling 40dff0 into 4159A0
		{
			goto obj_wall_2022; //now not for if climbing
		}
		float *_5_1 = out?_5:_1;
		
		/*EXPERIMENTAL
		//this was a test to try to fuse forward/backward interation
		//over MHM polygons to remove hiccups. it didn't really help
		//and every now and then shows what looks like discontinuity
		//(I think this is an interaction with the filtering effect)
		float sum[3] = {};
		float swap[3]; memcpy(swap,_5_1,sizeof(swap));
		for(int i=2;i-->0;)
		{*/
			//2017: cliffside abutting slope?
			//
			//NOTE: I worked very hard to make the save/recover function
			//work in most cases. it's imperfect however, and sometimes 
			//it helps to use your imagination. the fall can be cleaned
			//up, but the save function is more important than bumpiness
			//
			if(SOM::clipper.clip(_5_1,SOM::L.fence,_3,/*14*/12,0)
			 &&SOM::clipper.elevator!=1)
			{	
				//WARNING (2022)
				//THIS PUSHES BACK WALL SLOPES ON THE SAME LEVEL AS THE
				//FLOOR SLOPE (any fix requires input)
				
				//raise the clip shape above the walls according to the
				//floor polygon with the largest slope
				float e = som_clipc_opposite(_3/*+oo2*/,SOM::clipper.elevator);	
				float _2_e = max(oo1,_2-max(e,som_clipc_haircut));
				_5_1[1]+=e;
	//			EX::dbgmsg("e %f %f %d",e,_5_1[1],SOM::frame);
				out|=SOM::clipper.clip(_5_1,_2_e,_3,_4,0,_5);						
			}
			else out|=SOM::clipper.clip(_5_1,_2-som_clipc_haircut,_3,_4,0,_5);
			//REMINDER: _3!=radius is climbing back onto a ledge while walking 
			//off of it at full speed
			if(SOM::clipper.cling/*||_3!=radius*/)
			SOM::motions.cling = true;

			/*
			if(out) _5_1 = _5;
			sum[0]+=_5_1[0]; sum[2]+=_5_1[2];

			memcpy(_5_1,swap,3*sizeof(float));

			SOM::frame++;
		}
		SOM::frame-=2; _5_1[0] = sum[0]*0.5f; _5_1[2] = sum[2]*0.5f;*/

		//elevator requires this, but SOM actually uses the Y of this test
		//so set it back, just to be safe
		_5[1] = _1[1] = y; _2 = _22;
	}
	if(out){ _1[0] = _5[0]; _1[2] = _5[2]; }

	som_clipc_cling(_1); //_1[0] = SOM::xyz[0]; _1[2] = SOM::xyz[2];*/
				
	if(_4==5) //ceiling test
	{	
		//fyi: must come after wall test 	   __
		//to avoid contact with overhangs, eg. ->|__
		
	  //REMINDER: this is for a ceiling without a wall present
	  //Walls keep from entering a space. ceilings mainly just
	  //keep the PC from standing back up, once in said spaces

		if(!out){ _5[0] = x; _5[2] = z; } 

		assert(_2==SOM::L.height);

		float ouch = 0; //experimental
		float shape = min(_3,SOM::L.hitbox*SOM::motions.leanness);

		float probe = pc->player_character_height+o1;		
		SOM::clipper.clip(_1,probe,shape,14,0); 						
		if(SOM::clipper.ceiling<FLT_MAX
		//2017: disregard ceilings below knee level where falling
		&&SOM::clipper.ceiling>y+som_clipc_426D60_1+pc->player_character_fence) 
		{	
			SOM::arch = M_PI_2+asinf(SOM::clipper.ceilingarch);
			if(SOM::arch<=0) SOM::arch = 0;	//can be "-0"

			ouch = SOM::clipper.ceiling-y-oo1; if(ouch<_2) 
			{	
				float yy = SOM::clipper.ceiling-_2-oo1;
				/*EXPERIMENTAL
				//NOT WORKING, BUT IS TRIGGERED BY OTHER THINGS
				//2017: trying to eliminate harsh bump when exiting
				//a crawl space while standing up. see below...
				if(y-yy>som_clipc_426D60_1)
				{
					yy = y-som_clipc_426D60_1; 
				//	if(EX::debug) MessageBeep(-1); //ceiling test
				}*/
				//
				out = 1; _1[1] = _5[1] = yy; //SOM::clipper.ceiling-_2-oo1;
			}
		}		
		else SOM::arch = 0;

		SOM::clipper.mask = 14; //2022: object MHM?

		//object ceiling test		
		float top = _1[1]+_2;
		float top_haircut = top-som_clipc_haircut;
		for(int i=0,sz=SOM::L.ai3_size;i<sz;i++) //512
		{
			auto &ai = SOM::L.ai3[i];
			if(som_clipc_x40dff0(_1,probe,shape,i,0,_6,_6))
			{
				//2022: object MHM?
				float bottom = ai[SOM::AI::y3];
				if(!SOM::clipper.clip(ai,_6,_6)) //2022: MHM?
				continue;
				else if(SOM::clipper.obj_had_mhm)
				{
					if(-1!=SOM::clipper.ceilingarch)
					{
						//UNFINISHED //SOM::arch?
					}

					bottom = SOM::clipper.ceiling;

					if(bottom==FLT_MAX) continue;
				}
				
				float hit = bottom-y-oo1; if(!ouch||ouch>hit) 
				{
					ouch = hit; //NEW: probing

					//OLD: delta based hit test										
					if(top>bottom&&top_haircut<bottom)
					{
						float yy = bottom-_2-oo1;
						/*EXPERIMENTAL
						//NOT WORKING, BUT IS TRIGGERED BY OTHER THINGS
						//2017: trying to eliminate harsh bump when exiting
						//a crawl space while standing up. see above...
						if(y-yy>som_clipc_426D60_1)
						{	
							yy = y-som_clipc_426D60_1; 
						//	if(EX::debug) MessageBeep(-1); //ceiling test
						}*/
						//
						_1[1] = _5[1] = yy; //bottom-_2-oo1;
						//top = _1[1]+_2; top_haircut = top-som_clipc_haircut; 
						out = 1; 

						//todo: what if an NPC or enemy?
						SOM::motions.ceiling_object = ai[SOM::AI::object]; 
					}
				}
			}
		}

		//halt climbing?
		if(out&&_1[1]<y) if(som_clipc_climbing) 
		{
			SOM::L.pcstepladder = som_clipc_climbing;
			//SOM::L.pcstep = out;
			SOM::L.pcstep = som_clipc_climb(_5); 
		}
		
		if(SOM::L.pcstep) //climb into crawlspace
		if(top>=som_clipc_climb.y-som_clipc_426D60_1)
		{
			float hit = som_clipc_climb.y-som_clipc_426D60_1-_1[1];
			if(!ouch||ouch>hit) ouch = hit;
		}

		if(ouch>0) //EXPERIMENTAL
		if(!SOM::motions.ceiling||SOM::motions.ceiling>ouch)
		{
			SOM::motions.ceiling = ouch;
		}
		
		//KF2's pier
		//basically don't let the ceiling push down...
		//except it helps to not see through a ceiling
		//if jumping up at it
		if(out&&_5[1]<som_clipc_426D60_y)
		if(!SOM::motions.aloft||SOM::motions.freefall) //HACK
		{
			//going through the floor is a problem :(
			_1[1] = _5[1] = y;
		}
	}
	else if(_4==14) //climbing test 
	{
		//formerly som_clipc_40dff0 //2022
		{
			if(EX::debug) som_clipc_40D750_4[1] = 0; //hinge? (som_clipc_40D750)

			som_clipc_slipping = true; //assuming

	//		som_clipc_cling(_1); //_1[0] = SOM::xyz[0]; _1[2] = SOM::xyz[2];

			som_clipc_objectelevator = 1; //HACK

			assert(!out);
			int sz = SOM::L.ai3_size; //2020
			for(int i=0;i<sz;i++) if(som_clipc_40dff0(_1,_2,_3,i,1,_5,_6))
			{
				//_1[1] = _5[1]+oo1; //426d60 doesn't add 0.001
				_1[1] = _5[1];

				out = 1;
			}
			if(out) _5[1] = _1[1]; 

		//	_1[0] = x; //som_clipc_cling 
		//	_1[2] = z; //som_clipc_cling 
		}

		float up = !som_clipc_slipping
		?0.000001f:som_clipc_426D60_1;
	//	SOM::climber.current = -1;
		out = SOM::clipper.clip(_1,_2,_3,_4,up,_5);
		float slope = M_PI_2-asinf(SOM::clipper.slopenormal[1]);	
		if(slope<0) slope = 0; //-0? 
		if(out&&SOM::clipper.slopefloor<SOM::clipper.floor)
		_5[1] = SOM::clipper.floor+oo1; 
		
		//enable jumping on slope polygons
		if(SOM::motions.aloft) 	
		if(slope&&y>SOM::clipper.slopefloor)
		out = y<=SOM::clipper.floor; //0   

		if(out) SOM::slope = slope; 
		if(out) SOM::motions.floor_object = EX::NaN;

		//2021: standing astride a crack? (2+ platforms)
		if(!som_clipc_slipping3)		
		if(!out?som_clipc_slipping:!slope&&_1[1]+up>_5[1])
		if(_3<pc->player_character_shape2)
		{
			//maybe doesn't have to be so wide?
			//NOTE: can't lean out when falling
			//float r = radius;
			float r = pc->player_character_shape2;
			float h = SOM::L.duck;

			assert(!SOM::L.pcstep); //2022

			//som_clipc_cling?
			//assert(_1[0]==x&&_1[2]==z);
			float pos[3] = {_1[0],_1[1],_1[2]},p3[3],n3[3];			
			int hit = 0;
			for(int i=0,sz=SOM::L.ai3_size;i<sz;i++)	
			if(SOM::clipper_40dff0(pos,h,r,i,5,p3))
			{
				hit = 1; 
				pos[0] = p3[0]; pos[2] = p3[2];
			}		
			if(som_MHM_4159A0(pos,h,r,5,p3)) //NPC like? som.MHM.cpp
			{
				hit = 1;
				pos[0] = p3[0]; pos[2] = p3[2];
			}
			if(hit&1)
			for(int*i=som_clipc_objectstack,*e=i+SOM::L.ai3_size;i<e;i++)	
			if(SOM::clipper_40dff0(pos,h,r,*i,14,p3))
			{
				if(p3[1]>pos[1]) //2022: slope? //UNFINISHED?
				{
					hit|=2; pos[1] = p3[1];
				}
			}
			//want to avoid ceiling polygons... actually
			//it's because KF2's rope bridge has 2-sided
			//MHM polygons for rope but this is probably 
			//just more robust
			//if(hit&1) if(som_MHM_4159A0(pos,h,r,14,p3))
			if(hit&1) if(SOM::clipper.clip(pos,h,r,14,som_clipc_426D60_1,p3))
			{
				if(p3[1]>pos[1]) //2022: slope? //UNFINISHED?
				{
					hit|=2; pos[1] = p3[1];
				}
			}
			if(hit&2&&pos[1]>=y) //didn't fall?
			{
				out = 1; memcpy(_5,pos,sizeof(pos));
			}
		}

		//sliding/falling off stool?
		if(!out&&som_clipc_slipping)
		{
			if(!landing_2017)
			if(!SOM::motions.aloft)
			if(SOM::stool)	
			SOM::L.shape = *SOM::stool;

			som_clipc_slipping2 = true; //2020
		}		
		else if(slope) //1.2.1.8 //SKETCHY //SKETCHY
		{
			assert(out); //2021: do NPCs need this?

			//COULD USE MORE THOUGHT
			//SURPRISINGLY EFFECTIVE
			//this is to run over the top of triangle
			//and diamond shaped slopes
			//it was added as a final step to get the
			//som_MHM_416A50_slopetop stuff to work, for
			//both regular movements, and scaling slopes
			if(!SOM::motions.freefall&&!SOM::motions.aloft)
			{
				//is it a GLITCH? Or an edge-case? It seems
				//to go one way. which is lucky because it's
				//hard to differentiate from scaling maneuvers
				float diff = _5[1]-_1[7];
				//if(fabs(diff)>0.05f) //just a wild guess
				if(diff<-0.05f) //one way? works!
				//if(fabs(_5[1]-SOM::clipper.slopefloor)<=0.05f) //climbing?
				{
					_5[1] = _1[7]-0.02f;//+_copysign(0.02f,diff);
				}
			}
		}
		else assert(!out||_5[1]>=y); //impossible?

		out|=!som_clipc_slipping; //2022

		//2017: EXPERIMENTAL
		//this is an attempt to ameliorate one of the most
		//vexing problems, namesly jumping onto/into windows
		//without getting stuck standing on the inside of the 
		//window with the head/view outside facing a wall above
		//the window
		//NOTE: this includes som_mocap_surmount when jumping
		//onto a ledge and scaling it. jumps are just wildcards
		/*bool*/ landing_2017 = out&&SOM::motions.aloft;
	
		//RECURSIVE
		static int attempt = 1;
		//NEW: climbing into portholes or
		//stacked obects or if _3 won't fit... or now "landing_2017"
		if(_4==14&&attempt==1) if((out?_5:_1)[1]>_1[7]||landing_2017)
		{	 
			//starting point
			som_clipc_climb.y = FLT_MAX;
			som_clipc_climb.xz[0] = _1[0]; 
			som_clipc_climb.xz[1] = _1[2]; 
		
			float howlowcanyougo = pc->player_character_height2+o1;

			if(out) _1[1] = _5[1];		

			//TODO: INCLUDE 40DFF0 IN landing_2017 LOGIC (OBJECTS)

			if(!landing_2017) //climbing procedure?
			{
				float e = som_clipc_objectelevator;
				if(out) e = min(e,SOM::clipper.elevator);
				//if(out&&e!=1) //2017
				if(e!=1) //2022: som_clipc_objectelevator?
				{			
					//HACK? slopes project down below the connecting wall
					e = som_clipc_opposite(_3,e);

					_1[1]+=e; howlowcanyougo-=e;
				}

				//CORRECT? having problem climbing KF2's lighthouse
				//
				// I WORRY THE WALL GIVES WAY TO FLOORS/CEILINGS :(
				//
				// RATIONALE: a wall in the way can't pose a problem
				// for normal steps, since it seems absurd. IOW: for
				// the wall test to apply, the Action button must be
				// involved. IN HINDSIGHT I think this problem would
				// not have come to light with SOM's built-in models
				//
				if(_1[1]-_1[7]<pc->player_character_fence+o1)
				{
					//worried how this mixes with landing_2017 
					//if(!SOM::motions.aloft) 
					{
						goto easypass; //bypass wall tests?
					}
				}
				//else assert(0); //high perch

				//REMINDER (2022)
				//I've modified som.MHM.cpp to return false
				//for 1|4 (5) when !goingup and a climbable
				//slope is hit and nothing else
				for(int i=SOM::L.ai3_size;i-->0;) //2020
				if(SOM::clipper_40dff0(_1,howlowcanyougo,_3,i,5,_6))
				{
					attempt = 2; break; //optimizing?
				}
				if(attempt!=2&&SOM::clipper.clip(_1,howlowcanyougo,_3,5)) 
				{
					attempt = 2;			
				}
				easypass:; //bypassed wall tests?

				SOM::motions.cling = false; //WEAK LINK!!
			}
			else //landing_2017?	
			{
				//this had been applied over and over as needed. maybe it
				//still is. landing_2017 is tighter now. it may be 1 only
				_3 = max(radius,SOM::L.shape)+extra;
				_3 = min(radius*1.5f,_3);
			}

			if(attempt!=2) //hack: clearance
			if(SOM::clipper.clip(_1,pc->player_character_height+o1,_3,14,0)
			 &&SOM::clipper.ceiling<FLT_MAX)
			{	
				if(landing_2017) //crawlspace?			
				if(SOM::clipper.ceiling>_1[1]+howlowcanyougo)			
				SOM::motions.ceiling = SOM::clipper.ceiling-_1[1]-oo1;
				else goto pigeonhole;
				if(landing_2017
				&&(SOM::clipper.ceiling<y+SOM::L.height)) pigeonhole:
				{
					SOM::motions.ceiling = FLT_MAX; //can't enter crawlspace

					//there's a ceiling in the way!
					//at this point, somehow all available walls must push
					//away until only a floor remains
					//note that normally walls are for keeping out of places
					//but here there must be a ceiling
					//if(EX::debug&&radius>=*SOM::L.shape)
					//MessageBeep(-1);				
					SOM::L.shape = _3;

					//maybe want this instead?
					//_5[1] = som_clipc_426D60_y-som_clipc_426D60_1; 
					//_5[1] = y-som_clipc_426D60_1; 		
					_5[1] = y+som_clipc_426D60_1; 		
					////_5[1] = SOM::clipper.pclipos[1];
					if(!out){ out = 1; _5[0] = x; _5[2] = z; }
				}
				else som_clipc_climb.y = SOM::clipper.ceiling-_1[1]; //climb
			}
		
			if(attempt==2) //prevent climbing (any further)
			{
				_3 = SOM::L.hitbox;
				_1[0] = _1[6]; _1[2] = _1[8];
				//this is the same as _1[1] = y I think
				_1[1] = som_clipc_426D60_y-som_clipc_426D60_1; 

				//2022: I think there's potential bugs
				//here. at minimum I think 40dff0 must
				//have been intended to be mode 1 here
				//since it's keeping _1[1]. anyway I'm
				//not sure this is correct now or ever
				//was correct before. 40dff0(1) is now
				//called inside and som_clipc_slipping
				//will be reset to true before calling

				//2022: NOT true? false???
			//	som_clipc_slipping = false; //weak link!!						
				//REMINDER: ORDER-IS-IMPORTANT for som_clipc_40dff0
				/*2022: som_clipc_4159A0 implements this 
				int sz = SOM::L.ai3_size; //2020
				//2022: NOT 1? 0???
				for(int i=0;i<sz;i++) if(som_clipc_40dff0(_1,_2,_3,i,0,_5,_6))
				_1[1] = _5[1]+oo1;
				out = (bool)som_clipc_4159A0(_1,_2,_3,_4,_5); //RECURSIVE
				//NOTE: 426d60 doesn't add 0.001 (_5[1]+oo1)*/
				out = (bool)som_clipc_4159A0_w_40dff0(_1,_2,_3,_4,_5); //RECURSIVE

				attempt = 1;
			}

			//2020: tired of this killing my test session (although I wish I
			//understood the problem)
			//assert(!out||_5[1]>=y);
			static bool suppress = false;
			if(!out||_5[1]>=y) if(!suppress)
			{
				suppress = true; assert(!out||_5[1]>=y);
			}
		}
	}

	_1[0] = x; _1[1] = y; _1[2] = z;
	
	return (BYTE)out;
}

extern bool SOM::enteringcrawlspace(float futurepos[3])
{	
	static unsigned wall = SOM::motions.tick;
	//if('db2'!=SOM::image()) return false;	
	EX::INI::Player pc; 		
	float shape = pc->player_character_shape2;
	float fence = pc->player_character_fence+o1;
	float pos[3], norm[3];
	int sz = SOM::L.ai3_size; //2020
	for(int i=0;i<sz;i++)
	if(SOM::clipper_40dff0(futurepos,fence,shape,som_clipc_objectstack[i],14,pos,norm))
	futurepos[1] = pos[1]+oo1;
	if(SOM::clipper.clip(futurepos,fence,shape,14,0.000001f))	
	futurepos[1] = max(SOM::clipper.slopefloor,SOM::clipper.floor)+oo1;
	assert(futurepos[1]!=-FLT_MAX); 
	float height = pc->player_character_height+o1;
	float height2 = pc->player_character_height2+o1;
	if(SOM::clipper.clip(futurepos,height2,shape,5,0)) 
	{
		wall = SOM::motions.tick; return false; //illustrating
	}
	//optimizing? can do both heights for objects
	float floor = futurepos[1], ceiling = FLT_MAX;
	for(int i=0;i<sz;i++)
	if(som_clipc_x40dff0(futurepos,height,shape,i,0,pos,norm))
	{
		auto &ai = SOM::L.ai3[i];
		if(!SOM::clipper.clip(futurepos,height,shape,5,0,ai,pos,norm))
		continue;

		float bottom = ai[SOM::AI::y3];

		if(SOM::clipper.obj_had_mhm)
		{
			if(-1!=SOM::clipper.ceilingarch)
			{
				//UNFINISHED //SOM::arch?
			}

			bottom = SOM::clipper.ceiling;

			if(bottom==FLT_MAX) continue;
		}

		if(bottom<floor+height2)
		{
			wall = SOM::motions.tick; return false; //illustrating
		}
		else if(bottom<ceiling) ceiling = bottom;
	}	
	//was a wall encountered along the path ahead?
	if(SOM::motions.tick-wall<pc->tap_or_hold_ms_timeout) 
	return false;
	if(SOM::clipper.clip(futurepos,height,shape,14,0))
	if(SOM::clipper.ceiling<ceiling) 
	ceiling = SOM::clipper.ceiling;				
	float clearance = max(0,ceiling-floor);
	return clearance<height&&clearance>=height2;
}
extern bool SOM::surmountableobstacle(float futurepos[3]) //UNUSED
{							 
	//2020: WHAT DID THIS DO? 
	//the new "holding" feature makes it impossible to input this
	//scenario since it works the exact same way... I can't recall
	//why this was necessary... perhaps it's obsolete?

	//HACK: reproduce climbing logic
	//if('db2'!=SOM::image()) return false;	
	EX::INI::Player pc; 	
	float shape = pc->player_character_shape;
	float fence = pc->player_character_fence+o1;	
	float fence2 = pc->player_character_fence2+o1;	
	//this disables surmountedcrawlspace
	//if(fence2<=fence) return false;
	float pos[3], norm[3]; float crawlspace = futurepos[1];		 
	float y = futurepos[1];
	int sz = SOM::L.ai3_size; //2020
	for(int i=0;i<sz;i++)
	{
		//WORK IN PROGRESS
		int obj = som_clipc_objectstack[i];
		auto &ai = SOM::L.ai3[obj];
		auto *mdl = (SOM::MDL*)ai[SOM::AI::mdl3];
		auto *mdo = (SOM::MDO*)ai[SOM::AI::mdo3]; //assert(mdl||mdo);
		auto *mhm = mdo?mdo->mdo_data()->ext.mhm:mdl?mdl->mdl_data->ext.mhm:0;
		float h2 = mhm?ai[SOM::AI::height3]+oo2:fence2;
		if(som_clipc_x40dff0(futurepos,h2,shape,obj,1,pos,norm))
		{
			if(!mhm)
			{
				futurepos[1] = pos[1]+oo1; continue;
			}			
			SOM::Clipper pclip(futurepos,fence2,shape,14); //2022: MHM?
			if(pclip.clip(mhm,mdo,mdl,pos,norm))
			{
				//DUPLICATE (same as below)
				float e = som_clipc_opposite(shape*1.5f,pclip.elevator);
				futurepos[1] = max(pclip.slopefloor+e,pclip.floor)+oo1;
			}
		}
	}
	if(SOM::clipper.clip(futurepos,fence2,shape,14,0/*.000001f*/))	
	{
		//2017: slope clip points are projected to the shape's center
		//this must clear any walls below the slope
		float e = som_clipc_opposite(shape*1.5f,SOM::clipper.elevator);

		futurepos[1] = max(SOM::clipper.slopefloor+e,SOM::clipper.floor)+oo1;
	}
		//this disables surmountedcrawlspace
		//2017: was returning true as long as there were not ceilings
		//NOTE: it was only called if pushing/clinging, but cling did
		//not quite reach 0 for the bool test
		if(futurepos[1]-y<fence) 
		{
#ifdef NDEBUG
//NOTE: som_mocap_surmount has this pretty well constrained but it needs 
//a full investigation even though I'm going to publish v1.2.2.14 anyway
//NOTE: surmounting_staging solves this problem okay in a roundabout way
//#error fix me
int todolist[SOMEX_VNUMBER<=0x1020504UL]; //I think this can be removed?
#endif
			//REMINDER: it's too messy to try to rule out crawlspaces
			//return false;
		}

	//annoying me (climbing barrel in KF2 caves???)
	//assert(futurepos[1]!=-FLT_MAX); 
	float height = pc->player_character_height2+o1;
	for(int i=0;i<sz;i++) 
	if(SOM::clipper_40dff0(futurepos,height,shape,i,5|32,pos,norm)) //5|32?
	{
		return false; //wall
	}
	if(SOM::clipper.clip(futurepos,height,shape,5|32,0)) 
	{
		return false; //wall	
	}
	SOM::clipper.clip(futurepos,height,shape,14,0);
	if(SOM::clipper.ceiling>futurepos[1]+height)
	return true;
	//ambiguous case where climbing was ruled out by ceiling
	if(crawlspace+o1<futurepos[1]||fence2<=height)
	return false;
	futurepos[1] = crawlspace;
	for(int i=0;i<sz;i++) 
	if(SOM::clipper_40dff0(futurepos,height,shape,i,14|1,pos,norm))
	{
		return false;
	}
	return !SOM::clipper.clip(futurepos,height,shape,14|1,0.000001f);
} 
extern bool SOM::surmountedcrawlspace(float futurepos[3]) //UNUSED
{
	//HACK: reproduce climbing logic
	//if('db2'!=SOM::image()) return false;	
	EX::INI::Player pc; 		
	float shape = pc->player_character_shape2;
	float height = pc->player_character_height2+o1;
	float pos[3], norm[3];
	int sz = SOM::L.ai3_size; //2020
	for(int i=0;i<sz;i++)
	if(SOM::clipper_40dff0(futurepos,height,shape,i,14,pos,norm))
	return false; 
	return !SOM::clipper.clip(futurepos,height,shape,14,0.000001f);
}

static FLOAT som_clipc_g = 0;
static FLOAT som_clipc_stool = 0.25f;

extern void som_clipc_reprogram() //som_clipc_reprogram_image
{
	EX::INI::Option op; EX::INI::Player pc;

	som_clipc_objectstack = new int[512]();
	
	//2022: dispense with object loop (rolling into 4159A0) 
	{
		//00426DCA 7E 3D                jle         00426E09
		//0042708A 7E 5D                jle         004270E9
		*(BYTE*)(0x426DCA) = 0xEB; //jmp
		*(BYTE*)(0x42708A) = 0xEB; //jmp
	}

	//if(SOM::game) //slip/ceiling fix
	{
		//if(pc->player_character_slip)
		{				
			SOM::stool = &som_clipc_stool;
			*SOM::stool = pc->player_character_shape; 
		}
	
	//	if(image=='rt2')  //40D420/414dd0
		{									
			/*NO LONGER SUPPORTING
			if(SOM::stool) //REMOVE ME?
			som_state_route(+0x24f13,+1,(DWORD)SOM::stool);
			*(DWORD*)0x425f91 = (DWORD)som_clipc_40dff0-0x425f95;
			*(DWORD*)0x426052 = (DWORD)som_clipc_4159A0-0x426056;*/
		}
	//	else if(image__db2) 
		{
			//00424BB8 E8 A3 21 00 00       call        00426D60
			//this is the call to the master of these subroutines
			*(DWORD*)0x424BB9 = (DWORD)som_clipc_426D60-0x424BBD;
			//00424BC1 75 C0                jne         00424B83			
			//memset((void*)0x424BC1,0x90,2); //NOP
			{			
			//these subroutines only affect elevation
			//00426D63 A1 D4 5B B9 66       mov         eax,dword ptr ds:[66B95BD4h] 
			*(DWORD*)0x426D64 = (DWORD)SOM::stool;
			//00426DE0 E8 0B 72 FE FF   call        0040DFF0
	//		*(DWORD*)0x426DE1 = (DWORD)som_clipc_40dff0-0x426DE5; //objects //2022
			//00426E17 E8 84 EB FE FF   call        004159A0 
			*(DWORD*)0x426E18 = (DWORD)som_clipc_4159A0_w_40dff0-0x426E1C; //tiles			
			//maybe this one is testing for the ceiling???
			//(called only if first call succeeded in climbing)
			*(DWORD*)0x426EA2 = (DWORD)som_clipc_4159A0_w_40dff0-0x426EA6; //tiles
			//REMINDER: likely very similar to som_logic_40C8E0
			//40C2E0 SEEMS TO BE FOR CYLINDER SHAPES IN GENERAL
			//*(DWORD*)0x426f95 = (DWORD)som_clipc_40C2E0-0x426f99; //NPCs?		
			//00427039 E8 A2 52 FE FF       call        0040C2E0 //enemies?
					
			/*/// just using SOM::L.shape directly ///////////

				if(SOM::stool)
				{
				//+25E79+2 
				som_state_route(+0x25E79,+2,(DWORD)&SOM::stride); 	
				//+25f7d+2
				som_state_route(+0x25f7d,+2,(DWORD)&SOM::stride); 	
				//+25fA0+2 //+25fB4+2
				som_state_route(+0x25fA0,+2,(DWORD)&SOM::stride); 	
				som_state_route(+0x25fB4,+2,(DWORD)&SOM::stride); 	
				//+2601F+2
				som_state_route(+0x2601f,+2,(DWORD)&SOM::stride); 	
				//+26049+2 //+2605D+2 
				som_state_route(+0x26049,+2,(DWORD)&SOM::stride); 	
				som_state_route(+0x2605D,+2,(DWORD)&SOM::stride); 	
				//+2608C+1
				som_state_route(+0x2608C,+1,(DWORD)&SOM::stride);

				//+260BB+2 //+260CF+2: object clipping radius
				som_state_route(+0x260BB,+2,(DWORD)&SOM::stride); 	
				som_state_route(+0x260CF,+2,(DWORD)&SOM::stride); 	
				//+260E9+1: looks like the MPX clipping radius			
				som_state_route(+0x260E9,+1,(DWORD)&SOM::stride);

				//+267ae+2				
				som_state_route(+0x267ae,+2,(DWORD)&SOM::stride); 	
				}

			*/////////////////////////////////////////////////

			//THESE ARE THE X/Z CALLS
			//004270AB E8 40 6F FE FF       call        0040DFF0 //objects
	//		*(DWORD*)0x4270AC = (DWORD)som_clipc_40dff0-0x4270B0; //2022
			//00427102 E8 99 E8 FE FF       call        004159A0 //tiles			
			*(DWORD*)0x427103 = (DWORD)som_clipc_4159A0_w_40dff0-0x427107; 
			}
			
			//Non-PC
			//if(EX::debug) //1.2.1.8
			{
			///2017: exploring some more (slope+floor when cornering)///
			//0042A190 57                   push        edi  
			//0042A191 E8 0A B8 FE FF       call        004159A0  
			//0042A196 83 C4 14             add         esp,14h
			*(DWORD*)0x42A192 = (DWORD)som_MHM_4159A0-0x42A196; //npc?

			//0042A3A3 57                   push        edi  
			//0042A3A4 E8 F7 B5 FE FF       call        004159A0  
			//0042A3A9 83 C4 14             add         esp,14h 
			*(DWORD*)0x42A3A5 = (DWORD)som_MHM_4159A0-0x42A3A9; //npc?

			//00407AC6 68 8F C2 35 3F       push        3F35C28Fh  
			//00407ACB 57                   push        edi  
			//00407ACC E8 CF DE 00 00       call        004159A0  
			//00407AD1 83 C4 14             add         esp,14h  
			*(DWORD*)0x407ACD = (DWORD)som_MHM_4159A0-0x407AD1; //enemy

			//00407CBE 50                   push        eax  
			//00407CBF 57                   push        edi  
			//00407CC0 E8 DB DC 00 00       call        004159A0  
			*(DWORD*)0x407CC1 = (DWORD)som_MHM_4159A0-0x407CC5; //enemy
			}
		
			if(1) //plumbing the secrets of doors??
			{
				//most of the problem with this was the extra tolerance
				//added by som_clipc_40dff0
				//secret doors should not be 0.75m thick. but probably
				//the data files should be changed

				//these are to add a "tolerance" to door clip offsets
				//mainly so that secret doors can appear as much like
				//walls as possible
				//
				//regular objects
				//0040E2A0 E8 AB F4 FF FF       call        0040D750
			//	*(DWORD*)0x40E2A1 = (DWORD)som_clipc_40D750-0x40E2A5;
				//double doors while moving... single-wide
				//0040E485 E8 C6 F2 FF FF       call        0040D750
				*(DWORD*)0x40E486 = (DWORD)som_clipc_40D750-0x40E48a;
				//double doors while resting... double-wide				
				//0040E645 E8 06 F1 FF FF       call        0040D750
				*(DWORD*)0x40E646 = (DWORD)som_clipc_40D750-0x40E64a;
				//0040E91E E8 2D EE FF FF       call        0040D750
				*(DWORD*)0x40E91F = (DWORD)som_clipc_40D750-0x40E923;
				//secret and swing doors, and?
				//0040EB78 E8 D3 EB FF FF       call        0040D750
				*(DWORD*)0x40EB79 = (DWORD)som_clipc_40D750-0x40EB7D;									

				//D has a weird cylinder test. this knocks that code
				//0040EB92 66 83 78 52 0D       cmp         word ptr [eax+52h],0Dh 
				*(BYTE*)0x40EB96 = 0xFF;
			}
		}
	}
				
	if(op->do_g)
	{	
	//	if(image=='rt2')
		{				
			//TODO: player_character_clip
			//This is the F4 mode scalar (1/3)
			//The reciprocal is probably 0x457418 (3)
			//SOM::L.freefall = (FLOAT*)0x457514;

			//This is where the vertical component is scaled
	//		som_state_route(0x22d33,0,(DWORD)&som_clipc_g); 
		}
	//	if(image__db2)
		{							
			//TODO: player_character_clip
			//This is the F4 mode scalar (1/3)
			//The reciprocal is probably 0x458438 (3)
			//SOM::L.freefall = (FLOAT*)0x458534;

			//00424B79 D8 0D 94 AA 30 10    fmul        dword ptr [458534h]
	//		som_state_route(0x23b7b,0,(DWORD)&som_clipc_g);
			*(DWORD*)0x424B7b = (DWORD)&som_clipc_g;

			//2018/NOTE: these seem to be X/Z
			//00424B54 D8 0D 34 85 45 00    fmul        dword ptr ds:[458534h]  
			//*(DWORD*)0x424B56 = (DWORD)&som_clipc_g;
			//00424B5A C7 44 24 58 00 00 00 00 mov         dword ptr [esp+58h],0  
			//00424B62 BF 03 00 00 00       mov         edi,3  
			//00424B67 D9 5C 24 54          fstp        dword ptr [esp+54h]  
			//00424B6B D8 0D 34 85 45 00    fmul        dword ptr ds:[458534h] 
			//*(DWORD*)0x424B6D = (DWORD)&som_clipc_g;
		}	

		//To get the velocity you want to divide by 18
		//NOTE: WHAT THIS ACTUALLY DOES is divide by 3
		//for the three stage multi-pass clipper setup
		//but this way of controlling the gravity came
		//before the clipper was understood to do this
		SOM::g = &som_clipc_g; som_clipc_g = 0.333333f; 		

		/*breaks fall recovery.... need to do in som_clipc_426D60
		//delay start of climbing so speed can be changed for the first frame		
		if(image__db2) 
		{
			//WARNING: breaks SOM:emu 
			//WARNING: breaks SOM:emu
			//WARNING: breaks SOM:emu
			//00426E65 D8 44 24 40          fadd        dword ptr [esp+40h]  
			memset((void*)0x426E65,0x90,4);
			//00426E69 D9 1D 9C 1D 9C 01    fstp        dword ptr ds:[19C1D9Ch]  
			//00426E6F D8 64 24 40          fsub        dword ptr [esp+40h]  
			//00426E73 D9 1D CC 1D 9C 01    fstp        dword ptr ds:[19C1DCCh]
			memset((void*)0x426E6F,0x90,4);
		}*/
	}
}