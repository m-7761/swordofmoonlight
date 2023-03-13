							
#ifndef SOMTEXTURE_INCLUDED
#define SOMTEXTURE_INCLUDED

class Sompalette;
class Somgraphic; 
class Somenviron;

class Somtexture
{	
private: 

	Somtexture(int pool=0);	
	Somtexture(const Somtexture&);
	~Somtexture();

public: //readonly
	
	int width, height;

	//b/w selection mask
	const Somtexture *mask;

	inline void unmask()const
	{
		if(this) mask->release(), 
		const_cast<Somtexture*&>(mask) = 0; 
	}

	//256 entries when nonzero
	const Sompalette *palette; 
	
	enum //data channels (0 resolves to primary)
	{
	//FYI: BLACK is computed/converted when saving/opening files
	PAL=1|2, MAP=2, INDEX=4, RGB=8, ALPHA=16, BLACK=32, NOTES=64, 
	
	DEPTH=128, STENCIL=256, XYZ=512, //swap & vertex buffer use

	_ADD=PAL|INDEX|RGB|ALPHA|BLACK|NOTES, //.pal specification
	_CMP=MAP|INDEX|RGB|ALPHA|STENCIL,	 //undo compatibility
	_USE=PAL|INDEX|RGB|ALPHA,           //primary candidates

	CLEAR=1<<30, //synthetic channels? 
	};
	
	int primary()const; //primary channel 

	static bool primary_candidate(int ch) //must be singular
	{		
		switch(ch){	case PAL: case INDEX: case RGB: case ALPHA: return true; }
		return false;
	}

	inline int legacy()const //TXR compatability 
	{
		int ch = primary(); return ch==PAL||ch==RGB?ch:0;
	}

	enum //data channel formats: 0 if not represented
	{
	//NEW: -128 through -2 indicate signed integer formats 
	//GREY is 1D data. A 9 means GREY9 or 9bits. MED/HIGHP are floating point layouts
	GREY1=1, GREY2=2, GREY8=8, GREY16=16, GREY32=32, RGB8=129, RGBA8, MEDP, HIGHP, MIXED
	};
		
	int channel(int ch)const; //format	 	

	int pixel(int ch)const; //see register_pixel and describe_pixel for details
		
	//packed size of channels(s) in bytes
	//spec: works like ch except memory is included if spec is not represented
	//set ch and spec equal to get the memory required regardless of representation
	size_t memory(int ch=-1, int spec=0)const; 
	
	static bool known_channel(int ch) //single channel filter
	{
		switch(ch){ case PAL: case MAP: case INDEX: case RGB: case ALPHA: return true; }
		switch(ch){	case DEPTH: case STENCIL: case XYZ: case BLACK: case NOTES: return true; }
		return false;
	}
	static bool known_format(int f) //single format filter
	{
		switch(f){ case RGB8: case RGBA8: case MEDP: case HIGHP: case MIXED: return true; }
		return f>=1&&f<=128||f<=-2&&f>=-128; //GREY1~GREY128
	}
	
	bool readonly()const; //true if not opened with editing functionality

	//// EDITING //////////////////////////////////////////////////////////////////////////
	//
	// Procedures below will fail if a textures was not opened with editing functionality
	//
	// WARNING: except for addref/release, Somtexture is generally not atomic; so that it 
	// is important to ensure access is exclusive whenever modifying a texture's contents
	
	//use: false if ch is already primary
	//add: false if ch is already present

	bool use(int ch)const; //change primary 
	bool add(int ch)const; //add new channel(s)

	inline bool remove(int ch)const									
	{
		return add(-ch); //remove channels
	}
	
	//NOTES pal-id: 
	//read is filled with up to size-1 characters starting at skip and 0 terminated
	//write is written into the notes at skip per size, then copied into read (if nonzero)
	//Returns 0 on failure, else a counter that is incremented each time the notes are changed
	wchar_t notes(wchar_t *read=0, wchar_t size=0, wchar_t skip=0, const wchar_t *write=0)const;

	inline size_t notes_characters()const //including 0 terminator
	{
		size_t width = channel(NOTES)/CHAR_BIT; return width?memory(NOTES)/width:0;
	}

