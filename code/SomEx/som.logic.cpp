
#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

#include <hash_map> //Windows XP

#include "Ex.ini.h"
#include "Ex.output.h"

#include "SomEx.h" //SOMEX_VNUMBER

#include "som.state.h"
#include "som.game.h" //som_game_dist2
#include "som.files.h"

#define SOMVECTOR_MATH
#include "../Somplayer/Somvector.h"
#include "../lib/swordofmoonlight.h"

namespace DDRAW
{
	unsigned refreshrate;
}

extern int SomEx_pc,SomEx_npc;
extern int som_game_interframe;
extern BYTE __cdecl som_MDL_4416c0(SOM::MDL&);

enum{ som_logic_softclip=1 };
enum{ som_logic_soft_impedance=6 }; //8
//CONVERT ME TO float* (DEBUGGING)
static std::vector<float> som_logic_soft;

static float *som_logic_408cc0_yh(som_EVT *event) //2021
{
	auto e = (swordofmoonlight_evt_header_t::_item*)event;

	float y,h,*p;
	int i = e->subject; switch(e->type)
	{
	case 0: //NPC
	{
		auto &ai = SOM::L.ai2[i]; //3/4
		if(i>=128||1u<(unsigned)(4-ai[SOM::AI::stage2]))
		return 0;

		y = ai[SOM::AI::y2];
		h = ai[SOM::AI::height2];
		p = &ai[SOM::AI::xyzuvw2]; break;
	}
	case 1: //enemy
	{
		auto &ai = SOM::L.ai[i]; 
		if(i>=(int)SOM::L.ai_size
		||1u<(unsigned)(4-ai[SOM::AI::stage])) //3/4
		return 0;
		
		y = ai[SOM::AI::y];
		h = ai[SOM::AI::height];
		p = &ai[SOM::AI::xyzuvw]; break;
	}
	case 2: //object
	{
		auto &ai = SOM::L.ai3[i];
		if(i>=512 //2024: event object without model?
		||ai[SOM::AI::obj_valid]&&!ai[SOM::AI::obj_shown])
		return 0;

		y = ai[SOM::AI::y3];
		h = ai[SOM::AI::height3];
		p = &ai[SOM::AI::xyzuvw3]; break;

		//NOTE: SOM_MAP defaulted events
		//to this protocol causing every
		//unused event to be passed onto
	}	//408cc0
	default: assert(0); case 0xff: //return 0;
		//
		e->protocol = 0; return 0;
	}
	
	if(e->ext_zr_flags&1) //2022
	{
		float *z;
		if(e->protocol==4)
		z = &e->ext.square_z1; //breakpoint
		else
		z = &e->ext.radius_z1;
		
		if(z[0]>z[1]) std::swap(z[0],z[1]);

		y+=z[0]; h = z[1]-z[0];
	}
	else if(h<=1) //-/+1 below?
	{
		 h = 1; //2023: Moratheia 2.1
	}

	//EXTENSION
	//1.5 is the default PC's height
	if(y+h>=SOM::xyz[1]-1)
	if(y<=SOM::xyz[1]+SOM::L.duck+1)
	return p;
	return 0;
}
extern BYTE __cdecl som_logic_408cc0(som_EVT *event)
{
	int obj = event->us[16];

	if(!som_logic_408cc0_yh(event)) return 0;
	
	if(2==event->c[31]) //2022: match door activation?
	{		 		
		auto&ai = SOM::L.ai3[obj]; if(obj>=512) return 0;
		int prm = ai[SOM::AI::object]; if(prm>=1024) return 0;
		int pr2 = SOM::L.obj_prm_file[prm].us[18];
		if(pr2<1024)
		{
			int cmp = SOM::L.obj_pr2_file[pr2].us[41]; //DEBUGGING
			switch(cmp)
			{
			case 0xb: case 0xd: case 0xe: //door?

				extern bool som_state_42bca0_door(float(&)[3],int);
				float neg[3] = { -SOM::xyz[0],0,-SOM::xyz[2] };
				return som_state_42bca0_door(neg,event->us[16]);
			}
		}
	}
	
	return ((BYTE(__cdecl*)(void*))0x408CC0)(event);
}
static BYTE __cdecl som_logic_409200(DWORD event) //circle
{
	float *p = som_logic_408cc0_yh(SOM::L.events+event);
	if(!p) return 0;

	float dx = SOM::xyz[0]-p[0];
	float dz = SOM::xyz[2]-p[2];
	float d = sqrtf(dx*dx+dz*dz);

	auto e = (swordofmoonlight_evt_header_t::_item*)(SOM::L.events+event);

	//0.5 is presumably the PC's width
	return d<e->circle_r+0.25f;
}
static BYTE __cdecl som_logic_409080(DWORD event) //square
{
	float *p = som_logic_408cc0_yh(SOM::L.events+event);
	if(!p) return 0;

	//NOTE: the below code should reproduce 409080
	auto e = (swordofmoonlight_evt_header_t::_item*)(SOM::L.events+event);
	//if(~e->ext_zr_flags&2)
	//return ((BYTE(__cdecl*)(DWORD))0x409080)(event);

	int todolist[SOMEX_VNUMBER<=0x1020602UL];
	float d[3]; memcpy(d,SOM::xyz,sizeof(d));
	for(int i=3;i-->0;) d[i]-=p[i];

	//TODO: lateral rotation? PC width?	
	if(e->ext_zr_flags&2) SOM::rotate(d,0,-p[4]);

	//2x is because the square units are end to end
	//0.5 is presumably twice the PC's width
	//004091A7 D8 25 40 83 45 00    fsub        dword ptr ds:[458340h]
	d[0] = fabsf(d[0])*2-0.5f;
	d[2] = fabsf(d[2])*2-0.5f;
	if(d[0]<=e->square_x&&d[2]<=e->square_y)
	{
		return true; //breakpoint
	}
	return false;
}

//2018 October
extern void __cdecl som_logic_40C8E0_offset(float p[3], float r, const float cp[3])
{
	//HELPS
	//object explosion points are funky???	
	//REMINDER: 
	//this is not correct for splash effects, however
	//due to this being low-level there is no way to 
	//know. luckily the hit site appears to be ignored
	//in that case, at least for tested effects
	//p[1] = cp[1]; 
	memcpy(p,cp,3*sizeof(float)); //better

	//helps with explosion sprites
	//but what about MHM and NPCs?
	//if(1&&EX::debug)
	{
		//HACK: limit for 3-stage effects like the?
		//Moonlight Sword's magic (stage 2) 
		//I think the caller disregards the stage 2
		//hit location (and uses the NPC's instead)
		r = min(r,0.5f)*1.25f;

		for(int i=0;i<3;i++) p[i]-=SOM::pov[i]*r;
	}
}
//probing problem with projectile/cylinder object clipping
//0040ED80 E8 5B DB FF FF       call        0040C8E0 
//0040ED85 83 C4 1C             add         esp,1Ch
//NPCs
//0040B888 E8 53 10 00 00       call        0040C8E0
//enemies?
//0040B72E E8 AD 11 00 00       call        0040C8E0 
extern BYTE __cdecl som_logic_40C8E0 //CYLINDERS
(FLOAT _1[3], FLOAT _2, FLOAT _3[3], FLOAT _4, FLOAT _5, FLOAT _6[3], FLOAT _7[3])
{			
	//SFX?
	//at least some SomEx.dll routines repurpose som_logic_40C8E0
	//
	// WARNING
	// image based SFX effects (SFX_images) may come through here
	//
	bool sfx = (size_t)_1-(size_t)&SOM::L.SFX_instances<126*4*512;
	if(sfx) //som_logic_40c470_min(FLOAT*)
	{
		DWORD n = size_t((char*)_3-SOM::L.ai->c)/sizeof(*SOM::L.ai);

		if(n>=SOM::L.ai_size)
		{
			n = size_t((char*)_3-SOM::L.ai2->c)/sizeof(*SOM::L.ai2);

			if(n>=SOM::L.ai2_size)
			{
				n = 100000;
			}
			else SomEx_npc = 1<<12|(int)n;			
		}
		else 
		{
			SomEx_npc = n;

			if(som_logic_softclip&&!SOM::emu) //HACK
			{
				auto &mdl = *(SOM::MDL*)SOM::L.ai[n][SOM::AI::mdl];

				_3 = mdl.ext.clip.soft2;
			}
		}

		if(n!=100000)
		{
			EX::INI::Adjust ad;
			float f;
			if(&ad->npc_hitbox_clearance)
			f = ad->npc_hitbox_clearance();
			else
			f = ad->npc_hitbox_clearance.o(); //0.5?	

			SomEx_npc = -1; //NaN?

			if(f)
			{
				//because of scaling there's no other way that wouldn't effect
				//the clip radius adversely... assuming they're the same value
				_4 = max(_4-f,0.1f);	
			}
		}
		else sfx = false; //2022: let som_logic_40ec30 have it?
	}

	BYTE ret; if(1) //particle+cylinder
	{
		//there seems to be a problem for fat (disc like) cylinders
		//it's not difficult to implement this algorithm

		float diff[3]; Somvector::map(diff).copy<3>(_1).remove<3>(_3);
		if(diff[1]+_2<0||diff[1]>_5+_2) 
		return 0;
		//diff[1] = diff[2];
		//if(Somvector::map(diff).length<2>()-_2>_4)
		if(sqrtf(diff[0]*diff[0]+diff[2]*diff[2])-_2>_4)
		return 0;

		//2021: som.mocap.cpp is using this for some reason
		//(previously it just needed a hit test)
		assert(_6&&_7);
		{
			Somvector::map(diff).unit<3>();
			memcpy(_7,diff,3*sizeof(float));
			_6[0] = _3[0]+_7[0]*_4;
			_6[1] = _3[1];
			_6[2] = _3[2]+_7[2]*_4;
		}

		ret = 1;
	}
	else ret = 
	((BYTE(__cdecl*)(FLOAT*,FLOAT,FLOAT*,FLOAT,FLOAT,FLOAT*,FLOAT*))0x40C8E0)
	//_1 is projectile center
	//_2 is projectile radius
	//_3 is position
	//_4/_5 is radius/height (originally objects reversed these two)
	//_6/_7 return hit point/penetration
	(_1,_2,_3,_4,_5,_6,_7);

	//adjust explosion decal's depth
	if(ret&&sfx) som_logic_40C8E0_offset(_6,_2,_1);

	return ret; //breakpoint
}
/*2022: adding MHM (som_logic_40ec30)
static BYTE __cdecl som_logic_40dc70 //BOXES
(FLOAT _1[3], FLOAT _2, FLOAT _3[3], FLOAT _4, FLOAT _5, FLOAT _6, FLOAT _7, FLOAT _8[3], FLOAT _9[3])
{
	//ball traps seem to generate a projectile that hits other NPCs, and 
	//an object also. offsetting it on hit shouldn't hurt and might even
	//help if it can be tied to an SFX somehow
	BYTE ret = ((BYTE(__cdecl*)(FLOAT*,FLOAT,FLOAT*,FLOAT,FLOAT,FLOAT,FLOAT,FLOAT*,FLOAT*))0x40DC70)
	//_1 is projectile center (center of ball trap)
	//_2 is projectile radius (1.425 for ball trap... what is this?)
	//_3 is position
	//_4/_5 is height/width (1/3.4 for ball trap... this is another object)
	//_6/_7 is depth/rotation (3.4 for ball trap)
	//_8/_9 return hit point/penetration
	(_1,_2,_3,_4,_5,_6,_7,_8,_9);

	if(ret) som_logic_40C8E0_offset(_8,_2,_1); return ret;
}*/
static BYTE __cdecl som_logic_40ec30(FLOAT _1[3], FLOAT _2, DWORD obj, FLOAT _4[3], FLOAT _5[3])
{
	if(!((BYTE(__cdecl*)(FLOAT*,FLOAT,DWORD,FLOAT*,FLOAT*))0x40ec30)(_1,_2,obj,_4,_5))
	return 0;

	auto &ai = SOM::L.ai3[obj];
	auto *mdl = (SOM::MDL*)ai[SOM::AI::mdl3];
	auto *mdo = (SOM::MDO*)ai[SOM::AI::mdo3]; assert(mdl||mdo);
	if(auto*mhm=mdo?mdo->mdo_data()->ext.mhm:mdl?mdl->mdl_data->ext.mhm:0)
	{
		SOM::Clipper sfxclip(_1,_2,_2,0);
		if(!sfxclip.clip(mhm,mdo,mdl,_4,_5)) return 0;
	}
		
	som_logic_40C8E0_offset(_4,_2,_1); return 1;
}

extern int som_logic_4041d0_move = -1;
static void __cdecl som_logic_4041d0(SOM::Attack *atk)
{
	if(som_logic_4041d0_move>-1) //som_game_moveset 
	{
		auto *item_arm = SOM::PARAM::Item.arm->records; 
		atk->radius*=item_arm[som_logic_4041d0_move]._scale; //2023
	}
	((void(__cdecl*)(void*))0x4041d0)(atk);
}

#pragma optimize("",on) 
static BYTE __declspec(naked) som_logic_40c2e0 //disc vs disc
(FLOAT xyz[3], float h, float r, float cmp[3], float hh, float rr, DWORD mode, float[3], float[3])
{
	__asm mov eax,40c2e0h; 
	__asm jmp eax; 
}
#pragma optimize("",off) 

static int som_logic_40dff0 //2021
(FLOAT _1[3], FLOAT _2, FLOAT _3, DWORD _4, DWORD _5, FLOAT _6[3], FLOAT _7[3])
{
	extern int *som_clipc_objectstack;
	auto obj = _5?som_clipc_objectstack[_4]:SOM::frame&1?_4:SOM::L.ai3_size-1-_4;

	if(_5==1) //vertical?
	return SOM::clipper_40dff0(_1,_2,_3,obj,14,_6,_7);	
	else assert(0==_5);

	auto &ai = SOM::L.ai3[obj];	
	auto *mdl = (SOM::MDL*)ai[SOM::AI::mdl3];
	auto *mdo = (SOM::MDO*)ai[SOM::AI::mdo3]; //assert(mdl||mdo);
	auto *mhm = mdo?mdo->mdo_data()->ext.mhm:mdl?mdl->mdl_data->ext.mhm:0;	

	if(_5==0) //lateral?	
	{
		int cling = 0; bool open,door;

		static const float oo2 = 0.002f; //YUCK

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
					
		//	if(open) SOM::shoved = SOM::frame; //testing
			if(open) cling|=4;
		}				

		//if(!som_clipc_x40dff0(_1,_2,_3,obj,_5,_6,_7)) //2-som_clipc_haircut
		if(!((BYTE(*)(FLOAT*,FLOAT,FLOAT,DWORD,DWORD,FLOAT*,FLOAT*))0x40DFF0)
		(_1,_2,_3,obj,_5,_6,_7))
		return 0;
		if(mhm) //2022: refine?
		{
			float yy = _1[1];

			SOM::Clipper pclip(_1,_2,_3,/*14*/12); //5 //SOM::L.fence
			if(mhm->types124[2]
			&&pclip.clip(mhm,mdo,mdl,_6,_7)
			&&pclip.elevator!=1)
			{
				extern float som_clipc_opposite(float,float);
				float e = som_clipc_opposite(_3/*+oo1*2*/,pclip.elevator);	
				_1[1]+=e;
			//	pclip.height = max(0.001f,_2-max(e,som_clipc_haircut));
				pclip.height = max(0.001f,_2-e);
			}
			else pclip.height = _2; //_2-som_clipc_haircut;

			pclip.mask = 5;

			bool out = pclip.clip(mhm,mdo,mdl,_6,_7);

			_1[1] = yy; if(out)
			{
				//YUCK: just return a position like MHM
				//since slopes can't shift the position
				memcpy(_6,pclip.pclipos,3*sizeof(float));
				memset(_7,0x00,3*sizeof(float));

				if(pclip.cling) cling|=2; //SOM::motions.cling = true;
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
					extern float som_clipc_40D750_4[3];
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

			cling|=2; //SOM::motions.cling = true;

			//this fix is needed to stop clinging
			//(it just makes objects behave like MHM walls)
			_6[0]+=oo2*_7[0]; _6[2]+=oo2*_7[2];
		}

		return 1|cling;
	}
	else assert(0); return 0; //compiler //UNREACHABLE
}

