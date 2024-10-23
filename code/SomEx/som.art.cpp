
#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

#include <set> //sfx_mdl2
#include <hash_set> //Windows XP

namespace DDRAW
{
	extern unsigned noFlips;
}

#include "Ex.ini.h"
#include "Ex.cursor.h"
#include "Ex.window.h"

#include "som.state.h"
#include "som.files.h"
#include "som.tool.hpp"
#include "../lib/swordofmoonlight.h"
#include "../x2mdl/x2mdl.h"
#if SOMEX_VNUMBER!=0x100010aUL
#error need to copy Somimp/x2mdl.h into x2mdl/x2mdl.h
#endif

extern HWND som_tool_stack[16+1];
extern wchar_t som_tool_text[MAX_PATH];
extern int som_prm_tab;

extern IShellLinkW *som_art_link = 0;
static IPersistFile *som_art_link2 = 0;

//HACK: these are used to load files from
//nonstandard locations
extern wchar_t *som_art_CreateFile_w = 0;
extern wchar_t *som_art_CreateFile_w_cat(wchar_t w[MAX_PATH], const char *a)
{
	wcscpy(w,som_art_CreateFile_w);

	//this fails for map/texture as w and map/model as a
	//I don't think it's required anymore since textures
	//are handled as their own art but it still needs to
	//change the extension at least
	auto *p = PathFindExtensionW(w);
	auto *q = PathFindExtensionA(a);
		/*
	//REMOVE ME? 
	//2022: I think most of the code below can be removed
	//since TXR art has its own timestamps now
	#ifdef NDEBUG
//	#error TOO COMPLICATED
	#endif

	#define SLASH(x) (x=='/'||x=='\\')

//	wcscpy(w,som_art_CreateFile_w);

	wchar_t *fn = PathFindFileNameW(w);
	wchar_t *p = fn;
	p-=2;
	while(p>w&&!SLASH(*p))
	p--;

	int cmp = fn-p-1;

	char *q = PathFindFileNameA(a);
	q--;
	while(q>a)
	{
		q--;
		while(q>a&&!SLASH(*q))
		q--;

		int i = cmp;

		//NOTE: tolower was added for map/texture/bmp because SOM_MAP
		//converts them to upper case (I suppose it's required if to
		//be technically correct, but I'm removing this from SOM_MAP)
		while(i&&(q[i]==p[i]
		||SLASH(q[i])&&SLASH(p[i])
		||tolower(q[i])==tolower(p[i]))) 
		i--;
		if(!i) break;
	}

	#undef SLASH

//	while(*p++=*q++); return w;
		*/
	while(*p++=*q++); return w;
}

extern wchar_t *som_art_path()
{
	static wchar_t *path = 0; if(!path) 
	{
		path = new wchar_t[MAX_PATH+16]; //mipmap?

		//2024: it looks like Windows deletes random files in %TEMP%, trying %INSTALL%/art
	//	if(!ExpandEnvironmentStringsW(L"%TEMP%\\Swordofmoonlight.net\\art",path,MAX_PATH))
	//	path[0] = '\0'; //PARANOID
		Sompaste->path(wcscpy(path,Sompaste->get(L"ART")));	

		//TODO: this will need to be a suffix that
		//begins with a period depending on mipmap
		//extensions
		path[MAX_PATH] = '\0'; //do_mipmap?

		//COM can be absurd... not sure about the
		//overhead in frequently recreating these
		//interfaces
		if(SOM::tool==MapComp.exe)
		CoInitializeEx(0,COINIT_APARTMENTTHREADED); //multithreaded?
		IShellLinkW *i = 0; IPersistFile *ii = 0; 
		CoCreateInstance(CLSID_ShellLink,0,CLSCTX_INPROC_SERVER,IID_IShellLinkW,(LPVOID*)&i);
		if(i&&!i->QueryInterface(IID_IPersistFile,(void**)&ii))
		{
			som_art_link = i; som_art_link2 = ii; 
		}
		assert(i&&ii);
	}
	return path;
}

