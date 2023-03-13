						
#ifndef SOMGRAPHIC_INCLUDED
#define SOMGRAPHIC_INCLUDED

class Somshader;
class Somtexture;
class Sominstant;
class Somenviron;

class Somgraphic 
{		
private:

	Somgraphic(int pool=0);	
	Somgraphic(const Somgraphic&); 
	~Somgraphic();

	//TODO: provide cross pool copy
	//of non-pool state information

public: //readonly

  /*///////////////// WARNING //////////////////

	whenever an object with addref and release
	methods is returned the reference count is
	not incremented; repeat is NOT incremented

  /*////////////////////////////////////////////

	//width*height form a Sominstance* matrix
	//width is set when the graphic is opened
	//height grows as it needs to meet demand
	//width corresponds to intra model pieces
	//height corresponds to in game instances
	//internally the height can only increase
	//NEW: count sums not-collapsed instances

	int width, height, count()const; 	
	
	//_implementation subject to improvement_
	bool collapsed()const{return count()==0;}

	//// Per instance transform array ///////
	//
	// The left column is scaled so to form a
	// bounding box (axis aligned) if it were
	// to be multiplied with a unit cube. The
	// Sominstant appears to be empty but can
	// be used to construct the classic Sword
	// of Moonlight shadow effect. See shadow
	// and scale (both are below) for caveats
	//
	// When "collapsed" the entire row is set
	// aside; essentially ignored during draw
	// enumeration and only time is kept when
	// being animated [Sominstant::collapsed]
	//
	//WARNING: If n is greater than height more instances
	//are automatically generated to make up the difference
	//
	//FYI: we are letting each instance be its own pointer
	//so that applications can derive custom instance classes
	//as they see fit. Somgraphic only knows about the base class
	//of Sominstance.h and is not responsible for managing the memory
	//
	//ro: if true rows are not generated; the return may be less than row_s
	size_t list(Sominstant***rows, size_t rows_s, size_t skip=0, bool ro=false)const;
	
	////WARNING: ro is true by default this time ////
	inline Sominstant **instance(int n, bool ro=true)const 
	{
		Sominstant **out = 0; return list(&out,1,n,ro)?out:0;
	}
	inline Sominstant *instant(int n)const //see place (below)
	{
		const Sominstant *out = 0; if(!place(n,&out,1,'ro')) return 0;

		return const_cast<Sominstant*>(out);
	}
	template<typename T> inline T instant(int n)const
	{
		return dynamic_cast<T>(instant(n));
	}
	
	//IMPORTANT: call fill(n) immediately after opening to guarantee
	//a single memory block is used for holding the width*(n+1) instances
	inline size_t fill(size_t n, Sominstant **set=0, size_t set_s=0, size_t skip=0)const
	{
		Sominstant **row = instance(n,!'ro'); 		
		if(!row||width<=0) return 0; if(!set) return width; if(skip>=width) return 0;		
		memcpy(row+skip,set,sizeof(void*)*(set_s=set_s>width-skip?width-skip:set_s));		
		return set_s;
	}
					   	
	//Per column grouping
	//m: column along width (above)
	//Under different circumstances there 
	//can be a lot of overlap in terms of the 3D
	//reference frames so that numerically some columns 
	//are expected to be equivalent. You can optimize around 
	//this by setting the grouped columns to a common Sominstant*	
	//
	//group(0) belongs to the last 1 based group so that 
	//it also returns the total number of groups present
	//If a model is not animated it will probably return
	//2 for group(0) and 1 until m exceeds width; then 0
	int group(int m)const;  
	
	//Per instance IDs
	//For drawing selection buffers
	//These will be generated automatically
	//Or can be set manually (to accelerate lookups)
	//n: row along height (above)
	int id(int n, int set=-1)const;

	//Per instance scale (0~infinity)	
	//n: offset into the scale array. If out of 
	//bounds 0 is returned. If no scale has yet been set
	//then you will get a communal buffer set to all 1's instead
	//set: if >=0 the scale at n is set accordingly to the value of set and 
	//the nth instance is updated. By default MSM and MPX file formats are always 1
	const double *scale(int n=0, double set=-1)const;

	//Set placement of placements_s instances begininning with n
	//
	//n: designates the instance (per height) to commence placement
	//placements: an array of orthonormalized matrices that are to be
	//adjusted and then copied into the "left column" (explained above) 
	//Adjustments include scaling per the graphics bounding box scaled by
	//the provided scale (above) factor and recentering of the bounding box
	//
	//Note: the intra-model pieces (per width) are marked to be updated if
	//accessed [by list (above) or indirectly by channel (below) for example]
	//
	//ro: if true the left column pointers are copied into the placements array
	//Note: reading the left column this way does not trigger a width wide update 
	//	
	//The return is the number of instances placed (0 pointers are skipped but count)
	size_t place(int n, const Sominstant **placements, size_t placements_s, bool ro=false)const;
		
