
#include "Somtexture.pch.h"

#include "Somthread.h" 

//private implementation
#include "Somenviron.inl"

#include "Sominstant.h"

//private
Somenviron::Somenviron(){}
Somenviron::Somenviron(const Somenviron &cp){}
Somenviron::~Somenviron(){}

bool Somenviron::add(int traps, const Sominstant **in, size_t s)
{
	if(!this||!Somenviron_inl) return false;

	if(!Somenviron_inl->f&&!Somenviron_inl->d)
	{
		assert(0); return false; //did we forget about ascribe?
	}

	if(!in){ assert(0); return false; } //invalid input
	
	for(size_t i=0;i<s;i++)
	{
		if(in[i])
		{
			switch(traps)
			{
			case POV: case FOG: case SKY: case VIEW: case LAMP:

				Somenviron_inl->insert(traps,in[i]); break; //->

			default: assert(0); return false;
			}
		}
	}

	return true;
}

bool Somenviron::use(int traps, const Sominstant **in, size_t s)
{
	if(!this||!Somenviron_inl) return false;

	if(!in){ assert(0); return false; } //use 0 overload
	
	for(size_t i=0;i<s;i++)
	{
		if(in[i])
		{
			switch(traps)
			{
			case POV: case FOG: case SKY:

				Somenviron_inl.insert(traps,in[i]); break; //.

			default: assert(0); return false;
			}
		}
		else //.
		{
			if(traps&POV) Somenviron_inl.pov = 0;
			if(traps&FOG) Somenviron_inl.fog = 0;
			if(traps&SKY) Somenviron_inl.sky = 0;
		}

		assert(i==0);
	}

	return true;
}

bool Somenviron::remove(int traps, const Sominstant **in, size_t s)
{
	if(!this||!Somenviron_inl) return false;

	if(!in){ assert(0); return false; } //use 0 overload
	
	for(size_t i=0;i<s;i++)
	{
		if(in[i])
		{
			switch(traps)
			{
			case POV: case FOG: case SKY: 

				if(Somenviron_inl[traps]==in[i])
				{
					Somenviron_inl.remove(traps); continue;
				}
			}

			switch(traps)
			{		
			case POV: case FOG: case SKY: 
				
				assert(0); //TODO: warning?

			case VIEW: case WALL: case LAMP:

				Somenviron_inl->remove(traps,in[i]); break; //->

			default: assert(0); return false;
			}
		}
		else //.
		{
			if(traps&POV) Somenviron_inl.remove(POV);
			if(traps&FOG) Somenviron_inl.remove(FOG);
			if(traps&SKY) Somenviron_inl.remove(SKY);
		}

		assert(i==0);
	}

	return true;
}

bool Somenviron::trigger(int traps, const Sominstant **in, size_t s)
{
	if(!this||!Somenviron_inl) return false;

	assert(0); return false; //unimplemented
}

int Somenviron::map(const Sominstant*, int traps)const
{
	if(!this||!Somenviron_inl) return 0;

	assert(0); return false; //unimplemented
}

size_t Somenviron::list(int in, int traps, const Sominstant **inout, size_t s, size_t skip)const
{
	if(!this||!Somenviron_inl) return 0;

	if(traps==POV&&in==POV) //hack: shortcut
	{
		if(inout)
		{
			if(s) *inout = Somenviron_inl[POV]; return s?1:0;
		}
		else return !skip?1:0;
	}

	return 0; //unimplemented
}

void Somenviron::ascribe(scribe_f f, scribe_d d)
{
	if(!this||!Somenviron_inl.map) return;

	Somenviron_inl->f = f; Somenviron_inl->d = d;

	Somenviron_inl->salted = !f&&!d;
}
				
