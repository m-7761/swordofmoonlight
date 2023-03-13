
#include "Ex.h" 
EX_TRANSLATION_UNIT

#include <set>
#include <string>

#include "Ex.ini.h"
#include "Ex.langs.h"
#include "Ex.regex.h"

#include "som.932.h"
#include "som.932w.h"
#include "som.title.h"
#include "som.state.h"
#include "som.status.h"
#include "som.menus.h"

#include "som.enums.inl" //SOM_MENUS_INCLUDED

namespace DDRAW{ extern bool doFlipEx; }

static SOM::Menu::Element som_menus_case('case');
static SOM::Menu::Element som_menus_cond('cond');
static SOM::Menu::Element som_menus_cont('cont');
static SOM::Menu::Element som_menus_done('done');
static SOM::Menu::Element som_menus_item('item');
static SOM::Menu::Element som_menus_none('none');
static SOM::Menu::Element som_menus_trap('trap');

static SOM::Menu::Node *som_menus_pseudo_node = som_menus_none;
static SOM::Menu::Elem *som_menus_pseudo_elem = som_menus_none;
static SOM::Menu::Text *som_menus_pseudo_text = som_menus_none;

static SOM::Menu::Namespace som_menus_pseudo_namespace = 
{
	&som_menus_pseudo_node, &som_menus_pseudo_elem, &som_menus_pseudo_text
};

static SOM::Menu::Element *som_menus[SOM::Menu::TOTAL_SORTING][SOM::TOTAL_MENUS];

static int som_menus_nodes[SOM::TOTAL_MENUS];
static int som_menus_elements[SOM::TOTAL_MENUS];
static int som_menus_text[SOM::TOTAL_MENUS];

static SOM::Menu::Namespace som_menus_nslookups[SOM::Menu::TOTAL_SORTING][SOM::TOTAL_MENUS];

static SOM::Menu::Namespace *som_menus_namespaces_as_compiled = som_menus_nslookups[0];
static SOM::Menu::Namespace *som_menus_namespaces_by_uniqueid = som_menus_nslookups[1];
static SOM::Menu::Namespace *som_menus_namespaces_by_nodename = som_menus_nslookups[2];

const SOM::Menu::Namespace *SOM::Menu::namespaces = som_menus_namespaces_as_compiled;

namespace SOM
{	
#define PSEUDO(MENU) namespace MENU{ static const int NS = 0; }

	PSEUDO(YES) PSEUDO(NO) PSEUDO(MENU) PSEUDO(DATA) PSEUDO(FREE)

#undef PSEUDO

#define SOM_MENUS_X //som.menus.inl

#define NAMECLOSE
#define NAMESPACE(NAME)

#define MENU(X,f,e,x,ja,en) Menu::Elem X::x('menu',f+1,0/*#x*/,X::NS,ja,sizeof(ja)-1,en,e::NS);
#define STOP(X,f,e,x,ja,en) Menu::Elem X::x('stop',f+1,0/*#x*/,X::NS,ja,sizeof(ja)-1,en,0);
#define TEXT(X,f,e,x,ja,en) Menu::Text X::x('text',f+1,0/*#x*/,X::NS,ja,sizeof(ja)-1,en);
#define FLOW(...)	

#include "som.menus.inl"

#undef MENU
#undef STOP
#undef TEXT
#undef FLOW
#undef NAMECLOSE
#undef NAMESPACE

#undef SOM_MENUS_X

}//SOM

typedef int (*som_menus_cmp)(const SOM::Menu::Node*,const SOM::Menu::Node*); //qsorting

static int som_menus_cmp_ns(const SOM::Menu::Node*,const SOM::Menu::Node*); //namespace
static int som_menus_cmp_nn(const SOM::Menu::Node*,const SOM::Menu::Node*);	//node name
static int som_menus_cmp_id(const SOM::Menu::Node*,const SOM::Menu::Node*);	//unique id

static const som_menus_cmp som_menus_cmp_by[SOM::Menu::TOTAL_SORTING] =
{
	0, som_menus_cmp_nn, som_menus_cmp_id
};

static inline void som_menus_quicksort(som_menus_cmp,SOM::Menu::Node**,int sz);
static inline void som_menus_quicksort(som_menus_cmp,SOM::Menu::Elem**,int sz);
static inline void som_menus_quicksort(som_menus_cmp,SOM::Menu::Text**,int sz);

static SOM::Menu::Node *som_menus_binlookup(som_menus_cmp,const SOM::Menu::Node*,SOM::Menu::Node**,int sz);
static SOM::Menu::Elem *som_menus_binlookup(som_menus_cmp,const SOM::Menu::Node*,SOM::Menu::Elem**,int sz);
static SOM::Menu::Text *som_menus_binlookup(som_menus_cmp,const SOM::Menu::Node*,SOM::Menu::Text**,int sz);

static bool som_menus_initializing = 0; //paranoia

static inline bool som_menus_assert(bool x){ assert(x); return x; } //paranoia

static SOM::Menu::Namespace *som_menus_prep_namespace(int in) //initialize helper
{	
	if(!som_menus_assert(som_menus_initializing)) return 0; //paranoia

	int n = som_menus_nodes[in], e = som_menus_elements[in], t = som_menus_text[in];

	int ndx = (n+e+t+3)*SOM::Menu::TOTAL_SORTING;

	SOM::Menu::Node **buf = new SOM::Menu::Node*[ndx]; memset(buf,0x00,sizeof(void*)*ndx);

	SOM::Menu::Namespace *ns = 0; 

	for(int i=0;i<SOM::Menu::TOTAL_SORTING;i++)
	{
		ns = &som_menus_nslookups[i][in];

		ns->nodes = buf; buf+=n+1; 
		ns->elements = (SOM::Menu::Element**)buf; buf+=e+1;	
		ns->text = (SOM::Menu::Text**)buf; buf+=t+1;
	}

	return &som_menus_namespaces_as_compiled[in];
}

static void som_menus_copy_namespace(int in) //initialize helper
{
	if(!som_menus_assert(som_menus_initializing)) return; //paranoia

	int n = som_menus_nodes[in], e = som_menus_elements[in], t = som_menus_text[in];

	int ndx = n+e+t+3;

	SOM::Menu::Namespace &ns = som_menus_nslookups[0][in]; 

	for(int i=1;i<SOM::Menu::TOTAL_SORTING;i++)
	{
		SOM::Menu::Namespace &cp = som_menus_nslookups[i][in]; 

		memcpy(cp.nodes,ns.nodes,ndx*sizeof(void*));
	}
}