static void __cdecl som_logic_42b9e0(DWORD obj) //KF2 holds open chests
{
	//NOTE: som_game_410620 ignores items under chests

	auto &ai = SOM::L.ai3[obj]; 
	auto mdl = (SOM::MDL*)ai[SOM::AI::mdl3];	
//	int d = mdl?mdl->d:0;

	bool was_empty = ai.uc[36]==0xff;
	((void(__cdecl*)(DWORD))0x42b9e0)(obj);
	if(ai.s[63]!=8) return; //finished?

	//this never gets old! 
	int prm = ai[SOM::AI::object];
	WORD pr2 = SOM::L.obj_prm_file[prm].s[18];
	int type = SOM::L.obj_pr2_file[pr2].s[41];
	if(type!=0x15) return;

	//som_logic_reprogram has knocked out the subtitle
	//for empty containers, so it needs to be restored
	//in case using classic model
	if(ai.uc[36]==0xff&&(BYTE)ai.c[37]>=0xfa) //empty & unlocked?
	{
		//EXTENSION (do_kf2?)
		//currently requiring no close animation (5) to
		//request KF2 behavior
		//NOTE: will have to hold open and probably get
		//the state from the save file
		if(mdl&&5!=mdl->animation_id()) 
		{
			if(mdl->d>=mdl->running_time(mdl->c)-1)
			{
				if(5!=ai.c[0x7c])
				{
					ai.c[0x7c] = 5; //state

					extern bool som_game_410620_item_detected; //HACK
					if(was_empty&&!som_game_410620_item_detected)
					((BYTE(__cdecl*)(DWORD,DWORD))0x423ce0)(0x1d1adf0,0); //Empty?
				}
			}
			return;
		}

	//	((BYTE(__cdecl*)(DWORD,DWORD))0x423ce0)(0x1d1adf0,0); //Empty?
	}
	else //NOTE: this is knocked out by som_logic_reprogram
	{
		((void(__cdecl*)(DWORD,DWORD))0x42b510)(obj,5); //close?

		ai.us[0x7e/2] = 8*DDRAW::refreshrate/30; //2024: hold open?
	}
}
static void __cdecl som_logic_4218e0(DWORD obj) //2022: idle state?
{
	auto &ai = SOM::L.ai3[obj]; 
	if(auto*mdl=(SOM::MDL*)ai.i[SOM::AI::mdl3])
	if(mdl->d>1&&5==mdl->animation_id()) //standardize state after closing?
	{
		((BYTE(__cdecl*)(SOM::MDL*,DWORD))0x4412E0)(mdl,mdl->animation(4));
		assert(mdl->d<=1);
	}
}
static void __cdecl som_logic_42bb80(DWORD obj) //KF2 holds open doors
{
	//EXTENSION?
	//The vertical doors don't clip well when coming down on the 
	//head of the player character, and they can trap inside with
	//the som.hacks.cpp code that manages bad clips
	auto &ai = SOM::L.ai3[obj]; if(auto&t=ai.s[0x7e/2])
	{
		WORD pr2 = SOM::L.obj_prm_file[ai[SOM::AI::object]].s[18];
		WORD type = SOM::L.obj_pr2_file[pr2].s[41];
		if(type>=0xb&&type<=0xe)
		{
			bool sd = 0xb==type; //secret/sliding door?
			bool dd = (type&1)==0; //double door?
			bool cm = 1==SOM::L.obj_pr2_file[pr2].c[80];
				
			//don't close at the activation distance
			float reach = EX::INI::Player()->player_character_radius2;

			//NOTE: sliding doors are using a completely different
			//methodology below since they don't move outside of 1
			//dimension, the rest are using a center metric that's
			//weigthed toward the open door position, but still is
			//intended to not shut itself from activation distance

			//float *pos = &ai[SOM::AI::xyz3];
			float *box = &ai.f[SOM::AI::box3],_[3];
			//the secret doors are like superwide single doors, so
			//max doesn't work well at all in their case
			float r = box[1];
			if(cm) r = sd?min(r,box[2]):max(r,box[2]);
			//else r*=2; //diameter? double doors have a diameter?			
			//2020: cut double door distance down to a single door			
			if(dd) r/=2;
			if(sd) reach+=0.5f; //match som_state_42BCA0
			if(sd&&!cm) //not expecting this
			{
				//SOM's secret doors/walls are super wide
				assert(0);
				if(r/ai[SOM::AI::scale3]>2) r/=2;
			}
			if(!cm) //double doors? need to fix their PRF
			{
				r = r; //breakpoint
			}

			//find door's true center?
			float pos[3] = {}; 			
			auto mdl = (int*)ai.i[SOM::AI::mdl3];
			int *i7 = *(int**)(*mdl+0x9b0);
			if(i7&&*i7) //2?
			{				
				//Ghidra code
				//puVar3 = (undefined4*)
				//(i7[param_1[0xc] + 0x22] + param_1[0xd] * 0xc + (int)i7 + *(local_cc));
					
				float w = 1.0f/(dd?8:4);

				int *cps = i7+2;
				for(int i=dd?8:4;i-->0;)
				{
					auto cp = (float*)(i7[0x22]+(int)i7+cps[i]);
					for(int j=3;j-->0;) pos[j]+=cp[j]*w;
				}
				auto m = (float*)mdl+0x19;
				Somvector::map(pos).premultiply<4>(Somvector::map<4,4>(m));

				//using the close animation can gracefully handle
				//assymetry in the sides of the door
				if(1)
				{
					float pos2[3] = {}; //averaging

					//center of opened door?
					r*=0.8f;

					for(int i=dd?8:4;i-->0;)
					{
						float cp[3];
						((BYTE(__cdecl*)(void*,DWORD,FLOAT*))0x441600)(mdl,i,cp);
						for(int j=3;j-->0;) pos2[j]+=cp[j]*w;
					}
					for(int j=3;j-->0;) if(sd)
					{
						//factor new position into stest
						pos[j] = pos2[j]-pos[j];
					}
					else pos[j] = pos[j]*0.4f+pos2[j]*0.6f;
				}
			}
			else memcpy(pos,&ai[SOM::AI::xyz3],sizeof(pos));

			if(!sd) pos[1] = 0;
			float cmp[3] = {SOM::L.pcstate[0],0,SOM::L.pcstate[2]};
			//slide clip shape along with door?
			if(sd) cmp[1] = (&ai[SOM::AI::xyz3])[1];
			if(sd) for(int j=3;j-->0;) cmp[j]+=pos[j];
				
			//this is trying to match som_state_42BCA0 but I can't figure
			//out why it's different!
			const float fudge = 0.2f; //0.1f; 

			bool stest = sd&&som_logic_40dff0(cmp,box[0],r+reach+fudge,obj,0,_,_);

			if(stest||!sd&&som_logic_40C8E0(pos,r,cmp,reach,SOM::L.height,_,_))
			{
				assert(t<=120); 
				
				//if(t>1) t--; return; //t = 120;

				t = 120; return;
			}
			else if(t==120) //opening animations are plenty long to wait
			{
				//I think 120 is 4 seconds, but it's 2 seconds if the 60
				//fps fix is in play, either way it seems too long with
				//the door held open
				t = 30; 
			}
			else if(t==1)
			{
 				t = t; //breakpoint
			}
		}
		else if(type==0x1e) //spear trap? (som_logic_reprogram)
		{
			t = 0; //don't hold open traps
		}
	}	
	((void(__cdecl*)(DWORD))0x42bb80)(obj);
}

static void __cdecl som_logic_42BE70(SOM::Struct<46> *trap)
{			
	if(4==trap->c[0x7c]) //opening spear trap?
	{
		//TODO? support playing sound while closing?

		auto &ai = *trap;

		WORD pr2 = SOM::L.obj_prm_file[ai[SOM::AI::object]].s[18];

		auto obj2 = (swordofmoonlight_prf_object_t*)(SOM::L.obj_pr2_file[pr2].c+62);

		//HACK: play third sound?
		if(obj2->openingSND_delay)		
		if(int cmp=obj2->loopingSND_delay)
		{
			int *mdl = (int*)ai.i[SOM::AI::mdl3];
			if(cmp==mdl[0xd])
			SOM::se3D(ai.f+SOM::AI::xyz3,obj2->loopingSND,obj2->loopingSND_pitch);
		}
	}
	((void(__cdecl*)(void*))0x42BE70)(trap);
}

static void __cdecl som_logic_4066d0(DWORD _1, FLOAT _2)
{
	auto &ai = SOM::L.ai[_1]; 
	INT32 &status = ai.i[143]; //1
	assert(1==status);
	((void(__cdecl*)(DWORD,FLOAT))0x4066d0)(_1,_2); //spawn?
	if(status!=1)
	if(status==3)
	{
		//the goal at hand is to reset any events attached to
		//a series of monsters so every instance gets its own

		if(0!=ai[SOM::AI::entry]) return; //eventual/undying?

		//note sure what these are exactly... is the MAP data 
		//available? I think spawn is the current number
		BYTE instances = ai[SOM::AI::instances];
		
		//even if there's only 1 instance 100% of the time the
		//monster still respawns
		if(1==instances) return;

		DWORD i = SOM::L.events_size;
		while(i-->10)
		{
			auto &e = SOM::L.events[i];
			if(e.s[16]==_1&&1==e.c[31]) //enemy?
			{
				SOM::L.leafnums[i] = 0;
			}
		}
	}	
	else assert(2==status); //failed to spawn by random chance?
}

static void som_state_40c2e0_pc2(SOM::MDL *mdl)
{
	if(mdl)
	{
		mdl->ext.pc_clip = SOM::frame;
	}
	else assert(0); 
	
	//NOTE: this is in the context of som_state_426D60
	//and is just for the shield effect. I'm uncertain
	//it's correct for the other effects
	SOM::motions.cling = true; 
}
static BYTE __cdecl som_state_40c2e0_pc(FLOAT pc[3], float _2, float _3, 
float npc[3], float _5, float _6, DWORD _7, float _8[3], float _9[3])
{
	if(!((BYTE(__cdecl*)(FLOAT[3],float,float,float[3],
	float,float,DWORD,float[3],float[3]))0x40c2e0)(pc,_2,_3,npc,_5,_6,_7,_8,_9))
	return 0;

	SOM::MDL *mdl;
	DWORD i = ((DWORD)npc-(DWORD)SOM::L.ai)/596; 
	if(i>=SOM::L.ai_size)
	{		
		i = ((DWORD)npc-(DWORD)&SOM::L.ai2)/172; 

		auto &ai = SOM::L.ai2[i]; assert(npc==ai.f+SOM::AI::xyz2);

		mdl = i<128?(SOM::MDL*)ai.i[SOM::AI::mdl2]:0;
	}
	else
	{
		auto &ai = SOM::L.ai[i]; assert(npc==ai.f+SOM::AI::xyz);

		mdl = (SOM::MDL*)ai.i[SOM::AI::mdl];
	}
	som_state_40c2e0_pc2(mdl); return 1;	
}
 
#pragma optimize("",off) 
static void __cdecl som_logic_40c470_min(FLOAT *esp) //YUCK
{
	EX::INI::Adjust ad;

	DWORD n = size_t(*(char**)(esp+6)-SOM::L.ai->c)/sizeof(*SOM::L.ai);

	bool npc = n>=SOM::L.ai_size;

	extern int SomEx_npc;

	if(npc)
	{
		n = size_t(*(char**)(esp+6)-SOM::L.ai2->c)/sizeof(*SOM::L.ai2);

		SomEx_npc = 1<<12|(int)n;			
	}
	else SomEx_npc = n;

	float f = ad->npc_hitbox_clearance(); //0.5

	SomEx_npc = -1;

	if(!npc&&som_logic_softclip&&!SOM::emu) //HACK
	{
		auto &mdl = *(SOM::MDL*)SOM::L.ai[n][SOM::AI::mdl];

		((float**)esp)[6] = mdl.ext.clip.soft2;
	}

	//this increases the range. 0.5 is added to the PRF range :(
	//NOTE: this isn't the place to extend this. do that when the
	//weapon is equipped or the alternate attack is selected
	//esp[3]+=f; 

	//because of scaling there's no other way that wouldn't effect
	//the clip radius adversely... assuming they're the same value
	esp[8] = max(esp[8]-f,0.1f);
}
static BYTE __declspec(naked) som_logic_40c470()
{
	__asm //10 arguments is a lot to forward (trying something new)
	{		
	//fld         dword ptr [esp+20h]  
	//fsub        dword ptr ds:[som_logic_40c470_f]
	//fstp        dword ptr [esp+20h]
	push esp
	call som_logic_40c470_min
	pop esp
	//can't use call because caller has all the registers
	//it adds the return address to the ESP
	mov eax,40c470h
	jmp eax
	}
}
//2020: TESTING MODE 4 DAMAGE (CUBOIDAL SFX?)
static void __cdecl som_logic_40B1D0_assert()
{
	//40B1D0 uses SOM::AI::shadow as a damage
	//radius. what does this? it definitely 
	//needs to be fixed if anything
	//
	//TODO: SHOULD som_MPX_405de0 BE DIAMETER?
	assert(!som_logic_40B1D0_assert);
}
static BYTE __declspec(naked) som_logic_40B1D0()
{
	__asm
	{		
	push esp
	call som_logic_40B1D0_assert
	pop esp	
	mov eax,40B1D0h
	jmp eax
	}
}
//#pragma optimize("",on) 
//#pragma optimize("",off) 
//REMOVE ME?
/*crashes on unload... the SFX is absorbed into the target anyway
static BYTE __declspec(naked) som_logic_42e5c0_a(DWORD _sfx)
{
	__asm //10 arguments is a lot to forward (trying something new)
	{
	mov eax,dword ptr[esp+4]
	cmp eax,dword ptr[esp+284] //_sfx
	je _ret
	mov eax,42e5c0h
	jmp eax
	_ret: ret
	}
}
static BYTE __declspec(naked) som_logic_42e5c0_b(DWORD _sfx)
{
	__asm //10 arguments is a lot to forward (trying something new)
	{
	mov eax,dword ptr[esp+4]
	#error 284 is not right
	cmp eax,dword ptr[esp+284] //_sfx
	je _ret
	mov eax,42e5c0h
	jmp eax
	_ret: ret
	}
}*/
static void __cdecl som_logic_42f1f0(DWORD *sfx, DWORD _2, DWORD _3, DWORD _4)
{
	((void(__cdecl*)(DWORD*,DWORD,DWORD,DWORD))0x42f1f0)(sfx,_2,_3,_4);

	if(sfx[28]==2||sfx[28]==4) //guesses/testing
	{
		auto &dat = SOM::L.SFX_dat_file[((WORD*)sfx)[1]];

		//TODO??? interpret a silent/recursive SFX this way???
				
		if(-1==dat.f[2]&&-1==dat.f[3]) //meaningful to procedure 5?
		{
			//HACK: I don't know if any orginal SFX entries use -1,-1
			//for their explosions... (I've seen some use 65535,65535)
			//but I chose this to represent a windcutter behavior until
			//I can figure out a better system
			return;
		}
	}

	sfx[11] = 1; //NOTE: the caller does this when it's not knocked out
}
#pragma optimize("",on) 

