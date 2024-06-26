
#include "Ex.h"
EX_TRANSLATION_UNIT //(C)

#include <vector>
#include <algorithm>

#include "Ex.output.h" 

#define EX_INI_CPP(...) __VA_ARGS__	
#include "SomEx.ini.h"

#include "../lib/swordofmoonlight.h"

#include "SomEx.h" //SOMEX_VNUMBER

namespace Ex_ini_cpp 
{
	typedef const wchar_t *portion;
	typedef EX::INI::Section section;
	typedef section::private_data pdata;
	template<class T> class kindof_pdata 
	{ 		
		T* &p; public: 
		inline T *operator->(){ return p; }
		kindof_pdata(pdata*const &q):p((T*&)q)
		{
			if(!p) p = new T;
		}
	};
}

typedef const wchar_t *const *Ex_ini_e;
static Ex_ini_e Ex_ini(int i, wchar_t *e, int &cont)
{
	namespace ini = SWORDOFMOONLIGHT::ini; 
	static std::vector<ini::sections_t> out;
	static bool one_off = false; if(!one_off++) //???
	{
		for(size_t j=0;*EX::ini(j);j++)
		{
			extern void SomEx_environ(const wchar_t*kv[2],void*);
			ini::sections_t t; ini::open(t,EX::ini(j),1,SomEx_environ);
			out.push_back(t);
		}
	}
	for(size_t k=cont,n=ini::count(out[i]);k<n;k++)
	{
		const ini::text_t *p = ini::section(out[i],k);

		if(toupper(p[0][1])==toupper(*e)) //optimization
		{
			size_t len = wcslen(p[0]+1); if(p[0][len]==']') len--;

			cont = k+1;	if(!wcsnicmp(p[0]+1,e,len)) return p+1;
		}
	}
	cont = 0; return 0;
}
template <class T, typename mF>	//mF is private
static void Ex_ini_foreach(Ex_ini_e e, T t, mF mf) 
{
	const wchar_t *p,*q,*d; if(e) for(int i=0;p=e[i];i++) 
	{
		for(q=p;*q&&*q!='=';q++); d = q; if(*q) q++;	
		
		//2020: basic preprocessing for multiline values
		//(I couldn't open a URL with a newline on front)
		//NOTE: the newline values are \r\n, might convert
		//and/or right trim
		while(isspace(*q)) q++; 

		//2021: [Number] is passing these to Ex.number.cpp
		//for some reason all spaces are stripped out...
		//probably should fix lib/swordofmoonlight.h/cpp
		int todolist[SOMEX_VNUMBER<=0x1020602UL];
		if(*p=='#') continue; 

		//NEW: '.' reinitializes p+1
		if(*p=='.') (t->*mf)(p+1,d,0); else (t->*mf)(p,d,q);
	}
}
//REMOVE ME?
#define EX_INI_INITIALIZE_(Name,number) \
static bool initialized = false; if(!initialized++)\
{\
	wchar_t section[8] = L###Name L###number;\
	for(int i=0;*EX::ini(i);i++) for(int j=1;j<=number;j++)\
	{\
		section[6] = j>1?'0'+j:'\0';\
		for(int k=1,cont=0;k<=Section::passes;k++)\
		do EX::INI::Name(number)[k]+=Ex_ini(i,section,cont);\
		while(cont);\
	}\
	if(!EX::INI::Name(number)) _section->defaults(); /*hack*/\
} 
#define EX_INI_MENTION_(xxx,q,ctor_t) \
ctor_t = Ex_ini_xlat_##xxx##_val(q,ctor_t.enum_memset);

static void Ex_ini_apply_to_keymap(unsigned char,unsigned char);

template<class T>
static void Ex_ini_xlat_string_val(T &t, const wchar_t *in, int most=USHRT_MAX)
{
	typename T::value_type *out = t;

	for(int i=0;i<T::string_length&&*in;i++) if(unsigned(*in)>(unsigned)most)
	{
		*out = '\0'; return; //strictly converting Unicode to ASCII for now
	}
	else *out++ = *in++; *out = '\0';
}

template<typename T, size_t N>
static void Ex_ini_xlat_int_vals(T (&inout)[N], const wchar_t *in, const T &def=0)
{   	
	wchar_t *p = (wchar_t*)in, *q = p; for(int i=0;i<N;i++)
	{
		if(p) inout[i] = wcstol(p,&q,10); if(p==q) inout[i] = def; p = q;
	}
}
static int Ex_ini_xlat_int_val(const wchar_t *in, int def=0);
static int Ex_ini_xlat_hex_val(const wchar_t *in, int def=0);
static bool Ex_ini_xlat_bin_val(const wchar_t *in, bool def=0);
static DWORD Ex_ini_xlat_col_val(const wchar_t *in, DWORD def=0); 
static float Ex_ini_xlat_dec_val(const wchar_t *in, float def=0);
inline float Ex_ini_xlat_rad_val(const wchar_t *in, float def=0, float den=1)
{
	float dec = Ex_ini_xlat_dec_val(in,def); 	
	return dec>3.141592f/den?dec*3.141592f/180:dec; //convert from degrees
}
static int Ex_ini_xlat_4cc_val(const wchar_t *in, int def=0); //fourcc

static unsigned char Ex_ini_xlat_dik_val(const wchar_t *in, int def=0); //DirectInput keys

static EX_INI_ACT Ex_ini_xlat_act_val(const wchar_t *in, int def=0); //action to macro

template<typename T> //id retrieval from sorted id lookup tables
static int Ex_ini_do_binary_lookup(const T *in, const T *in_x, const T **id, int sz);

template<typename T> //symmetric null terminated lookup
static const T *Ex_ini_do_binary_lookup(const T *in, const T **id, int sz)
{
	int x;
	for(x=0;in[x]&&x<64;x++); if(x>=64) return 0; //paranoia
	
	return (const T*)Ex_ini_do_binary_lookup(in,in+x,id,sz);
}

template<typename T> //sorting of id lookup tables
static void Ex_ini_do_lookup_quicksort(const T **id, int sz);

static unsigned char Ex_ini_do_dik_lookup(const char *in);


#define SOMEX_INI_ACTION_SECTION

static int Ex_ini_action_unlocked = 0;

static const wchar_t **Ex_ini_action_lookup = 0;

static const unsigned char** //hack...
Ex_ini_action_macros = new const unsigned char*[64]; 
int Ex_ini_action_macros_storage = 64; //...
int Ex_ini_action_macros_defined = 0; //hack!!
//hack: supporting Action::operator+=
static void Ex_ini_action_undefine_all() //hack!!!
{
	for(int i=0;i<Ex_ini_action_macros_defined;i++)
	{
		delete[] Ex_ini_action_macros[i];

		Ex_ini_action_macros[i] = 0;
	}
	Ex_ini_action_macros_defined = 0;

	if(Ex_ini_action_lookup)
	for(int i=0;i<Ex_ini_action_macros_storage;i++)
	{
		delete[] Ex_ini_action_lookup[i];

		Ex_ini_action_lookup[i] = 0;
	}
}

static void Ex_ini_action_lock()
{
	Ex_ini_action_unlocked = 0;
}

static unsigned short Ex_ini_xlat_action_section_key(const wchar_t *in, const wchar_t *in_x)
{
	int out = Ex_ini_do_binary_lookup(in,in_x,Ex_ini_action_lookup,Ex_ini_action_unlocked);

	if(out<0xFFFF) return (unsigned short)out; else //...
	
	return EX_INI_BAD_MACRO;
}

static unsigned char Ex_ini_action_macro_buffer[64+6]; //debugging: static array

//Ex_ini_action_derive_macro: 
//generates a macro in Ex_ini_action_macro_buffer
//macro length stored in Ex_ini_action_macro_buffer[0]
//if possible, or -1 of return char pointer
static const unsigned char *Ex_ini_action_derive_macro(const wchar_t *in) //preview macro
{		
	//Notes: from what I'm able to gather, the present macro memory layout
	//is double null terminated, with each per frame sequence beginning with
	//a repeat count byte, then each byte in sequence, single null terminated.

	static unsigned char undefined; 

	unsigned char &len = *Ex_ini_action_macro_buffer; len = 1;

	int mul = 0; wchar_t *end = 0; //because wcstol() "endptr" is stupid
	
	int beg = len, op = '!'; //new sequence operator

	Ex_ini_action_macro_buffer[beg] = '\0'; //to hold repeat count later

	for(const wchar_t *p=in,*q,*d;*p&&len<64;p=q) //debug: 64 hard coded for now
	{			
		for(q=p;*q&&*q!=' '&&*q!='\t'&&*q!='+'&&*q!='*';q++); d = q;
		
		op = *q; if(*q) q++; while(*q&&*q==' '||*q=='\t') q++; 
			    
		switch(op==' '||op=='\t'?*q:op)
		{
		case '+': op = '+'; break; //combinatorial
		
		case '*': op = '*'; //repeat multiplier

			mul = wcstol(q,&end,10); q = end;

			Ex_ini_action_macro_buffer[beg] = mul;
			
			assert(mul>1); //unsupported

			while(*q&&*q==' '||*q=='\t') q++; 

			if(*q!='+'&&*q!='*') 
			goto fin;
			goto err; //syntax

		default: op = '!'; //no op
		}

top:	//try to match to two digit hexidecimal byte value

		if((*p>='0'&&*p<='9')||(*p>='a'&&*p<='f'))
		{
			if(p[1]&&d-p==2) if((p[1]>='0'&&p[1]<='9')||(p[1]>='a'&&p[1]<='f'))
			{					
				Ex_ini_action_macro_buffer[++len] = wcstol(p,0,16);
				
				goto fin; //finish
			}
			else goto dik; //try to match to a direct input key constant				
		}	   

dik:   //direct input constants

		static char dik_lookup_buffer[32];
		
		int i; for(i=0;p[i];i++)
		{
			if(p[i]>='0'&&p[i]<='9'||p[i]>='A'&&p[i]<='Z'||p[i]=='_')
			{
				if(i<32) dik_lookup_buffer[i] = p[i]; 
				else if(i>256) goto err;
			}
			else if(p[i]!=op&&!isspace(p[i])) //goto act //2018
			{
				goto act; //try to match to a defined action (Action segment)
			}
			else break;
		}	  
			
		dik_lookup_buffer[i] = '\0'; //should be all caps/underscore
		
		int nonzero = Ex_ini_do_dik_lookup(dik_lookup_buffer); if(!nonzero) goto err;

		Ex_ini_action_macro_buffer[++len] = nonzero; goto fin; //finish	
			
act:	if(!Ex_ini_action_unlocked) goto err; //not allowed for now (and ever??)		

		unsigned short x = Ex_ini_xlat_action_section_key(p,q);

		if(x>255)
		{
			//if(beg!=len-1) goto err; //todo: comboing of macros is case sensitive

			assert(0); //unsupported
		}
		else Ex_ini_action_macro_buffer[++len] = x; goto fin; //single key macro

fin:	switch(op)
		{
		default: assert(0); case '+': break; //nothing necessary

		case '!': if(len!=beg) Ex_ini_action_macro_buffer[beg] = 1; //FALL THRU

		case '*': Ex_ini_action_macro_buffer[++len] = '\0'; 
			
			Ex_ini_action_macro_buffer[beg=++len] = '\0';
		}
	}
	if(len>64) //macro too long for now
	{
		assert(0); return 0; //unimplemented
	}

	return Ex_ini_action_macro_buffer+1;

err: len = 0; Ex_ini_action_macro_buffer[1] = '\0'; 

	return Ex_ini_action_macro_buffer+1;
}

static EX_INI_MACRO Ex_ini_action_custom_macro(const unsigned char *in, int len)
{		
	if(Ex_ini_action_macros_defined>=64) //hardcoded for now
	{
		assert(0); return EX_INI_BAD_MACRO; //unimplemented
	}

	unsigned short out = 256+Ex_ini_action_macros_defined++; 

	unsigned char *dup = new unsigned char[len+2]; //!!!

	memcpy(dup,in,len); dup[len] = dup[len+1] = '\0'; //just to be safe

	Ex_ini_action_macros[out-256] = dup;

	return out;
}

static EX_INI_MACRO Ex_ini_action_define_macro(const wchar_t *key, 
											  const wchar_t *key_x, const wchar_t *val)
{	assert(!Ex_ini_action_unlocked);

	static int definitions = 0, available = 0;

	if(!Ex_ini_action_lookup)
	{			
		Ex_ini_action_lookup = new const wchar_t*[64]; 

		memset(Ex_ini_action_lookup,0x00,sizeof(wchar_t*)*64);

		definitions = 0; available = 32;
	}
	else if(definitions>=available-1) 
	{
		assert(0); //unimplemented

		return EX_INI_BAD_MACRO;
	}

	unsigned short out = EX_INI_BAD_MACRO;

	const unsigned char *m = Ex_ini_action_derive_macro(val);

	if(m[-1]!=4||m[0]!=1) //Ex_ini_action_derive_macro() stores len in -1
	{
		if(m[-1]==0) return EX_INI_BAD_MACRO; //bad syntax

		out = Ex_ini_action_custom_macro(m,m[-1]);		
	}
	else  out = m[1]; //single key macro
	
	Ex_ini_action_lookup[definitions*2] = (const wchar_t*)out;
	
	int len = key_x-key; wchar_t *def = new wchar_t[len+1];

	memcpy(def,key,2*len); def[len] = '\0';

	Ex_ini_action_lookup[definitions*2+1] = def;

	definitions++; return out;
}

static EX_INI_ACT Ex_ini_action_assign_macro(const wchar_t *val) //unnamed macro
{
	unsigned short out = EX_INI_BAD_MACRO;

	const unsigned char *m = Ex_ini_action_derive_macro(val);

	if(m[-1]!=4||m[0]!=1) //Ex_ini_action_derive_macro() stores len in -1
	{
		if(m[-1]==0) return EX_INI_NONCONTEXT(EX_INI_BAD_MACRO); //bad syntax

		return EX_INI_NONCONTEXT(Ex_ini_action_custom_macro(m,m[-1]));		
	}
	else out = m[1]; //single key macro

	return EX_INI_NONCONTEXT(out);
}

//note: Ex_ini_action_quicksort could be implicit in Ex_ini_action_unlock

static void Ex_ini_action_quicksort() 
{
	int array_size = 0; if(!Ex_ini_action_lookup) return;

	while(Ex_ini_action_lookup[++array_size*2]);

	Ex_ini_do_lookup_quicksort(Ex_ini_action_lookup,array_size);
}

static void Ex_ini_action_unlock()
{
	int array_size = 0; if(!Ex_ini_action_lookup) return;

	while(Ex_ini_action_lookup[++array_size*2]);

	Ex_ini_action_unlocked = array_size;
}

EX::INI::Action::Action(int section)
{		
	if(section>=1) 
	{
		static Section _1; //!

		_section = section==1?&_1:0;
		
		EX_INI_INITIALIZE_(Action,1) //...
	}
	else _section = 0;
}
void EX::INI::Action::operator+=(Ex_ini_e in)
{					
	if(!_section) return;
		
	if(_section->times_included++==0)
	{
		Ex_ini_action_undefine_all(); //hack
	}

	if(!in||!*in) return;

	Ex_ini_action_lock();

	Ex_ini_foreach(in,this,&Action::mention); //...

	Ex_ini_action_quicksort();

	Ex_ini_action_unlock();
}
void EX::INI::Action::mention(const wchar_t *p, const wchar_t *d, const wchar_t *q)
{	
	Section *nc = _section; nc->total_mentions++;

	int id = Ex_ini_action_define_macro(p,d,q); //hack
		
	if(id==EX_INI_BAD_MACRO) nc->total_failures++;
}

EX_INI_MACRO EX::INI::Action::Section::translate(const wchar_t *in, const wchar_t *in_x)const
{		
	if(!in_x) in_x = in+wcsnlen(in,MAX_PATH);

	EX_INI_MACRO out = Ex_ini_do_binary_lookup(in,in_x,Ex_ini_action_lookup,Ex_ini_action_unlocked); //todo: make thread safe	

	if(!out) //hack: used by default mouse configuration
	{
		EX_INI_ACT act = Ex_ini_action_assign_macro(in); //hack

		for(int i=1;i<EX::contexts-1;i++) assert(act[i]==act[0]);

		if(act) return act[0];
	}

	return out;
}

const unsigned char *EX::INI::Action::Section::xtranslate(const wchar_t *in, const wchar_t *in_x)const
{		
	unsigned short out; if(!in_x) in_x = in+wcslen(in); //todo: make secure

	out = Ex_ini_do_binary_lookup(in,in_x,Ex_ini_action_lookup,Ex_ini_action_unlocked); //todo: make thread safe

	return xtranslate(out);
}

const unsigned char *EX::INI::Action::Section::xtranslate(unsigned short x)const
{		
	static unsigned char undefined;

	if(x<256)
	{
		static unsigned char defaults[256][4] = {{0,0,0,0}};

		if(!defaults[0][0]) //initialize
		{
			memset(defaults,0x00,3*256); defaults[0][0] = 1; 
		}

		defaults[x][0] = 1; defaults[x][1] = x; //repeat x one_off
		defaults[x][2] = 0; defaults[x][3] = 0; //stop x & done

		return defaults[x]; //a simple one key sequence
	}
	else if(x-256>=Ex_ini_action_macros_defined) //undefined
	{
		undefined = 0; return &undefined;
	}
	else return Ex_ini_action_macros[x-256];
}

#undef SOMEX_INI_ACTION_SECTION 




#define SOMEX_INI_ADJUST_SECTION

#define LOOKUP(a,b,c) a, \

static enum
{
	#include "ini.lookup.inl"
};

#define LOOKUP(a,b,c) (const wchar_t*)a, L###c, \

static const wchar_t *Ex_ini_adjust_lookup[] = 
{
	#include "ini.lookup.inl"

	0, 0
};

#define LOOKUP(a,b,c) Ex_ini_adjust_lookup[a*2-2]=(const wchar_t*)a; Ex_ini_adjust_lookup[a*2-1]=L###c; \

static void Ex_ini_adjust_backup() 
{
	#include "ini.lookup.inl"
}

#undef LOOKUP

static int Ex_ini_xlat_adjust_section_key(const wchar_t *in, const wchar_t *in_x)
{		
	static int array_size = 0;

	if(!array_size) while(Ex_ini_adjust_lookup[++array_size*2]);

	return Ex_ini_do_binary_lookup(in,in_x,Ex_ini_adjust_lookup+2,array_size-1);
}

