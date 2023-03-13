
#include "Ex.h" 
EX_TRANSLATION_UNIT

#include <map>
#include <string>

#include "Ex.ini.h"

#include "SomEx.h"
#include "som.game.h"
#include "som.tool.hpp"
#include "som.state.h"
#include "som.files.h"

extern void som_game_60fps_move(SOM::Struct<22>[250],int);

namespace SOM //2021
{
	namespace Files = PARAM;

	namespace PARAM
	{
		enum _E //INTERNAL
		{
			Sys_dat=0,
			Item_prm,Item_pr2,Item_arm,
			Magic_prm,Magic_pr2,
			Enemy_prm,Enemy_pr2,
			NPC_prm,NPC_pr2,
			Obj_prm,Obj_pr2,
		};
		namespace cpp
		{
			static int _rmask = 0;

			bool read(_E e)
			{
				if(_rmask&(1<<e)) return true;

				_rmask|=(1<<e); return false;
			}
			void unread(_E e)
			{
				_rmask&=~(1<<e);
			}
		}
	}
}

#define SOM_FILES_FOPEN(b) _wfopen(\
(SOM::tool?SOM::Tool::project:SOM::Game::project)\
(SOMEX_(B)"\\"b,1),L"rb")

#define SOM_FILES_FOPENF(a,b,...) _wfopen(\
(SOM::tool?SOM::Tool::project:SOM::Game::project)\
((sprintf(&(*a='\0'),SOMEX_(B)"\\"b,__VA_ARGS__),a),1),L"rb")

struct SOM::PARAM::Sys SOM::PARAM::Sys; //extern

bool SOM::PARAM::Sys::Dat::open() //static
{
	if(SOM::PARAM::Sys.dat) return true;
	if(cpp::read(Sys_dat)) return false;

	delete SOM::PARAM::Sys.dat; //2021
	SOM::PARAM::Sys.dat = 0;

	FILE *f = SOM_FILES_FOPEN("PARAM\\Sys.dat");

	//should be checking the file's size here
	if(!f||fseek(f,0x719,SEEK_SET)) return false;

	SOM::PARAM::Sys::Dat *p = new SOM::PARAM::Sys::Dat;

	//SOM::L.counter_names
	if(!fread(p->counters,31*1024,1,f))	return false;

	p->counters[1023][30] = '\0';

	//SOM::L.magic32
	if(fseek(f,0x292,SEEK_SET)
	||!fread(p->magics,32,1,f)) return false;

	//SOM::L.sys_dat_messages_0_8
	if(fseek(f,0x2D8,SEEK_SET)
	||!fread(p->messages,41*9,1,f)) return false;
												 
	//3 more are on the end???
	//SOM::L.sys_dat_messages_9_11
	if(fseek(f,0x8360,SEEK_SET)
	||!fread(p->messages[9],41*3,1,f)) return false;
		
	FLOAT walk, dash; SHORT turn;

	if(fseek(f,256,SEEK_SET)
	||!fread(&walk,sizeof(FLOAT),1,f)
	||!fread(&dash,sizeof(FLOAT),1,f)
	||!fread(&turn,sizeof(SHORT),1,f)) return false;

	p->walk = walk; p->dash = dash;
	p->turn = 3.141592f/180*turn;

	fclose(f); 

	delete SOM::PARAM::Sys.dat; //2021

	SOM::PARAM::Sys.dat = p; return true;
}

struct SOM::PARAM::Item SOM::PARAM::Item; //extern

bool SOM::PARAM::Item::Prm::open() //static
{
	if(SOM::PARAM::Item.prm) return true;
	if(cpp::read(Item_prm)) return false;

	delete SOM::PARAM::Item.prm; //2021
	SOM::PARAM::Item.prm = 0;

	FILE *f = SOM_FILES_FOPEN("PARAM\\Item.prm"); 

	if(!f) return false; //||fseek(f,2,SEEK_SET)

	SOM::PARAM::Item::Prm *p = new SOM::PARAM::Item::Prm;
				
	for(int i=0;i<250;i++)
	{
		if(!fread(p->profiles+i,2,1,f)
		||!fread(p->records+i,31,1,f)
		||fseek(f,336-31-2,SEEK_CUR)&&i!=249) return false;

		p->records[i][30] = '\0';	
	}	
	strcpy(p->records[250],"--");
	strcpy(p->records[251],"--");
	strcpy(p->records[252],"--");
	strcpy(p->records[253],"--");
	strcpy(p->records[254],"--");
	strcpy(p->records[255],"--");

	fclose(f);

	SOM::PARAM::Item.prm = p; return true;
}

