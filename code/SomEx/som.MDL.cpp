#include "Ex.h" 
EX_TRANSLATION_UNIT

#include "Ex.output.h"

#define SOMVECTOR_MATH
#include "../Somplayer/Somvector.h"
#include "../lib/swordofmoonlight.h"
#include "../x2mdl/x2mdl.h"

#include "SomEx.ini.h"

#include "som.game.h"
#include "som.state.h"
#include "som.tool.hpp"

enum{ som_MDL_with_mdo=1 }; //TESTING

//2021: Obj/Ene/NpcEdit?
static void*(__cdecl*som_MDL_malloc)(size_t) = 0;
static void(__cdecl*som_MDL_free)(void*) = 0;
static DWORD(__cdecl*som_MDL_x4418b0)(SOM::MDL*,FLOAT*,DWORD) = 0;
static SOM::MDL::data*(__cdecl*som_MDL_x440030)(char*) = 0;
static void(__cdecl*som_MDL_x44d810)(void*) = 0;
static DWORD(__cdecl*som_TXR_x448fd0)(char*,int,int,int) = 0;
static BYTE(__cdecl*som_MDL_x448780)(DWORD) = 0;
static DWORD(__cdecl*som_MDL_x447ff0)(float,float,float,float,float,float,float) = 0;
static BYTE(__cdecl*som_MDL_x448100)(DWORD) = 0;
static SOM::MDO::data*(__cdecl*som_MDO_x445660)(char*) = 0; //MDO?
static void **som_MDL_x45f350 = 0; //som_MDL_4
static void som_MDL_4(SOM::MDL::data*,DWORD,BYTE**);

extern DWORD som_MDL_449d20_swing_mask(bool atk)
{
	//this algorithm tells if ARM.MDL's current animation
	//animates one or both arms (left and right)

	SOM::MDL &arms = *SOM::L.arm_MDL;

	//two arms is more complicated (will probably want to
	//use two instances eventually)
	//static DWORD o = ~0;
	static DWORD o,o2 = 0;
	int c1 = atk||arms.d>1?arms.c:65535;
	int c2 = arms.ext.d2?arms.ext.e2:65535;
	DWORD cmp = c1|c2<<16;
	cmp = ~cmp;
	if(o2==cmp) return o; o2 = cmp;

	namespace mdl = SWORDOFMOONLIGHT::mdl;
	mdl::image_t img; mdl::maptoram(img,arms->file_data,arms->file_size);

	//NOTE: som_game_equip is matching
	//this exactly 9 for skeleton_size
	if(9!=mdl::animationchannels(img))
	return o|=0xF;	

	o = 0; for(int arm=1;arm<=2;arm++)
	{	
		int c = arm==1?c1:c2;
		mdl::animation_t ani;
		if(!mdl::animations(img,&ani,1,0,c))
		continue;

		int t = c?1:2;
		mdl::hardanim_t ha[9] = {};
		mdl::animate(ani,ha,0,t,t==2,ani->time-t);
		for(int i=9;i-->0;) 
		if(ha[i].dvt) o|=1<<i;

		//these high bits mark two-hand animations
		//now since both animations are merged for
		//the regular bits making it unclear which
		//use both arms and so may be incompatible
		if(o&0xf&&o&0xf0) o|=1<<29+arm;
	}
	return o;
}

extern BYTE som_MDL_transform_hard(DWORD*);
extern BYTE som_MDL_4418b0(SOM::MDL&,float*,DWORD);
#if 1 //REFERENCE?
enum{ som_MDL_hz=0 }; //EXPERIMENTAL
static int som_MDL_animate_soft(SOM::MDL &mdl, float wt=1) //444410
{	
	namespace Sm = SWORDOFMOONLIGHT::mdl;

	//Ghidra's names
	auto puVar4 = (Sm::softanimframe_t*)mdl.softanim_read_head;
	
	//OPTIMIZING/INNOVATING
	int ff = mdl.d-mdl.f; assert(ff>=1); if(ff!=1)
	{
		int most = puVar4->time-mdl.softanim_read_tick;
		if(ff>most) ff = most;

		wt = wt*ff; //NOTE: staying in one frame
	}

	if(!wt) finish: //REMOVE ME?
	{
		assert(!EX::isNeg(wt)); //IMPLEMENT ME?

		for(int i=0;i<ff;i++)
		if(puVar4->time<=++mdl.softanim_read_tick) 
		{
			if(!*++puVar4)
			(WORD*&)puVar4 = mdl->soft_anim_buf[mdl.c].file_ptr2;
			mdl.softanim_read_head = (WORD*)puVar4;
			mdl.softanim_read_tick = 0;
		}
		return ff;
	}

	float cp[3]; 
	if(mdl.f<1 //NEW
	||!mdl.ext.subtract_base_control_point) //not advance by CP?
	{
		//EXTENSION
		//I think it's wise to ignore the establishing frame
		//(4418b0) doesn't seem to check for it
		memset(cp,0x00,sizeof(cp));
	}
	else //OLD
	{
		//BUG when animation isn't up-to-date get cp's delta
	//	som_MDL_4418b0(mdl,cp,~0);
		//YUCK: f+1 convention differs for som_MDL_animate_hard
		som_MDL_4418b0(mdl,cp,mdl.f+1); 

		//2021: instead MDL is kept in 1/1024 units the whole time
		//which is better for shaders/normals
	//	if(!mdl->ext.mdo)
		{
	//		for(int k=3;k-->0;) cp[k]*=wt*1024;
		}
	//	else //2021
		{
 			for(int k=3;k-->0;) cp[k]*=wt;
		}
	}
	wt*=0.00097656f; //1/1024

	//NOTE: I know how to do this, but I'm deliberately
	//following 444410 in case there's anything to learn
	//from it

	//auto puVar4 = (Sm::softanimframe_t*)mdl.softanim_read_head;
	assert(mdl->soft_anim_ptr==mdl->soft_anim_ptr2);
	WORD *puVar2 = mdl->soft_anim_ptr;
	UINT iVar8 = mdl->soft_anim_ptr2[1];
	
	//REWRITE?
	//uVar7 will be 0 or -1 (~0) so -uVar7 is +1 and
	//puVar4->lo^uVar7 inverts if -1 or nothing if 0
	auto uVar7 = puVar4->lo>>0x1f; //CDQ instruction
	iVar8+=puVar2[(puVar4->lo^uVar7)-uVar7+iVar8*2-1];

	WORD *pbVar9 = puVar2+iVar8*2;
	CHAR *p = (CHAR*)pbVar9+*pbVar9; //psVar11
	pbVar9+=1;

	//444410 makes this needlessly complicated
	//and bad performing (not doing that here)
	float delta = (uVar7?-1:1)*wt/puVar4->time;

	for(UINT i=0,n=mdl->file_head[4];i<n;i++) //primchans
	{
		//TODO: add to lib/swordofmoonlight.h account???
		if(i!=*(BYTE*)pbVar9) continue;

        uVar7 = mdl->l->ditto_prims_ptr[i].vertcount;

		//DIVIDE BY 8 WITH SOME ROUNDING LOGIC?
		//TODO: compare to lib/swordofmoonlight.h account
        UINT uVar5 = uVar7&0x80000007;
        //if((int)uVar5<0) //overflow paranoid compiler?
        //uVar5 = (uVar5-1|0xfffffff8)+1;
		iVar8 = ((uVar7+(uVar7>>0x1f&7))>>3)+(uVar5!=0);

		//TODO: interleaving would be more cache friendly
		auto x = (BYTE*)pbVar9+1; 
		auto y = x+iVar8;
		auto z = y+iVar8;

		//TODO: add to lib/swordofmoonlight.h account???
		(BYTE*&)pbVar9 = z+iVar8;

		float *v = mdl.anim_vertices[mdl->primitive_buf[i].vstart];
		for(int j=0;j<uVar7;j++,v+=3)
		{
			//this can be simpler
			uVar5 = j&0x80000007;
			//if((int)uVar5<0) //overflow paranoid compiler?
			//uVar5 = (uVar5-1|0xfffffff8)+1;
		//	BYTE bVar6 = 1<<((BYTE)uVar5&0x1f);
		//	if(!uVar5&&j)
			{
		//		x++; y++; z++;
			}
			uVar5 = 1<<(j&7); //...

			if(*x&uVar5) //bVar6
			v[0]+=delta*(1&*p?(INT)*p++:*((__unaligned SHORT*&)p)++);
			if(*y&uVar5) //bVar6
			v[1]-=delta*(1&*p?(INT)*p++:*((__unaligned SHORT*&)p)++);
			if(*z&uVar5) //bVar6
			v[2]+=delta*(1&*p?(INT)*p++:*((__unaligned SHORT*&)p)++);

			if(uVar5==1<<7){ x++; y++; z++; }

			for(int k=3;k-->0;) v[k]-=cp[k];
		}
	}

	goto finish;
}
#else
enum{ som_MDL_hz=1 };
static int som_MDL_animate_soft(SOM::MDL &mdl, float wt=1) //444410
{	
	/////UNFINISHED (having problems) EXPERIMENTAL///////

	namespace Sm = SWORDOFMOONLIGHT::mdl; wt*=0.00097656f; //1/1024

	//Ghidra's names
	auto p = (Sm::softanimframe_t*)mdl.softanim_read_head;
	
	const float hz_30 = SOM::motions.hz_30;
	const float l_hz_30 = SOM::motions.l_hz_30;
	
	int t0 = 0; //HACK: neeed time up to now
	{
		auto q = (Sm::softanimframe_t*)mdl->soft_anim_buf[mdl.c].file_ptr2;
		for(;q!=p;q++) t0+=q->time; //DANGER
	}

	int f = mdl.f, d = mdl.d;
	
	float dz = d*l_hz_30;

	int loop = 0;
	for(float w=wt;f<d;wt=w,loop++)
	{
		float fz = f*l_hz_30;
		float gz = min(dz,t0+p->time);

		if(mdl.softanim_read_tick)
		{
			gz = gz; //breakpoint
		}

		//this is the number of steps in this frame
		float fc = ceil(fz);
		int ticks = int(fz<fc&&gz>=fc)+(int)(gz-fc);
		assert(!EX::isNeg(wt)); //IMPLEMENT ME?

		int ff = f; f+=max(1,ticks);

		float cp[3] = {};
		if(ff>=1 //NEW
		&&mdl.ext.subtract_base_control_point) //not advance by CP?
		{
			//BUG when animation isn't up-to-date get cp's delta
			//som_MDL_4418b0(mdl,cp,~0);
			//YUCK: f+1 convention differs for som_MDL_animate_hard
			som_MDL_4418b0(mdl,cp,mdl.f+1); 
			for(int i=f;i-->ff;)
			{
				float cp2[3];
				som_MDL_4418b0(mdl,cp2,i);
			
				if(cp[2])
				{
					ff = ff; //breakpoint
				}

				float z = 1;
				if(i==f-1)
				{
					z = 1-(fz-(int)fz);
				}
				else if(i==ff)
				{
					z = dz-ticks;
				}
				z*=hz_30;

				for(int j=3;j-->0;)
				cp[j]+=cp2[j]*z;
			}

			//2021: instead MDL is kept in 1/1024 units the whole time
			//which is better for shaders/normals
		//	if(!mdl->ext.mdo)
			{
		//		for(int k=3;k-->0;) cp[k]*=wt*1024;
			}
		//	else //2021
			{
 				for(int k=3;k-->0;) cp[k]*=wt;
			}
		}

						wt*=gz-fz; //!!
											
					//	EX::dbgmsg("wt: %d %f %f-%f %d %d",mdl.f,wt,gz,fz,ticks,loop);

						if(loop==2)
						{
							wt = wt; //breakpoint
						}

		//NOTE: I know how to do this, but I'm deliberately
		//following 444410 in case there's anything to learn
		//from it

		//auto p = (Sm::softanimframe_t*)mdl.softanim_read_head;
		assert(mdl->soft_anim_ptr==mdl->soft_anim_ptr2);
		WORD *animp = mdl->soft_anim_ptr;
		UINT stride = mdl->soft_anim_ptr2[1];
	
		//REWRITE?
		//n will be 0 or -1 (~0) so -n is +1 and
		//p->lo^n inverts if -1 or nothing if 0
		auto n = p->lo>>0x1f; //CDQ instruction
		stride+=animp[(p->lo^n)-n+stride*2-1];

		//444410 makes this needlessly complicated
		//and bad performing (not doing that here)
		float delta = (n?-1:1)*wt/p->time;
		
		WORD *wordp = animp+stride*2;
		CHAR *b = (CHAR*)wordp+*wordp; //psVar11
		wordp+=1;

		for(UINT i=0,n=mdl->file_head[4];i<n;i++) //primchans
		{
			//TODO: add to lib/swordofmoonlight.h account???
			if(i!=*(BYTE*)wordp) continue;

			n = mdl->l->ditto_prims_ptr[i].vertcount;

			//DIVIDE BY 8 WITH SOME ROUNDING LOGIC?
			//TODO: compare to lib/swordofmoonlight.h account
			stride = n&0x80000007;
			//if((int)stride<0) //overflow paranoid compiler?
			//stride = (stride-1|0xfffffff8)+1;
			stride = ((n+(n>>0x1f&7))>>3)+(stride!=0);

			//TODO: interleaving would be more cache friendly
			auto x = (BYTE*)wordp+1; 
			auto y = x+stride;
			auto z = y+stride;

			//TODO: add to lib/swordofmoonlight.h account???
			(BYTE*&)wordp = z+stride;

			float *v = mdl.anim_vertices[mdl->primitive_buf[i].vstart];
			for(unsigned j=0;j<n;j++,v+=3)
			{
				//this can be simpler
				UINT bit = j&0x80000007;
				//if((int)bit<0) //overflow paranoid compiler?
				//bit = (bit-1|0xfffffff8)+1;
			//	BYTE bVar6 = 1<<((BYTE)bit&0x1f);
			//	if(!bit&&j)
				{
			//		x++; y++; z++;
				}
				bit = 1<<(j&7); //...

				if(*x&bit) //bVar6
				v[0]+=delta*(1&*b?(INT)*b++:*((__unaligned SHORT*&)b)++);
				if(*y&bit) //bVar6
				v[1]-=delta*(1&*b?(INT)*b++:*((__unaligned SHORT*&)b)++);
				if(*z&bit) //bVar6
				v[2]+=delta*(1&*b?(INT)*b++:*((__unaligned SHORT*&)b)++);

				if(bit==1<<7){ x++; y++; z++; }

				for(int k=3;k-->0;) v[k]-=cp[k];
			}
		}

		mdl.softanim_read_tick+=ticks;
		
		if(p->time<=mdl.softanim_read_tick) 
		{
			t0+=p->time;

			//these don't always line up but can be 
			//ignored
		//	assert(p->time==mdl.softanim_read_tick);

			if(!*++p) (WORD*&)p =
			mdl->soft_anim_buf[mdl.c].file_ptr2;
			mdl.softanim_read_head = (WORD*)p;
			mdl.softanim_read_tick = 0;
		}
	}

	return mdl.d-mdl.f; //REMOVE ME?
}
#endif
static void som_MDL_restart_hard(SOM::MDL &mdl, int ii, int n)
{
	for(int i=ii;i<n;i++)
	{
		float *b = mdl.skeleton[i].uvw;
		memset(b+0,0x00,3*sizeof(float)); //uvw
		for(int j=3;j<=5;j++) //scale
		*(DWORD*)(b+j) = 0x3f800000;
		memset(b+6,0x00,3*sizeof(float)); //xyz
	}
	mdl.hardanim_read_head = mdl->hard_anim_buf[mdl.e].file_ptr2;

	if(&mdl==SOM::L.arm_MDL) //EXTENSION
	{
		assert(SOM::game);

		float s = EX::INI::Player()->arm_bicep; //0.75
		float s2 = EX::INI::Player()->arm_bicep2;

		for(int pass=1;pass<=2;pass++)
		{
			int pc = pass==1?3:7;

			if(pc<ii||pc>=n) continue;

			//REMINDER: it's som_MDL_449f70_scale that really 
			//implements arm_bicep

			auto bicep = mdl.skeleton[pc].scale;
			bicep[0] = s;
			bicep[1] = s2;
			bicep[2] = s;
		}
	}
}
static void som_MDL_animate_hard(SOM::MDL &mdl, float wt=1) //445120
{
	//NOTE: I know how to do this, but I'm deliberately
	//following 445120 in case there's anything to learn
	//from it

	bool reverse = EX::isNeg(wt); //EXTENSION

	int first = 3; if(reverse) //EXTENSION
	{
		if(mdl.f==0){ assert(0); return; }

		WORD test1 = mdl.hardanim_read_head[-2];
		WORD test2 = mdl.hardanim_read_head[-1];

		//NOTE: som_db.exe didn't implement reverse animation
		//nonetheless these reverse iterators are part of MDL
		mdl.hardanim_read_head-=2*mdl.hardanim_read_head[-2];

		assert(test1==mdl.hardanim_read_head[0]&&!test2);
		assert(mdl.hardanim_read_head>mdl->hard_anim_buf[mdl.e].file_ptr2);
	}
	else if(mdl.f==0) //restarting?
	{
		first+=mdl.e==0; //CRAZINESS

		//d2 is swapped... so this is ambiguous... it
		//doesn't really belong here regardless
		//if(!mdl.ext.d2) //YUCK: arm.mdl?
		//som_MDL_restart_hard(mdl,0,mdl->skeleton_size);
		assert(mdl.hardanim_read_head==mdl->hard_anim_buf[mdl.e].file_ptr2);
	}

	if(!wt) finish: 
	{
		if(!reverse)
		mdl.hardanim_read_head+=2*mdl.hardanim_read_head[0];

		return;
	}

	//EXTENSION
	//
	// originally som_db.exe ignores the root's
	// translation component, but that requires
	// a dummy node. soft animations already used
	// this approach of subtracting the CP and
	// it's probably needed for blending
	// 
	// NOTE: I think maybe objects don't have their
	// positions advanced, and if they did that 
	// could pose a problem if they need to remain
	// stationary (owing to floating-point drift)
	// ...
	// 42afb0 would need to add the CP delta...
	// to leave the option open the MDL data needs
	// to be marked for objects and exempted below
		
	//ignoring the establishing frames makes
	//more sense in some tests with vertical
	//doors to cancel their animation... I'm
	//not sure yet if objects should be made
	//to move like NPCs but they don't right
	//now
	float cp[3]; 
	if(mdl.f<first-2 //ASSUMING MODERN?
	||!mdl.ext.subtract_base_control_point) //not advance by CP?
	{
		memset(cp,0x00,sizeof(cp));
	}
	else
	{
		/*BUG when animation isn't up-to-date get cp's delta
		som_MDL_4418b0(mdl,cp,~0);*/
		//YUCK: f+1 convention differs for som_MDL_animate_soft
		som_MDL_4418b0(mdl,cp,mdl.f); //f+1 overflows 60fps mode

		for(int k=3;k-->0;) cp[k]*=wt;
	}

	//2021: originally a scale matrix added 1/1024
	//however MDO files don't need this and it's
	//better to put MDL in the same units
	const float delta = wt*0.08789063f; //rotate (180/2048)
	const float delta2 = wt*0.00097656f; //translate (1/1024)
	//const float delta3 = wt*0.0078125f; //scale

	union //OPTIMIZING
	{
		int mask; 
		struct{ int diffmask:8,bitsmask:8; };
	};
	CHAR *p = (CHAR*)mdl.hardanim_read_head+4;
	for(int n=mdl.hardanim_read_head[1];n-->0;)
	{
		int channel = (BYTE)p[0];
		mask = (BYTE)p[1]|(BYTE)p[2]<<8;		
		p+=first;
		if(first==4) //CRAZINESS
		{
			//REMOVING BRANCH
			//mdl.skeleton[channel].mapped = true;
			//mdl.skeleton[channel].parent = p[-1];
			/*2021: can no longer allow 440f30 to run at MPX load time
			assert(mdl.skeleton[channel].mapped);*/
			auto &ch = mdl.skeleton[channel];
			if(!ch.mapped) //som_MDL_440f30_loading?
			{
				ch.mapped = true;				
				ch.parent = (BYTE)p[-1]; //255 (not -1)
			}
		}
		assert(channel<(int)mdl->skeleton_size); //arm.mdl is crashing 

		float *b = mdl.skeleton[channel].uvw;

		if(diffmask&1)
		b[0]+=delta*(bitsmask&1?(INT)*p++:*((__unaligned SHORT*&)p)++);
		if(diffmask&2)
		b[1]+=delta*(bitsmask&2?(INT)*p++:*((__unaligned SHORT*&)p)++);
		if(diffmask&4)
		b[2]+=delta*(bitsmask&4?(INT)*p++:*((__unaligned SHORT*&)p)++);
		if(diffmask&8)
		b[6]+=delta2*(bitsmask&8?(INT)*p++:*((__unaligned SHORT*&)p)++);
		if(diffmask&16)
		b[7]+=delta2*(bitsmask&16?(INT)*p++:*((__unaligned SHORT*&)p)++);
		if(diffmask&32)
		b[8]+=delta2*(bitsmask&32?(INT)*p++:*((__unaligned SHORT*&)p)++);
		if(mask&0x40C0)
		{
			//don't precompute delta3 because scaling is too rare
			const float delta3 = wt*0.0078125f;
			if(diffmask&64) b[3] = delta3*(BYTE)*p++; 
			if(diffmask&128) b[4] = delta3*(BYTE)*p++; 
			if(bitsmask&64) b[5] = delta3*(BYTE)*p++; 
		}
		//this is got from the CP file... 445120 stores the delta
		//and checks this before accumulating
		if(255==mdl.skeleton[channel].parent)
		{
			//EXTENSION (comments are up above)
			if(8&mdl->file_head[0]) 
			{
				b[6]-=cp[0];
				b[7]+=cp[1];
				b[8]-=cp[2];
			}
			else //Moratheia?
			{
				memset(b+6,0x00,3*sizeof(float));
			}
		}
	}

	goto finish;
}
static void som_MDL_animate(SOM::MDL &mdl)
{
	//NOTE: this is (I believe) identical to the original
	//445410, maybe it was a waste of time to write, but
	//the subroutines aren't

	int mode = 7&*mdl->file_head;
	if(!mode) return;

	assert(!mdl._soft_vertices);

	bool recover = false; //REMOVE ME
	bool uptodate = true;
	if(mdl.e!=mdl.c) reset:
	{
		mdl.e = mdl.c;
		mdl.f = mode&3?-1:0;
		if(mdl.ext.f2>=1&&!mdl.ext.d2)
		mdl.ext.f2 = mdl.f;
	}
	if(mdl.f<=0)
	{
		uptodate = false;

		if(mdl.f==-1&&mode&3) 
		{
			//moving out of 445120 (animate_hard)
			if(!mdl.ext.d2)
			som_MDL_restart_hard(mdl,0,mdl->skeleton_size);
		}
		if(mdl.f==0&&mode&4)
		{
			//NOTE: there's a second memcpy at
			//440FE9, immediately after, which
			//is disabled. likely it's normals
			if(mdl->ext.mdo) 
			memset(mdl.anim_vertices,0x00,mdl->vertex_count*12);
			else memcpy(mdl.anim_vertices,mdl->vertex_buf,mdl->vertex_count*12);
			mdl.softanim_read_head = mdl->soft_anim_buf[mdl.c].file_ptr2;
			mdl.softanim_read_tick = 0;
			if(mdl.ext.anim_read_head2)
			{
				mdl.ext.anim_read_head2 = mdl->soft_anim_buf[mdl.ext.e2].file_ptr2;
				mdl.ext.anim_read_tick2 = 0;
			}
		}
	}	
	else if(mdl.d<mdl.f) //reverse_d?
	{
		if(!mdl.ext.reverse_d) //arm?
		goto reset;

		uptodate = false; //EXPERIMENTAL

		if(mode&3) //hard
		{
			while(mdl.f>mdl.d)
			{
				mdl.f--;
				som_MDL_animate_hard(mdl,-1);
			}
		}
		else assert(0); goto reverse_d;
	}
	if(mdl.ext.d2) //arm.mdl? //TESTING
	{
		assert(mdl.ext.e2>=0);
		assert(&mdl==SOM::L.arm_MDL);

		if(mdl.f<=0) //YUCK
		{
			assert(mdl.d>=1);

			//can't help with two-handed attacks???
			if(som_MDL_449d20_swing_mask(0)&1<<30)
			{
				//TODO: need to solve elsewhere

				mdl.ext.d2 = mdl.ext.f2 = 0;
					
				mdl.f = -1; 
				
				som_MDL_restart_hard(mdl,0,mdl->skeleton_size);
				goto duce;
			}
			
			recover = true;

			assert(mdl.d>-1);
			if(mode&3)
			som_MDL_restart_hard(mdl,0,4);
		}

		if(mode&3) //legit mode??
		{
			if(int dir=mdl.ext.d2-mdl.ext.f2)
			{
				uptodate = false;

				for(int one_off=2;one_off-->0;)
				{
					std::swap(mdl.ext.d2,mdl.d);
					std::swap(mdl.ext.e2,mdl.e);
					std::swap(mdl.ext.f2,mdl.f);					
					std::swap(mdl.ext.anim_read_head2,mdl.hardanim_read_head);				
					if(one_off) if(dir>0)
					{
						if(-1==mdl.f) //YUCK
						{
							som_MDL_restart_hard(mdl,4,mdl->skeleton_size);
							for(int i=4;i-->0;)
							memcpy(mdl.skeleton[i]._uvw_scale_xyz2,mdl.skeleton[i].uvw,sizeof(float)*9);

						}
						while(mdl.f<mdl.d) 
						{
							mdl.f++;
							som_MDL_animate_hard(mdl);

							if(mdl.f==1) //YUCK
							for(int i=4;i-->0;)
							memcpy(mdl.skeleton[i].uvw,mdl.skeleton[i]._uvw_scale_xyz2,sizeof(float)*9);
						}
					}
					else while(mdl.f>mdl.d)
					{
						mdl.f--;
						som_MDL_animate_hard(mdl,-1);
					}
				}
			}
		}
		else assert(0);

		if(recover) //REMOVE ME
		{
			for(int i=mdl->skeleton_size;i-->4;)
			memcpy(mdl.skeleton[i]._uvw_scale_xyz2,mdl.skeleton[i].uvw,sizeof(float)*9);
			duce:;
		}
	}
	while(mdl.f<mdl.d) //-1?
	{
		uptodate = false;
		
		if(mode&4) //soft?
		{
			//OPTIMIZING
			//this is how SOM does it, but it's actually a lot more work this way
			//if the animation isn't up-to-date
			//mdl.f++;

			//diagonal gait?
			if(auto&swap1=mdl.ext.anim_read_head2)
			{
				auto &swap2 = mdl.ext.anim_read_tick2;

				//mdl.ext.f2++;

				//EX::dbgmsg("f2: %d %d (%d)",mdl.f,mdl.ext.f2,mdl.f==mdl.ext.f2);
				assert(mdl.f==mdl.ext.f2);
				assert(mdl.c!=mdl.ext.e2);
				
				while(mdl.f<mdl.d)
				mdl.f+=som_MDL_animate_soft(mdl,mdl.ext.anim_weights[0]);

				for(int one_off=2;one_off-->0;)
				{
					//YUCK: c/d is for som_MDL_4418b0
					std::swap(mdl.ext.e2,mdl.c);
					std::swap(mdl.ext.f2,mdl.f); //mdl.d
					std::swap(swap1,mdl.softanim_read_head);
					std::swap(swap2,mdl.softanim_read_tick);
					if(one_off)
					while(mdl.f<mdl.d)
					{
						//IMPLEMENT ME
						//TODO: these shouldn't be coupled
						//som_MPX_405de0 disables #3 if the running times differ
						mdl.f+=som_MDL_animate_soft(mdl,mdl.ext.anim_weights[1]);
					}
				}
			}
			else
			{			
				//((BYTE(__cdecl*)(void*))0x444410)(&mdl);
				mdl.f+=som_MDL_animate_soft(mdl);
			}
		}
		else if(mode&3) //hard
		{
			mdl.f++;

			//((BYTE(__cdecl*)(void*))0x445120)(&mdl);
			som_MDL_animate_hard(mdl);

			if(recover) if(mdl.f<=(mdl.e?1:2)) //YUCK
			{
				assert(&mdl==SOM::L.arm_MDL);
				for(int i=mdl->skeleton_size;i-->4;)
				memcpy(mdl.skeleton[i].uvw,mdl.skeleton[i]._uvw_scale_xyz2,sizeof(float)*9);
			}
			else recover = false;
		}
		else assert(0);
	}
	reverse_d:
	if(!uptodate)
	{
		if(mode&3)		
		som_MDL_transform_hard((DWORD*)&mdl);

		//2021: som_scene_swing needs to modify the skeleton 
		//after animation, before it's applied to a MDO mesh
		mdl.ext.mdo_uptodate = false;

		//CODE MOVED TO som_MDL_animate_post... 
	}
}
void SOM::MDO::data::ext_uv_uptodate()
{
	assert(ext.uv_tick);

	auto tick = SOM::motions.tick;

	//NOTE: uv_tick is set to 1 when initialized
	if(ext.uv_tick>=tick) return;

	auto diff = tick-ext.uv_tick;
	auto base = ext.uv_fmod;

	ext.uv_tick = tick; ext.uv_fmod+=diff;

	auto *els = chunks_ptr;		
	for(int i=chunk_count;i-->0;)
	if(auto*fx=els[i].extra().uv_fx)
	{
		float *uv = els[i].vb_ptr->uv;

		for(int j=0;j<2;j++,uv++)
		{
			float t = (&fx->tu)[j];
			if(!(t!=0)) continue; //NaN?

			//NOTE: when som_db adjusts UVs it wraps them to
			//stay in the range 0~1. this is both expensive
			//to compute and incorret if the initial values
			//lie outside this range. all this can be skipped
			//if it's safe to assume the GPU will somehow be
			//able to handle ever growing floating-point data
			//(I don't actually know if it can do that or not)
			if(1||!EX::debug)
			{
				float tt = fabsf(t);

				//JUST IN CASE?
				//NOTE: if t==1 fmod(1) is zero-divide below
				if(tt>1) t = fmodf(t,1);

				float l_t = 1/tt;

				auto tsteps = (unsigned)fabsf(1000*l_t);
				auto bsteps = base%tsteps;

				t*=diff/1000.0f; 
				if(bsteps+diff>=tsteps)
				t-=t>0?1:-1;
			}
			else t*=diff/1000.0f;

			for(int j=8*els[i].vertcount-8;j>=0;j-=8)
			{
				uv[j]+=t;
			}
		}
	}
}
static void som_MDL_animate_post(SOM::MDL &mdl)
{
	//WARNING: "mdo_uptodate" is covering legacy soft MDL too
	//(below) since it's appropriate that analogous logic is
	//implemented in the same location
	mdl.ext.mdo_uptodate = true;

	int mode = 7&*mdl->file_head;
	if(!mode) return;

	if(auto*mdo=mdl->ext.mdo) //MDO?
	{
		auto *els = mdo->chunks_ptr;			
		if(els->extradata)
		if(int elems=mdo->chunk_count)			
		for(int i=0;i<elems;i++)
		{
			auto se = mdl.ext.mdo_elements+i;

			//NOTE: som_MDL_447fc0_cpp has to dup vdata for
			//this to be feasible
			auto dst = se->vdata;
			//this preserves MDO (floating-point) precision
			auto base = els[i].vb_ptr;

			auto &ext = els[i].extra();

			if(mode&4)
			{
				//NOTE: som_MDL_440030 adjusts these indices to
				//index into anim_vertices
				if(WORD*index=ext.part_index)
				for(int j=els[i].vertcount;j-->0;dst++,base++)
				{
					auto src = mdl.anim_vertices[*index++];

					/*preserve MDO (floating-point) precision??
					for(int k=3;k-->0;) dst->pos[k] = src[k];*/
					for(int k=3;k-->0;) dst->pos[k] = base->pos[k]+src[k];
				}
				if(mode&3) //both?
				{
					base = dst = se->vdata; goto mixed;
				}
			}
			else mixed: if(se->tnl)
			{
				if(ext.part==0xff) continue; //PARANOID
					
				assert(mode&3);

				auto &m = mdl.skeleton[ext.part].xform;

				auto &m4x3 = Somvector::map<4,3>(m);
				auto &m3x3 = Somvector::map<3,3>(m);

				float mixed[3];
				for(int j=els[i].vertcount;j-->0;dst++,base++)
				{
					if(1) //having issues
					{
						//mixed is in case base==dst (mixed mode)
						//assuming it's just high level semantics
						Somvector::map(mixed).copy<3>(base->pos);
						Somvector::multiply<4>(mixed,m4x3,dst->pos);
						Somvector::map(mixed).copy<3>(base->lit);
						Somvector::multiply<3>(mixed,m3x3,dst->lit);
					}
					else //testing (works)
					{
						((void(*)(float*,float,float,float,float*,float*,float*))0x449ea0)
						(*m,base->pos[0],base->pos[1],base->pos[2],dst->pos+0,dst->pos+1,dst->pos+2);
					}
				}
			}
		}
	}
	else if(mode&4) //MDL
	{
		auto *el = mdl->elements;
		int elems = mdl->element_count;
		for(int i=0;i<elems;i++,el++)
		{
			//what could this be? 8 of something?
			auto se = mdl.elements[i].buf;
			//there's a branch here but both
			//have identical code 
			assert(3==(0xf&se->mode));
			//if(3==(0xf&&se->mode))
			for(int j=0,jN=el->vindex_count;j<jN;j++)
			{
				//I guess this is multicasting MDL vertices to
				//D3D formatted vertices
				int index = el->vindex_buf[j];
				auto dst = se->vdata+index;
				auto src = mdl.anim_vertices[el->soft_vindex_buf[index]];
				for(int k=3;k-->0;) dst->pos[k] = src[k];
			}			
		}
	}
}

