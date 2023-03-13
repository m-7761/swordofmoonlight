  
#ifndef SOMPAINT_VERSION
#define SOMPAINT_VERSION 0,0,0,2
#define SOMPAINT_VSTRING "0, 0, 0, 2"
#define SOMPAINT_WSTRING L"0, 0, 0, 2"
#endif

#ifndef SOMPAINT_INCLUDED
#define SOMPAINT_INCLUDED

/*C linkage to types bool etc is left as 
//an exercise to whoever wants to link it!*/

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#pragma push_macro("_")
#ifndef __cplusplus
#define _(default_argument)
#else
#define _(default_argument) = default_argument 
#endif	

#define SOMPAINT_LIB(x) Sword_of_Moonlight_Raster_Library_##x

#ifdef SOMPAINT_API_ONLY
#pragma push_macro("SOMPAINT")
#pragma push_macro("SOMPAINT_PAL")
#pragma push_macro("SOMPAINT_PIX")
#pragma push_macro("SOMPAINT_BOX")
#endif

/*modules export these*/
#ifndef SOMPAINT_MODULE_API
#define SOMPAINT_MODULE_API SOMPAINT_API
#else
#define SOMPAINT_API
#define SOMPAINT_NO_THUNKS
#endif

#ifdef SOMPAINT_DIRECT
#define protected public
#endif
					 
struct SOMPAINT_LIB(server);
#define SOMPAINT struct SOMPAINT_LIB(server)*

struct SOMPAINT_LIB(buffer); //video mem
#define SOMPAINT_PAL struct SOMPAINT_LIB(buffer)*

struct SOMPAINT_LIB(raster); //OS handle
#define SOMPAINT_PIX struct SOMPAINT_LIB(raster)*

struct SOMPAINT_LIB(window); //rectangle
#define SOMPAINT_BOX struct SOMPAINT_LIB(window)*

struct SOMPAINT_LIB(server) 
{
#ifndef __cplusplus 
	 /*probably should not rely on this working
	//Use the SOMPAINT_LIB(Status) API instead*/
	void *vtable; /*debugger*/
#endif

	int status;
	
	float progress; //0~1

	const wchar_t *message; 

#ifdef __cplusplus

	SOMPAINT_LIB(server)()
	{
		status = 0; message = L""; progress = 0.0;
	}

	/*0: Normal idling status
	//Busy at work if greater
	//Fatal error if negative*/
	inline operator int(){ return this?status:-1; }

	/*the device is lost and cannot reset
	//should only occur if expose is used*/
	inline bool lost(){ return this?status==-2:false; }

	/*compiler: MSVC2005 has trouble calling 
	//virtual members from an object reference*/
	inline SOMPAINT operator->(){ return this; }
	
	/*See struct SOMPAINT_LIB(buffer)*/
	virtual SOMPAINT_PAL buffer(void **io)=0;

	template<typename T>
	inline SOMPAINT_PAL pal(T *t){ return buffer(&t->pal); }
	template<typename T>
	inline SOMPAINT_PAL pal(T &t){ return buffer(&t.pal); }

	/*See C API comments before using*/
	virtual bool share(void **io, void **io2)=0;	
	virtual bool format2(void **io, const char*,va_list)=0;
	virtual bool source(void **io, const wchar_t[MAX_PATH])=0;
	virtual bool expose2(void **io, const char*,va_list)=0;
	virtual void discard(void **io)=0;

	inline bool format(void **io, const char *f,...)
	{
		va_list v; va_start(v,f); bool o = format2(io,f,v); va_end(v); return o;
	}
	inline bool expose(void **io, const char *f,...)
	{
		va_list v; va_start(v,f); bool o = expose2(io,f,v); va_end(v); return o;
	}

	virtual void *lock(void **io, const char *mode, size_t inout[4], int plane=0)=0;
	virtual void unlock(void **io)=0; 

	virtual const wchar_t *assemble(const char *code, size_t codelen=-1)=0;

	virtual bool run(const wchar_t*)=0;		
	virtual int load2(const char*,va_list)=0;
	virtual int load3(const char*,void**,size_t)=0;

	inline int load(const char *f,...)
	{
		va_list v; va_start(v,f); int o = load2(f,v); va_end(v); return o;
	}

	virtual void reclaim(const wchar_t*)=0;		

	/*C-like preprocessor macros APIs*/
	virtual void **define(const char *d, const char *def="", void **io=0)=0;		
		
	inline void **define(const char *d, int e, void **io=0)
	{
		char def[64]=""; if(sprintf(def,"%d",e)) return define(d,def,io);
	}
	inline void **define(const char *d, double e, void **io=0)	
	{
		char def[64]=""; if(sprintf(def,"%f",e)) return define(d,def,io);
	}
	inline void  **undef(const char *d, void **io=0)
	{
		return define(d,0,io);  	
	}

