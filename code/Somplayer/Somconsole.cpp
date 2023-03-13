
#include "Somplayer.pch.h"

#include "../Sompaste/Sompaste.h"
#include "../lib/swordofmoonlight.h" //play

static SOMPASTE Sompaste = 0;

#include "Somregis.h" 
#include "Somthread.h" 
#include "Somclient.h" 
#include "Somconsole.h"
#include "Somcontrol.h"
#include "Somdisplay.h"
#include "Somtexture.h"
#include "Somgraphic.h"

//scheduled obsolete
#include "Somwindows.h"

namespace Somconsole_cpp
{
	static Somthread_h::account keys;

	static std::map<long,Somconsole*> locks;

	typedef std::map<long,Somconsole*>::iterator lock_it;

	static Somthread_h::account logins;

	static const wchar_t *empty[7] = //LV_1+1
	{
		0,0,0,0,0, L"\0\0\0\0\0\0\0"+6, 0
	}; 
}

const wchar_t **const
Somconsole::empty = Somconsole_cpp::empty+6; //LV_1

extern Somplayer *Somconnect(const wchar_t v[4], const wchar_t *party)
{
	if(party){ assert(0); return 0; } //unimplemented

	return **(new Somconsole);
}

Somconsole::Somconsole() : key(0) //public
{
	new (this) Somconsole(new Somplayer()); //placement new syntax
}
Somconsole::Somconsole(Somplayer *p) : client(0)
,
key(p?++Somconsole_cpp::keys:0), host(p), party(p) 
{			
	if(p) 
	{
		Somthread_h::section cs;

		Somconsole_cpp::locks[key] = p->console = this;
	}

	for(int i=0;i<=ports;i++) controls[i] = 0;
	for(int i=0;i<=heads;i++) speakers[i] = 0;

	assert(empty==Somconsole_cpp::empty+LV_1);

	media = listing = columns = empty;
	files = players = targets = empty;
	items = friends = pickups = empty;
	field = objects = dangers = empty;

	textures = 0; graphics = 0; environ = 0;	
}
Somconsole::Somconsole(const Somconsole &cp) : key(0) //clone
{
	memcpy(this,&cp,sizeof(Somconsole)); (long&)key = 0; //same as equal operator
}
Somconsole &Somconsole::operator=(const Somconsole &cp) //clone
{
	memcpy(this,&cp,sizeof(Somconsole)); (long&)key = 0; //same as copy ctor

	return *this;
}			
	 
Somconsole::~Somconsole() //public
{
	if(!key) return; //temporary object

	//TODO: There should be another thread
	//for deallocating the consoles memory

	host = 0; open_for(party,0,0); 
	{
		Somthread_h::await aw = party->busy;
				
		for(int i=0;i<ports;i++) 
		{
			Somcontrol::tapout(controls[i]);
		}

		//paranoia: the party should be alone 
		for(Somplayer *p=party,*q;p&&p->console;p=q)
		{
			q = p->party; p->party = p;	p->console = 0;
		}

		Somthread_h::section cs;

		Somconsole_cpp::locks.erase(key);
	}

	Sleep(9); //See Somconsole::lock

	//probably not 100% bullet proof
	Somthread_h::await(party->busy);
}

Somplayer::Somplayer(Somconsole *join)
:
id(++Somconsole_cpp::logins), busy(0), captives(0)
{	
	out_of_character_focus = 0;

	character = 0; controller = 0; 

	if(console=join) 
	{	
		party = join->host->party; join->host->party = this;
	}
	else party = this; //party of one
}	
Somplayer::~Somplayer() //public
{
	if(!console->disconnect(this)) delete console;

	if(captives) TlsFree(captives);
}

Somplayer *Somconsole::connect(const wchar_t v[4]) //public
{
	Somthread_h::await aw = party->busy;

	return new Somplayer(this);
}			   
bool Somconsole::disconnect(Somplayer *p) //public
{
	if(!this) return true;
	
	if(p->party==p) return false;

	Somthread_h::await aw = party->busy;

	Somplayer *q = p->party; while(q->party!=p) q = q->party;

	q->party = p->party; p->console = 0;

	return true;
}

size_t Somconsole::open_for(Somplayer *in, const wchar_t path[MAX_PATH], bool and_play)
{
	if(!this||in!=party) return 0; 

	Somthread_h::await aw = party->busy;

	code(client->exit()); //translate exit code

	//closing....
	{
		Somplayer *p = party; //TODO: p->status/message/etc

		if(p) do{ p->capture(0); p = p->party; }while(p!=party);
		
		//Note: I am not sure this can be thread safe without fine grain 
		//reference counting. Hopefully the CATCH blocks in Somconsole.h
		//will allow any segfaults (in other threads) to exit gracefully
		
		if(files!=empty)
		{
			Somthread_h::await aw = files[LOCK]; 
		
			const Somtexture **t = textures; const Somgraphic **g = graphics;

			if(t) for(int i=0,n=files[META][SIZE];i<n;i++) if(t[i]) t[i]->release();
			if(g) for(int i=0,n=files[META][SIZE];i<n;i++) if(g[i]) g[i]->release();
						
			delete [] textures; textures = 0; delete [] graphics; graphics = 0;
		}

		free_and_empty(&players,&targets,&friends,&pickups,&objects,&dangers);		
		free_and_empty(&media,&listing,&columns,&files,&items,&field);		
	}

	if(!path||!open(path)||and_play&&!play(0,0)) return 0;

	if(and_play&&!main()) return 0; //kick off the client
											
	return listing[META][SIZE];
}

size_t Somconsole::listing_for(Somplayer *in, const wchar_t **inout, size_t inout_s, size_t skip)
{	
	if(!this||!in) return 0;

	size_t out = 0, total = listing[META][SIZE]; if(!inout) return total; 
	
	if(skip+inout_s>total) inout_s = total-skip; if(skip>=total) return 0; 

	for(const wchar_t **p=listing+skip;out<inout_s;out++) *inout++ = *p++; 
	
	return out;
}

const wchar_t *Somconsole::change_for(Somplayer *in, const wchar_t *inout, double skip)
{
	if(!this||in!=party) return 0; 

	Somthread_h::await aw = party->busy;

	if(!inout) srand((unsigned)std::time(0)); 
	if(!inout) inout = listing[1+(std::rand()+rand())%listing[META][SIZE]]; //shuffle

	if(!from(listing,inout)||inout[PATH]>=media[META][SIZE]) return 0; //paranoia

	int item = inout[ITEM]; if(item>=listing[META][SIZE]) return 0; //paranoia

	return play(item,skip)&&main()?inout:0;
}

bool Somconsole::current_for(Somplayer *in, const wchar_t *item, void *reserved)
{
	return this&&from(listing,item)||this&&from(field,item);
}

