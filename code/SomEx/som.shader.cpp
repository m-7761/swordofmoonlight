			  
#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

#include <string>	  

//REMINDER: The D3DX compiler limits
//#define to 1 line
#define HLSL2(x) #x
#define HLSL(...) HLSL2(__VA_ARGS__) "\n"
#define HLSL_DEFINE(x,...)\
"#define " #x " " HLSL2(__VA_ARGS__) "\n"

//I'm going to turn this on since it seems
//to make turning off supersampling pretty
//good and text legible in PSVR without SS
//(although texture AA in PSVR is extreme)
#if 1 || defined(_DEBUG)
#define SOM_SHADER_MAPPING2 1 //UNFINISHED
#else
#define SOM_SHADER_MAPPING2 0 //UNFINISHED
#endif

#include "dx.ddraw.h"

#include "Ex.ini.h"
#include "Ex.output.h"
#include "Ex.shader.h"

#include "som.state.h" //SOM::colorkey
#include "som.shader.h"

//2021: these were polluting the shaders!!
#undef min
#undef max
#include "som.shader.hpp" //OpenGL shaders
//GODDAMNIT
//GLSL won't promote float to vecN, HLSL
//won't construct a vecN from a float...
//the worst part of this is the DGAMMA_Y
//and DGAMMA_N extensions
#define HLSL_GLSL_CONV \
#define vec2(x) ((float2)x)\n\
#define vec3(x) ((float3)x)\n\
#define vec4(x) ((float4)x)\n

//NEW: general purpose register
//DFOGLINE,DSKYLINE,DSKYFLOOR,DSKYFLOOD
//pixel shaders use z to mark NPCs (w is reserved)
//NOTE: cColColorkey is no longer used except for w
//(it's being used as an extended general purpose register)
#define cSkyRegister "float4 skyRegister : register(c0);"
#define cRcpViewport "float4 rcpViewport : register(c1);"
#define cColFactors  "float4 colFactors  : register(c2);"
#define cColFunction "float4 colFunction : register(ps,c3);"	  
#define cColCorrect  "float2 colCorrect  : register(ps,c7);"
#define cColColorkey "float4 colColorkey : register(ps,c8);"
#define cVolRegister "float4 volRegister : register(ps,c10);"
#define cColColorize "float4 colColorize : register(ps,c26);"
#define cFarFrustum  "float4 farFrustum  : register(ps,c27);"
#define cTexMatrix "float4x4 x4mUV : register(c30);"

//#define cX4mWVP "float4x4 x4mWVP : register(vs,c4);" //REMOVE ME
#define cX4mWV  "float4x4 x4mWV  : register(vs,c8);"
#define cX4mW   "float4x4 x4mW   : register(vs,c12);"
#define cX4mV   "float4x4 x4mV   : register(vs,c16);"
//#define cX4mIV  "float4x4 x4mIV  : register(vs,c20);"
#define cX4mP   "float4x4 x4mP   : register(vs,c20);" //VR

#define cFogFactors "float4 fogFactors : register(c24);"
#define cFogColor   "float3 fogColor   : register(ps,c25);"
#define cMatAmbient "float4 matAmbient : register(vs,c25);"
#define cMatDiffuse "float4 matDiffuse : register(vs,c26);"
#define cMatEmitted "float4 matEmitted : register(vs,c27);"
#define cEnvAmbient "float4 envAmbient : register(vs,c28);"
#define cBmpAmbient "float4 bmpAmbient : register(vs,c29);" //c28+1

//2020: unused/experimental? (do_ambient?)
//#define cLitAmbient(n) "float3 litAmbient["#n"] : register(vs,c32);"
#define cLitDiffuse "float4 litDiffuse[16] : register(vs,c48);"
#define cLitVectors "float4 litVectors[16] : register(vs,c64);"
#define cLitFactors "float4 litFactors[16] : register(vs,c80);"

#define cMaxVS 80+16 //2021
#define cMaxPS 32+16 //2021

#define iNumLights "int numLights : register(vs,i0);"

static int som_shader_arraytable_vs[12] =
{
	12,1,'vs_c',4,  32,16,  48,16,  64,16,  80,16 //light constants
};			  
static void som_shader_setlightmax(int n)
{
	assert(n>=0&&n<=16);

	if(n<0) n = 0; if(n>16) n = 16;

	//som_shader_arraytable_vs[ 5] = n; //litAmbient array size
	som_shader_arraytable_vs[ 5] = 0; //litAmbient array size
	som_shader_arraytable_vs[ 7] = n; //litDiffuse array size
	som_shader_arraytable_vs[ 9] = n; //litVectors array size
	som_shader_arraytable_vs[11] = n; //litFactors array size

	DDRAW::maxlights = n;
}

/*This is all so to work backward to arrive at the barrel 
//shaders parameters
*/
namespace PSVR //PSVRFramework/VRVideoPlayer
{
	namespace ScreenProps
	{
		//is NEAR_DISTANCE a property of the screen??
		static const float METERS_PER_INCH = 0.0254f;
		//0.1f matches som_db.exe for what's it worth
		//(no longer, but I don't see why these would
		//matter)
		static const float NEAR_DISTANCE = 0.1f;
		//far? from what?
		//static const float FAR_DISTANCE = 100.0f;

		static const float DPI = 386.47f;
	}
	namespace LensProps
	{
		//AUTHOR SAYS THIS IS HORIZONTAL, BUT
		//SINGLE EYE OR BOTH OR IN-BETWEEN???
		//"Info used for this: https://support.google.com/cardboard/manufacturers/answer/6324808?hl=en&ref_topic=6322188 and https://github.com/borismus/webvr-boilerplate/blob/d91cc2866bd54e65d59022800f62c7e160dc9fee/src/device-info.js
		//ILD, BD and DC can be found experimentally, in the previous link are some tests describing how to check"
		float maxFov = 1.18682f; //~68 degrees (75 vertical)
		//"Distance between the lens centers, the hdm by default is configured for an IPD of 64mm, I assume this is the inter-lens distance"
		float interLensDistance = 0.0630999878f;
		//"Distance between viewer baseline and lens center in meters."
		float baselineDistance = 0.0394899882f;
		//"Distance between the lenses and the screen."
		float screenLensDistance = 0.0354f;
		//"Distortion coefficients, K1 and K2, must be tweaked"
		float distortionCoeffs[2] = { 0.22f, 0.24f }; 
	
		float distort(float radius)
		{
			float result = 1;
			float rFactor = 1;
			float rSquared = radius * radius;
		
			rFactor *= rSquared; 
			result += distortionCoeffs[0] * rFactor;
			rFactor *= rSquared;
			result += distortionCoeffs[1] * rFactor; 
			
			return radius * result;
		}
		float distortInverse(float radius)
		{
			float r0 = radius / 0.9f;
			float r1 = radius * 0.9f;
			float dr0 = radius - distort(r0);

			while (abs(r1 - r0) > 0.0001) //0.1mm
			{
				float dr1 = radius - distort(r1);
				float r2 = r1 - dr1 * ((r1 - r0) / (dr1 - dr0));
				r0 = r1;
				r1 = r2;
				dr0 = dr1;
			}
			return r1;
		}
	}
	namespace PhysProps
	{
		//"Cardboard residual data, left to test it"
		static const float bevelMeters = 0.004f; 
		//what is the context of 1920/1080 here? physical or confgured mode?
		static const float widthMeters = (ScreenProps::METERS_PER_INCH / ScreenProps::DPI) * 1920;
		static const float heightMeters = (ScreenProps::METERS_PER_INCH / ScreenProps::DPI) * 1080;		
	}

	struct Params
	{	
		float outerDist;
		float innerDist;
		float topDist;
		float bottomDist;
		float eyePosX;
		float eyePosY;
		Params() //getUndistortedParams
		{
			float sld = LensProps::screenLensDistance;	
			screenWidth = PhysProps::widthMeters / sld;
			screenHeight = PhysProps::heightMeters / sld;

			float halfLensDistance = LensProps::interLensDistance / 2 / sld;

			eyePosX = screenWidth / 2 - halfLensDistance;
			eyePosY = (LensProps::baselineDistance - PhysProps::bevelMeters) / sld;

			float viewerMax = LensProps::distortInverse(tan(LensProps::maxFov));
			outerDist = std::min(eyePosX, viewerMax);
			innerDist = std::min(halfLensDistance, viewerMax);
			bottomDist = std::min(eyePosY, viewerMax);
			topDist = std::min(screenHeight - eyePosY, viewerMax);
		}	

			//adding for Viewport to use (unsure what they mean, precisely)
			float screenWidth;
			float screenHeight;
	}pp;
	struct FieldOfView
	{
		float upRadians[2];
		float downRadians[2];
		float leftRadians[2];
		float rightRadians[2];		
		FieldOfView()
		{
			//getUndistortedFieldOfViewLeftEye
			leftRadians[0] = atan(pp.outerDist);
			rightRadians[0] = atan(pp.innerDist);
			downRadians[0] = atan(pp.bottomDist);
			upRadians[0] = atan(pp.topDist);

			//getDistortedFieldOfViewLeftEye
			float sld = LensProps::screenLensDistance;
			float outerDist = (PhysProps::widthMeters - LensProps::interLensDistance) / 2;
			float innerDist = LensProps::interLensDistance / 2;
			float bottomDist = LensProps::baselineDistance - PhysProps::bevelMeters;
			float topDist = PhysProps::heightMeters - bottomDist;
			float outerAngle = atan(LensProps::distort(outerDist / sld));
			float innerAngle = atan(LensProps::distort(innerDist / sld));
			float bottomAngle = atan(LensProps::distort(bottomDist / sld));
			float topAngle = atan(LensProps::distort(topDist / sld));		
			leftRadians[1] = std::min(outerAngle, LensProps::maxFov);
			rightRadians[1] = std::min(innerAngle, LensProps::maxFov);
			downRadians[1] = std::min(bottomAngle, LensProps::maxFov);
			upRadians[1] = std::min(topAngle, LensProps::maxFov);
		}
	}fov;
	struct Viewport 
	{
		int x,y,width,height;
		Viewport(int px1920, int py1080) //getUndistortedViewportLeftEye
		{
			float xPxPerTanAngle = px1920 / pp.screenWidth;
			float yPxPerTanAngle = py1080 / pp.screenHeight;
			x = ((pp.eyePosX - pp.outerDist) * xPxPerTanAngle)+0.5f; //round
			y = ((pp.eyePosY - pp.bottomDist) * yPxPerTanAngle)+0.5f; //round
			width = ((pp.eyePosX + pp.innerDist) * xPxPerTanAngle)+0.5f - x; //round
			height = ((pp.eyePosY + pp.topDist) * yPxPerTanAngle)+0.5f - y; //round
		}
	};
	struct BarrelShader
	{	
		float projectionLeft[4];
		float unprojectionLeft[4];
		float projectionRight[4];
		float unprojectionRight[4];		
		void init(int px=1920, int py=1080)
		{
			Viewport vp(px,py);
			float (&p)[4] = projectionLeft;
			p[0] = p[1] = 1; p[2] = p[3] = 0;
			getProjectionMatrixLeftEye(true,p[0],p[1],p[2],p[3]);
			float (&u)[4] = unprojectionLeft;
			u[0] = vp.width / float(px / 2);
			u[1] = vp.height / float(py);
			u[2] = 2 * (vp.x + vp.width / 2) / float(px / 2) - 1;
			u[3] = 2 * (vp.y + vp.height / 2) / float(py) - 1;		
			getProjectionMatrixLeftEye(false,u[0],u[1],u[2],u[3]);
			 
			//NOTE: THIS IS BIZARRE BECAUSE THESE VALUES TEND TO BE
			//-0.5, SO ADDING 1, AND NEGATING, YIELDS THE OLD VALUE
			//vec4 projectionRight = (projectionLeft+vec4(0,0,1.0,0))*vec4(1,1,-1,1);
			//vec4 unprojectionRight = (unprojectionLeft+vec4(0,0,1,0))*vec4(1,1,-1,1);
			memcpy(projectionRight,projectionLeft,sizeof(projectionLeft)*2);
			projectionRight[2]+=1; projectionRight[2]*=-1;
			unprojectionRight[2]+=1; unprojectionRight[2]*=-1;
		}
		static void getProjectionMatrixLeftEye(bool distorted, 
		float &xScale, float &yScale, float &xTrans, float &yTrans)
		{
			using namespace ScreenProps;
			float top = tan(fov.upRadians[distorted]) * NEAR_DISTANCE;
			float bottom = -tan(fov.downRadians[distorted]) * NEAR_DISTANCE;
			float left = -tan(fov.leftRadians[distorted]) * NEAR_DISTANCE;
			float right = tan(fov.rightRadians[distorted]) * NEAR_DISTANCE;

			//FAR_DISTANCE???? Luckily it's not needed here.
			//projectionMatrixToVector 
			//return glm::frustum(-left,right,-bottom,top, NEAR_DISTANCE, FAR_DISTANCE);
			//glm::vec4 vec = glm::vec4(
			//	matrix[0][0] * xScale,
			//	matrix[1][1] * yScale,
			//	matrix[2][0] - 1 - xTrans,
			//	matrix[2][1] - 1 - yTrans);
			//return vec /= 2.0f;
			xScale*=(NEAR_DISTANCE*2)/(right-left)/2;
			yScale*=(NEAR_DISTANCE*2)/(top-bottom)/2;
			xTrans = ((right+left)/(right-left)-1-xTrans)/2;
			yTrans = ((top+bottom)/(top-bottom)-1-yTrans)/2;
		}
	};
	//note these may depend on the resolution, and it seems
	//that they are identical because of default pupil dist
	//{0.423099697, 0.340755254, -0.499920547, -0.499986768}
	//{0.561066210, 0.498725533, -0.499953777, -0.499993265}
	//{0.423099697, 0.340755254, -0.500079453, -0.499986768}
	//{0.561066210, 0.498725533, -0.500046253, -0.499993265}
	//FIX? Making these square produces much better results
	//ESPECIALLY combined with dx.d3d9c.cpp matting... that
	//is not the best fix, but is acceptable for the moment	
	#define PSVR_PROJ_0 0.423099697f
	#define PSVR_PROJ_1 0.423099697f//0.340755254f
	#define PSVR_PROJ_2 -0.499920547f
	#define PSVR_PROJ_3 -0.499920547f //-0.499986768f
	#define PSVR_UNPROJ_0 (1.0f/0.561066210f) //DIV->MUL
//	#define PSVR_UNPROJ_1 (1.0f/0.498725533f) //DIV->MUL
	#define PSVR_UNPROJ_1 (1.0f/0.561066210f) //DIV->MUL
	#define PSVR_UNPROJ_2 -0.499953777f
	#define PSVR_UNPROJ_3 -0.499953777f//-0.499993265f
}

