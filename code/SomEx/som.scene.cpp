
#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

#include <vector>
#include <algorithm>

#include "dx.ddraw.h" 

#include "Ex.ini.h" 
#include "Ex.input.h" 
#include "Ex.output.h" 

#include "SomEx.h" 
#include "som.state.h" 
#include "som.status.h" //EXPERIMENTAL
#include "som.game.h" //sss
	
#define SOMVECTOR_MATH
#include "../Somplayer/Somvector.h"

extern int SomEx_npc;

namespace sss = som_scene_state; //2023

extern std::vector<SOM::MDL*> som_kage;

extern DWORD som_scene_ambient2 = 0;
extern DWORD som_scene_ambient2_vset9(float p5[3+2])
{
	extern DWORD som_MHM_ambient2(DWORD,float[3+2]);
	if(som_scene_ambient2)
	{
		union{ DWORD cmp; BYTE rgba[4]; };
		cmp = som_scene_ambient2;
		cmp = som_MHM_ambient2(cmp,p5);
		if(cmp!=som_scene_ambient2)
		{
			som_scene_ambient2 = cmp;

			if(cmp!=1)
			{
				float r[4]; 
				for(int i=4;i-->0;) r[i] = rgba[i]/255.0f;
				DDRAW::vset9(r,1,DDRAW::vsGlobalAmbient+1);
			}
		}
		return cmp; //DEBUGGING
	}
	return 0; //REMOVE ME?
}

extern "C" //#include <d3dx9math.h>
{
	struct D3DXMATRIX* WINAPI D3DXMatrixPerspectiveFovLH
	(D3DXMATRIX*,FLOAT,FLOAT,FLOAT,FLOAT);
	struct D3DXMATRIX* WINAPI D3DXMatrixPerspectiveOffCenterLH
	(D3DXMATRIX*,FLOAT,FLOAT,FLOAT,FLOAT,FLOAT,FLOAT);
}

extern void __cdecl SOM::frame_is_missing()
{
	//For when a map/som_state_409af0
	//opens to a text event.
	if(warped==frame) warped++;

	//in places SOM begins drawing a new// 
	//frame without having displayed the//
	//current frame. this could flip the//
	//frame; that hasn't been tried. But//
	//for now just keep up the heartbeat//

	SOM::frame++; //ie. DDRAW::noFlips++;
	
	//EXPERIMENTAL
	if(EX::debug) //frame_is_missing?
	{
		assert(!DDRAW::inScene);

		if(DDRAW::inScene) //NEW (2020)
		{
			//if(EX::debug) MessageBeep(-1);

			DDRAW::Direct3DDevice7->EndScene(); 
		}
	}
}

static const bool som_scene_skyshadow = true;

//even though this is 0.0001 the shadows sit 0.02 off the
//floor, and I'm not sure why that is
//when I softened them for KFII I began to notice the NPC
//shadows were darker than the PC's shadow
//static float &som_scene_shadowdecal = *(float*)0x458314; //0.0001
static const float som_scene_shadowdecal = 0.0201f;

extern void som_scene_xform_bsp_v(void *in, float **v, size_t sz, bool item)
{
	auto *se = (som_scene_element*)in;
	extern DX::D3DMATRIX som_hacks_item; 
	extern void som_hacks_recenter_item(DX::LPD3DMATRIX);
	if(item) som_hacks_recenter_item((DX::LPD3DMATRIX)se->worldxform);
	auto *w = (float*)se->vdata;
	for(;sz-->0;v++,w+=8) 
	{
		//float *p = *v+1;		
		float *p = *v+2; //d
		float *n = p+3, *m = w+3;
		Somvector::multiply<4>(Somvector::map<3>(w),se->worldxform,Somvector::map<3>(p));
		Somvector::multiply<3>(Somvector::map<3>(m),se->worldxform,Somvector::map<3>(n));
		p[6] = w[6]; //u
		p[7] = w[7]; //v
	}
}
extern void som_scene_lighting(bool l) //2023
{
	sss::lighting_desired = l; //2022
	sss::lighting_current = l; //2022

	DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_LIGHTING,l); 
}

static void som_scene_44d810_flush();

extern int som_hacks_shader_model;

static struct som_scenery //may be just transparency?
{	
	DWORD se_charge,se_commit; som_scene_elements *ses;

	bool empty(){ return 0==se_commit; }

	som_scenery &flush() //transparency
	{
		return sort().draw().clear();
	}
	som_scenery &sort()
	{
		//NOTE: the original depth-sort (this) is pretty crazy
		((BYTE(__cdecl*)(void*))0x44D5A0)(this); return *this;
	}
	som_scenery &draw() //transparency
	{
		 draw_44d7d0(this); return *this;
	}
	static void draw_44d7d0(som_scenery *transparency)
	{
		som_scene_44d810_flush(); //2021
		{
			if(EX::INI::Option()->do_alphasort&&som_hacks_shader_model) //hack
			{
				extern bool som_scene_hud; //compass? etc?
				if(!som_scene_hud)
				{
					som_scene_elements &ses = *transparency->ses;	
					extern DWORD som_bsp_add_vbufs(som_MDL::vbuf**,DWORD);
					transparency->se_commit =
					som_bsp_add_vbufs(ses,transparency->se_commit);
					extern void som_bsp_sort_and_draw(bool push);
					som_bsp_sort_and_draw(true);
					((BYTE(__cdecl*)(void*))0x44D7D0)(transparency); //unsorted
					transparency->clear(); //2022
					sss::setss(1); //HACK
					som_scene_lighting(0);
				}				
				else ((BYTE(__cdecl*)(void*))0x44D7D0)(transparency); 
			}
			else ((BYTE(__cdecl*)(void*))0x44D7D0)(transparency);
		}
		som_scene_44d810_flush(); //2021
	}
	som_scenery &clear()
	{
		((BYTE(__cdecl*)(void*))0x44D540)(this); return *this;
	}

	void push_back(som_scene_element *se)
	{
		if(se_commit<se_charge) (*ses)[se_commit++] = se;
		else assert(0); //no real recourse
	}

	static som_scenery *alloc(int n)
	{
		int s = sizeof(som_scenery)+n*sizeof(void*);
		auto p = (som_scenery*)new char[s];
		p->se_commit = 0;
		p->se_charge = n; (void*&)p->ses = p+1; return p;
	}

}*&som_scene_transparentelements = *(som_scenery**)0x4C223C;

//2021: som.MDL.cpp needs to work with workshop.cpp tools
extern void som_scene_push_back(void *s, void *se)
{
	((som_scenery*)s)->push_back((som_scene_element*)se);
}

static som_scenery *som_scene_transparentelements3 = 0; //swing model?

//2021: som_scene_44d810_batch
static som_scenery *som_scene_batchelements = 0;
static DWORD som_scene_batchelements_lock = 0;
extern DWORD &som_scene_batchelements_hold; //YUCK
struct som_scene_batchelements_off //RAII
{
	som_scenery *swap;
	DDRAW::IDirect3DVertexBuffer7 *swap1d69da4; //TRICKY
	som_scene_batchelements_off():swap(som_scene_batchelements)
	{
		assert(!swap||!swap->se_commit);

		som_scene_batchelements = 0;

		//TRICKY: see som_scene_44d810 comments 
		if(swap) swap1d69da4 = SOM::L.vbuffer;
		if(swap) SOM::L.vbuffer = 0;		
	}
	~som_scene_batchelements_off()
	{
		som_scene_batchelements = swap;

		assert(!swap||!swap->se_commit);

		if(swap) SOM::L.vbuffer = swap1d69da4;
	}
};

//KF2: som.hacks.cpp
extern void som_scene_zwritenable_text() 
{
	//HACK: if !zwe writing text to zbuffer
	//otherwise mask frames 
	DWORD &zwe = sss::zwriteenable; //assert(!zwe);	
	DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_ZENABLE,zwe);
	zwe = 1;
	DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_ZWRITEENABLE,1);
	DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_ZFUNC,DX::D3DCMP_LESS);
}
extern void som_scene_alphaenable(int fab=0xFAB)
{
	extern bool som_hacks_fab;
	//if(!alphaenable)
	{
		sss::alphaenable = fab!=0; //som_hacks_alphablendenable_fab(in);
		DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_ALPHABLENDENABLE,fab);		
	}
	//else som_hacks_fab = fab=0xFAB;
	if(!fab) return;
	//if(srcblend!=DX::D3DBLEND_SRCALPHA)
	{
		sss::srcblend = DX::D3DBLEND_SRCALPHA; //DX::D3DBLEND_SRCCOLOR;
		sss::destblend = DX::D3DBLEND_INVSRCALPHA; //DX::D3DBLEND_INVSRCCOLOR;
		DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_SRCBLEND,sss::srcblend);		
		DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_DESTBLEND,sss::destblend);		
	}
	//else assert(destblend==DX::D3DBLEND_INVSRCALPHA);
}
extern void som_scene_ops(DWORD color, DWORD alpha) //2022
{
	if(alpha!=sss::tex0alphaop)
	DDRAW::Direct3DDevice7->SetTextureStageState
	(0,DX::D3DTSS_ALPHAOP,sss::tex0alphaop=alpha);
	if(color!=sss::tex0colorop)
	DDRAW::Direct3DDevice7->SetTextureStageState
	(0,DX::D3DTSS_COLOROP,sss::tex0colorop=color);
}
extern void som_scene_translucent_frame()
{
	sss::zwriteenable = 1; //HACK: not set???
	som_scene_zwritenable_text();
	assert(!sss::alphaenable);
	som_scene_alphaenable();
}

static void (*som_scene_onreset_passthru)() = 0;
DX::IDirect3DVertexBuffer7 *som_scene::vbuffers[som_scene::vbuffersN] = {};
DX::IDirect3DVertexBuffer7 *som_scene::ubuffers[som_scene::vbuffersN] = {};
static void som_scene_onreset() //2021
{
	som_scene_onreset_passthru();

	//NOTE: som_scene_44d060 could implement this but
	//I feel like som.scene.cpp should have an onReset
	//callback installed to teardown its D3D interfaces
	if(som_scene::vbuffers[0])
	{
		//som_scene_44d060
		//assert(SOM::L.vbuffer==som_scene::vbuffers[0]);
		assert(!SOM::L.vbuffer);

		//som_hacks_CreateVertexBuffer is setting these up
		for(int i=1;i<som_scene::vbuffersN;i++)
		{
			if(auto*&ea=som_scene::vbuffers[i])
			{
				ea->Release(); ea = 0;
			}
			if(auto*&ea=som_scene::ubuffers[i])
			{
				ea->Release(); ea = 0;
			}
		}
		som_scene::vbuffers[0] = 0;
		som_scene::ubuffers[0] = 0;
	}
}
static void __cdecl som_scene_44d060()
{
	//set the mode 3 vbuffer back to the original?
	if(!som_scene::vbuffers[0]) assert(0);
	else
	{
		if(void*&vb=*(void**)0x1d69da4)
		vb = som_scene::vbuffers[0];
		if(void*&ub=*(void**)0x59892C)
		ub = som_scene::ubuffers[0];
	}
	((void(__cdecl*)())0x44d060)();
}

extern bool som_scene_gamma_n = false; //King's Field II?

extern float *som_scene_lit = 0;
extern float som_scene_lit5[10] = {};
extern bool som_scene_alit = false;
extern bool som_scene_44d810_disable = false; //EXPERIMENTAL
//this subroutine does all of the drawing
//som_hacks_DrawIndexedPrimitiveVB checks som_scene_lit
static void som_scene_44d810_batch(som_scene_element *se);
static BYTE __cdecl som_scene_44d810(som_scene_element *se, const DWORD batch_os=0)
{
	//EXPERIMENTAL
	if(som_scene_44d810_disable) //som_MPX_42dd40?
	{
		if(se->texture<1024)
		SOM::L.textures[se->texture].update_texture(0);
		return 1;
	}

	if(se->fmode==3)
	if(som_scene_batchelements) //TESTING
	{	
		som_scene_44d810_batch(se); return 1;
	}

	int batched; //DEBUGGING
	//if(se->abe) switch(se->mode) //transparent?
	if(se->batch) 
	{
		se->batch = 0; //technically correct

		//PROBLEMATIC
		//flush/44d7d0 have to manage this
		//som_scene_gamma_n = se->npc==3; //2021

		switch(se->fmode) //transparent? batched?
		{
		case 3: //lighting?

			if(!se->lit //2018: flames?
			||!sss::lighting_desired) //2022: sky?
			{
				som_scene_lit = 0;
			}
			else if(se->worldxform[0][3]) //hack: repurposing
			{
				//this was designed for transparent elements
				//som_scene_batchelements_hold is using lit5
				//som_scene_lit = som_scene_lit5;
				som_scene_lit = som_scene_lit5+5;
				som_scene_lit[0] = se->lightselector[0];
				som_scene_lit[1] = se->lightselector[1];
				som_scene_lit[2] = se->lightselector[2];
				som_scene_lit[3] = se->worldxform[0][3]; se->worldxform[0][3] = 0;
				som_scene_lit[4] = se->worldxform[1][3]; se->worldxform[1][3] = 0;

			//	DWORD debug = //having batching troubles
				som_scene_ambient2_vset9(som_scene_lit); //2020
			}
			else EX_BREAKPOINT(0); //2021

		//default:

			//som_scene_44d7d0
			//if(se==som_scene_transparentelements->red.tse)
			//som_scene_transparentelements->red.pop_back();
		}
	}
	/*else //TESTING
	{
		assert(3!=se->mode||!se->npc); //REMOVE ME

		assert(se->ai!=150||se->npc<2);
	}*/
	
	int todolist[SOMEX_VNUMBER<=0x1020704UL];
	//2021: HIGHLY DUBIOUS
	//2021: HIGHLY DUBIOUS
	//2021: HIGHLY DUBIOUS
	//testing: sufficient?
	if(som_scene_lit)
	{
		if(!DDRAW::isLit) //REMOVE ME?
		{
			//assert(se->fmode==3); //2022 //2024

			if(1||!EX::debug) //trying for a while without?
			{
				som_scene_lighting(1); //2022
			}
			else //2022: Moratheia? (nearby man's statue)
			{
				//I think this is triggered leaving picture menu drawn
				//with som_hacks_Blt_fan
			
				//assert(sss::lighting_desired==1); //2022
				EX_BREAKPOINT(0)
			}
		}	
	}
	else if(DDRAW::isLit) //2024: happened in som_scene_sky
	{
		//som_scene_lighting(0); //inverse does not apply
	}

	//som_scene_44d810_flush/batch_os?
	if(2==((BYTE(__cdecl*)(som_scene_element*))0x44D810)(se)) 
	{
		assert(se->fmode==3); //som_scene_reprogram?

		/*can't work because this is also the trigger to batch 
		//above... so 0x1d69da4 is set to 0 temporarily instead
		//that forces 44D810 to call DrawIndexedPrimitive, which
		//is clever but perplexing
		if(!som_scene_batchelements) //som_scene_batchelements_off?
		{
			DDRAW::Direct3DDevice7->DrawIndexedPrimitive(DX::D3DPT_TRIANGLELIST,274,se->vdata,se->vcount,se->idata,se->icount,0);
		}
		else*/
		{
			//crazy? D3DVERTEXBUFFERDESC::dwNumVertices is 4096 but the chunks are
			//always 128 or less. FVF size is 32. 32*128=4096
			/*this is no longer true for x2mdl generated MDO files
			assert(se->vcount<=128);*/
			assert(se->vcount<=4096);
			DDRAW::IDirect3DVertexBuffer7 *vb = SOM::L.vbuffer;
			DDRAW::Direct3DDevice7->DrawIndexedPrimitiveVB(DX::D3DPT_TRIANGLELIST,vb,batch_os,se->vcount,se->idata,se->icount,0);
		}
	}
	/*else //TESTING
	{
		assert(3!=se->mode||!se->npc); //REMOVE ME
	}*/

	som_scene_lit = 0; //reusing light configuration //???

	return 1;
}
extern void som_scene_44d810_extern(void *se)
{
	som_scene_44d810((som_scene_element*)se); //som.MDL.cpp
}
static void som_scene_44d810_flush()
{
	auto e = som_scene_batchelements; //assert(e);
	if(!e||!e->se_commit) return;

		som_scene_batchelements = 0; //som_scene_44d810

	DDRAW::IDirect3DVertexBuffer7 *vb = SOM::L.vbuffer; assert(vb);

	som_scene_element **p = *e->ses;

	//subtract elements that require processing?	
	DWORD s = som_scene_batchelements_lock; 
	DWORD n = som_scene_batchelements_hold; //YUCK
	if(!~n) n = e->se_commit;
	for(DWORD i=e->se_commit;i-->n;s-=p[i]->vcount)
	;
	char *f = 0; auto _ = s;
	if(!vb->partial_Lock(0x2820,(void**)&f,&_))
	{		
		for(DWORD i=0,sz;i<n;i++,f+=sz)
		memcpy(f,p[i]->vdata,sz=p[i]->vcount*32);
		vb->Unlock(); 
		
		som_scene_batchelements_lock-=s;
		if(~som_scene_batchelements_hold)
		som_scene_batchelements_hold = 0; //-=n
	}
	else assert(0); //FIX ME?

		bool npc2 = som_scene_gamma_n; //YUCK
		bool npc = npc2;

	for(DWORD sz,os=0,i=0;i<n;i++,os+=sz)
	{
		som_scene_element *se = p[i]; sz = se->vcount;

		assert(se->batch||!se->npc); //???

		//DDRAW::Direct3DDevice7->DrawIndexedPrimitiveVB(DX::D3DPT_TRIANGLELIST,vb,os,sz,se->idata,se->icount,0);
		if(npc!=(se->npc==3)) //REMOVE ME!
		{
			som_scene_gamma_n = npc=!npc;
		}
		som_scene_44d810(se,os);
	}
	//EXPERIMENTAL
	//test revolving vbuffers?
	for(DWORD i=0;i<som_scene::vbuffersN;)
	if(vb==som_scene::vbuffers[i++])
	{
		i%=som_scene::vbuffersN;
		SOM::L.vbuffer = (DDRAW::IDirect3DVertexBuffer7*)som_scene::vbuffers[i];
		break;
	}
	if(n>=e->se_commit)
	{
		assert(n==e->se_commit);

		e->se_commit = 0; 
	}
	else //~som_scene_batchelements_hold was nonzero?
	{
		memmove(p,p+n,(e->se_commit-n)*sizeof(*p));

		e->se_commit-=n;
	}
		som_scene_gamma_n = npc2; //YUCK: REMOVE ME!

		som_scene_batchelements = e; //som_scene_44d810 
}
static void som_scene_44d810_batch(som_scene_element *se)
{
	auto e = som_scene_batchelements;
	
	//with partial_Lock instead of filling the buffer this just
	//counts the size and fills on flush, assuming that the cost
	//of pushing a bunch of unused/junk memory beats out anything
	DWORD sz = se->vcount;
	if(sz+som_scene_batchelements_lock>=som_scene::vbuffer_size)
	{
		som_scene_44d810_flush(); //assert(!som_scene_batchelements_lock);
	}		
	som_scene_batchelements_lock+=sz;
	
	(*e->ses)[e->se_commit] = se;

	if(++e->se_commit==e->se_charge)
	{
		som_scene_44d810_flush();
	}
}

//REMOVE US
//prologger has better code for doing this
static BYTE som_scene_red_v;
static float som_scene_red_r;
static void *som_scene_red_SetMaterial;
static void *som_scene_SetMaterial_red(HRESULT *hr,DDRAW::IDirect3DDevice7*,DX::LPD3DMATERIAL7 &x)
{
	//this call (1 argument) gets the D3DMATERIAL7 
	//0044DE9C E8 AF A2 FF FF       call        00448150
	//...
	/*
	00448150 8B 44 24 04          mov         eax,dword ptr [esp+4]  
	00448154 85 C0                test        eax,eax  
	00448156 7C 18                jl          00448170  
	//1D3D250 is maximum? or shadow material? kage.mdl is nearby (0xdc?) 
	00448158 39 05 50 D2 D3 01    cmp         dword ptr ds:[1D3D250h],eax  
	//returns 0 if shadow material?
	0044815E 7E 10                jle         00448170  
	00448160 8D 0C C0             lea         ecx,[eax+eax*8]  
	00448163 8D 14 48             lea         edx,[eax+ecx*2]  
	00448166 A1 48 D2 D3 01       mov         eax,dword ptr ds:[01D3D248h]  
	0044816B 8D 44 90 04          lea         eax,[eax+edx*4+4]  
	0044816F C3                   ret  
	00448170 33 C0                xor         eax,eax  
	00448172 C3                   ret  
	*/
	DWORD &_mat = sss::material;

	//float r = x->emissive.r; 
	static DX::D3DCOLORVALUE swap; //2020	
	if(!hr) 
	{
		swap = x->emissive;

		EX::INI::Detail dt; if(&dt->red_saturation)
		{
			//NOTE: PC isn't using red_saturation?!

			DWORD rgb = (DWORD)dt->red_saturation(som_scene_red_r);				
			x->emissive.r+=(rgb>>16&0xff)/255.0f;
			x->emissive.g+=(rgb>>8&0xff)/255.0f;
			x->emissive.b+=(rgb&0xff)/255.0f;
		}
		else
		{
			//2020: try to deal with not using do_hdr when lighting 
			//is clamped to 1?
			if(!EX::INI::Option()->do_hdr) //EXPERIMENTAL
			{
				float r2 = som_scene_red_r/2;
				x->emissive.g-=r2;
				x->emissive.b-=r2;

				//x->emissive.r+=som_scene_red_r; som_scene_red_r = r;	
				x->emissive.r+=r2;
			}
			else x->emissive.r+=som_scene_red_r;		
		}
	}
	else
	{
		//x->emissive.r = som_scene_red_r; som_scene_red_r = r-som_scene_red_r;
		x->emissive = swap;
	}
	return som_scene_SetMaterial_red;
}
static DWORD som_scene_red_next(DWORD edi)
{
	int out = -1; DWORD diff = 149*4*1024; edi++;
	for(int i=0;i<SOM::Versus.x_s;i++) 
	if(SOM::Versus.x[i].EDI-edi<diff&&SOM::Versus.x[i].timer>666) 
	{
		if(SOM::Versus.x[i].drain) //zero divide (if HP is 0)
		{
			out = i; diff = SOM::Versus.x[i].EDI-edi;
		}
	}
	return out==-1?INT_MAX:SOM::Versus.x[out].EDI;
}
extern void som_scene_red(DWORD edi)
{
	if(edi!=0) 
	{
		for(int i=0;i<SOM::Versus.x_s;i++) 
		if(edi==SOM::Versus.x[i].EDI)
		{
			int time = SOM::Versus.x[i].timer; if(time>666)
			{	
				//todo: what about additive?
				float t = (SOM::Versus.x[i].timer-666)/333.0f; 
				som_scene_red_r = t<0||t>=1?0:sinf(t*M_PI);				
				//som_scene_red_r*=SOM::Red(SOM::Versus.x[i].drain,SOM::Versus.x[i].HP);
				som_scene_red_r*=SOM::Versus.x[i].red;
				if(!som_scene_red_r) 
				return;
				som_scene_red_v = i+1;
				break;		
			}
		}
		assert(som_scene_red_v&&som_scene_red_r>0);		
	}
	else
	{
		som_scene_red_v = 0; som_scene_red_r = 0;
	}	
	som_scene_red_SetMaterial = 
	DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_SETMATERIAL_HACK,
	edi?som_scene_SetMaterial_red:som_scene_red_SetMaterial);	
}

struct som_scene_static
{
	static DWORD bes,tes,red;
	static bool lit;
	static SOM::Struct<> *ai;
	#ifndef NDEBUG
	static bool paranoia; 
	#endif
};
DWORD som_scene_static::bes;
DWORD som_scene_static::tes,som_scene_static::red;
bool som_scene_static::lit;
SOM::Struct<> *som_scene_static::ai;
#ifndef NDEBUG
bool som_scene_static::paranoia;
#endif
extern DWORD &som_scene_batchelements_hold = som_scene_static::bes; //YUCK
template<class T>
struct som_scene_logger : som_scene_static
{
	som_scene_logger<T>(){ assert(!(paranoia=false)); }

	static void __cdecl prolog(DWORD EDI)
	{
		assert(paranoia=!paranoia);
		if(som_scene_batchelements)
		bes = som_scene_batchelements->se_commit;
		tes = som_scene_transparentelements->se_commit;
	//	SOM::Struct<> *ai = T::prologger(EDI);
		ai = T::prologger(EDI);
		lit = som_scene_lit;

		/*#ifdef _DEBUG
		switch(T::NPC) //DEBUGGING (2021)
		{
		default: som_scene_static::ai = 0; break;
		case 1: som_scene_static::ai = (som_NPC*)ai-SOM::L.ai2; break;
		case 2: som_scene_static::ai = (som_Enemy*)ai-SOM::L.ai; break;
		}
		if(2==T::NPC)
		if(150==som_scene_static::ai)
		{
			ai = ai; //breakpoint

			if(som_scene_lit5[1]==8.5f) //debugging (solved)
			{
				ai = ai; //???
			}
		}
		#endif*/
	 		
		if(T::NPC>0&&EDI==red)
		{
			som_scene_red(red);			
			red = som_scene_red_next(red); 
		}

		if(T::NPC&&DDRAW::vshaders9[8]) //KF2?
		{
			//NOTE: could precompute this but it's just temporary
			EX::INI::Detail dt;
			if(&dt->gamma_npc) switch(T::NPC)
			{
			case 1: //NPC?

				som_scene_gamma_n = //nan logic
				0<dt->gamma_npc(EX::NaN,ai->s[SOM::AI::npc]); break;

			case 2: //enemy?

				som_scene_gamma_n = //nan logic
				0<dt->gamma_npc(ai->s[SOM::AI::enemy],EX::NaN); break;

			default: //case 3: //magic?

				som_scene_gamma_n = true; break;
			}
			else som_scene_gamma_n = T::NPC!=1; 
		}

		if(som_scene_lit)
		if(!som_scene_batchelements)
		som_scene_ambient2_vset9(som_scene_lit);
	}
	static void __cdecl epilog()
	{
		assert(!(paranoia=!paranoia));
		
		transparent_or_batch(tes,som_scene_transparentelements);
		if(som_scene_batchelements) //2021
		transparent_or_batch(bes,som_scene_batchelements);

		som_scene_lit = 0; //anticipating next element		

		//Reminder: som_scene_red resets som_scene_red_v
		if(T::NPC&&som_scene_red_r) som_scene_red(0);

		if(T::NPC) som_scene_gamma_n = false; //KF2?

		T::epilogger(*ai); //2024
	}
	static void transparent_or_batch(DWORD &tes, som_scenery *e) //2021
	{
		//REMINDER: tes can be bes/som_scene_batchelements_hold!

		auto ses = *e->ses; //som_scene_transparentelements
		for(;tes<e->se_commit;tes++) 
		{
			som_scene_element *se = ses[tes];

			if(se->fmode!=3) continue; //shadow?

			se->batch = 1; //2021

			/*#ifdef _DEBUG
			se->ai = som_scene_static::ai;
			
			if(2==T::NPC)
			if(150==som_scene_static::ai)
			{
				e = e; //breakpoint

				if(som_scene_lit5[1]==8.5f) //debugging (solved)
				{
					e = e; //???
				}
			}
			#endif*/

			//NOTE: lighting had only be done to the first
			//element??? with sorting that couldn't have
			//been reliable? stable_sort?

			if(lit) //save lights for transparency pass
			{
				//ENSURE WILL BE SEEN BY som_scene_44d810
				//condition seen with objects and enemies
				//assert(se->worldxform[0][3]); //nonzero				
				if(!som_scene_lit5[3]) som_scene_lit5[3] = 0.1f;

				//TODO: depth-sorting needs more accurate
				//locii
				//I think this is bad for som_scene_44d5a0
				//BUT THE VALUES ARE SIMILAR REGARDLESS :(
				if(1) 
				{
					se->lightselector[0] = som_scene_lit5[0];
					se->lightselector[1] = som_scene_lit5[1];
					se->lightselector[2] = som_scene_lit5[2];	
				}
				//REMINDER: MAY INTERFERE WITH DEPTH-SORT
				//SINCE IT USES THE WHOLE TRANSFORM. IT'S
				//SINCE REWRITTEN HOWEVER
				se->worldxform[0][3] = som_scene_lit5[3];
				se->worldxform[1][3] = som_scene_lit5[4];				
			}

			//Reminder: the transparency buffer is sorted
			//after it is built, prior to being displayed
			//vs isn't 0 initialized, so must be set here
			se->vs = som_scene_red_v;
			se->npc = T::NPC?som_scene_gamma_n?3:T::NPC:0;
		}
	}
};