bool Somconsole::capture_for(Somplayer *in, const wchar_t *surrounding)
{
	if(!this) return false;

	if(surrounding) 
	{
		if(!from(field,surrounding)) return false;
	}
	else surrounding = field[1]; //TODO: players

	if(!in->captives) in->captives = TlsAlloc();

	return TlsSetValue(in->captives,(void*)surrounding);	
}

const wchar_t *Somconsole::captive_for(Somplayer *in)
{	
	//Reminder: should be console independent

	return (wchar_t*)TlsGetValue(in->captives);	
}

const wchar_t *Somconsole::release_for(Somplayer *in)
{
	const wchar_t *out = (wchar_t*)TlsGetValue(in->captives);
			 
	TlsSetValue(in->captives,0); return out;
}

bool Somconsole::priority_for(Somplayer *in, const wchar_t **inout, size_t inout_s)
{
	if(!this) return 0; 

	return false; //unimplemented
}

double Somconsole::vicinity_for(Somplayer *in, double meters, bool visibility)
{
	if(!this) return 0; 

	return 0; //unimplemented
}

size_t Somconsole::surrounding_for(Somplayer *in, const wchar_t **inout, size_t inout_s)
{
	if(!this) return 0; 

	size_t out = 0, total = field[META][SIZE]; if(!inout) return total; 
	
	if(inout_s>total) inout_s = total; 

	const wchar_t *captive = in->captive();

	for(const wchar_t **p=field+1;out<inout_s;p++) //1: COM
	{
		if(*p!=captive) //captive is not its own surrounding
		{
			*inout++ = *p; out++; //TODO: priorities and sorting etc
		}
	}
	
	return out;
}

size_t Somconsole::perspective_for(Somplayer *in, const wchar_t **inout, size_t inout_s)
{
	if(!this) return 0; 

	return 0; //unimplemented
}

size_t Somconsole::control_for(Somplayer *in, HWND window, size_t N)
{
	if(!this) return 0; 

	const wchar_t *p = in->captive();

	if(!this||!from(field,p)) return 0; 

	size_t out = view(p,window,CONTROL,N);

	if(out) Somwindows_h::windowman(window)->portholder = in->id;

	if(!controls[0]) import(0); 

	return out;
}

size_t Somconsole::picture_for(Somplayer *in, HWND window, size_t N)
{
	if(!this) return 0; 

	const wchar_t *p = in->captive();

	if(!this||!from(field,p)) return 0; 

	return view(p,window,PICTURE,N);
}

size_t Somconsole::texture_for(Somplayer *in, HWND window, size_t N)
{
	if(!this) return 0; 

	const wchar_t *p = in->captive();

	if(!this||!from(field,p)) return 0; 

	return view(p,window,TEXTURE,N);
}

size_t Somconsole::palette_for(Somplayer *in, HWND window, size_t N)
{
	if(!this) return 0; 

	const wchar_t *p = in->captive();

	if(!this||!from(field,p)) return 0; 

	return view(p,window,PALETTE,N);
}