EX::INI::Adjust::Adjust(int section)
{		
	if(section>=1) 
	{
		static Section _1; //!

		_section = section==1?&_1:0;
		
		EX_INI_INITIALIZE_(Adjust,1) //...
	}
	else _section = 0;
}
void EX::INI::Adjust::operator+=(Ex_ini_e in)
{
	if(!_section) return;

	_section->times_included++;

	Ex_ini_foreach(in,this,&Adjust::mention); //...
}
void EX::INI::Adjust::mention(const wchar_t *p, const wchar_t *d, const wchar_t *q)
{
	Section *nc = _section; nc->total_mentions++;

	switch(Ex_ini_xlat_adjust_section_key(p,d))
	{
	case ADJUST_INITIALIZATION_FAILURE: nc->total_failures++; break;

	case ABYSS_DEPTH: nc->abyss_depth = q; break;	

	case FOV_FRUSTUM_MULTIPLIER: nc->fov_frustum_multiplier = q; break;
//	case FOV_AMBIENT_SATURATION: nc->fov_ambient_saturation = q; break;
	case FOV_FOGLINE_SATURATION: nc->fov_fogline_saturation = q; break;
	case FOV_FOGLINE_ADJUSTMENT: if(!(q=nc->fov_fogline_adjustment<<=q)) break;
	case FOV_FOGLINE_ADJUSTMENT2: nc->fov_fogline_adjustment2 = q; break;
	case FOV_SKYLINE_ADJUSTMENT: if(!(q=nc->fov_skyline_adjustment<<=q)) break;
	case FOV_SKYLINE_ADJUSTMENT2: nc->fov_skyline_adjustment2 = q; break;
	case FOV_SKY_AND_FOG_MINIMA: if(!(q=nc->fov_sky_and_fog_minima<<=q)) break;
	case FOV_SKY_AND_FOG_MINIMA2: nc->fov_sky_and_fog_minima2 = q; break;
	case FOV_SKY_AND_FOG_POWERS: if(!(q=nc->fov_sky_and_fog_powers<<=q)) break;
	case FOV_SKY_AND_FOG_POWERS2: if(!(q=nc->fov_sky_and_fog_powers2<<=q)) break;
	case FOV_SKY_AND_FOG_POWERS3: if(!(q=nc->fov_sky_and_fog_powers3<<=q)) break;
	case FOV_SKY_AND_FOG_POWERS4: nc->fov_sky_and_fog_powers4 = q; break;

	case NPC_SHADOWS_MULTIPLIER: nc->npc_shadows_multiplier = q; break;
	case NPC_SHADOWS_MODULATION: nc->npc_shadows_modulation = q; break;
	case NPC_SHADOWS_SATURATION: nc->npc_shadows_saturation = q; break;

	case NPC_XZ_PLANE_DISTANCE: nc->npc_xz_plane_distance = q; break;
		
	case NPC_HITBOX_CLEARANCE: nc->npc_hitbox_clearance = q; break;

	case NPC_FENCE: nc->npc_fence = q; break;
	
	case RED_FLASH_TINT: EX_INI_MENTION_(col,q,nc->red_flash_tint); break;
	case BLUE_FLASH_TINT: EX_INI_MENTION_(col,q,nc->blue_flash_tint); break;
	case GREEN_FLASH_TINT: EX_INI_MENTION_(col,q,nc->green_flash_tint); break;
	case BLACKENED_TINT: EX_INI_MENTION_(col,q,nc->blackened_tint); break;
	case BLACKENED_TINT2: EX_INI_MENTION_(col,q,nc->blackened_tint2); break;
	case LETTERING_TINT: EX_INI_MENTION_(col,q,nc->lettering_tint); break;
	case LETTERING_CONTRAST: EX_INI_MENTION_(col,q,nc->lettering_contrast); break;
		
	case START_TINT: EX_INI_MENTION_(col,q,nc->start_tint); break;

	case START_BLACKOUT_FACTOR:	nc->start_blackout_factor = q; break;

	case PANELING_FRAME_TRANSLUCENCY: 		
		EX_INI_MENTION_(dec,q,nc->paneling_frame_translucency); break;
	case HUD_LABEL_TRANSLUCENCY:
		EX_INI_MENTION_(dec,q,nc->hud_label_translucency); break;

	case PANELING_OPACITY_ADJUSTMENT: //NEW
	case PANELLING_OPACITY_ADJUSTMENT: nc->paneling_opacity_adjustment = q; break;	
	case HIGHLIGHT_OPACITY_ADJUSTMENT: nc->highlight_opacity_adjustment = q; break;			
	case HUD_GAUGE_OPACITY_ADJUSTMENT: nc->hud_gauge_opacity_adjustment = q; break;
	case HUD_GAUGE_DEPLETED_HALF_TINT: 
		
		EX_INI_MENTION_(col,q,nc->hud_gauge_depleted_half_tint); break;	

	case HUD_GAUGE_BORDER: EX_INI_MENTION_(hex,q,nc->hud_gauge_border); break;

	case CHARACTER_IDENTIFIER: nc->character_identifier = q; break;
	case SKY_MODEL_IDENTIFIER: nc->sky_model_identifier = q; break;
	}
}

#undef SOMEX_INI_ADJUST_SECTION




#define SOMEX_INI_ANALOG_SECTION

#define LOOKUP(a,b,c) a, \

static enum
{
	#include "ini.lookup.inl"
};

#define LOOKUP(a,b,c) (const wchar_t*)a, L###c, \

static const wchar_t *Ex_ini_analog_lookup[] = 
{
	#include "ini.lookup.inl"

	0, 0
};

#define LOOKUP(a,b,c) Ex_ini_analog_lookup[a*2-2]=(const wchar_t*)a; Ex_ini_analog_lookup[a*2-1]=L###c; \

static void Ex_ini_analog_backup() 
{
	#include "ini.lookup.inl"
}

#undef LOOKUP

static int Ex_ini_xlat_analog_section_key(const wchar_t *in, const wchar_t *in_x)
{		
	static int array_size = 0;

	if(!array_size) while(Ex_ini_analog_lookup[++array_size*2]);

	return Ex_ini_do_binary_lookup(in,in_x,Ex_ini_analog_lookup+2,array_size-1);
}

EX::INI::Analog::Analog(int section)
{		
	if(section>=1) 
	{
		static Section _1; //!

		_section = section==1?&_1:0;
		
		EX_INI_INITIALIZE_(Analog,1) //...
	}
	else _section = 0;
}
void EX::INI::Analog::operator+=(Ex_ini_e in)
{
	if(!_section) return;

	_section->times_included++;

	Ex_ini_foreach(in,this,&Analog::mention); //...
}
void EX::INI::Analog::mention(const wchar_t *p, const wchar_t *d, const wchar_t *q)
{	
	Section *nc = _section; nc->total_mentions++;

	switch(Ex_ini_xlat_analog_section_key(p,d))
	{
	case ANALOG_INITIALIZATION_FAILURE: nc->total_failures++; break;

	case ERROR_IMPEDANCE:  if(!(q=nc->error_impedance<<=q)) break;
	case ERROR_IMPEDANCE2: nc->error_impedance2 = q; break;
	case ERROR_ALLOWANCE:  if(!(q=nc->error_allowance<<=q)) break;
	case ERROR_ALLOWANCE2: nc->error_allowance2 = q; break;
	case ERROR_TOLERANCE:  nc->error_tolerance = q;  break;
	case ERROR_CLEARANCE:  nc->error_clearance = q;  break;
	case ERROR_PARACHUTE:  nc->error_parachute = q;  break;
	}
}

#undef SOMEX_INI_ANALOG_SECTION




#define SOMEX_INI_AUTHOR_SECTION

#define LOOKUP(a,b,c) a, \

static enum
{
	#include "ini.lookup.inl"
};

#define LOOKUP(a,b,c) (const wchar_t*)a, L###c, \

static const wchar_t *Ex_ini_author_lookup[] = 
{
	#include "ini.lookup.inl"

	0, 0
};

#define LOOKUP(a,b,c) Ex_ini_author_lookup[a*2-2]=(const wchar_t*)a; Ex_ini_author_lookup[a*2-1]=L###c; \

static void Ex_ini_author_backup() 
{
	#include "ini.lookup.inl"
}

#undef LOOKUP

static int Ex_ini_xlat_author_section_key(const wchar_t *in, const wchar_t *in_x)
{		
	static int array_size = 0;

	if(!array_size) while(Ex_ini_author_lookup[++array_size*2]);

	return Ex_ini_do_binary_lookup(in,in_x,Ex_ini_author_lookup+2,array_size-1);
}

EX::INI::Author::Author(int section)
{		
	if(section>=1) 
	{
		static Section _1; //!

		_section = section==1?&_1:0;
		
		EX_INI_INITIALIZE_(Author,1) //...
	}
	else _section = 0;
}
void EX::INI::Author::operator+=(Ex_ini_e in)
{
	if(!_section) return;

	//Reminder: works identically to Boxart
		
	static wchar_t *information = 0; //hack

	if(_section->times_included++==0)
	{
		delete[] information; information = 0;
	}

	if(!in||!*in) return;

	Ex_ini_foreach(in,this,&Author::mention); //...
}
void EX::INI::Author::mention(const wchar_t *p, const wchar_t *d, const wchar_t *q)
{			
	Section *nc = _section; nc->total_mentions++;

	switch(Ex_ini_xlat_author_section_key(p,d))
	{
	case AUTHOR_INITIALIZATION_FAILURE: nc->total_failures++; break;

	case INTERNATIONAL_PRODUCTION_TITLE:

		wcscpy_s(nc->international_production_title,MAX_PATH,q); break; 

	case INTERNATIONAL_TRANSLATION_TITLE:

		wcscpy_s(nc->international_translation_title,MAX_PATH,q); break; 

	case THE_AUTHORITY_TO_CONTACT: nc->the_authority_to_contact = q; break;		
	case EMAIL_ADDRESS_OF_CONTACT: nc->email_address_of_contact = q; break;		
	case STREET_ADDRESS_OF_CONTACT: nc->street_address_of_contact = q; break;		
	case PHONE_NUMBER_OF_CONTACT: nc->phone_number_of_contact = q; break;		
	case HOURS_IN_WHICH_TO_PHONE: nc->hours_in_which_to_phone = q; break;				
	case HOMEPAGE: //2022
	case ONLINE_WEBSITE_TO_VISIT: nc->online_website_to_visit = q; break;		
	}	   								
}

#undef SOMEX_INI_AUTHOR_SECTION




#define SOMEX_INI_BITMAP_SECTION

#define LOOKUP(a,b,c) a, \

static enum
{
	#include "ini.lookup.inl"
};

#define LOOKUP(a,b,c) (const wchar_t*)a, L###c, \

static const wchar_t *Ex_ini_bitmap_lookup[] = 
{
	#include "ini.lookup.inl"

	0, 0
};

#define LOOKUP(a,b,c) Ex_ini_bitmap_lookup[a*2-2]=(const wchar_t*)a; Ex_ini_bitmap_lookup[a*2-1]=L###c; \

static void Ex_ini_bitmap_backup() 
{
	#include "ini.lookup.inl"
}

#undef LOOKUP

static int Ex_ini_xlat_bitmap_section_key(const wchar_t *in, const wchar_t *in_x)
{		
	static int array_size = 0;

	if(!array_size) while(Ex_ini_bitmap_lookup[++array_size*2]);

	return Ex_ini_do_binary_lookup(in,in_x,Ex_ini_bitmap_lookup+2,array_size-1);
}

EX::INI::Bitmap::Bitmap(int section)
{		
	if(section>=1) 
	{
		static Section _1; //!

		_section = section==1?&_1:0;
		
		EX_INI_INITIALIZE_(Bitmap,1) //...

		std::sort(_1.bitmaps.begin(),_1.bitmaps.end());
	}
	else _section = 0;
}
void EX::INI::Bitmap::operator+=(Ex_ini_e in)
{
	if(!_section) return;

	_section->times_included++;

	Ex_ini_foreach(in,this,&Bitmap::mention); //...
}
void EX::INI::Bitmap::mention(const wchar_t *p, const wchar_t *d, const wchar_t *q)
{	
	Section *nc = _section; nc->total_mentions++;
		
	if(isdigit(*p))
	{
		EX::INI::bitmap b;
		
		wchar_t *e; b.index = wcstol(p,&e,10);

		if(*e=='_'&&e>p&&e<d)
		{
			b.file_name = ++e; 
			while(!isspace(*e)&&e<d)
			e++;
			b.file_name_s = e-b.file_name;
			while(isspace(*q)) q++;
			b.event_name = q;
			nc->bitmaps.push_back(b);
			return;		
		}

		nc->total_failures++;
	}
	else switch(Ex_ini_xlat_bitmap_section_key(p,d))
	{
	case BITMAP_INITIALIZATION_FAILURE: nc->total_failures++; break;

	case BITMAP_AMBIENT: nc->bitmap_ambient = q; break;
	case BITMAP_AMBIENT_MASK: nc->bitmap_ambient_mask = q; break;
	case BITMAP_AMBIENT_SATURATION: nc->bitmap_ambient_saturation = q; break;
	}
}
	
#undef SOMEX_INI_BITMAP_SECTION




#define SOMEX_INI_BOXART_SECTION

#define LOOKUP(a,b,c) a, \

static enum
{
	#include "ini.lookup.inl"
};

#define LOOKUP(a,b,c) (const wchar_t*)a, L###c, \

static const wchar_t *Ex_ini_boxart_lookup[] = 
{
	#include "ini.lookup.inl"

	0, 0
};

#define LOOKUP(a,b,c) Ex_ini_boxart_lookup[a*2-2]=(const wchar_t*)a; Ex_ini_boxart_lookup[a*2-1]=L###c; \

static void Ex_ini_boxart_backup() 
{
	#include "ini.lookup.inl"
}

#undef LOOKUP

static int Ex_ini_xlat_boxart_section_key(const wchar_t *in, const wchar_t *in_x)
{		
	static int array_size = 0;

	if(!array_size) while(Ex_ini_boxart_lookup[++array_size*2]);

	return Ex_ini_do_binary_lookup(in,in_x,Ex_ini_boxart_lookup+2,array_size-1);
}

EX::INI::Boxart::Boxart(int section)
{		
	if(section>=1) 
	{
		static Section _1; //!

		_section = section==1?&_1:0;
		
		EX_INI_INITIALIZE_(Boxart,1) //...
	}
	else _section = 0;
}
void EX::INI::Boxart::operator+=(Ex_ini_e in)
{
	if(!_section) return;

	//Reminder: works identically to Author
		
	static wchar_t *information = 0; //hack

	if(_section->times_included++==0)
	{
		delete[] information; information = 0;
	}

	if(!in||!*in) return;

	Ex_ini_foreach(in,this,&Boxart::mention); //...
}
void EX::INI::Boxart::mention(const wchar_t *p, const wchar_t *d, const wchar_t *q)
{
	Section *nc = _section; nc->total_mentions++;

	switch(Ex_ini_xlat_boxart_section_key(p,d))
	{
	case AUTHOR_INITIALIZATION_FAILURE: nc->total_failures++; break;

#define GAME_(X,x) case GAME_##X: nc->game_##x = q; break;		

	GAME_(ARTIST,artist)
	GAME_(AUTEUR,auteur)
	GAME_(AUTHOR,author) //2020
	GAME_(BASED_UPON,based_upon)
	GAME_(CARETAKER,caretaker)
	GAME_(COMPANY,company)
	GAME_(CONCEPT,concept)
	GAME_(DEDICATION,dedication)
	GAME_(DESIGNER,designer)
	GAME_(DEVELOPER,developer) //2020
	GAME_(DIRECTOR,director)
	GAME_(DISCLAIMER,disclaimer)
	GAME_(DISTRIBUTOR,distributor)
	GAME_(EDITION,edition)
	GAME_(FEATURES,features)
	GAME_(FIRST_EDITION_DATE,first_edition_date)
	GAME_(FIRST_EDITION_YEAR,first_edition_year)
	GAME_(FORMAT,format)
	GAME_(IN_ASSOCIATION_WITH,in_association_with)		
	GAME_(LABEL,label)
	GAME_(LEGAL,legal)
	GAME_(LICENSE,license)
	GAME_(MASCOT,mascot)
	GAME_(MUSIC,music)
	GAME_(MUSING,musing)
	GAME_(ORIGINAL,original)
	GAME_(PARTNERS,partners)
	GAME_(PLANNER,planner)
	GAME_(PRESENTED_BY,presented_by)
	GAME_(PRESENTERS,presenters)
	GAME_(PRODUCER,producer)
	GAME_(PROGRAMMER,programmer)
	GAME_(PROJECT,project)
	GAME_(PUBLISHER,publisher)
	GAME_(PUBLISHING_DATE,publishing_date)
	GAME_(RATING,rating)
	GAME_(REGION,region)
	GAME_(SERIES,series)	
	GAME_(SOUNDSUITE,soundsuite)
	GAME_(SOUNDTRACK,soundtrack)	
	GAME_(STUDIO,studio)  
	GAME_(SUB_SERIES,sub_series)
	GAME_(SUBTEAM,subteam)
	GAME_(SUBTITLE,subtitle)		
	GAME_(SUGGESTED_RETAIL_PRICE,suggested_retail_price)
	GAME_(SUPER_TITLE,super_title)		
	GAME_(THANKS,thanks)
	GAME_(TITLE_WITH_SEQUEL,title_with_sequel)		
	GAME_(VERSION,version)
	GAME_(WRITER,writer)
	GAME_(YEAR_PUBLISHED,year_published)

#undef GAME	
	}
}

#undef SOMEX_INI_BOXART_SECTION





#define SOMEX_INI_BUGFIX_SECTION

#define LOOKUP(a,b,c) a, \

static enum
{
	#include "ini.lookup.inl"
};

#define LOOKUP(a,b,c) (const wchar_t*)a, L###c, \

static const wchar_t *Ex_ini_bugfix_lookup[] = 
{
	#include "ini.lookup.inl"

	0, 0
};

#define LOOKUP(a,b,c) Ex_ini_bugfix_lookup[a*2-2]=(const wchar_t*)a; Ex_ini_bugfix_lookup[a*2-1]=L###c; \

static void Ex_ini_bugfix_backup() 
{
	#include "ini.lookup.inl"
}

#undef LOOKUP

static int Ex_ini_xlat_bugfix_section_key(const wchar_t *in, const wchar_t *in_x)
{		
	static int array_size = 0;

	if(!array_size) while(Ex_ini_bugfix_lookup[++array_size*2]);

	return Ex_ini_do_binary_lookup(in,in_x,Ex_ini_bugfix_lookup+2,array_size-1);
}