	virtual const char *ifdef(const char *d, void **io=0)=0;

	/*portable abstraction layer APIs*/
	virtual bool reset(const char *unused=0)=0;	
	virtual bool frame(size_t inout[4])=0; 
	virtual bool clip(size_t inout[4])=0; 
	  
	/*portable operating systems APIs*/
	virtual SOMPAINT_PIX raster(void **io, void *local=0, const char *typeid_local_raw_name=0)=0;	
	virtual SOMPAINT_BOX window(void **io, void *local=0, const char *typeid_local_raw_name=0)=0;	

	template<typename T> 
	inline SOMPAINT_PIX raster(void **io, T *local)
	{
		return raster(io,local,typeid(T).raw_name()); 
	}
	template<typename T>
	inline SOMPAINT_BOX window(void **io, T *local)
	{
		return window(io,local,typeid(T).raw_name()); 
	}

protected: /*See C API comments*/
	
	/*disconnect should return 0 unless an 
	//alternative unload protocol needs to
	//be used within the build environment*/
	virtual void *disconnect(bool host)=0;

#endif
};

#ifndef SOMPAINT_API 
#define SOMPAINT_API static 
#include "../Somversion/Somversion.hpp" 
#define SOMPAINT_API_FINALE(type,API,prototype,arguments) \
SOMVERSION_HPP_INTERFACE(Sompaint,type,SOMPAINT_LIB(API),prototype,arguments)
#elif !defined(SOMPAINT_API_FINALE)
#define SOMPAINT_API_FINALE(...) ;
#endif

/*////FORWARD//////////////////////////////////////
//
// For the most part the intention is to facilitate
// a write-only API. The only legitimate source for
// read back is Lock (below) and probably some high
// level stuff concerning synthesization of shaders
//
// There are no facilities for resources management
// except for time sharing around overlapping locks 
//
// The ultimate objective is to render the prospect 
// of implementing and maintaining back-end modules
// as painless as can be barring compromises at the
// cost of platform neutrality and interoperability
//
// Of course we are only targeting a subset of most
// any commercial API. D3D11 is a beast but somehow
// the video games still look like they always have
*/

SOMPAINT_MODULE_API /*nonzero on success
//party: to connect to or 0 for your process' default 
//if party is 0 connection is "modal" ortherwise "modeless"
//v can be 0 but it should be the same as the SOMPAINT_VERSION macro above*/
SOMPAINT SOMPAINT_LIB(Connect)(const wchar_t v[4], const wchar_t *party _(0))
SOMPAINT_API_FINALE(SOMPAINT,Connect,(const wchar_t[4],const wchar_t*),(v,party))

#ifndef __cplusplus
/*This is for C compatability. Because of the vtable strict C code
//should use this API to get a working pointer to the data members (status, etc.)*/
SOMPAINT_API SOMPAINT SOMPAINT(Status)(SOMPAINT c)
SOMPAINT_API_FINALE(SOMPLAYER,Status,(SOMPAINT),(c))
#endif

SOMPAINT_MODULE_API
/*SOMPAINT is delete'd on Disconnect (so don't use it)
//host: "disconnect" all parties when SOMPAINT is host
//
//_Microsoft Visual C/C++_
//Modules implementing disconnect call FreeLibraryAndExitThread
//from inside a newly created thread so that it isn't necessary
//to implement an additional layer just for calling FreeLibrary
//
//Modules implementing Disconnect (with a D) do not FreeLibrary*/
void SOMPAINT_LIB(Disconnect)(SOMPAINT c, bool host _(false))
SOMPAINT_API_FINALE(void,Disconnect,(SOMPAINT,bool),(c,host))

#ifndef SOMPAINT_NO_THUNKS /*hide C-like API from Somversion.hpp?*/

SOMPAINT_API /*nonzero on success
//Associates a resource with a source identification string*/
bool SOMPAINT_LIB(Source)(SOMPAINT c, void **io, const wchar_t id[MAX_PATH])
SOMPAINT_API_FINALE(bool,Source,(SOMPAINT,void**,const wchar_t[MAX_PATH]),(c,io,id))

SOMPAINT_API /*guaranteed nonzero
//IMPORTANT: io is the argument itself; the pointer must persist
//until Discard (below) is called with io supplied as its argument
//io: if 0 the SOMPAINT_PAL of the default frame buffer is returned
//The 0 buffer defaults to 512x512 with 1 colour back buffer without
//any accessory buffers/planes. Formatting the 0 buffer might require 
//the entire device instance to be reset (it does so with D3D9 anyway)
//Note: A shared "discard" buffer is returned for unformatted buffers*/
SOMPAINT_PAL SOMPAINT_LIB(Buffer)(SOMPAINT c, void **io)
SOMPAINT_API_FINALE(SOMPAINT_PAL,Buffer,(SOMPAINT,void**),(c,io))

