
#ifndef SOMPLAYER_VERSION
#define SOMPLAYER_VERSION 0,0,0,4
#define SOMPLAYER_VSTRING "0, 0, 0, 4"
#define SOMPLAYER_WSTRING L"0, 0, 0, 4"
#endif

#ifndef SOMPLAYER_INCLUDED
#define SOMPLAYER_INCLUDED

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

#define SOMPLAYER_LIB(x) Sword_of_Moonlight_Media_Library_##x

#ifdef SOMPLAYER_API_ONLY
#pragma push_macro("SOMPLAYER")
#endif

#define SOMPLAYER struct SOMPLAYER_LIB(console)*

struct SOMPLAYER_LIB(console)
{
#ifndef __cplusplus 
	 /*probably should not rely on this working
	//Use the SOMPLAYER_LIB(Status) API instead*/
	void *vtable; /*debugger*/
#endif

	int status;
	
	float progress; //0~1

	const wchar_t *message; 

#ifdef __cplusplus

	SOMPLAYER_LIB(console)()
	{
		status = 0; message = L""; progress = 0.0;
	}

	/*0: Normal idling status
	//Busy at work if greater
	//Fatal error if negative*/
	inline operator int(){ return this?status:-1; }

	/*compiler: MSVC2005 has trouble calling 
	//virtual members from an object reference*/
	inline SOMPLAYER operator->(){ return this; }

	virtual size_t open(const wchar_t path[MAX_PATH], bool play=true)=0;

	virtual size_t listing(const wchar_t **inout=0, size_t inout_s=0, size_t skip=0)=0;

	virtual bool current(const wchar_t *item, void *reserved=0)=0;

	virtual const wchar_t *change(const wchar_t *play, double time=0)=0;

	virtual bool capture(const wchar_t *surrounding=0)=0;

	virtual const wchar_t *captive()=0;
	virtual const wchar_t *release()=0;

	virtual bool priority(const wchar_t **inout, size_t inout_s)=0;

	virtual double vicinity(double meters=-1, bool visibility=false)=0;

	virtual size_t surrounding(const wchar_t **inout=0, size_t inout_s=0)=0;
	virtual size_t perspective(const wchar_t **inout=0, size_t inout_s=0)=0;

	virtual size_t control(HWND=0, size_t N=0)=0;
	virtual size_t picture(HWND=0, size_t N=0)=0;
	virtual size_t texture(HWND=0, size_t N=0)=0;
	virtual size_t palette(HWND=0, size_t N=0)=0;

	virtual HMENU context(const wchar_t *item=0, HWND hwnd=0, const char *text=0, size_t *ID=0)=0;

#endif
};	  
			
#ifndef SOMPLAYER_API 
#define SOMPLAYER_API static 
#include "../Somversion/Somversion.hpp" 
#define SOMPLAYER_API_FINALE(type,API,prototype,arguments) \
SOMVERSION_HPP_INTERFACE(Somplayer,type,SOMPLAYER_LIB(API),prototype,arguments)
#elif !defined(SOMPLAYER_API_FINALE)
#define SOMPLAYER_API_FINALE(...) ;
#endif

/*A purely descriptive distinction*/
#define SOMPLAYER_THREAD_LOCAL_API SOMPLAYER_API 
				
SOMPLAYER_API /*nonzero on success
//party: to connect to or 0 for your process' default 
//if party is 0 connection is "modal" ortherwise "modeless"
//v can be 0 but it should be the same as the SOMPLAYER_VERSION macro above*/
SOMPLAYER SOMPLAYER_LIB(Connect)(const wchar_t v[4], const wchar_t *party _(0))
SOMPLAYER_API_FINALE(SOMPLAYER,Connect,(const wchar_t[4],const wchar_t*),(v,party))