	//Per instance opacity (0~1)
	//This is expected to be used to implement
	//Sword of Moonlight's fade in / fade out effect		
	//ro: if true memory is not allocated (0 is returned)
	//Currently 0 is returned for the MSM and MPX file formats
	//Tip: sometimes opacity is used to allow users to mask a thing
	//that is in the way of some other thing that is needed to be clicked
	double *opacity(bool ro=false)const;

	//Per instance shadow (0~infinity)
	//for classical Sword of Moonlight shadows
	//You can set shadows on or off with the map
	//editor that comes with SOM. So on would be 1
	//and off would be 0 (an extension might be used
	//to facilitate unlimited scale factors in theory)
	//ro: if true memory is not allocated (0 is returned)	
	//By default 0 is returned if the model is not animated
	double *shadows(bool ro=false)const;
		
	enum{ RGB_VS=-1, RGB_PS=-2 }; //GPU shaders

	enum{ INDEX=4, RGB=8, XYZ=512 }; //Somtexture.h

	//Enumerate reference strings. Returns 0 when done
	const wchar_t *reference(int number, int type=RGB)const; 

	//See shaders (below) to get the legacy texture numbers
	//FYI: A "legacy texture" is the colour texture that is not a multi-texture
	const Somtexture *reference_texture(int number, int type, bool ro=false)const; 

	//Type must be RGB_VS or RGB_PS (shaders are per legacy texture)
	//ro: if false the shader is automatically assembled (if it is not already)
	const Somshader *reference_shader(int number, int type, bool ro=false)const; 	

	//// Animation /////////////////////////////////////////////
	//
	// Somgraphic will keep track of time, or if you don't trust
	// it, you can clock things yourself. Instances that are not
	// "collapsed" are updated (if dirty) when list (above) gets
	// called. Same deal for channel (below) just for the record
	
	enum{ PAUSE=1, RESUME=2, REPEAT=4, REWIND=8 }; //play mode flags

	bool play(int n, int id=-1, int mode=0, double time=0)const; //playback

	size_t pose(int n, int id[], double t[], size_t t_s=1)const; //feedback	
		
	bool step(int n, int id, double time)const; //best in PAUSE mode

	inline bool step(int n, double time)const{ return step(n,-1,time); }

	bool stop(int n, int id=-1, double time=0)const; //time: to stop
			
	//Obtain the animation ID list and or side by side runtimes table
	//Returns the size of the list, which will be 1 more if timetable
	//The first entry in the timetable holds a universal scale factor
	//The first ID in listing will be -1 to line up the IDs and times
	size_t playlist(const int **listing, const double **timetable)const;

	//// INSTANTIATION /////////////////////////////////////////
	//
	// Unlike Somtexture a graphic is not fully opened until it
	// is "linked". See the link (below) comments for more info
	// NEW: its recommended you do fill(desired_height) (above)
	// first thing soon after a graphic is open'd and or link'd
	//
	// edit: Somgraphic does not have separate read/write modes
	// as does Somtexture. Opening the graphic using edit tells
	// the graphic to use Somtexture::edit to open its textures

	//pool: if 0 a new pool (video adapter interface) is created	
	//file: commandline (a 0 returns an empty graphic container)
	//file_s: sizeof file if nonhuge, else 0 termination assumed		
	static const Somgraphic *open(int pool, const wchar_t *file, size_t file_s=-1UL);
	static const Somgraphic *edit(int pool, const wchar_t *file, size_t file_s=-1UL);

	int pool()const; //returns pool id

	//Create a new graphic from the same pool
	inline const Somgraphic *open(const wchar_t *file, size_t file_s=-1UL)const
	{
		return Somgraphic::open(pool(),file,file_s);
	}
	inline const Somgraphic *edit(const wchar_t *file, size_t file_s=-1UL)const
	{
		return Somgraphic::edit(pool(),file,file_s);
	}
	
	bool readonly()const; //true if not opened with edit (above)

	//link: link_s Somtextures; by default colour textures are hidden
	//when a graphic is first opened so that the textures can be linked
	//ro: if true the textures in use are copied into link (w/out addref)
	//
	//Use reference (above) To figure out the count and order of the textures	
	//If a link slot is nonzero then its ref count gets incremented and it is used	
	//If a texture has already taken the slot then that texture will first be released
	size_t link(const Somtexture **link=0, size_t link_s=0, int type=RGB, bool ro=false)const;

