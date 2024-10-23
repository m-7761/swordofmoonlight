
#ifndef SOMPASTE_VERSION
#define SOMPASTE_VERSION 1,0,2,10
#define SOMPASTE_VSTRING "1, 0, 2, 10"
#define SOMPASTE_WSTRING L"1, 0, 2, 10"
#endif

//ISSUES //ISSUES //ISSUES
//The Choose Color procedure should call its callback
//from the originating thread. Various APIs and things
//won't work across threads.

#ifndef SOMPASTE_INCLUDED
#define SOMPASTE_INCLUDED

/*C linkage to types bool etc is left as 
//an exercise to whoever wants to link it!*/

/*TODO list
*try to use WM_QUERYUISTATE to override DS_CENTERMOUSE
*do something: Desktop is unimplemented; up in the air
*/

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#pragma push_macro("_")
#ifndef __cplusplus
#define _(default_argument)
#else
#define _(default_argument) = default_argument 
#endif

#define SOMPASTE_LIB(x) Sword_of_Moonlight_Client_Library_##x

#ifdef SOMPASTE_API_ONLY
#pragma push_macro("SOMPASTE")
#endif
struct SOMPASTE_LIB(module);
#define SOMPASTE struct SOMPASTE_LIB(module)*

#ifndef SOMPASTE_API 
#define SOMPASTE_API static 
#include "../Somversion/Somversion.hpp" 
#define SOMPASTE_API_FINALE(type,API,prototype,arguments) \
SOMVERSION_HPP_INTERFACE(Sompaste,type,SOMPASTE_LIB(API),prototype,arguments)
#elif !defined(SOMPASTE_API_FINALE)
#define SOMPASTE_API_FINALE(...) ;
#endif

SOMPASTE_API /*nonzero on success
//Set: if nonzero sets the client window. As long as the
//HWND remains valid it cannot be overwritten once it is set
//The current value of set is returned
//
//SetProp is used to establish some basic facts about the 
//client application. The following are officially recognized:
//
//Sompaste_caption: the app's undecorated title. Eg. L"PalEdit"
//Sompaste_history: the maximum number of undos to keep track of
//Sompaste_apponly: only the client app can issue undo directives*/
HWND SOMPASTE_LIB(Window)(HWND set _(0))	  
SOMPASTE_API_FINALE(HWND,Window,(HWND),(set))

SOMPASTE_API
/*For whatever reason Windows does not provide an API for querying
//the position of a window in client space; even though many an API
//are provided for setting these parameters. 
//
//Tip: just think GetWindowPos (what a concept!)
//
//window: is the window for which the position (extents) is desired
//client: is the window with the client area in which RECT will reside
//If client is 0, client is understood to be the parent (GetParent) of win
//frame: if true the bounding rectangle (non-client/border/all) is returned
//If false the rectangle is limited to the client area of the window*/
RECT SOMPASTE_LIB(Pane)(HWND window, HWND client _(0), bool frame _(false))
SOMPASTE_API_FINALE(RECT,Pane,(HWND,HWND,bool),(window,client,frame))

SOMPASTE_API
/*Converts inout into a pretty/canonical file system path
//FYI: Path pretends it doesn't know the current directory 
//The return is inout itself as a simple matter of courtesy*/
wchar_t *SOMPASTE_LIB(Path)(wchar_t inout[MAX_PATH])
SOMPASTE_API_FINALE(wchar_t*,Path,(wchar_t[]),(inout))

SOMPASTE_API /*nonzero on success
//A simple generic window factory. This is mainly because win32
//STATIC windows do not receive input/ouput events. So we want a static
//like window that is mouse aware so that it is good for much of anything
//The window will be a 0,0 mote at 0,0 with DefWindowProc and not much more
//Specifically: CS_PARENTDC DefWindowProc IDC_ARROW NULL_BRUSH WS_CHILD
//CS_DBLCLKS: omitted (want to discourage use of double clicks)*/
HWND SOMPASTE_LIB(Create)(HWND parent, int ID _(0))
SOMPASTE_API_FINALE(HWND,Create,(HWND,int),(parent,ID))

