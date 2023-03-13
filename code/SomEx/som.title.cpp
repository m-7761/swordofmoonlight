
#include "Ex.h" 
EX_TRANSLATION_UNIT
			 
#include "Ex.ini.h"
#include "Ex.langs.h" 
#include "Ex.output.h"
#include "Ex.regex.h"

#include "dx.ddraw.h"

#include "SomEx.h"
#include "som.932.h"
#include "som.state.h"
#include "som.files.h" 
#include "som.menus.h"
#include "som.title.h" 

///////////////////////////////
//                           //
// formerly of som.lexer.cpp //
//                           //
///////////////////////////////

#include "../lib/swordofmoonlight.h" 
namespace mo = SWORDOFMOONLIGHT::mo;

extern SOM::MO::memoryfilebase *SOM::MO::view = 0;

const char *SOM::gettext(const char *msgid, int cc) 
{		
	if(!*EX::Locales[0].catalog) return msgid;

	static mo::image_t x; static int one_off = 0; //???
	
	if(msgid&&!one_off++) SOM::gettext(0); //initialize
	
	static struct som
	{
		mo::range_t map[3],prm[5],sys[3];		
		som(){ memset(this,0x00,sizeof(*this)); }

	}som; using SOM::MO::view; static DWORD fresh = 0;

	if(!msgid) //hack: soft reset (scheduled obsolete?)
	{	
		if(one_off!=1) mo::unmap(x);
		if(view) UnmapViewOfFile(view);
		SOM::MO::memoryfilename name(EX::Locales[0].catalog);
		HANDLE fm = name?OpenFileMapping(FILE_MAP_READ,0,name):0; 
		if(fm) (void*&)view = MapViewOfFile(fm,FILE_MAP_READ,0,0,0);
		if(view) mo::maptorom(x,(char*)view+view->mobase,view->megabytes*1024*1024); 
		else mo::maptofile(x,EX::Locales[0].catalog);
		if(fm) CloseHandle(fm);
		
		//ensure 0 is not being passed erroneously
		static unsigned paranoia = -51; assert(SOM::frame-paranoia>50);
		paranoia = SOM::frame; 

		fresh = -1;	new(&som)struct som; return 0;
	}
	else if(!*msgid) return msgid; if(!x) return msgid;

	int out_s = 0;
	const char *out = msgid; if(cc)
	{	
		mo::range_t *r = 0; switch(cc)
		{
		case 'MAP2': r = &som.map[2]; break;
		case 'PRM0': r = &som.prm[0]; break;
		case 'PRM2': r = &som.prm[2]; break;
		case 'PRM4': r = &som.prm[4]; break;
		case 'PRM5': r = &som.prm[5]; break;
		case 'SYS0': r = &som.sys[0]; break;
		case 'SYS1': r = &som.sys[1]; break;
		case 'SYS2': r = &som.sys[2]; break;
		}
		assert(r); if(!r) return msgid;
		if(view&&fresh!=view->numberofalterationsinceopening)
		{
			fresh = view->numberofalterationsinceopening;
			new(&som)struct som;		
		}
		if(!r->n&&r->lowerbound!=size_t(-1))
		{	
			char msgctxt[8] = "0123\4";
			*(long*)msgctxt = _byteswap_ulong(cc);
			if(!mo::range(x,*r,msgctxt,5)) r->lowerbound = -1;
		}
		size_t i = mo::find(*r,msgid); if(i!=r->n)
		{
			out = mo::msgstr(*r,i);	out_s = r->strof[i].clen;
		}
	}
	else out_s = mo::gettext(x,msgid,out); 	

	if(out!=msgid&&SOM::game) //!!
	{
		EXML::Attribuf<8> attribs(out,out_s);
		const char *etc = SOM::exml_text_component(attribs);
		if(etc) out = etc; 
	}
	return out;
}

extern bool SOM::japanese() 
{		
	if(!EX::Locales[0].languages){ assert(0); return true; }
	static bool out = PRIMARYLANGID(LANGIDFROMLCID(EX::Locales[0]))==LANG_JAPANESE; 
	return out;
} 