extern void som_MDO_446010(SOM::MDO&,void*); //2024

//2021
struct som_scene_money //singleton 
:
som_scene_logger<som_scene_money>
{
	//2021: this is needed to get som_scene_ambient2_vset9
	//to work for money. "gold" is part of the scene so it
	//should be correctly lit. SOM doesn't seem to set the
	//lights up for gold, but this should do it

	enum{ NPC=0 };
	static SOM::Struct<> *prologger(DWORD edi)
	{
		auto gold = (SOM::Struct<>*)edi;

		if(som_scene_alit) //NECESSARY???
		{
			//I'm not sure this is the best
			som_scene_lit = som_scene_lit5;
			memcpy(som_scene_lit,gold->f+2,sizeof(FLOAT)*3);			
			som_scene_lit[3] =
			som_scene_lit[4] = 0.01f; //ARBITRARY
		}
		return gold;
	}
	static void epilogger(SOM::Struct<>&) //2024
	{
		//NOP
	}
	static void draw() //42B220
	{
		for(int i=0;i<32;i++) if(SOM::L.gold[i].c[0])
		{
			prolog((DWORD)&SOM::L.gold[i]);
			((BYTE(__cdecl*)(void*,void*))0x446010)
			(SOM::L.gold_MDO[i],som_scene_transparentelements);
			epilog();
		}		
	}
}som_scene_money; //singleton
//OBJECTS
struct som_scene_objects //singleton 
:
som_scene_logger<som_scene_objects>
{
	enum{ NPC=0 };
	static SOM::Struct<> *prologger(DWORD edi)
	{
		auto &ai = *(SOM::Struct<46>*)(edi-68); 

		if(som_scene_alit) //NECESSARY???
		{
			som_scene_lit = som_scene_lit5;
			memcpy(som_scene_lit,(void*)edi,sizeof(FLOAT)*3);
			//TODO: could use further investigation			
			//Moratheia's Scala caves triggers this.
			//I suppose it's a box with 0 thickness.
			//assert(ai.f[28]||!ai.f[26]); //cylinders?
			//2020: I think height is first (26)
			//som_scene_lit[3] = 0.5f*max(ai.f[27],ai.f[28]); //26
			som_scene_lit[3] = ai.f[29]; //computed radius?
			som_scene_lit[4] = ai.f[26]; //27
		}

		return (SOM::Struct<>*)&ai;
	}
	static void epilogger(SOM::Struct<> &ai) //2024
	{
		//NOP
	}
	/*2024: rewriting
	static void draw()	//42B220
	{
		__asm //TODO? TRANSLATE INTO C++			
		{
		sub         esp,24h  
		mov         ecx,dword ptr ds:[59893Ch]  
		mov         eax,dword ptr [ecx+130h]  
		push        ebx  
		lea         eax,[eax+eax*4]  
		push        esi  
		lea         ebx,[ecx+eax*8+108h]  
		mov         eax,dword ptr ds:[01A193F8h]  
		xor         esi,esi  
		cmp         eax,esi  
		push        edi  
		mov         dword ptr [esp+24h],ebx  
		mov         dword ptr [esp+14h],3F800000h  
		jbe         _0042B286  
		//Phantom Rod alpha	
		xor         edx,edx  
		mov         edi,1Eh  
		div edi //eax
		mov         dword ptr [esp+2Ch],esi  
		mov         dword ptr [esp+28h],edx  
		fild        qword ptr [esp+28h]  
		fmul        dword ptr ds:[4585B4h]  
		fsubr       dword ptr ds:[458234h]  
		fcos  
		fadd        dword ptr ds:[4582B4h]  
		fmul        dword ptr ds:[458290h]  
		fstp        dword ptr [esp+14h]  
		_0042B286:
		cmp         dword ptr ds:[1A1B3FCh],esi  
		mov         dword ptr [esp+18h],esi  
		jle         _0042B505  
		push        ebp  
		mov         esi,1A4447Ah  
		jmp         _0042B2A4  
		_0042B29E:	//top of loop
		mov         ecx,dword ptr ds:[59893Ch]  
		_0042B2A4:
		mov         al,byte ptr [esi-1]  
		test        al,al  
		je          _0042B4E7  
		cmp         byte ptr [esi],0  
		je          _0042B4E7
		mov         eax,dword ptr [esi-1Ah]  
		test        eax,eax  
		jne         _0042B2CA  
		mov         eax,dword ptr [esi-16h]  
		test        eax,eax  
		je          _0042B4E7  
		_0042B2CA:
		mov         eax,dword ptr [ecx+130h]  
		xor         edx,edx  
		mov         dl,byte ptr [esi-78h]  
		cmp         edx,eax  
		jne         _0042B4E7  
		mov         edx,dword ptr [esi-2Eh]  
		lea         eax,[esp+20h]  
		push        eax  
		mov         eax,dword ptr [esi-36h]  
		lea         ecx,[esp+28h]  
		push        ecx  
		lea         edi,[esi-36h]  
		push        edx  
		push        eax  
		//call        00415BC0  
		//Reminder: som.MHM.cpp incorporates 415BC0
		mov eax,00415BC0h __asm call eax
		add         esp,10h  
		test        al,al  
		je          _0042B4E7  
		mov         ecx,dword ptr [ebx]  
		imul        ecx,dword ptr [esp+20h]  
		mov         ebp,dword ptr [esp+24h]  
		mov         edx,dword ptr [ebx+8]  
		add         ecx,ebp  
		shl         ecx,4  
		//NOTE: craps out if unlit
		test        dword ptr [ecx+edx+8],80000000h  
		je          _0042B4E7  
		push        3  
		push        edi
		//DISABLED?
		//select lamps (even if not drawing!)
		//call        00411210  
		mov eax,00411210h __asm call eax		

	//REMINDER: DRAWING IS NOT GUARANTEED

	call prolog //passing EDI

		xor         eax,eax  
		mov         ax,word ptr [esi-7Ah]  
		add         esp,8  
		lea         ecx,[eax*8]  
		sub         ecx,eax  
		xor         eax,eax  
		mov         ax,word ptr [ecx*8+1A36424h]  
		lea         ebx,[ecx*8+1A36400h]  
		lea         eax,[eax+eax*2]  
		lea         edx,[eax+eax*8]  
		xor         eax,eax  
		mov         al,byte ptr [edx*4+1A1B43Eh]  
		lea         ebp,[edx*4+1A1B400h]  
		dec         eax  
		jne         _0042B3A4  
		lea         eax,[esp+10h]  
		push        eax  
		lea         ecx,[esp+30h]  
		push        ecx  
		lea         edx,[esp+1Ch]  
		push        edx  
		mov         ecx,4C2358h //__thiscall?
		//call        00401070  
		mov eax,00401070h __asm call eax
		fld         dword ptr [esp+14h]  
		fsub        dword ptr [edi]  
		fstp        dword ptr [esp+14h]  
		fld         dword ptr [esp+10h]  
		fsub        dword ptr [esi-2Eh]  
		fst         dword ptr [esp+10h]  
		fld         dword ptr [esp+14h]  
		fpatan  
		fadd        dword ptr ds:[4582A0h]  
		fstp        dword ptr [esi-26h]  
		_0042B3A4:
		//invisible trap test
		mov         al,byte ptr [ebp+69h]  
		test        al,al  
		jne         _0042B45F  
		mov         eax,dword ptr [esi-1Ah]  
		test        eax,eax  
		je          _0042B408  
		add         eax,4  
		mov         ecx,edi  
		mov         edx,dword ptr [ecx]  
		mov         dword ptr [eax],edx  
		mov         edx,dword ptr [ecx+4]  
		mov         dword ptr [eax+4],edx  
		mov         ecx,dword ptr [ecx+8]  
		mov         dword ptr [eax+8],ecx  
		mov         eax,dword ptr [esi-1Ah]  
		lea         edx,[esi-2Ah]  
		mov         ecx,dword ptr [edx]  
		add         eax,10h  
		mov         dword ptr [eax],ecx  
		mov         ecx,dword ptr [edx+4]  
		mov         dword ptr [eax+4],ecx  
		mov         edx,dword ptr [edx+8]  
		mov         dword ptr [eax+8],edx  
		mov         al,byte ptr [ebx+1Fh]  
		test        al,al  
		je          _0042B3F5  
		mov         eax,dword ptr [esi-1Ah]  
		mov         ecx,dword ptr [esp+18h]  
		mov         dword ptr [eax+28h],ecx  
		_0042B3F5:
		mov         edx,dword ptr ds:[4C223Ch]  
		mov         eax,dword ptr [esi-1Ah]  
		push        edx  
		push        eax
		//draw MDL
		//call        00440AB0  
		mov eax,00440AB0h __asm call eax
		add         esp,8  
		_0042B408:
		mov         eax,dword ptr [esi-16h]  
		test        eax,eax  
		je          _0042B45F  
		mov         ecx,dword ptr [edi]  
		add         eax,34h  
		mov         dword ptr [eax],ecx  
		mov         edx,dword ptr [edi+4]  
		mov         dword ptr [eax+4],edx  
		mov         ecx,dword ptr [edi+8]  
		mov         dword ptr [eax+8],ecx  
		mov         eax,dword ptr [esi-16h]  
		lea         edx,[esi-2Ah]  
		mov         ecx,dword ptr [edx]  
		add         eax,4Ch  
		mov         dword ptr [eax],ecx  
		mov         ecx,dword ptr [edx+4]  
		mov         dword ptr [eax+4],ecx  
		mov         edx,dword ptr [edx+8]  
		mov         dword ptr [eax+8],edx  
		mov         al,byte ptr [ebx+1Fh]  
		test        al,al  
		je          _0042B44C  
		mov         eax,dword ptr [esi-16h]  
		mov         ecx,dword ptr [esp+18h]  
		mov         dword ptr [eax+7Ch],ecx  
		_0042B44C:
		mov         edx,dword ptr ds:[4C223Ch]  
		mov         eax,dword ptr [esi-16h]  
		push        edx  
		push        eax
		//draw MDO
		//call        00446010
//		mov eax,00446010h __asm call eax
		mov eax,som_MDO_446010 __asm call eax //2024

		add         esp,8  

		//REMOVE ME
		//debugging stats?
		//
		_0042B45F:
		mov         eax,dword ptr [esi-1Ah]  
		xor         ecx,ecx  
		xor         edx,edx  
		test        eax,eax  
		je          _0042B48C  
		mov         eax,dword ptr [eax]  
		mov         edi,dword ptr [eax+0ACh]  
		test        edi,edi  
		jle         _0042B4BB  
		add         eax,0C4h  
		_0042B47B:
		mov         ebp,dword ptr [eax-8]  
		mov         ebx,dword ptr [eax]  
		add         ecx,ebp  
		add         edx,ebx  
		add         eax,24h  
		dec         edi  
		jne         _0042B47B  
		jmp         _0042B4BB  
		_0042B48C:
		mov         eax,dword ptr [esi-16h]  
		test        eax,eax  
		je          _0042B4BB  
		mov         edi,dword ptr [eax]  
		mov         edi,dword ptr [edi+18h]  
		test        edi,edi  
		mov         eax,dword ptr [eax+0B8h]  
		jle         _0042B4BB  
		add         eax,5Ch  
		_0042B4A5:
		xor         ebx,ebx  
		mov         bx,word ptr [eax+2]  
		add         eax,68h  
		add         ecx,ebx  
		xor         ebx,ebx  
		mov         bx,word ptr [eax-68h]  
		add         edx,ebx  
		dec         edi  
		jne         _0042B4A5  
		_0042B4BB:
		mov         ebp,dword ptr ds:[4C39B4h]  
		mov         ebx,dword ptr [esp+28h]  
		add         ebp,edx  
		mov         eax,55555556h  
		imul        ecx  
		mov         eax,dword ptr ds:[004C39D0h]  
		mov         ecx,edx  
		shr         ecx,1Fh  
		add         edx,ecx  
		add         eax,edx  
		mov         dword ptr ds:[4C39B4h],ebp  
		mov         dword ptr ds:[004C39D0h],eax
		//^debug stats? //REMOVE?

	call epilog

		_0042B4E7:
		mov         eax,dword ptr [esp+1Ch]  
		mov         ecx,dword ptr ds:[1A1B3FCh]  
		inc         eax  
		add         esi,0B8h  
		cmp         eax,ecx  
		mov         dword ptr [esp+1Ch],eax  
		jl          _0042B29E  
		pop         ebp  
		_0042B505:
		pop         edi  
		pop         esi  
		pop         ebx  
		add         esp,24h  
		//ret  
		}
	}*/
	static void draw()
	{
		float pr = 1.0f; 
		int prt = *(DWORD*)0x1a193f8; //phantom rod timer
		if(prt)
		{
			pr = *(float*)0x3e567750;
			pr = 2*M_PI-prt%30*pr;
			pr = (cosf(pr)+1)*0.5f;
		}

		som_MPX &mpx = *SOM::L.mpx->pointer;
		auto *l = (SOM::MPX::Layer*)&mpx[SOM::MPX::layer_0];

		int n = SOM::L.ai3_size;
		for(int i=0;i<n;i++)
		{
			auto &ai = SOM::L.ai3[i];
			auto *m = (SOM::MDL*)ai[SOM::AI::mdl3];
			auto *o = (SOM::MDO*)ai[SOM::AI::mdo3];
			if(!m&&!o||!ai[SOM::AI::obj_valid]||!ai[SOM::AI::obj_shown])
			continue;			

			auto *p = (float*)&ai[SOM::AI::xyz3];
			int x,z;
			if(!((BYTE(__cdecl*)(float,float,int*,int*))0x415bc0)(p[0],p[2],&x,&z)
			||!l->tiles[z*100+x].msb) continue;
			
			//select lamps?	
			((BYTE(__cdecl*)(float*,DWORD))0x411210)(p,3);			
			
			auto obj = ai[SOM::AI::object];
			auto *prm = SOM::L.obj_prm_file[obj].uc;
			auto *pr2 = SOM::L.obj_pr2_file[(WORD&)prm[36]].uc;

			if(1==pr2[62]) //billboard
			{
				p[4] = atan2f(SOM::cam[2]-p[2],SOM::cam[0]-p[0])+M_PI_2;
			}
			if(0==pr2[105]) //map-only visible?
			{
				prolog((DWORD)p);

				if(m)
				{
					memcpy(m->xyzuvw,p,6*sizeof(float));
					if(prm[31]) m->fade = pr;
					((BYTE(__cdecl*)(void*,void*))0x440ab0)(m,som_scene_transparentelements);
				}
				if(o)
				{
					memcpy(&o->f[13],p,3*sizeof(float));
					memcpy(&o->f[19],p+3,3*sizeof(float));
					if(prm[31]) o->fade = pr;
					som_MDO_446010(*o,som_scene_transparentelements);
				}

				epilog();
			}
		}
	}

}som_scene_objects; //singleton
//ENEMIES
struct som_scene_enemies //singleton 
:
som_scene_logger<som_scene_enemies>
{	
	enum{ NPC=2 }; //1 (gamma_n?)
	static SOM::Struct<> *prologger(DWORD edi)
	{
		auto &enemy = *(SOM::Struct<149>*)(edi-72);

		if(som_scene_alit) //NECESSARY???
		{
			som_scene_lit = som_scene_lit5;
			memcpy(som_scene_lit,(void*)edi,sizeof(FLOAT)*3);
			
			som_scene_lit[3] = enemy[SOM::AI::radius];
			som_scene_lit[4] = enemy[SOM::AI::height];
		}
		
		auto *m = (SOM::MDL*)enemy[SOM::AI::mdl];
		if(m->ext.kage&&m->ext.kage->ext.mdo_elements)
		if(m&&(unsigned)m->mdl_data->ext.kage+1u>4096) //2024
		{
			auto t = som_kage.size();			
			m->ext.kage->ext.mdo_elements->texture = (WORD)t+4096u;
			som_kage.push_back(m);
		}

		return (SOM::Struct<>*)&enemy;
	}	
	static void epilogger(SOM::Struct<>&) //2024
	{
		//NOP
	}
	/*2024: rewriting
	static void draw() //408090
	{
		DWORD x4C7834 = (DWORD)SOM::L.ai+0x6c;
		DWORD x4DA234 = x4C7834+SOM::L.ai_size*0x254;

		__asm //TODO? TRANSLATE INTO C++			
		{
		mov         ecx,dword ptr ds:[59893Ch]  
		mov         eax,dword ptr [ecx+130h]  
		sub         esp,8  
		push        ebx  
		push        esi  
		lea         eax,[eax+eax*4]  
		push        edi  
		lea         ebx,[ecx+eax*8+108h]  
		//mov         esi,4C7834h  
		mov         esi,x4C7834  
		jmp         _004080B9  
		_004080B3:
		mov         ecx,dword ptr ds:[59893Ch] 
		_004080B9:
		mov         eax,dword ptr [esi+1D0h]  
		cmp         eax,3  
		je          _004080CD  
		cmp         eax,4  
		jne         _004081EA  
		_004080CD:	//compare to layer?
		mov         eax,dword ptr [ecx+130h]  
		xor         edx,edx  
		mov         dl,byte ptr [esi-6Ch]  
		cmp         edx,eax  
		jne         _004081EA  
		mov         eax,dword ptr [esi-4]  
		test        eax,eax  
		je          _004081EA  
		mov         edx,dword ptr [esi-1Ch]  
		lea         eax,[esp+0Ch]  
		push        eax  
		mov         eax,dword ptr [esi-24h]  
		lea         ecx,[esp+14h]  
		push        ecx  
		lea         edi,[esi-24h]  
		push        edx  
		push        eax  
		//call        00415BC0
		//Reminder: som.MHM.cpp incorporates 415BC0
		mov eax,00415BC0h __asm call eax
		add         esp,10h  
		test        al,al  
		je          _004081EA  
		mov         ecx,dword ptr [ebx]  
		imul        ecx,dword ptr [esp+0Ch]  
		mov         eax,dword ptr [esp+10h]  
		mov         edx,dword ptr [ebx+8]  
		add         ecx,eax  
		shl         ecx,4  
		test        dword ptr [ecx+edx+8],80000000h  
		je          _004081EA  
		push        3  
		push        edi  
		//call        00411210
		mov eax,00411210h __asm call eax
	
	call prolog //passing EDI

		mov         eax,dword ptr ds:[004C223Ch]  
		mov         ecx,dword ptr [esi-4]  
		push        eax  
		push        ecx
		//draw/animate MDL
		//call        00440AB0  
		mov eax,00440AB0h __asm call eax
		mov         eax,dword ptr [esi]  
		add         esp,10h  
		test        eax,eax  
		je          _0040819F  

	//SHADOW/////////////////////////////
	//
	// It looks like the shadow is always
	// added, but it isn't? not even as 0

		//checks SOM::L.shadow
		//call        0043CE00
		//mov eax,0043CE00h __asm call eax
		//test        al,al  
		//je          _0040819F  
		//copy position into shadow
		mov         edx,dword ptr [esi-4]  
		mov         eax,dword ptr [esi]  
		add         edx,4  
		mov         ecx,dword ptr [edx]  
		add         eax,4  
		mov         dword ptr [eax],ecx  
		mov         ecx,dword ptr [edx+4]  
		mov         dword ptr [eax+4],ecx  
		mov         ecx,dword ptr [edx+8] //edx  
		mov         dword ptr [eax+8],ecx //edx...

			//2023: include rotation
			mov ecx,dword ptr [edx+16]  
			mov dword ptr [eax+16],ecx 

		mov         eax,dword ptr [esi]  
		//adds 0.001 to the elevation
		fld         dword ptr [eax+8]  		
		fadd        dword ptr ds:[458314h]  
		fstp        dword ptr [eax+8]  
		mov         eax,dword ptr [esi-4]  
		mov         ecx,dword ptr [esi]  
		mov         edx,dword ptr [eax+28h]  
		mov         dword ptr [ecx+28h],edx  
		mov         eax,dword ptr ds:[004C223Ch]  
		mov         ecx,dword ptr [esi]  
		push        eax  
		push        ecx  
		//draw/animate MDL
		//call        00440AB0
		mov eax,00440AB0h __asm call eax
		add         esp,8  
		
	//////////////////////////////////

		//REMOVE ME
		//debugging stats?
		//
		_0040819F:
		mov         eax,dword ptr [esi-4]  
		mov         eax,dword ptr [eax]  
		mov         edi,dword ptr [eax+0ACh]  
		xor         ecx,ecx  
		xor         edx,edx  
		test        edi,edi  
		jle         _004081C2  
		add         eax,0C4h  
		_004081B7:
		add         ecx,dword ptr [eax-8]  
		add         edx,dword ptr [eax]  
		add         eax,24h  
		dec         edi  
		jne         _004081B7 
		_004081C2:
		mov         edi,dword ptr ds:[4C39B0h]  
		add         edi,edx  
		mov         eax,55555556h  
		imul        ecx  
		mov         eax,dword ptr ds:[004C39C0h]  
		mov         ecx,edx  
		shr         ecx,1Fh  
		add         edx,ecx  
		add         eax,edx  
		mov         dword ptr ds:[4C39B0h],edi  
		mov         dword ptr ds:[004C39C0h],eax 
		//^debug stats? //REMOVE?
	
	call epilog

		_004081EA:
		add         esi,254h  
		//cmp         esi,4DA234h  
		cmp         esi,x4DA234
		jl          _004080B3
		pop         edi  
		pop         esi  
		pop         ebx  
		add         esp,8  
		//ret  
		}
	}*/
	static void draw()
	{
		som_MPX &mpx = *SOM::L.mpx->pointer;
		auto *l = (SOM::MPX::Layer*)&mpx[SOM::MPX::layer_0];

		int n = SOM::L.ai_size;
		for(int i=0;i<n;i++)
		{
			auto &ai = SOM::L.ai[i];
			auto *m = (SOM::MDL*)ai[SOM::AI::mdl];
			if(!m||ai[SOM::AI::stage]!=3&&ai[SOM::AI::stage]!=4) continue;			

			auto *p = (float*)&ai[SOM::AI::xyz];
			int x,z;
			if(!((BYTE(__cdecl*)(float,float,int*,int*))0x415bc0)(p[0],p[2],&x,&z)
			||!l->tiles[z*100+x].msb) continue;
			//select lamps?	
			((BYTE(__cdecl*)(float*,DWORD))0x411210)(p,3);			
			
			prolog((DWORD)p);
			((BYTE(__cdecl*)(void*,void*))0x440ab0)(m,som_scene_transparentelements);
			if(auto*k=(SOM::MDL*)ai[SOM::AI::kage_mdl])
			{
				k->xyzuvw[0] = m->xyzuvw[0];
				k->xyzuvw[1] = m->xyzuvw[1]+0.01f;
				k->xyzuvw[2] = m->xyzuvw[2];
				k->xyzuvw[4] = m->xyzuvw[4]; //EXTENSION
				k->fade = m->fade;
				((BYTE(__cdecl*)(void*,void*))0x440ab0)(k,som_scene_transparentelements);
			}
			epilog();
		}
	}

}som_scene_enemies; //singleton
//NPCS
struct som_scene_NPCs //singleton 
:
som_scene_logger<som_scene_NPCs>
{
	enum{ NPC=1 };
	static SOM::Struct<> *prologger(DWORD esi)
	{
		auto &npc = *(SOM::Struct<43>*)(esi-64);			

		if(som_scene_alit) //NECESSARY???
		{
			som_scene_lit = som_scene_lit5;
			memcpy(som_scene_lit,(void*)esi,sizeof(FLOAT)*3);
			//TODO: could use further investigation
			
			som_scene_lit[3] = npc[SOM::AI::radius2];
			som_scene_lit[4] = npc[SOM::AI::height2];
		}

		auto *m = (SOM::MDL*)npc[SOM::AI::mdl2];
		if(m->ext.kage&&m->ext.kage->ext.mdo_elements)
		if(m&&(unsigned)m->mdl_data->ext.kage+1u>4096) //2024
		{
			auto t = som_kage.size();			
			m->ext.kage->ext.mdo_elements->texture = (WORD)t+4096u;
			som_kage.push_back(m);
		}

		return (SOM::Struct<>*)&npc;
	}
	static void epilogger(SOM::Struct<>&) //2024
	{
		//NOP
	}
	/*2024: rewriting 
	static void draw()	//429B10
	{
		__asm //TODO: TRANSLATE INTO C++
		{
		mov         ecx,dword ptr ds:[59893Ch]  
		mov         eax,dword ptr [ecx+130h]  
		sub         esp,8  
		push        ebp  
		push        esi  
		lea         eax,[eax+eax*4]  
		push        edi  
		lea         ebp,[ecx+eax*8+108h]  
		mov         esi,1A12E30h  
		jmp         _00429B39
		_00429B33:
		mov         ecx,dword ptr ds:[59893Ch]  
		_00429B39:
		cmp         byte ptr [esi+39h],3  
		jb          _00429C45
		mov         eax,dword ptr [ecx+130h]  
		xor         edx,edx  
		mov         dl,byte ptr [esi-40h]  
		cmp         edx,eax  
		jne         _00429C45  
		mov         eax,dword ptr [esi+20h]  
		test        eax,eax  
		je          _00429C45  
		mov         edx,dword ptr [esi+8]  
		lea         eax,[esp+0Ch]  
		push        eax  
		mov         eax,dword ptr [esi]  
		lea         ecx,[esp+14h]  
		push        ecx  
		push        edx  
		push        eax  
		//call        00415BC0  
		//Reminder: som.MHM.cpp incorporates 415BC0
		mov eax,00415BC0h __asm call eax
		mov         ecx,dword ptr [ebp]  
		imul        ecx,dword ptr [esp+1Ch]  
		mov         edi,dword ptr [esp+20h]  
		mov         edx,dword ptr [ebp+8]  
		add         ecx,edi  
		shl         ecx,4  
		mov         eax,dword ptr [ecx+edx+8]  
		add         esp,10h  
		test        eax,80000000h  
		je          _00429C45  
		push        3  
		push        esi  
		//call        00411210
		mov eax,00411210h __asm call eax
			
	call prolog //passing ESI

		mov         eax,dword ptr ds:[004C223Ch]  
		mov         ecx,dword ptr [esi+20h]  
		push        eax  
		push        ecx  
		//call        00440AB0  
		mov eax,00440AB0h __asm call eax
		mov         eax,dword ptr [esi+24h]  
		add         esp,10h  
		test        eax,eax  
		je          _00429BFA  
		
	//SHADOW/////////////////////////////
	//
	// It looks like the shadow is always
	// added, but it isn't? not even as 0

		//checks SOM::L.shadow
		//call        0043CE00  
		//mov eax,0043CE00h __asm call eax
		//test        al,al  
		//je          _00429BFA  
		//copy position into shadow
		mov         edx,dword ptr [esi+24h]  
			//2023: source data from mdl 
			mov ecx,dword ptr [esi+20h]
		mov         eax,dword ptr [ecx+4] //esi 
		mov         dword ptr [edx+4],eax
		//adds 0.001 to the elevation
		fld         dword ptr [ecx+8] //esi+4
		mov         eax,dword ptr [esi+24h] //ecx
		fadd        dword ptr ds:[458314h]  
		fstp        dword ptr [eax+8] //ecx
		mov         edx,dword ptr [esi+24h]  
		mov         eax,dword ptr [ecx+12] //esi+8
		mov         dword ptr [edx+0Ch],eax 

			//2023: include rotation
			mov eax,dword ptr [ecx+20]  
			mov dword ptr [edx+14h],eax 

		mov         ecx,dword ptr ds:[4C223Ch]  
		mov         edx,dword ptr [esi+24h]  
		push        ecx  
		push        edx  
		//call        00440AB0
		mov eax,00440AB0h __asm call eax
		add         esp,8  
		
	//////////////////////////////////

		//REMOVE ME
		//debugging stats?
		//
		_00429BFA:
		mov         eax,dword ptr [esi+20h]  
		mov         eax,dword ptr [eax]  
		mov         edi,dword ptr [eax+0ACh]  
		xor         ecx,ecx  
		xor         edx,edx  
		test        edi,edi  
		jle         _00429C1D  
		add         eax,0C4h 
		_00429C12:
		add         ecx,dword ptr [eax-8]  
		add         edx,dword ptr [eax]  
		add         eax,24h  
		dec         edi  
		jne         _00429C12  
		_00429C1D:
		mov         edi,dword ptr ds:[4C39B8h]  
		add         edi,edx  
		mov         eax,55555556h  
		imul        ecx  
		mov         eax,dword ptr ds:[004C39BCh]  
		mov         ecx,edx  
		shr         ecx,1Fh  
		add         edx,ecx  
		add         eax,edx  
		mov         dword ptr ds:[4C39B8h],edi  
		mov         dword ptr ds:[004C39BCh],eax
		//^debug stats? //REMOVE?
			
	call epilog

		_00429C45:
		add         esi,0ACh  
		cmp         esi,1A18430h  
		jl          _00429B33  
		pop         edi  
		pop         esi  
		pop         ebp  
		add         esp,8  
		//ret  
		}
	}*/
	static void draw()
	{
		som_MPX &mpx = *SOM::L.mpx->pointer;
		auto *l = (SOM::MPX::Layer*)&mpx[SOM::MPX::layer_0];

		int n = SOM::L.ai2_size;
		for(int i=0;i<n;i++)
		{
			auto &ai = SOM::L.ai2[i];
			auto *m = (SOM::MDL*)ai[SOM::AI::mdl2];
			if(!m||ai[SOM::AI::stage2]<3) continue;			

			auto *p = (float*)&ai[SOM::AI::xyz2];
			int x,z;
			if(!((BYTE(__cdecl*)(float,float,int*,int*))0x415bc0)(p[0],p[2],&x,&z)
			||!l->tiles[z*100+x].msb) continue;

			//select lamps?	
			((BYTE(__cdecl*)(float*,DWORD))0x411210)(p,3);						
			prolog((DWORD)p);
			((BYTE(__cdecl*)(void*,void*))0x440ab0)(m,som_scene_transparentelements);
			if(auto*k=(SOM::MDL*)ai[SOM::AI::kage_mdl2])
			{
				k->xyzuvw[0] = m->xyzuvw[0];
				k->xyzuvw[1] = m->xyzuvw[1]+0.01f;
				k->xyzuvw[2] = m->xyzuvw[2];
				k->xyzuvw[4] = m->xyzuvw[4]; //EXTENSION
				k->fade = m->fade;
				((BYTE(__cdecl*)(void*,void*))0x440ab0)(k,som_scene_transparentelements);
			}
			epilog();
		}
	}

}som_scene_NPCs; //singleton
//ITEMS
struct som_scene_items //singleton 
:
som_scene_logger<som_scene_items>
{
	enum{ NPC=0 };
	static SOM::Struct<> *prologger(DWORD edi)
	{
		if(som_scene_alit) //NECESSARY???
		{
			som_scene_lit = som_scene_lit5;
			memcpy(som_scene_lit,(void*)edi,sizeof(FLOAT)*3);
			som_scene_lit[3] = som_scene_lit[4] = 1;
		}
		return 0;
	}
	static void epilogger(SOM::Struct<>&) //2024
	{
		//NOP
	}
	/*2024: rewriting
	static void draw()	//4103B0
	{
		__asm //TODO: TRANSLATE INTO C++
		{
		mov         ecx,dword ptr ds:[59893Ch]  
		mov         eax,dword ptr [ecx+130h]  
		sub         esp,8  
		push        ebx  
		push        ebp  
		push        esi  
		lea         eax,[eax+eax*4]  
		push        edi  
		lea         ebp,[ecx+eax*8+108h]  		
		mov         edi,57E03Ch //&Drops[0].x 
		jmp         _004103DA  
		_004103D4:
		mov         ecx,dword ptr ds:[59893Ch]  
		_004103DA:
		mov         al,byte ptr [edi-4]  
		test        al,al  
		je          _004104DB
		mov         eax,dword ptr [ecx+130h]  
		xor         edx,edx  
		mov         dl,byte ptr [edi-1]  
		cmp         edx,eax  
		jne         _004104DB  
		mov         edx,dword ptr [edi+8]  
		lea         eax,[esp+10h]  
		push        eax  
		mov         eax,dword ptr [edi]  
		lea         ecx,[esp+18h]  
		push        ecx  
		push        edx  
		push        eax  
		//call        00415BC0 
		//Reminder: som.MHM.cpp incorporates 415BC0
		mov eax,00415BC0h __asm call eax
		add         esp,10h  
		test        al,al  
		je          _004104DB  
		mov         ecx,dword ptr [ebp]  
		imul        ecx,dword ptr [esp+10h]  
		mov         esi,dword ptr [esp+14h]  
		mov         edx,dword ptr [ebp+8]  
		add         ecx,esi  
		shl         ecx,4  
		//This is & on the most-significant bit. Signed?
		test        dword ptr [ecx+edx+8],80000000h
		je          _004104DB  
		push        3  
		push        edi  
		//call        00411210  
		mov eax,00411210h __asm call eax

	call prolog //passing EDI

		xor         ecx,ecx  
		mov         cl,byte ptr [edi-3]  
		xor         edx,edx  
		mov         dl,byte ptr [edi-2]  
		lea         eax,[ecx*8]  
		sub         eax,ecx  
		lea         esi,[eax+eax*2]  
		mov         eax,dword ptr ds:[004C223Ch]  
		xor         ecx,ecx  
		shl         esi,4  
		add         esi,566FF0h  
		mov         cx,word ptr [esi]  
		push        eax  
		shl         ecx,6  
		add         ecx,edx  
		mov         eax,dword ptr [ecx*4+556FD0h]  
		push        eax  
		//draw MDO
		//556FD0 is render stuff for 446010
		//The data structures are pretty strange
		//call        00446010  
		//mov eax,00446010h __asm call eax
		mov eax,som_MDO_446010 __asm call eax //2024
			
		//REMOVE ME
		//debugging stats?
		//
		xor         ecx,ecx  
		mov         cx,word ptr [esi]  
		xor         eax,eax  
		mov         al,byte ptr [edi-2]  
		add         esp,10h  
		xor         edx,edx  
		xor         ebx,ebx  
		shl         ecx,6  
		add         ecx,eax  
		mov         ecx,dword ptr [ecx*4+556FD0h]  
		mov         eax,dword ptr [ecx]  
		mov         ecx,dword ptr [eax+18h]  
		test        ecx,ecx  
		jle         _004104C1  
		mov         eax,dword ptr [eax+1Ch]  
		add         eax,0Ah  
		_004104AB:
		xor         esi,esi  
		mov         si,word ptr [eax-2]  
		add         eax,14h  
		add         edx,esi  
		xor         esi,esi  
		mov         si,word ptr [eax-14h]  
		add         ebx,esi  
		dec         ecx  
		jne         _004104AB  
		add         dword ptr ds:[4C39D4h],ebx  
		_004104C1:
		mov         eax,55555556h  
		imul        edx  
		mov         eax,edx  
		shr         eax,1Fh  
		add         edx,eax  
		add         dword ptr ds:[4C39E4h],edx  
		//^debug stats? //REMOVE?
	
	call epilog

		_004104DB:
		add         edi,24h  
		cmp         edi,58043Ch  
		jl          _004103D4  
		pop         edi  
		pop         esi  
		pop         ebp  
		pop         ebx  
		add         esp,8  
		//ret  
		}
	}*/
	static void draw()
	{
		som_MPX &mpx = *SOM::L.mpx->pointer;
		auto *l = (SOM::MPX::Layer*)&mpx[SOM::MPX::layer_0];

		for(int i=0;i<256;i++)
		{
			auto &ea = SOM::L.items[i];

			//if(!*(DWORD*)&ea) continue; //leaves ghost items behind

			if(!ea.nonempty) continue;

			int x,z;
			if(!((BYTE(__cdecl*)(float,float,int*,int*))0x415bc0)(ea.x,ea.z,&x,&z)
			||!l->tiles[z*100+x].msb) continue;

			//select lamps?	
			((BYTE(__cdecl*)(float*,DWORD))0x411210)(&ea.x,3);

			auto &prm = SOM::L.item_prm_file[ea.prm];

			if(SOM::MDO*o=SOM::L.items_MDO_table[prm.us[0]][ea.mdo])
			{
				prolog((DWORD)&ea.x);
			   			
				som_MDO_446010(*o,som_scene_transparentelements);

				epilog();
			}
			else assert(o); //EXTENSION
		}
	}

}som_scene_items; //singleton
//MAGICS
struct som_scene_magics //singleton 
:
som_scene_logger<som_scene_magics>
{
	enum{ NPC=3 }; //1 (gamma_n?)
	static SOM::Struct<> *prologger(DWORD edi)
	{
		if(som_scene_alit) //NECESSARY???
		{
			som_scene_lit = som_scene_lit5;
			auto *p = (som_scene_picture*)edi;
			if(p>=SOM::L.SFX_images&&p<SOM::L.SFX_images+512)
			{
				som_scene_lit[0] = p->vdata[0].x;
				som_scene_lit[1] = p->vdata[0].y;
				som_scene_lit[2] = p->vdata[0].z;
				som_scene_lit[0]+=p->vdata[2].x;
				som_scene_lit[1]+=p->vdata[2].y;
				som_scene_lit[2]+=p->vdata[2].z;
				som_scene_lit[0]*=0.5f;
				som_scene_lit[1]*=0.5f;
				som_scene_lit[2]*=0.5f;
			}
			else //SFX_models
			{
				memcpy(som_scene_lit,((SOM::MDL*)edi)->xyzuvw,sizeof(FLOAT)*3);
			}
			som_scene_lit[3] = som_scene_lit[4] = 1;
		}
		assert(edi); return 0;
	}
	static void epilogger(SOM::Struct<>&) //2024
	{
		//NOP
	}
	/*2024: this is mysteriously broken today???
	static void draw_old() //42EC40
	{
		__asm //TODO: TRANSLATE INTO C++
		{
		//stage 1: projectile effects
		mov         eax,dword ptr ds:[01C9DD34h]  
		push        ebx  
		push        esi  
		xor         ebx,ebx  
		test        eax,eax  
		push        edi  
		jle         _0042ECBB  
		mov         edi,1C8DD70h  
		_0042EC53:
		mov         eax,dword ptr ds:[004C223Ch]  
		mov         ecx,dword ptr [edi]  
		push        eax  
		push        ecx  

	call prolog //passing ECX

	//for 1 run I fixed it by breaking these up
	//into 2 lines, but after that, subsequent
	//runs jumped to a crazy address on call
	//prolog (above) (below eax was receiving
	//a wrong address when combined into 1 line)

		//call        00440AB0  
		mov eax,00440AB0h __asm call eax

	call epilog

		mov         eax,dword ptr [edi]  
		mov         eax,dword ptr [eax]  
		mov         esi,dword ptr [eax+0ACh]  
		add         esp,8  
		xor         ecx,ecx  
		xor         edx,edx  
		test        esi,esi  
		jle         _0042EC86  
		add         eax,0C4h  
		_0042EC7B:
		add         ecx,dword ptr [eax-8]  
		add         edx,dword ptr [eax]  
		add         eax,24h  
		dec         esi  
		jne         _0042EC7B  
		_0042EC86:
		mov         eax,dword ptr ds:[004C39C8h]  
		mov         esi,dword ptr ds:[4C39E0h]  
		add         eax,edx  
		mov         dword ptr ds:[004C39C8h],eax  
		mov         eax,55555556h  
		imul        ecx  
		mov         eax,dword ptr ds:[01C9DD34h]  
		mov         ecx,edx  
		shr         ecx,1Fh  
		add         edx,ecx  
		add         esi,edx  	
		inc         ebx  
		add         edi,4  
		cmp         ebx,eax  
		mov         dword ptr ds:[4C39E0h],esi  
		jl          _0042EC53
	

		_0042ECBB:
		//stage 2: explosion effects
		mov         eax,dword ptr ds:[01C9DD30h]  
		xor         esi,esi  
		test        eax,eax  
		jle         _0042ED07  
		mov         edi,1C45D70h 
		_0042ECCB:
		mov         edx,dword ptr ds:[4C223Ch]  
		push        edi  
		push        edx  

	call prolog //passing EDX

		//call        0044D570  
		//mov eax,0044D570h __asm call eax
		mov eax,som_scene_push_back __asm call eax

	call epilog

		mov         eax,dword ptr ds:[004C39C8h]  
		mov         ebx,dword ptr ds:[4C39E0h]  
		add         eax,4  
		add         ebx,2  
		add         esp,8  
		mov         dword ptr ds:[004C39C8h],eax  
		mov         eax,dword ptr ds:[01C9DD30h]  
		inc         esi  
		add         edi,90h  
		cmp         esi,eax  
		mov         dword ptr ds:[4C39E0h],ebx  
		jl          _0042ECCB  
		_0042ED07:
		pop         edi  
		pop         esi  
		pop         ebx  
		//ret  
		}
	}*/
	static void draw() //42EC40
	{
		int n = SOM::L.SFX_models_size;
		for(int i=0;i<n;i++)
		{
			DWORD edi = (DWORD)SOM::L.SFX_models[i];
			prolog(edi);
			((BYTE(__cdecl*)(DWORD,void*))0x440ab0)(edi,som_scene_transparentelements);
			epilog();
		}
		n = SOM::L.SFX_images_size;
		for(int i=0;i<n;i++)
		{
			DWORD edi = (DWORD)&SOM::L.SFX_images[i];
			prolog(edi);
			//these are not all transparent
			//causes confusion around sky and DDRAW::isLit?!
			if(0) som_scene_44d810((som_scene_element*)edi); //EXTENSION
			else som_scene_transparentelements->push_back((som_scene_element*)edi);
			epilog();
		}
	}

}som_scene_magics; //singleton