SOMPAINT_API /*nonzero on success
Share a buffer. Note that a buffer is not truly lost until
all shares of the buffer have discarded it with Discard (below)
Note: sharing is only enforced by Share, Discard, Lock, and Unlock*/
bool SOMPAINT_LIB(Share)(SOMPAINT c, void **io, void **io2)
SOMPAINT_API_FINALE(bool,Share,(SOMPAINT,void**,void**),(c,io,io2))

SOMPAINT_API /*nonzero on success 
//The buffer/raster/window object is deleted. A dummy is returned
//For the record a server discards all buffers etc. on Disconnect (above)*/
void SOMPAINT_LIB(Discard)(SOMPAINT c, void **io)
SOMPAINT_API_FINALE(void,Discard,(SOMPAINT,void**),(c,io))

SOMPAINT_API /*nonzero on success; fails if bad status
//Format a buffer (the format string follows the printf spec)
//The format statement has two parts. On the left the type of buffer
//is declared (note: format may not always be limited to buffers however
//it is for now) and on the right a series of format functions can be invoked
//If the buffer was previously formatted then the left (and colon) can be omitted
//in order to retain the previous formatting (if not omitted the slate is wiped clean)
//
// Ex. "basic buffer: 2d(256,256,32), canvas, mipmap(0,2);" //case sensitive 
//
//Instead, basic can be one of: state, frame, point, or index (see PAL section)
//
//Instead of buffer, "buffer" can be "memory" in order to indicate an offline
//or non-resident scratch (ie. system only) memory buffer (eg. basic memory:)
//memory buffers are streamed (if not already cached) to the device on demand
//
//Alternatively, basic can be macro for C-like CPP macros (eg. macro buffer:)
//
//Functions: default values are assigned from right to left
//           and to white space where an argument should be
//           identifiers can take any form %:=(,); excluded
//           Specializations are assigned by the = operator
//
//_macro buffer_
//ascii: text encoding (this is the default)
//
//_basic buffer_
//canvas: requests a basic / frame buffer hybrid
//mipmap: requests a texture with mipmaps. Argument 1 specifies
//the shallowest mip level desired and argument 2 specifies the deepest level
//
//_basic/point/index buffer_
//1d: specifies the number of texels (vertices/indices) and the bits per each
//Bits must be a multiple of 8. Indices are 32 or less and may get rounded up
//
//_basic/frame buffer_
//2d: specifies width, height, and the bits per pixel (sum the planes)
//rgb: adds a colour plane and specifies the desired number of colour bits
//rgba: adds a colour plane with the alpha channel enabled. Same args as rgb
//clear: sets the Clear (below) values for each component of each plane present
//in the order of colour (4 values) depth (1 value) and stencil (1 value per bit)
//The values are in the units of 0,1 and will probably be clamped if they go outside
//... can be provided as the last argument to set all remaining values equal to the one
//before. The defaults are always 0; _except_ for depth planes, which default to 1
//
//Swizzling:
//Present and Fill (below) must implement the following for basic buffers  
//rgb[a] = aaa; for packing greyscale information into a spare alpha channel 
//rgb[a] = bgr; for overlaying Win32 colour layouts (eg. COLORREF/PALETTEENTRY)
//Switching between these modes should not incur a reallocation of buffer memory
//
//_frame buffer_
//depth: adds a depth plane and specifies the desired number of depth bits
//stencil: adds a stencil plane and specifies the desired number of stencil bits
//Note that if no planes are specified the default behavior is to add a colour plane
//(the default colour format for a frame buffer is rgb; a basic buffer defaults to rgba)
//x: specifies the number of colour back buffers. If unspecified a render target is assumed
//fullscreen: put the buffer in "fullscreen" mode. An argument can supply the video port number
//
//Note: colour, depth, and stencil plane bits may differ (you can Lock a plane and see)
//
//Note: a shared depth buffer will be utilized if needed: see Setup (below) for details
//
//Note: the behavior of x is restricted to the limits of Direct3D9 until further notice
//If x is 0 or 1 the buffer is double buffered with copy semantics. When greater than 1
//x becomes the number of back buffers with discard semantics. This should imply triple 
//buffering; but may actually result in double buffering. The number of back buffers is
//clipped to the maximum number supported and or permitted by the device and its driver
//
//State Buffers:
//
//A state buffer tracks "fixed function pipeline" graphics states 
//The states may be redundant with respect to the programmable pipeline
//or they may not. Therefore a client should usually implement both strategies
//Each function sets a state and establishes the tracking of that state by the buffer
//
//The syntax to disable the tracking of a state looks like the following:
//
// Ex. "blend = 0;" //probably unnecessary since no state is tracked by default
//
//The syntax is technically a functional specialization (where 0 is a special case)
//
//In addtion to 0, reset configures a state to its behavior after Reset:
//
// Ex. "z = reset;" //so Apply (below) would turn off Z buffer read/write
//
//_state buffer_
//blend: the first two arguments are the source / destination blend functions
//followed by four optional RGBA blend factors. A formal specification for the 
//functions has not been developed; something like sA, 1-sA, is recommended. For 
//now only 1 and 0 are required to be implemented. So that blend(1,0) effectively 
//disables blending. blend(1,1) is the default blend mode. With other combinations 
//being effectively useless. And instead make due with the following:
//
// Ex. "blend = mdo(1);" Ex. "blend = mdl(3);" //Sword of Moonlight's codes
//
//z: the two arguments are the depth write range between 0 and 1. The default
//behavior (for which there are no defined symbols) is to disable depth write
//The comparison function passes when the source is less than the destination
//A "decal" specialization should be implemented for less than or equal tests
//A "range" specialization should be implemented for write-only without tests
//
// Ex. "z = decal(0,1);" //less than or equal Z function with writing enabled
// Ex. "z = range(0,1);" //no contest (so no Z function) with writing enabled
//
//front: selects which face of triangles to Draw. front for clockwise winding
//and back for counter clockwise, and both (or reset) for both sides:
//
// Ex. "front = back;" //draw only the back facing triangles
// Ex. "front = both;" //draw both the back and front facing triangles
//
//Format returns 0 if a buffer was not created or every function fails
//Note that failure for state functions is not necessarily fatal since
//the features may be accessible within the programmable pipeline only
//
//io: if 0 the SOMPAINT_PAL of the default frame buffer is formatted
//(defaults for the default buffer is 512x512 without a depth plane)*/
bool SOMPAINT_LIB(Format2)(SOMPAINT c, void **io, const char *fstring, va_list courtesy)
SOMPAINT_API_FINALE(bool,Format2,(SOMPAINT,void**,const char*,va_list),(c,io,fstring,courtesy))
static bool SOMPAINT_LIB(Format)(SOMPAINT c, void **io, const char *fstring,...)
{
	va_list va; va_start(va,fstring);
	bool out = SOMPAINT_LIB(Format2)(c,io,fstring,va); va_end(va); return out;
}