extern const char *SOM::transl8(const char *ja, const char *en)
{
	const char *l10n = ja; 
	if(EX::Decode(932,&ja)==65001) l10n = SOM::gettext(ja);
	bool english = EX::INI::Script()->do_use_builtin_english_by_default;
	return l10n==ja&&english&&!SOM::japanese()?en:l10n; 
}

//////////////////////////////
//                          //
// formerly of Ex.menus.cpp //
//                          //
//////////////////////////////

static int som_title_at(const char **inout)
{
	const char *out = 0;
	const char *in = *inout; if(*in!='@') return 932;
	bool magic = in[1]=='-'; //includes -0
	int i = abs(atoi(in+1)); if(i>249) return 932;
	
	//REMOVE ME?
	extern int *som_game_nothing(); 
	int *nothing = som_game_nothing();
	if(!magic) 
	{
		for(int j=0;j<7;j++) if(i==nothing[j])
		{
			*inout = SOM::transl8(som_932_Nothing,"Nothing");
			return 65001;
		}
	}
	else if(nothing[7]!=0xFF)
	{
		if(i==SOM::L.magic32[nothing[7]])
		{
			*inout = SOM::transl8(som_932_Nothing,"Nothing");
			return 65001;
		}
	}

	if(magic) in = SOM::PARAM::Magic.prm->records[i];
	if(!magic) in = SOM::PARAM::Item.prm->records[i]; 
	if(EX::Decode(932,&in)!=65001) return 932;
	out = SOM::gettext(in,magic?'PRM2':'PRM0');
	if(out==in) out = SOM::gettext(in);
	*inout = out; return 65001;
}

static char *som_title_sprintf(int fcp, const char *f, const char *fp, int in_cp, const char **in)
{
 	long long out_x[6],*x = out_x;	
	int compile[sizeof(*x)==sizeof(double)];

	/////////////////////////////////
	//                             //
	// formerly som_title_printf_x //
	//                             //
	/////////////////////////////////

	if(!f||!fp||!in) return 0; //paranoia?

	int _sN = 0; //2017
	int fp_s = 0, s = 0; //som_title_printf_s
	for(;*fp&&fp_s<EX_ARRAYSIZEOF(out_x)&&*in;fp_s++,x++,fp++,in++)
	{
		if(*fp=='d')
		{				
			s+=32; *x = atoi(*in); 
		}
		else if(*fp=='f')
		{
			s+=64; (double&)*x = strtod(*in,0); 
		}
		else if(*fp=='s')
		{	
			//EX::Decode can be used only once without a buffer
			assert(_sN++==0); 

			const char *r,*u = *in; 						
			if(som_title_at(&u)==932)
			if(EX::Decode(in_cp,&u,0)==65001)
			{
				//REMINDER: 932==som_title_at means this is NOT
				//an item name
				while(*u==' ') u++; //trim spaces
				r = u; while(*r&&*r!=' ') r++; (char&)*r = '\0'; 
				//fyi: som_menus_x::classnames handles the 
				//only remaining case that requires a context
				u = SOM::gettext(u); 
			}
			else u = "(null)"; //safe: typical sprintf output			

			s+=strlen(u); *(const char**)x = u;
		}
		else //new extended menu item?
		{
			*x = 0; assert(0); break;
		}				
	}

	assert(!*fp); if(*fp) return 0;

	////////////////////////////////
	//                            //
	// formerly som_title_sprintf //
	//                            //
	////////////////////////////////
		
	static char out[250];
	char g[90]; int g_s = strlen(f);
	if(65001!=EX::Decode(fcp,&f,&g_s)
	||sizeof(g)<g_s+1||sizeof(out)<g_s+s) return 0;	
	else f = (char*)memcpy(g,f,g_s+1); 
	//HACK: +1 is in case first letter is $ but I think
	//a $ may still crash... I don't know why $ crashes
	//if it's first
	if(strchr(f+1,'$')) //libintl?
	{
		int i = 0,l;
		long long swap[EX_ARRAYSIZEOF(out_x)];		
		for(char *p=(char*)g,*q=p,*e;*q++=*p++;i++) 
		if(p[-1]=='%'&&*p!='%'&&*p)
		{
			l = strtol(p,(char**)&e,10); 
			swap[i] = out_x[*e=='$'&&l<fp_s?l:i]; 
			if(*e=='$') p = e+1;
		}
		memcpy(out_x,swap,sizeof(out_x));
	}	   		
	char *pack = (char*)out_x; x = out_x;
	for(const char *p=f;*p;p++) if(p[0]=='%'&&p[1]!='%')
	{		   
		if(fp_s--<0) return 0;
		while(*p&&!isalpha(*p)) p++; switch(tolower(*p))
		{		
		case 'c': case 'd':	case 'i': case 'o': case 'u': case 'x': 

			*((int*&)pack)++ = (int&)*x++; break;

		case 'e': case 'f': case 'g': case 'a':

			*((double*&)pack)++ = (double&)*x++; break;
			
		case 'p': case 's': //pointers

			*((void**&)pack)++ = (void*&)*x++; break;

		default: assert(0); return 0; //unsupported?
		}
	}			   
	int compile2[6==EX_ARRAYSIZEOF(out_x)];
	int err = _sprintf_s_l(out,sizeof(out),f,EX::Locale::C,
	out_x[0],out_x[1],out_x[2],out_x[3],out_x[4],out_x[5]);			
	return err<=0?0:out;
}

