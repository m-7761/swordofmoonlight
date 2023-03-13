
#include "Somtexture.pch.h"

#ifdef _D3D9_H_
#ifdef __D3DX9_H__
#define SOMSHADER_D3DX
#endif
#endif
					 
#ifdef SOMSHADER_C

#include "../Sompaint/Sompaint.h"

#ifdef SOMSHADER_HLSL
#ifndef SOMSHADER_D3DX

#error include d3dx9.h in Somtexture.pch.h

#endif //SOMSHADER_D3DX
#endif //SOMSHADER_HLSL	
#endif //SOMSHADER_C

#include "Somthread.h"

//private implementation
#include "Somshader.inl"

//private
Somshader::Somshader()
{
	medium = 0;

	ascii = 0; file = 0; source = 0; 

	compiler = 0; compiler_data = 0; assembly_data = 0;
}

//private
Somshader::Somshader(const Somshader&)
{
	/*do not enter*/ assert(0); 
}

//private
Somshader::~Somshader()
{
	compile_with(0,0);	

	delete [] Somshader_inl.media;

	if(source) source->release();
}
		
static size_t Somshader_source(Somshader *in)
{	 	
	const Somshader *src = in; assert(in);

	while(src->medium==Somshader::SOURCE) src = src->source;
	
	in->ascii = src->ascii; in->file = src->file;

	return src->Somshader_inl.length;
}

static size_t Somshader_source(const Somshader *in)
{
	return Somshader_source((Somshader*)in);
}

void Somshader::compile_with(Somshader::C c, const void *c_data)const
{
	if(!this||Somshader_inl.expired) return;
		
	if(!compiler)
	{				
	#ifdef SOMSHADER_C			

		if(compiler_data)
		((SOMPAINT)compiler_data)->reclaim(assembly_data);
		const_cast<const wchar_t*&>(assembly_data) = 0;

	#else //???

		/*do not enter*/ assert(0); 

	#endif
	}
	else compile(L"outgoing",0);
	
	Somshader_inl.expired = !c&&!c_data; 

	const_cast<C&>(compiler) = c; 
	const_cast<const void*&>(compiler_data) = c_data;

	if(!compiler) return; //SOMSHADER_C
		
	compile(L"incoming",0);
}

//subroutine
static const char *Somshader_profile(int lang, const wchar_t *in);

const wchar_t *Somshader::compile(const wchar_t *profile, const wchar_t *main)const
{
	Somthread_h::section cs; //lock ascii and file members

	if(!this||Somshader_inl.expired) return 0; //assembly_data;

	const wchar_t* &out = const_cast<const wchar_t*&>(assembly_data);

	size_t s = Somshader_source(this); //paranoia: update ascii and file

	if(compiler) return out = compiler(this,profile,main,(void*)compiler_data);

	if(!compiler_data) return 0; //was compile_with not called?

#ifdef SOMSHADER_C

	SOMPAINT Sompaint = (SOMPAINT)compiler_data;

	Sompaint->reclaim(assembly_data); out = 0; //assembly_data

	char i, f[64] = ""; if(main) for(i=0;main[i]&&i<64;i++) f[i] = main[i]; 

	if(i==64){ assert(0); return 0; }else f[i] = 0;

	const char *hlsl = 0;
	const char *code = ascii; size_t code_s = s;

	if(file){ assert(0); return 0; } //not implementing

#ifdef SOMSHADER_HLSL
		
	if(!out)
	if(hlsl=Somshader_profile('hlsl',profile))
	{	
		if(!code) switch(*hlsl)
		{
		case 'v': code = vs_hlsl; code_s = vs_hlsl_s; break; 
		case 'p': code = ps_hlsl; code_s = ps_hlsl_s; break;
		}

		LPD3DXBUFFER bytecode = 0, errors = 0, disasm = 0;

		if(code) D3DXCompileShader(code,code_s,0,0,f,hlsl,0,&bytecode,&errors,0);

		if(bytecode)
		{
			DWORD *bc = (DWORD*)bytecode->GetBufferPointer();

			if(D3DXDisassembleShader(bc,0,0,&disasm)==D3D_OK)
			{
				size_t disasm_s = disasm->GetBufferSize(); assert(!out);
									
				out = Sompaint->assemble((char*)disasm->GetBufferPointer(),disasm_s);

				disasm->Release();
			}
			bytecode->Release();
		}		
		else if(errors) 
		{				
			assert(0); //just debugging

			const char *debug = (char*)errors->GetBufferPointer();

			errors->Release();
		}
	}

#endif //SOMSHADER_HLSL	 
#endif //SOMSHADER_C

	return out;
}