static const char *som_shader_effects_vs = //HLSL
{	
	cSkyRegister

	HLSL(
	HLSL_GLSL_CONV
	struct VS_INPUT
	{
		float4 pos : POSITION;  
		float2 uv0 : TEXCOORD0; 
		float2 uv1 : TEXCOORD1; //fallback position
	};
	struct VS_OUTPUT
	{
		float4 pos : POSITION;  
		float2 uv0 : TEXCOORD0; 
		float2 uv1 : TEXCOORD1; //fallback position
		#ifdef DSTEREO
		float1 eye : TEXCOORD2;
		#endif
	};
	VS_OUTPUT f(VS_INPUT In)
	{
		VS_OUTPUT Out; 
	
		Out.pos = In.pos; //pre-converted to clip space
		Out.uv0 = In.uv0.xy; 		
		Out.uv1 = In.uv1; 

		#ifdef DSTEREO		   
		//WARNING: THIS REQUIRES THE SECTIONS TO BE DRAWN
		//SEPARATELY, AND THERE NEEDS TO BE SPACE DOWN THE
		//MIDDLE SO THAT IT ISN'T AMBIGUOUS
		//sign(0) is 0... assuming can be a little bit off
		Out.eye = In.uv0.x<0.5f?0.00001f:-0.5f;
		#endif

		return Out; 
	})
};
static const char *som_shader_effects_ps = //HLSL 
{
	cRcpViewport
	cColCorrect
	cColColorize  
	cSkyRegister //2021: ps2ndSceneMipmapping2?

	HLSL(
	HLSL_GLSL_CONV
	sampler2D sam0:register(s0);
	#ifdef DDISSOLVE
	sampler2D sam1:register(s1);
	#else
//	#ifdef DGAMMA //OLD do_gamma
//	sampler1D sam1:register(s1);
//	#endif
	#endif
	#ifdef DDITHER
	sampler2D sam3:register(s3);
	#endif
	float4 sample(float2 uv)
	{
		#if SOM_SHADER_MAPPING2 && 3<=SHADER_MODEL
		float4 mip0 = float4(skyRegister.xy,0.0,0.0);
		float4 mip1 = float4(skyRegister.zw,0.0,0.0);
		#else
		float4 mip0 = vec4(0.0), mip1 = vec4(0.0);
		#endif

		#ifdef DMIPTARGET //PSVR
			//Reminder: tex2Dlod is Shader Model 3
		//2021: tex2Dgrad solution:
		//http://gamedev.stackexchange.com/questions/101198/
			#if !SOM_SHADER_MAPPING2
			float4 mip = float4(uv,0,DMIPTARGET);
			float4 sum = tex2Dlod(sam0,mip); 
				#ifdef DDISSOLVE 
				sum+=tex2Dlod(sam1,mip); 
				#endif	
			#else //EXPERIMENTAL
			float4 sum = lerp(tex2Dlod(sam0,float4(uv,0.0,0.0)),
							  tex2Dlod(sam0,float4(mip0.xy+uv,0.0,1.0)),DMIPTARGET); 
				#ifdef DDISSOLVE
				sum+=lerp(tex2Dlod(sam1,float4(uv,0.0,0.0)),
						  tex2Dlod(sam1,float4(mip1.xy+uv,0.0,1.0)),DMIPTARGET); 
				#endif	
			#endif
		#elif DDISSOLVE==2 && !defined(DSTEREO) //2021 do_lap

			#ifdef DSS
			static const float W = 1.0;
			#else
			static const float W = 0.0;
			#endif

			float4 sum = float4(uv,0.0,W+1.0);
			float4 cmp0 = tex2Dlod(sam0,sum+mip0);
			float4 cmp1 = tex2Dlod(sam1,sum+mip1);
			sum.w = W;
			float4 sum0 = tex2Dlod(sam0,sum);
			float4 sum1 = tex2Dlod(sam1,sum);

			//REFERENCE
			/*sum0-sum1? was this a mistake?!
			//THE FOLLOWING CODE WAS BASED ON A MISTAKE

				sum = sum0-sum1;

			static const int P = 1; //power
			
			//separated color components?
			//NOTE: VR wouldn't be able to fuse because
			//DCA colors are nonlocal. VR is DMIPTARGET
			if(P==1) sum = abs(sum);
			else if(1) sum*=sum; 
			else sum = dot(sum,sum)*0.33333; //fused?

			//this small amount removes static feedback
			//from texture AA and animated textures. if
			//it's not subtracted it looks almost white
			//against the dark background... I expected
			//the value to be larger than 0.05~0.1 (0?)
		//	if(P==1) sum = max(sum-0.05,0);
			//want this to be as high as possible so it
			//doesn't cut into the AA effect when still
			//alpha cutouts seem hardest hit
		//	else if(0) sum = max(sum-0.000125,0);
		//	else sum = max(sum-0.00025,0); //exponentiated?

			//this is actually promising, but can ^2 be as good?
			//if(P==1) sum = pow(sum*2.5,2);
			if(P==1) sum = pow(sum*2.5,2.5);
			else if(1) sum*=3; //extrapolate? (tested with dot)
			else if(1) sum*=5; //saturate seems to be very stable

			//1.5 seems to beat 1.25
			if(P==1) sum = min(sum,1); //1
			else if(1) sum = min(sum,1); //saturated?
			*/
				//NEW FORMULA
				//this is (quick/dirty) optimized for turning off
				//do_smooth in supersampling mode
				//http://www.swordofmoonlight.net/bbs2/index.php?topic=324.msg3151#msg3151
				//sum = pow(cmp0-cmp1,2)*0.8f+0.05f;
				//WARNING: adding 0.1 is meant to soften a bit
				//to camouflage this defocusing effect because
				//it isn't applied uniformly
				//TODO: DSMOOTH
				//NOTE: tools aren't disabling do_smooth since
				//they only have two freeze frames to work with
				#if defined(DSS) && !defined(DIN_EDITOR_WINDOW)
				sum = pow(abs(cmp0-cmp1),vec4(1.5))*0.8+0.1;
				#else
				//NEEDS MORE ATTENTION
				//I've tweaked this just enough to be passable
				//it's pretty annoying on HUD elements on high
				//contrast backgrounds with texture AA
				//sum = min(1,pow(abs(cmp0-cmp1),1.5)*5);
				sum = min(vec4(1.0),pow(abs(cmp0-cmp1),vec4(1.5))*3.0);
				#endif
			
			#if 1 || !defined(DDEBUG) //TESTING
			{
				if(1) //2024: swing model afterimage?
				{
					float ai = 2-sum0.a;
					float ia = (2.0-ai)+0.4*(1.0-sum1.a); //over 1!
					ai-=0.2*(1.0-sum1.a);
					cmp0*=ai; cmp1*=ia; 
					sum0*=ai; sum1*=ia;
				//	cmp1*=sum1.a; cmp0*=sum0.a;
				//	sum1*=sum1.a; sum0*=sum0.a;
				}

				//NOTE: I think this is mathematically equivalent to
				//adding two lerp terms with or without crossing the
				//pairs i.e. lerp(sum0,cmp0) vs. lerp(sum0,cmp1)
				sum = lerp(sum0+sum1,cmp0+cmp1,sum);

				//there are dotted lines I didn't anticipate but
				//I think they should generally cancel out if not
				//moving
				if(0) sum = vec4(1.0)*abs(sum0+sum1-sum);
			}
			#elif 1
			{
				sum = sum0+sum1; //debugging
			}
			#else 
			{
				sum+=sum; //caller divides in half
			}
			#endif

		#elif defined(DSS)
			float4 sum = tex2Dlod(sam0,float4(uv,0.0,1.0));
			#ifdef DDISSOLVE //do_lap2 (formerly do_lap)
			sum+=tex2Dlod(sam1,float4(uv,0.0,1.0)); 
			#endif
		#else //basic?
			float4 sum = tex2D(sam0,uv);
			#ifdef DDISSOLVE //do_lap2 (formerly do_lap)
			sum+=tex2D(sam1,uv); 
			#endif	
		#endif
		return sum;
	})
		
	//PSVR
	HLSL( //NESTED DEFINE (#define must end with \n)
	#ifndef DSTEREO
	#define classic_stereo\n
	#else
	#define DCA/*TESTING*/\n
	#define classic_stereo uv = classic_stereo_sub(uv,In.eye);\n
	#endif
	)

	HLSL(	
	float classic_stereo_poly(float val)
	{
		float2 distortion = float2(0.22f,0.24f);
		return /*(showCenter == 1 && val < 0.00010) ? 
		10000.0 :*/ 1+(distortion.x+distortion.y*val)*val;
	}
	float2 classic_stereo_barrel(float2 v, float4 scale, float4 trans)
	{
		float2 w = (v+trans.yw)/*/*/*scale.yw; //DIVISION->MULTIPLY
		return scale.xz*(classic_stereo_poly(dot(w,w))*w)-trans.xz;
	}		
	float2 classic_stereo_sub(float2 uv, float1 eye)
	{
		//WARNING: APPEARS TO ASSUME PUPILS ARE SYMMETRICAL... WILL NEED
		//TO PASS FULL VALUES
		float4 scale = {PSVR_PROJ_0,PSVR_UNPROJ_0,  PSVR_PROJ_1,PSVR_UNPROJ_1};
		float4 trans = {PSVR_PROJ_2,PSVR_UNPROJ_2,  PSVR_PROJ_3,PSVR_UNPROJ_3};
		trans.xy = sign(eye.xx)*(trans.xy-eye.xx*2);
		float2 a = classic_stereo_barrel(float2((uv.x+eye.x)*2,uv.y),scale,trans);
		#ifdef DCA
		//assuming clipping is preferrable to doing the triple-sampling CA corrective
		//clip(a.x<0||a.x>1||a.y<0||a.y>1?-1:0);
		//HACK: DCA bleeds across the nose line... so a small mask helps to not see a
		//few pixels showing a wrapped image out of the corner of the eye
		//WHY? bleeding seems to be offcenter, favoring the left eye/side of the nose
		//clip(-step(a,0)-step(1,a));
		clip(-step(a,0.01f)-step(0.99f,a)); //NOTE: no z-test penalty
		return a*2-1;
		#else
		return float2(a.x/2-eye.x,a.y);
		#endif
	})
	
	HLSL
	(
	struct PS_INPUT
	{
		float2 uv0 : TEXCOORD0; 
		float2 pos : TEXCOORD1; //uv1
		#ifdef DSTEREO
		float1 eye : TEXCOORD2;
		#endif
	};
	struct PS_OUTPUT
	{
		float4 col:COLOR0; 
	};

	PS_OUTPUT f(PS_INPUT In)
	{
		PS_OUTPUT Out; 
		float2 uv = In.uv0;

		//doing before classic_stereo just so it doesn't have to 
		//be done with DCA corrected UVs (will probably not want
		//to use stipple with VR anyway
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
		#ifndef DCA
		Out.col = sample(uv);	
		#else	
		const float4 ca = float4 //red=1
		//(0.13*0.06,0.13*0.07 //green
		//,0.32*0.06,0.320*0.07)+1; //blue		
		(0.13*0.06,0.13*0.06 //green
		,0.32*0.06,0.320*0.06)+1; //blue		
		float4 gb =  (uv.xyxy*(ca)+1)/2; 	
		uv = (uv+1)/2; //red
		uv.x = uv.x/2-In.eye.x; 
		gb.xz = gb.xz/2-In.eye.xx;
		Out.col.r = sample(uv.xy).r;
		Out.col.g = sample(gb.xy).g;
		Out.col.b = sample(gb.zw).b;
		Out.col.a = 0;
		#endif
		Out.col.rgb*=samples;
		//http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
		#if DSRGB
			#if 0 //could 3 sqrt beat pow?
			float3 s1 = sqrt(Out.col.rgb), s2 = sqrt(s1), s3 = sqrt(s2);
		Out.col.rgb = 0.662002687*s1+0.684122060*s2-0.323583601*s3-0.0225411470*Out.col.rgb;
			#else
		Out.col.rgb = max(vec3(0),1.055*pow(Out.col.rgb,vec3(0.416666667))-0.055);
			#endif
		#endif
	
		//NEW do_gamma (King's Field II noodling)
		#if 3==DGAMMA_Y_STAGE 
		{
			//TODO: gamma_function
			//float gamma = 1.6;
			//I worked these out by hand/eye (should probably have
			//used a color matching software of some kind
			//these are all pretty good/might provide some insight
			//into what the real function is
			//the final one is more than acceptable
			//I don't understand why, however the multiply & power
			//parameters seem to work best if equal
			//THIS IS SCARY GOOD 
			//THIS IS SCARY GOOD 
			//THIS IS SCARY GOOD
			//Out.col.rgb = pow((Out.col.rgb+0.16)*1.55,1.55)-0.08;
			//TRYING TO DO A LITTLE BETTER... QUITE GOOD
			//Out.col.rgb = pow((Out.col.rgb+0.12)*1.6,1.6)-0.02;
			//BETTER
			//Out.col.rgb = pow((Out.col.rgb+0.13)*1.65,1.65)-0.04;
			//trying to remove yellow from terminal
			//LESS YELLOW, maybe more colorful sand
			//Out.col.rgb = pow((Out.col.rgb)*1.55+0.275,1.6)-0.12;
			//NEAR PERFECT
			//Out.col.rgb = pow((Out.col.rgb)*1.6+0.3,1.6)-0.14;
			//SLIGHT IMPROVEMENT (sand is more brown, less gray)
			//
			// differences at this point might have more to do
			// with lights than coloring; except the terminals
			// are still too yellow
			//
			#if 0 //textures with 7 added (whitened)
			Out.col.rgb = pow(Out.col.rgb*1.55+0.325,1.55)-0.155;
			#elif 0 //textures where black is 0 (or 8,8,8)
			//Out.col.rgb = pow(Out.col.rgb*1.55+0.38,1.55)-0.185;
			//these look like round numbers
			//looks good but doesn't darken sky enough to be blue
			//Out.col.rgb = pow(Out.col.rgb*1.5+0.5,1.5)-0.32;
			//the sky by the moon should be 0,0,21, and go as high
			//as 27 at maximum tilt
			//0.3525 makes the sky pure blue, whereas 0.35 has the
			//red/green be 1, but it doesn't change, and so it can
			//be a little brighter without banding. about 0.004 is
			//a difference of 1 pixel value
			Out.col.rgb = pow(Out.col.rgb*1.5+0.5,1.5)-0.3525;
			#else
			//D3DXMACRO doesn't seem to allow for parameters
			//Out.col.rgb = DGAMMA_Y(Out.col.rgb);
			float3 y = Out.col.rgb; y = DGAMMA_Y; Out.col.rgb = y;
			#endif
		}
		#endif	

		/*//CONTRAST?
		#ifdef DSTEREO		
		Out.col.rgb = (Out.col.rgb-0.5f)*1.1f+0.5f;
		#endif
		*/					

		//red/green/blue/black flashes
		//saturate step moved up from below
		Out.col.rgb+=colColorize.rgb; 
		Out.col.rgb	= saturate(Out.col.rgb); 
		Out.col.rgb*=1.0-colColorize.a; 
		//
		#ifdef DINVERT
		#ifdef DBRIGHTNESS
		//moved above to clamp color flashes
	//	Out.col.rgb	= saturate(Out.col.rgb); 
		float3 inv = vec3(1.0)-Out.col.rgb; 
		Out.col.rgb+=Out.col.rgb*colCorrect.y; 
		Out.col.rgb+=inv*colCorrect.x; 
		#endif
		#endif

		/*OLD: this was SOM's gamma ramp, although doing it
		//here is not the same as in the monitor and degrades
		//the 16-bit gamma table to 8-bit
		#ifdef DGAMMA 
		Out.col.r = tex1D(sam1,Out.col.r).r; 
		Out.col.g = tex1D(sam1,Out.col.g).r; 
		Out.col.b = tex1D(sam1,Out.col.b).r; 
		#endif*/
			
		//RESTORE ME (PSVR stopped working)
		/*2020: after attaching my PSVR to a PS4 (updating firmware)
		//this problem is corrected in the box (or something)
		#if 1 //TESTING/SHOULDN'T DO THIS AFTER CA CORRECTION
		#ifdef DSTEREO
		//PlayStation VR on Windows is washed out... this is to 
		//improve its color until something can be done about it
		//https://forum.unity.com/threads/saturation-shader.214470/
		//"Convert to grayscale numbers with magic luminance numbers"	
		//
		// according to https://en.wikipedia.org/wiki/Relative_luminance
		// this should be done in linear space
		//
		float1 gray = dot(Out.col.rgb,float3(0.222,0.707,0.071));
		//this can do saturation, but it's for evening out the gray
		//level down below
		#define DPSVR_SATURATION 1.2 //0
		#ifdef DPSVR_SATURATION
		Out.col.rgb = lerp(gray.rrr,Out.col.rgb,DPSVR_SATURATION);
		#endif
		#endif
		#endif*/
	
		//2020: I used to have the dither pattern centered on the
		//color by subtracting 4/255 but not doing that matches 
		//the "True Color" brightness exactly, so for artists it's
		//best to not have to split the difference between them
		//I actually think this is just more correct since 0 will
		//be black, and 1 will light up one dither pixel, and so
		//on, which seems correct
		#ifdef DDITHER	
		//0.0001f: helps Intel Iris Pro graphics
		Out.col.rgb+=tex2D(sam3,In.pos/(8.0*DDITHER)+0.0001).r/8.0; 
		#endif
		
		#ifdef DGREEN
		float3 highc = float3(31.0,63.0,31.0); 
		#else 
		float3 highc = float3(31.0,31.0,31.0); 
		#endif
	
		//NEW: 12/18/12
		#ifdef DHIGHCOLOR //#ifdef(DDITHER)
		//saturate ISN'T REQUIRED BUT CODE THAT COMES AFTER MAY
		//ASSUME SO?
		Out.col.rgb = floor(saturate(Out.col.rgb)*highc)/highc; 
		#endif		
		
		//PSVR seems to require this... otherwise blacks
		//are very green/uneven
		#if !defined(DBLACK) || defined(DSTEREO)
		//this was 8 for dither/colorkey but it seems excessively
		//bright
		//float3 ntsc = float3(8,8,8)/255;
		//1 is not enough to eliminate pure black pixels for PSVR
		//when DBRIGHTNESS is low, but 2 works on darkest setting
		//1 is not enough
		#ifdef DSTEREO 
		float3 ntsc = vec3(0.007843137254); //2/255 
		//Out.col.rgb = max(Out.col.rgb,ntsc); 
		//Out.col.rgb+=ntsc; 
		Out.col.b+=0.007843137254; //UNTESTED (PSVR) (OpenGL?)
		#else
		//2022: I'm reassessing this for my KF2 project's skydome
		//blue is the darkest component on displays, so it should
		//ensure the black pixels aren't brighter in a gradient
		//without brightening the screen too much overall. I have
		//to have some blue in my monitors personally, so I don't
		//feel it's too intrusive, and it's the smallest addition
		//Out.col.rgb = max(Out.col.rgb,0.003921568627); 
		//2022: += is probably more uniform
		//Out.col.rgb+=vec3(0.003921568627);
		Out.col.b+=0.003921568627;
		#endif
		#endif

		//RESTORE ME (PSVR stopped working)
		/*2020: after attaching my PSVR to a PS4 (updating firmware)
		//this problem is corrected in the box (or something)
		#if 1
		#ifdef DSTEREO
		//8 is very conservative. It may be somewhat green but is
		//not overly purple either
		//NOTE: 7.5f is green... purple is better than puke green
		#define DPSVR_GREENNESS 8 //10 
		#ifdef DPSVR_GREENNESS
		//NOTE: If made pure grayscale, the PSVR headset is green
		//for dark grays, but appears more even for brighter grays	
		//Out.col.rgb = lerp(Out.col.rgb,float3(1,0,1),(1-gray.r)/20);
		//green-gray: 1-pow is more purple but is less blotchy... so 
		//it's recommended, and GREENNESS should be reduced to achieve
		//an ideal gray when desaturated
		float gg =  (1-pow(gray.r,2)); //pow(1-gray.r,2)
		//DUNNO if lerp is the best approach here... this is just spitballing
		//it seems to do a good job. Can the math be simpler?
		Out.col.rgb = lerp(Out.col.rgb,float3(1,ntsc.g,1),gg*0.005*DPSVR_GREENNESS);	
		#endif
		#endif
		#endif*/			

		return Out; 
	})
};
static const char *som_shader_effects_xr = //HLSL 
{
//	cRcpViewport
//	cColCorrect
//	cColColorize  
//	cSkyRegister //2021: ps2ndSceneMipmapping2?

	HLSL(
	HLSL_GLSL_CONV
	cbuffer cb : register(b0) //ps_5_0
	{
		float4 skyRegister;	//cSkyRegister (ps2ndSceneMipmapping2?)
	//	float4 rcpViewport; //cRcpViewport
		float4 colCorrect;	//cColCorrect
		float4 colColorize;	//cColColorize
	}
	SamplerState sam0:register(s0)
	{
		Filter = MIN_MAG_MIP_LINEAR; //UNUSED
	};
	#ifdef DDISSOLVE
	SamplerState sam1:register(s1)
	{
		Filter = MIN_MAG_MIP_LINEAR; //UNUSED
	};
	#endif
	#ifdef DDITHER
	SamplerState sam3:register(s3)
	{
		Filter = MIN_MAG_MIP_POINT; //UNUSED
	};
	#endif
	)

	HLSL(	
	float4 sample(float2 uv)
	{
		#if SOM_SHADER_MAPPING2 && 3<=SHADER_MODEL
		float4 mip0 = float4(skyRegister.xy,0.0,0.0);
		float4 mip1 = float4(skyRegister.zw,0.0,0.0);
		#else
		float4 mip0 = vec4(0.0), mip1 = vec4(0.0);
		#endif

		#ifdef DMIPTARGET //PSVR?
			#if !SOM_SHADER_MAPPING2
			float4 mip = float4(uv,0,DMIPTARGET);
			float4 sum = tex2Dlod(sam0,mip); 
				#ifdef DDISSOLVE 
				sum+=tex2Dlod(sam1,mip); 
				#endif	
			#else //EXPERIMENTAL
			float4 sum = lerp(tex2Dlod(sam0,float4(uv,0.0,0.0)),
							  tex2Dlod(sam0,float4(mip0.xy+uv,0.0,1.0)),DMIPTARGET); 
				#ifdef DDISSOLVE
				sum+=lerp(tex2Dlod(sam1,float4(uv,0.0,0.0)),
						  tex2Dlod(sam1,float4(mip1.xy+uv,0.0,1.0)),DMIPTARGET); 
				#endif	
			#endif
		#elif DDISSOLVE==2 //2021 do_lap

			#ifdef DSS
			static const float W = 1.0;
			#else
			static const float W = 0.0;
			#endif

			float4 sum = float4(uv,0.0,W+1.0);
			float4 cmp0 = tex2Dlod(sam0,sum+mip0);
			float4 cmp1 = tex2Dlod(sam1,sum+mip1);
			sum.w = W;
			float4 sum0 = tex2Dlod(sam0,sum);
		//	float4 sum1 = tex2Dlod(sam1,sum);

			#if defined(DSS) && !defined(DIN_EDITOR_WINDOW)
			sum = pow(abs(cmp0-cmp1),vec4(1.5))*0.8+0.1;
			#else			
			sum = min(vec4(1.0),pow(abs(cmp0-cmp1),vec4(1.5))*3.0);
			#endif
			
			if(0) //2024: swing model afterimage? //???
			{
				float ai = 2-sum0.a;
				float ia = (2.0-ai)+0.4*(1.0-sum1.a); //over 1!
				ai-=0.2*(1.0-sum1.a);
				cmp0*=ai; cmp1*=ia; 
				sum0*=ai; sum1*=ia;
			//	cmp1*=sum1.a; cmp0*=sum0.a;
			//	sum1*=sum1.a; sum0*=sum0.a;
			}

			//DDISSOLVE is unnaceptable on WMR
//			sum = lerp(sum0+sum1,cmp0+cmp1,sum);
			sum = lerp(sum0,cmp0,sum);

		#elif defined(DSS)
			float4 sum = tex2Dlod(sam0,float4(uv,0.0,1.0));
//			#ifdef DDISSOLVE //do_lap2 (formerly do_lap)
//			sum+=tex2Dlod(sam1,float4(uv,0.0,1.0)); 
//			#endif
		#else //basic?
			float4 sum = tex2D(sam0,uv);
//			#ifdef DDISSOLVE //do_lap2 (formerly do_lap)
//			sum+=tex2D(sam1,uv); 
//			#endif	
		#endif
		return sum;
	})
	
	//DSTEREO_Y profiles?
	//REMINDER: preprocess directives can't be
	//used with #define because the require \n
	HLSL_DEFINE(HP_Reverb_G2,HP_Reverb_G2_y(y))

	HLSL
	(	
	struct ParamsLogC
	{
		float cut;
		float a, b, c, d, e, f;
	};
	static const ParamsLogC LogC =
	{
		0.011361, // cut
		5.555556, // a
		0.047996, // b
		0.244161, // c
		0.386036, // d
		5.301883, // e
		0.092819  // f
	};
	float3 LinearToLogC(float3 y)
	{
		return LogC.c*log10(LogC.a*y+LogC.b)+LogC.d;

	}
	float3 LogCToLinear(float3 y)
	{
		return (pow(10.0,(y-LogC.d)/LogC.c)-LogC.b)/LogC.a;
	}
	float3 LogCContrast(float3 y, float t) //contrast
	{
		//https://catlikecoding.com/unity/tutorials/custom-srp/color-grading/
		//y = lerp((float3)0.5,y,1.1);		
		return lerp((float3)0.4135884,y,t);
	}
	float3 LinearContrast(float3 y, float t) //contrast
	{
		return LogCToLinear(LogCContrast(LinearToLogC(y),t));
	}
	float3 LinearSaturate(float3 y, float t) //saturation
	{
		//according to https://en.wikipedia.org/wiki/Relative_luminance
		//this should be done in linear space
		float1 lum = dot(y,float3(0.222,0.707,0.071));
		return lerp(lum.rrr,y,t);
	}
	float3 LinearGamma(float3 y, float gamma) //meaningful?
	{
		y = pow(y,vec3(1.0/gamma)); //2.2
	}
	float3 HP_Reverb_G2_y(float3 y) //DSTEREO_Y?
	{			
		//this is trying to look like my monitor
		//with as few steps as possible. I figure
		//other sets have similar issues to mine

		//y*=1.3; //lighten? //1.6
			
		//float3 w = float3(1.25,1.15,1.2);
		float3 w = float3(0.05,-0.05,0);
		#ifdef DSTEREO_W
		w = DSTEREO_W;
		#endif
		y*=w+vec3(1.2);

		y = LinearContrast(y,1.05); //1.075

		//I think should come after contrast
		y = LinearSaturate(y,1.07);

		//can't go negative with gamma below
		//y = saturate(y);
		y = max((float3)0.0,y);

	//	y*=0.925; //darken?			

		//gamma (final?)
		//boost mid tones after contrast
		//y = pow(y,vec3(1.0/gamma)); //2.2
	//	y = LinearGamma(y,0.9);

	//	y*=1.075; //lighten?

		y*=0.9; //darken //0.9

		return y;
	}

	struct io
	{
		float4 sv_position:SV_POSITION;

		float2 uv0 : TEXCOORD0; 
	//	float2 pos : TEXCOORD1; //sv_position/sv_position.w?
	};
	float4 main(io i):SV_Target
	{
//		#ifdef DDISSOLVE
//		const float samples = 0.5; //int	
//		#else
		const float samples = 1.0; //int
//		#endif
		float4 o = sample(i.uv0);			
		o.rgb*=samples;

		//this can be a preset (HP_STEREO_G2) or it
		//can use the color grading functions above
		#ifdef DSTEREO_Y
		{
			float3 y = o.rgb; y = DSTEREO_Y; o.rgb = y;
		}
		#elif defined(DSTEREO_W)
		{
			float3 w = vec3(0); w = DSTEREO_W; o.rgb*=vec3(1)+w;
		}
		#endif

		//http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
		#if DSRGB
		#if 0 //could 3 sqrt beat pow?
			float3 s1 = sqrt(o.rgb), s2 = sqrt(s1), s3 = sqrt(s2);
		o.rgb = 0.662002687*s1+0.684122060*s2-0.323583601*s3-0.0225411470*o.rgb;
			#else
		o.rgb = max(vec3(0),1.055*pow(o.rgb,vec3(0.416666667))-0.055);
			#endif
		#endif
	
		//NEW do_gamma (King's Field II noodling)
		#if 3==DGAMMA_Y_STAGE 
		{
			float3 y = o.rgb; y = DGAMMA_Y; o.rgb = y;
		}
		#endif	
		
		//red/green/blue/black flashes
		//saturate step moved up from below
		o.rgb+=colColorize.rgb; 
		o.rgb = saturate(o.rgb); 
		o.rgb*=1.0-colColorize.a; 
		//
		#ifdef DINVERT
		#ifdef DBRIGHTNESS
		//moved above to clamp color flashes
		//o.rgb = saturate(o.rgb); 
		float3 inv = vec3(1.0)-o.rgb; 
		o.rgb+=o.rgb*colCorrect.y; 
		o.rgb+=inv*colCorrect.x; 
		#endif
		#endif
		
		#ifdef DDITHER //UNTESTED
		float2 i_pos = i.sv_position.xy/i.sv_position.w;
		//0.0001f: helps Intel Iris Pro graphics
		o.rgb+=tex2D(sam3,i_pos/(8.0*DDITHER)+0.0001).r/8.0; 
		#endif
		
		#ifdef DGREEN
		float3 highc = float3(31.0,63.0,31.0); 
		#else 
		float3 highc = float3(31.0,31.0,31.0); 
		#endif
	
		//NEW: 12/18/12
		#ifdef DHIGHCOLOR //#ifdef(DDITHER)
		//saturate ISN'T REQUIRED BUT CODE THAT COMES AFTER MAY
		//ASSUME SO?
		o.rgb = floor(saturate(o.rgb)*highc)/highc; 
		#endif
			
		#ifndef DBLACK
		//o.b+=0.003921568627;
		o.rgb+=0.003921568627;
		#endif

		//10.1. Swapchain Image Management
		//says DXGI_FORMAT_R8G8B8A8_UNORM_SRGB is pretty much
		//required (runtimes may not check the view's format)
		//return o; 
		return o*(o*(o*0.305306011+0.682171111)+0.012522878);
	})
};
static const char *som_shader_effects_11 = //HLSL 
{
//	cRcpViewport
//	cColCorrect
//	cColColorize  
//	cSkyRegister //2021: ps2ndSceneMipmapping2?

	HLSL(
	HLSL_GLSL_CONV
	cbuffer cb : register(b0) //ps_5_0
	{
		float4 skyRegister;	//cSkyRegister (ps2ndSceneMipmapping2?)
	//	float4 rcpViewport; //cRcpViewport
		float4 colCorrect;	//cColCorrect
		float4 colColorize;	//cColColorize
	}
	SamplerState sam0:register(s0)
	{
		Filter = MIN_MAG_MIP_LINEAR; //UNUSED
	};
	#ifdef DDISSOLVE
	SamplerState sam1:register(s1)
	{
		Filter = MIN_MAG_MIP_LINEAR; //UNUSED
	};
	#endif
	/*xr::effects needs work to allow point filter
	#ifdef DDITHER
	SamplerState sam3:register(s3)
	{
		Filter = MIN_MAG_MIP_POINT;
	};
	#endif*/
	)

	HLSL(	
	float4 sample(float2 uv)
	{
		#if SOM_SHADER_MAPPING2 && 3<=SHADER_MODEL
		float4 mip0 = float4(skyRegister.xy,0.0,0.0);
		float4 mip1 = float4(skyRegister.zw,0.0,0.0);
		#else
		float4 mip0 = vec4(0.0), mip1 = vec4(0.0);
		#endif

		#ifdef DMIPTARGET //PSVR?
			#if !SOM_SHADER_MAPPING2
			float4 mip = float4(uv,0,DMIPTARGET);
			float4 sum = tex2Dlod(sam0,mip); 
				#ifdef DDISSOLVE 
				sum+=tex2Dlod(sam1,mip); 
				#endif	
			#else //EXPERIMENTAL
			float4 sum = lerp(tex2Dlod(sam0,float4(uv,0.0,0.0)),
							  tex2Dlod(sam0,float4(mip0.xy+uv,0.0,1.0)),DMIPTARGET); 
				#ifdef DDISSOLVE
				sum+=lerp(tex2Dlod(sam1,float4(uv,0.0,0.0)),
						  tex2Dlod(sam1,float4(mip1.xy+uv,0.0,1.0)),DMIPTARGET); 
				#endif	
			#endif
		#elif DDISSOLVE==2 //2021 do_lap

			#ifdef DSS
			static const float W = 1.0;
			#else
			static const float W = 0.0;
			#endif

			float4 sum = float4(uv,0.0,W+1.0);
			float4 cmp0 = tex2Dlod(sam0,sum+mip0);
			float4 cmp1 = tex2Dlod(sam1,sum+mip1);
			sum.w = W;
			float4 sum0 = tex2Dlod(sam0,sum);
			float4 sum1 = tex2Dlod(sam1,sum);

			#if defined(DSS) && !defined(DIN_EDITOR_WINDOW)
			sum = pow(abs(cmp0-cmp1),vec4(1.5))*0.8+0.1;
			#else			
			sum = min(vec4(1.0),pow(abs(cmp0-cmp1),vec4(1.5))*3.0);
			#endif
			
			if(1) //2024: swing model afterimage?
			{
				float ai = 2-sum0.a;
				float ia = (2.0-ai)+0.4*(1.0-sum1.a); //over 1!
				ai-=0.2*(1.0-sum1.a);
				cmp0*=ai; cmp1*=ia; 
				sum0*=ai; sum1*=ia;
			}

			sum = lerp(sum0+sum1,cmp0+cmp1,sum);			

		#elif defined(DSS)
			float4 sum = tex2Dlod(sam0,float4(uv,0.0,1.0));
			#ifdef DDISSOLVE //do_lap2 (formerly do_lap)
			sum+=tex2Dlod(sam1,float4(uv,0.0,1.0)); 
			#endif
		#else //basic?
			float4 sum = tex2D(sam0,uv);
			#ifdef DDISSOLVE //do_lap2 (formerly do_lap)
			sum+=tex2D(sam1,uv); 
			#endif	
		#endif
		return sum;
	})
		
	HLSL
	(
	struct io
	{
		float4 sv_position:SV_POSITION;

		float2 uv0 : TEXCOORD0; 
	//	float2 pos : TEXCOORD1; //sv_position/sv_position.w?
	};
	float4 main(io i):SV_Target
	{
		#ifdef DDISSOLVE
		const float samples = 0.5; //int	
		#else
		const float samples = 1.0; //int
		#endif
		float4 o = sample(i.uv0);			
		o.rgb*=samples;

		//http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
		#if DSRGB
		#if 0 //could 3 sqrt beat pow?
			float3 s1 = sqrt(o.rgb), s2 = sqrt(s1), s3 = sqrt(s2);
		o.rgb = 0.662002687*s1+0.684122060*s2-0.323583601*s3-0.0225411470*o.rgb;
			#else
		o.rgb = max(vec3(0),1.055*pow(o.rgb,vec3(0.416666667))-0.055);
			#endif
		#endif
	
		//NEW do_gamma (King's Field II noodling)
		#if 3==DGAMMA_Y_STAGE 
		{
			float3 y = o.rgb; y = DGAMMA_Y; o.rgb = y;
		}
		#endif	
		
		//red/green/blue/black flashes
		//saturate step moved up from below
		o.rgb+=colColorize.rgb; 
		o.rgb = saturate(o.rgb); 
		o.rgb*=1.0-colColorize.a; 
		//
		#ifdef DINVERT
		#ifdef DBRIGHTNESS
		//moved above to clamp color flashes
		//o.rgb = saturate(o.rgb); 
		float3 inv = vec3(1.0)-o.rgb; 
		o.rgb+=o.rgb*colCorrect.y; 
		o.rgb+=inv*colCorrect.x; 
		#endif
		#endif
		
		/*xr::effects needs work to allow point filter
		#ifdef DDITHER //UNTESTED
		float2 i_pos = i.sv_position.xy/i.sv_position.w;
		//0.0001f: helps Intel Iris Pro graphics
		o.rgb+=tex2D(sam3,i_pos/(8.0*DDITHER)+0.0001).r/8.0; 
		#endif*/
		
		#ifdef DGREEN
		float3 highc = float3(31.0,63.0,31.0); 
		#else 
		float3 highc = float3(31.0,31.0,31.0); 
		#endif
	
		//NEW: 12/18/12
		#ifdef DHIGHCOLOR //#ifdef(DDITHER)
		//saturate ISN'T REQUIRED BUT CODE THAT COMES AFTER MAY
		//ASSUME SO?
		o.rgb = floor(saturate(o.rgb)*highc)/highc; 
		#endif
			
		#ifndef DBLACK
		o.b+=0.003921568627;
		#endif

		//10.1. Swapchain Image Management
		//says DXGI_FORMAT_R8G8B8A8_UNORM_SRGB is pretty much
		//required (runtimes may not check the view's format)
	//	return o; 
		return o*(o*(o*0.305306011+0.682171111)+0.012522878);
	})
};

