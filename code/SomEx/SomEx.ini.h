#ifndef SOM_EX_INI_INCLUDED
#define SOM_EX_INI_INCLUDED

#include <vector>
#include <map>

//REMOVE ME?
//alphafog_shadow_thickness
#define EX_INI_SHADOWVOLUME 0.2f
#define EX_INI_SHADOWRADIUS 0.512f
#define EX_INI_SHADOWUVBIAS 0.984375f

//REMOVE ME?
//#define EX_INI_STEREO_MODE 2
#define EX_INI_MENUDISTANCE 0.5f //reciprocal

//these define partitions in the 
//depth buffer. they seem to be
//independent of the far clipping
//plane. 0.001 may be squooshed
//KF2's compass uses it alongside
//menus. it may need increasing
//TODO? MAYBE THEY SHOULD BE EQUAL
//SIZED SO THE MENUS CAN USE 1?
#define EX_INI_Z_SLICE_1 0.001f
#define EX_INI_Z_SLICE_2 0.02f

#define EX_INI_NICE_JUMP 0.02f //BLACK MAGIC
					 
#define EPS	0.00001
#define PI 3.141592f  
#define INF	16777215 //i.e. ffffff
#include "Ex.number.h"
#define EX_INI_NUMBER(min,NaN,max) EX::Number\
<int(min),int((min-int(min))*100000)/*minimum*/\
,int(NaN),int((NaN-int(NaN))*100000)/*default*/\
,int(max),int((max-int(max))*100000)/*maximum*/>
#define EX_INI_SERIES(min,NaN,max) EX::Numbers\
<sizeof(NaN)/sizeof(float) /*length of series*/\
,int(min),int((min-int(min))*100000)/*minimum*/\
,NaN                      /*array of defaults*/\
,int(max),int((max-int(max))*100000)/*maximum*/>

#define EX_INI_MACRO unsigned short

#define EX_INI_BAD_MACRO ((unsigned short)0xFFFF)

#define EX_INI_BUTTON2KEY(...) EX::INI::BUTTON2KEY(__VA_ARGS__)
#define EX_INI_BUTTON2MACRO(...) EX::INI::BUTTON2MACRO(__VA_ARGS__)

#define EX_INI_CONTEXTUAL EX::INI::contextual_t

#define EX_INI_NONCONTEXT(T) EX::INI::context_less(T) //REMOVE ME?

#define EX_INI_ACT EX_INI_CONTEXTUAL<EX_INI_MACRO> //REMOVE ME?

#ifndef EX_INI_CPP
#define EX_INI_CPP(...)
#endif

namespace EX{
namespace INI
{
	//preload every section
	extern void initialize(); 
	
	struct Section
	{
		int	times_included;	
		int total_failures, total_mentions;

		static const int passes = 1; int pass;

		Section() : pass(0), pdata(0) //NEW
		{
			times_included = 0;			
			total_failures = 0;
			total_mentions = -1;
		}
		~Section(){ delete pdata; }
		
		struct private_data
		{
			virtual ~private_data(){}; //???

		}*pdata; //NEW: dynamic internals

		void defaults(){} //hack: Bugfix

		//2024
		std::map<std::wstring,std::wstring> storage;
		void store_mention(const wchar_t *p, const wchar_t *d, const wchar_t *q);
		virtual const wchar_t *get(const char *lv);
	};

	template<typename T> struct contextual_t
	{
		T set[EX::contexts];

		contextual_t<T>()
		{
			for(size_t i=0;i<EX::contexts;i++) set[i] = 0;
		}
		inline operator bool()const
		{
			for(size_t i=0;i<EX::contexts-1;i++) if(set[i]) return true;

			return false;
		}
		inline T operator[](size_t i)const
		{
			return set[i<EX::contexts?i:EX::contexts-1]; //0
		}
		inline T &operator[](size_t i)
		{
			return set[i<EX::contexts?i:EX::contexts-1]; //0
		}
	};
	template<typename T> //temporary measure
	inline contextual_t<T> context_less(T t) //REMOVE ME?
	{			
		contextual_t<T> out;
		
		for(size_t i=0;i<EX::contexts-1;i++) out.set[i] = t; //-1
		
		//out.set[EX::contexts-1] = 0;
		
		return out;
	}

	struct action_reaction_pair_t
	{
		unsigned int bits;

		inline operator EX_INI_MACRO()
		{
			return bits&0xFFFF;
		} 
		inline EX_INI_MACRO operator+() 
		{
			return bits&0xFFFF;
		}
		inline EX_INI_MACRO operator-() 
		{
			return bits>>16;
		}
	};
		
	static action_reaction_pair_t BUTTON2MACRO
	(EX_INI_CONTEXTUAL<int> *buttons, int i, int context=-1)
	{
		int c = context>-1?context:EX::context(); 

		action_reaction_pair_t out = 
		{
			i&&buttons&&abs(i)<=buttons[0][c]?(unsigned)buttons[abs(i)][c]:0u
		};
		
		if(i<0) out.bits = out.bits>>16|out.bits<<16;
		
		return out;
	}
	static action_reaction_pair_t BUTTON2MACRO
	(EX_INI_ACT *buttons, int i, int context=-1)
	{
		int c = context>-1?context:EX::context(); 

		action_reaction_pair_t out = 
		{
			i>0&&buttons&&i<=buttons[0][c]?(unsigned)buttons[i][c]:0u
		};
		
		return out;
	}
	 	
	typedef unsigned char escape_analog_modes_t;

	struct escape_analog_mode //escape_analog_modes_t
	{
		escape_analog_modes_t inplay:1,invert:1,motion:3,axisid:3;
		escape_analog_mode(){ *(escape_analog_modes_t*)this = 0; }
	};
	
	struct volume_texture
	{
		int group, file_name_s;

		const wchar_t *file_name;
		const wchar_t *event_name;
		
		bool match(const char *ASCII, int len)const
		{
			if(len==file_name_s) while(len-->0)			
			{
				int a = ASCII[len], b = file_name[len];
				if(a!=b&&b!='_') return false;
			}
			else return false; return true;
		}
		 		
		bool operator<(const volume_texture &cmp)const
		{
			if(group!=cmp.group) 
			return group<cmp.group;
			return *event_name&&!*cmp.event_name; 
		}
	};

	struct bitmap
	{
		int index, file_name_s;

		const wchar_t *file_name;
		const wchar_t *event_name;
				
		int width,height;

		const unsigned char *pixels;

		bitmap(){ memset(this,0x00,sizeof(*this)); }

		bool operator<(const bitmap &cmp)const
		{
			return index<cmp.index;
		}
		bool operator<(int cmp)const{ return index<cmp; }
		bool operator==(int cmp)const{ return index==cmp; } //MSVC?
	};