EX::INI::Bugfix::Bugfix(int section)
{		
	if(section>=1) 
	{
		static Section _1; //!

		_section = section==1?&_1:0;
		
		EX_INI_INITIALIZE_(Bugfix,1) //...
	}
	else _section = 0;
}
void EX::INI::Bugfix::operator+=(Ex_ini_e in)
{
	if(!_section) return;

	_section->times_included++;

	Ex_ini_foreach(in,this,&Bugfix::mention); //...
}
void EX::INI::Bugfix::mention(const wchar_t *p, const wchar_t *d, const wchar_t *q)
{	
	Section *nc = _section; nc->total_mentions++;

	bool yes = Ex_ini_xlat_bin_val(q);

	switch(Ex_ini_xlat_bugfix_section_key(p,d))
	{
	case BUGFIX_INITIALIZATION_FAILURE: nc->total_failures++; break;

	case DO_FIX_ANY_VITAL: nc->do_fix_any_vital(yes); break;

	case DO_FIX_ANY_TRIVIAL: nc->do_fix_any_trivial(yes); break;

	case DO_FIX_ANY_NONTRIVIAL: nc->do_fix_any_nontrivial(yes); break;

	case DO_FIX_CONTROLLER_CONFIGURATION:

		nc->do_fix_controller_configuration = yes; break;

	case DO_FIX_SPELLING_OF_ENGLISH_WORDS:

		nc->do_fix_spelling_of_english_words = yes; break;

	case DO_FIX_OUT_OF_RANGE_CONFIG_VALUES:

		nc->do_fix_out_of_range_config_values = yes; break;				

	case DO_FIX_LIFETIME_OF_GDI_OBJECTS:

		nc->do_fix_lifetime_of_gdi_objects = yes; break;

	case DO_FIX_SLOWDOWN_ON_SAVE_DATA_SELECT:

		nc->do_fix_slowdown_on_save_data_select = yes; break;

	case DO_FIX_CLIPPING_ITEM_DISPLAY:

		nc->do_fix_clipping_item_display = yes; break;
			
	case DO_FIX_OVERSIZED_COMPASS_NEEDLE:

		nc->do_fix_oversized_compass_needle = yes; break;

	case DO_FIX_LIGHTING_DROPOUT:

		nc->do_fix_lighting_dropout = yes; break;

	case DO_FIX_ELAPSED_TIME:

		nc->do_fix_elapsed_time = yes; break;

	case DO_FIX_ASYNCHRONOUS_SOUND:

		nc->do_fix_asynchronous_sound = yes; break;
		
	case DO_FIX_AUTOMAP_GRAPHIC:

		nc->do_fix_automap_graphic = yes; break;

	case DO_FIX_FOV_IN_MEMORY:

		nc->do_fix_fov_in_memory = yes; break;

	case DO_FIX_WIDESCREEN_FONT_HEIGHT:
								 
		nc->do_fix_widescreen_font_height = yes; break;
		
	case DO_FIX_ASYNCHRONOUS_INPUT: 

		nc->do_fix_asynchronous_input = yes; break;

	case DO_FIX_BOXED_EVENTS:
								 
		nc->do_fix_boxed_events = yes; break;
	
	case DO_FIX_DAMAGE_CALCULUS:
								 
		nc->do_fix_damage_calculus = yes; break;

	case DO_FIX_CLIPPING_PLAYER_CHARACTER:

		//nc->do_fix_clipping_player_character = yes; //1.2.1.8
		break;

	case DO_FIX_ZBUFFER_ABUSE:

		nc->do_fix_zbuffer_abuse = yes; break;

	//case DO_FIX_CIRCLE_TRIP_ZONE_RADIUS:
	case DO_FIX_TRIP_ZONE_RANGE:

		nc->do_fix_trip_zone_range = yes; break;

	//// experimental ///////////////////////////////

//	case DO_FIX_FRUSTUM_IN_MEMORY: //break; //ignoring for now

//		nc->do_fix_frustum_in_memory = yes; break;

	case DO_FIX_ANIMATION_SAMPLE_RATE:

		nc->do_fix_animation_sample_rate = yes; break;

	case DO_FIX_COLORKEY:

		nc->do_fix_colorkey = yes; break; 

	//// temporary ///////////////////////////////////	 

	case DO_TRY_TO_FIX_EXIT_FROM_MENUS:

		nc->do_try_to_fix_exit_from_menus = yes; break; //UNUSED
	}
}

#undef SOMEX_INI_BUGFIX_SECTION




static bool Ex_ini_button_assign_action
(EX_INI_ACT buttons[], int num, const wchar_t *in)
{
	int bt = EX::INI::Button::Section::pseudo_button(num);

	if(bt<0||bt>=EX::INI::Button::Section::buttons_s) return false;

	EX_INI_ACT x = Ex_ini_action_assign_macro(in);

	for(int c=0;c<EX::contexts-1;c++) buttons[bt][c] = x[c];			

	return true;
}

EX::INI::Button::Button(int section)
{		
	if(section>=1) 
	{
		static Section _1; //!

		_section = section==1?&_1:0;
		
		EX_INI_INITIALIZE_(Button,1) //...
	}
	else _section = 0;
}
void EX::INI::Button::operator+=(Ex_ini_e in)
{
	if(!_section) return;

	if(_section->times_included++==0) //hacks
	{
		//Start and Select (DIK_PAUSE and DIK_CAPITAL)
		_section->buttons[9] = EX_INI_NONCONTEXT(EX_INI_MACRO(0xC5));	
		//1: DIK_ESCAPE (not using Caps Lock for now/maybe never again)
		//_section->buttons[10] = EX_INI_NONCONTEXT(EX_INI_MACRO(0x3A));
		_section->buttons[10] = EX_INI_NONCONTEXT(EX_INI_MACRO(0x01)); 
	}

	Ex_ini_foreach(in,this,&Button::mention); //...
}
void EX::INI::Button::mention(const wchar_t *p, const wchar_t *d, const wchar_t *q)
{	
	Section *nc = _section; nc->total_mentions++;
		
	if(*p>='0'&&*p<='9') 
	{			
		wchar_t *end; int num = wcstol(p,&end,10); 
		if(end==q-1&&Ex_ini_button_assign_action(nc->buttons,num,q)) return; 
	}			
	nc->total_failures++;
}

#undef SOMEX_INI_BUTTON_SECTION




#define SOMEX_INI_DAMAGE_SECTION

#define LOOKUP(a,b,c) a, \

static enum
{
	#include "ini.lookup.inl"
};

#define LOOKUP(a,b,c) (const wchar_t*)a, L###c, \

static const wchar_t *Ex_ini_damage_lookup[] = 
{
	#include "ini.lookup.inl"

	0, 0
};

#define LOOKUP(a,b,c) Ex_ini_damage_lookup[a*2-2]=(const wchar_t*)a; Ex_ini_damage_lookup[a*2-1]=L###c; \

static void Ex_ini_damage_backup() 
{
	#include "ini.lookup.inl"
}

#undef LOOKUP

static int Ex_ini_xlat_damage_section_key(const wchar_t *in, const wchar_t *in_x)
{		
	static int array_size = 0;

	if(!array_size) while(Ex_ini_damage_lookup[++array_size*2]);

	return Ex_ini_do_binary_lookup(in,in_x,Ex_ini_damage_lookup+2,array_size-1);
}
EX::INI::Damage::Damage(int section)
{		
	if(section>=1) 
	{
		static Section _1; //!

		_section = section==1?&_1:0;
		
		EX_INI_INITIALIZE_(Damage,1) //...
	}
	else _section = 0;
}
void EX::INI::Damage::operator+=(Ex_ini_e in)
{
	if(!_section) return;

	_section->times_included++;	

	Ex_ini_foreach(in,this,&Damage::mention); //...
}
void EX::INI::Damage::mention(const wchar_t *p, const wchar_t *d, const wchar_t *q)
{	
	Section *nc = _section; nc->total_mentions++;
			
	switch(Ex_ini_xlat_damage_section_key(p,d))
	{
	case DAMAGE_INITIALIZATION_FAILURE: nc->total_failures++; break;
			
	case DO_NOT_HARM_DEFENSELESS_CHARACTERS:

		nc->do_not_harm_defenseless_characters = Ex_ini_xlat_bin_val(q); break;

	case HIT_POINT_MODE: EX_INI_MENTION_(int,q,nc->hit_point_mode); break;
		
	case HIT_POINT_MODEL: /*unimplemented: it's complicated*/ break; 

	case HIT_POINT_QUANTIFIER: if(!(q=nc->hit_point_quantifier<<=q)) break;
	case HIT_POINT_QUANTIFIER2: nc->hit_point_quantifier2 = q; break;
	case HIT_OFFSET_QUANTIFIER: if(!(q=nc->hit_offset_quantifier<<=q)) break;
	case HIT_OFFSET_QUANTIFIER2: nc->hit_offset_quantifier2 = q; break;
	case HIT_OUTCOME_QUANTIFIER: nc->hit_outcome_quantifier = q; break;
	case HIT_HANDICAP_QUANTIFIER: nc->hit_handicap_quantifier = q; break;
	case HIT_PENALTY_QUANTIFIER: nc->hit_penalty_quantifier = q; break;	

	case CRITICAL_HIT_POINT_QUANTIFIER:
		
		nc->critical_hit_point_quantifier = q; break;
	}
}

#undef SOMEX_INI_DAMAGE_SECTION




#define SOMEX_INI_DETAIL_SECTION

#define LOOKUP(a,b,c) a, \

static enum
{
	#include "ini.lookup.inl"
};

#define LOOKUP(a,b,c) (const wchar_t*)a, L###c, \

static const wchar_t *Ex_ini_detail_lookup[] = 
{
	#include "ini.lookup.inl"

	0, 0
};

#define LOOKUP(a,b,c) Ex_ini_detail_lookup[a*2-2]=(const wchar_t*)a; Ex_ini_detail_lookup[a*2-1]=L###c; \

static void Ex_ini_detail_backup() 
{
	#include "ini.lookup.inl"
}

#undef LOOKUP

static int Ex_ini_xlat_detail_section_key(const wchar_t *in, const wchar_t *in_x)
{		
	static int array_size = 0;

	if(!array_size) while(Ex_ini_detail_lookup[++array_size*2]);

	return Ex_ini_do_binary_lookup(in,in_x,Ex_ini_detail_lookup+2,array_size-1);
}

static int Ex_ini_modes
(EX::INI::escape_analog_mode (&modes)[8][8], const wchar_t *q)
{
	//2018: making these line up, so axisid is the subscript
	int out = 0; memset(modes,0x00,sizeof(modes));
	
	for(int i=0;i<EX::INI::Detail::Section::escape_analog_modes_s;i++)
	{
		//modes[i][0].inplay = 0;
		for(int j=0;j<8;j++) modes[i][j].axisid = j;

		if(*q!='\0') for(int j=0;*q&&j<8;j++)
		{
			modes[i][j].inplay = 1;
			//modes[i][j].axisid = j;
			modes[i][j].invert = *q>='A'&&*q<='Z';

			switch(*q++)
			{
			case 'x': case 'X': modes[i][j].motion=0; break;
			case 'y': case 'Y':	modes[i][j].motion=1; break;
			case 'z': case 'Z': modes[i][j].motion=2; break;
			case 'u': case 'U': modes[i][j].motion=3; break;
			case 'v': case 'V': modes[i][j].motion=4; break;

			//2017: this was broken for knocking out the first axis
			case '-': modes[i][j].inplay = 0; /*k--;*/ break;

			default: modes[i][j].inplay = 0; goto next_mode; //2017
			}			

			if(modes[i][j].inplay) out = i+1;
		}

		//2017: The parsing of this seemed to be broken.
		next_mode: while(isspace(*q)||*q==',') q++; 		
	}		 
	return out;
}

EX::INI::Detail::Detail(int section)
{		
	if(section>=1) 
	{
		static Section _1; //!

		_section = section==1?&_1:0;
		
		EX_INI_INITIALIZE_(Detail,1) //...
	}
	else _section = 0;
}
void EX::INI::Detail::operator+=(Ex_ini_e in)
{
	if(!_section) return;

	if(_section->times_included++==0) //hacks
	{
	//REMOVE ME?
	_section->mouse_button_actions = &_section->mouse_left_button_action;
	
	//Sword of Moonlight's defaults
	_section->escape_analog_modes[0][0].inplay = 1;
	_section->escape_analog_modes[0][0].motion = 0;
	_section->escape_analog_modes[0][0].axisid = 0;
	_section->escape_analog_modes[0][1].inplay = 1;
	_section->escape_analog_modes[0][1].motion = 2;
	_section->escape_analog_modes[0][1].axisid = 1;
	_section->escape_analog_modesN = 1;
	}

	Ex_ini_foreach(in,this,&Detail::mention); //...
}
void EX::INI::Detail::mention(const wchar_t *p, const wchar_t *d, const wchar_t *q)
{	
	Section *nc = _section; nc->total_mentions++;
			
	switch(Ex_ini_xlat_detail_section_key(p,d))
	{
	case DETAIL_INITIALIZATION_FAILURE: nc->total_failures++; break;
					
	case LOG_INITIAL_TIMEOUT: nc->log_initial_timeout = q; break;

	case LOG_SUBSEQUENT_TIMEOUT: nc->log_subsequent_timeout = q; break;

	case ESCAPE_ANALOG_MODES: 
		
		nc->escape_analog_modesN = 
		Ex_ini_modes(nc->escape_analog_modes,q); break;

	case ESCAPE_ANALOG_GAITS: nc->escape_analog_gaits = q; break;
	case ESCAPE_ANALOG_SHAPE: nc->escape_analog_shape = q; break;

	case NUMLOCK_STATUS: EX_INI_MENTION_(bin,q,nc->numlock_status); break;

	case CURSOR_HOURGLASS_MS_TIMEOUT: nc->cursor_hourglass_ms_timeout = q; break;

	case WASD_AND_MOUSE_MODE: EX_INI_MENTION_(int,q,nc->wasd_and_mouse_mode); break;
		
	case MOUSE_LEFT_BUTTON_ACTION: nc->mouse_left_button_action = Ex_ini_xlat_act_val(q); break;	
	case MOUSE_RIGHT_BUTTON_ACTION: nc->mouse_right_button_action = Ex_ini_xlat_act_val(q);  break;	
	case MOUSE_MIDDLE_BUTTON_ACTION: nc->mouse_middle_button_action = Ex_ini_xlat_act_val(q);  break;

	case MOUSE_4TH_BUTTON_ACTION: nc->mouse_4th_button_action = Ex_ini_xlat_act_val(q);  break;
	case MOUSE_5TH_BUTTON_ACTION: nc->mouse_5th_button_action = Ex_ini_xlat_act_val(q);  break;
	case MOUSE_6TH_BUTTON_ACTION: nc->mouse_6th_button_action = Ex_ini_xlat_act_val(q);  break;
	case MOUSE_7TH_BUTTON_ACTION: nc->mouse_7th_button_action = Ex_ini_xlat_act_val(q);  break;
	case MOUSE_8TH_BUTTON_ACTION: nc->mouse_8th_button_action = Ex_ini_xlat_act_val(q);  break;
			
	case MOUSE_TILT_LEFT_ACTION: nc->mouse_tilt_left_action = Ex_ini_xlat_act_val(q);  break;
	case MOUSE_TILT_RIGHT_ACTION: nc->mouse_tilt_right_action = Ex_ini_xlat_act_val(q);  break;
	case MOUSE_MENU_BUTTON_ACTION: nc->mouse_menu_button_action = Ex_ini_xlat_act_val(q);  break;	

	case MOUSE_VERTICAL_MULTIPLIER:	nc->mouse_vertical_multiplier = q; break;
	case MOUSE_HORIZONTAL_MULTIPLIER: nc->mouse_horizontal_multiplier = q; break;

	case MOUSE_SATURATE_MULTIPLIER:	nc->mouse_saturate_multiplier = q; break;
	case MOUSE_DEADZONE_MULTIPLIER: nc->mouse_deadzone_multiplier = q; break;
	case MOUSE_DEADZONE_MS_TIMEOUT:	nc->mouse_deadzone_ms_timeout = q; break;		

	case MIPMAPS_PIXEL_ART_POWER_OF_TWO: 
		
		EX_INI_MENTION_(dec,q,nc->mipmaps_pixel_art_power_of_two); break;

	case MIPMAPS_SATURATE: EX_INI_MENTION_(dec,q,nc->mipmaps_saturate); break;

	case MIPMAPS_KAISER_SINC_LEVELS: EX_INI_MENTION_(int,q,nc->mipmaps_kaiser_sinc_levels); break;

	case LIGHTS_LAMPS_LIMIT: nc->lights_lamps_limit = q; break;

	case LIGHTS_CALIBRATION_FACTOR: assert(0);
		
		//nc->lights_calibration_factor = q; break;

	case LIGHTS_CONSTANT_ATTENUATION: nc->lights_constant_attenuation = q; break;

	case LIGHTS_LINEAR_ATTENUATION: nc->lights_linear_attenuation = q; break;

	case LIGHTS_QUADRATIC_ATTENUATION: nc->lights_quadratic_attenuation = q; break;

	case LIGHTS_EXPONENT_MULTIPLIER: nc->lights_exponent_multiplier = q; break;
	case LIGHTS_DISTANCE_MULTIPLIER: nc->lights_distance_multiplier = q; break;

	case LIGHTS_DIFFUSE_MULTIPLIER:	nc->lights_diffuse_multiplier = q; break;
	case LIGHTS_AMBIENT_MULTIPLIER:	nc->lights_ambient_multiplier = q; break;

	case AMBIENT_CONTRIBUTION: nc->ambient_contribution = q; break;

	case ALPHAFOG_SKYFLOOR_CONSTANT: nc->alphafog_skyfloor_constant = q; break;
	case ALPHAFOG_SKYFLOOD_CONSTANT: nc->alphafog_skyflood_constant = q; break;

	case ALPHAFOG_COLORKEY: nc->alphafog_colorkey = q; break;

	//REMOVE US?
	//case RED_MULTIPLIER: nc->red_multiplier = q; break;
	//case RED_ADJUSTMENT: nc->red_adjustment = q; break;
	case RED_SATURATION: nc->red_saturation = q; break;

	case ST_MARGIN_MULTIPLIER: nc->st_margin_multiplier = q; break;	 
	
	case ST_STATUS_MODE: nc->st_status_mode = q; break;

	case START_MODE: nc->start_mode = q; break;

	case G:	nc->g = q; break;	 

	case DASH_JOGGING_GAIT:	nc->dash_jogging_gait = q; break;
	case DASH_RUNNING_GAIT:	nc->dash_running_gait = q; break;
	case DASH_STEALTH_GAIT:	nc->dash_stealth_gait = q; break;	

	case U2_POWER: nc->u2_power = q; break;
	case U2_HARDNESS: nc->u2_hardness = q; break;	
	
	case GAMMA_Y: while(isspace(*q)) q++; nc->gamma_y = q; break;
	case GAMMA_Y_STAGE: EX_INI_MENTION_(int,q,nc->gamma_y_stage); break;
	case GAMMA_N: while(isspace(*q)) q++; nc->gamma_n = q; break;
	case GAMMA_NPC: nc->gamma_npc = q; break;

	case NWSE_SATURATION: nc->nwse_saturation = q; break;
	}
}

#undef SOMEX_INI_DETAIL_SECTION




#define SOMEX_INI_EDITOR_SECTION

#define LOOKUP(a,b,c) a, \

