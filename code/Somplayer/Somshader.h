
#ifndef SOMSHADER_INCLUDED
#define SOMSHADER_INCLUDED

class Somshader 
{		
private: 

	Somshader();	
	Somshader(const Somshader&);
	~Somshader();

public: //readonly

	const char *ascii;

	const wchar_t *file;

	const Somshader *source; 
		
	int medium; enum{ ASCII=1, FILE=2, SOURCE=3 };

	//Compiler callback			//source code    //profile      //entrypoint
	typedef const wchar_t *(*C)(const Somshader*,const wchar_t*,const wchar_t*,void*);
							
	C compiler; const void *compiler_data; const wchar_t *assembly_data;
	
	void compile_with(C, const void *C_data=0)const;

	template<class T> void compile_with
	(const wchar_t *(*c)(const Somshader*,const wchar_t*,const wchar_t*,T*), T *d)const
	{
		return compile_with((C)c,(const void*)d);
	}

	//Before a shader can be compiled (and assembled) it must have a compiler 
	//associated with it. A compiler is a user provided callback that is set with 
	//compile_with (above) or set when open (below) is called with the SOURCE medium 
	//
	//compile_with can be called freely to change the compiler and the compiler private
	//data pointer (compiler_data) that is passed through the last argument of the callback
	//
	//#ifdef SOMSHADER_C 
	//If 0 is provided to compile_with the compiler that is built into Somshader.cpp is used
	//When using the builtin compiler C_data must provide a valid Sompaint.h SOMPAINT server
	//The default compiler uses preprocessor macros from the server's define(0) macro buffer
	//The default compiler will attempt to compile with ASCII but will abort if given a FILE
	//
	//If 0 is provided for both compile_with arguments future compile_with and compile calls
	//will be ignored. This is to allow the Somshader to expire regardless of its references
	//
	//When the compiler is changed a handshake is sent to the incoming and outgoing compilers
	//by the callback which should at the least be watched out for. The profile argument will
	//be set to L"incoming" so that the newly added compiler can manage references if it must
	//and L"outgoing" is sent to the original compiler (by the final release or compile_with)
	//
	//WARNING: the default compiler sets the assembly to 0 (after deleting it) on L"outgoing" 
	//
	//_if successful_
	//The result is copied to the assembly_data member (above) accessed by assembly (below)
	const wchar_t *compile(const wchar_t *profile, const wchar_t *entrypoint=L"main")const;

	inline const wchar_t *assembly()const{ return this?assembly_data:0; }

	//Get the character length of ascii or file
	size_t character_length(int filter=0)const; //ASCII or FILE 

	//// INSTANTIATION //////////////////////////////////////////
	
	//open: the following constants can be OR'd with medium

	enum{ NEW=32 }; //NEW: sourcecode is new allocated and can be delete'd (or kept)

	//medium is one of 0, ASCII, FILE, or SOURCE (detailed below)
	//
	//ASCII: sourcecode is a char* to the source code; a multi-line character string
	//FILE: sourcecode is a wchar_t* address of a file that is itself the source code
	//SOURCE: sourcecode is another Somshader object that makes use of the same source
	//0: the code that is builtin to Somshader.cpp will be used. sourcecode should be 0
	//
	//The input parameters are duplicated character for character and copied into ascii
	//or file (depending upon the medium) or taken from source; in which case addref is
	//called on source and release is called after Somshader is itself finally released
	static const Somshader *open(int medium=0, const void *sourcecode=0, size_t charlen=-1);	

	void close()const; //delete the sourcecode; the assembly and compiler remain viable

	//reopen the shader's source code [behaves identically to open]
	//WARNING: if you refresh the source of a SOURCE Somshader the SOURCE's
	//ascii and file members will point to old memory. The ascii and file members
	//are only guaranteed to hold correct for the duration of the C compiler callback
	bool refresh(int medium, const void *sourcecode=0, size_t charlen=-1)const;	

	//// REFERENCE COUNTING ///////////////////////////////////
	
	const Somshader *addref()const; //returns self
	const Somshader *release()const; //returns self or 0

	int refcount()const; //should be used for diagnostic only

	inline const Somshader *deplete()const //garbage collection
	{
		return refcount()==1?release():this; //ok a valid use
	}  

	//// MISCELLANEOUS ////////////////////////////////////////
	
	//Some dummy shaders #ifdef SOMSHADER_ASM
	static const char ARBvp1[]; static const size_t ARBvp1_s;
	static const char ARBfp1[]; static const size_t ARBfp1_s;
	static const char vs_2_0[]; static const size_t vs_2_0_s;
	static const char ps_2_0[]; static const size_t ps_2_0_s;

	//The builtin shaders #ifdef SOMSHADER_HLSL
	static const char vs_hlsl[]; static const size_t vs_hlsl_s;
	static const char ps_hlsl[]; static const size_t ps_hlsl_s;

	template<typename T, int N> struct registers
	{	
		typedef T tN[N]; inline operator tN*(){ return (tN*)this; }

		inline int operator[](void *r){ return (T*)r-(T*)this; }
	};

	//These are from SomEx.dll (som.shader.cpp)	
		
	//Underscored registers are not compatible with 
	//instancing and are therefore not useful to us
	//
	//Commented names to the side indicate that the
	//registers have been repurposed for instancing

	struct vs_c 
	:
	public registers<float,4>
	{
	float
	/*c0*/	dbugColor[4],
	/*c1*/	dimViewport[4], 
	/*c2*/	colFactors[4],  
	/*c3*/	fvfFactors[4],
	/*c4*/	_x4mWVP[4][4], //x4mVP
	/*c8*/	_x4mWV[4][4],  //x4mV
	/*c12*/	_x4mIWV[4][4], 
	/*c16*/	_x4mW[4][4],
	/*c20*/	x4mIV[4][4],
	/*c24*/ fogFactors[4],			
	/*c25*/ matAmbient[4],
	/*c26*/ matDiffuse[4],
	/*c27*/ matEmitted[4],
	/*c28*/ envAmbient[4];	

	//undecl_29_31[4][32-29],

	/*c32*/ //litAmbient[4][16],
	/*c48*/ //litDiffuse[4][16],
	/*c64*/ //litVectors[4][16],
	/*c80*/ //envFactors[4][16];
	};
	struct vs_i
	:
	public registers<int,4>
	{
	int
	/*i0*/ numLights[4];
	};		
	struct ps_c
	:
	public registers<float,4>
	{			
	float
	/*c0*/ dbugColor[4],
	/*c1*/ dimViewport[4], 
	/*c2*/ colFactors[4],  
	/*c3*/ colFunction[4],

	undecl_4_6[4][7-4],

	/*c7*/ colCorrect[4],
	/*c8*/ colColorkey[4],
	       
	undecl_9_23[4][24-9],

	/*c24*/ fogFactors[4],
	/*c25*/ fogColor[4];
	};
	
	static const size_t vs_c_s = sizeof(vs_c)/sizeof(float);
	static const size_t vs_i_s = sizeof(vs_i)/sizeof(int);
	static const size_t ps_c_s = sizeof(ps_c)/sizeof(float); 

	/*/////// private implementation ///////*/

	#include "Somshader.inl" //"pimpl"
};

#endif //SOMSHADER_INCLUDED