SOMPAINT_API /*nonzero on success
//Enable limited cooperation with the underlying implementation
//
//Expose is essentially an implementation dependent back door. There are
//no limitations placed upon its usage and likewise there are no guarantees
//
// Ex. "Expose(0,"IDirect3DDevice9*",&d3dd9); " //get at the device pointer
*/ 
bool SOMPAINT_LIB(Expose2)(SOMPAINT c, void **io, const char *fstring, va_list inout)
SOMPAINT_API_FINALE(bool,Expose2,(SOMPAINT,void**,const char*,va_list),(c,io,fstring,inout))
static bool SOMPAINT_LIB(Expose)(SOMPAINT c, void **io, const char *fstring,...)
{
	va_list va; va_start(va,fstring);
	bool out = SOMPAINT_LIB(Expose2)(c,io,fstring,va); va_end(va); return out;
}

SOMPAINT_API /*nonzero on success; fails if bad status 
//mode: one of "r", "w", or "w+" (read/write follows the fopen specification)
//inout: on the way in inout specifies the left, top, right, bottom region to lock
//on the way out inout specifies the bit offset, bit width, byte stride, and byte pitch
//plane: it is only possible to lock one plane at a time. Specify the 0 based plane to lock
//Note: each io can only lock one region at a time. But Share (above) can permit multi-locking
//Set mode to 0 to query inout even if r/w is impossible. 1 is returned. Unlock is still required
//Reminder: plane can double for the purpose of locking mipmaps if doing so is ever deemed necessary*/
void *SOMPAINT_LIB(Lock)(SOMPAINT c, void **io, const char *mode, size_t inout[4], int plane _(0))
SOMPAINT_API_FINALE(void*,Lock,(SOMPAINT,void**,const char*,size_t[4],int),(c,io,mode,inout,plane))

SOMPAINT_API 
void SOMPAINT_LIB(Unlock)(SOMPAINT c, void **io) /*assuming self explanatory*/
SOMPAINT_API_FINALE(void,Unlock,(SOMPAINT,void**),(c,io))