//enum{ som_MDL_440f30_440ab0=1||EX::debug };
/*static void __cdecl som_MDL_449d50(FLOAT m[4][4], FLOAT x, FLOAT y, FLOAT z)
{
	//I'm thinking about repuposing this (scaling) function
	//to be able to implement arm_bicep for the second arm

	//NOTE: I think only one or two SFX files use this feature
	//receptacles use it to scale to 0,0,0 but do it to the wrong
	//animation channel since the old MDL files have twice as many
	//channels as necessary, so this scaling has no effect

	//0042E7A2 E8 79 1D 01 00       call        00440520
	//this makes 16 MDL instances of every SFX effect
	//
	//0000.MDL uses scaling (these all do)
	//0001.MDL almost identical to 0000.MDL
	//0066.MDL uses scaling, it's a brownish shockwave
	//most shockwaves are vertex animations
	//0090.MDL (looks funky without scaling)	

	if(EX::debug&&x!=1||y!=1||z!=1)
	{
		x = x; //breakpoint
	}

	if(1||!EX::debug)
	{
		m[0][0] = x; m[1][1] = y; m[2][2] = z; m[3][3] = 1;
	}
	else m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1; 
}*/
static void __cdecl som_MDL_449f70_scale(FLOAT m[4][4], FLOAT *sm, FLOAT *rtm)
{
	//TODO: 449f70 is not efficent for matrix multiplies
	//(it lets the the destination be among the factors)

	FLOAT *s = sm-(0x6c-0x14)/4;

	if(s[0]==1&&s[1]==1&&s[2]==1)
	{
		memcpy(m,rtm,sizeof(float)*16); return;
	}

	float m2[4][4] = {}; //identity?
	for(int i=3;i-->0;) m2[i][i] = s[i]; m2[3][3] = 1;

	//2021: the new arm models put 0,0,0 betweem the shoulders
	//instead of on the right side
	//if(m==SOM::L.arm_MDL->skeleton[7].xform)
	size_t j = ((size_t)m-(size_t)SOM::L.arm_MDL->skeleton)/272;
	if(j<=7)
	{
		//NOTE: how MDL scaling works would require the 
		//scaled parts to be centered on 0,0,0 in order
		//to work intuitively without compensating with
		//translation. it's not inherited, probably its
		//main use is explosions (SFX) and hidden faces

		auto &mdl = *SOM::L.arm_MDL;

		//SAME AS som_scene_swing
		//Moratheia's model has 5
		//trying to do shoulder animation in the MDL file
		bool legacy = false; switch(mdl->skeleton_size)
		{
		case 5: if(mdl->file_head[0]&8) break; //Moratheia?
		case 4: legacy = true; break;
		}
		if(/*j==7||*/!legacy) //legacy can't have two arms
		{
			/*if(legacy) //REMOVE ME
			{
				//in this case the right arm was 0,0,0
				//if(0xf0&som_MDL_449d20_swing_mask(0))
				m2[3][0] = (1-s[0])*0.4f*1024;
			}
			else*/ if(j==7||j==3)
			{
				//in this case the shoulders are 0.2 meters
				//to the left or right of 0,0,0
				//float x = (1-s[0])*0.2f*1024;
				float x = (1-s[0])*0.2f;
		//		if(!mdl.ext.mdo_elements) x*=1024;
				m2[3][0] = j==3?-x:x;
			}
		}
	}

	((void(__cdecl*)(void*,void*,void*))0x449f70)(m,m2,rtm);
}
extern BYTE __cdecl som_MDL_transform_hard(DWORD *mdl)
{
	if(0) return ((BYTE(__cdecl*)(void*))0x445410)(mdl);

	auto *head = *(SWORDOFMOONLIGHT::mdl::header_t**)(*mdl+12);

	if(!mdl||!*mdl||!*(int*)(*mdl+0x80))
	{
		assert(0); //return 0; //probably unnecessary
	}
	for(int it=0,n=*(int*)(*mdl+0x84);n-->0;it+=0x110)
	{
		auto x = mdl[0x12]+it; if(!*(char*)x)
		{
			//3 identity matrices are loaded here
			if(0&&*(DWORD*)(x+0x2c)==0x3F800000)
			{
				//assert(*(DWORD*)(x+0x2c)==0x3F800000);
				assert(*(DWORD*)(x+0x6c)==0x3F800000);
				assert(*(DWORD*)(x+0xac)==0x3F800000);
			}
			else //ARM.MDL needed this at one point???
			{
				((void(__cdecl*)(DWORD))0x449cf0)(x+0x2c);
				((void(__cdecl*)(DWORD))0x449cf0)(x+0x6c);
				((void(__cdecl*)(DWORD))0x449cf0)(x+0xac);
			}
		}
		else //similar to som_MDL_440f30_drawing (440ab0)
		{
			//SUBROUTINE
			//Note, this is considerably more effecient than 445410 
			//auto &Euler = *(float(*)[3])(mdl+4);
			float *r = (float*)(x+8);
			const float rad = 0.01745329f;			
			auto &m = *(float(*)[4][4])(x+0x2c);
			
			//NOTE: this differs from the usual convention
			float Euler[3] = {r[0]*rad,-r[1]*rad,r[2]*rad}; if(0) 
			{
				//TESTING (looking for correct combination)
				float q[4];
				Somvector::map(q).quaternion<1,2,3>(Euler);
				Somvector::map(m).copy_quaternion<3,4>(q);
			}
			else Somvector::map(m).rotation<3,4,'xyz'>(Euler);

			m[3][0] = *(float*)(x+0x20);
			m[3][1] = -*(float*)(x+0x24);
			m[3][2] = *(float*)(x+0x28);			
			m[3][3] = 1;
			/*som_MDL_reprogram knocks this out (NOP)
			//I think the plan was to repurpose this matrix
			for(int i=0x14;i<=0x1c;i+=4)
			if(*(DWORD)(x+i)!=0x3F800000) //1,1,1 usually
			{
				break; //UNFINISHED (scale matrix is x+0x6c)
			}*/
		}
	}
	for(int it=0,n=head->primchans;n-->0;it+=0x110)
	{
			//NOTE: som_MDL_449f70_scale modifies this behavior

		int x = mdl[0x12]+it; if(*(char*)x)
		{
			//TODO: 449f70 is not efficient for matrix multiplies
			//(it lets the destination be among the factors)

			auto &dst = *(float(*)[4][4])(x+0xac);
			auto &src = *(float(*)[4][4])(x+0x6c);
			auto &mul = *(float(*)[4][4])(x+0x2c);
			//if(som_MDL_440f30_440ab0)
			{
				//FIX ME
				//matrix multiply but extra logic is slipped in
				//here (could hoisted out to here)
				som_MDL_449f70_scale(dst,*src,*mul);
			}
			//else Somvector::multiply<4,4>(mul,src,dst);

			x = *(int*)(x+4);

			while(-1<x&&x<*(int*)(*mdl+0x84)) //skeleton_size
			{
				auto y = x*0x110+mdl[0x12]; //skeleton
				if(!*(char*)y) break;

				//Somvector::map(dst).premultiply?
				((void(__cdecl*)(void*,void*,DWORD))0x449f70)(*dst,*dst,y+0x2c);

				x = *(int*)(y+4);
			}
		}
	}
	return 1;
}
static void som_MDL_440ab0_transforms(SOM::MDL &mdl) //440ab0
{
	//SUBROUTINE
	//
	// TODO: 4416c0 duplicates this code
	//
	//Note, this is considerably more effecient than 440ab0 
	union{ float*r; DWORD*r2; };
	r = mdl.xyzuvw+3;
	auto &m = mdl.xform;
	//EXPERIMENTAL: do_aa needs precision to conceal 
	//secrets/traps. I don't know if this helps... maybe
	//the 1/1024 scale is the problem, or something else
	//sin/cos are said to be imprecise, but I don't know
	//if that's true for multiples of pi
	switch(r2[0]||r2[2]?1:r2[1])
	{
	case 0x3fc90fdb: //1.57079637 (via SOM_MAP/MapComp)
 
		//need to collect more of these
		memset(&m,0x00,sizeof(m[0])*3);
		m[0][2] = m[1][1] = 1; m[2][0] = -1;
		break;

	case 0x40490fdc: //3.14159298

		memset(&m,0x00,sizeof(m[0])*3);
		m[0][0] = m[2][2] = -1; m[1][1] = 1;
		break;

	case 0x4096cbe4: //4.71238899 (1.5 pi?) 

		memset(&m,0x00,sizeof(m[0])*3);
		m[0][2] = -1; m[1][1] = m[2][0] = 1;
		break;

	case 0: //helpful?
 
		memset(&m,0x00,sizeof(m[0])*3);
		m[0][0] = m[1][1] = m[2][2] = 1;
		break;

	default: r = r; //breakpoint
	case 1:

		float Euler[3] = {r[0],r[1],r[2]}; if(0)
		{
			//TESTING (looking for correct combination)
			//post-multiply -negated- in zxy order (449e40)
			float q[4];
			Somvector::map(q).quaternion<2,1,3>(Euler);
			Somvector::map(m).copy_quaternion<3,4>(q);
		}
		else Somvector::map(m).rotation<3,4,'yxz'>(Euler);
	}
	Somvector::map(m[3]) //translation
	.copy<3>(mdl.xyzuvw).se<3>() = 1;
	auto mdl2 = (DWORD*)&mdl;
	/*2021: instead MDL is kept in 1/1024 units the whole time
	//which is better for shaders/normals
	if(!mdl->ext.mdo)
	{
		const float l_1024 = 0.00097656f;
		float scale[3] = {l_1024,l_1024,l_1024}; //1/1024
		for(int i=7;i<=9;i++) if(mdl2[i]!=0x3F800000) //1,1,1 usually
		scale[i-7]*=((float*)mdl2)[i];
		for(int i=3;i-->0;)
		for(int j=3;j-->0;) m[i][j]*=scale[i];
	}
	else*/ for(int i=7;i<=9;i++) if(mdl2[i]!=0x3F800000) //1,1,1 usually
	{
		//REMINDER: includes giant/baby monsters, etc.
		for(int i=3;i-->0;)
		for(int j=3;j-->0;) m[i][j]*=mdl.scale[i]; break;
	}
}