SOMPASTE_API
/*Propsheet creates a dialog that mimics PropertySheet 
//The return points to tabs+1 HWNDs. 0~tabs-1 are the "tabs"
//The final HWND is the container itself. The container simulates
//the Windows property sheets API, where each tab slot is a 0 based ID
//And each window in the slot is the owner of that ID. Tabs can share windows
//When a window becomes a child of the propsheet (WM_PARENTNOTIFY) it is assigned
//the the next tab available. It can add itself to additional tabs in its WM_INITDIALOG 
//phase. The title of the window initializes the tab text (but that is useless for dialogs) 
//PSM_GETTABCONTROL must be used to set the tab text of dialogs and tabs sharing a common window
//The dialog window keeps the HWND* in GetProp("Sompaste_propsheet") and does not need anything more
//
//FYI: the Apply button should use ID_APPLY_NOW from afxres.h (0x3021) in case you need to query its state*/
HWND *SOMPASTE_LIB(Propsheet)(HWND owner, int tabs, const wchar_t *title _(L"Properties"), HMENU menu _(0))
SOMPASTE_API_FINALE(HWND*,Propsheet,(HWND,int,const wchar_t*,HMENU),(owner,tabs,title,menu))

SOMPASTE_API
/*There can be only one "paper" window per process. It is a top-level
//window which must be closed before any other top-level window can be 
//activated. Windows provides the functionality for a single modal dialog
//with a single owner, but doesn't seem to have an equivalent funcitonality
//for a dialog with "multiple" owners. So that is basically what Paper is for
//
//To get the current paper window the window argument can be 0
//When setting Paper will test that the window is valid/visible before 
//returning the window. The set paper window if any is sent WM_CLOSE
//If the current paper does not close the new paper is ignored
//
//Note: the current paper is returned regardless...
//You should check the return and destroy your own window if you want
//_there should be a paper queue, but one is not yet implemented_
//
//msg: if nonzero the MSG is filtered according to best practices
//If the return is nonzero the MSG should be discarded by the app
//Note: msg.hwnd is returned and window is ignored*/
HWND SOMPASTE_LIB(Wallpaper)(HWND window _(0), MSG *msg _(0))
SOMPASTE_API_FINALE(HWND,Wallpaper,(HWND,MSG*),(window,msg))

SOMPASTE_API
/*Sword of Moonlight's many file formats include references to external files with file titles
//rather than absolute paths to the files. Traditionally the place to look for the external files
//is hardwired; either they are in the registered install file tree or inside of a game's file tree
//
//This is all well and good, but it tends to make Sword of Moonlight a very brittle experience
//if for instance, you need to open up files outside of one of these two contexts. Therefore Folder
//tries to ameliorate this by keeping track of search rules for all types of files inside the registry
//
//SOMPASTE: provides future compatibility. If 0 modeless is converted to a true of false value 
//If modeless, the dialog is hidden, and the output is written (SetProp) to the "Sompaste_select" property
//In modeless mode you will have to "subclass" the window to GetProp "Sompaste_select" prior to destruction
//modeless may be a callback in the future, however accessing it that way will require SOMPASTE to be nonzero
//
//owner: the "owner" of any dialog that results (if title is 0 then no dialogs will be created)
//inout: if filter is 0 inout provides a home folder, otherwise inout is understood to be a file
//In modal and windowless mode, if the (HWND) return is nonzero then inout returns the new file path
//Note: when filter is 0 there is no reason to modify inout and the HWND returned only indicates the 
//arguments provided were valid and or in modeless mode provides a handle to the new modeless window
//
//Important: when inout is a file (filter is nonzero) a file matching inout need not exist--inout should
//be the combination of a home folder path, and a title, separated by a slash. So if a MDO file was opened
//inside of a folder, then to find one of its textures, you take one of the texture reference strings inside
//the file and combine it with the home folder (same folder the MDL came from) and then use that for the input
//
//filter: supports either * for any type, or a series of ; separated file extensions that are deemed compatible
//title: if 0 no dialog is created: ie. "windowless" mode; the first match overwrites inout (same as modal mode)
//If title is "" (an empty string) a title is taken from inout/filter (something like something something Folder)
//If there is no default match and owner is nonzero a modal dialog may offer just in time assistance to the user 
//
//modeless: if nonzero the dialog is modeless (windowless dialogs are always effectively modal) 
//If SOMPASTE is nonzero its version number must indicate the correct interpretation of modeless 
//
//The return is nonzero upon success. Except in a modeless scenario, the handle will most likely be invalid
//NOTE: if inout is unchanged (and not modeless) the return is 0 regardles of whether such a file exists or not*/
HWND SOMPASTE_LIB(Folder)(SOMPASTE p, HWND owner, wchar_t inout[MAX_PATH], const wchar_t *filter _(L"*"), const wchar_t *title _(L""), void *modeless _(0))
SOMPASTE_API_FINALE(HWND,Folder,(SOMPASTE,HWND,wchar_t[],const wchar_t*,const wchar_t*,void*),(p,owner,inout,filter,title,modeless))