static void som_menus_sort_namespace(int in) //initialize helper
{
	if(!som_menus_assert(som_menus_initializing)) return; //paranoia

	for(int i=1;i<SOM::Menu::TOTAL_SORTING;i++)
	{
		SOM::Menu::Namespace &ns = som_menus_nslookups[i][in];	

		//scaling back going forward. This is unlikely to ever be used
		if(i==SOM::Menu::SORT_NODENAME&&!SOM::MAIN::Main.nn) continue;

		som_menus_quicksort(som_menus_cmp_by[i],ns.nodes,som_menus_nodes[in]);		
		som_menus_quicksort(som_menus_cmp_by[i],ns.elements,som_menus_elements[in]);		
		som_menus_quicksort(som_menus_cmp_by[i],ns.text,som_menus_text[in]);  		
	}	
}
					   
static void som_menus_onoption_callback(int ns, int code)
{
	switch(code)
	{
	case 0: SOM::option = SOM::frame; break;
	case 1: SOM::option = 0;
	}
}

static void som_menus_onpadcfg_callback(int ns, int code)
{
	switch(code)
	{
	case 0: //padcfg menu looks like dialog
		
		SOM::dialog = 0; //but isn't
		
		SOM::padcfg = SOM::frame;  break;

	case 1: SOM::padcfg = 0;
	}
}

static void som_menus_onsaving_callback(int ns, int code)
{
	switch(code)
	{
	case 0: SOM::saving = SOM::frame; break;
	case 1: SOM::saving = 0;
	}
}

namespace som_menus_x
{
static char out[64];
namespace x = som_menus_x;
static const char *ko(const char*,int)
{
	return ""; 
}
static const char *et(const char*,int)
{
	strcpy(x::out,"\n\n"); //som_menus_x::time
	const char *f = SOM::transl8(SOM::MAIN::_Time.id,SOM::MAIN::_Time.en);
	_sprintf_s_l(out+2,sizeof(out)-2,f,EX::Locale::C,SOM::et/3600,SOM::et%3600/60,SOM::et%3600%60);
	return out;
}
static const char *time(const char*, int)
{	
	if((SOM::L.pcstatus[SOM::PC::xxx]&0x1F)==0x1F)	
	return ""; //hack: make way for 5th status ailment
	strcpy(x::out,"\n\n"); //som_menus_x::et
	const char *x = SOM::transl8(SOM::MAIN::Time.id,SOM::MAIN::Time.en);
	strcpy_s(out+2,sizeof(out)-2,x); return out;
}		
static const char 
*pads(const char*,int),*padcfg(const char*,int),
*buttons(const char*,int),*strftime(const char*,int),
*colors(const char*,int),*colormodes(const char*,int),
*anisotropy(const char*,int),*texfilters(const char*,int),
*classnames(const char*,int),
*nonequipment(const char*,int),*shadow_tower(const char*,int),
*mode(int),*sellout(const char*,int);
const char *mode0(const char*,int){ return mode(0); }
const char *mode1(const char*,int){ return mode(1); }
}

static const char *som_menus_white(const char*,const char*,int);