extern const wchar_t *x2mdl_dll = L"x2mdl.dll";
typedef std::vector<const wchar_t*> som_argv_t;
extern int som_art(const wchar_t *path, HWND hwnd)
{
	int exit_code = 0;
	//HACK: map_442440_407c60 sets this to x2msm.dll
	//auto x2mdl_dll = L"x2mdl.dll";
	//if(void*p=GetProcAddress(LoadLibraryW(x2mdl_dll),"x2mdl"))
	{
		//HACK: it's just convenient this way
		const wchar_t *art_path = som_art_path();
		const wchar_t *do_mipmap = art_path+MAX_PATH;

		bool convert_job = path==art_path; //YUCK 

		enum{ reserved=6 };
		som_argv_t argv(8);
		argv.resize(reserved);
		argv[0] = x2mdl_dll;
		argv[1] = art_path;
		argv[2] = do_mipmap;
		//REMINDER: the last spot (reserved-1) should 
		//be a 0-separator
		const wchar_t *data;
		for(int i=0;*(data=EX::data(i));i++)
		{
			if(!convert_job) //OPTIMIZING
			{
				size_t len = wcslen(data);
				if(wcsncmp(data,path,len)||path[len]!='\\')
				continue;
			}
			argv.push_back(data);
		}
		HWND hwcmp = hwnd;
		int argcmp = argv.size(); assert(argcmp>reserved);
		//tools have to load models too
		//switch(SOM::tool)
		switch(convert_job?SOM::tool:0)
		{
		case 0: argv.push_back(path); break;

		default: exit_code = -1; assert(!exit_code); break;

			extern HWND SOM_PRM_art(som_argv_t&);
			extern HWND SOM_MAP_art(som_argv_t&);

		case SOM_PRM.exe: hwnd = SOM_PRM_art(argv); break;
		case SOM_MAP.exe: hwnd = SOM_MAP_art(argv); break;
		}
		int argc = argv.size();
		if(!exit_code&&argc>argcmp)
		{
			bool game = //WIP
			SOM::game&&EX::is_visible(); //?

			/*I'm drawing a blank here :(
			if(SOM::game&&!game&&!DDRAW::noFlips)
			{
				//SOM::splashed(0); 				
				//EX::showing_window();
				//ShowWindow(EX::display(),1);
			}*/

			if(!convert_job) 
			{
				if(game) //copying som.hacks.cpp
				{
					EX::showing_cursor('wait');

					EX::following_cursor(); //hack
				}
				else SetCursor(LoadCursor(0,IDC_WAIT));
			}

			static int was = EX::tick();
			static int cmp = DDRAW::noFlips;
			static int more = 0;
			//assume rendering has occurred?
			if(cmp-DDRAW::noFlips>5)
			{
				was = EX::tick(); more = 0;
			}			

			//FIX ME?
			// 
			// REMINDER: I've gotten into some bad situtations when
			// the model fails to convert and so will keep retrying
			// 
		//	exit_code = ((int(*)(int,void*,HWND))p)(argc,&argv[0],hwnd);
			exit_code = x2mdl(argc,&argv[0],hwnd);
			//
			assert(0==exit_code); //2022

			if(convert_job)
			for(int i=argc;i-->argcmp;) delete[] argv[i];
				
			if(!convert_job)
			{
				if(game) //copying som.hacks.cpp
				{
					EX::hiding_cursor('wait');

					EX::following_cursor(); //hack
				}
				else SetCursor(LoadCursor(0,IDC_ARROW));

				auto now = EX::tick();

				static unsigned mute = 0;

				unsigned to = EX::INI::Output()->art_action_center_note_ms_timeout;

				if(now-was>to&&to!=~0u) //10000 //10 seconds?
				{
					/*this doesn't seem like a useful method (it quietly
					//adds an icon to the systray that pops after reading it)
					NOTIFYICONDATA nid = {sizeof(nid)};
					nid.hWnd = FindWindow(L"Swordofmoonlight.net Systray WNDCLASS",0);
					nid.uID = 2; //1
					//how to associate a Window???
				//	IIDFromString(L"{793FB9A5-D687-4470-8229-6D9EF3C8286E}",&nid.guidItem);
					nid.uTimeout = 1000;
					nid.dwInfoFlags = NIIF_INFO | NIIF_NOSOUND;
					nid.uFlags = NIF_TIP | NIF_ICON | NIF_MESSAGE | NIF_INFO | NIF_SHOWTIP | NIF_REALTIME; //NIF_GUID
				//	nid.uCallbackMessage = WM_USER + 200;
					nid.hIcon = LoadIcon(NULL, IDI_INFORMATION);
					nid.hBalloonIcon = nid.hIcon;
					lstrcpy(nid.szTip, L"Finished converting Sword of Moonlight model (it took a while!)");
					lstrcpy(nid.szInfoTitle,L"x2mdl.dll");
					BOOL test = Shell_NotifyIconW(NIM_ADD,&nid);
					test = test; //1 requires nid.uID = !0 ???
					nid.uVersion = NOTIFYICON_VERSION_4;	
					Shell_NotifyIconW(NIM_SETVERSION,&nid);
					*/
					extern int Ex_toast_wmain(int argc, LPWSTR *argv);
					WCHAR msg[128];
					auto *pp = argv[7];
					auto *m = wcsstr(pp,L"\\model\\");
					if(!m) m = PathFindFileName(pp); //paranoia
					if(m!=pp&&m[-1]=='\\') m--; //same
					while(m!=pp&&m[-1]!='\\') m--; //strip back to top level model directory?
					swprintf(msg,(more //...
					?L"Converted art %s and %d more\n(it's taking a while)"
					:L"Converted art %s\n(it took while)"),m,more);
					WCHAR *toast[] = {L"",
					L"--text",msg,
					//a title like this is quietly ignored (i.e. no toast popup is generated)
					//L"--appname", L"WinToast by Mohammed Boujemaoui <mohabouje@gmail.com>",
					L"--appname", L"Sword of Moonlight",
					//Note: Windows generates an app icon, e.g. the moonlight sword for example
					L"--appid", L"Ex runtime",
					L"--audio-state",(mute&&now-mute<90000?L"1":L"0"),
				//	L"--alarm-audio",L"1", //HACK: override Focus Assist (doesn't work)
					};
					Ex_toast_wmain(EX_ARRAYSIZEOF(toast),toast);

					was = now; more = 0; mute = now;
				}
				else more++;
			}
		}
		if(convert_job)
		{
			//HACK: destroy progress bar dialog?
			if(hwcmp!=hwnd&&hwnd)
			{
				for(int i=5;i-->0;EX::sleep(100)) //10
				SendMessage(hwnd,PBM_SETPOS,argc-argcmp,0);				
				DestroyWindow(GetParent(hwnd));
				EX::sleep(100); //2021: transition?
			}			
			//relying on language packs for text
			const char *a = 0; switch(SOM::tool)
			{
			case SOM_MAP.exe: a = "MAP1012"; break;
			case SOM_PRM.exe: a = "PRM1012"; break;
			}
			if(a) //chime/task completion message?
			MessageBoxA(0,a,0,MB_ICONINFORMATION);
			else assert(0);
		}
		else //2024: keep "app is not responding" away?
		{
			MSG keepalive;
			while(PeekMessage(&keepalive,0,0,0,PM_REMOVE))
			{
				TranslateMessage(&keepalive);
				DispatchMessage(&keepalive); 
			}
		}
	}
	//else exit_code = -2; 
	return exit_code;
}

extern void SOM_MAP_error_033(int);
extern void som_art_2021(HWND hwndDlg)
{
	wchar_t *path = som_art_path();
	extern LPITEMIDLIST STDAPICALLTYPE 
	som_tool_SHBrowseForFolderA(LPBROWSEINFOA);
	BROWSEINFOA a = {hwndDlg,0,0,0,0,0,1,0}; //GetSaveFileName?
	SHParseDisplayName(path,0,(PIDLIST_ABSOLUTE*)&a.pidlRoot,0,0);		
	PIDLIST_ABSOLUTE ok = som_tool_SHBrowseForFolderA(&a); if(ok)
	{				
		*path = '\0'; SHGetPathFromIDList(ok,path); if(*path)
		{
			if(int exit_code=som_art(path,hwndDlg))
			{
				//UNIFINISHED: theres a %d formatter in this
				//code's string that might be the error code
				switch(SOM::tool)
				{
				case SOM_MAP.exe:

					//MessageBoxA(0,"MAP033",0,MB_ICONERROR);					
					SOM_MAP_error_033(exit_code); break;

				case SOM_PRM.exe: 

					MessageBoxA(0,"PRM133",0,MB_ICONERROR); break;
				}
			}
		}
		ILFree(ok);
	}
	ILFree((PIDLIST_ABSOLUTE)a.pidlRoot);	
}