HMENU Somconsole::context_for(Somplayer *in, const wchar_t *item, HWND window, const char *menutext, size_t *ID)
{
	if(!this) return 0; 

	//TODO: menus should be tailored to *in

	if(!window||!item) return 0; //unimplemented 

	Somwindows_h::windowman wm(window); if(!wm) return 0;

	if(wm->content!=item&&wm->subject!=item) return 0; //mismatch

	if(!menutext) return wm.context();

	int wintype = wm->windowtype;

	if(wintype!=TEXTURE) return 0; //unimplemented
	
	struct val
	{
		int set[3]; //deepest submenu

		val(int a=0, int b=0, int c=0)
		{
			set[0] = a; set[1] = b; set[2] = c; 
		}
	};

	typedef std::map<std::string,val> key;

	static key texmenu; //palmenu, picmenu;

	if(wintype==TEXTURE&&texmenu.empty()) //initialize
	{
		Somthread_h::section cs;

		if(!texmenu.empty()) return context_for(in,item,window,menutext,ID);

		HMENU context = wm.context(); if(!context) return 0; //paranoia

		//Note: we do not allow addressing of top-level menus this way
			
		//TODO: #include "Somcontexts.inl"

		enum 
		{		
			MAIN=1,

			MAIN_FILE=1,
			MAIN_EDIT,
			MAIN_TOOLS,
			MAIN_ZOOM,
			MAIN_SQUARE,
			MAIN_CIRCLE,
			MAIN_POINT,
			MAIN_MODEL,

			MAIN_EDIT_BLACK=1,

			MAIN_FILE_PALETTE=1,

			MAIN_TOOLS_MATTE=1,	
		};

		//Note: could strip ... before lookup
		texmenu["File"] = val(MAIN,MAIN_FILE);
		 texmenu["Folder"]      = val(ID_FILE_FOLDER);
		 texmenu["Browse"]      = val(ID_FILE_OPEN);
		 texmenu["Browse..."]   = val(ID_FILE_OPEN);
		 texmenu["Save"]        = val(ID_FILE_SAVE);
		 texmenu["Save As"]     = val(ID_FILE_SAVE_AS);
		 texmenu["Save As..."]  = val(ID_FILE_SAVE_AS);
		 texmenu["Component"]   = val(ID_FILE_RGB);
		 texmenu["RGB Palette"] = val(MAIN,MAIN_FILE_PALETTE);
		 //Ambiguous with Component above but we want to keep it
		  texmenu["RGB Palette > Component"] = val(ID_FILE_PAL);
		  texmenu["16x16 Tone Map"]          = val(ID_FILE_INDEX);
		  texmenu["Logical Index"]           = val(ID_FILE_PAL);
		  texmenu["Reassign"]                = val(ID_FILE_REASSIGN);
		 texmenu["Alpha Mask"]  = val(ID_FILE_ALPHA);
		 texmenu["Properties"]  = val(ID_FILE_PROPERTIES);
		texmenu["Edit"] = val(MAIN,MAIN_EDIT);
		 texmenu["Flood Fill"] = val(ID_EDIT_FILL);
		 texmenu["Select All"] = val(ID_EDIT_SELECT_ALL);
		 texmenu["Invert"]     = val(ID_EDIT_INVERT);
		 texmenu["Undo"]       = val(ID_EDIT_UNDO);
		 texmenu["Redo"]       = val(ID_EDIT_REDO);
		 texmenu["Cut"]        = val(ID_EDIT_CUT);
		 texmenu["Copy"]       = val(ID_EDIT_COPY);
		 texmenu["Paste"]      = val(ID_EDIT_PASTE);
		 texmenu["Black Mask"]    = val(MAIN,MAIN_EDIT,MAIN_EDIT_BLACK);
		  //texmenu["#000000"]    = val(ID_EDIT_COLORKEY);
		  //texmenu["#080808"]       = val(ID_EDIT_DARKGRAY);
		  //texmenu["Random"]      = val(ID_EDIT_DARKRAND);
		  //texmenu["Saturate"]      = val(ID_EDIT_DARKTONE);
		texmenu["Tools"] = val(MAIN,MAIN_TOOLS);
		 texmenu["Pencil"]  = val(ID_TOOLS_PENCIL);
		 texmenu["Eraser"]  = val(ID_TOOLS_ERASER);
		 texmenu["Marker"]  = val(ID_TOOLS_MARKER);
		 texmenu["Stamp"]   = val(ID_TOOLS_STAMP);
		 texmenu["Stencil"] = val(ID_TOOLS_STENCIL);
		 texmenu["Cursor"]  = val(ID_TOOLS_CURSOR);
		 texmenu["Matte"]   = val(MAIN,MAIN_TOOLS,MAIN_TOOLS_MATTE);
		  texmenu["New Layer"]    = val(ID_TOOLS_NEWLAYER);
		  texmenu["New Layer..."] = val(ID_TOOLS_NEWLAYER);
		  texmenu["Base Layer"]   = val(ID_TOOLS_BASELAYER);		 
		texmenu["Zoom"] = val(MAIN,MAIN_ZOOM);
		 texmenu["1x"]      = val(ID_ZOOM_1X);
		 texmenu["2x"]      = val(ID_ZOOM_2X);
		 texmenu["4x"]      = val(ID_ZOOM_4X);
		 texmenu["8x"]      = val(ID_ZOOM_8X);
		 texmenu["Magnify"] = val(ID_VIEW_FINDER);
		texmenu["Square"] = val(MAIN,MAIN_SQUARE);
		 texmenu["1x1"]    = val(ID_SQUARE_1X1);
		 texmenu["2x2"]    = val(ID_SQUARE_2X2);
		 texmenu["3x3"]    = val(ID_SQUARE_3X3);
		 texmenu["4x4"]    = val(ID_SQUARE_4X4);
		 texmenu["5x5"]    = val(ID_SQUARE_5X5);
		 texmenu["6x6"]    = val(ID_SQUARE_6X6);
		 texmenu["7x7"]    = val(ID_SQUARE_7X7);
		 texmenu["8x8"]    = val(ID_SQUARE_8X8);
		texmenu["Circle"] = val(MAIN,MAIN_CIRCLE);		
		 texmenu["Circle > 4x4"] = val(ID_CIRCLE_4X4);
		 texmenu["Circle > 6x6"] = val(ID_CIRCLE_6X6);
		 texmenu["Circle > 7x7"] = val(ID_CIRCLE_7X7);
		 texmenu["Circle > 8x8"] = val(ID_CIRCLE_8X8);
		texmenu["Point"] = val(MAIN,MAIN_POINT);
		 texmenu["Point > 4x4"] = val(ID_POINT_3X3);
		 texmenu["Point > 5x5"] = val(ID_POINT_5X5);
		 texmenu["Point > 7x7"] = val(ID_POINT_7X7);		 
		texmenu["Model"] = val(MAIN,MAIN_MODEL);
		 texmenu["UV Map"]   = val(ID_UV_MAP);
		 texmenu["Lighting"] = val(ID_LIGHTING);
		texmenu["Paint"] = val(ID_PAINT);
		texmenu["Paint..."] = val(ID_PAINT);
	}

	//TODO: select menu (and use a const ref)
	key::iterator it = texmenu.find(menutext);
		
	int *set = it==texmenu.end()?0:it->second.set;

	if(!set[0])
	{	
		const int n = 60;

		int i; char p[n+1], *q = p; 
		for(i=0;q<p+n-2&&menutext[i]&&i<n;i++,q++)
		{					  
			if((*q=menutext[i])=='>') //normalize menu arrows
			{
				const char *b = menutext+i+1; q--;
				while(q>=p&&(*q==' '||*q=='\t'||*q=='\r'||*q=='\n')) q--;
				while(i++<n&&(*b==' '||*b=='\t'||*b=='\r'||*b=='\n')) b++;
								
				i--; q++; *q++ = ' '; *q++ = '>'; *q = ' '; 
			}
		}

		for(*q=0,i=0;p[i];i++)
		{
			it = texmenu.find(p+i);

			if(it==texmenu.end())
			{
				assert(0); return 0; //don't want to be here
			};
			
			set = it->second.set; if(set[0]) break;
		
			while(p[i]&&p[i]!='>') i++;	i+=p[i]?1:-1;
		}
	}

	UINT id = set[0]; 

	if(id<10&&set[1]&&id) //sub menu
	{
		if(ID) *ID = 0; //paranoia

		HMENU out = wm.context(id-1);

		if(set[1]) out = GetSubMenu(out,set[1]-1);
		if(set[2]) out = GetSubMenu(out,set[2]-1);

		return out;
	}

	if(ID) *ID = id; if(!id) return 0;

	if(!ID)	SendMessage(window,WM_COMMAND,id,(LPARAM)window);

	return wm.context();
}

static void Somconsole_freelist(const wchar_t **ls, size_t s=0)
{
	if(s<32&&ls) //paranoia
	{
		assert(!ls[Somconsole::LOCK]);

		if(!s&&ls==Somconsole::empty) return;

		const void *pg = ls[Somconsole::PAGE]; 
		
		Somconsole_freelist(Somconsole::page(ls),s+1); 
		
		delete [] pg; 
	}
	else assert(!ls); //stack overflow?
}

void Somconsole::free(const wchar_t **list) //static
{
	Somconsole_freelist(list);
}

wchar_t Somconsole::type(const wchar_t *file_ext) //static
{
	if(!file_ext) return _BIN; 
	
	if(*file_ext=='.') file_ext++; 

	wchar_t ext[5] = {0,0,0,0,0};
	
	const int ext_s = sizeof(ext)/sizeof(wchar_t)-1;
								  
	for(int i=0;i<ext_s;i++) ext[i] = toupper(file_ext[i]);

	//could use std::map at this point but type 
	//will _probably_ be a workhorse at some point...

#define MAYBE(XXX,YYY) if(!wcsncmp(ext,L#XXX,ext_s)) return _##YYY;

	switch(*ext)
	{
	case 'B': MAYBE(BMP,BMP) break;
	case 'D': MAYBE(DDS,BMP) 
			  MAYBE(DIB,BMP) break;
	case 'H': MAYBE(HDR,BMP) break;
	case 'J': MAYBE(JPEG,BMP)
			  MAYBE(JPG,BMP) break;
	case 'M': MAYBE(MDL,MDL) 
			  MAYBE(MDO,MDO)
			  MAYBE(MSM,MSM) break;
	case 'P': MAYBE(PAL,PAL)
			  MAYBE(PFM,BMP)
			  MAYBE(PNG,BMP)
			  MAYBE(PPM,BMP) break;
	case 'T': MAYBE(TGA,BMP)
			  MAYBE(TIM,TIM)
			  MAYBE(TXR,TXR) break; 
	}

#undef MAYBE

	return _BIN;
}