static const char *Somshader_profile(int lang, const wchar_t *in)
{
	if(!in||!*in) return 0;

	switch(in[0])
	{
	default: assert(0); return 0;

	case 'v': case 'V': case 'p': case 'P': break;
	}	  
	switch(in[1])
	{
	default: assert(0); return 0;

	case 's': case 'S': break;
	}	
	switch(in[2])
	{
	default: assert(0); return 0;

	case ' ': case '_': case '\0': break;
	}  
	switch(in[0])
	{	
	case 'v': case 'V': 
	
		if(lang=='hlsl') return "vs_3_0"; break;

	case 'p': case 'P': 

		if(lang=='hlsl') return "ps_3_0"; break;
	}

	assert(0); return 0;
}
	
size_t Somshader::character_length(int filter)const
{
	size_t out = Somshader_source(this);

	switch(filter)
	{	
	default: return 0; 

	case FILE: return file?out:0;
	case ASCII: return ascii?out:0;

	case 0: return out;
	}
}

static Somshader*
Somshader_open(int medium, const void *src, size_t srclen, Somshader *out)
{
	switch(medium)
	{
	case 0: assert(!src); break;
	
	case Somshader::FILE: 
	case Somshader::ASCII: 	
	case Somshader::SOURCE: if(src) break;

	default: assert(0); return 0;	
	}

	if(out)
	{
		assert(!out); return out; //unimplemented
	}
	else out = new Somshader;

	switch(out->medium=medium)
	{
	case Somshader::ASCII:

		if(srclen==size_t(-1))
		{
			srclen = strlen((char*)src);
		}

		if(~medium&Somshader::NEW)
		{
			char *dup = new char[srclen+1];
			
			memcpy(out->Somshader_inl.media=dup,src,srclen); dup[srclen] = '\0';
		}
		else out->Somshader_inl.media = (void*)src;

		out->Somshader_inl.length = srclen;

	break;
	case Somshader::FILE:

		if(srclen==size_t(-1))
		{
			srclen = wcslen((wchar_t*)src);
		}

		if(~medium&Somshader::NEW)
		{
			wchar_t *dup = new wchar_t[srclen+1]; 
			
			memcpy(out->Somshader_inl.media=dup,src,srclen*2); dup[srclen] = '\0';
		}
		else out->Somshader_inl.media = (void*)src;

		out->Somshader_inl.length = srclen;		

	break;
	case Somshader::SOURCE:

		out->source = (Somshader*)src;

	break;
	}

	Somshader_source(out);

	return out;
}

//static
const Somshader *Somshader::open(int medium, const void *sourcecode, size_t charlen)
{
	return Somshader_open(medium,sourcecode,charlen,0);
}

void Somshader::close()const
{
	if(!this) return;

	assert(0); return; //unimplemented
}

bool Somshader::refresh(int medium, const void *sourcecode, size_t charlen)const
{
	if(!this) return false;

	assert(0); return false; //unimplemented
}

const Somshader *Somshader::addref()const
{
	if(!this) return 0;
	
	Somshader_inl.refs++;
	
	return this;
}

const Somshader *Somshader::release()const
{
	if(!this) return 0;
	
	if(--Somshader_inl.refs>0) return this;

	return 0;
}

int Somshader::refcount()const
{
	return this?Somshader_inl.refs:0;
}

#ifdef SOMSHADER_HLSL

#define HLSL_FOGLESS

#define HLSL_DEFINE(D) "\n#define " #D "\n"
#define HLSL_IF(D)     "\n#if " #D "\n"
#define HLSL_IFDEF(D)  "\n#ifdef " #D "\n"
#define HLSL_IFNDEF(D) "\n#ifndef " #D "\n"
#define HLSL_ELSE	   "\n#else\n"
#define HLSL_ENDIF	   "\n#endif\n"