///////////////////////////////
//                           //
// formerly of som.money.cpp //
//                           //
///////////////////////////////
		  
//REMOVE ME? (soon)
static const int som_money_cp = 932;
static char som_money_match_buffer[64];
static const char *som_money_parse_buffer[2+1] = {0};
static const char **som_money_parse_match()
{
	return som_money_parse_buffer[0]?som_money_parse_buffer:0; 
}
bool som_money_match(const char *in, int in_s) //SOM::Money::match
{
	som_money_parse_buffer[0] = 0; //reset
	if(!in) in = ""; if(!in_s) in_s = strlen(in); 
	static const int opt_sz = sizeof(som_932_Money)-sizeof("%d%s");
	if(in_s<opt_sz||memcmp(in+in_s-opt_sz,som_932_Money+sizeof("%d%s")-1,opt_sz)) 
	return false;
	static const int *re = EX::Regex::LOST;	 
	if(!*re) re = EX::Regex(som_money_cp,som_932_Money,sizeof(som_932_Money)-1); 
	//return som_money_match(re,in,in_s); 
	{
		EX::Regex regex(re);
		bool match = false; regex.reset(); 
		char *out = som_money_match_buffer;
		int out_s = sizeof(som_money_match_buffer);	
		while((match=regex.match(som_money_cp,&in,&in_s,&out,&out_s))&&in_s&&out_s);
		if(match&&in_s||!match){ assert(!match); return false; }	
		som_money_parse_buffer[regex.parse(som_money_parse_buffer,2)] = 0;
		return match;
	}
}		  
static const char *som_money_translate(const char **in)
{
	const char *x = SOM::transl8(som_932_Money,"%d%s"); 			
	assert(in==som_money_parse_buffer&&*in==som_money_match_buffer);
	return som_title_sprintf(65001,x,"ds",som_money_cp,in);
}

///////////////////////////////
//                           //
// formerly of som.lexer.cpp //
//                           //
///////////////////////////////

static size_t som_lexer_sub = 0;

enum{ som_lexer_subs_s=8, som_lexer_subs_t=96-1 };

static struct //todo: a more object oriented approach??
{
	size_t ticks, clock, timer; char chars[som_lexer_subs_t],cancel;

}som_lexer_subs[som_lexer_subs_s] = {{0}}; //circular queue