typedef struct //DEBUGGER
{
	char magic[4];
	char guid[16];
	DWORD idlist:1,filesys:1,desc:1,path:1,dir:1,cmdln:1,icon:1;
	//https://ithreats.files.wordpress.com/2009/05/lnk_the_windows_shortcut_file_format.pdf
	DWORD _file_attributes;
	DWORD _time123[6]; //QWORD _time1,_t2,_t3; //alignment?
	DWORD _file_length;
	DWORD icon_number; //implement me?
	DWORD _ShowWnd_value;
	DWORD _hotkey;
	DWORD _[2];
	///idlist data goes here if present //76B so far
	WORD idlist_sz;
	//first idlist item... //VARIABLE LENGTH
}som_art_lnk_start;
//static bool som_art_shortcut(WCHAR lnk[MAX_PATH], WCHAR ico[MAX_PATH])
static bool som_art_shortcut(WCHAR lnk[MAX_PATH]) //OPTIMIZING
{
		//DUPLICATE x2mdl_shortcut

	FILE *f = _wfopen(lnk,L"rb"); if(!f) return false;

	enum{ sizeof_buf=4096 }; //union
	union
	{
		char buf[sizeof_buf];
		wchar_t wbuf[sizeof_buf/2];
	};
	int sz = fread(buf,1,sizeof_buf,f); fclose(f);

	char *eof = buf+sz;

	//source (code is pretty iffy)
	//https://www.codeproject.com/Articles/24001/Workaround-for-IShellLink-GetPath
	//sites this source (its code doesn't get very far)
	//https://ithreats.files.wordpress.com/2009/05/lnk_the_windows_shortcut_file_format.pdf
	
	auto &l = *(som_art_lnk_start*)buf;
	//for some reason BMP->TXR links have path set to 0???
	//if(sz<sizeof(l)||!l.filesys) return false;
	if(sz<sizeof(l)) return false;

	int a = l.idlist?l.idlist_sz:-6;
	if(buf+a+78>eof) return false;

	if(l.idlist) //TESTING
	{
		//don't know why the codeproject source didn't just do this!
		SHGetPathFromIDListW((PCIDLIST_ABSOLUTE)(buf+78),lnk);		

		return true; //if(!ico) return true;
	}
	else assert(0); 
	
	return false; //if(!ico) return false;

	//Icon? //UNUSED
	//
	// this works but is untested with dir/cmdln
	// however the icons are being generated from
	// the MSM (art) file name with .ico extension
	// the shortcuts have icons set for in Explorer
	assert(0); WCHAR *ico; (void)ico;
	
	//this reproduces the codeproject site's code
	//what I got was a string to "C:\Users\" even
	//though my path was ANSI
	DWORD &b = (DWORD&)buf[78+a];
	/*File location info
	0078 74 00 00 00 Structure length (b)
	007C 1C 00 00 00 Offset past last item in structure
	0080 03 00 00 00 Flags (Local volume, Network volume)
	0084 1C 00 00 00 Offset of local volume table
	0088 34 00 00 00 Offset of local path string
	008C 40 00 00 00 Offset of network volume table
	0090 5F 00 00 00 Offset of final path string
		Local volume table
		0094 18 00 00 00 Length of local volume table
		0098 03 00 00 00 Fixed disk
		009C D0 07 33 3A Volume serial number 3A33-07D0
		00A0 10 00 00 00 Offset to volume label
		00A4 44 52 49 56 45 20 gDRIVE Ch,0
		43 00
		00AC 43 3A 5C 57 49 4E gC:\WINDOWS\h local path string
		44 4F 57 53 5C 00
		Network volume table
		00B8 1F 00 00 00 Length of network volume table
		00BC 02 00 00 00 ???
		00C0 14 00 00 00 Offset of share name
		00C4 00 00 00 00 ???
		00C8 00 00 02 00 ???
		00CC 5C 5C 4A 45 53 53 g\\JESSE\WDh,0 Share name
		45 5C 57 44 00
		00D7 44 65 73 6B 74 6F gDesktop\best_773.midh,0
		70 5C 62 65 73 74 Final path name
		5F 37 37 33 2E 6D
		69 64 00*/
	if(buf+b>eof-4) return false;

		//NOT 0X00 TERMINATED!

	/*Description string
	00EC 12 00 Length of string (c)
	00EE 42 65 73 74 20 37 gBest 773 midi fileh
	37 33 20 6D 69 64
	69 20 66 69 6C 65	
	Relative path
	0100 0E 00 Length of string (d)
	0102 2E 5C 62 65 73 74 g.\best_773.midh
	5F 37 37 33 2E 6D
	69 64
	Working directory
	0114 12 00 Length of string (e)
	0116 43 3A 5C 57 49 4E gC:\WINDOWS\Desktoph
	44 4F 57 53 5C 44
	65 73 6B 74 6F 70
	Command line arguments
	0128 06 00 (f)
	012A 2F 63 6C 6F 73 65 g/closeh
	Icon file
	0130 16 00 Length of string 
	0132 43 3A 5C 57 49 4E gC:\WINDOWS\Mplayer.exeh
	44 4F 57 53 5C 4D
	70 6C 61 79 65 72
	2E 65 78 65
	Ending stuff
	0148 00 00 00 00 Length 0 - no more stuff
	*/
	if(ico) *ico = '\0'; if(ico&&l.icon)
	{
		WCHAR *p = (WCHAR*)(buf+82+a+b);

		WCHAR *desc = 0, *path = 0, *dir = 0, *cmdln = 0, *icon = 0;

		if(l.desc){ desc = p; p+=p[-2]; }
	//	if(l.path){ path = p; p+=p[-2]; }
		if(l.path){ path = p; p+=p[-1]+1; }
		if(l.dir){ dir = p; p+=p[-2]; }
		if(l.cmdln){ cmdln = p; p+=p[-2]; }
		if(l.icon){ icon = p; p+=p[-1]; }
		
		if(icon)
		{
			#ifdef NDEBUG
		//	#error do on a loop (+are these ansi or wchar?)
			#endif
			if(icon+icon[-2]>(wchar_t*)eof) return false; //UNFINISHED

			assert(l.icon_number==0);

			//char *p = icon, *delim = icon+icon[-1];
			///int i; for(i=0;p<=delim;i++) wbuf[i] = *p++; wbuf[i] = '\0';
			//if(!GetLongPathNameW(wbuf,ico,MAX_PATH)) wcscpy(ico,wbuf);	
			if(icon[-1]<MAX_PATH)
			{
				wmemcpy(ico,icon,icon[-1]); ico[icon[-1]] = '\0';
			}
			else assert(0);
		}
	}
	return true;
}
static bool som_art_X2MDL_UPTODATE(FILETIME &t2)
{
	SYSTEMTIME st;
	FileTimeToSystemTime(&t2,&st);
	wchar_t dt[3] = {X2MDL_UPTODATE};
	wchar_t cmp[3] = {st.wYear,st.wMonth,st.wDay};
	return wmemcmp(dt,cmp,3)<=0;
}
extern bool som_art_nofollow = false; //DEBUGGING
//extern int som_art_model(WCHAR *cat, WCHAR w[MAX_PATH], WCHAR *ico)
extern int som_art_model(WCHAR *cat, WCHAR w[MAX_PATH])
{
	int data = cat[1]==':'?5:0; //SOMEX_?
	if(!wcsnicmp(cat+data,L"data\\",5)) cat+=data+5; //if(ico) *ico = 0;

	//REMINDER: I started out using the first . and
	//this had some problem that made me to use the
	//last . instead
	wchar_t *o = wcsrchr(cat,'.');
	if(!o||o[-1]=='\\'||o[-1]=='/')
	return 0;

	bool edit = o[1]=='*'; //2023

	wchar_t o1 = o[1]; o[1] = '*';
	wchar_t o2 = o[2]; o[2] = '\0';	
	WIN32_FIND_DATAW found;
	HANDLE glob = INVALID_HANDLE_VALUE; 
	int i,j,k; 
	if(cat[1]==':') //2024: known file?
	{
		i = wcslen(cat);
		wmemcpy(w,cat,i+1);
		glob = FindFirstFileW(w,&found);

		k = 0; //TODO: try to figure me out?
	}
	else for(k=0;*EX::data(k);k++)
	{
		i = swprintf(w,L"%s\\%s",EX::data(k),cat);
		//didn't notice any improvement
		//glob = FindFirstFileExW(w,FindExInfoBasic,&found,FindExSearchNameMatch,0,0);
		glob = FindFirstFileW(w,&found);
		if(glob!=INVALID_HANDLE_VALUE)
		break;
	}
	if(glob==INVALID_HANDLE_VALUE) return 0;

	j = i-2;
	while(i&&w[i]!='\\'&&w[i]!='/')
	i--;
	i++;
	j-=i; 
	o[1] = o1; o[2] = o2;	

	using namespace x2mdl_h; //_lnk, etc.
	int e = 0;

	bool art = false, exact = false; 
//	#ifdef _DEBUG
	bool nofollow = som_art_nofollow||SOM::retail; nofollow:
//	#else
//	bool nofollow = false; nofollow:
//	#endif

	FILETIME t1,t2,t3;		
	wchar_t lnk[MAX_PATH]; do
	if(~found.dwFileAttributes
	&FILE_ATTRIBUTE_DIRECTORY)
	{
		//model.mm3d.mdl will pick up a
		//model.mm3d file as an art file
		wchar_t *ext = found.cFileName+j;
		if('.'!=*ext) continue;

		//2024: x2mdl does this by default
		if(wcschr(ext+1,'.')) continue;

		char mc[4] = {ext[3],ext[2],ext[1],ext[0]};
		for(int i=4;i-->0;) mc[i] = tolower(mc[i]);

		switch(wcslen(ext)<=4?*(DWORD*)mc:0)
		{
		case '.lnk': //link?

			if(nofollow) continue; //???
		
			e|=_lnk;
			t2 = found.ftLastWriteTime;
			t3 = found.ftCreationTime; //icons
			wmemcpy(lnk,w,i);
			wmemcpy(lnk+i,found.cFileName,j+5);
			continue;

		case '.mdl': e|=_mdl; continue;
		case '.mdo': e|=_mdo; continue;

		case '.bp\0': e|=_bp; continue;
		case '.cp\0': e|=_cp; continue;
		case '.wt\0': e|=_wt; continue;

		//NOTE: this is so SFX will pick up textures
		case '.txr': e|=_txr; continue;

		case '.ico': e|=_ico; continue;
		case '.msm': e|=_msm; continue;
		case '.mhm': e|=_mhm; continue;
		}
		if(exact||nofollow)
		{
			if(!som_art_nofollow) continue;
		}

		#ifdef NDEBUG
//		#error disable BMP extensions overwritten by TXR //???
		#endif
		if(!wcsicmp(o,ext)) exact = true;

		if(art&&!exact) //collision policy?
		if(CompareFileTime(&t1,&found.ftLastWriteTime)>0)
		{
			//prefer newest unless name is an exact match
			continue;
		}
		
		art = true;
		t1 = found.ftLastWriteTime;
		wcscpy(w+i+j,found.cFileName+j);

		if(edit) return _art; 

	}while(FindNextFileW(glob,&found));

	FindClose(glob);

	if(!nofollow)
	{
		//HACK: be lenient around legacy file
		//just in case junk files are present
		if(e&_runtime_mask&&art)		
		if(e&_mdo&&!wcsicmp(o,L".mdo")
		 ||e&_mdl&&!wcsicmp(o,L".mdl")
		 ||e&_txr&&!wcsicmp(o,L".txr")
		 ||e&_msm&&!wcsicmp(o,L".msm")
		 ||e&_mhm&&!wcsicmp(o,L".mhm")
		 ||e&_ico&&!wcsicmp(o,L".ico"))
		{
			art = false;
		}
		if(!art)
		{
			wcscpy(w+i+j,o);
		}
		else if(e&_lnk&&!som_art_X2MDL_UPTODATE(t3)) //t2
		{
			e&=~_lnk; //2023: X2MDL_UPTODATE
		}
		else if(e&_lnk) //shortcut?
		{
			if(!som_art_link)
			if(!som_art_link2)
			{
				som_art_path(); //initialize?
			}
			else //HACK: workshop_file_system?
			{
				//2022: I've added som_art_path
				//to workshop.cpp initialization
				//to make sure PrtsEdit doesn't
				//go down the above branch the
				//first time it tries this hack

				goto short_circuit;
			}

			o = 0;
			IPersistFile *ll = som_art_link2;		
			if(!memcmp(&t1,&t2,sizeof(t1)))
				#if 0 //TRIPLES LOAD TIMES
			if(IShellLinkW*l=som_art_link)
			if(!ll->Load(lnk,STGM_READ))			
			if(!l->GetPath(lnk,MAX_PATH,0,0))
				#else
			if(som_art_shortcut(lnk)) //ico
				#endif
			if(PathFileExistsW(lnk)) 
			{
				o = wcsrchr(lnk,'.');
			}
			e&=~_lnk; if(o)
			{
				o1 = o[1]; o[1] = '*'; 
				o2 = o[2]; o[2] = '\0';
				glob = FindFirstFileW(lnk,&found);
				o[1] = o1; o[2] = o2;
				if(glob!=INVALID_HANDLE_VALUE)
				{
					i = o-lnk; wcscpy(w,lnk);
					j = i;
					while(i&&w[i]!='\\'&&w[i]!='/')
					i--;
					i++;
					j-=i;
					e = _lnk;
					nofollow = true; goto nofollow;
				}
				else assert(0); short_circuit:;
			}
		}
	}

	//SKETCHY
	//try to detect TXR based SFX models?
	//I think som_db tries MDL before TXR
	if(e&_txr)
	if(!art&&0==(e&(_lnk|_mdl|_mdo)))
	{
		o = w+i+j; //should be extension?

		if(!wcsicmp(o,L".mdl")) //SFX?
		{
			wmemcpy(o,L".txr",5);
		}
	}

	if(art||e&_runtime_mask)
	return e|art<<_art_shift|k<<_data_shift;
	return 0;
}
extern int som_art_model(const char *cat, wchar_t w[MAX_PATH])
{
	if(!SOM::game) //HACK: workshop.cpp?
	{
		char *a = PathFindFileNameA(cat);
		char *e = a;
		strtol(a,&e,10);
		if(e>a&&!*e)
		strcpy((char*)cat,SOMEX_(A)"\\data\\my\\model\\arm.mdl");
	}

	wchar_t buf[128]; //32+deepest data folder?
	swprintf_s(buf,L"%hs",cat); return som_art_model(buf,w);
}
struct som_art_32 //filter redundant models?
{
	char cmp[32];
	operator size_t()const
	{
		auto it = (size_t*)cmp, itt = (size_t*)(cmp+32);
		size_t o = 2166136261U;		
		while(it!=itt) o = 16777619U*o^*it++; return o;
	}
	typedef stdext::hash_set<som_art_32> set;
};
extern void som_art_model(som_argv_t &v, const char *d, const char m[32], som_art_32::set *f)
{	
	//this removes reused profiles/models 
	if(!*m) return; if(f)
	{
		//need to 0 pad and normalize hash		
		int i = 0; char m32[32];
		while((m32[i]=tolower(m[i]))&&i++<31);
		while(i<32) m32[i++] = 0;

		//NOTE: Sfx.dat models aren't using this
		if(!f->insert(*(som_art_32*)m32).second)
		return;
	}
		legacy: //map\model -> map\texture?

	wchar_t cat[64];
	int i = swprintf(cat,L"%hs\\%hs",d,m);
	if(i<=0) return;
	
		swing: //clunky swing model system?

	wchar_t copy[64+MAX_PATH], *w = copy+64;

	int e = som_art_model(cat,w); //,nullptr

	if(e&(x2mdl_h::_art)&&~e&x2mdl_h::_lnk) copy:
	{
		int j = wcslen(w);
		v.push_back(wmemcpy(new wchar_t[j+1],w,j+1));
	}
	else if(e&x2mdl_h::_runtime_mask) //copy syntax?
	{
		const wchar_t *a = v[1], *b = w; //som_art_path?
		for(;*a&&*b;a++,b++)
		if(*a!=*b&&tolower(*a)!=tolower(*b))
		break;
		if(*a) //source (before data) isn't destination?
		{
			swprintf(copy,L"\"%d|%s|%s\"",e,cat,w); 

			w = copy; goto copy;
		}
	}
	else if(!memcmp(d,"map\\m",5)) //legacy MPX texture?
	{
		//TODO? screen out "sky%02d.mdo"
		auto *ext = PathFindExtensionA(m);
		m = (char*)memcpy(copy,m,ext-m);
		memcpy((char*)m+(ext-m),".txr",5);
		d = "map\\texture"; goto legacy;
	}
	else return; if('i'==*d) if(!cat[i])
	{
		int j = i-1;
		while(j&&cat[j]!='.') 
		j--;
		if(cat[j]=='.')
		{
			wmemmove(cat+j+2,cat+j,i-j+1);
			cat[j] = '_';
			cat[j+1] = '0'; 
			i = j+1; goto swing;
		}
	}
	else if(++cat[i]<='2') goto swing;
}