//copied from som.shader.cpp (SomEx.dll)
#define cDbgColor	 "float4 dbgColor    : register(c0);"
#define cDimViewport "float2 dimViewport : register(c1);"
#define cFpsRegister "float4 fpsRegister : register(c1);" //c1[2]
#define cColFactors  "float4 colFactors  : register(c2);"	  
#define cColFunction "float4 colFunction : register(ps,c3);"	  
#define cColCorrect  "float2 colCorrect  : register(ps,c7);"
#define cColColorkey "float4 colColorkey : register(ps,c8);"
//
#define cFvfFactors "float4 fvfFactors : register(vs,c3);"
//
#define cX4mWVP "float4x4 x4mWVP : register(vs,c4);"
#define cX4mWV  "float4x4 x4mWV  : register(vs,c8);"
#define cX4mIWV "float4x4 x4mIWV : register(vs,c12);" //UNUSED!
#define cX4mW   "float4x4 x4mW   : register(vs,c16);"
#define cX4mIV  "float4x4 x4mIV  : register(vs,c20);"
//
#define cFogFactors "float4 fogFactors : register(c24);"
#define cFogColor   "float3 fogColor   : register(ps,c25);"
#define cMatAmbient "float4 matAmbient : register(vs,c25);"
#define cMatDiffuse "float4 matDiffuse : register(vs,c26);"
#define cMatEmitted "float4 matEmitted : register(vs,c27);"
#define cEnvAmbient "float4 envAmbient : register(vs,c28);"
//
#define cLitAmbient(n) "float3 litAmbient["#n"] : register(vs,c32);"
//#define cLitLookup1(n) "float  litLookups["#n"] : register(vs,c32[3]);" 
#define cLitDiffuse(n) "float3 litDiffuse["#n"] : register(vs,c48);"
//#define cLitLookup2(n) "float  litLookups["#n"] : register(vs,c48[3]);" 
#define cLitVectors(n) "float4 litVectors["#n"] : register(vs,c64);"
#define cLitFactors(n) "float4 litFactors["#n"] : register(vs,c80);"
//
#define iNumLights "int numLights : register(vs,i0);"

//reworked from som.shader.cpp (SomEx.dll)
const char Somshader::vs_hlsl[] =

//	registers(0~4)
//	cDbgColor
//	cDimViewport
	"float fvfFactors = 1.0f; "
	"float colFactors = 0.0f; "
//  incompatible with instancing
//	cX4mWVP
//	cX4mWV
//	cX4mW  	 	
//  changes (Somplayer.dll)
	"column_major float4x4 x4mVP : register(vs,c4); "
	"column_major float4x4 x4mV  : register(vs,c8); "
	"column_major float4x4 x4mW  : TEXCOORD1; "
	HLSL_DEFINE(x4mWVP mul(x4mW,x4mVP))
	HLSL_DEFINE(x4mWV mul(x4mW,x4mV))
	"float perInstance  : TEXCOORD5; "
	HLSL_DEFINE(perOpacity perInstance.x)
	HLSL_DEFINE(perTweeen  perInstance.y)	
	cFogFactors
	cMatAmbient
	cMatDiffuse
	cMatEmitted
	cEnvAmbient
	cLitAmbient(16)
	cLitDiffuse(16)
	cLitVectors(16)
	cLitFactors(16)
	iNumLights

#ifdef HLSL_FOGLESS
#define shader_fog() \
	"Out.fog.y = 1.0f; Out.fog.x=0.0f; "
#else
#define shader_fog() \
	HLSL_IFDEF(DRANGEFOG)\
	"	float Z = length(mul(x4mWV,In.pos)); "\
	HLSL_ELSE\
	"	float Z = mul(x4mWV,In.pos).z; "\
	HLSL_ENDIF\
	"	Out.fog.x = "\
	HLSL_IFNDEF(DALPHAFOG)\
	"	1.0f-fogFactors.w+fogFactors.w* "\
	HLSL_ENDIF\
	"	((fogFactors.y-Z)/(fogFactors.y-fogFactors.x)); "\
	HLSL_IFDEF(DFOGLINE)\
	"	float2 sky = float2(DFOGLINE,DSKYLINE)*fogFactors.y; "\
	"   Out.fog.y = (sky.y-Z)/(sky.y-sky.x); "\
	HLSL_ELSE\
	"	Out.fog.y = 1.0f; "\
	HLSL_ENDIF\
	"	Out.fog = saturate(Out.fog); "