const wchar_t *Somconsole::text(int type) //static
{
	switch(type)
	{	 	
	case _PAL: return L"pal"; case _TXR: return L"txr";
	case _TIM: return L"tim"; case _MSM: return L"msm"; 
	case _MDO: return L"mdo"; case _MDL: return L"mdl";	
	case _PRF: return L"prf"; case _PRT: return L"prt"; 
	case _BMP: return L"bmp";

	case PLAYER: return L"player"; case TARGET: return L"target"; 
	case FRIEND: return L"friend"; case DANGER: return L"danger"; 
	case PICKUP: return L"pickup"; case OBJECT: return L"object"; 

	case CONTROL: return L"control"; case PICTURE: return L"picture"; 
	case TEXTURE: return L"texture"; case PALETTE: return L"palette"; 

	default: return L"";
	}
}

bool Somconsole::open(const wchar_t *resource)
{
	wchar_t temp[MAX_PATH];
	wchar_t path[MAX_PATH]; if(!resource) return false;

	if(PathIsRelativeW(resource))
	{
		if(!GetCurrentDirectoryW(MAX_PATH,temp)) return false;

		if(!PathCombineW(path,temp,resource)) return false;
	}
	else wcscpy_s(path,resource);
			
	if(!PathFileExistsW(path)) //optimization?
	{
		SHFILEINFOW nonzero; DWORD_PTR exists = 
		SHGetFileInfoW(path,0,&nonzero,sizeof(SHFILEINFO),0); //SHGFI_ATTRIBUTES

		if(!exists) return false;
	}

	Sompaste->path(path); //canonicalize

	wchar_t *file = PathFindFileNameW(path); 

	if(PathIsDirectoryW(path))
	{
		//TODO: pull in all media (by extension) below directory up to some limit

		assert(0); return false; //unimplemented
	}
	else if(file==path) return false;

	wchar_t type = Somconsole::type(PathFindExtensionW(path));

	switch(type)
	{
	case _MSM: case _MDO: case _MDL: //assert(0);
	case _PAL: case _TXR: case _BMP: case _TIM: break;

	default: assert(0); return false; //unsupported media
	}

	assert(listing==empty&&media==listing);

	Somclient_h::listhelper lh(listing,TITLE), mh(media,MEDIA);
	
	file[-1] = '\0'; //cut filename off from base
		
	mh&=path; mh*=file; mh[TYPE] = type;	

	lh*=file; lh[PATH] = *mh; lh[TYPE] = type;	

	return true; //listhelper does the rest
}

static void Somconsole_play //forward declaring
(Somclient_h::listhelper&,wchar_t(&)[MAX_PATH],wchar_t*,int);

bool Somconsole::play(int item, double skip)
{	
	code(client->exit());

	int type = listing[item][TYPE];

	switch(type)
	{
	case _BMP: break; //D3DXLoadSurfaceFromFile API

	case _TXR: case _TIM: case _PAL: break; //images
	case _MSM: case _MDO: case _MDL: break; //models
		
	default: assert(0); return false; 
	}

	assert(field==empty&&files==field);

	Somclient_h::listhelper fh(field,FIELD), gh(files,FILES);

	Somplayer *p = party; wchar_t up[] = L"1UP"; fh*=L"COM"; //environs[0]
	
	do{ fh*=up; fh[TYPE] = PLAYER; p = p->party; up[0]++; }while(p!=party);

	switch(type)
	{
	default: assert(0); 
		
		return false; //unimplemented?

	case _MSM: case _MDO: case _MDL:
	case _TXR: case _PAL: case _TIM: case _BMP:
	
		wchar_t gpath[MAX_PATH];

		path(media,listing[item][PATH],gpath);

		wchar_t *gpath_name = PathFindFileNameW(gpath); 

		Somconsole_play(gh,gpath,gpath_name,type); //subroutine
		
		fh*=gpath_name; fh[PATH] = *gh; fh[TYPE] = OBJECT; 

		break;
	}

	(wchar_t&)listing[META][ITEM] = item; //now playing

	return true; //listhelper does the rest
}

static void Somconsole_play //Somconsole::play subroutine
(Somclient_h::listhelper &gh, wchar_t (&path)[MAX_PATH], wchar_t *path_name, int type)
{	
	assert(path_name>=path&&path_name<path+MAX_PATH);

	if(path_name!=path) path_name[-1] = '\0'; 

	gh&=path; gh*=path_name; gh[Somconsole::TYPE] = type; 

	if(path_name!=path) path_name[-1] = '\\'; 

	switch(type)
	{
	case Somconsole::_MDL: //FALL THRU

		if(!wcsnicmp(path_name,L"_naked.mdl",11)) return;

	case Somconsole::_MSM: case Somconsole::_MDO: break;		
	case Somconsole::_PAL: case Somconsole::_TXR: 
	case Somconsole::_TIM: case Somconsole::_BMP: return; 
	case Somconsole::_PRF: case Somconsole::_PRT:

		assert(0); return; //unimplemented

	default: assert(0); return; //???
	}

	const int X = 8; //hard limit

	int refs_s = 0, refs_path = -1;

	using namespace SWORDOFMOONLIGHT;
	
	swordofmoonlight_lib_image_t in; 
	swordofmoonlight_lib_reference_t refs[X]; 
	
	switch(type) //0x1: mapping just the headers 
	{
	case Somconsole::_MSM: msm::maptofile(in,path,'r',0x1); 
		
		refs_s = msm::texturereferences(in,refs,X); break;	

	case Somconsole::_MDO: mdo::maptofile(in,path,'r',0x1); 
		
		refs_s = mdo::texturereferences(in,refs,X); break;	

	case Somconsole::_MDL: mdl::maptofile(in,path,'r',0x1); 
		
		const mdl::header_t &hd = mdl::imageheader(in);

		refs_s = !hd?0:hd.timblocks>X?X:hd.timblocks;	
	
		static char *refs_tim[] = 
		{
			"_1.tim", "_2.tim", "_3.tim", "_4.tim", 
			"_5.tim", "_6.tim", "_7.tim", "_8.tim"
		};

		assert(sizeof(refs_tim)==sizeof(void*)*X);

		for(int i=0;i<refs_s;i++) refs[i] = refs_tim[i];
		break;

	}assert(refs_s<=X);

	if(refs_s<1)
	{
		swordofmoonlight_lib_unmap(&in); return;
	}		

	Somclient_h::listhelper_st st0(gh);

	switch(type) //archives 
	{	
	case Somconsole::_MDL: &gh = gh[Somconsole::ITEM];
	}

	wchar_t subitem, wref[MAX_PATH];
	
	HWND owner = Sompaste->window();
	
	size_t path_s = path_name==path?0:path_name-path-1;

	int k = Somconsole::XREF-refs_s*2;

	for(int i=0,j;i<refs_s;i++,k+=2)
	{	
		const char *ref = refs[i];

		for(j=0;ref[j]&&j<MAX_PATH-1;j++) 
			
			wref[j] = ref[j]; wref[j] = '\0';

		gh[k] = Somconsole::TEXTURE; subitem = -1;	

		Somclient_h::listhelper_st st1(gh); //push

		if(gh*=wref) 
		{	
			wchar_t type =
			Somconsole::type(j>3?wref+j-3:0); //lazy			
			switch(gh[Somconsole::TYPE]=type) //linking
			{
			default: assert(0); break; 

			case Somconsole::_TIM: subitem = *gh; break; //MDL
			case Somconsole::_BMP: subitem = *gh; 
				
				wchar_t source[MAX_PATH];
				wmemcpy(source,path,path_s); source[path_s] = '\\'; 				
				wcscpy_s(source+path_s+1,MAX_PATH-path_s-1,wref);
				  
				//TODO: tighten filter depending on what we are playing

				//Locate file via HKCU/Software/FROMSOFTWARE/SOM/DIRECT
				if(Sompaste->folder(owner,source,L"pal; txr; tim; bmp",0))
				{
					Somclient_h::listhelper_st st2(gh); //push

					if(gh&=source) gh[Somconsole::TYPE] =
					Somconsole::type(PathFindExtensionW(source));

					wchar_t link = *gh; st2.pop(); //!

					gh[Somconsole::LINK] = link; //popped
				}
			}			
		}
		else subitem = *gh; st1.pop(); //!
		
		gh[k+1] = subitem; //popped
	}
	gh[k] = 0; //XREF
	
	swordofmoonlight_lib_unmap(&in);

	assert(k==Somconsole::XREF);			
}