bool Somenviron::prescribe(const Sominstant *in, int *q, size_t q_s, size_t sz, void *get, size_t get_s)const
{
	if(!this||!Somenviron_inl) return false;
	
	if(!q||!q_s||!get||!get_s) return false;	

	if(!in&&!(in=Somenviron_inl[POV])) return false; //pov
	
	size_t rd = 0; 

	if(sz==sizeof(float)&&Somenviron_inl->f)
	{
		rd = Somenviron_inl->f((Sominstant*)in,q,q_s,(float*)get,get_s); if(rd==q_s) return true;
	}
	else if(sz==sizeof(double)&&Somenviron_inl->d)
	{
		rd = Somenviron_inl->d((Sominstant*)in,q,q_s,(double*)get,get_s); if(rd==q_s) return true;		
	}
	else //todo: temporary conversion buffer
	{
		assert(0); return false; //unimplemented
	}

	if(rd>q_s){ assert(0); return false; } //paranoia

	size_t pos = 0;

	for(size_t i=0;i<rd;i++) switch(q[i])
	{
	case FRUSTUM: pos+=16; break;

	case SIGNAL: case EVENT: pos+=4; break;

	case GLOBAL: case STATIC: case SPHERE: case HORIZON: pos+=1; break;

	default: return false;
	}

	void *set = (char*)get+pos*sz;
	
	//TODO: try the other callback??

	switch(q[rd]) //set default values
	{
	case FRUSTUM: case EVENT: 
	case SPHERE: case HORIZON: default: return false; 

	case GLOBAL: case STATIC: memset(set,0x00,sz); pos+=1; break; //0

	case SIGNAL: memset(set,0x00,sz*4); pos+=4; //0,0,0,1
		
	if(sz==sizeof(float)) ((float*)set)[3] = 1; else ((double*)set)[3] = 1; break;
	}

	if(pos<get_s) return prescribe(in,q+rd+1,q_s-rd-1,sz,(char*)get+pos*sz,get_s-pos);

	return pos==get_s;
}

//static
Somenviron *Somenviron::open(const Sominstant *in) 
{
	Somenviron *out = new Somenviron;

	Somenviron_inl::editor *ed = new Somenviron_inl::editor;

	out->Somenviron_inl.map = ed;
	out->Somenviron_inl.pov = ed->insert(POV,in);

	return out;
}

Somenviron *Somenviron::admit(const Sominstant *in)
{
	if(!this||!Somenviron_inl) return 0;

	Somenviron *out = new Somenviron;

	out->Somenviron_inl.inherit(Somenviron_inl);
	out->Somenviron_inl.insert(POV,in);
	
	return out;
}

Somenviron *Somenviron::close()
{
	if(!this||!Somenviron_inl||!release()) return 0;
		
	if(--Somenviron_inl->shares==0)
	{
		delete Somenviron_inl.map;
	}

	Somenviron_inl.map = 0; return this;
}

Sominstant *Somenviron::viewer()
{
	return this?(Sominstant*)Somenviron_inl.viewer:0;
}
const Sominstant *Somenviron::viewer()const
{
	return this?Somenviron_inl.viewer:0;
}

Sominstant *Somenviron::cosmos()
{
	return this?(Sominstant*)Somenviron_inl.cosmos:0;
}
const Sominstant *Somenviron::cosmos()const
{
	return this?Somenviron_inl.cosmos:0;
}

bool Somenviron::submit(const Sominstant *i, Sominstant *(Somenviron::*f)())
{
	if(!this) return false;

	if(f==&Somenviron::viewer)
	{
		Somenviron_inl.viewer = i; return true;
	}
	else if(f==&Somenviron::cosmos)
	{
		Somenviron_inl.cosmos = i; return true;
	}
	else return false;
}

Somenviron *Somenviron::addref()
{
	if(!this) return 0;

	Somenviron_inl.refs++;
	
	return this;
}
const Somenviron *Somenviron::addref()const
{
	return const_cast<Somenviron*>(this)->addref()?this:0;
}