static const char *som_lexer_subtitle(const char *in, int action)
{
	const char *out = "";

	if(!in||!*in||SOM::emu) return in; 
	
	EX::INI::Player pc;

	size_t delay = SOM::L.hold;

	size_t sub = som_lexer_sub, subzero = sub; 

	//bool cancel = true; //HACK

	if(som_lexer_subs[sub].ticks) //filtering
	{
		//ALGORITHM
		//seems this is designed to compare to the first subtitle in
		//the queue
		while(som_lexer_subs[++sub%=som_lexer_subs_s].ticks&&sub!=subzero)
		;	
		if(!strncmp(som_lexer_subs[--sub%=som_lexer_subs_s].chars,in,som_lexer_subs_t))
		{				
			if(som_lexer_subs[sub].ticks>=som_lexer_subs[sub].timer)
			{
				//-1 is used for money which should not be cancelled
				if(action>0&&!SOM::eventick) som_lexer_subs[sub].timer = 0; //cancel
			}			
			if(som_lexer_subs[sub].clock-som_lexer_subs[sub].ticks<=1000)
			{
				return out; //repeat showing
			}
			if(som_lexer_subs[sub].ticks>som_lexer_subs[sub].clock) //2020
			{
				return out; //som_title_further_delay_sub?
			}
		}
	}
	/*else if(SOM::eventick>SOM::motions.tick) //hack? //UNUSED
	{
		//NOTE: this is UNUSED and doesn't work YET

		//if(EX::debug) MessageBeep(-1); //manual subtitle?
	
		delay = SOM::eventick-SOM::motions.tick;

		SOM::eventick-=delay; 
		
		//cancel = false; //not working :(
	}*/
	else if(action) //a fresh Action subtitle
	{
		//allow time for the rising phase of duck
		if(!pc->do_not_dash&&pc->player_character_duck)
		{
			//REMINDER: increasing this doesn't delay
			//regular activation titles further... it
			//feels like they activate as the ducking
			//bottoms out

			//2021: note, 1.4 happens to work out for 
			//money if not canceled below. the title
			//appears just as a crouch touches down
			/*2020: som_title_further_delay_sub adds
			//delay
			delay*=2;*/
			if(action>0)
			delay*=1.4f; //2020
			else delay*=1.2f; //shaving off what can
		}
		if(action>0) //2021: SOM::eventick isn't set
		{
			//subtract wait for chest open animation
			//unsigned open = EX::tick()-SOM::eventick;
			unsigned open = SOM::motions.tick-SOM::eventick;

			delay = open<delay?delay-open:0;
		}
	}

	delay+=max(-int(delay),pc->subtitles_ms_interim);
		
	if(som_lexer_subs[sub].ticks) ++sub%=som_lexer_subs_s;

	som_lexer_subs[sub].timer = pc->subtitles_ms_timeout;  
	som_lexer_subs[sub].clock = som_lexer_subs[sub].timer+delay;
	som_lexer_subs[sub].ticks = som_lexer_subs[sub].clock;
	//som_lexer_subs[sub].cancel = cancel; //UNUSED
	som_lexer_subs[sub].cancel = action>0; //TESTING
	strcpy_s(som_lexer_subs[sub].chars,in); return out;
}
extern void som_title_further_delay_sub(int delay) //2020
{
	auto &sub = som_lexer_subs[som_lexer_sub];

	//NOTE: I tried increasing clock here... that might
	//be the right idea... but I think it makes the queue
	//incoherent
	if(sub.timer&&sub.ticks>sub.timer&&sub.cancel)
	{
		sub.ticks = delay<0?sub.timer-delay:sub.ticks+delay;
	}
}

const wchar_t *SOM::subtitle()
{
	const size_t out_s = 64;

	static wchar_t out[out_s] = L"";

	static unsigned time = 0;
	
	unsigned diff = 0; if(!EX::context()) 
	{
		unsigned now = SOM::motions.tick;

		if(SOM::motions.tick-time>=45)
		time = SOM::motions.tick; //synchronize?
		else diff = now-time; 
	}
	else if(DDRAW::noTicks<45&&SOM::context) //limbo?
	{
		diff = DDRAW::noTicks; //menu?
	}
	time+=diff;
			
	auto &sub = som_lexer_subs[som_lexer_sub];

	if(!sub.ticks) //return 0; //???
	{
		//2020: I'm worried sutitles will be shown out-of-order
		for(int i=som_lexer_subs_s;i-->0;)		
		if(som_lexer_subs[++som_lexer_sub%=som_lexer_subs_s].ticks)
		return SOM::subtitle();
		return 0;
	}
	else if(!SOM::eventick&&sub.cancel&&sub.ticks>sub.timer)
	{
		sub.ticks = 0; //2020
	}
	else if(sub.ticks-=min(sub.ticks,diff)) 
	{
		if(sub.ticks<=sub.timer)
		{
			const char *utf8 = som_lexer_subs[som_lexer_sub].chars;

			if(!*out) EX::need_unicode_equivalent(65001,utf8,out,out_s);
		}
		else *out = '\0'; return *out?out:0;
	}
	else *out = '\0'; 

	++som_lexer_sub%=som_lexer_subs_s;

	return SOM::subtitle();
}