static enum
{
	#include "ini.lookup.inl"
};

#define LOOKUP(a,b,c) (const wchar_t*)a, L###c, \

static const wchar_t *Ex_ini_editor_lookup[] = 
{
	#include "ini.lookup.inl"

	0, 0
};

#define LOOKUP(a,b,c) Ex_ini_editor_lookup[a*2-2]=(const wchar_t*)a; Ex_ini_editor_lookup[a*2-1]=L###c; \

static void Ex_ini_editor_backup() 
{
	#include "ini.lookup.inl"
}

#undef LOOKUP

static int Ex_ini_xlat_editor_section_key(const wchar_t *in, const wchar_t *in_x)
{		
	static int array_size = 0;

	if(!array_size) while(Ex_ini_editor_lookup[++array_size*2]);

	return Ex_ini_do_binary_lookup(in,in_x,Ex_ini_editor_lookup+2,array_size-1);
}

namespace Ex_ini_cpp
{
	struct psort : pdata
	{
		//61: just an arbitrary figure
		typedef wchar_t wchar_t_61[61];

		struct sort_pair_61{ wchar_t_61 first, second; }; 
				
		std::vector<sort_pair_61> sort_value_pairs; wchar_t_61 assorted;

		std::wstring sort_first_appendix; //concatenation

		psort(){ assorted[0] = '\0'; } virtual ~psort(){};
				
		bool sort(portion p, portion d, portion q)
		{
			if(!*q) //NEW: concatenation
			{
				sort_first_appendix = p;
				return true;
			}
			else if(p==d) //NEW: separator
			{
				sort_pair_61 sep={L""};
				wcscpy_s(sep.second,q);
				sort_value_pairs.push_back(sep);
				return true;
			}	

			bool s = false, _ = d[-1]=='_';

			size_t exploded = 0;
			wchar_t explode[16][32];
			while(exploded<16&&d-p>int(_))
			{
				wchar_t *q = explode[exploded];
				while(p!=d&&*p!='_') *q++ = *p++; *q = '\0'; p++;
				if(*explode[exploded]) exploded++; 
			}
			size_t cat = sort_first_appendix.size();
			if(cat) for(size_t i=0;exploded<16;i++)
			{
				size_t j = sort_first_appendix.find('_',i);		  
				if(j==sort_first_appendix.npos) j = cat;
				if(i==j||j-i>=32) break;
				sort_first_appendix.copy(explode[exploded],j-i,i);
				explode[exploded++][j-i] = '\0'; i = j;
			}

			wchar_t *dest_61 = 0; enum{ dest_s=60 };
									
			if(exploded) 
			{
				sort_pair_61 swap; enum{ swap_s=60 };
									   
				qsort(explode,exploded,sizeof(explode[0]),_qsort);

				for(size_t i=0,j=0;i<exploded;i++)
				{
					if(i) swap.first[j++] = ' ';

					for(size_t k=0;j<swap_s&&explode[i][k];k++)
					{
						swap.first[j++] = explode[i][k];
					}			   
					if(j>=swap_s) return false; 

					swap.first[j] = '\0';
				}				

				for(size_t i=0;i<sort_value_pairs.size();i++)
				if(!wcscmp(sort_value_pairs[i].first,swap.first))
				{
					sort_value_pairs.erase(sort_value_pairs.begin()+i);

				app: if(!*q) return true; sort_value_pairs.push_back(swap);

					dest_61 = sort_value_pairs.back().second; break;
				}
				if(!dest_61) goto app; //append
			}
			else dest_61 = assorted; 

			for(size_t i=0;i<dest_s;i++) switch(*q)
			{
			case '%': //%s (not doing %% or %ls)

				if(q[1]!='s'||61-i<4)
				{
					return dest_61[i] = false; 
				}
				else wcscpy(dest_61+i,L"%ls");

				s = true; i+=2; q+=2; break;

			default: dest_61[i] = *q++; break;

			case '\0': dest_61[i] = '\0';

				if(!s) //add the %ls to the end
				{
					const wchar_t *ls = L" %ls";

					if(61-i<5) return dest_61[i] = false; 

					wcscpy(dest_61+i,ls+_); //!
				}
				return true;
			}			 
			return dest_61[60] = false; 
		}
		static int _qsort(const void *a, const void *b)
		{
			return wcscmp((wchar_t*)a,(wchar_t*)b);
		}
	};

	typedef kindof_pdata<psort> editor_pdata;
}

EX::INI::Editor::Editor(int section)
{		
	if(section>=1) 
	{
		static Section _1; //!

		_section = section==1?&_1:0;
		
		EX_INI_INITIALIZE_(Editor,1) //...
	}
	else _section = 0;
}
void EX::INI::Editor::operator+=(Ex_ini_e in)
{
	if(!_section) return;

	_section->times_included++;

	Ex_ini_foreach(in,this,&Editor::mention); //...
}
static bool Ex_ini_editor_sort
(Ex_ini_cpp::pdata* &pdata, const wchar_t *p, const wchar_t *d, const wchar_t *q)
{
	if(*p=='s'&&!wcsncmp(p,L"sort_",5)&&d-p-5>=0) p+=5; else
	if(*p=='a'&&!wcsncmp(p,L"assorted",8)&&d-p-8<2) p+=8; else return false;

	bool assorted = p[-1]=='d'; if(assorted&&p!=d&&*p!='_') return false;

	return Ex_ini_cpp::editor_pdata(pdata)->sort(p,d,q);
}
void EX::INI::Editor::mention(const wchar_t *p, const wchar_t *d, const wchar_t *q)
{	
	Section *nc = _section; nc->total_mentions++;	
				
	switch(Ex_ini_xlat_editor_section_key(p,d))
	{
	case EDITOR_INITIALIZATION_FAILURE:
		
		if(Ex_ini_editor_sort(nc->pdata,p,d,q)) break;
		
		nc->total_failures++; break;
	
	case DO_HIDE_DIRECT3D_HARDWARE: 
		
		nc->do_hide_direct3d_hardware = Ex_ini_xlat_bin_val(q); break;

	case DO_OVERWRITE_PROJECT: EX_INI_MENTION_(bin,q,nc->do_overwrite_project); break;
	case DO_OVERWRITE_RUNTIME: EX_INI_MENTION_(bin,q,nc->do_overwrite_runtime); break;		

	case DO_SUBDIVIDE_POLYGONS: EX_INI_MENTION_(bin,q,nc->do_subdivide_polygons); break;		

	case TILE_VIEW_SUBDIVISIONS: EX_INI_MENTION_(int,q,nc->tile_view_subdivisions); break;

	case TILE_ELEVATION_INCREMENT: EX_INI_MENTION_(dec,q,nc->tile_elevation_increment); break;

	case CLIP_VOLUME_MULTIPLIER: nc->clip_volume_multiplier = q; break;

	case CLIP_MODEL_PIXEL_VALUE: EX_INI_MENTION_(col,q,nc->clip_model_pixel_value); break;

	case DEFAULT_PIXEL_VALUE: EX_INI_MENTION_(col,q,nc->default_pixel_value); break;
	case DEFAULT_PIXEL_VALUE2: EX_INI_MENTION_(col,q,nc->default_pixel_value2); break;
	case DEFAULT_PIXEL_VALUE3: EX_INI_MENTION_(col,q,nc->default_pixel_value3); break;

	case TEXTURE_SUBSAMPLES: nc->texture_subsamples = q; break;

	case TIME_MACHINE_MODE: EX_INI_MENTION_(int,q,nc->time_machine_mode); break;
	case SPELLCHECKER_MODE: EX_INI_MENTION_(int,q,nc->spellchecker_mode); break;

	case BLACK_SATURATION: nc->black_saturation = q; break;
	case WHITE_SATURATION: nc->white_saturation = q; break;
						 		
	case DO_NOT_DITHER: EX_INI_MENTION_(bin,q,nc->do_not_dither); break;
	case DO_NOT_SMOOTH: EX_INI_MENTION_(bin,q,nc->do_not_smooth); break;
	case DO_NOT_STIPPLE: EX_INI_MENTION_(bin,q,nc->do_not_stipple); break;

	case HEIGHT_ADJUSTMENT: nc->height_adjustment = q; break;

	case MAP_ENEMIES_LIMIT: EX_INI_MENTION_(int,q,nc->map_enemies_limit); break;

	case DO_NOT_OPEN_WITH_MM3D: nc->do_not_open_with_mm3d = q; break;
	case DO_NOT_GENERATE_ICONS: nc->do_not_generate_icons = q; break;
	
	case MAP_ICON_BRIGHTNESS: EX_INI_MENTION_(dec,q,nc->map_icon_brightness); break;
	}
}

size_t EX::INI::Editor::Section::assorted_(const wchar_t**rv)const
{	
	if(rv) *rv = L"%ls"; if(!pdata) return 0;

	Ex_ini_cpp::editor_pdata p(pdata);

	if(rv&&*p->assorted) *rv = p->assorted; return p->sort_value_pairs.size();
}
bool EX::INI::Editor::Section::sort_(size_t i, const wchar_t **lv, const wchar_t **rv)const
{	
	if(lv) *lv = L""; if(rv) *rv = L"";	 
	if(!pdata){ if(i==0&&rv) *rv = L"%ls"; return false; } //!

	Ex_ini_cpp::editor_pdata p(pdata);

	if(i<p->sort_value_pairs.size())
	{
		if(lv) *lv = p->sort_value_pairs[i].first; 
		if(rv) *rv = p->sort_value_pairs[i].second; return true;
	}
	else if(i==p->sort_value_pairs.size()) //courtesy
	{
		if(rv) *rv = *p->assorted?p->assorted:L"%ls"; return false; //!
	}
	else return false; 
}

#undef SOMEX_INI_EDITOR_SECTION





#define SOMEX_INI_JOYPAD_SECTION

#define LOOKUP(a,b,c) a, \

static enum
{
	#include "ini.lookup.inl"
};

#define LOOKUP(a,b,c) (const wchar_t*)a, L###c, \

static const wchar_t *Ex_ini_joypad_lookup[] = 
{
	#include "ini.lookup.inl"

	0, 0
};

#define LOOKUP(a,b,c) Ex_ini_joypad_lookup[a*2-2]=(const wchar_t*)a; Ex_ini_joypad_lookup[a*2-1]=L###c; \

static void Ex_ini_joypad_backup() 
{
	#include "ini.lookup.inl"
}

#undef LOOKUP

static int Ex_ini_xlat_joypad_section_key(const wchar_t *in, const wchar_t *in_x)
{		
	static int array_size = 0;

	if(!array_size) while(Ex_ini_joypad_lookup[++array_size*2]);

	return Ex_ini_do_binary_lookup(in,in_x,Ex_ini_joypad_lookup+2,array_size-1);
}

static void Ex_ini_joypad_assign_action
	(EX_INI_CONTEXTUAL<int>* &buttons, int num, const wchar_t *in)
{
	if(num==0) return;

	int l = num<0?0:16, r = 16-l; num = abs(num);

	EX_INI_ACT x = Ex_ini_action_assign_macro(in);

	if(!buttons) //hacK??
	{
		buttons = new EX_INI_CONTEXTUAL<int>[num>32?num:32]; 

		memset(buttons,0x00,(num>32?num:32)*sizeof(EX_INI_CONTEXTUAL<int>));

		//todo: store capacity in buttons[-1] and num in buttons[0]
		for(int c=0;c<EX::contexts-1;c++) buttons[0][c]=num>32?num:32;
	}
	else if(buttons[0][0]<=num) //hack
	{
		assert(0); return; //unimplemented
	}	

	for(int c=0;c<EX::contexts-1;c++)
	buttons[num][c] = (buttons[num][c]&0xFFFF<<l)|(x[c]<<r);			
}

EX::INI::Joypad::Joypad(int section)
{		
	if(section>=1) 
	{
		static Section _1; //!

		_section = section==1?&_1:0;
		
		EX_INI_INITIALIZE_(Joypad,1) //...
	}
	else _section = 0;
}
void EX::INI::Joypad::operator+=(Ex_ini_e in)
{
	if(!_section) return;

	Ex_ini_foreach(in,this,&Joypad::mention); //...
}
void EX::INI::Joypad::mention(const wchar_t *p, const wchar_t *d, const wchar_t *q)
{	
	Section *nc = _section; nc->total_mentions++;	

	bool fail = false, neg = *p=='_'; 
	
	if(neg) if(++p==d) fail = true;

	if(*p>='0'&&*p<='9'&&!fail) //buttons
	{
		wchar_t *end; int num = wcstol(p,&end,10); 

		if(end!=d&&num) //todo: localization
		{
			//anything goes here; eg. st/nd/rd/th as in 1st etc
			while(*end&&*end!='_') end++; if(*end!='_') fail = true;

			if(!wcscmp(end,L"_button_reaction")) //longhand syntax
			{
				neg = true; //note: permitting _1st_button_reaction
			}
			else if(!wcscmp(end,L"_button_action")) fail = true;
		}
		else fail = true;

		if(!fail) Ex_ini_joypad_assign_action(nc->buttons,neg?-num:num,q);				
		if(!fail) return;
	}
	else if(neg) fail = true;

	if(fail)
	{
		nc->total_failures++;
	}
	else switch(Ex_ini_xlat_joypad_section_key(p,d))
	{
	case JOYPAD_INITIALIZATION_FAILURE: nc->total_failures++; break;

	case JOYPAD_TO_USE_FOR_PLAY: 

		wcscpy_s(nc->joypad_to_use_for_play,MAX_PATH,q); break;		
			
	case POV_HAT_TO_USE_FOR_PLAY: 

		nc->pov_hat_to_use_for_play = Ex_ini_xlat_int_val(q,1); break;		

	case DO_NOT_ASSOCIATE_POV_HAT_DIAGONALS:

		nc->do_not_associate_pov_hat_diagonals = Ex_ini_xlat_bin_val(q); break;

	case PSEUDO_X_AXIS:	 nc->pseudo_x_axis  = Ex_ini_xlat_int_val(q); break;
	case PSEUDO_Y_AXIS:	 nc->pseudo_y_axis  = Ex_ini_xlat_int_val(q); break;
	case PSEUDO_Z_AXIS:	 nc->pseudo_z_axis  = Ex_ini_xlat_int_val(q); break;
	case PSEUDO_X_AXIS2: nc->pseudo_x_axis2 = Ex_ini_xlat_int_val(q); break;
	case PSEUDO_Y_AXIS2: nc->pseudo_y_axis2 = Ex_ini_xlat_int_val(q); break;
	case PSEUDO_Z_AXIS2: nc->pseudo_z_axis2 = Ex_ini_xlat_int_val(q); break;

	case PSEUDO_POV_HAT_0:     nc->pseudo_pov_hat_0     = Ex_ini_xlat_int_val(q); break;			  
	case PSEUDO_POV_HAT_4500:  nc->pseudo_pov_hat_4500  = Ex_ini_xlat_int_val(q); break;
	case PSEUDO_POV_HAT_9000:  nc->pseudo_pov_hat_9000  = Ex_ini_xlat_int_val(q); break; 		
	case PSEUDO_POV_HAT_13500: nc->pseudo_pov_hat_13500 = Ex_ini_xlat_int_val(q); break;			  
	case PSEUDO_POV_HAT_18000: nc->pseudo_pov_hat_18000 = Ex_ini_xlat_int_val(q); break;
	case PSEUDO_POV_HAT_22500: nc->pseudo_pov_hat_22500 = Ex_ini_xlat_int_val(q); break; 		
	case PSEUDO_POV_HAT_27000: nc->pseudo_pov_hat_27000 = Ex_ini_xlat_int_val(q); break;
	case PSEUDO_POV_HAT_31500: nc->pseudo_pov_hat_31500 = Ex_ini_xlat_int_val(q); break;

	case AXIS_ANALOG_GAITS: nc->axis_analog_gaits = q; break;
	case AXIS2_ANALOG_GAITS: nc->axis2_analog_gaits = q; break;
	case SLIDER_ANALOG_GAITS: nc->slider_analog_gaits = q; break;
	case SLIDER2_ANALOG_GAITS: nc->slider2_analog_gaits = q; break;
	}
}

#undef SOMEX_INI_JOYPAD_SECTION




/*REMOVE ME?
#define SOMEX_INI_KEYGEN_SECTION

#define LOOKUP(a,b,c) a, \

static enum
{
	#include "ini.lookup.inl"
};

#define LOOKUP(a,b,c) (const wchar_t*)a, L###c, \

static const wchar_t *Ex_ini_keygen_lookup[] = 
{
	#include "ini.lookup.inl"

	0, 0
};

#define LOOKUP(a,b,c) Ex_ini_keygen_lookup[a*2-2]=(const wchar_t*)a; Ex_ini_keygen_lookup[a*2-1]=L###c; \

static void Ex_ini_keygen_backup() 
{
	#include "ini.lookup.inl"
}

#undef LOOKUP

static int Ex_ini_xlat_keygen_section_key(const wchar_t *in, const wchar_t *in_x)
{		
	static int array_size = 0;

	if(!array_size) while(Ex_ini_keygen_lookup[++array_size*2]);

	return Ex_ini_do_binary_lookup(in,in_x,Ex_ini_keygen_lookup+2,array_size-1);
}

EX::INI::Keygen::Keygen(int section)
{		
	if(section>=1) 
	{
		static Section _1; //!

		_section = section==1?&_1:0;
		
		EX_INI_INITIALIZE_(Keygen,1) //...
	}
	else _section = 0;
}
void EX::INI::Keygen::operator+=(Ex_ini_e in)
{
	if(!_section) return;

	_section->times_included++;

	Ex_ini_foreach(in,this,&Keygen::mention); //...
}
void EX::INI::Keygen::mention(const wchar_t *p, const wchar_t *d, const wchar_t *q)
{	
	Section *nc = _section; nc->total_mentions++;
				
	switch(Ex_ini_xlat_keygen_section_key(p,d))
	{
	case KEYGEN_INITIALIZATION_FAILURE: nc->total_failures++; break;
								
	case DO_SOMDB_KEYGEN_DEFAULTS:

		nc->do_somdb_keygen_defaults = Ex_ini_xlat_bin_val(q); break;

	case DO_ENABLE_KEYGEN_AUDITING:

		nc->do_enable_keygen_auditing = Ex_ini_xlat_bin_val(q); break;

	case DO_DISABLE_KEYGEN_AUDITING:

		nc->do_disable_keygen_auditing = Ex_ini_xlat_bin_val(q); break;

	case DO_ENABLE_KEYGEN_AUTOMATION:

		nc->do_enable_keygen_automation = Ex_ini_xlat_bin_val(q); break;

	case KEYGEN_AUTOMATIC_FILTER:

		wcscpy_s(nc->keygen_automatic_filter,MAX_PATH,q); break;

	case KEYGEN_AUDIT_FOLDER:

		wcscpy_s(nc->keygen_audit_folder,MAX_PATH,q); break;

	case KEYGEN_TOPLEVEL_SUBKEY_IN_REGISTRY:

		wcscpy_s(nc->keygen_toplevel_subkey_in_registry,MAX_PATH,q); break;

	case KEYGEN_IMAGE_FILE:

		wcscpy_s(nc->keygen_image_file,MAX_PATH,q); break;

	case KEYGEN_MODEL_FILE:

		wcscpy_s(nc->keygen_model_file,MAX_PATH,q); break;

	case DO_ENABLE_KEYGEN_VISUALIZATION:

		nc->do_enable_keygen_visualization = Ex_ini_xlat_bin_val(q); break;

	case DO_ENABLE_KEYGEN_INSTANTIATION:

		nc->do_enable_keygen_instantiation = Ex_ini_xlat_bin_val(q); break;
	}	
}

#undef SOMEX_INI_KEYGEN_SECTION*/





