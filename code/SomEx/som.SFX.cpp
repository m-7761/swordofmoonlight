	
#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

#include <map>

#include "Ex.output.h"

#include "SomEx.ini.h"

#include "som.game.h"
#include "som.state.h"
#include "som.extra.h"

#define SOMVECTOR_MATH
#include "../Somplayer/Somvector.h"
#include "../lib/swordofmoonlight.h"

namespace DDRAW
{
	extern DWORD refreshrate;
}

typedef SOM::SFX sfx;
typedef SOM::sfx_init_params init;

static_assert(sizeof(sfx)==4*126,"sfx size"); //remove me

static auto &sfx_ref = SOM::L.SFX_refs.operator&();
static auto &sfx_dat = SOM::L.SFX_dat_file.operator&();

//NOTE: zeroes mdl and sets fx->sfx to in->sfx
#define X42EEB0 !((BYTE(*)(sfx*,init*))0x42eeb0)(fx,in)
#define RETURN0 return(assert(0),0);

extern som_scene_picture *__cdecl som_MDL_42f7a0(int,int);

extern SOM::sfx_pro_rec *som_SFX_pro = 0;

extern char* *som_SFX_models = 0; //256;
extern char* *som_SFX_sounds = 0; //1024;

static WORD *som_SFX_SNDs_reverse = 0;
static std::map<std::string,size_t> som_SFX_SNDs;

enum{ som_SND=1 };

extern WORD som_SFX_SND_to_sound(WORD snd)
{
	if(snd>=1008) return snd; //HACK

	if(snd<1024&&som_SFX_SNDs_reverse)
	{
		return som_SFX_SNDs_reverse[snd];
	}

	return 0xFFFF;
}

WORD SOM::SND(WORD snd)
{
	if(!som_SND) return snd;

	if(snd>=1008) return snd;

	char a[8]; sprintf(a,"%04d",snd);

	return SOM::SND(a);
}
WORD SOM::SND(char *snd)
{
	if(!som_SND) return atoi(snd);

	assert(som_SFX_sounds);
	assert(!strchr(snd,'.'));
	auto ins = som_SFX_SNDs.insert(std::make_pair(snd,som_SFX_SNDs.size()));
	WORD s = min(1023,(WORD)ins.first->second);
	if(s!=ins.first->second)
	EX::is_needed_to_shutdown_immediately(-1,"more than 1024 sfx.dat sounds");
	som_SFX_sounds[s] = (char*)ins.first->first.c_str();	

	if(isdigit(*snd)) //HACK: extensions need the reverse mapping
	{
		int i = atoi(snd); if(i<1024) som_SFX_SNDs_reverse[s] = i;
	}

	return s;
}
WORD SOM::SND(const wchar_t *snd)
{
	int i = 0;
	char a[32];
	while(*snd&&i<sizeof(a)-1&&*snd!='.') 
	a[i++] = (char)*snd++;
	a[i] = '\0';
	return i<32?SND(a):0xFFFF;
}

struct som_SFX_less
{
	bool operator()(char *a, char *b)const
	{
		return strcmp(a,b)<0;
	}
};

extern void som_SFX_init_SFX_pro(SOM::sfx_dat_rec *sfx_dat)
{		
	std::map<char*,size_t,som_SFX_less> models;

	wchar_t w[MAX_PATH];
	swprintf(w,L"%s\\PARAM\\SFX.PRO",EX::cd());
	if(FILE*f=_wfopen(w,L"rb"))
	{
		som_SFX_pro = new SOM::sfx_pro_rec[1024]();
		fread(som_SFX_pro,64*1024,1,f);
		fclose(f);

		for(int i=0;i<1024;i++)
		if(sfx_dat[i].procedure!=255)
		{
			if(!*som_SFX_pro[i].model)			
			sprintf(som_SFX_pro[i].model,"%04d.mdl",sfx_dat[i].model);
			if(!*som_SFX_pro[i].sound)
			if(sfx_dat[i].snd&&sfx_dat[i].snd!=0xffff)
			sprintf(som_SFX_pro[i].sound,"%04d.snd",sfx_dat[i].snd);

			*PathFindExtensionA(som_SFX_pro[i].model) = '\0';
			*PathFindExtensionA(som_SFX_pro[i].sound) = '\0';

			if(*som_SFX_pro[i].model)
			{
				auto ins = 
				models.insert(std::make_pair(som_SFX_pro[i].model,models.size()));
				sfx_dat[i].model = (BYTE)ins.first->second;
				if(sfx_dat[i].model!=ins.first->second)
				EX::is_needed_to_shutdown_immediately(-1,"more than 255 sfx.dat models");
				som_SFX_models[sfx_dat[i].model] = ins.first->first;
			}
			if(*som_SFX_pro[i].sound)
			{
				sfx_dat[i].snd = SOM::SND(som_SFX_pro[i].sound);
			}
						
			if(0==((DWORD(*)(DWORD))0x42ed10)(i)) //subordinate?
			{
				WORD snd = (int)sfx_dat[i].height;
				sfx_dat[i].height = SOM::SND(snd); //42e65c
			}
		}
	}
}