static const char *som_title_sub(const char *in, int in_s) 
{											   	
	//40: the character length of Sys.dat's messages
	enum{ message_s=40 };

	static int action = 0, cache_s = 0;

	static char message[som_lexer_subs_t], cache[message_s+1], *out = 0;

	if(!in||!in_s||in_s>message_s) return 0;

	//todo: support runtime locale switches
	if(cache_s==in_s&&!memcmp(in,cache,cache_s)) 
	{
		return out==message?som_lexer_subtitle(out,action):out;
	}
	
	message[0] = '\0'; out = 0; action = 0;

	if(som_money_match(in,in_s))					
	{
		const char *money = //SOM::Money::xparse();
		som_money_translate(som_money_parse_match());

		if(money&&!strcpy_s(message,money)) out = message; 
		
		action = -1; //hack: don't cancel
	}
	else //if(SOM::PARAM::Sys.dat->open()) //2022
	{
		//have to distinguish between action titles
		//with identical text
		int todolist[SOMEX_VNUMBER<=0x1020402UL];
		/*2022: won't work (423ce0 buffers subtitles)
		int i = -1;
		if(in>=SOM::L.sys_dat_messages_0_8[0]
		 &&in<=SOM::L.sys_dat_messages_0_8[8])
		{
			i = (in-SOM::L.sys_dat_messages_0_8[0])/41;
		}
		if(in>=SOM::L.sys_dat_messages_9_11[0]
		 &&in<=SOM::L.sys_dat_messages_9_11[2])
		{
			i = (in-SOM::L.sys_dat_messages_9_11[0])/41+9;
		}*/

		for(int i=0;i<12;i++) 
		{
			char *msg = SOM::L.sys_dat_messages_0_8[i];
			if(i>=9)
			msg = SOM::L.sys_dat_messages_9_11[i-9];

			if(*in!=*msg||strncmp(in,msg,41)) continue;
		
			if(i<2||i==9||i==10) action = i+1; //hack

			const char *utf8 = in; int utf8_s = in_s; 

			if(EX::Decode(932,&utf8,&utf8_s)==65001) 
			{	
				const char *x = SOM::gettext(utf8,'SYS1');
				if(x==utf8) x = SOM::gettext(utf8);
				if(!strcpy_s(message,x)) out = message;
				break;
			}
			else assert(0);
		}
	}

	memcpy(cache,in,in_s); cache[cache_s=in_s] = '\0';

	if(out==message) 
	return som_lexer_subtitle(out,action);	
	return out;	
}
extern const char *som_title_a = "\a";
extern const char *SOM::title(int cc, const char *txt)
{
	if(!cc) return som_title_a; //932: line-by-line mode
	return SOM::title(0,txt,cc)==932?txt:*txt?som_title_a:"";
}
extern int SOM::title_pixels = 0;
extern int SOM::title(void *procA, const char* &txt, int &len) 
{	
	if(!txt||!*txt||!len) return 932; 
	static som_menus_h::validator st; 
	int cc = (unsigned)len>65535?len:0; 	
	static std::string out; static int cp = 932; if(!cc) 
	{
		if(!procA||*txt==*som_title_a) //shadow/som_title
		{
			if(len=out.size()) txt = out.c_str(); return cp;
		}
		const char *sub = som_title_sub(txt,len); if(sub) //subtitle?
		{
			out = sub; len = out.size(); txt = out.c_str(); return cp = 65001;
		} 		
		static unsigned menu = 0;  
		if(!st||menu!=SOM::frame)
		new(&st)som_menus_h::validator(txt,len); menu = SOM::frame;
	}
	else len = 0; //paranoia

	out.clear(); cp = 932; if(!cc)
	{
		if(st(procA,txt,len))
		{
			const char *f = st->format(); //2017
			const char *x = f; 

			if(!x) //unformatted menu title
			{
				const char *x = st->translate(txt,len);
				if(x) txt = x; if(x) cp = 65001;			
			}
			else if(x=st->translate(x)) //!
			{			
				//2017 fix for Japanese sample project
				//2018 hack[32] couldn't fit "DATA%d____DATAnashi"
				char hack[32*4]; if(f!=x) strcpy_s(hack,x),x=hack;
				x = som_title_sprintf(65001,x,st->fchars(),st->cp,st->parse());
				assert(x!=hack);
				if(x) txt = x; if(x) cp = 65001; 
			}			
		}
		else if(*txt!='@') //irregular/non-menu title
		{
			cp = EX::Decode(932,&txt,cc?0:&len);
			if(cp==65001) txt = SOM::gettext(txt); 
		}
		else cp = som_title_at(&txt);

		//REMOVE ME? 
		if(st.menu.on) st.menu.on(st.menu.ns,!st);	

		if(SOM::title_pixels=st?st->pixels():0)
		DDRAW::doSuperSamplingMul(SOM::title_pixels); 
	}
	else //context sensitive title
	{
		//2020: guessing this should be set even if
		//932 is returned below?
		SOM::title_pixels = 0; 

		const char *u = txt, *x;
		cp = EX::Decode(932,&u,0);
		x = cp!=65001?u:SOM::gettext(u,cc); 
		if(x!=u) txt = x; else return 932; 

		//SOM::title_pixels = 0; //???
	}	
		
	out = txt,txt = out.c_str(); 
	len = out.size(); return cp;
}