#endif

	"struct FOG_INPUT"
	"{"
	"	float4 pos : POSITION; "
	"};"
	"struct BLIT_INPUT"
	"{"
	"	float4 pos : POSITION; "
	"	float4 col : COLOR;    "
	"	float2 uv0 : TEXCOORD; "
	"};"
	"struct UNLIT_INPUT"
	"{"
	"	float4 pos : POSITION; "
	"	float4 col : COLOR;    "
	"	float2 uv0 : TEXCOORD; "
	"};"
	"struct SPRITE_INPUT"
	"{"
	"	float4 pos : POSITION; "
	"	float4 col : COLOR;    "
	"	float2 uv0 : TEXCOORD; "
	"};"
	"struct BLENDED_INPUT"
	"{"
	"	float4 pos : POSITION; "
	"	float3 lit : NORMAL;   "
	"	float2 uv0 : TEXCOORD; "
	"};"

	"struct CLASSIC_OUTPUT"
	"{"
	"	float4 pos : POSITION;  "
	"	float4 col : COLOR;     "
	"	float2 uv0 : TEXCOORD0; "
	HLSL_IF(SHADER_MODEL==3)
	"	float2 fog : FOG;	    "
	HLSL_ELSE
	"	float2 fog : TEXCOORD1; "
	HLSL_ENDIF
	"};"

	"CLASSIC_OUTPUT blit(BLIT_INPUT In)"
	"{"
	"	CLASSIC_OUTPUT Out; "

	//conversion to clip-space 
//	"	Out.pos.x = In.pos.x/dimViewport.x*2.0f-1.0f; "
//	"	Out.pos.y = 1.0f-In.pos.y/dimViewport.y*2.0f; "	
//	"	Out.pos.z = 0.0f; Out.pos.w = 1.0f; "

	//currently pre-converting to clip-space
	//projection space if fvfFactors.w is 0, screen space if 1
	"/*	Out.pos	= In.pos*fvfFactors.w+ " //Out.pos
	"		mul(x4mWVP,In.pos)*(1.0f-fvfFactors.w); */"	  

	"	Out.pos	= In.pos; "	//Fyi: sprites moved to sprite()
	"	Out.col = In.col; "
	"	Out.uv0 = In.uv0; "
	"	Out.fog = 0.0f;	  "

	"	Out.col+=colFactors; " //select texture

	"	Out.col = saturate(Out.col); "

//	"	Out.col = float4(fvfFactors.w,0,1-fvfFactors.w,1); " //debugging

	"	return Out; "
	"}"

	"CLASSIC_OUTPUT unlit(UNLIT_INPUT In)"
	"{"
	"	CLASSIC_OUTPUT Out; "

	"	Out.pos	= mul(x4mVP,In.pos); "

	"	Out.col = In.col; " 
	"	Out.uv0 = In.uv0; "		

		shader_fog() 

//	"	Out.col = float4(0,1,0,1); " //debugging

	"	return Out; "
	"}"

	"CLASSIC_OUTPUT sprite(SPRITE_INPUT In)"
	"{"
	"	CLASSIC_OUTPUT Out; "

	"	Out.pos	= mul(x4mWVP,In.pos); "

	"	Out.col = In.col; "
	"	Out.uv0 = In.uv0; "
	"	Out.fog = 0.0f;	  " 

	//	shader_fog() 

	"	Out.col+=colFactors; " //select texture

	"	Out.col = saturate(Out.col); "

	"	return Out; "
	"}"

	"struct LIGHT{ float3 ambient; float3 diffuse; };"
	
	"LIGHT Light(int i, float3 P, float3 N)"
	"{"	
	"	float3 D = litVectors[i].xyz-P; float M = length(D); "

	//Note: assuming lights are in range (no longer)
	"	if((litFactors[i].x-M)*litVectors[i].w<0.0f) return (LIGHT)0; " //Model 2.0+

//	"/*	float att = saturate(ceil(litFactors[i].x-M*litFactors[i].w)); */" //Model 1.0

	"	float att = 1.0f; " 

	"	att/=litFactors[i].y+litFactors[i].z*M+litFactors[i].w*M*M; "
			
	"	float3 L = normalize(D*litVectors[i].w +      " //point light
	"		-litVectors[i].xyz*(1.0f-litVectors[i].w)); " //directional 

	"	LIGHT Out = {litAmbient[i].rgb*att, "
	"				 litDiffuse[i].rgb*att*max(0.0f,dot(N,L))}; "  
	"	return Out; "
	"}"

	"CLASSIC_OUTPUT blended(BLENDED_INPUT In, column_major float4x4 x4mW:TEXCOORD1)"
	"{"
	"	CLASSIC_OUTPUT Out; "

	"	Out.pos	= mul(x4mWVP,In.pos); "

	"	Out.uv0 = In.uv0; "		

