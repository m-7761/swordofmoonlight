
#ifndef SOM_SHADER_INCLUDED
#define SOM_SHADER_INCLUDED
		   
namespace SOM
{
	void initialize_shaders();

	struct Shader
	{
		enum choice
		{
		effects,
		effects_d3d11,
		classic,		
		classic_blit,
		classic_unlit,
		classic_sprite, 
		classic_shadow,		
		classic_blended,
		classic_backdrop,		

		//pixel shaders
		classic_fog, //use with unlit 
		classic_volume, //unlit or blended?
		_TOTAL_
		};

		const unsigned which;

		inline Shader(choice one):which(one){}

		inline operator unsigned(){ return which; }

		void log(); //debugging
	};

	struct Define
	{
		enum choice
		{
		defined=0, //undefine all definitions (undef only)
		stereo,stereoLOD,stereo_y,stereo_w,
		colorkey,
		lights,	 //int: static number of lights (vs_1_x)
		aa,dissolve, //antialiasing
		ss, //EXPERIMENTAL
		dither,		
		stipple,		
		highcolor,
		alphafog, //UNUSED
		rangefog, //UNUSED
		fogpowers,
		brightness,
		invert,
		green,	 
		hdr,		
		gamma_n,
		gamma_y,
		gamma_y_stage,
		ambient2,
		sRGB,
		index, //SHADER_INDEX
		choices //TOTAL
		};

		const int which;

		inline Define(choice one):which(one){}

		inline operator int(){ return which; }
	};	

	template<int N=3> struct VS
	{
		static const int model;

		static void define(SOM::Define,...);

		static void undef(SOM::Define); 

		static bool ifdef(SOM::Define); 

		static char *compile(SOM::Shader,int=-1);

		//links shaders / optimize dx.ddraw.h globals
		static void configure(int count, char*,...);

		static void release(int count, char*,...);

		static const DWORD *assemble(const char*);
		static const DWORD *debug(SOM::Shader,int=-1);

		//2021: these two just get GLSL strings
		static const char *header(SOM::Shader); //GLSL
		static const char *shader(SOM::Shader); //GLSL
	};

	template<int N=3> struct PS 
	{
		static const int model;

		static void define(SOM::Define,...);

		static void undef(SOM::Define);

		static bool ifdef(SOM::Define); 

		static char *compile(SOM::Shader,int=-1);

		//links shaders / optimize dx.ddraw.h globals
		static void configure(int count, char*,...);

		static void release(int count, char*,...);

		static const DWORD *assemble(const char*);
		static const DWORD *debug(SOM::Shader,int=-1);

		//2021: these two just get GLSL strings
		static const char *header(SOM::Shader); //GLSL
		static const char *shader(SOM::Shader); //GLSL
	};

	typedef VS<3> VS3; typedef PS<3> PS3;
	typedef VS<2> VS2; typedef PS<2> PS2;
	//typedef VS<1> VS1; typedef PS<1> PS1;
}

#endif //SOM_SHADER_INCLUDED