//HACK: this is an expedient way to avoid doing even
//more plumbing on Obj/Ene/NpcEdit since the current
//code is already getting out of hand
//static som_scene_element *som_MDL_tool_elements = 0;
static std::vector<som_scene_element> som_MDL_tool_elements;

static void som_MDL_440ab0_unanimated(SOM::MDL &mdl, void *tes)
{
	//WARNING: NORMALLY 440ab0 CALLS 440f30 FIRST THING BUT
	//IT'S ROUTED THROUGH 440f30 BECAUSE IT'S TOO DIFFICULT
	//TO DISTURB 440ab0

	//2021: som_scene_swing needs to modify the skeleton 
	//after animation, before it's applied to a MDO mesh
	if(!mdl.ext.mdo_uptodate) 
	{
		//mdl.ext.mdo_uptodate = true;

		som_MDL_animate_post(mdl); assert(mdl.ext.mdo_uptodate);
	}

	som_MDL_440ab0_transforms(mdl); //broken out

	BYTE mode = 7&*mdl->file_head;
	
	enum //MDO
	{
		//I think 10000000 is ZWRITEENABLE and for 
		//the sky models it needs to be overridden
		//(currently skies aren't MDL+MDO enabled)
		bm_none =0x58002124, //src (1d1b070)
		bm_alpha=0x68002654, //src*alpha+(1-alpha)*dst (1d1b074)
		bm_add  =0x60002254, //src*alpha+dst (1d1b078)
	};

	const auto &m = mdl.xform;	
	auto mdl2 = (DWORD*)&mdl;
	bool faded,solid = mdl2[10]==0x3F800000;
	float fade = mdl.fade;
	if(faded=mdl2[10]!=mdl2[11])
	{
		if(0x3f800000==mdl2[10]&&SOM::tool) //TOOL?
		{
			//THIS PATH IS ONE-OFF FOR TOOLS (2021)
			
			//mdl2[11] seems to be stuck at 0xbf800000?
			faded = false; mdl2[10] = 0x3f800000;

			//REMINDER: this is done here since there's no
			//hook into when the MDL instance is generated
			if(mdl->ext.mdo)
			{
				//HACK: this will be caught below, it's the
				//first chance to access the model instance
				mdl.ext.mdo_elements = som_MDL_tool_elements.data();
			}
			else //2021: it's simpler to keep MDL in 1m units
			{
				//REMINDER: these are already posed
				for(auto i=mdl->element_count;i-->0;)
				{	
					auto &se = mdl.elements[i].se;
					auto v = se->vdata;
					for(int j=se->vcount;j-->0;v++)
					for(int k=3;k-->0;)
					{
						v->pos[k]*=0.00097656f; //1/1024
					}					
				}
				if(3&mode)
				for(auto i=mdl->skeleton_size;i-->0;)
				{
					auto &m2 = mdl.skeleton[i].xform;
					for(int k=3;k-->0;)
					m2[3][k]*=0.00097656f; //1/1024
				}
			} 
		}
		else if(auto*mats=mdl.ext.mdo_materials) //MDO?
		{
			if(EX::debug&&!mdl2[10]) //TESTING
			{
				//these may depend on getCaps somehow?
				auto test = (DWORD*)0x1d1b070;
				assert(bm_none==test[0]
				&&bm_alpha==test[1]&&bm_add==test[2]);
			}

			auto *mdo = mdl->ext.mdo;
			for(int i=mdo->material_count;i-->0;)
			{
				float alpha = mdo->materials_ptr[i][3];
				auto *d = SOM::L.materials[mats[i]].f+1;
				assert(d[4]==d[0]&&d[5]==d[1]&&d[6]==d[2]);
				d[3] = d[7] = fade*alpha;
			}
		}
		else for(int i=0;i<3;i++) //MDL?
		{
			static const float alpha[3] = {1,0.5f,0.25f};

			//440ab0 sets several of these but I think it's 
			//redundant. the first 3 must be. the second 4
			//I assume must be ambient material property
			auto *d = SOM::L.materials[mdl.materials[i]].f+1;
			assert(d[4]==d[0]&&d[5]==d[1]&&d[6]==d[2]);
			d[3] = d[7] = fade*alpha[i];
		}

		mdl2[11] = mdl2[10];
	}

	#if 1
	#define MDL_BACKWARDS
	#endif
	if(auto*se=mdl.ext.mdo_elements) //MDO?
	{
		auto *mdo = mdl->ext.mdo;

		#ifndef MDL_BACKWARDS
		#error see below case
		#endif
		int i = mdo->chunk_count;
		for(se+=i-1;i-->0;se--)
		{
			if(!se->icount) continue; //tnl?

			auto &el = mdo->chunks_ptr[i];
			
			if(el.extradata)
			{
				auto &ext = el.extra();

				//EXTENSION
				//NOTE: because of the enemy "recovery time"
				//stat (during which no animatin plays) this
				//can't be done in som_MDL_animate_post (and
				//probably other reasons too)
				if(mdo->ext.uv_tick)
				if(ext.uv_fx&&el.extrasize>=3)
				{
					//NOTE: could do this for every model every
					//frame, but only doing it in respond to 
					//drawing is less overhead
					mdo->ext_uv_uptodate();

					auto dst = se->vdata;			
					auto base = el.vb_ptr;
					for(int j=el.vertcount;j-->0;)
					{
						dst[j].uv[0] = base[j].uv[0];
						dst[j].uv[1] = base[j].uv[1];
					}
				}
			
				if(3&mode&&ext.part!=0xFF&&~se->tnl) //tnl?	
				{
					auto &m2 = mdl.skeleton[ext.part].xform;
					Somvector::multiply<4,4>(m2,m,se->worldxform);
				}
				else memcpy(se->worldxform,m,sizeof(m));
			}
			else memcpy(se->worldxform,m,sizeof(m));

			//UNNECESSARY?
			//see below comments (do_lights?)
			memcpy(se->lightselector,mdl.xyzuvw,sizeof(FLOAT)*3);

			//REMOVE ME?
			//TODO? probably this can be done
			//only if faded in the above code
			DWORD blend_mode; if(el.blendmode)
			{
				blend_mode = bm_add;
			}
			//NOTE: SOM::L.materials is off limits for tools
			//NOTE: SOM::L.materials[mdl.ext.mdo_materials[i]].f[1+3] will
			//be 1.0 if this is false
			else if(fade!=1.0f||1.0f!=mdo->materials_ptr[el.matnumber][3])
			{
				blend_mode = bm_alpha;
			}
			else blend_mode = bm_none;

			se->flags = blend_mode;
			if(bm_none==blend_mode) //opaque?
			{
				//((BYTE(__cdecl*)(void*))0x44d810)(se);
				som_MDL_x44d810(se);
			}
			else //transparent?
			{
				extern void som_scene_push_back(void*,void*);
				som_scene_push_back(tes,se);
			}
		}
	}
	else //MDL?
	{
		//I rewrote this to see if reversing it can help
		//D3DCMP_LESSEQUAL with secret doors, traps, etc.
		//(to draw the stationary part last)
		auto *el = mdl->elements;	
			#ifndef MDL_BACKWARDS //forward?
			//the parts seem to include materials, so to
			//go in reverse changes the materials' order
		int elems = mdl->element_count;
		for(int i=0;i<elems;i++,el++)
			#else //reverse?
		int i = mdl->element_count;
		for(el+=i-1;i-->0;el--)
			#endif
		{
			auto se = mdl.elements[i].buf; //som_scene_element

			int blend_mode = mdl.elements[i].data->blend_mode;

			//TESTING: textures are interleaved per pc
			//if(EX::debug&&se->texture==0x13) continue;

			if(3&mode) //hard body?
			{
				auto &m2 = mdl.skeleton[el->body_part].xform;
				Somvector::multiply<4,4>(m2,m,se->worldxform);
			}
			else memcpy(se->worldxform,m,sizeof(m));

			//UNNECESSARY? (do_lights/do_fix_lighting_dropout)
			//som_scene_alit is overwriting this... especially
			//in batch mode that's having trouble with it, but
			//this should run before the selector is overridden
			memcpy(se->lightselector,mdl.xyzuvw,sizeof(FLOAT)*3);

			if(faded)
			{
				assert(15!=(se->mode&4)); //som_scene_swing?

				int l = 1; if(3!=(0xf&se->mode))
				{
					//NOTE: THIS PATH ONLY APPLIES TO SPRITES

					//note, there's a 4 case switch statement
					//here, but it looks like every block is 
					//identical except for the float constant
					if((unsigned)blend_mode<4)
					if(se->icount)
					{
						float x = 255; switch(blend_mode) //04583BC
						{
						case 0: x = *(float*)0x458618; break; //127.5
						case 3: x = *(float*)0x458614; break; //63.75
						}
						unsigned a = (int)(x*fade)<<0x18;					
						int n = se->vcount;
						auto *q = (unsigned int*)(se->vdata)+4; 
						for(int i=0;i<n;i++)
						{
							*q = *q&0xFFFFFF|a; q+=8; //alpha
						}					
					}

					if(-1==blend_mode) two:
					{
						//seems to be encoding blend modes
						int a = solid?2:5;
						int b = solid?1:6;
						//2021: doing this manually to avoid dependency on
						//som_db.exe (plus better that way)
						/*REFERENCE
						((void(__cdecl*)(DWORD*,
						int,int,int,int,int,int,int,int))0x44d4d0)
						(&se->flags,4,a,b,l,solid,!solid,1,1);*/
						DWORD f = 4|a<<4|b<<8|l<<12;							
						//f|=solid<<28|!solid<<29;
						f|=solid?0x10000000:0x20000000; //(zwrite or alphablend)
						se->flags = f|0x40000000|0x8000000; //1,1 (colorkey,?)
					}
				}
				else if(-1==blend_mode)
				{
					//add lightselector??
				//	se->mode|=0x80000000; l = 2; goto two;
					se->lit = 1; l = 2; goto two;
				}
			}

			if(-1==blend_mode&&solid) //opaque?
			{
				//((BYTE(__cdecl*)(void*))0x44d810)(se);				
				som_MDL_x44d810(se);
			}
			else //transparent?
			{
				extern void som_scene_push_back(void*,void*);
				som_scene_push_back(tes,se);
			}
		}
	}

	//0x440ab0 returns AL but no call sites check for it

	//return 1; //mov AL,1 
}
extern void som_MDL_440f30(SOM::MDL &mdl)
{
	BYTE mode = 7&*mdl->file_head;

	//bool beginning = mdl.f>mdl.d;
	bool beginning = mdl.f<=1||mdl.d<mdl.f;

	//TESTING A THEORY
	//
	// I think maybe only the first animation
	// is meant to have two dummy frames but
	// the other animations skip their first
	// frame since they're meant to have one
	// dummy frame
	//
	// NOTE: skeleton attacks look bad but the
	// glitch seems unrelated (its idle & walk
	// animation doesn't match the others)
	//
	// NOTE: most obj MDLs use this additional
	// frame, but some (the tall doors) do not
	// it seems like artists were mostly under
	// the impression the first frame is shown
	//
	//DWORD swap = mdl.d;
	if(mdl.d==1)
	if(mdl.c!=0)
	{
		if(mdl.e!=mdl.c)
		if(mode&3)
		if(&mdl!=SOM::L.arm_MDL)
		{
			mdl.e = mdl.c;
			mdl.f = -1;
			mdl.d = 0;
		}
	}
	//bool done = false; //if(EX::debug) //2021
	{
		if(mdl._mystery_mode[0])
		{
			//REMINDER: som_MDL_4418b0 sets this
			//when there's not a CP file present

			//assert(0); //_mystery_mode?
			assert(!mdl->cp_file);

			//I want to not allocate soft_vertices
			//at 44097b 
		}
		//done = som_MDL_animate(mdl);
		som_MDL_animate(mdl);
	}
	/*if(!done) 
	{
		((void(__cdecl*)(void*))0x440f30)(&mdl);
	}*/
	//mdl.d = swap;

	/*interferes with som.scene.cpp swing
	if(beginning&&!SOM::stereo)
	if(&mdl==SOM::L.arm_MDL) //EXTENSION
	{
		DWORD m = som_MDL_449d20_swing_mask(0);

		float s = EX::INI::Player()->arm_bicep; //0.75
		float s2 = EX::INI::Player()->arm_bicep2;

		for(int pass=1;pass<=2;pass++)
		{
			int pc = 3;
			if(pass==2) if(m&0xF0)
			{
				pc+=4;
			}
			else continue;

			if(mode&3)
			{
				auto bicep = mdl.skeleton[pc].scale;
				bicep[0] = s;
				bicep[1] = s2;
				bicep[2] = s;
			}
			else assert(0);
		}
		
		//HACK: refresh matrix		
		if(som_MDL_440f30_440ab0)
		som_MDL_transform_hard((DWORD*)&mdl);
		else ((BYTE(__cdecl*)(void*))0x445410)(&mdl);
	}*/
}
static void __cdecl som_MDL_440f30_drawing(SOM::MDL &mdl, DWORD x440ab0)
{
	som_MDL_440f30(mdl);

	//if(som_MDL_440f30_440ab0) //drawing? (som.scene.cpp)
	{
		//ATTENTION: x440ab0 is not a real parameter, it
		//just happens to be next available on the stack
		som_MDL_440ab0_unanimated(mdl,(void*)x440ab0);
	}
}
static void __cdecl som_MDL_440f30_loading(SOM::MDL &mdl)
{
		BYTE mode = 7&*mdl->file_head; //2021

	//2021: it's simpler to keep MDL in 1m units
	for(auto i=mdl->element_count;i-->0;)
	{
		auto &se = mdl.elements[i].se;
		auto v = se->vdata;
		for(int j=se->vcount;j-->0;v++)
		for(int k=3;k-->0;)
		{
			v->pos[k]*=0.00097656f; //1/1024
		}
	}
		if(!mode) return; //2021

	namespace Sm = SWORDOFMOONLIGHT::mdl;
	Sm::image_t img; Sm::maptoram(img,mdl->file_data,mdl->file_size);
	Sm::animation_t ani[32]; 
	//unsigned rt[64]; //som_MPX_405de0
	unsigned anis = Sm::animations(img,ani,64);
	for(unsigned i=0;i<anis;i++)
	{
		unsigned id = ani[i]->id;
		mdl.ext.anim_mask|=1<<id;
		//if(id<64) rt[id] = mdl.running_time(i); 
	}

	if(4&mode) //soft body?
	{
		//soft objects begin their life using
		//the first interpolant instead of the
		//base geometry. maybe if the frame is
		//defined (e.g. duplicated) this won't
		//happen (I don't remember) however a
		//fix isn't possible unless the entire
		//animation stepping code is rewritten
		// 
		// NOTE: it has been rewritten... this
		// poses a problem with round-tripping
		// x2mdl
		// 
		//if(mdl.e==-1&&mdl.d==1)
		{
			//assert(mdl.e==-1&&mdl.d==1);
			assert(mdl.e==-1&&mdl.d<=1); //2023??? high frame rate?

			//NEW: limit to animations that don't
			//have an establishing frame
			if((unsigned)mdl.c>=anis||1!=ani[mdl.c]->frames[0].time)
			{
				mdl.d = 0;
			}
		}
	}

	//TESTING A THEORY THERE IS A BUG
	//
	// I think maybe only the first animation
	// is meant to have two dummy frames but
	// the other animations skip their first
	// frame since they're meant to have one
	// dummy frame
	//
	// NOTE: skeleton attacks look bad but the
	// glitch seems unrelated (its idle & walk
	// animation doesn't match the others)
	//
	// NOTE: most obj MDLs use this additional
	// frame, but some (the tall doors) do not
	// it seems like artists were mostly under
	// the impression the first frame is shown
	//
	//DWORD swap = mdl.d;
	if(mdl.d==1)
	if(mdl.c!=0)
	if(mdl.e!=mdl.c)
	if(3&mode) //hard body?
	{
		mdl.e = mdl.c;
		mdl.f = -1;
		mdl.d = 0;
	}
	/*2021: not safe any longer with MDO files
	//maybe not without either!
	//NOTE: this was setting mapped/parent one
	//time instead of testing mapped each time
	((void(__cdecl*)(void*))0x440f30)(&mdl);*/
	som_MDL_440f30(mdl); //CORRECT?
	//mdl.d = swap;
}