//"Out.col=In.pos*0.5+0.5; "
//"Out.col.rgb=In.lit*0.5+0.5; "
//"Out.pos=1.0/In.pos;"	
//"Out.pos = In.pos*0.25; Out.pos.z+=1; Out.pos.w=1;"
//"Out.pos = mul(x4mVP,In.pos); "
//"Out.col.rgb = x4mVP[3].xyz;"
//"Out.col.a = 1; "
"Out.col = 1;"
"Out.fog = 0;"

//OpenGL->Direct3D clip-space
//"Out.pos.z/=-Out.pos.w;"
//"Out.pos.z+=1; Out.pos.z*=0.5;"
//"Out.pos.z*=Out.pos.w;"


	/*
		shader_fog() 
		
#ifndef HLSL_FOGLESS
	HLSL_IFDEF(DSKYFLOOD) //Pow2 Looks a bit better but is it worth it??
//	"	Out.fog.y+=(1.0f-fogFactors.w)*(pow(In.pos.y-DSKYFLOOR,2.0f)*DSKYFLOOD-1.0f); "
	"	Out.fog.y+=(1.0f-fogFactors.w)*((In.pos.y-DSKYFLOOR)*DSKYFLOOD-1.0f); "
	HLSL_ENDIF
#endif
	"	float3 P = mul(x4mW,In.pos).xyz; "
	"	float3 N = normalize(mul((float3x3)x4mW,In.lit)); "
	
	"	float4 ambient = {0,0,0,0}, diffuse = {0,0,0,1}; " 
						 	
	HLSL_IFDEF(DLIGHTS) //vs_1_x
	"	for(int i=0;i<DLIGHTS;i++) "
	HLSL_ELSE
	"	[loop] " //ensure dynamic flow control
	"	for(int i=0;i<numLights;i++) "
	HLSL_ENDIF
	"	{"
	"		LIGHT sum = Light(i,P,N); "
	
	"		ambient.rgb+=sum.ambient; "
	"		diffuse.rgb+=sum.diffuse; "
	"	}"

	"	Out.col = matEmitted; "

	HLSL_IFDEF(DHDR) //clamp global ambient/emissive	
	"	Out.col.rgb+=matAmbient.rgb*envAmbient.rgb; "
	"	Out.col.rgb = saturate(Out.col.rgb); " 
	"	Out.col+=matAmbient*ambient+matDiffuse*diffuse; "
	HLSL_ELSE
	"	ambient.rgb+=envAmbient.rgb; "
	"	Out.col+=matAmbient*ambient+matDiffuse*diffuse; "
	HLSL_ENDIF

//	"	Out.col+=colFactors; " //select texture 

	HLSL_IFDEF(DHDR)
//	"   float4 invFactors = float4(1,1,1,1)-colFactors; "
//	"	Out.col = Out.col*invFactors+saturate(Out.col)*colFactors; " 
	HLSL_ELSE
	"	Out.col = saturate(Out.col); "
	HLSL_ENDIF
	 	
//	"	Out.col.rgb = (In.lit.xyz+1.0f)/2; " //debugging
*/
	"	return Out; "
	"}";

const size_t Somshader::vs_hlsl_s = sizeof(Somshader::vs_hlsl);

//reworked from som.shader.cpp (SomEx.dll)
const char Somshader::ps_hlsl[] = 

	cDbgColor
	cFogColor
	cColFactors
	cColFunction
	cColCorrect
//	cColColorkey
	"float4 colColorkey = float4(0,0,0,1);"

#define shader_colorkey() \
	HLSL_IFDEF(DCOLORKEY)\
	"	clip(Out.col.a-0.3f*colColorkey.w); "\
	"	float4 ck = Out.col/Out.col.a; ck.a = 1.0f; "\
	"	Out.col = ck*colColorkey.w + "\
	"				Out.col*(1.0f-colColorkey.w); "\
	HLSL_ENDIF