SOMPAINT_API /*nonzero on success 
//Assemble a raster program (shader) into a handle
//If the returned handle is not an empty string then there were
//errors in the ASM code that will be returned. If a null pointer is returned
//then there was some error outside of the scope of the source code*/
const wchar_t *SOMPAINT_LIB(Assemble)(SOMPAINT c, const char *code, size_t codelen _(-1))
SOMPAINT_API_FINALE(const wchar_t*,Assemble,(SOMPAINT,const char*,size_t),(c,code,codelen))

SOMPAINT_API /*nonzero on success; fails if bad status
//Load a handle generated by Assemble into program memory
//If the program is already loaded then it will not be re-loaded
//but it will be brought into the foreground. If a program of the same
//type is already running, then that program is replaced. The program in the 
//foreground determines which registers (or memory) will be written to by Load...*/
bool SOMPAINT_LIB(Run)(SOMPAINT c, const wchar_t *prog)
SOMPAINT_API_FINALE(bool,Run,(SOMPAINT,const wchar_t*),(c,prog))

SOMPAINT_API /*fails if bad status
//Load program registers. The format string (char*) follows 
//the printf specification where the width field specifies the number
//of registers to load, and the precision field specifies the register offset
//The default width is 1. No flags are defined. Printf types do NOT apply. All types
//are pointers. Recognized types are f for float* (pointer) Lf for double* (will probably 
//be down converted to float) i for int (int*) and b for bool* or Lb for a 32bit bool pointer
//
// Ex. "%8.16f" loads 8 floats beginning at the address of the 16th float register
// Ex. "%8.16f %b" loads the floats and a single bool into the first bool register
//
//Load returns the number of registers written into. If the format string is not valid
//then registers may or may not be loaded up to the point where the string is not valid
//If a pointer is 0 a simulation is run. Afterward the register values will be undefined
//
//The width and precision fields can be a *. %n (Microsoft extension) will not be supported
//
//To ensure future compatibility strings should be limited to % directives and single spaces*/
int SOMPAINT_LIB(Load2)(SOMPAINT c, const char *fstring, va_list cdata)
SOMPAINT_API_FINALE(int,Load2,(SOMPAINT,const char*,va_list),(c,fstring,cdata))
static int SOMPAINT_LIB(Load)(SOMPAINT c, const char *fstring,...)
{
	va_list va; va_start(va,fstring);
	int out = SOMPAINT_LIB(Load2)(c,fstring,va); va_end(va); return out;
}
SOMPAINT_API /*fails if bad status
//size_t is the number of void pointers, which should match
//The number of % directives. When using * notation you must cast to (void*)*/
int SOMPAINT_LIB(Load3)(SOMPAINT c, const char *fstring, void **cdata, size_t count)
SOMPAINT_API_FINALE(int,Load3,(SOMPAINT,const char*,void**,size_t),(c,fstring,cdata,count))

SOMPAINT_API /*delete a wchar_t* handle
//For the record a server deletes all programs etc. on Disconnect (above)*/
void SOMPAINT_LIB(Reclaim)(SOMPAINT c, const wchar_t *prog)
SOMPAINT_API_FINALE(void,Reclaim,(SOMPAINT,const wchar_t*),(c,prog))

SOMPAINT_API /*set a preprocessor macro; nonzero on success
//FYI: there is no compiler (yet) but you can set an environment this way
//
//_locking_												
//A macro buffer cannot be partially Lock'ed. The inout rectangle is ignored for "r" on the way in
//The macros will be returned in the form of a .c file, one #define per line. On "w" the inout box
//must specify a 1D character array. The contents of the buffer are erased and will be parsed anew
//when the buffer is Unlock'ed. Define and Ifdef are UTF-8 APIs [for now we will stick with ASCII]
//
//The macro buffer that was written to is returned; or 0 upon failure. There is a default
//buffer that is obtained when io is 0. If d is 0 the handle is returned immediately (if locked)*/
void **SOMPAINT_LIB(Define)(SOMPAINT c, const char *d, const char *definition _(""), void **io _(0))
SOMPAINT_API_FINALE(void**,Define,(SOMPAINT,const char*,const char*,void**),(c,d,definition,io))

SOMPAINT_API /*get a preprocessor macro; nonzero on success
//The returned string is delete'd if the buffer is Discard'ed, "w" Lock'ed, or Format'ed
//Tip: to prevent deletion a buffer can be Lock'ed by passing 0 to Lock's "mode" argument
//Lock'ing also improves performance by bypassing multi-thread read/write holding patterns*/
const char *SOMPAINT_LIB(Ifdef)(SOMPAINT c, const char *d, void **io _(0))
SOMPAINT_API_FINALE(const char*,Ifdef,(SOMPAINT,const char*,void**),(c,d,io))