/*UNUSED/ABANDONED (2021)
//som_map_codecbnproc has some code
//that suggests the idea here was to
//filter some comboboxes in SOM_MAP
bool SOM::PARAM::Item::Pr2::open() //static
{
	if(SOM::PARAM::Item.pr2) return true;
	if(cpp::read(Item_pr2)) return false;

	delete SOM::PARAM::Item.pr2; //2021
	SOM::PARAM::Item.pr2 = 0;

	assert(0); //SOM_MAP reopens this file!
	assert(0); //before this can be used
	//SOM_FILES_FOPEN must open PR~ and PR2 
	//files (as is it will open PRO files only)
	FILE *f = SOM_FILES_FOPEN("PARAM\\Item.pr2"); 
	
	DWORD count = 0; 
	if(!f||!fread(&count,4,1,f)) return false;
	assert(count<=Prm::records_s);
	if(count>Prm::records_s) return false;

	SOM::PARAM::Item::Pr2 *p = new SOM::PARAM::Item::Pr2;
	memset(p,0x00,sizeof(*p));
	
	unsigned char a, b;

	for(size_t i=0;i<count&&!feof(f);i++)
	{
		if(fseek(f,62,SEEK_CUR)||!fread(&a,1,1,f)
		  ||fseek(f,9,SEEK_CUR)||!fread(&b,1,1,f)
		  ||fseek(f,15,SEEK_CUR)) return false;

		switch(a)
		{
		case 0: p->uses[i] = supply; break;
		case 1: p->uses[i] = weapon; break;
		case 2: p->uses[i] = attire; 
			
			switch(b)
			{
			case 0:	p->fits[i].head = 1; break;
			case 1:	p->fits[i].body = 1; break;
			case 2:	p->fits[i].hand = 1; break;
			case 3:	p->fits[i].feet = 1; break;
			case 4:	p->fits[i].head = 1; 
					p->fits[i].body = 1; 
					p->fits[i].feet = 1; break;
			case 5: p->uses[i] = shield; break;
			case 6: p->uses[i] = effect; break;
			}
			break;
		}
	}	

	fclose(f);

	SOM::PARAM::Item.pr2 = p;

	return true;
}*/

void SOM::PARAM::Item::Arm::clear() //static
{
	cpp::unread(Item_arm);
	delete SOM::PARAM::Item.arm; //2021
	SOM::PARAM::Item.arm = 0;	
}
bool SOM::PARAM::Item::Arm::open() //static
{
	if(SOM::PARAM::Item.arm) return true;
	if(cpp::read(Item_arm)) return false;

	clear();

	return SOM::game?_game():_tool();
}
bool SOM::PARAM::Item::Arm::_game() //static
{
	FILE *f = SOM_FILES_FOPEN("PARAM\\Item.arm"); 
	if(!f) return false;

	SOM::PARAM::Item::Arm *p = new SOM::PARAM::Item::Arm;

	size_t rd = fread(p->records,88,1024,f);
	memset(p->records+rd,0x00,sizeof(*p)-rd*sizeof(record));

	fclose(f);

	//adjust time keys for 60 fps animation?
	if(EX::INI::Bugfix()->do_fix_animation_sample_rate)
	som_game_60fps_move((SOM::Struct<22>*)p->records,1023);

	assert(!SOM::PARAM::Item.arm);

	SOM::PARAM::Item.arm = p; return true;
}
bool SOM::PARAM::Item::Arm::_tool() //static
{
	SOM::PARAM::Item::Arm *p = new SOM::PARAM::Item::Arm;
	memset(p,0x00,sizeof(*p));

	auto &prf = p->records[records_s-1]; //1023
	int compile[sizeof(record)==88];

	for(int i=0;*EX::data(i);i++)
	{
		int d = SOM::tool==ItemEdit.exe?i:0xFF;

		wchar_t buf[MAX_PATH];
		int cat = swprintf(buf,L"%s\\my\\arm\\*.prf",EX::data(i))-5;
		if(cat<0) goto err;
		WIN32_FIND_DATAW data; 
		auto h = FindFirstFile(buf,&data);
		FILE *f = 0;
		if(h!=INVALID_HANDLE_VALUE) do
		{
			wcsncpy(buf+cat,data.cFileName,MAX_PATH-cat);
			f = _wfopen(buf,L"rb");
			
			//TODO: check file size? //say something?
			if(!f||!fread(&prf,sizeof(prf),1,f)) continue; 

			DWORD mv = prf._0a|prf._0b<<8;
			
			if(mv==1023||p->records[mv].my) continue;

			auto &dst = p->records[mv];

			//YUCK: the model/name fields are swapped
			memcpy(dst.description,&prf,30);
			memcpy(dst.description+31,prf.description+31,88-31*2);
			char *e;
			dst.mid = (WORD)strtol(prf.description,&e,10);
			if(*e||*e=='-'||e==(char*)&dst)
			dst.mid = (WORD)mv;

			dst.mvs[0] = (WORD)mv;
			dst.mvs[1] = dst._1;
			dst.mvs[2] = dst._2;
			dst.mvs[3] = dst._3;

			//leave the 0-terminator 0
			memset(dst._reserved,0xff,sizeof(dst._reserved)-1);
			//ItemEdit?
			dst._reserved[sizeof(dst._reserved)-2] = (BYTE)i;

		}while(f?fclose(f):0,FindNextFileW(h,&data));
		FindClose(h);
	}
	memset(&prf,0x00,sizeof(prf)); //1023

	assert(!SOM::PARAM::Item.arm);

	SOM::PARAM::Item.arm = p;

	return true; err: assert(0);

	delete p; return false;
}