void SOM::rotate(float vec[3], float x, float y)
{
	if(x) //449d80
	{
		float swap = vec[2];
		float c = cosf(x);
		float s = sinf(x);
		vec[2] = swap*c-vec[1]*s;
		vec[1] = swap*s+vec[1]*c;
	}
	if(y) //449dc0
	{
		float swap = vec[0];
		float c = cosf(y);
		float s = sinf(y);
		vec[0] = swap*c-vec[2]*s;
		vec[2] = swap*s+vec[2]*c;
	}
}

//2021: this is combined clipping for enemies/NPCs
static bool som_logic_4079d0_42a0c0(SOM::MDL &mdl, float cp[3], 
const float *sdr, const float height, float xyzuvw[3], bool flying)
{
	assert(-1!=SomEx_npc);

	const float step = SOM::L.fade;

	auto &clip = mdl.ext.clip; //enum{ clip=1 };

	EX::INI::Adjust ad;
	float r = sdr[2]; //radius
	if(r<=0) //2022: preventing assert/drawing dummies
	{
		return false; //HACK: REMOVE ME?
	}
	float f = clip? //0.701f (0.2 is subtracted below)
	&ad->npc_fence?ad->npc_fence():ad->npc_fence.o() //0.25
	:	0.5f; //0.25 can push through the ground
	float h = f+0.01f;

	
	//NOTE: this was originally done differently
	//float *xyzuvw = &ai[SOM::AI::xyzuvw];
	for(int i=3;i-->0;) cp[i]+=xyzuvw[i];

	const float x = cp[0], y = cp[1], z = cp[2];

	/*I'm not sure NPCs have the same need as the PC in this
	//case... it does work
	if(som_logic_softclip) //som_state_cling?
	{
		float t = clip.clinging;
		
		//can't seem to climb stairs... I don't know if this
		//is strictly required now to avoid tunneling through
		//walls
	//	t*=0.5f;

		t = 1-t*t; 
		cp[0]+=(clip.soft[0]-cp[0])*t; 
		cp[2]+=(clip.soft[2]-cp[2])*t;
	}*/

	//TODO: implement do_g for NPCs
	//float g = 0.2f; //0.2 is a lot?
	//HACK: at least at 60 fps som_state_426D60_1 tends to be 0.01
	//it's 0.006*SOM::motions.tick although do_g manipulates 0.006
	//33ms is 0.2 (30 fps)
	//float g = EX::INI::Bugfix()->do_fix_animation_sample_rate?0.1f:0.2f;
	float g; if(EX::INI::Option()->do_g)
	{
		g = EX::INI::Detail()->g;

		//FIX ME
		//add enough to clear the MHM
		//tolerance, assuming gravity
		//is Earth like
		int todolist[SOMEX_VNUMBER<=0x1020602UL];
		float t = clip.falling+0.02f;
		float d1 = 0.5f*g*(t*t); 
		t+=step;
		float d2 = 0.5f*g*(t*t); 

		g = d2-d1;

		if(clip.falling)
		clip.falling2+=g; //debugging
	}
	else g = 6*step; //0.1/0.2 (same as above)

	if(!flying) //not flying?
	{
		//NOTE: originally flying monsters keep the fixed 0.701 height
		cp[1]-=g; h+=g; 
	}	

	//LEAKY: som_MHM_layers is quick/dirty only tests center of
	//objects (on the fly) within vertical bounds of MHM... THE
	//LAYER OWNERSHIP SHOULD BE COMPUTED ONLY WHEN OBJECTS MOVE
	int lm = 0; extern int som_MHM_layers_IMPRECISE_obj(int);

	float p3[3],n3[3]; //returned data
	extern BYTE __cdecl som_MHM_4159A0(FLOAT*,FLOAT,FLOAT,DWORD,FLOAT*);

	float yy = cp[1], summit = cp[1]; //HACK

	//NOTE: originally fliers could climb, but they would
	//never fall down, so I'm disabling it
	int passes = flying?0:clip.npcstep?1:2; //emu?
	while(passes-->0)
	{
		//EXTENSION
		//this does a second pass if climbing so it won't
		//climb until more of the NPC's body is over the
		//platform

		/*HOPPING?!
		//adding npcstep is causing the NPCs to climb and
		//fall at the same time... the climb isn't canceled
		//and it looks a little bit like they're hopping up
		//onto stairs, which is kind of cute... it makes no
		//sense to me that a larger radius induces falling?
		if(!passes&&!clip.npcstep)*/
		if(!passes)
		{
			/*at some point mulitplying wasn't working, but
			//it is now, which is good because softolerance
			//is a small value
			r-=clip.softolerance;
			r = max(r,0.001f);*/
			/*1/3 is best for KF2's monsers with the current
			//settings, but this needs to be extensible, and
			//the extension should be absolute terms so that
			//it doesn't break in case the parameters change
			r*=0.25f;*/			
			r*=0.3333f; //EXTENSION
		}

		bool footing = false; //som_state_slipping

		for(int i=0,n=SOM::L.ai3_size;i<n;i++)	
		if(som_logic_40dff0(cp,h,r,i,1,p3,n3)) //climbing?
		{
			footing = true;

			cp[1] = p3[1]; lm|=som_MHM_layers_IMPRECISE_obj(i);
		}		
		if(clip) //DUPLICATE? som_state_4159A0
		{
			//som_MHM_4159A0 alone is dropping through the floor with 
			//npc_fence (values less than 0.5)

			//HACK: at least at 60 fps som_state_426D60_1 tends to be 0.01
			//float up = !som_clipc_slipping?0.000001f:som_state_426D60_1;
			float up = 0;//footing?0.000001f:g;
			if(BYTE l=SOM::clipper.clip(cp,h,r,14,up,cp))
			{
				lm|=l; footing = true;

				//NOTE: I've never seen monsters struggle on slopes but
				//the PC does so they should too
				float slope = M_PI_2-asinf(SOM::clipper.slopenormal[1]);	
				if(slope<0) slope = 0; //-0? 
				if(SOM::clipper.slopefloor<SOM::clipper.floor)
				cp[1] = SOM::clipper.floor+0.001f; 
				if(slope&&clip.footing) 
				{
					if(cp[1]-xyzuvw[1]<-0.05f) cp[1] = xyzuvw[1]-0.02f; 
				}
			}
		}
		else if(BYTE l=som_MHM_4159A0(cp,h,r,14,p3)) //climbing?
		{
			lm|=l; footing = true; cp[1] = p3[1]; 
		}

		clip.footing = footing;

		bool climbing = cp[1]-y>=0.01f;

		if(footing) summit = max(summit,cp[1]);

		if(passes)
		{
			if(!footing) //falling?
			{
				clip.npcstep = 0;
			}

			//climb/fall with smaller radius?
			if(climbing||footing)
			{
				cp[1] = yy; continue;
			}
		}
		else if(climbing&&!clip.npcstep)
		{
			clip.npcstep = clip.npcstep2 = cp[1]-y; 
		}
		//WARNING
		//this is leaving xyzwuv at the climbed height
		yy = cp[1]; break;		
	}
	bool fell; if(!clip.footing&&!flying)
	{
		//NOTE: sdr[0]>r is usually true and harmless
		//to ignore, but it illustrates that the test
		//is unnecessary if sdr[0]<=r
		fell = !clip.falling&&sdr[0]>r; //shadow

		if(!clip.falling)
		clip.falling2 = 0; //debugging
		clip.falling+=step;
	}
	else 
	{
		fell = false; clip.falling = 0;
	}
	
	bool clinging = false;
	bool climbing = summit>y||clip.npcstep;
	
		//starting horizontal clipping

	if(som_logic_softclip)
	{
		if(clip.falling)
		{
		//	assert(sdr[0]<=sdr[2]);

			r = max(r,sdr[0]); //shadow
		}
		else r = sdr[2]; //radius

		cp[0] = x; cp[2] = z; //clinging?
	}
	cp[1] = summit+0.01f; //HACK

	/*NOTE: everything below uses this 90% radius except 
	//for the object test... it seems to work either way
	//I think maybe it's to reach a platform but strange
	//to decrease here instead of increase above
	if(cp[1]>xyzuvw[1]) //???																    
	{
		r*=0.9f; //???
	}*/
	h = height; //REPURPOSING

	//HACK: sloping ceilings seem to be an issue in 
	//KF2's tunnel stairs
	if(climbing||!clip.footing&&!flying)
	{
		if(h>f) h-=f; //REMOVE ME
	}

	//HACK: allow to clip through more than
	//one step on a staircase... can't go
	//over fence without breaking rules
	//
	// WARNING: this may let NPCs climb
	// very thin platforms to some extent
	//
	if(climbing) //if(clip.npcstep)
	{
		//FYI: 0 height passes through walls

		cp[1]+=f; if(h>f) h-=f; //haircut?
	}

	//WARNING: MAYBE NO LONGER REACHABLE
	//I don't know how to calculate this... relaxation
	//doesn't go quite down to 0... it depends on the
	//calibration
	const float tol = 0.01f;

	bool ret = false;
	float soft2[3] = {clip.soft[0],cp[1],clip.soft[2]};
	if(clip.soft2[3]>tol)
	{
		ret = true;
		
		passes = 2;
	}
	else passes = 1; while(passes-->0) fell2: //emu?
	{
		float *cq = passes?soft2:cp;

		bool hit = false; //YUCK

		//NOTE: som_state_40c2e0_pc WON'T WORK WITH cp
		if(som_logic_40c2e0(cq,h,r,SOM::L.pcstate,SOM::L.duck,SOM::L.hitbox2,0,p3,n3))
		{
			hit = true;

			cq[0] = p3[0]+r*n3[0]; cq[2] = p3[2]+r*n3[2];

			som_state_40c2e0_pc2(&mdl); //EXTENSION
		}		
		for(int i=0,n=SOM::L.ai_size;i<n;i++)	
		{	
			if(3!=SOM::L.ai[i][SOM::AI::stage]) continue;

			float *cmp = &SOM::L.ai[i][SOM::AI::xyz];
			float rr = SOM::L.ai[i][SOM::AI::radius];
			float hh = SOM::L.ai[i][SOM::AI::height];

			if(xyzuvw!=cmp)
			if(som_logic_40c2e0(cq,h,r,cmp,hh,rr,0,p3,n3))
			{
				hit = true;
				cq[0] = p3[0]+r*n3[0]; cq[2] = p3[2]+r*n3[2];
			}
		}
		for(int i=0,n=SOM::L.ai2_size;i<n;i++)	
		{
			if(3!=SOM::L.ai2[i][SOM::AI::stage2]) continue;

			float *cmp = &SOM::L.ai2[i][SOM::AI::xyz2];
			float rr = SOM::L.ai2[i][SOM::AI::radius2];
			float hh = SOM::L.ai2[i][SOM::AI::height2];

			if(xyzuvw!=cmp)
			if(som_logic_40c2e0(cq,h,r,cmp,hh,rr,0,p3,n3))
			{
				hit = true;
				cq[0] = p3[0]+r*n3[0]; cq[2] = p3[2]+r*n3[2];
			}
		}

		//WARNING: THIS TEST USES THE ORIGINAL r VALUE
		//BUT MAYBE IT'S A MISTAKE... I'M NOT SURE WHY
		//r IS REDUCED IN THE FIRST PLACE
		for(int i=0,n=SOM::L.ai3_size;i<n;i++)	
		if(int cling=som_logic_40dff0(cq,h,r,i,0,p3,n3)) //r?
		{
			if(cling&2) clinging = true;

			cq[0] = p3[0]+r*n3[0]; cq[2] = p3[2]+r*n3[2];

			lm|=som_MHM_layers_IMPRECISE_obj(i); //appropriate I guess?
		}
		if(0&&clip) //TESTING
		{
			if(BYTE l=SOM::clipper.clip(cq,h,r,15,0,cq))
			{
				lm|=l; clinging = true; ret = true;
			}
		}
		else if(DWORD l=som_MHM_4159A0(cq,h,r,15,p3))
		{
			lm|=l; cq[0] = p3[0]; cq[2] = p3[2];

			clinging = true; ret = true;
		}

		if(fell&&!passes) //standing astride a crack? (2+ platforms)
		{
			fell = false;

			//try to fall again with adjusted position

			float swap[3]; memcpy(swap,cp,sizeof(swap));

			for(int i=0,n=SOM::L.ai3_size;i<n;i++)	
			if(som_logic_40dff0(cp,h,r,i,1,p3,n3)) //climbing?
			{
				cp[1] = p3[1];
			}		
			if(som_MHM_4159A0(cp,h,r,14,p3)) //climbing?
			{
				cp[1] = p3[1]; 
			}
			assert(swap[0]==cp[0]&&swap[2]==cp[2]);

			if(cp[1]>swap[1])
			{
				cp[1] = max(cp[1],y);
			
				cp[0] = x; cp[2] = z; goto fell2;
			}
			else cp[1] = swap[1];
		}

		//REFACTOR ME
		//DUPLICATE som_state_4159A0
		//this makes the vertical edges of MHM seem as if they are
		//rounded off when moving around them. otherwise the clipper
		//can act up when switching from one face to the next making 
		//it noticeably kick out in the new direction
		if(clip&&som_logic_softclip)		
		if(clinging||clip.stem[passes]!=2) //2017: EXPERIMENTAL
		{ 
			float *cmp = passes?clip.soft:xyzuvw;

			//TODO? frame-rate might affect this
			//closer to 1 is smoother. not too smooth
			//(2020: the math looks correct but I've not tried
			//it this way since I'm unsure how to lock at 30fps
			//in windowed mode?)
			//const float power = pow(1.125f,fps);
			float power = 1.125f;
			//UNTESTED: try to accommodate 30fps?
			if(!EX::INI::Bugfix()->do_fix_animation_sample_rate)
			power*=power;

			float dx = cq[0]-cmp[0];
			float dz = cq[2]-cmp[2];
			float mag = sqrtf(dx*dx+dz*dz);
		
			//0.025 was based on a 0.5 radius
			//REMINDER: som.MHM.cpp pushes away according the 
			//radius and stem is always 2
			/*my sense is this should probably encompass the radius
			//(especially on the main pass)*/
			float lim = 0.025f*(0&&passes?clip.softolerance:sdr[2]);
		//	if(passes){ lim*=2; power = 1; }
			//static float stem = 2; //power of 2
			float &stem = clip.stem[passes]; assert(stem>=2);

			/*REMOVE ME?
			//HACK: extract som.mocap::speed
			float speeds[3]; EX::speedometer(speeds);			
			//the effect must be disabled at high speeds because it
			//sees through the wall, even if it never falls through
			//THIS MIGHT NOT WORK FOR INHUMAN SHAPES/WALKING SPEEDS
			float impact = speeds[0]/SOM::dash-SOM::walk/SOM::dash;
			impact = pow(1+max(0,impact),2);
			lim*=impact;*/
			
			if(clinging&&mag>lim*stem) //stem is at least 2
			{	
				float x = 1/mag*lim*stem;

				if(passes)
				{
					passes = passes; //breakpoint
				}
				
				cq[0] = cmp[0]+dx*x; //*1/mag*lim*stem;
				cq[2] = cmp[2]+dz*x; //*1/mag*lim*stem;				

				//increase the limit in case the violent movement is
				//appropriate: e.g. a head-on collision

				stem = pow(stem,power);
			}
			else stem = max(2,pow(stem,1/power)); 
		}
	}
	if(clip&&som_logic_softclip)	
	if(//!ret|| //glitches
	SOM::frame==mdl.ext.pc_clip&&!SOM::L.f4) //debugging
	{
		clip.soft2[0] = soft2[0];
		clip.soft2[2] = soft2[2];
	}
	else
	{
		//this looks fantastic except when it hangs
		//out around 0 for too long... usually when
		//NPCs push into the wall at oblique angles
		//float t = sqrt(1-sinf(clip.soft23*M_PI));
		//float t = 1;
		float t = sinf(clip.soft23*M_PI);		
		if(0) //2 looks good but is really too far
		{	
			t = sqrt(1-t);
			//2 looks good but is way too far
			//t = clip.reshape2*step*som_logic_soft_impedance; 
			t*=2;//som_logic_soft_impedance;			
			
		//	t = powf(t,1+clip.reshape2);
			t*=1+clip.reshape2*clip.reshape2;
		}			
		else //looks promising (still based on 2)
		{
			//NOTE: the goal here is to not bottom
			//out at 0 but smoothly round corners
			//with as high an output as possible
			//(4-t*2 translates to 2/1.4142)

			//maybe just because sqrt(2) less than 2
			//t = sqrt(2-t); //better
			t = sqrt(4-t*2); //tighter (best)
			//t = sqrt(5-t*3); //pushing it?

			//UNFINISHED
			//this is just a fudge factor to tighten
			//up so fast moving monsters don't clip
			//through corners so much... it distorts
			//everything and isn't scientific at all
			//
			// NOTE: when stopping reshape2 is being
			// artificially inflated to bring t to a
			// stop
			//
			t*=1+clip.reshape2*clip.reshape2;
		}
		t*=step;

		//0.1 is more than adequate to clip
		//Moratheia's fast moving monsters
		//(6*step at 60fps)
		//0.05 is indaequate??? 0.1 seems 
		//a little rough (SMALL WINDOW???)
		//Moratheia is over 0.2, under 0.3
		//t = max(0,min(0.1f,t));
		t = max(0,min(0.5f,t)); //1

		soft2[1] = clip.soft[1];
		float t2 = min(1,23*step+clip.falling);

		for(int i=0;i<=3;i++) //lerp?
		{
			float &x = clip.soft2[i], &y = soft2[i];
			x+=(y-x)*(i==1?t2:t);
		}
	}
	clip.cling = clinging;
	clip.clinging+=(float)(clinging-clip.clinging)*step*15;

	cp[1] = yy; //HACK
	//HACK? this remembers 5 frames to ensure
	//there isn't fighting
	//clip.layers = lm;
	if(clip.layers=lm=clip.layers>>6|lm<<24)
	{
		lm = (lm>>24|lm>>18|lm>>12|lm>>6|lm)&0x3f; //6 layers/bits

		som_MPX &mpx = *SOM::L.mpx->pointer;

		int i4,v10;
		#if 1 //EXTENSION
		#define SOM_LOGIC_E
		r+=clip.softolerance*0.5f; //DESTRUCTIVE
		//loop code taken from som_MHM_415450_inner 
		i4 = (int)r/2+1;		
		#endif
		int ex,ez; //REMOVE ME
		if(((BYTE(__cdecl*)(FLOAT,FLOAT,int*,int*))0x415bc0)(cp[0],cp[2],&ex,&ez))
		{
			#ifdef SOM_LOGIC_E
			const int l2c = max(0,ex-i4), v5 = min(99,ex+i4);
			const int l20 = max(0,ez-i4), v11 = min(99,ez+i4);
			const int b1 = l2c, e1 = v5+1;
			const int b2 = l20, e2 = v11+1;
			#else
			i4 = ex; v10 = ez;
			#endif

			//for(int li=0;li<=0;li++)
			for(int li=0;lm;li++,lm>>=1) if(lm&1)
			{
				auto &l = ((SOM::MPX::Layer*)&mpx[SOM::MPX::layer_0])[-li];
			
				#ifdef SOM_LOGIC_E
				for(i4=b2;i4!=e2;i4++) for(int v10=b1;v10!=e1;v10++)
				#endif
				{
					auto &tile = l.tiles[i4*100+v10];

					//NOTE: ffff wasn't required before SOM_LOGIC_E
					//som_MPX_411a20 is making a plug tile with the
					//e bit set to 1, which is blocking entry since
					//factoring in r
					if(tile.ev||0xFFFF==tile.mhm) continue;

					#ifdef SOM_LOGIC_E
					if(v10!=ex||i4!=ez)
					{
						if(v10<ex)
						{
							if(v10*2+1<cp[0]-r) continue;
						}
						else if(v10>ex)
						{
							if(v10*2-1>cp[0]+r) continue;
						}
						if(i4<ez)
						{
							if(i4*2+1<cp[2]-r) continue;
						}
						else if(i4>ez)
						{
							if(i4*2-1>cp[2]+r) continue;
						}
					}
					#endif
					
					clip.noentry = 1<<li; //debugging

					//EXTENSION
					//KF2's skeletons need to be able to fall from their
					//cages in the ceiling
					//return;
					xyzuvw[1] = cp[1]; return ret;
				}
			}
		}
	}
	clip.noentry = false; //debugging

	for(int i=3;i-->0;) xyzuvw[i] = cp[i]; return ret;
}
static bool som_logic_4079d0(som_Enemy &ai, float cp[3]) //__cdecl
{	
	//NOTE: som_logic_406ab0 already has all this 
	//(MAYBE PASS IT IN?)
	auto prm = SOM::L.enemy_prm_file[ai[SOM::AI::enemy]].uc;
	auto pr2 = SOM::L.enemy_pr2_data[*(WORD*)prm];
	auto prf = (swordofmoonlight_prf_enemy_t*)(pr2+62);
	if(prf->locomotion&1||prm[340]==3) return false; //immobilized?
		   	
	bool flying = 2==prf->locomotion;

	//NOTE: this was originally done differently
	float *xyzuvw = &ai[SOM::AI::xyzuvw];
	
	return //EXTENSION
	som_logic_4079d0_42a0c0(*(SOM::MDL*)ai[SOM::AI::mdl],cp,
	&ai[SOM::AI::shadow],ai[SOM::AI::height],xyzuvw,flying);
}
static void __cdecl som_logic_429410(DWORD _1) //2024: NPC state
{
	auto &ai = SOM::L.ai2[_1];
	auto &mdl = *(SOM::MDL*)ai[SOM::AI::mdl2];

	float *xyzuvw = &ai[SOM::AI::xyz2];

	if(0==*(DWORD*)&mdl.fade) //REMOVE ME
	{
	//	auto &st = ai[SOM::AI::ai_state2]; //? 
		auto &radius = ai[SOM::AI::radius2];

		//reset_clip();
		{
			mdl.ext.clip.stem[0] = 2;
			mdl.ext.clip.stem[1] = 2;
		
			memcpy(mdl.ext.clip.soft,xyzuvw,sizeof(float)*3);
			mdl.ext.clip.soft2[3] = 0;
			memcpy(mdl.ext.clip.soft2,xyzuvw,sizeof(float)*3);
			/*this gets stuck in the wall
			mdl.ext.clip.softolerance = min(0.5f,radius);*/
			mdl.ext.clip.softolerance = min(0.35f,radius*0.7f);
		}

		//TODO: st?
		/*randomize idle animations so KF2's slimes aren't moving
		//identical
		if(st!=5||-1==mdl.animation(9))
		{
			int len = mdl.running_time(mdl.c);
			if(len>0) mdl.d = SOM::rng()%len;
		}*/
	}

	//NOTE: last call to 42a0c0 is disabled to pick up ai+mdl
	((void(__cdecl*)(DWORD))0x429410)(_1); //42a0c0(&ai,cp)
	{	
		//2024: walk cp_accum even if not going to be drawn?
		mdl.update_animation();
				
		bool moving = false; //UNUSED (now)

		float cp[3];
	//	extern BYTE __cdecl som_MDL_4418b0(SOM::MDL&,float*,DWORD);
	//	som_MDL_4418b0(mdl,cp,~0);				
		memcpy(cp,mdl.ext.clip.cp_accum[0],sizeof(cp));
		if(cp[0]||cp[1]||cp[2])
		{
			if(1==ai.c[SOM::AI::standing_up2])
		//	if(0==ai.c[SOM::AI::standing_turning2])
			moving = true;

			//EXTENSION
			//if(st>=1&&st<=3&&mdl.ext.f2) //UNFINISHED			
			if(mdl.ext.anim_read_head2) //TODO: skeleton
			{
				if(1!=mdl.ext.anim_weights[0]) //debugging
				{
					float cp2[3];
					/*std::swap(mdl.ext.e2,mdl.c);
					std::swap(mdl.ext.f2,mdl.d);				
					som_MDL_4418b0(mdl,cp2,~0);
					std::swap(mdl.ext.e2,mdl.c);
					std::swap(mdl.ext.f2,mdl.d);*/
					memcpy(cp2,mdl.ext.clip.cp_accum[1],sizeof(cp));
					//this visibly shifts back/forth every frame
					//because it's overcorrecting
					//if(dir2<0)
					if(mdl.ext.inverse_lateral)
					cp2[0] = -cp2[0]; 
					for(int i=3;i-->0;)
					{
						cp[i]*=mdl.ext.anim_weights[0];
						cp[i]+=mdl.ext.anim_weights[1]*cp2[i];
					}
				}
				//if(sighted)
				//EX::dbgmsg("ai #%d: %f %f (%d)",_1,cp[0],cp[2],mdl.c);
			}
			memset(mdl.ext.clip.cp_accum,0x00,sizeof(mdl.ext.clip.cp_accum));

			SOM::rotate(cp,xyzuvw[3],xyzuvw[4]+1.570796f);
		
			for(int i=3;i-->0;) cp[i]*=ai[SOM::AI::scale2];

		//	if(moving&&(cp[0]||cp[1]||cp[2]))
		//	if(moving||(cp[0]||cp[1]||cp[2])) //2024: any kind?!
			{
			//	if(0)
			//	((void(__cdecl*)(void*,void*))0x42a0c0)(&ai,cp);
			//	else 
				int swap = SomEx_npc;
				SomEx_npc = 0x1000|_1;
				som_logic_4079d0_42a0c0(mdl,cp,
				&ai[SOM::AI::shadow2],ai[SOM::AI::height2],xyzuvw,false);
				SomEx_npc = swap;
			}
		}	
	}
}