	template<typename T, int int_memset, size_t N=1> struct constructionset_t
	{
		bool operator&()const{ int compile[-1]; } 
		//prevent EX::Number operators & and <<=
		template<typename T> void operator<<=(const T &t){ int compile[-1]; }

		T set[N]; constructionset_t(){ for(int i=0;i<N;i++) fp(set[i]=int_memset); }

		void fp(float &dec){ dec/=100000; } void fp(...){}

		inline operator const T&()const{ return set[0]; }		
		inline operator const T*()const{ return set; }

		inline T &operator=(const T &rv){ return set[0] = rv; }		
		inline operator T&(){ return set[0]; }
		inline operator T*(){ return set; }	  

		enum{ enum_memset=int_memset };
	};			 
	//printf: you gotta do what you gotta do
	template<typename T, int int_memset, size_t N> struct constructionset_f
	{	
		typedef T value_type; enum{ string_length=N-1 }; //2020

		/*what we'd really like is a compiler warning if*/
		/*and when these things are passed via a ... arg*/

		bool operator&()const{ int compile[-1]; } 
		//prevent EX::Number operators & and <<=
		template<typename T> void operator<<=(const T &t){ int compile[-1]; }

		typedef constructionset_t<T,int_memset,N> F;   

		F *f; struct align{ F f, *g; }; //important
																   
		constructionset_f(){ f = (F*)((char*)this-(int)&((align*)0)->g); } 

		inline operator const T&()const{ return *f; }		
		inline operator const T*()const{ return *f; }

		inline T &operator=(const T &rv){ return *f = rv; }		
		inline operator T&(){ return *f; }
		inline operator T*(){ return *f; }	  
	};

#define F(f,c) f##c
#define G(c) F(f,c)
//these are just doing EX_INI_NUMBER like initialization
#define EX_INI_MACROS(...) EX_INI_ACT //assuming ... is 0
#define EX_INI_BUTTON(val) EX::INI::constructionset_t<int,val>
#define EX_INI_OPTION(val) EX::INI::constructionset_t<bool,val>
#define EX_INI_FOURCC(val) EX::INI::constructionset_t<DWORD,val>
#define EX_INI_FIGURE(val) EX::INI::constructionset_t<float,int(val*100000)>
#define EX_INI_BUFFER(ptr) EX::INI::constructionset_t<const ptr*,0,1>
#define EX_INI_PRINTF(t,n) EX::INI::constructionset_t<t,0,n?n:MAX_PATH>\
G(__LINE__/*__COUNTER__*/); EX::INI::constructionset_f<t,0,n?n:MAX_PATH>
								
extern const float GAITS[7]
EX_INI_CPP(={.2,.333333,.5,.666666,.75,.875,.95}); 
extern const float SHAPE[5]
EX_INI_CPP(={-1,.0,.0,.0,.0}); 
extern const float ZERO3[3] EX_INI_CPP(={0,0,0}); 

//C++: taking advantage of template's evaluation
template<class Section> inline void _reset(Section *t)
{
	if(t){ t->Section::~Section(); new(t)Section(); }		
}	
template<class Section> inline int _mentions(Section *t)
{
	return t?t->total_mentions:0;
}		
template<class Section> inline int _pass(Section *t, int n=0)
{
	if(!t) return 0; if(n) t->pass = n; return t->pass; 
}
//todo: a template base class to shorten this macro
#define EX_INI_SECTION(ns) struct ns##__Section; struct ns\
{\
	void operator+=(const wchar_t *const *envp);\
	/*caution: passing 0 defers to ns(Section*)*/\
	typedef ns##__Section Section; ns(int section=1);\
	inline operator bool(){	return _mentions(_section)>0; }\
	inline const Section *operator->(){ return _section; }\
	inline bool operator==(ns nc){ return _section==nc._section; }\
	inline bool operator!=(ns nc){ return _section!=nc._section; }\
	inline void operator=(Section *section){ _section = section; }\
	inline void operator=(int s){ _section = 0; if(s) new(this) ns(s); }\
	inline void operator=(const wchar_t *const *e){ _reset(_section); *this+=e; }\
	inline ns operator[](int n){ _pass(_section,n); return *this; }\
	ns(Section *section){ _section = section; }\
	/*private:*/ Section *_section;\
	/*scheduled obsolete: called repeatedly by operator+=*/\
	void mention(const wchar_t*,const wchar_t*,const wchar_t*);\
};\
struct ns##__Section : public Section //...

EX_INI_SECTION(Action)
{		
	inline EX_INI_MACRO translate(const wchar_t *in)const
	{
		return translate(in,(wchar_t*)0);
	}
	inline EX_INI_MACRO translate(const wchar_t *in, size_t x)const
	{
		return translate(in,in+x);
	}

	EX_INI_MACRO translate(const wchar_t*, const wchar_t*)const;
	
	//// TODO: move this stuff to EX::Macro (Ex.macro.h) ////
	
	EX_INI_MACRO translate(const char*,...)const; //REMOVE ME?

	const unsigned char *xtranslate(EX_INI_MACRO)const;
	const unsigned char *xtranslate(const wchar_t*, const wchar_t*)const; 
	inline const unsigned char *xtranslate(const wchar_t *in, int x)const
	{
		return xtranslate(in,in+x);
	}
	inline const unsigned char *xtranslate(const wchar_t *in)const
	{
		return xtranslate(in,in+wcslen(in));
	}
};
EX_INI_SECTION(Adjust)
{	
	EX_INI_NUMBER(-INF,-10,0) abyss_depth;
	
	EX_INI_NUMBER(0,0,INF) character_identifier;

	//IMPLEMENT? REMOVE ME??
	//EX_INI_NUMBER(0,0,INF) fov_ambient_saturation;
	///////////////////////
	EX_INI_NUMBER(0,0,INF)
	fov_fogline_saturation,
	fov_fogline_adjustment, fov_fogline_adjustment2,
	fov_skyline_adjustment, fov_skyline_adjustment2;		
	//EX_INI_NUMBER(0.025,0.025,1) 
	EX_INI_NUMBER(0.0,0.025,1) 
	fov_sky_and_fog_minima, fov_sky_and_fog_minima2;
	EX_INI_NUMBER(0,1,INF) 
	fov_sky_and_fog_powers, fov_sky_and_fog_powers2,
	fov_sky_and_fog_powers3,fov_sky_and_fog_powers4; //DOCUMENT US
	//REMOVE ME?
	EX_INI_NUMBER(0,1.2,INF) fov_frustum_multiplier;			

	EX_INI_NUMBER(0,0,4) sky_model_identifier; //DOCUMENT ME

	//would like to use SomEx_npc with these, except
	//it's unknown what shadows belong to which NPCs
	EX_INI_NUMBER( 0, 1,  2) npc_shadows_multiplier;
	EX_INI_NUMBER( 0, 1,  1) npc_shadows_modulation;
	EX_INI_NUMBER( 0, 0,INF) npc_shadows_saturation;

	//DOCUMENT ME
	//this defines a flat plane like a water surface
	//the PRF files now have an "envelope" value for
	//controlling vertical movement. if the value is
	//negative it will hold the NPC below this value
	//animations can still jump over it but it won't
	//try to "swim" above if it's a fish. it will be
	//stranded in that case, vice versa for positive
	EX_INI_NUMBER(-INF,0,INF) npc_xz_plane_distance;
	
	//this decouples the hitboxes from the
	//clip shape. 0.6 is a pretty good fit
	//
	// NOTE: 0.5 is added to the player's weapon's
	// PRF range (length?) I don't know about the
	// monsters (yet)
	//
	//REMINDER: corresponds to player_character_shape3
	EX_INI_NUMBER(-INF,0.5,INF) npc_hitbox_clearance; //DOCUMENT ME	

	//DOCUMENT ME
	//2021: analogous to player_character_fence
	//NOTE: the classic value is 0.5
	//EX_INI_NUMBER(0,0.3,INF) npc_fence;
	EX_INI_NUMBER(0,0.25,INF) npc_fence;

	EX_INI_FOURCC(0xffff0000) red_flash_tint;
	EX_INI_FOURCC(0xff0000ff) blue_flash_tint;
	EX_INI_FOURCC(0xff00ff00) green_flash_tint;
	EX_INI_FOURCC(0x80000000) blackened_tint;
	EX_INI_FOURCC(0x64000000) blackened_tint2;
	EX_INI_FOURCC(0xffdcdcdc) lettering_tint;
	EX_INI_FOURCC(0xff464646) lettering_contrast;	
	EX_INI_FOURCC(0xffffffff) start_tint;

	EX_INI_NUMBER(0,0,1) start_blackout_factor;
	EX_INI_NUMBER(0,0.5,1) paneling_opacity_adjustment;	
	EX_INI_NUMBER(0.06,1,1) highlight_opacity_adjustment;
	EX_INI_NUMBER(0,1/3.0,1) hud_gauge_opacity_adjustment;

	EX_INI_FOURCC(0) hud_gauge_border; //DOCUMENT ME

	//todo: hud_gauge_depleted_half_tint2
	EX_INI_FOURCC(0x55808080) hud_gauge_depleted_half_tint;	

	//DOCUMENT US
	//(NOTE: paneling_frame_translucency isn't a Number)
	EX_INI_FIGURE(0.0) 
	paneling_frame_translucency, hud_frame_translucency,
	paneling_sword_translucency, hud_arrow_translucency,
	hud_label_translucency;
};
EX_INI_SECTION(Analog)
{
	EX_INI_NUMBER(0,   8,INF) error_impedance;
	EX_INI_NUMBER(0,   4,INF) error_impedance2;	
	EX_INI_NUMBER(0,0.02,INF) error_allowance;
	EX_INI_NUMBER(0,0.04,INF) error_allowance2;
	EX_INI_NUMBER(0,   0,INF) error_tolerance;
	EX_INI_NUMBER(0,   0,INF) error_clearance;
	EX_INI_NUMBER(0,   0,INF) error_parachute; //OBSOLETE?
};
EX_INI_SECTION(Author)
{
	EX_INI_PRINTF(wchar_t,0)
	international_production_title,
	international_translation_title;

	EX_INI_BUFFER(wchar_t) _info_minus_1,
	software_company_name,
	the_authority_to_contact,
	email_address_of_contact,
	street_address_of_contact,
	phone_number_of_contact,
	hours_in_which_to_phone,
	//2022: "homepage" can be used instead
	online_website_to_visit, _info_too_far; 

	inline const wchar_t *info(size_t i)const
	{
		if(&(const wchar_t*&)_info_minus_1+i>=_info_too_far) return 0;

		return *(&(const wchar_t*&)_info_minus_1+1+i);
	}
	//REMOVE ME?
	inline const wchar_t* &info(size_t i)
	{
		if(&(const wchar_t*&)_info_minus_1+i>=_info_too_far) 
		{
			return _info_too_far = 0;
		}
		const wchar_t* &out = *(&(const wchar_t*&)_info_minus_1+1+i);

		return out&&*out?out:out=L"";
	}
};
EX_INI_SECTION(Bitmap)
{
	//WORK IN PROGRESS

	//0_00 = West Melanat
	std::vector<EX::INI::bitmap> bitmaps;

	EX_INI_NUMBER(0,0,INF) bitmap_ambient;
	EX_INI_NUMBER(0,0,INF) bitmap_ambient_mask;
	EX_INI_NUMBER(-INF,0,INF) bitmap_ambient_saturation; //SKETCHY
};
EX_INI_SECTION(Boxart)
{
	EX_INI_BUFFER(wchar_t) _info_minus_1,
	game_series,
	game_sub_series,
	game_presented_by,
	game_in_association_with,
	game_super_title,
	game_title_with_sequel,
	game_subtitle,	
	game_based_upon,
	game_edition,
	game_version,
	game_project,	
	game_original,	
	game_year_published,
	game_publishing_date,	
	game_first_edition_year,
	game_first_edition_date,
	game_legal,
	game_auteur,
	game_author, //2020 (see also Author section)
	game_designer,
	game_developer, //2020 (see also Author section)
	game_director,
	game_concept,
	game_planner,
	game_artist,
	game_writer,
	game_music,
	game_soundtrack,
	game_soundsuite,
	game_programmer,
	game_caretaker,
	game_mascot,
	game_musing,
	game_presenters,
	game_producer,
	game_company,
	game_studio,
	game_subteam,
	game_partners,
	game_label,
	game_publisher,
	game_distributor,
	game_rating,
	game_region,
	game_format,
	game_license,	
	game_features,	
	game_disclaimer,	
	game_suggested_retail_price,
	game_thanks,
	game_dedication, _info_too_far;

	inline const wchar_t *info(size_t i)const
	{
		if(&(const wchar_t*&)_info_minus_1+i>=_info_too_far) return 0;

		return *(&(const wchar_t*&)_info_minus_1+1+i);
	}
	//REMOVE ME?
	inline const wchar_t* &info(size_t i)
	{
		if(&(const wchar_t*&)_info_minus_1+i>=_info_too_far) 
		{
			return _info_too_far = 0;
		}
		const wchar_t* &out = *(&(const wchar_t*&)_info_minus_1+1+i);

		return out&&*out?out:out=L"";
	}
};
EX_INI_SECTION(Bugfix)
{
	//EX_INI_OPTION(0) 
	EX_INI_OPTION(1) //1.2.3.4
	//do_fix_any_trivial
	do_fix_controller_configuration,
	do_fix_spelling_of_english_words,
	do_fix_out_of_range_config_values,
	do_fix_elapsed_time,
	do_fix_fov_in_memory,
	do_fix_widescreen_font_height,
	do_fix_boxed_events,
	do_fix_damage_calculus,
	do_fix_automap_graphic,	  	
	//do_fix_clipping_player_character, //1.2.1.8
	do_fix_zbuffer_abuse,
	do_fix_trip_zone_range; //2022 BREAKING CHANGE
	inline void do_fix_any_trivial(bool yes=true)
	{
		for(bool*p=(bool*)do_fix_controller_configuration;
		p<=(bool*)do_fix_trip_zone_range;*p++=yes);
	}

	//EX_INI_OPTION(0) 
	EX_INI_OPTION(1) //1.2.3.4
	//do_fix_any_nontrivial
	do_fix_oversized_compass_needle,
	do_fix_clipping_item_display,
	do_fix_lighting_dropout, 
	do_fix_asynchronous_sound, 	
	do_try_to_fix_exit_from_menus; //temporary 	
	inline void do_fix_any_nontrivial(bool yes=true)
	{
		for(bool*p=(bool*)do_fix_oversized_compass_needle;
		p<=(bool*)do_try_to_fix_exit_from_menus;*p++=yes);
	}

	EX_INI_OPTION(1) 
	//do_fix_any_vital
	do_fix_asynchronous_input, 
	do_fix_lifetime_of_gdi_objects, 
	do_fix_slowdown_on_save_data_select; 
	inline void do_fix_any_vital(bool yes=true)
	{
		for(bool*p=(bool*)do_fix_asynchronous_input;
		p<=(bool*)do_fix_slowdown_on_save_data_select;*p++=yes);
	}
		
	EX_INI_OPTION(1) 
	//do_fix_any_experimental //REMOVE ME
	//do_fix_frustum_in_memory //obsolete
//	do_fix_animation_sample_rate, //preliminary //DOCUMENT ME //below
	//TODO
	// som_hacks_CreateSurface4 needs to support this
	//
	do_fix_colorkey; //Moratheia //DOCUMENT ME

	void defaults()
	{
		do_fix_any_trivial();
		do_fix_any_nontrivial();
	}

	union dfasr //som.files.cpp
	{
		unsigned bits;

		struct
		{
			unsigned fix:1;
			unsigned enemy:1;
			unsigned npc:1;
			unsigned obj:1;
			unsigned magic:1;
			unsigned item:1;
			unsigned arm:1;
			unsigned sfx:1;
		};

		inline operator bool()const{ return fix; }
	};
	static dfasr do_fix_animation_sample_rate;
};
EX_INI_SECTION(Button)
{		
	enum{ buttons_s = 20 };

	//it looks like buttons[0] is unused
	EX_INI_MACROS(0) buttons[buttons_s];

	static int pseudo_button(int angle)
	{
		switch(angle)
		{
		case 0:	    return 0;
		case 9000:  return 17;
		case 18000:	return 18;
		case 27000: return 19;
		}
		return angle;
	}
};
EX_INI_SECTION(Damage)
{
	EX_INI_FOURCC(0)
	hit_point_mode, //2: Magic
	hit_point_model; //'void'+1

	EX_INI_NUMBER(0,0,INF)
	hit_point_quantifier, 
	hit_point_quantifier2,
	hit_offset_quantifier,
	hit_offset_quantifier2,
	hit_outcome_quantifier,
	hit_handicap_quantifier,
	hit_penalty_quantifier;

	EX_INI_OPTION(0)
	do_not_harm_defenseless_characters,
	do_not_harm_equipment; //UNDOCUMENTED
	
	//DOCUMENT ME
	//2020: want to cover animation #22 and new do_red
	//setup
	//NOTE: 1/3 is used internally only
	//
	// REMINDER: 1/5 is the classic value chosen by SOM's designers
	// I don't know... this system was only discovered in 2020... I
	// chose 1/3 then because that's what I decided best for do_red
	// as I recall, up to then it had used red_multiplier times 1/2 
	//
	/*0.3 is approximating 0.2916 halfway between 1/4 and 1/3
	//because I feel like it shouldn't be exactly 3 criticals
	//to kill but still somewhere in that range
	EX_INI_NUMBER(0,1/3.0f,65535) critical_hit_point_quantifier;*/
	EX_INI_NUMBER(0,0.3f,65535) critical_hit_point_quantifier;
};
EX_INI_SECTION(Detail)
{
	enum{ aa_supersample_anisotropy=4 };

	//REMOVE ME?
	EX_INI_NUMBER(0,0,INF) log_initial_timeout;
	EX_INI_NUMBER(0,0,INF) log_subsequent_timeout;
	
	enum{ escape_analog_modes_s=8 };
	 	
	int escape_analog_modesN;
	EX::INI::escape_analog_mode
	escape_analog_modes[escape_analog_modes_s][8]; 

	EX_INI_SERIES(0,GAITS,1) escape_analog_gaits;
	EX_INI_SERIES(-1,SHAPE,1) escape_analog_shape; //DOCUMENT ME
		   
	EX_INI_OPTION(0) numlock_status;

	EX_INI_NUMBER(0,0,INF) cursor_hourglass_ms_timeout;

	EX_INI_FOURCC(0) wasd_and_mouse_mode;

	//REMOVE ME?
	EX_INI_ACT *mouse_button_actions; //8...
	//
	EX_INI_MACROS(0)
	mouse_left_button_action,
	mouse_right_button_action,
	mouse_middle_button_action,
	mouse_4th_button_action,
	mouse_5th_button_action,
	mouse_6th_button_action,
	mouse_7th_button_action,
	mouse_8th_button_action, 
	mouse_tilt_left_action,
	mouse_tilt_right_action,
	mouse_menu_button_action;

	EX_INI_NUMBER(-INF,1,INF) mouse_vertical_multiplier;
	EX_INI_NUMBER(-INF,1,INF) mouse_horizontal_multiplier; 
	EX_INI_NUMBER(   0,1,INF) mouse_saturate_multiplier;
	EX_INI_NUMBER(   0,1,  2) mouse_deadzone_multiplier;	
	EX_INI_NUMBER(   0,0,INF) mouse_deadzone_ms_timeout;
	
	EX_INI_NUMBER(0,0,16) lights_lamps_limit;

	//DOCUMENT US
	EX_INI_FIGURE(0.0) mipmaps_pixel_art_power_of_two; //2020
	EX_INI_FIGURE(1.0) mipmaps_saturate; //2020
	//EXPERIMENTAL
	EX_INI_FOURCC(0) mipmaps_kaiser_sinc_levels; //2021

	//8: arbitrary (edit: 8 had depended on a saturation term)
	//2022: I had to remove a "saturate" step from the vshader
	//because it made the results highly dependent on vertices
	//the new approximate value should be between 1 and 2, I'm
	//just splitting the difference until I have some opinions
	//EX_INI_NUMBER(  0,    8,INF) alphafog_skyflood_constant;
	EX_INI_NUMBER(   0,   1.5,INF) alphafog_skyflood_constant;
	EX_INI_NUMBER(-INF,-0.125,INF) alphafog_skyfloor_constant;	

	//antialiasing of colorkey 
	//2000 is Som2k mode. //0 is the best algorithm
	EX_INI_NUMBER(0,0,2000) alphafog_colorkey;
															 
	//undocumented
	EX_INI_NUMBER(0,  0,INF) lights_constant_attenuation;
	EX_INI_NUMBER(0,0.4,INF) lights_linear_attenuation;
	EX_INI_NUMBER(0,  0,INF) lights_quadratic_attenuation;

	EX_INI_NUMBER(0,1,INF) 
	lights_exponent_multiplier,
	lights_distance_multiplier,
	lights_diffuse_multiplier,
	lights_ambient_multiplier;

	EX_INI_NUMBER(0,0.15,1) ambient_contribution;

	/*REMOVE US? (RESTORE US?)
	//
	// these controlled do_red but since discovering
	// animation #22 I feel like it should be based
	// on its threshold (critical_hit_point_quantifier)
	//
	//DOCUMENT ME?
	//EX_INI_NUMBER(0,0,INF) red_adjustment; //newer
	//2020: no hit animation at 49% just feels wrong
	//EX_INI_NUMBER(0,1,INF) red_multiplier; //older
	EX_INI_NUMBER(0,1.5,INF) red_multiplier; //older //2020*/

	//DOCUMENT ME?
	//I don't know if this is best, but it
	//seems som.shader.cpp should do this
	//effect later in the pixel shader 
	EX_INI_NUMBER(0,0,INF) red_saturation;

	EX_INI_NUMBER(0,0,2) st_status_mode;
	EX_INI_NUMBER(0,1,INF) st_margin_multiplier;

	EX_INI_NUMBER(0,2,4) start_mode; //KF2

	EX_INI_NUMBER(0,9.8,INF) g; //gravity

	//2020: I think I want to incorporate the
	//stealth gaits into swordplay somehow...
	//jogging is useful for approaching and 
	//moving sideways is moves rather quickly
	//maybe it can be like a lock-on/crabwalk
	EX_INI_NUMBER(  0,5,7) dash_running_gait;
	EX_INI_NUMBER(  0,4,7) dash_jogging_gait; //3	
	EX_INI_NUMBER(-16,1,7) dash_stealth_gait; //0 //DOCUMENT ME	

	//for a long time 1.5 was in use
	//the goal is to slow the lowest
	//gait down as much as it can be
	EX_INI_NUMBER(1,1.75,2) u2_power;
	EX_INI_NUMBER(0,0,1) u2_hardness; //UNUSED

	//DGAMMA
	EX_INI_FOURCC(0) gamma_y_stage; //DOCUMENT US
	EX_INI_BUFFER(wchar_t) gamma_y;
	EX_INI_BUFFER(wchar_t) gamma_n; //DOCUMENT US?
	EX_INI_NUMBER(0,0,1) gamma_npc;

	//NOTE: this is not a color, it's light
	//levels for the opaque and translucent
	//materials plus absolute opacity level
	EX_INI_NUMBER(0,0,INF) nwse_saturation; //DOCUMENT ME

	EX_INI_FIGURE(0.75) tobii_eye_head_ratio; //DOCUMENT ME
};
EX_INI_SECTION(Editor)
{
	EX_INI_OPTION(0) 
	do_hide_direct3d_hardware,
	do_overwrite_project,
	do_overwrite_runtime,
	do_subdivide_polygons,	
	do_not_dither,
	do_not_stipple,
	do_not_smooth,
	do_not_generate_icons, //UNDOCUMENTED
	do_not_open_with_mm3d; //UNDOCUMENTED
	
	//2020: the tool/SOM_EX.ini file was setting this
	//but it's not useful if that file isn't present
	//EX_INI_NUMBER(0,1,INF) clip_volume_multiplier;
	EX_INI_NUMBER(0,0.45,INF) clip_volume_multiplier;
	EX_INI_NUMBER(1,1,4) texture_subsamples;
	EX_INI_FOURCC(0) tile_view_subdivisions; //DOCUMENT ME

	EX_INI_FIGURE(0.0) tile_elevation_increment; //DOCUMENT ME

	EX_INI_NUMBER(0,0,INF) tile_illumination_bitmap; //DOCUMENT ME

	//0~1 multiplies, -2~0 dims, 1~2 brightens
	EX_INI_FIGURE(0) map_icon_brightness; //UNDOCUMENTED

	EX_INI_FOURCC(0xff000000) default_pixel_value;
	EX_INI_FOURCC(0xff0000be) default_pixel_value2;
	EX_INI_FOURCC(0xff000020) default_pixel_value3;
	
	//2022
	//KF2's gamma function makes this too bright 
	//39,135,167
	EX_INI_FOURCC(0xff2787a7) clip_model_pixel_value; //DOCUMENT ME

	//2018
	//Adding this to restore classic look & feel 
	//for historical investigation & restoration.
	EX_INI_FOURCC(0)
	time_machine_mode,spellchecker_mode; //2000
	EX_INI_NUMBER(0,0,INF) black_saturation; 
	EX_INI_NUMBER(0,0xFFFFFF,INF) white_saturation;	
	
	//I recommend 1.3x for PlayStation VR
	EX_INI_NUMBER(0,0,INF) height_adjustment;

	//DOCUMENT ME (1.2.3.2)
	//128 was the limit imposed by From Software
	EX_INI_FOURCC(512) map_enemies_limit;

	//size_t is number of sort_ indices
	size_t assorted_(const wchar_t**rv=0)const;
	//lv has underscores replaced by spaces
	//rv is a printf format string with a %ls
	//false if size_t is not less than assorted_
	//Note: if equal rv gets deferred to assorted_
	//NEW: separators are indicated by an empty *lv 
	bool sort_(size_t,const wchar_t**lv=0,const wchar_t**rv=0)const;
};
EX_INI_SECTION(Engine) //2024
{
	EX_INI_OPTION(0) do_KingsField25;
};
EX_INI_SECTION(Joypad)
{
	//TODO: recognize GUIDs
	EX_INI_PRINTF(wchar_t,0) joypad_to_use_for_play; 
	EX_INI_FOURCC(1) pov_hat_to_use_for_play;
	EX_INI_OPTION(0) do_not_associate_pov_hat_diagonals;

	//arrays of ints
	EX_INI_BUTTON(0) 
	pseudo_x_axis,
	pseudo_y_axis,
	pseudo_z_axis,
	pseudo_x_axis2,
	pseudo_y_axis2,
	pseudo_z_axis2,
	pseudo_slider,
	pseudo_slider2,
	pseudo_pov_hat_0,
	pseudo_pov_hat_9000,
	pseudo_pov_hat_18000,
	pseudo_pov_hat_27000,
	pseudo_pov_hat_4500,
	pseudo_pov_hat_13500,
	pseudo_pov_hat_22500,
	pseudo_pov_hat_31500;

	//REMINDER: THIS IS FLAWED SINCE PAIRED AXES
	//MIGHT STRADDLE GROUPS OR A SINGLE AXIS CAN
	//BE TACKED ONTO A GROUP
	EX_INI_SERIES(0,GAITS,1) 
	axis_analog_gaits,
	axis2_analog_gaits,
	slider_analog_gaits,
	slider2_analog_gaits;
	inline const float *analog_gaits(int xyz)const
	{
		if(xyz<3) return axis_analog_gaits; 
		if(xyz<6) return axis2_analog_gaits; 
		if(xyz!=7) xyz = 0; //HACK
		return slider2_analog_gaits+xyz;
	}
	
	//REMOVE ME?
	EX_INI_CONTEXTUAL<int> *buttons;
};
/*REMOVE ME?
EX_INI_SECTION(Keygen)
{	
	EX_INI_OPTION(0) 
	do_somdb_keygen_defaults,	
	do_enable_keygen_auditing,
	do_disable_keygen_auditing,
	do_enable_keygen_automation,
	do_enable_keygen_visualization,
	do_enable_keygen_instantiation;
		   
	EX_INI_PRINTF(wchar_t,0) keygen_automatic_filter;
	EX_INI_PRINTF(wchar_t,0) keygen_audit_folder;
	EX_INI_PRINTF(wchar_t,0) keygen_toplevel_subkey_in_registry;
	EX_INI_PRINTF(wchar_t,0) keygen_image_file;
	EX_INI_PRINTF(wchar_t,0) keygen_model_file;

	//unsupported (secondary ini files)
	//EX_INI_PRINTF(wchar_t,0) keygen_project_image_file;
	//EX_INI_PRINTF(wchar_t,0) keygen_project_model_file;
		
	//unimplemented
	//'rt' 'db' 'off' 'on' 'ro' 'r+'
	EX_INI_FOURCC(0) keygen_model_mode; 
	EX_INI_FOURCC(0) keygen_image_mode; 
}*/
EX_INI_SECTION(Keymap)
{	
	typedef unsigned char key;
	void translate(const key *in, key *out, size_t sz=256)const;

	//translation translation
	void xtranslate(unsigned char *inout, size_t sz=256)const;
	void xtranslate(unsigned short *inout, size_t sz=256)const;

	inline unsigned short xtranslate(unsigned short in)const
	{
		xtranslate(&in,1); return in;
	}

	//translation translation translation 
	void xxtranslate(unsigned char *inout, size_t sz=256)const;
	void xxtranslate(unsigned short *inout, size_t sz=256)const;

	inline unsigned short xxtranslate(unsigned short in)const
	{
		xxtranslate(&in,1); return in;
	}
};
EX_INI_SECTION(Keypad)
{
	//Reminder: ideally it would be nice
	//to be able to map macros to a macros

	//TODO: this will have to be context sensitive
	EX_INI_MACRO translate(unsigned short n)const; 

	//REMOVE ME?
	unsigned char turbo_keys[256]; 
};
EX_INI_SECTION(Launch)
{		
	EX_INI_PRINTF(wchar_t,0) 
	launch_title_to_use,
	launch_image_to_use;

	//2024: do_ask_to_attach_debugger
	//can't be put in Output because
	//it's needed before Ex::log is
	//needed which is filled out by
	//Output

	EX_INI_OPTION(0)
	do_ask_to_attach_debugger,
	do_without_the_extension_library;
};
EX_INI_SECTION(Number)
{
	//calls upon EX::numbers

	//1: EX::declaring_number
	//2: EX::assigning_number
	static const int passes = 2;

	static EX::Calculator *c; //2024
};
EX_INI_SECTION(Option)
{
	//2020: EX::INI::initialize() will call this
	//to enable do_aa2, do_smooth, and do_lap if
	//do_aa is enabled
	bool initialize_aa(bool force=false)const;

	EX_INI_OPTION(0)
	
	do_log, //REMOVE ME?
	do_2k, //Som2k emulation mode
	do_disasm,	
	do_pause,
	do_escape,
	do_system,
	do_numlock,
	do_cursor,
	do_wasd,
	do_mouse,
	do_mouse2, //WM_INPUT
	do_mipmaps,
	do_alphafog,
	do_rangefog,
	//do_fog, //virtual
	do_dither,	
	do_dither2, //DOCUMENT ME
	do_aa2,do_aa, //DOCUMENT US
	do_save, //2017
	do_smooth,
	do_invert,	
	do_green,
	do_bpp,do_highcolor,do_opengl, //2022: DOCUMENT ME
	do_anisotropy,
	do_lights,
	do_ambient,	
	do_sounds,
	do_syncbgm,		
	do_hdr,
	do_st,
	do_start,	
	do_walk,
	do_dash,
	do_hit,
	do_hit2,
	do_red, 
	do_g, 
	do_u,do_u2,
	do_stipple,do_stipple2,	
	//turns on do_srgb so old Ex.ini files get
	//the benefit to ghosting reduction and so
	//sRGB doesn't have to be opt-in
	do_lap,
	do_lap2, //2021

	//on the way
	do_ko,
	do_palsy,
	do_slow,

	//DOCUMENT US
	//KF2 color work
	//note: do_gamma had been around for a while
	//in a different form, but never implemented
	//for shaders
	do_gamma,
	do_black,
	//FIX ME: the F1 feature has to force this on
	//unless some work is done to switch textures
	//over to sRGB or vice versa
	do_srgb, //I'm having do_lap turn on for now

	//should do_sounds be a
	//Bugfix extension?
	//TODO: kf2_hud?
	do_kf2,do_nwse,do_reverb, //DOCUMENT US

	do_alphasort, //2023 (UNDOCUMENTED)

	do_tobii2;

	//probably best to NOT document these
	EX_INI_OPTION(1)		
	do_sfx,
	do_sixaxis,
	do_rumble,
	do_tobii;

	//do_shadow, //2023 (UNDOCUMENTED)
	enum{ do_shadow=1 }; //TODO: OpenGL requires GL_RED
};
EX_INI_SECTION(Output)
{
	EX_INI_OPTION(0) 
	do_missing_file_dialog_ok;

	//*
	//atiumdag.dll
	//atidxx32.dll
	//atipblag.dat
	//disable.txt
	//strstr(NvAdminDevice)
	static bool missing_whitelist(const char*); //2022

	EX_INI_FOURCC(0xc0) 
	function_overlay_mask;

	EX_INI_FOURCC(0)
	function_overlay_tint[1+12],
	function_overlay_contrast[1+12],
	function_overlay_eclipse[1+12];

	bool f(int)const;

	EX_INI_OPTION(0) do_f5_triple_buffer_fps; //DOCUMENT ME

	EX_INI_FOURCC(10000) 
	art_action_center_note_ms_timeout; //UNDOCUMENTED

	//log: user handler for custom logging directives
	//during initialization all key/value pairs are passed 
	//return true to short-circuit a pair (ie. take ownership)
	//short-circuited pairs are subsequently available via loglog (below)
	//warning! a non-zero log will disable built in convention
	//note: key argument will be in the form of "<key>=<val>" 
	static bool(*log)(const wchar_t *key, const wchar_t *val);

	//loglog: this is a log of the log directives
	//returns the number of log directives "logged"
	int loglog(int pair=0, const wchar_t **key=0, const wchar_t **val=0)const;
};			  
EX_INI_SECTION(Player)
{		
	EX_INI_OPTION(0) 
	do_not_jump, do_not_dash, 
	do_not_duck, do_not_rush, do_not_corkscrew,
	//DOCUMENT US
	do_not_squat,do_not_slide; //2020
	
	EX_INI_NUMBER(1,2,8) corkscrew_power; 

	//mainly noting this formula for equip-capacity
	//15+2.5 per 50 strength or 0.5 per 10. up to 65
	//EX_INI_NUMBER(0,0,INF) player_character_equip?

	//bob had been 0.075. that seems a bit obnoxious
	//(especially with PlayStation VR)
	//2018: trying different figures
	EX_INI_NUMBER(0, 0.05, INF) player_character_bob;
	EX_INI_NUMBER(0, 0.05, INF) player_character_bob2;
	EX_INI_NUMBER(0, 0,    INF) player_character_bob3; //EXPERIMENTAL
	EX_INI_NUMBER(0, PI/4,PI/2) player_character_nod;
	EX_INI_NUMBER(0, PI/2,PI/2) player_character_nod2;
	//1.2.3.2: NOTE: dip/dip2 are now FOV dependendent
	EX_INI_NUMBER(0, 0.25, INF) player_character_dip;
	EX_INI_NUMBER(0, PI/9,PI/2) player_character_dip2;
	EX_INI_NUMBER(0, 0.15, INF) player_character_duck;
	EX_INI_NUMBER(0,  0.5, INF) player_character_duck2;
	EX_INI_NUMBER(0, 0.75, INF) player_character_duck3;
	//shape3 is equal to shape2-npc_hitbox_clearance
	EX_INI_NUMBER(0,  0.5, INF) player_character_shape;
	EX_INI_NUMBER(0, 0.25, INF) player_character_shape2;
	EX_INI_NUMBER(0,  0.2, INF) player_character_shape3; //DOCUMENT ME
	EX_INI_NUMBER(0,  1.6, INF) player_character_height;
	EX_INI_NUMBER(0, 1.25, INF) player_character_height2;
	EX_INI_NUMBER(0, 1.25, INF) player_character_height3;
	//RULE-OF-THUMB? 50 happens to be the value later 
	//used by SOM_PRM at 100% Strength/Magic
	EX_INI_NUMBER(0,   50, INF) player_character_weight;	
	EX_INI_NUMBER(0,   50, INF) player_character_weight2;	
	//RULE-OF-THUMB? eye-level should be a measure of
	//what can be climbed onto (by jumping?)
	//REMINDER: the classic value is 0.5
	EX_INI_NUMBER(0,  0.3, INF) player_character_fence;	
	EX_INI_NUMBER(0,  1.3, INF) player_character_fence2;	
	EX_INI_NUMBER(0,  2.0, INF) player_character_fence3;
	//2021: after changing som_game_402070_timeGetTime
	//2.3 seemed too much, but I'm reducing it instead
	//in som.mocap.cpp as required... I think probably
	//the feet (Y) should raise up
	EX_INI_NUMBER(0, 2.3, INF) player_character_fence4; //2.28
	EX_INI_NUMBER(0,   0, INF) player_character_fence5; //???
	EX_INI_NUMBER(0,1.75, INF) player_character_radius;
	//2018: 0.75 feels inadequate	
	//EX_INI_NUMBER(0, 0.75, INF) player_character_radius2;
	EX_INI_NUMBER(0,  1.0, INF) player_character_radius2;
	//2019: now controls distance in onscreen item display
	EX_INI_NUMBER(0,    0, INF) player_character_radius3;
	EX_INI_NUMBER(0,    0, INF) player_character_radius4;	
	EX_INI_NUMBER(0,    1, INF) player_character_scale;
	EX_INI_NUMBER(0, 0.45, INF) player_character_shadow; //0.35
	EX_INI_NUMBER(0,  1.5, INF) player_character_stature;
	//2020: now scales _WALK and _DASH
	EX_INI_NUMBER(0,    1, INF) player_character_stride;
	EX_INI_NUMBER(0,  0.5, INF) player_character_inseam;
	EX_INI_NUMBER(0,    3, INF) player_character_slip;	
	//2020: since 1.2.3.2 this is being treated like a
	//boolean on/off. it was treated like exaggeration
	EX_INI_NUMBER(0,    4, INF) player_character_arm;

	//DOCUMENT US
	//som_scene_4412E0_swing expects 300
	//
	// FIX ME: som_game_60fps ASSUMES THIS VALUE IS FIXED
	//
	EX_INI_NUMBER(0,150,300) arm_ms_windup; 
	EX_INI_NUMBER(0,100,500) arm_ms_windup2;
	EX_INI_NUMBER(0.5,0.9,1.25) arm_bicep;
	EX_INI_NUMBER(0.75,1.0,1.25) arm_bicep2;

	//375 is snappy (ninja like even) but 400 is better
	//looking at 60 frames-per-second
	//SOMEWHERE THERE IS a BUG that seems to kick in if
	//there is a fractional component
	EX_INI_NUMBER(250,383/*.33*/,1000) tap_or_hold_ms_timeout;	

	EX_INI_NUMBER(-1000, 300,1000) subtitles_ms_interim;
	EX_INI_NUMBER( 1000,1700,4000) subtitles_ms_timeout;

	//DOCUMENT US (1.2.3.2)
	//save game extension for when SOM isn't installed
	//and players want to associate the extension with
	//their game's EXE
	EX_INI_PRINTF(wchar_t,16) player_file_extension;
	EX_INI_OPTION(0) do_2000_compatible_player_file;
};
EX_INI_SECTION(Sample)
{
	//REMINDER: these may return two sounds in the
	//future. it might be nice to have them return
	//a pitch/frequency value but frequency is too
	//large to include here
	EX_INI_NUMBER(0,0,4095)
	footstep_identifier, //SND 30
	headbutt_identifier,
	headwind_identifier, //SND 1000
	ricochet_identifier; //SND 55/96 (DOCUMENT ME)

	EX_INI_NUMBER(-1,0,1)
	headwind_ambient_wind_effect; //UNDOCUMENTED

	//DOCUMENT US
	//these are just an expedient way to adjust
	//the pitch/volume where SOM doesn't provide
	//a way
	//notes/pitch less than 128 are converted into 
	//IDirectSoundBuffer frequencies (SetFrequency)
	//higher values are interpreted as the frequency
	//EX_INI_NUMBER(-24,0,20) sample_pitch_adjustment;
	EX_INI_NUMBER(-24,0,100000) sample_pitch_adjustment;
	EX_INI_NUMBER(-100,0,0) sample_volume_adjustment;
};
EX_INI_SECTION(Script)
{						
	EX_INI_NUMBER(0,0,200) status_fonts_height; 
	
	EX_INI_FOURCC(0xffdcdcdc) status_fonts_tint;
	EX_INI_FOURCC(0xff464646) status_fonts_contrast;	
	EX_INI_FOURCC(0xffdcdcdc) system_fonts_tint;
	EX_INI_FOURCC(0xff464646) system_fonts_contrast;	

	EX_INI_PRINTF(wchar_t,0) status_fonts_to_use;
	EX_INI_PRINTF(wchar_t,0) system_fonts_to_use;

	const wchar_t *fonts_to_use_instead_of_(const wchar_t*)const;

  /*below were originally part of the long lost Locale section*/

	//NOTE: 24 is just in case of modifiers like @cyrillic
	EX_INI_PRINTF(char,24) locale_to_use_for_play;
	EX_INI_PRINTF(char,24) locale_to_use_for_play_if_not_found;
	EX_INI_PRINTF(char,24) locale_to_use_for_dates_and_figures;

	EX_INI_PRINTF(wchar_t,0) translation_to_use_for_play;		

	EX_INI_OPTION(0) 
	do_use_builtin_english_by_default,
	do_use_builtin_locale_date_format,
	do_not_show_button_names;
};
EX_INI_SECTION(Stereo)
{	
	//demonstration?
	EX_INI_FOURCC(9090) PSVRToolbox_status_port;
	EX_INI_FOURCC(14598) PSVRToolbox_command_port;	
	EX_INI_PRINTF(wchar_t,0) StartNoloServer_folder;

	//unlike Editor, these do not override Option
	//but are instead parallel to it since stereo
	//has special needs
	EX_INI_OPTION(0) 
	do_not_aa,
	do_not_lap,
	do_not_force_full_color_depth,
	do_not_dither,
	do_smooth,
	do_stipple;

	//som_hacks_PSVR_dim
	EX_INI_NUMBER(0,0,16) dim_adjustment;

	//DMIPTARGET
	//2021: som.shader.cpp had this hardwired
	//at 0.75. I'm assuming I forgot to change
	//the code... I'm thinking it's possible I
	//imagined 0.66666 was better (it happens)
	//EX_INI_NUMBER(0,0.666667,1) fuzz_constant;
	//2021: the supersampling ratio is passed as
	//the first parameter and defaults to 0.5 if
	//greater than 1
	//2021: DDRAW::ps2ndSceneMipmapping2 makes
	//things considerably blurrier by itself
	//EX_INI_NUMBER(0,0.75,1) fuzz_constant;
	EX_INI_NUMBER(0,0.5,1) fuzz_constant;

	//EXPERIMENTAL
	//0.1 seems best at preventing roll drift... 
	//could just be it's best overall, since roll
	//is easy to notice
	//(Upgrading to 64-bit helped a lot, so 0.05 
	//is being used for the time being, but 0.1 may
	//actually be better, for pure drift elmination)
	//UPDATE:
	//trying 0.035 to reduce wobble, since there
	//is now an antidrift integrator hardcoded to
	//use 0.1 (beta_constant2?)
	EX_INI_NUMBER(0.035,0.035,0.125) beta_constant;

	//94 looks right... 105~110 feels right moving/jumping
	//(94 looks foreshortened, but feels like it is solid)
	EX_INI_NUMBER(50,94,150) zoom;

	//this is an early demo of an UNDOCUMENTED bevel effect
	//that only works for KF2 and KF3 style frames
	//som.hacks.cpp uses it and it's off by default. 
	EX_INI_SERIES(-1,ZERO3,1) kf2_demo_bevel;

	//this is color correction like gamma_y
	//stereo_w isolates color/white balance
	EX_INI_BUFFER(wchar_t) stereo_y,stereo_w;

	//default is 100% with HP Reverb G2 set
	EX_INI_FOURCC(3100) aa_divisor;

};
EX_INI_SECTION(System)
{
	//unimplemented: returns number of "servers" registered
	int servers(int pair=0, const wchar_t **svr=0, const wchar_t **dll=0)const;

	//2017: this is to control dx_dsound_CommitDeferredSettingsProc
	//2022: 1 now turns off the som.MPX.cpp background thread stuff
	EX_INI_FOURCC(0) central_processing_units;
	//2022: this is provided just in case it helps some GPU drivers
	//NOTE: setting central_processing_units=1 disables multithread
	EX_INI_OPTION(0) do_direct3d_multi_thread;

	//2020: DOCUMENT US [Speaker?]
	EX_INI_FOURCC(0) master_volume_mute; //0~16	
	EX_INI_FOURCC(16) master_volume_loud; //0~16
	EX_INI_OPTION(0) do_hide_master_volume_on_startup;
};
EX_INI_SECTION(Volume)
{
	std::vector<EX::INI::volume_texture> textures;

	EX_INI_NUMBER(0,1,INF) volume_depth, volume_power;

	//UNIMPLEMENTED: this will (eventually) be used to optimize
	//textures that don't need inside-out rendering
	//mode 0 is experimental, to disable completely
	EX_INI_NUMBER(0,2,2) volume_sides;

	EX_INI_NUMBER(0,0,1) volume_fog_factor;
	EX_INI_NUMBER(0,0,INF) volume_fog_saturation;
};
EX_INI_SECTION(Window)
{
	//NOTE: this is the pop up window
	//do_hide_splash_screen_on_startup is renamed
	//to do_hide_company_title_on_startup
	EX_INI_FOURCC(2500) splash_screen_ms_timeout;
	
	//2022: forcing this on so device reset can be skipped
	//allowing maps to be loaded in the background
	EX_INI_OPTION(1) 
	do_scale_640x480_modes_to_setting;

	EX_INI_OPTION(0) 
	do_not_hide_window_on_startup,
	do_hide_company_title_on_startup,
	do_hide_opening_movie_on_startup, //UNFINISHED/UNDOCUMENTED
	do_force_fullscreen_inside_window,
	//do_scale_640x480_modes_to_setting,
	do_not_compromise_fullscreen_mode,
	do_center_picture_for_fixed_display,
	//2021: on my system this doesn't work unless the window is
	//smaller than the display... which is the opposite of what
	//you might expect
	do_force_immediate_vertical_refresh,
	do_use_interlaced_scanline_ordering,
	do_auto_pause_window_while_inactive,
	do_force_refresh_on_vertical_blank;

	//2021: might want to enable this for users to
	//test if it improves their game's frame rates
	enum{ do_not_clear=false };

	EX_INI_PRINTF(wchar_t,0) window_title_to_use;

	EX_INI_FOURCC(3) shader_compatibility_level;

	EX_INI_FOURCC(0) 
	shader_model_to_use,
	//obsolete
	//todo: custom resolution
	restrict_aspect_ratio_to; //'4:3'

	//2021: was defaulting to 0?
	//NOTE: I'm preventing dx7 for release builds in SomEx.ini.cpp
	EX_INI_FOURCC(11) 
	directx_version_to_use_for_window;

	//DOCUMENT US
	EX_INI_FOURCC(0) interior_border;
	EX_INI_FIGURE(0) seconds_per_frame; //restrict_frame_rate_to?

	void initialize_defaults()const; //2021

};}}

#undef INF
#undef EPS
#undef PI
#undef G
#undef F

#endif //SOM_EX_INI_INCLUDED