struct SOM::PARAM::Magic SOM::PARAM::Magic; //extern

bool SOM::PARAM::Magic::Prm::open() //static
{
	if(SOM::PARAM::Magic.prm) return true;
	if(cpp::read(Magic_prm)) return false;

	delete SOM::PARAM::Magic.prm; //2021
	SOM::PARAM::Magic.prm = 0;

	FILE *f = SOM_FILES_FOPEN("PARAM\\Magic.prm");	

	if(!f||fseek(f,2,SEEK_SET)) return false;

	SOM::PARAM::Magic::Prm *p = new SOM::PARAM::Magic::Prm;
	
	for(int i=0;i<250;i++)
	{
		if(!fread(p->records+i,31,1,f)
		||fseek(f,320-31,SEEK_CUR)&&i!=249)
		{
			fclose(f); return false;
		}

		p->records[i][30] = '\0';
	}	
	strcpy(p->records[250],"--");
	strcpy(p->records[251],"--");
	strcpy(p->records[252],"--");
	strcpy(p->records[253],"--");
	strcpy(p->records[254],"--");
	strcpy(p->records[255],"--");
	 
	fclose(f);

	SOM::PARAM::Magic.prm = p; return true;
}

struct SOM::PARAM::Enemy SOM::PARAM::Enemy; //extern

bool SOM::PARAM::Enemy::Prm::open() //static
{
	if(SOM::PARAM::Enemy.prm) return true;
	if(cpp::read(Enemy_prm)) return false;

	delete SOM::PARAM::Enemy.prm; //2021
	SOM::PARAM::Enemy.prm = 0;

	FILE *f = SOM_FILES_FOPEN("PARAM\\Enemy.prm");	

	if(!f||fseek(f,8,SEEK_SET)) return false;

	SOM::PARAM::Enemy::Prm *p = new SOM::PARAM::Enemy::Prm;

	for(int i=0;i<1024;i++)
	{
		if(!fread(p->records+i,31,1,f)
		||fseek(f,488-31,SEEK_CUR)&&i!=1023)
		{
			fclose(f); return false;
		}

		p->records[i][30] = '\0';
	}
		
	fclose(f);

	SOM::PARAM::Enemy.prm = p; return true;
}

struct SOM::PARAM::NPC SOM::PARAM::NPC; //extern

bool SOM::PARAM::NPC::Prm::open() //static
{
	if(SOM::PARAM::NPC.prm) return true;
	if(cpp::read(NPC_prm)) return false;

	delete SOM::PARAM::NPC.prm; //2021
	SOM::PARAM::NPC.prm = 0;

	FILE *f = SOM_FILES_FOPEN("PARAM\\NPC.prm");	

	if(!f||fseek(f,8,SEEK_SET)) return false;

	SOM::PARAM::NPC::Prm *p = new SOM::PARAM::NPC::Prm;

	for(int i=0;i<1024;i++)
	{
		if(!fread(p->records+i,31,1,f)
		||fseek(f,320-31,SEEK_CUR)&&i!=1023) 
		{
			fclose(f); return false;
		}

		p->records[i][30] = '\0';
	}
		
	fclose(f);

	SOM::PARAM::NPC.prm = p; return true;
}