#ifndef __cplusplus
/*This is for C compatability. Because of the vtable strict C code
//should use this API to get a working pointer to the data members (status, etc.)*/
SOMPLAYER_API SOMPLAYER SOMPLAYER_LIB(Status)(SOMPLAYER c)
SOMPLAYER_API_FINALE(SOMPLAYER,Status,(SOMPLAYER),(c))
#endif

SOMPLAYER_API
/*SOMPLAYER is delete'd on Disconnect (don't use it)
//host: "disconnect" all parties when SOMPLAYER is host
//If a SOMPLAYER is outstanding after a host disconnect the
//Current, Captive, and Release APIs will still work if needed
//for client side cleanup, but otherwise the SOMPLAYER just needs
//to Disconnect as soon as possible so that its memory can be freed*/
void SOMPLAYER_LIB(Disconnect)(SOMPLAYER c, bool host _(false))
SOMPLAYER_API_FINALE(void,Disconnect,(SOMPLAYER,bool),(c,host))

#ifndef SOMPLAYER_NO_THUNKS /*hide C-like API from Somversion.hpp?*/

SOMPLAYER_API /*nonzero on success
//Open a network resource / directory of resources
//play: the first item in the listing will begin playing if true
//Returns size of listing or 0 if the resource could not be opened/played*/
size_t SOMPLAYER_LIB(Open)(SOMPLAYER c, const wchar_t path[MAX_PATH], bool play _(true))
SOMPLAYER_API_FINALE(size_t,Open,(SOMPLAYER,const wchar_t[MAX_PATH],bool),(c,path,play))

SOMPLAYER_API
/*Get the listing for the currently opened resource / directory
//inout_s: the number of wchar_t pointers pointed to by inout (to be filled)
//skip: the number of items in the listing to skip before adding items to inout
//If inout is nonzero Listing returns the number of items returned by way of inout
//Else (if inout is 0) Listing returns the total number of items in the current listing*/
size_t SOMPLAYER_LIB(Listing)(SOMPLAYER c, const wchar_t **inout _(0), size_t inout_s _(0), size_t skip _(0))
SOMPLAYER_API_FINALE(size_t,Listing,(SOMPLAYER,const wchar_t**,size_t,size_t),(c,inout,inout_s,skip))

SOMPLAYER_API /*nonzero on success
//Returns true if item is current (an old item should not be read from)
//If current you can read from it for a second or two: make a copy if you need more time
//reserved: structure for querying the status of item (if 0 the playing listing is queried)*/
bool SOMPLAYER_LIB(Current)(SOMPLAYER c, const wchar_t *item, void *reserved _(0))
SOMPLAYER_API_FINALE(bool,Current,(SOMPLAYER,const wchar_t*,void*),(c,item,reserved))

SOMPLAYER_API /*nonzero on success
//Begin playing one of the items taken from the listing
//Hint: the returned -1 subscript should hold the listing number but no guarantees
//Returns the item that is playing which should be play unless play=0 (shuffle mode)*/
const wchar_t *SOMPLAYER_LIB(Change)(SOMPLAYER c, const wchar_t *play=0, double skip _(0))
SOMPLAYER_API_FINALE(const wchar_t*,Change,(SOMPLAYER,const wchar_t*,double),(c,play,skip))

SOMPLAYER_THREAD_LOCAL_API /*true on success
//Capture the deafult player character if 0 else capture surrounding*/
bool SOMPLAYER_LIB(Capture)(SOMPLAYER c, const wchar_t *surrounding _(0))
SOMPLAYER_API_FINALE(bool,Capture,(SOMPLAYER,const wchar_t*),(c,surrounding))

SOMPLAYER_THREAD_LOCAL_API /*0 on failure 
//Works consistently even when disconnected
//Retrieve the currently captive surrounding
//Note: The returned item is not guaranteed to be fresh (see Current)*/
const wchar_t *SOMPLAYER_LIB(Captive)(SOMPLAYER c)
SOMPLAYER_API_FINALE(const wchar_t*,Captive,(SOMPLAYER),(c))