void som_SFX_42e460_init_SFX_dat()
{
	som_SFX_SNDs_reverse = new WORD[1024]();

	BYTE ret = ((BYTE(*)())0x42e460)(); if(!ret) //HACK
	{
		//for some reason this is contingent on reading SFX.dat?
		//FUN_0044d4d0_bit_encode_scene_element_flags(&DAT_01ce1ce8_mdo_rop_0?,0,2,1,2,1,0,1,1);
		//FUN_0044d4d0_bit_encode_scene_element_flags(&DAT_01ce1cec_mdo_rop_1?,0,5,6,2,0,1,1,1);
		//FUN_0044d4d0_bit_encode_scene_element_flags(&DAT_01ce1cf0_mdo_rop_2?,0,5,2,2,0,1,1,0);
		((void(*)(int,int,int,int,int,int,int,int,int))0x44d4d0)(0x1ce1ce8,0,2,1,2,1,0,1,1);
		((void(*)(int,int,int,int,int,int,int,int,int))0x44d4d0)(0x1ce1cec,0,5,6,2,0,1,1,1);
		((void(*)(int,int,int,int,int,int,int,int,int))0x44d4d0)(0x1ce1cf0,0,5,2,2,0,1,1,0);
	}

	wchar_t w[MAX_PATH];
	swprintf(w,L"%s\\PARAM\\SFX.DAT",EX::cd());
	if(FILE*f=_wfopen(w,L"rb"))
	{
		fread(sfx_dat,48*1024,1,f);
		fclose(f);
	}
	else if(!ret)
	{
		memset(sfx_dat,0x00,sizeof(sfx_dat));
		for(int i=1024;i-->0;) 
		sfx_dat[i].procedure = 255;
	}

	//this messes up som_SFX_write_new_fx_prf below
	//som_SFX_init_SFX_pro(sfx_dat);

	EXLOG_LEVEL(3) << "SFX.DAT table...\n";

	//just scanning for each kind
	for(int i=0;i<1024;i++)
	{
		auto &d = sfx_dat[i];
		if(255==d.procedure) continue;
				
		if(0) //EXPERIMENTAL
		{
			extern void som_SFX_write_new_fx_prf(int,void*);
			som_SFX_write_new_fx_prf(i,&d);
		}

		EXLOG_LEVEL(3) <<
		"sfx:" << std::setw(4) << i << ' ' <<
		"p:" << std::setw(3) << (int)d.procedure << ' ' <<
		"m:" << std::setw(4) << (int)d.model << ' ' <<
		"b0-2: " << std::setw(4) << (int)d.unk2[0] << std::setw(4)  << (int)d.unk2[1] << std::setw(4)  << (int)d.unk2[2] << ' ' <<
		"b3-5: " << std::setw(4) << (int)d.unk2[3] << std::setw(4)  << (int)d.unk2[4] << std::setw(4)  << (int)d.unk2[5] << ' ' <<
			
		"w:" << std::setw(6) << d.width << ' ' <<
		"h:" << std::setw(6) << d.height << ' ' <<
		"r:" << std::setw(6) << d.radius << ' ' <<
		"s:" << std::setw(6) << d.speed << ' ' <<
		"x:" << std::setw(6) << d.scale << " X " << std::setw(6) << d.scale2 << ' ' <<
		"u:"<< std::setw(6) << d.unk5[0] << " X " << std::setw(6) << d.unk5[1] << ' ' <<			
		"snd:" << std::setw(4) << d.snd << '(' << (int)d.pitch << ')' << '\t' <<
		"chain:" << d.chainfx << '\n';
	}
	/*SFX.dat (unmodified)
	sfx:   6 p:131 m: 255 b0-2:    2   2   0 b3-5:    0   0   0 w:   255 h:   192 r:   255 s:   128 x:    16 X     32 u:     8 X   0.05 snd:   -1(0) chain:0
	sfx:   7 p:131 m: 255 b0-2:   15  15   0 b3-5:    0   0   0 w:   255 h:   255 r:     0 s:     0 x:   255 X      0 u:     0 X   0.05 snd:   -1(0) chain:0
	sfx:  10 p:131 m: 255 b0-2:   15  15   0 b3-5:    0   0   0 w:   255 h:     0 r:     0 s:   255 x:     0 X      0 u:   255 X 0.0666 snd:   -1(0) chain:0
	sfx:  11 p:132 m:   7 b0-2:   30   2   0 b3-5:    1  16   0 w:  0.25 h:   0.3 r:   128 s:     4 x:     4 X      0 u:     0 X      0 snd:   -1(0) chain:0
	sfx:  12 p:132 m:   7 b0-2:   30   2   0 b3-5:    1  16   0 w:   0.5 h:   0.8 r:   255 s:     4 x:     4 X      0 u:     0 X      0 snd:   -1(0) chain:0
	sfx:  13 p:132 m:   7 b0-2:   30   2   0 b3-5:    1  16   0 w:   0.2 h:   0.3 r:   255 s:     4 x:     4 X      0 u:     0 X      0 snd:   -1(0) chain:0
	sfx:  20 p:100 m:  10 b0-2:   45  15   2 b3-5:    1   0   0 w:   0.3 h:   0.3 r:   1.5 s:   1.5 x:    -1 X    0.2 u:   255 X      0 snd:   -1(0) chain:0
	sfx:  21 p:100 m: 181 b0-2:   45  15   2 b3-5:    1   0   0 w:   0.3 h:   0.3 r:   1.5 s:   1.5 x:    -1 X    0.2 u:   255 X      0 snd:   -1(0) chain:0
	sfx:  25 p: 21 m: 110 b0-2:   24  10   3 b3-5:    2 255  10 w: 65535 h: 65535 r:   0.5 s:   4.5 x:   0.1 X      2 u:     1 X      1 snd:   -1(0) chain:0
	sfx:  26 p: 12 m:  54 b0-2:   24  10   2 b3-5:    4   4   1 w: 65535 h: 65535 r:   0.5 s:     6 x:     1 X    2.5 u:   255 X   0.01 snd:   -1(0) chain:0
	sfx:  27 p:101 m:  54 b0-2:   45  15   2 b3-5:    2   0   0 w:     1 h:     2 r:     0 s:     0 x:     0 X      0 u:   255 X      0 snd:   -1(0) chain:0
	sfx:  48 p:  4 m:  48 b0-2:   24  10  64 b3-5:    0   0   0 w:   499 h:   882 r:   0.5 s:     5 x:   0.5 X   0.02 u:     0 X      0 snd:   -1(0) chain:0
	sfx:  50 p:132 m:  50 b0-2:   30   2   3 b3-5:    0   8   0 w:     1 h:     3 r:   255 s:     4 x:     2 X      0 u:     0 X      0 snd:   -1(0) chain:0
	sfx:  51 p:132 m:  50 b0-2:   30   2   3 b3-5:    0   8   0 w:     2 h:     3 r:   255 s:     4 x:     2 X      0 u:     0 X      0 snd:   -1(0) chain:0
	sfx:  52 p:132 m: 177 b0-2:    4   2   3 b3-5:    0   4   0 w:   0.5 h:   0.3 r:   128 s:     2 x:     2 X      0 u:     0 X      0 snd:   -1(0) chain:0
	sfx:  54 p: 21 m: 168 b0-2:   24  20   5 b3-5:   10   2 255 w:   509 h:   871 r:     1 s:     2 x:     1 X    1.5 u:     0 X      3 snd:   -1(0) chain:0
	sfx:  55 p:132 m:  50 b0-2:   30   2   3 b3-5:    0   8   0 w:     1 h:     6 r:   255 s:     4 x:     2 X      0 u:     0 X      0 snd:   -1(0) chain:0
	sfx:  56 p:132 m:  50 b0-2:   30   2   3 b3-5:    0   8   0 w:     2 h:     6 r:   255 s:     4 x:     2 X      0 u:     0 X      0 snd:   -1(0) chain:0
	sfx:  64 p:  5 m:  64 b0-2:   27  10  10 b3-5:    0   0   0 w: 65535 h:   870 r:   0.5 s:     7 x:   0.8 X      1 u:     0 X      0 snd:   -1(0) chain:0
	sfx:  66 p: 34 m: 140 b0-2:   24  10  30 b3-5:    0   0   3 w:   508 h:   871 r:     3 s:     8 x:     1 X      2 u:  0.25 X   0.25 snd:   -1(0) chain:0
	sfx:  67 p:  4 m: 199 b0-2:   27  10  64 b3-5:    0   0   0 w:   497 h:   871 r:     1 s:     6 x:  0.15 X  0.001 u:     0 X      0 snd:   -1(0) chain:0
	sfx:  69 p:128 m: 185 b0-2:   20  30   0 b3-5:    0   0   0 w:   0.5 h:     0 r:     1 s:     3 x:     0 X      1 u:     1 X      0 snd:   -1(0) chain:0
	sfx:  70 p:128 m: 198 b0-2:   20   1   0 b3-5:    0   0   0 w:     1 h:     1 r:     1 s:     1 x:     1 X      1 u:     1 X      1 snd:   -1(0) chain:0
	sfx:  71 p:  4 m: 196 b0-2:   20  10 128 b3-5:  128   0   0 w:   510 h:   879 r:   0.6 s:     7 x:     1 X      0 u:     0 X      0 snd:   -1(0) chain:0
	sfx:  73 p: 15 m: 199 b0-2:   27  10   1 b3-5:   90  70   0 w:   497 h:   871 r:   0.5 s:     5 x:  0.15 X      1 u:     0 X      0 snd:   -1(0) chain:0
	sfx:  74 p: 15 m: 199 b0-2:   27  10   1 b3-5:  100  90   0 w:   497 h:   871 r:   0.5 s:     5 x:  0.15 X      1 u:     0 X      0 snd:   -1(0) chain:0
	sfx:  75 p: 15 m: 199 b0-2:   27  10   1 b3-5:   90 110   0 w:   497 h:   871 r:   0.5 s:     5 x:  0.15 X      1 u:     0 X      0 snd:   -1(0) chain:0
	sfx:  80 p:  0 m:  80 b0-2:   30  10  64 b3-5:    0   0   0 w:   498 h:   875 r:   0.5 s:     8 x:   0.6 X 0.0001 u:     0 X      0 snd:   -1(0) chain:0
	sfx:  82 p:  0 m:  82 b0-2:   30  10 125 b3-5:    0   0   0 w:   500 h:   876 r:   0.5 s:     6 x:   0.6 X      0 u:     0 X      0 snd:   -1(0) chain:0
	sfx:  85 p: 30 m:  85 b0-2:   27  10   2 b3-5:  125   1   1 w: 65535 h:   880 r:  0.05 s:     5 x:   0.2 X    0.2 u:   255 X    0.1 snd:   -1(0) chain:0
	sfx:  86 p:  4 m:  86 b0-2:   27  10 110 b3-5:    0   0   0 w:   496 h:   878 r:   0.5 s:     8 x:   0.5 X      0 u:     0 X      0 snd:   -1(0) chain:0
	sfx:  91 p:  4 m:  91 b0-2:   24   0  90 b3-5:    0   0   0 w:   498 h:   873 r:  0.25 s:     8 x:     3 X      0 u:     0 X      0 snd:   -1(0) chain:0
	sfx:  93 p:  4 m:  93 b0-2:   36  10 120 b3-5:    0   0   0 w:   500 h:   874 r:   0.5 s:     6 x:   0.4 X   0.01 u:     0 X      0 snd:   -1(0) chain:0
	sfx:  94 p: 30 m:  94 b0-2:   27  30   2 b3-5:  128   1   1 w: 65535 h:   880 r:   0.2 s:     5 x:   0.4 X    0.4 u:   255 X   0.05 snd:   -1(0) chain:0
	sfx:  96 p: 30 m:  96 b0-2:   24  30   2 b3-5:  128   1   1 w: 65535 h:   880 r:  0.05 s:     5 x:   0.2 X    0.2 u:   255 X    0.1 snd:   -1(0) chain:0
	sfx: 100 p:  5 m:  57 b0-2:   30  10  10 b3-5:    0   0   0 w:    20 h:   873 r:   0.3 s:     6 x:  0.15 X    0.2 u:     0 X      0 snd:   -1(0) chain:0
	sfx: 136 p:  3 m: 161 b0-2:   24  10   2 b3-5:  110   0   0 w: 65535 h: 65535 r:   0.4 s:     8 x:   0.3 X    0.3 u:   255 X      0 snd:   -1(0) chain:0
	sfx: 137 p:  5 m: 165 b0-2:   30  10   3 b3-5:    0   0   0 w:   499 h:   883 r:   0.5 s:     4 x:   2.5 X      1 u:     0 X      0 snd:   -1(0) chain:0
	sfx: 138 p:  5 m: 174 b0-2:   24  10   5 b3-5:    0   0   0 w:   499 h:   882 r:  0.25 s:     8 x:     3 X      1 u:     0 X      0 snd:   -1(0) chain:0
	sfx: 139 p:  5 m: 175 b0-2:   24  10   5 b3-5:    0   0   0 w:    20 h:   883 r:  0.25 s:     6 x:     3 X      1 u:     0 X      0 snd:   -1(0) chain:0
	sfx: 140 p:  0 m:  82 b0-2:   20  10 125 b3-5:    0   0   0 w:   500 h:   877 r:   0.5 s:     6 x:     1 X      0 u:     0 X      0 snd:   -1(0) chain:0
	sfx: 141 p:  0 m:  64 b0-2:   27  10   3 b3-5:    8  32   0 w: 65535 h:   870 r:  0.25 s:     7 x:   0.8 X      0 u:     0 X      0 snd:   -1(0) chain:0
	sfx: 142 p:  0 m:  64 b0-2:   27  10   3 b3-5:  248 232   0 w: 65535 h:   870 r:  0.25 s:     7 x:   0.8 X      1 u:     0 X      0 snd:   -1(0) chain:0
	sfx: 143 p:  0 m:  64 b0-2:   27  10   3 b3-5:  248 248   0 w: 65535 h:   870 r:  0.25 s:     7 x:   0.8 X      1 u:     0 X      0 snd:   -1(0) chain:0
	sfx: 144 p:  0 m:  64 b0-2:   27  10   3 b3-5:    8  16   0 w: 65535 h:   870 r:  0.25 s:     7 x:   0.8 X      1 u:     0 X      0 snd:   -1(0) chain:0
	sfx: 145 p:  4 m:  86 b0-2:   27  10 110 b3-5:    0   0   0 w:   496 h:   878 r:   0.1 s:     8 x:   0.4 X      0 u:     0 X      0 snd:   -1(0) chain:0
	sfx: 146 p:  0 m:  80 b0-2:   30  10  64 b3-5:    0   0   0 w:   498 h:   875 r:   0.5 s:     8 x:     1 X 0.0001 u:     0 X      0 snd:   -1(0) chain:0
	sfx: 147 p:  5 m:  64 b0-2:   27  10   5 b3-5:    0   0   0 w: 65535 h:   870 r:   0.6 s:     7 x:   1.2 X      1 u:     0 X      0 snd:   -1(0) chain:0
	sfx: 148 p:  5 m:  86 b0-2:   24  10   1 b3-5:    0   0   0 w:   496 h:   878 r:   0.4 s:     7 x:     2 X      2 u:     0 X      0 snd:   -1(0) chain:0
	sfx: 149 p:  5 m:  80 b0-2:   24  10   1 b3-5:    0   0   0 w:   498 h:   875 r:   0.4 s:     7 x:     2 X      2 u:     0 X      0 snd:   -1(0) chain:0
	sfx: 150 p:128 m:  66 b0-2:   11   4   4 b3-5:    0   0   0 w:     0 h:     1 r:     1 s:     1 x:     1 X      1 u:     1 X      0 snd:   -1(0) chain:0
	sfx: 151 p:128 m: 188 b0-2:    4   5   5 b3-5:    0   0   0 w:     4 h:     1 r:     6 s:    10 x:     0 X      1 u:     0 X      0 snd:   -1(0) chain:0
	sfx: 152 p:  3 m:  85 b0-2:   30  10   2 b3-5:  125   8   0 w: 65535 h:   880 r:   0.1 s:     5 x:   0.3 X    0.3 u:   255 X      0 snd:   -1(0) chain:0
	sfx: 153 p:  4 m:  82 b0-2:   20  10 110 b3-5:    0   0   0 w:   500 h:   877 r:   0.5 s:     8 x:     1 X      0 u:     0 X      0 snd:   -1(0) chain:0
	sfx: 154 p:  3 m:  98 b0-2:   30  15   2 b3-5:  120   8   0 w: 65535 h:   880 r:   0.2 s:     5 x:   0.2 X    0.2 u:   255 X   0.05 snd:   -1(0) chain:0
	sfx: 156 p:  4 m:  88 b0-2:   27  10  64 b3-5:    0   0   0 w:   499 h:   882 r:   0.5 s:     6 x:   0.5 X      0 u:     0 X      0 snd:   -1(0) chain:0
	sfx: 157 p:  4 m:  86 b0-2:   24  10 110 b3-5:    0   0   0 w:   496 h:   878 r:     1 s:     8 x:  0.65 X      0 u:     0 X      0 snd:   -1(0) chain:0
	sfx: 158 p:128 m: 158 b0-2:   20  10   0 b3-5:    0   0   0 w:     1 h:     1 r:     8 s:    12 x:     1 X      1 u:     0 X      0 snd:   -1(0) chain:0
	sfx: 159 p:  4 m: 199 b0-2:   24  10  64 b3-5:  128   0   0 w:   497 h:   871 r:   0.6 s:     7 x:   0.2 X      0 u:     0 X      0 snd:   -1(0) chain:0
	sfx: 160 p:  3 m:  85 b0-2:   27  10   2 b3-5:  125   8   0 w: 65535 h:   880 r:  0.25 s:     5 x:   0.5 X    0.5 u:   255 X   0.02 snd:   -1(0) chain:0
	sfx: 161 p:  5 m: 166 b0-2:   24  10   2 b3-5:  128   0   0 w:   509 h:   883 r:     1 s:     5 x:   1.5 X      1 u:     0 X      0 snd:   -1(0) chain:0
	sfx: 162 p: 20 m:  82 b0-2:   24  25   5 b3-5:   24  28   1 w:   500 h:   875 r:     1 s:     7 x:    25 X      0 u:     1 X      0 snd:   -1(0) chain:0
	sfx: 400 p:128 m: 128 b0-2:   78  20   0 b3-5:    0   0   0 w:     3 h:     0 r:   1.5 s:   1.5 x:     0 X      1 u:     0 X      0 snd:   -1(0) chain:0
	sfx: 401 p:128 m:  67 b0-2:   20  10   0 b3-5:    0   0   0 w:     4 h:     0 r:   1.5 s:   1.5 x:     0 X      1 u:     0 X      0 snd:   -1(0) chain:0
	sfx: 410 p:131 m: 255 b0-2:    5   5   0 b3-5:    0   0   0 w:   255 h:   255 r:   255 s:   200 x:     0 X      0 u:     0 X 0.0666 snd:   -1(0) chain:0
	sfx: 449 p:128 m: 178 b0-2:    5  15   0 b3-5:    0   0   0 w:     2 h:   0.5 r:     1 s:     2 x:     0 X      1 u:     0 X      0 snd:   -1(0) chain:0
	sfx: 452 p:132 m: 179 b0-2:   30   2   0 b3-5:    0  16   0 w:   0.3 h:  0.45 r:   255 s:     4 x:     4 X      0 u:     0 X      0 snd:   -1(0) chain:0
	sfx: 480 p:129 m: 176 b0-2:   10   3   5 b3-5:    0   0   0 w:  0.15 h:     2 r:     2 s:     5 x:     0 X      1 u:     1 X      0 snd:   -1(0) chain:0
	sfx: 481 p:129 m: 176 b0-2:    4   2   2 b3-5:    0   0   0 w:     1 h:   0.5 r:   0.5 s: 0.001 x:     0 X      1 u:     1 X      0 snd:   -1(0) chain:0
	sfx: 482 p:129 m: 190 b0-2:   10   5   5 b3-5:    0   0   0 w:    10 h:     5 r:     5 s:   0.1 x:     0 X      1 u:     1 X      0 snd:   -1(0) chain:0
	sfx: 483 p:129 m: 189 b0-2:   20  10   5 b3-5:    0   0   0 w:    10 h:     6 r:     6 s:   0.1 x:     0 X      1 u:     1 X      0 snd:   -1(0) chain:0
	sfx: 484 p:129 m: 186 b0-2:   15   7   8 b3-5:    0   0   0 w:     7 h:     3 r:     3 s:   0.1 x:     0 X      1 u:     1 X      0 snd:   -1(0) chain:0
	sfx: 485 p: 20 m:  80 b0-2:   34   5   5 b3-5:    1   9   0 w:     0 h:     3 r:   0.5 s:     4 x:    10 X      0 u:     1 X      0 snd:   -1(0) chain:0
	sfx: 486 p:129 m: 191 b0-2:   10   3   7 b3-5:    0   0   0 w:   0.2 h:     1 r:     1 s:     3 x:     0 X      1 u:     1 X      0 snd:   -1(0) chain:0
	sfx: 487 p:129 m: 189 b0-2:   10   7   3 b3-5:    0   0   0 w:     3 h:   1.5 r:   1.5 s: 0.001 x:     0 X      1 u:     1 X      0 snd:   -1(0) chain:0
	sfx: 488 p:129 m: 190 b0-2:   10   7   3 b3-5:    0   0   0 w:     5 h:     3 r:     3 s:   0.1 x:     0 X      1 u:     1 X      0 snd:   -1(0) chain:0
	sfx: 489 p:129 m: 191 b0-2:   10   3   7 b3-5:    0   0   0 w:   0.2 h:     1 r:     1 s:     5 x:     0 X      1 u:     1 X      0 snd:   -1(0) chain:0
	sfx: 490 p:129 m: 194 b0-2:   10   5   3 b3-5:    0   0   0 w:     5 h:     2 r:     2 s:   0.1 x:     0 X      1 u:     1 X      0 snd:   -1(0) chain:0
	sfx: 494 p:132 m: 192 b0-2:    8   2   2 b3-5:    0   8   0 w:  0.75 h:  0.75 r:   255 s:     4 x:     2 X    0.2 u:   255 X      0 snd:   -1(0) chain:0
	sfx: 495 p:  3 m:  96 b0-2:   24  10   2 b3-5:  125   8   0 w: 65535 h: 65535 r:  0.15 s:     5 x:   0.3 X    0.3 u:   255 X  0.005 snd:   -1(0) chain:0
	sfx: 496 p:100 m: 197 b0-2:   45  15   2 b3-5:    1   0   0 w:   0.3 h:   0.3 r:   1.5 s:   1.5 x:    -1 X    0.2 u:   255 X      0 snd:   -1(0) chain:0
	sfx: 497 p:100 m:  10 b0-2:   45  15   2 b3-5:    1   0   0 w:   0.2 h:   0.2 r:     1 s:     1 x:    -1 X    0.2 u:   255 X      0 snd:   -1(0) chain:0
	sfx: 498 p:100 m:  89 b0-2:   45  15   2 b3-5:    1   0   0 w:   0.3 h:   0.3 r:     1 s:     1 x:    -1 X    0.2 u:   255 X      0 snd:   -1(0) chain:0
	sfx: 499 p:100 m:  99 b0-2:   45  15   2 b3-5:    1   0   0 w:   0.2 h:   0.2 r:     2 s:     2 x:    -1 X    1.5 u:   255 X      0 snd:   -1(0) chain:0
	sfx: 500 p:100 m:  97 b0-2:   45  15   2 b3-5:    1   0   0 w:   0.3 h:   0.3 r:     1 s:     1 x:    -1 X      1 u:   255 X      0 snd:   -1(0) chain:0
	sfx: 501 p:  0 m: 187 b0-2:   24   1   0 b3-5:    0   0   0 w: 65535 h:     1 r:   0.1 s:     8 x:     1 X      0 u:     0 X      0 snd:   -1(0) chain:0
	sfx: 502 p:101 m: 173 b0-2:   10  10   2 b3-5:    0   0   0 w:     0 h:   1.5 r:     0 s:     0 x:     0 X      0 u:   255 X      0 snd:   -1(0) chain:0
	sfx: 503 p:129 m: 173 b0-2:   10   2   5 b3-5:  255   0   0 w:   0.1 h:   1.5 r:   1.5 s:     2 x:     0 X      1 u:     1 X      0 snd:   -1(0) chain:0
	sfx: 504 p:132 m: 107 b0-2:    8   2   2 b3-5:    0   4   0 w:  0.75 h:  0.75 r:   255 s:     2 x:     2 X    0.2 u:   255 X      0 snd:   -1(0) chain:0
	sfx: 505 p:132 m: 112 b0-2:    8   2   2 b3-5:    0   8   0 w:   1.2 h:   1.2 r:   255 s:     4 x:     2 X    0.2 u:   255 X      0 snd:   -1(0) chain:0
	sfx: 506 p:134 m: 114 b0-2:   10   5   5 b3-5:    6   1   2 w:   2.5 h:   2.5 r:     2 s:     1 x:     1 X      0 u:     1 X      1 snd:   -1(0) chain:0
	sfx: 507 p:134 m: 114 b0-2:   10   5   5 b3-5:    6   1   2 w:   2.5 h:   2.5 r:   0.1 s:     1 x:     1 X      0 u:     1 X      1 snd:   -1(0) chain:0
	sfx: 508 p:101 m: 169 b0-2:   30  15   2 b3-5:    0   0   0 w:     1 h:     5 r:     0 s:     0 x:     0 X      0 u:   255 X      0 snd:   -1(0) chain:0
	sfx: 509 p:101 m: 167 b0-2:   30   5   2 b3-5:    2   0   0 w:   0.2 h:     2 r:     0 s:     0 x:     0 X      0 u:   255 X      0 snd:   -1(0) chain:0
	sfx: 510 p:100 m: 183 b0-2:   20  10   2 b3-5:    0   0   0 w:   0.5 h:   0.5 r:     1 s:     1 x:   120 X   0.05 u:   255 X      0 snd:   -1(0) chain:0
	sfx: 511 p:132 m: 162 b0-2:   16   2   2 b3-5:    0   8   0 w:     1 h:     1 r:   255 s:     4 x:     2 X      0 u:     0 X      0 snd:   -1(0) chain:0
	sfx: 800 p:  0 m: 151 b0-2:   24  15   0 b3-5:    0   0   0 w:   850 h:   873 r:  0.25 s:     7 x:   0.1 X      0 u:     0 X      0 snd: 820(24) chain:0
	sfx: 801 p: 20 m: 206 b0-2:   26  10  10 b3-5:    0  19   1 w:   857 h: 65535 r:   0.2 s:     7 x:    15 X      1 u:     1 X      0 snd: 167(24) chain:0
	sfx: 803 p:  4 m: 200 b0-2:   27  15  64 b3-5:   64   0   0 w:   851 h:   882 r:   0.5 s:     9 x:  0.35 X  0.001 u:     0 X      0 snd: 182(27) chain:0
	sfx: 804 p:  4 m: 154 b0-2:   27  15  64 b3-5:   64   0   0 w:   852 h:   871 r:  0.35 s:     8 x:   0.3 X  0.001 u:     0 X      0 snd: 171(27) chain:0
	sfx: 805 p:  9 m: 146 b0-2:   24  15   5 b3-5:  255   0   0 w:   853 h:   883 r:   0.5 s:     7 x:     3 X      1 u:     0 X      0 snd: 184(24) chain:0
	sfx: 806 p:  9 m: 147 b0-2:   24  15   5 b3-5:  255   0   0 w:   855 h:   882 r:   0.5 s:     7 x:     3 X      1 u:     0 X      0 snd: 437(24) chain:0
	sfx: 808 p:  9 m: 150 b0-2:   30  10   5 b3-5:    0   0   0 w:   861 h:   870 r:   0.5 s:     7 x:   0.8 X      1 u:     0 X      0 snd: 802(30) chain:0
	sfx: 809 p:  9 m: 150 b0-2:   24  10   5 b3-5:    0   0   0 w:   802 h:   870 r:   0.5 s:     7 x:     2 X      1 u:     0 X      0 snd: 802(24) chain:0
	sfx: 810 p: 30 m: 163 b0-2:   24   0   2 b3-5:  120   1   1 w: 65535 h:   880 r:   0.3 s:     7 x:     1 X      1 u:   255 X  0.025 snd: 162(24) chain:0
	sfx: 811 p: 22 m: 110 b0-2:    0   0   0 b3-5:    0   0   0 w:   497 h:     1 r:     0 s:     0 x:     0 X      0 u:     0 X      0 snd:   -1(0) chain:0
	sfx: 812 p: 31 m: 149 b0-2:   24  25   5 b3-5:    5   0   5 w:   857 h: 65535 r:     0 s:   1.5 x:     0 X      5 u:     4 X      1 snd: 806(24) chain:0
	sfx: 813 p: 32 m: 110 b0-2:   24  25   5 b3-5:    5   0   0 w:   497 h:     1 r:     0 s:     1 x:     0 X      5 u:   0.1 X    0.1 snd:   -1(0) chain:0
	sfx: 814 p: 34 m: 141 b0-2:   24  10  25 b3-5:    0   0   3 w:   854 h:   871 r:   1.5 s:     8 x:   0.6 X      2 u:   0.1 X   0.25 snd: 811(24) chain:0
	sfx: 815 p: 34 m: 148 b0-2:   24  10  15 b3-5:    0   0   0 w:   858 h:   882 r:     5 s:     8 x:     1 X      2 u:   0.5 X    0.8 snd: 182(24) chain:0
	sfx: 816 p: 21 m: 205 b0-2:   24  20   5 b3-5:   10   2 255 w:   853 h:   871 r:     1 s:     2 x:     1 X    1.5 u:     0 X      3 snd: 812(24) chain:0
	sfx: 817 p: 32 m: 220 b0-2:   24  40  10 b3-5:   10   0  30 w:   859 h: 65535 r:     1 s:     1 x:     0 X    7.5 u:     0 X      0 snd: 860(24) chain:0
	sfx: 818 p:  4 m: 184 b0-2:   24  15 120 b3-5:   64   0   0 w:   860 h:   879 r:   0.5 s:     7 x:     1 X      0 u:     0 X      0 snd: 823(24) chain:0
	sfx: 819 p: 34 m: 159 b0-2:   24  10  15 b3-5:    0   0   0 w:   858 h:   870 r:     5 s:     8 x:     1 X      2 u:   0.5 X   0.75 snd: 800(24) chain:0
	sfx: 820 p: 40 m: 105 b0-2:   24  10   2 b3-5:    0   2   2 w: 65535 h: 65535 r:    10 s:     0 x:     0 X      0 u:     1 X      6 snd: 194(24) chain:1
	sfx: 821 p: 41 m: 104 b0-2:   24   0   0 b3-5:    0   0   0 w: 65535 h: 65535 r:     0 s:     0 x:     0 X      0 u:   1.2 X      1 snd:   -1(0) chain:1
	sfx: 822 p: 42 m: 102 b0-2:   24  45  35 b3-5:   15  30  10 w:     0 h: 65535 r:     0 s:     7 x:    10 X      0 u:     1 X      0 snd:   -1(0) chain:1
	sfx: 823 p: 43 m: 103 b0-2:   24  45  35 b3-5:   15   0   0 w: 65535 h: 65535 r:     0 s:    10 x:    15 X      0 u:     1 X      0 snd:   -1(0) chain:0
	sfx: 827 p: 45 m: 153 b0-2:   24  15   0 b3-5:    1   0   0 w: 65535 h: 65535 r:   0.6 s:    10 x:     1 X      0 u:     0 X      0 snd: 802(24) chain:1
	sfx: 828 p: 22 m: 144 b0-2:   24   3  17 b3-5:    0  15   1 w:   865 h: 65535 r:  0.25 s:     5 x:    10 X      0 u:     1 X      0 snd: 885(24) chain:0
	sfx: 829 p: 46 m: 151 b0-2:   30  10   5 b3-5:    0   0   0 w: 65535 h: 65535 r:  0.25 s:     7 x:   0.6 X   0.35 u:     0 X      0 snd: 820(20) chain:1
	sfx: 830 p: 22 m:  82 b0-2:   30  10   5 b3-5:    0  12   0 w: 65535 h: 65535 r:  0.25 s:    14 x:     1 X      1 u:     0 X      0 snd: 884(24) chain:0
	sfx: 831 p: 45 m: 158 b0-2:   24  15   0 b3-5:    1   0   0 w: 65535 h: 65535 r:   0.5 s:     8 x:   0.3 X      0 u:     0 X      0 snd: 175(24) chain:1
	sfx: 832 p: 22 m: 155 b0-2:   24   3  17 b3-5:    0  15   1 w:   866 h: 65535 r:  0.25 s:     5 x:    10 X      0 u:     1 X      0 snd: 885(24) chain:0
	sfx: 833 p: 45 m: 200 b0-2:   24  15   0 b3-5:    1   0   0 w: 65535 h: 65535 r:   0.5 s:     9 x:     1 X      0 u:     0 X      0 snd: 182(24) chain:1
	sfx: 834 p: 22 m: 145 b0-2:   24   2  15 b3-5:    0  15   1 w:   851 h: 65535 r:     0 s:     1 x:     7 X      0 u:     1 X      0 snd: 882(20) chain:0
	sfx: 835 p: 45 m: 152 b0-2:   24  15   1 b3-5:    1   0   0 w: 65535 h: 65535 r:   0.5 s:     9 x:     1 X      0 u:     0 X      0 snd: 170(24) chain:1
	sfx: 836 p: 22 m: 152 b0-2:   24   2  15 b3-5:    3  15   1 w:   865 h: 65535 r:     0 s:     5 x:    12 X      0 u:     1 X      0 snd: 875(24) chain:0
	sfx: 837 p: 45 m: 157 b0-2:   24  15   0 b3-5:    1   0   0 w: 65535 h: 65535 r:  0.75 s:    10 x:     1 X      0 u:     0 X      0 snd: 170(20) chain:1
	sfx: 838 p: 22 m: 156 b0-2:   24   3  17 b3-5:    0  15   1 w:   863 h: 65535 r:   0.2 s:     4 x:    10 X      0 u:     1 X      0 snd: 885(24) chain:0
	sfx: 839 p: 45 m: 150 b0-2:   24  10   0 b3-5:    1   0   0 w: 65535 h: 65535 r:   0.5 s:     9 x:   2.5 X      0 u:     0 X      0 snd: 802(18) chain:1
	sfx: 840 p: 22 m: 142 b0-2:   24   2  15 b3-5:    2  15   1 w: 65535 h: 65535 r:   0.2 s:   2.5 x:     5 X      0 u:     1 X      0 snd: 870(20) chain:0
	sfx: 842 p: 14 m: 151 b0-2:    0   5  10 b3-5:   45  45   0 w:   850 h: 65535 r:  0.25 s:     7 x:     1 X      1 u:     0 X      0 snd:   -1(0) chain:0
	sfx: 850 p:100 m: 171 b0-2:   20   8   2 b3-5:    0  16   0 w:  0.05 h:  0.05 r:   1.5 s:   1.5 x:   -10 X      2 u:   255 X      0 snd:   -1(0) chain:0
	sfx: 851 p:100 m: 204 b0-2:   15   8   2 b3-5:    0   0   0 w:   0.1 h:   0.1 r:     2 s:     2 x:    -1 X    0.5 u:   255 X      0 snd:   -1(0) chain:0
	sfx: 852 p:100 m: 113 b0-2:   45  15   2 b3-5:    1   0   0 w:   0.2 h:   0.2 r:     2 s:     2 x:    -1 X    1.5 u:   255 X      0 snd:   -1(0) chain:0
	sfx: 853 p:101 m: 202 b0-2:   30   5   2 b3-5:    2   0   0 w:   0.1 h:     2 r:   0.2 s:     1 x:     0 X      0 u:   255 X      0 snd:   -1(0) chain:0
	sfx: 854 p:101 m: 201 b0-2:   30  15   2 b3-5:    0   0   0 w:     1 h:     5 r:     0 s:     0 x:     0 X      0 u:   255 X      0 snd:   -1(0) chain:0
	sfx: 855 p:101 m: 173 b0-2:   30  10   2 b3-5:    2   0   0 w:   0.1 h:     3 r:     0 s:     0 x:     0 X      0 u:   255 X      0 snd:   -1(0) chain:0
	sfx: 856 p:101 m: 173 b0-2:   30  10   2 b3-5:    2   0   0 w:   0.1 h:     3 r:     0 s:     0 x:     0 X      0 u:   255 X      0 snd:   -1(0) chain:0
	sfx: 857 p:101 m: 103 b0-2:   30  20   2 b3-5:    2   0   0 w:     1 h:     5 r:     0 s:     0 x:     0 X      0 u:   255 X      0 snd:   -1(0) chain:0
	sfx: 858 p:101 m:  48 b0-2:   40  15   2 b3-5:    0   1   0 w:   0.1 h:     3 r:     0 s:     0 x:     0 X      0 u:   255 X      0 snd:   -1(0) chain:0
	sfx: 859 p:101 m: 221 b0-2:   20  10   2 b3-5:    2   0   0 w:   0.1 h:     3 r:     0 s:     0 x:     0 X      0 u:   255 X      0 snd:   -1(0) chain:0
	sfx: 860 p:100 m: 182 b0-2:   20  10   2 b3-5:    0   0   0 w:   0.5 h:   0.5 r:     1 s:     1 x:   120 X   0.05 u:   255 X      0 snd:   -1(0) chain:0
	sfx: 861 p:100 m: 172 b0-2:   20   8   2 b3-5:    0  16   0 w:  0.05 h:  0.05 r:   1.5 s:   1.5 x:   -10 X      2 u:   255 X      0 snd:   -1(0) chain:0
	sfx: 862 p:101 m: 155 b0-2:   24   5  10 b3-5:    2   5   0 w:   857 h: 65535 r:   0.1 s:   2.5 x:     5 X      0 u:     1 X      0 snd:   -1(0) chain:0
	sfx: 863 p:100 m: 113 b0-2:   45  15   2 b3-5:    1   0   0 w:   0.2 h:   0.2 r:     4 s:     4 x:    -1 X    1.5 u:   255 X      0 snd:   -1(0) chain:0
	sfx: 864 p:100 m: 204 b0-2:   15   8   2 b3-5:    0   0   0 w:   0.1 h:   0.1 r:     5 s:     5 x:    -1 X    0.5 u:   255 X      0 snd:   -1(0) chain:0
	sfx: 865 p:100 m: 143 b0-2:   15   8   2 b3-5:    0   0   0 w:   0.1 h:   0.1 r:     4 s:     4 x:    -1 X    0.5 u:   255 X      0 snd:   -1(0) chain:0
	sfx: 866 p:101 m: 201 b0-2:   30  15   2 b3-5:    0   0   0 w:     1 h:     4 r:     0 s:     0 x:     0 X      0 u:   200 X      0 snd:   -1(0) chain:0
	sfx:1021 p: 22 m: 110 b0-2:    0   0   0 b3-5:    0   0   0 w:   497 h:     1 r:     0 s:     0 x:     0 X      0 u:     0 X      0 snd:   -1(0) chain:0
	sfx:1022 p: 21 m:   0 b0-2:   24  10   5 b3-5:    1 200   0 w: 65535 h: 65535 r:  0.75 s:     1 x:   1.5 X      0 u:   1.5 X      2 snd:   -1(0) chain:0
	sfx:1023 p: 20 m:  82 b0-2:   24  25   5 b3-5:   24  25   0 w:   497 h:     1 r:     1 s:    10 x:    10 X      0 u:     1 X      0 snd:   -1(0) chain:0
	*/

	assert(!som_SFX_models);

	som_SFX_models = new char*[256]();
	som_SFX_sounds = new char*[1024]();

	for(int i=16;i-->0;) //PIGGYBACKING 	
	SOM::L.sys_dat_16_sound_effects[i] = SOM::SND(SOM::L.sys_dat_16_sound_effects[i]);

	som_SFX_init_SFX_pro(sfx_dat);
}