void SOM::initialize_som_menus_cpp() //initialization
{
	static int one_off = 0; if(one_off++) return; //???

	som_menus_initializing = true; //paranoia

	int i; SOM::Menu::Namespace *ns = 0;
	for(i=1;i<SOM::Menu::TOTAL_SORTING;i++)	
	som_menus_nslookups[i][0] = som_menus_pseudo_namespace;
	
#define SOM_MENUS_X //som.menus.inl

#define NAMESPACE(NAME)\
\
	som_menus_nodes[SOM::NAME##_NS]    = SOM::NAME::TOTAL_NODES;\
	som_menus_elements[SOM::NAME##_NS] = SOM::NAME::TOTAL_ELEMENTS;\
	som_menus_text[SOM::NAME##_NS]     = SOM::NAME::TOTAL_TEXT;\
\
	ns = som_menus_prep_namespace(SOM::NAME##_NS);\
\
	for(i=0;i<SOM::NAME::TOTAL_NODES;i++)\
	{

#define TEXT(X,f,e,x,...) ns->nodes[i++] = SOM::X::x,
#define MENU(X,f,e,x,...) ns->nodes[i++] = SOM::X::x,
#define STOP(X,f,e,x,...) ns->nodes[i++] = SOM::X::x,
#define FLOW(X,c,remark)  ns->nodes[i++] = som_menus_##c, //static

#define NAMECLOSE ns->nodes[i] = 0;\
	}

#include "som.menus.inl"

#define NAMESPACE(NAME)\
\
	ns = &som_menus_namespaces_as_compiled[SOM::NAME##_NS];\
\
	for(i=0;i<SOM::NAME::TOTAL_ELEMENTS;i++)\
	{

#define TEXT(X,f,e,x,...)
#define MENU(X,f,e,x,...) ns->elements[i++] = SOM::X::x,
#define STOP(X,f,e,x,...) ns->elements[i++] = SOM::X::x,
#define FLOW(X,c,remark)  ns->elements[i++] = som_menus_##c, //static

#define NAMECLOSE ns->elements[i] = 0;\
	}
	
#include "som.menus.inl"

#define NAMESPACE(NAME)\
\
	ns = &som_menus_namespaces_as_compiled[SOM::NAME##_NS];\
\
	for(i=0;i<SOM::NAME::TOTAL_TEXT;i++)\
	{

#define TEXT(X,f,e,x,...) ns->text[i++] = SOM::X::x,
#define MENU(X,f,e,x,...) ns->text[i++] = SOM::X::x,
#define STOP(X,f,e,x,...) ns->text[i++] = SOM::X::x,
#define FLOW(X,c,remark)  ns->text[i++] = som_menus_##c, //static

#define NAMECLOSE ns->text[i] = 0;\
\
	}

#include "som.menus.inl"

#define MENU(...)
#define STOP(...)
#define TEXT(...)
#define FLOW(...)

#define NAMECLOSE
#define NAMESPACE(NAME)\
\
	som_menus_copy_namespace(SOM::NAME::NS);\
	som_menus_sort_namespace(SOM::NAME::NS);

#include "som.menus.inl"

#undef TEXT
#undef MENU
#undef STOP
#undef FLOW
#undef NAMECLOSE
#undef NAMESPACE

#undef SOM_MENUS_X
	
	for(i=1;i<SOM::Menu::TOTAL_SORTING;i++) 
	{
		som_menus[i][0] = som_menus_none;
	}
	for(i=1;i<SOM::TOTAL_MENUS;i++) //build menu lookup via top elements
	{
		ns = &som_menus_namespaces_as_compiled[i];	  
		for(int j=0;j<SOM::Menu::TOTAL_SORTING;j++)	som_menus[j][i] = ns->elements[0];
	}
	for(int i=0;i<SOM::Menu::TOTAL_SORTING;i++)	  
	{
		//scaling back going forward. This is unlikely to ever be used
		if(i==SOM::Menu::SORT_NODENAME&&!SOM::MAIN::Main.nn) continue;
		som_menus_quicksort(som_menus_cmp_by[i],som_menus[i],SOM::TOTAL_MENUS);
	}
	
	//translation callbacks
	namespace x = som_menus_x;

	//garbage that appears on screen from time to time
	SOM::ASCII::Ascii->cb = x::ko; SOM::ASCII::AsciiB.cb = x::ko;
	SOM::ASCII::AsciiC.cb = x::ko; SOM::ASCII::AsciiD.cb = x::ko;
	SOM::ASCII::AsciiE.cb = x::ko; SOM::ASCII::AsciiF.cb = x::ko;
	SOM::ASCII::Ascii->re = EX::Regex::EXACT; //% sign

	EX::INI::Script lc;	EX::INI::Option op;	EX::INI::Bugfix bf;
	
	//if(SOM::L.pcstatus) //compuslory for now
	{	
		SOM::MAIN::Mode.cb = x::mode0;
		SOM::STATS::Mode.cb = x::mode0;
		SOM::SYS::Mode.cb = x::mode0;
		SOM::MAIN::_Mode.cb = x::mode1;
		SOM::STATS::_Mode.cb = x::mode1;
		SOM::SYS::_Mode.cb = x::mode1;
	}

	if(SOM::image()=='db2') //som_state_408400 
	{
		//manage clock and push down 2 lines
		SOM::MAIN::_Time.cb = x::et;
		SOM::STATS::_Time.cb = x::et;
		SOM::SYS::_Time.cb = x::et;
		//adds \n to match x::et
		//and hides label when five modes are on
		SOM::MAIN::Time.cb = x::time;
		SOM::STATS::Time.cb = x::time;
		SOM::SYS::Time.cb = x::time;
	}	

	SOM::CONFIG::Config->cb = x::padcfg;

	SOM::OPTION::PadID.cb = x::pads;
	SOM::CONFIG::PadID.cb = x::pads;
	//SOM::CONFIG::_PadID.cb = x::ko;

	if(!lc->do_not_show_button_names)
	{
	SOM::CONFIG::Cfg1->cb = x::buttons;
	SOM::CONFIG::Cfg2->cb = x::buttons;
	SOM::CONFIG::Cfg3->cb = x::buttons;
	SOM::CONFIG::Cfg4->cb = x::buttons;
	SOM::CONFIG::Cfg5->cb = x::buttons;
	SOM::CONFIG::Cfg6->cb = x::buttons;
	SOM::CONFIG::Cfg7->cb = x::buttons;
	SOM::CONFIG::Cfg8->cb = x::buttons;	
	}

	if(lc->do_use_builtin_locale_date_format)
	{
		SOM::SAVE::_Date.cb = x::strftime;
		SOM::LOAD::_Date.cb = x::strftime;
	}

	if(op->do_highcolor||op->do_bpp||op->do_opengl)
	{
		if(op->do_opengl||op->do_highcolor)
		SOM::OPTION::Color->cb = x::colors;
		SOM::OPTION::_Color.cb = x::colormodes;    
	}

	if(op->do_anisotropy)
	SOM::OPTION::Filter->cb = x::anisotropy;
	SOM::OPTION::_Filter.cb = x::texfilters;

	SOM::OPTION::Option.on = som_menus_onoption_callback;
	SOM::CONFIG::Config.on = som_menus_onpadcfg_callback; 
	SOM::SAVE::Save.on     = som_menus_onsaving_callback;

	if(op->do_st)
	{
		SOM::OPTION::Hud->cb = x::shadow_tower;
		SOM::OPTION::Nav->cb = x::shadow_tower;
		SOM::OPTION::Inv->cb = x::shadow_tower;
		SOM::OPTION::Bob->cb = x::shadow_tower;
	}

	//simplify handling of %s 
	SOM::EQUIP::None.cb = x::nonequipment;
	SOM::MAIN::_Class.cb = x::classnames;
	SOM::STATS::_Class.cb = x::classnames;
	SOM::SYS::_Class.cb = x::classnames;

	//compound bug of some kind???
	SOM::SELL::Amount.cb = x::sellout;

	SOM::INFO::Free.cb = x::ko;

	som_menus_initializing = false; //paranoia

}//SOM::initialize_the_menu_model()

static SOM::Menu::Namespace *som_menus_namespaces(int in)
{
	SOM::Menu::Namespace *out = 0;	
	if(in>0&&in<SOM::TOTAL_MENUS) out = &som_menus_namespaces_as_compiled[in];
	return out;
}

static SOM::Menu &som_menus_from_namespace(int in)
{
	static SOM::Menu out;	 
	SOM::Menu::Namespace *ns = som_menus_namespaces(in);
	if(!ns) return *(new(&out)SOM::Menu());

	const char *id = ns->elements[0]->id;
	void (*on)(int,int) = ns->elements[0]->on;
	return *(new(&out)SOM::Menu(in,id,on,ns->nodes,ns->elements,ns->text));
}

extern SOM::Menu SOM::getMenuByNs(int in)
{
	return som_menus_from_namespace(in);
}

extern SOM::Menu SOM::getMenuById(const char *in, int in_s)
{		
	if(!in||!*in) return som_menus_from_namespace(0);

	if(*in=='P'&&!strncmp(in,"PadID: ",7)) //pad config hack
	{
		return som_menus_from_namespace(SOM::CONFIG::PadID.ns);
	}
	
	SOM::Menu::Node cmp(0,0,in,in_s?in_s:strlen(in),0), *out = 
	som_menus_binlookup(som_menus_cmp_id,&cmp,som_menus[SOM::Menu::SORT_UNIQUEID],SOM::TOTAL_MENUS);

	static const char *question_mark = som_932_Take+sizeof(som_932_Take)-3;
	if(!out&&cmp.sz>=sizeof(som_932_Take)-3&&!memcmp(in+cmp.sz-2,question_mark,2))
	{
		//SOM::TAKE (a difficult case)
		static const int *re = EX::Regex::LOST; 		
		if(!*re) re = EX::Regex(932,SOM::TAKE::Take.id,SOM::TAKE::Take.sz);
		if(EX::Regex(re).match(932,in,in_s))
		{
			out = &SOM::TAKE::Take; SOM::taking = SOM::frame; //bugfix
		}
	}	

	static int sync = -1; //SOM::dialog

	if(sync!=SOM::frame) //ie. at top of frame
	{
		if(!out) SOM::dialog = SOM::frame; //not a menu: must be text

		sync = SOM::frame; 
	}

	return som_menus_from_namespace(out?out->ns:0);
}

const SOM::Menu::Element **SOM::Menu::Element::top()const
{
	SOM::Menu::Namespace *out = 0;	
	if(menu>0&&menu<SOM::TOTAL_MENUS) out = som_menus_namespaces(menu);
	return out?(const SOM::Menu::Elem**)out->elements:0; 
}

static void som_menus_match_against(const void*); 

static const int *som_menus_regex(const char *id, int id_s); 

static bool som_menus_exact(const char *id, int sz, const char *in, int in_s);

static bool som_menus_match(EX::Regex,const char*,int);
static bool som_menus_match_only(EX::Regex,const char*,int);

static const char *som_menus_printf_chars(EX::Regex);
static const char **som_menus_parse_match(const void*);
		
bool SOM::Menu::Text::irregular(void *procA)const
{
	if(cc!='text') return false;

	switch(op)
	{	
	case 1: if(procA!=::TextOutA) return false; break;
	case 2:	if(procA!=::DrawTextA) return false; break;

	default: return false;
	}

	return sz==1&&*id=='*';
}

bool SOM::Menu::Text::match(void *procA, const char *in, int in_s)const
{
	som_menus_match_against(this); 

	if(cc!='text') return false;

	switch(op)
	{	
	case 1: if(procA!=::TextOutA) return false; break;
	case 2:	if(procA!=::DrawTextA) return false; break;

	default: return false;
	}

	if(!*re) re = som_menus_regex(id,sz); 
			
	if(*re<=EX::Regex::SIMPLE_CASES)
	{
		switch(*re)
		{
		case EX::REGEX_EXACT:
			
			return som_menus_exact(id,sz,in,in_s);
		
		case EX::REGEX_IFANY:
			
			return in&&*in&&in_s; 
		
		case EX::REGEX_ALWAYS:
		case EX::REGEX_WHITE:
		
			return true; //always match		

		default: return false;
		}
	}
	
	if(!in||!*in||!in_s) return false; //optimization??
	
	if(cb) return som_menus_match_only(re,in,in_s); 
	
	return som_menus_match(re,in,in_s); 
}

const char **SOM::Menu::Text::parse()const
{
	return som_menus_parse_match(this);
}		  
const char *SOM::Menu::Text::format()const
{
	if(cb) return 0; //does matching only
	if(!*re) re = som_menus_regex(id,sz);
	return *re<=EX::Regex::SIMPLE_CASES?0:id;
}		  
const char *SOM::Menu::Text::fchars()const
{
	if(cb) return 0; //does matching only

	if(!*re) re = som_menus_regex(id,sz); 

	if(*re<=EX::Regex::SIMPLE_CASES) return 0;

	return som_menus_printf_chars(re);
}		

const char *SOM::Menu::Text::translate(const char *in, int in_s)const
{
	if(!id||!*id) return id; //paranioa?

	const char *x = cb?cb(in,in_s):0; if(x) return x; //callback

	if(*id=='*') 
	{
		if(id[1]=='\0') //irregular
		{
			return 0; assert(0); //probably shouldn't be happening
		}
		else if(id[1]=='W'&&!strcmp(id,"*WS*"))
		{
			return som_menus_white(pf,in,in_s); 
		}
		assert(0); return 0;
	}
	else assert(*en!='*');

	//REMOVE ME?
	const char *u = pf; int u_s = sz; if(u) //msgctxt
	{
		u_s+=id-pf;
		if(EX::Decode(932,&u,&u_s)!=65001) return 0; //paranoia
		x = SOM::gettext(u); if(x!=u) return x;
		u = strchr(u,'\4')+1; u_s = strlen(u);
	}
	else if(EX::Decode(932,&(u=id),&u_s)!=65001) return 0; //paranoia	
	x = SOM::gettext(u);
	if(x==u&&!SOM::japanese())
	if(EX::INI::Script()->do_use_builtin_english_by_default)
	return en; return x;
}

static void som_menus_quicksort
(som_menus_cmp f, SOM::Menu::Node **in, int sz, int lo, int hi) 
{
	if(lo>=hi) return; SOM::Menu::Node *swap = 0; 
    	
	int r = rand(); r = lo<hi?lo+r%(hi-lo):hi+r%(lo-hi);

	swap = in[lo]; in[lo] = in[r]; in[r] = swap;

	int pivot = lo; SOM::Menu::Node **p = &in[pivot];
	
	for(int unknown=pivot+1;unknown<=hi;unknown++) if(f(in[unknown],*p)<0)
	{   		
		pivot++; swap = in[unknown]; in[unknown] = in[pivot]; in[pivot] = swap; 
	}

	swap = in[lo]; in[lo] = in[pivot]; in[pivot] = swap; 

	if(pivot) som_menus_quicksort(f,in,sz,lo,pivot-1); 
	
	if(pivot<=sz) som_menus_quicksort(f,in,sz,pivot+1,hi);
}

static inline void som_menus_quicksort(som_menus_cmp cmp, SOM::Menu::Node **id, int sz)
{
	if(cmp) som_menus_quicksort(cmp,id,sz,0,sz-1); 
}
static inline void som_menus_quicksort(som_menus_cmp cmp, SOM::Menu::Elem **id, int sz)
{
	if(cmp) som_menus_quicksort(cmp,(SOM::Menu::Node**)id,sz,0,sz-1);
}
static inline void som_menus_quicksort(som_menus_cmp cmp, SOM::Menu::Text **id, int sz)
{
	if(cmp) som_menus_quicksort(cmp,(SOM::Menu::Node**)id,sz,0,sz-1);
}

static SOM::Menu::Node *som_menus_binlookup
(som_menus_cmp f, const SOM::Menu::Node *in, SOM::Menu::Node **id, int sz)
{	
	if(!in||!id) return 0;

	int cmp, first = 0, last = sz-1, stop = -1;

	int x;
	for(x=(last+first)/2;x!=stop;x=(last+first)/2)
	{
		cmp = f(id[x],in); if(!cmp) return id[x]; 

		if(cmp>0) stop = last = x; else stop = first = x;
	}	  
	if(last==sz-1&&x==last-1) //round-off error
	{
		if(!f(id[last],in)) return id[last];
	}	  	
	return 0; //catch all
}			  

static inline SOM::Menu::Element *som_menus_binlookup
(som_menus_cmp f, const SOM::Menu::Node *in, SOM::Menu::Element **id, int sz)
{
	return (SOM::Menu::Element*)som_menus_binlookup(f,in,(SOM::Menu::Node**)id,sz);
}			  
static inline SOM::Menu::Text *som_menus_binlookup
(som_menus_cmp f, const SOM::Menu::Node *in, SOM::Menu::Text **id, int sz)
{
	return (SOM::Menu::Text*)som_menus_binlookup(f,in,(SOM::Menu::Node**)id,sz);
}

static int som_menus_cmp_ns(const SOM::Menu::Node *a, const SOM::Menu::Node *b)
{
	return a->ns-b->ns;
}	
static int som_menus_cmp_id(const SOM::Menu::Node *a, const SOM::Menu::Node *b)
{
	if(a->sz==0||b->sz==0) if(a->sz>0) return +1; else return b->sz==0?0:-1;
	
	int sz = min(a->sz,b->sz), out = memcmp(a->id,b->id,sz); 
	
	return out?out:a->id[sz]?1:b->id[sz]?-1:0;
}			
static int som_menus_cmp_nn(const SOM::Menu::Node *a, const SOM::Menu::Node *b)
{
	assert(!"unused?"); return strcmp(a->nn,b->nn);
}		

static char som_menus_match_buffer[256];
static const void *som_menus_match_check = 0;
static const char *som_menus_parse_buffer[8+1];
static void som_menus_match_against(const void *check)
{
	som_menus_match_check = check; som_menus_parse_buffer[0] = 0;
}

static const int *som_menus_regex(const char *id, int id_s)
{
	if(!id||!id_s) return EX::Regex::EMPTY; //paranoia??
	
	if(*id=='*')
	{
		if(id_s==1) return EX::Regex::IFANY; //or ALWAYS??

		switch(id[1])
		{
		case 'W': if(!strcmp(id,"*WS*")) return EX::Regex::WHITE;

		default: assert(0); //this should not be happening
			
			return EX::Regex::NEVER; //red flag
		}
	}
		
	//hack?? printf format spec  
	int i = 0; while(i<id_s&&id[i]!='%') i++; 

	//warning: does not do %% (escape sequence) 	
	if(id[i]!='%') return EX::Regex::EXACT;

	const int *out = EX::Regex(932,id,id_s);

	if(!out) return EX::Regex::NEVER;

	return out;
}

static bool som_menus_exact(const char *id, int sz, const char *in, int in_s)
{		
	if(!id||!in||sz<0||in_s<0) return false; //paranoia

	return sz==in_s?!memcmp(id,in,in_s):false;	
}

static bool som_menus_match(EX::Regex regex, const char *in, int in_s)
{	
	if(*regex<=EX::Regex::SIMPLE_CASES) 
	{
		assert(0); return false; //should not be called for trivial cases
	}

	if(!in) in = ""; if(!in_s&&*in) in_s = strlen(in);
	
	char *out = som_menus_match_buffer;
	int out_s = sizeof(som_menus_match_buffer);
	
	bool match = false; regex.reset(); //paranoia

	while((match=regex.match(932,&in,&in_s,&out,&out_s))&&in_s&&out_s);

	if(match&&in_s) //out_s was too small
	{
		assert(0); return false; //unimplemented				
	}
	else if(!match) return false; //not parsing mismatches
	
	int parse = regex.parse(); //paranoia	
	if(parse>=sizeof(som_menus_parse_buffer)) 
	{
		assert(0); return false; //unimplemented
	}
	if(regex.parse(som_menus_parse_buffer,
	EX_ARRAYSIZEOF(som_menus_parse_buffer)-1)!=parse) //todo: partial
	{
		assert(0); return false; //unimplemented
	}
	som_menus_parse_buffer[parse] = 0;

	return match;
}
static bool som_menus_match_only(EX::Regex regex, const char *in, int in_s)
{
	if(*regex<=EX::Regex::SIMPLE_CASES) 
	{
		assert(0); return false; //should not be called for trivial cases
	}

	if(!in) in = ""; if(!in_s&&*in) in_s = strlen(in);

	return regex.match(932,in,in_s); 
}

static const char **som_menus_parse_match(const void *check)
{
	if(som_menus_match_check!=check) return 0;

	if(!*som_menus_parse_buffer) return 0; //warning! not good enough

	return som_menus_parse_buffer;
}

static const char *som_menus_printf_chars(EX::Regex regex)
{
	if(*regex<=EX::Regex::SIMPLE_CASES) return 0;

	static char som_menus_printf_chars_buffer[10];

	const char *out = regex.printf_type_field_characters();
	
	if(out&&strcpy_s(som_menus_printf_chars_buffer,10,out))
	{
		assert(0); return 0; //unimplemented 
	}
	
	return out?som_menus_printf_chars_buffer:0;
}

static const char *som_menus_white(const char *pf, const char *in, int in_s)
{
	static char out[90]; out[0] = '\0';
	
	int out_x = 0; if(!in) return 0; if(!*in||!in_s) return "";

	char *q = out; const char *p = in, *u = in; int u_s = in_s;

	if(EX::Decode(932,&u,&u_s)!=65001) return 0; //paranoia

	const char *x = SOM::gettext(u); 
	if(x!=u) return strcpy_s(out,x)?0:out; //explosion unnecessary
	
	int pf_s = pf?strchr(pf,'\4')-pf+1:0; 
	if(pf) q = (char*)memcpy(out,pf,pf_s)+pf_s; assert(pf_s<sizeof(out));

#define WHITE *p==' '||*p=='\t'||*p=='\r'||*p=='\n' //TODO: Shift_JIS whitespace

	int i = 0; if(WHITE) goto white; //it's important i is initialized first

	for(i=0;;i++)
	{	
		if(int(q-out)==sizeof(out)) return 0; //hack

		if(!*p||i>=in_s||WHITE)
		{
			*q = '\0'; u = out+out_x;

			if(EX::Decode(932,&u)!=65001) return 0; //paranoia

			x = SOM::gettext(u); 

			if(pf&&x==u) x = SOM::gettext(u=strchr(u,'\4')+1); 

			if(x==u&&!SOM::japanese()) //English?
			if(EX::INI::Script()->do_use_builtin_english_by_default)
			x = SOM::getTermById(out+out_x+pf_s).en; //English
			if(x) u = x; 
			
			int ulen = strlen(u);
			if(out_x+ulen>=sizeof(out)) return 0; //hack

			memcpy(out+out_x,u,ulen); out_x+=ulen;			

			q = out+out_x; *q = '\0';

white:		while(*p&&(WHITE)&&i++<in_s) 
			{
				*q++ = *p++; if(int(q-out)==sizeof(out)) return 0; //hack
			}

			out_x = int(q-out);

			if(!*p||i>=in_s) break;

			if(pf&&memcpy_s(out+out_x,sizeof(out)-out_x,pf,pf_s)) return 0; 
			if(pf) q+=pf_s;
		}
		
		*q++ = *p++;		
	}	

	//return out; //trimming

#define WHITE *q==' '||*q=='\t'||*q=='\r'||*q=='\n' 

	if(out_x) for(q=out+out_x-1;q!=out&&(WHITE);*q--='\0'); //rtrim
	if(out_x) for(q=out;*q&&(WHITE);q++); return q; //ltrim

#undef WHITE

}
  
static const char *Press_any_button()
{
	return SOM::transl8(som_932_Press_any_button,"Press any button");
}
static const char *Now_press_another()
{
	return SOM::transl8(som_932_Now_press_another,"Now press another");
}									  
static const char *som_menus_x::padcfg(const char *in, int in_s)
{	
	static int past = 0; static bool down[16]; 
	
	if(SOM::doubleblack>SOM::frame-2)
	{
		SOM::padtest = past = 0; return Press_any_button();
	}
	else if(!SOM::Buttons(down,16)) return 0;

	int test = past; past = 0; 
	
	static unsigned block = 0, first = 0;

	for(int i=0;i<16;i++) if(down[i])
	{
		if(!first) first = i+1; past++;
	}

	if(past==0) first = 0;

	if(block<SOM::padcfg-1)
	{
		SOM::padtest = false;

		if(!past) block = SOM::padcfg;

		return Press_any_button();
	}
	else block = SOM::padcfg;

	//allow for yes/no prompt
	static int twoframes = 0; 

	if(!test) 
	{		
		twoframes = 0;

		SOM::padtest = false;

		return Press_any_button();
	}
	else if(test==1&&twoframes)
	{
		SOM::padtest = true;

		return Now_press_another();
	}
	else if(!twoframes++)
	{
		return Press_any_button();
	}

	static wchar_t wout[24];
	{
		int i = 0; //NEW
		if(first<=8) for(i=0;i<8;i++)
		wout[i] = down[i]?som_932w_Buttons[i+1]:som_932w_Buttons[0];		
		else for(i=0;i<8;i++) //NEW
		wout[i] = down[8+i]?som_932w_Buttons[8+i+1]:som_932w_Buttons[0];
		wout[i] = '\0';
	}
	
	const char *out = 0; int out_x = wcslen(wout);

	if(EX::Decode(wout,&out,&out_x)==65001) return out;

	return 0;
}
static const char *som_menus_x::buttons(const char *in, int in_s)
{
	const wchar_t *wout = 0;
	const int button = sizeof(som_932_Button)-1; 	
	if(button<in_s&&!memcmp(som_932_Button,in,button)) 
	{
		enum{ f_s=24 };
		wchar_t f[f_s] = L"", g[f_s] = L"";
		EX::need_unicode_equivalent(65001,SOM::gettext("(%d)"),f,f_s);
		wcscat_s(f,L" ");

		for(size_t i=1;i<=8;i++)
		if(!strcmp(in+button,som_932_Numerals[i])) //lazy
		{
			swprintf_s(g,f,i); wout = SOM::Joypad(i-1,g);
		}
		assert(wout); 
	}
	if(!wout||!*wout) return 0;
	const char *out = 0; int out_x = wcslen(wout);
	if(EX::Decode(wout,&out,&out_x)==65001) return out;
	return 0;
}
static const char *som_menus_x::pads(const char *in, int in_s)
{	
	//TODO: GIVE ORDINALS TO SOM AND LOOKUP THE FULL INSTANCE TITLE
	//(currently som.hacks.cpp is adding the PadID: prefix to them)

	if(strncmp(in,"PadID: ",7)){ assert(0); return 0; } 
	
	SOM::Menu::Text &PadId = //HACK: permit msgctxt
	SOM::frame-SOM::option>1?SOM::CONFIG::PadID:SOM::OPTION::PadID;
	const char *pf = PadId.pf; int pf_s = PadId.id-PadId.pf;
	if(EX::Decode(932,&pf,&pf_s)==65001) //paranoia
	strcpy((char*)pf+pf_s,"%s"); else return in+7; //"%-42.42s
	const char *f = SOM::gettext(pf); 
	if(f==pf) f = SOM::gettext(pf+pf_s); 
	//callbacks must do their formatting by themself
	x::out[0] = '\0';
	sprintf_s(out,f,in+7); return out;
}
static const char *som_menus_x::strftime(const char *in, int in_s)
{	
	if(!in||!in_s) return 0;
		
	static const int *re = EX::Regex::LOST;		  
	if(!*re) re = EX::Regex(932,som_932_Date,sizeof(som_932_Date)-1);	
	EX::Regex date(re); const char *yyyymmdd[3];
	if(!date.match(932,in,in_s)||date.parse(yyyymmdd,3)!=3) return 0;

	struct tm dt; memset(&dt,0x00,sizeof(dt));
	dt.tm_mon  = atoi(yyyymmdd[1]); if(dt.tm_mon<1||dt.tm_mon>12) return 0;
	dt.tm_mday = atoi(yyyymmdd[2]); if(dt.tm_mday<1||dt.tm_mday>31) return 0;
	dt.tm_year = atoi(yyyymmdd[0]); 	
	dt.tm_mon-=1; dt.tm_year-=1900; if(dt.tm_year<0) return 0;		 
	dt.tm_isdst = -1; //NEW: prevent 1 hour rollover

	//(contrary to docs _localtime_s does not exist)
	time_t mk = mktime(&dt); _localtime64_s(&dt,&mk); //day of the week

	const int out_s = 64; wchar_t wout[out_s];	
	//kind of annoying that the day is 0 prefixed. Eg. 01
	int out_x = _wcsftime_l(wout,out_s,L"%#x",&dt,EX::Locales[EX::Locale::USR]);

	const char *out = 0; //utf8 encoded output string	
	if(EX::Decode(wout,&out,&out_x)!=65001) return 0;
	return out;   
}
static const char *som_menus_x::colors(const char *in, int in_s)
{
	EX::INI::Option op;
	if(op->do_opengl)
	return SOM::transl8(som_932_Graphics,"Graphics");
	if(op->do_highcolor)
	return SOM::transl8(som_932_HighColor,"High Color");
	assert(0);
}
static const char *som_menus_x::colormodes(const char *in, int in_s)
{	
	if(!in||!in_s) return 0;

	static const int *re = EX::Regex::LOST;

	//TODO: look for author translation first
	if(!*re) re = EX::Regex(932,"%2d bpp",sizeof("%2d bpp")-1);

	EX::Regex bpp(re); if(!bpp.match(932,in,in_s)) return 0;

	const char *a; if(bpp.parse(&a,1)!=1) return 0;

	int bits = atoi(a);

	EX::INI::Option op;

	if(op->do_opengl)
	{
		switch(bits)
		{
		case 16: return ">>>>Direct3D 9 (PSVR)"; break;
		case 32: return ">>>>OpenGL 4.5 (PCVR)"; break;
		}
	}
	else if(op->do_highcolor)
	{
		switch(bits) 
		{
		case 16: return "5:5:5"; 
		case 32: return "5:6:5";
		}
	}
	else switch(bits) //do_bpp
	{			
	case 16: 
	
		if(EX::INI::Option()->do_green)		
		return SOM::transl8("16bpp",">>High Color"); 		
		return SOM::transl8("15bpp",">>High Color"); 

	case 32: return SOM::transl8("24bpp",">>True Color");
	}

	return 0;
}
static const char *som_menus_x::anisotropy(const char *in, int in_s)
{
	return SOM::transl8(som_932_Anisotropy,"Anisotropy");
}
static const char *som_menus_x::texfilters(const char *in, int in_s)
{	
	if(!in||!in_s) return 0; 
	
	while(*in==' ') in++; //trim

	const char *sic[3] = 
	{"Point filtering", "Biliner filtering","Triliner filtering"};
	const char *ids[3] = 
	{"Point filtering", "Bilinear filtering","Trilinear filtering"};

	int mode; switch(*in) 
	{
	case 'P': mode = 0; break;
	case 'B': mode = 1; break;
	case 'T': mode = 2; break; default: return 0;
	}
	if(strcmp(in,sic[mode])&&strcmp(in,ids[mode])) return 0; 

	const char *en[3] =
	{"Point Filter", "Bilinear Filter","Trilinear Filter"};
	const char *aniso[3] =
	{"4x (performance)","8x (high quality)","16x (best quality)"};	
	if(EX::INI::Option()->do_anisotropy)
	return SOM::transl8(som_932_AnisoModes[mode],aniso[mode]);
	return SOM::transl8(ids[mode],en[mode]);
}
static const char *som_menus_x::mode(int right)
{
	x::out[0] = '\0';
	
	int odd = 0, xxx = SOM::L.pcstatus[SOM::PC::xxx];

	//TODO: deal with hidden status effect (null string)
	if(xxx&1<<0) if(odd++%2==right) strcat_s(out,som_932_States[0]);
	if(xxx&1<<1) if(odd++%2==right) strcat_s(out,som_932_States[1]);
	if(xxx&1<<2) if(odd++%2==right) strcat_s(out,som_932_States[2]);
	if(xxx&1<<3) if(odd++%2==right) strcat_s(out,som_932_States[3]);
	if(xxx&1<<4) if(odd++%2==right) strcat_s(out,som_932_States[4]);
		
	const char *utf = som_menus_white(0,out,strlen(out)); 

	out[0] = out[1] = '\0'; //j=1

	for(int i=0,j=1;utf[i]&&j<sizeof(out)-1;i++,j++)
	{
		out[j] = utf[i]; if(utf[i]==' '&&i) out[++j] = '\n'; 
		
		if(!utf[i+1]) //terminating
		{
			if(right) out[++j] = ' '; out[j+1] = '\0';
		}
	}

	if(out[1]) out[0] = ' '; //j=1

	return out;
}
static const char *som_menus_x::shadow_tower(const char *in, int in_s)
{
	//For the record these are not strictly like ST

	if(!strcmp(in,som_932_Shadow_Tower[0]))
	{
		return SOM::transl8(som_932_Shadow_Tower[1],"Status Display");
	}
	else if(!strcmp(in,som_932_Shadow_Tower[2]))
	{
		return SOM::transl8(som_932_Shadow_Tower[3],"Health Display");
	}
	else if(!strcmp(in,som_932_Shadow_Tower[4]))
	{
		return SOM::transl8(som_932_Shadow_Tower[5],"Damage Display");
	}
	else if(!strcmp(in,som_932_Shadow_Tower[6]))
	{
		return SOM::transl8(som_932_Shadow_Tower[7],"Motion Capture");
	}
	else return 0;
}
static const char *som_menus_x::nonequipment(const char *in, int in_s)
{
	assert(in_s==20); if(in_s!=20||in[8]!=' '||in[7]==' ') return 0; 
	if(strncmp(in,SOM::EQUIP::None.id,SOM::EQUIP::None.sz)) return 0;
	return SOM::transl8(som_932_Nothing,SOM::EQUIP::None.en);
}
static const char *som_menus_x::classnames(const char *in, int in_s)
{
	//this exists so som.title.cpp's printf//
	//doesn't have to handle this lone case//

	const char *u = SOM::MAIN::_Class.pf;
	if(EX::Decode(932,&u)!=65001) return 0;
	const char *s = SOM::gettext(u); if(s==u) s = "%s";
	while(*in==' ') in++;
	if(EX::Decode(932,&in)!=65001) return 0;	
	while(*in==' ') in++; u = SOM::gettext(in,'SYS0'); 
	x::out[0] = '\0'; sprintf_s(out,s,u!=in?u:SOM::gettext(u));
	return out;
}
extern int som_state_sellout423480_beep;
extern bool som_state_sellout423480_beeped;
static const char *som_menus_x::sellout(const char *in, int in_s)
{
	static int was = 0; 
	if(*in++!='x') return 0; while(*in==' ') in++;
	int now = atoi(in);		
	if(now==1||now!=was)
	som_state_sellout423480_beeped = false;
	//when going from 2 to 1 manually play the beep
	if(now==1&&was==2&&som_state_sellout423480_beep)
	((void(__cdecl*)(DWORD))0x423480)(som_state_sellout423480_beep);
	was = now; assert(!SOM::SELL::Amount.pf);
	x::out[0] = '\0'; sprintf_s(out,SOM::gettext("x%3d"),now);
	return out;
}

bool som_menus_h::validator::operator()(void *procA, const char *in, int in_s)
{		   
	bool matched = false;
	const SOM::Menu::Text *condition = 0; //safety
	if(!text) 
	if(!menu.text) return false;
	else text = menu.text; 	
	else if(!*text) return false; //success?
	else text++; 
	if(!*text) //NEW: after text++ (spurious text?)
	{
		new(this)validator(); return false; //off the menu
	}
	top: switch((*text)->cc)
	{
	case 'case': 

		if(in_case) //leaving case
		{
			while(*text
			&&(*text)->cc!='done'
			&&(*text)->cc!='none'
			&&(*text)->cc!='cont') 
			text++;
			goto top;
		}
		else is_case = 1;
		text++; 
		goto top;

	case 'cond': 

		if(is_item) //delay rewinding
		{
			is_item = is_case = in_case = 0;

			if(in_item)
			{
				if(condition!=*text) 
				{
					condition = *text++; 
				}
				else text = 0; //infinite loop

				in_item = 0; is_cond = 1; 
			}
			else text++; goto top; 
		}
		else text = 0; break;

	case 'cont':
		
		if(is_item) //rewind
		{
			is_item = is_case = in_case = 0; 

			if(in_item)
			{
				while((*text)->cc!='item') text--;
			
				in_item = 0; 
			}
			else text++; goto top; 
		}
		else text = 0; break;

	case 'done': 
											
		if(is_case)
		{
			is_case = 0;

			if(in_case)
			{
				in_case = 0; text++; goto top; 			
			}
			else if(is_cond) goto cond; text = 0;
		}
		else text = 0; break;

	case 'item':
		
		is_item = 1; is_cond = 0; text++; goto top;

	case 'none': 
		
		if(is_case)
		{
			is_case = in_case = 0; text++; goto top;
		}
		else text = 0; break;

	case 'text': 

		if((*text)->irregular(procA))
		{
			if(is_case) in_case = 1;
			if(is_item) in_item = 1; is_cond = 0;
		}
		else if(matched=(*text)->match(procA,in,in_s))
		{					
			if(is_case) in_case = 1;
			if(is_item) in_item = 1;

			is_cond = 0;
		}
		else if(is_more)
		{
			while(*text&&(*text)->cc=='more') text++;

			is_more = 0; goto top;
		}
		else if(is_case&&!in_case||is_item&&!in_item)
		{
			while(*text) switch((*text)->cc)
			{
			case 'done': case 'cond': case 'cont': 
			case 'case': case 'none': goto top;

			default: text++;
			}
		}
		else if(is_cond) cond:
		{
			while((*text)->cc!='item') text--;

			in_item = is_case = in_case = is_cond = 0; goto top; 
		}
		else text = 0; break;

	case 'trap': text++; goto top; //meaningless

	default: return false;		
	}	  

	return matched;
}

int SOM::Menu::Text::pixels()const
{
	if(en==SOM::ITEM::Yes->en //-: centered
	 ||en==SOM::ITEM::No->en) return -1000; 
	//relies on string-pooling
	assert(SOM::ITEM::Yes->en==SOM::CAST::Yes->en);

	//looking for * or %s
	if(sz>2||op!=1) return 0; 

	int s = 0; //eg. %16s
	if(this==&SOM::EQUIP::Name)
	{
		s = 20;	//%-20.20s	
	}
	if(this==SOM::STORE::Menu.text)
	{
		s = 16; //%-16.16s
	}
	if(this==&SOM::OPTION::PadID
	 ||this==&SOM::CONFIG::PadID)
	{
		s = 42;	//%-42.42s
	}
	if(!s) return 0; 
	float ff = 1.1f; //fudge factor
	//2018: som_game_4212D0 MENUS ARE MORE OR LESS SQUARE
	//float scale = //SOM::fov[0]/640;
	//SOM::fov[0]>SOM::fov[1]?SOM::fov[1]/480:SOM::fov[0]/640;
	float scale = //SOM::fov[0]/640;
	SOM::width>SOM::height?SOM::height/480.0f:SOM::width/640.0f;
	//11: original half-width size of SOM's fixed width font		
	return (1.0f+(scale-1)*ff)*s*11;	
}

///////////////////////////////
//                           //
// formerly of som.terms.cpp //
//                           //
///////////////////////////////

static SOM::Term som_terms[] = 
{		
#define TERM(nn,id,en) {0/*#nn*/,id,en,sizeof(id)-1,0},
#include "som.terms.inl"
#undef TERM

	{0,0,0,0,0} //som_terms_unrecognized
};
static const int som_terms_total = 
sizeof(som_terms)/sizeof(SOM::Term)-1;
static SOM::Term &som_terms_unrecognized = som_terms[som_terms_total];
static SOM::Term *som_terms_lookup[som_terms_total] = {0}; 
static void som_terms_quicksort(SOM::Term**,int,int);
static void som_terms_sort_lookup()
{
	for(int i=0;i<som_terms_total;i++)
	som_terms_lookup[i] = &som_terms[i];
	som_terms_quicksort(som_terms_lookup,0,som_terms_total-1);
}

static void som_terms_quicksort(SOM::Term **in, int lo, int hi)
{
	if(lo>=hi) return; SOM::Term *swap = 0; 
    	
	int r = rand(); r = lo<hi?lo+r%(hi-lo):hi+r%(lo-hi);

	swap = in[lo]; in[lo] = in[r]; in[r] = swap;

	int pivot = lo; SOM::Term **p = &in[pivot];
	
	for(int unknown=pivot+1;unknown<=hi;unknown++) //...

	if(memcmp(in[unknown]->id,(*p)->id,min(in[unknown]->sz,(*p)->sz)+1)<0)
	{   		
		pivot++; swap = in[unknown]; in[unknown] = in[pivot]; in[pivot] = swap; 
	}

	swap = in[lo]; in[lo] = in[pivot]; in[pivot] = swap; 

	if(pivot) som_terms_quicksort(in,lo,pivot-1); 
	
	if(pivot<=som_terms_total) som_terms_quicksort(in,pivot+1,hi);
}

const SOM::Term &SOM::getTermById(const char *in, int in_s) //0
{
	if(!in) in = ""; if(!in_s) in_s = strlen(in); 
	
	if(!in_s) return som_terms_unrecognized; 	

	if(!*som_terms_lookup) som_terms_sort_lookup();
	
	SOM::Term *out = 0; 
	int cmp, first = 0, last = som_terms_total-1, stop = -1;
	int x;
	for(x=(last+first)/2;x!=stop;x=(last+first)/2)
	{
		out = som_terms_lookup[x];

		cmp = memcmp(out->id,in,min(out->sz,in_s)+1); 
		
		if(!cmp) return *out; else //...

		if(cmp>0) stop = last = x; else stop = first = x;
	}	
	if(last==som_terms_total-1&&x==last-1) //round-off error
	{
		out = som_terms_lookup[last];

		if(!memcmp(out->id,in,min(out->sz,in_s)+1)) return *out;
	}		
	return som_terms_unrecognized;
}