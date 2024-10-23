			
#ifndef EX_OUTPUT_INCLUDED
#define EX_OUTPUT_INCLUDED 

namespace EXLOG
{
	static int debug = 1; //debugging
	static int error = 7; //serious errors
	static int panic = 7; //undefined
	static int alert = 7; //warning
	static int hello = 7; //fun stuff
	static int sorry = 7; //woops

	static int *master = EX_LOG(SomEx); 
}

#define EXLOG_LEVEL(lv) if(lv<=EXLOG::debug&&lv<=*EXLOG::master) Ex.log
#define EXLOG_ERROR(lv) if(lv<=EXLOG::error&&lv<=*EXLOG::master) Ex.log
#define EXLOG_PANIC(lv) if(lv<=EXLOG::panic&&lv<=*EXLOG::master) Ex.log
#define EXLOG_ALERT(lv) if(lv<=EXLOG::alert&&lv<=*EXLOG::master) Ex.log
#define EXLOG_HELLO(lv) if(lv<=EXLOG::hello&&lv<=*EXLOG::master) Ex.log
#define EXLOG_SORRY(lv) if(lv<=EXLOG::sorry&&lv<=*EXLOG::master) Ex.log

namespace DDRAW
{
	extern int compat; //target
}

namespace EX
{
	class Font; //Ex.fonts.h

	extern const wchar_t *shadermodel;

	//PSVR was using 0.7
	//static const float stereo_font = 1?0.7f:0; //testing
	extern float stereo_font;

	//f5 overlay
	extern float
	fps, //frames per second //REMOVE ME
	dpi, //dots per inch
	clip, //clipped frames per second
	kmph, //clipped listener velocity  
	kmph2,e, //error
	mouse[3];
	extern BYTE keys[256];
	extern DWORD polls[256];
	#ifdef _DEBUG //sprintf is very slow
	#define EX_DBGMSG_DEFINED
	extern void dbgmsg(const char*,...); //debugging
	#elif defined(RELWITHDEBINFO)
	#define EX_DBGMSG_DEFINED
	extern void dbgmsg(const char*,...); //debugging
	#else
	inline void dbgmsg(...){} //NEW
	#endif //REMOVE ME?
	extern void (*speedometer)(float speeds[3]);

	extern bool output_overlay_f[1+12];
	extern bool &f1,&f2,&f3,&f4,&f5,&f6,&f7,&f8,&f9,&f10,&f11,&f12;

	extern void initialize_output_overlay();
	extern void initialize_output_console();

	extern void simulcasting_output_overlay_dik(unsigned char x, unsigned char unx);

	extern const wchar_t *need_a_logical_face_for_output_font(const wchar_t *lf, const wchar_t *description);
	extern const wchar_t *need_a_logical_face_for_output_font(int cp, const char *lf, const wchar_t *description);
	extern const wchar_t *describing_output_font(LOGFONTW &logical_font, const wchar_t *description,...);

	inline bool is_able_to_facilitate_font_output(){ return DDRAW::compat=='dx9c'; } 

	extern const EX::Font **reserving_output_font(LOGFONTW &logical_font, const wchar_t *description=0);

	extern HFONT need_a_client_handle_for_output_font(const EX::Font**); //EX::font::glyphs[0].hfont

	//2022: this calls Exselector::svg_view() when using OpenGL without OpenXR
	//in order to initialize the vertex shader transformation matrix just once
	//alternatively xr_f is forwarded to svg_end to enumerate the OpenXR views 
	extern void beginning_output_font(bool upsidedown=true,int(*xr_f)(int)=0);
	extern int displaying_output_font(const EX::Font**, HDC, DWORD flags, RECT&, const wchar_t*, int chars=-1, int clipx=0);
	extern bool releasing_output_font(const EX::Font**, bool d3d_interfaces_only=false);
}

#endif //EX_OUTPUT_INCLUDED