
#ifndef SOMCONSOLE_INCLUDED
#define SOMCONSOLE_INCLUDED

#ifndef SOMCONSOLE_PORTS
#define SOMCONSOLE_PORTS 8
#endif
#ifndef SOMCONSOLE_HEADS
#define SOMCONSOLE_HEADS 1
#endif

class Somplayer;
class Somclient;
class Somcontrol;
class Somspeaker;
class Somdisplay;
class Sominstant;
class Somtexture;
class Somgraphic;
class Somenviron;

namespace Somconsole_h
{
	struct enums 
	{
		//list column keys

		enum //:int
		{
		//FYI: these constants implement a mini DB
		//(that uses wchar_t pointers for handles)

		//level 1, Ex. media[META]
		META = -1, PAGE = -2, LIST = -3, LAST = -4,	
		LOCK = -5, BOOK = -6,
		LV_1 = 6,

		//Note: "level 2" (below) is for META only
		//Everything else is for memory management
		};

		enum
		{
		//level 2, Ex. media[META][SIZE]
		ITEM = -1, SIZE = -2, TYPE = -3, HEAD = -4, 		
		LV_2 = 4,

		//level 3, Ex. media[0...N][TYPE]
		PATH = -5, LINK = -6, HASH = -7, TREE = -8, 
		XREF = -9, 
		LV_3 = 9,

		//cross referencing///////////////////////
		//when XREF is 0 it is the terminator of a
		//variable length function/item index list
		//Note: 0 is a version; not a 0 terminator

		SLOT = LINK, //Somgraphic instance (field)
		};
		
		//list column values

		enum //:wchar_t
		{
		//media types
		_BIN=1, _TXT, //generic

		_PAL, _TXR, //Sword of Moonlight textures

		_MSM, _MDO, _MDL, //Sword of Moonlight models

		_BMP, //dds, png, jpg, tga, dib, hdr, pfm, ppm (D3DX)

		_TIM, _TMD, //Sony PlayStation

		_SOM, //Sword of Moonlight game or recording

		_PRF, _PRT, //Sword of Moonlight game piece

		//field types
		PLAYER, TARGET, FRIEND, DANGER, PICKUP, OBJECT,

		//window types
		CONTROL, PICTURE, TEXTURE, PALETTE, GRAPHIC,

		//list types
		MEDIA, TITLE, FILES, FIELD,
		
		//miscellaneous
		UNDEFINED = wchar_t(-1UL)
		};
	};
}