//TEMPORARY
//TODO: multiply by 3 for 90hz, etc.
enum{ som_MDL_fps=2 }; 

extern float som_MDL_arm_fps = 0.06f;

//THESE EXPAND THE ANIMATIONS TO 60FPS
//WITHOUT REWRITING THE ANIMATION CODE
static int __cdecl som_MDL_44fbfb(FILE *f)
{
	size_t len = som_game_ftell(f);

	//pass on ARM.MDL because historically it advanced two frames
	//per world frame (first call is kage.mdl, second is arm.mdl)
	if(!SOM::field) 
	{
		//HACK: tell som_game_60fps to adjust SOM::L.item_pr2_file
		if(!SOM::arm[0])
		{
			SOM::arm[0] = 60; som_MDL_arm_fps*=2; 
		}
	}

	swordofmoonlight_mdl_header_t hd;

	som_game_fseek(f,0,SEEK_SET);
	som_game_fread(&hd,sizeof(hd),1,f);

	//modern? this needs to be sure that 4 is the only
	//primitive pack, which it should be if converted by
	//the cache system
	if(~hd.animflags&8)
	{
		//disable optimizations (need the header to decide)
	
		assert(SOM::game); //0X442650 BELOW IS ASSUMING som_db.exe

		som_MDL_x45f350[4] = (void*)0x442650; //som_db.exe?!
	}

	//OPTIMIZING MEMORY #1
	if(som_MDL_x45f350[4]==som_MDL_4)
	{
		assert(SOM::game);

		size_t timstart = hd.primchanwords
		+hd.hardanimwords+hd.softanimwords+4;

		if(timstart*4<len)
		{
			hd.timblocks = 0;
			len = timstart*4;
		}
		else assert(0);
	}

	return len+(4*hd.hardanimwords)*(som_MDL_fps-1);
}
static int __cdecl som_MDL_44fb13(BYTE *pp, int, int len, FILE *f)
{
	int eof = som_game_fread(pp,1,len,f);

	namespace mdl = SWORDOFMOONLIGHT::mdl;

	//OPTIMIZING MEMORY #2
	if(som_MDL_x45f350[4]==som_MDL_4)	
	if(eof>=sizeof(mdl::header_t))
	{
		auto &hd = *(mdl::header_t*)pp;

		size_t timstart = hd.primchanwords
		+hd.hardanimwords+hd.softanimwords+4;

		if((size_t)eof>=timstart*4)
		{
			eof = timstart*4;
		}
	}
	else assert(0);

	mdl::image_t img; mdl::maptoram(img,pp,eof);
	
	auto &hd = mdl::imageheader(img);
	
	//NOTE: n will be 0 if the data is invalid
	mdl::animation_t anis[32];
	int n = mdl::animations(img,anis,EX_ARRAYSIZEOF(anis));
	if(!n) return len; //UNUSED

	union
	{
		uint8_t *p; uint16_t *p2;
	};
	p = (BYTE*)anis[0];

	int channels = 0;
	
	const int mode = hd.animflags&0x7;
	const int modern = hd.animflags&8; //UNUSED

	if(int add=len-eof) //som_MDL_44fbfb
	{
		p-=4; //animation header

		channels = min(255,*(uint16_t*)p);

		if(add!=(som_MDL_fps-1)*4*hd.hardanimwords)
		{
			assert(0); return len; //UNUSED
		}				
		hd.hardanimwords*=som_MDL_fps;
	
		memmove(p+add,p,eof-(p-pp));

		for(int i=n;i-->0;) (BYTE*&)anis[i]+=add;

		p+=4; //animation header
	}
	else if(4!=mode)
	{
		//this is for ARM.MDL because som.game.cpp
		//is knocking out the code that made it to
		//advance two animation frames every world
		//frame
		return len; //UNUSED
	}

	mdl::hardanim_t *ha = 0; int ha2 = 0;

	//2021: I was just allocating this for scaling
	//but it seems like rotations require wrapping
	if(channels)
	{
		ha = new mdl::hardanim_t[channels];
		ha2 = mdl::mapanimationchannels(img,ha,channels);
	}

	for(int i=0;i<n;i++) if(4==mode) //soft?
	{		
		int jN = mdl::softanimframestrlen(anis[i]->frames);
		for(int j=0;j<jN;j++)
		{
			auto &t = anis[i]->frames[j].time;

			if(j||t>1) t*=som_MDL_fps; 
		}
	}
	else if(3&mode) //hard?	
	{
		union
		{
			uint8_t *q; uint16_t *q2;
		};
		q = (BYTE*)anis[i];

		*p2++ = *q2++; *p2++ = *q2++; //id/time

		int time = p2[-1]; 

		//there are actually two dummy frames for
		//some reason, that makes things obnoxiously
		//complicated
		//
		//DOORS SPECIFIC?
		//skipping 2 is required to make doors that came
		//with SOM (2000) initialize to the right position
		//however, once opened they appear to behave fine
		//suggesting there's a bug in the original logic
		//
		//skipping 2 is what the animation subroutine does
		//
		//however skipping 1 for subsequent animations for
		//doors appears to make the closing animation look
		//more correct when it reaches its stopping point
		//
		//I don't understand why, but I can't see any visual
		//issues when not skipping 2 except for how doors
		//intialize, suggesting that once things or activated
		//it doesn't matter, which may be evidence of a buggy
		//system
		//
		//the animation code makes an exception for the special
		//data in the first frame of the first animation, just 
		//like this code, which is an odd choice. I don't know
		//if maybe skipping two is really because that isn't an
		//actual animation frame, but the animation code always
		//skips two for every animation, so it doesn't seem that
		//way. and from what I can tell every animation uses its
		//own first frame and not the frame with the extra data
		//
		//NOTE: 2==som_MDL_skip overrides this behavior. I think
		//it's probably more correct, but the door closing thing
		//seems to happen more when skipping 2, but I see it on
		//occassion with 1 too, maybe triple-buffering is dropping
		//frames? I will know more after I can take a look at the
		//models in my new modeling software. I think the animation
		//routines could be schizophrenic
		//
		//enum{ skip = 2 };
		int skip = i==0?min(time,2):1; //som_MDL_skip
		
		p2[-1] = time>=skip?skip+(time-skip)*som_MDL_fps:0;

		//bool scaling = false;

		//2021: I was just allocating this for scaling
		//but it seems like rotations require wrapping
		for(int j=0;j<skip;j++)
		{
			mdl::animate(anis[i],ha,channels,j,i==0);
		}
		
		//NOTE: mdl::animations does bound-checking against words
		BYTE *b,*d;
		for(int t=0;t<time;t++,q=b,p=d)
		{	
			auto words = *q2++; 
			auto diffs = *q2++;

			b = q-4+4*words;
			
			//first frame is special and shouldn't be duplicated
			//and the first animation holds an extra parent byte
			if(t<skip)
			{
				q-=4; memcpy(p,q,b-q); 
				
				d = p+(b-q); continue;
			}

			for(int channel=channels;channel-->0;)
			memcpy(ha[channel].ct,ha[channel].cv,6); //REPURPOSING

			auto *qq = q;			
			for(int x=som_MDL_fps;x-->0;q=qq,p=d)
			{
				for(int channel=channels;channel-->0;)
				memcpy(ha[channel].cv,ha[channel].ct,6); //REPURPOSING

				d = p+4*words;

				*p2++ = words;
				*p2++ = diffs;				

				for(int j=0;j<diffs;j++) 
				{
					//unsigned so right shift will 0 pad
					unsigned int channel  = *p++ = *q++;
					unsigned int diffmask = *p++ = *q++; 
					unsigned int bitsmask = *p++ = *q++; 				
				
					//REMINDER: 445120 seems to include some
					//scaling capability by 3 bytes that can
					//range from 0 to 1.9921875 and override
					//rather than accumulate. the top 2 bits
					//in diffmask mark two of the dimensions
					//the 3rd is marked by bit 7 in bitsmask
					for(int k=0;k<3+3;k++) if(diffmask&1<<k)
					{
						if(~bitsmask&1<<k)
						{
							auto n = *(int16_t*)q;

							if(k<3) //2021
							{
								int a = ha[channel].cv[k];
								int b = a+n;

								//shortest path?
								int c = (b-a+2048)%4096-2048;
								if(c<-2048) c+=4096;

								assert(abs(c)<=2048);

								n = (int16_t)c; goto k_3;
							}
							else if(abs(n)<512) k_3: //1/2 meter?
							{
								*(int16_t*)p = n/som_MDL_fps;
								if(!x) *(int16_t*)p+=n%som_MDL_fps;
							}
							else //teleporting? (per component)
							{
								//REMINDER: I originally set this up for the
								//pedestals that start their animation by 
								//jumping the hidden key item from its hiding
								//place inside their base

								/*this order was showing out of sequence with 
								//KF2 dagger test heavy attack (how is it moving
								//so far so fast? Euler discontinuity?)
								//I really don't see why it should make a difference
								//I think there may be more to it?
								*(int16_t*)p = x?0:n;*/ //warp? e.g. receptacle
								*(int16_t*)p = x?n:0;
							}
							p+=2; q+=2;

							if(k<3) ha[channel].cv[k]+=n;
						}
						else
						{
							auto n = *(int8_t*)q;
							*(int8_t*)p = n/som_MDL_fps;
							if(!x) *(int8_t*)p+=n%som_MDL_fps;
							p++; q++;

							if(k<3) ha[channel].cv[k]+=n;
						}
					}	
					if(diffmask=diffmask>>6|bitsmask>>4&4) //scaling?
					{
						//UNSOLVED MYSTERY
						//WARNING key receptacles set to 0,0,0 and 
						//never go back to 1,1,1. this is set into
						//its transform but it remains visible for
						//reason I can't explain. data breakpoints
						//show that the tranform remains 0,0,0 for
						//the opening animation and even afterward
						//(2021: I know now it's just a mistake in
						//the model)

						/*if(!scaling) //initialize scaling state?
						{
							scaling = true; assert(ha);
							if(ha2) for(int j=0;j<skip;j++) mdl::animate(anis[i],ha,channels,j,i==0);
							else for(int j=channels;j-->0;) memset(ha[j].cs,128,4); //???
						}*/
						BYTE *cmp = ha[channel].cs;

						for(int k=0;k<3;k++) if(diffmask&1<<k)
						{
							BYTE sx = *q++; if(x&&cmp[k]!=sx&&som_MDL_fps>1)
							{
								if(cmp[k]&&sx) //2022: don't interpolate hiding
								{
									if(som_MDL_fps>2) //for example
									sx = cmp[k]+float(sx-cmp[k])/som_MDL_fps*(som_MDL_fps-x);
									else sx = cmp[k]+(sx-cmp[k])/2;
								}
							}
							else cmp[k] = sx; *p++ = sx;
						}
					}
					assert(~bitsmask&0x80);
				}
				assert(q<=b);
				assert(x||p<=d-2); //...\

				//I think this might be used to run the animation
				//backward, but anyway, Swordofmoonlight.cpp does
				//this and it's been tested on every original MDL
				//
				// NOTE: mdl::animations expects this to be 32bit
				// with the word count in the low bit. all extant
				// MDL files are this way
				//
				((uint32_t*)d)[-1] = words;
			}			
		}
	}
	delete[] ha; return len; //UNUSED
}
//MDO: this filters out primitives when loading MDL with
//MDO files. 440030 subroutines can use it to detect if
//MDO files are in play
static void som_MDL_4(SOM::MDL::data*, DWORD, BYTE **_3) //RENAME?
{
	//2021: this skips textured triangle primitive packs
	//to speed up loading. currently x2mdl only knows #4
	*_3 = (*_3+4)+*(WORD*)(*_3+2)*24;
}
static BYTE __cdecl som_MDL_4447a0(SOM::MDL::data *l) //som_MDL_4?
{
	if(som_MDL_x45f350[4]==som_MDL_4) //OPTIMIZING
	{
		//HACK: this (TIM allocation) is called before
		//vertex/normal memory is allocated according
		//to these members
		l->vertex_count = l->normal_count = 0; 
		
		return 0; //som_MDL_4447a0...
	}
	return ((BYTE(__cdecl*)(void*))0x4447a0)(l); //load TIM data?
}
static BYTE __cdecl som_MDL_4441a0(SOM::MDL::data *l) //som_MDL_4?
{
	if(som_MDL_x45f350[4]==som_MDL_4) //OPTIMIZING 
	{
		//continuing from som_MDL_4447a0? 
		#ifdef _DEBUG
		assert(!l->vertex_count&&!l->normal_count);
		//YUCK: malloc(0) returns nonzero
		som_MDL_free(l->vertex_buf); //free
		som_MDL_free(l->normal_buf); //free
		l->vertex_buf = l->normal_buf = 0;
		#endif
		//vertex_count is needed to reset soft animation
		{		
			DWORD channels = l->file_head[4];
			auto *pcs = (SOM::MDL::part*)(l->file_head+16);			
			for(int i=channels;i-->0;)
			{
				l->vertex_count+=pcs[i].vertcount;
				l->normal_count+=pcs[i].normcount;
			}
		}
		return 0;
	}
	return ((BYTE(__cdecl*)(void*))0x4441a0)(l); //fill buffers?
}
namespace som_MPX_swap //2022
{
	extern bool models_type(void*);
	extern void*&models_ins(char*);
	extern SOM::Texture*&images_ins(char*);
}
extern char *som_MDL_skin = 0;
extern BYTE *som_MDL_skin2 = 0;
extern wchar_t *som_art_CreateFile_w; 
extern int som_art(const wchar_t*, HWND);
extern int som_art_model(const char*, wchar_t[MAX_PATH]);
static void som_MDO_init_ext(SOM::MDO::data *o)
{
	bool bsp = false;

	if(o) for(int i=o->chunk_count;i-->0;)
	{
		auto &el = o->chunks_ptr[i];

		if(el.blendmode) bsp = true;

		//NOTE: MDL+MDO is done elsewhere
		if(el.extrasize<=2) return;

		auto &ext = el.extra();

		if(ext.uv_fx) if(SOM::game)
		{
			assert(el.extrasize>=3);

			if(!o->ext.uv_tick)			
			o->ext.uv_tick = max(1,SOM::motions.tick);

			(void*&)ext.uv_fx = o->file+(DWORD)ext.uv_fx;
		}
		else ext.uv_fx = 0;
	}
	
	/*#ifdef NDEBUG 
	#error remove me
	#endif
	if(SOM::game)
	if(0&&EX::debug) if(o) //TESTING //2022
	{
		for(int i=o->material_count;i-->0;)
		if(o->materials_ptr[i][3]!=1.0f) bsp = true;
		extern som_BSP *som_bsp_ext(som_MDO::data&);
		if(bsp) o->ext.bsp = som_bsp_ext(*o);
	}*/
}
static SOM::MHM *som_MDL_mhm(char *a, char *ext) //MHM?
{
	memcpy(ext,".mhm",5);
	FILE *f = fopen(a,"rb");
	extern som_MHM *som_MHM_417630(FILE*,bool);
	auto *h = f?som_MHM_417630(f,false):0;
	if(f) fclose(f); return h;
}
extern SOM::MDO::data *__cdecl som_MDO_445660(char *a) //MDO?
{
	assert(!som_art_CreateFile_w); //_0?

	//2022: share ref counted models between maps
	void **ins = 0;
	if(SOM::game&&*(ins=&som_MPX_swap::models_ins(a)))
	return (SOM::MDO::data*)*ins;

	wchar_t art[MAX_PATH]; 	
	int e = som_art_model(a,art); //2021
	using namespace x2mdl_h;
	if(e&_art&&~e&_lnk)	
	if(!som_art(art,0)) //x2mdl exit code?
	{
		e = som_art_model(a,art); //retry?
	}
	if(0==(e&_mdo)) return 0; //2021 //_bp|_mdl
	
	som_art_CreateFile_w = art;

	auto *ext = PathFindExtensionA(a); strcpy(ext,".mdo");

	auto ret = som_MDO_x445660(a); som_MDO_init_ext(ret);

	if(e&_mhm&&SOM::game) //2022
	{
		(*ret).ext.mhm = som_MDL_mhm(a,ext);
	}

	som_art_CreateFile_w = 0; 
	
	if(ins&&(*ins=ret)) ret->ext.refs = 1; return ret;
}
extern DWORD __cdecl som_BMP_4485e0(char *a, int _2, int _3, int _4)
{
	//2022: this is just three BMP cases that aren't
	//inside critical sections to prevent data races
	extern SOM::Thread *som_MPX_thread;
	EX::section raii(som_MPX_thread->cs);
	//go ahead and skip the BMP/TXR extensions test?
	//return ((DWORD(__cdecl*)(char*,int,int,int))0x4485e0)(a,_2,_3,_4);
	return ((DWORD(__cdecl*)(char*,int,int,int))0x449350)(a,_2,_3,_4);
}
extern DWORD __cdecl som_TXR_4485e0(char *a, int _2, int _3, int _4)
{
	//NOTE: this subroutine technically can load BMP
	//files, however it shouldn't be installed where
	//BMP files are used

	if(som_MDL_skin) a = som_MDL_skin;
	
	assert(*a); //MPX or MSM dummy?

	//this replaces 448fd0's linear lookup and stops
	//opening shortcut (LNK) files for art detection
	SOM::Texture **ins = 0; 
	if(SOM::game&&*(ins=&som_MPX_swap::images_ins(a)))
	return *ins-SOM::L.textures;

	wchar_t art[MAX_PATH]; 	
	int e = som_art_model(a,art); //2021
	using namespace x2mdl_h;
	if(e&_art&&~e&_lnk)	
	if(!som_art(art,0)) //x2mdl exit code?
	{
		e = som_art_model(a,art); //retry?
	}
	if(0==(e&_txr)) //return 0xffffFFFF; //2022
	{
		if(*(DWORD*)(a+5)==0x5C70616D) //map/model?
		{
			//HACK: don't overwrite som.MPX.cpp's 
			//map.textures strings, which need to
			//be the same for images_ins to match 
			char aa[64];
			memcpy(aa,a,8);
			memcpy(aa+8,"\\texture",8);
			memcpy(aa+16,a+14,32);
			if(EX::data(aa+5,art)) e|=_txr; //2022
		}
		if(0==(e&_txr))
		{
			return 0xffffFFFF;
		}
	}
	////////////////////////////
	/////POINT-OF-NO-RETURN/////
	////////////////////////////
	//HACK: this is used to load files from
	//nonstandard locations
	auto swap = som_art_CreateFile_w;
	som_art_CreateFile_w = art;

	DWORD ret = som_TXR_x448fd0(a,_2,_3,_4); //x4485e0

	som_art_CreateFile_w = swap; //POINT-OF-NO-RETURN

	if(_4==16) //skin?
	if((unsigned)(SOM::tool-EneEdit.exe)>1)
	for(int i=som_MDL_tool_elements.size();i-->0;)
	som_MDL_tool_elements[i].texture = ret;

	if(ins&&ret!=0xffffFFFF)
	*ins = SOM::L.textures+ret; return ret;
}
static char *som_MDL_sfx_types = 0;
static void som_MDL_42e5c0_SFX_art(char *a, DWORD esi)
{
	BYTE &sfx_type = *(BYTE*)(0x1CDCD38+esi);

	char *ext = PathFindExtensionA(a);

	//2022: avoid touching shortcut (LNK) files, etc.
	//NOTE: 1CDCD38 holds this data but it's cleared
	//on MPX change
	if(!som_MDL_sfx_types)	
	memset(som_MDL_sfx_types=new char[256],0xff,256);
	unsigned i = atoi(ext-4);		
	char &cached = som_MDL_sfx_types[i];
	switch(i<256?cached:-2)
	{
	default: assert(0); //break;
	case -1: break;
	case 2: memcpy(ext,".txr",5); //break;
	case 1: 
	case 0: sfx_type = cached; return;
	}

	//NOTE: this is just determining the type
	//but there's no real way to know without
	//converting it
	wchar_t art[MAX_PATH]; 	
	int e = som_art_model(a,art); //2021
	using namespace x2mdl_h;
	if(e&_art&&~e&_lnk)	
	if(!som_art(art,0)) //x2mdl exit code?
	{
		e = som_art_model(a,art); //retry?
	}
	if(e&(_mdo|_mdl))
	{
		sfx_type = 1;
	}
	else if(e&_txr)
	{
		sfx_type = 2; memcpy(ext,".txr",5);
	}
	else sfx_type = 0;

	if(i<256) cached = sfx_type;
}
static SOM::MDL::data *som_MDL_440030_sub(char*,int);
extern void *som_MDL_401300_maybe_mdo(char *a, int *ee)
{	
	using namespace x2mdl_h;

	//2022: share ref counted models between maps
	void* &ins = som_MPX_swap::models_ins(a); 
	int e; if(ins)
	{
		if(ee)
		e = som_MPX_swap::models_type(ins)?_mdo:_mdl;
	}
	else
	{
		//NOTE: this is just determining the type
		//but there's no real way to know without
		//converting it
		wchar_t art[MAX_PATH]; 	
		e = som_art_model(a,art); //2021
		//using namespace x2mdl_h;
		if(e&_art&&~e&_lnk)	
		if(!som_art(art,0)) //x2mdl exit code?
		{
			e = som_art_model(a,art); //retry?
		}
		if(!e) goto ee; //return?

		char *ext = PathFindExtensionA(a);

		memcpy(ext,e&_mdl?".mdl":".mdo",5);
	
		//TODO? might avoid som_art_model if ins
		//is nonzero, but it would need to check
		//for both extensions
		som_art_CreateFile_w = art; 

		if(e&_mdl)
		{
			if(auto*mdl=som_MDL_440030_sub(a,e))
			{
				ins = mdl; mdl->ext.refs = 1;
			}
		}
		else if(auto*mdo=som_MDO_x445660(a))
		{
			som_MDO_init_ext(mdo);

			ins = mdo; mdo->ext.refs = 1;

			if(e&_mhm&&SOM::game) //2022
			{
				mdo->ext.mhm = som_MDL_mhm(a,ext);
			}
		}

		som_art_CreateFile_w = 0;
	}

ee: if(ee) *ee = e; return ins;
}
static void som_MDL_401300_obj_art(char *a, DWORD ebp)
{
	int e; void *ins = som_MDL_401300_maybe_mdo(a,&e);

	if(e&x2mdl_h::_mdl)
	{
		SOM::L.obj_MDL_files[ebp] = (BYTE*)ins;
	}
	else SOM::L.obj_MDO_files[ebp] = (BYTE*)ins;
}
static SOM::MDO::data *__cdecl som_MDO_445660_substitute(char *a) //MDO?
{
	if(SOM::game) 
	{
		auto *ret = som_MDO_x445660(a); som_MDO_init_ext(ret);

		return ret;
	}

	HANDLE h = CreateFileA(a,SOM_GAME_READ);
	if(h==INVALID_HANDLE_VALUE) return 0;

	SOM::MDO::data *o = 0;
	DWORD sz = GetFileSize(h,0);
	if(sz!=INVALID_FILE_SIZE)
	{			
		auto *p = (char*)som_MDL_malloc(sz);
		ReadFile(h,p,sz,&sz,0);
		namespace mdo = SWORDOFMOONLIGHT::mdo; 
		mdo::image_t img; mdo::maptorom(img,p,sz);		
		auto &t = mdo::textures(img);
		auto &m = mdo::materials(img);
		auto &c = mdo::channels(img);
		//NOTE: SOM would not even check these boundaries
		//TODO? test vertex bounds
		if(!img.bad)
		{	
			(void*&)o = som_MDL_malloc(sizeof(SOM::MDO::data));

			DWORD i = t.count, *q = (DWORD*)som_MDL_malloc(4*i);

			o->file = p;
			o->textures = q;
			o->texture_count = i;
			o->material_count = m.count;
			(void*&)o->materials_ptr = m.list;
			(void*&)o->cpoints_ptr = mdo::controlpoints(img);
			o->chunk_count = c.count;
			(void*&)o->chunks_ptr = c.list;

			char *d, path[MAX_PATH];
			int cat; if(d=strrchr(a,'\\'))
			{
				cat = d-a+1; memcpy(path,a,cat);
			}
			else cat = 0; d = path+cat; assert(cat);
			
			int len,dot;
			for(p=(char*)t.refs;i-->0;q++,p+=len+1)
			{
				*q = 0xffffffff;
				len = strlen(p);
				for(dot=len;dot-->0&&p[dot]!='.';); //!
				dot+=1;
				if(cat+dot+4<=MAX_PATH)
				{
					memcpy(d,p,dot); memcpy(d+dot,"txr",4);

					//for skins the last 0 is 0x16, but Ghidra gets this
					//parameter wrong (it assigns it to memory 148B away
					//from the stack address) where its disassembly says
					//it's used (it's not used there) so I don't know???
					*q = som_TXR_4485e0(path,0,0,0);
				}
			}
			
			//HACK: the real (eventual?) way to do this is to fill out
			//ext.mdo_elements and ext.mdo_materials but I don't want
			//to go that extra distance just yet.. how I'm having to 
			//map every subroutine this way is not how I'd like to do
			//this but is expedient for the moment
			//
			// NOTE: som_MDL_440ab0_unanimated is doing further setup
			// the first time it sees the model since there's no load
			// hook on the instance for the tools
			//
			i = c.count;
			p = o->file;
			//YUCK: the new model is loaded before the old
			//deleted, so som_MDL_4403f0_free can't do this
			auto se = som_MDL_tool_elements.data();
			for(int i=som_MDL_tool_elements.size();i-->0;)
			som_MDL_x448100(se[i].material);
			//(void*&)se = realloc(se,i*sizeof(*se));
			//som_MDL_tool_elements = se;
			som_MDL_tool_elements.resize(i);
			se = som_MDL_tool_elements.data();
			auto *cp = o->chunks_ptr;			
			for(;i-->0;se++,cp++)
			{
				se->mode = 0x80000003;
				if(cp->texnumber<t.count)
				se->texture = o->textures[cp->texnumber];
				else se->texture = 0xffff;
				//NOTE: SOM doesn't do a bound check here
				if(cp->matnumber<m.count)
				{
					auto *f = (float*)&m[cp->matnumber];
					se->material = som_MDL_x447ff0(f[0],f[1],f[2],f[3],f[4],f[5],f[6]);
				}
				else se->material = 0; //???
				se->vcount = cp->vertcount;
				se->icount = cp->ndexcount;
				(void*&)se->vdata = p+cp->vertblock;
				(void*&)se->idata = p+cp->ndexblock;
			}
		}
		else som_MDL_free(p);
	}
	CloseHandle(h); som_MDO_init_ext(o); return o;
}
extern SOM::MDL::data *__cdecl som_MDL_440030(char *a) //MDL+MDO?
{
	//2022: share ref counted models between maps
	void **ins = 0;
	if(SOM::game&&*(ins=&som_MPX_swap::models_ins(a)))
	return (SOM::MDL::data*)*ins;

	wchar_t art[MAX_PATH]; 	
	int e = som_art_model(a,art); //2021
	using namespace x2mdl_h;
	if(e&_art&&~e&_lnk)	
	if(!som_art(art,0)) //x2mdl exit code?
	{
		e = som_art_model(a,art); //retry?
	}
	//if(!e) return 0;

	if(0==(e&(_mdo|_bp|_mdl)))
	{
		if(SOM::tool>=EneEdit.exe)
		{
			//return a dummy model so the profile memory
			//pointer is nonzero?

			if(a[strlen(a)-1]!='\\') //not a new profile
			{
				//SfxEdit's model input system is just a
				//number field that requires inputting a
				//partial number to use it
				if(SOM::tool!=SfxEdit.exe)
				{
					//simulate the error message
					DWORD x = SOM::tool==NpcEdit.exe?0x4038f0:0x403910;				
					((void(*)(DWORD,DWORD))x)(0x6f,0x30);
				}
			}					
		}
		else return 0; //2021
	}

	////////////////////////////
	/////POINT-OF-NO-RETURN/////
	////////////////////////////
	//HACK: this is used to load files from
	//nonstandard locations
	som_art_CreateFile_w = art; 
	
	auto ret = som_MDL_440030_sub(a,e); 
	
	if(ins&&(*ins=ret)) ret->ext.refs = 1;

	assert(!som_art_CreateFile_w); return ret;
}
static SOM::MDL::data *som_MDL_440030_sub(char *a, int e) //MDL+MDO?
{
	using namespace x2mdl_h;
	char *ext = PathFindExtensionA(a);
	char *fmt = 0;
	if(e&(_bp|_mdo)) 	
	if(SOM::game) 
	fmt = e&_bp?".bp":".mdo";
	else fmt = e&_mdo?".mdo":".bp"; //BP??
	if(fmt) memcpy(ext,fmt,5);
	auto o = fmt&&som_MDL_with_mdo?som_MDO_445660_substitute(a):0;
	//REMINDER: art models can be anything
	memcpy(ext,".mdl",5);

	if(!SOM::game||~e&_mdl) //workshop.cpp? kage.mdl?
	{
		if(e&_txr&&SOM::tool==SfxEdit.exe)
		{
			e|=_mdl; //SfxEdit_CreateFile_TXR?
		}

		//WIP: workshop animation is unimplemented
		auto *l = !o&&e&_mdl?som_MDL_x440030(a):0;
		
		if(!l) //dummy MDL record
		{
			(void*&)l = som_MDL_malloc(sizeof(*l));
			memset(l,0x00,sizeof(*l));
			//file_head can't be 0. note that file_data is the
			//pointer that will be deleted after
			l->file_head = (BYTE*)&l->file_data;
			//this is always set to 1... it will crash if not
			//
			// 2022: I'm not so sure about that, but I recall
			// it happening
			//
			//l->_tim_atlas_mode = 1;
			l->_tim_atlas_mode = 0;
		}

		som_art_CreateFile_w = 0; //POINT-OF-NO-RETURN

		l->ext.mdo = o; return l; //TODO: som_MDL_reprogram me
	}
	assert(SOM::game); //0X442650 BELOW IS ASSUMING som_db.exe

	//som_MDL_4 is a marker and filters out textured triangles
	//(x2mdl output) to improve load times and conserve memory	
	//
	// NOTE: som_MDL_44fbfb disables this
	if(o) som_MDL_x45f350[4] = som_MDL_4;
	else if(som_MDL_skin2) *som_MDL_skin2 = 1; //Moratheia?
	auto l = som_MDL_x440030(a);
	if(o) som_MDL_x45f350[4] = (void*)0x442650; //som_db.exe?!

	som_art_CreateFile_w = 0; //POINT-OF-NO-RETURN

	if(!l) return 0;

	if(l->ext.mdo=o)
	{
		//UNFINISHED		
		bool okay = true;		
		DWORD channels = l->file_head[4];
		auto *pcs = (SOM::MDL::part*)(l->file_head+16);	
		for(int i=o->chunk_count;i-->0;)
		{
			auto &el = o->chunks_ptr[i];

			if(el.extradata) 
			{
				auto &ext = el.extra();

				DWORD pt = ext.part;

				//UNFINISHED
				//
				// have to handle incompatible (accidental) combos
				//				
				if(pt!=0xff&&pt>=channels)
				{
					okay = false; assert(0);
				}
				else if(ext.part_verts!=pcs[pt].vertcount)
				{
					okay = false; assert(0);
				}
				else assert(el.extrasize<=3); //2
			}
		}
		for(int i=o->chunk_count;i-->0;)
		{
			auto &el = o->chunks_ptr[i];

			if(!okay) //UNFINISHED
			{
				//NOTE: if there is a mismatch the model
				//will be displayed as an unanimated MDO

				el.extradata = el.extrasize = 0;
			}
			else if(el.extradata) 
			{
				auto &ext = el.extra();

				DWORD pt = ext.part;

				if(pt!=0xff)				
				if(auto&reloc=(DWORD&)ext.part_index) 
				{
					reloc+=(DWORD)o->file; //offset->pointer?

					//HACK: this just makes things easier
					if(WORD vs=l->primitive_buf[pt].vstart)
					{
						WORD *p = ext.part_index;
						for(int j=el.vertcount;j-->0;p++)
						*p+=vs;
					}
				}
				else assert(0);
			}
		}
	}
	else //2021: it's simpler to keep MDL in 1m units
	{
		//NOTE: som_scene_elements are done by
		//som_MDL_440f30_loading
		float *v = l->vertex_buf[0];
		for(auto i=3*l->vertex_count;i-->0;)
		v[i]*=0.00097656f; //1/1024
	}

	if(e&_mhm) l->ext.mhm = som_MDL_mhm(a,ext); //2022

	return l;
}
static DWORD __cdecl som_MDL_447fc0_cpp(SOM::MDL &mdl) //MDO?
{
	//this kills 2 birds with 1 stone by initializing
	//the MDO instance data, and prevents allocating
	//3 unused materials instances for the MDL data
	//(447fc0 allocates materials)

	if(auto*mdo=mdl->ext.mdo)
	{
		if(!mdl.ext.mdo_materials) //447fc0 is called 3 times
		{
			//REMOVE ME
			//this avoids writing code to fill these structures
			//until I'm certain how this works
			auto tmp = ((SOM::MDO*(__cdecl*)(void*))0x4458d0)(mdo);
		//	{
				//NOTE: som_MDL_4409d0_free frees/deletes this
				//memory

				mdl.ext.mdo_materials = tmp->materials;
		auto ses = mdl.ext.mdo_elements = tmp->elements;
		//	}
			som_MDL_free(tmp); //free

			//YUCK: som_scene_element needs its own buffer
			//unless the animation data were to be written
			//directly into the vertex buffer (which would
			//have to be done when it's drawn and not just
			//when it's updated)
			size_t sz = 0;
			for(int i=mdo->chunk_count;i-->0;)
			sz+=ses[i].vcount;
			//NOTE: this puts all memory in a contiguous
			//buffer that can be drawn at once if indices
			//are contiguous in the file (which they are
			//if generated by x2mdl circa Sep/Aug. 2021)
			auto *p = new float[8*sz]+8*sz;
			for(int i=mdo->chunk_count;i-->0;)
			{
				auto *se = ses+i;
				sz = 8*se->vcount;
				(void*&)se->vdata =
				memcpy(p-=sz,se->vdata,sz*sizeof(float));
			}
			
			//TODO: detect skinning?
			//TODO: Ex.ini opt out (for performance testing?)
			//(with vbuffer control extensions... [Vertex]??)
			//
			// I don't think this breaks lightselector since
			// it's using mdl.xyzuvw
			// 
			//extern bool som_scene_alit;
			//if(som_scene_alit) //merge?
			if(SOM::field) //arm.mdl needs to hide elements
				#ifdef NDEBUG
		//		#error fix me (animation isn't working???)
				#endif				
			if(0) //if(0||!EX::debug)
			{
				//TODO: figure this out
				DWORD vb_cap = som_scene::vbuffer_size;
				if(!vb_cap) vb_cap = 4096;

				bool tnl = false;
				DWORD rebase = 0;
				int n = mdo->chunk_count;
				if(n-->=2) for(int i=0;i<n;i++)
				{
					auto &se = ses[i];
					auto &se2 = ses[i+1];						
					se2.flags = 0; //HACK
					if(se2.texture==se.texture
					&&se2.material==se.material					
					&&se2.idata==se.idata+se.icount
					&&mdo->chunks_ptr[i].blendmode
					==mdo->chunks_ptr[i+1].blendmode)
					{
						tnl = true;

						if(rebase==0)
						{
							rebase = se.vcount;
						}
						if(rebase+se2.vcount<=vb_cap)
						{
							rebase+=se2.vcount;

							//HACK: just marking in this pass
							//(this is much simpler)
							se2.flags = 1; 
						}
						else rebase = 0;
					}
					else rebase = 0;
				}
				//this way even the last chunk if not merged
				//will be pretransformed to be consistent. if
				//skinning it has to be anyway
				if(tnl)
				{
					som_scene_element *merger = ses;

					for(int i=0,m=mdo->chunk_count;i<m;i++)
					{
						auto &se = ses[i];

						se.tnl = 1; //tnl?

						if(se.flags) //marked?
						{
							se.flags = 0; //unnecessary

							n = se.icount;
							se.icount = 0;
							merger->icount+=n;
							rebase = merger->vcount;
							merger->vcount+=se.vcount;
							se.vcount = 0;							
							//TRICKY: p is shared by all
							//instances, so do this once
							auto *p = se.idata;
							if(p[0]<rebase)
							while(n-->0) p[n]+=rebase;
						}
						else merger = &se;
					}
				}
			}
			else assert(SOM::game);
		}
		return 0;
	}
	return 0x447fc0; //...
}
__declspec(naked)
static DWORD __cdecl som_MDL_447fc0() //MDO?
{
	//NOTE: __asm is just to extract EBP... maybe it
	//would be better to try to find it on the stack
	__asm
	{
		//RELEASE BUILD FAILURE
		//
		// with optimizations pop ebp comes out two
		// dwords further back than esp post return
		// I assume it's a bug since it breaks call
		// convention. /Od fixes it
		// 
		// some 2010 "hotfixes" describe this but I
		// can't find download links
		// https://support.microsoft.com/en-us/topic/fix-incorrect-machine-code-generation-by-the-visual-c-compiler-for-certain-bit-field-member-operations-in-visual-studio-2010-7563a4a2-64a5-3cc4-cf16-2708f8bd6556
		// my C2.dll files are older than these are
		// 
		// NOTE: the error is in som_MDL_447fc0_cpp
		// not this __asm block
		//
		push ebp
		call som_MDL_447fc0_cpp //C++
		pop ebp
		xor ecx,ecx //zero
		cmp eax,ecx
		//jnz eax //doesn't work
		jz MDO
		jmp eax //forward call to 447fc0
	MDO:dec eax //0 would be removed by 4409d0 on unload
		ret //required
	}
}
extern void __cdecl som_MDO_445870(void *o) //2022
{
	//NOTE: SOM::Game.item_lock would be useful here but 
	//it's hard to erase items from som_MPX_swap::models
	//without a key, so instead som.game.cpp's menus are
	//calling som_MPX_swap::models_free

	int bp = o?((SOM::MDO::data*)o)->ext.refs:0; //DEBUGGER

	if(o) ((SOM::MDO::data*)o)->ext.refs--; //wait until map change?
}
extern void __cdecl som_MDL_4403F0(void *l) //2022
{
	if(l) ((SOM::MDL::data*)l)->ext.refs--; //wait until map change?
}
static void __cdecl som_MDL_4403f0_free(SOM::MDL::data *l) //MDO?
{
	if(auto*o=l->ext.mdo) if(SOM::tool)
	{
		//this is done after loading the model
		//for(int i=o->material_count;i-->0;)
		//som_MDL_x448100(som_MDL_tool_elements[i].material);
		for(int i=o->texture_count;i-->0;)			
		som_MDL_x448780(o->textures[i]);
		som_MDL_free(o->textures);
		som_MDL_free(o->file);
		som_MDL_free(o);			
	}
	else if(SOM::game)
	{
		assert(0); //UNUSED (models_free)

		((BYTE(__cdecl*)(void*))0x445870)(o);
	}

	som_MDL_free(l); //free
}
static void __cdecl som_MDL_4409d0_free(SOM::MDL *l) //MDO?
{
	if(l->ext.mdo_elements) //445ad0 (DUPLICATE)
	{
		//NOTE: som_MDL_447fc0_cpp is allocating this memory
		for(int i=l->mdl_data->ext.mdo->material_count;i-->0;)
		((BYTE(__cdecl*)(DWORD))0x448100)(l->ext.mdo_materials[i]);
		som_MDL_free(l->ext.mdo_materials); //free
		if(l->mdl_data->ext.mdo->chunk_count)
		delete[] l->ext.mdo_elements[0].vdata;
		som_MDL_free(l->ext.mdo_elements); //free
	}

	assert(SOM::game);

	som_MDL_free(l); //free
}
static BYTE __cdecl som_MDO_445ad0_free(SOM::MDO *o) //MDO?
{
	assert(SOM::game);

	/*#ifdef NDEBUG 
	#error remove me
	#endif
	if(void*p=o->mdo_data()->ext.bsp) //2022
	{
		delete[] (char*)p; //som_bsp_ext

		//HACK: just sprinkling around
		extern void som_bsp_ext_clear();
		som_bsp_ext_clear();
	}*/

	return ((BYTE(__cdecl*)(void*))0x445ad0)(o);
}