	//Register a change made to a channel so that it may be undone
	//
	//Notice: the following comments apply to the MAP/INDEX/RGB/ALPHA channels
	//And also the STENCIL channel of masking textures (used for editing purposes)
	//Tip: unmap/unpaint can be used for single/lightweight undo with the MAP channel
	//UPDATE: the XYZ channel (3D geometry) will probably be fast tracked sooner or later
	//
	//compare generates an internal diff against the previous comparison of the channel 
	//data bypassing the texture's mask. If no change is detected compare returns 0. Else
	//a new internal diff is generated and compare returns the amount of diff memory in use
	//Note: in order to compute diffs over the channels a full shadow copy of each channel is
	//basically kept at all times. Regardless the copies are unrepresented by the return figure
	//
	//memory: if 0 the amount of memory used for the undo history is unlimited. Otherwise 
	//memory is taken to be the limit. There is always room for one undo over the limit, no 
	//matter how large that it happens to be. So you can use this chance to alert a user that 
	//a particular undo was particularly costly (before their entire undo history is wiped out)
	//
	//Once registered changes may be traversed backwards and forwards by undo and redo respectively
	//
	//What is undone: pixel changes to the channels and well/tone mappings per MAP
	//What is not undone: well titles, notes, and any other channels not commented upon up top
	//
	//Both undo and redo return the current position in the timeline, which can be either 0 for
	//the "present" or a negative number for the "past". If the time changes then the internal diff
	//is applied to the channel. time (below) can simply retrieve the current position in the timeline
	//
	//history returns the span of the timeline in positive number form; eg. 1 for (-1,0)
	//	
	int compare(size_t memory=0)const; //FYI: Somtexture::mask is managed directly
	
	int undo(int steps=-1)const; inline int redo()const{ return undo(+1); }; 
	
	int history()const; inline int time()const{ return undo(0); }; 

	//UNIMPLEMENTED: _subject to reconsideration_
	//Self explanatory: per colour channel blit operation
	//ch: destination; must be one of MAP INDEX RGB ALPHA or RGB|ALPHA
	//When pasting to MAP specify the width: MAP|16<<MAP; the default width is 16
	bool paste(int ch, const Somtexture*, const RECT *src, const RECT *dest)const;

	//UNIMPLEMENTED: _subject to reconsideration_
	//Same as paste really except that the source is a window
	//If HWND is 0 the source is the Sompaste.dll system clipboard
	bool copy(int ch, HWND, const RECT *src, const RECT *dest)const;
	
	//Again, same as paste, except src is memory (nothing fancy)
	//new: z is the unit size for nib in bytes. Units are unsigned
	//nib is a list of indices into p. x/y are the dimensions of nib	
	//p is the palette where p[w] is the mask and everything is loaded 
	//into the top left (0,0) of the texture/mask width/height unchanged
	//If p is 0 nib is greyscale (expanded if need be for colour channels)
	bool load(int ch, void *nib, int x, int y, int z, int w, const PALETTEENTRY *p=0)const;

	inline bool loadsolidbackground(PALETTEENTRY clear) //see samplesolidbackground
	{
		return load(CLEAR,0,0,0,0,0,&clear); //overwrites palette[0] RGB if colorkey is in play	
	}

	//trim the fat off your Somtexture (or expand)
	bool crop(int width, int height, int x=0, int y=0, PALETTEENTRY *clear=0)const;

	//Reminder: _how we intend to do this_
	//The user drag-draws a box with the mouse
	//The quadrant of the box selects the logic op
	//TRUE, XOR, OR, AND, while shift then rotates the 
	//quadrants if need be. Once the mouse is let up the
	//box remains. The user then selects the side of the box
	//desired (inside/outside) or chooses an X in the corner of
	//the (resizeable) box to cancel the selection procedure (cool) 

	enum //logical ops
	{	
	XOR=2, //invert	 ---
	OR,   //union	 +++
	AND, //intersect ===
	};

	//1: to select, 0 to clear, box=0 is everything
	//not selects what is not box. Eg. AND+not=subtract
	bool boxselect(int op, RECT *box=0, bool not=false)const;

	//If Somtexture has a mask the mask will be pasted to mask,
	//Otherwise Somtexture itself is interpretted as a mask and pasted
	//to mask (same either way) per op and not (same behavior as boxselect)
	bool copyselect(int op, const Somtexture*, RECT *src, RECT *dst, bool not=false)const;

	//The palette entries from a to z are self selected
	bool mapselect(int op, int a, int z, bool not=false)const;