#define SOMEX_INI_KEYMAP_SECTION

static unsigned char Ex_ini_keymap_x[256] = {0,1,2,/*...*/};	
static unsigned char Ex_ini_keymap_xx[256] = {0,1,2,/*...*/};

static void Ex_ini_apply_to_keymap(unsigned char x, unsigned char y)
{
	assert(Ex_ini_keymap_x[x]==x||Ex_ini_keymap_x[x]==y);

	Ex_ini_keymap_x[x] = y;
}

static void Ex_ini_keymap_assign_action(int key, const wchar_t *val)
{
	int out = 0; if(key>255) return; //TODO: error messaging

	out = Ex_ini_xlat_dik_val(val,Ex_ini_keymap_x[(unsigned char)key]); 

	Ex_ini_keymap_x[(unsigned char)key] = out; return;
}

//REMOVE ME?
void EX::INI::Keymap::Section::translate
(const unsigned char *in, unsigned char *out, size_t sz)const
{	
	size_t i, n = min(sz,256); if(!in||!out||sz<1) return; 	

	for(i=0;i<n;i++) out[i] = in[Ex_ini_keymap_x[i]];
	for(;i<sz;i++) out[i] = in[i];
}
void EX::INI::Keymap::Section::xtranslate(unsigned char *inout, size_t sz)const
{
	size_t n = min(sz,256); if(!inout||sz<1) return;
	
	for(size_t i=0;i<sz;i++) inout[i] = Ex_ini_keymap_x[inout[i]];	
}
void EX::INI::Keymap::Section::xtranslate(unsigned short *inout, size_t sz)const
{
	size_t n = min(sz,256); if(!inout||sz<1) return;
	
	for(size_t i=0;i<sz;i++) if(inout[i]<256) inout[i] = Ex_ini_keymap_x[inout[i]];	
}
void EX::INI::Keymap::Section::xxtranslate(unsigned char *inout, size_t sz)const
{
	size_t n = min(sz,256); if(!inout||sz<1) return;

	if(Ex_ini_keymap_xx[0]) return; //assuming reverse keymap does not yet exist

	for(size_t i=0;i<sz;i++) inout[i] = Ex_ini_keymap_xx[inout[i]];	
}
void EX::INI::Keymap::Section::xxtranslate(unsigned short *inout, size_t sz)const
{
	size_t n = min(sz,256); if(!inout||sz<1) return;

	if(Ex_ini_keymap_xx[0]) return; //assuming reverse keymap does not yet exist
	
	for(size_t i=0;i<sz;i++) if(inout[i]<256) inout[i] = Ex_ini_keymap_xx[inout[i]];	
}

EX::INI::Keymap::Keymap(int section)
{		
	if(section>=1) 
	{
		static Section _1; //!

		_section = section==1?&_1:0;
		
		EX_INI_INITIALIZE_(Keymap,1) //...
	}
	else _section = 0;
}
void EX::INI::Keymap::operator+=(Ex_ini_e in)
{
	if(!_section) return;

	if(_section->times_included++==0) //hacks
	{
		for(int i=0;i<256;i++) Ex_ini_keymap_x[i] = i;
	}

	Ex_ini_foreach(in,this,&Keymap::mention); //....
	 		
	memset(Ex_ini_keymap_xx,0x00,256*sizeof(char));

	for(int i=0;i<256;i++) //build reverse keymap
	{
		Ex_ini_keymap_xx[Ex_ini_keymap_x[i]] = i;
	}
}
void EX::INI::Keymap::mention(const wchar_t *p, const wchar_t *d, const wchar_t *q)
{	
	Section *nc = _section; nc->total_mentions++;
		
	if(d==p+2)
	if((*p>='0'&&*p<='9')||(*p>='a'&&*p<='f'))
	{
		if(p[1]&&!p[2]) 
		if((p[1]>='0'&&p[1]<='9')||(p[1]>='a'&&p[1]<='f'))
		{
			return Ex_ini_keymap_assign_action(wcstol(p,0,16),q); 
		}		
	}	   

	nc->total_failures++; //!
	{
		static char dik_lookup_buffer[32];
		
		int i;
		for(i=0;p!=d;p++)
		{
			if(*p>='0'&&*p<='9'||*p>='A'&&*p<='Z'||*p=='_')
			{
				if(i<32) dik_lookup_buffer[i++] = *p; 
				
					else return; //too long to be a dx constant
			}
			else return; //not a valid key assignment
		}	  
			
		dik_lookup_buffer[i] = '\0'; //should be all caps/underscore
		
		unsigned char dik = Ex_ini_do_dik_lookup(dik_lookup_buffer); assert(dik);
						
		Ex_ini_keymap_assign_action(dik,q); 
	}
	nc->total_failures--;
}

#undef SOMEX_INI_KEYMAP_SECTION




#define SOMEX_INI_KEYPAD_SECTION

static unsigned short Ex_ini_keypad_x[256] = {0,1,2,/*...*/};

static void Ex_ini_apply_to_keypad(unsigned char x, unsigned char y)
{
	assert(Ex_ini_keypad_x[x]==x||Ex_ini_keypad_x[x]==y);

	Ex_ini_keypad_x[x] = y;
}

//REMOVE ME?
EX_INI_MACRO EX::INI::Keypad::Section::translate(unsigned short in)const
{
	if(!in||in>255) return in; return Ex_ini_keypad_x[in&0xFF];
}

void Ex_ini_keypad_assign_action(int key, const wchar_t *val)
{
	int out = 0; if(key>255) return;

	out = Ex_ini_xlat_act_val(val,Ex_ini_keypad_x[(unsigned char)key])[0]&0xFFFF;

	Ex_ini_keypad_x[(unsigned char)key] = out; return;
}

EX::INI::Keypad::Keypad(int section)
{		
	if(section>=1) 
	{
		static Section _1; //!

		_section = section==1?&_1:0;
		
		EX_INI_INITIALIZE_(Keypad,1) //...
	}
	else _section = 0;
}
void EX::INI::Keypad::operator+=(Ex_ini_e in)
{
	if(!_section) return;

	if(_section->times_included++==0) //hacks
	{
		for(int i=0;i<256;i++) 
		{
			Ex_ini_keypad_x[i] = i; _section->turbo_keys[i] = 0;
		}		  
		//REMOVE ME?
		#define TURBO(delay,key,dik) _section->turbo_keys[0x##key] = delay;
		#include "ini.keypad.inl"  
	}

	Ex_ini_foreach(in,this,&Keypad::mention); //...
}
void EX::INI::Keypad::mention(const wchar_t *p, const wchar_t *d, const wchar_t *q)
{	
	Section *nc = _section; nc->total_mentions++;
	
	if(d==p+2)
	if((*p>='0'&&*p<='9')||(*p>='a'&&*p<='f'))
	{
		if(p[1]&&!p[2]) 
		if((p[1]>='0'&&p[1]<='9')||(p[1]>='a'&&p[1]<='f'))
		{
			return Ex_ini_keypad_assign_action(wcstol(p,0,16),q); 			 
		}		
	}	   

	nc->total_failures++; //!
	{
		static char dik_lookup_buffer[32];
		
		int i;
		for(i=0;p!=d;p++)
		{
			if(*p>='0'&&*p<='9'||*p>='A'&&*p<='Z'||*p=='_')
			{
				if(i<32) dik_lookup_buffer[i++] = *p; 
				
					else return; //too long to be a dx constant
			}
			else return; //not a valid key assignment
		}	  
			
		dik_lookup_buffer[i] = '\0'; //should be all caps/underscore
		
		unsigned char dik = Ex_ini_do_dik_lookup(dik_lookup_buffer); assert(dik);
						
		Ex_ini_keypad_assign_action(dik,q); 
	}
	nc->total_failures--;
}

#undef SOMEX_INI_KEYPAD_SECTION





#define SOMEX_INI_LAUNCH_SECTION

#define LOOKUP(a,b,c) a, \

static enum
{
	#include "ini.lookup.inl"
};

#define LOOKUP(a,b,c) (const wchar_t*)a, L###c, \

static const wchar_t *Ex_ini_launch_lookup[] = 
{
	#include "ini.lookup.inl"

	0, 0
};

#define LOOKUP(a,b,c) Ex_ini_launch_lookup[a*2-2]=(const wchar_t*)a; Ex_ini_launch_lookup[a*2-1]=L###c; \

static void Ex_ini_launch_backup() 
{
	#include "ini.lookup.inl"
}

#undef LOOKUP

static int Ex_ini_xlat_launch_section_key(const wchar_t *in, const wchar_t *in_x)
{		
	static int array_size = 0;

	if(!array_size) while(Ex_ini_launch_lookup[++array_size*2]);

	return Ex_ini_do_binary_lookup(in,in_x,Ex_ini_launch_lookup+2,array_size-1);
}

EX::INI::Launch::Launch(int section)
{		
	if(section>=1) 
	{
		static Section _1; //!

		_section = section==1?&_1:0;
		
		EX_INI_INITIALIZE_(Launch,1) //...
	}
	else _section = 0;
}
void EX::INI::Launch::operator+=(Ex_ini_e in)
{
	if(!_section) return;

	_section->times_included++;

	Ex_ini_foreach(in,this,&Launch::mention); //...
}
void EX::INI::Launch::mention(const wchar_t *p, const wchar_t *d, const wchar_t *q)
{	
	Section *nc = _section; nc->total_mentions++;
				  				
	switch(Ex_ini_xlat_launch_section_key(p,d))
	{
	case LAUNCH_INITIALIZATION_FAILURE: nc->total_failures++; break;

	case DO_WITHOUT_THE_EXTENSION_LIBRARY:

		nc->do_without_the_extension_library = Ex_ini_xlat_bin_val(q); break;

	case LAUNCH_TITLE_TO_USE:

		wcscpy_s(nc->launch_title_to_use,MAX_PATH,q); break;

	case LAUNCH_IMAGE_TO_USE:

		wcscpy_s(nc->launch_image_to_use,MAX_PATH,q); break;
	}
}

#undef SOMEX_INI_LAUNCH_SECTION




#define SOMEX_INI_NUMBER_SECTION

EX::INI::Number::Number(int section)
{		
	if(section>=1) 
	{
		//EX::INI::Number(); //???

		static Section _1; //!

		_section = section==1?&_1:0;
	
		EX_INI_INITIALIZE_(Number,1) //...
	}
	else _section = 0;
}
void EX::INI::Number::operator+=(Ex_ini_e in)
{
	if(!_section) return;

	if(_section->times_included++==0) //hacks
	{
		if(EX::INI::Number()==*this) EX::numbers();
	}

	Ex_ini_foreach(in,this,&Number::mention); //... 
}
void EX::INI::Number::mention(const wchar_t *p, const wchar_t *d, const wchar_t *q)
{	
	Section *nc = _section; 

	switch(_section->pass)
	{
	case 0: assert(0);
	case 2: EX::assigning_number(0,p,q); return;
	case 1: EX::declaring_number(0,p,q); break;	
	}

	nc->total_mentions++;
}

#undef SOMEX_INI_NUMBER_SECTION




#define SOMEX_INI_OPTION_SECTION

#define LOOKUP(a,b,c) a, \

static enum
{
	#include "ini.lookup.inl"
};

#define LOOKUP(a,b,c) (const wchar_t*)a, L###c, \

static const wchar_t *Ex_ini_option_lookup[] = 
{
	#include "ini.lookup.inl"

	0, 0
};

#define LOOKUP(a,b,c) Ex_ini_option_lookup[a*2-2]=(const wchar_t*)a; Ex_ini_option_lookup[a*2-1]=L###c; \

static void Ex_ini_option_backup() 
{
	#include "ini.lookup.inl"
}

#undef LOOKUP

static int Ex_ini_xlat_option_section_key(const wchar_t *in, const wchar_t *in_x)
{		
	static int array_size = 0;

	if(!array_size) while(Ex_ini_option_lookup[++array_size*2]);

	return Ex_ini_do_binary_lookup(in,in_x,Ex_ini_option_lookup+2,array_size-1);
}

EX::INI::Option::Option(int section)
{		
	if(section>=1) 
	{
		static Section _1; //!

		_section = section==1?&_1:0;
		
		EX_INI_INITIALIZE_(Option,1) //...
	}
	else _section = 0;
}
void EX::INI::Option::operator+=(Ex_ini_e in)
{
	if(!_section) return;

	_section->times_included++;

	Ex_ini_foreach(in,this,&Option::mention); //...
}
bool EX::INI::Option::Section::initialize_aa(bool force)const
{
	Option::Section *nc = const_cast<Option::Section*>(this); 

	if(force) //F1 toggle?
	{
		//REMINDER: AA can look wrong at reduced frame rates

		if(nc->do_aa) return true;

		nc->do_aa = true; 
	}
	if(do_aa)
	{
		nc->do_aa2 = true;
		nc->do_smooth = 0&&EX::debug?false:true;

		if(!do_lap2) nc->do_lap = nc->do_lap2 = true;
	}
	if(do_lap||do_lap2) //HACK: PIGGYBACKING
	{
		nc->do_lap2 = true; 		
		nc->do_srgb = 0&&EX::debug?false:true;
	}
	return false; //force?
}
void EX::INI::Option::mention(const wchar_t *p, const wchar_t *d, const wchar_t *q)
{	
	bool yes = Ex_ini_xlat_bin_val(q);

	Section *nc = _section; nc->total_mentions++;	

	switch(Ex_ini_xlat_option_section_key(p,d))
	{
	case OPTION_INITIALIZATION_FAILURE: nc->total_failures++; break;
	
	case DO_2K: nc->do_2k = yes; break;

	case DO_LOG: nc->do_log = yes; break;

	case DO_DISASM:	nc->do_disasm = yes; break;

	case DO_WALK: nc->do_walk = yes; break;

	case DO_DASH: nc->do_dash = yes; break;

	case DO_PAUSE: nc->do_pause = yes; break;

	case DO_ESCAPE:	nc->do_escape = yes; break;

	case DO_NUMLOCK: nc->do_numlock = yes; break;

	case DO_CURSOR:	nc->do_cursor = yes; break;

	case DO_WASD: nc->do_wasd = yes; break;

	case DO_MOUSE2: nc->do_mouse2 = yes; //break;
	case DO_MOUSE: nc->do_mouse = yes; break;

	case DO_MIPMAPS: nc->do_mipmaps = yes; break;

	case DO_ALPHAFOG: nc->do_alphafog = yes; break;

	case DO_RANGEFOG: nc->do_rangefog = yes; break;

	case DO_FOG: //macro: nc->do_alphafog+nc->do_rangefog

		nc->do_alphafog = nc->do_rangefog = yes; break;
	
	case DO_SMOOTH:	nc->do_smooth = yes; break;

	case DO_INVERT: nc->do_invert = yes; break;

	case DO_GAMMA: nc->do_gamma = yes; break;
	case DO_BLACK: nc->do_black = yes; break;

	case DO_GREEN: nc->do_green = yes; break;

	case DO_ANISOTROPY: nc->do_anisotropy = yes; break;

	case DO_BPP: nc->do_bpp = yes; break;

	case DO_HIGHCOLOR: nc->do_highcolor = yes; break;

	case DO_OPENGL: nc->do_opengl = yes; break; //2022

	case DO_LIGHTS:	nc->do_lights = yes; break;

	case DO_AMBIENT: nc->do_ambient = yes; break;

	case DO_SOUNDS:	nc->do_sounds = yes; break;

	case DO_SYNCBGM: nc->do_syncbgm = yes; break;
		
	case DO_SYSTEM: nc->do_system = yes; break;

	case DO_HDR: nc->do_hdr = yes; break;

	case DO_ST: nc->do_st = yes; break;

	case DO_START: nc->do_start = yes; break;

	case DO_AA:	//2020: old do_aa is now "do_aa2"
		
		/*2020: initialize/initialize_aa DEFER THIS LOGIC SO ITS
		//SEMANTICS ARE CLEANER FOR Exselector AND GPU VENDOR AA
		if(yes) nc->do_aa2 = nc->do_smooth = nc->do_lap = true;*/
		
		nc->do_aa = yes; break;

	case DO_AA2: nc->do_aa2 = yes; break;

	case DO_LAP: //2021: old do_lap is now "do_lap2"
		
		nc->do_lap = yes; //break
	
	case DO_LAP2: nc->do_lap2 = yes; 

		if(yes) nc->do_srgb = true; break;
	
	case DO_HIT: nc->do_hit = yes; //break;

		if(!yes) nc->do_hit2 = false; break;

	case DO_HIT2: nc->do_hit2 = yes; //break;
		
		if(yes) nc->do_hit = true; break;
			
	case DO_RED: nc->do_red = yes; break;

	case DO_G: nc->do_g = yes; break;
	
	case DO_U: nc->do_u = yes; 

		if(!yes) nc->do_u2 = false; break;

	case DO_U2: nc->do_u2 = yes; 
		
		if(yes) nc->do_u = true; break;

	case DO_SAVE: nc->do_save = yes; break;
	
	case DO_STIPPLE2: //grainy alternating stipple
		
		nc->do_stipple2 = yes; 
		
		if(!yes) break; //falling thru

	case DO_STIPPLE: nc->do_stipple = yes; 
		
		if(!yes){ nc->do_stipple2 = false; break; }
				
	case DO_DITHER:	//falling thru
	
		if(0) case DO_DITHER2: //falling thru
		{
			nc->do_dither2 = yes;
		}
		nc->do_dither = yes; break;	

	case DO_KF2: nc->do_kf2 = yes;
		
		if(!yes) break; //break;

	case DO_NWSE: nc->do_nwse = yes; break;

	case DO_REVERB: nc->do_reverb = yes; break;

	case DO_SRGB: nc->do_srgb = yes; break;

	case DO_ALPHASORT: nc->do_alphasort = yes; break;

//	case DO_SHADOW: nc->do_shadow = yes; break; //constant?

	case DO_BSP: nc->do_bsp = yes; break;
	}
}

#undef SOMEX_INI_OPTION_SECTION






#define SOMEX_INI_OUTPUT_SECTION