struct SOM::DATA::Map SOM::DATA::Map[64]; //extern

struct SOM::DATA::Map::Sys SOM::DATA::Map::sys = {}; //static

bool SOM::DATA::Map::Evt::open(int n) //static
{
	if(n!=-1)
	{
		if(n<0||n>63) return false;

		if(SOM::DATA::Map[n].evt) 
		return !SOM::DATA::Map[n].evt->error;
	}
	else if(SOM::DATA::Map::sys.ezt)
	{
		return !SOM::DATA::Map::sys.ezt->error;
	}
	
	SOM::DATA::Map::Evt *p = new SOM::DATA::Map::Evt;

	if(n!=-1) SOM::DATA::Map[n].evt = p;
	if(n==-1) SOM::DATA::Map::sys.ezt = p;

	FILE *f = 0; char pathf[MAX_PATH];
	if(n==-1) f = SOM_FILES_FOPEN("DATA\\Map\\sys.ezt");
	if(n!=-1) f = SOM_FILES_FOPENF(pathf,"DATA\\Map\\%02d.evt",n);

	if(!f||!fread(p->header,4,1,f)
	||!fread(p->records,252*1024,1,f))
	{
		if(f) fclose(f); return false;		
	}
	if(f) fclose(f); p->error = false; return true;
}

struct SOM::DATA::Sfx SOM::DATA::Sfx; //extern

void SOM::DATA::Sfx::Dat::clear() //static
{
	delete SOM::DATA::Sfx.dat; 
	
	SOM::DATA::Sfx.dat = 0;
}
bool SOM::DATA::Sfx::Dat::open() //static
{
	if(SOM::DATA::Sfx.dat) 
	return !SOM::DATA::Sfx.dat->error;

	SOM::DATA::Sfx::Dat *p = new SOM::DATA::Sfx::Dat;
	SOM::DATA::Sfx.dat = p;

	//old projects may source Sfx.dat from the install
	//new projects should have it copied into the data
	//folder
	//FILE *f = SOM_FILES_FOPEN("DATA\\Sfx\\Sfx.dat");
	FILE *f = 0; wchar_t path[MAX_PATH];	
	if(EX::data(L"Sfx\\Sfx.dat",path))
	{
		f = _wfopen(path,L"rb");
	}
	if(!f||!fread(p->records,48*1024,1,f))
	{
		//2021: don't crash SOM_MAP_art_files
		memset(p->records,0x00,sizeof(p->records));

		if(f) fclose(f); return false;		
	}
	if(f) fclose(f); p->error = false; return true;
}

struct SOM::PARAM::Obj SOM::PARAM::Obj; 

bool SOM::PARAM::Item::Prm::wrote = false;
bool SOM::PARAM::Item::Pr2::wrote = false;
bool SOM::PARAM::Magic::Prm::wrote = false;
bool SOM::PARAM::Magic::Pr2::wrote = false;
bool SOM::PARAM::Enemy::Prm::wrote = false;
bool SOM::PARAM::Enemy::Pr2::wrote = false;
bool SOM::PARAM::NPC::Prm::wrote = false;
bool SOM::PARAM::NPC::Pr2::wrote = false;
bool SOM::PARAM::Obj::Prm::wrote = false;
bool SOM::PARAM::Obj::Pr2::wrote = false;

