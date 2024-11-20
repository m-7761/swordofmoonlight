	
#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

#include <vector>
#include <bitset>
#include <algorithm>
#include <hash_map> //Windows XP
#include <unordered_map> //snd?

#include "dx.ddraw.h"
#include "dx.dsound.h"

#include "SomEx.ini.h" //EX::INI::Joypad
#include "Ex.input.h" //EX::Affects[1]

#include "som.game.h"
#include "som.state.h"
#include "som.tool.hpp"
#include "som.extra.h"

#include "../lib/swordofmoonlight.h"
#include "../x2mdl/x2mdl.h"

enum{ som_MPX_448780_secured=-1024 };
static void som_MPX_once_per_frame_sky();
extern void som_MPX_4498d0(SOM::Texture*);

extern bool som_MPX_loading_shadows = false; //2024

//EXPERIMENTAL
//som_hacks_thread_main pauses/resumes this 
//thread by acquiring this critical-section
//Create/SetWaitableTimer is needed to stop
//under less than 15 ms
extern SOM::Thread *som_MPX_thread = 0; //2022
extern SOM::TextureFIFO *som_MPX_textures = 0;

extern void som_MPX_prefetch(HANDLE cp=GetCurrentProcess(), 
void *p=(void*)0x401000, size_t s=0x19692B4) //DEBUGGING
{
	static void *f = GetProcAddress
	(GetModuleHandleA("Kernel32.dll"),"PrefetchVirtualMemory");
	struct WIN32_MEMORY_RANGE_ENTRY{ PVOID f; SIZE_T s; }rng = {p,s};
	if(f) ((BOOL(WINAPI*)(HANDLE,ULONG_PTR,void*,ULONG))f)(cp,1,&rng,0);
}

struct som_MPX_mem
{
	//this is primarily to avoid memory
	//allocation when transferring maps

	BYTE *ptr;
	size_t sz,rem;	
	som_MPX_mem *etc;
	som_MPX_mem(size_t approx):etc()
	{
		ptr = new BYTE[rem=approx]; sz = 0;
	}
	~som_MPX_mem()
	{
		delete[] ptr; delete etc;
	}
	void clear()
	{
		rem+=sz; sz = 0; if(etc) etc->clear();
	}
	bool empty(){ return sz==0; }

	void *alloc(size_t n)
	{
		//HACK: advance ptr just in case
		//at end of this block
		if(!n) n = 4;

		while(n%4) n++;

		if(n>rem) return overflow(n);
		
		void *p = ptr+sz;
		
		sz+=n; rem-=n; return p;
	}
	void *overflow(size_t  n) //underbudgeted?
	{
		if(!etc)
		{
			//assert(etc); //this is bad

			size_t l_4th = (sz+rem)/4;

			etc = new som_MPX_mem(max(n,l_4th));
		}
		return etc->alloc(n);
	}

	void prefetch() //EXPERIMENTAL
	{
		//TODO: maybe MPX memory needs prefetch

		HANDLE cp = GetCurrentProcess();
		for(auto*p=this;p&&!p->empty();p=p->etc)
		som_MPX_prefetch(cp,p->ptr,p->sz+p->rem);
	}

	size_t capacity()
	{
		size_t o = sz+rem; 
		if(etc) o+=etc->capacity(); return o;
	}
	size_t front_capacity()
	{
		size_t o = 0; 
		for(auto*p=this;p&&!p->empty();p=p->etc)
		o+=p->sz+p->rem;
		return o;
	}	
	som_MPX_mem *back()
	{
		if(empty()||!etc) return this;

		return etc->back();
	}
	void back_alloc(size_t approx)
	{
		auto *b = back(); delete b->etc;
		if(!b->empty())
		{			
			b->etc = new som_MPX_mem(approx);
			return;
		}
		delete[] b->ptr; new(b) som_MPX_mem(approx);
	}

	bool claim(void *d)
	{
		if(d>=ptr&&d<ptr+sz)
		return true;
		return etc?etc->claim(d):false;
	}
	bool check(void *d, bool b=true){ return b==claim(d); }
};
extern som_MPX_mem *som_MPX_new = 0;
extern wchar_t *som_art_CreateFile_w;
extern int som_MPX_operator_new_lock = 0;
extern void *__cdecl som_MPX_operator_new(size_t sz)
{
	if(som_MPX_new)
	{
		//can't risk allocating to wrong heap
		//HACK: all non-instance data should go
		//through the art system. except audio is
		//covered by som_MPX_43f420 until it's part
		//of the art system
		//if(!som_art_CreateFile_w)
		if(!som_MPX_operator_new_lock) //2023
		{
			return som_MPX_new->alloc(sz); 
		}
		//else assert(!som_art_CreateFile_w);
	}
	return operator new[](sz);
}
extern void __cdecl som_MPX_operator_delete(void *p)
{
	if(som_MPX_new)
	{
		//can't risk allocating to wrong heap
		//HACK: all non-instance data should go
		//through the art system. except audio is
		//covered by som_MPX_43f420 until it's part
		//of the art system
		//if(!som_art_CreateFile_w)
		//
		// TODO: operator_new_lock_raii isn't set
		// up around models_free calls
		//
		if(!som_MPX_operator_new_lock) //2023
		{
			//assert(som_MPX_new->check(p));
			if(som_MPX_new->check(p)) return; //2023
			
			//assert(!"new->check");
			EX_BREAKPOINT(new_check)
		}
		else if(som_MPX_new->check(p)) return; //PARANOIA (2023)
		//else assert(!som_art_CreateFile_w);
	}
	//NOTE: delete[] has its own _BLOCK_TYPE_IS_VALID assert check
	return operator delete[](p);
}

struct som_MPX_sky
{
	som_MPX_sky():instance(),data(),loaded(){}

	SOM::MDO *instance;
	SOM::MDO::data *data; int loaded;

	void make_instance()
	{
		if(!instance) (void*&)instance = 
		((void*(__cdecl*)(void*))0x4458d0)(data);
	}	
	/*not needed anymore with 448158 material fix
	void free_instance()
	{
		((BYTE(__cdecl*)(void*))0x445ad0)(instance);
		instance = 0;
	}*/
};
static std::vector<som_MPX_sky> som_MPX_skies;

struct som_MPX_64
{
	char cmp[64];
	operator size_t()const
	{
		auto it = (size_t*)cmp, itt = (size_t*)(cmp+64);
		size_t o = 2166136261U;		
		while(it!=itt) o = 16777619U*o^*it++; return o;
	}
	typedef stdext::hash_map<som_MPX_64,void*> map;
};

namespace som_MPX_swap //2022
{
	som_MPX_mem *mem = 0;

	static som_MPX_64::map models;
	static som_MPX_64::map images;

	extern bool models_type(void *m)
	{
		//HACK: this is exploiting a
		//guaranteed way to tell the
		//model types apart
		auto *cmp = (SOM::MDO::data*)m;
		return cmp->material_count<65536;
	}	

	extern int &models_refs(void *m)
	{
		if(models_type(m))
		return ((SOM::MDO::data*)m)->ext.refs;
		return ((SOM::MDL::data*)m)->ext.refs;
	}

	static SOM::MHM *models_mhm(void *m)
	{
		if(models_type(m))
		return ((SOM::MDO::data*)m)->ext.mhm;
		return ((SOM::MDL::data*)m)->ext.mhm;
	}
	static BYTE *models_wt(void *m)
	{
		if(models_type(m))
		return ((SOM::MDO::data*)m)->ext.wt;
		if(auto*o=((SOM::MDL::data*)m)->ext.mdo)
		return o->ext.wt; return nullptr;
	}
	
	static void models_unload(void *m)
	{
		//delete models_mhm(m);
		if(auto*mhm=models_mhm(m))
		SOM::Game.free_401580((mhm->~som_MHM(),mhm)); //DUPLICATE
		if(auto*wt=models_wt(m))
		delete[] wt;
		DWORD x = models_type(m)?0x445870:0x4403f0;
		((BYTE(__cdecl*)(void*))x)(m);
	}
	extern void models_free() //som.game.cpp
	{
		auto it = models.begin(),itt = models.end();
		while(it!=itt) if(it->second)
		{
			int cmp = models_refs(it->second); 
			if(cmp<=0)
			{
				assert(cmp==0);
				models_unload(it->second);
				it = models.erase(it);
			}
			else it++;

			//NOTE: don't erase dummy fields... I
			//think they might be used to figure 
			//out if a job to load a model exists
		}
		else it++; 
	}
	extern void kage_release() //som.MPX.cpp
	{
		auto it = models.begin(),itt = models.end();
		for(;it!=itt;it++) if(it->second)
		{
			if(!models_type(it->second))
			{
				auto *m = ((SOM::MDL::data*)it->second);
				if(m->ext.kage2)
				if((unsigned)m->ext.kage2+1u>4096) //deprecated
				{
					for(auto&ea:*m->ext.kage2)
					{
						if(ea.texture) ea.release();
					}
				}
			}
		}
	}
	static void images_free()
	{
		auto it = images.begin(),itt = images.end();
		while(it!=itt) 
		if(auto*s=(SOM::Texture*)it->second)
		{
			//HACK: som_MPX_448780 sets below 0 to 
			//secure the slot over map transitions
			if(s->ref_counter<=0)
			{
				//NOTE: includes some extensions
				som_MPX_4498d0(s);
				SOM::L.textures_counter--;

				it = images.erase(it);
			}
			else it++;

			//NOTE: don't erase dummy fields... I
			//think they might be used to figure 
			//out if a job to load a model exists
		}
		else it++; 
	}
	static void sounds_free()
	{		
		//HACK: this is allowing for a huge margin
		//of error in ref-counting that sould never
		//occur. since the refs are 16-bit it's only
		//+/-1023 (som_MPX_448780_secured) (note that
		//som_MPX_448780's upper margin is +1024)
		WORD(&rcs)[1024] = SOM::L.SND_ref_counts;
		for(int i=1024;i-->0;)		
		if((__int16)rcs[i]<-1)		
		if((__int16)rcs[i]>+2*som_MPX_448780_secured)
		{
			assert(rcs[i]==(WORD)som_MPX_448780_secured);

			rcs[i] = 0; ((BYTE(__cdecl*)(DWORD))0x44b4e0)(i);
		}
	}

	static void _normalize64(char(&m64)[64], const char *a, int i=0)
	{
		//NOTE: keeping "data\" for right now
		if(!i&&a[1]==':'&&a[3]=='>') a+=5; //"A:\\>\\" 

		//need to 0 pad and normalize hash
		for(;(m64[i]=tolower(a[i]))&&i<63;i++)
		{
			if(m64[i]=='/') m64[i] = '\\';
		}
		while(i&&a[i-1]!='.') i--; //remove extension
		while(i<64) m64[i++] = 0;
	}

	static som_MPX_64::map::iterator 
	models_it(std::pair<som_MPX_64,void*> &ins, int i, const char cp[31]) 
	{
		_normalize64(ins.first.cmp,cp-i,i);
		
		return models.insert(ins).first;
	}
	static som_MPX_64::map::iterator 
	models_bit(std::pair<som_MPX_64,void*> &ins, int bit)
	{
		int i = bit/1024-2;
		int j = bit%1024;
		char cats[4] = {15,17,15,16}; //20,22,20,21
		static const DWORD paths[6] = 
		{0x4c0cc8,0x4c11e4,0x4c0728,0x4c1d1c};
		char *a = ins.first.cmp;
		char *b = (char*)paths[i];
	//	if(b[10]!=a[10]) memcpy(a+10,b+10,cats[i]-10);
		if(b[10]!=a[5]) memcpy(a+5,b+10,cats[i]-5); //"data\\"
		
		BYTE *cp; switch(i)
		{
		case 0: cp = SOM::L.NPC_pr2_data[j]; break;
		case 1: cp = SOM::L.enemy_pr2_data[j]; break;
		case 2: cp = SOM::L.obj_pr2_file[j].uc; break;
		case 3: cp = SOM::L.item_pr2_file[j].uc; break;
		default: assert(0);
		}
		return models_it(ins,cats[i],(char*)cp+31);
	}

	extern void* &models_ins(char *a) //som.MDL.cpp
	{
		char m64[64]; _normalize64(m64,a);
		void* &ins = models[*(som_MPX_64*)m64];
		if(ins) models_refs(ins)++;
		return ins;
	}
	extern SOM::Texture* &images_ins(char *a) //som.MDL.cpp
	{
		char m64[64]; _normalize64(m64,a);
		auto* &ins = (SOM::Texture*&)images[*(som_MPX_64*)m64];
		if(ins) 
		{
			//HACK: som_MPX_448780 sets below 0 to 
			//secure the slot over map transitions
			INT32 &rc = ins->ref_counter;
			if(rc<0)
			{
				assert(rc==som_MPX_448780_secured);

				rc+=1-som_MPX_448780_secured;

				assert(rc>0);
			}
			else 
			{
				rc++; assert(rc>1);
			}
		}
		return ins;
	}

	//NOTICE: REF-COUNT IS UNMODIFIED
	static bool images_erase(char *a)
	{
		char m64[64]; _normalize64(m64,a);
		return images.erase(*(som_MPX_64*)m64)!=0;
	}
	static bool models_erase(char *a)
	{
		char m64[64]; _normalize64(m64,a);
		return models.erase(*(som_MPX_64*)m64)!=0;
	}

	extern void *models_data(char *a)
	{
		char m64[64]; _normalize64(m64,a);
		auto it = models.find(*(som_MPX_64*)m64);
		return it!=models.end()?it->second:nullptr;
	}
	extern void models_refresh(char *a);

	typedef swordofmoonlight_mpx_npcs_t::_item npc;
	typedef swordofmoonlight_mpx_items_t::_item item;
	typedef swordofmoonlight_mpx_objects_t::_item obj;

	struct mpx : SOM::mpx_defs_t
	{
		mpx():mem(),wip(),evt(),ambient2()
		{
			_sky_instance = 0;
			corridor_mask = 0;			
			load = loaded = 0;

			instance_mat_estimate = 0;
			instance_mem_estimate = 0;
		}

		som_MPX *mem,*wip; BYTE *evt;

		//read into continguous block
	//	float _fov,znear,zfar,fog; DWORD bg,ambient; //24B
	//	swordofmoonlight_mpx_base_t::light lights[3]; //48B

		std::vector<som_MPX_64> textures;

		//NOTE: using vector to minimize footprint
		std::vector<obj> objects;
		std::vector<npc> npcs;
		std::vector<npc> enemies;
		std::vector<item> items;

	//	struct atlas;
	//	std::vector<void*> models; //REMOVE ME?
	//	std::vector<atlas*> atlases[2];

		BYTE *ambient2[7+1]; //EXTENSION

		HMMIO bgmmio; //HMMIO

		//these are reserved for prioritizing stuff
		//near the entrance, but there's no system
		//for loading models on the fly right now
		int corridor_mask;
		float corridor_xyz[3];

		//PRIVATE IMPLEMENTATION//
		std::bitset<5376> loading;
		size_t load,loaded;
		bool load_canceled;
		void load_models_etc(),unload_models_etc();
	//	void load_models_atlas(),load_mpx_atlas(WORD*,DWORD);
		bool load_bgm(); //2023
		bool load_finished(){ return !corridor_mask; }
		void load_sky(int);
		bool reload_evt(int); //2023

		size_t instance_mat_estimate;
		size_t instance_mem_estimate;

		//WARNING: assuming no single model
		//will go over 256 instances, while
		//that is possible
		BYTE *model_tally; //3*1024

		DWORD last_tick; 
	};	
	static mpx (*maps)[64] = 0;

	struct job //SKETCHY
	{
		//functions (f)
		//0: all fields should be zero
		//1: load
		//2: unload
		//3: other
		//
		//other (g)
		//1: texture
		//2: sky
		//3: model refresh
		unsigned map:6,f:2, g:8, index:10,:6;

		operator const int&()const{ return *(int*)this; }
	};
	struct job_queue : private std::vector<job>
	{
		size_t _next;

		EX::critical _cs;

		job_queue():_next(){}

		bool empty()
		{
			return _next==size();
		}
		void push_back(job j)
		{
			if(!j){ assert(0); return; }

			EX::section raii(_cs);

			bool e = empty();

			if(e||back()!=j) //sky?
			{
				vector::push_back(j);

				//TESTING SCENARIO
				//
				// I think this should signal to the 
				// som.hacks.cpp thread to resume the
				// MPX thread... but this is for tests
				// for now
				//
				if(e) //HACK: undecided how to manage it
				{
					//I think this checking the critical
					//section on the main thread has to
					//be the source of some stalling. if
					//it's limited to an empty queue at
					//least open jobs shouldn't block it
					som_MPX_thread->create(); 
				}
			}
		}
		job remove_front()
		{
			EX::section raii(_cs);

			job j; if(empty()) 
			{
				//assert(0); //signals empty?

				(int&)j = 0;
			}
			else
			{
				j = at(_next++);

				if(_next>=64)
				{
					_next-=64; erase(begin(),begin()+64);
				}
			}
			return j;
		}
	};
	
	static job_queue *jobs = 0;
};
SOM::mpx_defs_t &SOM::mpx_defs(int i)
{
	assert((unsigned)i<64);
	auto &o = (*som_MPX_swap::maps)[i];
	assert(o.mem);
	return o;
}

extern void __cdecl som_MPX_4498d0(SOM::Texture *s)
{
	//som_game_4498d0() //PIGGYBACKING
	{
		long t = s-SOM::L.textures;

		if(SOM::volume_textures) //[Volume]
		if(SOM::VT*&p=(*SOM::volume_textures)[t])
		{
			delete (*SOM::volume_textures)[t];
			p = 0;
		}
	}				
	((void(__cdecl*)(SOM::Texture*))0x4498D0)(s);	
}
static BYTE __cdecl som_MPX_448780(DWORD txr)
{
	if(txr<1024)
	{
		//NOTE: this is not deleting textures
		//so images_free can only delete ones
		//that don't survive map changes. the
		//down side is 

		auto &t = SOM::L.textures[txr];
		INT32 &rc = t.ref_counter;
		if(rc>0) 
		{
			rc--; if(!rc)
			{
				//NOTE: SOM::L.corridor->lock doesn't work
				//for loading maps in background
				if(SOM::Game.item_lock) //item menu?			
				{
					if(som_MPX_swap::images_erase(t.path_identifier))
					{
						som_MPX_4498d0(&t);
						SOM::L.textures_counter--;
						return 1;
					}
					else assert(0);
				}
				else //HACK: secure slot across map transitions
				{
					rc = som_MPX_448780_secured;
				}
			}
		}
		else assert(0);
	}
	return 0; //UNUSED
}
static BYTE __cdecl som_MPX_441990(SOM::MDL::data *mdl, DWORD txr)
{
	//reapplying the same skin messes up som_MPX_448780_secured
	if(mdl->tim_buf&&txr!=*(DWORD*)((BYTE*)(*mdl->tim_buf)+32))
	{
		((BYTE(__cdecl*)(void*,DWORD))0x441990)(mdl,txr);
	}
	else som_MPX_448780(txr); return 1; //dereference txr
}

extern BYTE __cdecl som_MPX_43f4f0(DWORD snd)
{
	WORD(&rcs)[1024] = SOM::L.SND_ref_counts, &rc = rcs[snd];
	if(snd>=1024||!rc) return 0;

	if(rc!=0xffff)
	{
		rc--; if(!rc)
		{
			//NOTE: SOM::L.corridor->lock doesn't work
			//for loading maps in background
			if(SOM::Game.item_lock) //equip?
			{
				((BYTE(__cdecl*)(DWORD))0x44b4e0)(snd);
			}
			else //HACK: secure slot across map transitions
			{
				(__int16&)rc = som_MPX_448780_secured;
			}
		}
	}
	return 1;
}
extern BYTE __cdecl som_MPX_43f420(DWORD snd, BYTE norc)
{
	if(snd>=1024) return 0;

	extern char* *som_SFX_sounds;
	char *aa = som_SFX_sounds[snd]; //2024

	WORD(&rcs)[1024] = SOM::L.SND_ref_counts, &rc = rcs[snd];
	
	if(rc)
	{
		if((__int16)rc<-1) //som_MPX_43f4f0? sounds_free?
		{
			(__int16&)rc+=-som_MPX_448780_secured;

			if(rc!=0)
			{
				//my KF2 project was hitting this on #25
				//it turned out the equip subroutiens had
				//many issues and so som_game_equip had to
				//rewrite them from scratch. it happened to
				//be hitting -1, which is a useful fail-safe
				//so I think it's best to leave this in place
				//assert(rc==0);
				assert(rc==0xffff);
				rc = 0xffff;
			}
		}

		//BUG FIX
		//this is missing from 43f420 (incrementing is wrong)
		if(rc!=0xffff) rc++; 
		
		if(norc) rc = 0xffff; return 1;
	}
		auto *swap = som_MPX_new; //assert(!swap);
		som_MPX_new = 0;

	char a[64]; if(!aa) //0x4C0A94 is "A:\>\data\sound\se\"
	{
		sprintf(a,(char*)0x45f31c,0x4C0A94,snd); 
		
		assert(snd>=1008); //som_SND
	}
	else  sprintf(a,SOMEX_(A)"\\data\\sound\\se\\%s.snd",aa);

	extern DWORD __cdecl som_game_44b450(char*,DWORD,DWORD);
	DWORD ret = som_game_44b450(a,2,snd); //2?

		som_MPX_new = swap; assert(!swap||ret==~0);

	if(ret==~0) return 0;
	
	rc = norc?0xffff:1; return 1;
}
extern void SOM::se(int snd, int pitch, int vol)
{	
	//2022: I think som_game_44b6f0 allows for volume?
	se_volume = vol; assert(pitch>=-24&&pitch<=24&&vol<=0);
	if(som_MPX_43f420(snd,1))
	{
		//2021: this is an unused subroutine that includes a pitch
		//2022: 423480 is for menu sounds (muted if SOM_SYS is so)
		//((void(__cdecl*)(DWORD))0x423480)(snd);
		((void(__cdecl*)(DWORD,INT32))0x43f540)(snd,pitch);		
	}
	else if(EX::debug) //debugging
	{
		som_MPX_43f420(snd,1); //breakpoint
	}
}
extern DWORD som_game_43fa30_freq;
extern float SOM::se3D(float pos[3], int snd, int pitch, int vol)
{
	if(snd<0||snd>=1024) return 0; //2024

	auto &wav = SOM::L.snd_bank[snd]; //2024

	se_volume = vol; assert(pitch>=-24&&pitch<=24&&vol<=0);

	if(som_MPX_43f420(snd,1))
	{
		((void(__cdecl*)(DWORD,INT32,float,float,float))0x43F5B0)
		(snd,pitch,pos[0],pos[1],pos[2]);
	}
	else if(EX::debug) //debugging
	{
		som_MPX_43f420(snd,1); assert(0); //breakpoint
	}

	if(!som_game_43fa30_freq) return 0; //too far away //zero divide
	
	if(!*wav.sb) return 0; //2024: assuming failed to load

	//return?
	//2024: trying to estimate duration of this sound
	//for som.CS.cpp (IGame::PlaySound)
	auto *sb = *(DSOUND::IDirectSoundMaster**)wav.sb;
	float sz = (float)sb->bytes;
	sz*=(float)wav.fmt.nSamplesPerSec/som_game_43fa30_freq;
	return sz/wav.fmt.nAvgBytesPerSec; //som.CS.cpp
}