SOMPASTE_API
/*Query a database of .prf and .prt files (use Folder to then follow these files' references)
//
//The database is built on demand. Once per SOMPASTE object. Using the DATA environment variable
//along with USER/data CD/data and DATASET. If an address is unable to be resolved then the Places
//dialog (see Place API below) will popup a dialog prompting the end user for assistance in locating
//the misplaced folder. This unavoidable at this time. Only owner is be passed to Place when necessary
//
//inout follows the traditional layout of Sword of Moonlight's data folders and files. The filenames
//are enumerated numerically with no gaps, as if they were virtual files. Extensions and DATA/ are not
//required but are acceptable. To get at overriden files append /1 for the 1st. /2 for the 2nd and so on
//
//A * can take the place of a number to retrieve a count. If title is 0 the count is returned cast to an int
//Alternatively. A number can be input as a single '0' based wchar_t. So 11+'0' is 11. It must be the first or
//last part or delimited on both ends by a / separator (file extensions cannot be mixed with this mode of input)
//
//filter: is a list of ' ' separated set names. Parentheses are reserved: sets should be comprised of letters only 
//Tip: to convert a file name into a file path, provide the name as a filter, and indicate '0' for the virtual index
//
//title: if 0 a dialog will not be created. Unless modeless the returned HWND is nonzero upon success. The HWND can be
//(int) cast to retrieve the first character of the file name part of the path held by inout (or if inout is * the count)*/
HWND SOMPASTE_LIB(Database)(SOMPASTE p, HWND owner, wchar_t inout[MAX_PATH], const wchar_t *filter _(0), const wchar_t *title _(L""), void *modeless _(0))
SOMPASTE_API_FINALE(HWND,Database,(SOMPASTE,HWND,wchar_t[],const wchar_t*,const wchar_t*,void*),(p,owner,inout,filter,title,modeless))

SOMPASTE_API
/*Convert a token into a "longname" obstensibly in order to support legacy .prf/.prt file names
//
//Note that the file name up to the . should be no longer than 10 wide characters. If it is then 
//in its place a single wide character greater than 255 is returned by Database (above) which must
//then be fed through Longname to convert it into a file name longer than 10 wide characters
//(Note also that while longnames are shunned, they may aid artists in the early stages)*/
const wchar_t *SOMPASTE_LIB(Longname)(wchar_t token)
SOMPASTE_API_FINALE(const wchar_t*,Longname,(wchar_t),(token))

SOMPASTE_API
/*2024
* Trying to extend database (EXPERIMENTAL)
* 
* HACK: This is a starting point to adding/inserting something new to/into the database.
*/
wchar_t SOMPASTE_LIB(Name)(const wchar_t *longname)
SOMPASTE_API_FINALE(wchar_t,Name,(const wchar_t*),(longname))

SOMPASTE_API
/*2024
* Trying to extend database (EXPERIMENTAL)
* 
* HACK: This is a starting point to adding/inserting something new to/into the database.
*/
bool SOMPASTE_LIB(Inject)(SOMPASTE p, HWND owner, wchar_t path[MAX_PATH], size_t kind_or_filesize)
SOMPASTE_API_FINALE(bool,Inject,(SOMPASTE,HWND,wchar_t[],size_t),(p,owner,path,kind_or_filesize))

SOMPASTE_API
/*Convert a "place" into a file system file / folder address
//
//in: is read up to the first new line / carriage return or semicolon
//If in begins with white space, including new lines, it is left trimmed 
//If in is in the project's registry that value is copied to out. Otherwise
//if in is an existing file / folder in is copied to out. Regardless if titled 
//a popup will prompt the user for a mapping to in. If not modeless the number of
//characters needed to advance in to its next input is returned in (int) casted form*/
HWND SOMPASTE_LIB(Place)(SOMPASTE p, HWND owner, wchar_t out[MAX_PATH], const wchar_t *in, const wchar_t *title _(L""), void *modeless _(0))
SOMPASTE_API_FINALE(HWND,Place,(SOMPASTE,HWND,wchar_t[],const wchar_t*,const wchar_t*,void*),(p,owner,out,in,title,modeless))