extern BYTE __cdecl som_MDL_4416c0(SOM::MDL &mdl)
{
	if(!&mdl) //happens??? (42b19c calling)
	{
		return 0; //breakpoint
	}

	//this is the first byte of a MDL file
	const int mode = *mdl->file_head;	

	//rigid animations have an additional binding
	//round for some reason
	INT32 skip = mode&3&&0==mdl.c?2:1; //0xe

	INT32 d = mdl.d, swap = d; //halving CP index 
	assert(swap<0xFFFF);
	if(d>skip)
	{
		//TODO: technically these need to be averaged
		d = (d-skip)/som_MDL_fps+skip;
	}	
	
	if(som_MDL_hz)
	d = (DWORD)(d*SOM::motions.l_hz_30); //2023

	BYTE ret = ((BYTE(__cdecl*)(void*))0x4416c0)(&mdl);
	mdl.d = swap; return ret;
}
extern BYTE __cdecl som_MDL_4418b0(SOM::MDL &mdl, float *cp, DWORD _)
{
	//ATTENTION
	//
	// 2021: THIS IS A BAD PRACTICE BECAUSE
	// TURNING ON 0x15 PUTS THE MDL INTO AN
	// ALTERNATIVE ANIMATION MODE WHICH MAY
	// NOT BE USED AND IS POORLY UNDERSTOOD
	// 
	// TODO: generate CP its data at runtime 
	//
	//4418b0 crashes with a CP file. this is
	//a way to make it return 0,0,0
	if(!mdl->cp_file)
	{
		//this byte is tested to be 0 before 
		//getting control point data. why is
		//it unset?
		//2021: because it's a _mystery_mode
		//interpolate animation mode!
		//mdl[0x15]|=1;
		mdl._mystery_mode[0] = 1; //ABUSING!
	}
		
	//this is the first byte of a MDL file
	const int mode = *mdl->file_head;

	//rigid animations have an additional binding
	//round for some reason
	DWORD skip = mode&3&&0==mdl.c?2:1;
	
	//som_db only passes -1 but som_MDL_animate_soft
	//passes _+1 to fix a bug
	DWORD d = _==~0u?mdl.d:_;

	if(som_MDL_hz)
	d = (DWORD)(d*SOM::motions.l_hz_30); //2023

	if(SOM::game) //tool?
	if(EX::INI::Bugfix()->do_fix_animation_sample_rate)
	{
		if(d>skip) //TESTING 60FPS
		{
			_ = (d-skip)/som_MDL_fps+skip;
		}	
		else _ = skip;

		BYTE ret = som_MDL_x4418b0(&mdl,cp,_);

		/*the returned value is relative to the next CP position
		if(ret&&mdl[0xd]%som_MDL_fps==0)
		{
			float cp2[3]; memcpy(cp2,cp,sizeof(cp2));

			ret = som_MDL_x4418b0(&mdl,cp,_+1);

			for(int i=3;i-->0;) (cp[i]+=cp2[i])/=som_MDL_fps; assert(ret);
		}*/

		float x = 1.0f/som_MDL_fps;

		if(som_MDL_hz) x*=SOM::motions.l_hz_30;

		for(int i=3;i-->0;) cp[i]*=x;

		return ret;
	}
	else if(d<=skip) //2022
	{
		//HACK: 4418b0 crashes around 441914 when passed -1
		//because it hard codes 0x88 and empty CP files end
		//by that point. I really don't understand how this
		//ever worked, other than I may have never tried to
		//turn off do_fix_animation_sample_rate with such a
		//file, so this duplicates the logic above, forcing
		//the animations length to be considered
		_ = skip;
	}

	return som_MDL_x4418b0(&mdl,cp,_);
}
static void __cdecl som_MDL_43a350(SOM::Struct<126> *sfx) //1c9dd38 table
{
	auto &dat = SOM::L.SFX_dat_file[sfx->s[1]];

	int &c = sfx->i[0x2c/4], swap = c;

	//som_game_60fps doubles the lifespan so that c/2 is able to work
	//if(swap>1) c = c/2*2; else c = 2;
	if(swap>1) c = c/2; //else assert(c);

	DWORD f; switch((BYTE)dat.c[0])
	{
	case 132: f = 0x43a350; break; //flame
	case 41: f = 0x4370e0; break; //lightning strike
	}
	((void(__cdecl*)(void*))f)(sfx);

	if(swap) c = swap-1;
}