static bool som_scene_skyfirst()
{
	if(DDRAW::doClearMRT||SOM::skystart>=SOM::skyend||!EX::INI::Option()->do_alphafog)
	return false; return true;
}

extern void som_hacks_skycam(float x); //som_scene_skycam?

namespace som_scene_skyswap 
{
	static DWORD te_commit;
	static bool swapped = false; static void __cdecl swap()
	{	
		auto tc = te_commit;
		auto &sc = som_scene_transparentelements->se_commit;
		auto swap = sc;
		sc = swapped?tc:swap-=tc;
		auto &ses = *som_scene_transparentelements->ses; 
		if(swap>tc&&tc) //2022
		{	
			//there seems to be a problem when the sky
			//is larger than the existitng elements 
			//since the swap maneuver eats into itself
			//I can't wrap my head around it, so this
			//is just getting it out of its own way
			if(!swapped)
			{
				//overflow? preventing memory overrun
				if(swap*2>1024) sc = swap-=1024-swap;

				memmove(ses+swap,ses+tc,swap*sizeof(void*));
			}
			tc = swap;
		}		
		while(swap-->0) std::swap(ses[swap],ses[tc+swap]);		
		swapped = !swapped;
	}
	static void draw()
	{
		if(!SOM::sky||SOM::L.fill==2) return; //2024: MHM?

		//2022: sky blending logic
		float t2 = SOM::mpx2_blend;
		auto &dst = *SOM::L.corridor;
		int n = dst.fade[1]==18&&t2?2:1;
		
		SOM::MDO *skies[2] = {};
		for(int i=n;i-->0;) skies[i] =
		SOM::mpx_defs(i?SOM::mpx2:SOM::mpx).sky_instance();
		if(!skies[0]&&!skies[1]) return;

		if(skies[1]) skies[1]->f[31] = t2; //fade?
		if(skies[0]) skies[0]->f[31] = n==2?1-t2:1;

		if(skies[0]&&skies[1]&&SOM::skyswap) //swap?
		{
			//this is designed to ensure the draw order
			//doesn't change when reentering maps before
			//the sky transition is complete
			std::swap(skies[0],skies[1]);
		}
		
		//2022: should be able to do just once
		som_hacks_skycam(+1);

		auto *d7 = DDRAW::Direct3DDevice7;

		//sss::push(); //2022: OVERKILL?
		
		//mov eax,dword ptr ds:[004C223Ch]
		som_scenery &te = *som_scene_transparentelements; 
		//assert(te.se_charge==1024);

		//TRANSPLANT FROM som.hacks.cpp		
		bool ze; if(ze=!som_scene_skyfirst()) //NEW: z-tested sky 
		{
			d7->SetRenderState(DX::D3DRENDERSTATE_ZENABLE,1);
			d7->SetRenderState(DX::D3DRENDERSTATE_ZWRITEENABLE,sss::zwriteenable=0);
			//necessary?
			//d7->SetRenderState(DX::D3DRENDERSTATE_ZFUNC,DX::D3DCMP_LESSEQUAL);
		}
		
		//INFORM ABOUT NO. OF TRANSPARENT ELEMENTS
		te_commit = te.se_commit; swapped = false; //paranoia
		{	
			DX::D3DVIEWPORT7 vp;
			d7->GetViewport(&vp); 
			float z = vp.dvMinZ, Z = vp.dvMaxZ; 
			if(DDRAW::doClearMRT&&EX::INI::Option()->do_alphafog)
			{																				
				//190: D3DRS_COLORWRITEENABLE1
				d7->SetRenderState((DX::D3DRENDERSTATETYPE)190,0);		
			//	d7->SetRenderState(DX::D3DRENDERSTATE_ALPHABLENDENABLE,1);		
				d7->SetRenderState(DX::D3DRENDERSTATE_SRCBLEND,DX::D3DBLEND_SRCALPHA);		
				d7->SetRenderState(DX::D3DRENDERSTATE_DESTBLEND,DX::D3DBLEND_INVSRCALPHA);		
				
				//REMOVE ME?
				DX::D3DMATRIX proj; //TODO: use SOM::zoom instead

				//optimizing: early-Z-test				
				d7->GetTransform(DX::D3DTRANSFORMSTATE_PROJECTION,&proj);				
				float vec[4] = {0,0,SOM::skystart,1};
				if(EX::INI::Option()->do_rangefog) //rotate to edge of frustum
				{
					float fov = atanf(1.0f/proj._11);
					vec[0] = sinf(fov)*vec[2]; vec[2]*=cosf(fov);
				}
				float *p = &proj._11;
				Somvector::map(vec).premultiply<4>(Somvector::map<4,4>(p));				
				//
				// 2021: debug build is getting 0.999724925
				// release build is 1.00200570 after unpausing
				// (problem was taking the D3D9C path/DDRAW::midFlip)
				//
				vp.dvMinZ = vp.dvMaxZ = z+vec[2]/vec[3]*(Z-z);
				
			//2017: this assert is fired between loading maps. DDRAW::inScene
			//is true. so som_hacks_BeginScene is maybe based on the last map 

				if(SOM::newmap<SOM::frame-1)
				{
					//2021: see above notes
					//NOTE: according to above these should be equal?
					//assert(vp.dvMinZ>=0&&vp.dvMaxZ<=1);
					if(vp.dvMinZ>1)
					{
						EX_BREAKPOINT(0)

						vp.dvMinZ = vp.dvMaxZ = min(1.0f,vp.dvMaxZ);
					}
				}
			}
			else vp.dvMinZ = vp.dvMaxZ = 1;	
			d7->SetViewport(&vp);		
						
			if(1) //2022: replacing __asm block
			{
				int fe; bool le;
				if(le=som_scene_alit&&DDRAW::isLit)
				som_scene_lighting(0); //2022
				if(fe=sss::fogenable)
				d7->SetRenderState(DX::D3DRENDERSTATE_FOGENABLE,sss::fogenable=0);

				//NOTE: this was disabling "NEW: z-tested sky" above
				//d7->SetRenderState(DX::D3DRENDERSTATE_ZENABLE,0);
				//d7->SetRenderState(DX::D3DRENDERSTATE_ZWRITEENABLE,sss::zwriteenable=0);

				if(DDRAW::xr) DDRAW::xr_depth_clamp(true);

				//2022: replacing __asm block
				//PREVENT THE EMPTYING OF TRANSPARENCY BUFFER////
				//		 //call        0044D540
				//		 mov eax,0044D540h __asm call eax
				
				//2022: I messed up by assuming this was working
				//since the __asm code below was setting ZENABLE
				//but this still doesn't work because the opaque 
				//elements override ZWRITENABLE
				if(ze) *(DWORD*)0x1d1b070&=~0x10000000;
				{
					for(int i=n;i-->0;) if(auto*o=skies[i])
					{
						//copy PC position into mdo data+34h
						//note: seems to include bob effect?
						//mov eax,00401070h __asm call eax
						memcpy(o->f+13,SOM::L.view_matrix_xyzuvw,3*sizeof(float));
						((BYTE(__cdecl*)(void*,void*))0x446010)(o,som_scene_transparentelements);
					}
				}
				if(ze) *(DWORD*)0x1d1b070|=0x10000000;

				som_scene_44d810_flush(); //2021

		swap(); //NAMESAKE

				//mov eax,0044D5A0h __asm call eax
				//mov eax,0044D7D0h __asm call eax
				som_scene_transparentelements->sort();
				//som_scene_transparentelements->draw();
				((BYTE(__cdecl*)(void*))0x44D7D0)(som_scene_transparentelements); 

				som_scene_44d810_flush(); //2021

				if(DDRAW::xr) DDRAW::xr_depth_clamp(false);

				if(le) som_scene_lighting(1);
				if(fe) d7->SetRenderState(DX::D3DRENDERSTATE_FOGENABLE,sss::fogenable=1);
			
				d7->SetRenderState(DX::D3DRENDERSTATE_ZENABLE,1);
				d7->SetRenderState(DX::D3DRENDERSTATE_ZWRITEENABLE,sss::zwriteenable=1);

				//SEEMINGLY POINTLESS Clear(Z)
				//(expected by do_fix_zbuffer_abuse)
				d7->Clear(0,0,2,0,1.0f,0);
			}
	
			vp.dvMinZ = z; vp.dvMaxZ = Z;
			d7->SetViewport(&vp);		

			if(!som_scene_skyshadow) //wait for shadows?
			d7->SetRenderState((DX::D3DRENDERSTATETYPE)190,0xF);		
					
			//som_scesne_state::pop(); //2022: OVERKILL?

			som_hacks_skycam(-1); //2022
		}
		swap(); //NAMESAKE //FIX TRANSPARENCY BUFFER
	}	
}
extern float *som_scene_nwse = 0;
extern void som_scene_compass(float *x)
{
		som_scene_batchelements_off _be; //RAII

	sss::push();

	//TESTING
	//som_hacks_DrawPrimitive calls this when the compass
	//needle would be displayed. I think it's unnecessary
	//som_scene_transparentelements->flush();
	assert(som_scene_transparentelements->empty());
	
	auto a2 = som_scene_ambient2; if(a2)
	{
		som_scene_ambient2 = 0; 

		float r[4] = {1,1,1,1}; 
		DDRAW::vset9(r,1,DDRAW::vsGlobalAmbient+1);
	}

	DX::D3DVIEWPORT7 vp;
	DDRAW::Direct3DDevice7->GetViewport(&vp); 
	{			
		//00445cd0 generates a matrix from two sets of 
		//non-matrix parameters and does 8 (unnecessary)
		//matrix multiplies on 9 matrices unless these
		//parameters are equal
		
		//2 works well for 62 fov but it's too fish eye
		//20 fov feels squashed even though it looks ok
		float fov = 30; //62;
		fov = SOM::reset_projection(fov,DDRAW::inStereo?1:4);

		DX::D3DVIEWPORT7 vp2; if(!DDRAW::inStereo)
		{
			float aspect = SOM::fov[0]/SOM::fov[1];

			//using dvMinZ
			float depth = 1; //0.025f;
			//x[3] = -6; //this also scales distance?
			//1.8 matches the HP/MP height but feels 
			//like it's closer than the HP/MP display
			//large, like it's placed in front of it
			//x[15] = depth*1.8f*62/fov; //2
			//2 (equal to 3.92666650)
			x[15] = depth*1.9f*62/fov; 
			//this seems a perfect fit but 0.9f scales down so it's less bulbous
			x[25] = x[26] = x[27] = depth*0.9f;
			extern RECT som_hacks_kf2_gauges;
			auto r = som_hacks_kf2_gauges;
			int sz = 2*(r.bottom-r.top)+r.left;
			sz+=2; r.left-=2;
			vp2.dwHeight = sz;
			vp2.dwWidth = sz*aspect;
			//NOTE: D3D can clip the viewport
			vp2.dwX = SOM::width-vp2.dwWidth;
			vp2.dwX-=r.left/2;
			vp2.dwX+=(vp2.dwWidth-sz)/2;
			//vp2.dwY = r.left/2;
			vp2.dwY = r.left/3.1f;
		}
		else vp2 = vp;
		vp2.dvMinZ = 0;
		vp2.dvMaxZ = EX_INI_Z_SLICE_1; //swing model partition		
		DDRAW::Direct3DDevice7->SetViewport(&vp2);

		auto *identity = (DX::LPD3DMATRIX)0x45F520;		
		DDRAW::Direct3DDevice7->SetTransform(DX::D3DTRANSFORMSTATE_VIEW,identity);

		//HACK: add bounce effects
		const float x19 = x[19];
		//this makes the compass less sensitive to looking up and down so the
		//lettering stays visible
		float ext = M_PI/30;
		//som.mocap.cpp is limiting too now so this makes little difference
		//x[19] = _copysign(powf(fabsf(x[19])/(M_PI_2),2),x[19])*M_PI_2+ext;
		x[19]+=ext;
		//remove VR contribution?
		//float speed = SOM::doppler[0]*SOM::pov[0]+SOM::doppler[2]*SOM::pov[2];
		float pov2[3] = {0,0,1}; SOM::rotate(pov2,0,SOM::uvw[1]); 
		float speed = SOM::doppler[0]*pov2[0]+SOM::doppler[2]*pov2[2];

		float tilt = (1-powf(1-fabs(speed/SOM::dash),2));		
		//max is to create a bottoming out effect and stomp when fast running
		//the limit is hardcoded to the default walking effect
		//tilt+=max(-0.04f,SOM::eye[3])*10; //negative		
		tilt-=pow(fabsf(max(-0.4f,SOM::eye[3]*10)),0.5f)/3;
		//dip forward if jumping/running
		static float dip = 0;
		float cmp = SOM::incline-SOM::uvw[0];		
		if(fabs(cmp)<0.001f) 
		dip = _copysign(cmp,speed>=1||SOM::L.pcstep?1:-1);
		tilt = tilt>0?_copysign(tilt,speed)*M_PI/11:0.0f;
		if(!DDRAW::xr)
		x[19]-=dip+tilt;

		const float x8 = x[8];

		//just mocking this up... it's not based on the gauge display size
		if(DDRAW::inStereo) 
		{
			//the PSVR is truncated on the sides so its aspect doesn't match
			//"aspect" I don't know off hand if the exact amount is written
			//down somewhere
			//const float ff = 0.75f; //1.77777 (1920x1080)
			//this changes a lot in the barrel shape the lower down you go
			//maybe it doesn't in the set?
			const float ff = 1.5f;

			//trying to push back to the menu distance (which is?)
			float size_and_dist = 3.0f;

			if(DDRAW::xr) //world space?
			{
				size_and_dist*=2.0f;

				//FIX ME
				float dt = powf(sinf(dip+tilt),2.0f)/2;

				//1,1,1 is just uncomfortable enough to
				//almost ensure you don't look at it by
				//accident. unfortunately it means that
				//you can't see where you're going when
				//looking up
				//
				// TODO: I think further out to the side
				// could reduce neck strain for muscular
				// reasons but the angle must change too
				x[13] = 1.0f; 
				x[14] = 0.9f;
				x[15] = 1.0f+dt;
				SOM::rotate(x+13,x19,SOM::uvw[1]);					

				//subtract the VR offset because the compass
				//doesn't move with the set in order to fade
				//it out. afterward (last) the view position
				//is added, which this also serves to cancel
				for(int i=3;i-->0;) x[13+i]-=SOM::pos[i];
				
				//FUDGE: I don't understand this
				//there needs to be a better way
				//it should "lookat" SOM::cam
				float extra1 = 0, extra2 = 0;
				if(x19>0)
				{
					//this is almost perfect but a little
					//askew horizontally. problem is extra2
					//rolls instead of corrects, and so 
					//does extra3... 
					//extra1 = x19*-0.5f;
					//extra3 = x19*0.5f; //x[7]

					extra2 = x19*0.25f; //rolls???			
					x[8]-=x19*0.7f;
				}
				else
				{
					//x[8] would probably be safer here
					//but it looks fine 
					extra1 = x19*-0.3f;
					extra2 = x19*-0.2f;
				}
				x[19]+=ext*4.0f+extra1;
				x[20] = SOM::uvw[1]-ext*7.0f+extra2;

				if(1) //fade?
				{
					//REMINDER: if SOM::hmd[2] is nonzero
					//the angle comes out wrong near the
					//top unless you roll your head... I
					//can't see why this happens so THIS
					//MAY BE A CLUE THERE'S A BUG IN SOME
					//FUNDAMENTAL CODE. (it's foced to be
					//0 in som.mocap.cpp in the meantime)

					float v[3]; memcpy(v,x+13,sizeof(v));
					float d = Somvector::map(v).unit<3>().dot<3>(Somvector::map(SOM::pov));
					float r = acosf(d);
				//	float fade1 = fmodf(SOM::uvw[1]+M_PI*4,2*M_PI);
				//	float fade2 = fmodf(SOM::hmd[1]+M_PI*4,2*M_PI);
				//	float fade = 3*powf(sinf(fade2-fade1),2.0f);
					float fade = 1;

					//probably good to have some fade so
					//to not be a fright
					float rr = M_PI_4*0.5f;
					float fade3 = 6*powf(max(0,rr-r)/rr,2.0f);
					fade*=min(1.0f,fade3);

					x[0x1f] = max(0.0f,min(1.0,fade));
				}
				
				for(int i=3;i-->0;) x[13+i]+=SOM::cam[i];
			}
			else
			{
				//NOTE: a little higher so compass doesn't touch
				//top of menus
				x[15] = 0.3f*size_and_dist; //~1' //Z?
				//seems to do better lower 
				//x[14] = x[15]*0.8f; //0.75f //Y/Z?
				x[14] = x[15]*0.3f; //Y/Z?
				x[13] = x[14]*ff; //X/Y?

				//I'm not 100% positive these are rotating correctly
				//but it seems unnecessary with it more in the middle 
				//x[19]+=M_PI/18;
				//x[20] = -M_PI/5; 
			}

			x[25] = x[26] = x[27] = 0.038f*size_and_dist; //~3" (diameter)
		}
		else
		{
			//fade? this is in case switching out of VR
			x[0x1f] = 1.0f;

			x[7] = x[9] = 0.0f; //TESTING

			//x[15] = 0.0f; //set in viewport block above

			x[13] = x[14] = x[20] = 0.0f;

			//2022: I can't see where this is reset, but
			//if it's a bug it's not in the current demo
			x[25] = x[26] = x[27] = 1.0f;
		}

		if(som_scene_nwse) //NOTE: includes nwse_saturation
		{
			int s = (int)EX::INI::Detail()->nwse_saturation;

			//auto mat = *(float**)som_hacks_kf2_compass;
			auto mat = *(float**)x;
			int m = 0, n = *(int*)(mat+3);
			for(mat=*(float**)(mat+4);n-->0;mat+=8,m+=4)
			{
				memcpy(mat,som_scene_nwse+m,4*sizeof(float));

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

		if(x[0x1f]) //fade? (OpenXR)
		{
			//adapted from the sky code
			//OPAQUE (LOCK VBUFFER/DRAW)
			((BYTE(__cdecl*)(void*,void*))0x446010)(x,som_scene_transparentelements);
			DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_ZFUNC,DX::D3DCMP_LESS);
			som_scene_transparentelements->flush();
			DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_ZFUNC,DX::D3DCMP_LESSEQUAL);		
		}

		x[19] = x19; x[8] = x8; 

		//SOM::reset_projection(); //needed?
	}
	DDRAW::Direct3DDevice7->SetViewport(&vp);

	//2022: copying som_scene_swing since objects are flickering
	//in and out... not sure how long this has been like this or
	//why the 2021 demo seems unaffected
	//som_scene_ambient2 = a2;
	if(a2) som_scene_ambient2 = 1; //HACK

	sss::pop();
}