	//Retrieve the colour (p) at f(x,y,z,w) and boolean mask 
	//f may be 0 for "nearest neighbor" filter or 1 to use mask
	//If the mask is absent true is returned else (if masked) false
	//Samples that would wrap return false regardless; p is unaltered
	//p->peFlags is 0, transparent, or 255, opaque. If 0 p is unaltered
	bool sample(int ch, int f, PALETTEENTRY *p, float x=0, float y=0, float z=0, float w=0)const;

	inline PALETTEENTRY samplesolidbackground() //same RGB as palette[0] if colorkey is in play
	{
		PALETTEENTRY out; out.peFlags = sample(CLEAR,0,&out)?0xFF:0; return out;
	}
	
 	//well constants: titled wells use TITLED+N where N is between 0 and 29
	//The RESERVED well tracks the unused entries at the back of the pallete
	enum{ COLORKEY = 0x40, UNTITLED = 0x4F, TITLED = 0x50, RESERVED = 0x80 };

	//The colorkey well should be tone 0 or nada; else something is amiss
	//Note: peFlags contains the constant for the well that the entry belongs to
	inline bool colorkey(bool def=0)const
	{
		return this&&palette?((PALETTEENTRY*)palette)->peFlags==COLORKEY:def; 
	}

	//Enumerate paint wells and the tones per well: first/last palette entry, 0 if none
	//The last well returns 0 and is the "reserved" well (tones are back to back in palette)
	//Alternatively wells may be obtained directly by using the constants above (COLORKEY, etc) 
	//Conceptually the RESERVED well is always last and contains entries deleted from the palette 
	//The COLORKEY well if present always comes first and contains the one and only transparent tone	
	//The returned wchar_t* memory remains operational as long as there are any outstanding references
	//A trick is well(0)[-1] (if nonzero) holds the well constant. if(well(0)==well(COLORKEY)) works too
	//
	//NEW: well(-1) and so on works to enumerate the wells in reverse order
	const wchar_t *well(int)const; const wchar_t *tones(const wchar_t *well)const;
	
	inline const wchar_t *palette_well(size_t tone)const //A less confusing way to get at peFlags
	{			
		return this&&palette&&tone<=255?well(((PALETTEENTRY*)palette)[tone].peFlags):0; 
	}
	
	size_t palette_index(size_t tone)const; //internal logical index per INDEX channel

	//WARNING: These assume that well is a valid well pointer
	static int id(const wchar_t *well){ return well?well[-1]:0; }
	static int no(const wchar_t *well){ return well?well[-1]-TITLED:-17; } 

	//Rename an existing paint well. All wells can be retitled, but special
	//wells may not survive saving to a file. UNTITLED can be retitled but it is 
	//better to empty out if possible. There is an internal limit, probably around 24
	//or 32 characters. The title will be cut off. Duplicate and empty titles are allowed
	//
	//Notes: if name is 0 the current title is returned. For reserved versions of wells 
	//the correspoding well pointer is returned, otherwise if name is 0 and well is valid
	//well will simply be returned (this is because the pointers themselves are to the title) 
	//If a reserved well portion is retitled, the corresponding well is retitled. Note that you
	//may retitle the RESERVED psuedo-well without affecting other wells. But its not recommended
	//
	//FYI: title always return nonzero unless the texture or well is invalid, otherwise 0
	const wchar_t *title(const wchar_t *well, const wchar_t *name=0, size_t namelen=0)const;

	//// paint/map /////////////////////////////////////////////////////

	//Enumerate wells displaced by the last [un]map/[un]paint operation
	//Notice: avoid reserve if sharing a texture in a multi-threaded setup. Use loop instead
	//
	//This is pretty confusing, but its the best conceptual schema I could come up with --Old Hand
	//
	//Basically a queue is kept for displaced wells. A paint/map call empties the reserve queue and
	//then will fill it up with displaced portions of the wells. You can directly access the reserve 
	//wells by combining a well constant with |RESERVED. However reserve does this automatically when
	//supplied with well constants. The relationship is 1:1 minus tones belonging to the RESERVED well
	//You can use REPLACED (defined below) to obtain the "reserved" portion of the RESERVED pseudo-well
	//You can use FINISHED to manually empty the queue (so to be sure it is not used again accidentally)
	//
	//The returned pointer if nonzero will point to an empty string. RESERVED nets you a 0 return
	//Use id (below) to get the constant and or title (above) to get the corresponding name string
	const wchar_t *reserve(int)const; enum{ REPLACED = 0xFE, FINISHED = 0xFF };