static DWORD som_MDL_444920_edge_flags_ext = 0;
static DWORD __cdecl som_MDL_442130_inner_loop(DWORD ebp, DWORD *ecx, DWORD *_edx, DWORD edi, DWORD *esp)
{
	DWORD&_5 = esp[0x28/4]; //4421ac //u component
	DWORD&_6 = esp[0x2C/4]; //4421b0 //v component
	DWORD&nx = esp[0x1C/4]; //4421E2
	DWORD&ny = esp[0x20/4]; //4421F0
	DWORD&nz = esp[0x24/4]; //4421FE

	//EXTENSION
	//see swordofmoonlight_tmd_fields_t
	//mdl_edge_flags_ext notes
	if(int ef=som_MDL_444920_edge_flags_ext)
	{
		som_MDL_444920_edge_flags_ext>>=2;

		if(ef&1) _5 = 0x3F800000; //1.0
		if(ef&2) _6 = 0x3F800000; //1.0
	}

	//ib is the MDL vertex lookup table... instead
	//of comparing positions compare to it so that
	//vertices won't be fused
	WORD *ib = (WORD*)_edx[8];
	DWORD _ebx; for(_ebx=0;_ebx<edi;_ebx++) 	
	{
		if(ebp!=ib[_ebx]) continue;
		DWORD *cmp = ecx+_ebx*8;
		if(cmp[6]==_5&&cmp[7]==_6)
		if(cmp[3]==nx&&cmp[4]==ny&&cmp[5]==nz)
		break;
	}
	//"return ebx" isn't generating an instruction
	//in the VS2010 compiler, so do it manually???
	__asm mov edx,_edx;
	__asm mov eax,_ebx; return ebx; //NOP???
}

static void __cdecl som_MDL_413770(DWORD fps)
{
	/*if(1||!EX::debug) //EXPERIMENTAL
	{
		((void(__cdecl*)(DWORD))0x413770)(fps); //sky //0.025 per second (1/40)
	}*/

	//2022: this animates the second sky
	for(int pass=1;pass<=2;pass++)	
	if(pass==1||(SOM::mpx2_blend&&SOM::L.corridor->fade[1]==18))
	if(auto*o=SOM::mpx_defs(pass==1?SOM::mpx:SOM::mpx2).sky_instance())
	{
		//2022: this ignores new MDO files with capacity
		//for explicit UV animations
		auto *d = o->mdo_data();
		if(d->chunks_ptr[0].extrasize<3)
		((void(__cdecl*)(void*,int,float,float))0x445b40)(d,0,0,fps*0.000025f); //0.025
	}
}
static void __cdecl som_MDL_445cd0(SOM::MDO *o) //updating MDO
{
	auto *d = o->mdo_data();
	if(d->ext.uv_tick) d->ext_uv_uptodate();

	((void(__cdecl*)(void*))0x445cd0)(o);
}
static void __cdecl som_MDL_445cd0_init(SOM::MDO *o) //initialize MDO
{
	//2022: set sort bit and assign som_BSP pointers?
	extern void som_bsp_make_mdo_instance(SOM::MDO*);
	som_bsp_make_mdo_instance(o);
	
	som_MDL_445cd0(o);
}
static void *__cdecl som_MDL_42f7a0(DWORD _1, DWORD _2) //initialize SFX
{
	void *ret = ((void*(__cdecl*)(DWORD,DWORD))0x42f7a0)(_1,_2);

	((SOM::sfx_element*)ret)->sort = 1; return ret;
}

static int __cdecl som_MDL_441430(SOM::MDL &mdl, int c)
{
	return mdl.running_time(c);
}