#define LOOKUP(a,b,c) a, \

static enum
{
	#include "ini.lookup.inl"
};

#define LOOKUP(a,b,c) (const wchar_t*)a, L###c, \

static const wchar_t *Ex_ini_output_lookup[] = 
{
	#include "ini.lookup.inl"

	0, 0
};

#define LOOKUP(a,b,c) Ex_ini_output_lookup[a*2-2]=(const wchar_t*)a; Ex_ini_output_lookup[a*2-1]=L###c; \

static void Ex_ini_output_backup() 
{
	#include "ini.lookup.inl"
}

#undef LOOKUP

static int Ex_ini_xlat_output_section_key(const wchar_t *in, const wchar_t *in_x)
{		
	static int array_size = 0;

	if(!array_size) while(Ex_ini_output_lookup[++array_size*2]);

	return Ex_ini_do_binary_lookup(in,in_x,Ex_ini_output_lookup+2,array_size-1);
}

static bool Ex_ini_output_log(const wchar_t*,const wchar_t*);
static void Ex_ini_output_loglog(const wchar_t*,const wchar_t*);

bool (*EX::INI::Output::Section::log)(const wchar_t*,const wchar_t*) = 0;

EX::INI::Output::Output(int section)
{		
	if(section>=1) 
	{
		static Section _1; //!

		_section = section==1?&_1:0;
		
		EX_INI_INITIALIZE_(Output,1) //...
	}
	else _section = 0;
}
void EX::INI::Output::operator+=(Ex_ini_e in)
{
	if(!_section) return;

	_section->times_included++;

	Ex_ini_foreach(in,this,&Output::mention); //...
}
void EX::INI::Output::mention(const wchar_t *p, const wchar_t *d, const wchar_t *q)
{	
	Section *nc = _section; nc->total_mentions++;
								 
	bool (*log)(const wchar_t*,const wchar_t*) = nc->log?nc->log:Ex_ini_output_log;
		
	if(log(p,q)) return Ex_ini_output_loglog(p,q);

	int f = 0; DWORD c;

	if(*p=='f') switch(p[1])
	{
	case '1':

		switch(p[2])
		{
		case '_':

			f = 1; p+=3; break;

		case '0': case '1': case '2':

			if(p[3]=='_') //f10~12_
			{
				f = 10+p[2]-'0'; p+=4;
			}
		}

		break;

	case '2': case '3': case '4': case '5': 
	case '6': case '7': case '8': case '9': 

		if(p[2]=='_') //f2~9
		{
			f = p[1]-'0'; p+=3;
		}
	}

	switch(Ex_ini_xlat_output_section_key(p,d))
	{
	case OUTPUT_INITIALIZATION_FAILURE: nc->total_failures++; break;

	case DO_MISSING_FILE_DIALOG_OK:

		nc->do_missing_file_dialog_ok = Ex_ini_xlat_bin_val(q); break;

	case DO_F5_TRIPLE_BUFFER_FPS: //2021

		nc->do_f5_triple_buffer_fps = Ex_ini_xlat_bin_val(q); break;

	case FUNCTION_OVERLAY_TINT:
	
		c = Ex_ini_xlat_col_val(q);

		if(!f) for(int i=1;i<=12;i++)
		{
			nc->function_overlay_tint[i] = c; 
		}
		else nc->function_overlay_tint[f] = c; 

		break;

	case FUNCTION_OVERLAY_CONTRAST:
	
		c = Ex_ini_xlat_col_val(q);

		if(!f) for(int i=1;i<=12;i++)
		{
			nc->function_overlay_contrast[i] = c; 
		}
		else nc->function_overlay_contrast[f] = c; 

		break;
			
	case FUNCTION_OVERLAY_MASK:
	
		if(f)
		{
			nc->function_overlay_mask|=(int)Ex_ini_xlat_bin_val(q)<<(12-f); 
		}
		else nc->function_overlay_mask = Ex_ini_xlat_hex_val(q); 

		//if(nc->function_overlay_mask)
		//courtesy as F12 breaks into debuggers
		//if(EX::debug) nc->function_overlay_mask|=1; 

		break;

	case FUNCTION_OVERLAY_ECLIPSE:
	
		c = Ex_ini_xlat_hex_val(q); 

		if(!f) for(int i=1;i<12;i++) //<=11
		{	
			nc->function_overlay_eclipse[i] = c;
		}
		else if(f!=12) //F12 is itself a mask
		{
			nc->function_overlay_eclipse[f] = c;
		}

		break;

	case ART_ACTION_CENTER_NOTE_MS_TIMEOUT: //2024

		nc->art_action_center_note_ms_timeout = Ex_ini_xlat_int_val(q,10000); 
		
		break;
	}
}

bool EX::INI::Output::Section::f(int i)const
{
	return i>0&&i<=12?function_overlay_tint[i]&0xFF000000:0;
}
bool EX::INI::Output::Section::missing_whitelist(const char *A)
{
	const char *a = PathFindFileNameA(A);

	//RE C:/windows/system32/atipblag.dat
	//there's a discussion of this in the forums
	//some AMD software crashes if its file isn't
	//in the system32 folder (seems unrelated to
	//SOM, but something triggers it)
	//http://www.swordofmoonlight.net/bbs2/index.php?topic=320.0
	if(*a=='*' //amdxc32.dll
	||!stricmp(a,"atiumdag.dll")
	||!stricmp(a,"atidxx32.dll")
	||!stricmp(a,"atipblag.dat") //just CreatFile shows this 
	||!strcmp(a,"disable.txt") //NVIDIA\\DXCache\\disable.txt
	||strstr(A,"NvAdminDevice"))
	{
		return true;
	}
	if(!strncmp(A,"\\\\.\\",4)) // \\.\PIPE ?
	{
		return true;
	}
	return false;
}

static bool Ex_ini_output_log(const wchar_t *key, const wchar_t *val)
{
	int level = Ex_ini_xlat_int_val(val,65535);

	if(level>=65535||level<=-65535) return false;

	if(wcsncmp(key,L"log_",4))
	{
		static const wchar_t *x = EX::log(); 

		static size_t x_s = x?wcslen(x):0;

		if(wcsncmp(key,x,x_s)||key[x_s]!='_') return false;

		if(wcsncmp(key+=x_s+1,L"log_",4)) return false;
	}
	
	const wchar_t *d = wcsrchr(key+=4,'=');

	int keylen = d?d-key:wcslen(key); 

	if(keylen<3||wcsncmp(key+keylen-4,L"_bar",4)) return false;

	wchar_t ns[8] = L"", obj[32] = L"";

	const wchar_t *bar = key+keylen-3;

	if(key!=bar)
	if(d=EX::is_log_object(key))
	{
		wcsncpy_s(obj,32,key,d-key);

		obj[d-key] = '\0';
		
		key = *d?d+1:d;
	}
	if(key!=bar)
	if(d=EX::is_log_namespace(key))
	{
		wcsncpy_s(ns,8,key,d-key);

		ns[d-key] = '\0';
		
		key = *d?d+1:d;
	}

	int logs = 0xF^EX::debug_log;

	if(key!=bar)
	{
		if(key+6!=bar) return false;

		switch(*key)
		{
		case 'd': logs = EX::debug_log; 
		if(wcsncmp(key,L"debug_",6)) return false; break;
		case 'e': logs = EX::error_log; 
		if(wcsncmp(key,L"error_",6)) return false; break;
		case 'p': logs = EX::panic_log; 
		if(wcsncmp(key,L"panic_",6)) return false; break;
		case 'a': logs = EX::alert_log; 
		if(wcsncmp(key,L"alert_",6)) return false; break;
		case 'h': logs = EX::hello_log; 
		if(wcsncmp(key,L"hello_",6)) return false; break;
		case 'i': logs = EX::hello_log; 
		if(wcsncmp(key,L"input_",6)) return false; break;

		default: return false;
		}
	}

	EX::logging(level,logs,ns,obj);

	return true;
}

static void Ex_ini_output_loglog(const wchar_t*,const wchar_t*)
{
	//TODO: implement this (unimplemented)
}

int EX::INI::Output::Section::loglog(int pair, const wchar_t **key, const wchar_t **val)const
{
	assert(0); return 0; //unimplemented
}

#undef SOMEX_INI_OUTPUT_SECTION




#define SOMEX_INI_PLAYER_SECTION

#define LOOKUP(a,b,c) a, \

static enum
{
	#include "ini.lookup.inl"
};

#define LOOKUP(a,b,c) (const wchar_t*)a, L###c, \

static const wchar_t *Ex_ini_player_lookup[] = 
{
	#include "ini.lookup.inl"

	0, 0
};

#define LOOKUP(a,b,c) Ex_ini_player_lookup[a*2-2]=(const wchar_t*)a; Ex_ini_player_lookup[a*2-1]=L###c; \

static void Ex_ini_player_backup() 
{
	#include "ini.lookup.inl"
}

#undef LOOKUP

static int Ex_ini_xlat_player_section_key(const wchar_t *in, const wchar_t *in_x)
{		
	static int array_size = 0;

	if(!array_size) while(Ex_ini_player_lookup[++array_size*2]);

	return Ex_ini_do_binary_lookup(in,in_x,Ex_ini_player_lookup+2,array_size-1);
}

EX::INI::Player::Player(int section)
{		
	if(section>=1) 
	{
		static Section _1; //!

		_section = section==1?&_1:0;
		
		EX_INI_INITIALIZE_(Player,1) //...
	}
	else _section = 0;
}
void EX::INI::Player::operator+=(Ex_ini_e in)
{
	if(!_section) return;

	_section->times_included++;

	Ex_ini_foreach(in,this,&Player::mention); //...
}
void EX::INI::Player::mention(const wchar_t *p, const wchar_t *d, const wchar_t *q)
{	
	Section *nc = _section; nc->total_mentions++;
		
	switch(Ex_ini_xlat_player_section_key(p,d))
	{
	case PLAYER_INITIALIZATION_FAILURE: nc->total_failures++; break;

	case DO_NOT_JUMP: nc->do_not_jump = Ex_ini_xlat_bin_val(q); break;
	case DO_NOT_RUSH: nc->do_not_rush = Ex_ini_xlat_bin_val(q); break;
	case DO_NOT_DASH: nc->do_not_dash = Ex_ini_xlat_bin_val(q); break;
	case DO_NOT_DUCK: nc->do_not_duck = Ex_ini_xlat_bin_val(q); break;

	case CORKSCREW_POWER: nc->corkscrew_power = q; break;						
	case DO_NOT_CORKSCREW: 
		
		nc->do_not_corkscrew = Ex_ini_xlat_bin_val(q); break;
	
	case DO_NOT_SQUAT: nc->do_not_squat = Ex_ini_xlat_bin_val(q); break;
	case DO_NOT_SLIDE: nc->do_not_slide = Ex_ini_xlat_bin_val(q); break;
	case PLAYER_CHARACTER_ARM: nc->player_character_arm = q; break;
	case PLAYER_CHARACTER_SLIP: nc->player_character_slip = q; break;
	case PLAYER_CHARACTER_BOB:  if(!(q=nc->player_character_bob<<=q)) break;
	case PLAYER_CHARACTER_BOB2: if(!(q=nc->player_character_bob2<<=q)) break;
	case PLAYER_CHARACTER_BOB3: nc->player_character_bob3 = q; break;	
	case PLAYER_CHARACTER_NOD:  if(!(q=nc->player_character_nod<<=q)) break;
	case PLAYER_CHARACTER_NOD2: nc->player_character_nod2 = q; break;
	case PLAYER_CHARACTER_DIP:  if(!(q=nc->player_character_dip<<=q)) break;
	case PLAYER_CHARACTER_DIP2: nc->player_character_dip2 = q; break;		
	case PLAYER_CHARACTER_DUCK:  if(!(q=nc->player_character_duck<<=q)) break;
	case PLAYER_CHARACTER_DUCK2: if(!(q=nc->player_character_duck2<<=q)) break;
	case PLAYER_CHARACTER_DUCK3: nc->player_character_duck3 = q; break;				
	case PLAYER_CHARACTER_SHAPE:  if(!(q=nc->player_character_shape<<=q)) break;
	case PLAYER_CHARACTER_SHAPE2:  if(!(q=nc->player_character_shape2<<=q)) break;
	case PLAYER_CHARACTER_SHAPE3: nc->player_character_shape3 = q; break;
	case PLAYER_CHARACTER_HEIGHT: if(!(q=nc->player_character_height<<=q)) break;
	case PLAYER_CHARACTER_HEIGHT2: nc->player_character_height2 = q; break;
	case PLAYER_CHARACTER_INSEAM: nc->player_character_inseam = q; break;
	case PLAYER_CHARACTER_WEIGHT: if(!(q=nc->player_character_weight<<=q)) break;
	case PLAYER_CHARACTER_WEIGHT2: nc->player_character_weight2 = q; break;
	case PLAYER_CHARACTER_FENCE:  if(!(q=nc->player_character_fence<<=q)) break;
	case PLAYER_CHARACTER_FENCE2: if(!(q=nc->player_character_fence2<<=q)) break;
	case PLAYER_CHARACTER_FENCE3: if(!(q=nc->player_character_fence3<<=q)) break;
	case PLAYER_CHARACTER_FENCE4: if(!(q=nc->player_character_fence4<<=q)) break;
	case PLAYER_CHARACTER_FENCE5: nc->player_character_fence5 = q; break;
	case PLAYER_CHARACTER_RADIUS: if(!(q=nc->player_character_radius<<=q)) break;
	case PLAYER_CHARACTER_RADIUS2: if(!(q=nc->player_character_radius2<<=q)) break;
	case PLAYER_CHARACTER_RADIUS3: if(!(q=nc->player_character_radius3<<=q)) break;
	case PLAYER_CHARACTER_RADIUS4: nc->player_character_radius4 = q; break;		
	case PLAYER_CHARACTER_SCALE: nc->player_character_scale = q; break;		
	case PLAYER_CHARACTER_SHADOW: nc->player_character_shadow = q; break;
	case PLAYER_CHARACTER_STATURE: nc->player_character_stature = q; break;	
	case PLAYER_CHARACTER_STRIDE: nc->player_character_stride = q; break;	

	case TAP_OR_HOLD_MS_TIMEOUT: nc->tap_or_hold_ms_timeout = q; break;

	case SUBTITLES_MS_INTERIM: nc->subtitles_ms_interim = q; break;
	case SUBTITLES_MS_TIMEOUT: nc->subtitles_ms_timeout = q; break;

	case PLAYER_FILE_EXTENSION:
	
		Ex_ini_xlat_string_val(nc->player_file_extension,q); break;

	case DO_2000_COMPATIBLE_PLAYER_FILE:

		nc->do_2000_compatible_player_file = Ex_ini_xlat_bin_val(q); break;

	case ARM_MS_WINDUP: if(!(q=nc->arm_ms_windup<<=q)) break;
	case ARM_MS_WINDUP2: nc->arm_ms_windup2 = q; break;

	case ARM_BICEP: if(!(q=nc->arm_bicep<<=q)) break;
	case ARM_BICEP2: nc->arm_bicep2 = q; break;
	}
}
	
#undef SOMEX_INI_PLAYER_SECTION




#define SOMEX_INI_SAMPLE_SECTION

#define LOOKUP(a,b,c) a, \

static enum
{
	#include "ini.lookup.inl"
};

#define LOOKUP(a,b,c) (const wchar_t*)a, L###c, \

static const wchar_t *Ex_ini_sample_lookup[] = 
{
	#include "ini.lookup.inl"

	0, 0
};

#define LOOKUP(a,b,c) Ex_ini_sample_lookup[a*2-2]=(const wchar_t*)a; Ex_ini_sample_lookup[a*2-1]=L###c; \

static void Ex_ini_sample_backup() 
{
	#include "ini.lookup.inl"
}

#undef LOOKUP

static int Ex_ini_xlat_sample_section_key(const wchar_t *in, const wchar_t *in_x)
{		
	static int array_size = 0;

	if(!array_size) while(Ex_ini_sample_lookup[++array_size*2]);

	return Ex_ini_do_binary_lookup(in,in_x,Ex_ini_sample_lookup+2,array_size-1);
}

EX::INI::Sample::Sample(int section)
{		
	if(section>=1) 
	{
		static Section _1; //!

		_section = section==1?&_1:0;
		
		EX_INI_INITIALIZE_(Sample,1) //...
	}
	else _section = 0;
}
void EX::INI::Sample::operator+=(Ex_ini_e in)
{
	if(!_section) return;

	_section->times_included++;

	Ex_ini_foreach(in,this,&Sample::mention); //...
}
void EX::INI::Sample::mention(const wchar_t *p, const wchar_t *d, const wchar_t *q)
{	
	Section *nc = _section; nc->total_mentions++;
		
	switch(Ex_ini_xlat_sample_section_key(p,d))
	{
	case SAMPLE_INITIALIZATION_FAILURE: nc->total_failures++; break;

	case FOOTSTEP_IDENTIFIER: nc->footstep_identifier = q; break;
	case HEADBUTT_IDENTIFIER: nc->headbutt_identifier = q; break;
	case HEADWIND_IDENTIFIER: nc->headwind_identifier = q; break;
	case RICOCHET_IDENTIFIER: nc->ricochet_identifier = q; break;

	case SAMPLE_PITCH_ADJUSTMENT: nc->sample_pitch_adjustment = q; break; 
	case SAMPLE_VOLUME_ADJUSTMENT: nc->sample_volume_adjustment = q; break; 

	case HEADWIND_AMBIENT_WIND_EFFECT: nc->headwind_ambient_wind_effect = q; break;
	}
}
	
#undef SOMEX_INI_SAMPLE_SECTION






#define SOMEX_INI_SCRIPT_SECTION

#define LOOKUP(a,b,c) a, \

static enum
{
	#include "ini.lookup.inl"
};

#define LOOKUP(a,b,c) (const wchar_t*)a, L###c, \

static const wchar_t *Ex_ini_script_lookup[] = 
{
	#include "ini.lookup.inl"

	0, 0
};

#define LOOKUP(a,b,c) Ex_ini_script_lookup[a*2-2]=(const wchar_t*)a; Ex_ini_script_lookup[a*2-1]=L###c; \

static void Ex_ini_script_backup() 
{
	#include "ini.lookup.inl"
}

#undef LOOKUP

static int Ex_ini_xlat_script_section_key(const wchar_t *in, const wchar_t *in_x)
{		
	static int array_size = 0;

	if(!array_size) while(Ex_ini_script_lookup[++array_size*2]);

	return Ex_ini_do_binary_lookup(in,in_x,Ex_ini_script_lookup+2,array_size-1);
}

static const wchar_t **Ex_ini_script_fonts_lookup = 0;