static const char *som_shader_classic_vs = //HLSL
{
	cSkyRegister
	cRcpViewport
	cColFactors	
//	cX4mWVP
	cX4mWV
	cX4mW
	cX4mV 
	cX4mP //cX4mIV //2==EX_INI_STEREO_MODE
//	cTexMatrix
	//cFogFactors
	cMatAmbient
	cMatDiffuse
	cMatEmitted
	cEnvAmbient
	cBmpAmbient
	//cLitAmbient(16)
	cLitDiffuse//(16)
	cLitVectors//(16)
	cLitFactors//(16)
	iNumLights	

	HLSL(
	HLSL_GLSL_CONV)

	#if 0 //REFERENCE
	//RETIRED
	//(classic_stereo_pos sets fog now)
	//subroutines
	HLSL_DEFINE(classic_z,
	//Out.fog = classic_z_sub(In.pos);
	)HLSL(
	float4 classic_z_sub(float4 In_pos)
	{
		//assuming per-pixel fog??
		float4 Out_fog = mul(x4mWV,In_pos);
		/*REFERENCE	
		#ifdef (DRANGEFOG)
		float Z = length(mul(x4mWV,In_pos)); 
		#else
		float Z = mul(x4mWV,In_pos).z; 
		#endif ENDIF
		Out.fog.x = 
		(fogFactors.y-Z)*fogFactors.x; 
	//	#ifdef DFOGLINE 
		Out.fog.y = (skyRegister.y-Z)*skyRegister.x; 
	//	#else
	//	Out.fog.y = 1.0f; 
	//	#endif
		Out_fog = saturate(Out_fog);*/
		return Out_fog;
	})
	#endif

	HLSL_DEFINE(classic_aa,	
	Out.pos = classic_aa_sub(Out.pos);
	)HLSL(
	float4 classic_aa_sub(float4 io) //Out_pos
	{
		//REMINDER (VR)
		//THIS IS A CONCEPT THAT COULD FULLY LEVERAGE DO_AA IN VR
		//UNFORTUNATELY IT'S NOT COMPATIBLE WITH OPEXR'S WORKFLOW
		//https://www.gamedeveloper.com/programming/vr-distortion-correction-using-vertex-displacement

	#ifdef DAA
		
		//THIS IS CHANGING TEXTURE AA DEPENDING ON IF A
		//MENU IS OPEN??? Not DDRAW::fxCounterMotion???
		//return io;

		//REMINDER: I think this is snapping to 1/2
		//pixel units

		float4 hvp = rcpViewport*io.w; 
		//FYI: this line is not strictly necessary
		//when at "full power" with a motion blur
		//(2020: it seems more stable... omitting
		//it doesn't reduce crawling edges) 
		io.xy-=fmod(io.xy,hvp.xy);
	
		//WARNING
		//now (after removing some code) enabling this
		//causes blurriness... have a feeling blurring
		//is more correct (but blurry)
		if(0)
		{
			//EXTENSION?
			//2020: adjusting by a half pixel was a 
			//big win for sharpness
			//2021: I'm no longer seeing a difference
			//(with texture AA and sRGB disabled) I've
			//changed classic_aa some, maybe this was
			//counteracting another issue??? I don't
			//actually know if centers are at 0.5 or 0
			//so this is just trial and error. turning
			//off to reduce instructions
			//TODO: SET UP A DYNAMIC TEST
			//
			// Update: without the magic gauge started
			// twitching at some resolution	in a way I
			// never saw before (1280x960)
			//
			// Notice: -= made the gauge height differ
			// I feel like both of these could be down
			// to chance
			//
			io.xy+=hvp.xy*0.5; //-=
		}

		//add negative/postive "dejag" offset
		io.xy+=hvp.zw;
	#endif
		return io;
	})

	HLSL( //NESTED DEFINE (#define must end with \n)
	#define classic_stereo_pos1(x) Out.fog = mul(x4mWV,x);\n
	#ifndef DSTEREO
	#define classic_stereo(scopic)\n
	#define classic_stereo_DEPTH\n
	#define classic_stereo_pos(x) \
		classic_stereo_pos1(x) Out.pos = mul(x4mP,Out.fog);\n
	#else
	#define classic_stereo_DEPTH float2 stereo:DEPTH;\n
	#define classic_stereo(scopic) \
	Out.stereo = In.stereo.x;\
	Out.pos = classic_stereo_sub(Out.pos,In.stereo.x/*,scopic*/);\n
	#define classic_stereo_pos(In_pos) \
	classic_stereo_pos1(In_pos) \
	Out.pos = classic_stereo_pos2(Out.fog,In.stereo);\n
	#endif
	)

	HLSL(
	float4 classic_stereo_sub(float4 io, float stereo/*, bool scopic*/) //Out_pos
	{
	#ifdef DSTEREO	
	//#if 2!=EX_INI_STEREO_MODE
	//	if(scopic) io.x+=stereo;
	//#endif	
		io.x-=float(sign(stereo))/2*io.w;
	#endif
		return io; //Out_pos
	}
	float4 classic_stereo_pos2(float4 io, float2 stereo) //Out_pos
	{	
	//#if 2!=EX_INI_STEREO_MODE
	//	io = mul(x4mWVP,io) //same as without DSTEREO
	//#else
		io.x+=stereo.x;
		#if 1 || !defined(DDEBUG)
		io = mul(x4mP,io);
		#elif 1
			/*
		//there are various ways to do this but they all seem
		//to introduce parallax turning/looking
		float scale = 1-abs(stereo.x)*10;//20;
		float4 row1 = x4mP[0]; row1.x*=scale;
		float4 row2 = x4mP[1]; row2.y*=scale;
		//by changing the depth of the perspective I feel like 
		//I'm the right height and the world isn't foreshortened
		//but turning/looking seems to shift the perspective
		//float scale = 1+abs(stereo.x)*20;
		float4 row3 = x4mP[2]; row3.z*=scale;
		float4 row4 = x4mP[3];// row4.z*=scale;
		//io = float4(dot(io,x4mP[0]),dot(io,x4mP[1]),dot(io,row3),dot(io,row4));
		io = float4(dot(io,row1),dot(io,row2),dot(io,row3),dot(io,row4));
			*/

			//REFERENCE/REMOVE ME
			//this is most promising... 5 helps a lot... 10 throws
			//off the perspective... maybe the FOV needs increasing
			//from 94 to compensate? 
			io = mul(x4mP,io);
			//io.xyz*=1-abs(stereo.x)*10;
			//io.xyz*=1-abs(stereo.x)*20;
			io.xyz*=1-abs(stereo.x)*5;
			//even a little bit going up gets cut off by the far plane
			//io.xyz*=1+0.0001; //10
			//io.xy*=1+abs(stereo.x)*10;

		#else //REFERENCE
		//computing skewed matrix
		//https://web.archive.org/web/20170924184212/http://doc-ok.org/?p=77
		//io = mul(x4mP,io);
		//all of these work, but which is fewer operations who can say?
		//note: nonzero: _11,22,31,33,43; one: _34
		float4 row1 = x4mP[0]; row1.z = stereo.y;
		io = float4(dot(io,row1),dot(io,x4mP[1]),dot(io,x4mP[2]),dot(io,x4mP[3]));
		//working out matrix layout
		//io.w = io.z;
		//io.x = x4mP[0].x*io.x+io.z*stereo.y;				
		//io.z = x4mP[2].z*io.z+x4mP[2].w;
		//io.y*=x4mP[1].y;		
		//straightforward?
		//io = float4
		//(x4mP[0].x*io.x+io.z*stereo.y
		//,x4mP[1].y*io.y
		//,x4mP[2].z*io.z+x4mP[2].w,		
		//io.z);
		//single mad instruction?
		//io = io.xyzz*float4(x4mP[0].x,x4mP[1].y,x4mP[2].z,1)+float4(io.z*stereo.y,0,x4mP[2].w,0);
		#endif
	//#endif
		return io;
	})
	
		//HACK: this is just for DGAMMA_N to use if wants
	HLSL(float3 rotate_y(float a, float3 n)
	{
		float1 s,c; sincos(a,s,c); //weird
		//return float3(dot(n,float3(cos(a),0,sin(a))),n.y,dot(n,float3(-sin(a),0,cos(a))));
		return float3(dot(n,float3(c,0,s)),n.y,dot(n,float3(-s,0,c)));
	})

	HLSL(
	struct FOG_INPUT
	{
		float4 pos : POSITION;
		float2 uv0 : TEXCOORD;
		classic_stereo_DEPTH
	};	
	struct LIT_INPUT
	{
		float4 pos : POSITION; 
		float3 lit : NORMAL;   
		classic_stereo_DEPTH 
		float2 uv0 : TEXCOORD;
	};
	struct UNLIT_INPUT
	{
		float4 pos : POSITION; 
		float4 col : COLOR;
		float2 uv0 : TEXCOORD0;
		classic_stereo_DEPTH
	};	
	struct CLASSIC_OUTPUT
	{
		float4 pos : POSITION;  
		float4 col : COLOR;     
		float4 uv0 : TEXCOORD0;
		classic_stereo_DEPTH 
		float4 fog : TEXCOORD1; 
	};	 
	CLASSIC_OUTPUT blit(UNLIT_INPUT In)
	{
		CLASSIC_OUTPUT Out; 

	//conversion to clip-space 
	//	Out.pos.x = In.pos.x/rcpViewport.x*2.0f-1.0; 
	//	Out.pos.y = 1.0-In.pos.y/rcpViewport.y*2.0f; 	
	//	Out.pos.z = 0.0; Out.pos.w = 1.0f; 

	//currently pre-converting to clip-space
	//projection space if fvfFactors.w is 0, screen space if 1
	//	Out.pos	= In.pos*fvfFactors.w+  //Out.pos
	//	mul(x4mWVP,In.pos)*(1.0f-fvfFactors.w); 

	//fyi: sprites moved to sprite()
		Out.pos	= In.pos; 	
		#ifdef DSTEREO
		Out.pos.x+=In.stereo.x*EX_INI_MENUDISTANCE;
		#endif
	
		Out.col = In.col; 
		Out.uv0 = float4(In.uv0.xy,0,0); 
		Out.fog = float4(0,0,0,0); //warning

		Out.col+=colFactors; //select texture

		Out.col = saturate(Out.col); 

	classic_stereo(!false)

	classic_aa //NEW: helps map edges... doesn't seem to hurt

		return Out; 
	}						                     
	CLASSIC_OUTPUT fog(FOG_INPUT In)
	{
		CLASSIC_OUTPUT Out; 

		//Out.pos = mul(x4mWVP,In.pos); 	
		classic_stereo_pos(In.pos)
		Out.col = vec4(0.0);
		Out.uv0 = float4(In.uv0.xy,0,0);

//	classic_z  		
	classic_stereo(true)
	classic_aa

		return Out; 
	} 
	CLASSIC_OUTPUT unlit(UNLIT_INPUT In)
	{
		CLASSIC_OUTPUT Out; 

		//Out.pos = mul(x4mWVP,In.pos); 	
		classic_stereo_pos(In.pos)
		Out.col = In.col;  
		Out.uv0 = float4(In.uv0.xy,0,0);

//	classic_z  		
	classic_stereo(true)
	classic_aa

		/*EXPERIMENTAL
		//try to match map geometry color
		//seems to work... might do it in unlit instead?
		Out.col*=DMPX_BALANCE;
		precision_snap(Out.col,255)*/ //REMOVED
		//Out.col/=DMPX_BALANCE;

		return Out; 
	} 
	CLASSIC_OUTPUT sprite(UNLIT_INPUT In)
	{
		CLASSIC_OUTPUT Out; 

		//Out.pos = mul(x4mWVP,In.pos); 
		classic_stereo_pos(In.pos)
		Out.col = In.col; 
		Out.uv0 = float4(In.uv0.xy,0,0);
	
//	classic_z //Out.fog = 0.0;	   	
	classic_stereo(true)
	classic_aa
		
		Out.col+=colFactors; //select texture

		Out.col = saturate(Out.col); 

		return Out; 
	}			
	struct SHADOW_INPUT
	{
		float4 pos : POSITION; 
		float4 col : COLOR;
		float4 center : TEXCOORD0; 
		float4 xforms : TEXCOORD1;
		classic_stereo_DEPTH
	};	
	struct CLASSIC_SHADOW
	{
		float4 pos : POSITION;  
		float4 col : COLOR0;     
		float4 fog : COLOR1;
		float4 xforms : COLOR2;
	float4x4 mat : TEXCOORD0; //perspective corrected 
		classic_stereo_DEPTH
	};
	//for projected shadows only
	CLASSIC_SHADOW shadow(SHADOW_INPUT In)
	{
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
							  
		Out.fog = Out.pos;
		Out.fog.z = 0.5/(Out.xforms.z/EX_INI_SHADOWUVBIAS);		
		
		//REFERENCE (x4mUV)
		//REMINDER: this code was based on a technique which
		//projects geometry onto planes to make flat shadows
		// 
		// REMINDER! these can't be passed with a constant
		// (pixel shader) because of perspective correction
		// 
		// Out.mat = x4mUV;
		Out.mat = transpose(x4mV);
		center = mul(x4mV,center);
		for(int i=0;i<3;i++)
		Out.mat[i].w = -dot(center.xyz,Out.mat[i].xyz);		

		return Out;	
	}					

	struct LIGHT
	{
		float3 /*ambient,*/ diffuse; 
	};	
	LIGHT Light(int i, float3 P, float3 N)
	{	
		float3 D = litVectors[i].xyz-P; float M = length(D); 

		//Note: assuming lights are in range (no longer)
		#if SHADER_MODEL<=2 //2021
		if((litFactors[i].x-M)*litVectors[i].w<0.0) return (LIGHT)0; //Model 2.0	
	//	float att = saturate(ceil(litFactors[i].x-M*litFactors[i].w)); //Model 1.0 
		#endif

		float att = 1.0f;  
		att/=litFactors[i].y+litFactors[i].z*M+litFactors[i].w*M*M; 
			
		float3 L = normalize(D*litVectors[i].w + //point light
		-litVectors[i].xyz*(1.0f-litVectors[i].w)); //directional 

		LIGHT Out;
		//Out.ambient = litAmbient[i].rgb*att;
		Out.diffuse = litDiffuse[i].rgb*att*max(0.0,dot(N,L));   
		return Out;		
	}
	CLASSIC_OUTPUT blended(LIT_INPUT In)
	{
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
					
		//OBSOLETE
		#ifdef DLIGHTS //vs_1_x
		for(int i=0;i<DLIGHTS;i++) 
		#else //loop: want dynamic flow control
		[loop] for(int i=0;i<numLights;i++) 
		#endif
		{	LIGHT sum = Light(i,P,N); 	
		//	ambient.rgb+=sum.ambient; 
			diffuse.rgb+=sum.diffuse; 
		}

		//Out.col = matEmitted;  

		//DOES SOM HAVE matAmbient?

		#ifdef DHDR //clamp global ambient/emissive	
		Out.col.rgb+=matAmbient.rgb*envAmbient.rgb; 
		Out.col.rgb = saturate(Out.col.rgb); 
		#else
		ambient.rgb+=envAmbient.rgb; 		
		#endif
		Out.col+=matAmbient*ambient+matDiffuse*diffuse; 

	//	Out.col+=colFactors; //select texture 

		#ifdef DHDR
	//  float4 invFactors = float4(1,1,1,1)-colFactors; 
	//	Out.col = Out.col*invFactors+saturate(Out.col)*colFactors;  
		#else
		Out.col = saturate(Out.col); 
		#endif
	 	
	//lights out! 
	//	basically this undoes all the work above!
		Out.col*=envAmbient.a; Out.col+=1.0-envAmbient.a;
	
	classic_stereo(true)
	classic_aa

			//EXPERIMENTAL
			#ifdef DMPX_BALANCE
			//try to match map geometry color
			//seems to help... might do it in unlit instead?
			//(seems not to work so well there)
			//THIS MAY BE DIFFERENT UNDER DIFFERENT LIGHTING
			//TODO: investigate MapComp
			//somewhere between 0.963 and around 0.977 should
			//be ideal, dither patterns are the best way to 
			//gauge accuracy. 0.97 seems best yet under a few
			//different lighting conditions. generally it is
			//invariant to lighting
			//NEAR PERFECT? this makes the secret doors very 
			//near perfect in my KF2 project
			Out.col.rgb*=DMPX_BALANCE; 
			//I thought maybe this would fix it by itself but
			//since darkening is more important I thought this
			//might be less so, but it seems necessary to get
			//ditter patterns to match (and it might be needed
			//even if MapComp is fixed)
			//precision_snap(Out.col,255) //REMOVED
		//	Out.col.rgb-=fmod(Out.col.rgb,0.003921568627);
			#endif

			#ifdef DAMBIENT2			
			//DUPLICATE 
			//should match som_MPX_411a20
			//Out.col.rgb*=pow(bmpAmbient.rgb,Out.col.rgb*4);
			Out.col.rgb = lerp(vec3(0.5),Out.col.rgb,bmpAmbient.rgb)-(vec3(1)-bmpAmbient.rgb)*0.25;
			//EXPERIMENTAL
			#ifdef DMPX_BALANCE
		//	Out.col.rgb-=fmod(Out.col.rgb,0.003921568627);
			#endif
			#endif

			Out.uv0.a = matEmitted.a; //white ghost?

		return Out; 
	}
	CLASSIC_OUTPUT backdrop(LIT_INPUT In)
	{
		CLASSIC_OUTPUT Out; 				   
		
		//REMOVE ME
		//not worth maintaining WVP just for this purpose
		//Out.pos = mul(x4mWVP,In.pos); 
		Out.pos = mul(x4mWV,In.pos); 
		Out.pos = mul(x4mP,Out.pos); 
		Out.uv0 = float4(In.uv0.xy,0,0);

	//IMPORTANT AA IS DONE BEFORE Out.fog = Out.pos BELOW	
	classic_stereo(false)
	classic_aa

		//trying to use viewport
//		#ifdef DFOGLINE
//		Out.pos.z = DFOGLINE*fogFactors.y*Out.pos.w-0.000001f; 
//		#else
//		Out.pos.z = Out.pos.w-0.000001f; 
//		#endif
		Out.fog = Out.pos; 
//		#ifdef DSKYFLOOD
// 	    //2022: saturate here distorts the blending... it seems safe
// 	    //to let the pixel shader output be unsaturated
		//Out.fog.z = saturate((In.pos.y-skyRegister.z)*skyRegister.w); 
		Out.fog.z = (In.pos.y-skyRegister.z)*skyRegister.w; 
//		#else
//		Out.fog.z = 1.0f; 
//		#endif	
		Out.col = matEmitted+matDiffuse;  
		//DHDR?
//		Out.col = saturate(Out.col); 	
	
		return Out; 
	})
};
static const char *som_shader_classic_ps = //HLSL 
{
	cSkyRegister
	cVolRegister //2023: psConstants10 
	cRcpViewport
	cFogColor
	cFogFactors 
	cColFactors
	cColFunction
	cColCorrect
	cColColorkey
	cFarFrustum
//	cTexMatrix //x4mUV
	HLSL(
	sampler2D sam0:register(s0);
	sampler2D sam2:register(s2) = sampler_state //MRT
	{ MinFilter = Point; MagFilter = Point; };) //testing

	HLSL(
	HLSL_GLSL_CONV)

	//subroutines
	HLSL_DEFINE(classic_colorkey,
	Out.col = classic_colorkey_sub(Out.col); //In.uv
	)HLSL(
	float4 classic_colorkey_sub(float4 io) //Out_col
	{
		//this is actually a useful way to see the games
		//without textures
		//return float4(io.aaa,1);
		#ifdef DCOLORKEY
		#if 2000==DCOLORKEY //shader model 2
			#ifndef TEXTURE0_NOCLIP //dx.d3d9X.cpp
			//NOCLIP should make colColorkey.w obsolete
			//clip(io.a-0.3f*colColorkey.w); 		
			clip(io.a-0.3f); 		
			//float4 ck = float4(io.rgb/io.a,1); 
			//reminder: colColorkey.w is 0 or 1 
			//io = lerp(io,ck,colColorkey.w);
			io.rgb = io.rgb/io.a; 
			#endif		
		#else
		#if 3>SHADER_MODEL
		//2020: volume() repurposes colColorkey.w? and the
		//shadow() shader doesn't require it here?
		//
		// NOTE: I think [Keygen] (IMAGES.INI) had relied 
		// on this. I must have forgotten about it
		//
	//	if(colColorkey.w) //NOCLIP? 
		#endif
		{
				//REMOVE ME
				//this incurs a big penalty with z-testing
				//ideally it would be limited to textures
				//with holes in them, but that's hard since
				//that would mean 2 sets of shaders (I plan
				//to work on it)

			#ifndef TEXTURE0_NOCLIP //dx.d3d9X.cpp
			//this doesn't work because SOM::colorkey fills
			//the empty pixels with neighboring color
			//clip(io.a+1.5f-colColorkey.w);
			clip(io.a-0.5f);
			#endif			
		}
		io.a = 1.0; //GLSL
		#endif
		#endif
		return io;
	})
	HLSL_DEFINE(classic_correct,
	Out.col.rgb = classic_correct_sub(Out.col.rgb);
	)HLSL(
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
	HLSL_DEFINE(classic_inverse,
	Out.col = classic_inverse_sub(Out.col);
	)HLSL(
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
	HLSL_DEFINE(classic_rangefog, 
	float3 fog = classic_rangefog_sub(In.fog).xyz;
	//FIX ME: pretty sure this is wrong for some blending
	//functions
	//reminder: fogging a brings out the edges of shadows	
	Out.col.rgb = lerp(fogColor,Out.col.rgb,fog.x);
	Out.col.a*=fog.y;)HLSL(
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
	HLSL_DEFINE(classic_z,
	//note, could swizzle all into rgba but theoretically these
	//could affect blending even though my hardware doesn't let
	//MRT output blend at all
	Out.z = float4(In.fog.z*farFrustum.w,0.0,0.0,1.0);) 	
	
	/*SetScissorRect performs better than instancing
	//with rolling buffers (see dx_d3d9c_drawprims2)
	HLSL( //NESTED DEFINE (#define must end with \n)
	#ifndef DSTEREO
	#define classic_stereo\n
	#else
	#define classic_stereo clip((0.5f-In.vpos.x*rcpViewport.x)*In.stereo);\n
	#endif
	)*/
	HLSL_DEFINE(classic_stereo) //NOP

	HLSL( //NESTED DEFINE (#define must end with \n)
	#if 1!=DGAMMA_Y_STAGE
	#define classic_gamma\n
	#else
	#define classic_gamma \
	{ float3 y = Out.col.rgb; y = DGAMMA_Y; Out.col.rgb = y; }\n
	#endif
	)
	
	HLSL(
	float4 sample(float2 uv)
	{		
		#ifdef DIN_EDITOR_WINDOW //2021
		float4 o = tex2Dbias(sam0,float4(uv,0,-1));
		#elif 0 && defined(DAA) && defined(DSRGB) && defined(DDEBUG)
		//
		// shimmering is less with sRGB but I don't know if going
		// down to 0.2 or so makes enough of a difference to make
		// this a thing
		//
		float4 o = tex2Dbias(sam0,float4(uv,0,-0.2)); //-0.3
		#else
		float4 o = tex2D(sam0,uv);  
		#endif
		//http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
		#if DSRGB
			#if 0 //could 3 sqrt beat pow?
			float3 s1 = sqrt(o.rgb), s2 = sqrt(s1), s3 = sqrt(s2);
		o.rgb = 0.662002687*s1+0.684122060*s2-0.323583601*s3-0.0225411470*o.rgb;
			#else
		o.rgb = max(vec3(0),1.055*pow(o.rgb,vec3(0.416666667))-0.055);
			#endif
		#endif
		return o;
	}
	float4 classic_aa_sub(float2 uv, float blit=1)
	{
		#if defined(DAA) //&& !defined(DDEBUG)
		float2 dx = ddx(uv)*rcpViewport.z*blit;
		float2 dy = ddy(uv)*rcpViewport.w*blit;
		return sample(uv+dx+dy);
		#else
		return sample(uv);
		#endif
	})
	//note, unrelated to the vertex shader classic_aa
	HLSL_DEFINE(classic_aa(blit),
	Out.col = classic_aa_sub(In.uv0.xy,float(blit));)

	//2021: adding DUV requirement for Shader Model 2
	HLSL(
	#ifdef DUV
	float4 sample1(float2 uv)
	{
		//TODO: DUV should just be a real-time constant
		//DUV transforms uv... it can't use parameters!
		uv = uv*float2(0.5,-0.5)+0.5;
		return tex2D(sam2,DUV+0.0001f);
	}
	#endif
	)

	HLSL(
	struct CLASSIC_INPUT
	{
		float4 col : COLOR0;    
		float4 uv0 : TEXCOORD0; 
		float4 fog : TEXCOORD1; //4 is for shadow? 
		#ifdef DSTEREO
		float stereo : DEPTH;
		#endif
		#if 3<=SHADER_MODEL
		float2 vpos : VPOS; //volume
		#endif
	};		
	struct CLASSIC_OUTPUT //MRT
	{
		float4 col:COLOR0, z:COLOR1; 
	};
	CLASSIC_OUTPUT blit(CLASSIC_INPUT In)
	{					 
	classic_stereo //NOP? 

		CLASSIC_OUTPUT Out; 

		//sample?
		//Out.col = tex2D(sam0,In.uv0.xy);  
		/*April 2021: feel like needs to tone down after changing back to
		//blurry mode
		classic_aa(0.8)*/
		#ifdef DSS
		classic_aa(0.8)
		#else
		classic_aa(0.2) //icons are too blurry
		#endif

	//alpha unreliable??? //maybe unnecessary??
	//	Out.col = saturate(Out.col+colFactors);  

	classic_colorkey
	//classic_gamma //optional?
	
		Out.col*=In.col;  

	classic_inverse

	//	Out.col.a = 0.5f; //debugging

		Out.z = vec4(0.0); //compiler

		return Out; 
	}	
	CLASSIC_OUTPUT fog(CLASSIC_INPUT In)
	{	
	classic_stereo //NOP?

		CLASSIC_OUTPUT Out;

		#ifndef TEXTURE0_NOCLIP
	classic_aa(1)
		#else
		//classic_colorkey sets a and rgb is
		//set to fogColor below
		#endif

		//NOTE: fog saturation happens after the color
		//correction functions, so they can be skipped
		//NOTE: volume-texture depth-write shares this
		//shader. it's currently not even used for fog
		//(IT WOULDN'T BE HARD TO DO EVERYTHING EXCEPT
		//TILES ARE DRAWN IN CHUNKS. I INTEND TO WRITE
		//SOME TILE DRAWING CODE SOMETIME-NOW IS 2022)

	classic_colorkey //clip

		Out.col.rgb = fogColor;

	classic_z //depth write

		return Out; 			 
	}
	CLASSIC_OUTPUT unlit(CLASSIC_INPUT In)
	{	
	classic_stereo //NOP?

		CLASSIC_OUTPUT Out; 

	classic_aa(1)
	classic_colorkey
	classic_gamma
	
		Out.col*=In.col; 					

		#ifndef DIN_EDITOR_WINDOW
	classic_rangefog

			/*doesn't quite work
			//claw back brightness?
			Out.col.rgb/=DMPX_BALANCE;*/
			
	classic_correct
	classic_z	
		#else
		Out.z = vec4(0.0); //compiler
		#endif

		return Out; 			 
	}	
	#if 3<=SHADER_MODEL
	CLASSIC_OUTPUT volume(CLASSIC_INPUT In)
	{
		CLASSIC_OUTPUT Out = unlit(In);

		//Shader Model 2 can't use In.fog like the shadow/sky shader
		//unless a new input parameter is added
		float2 vp = In.vpos*rcpViewport.xy;
		//SPECIAL NOTE
		//
		// the others use In.fog.xy/In.fog.w, but that doesn't seem
		// to fly here (actually see note just above) but this is a
		// problem in the GL shader, only because 2,-2 must be 2,+2
		//
		float2 uv = (vp-0.5f)*float2(2,-2); 
		float3 pos = farFrustum.xyz; 
		#ifndef DSTEREO
		pos.xy*=uv;
		#else
		pos.xy*=float2(uv.x+0.5f*sign(In.stereo),uv.y);
		#endif
		//NOTE: sample1 isn't require in this case
		//NOTE: abs is because NPCs/monsters don't cast shadows
		pos*=abs(tex2D(sam2,vp+0.0001f).x);
		float depth = volRegister.z; //skyRegister
		float power = volRegister.w; //skyRegister
		float alpha = pow(length(pos-In.fog.xyz)*depth,power);
		
		//TODO? conditionally compile this
		//2020 Feb.
		//this is a kind of depth fog effect added to recreate the dark blue
		//color of KF2's opening area.
		//is colColorkey available for repurposing?
		#if 2000!=DCOLORKEY
		alpha = saturate(alpha);
		Out.col.rgb = lerp(Out.col.rgb,colColorkey.rgb,alpha*colColorkey.w);
		#endif
		//FIX ME!
		//I think something like this is needed for alpha-fog? but it's not
		//working at the moment... where is the existing alpha coming from?
		//Out.col.a*=alpha;
		Out.col.a = alpha; //2023: assume opaque? e.g. KF2 water

		//2023: fade monsters to white on death
		float1 gray = dot(Out.col.rgb,float3(0.222,0.707,0.071));		
		Out.col.rgb = lerp(Out.col.rgb,gray.rrr,In.uv0.a)+In.uv0.a*0.5;
		//HACK: sqrt(fade)->fade*fade
		Out.col.a*=pow(1-In.uv0.a,3);

		return Out;
	}
	#endif
	CLASSIC_OUTPUT sprite(CLASSIC_INPUT In)
	{	
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

		return Out; 
	} 	
	struct CLASSIC_SHADOW
	{
		float4 col : COLOR0;     		
		float4 fog : COLOR1; 
		float4 xforms : COLOR2; 
	float4x4 mat : TEXCOORD0; //perspective corrected 
		#ifdef DSTEREO		
		float stereo : DEPTH; //needs vpos
		#endif
		#if 3<=SHADER_MODEL
		float2 vpos : VPOS; //Shader Model 3
		#endif	
	};
	//for projected shadows only
	CLASSIC_OUTPUT shadow(CLASSIC_SHADOW In)
	{	
	classic_stereo //NOP?

		CLASSIC_OUTPUT Out; 

		Out.col = In.col; 

		float2 uv = In.fog.xy/In.fog.w; 						
		float4 pos = float4(farFrustum.xyz,1);
		/*pos.xy*=uv;*/
		#ifndef DSTEREO
		pos.xy*=uv;
		#else		
		pos.xy*=float2(uv.x+0.5f*sign(In.stereo),uv.y);		
		//BLACK MAGIC
		//From classic_stereo_sub if(scopic) Out_pos.x+=stereo;
		//throws this off. this formula stabilizes the center of
		//the shadow at extreme degrees of paralax (crossing eyes)
		//34 is just a guess. right now the FOV is fixed at 68* but
		//that may be the wrong setting/may throw his off. Th entire
		//way perspective is done now is funky/probably wrong, but it
		//is not worth throwing away yet
		//(*NOTE: 34 is not 68/2.... it would be radians if an angle)
		//#if 2!=EX_INI_STEREO_MODE
		//pos.x-=In.stereo*34.0f/In.fog.w; //34 is specific to 960x1080
		//#endif
		#endif
		#if 3<=SHADER_MODEL
		//testing with vpos
		pos.xyz*=tex2D(sam2,In.vpos*rcpViewport.xy+0.0001).x;
		#else
		pos.xyz*=sample1(uv).x;
		#endif
		
	//	float3 st = mul(x4mUV,pos).xyz; //perspective uncorrected :(
		float3 st = mul(In.mat,pos).xzy; //!!			

		//2023: rotate shadow?
		float sx = st.x; float sy = st.y;
		st.x = sx*In.xforms.x+sy*-In.xforms.y;
		st.y = sx*In.xforms.y+sy*In.xforms.x;

		st.y/=In.xforms.w/In.xforms.z; //UV space

		st.xy = vec2(0.5f)-st.xy*In.fog.z;		
		//1.9: should (probably) be 2 (squeezing every last drop)
		st.z*=In.fog.z*(EX_INI_SHADOWRADIUS/EX_INI_SHADOWVOLUME*1.9); 
		//reminder: pow is taking the absolute value	
		//(abs does rsq/rcp in the assembled shader)
		//don't alter the power. 2 is round on ramps
		//(plus if st.z is negative, non-2 may bust)
		st.z*=st.z; //pow(st.z,2.0); 

			/*2021: these have a drastic effect on the frame rate
			//in supersampling mode (when looking down at the 
			//large shadow beneath your own feet)
			//(did I think not further processing the pixels might
			//perform better?)
		//NOTE: clamping is NOT setup on the sampler
		clip(st); clip(-st+1);
			*/

		//disabling mipmaps for now 
		//Out.col.a = tex2D(sam0,st.xz,0,0).a; 	 
		//http://bartwronski.com/2015/03/12/fixing-screen-space-deferred-decals/
		float4 dd = clamp(float4(ddx(st.xy),ddy(st.xy)),-0.5,0.5);
		//the artifacts are still visible in screenshots
		//from up close, but not visually noticeable otherwise		
	//Out.col = 1-tex2D(sam0,st.xy+0.0,dd.xy,dd.zw).a;
		float4 s1 = tex2D(sam0,st.xy+0.0,dd.xy,dd.zw);
		float4 s2 = tex2D(sam0,1.15*(st.xy-0.5)+0.5,dd.xy,dd.zw);
		Out.col.a*=(s1.a+s2.a)*0.5;
	//Out.col.a+=0.2;

		Out.col.a-=st.z;

	//Out.col.a = 1;
					
	//classic_rangefog	
	float3 fog = classic_rangefog_sub(pos).xyz;
	//Out.col.a*=fog.x; //too dark
	Out.col.rgb = lerp(fogColor,Out.col.rgb,fog.x);
	Out.col.a*=fog.y;
	classic_correct	

		Out.z = vec4(0.0); //compiler

		return Out; 
	}		  
	CLASSIC_OUTPUT blended(CLASSIC_INPUT In)	   
	{	
	classic_stereo //NOP?

		CLASSIC_OUTPUT Out; 

	classic_aa(1)
	classic_colorkey
	classic_gamma
	
		//WHAT'S THIS ABOUT AGAIN?
		//modulate or add? I can't recall why this matters but 
		//I can recall som_db makes use of both modes
		Out.col = lerp(Out.col*In.col,Out.col+In.col,colFunction);
						
		#ifndef DIN_EDITOR_WINDOW
	classic_rangefog	

			/*doesn't quite work
			//claw back brightness?
			Out.col.rgb/=DMPX_BALANCE;*/

	classic_correct	
	classic_z 
		Out.z.r*=skyRegister.z; //1 or -1 if NPC (for shadows)	
		#else
		Out.z = vec4(0.0); //compiler
		#endif	

		//2023: fade monsters to white on death
		float1 gray = dot(Out.col.rgb,float3(0.222,0.707,0.071));		
		Out.col.rgb = lerp(Out.col.rgb,gray.rrr,In.uv0.a)+In.uv0.a*0.5;

		return Out; 
	}
	CLASSIC_OUTPUT backdrop(CLASSIC_INPUT In)	   
	{		
	classic_stereo //NOP?

		CLASSIC_OUTPUT Out; 
	
	classic_aa(1)
	classic_colorkey
	classic_gamma //might want option to exempt sky
	
		Out.col = Out.col*In.col*(vec4(1.0)-colFunction)+ //modulate
				  Out.col*colFunction+In.col*colFunction; //add	

	//#ifdef DFOGLINE		
		float2 uv = In.fog.xy/In.fog.w; 
		//NEW: using 1D depth texture to make room for more stuff later on
		//Out.col.a*=1.0f-tex2D(sam2,float2(uv.x,-uv.y)/2+0.5f+0.0001f).y; 		
		//note: x may be negative but we want its length so that's alright
		float3 pos = farFrustum.xyz; 
		//pos.xy*=uv;
		#ifndef DSTEREO
		pos.xy*=uv;
		#else
		pos.xy*=float2(uv.x+0.5f*sign(In.stereo),uv.y);
		#endif
		#if 3<=SHADER_MODEL
		//testing with vpos
		pos*=tex2D(sam2,In.vpos*rcpViewport.xy+0.0001f).x;
		#else
		pos*=sample1(uv).x;
		#endif

		//length: ASSUMING RANGE FOG DESIRED
		Out.col.a*=1.0-saturate((skyRegister.y-length(pos))*skyRegister.x); 
	//#endif
	
		#if 0 //TESTING
		{
			//IS BLENDING TOO DIRTY
			//need to pass clear color?
			Out.col.rgb = lerp(float3(0,0,0),Out.col.rgb,In.fog.z);
		}
		#else 
		{
			//#ifdef DSKYFLOOD
			Out.col.a*=min(1.0,In.fog.z);  
			//#endif
		}
		#endif

	classic_correct

		Out.z = vec4(0.0); //compiler

		return Out; 
	})
};

//dropping support for vs_1/ps_1
//BEING SEPARATED IS PROBABLY UNWARRANTED AT THIS POINT
static const char *som_shader_vs2_d[SOM::Define::choices];
static const char *som_shader_vs3_d[SOM::Define::choices];
static const char *som_shader_ps2_d[SOM::Define::choices];
static const char *som_shader_ps3_d[SOM::Define::choices];
static const char **som_shader_d(int model, bool undef=false)
{
	const char **out = 0;

	switch(model) //REMOVE ME?
	{
	case 'vs_3': out = som_shader_vs3_d; break; 
	case 'ps_3': out = som_shader_ps3_d; break;
	case 'vs_2': out = som_shader_vs2_d; break;
	case 'ps_2': out = som_shader_ps2_d; break;

	default: assert(out);
	}				  
	if(undef&&out)
	for(int i=0;i<SOM::Define::choices;i++) out[i] = "";
	return out;
}

static bool som_shader_macro(int model, int d, const char *format, va_list va)
{	
	char buf[4096];
	size_t i = (size_t)vsnprintf(buf,sizeof(buf),format,va);	
	if(i>=sizeof(buf)) 
	{
		assert(i<sizeof(buf)); return false;
	}
	switch(model)
	{
	default: return false;
	case 'vs_2': case 'vs_3': i = d; break;
	case 'ps_2': case 'ps_3': i = d+SOM::Define::choices; break;	
	}
	static std::vector<std::string> mem(2*SOM::Define::choices);
	mem[i] = buf;
	som_shader_d(model)[d] = mem[i].c_str(); return true;
}

static char **som_shader_linkage(int count)
{
	static int x; 
	static char **link = new char*[x=std::max(count,8)];	  
	if(count>x)
	{
		delete[] link; link = new char*[x=count]; 
	}								   
	return link;
}

static void som_shader_define_vs(SOM::Define D, va_list va, int model)
{
	int i = (int)D; /*NEW*/	switch(D)
	{	
	case SOM::Define::rangefog:		 
		som_shader_d(model)[i] = "DRANGEFOG"; break; //UNUSED   
	case SOM::Define::alphafog:				
		som_shader_d(model)[i] = "DALPHAFOG"; break; //UNUSED  	
	case SOM::Define::lights:
		som_shader_macro(model,i,"DLIGHTS %d",va); break;
	case SOM::Define::hdr:
		som_shader_d(model)[i] = "DHDR"; break;
	case SOM::Define::aa:
		som_shader_d(model)[i] = "DAA"; break;	
	case SOM::Define::stereo:  
		if(model=='vs_3')
		som_shader_d(model)[i] = "DSTEREO"; break; 
	case SOM::Define::gamma_n:
		som_shader_macro(model,i,"DGAMMA_N %ls",va); break; 
	case SOM::Define::ambient2:
		som_shader_d(model)[i] = "DAMBIENT2"; break;	
	case SOM::Define::index:
		som_shader_macro(model,i,"SHADER_INDEX %d",va); break;
	default: assert(0); return;
	}

	//2020: permit additional definitions? (gamma_n) (index)
	if(EX::defining_shader_macro())
	EX::defining_shader_macro(som_shader_d(model)[i])--; //!
}
static void som_shader_define_vs(SOM::Define D, int model,...)
{
	va_list va; va_start(va,model);	som_shader_define_vs(D,va,model);
}

static void som_shader_define_ps(SOM::Define D, va_list va, int model)
{
	int i = (int)D; /*NEW*/	switch(D)
	{
	case SOM::Define::dither:  
		//som_shader_d(model)[i] = "DDITHER"; break; 
		som_shader_macro(model,i,"DDITHER %d.0",va); break;
	case SOM::Define::brightness:  
		som_shader_d(model)[i] = "DBRIGHTNESS"; break;	
	case SOM::Define::green:
		som_shader_d(model)[i] = "DGREEN"; break;
	case SOM::Define::colorkey:
		som_shader_d(model)[i] = //"DCOLORKEY";
		DDRAW::colorkey==SOM::colorkey?
		"DCOLORKEY 0":"DCOLORKEY 2000"; break;		
	case SOM::Define::invert:
		som_shader_d(model)[i] = "DINVERT"; break;
	case SOM::Define::highcolor:
		som_shader_d(model)[i] = "DHIGHCOLOR"; break;	
	case SOM::Define::stipple:
		som_shader_macro(model,i,"DSTIPPLE %d",va); break;
	case SOM::Define::aa:
		if(model=='ps_3') //ddx/ddy
		som_shader_d(model)[i] = "DAA"; break;
	case SOM::Define::ss: //EXPERIMENTAL
		if(model=='ps_3')
		som_shader_d(model)[i] = "DSS"; break;
	case SOM::Define::dissolve:
		som_shader_macro(model,i,"DDISSOLVE %d",va); break;
	case SOM::Define::stereo:  
		if(model=='ps_3')
		som_shader_d(model)[i] = "DSTEREO"; break; 
	case SOM::Define::stereoLOD:
		if(model=='ps_3')
		som_shader_macro(model,i,"DMIPTARGET %f",va); break; 
	case SOM::Define::stereo_y: //see gamma_y comment
		som_shader_macro(model,i,"DSTEREO_Y %ls",va); break; 
	case SOM::Define::stereo_w: //see gamma_y comment
		som_shader_macro(model,i,"DSTEREO_W %ls",va); break; 
	case SOM::Define::fogpowers:
		som_shader_macro(model,i,"DFOGPOWERS float4(%f,%f,%f,%f)",va); break;
	case SOM::Define::gamma_y:		
		//D3DXMACRO can't seem to allow for parameters
		//som_shader_macro(model,i,"DGAMMA_Y(y) %ls",va); break; 
		som_shader_macro(model,i,"DGAMMA_Y %ls",va); break; 
	case SOM::Define::gamma_y_stage:
		som_shader_macro(model,i,"DGAMMA_Y_STAGE %d",va); break;
	case SOM::Define::sRGB:
		som_shader_macro(model,i,"DSRGB %d",va); break;
	case SOM::Define::index:
		som_shader_macro(model,i,"SHADER_INDEX %d",va); break;
	default: assert(0); return;
	}

	//2020: permit additional definitions? (gamma_n) (index)
	if(EX::defining_shader_macro())
	EX::defining_shader_macro(som_shader_d(model)[i])--; //!
}						   
static void som_shader_define_ps(SOM::Define D, int model,...)
{
	va_list va; va_start(va,model);	som_shader_define_ps(D,va,model);
}

static void som_shader_undef(SOM::Define D, int model)
{		
	if(D==SOM::Define::defined)	som_shader_d(model,true);
	else if(D<SOM::Define::choices)
	{
		//2020: permit additional definitions? (gamma_n)
		if(EX::defining_shader_macro())
		EX::undefining_shader_macro(som_shader_d(model)[(int)D]);

		som_shader_d(model)[(int)D] = "";
	}
	else assert(0);
}
static bool som_shader_ifdef(SOM::Define D, int model)
{
	if(D>0&&D<SOM::Define::choices)
	return som_shader_d(model)[(int)D]!=""; return false;
}

//NEW: just makes the embedded HLSL easier to maintain
static const char *som_shader_cpprepare(const char *hlsl)
{
	static std::string out; out.clear();
	
	//nonstandard... really necessary?
	//out = "#pragma pack_matrix(row_major)\n";
	if(hlsl)
	for(const char *p=hlsl,*pp=p;*p;pp=p)
	{
		while(*p&&*p!='#') p++;

		out.append(pp,p-pp); out+='\n'; 
		
		if(!*p) break;

		pp = p++; //#

		if(p[2]=='i') //#elif
		{
			assert(!memcmp(p,"elif",4));
			p+=2;
		}

		bool compound = *p=='i'; //#if		
		if(compound) compound: //#if
		{
			while(!isspace(*p)) p++;
			while(isspace(*p)) p++; 
		}
		while(!isspace(*p)) p++;

		if(compound) 
		{
			while(isspace(*p)) p++; switch(*p)
			{
			case '&': case '|': goto compound;
			}
		}
		
		out.append(pp,p-pp); 
		
		//#define (HLSL_DEFINE)
		if(pp[1]!='d') out+='\n';
	}

	return out.c_str();
}
template<typename C>
static void *som_shader_compile_vs(C c, int model, int vs, int index)
{
	const char **d = som_shader_d(model);

	const char *main = 0, *code = 0;

	switch(vs)
	{ 
	case SOM::Shader::effects: main = "f"; break;

	case SOM::Shader::classic: assert(0); return 0;
	
	case SOM::Shader::classic_fog: main = "fog"; break;
	case SOM::Shader::classic_blit: main = "blit"; break;
	case SOM::Shader::classic_unlit: main = "unlit"; break;
	case SOM::Shader::classic_sprite: main = "sprite"; break;
	case SOM::Shader::classic_shadow: main = "shadow"; break;	
	case SOM::Shader::classic_blended: main = "blended"; break;
	case SOM::Shader::classic_backdrop: main = "backdrop"; break;

	default: assert(0); return 0;
	}

	switch(vs)
	{ 
	case SOM::Shader::effects: 
		
		code = som_shader_effects_vs; break;

	case SOM::Shader::classic_fog: 
	case SOM::Shader::classic_blit: 
	case SOM::Shader::classic_unlit:
	case SOM::Shader::classic_sprite:
	case SOM::Shader::classic_shadow:
	case SOM::Shader::classic_blended:
	case SOM::Shader::classic_backdrop:

		code = som_shader_classic_vs; break;
	}

	//HACK! FIX ME
	//this only works if there is at least one preprocessor 
	//definition per program
	static int defined = -1; //???
	if(EX::defining_shader_macro()!=defined) 
	{
		EX::undefining_shader_macros();
		
		switch(model) //DUPLICATE
		{
		case 'vs_3': EX::defining_shader_macro("SHADER_MODEL 3"); break;
		case 'vs_2': EX::defining_shader_macro("SHADER_MODEL 2"); break;
		case 'vs_1': EX::defining_shader_macro("SHADER_MODEL 1"); break;
		}		
		for(int i=0;i<SOM::Define::choices;i++) 			
		if(*d[i]) EX::defining_shader_macro(d[i]);
		if(EX::debug) //DDEBUG?
		EX::defining_shader_macro("DDEBUG");	
		if(SOM::tool) 
		EX::defining_shader_macro("DIN_EDITOR_WINDOW");					
		
		//EXPERIMENTAL
		//this is very finely tuned to KF2's polished stone walls
		//combined with fmod quantization to match dither pattern
		//(it seems to depend on the level ambient, this is tuned
		//for 55,55,55)
		//
		// TODO: I have to investigate MapComp to see if this can
		// be generalized to all projects (NOTE: do_kf2 is abused
		// here so my KF2 project is able to use this for a demo)
		//
		if(EX::INI::Option()->do_kf2)
		EX::defining_shader_macro("DMPX_BALANCE 0.96837");

		defined = EX::defining_shader_macro(); 
	}

	//2021: SHADER_INDEX
	bool si = som_shader_ifdef(SOM::Define::index,model);
	if(si||index!=-1)
	{
		if(si) som_shader_undef(SOM::Define::index,model);

		if(index!=-1)
		som_shader_define_vs(SOM::Define::index,model,index);
	}
	
	return (void*)c(model,main,som_shader_cpprepare(code),0,'hlsl');
}

template<typename C>
static void *som_shader_compile_ps(C c, int model, int ps, int index)
{
	const char **d = som_shader_d(model);

	const char *main = 0, *code = 0;

	switch(ps)
	{ 
	case SOM::Shader::effects: main = "f"; break;
		
	case SOM::Shader::classic: assert(0); return 0;

	case SOM::Shader::classic_fog: main = "fog"; break;
	case SOM::Shader::classic_blit: main = "blit"; break;
	case SOM::Shader::classic_unlit: main = "unlit"; break;
	case SOM::Shader::classic_volume: main = "volume"; break;
	case SOM::Shader::classic_sprite: main = "sprite"; break;
	case SOM::Shader::classic_shadow: main = "shadow"; break;
	case SOM::Shader::classic_blended: main = "blended"; break;
	case SOM::Shader::classic_backdrop: main = "backdrop"; break;

	default: assert(0); return 0;
	}

	switch(ps)
	{ 	
	case SOM::Shader::effects: 
		
		code = som_shader_effects_ps; break;
	
	case SOM::Shader::classic_fog:  
	case SOM::Shader::classic_blit: 
	case SOM::Shader::classic_unlit:
	case SOM::Shader::classic_volume:
	case SOM::Shader::classic_sprite:
	case SOM::Shader::classic_shadow:
	case SOM::Shader::classic_blended:
	case SOM::Shader::classic_backdrop:

		code = som_shader_classic_ps; break;
	}
	
	//HACK! FIX ME
	//this only works if there is at least one preprocessor 
	//definition per program
	static int defined = -1; //???
	if(EX::defining_shader_macro()!=defined) 
	{
		EX::undefining_shader_macros();

		switch(model) //DUPLICATE
		{
		case 'ps_3': EX::defining_shader_macro("SHADER_MODEL 3"); break;
		case 'ps_2': EX::defining_shader_macro("SHADER_MODEL 2"); break;
	//	case 'ps_1': EX::defining_shader_macro("SHADER_MODEL 1"); break;
		}
		for(int i=0;i<SOM::Define::choices;i++) 			
		if(*d[i]) EX::defining_shader_macro(d[i]);	
		if(EX::debug) //DDEBUG?
		EX::defining_shader_macro("DDEBUG");	
		if(SOM::tool) 
		EX::defining_shader_macro("DIN_EDITOR_WINDOW");

		//2021: ps_3 doesn't require DUV (nor does OpenGL)
		if(model=='ps_2')
		{
			//TODO: DUV should just be a real-time constant
			char buf[512];
			sprintf_s(buf,"DUV (float2(%g,%g)+uv*float2(%g,%g))",
			DDRAW::xyMapping[0]/SOM::width,
			DDRAW::xyMapping[1]/SOM::height,
			DDRAW::xyScaling[0],DDRAW::xyScaling[1]);
			EX::defining_shader_macro(buf);
		}
   		
		defined = EX::defining_shader_macro();
	}

	//2021: SHADER_INDEX
	bool si = som_shader_ifdef(SOM::Define::index,model);
	if(si||index!=-1)
	{
		if(si) som_shader_undef(SOM::Define::index,model);

		if(index!=-1)
		som_shader_define_ps(SOM::Define::index,model,index);
	}

	return (void*)c(model,main,som_shader_cpprepare(code),0,'hlsl');
}

static void som_shader_configure(int count, char *A, va_list va, int *at)
{
	if(!count) return;

	char **link = som_shader_linkage(count);

	link[0] = A;

	if(count>1)	for(int i=1;i<count;i++) link[i] = va_arg(va,char*);

	const int *p = EX::linking_shaders(link,count,at);

	//TODO: optimize dx.ddraw.h constant table
}

static void som_shader_release(int count, char *A, va_list va)
{
	if(!count) return;		
	for(int i=1;i<count;i++) 
	delete[] va_arg(va,char*); delete[] A;
}

static void som_shader_initialize_vs()
{		
	if(DDRAW::shader) return;

	//REMINDER: this is to not conflict with Exselector's
	//use of OpenGL via nanovg (GLNVG_FRAG_BINDING) since
	//it just binds to 0 (I don't know why OpenGL doesn't
	//have a "Gen" function to facilitate collaborations)
	int compile[10==GLSL_0]; (void)compile;
	DDRAW::compat_glUniformBlockBinding_1st_of_6 = GLSL_0;

	/*#ifdef _DEBUG
	//note these may depend on the resolution, and it seems
	//that they are identical because of default pupil dist
	//{0.423099697, 0.340755254, -0.499920547, -0.499986768}
	//{0.561066210, 0.498725533, -0.499953777, -0.499993265}
	//{0.423099697, 0.340755254, -0.500079453, -0.499986768}
	//{0.561066210, 0.498725533, -0.500046253, -0.499993265}
	PSVR::BarrelShader bs; bs.init(1920,1080);
	#endif*/

	som_shader_setlightmax(16);

	//// VERTEX SHADER CONSTANTS /////////

	//DDRAW::vsDebugColor = DDRAW::vsF+0;
	DDRAW::vsPresentState = DDRAW::vsF+1;
	DDRAW::vsColorFactors = DDRAW::vsF+2;

	for(int i=0;i<4;i++) 
	{
		//dx_d3d9c_worldviewproj?
		if(SOM::tool) 
		DDRAW::vsWorldViewProj[i] = DDRAW::vsF+4+i;
		DDRAW::vsWorldView[i]     = DDRAW::vsF+8+i;		
		DDRAW::vsWorld[i]	      = DDRAW::vsF+12+i;
		DDRAW::vsView[i]		  = DDRAW::vsF+16+i;
		//not used (using P for DSTEREO instead)
		//DDRAW::vsInvView[i]     = DDRAW::vsF+20+i;
		DDRAW::vsProj[i]		  = DDRAW::vsF+20+i;
		DDRAW::psTexMatrix[i]	  = DDRAW::psF+30+i; //PS
		DDRAW::vsTexMatrix[i]	  = DDRAW::vsF+30+i;
	}
	
	DDRAW::vsFogFactors      = DDRAW::vsF+24;
	DDRAW::vsMaterialAmbient = DDRAW::vsF+25;
	DDRAW::vsMaterialDiffuse = DDRAW::vsF+26;
	DDRAW::vsMaterialEmitted = DDRAW::vsF+27;
	DDRAW::vsGlobalAmbient   = DDRAW::vsF+28;
	//vsGlobalAmbient2		 = DDRAW::vsF+28+1; //ambient2?
	
	for(int i=0;i<16;i++) 
	{
	//	DDRAW::vsLightsAmbient[i] = DDRAW::vsF+32+i; //2020
		DDRAW::vsLightsDiffuse[i] = DDRAW::vsF+48+i;
		DDRAW::vsLightsVectors[i] = DDRAW::vsF+64+i;
		DDRAW::vsLightsFactors[i] = DDRAW::vsF+80+i;
	}

	//96 is setting the limit on float registers for OpenGL
	//buffers
	//DDRAW::vsI = 256; //integers (starting point)
	DDRAW::vsI = cMaxVS+1;
	DDRAW::psI = cMaxPS+1;

	DDRAW::vsLights = DDRAW::vsI+0; 
}

static void som_shader_initialize_ps()
{		
	if(DDRAW::shader) return;
	
	//// PIXEL SHADER CONSTANTS /////////

	DDRAW::psColorize = DDRAW::psF+26;
	DDRAW::psFarFrustum = DDRAW::psF+27;

	//NOTE: som.hacks.cpp zeroes if Shader Model 2
	if(SOM_SHADER_MAPPING2)
	DDRAW::ps2ndSceneMipmapping2 = DDRAW::psF+0; //2021

	//2023: vol power/depth in z/w (x/y is unallocated)
	//psConstants10 = 10; 
}

extern void SOM::initialize_shaders()
{
	assert(""==""); //string pooling enabled

	som_shader_initialize_vs();
	som_shader_initialize_ps();	

	som_shader_d('vs_3',true);
	som_shader_d('ps_3',true);
	som_shader_d('vs_2',true);
	som_shader_d('ps_2',true);
}

void SOM::Shader::log()
{
	switch(which)
	{	
	case SOM::Shader::classic: return;

		EXLOG_LEVEL(-1) << "\nvertex shader (classic vs)\n\n";

		EXLOG_LEVEL(-1) << som_shader_classic_vs << "\n\n";

		EXLOG_LEVEL(-1) << "\npixel shader (classic ps)\n\n";

		EXLOG_LEVEL(-1) << som_shader_classic_ps << "\n\n";

		return;		

	default: assert(0); return; //should use a one word shader 
	}
}

static const char *som_shader_source(bool i, SOM::Shader sh)
{
	if(!DDRAW::gl){ assert(0); return 0; } //IMPLEMENT ME?

	enum{ cppN=sh._TOTAL_+1+1 };
	if(sh>=cppN-1){ assert(0); return 0; }
	static std::string(*cpp)[cppN][2] = 0; if(!cpp)
	{
		(void*&)cpp = new std::string[cppN*2];
	}

	if(sh==sh.effects_d3d11) i = DDRAW::xr; //HACK

	//YUCK: insert '\n' around #macros
	auto &s = (*cpp)[sh][i]; if(s.empty())
	{
		using namespace som_shader_glsl; //som.shader.hpp
		#define _(x) case sh.classic_##x: c = classic_##x[i]; break;
		const char *c; switch(sh)
		{
		case sh.classic: c = classic[i]; break; //header?
		case sh.effects: c = effects[i]; break;
		_(fog)_(blit)_(unlit)_(sprite)_(volume)_(shadow)_(blended)_(backdrop)

		//2022: destined for Widgets95::xr::effects::reset_pshader
		case sh.effects_d3d11: //assert(i);
		c = i?som_shader_effects_xr:som_shader_effects_11; break;
		}	
		#undef _		
		s.assign(som_shader_cpprepare(c));
	}
	int hi = 0, hj;
	if(sh==sh.classic)
	{
		hi = sh._TOTAL_;
		hj = i?'ps_3':'vs_3';
	}
	if(sh==sh.effects_d3d11) //HACK: append defines
	{
		//hi = sh; 
		hi = sh._TOTAL_+1; 
		hj = 'ps_3';
	}
	if(!hi) return s.c_str();
	
//	auto &h = (*cpp)[sh._TOTAL_][i]; h.clear();
	auto &h = (*cpp)[hi][i]; h.clear();

	//dx.d3d9X.cpp needs to manage this
	//h.assign("#version ");
	//h.append(DDRAW::target=='dx95'?"310 es":"430");
	//h.push_back('\n');

	const char **d = som_shader_d(hj); //i?'ps_3':'vs_3'

	for(int j=0;j<SOM::Define::choices;j++) if(*d[j])
	{
		h.append("#define ").append(d[j]).push_back('\n');		
	}

	//DUPLICATE
	h.append("#define SHADER_MODEL 3\n");
	if(EX::debug)
	h.append("#define DDEBUG\n");
	if(SOM::tool) 
	h.append("#define DIN_EDITOR_WINDOW\n");

	if(!i) //vertex shaders?
	{
		if(EX::INI::Option()->do_kf2)
		h.append("#define DMPX_BALANCE 0.96837\n");
	}
	else //pixel shaders
	{
		//NOTE: DUV is not required for OpenGL since it's
		//not implementing Shader Model 2
	}

	h.append(s).push_back('\n'); return h.c_str();	
}

//REMOVE ME?
const int SOM::VS3::model = 'vs_3';
const int SOM::PS3::model = 'ps_3';
const int SOM::VS2::model = 'vs_2';
const int SOM::PS2::model = 'ps_2';
//const int SOM::VS1::model = 'vs_1';
//const int SOM::PS1::model = 'ps_1';
#define T VS3
#define F(f) f##_vs
#include "som.shader.inl"
#define T PS3
#define F(f) f##_ps
#include "som.shader.inl"
#define T VS2
#define F(f) f##_vs
#include "som.shader.inl"
#define T PS2
#define F(f) f##_ps
#include "som.shader.inl"
//#define T VS1
//#define F(f) f##_vs
//#include "som.shader.inl"
//#define T PS1
//#define F(f) f##_ps
//#include "som.shader.inl"
#undef T
#undef F



//// SOM::colorkey //// SOM::colorkey //// SOM::colorkey //// SOM::colorkey

//SOM::colorkey WILL NEED ITS OWN CPP FILE EVENTUALLY. FOR NOW THIS WILL DO.

namespace //EXPERIMENTAL
{		
	enum _EdgeFlag
	{
		//This will alter red/blue by 1 shade, but will avoid using a pixels.
		EdgeFlagH = (1 << 16),
		EdgeFlagV = (1 << 0),
	};

	struct Antialiaser
	{
		UINT *pixels;  
		int width; //Width of color pixels being processed.
		int height; //Width of color pixels being processed.		
		int pitch; // Row pitch of color pixels being processed (in pixels, not bytes).
		int stride; //Not using SSE and transpose, so must implement this transparently.
		float rcp_stride;
		int size;

		Antialiaser(UINT *pixels, int w, int h, int pbytes)
		:pixels(pixels),width(w),height(h),pitch(pbytes/4),stride(1),size(h*pbytes/4)
		{}
	};

	//MLAA: Efficiently Moving MLAABlending from the GPU to the CPU
	//This is copied from the following URL. But it doesn't work well for many shapes.
	//For many it does. But it's nothing like a traditional AA algoirthm.
	//https://software.intel.com/en-us/articles/mlaa-efficiently-moving-antialiasing-from-the-gpu-to-the-cpu

	//ComputeHc for b/w.
	static const float MonochromeHC = 0.5f; 

	//REMOVE ME?
	struct MLAABlend : private Antialiaser 
	{	
		_EdgeFlag EdgeFlag;

		MLAABlend(UINT *pixels, int w, int h, int pbytes)
		:Antialiaser(pixels,w,h,pbytes)
		{
			EdgeFlag = EdgeFlagH; rcp_stride = 1; 
			BlendBlock();
			std::swap(width,height); std::swap(pitch,stride);
			EdgeFlag = EdgeFlagV; rcp_stride = 1.0f/stride;
			BlendBlock();
		}

	private: void BlendBlock()const;				
		
		//The code this is adapted from mixed RGB using the A channel to store edge flags.
		//The situation is reversed here. 
		//THERE MAY BE A PROBLEM WITH DOING THIS WITH A SINGLE BUFFER, IF PREVIOUSLY BLENDED
		//COLORS BECOME BLEND INPUTS.
		//The original code does color->blend->work, work->blend->color, with blend done on the
		//input pixels drawing from the same pixels.
		void MixColors(int Pixel, float Weight1, int Color1, float Weight2, int Color2)const
		{
			UINT sum = UINT(Weight1*(pixels[Color1]>>24)+Weight2*(pixels[Color2]>>24));
			assert(sum<=255);
	
			pixels[Pixel] = sum<<24|pixels[Pixel]&0x00FFffFF;

			//pixels[Color1] = UINT(255*Weight1)<<24|0xFe;
			//pixels[Color2] = UINT(255*Weight2)<<24|0xFe00;

			//if(EX::debug) pixels[Pixel] = 0xFF000000|sum<<8;
			//if(EX::debug) pixels[Pixel] = 0xFF000000|UINT(255*Weight2);
		}

		int FindSeparationLine(int &OutOffsetLineStart, int &OutOffsetLineEnd, int OffsetCurrentPixel, int OffsetEndRow)const;
		
		template<bool How> //upper-bound if true, lower if false.
		void ComputeBounds(int& OutS0, int& OutS1, int OffsetStart, int OffsetEnd,int LineLength, int StepToPriorOrNextRow)const;

		void ComputeUpperBounds(int &a, int &b, int c, int d, int e, int StepToPreviousRow)const
		{
			ComputeBounds<true>(a,b,c,d,e,StepToPreviousRow);
		}

		void ComputeLowerBounds(int &a, int &b, int c, int d, int e, int StepToNextRow)const
		{
			ComputeBounds<false>(a,b,c,d,e,StepToNextRow);
		}

		template<bool UShape> 
		//Reminder: StepToCoBlendRow is either pitch or -pitch.
		void BlendInterval(int OffsetFirst, int OffsetLast, int StepToCoBlendRow)const;
	};	  
}
//-----------------------------------------------------------------------------------------------------------------------------------------
// Given a range of pixels offsets [OffsetCurrentPixel, OffsetEndRow], walk this range to find a discontinuity line.
// We call a "separation line" or "discontinuity line" a sequence of consecutive pixels that all have the same edge flag set.
// The 3 return values are: the length of the separation line, and the offsets in the pixels of the first and last pixel of the line.
//-----------------------------------------------------------------------------------------------------------------------------------------
int MLAABlend::FindSeparationLine(int& OutOffsetLineStart, int& OutOffsetLineEnd, int OffsetCurrentPixel, int OffsetEndRow)const
{
	////FIX-ME: THIS IS BORDER/WRAPAROUND LOGIC
	if(OffsetCurrentPixel>=OffsetEndRow)
	{	
		// We are done scanning this row/column; no separation line left to find.
		return 0;
	}

	// Find first extremity of the line...
	OutOffsetLineStart = -1;
	
	for(;;)
	{	
		// Not SSE-aligned so need to move pixel by pixel.
		if((pixels[OffsetCurrentPixel] & EdgeFlag) != 0)
		{
			OutOffsetLineStart = OffsetCurrentPixel;
			break;
		}
		OffsetCurrentPixel+=stride;
        if(OffsetCurrentPixel>OffsetEndRow)
        {
            OutOffsetLineStart = OutOffsetLineEnd = OffsetEndRow-stride;
            return 0;
        }
	}

	// Now look for the second extremity of the line.
	int LineLength = 1;
	OffsetCurrentPixel+=stride;
	while((OffsetCurrentPixel <= OffsetEndRow) && ((pixels[OffsetCurrentPixel] & EdgeFlag) != 0))
	{
		LineLength+=1;
		OffsetCurrentPixel+=stride;
	}
	OutOffsetLineEnd = OffsetCurrentPixel-stride; return LineLength;
}
template<bool Upper> //Or Lower if false.
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Walk the input separation line ("primary edge") from OffsetStart to OffsetEnd (and length LineLength), looking for an orthogonal separation line (to form the secondary edge)
// This particular function looks for secondary edges on the upper side of the primary edge.
//	Outputs are: OutS0: offset of first orthogonal sep. line found by walking left -> right (or top -> bottom), OutH0 the corresponding value of hc (cf. ComputeHc function)
//               OutS1: offset of first orthogonal sep. line found by walking right -> left (or bottom -> top), OutH1 the corresponding value of hc
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
inline void MLAABlend::ComputeBounds(int& OutS0, int& OutS1,
int OffsetStart, int OffsetEnd, int LineLength, int StepToPriorOrNextRow)const
{
    const int OrthoEdgeFlag = EdgeFlag ^ (EdgeFlagH | EdgeFlagV);
	OutS0 = OutS1 = -1;
	int CurrentOffset = OffsetStart;
	int T0 = -1, T1 = -1;

	do	// FIRST LOOP: WALK THE SEPARATION LINE FROM LEFT TO RIGHT (OR TOP TO BOTTOM FOR VERTICAL PASS)
	{
		int Offset = CurrentOffset;
		
		if(!Upper) Offset+=StepToPriorOrNextRow; // i.e. offset of pixel below the one being currently considered

		int Offset2 = Offset;

		if(Upper) Offset2+=StepToPriorOrNextRow; //StepToPreviousRow

		if( ((pixels[Offset] & OrthoEdgeFlag) != 0) && ((pixels[Offset2] & EdgeFlag) != 0) )
		{	
			// We have found a pattern of this form (in the case of the horizontal blending pass; "transpose" text below for the vertical pass):
			//   __
			//  ..x|____
			// Where the bottom edge is the edge we are currently walking; we have NSteps horizontal edge units to the left of the vertical edge (the dots), and (LineLength - NSteps) to the right.
			// Therefore the "primary edge" of this pattern is (LineLength - Nsteps) long, and we are now goin to compute hc (height of connection point on the vertical "secondary edge")
			// (x marks the pixel at current offset.)
			OutS0 = CurrentOffset+stride;
			break;
		}   
		
		if(((pixels[Offset] & OrthoEdgeFlag) != 0) && (T0 == -1))
		{	
			// Keep track of where we found the first orthogonal separation line (secondary edge)
			T0 = CurrentOffset;
		}

		CurrentOffset+=stride;

	}while(CurrentOffset<OffsetEnd);

	if((OutS0 == -1) && (T0 != -1))
	{	
		// If we found an orthogonal separation line, but no pattern, connect to the middle of the secondary edge.
		OutS0 = T0 + stride;
	}

	////FIX-ME: IS THIS BORDER/WRAPAROUND LOGIC?
	if(OffsetEnd + stride >= size)
	{
		if((pixels[OffsetEnd] & OrthoEdgeFlag) != 0)		
		{		
			T1 = OffsetEnd;
		}
		OffsetEnd-=stride;
	}

	CurrentOffset = OffsetEnd;
	do	// SECOND LOOP: WALK THE SEPARATION LINE FROM RIGHT TO LEFT (OR BOTTOM TO TOP FOR VERTICAL PASS)
	{
		int Offset = CurrentOffset;
		
		if(!Upper) Offset+=StepToPriorOrNextRow; // i.e. offset of pixel below the one being currently considered

		int Offset2 = Offset+stride;

		if(Upper) Offset2+=StepToPriorOrNextRow; //StepToPreviousRow

		if( ((pixels[Offset] & OrthoEdgeFlag) != 0) && ((pixels[Offset2] & EdgeFlag) != 0) )
		{	
			////ComputeUpperBounds said:
			// We have found a pattern of this form:
			//       _
			//  ___x|...
			//
			///ComputeLowerBounds said:
			// We have found a pattern of this form:
			// ___x...
			//    |
			//     -
			//Is this correct? Or a stale commentary?
			OutS1 = CurrentOffset;
			break;
		}
		
		if(((pixels[Offset] & OrthoEdgeFlag) != 0) && (T1 == -1))
		{	// Keep track of where we found the first orthogonal separation line (secondary edge)
			T1 = CurrentOffset;
		}
		CurrentOffset-=stride;

	}while(CurrentOffset>OffsetStart);

	if((OutS1 == -1) && (T1 != -1))
	{	
		// If we found an orthogonal separation line, but no pattern, connect to the middle of the secondary edge.
		OutS1 = T1;
	}
} 
template<bool UShape> //U-shaped if true.
inline void MLAABlend::BlendInterval(int OffsetFirst, int OffsetLast, int StepToCoBlendRow)const
{
	// Here's a quick explanation about the math. Let's consider the situation below, where our first shape is an L-shape where the secondary edge was found below the separation line.
	// Separation line is the y=1 line, secondary is the vertical line on the left which is one pixel high (from y=0 to y=1). hc is the corresponding value returned by ComputeHc.
	// x0 = offset of first pixel in the primary edge, x1 offset of the last pixel of the primary edge.
	// Considering the first pixel below the separation, we are going to blend this pixel with a weight equal to area A, with the pixel above with a weight equal to area B (A+B=1 of course)
	// The problem is to compute A. This is the area of a trapezoid of height 1 (x0+1-x0), left base hc. It is easy to show that the right base = hc + 2(1-hc)/(x1+1-x0)
	// Therefore, A = hc + (1-hc)/(x1+1-x0) . Also, as we walk to the next pixel, then the next, etc., the area A always increases by the same quantity 2(1-hc)/(x1+1-x0) from pixel to pixel. 
	//
	//      x0      x0+1                  Middle                         x1
	//  1 _ |_________|__________________________________________________|
	//      |                        _,.-'"^`
	//	    |    B    |      _,.-'"^`
	//	    |        _,.-'"^`
	//	hc _| _,.-'"^` 
	//	    |         |
	//	    |
	//	    |    A    |
	//	    |          
	//      |         |
	//  0 _ |_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _

	//assert(0); //Doubting these are offsets. +stride?
	float deltaAreaFirst = 2*(1-MonochromeHC) / ((OffsetLast - OffsetFirst)*rcp_stride + 1);
	//float deltaAreaLast  = 2*(1-MonochromeHC) / ((OffsetLast - OffsetFirst)*rcp_stride + 1);
	float deltaAreaLast  = deltaAreaFirst;
	float AreaA = MonochromeHC + 0.5f * deltaAreaFirst;

	if(StepToCoBlendRow < 0)
	{	// We are blending with the row above. In this case, we move down both offsets one row.
		// This is because for a given pixel, its flag indicates if there is a discontinuity with its bottom neighbor.
		OffsetFirst -= StepToCoBlendRow;
		OffsetLast  -= StepToCoBlendRow;
	}
	int Offset = OffsetFirst;
	int MiddlePixelOffset = (OffsetFirst + OffsetLast)/2;

	do // Process first L-shape
	{
        MixColors(Offset, AreaA, Offset, 1-AreaA, Offset+StepToCoBlendRow);
		
		AreaA+=deltaAreaFirst;
		Offset+=stride;
		assert((AreaA <= 1.0001) || (Offset>= MiddlePixelOffset));

	}while(Offset < MiddlePixelOffset);

	// Middle point special case.
	// Two cases can happen here depending on the parity of the length of the primary edge.
	// If the length is even, this is the simple case in which x0+x1 is odd, therefore (x0+x1+1)/2 = (floor)((x0+x1)/2)+1
	// In other words, the connection line (oblique line in diagram above) hits the top side of the middle pixel precisely at its top right corner.
	// On the other hand if the length is odd, the connection line will hit the top side of the middle pixel right in the middle.
	// We hand up with this for the middle pixel (case of an L-shape):
	//                         /
	//              __________/_________
	//              | T ,.-'"^`        |
	//	            |"^`               |
	//	            |                  |
	//             ...                ...
	//
	//	We have to do two blending passes: one for the left half/shape, and one for the second half/shape.
	//	The area of triangle T = 0.5 * 0.5 [half pixel length] * (1-hcFirst)/(OffsetLast-OffsetFirst+1) = deltaAreaFirst/8
	//	Similarly the corresponding triangle for the second half has an area of deltaAreaLast/8 ...
	if(Offset == MiddlePixelOffset)
	{	
		// Odd size case		
        MixColors(Offset, 1-deltaAreaFirst/8, Offset, deltaAreaFirst/8, Offset+StepToCoBlendRow);

		if(!UShape)
		{
            MixColors(Offset+StepToCoBlendRow, deltaAreaLast/8, Offset, 1-deltaAreaLast/8, Offset+StepToCoBlendRow);
		}
        Offset+=stride;
		AreaA = deltaAreaLast;
	}
	else AreaA = deltaAreaLast/2;

	int StepToBlendDestRow = StepToCoBlendRow;

	if(UShape)
	{	
		// We stay on the same side of the separation line
		StepToBlendDestRow = 0;
		AreaA = 1-AreaA;
		deltaAreaLast = -deltaAreaLast;
	}

	do // Process second L-shape
	{
        MixColors(Offset+StepToBlendDestRow, AreaA, Offset, 1-AreaA, Offset+StepToCoBlendRow);
		
		AreaA += deltaAreaLast;
		Offset+=stride;
		assert(((AreaA <= 1.0001) && (AreaA >= -1.e-6)) || (Offset > OffsetLast));

	}while(Offset <= OffsetLast);
} 
void MLAABlend::BlendBlock()const
{
	////FIX-ME: THIS IS BORDER/WRAPAROUND LOGIC
	const int OffsetLastRow = pitch*(height-1); //EdgeFlag==V? (width-1)*height

	int StepToPreviousRow = 0; ////FIX-ME: THIS IS BORDER/WRAPAROUND LOGIC

	for(int OffsetCurrentRow = 0; OffsetCurrentRow < OffsetLastRow; OffsetCurrentRow += pitch, StepToPreviousRow = 0-pitch)
	{
		int OffsetEndRow = OffsetCurrentRow + width*stride - stride; 
		int OffsetCurrentPixel = OffsetCurrentRow;
		int SeparationLineLength = 0;		           
		int OffsetLineStart = 0, OffsetLineEnd = 0;	
			
        for(;;OffsetCurrentPixel=OffsetLineEnd+stride)
        {
			SeparationLineLength =
			FindSeparationLine(OffsetLineStart, OffsetLineEnd, OffsetCurrentPixel, OffsetEndRow);
			if(SeparationLineLength<=0)
			break; 

			if(1==SeparationLineLength)
			{	
				// Special case of a single pixel
				assert((0 <= OffsetLineStart) && (OffsetLineStart < size));
				assert((0 <= OffsetLineStart + pitch) && (OffsetLineStart + pitch < size));
					
				const float BlendWeight = 7.0f/8;
                MixColors(OffsetLineStart, BlendWeight, OffsetLineStart, 1-BlendWeight, OffsetLineStart+pitch);
                MixColors(OffsetLineStart+pitch, 1-BlendWeight, OffsetLineStart, BlendWeight, OffsetLineStart+pitch);
			}
			else
			{
				if(OffsetLineStart==OffsetCurrentRow)
				{	
					// This is needed because of the patterns we are going to check for...
					OffsetLineStart+=stride;
					SeparationLineLength-=1;
				}

				int li0,li1, ui0,ui1; ////FIX-ME: pitch IS BORDER/WRAPAROUND LOGIC
				ComputeLowerBounds(li0, li1, OffsetLineStart-stride, OffsetLineEnd, SeparationLineLength, pitch);
				ComputeUpperBounds(ui0, ui1, OffsetLineStart-stride, OffsetLineEnd, SeparationLineLength, StepToPreviousRow);				

				if(ui0<li1&&ui0!=-1/*&&li1!=-1*/)
				{	
					// We found this combination of L-patterns (edge with ... is separation line investigated):
					//    _
					//   ..|____.._...
					//	           |_
					//
					// This is a Z-pattern.
					BlendInterval<false>(ui0,li1,pitch);
					continue;
				}

				if(li0<ui1&&li0!=-1/*&&ui1!=-1*/)
				{	
					// We found this combination of L-patterns (edge with ... is separation line investigated):
			        //               _
					//   ...____..__|..
					//	  _|        
					//
					// This is a Z-pattern.
					BlendInterval<false>(li0,ui1,-pitch);
					continue;
				}

				if(ui0<ui1&&ui0!=-1/*&&ui1!=-1*/)
				{	
					// We found this combination of L-patterns (edge with ... is separation line investigated):
					//    _         _
					//   ..|__...__|..
					//
					// This is a U-pattern.
					BlendInterval<true>(ui0,ui1,pitch);

					//ADDING continue THAT WASN'T ORIGINALLY HERE.
					//Assuming these pictograms can't be related?!
					continue;
				}						

				if(li0<li1&&li0!=-1/*&&li1!=-1*/)
				{	
					// We found this combination of L-patterns (edge with ... is separation line investigated):
					// ..__...__..
					// _|       |_
					// 
					// This is a U-pattern.
					BlendInterval<true>(li0,li1,pitch);

					//ADDING continue FOR CONSISTENCY SAKE.
					continue;
				}		                
			}            
        }
	}
}

template<class T>
static void som_colorkey 
(void *m, int pitch, int width, int height, bool wrapu, bool wrapv)
{	
	enum{ Bs = sizeof(T), Bshift = Bs==4?24:15, Bmask = ~(~0<<Bshift) };
	assert(Bs==2||Bs==4);	

	for(int row=0;row<height;)
	{													
		union{ BYTE *pixel; T *flags; };

		pixel = (BYTE*)m+pitch*row; 

		BYTE *right = pixel+Bs;
		BYTE *below = pixel+pitch;
		
		if(++row==height) below = wrapv?(BYTE*)m:pixel;

		for(int col=0;col<width;pixel+=Bs,right+=Bs,below+=Bs)
		{
			if(++col==width) right-=(wrapu?pitch:Bs);
							
			int v = *flags>>Bshift!=*(T*)right>>Bshift?+EdgeFlagV:0;
			int h = *flags>>Bshift!=*(T*)below>>Bshift?+EdgeFlagH:0;
						
			//HACK: fill the colors in. previously the shader would
			//divide by the alpha because it was equal to the black
			//neighborhood, linear filter wise
			//this floods diagonals but prefers side-by-side pixels
			//where available. it may be imperfect
			//NOTE: summing is an option, but is it always possible?
			//
			// TODO? https://github.com/rmitton/rjm/blob/master/rjm_texbleed.h
			// IS AN ALGORITHM THAT FILLS COLOR BASED ON DISTANCE THAT WOULD
			// BE LESS BIASED (MORE REGULAR) THAN THIS COLOR FILL HACK
			//
			if(*flags>>Bshift==0)
			{
				if(v) 
				{
					*flags = Bmask&*(T*)right;

					T &above = *(T*)(pixel-pitch); //top-left?
					if(row!=1&&above==0) 
					{
						above = *flags&~(EdgeFlagH|EdgeFlagV);
					}
				}
				if(h) 
				{
					*flags = Bmask&*(T*)below;
					
					if(col!=1&&flags[-1]==0) //top-left?
					{
						flags[-1] = *flags&~(EdgeFlagH|EdgeFlagV);
					}
				}
			}
			if(*flags!=0)
			{
				if(*(T*)right>>Bshift==0) *(T*)right = Bmask&*flags;
				if(*(T*)below>>Bshift==0) *(T*)below = Bmask&*flags;
			}
			
			//16bit AA is impossible, but som.shader.cpp needs 
			//to know if the pixels are black, and this is the 
			//easiest way for now.
			if(Bs==sizeof(UINT))
			*(UINT*)flags = h|v|(*flags&~(EdgeFlagH|EdgeFlagV));
		}		
    }
	
	//EXPERIMENTAL
	//MLAA isn't good enough for many things, but it's so good for
	//so many things, that it's used. If a shape is highly geometric
	//it may have to be thought of as bent, or use polygons instead.
	//(At least until more AA/non-AA modes become available.)
	if(Bs==sizeof(UINT))
	MLAABlend((UINT*)m,width,height,pitch/*/4*/);
}

extern int SOM::colorkey(DX::DDSURFACEDESC2 *in, D3DFORMAT f)
{	
	int knockouts = EX::colorkey(in,f);
	
	int w = in->dwWidth, h = in->dwHeight;

	void *bits = in->lpSurface; int pitch = in->lPitch;
	
	//16 is probably obsolete. it depends on SomEx.cpp
	//it might be used in debugging. for analysis only
	if(knockouts) 
	{		
		//2021: AVOID EXPENSIVE PROCESSING ON WHAT ARE
		//LIKELY TITLE SCREENS (Moratheia)
		if(w>h&&h>=720) return knockouts;

		DWORD was = EX::tick();

		switch(in->ddpfPixelFormat.dwRGBBitCount) 
		{

			//WARNING! THIS IS (SOMEHOW) SLIGHTLY DISTORTING
			//THE COLORS (IN OPAQUE REGIONS) BY ABOUT 1 PIXEL
			//VALUE... ODD VALUES SEEM TO BECOME EVEN BUT NOT
			//ALWAYS??????????????????????!!!!!!!!!!!!!!!


		//0 is border mode. wrap is not fully implemented.
		case 16: som_colorkey<SHORT>(bits,pitch,w,h,0,0); break;
		case 32: som_colorkey<DWORD>(bits,pitch,w,h,0,0); break;
		}

		was = EX::tick()-was; if(was>1)
		{
			was = was; //breakpoint 
		}
	}

	return knockouts;
}