/*/// PAL ////////////////////////////////////*/

/*bad things happen if you mix and match APIs*/
#define SOMPAINT_FRAME_BUFFER_API SOMPAINT_API /*render target*/
#define SOMPAINT_BASIC_BUFFER_API SOMPAINT_API /*mipmap buffer*/
#define SOMPAINT_POINT_BUFFER_API SOMPAINT_API /*vertex buffer*/
#define SOMPAINT_INDEX_BUFFER_API SOMPAINT_API /*number buffer*/
#define SOMPAINT_STATE_BUFFER_API SOMPAINT_API /*render states*/

SOMPAINT_API /*nonzero on success 
//Resets the render state so that buffers must be reestablished*/
bool SOMPAINT_LIB(Reset)(SOMPAINT c, const char *reserved _(0))
SOMPAINT_API_FINALE(bool,Reset,(SOMPAINT,const char*),(c,reserved))

SOMPAINT_API /*nonzero on success; fails if bad status 
//Specify the boundaries of the "viewport" in colour buffer coordinates
//inout: if 0 the viewport is set to the dimensions of the colour buffer
//If nonzero inout is clipped against the dimensions of the colour buffer
//FYI: inout defines a rectangle using left, top, right, bottom convention
//IMPORTANT: call Frame after Setup (below) when specifying a colour buffer*/
bool SOMPAINT_LIB(Frame)(SOMPAINT c, size_t inout[4])
SOMPAINT_API_FINALE(bool,Frame,(SOMPAINT,size_t[4]),(c,inout))

SOMPAINT_API /*nonzero on success; fails if bad status
//Specify the boundaries of the "scissor" in colour buffer coordinates
//IMPORTANT: caveats in the Frame (above) comments also apply to Clip*/
bool SOMPAINT_LIB(Clip)(SOMPAINT c, size_t inout[4])
SOMPAINT_API_FINALE(bool,Clip,(SOMPAINT,size_t[4]),(c,inout))

SOMPAINT_STATE_BUFFER_API /*nonzero on success; fails if bad status
//Bind a fixed function pipeline state buffer (i permits buffer specialization)*/
bool SOMPAINT_LIB(PAL_Apply)(SOMPAINT_PAL c, int i _(0))
SOMPAINT_API_FINALE(bool,PAL_Apply,(SOMPAINT_PAL,int),(c,i))

SOMPAINT_FRAME_BUFFER_API /*nonzero on success; fails if bad status
//Choose which frame-buffers to use. If two buffers are of the same type, 
//the buffer that was last "setup" will be the one that is used. If setting a
//buffer with multiple planes results in an incompatibility with the other buffers
//with multiple planes, then Setup will fail (i supplies the 0 based multi-target index)
//
//IMORTANT: the colour buffer is required to fit inside of the other buffers 
//If using multiple colour buffers (render targets) the dimensions must match
//
//If a depth plane is not setup a shared buffer will be used upon demand
//(calling Reset (above) causes the buffer to be cleared to 1.0 if used)*/
bool SOMPAINT_LIB(PAL_Setup)(SOMPAINT_PAL c, int i _(0))
SOMPAINT_API_FINALE(bool,PAL_Setup,(SOMPAINT_PAL,int),(c,i))

SOMPAINT_FRAME_BUFFER_API /*nonzero on success; fails if bad status
//Sets every pixel of the buffer according to the Format clear function
//mask: set each successive bit to 1 for each plane of the buffer to clear
//(so that bit 2 or more is only in play if your buffer has multiple planes)
//IMPORTANT: Clear is restricted to the windows of Frame and Clip (above)*/
bool SOMPAINT_LIB(PAL_Clear)(SOMPAINT_PAL c, int mask _(~0))
SOMPAINT_API_FINALE(bool,PAL_Clear,(SOMPAINT_PAL,int),(c,mask))

SOMPAINT_BASIC_BUFFER_API /*nonzero on success; fails if bad status
//Select a texture for proceeding calls to Draw (below; i facilitates multi-texture)*/
bool SOMPAINT_LIB(PAL_Sample)(SOMPAINT_PAL c, int i _(0))
SOMPAINT_API_FINALE(bool,PAL_Sample,(SOMPAINT_PAL,int),(c,i))

SOMPAINT_POINT_BUFFER_API /*nonzero on success; fails if bad status
//At somepoint maybe we will decide upon a framework for compiling layouts 
//But for now we are just passing along built-in layout codes that are peculiar to 
//Sword of Moonlight: MDO, MSM, MDL, MPX
//
//Built-in layouts will be described here once standardized*/
bool SOMPAINT_LIB(PAL_Layout)(SOMPAINT_PAL c, const wchar_t *p)
SOMPAINT_API_FINALE(bool,PAL_Layout,(SOMPAINT_PAL,const wchar_t*),(c,p))