SOMPASTE_API
/*Because of C runtimes and general schizophrenia on Windows 
//there is a need for central management of environment variables
//
//WARNING: Environ follows the rules of SOM files. You cannot unset
//a variable once set, however 0 is returned if a variable is unset
//(2017: This is kind of just the simple design of the API. I kinda
//wish it coudl be unset, since SOM_EX uses a mailslot now to spawn
//new instances of itself with the environment made fresh for each)
//
//If p is not 0 a private environment table is maintained by p
//An internal buffer is returned for each combination of p and var
//If var is 0 a block of 00 terminated wchar_t key value pairs is returned
//If set is 0 only the value is retrieved. A variable can be emptied but not unset
//If p is 0 the process environment is modified or retrieved on Windows. Not the C runtime*/
const wchar_t *SOMPASTE_LIB(Environ)(SOMPASTE p, const wchar_t *var _(0), const wchar_t *set _(0))
SOMPASTE_API_FINALE(const wchar_t*,Environ,(SOMPASTE,const wchar_t*,const wchar_t*),(p,var,set))

SOMPASTE_API  /*nonzero on success
//Clipboard ops: Cut/Copy/Paste/Grab->Drag & Drop
//owner: reserved. 0 or else the procedure fails!
//data: the underlying type of data depends on op
//data_s: the size of the underlying type of data 
//(note the size is in bytes, eg. not characters)
//HWND: treat as a 0/1 value until further notice
//On drop returns 1/WindowFromPoint(GetCursorPos)
//op: has 4 parts in all that look like "1 2:3.4"
//1) the operation, which can be X, C, or V (or Cut, Copy, Paste) 
//The first part is optional and separated from the second by a space.
//If not present the default behavior is to faciliate drag & drop, or grab.
//2) the data object, which can be "text" or "list" and followed by a colon (:)
//3) the application, which can be "file" or "text" and is followed by a period (.)
//4) the implementor, which can be "Windows". 
//
//Examples:
//"C text:file.Windows" copies a file to the clipboard.
//"C list:file.Windows" copies a list of files to the clipboard.
//"text.Windows" drags and drops some text. Note text:text is unnecessary.
//"clip.Windows" will refer to HBITMAP (see Clip below)
//
//Conversion:
//Unless the . before part 4 is an ! Xfer converts every way it knows how!
//
//Usage:
//The implementor part is used to register custom clipboard formats on Windows.
//For .Windows :text is UTF16LE type and text: is the character length of data_s.
//A list: begins with one or more separation characters including a terminating \0.*/
HWND SOMPASTE_LIB(Xfer)(const char *op, const HWND *wins, int count, void *data, size_t data_s)
SOMPASTE_API_FINALE(HWND,Xfer,(const char*,const HWND*,int, void*,size_t),(op,wins,count,data,data_s))

SOMPASTE_API
/*A top-level window that displays the contents of the clipboard
//It will be created only once (unless destroyed) with subsequent
//calls returning the same window
//
//owner: if 0 the clipboard will not be created
//if nonzero ownership of the clipboard is assigned to owner
//note: passed to SOMPASTE after each clipboard copy event
//
//See Clip (below) for additional comments pertaining to the clipboard*/
HWND SOMPASTE_LIB(Clipboard)(SOMPASTE p, HWND owner _(0), void *note _(0))
SOMPASTE_API_FINALE(HWND,Clipboard,(SOMPASTE,HWND,void*),(p,owner,note))

SOMPASTE_API
/*Get the handle of a bitmap stored in the clipboard's history
//WARNING: The handle is shared, and thus should not be deleted 
//
//time: 0 is equivalent to Sompaste_present (above) ... positive
//numbers are in the future (if the user has navigated backwards 
//thru the history) and negative numbers navigate back thru time
//Note: usually you just want to hold onto a bitmap's handle and
//assume that it is valid (or make yourself a copy to hold onto)
//Note: the Clipboard window allows a user to alter the contents 
//of its history (so making a copy may/may not be what you want)
//
//To get the number of bitmaps kept use GetProp on the Clipboard
//window (see Clipboard API above) with the following properties
//
//Sompaste_bitmaps: the number of bitmaps kept, 0 or greater
//Sompaste_present: the present time in the history, 0~bitmaps-1
//Sompaste_hotkeys: contains the clipboards HACCELL handle*/								 
HBITMAP SOMPASTE_LIB(Clip)(int time _(0))
SOMPASTE_API_FINALE(HBITMAP,Clip,(int),(time))

/*ChooseColor callback: for use with Choose*/