//appendix 
static const wchar_t *Somconsole_insert(const wchar_t*** _list, wchar_t (&path)[MAX_PATH])
{
	Sompaste->path(path);

	const wchar_t** &list = *_list;

	assert(list[Somconsole::LOCK]);

	Somclient_h::listhelper lh(list);

	wchar_t *path_name = PathFindFileNameW(path);
		
	int type = Somconsole::type(PathFindExtensionW(path_name));

	Somconsole_play(lh,path,path_name,type); //hack: subroutine 

	int item = *lh; lh.makelist(); 

	assert(item>=0&&item<Somconsole::size(list));

	return item>=0?list[item]:0;
}

//appendix
extern const wchar_t** //Somclient.h
Somconsole_makelist(std::vector<std::wstring> &in, const wchar_t **out, wchar_t type=-1UL)
{		
	if(type==wchar_t(-1UL)&&out) 
	type = out[Somconsole::META][Somconsole::TYPE];

	const size_t count = in.size();
	const size_t out_s = out?Somconsole::size(out):0; assert(out);

	if(count==0) return out?out:Somconsole::empty; //appending

	wchar_t **lo = 0, *hi = 0; //bounds of the page write hole

	if(out&&out[Somconsole::LIST])
	{
		lo = (wchar_t**)out+out_s;
		hi = (wchar_t*)out[Somconsole::LIST]; hi-=hi[Somconsole::HEAD];		
	}
	else assert(out==Somconsole::empty);

	if(out==Somconsole::empty) out = 0;
		
	size_t fill = 0;

	for(size_t i=0;i<count;i++)
	{
		const size_t body = in[i][0];
		const size_t size = in[i].size();

		if(body>size||!body)
		{
			assert(0); return out; //houston we have a problem			
		}

		size_t head = size-body; //+Somconsole::LV_2;
		head = std::max<size_t>(head,Somconsole::LV_3); 

		fill+=head+body;
	}

	const wchar_t *data = 0;

	for(size_t i=0;i<count;i++)
	{
		const size_t body = in[i][0];
		const size_t size = in[i].size();		
		const size_t diff = size-body-Somconsole::LV_2;
		
		size_t head = size-body; 
		head = std::max<size_t>(head,Somconsole::LV_3); 
		
		//collision (time to allocate a new page)
		if(!out||(void*)(lo+1)>(void*)(hi-(head+body)))
		{	
			size_t footprint = fill+Somconsole::LV_2+1;

			size_t index = Somconsole::LV_1+out_s+count;
				
			size_t total = sizeof(wchar_t)*footprint+sizeof(void*)*index;

			assert(total==total/sizeof(wchar_t)*sizeof(wchar_t));

			total/=sizeof(wchar_t); if(out&&total<4096) total = 4096;

			//hack: ensure the page is at least twice the size of the last
			if(out&&total<2*(out[Somconsole::LAST]-out[Somconsole::PAGE]))
			{
				total = 2*(out[Somconsole::LAST]-out[Somconsole::PAGE]);
			}

			wchar_t *page = new wchar_t[total];	
						
			wchar_t **list = (wchar_t**)page+Somconsole::LV_1;	 
			
			for(int j=0;j<=Somconsole::LV_1;j++) list[-j] = 0; //paranoia	
			
			wchar_t *meta = page+total-1; meta[0] = '\0';

			list[Somconsole::META] = meta; //1
			list[Somconsole::PAGE] = page; //2
			list[Somconsole::LIST] = meta-Somconsole::LV_2-body; //3
			list[Somconsole::LAST] = meta-Somconsole::LV_2-body; //4
			list[Somconsole::LOCK] = 0; //5
			list[Somconsole::BOOK] = (wchar_t*)out; //6			

			assert(Somconsole::LV_1==6);

			meta[Somconsole::ITEM] = -1UL; //1
			meta[Somconsole::SIZE] = out_s; //2
			meta[Somconsole::TYPE] = type; //3 
			meta[Somconsole::HEAD] = 4;	//4

			assert(Somconsole::LV_2==4);
			
			out = reinterpret_cast<const wchar_t**>
			(out?memcpy(list,out,sizeof(void*)*out_s+i):list); 

			lo = list+out_s+i; hi = meta-Somconsole::LV_2; 				   						
		}	

		lo++; hi-=head+body; fill-=head+body;

		wchar_t* &item = 
		const_cast<wchar_t*&>(out[out_s+i]); 
		
		data = in[i].data();		
		item = wcsncpy(hi+head,data+1,body-1); 
		item[body-1] = '\0';

		const int ITEM = body-Somconsole::ITEM-1;
		const int SIZE = body-Somconsole::SIZE-1;
		const int TYPE = body-Somconsole::TYPE-1;
		
		assert(data[ITEM]==i&&data[SIZE]==body-1); //paranoia

		////LV_2//////////////
		item[Somconsole::ITEM] = i;
		item[Somconsole::SIZE] = body-1;
		item[Somconsole::TYPE] = data[TYPE]; //NEW 
		item[Somconsole::HEAD] = head;		
		////LV_3 paranoia/////
		item[Somconsole::PATH] = -1UL;
	  //item[Somconsole::TYPE] = -1UL; TYPE is LV_2 now//
		item[Somconsole::LINK] = -1UL; 		
	  //item[Somconsole::HASH] = Somconsole::hash(data+1,body);
	  //item[Somconsole::TREE] = -1UL; //Somconsole_sortlist
		item[Somconsole::XREF] = -1UL;

		if(diff) //// LV_3 ////
		{	   				
			wchar_t *p = item-Somconsole::LV_2-1;

			const wchar_t *d = data+body+Somconsole::LV_2;

			for(size_t j=0;j<diff;j++) *p-- = *d++;
		}
	
		item[Somconsole::HASH] = Somconsole::hash(item);
		item[Somconsole::TREE] = -1UL; 

		out[Somconsole::LIST] = item;
	}

	const_cast<wchar_t&>
	(out[Somconsole::META][Somconsole::SIZE])+=count;

	assert(fill==0&&Somconsole::size(out)==out_s+count);

	return out;
}	