	//Define a paint well
	//Returns affected well upon success or 0 upon failure. If well is 0 a well is created if possible
	//
	//a/z: paint fails if a~z do not adjoin well. The tones from a~z are painted per op(p[a~z-a],mask) 
	//p: upon success the contents of p are replaced with the colours that were replaced during painting
	//This way p can be passed to unpaint (below) in order to reverse any changes that were made by paint
	//mask: when op is 1 mask is a per channel alpha mask for each colour of p. Eg. if 0 a~z are untouched
	//op: if 0 paint will fail (returning 0) and if negative paint will behave like unpaint (so be careful)
	//NEW: if op=0 paint now behaves as if op=1 while the contents of p are NOT replaced (so unpaint is out)
	//
	//Note: if p is 0 unpaint is not an option and the colours a~z are untouched as if mask is 0 and op is 1
	//Warning: if a new well cuts an existing well in two the right half is appended to the new well (beyond z)
	const wchar_t *paint(const wchar_t *well, int a, int z, PALETTEENTRY *p=0, COLORREF mask=0xFFFFFF, int op=1)const;	

	//UNIMPLEMENTED _may be unnecessary_
	//Reverse the effects of a (single successful) paint operation
	//Inputs must be the values passed to paint (not the returned value) or the result will be wrong
	//
	//p: upon success the contents of p are replaced with the colours that were replaced during painting
	//This way p can be passed back to paint (above) in order to reverse any changes that were made by unpaint
	inline const wchar_t *unpaint(const wchar_t *well, int a, int z, PALETTEENTRY *p, COLORREF mask=0, int op=1)const
	{
		return p&&op?paint(well,a,z,p,mask,-op):0;
	}	
	
	//move palette entries a~z to aa...
	//inputs 256~511 are automatically mapped to the RESERVED well (-256)
	//if the map operation is non-reversible the operation fails with 0 return
	//Disclaimer: moving a~z to aa does not mean that a will be at aa on success
	//
	//op=1: if aa is the front of a well, position a~z behind the well
	//op=2: if aa is the front of a well, add a~z to the front of the well
	//Note: if op is 0 map fails. If negative map acts like unmap (so be careful)
	//If op is BEHIND, and a~z is a subsection of a well, and aa is 0, map fails
	//Ideally any mapping which does not affect any change should return false	
	//
	//WARNING: map does not single out the COLORKEY well for special treatment
	bool map(int a, int z, int aa, int op=1)const; enum{ BEHIND=1, BEFORE=2 };

	//UNIMPLEMENTED _may be unnecessary_
	//Reverse the effects of a (single successful) map operation
	inline bool unmap(int a, int z, int aa, int op=1)const{ return map(a,z,aa,-op); }

	//Build a (lossy) palette and index given an RGB channel and mask
	//
	//toner: upper limit for the generation of tones (outside 1~256 will fail)
	//Returns 0 on failure, or the number of tones considered upon success. If a number
	//greater than toner is returned, then the index is lossy. Otherwise the index is a match
	//Note: if the return is less than toner then paint entries above and including the returned
	//value are not changed, and neither are any values greater than or equal too written to the index
	//
	//WARNING: index requires an RGB channel and will fail (returning 0) without one
	//An all 0 INDEX channel is added if one does not already exist (and index is positive)
	//
	//paint: is required to be filled up to toner (-1) with the generated colour palette
	//index: if less than zero the INDEX channel remains unchanged. Otherwise index provides the 
	//relative offset of the created index space. So that paint[0] maps to index+0 in the INDEX channel
	//If index is positive and index+toner is greater than 256 then you guessed it (failure)
	//
	//Disclaimer: in order to use index effectively paint must be passed to paint (above)
	//so that the changes can be validated and situated within one or more wells (and unpainted)
	//Otherwise the MAP channel will not reflect what changes are made to the INDEX channel 
	size_t index(size_t toner, PALETTEENTRY *paint, int index = -1)const; 