extern int som_hacks_shader_model;
typedef SOM::MPX::Layer::tile::scenery_ext som_scene_element2;
static std::vector<som_scene_element2> som_scene_transparentelements2;
static HRESULT __stdcall som_scene_4137F0_Lock2 //King's Field II
(SOM::MPX::Layer::tile *tile, DDRAW::IDirect3DVertexBuffer7 *vb, LPVOID *p
,SOM::MPX::Layer::tile::scenery::per_texture **model) 
{
	if((*model)->_transparent_indicesN) //special processing?
	{		
		if(!(*model)->triangle_indicesN) //EXPERIMENTAL
		{	
			som_MPX &mpx = *SOM::L.mpx->pointer;
			int ls = mpx[SOM::MPX::layer_selector];
			auto &l = ((SOM::MPX::Layer*)&mpx[SOM::MPX::layer_0])[ls];

			int x = tile-l.tiles, y = x/100; x-=y*100;

			som_scene_element2 el;
			el.locus[0] = 2*x; 
			el.locus[1] = tile->elevation;
			el.locus[2] = 2*y;
			el.angle = tile->rotation; el.model = *model;

			//will probably get around to pooling these... no big deal
			som_scene_transparentelements2.push_back(el);			
		}
		else assert(0); //UNIMPLEMENTED
	}

	//TODO: work with som_scene_4137F0 to not have to call Lock for every
	//tile!
	//TODO: MUST REPROGRAM Unlock CALL
	HRESULT hr = vb->Lock(0x2820,p,0); assert(!hr); return hr;
}

static void som_scene_zwritenable_text_clear() //HACK
{
	//this is called at the top of the frame to cleanup after 
	//som_scene_zwritenable_text... currently som_scene_4137F0 
	//and som_scene_swing call this so it's broken out to self
	//document its purpose
	
	//HACK: this restores LESSEQUAL mode after menus, and it's
	//also needed for when legacy bugs display a partial frame
	//on exiting the menu
	//REMINDER: som.hacks.cpp also does this for picture menus
	som_scene_zwritenable_text();
	DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_ZFUNC,DX::D3DCMP_LESSEQUAL);
}

//THESE HAPPEN IN SEQUENCE
extern bool som_scene_sky = false;
extern void __cdecl som_scene_4133E0() //2022 //DEBUGGING
{
	/*som_MPX &mpx = *SOM::L.mpx->pointer;
	int &ls = mpx[SOM::MPX::layer_selector];
	assert(0==ls);
	som_scene_4133E0_layers();*/
	return; //NOP //2023
}
static void __cdecl som_scene_4137F0() //TILES
{	
	som_kage.clear();

	 //REMOVE ME
	//2021: som.hacks.cpp refactor
	extern void som_game_once_per_scene();
	som_game_once_per_scene();
	
		if(som_scene_batchelements)
		assert(!som_scene_batchelements->se_commit);

	som_scene_lit = 0;

	som_scene_transparentelements2.clear(); //EXTENSION
		
	//TODO: confirm D3DCMP_LESSEQUAL is the default z-testing mode
	//HACK: undo som_scene_translucent_frame for function overlay?
	//if(som_hacks_tint==SOM::frame)
	{
		//REFACTOR
		//comments are in subroutine's body
		som_scene_zwritenable_text_clear();
	}
	if(!SOM::filtering) //NEW: reserve point filter for 3D things
	{
		DDRAW::Direct3DDevice7->SetTextureStageState(0,DX::D3DTSS_MINFILTER,DX::D3DTFG_POINT);
		DDRAW::Direct3DDevice7->SetTextureStageState(0,DX::D3DTSS_MAGFILTER,DX::D3DTFG_POINT);
	}

	//REMINDER: the whole of this subroutine is for the
	//sky, except at the very end (41391E) when the map
	//is drawn by 413f10, or when there's not a vbuffer
	//at [59892C] 413940
	/*
	0041391E A1 2C 89 59 00       mov         eax,dword ptr ds:[0059892Ch]  
	00413923 85 C0                test        eax,eax  
	00413925 74 09                je          00413930  
	00413927 E8 E4 05 00 00       call        00413f10  
	0041392C 83 C4 10             add         esp,10h  
	0041392F C3                   ret  
	00413930 E8 0B 00 00 00       call        00413940  
	00413935 83 C4 10             add         esp,10h  
	00413938 C3                   ret  
	*/

	if(som_scene_skyfirst()) //old way: sky comes first 
	{
		som_scene_sky = true; som_scene_skyswap::draw();
		som_scene_sky = false;
	}	
	//	extern unsigned som_hacks_fill;
	//
	// 2023: just do it every frame?
	//
	//	if(DDRAW::inStereo //menus don't reset tiles //VR
	//	&&!DDRAW::isPaused //debugging
	//	&&0!=EX::context()
	//	||som_hacks_fill==SOM::frame)
		{
			extern void som_mocap_403BB0(); //set up view frustum
			som_mocap_403BB0();
		//	som_scene_4133E0_layers(); //sets tiles_on_display	
			extern void som_MPY_4133E0_layers();
			som_MPY_4133E0_layers();
		}

	
	if(1) //som_MPY_reprogram
	{
		//2023: rewriting this subroutine to draw layers interleaved
		sss::worldtransformed = 0;
		DDRAW::Direct3DDevice7->SetTransform(DX::D3DTRANSFORMSTATE_WORLD,&DDRAW::Identity);
		som_scene_lighting(0);
		DDRAW::Direct3DDevice7->SetTextureStageState(0,DX::D3DTSS_COLOROP,sss::tex0colorop=DX::D3DTOP_MODULATE);
		DDRAW::Direct3DDevice7->SetTextureStageState(0,DX::D3DTSS_COLORARG1,sss::tex0colorarg1=D3DTA_TEXTURE);
		DDRAW::Direct3DDevice7->SetTextureStageState(0,DX::D3DTSS_COLORARG2,sss::tex0colorarg2=D3DTA_DIFFUSE);
		DDRAW::Direct3DDevice7->SetTextureStageState(0,DX::D3DTSS_ALPHAOP,sss::tex0alphaop=DX::D3DTOP_DISABLE);							  		
		//OpenGL needs this here
		//I think this is why sky makes map pieces colorkey transparent
		//DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_ALPHABLENDENABLE,0);
		//DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_SRCBLEND,sss::srcblend=DX::D3DBLEND_ONE);
		//DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_DESTBLEND,sss::destblend=DX::D3DBLEND_ZERO);	
		som_scene_alphaenable(0);

		extern void som_MPY_413f10(),som_MPY_413f10_mhm(); 

		//2024: I'm making this what players can see in case they need
		//to see the clipping geometry, otherwise it shows what's behind
		//the BSP system and doors (this removes X-Ray, todo, maybe switch)
		if(SOM::L.fill==2) som_MPY_413f10_mhm(); else som_MPY_413f10();	

		som_scene_lighting(1);
		DDRAW::Direct3DDevice7->SetTextureStageState(0,DX::D3DTSS_ALPHAOP,sss::tex0alphaop=DX::D3DTOP_SELECTARG1);
	}
	else //OLD WAY (faster?)
	{
		((void(__cdecl*)())0x413f10)();
		{
			auto &mpx2 = *SOM::L.mpx;
			som_MPX &mpx = *mpx2.pointer;
			int &ls = mpx[SOM::MPX::layer_selector]; //DEBUGGING
			assert(0==ls);

			DWORD *p = (DWORD*)&mpx;
			
			//HACK/OPTIMIZING			
		//	extern std::vector<WCHAR> som_tool_wector;
		//	//assert(som_tool_wector.empty());
			//2022: base layer may have 0 tiles... this is only a
			//problem inside menus because they don't update the
			//display list (som_state_reprogram_image forces this
			//update because the FOV may change)
			int swap = false; 

			for(int i=1;i<66;i++) //66 is layer memory //SIMPLIFY ME
			{
				if(p[i]) //assert(!p[i]);
				{
					if(100==p[i]&&100==p[i+1]&&p[i+2])
					{
						assert(0==(66-i)%10);
						i+=10-1; //assume som_MPX_413190

						//TEST: rendering with base's frustum
					//	int &ls = mpx[SOM::MPX::layer_selector];
						ls = i>=66?0:(66-i)/-10-1;
					//	if(!swap++)
					//	som_tool_wector.assign((WCHAR*)mpx2.tiles_on_display,
					//	(WCHAR*)mpx2.tiles_on_display+mpx2.tiles_to_display);
						((void(__cdecl*)())0x4133E0)(); //sets tiles_on_display
						((void(__cdecl*)())0x413F10)(); //draw
						ls = 0;
					}					   
					else assert(!p[i]); //breakpoint
				}
			}

			//crashes on reentry???
			//((void(__cdecl*)())0x4133E0)(); //sets tiles_on_display
			//if(!som_tool_wector.empty())			
		//	if(swap)
		//	wmemcpy((WCHAR*)mpx2.tiles_on_display,
		//	(WCHAR*)som_tool_wector.data(),mpx2.tiles_to_display=som_tool_wector.size());
			//som_tool_wector.clear();
		}
	}
		if(som_scene_batchelements)
		assert(!som_scene_batchelements->se_commit);
}
static void __cdecl som_scene_404070() //COINS?
{
	if(1) som_scene_money.draw();
	else ((void(__cdecl*)())0x404070)();
}
static void __cdecl som_scene_42B220() //OBJECTS
{	 		
	if(som_scene_ambient2) //2022
	{
		//shader register corruption???

		som_scene_ambient2 = 1; //HACK //TESTING
	}

	if(1) som_scene_objects.draw();
	else ((void(__cdecl*)())0x42B220)();	
}
extern float som_hacks_skyconstants[4];
extern float som_hacks_volconstants[4];
static void __cdecl som_scene_408090() //ENEMIES
{
		som_scene_44d810_flush(); //som_hacks_skyconstants[2]?

	//not covered by NPC shadows
	som_hacks_skyconstants[2] = -1; 
	DDRAW::pset9(som_hacks_skyconstants);

	if(EX::INI::Option()->do_red) //EXPERIMENTAL
	{
		SOM::Versus();
		//som_scene_enemies.red = som_scene_red_next(0x4C77C8);
		som_scene_enemies.red = som_scene_red_next((DWORD)SOM::L.ai);
	}

	if(1) som_scene_enemies.draw();
	else ((void(__cdecl*)())0x408090)();	
}
static void __cdecl som_scene_429B10() //NPCS
{
	if(EX::INI::Option()->do_red) //EXPERIMENTAL
	{
		som_scene_NPCs.red = som_scene_red_next(0X1A12DF0);
	}

	if(1) som_scene_NPCs.draw();
	else ((void(__cdecl*)())0x429B10)();		
}
static void __cdecl som_scene_4103B0() //ITEMS
{
		som_scene_44d810_flush(); //som_hacks_skyconstants[2]?

	//are covered by NPC shadows
	som_hacks_skyconstants[2] = 1; 
	DDRAW::pset9(som_hacks_skyconstants);	
	
	if(1) som_scene_items.draw();
	else ((void(__cdecl*)())0x4103B0)();
}
static void __cdecl som_scene_42EC40() //MAGIC (ALL SFX)
{	
		som_scene_44d810_flush(); //som_hacks_skyconstants[2]?

	//not covered by NPC shadows
	som_hacks_skyconstants[2] = -1; 
	DDRAW::pset9(som_hacks_skyconstants);

	//NOTE: doesn't include screen SFXs
	if(1) som_scene_magics.draw();
	else ((void(__cdecl*)())0x42EC40)();	

		//ATTENTION
		//this is strategically placed here
		//to come after the main drawing loops
		//and so reset this to the default value
		//so no further "holds" are placed
		som_scene_batchelements_hold = ~0u; //YUCK

  /////// PIGGYBACKING (draw deferred sky) //////////

	if(!som_scene_skyfirst()) //new way: opaque->sky->transparent elements
	{
			//HACK? sky appears to be drawn in front of elements?
			//(I guess it literally is, but som_scene_sky cannot
			//be nonzero here)
			som_scene_44d810_flush();

		som_scene_sky = true; som_scene_skyswap::draw();
		som_scene_sky = false;
	}	
}
static void __cdecl som_scene_42DC20() //MAGIC (SCREEN EFFECTS)
{
		som_scene_44d810_flush(); //HACK: last in 402400

	//2020: I don't know if depth-tests are required at all here, but
	//make sure this covers the compass/swing model... I think in the
	//original days maybe the depth-buffer was cleared, but maybe not
	//I seem to remember the HUD being in front of the screen effects
	DX::D3DVIEWPORT7 vp;
	DDRAW::Direct3DDevice7->GetViewport(&vp);
	float swap[2] = {vp.dvMinZ,vp.dvMaxZ};
	vp.dvMinZ = 0;
	vp.dvMaxZ = EX_INI_Z_SLICE_1;
	DDRAW::Direct3DDevice7->SetViewport(&vp);
	{	
		((void(__cdecl*)())0x42DC20)();

		if(som_scene_batchelements)
		assert(!som_scene_batchelements->se_commit);
	}
	vp.dvMinZ = swap[0];
	vp.dvMaxZ = swap[1];
	DDRAW::Direct3DDevice7->SetViewport(&vp);

	//there are up to 5 (or 6?) effects objects starting at 1C43E48
	//that are each 1592B (638h.) this probably includes all of the
	//sprite effects, but doesn't include menu overlays/backgrounds 
	//magics appear to be able to have two parts. So that 2 buffers
	//are used up (e.g. bubbles with fullscreen filter)

	//THE MAGIC TEXTURES ARE HARDCODED!
	//0042D5B0 81 EC 04 01 00 00    sub         esp,104h
	//bubbles loaded at start (207.txr)
	//0042D5BE 68 CF 00 00 00       push        0CFh
	//0042D606 A3 60 5D C4 01       mov         dword ptr ds:[01C45D60h],eax
	//shields loaded at start (208.txr through 209.txr)
	//0042D5F2 68 D0 00 00 00       push        0D0h
	//0042D606 A3 60 5D C4 01       mov         dword ptr ds:[01C45D60h],eax

	/*
	0042DC20 53                   push        ebx  
	0042DC21 8B 1D 6C 80 45 00    mov         ebx,dword ptr ds:[45806Ch]  
	0042DC27 56                   push        esi  
	0042DC28 57                   push        edi  
	0042DC29 BE 48 3E C4 01       mov         esi,1C43E48h  
	0042DC2E BF 05 00 00 00       mov         edi,5  
	0042DC33 80 3E 00             cmp         byte ptr [esi],0  
	0042DC36 74 5A                je          0042DC92  
	0042DC38 33 C0                xor         eax,eax  
	0042DC3A 8A 46 01             mov         al,byte ptr [esi+1]  
	0042DC3D 83 F8 10             cmp         eax,10h  
	0042DC40 77 49                ja          0042DC8B  
	0042DC42 FF 24 85 A0 DC 42 00 jmp         dword ptr [eax*4+42DCA0h]
	///
	0x0042DCA0  0042dc49 0042dc54 0042dc49 0042dc54  
	0x0042DCB0  0042dc5f 0042dc5f 0042dc5f 0042dc6a  
	0x0042DCC0  0042dc6a 0042dc6a 0042dc75 0042dc75  
	0x0042DCD0  0042dc75 0042dc80 0042dc80 0042dc80  
	0x0042DCE0  0042dc80 90909090 90909090 90909090
	///
	//white fade out, etc.
	//ESI is 0x01c43e48 (reserved?)
	0042DC49 56                   push        esi  
	0042DC4A E8 31 04 00 00       call        0042E080  
	0042DC4F 83 C4 04             add         esp,4  
	0042DC52 EB 3E                jmp         0042DC92
	//IDENTICAL TO ABOVE???
	//fade to white
	//ESI is 0x01c43e48 (same as above)
	0042DC54 56                   push        esi  
	0042DC55 E8 26 04 00 00       call        0042E080  
	0042DC5A 83 C4 04             add         esp,4  
	0042DC5D EB 33                jmp         0042DC92
	//IDENTICAL TO BELOW???
	0042DC5F 56                   push        esi  
	0042DC60 E8 5B 04 00 00       call        0042E0C0  
	0042DC65 83 C4 04             add         esp,4  
	0042DC68 EB 28                jmp         0042DC92  
	//magic	(bubbles? first)
	//ESI is 0x01c44ab8 (reserved?)
	0042DC6A 56                   push        esi  
	0042DC6B E8 50 06 00 00       call        0042E2C0  
	0042DC70 83 C4 04             add         esp,4  
	0042DC73 EB 1D                jmp         0042DC92  
	//
	0042DC75 56                   push        esi  
	0042DC76 E8 95 07 00 00       call        0042E410  
	0042DC7B 83 C4 04             add         esp,4  
	0042DC7E EB 12                jmp         0042DC92  
	//IDENTICAL TO magic CASE???
	//magic (blue fullscreen filter)
	//ESI is 0x01c450f0 (reserved?)
	0042DC80 56                   push        esi  
	0042DC81 E8 3A 04 00 00       call        0042E0C0  
	0042DC86 83 C4 04             add         esp,4  
	0042DC89 EB 07                jmp         0042DC92  
	//outputs "Error ScrnEfct_Render()"
	0042DC8B 68 20 E6 45 00       push        45E620h  
	0042DC90 FF D3                call        ebx  
	//
	0042DC92 81 C6 38 06 00 00    add         esi,638h  
	0042DC98 4F                   dec         edi  
	0042DC99 75 98                jne         0042DC33  
	0042DC9B 5F                   pop         edi  
	0042DC9C 5E                   pop         esi  
	0042DC9D 5B                   pop         ebx  
	0042DC9E C3                   ret  
	*/
}					