//2023: som_db refreshes models in som.files.cpp
static std::vector<std::string> som_MPX_refresh;
struct som_MPX_refresh_t{ int *refs, otype, ntype; };
void som_MPX_refresh_swap(int c, void **p, void *d, som_MPX_refresh_t &t)
{
	*t.refs+=c; while(c-->0)
	{
		union //debugging
		{
			SOM::MDO *o;
			SOM::MDL *l;
			void *v;
		};
		v = *p;

		if(t.ntype) //atomic?
		*p = ((SOM::MDO*(__cdecl*)(void*))0x4458d0)(d);
		else *p = ((SOM::MDL*(__cdecl*)(void*))0x440520)(d);

		extern void som_MDL_4409d0_free(SOM::MDL*);
		extern BYTE som_MDO_445ad0_free(SOM::MDO*);
		if(t.otype) som_MDO_445ad0_free(o);
		else som_MDL_4409d0_free(l);
	}
}
const void *SOM::L::zero = 0; //HACK
//void som_MPX_refresh_model(int index)
void som_MPX_refresh_model(char *a)
{
	//auto a = (char*)som_MPX_refresh[index].c_str();

	auto *swap = som_MPX_new; //MEMORY LEAK?
	som_MPX_new = som_MPX_swap::mem; //HACK: try it this way?

	//if(void *d=som_MPX_swap::models_data(a))
	//{
		char m64[64]; som_MPX_swap::_normalize64(m64,a);
		auto it = som_MPX_swap::models.find(*(som_MPX_64*)m64);
	//	return it!=som_MPX_swap::models.end()?it->second:nullptr;
	//}
	if(it!=som_MPX_swap::models.end()) 
	{		
		void* &d = it->second;

		int cmp = som_MPX_swap::models_refs(d);

		void *n,*o = d; //new/old

		extern void *som_MDL_401300_maybe_mdo(char*,int*);
		{
			//HACK: "non-instance data should go through
			//the art system"
			SOM::Game::operator_new_lock_raii raii; //2024 

			som_MPX_swap::models_erase(a);

			n = som_MDL_401300_maybe_mdo(a,nullptr);
		}
		if(n)
		{
			som_MPX_refresh_t t;
			t.otype = som_MPX_swap::models_type(o);
			t.ntype = som_MPX_swap::models_type(n);
			t.refs = &som_MPX_swap::models_refs(n);
			{
				assert(*t.refs==1);

				#define _2(c,x,y) \
				if((!c||x)&&o==(void*&)y)\
				som_MPX_refresh_swap(c,(void**)&x,(void*&)y=n,t);
				#define _1(c,x,y) _2(c,SOM::L.x,SOM::L.y)

				_1(1,arm_MDL,arm_MDL_file);
				for(int i=4;i-->0;)
				_1(1,arm_MDO[i],arm_MDO_files[i]);
				for(int i=4;i-->0;)
				_1(1,arm2_MDO[i],arm2_MDO[i]->i[0]);
				for(int i=250;i-->0;)
				_1(64,items_MDO_table[i],item_MDO_files[i]);
				_1(32,gold_MDO,gold_MDO_file);
				for(int i=255;i-->0;)
				if(1==SOM::L.SFX_refs[i].type)
				_1(16,SFX_refs[i].mdl_instances,SFX_refs[i].mdl_or_txr)				
				for(int i=SOM::L.ai_size;i-->0;)
				{
					auto &ai = SOM::L.ai[i];
					_2(1,ai[SOM::AI::mdl],*(void**)ai[SOM::AI::mdl])
					_2(1,ai[SOM::AI::kage_mdl],SOM::L.kage_mdl_file)
				}
				for(int i=1024;i-->0;)
				_1(0,zero,enemy_mdl_files[i])
				for(int i=SOM::L.ai2_size;i-->0;)
				{
					auto &ai = SOM::L.ai2[i];
					_2(1,ai[SOM::AI::mdl2],*(void**)ai[SOM::AI::mdl2])
					_2(1,ai[SOM::AI::kage_mdl2],SOM::L.kage_mdl_file)
				}
				for(int i=1024;i-->0;)
				_1(0,zero,NPC_mdl_files[i])
				_1(0,zero,kage_mdl_file);
				for(int i=SOM::L.ai3_size;i-->0;)
				{
					auto &ai = SOM::L.ai3[i];
					_2(1,ai[SOM::AI::mdl3],*(void**)ai[SOM::AI::mdl3])
					_2(1,ai[SOM::AI::mdo3],*(void**)ai[SOM::AI::mdo3])
				}
				for(int i=1024;i-->0;)
				{
					_1(0,zero,obj_MDO_files[i])
					_1(0,zero,obj_MDL_files[i])
				}
				for(int i=4;i-->0;)
				_2(1,som_MPX_skies[i].instance,som_MPX_skies[i].data);
				
				assert(SOM::menupcs); //menu object
				char *i = (char*)SOM::menupcs-0xA8+0x3c;
				char *j = i-4; //lvalue
				_2(1,i,j)

				#undef _1
				#undef _2 //finished?
			}
			d = n; som_MPX_swap::models_unload(o);

			*t.refs = cmp; //assert(*t.refs==cmp); //todo: background loaded mpx models?
		}
		else //restore?
		{
			som_MPX_swap::models_ins(a) = o; assert(0);
		}
	}
	else assert(0); //som_MPX_refresh[index].clear();

	som_MPX_new = swap;
}
void som_MPX_swap::models_refresh(char *a) //som.files.cpp
{
	//this needs to be called from the main thread with lock
	//NOTE: this is because some subroutines hold onto the 
	//mdl pointer
	assert(EX::threadmain==GetCurrentThreadId());
	return som_MPX_refresh_model(a);

	/*
	int i = (int)som_MPX_refresh.size();
	while(i-->0)
	{
		if(som_MPX_refresh[i].empty())
		{
			som_MPX_refresh[i] = a; break;
		}
	}
	if(i==-1)
	{
		i = som_MPX_refresh.size();
		som_MPX_refresh.push_back(a);		
	}

	som_MPX_swap::job j = {0,3,3,i}; //other/model
	som_MPX_swap::jobs->push_back(j);*/
}

static HBITMAP som_MPX_bitmap(EX::INI::bitmap &b, HBITMAP hbm)
{
	DWORD wr = hbm?1:0;

	//HACK: leaning on som.game.cpp for SOM_MAP?!
	char a[MAX_PATH];
	sprintf_s(a,"data\\map\\%.*ls.bmp",b.file_name_s,b.file_name);	
	auto w = SOM::Game::project(a);
	if(!w) return 0;

	//NOTE: might use LR_CREATEDIBSECTION to load into memory
	//I worry it will require coding paths for compression or
	//palettes, etc. just to open BMP files
	//
	// https://stackoverflow.com/questions/49784146/ says the
	// GetDIBits bitmap can be a DIB. MSDN says it must be a 
	// "compatible bitmap (DDB)" so I guess it's worth a shot
	//
	if(!hbm) hbm = (HBITMAP)
	LoadImage(0,w,0,0,0,LR_LOADFROMFILE|LR_CREATEDIBSECTION);
	if(!hbm) return 0;

	BITMAP bm; GetObject(hbm,sizeof(BITMAP),&bm);

	BITMAPINFOHEADER bih = {sizeof(BITMAPINFOHEADER)};
	bih.biWidth = bm.bmWidth;    
	bih.biHeight = bm.bmHeight;  
	bih.biPlanes = 1;    
	bih.biBitCount = 24; //32
	bih.biCompression = BI_RGB;    

	//from "Capturing an Image" MSDN example code
	DWORD psz = ((bih.biWidth*bih.biBitCount+31)/32)*4*bih.biHeight;
  
	char *p = new char[psz+1]; //+1 for *(DWORD*) cast

	HDC dc = CreateCompatibleDC(0); 
	GetDIBits(dc,hbm,0,(UINT)bih.biHeight,p,(BITMAPINFO*)&bih,DIB_RGB_COLORS);	
	ReleaseDC(0,dc);

	if(wr) //SOM_MAP.cpp needs to write bitmaps
	{
		BITMAPFILEHEADER bfh = {};
		
		bfh.bfType = 0x4D42; //BM
		bfh.bfOffBits = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
		bfh.bfSize = bfh.bfOffBits+psz;
   
		HANDLE f = CreateFile(w,GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0);

		WriteFile(f,&bfh,sizeof(BITMAPFILEHEADER),&wr,0);
		WriteFile(f,&bih,sizeof(BITMAPINFOHEADER),&wr,0);
		WriteFile(f,p,psz,&wr,0);

		CloseHandle(f);
	}

	b.width = bih.biWidth; b.height = bih.biHeight;

	b.pixels = (BYTE*)p; //SOM_MAP.exe can delete this

	return hbm; //som_db.exe can delete this //DeleteObject(hbm);
}

