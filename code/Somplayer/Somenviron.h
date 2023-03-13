
#ifndef SOMENVIRON_INCLUDED
#define SOMENVIRON_INCLUDED

class Sominstant; //Sominstant.h

class Somenviron 
{
	//// What Somenviron is (and isn't) ///////////
	//
	// Somenviron is _not_ a monolithic environment 
	// from which a Sword of Moonlight game unfolds
	// 
	// What it _is_ is a minimal bridge between the
	// Somtexture and Somgraphic APIs so that a bit
	// of mission critical state information can be
	// communicated to the underlying hardware APIs
	//
	// That said, it can be used for other business
	// and with the help of a persistent Sominstant
	// regime it might get you half way to a proper
	// Sword of Moonlight presentation. But the rub
	// is at this level we don't want to dictate to
	// the user what their Sominstant "regime" will
	// look like. So a callback framework is a must
	//
	// For things to work you will have to derive a 
	// class or two from Sominstant and downcasting
	// is a hard requirement; and therefore you are
	// responsible for seeing that only Sominstants
	// that are compatible with your callbacks will
	// ever be added to the environment. Note: that 
	// a const Somenviron will not accept additions

	Somenviron();	
	Somenviron(const Somenviron&); 
	~Somenviron();

public:

	enum //trappings
	{ 
	//subjective

		POV = 1, FOG = 2, SKY = 4, 
		
	//objective

		VIEW = 8, WALL = 16, LAMP = 32,
	}; 

	enum //qualities
	{	
	//boolean values

		GLOBAL = 1, STATIC = 2, 

	//scalar values 

		SPHERE = 4, //inner radius

		HORIZON = 8, //outer radius
		
	//4D vector values 
		
		SIGNAL = 16, //unit RGBA 

		EVENT = 32, //point vector

	//4x4 matrix values

		FRUSTUM = 64, //projection matrix

	//// book keeping ////////
				
		//sent to a scribe by add / remove (below)
		//in case you need per instance ref counts
		ADDED = -1, REMOVED = -2
	}; 

  /*///////////////// WARNING //////////////////

	where ever you see Sominstant in this file
	it refers to a bounding box matrix, either
	axis aligned or a canoncial viewing volume 
	in the case of the point-of-view instances

  /*////////////////////////////////////////////
	
	//// Maintainer APIs ///////////////
	//add size_t trappings to the environment
	//Before any trappings can be added a callback
	//must be added via ascribe (below) or add will fail
	//
	// Ex. "add(Somenviron::LAMP,lamps,n);" //See overloads below
	//
	bool add(int trapping, const Sominstant**, size_t); 

	//use should be used with subjective trappings:
	//POV, FOG, SKY. An add is implicit when using use
	//It is safe to remove (below) a trapping while in use
	//However which Sominstant* will then be used is undefined
	//when setting to 0 trapping can be a combination of constants 
	bool use(int trapping, const Sominstant**, size_t); 

	//Tip: removal should be avoided whenever possible
	//When setting to 0 the subjective trappings are removed
	//when setting to 0 trapping can be a combination of constants 
	//Important: remove() must be called before close or release (below)
	//unless it is desirable to keep the POV, FOG, SKY, ETC. around for later
	//
	//_multi-threading_
	//Somenviron is optimized for list'ing. It does not guarantee that
	//outstanding Sominstant pointers will not be delete'd if remove'd
	//You should either ensure this won't happen or rely on exceptions
	bool remove(int trapping, const Sominstant**, size_t);

	//will cause STATIC quality to be reevaluated
	//FYI: Helps to call before the Sominstant* moves
	//When setting to 0 the subjective trappings are triggered
	//when setting to 0 trapping can be a combination of constants 
	bool trigger(int trapping, const Sominstant**, size_t);
	
	inline bool add(int trapping, const Sominstant *i)
	{
		return add(trapping,&i,1);
	}
	inline bool use(int trapping=~0, const Sominstant *i=0)
	{
		return use(trapping,&i,1);
	}
	inline bool remove(int trapping=~0, const Sominstant *i=0)
	{
		return remove(trapping,&i,1);
	}
	inline bool trigger(int trapping=~0, const Sominstant *i=0)
	{
		return trigger(trapping,&i,1);
	}

	//reserve room for count trappings
	//(for memory allocation optimization)
	void reserve(int trapping, size_t count);

	//// Interactive APIs /////////////////////////
	//get a map key for the placement of Sominstant
	//The key is opaque but can be used to test for equality 
	//0 is returned if Sominstant is deemed to be outside of the map
	//The key can be passed to list (below) which also defines some additional 
	//special keys which map will never return. The return can be filtered by trappings
	//
	//POV: returns 0 if Sominstant is not in view
	//FOG: returns 0 if Sominstant is not saturated by fog
	//SKY: returns 0 if Sominstant is not beyond the sky fog line
	//
	//LAMP: returns 0 if Sominstant is not lit by lamps (map lights are lamps)
	//
	//Sominstant is cached so that you can call these seperately if you need to
	//Note: only the pointer is compared for cache hits. map(0) should be called 
	//to clear the cache just to be safe whenever the state is expected to change
	int map(const Sominstant*, int trappings=POV)const;

	int top()const; //returns a map key for the entire map or 0 for empty maps
		
	//Technically this returns the number of lamps that are off the map; at most 3
	inline size_t global_illumination(size_t s=3)const{ return count(0,LAMP,s); }

