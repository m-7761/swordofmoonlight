
#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

#include <map>
#include <string>

#include "Ex.ini.h"

#include "SomEx.h"
#include "som.game.h"
#include "som.tool.hpp"
#include "som.state.h"
#include "som.files.h"

#include "../lib/swordofmoonlight.h"
#include "../x2mdl/x2mdl.h"

extern void som_game_60fps_move(SOM::Struct<22>[250],int);

namespace som_MPX_swap
{
	extern void *models_data(char*),models_refresh(char*); 
}

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
typedef SWORDOFMOONLIGHT::zip::inflate_t xprof;
extern size_t som_tool_xdata(const wchar_t*,xprof&);
bool SOM::PARAM::Item::Arm::_tool() //static
{
	xprof *xp = new xprof, &x = *xp; //2023

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

			//2023: store filename and translate item.arm
			wmemcpy(p->files[mv].name,data.cFileName,30); 
			p->files[mv].data = (BYTE)i;
			swprintf(data.cFileName,L"data\\my\\arm\\%s",p->files[mv].name);
			size_t xsz = som_tool_xdata(data.cFileName,x);
			if(xsz) memcpy(&prf,&x,30);

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

		}while(f?fclose(f):0,FindNextFileW(h,&data));
		FindClose(h);
	}
	memset(&prf,0x00,sizeof(prf)); //1023

	assert(!SOM::PARAM::Item.arm);

	SOM::PARAM::Item.arm = p;

	return true; err: assert(0);

	delete xp; //2023

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
	swprintf(path,L"%s\\PARAM\\SFX.DAT",EX::cd()); //2024
	f = _wfopen(path,L"rb");
	if(!f&&EX::data(L"Sfx\\Sfx.dat",path))
	f = _wfopen(path,L"rb");
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