extern void __cdecl som_SFX_42f1f0(sfx *fx, float xyz[3], float look_vec[3], DWORD tt)
{
	//this is chaining a secondary explosion sfx if one is set up... the
	//rest of this code is extension stuff to let windcutter not explode
	((void(__cdecl*)(sfx*,float*,float*,DWORD))0x42f1f0)(fx,xyz,look_vec,tt);

	//EXTENSION
	assert(tt==fx->dmg_src.target_type); if(tt==2||tt==4) //enemy/NPC
	{
		auto &dat = sfx_dat[fx->sfx]; //WINDCUTTER

		//TODO??? interpret a silent/recursive SFX this way???
				
		if(-1==dat.width||65535==dat.width)
		if(-1==dat.height||65535==dat.height) //meaningful to procedure 5?
		{
			//HACK: I don't know if any orginal SFX entries use -1,-1
			//for their explosions... (I've seen some use 65535,65535)
			//but I chose this to represent a windcutter behavior until
			//I can figure out a better system
			return;
		}
	}
	fx->duration = 1; //HACK: the caller does this when it's not knocked out
}
static void som_SFX_standard_hit_test(sfx *fx)
{
	memcpy(fx->dmg_src.attack_origin,fx->xyz,3*sizeof(float));

	//FUN_0040b600_ball_hit_test_using_computed_radius		
	if(((BYTE(*)(float[3],float,int,void*,FLOAT*))0x40b600)
	(fx->dmg_src.attack_origin,
	 fx->dmg_src.radius*fx->scale,31, //31?
	 fx->dmg_src.source,SOM::L.hit_buf_pos)) //hit_buffer
	{
		(void*&)fx->dmg_src.target = SOM::L.target;

		int tt = SOM::L.target_type; //1 2 4 8 16			
		tt = tt==16?0:tt;
		fx->dmg_src.target_type = tt;

		((void(*)(void*))0x4041d0)(&fx->dmg_src); //add_damage_source
		som_SFX_42f1f0(fx,SOM::L.hit_buf_pos,SOM::L.hit_buf_look_vec,tt);

		//fx->duration = 1; //HACK: som_SFX_42f1f0 manages this
	}
}

static BYTE som_SFX_basic_init(int type, sfx *fx, init *in)
{
	auto &dat = sfx_dat[in->sfx];

	if(type!=sfx_ref[dat.model].type||X42EEB0) RETURN0
	
	memcpy(fx->xyz,in->xyz,3*sizeof(float));

	fx->dmg_src = in->dmg_src;

	fx->dmg_src.unknown_sfx = 3;
	fx->dmg_src.radius = dat.radius;

	for(int i=3;i-->0;)
	fx->const_vel[i] = dat.speed*SOM::L.rate*in->look_vec[i];

	//fx->duration = (int)(in->duration/dat.speed*30);
	fx->duration = (int)(in->duration/dat.speed*DDRAW::refreshrate);

	return 1;	
}

static BYTE __cdecl som_SFX_0a_42ed50_needle(sfx *fx, init *in)
{
	if(0||SOM::emu)
	{
		((BYTE(*)(sfx*,init*))0x42ed50)(fx,in);
		return 1; //breakpoint
	}

	auto &dat = sfx_dat[in->sfx];

	if(!som_SFX_basic_init(1,fx,in)) return 0;

	//NOTICE! this is assigned to the mdl's pitch!!
	fx->incline = -in->look_vec[1];
	fx->yaw = atan2f(in->look_vec[2],in->look_vec[0])+M_PI_2;
	//fx->roll = 0.0f;

	//NOTICE! passing "incline" in as "pitch" here?!
	memcpy(fx->mdl->xyzuvw,fx->xyz,6*sizeof(float));
	fx->mdl->fade = 1.0f;
	fx->mdl->scale[0] =	
	fx->mdl->scale[1] =
	fx->mdl->scale[2] = dat.scale*in->scale;
	fx->mdl->d = 1; //set_animation_goal_frame (HACK)	

	return 1;
}
static void __cdecl som_SFX_0b_42efe0_needle(sfx *fx)
{
	if(0||SOM::emu) 
	return ((void(*)(sfx*))0x42efe0)(fx); //breakpoint

	auto &dat = sfx_dat[fx->sfx];

	if(!fx->duration) //terminate?
	{		
		return ((void(*)(sfx*))0x42f190)(fx); 
	}
	else if(fx->duration<dat.unk2[1])
	{
		fx->mdl->fade = fx->duration*SOM::L.rate2;
	}
	else som_SFX_standard_hit_test(fx);
	
	for(int i=3;i-->0;) fx->xyz[i]+=fx->const_vel[i];

	memcpy(fx->mdl->xyzuvw,fx->xyz,6*sizeof(float));
	
	if(!fx->mdl->advance()) fx->mdl->d = 1;

	fx->duration--;
}

static BYTE __cdecl som_SFX_1a_42f2e0_spinner(sfx *fx, init *in)
{
	return som_SFX_0a_42ed50_needle(fx,in);
}
static void __cdecl som_SFX_1b_42f300_spinner(sfx *fx) //UNTESTED //UNUSED
{
	assert(0); //what uses this?? //nothing :(

	som_SFX_0b_42efe0_needle(fx);

	//FUN_00401070_get_XYZ_thiscall?_4c2358_view_matrix_only?
	void *vmo = (void*)0x4c2358; float x,y,z;
	((BYTE(__thiscall*)(void*,float*,float*,float*))0x401070)(vmo,&x,&y,&z);
	x-=fx->xyz[0]; 
	y-=fx->xyz[1];
	z-=fx->xyz[2];		
	fx->mdl->xyzuvw[3] = -atan2f(y,sqrtf(z*z+x*x));
	fx->mdl->xyzuvw[4] = +atan2f(z,x)+M_PI_2;
}

static BYTE __cdecl som_SFX_2a_42f380_picture(sfx *fx, init *in)
{
	if(0||SOM::emu)	
	return ((BYTE(*)(sfx*,init*))0x42f380)(fx,in); 

	if(!som_SFX_basic_init(2,fx,in)) return 0;

	return 1;
}
static void __cdecl som_SFX_2b_42f450_picture(sfx *fx)
{
	if(0||SOM::emu)
	{
		extern som_scene_picture *som_MDL_42f7a0_se;
		((void(*)(sfx*))0x42f450)(fx); 
		som_scene_picture *se = som_MDL_42f7a0_se;
		se = se;
		se = se;
		return; //breakpoint
	}

	auto &dat = sfx_dat[fx->sfx];

	if(!fx->duration) //terminate?			
	return ((void(*)(sfx*))0x42f190)(fx);

	//0042f48e d9 47 20	FLD	dword ptr [EDI + 0x20]=>DAT_01c91d50
	//uVar6 = __ftol();
	DWORD color = (DWORD)dat.unk5[0];
	if(fx->duration<dat.unk2[1])
	{
		color = (fx->duration*color)/0xf;
	}
	else som_SFX_standard_hit_test(fx);
	
	for(int i=3;i-->0;) fx->xyz[i]+=fx->const_vel[i];

	//FUN_00401070_get_XYZ_thiscall?_4c2358_view_matrix_only?
	void *vmo = (void*)0x4c2358; float x,y,z;
	((BYTE(__thiscall*)(void*,float*,float*,float*))0x401070)(vmo,&x,&y,&z);
	x-=fx->xyz[0];
	y-=fx->xyz[1]; 
	z-=fx->xyz[2];
	fx->pitch = -atan2f(y,sqrt(z*z+x*x));	
	fx->yaw = atan2f(z,x)+M_PI_2;
	//fx->roll = 0.0f; //som_SFX_3b_42f920 sets this to fx->randomizer

	//se = (uint*)FUN_0042f7a0_block_alloc_image_based_sfx
	auto se = som_MDL_42f7a0(1,dat.unk2[2]);
	if(!se) return;
	
	float s1 = dat.scale*fx->scale*0.5;
	float s2 = dat.scale2*fx->scale*0.5;		

	se->vdata[0].x = -s1;		
	se->vdata[1].x = s1;
	se->vdata[2].x = s1;
	se->vdata[3].x = -s1;
	se->vdata[0].y = s2;
	se->vdata[1].y = s2;
	se->vdata[2].y = -s2;
	se->vdata[3].y = -s2;
	se->vdata[0].s = 0.0;		
	se->vdata[1].s = 1.0;		
	se->vdata[2].s = 1.0;
	se->vdata[3].s = 0.0;
	se->vdata[0].t = 0.0;		
	se->vdata[1].t = 0.0;
	se->vdata[2].t = 1.0;
	se->vdata[3].t = 1.0;

	DWORD diffuse; if(dat.unk2[2]==2) //rgb blend?
	{
		diffuse = ((color|0xffffff00)<<8|color)<<8|color;
	}
	else diffuse = color<<24|0xffffff; //alpha

	auto *p = se->vdata;
	for(int i=4;i-->0;p++)
	{
		p->z = 0; p->diffuse = diffuse;
	}		

	se->texture = fx->txr;

	float m[4][4];
	Somvector::map(m).rotation<3,4,'xyz'>(fx->uvw);
	Somvector::map(m[3]).copy<3>(fx->xyz).se<3>() = 1;		

	p = se->vdata;
	for(int i=4;i-->0;p++)
	{
		auto *v = &p->x; //YUCK
		Somvector::map<3>(v).premultiply<4>(m);
	}
	
	fx->duration--;
}

static BYTE __cdecl som_SFX_3a_42f810_waterbullet(sfx *fx, init *in)
{
	auto &dat = sfx_dat[in->sfx];

	if(!som_SFX_basic_init(2,fx,in)) return 0;
	
	int cVar4 = dat.unk2[3]==0xff?SOM::rng():dat.unk2[3]+129;

	float fps = 30.0f/DDRAW::refreshrate; //EXTENSION

	fx->randomizer = (char)(BYTE)cVar4*M_PI/180*fps; //0.01745329

	return 1;	
}
static void __cdecl som_SFX_3b_42f920_waterbullet(sfx *fx)
{
	fx->roll+=fx->randomizer;

	som_SFX_2b_42f450_picture(fx);
}

static BYTE __cdecl som_SFX_4a_42feb0_fireball(sfx *fx, init *in)
{
	auto &dat = sfx_dat[in->sfx];

	if(!som_SFX_0a_42ed50_needle(fx,in)) return 0;
	
	int cVar4 = dat.unk2[3]==0xff?SOM::rng():dat.unk2[3]+129;

	float fps = 30.0f/DDRAW::refreshrate; //EXTENSION

	fx->randomizer = (char)(BYTE)cVar4*M_PI/180*fps; //0.01745329

	fx->explode_sz[0] = in->scale*dat.scale;
	fx->explode_sz[1] =	in->scale*dat.scale2;

	return 1;	
}
static void __cdecl som_SFX_4b_42feb0_fireball(sfx *fx)
{
	auto &dat = sfx_dat[fx->sfx];

	fx->roll+=fx->randomizer;

	som_SFX_0b_42efe0_needle(fx);	
}

static BYTE __cdecl som_SFX_5a_430090_windcutter(sfx *fx, init *in)
{
	auto &dat = sfx_dat[in->sfx];

	if(!som_SFX_0a_42ed50_needle(fx,in)) return 0;

	fx->randomizer = dat.speed*SOM::L.rate;

	//why zero out some things?
	fx->offset[0] = 0.0f;
	fx->offset[1] = 0.0f;
	fx->offset[2] = 0.0f;
	fx->uc[0x1a8] = dat.unk2[2];	
	fx->uc[0x1a9] = 0;

	return 1;
}
static void __cdecl som_SFX_5b_430220_windcutter(sfx *fx)
{
	if(0||SOM::emu) 
	return ((void(*)(sfx*))0x430220)(fx); //breakpoint

	auto &dat = sfx_dat[fx->sfx];

	if(!fx->duration) //terminate?
	{		
		return ((void(*)(sfx*))0x42f190)(fx);
	}
	else if(fx->duration<dat.unk2[1])
	{
		fx->mdl->fade = fx->duration*SOM::L.rate2;
	}
	else som_SFX_standard_hit_test(fx);
	
	float x = SOM::xyz[0]-fx->mdl->xyzuvw[0];
	float y = (SOM::xyz[1]+0.9)-fx->mdl->xyzuvw[1];
	float z = SOM::xyz[2]-fx->mdl->xyzuvw[2];

	float len = sqrtf(x*x+y*y+z*z);

	float nx = x/len-fx->offset[0]; //0
	float ny = y/len-fx->offset[1]; //0
	float nz = z/len-fx->offset[2]; //0
	
	x = y = z = dat.scale2*M_PI/180; 
		
	if(nx<=z)
	{
		x = nx; if(nx<-z) x = -z;
	}	
	if(ny<=z)
	{
		y = ny; if(ny<-z) y = -z;
	}	
	if(nz<=z)
	{
		ny = -z; z = nz; if(nz<ny) z = ny;
	}

	x+=fx->offset[0]; //0
	y+=fx->offset[1]; //0
	z+=fx->offset[2]; //0
	len = sqrtf(x*x+y*y+z*z);
	nx = x/len;
	ny = y/len;
	nz = z/len;
	
	BYTE bVar3 = fx->uc[0x1a9]++;

	if(fx->uc[0x1a8]<bVar3) //dat.unk2[2]
	{
		fx->xyz[0]+=nx*fx->randomizer;
		fx->xyz[1]+=ny*fx->randomizer;
		fx->xyz[2]+=nz*fx->randomizer;
	}
	else
	{
		for(int i=3;i-->0;)
		fx->xyz[i]+=fx->const_vel[i];

		x = fx->const_vel[0];
		y = fx->const_vel[1];
		z = fx->const_vel[2];		
		len = sqrtf(x*x+y*y+z*z);
		nx = x/len;
		ny = y/len;
		nz = z/len;
	}

	fx->offset[0] = nx;
	fx->offset[1] = ny;
	fx->offset[2] = nz;

	memcpy(fx->mdl->xyzuvw,fx->xyz,3*sizeof(float));
	
	fx->mdl->xyzuvw[3] = -y;
	fx->mdl->xyzuvw[4] = atan2f(nz,nx)+M_PI_2;

	if(!fx->mdl->advance()) fx->mdl->d = 1;

	fx->duration--;
}

static BYTE __cdecl som_SFX_9a_4313e0_missile(sfx *fx, init *in)
{
	if(0||SOM::emu) 
	return ((BYTE(*)(sfx*,init*))0x4313e0)(fx,in);

	auto &dat = sfx_dat[in->sfx];

	if(!som_SFX_0a_42ed50_needle(fx,in)) return 0;

	//better to use SOM_PRM's setting 
	//I think!?
//	fx->duration = 256; //???

	fx->uc[0x1a8] = dat.unk2[2];
	fx->uc[0x1a9] = 0;
	fx->uc[0x1aa] = 0;
	fx->uc[0x1ab] = 0;

	unsigned r = SOM::rng()&0x8000001f;	
	if((int)r<0) r = (r-1|0xffffffe0)+1;
	fx->randomizer = ((int)r-16)*-0.001f;
	
	r = SOM::rng();
	fx->explode_sz[0] = (float)(r%0xb4+0xb4)*0.01;
	
	r = SOM::rng()&0x8000001f;
	if((int)r<0) r = (r-1|0xffffffe0)+1;
	fx->explode_sz[1] = ((int)r-16)*-0.001f;

	r = SOM::rng();
	fx->explode_sz[2] = (float)(r%0xb4+0xb4)*0.01;
	fx->explode_sz[3] = dat.speed*SOM::L.rate;
	fx->explode_sz[4] = 0;
	
	memset(fx->c+0x150,0x00,20);	
	
	(float&)fx->c[0x168] = in->look_vec[0];
	(float&)fx->c[0x16c] = in->look_vec[1];
	(float&)fx->c[0x170] = in->look_vec[2];
		
	//matrix diagonal?
	(float&)fx->c[0x1c4] = 1.0f;
	(float&)fx->c[0x1d4] = 1.0f;
	(float&)fx->c[0x1e4] = 1.0f;
	(float&)fx->c[0x1f4] = 1.0f;

	(float&)fx->c[0x1c0] = in->look_vec[0];
	(float&)fx->c[0x1d0] = in->look_vec[1];
	(float&)fx->c[0x1e0] = in->look_vec[2];

	float m[4][4];
	fx->uvw[0]+=M_PI_2;
	Somvector::map(m).rotation<4,4,'xyz'>(fx->uvw);
	fx->uvw[0]-=M_PI_2;

	//NOTE#1 the original code does this with a 4x4
	//matrix with the top row set to 0,0,1,1 all 0s
	//float v[3] = {0,0,1};
	//Somvector::map<3>(v).premultiply<4>(m);	
	(float&)fx->c[0x1bc] = m[2][0];
	(float&)fx->c[0x1cc] = m[2][1];
	(float&)fx->c[0x1dc] = m[2][2];

	fx->uvw[1]-=M_PI_2;
	Somvector::map(m).rotation<4,4,'xyz'>(fx->uvw);
	fx->uvw[1]+=M_PI_2;

	//NOTE#2 same deal as before
	(float&)fx->c[0x1b8] = m[2][0];
	(float&)fx->c[0x1c8] = m[2][1];
	(float&)fx->c[0x1d8] = m[2][2]; 
	
	return 1;
}
static void __cdecl som_SFX_9b_431850_missile2(sfx *fx)
{	
	auto &dat = sfx_dat[fx->sfx];

	char *c = fx->c; float *f = (float*)c; //shorthand

	BYTE bVar2 = fx->uc[0x1a9]++;

	if(fx->uc[0x1a8]<bVar2)
	{
		char cVar12 = fx->c[0x1aa];

		if(cVar12=='\x01')
		{
			auto &ai = SOM::L.ai[fx->uc[0x1ab]]; //lock-on

			float *xyz = (float*)&ai[SOM::AI::xyz];

			float f1 = xyz[0]-fx->mdl->xyzuvw[0];
			float f7 = xyz[1]-fx->mdl->xyzuvw[1]+ai[SOM::AI::height]*0.5;
			float f5 = xyz[2]-fx->mdl->xyzuvw[2];

			float f4 = sqrtf(f1*f1+f7*f7+f5*f5);
			f1 = f1/f4-f[0x15c/4];
			f7 = f7/f4-f[0x160/4];
			float z = f5/f4-f[0x164/4];
			f4 = dat.scale2*M_PI/180;
		
			float x = f4; if(f1<=f4)
			{
				x = f1; if(f1<-f4) x = -f4;
			}
			float y = f4; if(f7<=f4)
			{
				y = f7; if(f7<-f4) y = -f4;
			}
			if(z<=f4)
			{
				if(z<-f4) z = -f4;
			}
			else z = f4;

			x+=f[0x15c/4];
			y+=f[0x160/4];
			z+=f[0x164/4];

			float len = sqrtf(x*x+y*y+z*z);
					
			float nx = x/len;			
			float ny = y/len;
			float nz = z/len;
			
			f[0x15c/4] = nx;
			f[0x160/4] = ny;
			f[0x164/4] = nz;

			fx->xyz[0]+=nx*f[0x148/4];
			fx->xyz[1]+=ny*f[0x148/4];
			fx->xyz[2]+=nz*f[0x148/4];

			fx->mdl->xyzuvw[3] = -ny;
			fx->mdl->xyzuvw[4] = atan2f(nz,nx)+M_PI_2;			
		}
		else if(cVar12=='\x02') //straight
		{
			for(int i=3;i-->0;) fx->xyz[i]+=fx->const_vel[i];
		}
		else if(cVar12=='\x03') //scanning?
		{
			float f1 = fx->explode_sz[4]+=fx->explode_sz[3];			
			float f4 = (f1-fx->explode_sz[0])*fx->randomizer*f1;
			float f5 = (f1-fx->explode_sz[2])*fx->explode_sz[1]*f1;
			fx->xyz[0] = f4*f[0x1b8/4]+f5*f[0x1bc/4]+f1*f[0x1c0/4]+f[0x150/4];
			fx->xyz[1] = f4*f[0x1c8/4]+f5*f[0x1cc/4]+f1*f[0x1d0/4]+f[0x154/4];			
			fx->xyz[2] = f4*f[0x1d8/4]+f5*f[0x1dc/4]+f1*f[0x1e0/4]+f[0x158/4];			
		}
	}
	else
	{
		fx->xyz[0]+=fx->const_vel[0];
		fx->xyz[1]+=fx->const_vel[1];
		fx->xyz[2]+=fx->const_vel[2];

		float x = fx->const_vel[0];		
		float y = fx->const_vel[1];
		float z = fx->const_vel[2];		
		
		float len = sqrtf(x*x+y*y+z*z);
		f[0x15c/4] = x/len;
		f[0x160/4] = y/len;
		f[0x164/4] = z/len;

		if(c[0x1a8]==c[0x1a9])
		{
			//NOTE: the original code allocates 128
			//float scores on the stack and searches
			//for the best one afterward... that may
			//make sense for targeting multiple guys
			float lo = 999.9f;
			int best = -1, multi_targets = 0; //iVar14

			assert(SOM::L.ai_size<256);

			for(size_t i=0;i<SOM::L.ai_size;i++)
			{
				auto &ai = SOM::L.ai[i]; //lock-on

				if(3!=ai[SOM::AI::stage]) continue;

				float *xyz = (float*)&ai[SOM::AI::xyz];
				
				x = f[0x168/4];
				y = f[0x16c/4];				
				z = f[0x170/4];
				len = sqrtf(x*x+y*y+z*z);
				float nx = x/len;
				float ny = y/len;
				float nz = z/len;

				float dx = xyz[0]-fx->mdl->xyzuvw[0];
				float dy = xyz[1]-fx->mdl->xyzuvw[1];
				float dz = xyz[2]-fx->mdl->xyzuvw[2];
				float dlen = sqrtf(dx*dx+dy*dy+dz*dz);

				float f1 = dx/dlen*nx+dy/dlen*ny+dz/dlen*nz;

				//0.0 seems redundant?
				//if(f1>0.0f&&0.86f<f1&&dlen<21.0)
				if(0.86f<f1&&dlen<21.0)
				{
					multi_targets++;

					if(dlen<lo)
					{
						lo = dlen; best = i; 
					}
				}
			}

			if(multi_targets==0)
			{
				unsigned r = SOM::rng()&0x80000001;
				if((int)r<0) r = (r-1|0xfffffffe)+1;
				c[0x1aa] = (char)r+'\x02';

				f[0x150/4] = fx->xyz[0];
				f[0x154/4] = fx->xyz[1];
				f[0x158/4] = fx->xyz[2];
				f[0x14c/4] = 0.0f;
				
			}
			else
			{
				fx->c[0x1aa] = 1;
				fx->uc[0x1ab] = (BYTE)best;
			}			
		}
	}	
}
static void __cdecl som_SFX_9b_431850_missile(sfx *fx)
{
	if(0||SOM::emu) 
	return ((void(*)(sfx*))0x431850)(fx);
		
	auto &dat = sfx_dat[fx->sfx];

	if(!fx->duration) //terminate?
	{		
		return ((void(*)(sfx*))0x42f190)(fx);
	}
	else if(fx->duration<dat.unk2[1])
	{
		fx->mdl->fade = fx->duration*SOM::L.rate2;
	}
	else som_SFX_standard_hit_test(fx);

	som_SFX_9b_431850_missile2(fx);

	memcpy(fx->mdl->xyzuvw,fx->xyz,3*sizeof(float));

	if(!fx->mdl->advance()) fx->mdl->d = 1;

	fx->duration--;
}

