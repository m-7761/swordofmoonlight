					 
//private implementation
#ifndef SOMENVIRON_INCLUDED
#define SOMENVIRON_INTERNAL

class Sominstant;

struct Somenviron_inl
{	
	struct editor 
	{  	
		Somthread_h::account shares; 
		
		size_t views_s, walls_s, skies_s, lamps_s;
				
		const Sominstant **views, **walls, **skies, **lamps; 

		size_t (*f)(Sominstant*,int*,size_t,float*,size_t);
		size_t (*d)(Sominstant*,int*,size_t,double*,size_t);

		bool salted; //should be true when !f&&!d
		
		//Somenviron.cpp		
		int insert(int, const Sominstant*);
		void remove(int, const Sominstant*);
		int find(int, const Sominstant*);
		
		editor() : shares(1) //refs
		{			
			views = walls = skies = lamps = 0; 
			
			views_s = walls_s = skies_s = lamps_s = 0;

			f = 0; d = 0; salted = !f&&!d;
		}
		~editor()
		{
			delete [] views; delete [] walls;
			delete [] skies; delete [] lamps;			
		}

	private: Somthread_h::intersect section; 

		/*TODO: KEEP USAGE STATS FOR THE SUBJECTIVE KEYS*/
		typedef std::pair<const Sominstant*,int> hash_key;	 		
		typedef std::map<hash_key,int>::iterator hash_it;

		std::map<hash_key,int> hash; 

	}*map;

	inline editor *operator->(){ return map; }	

	inline operator bool(){ return map&&!map->salted; }
		
	int pov, fog, sky; //classical fog sky model

	const Sominstant *pov_s, *fog_s, *sky_s; //debug?

	//subjective access (defined below)
	inline const Sominstant *operator[](int);
	
	//subject insertion (defined below)
	inline void insert(int, const Sominstant*);
	inline void remove(int);

	//Somenviron::viewer
	const Sominstant *viewer, *cosmos; 

	Somthread_h::account refs; 

	Somenviron_inl()
	{
		memset(this,0x00,sizeof(*this)); 
		refs = 1;
	};									  
	~Somenviron_inl()
	{
		if(map&&--map->shares==0) delete map;
	}	

	inline void inherit(Somenviron_inl &cp)
	{
		assert(refs==1); *this = cp; refs = 1; //hack
				
		cp->shares++;
	}
};

#include "Somenviron.h"

inline const Sominstant *Somenviron_inl::operator[](int trap)
{
	switch(trap)
	{
	case Somenviron::POV: return pov&&pov_s==map->views[pov]?pov_s:0; 
	case Somenviron::FOG: return fog&&fog_s==map->walls[fog]?fog_s:0; 
	case Somenviron::SKY: return sky&&sky_s==map->skies[sky]?sky_s:0; 

	default: assert(0); return 0; //unimplemented??
	}
}

inline void Somenviron_inl::insert(int trap, const Sominstant *in)
{
	int ins = map->insert(trap,pov_s=in);

	switch(trap)
	{
	case Somenviron::POV: pov = ins; break;
	case Somenviron::FOG: fog = ins; break;
	case Somenviron::SKY: sky = ins; break;

	default: assert(0); //not good
	}
}

inline void Somenviron_inl::remove(int trap)
{
	map->remove(trap,operator[](trap));

	switch(trap)
	{
	case Somenviron::POV: pov = 0; break;
	case Somenviron::FOG: fog = 0; break;
	case Somenviron::SKY: sky = 0; break;

	default: assert(0); //not good
	}
}

#else
#ifdef SOMENVIRON_INTERNAL //"pimpl"//
	
	mutable Somenviron_inl Somenviron_inl;

#endif //SOMENVIRON_INTERNAL
#endif //SOMENVIRON_INCLUDED