struct SOM_MAP_art_files
{
	bool MAP035; //error

	bool *items, *sfx_mdl, *shops;

	std::set<std::string> sfx_mdl2;

	//NOTE: will be 0 filled on failure
	const SOM::DATA::Sfx::Dat *sfx_dat;

	swordofmoonlight_prm_object_t obj_prm[1024];
	swordofmoonlight_prm_enemy_t enemy_prm[1024];
	swordofmoonlight_prm_npc_t npc_prm[1024];

	//WARNING INHERITS ALIGNMENT
	struct prf_header
	{
		char name[31],model[31]; 
	};	
	template<class T>
	struct pr2 : prf_header,T{};

	uint32_t obj_size;
	pr2<swordofmoonlight_prf_object_t> obj_prf[1024];		
	pr2<swordofmoonlight_prf_enemy_t> **enemy_prf; //[1024]
	pr2<swordofmoonlight_prf_npc_t> **npc_prf; //[1024]
	SOM::sfx_pro_rec sfx_pro[1024];

	void init_file(char *b, const char *a, char* &p, size_t sz)
	{
		auto *w = SOM::Tool.project(strcpy(b+11,a)-11);
		FILE *f = _wfopen(w,L"rb");
		if(!f){ MAP035 = true; return; }
		if(!p)
		{
			fseek(f,0,SEEK_END); sz = max((long)sz,ftell(f));
			fseek(f,0,SEEK_SET); p = new char[sz];
		}
		size_t rd = fread(p,1,sz,f); if(rd<sz)
		{
			memset(p+rd,0x00,sz-rd); assert((void*)p==&obj_size);
		}
		fclose(f);
	}
	SOM_MAP_art_files():MAP035()
	{
		//memset(this,0x00,sizeof(*this));		
		char *p,b[32] = SOMEX_(B)"\\param\\";
		init_file(b,"obj.prm",p=(char*)obj_prm,sizeof(obj_prm));
		init_file(b,"enemy.prm",p=(char*)enemy_prm,sizeof(enemy_prm));
		init_file(b,"npc.prm",p=(char*)npc_prm,sizeof(npc_prm));
		init_file(b,"obj.pr2",p=(char*)&obj_size,4+sizeof(obj_prf));
		init_file(b,"enemy.pr2",(char*&)enemy_prf=0,1024*4);
		init_file(b,"npc.pr2",(char*&)npc_prf=0,1024*4);	
		init_file(b,"sfx.pro",p=(char*)sfx_pro,sizeof(sfx_pro));	
		for(int i=1024;i-->0;)
		if(enemy_prf[i]) (char*&)enemy_prf[i]+=(size_t)enemy_prf;
		for(int i=1024;i-->0;)
		if(npc_prf[i]) (char*&)npc_prf[i]+=(size_t)npc_prf;
		//YUCK: can't use init_file
		SOM::DATA::Sfx.dat->clear();
		if(!SOM::DATA::Sfx.dat->open()) MAP035 = true;
		sfx_dat = SOM::DATA::Sfx.dat;
	}
	~SOM_MAP_art_files()
	{
		delete[] enemy_prf; delete[] npc_prf;
		SOM::DATA::Sfx.dat->clear();
	}