//////////////////////////
//                      //
// PRETTY MUCH OBSOLETE //
//                      //
//////////////////////////

static wchar_t *som_lexer_tmp = 0;
static size_t som_lexer_tmp_s = 0;
static void som_lexer_tag(const wchar_t*,int&,wchar_t*,int&,size_t);
extern const wchar_t *SOM::lex(const wchar_t *in, wchar_t *out, size_t out_s)
{
	bool tagged = false;

	wchar_t* &tmp = som_lexer_tmp; 		
	size_t &tmp_s = som_lexer_tmp_s;

	int i, j;
	for(i=0,j=0;in[i];i++,j++)
	{
		if(out_s&&size_t(i)>=out_s) break;

		if(in[i]=='<')
		{	
			if(!tagged)
			{							
				if(!out&&(!tmp||out_s>tmp_s||size_t(i*2)>tmp_s))
				{
					if(tmp_s<=out_s) tmp_s = max(out_s,(size_t)max(i*2,256));

					delete[] tmp; tmp = new wchar_t[tmp_s];
				}

				if(!out) out = tmp;	wcsncpy(out,in,i);
			}
						
			som_lexer_tag(in,i,out,j,out_s); j--;

			if(in[i]!='>') return 0;

			tagged = true;
		}
		else if(tagged) //TODO: i18n
		{
			out[i] = in[i];
		}
	}

	if(!tagged) return in;

	out[j] = '\0'; return out;	
}  
static void som_lexer_tag(const wchar_t *in, int &i, wchar_t *out, int &j, size_t out_s)
{
	assert(in[i]=='<');

	const wchar_t *p = in+i+1, *q = p; 

	while(*q&&*q!=' '&&*q!='>') q++; if(*q!='>'&&*q!=' ') return;
		
	const wchar_t *d = q; while(*d&&*d!='>') d++; if(*d!='>') return;

	if(q[-1]=='/') q--; //<tag/> syntax

	size_t &_s = out_s?out_s:som_lexer_tmp_s;

	wchar_t *txt = 0; size_t len = 0;

	switch(*p) //hack
	{
	case 'f': case 'F':

		if(!wcsncmp(L"fps",p,q-p))
		{			
			static wchar_t fps[32];

			*fps = '\0'; _itow(int(EX::clip),fps,10);

			if(!*fps) return;

			if(fps[1]=='\0') //pad
			{
				fps[1] = ' '; fps[2] = '\0';
			}

			txt = fps;
		}	  
		break;
	}

	if(!txt) return;

	if(txt&&*txt&&!len) len = wcslen(txt);
		
	if(j+len>_s)
	{
		if(out_s) return;

		//expand temporary buffer

		assert(0); //unimplemented 

		return;
	}

	wcsncpy(out+j,txt,len); j+=len;

	i+=d-p+1; assert(in[i]=='>');
}