SOMPLAYER_THREAD_LOCAL_API
/*Works consistently even when disconnected
//Release the currently captive surrounding (if any)
//Returns the surrounding that was released (if any; else 0)
//Note: The returned item is not guaranteed to be fresh (see Current)*/
const wchar_t *SOMPLAYER_LIB(Release)(SOMPLAYER c)
SOMPLAYER_API_FINALE(const wchar_t*,Release,(SOMPLAYER),(c))

SOMPLAYER_THREAD_LOCAL_API /*nonzero on success
//inout is either 0 (no priorities) or a list of priorities...
//"players", "targets", "friends", "dangers", "pickups", "objects"
//A lone model file or resource of some kind will show up as an "object" 
//Notice: the pointers can be replaced with internal pointers for fast parsing*/
bool SOMPLAYER_LIB(Priority)(SOMPLAYER c, const wchar_t **inout, size_t inout_s)
SOMPLAYER_API_FINALE(bool,Priority,(SOMPLAYER,const wchar_t**,size_t),(c,inout,inout_s))

SOMPLAYER_THREAD_LOCAL_API /*0 on failure
//Establish the perimeter radius for discovering surroundings
//If meters is less than 0 the current maximum effective radius is returned
//visibility: clip the vicinity to what the captive is presently able to see
//Upon success the new radius is returned. It may be less than meters. Else 0*/
double SOMPLAYER_LIB(Vicinity)(SOMPLAYER c, double meters _(-1), bool visibility _(false))
SOMPLAYER_API_FINALE(double,Vicinity,(SOMPLAYER,double,bool),(c,meters,visibility))

SOMPLAYER_THREAD_LOCAL_API /*0 on failure
//Fill inout with the current captive's surroundings per some...
//Criteria: The captive itself is not considered to be a surrounding
//Map tiles (and possibly inert objects) will not be seen as surroundings
//Surroundings are sorted by distance from the captive and clipped if not a
//Priority (API above) or fall outside of the specified Vicinity (also above)
//The number of surroundings is returned where inout_s is the maximum desired...
//Except when inout is 0 to query the total (raw) number of surroundings in play*/
size_t SOMPLAYER_LIB(Surrounding)(SOMPLAYER c, const wchar_t **inout _(0), size_t inout_s _(0))
SOMPLAYER_API_FINALE(size_t,Surrounding,(SOMPLAYER,const wchar_t** ,size_t),(c,inout,inout_s))

SOMPLAYER_THREAD_LOCAL_API /*0 on failure
//Same as Surrounding only the output is also clipped to the captive's field of view*/
size_t SOMPLAYER_LIB(Perspective)(SOMPLAYER c, const wchar_t **inout _(0), size_t inout_s _(0))
SOMPLAYER_API_FINALE(size_t,Perspective,(SOMPLAYER,const wchar_t** ,size_t),(c,inout,inout_s))

/*//// Notes on the HWND APIs (that follow below) 
//
// The library APIs here appear deceptively very simple
// However this is because they are designed to largely
// leverage the existing Win32 Windows SendMessage APIs
// 
// WM_SHOWWINDOW: sent to parent when ready to show/hide
// WM_CONTEXTMENU: send to window to open a context menu
// WM_GETTEXT: get the window name for tabs and whatever
// WM_NCDESTROY: send to window to empty/stop management
//
// Everything else is pretty much undocumented. WYSIWYG.
//
// Somplayer.dll does keep track of its HWND handles. If 
// the same handle is passed twice, then the most recent
// API is used to display the window (with the exception 
// of combining a Control window with a Picture window.)
//
// If you pass a window to one of the APIs it is correct
// to assume it is managed by Somplayer.dll. Even if you
// get back a 0. So send it a WM_NCDESTROY to reclaim it 
//
// Note: Sompaste.dll has a factory API that makes static-
// like controls that will be sent input/output messages by  
// the DefWindowProc/DefDlgProc window procedures (Create)
*/
SOMPLAYER_THREAD_LOCAL_API /*nonzero on success
//Forward: see notes on HWND APIs above Contrl API
//Returns 1 if the current captive can be controlled
//Take control of the captive by means of the keyboard etc.
//HWND is a window to be used for the configuration of controls
//N: In theory the captive may allow for more than one controller*/
size_t SOMPLAYER_LIB(Control)(SOMPLAYER c, HWND window _(0), size_t N _(0))
SOMPLAYER_API_FINALE(size_t,Control,(SOMPLAYER,HWND,size_t),(c,window,N))