static void Ex_ini_script_use_fonts(const wchar_t *key, 
									const wchar_t *key_x, const wchar_t *val)
{	
	static int replacing = 0, available = 0;

	if(!key||!key_x||key_x<key+1||key_x-key>64||!val) return;

	if(!Ex_ini_script_fonts_lookup)
	{			
		Ex_ini_script_fonts_lookup = new const wchar_t*[32]; 

		memset(Ex_ini_script_fonts_lookup,0x00,sizeof(wchar_t*)*32);

		replacing = 0; available = 16;
	}
	else if(replacing>=available-1) 
	{
		//crazy! mixing new/delete and wcsdup/free

		assert(0); //unimplemented

		return;
	}

	Ex_ini_script_fonts_lookup[replacing*2] = wcsdup(val);
	
	int len = key_x-key; wchar_t *old = new wchar_t[len+1];

	memcpy(old,key,2*len); old[len] = '\0';

	for(int i=0;i<len;i++) if(old[i]=='_') old[i] = ' ';

	Ex_ini_script_fonts_lookup[replacing*2+1] = old;

	replacing++; 
}

//REMOVE ME?
const wchar_t *EX::INI::Script::Section::fonts_to_use_instead_of_(const wchar_t *in)const
{
	if(!Ex_ini_script_fonts_lookup) return 0; static int one_off = -1; //??? 

	if(one_off==-1) for(one_off=0;Ex_ini_script_fonts_lookup[one_off*2];) one_off++; //??? 

	static const wchar_t *cache = 0, *cached = 0; if(one_off<1) return 0;

	if(cache&&!wcscmp(in,cache)) return cached; cache = in;

	cached = Ex_ini_do_binary_lookup(in,Ex_ini_script_fonts_lookup,one_off);

	return cached;
}

EX::INI::Script::Script(int section)
{		
	if(section>=1) 
	{
		static Section _1; //!

		_section = section==1?&_1:0;
		
		EX_INI_INITIALIZE_(Script,1) //...
	}
	else _section = 0;
}
void EX::INI::Script::operator+=(Ex_ini_e in)
{
	if(!_section) return;

	_section->times_included++;

	if(!in||!*in) return;

	Ex_ini_foreach(in,this,&Script::mention); //...
	 
	if(Ex_ini_script_fonts_lookup)
	{
		int i; for(i=0;Ex_ini_script_fonts_lookup[i*2];i++);

		Ex_ini_do_lookup_quicksort(Ex_ini_script_fonts_lookup,i);
	}
}
void EX::INI::Script::mention(const wchar_t *p, const wchar_t *d, const wchar_t *q)
{	
	Section *nc = _section; nc->total_mentions++;
	
	switch(Ex_ini_xlat_script_section_key(p,d))
	{	
	case SCRIPT_INITIALIZATION_FAILURE: //! 
	{	
		//nc->total_failures++; break;
				
		const wchar_t *instead = L"fonts_to_use_instead_of_";

		size_t insteadlen = sizeof("fonts_to_use_instead_of_")-1;
				
		if(size_t(q-p)>insteadlen&&!wcsncmp(instead,p,insteadlen))
		{
			return Ex_ini_script_use_fonts(p+insteadlen,q-1,q);
		}
		
		nc->total_failures++; break;
	}	  	
	case SYSTEM_FONTS_TO_USE: wcscpy_s(nc->system_fonts_to_use,MAX_PATH,q); break; 
	case STATUS_FONTS_TO_USE: wcscpy_s(nc->status_fonts_to_use,MAX_PATH,q); break; 

	case STATUS_FONTS_HEIGHT: nc->status_fonts_height = q; break;
	
	case SYSTEM_FONTS_TINT: EX_INI_MENTION_(col,q,nc->system_fonts_tint); break;
	case STATUS_FONTS_TINT: EX_INI_MENTION_(col,q,nc->status_fonts_tint); break;
	
	case SYSTEM_FONTS_CONTRAST: EX_INI_MENTION_(col,q,nc->system_fonts_contrast); break;
	case STATUS_FONTS_CONTRAST: EX_INI_MENTION_(col,q,nc->status_fonts_contrast); break;
		
	case LOCALE_TO_USE_FOR_PLAY:

		Ex_ini_xlat_string_val(nc->locale_to_use_for_play,q,126); break; 

	case LOCALE_TO_USE_FOR_PLAY_IF_NOT_FOUND:

		Ex_ini_xlat_string_val(nc->locale_to_use_for_play_if_not_found,q,126); break; 

	case LOCALE_TO_USE_FOR_DATES_AND_FIGURES:

		Ex_ini_xlat_string_val(nc->locale_to_use_for_dates_and_figures,q,126); break; 

	case TRANSLATION_TO_USE_FOR_PLAY:

		wcscpy_s(nc->translation_to_use_for_play,MAX_PATH,q); break; 
		
	case DO_USE_BUILTIN_ENGLISH_BY_DEFAULT:

		nc->do_use_builtin_english_by_default = Ex_ini_xlat_bin_val(q); break;

	case DO_USE_BUILTIN_LOCALE_DATE_FORMAT:

		nc->do_use_builtin_locale_date_format = Ex_ini_xlat_bin_val(q); break;

	case DO_NOT_SHOW_BUTTON_NAMES:
		
		nc->do_not_show_button_names = Ex_ini_xlat_bin_val(q); break;
	}
}

#undef SOMEX_INI_SCRIPT_SECTION






#define SOMEX_INI_STEREO_SECTION

#define LOOKUP(a,b,c) a, \

static enum 
{
	#include "ini.lookup.inl"
};

#define LOOKUP(a,b,c) (const wchar_t*)a, L###c, \

static const wchar_t *Ex_ini_stereo_lookup[] = 
{
	#include "ini.lookup.inl"

	0, 0
};

#define LOOKUP(a,b,c) Ex_ini_stereo_lookup[a*2-2]=(const wchar_t*)a; Ex_ini_stereo_lookup[a*2-1]=L###c; \

static void Ex_ini_stereo_backup() 
{
	#include "ini.lookup.inl"
}

#undef LOOKUP

static int Ex_ini_xlat_stereo_section_key(const wchar_t *in, const wchar_t *in_x)
{		
	static int array_size = 0;

	if(!array_size) while(Ex_ini_stereo_lookup[++array_size*2]);

	return Ex_ini_do_binary_lookup(in,in_x,Ex_ini_stereo_lookup+2,array_size-1);
}

EX::INI::Stereo::Stereo(int section)
{		
	if(section>=1) 
	{
		static Section _1; //!

		_section = section==1?&_1:0;
		
		EX_INI_INITIALIZE_(Stereo,1) //...
	}
	else _section = 0;
}
void EX::INI::Stereo::operator+=(Ex_ini_e in)
{
	if(!_section) return;

	_section->times_included++;

	Ex_ini_foreach(in,this,&Stereo::mention); //...
}
void EX::INI::Stereo::mention(const wchar_t *p, const wchar_t *d, const wchar_t *q)
{	
	Section *nc = _section; nc->total_mentions++;
		
	switch(Ex_ini_xlat_stereo_section_key(p,d))
	{
	case STEREO_INITIALIZATION_FAILURE: nc->total_failures++; break;

	case DIM_ADJUSTMENT: nc->dim_adjustment = q; break;

	case BETA_CONSTANT: nc->beta_constant = q; break;

	case FUZZ_CONSTANT: nc->fuzz_constant = q; break; 

	case ZOOM: nc->zoom = q; break; //EXPERIMENTAL

	case PSVRTOOLBOX_STATUS_PORT: //demonstration?
		
		nc->PSVRToolbox_status_port = Ex_ini_xlat_int_val(q); break;

	case PSVRTOOLBOX_COMMAND_PORT: //demonstration? 
		
		nc->PSVRToolbox_command_port = Ex_ini_xlat_int_val(q); break;

	case STARTNOLOSERVER_FOLDER:

		wcscpy_s(nc->StartNoloServer_folder,MAX_PATH,q); break;

	case DO_NOT_AA_in_Stereo: nc->do_not_aa = Ex_ini_xlat_bin_val(q); break;

	case DO_NOT_DITHER_in_Stereo: nc->do_not_dither = Ex_ini_xlat_bin_val(q); break; 

	case DO_NOT_FORCE_FULL_COLOR_DEPTH: 
		
		nc->do_not_force_full_color_depth = Ex_ini_xlat_bin_val(q); break;

	case DO_NOT_LAP: nc->do_not_lap = Ex_ini_xlat_bin_val(q); break;

	case DO_SMOOTH_in_Stereo: nc->do_smooth = Ex_ini_xlat_bin_val(q); break;

	case DO_STIPPLE_in_Stereo: nc->do_stipple = Ex_ini_xlat_bin_val(q); break;

	case KF2_DEMO_BEVEL: nc->kf2_demo_bevel = q; break; //EXPERIMENTAL

	case STEREO_Y: while(isspace(*q)) q++; nc->stereo_y = q; break;

	case STEREO_W: nc->stereo_w = q; break;

	case AA_DIVISOR: nc->aa_divisor = Ex_ini_xlat_int_val(q); break;
	}
}
	
#undef SOMEX_INI_STEREO_SECTION





#define SOMEX_INI_SYSTEM_SECTION

#define LOOKUP(a,b,c) a, \

static enum
{
	#include "ini.lookup.inl"
};

#define LOOKUP(a,b,c) (const wchar_t*)a, L###c, \

static const wchar_t *Ex_ini_system_lookup[] = 
{
	#include "ini.lookup.inl"

	0, 0
};

#define LOOKUP(a,b,c) Ex_ini_system_lookup[a*2-2]=(const wchar_t*)a; Ex_ini_system_lookup[a*2-1]=L###c; \

static void Ex_ini_system_backup() 
{
	#include "ini.lookup.inl"
}

#undef LOOKUP

static int Ex_ini_xlat_system_section_key(const wchar_t *in, const wchar_t *in_x)
{		
	static int array_size = 0;

	if(!array_size) while(Ex_ini_system_lookup[++array_size*2]);

	return Ex_ini_do_binary_lookup(in,in_x,Ex_ini_system_lookup+2,array_size-1);
}

static void Ex_ini_system_register(const wchar_t *dll)
{
	//TODO: Find out if the dll is already registered?

	//Pointing HKLM to HKCU for Dll registration purposes

	//Rationale: registering under HKLM requires admin elevation

	HKEY key = 0; LONG err = 0;
	RegOpenKeyA(HKEY_CURRENT_USER,"Software\\Classes",&key);

	//Note: most online sources including Microsofts docs
	//will say to "use classes root" which is HKCU with a 
	//fallback to HKLM. However this does not seem to work!
	//It may have worked pre-Vista. Sources do not recommend
	//explicitly using HKLM, but it works and seems right.
	//Might be a good idea to do both...

	if(key) err|=RegOverridePredefKey(HKEY_LOCAL_MACHINE,key);

	if(key) RegCloseKey(key); assert(err==ERROR_SUCCESS);

	bool ok = false;
	HMODULE m = LoadLibraryW(dll); if(m)
	{
		wchar_t name[MAX_PATH] = L"";
		GetModuleFileNameW(m,name,MAX_PATH);

		EXLOG_HELLO(0) << "Registering " << name << '\n';

		HRESULT (__stdcall *cb)() = 0;
		*(void**)&cb = GetProcAddress(m,"DllRegisterServer");
		
		if(cb) ok = cb()==S_OK; assert(ok);

		FreeLibrary(m); assert(cb);
	}
	else assert(0);

	if(!ok) EXLOG_ALERT(0) << "Warning! Unable to regiser dll (" << dll << "\n";

	RegOverridePredefKey(HKEY_LOCAL_MACHINE,0);
}

EX::INI::System::System(int section)
{		
	if(section>=1) 
	{
		static Section _1; //!

		_section = section==1?&_1:0;
		
		EX_INI_INITIALIZE_(System,1) //...
	}
	else _section = 0;
}
void EX::INI::System::operator+=(Ex_ini_e in)
{
	if(!_section) return;

	_section->times_included++;

	Ex_ini_foreach(in,this,&System::mention); //...
}
void EX::INI::System::mention(const wchar_t *p, const wchar_t *d, const wchar_t *q)
{	
	Section *nc = _section; nc->total_mentions++;
	
	switch(Ex_ini_xlat_system_section_key(p,d))
	{
	case SYSTEM_INITIALIZATION_FAILURE: //!
	{
		//nc->total_failures++; break;

		const wchar_t *register_ = L"register_";

		size_t register_s = sizeof("register_")-1;
		
		if(!wcsncmp(p,L"register_",register_s))
		{
			return Ex_ini_system_register(q);
		}				
	
		nc->total_failures++; break;
	}
	case CENTRAL_PROCESSING_UNITS:
		nc->central_processing_units = Ex_ini_xlat_int_val(q); break;
		break;
	case DO_DIRECT3D_MULTI_THREAD: //2022
		nc->do_direct3d_multi_thread = Ex_ini_xlat_bin_val(q); 
		break;

	case MASTER_VOLUME_MUTE:
		nc->master_volume_mute = Ex_ini_xlat_int_val(q); break;
		break;
	case MASTER_VOLUME_LOUD:
		nc->master_volume_loud = Ex_ini_xlat_int_val(q); break;
		break;
	case DO_HIDE_MASTER_VOLUME_ON_STARTUP:
		nc->do_hide_master_volume_on_startup = Ex_ini_xlat_bin_val(q); break;
		break;
	}
}

#undef SOMEX_INI_SYSTEM_SECTION




#define SOMEX_INI_VOLUME_SECTION

#define LOOKUP(a,b,c) a, \

static enum
{
	#include "ini.lookup.inl"
};

#define LOOKUP(a,b,c) (const wchar_t*)a, L###c, \

static const wchar_t *Ex_ini_volume_lookup[] = 
{
	#include "ini.lookup.inl"

	0, 0
};

#define LOOKUP(a,b,c) Ex_ini_volume_lookup[a*2-2]=(const wchar_t*)a; Ex_ini_volume_lookup[a*2-1]=L###c; \

static void Ex_ini_volume_backup() 
{
	#include "ini.lookup.inl"
}

#undef LOOKUP

static int Ex_ini_xlat_volume_section_key(const wchar_t *in, const wchar_t *in_x)
{		
	static int array_size = 0;

	if(!array_size) while(Ex_ini_volume_lookup[++array_size*2]);

	return Ex_ini_do_binary_lookup(in,in_x,Ex_ini_volume_lookup+2,array_size-1);
}

EX::INI::Volume::Volume(int section)
{		
	if(section>=1) 
	{
		static Section _1; //!

		_section = section==1?&_1:0;
		
		EX_INI_INITIALIZE_(Volume,1) //...

		std::sort(_1.textures.begin(),_1.textures.end());
	}
	else _section = 0;
}
void EX::INI::Volume::operator+=(Ex_ini_e in)
{
	if(!_section) return;

	_section->times_included++;

	Ex_ini_foreach(in,this,&Volume::mention); //...
}
void EX::INI::Volume::mention(const wchar_t *p, const wchar_t *d, const wchar_t *q)
{	
	Section *nc = _section; nc->total_mentions++;
		
	if(isdigit(*p))
	{
		EX::INI::volume_texture t;
		
		wchar_t *e; t.group = wcstol(p,&e,10);

		if(*e=='_'&&e>p&&e<d)
		{
			t.file_name = ++e; 
			while(!isspace(*e)&&e<d)
			e++;
			t.file_name_s = e-t.file_name;
			while(isspace(*q)) q++;
			t.event_name = q;
			nc->textures.push_back(t);
			return;		
		}

		nc->total_failures++;
	}
	else switch(Ex_ini_xlat_volume_section_key(p,d))
	{
	case VOLUME_INITIALIZATION_FAILURE: nc->total_failures++; break;

	case VOLUME_DEPTH: nc->volume_depth = q; break;
	case VOLUME_POWER: nc->volume_power = q; break;
	case VOLUME_SIDES: nc->volume_sides = q; break;

	case VOLUME_FOG_FACTOR: nc->volume_fog_factor = q; break;

	case VOLUME_FOG_SATURATION: nc->volume_fog_saturation = q; break;
	}
}
	
#undef SOMEX_INI_VOLUME_SECTION




#define SOMEX_INI_WINDOW_SECTION

#define LOOKUP(a,b,c) a, \

static enum
{
	#include "ini.lookup.inl"
};

#define LOOKUP(a,b,c) (const wchar_t*)a, L###c, \

static const wchar_t *Ex_ini_window_lookup[] = 
{
	#include "ini.lookup.inl"

	0, 0
};

#define LOOKUP(a,b,c) Ex_ini_window_lookup[a*2-2]=(const wchar_t*)a; Ex_ini_window_lookup[a*2-1]=L###c; \

static void Ex_ini_window_backup() 
{
	#include "ini.lookup.inl"
}

#undef LOOKUP

static int Ex_ini_xlat_window_section_key(const wchar_t *in, const wchar_t *in_x)
{		
	static int array_size = 0;

	if(!array_size) while(Ex_ini_window_lookup[++array_size*2]);

	return Ex_ini_do_binary_lookup(in,in_x,Ex_ini_window_lookup+2,array_size-1);
}