static BYTE __cdecl som_SFX_12a_4331c0_firecolumn(sfx *fx, init *in)
{
	if(0||SOM::emu) 
	return ((BYTE(*)(sfx*,init*))0x4331c0)(fx,in);

	auto &dat = sfx_dat[in->sfx]; //26

	assert(2==sfx_ref[dat.model].type); //UNUSED?

	if(!som_SFX_basic_init(2,fx,in)) return 0;

	int cVar4 = dat.unk2[3]==0xff?SOM::rng():dat.unk2[3]+129;

	float fps = 30.0f/DDRAW::refreshrate; //EXTENSION

	fx->randomizer = (char)(BYTE)cVar4*M_PI/180*fps; //0.01745329

	return 1;
}
static void __cdecl som_SFX_12b_4332d0_firecolumn(sfx *fx)
{
	if(1||SOM::emu) 
	return ((void(*)(sfx*))0x4332d0)(fx);

	//UNFINISHED //UNUSED?

	//NOTE: this is a picture sfx but the one that uses it
	//(26) is assigned to a MDL number of a column of fire
}

static BYTE __cdecl som_SFX_20a_435130_earthwave(sfx *fx, init *in)
{
	if(0||SOM::emu) 
	return ((BYTE(*)(sfx*,init*))0x435130)(fx,in);

	auto &dat = sfx_dat[in->sfx];

	if(1!=sfx_ref[dat.model].type||X42EEB0) RETURN0

	fx->dmg_src = in->dmg_src;

	fx->dmg_src.unknown_sfx = 3;
	//height is subordinate sound effect
	//fx->dmg_src.radius = dat.height; //65535?
	fx->dmg_src.radius = dat.radius;

	//fx->duration = (int)(in->duration/dat.speed*30);
	fx->duration = dat.unk2[2]+dat.unk2[1]; //???

	if(fx->dmg_src.source==(void*)0x19c1a18)
	{
		memcpy(fx->xyz,SOM::xyz,3*sizeof(float));
	}
	else memcpy(fx->xyz,in->xyz,3*sizeof(float));

	fx->uvw[0] = 0.0f;
	fx->uvw[1] = SOM::rng()%360*M_PI/180;
	fx->uvw[2] = 0.0f;

	fx->uc30[0] = dat.unk2[1];
	fx->uc30[1] = dat.unk2[2];

	fx->f30[1] = dat.unk2[3]*M_PI/180;
	fx->f30[2] = dat.radius;

	fx->unk33[0] = dat.speed; //84 0
	fx->unk33[1] = dat.scale; //88 1
	fx->unk33[2] = dat.scale2; //8c 2
	fx->unk33[3] = dat.unk5[0]; //90 3
	fx->unk33[4] = dat.unk5[1]; //94 4
		
	memcpy(fx->mdl->xyzuvw,fx->xyz,6*sizeof(float));
	fx->mdl->scale[0] =	
	fx->mdl->scale[1] =
//	fx->mdl->scale[2] = dat.scale*in->scale;
	fx->mdl->scale[2] = fx->scale*dat.radius; //???
//	fx->mdl->fade = 1.0f;
	fx->mdl->fade = dat.scale2; //???

	fx->mdl->d = 1; //set_animation_goal_frame (HACK)	

	return 1;
}
static void __cdecl som_SFX_20b_435490_earthwave(sfx *fx)
{
	if(0||SOM::emu) 
	return ((void(*)(sfx*))0x435490)(fx);

	auto &dat = sfx_dat[fx->sfx];

	if(!fx->duration) //terminate?			
	return ((void(*)(sfx*))0x42f190)(fx);
	
	char *c = fx->c; float *f = (float*)c; //shorthand

	float f6,f5 = fx->const_vel[1]+fx->uvw[1];
	fx->uvw[1] = fx->mdl->xyzuvw[4] = f5;
	float f7 = fx->uc30[0];
	unsigned u9 = fx->uc30[1];
	unsigned u3 = fx->duration;
	if(u9<u3)
	{
		f6 = (float)((fx->uc30[0]-u3)+u9);
		f5 = ((fx->unk33[3]-fx->unk33[2])*f6)/f7+fx->unk33[2];
		f6 = ((fx->unk33[0]-fx->f30[2])*f6)/f7+fx->f30[2];
	}
	else
	{
		f5 = ((fx->unk33[4]-fx->unk33[3])*(u9-u3))/u9+fx->unk33[3];
		f6 = ((fx->unk33[1]-fx->unk33[0])*(u9-u3))/u9+fx->unk33[0];
	}
	fx->mdl->fade = f5;
	fx->mdl->scale[0] = 
	fx->mdl->scale[1] = 
	fx->mdl->scale[2] = f6*fx->scale;
	
	if((fx->uc30[0]+fx->uc30[1])-fx->duration==dat.unk2[4])
	{
		//FUN_0040b600_ball_hit_test_using_computed_radius
		if(((BYTE(*)(float[3],float,int,void*,FLOAT*))0x40b600)
		(fx->xyz,f6*fx->scale*0.5,15, //15?
		 fx->dmg_src.source,SOM::L.hit_buf_pos)) //hit_buffer
		{
			if(SOM::L.hit_buf_pc_hit) //0x556274
			{
				fx->dmg_src.target = (som_NPC*)0x19c1a18;
				fx->dmg_src.target_type = 1;				
				((void(*)(void*))0x4041d0)(&fx->dmg_src); //add_damage_source
				float xyz[3]; memcpy(xyz,SOM::xyz,sizeof(xyz));
				if(!dat.unk2[5]) xyz[1]+=0.9f;
				som_SFX_42f1f0(fx,xyz,(float*)0x556284,1);
			}
			for(DWORD i=0;i<SOM::L.hit_buf_enemies_hit;i++)
			{
				auto &s = SOM::L.hit_buf_enemies[i];

				fx->dmg_src.target_enemy = (som_Enemy*)s.i[0];
				fx->dmg_src.target_type = 2;
				((void(*)(void*))0x4041d0)(&fx->dmg_src); //add_damage_source
				int i4 = s.i[0];
				float xyz[3]; memcpy(xyz,(float*)(i4+0x48),sizeof(xyz));
				if(!dat.unk2[5]) xyz[1]+=*(float*)(i4+0x22c)*0.5;
				som_SFX_42f1f0(fx,xyz,s.f+4,2); //1
			}
			for(DWORD i=0;i<SOM::L.hit_buf_NPCs_hit;i++)
			{
				auto &s = SOM::L.hit_buf_NPCs[i];
				
				fx->dmg_src.target = (som_NPC*)s.i[0];
				fx->dmg_src.target_type = 4;
				((void(*)(void*))0x4041d0)(&fx->dmg_src); //add_damage_source
				int i4 = s.i[0];
				float xyz[3]; memcpy(xyz,(float*)(i4+0x40),sizeof(xyz));
				if(!dat.unk2[5]) xyz[1]+=*(float*)(i4+0x68)*0.5;
				som_SFX_42f1f0(fx,xyz,s.f+4,4); //1
			}
		}
	}

	memcpy(fx->mdl->xyzuvw,fx->xyz,6*sizeof(float));
	
	if(!fx->mdl->advance()) fx->mdl->d = 1;

	fx->duration--;
}

static BYTE __cdecl som_SFX_21a_43bbd0_firewall(sfx *fx, init *in)
{
	if(0||SOM::emu) 
	return ((BYTE(*)(sfx*,init*))0x43bbd0)(fx,in);

	auto &dat = sfx_dat[in->sfx];

	if(2!=sfx_ref[dat.model].type||X42EEB0) RETURN0
	
	memcpy(fx->f30,in,0x1a*sizeof(float));
	memcpy(fx->xyz,in->xyz,3*sizeof(float));

	//FUN_00417540_transform_pos_to_tile_xy?_elevation?
	if(!((BYTE(__cdecl*)(FLOAT,FLOAT,FLOAT*))0x417540)(fx->xyz[0],fx->xyz[2],fx->xyz+1))
	return 0;

	fx->uvw[0] = 0.0f;	
	fx->uvw[1] = atan2f(in->look_vec[2],in->look_vec[0])+M_PI_2;
	fx->uvw[2] = 0.0f;
	
	fx->dmg_src = in->dmg_src;

	memcpy(fx->dmg_src.attack_origin,fx->xyz,3*sizeof(float));

	fx->dmg_src.target = (som_NPC*)0x2;
	fx->dmg_src.radius = dat.radius;	
	fx->dmg_src.height = dat.speed;
	fx->dmg_src.target_type = 9;

	fx->duration = dat.unk2[1]; //???
	
	return 1;
}
static void __cdecl som_SFX_21b_43bce0_firewall(sfx *fx)
{
	if(0||SOM::emu) 
	return ((void(*)(sfx*))0x43bce0)(fx);

	auto &dat = sfx_dat[fx->sfx];

	if(!fx->duration) //terminate?
	return ((void(*)(sfx*))0x42f190)(fx);
		
	DWORD color = dat.unk2[5];
	if(fx->duration<dat.unk2[2])
	color = (fx->duration*color)/dat.unk2[2];

	void *vmo = (void*)0x4c2358; float x,y,z;
	((BYTE(__thiscall*)(void*,float*,float*,float*))0x401070)(vmo,&x,&y,&z);
	x-=fx->xyz[0];
	z-=fx->xyz[2];
	fx->yaw = atan2f(z,x)+M_PI_2;

	//ALLOCATING 2
	//se = (uint*)FUN_0042f7a0_block_alloc_image_based_sfx
	auto se = som_MDL_42f7a0(2,dat.unk2[4]);
	if(!se) return;
	
	float s2 = dat.unk5[1];	
	if(dat.unk2[2]<=fx->duration) //lerp?
	s2-=(s2-dat.unk5[0])*(fx->duration-dat.unk2[2])/(dat.unk2[1]-dat.unk2[2]);		
	s2*=fx->scale;
	float s1 = dat.scale2*0.5*fx->scale;

	se->vdata[0].x = -s1;
	se->vdata[1].x = s1;
	se->vdata[2].x = s1;
	se->vdata[3].x = -s1;
	se->vdata[0].y = s2;
	se->vdata[1].y = s2;	
	se->vdata[2].y = 0.0;
	se->vdata[3].y = 0.0;
	se->vdata[0].s = 0.0;
	se->vdata[1].s = 1.0;	
	se->vdata[2].s = 1.0;
	se->vdata[3].s = 0.0;
	se->vdata[0].t = 0.0;
	se->vdata[1].t = 0.0;
	se->vdata[2].t = 1.0;	
	se->vdata[3].t = 1.0;

	DWORD diffuse; if(dat.unk2[4]==2) //rgb blend?
	{
		diffuse = ((color|0xffffff00)<<8|color)<<8|color;
	}
	else diffuse = color<<24|0xffffff; //alpha

	auto *p = se->vdata;
	for(int i=4;i-->0;p++)
	{
		p->z = 0; p->diffuse = diffuse;
	}	

	//ALLOCATING 2
	se[1].texture = se->texture = fx->txr;

	float m[4][4];
	Somvector::map(m).rotation<3,4,'xyz'>(fx->uvw);
	Somvector::map(m[3]).copy<3>(fx->xyz).se<3>() = 1;

	p = se->vdata;
	for(int i=4;i-->0;p++)
	{
		auto *v = &p->x; //YUCK
		Somvector::map<3>(v).premultiply<4>(m);
	}

	//BACK-FACE?
	for(int i=0;i<4;i++)	
	memcpy(se[1].vdata+3-i,se->vdata+i,sizeof(*se->vdata));
		
	char *c = fx->c; float *f = (float*)c; //shorthand

	if(dat.unk2[1]==fx->duration)
	{
		float xyz[3];
		memcpy(xyz,fx->dmg_src.attack_origin,sizeof(xyz));
		xyz[1]+=0.1f;
		//FUN_0040bb40_step_damage_sub
		if(((BYTE(*)(float[3],float,float,int,void*,FLOAT*))0x40bb40)
		(xyz,fx->dmg_src.height,fx->dmg_src.radius,31,
		 fx->dmg_src.source,SOM::L.hit_buf_pos)) //hit_buffer
		{
			(void*&)fx->dmg_src.target = SOM::L.target;

			int tt = SOM::L.target_type; //1 2 4 8 16			
			tt = tt==16?0:tt;
			fx->dmg_src.target_type = tt;

			((void(*)(void*))0x4041d0)(&fx->dmg_src); //add_damage_source
			som_SFX_42f1f0(fx,SOM::L.hit_buf_pos,SOM::L.hit_buf_look_vec,tt);

			f[0x98/4] = 0.0f; //???
		}
	}

	if(dat.unk2[1]-fx->duration==dat.unk2[3])
	{
		f[0x98/4]-=dat.scale; if(0.0<=f[0x98/4])
		{
			float x = f[0x8c/4];
			float z = f[0x94/4];									
			float len = sqrtf(x*x+z*z);
			fx->const_vel[2] = fx->xyz[0]+(x*dat.scale)/len;
			f[0x84/4] = fx->xyz[1];
			f[0x88/4] = fx->xyz[2]+(z*dat.scale)/len;
			//FUN_0042ea10_init_sfx_instance((init*)fx->const_vel);
			((BYTE(*)(init*))0x42ea10)((init*)fx->const_vel);
		}
	}

	fx->duration--;
}

static BYTE __cdecl som_SFX_22a_4352f0_explosion(sfx *fx, init *in)
{
	if(!som_SFX_20a_435130_earthwave(fx,in)) return 0;

	//NOTE: this is the only difference
	if(fx->dmg_src.source==(void*)0x19c1a18)
	{
		assert(0); //does sword magic do this?

		//NOTE: this is undoing putting xyz at feet 
		//level (which might be an improvement)
		memcpy(fx->xyz,in->xyz,3*sizeof(float));
		memcpy(fx->mdl->xyzuvw,in->xyz,3*sizeof(float));
	}

	return 1;
}

static BYTE __cdecl som_SFX_30a_43b6d0_haze(sfx *fx, init *in)
{
	if(0||SOM::emu) 
	return ((BYTE(*)(sfx*,init*))0x43b6d0)(fx,in);

	auto &dat = sfx_dat[in->sfx];

	if(!som_SFX_basic_init(2,fx,in)) return 0;
	
	int cVar4 = dat.unk2[3]==0xff?SOM::rng():dat.unk2[3]+129;

	float fps = 30.0f/DDRAW::refreshrate; //EXTENSION

	fx->randomizer = (char)(BYTE)cVar4*M_PI/180*fps; //0.01745329

	(DWORD&)fx->c[0x84] = fx->duration; //???

	return 1;
}
static void __cdecl som_SFX_30b_43b7e0_haze(sfx *fx)
{
	if(0||SOM::emu) 
	return ((void(*)(sfx*))0x43b7e0)(fx);

	auto &dat = sfx_dat[fx->sfx];

	if(!fx->duration) //terminate?
	return ((void(*)(sfx*))0x42f190)(fx);
	
	som_SFX_standard_hit_test(fx);

	DWORD color = fx->duration*255/(DWORD&)fx->c[0x84];

	for(int i=3;i-->0;) fx->xyz[i]+=fx->const_vel[i];

	//FUN_00401070_get_XYZ_thiscall?_4c2358_view_matrix_only?
	void *vmo = (void*)0x4c2358; float x,y,z;
	((BYTE(__thiscall*)(void*,float*,float*,float*))0x401070)(vmo,&x,&y,&z);
	x-=fx->xyz[0]; 
	y-=fx->xyz[1];
	z-=fx->xyz[2];		
	fx->uvw[0] = -atan2f(y,sqrtf(z*z+x*x));
	fx->uvw[1] = +atan2f(z,x)+M_PI_2;

	//se = (uint*)FUN_0042f7a0_block_alloc_image_based_sfx
	auto se = som_MDL_42f7a0(1,dat.unk2[2]);
	if(!se) return;
		
	float scaleSq = fx->scale*fx->scale; //???

	float s1 = scaleSq*dat.scale*0.5;
	float s2 = scaleSq*dat.scale2*0.5;

	fx->dmg_src.radius = dat.radius*fx->scale;
	fx->scale+=dat.unk5[1];

    se->vdata[0].x = -s1;
	se->vdata[1].x = s1;
	se->vdata[2].x = s1;
	se->vdata[3].x = -s1;
    se->vdata[0].y = s2;
    se->vdata[1].y = s2;
    se->vdata[2].y = -s2;
    se->vdata[3].y = -s2;
    int iVar10 = dat.unk2[3]-fx->duration%dat.unk2[3];
    float s = (float)(iVar10%dat.unk2[4])*(1.0f/dat.unk2[4]);
    float t = (float)(iVar10/dat.unk2[5])*(1.0f/dat.unk2[5]);
    se->vdata[0].s = s;
    se->vdata[1].s = 
	se->vdata[2].s = s+1.0f/dat.unk2[4];
	se->vdata[3].s = s;
	se->vdata[0].t = t;
    se->vdata[1].t = t;
    se->vdata[2].t = 
    se->vdata[3].t = t+1.0f/dat.unk2[5];    

	DWORD diffuse; if(dat.unk2[2]==2) //rgb blend?
	{
		diffuse = ((color|0xffffff00)<<8|color)<<8|color;
	}
	else diffuse = color<<24|0xffffff; //alpha

	auto *p = se->vdata;
	for(int i=4;i-->0;p++)
	{
		p->z = 0; p->diffuse = diffuse;
	}		

	se->texture = fx->txr;

	float m[4][4];
	Somvector::map(m).rotation<3,4,'xyz'>(fx->uvw);
	Somvector::map(m[3]).copy<3>(fx->xyz).se<3>() = 1;		

	p = se->vdata;
	for(int i=4;i-->0;p++)
	{
		auto *v = &p->x; //YUCK
		Somvector::map<3>(v).premultiply<4>(m);
	}
	
	fx->duration--;
}