#define shader_correct() \
	HLSL_IFNDEF(DINVERT)\
	HLSL_IFDEF(DBRIGHTNESS)\
	"	Out.col.rgb	= saturate(Out.col.rgb); "\
	"	float3 inv = float3(1,1,1)-Out.col.rgb; "\
	"	Out.col.rgb+=Out.col.rgb*colCorrect.y; "\
	"	Out.col.rgb+=inv*colCorrect.x; "\
	HLSL_ENDIF\
	HLSL_ENDIF

#define shader_inverse() \
	HLSL_IFDEF(DINVERT)\
	HLSL_IFDEF(DBRIGHTNESS)\
	"	Out.col.rgb	= saturate(Out.col.rgb); "\
	"	float3 inv = float3(1,1,1)-Out.col.rgb; "\
	"	Out.col.rgb-=Out.col.rgb*colCorrect.y; "\
	"	Out.col.rgb-=inv*colCorrect.x; "\
	HLSL_ENDIF\
	HLSL_ENDIF

#define shader_fogline() 
//#ifdef HLSL_EXPERIMENTAL
//	HLSL_IFDEF(DFOGLINE)\
//	HLSL_IF(SHADER_MODEL>3)\
//	"	if(In.fog.y<1.0f) Out.pos = 0.997f; " \
//	HLSL_ENDIF
//	HLSL_ENDIF
//#endif

	"sampler2D sam0:register(s0);"

	"struct CLASSIC_INPUT"
	"{"
	"	float4 col : COLOR;     "
	"	float2 uv0 : TEXCOORD0; "
	HLSL_IF(SHADER_MODEL==3)
	"	float2 fog : FOG;	    "
	HLSL_ELSE
	"	float2 fog : TEXCOORD1; "
	HLSL_ENDIF
	"};"

	"struct CLASSIC_OUTPUT"
	"{"
	"	float4 col:COLOR0; "
	HLSL_IFDEF(DFOGLINE)
	"	float  pos:DEPTH; "
	HLSL_ENDIF
	"};"

	"CLASSIC_OUTPUT fog(CLASSIC_INPUT In)"
	"{"
	"	CLASSIC_OUTPUT Out; "
												 
	"	Out.col.rgb = fogColor; Out.col.a = 1.0f; "	  

		shader_correct() 

	"	return Out; "
	"}"	

	"CLASSIC_OUTPUT blit(CLASSIC_INPUT In)"
	"{"
	"	CLASSIC_OUTPUT Out; "

	"	Out.col = tex2D(sam0,In.uv0); " 

	//alpha unreliable???  
//	"	Out.col = saturate(Out.col+colFactors); " //maybe unnecessary

		shader_colorkey()
	
	"	Out.col*=In.col; " 

		shader_inverse()

//	"	Out.col*=dbgColor;	"

//	"	Out.col.ga = 1.0f; " //debugging

	"	return Out; "
	"}"	

	"CLASSIC_OUTPUT unlit(CLASSIC_INPUT In)"
	"{"
	"	CLASSIC_OUTPUT Out; "

	"	Out.col = tex2D(sam0,In.uv0); "

		shader_colorkey()
	
	"	Out.col*=In.col; "					

#ifndef HLSL_FOGLESS
	"   Out.col.rgb = lerp(fogColor,Out.col.rgb,In.fog.x); "
	"   Out.col.a = In.fog.y; "
#endif	
		shader_correct()

//	"	Out.col.ba = 1.0f; " //debugging

	"	return Out; "			 
	"}"	

	"CLASSIC_OUTPUT sprite(CLASSIC_INPUT In)"
	"{"
	"	CLASSIC_OUTPUT Out; "

	"	Out.col = tex2D(sam0,In.uv0); " 

		shader_colorkey()
	
	"	Out.col*=In.col; " 

		shader_correct()

	"	return Out; "
	"}"		  

	"CLASSIC_OUTPUT blended(CLASSIC_INPUT In)"	   
	"{"
	"	CLASSIC_OUTPUT Out; "
	
	"	Out.col = tex2D(sam0,In.uv0); "