/*a: the handle to the modeless ChooseColor dialog
//b: the user provided window as was passed to Choose
//c: the currently chosen color. *c will be -1 before
//the dialog is created. cccb can take this opportunity
//to initialize *c. Choose returns this initial value 
//d: is set to 0 for the final call to cccb. If 0 then
//*c should be taken to be the final decided upon color
//Note: by default d is 1. Other values are reserved but
//can all be safely treated as 1 as long as d is nonzero
//e: the args pointer passed to Choose. In case you need
//to store some private data of any kind in there
//
//cccb is called at regular intervals whenever the value
//of *c changes. cccb should return the time in milliseconds
//it would like the dialog to wait before calling back again
//If 0 is returned cccb will not be called again. If less
//than 0 the resulting behavior has not been defined
//
//FYI: void* is used instead of COLORREF so that Choose 
//may be able to field any kind of type it is needed to*/
typedef int (*SOMPASTE_LIB(cccb))(HWND a,HWND b,void**c,int d,wchar_t*e);

//TODO: REALLY THE CALLBACK SHOULD BE SENT ON THE SAME THREAD
//AS THE CALLER. 
//Somplayer is having issues because SetWindowSubclass refuses
//to cross threads. Window procedures should stay on the thread.
SOMPASTE_API
/*Create a ChooseColor dialog: there can be only one at a time...
//If a dialog is already open it is closed as if the OK button was chosen
//If modeless is 0 the chosen COLORREF is returned. It's advised to use modeless
//args: the title of the dialog window will be Choose followed by a space and then
//the non-optional part of args. No commandline-like options are defined at this time
//window is taken to be the owner of the dialog, it is also passed to cccb in the 
//second HWND argument. See the cccb typed comments above for the use of cccb
//
//Note: If modeless the dialog is initiatially hidden. It will be shown and 
//activated after the second call to cccb returns if still invisible by then*/
void *SOMPASTE_LIB(Choose)(wchar_t *args _(L"Color"), HWND window _(0), SOMPASTE_LIB(cccb) modeless _(0))
SOMPASTE_API_FINALE(void*,Choose,(wchar_t*,HWND,SOMPASTE_LIB(cccb)),(args,window,modeless))

SOMPASTE_API /*true on success 
//Center HWND in the desktop most overlapped by ref
//If ref is 0 Sompaste.dll emulates DS_CENTERMOUSE as best it can
//
//NEW: ensures that keyboard focus is on the same monitor as mouse
//Otherwise the window is centered in the keyboard focus's window
//Probably use of Center should be adopted over DS_CENTERMOUSE*/
bool SOMPASTE_LIB(Center)(HWND win, HWND ref _(0), bool activate _(true))
SOMPASTE_API_FINALE(bool,Center,(HWND,HWND,bool),(win,ref,activate))

SOMPASTE_API /*true on success 
//Because CascadeWindows is worthless*/
bool SOMPASTE_LIB(Cascade)(const HWND *wins, int count)
SOMPASTE_API_FINALE(bool,Cascade,(const HWND*,int),(wins,count))

SOMPASTE_API /*true on success 
//
//TODO: DM_REPOSITION may be just the thing for this
//
//UNIMPLEMENTED: we need an API like this but the spec could be better
//
//Move windows into full view per the first window's desktop
//If count is greater than 1 the first window is an anchor and not moved
//flash: FlashWindowEx the first window if any of the windows must be moved
//Z-order is unmodified (though if flashed the first window will end up on top)
//Returns true if any window was moved (so false does not necessarily mean failure)*/
bool SOMPASTE_LIB(Desktop)(const HWND *wins, int count, bool flash _(false))
SOMPASTE_API_FINALE(bool,Desktop,(const HWND*,int,bool),(wins,count,flash))

SOMPASTE_API /*true on success
//Establish which windows are "stuck" to which other windows. The algorithm
//is subject to change/improvement, however conceptually each window is like 
//a sticker, and if it overlaps with another window then it is then said to be
//stuck to; like a post-it note. The first nonzero HWND is taken to be the app's
//main window; which may or may not be important to the alorithm that is in use...
//
//In the future it may be possible for users to customize the Sticky behavior with-
//out alerting the client; and it may be possible for the client to affect different 
//behavior by assigning properties to windows with the win32 SetProp APIs.
//
//sticky: if 0 all windows will be non-sticky (only memory will be filled)
//If nonzero an index per HWND is returned indicating which is stuck to which with
//-1 indicating that the window is not stuck to any window (it is free to move about)
//
//memory: if nonzero the (GetWindowRect) rectangles of HWND will be stored in memory
//
//zorder: if true the windows will be arranged (with SetWindowPos) Z-wise in order per sticky*/
bool SOMPASTE_LIB(Sticky)(const HWND *wins, int count, int *sticky _(0), RECT *memory _(0), bool zorder _(true))
SOMPASTE_API_FINALE(bool,Sticky,(const HWND*,int,int*,RECT*,bool),(wins,count,sticky,memory,zorder))