//EXTENSION?
//0.5 is too jerky in transition both from slow gear to
//normal gear and to fast turning at 2x. this is 1.5 in
//fast turn and 0.66 slow gear (when unaware of the PC)
static const float som_logic_406ab0_fast_turn = 0.66666f; //0.5f
static void __cdecl som_logic_406ab0(DWORD _1, FLOAT _2)
{
	if(0) return ((void(__cdecl*)(DWORD,FLOAT))0x406ab0)(_1,_2);

	//NOTE: this is a full rewrite of 406ab0

	//various extensions use this, including sample_pitch_adjustment
	//indirectly (and volume)
	SomEx_npc = _1;

	auto &ai = SOM::L.ai[_1];
	auto &mdl = *(SOM::MDL*)ai[SOM::AI::mdl];
	auto prm = (BYTE*)SOM::L.enemy_prm_file[ai[SOM::AI::enemy]].c;
	auto pr2 = SOM::L.enemy_pr2_data[*(WORD*)prm];
	auto prf = (swordofmoonlight_prf_enemy_t*)(pr2+62);	

	//yes, mp? I don't know what else this could be?
	auto &hp = (WORD&)ai[SOM::AI::hp];
	auto &mp = (WORD&)ai[SOM::AI::mp];
	auto &max_hp = *(WORD*)(prm+296);
	auto &max_mp = *(WORD*)(prm+298);
	auto &st = ai[SOM::AI::ai_state]; 
	auto &radius = ai[SOM::AI::radius];

	float *xyzuvw = &ai[SOM::AI::xyzuvw];

	const float step = SOM::L.fade;

	//HACK: spawning?
	auto reset_clip = [&]() //REMOVE ME
	{
		mdl.ext.clip.stem[0] = 2;
		mdl.ext.clip.stem[1] = 2;
		memcpy(mdl.ext.clip.soft,xyzuvw,sizeof(float)*3);
		mdl.ext.clip.soft2[3] = 0;
		memcpy(mdl.ext.clip.soft2,xyzuvw,sizeof(float)*3);
		/*this gets stuck in the wall
		mdl.ext.clip.softolerance = min(0.5f,radius);*/
		mdl.ext.clip.softolerance = min(0.35f,radius*0.7f);
	};
	if(0==*(DWORD*)&mdl.fade) //REMOVE ME
	{
		reset_clip();

		//white ghost?
		if(auto*d=mdl->ext.mdo)				
		for(int i=d->material_count;i-->0;)
		{
			float *m7 = SOM::L.materials[mdl.ext.mdo_materials[i]].f+1;
			m7[12+3] = 0.0f;
		}

		//randomize idle animations so KF2's slimes aren't moving
		//identical
		if(st!=5||-1==mdl.animation(9))
		{
			int len = mdl.running_time(mdl.c);
			if(len>0) mdl.d = SOM::rng()%len;
		}
	}

	//fading in?
	mdl.fade = min(1,mdl.fade+SOM::L.fade);

	//what's this?
	if(*(WORD*)(pr2+0x8c)!=0xFFFF)
	{
		if(ai.c[592]) ai.c[592]--;
	}

	//NOTE: fog may be much closer
	//(may want to fade them out?)
	float zfar = SOM::L.frustum[1]; if(_2>zfar) //out of range?
	{
		//FIX ME
		//I feel like this should be unspawning?!
		int todolist[SOMEX_VNUMBER<=0x1020602UL];

		//"retroactive" mode?
		float *sp = &ai[SOM::AI::_xyzuvw];				
		if(prm[340]==2&&16!=st
		&&zfar<som_game_dist2(SOM::xyz[0],SOM::xyz[2],sp[0],sp[2]))
		{		
			memcpy(xyzuvw,sp,sizeof(float)*6);

			st = 5; //trigger (#9)

			ai.c[0x248] = 1; //UNION

			mdl.xyzuvw[0] = xyzuvw[0];
			mdl.xyzuvw[1] = xyzuvw[1]+0.01f;
			mdl.xyzuvw[2] = xyzuvw[2];
			mdl.xyzuvw[4] = xyzuvw[4]+1.57079637f;
			mdl.fade = 1; //0x3f800000;

			int id = SOM::L.animation_id_table[st];
			id = mdl.animation(id);
			//FUN_004412e0_set_current_animation(&mdl,id);
			((BYTE(__cdecl*)(void*,DWORD))0x4412E0)(&mdl,id);
		}
		if(prm[328]==1) //SOM_PRM setting?
		{
			//yes, mp? I don't know what else this could be?
			hp = max_hp; mp = max_mp;
		}

		reset_clip(); return;
	}
	
	//NOTE: 0x220 and 0x224 (see below)
	//are set at the same time as 0x21c
 	auto &dmg = ai.i[0x21c/4];
	if(dmg&&st<=13||dmg>=hp&&st!=16)
	{
		int id = 0; //evade?

		auto dest = (som_Enemy*)ai.i[0x220];

		if((void*)dest==SOM::L.pcstore) //PC source? 
		{
			bool inc = !ai.c[0x224];
			//FUN_00427b70_level_up_ep_str_mag(0,inc,!inc);
			//FUN_00427d70_vamp_hp_and_mp(dmg);
			((void(__cdecl*)(DWORD,BYTE,BYTE))0x427b70)(0,inc,!inc);
			((void(__cdecl*)(DWORD))0x427d70)(dmg);

			assert(SomEx_pc==12288);
		}
		
		if(dmg<hp) //hit? 
		{	
			//EXTENSION
			EX::INI::Damage hq; //hp;
			//note: designers may want this to be a downward
			//spiral by basing it on current HP instead of max
			int crit; if(&hq->critical_hit_point_quantifier)
			{
				if((void*)dest!=SOM::L.pcstore)
				{
					size_t i = dest-SOM::L.ai;
					if(i>=SOM::L.ai_size) //trap?
					{
						//I'm not 100% sure this is possible
						//but I think so (I need to test it)
						i = (som_Obj*)dest-SOM::L.ai3;
						if(i<512) SomEx_pc = 2<<12|i;
						assert(i<512);
					}			
					else SomEx_pc = i;
				}
				else assert(SomEx_pc==12288);

				//SomEx_npc = _1;
				{
					//DUPLICATES SOM::Red (arguments?)
					crit = hq->critical_hit_point_quantifier();
				}
				SomEx_pc = 12288; //SomEx_npc = -1;
			}
			else //crit = max_hp/5;
			{				
				//the original value was 1/5th but nobody knew
				//this feature existed and I feel like 1/5th is
				//very low, and I want to sync it up with do_red

				//HACK: o is 1/3 so other uses are consistent
				crit = max_hp*hq->critical_hit_point_quantifier.o();
			}

			//NEW FORMULA
			//if(dmg*5<max_hp) //non-critical?
			if(dmg<crit)
			{
				//NOTE: 441490 does some defensive programming tests
				//unsigned d = FUN_00441490_get_animation_goal_frame(mdl);
				unsigned d = mdl.d;

				if(st!=0xc //defend?
				||d<prf->defend_window[0]||prf->defend_window[1]<d)
				{
					//EXTENSION?
					//don't short-circuit attacks in progress with anything
					//less than a critical hit
					if(st<6||st>11||!EX::INI::Option()->do_red)
					{
						id = 20;
						st = 14; //hit (still set?)
					}
				}
			}
			else //critical? //cancels evade?
			{
				id = -1==mdl.animation(22)?20:22;
				st = id==22?15:14;
			}

			hp-=dmg;

			//in view or aware?
			//NOTE: som_state_404470 is doing this when
			//a hit animation isn't triggered. probably
			//its logic belongs here (it is much older)
			ai[SOM::AI::ai_bits]|=4; 
		}
		else //death
		{
			hp = 0; if(ai.c[0x24]) //decrement spawn count?
			{
				if(ai.c[0x34]) ai.c[0x34]-=1; else assert(0);
			}										
			id = 4; st = 16;
		}					
		if(id)
		{
			//FUN_00407950_set_animation_indirect?_for_enemy?(&ai,id,1);
			((void(__cdecl*)(void*,DWORD,BYTE))0x407950)(&ai,id,true);

			assert(mdl.e!=mdl.c); //bug :(
		}

		ai.c[0x225] = 0; //?
	}
	dmg = 0;
	
	float cp[3] = {}; if(mdl._mystery_mode[0]) //?
	{
		/*assert(0); //interpolation system???

		//FUN_00441510_advance_MDL(&mdl)
		((BYTE(__cdecl*)(void*))0x441510)(&mdl); 
		
		if(*mdl._mystery_mode) 
		{
			//FUN_004416c0_update_MDL_xform_and_CPs(&mdl);
			som_MDL_4416c0(mdl);
			//FUN_00407ff0_step_enemy_flame?(&ai);
			//FUN_004079d0_clip_enemy_and_checkpoint(&ai,cp);
			((void(__cdecl*)(void*))0x407ff0)(&ai);
			assert(0); //som_logic_4079d0?
			((void(__cdecl*)(void*,FLOAT[3]))0x4079d0)(&ai,cp);
			return;
		}
		else //error?
		{
			//FUN_004414c0_set_animation_goal_frame(&mdl,0);
			((BYTE(__cdecl*)(void*,DWORD))0x4414c0)(&mdl,0);
		}
		*/
		assert(!mdl->cp_file); //som_MDL_4418b0?
	}

	//activation radius?
	if(st==5&&ai.c[0x248]) //UNION
	{
		//TODO: add player diameter?
		auto lim = ai.f[0x238/4]+0.5f; //SOM::L.half

		if(_2>max(lim,*(float*)(prm+336)))
		{
			//FUN_00407ff0_step_enemy_flame?(&ai);
			((void(__cdecl*)(void*))0x407ff0)(&ai);
			return;
		}
		ai.c[0x248] = 0;
	}

	//2020: moving up so "wait" can be factored into
	//viewing/sighted
	bool eval = false; 
	bool live = st!=5||!ai.c[0x248]; //UNION		
	//2021: this was done last, after advancing the
	//animation, which is a problem because it means
	//the first valid time is 2, which is too much of
	//a "gotcha" or quirk in the PRF editor
	((void(__cdecl*)(void*))0x407f30)(&ai); //SND
	//FUN_00441510_advance_MDL(&mdl)	
	bool wait = live?((BYTE(__cdecl*)(void*))0x441510)(&mdl)!=0:false;

	
		const int mdl_cc = mdl.c; //TESTING
		//const int mdl_ee = mdl.e; //TESTING


	bool seen = (ai[SOM::AI::ai_bits]&4)!=0;
	bool viewing = false;
	bool sighted = false;

	//angle in direction of PC (pcstate?)
	auto dir = atan2(SOM::xyz[2]-xyzuvw[2],SOM::xyz[0]-xyzuvw[0]);	
	float dir2 = ((FLOAT(__cdecl*)(FLOAT))0x44cc20)(dir-xyzuvw[4]);

	//EXTENSION
	//wait until (fast) turn completes
	//2021: I'm thinking to reevaluate this only once per idle cycle
	//so that SOM_PRM's spotting percentage is more meaningful since
	//it's how attack chances work too
	//(I have more rng thoughts below)
	if(!wait/*||st!=3*/)
	{
		//SHOULD VISIBILITY SCALE????
		auto len = *(float*)(prm+312); 
		
			len*=ai[SOM::AI::_scale]; //NEW?
		
		if(_2<=len) //view cone?
		{
			//TODO: add player diameter? 
			//SOM::L.half*M_PI/180 (458318)
			float arc = *(WORD*)(prm+310)*0.008726645f; //pi/180/2

			//NOTE: result is returned on st0
			//float cmp = FUN_0044cc20_fmod_pi?(dir-xyzuvw[4]);
			//float cmp = ((FLOAT(__cdecl*)(FLOAT))0x44cc20)(dir-xyzuvw[4]);				
			//viewing = cmp>-arc&&cmp<arc;
			viewing = dir2>-arc&&dir2<arc;
		}
		if(!seen) //or aggrevated? Japanese is recognition
		{
			//32767/328=~100 ... I think %100 works just
			//as well in theory... but I wonder because
			//this is at 60 frames-per-second, so is it
			//really meaningful???
			//(I've tried to address this above by doing
			//this at the top of the animation cycle. it
			//might be an idea to randomize which frame
			//reevaluates if players learn how it works)
			sighted = viewing&&SOM::rng()/328<=prm[316];
		}
		else sighted = _2<=len;

		ai[SOM::AI::ai_bits]&=~(4|8);
		if(sighted) 
		ai[SOM::AI::ai_bits]|=4;
		if(viewing) 
		ai[SOM::AI::ai_bits]|=8;
	}
	else //EXTENSION
	{
		sighted = seen;
		viewing = (ai[SOM::AI::ai_bits]&8)!=0;
	}	
	
	//I think 408210 sets 0x255?
	if(ai.c[0x225])
	if(!som_game_interframe) //every other frame at 60fps?
	if(!--ai.c[0x226]) //evade or defend?
	{
		ai.c[0x225] = 0; //bool (408210?)
		
		//TODO: SFX should trigger too???

		//monsters respond from any/all distance/directions???

		//EXTENSION
		//ignore if unaware of PC (assuming monster doesn't
		//possess any superhuman awareneess)
		//FURTHER restricting it to "viewing" since the animation
		//is probably not omni-directional (but can be if the 
		//monster has 360 vision
		bool vis = viewing&&(sighted||seen); //direction?

		if(vis) //layer? //EXTENSION 
		{
			//DUPLICATE attack/reevalute below
			//don't want sound effects playing from other
			//level
			float y = xyzuvw[1], h = ai[SOM::AI::height];
			if(y>SOM::xyz[1]+SOM::L.duck||y+h<SOM::xyz[1])
			vis = false;
		}
		if(vis) //radius+1? //EXTENSION
		{	
			float r; if(auto*mv=SOM::movesets[0][0]) 
			{
				r = mv->f[20];
			}
			else //YUCK
			{
				int id = SOM::L.pcequip[0];
				id = id>=250?-1:SOM::L.item_prm_file[id].s[0];
				r = id<0?0:SOM::L.item_pr2_file[id].f[20];
			}
			if(_2-radius>r+SOM::L.hitbox2+1)
			vis = false;
		}
		if(vis) //looking away? //EXTENSION
		{
			//TODO: how about a dot product?
			auto cmp = atan2(xyzuvw[2]-SOM::xyz[2],xyzuvw[0]-SOM::xyz[0]);
			cmp = ((FLOAT(__cdecl*)(FLOAT))0x44cc20)(cmp-(SOM::uvw[1]+M_PI_2));
			vis = cmp>-M_PI_2&&cmp<M_PI_2; 
		}

		if(vis) //EXTENSION
		{	
			bool roll = SOM::rng()%100<prm[325];
				
			int id = 0; if(st!=0xd&&1==prm[324]) //evade?
			{
				if(roll){ st = 0xd; id = 7; }
			}
			else if(st!=0xc&&2==prm[324]) //defend
			{
				if(roll){ st = 0xc; id = 6; }
			}
			else assert(0); if(id)
			{		
				//FUN_00407950_set_animation_indirect?_for_enemy?(&ai,id,0);
				((void(__cdecl*)(void*,DWORD,BYTE))0x407950)(&ai,id,false);
				return;
			}
		}
	}

	//MOVED ABOVE
	//FUN_00441510_advance_MDL(&mdl)
	//bool wait = live?((BYTE(__cdecl*)(void*))0x441510)(&mdl)!=0:false;

	if(live) switch(st)
	{
	case 0: //idle countdown? (random 1, 2, or 3 cycle wait)
	{
		if(wait) break;

		if(0<--ai.i[0x248/4]) //UNION
		{
			//FUN_004414c0_set_animation_goal_frame(&mdl,1);
			((BYTE(__cdecl*)(void*,DWORD))0x4414c0)(&mdl,1);
			
			break;
		}
		else eval = true; break;
	}
	case 1: //walking
	case 2: //2 seems to be walking when inside vision cone? (407882)

		if(wait)
		{ 			
			eval = _2<=ai[SOM::AI::shadow]+0.25f+0.1f;

			break;
		}
		else eval = true; break;	

	case 3: //walking/turning?
	{	
		if(!wait) //error?
		{
			//FUN_004414c0_set_animation_goal_frame(&mdl,1);
			((BYTE(__cdecl*)(void*,DWORD))0x4414c0)(&mdl,1);
		}

		//EXTENION
		//fast-turn so that monster have a chance of catching the PC behind them
		float x = sighted&&!viewing?som_logic_406ab0_fast_turn:1;

		if(float sp=mdl.ext.speed) x/=sp;

		//NOTE: 407490 sets these values in general-purpose memory region at end
		//cVar8 = FUN_0044cd10_inc_npc_aim_and?(xyzuvw+4,ai.f[0x24c/4],ai.f[0x248/4]); //UNION
		if(((BYTE(__cdecl*)(FLOAT*,FLOAT,FLOAT))0x44cd10)(xyzuvw+4,ai.f[0x24c/4],ai.f[0x248/4]/x)) //UNION
		{
			eval = true;
		}
		break;
	}
	case 4: //#8? (does anything assign 4?)
	case 5: //trigger? (#9)
	case 12: //defend? (#6)
	case 13: //evade? (#7)
		
		if(ai.i[0x248/4]) break;

		eval = !wait; break;

	case 6:
	case 7:
	case 8: //attacking (directly)
					
		if(!wait)
		{
			//I suspect this is to keep monsters from constantly attacking 
			if(!ai.c[0x227]) 
			{
				eval = true;
			}
			else if(!som_game_interframe) //every other frame at 60fps?
			{
				ai.c[0x227]--;
			}
		}
		else 
		{
			auto atk = st-6;

			//int d = mdl.d;
			float d = mdl.f+mdl.ext.s;
			
			for(int i=3;i-->0;)
			{
				float cmp = prf->hit_delay[atk][i]; //pr2[st+0x7a+atk*2+i]

				if(cmp) if(d>=cmp&&d<cmp+mdl.ext.speed)
				{
					//eval = false; //initialized to false?

					union
					{
						SOM::Attack att; //debug
						DWORD tmp[0x11]; //17
					};
					memcpy(tmp,ai.c+116+atk*0x44,0x44);
					tmp[5] = (DWORD)&ai;
					tmp[8] = ai.i[18]; //x
					tmp[9] = ai.i[19]; //y
					tmp[10] = ai.i[20]; //z
					tmp[13] = ai.i[22]; //v
					//som_MPX_405de0 precomputes this
				//	att.radius*=ai[SOM::AI::scale]/ai[SOM::AI::_scale]; //2023
					//FUN_004041d0_add_damage_source?(tmp);
					((void(__cdecl*)(void*))0x4041d0)(tmp);

					break;
				}
			}
		}
		break;
	
	case 9:
	case 10:
	case 11: //attacking (indirectly)
		
		eval = !wait;

		//EXTENSION (REQUIRING turning_table SETUP)
		/*turning mid attack? (som.game.cpp HAD disabled this)
		//if(prm[201+st*0x18]) //201?
		if(prm[417+(st-9)*24]) //201?
		{
			//FUN_0044cd10_inc_npc_aim_and?(xyzuvw+4,dir,ai.f[0x60/4]);
			((BYTE(__cdecl*)(FLOAT*,FLOAT,FLOAT))0x44cd10)(xyzuvw+4,dir,ai.f[24]);
		}*/
		break;

	case 14:
	case 15: //hit and 22? (critical-hit?)
		
		eval = !wait; break;

	case 16: // Death ///

		if(!wait) ai[SOM::AI::stage] = 4; break;
	}
	//EXTENSION	
	int tt = prf->turning_table;
	float turn_rate = ai[SOM::AI::turning_rate];
	if(tt&1<<SOM::L.animation_id_table[st])
	{
		if(float sp=mdl.ext.speed) turn_rate/=sp;

		//EXTENSION
		//TODO? what direction to turn if unsighted?
		if(sighted&&st!=3)	
		((BYTE(__cdecl*)(FLOAT*,FLOAT,FLOAT))0x44cd10)(xyzuvw+4,dir,turn_rate);
		
		if(*prf->inclination) //EXTENSION
		if(prf->locomotion&2) //assume aerial for time being
		{
			float fe = prf->flight_envelope;

			//TODO: turn on prf->flight_envelope
			float dir2 = 0;
			if(!sighted) turn_rate*=0.5f;
			else dir2 = -atan2(SOM::cam[1]-xyzuvw[1],_2);

			EX::INI::Adjust ad; if(dir2&&&ad->npc_xz_plane_distance)
			{
				//SomEx_npc = _1;
				float d = *ad->npc_xz_plane_distance(); if(!EX::isNaN(d))
				{
					//HACK: this includes -0.0
					bool fish = EX::isNeg(fe); //fish or fowl?

					if(fish?xyzuvw[1]>=d:xyzuvw[1]<=d) 
					{
						if(fish?dir2<0:dir2>0) dir2 = 0;
					}
				}			
				//SomEx_npc = -1;
			}

			((BYTE(__cdecl*)(FLOAT*,FLOAT,FLOAT))0x44cd10)(xyzuvw+3,dir2,turn_rate);
		}
	}
	
	/*emails with Ben suggest this can foil Evade/Defend
	//16 is maximum (confusing???)
	//I think just 0-4,12,13 (4 is #8?) 12/13 are evade/defend (#6,#7)
	////if(st!=5&&st!=16&&st!=15&&st!=14&&(st<6||11<st)) 
	//if(st!=5&&st<14&&(st<6||11<st)) 
	if(st<5||st==12||st==13)*/
	if(st<5)
	{
		if(seen!=sighted) 
		{
			eval = true;
		}
		
		//DISABLED (FOR NOW)
		//TODO? what about indirect attacks?
		/*HACK: som_MPX_405de0 sets AI::diameter equal to the maximum
		attack range, which is better but separates the value from the
		shadow radius (AI::diameter is unused)
		//note: 0.25f+0.1f is [4582cc]+[458328]
		//if(_2<=ai[SOM::AI::shadow]+0.25f+0.1f) //shadow diameter???*/
		//				
		//seems this is what makes monsters twitch at close range
		//(maybe only if their walk animation doesn't move?)
		//why?
		//but with a larger radius it makes them constantly attack
		//(could 407490 prevent this?)
			/*if(0) 
		if(_2<=ai[SOM::AI::diameter]) //needs scaling!
		{
			eval = true;
		}*/
			/*2021: sometimes monsters seem confused at very close 
			//range but this doesn't seem to help
			if(!eval&&!wait&&!prm[317] //hunter? 
			&&_2-SOM::L.hitbox2<=ai[SOM::AI::perimeter] //scale?
			&&ai[SOM::AI::perimeter]!=FLT_MAX)
			{
				eval = true; assert(0); //needs scaling
			}*/
			if(mdl.ext.pc_clip==SOM::frame) //2021
			{
				ai[SOM::AI::ai_bits]|=4; //become aware of the PC?
			}
	}

	//EXTENSION
	//advance/turn at same time King's Field II style?
	//NOTE: prm[340] is "immobilized", prm[317] is hunter/sniper AI model 
	bool hunting = st!=3||!sighted||!prm[317]||_2>=ai[SOM::AI::perimeter];

	//2021: let tt&2 control if required to stop moving to turn
	//note, this is how it originally worked, however there are 
	//important reasons the "sniper" mode needs to stop to turn
	//that are factored into hunting
	if(st==3&&~tt&2) hunting = false;

	//2024: walk cp_accum even if not going to be drawn?
	mdl.update_animation();

	//som_logic_reprogram removes st!=3
	//if(st!=3&&pr2[0x60]&0x30)!=0x10)
	if(hunting&&~prf->locomotion&1) //could just omit CP?
	{
		//paused after attack?
		//
		// REMINDER: no animation plays during this
		// time (it couldn't hurt to improve that!)
		//
		if(st<6||st>11 //note: == should be fine
		||prm[318]<=(BYTE)ai[SOM::AI::attack_recovery])
		{
			//FUN_004418b0_get_cyan_CP_delta(&mdl,cp,~0);
		//	extern BYTE __cdecl som_MDL_4418b0(SOM::MDL&,float*,DWORD);
		//	som_MDL_4418b0(mdl,cp,~0);				
			memcpy(cp,mdl.ext.clip.cp_accum[0],sizeof(cp));
			//EXTENSION
			//if(st>=1&&st<=3&&mdl.ext.f2) //UNFINISHED			
			if(mdl.ext.anim_read_head2) //TODO: skeleton
			{
				if(1!=mdl.ext.anim_weights[0]) //debugging
				{
					float cp2[3];
					/*std::swap(mdl.ext.e2,mdl.c);
					std::swap(mdl.ext.f2,mdl.d);				
					som_MDL_4418b0(mdl,cp2,~0);
					std::swap(mdl.ext.e2,mdl.c);
					std::swap(mdl.ext.f2,mdl.d);*/
					memcpy(cp2,mdl.ext.clip.cp_accum[1],sizeof(cp));
					//this visibly shifts back/forth every frame
					//because it's overcorrecting
					//if(dir2<0)
					if(mdl.ext.inverse_lateral)
					cp2[0] = -cp2[0]; 
					for(int i=3;i-->0;)
					{
						cp[i]*=mdl.ext.anim_weights[0];
						cp[i]+=mdl.ext.anim_weights[1]*cp2[i];
					}
				}
				//if(sighted)
				//EX::dbgmsg("ai #%d: %f %f (%d)",_1,cp[0],cp[2],mdl.c);
			}
			memset(mdl.ext.clip.cp_accum,0x00,sizeof(mdl.ext.clip.cp_accum));

			SOM::rotate(cp,xyzuvw[3],xyzuvw[4]+1.570796f);
		
			for(int i=3;i-->0;) cp[i]*=ai[SOM::AI::scale];
		}
	}

	auto &clip = mdl.ext.clip; //EXPERIMENTAL

	bool no_clip = 0&&EX::debug; //TESTING

	//TESTING (INCONCLUSIVE)
	//WHY WAS THIS DONE AFTER? IS THIS THE VIOLENT KICK BUG???
	//NOTE: KICK BUG EXISTS IN EMU MODE
	bool before_or_after = 0||SOM::emu; //1
	if(!before_or_after) before_or_after:
	{
		//TODO: fall while dying (even if flying)
		//FUN_004079d0_clip_enemy_and_checkpoint(&ai,cp);
		if(!SOM::emu&&!no_clip)
		{
			//FIX ME: som_logic_4079d0 modifies cp!!
			float d = Somvector::map(cp).length<3>();
			//clip.velocity = d/step; //DEBUGGING

			//SPITBALLING
			bool soft;			
			float x = clip.soft[0]+=cp[0];
			float y = clip.soft[1]+=cp[1];
			float z = clip.soft[2]+=cp[2];
			{
				soft = som_logic_4079d0(ai,cp);
			}			
			float dx = xyzuvw[0]-x;
			float dy = xyzuvw[1]-y; 
			float dz = xyzuvw[2]-z;
			float ll = 1;			
			
				ll = sqrtf(dx*dx+dz*dz);
				if(!ll) ll = 0.00001f; //zero divide
				clip.soft2[3] = min(1,ll/clip.softolerance);
				int rate = 15; //d?30:15;
				clip.soft23+=(clip.soft2[3]-clip.soft23)*step*d;

			clip.reshape2*=1-step; //SKETCHY				
			if(som_logic_softclip) 
			if(!soft||
			SOM::frame==mdl.ext.pc_clip&&!SOM::L.f4)
			{
				//TODO: pushing with F4 is currently out-of-order

				ll = max(0.05f,1-clip.soft2[3]); //HACK: snap to?
				ll = sqrtf(ll);

				//HACK: try to bring soft2 to halt
				//so won't slide into idle animation
				//TODO: should really be exponential
				//because of 1-step above this has the
				//interesting property of converging on
				//step*x				
				clip.reshape2+=step*4;
				if(clip.reshape2<4) //finite?
				clip.reshape2+=step*clip.reshape2;
				clip.stopped = true;
			}
			else //if(1)
			{
				if(clip.stopped) //UNUSED
				{
					clip.stopped = false;
				//	clip.reshape2 = 0; //discontinuous
				}

				//m/s times 10 for 0.1 units per integer
				int m_s_10 = (int)(10*d/step);
				float s = som_logic_soft[min(99,m_s_10)];
				//som_mocap_am_reshape?
				s/=clip.softolerance; 
				float ss = clip.reshape;
				if(1) s = ss+(s-ss)*step*7; //lerp
				clip.reshape = s;
				//clip.reshape2*=1-step; //SKETCHY
				clip.reshape2 = max(clip.reshape2,s);

				//WORK IN PROGRESS
				//if correct this should match
				//som_mocap_am_calibrate at 1.7
				//walking speed for a 0.5 radius
				//(which is 0.571012914f) I don't
				//know if it would help to tailor
				//it to each monster but maybe it
				//should be the default 1.5 or it
				//should match the PC walk speed?

				//REFACTOR
				//see som_logic_4079d0 DUPLICATE!

				//WARNING: 8 IS error_impedance
				//IF IT NEEDS TO CHANGE THEN IT
				//NEEDS TO MATCH som_logic_soft
				//som_mocap_am_relax?
				float relax = step*som_logic_soft_impedance; //8 
				//reshape?
				//this is the value for the PC in
				//KF2 at 1.7 walk speed
				//TODO: faster NPCs need this to
				//be somewhat higher
				//relax*=0.571012914f;
				relax*=clip.reshape;
				//lasso_length? //it cancels out
				//relax*=ll;					
				ll = relax;
			}
			ll = max(0,min(1,ll));
			clip.soft[0]+=dx*ll;
			clip.soft[2]+=dz*ll;
			if(clip.npcstep?dy>=0:dy>0)
			{
				if(clip.npcstep)
				{
					//NO CLUE HERE BUT 
					//if this isn't fast enough the monster
					//needs to be slowed down
					//NOTE: relying on soft2 to slow down
					//initially
					/*step is built into d
					y = (1.5f+d*2)*step;*/
					y = (step+d);
					y = min(y,clip.npcstep2);
					if((clip.npcstep2-=y)<=0)
					{
						clip.npcstep = 0;
					}
				}
				else y = dy*step*5;
	
				clip.soft[1]+=y; 
			}
			else //do_g?
			{
				clip.soft[1] = xyzuvw[1];
				
				if(dy&&clip.npcstep)
				{
					clip.npcstep = 0; //breakpoing
				}
			}
		}
		else ((void(__cdecl*)(void*,FLOAT[3]))0x4079d0)(&ai,cp);

		if(before_or_after) goto before_or_after2;
	}

	if(no_clip||SOM::emu)
	{
		memcpy(mdl.xyzuvw,xyzuvw,3*sizeof(float));
		mdl.xyzuvw[1]+=0.01f;
	}
	else if((0?SOM::frame&1:0)&&EX::debug||!clip)
	{
		bool odd = 1||!EX::debug?true:SOM::frame&1;

		mdl.xyzuvw[0] = odd&&clip?clip.soft[0]:xyzuvw[0];
		mdl.xyzuvw[1] = (clip?clip.soft:xyzuvw)[1]+0.01f;
		mdl.xyzuvw[2] = odd&&clip?clip.soft[2]:xyzuvw[2];	
	}
	else 
	{
		//clip.soft is perfect (should be perfect) however
		//the NPC melts into the walls, deeper depending on
		//how far their CP moves. soft2 is then a second ring
		//that's pushed away from the wall

		//REMINDER: soft2 also mitigates bumps caused by
		//archways that are hard to smooth out otherwise
		//"soft" should absorb their shock too in theory
		//but it's not good at it and that's not its job

		mdl.xyzuvw[0] = clip.soft2[0];
		mdl.xyzuvw[2] = clip.soft2[2];
		mdl.xyzuvw[1] = clip.soft2[1]+0.01f;
	}
	if(tt&8) //EXPERIMENTAL slime mode?
	mdl.xyzuvw[4] = xyzuvw[4]+1.57079637f;
	if(auto&u=mdl.xyzuvw[3]=xyzuvw[3]) //EXTENSION
	if(prf->inclination[1]) //EXTENSION
	{
		float ci = min(prf->inclination[0],prf->inclination[1]);
		ci*=M_PI/180;
		u = u<0?min(0,u+ci):max(0,u-ci);
	}
	//FUN_004416c0_update_MDL_xform_and_CPs(&mdl);
	som_MDL_4416c0(mdl);
	//FUN_00407d50_step_enemy_state_sub_SFX?(&ai);
	//FUN_00407f30_play_enemy_sound_effects(&ai);
	//FUN_00407ff0_step_enemy_flame?(&ai);
	((void(__cdecl*)(void*))0x407d50)(&ai);	
	//((void(__cdecl*)(void*))0x407f30)(&ai); //moving above
	((void(__cdecl*)(void*))0x407ff0)(&ai);
	if(eval)	
	{	
		WORD &atks = *(WORD*)&prf->direct; //indirect too
		WORD swap = atks;
		{
			//DUPLICATE evade/defend above
			//2021: this is hack to prevent monsters on different
			//layers from attacking... this should be done inside
			//407490
			float y = xyzuvw[1], h = ai[SOM::AI::height];
			if(y>SOM::xyz[1]+SOM::L.duck||y+h<SOM::xyz[1])
			atks = 0;
			//FUN_00407490_step_enemy_state_sub_reevaluate(&ai);
			((void(__cdecl*)(void*))0x407490)(&ai);
		}
		atks = swap;

		//counter-attack?
		//NOTE: radius being larger than the hitbox requires
		//players close the distance
		if(st>=6&&st<=9) 
		if(2&SOM::motions.swing_move)
		if(auto*mv=SOM::shield_or_glove(0))
		if(_2-radius<mv->f[20]+SOM::L.hitbox2) 
		{
			auto cmp = atan2(xyzuvw[2]-SOM::xyz[2],xyzuvw[0]-SOM::xyz[0]);
			cmp = ((FLOAT(__cdecl*)(FLOAT))0x44cc20)(cmp-(SOM::uvw[1]+M_PI_2));

			//FIX ME? THIS MAY BE VERY NARROW
			//BECAUSE IT'S ALSO THE HIT RADIUS
			//float arc = mv->s[39]*0.008726645f; //pi/180/2
			//arc*=1.5f; //FUDGING
			//float arc = mv->s[39]*0.01745329; //pi/180
			float arc = M_PI_4;

			if(cmp>-arc&&cmp<arc) //attack.pie?
			{
				extern void som_mocap_counter_attack();
				som_mocap_counter_attack();
			}
		}

		mdl.ext.inverse_lateral = dir2<0;
	}
	//if(eval||(sst>=1&&sst<=3)!=(st>=1&&st<=3)) //EXTENSION
	if(mdl.c!=mdl_cc||!(st>=1&&st<=3)||!mdl.ext.f2)
	{
		mdl.ext.f2 = 0; //TESTING		

		WORD *lat = 0; if(st>=1&&st<=3) 
		{
			if(mdl.ext.anim_mask&1<<3)
			{				
				//HACK: force animation to restart when monster
				//reappears on screen so these will be in sync
				mdl.e = !mdl.c;
				mdl.f = 1;

				mdl.ext.c2 = mdl.animation(3);
				mdl.ext.f2 = !0;

				if(4&*mdl->file_head)
				lat = mdl->soft_anim_buf[mdl.ext.c2].file_ptr2;
				
				mdl.ext.anim_weights[0] = 1;
				mdl.ext.anim_weights[1] = 0;
			}
		}
		mdl.ext.anim_read_head2 = lat;		
		mdl.ext.anim_read_tick2 = 0;
	}
	if(mdl.ext.f2)
	{
		//assert(mdl.ext.f2==mdl.f);

		float *w = mdl.ext.anim_weights;

		if(sighted) //&&viewing)
		{
			float wt = fabsf(dir2/(float)M_PI_2);

			wt = min(1,wt);

			//in the current system if you don't bias
			//it the monster will turn so fast as to
			//never let the lateral movement be enough
			//to be noticed
			if(~prf->locomotion&1) //HACK?
			{
				//e is sqrt at half turning speed
				float e = 1-turn_rate/(float)M_PI/SOM::L.rate;
			//	if(float sp=mdl.ext.speed) e*=sp;
				wt = powf(wt,max(e,0.5f));
			}

			if(0&&EX::debug)
			{
				w[0] = 1;
				w[1] = 0;
			}
			else
			{
				w[0] = 1-wt;
				w[1] = wt;
			}

			//REMINDER: swapping 0/1 is interesting to
			//make the monster stay in front of the PC
			//as if blocking their path
			if(0) std::swap(w[0],w[1]);				
		}
		else if(1)
		{
			w[0] = 1;
			w[1] = 0;
		}
		else if(w[0]!=1) //assuming biased movement model
		{
			int len = mdl.running_time(mdl.c);
			float step = 1.0f/len;
			w[0] = min(1,w[0]+step);
			w[1] = max(0,w[1]-step);
		}

		//if(sighted)		
		//EX::dbgmsg("ai #%d: %f %f (%f)",_1,w[1],w[0],dir2);		
	}

	//2021: it looks like this might be the reason for the
	//violent kick bug coming around corners... it's simply
	//done after the model transform is set up???
	////FUN_004079d0_clip_enemy_and_checkpoint(&ai,cp);
	if(before_or_after) goto before_or_after;

	before_or_after2: SomEx_npc = -1;
}