SOM::MT *SOM::MT::lookup(const char *v)
{	
	typedef stdext::hash_map<std::string,SOM::MT> lut;
	static lut *o = 0;
	static std::string *cmp = 0; //YUCK
	if(o) find:
	{
		*cmp = v; //construct std::string just for a hash?

		auto np = cmp->find_last_of('.'); //C++
		if(~np) cmp->erase(np);
		//map_442830_407470 is depending on x2mdl using /
		//np = cmp->find_first_of('\\'); //2022
		//if(~np) cmp[np] = '/';

		auto it = o->find(*cmp);

		return it==o->end()?0:&it->second;
	}
	o = new lut; cmp = new std::string;

	//2022: SOM_MAP_reprogram disabled?
	//bool uc = SOM::tool==SOM_MAP.exe; //2021

	WIN32_FIND_DATA fd; 
	WIN32_FIND_DATA fdir;
	wchar_t w[MAX_PATH+8],art[MAX_PATH+8];  //2024
	for(int i=0;*EX::data(i);i++)	
	{
		//TODO: EX::user(i) pass
		//TODO? add enemy/texture, npc/texture, obj/texture pass
		int cat2 = swprintf(w,L"%s\\map\\texture\\",EX::data(i));		

		//2022: need to scan subdirectories (just 1 level deep?)
		wmemcpy(w+cat2,L"*",2);
		HANDLE gg = FindFirstFileExW(w,FindExInfoBasic,&fdir,FindExSearchLimitToDirectories,0,0);
		if(gg!=INVALID_HANDLE_VALUE)
		do
		for(int pass=1;pass<=2;pass++)
		{			
			int cat = cat2; if(fdir.cFileName[0]=='.')
			{
				if(fdir.cFileName[1]=='.') continue;

				wmemcpy(w+cat,pass==1?L"*.mm3d":L"*.mdo",6); cmp->clear(); //cmp_s
			}
			else 
			{
				//FindExSearchLimitToDirectories doesn't actually work
				//https://stackoverflow.com/questions/7291797/efficiently-list-all-sub-directories-in-a-directory
				if(~fdir.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
				continue;

				swprintf(w+cat2,pass==1?L"%s/*.mm3d":L"%s/*.mdo",fdir.cFileName);

				cat+=1+wcslen(fdir.cFileName);

				cmp->resize(cat-cat2);

				for(auto i=cmp->size();i-->0;)
				{
					(*cmp)[i] = w[cat2+i]; //w->a
				}
			}

			size_t cmp_s = cmp->size();

			HANDLE ff = FindFirstFile(w,&fd);
			if(ff!=INVALID_HANDLE_VALUE) do
			{
				if(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
				continue;

				wcscpy(w+cat,fd.cFileName); 

				if(pass==1) //2024: art?
				{			
					extern int som_art_model(WCHAR*,WCHAR[]);
					extern int som_art(const wchar_t*,HWND);
					int e = som_art_model(w,art);
					using namespace x2mdl_h;
					if(e&_art&&~e&_lnk)	
					if(!som_art(art,0)) //x2mdl exit code?
					{
						e = som_art_model(w,art); //retry?
					}
					if(0==(e&_mdo)) continue;
				}
				auto *ww = pass==1?art:w;

				namespace mdo = SWORDOFMOONLIGHT::mdo;
				mdo::image_t img; mdo::maptofile(img,ww,'r');
				const mdo::channels_t &c = mdo::channels(img);			
				const mdo::textures_t &t = mdo::textures(img);
				const mdo::materials_t &m = mdo::materials(img);			
				if(!img) unmap: mdo::unmap(img); else
				{
					int i,j = 0;
					const char *pp,*p = t.refs;
					for(pp=p;p<(char*)&m;p++) if(!*p)
					{	
						SOM::MT mat = {};

						for(i=0;i<(int)c.count;i++) if(j==c[i].texnumber)
						{
							int k = c[i].matnumber; if(k>=(int)m.count)
							{
								assert(0); continue; 
							}
						
							memcpy(mat.data,&m[k],7*sizeof(float));

							//darkening?
							if(mat.data[0]<1||mat.data[1]<1||mat.data[2]<1)
							{
								std::swap(mat.data[0],mat.data[2]); //argb
								mat.mode|=1;
							}					
							//transparent?
							if(mat.data[3]!=1) mat.mode|=2;						
							//glowing?
							if(mat.data[4]!=0||mat.data[5]!=0||mat.data[6]!=0)
							{
								std::swap(mat.data[4],mat.data[6]); //argb
								mat.mode|=4;
							}
							//animation?
							int kN = c[i].vertcount; if(kN>4) 
							{	
								//if there is one highest vertex, assuming it 
								//is a fin (local)
								//if there is three, assuming it's an unattached
								//triangle (global)
								float eps = 0.0003f;
								float flr = 10000, ceil = -flr; int ceilN;
								mdo::vertex_t *vb = mdo::tnlvertexpackets(img,i);
								i = -1; //!!
								if(vb) for(k=0;k<kN;k++) 
								{
									float &y = vb[k].pos[1];
									if(y>=ceil-eps)
									{
										if(y>ceil+eps)
										{
											ceilN = 0; ceil = y; i = k; 
										}
										ceilN++;
									}
									if(y<flr) flr = y;
								}								
								if(i!=-1&&fabsf(ceil-flr)>eps*10)
								{
									if(ceilN==1) mat.mode|=8; //relative?
									if(ceilN==3) mat.mode|=16; //absolute?

									//WARNING: for some stupid reason som.scene.cpp
									//scales this according to the sky animation rate
									//(1/40) instead of the real rate 
									mat.data[7] = vb[i].uvs[0];									
									mat.data[8] = 1-vb[i].uvs[1];
								}
							}

							mat.mode|=c[i].blendmode<<8; //2021

							break;					
						}

						int cp = p-pp+1; if(cp>1&&j<(int)t.count)
						{
							if(t.count==1)
							{
								//UNFINISHED
								#ifdef NDEBUG
								//when I rewrote this to be usable in som_db
								//I didn't understand I was removing a system
								//I'd setup for MapComp that assigns materials
								//to the same textures by treating single count
								//files as redirects. the file name redirects to
								//the name in the file
								int todolist[SOMEX_VNUMBER<=0x1020704UL];
								#endif 

								/*this code would detect a redirect... for now
								//use the MDO file's name to leave the redirect 
								//path open
								for(i=0;i<cp;i++)
								if(toupper(pp[i])!=toupper(fd.cFileName[i]))
								{
									if('.'==pp[i-1])
									i = cp; 
									break;
								}
								if(i!=cp)*/
								{
									auto ext = PathFindExtensionW(fd.cFileName);
									cmp->resize(cmp_s+(ext-fd.cFileName));
									for(auto i=cmp->size();i-->cmp_s;)
									{
										char &c = (*cmp)[i] = 0x7f&fd.cFileName[i-cmp_s];
								//		if(uc) c = (char)toupper(c);
									}
									(*o)[*cmp] = mat; //2024 									
								}
								//if(i!=cp) goto redirect;
							}
							else //redirect: 
							{
								auto ext = PathFindExtensionA(pp);
								cmp->resize(cmp_s+(ext-pp));
								for(auto i=cmp->size();i-->cmp_s;)
								{
									char &c = (*cmp)[i] = pp[i-cmp_s];
								//	if(uc) c = (char)toupper(c);
								}								
								(*o)[*cmp] = mat;
							}
						}
						else assert(0); j++; pp = p+1;

						if(!*pp||(char*)&m-pp<=3) break; //4B aligned?
					}
					goto unmap;
				}			

			}while(FindNextFile(ff,&fd)); FindClose(ff);

		}while(FindNextFile(gg,&fdir)); FindClose(gg);
	}

	if(!o->empty())
	(void*&)SOM::material_textures = new SOM::MT*[1024](); //C++

	goto find;
}
static som_MHM** __cdecl som_MPX_401500(LONG n) //REMOVE ME
{
	//HACK: defined in som_MHM.cpp
	//typedef DWORD som_MHM[10];
	//add a dummy MHM onto the end so NPCs will enter
	//layers where there is no tile on the base layer
	SOM::Struct<10> *dummy;
	SOM::Game.malloc_401500(dummy,1);
	memset(dummy,0x00,sizeof(*dummy));
	som_MHM **o; n/=sizeof(void*);
	SOM::Game.malloc_401500(o,n+1);
	o[n] = (som_MHM*)dummy;
	//the pointer isn't yet assigned. som_MPX_411a20
	//must perform this procedure
	//(caller would overwrite it anyway)
	//som_MPX &mpx = *SOM::L.mpx->pointer;
	//mpx[SOM::MPX::mhm_counter] = n+1;
	return o;
}

extern char *som_MDL_skin;
extern BYTE *som_MDL_skin2;
extern DWORD som_TXR_4485e0(char*,int,int,int);
static void som_MPX_chardata(BYTE *prf, WORD(*sfx)[2], WORD(*snd)[2], bool ul)
{
	for(int k=0;k<32;k++) for(int pass=1;pass<=2;pass++)
	{
		auto sxx = pass==1?sfx:snd;
		if(int j=sxx[k][0])
		for(auto*d=(swordofmoonlight_prf_data_t*)(prf+sxx[k][1]);j-->0;d++)
		{
			if(pass==1) ((BYTE(__cdecl*)(WORD))(ul?0x42e7e0:0x42e5c0))(d->effect);
			if(pass==2) ul?som_MPX_43f4f0(d->effect):som_MPX_43f420(d->effect,0);
		}
	}
}
static size_t som_MPX_chardata(char *prf, WORD(*sfx)[2], WORD(*snd)[2], std::bitset<5376> &loading)
{
	size_t load = 0;
	for(int k=0;k<32;k++) for(int pass=0;pass<=1024;pass+=1024)
	{
		auto sxx = pass==0?sfx:snd;
		if(int j=sxx[k][0])
		for(auto*d=(swordofmoonlight_prf_data_t*)(prf+sxx[k][1]);j-->0;d++)
		{
			size_t e = (WORD)d->effect; if(e>=1024) continue;

			auto b = loading[pass+e]; if(!b){ b = true; load++; }
		}
	}
	return load;
}
static BYTE som_MPX_load_event_target(DWORD f, void *p, DWORD &sz)
{
	DWORD cmp = sz, ret = ((BYTE(__cdecl*)(void*))f)(p);
	
	if(sz==cmp+1) return 1; assert(!ret==(sz==sz));

	*(WORD*)((BYTE*)p+(f==0x42a5f0?0:8)) = 0xffff; //TODO: fill in?

	((BYTE(__cdecl*)(void*))f)(p); return 0; //just clobber it?
}
static BYTE __cdecl som_MPX_428950(void *_1) //load npc instance
{
	auto rec = (swordofmoonlight_mpx_enemies_t::_item*)_1;
	if(rec->kind==0xffff)
	return ((BYTE(__cdecl*)(void*))0x428950)(_1); //dummy?

	auto prm = SOM::L.NPC_prm_file+rec->kind;
	auto pr2 = SOM::L.NPC_pr2_data[*prm->us];	
	auto &mf = SOM::L.NPC_mdl_files[*prm->us];
	auto prf = (swordofmoonlight_prf_npc_t*)(pr2+62);

	char skin_buf[64];
	
	if(prf->skin&&*prf->skinTXR) if(!mf)
	{
		prf->skin = 0;

		som_MDL_skin2 = &prf->skin; //Moratheia?
		
		som_MDL_skin = skin_buf;

		//SOMEX_(A)"npc\\texture\\%s"
		sprintf(som_MDL_skin,"%s%s",(char*)0x4c2030,prf->skinTXR);
	}
	//BYTE ret = ((BYTE(__cdecl*)(void*))0x428950)(_1);
	BYTE ret = som_MPX_load_event_target(0x428950,_1,SOM::L.ai2_size);
	{
		if(som_MDL_skin)
		{
			prf->skin = 1; 

			som_MDL_skin = 0; som_MDL_skin2 = 0;
		}
	}
	if(!ret) return 0;

	auto &ai = SOM::L.ai2[SOM::L.ai2_size-1];
	auto &mdl = *(SOM::MDL*)ai[SOM::AI::mdl2];		
	mdl.ext.subtract_base_control_point = true;

	//2024: only base layer is present :(
	//YUCK: what is not setting this to 0 upstream???
	assert(ai.uc[0]==0);
	ai.uc[0] = 0;
	
	if(mdl.ext.kage=(som_MDL*)ai[SOM::AI::kage_mdl2])
	{
		if(auto*e=mdl.ext.kage->ext.mdo_elements)
		{
			e->texture = mdl->ext.kage; //TEMPORARY
			e->kage = 1;
			e->ai = ~(SOM::L.ai2_size-1);
			if(mdl->ext.kage) for(int i=3;i-->0;)
			{		
				if(prf->shadow)	mdl.ext.kage->scale[i]/=prf->shadow; //zero divide
			}
		}
	}

	mdl.xyzuvw[3] = rec->xyzuvw[3];
	mdl.xyzuvw[5] = rec->xyzuvw[5];
		
	return 1;
}
//REMINDER: 405de0 applies skins to the 
//shared model, so a map can't have more
//than 1 version (skin) of skinned models
static void *som_MPX_singular_skin(char *a, int npc) //preload skin?
{
	DWORD path;
	BYTE *skin; if(npc<1024)
	{
		auto pr2 = SOM::L.NPC_pr2_data[npc];
		auto prf = (swordofmoonlight_prf_npc_t*)(pr2+62);
		skin = &prf->skin;
		path = 0x4c2030; //SOMEX_(A)"npc\\texture\\%s";
	}
	else
	{
		auto pr2 = SOM::L.enemy_pr2_data[npc-1024];
		auto prf = (swordofmoonlight_prf_enemy_t*)(pr2+62);
		skin = &prf->skin;
		path = 0x4c0990; //SOMEX_(A)"enemy\\texture\\%s";
	}
	if(!skin[0]||!skin[1]) return 0;

	char skin_buf[64];

	skin[0] = 0;
	som_MDL_skin2 = skin; //Moratheia?
	som_MDL_skin = skin_buf;	
	sprintf(som_MDL_skin,"%s%s",(char*)path,skin+1);
	extern SOM::MDL::data *som_MDL_440030(char*);
	auto *l = som_MDL_440030(a);
	if(l&&skin[0]) //Moratheia?
	{
		DWORD txr = som_TXR_4485e0(som_MDL_skin,0,0,16);
		if(txr!=0xFFFFffff) som_MPX_441990(l,txr);
	}
	skin[0] = 1; 
	som_MDL_skin = 0; som_MDL_skin2 = 0;	
	return l;
}
static BYTE __cdecl som_MPX_405de0(void *_1) //load enemy instance
{
	auto rec = (swordofmoonlight_mpx_enemies_t::_item*)_1;
	if(rec->kind==0xffff)
	return ((BYTE(__cdecl*)(void*))0x405de0)(_1); //dummy?

	auto prm = SOM::L.enemy_prm_file+rec->kind;
	auto pr2 = SOM::L.enemy_pr2_data[*prm->us];	
	auto &mf = SOM::L.enemy_mdl_files[*prm->us];
	auto prf = (swordofmoonlight_prf_enemy_t*)(pr2+62);

	char skin_buf[64]; 
	
	if(prf->skin&&*prf->skinTXR) if(!mf)
	{
		prf->skin = 0; 

		som_MDL_skin2 = &prf->skin; //Moratheia?
		
		som_MDL_skin = skin_buf;

		//SOMEX_(A)"enemy\\texture\\%s"
		sprintf(som_MDL_skin,"%s%s",(char*)0x4c0990,prf->skinTXR);
	}
	//BYTE ret = ((BYTE(__cdecl*)(void*))0x405de0)(_1);
	BYTE ret = som_MPX_load_event_target(0x405de0,_1,SOM::L.ai_size);
	{
		if(som_MDL_skin)
		{
			prf->skin = 1; 

			som_MDL_skin = 0; som_MDL_skin2 = 0;
		}
	}	
	if(!ret) return 0;

	auto &ai = SOM::L.ai[SOM::L.ai_size-1];
	auto &mdl = *(SOM::MDL*)ai[SOM::AI::mdl];
	mdl.ext.subtract_base_control_point = true;

	//2024: only base layer is present :(
	//YUCK: what is not setting this to 0 upstream???
	assert(ai.uc[0]==0);
	ai.uc[0] = 0;
	
	if(mdl.ext.kage=(som_MDL*)ai[SOM::AI::kage_mdl])
	{
		if(auto*e=mdl.ext.kage->ext.mdo_elements)
		{
			e->texture = mdl->ext.kage; //TEMPORARY
			e->kage = 1;
			e->ai = SOM::L.ai_size-1;
			if(mdl->ext.kage) for(int i=3;i-->0;)
			{
				if(prf->shadow)	mdl.ext.kage->scale[i]/=prf->shadow; //zero divide
			}
		}
	}
	//40b1d0 (cube hit test?) uses the shadow
	//for some reason. it's too hard to reprogram
	//so this just overwrites it
	//NOTE: this works because 405de0 already
	//scaled SOM::AI::kage_mdl
	//TODO: might need to 2*radius (radius or diameter?)
	ai[SOM::AI::shadow] = ai[SOM::AI::radius];

	//EXPERIMENTAL
	//this was an alternative to ai[SOM::AI::shadow]+0.25f+0.1f
	//but this feataure is currently disabled since it has other
	//issues
	/*UNUSED (might want to use enemy-PC clip instead?)
	//TODO? what about indirect attacks?
	float r = 0;	
	for(int i=pr2[144];i-->0;) //direct attacks?
	if(prm->uc[344+i*24]) //used?
	{
		//NOTE: this may permit the
		//smaller to be larger so the
		//monster attacks prematurely
		r = max(r,*(float*)(pr2+272)); //larger radius
		r = max(r,*(float*)(pr2+284)); //smaller?
	}
	r*=ai[SOM::AI::scale];

	//0.1f comes from 406ab0 (no real reason I know of)
	float r2 = ai[SOM::AI::radius]+SOM::L.hitbox2+0.1f;

	//som_logic_406ab0 uses this instead of the shadow
	ai[SOM::AI::diameter] = max(r,r2);*/

	//0 in SOM_PRM produces infinity, which mostly acts
	//like 0 but it can flip on its sign
	//NOTE: 407490 checks the PRM file prior to turning
	float &t = ai[SOM::AI::turning_rate];
	//som_game_60fps_npc_etc initializes it/turning_table
	//if(prf->turning_ratio) //EXTENSION (NpcEdit parity)
	t/=prf->turning_ratio;
	if(!_finite(t)) t = 0; //KF2's slimes glitch with 0

	float test = ai[SOM::AI::radius];

	//REMOVE ME
	//this is totally implementable but for now I don't want to
	//make time to work on it
	if(mdl.ext.anim_mask&1<<3)
	{
		//REMINDER: som_MDL_animate relies on this
		int a = mdl.running_time(mdl.animation(1));
		int b = mdl.running_time(mdl.animation(3));
		if(a!=b)
		{
			assert(0); //UNIMPLEMENTED			
			mdl.ext.anim_mask&=~(1<<3); //HACK: disabling/hiding #3
		}
	}

	mdl.xyzuvw[3] = rec->xyzuvw[3];
	mdl.xyzuvw[5] = rec->xyzuvw[5];

	//2023: fix problem with attack radii
	float prm_scale = ai[SOM::AI::scale]/ai[SOM::AI::_scale];
	for(int i=3;i-->0;)
	{
		((float*)(ai.c+116+i*0x44))[11]*=prm_scale; //2023: bad attack radius
	//	((float*)(ai.c+116+i*0x44))[11]*=prm_scale; //indirect?

		((float*)(ai.c+116+i*0x44))[12]*=prm_scale; //2024: bad attack height
	}
	/*som_logic_reprogram fixes this now
	//it can't fix the above matters
	if(ai.i[138]!=0x7f7fffff&&ai.i[138]) //4060aa 
	{
		ai.f[138]*=prm_scale; //perimeter???
	}*/

	return 1;
}
static BYTE __cdecl som_MPX_42a5f0(void *_1) //load object instance
{
	auto rec = (swordofmoonlight_mpx_objects_t::_item*)_1;

	auto prm = SOM::L.obj_prm_file+rec->kind;

	//BYTE ret = ((BYTE(__cdecl*)(void*))0x42a5f0)(_1);
	BYTE ret = som_MPX_load_event_target(0x42a5f0,_1,SOM::L.ai3_size);	
	if(!ret) return 0;
	
	auto pr2 = SOM::L.obj_pr2_file[prm->us[18]].c;
	auto prf = (swordofmoonlight_prf_object_t*)(pr2+62);

	auto &ai = SOM::L.ai3[SOM::L.ai3_size-1];

	return 1;
}
static BYTE __cdecl som_MPX_40fde0(int _1, void *_2) //load item instance
{
	auto rec = (swordofmoonlight_mpx_items_t::_item*)_2;
		
	//2024: only base layer is present :(
	//YUCK: what is not setting this to 0 upstream???
	for(int i=_1;i-->0;) 
	{
		assert(rec[i]._zindex==0);
		rec[i]._zindex = 0;
	}

	BYTE ret = ((BYTE(__cdecl*)(int,void*))0x40fde0)(_1,_2);

	return 1;
}

extern void som_MPX_reset_model_speeds()
{	
	//if(!EX::INI::Bugfix()->do_fix_animation_sample_rate)
	//return;
	
	float fps = 30.0f*som_MDL::fps/DDRAW::refreshrate;

	auto scale = [fps](SOM::MDL &m, float x)
	{
		x = fps/sqrtf(x);

		if(*m->file_head&16) x*=2; //60fps?
		
		m.ext.speed2 = m.ext.speed = x;
	};

	/*
	for(int i=255;i-->0;)
	{
		auto &sr = SOM::L.SFX_refs[i];
		if(sr.ref_count)			
		for(int j=16;i-->0;) if(auto*m=sr.mdl_instances[i])
		{
			scale(*m,m->scale[0]); //???
		}
	}*/
	for(int i=SOM::L.ai_size;i-->0;)
	{
		auto &ai = SOM::L.ai[i];
		if(auto*m=(SOM::MDL*)ai[SOM::AI::mdl])
		{
			scale(*m,ai[SOM::AI::_scale]);
		}
	}
	for(int i=SOM::L.ai2_size;i-->0;)
	{
		auto &ai = SOM::L.ai2[i];
		if(auto*m=(SOM::MDL*)ai[SOM::AI::mdl2])
		{
			scale(*m,ai[SOM::AI::_scale]);
		}
	}
	for(int i=SOM::L.ai3_size;i-->0;)
	{
		auto &ai = SOM::L.ai3[i];
		if(auto*m=(SOM::MDL*)ai[SOM::AI::mdl3])
		{
			scale(*m,ai[SOM::AI::_scale3]);
		}
	}
}

extern void som_MPX_411a20_ambient2(const unsigned m, int ll=0)
{	
	stdext::hash_map<DWORD,DWORD> rgb_index;

	DWORD rgb_mask = 0xFFFFFF, rgb_shift = 0;

	EX::INI::Bitmap bm; 	
	EX::INI::bitmap *bitmap = 0;
	if(!&bm->bitmap_ambient) return;
	
	BYTE **ambient2; if(SOM::tool)
	{
		extern BYTE *SOM_MAP_overlay_ambient2;
		ambient2 = &SOM_MAP_overlay_ambient2;
		goto tool;
	}
	else for(ll=0;ll<=6;ll++)
	{
		if(SOM::game)
		{
			extern int som_hacks_shader_model;
			if(!som_hacks_shader_model) return;

			auto &map = (*som_MPX_swap::maps)[m];

			auto &mpx = *map.wip;
			typedef SOM::MPX::Layer L;
			L &base = *(L*)&mpx[SOM::MPX::layer_0];	

			L &l = *(&base-ll); if(!l.tiles) continue;

			ambient2 = &map.ambient2[ll]; tool:;
		}

		//FIX ME: should ll correspond to the SOM_MAP
		//layer number?!
		float index = bm->bitmap_ambient(m,ll);

		if(&bm->bitmap_ambient_mask)
		{
			rgb_mask = 0xFFFFFF&(int)
			bm->bitmap_ambient_mask(m,ll,index);
			rgb_shift = 0;
			while(rgb_shift<24&&~rgb_mask&1<<rgb_shift) 
			rgb_shift++;
			rgb_mask>>=rgb_shift;
		}

		int i = EX::isNaN(index)?-1:(int)index;

		if(!bitmap||i!=bitmap->index)
		{
			auto it = std::find(bm->bitmaps.begin(),bm->bitmaps.end(),i);
			bitmap = it==bm->bitmaps.end()?0:const_cast<EX::INI::bitmap*>(&*it);
			if(bitmap)
			if(!bitmap->pixels)
			if(HBITMAP hbm=som_MPX_bitmap(*bitmap,0))
			{
				DeleteObject(hbm);
			}
			else bitmap = 0;
		}

		//2022: this used to a global, but now each
		//map has its own set of ambient bitmaps. I
		//don't know if everything here makes sense
		//in this different scenario, but it should
		//work either way
		//BYTE* &all = SOM::ambient2[ll];
		BYTE* &all = *ambient2, *del = all; 
		bool owned = false;
		if(all)
		for(size_t i=bm->bitmaps.size();i-->0;)				
		if(all==bm->bitmaps[i].pixels)
		{
			owned = true; break;
		}

		if(!bitmap||100!=bitmap->width||100!=bitmap->height)
		{
			all = 0;
		}
		else if(!&bm->bitmap_ambient_saturation)
		{
			all = (BYTE*)bitmap->pixels;
		}
		else
		{
			if(!all||owned&&!SOM::tool)
			all = new BYTE[3*100*100+1]; //+1 for *(DWORD*) cast
			
			const BYTE *rgb = bitmap->pixels;
			BYTE *rgb2 = all;
			for(int k=100*100;k-->0;rgb+=3,rgb2+=3)
			{
				DWORD j = *(DWORD*)rgb>>rgb_shift&rgb_mask;
				auto ins = rgb_index.insert(std::make_pair(j,0));
				if(ins.second)
				ins.first->second = bm->bitmap_ambient_saturation(j);
				*(DWORD*)rgb2 = ins.first->second;
			}
		}
				
		if(!owned&&del!=all) delete[] del;

		if(SOM::tool) break;
	}
}
static bool som_MPX_411a20_ltd(const unsigned m) //load mpx
{
	//2022: this code began as a layer loading 
	//system, but I'm extending it to load the
	//base as well to break free of the global
	//MPX pointer
	char a[64]; //som_db uses data\map here
	int dd = sprintf(a,"data\\map\\%02d.mpx",m)-6;
	//REMINDER: need to ensure MPY and MPX files
	//are sourced from the same folder
	wchar_t w[MAX_PATH];
	/*I guess the som_game_CreateFile code has to
	//be ripped out anyway since this code isn't
	//changing the game's map
	FILE *f = fopen(a,"rb");*/
	FILE *f = EX::data(a+5,w)?_wfopen(w,L"rb"):0;
	if(!f) return false;

	auto &map = (*som_MPX_swap::maps)[m];

	assert(m<64&&!map.mem&&!map.wip);

	//NOTE: 411a20 writes the file name (without extension)
	//starting at byte 1, but this will cut into the layers
	//if it goes over 23 bytes
	auto &mpx = *(map.wip=new som_MPX());
	mpx.c[0] = (BYTE)m;
	mpx.c[1] = a[dd+0];
	mpx.c[2] = a[dd+1];

	//WORD plug = 0xFFFF&mhm_counter++;	
	WORD plug;

	//NOTE: SOM_MAP.cpp uses A so at least this
	//string will be pooled?
	//NOTE: this is should match 448fd0's index
	//NOTE: I was using SOMEX_(A) here but it's
	//the naked path that gets first crack with
	//som_db.exe, so historically that's what's
	//been used
//	enum{ tdir_s=17 };
//	const char(*tdir)[tdir_s+1] = &"data\\map\\texture\\";
	enum{ tdir_s=15 };
	const char(*tdir)[tdir_s+1] = &"data\\map\\model\\";

	typedef SOM::MPX::Layer L;
	L &base = *(L*)&mpx[SOM::MPX::layer_0];		

	//TODO: WHAT ABOUT GAPS IN THE LAYERS?!
	//bitmap_ambient suggests its arguments
	//match SOM_MAP. MapComp could ouptut a
	//0x0 map in the MPY file?
	int ll,error = 0;	
	for(ll=0;ll<=6;ll++)
	{	
		if(ll==1) //MPY?
		{
			fclose(f);

			w[wcslen(w)-1] = 'y';
			f = _wfopen(w,L"rb");

			if(!f) break;

			plug = 0xFFFF&mpx[SOM::MPX::mhm_counter]++;
		}
		
		DWORD flags, layer_selector;

		if(ll) //MPY?
		{
			//DELICATE
			//feof requires fread... and fseek
			//seeks behind EOF, so therefor it
			//is probably a better idea to get
			//the size of the file and test it
			//instead!
			fread(&flags,4,1,f);
			if(feof(f)) break;
			//fseek(f,280-4-4,SEEK_CUR);
			char view[280-4-4];
			fread(view,sizeof(view),1,f);

			//2022: this was memory that held
			//the layer count... if it's zero
			//then it can be used to select a
			//different layer than ll in case
			//there's holes in the MPY groups
			fread(&layer_selector,4,1,f);

			assert(!layer_selector); //0 or 1?

			error = ll;
		}
		else //MPX? //2022
		{			
			//error = ll; //errors if !ll

			//this is broken up into several
			//read operations, but is really
			//contiguous
			DWORD *p = mpx.ui+SOM::MPX::flags; //77
			fread(p,4+32+32+96,1,f);
			flags = *p;
			
			//2020: saw an \n on the end of the title?
			{
				char *p = &mpx[SOM::MPX::title];
				for(int i=strlen(p);i-->0;) switch(p[i])
				{
				case '\r': case '\n': p[i] = '\0'; break;
				default: i = 0;
				}
			}

			//various states not stored in the
			//MPX global for some reason
			//
			// caller must pass these to 403ac0
			// 4115f0, 4113f0 and 411460
			//
			fread(&map._fov,24+48,1,f); 

			p = mpx.ui+0x78; //400

			fread(p,4*5,1,f); //starting point?

			(float&)p[4]*=M_PI/180; //SOM::MPX::starturn

			//this system can't set up instances
			//since those go into global buffers
			DWORD c;
			if(fread(&c,4,1,f)) if(c<=512)
			{
				map.objects.resize(c);
				if(c)
				fread(map.objects.data(),68*c,1,f);
			}
			else break; //error
			if(fread(&c,4,1,f)) if(c<=4096) //SOM_MAP_reprogram?
			{
				map.enemies.resize(c);
				if(c)
				fread(map.enemies.data(),52*c,1,f);
			}
			else break; //error
			if(fread(&c,4,1,f)) if(c<=128)
			{
				map.npcs.resize(c);
				if(c)
				fread(map.npcs.data(),52*c,1,f);
			}
			else break; //error
			if(fread(&c,4,1,f)) if(c<=256)
			{
				map.items.resize(c);
				if(c)
				fread(map.items.data(),40*c,1,f);
			}
			else break; //error

			p = mpx.ui+SOM::MPX::sky_index; //sky if nonzero

			fread(p,4,1,f); //411a20 allocates the model here

			fread(&layer_selector,4,1,f); //ignored layer count
		}		

		//TODO? THIS BLOCK GUARDS malloc_401500
		//CALLS IN CASE OF ERRORS, AND THE ERROR
		//CHECK IS ALSO DETERMINING THE NUMBER OF
		//LAYERS. THE PLAYER SHOULD JUST TRUST THE
		//THE mpy FILES ONCE ITS FEATURE STABILIZES

		L &l = *(&base-ll); assert(!l.width);
		{
			//NOTE: prior to this is a layer counter
			//that som_db ignores
			fread(&l.width,8,1,f);
			if(100*100!=l.width*l.height) //!!
			break;
			SOM::Game.malloc_401500(l.tiles,100*100);	
			if(!fread(l.tiles,12*100*100,1,f))
			break;
			DWORD *p = (DWORD*)l.tiles+4*100*100;
			DWORD *q = (DWORD*)l.tiles+3*100*100;
			L::tile *r = base.tiles+100*100;
			for(int i=0;i<100*100;i++)
			{
				//insert pointer member
				p-=4; q-=3; r--;
				
				auto t = (L::tile*)p; 

				struct mpx_tile_t //2024
				{
					DWORD mm; float e;

					//MapComp_reprogram reallocates these somewhat
					unsigned rot:2,box:4,ev:8,xxx:1,hit:1,icon:8,msb:8;
				};
				//REMINDER: doing this as a union (t&tt) caused
				//erroneous pit traps to appear on pinned tiles
				//(I don't understand why)
				auto tt = *(mpx_tile_t*)q;

				p[3] = 0; p[2] = q[2]; p[1] = q[1]; p[0] = q[0];
				if(*p==0xFFffFFff) continue;

				if(isupper(tt.ev))
				{
					t->nobsp = 1;
					t->ev = 'e'==tolower(tt.ev);
				}
				else t->ev = tt.ev&1; //2023

				t->hit = tt.hit; t->xxx = tt.xxx; //2024

				//EXPERIMENTAL
				//HACK: this is to coax NPCs into entering the 
				//layer, since the AI routines know only about
				//the base layer			
				if(r->mhm==0xFFFF) if(ll) //layer hack?
				{
					//00415576 66 8B 06             mov         ax,word ptr [esi]  
					//00415579 66 3D FF FF          cmp         ax,0FFFFh  
					//0041557D 0F 84 9C 00 00 00    je          0041561F 
					r->mhm = plug; 
					//r->msm = plug; //helpful?
					//what is MSB? it's not set by the MPX file
					//when is it set? and why isn't this unset?
					r->msb = 0x80;	//REQUIRED
					//r->unknown2 = ~r->unknown2;
					/*2020: this is setting the checkpoint bit
					r->nonzero = 16; //REQUIRED*/
					//
					// WARNING: this may be counterproductive since
					// som_logic_4079d0_42a0c0 began taking the radius
					// into account, but for now it's ignoring ffff
					//
					r->ev = 1; //2020
				}

				//this can change if MapComp is changed
				assert(*p>>16==(*p&0xFFFF));
				//when is the MSB set? what does it do?
				assert(p[2]>>31==0); 
			}
		}

		if(1&flags) //BSP data?
		{
			//00412420 E8 EE D6 03 00       call        0044FB13
			if(!fread(&l.count1,4,1,f))
			break;
			//00412435 E8 C6 F0 FE FF       call        00401500
			SOM::Game.malloc_401500(l.pointer1,l.count1);
			//loop on count1?
			//0041249E 3B 51 0C             cmp         edx,dword ptr [ecx+0Ch]  
			//004124A1 0F 8D 6F 01 00 00    jge         00412616
			for(L::struct1 *p=l.pointer1,*d=p+l.count1;p<d;p++)
			{
				//4->16 (4124D6) //unknown1
				fread(&p->unknown1,4,1,f);
				//4->0 (4124F0) //xy1~
				//4->4 (41250D)
				//4->8 (41252A)
				//4->12 (412547) //~xy2
				fread(&p->xy1,4*4,1,f);
				//4->20	(412564) //unknown2_count
				if(!fread(&p->unknown2_count,4,1,f))
				goto error;
				//00412579 E8 82 EF FE FF       call        00401500				
				SOM::Game.malloc_401500(p->unknown2_pointer,p->unknown2_count);
				/*WHY LOOP ON THIS?
				//looping on 24 ... exit is 412611 (412616 on outer loop)
				//004125BD 3B 42 14             cmp         eax,dword ptr [edx+14h]  
				//004125C0 7D 4F                jge         00412611  
				for(int i=0,iN=p->unknown2_count;i<iN;i++)
				{
					//2 levels of direction (no optimization)
					//4->0 (4125E0)
					//4->4 (412607)
				}*/
				fread(p->unknown2_pointer,8,p->unknown2_count,f);

				p->pointer_into_pointer4 = 0; //0-fill
			}

			//0041262B E8 E3 D4 03 00       call        0044FB13
			if(!fread(&l.count2,4,1,f))
			break;
			//00412640 E8 BB EE FE FF       call        00401500
			SOM::Game.malloc_401500(l.pointer2,l.count2);
			/*WHY LOOP ON THIS?
			//loop on count2? exit is 41280D?
			//0041268C EB 0F                jmp         0041269D  
			//0041268E 8B 85 CC FC FF FF    mov         eax,dword ptr [ebp-334h]  
			//00412694 83 C0 01             add         eax,1 
			for(L::struct2 *p=l.pointer2,*d=p+l.count2;p<d;p++)
			{
				//4->0 (4126DE)
				//4->4 (4126FB)
				//4->8 (412718)
				//4->12 (412735)
				//4->16 (412752)
				//4->20 (41276F)
				//4->24 (41278C)
				//4->28 (4127A9)
				//4->32 (4127C6)
				//4->36 (4127E3)
				//4->40 (412800)
			}*/
			fread(l.pointer2,44,l.count2,f);

			//00412822 E8 EC D2 03 00       call        0044FB13
			if(!fread(&l.count3,4,1,f))
			break;
			//00412837 E8 C4 EC FE FF       call        00401500
			SOM::Game.malloc_401500(l.pointer3,l.count3);
			/*SAME
			//loop on count3? exit is 412973?
			//00412883 EB 0F                jmp         00412894  
			//00412885 8B 85 CC FC FF FF    mov         eax,dword ptr [ebp-334h]  
			//0041288B 83 C0 01             add         eax,1 
			for(L::struct3 *p=l.pointer3,*d=p+l.count3;p<d;p++)
			{
				//4->0 (4128D5)
				//4->4 (4128F2)
				//4->8 (41290F)
				//4->12 (41292C)
				//4->16 (412949)
				//4->20 (412966)
			}*/
			fread(l.pointer3,l.count3,24,f);

			//SIGNED DIVISION BY 8?
			//I don't understand this code
			//so it's reproduced just to be
			//safe. I think the 64-bit parts
			//are the compiler being paranoid
			//(cdq promotes to a signed QWORD)
			/*DWORD stride4 = l.count1;
			__asm
			{
				push eax
				push edx
				mov eax,stride4
				add         eax,7	//00412979 8B 40 0C				
				//this should have no effect, probably
				//a side effect of using a signed type
			//	cdq					//0041297F 99
			//	and         edx,7	//00412980 83 E2 07
			//	add         eax,edx	//00412983 03 C2
				//divide by 8
				sar         eax,3	//00412985 C1 F8 03  
				mov stride4,eax
				pop edx
				pop eax
			}
			//I think +7 is DWORD alignment
			assert(stride4==l.count1+7>>3);*/
			DWORD stride4 = (l.count1+7)/8; //IOW
			DWORD count4 = stride4*l.count1;			
	//		assert(count4%4==0); //is MPX word aligned? //seems not?
			SOM::Game.malloc_401500(l.pointer4,count4);
			
				//PEEKING???
				//[ebp-330h] is not referenced anywhere else (not via
				//EBP)
				//THIS CODE IS PEEKING AT A BYTE
				//THE BYTE IN QUESTION IS ABOUT TO BE READ, AND SO IT
				//MAKES NO SENSE TO PEEK AT IT!!
				//where is [ebp-330h] used?
				//why not just read it from l.pointer4??? 
			//004129C7 8D 85 D0 FC FF FF    lea         eax,[ebp-330h]
				//BYTE ebp_330h;
				//fread(&ebp_330h,1,1,f);
			//004129E1 E8 6D D3 03 00       call        0044FD53				
				//fseek(f,-1,SEEK_CUR);

			//004129FC 0F AF 48 0C          imul        ecx,dword ptr [eax+0Ch]  
			//00412A00 51                   push        ecx
			//00412A0D E8 01 D1 03 00       call        0044FB13 
			fread(l.pointer4,count4,1,f);
			//filling out pointer_into_pointer4
			//00412A1F EB 0F                jmp         00412A30  
			//00412A21 8B 8D CC FC FF FF    mov         ecx,dword ptr [ebp-334h]  
			//00412A27 83 C1 01             add         ecx,1  			
			for(DWORD i=0;i<l.count1;i++) 
			{
				l.pointer1[i].pointer_into_pointer4 = l.pointer4+i*stride4; 
			}
		}

		if(ll) //MPY?
		{
			//TODO: alignment textures?
			
			//texture/vbuffer containers
			//fseek(f,8,SEEK_CUR); 
			DWORD oo[2]; fread(oo,8,1,f);
			if(oo[0]||oo[1]){ assert(0); break; }
		}
		else //MPX? //2022
		{		
			//TEXTURES
			DWORD c,i = 0;
			if(fread(&c,4,1,f)) if(c) if(c<=1024)
			{			
				auto &tp = (WORD*&)mpx[SOM::MPX::texture_pointer];
				SOM::Game.malloc_401500(tp,c);
				memset(tp,0xFF,2*c);

				map.textures.resize(c);
				
				auto *tq = map.textures.data();
				//these need to be 0 filled in case
				//they're passed to som_MPX_64::map 
				memset(tq,0x00,c*sizeof(*tq));
				while(i<c)
				{
					//NOTE: 411a20 reads these one char
					//at a time (this is an improvement)
					char buf[512];
					char *pp = buf;
					int rem = fread(pp,1,sizeof(buf),f);
					char *p,*d = pp+rem;
					for(;i<c;i++,tq++)
					{
						for(p=pp;p<d;p++) if(!*p)
						{
							if(p-pp<31&&*pp)
							{
								char *q = tq->cmp;
								memcpy(q,tdir,tdir_s);
								memcpy(q+=tdir_s,pp,p-pp+1); //!
								
								//NOTE: I'm avoiding PathFindExtension
								//because I expect it allocates/deletes
								//a bunch of memory for this simple case
								//(401410 does so and should be replaced)
								char *qq = q+(p-pp);
								while(qq>=q&&*qq!='.')
								qq--;
								if(qq>=q)
								memcpy(qq,".txr",5);
								else strcat(q,".txr");
							}

							rem-=p-pp+1;

							pp = p+1; break;
						}
						if(pp<=p) break;
					}

					fseek(f,-rem,SEEK_CUR);
				}
				//remove dummy alignment textures
				while(c&&!*map.textures.back().cmp)
				{
					c--; map.textures.pop_back();
				}
				//caller must call 4485e0 on these
				mpx[SOM::MPX::texture_counter] = c;
			}
			else break; //error

			//VBUFFER
			if(fread(&c,4,1,f)) if(c)
			{
				mpx[SOM::MPX::vertex_counter] = c;
				auto &vp = (float*&)mpx[SOM::MPX::vertex_pointer];
				SOM::Game.malloc_401500(vp,c*5);
				if(!fread(vp,20*c,1,f)) break; //error
			}
		}

		//NOTE: this is the MSM data, but there's no
		//counter besides the existence of the tiles
		DWORD t,tN = mpx[SOM::MPX::texture_counter];
		for(L::tile *p=l.tiles,*d=p+100*100;p<d;p++)
		{
			WORD m = p->msm; if(m==0xFFFF) continue;
			
		  //this code reproduces 418230 since it's
		  //easier than using som_db's FILE family

			//0041824C 6A 04                push        4
			fread(&t,4,1,f); 
			assert(t<=tN); //MapComp may break these into pieces??
			if(t>tN||!t) goto error;

			//00418236 6A 08                push        8  
			//00418238 E8 C3 92 FE FF       call        00401500  
			SOM::Game.malloc_401500(p->pointer);			
			//00418261 E8 9A 92 FE FF       call        00401500  
			SOM::Game.malloc_401500(p->pointer->pointer,t);
			p->pointer->counter = t;
			L::tile::scenery::per_texture *q = p->pointer->pointer;
			WORD test = (WORD)0xffff;
			do if(!fread(q,6,1,f)||q->texture>=tN&&q->texture!=0xFFFF) 
			{
				//2022: this is temporary until I can fix MapComp's
				//handling of dummy alignmentt textures in MSM files
				//since it seems ot emit empty data per texture with 
				//or without triangles
				if(q->vcolor_indicesN) goto error;

				assert(!q->triangle_indicesN); //fix me!

				//2022: MapComp_4092f0/409570 should prevent this
				//now, but it can't hurt to leave this sanitizing
				//code in place
				fix_me: 
				
				//I keep hitting this when building an MHM map???
				//assert(q->triangle_indicesN);

				//hack: hide unitialized data from unloader routine
				q--; p->pointer->counter--;
			}
			else
			{	
				#ifdef _DEBUG
				//trying to work out if MapComp breaks apart pieces
				//to fit into the player's vertex buffer
				if(test!=q->texture)
				{
					if(!q->triangle_indicesN) goto fix_me; //2022

					assert(test!=q->texture);
				}
				test = q->texture;
				#endif
				if(!q->triangle_indicesN) goto fix_me; //2022

				//TODO: this can hold some transparency information
				//maybe blend modes can be placed in a final vcolor
				//entry that is not referenced
				//
				//q->zero1 = 0; //these routines always 0-fill at top
				q->_transparent_indicesN = 0;

				//MPX stores these backward
				std::swap(q->vcolor_indicesN,q->triangle_indicesN);
				//004182DB E8 20 92 FE FF       call        00401500
				SOM::Game.malloc_401500(q->triangle_indices,q->triangle_indicesN);
				//004182FF E8 FC 91 FE FF       call        00401500
				SOM::Game.malloc_401500(q->vcolor_indices,q->vcolor_indicesN);

				fread(q->triangle_indices,q->triangle_indicesN*2,1,f);
				fread(q->vcolor_indices,q->vcolor_indicesN*8,1,f);

				//2022: it seems MapComp may not restrict chunks to
				//the size of som_db.exe's buffer
				#ifdef NDEBUG
				int todolist[SOMEX_VNUMBER<=0x1020704UL];
				#endif
				//Moratheia's first map exceeds both
				//assert(q->triangle_indicesN<=2688);
				//assert(q->vcolor_indicesN<=896);
				if(q->vcolor_indicesN>896)
				{
					auto *p = q->triangle_indices;
					int i,j,n = q->triangle_indicesN;
					for(i=0,j=0;i<n;i+=3)
					{
						if(p[i]>=896||p[i+1]>=896||p[i+2]>=896)
						continue;

						p[j] = p[i]; p[j+1] = p[i+1]; p[j+2] = p[i+2];

						if((j+=3)>=2688) break;
					}
					q->triangle_indicesN = j; q->vcolor_indicesN = 896;
				}
				else q->triangle_indicesN = min(2688,q->triangle_indicesN);
				
			}while(q++,--t);
		}

		if(ll) //MPY?
		{
			DWORD oo[2]; fread(oo,4,1,f); //MHM container
			if(*oo){ assert(0); break; } 
		}
		else //MPX? //2022
		{
			//MHM
			DWORD c;
			if(fread(&c,4,1,f)) if(c)
			{
				char _buf[4096]; 
				char *buf = _buf; DWORD bsz = 4096;

				mpx[SOM::MPX::mhm_counter] = c;
				auto &mp = (som_MHM**&)mpx[SOM::MPX::mhm_pointer];
				//YUCK: this just adds a dummy MHM
				//in case there are extra layers
				//of course it used to reprogrammed
				//(and probably still is even though
				//411a20 is no longer called)
				//SOM::Game.malloc_401500(mp,c);
				mp = som_MPX_401500(c*sizeof(void*)); //plug?
				for(auto*p=mp;c-->0;p++)
				{
					DWORD sz; fread(&sz,4,1,f);
					
					if(sz>bsz)
					{
						if(buf!=_buf) delete[] buf;

						bsz = (sz/1024+1)*1024;

						buf = new char[bsz];
					}					
					fread(buf,sz,1,f);

					//WARNING: for this to work the two
					//fread calls need to be rewritten
					//with SomEx.dll's fread... it would
					//be a good idea to rewrite 417630
					//because it uses a temporary buffer
					//that's new allocated and deleted
					//each time
					//*p = ((som_MHM*(__cdecl*)(FILE*))0x417630)(f);
					extern som_MHM *som_MHM_417630(char*,DWORD);

					*p = som_MHM_417630(buf,sz);
				}

				if(buf!=_buf) delete[] buf;
			}
		}

		if(!ferror(f)) error = 0;
		else error: break;
	}

	if(f) fclose(f); f = 0;

	if(error||!ll) evt_error:
	{	
		//2022: at a minimum be sure players 
		//aren't playing a partial game without
		//realizing it (don't ruin their save file)
		
		//not really som_db's style?
		#ifdef NDEBUG
		//#error log. MessageBox? 
		int todolist[SOMEX_VNUMBER<=0x1020704UL];
		#endif 

		assert(0); return false; //2022
	}
	else if(!map.reload_evt(m)) //EVT?
	{
		goto evt_error;
	}
		
	map.mem = map.wip; //2022 

	//MDO material system? //Volume //Bitmap?

	som_MPX_411a20_ambient2(m); //REFACTOR

	/*2022: this strategy won't work unless the 
	//textures are first loaded into memory
	//(that's a likely scenario but conceptually
	//this subroutine doesn't work that way)
	SOM::VT **vt = *SOM::volume_textures;
	SOM::MT **mt = *SOM::material_textures; //2021
	WORD *tp = (WORD*)mpx[SOM::MPX::texture_pointer];*/
	const int tc = map.textures.size();
	std::vector<BYTE> vt;
	std::vector<SOM::MT*> mt;
	SOM::MT::lookup(""); //HACK: set SOM::material_textures
	if(SOM::material_textures) 
	for(int i=tc;i-->0;)
	if(SOM::MT*p=SOM::MT::lookup(map.textures[i].cmp+tdir_s))
	{
		if(mt.empty()) mt.resize(tc); mt[i] = p;
	}
	auto &vv = EX::INI::Volume()->textures;
	//if(SOM::volume_textures)
	if(!vv.empty()) for(int i=tc;i-->0;)
	{
		//char *fn = PathFindFileNameA(p);
		char *p = map.textures[i].cmp; 			
		if(!*p) continue;
		char *fn = p+tdir_s;
		//int len = PathFindExtensionA(fn)-fn;
		p = strrchr(fn,'.');
		int len = p?p-fn:strlen(fn);		
		for(size_t j=vv.size();j-->0;)		
		if(vv[j].match(fn,len))
		{
			if(vt.empty()) vt.resize(tc);

			vt[i] = true; break;
		}
	}
	for(ll=0;ll<=6;ll++)
	{
		L &l = *(&base-ll); if(!l.tiles) continue;

		//REFACTOR
		//code here was moved to som_MPX_411a20_ambient2
		//for saving/restoring som_MPX_swap::mpx memory
		BYTE *rgb = map.ambient2[ll];
		
		for(L::tile *p=l.tiles,*d=p+100*100;p<d;p++)
		{
			float bc[3],bc2[3]; if(rgb) for(int i=3;i-->0;)
			{
				bc[i] = *rgb++/255.0f;	
				bc2[i] = (1-bc[i])*0.25f;
			}			
			if(!p->pointer) continue;
		
			L::tile::scenery::per_texture *q = p->pointer->pointer;
			for(int i=p->pointer->counter;i-->0;q++)
			{
				if(q->vcolor_indices[0][1]>>24) //transparent?
				{
			vt:		//q->zero1 = q->triangle_indicesN;
					q->_transparent_indicesN = q->triangle_indicesN;
					q->triangle_indicesN = 0;					
				}
				else //HACK: special textures?
				{
					int txr = q->texture; if(txr<tc)
					{
					//	txr = tp[txr];					

						//if(vt&&vt[txr]) mt: //hack
						if(!vt.empty()&&vt[txr]) mt: //hack
						{
							//mark for transparency processing
							for(int k=q->vcolor_indicesN;k-->0;)
							{
								//assuming default desired to be opaque
								q->vcolor_indices[k][1]|=0xFF000000;
							}
							goto vt;
						}
						else if(!mt.empty()&&mt[txr]) //2021: additive mode?
						{
							if(mt[txr]->mode&0x100) goto mt;
						}
					}
				}

				if(rgb) for(int k=q->vcolor_indicesN;k-->0;)
				{						
					BYTE *vc = (BYTE*)&q->vcolor_indices[k][1];

					//there needs to be a way to speficy what kind
					//of algorithm to use... this needs to be tested
					//without KFII's unusual gamut
					#ifdef NDEBUG					
					//#error I almost forgot about this??? (2022)
					int todolist[SOMEX_VNUMBER<=0x1020704UL];
					#endif
					for(int i=3;i-->0;) if(1) //KF2: less contrast?
					{
						//I can't even remember coming up with this
						//formula one day later
						//is it the best?						
						//vc[i]*=bc[i]+(1-vc[i]/255.0f)*(1-bc[i]);
						float c = vc[i]/255.0f;
					//	c = 0.5f+(c-0.5f)*bc[i]; //lerp(0.5f,c,t);
						//c = 0.5f+(c-0.5f)*powf(bc[i],1+c);
					//	vc[i] = bc[i]*c*255;

						//DUPLICATE
						//should match som_shader_classic_vs
						//this is the simplest approach but it requires 
						//different bitmap values and doesn't reduce the
						//contrast for dark polygons
						//vc[i]*=powf(bc[i],c*4);
						//this is a straightforward contrast reduction plus
						//a little darkening that seems like a good match
						//for KF2... I don't know how if it makes sense to
						//do this to regular SOM shading
						vc[i] = 255*(0.5f+(c-0.5f)*bc[i]-bc2[i]);
					}
					else vc[i]*=bc[i];
				}
			}				
		}
	}

	return true;
}
bool som_MPX_swap::mpx::reload_evt(int m)
{
	if(evt) SOM::Game.free_401580(evt);

	//portable events?
	//2022: som_tool_CreateFile had initialized sys.ezt
	//however it did so on top of the first evt file it
	//saw. that just happened to be okay because som_db
	//must use a naked data\path for maps and evt files
	extern HANDLE som_zentai_ezt; if(!som_zentai_ezt)
	{
		//2022: hooking into som_game_CreateFile's data
		//folders traversal
		HANDLE ezt = CreateFileA("data\\map\\sys.ezt",GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_EXISTING,0,0);
		if(ezt!=INVALID_HANDLE_VALUE)
		SOM::zentai_init(ezt);
	}
	//memcpy(a+dd+3,"evt",4);
	char a[64]; //som_db uses data\map here
	sprintf(a,"data\\map\\%02d.evt",m);
	HANDLE h = CreateFileA(a,SOM_GAME_READ);
	if(h==INVALID_HANDLE_VALUE)
	{
		assert(0); return false; //goto evt_error; //2022
	}
	DWORD rd,sz = GetFileSize(h,0);
	if(sz!=0&&sz!=INVALID_FILE_SIZE)
	{
		BYTE *buf;
		SOM::Game.malloc_401500(buf,sz);

		ReadFile(h,buf,sz,&rd,0);

		if(sz!=rd) //som.zentai.cpp?
		{
			assert(0);
			SOM::Game.free_401580(buf);
		}
		else evt = buf;
	}
	CloseHandle(h); return true;
}
static som_MPX *som_MPX_413190_ptr = 0;
static void som_MPX_413190_ltd(som_MPX &mpx) //unload
{
	//2022: this is a variation on 413190 that 
	//works on any MPX object

	typedef SOM::MPX::Layer L;
	L &base = *(L*)&mpx[SOM::MPX::layer_0];

	//REMINDER: som_MPX_job_2 use this function
	mpx[SOM::MPX::sky_model] = 0;
	mpx[SOM::MPX::sky_model2] = 0;

	som_MPX_413190_ptr = &mpx;
	((void(__cdecl*)())0x413190)(); //base layer

	//OBSOLETE
	//don't flush save state for layers?
	//*(BYTE*)0x4c0ca2 = 0;

	//zero the static/shared part of the MPX data
	memset((DWORD*)&mpx+66+10,0x00,(132-66-10)*4);

	for(int ll=1;ll<=6;ll++) //extra layers?
	{	
		L &l = (&base)[-ll]; if(!l.tiles) continue;

		//copy the layer over the base's
		//memory that 0x413190 will free
		base = l;
		//SOM::L.mpx->pointer = &mpx;
		som_MPX_413190_ptr = &mpx;
		((void(__cdecl*)())0x413190)();
	}			

	//som_MPX_reprogram knocks this out so "&mpx" isn't
	//freed
	//004133C7 E8 B4 E1 FE FF       call        00401580
	/*2022: caller owns mpx
	SOM::Game.free_401580(&mpx);*/

	assert(!som_MPX_413190_ptr);
}
static void som_MPX_prepare_for_42dd40_in_critical_section()
{
	//42dd40 draws the fade in (and out) effect, but it has
	//numerous problems that this code seeks to address. I
	//moved the code into this routine so it completes inside
	//the critical-section in order to ensure it doesn't come
	//into conflict with the second loading thread

	//something is needed to not throw off the first frame
	//of the fade event since texture updates will slow down
	//the frame (there's actually an initial frame that
	//measures ticks between the message pump and BGM setup
	//that could also be delayed)

	//calling these here should take the load off 42dd40
	//so the first flash frame doesn't incure lag, and also
	//this helps ensure the BGM doesn't skip
	((void(__cdecl*)())0x401fe0)(); //msg pump?
	((BYTE(__cdecl*)(DWORD))0x43f660)(0); //BGM? //sounds ok

	//TESTING
	//this strategy is too CPU intensive to not
	//risk degrading the frame rate (computing
	//2 CPU frames in one) so instead textures
	//in play need to be computed by hand here
	int todolist[SOMEX_VNUMBER<=0x1020704UL];
	//HACK: this prevents drawing to speed up load times
	//I don't know if the overhead is worth it thereafter
	//som_scene_update_texture is used to update textures
	extern bool som_scene_44d810_disable;
	som_scene_44d810_disable = true;
	{
		//this computes tile visibility... it will be done
		//again when this subroutine exits, however it has
		//to be recomputed if the map has layers since the
		//layer selection replaces the base layer selection
		((void(__cdecl*)())0x402c10)();
		((void(__cdecl*)())0x4023c0)(); //step world (spawn)
		extern void som_CS_commit();
		som_CS_commit();
		((void(__cdecl*)())0x402c00)(); //draw world (zero fade?)
		//SOM::frame++;
	}
	som_scene_44d810_disable = false;

	//same logic as above... assuming some time has passed
	//since the above calls, leading into 42dd40 when this
	//subroutine exits
	((void(__cdecl*)())0x401fe0)(); //msg pump?
	((BYTE(__cdecl*)(DWORD))0x43f660)(0); //BGM? //sounds ok
}
static void som_MPX_push_back_other_job(int g, int i=0)
{
	if(1==EX::central_processing_units) return;

	som_MPX_swap::job j = {0,3,(unsigned)g,(unsigned)i};
	som_MPX_swap::jobs->push_back(j);
}
extern void som_MPX_push_back_transfer_job(int,int);
extern void som_MPX_push_back_textures_job()
{
//	if(!DDRAW::doMultiThreaded) return; //REMOVE ME?

	if(DDRAW::compat) //update textures?
	{
		som_MPX_push_back_other_job(1);
	}
}
void SOM::queue_up_game_map(int m)
{
	assert(m>=0&&m<64);
	//if(1) return; //TESTING 
	som_MPX_push_back_transfer_job(m,1);
	som_MPX_push_back_textures_job();
}

static void __cdecl som_MPX_448560_release_textures()
{
	//TODO: would like to cancel som_MPX_job_3
	//but it will just have to run to the end

	EX::section raii(som_MPX_thread->cs);

//	som_MPX_textures->clear(); //som_MPX_atlas2?

	((void(__cdecl*)())0x448560)();

	som_MPX_swap::kage_release(); //2024: I guess?
}
static void __cdecl som_MPX_448400_rebuild_textures()
{
	EX::section raii(som_MPX_thread->cs);

	((void(__cdecl*)())0x448400)();

	som_MPX_push_back_textures_job();
	
	som_MPX_swap::kage_release(); //2024
}

namespace DDRAW{ extern bool inStereo; }
static const float som_MPX_center_u_amount = -M_PI/36/2; //2.5;
static void __cdecl som_MPX_center_u()
{
	SOM::L.pcstate[3] = DDRAW::inStereo?0:som_MPX_center_u_amount;
}

extern void som_MPX_job_2(som_MPX_swap::job,bool);
static void som_MPX_411a20_evt(const unsigned m) //load evt
{	
	auto &map = (*som_MPX_swap::maps)[m];

	memset(SOM::L.leafnums,0x00,1024);
	memset(SOM::L.evt_trip_zone_bits,0x00,128); //555dc4

	SOM::L.evt_file = map.evt;
	SOM::L.events = (som_EVT*)(map.evt+4);
	SOM::L.events_size = *(DWORD*)map.evt;
	//DUPLICATE som_game_ReadFile
	if(BYTE*e=SOM::L.events->uc)
	for(int i=0,n=SOM::L.events_size;i<n;i++,e+=252) switch(e[31])
	{
	case 0xFE: //System
	default: SOM::ezt[i] = false; break; 
	case 0xFD: //Systemwide
	case 0xFC: //conflicted (shouldn't be seen but)
		if(i<10) e[31] = 0xFE; //System
		SOM::ezt[i] = true; break;
	}
}
static BYTE __cdecl som_MPX_411a20(char *dd) //load
{	
	typedef SOM::MPX::Layer L;

	auto &dst = *SOM::L.corridor;

	if(!SOM::field) if(dd!=(char*)0x4C0B9C) //EXPERIMENTAL
	{	
		//trying black fade in effect at top of game
		//after intros. since intros use black fades
		//it seems to me black might be more natural
		//as opposed to a white flash. it feels like
		//opening your eyes to a world for the first
		//time, but it doesn't quite feel right with
		//the existing timing. maybe if it were sped
		//up it would be worth making this an option

	//	dst.fade[1] = 0; //401ae1 (401ae7) sets this to 2
	}

	//2021: if SOM_SYS doesn't set the map SOM_EDIT tries 
	//to load ".mpx" ... could consult the SOM file here?
	if(!*dd)
	{
		//NOTE: maybe the mystery SOM_SYS mode is just for
		//previewing title screens, but in this case the
		//player has chosen to start a game, so I guess
		//this makes sense

		//TODO: provide SOM_SYS hint message box and scan
		//for first available MPX
		assert(dd==(char*)0x4c0b9c);
		dd = (char*)EX::need_ansi_equivalent(932,Sompaste->get(L".MAP"));
		if(!*dd) dd = (char*)0x4c0b9c;
		if(!*dd) dd[0] = dd[1] = '0';
	}			
	int m = atoi(dd); m = max(0,min(63,m));

	int mm = SOM::mpx, m2 = dst.map_event?mm:m;

	if(2==dst.nosetting) //2023: som_files_wrote_db?
	{
		assert(m==mm);

		som_MPX_swap::job j = {(unsigned)m};

		j.f = 2; som_MPX_job_2(j,true); //unload?
	}

	auto &map = (*som_MPX_swap::maps)[m];
	auto &map2 = (*som_MPX_swap::maps)[m2];
	auto &mmap = (*som_MPX_swap::maps)[mm];

	auto *mem = som_MPX_swap::mem; 

	//2022: take control of resources?
	map.load_canceled = true;
	EX::section raii(som_MPX_thread->cs);	

	DWORD was = EX::tick();

	DWORD cmp_ai_size = SOM::L.ai_size;

	//PROFILING
	//unfortunately eliminating disk access isn't enough... for a real
	//project it seems like the only source of multi-millisecond times
	//is new/delete, or possibly initializng large arrays in instances
	//this will need to stay up so it can be used to confirm solutions
	#ifndef NDEBUG
	DWORD was2[30];
	#define _(n) was2[n] = EX::tick()-was; assert(n<EX_ARRAYSIZEOF(was2));
	#else
	#define _(...)
	#endif

	auto **debug = &SOM::L.mpx->pointer; //REMOVE ME

	if(SOM::L.mpx->pointer) //2022: unload? //413190
	{
		som_MPX &mpx = *SOM::L.mpx->pointer;

		//som_MPX_reprogram knocks these out so they're
		//done just for the first layer
	//	((void(__cdecl*)())0x43f8d0)(); //BGM (might want to fade?)
		//trying to skip this (som_MPX_reprogram had disabled this)
		//FUN_004275e0_unload_PC_arm_equipment(0,0);    
		((void(__cdecl*)())0x403fa0)(); //money				
		//((void(__cdecl*)())0x42e970)(); //sfx (moved from below)
		{
			//don't dump all the SFX references (just live records)
			//NOTE: 42e970 wipes entire buffers
			//2022: these counters seem unreliable, but I don't know
			//how flames are surviving transfer
			DWORD &sfx_models = SOM::L.SFX_models_size;
			DWORD &sfx_images = SOM::L.SFX_images_size;
_(0)		memset(SOM::L.SFX_images,0x00,144*512); //sfx_images
			sfx_images = 0;
			memset(SOM::L.SFX_models,0x00,4*512); //sfx_models
			memset(SOM::L.SFX_instances,0x00,504*512); //sfx_models
			sfx_models = 0;
		}

		som_MPX_new = mem&&!mem->empty()?mem:0;
		{
			//unload models (and instances)
			som_MPX_loading_shadows = true;
	_(3)   	((void(__cdecl*)())0x428e60)(); //npcs
	   		((void(__cdecl*)())0x406530)(); //enemies
			som_MPX_loading_shadows = false;
	_(4)   	((void(__cdecl*)())0x42aac0)(); //objects
	_(5)   	((void(__cdecl*)())0x410230)(); //items
		}	
		som_MPX_new = 0; if(mem) mem->clear();

		/*I forgot som_MPX_413190_ltd does this 
		{
			//NOTE: these will have to be blended eventually
			//so it may be a second instance is needed or to
			//keep the other map's sky alive
			((void(__cdecl*)(void*))0x445ad0)
			((void*)mpx[SOM::MPX::sky_model]); //sky instance
			mpx[SOM::MPX::sky_model] = 0;
			extern void __cdecl som_MDO_445870(void*);
			som_MDO_445870((void*)mpx[SOM::MPX::sky_model2]); //MDO file
			mpx[SOM::MPX::sky_model2] = 0;			
		}*/		
			//don't let 413190 delete skies and free
			//the instance data because the materials
			//indices are stale
			mpx[SOM::MPX::sky_model] = 0;
			mpx[SOM::MPX::sky_model2] = 0;
_(6)	//	for(size_t i=som_MPX_skies.size();i-->0;)
		//	som_MPX_skies[i].free_instance();

		//((void(__cdecl*)())0x42e970)(); //sfx (moved above)
_(7)	((void(__cdecl*)())0x4111b0)(); //lamps		
		if(dst.map_event) //save state? //0x4c0ca2
		{
			//dst.map_event = 0; //UNNECESSARY (was for layers)

			((void(__cdecl*)())0x42d300)(); //flush save state

			//I think this was in som_game_CreateFile
			for(int i=0;i<1024;i++) if(SOM::ezt[i])
			{
				for(int j=0;j<64;j++)
				SOM::L.mapsdata_events[j].c[i] = SOM::L.leafnums[i];
			}
		}		

		//NOTE: there was code here for deleting the
		//current MPX but it seems som_MPX_job_2 has
		//responsibility for the data structures now		
		if(1==EX::central_processing_units)
		{
			//HACK: just reuse code for obscure case
			som_MPX_swap::job j = {(unsigned)mm,2};
			som_MPX_job_2(j,false);
		}		
		else mmap.last_tick = was;
_(8) 
		/*2023: som_MPX_411a20_evt fuses these
		//2022: 413190 actually keeps these in memory
		//until 411a20 replaces them
		{
			SOM::L.evt_file = 0;
			SOM::L.events = 0;
			SOM::L.events_size = 0; 
			memset(SOM::L.leafnums,0x00,1024);
			memset(SOM::L.evt_trip_zone_bits,0x00,128); //555dc4
		}*/
	}
	else
	{
		//som_MDL_447fc0_cpp is relying on SOM::field to 
		//prevent merging arm.mdl into a single MDO mesh
		//so its pieces can be hidden individually. that
		//might not be best for SOM::field because other
		//uses are dealing with menu stuff
		extern void som_game_field_init(); assert(!SOM::field);
		som_game_field_init();
	}
	//formerly in some_game_ReadFile //EXPLAIN?
	{
		//BUG-FIX
		//NOTE: the timing of this is delicate. doing in here
		//seems to work
		//normally this is already done, but in some cases it
		//is never done. it'd be worthwhile to figure out why
		//2024: why 64? (save games may only want the top 64)
		//SOM::Item(&items)[64] = SOM::L.items;
		SOM::Item(&items)[256] = SOM::L.items;
		//2024: seems more prudent looking at som_scene_items
		//for(int i=64;i-->0;) items[i].nonempty = 0;
		for(int i=256;i-->0;) *(DWORD*)&items[i] = 0; 
	}
_(9)
	BYTE ret = 1; if(!map.mem)
	{
		if(!map.wip) //load map?
		{
			ret = som_MPX_411a20_ltd(m);
		}
		else ret = 0;
	}
_(10)
	if(!ret) swap_ret:
	{
		//kill BGM held open by extension above?
		if(!ret) ((void(__cdecl*)())0x43f8d0)();

		//images_free and sounds_free must always
		//run to do cleanup
		//if(1||!EX::debug)
		if(1==EX::central_processing_units)
		{
			som_MPX_swap::models_free();
			som_MPX_swap::images_free();
			som_MPX_swap::sounds_free();
		}
_(28)		
		//this prevents dropping frames on map transfers
		//this is supporting the som_MPX_program edit at 
		//402152
		dst.dbsetting = 0; //0x4c0ca1

		//this is normally zeroed just before the map is
		//loaded, but som_MPX_reprogram edits 402CB3 out
		//to wait until after (so it can be queried) and
		//so zero it
		dst.lock = 0; //0x4C0B98

		if(ret&&dst.fade[1]<=16) //42dd40() //EXPEDIANT HACK
		{
			//NOTE: there's a glitch when there's no fade out
			som_MPX_prepare_for_42dd40_in_critical_section();
		}

_(29)	was = EX::tick()-was; //DEBUGGING

		//HACK: just sprinkling around
	//	extern void som_bsp_ext_clear();
	//	som_bsp_ext_clear();

		assert(ret==1); return ret; 
	}

	if(!map.load||map.loaded<map.load)	
	{
		map.load_canceled = false;
		map.load_models_etc();
	//	map.load_models_atlas();
		som_MPX_push_back_textures_job();
		mem = som_MPX_swap::mem;
	}
	//assert(mem); //may be 0 if map is just tiles
	
	//finish work of 411a20 to set global state?
	{
		if(!SOM::L.mpx->pointer)
		SOM::Game.malloc_401500(SOM::L.mpx->pointer);

		som_MPX &mpx = *SOM::L.mpx->pointer;
		memcpy(&mpx,map.mem,sizeof(mpx));
_(13)		
		//TODO: all of these will have to be blended
		//somehow in order to smooth map transitions
		//som_game_ReadFile()
		//{
			//43:draw distance //44:fog start (as percentage of 43)
			//SOM subtracts 2 from the draw distance for the fogend
		//	SOM::fog[1] = map.zfar-2;
		//	SOM::fog[0] = map.fog*SOM::fog[1]; 							
			//2020: som_mocap_am_reshape needs this tightened
			assert(map.znear==0.1f);
			//SOM::fov[2] = map.znear = 0.01f;
			float &map_znear = SOM::fov[2] = 0.01f; //...
			SOM::fov[3] = map.zfar;
			/*VERBATIM (som_game_ReadFile)
			//NEW: mark fog color for clear
			//*((DWORD*)to+46) = 0xFFFFFF; //ambient lighting
			SOM::clear = *((DWORD*)to+45);
			*((DWORD*)to+45) = 0xFFFFFF; //fog color
			*/
			//REMOVE ME? (ANCIENT CODE)
			//som_hacks_Clear picks up on this and
			//som_hacks_Begin is highly dependent on
			//it (it was a source of a glitch because
			//for some reason it detected two Begins
			//for the same frame... which is not good)
			//2022: setting high 8 bits to distinguish
			//between white backgrounds
			SOM::clear = 0xFF000000|map.bg; enum{ map_bg=0xffFFFFFF }; //...
		//}
		((DWORD(__cdecl*)(float,float,float))0x403ac0)(0.8726646f,map_znear,map.zfar);
_(14)	((BYTE(__cdecl*)(float,DWORD))0x4115f0)(map.fog,map_bg);
_(15)	((BYTE(__cdecl*)(DWORD))0x4113f0)(map.ambient);
		for(int i=0;i<3;i++)
		{
			auto &l = map.lights[i];
			((BYTE(__cdecl*)(int,DWORD,float[3]))0x411460)(i,(DWORD&)l.col,l.dir);
		}
		if(SOM::ambient2&&!map.ambient2) //disable?
		{			
	   		float r[4] = {1,1,1,1}; 
			DDRAW::vset9(r,1,DDRAW::vsGlobalAmbient+1);
		}
		if(SOM::ambient2=map.ambient2) //EXTENSION
		{
			extern DWORD som_scene_ambient2; //enable?
			som_scene_ambient2 = 1;
		}
	
	//can't allocate materials on som_MPX_new 
	if(size_t mats=map.instance_mat_estimate)
	{
		while(mats>SOM::L.materials_capacity)
		{
			assert(!som_MPX_new);
			if(!((BYTE(__cdecl*)())0x448290)()) 
			{
				assert(0); break; //65535?
			}
		}
	}
	
	if(mem){ assert(mem->empty()); mem->clear(); }
	som_MPX_new = mem;

_(16)	for(int i=0,n=map.objects.size();i<n;i++)
		{
			auto &o = map.objects[i];
			som_MPX_42a5f0(&o);
			if(1==o.light_up_objects) //OPTIMIZING?
			if(o.kind<=1024) //lamp?
			{
				//THERE NEEDS TO BE AN EASIER WAY!!
				auto prm = SOM::L.obj_prm_file+o.kind;
				auto pr2 = ((swordofmoonlight_prm_object_t*)prm)->profile;
				if(pr2<1024)
				{
					auto prf = SOM::L.obj_pr2_file[pr2].c+62;
					if(0xa==((swordofmoonlight_prf_object_t*)prf)->operation)
					{
						//if(0!=o.light_up_objects)
						((void(__cdecl*)(int))0x411050)(i); 
					}
				}
			}
		}
_(17)	for(int i=0,n=map.npcs.size();i<n;i++)
		{
			som_MPX_428950(&map.npcs[i]);
		}		
		extern void som_game_ai_resize(DWORD);
		{
			DWORD n = map.enemies.size();			
			DWORD nn = mmap.enemies.size();
			DWORD sz = cmp_ai_size;

			//2022: I'm trying to avoid reallocation
			//in case it causes map transfer hiccups
			//however it doesn't really help where a
			//larger map comes in. still this avoids
			//having to always back-fill the dummies
			if(0) som_game_ai_resize(sz=n);
			else if(n>sz)
			som_game_ai_resize(sz=max(256,n*3/2));
			
			int _ = 0;
			for(DWORD i=0;i<n;i++)
			{
				som_MPX_405de0(&map.enemies[i]);
				_ = SOM::L.ai_size;
			}
			assert(_==n); n = _; //mdl failure?

			if(n<nn||cmp_ai_size<sz)
			{
				som_MPX_swap::npc dummy = {};
				dummy.kind = 0xffff;
				_ = SOM::L.ai_size;
				if(som_MPX_405de0(&dummy))
				{
					_ = SOM::L.ai_size;
					som_Enemy *ai = SOM::L.ai;
					auto &cp = ai[n++];
					SOM::L.ai_size = sz;
					while(n<nn) memcpy(ai+n++,&cp,sizeof(cp));
					//NOTE: nn/cmp may both be zero
					n = max(n,max(nn,cmp_ai_size));
					while(n<sz) memcpy(ai+n++,&cp,sizeof(cp));
				}
			}		
		}
_(18)	if(!map.items.empty())
		{
			//((BYTE(__cdecl*)(DWORD,void*))0x40fde0)(map.items.size(),map.items.data());
			som_MPX_40fde0(map.items.size(),map.items.data());
		}	
_(19)
		som_MPX_reset_model_speeds(); //2023

	som_MPX_new = 0; //// BACK TO operator new[] ////

		int tc = mpx[SOM::MPX::texture_counter];
		int tp = mpx[SOM::MPX::texture_pointer];
		for(int i=0;i<tc;i++)
		{
			char *a = map.textures[i].cmp;
			if(*a=='\0') continue;

			//NOTE: this is finishing up after som_MPX_job_1 
			WORD &t = ((WORD*)tp)[i];
			if(t==0xffff)
			{
				t = (WORD)som_TXR_4485e0(a,0,0,0);
			}
		}
//		map.load_mpx_atlas((WORD*)tp,(DWORD)tc); //2023
		
_(20)
		/*//som_MPX_411a20_evt
		SOM::L.evt_file = map.evt;
		SOM::L.events = (som_EVT*)(map.evt+4);
		SOM::L.events_size = *(DWORD*)map.evt;
		//DUPLICATE som_game_ReadFile
		if(BYTE*e=SOM::L.events->uc)
		for(int i=0,n=SOM::L.events_size;i<n;i++,e+=252) switch(e[31])
		{
		case 0xFE: //System
		default: SOM::ezt[i] = false; break; 
		case 0xFD: //Systemwide
		case 0xFC: //conflicted (shouldn't be seen but)
			if(i<10) e[31] = 0xFE; //System
			SOM::ezt[i] = true; break;
		}*/som_MPX_411a20_evt(m);

		//((void(__cdecl*)())0x42cf40)(); //save state
		{
			extern bool som_game_42cf40(); som_game_42cf40();
		}
		//((void(__cdecl*)())0x427170)(); //equip		
_(21)	if(!dst.map_event) //2022
		{
			extern void som_game_equip(); som_game_equip();
		}
_(22)
		float p[6]; if(!dst.nosetting)
		{
			p[0] = 2*dst.setting[0];
			p[2] = 2*dst.setting[1];
			int xz = 100*dst.setting[1]+dst.setting[0];
			int zi = dst.zindex; //EXTENSION			
			L &l = ((L*)&mpx[SOM::MPX::layer_0])[-zi];
			p[1] = zi<=6&&l.tiles?l.tiles[xz].elevation:0;
			for(int i=3;i-->0;) if(dst.settingmask&(2<<i))
			{
				p[i]+=dst.offsetting[i];
			}
			if(dst.settingmask&1) for(int i=3;i-->0;)
			{
				p[3+i] = dst.heading[i];
			}
			else for(int i=6;i-->3;)
			{
				p[i] = SOM::L.pcstate[i];
			}
		}
		else if(2==dst.nosetting) //som_files_wrote_db  
		{
			memcpy(p,SOM::L.pcstate,6*sizeof(*p));
		}
		else
		{
			memcpy(p,mpx.f+121,3*sizeof(*p));
			//EXPERIMENTAL
			//2020: start the game at -5 degrees, I think
			//most people naturally look slightly downward
			//and it can feel like you're looking up if you
			//look at an even plane
			//EXTENSION? might want to recenter to this level
			//but would it be bad or good for VR?
			//
			// WAS IN som_state_pov_vector_from_xyz3_and_pov3
			//
			//som_MPX_center_u
			//p[3] = -M_PI/36/2; //2.5;
			p[3] = DDRAW::inStereo?0:som_MPX_center_u_amount;
			p[4] = mpx.f[124];
			p[5] = 0;
		}

		#ifdef NDEBUG
		//#error what are the exact criteria?		
		//TODO: SOM::warp should cancel sounds
		//if taking the 0== path below
		int todolist[SOMEX_VNUMBER<=0x1020704UL];
		#endif
		if(0==(dst.settingmask&16))
		{
			SOM::pcdown = SOM::frame;
			SOM::warp(p,SOM::L.pcstate);
			memcpy(SOM::L.pcstate,p,sizeof(p));
			memcpy(SOM::L.pcstate2,p,sizeof(p));
		}
		else SOM::warp(p); //2022

		//EXTENSION
		//overwriting starting point for blending
		memcpy(mpx.f+121,SOM::xyz,3*sizeof(*p));
		if(0!=(dst.settingmask&16))
		{
			//HACK: this if for blending assuming
			//the starting point isn't going to be
			//used for any other purpose
			mpx.f[121]-=EX::Affects[1].position[0];
			mpx.f[123]-=EX::Affects[1].position[2];
		}

		//this is resuming BGM from the save file
		//but not from the time where it left off
		char *bgm; if(-1!=*SOM::L.mapsdata[m].c)
		{
			bgm = SOM::L.mapsdata[m].c+1;
		}
		else bgm = mpx.c+344;

		//DirectSound crackles for some reason
		//when there's no BGM so a dummy has to
		//be generated
		extern DWORD som_game_44c4b0_fade;
		extern void som_game_silence_wav(char*);
		som_game_silence_wav(bgm);					
_(23)
		//EXTENSION?
		//trying to avoid reloading the BGM file
		//might be nice to fuse it anyway right?
		char *bgm2 = SOM::L.bgm_file_name;
		if(!dst.map_event||stricmp(bgm,bgm2))
		{
			//2022: fade out BGM on seamless transfer?
			if(SOM::L.bgm_mode==1&&16&dst.settingmask)
			{
				assert(dst.map_event);

				if(*bgm2&&memcmp(bgm2,".silence.wav",13))
				{
					//NOTE: bgm2 is MAX_PATH long but only the
					//file name is copied
					memcpy(bgm2,bgm,32);

					if(!som_game_44c4b0_fade)
					som_game_44c4b0_fade = 1;

					bgm = 0; 
				}
			}
			if(bgm) SOM::bgm(bgm,1);
		}
	}
_(24)
	////formerly som_tool_CreateFile////
	// 
	//2022: all the logic run on MPX load is moved here
	//NOTE: there's some more setup below that could be
	//refactored to not have to wait until after 411A20
	extern void som_game_on_map_change(int);
	som_game_on_map_change(m);
_(25)
	////formerly som_tool_ReadFile////

	som_MPX &mpx = *SOM::L.mpx->pointer;

	assert(SOM::newmap||SOM::frame);

	SOM::newmap = SOM::frame; 

	bool blend = false;
	if(16&dst.settingmask)
	for(int i=2;i-->0;) switch(dst.fade[i])
	{
	case 17: case 18: blend = true; break;
	}
	if(blend)
	{
		SOM::skyswap = SOM::mpx2_blend!=0;

		SOM::mpx2_blend = 1-SOM::mpx2_blend;
	}
	else SOM::mpx2_blend = 0;
	SOM::mpx2_blend_start = 1-SOM::mpx2_blend;
	SOM::mpx2 = (BYTE)m2;
	SOM::mpx = (BYTE)m;
	int sky = mpx[SOM::MPX::sky_index];
	{
		//HACK: force sky_model_identifier to load
		//NOTE: this is hypothetical until the sky
		//models can be preloaded to avoid hiccups
		map._sky = map.sky = sky;
		som_MPX_once_per_frame_sky();
		map._sky = map.sky;
		map.sky_instance();
	}
	map._sky = sky;
	SOM::sky = map.sky!=0;
	if(m!=SOM::mpx2&&dst.fade[1]==18) //blend sky?
	{
		if(SOM::sky)
		{
			//disable if sky is identical?
			//NOTE: this simplifies the job of the
			//frame by frame blending code
			if(map.sky==map2.sky)
			{
				dst.fade[1] = 0xff;
			}
		}
		else if(SOM::mpx2_blend)
		{
			SOM::sky = map2.sky!=0;
		}
	}

_(26)
	//som_MPX &mpx = *SOM::L.mpx->pointer;
	
		//WARNING
		//some elements sanitation code is moved
		//to som_game_42cf40 so it kicks in prior
		//to loading the save data
	
	//NOTE: this is for ambient2 data but other 
	//things too
	extern void som_MHM_y_init(); som_MHM_y_init();
_(27)	
	#undef _

	goto swap_ret; //return ret;
}

static void som_MPX_once_per_frame_sky()
{
	EX::INI::Adjust ad;

	auto &f = ad->sky_model_identifier;
	if(!&f) return; 
	auto &d = SOM::mpx_defs(SOM::mpx);
	float id = *f((float)SOM::mpx,d._sky);
	d.sky = EX::isNaN(id)?d._sky:(int)id;
}
extern void som_MPX_once_per_frame() //som.game.cpp
{
	som_MPX_once_per_frame_sky();

	  //////// unload maps? //////////
	
	//avoid busy work? see som_MPX_job_2
	//has notes on why maps need to wait
	//for this to unload
	if(!som_MPX_textures->empty()) return;

	if(1!=EX::central_processing_units)
	{
		DWORD now = EX::tick();

		int m = SOM::mpx, mm = SOM::mpx2;

		for(int i=64;i-->0;) if(i!=m&&i!=mm)
		{
			auto &cmp = (*som_MPX_swap::maps)[i];

			//maybe use the play clock for this?
			int todolist[SOMEX_VNUMBER<=0x1020704UL];
			if(cmp.wip)
			if(now-cmp.last_tick>1000*60*3) //3 minutes?
			{
				cmp.load_canceled = true;

				som_MPX_swap::job j = {(unsigned)i,2}; //unload map
				som_MPX_swap::jobs->push_back(j);
			}
		}
	}
}

static void som_MPX_job_1(som_MPX_swap::job j) //load
{
	int m = j.map;

	//CRITICAL_SECTION
	//note these sections take ownership
	//of resource loading
	auto &map = (*som_MPX_swap::maps)[m];
	{
		assert(map.corridor_mask);

		EX::section raii(som_MPX_thread->cs);

		assert(map.corridor_mask);

		//HACK: load_models_etc should pick up
		//on this (critical section is needed)
		map.load_canceled = !map.mem&&!som_MPX_411a20_ltd(m);

		if(!map.load_canceled)
		{
			auto &mpx = *map.mem;
			int tc = mpx[SOM::MPX::texture_counter];
			int tp = mpx[SOM::MPX::texture_pointer];
			for(int i=0;i<tc;i++) 
			if(((WORD*)tp)[i]==0xffff)
			{
				char *a = map.textures[i].cmp;
				if(*a!='\0') goto do_textures;
			}
		}
	}	
	if(0) for(;;) do_textures: //goto
	{
		int i = 0; //CRITICAL_SECTION

		EX::section raii(som_MPX_thread->cs);

		if(map.load_canceled) break;

		auto &mpx = *map.mem;
		int tc = mpx[SOM::MPX::texture_counter];
		int tp = mpx[SOM::MPX::texture_pointer];
		for(;i<tc;i++) 
		if(((WORD*)tp)[i]==0xffff)
		{
			char *a = map.textures[i].cmp;
			if(*a=='\0') continue;

			DWORD t = som_TXR_4485e0(a,0,0,0);
			if(t==0xFFFFffff) continue;

			((WORD*)tp)[i] = (WORD)t;
			if(1<SOM::L.textures[t].ref_counter)
			continue;

			i++; break;
		}
		if(i==tc) //break;
		{
//			map.load_mpx_atlas((WORD*)tp,(DWORD)tc); //2023

			break;
		}
	}

	//NOTE: assuming highest priority //???
	map.load_sky(m);

	//NOTE: this has its own critical sections
	//that run in a loop so to be interuptable
	map.load_models_etc();

//	map.load_models_atlas(); //2023

	extern HMMIO som_MPX_mmioOpenA_BGM_open(LPSTR);
	{
		EX::section raii(som_MPX_thread->cs);

		char *bgm = &(*map.mem)[SOM::MPX::bgm];

		//HACK!!! som_files_wrote_db needs this
		if(!*bgm) strcpy(bgm,".silence.wav"); 

		map.bgmmio = som_MPX_mmioOpenA_BGM_open(bgm);
	}

		assert(map.corridor_mask);

	//HACK: load_finished?
	map.corridor_mask = 0;
}
static som_MPX_sky *som_MPX_load_sky(int i, bool sky_job=false)
{
	if(!i||i>4) return 0; //EXTENSION?

	//som_MPX_skies.resize(4);

	auto *sky = &som_MPX_skies[i-1]; 

	auto *o = sky->data; if(!o)
	{
		EX::section raii(som_MPX_thread->cs);

		char a[64]; //NOTE: 411a20 matches this, but uses "%2.2d"?
		sprintf(a,SOMEX_(A)"\\data\\map\\model\\sky%02d.mdo",i);
		extern SOM::MDO::data *__cdecl som_MDO_445660(char*);			
		o = som_MDO_445660(a);

		//WARNING: this ++ is intended to hold all skies
		//in memory in case they're swapped out... it may
		//not be the best idea since they may have very
		//large textures, but for now SOM_MAP only really
		//allows for 4 skies, and the code above enforces
		//this limit

		if(o)
		{
			o->ext.refs++; //!!!

			if(!o->chunk_count) o = 0; //Moratheia?
		}

		sky->data = o; 
		sky->loaded = o?1:-1;

		//HACK: shouldn't use loaded to track textures
		if(o&&!sky_job) sky->loaded+=o->texture_count;
	}
	return sky->loaded>0?sky:0;
}
SOM::MDO *SOM::mpx_defs_t::sky_instance()
{
	auto *p = sky?&som_MPX_skies[sky-1]:0;

	if(sky)
	if(sky!=_sky //change?
	||_sky_instance
	&&_sky_instance!=&p->instance) //default?
	{
		auto si = _sky_instance;
		
		if(si) if(!p->loaded)
		{		
			//HACK: I think pushing back more
			//than one job may be stalling the 
			//main thread on create()
			p->loaded = -2;

			som_MPX_push_back_other_job(2,sky);
		}
		else if(p->loaded!=-1) //som_MPX_job_3?
		{
			if(p->data) //-2?
			{
				int load = 1+p->data->texture_count;

				if(p->loaded>=load)
				{
					si = 0; assert(p->loaded==load);
				}
			}
		}
		else si = 0; if(si&&*si) return *si;
	}

	p = som_MPX_load_sky(sky);

	if(p) p->make_instance(); else sky = 0; //!!

	auto m = static_cast<som_MPX_swap::mpx*>(this)-*som_MPX_swap::maps;
	if(m==SOM::mpx&&SOM::L.mpx->pointer)
	{
		auto &mpx = *SOM::L.mpx->pointer;
		//mpx[SOM::MPX::sky_index] = sky;
		mpx[SOM::MPX::sky_model2] = p?(INT32)p->data:0;
		mpx[SOM::MPX::sky_model] = p?(INT32)p->instance:0;
	}

	auto si = p?&p->instance:0; 
	
	_sky_instance = si; return si?*si:0;
}
void som_MPX_swap::mpx::load_sky(int m)
{
	EX::section raii(som_MPX_thread->cs);

	if(!mem) return; //load_canceled

	sky = _sky = (*mem)[SOM::MPX::sky_index];

	//HACK: force sky_model_identifier to load?
	/*this won't work since the extension is likely
	//to need want to know things like the location
	som_MPX_once_per_frame_sky(m);*/
	
	//currently I'm just focusing on my KF2 project
	//which doesn't need to change the sky on entry
	//if(&EX::INI::sky_model_identifier)
	{
		//TEMPORARY FIX
		//if the INI file can drive the sky model
		//then there has to be a way to figure out
		//what model the map is going to use after
		//it's loaded... the Standby Map event can
		//be of use here
		#ifdef NDEBUG
		int todolist[SOMEX_VNUMBER<=0x1020704UL];
		#endif
	//	for(int i=1;i<=4;i++)
	//	som_MPX_load_sky(i);
	}
	//else
	{
		//int i = (*mem)[SOM::MPX::sky_index];
		auto *p = som_MPX_load_sky(sky);

		//originally this wasn't possible when
		//free_instance was required but it's best
		//to go ahead and get it out of the way
		if(p) p->make_instance();
	}
}
void som_MPX_swap::mpx::load_models_etc()
{
	enum //DUPLICATE
	{
		sfx_0=0,
		snd_0=1024,		
		npc_0=2048,
		enemy_0=3072,
		obj_0=4096,
		item_0=5120, //256
		total=5376,
	};
	if(load)
	{
		if(loaded==load) return;
	}
	else //CRITICAL_SECTION
	{
		EX::section raii(som_MPX_thread->cs);

		if(load_canceled) return;

		instance_mat_estimate = 0;
		instance_mem_estimate = 0;
		model_tally = new BYTE[3*1024]();

		assert(loaded==0);
		assert(loading.none());

		size_t load = 0; //SHADOWING
		
		auto *np = npcs.data();
		for(size_t k,i=npcs.size();i-->0;np++) 
		{
			k = np->kind; if(k>=1024) continue;
						
			if(np->item<250) //MUST CHECK EVERY INSTANCE
			{
				size_t kk = SOM::L.item_prm_file[np->item].us[0];

				if(auto b=loading[item_0+kk]);else{ load++; b = true; }
			}

			typedef swordofmoonlight_prm_npc_t prm_t;

			prm_t *prm = (prm_t*)&SOM::L.NPC_prm_file[k];
			
			if(prm->item<250) //THIS TOO IF k IS PROFILE
			{
				k = SOM::L.item_prm_file[prm->item].us[0];

				if(auto b=loading[item_0+k]);else{ load++; b = true; }
			}

			k = prm->profile; //items2[prm->item] = true;

			model_tally[k+npc_0-npc_0]++;

			auto b = loading[npc_0+k]; if(b) continue;

			auto pr2 = (char*)SOM::L.NPC_pr2_data[k];			

			load++; b = true;

			//out of order (really just a problem for skins)
			//loading_models.push_back(models_it(ins,20,pr2+31)); //20

			auto prf = (swordofmoonlight_prf_npc_t*)(pr2+62);

			if((k=(WORD)prf->flameSFX)<1024)
			{
				if(auto b=loading[sfx_0+k]);else{ load++; b = true; }
			}
			load+=som_MPX_chardata(pr2,prf->dataSFX,prf->dataSND,loading);
		}		
		auto *ep = enemies.data();
		for(size_t k,i=enemies.size();i-->0;ep++)
		{
			k = ep->kind; if(k>=1024) continue;
			
			if(ep->item<250) //MUST CHECK EVERY INSTANCE
			{
				size_t kk = SOM::L.item_prm_file[ep->item].us[0];

				if(auto b=loading[item_0+kk]);else{ load++; b = true; }
			}

			typedef swordofmoonlight_prm_enemy_t prm_t;

			prm_t *prm = (prm_t*)&SOM::L.enemy_prm_file[k];
			
			if(prm->item<250) //THIS TOO IF k IS PROFILE
			{
				k = SOM::L.item_prm_file[prm->item].us[0];

				if(auto b=loading[item_0+k]);else{ load++; b = true; }
			}

			k = prm->profile; //items2[prm->item] = true;
			
			model_tally[k+enemy_0-npc_0]++;

			auto b = loading[enemy_0+k]; if(b) continue;

			auto pr2 = (char*)SOM::L.enemy_pr2_data[k];
			
			load++; b = true; 
			
			//out of order (really just a problem for skins)
			//loading_models.push_back(models_it(ins,22,pr2+31)); //22

			auto prf = (swordofmoonlight_prf_enemy_t*)(pr2+62);
			
			if((k=(WORD)prf->flameSFX)<1024)
			{
				if(auto b=loading[sfx_0+k]);else{ load++; b = true; }
			}
			load+=som_MPX_chardata(pr2,prf->dataSFX,prf->dataSND,loading);
		}
		auto *op = objects.data();
		for(size_t k,i=objects.size();i-->0;op++)
		{
			k = op->kind; if(k>=1024) continue;

			k = SOM::L.obj_prm_file[k].us[18];

			auto pr2 = SOM::L.obj_pr2_file[k].c;			
			auto prf = (swordofmoonlight_prf_object_t*)(pr2+62);

			switch(prf->operation) //MUST CHECK EVERY INSTANCE
			{
			case 0x14: //box item
			case 0x15: //chest item
			case 0x16: //corpse item
			case 0x29: //receptacle item

				if(op->kept_item<250)
				{
					size_t kk = SOM::L.item_prm_file[op->kept_item].us[0]; 

					if(auto b=loading[item_0+kk]);else{ load++; b = true; }					
				}
				break;
			}

			model_tally[k+obj_0-npc_0]++;

			auto b = loading[obj_0+k]; if(b) continue;

			load++; b = true; 

			//out of order (really just a problem for skins)
			//loading_models.push_back(models_it(ins,20,pr2+31)); //20
					
			switch(prf->operation)
			{
			case 0x1e: case 0x1f: //trap sfx

				if((k=(WORD)prf->trapSFX)<1024)
				if(auto b=loading[sfx_0+k]);else{ load++; b = true; }
				break;
			}
			if((k=(WORD)prf->flameSFX)<1024&&k)
			{
				if(auto b=loading[sfx_0+k]);else{ load++; b = true; }
			}
			for(int j=3;j-->0;)
			if((k=(WORD)(&prf->loopingSND)[j])<1024)
			{
				if(auto b=loading[snd_0+k]);else{ load++; b = true; }
			}
		}
		auto *ip = items.data();
		for(size_t k,i=items.size();i-->0;ip++)
		{
			k = ip->kind; if(k>=250) continue;

			k = SOM::L.item_prm_file[k].us[0];

			auto b = loading[item_0+k]; if(b) continue;

			//auto pr2 = SOM::L.item_pr2_file[k].c;

			load++; b = true; 
						
			//out of order (really just a problem for skins)
			//loading_models.push_back(models_it(ins,21,pr2+31)); //21
		}

		this->load = load; //SHADOWING
	}

	auto *tally = model_tally;

	DWORD est = 0, kage = 0, mats = 0;

	std::pair<som_MPX_64,void*> ins;
	char(&path)[64] = ins.first.cmp;
	//memcpy(path,SOMEX_(a)"\\data\\",10); //10
	memcpy(path,"data\\",5); //10

	//THREAD-SAFE?
	auto *pos = (BYTE*)&loading; //OPTIMIZING
	void *end = (char*)pos+total;
	enum{ pos_bit=sizeof(*pos)*CHAR_BIT };

	size_t resume = 0;
	for(int bit=0;;) //CRITICAL_SECTION
	{
		EX::section raii(som_MPX_thread->cs);

		//canceling will compromise model_tally
		faster: if(loaded==load||load_canceled)
		{
			break; //return; //breakpoint
		}
		resume: if(bit%pos_bit==0&&!*pos) //!
		{
			auto *cmp = pos++; while(!*pos) pos++;

			bit+=(pos-cmp)*pos_bit; assert(pos<end);
		}
	//	while(!loading.test(bit)) bit++; 
		while((size_t)bit<loading.size()) //2024
		{
			if(!loading.test(bit)) bit++; 
			else break;
		}
		if(bit==loading.size()) //2024
		{
			//NOTE: I'm seeing this after moving
			//som_game_60fps from som_game_field_init
			//to som_game_4245e0_init_pc_arm_and_hud_etc

			assert(loaded==load-1); //impossible?

			//I feel this is indicative of an error
			loaded = load; break; 
		}

		if(resume++<loaded) goto resume;

		bool fast; if(bit<snd_0+1024) //SFX or SND?
		{
			if(bit<snd_0) //SFX
			{
				int sfx = bit-sfx_0; //equal to bit

				//WARNING: fast here is inexact since SFX will
				//also load sound files and subordinate models
				//to improve this 43f420 needs to be rewritten

				int mdl = SOM::L.SFX_dat_file[sfx].model;

				fast = mdl==255||SOM::L.SFX_refs[mdl].ref_count;

				((BYTE(__cdecl*)(WORD))(0x42e5c0))((WORD&)sfx);
			}
			else //SND
			{
				int snd = bit-snd_0;

				fast = 0!=SOM::L.SND_ref_counts[snd];

				som_MPX_43f420(snd,0);
			}
		}
		else
		{
			//WARNING: textures aren't represented. ideally it
			//should be one file per critical section but even
			//then some files are bigger than others, and then
			//there are groups of files like MDL+MDO+CP groups

			auto it = models_bit(ins,bit);
						
			union
			{
				SOM::MDL::data *mdl;
				SOM::MDO::data *mdo;
				void *cmp; 
			};

			if(!(fast=0!=it->second))
			{
				char a[64]; memcpy(a,it->first.cmp,64);
	
				if(bit<obj_0) //MDL only?
				{
					extern SOM::MDL::data *som_MDL_440030(char*);
					if(!(cmp=som_MPX_singular_skin(a,bit-npc_0)))
					som_MPX_loading_shadows = true;
					cmp = som_MDL_440030(a);
					som_MPX_loading_shadows = false;
				}
				else if(bit<obj_0+1024) //MDO or MDL?
				{
					extern void *som_MDL_401300_maybe_mdo(char*,int*);
					cmp = som_MDL_401300_maybe_mdo(a,0);
				}
				else //MDO only? //item?
				{
					extern SOM::MDO::data *som_MDO_445660(char*);
					cmp = som_MDO_445660(a);
				}
				assert(cmp==it->second);

//				models.push_back(cmp); //2023: atlas
			}
			else models_refs(cmp=it->second)++;
			
			if(cmp&&tally)
			{
				DWORD n = bit>=item_0?65:tally[bit-npc_0];					
				if(models_type(cmp))
				{
					mats+=n*mdo->material_count;
					est+=n*mdo->_instance_mem_estimate();
				}
				else
				{
					if(auto*o=mdl->ext.mdo)
					mats+=n*o->material_count;
					else
					mats+=n*3;
					est+=n*mdl->_instance_mem_estimate();
				}
				if(bit<obj_0) kage+=n;
			}
		}

		bit++; loaded++; if(fast) goto faster;
	}

	if(est)
	{
		if(auto*mdl=kage?
		(SOM::MDL::data*)
		(BYTE*)SOM::L.kage_mdl_file:0)
		{			
			if(auto*o=mdl->ext.mdo)
			mats+=kage*o->material_count;
			mats+=kage*3;
			est+=kage*mdl->_instance_mem_estimate();			
		}
		instance_mem_estimate+=est;
		instance_mat_estimate+=mats+64;

		//faster to allocate here 
		if(loaded==load)
		{
			#ifdef NDEBUG
	//		#error work to do
			#endif
			est+=est/4; //REMOVE ME

			instance_mat_estimate+=64; //REMOVE ME

			delete[] model_tally; model_tally = 0;

			est+=sizeof(som_MPX);

			if(auto&mem=som_MPX_swap::mem)
			{
				//this compactifies somewhat
				size_t cap = mem->capacity();
				if(est>cap)
				{
					est-=mem->front_capacity();
					while(est<cap/4) est*=2;
					mem->back_alloc(est);
				}
			}
			else mem = new som_MPX_mem(est);
		}	
	}
}
DWORD SOM::MDL::data::_instance_mem_estimate()
{
	DWORD m = *file_head&7;
	DWORD o = sizeof(SOM::MDL);	
	if(m&3) o+=272*skeleton_size;	
	o+=12*vertex_count; //soft animation only? 440947
	if(m&4) o+=12*vertex_count;	
	if(ext.mdo)
	o+=ext.mdo->_instance_mem_estimate()-sizeof(SOM::MDO);
	else for(auto i=element_count;i-->0;) //44d3b0 
	{
		auto &e = elements[i];
		o+=32*e.vertex_count+2*e.vindex_count; //WASTEFUL
	}
	return o+=8*element_count;
}
DWORD SOM::MDO::data::_instance_mem_estimate()
{
	return sizeof(SOM::MDO)+2*material_count+104*chunk_count;
}
extern void som_MPX_job_2(som_MPX_swap::job j, bool cancel=false) //unload
{
	int m = j.map;

	//CRITICAL_SECTION
	//note these sections take ownership
	//of resource loading
	auto &map = (*som_MPX_swap::maps)[m];
	{
		EX::section raii(som_MPX_thread->cs);

		if(!cancel)
		{
			if(1!=EX::central_processing_units)
			{
				if(SOM::mpx==m||SOM::mpx2==m) return;

				if(!map.load_finished()) return;
			}
			
			//can't do anything complicated here since the
			//menus may need to take the critical section
			//so there's nothing to but abort
			if(!som_MPX_textures->empty()) return;
		}
		else map.load_canceled = true;

		if(map.mem)
		{
			auto &mpx = *map.mem;
			int tc = mpx[SOM::MPX::texture_counter];
			int tp = mpx[SOM::MPX::texture_pointer];
			for(int i=0;i<tc;i++)
			{
				auto &t = ((WORD*)tp)[i];
				som_MPX_448780(t); t = 0xFFFF;
			}
		}
		if(map.wip) //free up MPX memory?
		{
			som_MPX_413190_ltd(*map.wip);

			if(map.evt) SOM::Game.free_401580(map.evt); 

			map.wip = map.mem = 0; map.evt = 0;
		}	

		map._sky_instance = 0; //HACK

		//NOTE: keeping in single section
		map.unload_models_etc();

		extern MMRESULT WINAPI som_MPX_mmioOpenA_BGM_close(HMMIO);
		som_MPX_mmioOpenA_BGM_close(map.bgmmio);
		map.bgmmio = 0;
	}
}
void som_MPX_swap::mpx::unload_models_etc()
{
	//som_MPX_job_2 should've called this
	EX::section raii(som_MPX_thread->cs);

	enum //DUPLICATE
	{
		sfx_0=0,
		snd_0=1024,		
		npc_0=2048,
		enemy_0=3072,
		obj_0=4096,
		item_0=5120, //256
		total=5376,
	};

	std::pair<som_MPX_64,void*> ins;
	char(&path)[64] = ins.first.cmp;
	//memcpy(path,SOMEX_(a)"\\data\\",10); //10
	memcpy(path,"\\data\\",5); //10

	auto *pos = (BYTE*)&loading; //OPTIMIZING
	void *end = (char*)pos+total;
	enum{ pos_bit=sizeof(*pos)*CHAR_BIT };

	size_t resume = 0;
	for(int bit=0,m=0;resume++<loaded;bit++) 
	{
		if(bit%pos_bit==0&&!*pos) //!
		{
			auto *cmp = pos++; while(!*pos) pos++;

			bit+=(pos-cmp)*pos_bit; assert(pos<end);
		}
		while(!loading.test(bit)) bit++;

		if(bit<snd_0+1024) //SFX or SND?
		{
			if(bit<snd_0) //SFX
			{
				int sfx = bit-sfx_0;
				((BYTE(__cdecl*)(WORD))(0x42e7e0))((WORD&)sfx);
			}
			else som_MPX_43f4f0(bit-snd_0); //SND
		}
		else 
		{
			auto it = models_bit(ins,bit);

			if(it->second) models_refs(it->second)--;
		}
	}

	loaded = 0;

	//HACK: don't want to do this when som_MPX_411a20
	//calls som_MPX_job_2
	if(1!=EX::central_processing_units)
	{
		models_free(); images_free(); sounds_free();
	}
}
extern bool som_MPX_update_texture(int i)
{
	//NOTE: 4 is required to not freeze
	//on UpdateTexture even when using
	//D3DCREATE_MULTITHREADED
	//knockout texture updates can take
	//several milliseconds
	auto &t = SOM::L.textures[i];
	if(t.ref_counter&&t.update_texture(4))
	{
		//som_hacks_onflip is consuming
		//these, 1 per frame, since even
		//in D3DCREATE_MULTITHREADED mode
		//UpdateTexture freezes on another
		//thread (unless I'm responsible)
		som_MPX_textures->push_back(i);
		return true;
	}
	return false;
}
static void som_MPX_job_3(som_MPX_swap::job j) //other?
{
	if(j.g==1) //update textures
	{
		//NOTE: this is CPU bound instead of IO
		//it might run better on another thread
		//but it's useful to know it's inactive
		//while the other jobs are active

		for(int i=0;i<1024;)
		{
			EX::section raii(som_MPX_thread->cs);

			while(i<1024) 
			if(som_MPX_update_texture(i++)) break;
		}

		return; //breakpoint
	}
	else if(j.g==2) //update sky
	{
		som_MPX_sky *p; if(1) //RAII
		{
			EX::section raii(som_MPX_thread->cs);

			p = som_MPX_load_sky(j.index,true);

			if(p) p->make_instance(); //good idea

			if(!p||p->loaded!=1) return;
		}
		auto *o = p->data;
		for(int j=0,i=o->texture_count;i-->0;p->loaded++)
		{
			DWORD t = o->textures[i]; 
			auto &ti = SOM::L.textures[t];
			if(t<1024&&DDRAW::compat&&!ti.uptodate())
			{
				EX::section raii(som_MPX_thread->cs);

				j++; som_MPX_update_texture(t);
			}

			//SOM::TextureFIFO?
			//give some time for the textures to finish uploading
			//(before last loaded++ to delay drawing)
			if(i==0&&j>0) EX::sleep(j*SOM::motions.diff); //ARBITRARY
		}

		return; //breakpoint
	}
	/*else if(j.g==3) //refresh model
	{
		EX::section raii(som_MPX_thread->cs);

		som_MPX_refresh_model(j.index);
	} */
	else assert(0);
}
extern DWORD som_MPX_thread_id = 0;
static DWORD WINAPI som_MPX_thread_main(SOM::Thread *tp) //2022
{
	//SOM_GAME_DETOUR_MULTITHREAD needs this to allow
	//som_game_CreatFile, etc. to work on this thread
	assert(!som_MPX_thread_id);
	som_MPX_thread_id = GetCurrentThreadId(); 

	//for now this runs as a loop until jobs is empty
	for(;;)
	{
		auto job = som_MPX_swap::jobs->remove_front();

		if(!job)
		{
			//I think this needs to wait with the test
			//pattern of calling create after posting a job?
			/*seems unnecessary... I wonder if source of
			//stalling on main thread
			EX::section raii(tp->cs);*/

			job = som_MPX_swap::jobs->remove_front();

			if(!job) //double-lock?
			{
				tp->exited = ~0; break; 
			}
		}

		switch(job.f)
		{
		case 1: som_MPX_job_1(job); break; //load
		case 2: som_MPX_job_2(job); break; //unload
		case 3: som_MPX_job_3(job); break; //other?
		}
	}

	som_MPX_thread_id = 0; return ~tp->exited;
}

static void som_MPX_device_reset() //HACK
{
	//this is patch code to avoid a device
	//reset after the introductory screens

	//Alt+F4? I think this might be the cause of the
	//window coming back to life in full screen when
	//the game is quitting out
	if(SOM::altf&(1<<4)) return;
	
	//bool start = SOM::L.startup!=-1;	
	//if(GetEnvironmentVariableW(L"LOAD",0,0)) //2022
	//start = false;

	//copying som_hacks_device_reset
	extern void *som_hacks_SetDisplayMode(HRESULT*,DDRAW::IDirectDraw7*,DWORD&,DWORD&,DWORD&,DWORD&,DWORD&);
	DWORD *xyz = (DWORD*)0x4c0974;
	//401a22 obsoletes this
	//xyz[0] = start?640:SOM::width; DWORD _ = 0;
	//xyz[1] = start?480:SOM::height; 
	xyz[0] = SOM::width; DWORD _ = 0;
	xyz[1] = SOM::height; 
	//xyz[2] = SOM::bpp; //do_opengl?
	DWORD z = 0;
	som_hacks_SetDisplayMode(0,0,xyz[0],xyz[1],z,_,_);
	DeleteObject(SOM::font); SOM::font = 0;

	/*copying som_game_4212D0
	SOM::menupcs = (DWORD*)0x1A5B4A8;
	((BYTE(__cdecl*)(FLOAT*))0x4212D0)((FLOAT*)(0x1A5B4A8-0xa8/4));	
	extern void som_game_menucoords_set(bool reset);
	som_game_menucoords_set(false);*/
	//4212D0 uses these values
	*(DWORD*)0x1d3d20c = xyz[0];
	*(DWORD*)0x1d3d218 = xyz[1];
	//*(DWORD*)0x1d3d208 = xyz[2]; //do_opengl?
	*(DWORD*)0x1d3d208 = z;
}

extern HMMIO som_MPX_mmioOpenA_BGM = 0;
extern void som_MPX_mmioOpenA_BGM_touch(HMMIO io)
{
	som_MPX_mmioOpenA_BGM = io; //queue up for map transfer

	char buf[8192]; //try to keep BGM data in the MM cache
	mmioSeek(io,0,SEEK_SET); 	
	auto _ = mmioRead(io,buf,sizeof(buf));
	mmioSeek(io,0,SEEK_SET);
}
extern HMMIO som_MPX_mmioOpenA_BGM_open(LPSTR mpx)
{
	char buf[MAX_PATH]; sprintf(buf,"data\\bgm\\%s",mpx);

	extern HMMIO som_game_mmioOpenA_BGM;
	HMMIO swap = som_game_mmioOpenA_BGM;
	extern HMMIO WINAPI som_game_mmioOpenA(LPSTR,LPMMIOINFO,DWORD);
	//som_game_mmioOpenA_BGM = 0;
	HMMIO ret = som_game_mmioOpenA(buf,0,MMIO_READ);
	som_game_mmioOpenA_BGM = swap;
	som_MPX_mmioOpenA_BGM_touch(ret); return ret;
}
extern MMRESULT WINAPI som_MPX_mmioOpenA_BGM_close(HMMIO io)
{
	if(som_MPX_mmioOpenA_BGM==io) som_MPX_mmioOpenA_BGM = 0;

	extern HMMIO som_game_mmioOpenA_BGM;
	return som_game_mmioOpenA_BGM==io?0:mmioClose(io,0); 
}

extern void som_MPX_reprogram()
{
	som_MPX_skies.resize(4);

	//2022: have to initialize these somewhere	
	som_MPX_thread = new SOM::Thread(som_MPX_thread_main);
	(void*&)som_MPX_swap::maps = new som_MPX_swap::mpx[64];
	(void*&)som_MPX_swap::jobs = new som_MPX_swap::job_queue;
	som_MPX_textures = new SOM::TextureFIFO;

	//avoid device reset? //OPTIMIZING?
	//I'm forcing this on
	//if(EX::INI::Window()->do_scale_640x480_modes_to_setting)
	{
		//401a96 is two calls that teardown and rebuild the device
		//knocking them out works but the HUD and menus don't get
		//the memo, so som_MPX_device_reset tries to patch them 
		memset((char*)0x401a96+5,0x90,10-5);
		*(DWORD*)0x401a97 = (DWORD)som_MPX_device_reset-0x401a9b;

		if(0) //viewport isn't set up (OPTIMIZING)
		{
			//this is an earlier call that's not required as far as I
			//can see
			//401a47
			memset((char*)0x401a47+5,0x90,10-5);
			*(DWORD*)0x401a48 = (DWORD)som_MPX_device_reset-0x401a4c;

			//same for movie?
			//4237a7
			memset((char*)0x4237a7+5,0x90,10-5);
			*(DWORD*)0x4237a8 = (DWORD)som_MPX_device_reset-0x4237ac;

			//again post movie
			//42389c
			memset((char*)0x42389c+5,0x90,10-5);
			*(DWORD*)0x42389d = (DWORD)som_MPX_device_reset-0x4238a1;

			//SOM_SYS?
			//40370b
		}
	}
	
	//401fc4 call 448560 (release textures)
	*(DWORD*)0x401fc5 = (DWORD)som_MPX_448560_release_textures-0x401fc9;
	//401e49 call 448400 (rebuild textures)
	*(DWORD*)0x401e4a = (DWORD)som_MPX_448400_rebuild_textures-0x401e4e;
	
	//don't unlock until after loading
	//
	// UNUSED: 43f4f0 was using this, but it's actually not
	// useful for loading in the background. hypothetically
	// it could still be useful
	//
	//som_MPX_411a20 must zero 4C0B98h
	//00402CB3 C6 05 98 0B 4C 00 00 mov         byte ptr ds:[4C0B98h],0
	memset((void*)0x402CB3,0x90,7);

	//unloading subroutine
	//2022: have som_MPX_411a20 manually unload the MPX
	//this knocks out almost everything except deleting
	//the MPX layer and flushing data to the save file
	memset((void*)0x4131b0,0x90,50);
	//2022: don't flush save either
	memset((void*)0x4131a8,0x90,5); //for som_MPX_413190
	//don't delete the MPX memory
	//004133C7 E8 B4 E1 FE FF       call        00401580
	memset((void*)0x4133C7,0x90,5);
	//00402229 E8 62 0F 01 00       call        00413190
	//00402CA7 E8 E4 04 01 00       call        00413190
	//00411A39 E8 52 17 00 00       call        00413190 
	//004253D2 E8 B9 DD FE FF       call        00413190
	/*2022: just keep the map in memory until displaced
	*(DWORD*)0x40222A = (DWORD)som_MPX_413190-0x40222E;	//quit
	*(DWORD*)0x402CA8 = (DWORD)som_MPX_413190-0x402CAC;	//unload
	*(DWORD*)0x411A3a = (DWORD)som_MPX_413190-0x411A3e;	//unload? (2021)
	*(DWORD*)0x4253D3 = (DWORD)som_MPX_413190-0x4253D7; //unload? (2021)*/
	memset((void*)0x402229,0x90,5);
	memset((void*)0x402CA7,0x90,5);
	memset((void*)0x411A39,0x90,5);
	memset((void*)0x4253D2,0x90,5);
	//2022: don't delete global MPX
	{
		//TODO: I'll rewrite this from scratch eventually
		//these are pathological references to the global
		//MPX pointer
		static const WORD x[] = //17
		{0x191,0x1a2,0x1e4,0x223,0x241,0x289,0x2a2,0x2bb,0x2d4,0x300,0x320,0x34d,0x36d,0x389,0x3a2,0x3bb,0x3d1};
		for(int i=EX_ARRAYSIZEOF(x);i-->0;)
		{
			*(DWORD*)(0x413000+x[i]) = (DWORD)&som_MPX_413190_ptr;
		}
	}

	//loading subroutine
	//00402CEE E8 2D ED 00 00       call        00411A20
	//00402CC1 E8 5A ED 00 00       call        00411A20
	*(DWORD*)0x402CEF = (DWORD)som_MPX_411a20-0x402CF3; //load	
	*(DWORD*)0x402CC2 = (DWORD)som_MPX_411a20-0x402CC6; //SOM_EDIT (load)	
	//00411F13 E8 C8 3E FF FF       call        00405DE0
	*(DWORD*)0x411F14 = (DWORD)som_MPX_405de0-0x411F18; //enemy
	//00411F8A E8 C1 69 01 00       call        00428950
	*(DWORD*)0x411f8b = (DWORD)som_MPX_428950-0x411f8f; //npc
	//00411E28 E8 C3 87 01 00       call        0042A5F0 
	*(DWORD*)0x411E29 = (DWORD)som_MPX_42a5f0-0x411E2d; //object
	//00412008 E8 d3 dd ff ff       call        40fde0 
	*(DWORD*)0x412009 = (DWORD)som_MPX_40fde0-0x41200d; //items (2024)
	{
		//NOTE: the MHM loading routines has an unnecessary
		//new/delete and "401500" isn't really needed if the
		//original MPX loader code will never runs again once
		//som_MPX_swap is implemented
		#ifdef NDEBUG
		//#error consider removing this stuff around MHM reads
		int todolist[SOMEX_VNUMBER<=0x1020704UL];
		#endif
		//REMOVE ME? (401500)
		//00412E63 E8 98 E6 FE FF       call        00401500
		*(DWORD*)0x412E64 = (DWORD)som_MPX_401500-0x412E68; //dummy MHM, etc.
		//2022: need to replace these two MHM fread calls to
		//reuse this subroutine with SomEx.dll's fread
		//00417645 E8 C9 84 03 00       call        0044FB13
		//00417663 E8 AB 84 03 00       call        0044FB13 
		*(DWORD*)0x417646 = (DWORD)fread-0x41764a;
		*(DWORD*)0x417664 = (DWORD)fread-0x417668;
	}
	if(1) //don't drop frames on map change (come what may)
	{
		//UPDATE: when I disable this block I don't visually
		//see a hiccup in the tests I did, but I'm afraid to
		//disable it since I know I threw in some extra stuff
		//and it's quite complicated

		//REMINDER: this depends on som_game_402070_timeGetTime
		//to limit the input loop to 1 per frame... assuming it
		//will always do so going forward

		//NOTE: to determine this I setup the loading routine
		//to not do anything except process the map transfer
		//event, since I couldn't rule out disk IO hiccups
		
		//00402159 0F 85 2A FF FF FF    jne         00402089
		//00402272 0F 85 3C 01 00 00    jne         004023B4
		memset((void*)0x402272,0x90,6);
		//402159 glitches on reset after dying
		//this code switches the byte to check to the one that's
		//set on death or starting a test
		//som_MPX_411a20 has to zero this byte to not trigger it
		//memset((void*)0x402159,0x90,6); 
		//00402152 A0 98 0B 4C 00       mov         al,byte ptr ds:[004C0B98h]
		*(DWORD*)0x402153 = 0x4c0ca1; //Corridor::dbsetting
		//set dbsetting to 1 on save game load
		//0042CD5E C6 05 A1 0C 4C 00 00 mov         byte ptr ds:[4C0CA1h],0  
		*(BYTE*)0x42CD64 = 1;
	}

	if(1) //takeover TXR management?
	{ 
		//knockout linear lookup
		//NOTE: the DIB pathway uses the same lookup system
		//00449004 33 ED                xor         ebp,ebp
		*(WORD*)0x449004 = 0x55eb; //jmp 44905b
		//install wrapper
		//actually som_TXR_4485e0 has to wrap the art system
		//00448634 E8 97 09 00 00       call        00448FD0
		//*(DWORD*)0x448635 = (DWORD)som_MPX_448fd0-0x448639;
		// 
		//the MDL (TIM) path does this too even though it's
		//nonsensical because MDL textures are unique. the
		//TIM subroutines just repurpose this path that I
		//believe should only be used by unique textures 
		//00448692 33 ed           XOR        EBP,EBP
		*(WORD*)0x448692 = 0x4ceb; //jmp 4486e0
 
		//texture deleting subroutine (select TXR calls only)
		//413346 //mpx
		//41962c //item menu (map item display)
		//42e8f8 //sfx
		//42e989 //sfx (mpx)
		//4419f8 //skin
		//4419cf //skin #2 (unused atlas mode)
		//444c24 //mdl
		//444c48 //mdl #2 (unused atlas mode)
		//445895 //mdo
		*(DWORD*)0x413347 = (DWORD)som_MPX_448780-0x41334b; //mpx
		*(DWORD*)0x42e8f9 = (DWORD)som_MPX_448780-0x42e8fd; //sfx
		*(DWORD*)0x42e98a = (DWORD)som_MPX_448780-0x42e98e;	//sfx
		*(DWORD*)0x4419f9 = (DWORD)som_MPX_448780-0x4419fd; //skin
		*(DWORD*)0x444c25 = (DWORD)som_MPX_448780-0x444c29; //mdl
		*(DWORD*)0x445896 = (DWORD)som_MPX_448780-0x44589a; //mdo

		//don't reapply skin to legacy MDL data
		//00405FD3 E8 B8 B9 03 00       call        00441990 //enemy
		//00428B3A E8 51 8E 01 00       call        00441990 //npc 
		*(DWORD*)0x405FD4 = (DWORD)som_MPX_441990-0x405FD8;
		*(DWORD*)0x428B3b = (DWORD)som_MPX_441990-0x428B3f;
	}
	if(1) //takeover SND and SFX managaement?
	{
		#ifdef NDEBUG
		//TODO: I think object, enemy and npc sounds set to
		//animation frames may not be unloaded and may also
		//increase the ref counts for each instance so that
		//in theory they could reach 65535		
		int todolist[SOMEX_VNUMBER<=0x1020704UL];
		#endif
		
		//load SND (bug fix)
		//0040643D E8 DE 8F 03 00       call        0043F420 //enemy
		//0042727C E8 9F 81 01 00       call        0043F420 //equip
		//00428C87 E8 94 67 01 00       call        0043F420 //npc
		//0042A823 E8 F8 4B 01 00       call        0043F420 //object
		//0042E612 E8 09 0E 01 00       call        0043F420 //sfx (instance)
		//0042E65C E8 BF 0D 01 00       call        0043F420 //sfx (instance)
		//0043f36a main (16 sys.dat sounds)
		//0043f399 main (sys.dat menu sounds?) (not sure)
		//0043f3ae main (890)
		//0043f3ba main (891)
		*(DWORD*)0x40643e = (DWORD)som_MPX_43f420-0x406442; //enemy
		*(DWORD*)0x42727d = (DWORD)som_MPX_43f420-0x427281; //equip
		*(DWORD*)0x428C88 = (DWORD)som_MPX_43f420-0x428C8c; //npc
		*(DWORD*)0x42A824 = (DWORD)som_MPX_43f420-0x42A828; //object
		*(DWORD*)0x42E613 = (DWORD)som_MPX_43f420-0x42E617; //sfx
		*(DWORD*)0x42E65d = (DWORD)som_MPX_43f420-0x42E661; //sfx

		//unload SND (map transfer optimization)
		//00427667 E8 84 7E 01 00       call        0043F4F0 //equip
		//0042E822 E8 B9 FF FF FF       call        0042E7E0 //sfx (fix)
		//0042E86A E8 81 0C 01 00       call        0043F4F0 //sfx 
		//0042EB42 E8 A9 09 01 00       call        0043F4F0 //sfx (wrong)
		//0042EB8A E8 61 09 01 00       call        0043F4F0 //sfx (wrong)
		*(DWORD*)0x427668 = (DWORD)som_MPX_43f4f0-0x42766c; //equip (04275e0)
		*(DWORD*)0x42e86b = (DWORD)som_MPX_43f4f0-0x42e86f; //sfx
	//	*(DWORD*)0x42eb43 = (DWORD)som_MPX_43f4f0-0x42eb47; //sfx (below)
	//	*(DWORD*)0x42eb8b = (DWORD)som_MPX_43f4f0-0x42eb8f; //sfx (below)
		//BAD BUG (MEMORY LEAK) (older bug fix)
		//
		// here unloading code is calling the SFX unloading 
		// routine instead of the SND unloading routine. it
		// crashes on -1.0 for some reason. if not for that
		// it might have stayed unnoticed
		//
		//0042E822 E8 B9 FF FF FF       call        0042E7E0  
		//*(DWORD*)0x42E823 = 0x43f4f0-0x42E827;
		*(DWORD*)0x42E823 = (DWORD)som_MPX_43f4f0-0x42E827; //sfx (fix)
		//
		// 2022: more SFX mistakes... these unload the SND
		// in the wrong context, so the real subroutine is
		// unloading it twice in addition to these. (these
		// subroutines seem to withdraw an SFX's instances
		// in a way that may be abrupt/incorrect. PC only)
		//
		memset((void*)0x42EB42,0x90,5);
		memset((void*)0x42EB8A,0x90,5);

		//disable adding SFX and SND instances for every
		//enemy/NPC instance
		//som_MPX_405de0 and som_MPX_428950 will put the
		//code back
		// 
		// 2022: [un]load_models_etc is taking over this
		// 
		//004063B5 74 09                je          004063C0
		*(BYTE*)0x4063B5 = 0xe9; //jmp 406460
		*(DWORD*)0x4063B6 = 0xa6;
		memset((void*)0x4063Ba,0x90,6); //keep code clean
		//NPC
		//00428BAB 74 09                je          00428BB6
		*(BYTE*)0x428BAB = 0xe9; //jmp 428cb6
		*(DWORD*)0x428BAc = 0x106;
		memset((void*)0x428Bb0,0x90,6); //keep code clean
		//object
		//0042A7FD 74 09                je          0042A808
		*(WORD*)0x42A7FD = 0x57eb; //jmp 42a856
	}

	//MATERIALS FIX
	{
		//this code compares against the material counter
		//instead of capacity. the counter is meaningless
		//once materials are removed and if any ever fail 
		//to be removed, but this is needed too so to not
		//have to remake sky model instances and also SFX
		//model instances are currently not remade either		
		//00448158 39 05 50 D2 D3 01    cmp         dword ptr ds:[1D3D250h],eax
		*(DWORD*)0x44815a+=4;
	}

	//don't waste time releasing resources on exit
	{
		/*som_MPX_new requires releasing instance data on
		//exit, but it's always kind of stupid to release
		//data on exit as it just wastes time. there were
		//stats on memory leaks, but som_MPX_operator_new
		//bypasses them when using the SomEx.dll CRT heap
		00401B3D E8 4E BB 02 00       call        0042D690  
		00401B42 E8 39 2B 02 00       call        00424680  
		00401B47 E8 C4 6D 02 00       call        00428910  
		00401B4C E8 4F 42 00 00       call        00405DA0  
		00401B51 E8 6A 8A 02 00       call        0042A5C0  
		00401B56 E8 15 E0 00 00       call        0040FB70  
		00401B5B E8 20 23 00 00       call        00403E80  
		00401B60 E8 FB B2 03 00       call        0043CE60  
		00401B65 E8 86 68 00 00       call        004083F0  
		00401B6A E8 31 CA 02 00       call        0042E5A0  
		00401B6F E8 EC B1 03 00       call        0043CD60  
		00401B74 E8 D7 1E 00 00       call        00403A50  
		00401B79 E8 52 21 02 00       call        00423CD0  
		00401B7E E8 7D F4 00 00       call        00411000  
		00401B83 E8 B8 D6 00 00       call        0040F240  
		00401B88 E8 43 D8 03 00       call        0043F3D0  
		00401B8D E8 6E 61 04 00       call        00447D00  
		00401B92 E8 F9 67 04 00       call        00448390
			NOP (446b70) (vbuffer release is out of order)
		00401B9C 8B 0D 3C 22 4C 00    mov         ecx,dword ptr ds:[4C223Ch]  
		00401BA2 51                   push        ecx  
		00401BA3 E8 D8 B7 04 00       call        0044D380  
		00401BA8 83 C4 04             add         esp,4  
		00401BAB E8 B0 B3 04 00       call        0044CF60  
		00401BB0 E8 FB 03 00 00       call        00401FB0  
			//prints heap stats
		00401BB5 E8 26 FA FF FF       call        004015E0*/
			//TODO: jmp?
		memset((void*)0x401B3D,0x90,0x401BBA-0x401B3D);
	}

	//change center point of neutral head position?
	{
		//00424A9A C7 05 A4 1D 9C 01 00 00 00 00 mov         dword ptr ds:[19C1DA4h],0
		memset((void*)0x424A9A,0x90,10);
		*(BYTE*)0x424A9A = 0xe8; //CALL
		*(DWORD*)0x424A9b = (DWORD)som_MPX_center_u-0x424A9f; 
	}

	//tile draw order and visibility
	extern void som_MPY_reprogram(); som_MPY_reprogram(); //2023
}

SOM::Thread::Thread(DWORD(WINAPI*f)(Thread*))
{
	memset(this,0x00,sizeof(*this)); main = f;

	cs = new EX::critical; //could be optional
}
SOM::Thread::~Thread()
{
	delete cs;
}
bool SOM::Thread::close()
{
	if(!cs) return false; //suppressing?

	EX::section raii(cs); //EX_CRITCAL_SECTION
	
	if(!handle) return false;

	if(CloseHandle(handle))
	{
		suspended = 0;

		handle = 0; return true;		
	}
	else assert(0); return false;
}
int SOM::Thread::suspend()
{
	if(!cs) return 0; //suppressing?

	EX::section raii(cs); //EX_CRITCAL_SECTION

	if(suspended||!handle) return 0;

	//GetThreadContext is needed to force suspension:
	//"The SuspendThread function suspends a thread, but it does so asynchronously"
	//https://devblogs.microsoft.com/oldnewthing/20150205-00/?p=44743
	if(SuspendThread(handle)) 
	assert(0);
	CONTEXT _;
	GetThreadContext(handle,&_);

	return suspended = 1;
}
int SOM::Thread::resume()
{
	EX::section raii(cs); //EX_CRITCAL_SECTION

	if(1!=suspended||!handle) return 0;

	return suspended = ResumeThread(handle);
}
bool SOM::Thread::create()
{
	if(1==EX::central_processing_units)
	{
		assert(1!=EX::central_processing_units);
		return false;
	}

	//I've put this in the constructor so it's
	//not a race condition
	//if(!cs) cs = new EX::critical;

	EX::section raii(cs); //EX_CRITCAL_SECTION

	if(exited) //testing?
	{
		if(CloseHandle(handle)) handle = 0;
	}
	if(handle) return false;

	exited = 0;
	auto f = (LPTHREAD_START_ROUTINE)main;
	handle = CreateThread(0,0,f,this,0,0); return true;
}

SOM::TextureFIFO::TextureFIFO()
{
	_front = _back = 0;
	memset(_buffer,0xff,sizeof(_buffer));
}
void SOM::TextureFIFO::push_back(WORD t)
{
	LONG a,b; /*if(0) //volatile?
	{
		do{ a = _back; b = (a+1)%1024; }
		while(a!=InterlockedCompareExchange(&_back,b,a);
	}
	else*/{ a = _back; _back = (a+1)%1024; }

	_buffer[a] = t; 
}
WORD SOM::TextureFIFO::remove_front()
{
	//WARNING: this could jam if there were 1024 textures
	//however, there should always be some textures that
	//don't require mipmaps/colorkey so it should be okay
	if(empty()) return 0xffff;

	LONG a,b; /*if(0) //volatile?
	{
		do{ a = _front; b = (a+1)%1024; }
		while(a!=InterlockedCompareExchange(&_front,b,a);
	}
	else*/{ a = _front; _front = (a+1)%1024; }

	WORD ret = _buffer[a]; _buffer[a] = 0xffff;

	assert(ret<1024); return ret;
}

extern void som_MPX_push_back_transfer_job(int m, int mask)
{
	if((DWORD)m>=64) return;

	if(1==EX::central_processing_units) return;

	//REMINDER: need to do this even if the job
	//isn't required
	if(m!=SOM::mpx) SOM::mpx2 = m; //I guess?

	auto &map = (*som_MPX_swap::maps)[m];

	if(map.corridor_mask //HACK: load_finished (0x100)
	||map.load&&map.loaded==map.load) //can't load anymore?
	{
		extern void som_MPX_mmioOpenA_BGM_touch(HMMIO);
		som_MPX_mmioOpenA_BGM_touch(map.bgmmio);

		return; 
	}
	
	//can't load anymore?
	if(map.load&&map.loaded==map.load) return;

	//UNUSED: these aren't useful until there's
	//a system in place for loading map content
	//after a map is in play	
	//map.corridor_xyz[0] = evt->setting[0]*2;
	//map.corridor_xyz[1] = evt->zsetting;
	//map.corridor_xyz[2] = evt->setting[1]*2;
	//map.corridor_mask = 0x100|evt->nosettingmask;
	map.corridor_mask = 0x100|mask;

	som_MPX_swap::job j = {(unsigned)m,1}; //load map
	som_MPX_swap::jobs->push_back(j);
	som_MPX_push_back_textures_job();
}
extern void __cdecl som_MPX_corridor(DWORD event, DWORD *stack)
{
	auto evt = (swordofmoonlight_evt_code3b_t*)*stack;
	*stack+=evt->x0c; 
	
	if(1==EX::central_processing_units) return;

	int m = evt->map; if(m>=64) return;
	
	auto &map = (*som_MPX_swap::maps)[m];

	map.last_tick = EX::tick();

	if(m==SOM::mpx2) //redundant event?
	{
		if(map.wip) return;
	}
	if(m==SOM::mpx) return; //???

	//HACK: load_finished (0x100)
	if(map.corridor_mask) return;
		
	//UNUSED: these aren't useful until there's
	//a system in place for loading map content
	//after a map is in play	
	map.corridor_xyz[0] = evt->setting[0]*2;
	map.corridor_xyz[1] = evt->zsetting;
	map.corridor_xyz[2] = evt->setting[1]*2;
	//map.corridor_mask = 0x100|evt->nosettingmask;
	som_MPX_push_back_transfer_job(m,evt->nosettingmask);
}

extern void som_MPX_refresh_mpx(int m)
{
	if((DWORD)m>=64) return;

	auto &map = (*som_MPX_swap::maps)[m];

	if(!map.loaded) return;
	
	if(m!=SOM::mpx)
	{
		som_MPX_swap::job j = {(unsigned)m};

		j.f = 2; som_MPX_job_2(j,true); //unload?

		map.corridor_mask = 0x100; //I guess

		j.f = 1; som_MPX_job_1(j); //load

	//	if(m==SOM::mpx) som_MPX_411a20_evt(m);	
	}
}
extern void som_MPX_refresh_evt(int m)
{
	if((DWORD)m>=64) return;

	auto &map = (*som_MPX_swap::maps)[m];

	if(!map.load) return;

	if(m!=SOM::mpx)
	{
		map.reload_evt(m);

	//	if(m==SOM::mpx) som_MPX_411a20_evt(m);
	}
}

SOM::Animation::Animation()
{
	assert((unsigned)this+1u>4096); //deprecated

	memset(this,0x00,sizeof(*this)); 
}
SOM::Animation::~Animation()
{
	assert((unsigned)this+1u>4096); //deprecated

	delete[] data; release();
}
void SOM::Animation::release()
{
	if(texture) ((IUnknown*)texture)->Release(); texture = nullptr;
}
void SOM::Animation::upload()
{
	if(texture||!data) return;

	DX::DDSURFACEDESC2 d = {sizeof(d),
	DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT,(unsigned)h,(unsigned)w,data_s/h};
	d.dwHeight = h;
	d.dwWidth = w;
	d.dwFlags|=DDSD_CAPS;
	d.ddsCaps.dwCaps = DDSCAPS_TEXTURE; 
	int bpp = data_s/(w*h);
	DX::DDPIXELFORMAT pf = {0,DDPF_RGB,0,(unsigned)(8*bpp)};
	if(bpp>=2) pf.dwRBitMask = 0xff0000;
	if(bpp>=2) pf.dwGBitMask = 0x00ff00;
	if(bpp>=3) pf.dwBBitMask = 0x0000ff;
	if(bpp==1||bpp==4) pf.dwRGBAlphaBitMask = 0xff000000;
	if(bpp==1||bpp==4) pf.dwFlags|=DDPF_ALPHAPIXELS;
	d.ddpfPixelFormat = pf;
	auto swap = DDRAW::colorkey;
	auto swap2 = DDRAW::mipmap;
	if(bpp==1||bpp==4) 
	DDRAW::colorkey = nullptr;
	DDRAW::mipmap = nullptr;
	HRESULT hr = DDRAW::DirectDraw7->CreateSurface(&d,(DX::LPDIRECTDRAWSURFACE7*)&texture,0);
	DDRAW::colorkey = swap;
	DDRAW::mipmap = swap2;
	if(!hr&&!texture->Lock(0,&d,DDLOCK_WAIT|DDLOCK_WRITEONLY,0))
	{
		int p = w*bpp; if(d.lPitch!=p)
		{
			if(d.lPitch>p) for(int i=0;i<h;i++)
			{
				auto *row = (BYTE*)d.lpSurface+d.lPitch*i;
				(DWORD&)(row[p/4*4]) = 0; //~0?
				memcpy(row,data+p*i,p);
			}
			else assert(d.lPitch>p);
		}
		else memcpy(d.lpSurface,data,p*h);

		texture->Unlock(0);
		texture->updating_texture(); //debugging OpenXR
	}
	else assert(!hr);
}

	//ATLAS //ATLAS //ATLAS //ATLAS //ATLAS //ATLAS //ATLAS //ATLAS //ATLAS //ATLAS

#if 0  //EXPERIMENTAL //UNUSED //REMOVE ME?

// stb_rect_pack.h - v1.00 - public domain - rectangle packing
// Sean Barrett 2014
#define STB_RECT_PACK_IMPLEMENTATION
#include "../Exselector/src/nanovg/stb_rect_pack.h"
struct som_MPX_swap::mpx::atlas
{
	bool a; //marking as transparent

	stbrp_context c;

	stbrp_node m[1024]; //OVERKILL?

	DDRAW::IDirectDrawSurface7 *texture;

	atlas(bool a):a(a),texture()
	{
		stbrp_init_target(&c,8192,8192,m,1024);
	}

	bool pack(stbrp_rect &p)
	{
		stbrp_pack_rects(&c,&p,1); return p.was_packed!=0;
	}
};
extern void som_MPX_atlas2(DWORD t) //som.game.cpp
{
	enum{ border=8 }; //UNIMPLEMENTED

	static const float l_8192 = 1/8192.0f;

	if(t<1024) for(int a=2;a-->0;) 
	{
		auto &ta = SOM::TextureAtlas[t][a];

		if(DWORD m=ta.pending) 
		{	
			ta.pending = false;

			m = (BYTE)~m; //HACK

			auto &map = (*som_MPX_swap::maps)[m];

			SOM::Texture &tt = SOM::L.textures[t];

			DX::DDSURFACEDESC2 desc = {sizeof(DX::DDSURFACEDESC2)}; //REMOVE ME			
			desc.dwFlags = DDSD_HEIGHT|DDSD_WIDTH;
			tt.texture->GetSurfaceDesc(&desc);

			stbrp_rect r = {t};
			r.w = desc.dwWidth+border; r.h = desc.dwHeight+border;

			auto &v = map.atlases[a];

			size_t i; for(i=0;i<v.size();i++)
			{
				if(v[i]->pack(r)) break;
			}
			if(!r.was_packed)
			{
				v.push_back(new som_MPX_swap::mpx::atlas(a==1)); 
				v.back()->texture = DDRAW::CreateAtlasTexture(8192);
				v.back()->pack(r);
			}

			ta.s = r.x*l_8192; ta.t = r.y*l_8192;
			ta.x = r.w*l_8192; ta.y = r.h*l_8192;

			ta.texture = v[i]->texture;

			DDRAW::BltAtlasTexture(ta.texture,tt.texture,border,r.x,r.y);
		}
	}
}

static void som_MPX_atlas_pending(som_MPX_swap::mpx &map, DWORD t, bool mat)
{
	SOM::MT **mt = *SOM::material_textures;
	
	if(!SOM::TextureAtlas[t][1].texture)
	if((SOM::volume_textures[t]
	||mat||mt[t]&&(mt[t]->data[3]!=1.0f||mt[t]->mode&0x100)))
	{
		SOM::TextureAtlas[1][t].pending = (BYTE)~(&map-*som_MPX_swap::maps); //HACK
	}	
}
void som_MPX_swap::mpx::load_mpx_atlas(WORD *tp, DWORD tc)
{		
	for(DWORD i=tc;i-->0;)
	{
		DWORD t = tp[i]; if(t<1024)
		{
		//	if(!SOM::TextureAtlas[t][0].texture)
		//	SOM::TextureAtlas[t][0].pending = (BYTE)~(&map-*som_MPX_swap::maps);
			som_MPX_atlas_pending(*this,t,false);

			som_MPX_textures->push_back(t); //HACK
		}
	}
}
void som_MPX_swap::mpx::load_models_atlas()
{	
	for(size_t i=models.size();i-->0;) 
	{
		union
		{
			SOM::MDL::data *mdl;
			SOM::MDO::data *mdo;
			void *cmp; 
		};

		cmp = models[i];

		if(!models_type(cmp)) mdo = mdl->ext.mdo;

		if(!mdo) continue;

		for(int j=mdo->chunk_count;j-->0;)
		{
			auto &ch = mdo->chunks_ptr[i];

			DWORD t = ch.matnumber;

			if(t>=1024||SOM::TextureAtlas[t][1].pending)
			continue;

			//same as som_bsp_make_mdo_instance				
			if(ch.blendmode||mdo->materials_ptr[t][3]!=1.0f)
			{
				som_MPX_atlas_pending(*this,ch.texnumber,true);
			}			
			else som_MPX_atlas_pending(*this,t,false);

			som_MPX_textures->push_back(t); //HACK
		}
	}	
}

#endif //EXPERIMENTAL