SOMPASTE_API /*true on success
//Arrange windows about the first window without going outside of desktop
//Where desktop can also mean the owner. All windows are assumed to be peers
//
//sticky: inform Arrange of the results of Sticky (above) so that it can take
//that into consideration while arranging the windows. If all windows are free
//
//Warning! Remember that Z-order for Windows is reversed/dumb/not documented!
//
//memory: the original arrangement of the HWND, for instance if the user is dragging
//the first window about then memory should remain constant throughout. Try for yourself
//Like sticky, memory can be taken from the Sticky API above. If 0 memory is not considered*/
bool SOMPASTE_LIB(Arrange)(const HWND *wins, int count, const int *sticky _(0), const RECT *memory _(0))
SOMPASTE_API_FINALE(bool,Arrange,(const HWND*,int,const int*,const RECT*),(wins,count,sticky,memory))

SOMPASTE_API
/*Allocate memory that Sompaste.dll can successfully delete
//This memory should be used when providing memory for Sompaste.dll
//to keep track of; such as via the "add" argument of the Undo API below 
//It is probably a safe bet to assume this is equivalent to using the default
//C++ new operator to allocate some memory. You can use that yourself; but to be
//absolutely safe use the New API and placement new syntax if you need to construct
//an object. FYI: Placement new looks like... new (this) CMyClass(arg1,arg2);*/
void *SOMPASTE_LIB(New)(size_t sizeinbytes)
SOMPASTE_API_FINALE(void*,New,(size_t),(sizeinbytes))

SOMPASTE_API
/*A SOMPASTE should depart before it becomes no longer available
//To field any outstanding undo/redo history. The history will be lost
//upon departure along with any dependencies (the module may be unaware of)*/
void SOMPASTE_LIB(Depart)(SOMPASTE p)
SOMPASTE_API_FINALE(void,Depart,(SOMPASTE),(p))

SOMPASTE_API /*nonzero on success
//If add is nonzero, add an undo to the history
//If add is 0, then undo the last undo "of" the history
//hist: if add then hist is a window that will own the undo
//If hist is 0 (and add) then it is taken to be the Window HWND
//----------------
//If add is 0 then hist is the focus. If hist is 0 then hist is 
//taken from the GetFocus win32 API. The first window with a history
//found above hist will then have its most recent undo "undone"
//
//add: add is a pointer on the heap that Sompaste.dll should be able
//to delete need be (if/when the history grows too large for instance) 
//The value of add will be passed back to SOMPASTE if the user undoes
//the undo by way of SOMPASTE->undo which should return true in order
//to indicate a successful undo. If it returns false then some happy
//user will be interrupted and forced to to make some dire decisions
//
//Note: needless to say, if add is 0 then SOMPASTE should be nonzero
//and SOMPASTE->undo (ideally redo too) should be in working order
//
//Note: there is no mechanism to avoid double undos if more than
//one agent is triggering undos. If SOMPASTE is 0 it is taken to 
//be the client app. Sompaste.dll will ignore a nonzero SOMPASTE
//on undo if the client window sets Sompaste_apponly (Window API)
//Programmers should work together to avoid any such contentions*/
bool SOMPASTE_LIB(Undo)(SOMPASTE p, HWND hist _(0), void *add _(0))
SOMPASTE_API_FINALE(bool,Undo,(SOMPASTE,HWND,void*),(p,hist,add))

SOMPASTE_API /*nonzero on success
//Same as Undo only in reverse and you (of course) cannot add a redo*/
bool SOMPASTE_LIB(Redo)(SOMPASTE p, HWND hist _(0))
SOMPASTE_API_FINALE(bool,Redo,(SOMPASTE,HWND),(p,hist))

SOMPASTE_API
/*Delete all undo/redo for hist
//If hist is 0 all undo/redo histories are deleted*/
void SOMPASTE_LIB(Reset)(HWND hist _(0))
SOMPASTE_API_FINALE(void,Reset,(HWND),(hist))