	//If ch is INDEX and op is 0 the INDEX channel is 
	//rearranged so that 0 is the first palette entry and so on
	//returns false if order is unchanged (eg. already sorted or failure)
	bool sort(int ch=INDEX, int op=0, int a=0, int z=0)const;

	//// REGISTRATION //////////////////////////////////////////
	 														
	//Somtexture.cpp will copy your inputs and return a "pixel description token" to you
	//The token can then be used for retrieval of the custom pixel descriptor via pixel (above)
	//
	//subformat: list of air tight packed subformats (GREY1,GREY2,etc) of size_t length
	//description: optional text labels for each subformat separated by \n (new line) characters
	//After the last label line a brief description of the pixel in total may be provided or not
	static int register_pixel(const int *subformat, size_t, const wchar_t *description, size_t);

	template<size_t M, size_t N>
	static int register_pixel(const int (&subformat)[M], const wchar_t (&description)[N])
	{
		return register_pixel(subformat,M,description,N);
	}

	//bits_per_pixel: the format constants (eg. RGB8) have built in pixel identifiers
	//and so if you need to get the bits per format so to speak you can pass the formats
	//to bits_per_pixel; for instance you can programmatically walk the subformats obtained
	//from describe_pixel (below) 

	static size_t bits_per_pixel(int px); //supplies the total number of bits in a pixel

	inline int depth(int ch)const{ return bits_per_pixel(pixel(ch)); }

	//Retrieve a description of a pixel previously registered via register_pixel (above)
	//Note: description tokens should be provided for basic built in formats but may not be so
	//
	//subformat: is filled up to size_t with the subformat list; excess is zeroed out	
	//The size of the subformat array is returned. It may be more or less than size_t
	//
	//description: if available description will be aimed at an internal copy of the 
	//text provided to register_pixel (the description text may or may not be localized) 
	static size_t describe_pixel(int px, int *subformat=0, size_t=0, const wchar_t **description=0);

	//WARNING: be careful you do not accidentally call describe_pixel (or vice versa)
	inline size_t describe(int ch, int *subformat=0, size_t f_s=0, const wchar_t **description=0)const
	{
		return Somtexture::describe_pixel(pixel(ch),subformat,f_s,description);
	}			

	//// READONLY //////////////////////////////////////////////
	//
	// In theory these APIs are available to all texture objects
	
	//Render channel to an HWND (the destination)
	//zoom: src must be pre-multiplied according to zoom
	//mask: opacity -255~255; 0 is transparent 255 is opaque -255 is reversed
	//When presenting MAP specify the destination width: MAP|16<<MAP; the default method
	//is to wrap the map around the width of dest where src.top should be 0 and src.bottom 
	//must be equal to zoom and src.left is the first*zoom and src.right is the last*zoom+zoom
	//IMPORTANT: presentation usually occurs immediately if not open'ed with a "vsync" parameter
	//There is a wait if presenting a non frame buffer while another thread owns apply_state (below)
	bool present(int ch, HWND, const RECT *src, int zoom, const RECT *dest, int mask=0)const;
	
	//Format a channel (usually from scratch)
	//Tip: in addition to format you may want to pass some parameters to open (below)
	//This is primarily for setting up a render target or vertex and index buffers for 3D geometry
	//ch: RGB for render targets or XYZ and INDEX for 3D geometry buffers (use two separate textures)
	//Use DEPTH and STENCIL to add a depth-stencil buffer (for a Z buffer and other kinds of effects)
	//x/y: the width and height of the texture respectively. For 3D geometry the height (y) should be 1 
	//depth: depth in bits. Note that RGB formats are expected to be 32bits and can be only one per pixel
	//pixel: Use register_pixel to make a complex pixel description. If 0 a builtin description is provided
	//tx/ty: when resizing set tx / ty to an optimum internal size that is greater than or equal to x / y
	bool format(int ch, int x, int y, int depth=0, int format=0, int pixel=0, int tx=0, int ty=0)const;

	 //// _locking methods: buffer, read_/////////////
	//Use this to write pixels to a format'ted (above) textures
	//lock: if true the texture will remain locked between subsequent calls
	//WARNING: setting lock to true will result in the entire texture being locked;
	//Use lock for initialization. The setting may be overridden at runtime if so desired
	//For the record: do not mix locking methods (buffer, read, adjust) or chaos will ensue!
	bool buffer(int ch, void *set, size_t pitch=0, const RECT *dst=0, bool lock=false)const;	
	 //////////////////////////////////////////////////////////
	//Use this to read back pixels from readonly (above) textures
	//lock: works like buffer (above) but is probably less important to reading operations
	bool read(int ch, void *get, size_t pitch=0, const RECT *src=0, bool lock=false)const;