bool Somconsole::main(bool soft_reset)
{
	assert(party->busy);

	if(!soft_reset) client->exit();

	bool out = client->main(this); assert(out);

	return out;
}

int Somconsole::view(const wchar_t *it, HWND window, int type, int n)
{	
	int out = count(it,type);
	
	if(!out||out<=n||!window) return !window?out:0; 
	
	if(GetCurrentThreadId()!=
	   GetWindowThreadProcessId(window,0))
	{
		//// would need a critical section if ////
		//// the window doesn't belong to the ////
		//// current thread (pretty unlikely) ////

		assert(0); return 0; //should implement it
	}
	else
	{
		Somwindows_h::windowman wm(window); 	
		
		if(!wm.subclass(it,type,n)) return 0;		
		if(!wm.attachto(this,false)) return 0;
		
		SetWindowTextW(window,source(id(it,n,type))); 

		wm.resize(); //initialize layout

		ShowWindow(GetParent(window),SW_SHOW); //debugging					

		ShowWindow(window,SW_SHOW); //testing
	}

	return out;
}

bool Somconsole::lock(long key, bool if_busy)
{	
	if(!this) return false;

	//Alternatively: consoles could just
	//remain in memory until the library 
	//is unloaded: but this seems likely
	//to be more responsible; if say the
	//library remains in memory for days
	//or longer (which is a possibility)	
	{
		Somthread_h::section cs;
		Somconsole_cpp::lock_it it = 
		Somconsole_cpp::locks.find(key);				
		if(it!=Somconsole_cpp::locks.end())
		{
			assert(it->second==this); 
			
			if(it->second!=this) return false;
		}
		else return false;
	}
	//Otherwise we expect the console to 
	//be around for at least a second yet...

	//reminder: have a phantom party sit on
	//a disconnected console until deletion
	if(!if_busy&&party->busy) return false;

	Somthread_h::await aw = party->busy;
	Somthread_h::await_lock(party->busy);
	
	return true; //...	
}
void Somconsole::unlock()
{
	Somthread_h::await_unlock(party->busy);
}

bool Somconsole::pause(bool rtc)
{
	assert(0); return false; //unimplemented
}

bool Somconsole::unpause(bool rtc)
{
	assert(0); return false; //unimplemented
}

static size_t Somconsole_subquery //forward declaring
(const wchar_t**,const wchar_t*,const int*,size_t,const wchar_t**,size_t,size_t,int);

size_t Somconsole::query(const wchar_t *in, const int *t, size_t t_s, const wchar_t **inout, size_t inout_s, size_t skip)
{		
	if(!inout) inout_s = 0;

	if(!in||!t||inout&&!inout_s) return 0; 

	size_t out = 0;	

	const wchar_t **f = 0, **g = 0, *q = in;
	
	if(!from(f=field,in)&&!from(f=listing,in))
	{			
		if(!from(g=files,in)&&!from(g=media,in)) return 0;
	}
	else //top-level query
	{
		for(int i=0;i<t_s;i++)
		{
			bool cont = true;

			switch(t[i])
			{
			case 0: i = t_s; break;

			case CONTROL: case PICTURE:

			//Assuming one of the following:
			//PLAYER TARGET FRIEND DANGER PICKUP OBJECT
			//Also assuming any one of these applicable

				cont = f!=field; break;

			default:

				if(in[TYPE]==t[i]) out++;
			}

			if(cont) continue;
			
			if(!skip&&inout) 
			{
				inout[0] = in; assert(inout_s);			
				
				if(inout_s==1) return 1;
			}
			
			out++; break;
		}

		g = f==field?files:media;

		if(in[PATH]>=g[META][SIZE]) return out; //-1

		q = g[in[PATH]];
	}		

	size_t m = out>skip?out-skip:0;
	size_t n = out>skip?0:skip-out; if(!inout) m = 0;

	out+=Somconsole_subquery(g,q,t,t_s,inout+m,inout_s-m,n,0);
	
	if(out<=skip) return 0;

	assert(!inout||out-skip<=inout_s);

	return out-skip;
}

static size_t //Somconsole::query subroutine
Somconsole_subquery(const wchar_t **list, const wchar_t *in, 					
					const int *t, size_t t_s, const wchar_t **inout, size_t inout_s, size_t skip, int subd)
{	size_t out = 0;
		
	for(int i=0;i<t_s;i++)
	{
		bool cont = true;

		switch(t[i])
		{
		case 0: //zero terminated
			
			i = t_s; break; 

		case Somconsole::TEXTURE: 
		case Somconsole::PALETTE:

			switch(in[Somconsole::TYPE])
			{
			case Somconsole::_PAL: case Somconsole::_TXR: 
			case Somconsole::_TIM: case Somconsole::_BMP: 

					cont = false; 
			}break;

		case Somconsole::GRAPHIC:

			switch(in[Somconsole::TYPE])
			{
			case Somconsole::_MDO: case Somconsole::_MDL: 
			case Somconsole::_MSM: case Somconsole::_TMD: 

					cont = false; 
			}break;

		default: if(!cont) break;
			
			cont = in[Somconsole::TYPE]!=t[i];
		}

		if(cont) continue;
		
		if(inout&&out>=skip)
		{
			size_t n = out-skip;

			if(n<inout_s) inout[n] = in;

			if(n>=inout_s-1) //full
			{
				assert(n==inout_s-1);
				return n+1; 
			}
		}
		
		out++; break;
	}

	const wchar_t *ln = Somconsole::link(list,in);
	
	in = 0; if(!ln||subd>2) return 0; //paranoia

	if(ln[Somconsole::XREF]!=0) return out;

	const wchar_t *p = ln-ln[Somconsole::HEAD];		

	for(const wchar_t *q=ln+Somconsole::XREF;p<q;p+=2)
	{
		if(p[1]<list[Somconsole::META][Somconsole::SIZE]) //paranoia
		{
			size_t m = out>skip?out-skip:0;
			size_t n = out>skip?0:skip-out; if(!inout) m = 0;

			out+=Somconsole_subquery(list,list[p[1]],t,t_s,inout+m,inout_s-m,n,subd+1);

			if(inout&&out-skip>=inout_s) break;
		}
	}

	return out;
}	  