extern int som_scene_volume;
namespace som_scene_shadows
{
	static bool on()
	{	
		som_scene_gamma_n = false;

		//if(shadow_pass==transparency_pass)
		som_scene_volume = 0; 

		//sss::zwriteenable = 0;
		//DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_ZWRITEENABLE,0);	
		assert(!sss::zwriteenable);
		//hack: make DDRAW::vsWorldView vsView
		sss::worldtransformed = 0; 
		DDRAW::Direct3DDevice7->SetTransform(DX::D3DTRANSFORMSTATE_WORLD,&DDRAW::Identity);			
		sss::alphaenable = 1;
		DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_ALPHABLENDENABLE,1);
		sss::srcblend = DX::D3DBLEND_SRCALPHA;
		sss::destblend = DX::D3DBLEND_INVSRCALPHA; 						  		
		DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_SRCBLEND,sss::srcblend);
		DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_DESTBLEND,sss::destblend);	

		//assert(shadow==0);
		//call 4487D0: texture* is at 0x1D3D2F0+A0*shadow	   		 
	//	sss::texture = 0; 
	//	DDRAW::Direct3DDevice7->SetTexture(0,*(DX::IDirectDrawSurface7**)0x1D3D2F0); 
		DDRAW::Direct3DDevice7->SetTextureStageState(0,DX::D3DTSS_ADDRESS,DX::D3DTADDRESS_CLAMP);

		DDRAW::vs = DDRAW::ps = 7;
		//I think does no good inside of som_scene_44d7d0
		//190: D3DRS_COLORWRITEENABLE1 (switch MRT to texture 1)
		//DDRAW::Direct3DDevice7->SetRenderState((DX::D3DRENDERSTATETYPE)190,0);
		return true;
	}
	static bool off()
	{
		//I think does no good inside of som_scene_44d7d0
		//DDRAW::Direct3DDevice7->SetRenderState((DX::D3DRENDERSTATETYPE)190,0xF);
		return false;
	}
	static void draw(int txr, float (&worldxform)[4][4], DX::D3DCOLOR color)
	{
		EX::INI::Adjust ad;

		int passes = 1;

		float tween = 1.0f;
		bool first = false;
		float _kage_bbox2[2][6] = {};
		float *kage_bbox = nullptr;
		SOM::Animation *ba = nullptr;		
		if(txr>=4096) //2024: kage2?
		{
			int k = txr-4096;
			if(k<(int)som_kage.size()) 
			{
				passes = 2;

				SOM::MDL *l = som_kage[k];

				ba = l->find_first_kage();

				float t = som_kage[k]->f;
				t = (t-ba[0].t)/(ba[1].t-ba[0].t);
				t = 1-max(0,min(1,t));
				
				if(1) //try lightest first?
				{
					if(t>0.5f) //try darkest first?
					{
						t = 1-t; first = true;
					}
				}
				else //try darkest first?
				{
					if(t<0.5f) 
					{
						t = 1-t; first = true;
					}
				}
				
				//this is a contrast filter to eliminate visible strobing
				//t = t*0.9f+0.1f;
				//this formulation seems to work really well
				float c = fabs(t-0.5f); t = (0.5f-t)*c*c+t;

				tween = t;

				//TODO: do this in SOM::MDL::control_point?
			//	int skip = l->c==0&&l->mdl_data->hard_anim_buf; //YUCK
				enum{ skip=0 };

				memcpy(_kage_bbox2[0],ba[0].bbox,sizeof(_kage_bbox2[0]));	
				memcpy(_kage_bbox2[1],ba[1].bbox,sizeof(_kage_bbox2[0]));	
				float cp[3] = {};
				//this way stutters more
			//	l->control_point(cp,l->c,l->f,-1,true);
				//TODO? maybe x2mdl should consult the CP file
				//when figuring out the center of its shadow??
				//(it seems there's only be a correspondance!)
				l->control_point(cp,l->c,ba[0].t+skip,-1,!true);
				_kage_bbox2[0][0]-=cp[0];
				_kage_bbox2[0][2]-=cp[1];
				_kage_bbox2[0][4]-=cp[2];		
				l->control_point(cp,l->c,ba[1].t+skip,-1,!true);				
				_kage_bbox2[1][0]-=cp[0];
				_kage_bbox2[1][2]-=cp[1];
				_kage_bbox2[1][4]-=cp[2];

				if(sss::texture!=txr) //2023: som.kage.cpp
				{
					sss::texture = txr;
					if(tween)
					DDRAW::Direct3DDevice7->SetTexture(0,ba[first].texture);
				//	DDRAW::Direct3DDevice7->SetTexture(1,ba[1].texture);
				}
			}
		}
		else //classic way
		{
			if(sss::texture!=txr)
			{
				sss::texture = txr;
				DDRAW::Direct3DDevice7->SetTexture(0,SOM::L.textures[txr].texture);	
			//	DDRAW::Direct3DDevice7->SetTexture(1,SOM::L.textures[txr].texture);	
			}
			kage_bbox = txr?SOM::L.textures[txr].kage_bbox:0; //single shadow icon?
		}
		float *bb = kage_bbox?kage_bbox:_kage_bbox2[first];
		for(int pass=1;pass<=2;pass++)
		{
			if(pass==2)
			{
				bb = _kage_bbox2[!first];				
				if(tween=1-tween)
				DDRAW::Direct3DDevice7->SetTexture(0,ba[pass==1?first:!first].texture);
			}			
			if(!tween) continue;
		
			float x = EX_INI_SHADOWRADIUS, //0.512f
			t = EX_INI_SHADOWVOLUME, b = -EX_INI_SHADOWVOLUME; //0.2
				
			float sx = worldxform[1][1]; //*1024; //MDL
			float rx = 1/sx*worldxform[0][0];
			float rz = 1/sx*worldxform[2][0];
			sx*=ad->npc_shadows_multiplier();

			float tx,sz = sx; if(bb[1]) 
			{
				//tx = 0.5/(sx/EX_INI_SHADOWUVBIAS);
			//	tx = 5*sx*bb[1]/(1+0.15); //X2ICO_SHADOW_EXPAND?
				tx = 5*sx*bb[1];

				bb[0]*=sx; sx*=bb[1];
				bb[4]*=sz; sz*=bb[5];
			}
			else 
			{
				tx = 0.5/(sx/EX_INI_SHADOWUVBIAS);

				sz*=x; sx*=x;
			}
		
			float fade = color>>24; assert(color<<8==0xffffff00);
		//	fade = powf(fade/255,0.55f)*255;
		//	int alpha = sqrtf(ad->npc_shadows_modulation()*0.5f)*fade*sqrtf(tween);
			int alpha = ad->npc_shadows_modulation()*fade*sqrtf(tween);
			//color = alpha<<24|(DWORD)ad->npc_shadows_saturation();
			DWORD color2 = alpha<<24|(DWORD)ad->npc_shadows_saturation();
		//	if(pass==1) color2|=0xff; 
		//	if(pass==2) color2|=0xff0000;

			BYTE *c = (BYTE*)&color2; if(fade!=0xFF) //white ghost?		
			{
				float f = 1-fade/255.0f;
				static const float l[3] = { 0.00087058632f,0.00277254292f,0.00027843076f};
				float gray = l[0]*c[0]+l[1]*c[1]+l[2]*c[2];
				for(int i=3;i-->0;)
				{
					float g = (c[i]+(gray-c[i])*f)+f*0.6f; //som.shader.cpp
					c[i] = (BYTE)min(255,g);
				}
			}

			float v[8][12] = //const
			{
			{-sx,t,-sz}, {+sx,t,-sz}, //0,1
				{-sx,t,+sz}, {+sx,t,+sz}, //2,3
			{-sx,b,-sz}, {+sx,b,-sz}, //4,5
				{-sx,b,+sz}, {+sx,b,+sz}, //6,7
			};
			static const WORD i[6*6] = 
			{
			1,0, 2,2 ,3,1, //top
			4,5, 6,6 ,5,7, //bottom
			2,0, 6,6 ,0,4, //left
			1,3, 7,7 ,5,1, //right
			3,2, 6,6 ,7,3, //front
			0,1, 5,5 ,4,0, //back
			};						

			/*DSTEREO_SKY seems to address this
			//HACK: the shift in perspective sands the sides off 
			//the shadows. This pulls the UVs in while the scale
			//is enlarged to compensate inside som_scene_shadows
			if(DDRAW::inStereo) sx+=DDRAW::stereo; //0.1f; */

			/*update shadow matrix?
			{			
				float center[4];
				Somvector::multiply<4>(worldxform[3],SOM::analogcam,center);
				for(int i=0;i<3;i++) 
				{
					float col[3] = {SOM::shadowcam[0][i],SOM::shadowcam[1][i],SOM::shadowcam[2][i]};
					SOM::shadowcam[3][i] = -(col[0]*center[0]+col[1]*center[1]+col[2]*center[2]);
				}
				DDRAW::pset9(SOM::shadowcam[3],1,DDRAW::psTexMatrix[3]);
				DDRAW::vset9(SOM::shadowcam[0],4,DDRAW::vsTexMatrix[0]);
			}*/
		
			const DWORD fvf = D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX2|D3DFVF_TEXCOORDSIZE4(0)|D3DFVF_TEXCOORDSIZE4(1);

			//2021: som_scene_batchelements is using DrawIndexedPrimitive ATM
			void *swap = DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_DRAWINDEXEDPRIMITIVE_HACK,0);
			{
				for(int i=8;i-->0;) //center
				{
					(DWORD&)v[i][3] = color2;
					v[i][4] = worldxform[3][0]+(-bb[0]*-rx-bb[4]*-rz);				
					v[i][5] = worldxform[3][1]-som_scene_shadowdecal;
					v[i][6] = worldxform[3][2]+(-bb[0]*rz-bb[4]*-rx);
					v[i][7] = tx;
					v[i][8] = rx;
					v[i][9] = rz;
					v[i][10] = sx;
					v[i][11] = sz;					
				}
				//NOTE: SOM doesn't use DrawIndexedPrimitive (except in software mode?)			
				DDRAW::Direct3DDevice7->DrawIndexedPrimitive(DX::D3DPT_TRIANGLELIST,fvf,(LPVOID)v,8,(LPWORD)i,6*6,0);
			}
			DDRAW::hack_interface(DDRAW::DIRECT3DDEVICE7_DRAWINDEXEDPRIMITIVE_HACK,swap);
		}
	}
	static void draw(float radius)
	{			
		float tmp[3],worldxform[4][4] = {}; 
		//2020: can't hurt to do all of these
		worldxform[0][0] = worldxform[1][1] = 
		worldxform[2][2] = radius/EX_INI_SHADOWRADIUS; ///1024; //MDL
		worldxform[3][3] = 1;
		memcpy(worldxform[3],SOM::motions.place_shadow(tmp),3*sizeof(float));
		//HACK: for some reason the elevation tends to be 
		//0.0001 off the floor, which is also the amount that
		//the shadows are raised, but I've noticed that the matrix
		//is already 0.02 off the floor, so changed som_scene_shadowdecal
		//to be a constant of 0.0201... it's possible to knock out the 0.0001
		//code in some.scene.cpp
		worldxform[3][1]+=som_scene_shadowdecal-0.0001f;

		float rx = cos(-SOM::L.pcstate[4]);
		float rz = sin(-SOM::L.pcstate[4]);
		worldxform[0][0] = rx; worldxform[0][2] = -rz;
		worldxform[2][0] = rz; worldxform[2][2] = rx;

		draw(0,worldxform,0xffffffff);		
	}
}
//transparency/shadows sort call
static bool som_scene_44d5a0_psort(som_scene_element *a, som_scene_element *b)
{
	return a->depth_44D5A0>b->depth_44D5A0; 
}
static float som_scene_depth2(float a[3], float b[3])
{
	//float o = 0; for(int i=0;i<3;i++) o+=powf(a[i]-b[i],2); return o;
	return (a[0]-b[0])*(a[0]-b[0])+(a[1]-b[1])*(a[1]-b[1])+(a[2]-b[2])*(a[2]-b[2]);
}
static void __cdecl som_scene_44d5a0(som_scenery *ts) //depth-sort
{
	//TODO! MAKE lightselector MORE ACCURATE
	int todolist[SOMEX_VNUMBER<=0x1020704UL];

	EX::INI::Option op;

	float eye[3];

	int i,iN;
	som_scene_elements &ses = *ts->ses;	
	if(op->do_alphasort&&som_hacks_shader_model)
	{
		extern DWORD som_bsp_add_vbufs(som_MDL::vbuf**,DWORD);
		ts->se_commit = som_bsp_add_vbufs(ses,ts->se_commit);
	}
	else
	{
		float *p = SOM::xyz;
		//float eye[3] = {p[0],p[1]+SOM::eye[1],p[2]};
		eye[0] = p[0];
		eye[1] = p[1]+SOM::eye[1];
		eye[2] = p[2];
		float *prev = (float*)&i;
		for(i=0,iN=ts->se_commit;i<iN;i++,prev=p)
		{
			switch(ses[i]->fmode)
			{			
			/*mode 2??? I think this is averaging a quad (in screen space)
			0044D6D4 D9 40 7C             fld         dword ptr [eax+7Ch]  
			0044D6D7 D8 40 5C             fadd        dword ptr [eax+5Ch]  
			0044D6DA D8 40 3C             fadd        dword ptr [eax+3Ch]  
			0044D6DD D8 40 1C             fadd        dword ptr [eax+1Ch]  
			0044D6E0 D8 0D 84 83 45 00    fmul        dword ptr ds:[458384h]  //0.25
			0044D6E6 D9 58 08             fstp        dword ptr [eax+8]
			*/
			case 2: assert(0); continue;
			case 0: assert(0); //does mode 0 differ from 1?
			//HACK: this is a D3DLVERTEX, should average 2 of its corners
			case 1: p = ses[i]->worldxform[0]; break;
			case 3:
			case 4: p = ses[i]->lightselector; break;
			}

			//float depth = Somvector::measure<3>(p,eye);
			float depth2 = som_scene_depth2(p,eye);
			if(depth2<6*6) 
			{
				//TODO? negate if behind
			}
			if(fabsf(*prev-depth2)<0.1f*0.1f) //stable_sort?
			{
				//try to group composite elements together so they won't
				//fight
				depth2 = *prev; p = prev;
			}
			ses[i]->depth_44D5A0 = depth2;
		}
		//TODO: std::sort would probably work fine if the lightselector
		//fields were more accurate, etc. for most purposes, yet stable
		//sort remains safer if centers overlap/are very close together
		std::stable_sort(ses,ses+iN,som_scene_44d5a0_psort);
	}

	//som_MPX &mpx = *SOM::L.mpx->pointer;
	//float pos2[3]; 
	//float *vb = (float*)mpx[SOM::MPX::vertex_pointer];		
	for(i=0,iN=som_scene_transparentelements2.size();i<iN;i++)
	{
		som_scene_element2 &el = som_scene_transparentelements2[i];
				
		if(op->do_alphasort&&som_hacks_shader_model)
		{
			//2022: transparency sort/split?
			extern void som_bsp_add_tiles(som_scene_element2*,size_t);
			som_bsp_add_tiles(&el,iN);
			break;
		}
		else
		{
			float *pos = el.locus; /*if(0) //UNUSED
			{
				//light_selector
				float *v = vb+5*el.model->vcolor_indices[0][0];

				//temporary fix for KF2 water levels... less good for
				//the waterfall
				memcpy(pos2,el.locus,12);
				//pos2[0]+=v[0];
				pos2[1]+=v[1]; 
				//pos2[2]+=v[2];

				pos = pos2;
			}*/
			//TODO: retrieve locus from MPX vertex buffer
			//TODO: wouldn't hurt to sort these, even though
			//they're already sorted with regard to tile centers
			//el.depth_44D5A0 = Somvector::measure<3>(pos,eye);
			el.depth_44D5A0 = som_scene_depth2(pos,eye);
		}
	}

	if(op->do_alphasort&&som_hacks_shader_model)
	{
		som_scene_transparentelements2.clear(); //2022
	}
	else
	{
		//REMOVE ME??
		//TODO: sort pointers (according to texture groups?)
		//if(1) //TESTING (waterfall)
		std::stable_sort(som_scene_transparentelements2.begin(),som_scene_transparentelements2.end());
	}
}
//transparency/shadows draw call
extern int som_scene_volume = 0;
extern void som_scene_volume_select2(SOM::VT *v)
{
	assert(v->frame!=SOM::frame); //hack?
	
	//som_game_once_per_event_cycle?
	v->frame = SOM::frame; 

	//I think SOM::VT is unnecessary?
	//it does cache the calculations?
	EX::INI::Volume vt;
	float vg = (float)v->group;
	v->sides = (int)vt->volume_sides(vg);
	v->depth = 1/vt->volume_depth(vg);
	v->power = vt->volume_power(vg);
			
	//MLAA mode is required to reuse
	//all 4 components of psColorkey
	if(SOM::VT::fog_register) 
	{
		DWORD rgb = (DWORD)vt->volume_fog_saturation(vg);
		v->fog[0] = (rgb>>16&0xff)/255.0f;
		v->fog[1] = (rgb>>8&0xff)/255.0f;
		v->fog[2] = (rgb&0xff)/255.0f;
		v->fog[3] = vt->volume_fog_factor(vg);
	}
}
extern int som_scene_volume_select(DWORD txr, int vg=-1)
{
	#ifndef NDEBUG //TESTING
	if(1&&som_scene_batchelements) return -1;
	#else
	//batch?
	int todolist[SOMEX_VNUMBER<=0x1020704UL];
	#endif

	if(SOM::volume_textures&&txr!=65535)	
	if(SOM::VT*v=(*SOM::volume_textures)[txr])
	{	
		if(v->group==vg) return vg;

		if(v->frame!=SOM::frame) //hack?
		{
			som_scene_volume_select2(v);
		}
		som_hacks_volconstants[2] = v->depth; //skyconstants
		som_hacks_volconstants[3] = v->power; //skyconstants
		DDRAW::pset9(som_hacks_volconstants,1,DDRAW::psConstants10);
		if(SOM::VT::fog_register) 
		DDRAW::pset9(v->fog,1,SOM::VT::fog_register); //DDRAW::psColorkey

		som_scene_volume = v->sides; return v->group;
	}
	som_scene_volume = 0; return -1;
}
extern bool som_scene_shadow = false;
static void som_scene_413f10_maybe_flush
(SOM::MPX::Layer::tile::scenery::per_texture *pt)
{
	//004140E4 A1 68 A9 9A 01       mov         eax,dword ptr ds:[019AA968h]  
	//004140E9 03 C8                add         ecx,eax  
	//004140EB 81 F9 80 03 00 00    cmp         ecx,380h  	
	DWORD &vbN = SOM::L.mpx_vbuffer_size; //896
	//004140F1 8B 0D 20 A9 58 00    mov         ecx,dword ptr ds:[58A920h] 
	//00414101 81 FA 80 0A 00 00    cmp         edx,0A80h 
	DWORD &ibN = SOM::L.mpx_ibuffer_size; //2688

	if(pt)
	{
		assert(pt->texture<1024); //65535?

		som_MPX &mpx = *SOM::L.mpx->pointer;
		DWORD txr = ((WORD*)mpx[SOM::MPX::texture_pointer])[pt->texture];
		DWORD sss = sss::texture;
		assert((int)sss<1024);

		//2022: this was overflowing the ibuffer array because it didn't
		//factor in transparent_indicesN. because this code is scanning
		//pt it can't be bypassed
		int indices2 = pt->triangle_indicesN+pt->_transparent_indicesN;

		if(txr!=sss||896<vbN+pt->vcolor_indicesN||2688<ibN+indices2)
		{
			if(ibN) som_scene_413f10_maybe_flush(0); 
			
			if(txr!=sss&&txr<1024)
			{	
				sss::texture = txr;
				
				DDRAW::IDirectDrawSurface7 *t = SOM::L.textures[txr].texture;
				DDRAW::Direct3DDevice7->SetTexture(0,t);

				//NOTE: doing after SetTexture because it had set psColorkey 
				som_scene_volume_select(txr);
			}
		}
		return;
	}
	else if(!ibN) return;

	//NOTE: I hit this with ibN being over limit. I think fixing
	//code at 004143e0 should prevent it... I'm thinking this has
	//to be related to the crashing I'm seeing since adding thread
	//stuff to som.MPX.cpp but I'm not sure why it took so long to
	//hit this so I'm not sure
	assert(vbN<=896&&ibN<=2688); //2022

	//00414169 8D 14 4D 68 7F 5A 00 lea         edx,[ecx*2+5A7F68h]
	WORD *ib = SOM::L.mpx->ibuffer;
	auto *vb = SOM::L.mpx->vbuffer;	
	vb->Unlock();	
	DDRAW::Direct3DDevice7->DrawIndexedPrimitiveVB
	(DX::D3DPT_TRIANGLELIST,vb,0,vbN,ib,ibN,0);	
	vbN = ibN = 0;
}
static void som_scene_413f10_mhm(float xyz[3], int a, SOM::MHM &h, float* &p)
{
	DWORD &ibN = SOM::L.mpx_ibuffer_size;
	DWORD &vbN = SOM::L.mpx_vbuffer_size; 

	int iN = 0;
	int vN = h.verts;
	for(int n,i=h.polies;i-->0;iN+=n)
	{
		n = h.poliesptr[i].verts; n+=2*(n-3);
	}	
		
	if(896<vbN+vN||2688<ibN+iN)
	{
		som_scene_413f10_maybe_flush(0);
	}	
	if(!ibN)
	{
		DWORD f = DDLOCK_DISCARDCONTENTS|DDLOCK_WAIT|DDLOCK_WRITEONLY;
		SOM::L.mpx->vbuffer->Lock(f,(void**)&p,0);	
	}
	assert(ibN<=2688);

	WORD *q = SOM::L.mpx->ibuffer+ibN; //emulate 413f10

	int vbase = vbN;
	for(int i=h.polies;i-->0;)
	{
		auto &pi = h.poliesptr[i];
		auto *vp = pi.vertsptr;		
		*q++ = vbase+*vp++;
		*q++ = vbase+*vp++;
		*q++ = vbase+*vp++;
		if(pi.verts>3)
		{
			auto *v0 = vp-3;
			for(int j=pi.verts-3;j-->0;vp++)
			{
				*q++ = vbase+*v0; 
				*q++ = vbase+vp[-1];
				*q++ = vbase+*vp;
			}
		}
	}
	ibN+=iN;
	vbN+=vN;

	for(auto*vp=h.vertsptr;vN-->0;vp+=3,p+=6)
	{
		float *v = vp;
		p[0] = xyz[0];
		p[1] = xyz[1]+v[1];
		p[2] = xyz[2];
		switch(a) //might want to unroll this part 
		{
		case 0: p[0]+=v[0]; p[2]+=v[2]; break; //S
		case 1: p[0]-=v[2]; p[2]+=v[0]; break; //W 
		case 2: p[0]-=v[0]; p[2]-=v[2]; break; //N
		case 3: p[0]+=v[2]; p[2]-=v[0]; break; //E
		}
	//	(DWORD&)p[3] = 0xff2787a7; //clip_model_pixel_value
		(DWORD&)p[3] = 0xff3ACAFB; //observed SOM_MAP value
		p[4] = 0.0f; //u
		p[5] = 0.0f; //v
	}
}
extern float som_scene_413f10_uv = 0; //testing
static void som_scene_413f10(float xyz[3], int a,
SOM::MPX::Layer::tile::scenery::per_texture *pt, float* &p)
{	
	som_MPX &mpx = *SOM::L.mpx->pointer;

	//transparent triangles are hidden like so	
	if(int i=pt->_transparent_indicesN)
	{
		if(!pt->triangle_indicesN) //EXPERIMENTAL
		{	
			som_scene_element2 el = {{xyz[0],xyz[1],xyz[2]},a,pt};

			//will probably get around to pooling these... no big deal
			som_scene_transparentelements2.push_back(el);			
		}
		else assert(0); return; //UNIMPLEMENTED
	}

	som_scene_413f10_maybe_flush(pt);

	int tuv = 0; float tu,tv;
	if(SOM::material_textures)
	{
		DWORD txr = ((WORD*)mpx[SOM::MPX::texture_pointer])[pt->texture];

		if(txr<1024) if(auto*mt=(*SOM::material_textures)[txr])
		{
			if(tuv=mt->mode&(8|16))
			{
				tu = mt->data[7]*som_scene_413f10_uv;
				tv = mt->data[8]*som_scene_413f10_uv;
			}
		}
	}

	DWORD &ibN = SOM::L.mpx_ibuffer_size;
	DWORD &vbN = SOM::L.mpx_vbuffer_size; 
	float *vp = (float*)mpx[SOM::MPX::vertex_pointer];

	if(!ibN)
	{
		DWORD f = DDLOCK_DISCARDCONTENTS|DDLOCK_WAIT|DDLOCK_WRITEONLY;
		SOM::L.mpx->vbuffer->Lock(f,(void**)&p,0);	
	}
	assert(ibN<=2688);

	WORD *q = SOM::L.mpx->ibuffer+ibN; //emulate 413f10

	int i = pt->triangle_indicesN;
	ibN+=i;
	memcpy(q,pt->triangle_indices,2*i);		
	WORD vo = vbN;
	for(;i-->0;q++)
	*q+=vo;
	i = pt->vcolor_indicesN;
	vbN+=i;
	DWORD *vc = *pt->vcolor_indices;		
	for(;i-->0;vc+=2,p+=6)
	{
		float *v = vp+5*vc[0];
		p[0] = xyz[0];
		p[1] = xyz[1]+v[1];
		p[2] = xyz[2];
		switch(a) //might want to unroll this part 
		{
		case 0: p[0]+=v[0]; p[2]+=v[2]; break; //S
		case 1: p[0]-=v[2]; p[2]+=v[0]; break; //W 
		case 2: p[0]-=v[0]; p[2]-=v[2]; break; //N
		case 3: p[0]+=v[2]; p[2]-=v[0]; break; //E
		}
		(DWORD&)p[3] = vc[1];
		p[4] = v[3];
		p[5] = v[4];
		if(tuv) switch(tuv&16?a:0) //2020
		{				
		case 0: p[4]+=tu; p[5]+=tv; break; //KF2
		case 1: p[4]-=tu; p[5]+=tv; break; //UNTESTED
		case 2: p[4]-=tu; p[5]-=tv; break; //UNTESTED
		case 3: p[4]+=tu; p[5]-=tv; break; //UNTESTED
		}
	}
}
static void som_scene_413f10(float &depth,
som_scene_element2* &se2, som_scene_element2*send2)
{	
	som_MPX &mpx = *SOM::L.mpx->pointer;	

	if(!sss::alphaenable)
	DDRAW::Direct3DDevice7->SetRenderState
	(DX::D3DRENDERSTATE_ALPHABLENDENABLE,sss::alphaenable=1); //???
	DDRAW::Direct3DDevice7->SetRenderState
	(DX::D3DRENDERSTATE_SRCBLEND,sss::srcblend=DX::D3DBLEND_SRCALPHA);	
	//DDRAW::Direct3DDevice7->SetRenderState
	//(DX::D3DRENDERSTATE_DESTBLEND,sss::destblend=destblend);
	if(1!=sss::tex0alphaop)
	DDRAW::Direct3DDevice7->SetTextureStageState
	(0,DX::D3DTSS_ALPHAOP,sss::tex0alphaop=DX::D3DTOP_DISABLE); //unlit	

	DWORD &cop = sss::tex0colorop; //4
	DWORD &ca1 = sss::tex0colorarg1; //2
	DWORD &ca2 = sss::tex0colorarg2; //1
	if(cop!=4) DDRAW::Direct3DDevice7->SetTextureStageState(0,DX::D3DTSS_COLOROP,cop=DX::D3DTOP_MODULATE);
	if(ca1!=2) DDRAW::Direct3DDevice7->SetTextureStageState(0,DX::D3DTSS_COLORARG1,ca1=D3DTA_TEXTURE);
	if(ca2!=0) DDRAW::Direct3DDevice7->SetTextureStageState(0,DX::D3DTSS_COLORARG2,ca2=D3DTA_DIFFUSE);

	//2022: this should be off (not on?) does som.hacks.cpp manage this?
	//assert(sss::lighting_current);
//	DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_LIGHTING,1); //1?
	
	assert(sss::colorkeyenable&&!sss::zwriteenable);

	if(sss::worldtransformed)
	{
		sss::worldtransformed = 0;
		DDRAW::Direct3DDevice7->SetTransform(DX::D3DTRANSFORMSTATE_WORLD,&DDRAW::Identity);
	}

	float *p; WORD *q;
	DWORD &ibN = SOM::L.mpx_ibuffer_size; assert(!ibN);
	DWORD &vbN = SOM::L.mpx_vbuffer_size; assert(!vbN);
	float *vp = (float*)mpx[SOM::MPX::vertex_pointer];

	DWORD destblend = ~0u;

	int tex = -1, tuv = 0; float tu,tv; 

	WORD *ib = SOM::L.mpx->ibuffer; do //emulate 413f10
	{
		//transparent triangles are hidden like so
		//int i = 2*se2->model->triangle_indicesN;
		int i = se2->model->_transparent_indicesN; if(!i) 
		{
			//paranoia: relying on !ibN to lock
			assert(0); continue; 
		}

		som_scene_413f10_maybe_flush(se2->model);

		if(tex!=se2->model->texture)
		{
			tuv = 0; tex = se2->model->texture; 

			DWORD blend = DX::D3DBLEND_INVSRCALPHA;

			if(SOM::material_textures)
			{
				DWORD txr = ((WORD*)mpx[SOM::MPX::texture_pointer])[tex];
				//assert(txr==sss::texture);

				if(txr<1024)
				if(SOM::MT *mt=(*SOM::material_textures)[txr])
				{
					if(tuv=mt->mode&(8|16))
					{
						tu = mt->data[7]*som_scene_413f10_uv;
						tv = mt->data[8]*som_scene_413f10_uv;
					}
				
					if(mt->mode&0x100) blend = DX::D3DBLEND_ONE;
				}
			}

			if(destblend!=blend) //NOTE: not relying on sss::destblend
			{
				destblend = blend;

				DDRAW::Direct3DDevice7->SetRenderState
				(DX::D3DRENDERSTATE_DESTBLEND,sss::destblend=destblend);
			}
		}

		if(!ibN)
		{
			DWORD f = DDLOCK_DISCARDCONTENTS|DDLOCK_WAIT|DDLOCK_WRITEONLY;
			SOM::L.mpx->vbuffer->Lock(f,(void**)&p,0);
			q = ib;
		}
		assert(ibN+i<=2688);

		ibN+=i;
		memcpy(q,se2->model->triangle_indices,2*i);		
		WORD vo = vbN;
		for(;i-->0;q++)
		*q+=vo;
		i = se2->model->vcolor_indicesN;
		vbN+=i;
		int a = se2->angle;
		float l[3],*cp = se2->locus; //???
		l[0] = cp[0];
		l[1] = cp[1];
		l[2] = cp[2];
		DWORD *vc = *se2->model->vcolor_indices;		
		for(;i-->0;vc+=2,p+=6)
		{
			float *v = vp+5*vc[0];
			p[0] = l[0];
			p[1] = l[1]+v[1];
			p[2] = l[2];
			switch(a) //might want to unroll this part 
			{
			case 0: p[0]+=v[0]; p[2]+=v[2]; break; //S
			case 1: p[0]-=v[2]; p[2]+=v[0]; break; //W 
			case 2: p[0]-=v[0]; p[2]-=v[2]; break; //N
			case 3: p[0]+=v[2]; p[2]-=v[0]; break; //E
			}
			(DWORD&)p[3] = vc[1];
			p[4] = v[3];
			p[5] = v[4];
			if(tuv) switch(tuv&16?a:0) //2020
			{				
			case 0: p[4]+=tu; p[5]+=tv; break; //KF2
			case 1: p[4]-=tu; p[5]+=tv; break; //UNTESTED
			case 2: p[4]-=tu; p[5]-=tv; break; //UNTESTED
			case 3: p[4]+=tu; p[5]-=tv; break; //UNTESTED
			}
		}

	}while(--se2>send2&&se2->depth_44D5A0>=depth);

	som_scene_413f10_maybe_flush(0); 

	depth = se2==send2?0:se2->depth_44D5A0;
}
static BYTE __cdecl som_scene_44d7d0(som_scenery *ts)
{
		som_scene_44d810_flush(); //HACK: 402400 switch from opaque?

	if(0) return ((BYTE(__cdecl*)(DWORD))0x44D7D0)((DWORD)ts);

	//som_scene_shadows::on() had done this, but transparent elements
	//cannot benefit from turning the depth texture off & on
	//Note: shadows and volume textures rely on this texture
	//190: D3DRS_COLORWRITEENABLE1 (switch MRT to texture 1)
	DDRAW::Direct3DDevice7->SetRenderState((DX::D3DRENDERSTATETYPE)190,0);

	sss::texture = -1; //som_scene_volume	

	//2018: does 44D7D0 set this up? took a long time to notice
	//this was missing... I guess depth sorting makes it rarely
	//an issue		
	if(sss::zwriteenable) //assert(!sss::zwriteenable)
	{
		DDRAW::Direct3DDevice7->SetRenderState
		(DX::D3DRENDERSTATE_ZWRITEENABLE,sss::zwriteenable=0);
	}
	else //assert(0); //2021???
	{
		//som_scene_44d810_flush changes to 0
		//there doesn't seem to be any code in 402400 that ensures
		//zwriteenable is 1... but it seems an unlikely coincidence
		//that this was never triggered before (som_scene_skyswap?)
	}

	som_scene_element2 *se2 = 0, *send2 = 0;

	EX::INI::Option op;

	float depth;
	if(!op->do_alphasort||!som_hacks_shader_model)
	{
		if(!som_scene_transparentelements2.empty())
		{
			se2 = &som_scene_transparentelements2.back();
			send2 = &som_scene_transparentelements2[0]-1;
		}
		depth = se2==send2?0:se2->depth_44D5A0;
	}
	else depth = 0; //2022

	BYTE v = 0;
	
	//NOTE: shadows have the 0th texture identifier (if kage.mdl fails gold.mdo is #0)
	//TODO: sort shadows/transparent elements according to light selectors
	//(REMINDER: se->worldxform[0][3] is used to group transparent elements for lighting)
	WORD shadow = DDRAW::doClearMRT&&DDRAW::vshaders9[7]?0:1024; //4096;	

	if(shadow)
	{				
		//2020: legacy modes (som_hacks_DrawIndexedPrimitiveVB) 
		//assume fog is disabled
		sss::fogenable = 0;
		DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_FOGENABLE,0);
		som_scene_alphaenable(1);
	}
	/*else //2023: building shadow shader matrix?
	{		
		Somvector::transpose<4,4>(SOM::analogcam,SOM::shadowcam);

		DDRAW::pset9(SOM::shadowcam[0],3,DDRAW::psTexMatrix[0]);
	}*/

	//TRYING SHADOWS ON TRANSPARENT
	//(since the PC shadow is also functional)
	//if shadows are first they won't appear on transparent
	//surfaces... however neither can they appear on more than
	//one surface per pixel, and they will appear fully opaque
	//
	//WARNING: some blend modes may not interact correctly with
	//the MRT sky/shadow depth buffer...
	//
	//BUMMER: shadows aren't appearing... I'm thinking that the
	//D3DPMISCCAPS_MRTPOSTPIXELSHADERBLENDING capability is not
	//reliable
	enum
	{
		shadow_pass = som_scene_skyshadow?1:1,
		transparency_pass = som_scene_skyshadow?2:1,

		//can't write to depth-texture when blending... not for
		//some chipsets anyway (maybe most)
		npc_mask = 0 
	};

	//npc_mask disables this... transparency_pass may do so too
	int npc = -1==som_hacks_skyconstants[2];

	//if(EX::debug) //testing (KF2 water/waterfall)
	{
		//this is 0.00025... the sky happens to use 0.000025
		//objects use 0.001 (1s?)		
		//NOTE: sky/objects should probably be reimplemented
		//
		// FIX ME: this should be based on 1s, not 1/4s :(
		// 
		// FIX ME: I'm thinking this is only implemented for
		// transparent geometry
		//
		som_scene_413f10_uv = SOM::motions.tick%4000/4000.0f;
	}

	int pass = shadow?2:1; pass2:

	if(som_scene_ambient2) som_scene_ambient2 = 1; //2023: seeing flickering

	DWORD i; //technically this subroutine returns a value
	for(i=0;i<ts->se_commit;i++)
	{
		som_scene_element *se = (*ts->ses)[i];
		 
		if(se->vs!=v) //reminder: elements are depth sorted
		{
			v = se->vs;
			som_scene_red(v?SOM::Versus[v-1].EDI:0);
		}

		if(se->kage||se->texture==shadow) //2023
		{	
			if(pass!=shadow_pass) continue;

			#ifdef _DEBUG
			//REMOVE ME
			//the color was passed to draw but 4435a0 throws
			//out color and MDO uses D3DVERTEX
			auto vdat = (DX::D3DLVERTEX*)se->vdata;	//0.984375f
			auto vdat2 = (DX::D3DVERTEX*)se->vdata;	//0.984375f
			//this is now 512/1024?
			//assert(vdat->x==-512);
			//assert((int)(vdat->x*1024-0.5f)==-512); //2021
			float *test = &vdat->tu; //MDL?
			if(se->fmode==3) test+=8; //MDO?
			assert(*test==EX_INI_SHADOWUVBIAS);
			#endif

			//2021: MDO? vdat2? material?
			//NOTE: color is really just for the fade out effect
			//(it might use color in the future?)
			DX::D3DCOLOR color; if(se->fmode!=4)
			{
				//neither the shader nor old path use materials
				//in this case... this could be a problem for SFX
				//too? what else uses mode 4??? must track it down!

				if(se->material!=0xffff)
				{
					auto *d = SOM::L.materials[se->material].f+1;

					color = int(0xff*d[3])<<24|0xffffff;
				}
				else{ assert(0); color = ~0; }
			}
			else
			{
				color = ((DX::D3DLVERTEX*)se->vdata)->color;

				assert((color&0xffffff)==0xffffff);
			}

			if(!som_scene_shadow)
			som_scene_shadow = som_scene_shadows::on();

			SomEx_npc = se->ai;
			if(SomEx_npc&0x400) //0x800 won't fit into se->ai
			SomEx_npc = 1<<12|~SomEx_npc;
			som_scene_shadows::draw(se->texture,se->worldxform,color);
			SomEx_npc = -1;
		}
		else //som_scene_44d810(se);
		{
		//	assert(se->mode!=4); //2021 //Moratheia?

			if(pass!=transparency_pass) continue;
						
			if(som_scene_shadow)
			som_scene_shadow = som_scene_shadows::off();
						
			if(npc!=se->npc&&2!=transparency_pass)
			{
				npc = se->npc; npc:
				if(npc_mask) som_hacks_skyconstants[2] = npc?-1:1; 
				if(npc_mask) DDRAW::pset9(som_hacks_skyconstants);
			}
			
			if(se->depth_44D5A0<depth)
			{
				if(npc&&2!=transparency_pass)
				{
					npc = false; goto npc; 
				}
				som_scene_413f10(depth=se->depth_44D5A0,se2,send2);
			}

			if(sss::texture!=se->texture)
			{
				som_scene_volume_select(se->texture);
			}

			//WTH???
			//shouldn't matter with batching logic in 44d810
			//except som_scene_413f10 has troubles 
			if(se->npc==3&&DDRAW::vshaders9[8])
			{
				som_scene_gamma_n = true; som_scene_44d810(se);
				som_scene_gamma_n = false;
			}
			else som_scene_44d810(se);
		}
	}
	if(v){ v = 0; som_scene_red(0); }
				
	if(pass==shadow_pass)
	{
		if(shadow==0) //todo? oldstyle PC shadow?
		{
			if(!som_scene_shadow)
			som_scene_shadow = som_scene_shadows::on();
			SomEx_npc = 12288;
			som_scene_shadows::draw(EX::INI::Player()->player_character_shadow);	
			SomEx_npc = -1;
		}
	}
	if(som_scene_shadow) 
	som_scene_shadow = som_scene_shadows::off();	
		
	if(pass==transparency_pass) 
	{
		if(op->do_alphasort&&som_hacks_shader_model) //TESTING
		{
			sss::worldtransformed = 0; 
			DDRAW::Direct3DDevice7->SetTransform(DX::D3DTRANSFORMSTATE_WORLD,&DDRAW::Identity);
			extern void som_bsp_sort_and_draw(bool push=false);
			som_bsp_sort_and_draw();
			sss::setss(1); //HACK
			som_scene_lighting(0);
		}
		else
		{
			if(se2>send2)
			{
				if(npc&&2!=transparency_pass)
				{
					if(npc_mask) som_hacks_skyconstants[2] = 1; 
					if(npc_mask) DDRAW::pset9(som_hacks_skyconstants);
				}
				som_scene_413f10(depth=0,se2,send2);  
			}
		}

		som_scene_44d810_flush(); //HACK: 402400 final batch?
	}

	if(pass==1)
	if(transparency_pass!=shadow_pass) 
	{	
		pass = 2; goto pass2;
	}		

	som_scene_volume = 0;
					
	//som_scene_shadows::off() had done this, but transparent elements
	//cannot benefit from turning the depth texture off & on
	//190: D3DRS_COLORWRITEENABLE1 (Reenable depth-texture write)
	DDRAW::Direct3DDevice7->SetRenderState((DX::D3DRENDERSTATETYPE)190,0xF);

	//REMOVE ME (TESTING)
	//this is normally done at the top of the frame, but it's done before
	//the swing model also?
	som_scene_transparentelements->clear();

	return i; //unused by caller
}