SOMPAINT_POINT_BUFFER_API /*fails if bad status
//Prepare to render up to n instances of the buffer's contents where
//up provides per instance "immediate" data to be uploaded for one time use or
//for however many times Draw (below) is called upon before the next usage of Stream 
//The format of up is part of the Layout (above) arrangement. The size of the upload is up_s
//which is divided by n to get the per instance stride of the upload. The stride can be less than 
//what is specified by the layout, in which case the tail end of the layout is undefined
//
//Stream returns the number of instances that it is able to render / was able to upload
//If the return is less than n then it is up  to the caller to buffer the contents of their
//stream (by repeatedly calling Stream and Draw while advancing the up pointer etc.)*/
int SOMPAINT_LIB(PAL_Stream)(SOMPAINT_PAL c, int n, const void *up, size_t up_s)
SOMPAINT_API_FINALE(int,PAL_Stream,(SOMPAINT_PAL,int,const void*,size_t),(c,n,up,up_s))

SOMPAINT_INDEX_BUFFER_API /*nonzero on success; fails if bad status
//Render the contents of the Stream (above) between start and start_count indices
//Caveats: the type of primitive to be drawn is assumed to be a triangle list (count/3) 
//Most APIs have a Begin/End pair of functions that bookend calls to the drawing procedures
//We do not have explicit APIs for these. Begin happens before the first Draw after Setup (above)
//And End happens either before Setup or before Present (below) whenever it is necessary*/
bool SOMPAINT_LIB(PAL_Draw)(SOMPAINT_PAL c, int start, int count, int vstart, int vcount)
SOMPAINT_API_FINALE(bool,PAL_Draw,(SOMPAINT_PAL,int,int,int,int),(c,start,count,vstart,vcount))

/*/// Portable Operating Systems APIs ////////*/

SOMPAINT_API /*nonzero on success
//Memory is reserved even when Raster returns 0. See Discard (above)
//
//What Raster (and Window) does is convert a structure that is provided by the OS
//which represents a 2D grid of pixels, or something that can be readily mapped to such a 
//thing into a typed object that the server can deal with. If the object is remote then the type
//of the object does not matter. If the object is local then it needs a type. The type is provided via
//the typeid operator (this may change if it turns out that typeid is not standard)
//
//Of course this is a C API. And C does not do typeid. We assume C programmers can work around it
//
//IMPORTANT: Like with Buffer (above) io must point to a persistent address
//Raster is not a property of Buffer (eg. do not mix the void** io pointers)
//Thoughts: it may be useful to pass a void** to obtain a Raster for a Buffer
//
//local: this is a pointer to the OS object which must persist as long as it is assigned to the 
//raster io. The value of local can be changed by calling Raster again with the same io argument
//Of course if the raster is remote (or on some other computer somewhere) local should not be set 
//
//SOMPAINT_PIX is an opaque type that is implemented by the server. use Discard (above) when 
//you are finished with the raster (unless you intend to let the server do so upon Disconnect)
//
// Ex. "SOMPAINT_PIX pix = Raster(&raster,&hwnd,typeid(hwnd).raw_name()); //typeid(HWND)"
//
//typeid_local_raw_name: if 0 local is assumed to be a previously established type...*/
SOMPAINT_PIX SOMPAINT_LIB(Raster)(SOMPAINT c, void **io, void *local _(0), const char *typeid_local_raw_name _(0))
SOMPAINT_API_FINALE(SOMPAINT_PIX,Raster,(SOMPAINT,void**,void*,const char*),(c,io,local,typeid_local_raw_name))

SOMPAINT_API /*nonzero on success
//Memory is reserved even when Window returns 0. See Discard (above)
//
//The situation with Window is identical to Raster(above) except that window conceptually
//represents a rectangular region of a raster. The window is typically four numbers defining
//coordinates of four corners of a box within the coordinate space of a compatible raster object
//
// Ex. "SOMPAINT_BOX box = Window(&window,&rect,typeid(rect).raw_name()); //typeid(RECT)"
*/
SOMPAINT_BOX SOMPAINT_LIB(Window)(SOMPAINT c, void **io, void *local _(0), const char *typeid_local_raw_name _(0))
SOMPAINT_API_FINALE(SOMPAINT_BOX,Window,(SOMPAINT,void**,void*,const char*),(c,io,local,typeid_local_raw_name))	