/*FYI: these return void* instead of bool*/

/*un/redo callbacks: return 0 to delete/skip*/
typedef void* (*SOMPASTE_LIB(undo))(SOMPASTE,HWND,void*);

/*free callback: will delete gc if returned*/
typedef void* (*SOMPASTE_LIB(free))(SOMPASTE,void*gc);

/*notify callback: return 0 to cancel note*/
typedef void* (*SOMPASTE_LIB(note))(SOMPASTE,void*);

struct SOMPASTE_LIB(module)
{		
	int hint; /*for internal use*/
		
	wchar_t version[4]; /*= {SOMPASTE_VERSION};*/

	/*some dogtags: 0 terminated user provided strings*/
	const wchar_t *name, *note, *reserved1, *reserved2;
	
	SOMPASTE_LIB(undo) un, re; /*un/redo callbacks*/
	SOMPASTE_LIB(free) gc;    /*garbage callback*/
	SOMPASTE_LIB(note) cb;   /*notify callback*/

#ifdef __cplusplus

	typedef SOMPASTE_LIB(undo) f;
	typedef SOMPASTE_LIB(free) g;

	SOMPASTE_LIB(module)(f u=0, f r=0)
	{
		wchar_t v[4] = {SOMPASTE_VERSION};
		for(int i=0;i<4;i++) version[i] = v[i];

		name = note = reserved1 = reserved2 = 0;

		un = u; re = r; gc = 0; cb = 0;
	}
	#ifndef SOMPASTE_NO_STAY
	~SOMPASTE_LIB(module)(){ depart(); }
	#endif