class Somconsole
:
public Somconsole_h::enums
{	
public: const long key;

	enum
	{
	ports=SOMCONSOLE_PORTS,
	heads=SOMCONSOLE_HEADS,
	};	

protected: //party

	Somplayer *host;
	Somplayer *party;
	Somclient *client; 

	friend class Somclient;
	friend class Somclient2;
	friend class Somclient3;
	friend class Somclient4;
	friend class Somclient5;
	friend class Somclient6;
	//can add more if needed

	//Reminder: these may have to
	//be shared among consoles if
	//registry keys are entangled
	Somcontrol *controls[ports+1];
	Somspeaker *speakers[heads+1];	

protected: //list members

	const wchar_t **media; 
	const wchar_t **listing; 
	const wchar_t **columns;

	const wchar_t **files; 	
	const wchar_t **items; 
	const wchar_t **field; 
	//these index into field
	const wchar_t **players; 
	const wchar_t **targets; 
	const wchar_t **friends; 
	const wchar_t **dangers; 
	const wchar_t **pickups; 
	const wchar_t **objects;

	//each per files[META][SIZE]
	const Somtexture **textures; 
	const Somgraphic **graphics;

public: //only public because static

	static const wchar_t **const empty;

	static void free(const wchar_t **list);
		
	static size_t size(const wchar_t **list)
	{
		return list[META][SIZE]; //just a shorthand
	}
	static const wchar_t **page(const wchar_t **list)
	{
		return (const wchar_t**)list[BOOK]; //get the next page 
	}	
	static bool from(const wchar_t **list, const wchar_t *item) //range condition
	{
		do{ if(item>=list[LIST]&&item<=list[LAST]) break; }while(list=page(list)); 
		
		return list&&item[ITEM]<list[META][SIZE]&&list[item[ITEM]]==item;
	}
	static size_t path(const wchar_t **list, size_t item, wchar_t out[MAX_PATH], wchar_t sep=0)
	{
		if(item>=list[META][SIZE]){ assert(item==UNDEFINED); return *out = 0; }

		const wchar_t *i = list[item]; size_t s = i[SIZE], _ = path(list,i[PATH],out,'\\');

		wmemcpy_s(out+_,MAX_PATH-1-_,i,s); if(out[_+=s]=sep) _++; return _;
	}
	static const wchar_t *link(const wchar_t **list, const wchar_t *inout)
	{			
		assert(list[META][TYPE]==FILES||list[META][TYPE]==MEDIA);
		assert(!inout||from(list,inout)&&(inout[LINK]==UNDEFINED||inout[LINK]<list[META][SIZE]));

		return inout&&inout[LINK]<list[META][SIZE]?list[inout[LINK]]:inout;
	}
	static wchar_t slot(const wchar_t **list, const wchar_t *item, wchar_t new_assignment)
	{
		assert(list[META][TYPE]==FIELD);
		assert(item&&from(list,item)&&(item[SLOT]==UNDEFINED||new_assignment==UNDEFINED));

		return const_cast<wchar_t&>(item[SLOT]) = new_assignment;
	}
	static wchar_t hash(const wchar_t *in, size_t in_s=-1UL) //djb2 
	{			
	    wchar_t out = 5381; //unsigned
		for(int i=0;i<in_s&&in[i];i++) out = ((out<<5)+out)+tolower(in[i]);
		return out; 
	}
	static const wchar_t *find(const wchar_t **list, const wchar_t rel_path[], wchar_t r=-1, int s=MAX_PATH)
	{
		for(int i=0;i<s;i++) //TODO: binary search over TREE
		if(rel_path[i]=='\0'||rel_path[i]=='\\'||rel_path[i]=='/')
		{	
			wchar_t cmp = hash(rel_path,i); for(int j=0,n=list[META][SIZE];j<n;j++)
			if(list[j][HASH]==cmp&&list[j][PATH]==r&&list[j][SIZE]==i&&!wcsnicmp(list[j],rel_path,i)) 
			return rel_path[i]=='\0'?list[j]:find(list,rel_path+i+1,list[j][ITEM],s-i-1);
			break;
		}return 0;
	}
	static void free_and_empty(const wchar_t ***_list=0, void *l2=0, void *l3=0, void *l4=0, void *l5=0, void *l6=0)
	{
		void *x[6] = {_list,l2,l3,l4,l5,l6}; for(int i=0;i<6;i++) if(x[i]){ free(*(const wchar_t***)x[i]); *(void**)x[i] = (void*)empty; }
	}
	static size_t round(size_t sz)
	{
		return sz/32*32+32; //brain dead index allocation
	}

	//TODO: implement types other than file extensions
	static wchar_t type(const wchar_t *file_extension);
		
	static const wchar_t *text(int type); //conversion

	//// implementation //////////////////////

protected: //subroutines

	bool open(const wchar_t *path);

	bool play(int item, double skip);

	bool main(bool soft_reset=false); 

	int view(const wchar_t*, HWND, int type, int n);

	void code(int status){ /*unimplemented*/ };

public: //factory members
	
	Somconsole(); Somplayer *connect(const wchar_t version[4]); 
	
	~Somconsole(); bool disconnect(Somplayer *multiplayer_only); //true on success

private: Somconsole(Somplayer*); Somconsole(const Somconsole&);
 
	friend class Somplayer; //virtual interfaces for Somplayer 

	size_t open_for(Somplayer*, const wchar_t path[MAX_PATH], bool play=true);

	size_t listing_for(Somplayer*, const wchar_t **inout=0, size_t inout_s=0, size_t skip=0);

	const wchar_t *change_for(Somplayer*, const wchar_t *play=0, double skip=0);

	bool current_for(Somplayer*, const wchar_t *item, void *reserved=0);

	bool capture_for(Somplayer*, const wchar_t *surrounding=0);

	const wchar_t *captive_for(Somplayer*);	//console independent
	const wchar_t *release_for(Somplayer*);	//console independent

	bool priority_for(Somplayer*, const wchar_t **inout, size_t inout_s);

	double vicinity_for(Somplayer*, double meters=-1, bool visibility=false);

	size_t surrounding_for(Somplayer*, const wchar_t **inout=0, size_t inout_s=0);
	size_t perspective_for(Somplayer*, const wchar_t **inout=0, size_t inout_s=0);

	size_t control_for(Somplayer*, HWND=0, size_t N=0);
	size_t picture_for(Somplayer*, HWND=0, size_t N=0);
	size_t texture_for(Somplayer*, HWND=0, size_t N=0);
	size_t palette_for(Somplayer*, HWND=0, size_t N=0);

	HMENU context_for(Somplayer*, const wchar_t *item=0, HWND window=0, const char *menutext=0, size_t *ID=0);

	//// some operators //////////////////////

private: Somconsole &operator=(const Somconsole&);

public:	Somplayer *operator*(){ return party; }
				
	//// public utilities ////////////////////

	//lock does two things; it ensures that
	//"this" has not been delete'd and will
	//not be delete'd until it is unlock'ed
	//lock also places the console into its
	//busy state; so a lock should be brief
	bool lock(long key, bool if_busy=true);	
	void unlock(); 

	////WARNING: _to avoid segfault etc_ ////
	//The following members must be bookended
	//by lock() and unlock() as seen up above
	//Exception: the SOMPLAYER routines above
	//assume the player will remain connected
	//Exception: Somclient is implicitly safe

	//Pause to safely take Sominstant snapshots
	//rtc: if 0 the clock is running, be brief, and unpause
	//returns false if rtc is true and the client is not able to pause 
	bool pause(bool rtc=true), unpause(bool rtc=true);

	//fill r with the subitems of q per types t (eg. _BMP, PLAYER, TEXTURE, etc.)
	size_t query(const wchar_t *q, const int *t, size_t t_s, const wchar_t **r, size_t s=0, size_t skip=0);
	
	//some friendlier accessories to query (feel free to add t4 and so on)
	inline const wchar_t *id(const wchar_t *item, int n, int type, int t2=0, int t3=0)
	{
		const int t_s = 3, types[t_s] = {type,t2,t3}; 
		const wchar_t *out = 0; query(item,types,t_s,&out,1,n); return out;
	}
	inline size_t count(const wchar_t *item, int type, int t2=0, int t3=0)
	{
		const int t_s = 3, types[t_s] = {type,t2,t3}; return query(item,types,t_s,0);
	}

	inline const wchar_t *title() //obtain the currently playing listing
	{
		wchar_t i = listing[META][ITEM]; return i==UNDEFINED?0:listing[i];
	}

	//retrieve url of the folder containing item; non-zero on success
	//returns the media or files item for home or L"" if item is a root 
	const wchar_t *home(const wchar_t *item, wchar_t inout[MAX_PATH]=0, bool link=true);

	//retrieve source url or path for link; returns media or files item 
	const wchar_t *source(const wchar_t *item, wchar_t inout[MAX_PATH]=0, bool link=true);

	//Careful: failure returns UNDEFINED if C is nonzero
	inline wchar_t current(const wchar_t *item, int C=0)
	{
		return source(item)&&C<=0&&-C<=item[HEAD]?item[C]:C?UNDEFINED:0;
	}

	//relocate file (sets item[LINK]; may require ammending files)
	bool browse(const wchar_t *item, const wchar_t path[MAX_PATH]);

	//Important: Somtexture::release when done 
	//Important: item must come either from the files, items, or field list
	const Somtexture *pal(const wchar_t *item, int n=0, bool readonly=false);	

	//Important: Somgraphic::release when done 
	//Important: item must come either from the files, items, or field list
	const Somgraphic *mdo(const wchar_t *item, int n=0, bool readonly=false);	
	
	//Important: Somenviron::release when done 
	//point_of_view should return 0 after a map change
	//item: this is a player or NPC that is part of the MPX map
	//blinded: if true the view will initially see a picture background
	const Somenviron *mpx(const wchar_t *item, int n=0, bool blinded=false);
	
	//render the view from Somenviron* to Somtexture *target
	//returns a number that is adjusted whenever the picture changes
	//The number should be fed back to render via st so to avoid any redraw
	//ops: a combination of Somtexture::apply_state constants (eg. 0,SOLID,LIGHT)
	//NEW: target is now a Somdisplay and the old return is stored in its screen member
	bool render(const Somenviron*, const Somdisplay *target, int ops=-1);

	//identical to render except a selection image is drawn to target
	bool select(const Somenviron*, const Somdisplay *target, int ops=-1);

	//get the item at select_target's x by y pixel per select (above)
	//col: returns the specific column (intra graphic piece) for the selected item
	const wchar_t *click(int x, int y, const Somdisplay *select_target, int *col=0);

	//get the default item (name) for a player
	//i: players are assigned an id by name for comparison purposes only	
	const wchar_t *player(long i){ return i>0?L"1UP":L""; }; 

	//get at Somplayer::character and controller respectively per player(i)
	const wchar_t *player_character(long i); int player_controller(long i);
	
	//get an id for name; an id is always generated 
	//name: an empty string string returns -1 and 0 returns 0
	long player_id(const wchar_t *name);
	
	//place a controller in the hands of player(i) playing the part of item per n
	//
	//multitap: expected to be 0 or one of Somcontrol::multitap. If 0 the player must
	//already be holding one or more controllers, the first of which will be returned
	//
	//Note: if multitap refers to the controls of a Somspeaker it is added to speakers
	//instead (the controls are returned) and if there is not room for a controller or 
	//a speaker then any without a player assigned to them will be assigned to the job
	//
	//i: is a player id obtained from somewhere (Somplayer pointers are for clients only)
	//Alternatively i, item, and n can be 0 to add/acquire a Somcontrol matching multitap
	//If the controller is not being held 0 is returned. Negative i's can be placeholders
	//
	//item/n: effectively Somplayer::character / controller per i is assigned to item / n
	//
	//tap will return 0 if the player does not have at least 1 controller and a character
	//If you want a non-character the best you can do is player(i) for item (and 0 for n)
	//A player is allowed to have 0 controllers but if that is the case tap must return 0	
	Somcontrol *tap(const Somcontrol *multitap, long i=0, const wchar_t *item=0, int n=0);

	//choose a point of view for out-of-character input contexts
	//These include local camera adjustments and menu navigation
	bool use(const Somenviron*, long player_id); 

	inline Somcontrol *control(int n){ return n>=0&&n<ports?controls[n]:0; }
	inline Somspeaker *speaker(int n){ return n>=0&&n<heads?speakers[n]:0; }
		
	Somdisplay *display(HWND); //new allocate a display

	//write multitap to HKCU\Software\FROMSOFTWARE\SOM\PLAYER
	//ex/imports to/from HKCU\Software\FROMSOFTWARE\SOM\CONFIG
	//See Somcontrol::export, import, and apply for the details
	bool export(HWND owner); inline bool apply(){ return export(0); }
	bool import(HWND owner);

	#ifdef SOMTEXTURE_INCLUDED
	//one way to get a compatible pool for a render target
	inline int pool(){ return pal(0)->release()->pool(); }
	#endif
};