	//Re-open graphic (non colour textures are discarded)		
	//FYI: you must use link (above) in order to purge textures
	//
	//IMPORTANT: before refreshing a graphic recall that you are responsible
	//for managing the instances memory. If the refresh causes the new width 
	//to be less than the previous width the instance arrays are not cropped
	//to be less than the previous width. You can use this time for recovery
	//
	//edit: if true readonly status is _inverted_ [you're right; it is confusing!]
	bool refresh(const wchar_t *file=0, size_t file_s=-1UL, bool edit=false)const;

	//// REFERENCE COUNTING /////////////////////////////////
	
	const Somgraphic *addref()const; //returns self
	const Somgraphic *release()const; //returns self or 0

	int refcount()const; //should be used for diagnostic only

	inline const Somgraphic *deplete()const //garbage collection
	{
		return refcount()==1?release():this; //ok a valid use
	}
	
	//// INSTANCE BASED RENDERING ////////////////////////

	//multi-thread read/write lock: frozen graphics cannot
	//move until thawed so to prevent seams from appearing
	//If freeze returns true thaw should be called or else
	//the graphic will remain visually frozen. If graphics
	//freezing up becomes a problem Somgraphic may monitor
	//for such events as being unthawed is purely cosmetic
	//freeze is true when successful; you should call thaw
	//thaw is only true if the graphic is no longer frozen 
	bool freeze()const, frozen()const, thaw()const; 

	//// render-to-texture APIs //////////////////////////
	//
	// These methods are expected to be used by Somtexture
	//
	// *NOTE: a "channel" in this context is any subset of 
	//  instances which are deemed suitable for instancing
	//
	// *Important: if the first Sominstant in a row is not
	//  "collapsed" then the instance should be enumerated
	//
	// *Disclaimer: there is no reason that these couldn't
	//  be one big grand unified function call except that
	//  it would look pretty unwieldy. There would be less
	//  call overhead that way (but optimization can wait)
	//
	// *NEW: ch is now a pointer so it will be incremented
	//  whenever a channel is skipped over or merged. This
	//  way you can simply call channel until it returns 0 
	//  When ch is 0 the method will succeed if the result 
	//  is uniform across every single one of the channels 
			
	//coop optimization: filter results of channel (below)
	//In short a per row mask is built by running the left
	//column of each row through Somenviron::map. If 0 the 
	//mask is cleared. size_t counts the number of passing
	//instances [if no instances pass the mask is cleared]
	size_t fence(const Somenviron*, int trappings=0)const;
	
	//Enumerate render channels (i receives n*width+m indices for tween/opacity)
	size_t channel(int ch[1], const Sominstant**, int *i, size_t s, size_t skip)const;
	//Retrieve per render channel textures (i is reserved for per texture stuff)
	size_t texture(int ch[1], const Somtexture**, int *i, size_t s, size_t skip)const;
	//Retrieve per channel vertex buffer texture (and the two tweening textures)
	const Somtexture *model(int ch[1], const Somtexture**t, size_t, size_t skip)const;
	//Retrieve per channel index buffer and range (and the vertex caching range) 
	const Somtexture *index(int ch[1], size_t range[2], size_t cache[2], size_t)const;
	
	const double *tween()const; //Per instance "t" (0~1)

	enum{ MDO=1, MDL=2, MPX=3, MSM=MDO };

	//per vertex buffer format
	//Returns one of the enum constants above	
	//MDO: float[3] vertex, float[3] normal, float[2] uv
	//MDL: short[4] vertex, short[4] normal, octet[4] uv
	//MPX: float[3] vertex, octet[4] colour, float[2] uv
	//Notes: the 4th shorts in MDL contain an index back
	//to the original vertex/normal in the MDL file, and
	//the last 2 octets of uv store a short index to the
	//face primitive from which the UV coords were taken
	int vertex(int m=-1)const; 

	inline const wchar_t *vertex_string(int m=-1)const
	{
		switch(vertex(m))
		{
		case MDO: return L"MDO"; case MDL: return L"MDL"; 
		case MPX: return L"MPX"; default: return 0;
		}
	}