static int som_scene_4412E0_frame()
{
	//EXTENSION
	//skip ahead ~170ms to account for the weapon waiting
	//until the button is released
	//REMINDER: THIS CAN BE DYNAMIC ALTHOUGH I DON'T THINK
	//THAT'S FULLY IMPLEMENTED, BUT IT SHOULDN'T BE STORED
	//AS A CONSTANT
	extern float som_MDL_arm_fps; //0.06f
	return 1+(int)(som_MDL_arm_fps*(300-EX::INI::Player()->arm_ms_windup));
}
static BYTE __cdecl som_scene_4412E0_swing(SOM::MDL &mdl, DWORD id)
{
	extern void som_game_moveset(int,int);
	for(int j=4;j-->0;) som_game_moveset(0,j);

	//HACK: this is a SOM::PARARM::Item.arm record
	//NOTE: currently the glove overwrites weapons
	if(auto*mv=SOM::movesets[0][0]) 
	{
		id = mdl.animation(mv->us[0]);
	}
	if(!((BYTE(__cdecl*)(void*,DWORD))0x4412E0)(&mdl,id))
	return 0;
	int d = som_scene_4412E0_frame();
	mdl.rewind(); //mdl.f = -1;
	return ((BYTE(__cdecl*)(void*,DWORD))0x4414c0)(&mdl,d);
}
extern DWORD som_MDL_449d20_swing_mask(bool);
static bool som_scene_swing_mirror(bool mirror, som_scenery &tes)
{
	auto ses = *tes.ses;
	int n = tes.se_commit;
	for(int i=0,j=0;i<n;i=j)
	{
		auto mirror2 = ses[i]->npc;
		while(j<n&&mirror2==ses[j]->npc)
		{
			ses[j++]->npc = 0; //SEEMS UNNECESSARY
		}

		if(mirror!=(bool)mirror2)
		{
			mirror = mirror2;
			DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_CULLMODE,mirror?DX::D3DCULL_CW:DX::D3DCULL_CCW);
		}
		tes.se_commit = j-i;
		memmove(ses,ses+i,sizeof(void*)*(j-i));
		tes.draw();
	}
	tes.se_commit = n; //assert(tes->empty()) was hit???
	tes.clear(); 
	return mirror;
}
extern void som_scene_swing(bool clear, float alpha)
{  	
	auto test = SOM::frame;

	auto &mdl = *SOM::L.arm_MDL;

	auto tes = som_scene_transparentelements3;
	if(clear)
	{
		som_scene_zwritenable_text_clear(); //HACK

		if(!tes) //allocate enough transparent elements for arm?
		{
			//OPTIMIZING
			//this will just hold any transparent elements of the
			//non-afterimage models
			tes = som_scenery::alloc(32); //MIGHT NEED TO INCREASE?
			som_scene_transparentelements3 = tes;
		}
		//sometimes som_scene_transparentelements3 isn't empty 
		//after changing maps
	//	assert(tes->empty()); 
		
		tes->clear();
	}
	else if(alpha||tes&&!tes->empty()) 
	{
		tes = som_scene_transparentelements;
	}
	else //return; //OPTIMIZING?
	{
		//mdl.update_animation(); //sync f/d??

		return; //OPTIMIZING?
	}

	//maybe unnecessary
	//425d50 assumes the MDL isn't transparent so its clear
	//code is knocked out by som_scene_reprogram
	//tes->clear();
	assert(tes->empty());

	DWORD sm = som_MDL_449d20_swing_mask(0);

	int rt = mdl.running_time(mdl.c);

	//REMOVE ME?
	//
	// I'd like to remove this (especially if 0==legacy)
	// but it does look and feel much better in general
	// probably heavy attacks should do it... but how to
	// merge two waves? I think linear interpolation in
	// the MDL animations would be inadequate... but if
	// exports use nonlinear interpolation it'd be fine
	// although that would preclude further coordination
	//
	//HACK: fade is limited to regular attacks... the
	//new guard ability shouldn't be subject to it
	bool swing; //UNACCEPTABLE
	float fade; if(swing=mdl.d>1&&!SOM::motions.swing_move)
	{
		//this is a neglible difference however there's a minor
		//glitch if the shield sits at 0 and the attack starts
		//at arm_ms_windup as the shield must snap into place
		int f = som_scene_4412E0_frame();
		//float fade = M_PI*mdl.d/rt;
		fade = M_PI*max(0,mdl.d-f)/(rt-f);
		//EX::dbgmsg("fade: %f (%f)",fade,sinf(fade));
		fade = sqrtf(sinf(fade));
	}
	else fade = 0;

	//this lowers the weapon mid swing so it can't go above
	//the crosshair (center line) at 0
	float fade4 = max(0,pow(fade,4)-0.25f);
	float l_fade4 = 1-fade4;

	//Moratheia's model has 5
	//trying to do shoulder animation in the MDL file
	bool legacy = false; switch(mdl->skeleton_size)
	{
	case 5: if(mdl->file_head[0]&8) break; //Moratheia?
	case 4: legacy = true; break;
	}

	//REMOVE ME
	//adding 0.00097656f constants around som.MDL.cpp
	//moves MDL out of PlayStation 1/1024 units
	//float l024 = mdl.ext.mdo_elements?1:1024; //2021
	//enum{ l024=1 };

	//VR? 8 inches (approximately a real shoulder position)			
	//NOTE: the field of view change below doesn't apply to
	//VR
	float x = legacy?0.2f:0; //(it turns out 0.2 is KF2's shoulder span)
	//float lx = (legacy?x:0.2f)*(legacy?2048:1024); 
	float lx = 0.2f;//*l024;
	float y = -0.2f;
	float z = 0.03f;	
	//NOTE: it's good to use a fixed field of view
	//as a new way to avoid different FOV effects
	if(!DDRAW::inStereo)
	{
		if(legacy)
		x = 0.14f; 
		y = -0.17f;
		z = 0.3f; //needs shoulders	
	}

	#ifdef NDEBUG
//	#error fix me
	#endif
//	z+=1;

	//NOTE: same for VR?
	//y-=0.025f*l_fade4*(1-SOM::motions.shield*3.75f); 
	float yy = y-0.025f*(1-SOM::motions.shield*3.75f);
	y-=(y-yy)*l_fade4;
	//this tends to look nice to fake a topdown 
	//perspective (we don't look straight ahead
	//IRL)
	float u = M_PI/18*l_fade4; //10 degrees
	//this has a glitch where attacking while lowering shield
//	u*=powf(SOM::motions.swing+SOM::motions.shield*1.25f,0.5f);
	//2022: VR cuts the shield into your eye space
	//NOTE: SOM::rotate had a bug in this angle that I think
	//tended to exaggerate it
	//u*=powf(1+SOM::motions.shield,0.5f);
	u*=powf(0.5f+0.5f*SOM::motions.shield+1.5f*SOM::motions.shield3,0.5f);
	//EX::dbgmsg("u: %f (%f) %f",u,SOM::motions.shield3,SOM::motions.shield);
	//if(alpha) EX::dbgmsg("fade: %f (%f) %f",fade,fade4,l_fade4);

	//REMINDER: scaling x doesn't change anything
	//there's a conformal effect so that it's
	//equivalent to adjusting y and z. but x needs
	//to scale to center, although it doesn't help
	if(1&&!DDRAW::inStereo)
	{
		//EXTENSION?
		//NPCs feel too large compared to the arm
		//so I have scaled it up
		//KF2's two-arm weapons are closer/larger
		//than this
		float s = 1.25f;
		x*=s;
		for(int i=3;i-->0;)
		mdl.scale[i] = s;
	}
			 
	if(!DDRAW::inStereo)
	if(1&&SOM::zoom>=62) //KF2 test
	{
		z-=0.03f; //a little less shoulder?
			
		SOM::reset_projection(62,3);
	}
	else //NEW: fixed field-of-view for arm
	{
		z+=0.015f; //a little more shoulder?

		//REMINDER: SOM::Motions::place_camera INTERFERES WITH 
		//THIS (fixed field of view)
		SOM::reset_projection(55,3);
	}
	//VERY VERY SKETCHY (NEEDS THOUGHT)
	//the shield's scale depends on if it's pressed up against
	//something and it can't revert back when swinging so this
	//lunge effect has to be canceled out. however for weapons
	//canceling to the same doesn't look correct... meaning it
	float ac = SOM::motions.arm_clear;
	float anticling = 1-fade*(1-SOM::motions.clinging2*ac);
	float shield = powf(0.8f*(1+SOM::motions.shield*1.25f),2);
	//NOTE: in theory it would be good if new (non-legacy)
	//arms didn't have to counteract this movement but the
	//MDL format can't move this smoothly with just linear
	//interpolation and even so it would be hard to get it
	//right and this movement provides some confusion that
	//can help to mask animation issues
	float zz = z-0.1f*shield*anticling;
	if(2&SOM::motions.swing_move) z = zz;
	else z-=0.15f*anticling;
	float xx = x; if(1) //TESTING
	{
		//NOTE: for now it has to be extra dramatic to hide
		//the fact that the arm isn't bending :(
		float f = 0.66666f;
		float f2 = 0.3333f;
		float f3 = 0.75f*SOM::motions.clinging2*ac;
		f-=f3*f; f2-=f3*f2;
		zz+=f*SOM::motions.shield2[1];
		z+=f2*SOM::motions.shield2[1];
		xx+=f*SOM::motions.shield2[0];
		x+=f2*SOM::motions.shield2[0];
	}

		
		som_scene_batchelements_off _be; //RAII
		
	sss::push();

	som_scene_gamma_n = true; //KF2?
	
	//select lamps?	//2024 //EXTENSION?
	((BYTE(__cdecl*)(float*,DWORD))0x411210)(SOM::L.pcstate,3);

	som_scene_lighting(1); //2024
	som_scene_lit = som_scene_lit5;
	memcpy(som_scene_lit,SOM::xyz,sizeof(FLOAT)*3);
	som_scene_lit[3] = som_scene_lit[4] = 1;
		
	if(som_scene_ambient2)
	{
		//2022: I'm not sure why forcing this to refresh (-1)
		//(every ghost image) is needed (it should just work)
		//but it used to be done on 'clear' (at the beginning
		//of the frame) but the transparency showed the wrong
		//color (I assume the shader registers are corrupted)

		som_scene_ambient2 = 1; //HACK

		float a2[3+2] = {SOM::xyz[0],SOM::xyz[1],SOM::xyz[2],0.25f,SOM::L.duck};
		som_scene_ambient2_vset9(a2);
	}

	//2024: create afterimage when compositing?
	extern void dx_d3d9X_SEPARATEALPHABLENDENABLE_knockout(bool);
	if(!DDRAW::isPaused)
	{
		//this sets alpha pixels to black and som.shader.cpp
		//has this code for testing it:
		//cmp1*=sum1.a; cmp0*=sum0.a;
		//sum1*=sum1.a; sum0*=sum0.a;
		dx_d3d9X_SEPARATEALPHABLENDENABLE_knockout(true);		
		//have to enable blending for SEPARATEALPHABLENDENABLE
		//to work
		som_scene_alphaenable(1);sss::alphaenable=0; //???
	}
	
	if(!clear&&som_scene_transparentelements3)	
	if(!som_scene_transparentelements3->empty())
	{
		//som_scene_transparentelements3->draw().clear(); //flush()
		if(som_scene_swing_mirror(false,*som_scene_transparentelements3))
		DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_CULLMODE,DX::D3DCULL_CCW);
	}

	if(alpha) //not just clearing tes?
	{
		mdl.fade = alpha;

		//NOTE: classic SOM values are 0.25,-0.2,0.3 (425d50)
		mdl.xyzuvw[0] = x; xx-=x;
		mdl.xyzuvw[1] = y; yy-=y;
		mdl.xyzuvw[2] = z; zz-=z;
		SOM::rotate(mdl.xyzuvw,u+SOM::L.pcstate[3],SOM::L.pcstate[4]);		
		//2024: pcstate holds swing adjustments
		mdl.xyzuvw[0]+=SOM::L.pcstate[0]; //???
	//	mdl.xyzuvw[0]+=SOM::xyz[0];
		mdl.xyzuvw[1]+=SOM::L.pcstate[1]+1.5f+SOM::L.bobbing;
		mdl.xyzuvw[2]+=SOM::L.pcstate[2]; //???
	//	mdl.xyzuvw[2]+=SOM::xyz[2];
		mdl.xyzuvw[3] =-SOM::L.pcstate[3]-u;
		mdl.xyzuvw[4] = SOM::L.pcstate[4]+3.141593f;
		mdl.xyzuvw[5] = SOM::L.pcstate[5];

		//REMINDER: must do this before cp manipulations
		mdl.update_animation();
		mdl.update_transform();
		enum{ cp_skeleton=1 }; //YUCK
		extern BYTE __cdecl som_MDL_transform_hard(SOM::MDL&);
		som_MDL_transform_hard(mdl);

		float cp[2][3] = {}; if(!legacy) //Moratheia?
		{
			if(mdl.d>1)
			mdl.control_point(cp[0],mdl.c,mdl.d);
			if(mdl.ext.d2)
			mdl.control_point(cp[1],mdl.ext.c2,mdl.ext.d2);
			//2022: I think I inverted the Y axis in cpgen
			//at some point (rightly so--I think) but with
			//the old CP file I had this was right side up
			cp[0][1] = -cp[0][1];
			cp[1][1] = -cp[1][1];
			//for(int i=3*2;i-->0;)
			//cp[0][i]*=l024;
			if(~sm&1<<30)
			{
				//2024: changing to half
				cp[0][0]+=cp[1][0]*0.5f; //recenter weapon?
				cp[1][0]-=xx;//*l024; //HACK				
				cp[1][1]-=yy;//*l024; //HACK
				cp[1][2]-=zz;//*l024; //HACK
			}
			else memcpy(cp[1],cp[0],12);
			
		//EX::dbgmsg("z %f %f",cp[1][2],mdl.skeleton[7].xform[3][2]);
		//cp[1][2] = 0;

			if(cp_skeleton)
			for(int i=mdl->skeleton_size;i-->0;)
			for(int j=3;j-->0;)
			mdl.skeleton[i].xform[3][j]+=cp[i>=4][j];
		}

		//2022: MDL+MDO makes this more complicated
		int sm2 = 0; 
		for(int i=0;i<8;i++)
		{
			//hide/show the right or left arm if not animated
			if(sm&(i<4?0xf:0xf0))
			sm2|=1<<i;
		
			//EXTENSION
			//HACK: hide ARM.MDL pieces?
			//NOTE: this approach is more flexible than the original
			//since that had hidden the entire model if the _0 model
			//is present
			if(i<4?SOM::L.arm_MDO[i]:SOM::L.arm2_MDO[i-4])
			sm2&=~(1<<i);
		}
		if(auto*mdo=mdl->ext.mdo) //MDL+MDO?
		{
			auto *els = mdo->chunks_ptr;
			if(els->extradata)
			for(int j=mdo->chunk_count;j-->0;)
			{			
				auto pt = els[j].extra().part;
				mdl.ext.mdo_elements[j].fmode = sm2&(1<<pt)?3:15;					
			}
			else assert(0);
		}
		else //MDL?
		{
			DWORD *pcs = &mdl->element_count;
			DWORD *p = pcs+1;
			for(DWORD j=0;j<*pcs;j++,p+=9)
			mdl.elements[j].se->fmode = sm2&(1<<*p)?3:15;
		}

		//drawing after so it will cover the weapon
		//and disabling sorting until a BSP algorithm
		//is implemented
		bool f2b = alpha==1; 				 
		if(!f2b) //back-to-front?
		{
			//mdl.draw does this
	//		mdl.update_transform(); //mdl.xform //cp_skeleton
		}
		else mdl.draw(tes); //front-to-back?

		bool mirror = false;

		//front-to-back is better for opaque depth-culling
		for(int ii=0,i;i=f2b?7-ii:ii,ii<8;ii++) 
		if(sm&(i>=4?0xf0:0xf))
		{
			auto *mdo = i<4?SOM::L.arm_MDO[i]:SOM::L.arm2_MDO[i-4];
			if(!mdo) continue;

			//auto *f = SOM::L.arm_MDO_files[i];
			auto *f = (SOM::Struct<>*)mdo->i[0];

			int pcs = f->i[6]; if(!pcs) continue;

			if(i==0||i==4) //weapon?
			{
				//need to save fade in front-to-back order
				float fade2 = fade; if(i==4) 
				{
					if(!mdl.ext.d2) continue;

					//handle wraparound behavior?
					if(auto*mv=SOM::shield_or_glove(5))
					{
						int lo = mv->uc[86]-1, hi = mv->uc[87];
						int rt2 = mdl.running_time(mdl.ext.c2);
						int dd = mdl.ext.d2;
						if(dd>hi) dd = hi-(dd-hi);
						//fade2 = sinf(M_PI*mdl.ext.d2/rt2);
						fade2 = sinf(M_PI*dd/hi);
					}
					else assert(0);
				}
				else 
				{
					if(mdl.d<=1) continue;

					if(!swing) fade2 = 1; //UNACCEPTABLE
				}

				//fade out long swords (Moratheia) so they
				//don't hang out awkwardly after animation
				mdo->f[0x1f] = alpha*pow(min(1,fade2*1.5f),4);
			}
			else mdo->f[0x1f] = alpha;

			bool mirror2 = mirror;
			if(i>=5&&SOM::L.arm_MDO[i-4])
			{
				auto *g = (SOM::Struct<>*)SOM::L.arm_MDO[i-4]->i[0];
				mirror = f==g;
			}
			else mirror = false;

			float *m = (float*)(mdo->i[0x2e]+16); 
			auto &mm = mdl.xform;
			memcpy(m,mdl.skeleton[i].xform,4*4*4);			
			if(!legacy)
			{
				for(int j=3;j-->0;) 
				m[12+j]+=m[j]*(i>=4?lx:-lx);
			}
			if(mirror) for(int j=3;j-->0;)
			m[j] = -m[j];			
			//TODO: arm_bicep?
			/*if(!mdl.ext.mdo_elements) //2021
			for(int j=3;j-->0;) //upscale mdl matrix
			for(int k=3;k-->0;) m[j*4+k]*=l024;*/
			Somvector::map<4,4>(m).premultiply<4,4>(mm);			
			while(--pcs) memcpy(m+pcs*0x1a,m,4*4*4);	
			
			if(mirror2!=mirror)
			DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_CULLMODE,mirror?DX::D3DCULL_CW:DX::D3DCULL_CCW);
			DWORD j = tes->se_commit;
			((BYTE(__cdecl*)(void*,void*))0x446010)(mdo,tes);
			while(j<tes->se_commit)			
			(*tes->ses)[j++]->npc = mirror; //REPURPOSING
		}
		else
		{
			//might like to render the MDL part here independent
			//of the rest of the model
		}

		if(f2b) //front-to-back?
		{
			std::reverse(*tes->ses,*tes->ses+tes->se_commit);
		}
		else //back-to-front?
		{
			int prev = tes->se_commit;

			if(mirror)
			DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_CULLMODE,DX::D3DCULL_CCW);
			mirror = false;

			mdl.draw(tes); 
		
			//som_MDL_440ab0_unaminated reverses the draw order
			std::reverse(*tes->ses+prev,*tes->ses+tes->se_commit);
		}
		
		//NOTE: this has to be done every time because the models are the
		//same instances
		if(!clear) 
		{
	//		REMINDER: disabling depth-testing is more pixels to draw
	//		when supersampling and it can reveal backfacing polygons
	//		that might mostly be subsumed into the main opaque image

			//NEW: I think afterimages looks better if they don't clip
			//(just as the do_lap extension's double exposure doesn't)
	//		if(!f2b) DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_ZENABLE,0);

			//not sorting because I want the hand/armor to definitely
			//appear in front of the weapon until some BSP system can
			//be implemented
			//tes->draw().clear(); //flush()
			mirror = som_scene_swing_mirror(mirror,*tes);

	//		if(!f2b) DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_ZENABLE,1);
		}

		if(mirror) DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_CULLMODE,DX::D3DCULL_CCW);
		
		if(cp_skeleton) //HACK: can't accumulate
		{
			for(int i=mdl->skeleton_size;i-->0;)
			for(int j=3;j-->0;)
			mdl.skeleton[i].xform[3][j]-=cp[i>=4][j];

			//EX::dbgmsg("z %f %f",cp[1][2],mdl.skeleton[7].xform[3][2]);
		}
	}
	//else mdl.update_animation(); //sync f/d??

	som_scene_lit = 0;

	som_scene_gamma_n = false; //KF2?
	
	som_scene_44d810_flush(); //Clear? //SEPARATEALPHABLENDENABLE

	dx_d3d9X_SEPARATEALPHABLENDENABLE_knockout(false); //2024

	sss::pop(); 

	SOM::reset_projection();
}
extern bool som_scene_hud = false;
static void __cdecl som_scene_425d50()
{
	auto &mdl = *SOM::L.arm_MDL;	

	BYTE &swing = SOM::L.swinging,swap;
	if((swap=swing)||mdl.d>1||mdl.ext.d2!=0) //do_arm?
	{
		bool weapon = (swap&1)!=0||mdl.d>1;

		swing = 0;
		
		//call som_hacks_Clear (emulating 425d50)
		DDRAW::Direct3DDevice7->Clear(0,0,D3DCLEAR_ZBUFFER,0,1,0);
		
		//som_hacks_Clear will draw this
		//(there used to be after-images drawn here)
		if(!EX::INI::Bugfix()->do_fix_zbuffer_abuse)
		som_scene_swing(false,1.0f); 
		som_scene_swing(false,0); //transparency?						
		
		int rt = mdl.running_time(mdl.c)-1;
		int rt2 = mdl.ext.d2?mdl.running_time(mdl.ext.c2)-1:0;

		mdl.ext.speed = mdl.ext.speed2 = 1.25f;
		{
			auto *prm = &SOM::L.item_prm_file;

			int r = SOM::L.pcequip[0];
			int h = SOM::L.pcequip[3];
			int l = SOM::L.pcequip[5];
			float wt = h!=0xff?prm[h].f[74]/2:0;
			float wt2 = wt;
			if(r!=h&&r!=0xff) wt+=prm[r].f[74];
			if(l!=h&&l!=0xff) wt2+=prm[l].f[74];

			WORD *st = SOM::L.pcstatus;
			int str = st[SOM::PC::str];

			EX::INI::Player pc;
			float sw = str/2/(wt*5+pc->player_character_weight/2);
			float sw2 = str/2/(wt2*5+pc->player_character_weight2/2);
			if(sw>1) mdl.ext.speed+=sw-1;
			if(sw<1) mdl.ext.speed-=1-sw;
			if(sw2>1) mdl.ext.speed2+=sw2-1;			
			if(sw2<1) mdl.ext.speed2-=1-sw2;

			float &sp = mdl.d>1?mdl.ext.speed:mdl.ext.speed2;
			sp+=min(5000,st[SOM::PC::attack_power])/5000.0f-0.5;

			if(sp>0) sp = sqrtf(sp); //som_MPX_reset_model_speeds does this

			sp = max(0.75f,min(sp,1.75f));

		//	EX::dbgmsg("sp: %f %f",sp);
		}
		float x = 30.0f*som_MDL::fps/DDRAW::refreshrate;

		x*=2; //arm animations are twice as long

		if(16&*mdl->file_head) x*=2; //60fps?

		mdl.ext.speed*=x; mdl.ext.speed2*=x; 
		
		if(!SOM::motions.swing_move)
		{									
			int pce = SOM::L.pcequip[0]; if(pce!=255)
			{
				auto &prm = SOM::L.item_prm_file[pce];
				auto &pr2 = SOM::L.item_pr2_file[prm.s[0]];

				if(SOM::frame-SOM::swing>15) //2023
				{
					int snd = pr2.uc[73];			
					if(mdl.d>=snd-1&&mdl.d<=snd+4)
					{
						SOM::swing = SOM::frame;

						//I think 0 is silence or ignored 
						//but this is for som_game_nothing
						SomEx_npc = 12288;
						if(snd=pr2.us[37]) SOM::se(snd,pr2.c[85]);
						SomEx_npc = -1;
					}
				}
			}
		}

		if(weapon&&!SOM::motions.swing_move)
		mdl.ext.dir = 1;
		//int dir = mdl.d-mdl.f;		
		int dir  = mdl.ext.dir; //holding up?
		//int dir2 = mdl.ext.d2-mdl.ext.f2;		
		int dir2 = mdl.ext.dir2; //holding up?
		if(!weapon) dir = 0;

		//425940 is no longer incrementing high speed
		//if(2==inc&&!DDRAW::isPaused)
		if(!DDRAW::isPaused&&0==EX::context())
		{
			//F1 supersampling mode means do_lap can't
			//be a constant setting
			//if(mdl.d>1) mdl.d+=dir*inc;
			//if(mdl.ext.d2>1) mdl.ext.d2+=dir2*inc;
			if(mdl.d>1)
			{
				mdl.advance(dir);
			}
			if(dir2<0&&mdl.d>1)
			{
				//keep shield raise so animation stays in
				//combinated mode for its entire lifetime
				if(mdl.ext.d2-mdl.ext.s2-3>mdl.ext.speed2) 
				{
					mdl.advance2(dir2);
				}
			}
			else if(mdl.ext.d2>1) mdl.advance2(dir2);

			EX::dbgmsg("d2: %d (%f)",mdl.ext.d2,mdl.ext.s2);
		}
		
		if(weapon&&mdl.d)
		{
			int dd = mdl.d;

			if(dd>=rt)
			{
				auto *mv = SOM::shield_or_glove(0);
				if(!mv||mv->s[0]!=mdl.animation_id())
				{
					if(mdl.ext.d2) //shield?
					{
						//NOTE: I think som_MDL_449d20_swing_mask
						//will hide the arm
						dd = 1; swap = 0; assert(mdl.f>dd); 
					}
				}
				else //handle wraparound behavior?
				{
					//assert(0); //REMOVE ME
					int lo = mv->uc[86]-1, hi = mv->uc[87];
					dd = lo;
					mdl.rewind(); //mdl.f = -1
				}
			}
			mdl.d = min(dd,rt);
		}
		if(mdl.ext.d2)
		{
			int dd = mdl.ext.d2;
				
			assert(dd<=rt2);

			if(dd>=rt2)
			{
				auto *mv = SOM::shield_or_glove(5);
				if(!mv||mv->s[0]!=mdl.animation_id(mdl.ext.c2))
				{
					assert(0);

					//duplicating above logic... does
					//this even make sense for shield?

					if(mdl.d>1) //weapon?
					{
						dd = 0;
					}
				}
				else //handle wraparound behavior?
				{
					int lo = mv->uc[86]-1, hi = mv->uc[87];
					dd = lo;
					mdl.rewind2(); //mdl.ext.f2 = -1
					
				}
			}
			mdl.ext.d2 = min(dd,rt2);
		}

		//2020: som_logic_reprogram enables this by
		//knocking out 4250F6		
		//2021: I'm moving this from som.mocap.cpp and
		//trying to let the animation finish playing
		//REMINDER: this is designed not frustrate the
		//player when the animation appears to be done
		//but pressing the attack button still doesn't
		//do anything... after this point everything
		//in the PRF is ignored but normally nothing
		//should appear so late
		if(weapon&&!SOM::motions.swing_move)
		{
			//300ms or maybe 150ms since arm models are sped
			//up?
			if(rt-mdl.d<18) //300ms?
			{
				//if(EX::INI::Option()->do_arm) //EXTENSION?
				{
					if(!SOM::L.pcequip2) //equipping?
					{
						swap = 0;

						//mdl.d+=inc; //425c8a
						mdl.advance();
					}
				}
			}
			if(mdl.d>=rt) //425cfc
			{
				mdl.d = 1; assert(mdl.f>mdl.d); 

				mdl.rewind(); //mdl.f = -1;
				
				//SOM::L.pcequip2 is beginning a new swing
				//after equipping
				swap = 0; 
			}
		}
	}

	DX::D3DVIEWPORT7 vp;
	DDRAW::Direct3DDevice7->GetViewport(&vp); 	
	float z[2] = {vp.dvMinZ,vp.dvMaxZ};
	vp.dvMinZ = 0;
	vp.dvMaxZ = EX_INI_Z_SLICE_1;
	DDRAW::Direct3DDevice7->SetViewport(&vp);
	vp.dvMinZ = z[0];
	vp.dvMaxZ = z[1];

	som_scene_hud = true;
	((void(__cdecl*)())0x425d50)();
	som_scene_hud = false;

	DDRAW::Direct3DDevice7->SetViewport(&vp);

	if(swap&1) swing = 1;
}
static void __cdecl som_scene_43F540_swing(DWORD snd)
{
	auto &mdl = *SOM::L.arm_MDL; //debugging

	INT32 pitch = 0;
	int w = SOM::L.pcequip[0]; if(w<250)
	{
		WORD prm = SOM::L.item_prm_file[w].s[0];
		char *prf = SOM::L.item_pr2_file[prm].c;
		char *p = prf+84;

		//NOTE: this is an extension of the PRF format
		//if p[0] is nonzero this is a new kind of PRF
		//with 4 attacks
		/*2021: som_game_moveset manages weapons 
		if(!p[0]) pitch = p[1]; else assert(0);*/
		pitch = p[1];
	}
	if(0) //3D?
	{
		//what if the sound tracks the hand? but this is
		//called before a matrix is available and there's
		//no precedent for tracking a sound source (though
		//that would be a nice upgrade)
		//REMINDER: default DirectSound min-distance is 1m
		//(som_db actually sets it to 2.5m)
		//auto &mdl = *SOM::L.arm_MDL;
	}
	((void(__cdecl*)(DWORD,INT32))0x43f540)(snd,pitch);
}
static int __cdecl som_scene_428220_magic_shield(INT32 esi, DWORD timer)
{
	auto &mpf = SOM::L.magic_prm_file; //HACK: save file is pre-adjusted
	if(esi>=(INT32)&mpf) esi = (esi-(INT32)&mpf)/320;

	int prf = mpf[esi].us[0];
	int index = SOM::L.magic_pr2_file[prf].us[16]; //recovering first argument

	if(index<0xf)
	{
		if(!((int(__cdecl*)(INT32,DWORD))0x428220)(index,timer))
		{
	err:	assert(0); return 0; 
		}
	}
	else if(timer) //EXTENSION (green Missile Shield magic?)
	{
		int ti = 207+index-11;
		if(ti<211||ti>219) goto err; //these are reserved texture numbers

		void* &t = SOM::L.fullscreen_SFX_images_ext[ti-211]; 
		if(t==(void*)~0) goto err;

		if(!t) //YUCK: this locks the MPX thread and so may pause the game
		{
			char a[MAX_PATH];
			sprintf(a,(char*)0x45aadc,(void*)0x4c0dcc,ti); //%s%4.4d.txr
			extern DWORD __cdecl som_TXR_4485e0(char*,int,int,int);
			t = (void*)som_TXR_4485e0(a,0,0,0);
			if(t==(void*)~0) goto err;
		}

		int iVar3 = (-1&0xfffffffe)+2; //428220
		auto puVar2 = &SOM::L.fullscreen_quads[iVar3]; //42d8eb
		if(puVar2->uc[0]=((BYTE(__cdecl*)(void*,void*,DWORD))0x42e380)(puVar2,t,1000))
		{
			puVar2->uc[12] = timer; puVar2->uc[1] = 10; //index-2

			//891 is magic shield sound
			WORD snd = SOM::SND(0x37b); //2024: build sound table?

			SOM::se(snd,0); //0x37b
		}
		else goto err;
	}
	SOM::L.pcmagic_support_index = esi; //note: save files will read this in
	SOM::L.pcmagic_support_timer = timer;

	//TODO: accumulate timers and damage ratings unless the same
	//sfx texture is used, in which case subtract the old values
	//and add the new values/time
	for(int i=8;i-->0;) SOM::L.pcmagic_shield_timers[i] = timer; //BYTE->DWORD

	return 1;
}