static BYTE __cdecl som_SFX_31a_435790_vortex(sfx *fx, init *in)
{
	if(0||SOM::emu) 
	return ((BYTE(*)(sfx*,init*))0x435790)(fx,in);

	auto &dat = sfx_dat[in->sfx];

	if(1!=sfx_ref[dat.model].type||X42EEB0) RETURN0

	fx->dmg_src = in->dmg_src;

	fx->dmg_src.target_type = 2;
	fx->dmg_src.target = (som_NPC*)0x2;

	memcpy(fx->dmg_src.attack_origin,fx->xyz,3*sizeof(float));
	
	fx->dmg_src.radius = 1.0f;
	fx->dmg_src.height = 1.0f;

	float local_c = in->xyz[0];
	float local_4 = in->xyz[2];

	float x = in->look_vec[0];
	float z = in->look_vec[2];	
	float len = sqrtf(x*x+z*z);

	float *enemy = 0;
	
	if(0.0f<in->duration-dat.radius)
	{
		local_c = (x/len)*in->duration+local_c;
		local_4 = (z/len)*in->duration+local_4;
	}
	if(in->dmg_src.source==(som_NPC*)0x19c1a18)
	{
		if(SOM::L.ai_size)
		{
			float s2 = dat.scale2; //limit range?

			for(size_t i=SOM::L.ai_size;i-->0;) 
			{
				auto &ai = SOM::L.ai[i];
				if(3!=ai[SOM::AI::stage]) continue;

				auto *st = &ai[SOM::AI::ai_state];				
				//if(*piVar14!=5||*(char*)(piVar14+2)=='\0') //???
				if(5!=*st||*(char*)(st+2)==0) //trigger related???
				{  
					//psVar6 = DAT_004c67c8_enemy_pr2
					//[*(ushort *)(&DAT_004da1c8+*(ushort *)(piVar14+-0x88)*0x1e8)];
		//			WORD prm = SOM::L.enemy_prm_file[ai[SOM::AI::enemy]].us[0];
		//			BYTE *pr2 = SOM::L.enemy_pr2_data[prm];
		//			auto *prf = (swordofmoonlight_prf_enemy_t*)(pr2+31*2);

		//		this is disabling if there are evasions set up, which 
		//		causes the snare to not lock-on???

					//wth does all this this mean?
					//uVar7._0_1_ = psVar6->flags;
					//uVar7._1_1_ = psVar6->activation;
					//uVar7._2_2_ = psVar6->unknown2;									   
					//if((uVar7&0x30)!=0x10&&(uVar7&0x30)!=0x20)
		//			if((prf->countermeasures&3)!=1&&(prf->countermeasures&3)!=2) //???
					{
						x = (&ai[SOM::AI::xyz])[0]-local_c;
						z = (&ai[SOM::AI::xyz])[2]-local_4;

						len = sqrtf(x*x+z*z);

						if(len<s2)
						{
							s2 = len; enemy = (float*)&ai;
						}
					}
				}
			};

			if(enemy)
			{
				fx->xyz[0] = enemy[0x12];
				fx->xyz[1] = enemy[0x13];
				fx->xyz[2] = enemy[0x14];
				fx->dmg_src.target_enemy = (som_Enemy*)enemy;
			//	fx->dmg_src.target_type = 2;
				goto LAB_00435a1c;
			}
		}
		fx->xyz[0] = local_c;
		fx->xyz[1] = in->xyz[1];
		fx->xyz[2] = local_4;
		fx->dmg_src.target_enemy = (som_Enemy*)enemy;
	//	fx->dmg_src.target_type = 2;
	}
	else
	{
		x = SOM::xyz[0]-local_c;
		z = SOM::xyz[2]-local_4;
		if(dat.scale2<sqrtf(x*x+z*z))
		{
			fx->xyz[0] = local_c;
			fx->xyz[1] = in->xyz[1];
			fx->xyz[2] = local_4;
			fx->dmg_src.target = 0;
		}
		else
		{
			fx->xyz[0] = SOM::xyz[0];
			fx->xyz[1] = SOM::xyz[1];
			fx->xyz[2] = SOM::xyz[2];
			fx->dmg_src.target = (som_NPC*)0x19c1a18;
		}
		fx->dmg_src.target_type = 1;
	}
	LAB_00435a1c:

	//FUN_00417540_transform_pos_to_tile_xy?_elevation?
	((BYTE(__cdecl*)(FLOAT,FLOAT,FLOAT*))0x417540)(fx->xyz[0],fx->xyz[2],fx->xyz+1);
	
	fx->uvw[0] = 0.0;
	fx->uvw[1] = 0.0;
	fx->uvw[2] = 0.0;
	memcpy(fx->mdl->xyzuvw,fx->xyz,6*sizeof(float));

	fx->mdl->scale[0] = 
	fx->mdl->scale[2] = fx->scale;
	fx->mdl->scale[1] = dat.radius*fx->scale;
	fx->mdl->fade = fx->scale;

	fx->duration = dat.unk2[1];

	return 1;
}
static void __cdecl som_SFX_31b_435ab0_vortex(sfx *fx)
{
	if(0||SOM::emu) 
	return ((void(*)(sfx*))0x435ab0)(fx);

	auto &dat = sfx_dat[fx->sfx];

	if(!fx->duration) //terminate?
	{		
		//animate? UVs? Y components?
		((void(*)(SOM::MDL*))0x441a30)(fx->mdl);

		return ((void(*)(sfx*))0x42f190)(fx); 
	}

	if(fx->dmg_src.target&&(dat.unk2[1]-fx->duration==dat.unk2[5]))
	{
		//add_damage_source
		((void(*)(void*))0x4041d0)(&fx->dmg_src); 
		DWORD tt = fx->dmg_src.target==(void*)0x19c1a18?1:2;
		float look_vec[3] = {0,1,0};
		som_SFX_42f1f0(fx,fx->xyz,look_vec,tt);
	}
	
	int local_c = dat.unk2[2];
	int iVar3 = dat.unk2[1]-fx->duration;
	if(iVar3<(int)local_c)
	{
		fx->mdl->scale[1] = ((dat.speed-dat.radius)*(float)iVar3)/(float)local_c+dat.radius;
		fx->mdl->scale[1]*=fx->scale;
	}
	
	int uVar4 = fx->duration;
	if(uVar4<dat.unk2[3])
	{
		fx->mdl->fade = (uVar4*dat.speed)/(float)dat.unk2[3];
		local_c = uVar4;
	}

	if(!fx->mdl->advance()) fx->mdl->d = 1; 

	/*Ghidra hates ftol() ???
    00435bd9 d9 46 20        FLD        dword ptr [ESI + 0x20]=>DAT_01c91d30_SFX_dat_file[0].unk5[0]
    00435bdc e8 57 9d 01 00  CALL       __ftol
    00435be1 d9 46 24        FLD        dword ptr [ESI + 0x24]=>DAT_01c91d30_SFX_dat_file[0].unk5[1]
    00435be4 8b d8           MOV        EBX,EAX
    00435be6 89 5c 24 1c     MOV        dword ptr [ESP + fx],EBX
    00435bea e8 49 9d 01 00  CALL       __ftol
	*/
	iVar3 = (int)dat.unk5[0]; //iVar3 = __ftol();	
	local_c = (int)dat.unk5[1]; //local_c = __ftol();

	uVar4 = (dat.unk2[1]-fx->duration)%(local_c*iVar3);	
	if(uVar4==0)
	{
		//animate? UVs? Y components?
		((void(*)(SOM::MDL*))0x441a30)(fx->mdl);

		fx->duration--;

		return;
	}
	
	float f8,f6; if((int)uVar4%iVar3==0)
	{
		f8 = 1.0f/(float)local_c;
		f6 = (1.0f/(float)iVar3)*(float)(iVar3+-1);
	}
	else
	{
		f8 = 0.0f;
		f6 = 1.0f/(float)iVar3;
	}	
	((void(*)(SOM::MDL*,float,float))0x441ab0)(fx->mdl,f6,f8);

	fx->duration--;
}

static BYTE __cdecl som_SFX_32a_435c90_dragon(sfx *fx, init *in)
{
	if(0||SOM::emu) 
	return ((BYTE(*)(sfx*,init*))0x435c90)(fx,in);

	if(!som_SFX_31a_435790_vortex(fx,in)) return 0;

	fx->mdl->xyzuvw[4] = fx->uvw[1] = SOM::rng()%360*M_PI/180;

	return 1;
}
static void __cdecl som_SFX_32b_435fc0_dragon(sfx *fx)
{
	if(0||SOM::emu) 
	return ((void(*)(sfx*))0x435fc0)(fx);

	auto &dat = sfx_dat[fx->sfx];

	if(!fx->duration) //terminate?
	{		
		//animate? UVs? Y components?
		((void(*)(SOM::MDL*))0x441a30)(fx->mdl);

		return ((void(*)(sfx*))0x42f190)(fx); 
	}

	if(fx->dmg_src.target&&(dat.unk2[1]-fx->duration==dat.unk2[5]))
	{
		//add_damage_source
		((void(*)(void*))0x4041d0)(&fx->dmg_src); 
		DWORD tt = fx->dmg_src.target==(void*)0x19c1a18?1:2;
		float look_vec[3] = {0,1,0};
		som_SFX_42f1f0(fx,fx->xyz,look_vec,tt);
	}
	
	int local_c = dat.unk2[2];
	int iVar3 = dat.unk2[1]-fx->duration;
	if(iVar3<(int)local_c)
	{
		fx->mdl->scale[1] = ((dat.speed-dat.radius)*(float)iVar3)/(float)local_c+dat.radius;
		fx->mdl->scale[1]*=fx->scale;
	}
	
	int uVar4 = fx->duration;
	if(uVar4<dat.unk2[3])
	{
		fx->mdl->fade = (uVar4*dat.speed)/(float)dat.unk2[3];
		local_c = uVar4;
	}

	if(!fx->mdl->advance()) fx->mdl->d = 1; 

	((void(*)(SOM::MDL*,float,float))0x441ab0)(fx->mdl,dat.unk5[0],dat.unk5[1]);

	fx->duration--;
}

static BYTE __cdecl som_SFX_34a_4365c0_tornado(sfx *fx, init *in)
{
	if(0||SOM::emu) 
	return ((BYTE(*)(sfx*,init*))0x4365c0)(fx,in);

	auto &dat = sfx_dat[in->sfx];

	if(1!=sfx_ref[dat.model].type||X42EEB0) RETURN0

	float z = in->look_vec[2];	
	float x = in->look_vec[0];	
	float len = sqrtf(x*x+z*z);
	float nx = x/len;
	float nz = z/len;
	memcpy(fx->xyz,in->xyz,3*sizeof(float));
	fx->uvw[0] = 0.0;
	fx->uvw[1] = atan2f(nz,nx)+M_PI_2;
	fx->uvw[2] = 0.0;		
	fx->const_vel[0] = nx*dat.speed*fx->scale*SOM::L.rate;
	fx->const_vel[1] = 0.0;	
	fx->const_vel[2] = nz*dat.speed*fx->scale*SOM::L.rate;

	fx->dmg_src = in->dmg_src;
	fx->dmg_src.target_type = 2;
	fx->dmg_src.radius = dat.scale;
	fx->dmg_src.height = dat.scale2;

	fx->c[0x84] = 0; //?

	fx->duration = (int)(in->duration/dat.speed*DDRAW::refreshrate);

	float *puVar4 = (float*)fx->dmg_src.source;

	if(puVar4==(float*)0x19c1a18)
	{
		fx->xyz[1] = SOM::xyz[1];
	}
	else fx->xyz[1] = puVar4[0x26]; fx->xyz[1]+=0.1f;

	memcpy(fx->mdl->xyzuvw,fx->xyz,6*sizeof(float));

	fx->mdl->scale[0] = 
	fx->mdl->scale[1] = 
	fx->mdl->scale[2] = fx->scale;	
	fx->mdl->fade = 1.0;
	return 1;
}
static void __cdecl som_SFX_34b_436760_tornado(sfx *fx)
{
	if(0||SOM::emu) 
	return ((void(*)(sfx*))0x436760)(fx);

	auto &dat = sfx_dat[fx->sfx];

	if(!fx->duration) //terminate?
	{		
		//animate? UVs? Y components?
		((void(*)(SOM::MDL*))0x441a30)(fx->mdl);

		return ((void(*)(sfx*))0x42f190)(fx);
	}
	
	if(!fx->c[0x84]) 
	{
		if(fx->duration>=dat.unk2[1])
		{
			memcpy(fx->dmg_src.attack_origin,fx->xyz,3*sizeof(float));

			//FUN_0040bb40_step_damage_sub
			if(((BYTE(*)(float[3],float,float,int,void*,FLOAT*))0x40bb40)
			(fx->dmg_src.attack_origin,fx->dmg_src.height,fx->dmg_src.radius,31,
			 fx->dmg_src.source,SOM::L.hit_buf_pos))
			{
				fx->c[0x84] = 1;

				(void*&)fx->dmg_src.target = SOM::L.target;

				switch(DWORD tt=SOM::L.target_type)
				{
				case 1: case 2: case 4:

					fx->dmg_src.target_type = tt;
					((void(*)(void*))0x4041d0)(&fx->dmg_src); //add_damage_source
					som_SFX_42f1f0(fx,SOM::L.hit_buf_pos,SOM::L.hit_buf_look_vec,tt);

					fx->duration = dat.unk2[2]+dat.unk2[1];
					break;

				case 8: case 16:

					fx->duration = dat.unk2[1];
					break;
				}
			}
		}
	
		for(int i=3;i-->0;) fx->xyz[i]+=fx->const_vel[i];

		memcpy(fx->mdl->xyzuvw,fx->xyz,3*sizeof(float));
	}

	if(fx->duration<dat.unk2[1])
	{
		fx->mdl->fade = fx->duration*SOM::L.rate2;
	}

	if(!fx->mdl->advance()) fx->mdl->d = 1;

	((void(*)(SOM::MDL*,float,float))0x441ab0)(fx->mdl,dat.unk5[0],dat.unk5[1]);

	fx->duration--;
}

static BYTE __cdecl som_SFX_40a_4369b0_lightning(sfx *fx, init *in)
{
	if(0||SOM::emu) 
	return ((BYTE(*)(sfx*,init*))0x4369b0)(fx,in);

	auto &dat = sfx_dat[in->sfx];

	if(2!=sfx_ref[dat.model].type||X42EEB0) RETURN0
	
	float x = in->look_vec[0];
	float z = in->look_vec[2];
	float len = sqrtf(x*x+z*z); //4500d0
	float local_34 = in->xyz[0];
	float local_2c = in->xyz[2];
	float fVar6 = in->duration-dat.radius;
	if(0.0<fVar6)
	{
		local_34 = (x/len)*fVar6+local_34;
		local_2c = (z/len)*fVar6+local_2c;
	}

	float s2 = dat.radius;
	float *enemy = 0;
	for(size_t i=0;i<SOM::L.ai_size;i++)
	{
		auto &ai = SOM::L.ai[i];
		if(3!=ai[SOM::AI::stage]) continue;

		//ignoring this (same deal as vortex???)
	//	if(((*(int *)&local_14->field_0x23c==3)&&
	//	   (psVar5 = DAT_004c67c8_enemy_pr2
	//	   [*(ushort *)(&DAT_004da1c8+(uint)local_14->field_0x20_enemy*0x1e8)],
	//	   uVar9._0_1_ = psVar5->flags,uVar9._1_1_ = psVar5->activation,
	//	   uVar9._2_2_ = psVar5->unknown2,(uVar9&0x30)!=0x10))&&((uVar9&0x30)!=0x20))
		{
			float xx = (&ai[SOM::AI::xyz])[0]-local_34;
			float zz = (&ai[SOM::AI::xyz])[2]-local_2c;
			len = sqrtf(xx*xx+zz*zz);
			if(len<s2)
			{
				s2 = len; enemy = (float*)&ai;
			}
		}
	}
	if(enemy==0)
	{
		fx->xyz[0] = local_34;
		fx->xyz[1] = in->xyz[1];
		fx->xyz[2] = local_2c;
	}
	else
	{
		fx->xyz[0] = enemy[0x48/4];
		fx->xyz[1] = enemy[0x4c/4];
		fx->xyz[2] = enemy[0x50/4];
	}

	//FUN_00417540_transform_pos_to_tile_xy?_elevation?
	if(!((BYTE(__cdecl*)(FLOAT,FLOAT,FLOAT*))0x417540)(fx->xyz[0],fx->xyz[2],fx->xyz+1))
	return 0;
		
	fx->uvw[0] = 0.0;
	fx->uvw[2] = 0.0;
	fx->uvw[1] = atan2f(z,x); //4500ba
		
	fx->dmg_src = in->dmg_src;

	fx->duration = dat.unk2[1];
	return 1;
}
static void __cdecl som_SFX_40b_436c40_lightning(sfx *fx)
{
	if(0||SOM::emu) 
	return ((void(*)(sfx*))0x436c40)(fx);

	auto &dat = sfx_dat[fx->sfx];

	if(!fx->duration) //terminate?
	return ((void(*)(sfx*))0x42f190)(fx);

	int local_c = dat.unk2[1]-fx->duration;
	if(local_c<dat.unk2[2])
	{
		//FUN_00401070_get_XYZ_thiscall?_4c2358_view_matrix_only?
		void *vmo = (void*)0x4c2358; float x,_,z;
		((BYTE(__thiscall*)(void*,float*,float*,float*))0x401070)(vmo,&x,&_,&z);
		x-=fx->xyz[0];
		z-=fx->xyz[2];
		fx->uvw[1] = atan2f(z,x)+M_PI_2; //4500ba

		//se = (uint*)FUN_0042f7a0_block_alloc_image_based_sfx
		if(auto se=som_MDL_42f7a0(1,2))
		{		
			float s1 = dat.unk5[0]*0.5f;
			float s2 = dat.unk5[1];
			se->vdata[0].x = -s1;
			se->vdata[1].x = s1;
			se->vdata[2].x = s1;
			se->vdata[3].x = -s1;
			se->vdata[0].y = s2;			
			se->vdata[1].y = s2; 			
			se->vdata[2].y = 0.0;			
			se->vdata[3].y = 0.0;
			se->vdata[0].s = 0.0;			
			se->vdata[1].s = 1.0;
			se->vdata[2].s = 1.0;
			se->vdata[3].s = 0.0;
			se->vdata[0].t = 0.0;
			se->vdata[1].t = 0.0;			
			se->vdata[2].t = 1.0;			
			se->vdata[3].t = 1.0;
			
			auto *p = se->vdata;
			for(int i=4;i-->0;p++)
			{
				p->z = 0; p->diffuse = 0xffffffff;
			}
			
			se->texture = fx->txr;

			float m[4][4];
			Somvector::map(m).rotation<3,4,'xyz'>(fx->uvw);
			Somvector::map(m[3]).copy<3>(fx->xyz).se<3>() = 1;

			p = se->vdata;
			for(int i=4;i-->0;p++)
			{
				auto *v = &p->x; //YUCK
				Somvector::map<3>(v).premultiply<4>(m);
			}			
		}
		
	}	
	if(local_c==dat.unk2[3])
	{
		init in = {};		
		in.sfx = fx->sfx+1; //!!
		memcpy(in.xyz,fx->xyz,3*sizeof(float));
		((BYTE(*)(init*))0x42ea10)(&in);
	}
	if(local_c==dat.unk2[4])
	{
		init in = {};
		in.sfx = fx->sfx+2; //!! 		
		in.dmg_src = fx->dmg_src;
		memcpy(in.xyz,fx->xyz,3*sizeof(float));
		((BYTE(*)(init*))0x42ea10)(&in);
	}
	if(local_c==dat.unk2[5])
	{
		init in = {};
		in.sfx = fx->sfx+3; //!!
		memcpy(in.xyz,fx->xyz,3*sizeof(float));
		in.xyz[1]+=0.1f;
		((BYTE(*)(init*))0x42ea10)(&in);
	}
	fx->duration--;
}

static BYTE __cdecl som_SFX_45a_437cd0_moonlight(sfx *fx, init *in)
{
	if(0||SOM::emu) 
	return ((BYTE(*)(sfx*,init*))0x437cd0)(fx,in);

	return som_SFX_0a_42ed50_needle(fx,in);
}
static void __cdecl som_SFX_45b_437e30_moonlight(sfx *fx)
{
	if(0||SOM::emu) 
	return ((void(*)(sfx*))0x437e30)(fx);

	auto &dat = sfx_dat[fx->sfx];

	if(!fx->duration) //terminate?
	return ((void(*)(sfx*))0x42f190)(fx);

	if(fx->duration<dat.unk2[1])
	{
		fx->mdl->fade = fx->duration*SOM::L.rate2;
	} 	
	else
	{
		memcpy(fx->dmg_src.attack_origin,fx->xyz,3*sizeof(float));

		//FUN_0040b600_ball_hit_test_using_computed_radius		
		if(((BYTE(*)(float[3],float,int,void*,FLOAT*))0x40b600)
		(fx->dmg_src.attack_origin,
		 fx->dmg_src.radius*fx->scale,31, //31?
		 fx->dmg_src.source,SOM::L.hit_buf_pos)) //hit_buffer
		{
			(void*&)fx->dmg_src.target = SOM::L.target;

			int tt = SOM::L.target_type; //1 2 4 8 16			
		//	tt = tt==16?0:tt;
		//	fx->dmg_src.target_type = tt;

			som_SFX_42f1f0(fx,SOM::L.hit_buf_pos,SOM::L.hit_buf_look_vec,tt);

			fx->duration = 1;

			init in = {};
			in.scale = fx->scale;

			in.sfx = fx->sfx+1; //!!

			memcpy(in.xyz,SOM::L.hit_buf_pos,sizeof(in.xyz));
			if(dat.unk2[3]==1)
			{
				switch(tt)
				{
				case 1:
					
					memcpy(in.xyz,SOM::xyz,sizeof(in.xyz));
					break;

				case 2: case 4:
					
					memcpy(in.xyz,SOM::L.target,sizeof(in.xyz));
					break;
				}
			}
			if(dat.unk2[2]==0)
			{
				memcpy(in.look_vec,fx->const_vel,sizeof(in.xyz));
				Somvector::map(in.look_vec).unit<3>();
			}
			else
			{
				memcpy(in.look_vec,SOM::L.hit_buf_look_vec,sizeof(in.xyz));
			}
			in.duration = (int)dat.scale2;
			in.dmg_src = fx->dmg_src;
			((BYTE(*)(init*))0x42ea10)(&in);
		}
	}

	for(int i=3;i-->0;) fx->xyz[i]+=fx->const_vel[i];
	
	memcpy(fx->mdl->xyzuvw,fx->xyz,6*sizeof(float));

	if(!fx->mdl->advance()) fx->mdl->d = 1;

	fx->duration--;
}