	//SCHEDULED FOR REMOVAL
	//HACK: Trying to get the holes of the legacy TXRs.
	bool where_BLACK(int ch, COLORREF(COLORREF))const;

	//// INSTANTIATION /////////////////////////////////////////

	enum{ SHORT = 1, LOCAL = 2 };
	//NEW: enumerate the available PAL implementations
	//item can be positive or negative. 0 is the default item
	//The returned strings are in perma memory. mode can be 0 
	//for long form or SHORT for short form or LOCAL for long
	//and localized form but no other combination; SHORT form
	//should not be localized. From there you can then pass a
	//negative item to open/edit (below) as the pool argument
	//in order to create a pool using that PAL implementation
	static const wchar_t *menu(int item, int mode=0);

	//open: when opening the first Somtexture in a pool these 
	//constants guarantee certain types of pools if available

	enum{ SOMPAINT_D3D9=-100 }; //Sompaint_D3D9 library server

	//open textures are readonly and edit textures can be edited
	//pool: if 0 a new pool (video adapter interface) is created	
	//file: commandline (a 0 returns an empty texture container)
	//file_s: sizeof file if nonhuge, else 0 termination assumed
	//
	//commandline options: [-xv] or [-m] [path to an image file]
	//
	//_render targets and frame buffers_
	//-x indicates a frame buffer is desired over a render target
	//-v indicates present is to wait for the next vertical blank
	//-m indicates mipmaps should be generated from top to bottom
	//A render target created with mipmaps is usable as a texture
	//but cannot be a frame buffer; -xv and -m are not compatible
	//
	//_classic texture in an image file_
	//-m indicates mipmaps should be discarded if present and not 
	//generated; the default behavior is to facilitate mipmapping
	static const Somtexture *open(int pool, const wchar_t *file, size_t file_s=-1UL);
	static const Somtexture *edit(int pool, const wchar_t *file, size_t file_s=-1UL);
	
	//Seems to come in handy more often than not
	static const Somtexture *open_or_edit(bool ro, int p, const wchar_t *f, size_t f_s=-1UL)
	{
		return ro?Somtexture::open(p,f,f_s):Somtexture::edit(p,f,f_s); //ro: readonly
	}

	int pool()const; //the pool membership id

	//Create a new texture from the same pool
	inline const Somtexture *open(const wchar_t *file, size_t file_s=-1UL)const
	{
		return Somtexture::open(pool(),file,file_s);
	}
	inline const Somtexture *edit(const wchar_t *file, size_t file_s=-1UL)const
	{
		return Somtexture::edit(pool(),file,file_s);
	}

	//Re-open texture (the texture's undo history is discarded)	
	//edit: if true readonly status is _inverted_ [you're right; it is confusing!]
	bool refresh(const wchar_t *file=0, size_t file_s=-1UL, bool edit=false)const;		
	
	//Create a thumbnail (for Windows' Shell Extensions)
	HBITMAP thumb(int width=0, int height=0, bool enlarge=false)const;
		
	//Compute rectangle bounded by width/height offset by x/y
	RECT aspect(int width, int height, int x=0, int y=0)const;

	//// REFERENCE COUNTING /////////////////////////////////
	
	const Somtexture *addref()const; //returns self
	const Somtexture *release()const; //returns self or 0

	int refcount()const; //should be used for diagnostic only

	inline const Somtexture *deplete()const	//garbage collection
	{
		return refcount()==1?release():this; //ok a valid use
	}

	//// Render-to-Texture APIs /////////////////////////////
		
	//apply_state: the following constants are OR'd to affect
	//the behavior of apply_state and therefore apply (below)
	//
	//BLEND: assuming that graphics are either translucent or
	//opaque. If set the opaque are not drawn. And vice versa
	//SOLID: if set BLEND is ignored and everything is opaque
	//LIGHT: if set lighting is in play. Otherwise lights out	
	//ID: for selection buffers; the individual pieces of the 
	//graphics are rendered with solid colours that are taken
	//from Somgraphic::id, multiplied by 128, summed with the
	//column (piece) number, and converted into an RGB colour
	//OPAQUE: requires Somgraphic::opacity to be equal to 1.0
	//Not to be confused with BLEND or SOLID. For use with ID
	//FENCE: apply (below) runs sections by Somgraphic::fence