	bool object(int &o)
	{
		o = obj_prm[o].profile;
		if(o>=1024) return false;
		DWORD fx = obj_prf[o].flameSFX;
		for(int i=0;i<2;i++)
		{
			if(i) fx = obj_prf[o].trapSFX;
			if(fx<1024) fx = sfx_dat->records[fx][1];
			if(fx<256)
			{
				if(*sfx_pro[fx].model)
				sfx_mdl2.insert(sfx_pro[fx].model);
				else sfx_mdl[fx] = true; //legacy? //remove me
			}
		}
		return true;
	}
	bool enemy(int &e)
	{
		DWORD i = enemy_prm[e].item;
		if(i<250) items[i] = true;
		e = enemy_prm[e].profile;
		if(e>=1024) return false;
		if(auto*p=enemy_prf[e])
		sfx((char*)p,p->flameSFX,p->dataSFX); return true;
	}
	bool npc(int &n)
	{
		DWORD i = npc_prm[n].item;
		if(i<250) items[i] = true;
		n = npc_prm[n].profile;
		if(n>=1024) return false;
		if(auto*p=npc_prf[n])
		sfx((char*)p,p->flameSFX,p->dataSFX); return true;
	}
	void sfx(char *prf, DWORD flame, WORD(*sfx)[2])
	{
		if(flame<1024) flame = sfx_dat->records[flame][1];

		if(flame<256) //sfx_mdl[flame] = true;
		{
			if(*sfx_pro[flame].model)
			sfx_mdl2.insert(sfx_pro[flame].model);
			else sfx_mdl[flame] = true; //legacy? //remove me
		}

		for(int k=0;k<32;k++) if(int j=sfx[k][0])
		for(auto*d=(swordofmoonlight_prf_data_t*)(prf+sfx[k][1]);j-->0;d++)
		{
			if(d->effect&&d->effect<1024)
			{
				DWORD fx = sfx_dat->records[d->effect][1];
				
				if(*sfx_pro[fx].model)
				sfx_mdl2.insert(sfx_pro[fx].model);
				else sfx_mdl[fx] = true; //legacy? //remove me
			}
		}
	}