static BYTE __cdecl som_SFX_46a_43c0b0_triplefang(sfx *fx, init *in)
{
	if(0||SOM::emu) 
	return ((BYTE(*)(sfx*,init*))0x43c0b0)(fx,in);

	auto *src = in->dmg_src.source; //CHEAT! //RECURSIVE
	if(!src)
	(DWORD&)in->dmg_src.source = 0x019c1a18;

	auto &dat = sfx_dat[in->sfx];

	som_SFX_basic_init(1,fx,in);

	//NOTICE! this is assigned to the mdl's pitch!!
	fx->incline = -in->look_vec[1];
	fx->yaw = atan2f(in->look_vec[2],in->look_vec[0])+M_PI_2;
	//fx->roll = 0.0f;

	fx->duration = 256; //???

	char *c = fx->c; float *f = (float*)c; //shorthand

	c[0x1a8] = dat.unk2[2];
	c[0x1a9] = 0;
	c[0x1aa] = 0;
	c[0x1ab] = 0;
	
	unsigned r = SOM::rng()&0x8000001f;	
	if((int)r<0) r = (r-1|0xffffffe0)+1;
	fx->randomizer = ((int)r-16)*-0.001f;
		
	f[0x13c/4] = (SOM::rng()%180+180)*0.01f;
	
	r = SOM::rng()&0x8000001f;	
	if((int)r<0) r = (r-1|0xffffffe0)+1;
	f[0x140/4] = ((int)r-16)*-0.001f;

	memset(c+0x14c,0x00,28);
	
	f[0x144/4] = (SOM::rng()%180+180)*0.01f;

	f[0x148/4] = dat.speed*0.03333334;

	memcpy(f+0x168/4,in->look_vec,3*sizeof(float));

	f[0x1f4/4] = 1.0f;
	f[0x1e4/4] = 1.0f;
	f[0x1d4/4] = 1.0f;
	f[0x1c4/4] = 1.0f;

	memcpy(f+0x168/4,in->look_vec,3*sizeof(float));

	f[0x1c0/4] = in->look_vec[0]; //???
	f[0x1d0/4] = in->look_vec[1]; //???
	f[0x1e0/4] = in->look_vec[2]; //???

	float m[4][4];
	fx->uvw[0]+=M_PI_2;
	Somvector::map(m).rotation<4,4,'xyz'>(fx->uvw);
	fx->uvw[0]-=M_PI_2;

	//NOTE#1 the original code does this with a 4x4
	//matrix with the top row set to 0,0,1,1 all 0s
	//float v[3] = {0,0,1};
	//Somvector::map<3>(v).premultiply<4>(m);	
	(float&)fx->c[0x1bc] = m[2][0];
	(float&)fx->c[0x1cc] = m[2][1];
	(float&)fx->c[0x1dc] = m[2][2];

	fx->uvw[1]-=M_PI_2;
	Somvector::map(m).rotation<4,4,'xyz'>(fx->uvw);
	fx->uvw[1]+=M_PI_2;

	//NOTE#2 same deal as before
	(float&)fx->c[0x1b8] = m[2][0];
	(float&)fx->c[0x1c8] = m[2][1];
	(float&)fx->c[0x1d8] = m[2][2]; 

	memcpy(fx->mdl->xyzuvw,fx->xyz,6*sizeof(float));	
	fx->mdl->fade = 1.0;
	fx->mdl->scale[0] = 
	fx->mdl->scale[1] = 
	fx->mdl->scale[2] = dat.scale*in->scale;	
	
	fx->mdl->d = 1; //set_animation_goal_frame (HACK)

	if(src) //2 extra bullets? //CHEAT //RECURSIVE
	{
		//init in2 = *in, in3 = *in;		
		//FUN_00449dc0_y_rotation_matrix
		//FUN_00449f10_matrix_vector_operation?_xform_normal?
	//	((void(*)(void*,float))0x449dc0)(m,0.3490659f);		
	//	((void(*)(void*,float*))0x449f10)(m,in->look_vec);
		SOM::rotate(in->look_vec,0,0.3490659f);
		in->dmg_src.source = 0; //CHEAT //RECURSIVE
		((BYTE(*)(init*))0x42ea10)(in);
		//FUN_00449dc0_y_rotation_matrix
		//FUN_00449f10_matrix_vector_operation?_xform_normal?
	//	((void(*)(void*,float))0x449dc0)(m,-0.3490659f*2);		
	//	((void(*)(void*,float*))0x449f10)(m,in->look_vec);
		SOM::rotate(in->look_vec,0,-0.3490659f*2);
		in->dmg_src.source = 0; //CHEAT //RECURSIVE
		((BYTE(*)(init*))0x42ea10)(in);
	}
	return 1;
}
static void __cdecl som_SFX_46b_43c5e0_triplefang(sfx *fx)
{
	if(0||SOM::emu) 
	return ((void(*)(sfx*))0x43c5e0)(fx);

	auto &dat = sfx_dat[fx->sfx]; //UNFINISHED

	if(!fx->duration) //terminate?
	return ((void(*)(sfx*))0x42f190)(fx);

	if(fx->duration<dat.unk2[1])
	{
		fx->mdl->fade = fx->duration*SOM::L.rate2;
	}
	else
	{
		memcpy(fx->dmg_src.attack_origin,fx->xyz,3*sizeof(float));

		//FUN_0040b600_ball_hit_test_using_computed_radius		
		if(((BYTE(*)(float[3],float,int,void*,FLOAT*))0x40b600)
		(fx->dmg_src.attack_origin,
		 fx->dmg_src.radius*fx->scale,31, //31?
		 fx->dmg_src.source,SOM::L.hit_buf_pos)) //hit_buffer
		{
			(void*&)fx->dmg_src.target = SOM::L.target;

			int tt = SOM::L.target_type; //1 2 4 8 16			
		//	tt = tt==16?0:tt;
		//	fx->dmg_src.target_type = tt;

			som_SFX_42f1f0(fx,SOM::L.hit_buf_pos,SOM::L.hit_buf_look_vec,tt);

			fx->duration = 1;

			init in = {};
			in.scale = fx->scale;
			in.sfx = fx->sfx+1;
			memcpy(in.xyz,SOM::L.hit_buf_pos,sizeof(in.xyz));
			if(dat.unk2[5]==0)
			{
				memcpy(in.look_vec,fx->const_vel,sizeof(in.xyz));
				Somvector::map(in.look_vec).unit<3>();
			}
			else
			{
				memcpy(in.look_vec,SOM::L.hit_buf_look_vec,sizeof(in.xyz));
			}
			
			in.duration = (int)dat.scale2;
			in.dmg_src = fx->dmg_src;
			((BYTE(*)(init*))0x42ea10)(&in);
		}
	}

	som_SFX_9b_431850_missile2(fx);	

	memcpy(fx->mdl->xyzuvw,fx->xyz,3*sizeof(float));
	
	if(!fx->mdl->advance()) fx->mdl->d = 1;

	fx->duration--;
}

static BYTE __cdecl som_SFX_100a_438080_explode2D(sfx *fx, init *in)
{
	if(0||SOM::emu) 
	return ((BYTE(*)(sfx*,init*))0x438080)(fx,in);

	auto &dat = sfx_dat[in->sfx];
	
	if(2!=sfx_ref[dat.model].type||X42EEB0) RETURN0

	fx->duration = dat.unk2[0];

	if(dat.unk2[3]==0)
	{	
		memcpy(fx->xyz,in->xyz,3*sizeof(float));
	}
	else if(dat.unk2[3]==1) //???
	{
		memcpy(fx->xyz,in->dmg_src.explosion_origin,3*sizeof(float));
	}
	//if(in->duration==2.242078e-44) //????
	if(in->duration==0x10) //????
	{
		assert(0);
		//NOTE: ==0x10 makes no sense at all???
		for(int i=3;i-->0;)
		fx->xyz[i] = in->look_vec[i]*dat.unk5[1]+fx->xyz[i];		
	}

	if(dat.scale>=0.0) //???
	{
		fx->uvw[2] = dat.scale2*0.01745329f;
	}
	else fx->uvw[2] = (float)(SOM::rng()%360)*0.01745329f;

	return 1;
}
static void __cdecl som_SFX_100b_438180_explode2D(sfx *fx)
{
	if(0||SOM::emu) 
	return ((void(*)(sfx*))0x438180)(fx);

	auto &dat = sfx_dat[fx->sfx]; //UNFINISHED

	if(!fx->duration) //terminate?
	return ((void(*)(sfx*))0x42f190)(fx);

	//FUN_00401070_get_XYZ_thiscall?_4c2358_view_matrix_only?
	void *vmo = (void*)0x4c2358; float x,y,z;
	((BYTE(__thiscall*)(void*,float*,float*,float*))0x401070)(vmo,&x,&y,&z);
	x-=fx->xyz[0]; 
	y-=fx->xyz[1];
	z-=fx->xyz[2];		
	fx->uvw[0] = -atan2f(y,sqrtf(z*z+x*x));
	fx->uvw[1] = +atan2f(z,x)+M_PI_2;

	//se = FUN_0042f7a0_block_alloc_image_based_sfx
	auto se = som_MDL_42f7a0(1,dat.unk2[2]);
	if(!se) return;
	
	float f1 = (float)(dat.unk2[0]-fx->duration);	
	float f2 = (float)dat.unk2[0];
	float s1 = ((dat.radius-dat.width)*0.5*f1/f2+dat.width)*fx->scale;
	float s2 = ((dat.speed-dat.height)*0.5*f1/f2+dat.width)*fx->scale;	
	se->vdata[0].x = -s1;
	se->vdata[1].x = s1;
	se->vdata[2].x = s1;
	se->vdata[3].x = -s1;
	se->vdata[0].y = s2;
	se->vdata[1].y = s2;
	se->vdata[2].y = -s2;
	se->vdata[3].y = -s2;	
	se->vdata[0].s = 0.0;
	se->vdata[1].s = 1.0;
	se->vdata[2].s = 1.0;
	se->vdata[3].s = 0.0;
	se->vdata[0].t = 0.0;
	se->vdata[1].t = 0.0;
	se->vdata[2].t = 1.0;	
	se->vdata[3].t = 1.0;	

	DWORD color = (DWORD)dat.unk5[0];
	if(fx->duration<dat.unk2[1])
	{
		color = (fx->duration*color)/dat.unk2[1];
	}
	DWORD diffuse; if(dat.unk2[2]==2) //rgb blend?
	{
		diffuse = ((color|0xffffff00)<<8|color)<<8|color;
	}
	else diffuse = color<<24|0xffffff; //alpha

	auto *p = se->vdata;
	for(int i=4;i-->0;p++)
	{
		p->z = 0; p->diffuse = diffuse;
	}		

	se->texture = fx->txr;

	float m[4][4];
	Somvector::map(m).rotation<3,4,'xyz'>(fx->uvw);
	Somvector::map(m[3]).copy<3>(fx->xyz).se<3>() = 1;

	p = se->vdata;
	for(int i=4;i-->0;p++)
	{
		auto *v = &p->x; //YUCK
		Somvector::map<3>(v).premultiply<4>(m);
	}

	fx->uvw[2] = //FUN_0044cc20_fmod_pi?
	((FLOAT(__cdecl*)(FLOAT))0x44cc20)(fx->uvw[2]+dat.scale2*0.01745329);

	fx->duration--;
}

static BYTE __cdecl som_SFX_101a_438450_explode3D(sfx *fx, init *in)
{
	if(0||SOM::emu) 
	return ((BYTE(*)(sfx*,init*))0x438450)(fx,in);

	auto &dat = sfx_dat[in->sfx];

	if(1!=sfx_ref[dat.model].type||X42EEB0) RETURN0

	fx->duration = dat.unk2[0];
	
	//if(in->duration==2.242078e-44)
	if(in->duration==0x16)
	{
		for(int i=3;i-->0;)
		fx->xyz[i] = in->look_vec[i]*dat.unk5[1]+fx->xyz[i];
		goto LAB_004385ad;
	}
	//if(in->duration==1.121039e-44)
	if(in->duration==0x8)
	{
		if(dat.unk2[3]!=0)
		{
			if(dat.unk2[3]!=1)
			{
				memcpy(fx->xyz,in->xyz,3*sizeof(float));
				goto LAB_004385ad;
			}
			LAB_004384f4:
			memcpy(fx->xyz,in->dmg_src.explosion_origin,3*sizeof(float));
			goto LAB_004385ad;
		}
	}
	else if(dat.unk2[3]!=0)
	{
		if(dat.unk2[3]==1) goto LAB_004384f4;
		if(dat.unk2[3]==2)
		{
		//	if(in->duration==1.401298e-45)
			if(in->duration==1)
			{
				memcpy(fx->xyz,SOM::xyz,3*sizeof(float));
			}
		//	else if(in->duration==2.802597e-45)
			else if(in->duration==2)
			{
				//FUN_00441600_get_stored_CP
				((BYTE(__cdecl*)(void*,int,FLOAT*))0x441600)
				((SOM::MDL*)(*(int*)&in->dmg_src+0x68),0xff,fx->xyz);
			}
		//	else if(in->duration==5.605194e-45)
			else if(in->duration==4)
			{
				//FUN_00441600_get_stored_CP?
				((BYTE(__cdecl*)(void*,int,FLOAT*))0x441600)
				((SOM::MDL*)(*(int *)&in->dmg_src+0x60),0xff,fx->xyz);
			}
			goto LAB_004385ad;
		}
	}
	memcpy(fx->xyz,in->xyz,3*sizeof(float));
	
	LAB_004385ad:
	
	memcpy(fx->mdl->xyzuvw,fx->xyz,6*sizeof(float));
	
	fx->mdl->fade = 1.0;
	fx->mdl->scale[0] =
	fx->mdl->scale[1] = 
	fx->mdl->scale[2] = dat.width;

	fx->randomizer = (dat.height-dat.width)/(float)dat.unk2[0];

	return 1;
}
static void __cdecl som_SFX_101b_438630_explode3D(sfx *fx)
{
	if(0||SOM::emu) 
	return ((void(*)(sfx*))0x438630)(fx);

	auto &dat = sfx_dat[fx->sfx]; //UNFINISHED

	if(!fx->duration) //terminate?
	return ((void(*)(sfx*))0x42f190)(fx);

	if(dat.unk2[4]!=0)
	{
		//FUN_00401070_get_XYZ_thiscall?_4c2358_view_matrix_only?
		void *vmo = (void*)0x4c2358; float x,y,z;
		((BYTE(__thiscall*)(void*,float*,float*,float*))0x401070)(vmo,&x,&y,&z);
		x-=fx->xyz[0]; 
		y-=fx->xyz[1];
		z-=fx->xyz[2];		
		fx->uvw[0] = -atan2f(y,sqrtf(z*z+x*x));
		fx->uvw[1] = +atan2f(z,x)+M_PI_2;
	}

	fx->mdl->fade = fx->duration*SOM::L.rate2;
	fx->mdl->scale[0] = 
	fx->mdl->scale[1] = 
	fx->mdl->scale[2] = fx->randomizer+fx->mdl->scale[0];

	fx->duration--;
}

static BYTE __cdecl som_SFX_128a_438b10(sfx *fx, init *in)
{
	if(0||SOM::emu) 
	return ((BYTE(*)(sfx*,init*))0x438b10)(fx,in);

	auto &dat = sfx_dat[in->sfx];
	
	if(1!=sfx_ref[dat.model].type||X42EEB0) RETURN0

	if(!in->mdl_cp) //???
	{
		if(in->mdo_cp)
		{
			//FUN_004461d0_param1_is_MDO_instance?_MDO_cp?
			((BYTE(__cdecl*)(SOM::MDO*,int,FLOAT*))0x441600)(in->mdo_cp,in->cp,fx->xyz);
		}
		else memset(fx->xyz,0x00,3*sizeof(float));
	}
	else
	{
		//FUN_00441600_get_stored_CP?
		((BYTE(__cdecl*)(SOM::MDL*,int,FLOAT*))0x441600)(in->mdl_cp,in->cp,fx->xyz);
	}
	fx->uvw[0] = 0.0;	
	fx->uvw[1] = SOM::rng()%360*M_PI/180;
	fx->uvw[2] = 0.0;

	fx->duration = dat.unk2[1]+dat.unk2[0];

	fx->uc30[0] = dat.unk2[0];
	fx->uc30[1] = dat.unk2[1];
	fx->const_vel[1] = dat.width;
	fx->const_vel[2] = dat.height;

	//*(float *)&fx->field_0x84 = dat.radius;
	//*(float *)&fx->field_0x88 = dat.speed;
	//*(float *)&fx->field_0x8c = dat.scale;
	//*(float *)&fx->field_0x90 = dat.scale2;
	//*(float *)&fx->field_0x94 = dat.unk5[0];
	memcpy(fx->c+0x84,&dat.radius,5*sizeof(float));

	memcpy(fx->mdl->xyzuvw,fx->xyz,6*sizeof(float));

	fx->mdl->scale[0] = 
	fx->mdl->scale[1] = 
	fx->mdl->scale[2] = fx->scale*fx->const_vel[2];

	fx->mdl->fade = dat.scale; //fx->field_0x8c?

	fx->mdl->d = 1; //FUN_004414c0_set_animation_goal_frame

	return 1;
}
static void __cdecl som_SFX_128b_438ce0(sfx *fx)
{
	if(0||SOM::emu) 
	return ((void(*)(sfx*))0x438ce0)(fx);

	auto &dat = sfx_dat[fx->sfx]; //UNFINISHED

	if(!fx->duration) //terminate?
	return ((void(*)(sfx*))0x42f190)(fx);

	fx->mdl->xyzuvw[4] = fx->uvw[1]+=fx->const_vel[1];

	BYTE u7 = fx->uc30[1];
	BYTE u2 = fx->duration;
	
	float f3,f4; if(u7<u2)
	{
		f4 = (float)(u7-u2+u7);
		float f5 = (float)fx->uc30[0];
	//	f3 = ((fx->field_0x90-fx->field_0x8c)*f4)/f5+fx->field_0x8c;
	//	f4 = ((fx->field_0x84-fx->const_vel[2])*f4)/f5+fx->const_vel[2];
		f3 = ((dat.scale2-dat.scale)*f4)/f5+dat.scale;
		f4 = ((dat.radius-fx->const_vel[2])*f4)/f5+fx->const_vel[2];
	}
	else //???
	{
	//	f3 = ((fx->field_0x94-fx->field_0x90)*(u7-u2))/u7+fx->field_0x90;
	//	f4 = ((fx->field_0x88-fx->field_0x84)*(u7-u2))/u7+fx->field_0x84;
		f3 = ((dat.unk5[0]-dat.scale2)*(u7-u2))/u7+dat.scale2;
		f4 = ((dat.speed-dat.radius)*(u7-u2))/u7+dat.radius;
	}
	fx->mdl->fade = f3;
	
	fx->mdl->scale[0] = 
	fx->mdl->scale[1] = 	
	fx->mdl->scale[2] = f4*fx->scale;

	if(!fx->mdl->advance()) fx->mdl->d = 1;

	fx->duration--;
}

static BYTE __cdecl som_SFX_129a_438e10(sfx *fx, init *in)
{
	if(0||SOM::emu) 
	return ((BYTE(*)(sfx*,init*))0x438e10)(fx,in);

	auto &dat = sfx_dat[in->sfx];
	
	if(1!=sfx_ref[dat.model].type||X42EEB0) RETURN0

	fx->cp1 = fx->cp2 = 0; //EXTENSION //LAB_00438eac?

	if(!in->mdl_cp)
	{
		if(!in->mdo_cp) goto LAB_00438eac; //???

		//FUN_00446230_sfx_sub?_get_stored_CP_pointer?_type1?_MDO?
		(void*&)fx->cp1 =
		((void*(__cdecl*)(SOM::MDO*,int))0x446230)(in->mdo_cp,in->cp);
		(void*&)fx->cp2 =
		((void*(__cdecl*)(SOM::MDO*,int))0x446230)(in->mdo_cp,in->cp2);
	}
	else
	{
		//FUN_00441680_sfx_sub?_get_stored_CP_pointer?_type2?_MDL?
		(void*&)fx->cp1 =
		((void*(__cdecl*)(SOM::MDL*,int))0x441680)(in->mdl_cp,in->cp);
		(void*&)fx->cp2 =
		((void*(__cdecl*)(SOM::MDL*,int))0x441680)(in->mdl_cp,in->cp2);
	}
	LAB_00438eac: //???

	fx->mdl->xyzuvw[3] = fx->uvw[0] = 0.0;
	fx->mdl->xyzuvw[4] = fx->uvw[1] = 0.0;
	fx->mdl->xyzuvw[5] = fx->uvw[2] = 0.0;
	
	fx->duration = dat.unk2[0];

	fx->mdl->scale[0] =
	fx->mdl->scale[1] =
	fx->mdl->scale[2] = fx->scale;

	fx->mdl->fade = 1.0;

	fx->mdl->d = 1; //FUN_004414c0_set_animation_goal_frame

	return 1;
}
static void __cdecl som_SFX_129b_438f30(sfx *fx)
{
		assert(!fx); //what uses this?

	if(1||SOM::emu) 
	return ((void(*)(sfx*))0x438f30)(fx);

	auto &dat = sfx_dat[fx->sfx]; //UNFINISHED

	if(!fx->duration) //terminate?
	return ((void(*)(sfx*))0x42f190)(fx);

		/*
	pfVar11 = (float *)fx->const_vel[0];
	uVar13 = (uint)fx->sfx;
	pfVar4 = (float *)fx->const_vel[1];
	iVar16 = uVar13*0x30;
	local_c = *pfVar4-*pfVar11;
	local_8 = pfVar4[1]-pfVar11[1];
	local_4 = pfVar4[2]-pfVar11[2];
	FUN_004466c0_normalize(&local_c,&local_8,&local_4);
	if(dat.unk2[3]==0)
	{
		pfVar11 = (float *)fx->const_vel[0];
	}
	else
	{
		pfVar11 = (float *)fx->const_vel[1];
	}
	fx->xyz[0] = *pfVar11;
	fx->xyz[1] = pfVar11[1];
	fx->xyz[2] = pfVar11[2];
	fVar17 = FUN_0044fde0();
	fx->uvw[0] = (float)fVar17;
	fx->uvw[2] = 0.0;
	fVar17 = fpatan(local_4,local_c);
	fx->uvw[1] = (float)(fVar17+1.570796);
	uVar15 = (uint)dat.unk2[0];
	uVar14 = (uint)dat.unk2[1];
	uVar5 = fx->duration;
	if(uVar15-uVar14<uVar5)
	{
		fVar8 = (float)uVar14;
		iVar12 = 0;
		fVar9 = (float)(ulonglong)((uVar14-uVar15)+uVar5);
	}
	else
	{
		uVar13 = (uint)dat.unk2[2];
		if(uVar13<uVar5)
		{
			fVar8 = (float)uVar15-(float)(uVar13+uVar14);
			fVar9 = (float)(ulonglong)(uVar5-uVar14);
			iVar12 = 1;
		}
		else
		{
			fVar8 = (float)uVar13;
			fVar9 = (float)(ulonglong)uVar5;
			iVar12 = 2;
		}
	}
	iVar12 = iVar12+4;
	psVar6 = fx->mdl;
	fVar7 = ((*(float *)(iVar16+0x1c91d2c+iVar12*4)-
			 *(float *)(iVar16+0x1c91d28+iVar12*4))*(fVar8-fVar9))/fVar8+
		*(float *)(iVar16+0x1c91d28+iVar12*4);
	fVar1 = *(float *)(iVar16+0x1c91d3c+iVar12*4);
	fVar2 = *(float *)(iVar16+0x1c91d38+iVar12*4);
	fVar3 = *(float *)(iVar16+0x1c91d38+iVar12*4);
	psVar6->xyzuvw[0] = fx->xyz[0];
	psVar6->xyzuvw[1] = fx->xyz[1];
	psVar6->xyzuvw[2] = fx->xyz[2];
	psVar6 = fx->mdl;
	psVar6->xyzuvw[3] = fx->uvw[0];
	psVar6->xyzuvw[4] = fx->uvw[1];
	psVar6->xyzuvw[5] = fx->uvw[2];
	fx->mdl->scale[2] = fVar7;
	fx->mdl->scale[1] = fVar7;
	fx->mdl->scale[0] = fVar7;
	fx->mdl->fade = fVar8/((fVar1-fVar2)*(fVar8-fVar9))+fVar3;
	cVar10 = FUN_00441510_advance_MDL(fx->mdl);
	if(cVar10=='\0')
	{
		FUN_004414c0_set_animation_goal_frame(fx->mdl,1);
	}
		*/

	fx->duration--;
}