extern void som_scene_reprogram()
{
	//2022: must initialize SOM::cam before world step
	{
		//2020: flame (billboard) tracking
		//NEEDS MORE WORK TO FOLLOW LOOKING DIRECTION (IS EYE LEVEL ADDED?)
		//(note, KFII probably just blits sprites directly onto the screen)
		//0043A3B7 B9 58 23 4C 00       mov         ecx,4C2358h
		*(FLOAT**)0x43A3B8 = SOM::cam;
		//00425004 D8 05 30 85 45 00    fadd        dword ptr ds:[458530h]
		*(FLOAT**)0x425006 = SOM::cam+3;
		//
		//magic particle emitter
		//00427FF2 A1 A4 1D 9C 01       mov         eax,dword ptr ds:[019C1DA4h]
		*(FLOAT**)0x427FF3 = SOM::uvw+0;
		//might as well thse too for the wall-grab feature
		//00424FED D8 05 98 1D 9C 01    fadd        dword ptr ds:[19C1D98h]
		*(FLOAT**)0x424FEf = SOM::cam;
		//00425015 D8 05 A0 1D 9C 01    fadd        dword ptr ds:[19C1DA0h]
		*(FLOAT**)0x425017 = SOM::cam+2;
	}
	
	//SWING/MAGIC BUSINESS
	{
		//change start frame?
		//00424CC5 E8 16 C6 01 00       call        004412E0
		*(DWORD*)0x424CC6 = (DWORD)som_scene_4412E0_swing-0x424CCa;

		//plays sound without pitch? (pitch is in the PRF file...
		//actually it's an extension)
		//00425CE7 E8 54 98 01 00       call        0043F540
		*(DWORD*)0x425CE8 = (DWORD)som_scene_43F540_swing-0x425CEc;

		if(1) //make magic forgiving to encourage cover based play?
		{
			//there is a delay between pressing/releasing the button
			//that should be about equal to the time it takes the 
			//gauge to drain from full to this relaxed limit, so it
			//seems to be more fair in a hand/eye coordination sense

			//note, it seems like the guage doesn't beging to drain
			//at 4500
			
			//00424F5D BD 88 13 00 00       mov         ebp,1388h  
			//00424F62 66 3B F5             cmp         si,bp  
			//00424F65 0F 85 24 01 00 00    jne         0042508F
			*(DWORD*)0x424F5e = 3800; //5000
			*(WORD*)0x424F65 = 0x820f; //jb
		}

		//2021: som_scene_swing support 
		//swapping Clear and BeginScene calls so som_hacks_Clear can
		//draw the arm model on the back of the Clear call...I think
		//Clear should be called after anyway according to MSDN docs
		//004024E8 8B 16                mov         edx,dword ptr [esi]
		//004024E6 56                   push        esi
		//004024E7 FF 52 14             call        dword ptr [edx+14h]		
		/*it's cleaner to swap, not involving som.hacks.cpp
		memset((void*)0x4024E6,0x90,4);*/
		memmove((void*)0x4024d2,(void*)0x4024cc,24);
		*(WORD*)0x4024cc = 0x168B;
		*(DWORD*)0x4024ce = 0x1452FF56;

		if(1) //2024: fix missing defense ratings to support magic
		{
			//00425B9B 75 02                jne         00425B9F
			//*(BYTE*)0x425B9B = 0x74; //jne->je (bug?)
			//EXTENSION 
			//0042566a 74 06           JZ         LAB_00425672
			*(WORD*)0x42566a = 0x9090; //nop will always set the ratings
			//004256bb 50              PUSH       EAX
			*(BYTE*)0x4256bb = 0x56; //eax->esi
			//004256BC E8 5F 2B 00 00       call        00428220
			//0042cd07 e8 14 b5 ff ff       call        00428220
			*(DWORD*)0x4256Bd = (DWORD)som_scene_428220_magic_shield-0x4256c1;
			*(DWORD*)0x42cd08 = (DWORD)som_scene_428220_magic_shield-0x42cd0c;
			//00427a11 8a 86 3c 1d 9c 01    mov        al,byte ptr[ESI+DAT_019c1d3c]
			*(WORD*)0x427a11 = 0x868b;
			*(DWORD*)0x427a13 = (DWORD)SOM::L.pcmagic_shield_timers; //not 019c1d3c
		}
	}	

	//sky flag?
	//004024F4 38 1D 50 23 4C 00    cmp         byte ptr ds:[4C2350h],bl  
	//004024FA 75 07                jne         00402503  
	//MSM/sky renderer subroutine
	//004024FC E8 EF 12 01 00       call        004137F0  		
	*(DWORD*)0x4024FD = (DWORD)som_scene_4137F0-0x402501;
	//00402501 EB 05                jmp         00402508  				
	//memset((BYTE*)0x4024FC,nop,5); //4137F0
	{
		//004137F0 A1 3C 89 59 00       mov         eax,dword ptr ds:[0059893Ch]  
		//004137F5 83 EC 10             sub         esp,10h  
		//004137F8 85 C0                test        eax,eax  
		//004137FA 75 06                jne         00413802  
		//004137FC 32 C0                xor         al,al  
		//004137FE 83 C4 10             add         esp,10h  
		//00413801 C3                   ret  
		//BLOT OUT THE SKY (mov ecx,0   nop)
		//00413802 8B 88 0C 02 00 00    mov         ecx,dword ptr [eax+20Ch] 
		memcpy((void*)0x413802,"\xB9\x00\x00\x00\x00\x90",6); 
		//00413808 85 C9                test        ecx,ecx  
		//0041380A 0F 84 0E 01 00 00    je          0041391E 
		{
			//DRAWING THE SKY
		}
		//DRAWING THE TILES	(59892Ch is vertex buffer)
		//0041391E A1 2C 89 59 00       mov         eax,dword ptr ds:[0059892Ch]  
		//trying to call 413940 below
		//memcpy((void*)0x41391E,"\xB8\x00\x00\x00\x00",5); 
		//00413923 85 C0                test        eax,eax  
		//00413925 74 09                je          00413930  
		//00413927 E8 E4 05 00 00       call        00413f10  
		//0041392C 83 C4 10             add         esp,10h  
		//0041392F C3                   ret  
		//NO VERTEX BUFFER path
		//calls IDirect3DDevice7::DrawIndexedPrimitive which 
		//triggers an assert saying software mode expected???
		//00413930 E8 0B 00 00 00       call        00413940  
		//00413935 83 C4 10             add         esp,10h  
		//00413938 C3                   ret  
	}		
	//00402503 E8 F8 28 01 00       call        00414E00  
	//pop render-state
	//00402508 E8 F3 AC 04 00       call        0044D200   
	//memset((BYTE*)0x402508,nop,5); //NO EFFECT???
	//EMPTY TRANSPARENCY BUFFER
	//0040250D A1 3C 22 4C 00       mov         eax,dword ptr ds:[004C223Ch]  
	//00402512 50                   push        eax  
	//00402513 E8 28 B0 04 00       call        0044D540  
	//memset((BYTE*)0x402513,nop,5); 
	
	//2021: draw gold pieces
	//00402518 E8 53 1B 00 00       call        00404070  
	*(DWORD*)0x402519 = (DWORD)som_scene_404070-0x40251d;

	//memset((BYTE*)0x402518,nop,5); //NO EFFECT???
	//OBJECTS
	//0040251D E8 FE 8C 02 00       call        0042B220  
	*(DWORD*)0x40251E = (DWORD)som_scene_42B220-0x402522;
	//ENEMIES
	//00402522 E8 69 5B 00 00       call        00408090  
	*(DWORD*)0x402523 = (DWORD)som_scene_408090-0x402527;
	//NPCS
	//00402527 E8 E4 75 02 00       call        00429B10  
	*(DWORD*)0x402528 = (DWORD)som_scene_429B10-0x40252C;
	//ITEMS
	//0040252C E8 7F DE 00 00       call        004103B0
	*(DWORD*)0x40252D = (DWORD)som_scene_4103B0-0x402531;
	//MAGIC (ALL SFX)
	//00402531 E8 0A C7 02 00       call        0042EC40  
	//memset((BYTE*)0x402531,nop,5); 
	//piggybacking z-tested sky (after magic)
	*(DWORD*)0x402532 = (DWORD)som_scene_42EC40-0x402536;
	//00402536 8B 0D 3C 22 4C 00    mov         ecx,dword ptr ds:[4C223Ch]  
	//0040253C 51                   push        ecx  
	//0040253D E8 5E B0 04 00       call        0044D5A0 (DEPTH-SORTING)  
	//if(0&&EX::debug) memset((BYTE*)0x40253D,0x90,5);
	//00402542 8B 15 3C 22 4C 00    mov         edx,dword ptr ds:[4C223Ch]  
	//00402548 52                   push        edx  
	//SHADOWS/TRANSPARENCY 
	//00402549 E8 82 B2 04 00       call        0044D7D0  
	*(DWORD*)0x40254A = (DWORD)som_scene_44d7d0-0x40254E;
	//
	// 	   2021: flush batch drawing on transparency?
	// 
	// 	//main menu
	//	00418598 E8 33 52 03 00       call        0044D7D0
	// 	//item pick up
	// 	00410D57 E8 74 CA 03 00       call        0044D7D0
	// 	//sky model?
	// 	004138BD E8 0E 9F 03 00       call        0044D7D0
	// 	//arm model?
	// 	00425FE6 E8 E5 77 02 00       call        0044D7D0
	// 	//shop menu
	// 	0043D003 E8 C8 07 01 00       call        0044D7D0
	*(DWORD*)0x418599 = (DWORD)som_scenery::draw_44d7d0-0x41859d;
	*(DWORD*)0x410D58 = (DWORD)som_scenery::draw_44d7d0-0x410D5c;
	*(DWORD*)0x4138Be = (DWORD)som_scenery::draw_44d7d0-0x4138c2;
	*(DWORD*)0x425FE7 = (DWORD)som_scenery::draw_44d7d0-0x425FEb;
	*(DWORD*)0x43D004 = (DWORD)som_scenery::draw_44d7d0-0x43D008;
	//0040254E 83 C4 0C             add         esp,0Ch
	//SWING MODEL, FULLSCREEN SFX?
	//00402551 E8 FA 37 02 00       call        00425D50 
	*(DWORD*)0x402552 = (DWORD)som_scene_425d50-0x402556;
	//SCREEN EFFECTS MAGIC, ETC. (OR FLASH EFFECTS MAYBE)
	//NOTE: comes after transparent elements
	//00402556 E8 C5 B6 02 00       call        0042DC20  
	*(DWORD*)0x402557 = (DWORD)som_scene_42DC20-0x40255B;

	/*

	//??????
	0040255B 38 1D 51 23 4C 00    cmp         byte ptr ds:[4C2351h],bl  
	00402561 74 05                je          00402568  
	00402563 E8 78 1F 01 00       call        004144E0

	//pop render-state
	00402568 E8 93 AC 04 00       call        0044D200  
	0040256D 8B 06                mov         eax,dword ptr [esi]  
	0040256F 56                   push        esi  
	00402570 FF 50 18             call        dword ptr [eax+18h]  
	00402573 E8 78 18 02 00       call        00423DF0  
	*/

	//DECIPHERING 44D810 (draw calls)
	//00440EC5 E8 46 C9 00 00       call        0044D810
	*(DWORD*)0x440EC6 = (DWORD)som_scene_44d810-0x440ECA;
	//shops draw item names via this call; Models with 4230C0
	//004461A3 E8 68 76 00 00       call        0044D810 
	*(DWORD*)0x4461A4 = (DWORD)som_scene_44d810-0x4461A8;
	//2017: This one draws the auto-map;
	//red&blue gauges; Menu frames,backgrounds,highlights
	//it's entered every frame even if it's not required
	//00448CEC E8 1F 4B 00 00       call        0044D810 
//thinking maybe this should be on, just to be safe
//	*(DWORD*)0x448CED = (DWORD)som_scene_44d810_2017-0x448CF1;
	*(DWORD*)0x448CED = (DWORD)som_scene_44d810-0x448CF1;
	//full screen tints? discovered with 0xa9000000 dark bug
	//healng magic, etc.
	//0044A25C E8 AF 35 00 00       call        0044D810
	/*som_MPX_42dd40 needs to draw fade effect
	*(DWORD*)0x44A25D = (DWORD)som_scene_44d810-0x44A261;*/
	//0044D7F6 E8 15 00 00 00       call        0044D810
	*(DWORD*)0x44D7F7 = (DWORD)som_scene_44d810-0x44D7FB;

	//TODO: REQUIRE som_scene_alit TO ALLOW BATCHING?
	som_scene_alit = 1&&
	EX::INI::Option()->do_lights
	||EX::INI::Bugfix()->do_fix_lighting_dropout;
	//disable lighting subroutine (411210)
	if(som_scene_alit) *(BYTE*)0x411210 = 0xC3; //ret

	//trying to fix VertexBuffer lock problem inside of 413940
	//(it seems to rely on the contents never being discarded)
	//
	//
	//COLLECTING BRANCHES
	//top of draw/lock loop
	//00414042 0F 86 0B 04 00 00    jbe         00414453
	//if(SetTexture)
	//00414075 0F 8C 5D 03 00 00    jl          004143D8
	//DrawIndexPrimitiveVB
	//004143FC FF 91 80 00 00 00    call        dword ptr [ecx+80h]
	//3 jumps?
	//0041421A 0F 87 67 01 00 00    ja          00414387  
	//JUMP TABLE (00414227 00414282 004142dd 00414335)
	//REMINDER: tried forcing the table to all four to no avail
	//00414220 FF 24 95 CC 44 41 00 jmp         dword ptr [edx*4+4144CCh]  
	//00414227 85 C9                test        ecx,ecx  
	//00414229 0F 8E 58 01 00 00    jle         00414387  
	//
	//
	//SEEING WHAT HAPPENS 
	//(41406A/SetTexture fails... passed invalid pointer)
	//00414055 E8 76 47 03 00       call        004487D0
	//memset((void*)0x414055,0x90,5);

	//TODO? can this be part of som_mocap_403BB0?
	//EXTENSION?
	//default is 2.0 (40000000)
	//NOTE: 6 is the buffer used going backward/sideways
	enum{ six=0x40C00000 };
	//pushing frustum back (KF2's waterfall goes outside 2x2 boundary)	
	//why is there 2 paths?
	//00417DCA A1 D4 83 45 00       mov         eax,dword ptr ds:[004583D4h]
	*(BYTE*)0x417DCA = 0xB8; //mov eax,IMM
	*(DWORD*)0x417DCB = six; //6.0
	//standard path?
	//004134B2 8B 0D D4 83 45 00    mov         ecx,dword ptr ds:[4583D4h] 
	*(BYTE*)0x4134B2 = 0xB9; //mov ecx,IMM
	*(DWORD*)0x4134B3 = six; //6.0
	*(BYTE*)0x4134B7 = 0x90; //NOP
	//
	//4C23F4 is the draw distance, [esp+20] is [004583D4h]
	//00403CFE D9 05 F4 23 4C 00    fld         dword ptr ds:[4C23F4h]  
	//00403D04 D8 44 24 20          fadd        dword ptr [esp+20h]


	//TRANSPARENT TILES (ETC.) 
	//
	// 413f10 renders tiles. the objective here is to 1) keep it from
	// locking/unlocking the vbuffer in an unhealthy pattern. 2) have
	// the tile/model passed to a filter that piggy-backs on the Lock
	// call that will A) collect transparent models, and B) on Unlock 
	// animate the UV map as necessary
	// 
	// the filter is passed EDI and the saved value of ESI along with
	// the stack pointer for forwarding to the Lock method (or not if
	// the lock is held for the duration of 413f10, and flushed where
	// necessary)
	//
	//stores the per-texture MSM pointer (will need tile index also)
	//004140C5 89 74 24 1C          mov         dword ptr [esp+1Ch],esi
	//calls IDirect3DVertexBuffer7::Lock (going to hollow this out)
	//004141B1 6A 00                push        0  
	//will need to pass this stack pointer (and esp+1C from 004140C5)
	//004141B3 8D 4C 24 28          lea         ecx,[esp+28h]  
	//004141B7 51                   push        ecx  
	//004141B8 68 20 28 00 00       push        2820h  
	//004141BD C7 44 24 30 00 00 00 00 mov         dword ptr [esp+30h],0  
	//004141C5 8B 10                mov         edx,dword ptr [eax]  
	//004141C7 50                   push        eax  
	//004141C8 FF 52 0C             call        dword ptr [edx+0Ch]  
					/*2020: code[] includes this older change
		//0x0219fa50 <- 0x101df9c0
		//0x0219fa34 //219fa50-219fa34=1C (no change)
		BYTE *p = (BYTE*)0x4141B1;		
		*(DWORD*)p = 0x1c244c8d; //lea ecx,[esp+1ch] //model
		p+=4;
		*p++ = 0x51; //push ecx
		*(DWORD*)p = 0x28244c8d; //lea ecx,[esp+28h] //lock pointer
		p+=4;
		*p++ = 0x51; //push ecx
		*p++ = 0x50; //push eax //IDirect3DVertexBuffer7		
		*p++ = 0x57; //push edi //tile
		p+=8; //mov dword ptr [esp+30h],0 (defaulting lock size)
		p[-5] = 0x34; //slight adjustment
		*p = 0xE8; //call som_scene_4137F0_Lock2
		*(DWORD*)(p+1) = (DWORD)som_scene_4137F0_Lock2-DWORD(p+5);
		p[5] = 0x90; //nop
		//TODO: MUST REPROGRAM Unlock CALL
					*/
		//2022: prevent Lock failure from letting ibuffer_size go
		//over its limit by always zeroing
		//004143e0 72 59           jc        0041443b
		*(BYTE*)0x4143e1-=12; //if(1)
		{
			//really this is not enough, but I've never seen Lock
			//fail via dx.d3d9c.cpp???
			//
			// NOTE: the real problem was som_scene_413f10_maybe_flush
			// wasn't checking _transparent_indicesN but this is still
			// good practice in case Lock fails, and it's much simpler
			// than the above approach given the additional complexity
			//
			char code[] = 
			"\xA1\x2C\x89\x59\x00"       //mov         eax,dword ptr ds:[0059892Ch]  
			"\x8D\x4C\x24\x1C"          //lea         ecx,[esp+1Ch]  
			"\x51"                   //push        ecx  
			"\x8D\x4C\x24\x28"          //lea         ecx,[esp+28h]  
			"\x51"                   //push        ecx  
			"\x50"                   //push        eax  
			"\x57"                   //push        edi  
			"\xC7\x44\x24\x34\x00\x00\x00\x00" //mov         dword ptr [esp+34h],0  
			"\xE8\x48\x94\xAA\x7A"       //call        som_scene_4137F0_Lock2 (7AEBD600h)  
			"\x90"                   //nop  
			"\x85\xC0"                //test        eax,eax  
			"\x0F\x85\xB4\x01\x00\x00"   //jne         00414387
			"\x8B\x0D\x20\xA9\x58\x00"    //mov         ecx,dword ptr ds:[58A920h]  
			"\x8B\x6C\x24\x10"          //mov         ebp,dword ptr [esp+10h]  
			"\x03\xCD"                //add         ecx,ebp  
			"\x89\x0D\x20\xA9\x58\x00";   //mov         dword ptr ds:[58A920h],ecx  
			memcpy((void*)0x41419A,code,sizeof(code)-1);
			*(DWORD*)0x4141B4 = (DWORD)som_scene_4137F0_Lock2-0x4141B8;
			*(BYTE*)0x4141bd+=18;
			//I had a nightmare time finding this
			//0041417A 7E 28                jle         004141A4
			*(BYTE*)0x41417b-=10;
		}

	//DEPTH-SORTING (can be improved)	
	{
		//NOTE: this algorithm is done in homogeneous space, multiplying
		//the transforms as neccessary... I can't understand why it does
		//not just work in world space
		//som_scene_44d5a0 uses a stable_sort algorithm in order to keep
		//some bad glitches at bay. also, the lighting extension work is
		//repurposing the 4th column of the element's transforms... that
		//might produce bad depths using 44D5a0. I can't see any, but it
		//appears to use the whole transform

		//ESI is IDirect3DDevice7
		//0044D5E6 8B 06                mov         eax,dword ptr [esi] 
		//GetTransform? (VIEW,PROJECTION)
		//0044D5F4 FF 50 30             call        dword ptr [eax+30h]
		//0044D604 FF 52 30             call        dword ptr [edx+30h]
		//0044d6eb does the same, so must be multiply
		//0044D61C E8 4F C9 FF FF       call        00449F70
		//loop...
		//switches on mode (jump table has 5 entries?)
		//0044D63F 83 F9 04             cmp         ecx,4		
	}
	//0040253D E8 5E B0 04 00       call        0044D5A0
 	*(DWORD*)0x40253E = (DWORD)som_scene_44d5a0-0x402542;

	//EXPERIMENTAL
	//
	// 2021: OpenGL performance on my new AMD system was really
	// bad, so I did a little project to see if it was vbuffers
	// 
	// this extension fills the buffers up to the brim prior to
	// issuing batched drawing commands
	//
	//7 doesn't implement partial_Lock?
	if(7<EX::directx())
	if(0&&EX::debug&&som_scene_alit)
	{
		som_scene_batchelements_hold = ~0u; //YUCK (initialize)

		//HISTORY
		// 
		// som_db.exe allocates 4096 vertices but only actually
		// uses 128. a theory is the FVF is 32B and 32*128=4096
		// (or could be the chunking and vbuffer coders weren't
		// in communication)
		// 
		//som_hacks_CreateVertexBuffer
	//	som_scenery::vbuffer_size = 4096*(EX::debug?4:1); //EXTENSION?

		//return 2 to som_scene_44d810 so it can apply
		//an offset into the batched vbuffer
		memset((void*)0x44deeb,0x90,0x44df5d-0x44deeb); //114
		*(WORD*)0x44deeb = 0x70eb; //jmp (faster than nop?)
		*(BYTE*)0x44df61 = 2; 

		//NOTE: I think 128 is the theoretical minimum
		//based on bone limits. however since the chunk
		//size was about 128 (no relation) vertices each
		//model has a lot of chunks, but probably not 128
		som_scene_batchelements = som_scenery::alloc(128);

		//UNFINSHED
		#ifdef NDEBUG
		//#error do_red? som_scene_volume_select?
		int todolist[SOMEX_VNUMBER<=0x1020704UL];
		#endif
	}
	//TEARDOWN
	//these release 3 vbuffers, all but the MSM vbuffer
	//this sets the MDO/MDL one to som_scene::vbuffers[0]
	//prior to teardown
	//00401FBF E8 9C B0 04 00       call        0044D060
	//0044CF60 E8 FB 00 00 00       call        0044D060
	*(DWORD*)0x401Fc0 = (DWORD)som_scene_44d060-0x401Fc4;
	*(DWORD*)0x44CF61 = (DWORD)som_scene_44d060-0x44CF65;

	som_scene_onreset_passthru = DDRAW::onReset; //2021
	DDRAW::onReset = som_scene_onreset;
}