	//Retrieve legacy blending model
	//Basically this is just to get at the blend/material
	//numbers after an MSM, MDO, MDL/TMD file is opened 
	//
	//factors: first 4 floats are the diffuse colour components
	//And the last 4 floats are the emissive colour components
	//These default to 1,1,1,1,0,0,0,0 for MSM, MDL, and TMD
	//
	//A blend code is returned; if 0 the graphic is opaque...
	//However if factors[3] is not 1.0 alpha is still in play
	//
	//If bit 1 is 0, then MDO blending codes are used
	//If bit 1 is 1, then MDL blending codes are used
	//
	//Shift the returned code down 1 bit to get the integer
	//pulled from the file itself. Consult the specification
	//for each format for the understanding of each blend code
	int blendmode(int m, float factors[8]=0)const;

	//Retrieve legacy material index for m
	//If edit is greater than the number of materials
	//a new material will be added, however its index may
	//not be equal to edit. The new index is returned; so that
	//edit can easily be set to a maximum permitted material index
	//Use blendmode (above) to change the colours of materials
	//WARNING: materials can only be saved by MDO files
	int material(int m)const;

	//Retrieve legacy texture index for m
	//(shaders are customized on a per texture basis)
	int shaders(int m)const;

	enum{ BLIT=1, UNLIT=2, SPRITE=3, BLENDED=4, PROFILES };

	//Shader profile (these are built in)
	//Returns one of the enum constants above	
	//BLIT is for on screen elements, menus, HUD, etc.
	//UNLIT is for map tiles mainly (w/ vertex colour)
	//SPRITE is for 3D sprites, including billboards
	//BLENDED is for lit models (everything else)	
	int profile(int m=-1)const; 

	inline const wchar_t *profile_string(int m=-1)const
	{
		switch(profile(m))
		{
		case BLIT: return L"blit";
		case UNLIT: return L"unlit"; 
		case SPRITE: return L"sprite"; 
		case BLENDED: return L"blended"; default: return 0;
		}
	}

	//// EDITING ///////////////////////////////////////////
	//
	// When readonly (above) returns true these return false
		
	bool re(int,int (Somgraphic::*)(int)const,int)const;

	bool re2(int,int (Somgraphic::*)(int,float*)const,int,float*)const;

	inline bool remodel(int r, int (Somgraphic::*f)(int)const, int a)const
	{
		return re(r,f,a); //group, vertex, material, shaders, profile
	}
	inline bool remodel(int r, int (Somgraphic::*f)(int,float*)const,int a, float *b)const
	{
		return re2(r,f,a,b); //Ex. g->remodel(1<<1|0,&Somgraphic::blendmode,m,0);
	}

	//// SECTIONING ////////////////////////////////////////
	//
	// King's Field games have in the past featured monsters
	// with sections of their body that can be independently 
	// bested. These are an extension for Sword of Moonlight
	// and should not be confused with models that are built
	// up of many separate models that are attached together
	
	//Only a top-level graphic is able to enumerate sections
	//A value of 0 can return a graphic that is functionally
	//equivalent to the first but unable to enumerate itself
	//
	//When a graphic is refreshed sections are not truncated
	//When a reference is addref'd or release'd it is toward
	//the top graphic's reference total (See bisect (below))
	const Somgraphic *section(int)const;

	//Separate a graphic into two groups of sections so that
	//one or the other can be released. In theory you can do
	//other kinds of stuff with this; but it was designed to
	//release any "phantom" sections after a refresh (above)
	const Somgraphic *bisect(int)const;				 
		
	//// ANNOYANCES ////////////////////////////////////////

	template<class T> size_t list(T rows, size_t rows_s, size_t skip, bool ro=false)const
	{
		if(0) Sominstant *compile = static_cast<Sominstant*>(**rows); 

		return list((Sominstant***)rows,rows_s,skip,ro);
	}
	template<class T> size_t fill(size_t rows_s, T set, size_t set_s)const
	{
		if(0) Sominstant *compile = static_cast<Sominstant*>(*set); 
																	   
		return fill(rows_s,(Sominstant**)set,set_s);
	}
	template<class T> size_t place(int n, T placements, size_t placements_s, bool ro=false)const
	{
		if(0) const Sominstant *compile = static_cast<const Sominstant*>(*placements); 

		return place(n,(const Sominstant**)placements,placements_s,ro);
	}
	template<class T> size_t channel(int ch, T out, int *i, size_t s, size_t skip)const
	{
		if(0) const Sominstant *compile = static_cast<const Sominstant*>(*out); 

		return channel(ch,(const Sominstant**)out,i,s,skip);
	}

	/*/////// private implementation ///////*/

	#include "Somgraphic.inl" //"pimpl"
};

#endif //SOMGRAPHIC_INCLUDED