//"Out.col*=In.col;" //*=
/*	
		shader_colorkey()
	
	"	float4 one = {1.0f,1.0f,1.0f,1.0f}; "
	
	"	Out.col = Out.col*In.col*(one-colFunction)+ " //modulate
	"			  Out.col*colFunction+In.col*colFunction; " //add	

//	"	Out.col = In.col; Out.col.a = 1.0f; " //debugging

#ifndef HLSL_FOGLESS
	"   Out.col.rgb = lerp(fogColor,Out.col.rgb,In.fog.x); "
	"   Out.col.a*=In.fog.y; " 
#endif	
		shader_correct() 

//	"	Out.col.rgb = In.col.rgb; " //debugging

//	"	Out.col.ra = 1.0f; " //debugging
*/
	"	return Out; "
	"}";	

const size_t Somshader::ps_hlsl_s = sizeof(Somshader::ps_hlsl);

#endif //SOMSHADER_HLSL

#ifdef SOMSHADER_ASM

const char Somshader::ARBvp1[] = //OpenGL vertex shader
//http://www.codesampler.com/source/ogl_arb_shader_simple_vs2ps.zip
"!!ARBvp1.0\n\
\n\
# Constant Parameters\n\
PARAM mvp[4] = { state.matrix.mvp }; # Model-view-projection matrix\n\
\n\
# Per-vertex inputs\n\
ATTRIB inPosition = vertex.position;\n\
ATTRIB inColor    = vertex.color;\n\
ATTRIB inTexCoord = vertex.texcoord;\n\
\n\
# Per-vertex outputs\n\
OUTPUT outPosition = result.position;\n\
OUTPUT outColor    = result.color;\n\
OUTPUT outTexCoord = result.texcoord;\n\
\n\
DP4 outPosition.x, mvp[0], inPosition;   # Transform the x component of the per-vertex position into clip-space\n\
DP4 outPosition.y, mvp[1], inPosition;   # Transform the y component of the per-vertex position into clip-space\n\
DP4 outPosition.z, mvp[2], inPosition;   # Transform the z component of the per-vertex position into clip-space\n\
DP4 outPosition.w, mvp[3], inPosition;   # Transform the w component of the per-vertex position into clip-space\n\
\n\
MOV outColor, inColor;       # Pass the color through unmodified\n\
MOV outTexCoord, inTexCoord; # Pass the texcoords through unmodified\n\
\n\
END";

const size_t Somshader::ARBvp1_s = sizeof(Somshader::ARBvp1);

const char Somshader::ARBfp1[] = //OpenGL pixel shader
//http://www.codesampler.com/source/ogl_arb_shader_simple_vs2ps.zip
"!!ARBfp1.0\n\
\n\
# Fragment inputs\n\
ATTRIB inTexCoord = fragment.texcoord;      # First set of texture coordinates\n\
ATTRIB inColor    = fragment.color.primary; # Diffuse interpolated color\n\
\n\
# Fragment outputs\n\
OUTPUT outColor   = result.color;\n\
\n\
TEMP texelColor;\n\
TXP texelColor, inTexCoord, texture, 2D;\n\
\n\
MUL outColor, texelColor, inColor;  # Modulate texel color with vertex color\n\
#ADD outColor, texelColor, inColor;  # Add texel color to vertex color\n\
\n\
END";

const size_t Somshader::ARBfp1_s = sizeof(Somshader::ARBfp1);

const char Somshader::vs_2_0[] = //Direct3D pixel shader
//http://devmaster.net/forums/topic/4423-opengl-vertexfragment-shader-in-asm-simple-texturing-shader/
"vs_2_0\n\
\n\
dcl_position v0\n\
dcl_color v1\n\
dcl_texcoord v2\n\
\n\
dp4 oPos.x, v0, c0\n\
dp4 oPos.y, v0, c1\n\
dp4 oPos.z, v0, c2\n\
dp4 oPos.w, v0, c3\n\
\n\
mov oD0, v1\n\
mov oT0, v2";

const size_t Somshader::vs_2_0_s = sizeof(Somshader::vs_2_0);

const char Somshader::ps_2_0[] = //Direct3D pixel shader
//http://devmaster.net/forums/topic/4423-opengl-vertexfragment-shader-in-asm-simple-texturing-shader/
"ps_2_0\n\
\n\
dcl v0\n\
dcl t0\n\
dcl_2d s0\n\
\n\
texld r0, t0, s0\n\
mov oC0, r0";

const size_t Somshader::ps_2_0_s = sizeof(Somshader::ps_2_0);

#endif //SOMSHADER_ASM