extern WORD workshop_category;
static FILETIME som_files_wrote_init = {};
static VOID CALLBACK som_files_wrote_pr(HWND win, UINT, UINT_PTR idEvent, DWORD)
{
	const wchar_t *dir = (wchar_t*)idEvent;

	if(!dir) assert(dir); //2023
	if(!dir) return SOM::PARAM::trigger_write_monitor(); //PARAM\parts?	

	EX_CRITICAL_SECTION //mainly for onWrite at the end

	if(win&&!KillTimer(win,idEvent)){ assert(0); return; }	
	
	//2022: expecting \PARAM or \parts
	//int p = wcsrchr(dir,'/')[1]; assert(p=='p'||p=='P');
	int p = *PathFindFileName(dir); assert(p=='p'||p=='P');

	enum
	{
		Prm=0, Pr2, Pro, M,
		Item=0, Magic, Enemy, NPC, Obj, N,		
	};
	struct times
	{
		FILETIME tt[M][N]; times()
		{
			for(int i=M*N;i-->0;)			
			tt[0][i] = som_files_wrote_init;
		}
	};
	static std::map<std::wstring,times> timetable;
	
	if(!dir) //trigger_write_monitor mode (forced refresh)
	{
		for(auto it=timetable.begin();it!=timetable.end();it++) 
		som_files_wrote_pr(0,0,(UINT_PTR)it->first.c_str(),0); return;
	}
	auto writ = timetable.end(); if(p=='P') //PARAM?
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
	int fn = swprintf_s(spec,L"%ls\\*.pr?",dir)-5; //* -1	
	HANDLE glob = FindFirstFileW(spec,&found);
	if(glob!=INVALID_HANDLE_VALUE) do
	{
		size_t i,j; wchar_t *ext = 
		PathFindExtensionW(found.cFileName);

		//HACK: covers prm, pro (pr2) prt and prf (if need be)
		if(ext[0]!='.'||tolower(ext[1])!='p'||tolower(ext[2])!='r')
		{
			assert(0); continue;
		}
		
		if(p=='p') //parts?
		{
			if(tolower(ext[3])!='t'||ext[4]) continue; 
					
			//NOTE: this is really not effecient in terms of 
			//scanning the PARTS directory (which can be big)
	app:	auto &k = SOM_MAP.prt[found.cFileName]; if(k)
			{
				if(CompareFileTime(&k.writetime,&found.ftLastWriteTime)>=0)
				continue;
			}
			else //TODO: try to add PRT to SOM_MAP?
			{
				extern bool som_map_append_prt(wchar_t path[MAX_PATH]);
				if(!som_map_append_prt(wcscpy(spec+fn,found.cFileName)-fn))
				continue;
				goto app;
			}

			k.writetime = found.ftLastWriteTime;	

			//NOTE: assuming PRT is in the project data folder
			//so it won't be confused with another data folder
			//NOTE: "user" directory dominance isn't protected

			extern void som_map_syncup_prt(int,wchar_t[]);
			som_map_syncup_prt(k.number(),wcscpy(spec+fn,found.cFileName)-fn);

			//HACK: this will almost always be true unless the
			//files were modified outside
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

			if(i>=M||j>=N) continue; //paranoia

			FILETIME &ft = writ->second.tt[i][j];
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
		//this isn't working... can't trace beyond 4300af?
		//(debugger stops and app is raised to foreground)
	//	auto *tp = SOM_MAP_app::CWnd(win);
	//	((void(__thiscall*)(void*,DWORD))0x430080)(tp,SOM_MAP.prt[workshop_category].part_number());		
	}
}						
extern void SOM::PARAM::trigger_write_monitor()
{
	som_files_wrote_pr(0,0,(UINT_PTR)L"\\PARAM",0); //force refresh
	som_files_wrote_pr(0,0,(UINT_PTR)L"\\parts",0); //2022
}
extern void (*SOM::PARAM::onWrite)() = 0;

extern void som_MPX_refresh_mpx(int);
extern void som_MPX_refresh_evt(int);
static VOID CALLBACK som_files_wrote_db(UINT_PTR cat, wchar_t *dir) //RECURSIVE
{
	static std::map<std::wstring,FILETIME> timetable;

	for(auto*p=dir+cat;*p;p++) *p = tolower(*p);

	auto ins = timetable.insert(std::make_pair(dir,som_files_wrote_init));

	FILETIME &writ = ins.first->second;
	
	auto *w = ins.first->first.c_str();

	auto map = wcsstr(w,L"\\map");

	if(map)
	{
		if(map[4]=='\\') return; //!
		if(map[4]!='\0') map = nullptr;
	}
	bool model = wcsstr(w,L"\\model");

	bool prof = wcsstr(w,L"\\prof"); //2024

	if(SOM::tool==SOM_PRM.exe) 
	{
		model = map = false;
	}
	else prof = false;

	FILETIME time = writ;

	WIN32_FIND_DATAW found; 
	wchar_t spec[MAX_PATH] = L"";
	int fn = swprintf_s(spec,L"%ls\\*",dir)-1;
	HANDLE glob = FindFirstFileW(spec,&found);
	if(glob!=INVALID_HANDLE_VALUE) do
	{
		if(found.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
		{
			if('.'!=found.cFileName[0])
			{
				wchar_t dir2[MAX_PATH];
				swprintf(dir2,L"%s\\%s",dir,found.cFileName);
				som_files_wrote_db(cat,dir2);
			}
			continue;
		}

		if(CompareFileTime(&writ,&found.ftLastWriteTime)<0)
		{
			writ = found.ftLastWriteTime;
		}
		if(CompareFileTime(&time,&found.ftLastWriteTime)<0)
		{	
			//NOTE: if these are reloaded from the wrong file
			//it will cause the right file to be reloaded, so
			//no check is done here...

			extern SOM::Thread *som_MPX_thread;
			EX::section raii(som_MPX_thread->cs);

			if(model)
			{
				char a[10+64];
				snprintf(a,sizeof(a),"A:\\>\\data\\%ls\\%ls",dir+cat+1,found.cFileName);
				if(void *m=som_MPX_swap::models_data(a))
				{
					using namespace x2mdl_h;
					extern int som_art_model(WCHAR*,wchar_t[MAX_PATH]);
					extern int som_art(const wchar_t *path, HWND hwnd);
					wchar_t w[MAX_PATH],art[MAX_PATH];
					swprintf(w,L"%s\\%s",dir+cat+1,found.cFileName);
					int e = som_art_model(w,art); 
					if(e&_art&&~e&_lnk)	
					if(!som_art(art,0)) //x2mdl exit code?
					{
						e = som_art_model(w,art); //retry?
					}
					if(e&(_mdl|_mdo)) //generically reload?
					{
						som_MPX_swap::models_refresh(a);
					}
					else assert(0);
				}
			}
			else if(map)
			{
				if(!isdigit(found.cFileName[0])) continue;

				int i = _wtoi(found.cFileName);
				auto ext = PathFindExtension(found.cFileName);
				if(!wcsicmp(L".mpx",ext))
				{
					if(i==SOM::mpx)
					{
						auto &dst = *SOM::L.corridor;

						som_MPX &mpx = *SOM::L.mpx->pointer;

						//handoff to som_MPX_411a20?
						dst.lock = 1;
						dst.nosetting = 2;
					//	memcpy(mpx.f+121,SOM::L.pcstate,3*sizeof(float));
					}
										
					som_MPX_refresh_mpx(i);
				}
				//else if(!wcsicmp(L".evt",ext))
				{
				//	som_MPX_refresh_evt(i);					
				}
			}
			else if(prof) //2024: now what?
			{
				auto ext = PathFindExtension(found.cFileName);
				if(wcsicmp(ext,L".prf")) continue;

				wchar_t path[MAX_PATH];
				swprintf(path,L"%s\\%s",path,found.cFileName);

				HANDLE h = CreateFile(path,SOM_GAME_READ);
				DWORD sz = GetFileSize(h,0);
				CloseHandle(h);

				extern HWND &som_tool; //SOM_PRM

				//TODO: update current tab?
				if(h!=INVALID_HANDLE_VALUE)				
				Sompaste->database_insert(som_tool,path,sz);				
			}
		}

	}while(FindNextFileW(glob,&found));
	FindClose(glob);
}
static VOID CALLBACK som_files_wrote_db2(HWND win, UINT, UINT_PTR idEvent, DWORD)
{	
	EX_CRITICAL_SECTION //mainly for onWrite at the end

	if(win&&!KillTimer(win,idEvent)){ assert(0); return; }

	if(idEvent<65536) //data?
	{
		auto *dir = EX::data(idEvent);
		som_files_wrote_db(wcslen(dir),const_cast<wchar_t*>(dir));
	}
	else //PARAM?
	{
		//FUN_0043ce10_load_PARAM_shop_dat();
				
		//EXPERIMENTAL
		#define _(x) delete x; x = nullptr;
		auto &fix = EX::INI::Bugfix()->do_fix_animation_sample_rate;
		if(1)
		{
			//TODO: reload tables in memory? mpx?
			_(SOM::PARAM::Sys.dat)
			if(FILE*f=SOM_FILES_FOPEN("param\\sys.dat","rb"))
			{
				fread(SOM::L.sys_dat,1,33760,f);
				fclose(f);
			}
			_(SOM::PARAM::Item.prm)
			//FUN_0040fa10_init_items_data_various()
			if(FILE*f=SOM_FILES_FOPEN("param\\item.prm","rb"))
			{
				fread(SOM::L.item_prm_file,1,250*84*4,f);
				fclose(f);

				if(f=SOM_FILES_FOPEN("param\\item.pr2","rb"))
				{
					int n = fread(&SOM::L.item_pr2_size,4,1,f);
					int rd = fread(SOM::L.item_pr2_file,4*22,256,f);
					assert(rd==SOM::L.item_pr2_size);
					fclose(f);
				}
				
				fix.item = 1;
			}
			_(SOM::PARAM::Magic.prm)			
			//FUN_004260f0_load_magic_param_files()
			if(FILE*f=SOM_FILES_FOPEN("param\\magic.prm","rb"))
			{
				fread(SOM::L.magic_prm_file,1,250*80*4,f);
				fclose(f);

				if(f=SOM_FILES_FOPEN("param\\magic.pr2","rb"))
				{
					int n = fread(&SOM::L.magic_pr2_size,4,1,f);
					int rd = fread(SOM::L.magic_pr2_file,4*10,256,f);
					assert(rd==SOM::L.magic_pr2_size);
					fclose(f);
				}

				fix.magic = 1;
			}
			//FUN_0042a430_load_object_param_data()
			if(FILE*f=SOM_FILES_FOPEN("param\\obj.prm","rb"))
			{
				fread(SOM::L.obj_prm_file,1,1024*14*4,f);
				fclose(f);

				if(f=SOM_FILES_FOPEN("param\\obj.pr2","rb"))
				{
				//	int m = SOM::L.obj_pr2_size; //not kept???
				//	DWORD n = fread(&SOM::L.obj_pr2_size,4,1,f);
					DWORD sz,n = fread(&sz,4,1,f);
					DWORD rd = fread(SOM::L.obj_pr2_file,4*27,1024,f);
					assert(rd==sz);
					fclose(f);
				}

				fix.obj = 1;
			}
			_(SOM::PARAM::Enemy.prm)
			//FUN_00405bb0_load_enemy_param_data()
			if(FILE*f=SOM_FILES_FOPEN("\param\\enemy.prm","rb"))
			{
				fread(SOM::L.enemy_prm_file,1,1024*122*4,f);
				fclose(f);

				  /*see som_game_reload_enemy_npc_pr2_files*/

				fix.enemy = 1; //som.game.cpp synchronizes these
			}
			_(SOM::PARAM::NPC.prm)  
			//FUN_00428780_load_npc_param_data()
			if(FILE*f=SOM_FILES_FOPEN("\param\\npc.prm","rb"))
			{
				fread(SOM::L.NPC_prm_file,1,1024*80*4,f);
				fclose(f);

				  /*see som_game_reload_enemy_npc_pr2_files*/

				fix.npc = 1; //som.game.cpp synchronizes these
			}
			SOM::PARAM::Item.arm->clear();

			extern void som_game_60fps(),som_game_equip();
			som_game_60fps();			
			som_game_equip();

		}
		#undef _
	}
}

//demo: Obtaining Directory Change Notifications 
//Reminder: ReadDirectoryChangesW can be used to 
//get at the individual files that were modified
//however its kind of a hairy beast just to look
//at and scanning the directory isn't a big deal
static DWORD WINAPI som_files_threadproc(LPVOID hw)
{	
	bool db = SOM::game&&!SOM::retail; assert(!SOM::game||db);

	bool som_prm = SOM::tool==SOM_PRM.exe; //2024

	if(som_prm) db = true;

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

	UINT_PTR m = som_prm?0:*EX::user(0)?2:1;
	UINT_PTR n = m;
	if(SOM::tool==SOM_MAP.exe||db)
	for(UINT_PTR i=0;*EX::data(i);i++) 
	n++;	 

	std::vector<HANDLE> dwChangeHandles;
	std::vector<std::wstring> dwChangeFolders;

	for(UINT_PTR i=0;i<n;i++)
	{
		auto src = i<m?!i?EX::cd():EX::user(1):EX::data(i-m);
		auto fmt = i<m?L"%ls\\PARAM":db?L"%s":L"%ls\\map\\parts"; 

		//HACK: som_files_wrote_pr USES /p OR /P TO FIND ITS BERINGS!!
		//swprintf_s(dir,i%2?L"%ls/data/map/parts":L"%ls/PARAM",src);
		wchar_t dir[MAX_PATH]; swprintf_s(dir,fmt,src);

		HANDLE h;		
		if(INVALID_HANDLE_VALUE!=(h = 
		FindFirstChangeNotificationW(dir,SOM::game,FILE_NOTIFY_CHANGE_LAST_WRITE)))
		{
			dwChangeHandles.push_back(h);
			dwChangeFolders.push_back(dir); nChangeHandles++;
		}
	}

	auto *f = db?som_files_wrote_db2:som_files_wrote_pr;

	//if(db) 
	{
		SYSTEMTIME st;
		GetSystemTime(&st);
		SystemTimeToFileTime(&st,&som_files_wrote_init);
	}
	
	if(nChangeHandles) for(;;) //wait to be notified
	{ 		
		//read this might help somewhere
		DWORD hacked = INFINITE; //1000;

		DWORD dwWaitStatus = 
		WaitForMultipleObjects(nChangeHandles,dwChangeHandles.data(),0,hacked); 		
		if(dwWaitStatus<nChangeHandles) 
		{
			UINT_PTR id = dwWaitStatus;
			id = db&&id>=m?id-m:(UINT_PTR)dwChangeFolders[id].c_str();

			enum{ t=500 }; //1000 is too long for saving PRT files
			SetTimer((HWND)hw,id,t,f);
			FindNextChangeNotification(dwChangeHandles[dwWaitStatus]);		
		}
		else //if(hacked!=INFINITE)
		{
			while(nChangeHandles-->0)
			FindCloseChangeNotification(dwChangeHandles[nChangeHandles]);
			
			goto reset;
		}
	}	
	else assert(0); 
	
	assert(0); return 0;
}

void SOM::PARAM::kickoff_write_monitoring_thread(HWND hwnd)
{
	static DWORD one_off = 0; if(!one_off&&hwnd)
	CloseHandle(CreateThread(0,0,som_files_threadproc,hwnd,0,&one_off));	
}