SOMPAINT_API /*nonzero on success; fails if bad status
//Project the contents of a colour (basic or frame) buffer onto an operating system raster object
//zoom: if 0 the contents of src will be stretch blit'ed into the the region of win defined by dest
//If zoom is nonzero no stretching will be applied and src must be premultiplied by the value of zoom
//IMPORTANT: normally a "present" is a blocking procedure unless indicated otherwise. We assume blocking
//is desirable. A present should be isolated to its own thread so that the thread can wait for it to finish
//If SOMPAINT_PAL is not a "first class" frame buffer it gets copied to Buffer(0) piece by piece if necessary
//
//Considerations: under Direct3D9 when using a frame buffer without copy semantics (see Format comments) 
//in order to achieve a direct presentation zoom, src, and dst must all be 0. The entire buffer will get
//stretched over the entire client area of pix. If using a frame buffer with copy semantics zoom must be
//set to 0 also (every other mode of presentation will probably be copied piece by piece into Buffer(0))
//
//WARNING: If Buffer(0) does not have copy semantics indirect modes of presentation will always fail. As
//will Buffer(0) in attempts to indirect present into itself (modules should not attempt to remedy this)
//
//_Microsoft Visual C/C++_
//A local SOMPAINT_PIX is expected to be HWND type; SOMPAINT_BOX is expected to be RECT type*/
bool SOMPAINT_LIB(PAL_Present)(SOMPAINT_PAL c, int zoom, SOMPAINT_BOX src, SOMPAINT_BOX dst, SOMPAINT_PIX window)
SOMPAINT_API_FINALE(bool,PAL_Present,(SOMPAINT_PAL,int,SOMPAINT_BOX,SOMPAINT_BOX,SOMPAINT_PIX),(c,zoom,src,dst,window))

SOMPAINT_API  /*nonzero on success; fails if bad status
//Create a fill pattern given a buffer of pixels. See source code to get an idea of what this does
//To get an idea, basically it is for palette visualization and creating background / fill patterns
//Reminder: if we need to we can use Raster with the void** of a Buffer to pass it thru SOMPAINT_PIX*/
bool SOMPAINT_LIB(PAL_Fill)(SOMPAINT_PAL c, int m, int n, int wrap, int zoom, SOMPAINT_BOX src, SOMPAINT_BOX dst, SOMPAINT_PIX window)
SOMPAINT_API_FINALE(bool,PAL_Fill,(SOMPAINT_PAL,int,int,int,int,SOMPAINT_BOX,SOMPAINT_BOX,SOMPAINT_PIX),(c,m,n,wrap,zoom,src,dst,window))

SOMPAINT_API  /*nonzero on success; fails if bad status
//The difference between Print and Present is Print creates a new object such
//as a bitmap that will contain the thumbnail or enlarged (or 1:1) copy of the image
//Do not forget that a new object overwrites an old object (Print does not delete a thing)
//
//_Microsoft Visual C/C++_
//A local SOMPAINT_PIX is expected to be HBITMAP type (remote may not make sense)*/
bool SOMPAINT_LIB(PAL_Print)(SOMPAINT_PAL c, int width, int height, SOMPAINT_PIX bitmap)
SOMPAINT_API_FINALE(bool,PAL_Print,(SOMPAINT_PAL,int,int,SOMPAINT_PIX),(c,width,height,bitmap))

#endif /*SOMPAINT_NO_THUNKS*/

#pragma pop_macro("_")

struct SOMPAINT_LIB(buffer)
{
#ifdef __cplusplus

	virtual bool apply(int i=0)=0;
	virtual bool setup(int i=0)=0;
	virtual bool clear(int mask=~0)=0;	
	virtual bool sample(int i=0)=0;
	virtual bool layout(const wchar_t*)=0;
	virtual int stream(int n, const void *up, size_t up_s)=0;
	virtual bool draw(int start, int count, int vstart, int vcount)=0;

	/*portable operating systems APIs*/
	virtual bool present(int zoom, SOMPAINT_BOX src, SOMPAINT_BOX dst, SOMPAINT_PIX)=0;
	virtual bool fill(int m, int n, int wrap, int zoom, SOMPAINT_BOX, SOMPAINT_BOX, SOMPAINT_PIX)=0;
	virtual bool print(int width, int height, SOMPAINT_PIX)=0;

#endif
};

#ifdef SOMPAINT_API_ONLY
#pragma pop_macro("SOMPAINT")
#pragma pop_macro("SOMPAINT_PAL")
#pragma pop_macro("SOMPAINT_PIX")
#pragma pop_macro("SOMPAINT_BOX")
#endif

#ifdef SOMPAINT_DIRECT
#undef protected
#endif

#endif /*SOMPAINT_INCLUDED*/

/*RC1004*/