	static HWND window()
	{
		return SOMPASTE_LIB(Window)(); 
	}
	inline const wchar_t *get(const wchar_t *var)
	{
		return get(var,L""); //defaults to empty string
	}
	inline const wchar_t *get(const wchar_t *var, const wchar_t *def)
	{
		const wchar_t *out = SOMPASTE_LIB(Environ)(this,var); 		
		//2017: This had default only if out is undefined. But given
		//that unsetting is impossible and empty strings are probably
		//unwanted or can be fed to def, should empty string default?
		return out&&out[0]!='\0'?out:def;
	}
	inline const wchar_t *set(const wchar_t *var, const wchar_t *val)
	{
		//2017: I think it's better to check for nullptr. The API is
		//such that it doesn't have a way to unset itself. It might
		//have been useful to be able to, now that there can be only
		//one SOM_EX, and so it must reset its environment. It's not
		//meant to be a "try to set" kind of deal.
		return SOMPASTE_LIB(Environ)(this,var,val?val:L"");
	}
	static RECT pane(HWND window, HWND client=0)
	{
		return SOMPASTE_LIB(Pane)(window,client,false); 
	}
	static RECT frame(HWND window, HWND client=0)
	{
		return SOMPASTE_LIB(Pane)(window,client,true); 
	}
	static wchar_t *path(wchar_t inout[MAX_PATH])
	{
		return SOMPASTE_LIB(Path)(inout); 
	}
	static HWND create(HWND owner, int ID=0)
	{
		return SOMPASTE_LIB(Create)(owner,ID); 
	}
	static HWND *propsheet(HWND owner, int tabs, const wchar_t *title=L"Properties", HMENU menu=0)
	{
		return SOMPASTE_LIB(Propsheet)(owner,tabs,title,menu); 
	}
	static HWND wallpaper(HWND window=0, MSG *msg=0)
	{
		return SOMPASTE_LIB(Wallpaper)(window,msg); 
	}
	inline HWND folder(HWND owner, wchar_t inout[MAX_PATH], const wchar_t *filter=L"*", const wchar_t *title=L"", void *modeless=0)
	{
		return SOMPASTE_LIB(Folder)(this,owner,inout,filter,title,modeless); 
	}
	inline HWND database(HWND owner, wchar_t inout[MAX_PATH], const wchar_t *filter=0, const wchar_t *title=L"", void *modeless=0)
	{
		return SOMPASTE_LIB(Database)(this,owner,inout,filter,title,modeless); 
	}
	inline int database(HWND owner, wchar_t inout[MAX_PATH], const wchar_t *filter, int)
	{
		return (int)SOMPASTE_LIB(Database)(this,owner,inout,filter,0,0); 
	}
	inline bool database_insert(HWND owner, wchar_t path[MAX_PATH], size_t kind_or_filesize)
	{
		return SOMPASTE_LIB(Inject)(this,owner,path,kind_or_filesize); 
	}
	static const wchar_t *longname(wchar_t token)
	{
		return SOMPASTE_LIB(Longname)(token);
	}
	static const wchar_t *longname(const wchar_t *token)
	{
		return token[1]?token:SOMPASTE_LIB(Longname)(token[0]);
	}
	static wchar_t longname_token(const wchar_t *longname) //2024
	{
		return SOMPASTE_LIB(Name)(longname); //EXPERIMENTAL
	}
	inline HWND place(HWND owner, wchar_t out[MAX_PATH], const wchar_t *in, const wchar_t *title=L"", void *modeless=0)
	{
		return SOMPASTE_LIB(Place)(this,owner,out,in,title,modeless); 
	}
	inline int place(HWND owner, wchar_t out[MAX_PATH], const wchar_t *in, int _)
	{
		return (int)SOMPASTE_LIB(Place)(this,owner,out,in,_=='?'?L"?":0); 
	}
	static HWND xfer(const char *op, void *data, size_t data_s)
	{
		return SOMPASTE_LIB(Xfer)(op,0,0,data,data_s);
	}
	static HWND xfer(const char *op, HWND paste)
	{
		HWND src_and_dest[2] = {0,paste};

		return SOMPASTE_LIB(Xfer)(op,src_and_dest,2,0,0);
	}
	inline HWND clipboard(HWND owner=0, void *note=0)
	{
		return SOMPASTE_LIB(Clipboard)(this,owner,note); 
	}
	static HBITMAP clip(int time=0)
	{
		return SOMPASTE_LIB(Clip)(time); 
	}
	template<typename T> 
	static T choose(const wchar_t *color, HWND win, int (*cb)(HWND,HWND,T*,int,wchar_t*))
	{
		return (T)SOMPASTE_LIB(Choose)((wchar_t*)color,win,(SOMPASTE_LIB(cccb))cb); 
	}
	static bool center(HWND window, HWND reference=0, bool activate=true)
	{
		return SOMPASTE_LIB(Center)(window,reference,activate); 
	}
	template<int N>
	static bool cascade(const HWND (&wins)[N])
	{
		return SOMPASTE_LIB(Cascade)(wins,N); 
	}
	static bool cascade(const HWND *wins, int n)
	{
		return SOMPASTE_LIB(Cascade)(wins,n); 
	}
	template<int N>
	static bool desktop(const HWND (&wins)[N], bool flash=false)
	{
		return SOMPASTE_LIB(Desktop)(wins,N,flash); 
	}
	static bool desktop(const HWND *wins, int n, bool flash=false)
	{
		return SOMPASTE_LIB(Desktop)(wins,n,flash); 
	}
	static bool desktop(HWND win, bool flash=false)
	{
		return SOMPASTE_LIB(Desktop)(&win,1,flash); 
	}
	template<int N>
	static bool arrange(const HWND (&wins)[N], const int *sticky=0, const RECT *memory=0)
	{
		return SOMPASTE_LIB(Arrange)(wins,N,sticky,memory); 
	}
	static bool arrange(const HWND *wins, int n, const int *sticky=0, const RECT *memory=0)
	{
		return SOMPASTE_LIB(Arrange)(wins,n,sticky,memory); 
	}
	template<int N>
	static bool sticky(const HWND (&wins)[N], int *sticky=0, RECT *memory=0, bool zorder=true)
	{
		return SOMPASTE_LIB(Sticky)(wins,N,sticky,memory,zorder); 
	}
	static bool sticky(const HWND *wins, int n, int *sticky=0, RECT *memory=0, bool zorder=true)
	{
		return SOMPASTE_LIB(Sticky)(wins,n,sticky,memory,zorder); 
	}
	inline void depart()
	{
		return SOMPASTE_LIB(Depart)(this); 
	}
	inline bool undo(HWND hist=0, void *add=0)
	{
		return SOMPASTE_LIB(Undo)(this,hist,add); 
	}	
	inline bool redo(HWND hist=0)
	{
		return SOMPASTE_LIB(Undo)(this,hist); 
	}
	static void reset(HWND hist=0)
	{
		return SOMPASTE_LIB(Reset)(hist); 
	}

#endif
};

#ifdef SOMPASTE_API_ONLY
#pragma pop_macro("SOMPASTE")
#endif

#pragma pop_macro("_")

#endif /*SOMPASTE_INCLUDED*/

/*RC1004*/