class Somplayer : public SOMPLAYER_LIB(console)
{
public: //Somclient
	
	//virtuals: Somplayer.h

#define CATCH(...) try{ return __VA_ARGS__; }catch(...){ return 0; }

	size_t open(const wchar_t path[MAX_PATH], bool play=true) 
	{		
		CATCH(console->open_for(this,path,play))
	}
	size_t listing(const wchar_t **inout=0, size_t inout_s=0, size_t skip=0)
	{
		CATCH(console->listing_for(this,inout,inout_s,skip))
	}
	const wchar_t *change(const wchar_t *play=0, double skip=0)
	{
		CATCH(console->change_for(this,play,skip))
	}
	bool current(const wchar_t *item, void *reserved=0)
	{
		CATCH(console->current_for(this,item,reserved))
	}
	bool capture(const wchar_t *surrounding=0)
	{
		CATCH(console->capture_for(this,surrounding))
	}
	const wchar_t *captive()
	{
		CATCH(console->captive_for(this))
	}
	const wchar_t *release()
	{
		CATCH(console->release_for(this))
	}
	bool priority(const wchar_t **inout, size_t inout_s)
	{
		CATCH(console->priority_for(this,inout,inout_s))
	}
	double vicinity(double meters=-1, bool visibility=false)
	{
		CATCH(console->vicinity_for(this,meters,visibility))
	}
	size_t surrounding(const wchar_t **inout=0, size_t inout_s=0)	   
	{
		CATCH(console->surrounding_for(this,inout,inout_s))
	}
	size_t perspective(const wchar_t **inout=0, size_t inout_s=0)
	{
		CATCH(console->perspective_for(this,inout,inout_s))
	}
	size_t control(HWND window=0, size_t N=0)
	{
		CATCH(console->control_for(this,window,N))
	}
	size_t picture(HWND window=0, size_t N=0)
	{
		CATCH(console->picture_for(this,window,N))
	}
	size_t texture(HWND window=0, size_t N=0)
	{
		CATCH(console->texture_for(this,window,N))
	}
	size_t palette(HWND window=0, size_t N=0)
	{
		CATCH(console->palette_for(this,window,N))
	}	
	HMENU context(const wchar_t *item=0, HWND window=0, const char *menutext=0, size_t *ID=0)
	{
		CATCH(console->context_for(this,item,window,menutext,ID))
	}

#undef CATCH
		
	//// lone operator ////////////////////

	//todo: provide some iterator mechanism
	Somplayer *operator*(){ return party; }

	//// disconnect //////////////////////

	~Somplayer(); //scheduled obsolete
	
public: const long id; //Somclient

	//players control one character at a time
	const wchar_t *character; int controller;

	//window that receives camera/menu inputs
	const Sominstant *out_of_character_focus;

private: friend class Somconsole;
	
	Somplayer(Somconsole *join=0);

	Somconsole *console;

	Somplayer *party; 

	long captives; //TLS

	long busy; //atomic
};

#endif //SOMCONSOLE_INCLUDED