	typedef SWORDOFMOONLIGHT::evt::event_t evt_event_t;
	typedef SWORDOFMOONLIGHT::evt::image_t evt_image_t;

	void event(evt_image_t &img, evt_event_t &e)
	{
		union
		{
			swordofmoonlight_evt_code_t *ea;
			swordofmoonlight_evt_code17_t *ea17;		
			swordofmoonlight_evt_code50_t *ea50;
		};
		for(int i=16;i-->0;)
		if(auto os=e.loops[i].offset) 
		{
			while(img>=os/4+1) //safety first?
			{
				(void*&)ea = img+os/4;

				if(0xFFFF!=ea->function)
				{
					os+=ea->nextcode;
				}
				else break;

				switch(ea->function)
				{
				case 0x17:

					if(img>=ea17+1)
					if(ea17->shop<128) 
					shops[ea17->shop] = true;
					break;
				
				case 0x50:

					if(img>=ea50+1)
					if(ea50->status==4) 
					if(ea50->status2<250)
					items[ea50->status2] = true;
					break;
				}
			}				
		}
	}
};

extern void SOM_MAP_165_maps(HWND);
extern char SOM_MAP_layer(WCHAR[]=0,DWORD=0);
extern int SOM_MAP_201_multisel(int(&maps)[64]);
extern HWND SOM_MAP_or_SOM_PRM_progress(HWND p, int mpx);
extern HWND SOM_MAP_art(som_argv_t &v)
{
	int dlg = som_tool_stack[2]?1:0;
	enum{ mapsN=64 }; int maps[mapsN];
	int mpx; if(mpx=!som_tool_stack[2])
	{
		int i = SOM_MAP_layer();
		if(-1==i){ assert(0); return 0; }

		memset(maps,0xff,sizeof(maps));
		maps[i] = i;
	}
	else mpx = SOM_MAP_201_multisel(maps);

	//TODO: select everything?
	if(!mpx){ MessageBeep(-1); return 0; }

	auto compile = 
	SendDlgItemMessage(som_tool_stack[1+dlg],2022,BM_GETCHECK,0,0);
	DestroyWindow(som_tool_stack[1+dlg]);
	HWND pb = SOM_MAP_or_SOM_PRM_progress(som_tool_stack[dlg],mpx);
	HWND task = GetParent(pb);

	if(compile) if(dlg) 
	{
		SOM_MAP_165_maps(pb);
	}
	else //good enough?
	{
		if(SOM_MAP_4921ac.modified_flags()) //save MAP?
		{
			//00418ED2 E8 59 34 05 00       call        0046C330  
			if(IDYES==(((DWORD(__stdcall*)(int,int,int))0x46c330)(0x48fba0,4,0)))
			((void(__thiscall*)(void*))0x413750)(&SOM_MAP_4921ac);			
		}
		PROCESS_INFORMATION pi;
		if(CreateProcessA(0,"mapcomp",0,0,1,0,0,0,0,&pi))
		{
			WaitForSingleObject(pi.hProcess,INFINITE);

			//I think MapComp generates a nonzero exit code
			DWORD ec; if(!GetExitCodeProcess(pi.hProcess,&ec))
			{
				assert(0); //UNTESTED
			}
			else if(ec) //EXPERIMENTAL
			{
				//MessageBoxA(0,"MAP033",0,MB_ICONERROR);
				extern void SOM_MAP_error_033(int);
				SOM_MAP_error_033(ec);
			}

			CloseHandle(pi.hThread); //redundant?
			CloseHandle(pi.hProcess);			 			
			//SendMessage(pb,PBM_STEPIT,0,0);

			for(int i=5;i-->0;EX::sleep(100))	
			SendMessage(pb,PBM_SETPOS,1,0);
		}
	}
	SendMessage(pb,PBM_SETPOS,0,0);

	HWND text = GetDlgItem(task,2021);	
	GetWindowTextW(task,som_tool_text,MAX_PATH);
	if(wchar_t*p=wcschr(som_tool_text,'('))
	wcscpy(p+1,L"x2mdl.dll)");
	else if(wchar_t*p=wcschr(som_tool_text,L'\x0xff088ff'))
	wcscpy(p+1,L"x2mdl.dll\xff09");
	SetWindowTextW(task,som_tool_text);

	if(text) //Indexing 3D maps. Please wait...
	{
		GetWindowTextW(text,som_tool_text,MAX_PATH);
		SetDlgItemTextW(task,1151,som_tool_text);
	}

	auto *fs = new SOM_MAP_art_files;

	if(fs->MAP035) MAP035: //failed to load file?
	{
		MessageBoxA(0,"MAP035",0,MB_ICONERROR);

		delete fs; return pb;
	}
	
	bool sfx_mdl[256] = {};
	bool objects[1024] = {}, items[250] = {};
	bool enemies[1024] = {}, npcs[1024] = {};
	bool shops[128] = {}, sky[5] = {};
	fs->items = items;
	fs->shops = shops;
	fs->sfx_mdl = sfx_mdl;

	//FIX ME
	//SOM::Tool.project monkeys with mpx/evt
	//files to route them to the current map
	wchar_t fix_me[MAX_PATH];

	//zentai?
	namespace evt = SWORDOFMOONLIGHT::evt;
	evt::image_t img;
	if(EX::data("map\\sys.ezt",fix_me))
	{
		evt::maptofile(img,fix_me);
		if(auto&head=evt::imageheader(img))
		{
			auto *pp = head.list;
			for(auto i=head.count;i-->0;)
			{
				//only acknowledge systemwide?
				if(pp[i].type!=0xFD) continue;

				fs->event(img,pp[i]);
			}
		}
		evt::unmap(img); //optional???
	}

	som_art_32::set f;

	for(int i=0;i<mapsN;i++) if(-1!=maps[i])
	{
		//note, this will fail if the file is an MPY
		//and may be false positive if there's an MPX
		//that should be an MPY

		char a[32];
		//YUCK: project is set up to modify the
		//map number if it has an MPX extension
		//sprintf(a,SOMEX_(B)"\\data\\map\\%02d.mpx",i);
		int ext = sprintf(a,"map\\%02d.mpx",i)-3;
		if(EX::data(a,fix_me))
		{			
			namespace mpx = SWORDOFMOONLIGHT::mpx;
			mpx::maptofile(img,fix_me);
			if(auto&base=mpx::base(img))
			{
				mpx::body_t m(base);

				for(int j,i=m.objects.count;i-->0;)
				if((j=m.objects.list[i].kind)<1024)
				{
					if(fs->object(j)) objects[j] = true;
				}
				for(int j,i=m.enemies.count;i-->0;)
				if((j=m.enemies.list[i].kind)<1024)
				{
					if(fs->enemy(j)) enemies[j] = true;
				}
				for(int j,i=m.npcs.count;i-->0;)
				if((j=m.npcs.list[i].kind)<1024)
				{
					if(fs->npc(j)) npcs[j] = true;
				}
				for(int j,i=m.items.count;i-->0;)
				if((j=m.items.list[i].kind)<250)
				{
					items[j] = true;
				}
				if(m.tiles.sky<sizeof(sky))
				{
					sky[m.tiles.sky] = true;
				}
				if(auto&t=mpx::data(img,m,2).textures)
				{
					auto *p = t.refs;
					for(int i=t.count;i-->0;p++)
					{
						som_art_model(v,"map\\model",p,&f);

						while(*p) p++;
					}
				}
			}
			if(mpx::unmap(img)) goto MAP035; //!!
		}
				
		//events?
		memcpy(a+ext,"evt",4);
		if(EX::data(a,fix_me))
		{
			evt::maptofile(img,fix_me);
			if(auto&head=evt::imageheader(img))
			{
				auto *pp = head.list;
				for(auto i=head.count;i-->0;)
				{
					//ignore systewide and deleted
					switch(pp[i].type)
					{
					case 0xFD: case 0xFF: continue;
					}

					fs->event(img,pp[i]);
				}
			}
			evt::unmap(img); //optional???
		}

		//shops?
		{
			auto &dat = *(SOM::Shops*)SOM_MAP_4921ac.shop_DAT_file;

			for(int i=128;i-->0;) if(shops[i])
			{
				auto &shop = dat[i]; for(int j=250;j-->0;)
				{
					if(shop.stock[j]) items[j] = true;
				}			
			}
		}

		//starting equipment?
		{
			auto *sup = (SOM::Setup*)(SOM_MAP_4921ac.SYS_DAT_file+1264);

			//both? don't know which one will be used to play 
			for(int set=2;set-->0;sup++)
			{
				for(int i=8;i-->0;)
				if(sup->equipment[i]<250)
				items[sup->equipment[i]] = true;

				for(int i=250;i-->0;)
				if(sup->items[i]) items[i] = true;
			}
		}

		SendMessage(pb,PBM_STEPIT,0,0);
	}

	delete fs;
	
	if(text) //Converting 3D art. Please wait...
	{
		if(text=GetWindow(text,GW_HWNDNEXT))
		if(GetWindowTextW(text,som_tool_text,MAX_PATH))
		SetDlgItemTextW(task,1151,som_tool_text);
	}
	SendMessage(pb,PBM_SETPOS,0,0);

	size_t cmp = v.size();

	f.clear();
	if(auto*ss=SOM_MAP_4921ac::npcskins)
	{		
		for(int i=0;i<1024;i++) if(npcs[i])
		{
			if(ss[i].skin)
			som_art_model(v,"npc\\texture",ss[i].file,&f);
		}
		f.clear();
		ss+=1024;
		for(int i=0;i<1024;i++) if(enemies[i])
		{
			if(ss[i].skin)
			som_art_model(v,"enemy\\texture",ss[i].file,&f);
		}
		f.clear();
	}
	auto *op = SOM_MAP_4921ac.objectprofs;
	auto *ep = SOM_MAP_4921ac.enemyprofs;
	auto *np = SOM_MAP_4921ac.NPCprofs;
	auto *ip = SOM_MAP_4921ac.itemprofs;
	for(int i=0;i<1024;i++) if(objects[i])	
	som_art_model(v,"obj\\model",op[i].model,&f); f.clear();
	for(int i=0;i<1024;i++) if(enemies[i])
	som_art_model(v,"enemy\\model",ep[i].model,&f); f.clear();	
	for(int i=0;i<1024;i++) if(npcs[i])
	som_art_model(v,"npc\\model",np[i].model,&f); f.clear();	
	for(int i=0;i<250;i++) if(items[i])
	{
		int j = SOM_MAP_4921ac.itemprops[i].profile;
		if(j<256)
		som_art_model(v,"item\\model",ip[j].model,&f); //f.clear();
	}
	for(int i=1;i<256;i++) if(sfx_mdl[i])
	{
		//NOTE: som_art_model switches to .txr when needed 
		char m[32]; sprintf(m,"%04d.mdl",i);
		som_art_model(v,"sfx\\model",m,0);
	}
	for(auto&ea:fs->sfx_mdl2)
	{
		//NOTE: som_art_model switches to .txr when needed 
		som_art_model(v,"sfx\\model",ea.c_str(),0);
	}
	for(int i=1;i<sizeof(sky);i++) if(sky[i])
	{
		char m[32]; sprintf(m,"sky%02d.mdo",i);
		som_art_model(v,"map\\model",m,0);
	}
	//I guess?
	som_art_model(v,"item\\model","gold.mdo",0);
	som_art_model(v,"my\\model","arm.mdl",0);
	som_art_model(v,"enemy\\model","kage.mdl",0);

	SendMessage(pb,PBM_SETRANGE,0,MAKELPARAM(0,v.size()-cmp));

	return pb; //x2mdl does PBM_STEPIT
}