extern void som_MDL_reprogram() //som.state.cpp
{
	DWORD cmp = 0; //THERE IS A BUG IN THIS API

	switch(SOM::tool) //YUCK
	{
		//NOTE: "4418b0" retrieves the current base CP delta

	case ItemEdit.exe:

		//som.art.cpp (MDO path)
		//004069B5 E8 E6 AD FF FF       call        004017A0
		*(DWORD*)0x4069B6 = (DWORD)som_MDO_445660-0x4069Ba;
		//defeat MDO extension test
		//0040699E 68 50 62 42 00       push        426250h
		memset((void*)0x40699E,0x90,18);

		//som.art.cpp (TXR path)
		//00401915 E8 D6 20 00 00       call        004039F0
		*(DWORD*)0x401916 = (DWORD)som_TXR_4485e0-0x40191a;

		(DWORD&)som_MDO_x445660 = 0x4017A0; //load MDO (art)
		(DWORD&)som_TXR_x448fd0 = 0x4039f0; //load_TXR_etc (art) 
		break;

	case ObjEdit.exe:

		//00405199 E8 22 D9 FF FF       call        00402AC0
		*(DWORD*)0x40519a = (DWORD)som_MDL_4418b0-0x40519e;
		cmp = 0x402AC0;

		//2021: MDL+MDO (limited)
		//0040B515 E8 36 62 FF FF       call        00401750		
		*(DWORD*)0x40B516 = (DWORD)som_MDL_440030-0x40B51a;		
		//00401769 68 B8 09 00 00       push        9B8h
		*(DWORD*)0x40176a = sizeof(SOM::MDL::data);
		//00401c2f F3 AB                rep stos    dword ptr es:[edi] 
		//00401c31 E8 F6 9D 01 00       call        0041ba2c 
		*(WORD*)0x401c2f = 0x9090;
		*(DWORD*)0x401c32 = (DWORD)som_MDL_4403f0_free-0x401c36;
		//0040B15E E8 3D 70 FF FF       call        004021A0		
		*(DWORD*)0x40B15f = (DWORD)som_MDL_440ab0_unanimated-0x40B163;

		//som.art.cpp (MDO path)		
		//hack: force down MDL path just to cover extensions
		//other than MDL/MDO
		//0040b553 E8 78 AE FF FF       call        004063d0
		//*(DWORD*)0x40b554 = (DWORD)som_MDO_445660-0x40b558;
		//0040B50E 74 28                je          0040B538
		*(WORD*)0x40B50E = 0x9090;

		//som.art.cpp (TXR path)
		//00406545 E8 76 21 00 00       call        004086C0
		*(DWORD*)0x406546 = (DWORD)som_TXR_4485e0-0x40654a;

		(DWORD&)som_MDL_malloc = 0x41ba03; //malloc?
		(DWORD&)som_MDL_free = 0x41ba2c; //free?
		(DWORD&)som_MDL_x4418b0 = 0x402AC0; //get CP delta
		(DWORD&)som_MDL_x440030 = 0x401750; //load_MDL_file
		(DWORD&)som_MDL_x44d810 = 0x40c360; //draw_scene_element
		(DWORD&)som_TXR_x448fd0 = 0x4086c0; //load_TXR_etc (art) 
		(DWORD&)som_MDL_x448780 = 0x408860; //unload_TXR
		(DWORD&)som_MDL_x447ff0 = 0x4080f0; //reserve_material
		(DWORD&)som_MDL_x448100 = 0x408200; //unload_material
		(DWORD&)som_MDO_x445660 = 0x4063d0; //load MDO (art)
		(DWORD&)som_MDL_x45f350 = 0x42b0b0; //som_MDL_4
		break;
		
	case EneEdit.exe: case SfxEdit.exe: //same EXE code wise

		//00407FD9 E8 F2 D4 FF FF       call        004054D0
		*(DWORD*)0x407FDa = (DWORD)som_MDL_4418b0-0x407FDe;
		cmp = 0x4054D0;

		//2021: MDL+MDO (limited)
		//0040326A E8 21 07 00 00       call        00403990 
		//00403781 E8 0A 02 00 00       call        00403990 //skin???		
		*(DWORD*)0x40326b = (DWORD)som_MDL_440030-0x40326f;
		*(DWORD*)0x403782 = (DWORD)som_MDL_440030-0x403786;
		//004039a9 68 B8 09 00 00       push        9B8h
		*(DWORD*)0x4039aa = sizeof(SOM::MDL::data);
		//00403E6F F3 AB                rep stos    dword ptr es:[edi] 
		//00403E71 E8 12 90 00 00       call        0040CE88 
		*(WORD*)0x403E6F = 0x9090;
		*(DWORD*)0x403E72 = (DWORD)som_MDL_4403f0_free-0x403E76;
		//00402FBD E8 FE 13 00 00       call        004043C0
		*(DWORD*)0x402FBe = (DWORD)som_MDL_440ab0_unanimated-0x402Fc2;

		//som.art.cpp (TXR path)
		//00403373 E8 08 77 00 00       call        0040AA80
		//0040383D E8 3E 72 00 00       call        0040AA80
		*(DWORD*)0x403374 = (DWORD)som_TXR_4485e0-0x403378;
		*(DWORD*)0x40383e = (DWORD)som_TXR_4485e0-0x403842;

		(DWORD&)som_MDL_malloc = 0x40CE93; //operator new?
		(DWORD&)som_MDL_free = 0x40ce88; //operator delete?
		(DWORD&)som_MDL_x4418b0 = 0x4054D0; //get CP delta
		(DWORD&)som_MDL_x440030 = 0x403990; //load_MDL_file
		(DWORD&)som_MDL_x44d810 = 0x40c4d0; //draw_scene_element
		(DWORD&)som_TXR_x448fd0 = 0x40aa80; //load_TXR_etc (art)
		(DWORD&)som_MDL_x448780 = 0x40ac20; //unload_TXR
		(DWORD&)som_MDL_x447ff0 = 0x40a530; //reserve_material
		(DWORD&)som_MDL_x448100 = 0x40a620; //unload_material
		(DWORD&)som_MDL_x45f350 = 0x4161a8; //som_MDL_4
		break;

	case NpcEdit.exe:
		
		//00407FB9 E8 F2 D4 FF FF       call        004054B0 
		*(DWORD*)0x407FBa = (DWORD)som_MDL_4418b0-0x407FBe;
		cmp = 0x4054B0;

		//2021: MDL+MDO (limited)
		//0040324A E8 21 07 00 00       call        00403970
		//00403761 E8 0A 02 00 00       call        00403970 //skin???
		*(DWORD*)0x40324b = (DWORD)som_MDL_440030-0x40324f;
		*(DWORD*)0x403762 = (DWORD)som_MDL_440030-0x403766;
		//00403989 68 B8 09 00 00       push        9B8h
		*(DWORD*)0x40398a = sizeof(SOM::MDL::data);
		//00403e4f F3 AB                rep stos    dword ptr es:[edi] 
		//00403e51 E8 12 90 00 00       call        0040ce68 
		*(WORD*)0x403e4f = 0x9090;
		*(DWORD*)0x403e52 = (DWORD)som_MDL_4403f0_free-0x403e56;
		//00402FA9 E8 F2 13 00 00       call        004043A0
		*(DWORD*)0x402FAa = (DWORD)som_MDL_440ab0_unanimated-0x402FAe;

		//som.art.cpp (TXR path)
		//00403353 E8 08 77 00 00       call        0040AA60
		//0040381D E8 3E 72 00 00       call        0040AA60
		*(DWORD*)0x403354 = (DWORD)som_TXR_4485e0-0x403358;
		*(DWORD*)0x40381e = (DWORD)som_TXR_4485e0-0x403822;

		(DWORD&)som_MDL_malloc = 0x40CE73; //operator new?
		(DWORD&)som_MDL_free = 0x40CE68; //operator delete?
		(DWORD&)som_MDL_x4418b0 = 0x4054B0; //get CP delta
		(DWORD&)som_MDL_x440030 = 0x403970; //load_MDL_file
		(DWORD&)som_MDL_x44d810 = 0x40c4b0; //draw_scene_element
		(DWORD&)som_TXR_x448fd0 = 0x40aa60; //load_TXR_etc (art)
		(DWORD&)som_MDL_x448780 = 0x40ac00; //unload_TXR
		(DWORD&)som_MDL_x447ff0 = 0x40a510; //reserve_material
		(DWORD&)som_MDL_x448100 = 0x40a600; //unload_material
		(DWORD&)som_MDL_x45f350 = 0x41619c; //som_MDL_4
		break;
	
	case 0:
	
		//seems like a mistake, causing soft animations to show the first
		//frame when in an inanimate state
		//00440FC7 C7 45 3C 00 00 00 00 mov         dword ptr [ebp+3Ch],0
		//*(DWORD*)(0x440FC7+3) = 1;

		//optimizing?
		//don't waste memory on SOM::MDL::soft_vertices
		//0044097B 74 36                je          004409B3
		*(BYTE*)0x44097B = 0xeb; //jmp

		//intializing?
		//2021: send unanimated MDLs to som_MDL_440f30_loading?
		//004408F2 0F 84 CC 00 00 00    je          004409C4 
		*(BYTE*)0x4408F4-=17;
		//004409BC E8 6F 05 00 00       call        00440F30
		*(DWORD*)0x4409BD = (DWORD)som_MDL_440f30_loading-0x4409C1;
		//drawing call site?
		//00440AFA E8 31 04 00 00       call        00440F30		
		//*(DWORD*)0x440AFB = (DWORD)som_MDL_440f30_drawing-0x440AFF;
		//if(som_MDL_440f30_440ab0)
		{
			//004410F9 E8 12 43 00 00       call        00445410
			//00440F94 E8 77 44 00 00       call        00445410
			*(DWORD*)0x4410Fa = (DWORD)som_MDL_transform_hard-0x4410Fe;
			*(DWORD*)0x440F95 = (DWORD)som_MDL_transform_hard-0x440F99;
			//EXPERIMENTAL (scaling)
			//00445529 E8 22 48 00 00       call        00449D50
			//*(DWORD*)0x44552a = (DWORD)som_MDL_449d50-0x44552e;
			memset((void*)0x445529,0x90,5);
			//0044559C E8 CF 49 00 00       call        00449F70
			*(DWORD*)0x44559d = (DWORD)som_MDL_449f70_scale-0x4455a1;

			//this is a "trampoline" on 440ab0 (drawing routine) since
			//it's too much trouble to disrupt som.scene.cpp's __asm
			//code. my goal is to try to draw the MDL parts in reverse
			//to see if it helps with secret doors, traps, etc. since
			//D3DCMP_LESSEQUAL goes to the last drawn polygon

			*(BYTE*)0x440ab0 = 0xe9; //jmp
			*(DWORD*)0x440ab1 = (DWORD)som_MDL_440f30_drawing-0x440ab5;
			*(BYTE*)0x440ab5 = 0xc3; //ret
		}
		//else *(DWORD*)0x440AFB = (DWORD)som_MDL_440f30_drawing-0x440AFF;

		//base CP delta
		//00407389 E8 22 A5 03 00       call        004418B0
		*(DWORD*)0x40738A = (DWORD)som_MDL_4418b0-0x40738E;
		//004297EE E8 BD 80 01 00       call        004418B0
		*(DWORD*)0x4297EF = (DWORD)som_MDL_4418b0-0x4297F3;
		//00444429 E8 82 D4 FF FF       call        004418B0
		*(DWORD*)0x44442A = (DWORD)som_MDL_4418b0-0x44442E;
		cmp = 0x4418b0;

		//MDO+MDL
		//this is defined outside this switch block (below)

		//som.art.cpp (MDO path)
		//00403E0E E8 4D 18 04 00       call        00445660 //gold
		//0040FC93 E8 C8 59 03 00       call        00445660 //item
		//0041091B E8 40 4D 03 00       call        00445660 //take
		//0041208C E8 CF 35 03 00       call        00445660 //sky
		//00423121 E8 3A 25 02 00       call        00445660 //menu
		//0042725E E8 FD E3 01 00       call        00445660 //equip
		//00427371 E8 EA E2 01 00       call        00445660 
		//00427463 E8 F8 E1 01 00       call        00445660  
		//0042A740 E8 1B AF 01 00       call        00445660 //object
		*(DWORD*)0x403E0f = (DWORD)som_MDO_445660-0x403E13;
		*(DWORD*)0x40FC94 = (DWORD)som_MDO_445660-0x40FC98;
		*(DWORD*)0x41091c = (DWORD)som_MDO_445660-0x410920;
		*(DWORD*)0x41208d = (DWORD)som_MDO_445660-0x412091;
		*(DWORD*)0x423122 = (DWORD)som_MDO_445660-0x423126;
		*(DWORD*)0x42725f = (DWORD)som_MDO_445660-0x427263; //UNUSED
		*(DWORD*)0x427372 = (DWORD)som_MDO_445660-0x427376; //UNUSED
		*(DWORD*)0x427464 = (DWORD)som_MDO_445660-0x427468; //UNUSED
		*(DWORD*)0x42A741 = (DWORD)som_MDO_445660-0x42A745;
		//object support for non MDO/MDL extension
		//0042A6FB 68 50 E5 45 00       push        45E550h 
		//0042A703 E8 F8 6B FD FF       call        00401300
		//0042A70D 74 16                je          0042A725
		*(BYTE*)0x42A6FB = 0x55; //push ebp
		*(DWORD*)0x42A6Fc = 0x90909090;
		*(DWORD*)0x42A704 = (DWORD)som_MDL_401300_obj_art-0x42A708;		
		*(WORD*)0x42A70D = 0x40eb; //jmp 42a74f

		//som.art.cpp (TXR path)
		// 
		// REMINDER: not including DATA/PICTURE call sites
		// 
		//00405FB8 E8 23 26 04 00       call        004485E0 //enemy (skin)
		//00412BBF E8 1C 5A 03 00       call        004485E0 //msm
		//00428B1F E8 BC FA 01 00       call        004485E0 //NPC (skin)
		//0042D5ED E8 EE AF 01 00       call        004485E0 //sfx/0207.txr
		//0042D618 E8 C3 AF 01 00       call        004485E0 //sfx/0208.txr
		//0042D646 E8 95 AF 01 00       call        004485E0 //sfx/0209.txr
		//0042D671 E8 6A AF 01 00       call        004485E0 //sfx/0210.txr
		//0042E752 E8 89 9E 01 00       call        004485E0 //sfx
		//004457D5 E8 06 2E 00 00       call        004485E0 //mdo
		//BMP files shouldn't use som_TXR_4485e0
		//because it bypasses the extension test
		//405453 slideshow
		//405adf slideshow
		//40973c image event //HACK adding critical section
		*(DWORD*)0x405454 = (DWORD)som_BMP_4485e0-0x405458;
		*(DWORD*)0x405ae0 = (DWORD)som_BMP_4485e0-0x405ae4;
		*(DWORD*)0x40973d = (DWORD)som_BMP_4485e0-0x409741;
		//412bbf mpx (msm)
		//412c81 mpx (msm retry with absolute path)
		//42144b menu icons
		//42c785 pushstart.bmp
		//42c7ae newgame.bmp
		//42c7da continue.bmp
		//42c838 title.bmp (42c80f loads the EXE logo.bmp I think) 
		*(DWORD*)0x405FB9 = (DWORD)som_TXR_4485e0-0x405FBd;
		*(DWORD*)0x412Bc0 = (DWORD)som_TXR_4485e0-0x412Bc4;
		*(DWORD*)0x428B20 = (DWORD)som_TXR_4485e0-0x428B24;
		*(DWORD*)0x42D5Ee = (DWORD)som_TXR_4485e0-0x42D5f2;
		*(DWORD*)0x42D619 = (DWORD)som_TXR_4485e0-0x42D61d;
		*(DWORD*)0x42D647 = (DWORD)som_TXR_4485e0-0x42D64b;
		*(DWORD*)0x42D672 = (DWORD)som_TXR_4485e0-0x42D676;
		*(DWORD*)0x42E753 = (DWORD)som_TXR_4485e0-0x42E757;
		*(DWORD*)0x4457D6 = (DWORD)som_TXR_4485e0-0x4457Da;
		/*SFX choose MDL or TXR
		0042E6E6 68 D0 A6 45 00       push        45A6D0h  
		0042E6EB 52                   push        edx  
		0042E6EC E8 AD 0E 02 00       call        0044F59E  
		0042E6F1 83 C4 18             add         esp,18h  
		0042E6F4 85 C0                test        eax,eax  
		0042E6F6 74 09                je          0042E701 		
		*/
		*(BYTE*)0x42E6E6 = 0x56; //push esi
		*(DWORD*)0x42E6E7 = 0x90909090; //nop
		*(DWORD*)0x42E6Ed = (DWORD)som_MDL_42e5c0_SFX_art-0x42E6f1;		
		*(WORD*)0x42E6f6 = 0x41eb; //jmp 42E739

		//som_MPX_swap (add ref counting to unload subs)
		//00406570 E8 7B 9E 03 00       call        004403F0 //enemy
		//0042469C E8 4F BD 01 00       call        004403F0 //arm.mdl?
		//00428EAD E8 3E 75 01 00       call        004403F0 //npc
		//0042AB0E E8 DD 58 01 00       call        004403F0 //object
		//0042E8C4 E8 27 1B 01 00       call        004403F0 //sfx
		//0042E9B1 E8 3A 1A 01 00       call        004403F0 //sfx
		//0043CD6A E8 81 36 00 00       call        004403F0 //kage.mdl?
		*(DWORD*)0x406571 = (DWORD)som_MDL_4403F0-0x406575; //enemy
		*(DWORD*)0x428EAe = (DWORD)som_MDL_4403F0-0x428Eb2; //npc
		*(DWORD*)0x42AB0f = (DWORD)som_MDL_4403F0-0x42AB13; //object
		*(DWORD*)0x42E8C5 = (DWORD)som_MDL_4403F0-0x42E8C9; //sfx
		*(DWORD*)0x42E9B2 = (DWORD)som_MDL_4403F0-0x42E9B6; //sfx
		//00403EA9 E8 C2 19 04 00       call        00445870 //gold
		//0040FD11 E8 5A 5B 03 00       call        00445870 //item load error?
		//0040FDBF E8 AC 5A 03 00       call        00445870 //item
		//00410B7E E8 ED 4C 03 00       call        00445870 //item pickup?
		//00412164 E8 07 37 03 00       call        00445870 //sky load error?
		//004133B4 E8 B7 24 03 00       call        00445870 //sky
		//00423262 E8 09 26 02 00       call        00445870 //menu
		//0042764F E8 1C E2 01 00       call        00445870 //unequip
		//00427680 E8 EB E1 01 00       call        00445870 //unequip
		//0042AB21 E8 4A AD 01 00       call        00445870 //object
		*(DWORD*)0x403EAa = (DWORD)som_MDO_445870-0x403EAe; //item/gold
		*(DWORD*)0x40FDc0 = (DWORD)som_MDO_445870-0x40FDc4; //item
		*(DWORD*)0x410B7f = (DWORD)som_MDO_445870-0x410B83; //pickup
		*(DWORD*)0x4133B5 = (DWORD)som_MDO_445870-0x4133B9; //sky
		*(DWORD*)0x423263 = (DWORD)som_MDO_445870-0x423267; //menu
		*(DWORD*)0x427650 = (DWORD)som_MDO_445870-0x427654; //unequip?
		*(DWORD*)0x427681 = (DWORD)som_MDO_445870-0x427685; //unequip?
		*(DWORD*)0x42AB22 = (DWORD)som_MDO_445870-0x42AB26; //object

		(DWORD&)som_MDL_malloc = 0x401500;
		(DWORD&)som_MDL_free = 0x401580;
		(DWORD&)som_MDL_x4418b0 = 0x4418b0; //get CP delta
		(DWORD&)som_MDL_x440030 = 0x440030; //load_MDL_file
		extern void som_scene_44d810_extern(void*);
		som_MDL_x44d810 = som_scene_44d810_extern;
		(DWORD&)som_TXR_x448fd0 = 0x4485e0; //load_TXR_etc
		(DWORD&)som_MDL_x448780 = 0x448780; //unload_TXR
		(DWORD&)som_MDL_x447ff0 = 0x447ff0; //reserve_material
		(DWORD&)som_MDL_x448100 = 0x448100; //unload_material
		(DWORD&)som_MDO_x445660 = 0x445660; //load MDO (art)
		(DWORD&)som_MDL_x45f350 = 0x45f350; //som_MDL_4
		break;
	}
	if(cmp) //bug: this comparison is backward
	{
		//00441904 3B F8                cmp         edi,eax
		auto w = (WORD*)(cmp+(0x441904-0x4418b0));
		if(*w==0xf83b) *w = 0xf839;
		else assert(*w==0xf83b);
	}

	//hide all CPs??? why does this test for red CPs (0~31) only
	//cyan CPs (soft animation only)????
	//
	// also correctly handle out-of-bounds UV maps
	//
	switch(SOM::tool)
	{
	case 0:
	
		//00441F29 B1 01                mov         cl,1
		*(BYTE*)0x441F2a = 0; //CPs
		
		//correctly handle out-of-bounds UV maps
		//004449DA 7C 0A                jl          004449E6
		//004449E4 33 C0                xor         eax,eax
		*(BYTE*)0x4449DB+=1;
		memmove((void*)0x4449E7,(void*)0x4449E6,6);
		memcpy((void*)0x4449E4,"\x83\xc8\xff",3); //or eax,0xffffffff

		break;

	case SOM_MAP.exe:
		
		/*UNFINISHED! arrows trap? (O244.MDL)
		//00401279 B1 01                mov         cl,1
		*(BYTE*)0x40127a = 0; //CPs */
		
		//FORMERLY SOM_MAP_reprogram SOM_MAP.cpp
		//
		// 2020: this also correctly handles out-of-bounds UV maps 
		//
		//allow MDL files without TIM sections
		{
			//NOTE: most of this is just showing the thought process
			//I developed it while working on SOM_MAP_kfii_reprogram

			//assigning MDL data pointer
			//00408659 89 75 0C             mov         dword ptr [ebp+0Ch],esi
			//reading MDL data? malloc
			//00408644 E8 F6 D0 05 00       call        0046573F  
			//00408649 8B F0                mov         esi,eax
			/*
			00408644 E8 F6 D0 05 00       call        0046573F  
			00408649 8B F0                mov         esi,eax  
			0040864B B9 11 02 00 00       mov         ecx,211h  
			00408650 33 C0                xor         eax,eax  
			00408652 8B FE                mov         edi,esi  
			00408654 F3 AB                rep stos    dword ptr es:[edi]
			*/
			//this set the texture (material?) to -1 but is unnecessary
			//memset((void*)0x40864b,0x90,11);
			//*(DWORD*)0x408645 = (DWORD)SOM_MAP_46573f_mdl-0x408649;
			//no good? this sets to 0
			//00401419 89 46 08             mov         dword ptr [esi+8],eax
			//this is the call site, param 4 of 6 is the texture id
			//00401A8A E8 01 F9 FF FF       call        00401390
			//
			//trying something...
			//
			// this causes 401000 to return -1 by default instead of 0
			// it seems to do the trick. or is 1B more than xor so the
			// extra code is just to make room
			//
			//004010BA 7C 0A                jl          004010C6
			//004010C4 33 C0                xor         eax,eax
			*(BYTE*)0x4010bb+=1;
			memmove((void*)0x4010c7,(void*)0x4010c6,6);
			memcpy((void*)0x4010c4,"\x83\xc8\xff",3); //or eax,0xffffffff
		}
		break;
	}


			if(!SOM::game) return;

	
	if(EX::INI::Bugfix()->do_fix_animation_sample_rate)
	{
		//OBJECTS ARE INTIALIZED TO #6???
		//OBJECTS ARE INTIALIZED TO #6???
		//OBJECTS ARE INTIALIZED TO #6???
		//0042A9D8 6A 06                push        6

		//this is power/magic gauge drain...
		//00425238 C1 E1 06             shl         ecx,6
		//I'm not sure this will work
		//*(BYTE*)0x42523a = 5;

		//this makes the swing model skip a frame
		//00425C71 E8 9A B8 01 00       call        00441510
		//00425C8A E8 81 B8 01 00       call        00441510
		memset((void*)0x425C71,0x90,5);

		//loading MDL
		//004400cc e8 2a fb 00 00       call        0044fbfb //ftell
		//004400f2 e8 1c fa 00 00       call        0044fb13 //fread
		*(DWORD*)0x4400cd = (DWORD)som_MDL_44fbfb-0x4400d1;
		*(DWORD*)0x4400f3 = (DWORD)som_MDL_44fb13-0x4400f7;

		//extend fade out (death) time... note this isn't tied
		//to the length of the death animation
		SOM::L.fade/=som_MDL_fps;
		//
		// this affects turn rates, but there's many instances of this
		// constant, so this is pretty unsafe (pretty much all of them
		// look like SFX procedures)
		//
		*(FLOAT*)0x45831C/=som_MDL_fps; //SOM::L.rate

		//testing a theory
		//00440F69 C7 45 3C FF FF FF FF mov         dword ptr [ebp+3Ch],0FFFFFFFFh
		//memset((void*)0x440F6c,0x0,4);

		//storing control points
		//enemies
		//0040740E E8 AD A2 03 00       call        004416C0
		//NPCs?
		//0042988C E8 2F 7E 01 00       call        004416C0
		//objects...
		//0042A9D0 E8 EB 6C 01 00       call        004416C0
		//0042B19C E8 1F 65 01 00       call        004416C0
		//0042B87A E8 41 5E 01 00       call        004416C0  
		//0042D17F E8 3C 45 01 00       call        004416C0  
		*(DWORD*)0x40740F = (DWORD)som_MDL_4416c0-0x407413;
		*(DWORD*)0x42988D = (DWORD)som_MDL_4416c0-0x429891;
		*(DWORD*)0x42A9D1 = (DWORD)som_MDL_4416c0-0x42A9D5;
		*(DWORD*)0x42B19D = (DWORD)som_MDL_4416c0-0x42B1a1;
		*(DWORD*)0x42B87B = (DWORD)som_MDL_4416c0-0x42B87F;
		*(DWORD*)0x42D180 = (DWORD)som_MDL_4416c0-0x42D184;
		//these are called by nonstatic subroutine pointers
		//00406E18 E8 A3 A8 03 00       call        004416C0
		//00429585 E8 36 81 01 00       call        004416C0
		//0042B040 E8 7B 66 01 00       call        004416C0
		*(DWORD*)0x406E19 = (DWORD)som_MDL_4416c0-0x406E1d;
		*(DWORD*)0x429586 = (DWORD)som_MDL_4416c0-0x42958a;
		*(DWORD*)0x42B041 = (DWORD)som_MDL_4416c0-0x42B045;

		//SFX procedure table hacks
		//
		// these are animated pictures (this seems to
		// cover all of them)
		//
		//som_game_60fps (workshop.cpp) extends flames
		//41 is hardcoded, extended directly below...
		*(void**)0x45EA6C = som_MDL_43a350;
		//lightning strike (41)
		*(void**)0x45E794 = som_MDL_43a350; //004370e0
		//
		// hard coded timing??? (fix me)
		//
		//lightning strike (41)
		//004370C4 C7 41 2C 04 00 00 00 mov         dword ptr [ecx+2Ch],4
		*(DWORD*)0x4370C7*=2;

		//flashes? //UNFINISHED
		//
		// it looks like most of the fullscreen effects update in
		// real-time, but not the following one since it happens 
		// in the world-step phase
		//
		// WHAT ABOUT MENU PULSES?
		//
		//pulses power gauges and I think blindness status and maybe
		//one or two other things
		//004023ED FF 05 44 22 4C 00    inc         dword ptr ds:[4C2244h]
		//memset((void*)0x4023ed,0x90,6); //pulse?

		//NPC animation frame rate?
		//00429f09 83 f9 21        CMP        ECX,0x21
		//00429f56 83 fa 21        CMP        EDX,0x21
		*(BYTE*)0x429f0b = 13;
		*(BYTE*)0x429f58 = 13;

		//4c2244 world counter... can't just skip frames
		//because it will inflict 2x poison
		//poison
		//00425c28 b9 1e 00 00 00        MOV        ECX,0x1e //30
		*(BYTE*)0x425c29 = 0x1e*2;
		//something?
		//0042602e b9 1e 00 00 00        MOV        ECX,0x1e //30
		*(BYTE*)0x42602f = 0x1e*2;
		//power gauges
		/*there's way more to this. 426027 could map to a new
		//counter (som_scene_44D810 violates an assert)
		//004269b5 b9 3c 00 00 00       MOV        ECX,0x3c //60
		*(BYTE*)0x4269b6 = 0x3c*2;*/
		//hp regen/degen
		//004252a6 b8 2c 01 00 00       MOV        EAX,0x12c //300
		//004252e1 b8 2c 01 00 00       MOV        EAX,0x12c //300
		*(WORD*)0x4252a7 = 0x12c*2; *(WORD*)0x4252e2 = 0x12c*2;
		//mp regen/degen
		//00425320 b8 2c 01 00 00        MOV        EAX,0x12c //300
		//00425359 b8 2c 01 00 00        MOV        EAX,0x12c //300
		*(WORD*)0x425321 = 0x12c*2; *(WORD*)0x42535a = 0x12c*2;

		//2022: move object event frame count?
		//NOTE: som_state_40a480 may already have made this obsolete
		//0040a573 c1 ea 03        SHR        EDX,0x3
		*(BYTE*)0x40a575 = 2; //shr edx,2
	}	
	if(som_MDL_hz) //testing //2023 //som_MDL_hz
	{
		SOM::motions.hz_30 = SOM::motions.l_hz_30 = 1;

		//animation length
		//0040E2D7 E8 54 31 03 00       call        00441430
		//0042A92B E8 00 6B 01 00       call        00441430
		//0042D0DB E8 50 43 01 00       call        00441430
		//0x4415DA E8 51 FE FF FF       call        0x441430  
		//0x4418FC E8 2F FB FF FF       call        0x441430 
		*(DWORD*)0x40E2D8 = (DWORD)som_MDL_441430-0x40E2Dc;
		*(DWORD*)0x42A92c = (DWORD)som_MDL_441430-0x42A930;
		*(DWORD*)0x42D0Dc = (DWORD)som_MDL_441430-0x42D0e0;
		*(DWORD*)0x4415Db = (DWORD)som_MDL_441430-0x4415Df;
		*(DWORD*)0x4418Fd = (DWORD)som_MDL_441430-0x441901;
	}

	//if(SOM::game) //fix (it actually worked)
	{
		//this code treats the soft frame times as char instead of unsigned
		//it stores negative time values that are erroneous and limits time
		
		//this is at time of loading
		//004443A1 0F BE 49 01          movsx       ecx,byte ptr [ecx+1]
		*(DWORD*)0x4443A1 = 0x149b60f; //movzx ecx,byte ptr [ecx+1]

		//FIX ME 
		//
		// This is converting a byte to a float and dividing when it
		// could have computed a float at the top to be multiplied!!
		//
		//this is animation code... it recomputes the floating point step
		//size everytime it decodes a delta! like so:
		//(float)iVar6 / (float)(int)*(char *)((int)psVar4 + 1
		//0044466B 0F BE 41 01          movsx       eax,byte ptr [ecx+1]
		*(DWORD*)0x44466B = 0x141b60f;
		//004446A9 0F BE 41 01          movsx       eax,byte ptr [ecx+1]
		*(DWORD*)0x4446A9 = 0x141b60f;
		//004446E9 0F BE 49 01          movsx       ecx,byte ptr [ecx+1]
		*(DWORD*)0x4446E9 = 0x149b60f;
		//004445AD 0F BE 41 01          movsx       eax,byte ptr [ecx+1]
		*(DWORD*)0x4445AD = 0x141b60f;
		//004445EB 0F BE 41 01          movsx       eax,byte ptr [ecx+1] 
		*(DWORD*)0x4445EB = 0x141b60f;
		//0044462F 0F BE 49 01          movsx       ecx,byte ptr [ecx+1]
		*(DWORD*)0x44462F = 0x149b60f;
		//NOTE: this one is outside the loop? it's something else
		//00444760 0F BE 50 01          movsx       edx,byte ptr [eax+1]
		*(DWORD*)0x444760 = 0x150b60f;
	}

	//this changes the MDL loading routine to use vertex/normal index
	//based comparison instead of position
	//there's too many call sites to implement it outside the routine
	if(1)
	{
		//set som_MDL_444920_edge_flags_ext to high TSB byte
		//push esi
		//push edi
		//mov al,byte ptr [esp + 19h]
		//shr al,2
		//mov byte ptr [som_MDL_444920_edge_flags_ext],al
		memcpy((void*)0x444926,"\x56\x57\x8a\x44\x24\x19\xc0\xe8\x02\xa2",10);
		*(DWORD**)0x444930 = &som_MDL_444920_edge_flags_ext;
		//004421B4 7E 7A                jle         00442230
		memset((void*)0x4421B4,0x90,2);


		const char code[] = /*
		push esp
		push edi
		push edx
		push ecx
		push ebp
		call
		add esp,0x14
		mov ebx,eax*/
		"\x54\x57\x52\x51\x55\xe8\xf6\xff\xff\xff\x83\xc4\x14\x89\xc3";
		memset((void*)0x4421b9,0x90,0x442230-0x4421b9);
		memcpy((void*)0x4421b9,code,sizeof(code)-1);
		*(DWORD*)0x4421bf = (DWORD)som_MDL_442130_inner_loop-0x4421c3;
	}

	//if(som_MDL_with_mdo) //2021: MDO+MDL?
	{
		//malloc(2488)
		//00440049 68 B8 09 00 00       push        9B8h  
		//0044004E E8 AD 14 FC FF       call        00401500
		//00440057 B9 6E 02 00 00       mov         ecx,26Eh
		*(DWORD*)0x44004a = sizeof(SOM::MDL::data);
		*(DWORD*)0x440058 = sizeof(SOM::MDL::data)/4;

		//2022: add ref counter to MDO data?
		//4456d1
		//4456dc //this isn't really required ATM
		*(BYTE*)0x4456d2 = sizeof(SOM::MDO::data);		
		*(BYTE*)0x4456dd = sizeof(SOM::MDO::data)/4;
		
		//2022: increase MDO (and MDL?) size for ext fields?
		//00440535 68 E8 03 00 00       push        3E8h 
		//00440543 B9 FA 00 00 00       mov         ecx,0FAh
	//	*(DWORD*)0x4458Ea = sizeof(SOM::MDL); //smaller
	//  *(DWORD*)0x440544 = sizeof(SOM::MDL)/4;
		int compile[sizeof(SOM::MDL)<=1000]; (void)compile;
		//004458E9 68 BC 00 00 00       push        0BCh
		//004458F7 B9 2F 00 00 00       mov         ecx,2Fh
		*(DWORD*)0x4458Ea = sizeof(SOM::MDO);
		*(DWORD*)0x4458F8 = sizeof(SOM::MDO)/4;		

		//0042AAE9 E8 E2 AF 01 00       call        00445AD0 
		*(DWORD*)0x42AAEa = (DWORD)som_MDO_445ad0_free-0x42AAEe;
	}
	if(som_MDL_with_mdo)
	{
		//MDO+MDL
		//00405F18 E8 13 A1 03 00       call        00440030 //enemy
		//0042464C E8 DF B9 01 00       call        00440030 //arm
		//00428A7F E8 AC 75 01 00       call        00440030 //npc
		//0042A714 E8 17 59 01 00       call        00440030 //object
		//0042E77E E8 AD 18 01 00       call        00440030 //sfx
		//0043CD3C E8 EF 32 00 00       call        00440030 //kage
		*(DWORD*)0x405F19 = (DWORD)som_MDL_440030-0x405F1d;
		*(DWORD*)0x42464d = (DWORD)som_MDL_440030-0x424651;
		*(DWORD*)0x428A80 = (DWORD)som_MDL_440030-0x428A84;
		*(DWORD*)0x42A715 = (DWORD)som_MDL_440030-0x42A719;
		*(DWORD*)0x42E77f = (DWORD)som_MDL_440030-0x42E783;
		*(DWORD*)0x43CD3d = (DWORD)som_MDL_440030-0x43CD41;
		//OPTIMIZING (subroutine)
		//0044026B E8 30 45 00 00       call        004447A0
		//004402E5 E8 B6 3E 00 00       call        004441A0
		*(DWORD*)0x44026c = (DWORD)som_MDL_4447a0-0x440270;
		*(DWORD*)0x4402E6 = (DWORD)som_MDL_4441a0-0x4402Ea;

		//for some stupid reason the structure is zeroed out
		//before freeing
		//0044050F F3 AB                rep stos    dword ptr es:[edi] 
		//00440511 E8 6A 10 FC FF       call        00401580
		*(WORD*)0x44050F = 0x9090;
		*(DWORD*)0x440512 = (DWORD)som_MDL_4403f0_free-0x440516;
		//00440A83 F3 AB                rep stos    dword ptr es:[edi]
		//00440A85 E8 B6 75 66 78       call        004409d0
		*(WORD*)0x440A83 = 0x9090;
		*(DWORD*)0x440a86 = (DWORD)som_MDL_4409d0_free-0x440a8a;

		//these prevent allocating materials and allocate MDO
		//instance data
		//0044058c //0044059f //004405b2
		*(DWORD*)0x44058d = (DWORD)som_MDL_447fc0-0x440591;
		*(DWORD*)0x4405a0 = (DWORD)som_MDL_447fc0-0x4405a4;
		*(DWORD*)0x4405b3 = (DWORD)som_MDL_447fc0-0x4405b7;
	}

	//UV animation for MDL, including sky control extensions
	{
		//0040235C E8 0F 14 01 00       call        00413770
		*(DWORD*)0x40235d = (DWORD)som_MDL_413770-0x402361;

		//PIGGYBACKING
		//MDO drawing routine (446010) (object matrix)
		// 
		// TODO: would like to improve this subroutine since
		// it's very CPU heavy (for no reason) but this is
		// just a convenient place to do the UV animations
		// 
		//00446043 E8 88 FC FF FF       call        00445CD0
		*(DWORD*)0x446044 = (DWORD)som_MDL_445cd0-0x446048;
	}
	//2022: set sorting bit on transparent elements	
	{
		//00445ab1 e8 1a 02 00 00       call        00445CD0
		*(DWORD*)0x445ab2 = (DWORD)som_MDL_445cd0_init-0x445ab6;

		//2023: yuck
		//0043A432 E8 69 53 FF FF       call        0042F7A0
		*(DWORD*)0x43A433 = (DWORD)som_MDL_42f7a0-0x43A437;
	}
}