	//map is a map key returned by map (above) and may also be 0 for things without 
	//a definite location or POV for anything located at the same key as the point of view
	//
	//Note: classical FOG and SKY are globals. So set map to 0 instead of POV...
	//
	//VIEW returns an "objective" listing of all POV trappings on the map. Likewise WALL 
	//returns a list of FOG like trappings including FOG trappings. A FOG is a global WALL 
	//
	//When listing lamps set skip equal to global_illumination to skip directional lights
	size_t list(int map, int trapping, const Sominstant**, size_t s, size_t skip=0)const;

	inline size_t count(int map, int trapping, size_t limit=1000)const
	{
		return list(map,trapping,0,limit); 
	}	 	
	inline const Sominstant *point_of_view()const
	{
		const Sominstant *out = 0; list(POV,POV,&out,1); return out;
	}	
	template <typename T>
	inline bool point_of_view_ambient_lens(T *rgba, size_t rgba_s)const //=4
	{
		return describe(0,SIGNAL,rgba,rgba_s); //(below)
	}
	template <typename T>
	inline T point_of_view_horizon(T *out2=0)const
	{
		T out = 0; describe(0,HORIZON,&out,1); return out2?*out2=out:out;
	}	
	
	//describe: retrieve one or more qualities of a trapping
	//i: an instant from list (if 0 the POV instant is assumed)
	//qualities: if a quality is 0 one unit will be skipped over
	//get_s: must be EXACTLY enough to hold qualities back to back
	#define SOMENVIRON_DESCRIBE_(T) \
	\
	inline bool describe(const Sominstant *i, int quality, T *get, size_t get_s)const\
	{\
		return prescribe(i,&quality,1,sizeof(T),get,get_s);\
	}\
	template<int N>\
	inline bool describe(const Sominstant *i, int (&qualities)[N], T *get, size_t get_s)const\
	{\
		return prescribe(i,qualities,N,sizeof(T),get,get_s);\
	}
	//
	SOMENVIRON_DESCRIBE_(float) SOMENVIRON_DESCRIBE_(double) //SOMENVIRON_DESCRIBE_(bool)
	//
	#undef SOMENVIRON_DESCRIBE_

	////TODO: explain callback mechanism in this space /////////////
	//
	// returns the number of qualities successfully written into get
	typedef size_t (*scribe_f)(Sominstant*, int *qualities, size_t, float *get, size_t);
	typedef size_t (*scribe_d)(Sominstant*, int *qualities, size_t, double *get, size_t);
	
private: //You must ascribe (below) one or both scribe callbacks

	//supplies default values and float<->double conversion where necessary
	bool prescribe(const Sominstant*,int*,size_t,size_t,void*,size_t)const;

public: //ascribe: install your scribes
	
	void ascribe(scribe_f, scribe_d=0); 
	
	//render all instances irrevocably close'd
	inline void salt(){ ascribe(0); close(); }

	//Note: Somenviron does not take part in pooling
	//WARNING: pov cannot receive the ADDED callback
	static Somenviron *open(const Sominstant *pov=0); 

	//// Multiple Points of View ////////////////////////////
	//
	// More or less Somenviron can create "subjective" copies
	// of itself. A purely "objective" copy lacks an observer 
	// If you are in a single-player per client scenario then 
	// you probably do not require an objective copy but when
	// there are multiple views (per client) some objectivity
	// may be in order. Like-wise subjective copies allow for
	// multiple views. Which is good for multiplayer and with
	// viewports in editors, and maybe even some cool in game
	// effects or maybe even something an AI opponent can use
	//
	// Bottom line is. Some "trappings" are objective and the
	// rest are subjective. You use admit/close to create and
	// destroy new copies of the environment. Objective stuff
	// stays the same; subjective stuff is at first inherited
	// A "copy" is a bit of a misnomer; it is really a "view"
	// 
	// When you close an environment; it might linger because
	// of outstanding references; however the environ will be
	// lost. All methods will fail except for release (below)
	//
	// The real advantage for pure objectivity is when POV is 
	// used as a filter it is a super point of view that is a
	// combination of every subjective copy that is out there 
	// So for example; you can find out easily when a monster  
	// is not seen by anyone and let its animation go standby
	// to whatever degree ["if a tree falls in the woods..."]
	// 
	Somenviron *admit(const Sominstant *a_new_point_of_view);

	Somenviron *close(); //A release is implicit upon closing
		
	//// MISCELLANEOUS //////////////////////////////////////
	
	//associate a viewer with the environment
	//The viewer is not used by Somenviron internally      
	//but the functionality is provided for its convenience
	//cosmos: intended to be shared by environs of a common origin
	Sominstant *viewer(), *cosmos(); 
	
	const Sominstant *viewer()const, *cosmos()const; 
					  
	//Ex. e->submit(v,&Somenviron::viewer);
	bool submit(const Sominstant*,Sominstant *(Somenviron::*)());

	//// REFERENCE COUNTING /////////////////////////////////
	
	const Somenviron *addref()const; //returns self
	const Somenviron *release()const; //returns self or 0

	Somenviron *addref(); Somenviron *release(); //const

	int refcount()const; //should be used for diagnostic only

	inline const Somenviron *deplete()const //garbage collection
	{
		return refcount()==1?release():this; //ok a valid use
	}

	/*/////// private implementation ///////*/

	#include "Somenviron.inl" //"pimpl"
};

#endif //SOMENVIRON_INCLUDED