Somenviron *Somenviron::release()
{
	if(!this) return 0;
	
	if(--Somenviron_inl.refs>0) return this;
	
	delete this; return 0;	
}
const Somenviron *Somenviron::release()const
{
	return const_cast<Somenviron*>(this)->release()?this:0;
}

int Somenviron::refcount()const
{
	if(!this) return 0;
	
	return Somenviron_inl.refs;
}

template <typename T>
static void Somenviron_prepare(T* &inout, size_t round)
{
	assert(inout==0&&sizeof(T)==sizeof(void*)); 
	
	if(!inout) memset(inout=new T[round],0x00,sizeof(T)*round);
}

template <typename T>
static void Somenviron_reserve(T* &inout, size_t before, size_t after)
{
	if(!inout) return; //defer allocation
	
	assert(sizeof(T)==sizeof(void*)&&after>before); 

	T *cp = new T[after]; memcpy(cp,inout,sizeof(T)*before);

	memset(cp+before,0x00,sizeof(T)*(after-before)); 

	delete [] inout; cp = inout;
}

static int Somenviron_scour(const Sominstant **in, size_t s)
{
	if(!in) return 0; 

	size_t r = rand()%s+1;

	for(size_t i=r;i<s;i++) if(!in[i]) return i;

	for(size_t i=r-1;i>0;i--) if(!in[i]) return i;

	return 0;
}
 
int Somenviron_inl::editor::find(int trap, const Sominstant *in) 
{
	if(!in) return 0;

	Somthread_h::section cs = section;

	hash_it it = hash.find(hash_key(in,trap));

	return it==hash.end()?0:it->second;
}

int Somenviron_inl::editor::insert(int trap, const Sominstant *in)
{	
	if(!in) return 0;

	Somthread_h::section cs = section;

	int out = find(trap,in); if(out) return out;
	
	const Sominstant ***p = 0; size_t *s = 0;

	switch(trap)
	{
	case Somenviron::POV:
	case Somenviron::VIEW: p = &views; s = &views_s; break;
	case Somenviron::FOG:
	case Somenviron::WALL: p = &walls; s = &walls_s; break;
	case Somenviron::SKY:  p = &skies; s = &skies_s; break;	 
	case Somenviron::LAMP: p = &lamps; s = &lamps_s; break;
	}

	if(!p){ assert(0); return 0; }
		
	out = Somenviron_scour(*p,*s);

	if(!out)
	{	
		if(*s==0)
		{
			switch(trap)
			{
			default: *s = 5; break; //conservative

			case Somenviron::LAMP: *s = 33; break; //generous
			}

			Somenviron_prepare(*p,*s);

			out = 1; //0 is reserved
		}
		else
		{
			Somenviron_reserve(*p,*s,*s*2);

			out = *s; *s*=2;
		}
	}

	if(out) 
	{
		(*p)[out] = in;

		int q = Somenviron::ADDED; 
		
		if(!f&&d) d((Sominstant*)in,&q,1,0,0);

		if(f) f((Sominstant*)in,&q,1,0,0);		
	}

	return out;
}
		
void Somenviron_inl::editor::remove(int trap, const Sominstant *in) 
{
	if(!in) return;

	Somthread_h::section cs = section;

	hash_it it = hash.find(hash_key(in,trap));
		
	switch(it!=hash.end()?trap:0)
	{
	case Somenviron::POV:
	case Somenviron::VIEW: views[it->second] = 0; break;
	case Somenviron::FOG: 
	case Somenviron::WALL: walls[it->second] = 0; break;
	case Somenviron::SKY:  skies[it->second] = 0; break; 	 
	case Somenviron::LAMP: lamps[it->second] = 0; break;

	default: assert(0); return;
	}

	int q = Somenviron::REMOVED; 
	
	if(!f&&d) d((Sominstant*)it->first.first,&q,1,0,0);

	if(f) f((Sominstant*)it->first.first,&q,1,0,0);	
}