const wchar_t *Somconsole::home(const wchar_t *item, wchar_t inout[MAX_PATH], bool ln)
{
	const wchar_t *out = 0, **list = 0, *f = source(item,0,ln);

	if(!from(list=files,f)&&!from(list=media,f)) return 0;
	
	static const wchar_t root[] = {0,0,0,0,0,0}; //paranoia #1

	static int compiler_assert[sizeof(root)/sizeof(wchar_t)-LV_2];

	if(f[PATH]==UNDEFINED) return root+LV_2; //paranoia #2
	
	if(f[PATH]>=list[META][SIZE]) return 0; //extreme paranoia

	out = list[f[PATH]]; return !inout?out:source(out,inout,ln);
}

const wchar_t *Somconsole::source(const wchar_t *item, wchar_t inout[MAX_PATH], bool ln)
{	
	const wchar_t *out = 0, **list = 0;	

	if(!from(list=files,item)&&!from(list=media,item)) 
	if(from(list=field,item)||from(list=listing,item))
	{	
		list = list==field?files:media;

		if(item[PATH]>=list[META][SIZE]) return 0; //paranoia

		item = list[item[PATH]];
	}
	else return 0; 
	
	out = ln?link(list,item):item;

	if(!inout||!out) return out;

	path(list,out[ITEM],inout); 

	Sompaste->path(inout);

	return out;
}

template <typename T>
static void Somconsole_prepare(T* &inout, size_t round)
{
	assert(inout==0&&sizeof(T)==sizeof(void*)); 
	
	if(!inout) memset(inout=new T[round],0x00,sizeof(T)*round);
}

template <typename T>
static void Somconsole_reserve(T* &inout, size_t before, size_t after)
{
	if(!inout) return; //defer allocation
	
	assert(sizeof(T)==sizeof(void*)&&after>before); 

	T *cp = new T[after]; memcpy(cp,inout,sizeof(T)*before);

	memset(cp+before,0x00,sizeof(T)*(after-before)); 

	delete [] inout; cp = inout;
}

bool Somconsole::browse(const wchar_t *item, const wchar_t _path[MAX_PATH])
{
	if(files==empty) return false;

	Somthread_h::await aw = files[LOCK];

	if(!from(files,item)) return false; 

	if(PathIsRelativeW(_path)) return false;

	wchar_t path[MAX_PATH]; wcscpy_s(path,_path);

	size_t r = round(files[META][SIZE]); 

	const wchar_t *link = Somconsole_insert(&files,path);
	
	size_t s = round(files[META][SIZE]); 

	if(r<s) Somconsole_reserve(textures,r,s);
	if(r<s) Somconsole_reserve(graphics,r,s);

	const_cast<wchar_t&>(item[LINK]) = link[ITEM]; 	

	if(textures) textures[item[ITEM]]->refresh(path);
	if(graphics) graphics[item[ITEM]]->refresh(path); 
	
	return true;
}

const Somtexture *Somconsole::pal(const wchar_t *item, int n, bool readonly)
{		
	if(files==empty) return 0;

	//TODO: restrict locking to writing
	Somthread_h::await aw = files[LOCK];

	const wchar_t *t = id(item,n,TEXTURE);
		
	if(!textures)
	{
		size_t r = round(size(files)); 

		Somconsole_prepare(textures,r);

		textures[0] = Somtexture::open(0,0); 
	}  

	int f = t?t[ITEM]:0; //textures and files are symmetric

	if(!textures[f]
	 ||!readonly&&textures[f]->readonly()&&f)
	{
		wchar_t src[MAX_PATH] = L""; source(t,src); 
		
		if(!textures[f])
		{
			if(readonly) textures[f] = textures[0]->open(src);
			if(!readonly) textures[f] = textures[0]->edit(src); 
		}
		else textures[f]->refresh(src,MAX_PATH,'edit');
	}

	if(!textures[f]) //assign nul texture
	{
		textures[f] = textures[0]; assert(0);
	}			

	textures[f]->addref();

	assert(textures[f]);
	return textures[f];
}

const Somgraphic *Somconsole::mdo(const wchar_t *item, int n, bool readonly)
{	
	if(files==empty||n!=0) return 0;

	//Somthread_h::await aw = files[LOCK];

	if(!textures) pal(0); //hack: establish pool...

	//TODO: restrict locking to writing
	Somthread_h::await aw = files[LOCK];

	const wchar_t *g = id(item,n,GRAPHIC);
		
	if(!graphics)
	{			
		size_t r = round(size(files)); 

		Somconsole_prepare(graphics,r);

		int pool = textures[0]->pool(); //hack

		graphics[0] = Somgraphic::open(pool,0); 
	}  

	int f = g?g[ITEM]:0; //graphics and files are symmetric

	if(!graphics[f]
	 ||!readonly&&graphics[f]->readonly()&&g)
	{
		wchar_t src[MAX_PATH] = L""; source(g,src); 
		
		if(!graphics[f])
		{
			if(readonly) graphics[f] = graphics[0]->open(src);
			if(!readonly) graphics[f] = graphics[0]->edit(src); 
		}
		else graphics[f]->refresh(src,MAX_PATH,'edit');

		//hack: the .MDL format embeds its textures 
		wchar_t root = g[TYPE]==_MDL?g[ITEM]:g[PATH]; 

		const wchar_t *rgb; //enumerate colour textures

		const int refs_s = 8; const Somtexture *refs[refs_s];

		for(int i=0;i<refs_s;i++) if(rgb=graphics[f]->reference(i))
		{
			const wchar_t *t = find(files,rgb,root); 

			int f = t?t[ITEM]:0; //shadowing

			if(!textures[f]
			||!readonly&&textures[f]->readonly()&&f)
			{
				source(t,src);

				if(!textures[f])
				{
					if(readonly) textures[f] = textures[0]->open(src);
					if(!readonly) textures[f] = textures[0]->edit(src); 
				}
				else textures[f]->refresh(src,MAX_PATH,'edit');
			}

			refs[i] = textures[f];
		}
		else refs[i] = 0; 

		graphics[f]->link(refs,refs_s);
	}

	if(!graphics[f]) //assign nul graphic
	{
		graphics[f] = graphics[0]; assert(0);
	}			

	graphics[f]->addref();

	assert(graphics[f]);
	return graphics[f];
}

const Somenviron *Somconsole::mpx(const wchar_t *item, int n, bool blind)
{
	const Somenviron *out = from(field,item)?client->watch(item,n):0;

	if(out&&blind) client->blind(out); return out;
}