static BYTE __cdecl som_SFX_131a_439450(sfx *fx, init *in)
{
	if(1||SOM::emu) 
	return ((BYTE(*)(sfx*,init*))0x439450)(fx,in);

	auto &dat = sfx_dat[in->sfx];
	
	assert(0!=sfx_ref[dat.model].type); //2 or 0???
	if(2!=sfx_ref[dat.model].type||X42EEB0) RETURN0

	fx->cp1 = fx->cp2 = 0; //EXTENSION //LAB_004394f1?

	if(!in->mdl_cp)
	{
		if(!in->mdo_cp) goto LAB_004394f1; //???

		//FUN_00446230_sfx_sub?_get_stored_CP_pointer?_type1?_MDO?
		(void*&)fx->cp1 =
		((void*(__cdecl*)(SOM::MDO*,int))0x446230)(in->mdo_cp,in->cp);
		(void*&)fx->cp2 =
		((void*(__cdecl*)(SOM::MDO*,int))0x446230)(in->mdo_cp,in->cp2);
	}
	else
	{
		//FUN_00441680_sfx_sub?_get_stored_CP_pointer?_type2?_MDL?
		(void*&)fx->cp1 =
		((void*(__cdecl*)(SOM::MDL*,int))0x441680)(in->mdl_cp,in->cp);
		(void*&)fx->cp2 =
		((void*(__cdecl*)(SOM::MDL*,int))0x441680)(in->mdl_cp,in->cp2);
	}
	LAB_004394f1: //???

	fx->duration = dat.unk2[0]+2;

	return 1;
}
static void __cdecl som_SFX_131b_439510(sfx *fx)
{
		assert(!fx); //what uses this?

	if(1||SOM::emu) 
	return ((void(*)(sfx*))0x439510)(fx);

	auto &dat = sfx_dat[fx->sfx]; //UNFINISHED

	if(!fx->duration) //terminate?
	return ((void(*)(sfx*))0x42f190)(fx);

		/*
	iVar30 = uVar33-dat.unk2[0];
	if(uVar33==dat.unk2[0])
	{
		puVar4 = (undefined4 *)fx->const_vel[0];
		fVar13 = *(float *)&fx->field_0x98-fx->const_vel[2];
		puVar1 = &fx->field_0xb0;
		fVar15 = *(float *)&fx->field_0x9c-*(float *)&fx->field_0x84;
		*(undefined4 *)puVar1 = *puVar4;
		*(undefined4 *)&fx->field_0xb4 = puVar4[1];
		fVar16 = *(float *)&fx->field_0xa0-*(float *)&fx->field_0x88;
		*(undefined4 *)&fx->field_0xb8 = puVar4[2];
		puVar4 = (undefined4 *)fx->const_vel[1];
		*(undefined4 *)&fx->field_0xbc = *puVar4;
		fVar17 = *(float *)puVar1-*(float *)&fx->field_0x98;
		*(undefined4 *)&fx->field_0xc0 = puVar4[1];
		*(undefined4 *)&fx->field_0xc4 = puVar4[2];
		fVar18 = *(float *)&fx->field_0xb4-*(float *)&fx->field_0x9c;
		fVar41 = *(float *)&fx->field_0x98;
		fVar5 = *(float *)&fx->field_0x9c;
		fVar6 = *(float *)&fx->field_0xa0;
		fVar19 = *(float *)&fx->field_0xb8-*(float *)&fx->field_0xa0;
		fVar7 = *(float *)puVar1-fVar41;
		fVar9 = *(float *)&fx->field_0xb4-fVar5;
		fVar8 = *(float *)&fx->field_0xb8-fVar6;
		fVar20 = SQRT(fVar7*fVar7+fVar9*fVar9+fVar8*fVar8);
		fVar14 = fVar20/SQRT(fVar13*fVar13+fVar15*fVar15+fVar16*fVar16);
		puVar2 = &fx->field_0xa4;
		fVar7 = *(float *)puVar2;
		fVar8 = *(float *)&fx->field_0xa8;
		fVar9 = *(float *)&fx->field_0xac;
		pfVar38 = (float *)fx->const_vel[1];
		fVar10 = *pfVar38;
		fVar11 = pfVar38[1];
		fVar12 = pfVar38[2];
		fVar20 = fVar20/SQRT(fVar17*fVar17+fVar18*fVar18+fVar19*fVar19);
		fVar21 = *(float *)puVar2-*(float *)&fx->field_0x8c;
		fVar22 = *(float *)&fx->field_0xa8-*(float *)&fx->field_0x90;
		fVar23 = *(float *)&fx->field_0xac-*(float *)&fx->field_0x94;
		fVar24 = *(float *)&fx->field_0xbc-*(float *)puVar2;
		fVar25 = *(float *)&fx->field_0xc0-*(float *)&fx->field_0xa8;
		fVar26 = *(float *)&fx->field_0xc4-*(float *)&fx->field_0xac;
		fVar27 = fVar10-fVar7;
		fVar29 = fVar11-fVar8;
		fVar28 = fVar12-fVar9;
		fVar27 = SQRT(fVar27*fVar27+fVar29*fVar29+fVar28*fVar28);
		fx->const_vel[2] = fVar41;
		*(float *)&fx->field_0x84 = fVar5;
		*(float *)&fx->field_0x88 = fVar6;
		*(float *)&fx->field_0x8c = fVar14*fVar13;
		*(float *)&fx->field_0x90 = fVar14*fVar15;
		*(float *)&fx->field_0x94 = fVar14*fVar16;
		*(undefined4 *)&fx->field_0x98 = *(undefined4 *)puVar1;
		*(undefined4 *)&fx->field_0x9c = *(undefined4 *)&fx->field_0xb4;
		*(undefined4 *)&fx->field_0xa0 = *(undefined4 *)&fx->field_0xb8;
		*(float *)puVar2 = fVar20*fVar17;
		*(float *)&fx->field_0xa8 = fVar20*fVar18;
		*(float *)&fx->field_0xac = fVar20*fVar19;
		*(float *)puVar1 = fVar7;
		*(float *)&fx->field_0xb4 = fVar8;
		*(float *)&fx->field_0xb8 = fVar9;
		fVar41 = fVar27/SQRT(fVar21*fVar21+fVar22*fVar22+fVar23*fVar23);
		*(float *)&fx->field_0xbc = fVar41*fVar21;
		*(float *)&fx->field_0xc0 = fVar41*fVar22;
		*(float *)&fx->field_0xc8 = fVar10;
		*(float *)&fx->field_0xc4 = fVar41*fVar23;
		*(float *)&fx->field_0xcc = fVar11;
		fVar27 = fVar27/SQRT(fVar24*fVar24+fVar25*fVar25+fVar26*fVar26);
		*(float *)&fx->field_0xd0 = fVar12;
		*(float *)&fx->field_0xd4 = fVar27*fVar24;
		*(float *)&fx->field_0xd8 = fVar27*fVar25;
		*(float *)&fx->field_0xdc = fVar27*fVar26;
		iVar30 = __ftol();
		*(int *)&fx->field_0xf0 = iVar30;
		if(iVar30==0)
		{
			*(undefined4 *)&fx->field_0xf0 = 1;
		}
	}
	else
	{
		if(iVar30==1)
		{
			puVar4 = (undefined4 *)fx->const_vel[0];
			*(undefined4 *)&fx->field_0x98 = *puVar4;
			*(undefined4 *)&fx->field_0x9c = puVar4[1];
			*(undefined4 *)&fx->field_0xa0 = puVar4[2];
			puVar4 = (undefined4 *)fx->const_vel[1];
			*(undefined4 *)&fx->field_0xa4 = *puVar4;
			*(undefined4 *)&fx->field_0xa8 = puVar4[1];
			*(undefined4 *)&fx->field_0xac = puVar4[2];
			fx->duration = fx->duration-1;
			return;
		}
		if(iVar30==2)
		{
			pfVar38 = (float *)fx->const_vel[0];
			fx->const_vel[2] = *pfVar38;
			*(float *)&fx->field_0x84 = pfVar38[1];
			*(float *)&fx->field_0x88 = pfVar38[2];
			puVar4 = (undefined4 *)fx->const_vel[1];
			*(undefined4 *)&fx->field_0x8c = *puVar4;
			*(undefined4 *)&fx->field_0x90 = puVar4[1];
			*(undefined4 *)&fx->field_0x94 = puVar4[2];
			fx->duration = fx->duration-1;
			return;
		}
	}
	psVar31 = FUN_0042f7a0_block_alloc_image_based_sfx(*(int *)&fx->field_0xf0<<1,2);
	if(psVar31!=(struct SOM_scene_picture *)0x0)
	{
		iVar30 = *(int *)&fx->field_0xf0+-1;
		iVar32 = __ftol();
		if(fx->duration<(uint)dat.unk2[1])
		{
			local_68 = __ftol();
			local_40 = __ftol();
			local_48 = __ftol();
			fVar41 = (float)__ftol();
			local_70 = (float)__ftol();
			local_50 = __ftol();
			uStack_64 = __ftol();
			local_3c = __ftol();
			local_44 = __ftol();
			local_78 = __ftol();
			local_6c = __ftol();
			local_4c = __ftol();
			uVar33 = iVar32<<8;
			psVar31->vdata[0].diffuse = ((uVar33|(uint)fVar41)<<8|(uint)local_70)<<8|local_50;
			psVar31->vdata[1].diffuse = ((uVar33|local_68)<<8|local_40)<<8|local_48;
			psVar31[iVar30].vdata[2].diffuse = ((uVar33|uStack_64)<<8|local_3c)<<8|local_44;
			psVar31[iVar30].vdata[3].diffuse = ((uVar33|local_78)<<8|local_6c)<<8|local_4c;
		}
		else
		{
			uVar33 = __ftol();
			uVar34 = __ftol();
			uVar35 = __ftol();
			fVar41 = (float)(((uVar33|iVar32<<8)<<8|uVar34)<<8|uVar35);
			uVar33 = __ftol();
			uVar34 = __ftol();
			uVar35 = __ftol();
			local_70 = (float)(((uVar33|iVar32<<8)<<8|uVar34)<<8|uVar35);
			psVar31->vdata[0].diffuse = (dword)local_70;
			psVar31->vdata[1].diffuse = (dword)fVar41;
			psVar31[iVar30].vdata[2].diffuse = (dword)fVar41;
			psVar31[iVar30].vdata[3].diffuse = (dword)local_70;
		}
		psVar36 = psVar31+iVar30;
		fVar6 = (float)(uint)dat.unk2[0];
		uVar33 = (uint)dat.unk2[0]-fx->duration;
		fVar5 = (float)(ulonglong)(uVar33+1)/fVar6;
		psVar31->vdata[0].x = fx->const_vel[2];
		psVar31->vdata[0].y = *(float *)&fx->field_0x84;
		psVar31->vdata[0].z = *(float *)&fx->field_0x88;
		psVar31->vdata[0].t = 0.9;
		psVar31->vdata[0].s = fVar5;
		psVar31->vdata[1].x = *(float *)&fx->field_0xb0;
		psVar31->vdata[1].y = *(float *)&fx->field_0xb4;
		psVar31->vdata[1].z = *(float *)&fx->field_0xb8;
		psVar31->vdata[1].s = fVar5;
		psVar31->vdata[1].t = 0.1;
		psVar36->vdata[2].x = *(float *)&fx->field_0xc8;
		psVar36->vdata[2].y = *(float *)&fx->field_0xcc;
		psVar36->vdata[2].z = *(float *)&fx->field_0xd0;
		fVar6 = fVar5-(float)(ulonglong)uVar33/fVar6;
		fVar7 = fVar5-fVar6;
		psVar36->vdata[2].s = fVar7;
		psVar36->vdata[2].t = 0.1;
		psVar36->vdata[3].x = *(float *)&fx->field_0x98;
		psVar36->vdata[3].y = *(float *)&fx->field_0x9c;
		psVar36->vdata[3].z = *(float *)&fx->field_0xa0;
		psVar36->vdata[3].s = fVar7;
		psVar36->vdata[3].t = 0.9;
		local_cc = *(int *)&fx->field_0xf0;
		local_d0 = 1;
		if(1<local_cc)
		{
			local_c0 = (struct LVERTEX *)local_48;
			local_c4 = (struct LVERTEX *)local_68;
			local_54 = local_40;
			local_58 = local_50;
			local_60 = local_70;
			pfVar38 = &psVar31[1].vdata[0].z;
			local_5c = fVar41;
			do
			{
				fVar11 = (float)local_d0/(float)local_cc;
				fVar7 = fVar11-1.0;
				fVar8 = (fVar11+fVar11+1.0)*fVar7*fVar7;
				fVar10 = (3.0-(fVar11+fVar11))*fVar11*fVar11;
				fVar9 = fVar7*fVar7*fVar11;
				fVar7 = fVar7*fVar11*fVar11;
				((struct LVERTEX *)(pfVar38+-2))->x =
					fVar10**(float *)&fx->field_0x98+
					fVar8*fx->const_vel[2]+
					fVar7**(float *)&fx->field_0xa4+fVar9**(float *)&fx->field_0x8c;
				pfVar38[-1] = fVar8**(float *)&fx->field_0x84+
					fVar9**(float *)&fx->field_0x90+
					fVar7**(float *)&fx->field_0xa8+
					fVar10**(float *)&fx->field_0x9c;
				*pfVar38 = fVar7**(float *)&fx->field_0xac+
					fVar10**(float *)&fx->field_0xa0+
					fVar9**(float *)&fx->field_0x94+fVar8**(float *)&fx->field_0x88;
				pfVar38[6] = fVar8**(float *)&fx->field_0xb0+
					fVar9**(float *)&fx->field_0xbc+
					fVar7**(float *)&fx->field_0xd4+
					fVar10**(float *)&fx->field_0xc8;
				pfVar38[7] = fVar8**(float *)&fx->field_0xb4+
					fVar10**(float *)&fx->field_0xcc+
					fVar9**(float *)&fx->field_0xc0+fVar7**(float *)&fx->field_0xd8
					;
				pfVar38[8] = fVar8**(float *)&fx->field_0xb8+
					fVar9**(float *)&fx->field_0xc4+
					fVar10**(float *)&fx->field_0xd0+
					fVar7**(float *)&fx->field_0xdc;
				fVar7 = fVar5-fVar11*fVar6;
				pfVar38[0xc] = fVar7;
				pfVar38[4] = fVar7;
				pfVar38[5] = 0.9;
				pfVar38[0xd] = 0.1;
				if(fx->duration<(uint)dat.unk2[1])
				{
					uVar33 = *(uint *)&fx->field_0xf0;
					iVar30 = uVar33-local_d0;
					pfVar38[2] = (float)(((iVar30*local_6c)/uVar33+(uint)local_60/uVar33)*0x100|
										 ((iVar30*local_78)/uVar33+(uint)local_5c/uVar33)*0x10000|
										 (iVar30*local_4c)/uVar33+local_58/uVar33|iVar32<<0x18);
					pfVar38[10] = (float)(((iVar30*local_3c)/uVar33+local_54/uVar33)*0x100|
										  ((iVar30*uStack_64)/uVar33+(uint)local_c4/uVar33)*0x10000
										  |(iVar30*local_44)/uVar33+(uint)local_c0/uVar33|
										 iVar32<<0x18);
				}
				else
				{
					pfVar38[2] = local_70;
					pfVar38[10] = fVar41;
				}
				pfVar39 = pfVar38+6;
				pfVar42 = pfVar38+-0x16;
				for(iVar30 = 8; iVar30!=0; iVar30 = iVar30+-1)
				{
					*pfVar42 = *pfVar39;
					pfVar39 = pfVar39+1;
					pfVar42 = pfVar42+1;
				}
				psVar40 = (struct LVERTEX *)(pfVar38+-2);
				pfVar39 = pfVar38+-0xe;
				for(iVar30 = 8; iVar30!=0; iVar30 = iVar30+-1)
				{
					*pfVar39 = psVar40->x;
					psVar40 = (struct LVERTEX *)&psVar40->y;
					pfVar39 = pfVar39+1;
				}
				pfVar38[-0x12] = pfVar38[10];
				pfVar38[-10] = pfVar38[2];
				local_60 = (float)((int)local_60+(int)local_70);
				local_5c = (float)((int)local_5c+(int)fVar41);
				local_58 = local_58+local_50;
				local_54 = local_54+local_40;
				local_c4 = (struct LVERTEX *)((int)local_c4+local_68);
				local_cc = *(int *)&fx->field_0xf0;
				local_d0 = local_d0+1;
				local_c0 = (struct LVERTEX *)((int)local_c0+local_48);
				pfVar38 = pfVar38+0x24;
			} while(local_d0<local_cc);
		}
		local_d0 = 0;
		if(0<*(int *)&fx->field_0xf0)
		{
			local_c4 = psVar31->vdata+3;
			do
			{
				iVar30 = 0;
				local_c0 = local_c4;
				do
				{
					psVar40 = local_c0;
					pfVar38 = (float *)((int)&psVar31[*(int *)&fx->field_0xf0+local_d0].vdata[0].x+
									   iVar30);
					for(iVar32 = 8; iVar32!=0; iVar32 = iVar32+-1)
					{
						*pfVar38 = psVar40->x;
						psVar40 = (struct LVERTEX *)&psVar40->y;
						pfVar38 = pfVar38+1;
					}
					iVar30 = iVar30+0x20;
					local_c0 = local_c0+-1;
				} while(iVar30<0x80);
				local_d0 = local_d0+1;
				local_c4 = (struct LVERTEX *)((int)(local_c4+4)+0x10);
			} while(local_d0<*(int *)&fx->field_0xf0);
		}
		iVar30 = 0;
		if(0<*(int *)&fx->field_0xf0<<1)
		{
			puVar37 = &psVar31->texture;
			do
			{
				*puVar37 = fx->txr;
				iVar30 = iVar30+1;
				puVar37 = puVar37+0x48;
			} while(iVar30<*(int *)&fx->field_0xf0*2);
		}
		*/

	fx->duration--;
}