static int __cdecl som_logic_4413a0_talk(SOM::MDL *m, int id)
{
	int i = m->animation(id); if(i!=-1) return i; 

	switch(id)
	{
	case 4: case 5: i = 6; break;

	case 20: case 21: i = 22; break;

	default: return -1;
	}

	return m->animation(i);
}

static void __cdecl som_logic_4041d0_trap_cp_size(SOM::Attack *dmg) //2023
{
	//NOTE: cp is not a formal argument but is on the stack
	dmg->radius = 0.000001f;
	
	int cp = (int)dmg->height; dmg->height++; //HACK

	//TODO: pass cp number to extension

	((void(__cdecl*)(void*))0x4041d0)(dmg);
}

static void som_logic_43f5b0(WORD snd, char pitch, FLOAT x, FLOAT y, FLOAT z)
{
	//2024: hack to exclude variable frame rate duplicates
	static stdext::hash_map<QWORD,DWORD> hm;
	static DWORD clean = 0;
	DWORD cmp = SOM::frame;
	if(clean!=cmp)
	{
		clean = cmp;

		for(auto it=hm.begin();it!=hm.end();)
		{
			if(clean-it->second>=4) 
			{
				auto itt = it; itt++; //+1
				it = hm.erase(it,itt);
			}
			else it++;
		}
	}
	QWORD k = (WORD)(x*100)+(WORD)(z*100)<<16;	
	k|=(QWORD)((WORD)(z*100)+(WORD)(snd)<<16)<<32ull;
	DWORD &v = hm[k]; if(cmp-v<4) return; 

	v = SOM::frame;

	((void(__cdecl*)(DWORD,char,float,float,float))0x43F5B0)(snd,pitch,x,y,z);	
}

