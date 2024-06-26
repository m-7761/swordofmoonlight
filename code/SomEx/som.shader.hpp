
#define GLSL HLSL
#define GLSL_DEFINE HLSL_DEFINE

//how stupid is it 'layout' can't add numbers?
//4.3 GL_ARB_enhanced_layouts add this ability
//to plain OpenGL. it also adds "offset" which
//is sorely needed here but won't work with ES
//DDRAW::compat_glUniformBlockBinding_1st_of_6
//ANGLE limits MAX_UNIFORM_BUFFER_BINDINGS for
//D3D11 to 24! :(
#define GLSL_0 10
#define GLSL_1 11 //vsI
#define GLSL_2 12 //vsF
#define GLSL_5 15 //psF

//UNDEFINE US!! (BELOW)
//it's probably for the best to work with GLSL
//keywords to make maintenance straightforward
#define saturate(x) clamp(x,0.0,1.0)
//may need to modify on case-by-case
//https://stackoverflow.com/questions/7610631/glsl-mod-vs-hlsl-fmod
//#define fmod(x,y) (x-y*trunc(x/y))
#define fmod(x,y) mod(x,y)
#define clip(x) if((x)<0.0) discard;
#define sample samp1e //reserved?
#define ddx dFdx
#define ddy dFdy
#define tex2D texture //texture2D says "dimensions mismatch"
//NOTE: these are all (mat,vec) multiplies
//#define mul(x,y) (x*y)
#define mul(x,y) (y*x) //UNTESTED: D3D/GL are reverse I think
#define float3x3 mat3
#define float3x4 mat3x4
#define float4x4 mat4
//these support DGAMMA_Y
#define GLSL_HLSL_CONV \
#define float1 float\n\
#define float2 vec2\n\
#define float3 vec3\n\
#define float4 vec4\n\
#define lerp mix\n