static VOID CALLBACK som_files_wrote(HWND win, UINT, UINT_PTR idEvent, DWORD)
{
	const wchar_t *dir = (wchar_t*)idEvent;

	EX_CRITICAL_SECTION //mainly for onWrite at the end

	if(win&&!KillTimer(win,idEvent)){ assert(0); return; }	
	
	//2022: expecting /PARAM or /parts
	int p = wcsrchr(dir,'/')[1]; assert(p=='p'||p=='P');

	enum
	{
		Prm=0, Pr2, Pro, m,
		Item=0, Magic, Enemy, NPC, Obj, n,		
	};
	struct times{ FILETIME tt[m][n]; };
	static std::map<std::wstring,times> timetable;
	typedef std::map<std::wstring,times>::iterator it;
	if(!dir) //trigger_write_monitor mode (forced refresh)
	{
		for(it i=timetable.begin();i!=timetable.end();i++) 
		som_files_wrote(0,0,(UINT_PTR)i->first.c_str(),0); return;
	}
	it writ; if(p=='P') //PARAM?
	{
		writ = timetable.find(dir); if(writ==timetable.end()) 
		{
			times ins; memset(&ins,0x00,sizeof(ins));
			writ = timetable.insert(std::make_pair(dir,ins)).first;
		}
	}

	bool wrote = false; //2022

	WIN32_FIND_DATAW found; 
	wchar_t spec[MAX_PATH] = L"";
	int fn = swprintf_s(spec,L"%ls\\*",dir)-1;	
	HANDLE glob = FindFirstFileW(spec,&found);
	if(glob!=INVALID_HANDLE_VALUE) do
	{
		size_t i,j; wchar_t *ext = 
		PathFindExtensionW(found.cFileName);

		//HACK: covers prm, pro (pr2) prt and prf (if need be)
		if(ext[0]!='.'||tolower(ext[1])!='p'||tolower(ext[2])!='r')
		continue;
		
		if(p=='p') //parts?
		{
			if(tolower(ext[3])!='t'||ext[4]) continue;
				
			//NOTE: this is really not effecient in terms of 
			//scanning the PARTS directory (which can be big)
			auto &k = SOM_MAP.prt[found.cFileName];
			if(!k||CompareFileTime(&k.writetime,&found.ftLastWriteTime)>=0)
			continue;

			k.writetime = found.ftLastWriteTime;	

			//NOTE: assuming PRT is in the project data folder
			//so it won't be confused with another data folder
			//NOTE: "user" directory dominance isn't protected

			extern void som_map_syncup_prt(int,wchar_t[]);
			som_map_syncup_prt(k.number(),wcscpy(spec+fn,found.cFileName)-fn);

			//HACK: this will almost always be true unless the
			//files were modified outside
			extern WORD workshop_category;
			if(workshop_category==k.number()) wrote = true;
		}
		else if(p=='P') //PARAM?
		{
			switch(tolower(ext[3]))
			{			
			case 'm': i = Prm; if(!ext[4]) break;
			case '2': i = Pr2; if(!ext[4]) break;
			case 'o': i = Pro; if(ext[4]) default: continue;
			}			
			#define _(x) j = x; \
			if(!wcsnicmp(L#x L".",found.cFileName,sizeof(#x)))\
			if(ext==found.cFileName+sizeof(#x)-1) break;				
			switch(tolower(*found.cFileName))
			{
			case 'm': _(Magic) case 'i': _(Item) 
			case 'e': _(Enemy) case 'n': _(NPC)	
			case 'o': _(Obj) default: continue;			  			
			}

			if(i>=m||j>=n) continue; //paranoia

			FILETIME &ft = writ->second.tt[i][j];
			if(ft.dwLowDateTime||ft.dwHighDateTime)
			if(CompareFileTime(&ft,&found.ftLastWriteTime)<0)
			{
				switch(i)
				{
				case Prm: switch(j)
				{
				#define _(x) case x: SOM::PARAM::x::Prm::wrote = wrote = true;\
				SOM::PARAM::cpp::unread(SOM::PARAM::x##_##prm); break; //2021
				_(Item)_(Magic)_(Enemy)_(NPC)_(Obj)
				}
				case Pr2: case Pro: switch(j)
				{
				#define _(x) case x: SOM::PARAM::x::Pr2::wrote = wrote = true;\
				SOM::PARAM::cpp::unread(SOM::PARAM::x##_##pr2); break; //2021
				_(Item)_(Magic)_(Enemy)_(NPC)_(Obj)
				}}
				#undef _
			}
			ft = found.ftLastWriteTime;	
		}
		else assert(0);

	}while(FindNextFileW(glob,&found));
	FindClose(glob);	

	if(wrote) if(p=='P') //PARAM?
	{
		if(SOM::PARAM::onWrite) SOM::PARAM::onWrite();
	}
	else if(p=='p'&&SOM::tool==SOM_MAP.exe) //parts
	{
		//2022: just update the palette view preview?
		//NOTE: icons are more work. I think I will get
		//around to them before long
		extern WORD workshop_category; //HACK (used elsewhere)
		auto &k = SOM_MAP.prt[workshop_category];
		auto *tp = ((SOM_CWnd*(__stdcall*)(HWND))0x468441)(win);
		if(auto*p=SOM_MAP_4921ac.find_part_number(k.part_number()))
		((void(__thiscall*)(void*,int,int))0x417250)(tp,p-SOM_MAP_4921ac.parts,1);
	}
}						
extern void SOM::PARAM::trigger_write_monitor()
{
	som_files_wrote(0,0,0,0); //force refresh
}
extern void (*SOM::PARAM::onWrite)() = 0;

//demo: Obtaining Directory Change Notifications 
//Reminder: ReadDirectoryChangesW can be used to 
//get at the individual files that were modified
//however its kind of a hairy beast just to look
//at and scanning the directory isn't a big deal
static DWORD WINAPI som_files_threadproc(LPVOID hw)
{	
	//Reminder: read somewhere the Samba team isn't
	//supporting ReadDirectoryChangesW and that the
	//implementation of FindFirstChangeNotification
	//does not support or extend to sub-directories

	LUID priv; HANDLE tok; //http://support.microsoft.com/kb/188321
	if(LookupPrivilegeValueA(0,"SeBackupPrivilege",&priv) //SE_BACKUP_NAME
	&&OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES,&tok))
	{
		TOKEN_PRIVILEGES tp = {1};
		tp.Privileges[0].Luid = priv;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		if(!AdjustTokenPrivileges(tok,0,&tp,0,0,0)) 
		assert(0);
	}
	else assert(0);

	reset: //not sure why this is needed but it's always been here

	size_t nChangeHandles = 0;
	HANDLE dwChangeHandles[4]; 
	wchar_t *dwChangeFolders[4];
	int n = wcsicmp(EX::user(0),EX::cd())?4:2;
	for(int i=0;i<n;i++)
	{
		auto src = i<2?EX::cd():EX::user(0);
		auto fmt = L"%ls/PARAM"; if(i%2) //2022
		{
			fmt = L"%ls/data/map/parts"; //SOM_MAP?

			if(SOM::tool!=SOM_MAP.exe){ assert(0); continue; }
		}
		//HACK: som_files_wrote USES /p OR /P TO FIND ITS BERINGS!!
		//swprintf_s(dir,i%2?L"%ls/data/map/parts":L"%ls/PARAM",src);
		wchar_t dir[MAX_PATH]; swprintf_s(dir,fmt,src);

		if(INVALID_HANDLE_VALUE!=(dwChangeHandles[nChangeHandles] = 
		FindFirstChangeNotificationW(dir,0,FILE_NOTIFY_CHANGE_LAST_WRITE)))
		{
			dwChangeFolders[nChangeHandles++] = wcsdup(dir); //2022
		}
	}

	//if(!one_off) //initialize write times
	{
		//2022: kickoff_write_monitoring_thread (below) is
		//guarding against more than one thread ever being
		//created
		//one_off = true;

		//2022: I'm pretty sure I noticed a reason this is 
		//inefficient (skins maybe? can't recall)
		int todolist[SOMEX_VNUMBER<=0x1020402UL];

		for(size_t i=0;i<nChangeHandles;i++)
		som_files_wrote(0,0,(UINT_PTR)dwChangeFolders[i],0);	
	}

	if(nChangeHandles) for(;;) //wait to be notified
	{ 		
		//read this might help somewhere
		DWORD hacked = INFINITE; //1000;

		DWORD dwWaitStatus = 
		WaitForMultipleObjects(nChangeHandles,dwChangeHandles,0,hacked); 		
		if(dwWaitStatus<nChangeHandles) 
		{
			enum{ t=500 }; //1000 is too long for saving PRT files
			SetTimer((HWND)hw,(UINT_PTR)dwChangeFolders[dwWaitStatus],t,som_files_wrote);
			FindNextChangeNotification(dwChangeHandles[dwWaitStatus]);		
		}
		else //if(hacked!=INFINITE)
		{
			while(nChangeHandles-->0)
			{
				free(dwChangeFolders[nChangeHandles]); //2022
				
				FindCloseChangeNotification(dwChangeHandles[nChangeHandles]);
			}
			goto reset;
		}
	}	
	else assert(0); return 0;
}

void SOM::PARAM::kickoff_write_monitoring_thread(HWND hwnd)
{
	static DWORD one_off = 0; if(!one_off&&hwnd)
	CloseHandle(CreateThread(0,0,som_files_threadproc,hwnd,0,&one_off));	
}