static BYTE __cdecl som_SFX_132a_43a1e0_candle(sfx *fx, init *in)
{
	if(0||SOM::emu) 
	return ((BYTE(*)(sfx*,init*))0x43a1e0)(fx,in);

	auto &dat = sfx_dat[in->sfx];
	
	if(2!=sfx_ref[dat.model].type||X42EEB0) RETURN0
	
	fx->cp1 = fx->cp2 = 0; //EXTENSION //LAB_0043a28c?

	if(!in->mdl_cp)
	{
		if(!in->mdo_cp) goto LAB_0043a28c; //???

		//FUN_00446230_sfx_sub?_get_stored_CP_pointer?_type1?_MDO?
		(void*&)fx->cp1 =
		((void*(__cdecl*)(SOM::MDO*,int))0x446230)(in->mdo_cp,in->cp);
		(void*&)fx->cp2 =
		((void*(__cdecl*)(SOM::MDO*,int))0x446230)(in->mdo_cp,in->cp2);
	}
	else
	{
		//FUN_00441680_sfx_sub?_get_stored_CP_pointer?_type2?_MDL?
		(void*&)fx->cp1 =
		((void*(__cdecl*)(SOM::MDL*,int))0x441680)(in->mdl_cp,in->cp);
		(void*&)fx->cp2 =
		((void*(__cdecl*)(SOM::MDL*,int))0x441680)(in->mdl_cp,in->cp2);
	}
	LAB_0043a28c: //???

	fx->duration = dat.unk2[0];

	if(fx->cp1) memcpy(fx->xyz,fx->cp1,3*sizeof(float));

	//aligning with a CP pair??
	if(fx->cp2&&dat.unk2[2]==3)
	{
		auto &pf3 = *fx->cp2;
		float x = (*fx->cp2)[0]-fx->xyz[0];
		float y = (*fx->cp2)[1]-fx->xyz[1];
		float z = (*fx->cp2)[2]-fx->xyz[2];
		//FUN_004466c0_normalize		
		((void(__cdecl*)(FLOAT*,FLOAT*,FLOAT*))0x4466c0)(&x,&y,&z);
		
		fx->uvw[0] = atan2f(y,sqrtf(x*x+z*z));
		fx->uvw[1] = //FUN_0044cc20_fmod_pi?
		((FLOAT(__cdecl*)(FLOAT))0x44cc20)(atan2f(z,x)+M_PI_2);
		fx->uvw[2] = 0.0;
	}
	else fx->uvw[0] = fx->uvw[1] = fx->uvw[2] = 0.0;

	return 1;
}
static void __cdecl som_SFX_132b_43a350_candle(sfx *fx)
{
		//som_MDL_43a350_sfx_fixes
		auto &c = fx->duration, swap = c;
		if(EX::INI::Bugfix()->do_fix_animation_sample_rate)
		{
			//som_game_60fps doubles the lifespan so that c/2 is able to work
			if(swap>1) c = c/2; //else assert(c);
		}

	if(0||SOM::emu)
	{
		((void(*)(sfx*))0x43a350)(fx);

		if(swap) c = swap-1; return;
	}

	auto &dat = sfx_dat[fx->sfx]; //UNFINISHED

	if(!fx->duration) //terminate?
	return ((void(*)(sfx*))0x42f190)(fx);

	if(dat.unk2[3]==1)
	{
		memcpy(fx->xyz,*fx->cp1,3*sizeof(float));
	}
	
	//FUN_00401070_get_XYZ_thiscall?_4c2358_view_matrix_only?
	void *vmo = (void*)0x4c2358; float x,y,z;
	((BYTE(__thiscall*)(void*,float*,float*,float*))0x401070)(vmo,&x,&y,&z);
	x-=fx->xyz[0];
	y-=fx->xyz[1];
	z-=fx->xyz[2];
	
	if(dat.unk2[2]==0||dat.unk2[2]==2)
	{
		fx->uvw[1] = atan2f(z,x)+M_PI_2;
	}
	if(dat.unk2[2]==1||dat.unk2[2]==2)
	{
		fx->uvw[0] = -atan2f(y,sqrtf(z*z+x*x));
	}

	//FUN_0042f7a0_block_alloc_image_based_sfx
	auto se = som_MDL_42f7a0(1,dat.unk2[1]);
	if(!se) return;

	float s1 = dat.width*fx->scale*0.5;
	float s2 = dat.height*fx->scale*0.5;
	se->vdata[0].x = -s1;
	se->vdata[1].x = s1;
	se->vdata[2].x = s1;
	se->vdata[3].x = -s1;
	se->vdata[0].y = s2;
	se->vdata[1].y = s2;	
	se->vdata[2].y = -s2;
	se->vdata[3].y = -s2;
	
	int i6 = dat.unk2[4]-fx->duration%dat.unk2[4];
	int i4 = (int)dat.speed;
	//local_98 = (float)(i6%i4); //???
	float extraout_ST0 = (float)(i6%i4)/dat.speed; //guessing?
	i4 = (int)dat.scale;
	float local_98 = (float)(i6/i4)/dat.scale;

	se->vdata[0].s = extraout_ST0;
	se->vdata[1].s = extraout_ST0+1.0/dat.speed;
	se->vdata[2].s = extraout_ST0+1.0/dat.speed;
	se->vdata[3].s = extraout_ST0;
	se->vdata[0].t = local_98;	
	se->vdata[1].t = local_98;	
	se->vdata[2].t = local_98+1.0/dat.scale;
	se->vdata[3].t = local_98+1.0/dat.scale;	

	DWORD color = (int)dat.radius;

	DWORD diffuse; if(dat.unk2[1]==2) //rgb blend?
	{
		diffuse = ((color|0xffffff00)<<8|color)<<8|color;
	}
	else diffuse = color<<24|0xffffff; //alpha

	auto *p = se->vdata;
	for(int i=4;i-->0;p++)
	{
		p->z = 0; p->diffuse = diffuse;
	}
		
	se->texture = fx->txr;

	float m[4][4];
	Somvector::map(m).rotation<3,4,'xyz'>(fx->uvw);
	Somvector::map(m[3]).copy<3>(fx->xyz).se<3>() = 1;		

	p = se->vdata;
	for(int i=4;i-->0;p++)
	{
		auto *v = &p->x; //YUCK
		Somvector::map<3>(v).premultiply<4>(m);
	}

	fx->duration--;

		if(swap) c = swap-1;
}

static BYTE __cdecl som_SFX_134a_43b240(sfx *fx, init *in)
{
	if(0||SOM::emu) 
	return ((BYTE(*)(sfx*,init*))0x43b240)(fx,in);

	auto &dat = sfx_dat[in->sfx];
	
	if(2!=sfx_ref[dat.model].type||X42EEB0) RETURN0
		
	fx->cp1 = fx->cp2 = 0; //EXTENSION //LAB_0043b2e0?

	if(!in->mdl_cp)
	{
		if(!in->mdo_cp) goto LAB_0043b2e0; //???

		//FUN_00446230_sfx_sub?_get_stored_CP_pointer?_type1?_MDO?
		(void*&)fx->cp1 =
		((void*(__cdecl*)(SOM::MDO*,int))0x446230)(in->mdo_cp,in->cp);
		(void*&)fx->cp2 =
		((void*(__cdecl*)(SOM::MDO*,int))0x446230)(in->mdo_cp,in->cp2);
	}
	else
	{
		//FUN_00441680_sfx_sub?_get_stored_CP_pointer?_type2?_MDL?
		(void*&)fx->cp1 =
		((void*(__cdecl*)(SOM::MDL*,int))0x441680)(in->mdl_cp,in->cp);
		(void*&)fx->cp2 =
		((void*(__cdecl*)(SOM::MDL*,int))0x441680)(in->mdl_cp,in->cp2);
	}
	LAB_0043b2e0:

	fx->duration = dat.unk2[0];

	if(fx->cp1) memcpy(fx->xyz,fx->cp1,3*sizeof(float));

//	fx->uvw[0] = 0.0;
	
	if(fx->cp2&&fx->cp1) //EXTENSION
	{
		float dx = (*fx->cp2)[0]-(*fx->cp1)[0];
		float dz = (*fx->cp2)[2]-(*fx->cp1)[2];	
		fx->uvw[1] = //FUN_0044cc20_fmod_pi?
		((FLOAT(__cdecl*)(FLOAT))0x44cc20)(atan2f(dz,dx)+M_PI_2);
	}
//	else fx->uvw[1] = 0; //EXTENSION

	fx->uvw[2] = SOM::rng()%360*M_PI/180;

	return 1;
}
static void __cdecl som_SFX_134b_43b360(sfx *fx)
{
		assert(!fx); //what uses this?

	if(1||SOM::emu) 
	return ((void(*)(sfx*))0x43b360)(fx);

	auto &dat = sfx_dat[fx->sfx]; //UNFINISHED

	if(!fx->duration) //terminate?
	return ((void(*)(sfx*))0x42f190)(fx);

		/*
	if(dat.unk2[4]!=0)
	{
		pfVar12 = (float *)fx->const_vel[0];
		fx->xyz[0] = *pfVar12;
		fx->xyz[1] = pfVar12[1];
		fx->xyz[2] = pfVar12[2];
		fVar17 = *(float *)fx->const_vel[1];
		fVar1 = *pfVar12;
		fVar2 = ((float *)fx->const_vel[1])[2];
		fVar3 = pfVar12[2];
		fx->uvw[0] = 0.0;
		fVar16 = fpatan(fVar2-fVar3,fVar17-fVar1);
		fVar17 = FUN_0044cc20_fmod_pi?((float)(fVar16+1.570796));
		fx->uvw[1] = fVar17;
		fVar17 = FUN_0044cc20_fmod_pi?
			((float)(uint)dat.unk2[3]*0.01745329+
			 fx->uvw[2]);
		fx->uvw[2] = fVar17;
	}
	fVar17 = FUN_0044cc20_fmod_pi?
		(dat.unk5[0]*0.01745329+fx->uvw[2]);
	fx->uvw[2] = fVar17;
	psVar6 = FUN_0042f7a0_block_alloc_image_based_sfx
	(2,(uint)dat.unk2[5]);
	if(psVar6!=(struct SOM_scene_picture *)0x0)
	{
		uVar7 = __ftol();
		fVar17 = (float)(extraout_ST0*fx->scale*
						 dat.width*0.5);
		fVar1 = dat.height;
		fVar2 = fx->scale;
		psVar6->vdata[1].x = fVar17;
		psVar6->vdata[2].x = fVar17;
		psVar6->vdata[0].s = 0.0;
		psVar6->vdata[0].t = 0.0;
		psVar6->vdata[1].t = 0.0;
		psVar6->vdata[3].s = 0.0;
		fVar1 = (float)(fVar1*fVar2*extraout_ST0*0.5);
		psVar6->vdata[0].y = fVar1;
		psVar6->vdata[1].y = fVar1;
		psVar6->vdata[0].x = -fVar17;
		psVar6->vdata[2].y = -fVar1;
		psVar6->vdata[3].y = -fVar1;
		psVar6->vdata[3].x = -fVar17;
		psVar6->vdata[1].s = 1.0;
		psVar6->vdata[2].s = 1.0;
		psVar6->vdata[2].t = 1.0;
		psVar6->vdata[3].t = 1.0;
		local_8c = 4;
		psVar5 = psVar6;
		do
		{
			pdVar9 = &psVar5->vdata[0].diffuse;
			psVar5->vdata[0].z = 0.0;
			if(dat.unk2[5]==2)
			{
				dVar10 = ((uVar7|0xffffff00)<<8|uVar7)<<8|uVar7;
			}
			else
			{
				dVar10 = uVar7<<0x18|0xffffff;
			}
			*pdVar9 = dVar10;
			local_8c = local_8c+-1;
			psVar5 = (struct SOM_scene_picture *)pdVar9;
		} while(local_8c!=0);
		FUN_00449e00_z_rotation_matrix(local_80,fx->uvw[2]);
		FUN_00449d80_x_rotation_matrix(local_40,fx->uvw[0]);
		FUN_00449f70_matrix_mul(local_80,local_80,local_40);
		FUN_00449dc0_y_rotation_matrix(local_40,fx->uvw[1]);
		FUN_00449f70_matrix_mul(local_80,local_80,local_40);
		FUN_00449d20_translation_matrix(local_40,fx->xyz[0],fx->xyz[1],fx->xyz[2]);
		FUN_00449f70_matrix_mul(local_80,local_80,local_40);
		pfVar12 = &psVar6->vdata[0].y;
		iVar14 = 4;
		do
		{
			FUN_00449ea0_xform_vector
			(local_80,((struct LVERTEX *)(pfVar12+-1))->x,*pfVar12,pfVar12[1],
			 (struct LVERTEX *)(pfVar12+-1),pfVar12,pfVar12+1);
			pfVar12 = pfVar12+8;
			iVar14 = iVar14+-1;
		} while(iVar14!=0);
		psVar11 = psVar6[1].vdata;
		psVar8 = psVar6->vdata+3;
		local_8c = 4;
		do
		{
			psVar13 = psVar8;
			psVar15 = psVar11;
			for(iVar14 = 8; iVar14!=0; iVar14 = iVar14+-1)
			{
				psVar15->x = psVar13->x;
				psVar13 = (struct LVERTEX *)&psVar13->y;
				psVar15 = (struct LVERTEX *)&psVar15->y;
			}
			psVar8 = psVar8+-1;
			psVar11 = psVar11+1;
			local_8c = local_8c+-1;
		} while(local_8c!=0);
		uVar4 = fx->txr;
		psVar6[1].texture = uVar4;
		psVar6->texture = uVar4;
	//	fx->duration = fx->duration-1;
	}
		*/
	fx->duration--;
}

extern void som_SFX_reprogram()
{
	void*(*fptr)[255][2]; (DWORD&)fptr = 0x45e648;

	auto &f = *fptr; if(EX::INI::Option()->do_sfx)
	{	
	//NOTE: THIS CODE DOESN'T ACCOMPLISH ANYTHING//
	//IT'S TO SEE IF THE SFX SYSTEM IS UNDERSTOOD//

		f[0][0] = som_SFX_0a_42ed50_needle; //1	//501 is arrow
		f[0][1] = som_SFX_0b_42efe0_needle; //1
		f[1][0] = som_SFX_1a_42f2e0_spinner; //UNUSED //1
		f[1][1] = som_SFX_1b_42f300_spinner; //UNUSED //1
		f[2][0] = som_SFX_2a_42f380_picture; //UNUSED //2
		f[2][1] = som_SFX_2b_42f450_picture; //UNUSED //2
		f[3][0] = som_SFX_3a_42f810_waterbullet; //2
		f[3][1] = som_SFX_3b_42f920_waterbullet; //2
		f[4][0] = som_SFX_4a_42feb0_fireball; //3
		f[4][1] = som_SFX_4b_42feb0_fireball; //3
		f[5][0] = som_SFX_5a_430090_windcutter; //4 
		f[5][1] = som_SFX_5b_430220_windcutter;	//4
		//9 is the next one with an SFX entry
	//	f[6][0] = UNUSED //430620
	//	f[6][1] = UNUSED //4307d0
	//	f[7][0] = UNUSED //430970
	//	f[7][1] = UNUSED //430a90
	//	f[8][0] = UNUSED //430de0
	//	f[8][1] = UNUSED //4311f0
		f[9][0]	= som_SFX_9a_4313e0_missile;
		f[9][1]	= som_SFX_9b_431850_missile;
	//	f[10][0] = UNUSED //431f00
	//	f[10][1] = UNUSED //4322d0
	//	f[11][0] = UNUSED //432810
	//	f[11][1] = UNUSED //432c00
		f[12][0] = som_SFX_12a_4331c0_firecolumn; //UNUSED picture?
		f[12][1] = som_SFX_12b_4332d0_firecolumn; //UNUSED picture?
	//	f[13][0] = UNUSED 
	//	f[13][1] = UNUSED 
	//	f[14][0] = UNUSED //big needle crashing diagonally at feet?
	//	f[14][1] = UNUSED 
	//	f[15][0] = UNUSED //curveball???
	//	f[15][1] = UNUSED
	//	========
		f[20][0] = som_SFX_20a_435130_earthwave;
		f[20][1] = som_SFX_20b_435490_earthwave;
		f[21][0] = som_SFX_21a_43bbd0_firewall;
		f[21][1] = som_SFX_21b_43bce0_firewall;
		f[22][0] = som_SFX_22a_4352f0_explosion;
		f[22][1] = som_SFX_20b_435490_earthwave; //REUSED
	//	========
		f[30][0] = som_SFX_30a_43b6d0_haze;
		f[30][1] = som_SFX_30b_43b7e0_haze;
		f[31][0] = som_SFX_31a_435790_vortex;
		f[31][1] = som_SFX_31b_435ab0_vortex;
		f[32][0] = som_SFX_32a_435c90_dragon;
		f[32][1] = som_SFX_32b_435fc0_dragon;
	//	f[33][0] = UNUSED 
	//	f[33][1] = UNUSED
		f[34][0] = som_SFX_34a_4365c0_tornado;
		f[34][1] = som_SFX_34b_436760_tornado;
	//	========
		f[40][0] = som_SFX_40a_4369b0_lightning;
		f[40][1] = som_SFX_40b_436c40_lightning;
	//	f[41][0] = //lightning #2 //UNFINISHED?
	//	f[41][1] = //lightning #2 //UNFINISHED?
	//	f[42][0] = //lightning #3 //UNFINISHED?
	//	f[42][1] = //lightning #3 //UNFINISHED?
	//	f[43][0] = //lightning #4 //UNFINISHED?
	//	f[43][1] = //lightning #4 //UNFINISHED?
	//  ========
		f[45][0] = som_SFX_45a_437cd0_moonlight;
		f[45][1] = som_SFX_45b_437e30_moonlight;
		f[46][0] = som_SFX_46a_43c0b0_triplefang;
		f[46][1] = som_SFX_46b_43c5e0_triplefang;
	//  ========
		f[100][0] = som_SFX_100a_438080_explode2D;
		f[100][1] = som_SFX_100b_438180_explode2D;
		f[101][0] = som_SFX_101a_438450_explode3D;
		f[101][1] = som_SFX_101b_438630_explode3D;
	//	f[102][0] = UNUSED
	//	f[102][1] = UNUSED
	//  =========
		f[128][0] = som_SFX_128a_438b10;
		f[128][1] = som_SFX_128b_438ce0;
		f[129][0] = som_SFX_129a_438e10;
		f[129][1] = som_SFX_129b_438f30;
	//	f[130][0] = som_SFX_130a_439110; //UNUSED
	//	f[130][1] = som_SFX_130b_4391f0; //UNUSED
		f[131][0] = som_SFX_131a_439450;
		f[131][1] = som_SFX_131b_439510;
		f[132][0] = som_SFX_132a_43a1e0_candle;
		f[132][1] = som_SFX_132b_43a350_candle;
	//	f[133][0] = som_SFX_133a_43a610; //UNUSED
	//	f[133][1] = som_SFX_133b_43a690; //UNUSED //reverses animation
		f[134][0] = som_SFX_134a_43b240;
		f[134][1] = som_SFX_134b_43b360;
	}
		
	//00401976 e8 e5 ca 02 00	CALL	FUN_0042e460_init_SFX_dat                 
	*(DWORD*)0x401977 = (DWORD)som_SFX_42e460_init_SFX_dat-0x40197b;
	//call these even if SFX.dat doesn't open (it isn't available)
	//FUN_0044d4d0_bit_encode_scene_element_flags(&DAT_01ce1ce8_mdo_rop_0?,0,2,1,2,1,0,1,1);
    //FUN_0044d4d0_bit_encode_scene_element_flags(&DAT_01ce1cec_mdo_rop_1?,0,5,6,2,0,1,1,1);
    //FUN_0044d4d0_bit_encode_scene_element_flags(&DAT_01ce1cf0_mdo_rop_2?,0,5,2,2,0,1,1,0);
	//0042e519 74 78           JZ         LAB_0042e593
	//*(BYTE*)0x42e51a = 24;
}

extern void som_SFX_write_new_fx_prf(int i, void *dat) //8/11/2024
{
	namespace prf = SWORDOFMOONLIGHT::prf;

	char buf[MAX_PATH];
	//sprintf(buf,SOMEX_(A)"\\data\\sfx\\prof\\%04d.prf",i);
	sprintf(buf,"C:\\Users\\Michael\\Sword of Moonlight\\data\\sfx\\new-prof\\%04d.prf",i);
	FILE *f = som_game_fopen(buf,"wb");
	if(!f) return;

	memset(buf,0x00,32);
	sprintf(buf,"SFX #%d",i);
	som_game_fwrite(buf,31,1,f);

	prf::magic2_t m2 = {};
	m2.type = 2;
	m2.SFX = (WORD)i;
	auto &d = m2.SFX_dat;
	memcpy(&d,dat,sizeof(d));

	sprintf(buf,SOMEX_(A)"\\data\\sfx\\model\\%04d.%bmp",d.model);	
	FILE *g = som_game_fopen(buf,"rb");
	if(g) som_game_fclose(g); 

	sprintf(m2.model,"%04d.%s",d.model,g?"bmp":"mdl");	

	if(d.snd&&d.snd!=0xffff)
	{
		sprintf(m2.sound,"%04d.wav",d.snd);
	}
		
	som_game_fwrite(&m2,sizeof(m2),1,f);

	prf::history_t note = {};
	//strcpy(note,"auto-generated 8/11/2024");	
	int len = sprintf(buf,
	"a%d\nb%d\nc%d\nd%d\ne%d\nf%d\ng%d\nh%d\n"
	"i%g\nj%g\nk%g\nl%g\nm%g\nn%g\no%g\np%g\n"
	"q%d\nr%d\ns%d\nt%d\nu%d\n",
	d.procedure,d.model,d.unk2[0],d.unk2[1],d.unk2[2],d.unk2[3],d.unk2[4],d.unk2[5],
	d.width,d.height,d.radius,d.speed,d.scale,d.scale2,d.unk5[0],d.unk5[1],
	d.snd,d.chainfx,(int)d.pitch,(int)d._pad2,d._unk6);
	memcpy(note,buf,min(len,sizeof(note)-1));

	som_game_fwrite(&note,sizeof(note),1,f);

	som_game_fwrite("fx",2,1,f);

	som_game_fclose(f);
}

extern void som_SFX_write_PARAM_SFX_dat()
{
	SOM::sfx_dat_rec sfx_dat[1024] = {}; //SHADOWING
	for(int i=1024;i-->0;)
	{
		sfx_dat[i].procedure = 255; //IMPORTANT
		sfx_dat[i].model = 255;
	}

	SOM::sfx_pro_rec sfx_pro[1024] = {};

	//start with anything in the data/sfx/sfx.dat
	wchar_t w[MAX_PATH];
	if(EX::data(L"Sfx\\Sfx.dat",w))
	{
		if(FILE*f=_wfopen(w,L"rb"))
		{
			fread(sfx_dat,sizeof(sfx_dat),1,f);
			fclose(f);
		}
		else assert(0);
	}

	int i = 0;
	while(*EX::data(i)) 
	i++;
	while(i-->0)
	{
		int w_s = swprintf(w,L"%s\\sfx\\prof\\*.prf",EX::data(i))-5;

		WIN32_FIND_DATAW data; 
		HANDLE find = FindFirstFileW(w,&data);
		if(find!=INVALID_HANDLE_VALUE) do		
		{
			wcscpy_s(w+w_s,MAX_PATH-w_s,data.cFileName);
			BYTE m2[243];
			FILE *f = _wfopen(w,L"rb"); if(!f)
			{
				assert(0); continue; 
			}				
			if(243==fread(m2,1,sizeof(m2),f))
			{
				WORD fx = *(WORD*)(m2+32);
				if(fx<1024)
				{
					memcpy(&sfx_dat[fx],m2+34,48);

					sfx_pro[fx].slot = fx;
					memcpy(sfx_pro[fx].model,m2+34+48,31);
					memcpy(sfx_pro[fx].sound,m2+34+48+31,31);
				}
				else assert(0);
			}
			fclose(f);

		}while(FindNextFileW(find,&data));
	}

	swprintf(w,L"%s\\PARAM\\SFX.DAT",EX::cd());
	if(FILE*f=_wfopen(w,L"wb"))
	{
		fwrite(sfx_dat,sizeof(sfx_dat),1,f);
		fclose(f);
	}
	else assert(0);

	swprintf(w,L"%s\\PARAM\\SFX.PRO",EX::cd());
	if(FILE*f=_wfopen(w,L"wb"))
	{
		/*the generated files have model fields
		for(int i=1;i<1024;i++) if(sfx_pro[i].slot)		
		fwrite(&sfx_pro[i],sizeof(sfx_pro[i]),1,f);
		*/
		fwrite(sfx_pro,sizeof(sfx_pro),1,f);
		fclose(f);
	}
	else assert(0);
}