	#undef OPAQUE //wingdi.h

	enum{ BLEND=1, SOLID=2, Z=4, LIGHT=8, ID=16, OPAQUE=32 };
	enum{ FENCE=64 }; 
														
	//Bookends calls to apply (below)
	//Call with env=0 when closing an apply block
	//apply_state will fail if not closed on a per pool basis
	//IMPORTANT: see present (above) docs before calling apply_state
	//NEW: calling back-to-back with the same Somenviron preserves the state
	bool apply_state(const Somenviron *env, int blend_etc=SOLID|Z)const;
		
	enum{ KEEP=1, POST=2 }; 
	//Call after apply_state in order to filter the graphical channels
	//int: the column; int: filter code; void* is passed thru the callback
	//The callback should return 0 to discard the graphics. Note that graphics
	//are rendered by channel rather than column. Multiple columns may be rendered
	//if the columns are self similar. Return nonzero (KEEP) to not discard and POST if
	//you need a two pass filter (the filter code can be OR'd with POST when postfiltering)	
	bool apply_filter(int(*)(const Somgraphic*,int,int,void*), void*)const;
	
	//Render a graphic per apply_state (above)
	//n: the instance to draw; if -1 all instances are drawn
	//Note: apply will do its best to draw the instances in parallel
	//Tip: An empty container texture can be used to "preapply" any graphics
	//that need to be displayed promptly in a real-time scenario. If not prepared
	//in this way there may be an avoidable pause when a resource is apply'd the first time
	bool apply(const Somgraphic*, int n=-1)const; 
												  
	//Fill a channel with its clear value
	//For colour channels the clear colour is taken from the Somenviron
	//passed to apply_state; except in ID mode the default colour 0,0,0,1 is always used 
	inline bool clear(int ch=-1)const
	{
		return apply(0,ch); //clear channel(s)
	} 	
												
	/*/////// private implementation ///////*/
		
#ifdef SOMPAINT
friend class SOMPAINT_LIB(server); //Sompaint.h 
	SOMPAINT server()const;
#endif

private: mutable void *pal; //SOMPAINT_PAL

#include "Somtexture.inl" //"pimpl"
};

class Sompalette
{
public:

	union //blue>green>red layout
	{
		PALETTEENTRY paletteentries[1];

		struct //mirroring PALETTEENTRY members
		{
			BYTE peRed, peGreen, peBlue, peFlags;
		};	  
		struct //Somtexture::palette stores well ID in paint 
		{
			unsigned char bgrRed, bgrGreen, bgrBlue, paint; 
		};
		struct //internal layout (red>green>blue)
		{	
			unsigned char rgbBlue, rgbGreen, rgbRed, alpha; 
		};
	};	
	
	inline Sompalette(){}
	inline Sompalette(int r, int g, int b, int a=0)
	{
		peRed = r; peGreen = g; peBlue = b; peFlags = a;
	}
	inline Sompalette operator!()const //bgr<->rgb
	{
		return Sompalette(peBlue,peGreen,peRed,peFlags);
	}
	inline Sompalette(PALETTEENTRY p)
	{
		*(PALETTEENTRY*)this = p;
	}
	inline operator PALETTEENTRY()const
	{
		return *(PALETTEENTRY*)this; 
	}	
	inline Sompalette operator=(PALETTEENTRY p)
	{
		return *(PALETTEENTRY*)this = p; 
	}	
	inline Sompalette(COLORREF r) //DWORD
	{
		*(COLORREF*)this = r;
	}
	inline operator COLORREF()const
	{
		return *(COLORREF*)this; 
	}
	inline Sompalette operator=(COLORREF r)
	{
		return *(COLORREF*)this = r; 
	}

	//can be passed to Somtexture's members
	inline operator const PALETTEENTRY*()const
	{
		return (const PALETTEENTRY*)this; 
	}
	inline operator PALETTEENTRY*() //the same
	{
		return (PALETTEENTRY*)this; 
	}
};

#ifdef SOMTEXTURE_PROTECTED
#undef private
#endif

#endif //SOMTEXTURE_INCLUDED