/////////////////////////////////////////////////////////

//THIS CODE MAY HIT FPS HARD IN OTHER TRANSLATION UNIT?
static void __cdecl som_MPY_417c60_recursive(float *_1, int _2, int _3, int _4)
{
	//NOTE: some names are derived from Ghidra's variable names
	
	som_MPX &mpx = *SOM::L.mpx->pointer;	
	int ls = mpx[SOM::MPX::layer_selector];
	auto *lp = (SOM::MPX::Layer*)&mpx[SOM::MPX::layer_0];
	auto &l = lp[ls];

	auto *p2 = l.pointer2; //iVar7
	auto *p1 = l.pointer1; //piVar2[4]

	auto &t2 = p2[_3]; //piVar1

 	int iv8 = t2.unknown1[0]; //iVar8

	if((iv8!=-1)&&(iv8!=_4))
	som_MPY_417c60_recursive(_1,_2,iv8,_3);

	//uVar3 = piVar1[3];
	int uv3 = t2.leaf; if(uv3!=-1) //uVar3
	{
		/*Ghidra's code doesn't seem to match this???
		00417ccc 8b 54 24 24     MOV        EDX,dword ptr [ESP + param_3]
		00417cd0 8b 4a 0c        MOV        ECX,dword ptr [EDX + 0xc]
		00417cd3 8b 54 24 10     MOV        EDX,dword ptr [ESP + 0x10] //local_8 (l)
		00417cd7 8b 6a 10        MOV        EBP,dword ptr [EDX + 0x10]
		00417cda 8b c6           MOV        EAX,ESI
		00417cdc 99              CDQ //makes EDX 0 (probably always)
		00417cdd c1 e1 05        SHL        ECX,0x5	//*0x20
		00417ce0 8b 6c 29 1c     MOV        EBP,dword ptr [ECX + EBP*0x1 + 0x1c]
		00417ce4 83 e2 07        AND        EDX,0x7
		00417ce7 03 c2           ADD        EAX,EDX
		00417ce9 8b ce           MOV        ECX,ESI
		00417ceb c1 f8 03        SAR        EAX,0x3
		00417cee 81 e1 07        AND        ECX,0x80000007
					00 00 80
		00417cf4 79 05           JNS        LAB_00417cfb
		00417cf6 49              DEC        ECX
		00417cf7 83 c9 f8        OR         ECX,0xfffffff8
		00417cfa 41              INC        ECX
								LAB_00417cfb                                    XREF[1]:     00417cf4(j)  
		00417cfb ba 01 00        MOV        EDX,0x1
					00 00
		00417d00 d3 e2           SHL        EDX,CL
		00417d02 84 14 28        TEST       byte ptr [EAX + EBP*0x1],DL
		*/
		UINT uv6 = uv3&0x80000007; //uVar6
		if((int)uv6<0)
		{
			assert(0); //unnecessary?
			uv6 = (uv6-1|0xfffffff8)+1; 
		}
		assert(uv3>=0); //CDQ should be 0?
		if(0==(
		//  *(int*)(*(int*)(iVar7+_2*0x2c+0xc)*0x20+0x1c+piVar2[4]))
			p1[p2[_2].unknown1[3]].pointer_into_pointer4
			[
		//	((int)(uv3+((int)uv3>>0x1f&7))>>3) //0x1f???
		 	((int)(uv3+((int)uv3>>0x1f&7))/8) //0x1f???
			]			
		//	&((BYTE)(1<<((BYTE)uv6&0x1f)))) //0x1f???
		    &((BYTE)(1<<((BYTE)uv6&0x1f))))) //0x1f???
		{			
			return;
		}
	}
	if(_2!=_3)
	{
		//float iv5,iv10,iv8,iv7;
		float x1,y1,x2,y2;
		if(uv3==-1) //uVar3
		{
			x1 = t2.xy1[0]; y1 = t2.xy1[1]; 
			x2 = t2.xy2[0]; y2 = t2.xy2[1];
		}
		else
		{
			auto &t1 = p1[uv3];
			x1 = t1.xy1[0]; y1 = t1.xy1[1];
			x2 = t1.xy2[0]; y2 = t1.xy2[1];
		}
		//FUN_00417e50_tile_visibility_math?(_1,iVar5,iVar10,iVar8,iVar7)
		if(!((BYTE(__cdecl*)(float*,float,float,float,float))0x417e50)(_1,x1,y1,x2,y2))
		{
			return;
		}
	}

	WORD *tod = (WORD*)SOM::L.mpx->tiles_on_display;
	DWORD &ttd = SOM::L.mpx->tiles_to_display;

	int iv7 = (int)t2.unknown1[1]; //iVar7
	if((iv7!=-1)&&(iv7!=_4))
	som_MPY_417c60_recursive(_1,_2,iv7,_3);

	iv7 = (int)t2.unknown1[2]; //iVar7
	if((iv7!=-1)&&(iv7!=_4))
	som_MPY_417c60_recursive(_1,_2,iv7,_3);

	if(-1!=(iv7=(int)t2.unknown1[3])) //iVar7
	{
		BYTE bit = 1<<-ls; //EXTENSION

		auto add = [&](SOM::MPX::Layer::tile *t)
		{
			DWORD yx = t-lp->tiles;
			if(l.tiles[yx].pointer)
			{
				if(~t->msb&0x80)
				{
					//TODO: check 403cc0 here?				
							
					t->msb|=0x80; 

					tod[ttd++] = yx/100|yx%100<<8;
				}

				t->msb|=bit;
			}
		};

		int w = l.width, h = l.height;

		auto &t1 = p1[iv7]; //iVar8
		for(DWORD i=0;i<t1.unknown2_count;i++)
		{
			int y = t1.unknown2_pointer[i][1]; //iVar5
			int x = t1.unknown2_pointer[i][0]; //iVar7

			int yx = w*y+x; auto *t = lp->tiles+yx;

			/*som_MPY_4133E0_layers calls 403cc0
			//FUN_00403cc0_tile_visibility_sub?_frustum_test_only?
			if(((BYTE(__cdecl*)(float,float,int))0x403cc0)(x*2,y*2,0x40000000))*/
			if(t->msb&0x80&&l.tiles[yx].pointer)
			{
			//	if(!t.msb) tod[ttd++] = (WORD)x<<8|(WORD)y;

				if(t->_bsp) //HACK: this reduces need for "pin" in SOM_MAP
				{
					auto *s = t-1, *u = t+1;

					int i = 3; if(y)
					{
						s-=w; t-=w; u-=w;							
					}
					else i--; if(y+1==h) i--;

					for(;i-->0;add(t),s+=w,t+=w,u+=w)
					{
						if(x) add(s); if(x+2<w) add(u);
					}
				}
				else t->msb|=bit;
			}
			//else assert(0); //adding dummies everywhere for layers
		}
	}
}
static BYTE __cdecl som_MPY_413540() //tile visibility
{
	som_MPX &mpx = *SOM::L.mpx->pointer;	
	int &ls = mpx[SOM::MPX::layer_selector]; 
	auto *lp = (SOM::MPX::Layer*)&mpx[SOM::MPX::layer_0];

	assert(ls==0);
	//for(DWORD i=lp->height*lp->width;i-->0;)
	//lp->tiles[i].msb = 0;
	//auto &ttd = SOM::L.mpx->tiles_to_display;
	//ttd = 0;

	float *x_z = SOM::L.view_matrix_xyzuvw;

	for(;ls>-7;ls--)
	{
		auto &l = lp[ls]; if(!l.tiles) continue;

		if(!l.pointer2)
		{
			assert(0); continue;
		}

		BYTE bit = 1<<-ls; //EXTENSION

		//iVar5 = piVar2[6];
		auto *p2 = l.pointer2;

		//iVar6 = *(int*)(iVar5+0xc);
		int leaf = p2->leaf;
		//_DAT_00598940 = 0;
		int node = 0;
		SOM::L.mpx->current_node = 0; //NODE_%s readout???
		auto *p = p2; //iVar7 = iVar5;
		for(;;)
		{
			if(leaf!=-1)
			{
				SOM::L.mpx->current_node = node;
				SOM::L.mpx->current_leaf = leaf; //LEAF_%s readout???

				WORD *tod = (WORD*)SOM::L.mpx->tiles_on_display;
				//DWORD &ttd = SOM::L.mpx->tiles_to_display;
				//ttd = 0; //???

				som_MPY_417c60_recursive(x_z,node,node,-1);

				int x,y; //&local_20,&local_1c
				//FUN_00415bc0_transform_pos_to_tile_xy?(x_z[0],x_z[2],&x,&y);
				((BYTE(__cdecl*)(FLOAT,FLOAT,int*,int*))0x415bc0)(x_z[0],x_z[2],&x,&y);
				int i = y-3;
				for(;i<=y+3;i++) if(-1<i&&i<(int)l.height)
				{
					for(int j=x-3;j<=x+3;j++) if(-1<j&&j<(int)l.width)
					{
						int yx = l.width*i+j;

						auto &t = lp->tiles[yx];

						if(l.tiles[yx].pointer)
						{							
						//	if(!t.msb) tod[ttd++] = (WORD)(j<<8|i);

							//NOTE: this is 6x6 squares by the pc
							t.msb|=0x80|bit;
						}
					}
				}
				goto _3; //return 1;
			}
			static const float l = 1; //1

			int bm = p->blinders; //bitmask
			if(~bm&1)
			{
				if(~bm&2)
				{
					if(~bm&4)
					{
						if(~bm&8)
						{
							assert(0); //NOP?
						}
						else
						{
							if(p->x*2+l<x_z[0]) goto _2; goto _1;
						}
					}
					else
					{
						if(p->z*2-l>x_z[2]) goto _2; goto _1;
					}
				}
				else
				{
					if(p->x*2-l>x_z[0]) goto _2; goto _1;
				}
			}
			else if(p->z*2+l<x_z[2]) _2:
			{
				node = p->left;
			}
			else _1:
			{
				node = p->right;
			}

			p = p2+node; leaf = p->leaf;
		}
	
		_3:; //return 1;
	}
	ls = 0;

	return 1;
}
extern bool som_MPY_nobsp = false;
extern void som_MPY_4133E0_layers() //2023
{	
	som_MPX &mpx = *SOM::L.mpx->pointer;
	assert(0==mpx[SOM::MPX::layer_selector]);
	
	auto &mpx2 = *SOM::L.mpx;
	auto *dl = mpx2.tiles_on_display;
	DWORD jN = mpx2.tiles_to_display;

	typedef SOM::MPX::Layer L;
	auto *lp = (L*)&mpx[SOM::MPX::layer_0];

	auto &flags = mpx[SOM::MPX::flags];
	int bsp1 = flags&1; 
	bool bsp = bsp1==1;
	if(som_MPY_nobsp) //2024
	bsp = false;
	flags&=~1;
	//if(!bsp)
	{	
		//2024: 3x3 sample to reduce "pins" in SOM_MAP
		if(bsp1) if(0)
		{
			for(DWORD i=lp->height*lp->width;i-->0;)
			{
				lp->tiles[i].msb = 0; //unused
			}
		}
		else for(DWORD j=jN;j-->0;) //faster?
		{
			auto &yx = dl[j];

			DWORD k = 100*yx[0]+yx[1];

			lp->tiles[k].msb = 0;
		}

		//2023: this now includes every cell
		//(som_MPY_reprogram nops 4134a0)
		((void(__cdecl*)())0x4133E0)();
	}
	flags|=bsp1; //!
	
	jN = mpx2.tiles_to_display; //2024: 3x3?	

	if(bsp)
	{
		for(DWORD j=jN;j-->0;)
		{
			auto &yx = dl[j];

			DWORD k = 100*yx[0]+yx[1];

			lp->tiles[k].msb = 0x80; //not ff

			lp->tiles[k]._bsp = 1; //2024: 3x3?
		}
		
		som_MPY_413540();
	}

	jN = mpx2.tiles_to_display; //2024: 3x3?

	if(*(WORD*)0x4134a0==0x9090) //som_MPY_reprogram
	{
		DWORD kept = 0;	
		int lN = 0; while(lN>-7&&lp[lN].tiles) lN--;
		for(DWORD j=jN;j-->0;)
		{
			auto &yx = dl[j];

			DWORD k = 100*yx[0]+yx[1]; assert(k<=10000);

			if(bsp)
			{
				lp->tiles[k]._bsp = 0; //2024: 3x3? 

				for(int l=0;l>lN;l--) 
				if(lp[l].tiles[k].nobsp)				
				lp->tiles[k].msb|=1<<-l;

				if(0x80==lp->tiles[k].msb)
				{
					lp->tiles[k].msb = 0;
					*(WORD*)&yx = 0xffff; continue;
				}
			}

			for(int l=0;l>lN;l--) 
			if(lp[l].tiles[k].pointer)		
			goto keep;
			{
				lp->tiles[k].msb = 0;
				*(WORD*)&yx = 0xffff; continue;
			}
			keep: kept++;		
		}
		mpx2.tiles_to_display = kept;
	
		int y = (int)(SOM::xyz[2]*0.5f);
		int x = (int)(SOM::xyz[0]*0.5f);
		struct yx{ BYTE y,x; };
		std::sort((yx*)dl,(yx*)dl+jN,[x,y](yx &a, yx &b)->bool
		{
			if(*(WORD*)&b==0xffff) 
			return *(WORD*)&a!=0xffff; //marked?

			//sort back-to-front (tiles are drawn in reverse)
			int ax = x-a.x, ay = y-b.y;
			int bx = x-b.x, by = y-b.y;
		//	return ax*ax+ay*ay>bx*bx+by*by; 
			return ax*ax+ay*ay<bx*bx+by*by; 
		});
	}
}
extern void som_MPY_413f10()
{
	auto &mpx2 = *SOM::L.mpx;
	som_MPX &mpx = *mpx2.pointer;
	int &ls = mpx[SOM::MPX::layer_selector]; //DEBUGGING

	typedef SOM::MPX::Layer L;
	auto *lp = (L*)&mpx[SOM::MPX::layer_0];
	int lN = 0; while(lN>-7&&lp[lN].tiles) lN--;

	float *p = 0; //DETAIL

	//scanning for the crash so I can inspect it
	auto *dl = mpx2.tiles_on_display;
	DWORD jN = mpx2.tiles_to_display;
	auto tc = mpx[SOM::MPX::texture_counter];
	auto tp = (WORD*)mpx[SOM::MPX::texture_pointer];
	
	//			auto *ib = SOM::L.mpx->ibuffer;
	//			auto *vb = SOM::L.mpx->vbuffer;	
	//			DWORD &ibN = SOM::L.mpx_ibuffer_size;
	//			DWORD &vbN = SOM::L.mpx_vbuffer_size; 
	//			float *vp = (float*)mpx[SOM::MPX::vertex_pointer];

	if(1) //interleaved?
	{
		int chunk = 16; 
		
		DWORD jj = 0; jj: 
		DWORD jjN = min(jN,jj+chunk);

		for(int i=0;i<tc;i++)
		{
			//for(DWORD j=jN;j-->0;) //413F10 scans backwards
			for(DWORD j=jj;j<jjN;j++) //this makes std::sort with ffff easier
			{
				auto &yx = dl[j];
				float xyz[3] = {2.0f*yx[1],0,2.0f*yx[0]};
				DWORD k = 100*yx[0]+yx[1]; assert(k<=10000);
				if(int msb=lp->tiles[k].msb)
				{
					for(int l=0;l>lN;l--) if(msb&1<<-l)
					{
						auto &t = lp[l].tiles[k];
						if(auto*tsp=t.pointer)
						{
							xyz[1] = t.elevation;

							auto *pp = tsp->pointer;
							DWORD pc = tsp->counter;
							for(;pc-->0;pp++)						
							if(i==pp->texture)
							som_scene_413f10(xyz,t.rotation,pp,p);
						}
					}
				}
				else assert(0);
			}
		}
		jj = jjN; chunk = chunk*3/2; 

		if(jjN<jN) goto jj;
	}
	else for(int l=0;l>lN;l--) //uninterleaved
	{
		assert(0); //msb?

		if(ls=l)
		{
			if(*(WORD*)0x4134a0!=0x9090) //som_MPY_reprogram
			{
				((void(__cdecl*)())0x4133E0)();
			}
		}

		if(0) //TESTING PERFORMANCE
		{
			((void(__cdecl*)())0x413F10)(); //draw
		}
		else for(int i=0;i<tc;i++)
		{
			//for(DWORD j=jN;j-->0;) //413F10 scans backwards
			for(DWORD j=0;j<jN;j++) //this makes std::sort with ffff easier
			{
				auto &yx = dl[j];
				float xyz[3] = {2.0f*yx[1],0,2.0f*yx[0]};
				DWORD k = 100*yx[0]+yx[1]; assert(k<=10000);
			//	for(int l=0;l>lN;l--) 
				{		
					auto &t = lp[l].tiles[k];
					if(t.msm==0xffff) continue;

					xyz[1] = t.elevation; int a = t.rotation;

					if(auto*tsp=t.pointer)
					{
						auto *pt = tsp->pointer;
						DWORD pc = tsp->counter;
						for(;pc-->0;pt++)
						{
							if(i==pt->texture)
							{
								if(0) //TESTING PERFORMANCE
								{	
									//RESULTS: need to put these functions in the same file
									//(translation unit) to avoid having a much reduced fps

									/*som_scene_413f10_maybe_flush()
									{										
										DWORD txr = tp[pt->texture];
										DWORD sss = som_scene_state::texture;

										int indices2 = pt->triangle_indicesN+pt->_transparent_indicesN;								

										if(txr!=sss||896<vbN+pt->vcolor_indicesN||2688<ibN+indices2)
										{
											if(ibN) //som_scene_413f10_maybe_flush(0)
											{ 
												vb->Unlock();	
												DDRAW::Direct3DDevice7->DrawIndexedPrimitiveVB
												(DX::D3DPT_TRIANGLELIST,vb,0,vbN,ib,ibN,0);	
												vbN = ibN = 0;
											}
			
											if(txr!=sss&&txr<1024)
											{	
												som_scene_state::texture = txr;
				
												DDRAW::IDirectDrawSurface7 *t = SOM::L.textures[txr].texture;
												DDRAW::Direct3DDevice7->SetTexture(0,t);

												//NOTE: doing after SetTexture because it had set psColorkey 
									//			som_scene_volume_select(txr);
											}
										}
									}

									//som_scene_413f10()
									{
										if(!ibN)
										{
											DWORD f = DDLOCK_DISCARDCONTENTS|DDLOCK_WAIT|DDLOCK_WRITEONLY;
											SOM::L.mpx->vbuffer->Lock(f,(void**)&p,0);	
										}
										assert(ibN<=2688);

										WORD *q = ib+ibN; //emulate 413f10

										int i = pt->triangle_indicesN;
										ibN+=i;
										memcpy(q,pt->triangle_indices,2*i);		
										WORD vo = vbN;
										for(;i-->0;q++)
										*q+=vo;
										i = pt->vcolor_indicesN;
										vbN+=i;
										DWORD *vc = *pt->vcolor_indices;		
										for(;i-->0;vc+=2,p+=6)
										{
											float *v = vp+5*vc[0];
											p[0] = xyz[0];
											p[1] = xyz[1]+v[1];
											p[2] = xyz[2];
											switch(a) //might want to unroll this part 
											{
											case 0: p[0]+=v[0]; p[2]+=v[2]; break; //S
											case 1: p[0]-=v[2]; p[2]+=v[0]; break; //W 
											case 2: p[0]-=v[0]; p[2]-=v[2]; break; //N
											case 3: p[0]+=v[2]; p[2]-=v[0]; break; //E
											}
											(DWORD&)p[3] = vc[1];
											p[4] = v[3];
											p[5] = v[4];									
										}
									}*/
								}
								else som_scene_413f10(xyz,a,pt,p);
							}
						}
					}
					else assert(0);
				}
			}
		}
	}
	ls = 0; //uninterleaved?

	som_scene_413f10_maybe_flush(nullptr);
}
extern void som_MPY_413f10_mhm()
{
	DDRAW::Direct3DDevice7->SetTexture(0,0);

	auto &mpx2 = *SOM::L.mpx;
	som_MPX &mpx = *mpx2.pointer;

	typedef SOM::MPX::Layer L;
	auto *lp = (L*)&mpx[SOM::MPX::layer_0];
	int lN = 0; while(lN>-7&&lp[lN].tiles) lN--;
	
	float *p = 0; //DETAIL

	auto *dl = mpx2.tiles_on_display;
	DWORD jN = mpx2.tiles_to_display;

	auto **pp = (SOM::MHM**)mpx[SOM::MPX::mhm_pointer];

	int chunk = 16; //UNNECESSARY //might change colors?
	{	
		DWORD jj = 0; jj: 
		DWORD jjN = min(jN,jj+chunk);

		//for(DWORD j=jN;j-->0;) //413F10 scans backwards
		for(DWORD j=jj;j<jjN;j++) //this makes std::sort with ffff easier
		{
			auto &yx = dl[j];
			float xyz[3] = {2.0f*yx[1],0,2.0f*yx[0]};
			DWORD k = 100*yx[0]+yx[1]; assert(k<=10000);
			if(int msb=lp->tiles[k].msb)
			{
				for(int l=0;l>lN;l--) if(msb&1<<-l)
				{
					auto &t = lp[l].tiles[k]; if(t.mhm!=0xffff)
					{
						auto &h = *pp[t.mhm]; if(h.polies) //debugging
						{
							assert(t.msm==t.mhm||t.msm==0xffff);

							xyz[1] = t.elevation;
							som_scene_413f10_mhm(xyz,t.rotation,h,p);
						}
					}
				}
			}
			else assert(0);
		}

		jj = jjN; chunk = chunk*3/2; 

		if(jjN<jN) goto jj;
	}

	som_scene_413f10_maybe_flush(nullptr);
}

extern void som_MPY_reprogram()
{
	//accept empty for frustrum visibility test? layers? nobsp?
	//004134a0 74 67 JZ LAB_00413509
	*(WORD*)0x4134a0 = 0x9090;
	//set msb for all layers
	//00413501 81 c9 00 00 00 80 OR ECX,0x80000000
	*(BYTE*)0x413506 = 0xff; 

	//004136b6 e8 a5 45 00 00 CALL 00417c60
	*(DWORD*)0x4136b7 = (DWORD)som_MPY_417c60_recursive-0x4136bb;
}