bool Somconsole::render(const Somenviron *env, const Somdisplay *dst, int ops)
{	
	if(!this||!env||!dst) return false; assert(party->busy);

	////todo: generalize this stuff for use by Somconsole::select ////
		
	if(!dst->picture||dst->picture&&dst->picture->pool()!=pool())
	{			
		dst->picture->release(); dst->picture = 0; 
		
		dst->picture = Somtexture::open(pool(),0); assert(dst->picture);

		if(!dst->picture) return false;			
	}

	if(dst->width!=dst->picture->width||dst->height!=dst->picture->height)
	{		
		int x = dst->width, y = dst->height, tx = 0, ty = 0; if(!x||!y) return false;

		if(dst->picture->width){ tx = x/128*128+128; ty = y/128*128+128; } //128: guard band

		if(!dst->picture->format(Somtexture::RGB,x,y,32,Somtexture::RGB8,0,tx,ty)) return false;

		dst->screen = 0; //force refresh
	}
	else if(dst->content!=&Somconsole::render||dst->renderops!=ops)
	{
		dst->screen = 0; //force refresh
	}

	int fresh = client->frame(env); 

	if(dst->screen==fresh||!fresh) return fresh;

	if(!client->shape((Somenviron*)env,dst->picture)) return false;

	dst->content = &Somconsole::render; dst->renderops = ops; 
	dst->screen = fresh; 

	//////////////////////////////////////////////////////////////////

	client->layer(0,(Somenviron*)env,dst->picture,0); //testing

	return true;
}

long Somconsole::player_id(const wchar_t *name)
{
	if(!name) return 0; if(!*name) return -1; 
	
	if(wcscmp(name,L"1UP")){ assert(0); return 0; } //unimplemented

	return party?party->id:0; //toy implementation
}

Somcontrol *Somconsole::tap(const Somcontrol *mt, long pid, const wchar_t *item, int n)
{
	Somcontrol *out = 0;
	
	if(mt)
	{
		size_t i = 0;

		for(i=0;i<Somconsole::ports;i++) if(controls[i])
		{			
			if(controls[i]->tap==mt->tap){ out = controls[i]; break; }
		}
		else break;
		 		
		if(!out) for(i=0;!out&&i<Somconsole::ports;i++) if(controls[i])
		{
			if(!controls[i]->portholder) *(out=controls[i--]) = *mt; 
		}
		else out = controls[i--] = new Somcontrol(*mt); 			

		if(out) while(i&&controls[i-1]->tap>out->tap)
		{
			std::swap(controls[i-1],controls[i]); i--; //sort
		}

		if(pid&&out&&out->portholder!=pid)
		{
			out->reset(player(out->portholder=pid));
		}

		if(out&&!out->portholder) out = 0;

		if(!item) return out;
	}

	//todo: tighten this stuff up with regard to tap's comments

	Somplayer *p = party;
	while(p->party!=party&&p->party!=p&&p->id!=pid) p = p->party;

	if(p->id!=pid) return out; 

	if(!out) for(int i=0;i<ports;i++)
	{
		 if(controls[i]&&controls[i]->portholder==pid)
		 {
			 if(*controls[i]) out = controls[i]; break;
		 }
	}
	
	if(item)
	{
		p->character = item; p->controller = n;
	}

	return out;
}

bool Somconsole::use(const Somenviron*, long player_id)
{
	return true; //unimplemented
}

Somdisplay *Somconsole::display(HWND hwnd)
{
	return new Somdisplay; //todo: import session
}

#define Somconsole_SOM L"Software\\FROMSOFTWARE\\SOM"

namespace Somconsole_cpp
{			  
	typedef Somregis_h::multi_sz<Somconsole*,128> multi_sz;
}

inline int Somconsole_cpp::multi_sz::f(Somconsole *in, wchar_t (&ln)[128], size_t &i)
{		
	//Multitap
	//1UP, {BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}

	if(i==0) return wcscpy(ln,L"Multitap"), 8;
	
	Somcontrol *p = in->control(i-1); if(!p||!*p) return 0;

	const wchar_t *pid = p->persistent_identifier(), *player = in->player(p->portholder);
	
	if(*player&&*pid) return swprintf_s(ln,L"%s, %s",player,pid);

	return f(in,ln,++i); 
}

inline int Somconsole_cpp::multi_sz::g(Somconsole *out, wchar_t (&ln)[128], size_t &i)
{
	if(i==0) //unplug everything
	{
		for(int i=0;i<out->ports;i++)
		{
			if(!out->control(i)->unplug()) break;
		}

		//this is effectively a versioning line
		return !wcsncmp(ln,L"Multitap",9)?1:-1;
	}
	else if(!ln[0]) return 0; //last line

	//we peel the controller id off the back 
	//so that usernames can use any character

	wchar_t *pid = wcsrchr(ln,','); //GUID

	if(!pid||pid[1]!=' '||!pid[2]) return -1;
	
	*pid = 0; //0 terminate the player's name
		
	const Somcontrol *scan = 0; pid+=2; //GUID

	for(int i=0;scan=Somcontrol::multitap(i);i++)
	{
		if(!wcscmp(pid,scan->persistent_identifier())) 
		{
			out->tap(scan,out->player_id(ln)); break;
		}
	}

	return 1; //keep going
}
 
bool Somconsole::export(HWND owner)
{
	if(owner) //TODO: Export dialog
	{
		assert(0); return false; //unimplemented
	}

	const size_t subkey_s = 128; 	

	wchar_t subkey[subkey_s] = L"", *config = L"PLAYER";

	if(owner) config = L"CONFIG"; //TODO: allow alternative key

	int cat = swprintf_s(subkey,L"%s\\%s",Somconsole_SOM,config);
	
	Somconsole_cpp::multi_sz wr = this; if(wr.cb<sizeof(wchar_t)*2) return false;
	
	const wchar_t *home = L""; //TODO: get the current project/game directory

	LONG err = SHSetValueW(HKEY_CURRENT_USER,subkey,home,REG_MULTI_SZ,wr.sz,wr.cb);

	return err==ERROR_SUCCESS;	
}

bool Somconsole::import(HWND owner)
{
	if(owner) //TODO: Imprt dialog
	{
		assert(0); return false; //unimplemented
	}
	else Somcontrol::multitap(Somcontrol::DISCOVER);
		   
	const size_t subkey_s = 128; 	

	wchar_t subkey[subkey_s] = L"", *config = L"PLAYER";

	if(owner) config = L"CONFIG"; //TODO: allow alternative key

	int cat = swprintf_s(subkey,L"%s\\%s",Somconsole_SOM,config);
	
	const wchar_t *home = L""; //TODO: get the current project/game directory

	DWORD cb = 0; LONG err = 
	SHGetValueW(HKEY_CURRENT_USER,subkey,home,0,0,&cb);

	if(!cb||err) return false;

	Somconsole_cpp::multi_sz rd(this,cb); err = 
	SHGetValueW(HKEY_CURRENT_USER,subkey,home,0,rd.sz,&rd.cb);	

	return err==ERROR_SUCCESS;
}