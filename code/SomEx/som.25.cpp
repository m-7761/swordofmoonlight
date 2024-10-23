#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

#include "SomEx.ini.h"
#include "som.state.h"

#import "..\KingsField25\bin\Debug\KingsField25.tlb" //C#

static struct som_25_t
{
	KingsField25::IEngine *e;
	KingsField25::IFrame *a,*b;

	void collect(int all),compare();

}som_25 = {};

struct som_25_Game_i : KingsField25::IGame //EXPERIMENTAL
{
	ULONG _rc;
	som_25_Game_i():_rc(1){}
	~som_25_Game_i(){ assert(0); }
	virtual ULONG __stdcall AddRef(){ return ++_rc; }
    virtual ULONG __stdcall Release(){ return --_rc; }
	virtual HRESULT __stdcall QueryInterface(REFIID,void**);
	virtual HRESULT __stdcall raw_sound(BSTR,long*);
	virtual HRESULT __stdcall raw_PlaySound(long,float,float,float,long,long,float*);
};

extern void som_CS_init() //C#
{	
	EX::INI::Engine en;

	if(en->do_KingsField25)
	{
		auto &uuid1 = __uuidof(KingsField25::Engine);
		auto &uuid2 = __uuidof(KingsField25::IEngine);
		CoCreateInstance(uuid1,0,CLSCTX_ALL,uuid2,(void**)&som_25.e);
		auto &uuid3 = __uuidof(KingsField25::Frame);
		auto &uuid4 = __uuidof(KingsField25::IFrame);
		CoCreateInstance(uuid3,0,CLSCTX_ALL,uuid4,(void**)&som_25.a);
		CoCreateInstance(uuid3,0,CLSCTX_ALL,uuid4,(void**)&som_25.b);

		if(som_25.e) som_25.e->game = new som_25_Game_i;
		if(som_25.b) som_25.b->map = -1;
	}
}

template<class T> 
//CComSafeArray?
//
// MEMORY LEAK
// as far as I can tell these have to be created
// and destroyed everytime you want to access it
// their getters always return a new pointer, so
// I think SafeArrayDestroy is required. I dunno
//
struct sa_copy
{
	SAFEARRAY *a;

	sa_copy(SAFEARRAY *a):a(a)
	{}
	~sa_copy()
	{
		if(a) SafeArrayDestroy(a);
	}

	const T &operator[](int i)
	{
		return ((T*)a->pvData)[i];
	}
};

void som_25_t::collect(int all)
{
	a->clock = SOM::motions.tick;

	a->map = SOM::mpx;
	a->sky = SOM::mpx_defs(SOM::mpx)._sky;

	sa_copy<WORD> ac = a->counters;

	//doesn't work???
	//SAFEARRAYBOUND sab = {1024,0};
	//static SAFEARRAY *sac = SafeArrayCreate(VT_UI2,1,&sab);
	//memcpy(sac->pvData,SOM::L.counters,2*1024);
	//a->counters = sac;
	for(int i=1024;i-->0;) if(ac[i]!=SOM::L.counters[i])
	{
		//NOTE: set_counter is a custom method
		a->set_counter(i,SOM::L.counters[i]);
	}

	sa_copy<WORD> eo = e->objflags;

	int flags = all;

	for(int i=SOM::L.ai3_size;i-->0;) 
	{
		if(!all) flags = eo[i]; 
		
		if(!flags) continue;

		auto o = a->Obj(i);

		auto &ai = SOM::L.ai3[i];
				
		o->spawn = ai[SOM::AI::obj_shown]!=0;

		o->profile = ai[SOM::AI::object];

		float *xyzuvw = &ai[SOM::AI::xyzuvw3];
		o->x = xyzuvw[0];
		o->y = xyzuvw[1];
		o->z = xyzuvw[2];
		o->u = xyzuvw[3];
		o->v = xyzuvw[4];
		o->w = xyzuvw[5];
	}	

	float *xyzuvw = SOM::L.pcstate;
	a->x = xyzuvw[0];
	a->y = xyzuvw[1];
	a->z = xyzuvw[2];
	a->v = xyzuvw[4];
}
void som_25_t::compare()
{
	if(a->map!=SOM::mpx)
	{
		//UNIMPLEMENTED
	}

	SOM::mpx_defs(SOM::mpx).sky = a->sky;

	sa_copy<WORD> ac = a->counters;

	memcpy(SOM::L.counters,&ac[0],2*1024);

	sa_copy<WORD> eo = e->objflags;

	for(int i=SOM::L.ai3_size;i-->0;) if(int flags=eo[i])
	{
		auto o = a->Obj(i);

		auto &ai = SOM::L.ai3[i];
				
		float t = o->spawn;
		bool nz = t!=0;
		if(nz!=(ai[SOM::AI::obj_shown]!=0))
		{
			ai[SOM::AI::obj_shown] = nz;
		}
		//shadowing (o)
		{
			auto *l = (SOM::MDL*)ai[SOM::AI::mdl3];
			auto *o = (SOM::MDO*)ai[SOM::AI::mdo3];
			if(l) l->fade = t;
			if(o) o->fade = t;
		}

		ai[SOM::AI::object] = o->profile;

		float *xyzuvw = &ai[SOM::AI::xyzuvw3];
		xyzuvw[0] = o->x;
		xyzuvw[1] = o->y;
		xyzuvw[2] = o->z;
		xyzuvw[3] = o->u;
		xyzuvw[4] = o->v;
		xyzuvw[5] = o->w;
	}

	float *xyzuvw = SOM::L.pcstate;
	xyzuvw[0] = a->x;
	xyzuvw[1] = a->y;
	xyzuvw[2] = a->z;
	xyzuvw[4] = a->v;
}
extern void som_CS_commit()
{
	if(som_25.e&&som_25.a)	
	{
		//not working???
		//if(som_25.a->map!=som_25.b->map)
		if(SOM::frame==SOM::newmap)
		{
			som_25.a->map = SOM::mpx;
			som_25.collect(~0);
			som_25.e->Change(som_25.a,som_25.b);
			som_25.compare();
			som_25.b->map = SOM::mpx;
		}
		else
		{
			std::swap(som_25.a,som_25.b);
			som_25.collect(0);
			som_25.e->Commit(som_25.a,som_25.b);
			som_25.compare();
		}
	}
}

HRESULT __stdcall som_25_Game_i::raw_sound(BSTR filename, long *snd)
{
	*snd = SOM::SND(filename);
	return S_OK;
}
HRESULT __stdcall som_25_Game_i::raw_PlaySound(long snd, float x, float y, float z, long pitch, long volume, float *duration)
{
	float pos[3] = {x,y,z}; 
	*duration = SOM::se3D(pos,snd,pitch,volume); 
	return S_OK;
}
HRESULT __stdcall som_25_Game_i::QueryInterface(REFIID rid, void **o)
{
	if(rid==IID_IUnknown) //happens???
	{
		*o = this; return S_OK;
	}
	if(rid==__uuidof(KingsField25::IGame)) //"038c7e1f-b7c5-4b3a-afed-057991bd347f"
	{
		*o = this; return S_OK;
	}
	return E_NOINTERFACE; 
}	