SOMPLAYER_THREAD_LOCAL_API /*nonzero on success
//Forward: see notes on HWND APIs above Contrl API
//Returns 1 if the current captive can have a picture
//The captive's perspective will be rendered to HWND in real-time
//N: In theory the captive may allow for more than one perspective*/
size_t SOMPLAYER_LIB(Picture)(SOMPLAYER c, HWND window _(0), size_t N _(0))
SOMPLAYER_API_FINALE(size_t,Picture,(SOMPLAYER,HWND,size_t),(c,window,N))

SOMPLAYER_THREAD_LOCAL_API /*nonzero on success
/*Forward: see notes on HWND APIs above Contrl API
//Associate the current captive's Nth texture with HWND*/
//Returns captive's texture count if greater than N
size_t SOMPLAYER_LIB(Texture)(SOMPLAYER c, HWND window _(0), size_t N _(0))
SOMPLAYER_API_FINALE(size_t,Texture,(SOMPLAYER,HWND,size_t),(c,window,N))

SOMPLAYER_THREAD_LOCAL_API /*nonzero on success
//Forward: see notes on HWND APIs above Contrl API
//Associate the current captive's Nth palette with HWND
//Returns captive's texture count if greater than N*/
size_t SOMPLAYER_LIB(Palette)(SOMPLAYER c, HWND window _(0), size_t N _(0))
SOMPLAYER_API_FINALE(size_t,Palette,(SOMPLAYER,HWND,size_t),(c,window,N))

SOMPLAYER_API /*nonzero on success
//Get the context menu and menu item id for a given item within a given window
//
//item: listing or surrounding in question. If window is nonzero the item must match
//window: if nonzero, a window previously passed to one of the four HWND APIs (up above)
//menutext: the desired default ASCII (English) text as it appears in an item in the menu
//If item is 0 a "File" menu is obtained. It can be used to open any kind of file for example
//ID: matching the menutext is stored here; as in ::MENUITEMINFO::wID (not the menu position)
//
//menutext and ID are optional:
//If ID is 0 then a WM_COMMAND message will be sent to window for the ID (as if it is clicked)
//You can use > to denote a sub menu, but it should not be necessary. It will be used to resolve
//ambiguity, but it will not exclude an unambiguous item that matches text to the right of the last >
//
//Submenus: if menutext corresponds to the text of a submenu heading, the submenu is returned
//WARNING: if you rearrange the menu yourself odds are the submenu will be wrong / not found
//
//Notes: conceptually this is as if the user right clicked on the item in the window
//The 0 window is an imaginary window which a client can track by itself if it wants to
//For picture windows the current captive of SOMPLAYER is the subject of the context menu*/
HMENU SOMPLAYER_LIB(Context)(SOMPLAYER c, const wchar_t *item _(0), HWND window _(0), const char *menutext _(0), size_t *ID _(0))
SOMPLAYER_API_FINALE(HMENU,Context,(SOMPLAYER,const wchar_t*,HWND,const char*,size_t*),(c,item,window,menutext,ID))

#endif /*SOMPLAYER_NO_THUNKS*/

#pragma pop_macro("_")

#ifdef SOMPLAYER_API_ONLY
#pragma pop_macro("SOMPLAYER")
#endif

#endif /*SOMPLAYER_INCLUDED*/

/*RC1004*/