namespace som_shader_glsl{ //NAMESPACE

static const char *classic[2] = //GLSL
{
	GLSL( //VERTEX SHADER INCLUDE (HEADER)
	GLSL_HLSL_CONV
	layout(binding=GLSL_1,std140) uniform vsI
	{
		//NOTE: I don't know how GLSL packs
		//for-loop inputs, under D3D9 "x" is
		//the loop count, and "z" is the 
		//increment, and so on. I can't find
		//GLSL docs
		int numLights; //layout(offset=0)
	};
	layout(binding=GLSL_2,std140) uniform vsF
	{
		//REMINDER: names can't match those
		//in the pixel shader since OpenGL 
		//pretends they're the same logical
		//variable (i.e. add #define to psF)

		vec4 skyRegister; //layout(offset=0)
		vec4 rcpViewport; //1
		vec4 colFactors; //2
		vec4 _pad_1_vsF; //3 PADDING
		//EXPERIMENTAL
		//I think row_major matches som.shader.cpp
		//for array subscript purposes, I'm not so
		//sure about operator*/mul. "mul" tends to 
		//need to be swapped when this changes
		//REMINDER: layout(row_major) doesn't work
		//for mat objects declared inside the code
		layout(row_major) mat4 x4mWVP; //4-7 //REMOVE ME
		layout(row_major) mat4 x4mWV; //8-11 //REMOVE ME
		layout(row_major) mat4 x4mW; //12-15 //blended vsh
		layout(row_major) mat4 x4mV; //16-19 //shadow vsh
		layout(row_major) mat4 x4mP; //20-23 //VR //x4mIV
		vec4 fogFactors; //24
		vec4 matAmbient; //25
		vec4 matDiffuse; //26
		vec4 matEmitted; //27
		vec4 envAmbient; //28
		vec4 bmpAmbient; //29
		vec4 _pad_2_vsF[18]; //30-47 PADDING
		vec4 litDiffuse[16]; //48-63
		vec4 litVectors[16]; //64-79
		vec4 litFactors[16]; //80-95
	};)

	//HACK: this is just for DGAMMA_N to use if wants
	GLSL(float3 rotate_y(float a, float3 n)
	{
		//LOW PRIORITY
		//GLSL doesn't have sincos (not this way anyway)
		//float1 s,c; sincos(a,s,c); //weird
		//return float3(dot(n,float3(c,0,s)),n.y,dot(n,float3(-s,0,c)));
		return float3(dot(n,float3(cos(a),0,sin(a))),n.y,dot(n,float3(-sin(a),0,cos(a))));		
	})

	GLSL(
	//NOTE: SHADER_INDEX comes from dx.d3d9X.cpp
	#if SHADER_INDEX>=16
	struct VS_INPUT //FX
	{
		/*
		float4 pos : POSITION;  
		float2 uv0 : TEXCOORD0; 
		float2 uv1 : TEXCOORD1; //fallback position
		*/
		float4 pos; float2 uv0; float2 uv1;
	};
	//in struct VS_INPUT
	//{
		layout(location=0) in float4 In_pos;
		layout(location=10) in float2 In_uv0;
		layout(location=11) in float2 In_uv1;
	//}In;	
	#define INPUT VS_INPUT(In_pos,In_uv0,In_uv1)\n
	struct VS_OUTPUT //FX
	{
		/*
		float4 pos : POSITION;  
		float2 uv0 : TEXCOORD0; 
		float2 uv1 : TEXCOORD1; //fallback position
		#ifdef DSTEREO
		float1 eye : TEXCOORD2;
		#endif
		*/
		float2 uv0; float2 uv1; float4 pos; //gl_Position
	};
	//D3D11 rejects GL_EXT_shader_io_blocks
	//layout(location=0) out VS_OUTPUT
	//{
		layout(location=0) out float2 Out_uv0;
		layout(location=1) out float2 Out_uv1;
	//}Out;
	#define OUTPUT(x) Out_uv0 = x.uv0;\
	Out_uv1 = x.uv1; gl_Position = Out.pos;\n
	#else	
	void set_gl_Position(float4); //predeclaring
	#if SHADER_INDEX!=7 //SHADOW
	struct CLASSIC_INPUT
	{
		/*LIT_INPUT
		float4 pos : POSITION; 
		float3 lit : NORMAL;   
		classic_stereo_DEPTH 
		float2 uv0 : TEXCOORD;
		*/
		/*UNLIT_INPUT
		float4 pos : POSITION; 
		float4 col : COLOR;
		float2 uv0 : TEXCOORD;
		classic_stereo_DEPTH
		*/
		float4 pos; float3 lit; float4 col; float2 uv0;
	};
	//in struct CLASSIC_INPUT
	//{
		layout(location=0) in float4 In_pos; 
		layout(location=1) in float3 In_lit; //LIT_INPUT
		layout(location=3) in float4 In_col; //UNLIT_INPUT
		layout(location=10) in float2 In_uv0;

		//EXPERIMENTAL
		//these come in via a glVertexBindingDivisor side channel
		#ifdef DSTEREO
		layout(location=5) in float4 Xr_fov; //asymmetric projection
		layout(location=6) in float4 Xr_mvP; //model-view position
		layout(location=7) in float4 Xr_mvQ; //model-view quaternion
		layout(location=8) in float4 Xr_vpR; //viewport and frustum
		#endif

	//}In;
	#define FOG_INPUT CLASSIC_INPUT(In_pos,vec3(0.0),vec4(0.0),In_uv0)\n
	#define LIT_INPUT CLASSIC_INPUT(In_pos,In_lit,vec4(0.0),In_uv0)\n
	#define UNLIT_INPUT CLASSIC_INPUT(In_pos,vec3(0.0),In_col.bgra,In_uv0)\n
	struct CLASSIC_OUTPUT
	{
		/*
		float4 pos : POSITION;  
		float4 col : COLOR;     
		float4 uv0 : TEXCOORD0;
		classic_stereo_DEPTH 
		float4 fog : TEXCOORD1; 
		*/
		float4 col; float4 uv0; float4 fog; float4 pos; //gl_Position
	};
	//D3D11 rejects GL_EXT_shader_io_blocks
	//layout(location=0) out CLASSIC_OUTPUT
	//{
		layout(location=0) out float4 Out_col;
		layout(location=1) out float4 Out_uv0;
		layout(location=2) out float4 Out_fog; //model-view pos
		layout(location=3) out float4 Out_pos; //projection pos
		#ifdef DSTEREO
		layout(location=4) out float4 Xr_dpos;
		#endif
	//}Out;
	#define OUTPUT(x) Out_col = x.col;\
	Out_uv0 = x.uv0; Out_fog = x.fog; set_gl_Position(x.pos);\n
	#else
	struct SHADOW_INPUT
	{
		/*LIT_INPUT
		float4 pos : POSITION; 
		float4 col : COLOR;
		float4 center : TEXCOORD0; 
		float3 xforms : TEXCOORD1;
		classic_stereo_DEPTH
		*/
		float4 pos; float4 col; 
		
		float4 center; float4 xforms;
	};
	//in struct SHADOW_INPUT
	//{
		layout(location=0) in float4 In_pos; 
		layout(location=3) in float4 In_col;
		layout(location=10) in float4 In_center; 
		layout(location=11) in float4 In_xforms; 

		//EXPERIMENTAL
		//these come in via a glVertexBindingDivisor side channel
		#ifdef DSTEREO
		layout(location=5) in float4 Xr_fov; //asymmetric projection
		layout(location=6) in float4 Xr_mvP; //model-view position
		layout(location=7) in float4 Xr_mvQ; //model-view quaternion
		layout(location=8) in float4 Xr_vpR; //viewport and frustum
		#endif

	//}In;
	#define INPUT SHADOW_INPUT(In_pos,In_col,In_center,In_xforms)\n
	struct CLASSIC_SHADOW
	{
		/*
		float4 pos : POSITION;  
		float4 col : COLOR0;     
		float4 fog : COLOR1;
		float4 xforms : COLOR2;
		float4x4 mat : TEXCOORD0; 
		classic_stereo_DEPTH
		*/
		float4 col; float4x4 mat; float4 pos; //gl_Position

		float4 xforms; //2023

		float4 fog; //classic_stereo_pos?
	};
	//D3D11 rejects GL_EXT_shader_io_blocks
	//layout(location=0) out CLASSIC_SHADOW
	//{
		layout(location=0) out float4 Out_col; //flat?
		layout(location=1) out float4 Out_pos; //Out_fog
		layout(location=2) out float4 Out_xforms;
		layout(location=3) out float4x4 Out_mat; //flat?
		//REMINDER: location=4,5,6 is float4x4...
		#ifdef DSTEREO
		layout(location=7) out float4 Xr_dpos;
		#endif
	//}Out;
	#define OUTPUT(x) Out_col = x.col;\
	Out_xforms = x.xforms; Out_mat = x.mat; set_gl_Position(x.pos);\n
	#endif //SHADER_INDEX!=7 
	)
	
	//doing GL viewspace conventions in shaders for now
	//may convert to matrix form later, but stereo might
	//not agree with that... NOTE: flipping the Y axis is
	//easily done in the effects copy
	GLSL(void set_gl_Position(float4 o)
	{
		Out_pos = o; //2022: this is Out.fog in som.shader.cpp

		//MAGIC FROM HERE!
		//https://veldrid.dev/articles/backend-differences.html
		o.z = o.z*2.0-o.w;
		//again, same link comes to the rescue (I wouldn't think
		//straight -y would work) anyway, for some reason having
		//the effects pass flip the blit is wrong for glViewport
		//(I don't understand why) and the volume() shader had a
		//gotcha when reconstructing the position that this also
		//solves
		 o.y = -o.y; gl_Position = o;
	})
	
	GLSL(
	#ifdef DSTEREO //OpenXR?
	float3 openxr_rot(float3 v)
	{
		float4 q = Xr_mvQ;

		//alternative? (I think these come from glm)
		return v+2.0*cross(q.xyz,cross(q.xyz,v)+q.w*v);
	//	return v*(q.w*q.w-dot(q.xyz,q.xyz))+2.0*q.xyz*dot(q.xyz,v)+2.0*q.w*cross(q.xyz,v);
	}
	float4 openxr_fov(float4 v) //openxr_x4mP?
	{
		//NOTE: THE FOLLOWING CODE JUST BUILDS
		//A PROJECTION MATRIX
		
	  //source: XMMatrixPerspectiveOffCenterLH

		float nz = Xr_vpR.x;
		float fz = Xr_vpR.y;
		float4 f = Xr_fov*nz;
	//	float rw = 1.0/(f.y-f.x); //Xr_vpR.z?
	//	float rh = 1.0/(f.z-f.w); //-Xr_vpR.w?
		float2 r = 1.0/float2(f.y-f.x,f.z-f.w);		
		float range = fz/(fz-nz);

		float2 vp = 2.0*nz*r; //00,11

		//TODO: hand optimize this once works?
		//NOTE: GLSL doesn't optimize anything
		return mul(transpose(float4x4(
		float4(vp.x,0.0,0.0,0.0),
		float4(0.0,vp.y,0.0,0.0),
		float4(-(f.x+f.y)*r.x, //left+right
			   -(f.z+f.w)*r.y, //top+bottom
			   range,1.0),
		float4(0.0,0.0,nz*-range,0.0))),v);
	}	
	float4x4 openxr_x4mV()
	{
		float3 p = openxr_rot(Xr_mvP.xyz);
		float4 q = Xr_mvQ;		

		float qxx = q.x*q.x;
		float qyy = q.y*q.y;
		float qzz = q.z*q.z;
		float qxz = q.x*q.z;
		float qxy = q.x*q.y;
		float qyz = q.y*q.z;
		float qwx = q.w*q.x;
		float qwy = q.w*q.y;
		float qwz = q.w*q.z;

		/*I think maybe this is a right-handed matrix?
		return float4x4(
		float4(1.0-2.0*(qyy+qzz),2.0*(qxy+qwz),2.0*(qxz-qwy),p.x),
		float4(2.0*(qxy-qwz),1.0-2.0*(qxx+qzz),2.0*(qyz+qwx),p.y),
		float4(2.0*(qxz+qwy),2.0*(qyz-qwx),1.0-2.0*(qxx+qyy),p.z),
		float4(0.0,0.0,0.0,1.0));*/
		return transpose(float4x4(
		float4(1.0-2.0*(qyy+qzz),2.0*(qxy+qwz),2.0*(qxz-qwy),0.0),
		float4(2.0*(qxy-qwz),1.0-2.0*(qxx+qzz),2.0*(qyz+qwx),0.0),
		float4(2.0*(qxz+qwy),2.0*(qyz-qwx),1.0-2.0*(qxx+qyy),0.0),
		float4(p,1.0)));
	}
	#endif
	)

	GLSL( //NESTED DEFINE (#define must end with \n)
	#ifndef DSTEREO
	#define classic_stereo_pos(x) \
	Out.fog = mul(x4mWV,x);\
	Out.pos = mul(x4mP,Out.fog);\n
	#define classic_stereo(_) \n
	#else
	#define classic_stereo_pos(x)\
	Out.fog = float4(openxr_rot(mul(x4mW,x).xyz+Xr_mvP.xyz),1.0);\
	Out.pos = openxr_fov(Out.fog);\n
	#define classic_stereo(_) \
	Xr_dpos = Xr_fov*Xr_vpR.y;\n
	#endif	
	)

	GLSL_DEFINE(classic_aa,	
	Out.pos = classic_aa_sub(Out.pos);
	)GLSL(
	float4 classic_aa_sub(float4 io) //Out_pos
	{
		//REMINDER (VR)
		//THIS IS A CONCEPT THAT COULD FULLY LEVERAGE DO_AA IN VR
		//UNFORTUNATELY IT'S NOT COMPATIBLE WITH OPEXR'S WORKFLOW
		//https://www.gamedeveloper.com/programming/vr-distortion-correction-using-vertex-displacement
		#ifdef DAA
		#ifndef DSTEREO
		float4 hvp = rcpViewport*io.w; 
		#else //OpenXR
		//HACK: Xr_mvP.w is storing the "staircase"
		//constant for lack of a better arrangement
		//(I mean, technically it is shifting view)
		float2 rzw = Xr_vpR.zw;
		float4 hvp = float4(rzw,rzw+rzw*Xr_mvP.w)*io.w;
		#endif		
		io.xy-=fmod(io.xy,hvp.xy);	
		io.xy+=hvp.zw;
		#endif
		return io;
	}
	#endif //SHADER_INDEX>=16
	)
			
		,


	GLSL( //FRAGMENT SHADER INCLUDE (HEADER)
	GLSL_HLSL_CONV
	layout(binding=GLSL_5,std140) uniform psF
	{
		//these names clash with vsF that
		//OpenGL wants to pretend are the
		//same logical variable and so it
		//won't link them because they're
		//in buffer blocks
		#define skyRegister skyRegister2\n
		#define rcpViewport rcpViewport2\n
		#define colFactors colFactors2\n
		#define fogFactors fogFactors2\n
		vec4 skyRegister; //layout(offset=0)
		vec4 rcpViewport; //1
		vec4 colFactors; //2
		vec4 colFunction; //3
		vec4 _pad_1_psF[3]; //4-6 PADDING
		vec2 colCorrect; //7
		vec4 colColorkey; //8		
		vec4 _pad_2_psF; //9 PADDING
		vec4 volRegister; //10
		vec4 _pad_3_psF[13]; //11-23 PADDING
		vec4 fogFactors; //24
		vec3 fogColor; //25
		vec4 colColorize; //26
		vec4 farFrustum; //27
	};	
	layout(binding=0) uniform sampler2D sam0;
	layout(binding=1) uniform sampler2D sam1;
	layout(binding=2) uniform sampler2D sam2;
	layout(binding=3) uniform sampler2D sam3;
	//EXPERIMENTAL
	//layout(origin_upper_left) in vec2 gl_FragCoord; //vec4
	)
		//of course GLSL requires this
		"#ifndef DGAMMA_Y_STAGE\n"
		"#define DGAMMA_Y_STAGE 0\n"
		"#endif\n"

	//DISABLING DSTEREO
	//SetScissorRect performs better than instancing
	//with rolling buffers (see dx_d3d9c_drawprims2)
	GLSL_DEFINE(classic_stereo) //NOP

	GLSL_DEFINE(classic_colorkey,
	Out.col = classic_colorkey_sub(Out.col); //In.uv
	)GLSL(
	float4 classic_colorkey_sub(float4 io) //Out_col
	{
		//SHADER_MODEL>=3
		#ifndef TEXTURE0_NOCLIP
		clip(io.a-0.5f);
		#endif		
		io.a = 1.0; //GLSL
		
		return io;
	})
	GLSL_DEFINE(classic_correct,
	Out.col.rgb = classic_correct_sub(Out.col.rgb);
	)GLSL(
	float3 classic_correct_sub(float3 y) //Out_col
	{
		//DGAMMA expects "y" macro parameter
		#if 2==DGAMMA_Y_STAGE
		y = DGAMMA_Y;
		#endif
		#ifndef DINVERT
		#ifdef DBRIGHTNESS
		y = saturate(y); 
		float3 inv = vec3(1.0)-y; 
		y+=y*colCorrect.y; 
		y+=inv*colCorrect.x; 
		#endif
		#endif
		return y;
	})
	GLSL_DEFINE(classic_inverse,
	Out.col = classic_inverse_sub(Out.col);
	)GLSL(
	float4 classic_inverse_sub(float4 io) //Out_col
	{
		#ifdef DINVERT
		#ifdef DBRIGHTNESS
		io.rgb	= saturate(io.rgb); 
		float3 inv = vec3(1.0)-io.rgb; 
		io.rgb-=io.rgb*colCorrect.y; 
		io.rgb-=inv*colCorrect.x; 
		#endif
		#endif
		return io;
	})		
	GLSL_DEFINE(classic_rangefog, 
	float3 fog = classic_rangefog_sub(In.fog).xyz;
	Out.col.rgb = lerp(fogColor,Out.col.rgb,fog.x);
	Out.col.a*=fog.y;)GLSL(
	float4 classic_rangefog_sub(float4 io) //In_fog
	{
		io.z = length(io.xyz); 
		io.x = (fogFactors.y-io.z)*fogFactors.x; 		
		io.y = (skyRegister.y-io.z)*skyRegister.x; 
		io.xy = saturate(io.xy);
		#ifdef DFOGPOWERS
		#if SHADER_INDEX==5 //sprite?
		io.xy = pow(io.xy,DFOGPOWERS.zw);
		#else
		io.xy = pow(io.xy,DFOGPOWERS.xy);
		#endif
		#endif
		return io;
	})
		
	GLSL_DEFINE(classic_z,
	Out.z = float4(In.fog.z*farFrustum.w,0.0,0.0,1.0);)

	GLSL( //NESTED DEFINE (#define must end with \n)
	#if 1!=DGAMMA_Y_STAGE
	#define classic_gamma\n
	#else
	#define classic_gamma \
	{ float3 y = Out.col.rgb; y = DGAMMA_Y; Out.col.rgb = y; }\n
	#endif
	)
	
	GLSL(
	#if SHADER_INDEX<16 //2 functions are named "sample"
	float4 sample(float2 uv)
	{
		#ifdef DIN_EDITOR_WINDOW //2021
		float4 o = tex2D(sam0,uv,-1); 
		#else
		float4 o = tex2D(sam0,uv); 
		#endif
		//http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
		#if DSRGB			
		#if 0 //could 3 sqrt beat pow?
			float3 s1 = sqrt(o.rgb), s2 = sqrt(s1), s3 = sqrt(s2);
		o.rgb = 0.662002687*s1+0.684122060*s2-0.323583601*s3-0.0225411470*o.rgb;
			#else
		o.rgb = max(1.055*pow(o.rgb,vec3(0.416666667))-0.055,vec3(0.0));
			#endif
		#endif
		//return o;
		return o.bgra;
	}	
	float4 classic_aa_sub(float2 uv, float blit) //blit=1
	{
		#if defined(DAA) //&& !defined(DDEBUG)
		float2 dx = ddx(uv)*rcpViewport.z*blit;
		float2 dy = ddy(uv)*rcpViewport.w*blit;
		return sample(uv+dx+dy);
		#else
		return sample(uv);
		#endif
	}
	#endif
	)	
	//note, unrelated to the vertex shader classic_aa
	GLSL_DEFINE(classic_aa(blit),
	Out.col = classic_aa_sub(In.uv0.xy,float(blit));)

	GLSL(
	//NOTE: SHADER_INDEX comes from dx.d3d9X.cpp
	#if SHADER_INDEX>=16	
	struct PS_INPUT //FX
	{
		/*
		float2 uv0 : TEXCOORD0; 
		float2 pos : TEXCOORD1; //uv1 //gl_FragCoord/gl_FragCoord.w?
		#ifdef DSTEREO
		float1 eye : TEXCOORD2;
		#endif
		*/
		float2 uv0; float2 pos;
	};
	//D3D11 rejects GL_EXT_shader_io_blocks
	//layout(location=0) in PS_INPUT
	//{
		layout(location=0) in float2 In_uv0;
		layout(location=1) in float2 In_pos; //uv1 //gl_FragCoord? 
	//}In;
	#define INPUT PS_INPUT(In_uv0,In_pos)\n
	struct PS_OUTPUT //FX
	{
		float4 col;
	};
	//out PS_OUTPUT
	//{
		layout(location=0) out float4 Out_col;
	//}Out;	
	#define OUTPUT(x) Out_col = x.col;\n
	#else
	#if SHADER_INDEX!=7 //SHADOW	
	struct CLASSIC_INPUT
	{
		/*
		float4 col : COLOR0;    
		float4 uv0 : TEXCOORD0; 
		float4 fog : TEXCOORD1; //4 is for shadow? 
		#ifdef DSTEREO
		float stereo : DEPTH;
		#endif
		#if 3<=SHADER_MODEL
		float2 vpos : VPOS; //volume
		#endif
		*/
		float4 col; float4 uv0; float4 fog; float4 pos; float2 vpos;
	};
	//D3D11 rejects GL_EXT_shader_io_blocks
	//layout(location=0) in CLASSIC_INPUT
	//{
		layout(location=0) in float4 In_col;
		layout(location=1) in float4 In_uv0;
		layout(location=2) in float4 In_fog;
		layout(location=3) in float4 In_pos;
		#ifdef DSTEREO
		layout(location=4) in float4 Xr_dpos;
		#endif

	//}In;
	#define INPUT CLASSIC_INPUT(In_col,In_uv0,In_fog,In_pos,gl_FragCoord.xy)\n
	#else
	struct CLASSIC_SHADOW
	{
		/*
		float4 col : COLOR0;     		
		float4 fog : COLOR1; 
		float3 xforms : COLOR2;
		float4x4 mat : TEXCOORD0; 
		#ifdef DSTEREO		
		float stereo : DEPTH; //needs vpos
		#endif
		#if 3<=SHADER_MODEL
		float2 vpos : VPOS; //Shader Model 3
		#endif	
		*/
		float4 col; float4 xforms; float4x4 mat; float4 pos; float2 vpos;
	};
	//D3D11 rejects GL_EXT_shader_io_blocks
	//layout(location=0) in CLASSIC_SHADOW
	//{

		//I don't know if "flat" here will improve performance
		//but I think it might hurt it

		layout(location=0) in float4 In_col; //flat?
		layout(location=1) in float4 In_pos; //In_fog
		layout(location=2) in float4 In_xforms; //In_xforms
		layout(location=3) in float4x4 In_mat; //flat?
		//REMINDER: location=4,5,6 is float4x4...
		#ifdef DSTEREO
		layout(location=7) in float4 Xr_dpos;
		#endif
	//}In;	
	#define INPUT CLASSIC_SHADOW(In_col,In_xforms,In_mat,In_pos,gl_FragCoord.xy)\n
	#endif
	struct CLASSIC_OUTPUT
	{
		/*
		float4 col:COLOR0, z:COLOR1;
		*/
		float4 col; float4 z; //MRT
	};
	//out struct CLASSIC_OUTPUT
	//{
		layout(location=0) out float4 Out_col;
		layout(location=1) out float4 Out_z;
	//}Out;
	#define OUTPUT(x) Out_col = x.col; Out_z = x.z;\n		
	
	float3 stereo_dpos(float2 uv) //DEEP POSITION
	{
		float3 o;
		#ifdef DSTEREO //OpenXR?
		uv = uv*0.5+0.5;
		o = float3(lerp(Xr_dpos.xw,Xr_dpos.yz,uv),farFrustum.z);
		#else
		o = farFrustum.xyz; o.xy*=uv; //symmetrical?
		#endif
		return o;
	}

	#endif
	)
};

static const char *effects[2] = //GLSL
{	
	GLSL(
	void main() //effects() VERTEX SHADER
	{
		VS_INPUT In = INPUT; //const

		VS_OUTPUT Out;

		Out.pos = In.pos; //pre-converted to clip space
		Out.uv0 = In.uv0.xy;
		Out.uv1 = In.uv1;

	//	#ifdef DSTEREO		   
		//WARNING: THIS REQUIRES THE SECTIONS TO BE DRAWN
		//SEPARATELY, AND THERE NEEDS TO BE SPACE DOWN THE
		//MIDDLE SO THAT IT ISN'T AMBIGUOUS
		//sign(0) is 0... assuming can be a little bit off
	//	Out.eye = In.uv0.x<0.5f?0.00001f:-0.5f;
	//	#endif
		
		OUTPUT(Out); //return Out; 
	}),
	GLSL( //effects() FRAGMENT SHADER
	float4 sample(float2 uv)
	{
		#if SOM_SHADER_MAPPING2 && 3<=SHADER_MODEL
		#define mip0 skyRegister.xy \n //const
		#define mip1 skyRegister.zw \n //const
		#else
		const float2 mip0 = vec2(0.0), mip1 = vec2(0.0);
		#endif
		#ifdef DMIPTARGET
			#if !SOM_SHADER_MAPPING2			
			float4 sum = textureLod(sam0,uv,DMIPTARGET); 
				#ifdef DDISSOLVE 
				sum+=textureLod(sam1,DMIPTARGET); 
				#endif	
			#else //EXPERIMENTAL
			float4 sum = lerp(textureLod(sam0,uv,0.0),
							  textureLod(sam0,mip0.xy+uv,1.0),DMIPTARGET); 
				#ifdef DDISSOLVE
				sum+=lerp(textureLod(sam1,uv,0),
						  textureLod(sam1,mip1.xy+uv,1.0),DMIPTARGET); 
				#endif	
			#endif
		#elif DDISSOLVE==2 && !defined(DSTEREO) //2021 do_lap

			#ifdef DSS
			const float W = 1.0;
			#else
			const float W = 0.0;
			#endif

			float4 cmp0 = textureLod(sam0,uv+mip0.xy,W+1.0);
			float4 cmp1 = textureLod(sam1,uv+mip1.xy,W+1.0);
			float4 sum0 = textureLod(sam0,uv,W);
			float4 sum1 = textureLod(sam1,uv,W);

			#if defined(DSS) && !defined(DIN_EDITOR_WINDOW)
			float4 sum = pow(abs(cmp0-cmp1),vec4(1.5))*0.8+0.1;
			#else
			float4 sum = min(vec4(1),pow(abs(cmp0-cmp1),vec4(1.5))*3.0);
			#endif

			//4/10/24 error C7011: implicit cast from "int" to "bool"
			//if(1) //2024: swing model afterimage?
			{
				float ai = 2-sum0.a;
				float ia = (2.0-ai)+0.4*(1.0-sum1.a); //over 1!
				ai-=0.2*(1.0-sum1.a);
				cmp0*=ai; cmp1*=ia; 
				sum0*=ai; sum1*=ia;
			}

			sum = lerp(sum0+sum1,cmp0+cmp1,sum);
			
		#elif defined(DSS)
			float4 sum = textureLod(sam0,uv,1.0);
			#ifdef DDISSOLVE //do_lap2 (formerly do_lap)
			sum+=textureLod(sam1,uv,1.0); 
			#endif
		#else //basic?
			float4 sum = tex2D(sam0,uv);
			#ifdef DDISSOLVE //do_lap2 (formerly do_lap)
			sum+=tex2D(sam1,uv); 
			#endif	
		#endif
		return sum;
	}
	void main() //effects() FRAGMENT SHADER
	{
		PS_INPUT In = INPUT;

		PS_OUTPUT Out;

		//NOTE: som.shader.cpp has the full shader with comments
		//this shader omits PSVR style VR, unused code, and most
		//comments. it must be kept up-to-date with the original

		float2 uv = In.uv0;
		
		#ifdef DSTIPPLE
		#if DSTIPPLE==2 //2020: repurposing rcpViewport.zw for DAA
		uv+=fmod(round(In.pos.yx+rcpViewport.zw),vec2(2.0))*rcpViewport.xy;
		#else
		uv+=fmod(round(In.pos.yx),vec2(2.0))*rcpViewport.xy;
		#endif
		#endif

	classic_stereo		
		//:(
		//SADLY (FRIGHTENINGLY) IF THESE ARE GLOBAL CONSTANTS 
		//THEY DON'T WORK. IS IT A "LINKING" ERROR?
		#ifdef DDISSOLVE
		const float samples = 0.5; //int	
		#else
		const float samples = 1.0; //int
		#endif
	//	#ifndef DCA
		Out.col = sample(uv);	
	//	#else	
			//REMOVED: "chromatic aberration" was here
	//	#endif
		Out.col.rgb*=samples;
		//http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
		#if DSRGB
		Out.col.rgb = max(1.055*pow(Out.col.rgb,vec3(0.416666667))-0.055,vec3(0.0));
		#endif
	
		//NEW do_gamma (King's Field II noodling)
		#if 3==DGAMMA_Y_STAGE 
		{
			//D3DXMACRO doesn't seem to allow for parameters
			//Out.col.rgb = DGAMMA_Y(Out.col.rgb);
			float3 y = Out.col.rgb; y = DGAMMA_Y; Out.col.rgb = y;
		}
		#endif

		Out.col.rgb+=colColorize.rgb; 
		Out.col.rgb	= saturate(Out.col.rgb); 
		Out.col.rgb*=1.0-colColorize.a; 
	
		#ifdef DINVERT
		#ifdef DBRIGHTNESS
		float3 inv = vec3(1.0)-Out.col.rgb; 
		Out.col.rgb+=Out.col.rgb*colCorrect.y; 
		Out.col.rgb+=inv*colCorrect.x; 
		#endif
		#endif
	
		#ifdef DDITHER	
		//0.0001f: helps Intel Iris Pro graphics
		Out.col.rgb+=tex2D(sam3,In.pos/(8.0*DDITHER)+0.0001).r/8.0; 
		#endif
		
		#ifdef DGREEN
		float3 highc = float3(31.0,63.0,31.0); 
		#else 
		float3 highc = float3(31.0,31.0,31.0); 
		#endif
	
		#ifdef DHIGHCOLOR
		Out.col.rgb = floor(saturate(Out.col.rgb)*highc)/highc; 
		#endif				
	
		#if !defined(DBLACK) || defined(DSTEREO)	
		#ifdef DSTEREO 
//		Out.col.rgb+=vec3(0.007843137254); //2/255 
		Out.col.b+=0.007843137254; //UNTESTED (PSVR)
		#else
		Out.col.b+=0.003921568627; //1/255  
		#endif
		#endif

		OUTPUT(Out); //return Out;
	})
};

static const char *classic_blit[2] = //GLSL
{	
	GLSL(void main() //blit() VERTEX SHADER
	{
		CLASSIC_INPUT In = UNLIT_INPUT; //const

		CLASSIC_OUTPUT Out;
		
		#if defined(DSTEREO) && 1==SHADER_INDEX
		//Out.pos = mul(x4mWVP,In.pos);
			#if 0 //som_hacks_menuxform?
		classic_stereo_pos(In.pos)
			#else //no world transform
		Out.fog = float4(openxr_rot(In.pos.xyz+Xr_mvP.xyz),1.0);
		Out.pos = openxr_fov(Out.fog);
			#endif
		#else
		Out.pos = In.pos;
		Out.fog = float4(0,0,0,0); //warning
		#endif

		Out.col = In.col; 
		Out.uv0 = float4(In.uv0.xy,0,0); 		

		Out.col+=colFactors; //select texture

		Out.col = saturate(Out.col); 

	classic_stereo(!false)

	classic_aa //NEW: helps map edges... doesn't seem to hurt

		OUTPUT(Out); //return Out;
	}),
	GLSL(void main() //blit() FRAGMENT SHADER
	{
		CLASSIC_INPUT In = INPUT;

	classic_stereo //NOP?

		CLASSIC_OUTPUT Out;

		#ifdef DSS
		classic_aa(0.8)
		#else
		classic_aa(0.2) //icons are too blurry
		#endif

	classic_colorkey
	
		Out.col*=In.col;  

	classic_inverse

		Out.z = vec4(0.0); //compiler
//Out.col.ra = vec2(1.0);	
		OUTPUT(Out); //return Out;
	})
};

static const char *classic_fog[2] = //GLSL
{	
	GLSL(void main() //VS
	{
		CLASSIC_INPUT In = FOG_INPUT; //const

		CLASSIC_OUTPUT Out;

		//Out.pos = mul(x4mWVP,In.pos);
		classic_stereo_pos(In.pos)
	//	Out.col = vec4(0.0);
		Out.uv0 = float4(In.uv0.xy,0,0);

//	classic_z  		
	classic_stereo(true)
	classic_aa

		OUTPUT(Out); //return Out; 
	}),
	GLSL(void main() //fog() FRAGMENT SHADER
	{
		CLASSIC_INPUT In = INPUT;

	classic_stereo //NOP?

		CLASSIC_OUTPUT Out;
	
		#ifndef TEXTURE0_NOCLIP
	classic_aa(1)
		#else
		//classic_colorkey sets a and rgb is
		//set to fogColor below
		#endif

	classic_colorkey //clip (discard)

		Out.col.rgb = fogColor;

	classic_z

		OUTPUT(Out); //return Out;
	})
};

static const char *classic_unlit[2] = //GLSL
{	
	GLSL(void main() //unlit() VERTEX SHADER
	{
		CLASSIC_INPUT In = UNLIT_INPUT; //const

		CLASSIC_OUTPUT Out;

		//Out.pos = mul(x4mWVP,In.pos);
		classic_stereo_pos(In.pos)
		Out.col = In.col;  
		Out.uv0 = float4(In.uv0.xy,0,0);

//	classic_z  		
	classic_stereo(true)
	classic_aa

		OUTPUT(Out); //return Out; 
	}),
	///////////////
	//			 //
	// DUPLICATE //
	//			 //
	///////////////
	GLSL(void main() //unlit() FRAGMENT SHADER
	{
		CLASSIC_INPUT In = INPUT;

	classic_stereo //NOP?

		CLASSIC_OUTPUT Out;

	classic_aa(1)
	classic_colorkey
	classic_gamma
	
		Out.col*=In.col; 					

		#ifndef DIN_EDITOR_WINDOW
	classic_rangefog		
	classic_correct
	classic_z	
		#else
		Out.z = vec4(0.0); //compiler
		#endif

		OUTPUT(Out); //return Out;
	})
};
static const char *classic_volume[2] = //GLSL
{	
	GLSL(void main() //VS
	{
		#error volume is pixel shader only
	}),
	GLSL(void main() //volume() FRAGMENT SHADER
	{
		CLASSIC_INPUT In = INPUT;

		CLASSIC_OUTPUT Out;

		///////////////
		//			 //
		// DUPLICATE //
		//			 //
		///////////////
		//CLASSIC_OUTPUT Out = unlit(In);
		{
			classic_stereo //NOP?

			classic_aa(1)
			classic_colorkey
			classic_gamma
	
				Out.col*=In.col; 					

			classic_rangefog		
			classic_correct
			classic_z	
		}

		float2 vp = In.vpos*rcpViewport.xy;

		float3 pos = stereo_dpos(In.pos.xy/In.pos.w);

		//pos*=abs(tex2D(sam2,vp+0.0001f).x);
		pos*=abs(tex2D(sam2,vp).x);
		float depth = volRegister.z; //skyRegister
		float power = volRegister.w; //skyRegister
		float alpha = pow(length(pos-In.fog.xyz)*depth,power);

		//REMINDER: this is repurposing psColorkey
		//#if 2000!=DCOLORKEY
		alpha = saturate(alpha);
		Out.col.rgb = lerp(Out.col.rgb,colColorkey.rgb,alpha*colColorkey.w);
		//#endif

		Out.col.a = alpha; 

		//2023: fade monsters to white on death
		float1 gray = dot(Out.col.rgb,float3(0.222,0.707,0.071));		
		Out.col.rgb = lerp(Out.col.rgb,gray.rrr,In.uv0.a)+In.uv0.a*0.5;
		//HACK: sqrt(fade)->fade*fade
		Out.col.a*=pow(1-In.uv0.a,3);

	//	Out.col.rgb = pos; //DEBUGGING
		
		OUTPUT(Out); //return Out;
	})
};

static const char *classic_sprite[2] = //GLSL
{	
	GLSL(void main() //sprite() VERTEX SHADER
	{
		CLASSIC_INPUT In = UNLIT_INPUT; //const

		CLASSIC_OUTPUT Out;

		//Out.pos = mul(x4mWVP,In.pos);
		classic_stereo_pos(In.pos)
		Out.col = In.col; 
		Out.uv0 = float4(In.uv0.xy,0,0);
	
//	classic_z //Out.fog = 0.0f;	   	
	classic_stereo(true)
	classic_aa
		
		Out.col+=colFactors; //select texture

		Out.col = saturate(Out.col); 

		OUTPUT(Out); //return Out; 
	}),
	GLSL(void main() //sprite() FRAGMENT SHADER
	{
		CLASSIC_INPUT In = INPUT;

	classic_stereo //NOP?

		CLASSIC_OUTPUT Out; 		

	classic_aa(1)
	classic_colorkey
	classic_gamma
	
		Out.col*=In.col;  		

		//2020: I've resisted adding fog to
		//sprites because it's cool to see the
		//light of the flames or eyes off in the 
		//distance
		//NOTE: fov_sky_and_fog_powers3 and 4 is
		//added to partially restore the old way
		classic_rangefog 

	classic_correct
	classic_z

		OUTPUT(Out); //return Out; 
	})
};

static const char *classic_shadow[2] = //GLSL
{	
	GLSL(void main() //shadow() VERTEX SHADER
	{
		SHADOW_INPUT In = INPUT; //const

		CLASSIC_SHADOW Out;

		Out.col = In.col;
		Out.xforms = In.xforms;

		float4 center,corner;				  
		center.xyz = In.center.xyz;		
		center.w = corner.w = 1.0;
		corner.xyz = In.pos.xyz;
	//	corner.xz*=Out.xforms.zw;

		//2023: rotate shadow?
		float cx = corner.x, cz = corner.z;
		corner.x = cx*In.xforms.x+cz*In.xforms.y;
		corner.z = cx*-In.xforms.y+cz*In.xforms.x;

		corner.xyz+=center.xyz;
		//Out.pos = mul(x4mWVP,corner);
		classic_stereo_pos(corner)

	//IMPORTANT AA IS DONE BEFORE Out.fog = Out.pos BELOW	
	classic_stereo(true) classic_aa

		#ifdef DSTEREO
		Out.mat = openxr_x4mV();
		center = mul(Out.mat,center);
		Out.mat = transpose(Out.mat);
		#else
		center = mul(x4mV,center);
		Out.mat = transpose(x4mV);		
		#endif
		for(int i=0;i<3;i++)
		Out.mat[i].w = -dot(center.xyz,Out.mat[i].xyz);

		OUTPUT(Out); //return Out;
		
		//2022: rename fog->pos and repurpose fog/pos.z
		//after set_gl_Position
		//Out.fog = Out.pos;		
		//float bias = EX_INI_SHADOWUVBIAS;
		//Out_pos.z = 0.5/(Out.xforms.z*EX_INI_SHADOWRADIUS/bias);
		Out_pos.z = 0.5/(Out.xforms.z/EX_INI_SHADOWUVBIAS); //not Out.pos!
	}),
	GLSL(void main() //shadow() FRAGMENT SHADER
	{
		CLASSIC_SHADOW In = INPUT;

	classic_stereo //NOP?

		CLASSIC_OUTPUT Out;

		Out.col = In.col; 

		float4 pos = float4(stereo_dpos(In.pos.xy/In.pos.w),1.0); 
		
		//SHADER_MODEL>=3
		//pos.xyz*=tex2D(sam2,In.vpos*rcpViewport.xy+0.0001).x;
		pos.xyz*=tex2D(sam2,In.vpos*rcpViewport.xy).x;
		
		float3 st = mul(In.mat,pos).xzy; //!!
		float sx = st.x; float sy = st.y;
		st.x = sx*In.xforms.x+sy*-In.xforms.y;
		st.y = sx*In.xforms.y+sy*In.xforms.x;
		st.y/=In.xforms.w/In.xforms.z; //UV space
		st.xy = vec2(0.5f)-st.xy*In.pos.z;
		//1.9: should (probably) be 2 (squeezing every last drop)
		st.z*=In.pos.z*(EX_INI_SHADOWRADIUS/EX_INI_SHADOWVOLUME*1.9);
		st.z*=st.z; //pow(st.z,2.0); 

		float4 dd = clamp(float4(ddx(st.xy),ddy(st.xy)),-0.5,0.5);
		float4 s1 = textureGrad(sam0,st.xy+0.0,dd.xy,dd.zw);
		float4 s2 = textureGrad(sam0,1.15*(st.xy-0.5)+0.5,dd.xy,dd.zw);
		Out.col.a*=(s1.r+s2.r)*0.5;

		Out.col.a-=st.z;
					
	//classic_rangefog	
	float3 fog = classic_rangefog_sub(pos).xyz;
	//Out.col.a*=fog.x; //too dark
	Out.col.rgb = lerp(fogColor,Out.col.rgb,fog.x);
	Out.col.a*=fog.y;
	classic_correct	

		Out.z = vec4(0.0); //compiler

		OUTPUT(Out); //return Out;
	})
};

static const char *classic_blended[2] = //GLSL
{	
	GLSL(//blended() VERTEX SHADER
	struct LIGHT
	{
		float3 /*ambient,*/ diffuse; 
	};	
	LIGHT Light(int i, float3 P, float3 N)
	{	
		float3 D = litVectors[i].xyz-P; float M = length(D); 

		float att = 1.0f;  
		att/=litFactors[i].y+litFactors[i].z*M+litFactors[i].w*M*M; 
			
		float3 L = normalize(D*litVectors[i].w + //point light
		-litVectors[i].xyz*(1.0f-litVectors[i].w)); //directional 

		LIGHT Out;
		//Out.ambient = litAmbient[i].rgb*att;
		Out.diffuse = litDiffuse[i].rgb*att*max(0.0,dot(N,L));   
		return Out;
	}	
	void main() //blended() VERTEX SHADER
	{
		CLASSIC_INPUT In = LIT_INPUT; //const

		CLASSIC_OUTPUT Out;

		//Out.pos = mul(x4mWVP,In.pos);
		classic_stereo_pos(In.pos)
		Out.uv0 = float4(In.uv0.xy,0,0);

		#ifdef DIN_EDITOR_WINDOW
		Out.fog = float4(0,0,0,0);
		#else
		//TODO: TRY TO DO BELOW
//	classic_z
		#endif

		//HACK: letting DGAMMA_N modify this to adjust
		//ambient value to demo King's Field II
		Out.col = float4(matEmitted.rgb,0);
		float4 ambient = float4(0,0,0,0);
		float4 diffuse = float4(0,0,0,1);  

		//NOTE: normalize is because MDL is 1024 scale
		//(soft body animations don't seem to have this
		//problem? but maybe if the editors scales them?)
		float3 P = mul(x4mW,In.pos).xyz; 
		//NOTE: normalize is for giant/baby monsters and
		//originally MDL used a different scale than MDO
		/*HLSL only accepts cast (GLSL only constructor)
		float3 N = normalize(mul((float3x3)x4mW,In.lit));*/
	//	float3 N = normalize(mul(float3x3(x4mW),In.lit)); 
		//2022: I DON'T GET IT. WHAT'S WRONG WTIH mat3(mat4)?
		float3 N = normalize(mul(x4mW,float4(In.lit,0.0))).xyz; 

		//EXPERIMENTAL
		//Note: relying on compiler to optimize out the
		//computation of N
		#ifdef DGAMMA_N
		#if SHADER_INDEX==8 //duplicate of blended shader?
		{
			//DGAMMA_Y is lowercase
			float3 n = In.lit;
			//King's Field II
			//N = pow(abs(N),1.25)*float3(-1,sign(n.y),1);
			n = DGAMMA_N;
			N = n;
		}
		#endif
		#endif
		
		for(int i=0;i<numLights;i++) 
		{	
			LIGHT sum = Light(i,P,N); 	
		//	ambient.rgb+=sum.ambient;
			diffuse.rgb+=sum.diffuse; 
		}

		//Out.col = matEmitted;  

		#ifdef DHDR //clamp global ambient/emissive	
		Out.col.rgb+=matAmbient.rgb*envAmbient.rgb; 
		Out.col.rgb = saturate(Out.col.rgb); 
		#else
		ambient.rgb+=envAmbient.rgb; 		
		#endif
		Out.col+=matAmbient*ambient+matDiffuse*diffuse; 

	//	Out.col+=colFactors; //select texture 

		#ifndef DHDR
		Out.col = saturate(Out.col); 
		#endif
	 	
	//lights out! 
	//	basically this undoes all the work above!
		Out.col*=envAmbient.a; Out.col+=1.0-envAmbient.a; 
	
	classic_stereo(true)
	classic_aa

			//EXPERIMENTAL
			#ifdef DMPX_BALANCE
		//	Out.col.rgb*=DMPX_BALANCE; 
			#endif
			#ifdef DAMBIENT2
			Out.col.rgb = lerp(vec3(0.5),Out.col.rgb,bmpAmbient.rgb)-(vec3(1.0)-bmpAmbient.rgb)*0.25;
			#endif

		Out.uv0.a = matEmitted.a; //white ghost?

		OUTPUT(Out); //return Out; 
	}),
	GLSL(void main() //blended() FRAGMENT SHADER
	{
		CLASSIC_INPUT In = INPUT;

	classic_stereo //NOP?

		CLASSIC_OUTPUT Out; 

	classic_aa(1)
	classic_colorkey
	classic_gamma
	
		Out.col = lerp(Out.col*In.col,Out.col+In.col,colFunction);
						
		#ifndef DIN_EDITOR_WINDOW
	classic_rangefog
	classic_correct	
	classic_z 
		Out.z.r*=skyRegister.z; //1 or -1 if NPC (for shadows)	
		#else
		Out.z = vec4(0.0); //compiler
		#endif		
	
//2022: MDO textures are black???
//Out.col = tex2D(sam0,In.uv0.xy);
//Out.col.rg = In.uv0.xy;

		//2023: fade monsters to white on death
		float1 gray = dot(Out.col.rgb,float3(0.222,0.707,0.071));		
		Out.col.rgb = lerp(Out.col.rgb,gray.rrr,In.uv0.a)+In.uv0.a*0.5;

		OUTPUT(Out); //return Out;
	})
};

static const char *classic_backdrop[2] = //GLSL
{	
	GLSL(void main() //backdrop() VERTEX SHADER
	{
		CLASSIC_INPUT In = LIT_INPUT; //const

		CLASSIC_OUTPUT Out;

		Out.pos = In.pos;
		#ifdef DSTEREO
		//HACK: somehow depth must be canceled
		/*this can interact poorly with the bob/walk effect
		Out.pos.xyz = normalize(Out.pos.xyz)*(Xr_vpR.y-1.0);
		//this expects som.scene.cpp to set xr_depth_clamp*/
		Out.pos.xyz*=100;
		#endif

		/*OpenXR?
		//REMOVE ME
		//not worth maintaining WVP just for this purpose
		//Out.pos = mul(x4mWVP,In.pos);
		Out.pos = mul(x4mWV,In.pos); //In.pos
		Out.pos = mul(x4mP,Out.pos);*/
		classic_stereo_pos(Out.pos) //OpenXR?
		Out.uv0 = float4(In.uv0.xy,0,0);

	//IMPORTANT AA IS DONE BEFORE Out.fog = Out.pos BELOW
	classic_stereo(false)
	classic_aa

		//2022: this was repurposing fog to store Out_pos
	//	Out.fog = Out.pos;
	//	Out.fog.z = (In.pos.y-skyRegister.z)*skyRegister.w;		
		Out.fog = float4(0.0,0.0,(In.pos.y-skyRegister.z)*skyRegister.w,0.0);

		Out.col = matEmitted+matDiffuse;  
	
		OUTPUT(Out); //return Out;
	}),
	GLSL(void main() //backdrop() FRAGMENT SHADER
	{
		CLASSIC_INPUT In = INPUT;

	classic_stereo //NOP?

		CLASSIC_OUTPUT Out;
	
	classic_aa(1)
	classic_colorkey
	classic_gamma //might want option to exempt sky
	
		Out.col = Out.col*In.col*(vec4(1.0)-colFunction)+ //modulate
				  Out.col*colFunction+In.col*colFunction; //add	

		float3 pos = stereo_dpos(In.pos.xy/In.pos.w);
		
		//SHADER_MODEL>=3
		//pos*=tex2D(sam2,In.vpos*rcpViewport.xy+0.0001f).x;
		pos*=tex2D(sam2,In.vpos*rcpViewport.xy).x;

		Out.col.a*=1.0-saturate((skyRegister.y-length(pos))*skyRegister.x);
		Out.col.a*=min(1.0,In.fog.z);  

	classic_correct
		
		Out.z = vec4(0.0); //compiler

		OUTPUT(Out); //return Out; 
	})
};

}//namespace som_shader_hpp

#undef float3x3
#undef float3x4
#undef float4x4
//#undef float1
//#undef float2
//#undef float3
//#undef float4
//#undef lerp
#undef saturate
#undef fmod
#undef clip
#undef sample
#undef ddx
#undef ddy
#undef tex2D
#undef mul