int SOM::MDL::animation(int id)
{
	assert(SOM::game);
	return ((DWORD(__cdecl*)(MDL*,DWORD))0x4413a0)(this,id);
}
int SOM::MDL::animation_id(int c)
{
	assert(SOM::game);
	return ((DWORD(__cdecl*)(MDL*,DWORD))0x441340)(this,c);
}
int SOM::MDL::running_time(int c)
{
	assert(SOM::game);
	c = ((int(__cdecl*)(MDL*,int))0x441430)(this,c);

	//2023: try to scale to framerate
	return som_MDL_hz?(int)(c*SOM::motions.hz_30):c;
}
bool SOM::MDL::ending_soon(int f)
{
	int fps = EX::INI::Bugfix()->do_fix_animation_sample_rate?2:1;
	return d<=1||d>running_time(c)-f*fps;
}
bool SOM::MDL::ending_soon2(int f)
{
	int fps = EX::INI::Bugfix()->do_fix_animation_sample_rate?2:1;
	return ext.d2<=1||ext.d2>running_time(ext.e2)-f*fps;
}
bool SOM::MDL::control_point(float avg[3], int c, int f, int cp)
{
	DWORD *cps = mdl_data->cp_file;
	if(!cps||0==~cps[2+cp]) //UNFINISHED
	{
		memset(avg,0x00,12); return false;
	}
	
	bool partial = false;

	if(EX::INI::Bugfix()->do_fix_animation_sample_rate)
	{
		int skip = 0==c&&mdl_data->file_head[0]&3?2:1; 

		if(f>skip)
		{
			f-=skip;
			partial = (f&1)==0; assert(2==som_MDL_fps);
			f/=som_MDL_fps;
			f+=skip;
		}
	}
	
	BYTE *fp = (BYTE*)cps+cps[2+cp]+cps[c+34]+f*12;
	memcpy(avg,fp,12);
	if(partial) for(int i=3;i-->0;)
	{
		avg[i] = (avg[i]+((FLOAT*)fp)[i-3])*0.5f;
	}
	return true;
}
void SOM::MDL::draw(void *tes)
{
	assert(SOM::game);
	((BYTE(__cdecl*)(MDL*,void*))0x440ab0)(this,tes);
}
void SOM::MDL::update_animation()
{
	if(SOM::game) som_MDL_440f30(*this); else som_MDL_animate(*this);
}
void SOM::MDL::update_transform()
{
	som_MDL_440ab0_transforms(*this);
}