EX::INI::Window::Window(int section)
{		
	if(section>=1) 
	{
		static Section _1; //!

		_section = section==1?&_1:0;
		
		EX_INI_INITIALIZE_(Window,1) //...
	}
	else _section = 0;
}
void EX::INI::Window::operator+=(Ex_ini_e in)
{
	if(!_section) return;

	_section->times_included++;

	Ex_ini_foreach(in,this,&Window::mention); //...
}
void EX::INI::Window::Section::initialize_defaults()const
{
	auto *nc = const_cast<Window::Section*>(this); 

		if(total_mentions) return;

	nc->do_force_fullscreen_inside_window = true;
	assert(nc->do_scale_640x480_modes_to_setting); //2022
	nc->do_auto_pause_window_while_inactive = true;
}
void EX::INI::Window::mention(const wchar_t *p, const wchar_t *d, const wchar_t *q)
{	
	Section *nc = _section; nc->total_mentions++;
				
	switch(Ex_ini_xlat_window_section_key(p,d))
	{
	case WINDOW_INITIALIZATION_FAILURE: nc->total_failures++; break;

	case DO_FORCE_FULLSCREEN_INSIDE_WINDOW:

		nc->do_force_fullscreen_inside_window = Ex_ini_xlat_bin_val(q); break;

	case DO_SCALE_640X480_MODES_TO_SETTING:

		//2022: forcing on for som_MPX_device_reset, etc.
		//nc->do_scale_640x480_modes_to_setting = Ex_ini_xlat_bin_val(q); break;
		break;

	case DO_NOT_COMPROMISE_FULLSCREEN_MODE:
		
		nc->do_not_compromise_fullscreen_mode = Ex_ini_xlat_bin_val(q); break;

	case DO_CENTER_PICTURE_FOR_FIXED_DISPLAY:
		
		nc->do_center_picture_for_fixed_display = Ex_ini_xlat_bin_val(q); break;

	case DO_FORCE_IMMEDIATE_VERTICAL_REFRESH:
		
		nc->do_force_immediate_vertical_refresh = Ex_ini_xlat_bin_val(q); break;

	case DO_USE_INTERLACED_SCANLINE_ORDERING:
		
		nc->do_use_interlaced_scanline_ordering = Ex_ini_xlat_bin_val(q); break;

	case DO_AUTO_PAUSE_WINDOW_WHILE_INACTIVE:
		
		nc->do_auto_pause_window_while_inactive = Ex_ini_xlat_bin_val(q); break;
	
	case DO_FORCE_REFRESH_ON_VERTICAL_BLANK:
		
		nc->do_force_refresh_on_vertical_blank = Ex_ini_xlat_bin_val(q); break;

	case DO_NOT_HIDE_WINDOW_ON_STARTUP:
		
		nc->do_not_hide_window_on_startup = Ex_ini_xlat_bin_val(q); break;

	case DO_HIDE_COMPANY_TITLE_ON_STARTUP:
	case DO_HIDE_SPLASH_SCREEN_ON_STARTUP: //deprecated
		
		nc->do_hide_company_title_on_startup = Ex_ini_xlat_bin_val(q); break;

	case DO_HIDE_OPENING_MOVIE_ON_STARTUP:

		nc->do_hide_opening_movie_on_startup = Ex_ini_xlat_bin_val(q); break;

	case RESTRICT_ASPECT_RATIO_TO: //REMOVE ME?

		nc->restrict_aspect_ratio_to = Ex_ini_xlat_4cc_val(q); break;

	case WINDOW_TITLE_TO_USE:

		wcscpy_s(nc->window_title_to_use,MAX_PATH,q); break; 

	case DIRECTX_VERSION_TO_USE_FOR_WINDOW:
	{
		//2021: this was defaulting to 0... I want to give it
		//a reliable default so it doesn't have to be tested
		//always at runtime (it's sometimes accessed during 
		//the reprogramming phase before initialization)
		int i = Ex_ini_xlat_int_val(q); switch(i)
		{
		case 7: default: assert(i==7); //NEW
			
			if(!EX::debug) i = 9; //7 is too broken for now

		case 9: case 11: case 12: case 32: case 95: break;
		}
		nc->directx_version_to_use_for_window = i; break;
	}
	case SHADER_MODEL_TO_USE:

		nc->shader_model_to_use = Ex_ini_xlat_int_val(q); break;

	case SHADER_COMPATIBILITY_LEVEL:

		nc->shader_compatibility_level = Ex_ini_xlat_int_val(q,3); break;

	case SPLASH_SCREEN_MS_TIMEOUT:

		nc->splash_screen_ms_timeout = Ex_ini_xlat_int_val(q,3); break;

	case INTERIOR_BORDER:

		for(int i=0;q[i];i++) if(!isdigit(q[i])) ib_hex:
		{
			EX_INI_MENTION_(hex,q,nc->interior_border);
			return; //break;
		}
		if(DWORD x=wcstol(q,0,10)) if(x<=255)
		{
			nc->interior_border = x<<24|x<<16|x<<8|x; 
		}
		else goto ib_hex; break;

	case SECONDS_PER_FRAME:

		nc->seconds_per_frame = Ex_ini_xlat_dec_val(q); break;
	}
}

#undef SOMEX_INI_WINDOW_SECTION

void EX::INI::initialize()
{		
	       //shortcodes
	EX::INI::Number no; //first	
	EX::INI::Script lc;
	EX::INI::Launch go;
	EX::INI::System os;
	EX::INI::Output tt;
	EX::INI::Editor ed;	
	EX::INI::Bugfix bf; //games	
	EX::INI::Author au;
	EX::INI::Player pc;
	EX::INI::Boxart cd;
	
//	EX::INI::Keygen kg;
	EX::INI::Option op; op->initialize_aa();
	EX::INI::Detail dt;
	EX::INI::Damage hp;
	EX::INI::Action ax;
	EX::INI::Keymap km;
	EX::INI::Keypad kp;		
	EX::INI::Joypad jp;
	EX::INI::Analog am;
	EX::INI::Button bt;
	EX::INI::Adjust ad;	
	EX::INI::Window wn; wn->initialize_defaults();		
	EX::INI::Sample se;	
	EX::INI::Stereo vr;
	EX::INI::Volume vt;
	EX::INI::Bitmap bm;
}

static const char *Ex_ini_dik_lookup[] =
{
	"\x0B" "0",	//#define DIK_0 0x0B //dinput.h
	"\x02" "1",
	"\x03" "2",
	"\x04" "3",
	"\x05" "4",
	"\x06" "5",
	"\x07" "6",
	"\x08" "7",
	"\x09" "8",
	"\x0A" "9",
	"\x1E" "A",
	"\x73" "ABNT_C1",
	"\x7E" "ABNT_C2",
	"\x4E" "ADD",
	"\x28" "APOSTROPHE",
	"\xDD" "APPS",
	"\x91" "AT",
	"\x96" "AX",	
	"\x30" "B",
	"\x0E" "BACK",	
	"\x2B" "BACKSLASH",
	"\x0E" "BACKSPACE",

	//REFERENCE
	//2020: undocumented Control+Pause that toggled once
	//but I can't make it toggle again
	"\xC6" "BREAK", 

	"\x2E" "C",
	"\xA1" "CALCULATOR",
	"\x3A" "CAPITAL",
	"\x3A" "CAPSLOCK",		
	"\x90" "CIRCUMFLEX",
	"\x92" "COLON",
	"\x33" "COMMA",	
	"\x79" "CONVERT",	
	"\x20" "D",
	"\x53" "DECIMAL",
	"\xD3" "DELETE",
	"\xB5" "DIVIDE",
	"\xD0" "DOWN",
	"\xD0" "DOWNARROW",
	"\x12" "E",
	"\xCF" "END",
	"\x0D" "EQUALS",
	"\x01" "ESCAPE",
	"\x21" "F",	 	
	"\x3B" "F1",
	"\x44" "F10",
  	"\x57" "F11",			
	"\x58" "F12",
	"\x64" "F13",
	"\x65" "F14",
	"\x66" "F15",	
	"\x3C" "F2",
	"\x3D" "F3",	
	"\x3E" "F4",
	"\x3F" "F5",			
	"\x40" "F6",
	"\x41" "F7",
	"\x42" "F8",
	"\x43" "F9",		
	"\x22" "G",
	"\x29" "GRAVE",
	"\x23" "H",
	"\xC7" "HOME",
	"\x17" "I",
	"\xD2" "INSERT",
	"\x24" "J",
	"\x25" "K",
	"\x70" "KANA",
	"\x94" "KANJI",
	"\x26" "L",
	"\x38" "LALT",	
	"\x1A" "LBRACKET",
	"\x1D" "LCONTROL",
	"\xCB" "LEFT",
	"\xCB" "LEFTARROW",		
	"\x38" "LMENU",
	"\x2A" "LSHIFT",
	"\xDB" "LWIN",	
	"\x32" "M",
	"\xEC" "MAIL",
	"\xED" "MEDIASELECT",	
	"\xA4" "MEDIASTOP",
	"\x0C" "MINUS",
	"\x37" "MULTIPLY",
	"\xA0" "MUTE",	
	"\xEB" "MYCOMPUTER",
	"\x31" "N",
	"\xD1" "NEXT",
	"\x99" "NEXTTRACK",
	"\x7B" "NOCONVERT",
	"\x45" "NUMLOCK",
	"\x52" "NUMPAD0",
	"\x4F" "NUMPAD1",
	"\x50" "NUMPAD2",
	"\x51" "NUMPAD3",
	"\x4B" "NUMPAD4",
	"\x4C" "NUMPAD5",
	"\x4D" "NUMPAD6",
	"\x47" "NUMPAD7",
	"\x48" "NUMPAD8",
	"\x49" "NUMPAD9",
	"\xB3" "NUMPADCOMMA",
	"\x9C" "NUMPADENTER",
	"\x8D" "NUMPADEQUALS",
	"\x4A" "NUMPADMINUS",	
	"\x53" "NUMPADPERIOD",
	"\x4E" "NUMPADPLUS",	
	"\xB5" "NUMPADSLASH",
	"\x37" "NUMPADSTAR",	
	"\x18" "O",
	"\x56" "OEM_102",
	"\x19" "P",
	"\xC5" "PAUSE",
	"\x34" "PERIOD",
	"\xD1" "PGDN",
	"\xC9" "PGUP",	
	"\xA2" "PLAYPAUSE",
	"\xDE" "POWER",
	"\x90" "PREVTRACK",	
	"\xC9" "PRIOR",
	"\x10" "Q",
	"\x13" "R",
	"\xB8" "RALT",
	"\x1B" "RBRACKET",
	"\x9D" "RCONTROL",	
	"\x1C" "RETURN",
	"\xCD" "RIGHT",
	"\xCD" "RIGHTARROW",
	"\xB8" "RMENU",
	"\x36" "RSHIFT",
	"\xDC" "RWIN",
	"\x1F" "S",
	"\x46" "SCROLL",
	"\x27" "SEMICOLON",
	"\x35" "SLASH",	
	"\xDF" "SLEEP",	
	"\x39" "SPACE",
	"\x95" "STOP",
	"\x4A" "SUBTRACT",
	"\xB7" "SYSRQ",	
	"\x14" "T",
	"\x0F" "TAB",
	"\x16" "U",
	"\x93" "UNDERLINE",
	"\x97" "UNLABELED",
	"\xC8" "UP",
	"\xC8" "UPARROW",
	"\x2F" "V",
	"\xAE" "VOLUMEDOWN",
	"\xB0" "VOLUMEUP",
	"\x11" "W",	
	"\xE3" "WAKE",		
	"\xEA" "WEBBACK",	
	"\xE6" "WEBFAVORITES",	
	"\xE9" "WEBFORWARD",
	"\xB2" "WEBHOME",
	"\xE7" "WEBREFRESH",
	"\xE5" "WEBSEARCH",
	"\xE8" "WEBSTOP",
	"\x2D" "X",
	"\x15" "Y",			
	"\x7D" "YEN",
	"\x2C" "Z",	
	"\x00" "\0"
};

static unsigned char Ex_ini_do_dik_lookup(const char *in)
{
	static int array_size = 0;

	if(!array_size) while(*Ex_ini_dik_lookup[++array_size]);
		
	if(!in||!*in) return 0;

	int first = 0, last = array_size-1, stop = -1;

	int x;
	for(x=(last+first)/2;x!=stop;x=(last+first)/2)
	{
		//out = *Ex_ini_dik_lookup[x];		
		int cmp = strcmp(Ex_ini_dik_lookup[x]+1,in);
		if(cmp>0) stop = last = x; 						
		else if(cmp<0) stop = first = x; 								
		else return *Ex_ini_dik_lookup[x];
	}

	if(last==array_size-1&&x==last-1 //round-off error	
	&&!strcmp(Ex_ini_dik_lookup[last]+1,in))
	return *Ex_ini_dik_lookup[last]; return 0; //catch all
}

template<typename T>
static int Ex_ini_do_safe_template_cmp(const T*,const T*); //pure specialization

template<>
static inline int Ex_ini_do_safe_template_cmp(const wchar_t *a, const wchar_t *b)
{
	return wcscmp(a,b);
}
template<>
static inline int Ex_ini_do_safe_template_cmp(const char *a, const char *b)
{
	return strcmp(a,b);
}

template<typename T>
static inline void Ex_ini_do_lookup_quicksort(const T **id, int sz, int lo, int hi) 
{
	if(lo>=hi) return; const T *swap = 0; //hack: slow swapping
    	
	int r = rand(); r = lo<hi?lo+r%(hi-lo):hi+r%(lo-hi);
	swap = id[lo*2]; id[lo*2] = id[r*2]; id[r*2] = swap;  
	swap = id[lo*2+1]; id[lo*2+1] = id[r*2+1]; id[r*2+1] = swap;

	int pivot = lo; const T *p = id[pivot*2+1];		
	for(int unknown=pivot+1;unknown<=hi;unknown++)
	if(Ex_ini_do_safe_template_cmp(id[unknown*2+1],p)<0)
	{   		
		pivot++; swap = id[unknown*2]; id[unknown*2] = id[pivot*2]; id[pivot*2] = swap;	 
		swap = id[unknown*2+1]; id[unknown*2+1] = id[pivot*2+1]; id[pivot*2+1] = swap;
	}

	swap = id[lo*2]; id[lo*2] = id[pivot*2]; id[pivot*2] = swap; 
	swap = id[lo*2+1]; id[lo*2+1] = id[pivot*2+1]; id[pivot*2+1] = swap; 

	if(pivot) Ex_ini_do_lookup_quicksort(id,sz,lo,pivot-1); 
	
	if(pivot<=sz) Ex_ini_do_lookup_quicksort(id,sz,pivot+1,hi);
}

template<typename T> //sorting of id lookup tables
static void Ex_ini_do_lookup_quicksort(const T **id, int sz)
{
	Ex_ini_do_lookup_quicksort(id,sz,0,sz-1); 
}

template<typename T>
static int Ex_ini_do_binary_lookup(const T *in, const T *in_x, const T **id, int sz)
{	
	if(!id||!sz) return 0;

	//takes an array of type T of interleaved id
	//and null terminated type T strings of sz*2

	int in_sz = sizeof(T)*(in_x-in);	
	int out = 0; if(!in||!*in) return 0;
	int x,cmp, first = 0, last = sz-1, stop = -1;
	for(x=(last+first)/2;x!=stop;x=(last+first)/2)
	{
		out = (int)id[x*2];	  		
		cmp = memcmp(id[x*2+1],in,in_sz); if(!cmp) 
		{
			if(id[x*2+1][in_sz/sizeof(T)]=='\0') 
			return out; 
			cmp = 1;
		}
		stop = (cmp>0?last:first) = x; 
	}
	if(last==sz-1&&x==last-1) //round-off error
	{
		if(!memcmp(id[last*2+1],in,in_sz))
		if(id[last*2+1][in_sz/sizeof(T)]=='\0')
		return out = (int)id[last*2];
	}	
	return 0; //catch all
}

static bool Ex_ini_xlat_bin_val(const wchar_t *in, bool def)
{
	if(in) switch(wcslen(in))
	{
	case 0: return 1; //sans equal sign
			break; //2021
	case 1: if(*in=='1'||*in=='y') return 1;
			if(*in=='0'||*in=='n') return 0;
			break; //2021
	case 2: if(in[0]=='o'&&in[1]=='k') return 1;			
			if(in[0]=='n'&&in[1]=='o') return 0;			
	//		if(in[0]=='i'&&in[1]=='s') return 0; //???
			break; //2021
	case 3: if(in[0]=='y'&&in[1]=='e'&&in[2]=='s') return 1;
	//		if(in[0]=='n'&&in[1]=='o'&&in[2]=='t') return 0; //???
			break; //2021
			//2021: programmer's habit
	case 4: if(!wmemcmp(in,L"true",4)) return 1; break;
	case 5: if(!wmemcmp(in,L"false",5)) return 0; break;
	}
	return def; //default
}

static int Ex_ini_xlat_int_val(const wchar_t *in, int def)
{
	wchar_t *p = 0;
	int out = in?wcstol(in,&p,10):def; 
	return p&&p>in?out:def;
}

static int Ex_ini_xlat_hex_val(const wchar_t *in, int def)
{
	if(in) for(int i=0,ini;ini=tolower(in[i]);i++)
	{
		if(ini>='0'&&ini<='9'||ini>='a'&&ini<='f')
		continue; else return def;
	}

	wchar_t *p = 0; int out = in?wcstol(in,&p,16):def; 

	return p&&p>in?out:def;
}

static float Ex_ini_xlat_dec_val(const wchar_t *in, float def)
{
	wchar_t *p = 0;	
	float out = in?wcstod(in,&p):def; //hack
	return p&&p>in?out:def;
}

static int Ex_ini_xlat_4cc_val(const wchar_t *in, int def)
{
	int i,out = 0;
	for(i=0;i<4&&in[i];i++)
	{
		if(i<0||i>127) return def;

		out|=int(in[i])<<(3-i)*8;
	}
	return out>>=(4-i)*8;
}

static DWORD Ex_ini_xlat_col_val(const wchar_t *in, DWORD def)
{
	int len = wcsnlen(in,9); 
	
	if(len<=0||len>8) return def;

	int hex[8]; 

	for(int i=0;i<len;i++)
	{
		int ini = tolower(in[i]);

		if(ini>='0'&&ini<='9')
		{
			hex[i] = ini-'0'; continue;
		}
		else if(ini>='a'&&ini<='f')
		{
			hex[i] = 10+ini-'a'; continue;
		}
		else return def; //not hexadecimal
	}

	//taken from cmd.exe colors
	static const DWORD brushes[16] = 
	{
		0xFF000000, //black
		0xFF000080,	//navy blue
		0xFF008000, 
		0xFF008080,
		0xFF800000,
		0xFF800080,
		0xFF808000,
		0xFFC0C0C0, //lite grey
		0xFF808080, //dark grey
		0xFF0000FF,	//true blue
		0xFF00FF00,
		0xFF00FFFF,
		0xFFFF0000,
		0xFFFF00FF,
		0xFFFFFF00,
		0xFFFFFFFF, //white
	};

	switch(len)
	{
	case 1: //brush

		return brushes[hex[0]];

	case 2: //4bit alpha component + brush

		return hex[0]*17<<24|0x00FFFFFF&brushes[hex[1]];

	case 3: //4bit rgb value

		return 255<<24|hex[0]*17<<16|hex[1]*17<<8|hex[2]*17;

	case 4: //4bit alpha component + 4bit rgb value

		return hex[0]*17<<24|hex[1]*17<<16|hex[2]*17<<8|hex[3]*17;

	case 6: //8bit rgb value

		return 255<<24|hex[0]*16+hex[1]<<16|hex[2]*16+hex[3]<<8|hex[4]*16+hex[5];

	case 8: //8bit alpha component + 8bit rgb value

		return hex[0]*16+hex[1]<<24|hex[2]*16+hex[3]<<16|hex[4]*16+hex[5]<<8|hex[6]*16+hex[7];
	}

	return def;
}

static unsigned char Ex_ini_xlat_dik_val(const wchar_t *in, int def) //action to key
{					
	unsigned char out = def<256?def:0;
	
	const unsigned char *pv = Ex_ini_action_derive_macro(in);	

	if(*pv==1&&pv[2]=='\0'&&pv[3]=='\0') //single key macro
	out = pv[1]; 
	
	return out;
}

static EX_INI_ACT Ex_ini_xlat_act_val(const wchar_t *in, int def)
{
	EX_INI_ACT out = Ex_ini_action_assign_macro(in);

	for(int c=0;c<EX::contexts-1;c++)
	if(out[c]==EX_INI_BAD_MACRO) out[c] = def; //best practice??	

	return out;
}