extern HWND SOM_PRM_art(som_argv_t &v)
{
	size_t cmp = v.size();

	//REMINDER: instead of doing items (drops, etc.)
	//only SFX is done because there's no tab for it
	bool sfx_mdl[256] = {};
	SOM::DATA::Sfx.dat->clear();
	
	//TODO: SOM_PRM needs multi-select
	som_art_32::set f;
	char size[7] = {88,0,40,0,8,8,108};
	int sz = size[som_prm_tab-1001];
	int span = sz;
	if(sz==8) //size/pointer pair?
	{
		bool npc = som_prm_tab!=1005;
		char *dir = npc?"npc/model":"enemy/model";
		sz = som_prm_tab==1005?0x47A838:0x47d838;
		for(int i=0;i<1024;i++,sz+=8)
		{
			char *prf = *(char**)(sz+4); if(!prf) continue;
			
			som_art_model(v,dir,prf+31,&f);

			char *skin;
			WORD(*sfx)[2], flame_sfx; if(npc)
			{
				auto p = (swordofmoonlight_prf_npc_t*)(prf+62);
				sfx = p->dataSFX;
				flame_sfx = p->flameSFX; skin = (char*)&p->skin;
			}
			else
			{
				auto p = (swordofmoonlight_prf_enemy_t*)(prf+62);
				sfx = p->dataSFX;
				flame_sfx = p->flameSFX; skin = (char*)&p->skin;
			}
			if(*skin&&skin[1])
			{
				//HACK: counting on different extensions to not break filter
				som_art_model(v,npc?"npc\\texture":"enemy\\texture",skin+1,&f);
			}

			for(int j,k=0;k<32;k++) if(j=sfx[k][0])
			for(auto*d=(swordofmoonlight_prf_data_t*)(prf+sfx[k][1]);j-->0;d++)
			{
				if(d->effect&&d->effect<1024)
				if(SOM::DATA::Sfx.dat->open())
				sfx_mdl[SOM::DATA::Sfx.dat->records[d->effect][1]] = true;
			}
			if(flame_sfx<1024&&SOM::DATA::Sfx.dat->open())
			{
				sfx_mdl[SOM::DATA::Sfx.dat->records[flame_sfx][1]] = true;
			}
		}
		goto kage; //I guess?
	}
	else if(sz)
	{
		bool my = false, obj = false;
		char *dir; switch(som_prm_tab)
		{
		case 1001: sz = 0x158c8; dir = "item\\model"; break;
		case 1003: sz = 0x1410c; my = true; break;
		case 1007: sz = 0xe6a4; obj = true; dir = "obj\\model"; break;
		default: assert(0); goto beep; //shops?
		}	
		sz+=(int)&SOM_PRM_47a708.page(); 
		char *ll = *(char**)((char*)sz);
		for(int i=som_prm_tab==1007?1024:250;i-->0&&ll;)
		{	
			char *m = ll+31; if(my)
			{				
				DWORD sfx = *(WORD*)(m+1); 
				
				if(*m) //workshop_ReadFile_my
				{
					sfx = sfx<12?207:208+sfx-12;
				}
				else if(sfx<1024&&SOM::DATA::Sfx.dat->open())
				{
					sfx = SOM::DATA::Sfx.dat->records[sfx][1];					
				}
				else sfx = ~0;

				if(sfx&&sfx<256) sfx_mdl[sfx] = true; else assert(0);
			}
			else
			{
				som_art_model(v,dir,m,&f);

				if(obj) //SFX?
				{
					auto p = ((swordofmoonlight_prf_object_t*)ll+62);

					for(int j=2;j-->0;)
					{
						WORD sfx = j?p->flameSFX:p->trapSFX;

						if(sfx<1024&&SOM::DATA::Sfx.dat->open())
						{
							sfx_mdl[SOM::DATA::Sfx.dat->records[sfx][1]] = true;
						}
					}
				}
			}
			ll = *(char**)(ll+span);
		}	
		if(som_prm_tab==1001) //I guess?
		{
			som_art_model(v,"item\\model","gold.mdo",0);
			som_art_model(v,"my\\model","arm.mdl",0);
	kage:	som_art_model(v,"enemy\\model","kage.mdl",0);
		}
	}
	else if(som_prm_tab==1004) //shops?
	{
		auto *shop = (SOM::Shop*)SOM_PRM_47a708.data();

		bool items[250] = {}; for(int i=128;i-->0;shop++)
		{
			for(int j=250;j-->0;) if(shop->stock[j]) 
			{
				items[j] = true;
			}
		}

		char *item_pr2 = 0; //TODO: IMPLEMENT prf::maptofile
		{
			const wchar_t *w = 
			SOM::Tool.project(SOMEX_(B)"\\param\\item.pr2");
			if(FILE*g=_wfopen(w,L"rb"))
			{
				fseek(g,0,SEEK_END); sz = ftell(g);
				fseek(g,0,SEEK_SET);				
				item_pr2 = new char[sz];
				sz = fread(item_pr2,1,sz,g); fclose(g);
			}
			else sz = 0;
		}

		char *i = (char*)&SOM_PRM_47a708.page()+0x174;

		for(int j=0;j<250;j++,i+=336) if(items[j]) 
		{
			int prf = *(WORD*)i*88; 			
			if(prf<250*88&&prf+62+4<sz)
			som_art_model(v,"item\\model",item_pr2+prf+31+4,&f);
		}

		delete[] item_pr2;
	}
	else assert(0); //what is/was 1002?

	//f.clear(); //sfx?
	{
		char m[32];	
		for(int i=1;i<256;i++) if(sfx_mdl[i])
		{
			//NOTE: som_art_model switches to .txr when needed 
			sprintf(m,"%04d.mdl",i);
			som_art_model(v,"sfx\\model",m,0);
		}
	}
	
	if(v.size()==cmp) beep:
	{
		//MessageBeep(-1); //MessageBox should be generated
		return 0;
	}

	HWND pb = SOM_MAP_or_SOM_PRM_progress(som_tool_stack[1],v.size()-cmp);
	HWND task = GetParent(pb);
	HWND text = GetDlgItem(task,2021);	
	if(text) //Converting 3D art. Please wait...
	{
		GetWindowTextW(text,som_tool_text,MAX_PATH);
		SetDlgItemTextW(task,1151,som_tool_text);
	}
	return pb; //x2mdl does PBM_STEPIT
}