extern void som_logic_reprogram()
{
	//NOTE: this is an off branch of som.game.cpp as of
	//late 2020. oddly there isn't very much game logic
	//up to now. a lot of som.state.cpp's code would be
	//at home here. I think I'd like to refactor it out

	EX::INI::Option op; EX::INI::Adjust ad;
	EX::INI::Bugfix bf;

		////////som.state.cpp originals/////////

	//PARTICLE EFFECTS V CYLINDER
	//0040B995 E8 96 32 00 00       call        0040EC30
	{
		/*something is wrong for cylinders?
		0040ED54 8A 47 50             mov         al,byte ptr [edi+50h]  
		0040ED57 84 C0                test        al,al
		0040ED5B 8B 54 24 6C          mov         edx,dword ptr [esp+6Ch]  
		0040ED5F 8B 44 24 68          mov         eax,dword ptr [esp+68h]  
		0040ED63 8B 8E 74 44 A4 01    mov         ecx,dword ptr [esi+1A44474h]  
		0040ED69 52                   push        edx  
		0040ED6A 8B 54 24 68          mov         edx,dword ptr [esp+68h]  
		0040ED6E 50                   push        eax  
		0040ED6F 51                   push        ecx //WRONG 
		0040ED70 8B 4C 24 6C          mov         ecx,dword ptr [esp+6Ch]  
		0040ED74 52                   push        edx //WRONG 
		0040ED75 8B 54 24 6C          mov         edx,dword ptr [esp+6Ch]  
		0040ED79 8D 44 24 38          lea         eax,[esp+38h]  
		0040ED7D 50                   push        eax  
		0040ED7E 51                   push        ecx  
		0040ED7F 52                   push        edx  
		//boxes call 0040DC70 //NPC call is at 40B888
		0040ED80 E8 5B DB FF FF       call        0040C8E0  				
		0040ED85 83 C4 1C             add         esp,1Ch  
		0040ED88 5F                   pop         edi  
		0040ED89 5E                   pop         esi  
		0040ED8A 5D                   pop         ebp  
		0040ED8B 5B                   pop         ebx  
		0040ED8C 83 C4 48             add         esp,48h  
		0040ED8F C3                   ret  
		*/
		//unreverse the argument order?
		*(BYTE*)0x40ED6F = 0x52; //push edx
		*(BYTE*)0x40ED70 = 0x51; //push ecx
		*(DWORD*)0x40ED71 = 0x70244C8B; //mov ecx,dword ptr [esp+70h]

		//going ahead and doing sprite offset
		//if(1&&EX::debug)
		*(DWORD*)0x40ED81 = (DWORD)som_logic_40C8E0-0x40ED85; //object
		//NPC call (height/raidus are reversed???)
		//0040B888 E8 53 10 00 00       call        0040C8E0
		*(DWORD*)0x40B889 = (DWORD)som_logic_40C8E0-0x40B88D;
		//enemy call?
		//0040B72E E8 AD 11 00 00       call        0040C8E0 
		*(DWORD*)0x40B72F = (DWORD)som_logic_40C8E0-0x40B733;
		//PC call? seems to be traps, but what of enemy fire?
		//0040B675 68 98 1D 9C 01       push        19C1D98h  
		//0040B67A 52                   push        edx  
		//0040B67B 55                   push        ebp  
		//0040B67C E8 5F 12 00 00       call        0040C8E0
		*(DWORD*)0x40B67d = (DWORD)som_logic_40C8E0-0x40B681; //2023

		/*2022: MHM must be 1 level up
		//boxes (objects)
		//standard box, but always running on maps with trap
		//0040EDC3 E8 A8 EE FF FF       call        0040DC70
		*(DWORD*)0x40EDC4 = (DWORD)som_logic_40dc70-0x40EDC8;
		//doors?
		//0040EEF1 E8 7A ED FF FF       call        0040DC70
		*(DWORD*)0x40EEF2 = (DWORD)som_logic_40dc70-0x40EEF6;
		//???
		//0040F00F E8 5C EC FF FF       call        0040DC70
		*(DWORD*)0x40F010 = (DWORD)som_logic_40dc70-0x40F014;
		*/
		//0040B995 E8 96 32 00 00       call        0040EC30
		*(DWORD*)0x40B996 = (DWORD)som_logic_40ec30-0x40B99a;
	}
	
		////////som.logic.cpp originals/////////
	
		//WHERE TO PUT THIS????
		//004250F6 75 73                jne         0042516B
		//
		// let power guage begin refilling while waiting on 
		// attack animation
		//
		// som.mocap.cpp cancels the animation if nearly at
		// the end so it's not frustrating
		//
		//if(op->do_arm) //EXTENSION?
		{
			*(WORD*)0x4250F6 = 0x9090; //2020
		}

		//reduced hitbox for monsters?
		bool c; if(ad->npc_hitbox_clearance(&c)||!c)
		{
			//player is already too small and can be set directly
			//but it may be repurposed in places?
			//0040C00B E8 60 04 00 00       call        0040C470
			//*(DWORD*)0x40C00C = (DWORD)som_logic_40c470-0x40C010;
			//monsters
			//0040C0D4 E8 97 03 00 00       call        0040C470
			*(DWORD*)0x40C0D5 = (DWORD)som_logic_40c470-0x40C0D9;
			//NPCs?
			//0040C1E5 E8 86 02 00 00       call        0040C470
			*(DWORD*)0x40C1E6 = (DWORD)som_logic_40c470-0x40C1Ea;

			//cube damage hit test???
			//ASSERT if 40B1D0 is entered (mode 4 damage source)
			#ifdef _DEBUG
			//004042FD E8 CE 6E 00 00       call        0040B1D0
			*(DWORD*)0x4042FE = (DWORD)som_logic_40B1D0-0x404302;
			#endif
		}
		//TODO: don't add (or adjust?) clip shape to weapon length?
		//
		// switch weapon range offset from extended clip shape to a
		// smaller hitbox
		//
		// Update: switching to even smaller hitbox. traps on using
		// the larger box
		//
		//hit damage source radius
		//004277AE D9 05 00 85 45 00    fld         dword ptr ds:[458500h] 
		//004277DF D8 04 C5 90 04 58 00 fadd        dword ptr [eax*8+580490h]
	//	*(DWORD*)0x4277B0 = 0x458380; //SOM::L.hitbox		
		*(DWORD*)0x4277B0 = 0x4582cc; //SOM::L.hitbox2		
		//
		// switch monster-vs-pc clip radius to smaller hitbox shape 
		// so it's easier to move around them (this was the original
		// shape anyway. note, my main concern is jumping around them
		// tends to bounce off instead of circle around
		//
		// TODO: this can be done for NPCs but maybe they need more
		// personal space
		//
		// NOTE: it looks like monsters can be pushed if there is a
		// wall touching the PC... I don't know the mechanism or if
		// it matters if monsters have walking CPs (mine don't yet)
		//		
		//0042701F 8B 15 00 85 45 00    mov         edx,dword ptr ds:[458500h]		
		//00427049 D8 0D 00 85 45 00    fmul        dword ptr ds:[458500h]
		//0042705D D8 0D 00 85 45 00    fmul        dword ptr ds:[458500h]
		*(DWORD*)0x427021 = 0x4582cc; //SOM::L.hitbox2		
		*(DWORD*)0x42704b = 0x4582cc; //SOM::L.hitbox2	
		*(DWORD*)0x42705f = 0x4582cc; //SOM::L.hitbox2	
		//0040B661 8B 0D 80 83 45 00    mov         ecx,dword ptr ds:[458380h] 
	//	*(DWORD*)0x40B663 = 0x4582cc; //SOM::L.hitbox2//2023: traps?
		/*USING SOM::L.hitbox2
		//this is the monsters' code
		//this is another 0.25
		//00407AFF 8B 15 CC 82 45 00    mov         edx,dword ptr ds:[4582CCh]				
		//this is part of the AI routine, NPCs have one as well
		//00407311 D9 05 CC 82 45 00    fld         dword ptr ds:[4582CCh]		
		//004070D6 D9 05 CC 82 45 00    fld         dword ptr ds:[4582CCh]
		*(DWORD*)0x407B01 = 0x458380; //SOM::L.hitbox
		*(DWORD*)0x407313 = 0x458380;
		*(DWORD*)0x4070d8 = 0x458380;*/
		//change height to crouch height (from 1.8)
		//00407B0A A1 C8 82 45 00       mov         eax,dword ptr ds:[4582C8h]
		*(DWORD*)0x407B0B = 0x45837C; //SOM::L.duck
		//hit damage source height? (from 1.8)
		//004277E6 8B 15 FC 84 45 00    mov         edx,dword ptr ds:[4584FCh] 
		*(DWORD*)0x4277E8 = 0x45837C; //SOM::L.duck		
		//00427039 E8 A2 52 FE FF       call        0040C2E0
			//enemy-pc-hit-test (426f94 is NPC)
		*(DWORD*)0x42703a = (DWORD)som_state_40c2e0_pc-0x42703e;
		*(DWORD*)0x426f95 = (DWORD)som_state_40c2e0_pc-0x426f99;
		
	//add weapon scale factor
	//00424DBA E8 11 F4 FD FF       call        004041D0
	*(DWORD*)0x424DBb = (DWORD)som_logic_4041d0-0x424DBf; //2023

		//reset respawning enemy events?
		{
			//0040666E E8 5D 00 00 00       call        004066D0
			*(DWORD*)0x40666F = (DWORD)som_logic_4066d0-0x406673;
		}

	//KF2: HOLDING OPEN DOORS (AND CONTAINERS)
	{
			//TODO? manual closure with action button

		//closing?
		//looking for timed door close (#5) directive
		//chest (15h)
		//0042BAE9 E8 22 FA FF FF       call        0042B510
		//switch (28h)
		//0042ACD7 E8 34 08 00 00       call        0042B510
		//receptacle (29h)
		//0042AD40 E8 CB 07 00 00       call        0042B51
		//doors
		//0042BB28 E8 E3 F9 FF FF       call        0042B510
		//I think this decrements the timer (42bb80)
		//0042BBFC 48                   dec         eax
		//call site
		//0042B084 E8 F7 0A 00 00       call        0042BB80
		*(DWORD*)0x42B085 = (DWORD)som_logic_42bb80-0x42B089;

		//opening?
		//this is for chests
		//0042B07C E8 5F 09 00 00       call        0042B9E0
		*(DWORD*)0x42B07d = (DWORD)som_logic_42b9e0-0x42B081;
		//have to show the subtitle manually
		//0042BAD8 E8 03 82 FF FF       call        00423CE0 
		memset((void*)0x42BAD8,0x90,5);
		//don't close automatically
		memset((void*)0x42bae9,0x90,5);
		memset((void*)0x42bb12,0x90,5);

		//2022: this subroutine is a NOP idle state for objects
		//I'm thinking here of trying to standardize the frames
		//for animations by resetting opening/closing objects to
		//the first opening frame, so animations don't have to be
		//edited both on the front and back end to do things like
		//hide coinciding polygons
		//0042B05A E8 81 68 FF FF       call        004218E0
		*(DWORD*)0x42B05b = (DWORD)som_logic_4218e0-0x42B05f;
	}		

	//ENABLE SPEAR TRAPS
	{
		//change trap to use "switch" case?
		//subtract -0xb
		//0042ABBB 83 C1 F5             add         ecx,0FFFFFFF5h  
		//0042ABBE 83 F9 1E             cmp         ecx,1Eh  
		//0042ABC1 0F 87 81 01 00 00    ja          0042AD48
		//0042ABC7 33 D2                xor         edx,edx  
		//0042ABC9 8A 91 6C AD 42 00    mov         dl,byte ptr [ecx+42AD6Ch]  
		//0042ABCF FF 24 95 50 AD 42 00 jmp         dword ptr [edx*4+42AD50h]
		*(BYTE*)0x42AD7F = 4;

		//at the stage the trap opens once, but doesn't work again

		//same pattern as above
		//0042BA90 8A 88 58 BB 42 00    mov         cl,byte ptr [eax+42BB58h]
		//0042BBE3 8A 90 80 BC 42 00    mov         dl,byte ptr [eax+42BC80h]
		*(BYTE*)0x42BB6B = 0;
		*(BYTE*)0x42BC93 = 0;

		//where does the size of the CP hit test come from?
				
		//QUICK FIX
		//this doesn't disable damage
		//0040E17B 0F 85 45 08 00 00    jne         0040E9C6  
		//memset((void*)0x40E17B,0x90,6);
		//this appears to use the height/2 as the CP radius?
		//0042B099 E8 D2 0D 00 00       call        0042BE70
		if(op->do_hit)
		{
			//YUCK: I'm just using do_hit to disable clip test
			//on traps altogether since it seems to be knocking
			//the player out of the traps way and I think it may
			//get first crack before the hit test
			//
			// NOTE: this solves climbing problem too, that is 
			// it was possible (and easy) to climb onto the CPs
			//
			// TODO: DO WITHOUT do_hit REQUIREMENT (short order)
			//
			//0040E0A5 0F 85 F2 00 00 00    jne         0040E19D 
			*(WORD*)0x40E0A6 = 0xe884; //je 40e193
			*(BYTE*)0x40e0ab = 0xe9; //jmp
			*(DWORD*)0x40e0ac = 0x000000ed; //40E19D
			*(WORD*)0x40e0b0 = 0x9090; //nop

			//NOTE: clip tests use the 4th float field that's
			//calculated around 42aa9f I think this should use
			//the same value to ensure damage is dealt (height
			//seems an odd value?
			//
			// NOTE: this is equal to the radius for a round
			// shape. I don't know if it's correct for traps
			// since they use mode 3, but it's what the clip
			// test is using (the formula is quite involved)
			//
			//0042BEF3 D9 46 68             fld         dword ptr [esi+68h]
			*(BYTE*)0x42BEF5 = 0x74;
			//don't halve 0x74 ... NOTE: may want to double instead?
			//0042BEF9 D8 0D 90 82 45 00    fmul        dword ptr ds:[458290h]  
		//	if(1) *(WORD*)0x42BEF9 = 0x35d8; //fdiv //a bit much
		//	else memset((void*)0x42BEF9,0x90,6); //not quite enough?
			//TODO: maybe use 4th value in PRF file. I think 0x74 is the 
			//value it once represented but computed by fitting a radius
			//to a square's far corner instead
			//
			// NEW STRATEGY?
			//
			//HMMM: maybe this is height?
			//this is the clip test... I suspect doubling the radius is 
			//inappropriate 
			//0040E142 DC C0                fadd        st(0),st 
		//	*(WORD*)0x40E142 = 0x9090;
		//	static const FLOAT f = 0.5f; //1.5f;
		//	*(DWORD*)0x42befb = (DWORD)&f;
		//	*(DWORD*)0x42BEF9 = 0x05D8; //fmul->fadd //NO GOOD
		}
		//this is active while opening, so I'm using it to play 
		//a third sound effect for the spear
		*(DWORD*)0x42B09a = (DWORD)som_logic_42BE70-0x42B09e;
	}

	//recursive SFX? (stack overflow on same SFX as subordinate)
	{
		/*wanted to see if windcutter could go through the target
		//these should've been called after loading afterward
		//0042E649 E8 72 FF FF FF       call        0042E5C0
		//0042E628 E8 93 FF FF FF       call        0042E5C0
		*(DWORD*)0x42E64a = (DWORD)som_logic_42e5c0_a-0x42E64e;
		*(DWORD*)0x42E629 = (DWORD)som_logic_42e5c0_b-0x42E62f;*/

		//try two?
		//00430308 E8 E3 EE FF FF       call        0042F1F0  
		//0043030D 83 C4 14             add         esp,14h  
		//00430310 C7 46 2C 01 00 00 00 mov         dword ptr [esi+2Ch],1 
		*(DWORD*)0x430309 = (DWORD)som_logic_42f1f0-0x43030d;
		memset((void*)0x430310,0x90,7);
	}
	
	//2020/2021: monster AI?
	{
		/*som_logic_406ab0 NOW COVERS THIS
		//this prevents movement in mode 3
		//it's walking and turning at the same time but not
		//actually moving, which is strange, especially since
		//the PC is in view
		//00407333 83 F9 03             cmp         ecx,3  
		//00407336 0F 84 9E 00 00 00    je          004073DA 
		memset((void*)0x407336,0x90,6);*/

		/*I think once sighted the monster remains aware even
		after out of view until out of range
		//speed up turning to give a chance?
		//004077F9 D8 0d 90 82 45 00    fmul        dword ptr ds:[458290h] 
		*(WORD*)0x4077f9 = 0x35d8; //fdiv*/
		*(DWORD*)0x4077fb = (DWORD)&som_logic_406ab0_fast_turn; //0.5f
	
		float step = 1;
		step/=bf->do_fix_animation_sample_rate?60:30;
		som_logic_soft.resize(100);
		extern float som_mocap_am_calibrate(float,float,float);
		for(int i=100;i-->1;0)
		som_logic_soft[i] = som_mocap_am_calibrate(i*0.1f,step,som_logic_soft_impedance);
		//00406688 E8 23 04 00 00       call        00406AB0
		*(DWORD*)0x406689 = (DWORD)som_logic_406ab0-0x40668d;

		//use the other attack radius for attempting the attack
		//(I've so far found no reference to this value)
		//00407592 8D AB 1C 01 00 00    lea         ebp,[ebx+11Ch]
		*(BYTE*)0x407594 = 0x10;

		//2024
		//use the real scale, including SOM_PRM scaling
		//004075C5 D9 46 1C             fld         dword ptr [esi+1Ch] 
		//0040767A D8 4E 1C             fmul        dword ptr [esi+1Ch]
		//0040768B D8 4E 1C             fmul        dword ptr [esi+1Ch]		
		*(BYTE*)0x4075C7 = *(BYTE*)0x40767c = *(BYTE*)0x40768d = 0x64;
		//00407E5D D8 4B 1C             fmul        dword ptr [ebx+1Ch]
		*(BYTE*)0x407E5f = 0x64; //sfx
		//these are on load. it's an AI parameter
		//som_MPX_405de0 adjusts the radius/height
		//004060CF D8 4D 1C             fmul        dword ptr [ebp+1Ch]
		//0040611A D8 4D 1C             fmul        dword ptr [ebp+1Ch] 
		*(BYTE*)0x4060d1 = *(BYTE*)0x40611c = 0x64; 
	}
	if(1) //2024: NPC AI?
	{
		//00428f59 e8 b2 04 00 00       call       00429410
		*(DWORD*)0x428f5a = (DWORD)som_logic_429410-0x428f5e;		
		//HACK: knockout (NOP) last function call instead?
		//00429A29 E8 92 06 00 00       call       0042A0C0
		//*(DWORD*)0x429A2a = (DWORD)som_logic_42a0c0-0x429A2e;
		memset((void*)0x429a29,0x90,5);		
		//remove this 0,0,0 test for turning while walking?
		//0042a11a 0f 85 9f 02 00 00        JNZ        LAB_0042a3bf
		memset((void*)0x42a11a,0x90,6);
	}
	if(0) //2020: let flying monsters have shadows?
	{			
		//this checks bit 2 in the PRF flags/locomotion byte
		//the only monster that really needs no shadow is the
		//Stoneface. the shadow radius is being used as a hit
		//box when it really shouldn't be

		//00406374 F6 46 60 02          test        byte ptr [esi+60h],2
		//00406378 75 2E                jne         004063A8  
		*(WORD*)0x406378 = 0x9090;
	}
	//2023: don't use shadow for clip radius
	//00406174 75 08                jne         0040617E
	//00428D98 74 05                je          00428D9F
	*(BYTE*)0x406174 = 0xEB; //jmp
	*(BYTE*)0x428d98 = 0xEB; //jmp

	//2021: limit activation by height
	{
		//00408566 E8 55 07 00 00       call        00408CC0
		//00408612 E8 A9 06 00 00       call        00408CC0
		//0040874C E8 6F 05 00 00       call        00408CC0
		*(DWORD*)0x408567 = (DWORD)som_logic_408cc0-0x40856b;
		*(DWORD*)0x408613 = (DWORD)som_logic_408cc0-0x408617;
		*(DWORD*)0x40874d = (DWORD)som_logic_408cc0-0x408751;
	}
	//2022: ext_zr_flags (som_logic_408cc0 does action events)
	if(bf->do_fix_trip_zone_range)
	{
		//004088C0 E8 BB 07 00 00       call        00409080 //square
		//00408922 E8 D9 08 00 00       call        00409200 //circle
		*(DWORD*)0x4088C1 = (DWORD)som_logic_409080-0x4088C5;		
		*(DWORD*)0x408923 = (DWORD)som_logic_409200-0x408927;
		//SOM::xyz
		//004092C2 8B 0D 98 1D 9C 01    mov         ecx,dword ptr ds:[19C1D98h]
		//004092BC A1 A0 1D 9C 01       mov         eax,dword ptr ds:[19C1DA0h] 
		//00409323 8B 15 98 1D 9C 01    mov         edx,dword ptr ds:[19C1D98h]
		//0040931D 8B 0D A0 1D 9C 01    mov         ecx,dword ptr ds:[19C1DA0h]
		//square
		//004090E7 D9 05 98 1D 9C 01    fld         dword ptr ds:[19C1D98h]  
		//004090F0 D9 05 A0 1D 9C 01    fld         dword ptr ds:[19C1DA0h]  
		//00409137 D9 05 98 1D 9C 01    fld         dword ptr ds:[19C1D98h]  
		//00409140 D9 05 A0 1D 9C 01    fld         dword ptr ds:[19C1DA0h]
		//00409182 D9 05 98 1D 9C 01    fld         dword ptr ds:[19C1D98h]  
		//0040918B D9 05 A0 1D 9C 01    fld         dword ptr ds:[19C1DA0h]
		const static WORD xz[10] = 
		{ 0x2C4,0x2Bd,0x325,0x31f,0x0E9,0x0F2,0x139,0x142,0x184,0x18d };
		for(int i=10;i-->0;)
		*(DWORD*)(0x409000+xz[i]) = (DWORD)SOM::xyz+(i&1)*8;
	}

	//2023: default animations for NPCs
	{
		//429e47 //429e6c //429e80 //429ec6
		*(DWORD*)0x429e48 = (DWORD)som_logic_4413a0_talk-0x429e4c;
		*(DWORD*)0x429e6d = (DWORD)som_logic_4413a0_talk-0x429e71;
		*(DWORD*)0x429e81 = (DWORD)som_logic_4413a0_talk-0x429e85;
		*(DWORD*)0x429ec7 = (DWORD)som_logic_4413a0_talk-0x429ecb;
	}

	//2023: trap CP radii
	{
		//NOTE: the height field is overloaded with the cp radius
		//by dividing by 2. this is too big
		 
		//0042BF64 E8 67 82 FD FF       call        004041D0  
		*(DWORD*)0x42BF65 = (DWORD)som_logic_4041d0_trap_cp_size-0x42BF69;
	}

	//2024: variable frame rate //som_state_43f5b0
	{	
		//Enemies
		//00407FCA E8 E1 75 03 00       call        0043F5B0 
		*(DWORD*)0x407fcb = (DWORD)som_logic_43f5b0-0x407fcf;
		//2024: event
	//	*(DWORD*)0x409a3e = (DWORD)som_logic_43f5b0-0x409a42;
		//NPCs
		//004299FB E8 B0 5B 01 00       call        0043F5B0
		*(DWORD*)0x4299FC = (DWORD)som_logic_43f5b0-0x429A00;
		//2024: traps
		*(DWORD*)0x42b840 = (DWORD)som_logic_43f5b0-0x42b844;
		//door opening
		//0042BA60 E8 4B 3B 01 00       call        0043F5B0
		*(DWORD*)0x42BA61 = (DWORD)som_logic_43f5b0-0x42BA65;
		//door closing
		//0042BC63 E8 48 39 01 00       call        0043F5B0
		*(DWORD*)0x42BC64 = (DWORD)som_logic_43f5b0-0x42BC68;
		//magic
		//0042EAE9 E8 C2 0A 01 00       call        0043F5B0
		*(DWORD*)0x42EAEA = (DWORD)som_logic_43f5b0-0x42EAEE;
		//2024: sfx
		*(DWORD*)0x42f2ca = (DWORD)som_logic_43f5b0-0x42f2ce;
	}
}