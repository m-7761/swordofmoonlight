
#include "directx.h" 
DX_TRANSLATION_UNIT //(C)

	//PREAMBLE: this file implements conversion from
	//D3D7 to OpenGL ES 3 (via ANGLE) with semantics
	//that match the D3D7->D3D9 conversion this file
	//is based on (dx.d3d9c.cpp) in order to be able
	//to use OpenXR for VR sets. it also has another
	//D3D9 implementation that's streamlined to only
	//client shaders. I might try to add D3D10 later
	//it was just easier that way but it's also neat
	//to have a kind of Rosetta Stone

#define WIP assert(!'WIP');

#include <map>
#include <vector>

#include <ddraw.h>

	//REFERENCE
	//NOTE: this might be helpful later to add D3D10
	//or to do OpenXR interop with ANGLE
	//this was part of a failed experiment (doD3D12)
	#include <dxgi.h> //TESTING	
	#include <sal.h> //_In_ //VS2010	
	//SAL2
	#define _In_reads_(x)
	#define _In_reads_opt_(x)
	#define _In_reads_bytes_(x)
	#define _In_reads_bytes_opt_(x)
	#define _Out_writes_(x)
	#define _Out_writes_opt_(x)
	#define _Out_writes_bytes_(x)
	#define _Out_writes_bytes_opt_(x)
	#define _Out_writes_bytes_to_(x,y)
	#define _Out_writes_all_opt_(x)
	#define _Out_writes_to_opt_(x,y)
	#define _Inout_updates_bytes_(x)
	#define _Always_(x)
	#define _Outptr_opt_result_bytebuffer_(x)
	#define _COM_Outptr_
	#define _COM_Outptr_opt_	
	#define _Field_size_(x)
	#define _Field_size_bytes_full_(x)
	#define _Field_size_full_(x)
	#define _Field_size_full_opt_(x)
	#define _Field_size_bytes_full_opt_(x)
	//THESE AND winapifamily.h ARE SOURCED FROM THE 
	//THE "SOM_SDK" ENVIRONMENT VARIABLE CONTAINING
	//sdk.swordofmoonlight.net
	typedef struct _DXGI_RGBA{ float r,g,b,a; }DXGI_RGBA; //dxgitype.h
	#include <dxgi1_2.h> //CreateSwapChainForHwnd?
	//#include "dx12-headers/d3d12.h" //TESTING
	struct IDXGIFactory3 : IDXGIFactory2
	{
		virtual UINT STDMETHODCALLTYPE GetCreationFlags(void) = 0;        
	};
	MIDL_INTERFACE("1bc6ea02-ef36-464f-bf0c-21ca39e5168a")
	IDXGIFactory4 : IDXGIFactory3
	{
		virtual HRESULT STDMETHODCALLTYPE EnumAdapterByLuid(LUID,REFIID,void**) = 0;        
		virtual HRESULT STDMETHODCALLTYPE EnumWarpAdapter(REFIID,void**) = 0;        
	};

#ifdef _DEBUG
//#define D3D_DEBUG_INFO
#endif
#include <d3d9.h>
#include <d3dx9.h> //REMOVE ME?
#pragma comment(lib,"d3dx9.lib")
#ifdef D3D_DEBUG_INFO
#define DDRAW_ASSERT_REFS(x,y) ; //winsanity
#else 
static int DDRAW_ASSERT_REFS_refs;
#define DDRAW_ASSERT_REFS(x,y) if(DX::debug&&x) \
{ int &refs = DDRAW_ASSERT_REFS_refs; \
refs = -1+x->AddRef(); assert(refs y); x->Release(); }
#endif

//interface?
#define DDRAW_OVERLOADS	
#include "dx.d3d9c.h"

//this packages ANGLE. for SomEx it's built
//into Exselector.dll
#define WIDGETS_95_NOUI
#define WIDGETS_95_DELAYLOAD
#undef interface //MSVC
#define snprintf _snprintf //VS2010
#include <widgets95.h>
WIDGETS_95_DELAYLOAD_DEFINITION //delayload

//these aren't isolated to single uses
#define GL_DEPTH_TEST			0x0B71
#define GL_SCISSOR_TEST			0x0C11
#define GL_VIEWPORT				0x0BA2
#define GL_DEPTH_CLAMP			0x864F //GL_ARB_depth_clamp
#define GL_DEPTH_RANGE			0x0B70
#define GL_TEXTURE_2D			0x0DE1
#define GL_TEXTURE_WIDTH		0x1000
#define GL_TEXTURE_HEIGHT		0x1001
#define GL_TEXTURE_WRAP_S		0x2802
#define GL_TEXTURE_WRAP_T		0x2803
#define GL_CLAMP_TO_EDGE		0x812F
#define GL_REPEAT				0x2901
#define GL_MIRRORED_REPEAT		0x8370	
#define GL_TEXTURE_MAG_FILTER	0x2800
#define GL_NEAREST				0x2600
#define GL_LINEAR				0x2601
#define GL_TEXTURE_MIN_FILTER		0x2801
#define GL_NEAREST_MIPMAP_NEAREST	0x2700
#define GL_LINEAR_MIPMAP_NEAREST	0x2701
#define GL_NEAREST_MIPMAP_LINEAR	0x2702
#define GL_LINEAR_MIPMAP_LINEAR		0x2703
//#define GL_RGB8					0x8051
#define GL_RGBA8				0x8058
//#define GL_SRGB8				0x8C41
#define GL_SRGB8_ALPHA8			0x8C43
#define GL_R8					0x8229
#define GL_RED					0x1903
//#define GL_LUMINANCE			0x1909
#define GL_RGBA					0x1908
#define GL_UNSIGNED_BYTE		0x1401
#define GL_FLOAT				0x1406
#define GL_TEXTURE0				0x84C0
#define GL_COLOR_ATTACHMENT0	0x8CE0
#define GL_DEPTH_ATTACHMENT		0x8D00
//#define GL_DEPTH_STENCIL_ATTACHMENT	0x821A
#define GL_FRAMEBUFFER			0x8D40
#define GL_READ_FRAMEBUFFER		0x8CA8
#define GL_DRAW_FRAMEBUFFER		0x8CA9
#define GL_RENDERBUFFER			0x8D41
#define GL_ELEMENT_ARRAY_BUFFER	0x8893
#define GL_ARRAY_BUFFER			0x8892
#define GL_UNIFORM_BUFFER		0x8A11
//#define GL_TEXTURE_BUFFER		0x8C2A
#define GL_STREAM_DRAW			0x88E0
//#define GL_DYNAMIC_DRAW		0x88E8 //???
#define GL_FRAGMENT_SHADER		0x8B30
#define GL_VERTEX_SHADER		0x8B31
#define GL_VERTEX_SHADER_BIT	0x00000001
#define GL_FRAGMENT_SHADER_BIT	0x00000002
#define GL_COLOR_BUFFER_BIT		0x4000
//extensions?
#define GL_TEXTURE_MAX_ANISOTROPY_EXT		0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT	0x84FF

#pragma comment(lib,"opengl32.lib") //REMOVE ME?
#define DX_D3D9X_X \
X(glActiveTexture)\
X(glAttachShader)\
X(glBindBuffer)\
X(glBindBufferRange)\
X(glBindFramebuffer)\
X(glBindProgramPipeline)\
X(glBindRenderbuffer)\
X(glBindSampler)\
X(glBindTexture)\
X(glBindVertexArray)\
X(glBindVertexBuffer)\
X(glBlitFramebuffer)\
X(glBlendFunc)\
X(glBlendFuncSeparate)\
X(glBufferData)\
/*X(glBufferStorage)//needs 4.4, no ES support*/\
X(glBufferSubData)\
X(glClear)\
X(glClearBufferfv)\
X(glClearBufferfi)\
X(glClearColor)\
X(glClearDepthf)\
X(glClearStencil)\
X(glColorMaski)\
X(glCompileShader)\
X(glCreateProgram)\
X(glCreateShader)\
X(glCreateShaderProgramv)\
X(glCullFace)\
X(glDebugMessageCallback)\
X(glDeleteBuffers)\
X(glDeleteFramebuffers)\
X(glDeleteProgram)\
X(glDeleteProgramPipelines)\
X(glDeleteRenderbuffers)\
X(glDeleteSamplers)\
X(glDeleteShader)\
X(glDeleteTextures)\
X(glDeleteVertexArrays)\
X(glDepthFunc)\
X(glDepthMask)\
X(glDepthRangef)\
X(glDisable)\
X(glDisableVertexAttribArray)\
X(glDrawArrays)\
X(glDrawBuffers)\
X(glDrawRangeElements)\
X(glEnable)\
X(glEnableVertexAttribArray)\
X(glFlush)/*TESTING*/\
X(glFramebufferRenderbuffer)\
X(glFramebufferTexture)\
X(glFrontFace)/*TESTING*/\
X(glGenBuffers)\
X(glGenerateMipmap)/*TESTING*/\
X(glGenFramebuffers)\
X(glGenProgramPipelines)\
X(glGenRenderbuffers)\
X(glGenSamplers)\
X(glGenTextures)\
X(glGenVertexArrays)\
X(glGetError)\
X(glGetFloatv)\
X(glGetFramebufferAttachmentParameteriv)/*TESTING*/\
X(glGetIntegerv)\
X(glGetProgramiv)\
X(glGetProgramInfoLog)\
X(glGetShaderiv)\
X(glGetShaderInfoLog)\
X(glGetString)\
X(glGetTexLevelParameterfv)\
X(glGetTexLevelParameteriv)\
X(glLinkProgram)\
/*X(glMapBuffer) //OpenGL ES only has glMapBufferRange*/\
X(glMapBufferRange)\
X(glPixelStorei)\
/*X(glPolygonMode)*/\
X(glRenderbufferStorage)\
X(glSamplerParameteri)\
X(glScissor)\
/*X(glShadeModel)//UNUSED*/\
X(glShaderSource)\
X(glTexImage2D)\
X(glTexStorage2D)\
X(glTexSubImage2D)\
X(glUnmapBuffer)\
X(glUseProgram)\
X(glUseProgramStages)\
X(glVertexAttribBinding)\
X(glVertexAttribFormat)\
X(glVertexBindingDivisor)\
X(glViewport)\
//
#define X(f) static auto *f = 0?Widgets95::gle::f:0;
DX_D3D9X_X
static void(__stdcall*glPolygonMode)(GLenum,GLenum) = 0;
static void(__stdcall*glShadeModel)(GLenum) = 0; //UNUSED

//GL_NV_shading_rate_image (for fixed foveated rendering)
//
void(WINAPI*glBindShadingRateImageNV)(GLuint texture);
void(WINAPI*glShadingRateImagePaletteNV)(GLuint viewport, GLuint first, GLsizei count, const GLenum *rates);
//void(WINAPI*glGetShadingRateImagePaletteNV)(GLuint viewport, GLuint entry, GLenum *rate);
//void(WINAPI*glShadingRateImageBarrierNV)(GLboolean synchronize);
//what's the use case of these? (I can't find any writing on them)
//void(WINAPI*glShadingRateSampleOrderNV)(GLenum order);
//void(WINAPI*glShadingRateSampleOrderCustomNV)(GLenum rate, GLuint samples, const GLint *locations);
//void(WINAPI*glGetShadingRateSampleLocationivNV)(GLenum rate, GLuint samples, GLuint index, GLint *location);
//
//    New Tokens
//    Accepted by the <cap> parameter of Enable, Disable, and IsEnabled, by the
//    <target> parameter of Enablei, Disablei, IsEnabledi, EnableIndexedEXT,
//    DisableIndexedEXT, and IsEnabledIndexedEXT, and by the <pname> parameter
//    of GetBooleanv, GetIntegerv, GetInteger64v, GetFloatv, GetDoublev,
//    GetDoubleIndexedv, GetBooleani_v, GetIntegeri_v, GetInteger64i_v,
//    GetFloati_v, GetDoublei_v, GetBooleanIndexedvEXT, GetIntegerIndexedvEXT,
//    and GetFloatIndexedvEXT:
//
#define GL_SHADING_RATE_IMAGE_NV                           0x9563
//
//    Accepted in the <rates> parameter of ShadingRateImagePaletteNV and the
//    <rate> parameter of ShadingRateSampleOrderCustomNV and
//    GetShadingRateSampleLocationivNV; returned in the <rate> parameter of
//    GetShadingRateImagePaletteNV:
//
#define GL_SHADING_RATE_NO_INVOCATIONS_NV                  0x9564
#define GL_SHADING_RATE_1_INVOCATION_PER_PIXEL_NV          0x9565
#define GL_SHADING_RATE_1_INVOCATION_PER_1X2_PIXELS_NV     0x9566
#define GL_SHADING_RATE_1_INVOCATION_PER_2X1_PIXELS_NV     0x9567
#define GL_SHADING_RATE_1_INVOCATION_PER_2X2_PIXELS_NV     0x9568
#define GL_SHADING_RATE_1_INVOCATION_PER_2X4_PIXELS_NV     0x9569
#define GL_SHADING_RATE_1_INVOCATION_PER_4X2_PIXELS_NV     0x956A
#define GL_SHADING_RATE_1_INVOCATION_PER_4X4_PIXELS_NV     0x956B
//#define GL_SHADING_RATE_2_INVOCATIONS_PER_PIXEL_NV         0x956C
//#define GL_SHADING_RATE_4_INVOCATIONS_PER_PIXEL_NV         0x956D
//#define GL_SHADING_RATE_8_INVOCATIONS_PER_PIXEL_NV         0x956E
//#define GL_SHADING_RATE_16_INVOCATIONS_PER_PIXEL_NV        0x956F
//
//    Accepted by the <pname> parameter of GetBooleanv, GetDoublev,
//    GetIntegerv, and GetFloatv:
//
//#define GL_SHADING_RATE_IMAGE_BINDING_NV                   0x955B
#define GL_SHADING_RATE_IMAGE_TEXEL_WIDTH_NV               0x955C
#define GL_SHADING_RATE_IMAGE_TEXEL_HEIGHT_NV              0x955D
#define GL_SHADING_RATE_IMAGE_PALETTE_SIZE_NV              0x955E
//#define GL_MAX_COARSE_FRAGMENT_SAMPLES_NV                  0x955F
//
//    Accepted by the <order> parameter of ShadingRateSampleOrderNV:
//
//#define GL_SHADING_RATE_SAMPLE_ORDER_DEFAULT_NV            0x95AE
//#define GL_SHADING_RATE_SAMPLE_ORDER_PIXEL_MAJOR_NV        0x95AF
//#define GL_SHADING_RATE_SAMPLE_ORDER_SAMPLE_MAJOR_NV       0x95B0

namespace D3D9X //dx.d3d9X.h
{
	extern int is_needed_to_initialize();

	extern bool updating_texture(DDRAW::IDirectDrawSurface7*,int);

	using D3D9C::IDirectDraw;
	using D3D9C::IDirectDrawClipper;
	using D3D9C::IDirectDrawPalette;
	using D3D9C::IDirectDrawSurface;
	using D3D9C::IDirectDrawGammaControl;

	//template<int X>
	class IDirectDraw7 : public D3D9C::IDirectDraw7
	{
	DDRAW_INTRAFACE(IDirectDraw7) //public

		//Could remove IDirectDraw7 entirely, but it sets forth
		//some differences between this file and dx.d3d9c.cpp's
		
		HRESULT __stdcall CreateSurface(DX::LPDDSURFACEDESC2,DX::LPDIRECTDRAWSURFACE7*,IUnknown*);
	};
	//template<int X>
	class IDirectDrawSurface7 : public D3D9C::IDirectDrawSurface7
	{		
		HRESULT flip();
		template<int X> HRESULT flip(); //Flip/Blt subroutine

	DDRAW_INTRAFACE(IDirectDrawSurface7) //public

		//NOTE: Flip/Blt call flip subroutine
		HRESULT __stdcall Blt(LPRECT,DX::LPDIRECTDRAWSURFACE7,LPRECT,DWORD,DX::LPDDBLTFX);
	//	HRESULT __stdcall BltFast(DWORD,DWORD,DX::LPDIRECTDRAWSURFACE7,LPRECT,DWORD);
		HRESULT __stdcall Flip(DX::LPDIRECTDRAWSURFACE7,DWORD);

		//might want to implement these in the future but for now
		//I'm trying to leverage IDirect3DDevice9's ability to make
		//D3DPOOL_SYSTEMMEM surfaces so there should be less code
		//if it works. maybe some proxies of D3D9 could make OpenGL
		//fully compatible with DDRAW::compat=='dx9c' on client end

		//I'd already written some code for GetAttachedSurface and
		//GetSurfaceDesc that's easier to read than dx.d3d9c.cpp's
		HRESULT __stdcall GetAttachedSurface(DX::LPDDSCAPS2,DX::LPDIRECTDRAWSURFACE7*);
	//	HRESULT __stdcall GetDC(HDC*);
		HRESULT __stdcall GetSurfaceDesc(DX::LPDDSURFACEDESC2);
	//	HRESULT __stdcall Lock(LPRECT,DX::LPDDSURFACEDESC2,DWORD,HANDLE);
	//	HRESULT __stdcall ReleaseDC(HDC);
	//	HRESULT __stdcall SetColorKey(DWORD,DX::LPDDCOLORKEY);
	//	HRESULT __stdcall Unlock(LPRECT);
	};
	template<int X>
	class IDirect3D7 : public D3D9C::IDirect3D7
	{
	DDRAW_INTRAFACE(IDirect3D7) //public

		HRESULT __stdcall CreateDevice(REFCLSID,DX::LPDIRECTDRAWSURFACE7,DX::LPDIRECT3DDEVICE7*);
		HRESULT __stdcall CreateVertexBuffer(DX::LPD3DVERTEXBUFFERDESC,DX::LPDIRECT3DVERTEXBUFFER7*,DWORD);
	};
	template<int X>
	class IDirect3DDevice7 : public D3D9C::IDirect3DDevice7
	{
		//TODO: I don't know that these ever should've been class members. they could 
		//be useful if external code wanted them but, but through dx.ddraw.h instead?
		public: //friend dx_d3d9X_backblt;
		static HRESULT drawprims(DX::D3DPRIMITIVETYPE,DWORD,LPVOID,DWORD,LPWORD=0,DWORD=0);
		static HRESULT drawprims2(DX::D3DPRIMITIVETYPE,DWORD,LPVOID,DWORD,LPWORD=0,DWORD=0);

	DDRAW_INTRAFACE(IDirect3DDevice7) //public

		//unimplemented methods here are fixed-function that interfaces with shaders
		//through dx_d3d9X_p/vsconstant or are trivial for dx.d3d9c.cpp to implement 
		HRESULT __stdcall Clear(DWORD,DX::LPD3DRECT,DWORD,DX::D3DCOLOR,DX::D3DVALUE,DWORD);
	//	HRESULT __stdcall SetTransform(DX::D3DTRANSFORMSTATETYPE,DX::LPD3DMATRIX);
	//	HRESULT __stdcall GetTransform(DX::D3DTRANSFORMSTATETYPE,DX::LPD3DMATRIX);
		HRESULT __stdcall SetViewport(DX::LPD3DVIEWPORT7);
		HRESULT __stdcall GetViewport(DX::LPD3DVIEWPORT7);
	//	HRESULT __stdcall SetMaterial(DX::LPD3DMATERIAL7);
	//	HRESULT __stdcall GetMaterial(DX::LPD3DMATERIAL7);
	//	HRESULT __stdcall SetLight(DWORD,DX::LPD3DLIGHT7);
		HRESULT __stdcall SetRenderState(DX::D3DRENDERSTATETYPE,DWORD);
		HRESULT __stdcall GetRenderState(DX::D3DRENDERSTATETYPE,LPDWORD);
		HRESULT __stdcall DrawPrimitive(DX::D3DPRIMITIVETYPE,DWORD,LPVOID,DWORD,DWORD);
		HRESULT __stdcall DrawIndexedPrimitive(DX::D3DPRIMITIVETYPE,DWORD,LPVOID,DWORD,LPWORD,DWORD,DWORD);
		HRESULT __stdcall DrawIndexedPrimitiveVB(DX::D3DPRIMITIVETYPE,DX::LPDIRECT3DVERTEXBUFFER7,DWORD,DWORD,LPWORD,DWORD,DWORD);
	//	HRESULT __stdcall GetTexture(DWORD,DX::LPDIRECTDRAWSURFACE7*);
		HRESULT __stdcall SetTexture(DWORD,DX::LPDIRECTDRAWSURFACE7);
		HRESULT __stdcall GetTextureStageState(DWORD,DX::D3DTEXTURESTAGESTATETYPE,LPDWORD);
		HRESULT __stdcall SetTextureStageState(DWORD,DX::D3DTEXTURESTAGESTATETYPE,DWORD);
		//dx.d3d9c.cpp knows enough about OpenGL
		//to manage this
	//	HRESULT __stdcall ApplyStateBlock(DWORD);
	//	HRESULT __stdcall CaptureStateBlock(DWORD);
	//	HRESULT __stdcall DeleteStateBlock(DWORD);
	//	HRESULT __stdcall CreateStateBlock(DX::D3DSTATEBLOCKTYPE,LPDWORD);
	//	HRESULT __stdcall LightEnable(DWORD,BOOL);
	};
	template<int X>
	class IDirect3DVertexBuffer7 : public D3D9C::IDirect3DVertexBuffer7
	{
	DDRAW_INTRAFACE(IDirect3DVertexBuffer7) //public

		HRESULT __stdcall Lock(DWORD,LPVOID*,LPDWORD);
		HRESULT __stdcall Unlock();
	};
}

extern const D3DPOOL dx_d3d9c_texture_pool; //D3DPOOL_SYSTEMMEM

//NOTE: defining in this file enables static_assert style usage
extern const bool dx_d3d9c_immediate_mode = false;
extern const bool dx_d3d9c_imitate_colorkey;
extern const bool dx_d3d9c_stereo_scissor; //2021

extern IDirect3DDevice9 *dx_d3d9c_old_device;
extern int dx_d3d9c_force_release_old_device();
static auto*const dx_d3d9c_virtual_group = (IDirect3DTexture9*)8;

//TODO: THESE SHOULD GO INSIDE Query9
extern UINT dx_d3d9c_ibuffers_s[D3D9C::ibuffersN]; 
extern UINT dx_d3d9c_vbuffers_s[D3D9C::ibuffersN]; 
extern IDirect3DIndexBuffer9 *dx_d3d9c_ibuffers[D3D9C::ibuffersN]; 
extern IDirect3DVertexBuffer9 *dx_d3d9c_vbuffers[D3D9C::ibuffersN]; 
static auto &dx_d3d9X_ibuffers = (GLuint(&)[D3D9C::ibuffersN])dx_d3d9c_ibuffers;
static auto &dx_d3d9X_vbuffers = (GLuint(&)[D3D9C::ibuffersN])dx_d3d9c_vbuffers;
extern IDirect3DVertexShader9 *dx_d3d9c_vshaders[16+4];
extern IDirect3DPixelShader9 *dx_d3d9c_pshaders[16+4];
extern IDirect3DPixelShader9 *dx_d3d9c_pshaders2[16+4];
extern IDirect3DPixelShader9**dx_d3d9c_pshaders_noclip;
enum{ dx_d3d9X_shaders_are_programs=0 };
static auto &dx_d3d9X_vshaders = (GLuint(&)[16+4])dx_d3d9c_vshaders;
static auto &dx_d3d9X_pshaders = (GLuint(&)[16+4])dx_d3d9c_pshaders;
static auto &dx_d3d9X_pshaders2 = (GLuint(&)[16+4])dx_d3d9c_pshaders2;
static auto &dx_d3d9X_pshaders_noclip = (GLuint*&)dx_d3d9c_pshaders_noclip;
extern IDirect3DSurface9 *dx_d3d9c_alphasurface;
extern IDirect3DTexture9 *dx_d3d9c_alphatexture;
extern DWORD dx_d3d9c_anisotropy_max; 
extern DWORD dx_d3d9c_beginscene, dx_d3d9c_endscene;

extern bool dx_d3d9c_effectsshader_enable;
extern bool dx_d3d9c_effectsshader_toggle;
extern void dx_d3d9X_enableeffectsshader(bool enable);
extern void dx_d3d9c_discardeffects();

extern void dx_d3d9c_vstransform(int dirty=0);
extern void dx_d3d9c_vslight(bool clean=true);
extern void dx_d3d9c_vstransformandlight();
extern void dx_d3d9c_update_vsPresentState(D3DVIEWPORT9 &vp);
extern void dx_d3d9X_vsconstant(int reg, const D3DCOLORVALUE &set, int mode=1);
extern void dx_d3d9X_vsconstant(int reg, const D3DMATRIX &set, int mode=4);
extern void dx_d3d9X_vsconstant(int reg, const DWORD &set, int mode);
extern void dx_d3d9X_vsconstant(int reg, const float &set, int mode);
extern void dx_d3d9X_vsconstant(int reg, const float *set, int mode=1);
extern void dx_d3d9X_psconstant(int reg, const DWORD &set, int mode);
extern void dx_d3d9X_psconstant(int reg, const float &set, int mode);
extern void dx_d3d9X_psconstant(int reg, const float *set, int mode=1);
extern void dx_d3d9c_level(IDirect3DPixelShader9 *ps, bool reset=false);

extern const int dx_d3d9c_formats_enumN;
extern const D3DFORMAT dx_d3d9c_formats_enum[];
extern const DX::DDPIXELFORMAT &dx_d3d9c_format(D3DFORMAT f);
extern D3DFORMAT dx_d3d9c_format(DX::DDPIXELFORMAT &f);
extern UINT dx_d3d9c_sizeofFVF(DWORD in);

//dx_d3d9X_d3ddrivercaps: does not set dwDeviceZBufferBitDepth
extern DX::D3DDEVICEDESC7 &dx_d3d9c_d3ddrivercaps(D3DCAPS9 &in, DX::D3DDEVICEDESC7 &out);

extern int dx_d3d9c_ibuffer_i;
extern int dx_d3d9c_vbuffer_i;
extern HRESULT dx_d3d9c_ibuffer(IDirect3DDevice9 *d, IDirect3DIndexBuffer9* &ib, DWORD r, LPWORD q);
extern HRESULT dx_d3d9c_vbuffer(IDirect3DDevice9 *d, IDirect3DVertexBuffer9* &vb, DWORD qN, LPVOID q);

extern bool dx_d3d9c_colorkeyenable; 
extern bool dx_d3d9c_alphablendenable; //???

extern bool dx_d3d9c_autogen_mipmaps;
extern void dx_d3d9c_mipmap(DDRAW::IDirectDrawSurface7*p);
extern IDirect3DSurface9 *dx_d3d9c_alpha(IDirect3DSurface9*);
extern IDirect3DTexture9 *dx_d3d9c_alpha(IDirect3DTexture9*);

extern IDirect3DStateBlock9 *dx_d3d9c_bltstate;
static auto &dx_d3d9X_bltstate = (DWORD&)dx_d3d9c_bltstate;
extern HRESULT dx_d3d9c_backblt(IDirect3DTexture9 *texture, RECT &src, RECT &dst, IDirect3DPixelShader9 *ps, int vs=-1, DWORD white=0xFFFFFF);

extern LONG dx_d3d9c_pitch(IDirect3DSurface9 *p);
extern LONG dx_d3d9c_pitch(IDirect3DTexture9 *p);
extern bool dx_d3d9c_colorfill(IDirect3DSurface9 *p, RECT *r, D3DCOLOR c);
extern char dx_d3d9c_dither65[];
extern bool dx_d3d9c_dither(IDirect3DTexture9 *p, int n, bool dither=true);

struct DDRAW::IDirect3DDevice::QueryGL::UniformBlock
{
	enum{ chunk=8*4 }; //8 registers

	UniformBlock(){ memset(this,0x00,sizeof(*this)); }
	~UniformBlock(){ delete[] f; }

	int dirty_mask; //TODO? fixed sized buffers?

	//BOOL is unused and too small for the 256B
	//alignment ANGLE chooses for Direct3D 11. I
	//don't even know how OpenGL implements BOOL
	//int b[16];
	float *f;
	int fmost;
	int i[chunk*2]; //256B	

	void dirty(); //UNUSED?
	void dirty(int,int*,int);
	void dirty(int,float*,int);
	void flush(int);
};

static bool dx_d3d9X_xpaused = false; //xcenter

extern HRESULT dx_d3d9c_present(IDirect3DDevice9Ex *p);
static HRESULT dx_d3d9X_present()
{
	int cmp = DDRAW::Direct3DDevice7->target;
	if(cmp=='dx9c') 
	return dx_d3d9c_present(DDRAW::Direct3DDevice7->proxy9);	

	auto *q = DDRAW::Direct3DDevice7->queryGL;

	DWORD ticks_before_wait = DX::tick(); //EXPEREMENTAL

	if(DDRAW::inStereo) //OpenXR?
	{
		//EXPERIMENTAL
		//may as well do this here, there's no other way
		//to get a meaningful value for DDRAW::flipTicks.
		cmp = q->xr->xwait();
		
		if(dx_d3d9X_xpaused&&!DDRAW::isPaused)
		{
			dx_d3d9X_xpaused = false;			

			q->xr->xcenter();
		}

		if(DDRAW::noTicks)
		{
			if(cmp) //2024
			{
				float ms = cmp/1000000.0f;
				DDRAW::refreshrate = (int)(1000/ms+0.5f);
			}
			else
			{
				assert(cmp); //2024

				//2023: can't poll refresh rate
				DDRAW::refreshrate = (1.0f/DDRAW::noTicks)*1000+0.5f;				
			}
			DDRAW::refreshrate = min(240,max(60,DDRAW::refreshrate));
		}
	}
	else
	{	
		if(q->xr) cmp = Widgets95::xr::swap_buffers();
		else if(q->wgl) cmp = SwapBuffers(q->wgldc); //wglGetCurrentDC()
		else{ cmp = false; assert(cmp); }
	}

	if(cmp) DDRAW::flipTicks = DX::tick()-ticks_before_wait;
	return !cmp;
}

extern void dx_d3d9c_reset();
extern void dx_d3d9X_resetdevice();

extern int dx_d3d9X_gl_viewport[4] = {};
static HRESULT dx_d3d9X_viewport(bool X, 
D3DVIEWPORT9 &vp, DDRAW::IDirect3DDevice7 *p=DDRAW::Direct3DDevice7)
{
	//2022: OpenXR doesn't maintain SetViewport
	//if(X) glGetIntegerv(GL_VIEWPORT,(int*)&vp);
	if(X) memcpy(&vp,dx_d3d9X_gl_viewport,4*sizeof(int));
	if(X) glGetFloatv(GL_DEPTH_RANGE,&vp.MinZ);
	if(!X) return p->proxy9->GetViewport(&vp);
	return D3D_OK; //ERROR?
}
extern void DDRAW::dejagrate_update_vsPresentState()
{
	if(DDRAW::compat!='dx9c'){ assert(0); return; }
	D3DVIEWPORT9 vp; dx_d3d9X_viewport(DDRAW::gl,vp); 
	dx_d3d9c_update_vsPresentState(vp);	
}

//this was part of a failed experiment (doD3D12)
//but still briefer than GetBackBuffer I suppose
static IDirect3DSurface9 *dx_d3d9X_backbuffer9()
{
	assert(!DDRAW::D3D12Device);
//	auto bbuffer = 
//	1?0:DDRAW::doD3D12_backbuffers[0].d3d9; //2021
	IDirect3DSurface9 *bbuffer = 0;
	if(!bbuffer)
	DDRAW::Direct3DDevice7->proxy9
	->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&bbuffer);
	else bbuffer->AddRef();
	return bbuffer;
}

static void __stdcall dx_d3d9X_glMsg_db(GLenum src, GLenum type, GLuint id, GLenum severity, GLsizei len, const char *msg, const void *up)
{
	assert(src==0x8246||src==0x8248); //GL_DEBUG_SOURCE_API||GL_DEBUG_SOURCE_SHADER_COMPILER

	//OutputDebugStringA(msg); DEBUGGING

	OutputDebugStringA(msg); OutputDebugStringA("\r\n"); //???

	//GL_DEBUG_TYPE_OTHER?
	if(type==0x8251) return; //Nvidia "hints" are pure noise/just feedback (not issues)
	
	//getting "API_ID_RECOMPILE_FRAGMENT_SHADER performance warning has been generated. Fragment shader recompiled due to state change."
	//on my Intel system
	if(type==0x8250) //GL_DEBUG_TYPE_PERFORMANCE
	{
		if(id==2) return; //API_ID_RECOMPILE_FRAGMENT_SHADER?
	}
	msg = msg; //breakpoint

//	OutputDebugStringA(msg);
}

////////////////////////////////////////////////////////
//              DIRECTX9 INTERFACES                   //
////////////////////////////////////////////////////////


//extern: dx.ddraw.cpp
extern void *dx_ddraw_hacks[DDRAW::TOTAL_HACKS]; 

#define DDRAW_PUSH_HACK(h,...)\
\
	HRESULT out = 1; void *hP,*hK;\
	for(hP=0,hK=dx_ddraw_hacks?dx_ddraw_hacks[DDRAW::h##_HACK]:0;hK;hK=0)hP=((void*(*)(HRESULT*,DDRAW::__VA_ARGS__))hK)

#define DDRAW_POP_HACK(h,...)\
\
	pophack: if(hP) ((void*(*)(HRESULT*,DDRAW::__VA_ARGS__))hP)

#define DDRAW_PUSH_HACK_IF(cond,h,...)\
\
	HRESULT out = 1;\
	void *hP=0,*hK=dx_ddraw_hacks?dx_ddraw_hacks[DDRAW::h##_HACK]:0;\
	if(cond) for(;hK;hK=0)hP=((void*(*)(HRESULT*,DDRAW::__VA_ARGS__))hK)

extern DDRAW::IDirectDrawSurface7 *DDRAW::CreateAtlasTexture(int sz)
{
	DX::DDSURFACEDESC2 desc = {sizeof(DX::DDSURFACEDESC2)};

	desc.dwFlags|=DDSD_HEIGHT|DDSD_WIDTH|DDSD_CAPS|DDSD_PIXELFORMAT;
	desc.dwWidth = desc.dwHeight = sz;

	desc.ddpfPixelFormat = dx_d3d9c_format(D3DFMT_A8R8G8B8);
	desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE|DDSCAPS_VIDEOMEMORY|DDSCAPS_WRITEONLY;

	DX::LPDIRECTDRAWSURFACE7 y = nullptr;

	DDRAW::DirectDraw7->CreateSurface(&desc,&y,0); assert(y); return (DDRAW::IDirectDrawSurface7*)y;
}
extern void dx_d3d9X_updating_texture2(DDRAW::IDirectDrawSurface7*,int,int);
extern void DDRAW::BltAtlasTexture(DDRAW::IDirectDrawSurface7 *atlas, DDRAW::IDirectDrawSurface7 *p, int border, int x, int y)
{
	const bool X = DDRAW::gl;

	POINT pt; pt.x = x; pt.y = y;

	auto d3dd9 = DDRAW::Direct3DDevice7->proxy9;

	IDirect3DSurface9 *s,*t; p->queryX->group9->GetSurfaceLevel(0,&s);

	if(X)
	{
		//glBindTexture(GL_TEXTURE_2D,atlas->queryX->updateGL); 
		dx_d3d9X_updating_texture2(p,x,y);

		//HACK: just reset glBindTexture 
		if(p!=DDRAW::TextureSurface7[0])
		DDRAW::Direct3DDevice7->SetTexture(0,DDRAW::TextureSurface7[0]);
	}
	else
	{
		auto q = atlas->queryX; if(!q->update9) //2021 (updating_texture)
		{
			//int lvs = g?g->GetLevelCount():1; 
			int lvs = max(1,q->mipmaps);
			d3dd9->CreateTexture(q->width,q->height,lvs,0,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&q->update9,0);
		}
		for(unsigned i=0;i<atlas->queryX->mipmaps;i++)
		{
			q->update9->GetSurfaceLevel(i,&t);

			d3dd9->UpdateSurface(s,nullptr,t,&pt); //TODO border/mipmaps (render target?)

			t->Release(); pt.x/=2; pt.y/=2; //good enough?
		}
	}

	s->Release();
}
HRESULT D3D9X::IDirectDraw7::CreateSurface(DX::LPDDSURFACEDESC2 x, DX::LPDIRECTDRAWSURFACE7 *y, IUnknown *z)
{
	DDRAW_LEVEL(7) << "D3D9X::IDirectDraw7::CreateSurface()\n";
		
	if(!DDRAW::Direct3DDevice7) reset: //virtual_group?
	{
		return D3D9C::IDirectDraw7::CreateSurface(x,y,z);
	}
	else if(x->dwFlags&DDSD_CAPS&&x->ddsCaps.dwCaps&DDSCAPS_PRIMARYSURFACE) 
	{
		//HACK: assuming the device is thought to be lost / being reset

		dx_d3d9X_resetdevice();	dx_d3d9c_reset(); goto reset;			
	}

	//DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)   
		
	DDRAW_PUSH_HACK(DIRECTDRAW7_CREATESURFACE,IDirectDraw7*,
	DX::LPDDSURFACEDESC2&,DX::LPDIRECTDRAWSURFACE7*&,IUnknown*&)(0,this,x,y,z);
		
	if(!x||!y) DDRAW_FINISH(out)

	if(DDRAWLOG::debug>=8) //#ifdef _DEBUG
	{
#ifdef _DEBUG
#define OUT(Y) if(x->ddsCaps.dwCaps&DDSCAPS_##Y) DDRAW_LEVEL(8) << ' ' << #Y << '\n';
						   
	OUT(PRIMARYSURFACE)
	OUT(FRONTBUFFER)
	OUT(BACKBUFFER)
	OUT(ZBUFFER)
	OUT(ALPHA)
	OUT(FLIP)
	OUT(OFFSCREENPLAIN)
	OUT(PALETTE)
	OUT(SYSTEMMEMORY)
	OUT(TEXTURE)
	OUT(3DDEVICE)
	OUT(VIDEOMEMORY)
	OUT(VISIBLE)
	OUT(WRITEONLY)
	OUT(OWNDC)
	OUT(LIVEVIDEO)
	OUT(HWCODEC)
	OUT(MIPMAP)
	OUT(ALLOCONLOAD)
	OUT(VIDEOPORT)
	OUT(LOCALVIDMEM)
	OUT(NONLOCALVIDMEM)
	OUT(STANDARDVGAMODE)
	OUT(OPTIMIZED)

#undef OUT
#endif
	}

	assert(x->dwSize==sizeof(DX::DDSURFACEDESC2)); //paranoia

	out = !DD_OK;
						  
	if((x->dwFlags&(DDSD_HEIGHT|DDSD_WIDTH))==0)
	{
		DDRAW_FINISH(DDRAW_UNIMPLEMENTED)
	}

	DDRAW_LEVEL(7) << x->dwWidth << " x " << x->dwHeight << "\n";
				
	//Note: D3DUSAGE_DYNAMIC required for GDI (GetDC) support
	DWORD usage = D3DUSAGE_DYNAMIC;

	D3DPOOL pool = dx_d3d9c_texture_pool;
	/*dx_d3d9c_texture_pool should be set to SYSTEMMEM if not
	if(x->ddsCaps.dwCaps&DDSCAPS_SYSTEMMEMORY)
	pool = D3DPOOL_SYSTEMMEM;*/
	assert(pool==D3DPOOL_SYSTEMMEM);

	//2021: simplifing versus dx.dd3d9c.cpp
	//
	// WARNING: this won't work with Lock/Unlock unless the
	// caller respects DDSURFACEDESC2::ddpfPixelFormat being
	// different from the size used to create the surface
	//
	// For OpenGL that will require a tempory buffer, since
	// it doesn't really have the concept of system buffers
	// maybe EGL pixmaps?
	//
	D3DFORMAT client = dx_d3d9c_format(x->ddpfPixelFormat);
	D3DFORMAT format = client==D3DFMT_A8?client:D3DFMT_A8R8G8B8;

	LPDIRECT3DSURFACE9 s = 0;
	LPDIRECT3DTEXTURE9 t = 0; GLuint teX = 0; //2021
		
	//auto d3ddev = X?0:DDRAW::Direct3DDevice7->proxy9;
	auto d3ddev = DDRAW::Direct3DDevice7->proxy9; assert(d3ddev);

	//REMINDER: GetDesc is using 0==mipmaps to infer
	//DDSCAPS_OFFSCREENPLAIN
	int mipmaps = 0;
	int width = x->dwWidth, height = x->dwHeight;

	if(x->ddsCaps.dwCaps&DDSCAPS_OFFSCREENPLAIN)
	{
		// a true offscreen "plain" surface won't work here //
		// because d3dd9 style blitting is missing from d3d //

		/*if(X) //see D3D9X::updating_texture
		{
			glGenTextures(1,&teX);
		}
		else*/ if(pool!=D3DPOOL_DEFAULT)
		{
			//NOTE: this is just for group9/update9 pattern
			out = d3ddev->CreateOffscreenPlainSurface(width,height,format,pool,&s,0);
		}
		else 
		{					
			//hack: D3D9 just won't map a texture from system memory!!
			////pool = D3DPOOL_DEFAULT;
			assert(0);
			//out = d3ddev->CreateTexture(width,height,1,usage,format,pool,&t,0);
			out = !D3D_OK;
		}

		if(out==D3D_OK&&t) out = t->GetSurfaceLevel(0,&s);
	}
	else if(x->ddsCaps.dwCaps&DDSCAPS_TEXTURE)
	{
		if(DDRAW::doMipmaps&&dx_d3d9c_autogen_mipmaps)
		{
			usage|=D3DUSAGE_AUTOGENMIPMAP;
		}
		else if(!DDRAW::doMipmaps)			
		{
			if(x->ddsCaps.dwCaps&DDSCAPS_MIPMAP)
			{
				if(x->dwFlags&DDSD_MIPMAPCOUNT) 
				{
					mipmaps = x->dwMipMapCount; //appropriate??
				}
			}
			else mipmaps = 1;
		}
		else if(!DDRAW::mipmap) mipmaps = 1; //2021

		if(!mipmaps) //OpenGL?
		{
			//NOTE: for 300x300 ceil is generating 10, which is not
			//accepted by CreateTexture. somewhere I saw floor in 
			//use instead. OpenGL sucks (as usual) here
			//mipmaps = ceil(log2(max(width,height)))+1; 
			mipmaps = (int)floor(log((double)max(width,height))/log(2.0))+1;
		}

		/*if(X) //see D3D9X::updating_texture
		{
			glGenTextures(1,&teX);
		}
		else*/
		{
			out = d3ddev->CreateTexture(width,height,mipmaps,usage,format,pool,&t,0);
			DDRAW_ASSERT_REFS(t,==1)
			if(out==D3D_OK)
			{				
				out|=t->GetSurfaceLevel(0,&s); DDRAW_ASSERT_REFS(t,==2)
			}
		}
	}
	else DDRAW_FINISH(DDRAW_UNIMPLEMENTED)	

	DDRAW::IDirectDrawSurface7 *p = 0;

	if(s&&out==D3D_OK)
	{
		p = new DDRAW::IDirectDrawSurface7(DDRAW::target);

		*y = (DX::LPDIRECTDRAWSURFACE7)p;

		p->queryX->nativeformat = client; //alpha?

		p->queryX->format = format; //2021

		//2021
		p->queryX->isTextureLike = true;
		p->queryX->width = width;
		p->queryX->height = height;		
		p->queryX->mipmaps = mipmaps;
		p->queryX->mipmap_f = DDRAW::mipmap;
		p->queryX->colorkey_f = DDRAW::colorkey;

		/*if(X) //see D3D9X::updating_texture
		{
			//glBindTexture(GL_TEXTURE_2D,teX); //HOOK
			//glTexStorage2D(GL_TEXTURE_2D,max(1,mipmaps),sfmt,width,height);
		
			p->queryX->group = this;
			p->queryX->updateGL = teX;
			p->queryX->pitch = 4*width; //GL_RGBA8
		}
		else*/
		{
			p->query9->pitch = dx_d3d9c_pitch(s);

			assert(4*width==p->query9->pitch||x->ddpfPixelFormat.dwRGBBitCount==8);

			//note: group9 only set for textures		
			if(x->ddsCaps.dwCaps&DDSCAPS_TEXTURE) //plain surfaces are textures now
			if(s->GetContainer(__uuidof(IDirect3DTexture9),(void**)&p->query9->group9))			
			assert(!p->query9->group9); //paranoia
			
			DDRAW_ASSERT_REFS(p->query9->group9,==3)

			/*if(dx_d3d9c_texture_pool==D3DPOOL_DEFAULT) 
			{
				p->query9->update9 = t; if(t) t->AddRef(); else assert(0);

				DDRAW_ASSERT_REFS(p->query9->group9,==4)
			}*/
			assert(dx_d3d9c_texture_pool!=D3DPOOL_DEFAULT);

			s->AddRef(); //WHY? because Release is called below???

			p->proxy9 = s; 

			DDRAW_ASSERT_REFS(p->query9->group9,>=3)
		}
	}
	//if(!X)
	{
		if(s) s->Release(); if(t) t->Release();

		if(p) DDRAW_ASSERT_REFS(p->query9->group9,==2)
	}
	
	if(y) DDRAW_LEVEL(7) << "Created Surface (" << (unsigned)*y << ")\n";
		
	DDRAW_POP_HACK(DIRECTDRAW7_CREATESURFACE,IDirectDraw7*,
	DX::LPDDSURFACEDESC2&,DX::LPDIRECTDRAWSURFACE7*&,IUnknown*&)(&out,this,x,y,z);

	if(!x||!y) return out; //hack: assuming short-circuited by hack

		//Known behavior:
	//Sword of Moonlight will request comically large width
	//2022: today width was exactly 4096*4 and height was 67108864
	//NOTE: bad Gimp formatted BMP produces this... I know if makes wrong
	//BMP files from the past, that I have to re-save from Paint.net. I remember
	//there is something incorrect with their format
	if(x->dwWidth>4096*4||x->dwHeight>4096*4)
	{
		assert(0); return out; //2021: should be able to fix at source with Ghidra
	}
	
	DDRAW_RETURN(out)
}

typedef unsigned __int64 QWORD;
static GLuint dx_d3d9X_pipeline;
static std::vector<QWORD> dx_d3d9X_pipelines;
static void dx_d3d9X_flushaders(int vs=DDRAW::vs, int ps=DDRAW::ps)
{
	//TODO? maybe defer queryGL->state until here

	//REMINDER: flush below depends this going first
	if(ps!=DDRAW::fx) dx_d3d9c_vstransformandlight();

	assert(vs>=0&&dx_d3d9X_vshaders[vs]);
	assert(ps>=0&&dx_d3d9X_pshaders_noclip[ps]);
		
	if(DDRAW::target=='dx9c')
	{
		auto d3dd9 = DDRAW::Direct3DDevice7->proxy9;
		d3dd9->SetVertexShader(dx_d3d9c_vshaders[vs]);
		d3dd9->SetPixelShader(dx_d3d9c_pshaders_noclip[ps]);
	}
	else
	{
		assert(DDRAW::gl);
		
		auto q = DDRAW::Direct3DDevice7->queryGL;
		auto u2 = q->uniforms2;
		for(int i=2;i-->0;) if(u2[i].dirty_mask)
		{
			u2[i].flush(q->uniforms[i]);
		}

		unsigned hash = ps|vs<<16; assert(hash);

		if(dx_d3d9X_pshaders2==dx_d3d9X_pshaders_noclip)
		{
			hash|=0x8000;
		}

		if(dx_d3d9X_pipeline!=hash)
		{
			dx_d3d9X_pipeline = hash;

			//ANGLE doesn't actually support pipelines
			enum{ sap=dx_d3d9X_shaders_are_programs };

			GLuint prog;
			QWORD *v = dx_d3d9X_pipelines.data();
			int i = (int)dx_d3d9X_pipelines.size();
			while(i-->0)
			{
				if((v[i]&~0u)==hash)
				{
					prog = (GLuint)(v[i]>>32);
					break;
				}
			}
			if(i==-1)
			{
				if(sap) glGenProgramPipelines(1,&prog);
				else prog = glCreateProgram();
								
				if(sap) glUseProgramStages(prog,GL_VERTEX_SHADER_BIT,dx_d3d9X_vshaders[vs]);
				else glAttachShader(prog,dx_d3d9X_vshaders[vs]);
				if(sap) glUseProgramStages(prog,GL_FRAGMENT_SHADER_BIT,dx_d3d9X_pshaders_noclip[ps]);			
				else glAttachShader(prog,dx_d3d9X_pshaders_noclip[ps]);
				if(!sap) glLinkProgram(prog);

				dx_d3d9X_pipelines.push_back((QWORD)prog<<32|(QWORD)hash);
			}

			(sap?glBindProgramPipeline:glUseProgram)(prog);
		}
	}
}
static bool dx_d3d9X_prepareblt(bool X, int abe=0)
{
	if(!X) //fewer states than dx.d3d9c.cpp
	{
		auto p = DDRAW::Direct3DDevice7->proxy9;

		if(!dx_d3d9c_bltstate) 
		{
			if(p->CreateStateBlock(D3DSBT_ALL,&dx_d3d9c_bltstate)!=D3D_OK) return false;
		}
		else if(dx_d3d9c_bltstate->Capture()!=D3D_OK) return false;

		p->SetRenderState(D3DRS_ZENABLE,0);	
		//2021: I think redundant with ZENABLE off
		//p->SetRenderState(D3DRS_ZWRITEENABLE,0);	
		p->SetRenderState(D3DRS_ALPHABLENDENABLE,abe); 
		if(abe) //DDRAW_PUSH_HACK extension
		{		
			p->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);		
			p->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
		}
		//p->SetRenderState(D3DRS_SHADEMODE,D3DSHADE_FLAT); //???
		p->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);
		p->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
	
		p->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);
		p->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);
	}
	else
	{
		auto &st = DDRAW::Direct3DDevice7->queryGL->state;

		dx_d3d9X_bltstate = st.dword;
				
		st.zenable = 0;
		//st.zwriteenable = 0; //rendundant?
		st.alphablendenable = abe;
		if(abe)
		{
			st.srcblend = D3DBLEND_SRCALPHA;
			st.destblend = D3DBLEND_INVSRCALPHA;
		}
		st.fillmode = true; //D3DFILL_SOLID;
		st.cullmode = D3DCULL_NONE;	
		st.addressu = st.addressv = D3DTADDRESS_CLAMP; 
		
		st.apply(dx_d3d9X_bltstate);		
	}
	return true;
}
static bool dx_d3d9X_cleanupblt(bool X)
{
	bool out = true; if(X)
	{	
		auto q = DDRAW::Direct3DDevice7->queryGL;

		//TODO: and shader constants?
		auto cmp = q->state;
		q->state = dx_d3d9X_bltstate;
		q->state.apply(cmp);
	}
	else if(!dx_d3d9c_bltstate||dx_d3d9c_bltstate->Apply())
	{
		out = false;
	}
	//2021: dx_d3d9c_bltstate should include constants??
	//DDRAW::refresh_state_dependent_shader_constants();
	return out;
}
extern HRESULT dx_d3d9X_backblt(bool X, void *texture, RECT &src, RECT &dst, int ps, int vs, DWORD white=0xFFFFFF)
{
	if(!X) //TEMPORARY? TEMPLATE?
	return dx_d3d9c_backblt((IDirect3DTexture9*)texture,src,dst,dx_d3d9c_pshaders_noclip[ps],vs,white);

	auto d = DDRAW::Direct3DDevice7;

	int width,height;
	glBindTexture(GL_TEXTURE_2D,(GLuint)texture); //HOOK
	glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT,&height);
	
	enum{ fvf=D3DFVF_XYZW|D3DFVF_DIFFUSE|D3DFVF_TEX2 }; //D3DFVF_TEX1

	float l = src.left, r = src.right;	
	float t = src.top, b = src.bottom, c = 0; //hack...

	//bool fx = texture==DDRAW::Direct3DDevice7->query9->effects[0];
	bool fx = ps==DDRAW::fx;
	if(fx) //hack
	{
		//TODO: filter stuff can be done in CreateDevice
		//since OpenGL (ES) doesn't have sampler objects

		DWORD filter = D3DTEXF_POINT;
		DWORD mipfilter = //D3DTEXF_NONE
		DDRAW::doMipSceneBuffers?D3DTEXF_POINT:D3DTEXF_NONE;

		if(DDRAW::inStereo)
		{
			//undistortion requires linear
			filter = D3DTEXF_LINEAR; 
			
			if(DDRAW::do2ndSceneBuffer
			 &&DDRAW::doMipSceneBuffers)
			{
				if(!DDRAW::ps2ndSceneMipmapping2)
				mipfilter = D3DTEXF_LINEAR;
			}
			else if(DDRAW::doSmooth) 
			{
				//warning: the .5 pixel offset probably
				//somewhat defeats undistortion filters
				goto smooth;
			}
		}
		else if(DDRAW::doSmooth) //sample in the middle of the pixels
		{
			filter = D3DTEXF_LINEAR; 
			
			//NOTE: I tested this with do_aa disabled... it's hard to
			//say if it's accurate otherwise. fxInflateRect had thrown
			//it off, but it somehow seemed more stable before?
			if(DDRAW::doSuperSampling)
			{
				//WARNING: I've not adjusted fxInflateRect so that this
				//samples a little bit randomly across the entire image
				//this just helps with high-contrast movements. because
				//I guess that keeps the pixels from synchronizing

				//I think this is correct because the shader is using
				//tex2dlod to sample the first mipmap between the four
				//pixels that become one pixel in the mipmap
				if(1){ l+=0.98f; t+=0.98f; r+=0.98f; b+=0.98f; }
				else{ l+=1; t+=1; r+=1; b+=1; }
			}
			else smooth:
			{
				//2020: .49 is for filter2 //???
				if(1){ l+=0.49f; t+=0.49f; r+=0.49f; b+=0.49f; }
				else{ l+=0.5f; t+=0.5f; r+=0.5f; b+=0.5f; }
			}
		}
		//works magic for DDRAW::ps2ndSceneMipmapping2 on my Intel
		//system... I guess it could help nearest neighbor broadly
		//if(1&&DDRAW::ps2ndSceneMipmapping2)
		if(filter==D3DTEXF_POINT)
		{
			//EXPERIMENTAL
			//in theory mipmap effects can benefit from linear 
			//sample by pulling in their neightborhood?
			if(DDRAW::doMipSceneBuffers)
			{
				filter = D3DTEXF_LINEAR; //EXPERIMENTAL
			}

			float x = 0.001f; l+=x; t+=x; r+=x; b+=x; //Intel
		}
		
		if(X)
		{
			bool linear = filter==D3DTEXF_LINEAR;
			GLint minf,magf = linear?GL_LINEAR:GL_NEAREST;
			switch(mipfilter)
			{
			case 0: minf = magf; break;
			case D3DTEXF_LINEAR: minf = GL_NEAREST_MIPMAP_LINEAR+linear; break;
			case D3DTEXF_POINT: minf = GL_NEAREST_MIPMAP_NEAREST+linear; break;
			}

			auto sam = d->queryGL->samplers;
		
			//NOTE: this is reset to GL_NEAREST below since there isn't
			//a "bltstate" for it and using 1 sampler seems complicated
			if(DDRAW::do2ndSceneBuffer)
			{
				glSamplerParameteri(sam[1],GL_TEXTURE_MIN_FILTER,minf);
				glSamplerParameteri(sam[1],GL_TEXTURE_MAG_FILTER,magf);
			}		
			glSamplerParameteri(sam[0],GL_TEXTURE_MIN_FILTER,minf);
			glSamplerParameteri(sam[0],GL_TEXTURE_MAG_FILTER,magf);
		}

		if(X) //OpenGL must already be centered
		{
			//c = -0.5f;
		}
		else //Direct3D?
		{
			c = -0.5f; //black magic???
		}
	}
	
	//I guess OpenGL is upside down, it's trivial
	//to correct now since it requires an effects
	//pass
	auto c_dst_top = c+dst.top;
	auto c_dst_bot = c+dst.bottom;
//	if(fx&&X) std::swap(c_dst_top,c_dst_bot);

	FLOAT blt[] = //passing in screenspace coords (tex2) as courtesy to effects shaders
	{
		c+dst.left,  c_dst_top, 0.0f,1.0f,*(float*)&white,l/width,t/height,0,0, 
		c+dst.right, c_dst_top, 0.0f,1.0f,*(float*)&white,r/width,t/height,0,0, 
		c+dst.right, c_dst_bot, 0.0f,1.0f,*(float*)&white,r/width,b/height,0,0, 
		c+dst.left,  c_dst_bot, 0.0f,1.0f,*(float*)&white,l/width,b/height,0,0, 
	};
		
	//2018: !fx doesn't work here
	//2020: restoring !fx by factoring in viewport X/Y into flip() 
	if(!DDRAW::doNothing&&!fx)
	{
		if(vs!=DDRAW::fx)
		{
			if(DDRAW::doScaling&&DDRAW::xyScaling[0]!=1) 
			{
				for(int i=0;i<4*9;i+=9) blt[i]*=DDRAW::xyScaling[0]; 	
			}		
			if(DDRAW::doScaling&&DDRAW::xyScaling[1]!=1) 
			{
				for(int i=0;i<4*9;i+=9) blt[i+1]*=DDRAW::xyScaling[1];	
			}
		}
		else assert(vs!=DDRAW::fx); //!fx should preclude this?

		/*REMOVE ME
		if(D3DFVF_XYZRHW==(fvf&D3DFVF_POSITION_MASK)) //ALWAYS FALSE
		{
			if(DDRAW::doMapping&&DDRAW::xyMapping[0]!=0) 
			{
				for(int i=0;i<4*9;i+=9) blt[i]+=DDRAW::xyMapping[0]; 	
			}
			if(DDRAW::doMapping&&DDRAW::xyMapping[1]!=0) 
			{
				for(int i=0;i<4*9;i+=9) blt[i+1]+=DDRAW::xyMapping[1];
			}
		}*/

		if(DDRAW::doSuperSampling)
		{
			assert(c==0);
			for(int i=0;i<4*9;i+=9)
			DDRAW::doSuperSamplingMul(blt[i],blt[i+1]);
		}
	}
	//if(D3DFVF_XYZRHW!=(fvf&D3DFVF_POSITION_MASK)) //ALWAYS TRUE
	{
		//D3DFVF_XYZW: convert to clipping space

		int vp[4]; glGetIntegerv(GL_VIEWPORT,vp);

		for(int i=0;i<4*9;i+=9) //clip space
		{
			blt[i+7] = blt[i]; blt[i+8] = blt[i+1]; //tex2

			blt[i] = blt[i]/vp[2]*2-1;

			blt[i+1] = 1-blt[i+1]/vp[3]*2;
		}			
	}	

	dx_d3d9X_flushaders(vs,ps);
	dx_d3d9c_level(dx_d3d9c_pshaders_noclip[ps]); //???

	HRESULT out = D3D_OK; 

	//HACK: just need to disable stereo mode for FX
	bool stereo; 
	if(fx) std::swap(stereo=false,DDRAW::inStereo);	
	if(X) //out = d->DrawPrimitive(DX::D3DPT_TRIANGLEFAN,fvf,blt,4,0);
	{
		auto d = (D3D9X::IDirect3DDevice7<true>*)DDRAW::Direct3DDevice7;
		out = d->drawprims2(DX::D3DPT_TRIANGLEFAN,fvf,blt,4);
	}
	else 
	{
		auto d = (D3D9X::IDirect3DDevice7<false>*)DDRAW::Direct3DDevice7;
		out = d->drawprims2(DX::D3DPT_TRIANGLEFAN,fvf,blt,4);
	}	
	if(fx) DDRAW::inStereo = stereo;

	if(fx&&X&&DDRAW::do2ndSceneBuffer) //HACK
	{
		auto sam = d->queryGL->samplers;

		//REMINDER: dx_d3d9X_cleanupblt can't easily account for this
		glSamplerParameteri(sam[1],GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_NEAREST);
		glSamplerParameteri(sam[1],GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	}

	return out;
}
HRESULT D3D9X::IDirectDrawSurface7::Blt(LPRECT x, DX::LPDIRECTDRAWSURFACE7 y, LPRECT z, DWORD w, DX::LPDDBLTFX q)
{
	auto p = DDRAW::is_proxy<IDirectDrawSurface7>(y);
	if(!p||!DDRAW::Direct3DDevice7)
	return D3D9C::IDirectDrawSurface7::Blt(x,y,z,w,q);

	DDRAW_LEVEL(7) << "D3D9X::IDirectDrawSurface7::Blt()\n";

	//DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	DDRAW_PUSH_HACK(DIRECTDRAWSURFACE7_BLT,IDirectDrawSurface7*,
	LPRECT&,DX::LPDIRECTDRAWSURFACE7&,LPRECT&,DWORD&,DX::LPDDBLTFX&)(0,this,x,y,z,w,q);

	if(x&&IsRectEmpty(x)) return DD_OK; //hack: allow short circuiting

	DDRAW_LEVEL(7) << "source is " << (UINT)y << "(" << (UINT)z << ")\n";

	if(DDRAWLOG::debug>=7)
	{
	#define OUT(X) if(w&X) DDRAW_LEVEL(7) << ' ' << #X << '\n';

		OUT(DDBLT_ALPHADEST)
		OUT(DDBLT_ALPHADESTCONSTOVERRIDE)
		OUT(DDBLT_ALPHADESTNEG)
		OUT(DDBLT_ALPHADESTSURFACEOVERRIDE)
		OUT(DDBLT_ALPHAEDGEBLEND)
		OUT(DDBLT_ALPHASRC)
		OUT(DDBLT_ALPHASRCCONSTOVERRIDE)
		OUT(DDBLT_ALPHASRCNEG)
		OUT(DDBLT_ALPHASRCSURFACEOVERRIDE)
		OUT(DDBLT_ASYNC)
		OUT(DDBLT_COLORFILL)
		OUT(DDBLT_DDFX)
		OUT(DDBLT_DDROPS)
		OUT(DDBLT_KEYDEST)
		OUT(DDBLT_KEYDESTOVERRIDE)
		OUT(DDBLT_KEYSRC)
		OUT(DDBLT_KEYSRCOVERRIDE)
		OUT(DDBLT_ROP)
		OUT(DDBLT_ROTATIONANGLE)
		OUT(DDBLT_ZBUFFER)
		OUT(DDBLT_ZBUFFERDESTCONSTOVERRIDE)
		OUT(DDBLT_ZBUFFERDESTOVERRIDE)
		OUT(DDBLT_ZBUFFERSRCCONSTOVERRIDE)
		OUT(DDBLT_ZBUFFERSRCOVERRIDE)
		OUT(DDBLT_WAIT)
		OUT(DDBLT_DEPTHFILL)
		OUT(DDBLT_DONOTWAIT)    

	#undef OUT
	
		DWORD *dwFx = 0;

	#define DWORD_(x) { DDRAW_LEVEL(7) << #x " is " << *(dwFx=&q->x) << "\n"; }

	#define DWORD_AND(x) if(*dwFx&x){ DDRAW_LEVEL(3) << #x "\n"; }

		if(q)
		{
		DDRAW_LEVEL(7) << "PANIC! LPDDBLTFX was passed...\n";

		#include "log.ddbltfx.inl"
		}

	#undef DWORD_
	#undef DWORD_AND
	}
	
	if(p->query9->isPrimary) 
	DDRAW_LEVEL(2) << " Source is primary surface\n"; //???

	RECT dest,src; if(!query9->isPrimary)
	{	
		const bool X = DDRAW::gl;

		if(!x)
		{
			dest.left = dest.top = 0;
			dest.right = queryX->width; dest.bottom = queryX->height;
			x = &dest;
		}
		if(!z) 
		{
			src.left = src.top = 0;
			src.right = p->queryX->width; src.bottom = p->queryX->height;
			z = &src;
		}
		
		//DDRAW_PUSH_HACK extension
		int abe = w&DDBLT_ALPHASRCCONSTOVERRIDE?1:0;

		//assuming blitting to back buffer
		if(query9->group9!=dx_d3d9c_virtual_group)
		{
			out = DDRAW_UNIMPLEMENTED;
		}
		else if(dx_d3d9X_prepareblt(X,abe)) //colorkey
		{
			D3D9X::updating_texture(p,false);
			auto texture = p->queryX->update;

			/*dx_d3d9c_texture_pool==D3DPOOL_DEFAULT???
			if(!texture)
			out = p->proxy9->GetContainer(__uuidof(IDirect3DTexture9),(void**)&texture);										
			else out = D3D_OK;
			if(out==D3D_OK)*/
			{
				/*discontinuing this in favor of TEXTURE0_NOCLIP
				if(DDRAW::psColorkey)
				dx_d3d9X_psconstant(DDRAW::psColorkey,p->query9->knockouts?1ul:0ul,'w'); //colorkey
				*/
							
				if(abe) //DDRAW_PUSH_HACK extension
				{
					assert(8==q->dwAlphaDestConstBitDepth);							
					abe = q->dwAlphaSrcConst<<24|0xFFFFFF;
				}
				else abe = 0xFFFFFFFF;
				out = dx_d3d9X_backblt(X,texture,*z,*x,DDRAW::ps,DDRAW::vs,abe); //???

				//dx_d3d9c_texture_pool==D3DPOOL_DEFAULT???
				//if(!p->query9->update9) p->query9->update9->Release();

				if(!dx_d3d9X_cleanupblt(X)) assert(0); //warning: potentially disastrous
			}
		}
		else out = !D3D_OK; //2021
	}
	else out = D3D9X::IDirectDrawSurface7::flip(); //a subroutine of Flip

	if(out!=D3D_OK) DDRAW_LEVEL(2) << "Blt FAILED\n";	

	DDRAW_POP_HACK(DIRECTDRAWSURFACE7_BLT,IDirectDrawSurface7*,
	LPRECT&,DX::LPDIRECTDRAWSURFACE7&,LPRECT&,DWORD&,DX::LPDDBLTFX&)(&out,this,x,y,z,w,q);
	
	DDRAW_RETURN(out)
}
static int dx_d3d9X_xr_discard = 0;
template<int X>
HRESULT D3D9X::IDirectDrawSurface7::flip()
{
	HRESULT out = !DD_OK;

	const auto d = DDRAW::Direct3DDevice7;

	//detecting fx9_effects_buffers_require_reevaluation??
	auto cmp = d->query9->effects[0];
		
	//WARNING: what if fx9_effects_buffers_require_reevaluation
	//is called inside?
	DDRAW::onEffects();

	bool xdiscarded = false; //HACK: draw to regular window?

	//REMINDER: som.hacks.cpp has DDRAW::onEffects enable OpenXR
	bool XR = DDRAW::xr; if(XR)
	{
		if(dx_d3d9X_xr_discard)
		{
			dx_d3d9X_xr_discard--;
		}
		else if(0&&DX::debug||d->queryGL->xr->xdiscarded())
		{
			if(!dx_d3d9X_xr_discard)
			{
				xdiscarded = true;

				XR = false; //!!
			}
		}
	}

	//WARNING: THIS IS GLuint IF X==true (OpenGL)
	auto* &pp = d->query9->effects[0], *p = pp;

	if(DDRAW::isPaused) dx_d3d9X_xpaused = XR!=0; //xcenter

	if(DDRAW::isPaused&&!DDRAW::onPause()||p!=cmp) //2021
	{
		//hack: when unpausing in menus sometimes the lights go awry
		DDRAW::noFlips++;

		out = D3D_OK; goto xdiscarded; //DDRAW_RETURN(D3D_OK);
	}
	
	auto d3dd9 = d->proxy9;			

	bool d3d11 = XR||X&&DDRAW::WGL_NV_DX_interop2;
	
	if(!p||!dx_d3d9c_effectsshader_enable)
	{
		out = DDRAW::onFlip()?dx_d3d9X_present():S_OK;
	}
	else 
	{
		//2018: THIS IS VERY CONFUSING, BUT WHEN THE VIEWPORT IS SMALLER 
		//THAN THE RENDER-TARGET THE vs/psPresentState NEED TO BE EITHER
		//IN VIEWPORT OR RENDER-TARGET SPACE RESPECTIVELY
		D3DVIEWPORT9 vp; dx_d3d9X_viewport(X,vp,d);

		/*REMOVE ME
		float r = 0, t = 0; if(X)
		{			
			glBindTexture(GL_TEXTURE_2D,(GLuint)p);
			glGetTexLevelParameterfv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&r);
			glGetTexLevelParameterfv(GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT,&t);
		}
		else
		{
			D3DSURFACE_DESC desc; p->GetLevelDesc(0,&desc); 
			r = desc.Width; t = desc.Height;
		}*/
		float r = d->queryX->effects_w;
		float t = d->queryX->effects_h;

		//VIEWPORT space (do_aa/DAA)
		//REMINDER: THE FIRST FRAME WILL BE GARBAGE
		//(dejagrate_update_vsPresentState is provided in case it needs to
		//be cleaned up)
		if(DDRAW::dejagrate>1) //3D "anti-aliasing"		
		dx_d3d9c_update_vsPresentState(vp);
		DDRAW::dejagrate_update_psPresentState();
  
		IDirect3DSurface9 *rt = 0;
		//nice: do swap in logical order for fx2ndSceneBuffer 
		GLuint rtX = d->query9->effects[1]?1:0;
		if(X) rtX = d->queryGL->effects[rtX];
		else d->query9->effects[rtX]->GetSurfaceLevel(0,&rt);
		
		auto *bbuffer = X?0:dx_d3d9X_backbuffer9();
		if(X||bbuffer)
		{	
			const int w = DDRAW::PrimarySurface7->queryX->width;
			const int h = DDRAW::PrimarySurface7->queryX->height;
			assert(w&&h);
			float ww = (float)w, hh = (float)h;
			DDRAW::doSuperSamplingMul(ww,hh);
			
			//NOTE: glBlitFramebuffer can't switch to the//
			//backbuffer here, so it's done after mipmaps//
			//(it would just have to be redone afterward)//

			bool matte = false;
			if(d3d11||dx_d3d9X_prepareblt(X)) //XR
			{
				for(int i=0;i<(int)DDRAW::doMipSceneBuffers;i++)
				{
					IDirect3DSurface9 *lv0 = 0,*lv1 = 0;
					if(!X) p->GetSurfaceLevel(i,&lv0);
					if(!X) p->GetSurfaceLevel(i+1,&lv1);
					else if(X)
					{
						/*if(1) //TESTING 
						{
							//glBlitFramebuffer isn't working for plain Intel OpenGL

							glBindTexture(GL_TEXTURE_2D,(int)p);
							glGenerateMipmap(GL_TEXTURE_2D);
							break;
						}*/

						//glColorMaski?
						//https://bugs.chromium.org/p/angleproject/issues/detail?id=6285
						GLenum tmp = GL_COLOR_ATTACHMENT0;
						if(!i) glDrawBuffers(1,&tmp);
						else glBindFramebuffer(GL_READ_FRAMEBUFFER,d->queryGL->framebuffers[0]);
						glFramebufferTexture(GL_READ_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,(int)p,i);
						//Note: ANGLE validation doesn't allow self blit currently. Intel's
						//driver doesn't report and error but the result is a black screen?
						glBindFramebuffer(GL_DRAW_FRAMEBUFFER,d->queryGL->framebuffers[1]);
						glFramebufferTexture(GL_DRAW_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,(int)p,i+1);
					}
					int rr = r, tt = t;
					if(i){ rr/=2; tt/=2; }; assert(i<2);
					if(DDRAW::xyMapping2[0]&&i==DDRAW::doMipSceneBuffers-1)
					{
						//FYI: this is a complicated effect to shift the dissolved mipmaps
						//over 1px on the odd frames so that the apparent resolution is 2x
						if(!X)
						{
							RECT src = {1,1,rr-1,tt-1}, dst = {0,0,rr/2-1,tt/2-1};
							d->proxy9->StretchRect(lv0,&src,lv1,&dst,D3DTEXF_LINEAR);
						}
						else glBlitFramebuffer(1,1,rr-1,tt-1,0,0,rr/2-1,tt/2-1,GL_COLOR_BUFFER_BIT,GL_LINEAR);
					}
					else if(X)
					{
						//#error report ANGLE error?
						//TODO: will opengl32.dll read/write to same framebuffer or not?
						glBlitFramebuffer(0,0,rr,tt,0,0,rr/2,tt/2,GL_COLOR_BUFFER_BIT,GL_LINEAR);
					}
					else d->proxy9->StretchRect(lv0,0,lv1,0,D3DTEXF_LINEAR);
					
					if(lv0) lv0->Release(); else assert(X);
					if(lv1) lv1->Release(); else assert(X);
				}

				if(!X) //FINALLY: can switch to backbuffer?
				{							
					//NOTE: SetRenderTarget automatically 
					//resets the viewports
					for(int i=0;i<3;i++)
					if(d->query9->mrtargets[i])
					d->proxy9->SetRenderTarget(1+i,0);
					out = d->proxy9->SetRenderTarget(0,bbuffer);

					//REFERENCE
					//SetRenderTarget is supposed to do this automatically
					//D3DVIEWPORT9 full = {0,0,w,h,0,1};
					//d3dd9->SetViewport(&full);
				}
				else if(XR)
				{
					out = D3D_OK; //I guess?
				}
				else //X 
				{
					glBindFramebuffer(GL_DRAW_FRAMEBUFFER,0);
					//I think the framebuffers remember this
					//GLenum tmp = 0x0405; //GL_BACK
					//glDrawBuffers(1,&tmp);
					out = D3D_OK; //ERROR?

					//NOTE: supersampling requires this here
					//I guess viewport is independent of the
					//frame buffer state
					glViewport(0,0,w,h);
				}

				int l = DDRAW::fx2ndSceneBuffer;
				
				//DDRAW::do2ndSceneBuffer?
				if(d->query9->effects[1])
				{
					//REMINDER: som_status_ddraw_is_pausing() is
					//entered differently for OpenGL so that the
					//fx2ndSceneBuffer=0 comes too late
					IDirect3DTexture9 *p2 =
					d->query9->effects[DDRAW::fx2ndSceneBuffer];
					if(X)
					{
						glActiveTexture(GL_TEXTURE0+1);
						glBindTexture(GL_TEXTURE_2D,(int)p2);
						glActiveTexture(GL_TEXTURE0+0);
					}
					else d->proxy9->SetTexture(1,p2);	

					if(DDRAW::ps2ndSceneMipmapping2) //EXPERIMENTAL
					if(int i=DDRAW::doMipSceneBuffers)
					{
						//I'm pretty sure rcp should be 2 here but 1.5 or 1.75
						//looks better to me... there are hard to account for 
						//blending/contrast artifacts in the antialiased edges
						//float rcp = i==2?-2:-1;
						float rcp = i==2?-1.66f:-1;
						float c[4] = {rcp/r,rcp/t};

						if(DDRAW::xyMapping2[0!=DDRAW::fx2ndSceneBuffer])
						{
							c[2] = c[0]; c[3] = c[1]; 
						}
						if(!DDRAW::xyMapping2[0]) 
						{
							c[0] = c[1] = 0;
						}

						if(XR) //HACK: assuming first register is reserved for this?
						{
							if(float*cc=(float*)d->queryGL->effects_cbuf)
							memcpy(cc,c,sizeof(c));
						}
						else dx_d3d9X_psconstant(DDRAW::ps2ndSceneMipmapping2,c,'xyzw');						
					}
					DDRAW::fx2ndSceneBuffer = 1;
				}
	
				if(DDRAW::psColorize) 					
				dx_d3d9X_psconstant(DDRAW::psColorize,DDRAW::fxColorize,'rgba'); 

				if(XR) //OpenXR?
				{
					auto *q = d->queryGL;

				//	assert(!q->xr->xdiscarded()); //DRAW?

					Widgets95::xr::effects fx = 
					{
					   {q->effects[0],q->effects[1],0,0}, //dither?
						q->effects_hlsl,
						q->effects_cbuf,q->effects_cbuf_size
					};
					
					auto *xs = q->xr->xfirst();
					auto &sv = q->stereo_views;
					for(int i=0;i<_countof(sv);i++,xs=xs->next())
					{
						auto &v = sv[i];
						auto *s = (const Widgets95::xr::section*)v.src;
						if(!s||s!=xs->find_sections())
						{
							assert(!s); break;
						}

						int x = s->virtual_off[0];
						int y = s->virtual_off[1];
						int w = s->virtual_dim[0];
						int h = s->virtual_dim[1];

						/*REFERENCE
						//this strategy turned out to be nonstarter since
						//D3D11_RESOURCE_MISC_RESTRICT_SHARED_RESOURCE is
						//set in WMR/OpenXR preventing WGL_NV_DX_interop2
						if(0)
						{
							//NOTE: assuming source image is upside-down
							RECT src,dst = {x,y,x+w,y+h};
							src.left = v.x;
							src.right = src.left+v.w;
							src.bottom = v.y;
							src.top = src.bottom+v.h;						


							int lock = xs->lock_framebuf();
							glBindFramebuffer(GL_DRAW_FRAMEBUFFER,lock);
							glViewport(x,y,w,h);						
							dx_d3d9X_backblt(X,p,src,dst,DDRAW::fx,DDRAW::fx);
							xs->unlock_framebuf(lock);
						}
						else*/
						{
							//NOTE: assuming source image is upside-down
							//TODO: DDRAW::doSmooth needs to adjust this
							float src[4] = {v.x,v.y+v.h,v.w,-v.h};
							float dst[4] = {x,y,w,h};
														
							assert(!DDRAW::doSuperSampling);

							if(DDRAW::doSmooth) src[0]+=0.5f;
							if(DDRAW::doSmooth) src[1]+=0.5f;

							//D3D9 likes this... this is going to
							//D3D11 (see dx_d3d9X_backblt) 
						//	dst[0]-=0.5f; dst[1]-=0.5f;

							xs->blit_framebuf(&fx,dst,src);

							fx.constant_buffer = nullptr; //double-copy?
						}
					}
				}
				else
				{
					#if 0 //old way
					RECT src = {0,0,r,t}; RECT dst = {0,0,w,h}; 
					#else //conservative when viewport is smaller, but doesn't scale up/down
					RECT src = {0,0,vp.Width,vp.Height}; RECT dst = {0,0,vp.Width,vp.Height}; 
					DDRAW::doSuperSamplingDiv(dst.right,dst.bottom);
					#endif
					if(r!=ww||t!=hh)
					{
						assert(r==ww&&t==hh||xdiscarded); 

						src.right = r; src.bottom = t; dst.right = w; dst.bottom = h;
					}

					int l2 = 1;

					if(d3d11) //2022
					{
						//HACK: this lets Ex.output.cpp draw in
						//the margins
						src.right = r; src.bottom = t;
						dst.right = w; dst.bottom = h;
					}
					else
					{
						if(DDRAW::doSuperSampling)
						{					
							int x = vp.X, y = vp.Y; 
							OffsetRect(&src,x,y);
							DDRAW::doSuperSamplingDiv(x,y);
							OffsetRect(&dst,x,y);					
						}
						else
						{
							OffsetRect(&dst,vp.X,vp.Y); //2020
							OffsetRect(&src,vp.X,vp.Y); //2020
						}

					//	int l2 = 1;
						int ir = DDRAW::fxInflateRect;
						int ir2 = ir;
						if(1&&DDRAW::doSuperSampling)
						{
							l2*=2; ir2*=2;
						}
						InflateRect(&src,ir2,ir2),
						InflateRect(&dst,ir,ir);
					}
						
					//OpenGL's origin is at the bottom
					//
					// NOTE: this is more important for tools that use a 
					// variable sized viewport and a fixed back buffer a
					// la ItemEdit and friends
					//
					if(X) std::swap(dst.top=h-dst.top,dst.bottom=h-dst.bottom);

					if(d3d11) //WGL_NV_DX_interop2
					{
						auto *q = d->queryGL;

						Widgets95::xr::effects fx = 
						{
						   {q->effects[0],q->effects[1],0,0}, //dither?
							q->effects_hlsl,
							q->effects_cbuf,q->effects_cbuf_size
						};
										
						//int x = dst.left;
						//int y = dst.bottom;
						//float fsrc[4],fdst[4] = {x,h-y,w,h};

						float fsrc[4],fdst[4];

						fdst[0] = dst.left; 
						fdst[1] = h-dst.bottom;
						fdst[2] = dst.right-dst.left;
						fdst[3] = dst.bottom-dst.top;

						fsrc[0] = src.left; 
						fsrc[1] = src.bottom;
						fsrc[2] = src.right-src.left;
						fsrc[3] = src.top-src.bottom;

						if(DDRAW::doSmooth)
						{
							float s = 0.5f;
							DDRAW::doSuperSamplingMul(s);
							fsrc[0]+=s;
							fsrc[1]+=s;
						}

						//D3D9 likes this... this is going to
						//D3D11 (see dx_d3d9X_backblt) 
					//	fdst[0]-=0.5f; fdst[1]-=0.5f;

						auto *xs = q->xr->first_surface();

						xs->blit_framebuf(&fx,fdst,fsrc);						
					}
					else if(DDRAW::inStereo&&!xdiscarded)
					{
						matte = true/*&&DX::debug*/;
						//+2 resolve ambiguity on the edge in the vertex shader
						//+1???
						int sx = (src.right-src.left)/2+l2; //1
						int dx = (dst.right-dst.left)/2+1; 
						src.right-=sx; dst.right-=dx;
						/*DISABLING after changes to PSVR_PROJ_1
						//etc.
						if(1) //circles are oval???
						{
							//NOTE: MIGHT BE INCREASING "CHROMATIC
							//ABERRATION"
							#ifdef NDEBUG
							#error should FOV be asymmetrical?
							#endif
							//THIS IS A NECESSARY CORRECTION...
							//should the barrel shader be changed?							
							int dy2 = ((dst.bottom-dst.top)-dx)/2;
							//After changing som.shader.cpp the oval
							//is wide instead of tall, and no longer
							//matches the square dimensions? THIS IS
							//APPROACHING 0! 
							//dy2 = dy2*0.75f+0.5f; 
							dst.top+=dy2; dst.bottom-=dy2;
						}*/
						dx_d3d9X_backblt(X,p,src,dst,DDRAW::fx,DDRAW::fx);
						src.right+=sx; src.left+=sx;
						dst.right+=dx; dst.left+=dx;
						dx_d3d9X_backblt(X,p,src,dst,DDRAW::fx,DDRAW::fx);
					}
					else dx_d3d9X_backblt(X,p,src,dst,DDRAW::fx,DDRAW::fx);
					
					if(!d3d11) dx_d3d9X_cleanupblt(X);
				}

				if(d->query9->effects[1])
				{
					if(DDRAW::ps2ndSceneMipmapping2) //EXPERIMENTAL
					std::swap(DDRAW::xyMapping2[0],DDRAW::xyMapping2[1]);
					std::swap(pp,d->query9->effects[1]);

					if(X) //TODO? retrieve/restore texture #1?
					{
						glActiveTexture(GL_TEXTURE0+1);
						glBindTexture(GL_TEXTURE_2D,d->queryGL->black);
						glActiveTexture(GL_TEXTURE0+0);
					}
					else 
					{
						//2021: CreateDevice sets DDRAW_TEX0 to black instead of 0
						d->proxy9->SetTexture(1,d->query9->black);
					}
				}						
			}						

			bool ss = false;
			DWORD sw = vp.Width, sh = vp.Height;
			{
				if(DDRAW::doSuperSampling)
				{
					DDRAW::doSuperSamplingDiv(vp.Width,vp.Height);
					std::swap(ss,DDRAW::doSuperSampling);
				}
				//NOTE: I guess this is for onFlip since I don't
				//think SwapBuffers or Present consult this
				if(X)
				{
					//this is incorrect in ItemEdit but the one
					//set up earlier is correct. I think maybe
					//the D3D one is undoing SetRenderTarget
					//since it implicitly sets the viewport
					glViewport(vp.X,vp.Y,vp.Width,vp.Height);
				}
				else d->proxy9->SetViewport(&vp); 
			}
			vp.Width = sw; vp.Height = sh;

			if(DDRAW::onFlip()) 
			{
				if(out==D3D_OK)
				{
					if(xdiscarded)
					{
						DDRAW::inStereo = false;
						{
							out = dx_d3d9X_present();
						}
						DDRAW::inStereo = true;
					}
					else out = dx_d3d9X_present();
				}
			}

			//WARNING! JUST REALIZED THIS WAS GETTING IN THE 
			//WAY OF AN ASYNC PRESENT EXPERIMENT THAT SHOULD
			//MAYBE BE REVISITED
			if(matte) if(X)
			{
				glClearColor(0,0,0,0);
				glClear(GL_COLOR_BUFFER_BIT);
			}
			else d->proxy9->Clear(0,0,D3DCLEAR_TARGET,0,0,0);

			std::swap(ss,DDRAW::doSuperSampling);
						 
			//REMINDER: SetRenderTarget sets the viewport 
			if(X) 
			{			
				glBindFramebuffer(GL_FRAMEBUFFER,*d->queryGL->framebuffers);
				glFramebufferTexture(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,rtX,0);
			}
			else if(rt&&d->proxy9->SetRenderTarget(0,rt))
			{
				assert(0); //dx_d3d9c_discardeffects(); 
			}

			//unset SetRenderTarget
			if(X) glViewport(vp.X,vp.Y,vp.Width,vp.Height);
			else d->proxy9->SetViewport(&vp);

			if(!X) bbuffer->Release();

			if(X) //repair MRT targets
			{
				auto &st = d->queryGL->state, cmp = st;
				assert(cmp.colorwriteenable&8);
				cmp.colorwriteenable = 8;
				st.apply(cmp);
			}
			else //repair MRT targets
			{
				for(DWORD i=0,cwe;i<3;i++)
				if(d->query9->mrtargets[i])
				if(!d->proxy9->GetRenderState((D3DRENDERSTATETYPE)(D3DRS_COLORWRITEENABLE1+i),&cwe)&&cwe)
				d->SetRenderState((DX::D3DRENDERSTATETYPE)(D3DRS_COLORWRITEENABLE1+i),cwe);				
			}
		}
		else{ out = DDRAW_UNIMPLEMENTED; assert(0); }
		
		if(rt) rt->Release(); //effects
	}	

	if(dx_d3d9c_effectsshader_toggle!=dx_d3d9c_effectsshader_enable)
	{
		dx_d3d9X_enableeffectsshader(dx_d3d9c_effectsshader_toggle);
	}
	
	xdiscarded: if(xdiscarded)
	{		
		//xcenter works better if this is kept running
		//the screen goes black either way
		//if(!DDRAW::isPaused)
		d->queryGL->xr->xwait();
	}

	DDRAW_RETURN(out)
}
HRESULT D3D9X::IDirectDrawSurface7::flip() //C2908
{	
	//2021: not really okay but WM_PAINT is likely causing
	//a cascade of assert message box windows :(
	if(DDRAW::midFlip) 
	{
		return D3D_OK; //breakpoint
	}
	DDRAW::midFlip = true;

	HRESULT ret = target=='dx9c'?flip<false>():flip<true>(); 

	DDRAW::midFlip = false; return ret;
}
HRESULT D3D9X::IDirectDrawSurface7::Flip(DX::LPDIRECTDRAWSURFACE7 x, DWORD y)
{
	DDRAW_LEVEL(7) << "D3D9X::IDirectDrawSurface7::Flip() " << DDRAW::noFlips << '\n';

	//DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)

	DDRAW_PUSH_HACK(DIRECTDRAWSURFACE7_FLIP,IDirectDrawSurface7*,
	DX::LPDIRECTDRAWSURFACE7&,DWORD&)(0,this,x,y);

	assert(!x&&y<=1); //2021: DDFLIP_WAIT

	out = D3D9X::IDirectDrawSurface7::flip(); 

	DDRAW_POP_HACK(DIRECTDRAWSURFACE7_FLIP,IDirectDrawSurface7*,
	DX::LPDIRECTDRAWSURFACE7&,DWORD&)(&out,this,x,y);
		
	//DDRAW_RETURN(out) //annoying error firing on close
	//return out;
	DDRAW_RETURN(out) //2021: still firing?
}
HRESULT D3D9X::IDirectDrawSurface7::GetAttachedSurface(DX::LPDDSCAPS2 x, DX::LPDIRECTDRAWSURFACE7 *y)
{
	if(query9->group9==dx_d3d9c_virtual_group) 
	{
		return D3D9C::IDirectDrawSurface7::GetAttachedSurface(x,y);
	}

	DDRAW_LEVEL(7) << "D3D9X::IDirectDrawSurface7::GetAttachedSurface()\n";
	
	//DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	if(!x||!y) DDRAW_RETURN(!DD_OK)

	if(DDRAWLOG::debug>=8) //#ifdef _DEBUG
	{
#ifdef _DEBUG
#define OUT(X) if(x->dwCaps&DDSCAPS_##X) DDRAW_LEVEL(8) << ' ' << #X << '\n';
						   
	OUT(PRIMARYSURFACE)
	OUT(FRONTBUFFER)
	OUT(BACKBUFFER)
	OUT(ZBUFFER)
	OUT(ALPHA)
	OUT(FLIP)
	OUT(OFFSCREENPLAIN)
	OUT(PALETTE)
	OUT(SYSTEMMEMORY)
	OUT(TEXTURE)
	OUT(3DDEVICE)
	OUT(VIDEOMEMORY)
	OUT(VISIBLE)
	OUT(WRITEONLY)
	OUT(OWNDC)
	OUT(LIVEVIDEO)
	OUT(HWCODEC)
	OUT(MIPMAP)
	OUT(ALLOCONLOAD)
	OUT(VIDEOPORT)
	OUT(LOCALVIDMEM)
	OUT(NONLOCALVIDMEM)
	OUT(STANDARDVGAMODE)
	OUT(OPTIMIZED)

#undef OUT
#endif
	}

	HRESULT out = DD_OK;
	if(~x->dwCaps&DDSCAPS_TEXTURE) out = DDRAW_UNIMPLEMENTED; //???
	else if(~x->dwCaps&DDSCAPS_MIPMAP) out = !DD_OK;
	else if(!queryX->group||queryX->mipmaps<=1) out = !DD_OK;
	
	if(out||DDRAW::doMipmaps) return DDERR_NOMIPMAPHW;

	IDirect3DSurface9 *s = 0;

	int width = 0, height = 0;

	int lv = queryX->isMipmap+1;

	if(!(out=query9->group9->GetSurfaceLevel(lv,&s)))
	{
		D3DSURFACE_DESC desc; //2021
		s->GetDesc(&desc);
		width = width; height = height;
	}
	if(!out)
	{
		auto p = new DDRAW::IDirectDrawSurface7(DDRAW::target);
						
		p->queryX->isMipmap = lv; 

		p->queryX->group = queryX->group;

		queryX->group->AddRef(); //copying dx.d3d9c.cpp
		
		p->queryX->pitch = dx_d3d9c_pitch(s);

		p->proxy9 = s; 

		p->queryX->nativeformat = queryX->nativeformat; //alpha?

		p->queryX->format = queryX->format; //2021

		p->queryX->colorkey2 = query9->colorkey2;

		p->queryX->colorkey = queryX->colorkey; //shares memory with mipmaps

		p->queryX->width = width; p->queryX->height = height;
			
		*y = p; return DD_OK;
	}

	DDRAW_RETURN(out)
}
HRESULT D3D9X::IDirectDrawSurface7::GetSurfaceDesc(DX::LPDDSURFACEDESC2 x)
{
	if(query9->group9==dx_d3d9c_virtual_group)
	{
		return D3D9C::IDirectDrawSurface7::GetSurfaceDesc(x);
	}

	DDRAW_LEVEL(7) << "D3D9X::IDirectDrawSurface7::GetSurfaceDesc()\n";

	//DDRAW_IF_NOT_TARGET_RETURN(!DD_OK)
	
	if(!x||x->dwSize!=sizeof(DX::DDSURFACEDESC2)) DDRAW_RETURN(!DD_OK)
		
	x->dwFlags = DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT;

	x->ddsCaps.dwCaps = 0x00000000;

	if(query9->group9) //mipmapped texture
	{
		if(!query9->isMipmap)
		{
			x->dwMipMapCount = query9->group9->GetLevelCount();
			x->dwFlags|=DDSD_MIPMAPCOUNT;
		}
		x->ddsCaps.dwCaps|=DDSCAPS_COMPLEX; //???
		x->ddsCaps.dwCaps|=DDSCAPS_MIPMAP; //ok for first level??		

		/*NECESSARY?
		//how/why is this communicated?? 
		x->dwFlags|=DDSD_TEXTURESTAGE;
		x->dwTextureStage = 0;*/

		x->ddsCaps.dwCaps|=DDSCAPS_TEXTURE|DDSCAPS_VIDEOMEMORY|DDSCAPS_WRITEONLY;
	}
	else x->ddsCaps.dwCaps|=DDSCAPS_OFFSCREENPLAIN|DDSCAPS_SYSTEMMEMORY;

	x->dwHeight = queryX->height;
	x->dwWidth = queryX->width;
	x->ddpfPixelFormat = dx_d3d9c_format(queryX->format); //or nativeformat?	
	if(x->lPitch=query9->pitch) x->dwFlags|=DDSD_PITCH; //!

	return DD_OK;
}

/*EXPERIMENTAL
this all works except D3D9 isn't accepting the HANDLE handles 
the DXGI startup code may be useful for D3D10 or OpenXR later
static void dx_d3d9X_d3d12(DDRAW::IDirect3D7 *p, UINT w, UINT h, D3DFORMAT fxbpp)
{
	//I had trouble with "D3D9On12" so I'm trying here to make a shared
	//surface out of the D3D12 backbuffer(s) to replace the D3D9 buffer
	//https://github.com/microsoft/D3D9On12/issues/2
	ID3D12Device *d12 = 0;
	auto &h12 = DDRAW::doD3D12_backbuffers;
	memset(h12,0x00,sizeof(h12));
	if(DDRAW::doD3D12||1&&DX::debug) //TESTING
	if(DDRAW::doFlipEx)
	if(!D3D9C::DLL::Direct3DCreate9On12Ex) //DDRAW::doD3D9on12?
	{
		static void *dxgi = GetProcAddress(LoadLibraryA("dxgi.dll"),"CreateDXGIFactory1");
		static void *cd12 = GetProcAddress(LoadLibraryA("d3d12.dll"),"D3D12CreateDevice");
		
		auto CreateDXGIFactory1 = (HRESULT(WINAPI*)(REFIID,void**))dxgi;
		auto D3D12CreateDevice = (HRESULT(WINAPI*)(IUnknown*,D3D_FEATURE_LEVEL,REFIID,void**))cd12;

		//IDXGIFactory4 *pFactory; //not defined... assuming 1 works too
		IDXGIFactory2 *pFactory; 
		if(cd12&&dxgi)
		if(!CreateDXGIFactory1(IID_PPV_ARGS(&pFactory)))
		{
			LUID luid;
			if(!p->proxy9->GetAdapterLUID(p->query9->adapter,&luid))
			{
				IDXGIAdapter1 *pAdapter;
				int i = 0;
				while(DXGI_ERROR_NOT_FOUND!=pFactory->EnumAdapters1(i++,&pAdapter))
				{
					DXGI_ADAPTER_DESC desc;
					if(!pAdapter->GetDesc(&desc)&&!memcmp(&luid,&desc.AdapterLuid,sizeof(LUID)))
					{
						//NOTE: D3D_FEATURE_LEVEL_11_1 is what my older PC supports for testing
						//might need to increase to 12_1 to be meaningful
						HRESULT test = 
						D3D12CreateDevice(pAdapter,D3D_FEATURE_LEVEL_11_1,IID_PPV_ARGS(&d12));						
						break;					
					}
					pAdapter->Release();
				}
			}
			
			if(d12)
			{
				//here it's said d3d12 requires CreateSwapChainForHwnd
				//https://gamedev.stackexchange.com/questions/149822/direct3d-12-cant-create-a-swap-chain				
				IDXGISwapChain1 *scI = 0;
				DXGI_SWAP_CHAIN_DESC1 desc = {};
				desc.Width = w;
				desc.Height = h;
				desc.Format = fxbpp==D3DFMT_X8R8G8B8?
				DXGI_FORMAT_R8G8B8A8_UNORM:DXGI_FORMAT_R10G10B10A2_UNORM;
				desc.Stereo = FALSE; //true?
				desc.SampleDesc.Count = 1; //no multisampling //0?
				//desc.SampleDesc.Quality = 0;
				desc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
				desc.BufferCount = 2; //3? 
				desc.Scaling = DXGI_SCALING_NONE;
				//I think this is D3DSWAPEFFECT_FLIPEX
				//https://devblogs.microsoft.com/directx/dxgi-flip-model/
				desc.SwapEffect = (DXGI_SWAP_EFFECT)4; //DXGI_SWAP_EFFECT_FLIP_DISCARD;
				desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
				//desc.Flags = 0;
				D3D12_COMMAND_QUEUE_DESC cqdesc = {};
				cqdesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
				cqdesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
				ID3D12CommandQueue *cqI = 0;
				if(!d12->CreateCommandQueue(&cqdesc,IID_PPV_ARGS(&cqI)))
				{
					pFactory->CreateSwapChainForHwnd(cqI,DDRAW::window,&desc,0,0,&scI);

					//NOTE: I think might want to keep this around. ExecuteCommandLists?
					cqI->Release();
				}

				if(scI)
				{
					ID3D12Resource *bb;

					//NOTE: docs call these "back buffers" (despite its name)
					//FUNNY: it enumerates 2 (front buffer?))
					int i = 0;				
					for(int i=0;!scI->GetBuffer(i,IID_PPV_ARGS(&bb));i++)
					{
						//NOTE: only GENERIC_ALL works for D3D12 (GENERIC_WRITE don't)
						if(!d12->CreateSharedHandle(bb,0,GENERIC_ALL,0,&h12[i].nt_handle))
						{
							h12[i].d3d12 = bb; assert(i<DX_ARRAYSIZEOF(h12)-1);
						}
						bb->Release();
					}

					scI->Release();
				}
			}

			pFactory->Release();
		}
	}
	DDRAW::D3D12Device = d12;
	DDRAW::target_backbuffer = d12?'dx12':DDRAW::target; //Ex.movie.cpp
}*/
extern bool dx_d3d9X_feature_level_11(LUID luid)
{
	auto CreateDXGIFactory1 = (HRESULT(WINAPI*)(REFIID,void**))
	GetProcAddress(LoadLibraryA("dxgi.dll"),"CreateDXGIFactory1");

	//NOTE: UNKOWN is required when specifying adapters
	enum D3D_DRIVER_TYPE{ D3D_DRIVER_TYPE_UNKNOWN = 0 };
	enum D3D_FEATURE_LEVEL{ D3D_FEATURE_LEVEL_11_0 = 0xb000 };

	auto D3D11CreateDevice = 
	(HRESULT(WINAPI*)(IDXGIAdapter*,D3D_DRIVER_TYPE,
	HMODULE,UINT,CONST D3D_FEATURE_LEVEL*,UINT,UINT,IUnknown**,D3D_FEATURE_LEVEL*,IUnknown**))
	GetProcAddress(LoadLibraryA("d3d11.dll"),"D3D11CreateDevice");

	if(CreateDXGIFactory1&&D3D11CreateDevice)
	{				
		IDXGIAdapter1 *i = nullptr;
		IDXGIFactory4 *ii;
		if(!CreateDXGIFactory1(__uuidof(IDXGIFactory4),(void**)&ii))
		{
			ii->EnumAdapterByLuid(luid,__uuidof(IDXGIAdapter1),(void**)&i);
			ii->Release();
		}
		//NOTE: _ is required and a debug message says can't overlap???
		D3D_FEATURE_LEVEL _,featureLevels[] = {D3D_FEATURE_LEVEL_11_0}; //11_1?
		
		//HACK: without d it returns S_FALSE (does that mean true???)
		IUnknown *d = 0;
		int ver = 7; //D3D11_SDK_VERSION //this is just what is in widgets95.dll
//		auto hr = D3D11CreateDevice(i,D3D_DRIVER_TYPE_UNKNOWN,0,2*DX::debug,featureLevels,_countof(featureLevels),ver,0,&_,0);
		auto hr = D3D11CreateDevice(i,D3D_DRIVER_TYPE_UNKNOWN,0,2*DX::debug,featureLevels,_countof(featureLevels),ver,&d,&_,0);
		
		//TODO: SUCCEEDED(h) might be correct, but S_FALSE?

		//the docs says these can be 0 :(
		if(d) d->Release(); 
//		if(c) c->Release();

		i->Release(); if(!hr) return true;
	}
	return false;
}
extern int dx_d3d9X_supported(HMONITOR h) //UNIMPLEMENTED
{
	assert(DDRAW::target!='dx9c');
	/*THIS assert WON'T FIRE LIKE THIS
	#ifdef NDEBUG
	extern int SOMEX_vnumber; //WIP
	assert(SOMEX_vnumber<=0x1020307UL);
	#endif*/
	switch(DDRAW::target)
	{
	case 'dxGL': return 450;
	case 'dx95': return 310; //can ANGLE even do this?
	}
	return 0;
}
static void *dx_d3d9X_dxGL_load(HMODULE dll, const char *fn)
{
	void *f = wglGetProcAddress(fn);
	if(!f) f = GetProcAddress(dll,fn);
	
	assert(f||strstr(fn,"NV")); //HACK
	
	return f;
}
static void dx_d3d9X_dxGL_load()
{
	//NOTE: I had a bad time here because the debugger
	//was showing 0 for about half (interleaved) of the
	//global function pointers even though they weren't 0
	HMODULE dll = GetModuleHandleA("opengl32.dll");
	#undef X
	#define X(f) (void*&)f = dx_d3d9X_dxGL_load(dll,#f);
	DX_D3D9X_X
	X(glPolygonMode)
	X(glShadeModel) //UNUSED
	X(glBindShadingRateImageNV)
	X(glShadingRateImagePaletteNV)
}
static void* &dx_d3d9X_dxGL_arb(int adapter)
{
	static void *arb = 0; //REMOVE ME?
	static int cmp = -1; if(cmp!=adapter)
	{
		//HACK: different adapter can have completely
		//different driver, but assuming adapters are
		//not hotswapping
		arb = 0; cmp = adapter;
	}
	return arb;
}
static HGLRC dx_d3d9X_dxGL_init(int adapter, HDC *hdc)
{
	//2020: I've pulled this out for xr_NOGLE_OpenGL_WIN32
	void* &arb = dx_d3d9X_dxGL_arb(adapter);
	if(arb==(void*)1) arb = 0; //PARANOIA

	HDC dc = GetDC(DDRAW::window);

	//NOTE: OpenGL only has sRGB formats for RGB-8 and
	//32-bit depth buffer isn't allowed in ES or D3D10
	//except for floating-point
	PIXELFORMATDESCRIPTOR pfd = 
	{ 
	sizeof(PIXELFORMATDESCRIPTOR),   // size of this pfd  
	1,                     // version number  
	PFD_DRAW_TO_WINDOW |   // support window  
	PFD_SUPPORT_OPENGL |   // support OpenGL  
	PFD_DOUBLEBUFFER,      // double buffered  
	PFD_TYPE_RGBA,         // RGBA type  
	24,                    // 24-bit color depth  
	};
	/*actually the FBO has its own 
	//depth-buffer
	pfd.cDepthBits = 24; //stencil?*/

	int i = ChoosePixelFormat(dc,&pfd);

	SetPixelFormat(dc,i,&pfd);

	bool once = !arb;

	HGLRC c; if(!arb) //OPTIMIZING?
	{
		c = wglCreateContext(dc);
	
		if(!c) err:
		{
			ReleaseDC(DDRAW::window,dc);
			return 0; 
		}

		wglMakeCurrent(dc,c);

		arb = wglGetProcAddress("wglCreateContextAttribsARB");
		
		wglMakeCurrent(0,0);
		wglDeleteContext(c);

		if(!arb) goto err;
	}
	const int al[] = 
	{
		0x2091,4, //major
		0x2092,3, //minor
		0x2094,DX::debug, //flags
		0x9126,1, //core profile
		0
	};
	c = ((HGLRC(WINAPI*)(HDC,HGLRC,const int*))arb)(dc,0,al);
	if(!c) goto err;

	wglMakeCurrent(dc,c);

	//2020: I've pulled this out for xr_NOGLE_OpenGL_WIN32
	if(once) dx_d3d9X_dxGL_load();
	
	*hdc = dc; return c;
}
extern int DDRAW::stereo_status_OpenXR()
{
	auto d = DDRAW::Direct3DDevice7;

	return DDRAW::xr?d->queryGL->xr->xstatus():0;
}
static void dx_d3d9X_vao_enable_xr(bool);
extern bool DDRAW::stereo_toggle_OpenXR(bool toggle)
{
	auto d = DDRAW::Direct3DDevice7;
	if(!d||!DDRAW::WGL_NV_DX_interop2) 
	return false;

	auto *q = d->queryGL;
	auto *x = q->xr;

	if(q->stereo_fovea)
	{
		glBindShadingRateImageNV(0);
		glDisable(GL_SHADING_RATE_IMAGE_NV);
		glDeleteTextures(1,&q->stereo_fovea);		
		q->stereo_fovea = 0;
		q->stereo_fovea_cmp[0] = 0; //invalidate
	}

	if(!toggle) //supersampling?
	{
		if(!DDRAW::xr) return false;

		int st = x->xstatus();

		x->xinit(DDRAW::xrSuperSampling);

		if(st>=3) goto resize_only;
	}
	else
	{
		if(DDRAW::inStereo)
		{
			if(x->xquit())
			{
				DDRAW::inStereo = DDRAW::xr = false;

				dx_d3d9X_vao_enable_xr(false);

				memset(q->stereo_views,0x00,sizeof(q->stereo_views));
			}
			else assert(0);

			return !DDRAW::xr;
		}
		else assert(!DDRAW::xr);

		DDRAW::xr = false; //PARANOID

		auto s = x->xinit(DDRAW::xrSuperSampling);
		
		if(!s) return false;
	}

	int res = x->xbegin();
	
	if(res){ x->xquit(); return false; } //I guess?

	//subsequent calls are made by dx_d3d9X_present
	x->xwait();

	resize_only: //just changing super sample size?
		
	BOOL xr = 0;

	auto &sv = d->queryGL->stereo_views;
	memset(sv,0x00,sizeof(sv));
	if(auto*f=x->xstatus()?x->xfirst():nullptr)
	for(int i=0;i<_countof(sv);i++,f=f->next())
	{		
		//QUAD isn't necessarily the same size
		//really "Varjo" is the only quad mode
		assert(i<2);

		auto &v = sv[i];
		auto *s = f?f->find_sections():nullptr;
		if(!s) break;

		v.src = s; xr++;

		/*need to defer this since supersampling
		//may change and in theory something else
		//may change the resolution
		DDRAW::doSuperSamplingMul(*(RECT*)
		memcpy(&v.x,s->virtual_off,4*sizeof(int)));
	
		if(i&1) v.x+=sv[i-1].w; //HACK?
		if(i>1) v.y+=sv[i-2].h; //QUAD?
		*/

		if(s->iterator_count==1) break; //YUCK
	}
	if(!sv[0].src) //xstatus? 
	{
		assert(0); //UNEXPECTED

		x->xquit(); return false; //I guess?
	}

	DDRAW::xr = xr; //DDRAW::xr = true;

	//FYI: the caller is responsible for rebuilding
	//the effects buffers and shaders
	//i.e. fx9_effects_buffers_require_reevaluation

	dx_d3d9X_xr_discard = 30; //TESTING

	dx_d3d9X_vao_enable_xr(true);

	return DDRAW::inStereo = true;
}
//extern bool DDRAW::stereo_enable_OpenXR(const char *hlsl, void *c, size_t c_s)
extern bool DDRAW::fxWGL_NV_DX_interop2(const char *hlsl, void *c, size_t c_s)
{
	//if(!DDRAW::xr){ assert(0); return false; }
	if(!DDRAW::WGL_NV_DX_interop2){ assert(0); return false; }

	auto d = DDRAW::Direct3DDevice7;
	auto x = d->queryGL->xr;

	d->queryGL->effects_cbuf = c;
	d->queryGL->effects_cbuf_size = c_s;
	bool ret = true;
	if(hlsl)
	{
		Widgets95::xr::effects fx = {};
		d->queryGL->effects_hlsl = d->queryGL->effects_hlsl;
		ret = fx.reset_pshader(x->first_surface(),hlsl);
		d->queryGL->effects_hlsl = fx.hlsl_pshader;
	}
	else d->queryGL->effects_hlsl = nullptr; return ret;
}
void dx_d3d9X_RotateVectorForD3DX(D3DXVECTOR3 &pos, D3DXVECTOR3 &v, D3DXQUATERNION &q)
{
	//YUCK: D3DX doesn't include quaternion*vector math?!
	//return v*(q.w*q.w-dot(q.xyz,q.xyz))+2.0*q.xyz*dot(q.xyz,v)+2.0*q.w*cross(q.xyz,v);
	// 
//	auto &q = ior; //SHADOWING
//	auto &v = svp;
	//first term
	float dot = 0.0f;
	for(int j=3;j-->0;) //dot(q.xyz,q.xyz) 
	dot+=q[j]*q[j];
	D3DXVECTOR3 t1 = v*(q.w*q.w-dot);
	//second term
	dot = 0.0f;
	for(int j=3;j-->0;) //dot(q.xyz,v)
	dot+=q[j]*v[j];
	D3DXVECTOR3 t2 = *(D3DXVECTOR3*)&q*(2.0f*dot);
	//third term
	D3DXVECTOR3 t3;
	D3DXVec3Cross(&t3,(D3DXVECTOR3*)&q,&v);
	t3 = t3*(2.0f*q.w);
	for(int j=3;j-->0;) pos[j] = t1[j]+t2[j]+t3[j];
}
extern float(*DDRAW::stereo_locate_OpenXR_mats)[4][4] = 0;
extern float dx_d3d9c_update_vsPresentState_staircase(int);
extern bool DDRAW::stereo_locate_OpenXR(float nz, float fz, float io[4+3], float post[4])
{
	auto d = DDRAW::Direct3DDevice7;

	if(!d||!DDRAW::xr){ assert(0); return false; }

	auto *q = d->queryGL;
	auto *x = q->xr;

	auto *s = (const Widgets95::xr::section*)q->stereo_views[0].src;
	if(!s||!s->projection_fov[0])
	{
		//this will overrun the fovea buffer?
		assert(0); return false; //breakpoint
	}
		
	BYTE *need_f = 0; int fw,fh,fp;
	
	//NOTE: I don't think the far plane or near plane should move
	//the center of projection, but if so, they should be compared
	//(either way I think this will be an acceptable approaximation)
	//(NOTE: memcmp only tests one eye/view surface)
	if(glBindShadingRateImageNV) if(!q->stereo_fovea
	||memcmp(q->stereo_fovea_cmp,s->projection_fov,4*sizeof(float)))
	{
		memcpy(q->stereo_fovea_cmp,s->projection_fov,4*sizeof(float));

		glBindShadingRateImageNV(0); //NECESSARY?

		glGetIntegerv(GL_SHADING_RATE_IMAGE_TEXEL_WIDTH_NV,&fw);
		glGetIntegerv(GL_SHADING_RATE_IMAGE_TEXEL_HEIGHT_NV,&fh);	

		GLsizei w = q->effects_w/fw+1; fp = w; 
		GLsizei h = q->effects_h/fh+1;

		bool z = !q->stereo_fovea; 
		
		if(z) glGenTextures(1,&q->stereo_fovea);

		glBindTexture(GL_TEXTURE_2D,q->stereo_fovea);

		if(z) glTexStorage2D(GL_TEXTURE_2D,1,0x8232,w,h); //GL_R8UI

		//TODO: maybe grow this but never deallocate
		need_f = new BYTE[w*h]();
	}
	
	bool need_p = need_f||stereo_locate_OpenXR_mats;

	x->xorient(io); //HACK: timing is kind of dicey

	enum
	{
		vecs = 4, //fov/rot/pos/vp1/vp2
		secs = _countof(q->stereo_views), //2?
		stride = vecs*4, //x/y/z/w
		len = secs*stride,
		mem = len*sizeof(float)
	};

	//this is updating the per-instance stereo VBO
	//with the game character/camera pose provided
	//by io (that will return the end-user's pose)

	io[0] = -io[0];
	io[1] = -io[1];
	io[6] = -io[6];
	float buf[len] = {}, *p = buf;
	for(int i=0;i<secs;i++,p+=stride)
	{
		auto &v = q->stereo_views[i];
		auto *s = (const Widgets95::xr::section*)v.src; //P0289R0
		if(!s) break;

		float *fov = p;
		float *pos = p+4;
		float *rot = p+8;
		float *vp = p+12;
		vp[0] = nz;
		vp[1] = fz;
		vp[2] = 1.0f/v.w;
		vp[3] = 1.0f/v.h;

		//fov is in radians for some reason
		for(int j=4;j-->0;)
		fov[j] = tanf(s->projection_fov[j]);
		
		auto &ior = *(D3DXQUATERNION*)io;
		auto svr = *(D3DXQUATERNION*)s->model_view_rot;		
		auto svp = *(D3DXVECTOR3*)s->model_view_pos;
		svr[0] = -svr[0]; 
		svr[1] = -svr[1];
		svp[2] = -svp[2];
				
		//rot is an OpenGL-like quaternion
		//it replaces the model-view matrix
		//so it's inverted
		// 
		// NOTE: after xorient io only contains
		// yaw information
		// 
		//memcpy(rot,&svr,sizeof(svr)); rot[3] = -rot[3];
		D3DXQUATERNION tmp;
		D3DXQuaternionMultiply(&tmp,&svr,&ior);
		if(post) //REMOVE ME
		{
			D3DXQuaternionMultiply(&tmp,(D3DXQUATERNION*)post,&svr);
			D3DXQUATERNION tmp2 = tmp;
			D3DXQuaternionMultiply(&tmp,&tmp2,&ior);			
		}
		else D3DXQuaternionMultiply(&tmp,&svr,&ior);
		D3DXQuaternionInverse((D3DXQUATERNION*)rot,&tmp);		
		if(1)
		{
			D3DXQUATERNION tmp2 = tmp;
			D3DXQuaternionNormalize(&tmp,&tmp2);
			tmp = tmp2;
		}
		
		//pos needs to be multiplied by the
		//"parent" transform and inverted to
		//likewise form the model-view matrix
		//YUCK: D3DX doesn't include quaternion*vector math?!		
		//return v*(q.w*q.w-dot(q.xyz,q.xyz))+2.0*q.xyz*dot(q.xyz,v)+2.0*q.w*cross(q.xyz,v);
		dx_d3d9X_RotateVectorForD3DX(*(D3DXVECTOR3*)pos,svp,ior);
		for(int j=3;j-->0;)
		pos[j] = -(pos[j]+io[4+j]);
		//REPURPOSING
		//pos.w isn't needed for anything, so it's
		//overloaded by this value so stereo shaders
		//can use it to reconstruct vsPresentState
		pos[3] = dx_d3d9c_update_vsPresentState_staircase(i%2);

		D3DXMATRIX p; if(need_p)
		{
			D3DXMatrixPerspectiveOffCenterLH(&p,nz*fov[0],nz*fov[1],nz*fov[2],nz*fov[3],nz,fz);
		}

		if(auto*oo=stereo_locate_OpenXR_mats) 
		{
			D3DXMATRIX v;
			D3DXMatrixRotationQuaternion(&v,(D3DXQUATERNION*)rot); 
			//memcpy(v.m[3],pos,sizeof(float)*3);
			dx_d3d9X_RotateVectorForD3DX(*(D3DXVECTOR3*)v.m[3],*(D3DXVECTOR3*)pos,*(D3DXQUATERNION*)rot);			
			D3DXMatrixMultiply((D3DXMATRIX*)oo[i],&v,&p);
		}

		if(need_f)
		{
			int cx,cy; //find center point?
			{
				D3DXVECTOR4 _c(0,0,-nz,1),c;
				D3DXVec4Transform(&c,&_c,&p);
				cx = (int)((v.x+v.w*(c.x/c.w+1)/2)/fw+0.5f);
				cy = (int)((v.y+v.h*(c.y/c.w+1)/2)/fh+0.5f);

				assert(cx!=0x80000000); //???
			}

			//I copied this code and constants comes the following
			//github project:
			//https://github.com/fholger/thedarkmodvr/blob/master/renderer/vr/VRFoveatedRendering.cpp
		//	float r = 0.5f*h;
		//	float fOuter = 0.85f*r; //vr_foveatedOuterRadius
		//	float fMid = 0.75f*r; //vr_foveatedMidRadius
		//	float fInner = 0.3f*r; //vr_foveatedInnerRadius
			//0.05 increments up to sqrt(2) (1.41)
			//20 is the number increments up to 1 (1/0.05)
			float l_r_20 = 1/((float)v.h/fh*0.5)*20;
			//+4 is to make up for some imprecision
			BYTE palette[20+10] = {1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,3,3,4,4,4,+4}; //1.0
			
			//int x = v.x/fw, w = v.w/fw;
			//int y = v.y/fh, h = v.h/fh;	
			int x = (int)((float)v.x/fw+0.5f);
			int xw = (int)((float)(v.x+v.w)/fw+0.5f);
			int y = (int)((float)v.y/fh+0.5f);
			int yh = (int)((float)(v.y+v.h)/fh+0.5f);

			float m1 = (float)(cx-x)/(xw-x);
			float m2 = (1-m1)*2;m1*=2;

			//I think this needs to be upside-down
			//for(int i=y+h;i-->y;)
			for(int i=yh;i-->y;)
			{
				BYTE *row = need_f+fp*i;

				//for(int j=x+w;j-->x;)
				for(int j=xw;j-->x;)
				{
					int jx = j-cx, iy = i-cy;

					jx*=j>cx?m1:m2;

					float dist = sqrtf(jx*jx+iy*iy);

					/*maybe just divide into palette?
					if(dist>=r) *p = 0
					else if(dist>=outerRadius) *p = 4;
					else if(dist>=midRadius) *p = 3;
					else if(dist>=innerRadius) *p = 2;
					else *p = 1;*/
					int pe = (int)(dist*l_r_20); assert(pe<_countof(palette));
					row[j] = palette[pe];
				}
			}
		}
	}
	if(!q->stereo)
	glGenBuffers(1,&q->stereo);
	glBindBuffer(GL_ARRAY_BUFFER,q->stereo);
	glBufferData(GL_ARRAY_BUFFER,mem,buf,GL_STREAM_DRAW); //GL_DYNAMIC_DRAW?

	if(need_f)
	{
		GLsizei w = q->effects_w/fw+1;
		GLsizei h = q->effects_h/fh+1;
		glBindTexture(GL_TEXTURE_2D,q->stereo_fovea);
		glPixelStorei(0x0CF5,1); //GL_UNPACK_ALIGNMENT (damn you OpenGL!!!)
		glTexSubImage2D(GL_TEXTURE_2D,0,0,0,w,h,0x8D94,GL_UNSIGNED_BYTE,need_f); //GL_RED_INTEGER
		glPixelStorei(0x0CF5,4);

		glBindTexture(GL_TEXTURE_2D,0);

		delete[] need_f;		

		//NOTE: THIS CAN'T BE DONE BEFORE glTexSubImage2D
		//(or it will generate an GL_INVALID_VALUE error)

		glBindShadingRateImageNV(q->stereo_fovea);
			
		//HACK: until I can figure out the mask requirements for
		//a Pimax set this just fills in every pixel 
	//	int pimax = GL_SHADING_RATE_NO_INVOCATIONS_NV;
	//	if(q->stereo_ultrawide)
	//	pimax = GL_SHADING_RATE_1_INVOCATION_PER_4X4_PIXELS_NV; 

		GLenum palette[5];
		/*bright clear colors are very visible. the edge of the
		//frame buffer is right on the edge of my HP Reverb G2
		palette[0] = pimax;*/
		palette[0] = GL_SHADING_RATE_1_INVOCATION_PER_4X4_PIXELS_NV;
		palette[1] = GL_SHADING_RATE_1_INVOCATION_PER_PIXEL_NV;
		palette[2] = GL_SHADING_RATE_1_INVOCATION_PER_2X2_PIXELS_NV;
		palette[3] = GL_SHADING_RATE_1_INVOCATION_PER_2X4_PIXELS_NV; //!!
		palette[4] = GL_SHADING_RATE_1_INVOCATION_PER_4X4_PIXELS_NV;
		glShadingRateImagePaletteNV(0,0,_countof(palette),palette);

		glEnable(GL_SHADING_RATE_IMAGE_NV);
	}

	//HACK: probably want to make this a separate API
	//if consumers intend to use it to drive movement
	memcpy(io,x->xpose()->center.rot,7*sizeof(float)); return true;
}
template<int X>
HRESULT D3D9X::IDirect3D7<X>::CreateDevice(REFCLSID xIn, DX::LPDIRECTDRAWSURFACE7 y, DX::LPDIRECT3DDEVICE7 *z)
{
	const GUID *x = &xIn;

	DDRAW_LEVEL(7) << "D3D9X::IDirect3D7::CreateDevice()\n";
	
	//DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK);

	DDRAW_PUSH_HACK(DIRECT3D7_CREATEDEVICE,IDirect3D7*,	
	const GUID*&,DX::LPDIRECTDRAWSURFACE7&,DX::LPDIRECT3DDEVICE7*&)(0,this,x,y,z);

	if(7<=DDRAWLOG::debug)
	{
		LPOLESTR o; if(StringFromCLSID(*x,&o)==S_OK) DDRAW_LEVEL(7) << ' ' << o << '\n';
								
		if(*x==DX::IID_IDirect3DRGBDevice)
		{
			DDRAW_LEVEL(7) << "(IDirect3DRGBDevice)\n";
		}
		else if(*x==DX::IID_IDirect3DHALDevice)
		{
			DDRAW_LEVEL(7) << "(IDirect3DHALDevice)\n";
		}
		else if(*x==DX::IID_IDirect3DTnLHalDevice)
		{
			DDRAW_LEVEL(7) << "(IDirect3DTnLHalDevice)\n";
		}
		else assert(0); //unexpected behavior
	}
		
	auto psurf = DDRAW::is_proxy<IDirectDrawSurface7>(y);

	if(!psurf||psurf->query9->group9!=dx_d3d9c_virtual_group) 
	{
		DDRAW_FINISH(!D3D_OK) //paranoia	  
	}

	DX::DDSURFACEDESC2 zbuffer, primary; //note: primary here can be the backbuffer

	zbuffer.dwSize = primary.dwSize = sizeof(DX::DDSURFACEDESC2);

	//C4533
	{
		DX::LPDIRECTDRAWSURFACE7 zsurf = 0;
		DX::DDSCAPS2 zcaps = { DDSCAPS_ZBUFFER };
		if(psurf->GetAttachedSurface(&zcaps,&zsurf)==D3D_OK)
		{	
			zsurf->GetSurfaceDesc(&zbuffer); zsurf->Release();
			psurf->GetSurfaceDesc(&primary);
		}
		else DDRAW_FINISH(!D3D_OK)

		//2021: this is expected below
		if(!DDRAW::BackBufferSurface7)
		{
			zcaps.dwCaps = DDSCAPS_BACKBUFFER;
			//ItemEdit passes an offscreen-surface to CreateDevice
			//that's not its back-buffer... maybe generating fonts
			//psurf->GetAttachedSurface(&zcaps,&zsurf);
			if(DDRAW::PrimarySurface7)
			if(!DDRAW::PrimarySurface7->GetAttachedSurface(&zcaps,&zsurf))
			zsurf->Release();
			if(!DDRAW::BackBufferSurface7) //no primary?
			{
				assert(0); DDRAW_FINISH(!D3D_OK);
			}
		}
	}	

	const auto w = primary.dwWidth, h = primary.dwHeight; //2021
	
	//D3DDEVTYPE dev = *x==DX::IID_IDirect3DRGBDevice?D3DDEVTYPE_SW:D3DDEVTYPE_HAL;	
	//if(dev==D3DDEVTYPE_SW) dev = D3DDEVTYPE_HAL;
	auto dev = D3DDEVTYPE_HAL;
	/*2021: Seeing if "deep color" helps? I got the idea from ANGLE
	//defaulting to 10:10:10 color... even though it seems wrong to
	//me since the monitor was 8:8:8!
	D3DFORMAT highcolor = DDRAW::do565?D3DFMT_R5G6B5:D3DFMT_X1R5G5B5;
	D3DFORMAT fxbpp = DDRAW::bitsperpixel==16?highcolor:dx_d3d9_fxformat;*/
	D3DFORMAT fxbpp = D3DFMT_A8R8G8B8;
	//I can't really see any difference... better or worse. it might be good for the
	//brightness adjustment, to be less destructive? since it's not known what's the
	//back-buffer format, in theory this should ensure the effects buffer isn't less
	if(0) //UNUSED
	{
		//OpenGL doesn't have an sRGB format for 10/30-bit color, and I don't
		//know if that means D3D is not using it really (I can't see a difference)
		//or if it's more expensive. not even GL_EXT_texture_sRGB_decode will
		//convert non 8-bit formats

		if(!proxy9->CheckDeviceFormat(query9->adapter,dev,D3DFMT_X8R8G8B8,
		//D3DUSAGE_QUERY_SRGBREAD| //???
		D3DUSAGE_RENDERTARGET,D3DRTYPE_TEXTURE,D3DFMT_A2R10G10B10))
		fxbpp = D3DFMT_A2R10G10B10;
	}

	//Disable doFlipEx without Windows 7+
	if(DDRAW::doFlipEx&&!D3D9C::DLL::Direct3DCreate9Ex
	||GetVersion()<0x106) DDRAW::doFlipEx = false;
	
	//EXPERIMENTAL (FAILED)
	//try to get a D3D backbuffer for OpenXR interop
	//NOTE: there's more to this at the very bottom
	//if(!X) dx_d3d9X_d3d12(this,w,h,fxbpp);
	auto d12 = DDRAW::D3D12Device;
		
	D3DDISPLAYMODE dm;	  
	proxy9->GetAdapterDisplayMode(query9->adapter,&dm);	
	DDRAW::refreshrate = dm.RefreshRate; //2021
	switch(DDRAW::refreshrate)
	{
	case 59: DDRAW::refreshrate = 60; break;
	}
	D3DFORMAT bpp = DDRAW::fullscreen?dm.Format:D3DFMT_UNKNOWN; 

	D3DFORMAT dsbpp = D3DFMT_D16;
	switch(zbuffer.ddpfPixelFormat.dwZBufferBitDepth)
	{
	case 24: dsbpp = D3DFMT_D24X8; break; 
		//D3D10 and OpenGL ES only have 32F formats
	case 32: dsbpp = target=='dx9c'?D3DFMT_D32:D3DFMT_D24X8; break;
	}
	if(DDRAW::DepthStencilSurface7) //2021		
	DDRAW::DepthStencilSurface7->queryX->nativeformat = dsbpp;
	else assert(0);

	D3DPRESENT_PARAMETERS pps;
	//2: triple buffering? //tearing in fullscreen
	//1: not seeing a benefit???
	//2: D3DSWAPEFFECT_FLIPEX wants 2
	//3: 2 doesn't seem to make a difference with FLIPEX
	//so I think that maybe non-FLIPEX might benefit from
	//3, in order to not exclude Vista/XP owners
	//1: must be 1 for non real-time productivity tools
	//ALSO Present with a rectangle fails if it's not 1
	//(DDRAW::?)
	//
	// 2021: maybe because SetMaximumFrameLatency(1) was 
	// called below? NOTE (1) may help for input feel???
	//
	pps.BackBufferCount = 1; //doFlipEx sets this below
	pps.BackBufferWidth = X||d12?1:w; //hack
	pps.BackBufferHeight = X||d12?1:h; //hack	
	pps.BackBufferFormat = bpp;

	//the backbuffer is not antialiased
	pps.MultiSampleType = D3DMULTISAMPLE_NONE;
	pps.MultiSampleQuality = 0;

	pps.hDeviceWindow = DDRAW::window;
	pps.Windowed = X?1:!DDRAW::fullscreen;
	 

		//TODO? utilize D3DPRESENTSTATS?


	if(X) 
	{
		//TODO? OpenGL might run smoother
		//with FLIPEX:
		//https://www.khronos.org/registry/OpenGL/extensions/NV/WGL_NV_DX_interop.txt
	}
	

	//trying to override if D3DSWAPEFFECT_FLIPEX... 
	pps.PresentationInterval = DDRAW::isSynchronized?
	D3DPRESENT_INTERVAL_ONE:D3DPRESENT_INTERVAL_IMMEDIATE; //DEFAULT	

	if(!X&&DDRAW::doFlipEx&&pps.Windowed) 
	{
		/*2020: I'm trying to average the timing frame to frame
		3 seems excessive?
		//I think 3 evens things out to be as good as it can be
		//if the timing was managed by DDRAW::doFlipEX_MaxTicks
		pps.BackBufferCount = 3;*/
		pps.BackBufferCount = DDRAW::doTripleBuffering?2:1;
		pps.SwapEffect = (D3DSWAPEFFECT)5; //D3DSWAPEFFECT_FLIPEX;
		//I think this leaves no hope for
		//throttling
		//pps.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	}
	else if(DDRAW::doFlipDiscard||DDRAW::doFlipEx) //August 2021
	{
		//dx_d3d9c_present now fails unless COPY is used like the
		//documentation says to use
		pps.SwapEffect = D3DSWAPEFFECT_DISCARD; //game like
	}
	else pps.SwapEffect = D3DSWAPEFFECT_COPY; //tool like
								
	//now deferring depthbits
	pps.EnableAutoDepthStencil = 0; 
	//pps.AutoDepthStencilFormat = X?0:dsbpp;

	pps.Flags =			
	(pps.EnableAutoDepthStencil
	?D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL:0);
//	|D3DPRESENTFLAG_DEVICECLIP;	//D3DSWAPEFFECT?
//	|D3DPRESENTFLAG_NOAUTOROTATE; //potential optimization	
		
	//note: required to GetDC backbuffer
	/*GetDC returning EX::window HDC for now*/
	//if(!msaa) //hack: does not fly for multisampling
	//pps.Flags|=D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

	pps.FullScreen_RefreshRateInHz =
	!X&&DDRAW::fullscreen?60:0; //primary.dwRefreshRate; 
	
	//if(DDRAW::fullscreen) //testing (seems to be ignored)
	//pps.PresentationInterval = D3DPRESENT_INTERVAL_TWO;

	//note: requires Desktop Window Manager (DWM)
	//support Aero and taskbar thumbnails under Vista/7 
//	pps.PresentationInterval|=D3DPRESENT_VIDEO_RESTRICT_TO_MONITOR;

	DWORD behavior = D3DCREATE_HARDWARE_VERTEXPROCESSING;

	if(DDRAW::doMultiThreaded) behavior|=D3DCREATE_MULTITHREADED;

	//NOTE: FOR OpenGL THIS WILL A DUMMY/RELEASED BEFORE RETURNING
	IDirect3DDevice9 *d = 0;
	IDirect3DDevice9Ex* &dEx = *(IDirect3DDevice9Ex**)&d;

	HWND wth = !X&&DDRAW::fullscreen?0:DDRAW::window; //???
		
		//TESTING
		//sometimes CreateDeviceEx returns E_ACCESSDENIED
		//and sometimes it doesn't, but CreateSwapChainForHwnd
		//in xcv_d3d11.cpp always returns it
		dx_d3d9c_force_release_old_device(); 

	assert(out); do //NEW: triple buffering?
	{
		if(D3D9C::DLL::Direct3DCreate9Ex)
		{
			D3DDISPLAYMODEEX dmEx, *mode = 0; 
		
			if(!X&&DDRAW::fullscreen) //docs do not say so 
			{
				dmEx.Size = sizeof(D3DDISPLAYMODEEX);
				dmEx.Width = w; 
				dmEx.Height = h;
				dmEx.Format = bpp; 
				
				//0 is erratic
				//trying to match //pps.FullScreen_RefreshRateInHz
				dmEx.RefreshRate = primary.dwRefreshRate;
				pps.FullScreen_RefreshRateInHz = dmEx.RefreshRate;

				dmEx.ScanLineOrdering = DDRAW::isInterlaced?
				D3DSCANLINEORDERING_INTERLACED:D3DSCANLINEORDERING_PROGRESSIVE;

				mode = &dmEx; //but "NULL" must be passed in windowed mode
			}

			//	int retries = 0; //debugging

retry:		if(out=proxy9->CreateDeviceEx(query9->adapter,dev,wth,behavior,&pps,mode,&dEx))
			pps.FullScreen_RefreshRateInHz = 0;

			if(out!=D3D_OK)	
			{
				//FIX ME //DUPLICATE
				if(out==E_ACCESSDENIED&&dx_d3d9c_old_device) //YUCK
				{
					//MOVED ABOVE FOR OpenGL (DOESN'T ALWAYS TRIGGER)
					assert(0); 

					dx_d3d9c_force_release_old_device(); goto retry;
				}

				//NEW: try fewer buffers before giving up on IDirect3DDevice9Ex
				//just in case hardware/memory imposes a limit... in which case
				//the limit is probably among advertised capabilities, but it's
				//easier to find out this way
				if(1==pps.BackBufferCount) 
				{
					D3D9C::DLL::Direct3DCreate9Ex = 0; 
					if(pps.SwapEffect==(D3DSWAPEFFECT)5)
					{
						pps.SwapEffect = D3DSWAPEFFECT_DISCARD;					
						pps.PresentationInterval = DDRAW::isSynchronized?
						D3DPRESENT_INTERVAL_ONE:D3DPRESENT_INTERVAL_IMMEDIATE; //DEFAULT	
					}

					//OBSOLETE? I'm thinking there cannot possibly be any benefit
					//in trying this, but may as well for back-compatibility sake
					goto without_Ex;
				}
			}
			else if(pps.SwapEffect==(D3DSWAPEFFECT)5)
			{
				//dEx->SetMaximumFrameLatency(1); //2021?????????????
			}
			dx_d3d9c_old_device = 0; //2021
		}
		else without_Ex: //should implicate Windows XP (or a legit error)
		{
			out = proxy9->CreateDevice(query9->adapter,dev,wth,behavior,&pps,&d);
			assert(out||(GetVersion()&0xFF)<6);
		}

	}while(out!=D3D_OK&&--pps.BackBufferCount);

	if(d)
	{
		d->SetDialogBoxMode(TRUE);

		//dx_d3d9X_SEPARATEALPHABLENDENABLE_knockout(bool e);
		d->SetRenderState(D3DRS_COLORWRITEENABLE,7);
	}
					 
	bpp = pps.BackBufferFormat;

	//multisampling must match the render target
	if(!pps.EnableAutoDepthStencil&&out==D3D_OK)
	{
		if(dsbpp==D3DFMT_D16) 
		if(proxy9->CheckDepthStencilMatch(query9->adapter,dev,bpp,fxbpp,D3DFMT_D24X8)==D3D_OK) 
		dsbpp = D3DFMT_D24X8;
		//D3D10 and OpenGL ES only have 32F formats
		if(target=='dx9c')
		if(dsbpp==D3DFMT_D24X8)
		if(proxy9->CheckDepthStencilMatch(query9->adapter,dev,bpp,fxbpp,D3DFMT_D32)==D3D_OK) 
		dsbpp = D3DFMT_D32;		
		
		//2021: fx9_effects_buffers_require_reevaluation does the 
		//actual work to allocate the depth-stencil buffers below

		if(DDRAW::DepthStencilSurface7) //2021		
		DDRAW::DepthStencilSurface7->queryX->format = dsbpp;
		else assert(0); 
	}
	else assert(0); //2021
	
	HDC dc = 0; //2024

	HGLRC wgl = 0;

	Widgets95::xr *xr = 0;

	DDRAW::WGL_NV_DX_interop2 = false;

	if(X&&out==D3D_OK) //OpenGL?
	{
		using namespace Widgets95;
				
		int xe = 1; //2022
		int dm = 3; //glutext::GLUT_GLES|glute::GLUT_DOUBLE

		if(target=='dx95') nogle: //ANGLE? //'dxGL'?
		{
			//try to use WGL_NV_DX_interop2?
			//NOTE: if this fails users must use D3D9 instead
			int d3d11 = xe==xr_NOGLE_OpenGL_WIN32&&DDRAW::doFlipEx?2:1; 

			DDRAW::WGL_NV_DX_interop2 = 2==d3d11;

			xr = new class xr(xr::make_attribs(xe,dm));
			xr->attribs.context_flags = DX::debug;
			proxy9->GetAdapterLUID(query9->adapter,&xr->attribs.display_adapter_luid);
			if(!xr->make_current
			(new xr::surface(xr,(void*)DDRAW::window,d3d11)
			,new xr::context(xr)))
			{
				out = !D3D_OK; delete xr; xr = 0; assert(0);
			}

			if(xr&&xe==xr_NOGLE_OpenGL_WIN32) //2022
			{
				//HACK: still need to do dx_d3d9X_dxGL_init but
				//without its context creation
				void* &arb = dx_d3d9X_dxGL_arb(queryX->adapter);
				if(!arb) dx_d3d9X_dxGL_load();
				arb = (void*)1; //HACK
			}
		}
		
		if(target=='dxGL') //opengl32.dll?
		{
		//	if(xe==DX::debug) //EXPERIMENTAL (2022)
			if(xe==1)
			{
				dm&=~1; //remove glutext::GLUT_GLES?

				//if dxGL is going to be used with OpenXR
				//it will have to do so via Widgest95::xr.

				xe = xr_NOGLE_OpenGL_WIN32; goto nogle;
			}

			if(!xr) //OBSOLETE?
			{
				wgl = dx_d3d9X_dxGL_init(queryX->adapter,&dc);
				out = !wgl;
			}
					
			//2022: 1 is the default. Or is it???
			// 
			// WARNING: I think 1 is supposed to be the default
			// but it's clearly not! Confirmed for Intel or AMD.
			// 
			// NOTE: xr is setting this every time swap_buffers
			// is called.
			//
			if(void*ext=wglGetProcAddress("wglSwapIntervalEXT"))
			{
				((BOOL(WINAPI*)(int))ext)(1); //not default 
			}
		}
		else assert(0);

		#ifdef _DEBUG
		if(!out) glDebugMessageCallback(dx_d3d9X_glMsg_db,0);
		#endif
	}

	DDRAW::IDirect3DDevice7 *p = 0;

	if(out==D3D_OK)
	{	
		assert(!query->d3ddevice); //assuming singular device

		p = new DDRAW::IDirect3DDevice7(DDRAW::target);

		p->query->d3d = query; query->d3ddevice = p->query;		
		
		p->proxy9 = (IDirect3DDevice9Ex*)d;		

		//	p->query9->depthstencil = depthstencil; 
		
		p->AddRef(); //obsolete? see Release method		 

		p->queryX->emulate = 0; if(D3D9C::doEmulate)
		{
			if(*x==DX::IID_IDirect3DRGBDevice) p->query9->emulate = 'rgb';
			if(*x==DX::IID_IDirect3DHALDevice) p->query9->emulate = 'hal';
			if(*x==DX::IID_IDirect3DTnLHalDevice) p->query9->emulate = 'tnl';
		}
		
		assert(!DDRAW::ff); //DDRAW::ff = false; 

		if(X) p->queryGL->xr = xr;
		if(X) p->queryGL->wgl = wgl;
		if(X) p->queryGL->wgldc = dc;
	}
	
	UINT ww = w, hh = h;
	DDRAW::doSuperSamplingMul(ww,hh);
	
	if(out==D3D_OK)
	{			
		if(X) //intialize/configure OpenGL to behave like D3D
		{
			auto q = p->queryGL;
			q->state.init();
			glDepthFunc(0x0203); //GL_LEQUAL (LESS is default)
			glEnable(0x0B44); //GL_CULL_FACE
			glCullFace(0x0405); //GL_BACK
			//don't need this if y is set to -y in the shader
			//glFrontFace(0x0900); //GL_CW

			dx_d3d9X_pipelines.reserve(16);
			
			glGenSamplers(3,q->samplers);
			for(int i=3;i-->0;) 
			{
				auto s = q->samplers[i];
				glBindSampler(i,s); 
				//match state.init() (GL_REPEAT seems to be default)
				//glSamplerParameteri(s,GL_TEXTURE_WRAP_S,GL_REPEAT);
				//glSamplerParameteri(s,GL_TEXTURE_WRAP_T,GL_REPEAT);
				//NOTE: GL_NEAREST_MIPMAP_NEAREST wipes out 2x2 dither
				glSamplerParameteri(s,GL_TEXTURE_MIN_FILTER,GL_NEAREST/*_MIPMAP_NEAREST*/);
				glSamplerParameteri(s,GL_TEXTURE_MAG_FILTER,GL_NEAREST);				
			}						
			glGenBuffers(2,q->uniforms);
			q->uniforms2 = new DDRAW::IDirect3DDevice::QueryGL::UniformBlock[2];
			for(int i=2;i-->0;)
			{
				//this is a rare instance where OpenGL makes it impossible to
				//easily coordinate multiple software components on a context
				int binding = i*3+DDRAW::compat_glUniformBlockBinding_1st_of_6;

				//ANGLE sets UNIFORM_BUFFER_OFFSET_ALIGNMENT to 256 bytes for
				//D3D11. since D3D9's bool registers are too small for it and
				//not used it seems best to ignore them right now
				//REMINDER: 4+16+224 is 244
				enum{ chunk=16*4*4 }; //256B
				using namespace DDRAW;
				auto fmost = min(224,(unsigned)(i?psI-psF:vsI-vsF));
				if(fmost%16) fmost = (fmost/16+1)*16;
				assert(vsI&&psI&&!vsB&&!psB);
				q->uniforms2[i].f = new float[fmost*4]();
				q->uniforms2[i].fmost = fmost;

				glBindBuffer(GL_UNIFORM_BUFFER,q->uniforms[i]);
				//this is the limit for ps_3_0. vs_3_0 is 256 minimum, but 
				//may go higher if queried. just going to assume 224 is ok
				//GLSL shaders are required to set up an "interface block"
				//for each register type it will use, called vsB, vsI, etc.
				//glBufferData(GL_UNIFORM_BUFFER,(16+4*16+4*224)*4,0,GL_STREAM_DRAW);
				glBufferData(GL_UNIFORM_BUFFER,(16*4+fmost*4)*4,0,GL_STREAM_DRAW);
				//here 4 is either as in int4 or sizeof(int) as in 4 bytes
				//glBindBufferRange(GL_UNIFORM_BUFFER,binding+0,q->uniforms[i],0,16*4);
				//glBindBufferRange(GL_UNIFORM_BUFFER,binding+1,q->uniforms[i],16*4,16*4*4);
				//glBindBufferRange(GL_UNIFORM_BUFFER,binding+2,q->uniforms[i],16*5*4,224*4*4);
				glBindBufferRange(GL_UNIFORM_BUFFER,binding+1,q->uniforms[i],0,chunk);
				glBindBufferRange(GL_UNIFORM_BUFFER,binding+2,q->uniforms[i],chunk,fmost*4*4);
			}			
		}

		assert(~pps.Flags&D3DPRESENTFLAG_LOCKABLE_BACKBUFFER);
		/*REMINDER: dx_d3d9X_backbuffer9 CAN'T WORK AT THIS STAGE
		IDirect3DSurface9 *bbuffer = 0;
		if(pps.Flags&D3DPRESENTFLAG_LOCKABLE_BACKBUFFER
		&&d->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&bbuffer)==D3D_OK)
		{
			//TODO? could override proxy9?
			//dx_d3d9c_backbuffer_pitch =
			DDRAW::BackBufferSurface7->queryX->pitch = dx_d3d9c_pitch(bbuffer);
			bbuffer->Release();
		}
		else*/ //not ideal //BOGUS?
		{
			//dx_d3d9c_backbuffer_pitch = 
			DDRAW::BackBufferSurface7->queryX->pitch = 			
			pps.BackBufferWidth*dx_d3d9c_format(bpp).dwRGBBitCount/8;
		}
		//dx_d3d9c_backbuffer_format = bpp;
		DDRAW::BackBufferSurface7->queryX->format = bpp;

		//2021: because of supersampling can't spoof effects 
		//buffer when requesting backbuffer descriptor (GetDesc)
		//dx_d3d9c_rendertarget_pitch = //defaults
		//DDRAW::BackBufferSurface7->queryX->pitch; 
		//dx_d3d9c_rendertarget_format = bpp;
	//	assert(fxbpp==bpp); //X8->A8
		//dx_d3d9c_rendertarget_format = fxbpp;
		DDRAW::mrtargets9[0] = fxbpp;
		//2021: this now does Z+FX+MRT
		//
		//
		//
		DDRAW::Direct3DDevice7 = p; if(DDRAW::xr)
		{
			DDRAW::stereo_toggle_OpenXR(false); //EXPERIMENTAL
		}
		DDRAW::fx9_effects_buffers_require_reevaluation(); //2021
		//
		//
		//
		//MRT WRT D3DRS_COLORWRITEENABLE123
		if(X) 
		{
			auto &st = p->queryGL->state;
			auto cmp = st;
			for(int i=3;i-->0;) 
			if(p->queryGL->mrtargets[i])
			st.colorwriteenable|=1<<i;
			st.apply(cmp);
		}
		else for(int i=1;i<4;i++) //MRT
		{
			IDirect3DSurface9 *rt = 0;
			IDirect3DTexture9* &t = p->query9->mrtargets[i-1];
			if(t&&!t->GetSurfaceLevel(0,&rt)&&d->SetRenderTarget(i,rt))
			{
				t->Release(); t = 0; assert(0); 
			}
			if(rt) rt->Release(); 
			d->SetRenderState((D3DRENDERSTATETYPE)((int)D3DRS_COLORWRITEENABLE1+i-1),t?0xF:0);
		}

		for(int i=DDRAW::doWhite;i>=0;i--)
		{
			auto &bw = i?p->query9->white:p->query9->black;
			if(X)
			{
				glGenTextures(1,&(GLuint&)bw);				
				glBindTexture(GL_TEXTURE_2D,(GLuint)bw);
				DWORD px = i?0xFFFFFFFF:0xFF000000;
				glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,&px);
			}
			else if(!d->CreateTexture(1,1,1,D3DUSAGE_DYNAMIC,D3DFMT_L8,D3DPOOL_DEFAULT,&bw,0))
			{
				D3DLOCKED_RECT lock; 			
				if(bw->LockRect(0,&lock,0,0)==D3D_OK)
				{
					*(BYTE*)lock.pBits = i?255:0; bw->UnlockRect(0);
				}
				else //PARANOID??? 
				{
					bw->Release(); bw = 0; assert(0);
				}
			}
			assert(bw); //2021
		}

		if(DDRAW::doDither) if(X)
		{
			glGenTextures(1,&p->queryGL->dither);
			glBindTexture(GL_TEXTURE_2D,p->queryGL->dither);
			glTexImage2D(GL_TEXTURE_2D,0,GL_R8,8,8,0,GL_RED,GL_UNSIGNED_BYTE,dx_d3d9c_dither65);
			//OpenGL gonna be OpenGL
			glActiveTexture(GL_TEXTURE0+DDRAW_TEX0);
			glBindTexture(GL_TEXTURE_2D,p->queryGL->black); //???
			glActiveTexture(GL_TEXTURE0+DDRAW_TEX1);
			glBindTexture(GL_TEXTURE_2D,p->queryGL->dither);
			glActiveTexture(GL_TEXTURE0);
		}
		else if(d->CreateTexture(8,8,1,D3DUSAGE_DYNAMIC,D3DFMT_L8,D3DPOOL_DEFAULT,&p->query9->dither,0)==D3D_OK)
		{	
			if(!dx_d3d9c_dither(p->query9->dither,65))
			{
				p->query9->dither->Release(); p->query9->dither = 0;
			}
			else if(1) //hack: assuming texture 2 unusued
			{
					//REMINDER: TEX0/TEX1 are really 1/2 (very misleading)

				//NOTE: black is the first MRT texture?!
				d->SetTexture(DDRAW_TEX0,p->query9->black); //???
				d->SetTexture(DDRAW_TEX1,p->query9->dither); 
			}
		}		

		D3DCAPS9 caps;
		d->GetDeviceCaps(&caps);
		{
			/*unset even for proxy9->GetDeviceCaps(adapter)???
			//2021: assuming D3DSAMP_SRGBTEXTURE present if so? 
			if(~caps.Caps3&D3DCAPS3_LINEAR_TO_SRGB_PRESENTATION)
			DDRAW::sRGB = 0;*/

			//using user mipmap routine if provided
			if(DDRAW::mipmap||~caps.Caps2&D3DCAPS2_CANAUTOGENMIPMAP
			||proxy9->CheckDeviceFormat(query9->adapter,dev,bpp,D3DUSAGE_DYNAMIC|D3DUSAGE_AUTOGENMIPMAP,D3DRTYPE_TEXTURE,D3DFMT_A8R8G8B8)!=D3D_OK)
			dx_d3d9c_autogen_mipmaps = false;

			if(X)
			{
				//trust D3D/OpenGL drivers match?
				int test = 0;
				glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT,&test);
				assert(test==caps.MaxAnisotropy);
				if(test) caps.MaxAnisotropy = test;
			}
			dx_d3d9c_anisotropy_max = caps.MaxAnisotropy;

			//2021: saving caps simplifies dx.d3d9X.cpp
			dx_d3d9c_d3ddrivercaps(caps,p->queryX->caps);
			DWORD zcap = 0; switch(dsbpp)
			{
			case D3DFMT_D16: zcap = DDBD_16; break;
			case D3DFMT_D24X8: zcap = DDBD_24; break;
			case D3DFMT_D32: zcap = DDBD_32; break;
			}
			p->queryX->caps.dwDeviceZBufferBitDepth = zcap;
			/*SOM (449530) expects 16bpp formats... it passes
			//all-zeroes descriptors to CreateSurface otherwise
			p->queryX->texture_format_mask = 1|2; //RGB/RGBA*/
			p->queryX->texture_format_mask = 0xf;
		}
										
		DDRAW::lights = 0; //??? 
		//DDRAW::refresh_state_dependent_shader_constants(); //too soon?	

		//EXT_sRGB_write_control
		if(X) glDisable(0x8DB9); //FRAMEBUFFER_SRGB_EXT

		if(DDRAW::sRGB) if(X)
		{
			//NOTE: I think textures have to use GL_SRGB8_ALPHA8
			//to get filtering to convert to linear space and back
			//
			//that's all DDRAW::sRGB is doing so far. I think from
			//experience this is like D3DPRESENT_LINEAR_CONTENT but
			//I read it may enable sRGB blending, which is desirable
			//I think D3D9 does blending automatically if available
			//(the last paragraph below--GL_EXT_texture_sRGB_decode--
			//suggests blending matches the textures but this may be
			//an alternative mode using "DECODE_EXT")
			//
			/* "When rendering into a surface in Direct3D 9 with the
				D3DRS_SRGBWRITEENABLE render state (set by SetRenderState) set to false,
				the pixel updates (including blending) need to operate with
				GL_FRAMEBUFFER_SRGB disabled.  So:

				  glDisable(GL_FRAMEBUFFER_SRGB);

				Likewise when the D3DRS_SRGBWRITEENABLE render state is true,
				OpenGL should operate

				  glEnable(GL_FRAMEBUFFER_SRGB);				  
				 
				Any texture with an sRGB internal format (for example,
				GL_SRGB8_ALPHA8 for the internal format) will perform sRGB decode
				before blending and encode after blending.  This matches the Direct3D9
				semantics when D3DUSAGE_QUERY_SRGBWRITE is true of the resource format."
			*/
			//glEnable(GL_FRAMEBUFFER_SRGB);
		}
		else
		{
			//going to assume this works if shaders are
			//working? could set DDRAW::sRGB to 0 if it
			//weren't already too late to reset shaders
			d->SetSamplerState(0,D3DSAMP_SRGBTEXTURE,1);
			if(p->query9->effects[1])
			d->SetSamplerState(1,D3DSAMP_SRGBTEXTURE,1);
		}

		out = D3D_OK;
	}
	else DDRAW_LEVEL(7) << "CreateDevice FAILED\n"; 

	DDRAW::mrtargets9[0] = d?fxbpp:D3DFMT_UNKNOWN;

	*z = DDRAW::Direct3DDevice7 = p; 
	
	//funny, software creates the shaders but 
	//then has other probs later. Should investigate
	if(p) DDRAW::vshaders9_pshaders9_require_reevaluation();
	if(p) DDRAW::refresh_state_dependent_shader_constants();

	//setup scene or MSAA render target
	dx_d3d9X_enableeffectsshader(DDRAW::doEffects); 
	
	if(!X)
	{
		//2018: PlayStation VR
		if(DDRAW::inStereo) DDRAW::stereo_update_SetStreamSource();
	}

	dx_d3d9c_pshaders_noclip = dx_d3d9c_pshaders;

	if(d12) //DDRAW::D3D12Device?
	{
		assert(0);
		/*FOR SOME REASON I THOUGHT THIS WOULD WORK :(
		auto &h12 = DDRAW::doD3D12_backbuffers;
		for(int i=0;i<DX_ARRAYSIZEOF(h12)-1;i++) if(!out)			 
		{
			if(h12[i].nt_handle)
			{
				HRESULT test = 
				d->CreateRenderTargetEx(w,h,fxbpp,D3DMULTISAMPLE_NONE,0,0,&h12[i].d3d9,&h12[i].nt_handle);
				assert(h12[i].d3d9);
			}
		}
		else if(h12[i].nt_handle)
		{
			CloseHandle(h12[i].nt_handle);
		}
		if(out)
		{
			d12->Release(); DDRAW::D3D12Device = 0;
		}*/
	}

	DDRAW_POP_HACK(DIRECT3D7_CREATEDEVICE,IDirect3D7*,
	const GUID*&,DX::LPDIRECTDRAWSURFACE7&,DX::LPDIRECT3DDEVICE7*&)(&out,this,x,y,z);

	DDRAW_RETURN(out) 
}
static void dx_d3d9X_release_shaders(bool X)
{
	//NOTE: I don't know if these depend on the shaders
	if(X) for(size_t i=dx_d3d9X_pipelines.size();i-->0;)
	{
		GLuint x = (GLuint)(dx_d3d9X_pipelines[i]>>32);
		glDeleteProgramPipelines(1,&x);
	}
	dx_d3d9X_pipelines.clear(); dx_d3d9X_pipeline = 0;

	for(int i=0;i<DDRAW::fx;i++) //NOCLIP?
	{
		if(auto&ea=dx_d3d9c_pshaders2[i]) //dx_d3d9X_pshaders
		{
			if(ea!=dx_d3d9c_pshaders[i])
			if(X)
			if(dx_d3d9X_shaders_are_programs)
			glDeleteProgram((GLuint)ea);
			else glDeleteShader((GLuint)ea);
			else ea->Release(); ea = 0;
		}
	}
	for(int i=DX_ARRAYSIZEOF(dx_d3d9c_vshaders);i-->0;)
	{
		if(auto&ea=dx_d3d9c_vshaders[i]) //dx_d3d9X_vshaders
		{
			if(X) 
			if(dx_d3d9X_shaders_are_programs)
			glDeleteProgram((GLuint)ea);
			else glDeleteShader((GLuint)ea);
			else ea->Release(); ea = 0;
		}
		if(auto&ea=dx_d3d9c_pshaders[i]) //dx_d3d9X_pshaders
		{
			if(X)
			if(dx_d3d9X_shaders_are_programs)
			glDeleteProgram((GLuint)ea);
			else glDeleteShader((GLuint)ea);
			else ea->Release(); ea = 0;
		}
	}
}
/*REMOVE ME?
//I think shaders can manually specify this
static void dx_d3d9X_uniforms(int u, int p)
{
	if(1){ assert(0); return; } //REMOVE ME? //WIP

	//this is a rare instance where OpenGL makes it impossible to
	//easily coordinate multiple software components on a context
	int binding = u*3+DDRAW::compat_glUniformBlockBinding_1st_of_6;

	int i[3] = 
	{
		glGetUniformBlockIndex(p,u?"psB":"vsB"),
		glGetUniformBlockIndex(p,u?"psI":"vsI"),
		glGetUniformBlockIndex(p,u?"psF":"vsF"),
	};
	for(int j=3;j-->0;) if(i[j]!=~0u) //GL_INVALID_INDEX
	{
		glUniformBlockBinding(p,i[j],binding+j);
	}
}*/
static bool dx_d3d9X_program_db(int s, const char **code)
{
	enum{ sap=dx_d3d9X_shaders_are_programs };

	int i = 0; //GL_COMPILE_STATUS/GL_LINK_STATUS
	(sap?glGetProgramiv:glGetShaderiv)(s,0x8B81+sap,&i); 
	if(i) return true; //GL_INFO_LOG_LENGTH
	(sap?glGetProgramiv:glGetShaderiv)(s,0x8B84,&i);	
	char *l = new char[i+1];
	(sap?glGetProgramInfoLog:glGetShaderInfoLog)(s,i+1,&i,l);

	//Ex_shader_hlsl
	{
		const char *debug = l;
		const char *debug2[32]; 
		for(int i=0;i<DX_ARRAYSIZEOF(debug2)&&debug;i++) 
		{
			const char *n = strchr(debug,'\n');
			debug2[i] = debug;
			if(debug2[i]>n) debug2[i] = debug;
			debug = n?n+1:0;
		}

		int i = 1;
		const char *code2[256]; 
		code2[0] = "";
		for(;*code&&i<sizeof(code2);code++)
		{
			auto p = *code;
			code2[i++] = p;
			while((p=strchr(p,'\n'))&&p[1])
			{
				code2[i] = ++p; 
				
				if(++i==DX_ARRAYSIZEOF(code2)) break;
			}
		}

		//REMINDER: to break into ANGLE's compile it's 
		//necessary (for some reason) to put the break
		//point at TranslateTaskGL::operator() or else
		//in ShaderLang.cpp sh::Compile. the functions
		//don't kick until after that even though they
		//are called "compile"

		#ifdef NDEBUG
		//#error uncomment me
		#endif
		assert(0);
		i = i; //breakpoint
	}

	delete[] l; return false;
}
extern void DDRAW::vshaders9_pshaders9_require_reevaluation() 
{	
	const bool X = DDRAW::gl;

	dx_d3d9X_release_shaders(X);

	assert(!X||DDRAW::pshadersGL_include);

	bool noclip = X&&!strstr(DDRAW::pshadersGL_include,"TEXTURE0_NOCLIP");

	auto v = DDRAW::target=='dx95'?
	"#version 310 es\n"
	//D3D11 (ANGLE) rejects IO blocks :(
	//https://bugs.chromium.org/p/angleproject/issues/detail?id=6272
//	"#extension GL_EXT_shader_io_blocks : enable\n"
//	"#extension GL_ARB_fragment_coord_conventions : enable\n"
	//supposedly mobile is supposed to use mediump but OpenGL
	//is really stupid here because it requires the interface
	//variables to have the same precision, and vertex shader
	//precision defaults to highp (OpenGL people are sadists)
	"precision highp float;\n"	
		//NOTE: GLSL_NV_shading_rate_image wants 450 (NV_shading_rate_image)
		:"#version 450\n";
	char si[] = "#define SHADER_INDEX 0\n\0";
	const char *vsGL[4+2] = {v,si,DDRAW::vshadersGL_include,0};
	const char *psGL[5+1] = {v,si,DDRAW::pshadersGL_include,0,0};
	if(!vsGL[2]) vsGL[2] = "";
	if(!psGL[2]) psGL[2] = "";
	if(auto*p=DDRAW::Direct3DDevice7)
	for(int i=0;i<DX_ARRAYSIZEOF(dx_d3d9c_vshaders);i++) 
	{
		if(X) for(int j=0;j<3;j++) //2
		{
			auto *nc = j==2?dx_d3d9X_pshaders2:dx_d3d9X_pshaders;
			auto &s = (j?nc:dx_d3d9X_vshaders)[i];
			auto &sGL = j?psGL:vsGL;

			sGL[3] = (j?DDRAW::pshadersGL:DDRAW::vshadersGL)[i];
			if(!sGL[3]) continue;

			if(j==2) //MESSY
			{
				if(noclip&&!strstr(sGL[3],"TEXTURE0_NOCLIP"))
				{
					s = dx_d3d9X_pshaders[i]; continue; 
				}

				sGL[4] = sGL[3];
				sGL[3] = sGL[2];
				sGL[2] = "#define TEXTURE0_NOCLIP \n";
			}
			
			if(!dx_d3d9X_shaders_are_programs)
			{
				s = glCreateShader(GL_FRAGMENT_SHADER+!j);
				glShaderSource(s,j==2?5:4,sGL,0);
				glCompileShader(s);
			}
			else s = glCreateShaderProgramv(GL_FRAGMENT_SHADER+!j,j==2?5:4,sGL);

			if(DX::debug&&dx_d3d9X_program_db(s,sGL))
			{
				//dx_d3d9X_uniforms(0,s); //REMOVE ME?
			}

			if(j==2) for(int i=2;i<=4;i++)
			{
				sGL[i] = sGL[i+1];
			}
		}
		else
		{
			if(auto*ea=DDRAW::vshaders9[i])
			p->proxy9->CreateVertexShader(ea,&dx_d3d9c_vshaders[i]);
			if(auto*ea=DDRAW::pshaders9[i])
			p->proxy9->CreatePixelShader(ea,&dx_d3d9c_pshaders[i]);
			//TODO: need to set this up sometime
			//if(auto*ea=DDRAW::pshaders9_noclip[i])
			//p->proxy9->CreatePixelShader(ea,&dx_d3d9c_pshaders2[i]);
		}

		if(i>=9) if(i==9) //sprintf(si+21,"%d\n",i);
		{
			si[21] = '1'; si[22] = '0'; si[23] = '\n';
		}
		else si[22]++; else si[21]++;
	}
}
extern void DDRAW::fx9_effects_buffers_require_reevaluation() 
{
	//2021: CreateDevice hadn't called this subroutine to set
	//this up origianlly, so it's a little tortured, but with
	//the addition OpenGL logic it doesn't make sense to have
	//this code duplicated by CreateDevice

	//auto fxbpp = dx_d3d9c_rendertarget_format; 	
	auto fxbpp = DDRAW::mrtargets9[0];

	const bool X = DDRAW::gl;

	if(DDRAW::shader||DDRAW::compat!='dx9c')
	{
		assert(0); return;
	}
	
	auto p = DDRAW::Direct3DDevice7; assert(p);	
	auto d = p->proxy9;

	UINT ww,hh; if(DDRAW::xr)
	{
		ww = 0; hh = 0;

		auto &sv = p->queryGL->stereo_views;
		for(int i=0;i<_countof(sv);i++)
		{		
			//QUAD isn't necessarily the same size
			//really "Varjo" is the only quad mode
			assert(i<2);

			auto &v = sv[i];
			auto *s = (Widgets95::xr::section*)v.src;
			if(!s) break;

			assert(v.src==s); //v.src = s;

			//NOTE: stereo_toggle_OpenXR shouldn't do
			//this since doSuperSampling may change in
			//the interim
		//	DDRAW::doSuperSamplingMul(*(RECT*)
		//	memcpy(&v.x,s->virtual_off,4*sizeof(int)));
			assert(!DDRAW::doSuperSampling);
			memcpy(&v.x,s->virtual_off,4*sizeof(int));
				
			//Pimax sets may exceed the max texture
			//size if not stacked. Note, the only real
			//reason to not stack is if xdiscarded draws
			//to the monitor in debug mode
			p->queryGL->stereo_ultrawide =
			1.2f<(float)s->virtual_dim[0]/s->virtual_dim[1];
			if(DX::debug?p->queryGL->stereo_ultrawide:true)
			{
				if(i&1) v.y+=sv[i-1].h; //HACK?
				if(i>1) v.x+=sv[i-2].w; //QUAD?
			}
			else
			{
				if(i&1) v.x+=sv[i-1].w; //HACK?
				if(i>1) v.y+=sv[i-2].h; //QUAD?		
			}

			ww = max((int)ww,v.x+v.w);
			hh = max((int)hh,v.y+v.h);
		}
	}
	else
	{
		ww = DDRAW::PrimarySurface7->queryX->width;
		hh = DDRAW::PrimarySurface7->queryX->height;

		DDRAW::doSuperSamplingMul(ww,hh);
	}
	assert(ww%2==0&&hh%2==0); //mipmaps?

	p->queryX->effects_w = ww;
	p->queryX->effects_h = hh;
		
	//Z?
	if(!DDRAW::DepthStencilSurface7
	 ||!DDRAW::DepthStencilSurface7->query9->format)
	{
		assert(0);
	}
	else if(X) //TODO: return on same size?
	{
		//GL_DEPTH_COMPONENT and 
		//GL_DEPTH_COMPONENT32F are ambiguous 
		//#define GL_DEPTH_COMPONENT                0x1902
		//#define GL_DEPTH_COMPONENT16              0x81A5
		//#define GL_DEPTH_COMPONENT32F             0x8CAC
		//#define GL_DEPTH32F_STENCIL8              0x8CAD
		//#define GL_DEPTH_COMPONENT24              0x81A6
		//#define GL_UNSIGNED_INT_24_8              0x84FA
		//#define GL_DEPTH24_STENCIL8               0x88F0
		int dfmt; switch(DDRAW::DepthStencilSurface7->query9->format)
		{
		case D3DFMT_D16: dfmt = 0x81A5; break; //COMPONENT16
		case D3DFMT_D32: assert(0);
		case D3DFMT_D24X8: dfmt = 0x81A6; break; //COMPONENT24
		}
		auto &fb =*p->queryGL->framebuffers;
		auto &ds = p->queryGL->depthstencil;
		if(!fb) glGenFramebuffers(2,&fb); //1
		if(!ds) glGenRenderbuffers(1,&ds);
		glBindFramebuffer(GL_FRAMEBUFFER,fb);
		glBindRenderbuffer(GL_RENDERBUFFER,ds);
		glRenderbufferStorage(GL_RENDERBUFFER,dfmt,ww,hh); //GL_DEPTH_COMPONENT
		//glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_RENDERBUFFER,ds);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,ds);
	}
	else if(auto&ds=p->query9->depthstencil)
	{	
		D3DSURFACE_DESC desc;
		if(ds->GetDesc(&desc)||desc.Width!=ww||desc.Height!=ww)
		{
			ds->Release(); ds = 0; goto ds;
		}
		else return; //!!! //NOTE: ASSUMING NO CHANGE REQUIRED!
	}
	else ds:
	{
		D3DSURFACE_DESC desc = {}; //dx.d3d9c.cpp?
		if(p->query9->multisample)
		p->query9->multisample->GetDesc(&desc); //YUCK

		if(!d->CreateDepthStencilSurface(ww,hh,
		DDRAW::DepthStencilSurface7->query9->format,
		desc.MultiSampleType,desc.MultiSampleQuality,true,&ds,0))
		{
			d->SetDepthStencilSurface(ds);
		}
		else assert(0);
	}

	bool CreateDevice = !p->query9->effects[0]; //HACK!

	//FX?
	DDRAW::fx2ndSceneBuffer = 0;
	if(fxbpp)
	for(int i=0;i<2;i++) if(X) 
	{
		auto *x = p->queryGL->xr;
		auto &fx = *(Widgets95::xr::effects*)p->queryGL->effects;
		if(auto&t=fx.gl_bind_texture[i])
		{
			//NOTE: this may fail if t is leftover from non-stereo
			//drawing, which it typically is
			if(!x||!fx.reset_texture(x->first_surface(),i,nullptr))
			{
				glDeleteTextures(1,(UINT*)&t); t = 0; 
			}
		}
	}
	else if(auto&fx=p->query9->effects[i])
	{
		fx->Release(); fx = 0;
	}
	if(fxbpp)
	for(int i=1+DDRAW::do2ndSceneBuffer;i-->0;) if(DDRAW::xr) x:
	{
		auto &fx = *(Widgets95::xr::effects*)p->queryGL->effects;
		int dims[3] = {ww,hh,1+DDRAW::doMipSceneBuffers};
		fx.reset_texture(p->queryGL->xr->first_surface(),i,dims);
		assert(fx.gl_bind_texture[i]);
	}
	else if(X&&DDRAW::WGL_NV_DX_interop2)
	{
		goto x; //EXPERIMENTAL D3D11 (WGL_NV_DX_interop2) path?
	}
	else if(X)
	{		
		auto &fx = p->queryGL->effects[i];

		//see notes above, why this can't be satified under OpenGL terms
		//or sRGB decoding purposes :(
		//assert(fxbpp!=D3DFMT_A2R10G10B10);
		assert(fxbpp==D3DFMT_X8R8G8B8);

		//ANGLE refuses to draw to the textures
		//I guess OpenGL just doesn't have an "XRGB" concept
		//int sfmt = DDRAW::sRGB?GL_SRGB8:GL_RGB8;
		int sfmt = DDRAW::sRGB?GL_SRGB8_ALPHA8:GL_RGBA8; //32bpp
		
		glGenTextures(1,&fx);
		glBindTexture(GL_TEXTURE_2D,fx);
		int mipmaps = DDRAW::doMipSceneBuffers;
		glTexStorage2D(GL_TEXTURE_2D,1+mipmaps,sfmt,ww,hh);
		//mipmaps are black in shader?
		//for(int i=1,j=2;i<mipmaps;i++,j*=2)
		//glTexSubImage2D(GL_TEXTURE_2D,i,0,0,ww/j,hh/j,GL_RGBA,GL_UNSIGNED_BYTE,0);
		//glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		auto &fx = p->query9->effects[i];
		d->CreateTexture(ww,hh,DDRAW::doMipSceneBuffers+1, 
		D3DUSAGE_RENDERTARGET,fxbpp,D3DPOOL_DEFAULT,&fx,0);
	//	dx_d3d9c_rendertarget_pitch = ww*dx_d3d9c_format(fxbpp).dwRGBBitCount/8;	
	}
	if(!fxbpp)
	{
		assert(fxbpp); //NOTE: DDRAW::xr requires fxbpp, but is it always nonzero?
	}
	else if(X)
	{
		glFramebufferTexture(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,p->queryGL->effects[0],0);
		/*//GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING
		int test;
		if(DX::debug)
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,0x8210,&test);
		test = test; //breakpoint*/
	}
	else if(!p->query9->multisample)
	{
		auto &fx = p->query9->effects[0];

		//this freezes up on toggle, I don't get it
		//(2021: CreateDevice needs this to happen)
		if(!CreateDevice)
		{
			d->SetRenderTarget(0,0);
		}
		else if(fx)
		{
			IDirect3DSurface9 *rt = 0;
			if(!fx->GetSurfaceLevel(0,&rt))
			assert(rt);
			d->SetRenderTarget(0,rt); if(rt) rt->Release();
		}
		else assert(fx);
	}

	//MRT?
	if(fxbpp) //D3DDEVTYPE_HAL?
	for(int i=1;i<4;i++) if(int fmt=DDRAW::mrtargets9[i]) if(X)
	{
		for(int j=2;j-->0;)
		if(DDRAW::DirectDraw7->proxy9->CheckDeviceFormat
		(DDRAW::DirectDraw7->queryX->adapter,D3DDEVTYPE_HAL,
		D3DFMT_X8R8G8B8,D3DUSAGE_RENDERTARGET,D3DRTYPE_TEXTURE,(D3DFORMAT)fmt))
		fmt = j?DDRAW::altargets9[i]:D3DFMT_UNKNOWN;
		else break;
		switch(fmt)
		{
		case D3DFMT_R16F: fmt = 0x822D; break; //GL_R16F
		case D3DFMT_R32F: fmt = 0x822E; break; //GL_R32F
		default: assert(0); continue;
		}

		auto &mrt = p->queryGL->mrtargets[i-1];
		if(mrt) 
		glDeleteTextures(1,&mrt); mrt = 0; //NOTE: glTexStorage requires this	
		glGenTextures(1,&mrt);
		glBindTexture(GL_TEXTURE_2D,mrt);
		glTexStorage2D(GL_TEXTURE_2D,1,fmt,ww,hh);

		glFramebufferTexture(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0+i,mrt,0);
	}
	else if(DDRAW::mrtargets9[i])
	{
		auto &mrt = p->query9->mrtargets[i-1];

		bool CreateDevice = !mrt;

		if(mrt) mrt->Release(); mrt = 0;

		if(d->CreateTexture(ww,hh,1,D3DUSAGE_RENDERTARGET,DDRAW::mrtargets9[i],D3DPOOL_DEFAULT,&mrt,0))
		if(DDRAW::altargets9[i]) //fall back?
		d->CreateTexture(ww,hh,1,D3DUSAGE_RENDERTARGET,DDRAW::altargets9[i],D3DPOOL_DEFAULT,&mrt,0);
		
		if(!CreateDevice)
		{
			DWORD st = 0xF;
			d->GetRenderState((D3DRENDERSTATETYPE)((int)D3DRS_COLORWRITEENABLE1+i-1),&st);
			if(st)
			{
				IDirect3DSurface9 *rt = 0;
				if(!mrt||mrt->GetSurfaceLevel(0,&rt))
				assert(rt);
				d->SetRenderTarget(i,rt); if(rt) rt->Release(); 
			}
			else d->SetTexture(DDRAW_TEX0+i-1,mrt);
		}
	}	
}
extern void dx_d3d9X_enableeffectsshader(bool enable) //dx.d3d9c.cpp
{	
	//FIX ME
	//I guess this is counted on to turn on effects :(
	//if(!DX::debug||!DDRAW::shader) return; //2021

	assert(DDRAW::Direct3DDevice7);

	auto fx = DDRAW::Direct3DDevice7->query9->effects[0];

	if(!fx||dx_d3d9c_effectsshader_enable==enable) return; 
	
	auto d3dd9 = DDRAW::Direct3DDevice7->proxy9;

	if(DDRAW::gl) //REMINDER: this feature is debug only
	{
		//DICEY
		/*can't work since MRT can't be bound to the back-buffer
		//NEED DEPTH BUFFER FOR THIS?		
		glBindFramebuffer(GL_FRAMEBUFFER,
		enable?*DDRAW::Direct3DDevice7->queryGL->framebuffers:0);*/
		if(!enable) return;		
	}
	else if(enable)
	{
		auto ms = DDRAW::Direct3DDevice7->query9->multisample; //dx.d3d9c.cpp?
		auto rt = ms;
		if(rt||fx->GetSurfaceLevel(0,&rt)==D3D_OK)
		{
			if(DDRAW::inScene) d3dd9->EndScene();
			if(DDRAW::inScene) d3dd9->BeginScene();

			D3DVIEWPORT9 vp; d3dd9->GetViewport(&vp);
			
			if(d3dd9->SetRenderTarget(0,rt)!=D3D_OK)
			{
				assert(0); 
				
				if(DDRAW::shader) dx_d3d9c_discardeffects(); //dx.d3d9c.cpp?
			}
			else if(rt!=ms) //dx.d3d9c.cpp?
			{
				rt->Release();
			}

			d3dd9->SetViewport(&vp);
		}			
	}
	else //disable
	{
		if(auto*bbuffer=dx_d3d9X_backbuffer9())
		{
			if(DDRAW::inScene) d3dd9->EndScene();
			if(DDRAW::inScene) d3dd9->BeginScene();
			D3DVIEWPORT9 vp; d3dd9->GetViewport(&vp);
			d3dd9->SetRenderTarget(0,bbuffer); 
			d3dd9->SetViewport(&vp); bbuffer->Release();
		}			
	}

	dx_d3d9c_effectsshader_enable = enable;
	dx_d3d9c_effectsshader_toggle = enable;
}
template<int X>
HRESULT D3D9X::IDirect3D7<X>::CreateVertexBuffer(DX::LPD3DVERTEXBUFFERDESC x, DX::LPDIRECT3DVERTEXBUFFER7 *y, DWORD z)
{
	DDRAW_LEVEL(7) << "D3D9X::IDirect3D7::CreateVertexBuffer()\n";

	//DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK) 
	
	DDRAW_PUSH_HACK(DIRECT3D7_CREATEVERTEXBUFFER,IDirect3D7*,
	DX::LPD3DVERTEXBUFFERDESC&,DX::LPDIRECT3DVERTEXBUFFER7*&,DWORD&)(0,this,x,y,z);						  
	
	if(!x||!y) DDRAW_FINISH(D3D_OK) //short-circuit? //!D3D_OK //2022

	if(z!=0) DDRAW_LEVEL(7) << "PANIC! last argument non-zero\n";

	if(!DDRAW::Direct3DDevice7) //???
	{
		DDRAW_FINISH(!D3D_OK) //todo: if necessary save buffer contents for later
	}

	UINT sizeofFVF = dx_d3d9c_sizeofFVF(x->dwFVF);

	UINT mem = sizeofFVF*x->dwNumVertices;	

	GLuint vbo = 0, vao = 0;

	IDirect3DVertexBuffer9 *b = 0;	

	auto fvf = x->dwFVF; //TESTING

	if(X)
	{
		if(D3DFVF_XYZRHW==(fvf&D3DFVF_POSITION_MASK)) //OPTIMIZING?
		{
			fvf&=~D3DFVF_XYZRHW; fvf|=D3DFVF_XYZW;
		}
		if(vao=dx_d3d9X_vao(fvf,sizeofFVF))
		{
			glGenBuffers(1,&vbo);
			glBindBuffer(GL_ARRAY_BUFFER,vbo);
			glBufferData(GL_ARRAY_BUFFER,mem,0,GL_STREAM_DRAW); //GL_DYNAMIC_DRAW?
			out = !vbo;
		}
		else assert(0);
	}
	else
	{
		DDRAW::IDirect3DDevice7 *d3dd = DDRAW::Direct3DDevice7;
	
		D3DPOOL pool = D3DPOOL_DEFAULT; 	
		if(x->dwCaps&D3DVBCAPS_SYSTEMMEMORY){ assert(0); pool = D3DPOOL_SYSTEMMEM; }
	
		//required updating buffer (more than once)
		DWORD usage = D3DUSAGE_DYNAMIC; 
		//d3d9d.dll: Direct3D9: (WARN) :
		//Vertexbuffer created with POOL_DEFAULT but WRITEONLY not set. Performance penalty could be severe.
		if(pool==D3DPOOL_DEFAULT) usage|=D3DUSAGE_WRITEONLY;
		if(x->dwCaps&D3DVBCAPS_WRITEONLY) usage|=D3DUSAGE_WRITEONLY;
		if(x->dwCaps&D3DVBCAPS_DONOTCLIP) usage|=D3DUSAGE_DONOTCLIP;	

		//NOTE: mem is used to get the size from the descriptor, but unfilled
		out = d3dd->proxy9->CreateVertexBuffer(mem,usage,x->dwFVF,pool,&b,0);
	}

	if(out==D3D_OK)
	{					   
		auto p = new DDRAW::IDirect3DVertexBuffer7(DDRAW::target);

		if(X)
		{
			p->queryX->vaoGL = vao;
			p->queryX->vboGL = vbo;
		}
		else p->proxy9 = b;

		p->queryX->FVF = fvf; //x->dwFVF; 
		p->queryX->sizeofFVF = sizeofFVF;
		p->queryX->mixed_mode_lock = mem;
		p->queryX->mixed_mode_size = mem;

		*y = (DX::LPDIRECT3DVERTEXBUFFER7)p;
	}
	
	DDRAW_POP_HACK(DIRECT3D7_CREATEVERTEXBUFFER,IDirect3D7*,
	DX::LPD3DVERTEXBUFFERDESC&,DX::LPDIRECT3DVERTEXBUFFER7*&,DWORD&)(&out,this,x,y,z);

	DDRAW_RETURN(out)
}






template<int X>
HRESULT D3D9X::IDirect3DDevice7<X>::Clear(DWORD x, DX::LPD3DRECT y, DWORD z, DX::D3DCOLOR w, DX::D3DVALUE q, DWORD r)
{
	DDRAW_LEVEL(7) << "D3D9X::IDirect3DDevice7::Clear()\n";
 
	//DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK)

	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_CLEAR,IDirect3DDevice7*,
	DWORD&,DX::LPD3DRECT&,DWORD&,DX::D3DCOLOR&,DX::D3DVALUE&,DWORD&)(0,this,x,y,z,w,q,r);
						  
	if(z==0) DDRAW_FINISH(DD_OK) //!DD_OK

	int fb,hh,zGL; float rgba[4]; if(X)
	{
		fb = 0; glGetIntegerv(0x8CA6,&fb); //GL_DRAW_FRAMEBUFFER_BINDING
				
		//MAY NEED A SUBROUTINE FOR THIS
		hh = DDRAW::PrimarySurface7->queryX->height;			
			
		if(!fb) //HACK: workshop.cpp (ItemEdit, etc.)
		{
			RECT cr;
			if(DDRAW::window_rect(&cr)) hh = cr.bottom;
			else assert(DDRAW::window);
		}
		else DDRAW::doSuperSamplingMul(hh);
			
		zGL = 0; if(z&D3DCLEAR_TARGET)
		{
			zGL|=0x4000; //GL_COLOR_BUFFER_BIT
 
			rgba[0] = (w>>16&0xff)*0.003921568627f; //1/255
			rgba[1] = (w>>8&0xff)*0.003921568627f;
			rgba[2] = (w>>0&0xff)*0.003921568627f;
			rgba[3] = 1;//(w>>24&0xff)*0.003921568627f;
			glClearColor(rgba[0],rgba[1],rgba[2],rgba[3]);
		}
		if(z&D3DCLEAR_ZBUFFER)
		{
			zGL|=0x100; glClearDepthf(q); //GL_DEPTH_BUFFER_BIT
		}
		if(z&D3DCLEAR_STENCIL)
		{
			zGL|=0x400; glClearStencil(r); //GL_STENCIL_BUFFER_BIT
		}		
										
		//Direct3D7 doesn't have a scissor (D3D9 does)
		//glGetIntegerv(GL_SCISSOR_BOX,sc);
		//glGetIntegerv(GL_SCISSOR_TEST,sc+4);
		if(!DDRAW::xr) //HACK
		glEnable(GL_SCISSOR_TEST); else assert(!y);
	}

	D3DRECT buf[4]; auto buf_s = _countof(buf);
	for(DWORD i=0,iN=max(x,1);i<iN;i+=buf_s)
	{
		D3DRECT *src;
		if(y&&(!DDRAW::doNothing||DDRAW::doSuperSampling))
		{
			for(DWORD j=0;i+j<iN&&j<buf_s;j++)
			{
				auto &temp = buf[j]; 
				
				temp = ((D3DRECT*)y)[i+j];

				if(DDRAW::doNothing) goto ss;
				
				if(DDRAW::doScaling)
				{
					temp.x1*=DDRAW::xyScaling[0]; temp.x2*=DDRAW::xyScaling[0]; 
					temp.y1*=DDRAW::xyScaling[1]; temp.y2*=DDRAW::xyScaling[1]; 
				}
				if(DDRAW::doMapping)
				{
					temp.x1+=DDRAW::xyMapping[0]; temp.y1+=DDRAW::xyMapping[1]; 
					temp.x2+=DDRAW::xyMapping[0]; temp.y2+=DDRAW::xyMapping[1]; 		
				}

				ss: DDRAW::doSuperSamplingMul(temp.x1,temp.x2,temp.y1,temp.y2);
			}
			src = buf;
		}
		else src = (D3DRECT*)y+i;

		auto yy = y?src:0;

		int xx = y?min(iN-i,buf_s):1;

		for(int k=xx;k-->0;yy+=X?1:buf_s)
		{
			if(!X) k-=buf_s-1; else if(y)
			{
				glScissor(yy->x1,hh-yy->y2,yy->x2-yy->x1,yy->y2-yy->y1);
			}
			else if(!DDRAW::xr) //2022: emulate D3D
			{
				int x1 = dx_d3d9X_gl_viewport[0];
				int x2 = dx_d3d9X_gl_viewport[2]+x1;
				int y1 = dx_d3d9X_gl_viewport[1];
				int y2 = dx_d3d9X_gl_viewport[3]+y1;
				glScissor(x1,hh-y2,x2-x1,y2-y1);
			}

			if(~z&D3DCLEAR_TARGET||!DDRAW::doClearMRT||X&&!fb)
			{
				if(X)
				{
					glClear(zGL); out = D3D_OK; //ERROR?
				}
				else
				{
					out = proxy9->Clear(xx,yy,z,w,q,r);
				}
			}
			else if(X) //if(2==DDRAW::doClearMRT) //2021
			{
				auto cwe = queryGL->state.colorwriteenable;
		
				//NOTE: i is an index into the values passed
				//to glDrawBuffers. currently GL_NONE is set
				//for discarded buffers like SetRenderTarget
				//I don't know if there's a penalty for that
				for(int i=0;i<4;i++) 
				{
					//TESTING
					//this is the only thing I can get to work with OpenXR+D3D11
					//drawing triangles just doesn't take
				//	rgba[0] = DDRAW::noFlips%60/60.0f;

					if(!i||cwe&(1<<(i-1)))
					if(DDRAW::ClearMRT[i])
					{
						float rgba2[4] = {};
						sscanf(DDRAW::ClearMRT[i],"%f,%f,%f,%f",rgba2+0,rgba2+1,rgba2+2,rgba2+3);
						glClearBufferfv(0x1800,i,rgba2); //GL_COLOR
					}
					else glClearBufferfv(0x1800,i,rgba); //GL_COLOR
				}
				if(z&(D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL))
				{
					glClearBufferfi(0x84F9,0,q,r); //GL_DEPTH_STENCIL
				}

				out = D3D_OK; //ERROR?
			}
			else
			{
				//I had originally implemented this but
				//commented it out. I think it's safe. it
				//seems to do slightly faster supersampling

				//this is needed to repair the viewport after
				//SetRenderTarget... mainly MinZ/MaxZ are reset
				//to 0,1 and for some reason it broke my KF2 demo
				//on certain screen resolution settings only
				D3DVIEWPORT9 vp; proxy9->GetViewport(&vp);

				IDirect3DSurface9 *mrt[4] = {0,0,0,0}; 			
				for(int i=0;i<4;i++)
				{		
					proxy9->GetRenderTarget(i,&mrt[i]);
					if(mrt[i]&&i!=0)
					proxy9->SetRenderTarget(i,0);
				}
				assert(!DDRAW::ClearMRT[0]); //2021
				out = proxy9->Clear(xx,yy,z,w,q,r);
				for(int i=1;i<4;i++) if(mrt[i])
				{
					DWORD rgba; if(DDRAW::ClearMRT[i])
					{
						float r = 0, g = 0, b = 0, a = 0;
						sscanf(DDRAW::ClearMRT[i],"%f,%f,%f,%f",&r,&g,&b,&a);
						rgba = D3DCOLOR_COLORVALUE(r,g,b,a);
					}
					else rgba = w;
					proxy9->SetRenderTarget(0,mrt[i]);
					proxy9->Clear(xx,yy,D3DCLEAR_TARGET,rgba,0,0);
				}
				for(int i=0;i<4;i++) if(mrt[i])
				{
					proxy9->SetRenderTarget(i,mrt[i]); mrt[i]->Release();
				}

				proxy9->SetViewport(&vp);
			}
			/*else //draw fullscreen quad //dx_d3d9X_clearshader?
			{		
				//// see dx.d3d9c.cpp for removed algoritm ////
			}*/	
		}
	}

	//Direct3D7 doesn't have a scissor (D3D9 does)
	//glScissor(sc[0],sc[1],sc[2],sc[3]);
	if(X&&!DDRAW::xr) //HACK
	{
		glDisable(GL_SCISSOR_TEST);
	}

	DDRAW_POP_HACK(DIRECT3DDEVICE7_CLEAR,IDirect3DDevice7*,
	DWORD&,DX::LPD3DRECT&,DWORD&,DX::D3DCOLOR&,DX::D3DVALUE&,DWORD&)(&out,this,x,y,z,w,q,r);

	DDRAW_RETURN(out)
}
template<int X>
HRESULT D3D9X::IDirect3DDevice7<X>::GetViewport(DX::LPD3DVIEWPORT7 x)
{
	DDRAW_LEVEL(7) << "D3D9X::IDirect3DDevice7::GetViewport()\n";

	//DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK) 
	
	if(!x) DDRAW_RETURN(!D3D_OK)

	HRESULT out = dx_d3d9X_viewport(X,*(D3DVIEWPORT9*)x,this);
	
	if(x&&DDRAW::doSuperSampling) //EXPERIMENTAL
	{
		DDRAW::doSuperSamplingDiv(x->dwX,x->dwY,x->dwWidth,x->dwHeight);
	}
	if(x&&out==D3D_OK&&!DDRAW::doNothing)
	{
		//2020: this is actually rather complicated if scaling X/Y

		float s = DDRAW::doScaling?DDRAW::xyScaling[0]:1;
		float t = DDRAW::doScaling?DDRAW::xyScaling[1]:1;
		
		if(DDRAW::doMapping) 
		{
			x->dwX = (x->dwX-DDRAW::xyMapping[0])/s+0.5f;
			x->dwY = (x->dwY-DDRAW::xyMapping[1])/t+0.5f;
		}
		else if(DDRAW::doScaling) 
		{
			x->dwX = x->dwX*s+0.5f; x->dwY = x->dwY*t+0.5f;
		}

		if(DDRAW::doScaling) x->dwWidth = float(x->dwWidth)/s+0.5f;
		if(DDRAW::doScaling) x->dwHeight = float(x->dwHeight)/t+0.5f;
	}
		  
	DDRAW_RETURN(out) 
}
template<int X>
HRESULT D3D9X::IDirect3DDevice7<X>::SetViewport(DX::LPD3DVIEWPORT7 x)
{
	DDRAW_LEVEL(7) << "D3D9X::IDirect3DDevice7::SetViewport()\n";

	//DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK) 
	
	if(!x) DDRAW_RETURN(!D3D_OK)   
	
	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_SETVIEWPORT,IDirect3DDevice7*,
	DX::LPD3DVIEWPORT7&)(0,this,x);

	DX::D3DVIEWPORT7 temp;
	if(x&&(!DDRAW::doNothing||DDRAW::doSuperSampling)) 
	{
		if(DDRAW::doNothing){ temp = *x; goto ss; }

		//2020: this is actually rather complicated if scaling X/Y

		float s = DDRAW::doScaling?DDRAW::xyScaling[0]:1;
		float t = DDRAW::doScaling?DDRAW::xyScaling[1]:1;

		if(DDRAW::doMapping) 
		{
			temp.dwX = DDRAW::xyMapping[0]+x->dwX*s+0.5f;
			temp.dwY = DDRAW::xyMapping[1]+x->dwY*t+0.5f;
		}
		else
		{
			temp.dwX = x->dwX*s+0.5f; temp.dwY = x->dwY*t+0.5f;
		}

		if(DDRAW::doScaling) temp.dwWidth = s*float(x->dwWidth)+0.5f;
		if(DDRAW::doScaling) temp.dwHeight = t*float(x->dwHeight)+0.5f;
		
		temp.dvMinZ = x->dvMinZ; temp.dvMaxZ = x->dvMaxZ;
				
		ss: //EXPERIMENTAL
		{
			DDRAW::doSuperSamplingMul(temp.dwX,temp.dwY,temp.dwWidth,temp.dwHeight);
		}

		x = &temp; 
	}
	
	if(X)
	{
		//2022: OpenXR doesn't maintain SetViewport
		memcpy(dx_d3d9X_gl_viewport,&x->dwX,sizeof(int)*4);

		glViewport(x->dwX,x->dwY,x->dwWidth,x->dwHeight);
		glDepthRangef(x->dvMinZ,x->dvMaxZ);

		out = D3D_OK; //ERROR?
	}
	else out = proxy9->SetViewport((D3DVIEWPORT9*)x);
			   
	DDRAW_POP_HACK(DIRECT3DDEVICE7_SETVIEWPORT,IDirect3DDevice7*,
	DX::LPD3DVIEWPORT7&)(&out,this,x);

	DDRAW_RETURN(out)
}
template<int X>
HRESULT D3D9X::IDirect3DDevice7<X>::SetRenderState(DX::D3DRENDERSTATETYPE x, DWORD y)
{
	#define CASE_(x) break; case DX::D3DRENDERSTATE_##x:

	//these are meaningful to OpenGL	
	switch(X?x:0) //DUPLICATE
	{
	default: return D3D9C::IDirect3DDevice7::SetRenderState(x,y);
	CASE_(ZENABLE)
	CASE_(FILLMODE)
	CASE_(SHADEMODE)
	CASE_(ZWRITEENABLE)
	CASE_(SRCBLEND)
	CASE_(DESTBLEND)
	CASE_(CULLMODE)
	CASE_(ZFUNC)    
	CASE_(ALPHABLENDENABLE)
	case D3DRS_COLORWRITEENABLE:
	case D3DRS_COLORWRITEENABLE1:
	case D3DRS_COLORWRITEENABLE2:		
	case D3DRS_COLORWRITEENABLE3: break;
	}

	DDRAW_LEVEL(7) << "D3D9X::IDirect3DDevice7::SetRenderState()\n";

	DDRAW_LEVEL(7) << " Render State is " << x << " (setting to " << y << ")\n";

	//DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK)

	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_SETRENDERSTATE,IDirect3DDevice7*,
	DX::D3DRENDERSTATETYPE&,DWORD&)(0,this,x,y);

	auto &st = queryGL->state;
	auto cmp = st;
	switch(x) //DUPLICATE
	{
	CASE_(ZENABLE) st.zenable = y!=0;
	CASE_(FILLMODE) st.fillmode = y==D3DFILL_SOLID;
	CASE_(SHADEMODE) st.shademode = y==D3DSHADE_GOURAUD;
	CASE_(ZWRITEENABLE) st.zwriteenable = y!=0;
	CASE_(SRCBLEND) st.srcblend = y;
	CASE_(DESTBLEND) st.destblend = y;
	CASE_(CULLMODE) st.cullmode = y;
	CASE_(ZFUNC) st.zfunc = y;
	CASE_(ALPHABLENDENABLE) st.alphablendenable = y!=0; 
	break;
	case D3DRS_COLORWRITEENABLE:

		if(y) st.colorwriteenable|=1<<3; 
		if(!y) st.colorwriteenable&=~(1<<3); break;

	case D3DRS_COLORWRITEENABLE1: 
	case D3DRS_COLORWRITEENABLE2: 
	case D3DRS_COLORWRITEENABLE3:

		if(y) st.colorwriteenable|=1<<(x-D3DRS_COLORWRITEENABLE1);
		if(!y) st.colorwriteenable&=~(1<<(x-D3DRS_COLORWRITEENABLE1));
		break;
	}
	st.apply(cmp); //HACK

	out = D3D_OK; //ERROR?

	DDRAW_POP_HACK(DIRECT3DDEVICE7_SETRENDERSTATE,IDirect3DDevice7*,
	DX::D3DRENDERSTATETYPE&,DWORD&)(&out,this,x,y);

	DDRAW_RETURN(out)

	#undef CASE_
}
template<int X>
HRESULT D3D9X::IDirect3DDevice7<X>::GetRenderState(DX::D3DRENDERSTATETYPE x, LPDWORD y)
{	
	#define CASE_(x) break; case DX::D3DRENDERSTATE_##x:

	//these are meaningful to OpenGL
	switch(X?x:0) //DUPLICATE
	{
	default: return D3D9C::IDirect3DDevice7::GetRenderState(x,y);
	CASE_(ZENABLE)
	CASE_(FILLMODE)
	CASE_(SHADEMODE)
	CASE_(ZWRITEENABLE)
	CASE_(SRCBLEND)
	CASE_(DESTBLEND)
	CASE_(CULLMODE)
	CASE_(ZFUNC)    
	CASE_(ALPHABLENDENABLE)
	case D3DRS_COLORWRITEENABLE:
	case D3DRS_COLORWRITEENABLE1:
	case D3DRS_COLORWRITEENABLE2:		
	case D3DRS_COLORWRITEENABLE3: break;
	}

	DDRAW_LEVEL(7) << "D3D9X::IDirect3DDevice7::GetRenderState()\n";

	DDRAW_LEVEL(7) << " Render State is " << x << "\n";

	//DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK)

	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_GETRENDERSTATE,IDirect3DDevice7*,
	DX::D3DRENDERSTATETYPE&,LPDWORD&)(0,this,x,y);

	auto st = queryGL->state;

	switch(x) //DUPLICATE
	{
	CASE_(ZENABLE) *y = st.zenable; 
	CASE_(FILLMODE) *y = st.fillmode?D3DFILL_SOLID:D3DFILL_WIREFRAME;
	CASE_(SHADEMODE) *y = st.shademode?D3DSHADE_GOURAUD:D3DSHADE_FLAT;
	CASE_(ZWRITEENABLE) *y = st.zwriteenable;
	CASE_(SRCBLEND) *y = st.srcblend;
	CASE_(DESTBLEND) *y = st.destblend;
	CASE_(CULLMODE) *y = st.cullmode;
	CASE_(ZFUNC) *y = st.zfunc;
	CASE_(ALPHABLENDENABLE) *y = st.alphablendenable;
	break;
	case D3DRS_COLORWRITEENABLE: *y = st.colorwriteenable>>3?0xf:0;
	case D3DRS_COLORWRITEENABLE1: 
	case D3DRS_COLORWRITEENABLE2: 
	case D3DRS_COLORWRITEENABLE3:
		*y = st.colorwriteenable>>(x-D3DRS_COLORWRITEENABLE1)&1?0xf:0;
		break;
	}

	out = D3D_OK; //ERROR?

	DDRAW_POP_HACK(DIRECT3DDEVICE7_GETRENDERSTATE,IDirect3DDevice7*,
	DX::D3DRENDERSTATETYPE&,LPDWORD&)(&out,this,x,y);
	
	DDRAW_RETURN(out)

	#undef CASE_
}
static DWORD dx_d3d9X_drawprims_static(bool X, DWORD y, LPVOID z, DWORD w)
{	
	dx_d3d9X_flushaders();
	dx_d3d9c_level(dx_d3d9c_pshaders_noclip[DDRAW::ps]); //???

	//WARNING! overwriting input buffer
	//WARNING! overwriting input buffer
	//WARNING! overwriting input buffer
	//WARNING! overwriting input buffer
	if(z) switch(y&D3DFVF_POSITION_MASK)
	{
	case D3DFVF_XYZRHW: case D3DFVF_XYZW:
		
		//TODO: SWITCH TO STATIC BUFFER
		//TODO: SWITCH TO STATIC BUFFER
		//TODO: SWITCH TO STATIC BUFFER
		//TODO: SWITCH TO STATIC BUFFER
		//for SOM this isn't a problem, but this just needs some work sometime
		//
		// REMINDER: 0x4C2351 in SOM is a feature that passes polygons that
		// aren't quads and they don't appear onscreen (possibly because this
		// destroys its vertices)
		//
	//	assert(w<=4);

		y&=~D3DFVF_XYZRHW; y|=D3DFVF_XYZW;

		UINT sizeofFVF = dx_d3d9c_sizeofFVF(y);

		if(!DDRAW::doNothing)
		if(DDRAW::doScaling&&DDRAW::xyScaling[0]!=1.0f) for(size_t i=0;i<w;i++)
		{
			*(float*)(((BYTE*)z)+sizeofFVF*i)*=DDRAW::xyScaling[0]; 	
		}
		if(!DDRAW::doNothing)
		if(DDRAW::doScaling&&DDRAW::xyScaling[1]!=1.0f) for(size_t i=0;i<w;i++)
		{
			*(float*)(((BYTE*)z)+sizeofFVF*i+sizeof(float))*=DDRAW::xyScaling[1]; 	
		}
	
		auto *d = DDRAW::Direct3DDevice7;

		//D3DFVF_XYZW: convert to clipping space
		D3DVIEWPORT9 vp; vp.Width = 0; //PARANOID
		if(X) if(DDRAW::xr) //ASSUMING SYMMETRIC!
		memcpy(&vp,&d->queryGL->stereo_views->x,4*sizeof(int));
		else glGetIntegerv(GL_VIEWPORT,(int*)&vp);
		else d->proxy9->GetViewport(&vp);
		if(vp.Width&&vp.Height) //divide by zero?
		{
			int two = 2;
			DDRAW::doSuperSamplingMul(two);

			for(auto i=w;i-->0;) //clip space
			{
				float *xy = (float*)(((BYTE*)z)+sizeofFVF*i);

				xy[0] = xy[0]/vp.Width*two-1;
				xy[1] = 1-xy[1]/vp.Height*two;
			}
		}
		else assert(0);
	}
	return y; //TODO: set y/z by reference
}
template<int X>
HRESULT D3D9X::IDirect3DDevice7<X>::drawprims(DX::D3DPRIMITIVETYPE x, DWORD y, LPVOID z, DWORD w, LPWORD zi, DWORD wi)
{
	//TODO: modify both y/z by reference
	y = dx_d3d9X_drawprims_static(X,y,z,w);
	//body is moved out so dx_d3d9X_backblt can make use of it
	return D3D9X::IDirect3DDevice7<X>::drawprims2(x,y,z,w,zi,wi);
}
static HRESULT dx_d3d9X_ibuffer(GLuint &ib, DWORD r, LPWORD q)
{
	int &i = dx_d3d9c_ibuffer_i; assert(!dx_d3d9c_immediate_mode);

	i = ++i%D3D9C::ibuffersN;
	UINT &s = dx_d3d9c_ibuffers_s[i]; if(s<r||!dx_d3d9X_ibuffers[i])
	{						
		if(!dx_d3d9X_ibuffers[i]) 
		glGenBuffers(1,dx_d3d9X_ibuffers+i);		
		s = max(s*3/2,max(r,4096));
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,dx_d3d9X_ibuffers[i]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,s*2,0,GL_STREAM_DRAW);
	}	
	ib = dx_d3d9X_ibuffers[i]; 	
	/*WORD buf[4]; if(!q) //glMapBufferRange?
	{
		if(r<=4)
		{
			q = buf;
			q[0] = 0; q[1] = 1; q[2] = 2; q[3] = 3;
		}
		else{ assert(0); return !D3D_OK; }
	}*/assert(q); //2021
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ib);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,0,r*2,q);
	return D3D_OK; //ERROR
}
static HRESULT dx_d3d9X_vbuffer(GLuint &vb, DWORD qN, LPVOID q)
{
	int &i = dx_d3d9c_vbuffer_i; assert(!dx_d3d9c_immediate_mode);

	i = ++i%D3D9C::ibuffersN;
	UINT &s = dx_d3d9c_vbuffers_s[i]; if(s<qN||!dx_d3d9X_vbuffers[i])
	{						
		if(!dx_d3d9X_vbuffers[i]) 		
		glGenBuffers(1,dx_d3d9X_vbuffers+i);		
		s = max(s*3/2,max(qN,4096));
		glBindBuffer(GL_ARRAY_BUFFER,dx_d3d9X_vbuffers[i]);
		glBufferData(GL_ARRAY_BUFFER,s,0,GL_STREAM_DRAW);
	}	
	vb = dx_d3d9X_vbuffers[i];
	glBindBuffer(GL_ARRAY_BUFFER,vb);
	glBufferSubData(GL_ARRAY_BUFFER,0,qN,q);
	return D3D_OK; //ERROR
}
static std::vector<QWORD> dx_d3d9X_varrays;
static GLuint dx_d3d9X_vao(DWORD fvf, GLsizei s)
{		
	for(size_t i=dx_d3d9X_varrays.size();i-->0;)
	{
		auto &ea = dx_d3d9X_varrays[i];
		if((ea&~0u)==fvf)
		return (GLuint)(ea>>32); //hit!
	}
	GLuint vao = 0;
	glGenVertexArrays(1,&vao);
	QWORD hash = fvf|(QWORD)vao<<32;
	dx_d3d9X_varrays.push_back(hash); //miss
	
	glBindVertexArray(vao);

	int k = 0, p = 0;

	if(D3DFVF_XYZ==(fvf&D3DFVF_POSITION_MASK)) //0
	{
		k|=1<<0;
		glVertexAttribFormat(0,3,GL_FLOAT,0,0);
		p+=3*sizeof(float);
	}
	else 
	{
		k|=1<<0;
		glVertexAttribFormat(0,4,GL_FLOAT,0,0);
		p+=4*sizeof(float);
	}

	if(fvf&D3DFVF_NORMAL) //1
	{
		k|=1<<1;
		glVertexAttribFormat(1,3,GL_FLOAT,0,p);
		p+=3*sizeof(float);
	}

	//REMINDER: D3DFVF_LVERTEX actually includes this
	//it's also equal to PSIZE
	//REMINDER: PSIZE is used by dx_d3d9c_stereoVD, I
	//guess there's no conflict?
	//if(fvf&D3DFVF_PSIZE) //2
	if(fvf&D3DFVF_RESERVED1)
	{
		//NOTE: for OpenXR this can plug a hole where
		//D3DFVF_XYZRHW makes dx_d3d9X_drawprims_static
		//map screen coordinates to viewport coordinates
		//som_hacks_DrawPrimitive is trying this out since
		//D3DFVF_XYZW is currently taken as D3DFVF_XYZWRHW
		//but I may change this so it can use XYZW

		//k|=1<<2;
		//assert(0==(fvf&D3DFVF_PSIZE)); //D3DFVF_RESERVED1
		p+=4;
	}

	if(fvf&D3DFVF_DIFFUSE) //3
	{
		k|=1<<3;
		glVertexAttribFormat(3,4,GL_UNSIGNED_BYTE,true,p); 
		p+=4;
	}
	if(fvf&D3DFVF_SPECULAR) //4
	{
		k|=1<<4;
		glVertexAttribFormat(4,4,GL_UNSIGNED_BYTE,true,p); 
		p+=4;
	}

	//2022: 5, 6, 7, 8 are supplying OpenXR parameters
	//on an instance VBO
	for(int ii=5,i=ii;i<=8;i++) 
	{
		//layout(location=5) in float4 Xr_fov; //asymmetric projection
		//layout(location=6) in float4 Xr_mvP; //model-view position/aa?
		//layout(location=7) in float4 Xr_mvQ; //model-view quaternion
		//layout(location=8) in float4 Xr_vpR; //viewport and frustum
		glVertexAttribBinding(i,1); //queryGL->stereo?
		glVertexAttribFormat(i,4,GL_FLOAT,0,(i-ii)*4*sizeof(float));
		if(DDRAW::xr)
		glEnableVertexAttribArray(i);
	}
	//instancing?
	//NOTE: passing 0 stride to glBindVertexBuffer works too?
	glVertexBindingDivisor(1,1);

	int n = (fvf&D3DFVF_TEXCOUNT_MASK)>>D3DFVF_TEXCOUNT_SHIFT;
	for(int x,i=0;i<n;i++) //10-24
	{
		//1/1/1/1: a two bit mask is used, and 1 covers both bits
		if((fvf&D3DFVF_TEXCOORDSIZE1(i))==D3DFVF_TEXCOORDSIZE2(i))
		x = 2;
		else if((fvf&D3DFVF_TEXCOORDSIZE1(i))==D3DFVF_TEXCOORDSIZE1(i))
		x = 1;
		else if((fvf&D3DFVF_TEXCOORDSIZE1(i))==D3DFVF_TEXCOORDSIZE3(i))
		x = 3;
		else if((fvf&D3DFVF_TEXCOORDSIZE1(i))==D3DFVF_TEXCOORDSIZE4(i))
		x = 4;
		else continue; k|=1<<10+i;
		glVertexAttribFormat(10+i,x,GL_FLOAT,0,p);
		p+=sizeof(float)*x;
	}

	assert(p==s);

	for(int i=0;k;k>>=1,i++) if(k&1)
	{
		glVertexAttribBinding(i,0); glEnableVertexAttribArray(i);
	}
	return vao;
}
static void dx_d3d9X_vao_enable_xr(bool how)
{
	auto &va = dx_d3d9X_varrays;		
	for(auto i=va.size();i-->0;)
	{
		GLuint vao = va[i]>>32;
		glBindVertexArray(vao);
		for(int j=5;j<=8;j++) 
		(how?glEnableVertexAttribArray:glDisableVertexAttribArray)(j);
	}
}
template<int X>
HRESULT D3D9X::IDirect3DDevice7<X>::drawprims2(DX::D3DPRIMITIVETYPE x, DWORD y, LPVOID z, DWORD vN, LPWORD zi, DWORD wi)
{
	//2021: I THINK NEARLY ALL DRAWING CALLS NOW REDIRECT TO THIS SUBROUTINE

	//y is fvf, z is verts, w/vN is num verts, zi is indices, wi is num indices

	enum{ v=0,v0=0,i=0 }; //these aren't used

	HRESULT out = D3D_OK;
	
	UINT sizeofFVF; if(z) sizeofFVF = dx_d3d9c_sizeofFVF(y);

	//REFERENCE: see dx.d3d9c.cpp for immediate algorithm
	int compile[!dx_d3d9c_immediate_mode]; (void)compile;
	if(X)
	{
		GLenum mode; switch(x)
		{
		case D3DPT_TRIANGLELIST: mode = 4; break; //GL_TRIANGLES
		case D3DPT_TRIANGLESTRIP: mode = 5; break; //GL_TRIANGLE_STRIP
		case D3DPT_TRIANGLEFAN: mode = 6; break; //GL_TRIANGLE_FAN
		case D3DPT_LINELIST: mode = 1; break; //GL_LINES
		default: assert(0); return !D3D_OK; //lines/points??
		}

		//OpenGL needs VB to go first since glBindBuffer
		//really binds the elements to the current VAO
		//https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindBuffer.xhtml
		GLuint ib = 0, vb = 0; if(z)
		{
			out = dx_d3d9X_vbuffer(vb,vN*sizeofFVF,z);
			//dx_d3d9X_ibuffer has to bind the elements
			//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ib);
			glBindVertexArray(dx_d3d9X_vao(y,sizeofFVF));
			glBindVertexBuffer(0,vb,0,sizeofFVF);
		}

		if(zi)
		{
			out = dx_d3d9X_ibuffer(ib,wi,zi); //binds to VAO		
			out = D3D_OK; //ERROR?
		}

		if(DDRAW::inStereo) //OpenXR?
		{
			//NOTE: PlayStation VR style stereo should use Direct3D 9
			//REMINDER: glVertexBindingDivisor is for instancing, but
			//the scissor (stereo) method would probably do better if
			//it used shader constants instead of instancing that the
			//old approach relied on (with clip/discard instructions)
			//
			// NOTE: OpenXR would need to pack 3 registers into the
			// instance channel and unpack them into the projection
			// and model-view matrix

			auto *q = DDRAW::Direct3DDevice7->queryGL;
			assert(q->stereo);
						
			#ifdef NDEBUG
//			#error is this unbound on xquit? hard-coding 4?
			#endif
			enum{ vstride=4*4*sizeof(float) };

			//TODO: need to iterate over queryGL->stereo and keep it
			//up-to-date
			for(int i=0;i<_countof(q->stereo_views);i++)
			{
				auto &v = q->stereo_views[i]; if(!v.src) break;

				//I don't think SCISSOR_TEST will cut it but I
				//don't know what else to do about leaving the
				//viewport set to the last view's
				glViewport(v.x,v.y,v.w,v.h);

				//NOTE: 0 stride might implement instancing, but
				//I'm not sure. glVertexBindingDivisor is set to
				//facilitate instances
				glBindVertexBuffer(1,q->stereo,i*vstride,vstride);

				if(zi)
				{
					glDrawRangeElements(mode,0,vN-1,wi,0x1403,0);
				}
				else glDrawArrays(mode,0,vN);
			}
		}
		else if(zi)
		{
			//GL_UNSIGNED_SHORT
			//NOTE: relying on glBindVertexBuffer to set the baseline
			//(might help to pass it, giving drivers too much credit)
			glDrawRangeElements(mode,0,vN-1,wi,0x1403,0);
		}
		else glDrawArrays(mode,0,vN);
	}	
	else 
	{		
		//TODO? these probably shouldn't be "this" methods since they call global
		//functions that use DDRAW::IDirect3DDevice7
		//auto *p = this->proxy9; assert(this==DDRAW::IDirect3DDevice7);
		auto *p = DDRAW::Direct3DDevice7->proxy9; 

		if(!zi) wi = vN; //non-indexed mode?

		int prims; switch(x)
		{
		case D3DPT_TRIANGLELIST: prims = wi/3; break;
		case D3DPT_TRIANGLESTRIP: prims = wi-2; break;
		case D3DPT_TRIANGLEFAN: prims = wi-2; break;
		case D3DPT_LINELIST: prims = wi/2; break; //2018
		default: assert(0); return !D3D_OK; //lines/points??
		}
		
		bool stereo = false; if(DDRAW::inStereo)
		{
			if(IDirect3DVertexDeclaration9*vd=dx_d3d9c_stereoVD(y))
			{
				stereo = true; //2021

				p->SetVertexDeclaration(vd);
			}
			else //goto fvf;
			{
				goto fvf; //breakpoint
			}
		}
		else fvf: out = p->SetFVF(y);
				
		IDirect3DIndexBuffer9 *ib = 0;
		IDirect3DVertexBuffer9 *vb = 0;
		if(zi&&!out) out = dx_d3d9c_ibuffer(p,ib,wi,zi);		
		if(z&&!out) out = dx_d3d9c_vbuffer(p,vb,vN*sizeofFVF,z);

		if(out) return out;

		if(ib) p->SetIndices(ib); //assert(ib);
		if(vb) p->SetStreamSource(0,vb,0,sizeofFVF);
		//z/zi are already built into ib/vb above
		//out = p->DrawIndexedPrimitive((D3DPRIMITIVETYPE)x,z,0,vN,zi,prims);
		//out = p->DrawIndexedPrimitive((D3DPRIMITIVETYPE)x,0,0,vN,0,prims);			
		//out = dx_d3d9c_drawprims2(stereo,(D3DPRIMITIVETYPE)x,vN,prims);
		{
			if(stereo&&dx_d3d9c_stereo_scissor) //TESTING
			{
				//NOTE: dx.d3d9c.cpp has a test done with clipping+instancing
				//that didn't quite work on my Intel system, but drawing both
				//separately with scissor is better performance and removes a
				//"clip" instruction from the shader that blocks early-Z test

				D3DVIEWPORT9 vp; p->GetViewport(&vp);
				RECT sr = {0,0,vp.Width/2,vp.Height};
				p->SetScissorRect(&sr);
				p->SetRenderState(D3DRS_SCISSORTESTENABLE,1);
				out = p->SetStreamSource(1,DDRAW::Direct3DDevice7->query9->stereo,8,8);				
				if(ib) out = p->DrawIndexedPrimitive((D3DPRIMITIVETYPE)x,v,v0,vN,i,prims);
				if(!ib) out = p->DrawPrimitive((D3DPRIMITIVETYPE)x,v,prims);
				p->SetStreamSource(1,DDRAW::Direct3DDevice7->query9->stereo,0,8);
				sr.left = sr.right; sr.right = vp.Width;
				p->SetScissorRect(&sr);
				if(ib) out = p->DrawIndexedPrimitive((D3DPRIMITIVETYPE)x,v,v0,vN,i,prims);
				if(!ib) out = p->DrawPrimitive((D3DPRIMITIVETYPE)x,v,prims);
				p->SetRenderState(D3DRS_SCISSORTESTENABLE,0);
			}
			else if(ib)
			{
				out = p->DrawIndexedPrimitive((D3DPRIMITIVETYPE)x,v,v0,vN,i,prims); 
			}
			else out = p->DrawPrimitive((D3DPRIMITIVETYPE)x,v,prims); 
		}
	}
	
	return out;
}
template<int X>
HRESULT D3D9X::IDirect3DDevice7<X>::DrawPrimitive(DX::D3DPRIMITIVETYPE x, DWORD y, LPVOID z, DWORD w, DWORD q)
{
	DDRAW_LEVEL(7) << "D3D9X::IDirect3DDevice7::DrawPrimitive()\n";

	//DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK)

	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_DRAWPRIMITIVE,IDirect3DDevice7*,
	DX::D3DPRIMITIVETYPE&,DWORD&,LPVOID&,DWORD&,DWORD&)(0,this,x,y,z,w,q);
							 
	if(!w||!z) DDRAW_FINISH(D3D_OK)

	assert(q==0); //what is q?

	DDRAW_LEVEL(7) << " Primitive Type is " << x << " (" << w << " vertices)\n";
		
	if(DDRAWLOG::debug>=7) //???
	{
		DDRAW::log_FVF(y);

		#define OUT(X) if(x==D3DPT_##X) DDRAW_LEVEL(7) << ' ' << #X << '\n';
		OUT(TRIANGLELIST)
		OUT(TRIANGLESTRIP)
		OUT(TRIANGLEFAN)
		#undef OUT
	}

	out = D3D9X::IDirect3DDevice7<X>::drawprims(x,y,z,w);

	DDRAW_POP_HACK(DIRECT3DDEVICE7_DRAWPRIMITIVE,IDirect3DDevice7*,
	DX::D3DPRIMITIVETYPE&,DWORD&,LPVOID&,DWORD&,DWORD&)(&out,this,x,y,z,w,q);	

	DDRAW_RETURN(out)
}
template<int X>
HRESULT D3D9X::IDirect3DDevice7<X>::DrawIndexedPrimitive(DX::D3DPRIMITIVETYPE x, DWORD y, LPVOID z, DWORD w, LPWORD q, DWORD r, DWORD s)
{
	//assuming: y is fvf, z is verts, w is vertex count, q is indices, r is index count, s flags (but not sure)

	DDRAW_LEVEL(7) << "D3D9X::IDirect3DDevice7::DrawIndexedPrimitive()\n";

	//DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK)

	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_DRAWINDEXEDPRIMITIVE,IDirect3DDevice7*,
	DX::D3DPRIMITIVETYPE&,DWORD&,LPVOID&,DWORD&,LPWORD&,DWORD&,DWORD&)(0,this,x,y,z,w,q,r,s);	

	//2022: see DrawIndexedPrimitiveVB comment
	//on crashes
	if(!w||!r) DDRAW_FINISH(D3D_OK) //short-circuit this way

	out = D3D9X::IDirect3DDevice7<X>::drawprims(x,y,z,w,q,r);
		
	DDRAW_POP_HACK(DIRECT3DDEVICE7_DRAWINDEXEDPRIMITIVE,IDirect3DDevice7*,
	DX::D3DPRIMITIVETYPE&,DWORD&,LPVOID&,DWORD&,LPWORD&,DWORD&,DWORD&)(&out,this,x,y,z,w,q,r,s);	

	DDRAW_RETURN(out);
}
template<int X>
HRESULT D3D9X::IDirect3DDevice7<X>::DrawIndexedPrimitiveVB(DX::D3DPRIMITIVETYPE x, DX::LPDIRECT3DVERTEXBUFFER7 y, DWORD z, DWORD w, LPWORD q, DWORD r, DWORD s)
{
	//Note: z is vertex offset, w is vertex count, q is index pointer, r is index count, s is wait flags

	DDRAW_LEVEL(7) << "D3D9X::IDirect3DDevice7::DrawIndexedPrimitiveVB()\n";

	//DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK) 
	
	if(!y) DDRAW_RETURN(!D3D_OK)

	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_DRAWINDEXEDPRIMITIVEVB,IDirect3DDevice7*,
	DX::D3DPRIMITIVETYPE&,DX::LPDIRECT3DVERTEXBUFFER7&,DWORD&,DWORD&,LPWORD&,DWORD&,DWORD&)(0,this,x,y,z,w,q,r,s);

	//2022: I think crashes started happening when I removed these
	//checks... dx_d3d9c_ibuffer can't handle 0 r, but even with a
	//fix it still crashes somewhere :(
	#if 0
	if(!w/*||!q||!r*/) DDRAW_FINISH(D3D_OK) //short-circuit this way
	#else
	if(!w||!r) DDRAW_FINISH(D3D_OK) //short-circuit this way
	#endif

	if(DDRAWLOG::debug>=7)
	{
#define OUT(X) if(x==D3DPT_##X) DDRAW_LEVEL(7) << ' ' << #X << '\n';
						   
		OUT(TRIANGLELIST)
		OUT(TRIANGLESTRIP)
		OUT(TRIANGLEFAN)

#undef OUT
	}

	DDRAW_LEVEL(7) << "vertex offset/count is " << z << '/' << w << "\n";
	DDRAW_LEVEL(7) << "index address/count is " << (unsigned)q << '/' << r << "\n";

	auto p = DDRAW::is_proxy<IDirect3DVertexBuffer7<X>>(y);
	if(!p) DDRAW_FINISH(!D3D_OK)		   

	//REFERENCE: see dx.d3d9c.cpp for immediate algorithm
	int compile[!dx_d3d9c_immediate_mode]; (void)compile;
	/*if(dx_d3d9c_immediate_mode)
	{
		out = drawprims(x,p->queryX->FVF,p->queryX->upbuffer,w,q,r);
		DDRAW_FINISH(out)
	}*/
			
	UINT stride = p->queryX->sizeofFVF;

	//2021: dx_d3d9X_drawprims_static needs to modify XYZW
	assert(D3DFVF_XYZ==(p->queryX->FVF&D3DFVF_POSITION_MASK));

	//TODO? It's probably better to just use
	//dx_d3d9c_vbuffer here, except unlike the
	//non-VB APIs indices can be offset into the
	//vbuffer		
	if(p->queryX->upbuffer)
	{
		//UINT floor = z*stride;
		UINT limit = (z+w)*stride; 			
		if(0) //2021: this always works, doesn't reuse though
		{
			//NOTE: another downside I can see here is even
			//though dx_d3d9c_vbuffer rotates a few buffers
			//it still starts at 0 everytime, whereas the
			//staggered implementation below might have the
			//benefit of not colliding. although it depends
			//on the app

			//TEST MY FPS SOMETIME
			out = D3D9X::IDirect3DDevice7<X>::
			drawprims(x,p->queryX->FVF,(char*)p->queryX->upbuffer+z*stride,w,q,r);
			DDRAW_FINISH(out)
		}
		else if(p->queryX->mixed_mode_lock<limit) //???
		{
			//2021: let me explain, in Direct3D 7 the vertex buffers seem
			//able to be freely locked and unlocked and there is no partial
			//locking. I don't know if it flushes the entire buffer on unlock
			//I hope not because SOM does lots of little locks/unlocks instead
			//of batching, so the rationale here is to flush the buffer as a
			//reponse to a drawing call. I don't think D3D7 vbuffers were VRAM
			//buffers but were DMA buffers. I think they were implemented mainly
			//in software since they came on the scene long before VRAM buffers
			//SOM wouldn't be able to function if they worked differently since
			//it draw calls would just be sending garbage

			assert(limit<=p->queryX->mixed_mode_size);
				  
			//2021: this used to be z*stride but this seems better to me
			UINT floor = p->queryX->mixed_mode_lock;

			//I think this is envisioning iterating through the buffer making
			//individual drawing calls
			/*2021: I think restarting doesn't make sense since D3D7 can only
			//fill all or nothing? I.e. it will just upload everything again!
			p->queryX->mixed_mode_lock = z?limit:0;*/
			p->queryX->mixed_mode_lock = limit; 
		
			if(X)
			{
				glBindBuffer(GL_ARRAY_BUFFER,p->queryX->vboGL);
				glBufferSubData(GL_ARRAY_BUFFER,floor,limit-floor,(char*)p->queryX->upbuffer+floor);
				out = D3D_OK; //ERROR?
			}
			else
			{
				void *b;
				out = p->proxy9->Lock(floor,limit-floor,&b,D3DLOCK_DISCARD);
				if(!out) memcpy(b,(char*)p->queryX->upbuffer+floor,limit-floor);
				if(!out) out = p->proxy9->Unlock();
			}
			if(out) DDRAW_FINISH(out)
		}
		else assert(p->queryX->mixed_mode_size>=limit); 
	}

	if(X)
	{
		glBindVertexArray(p->queryX->vaoGL);
		glBindVertexBuffer(0,p->queryX->vboGL,z*stride,stride);
	}
	else 
	{
		//NOTE: before 2021 this hadn't used drawprims... since it is
		//now, SetStreamSource is the only way to offset to z without
		//passing z to drawprims (SOM doesn't use nonzero z)
		proxy9->SetStreamSource(0,p->proxy9,z*stride,stride);	
	}

	out = D3D9X::IDirect3DDevice7<X>::drawprims(x,p->queryX->FVF,0,w,q,r);

	if(X)
	{
		glBindVertexBuffer(0,0,0,stride); //NICE?

		out = D3D_OK; //ERROR?
	}
	else proxy9->SetStreamSource(0,0,0,0); //NICE

	DDRAW_POP_HACK(DIRECT3DDEVICE7_DRAWINDEXEDPRIMITIVEVB,IDirect3DDevice7*,
	DX::D3DPRIMITIVETYPE&,DX::LPDIRECT3DVERTEXBUFFER7&,DWORD&,DWORD&,LPWORD&,DWORD&,DWORD&)(&out,this,x,y,z,w,q,r,s);

	DDRAW_RETURN(out)
}
template<int X>
HRESULT D3D9X::IDirect3DDevice7<X>::SetTexture(DWORD x, DX::LPDIRECTDRAWSURFACE7 y)
{
	DDRAW_LEVEL(7) << "D3D9X::IDirect3DDevice7::SetTexture()\n";

	//DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK) 
		
	if(x>=DDRAW::textures){	assert(!y); return D3D_OK; }

//	DDRAW_LEVEL(7) << " Texture is " << x << " (surface is " << y << ")\n";		 	

	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_SETTEXTURE,IDirect3DDevice7*,
	DWORD&,DX::LPDIRECTDRAWSURFACE7&)(0,this,x,y);

	auto p = DDRAW::is_proxy<IDirectDrawSurface7>(y);
	if(!p)
	{
		DDRAW::TextureSurface7[0] = 0;

		/*discontinuing this in favor of TEXTURE0_NOCLIP
		dx_d3d9X_psconstant(DDRAW::psColorkey,0.0f,'w');
		*/

		if(X)
		{
			glBindTexture(GL_TEXTURE_2D,queryGL->white);

			out = D3D_OK; //ERROR?
		}
		else out = proxy9->SetTexture(x,query9->white);

		DDRAW_FINISH(y?!D3D_OK:out)
	}

	D3D9X::updating_texture(p,false);

	if(X)
	{
		glBindTexture(GL_TEXTURE_2D,p->queryX->updateGL);

		out = D3D_OK; //ERROR?
	}
	else out = proxy9->SetTexture(x,p->query9->update9);
			
	DDRAW::TextureSurface7[0] = out==D3D_OK?p:0;

	if(X) //I'M DISABLING THIS FOR D3D9 FOR NOW BECAUSE IT'S HARDER TO SET IT UP
	{
		//TODO? dx_d3d9c_colorkeyenable MIGHT BE FACTORED INTO THIS CALCULATION
		dx_d3d9c_pshaders_noclip = !p->queryX->knockouts?dx_d3d9c_pshaders2:dx_d3d9c_pshaders;
	}

	/*discontinuing this in favor of TEXTURE0_NOCLIP
	float ck = out==D3D_OK?(p->queryX->knockouts&&dx_d3d9c_colorkeyenable?1.0f:0.0f):0.0f; //colorkey
	if(DDRAW::psColorkey) dx_d3d9X_psconstant(DDRAW::psColorkey,ck,'w');
	*/

	if(DDRAW::psTextureState) //2022
	{
		float ts[4] = {p->queryX->width,p->queryX->height};
		ts[2] = 1/ts[0]; ts[3] = 1/ts[1];
		dx_d3d9X_psconstant(DDRAW::psTextureState,ts);
	}

	DDRAW_POP_HACK(DIRECT3DDEVICE7_SETTEXTURE,IDirect3DDevice7*,
	DWORD&,DX::LPDIRECTDRAWSURFACE7&)(&out,this,x,y);

	DDRAW_RETURN(out) 
}
template<int X>
HRESULT D3D9X::IDirect3DDevice7<X>::GetTextureStageState(DWORD x, DX::D3DTEXTURESTAGESTATETYPE y, LPDWORD z)
{
	//REMINDER: StateBlock only has room for one texture
	//if(x>=DDRAW::textures){ assert(0); return D3D_OK; }
	if(x){ assert(0); assert(DDRAW::textures==1); return !D3D_OK; }

	#define CASE_(y) break; case DX::D3DTSS_##y:

	//these are meaningful to OpenGL
	switch(X?y:0) //DUPLICATE
	{
	default: return D3D9C::IDirect3DDevice7::GetTextureStageState(x,y,z);
	CASE_(ADDRESS) assert(0);
	CASE_(ADDRESSU)		
	CASE_(ADDRESSV)
	CASE_(MAGFILTER)
	CASE_(MINFILTER)
	CASE_(MIPFILTER) break;
	}

	DDRAW_LEVEL(7) << "D3D9X::IDirect3DDevice7::GetTextureStageState()\n";

	//DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK)

	DDRAW_LEVEL(7) << " Stage is " << x << " (" << y << ")\n";

	auto st = queryGL->state;

	switch(y) //DUPLICATE
	{
	CASE_(ADDRESSU) *z = st.addressu;
	CASE_(ADDRESSV) *z = st.addressv;
	CASE_(MAGFILTER) *z = st.magfilter?D3DTEXF_LINEAR:D3DTEXF_POINT;
	CASE_(MINFILTER) *z = st.minfilter?D3DTEXF_LINEAR:D3DTEXF_POINT;
	CASE_(MIPFILTER) *z = st.mipfilter; break;
	}

	#undef CASE_

	return D3D_OK;
}
template<int X>
HRESULT D3D9X::IDirect3DDevice7<X>::SetTextureStageState(DWORD x, DX::D3DTEXTURESTAGESTATETYPE y, DWORD z)
{
	//REMINDER: StateBlock only has room for one texture
	//if(x>=DDRAW::textures){ assert(0); return D3D_OK; }
	if(x){ assert(0); assert(DDRAW::textures==1); return !D3D_OK; }

	#define CASE_(y) break; case DX::D3DTSS_##y:

	//these are meaningful to OpenGL
	switch(X?y:0) //DUPLICATE
	{
	default: return D3D9C::IDirect3DDevice7::SetTextureStageState(x,y,z);
	CASE_(ADDRESS)
	CASE_(ADDRESSU)		
	CASE_(ADDRESSV)
	CASE_(MAGFILTER)
	CASE_(MINFILTER)
	CASE_(MIPFILTER) break;
	}

	DDRAW_LEVEL(7) << "D3D9X::IDirect3DDevice7::SetTextureStageState()\n";

	//DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK)
	
	DDRAW_LEVEL(7) << " Stage is " << x << " (" << y << " set to " << z << ")\n";

	DDRAW_PUSH_HACK(DIRECT3DDEVICE7_SETTEXTURESTAGESTATE,IDirect3DDevice7*,
	DWORD&,DX::D3DTEXTURESTAGESTATETYPE&,DWORD&)(0,this,x,y,z);

	assert(x==0); //assuming

	if(y==DX::D3DTSS_MIPFILTER) //2018
	{
		z--; assert(z>=0&&z<=2); //d3d8/7 differs from d3d9		
	}
	if(DDRAW::doMipmaps)
	if(y==DX::D3DTSS_MINFILTER) //hack 
	{	
		DWORD n = DDRAW::anisotropy;
		
		n = n?min(n,dx_d3d9c_anisotropy_max):dx_d3d9c_anisotropy_max;

		//if(n>1) //NEW: 2018
		{
			//z = D3DTEXF_ANISOTROPIC;

			int sam0 = queryGL->samplers[0];
			glSamplerParameteri(sam0,GL_TEXTURE_MAX_ANISOTROPY_EXT,n);
		}
	}
	//forcing linear filter (for now)
	if(y==DX::D3DTSS_MAGFILTER||y==DX::D3DTSS_MIPFILTER)
	{
		if(DDRAW::linear) z = D3DTEXF_LINEAR;
	}

	auto &st = queryGL->state;
	auto cmp = st;
	switch(y) //DUPLICATE
	{
	CASE_(ADDRESS) st.addressu = st.addressv = z;
	CASE_(ADDRESSU) st.addressu = z;
	CASE_(ADDRESSV) st.addressv = z;
	CASE_(MAGFILTER) st.magfilter = z==D3DTEXF_LINEAR;
	CASE_(MINFILTER) st.minfilter = z==D3DTEXF_LINEAR;
	CASE_(MIPFILTER) st.mipfilter = z; break;
	}
	st.apply(cmp);

	out = D3D_OK; //ERROR?

	DDRAW_POP_HACK(DIRECT3DDEVICE7_SETTEXTURESTAGESTATE,IDirect3DDevice7*,
	DWORD&,DX::D3DTEXTURESTAGESTATETYPE&,DWORD&)(&out,this,x,y,z);

	DDRAW_RETURN(out)

	#undef CASE_
}



template<int X>
HRESULT D3D9X::IDirect3DVertexBuffer7<X>::Lock(DWORD x, LPVOID *y, LPDWORD z)
{
	//x: DDLOCK_DISCARDCONTENTS|DDLOCK_NOSYSLOCK|DDLOCK_WRITEONLY

	DDRAW_LEVEL(7) << "D3D9X::IDirect3DVertexBuffer7::Lock()\n";

	//DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK)
		
	DDRAW_PUSH_HACK(DIRECT3DVERTEXBUFFER7_LOCK,IDirect3DVertexBuffer7*,
	DWORD&,LPVOID*&,LPDWORD&)(0,this,x,y,z);
	
	DWORD sz; if(x&0x80000000L) //2021: partial_Lock?
	{
		sz = *z*query9->sizeofFVF;			
		assert(sz<=query9->mixed_mode_size);
	}
	else sz = queryX->mixed_mode_size;

	if(queryX->upbuffer) mixed_mode:
	{
		//2021: this was done below? I don't undestand
		//how DrawPrimitiveVB expects to use this???
		//
		// I think the idea is the program will make
		// lots of little locks/unlocks so that this
		// would start over at the begging. whenever
		// DrawIndexedPrimitiveVB draws at vertex #0
		// it rolls this back to 0. but I think that
		// if the buffer is just a DMA receiving bay
		// it probably makes sense to always send it
		// over and never place anything into proxy9
		//
		queryX->mixed_mode_lock = 0; //2021

		if(!y) DDRAW_FINISH(!D3D_OK)

		*y = queryX->upbuffer; if(z) *z = sz;
		
		DDRAW_FINISH(D3D_OK);
	}

	DWORD flags = 0; 
	//THIS IS dx_d3d9c_immediate_mode's BOTTLENECK
	//D3DLOCK_DISCARD; //not ok for SOM map pieces
	//(still DDLOCK_DISCARDCONTENTS is sent to it)
	//2021: I think this is bad for SOM since D3D7
	//can't partial lock and it locks on each draw
	//although I'm about to implement a batch path
	//(I think I see a positive performance impact
	//although not a lot)
	if(1 //2021: TESTING
	//&&proxy9 //2021: OpenGL?
	&&x&DDLOCK_DISCARDCONTENTS)
	{
		if(X)
		{
			glBindBuffer(GL_ARRAY_BUFFER,queryX->vboGL);
			*y = //GL_MAP_WRITE_BIT|GL_MAP_INVALIDATE_BUFFER_BIT
			glMapBufferRange(GL_ARRAY_BUFFER,0,sz,2|8);
			out = D3D_OK; //ERROR?
			goto X;			
		}
		
		//doesn't work with map pieces, but it still
		//has same meaning as DDLOCK_DISCARDCONTENTS
		flags|=D3DLOCK_DISCARD; 
		//flags|=D3DLOCK_NOOVERWRITE;
	}
	else if(1) //som_scene_4137F0_Lock
	{
		queryX->mixed_mode_lock = 0; //???

		if(!queryX->upbuffer) 
		{
			assert(!dx_d3d9c_immediate_mode);
			queryX->upbuffer = new char[queryX->mixed_mode_size];
			goto mixed_mode;
		}
	}

//	if((x&DDLOCK_WAIT)==0) flags|=D3DLOCK_DONOTWAIT;

	assert(proxy9); //2021: dx.d3d9X.cpp may not allocate a proxy

	out = proxy9->Lock(0,sz,y,flags); 

X:	if(!out&&z) *z = sz; 
	
	DDRAW_POP_HACK(DIRECT3DVERTEXBUFFER7_LOCK,IDirect3DVertexBuffer7*,
	DWORD&,LPVOID*&,LPDWORD&)(&out,this,x,y,z);

	DDRAW_RETURN(out)
}
template<int X>
HRESULT D3D9X::IDirect3DVertexBuffer7<X>::Unlock()
{
	DDRAW_LEVEL(7) << "D3D9X::IDirect3DVertexBuffer7::Unlock()\n";

	//DDRAW_IF_NOT_TARGET_RETURN(!D3D_OK)

	if(query9->upbuffer) return D3D_OK;

	HRESULT out; if(X)
	{
		glBindBuffer(GL_ARRAY_BUFFER,queryX->vboGL);
		glUnmapBuffer(GL_ARRAY_BUFFER);
		out = D3D_OK; //ERROR?
	}
	else out = proxy9->Unlock();

	DDRAW_RETURN(out)
}



////////////////////////////////////////////////////////
//              PROCEDURAL INTERFACE                  //
////////////////////////////////////////////////////////

extern void dx_d3d9X_register()
{
	DDRAW::ff = false;
	DDRAW::gl = false; switch(DDRAW::target)
	{
	case 'dx95': case 'dxGL': DDRAW::gl = true;
	break;
	default: assert('dx9c'==DDRAW::target); break;
	}

	#define DDRAW_TABLES(Interface) \
	D3D9X::Interface().register_tables(DDRAW::target);

	DDRAW_TABLES(IDirectDraw) 
	DDRAW_TABLES(IDirectDraw7)
	DDRAW_TABLES(IDirectDrawClipper) 
	DDRAW_TABLES(IDirectDrawPalette) 
	DDRAW_TABLES(IDirectDrawSurface) 
	DDRAW_TABLES(IDirectDrawSurface7)
	DDRAW_TABLES(IDirectDrawGammaControl)
	if(DDRAW::gl) DDRAW_TABLES(IDirect3D7<true>) 
	if(DDRAW::gl) DDRAW_TABLES(IDirect3DDevice7<true>)
	if(DDRAW::gl) DDRAW_TABLES(IDirect3DVertexBuffer7<true>) 
	if(!DDRAW::gl) DDRAW_TABLES(IDirect3D7<false>) 
	if(!DDRAW::gl) DDRAW_TABLES(IDirect3DDevice7<false>) 
	if(!DDRAW::gl) DDRAW_TABLES(IDirect3DVertexBuffer7<false>) 

	#undef DDRAW_TABLES
}
static void __stdcall dx_d3d9X_glPolygonMode_nop(GLenum,GLenum){}
static void __stdcall dx_d3d9X_glShadeModel_nop(GLenum){} //UNUSED
extern int D3D9X::is_needed_to_initialize()
{
	static int one_off = 0; 
	if(one_off) return one_off; one_off = 1; //??? 

	//this library is intended for x86 only, but anyway the
	//GLuint handles need to overlap with Direct3D pointers
	int compile[sizeof(void*)==sizeof(int)]; (void)compile;

	if('dx95'==DDRAW::target)
	{
		//WARNING: MINIMAL IS 3.1
		// 
		//TODO: how to supply a user designated backend?
		using namespace Widgets95;
		//REMINDER: I think xr_Direct3D_11 can't do sRGB
		//correctly, so I guess there's no point in this
		//since 'dxGL' offers OpenGL
		//https://bugs.chromium.org/p/angleproject/issues/detail?id=6309
		int xe = 1&&DX::debug?xr_Direct3D_11:xr_OpenGL;

		//TEMPORARY
		//probably the client/app code should manage 
		//this
		#ifdef WIDGETS_95_DELAYLOAD
		Widgets95::delayload(xe);
		#endif

		//I'm thinking this might not be built into Exselector
		//WIDGETS_95_NOUI is in the way of this unfortunately?
		//if(Widgets95::delayload.glut->get_ANGLE_enabled())
		if(Widgets95::gle::glEnable)
		{
			#undef X
			#define X(f) f = Widgets95::gle::f;
			DX_D3D9X_X
			glPolygonMode = dx_d3d9X_glPolygonMode_nop;
			glShadeModel = dx_d3d9X_glShadeModel_nop; //UNUSED
		}
		else DDRAW::target = 'dxGL';
	}
	if('dxGL'==DDRAW::target) //else
	{
		//WARNING: MINIMAL IS 4.5

		//2022: this isn't necessary, but xr_NOGLE_OpenGL_WIN32
		//is using xr and in the future it may to to initialize
		//something
		#ifdef WIDGETS_95_DELAYLOAD
		Widgets95::delayload(0);
		#endif
	}

	dx_d3d9X_register(); //DirectDrawCreateEx
	
	const char *msg = 0; switch(DDRAW::target)
	{
	default: assert(0); //UNEXPECTED
		
		DDRAW::target = 'dx9c'; //D3D9C::is_needed_to_initialize?

	case 'dx9c': msg = "Direct3D9"; break;
	case 'dx95': msg = "OpenGL-ES"; break;
	case 'dxGL': msg = "OpenGL-32"; break;
	}
	DDRAW_HELLO(0) << msg << " initialized\n";

	return 1; //for static thunks //???
}

namespace D3D9C
{
	extern bool updating_texture(DDRAW::IDirectDrawSurface7*,int);
}
extern void dx_d3d9X_updating_texture2(DDRAW::IDirectDrawSurface7 *p, int x, int y)
{
	auto q = p->query9;
	auto g = q->group9;

	int lvs = max(1,q->mipmaps);
			
	auto &teX = q->updateGL; assert(p->proxy9);
		
	//NOTE: D3D does this with D3DSAMP_SRGBTEXTURE
	int sfmt = DDRAW::sRGB?GL_SRGB8_ALPHA8:GL_RGBA8;

	if(!teX)
	{
		if(D3DFMT_A8==p->queryX->format) sfmt = GL_R8;

		glGenTextures(1,&teX);
		glBindTexture(GL_TEXTURE_2D,teX); //HOOK
		glTexStorage2D(GL_TEXTURE_2D,lvs,sfmt,q->width,q->height);
	}
	else glBindTexture(GL_TEXTURE_2D,teX); //HOOK

	IDirect3DSurface9 *pp = p->proxy9;
	for(int lv=0;lv<lvs;lv++)
	{
		if(g&&g->GetSurfaceLevel(lv,&pp))
		{
			assert(0); break;
		}
		D3DLOCKED_RECT lock;
		if(!pp->LockRect(&lock,0,D3DLOCK_NO_DIRTY_UPDATE|D3DLOCK_READONLY|D3DLOCK_NOSYSLOCK))
		{			
			D3DSURFACE_DESC desc; pp->GetDesc(&desc);

			auto f = desc.Format==D3DFMT_A8?GL_RED:GL_RGBA; //GL_R8

			//NOTE: OpenXR code had set this to 1 (foveated rendering)
			//you need to know lock.Pitch expects 4 (for GL_R8)
			//glPixelStorei(0x0CF5,4); //GL_UNPACK_ALIGNMENT

			//GL_ARB_clip_control?
			//interesting article?
			//https://developer.nvidia.com/content/depth-precision-visualized
			//TODO: will probably have to use glMapBuffer to flip upside-down
			//
			// OR? maybe they cancel out?
			//
			//kooky OpenGL... ANGLE says texture is immutable (glTexStorage2D)
			//glTexImage2D(GL_TEXTURE_2D,lv,sfmt,desc.Width,desc.Height,0,f,GL_UNSIGNED_BYTE,lock.pBits);
			glTexSubImage2D(GL_TEXTURE_2D,lv,x,y,desc.Width,desc.Height,f,GL_UNSIGNED_BYTE,lock.pBits);

			pp->UnlockRect(); x/=2; y/=2; //BltAtlasTexture
		}
		else assert(0); 

		if(g) pp->Release();
	}
}
extern bool D3D9X::updating_texture(DDRAW::IDirectDrawSurface7 *p, int force)
{
	//HACK: there's a lot of D3DPOOL_SYSTEMMEM logic at the 
	//top of this routine that's not worth duplicating here
	if(!D3D9C::updating_texture(p,force)) return false;	
	else if(p->target=='dx9c') return true;

	assert(p->target=='dxGL'||p->target=='dx95');
	
	auto q = p->query9;

	dx_d3d9X_updating_texture2(p,0,0);
	
	//HACK: just reset glBindTexture 
	if(p!=DDRAW::TextureSurface7[0])
	DDRAW::Direct3DDevice7->SetTexture(0,DDRAW::TextureSurface7[0]);

	return true;
}

template<typename T>
static inline void dx_d3d9X_vsconstant_t(int reg, const T &set, int mode)
{
	auto p = DDRAW::Direct3DDevice7; if(reg<=0||!p) return;
	
	if(DDRAW::ff) return; //dx.d3d9c.cpp?

	const bool X = DDRAW::gl;

	HRESULT ok = !X; //debugging

	//2021
	//these aren't used internally, so instead they should be
	//set by a DDRAW::vset9 overload
	#ifdef _DEBUG 
	if(DDRAW::vsB&&reg>=DDRAW::vsB) //bool
	{
		assert(0); //REMOVE US?
	}
	else if(DDRAW::vsI&&reg>=DDRAW::vsI) //integer
	{
		//NOTE: dx_d3d9c_vslight calls SetVertexShaderConstantI
		//directly
		assert(0); //REMOVE US?
	}
	else //float4
	#endif
	{
		switch(mode)
		{
		case 'rgba': //D3DCOLOR
			
			if(sizeof(T)==sizeof(DWORD)&&std::is_integral<T>::value)
			{
				float col[4] = 
				{
					float(0xFF&(*(DWORD*)&set>>16))*0.003921568627f, //1/255
					float(0xFF&(*(DWORD*)&set>>8 ))*0.003921568627f,
					float(0xFF&(*(DWORD*)&set    ))*0.003921568627f,	
					float(0xFF&(*(DWORD*)&set>>24))*0.003921568627f 
				};
				if(X) p->queryGL->uniforms2[0].dirty(reg,col,1);
				else 
				ok = p->proxy9->SetVertexShaderConstantF(reg-DDRAW::vsF,col,1); 			
				break;
			}
			//break;		
		case 'xyzw': mode = 1; //courtesy		
		//case 1: //vector
		//case 4: //matrix
		default: assert((unsigned)mode<=32); //not in the ASCII range
		
			if(X) p->queryGL->uniforms2[0].dirty(reg,(float*)&set,mode);
			else
			ok = p->proxy9->SetVertexShaderConstantF(reg-DDRAW::vsF,(float*)&set,mode); 
			break;
		case 'xyz':
		case 'x': case 'y':	case 'z': case 'w': //component
		{
			float get[4]; if(X) 
			{
				auto &v = p->queryGL->uniforms2[0];
				unsigned s = reg-DDRAW::vsF;
				if(s>=(unsigned)v.fmost)
				{
					assert(0); //preallocated?
					memset(get,0x00,sizeof(get));
				}
				else memcpy(get,v.f+s*4,sizeof(get));
			}
			else ok = p->proxy9->GetVertexShaderConstantF(reg-DDRAW::vsF,get,1);

			if(mode>255)
			{
				assert(mode=='xyz');				
				get[0] = set; get[1] = *(&set+1); get[2] = *(&set+2);
			}
			else get[mode=='w'?3:mode-'x'] = set; //component

			if(X) p->queryGL->uniforms2[0].dirty(reg,get,1);
			else
			ok|=p->proxy9->SetVertexShaderConstantF(reg-DDRAW::vsF,get,1); 			
			break; 
		}}
	}

	assert(ok==D3D_OK);
}
extern void dx_d3d9X_vsconstant(int reg,const DX::D3DCOLORVALUE &set, int mode) //1
{
	return dx_d3d9X_vsconstant_t(reg,set.r,mode);
}
extern void dx_d3d9X_vsconstant(int reg, const D3DMATRIX &set, int mode) //4
{
	return dx_d3d9X_vsconstant_t(reg,set._11,mode);
}
extern void dx_d3d9X_vsconstant(int reg, const float &set, int mode)
{
	return dx_d3d9X_vsconstant_t(reg,set,mode);
}
extern void dx_d3d9X_vsconstant(int reg, const DWORD &set, int mode)
{
	return dx_d3d9X_vsconstant_t(reg,set,mode);
}
extern void dx_d3d9X_vsconstant(int reg, const float *set, int mode) //1
{
	return dx_d3d9X_vsconstant_t(reg,*set,mode);
}
extern void DDRAW::vset9(float *f, int registers, int c)
{	
	//dx_d3d9X_vsconstant(c,f,registers);

	if(auto*p=DDRAW::Direct3DDevice7) switch(p->target)
	{	
	case 'dx95': case 'dxGL': assert(DDRAW::gl);

		return p->queryGL->uniforms2[0].dirty(c,f,registers);

	case 'dx9c': assert(!DDRAW::gl);

		if(!DDRAW::ff) //dx.d3d9c.cpp?
		if(p->proxy9->SetVertexShaderConstantF(c-DDRAW::vsF,f,registers))
		assert(0); return;
	}
}
extern void DDRAW::vset9(int *i, int registers, int c)
{	
	auto p = DDRAW::Direct3DDevice7; switch(p?p->target:0)
	{	
	case 'dx95': case 'dxGL': assert(DDRAW::gl);

		p->queryGL->uniforms2[0].dirty(c,i,registers);
		default: return;

	case 'dx9c': assert(!DDRAW::gl); break;
	}
	if(DDRAW::ff) return; //dx.d3d9c.cpp?

	HRESULT ok = !D3D_OK; //debugging

	if(DDRAW::vsB&&c>=DDRAW::vsB) //bool
	{
		ok = p->proxy9->SetVertexShaderConstantB(c-DDRAW::vsB,i,registers); 
	}
	else if(DDRAW::vsI&&c>=DDRAW::vsI) //integer
	{
		ok = p->proxy9->SetVertexShaderConstantI(c-DDRAW::vsI,i,registers); 
	}
	assert(ok==D3D_OK);
}
template<typename T>
static inline void dx_d3d9X_psconstant_t(int reg, const T &set, int mode)
{
	auto p = DDRAW::Direct3DDevice7; if(!reg||!p) return;

	if(DDRAW::ff) return; //dx.d3d9c.cpp?
  
	const bool X = DDRAW::gl;

	HRESULT ok = !X; //debugging

	//2021
	//these aren't used internally, so instead they should be
	//set by a DDRAW::pset9 overload
	#ifdef _DEBUG 
	if(DDRAW::psB&&reg>=DDRAW::psB) //bool
	{
		assert(0); //REMOVE US?
	}
	else if(DDRAW::psI&&reg>=DDRAW::psI) //integer
	{
		//NOTE: dx_d3d9c_vslight calls SetVertexShaderConstantI
		//directly
		assert(0); //REMOVE US?
	}
	else //float4
	#endif
	{
		switch(mode)
		{
		case 'rgba': //D3DCOLOR
			
			if(sizeof(T)==sizeof(DWORD)&&std::is_integral<T>::value)
			{
				float col[4] = 
				{
					float(0xFF&(*(DWORD*)&set>>16))*0.003921568627f, //1/255
					float(0xFF&(*(DWORD*)&set>>8 ))*0.003921568627f,
					float(0xFF&(*(DWORD*)&set    ))*0.003921568627f,	
					float(0xFF&(*(DWORD*)&set>>24))*0.003921568627f 
				};
				if(X) p->queryGL->uniforms2[1].dirty(reg,col,1);
				else
				ok = p->proxy9->SetPixelShaderConstantF(reg-DDRAW::psF,col,1);
				break;
			}
			//break;		
		case 'xyzw': mode = 1; //courtesy		
		//case 1: //vector
		//case 4: //matrix
		default: assert((unsigned)mode<=32); //not in the ASCII range
		
			if(X) p->queryGL->uniforms2[1].dirty(reg,(float*)&set,mode);
			else
			ok = p->proxy9->SetPixelShaderConstantF(reg-DDRAW::psF,(float*)&set,mode); 
			break;
		case 'xyz':
		case 'x': case 'y':	case 'z': case 'w': //component
		{
			float get[4]; if(X) 
			{
				auto &v = p->queryGL->uniforms2[1];
				unsigned s = (reg-DDRAW::psF);
				if(s>=(unsigned)v.fmost)
				{
					assert(0); //preallocated?
					memset(get,0x00,sizeof(get));
				}
				else memcpy(get,v.f+s*4,sizeof(get));
			}
			else ok = p->proxy9->GetPixelShaderConstantF(reg-DDRAW::psF,get,1);

			if(mode>255)
			{
				assert(mode=='xyz');				
				get[0] = set; get[1] = *(&set+1); get[2] = *(&set+2);
			}
			else get[mode=='w'?3:mode-'x'] = set; //component

			if(X) p->queryGL->uniforms2[1].dirty(reg,get,1);
			else
			ok|=p->proxy9->SetPixelShaderConstantF(reg-DDRAW::psF,get,1); 			
			break; 
		}}
	}

	assert(ok==D3D_OK);
}
extern void dx_d3d9X_psconstant(int reg, const float &set, int mode)
{
	return dx_d3d9X_psconstant_t(reg,set,mode);
}
extern void dx_d3d9X_psconstant(int reg, const DWORD &set, int mode)
{
	return dx_d3d9X_psconstant_t(reg,set,mode);
}
extern void dx_d3d9X_psconstant(int reg, const float *set, int mode) //1
{
	return dx_d3d9X_psconstant_t(reg,*set,mode);
}
extern void DDRAW::pset9(float *f, int registers, int c)
{	
	//dx_d3d9X_psconstant(c,f,registers);

	if(auto*p=DDRAW::Direct3DDevice7) switch(p->target)
	{	
	case 'dx95': case 'dxGL': assert(DDRAW::gl);

		return p->queryGL->uniforms2[1].dirty(c,f,registers);

	case 'dx9c': assert(!DDRAW::gl);

		if(!DDRAW::ff) //dx.d3d9c.cpp?
		if(p->proxy9->SetPixelShaderConstantF(c-DDRAW::psF,f,registers))
		assert(0); return;
	}	
}
extern void DDRAW::pset9(int *i, int registers, int c)
{	
	auto p = DDRAW::Direct3DDevice7; switch(p?p->target:0)
	{	
	case 'dx95': case 'dxGL': assert(DDRAW::gl);

		p->queryGL->uniforms2[1].dirty(c,i,registers);
		default: return;

	case 'dx9c': assert(!DDRAW::gl); break;
	}
	if(DDRAW::ff) return; //dx.d3d9c.cpp?

	HRESULT ok = !D3D_OK; //debugging

	if(DDRAW::psB&&c>=DDRAW::psB) //bool
	{
		//NOTE: apparently 1-dimensional (unlike float/int registers)
		ok = p->proxy9->SetPixelShaderConstantB(c-DDRAW::psB,i,registers); 
	}
	else if(DDRAW::psI&&c>=DDRAW::psI) //integer
	{
		ok = p->proxy9->SetPixelShaderConstantI(c-DDRAW::psI,i,registers); 
	}
	assert(ok==D3D_OK);
}
void DDRAW::IDirect3DDevice::QueryGL::UniformBlock::dirty(int c, int *p, int n)
{
	if(DDRAW::vsI&&c>=DDRAW::vsI) //integer
	{
		c-=DDRAW::vsI; assert(c+n<=fmost); //assert(c<16);

		c*=4; n*=4; 

		if(!memcmp(i+c,p,n*sizeof(int))) //4
		return;
		
		dirty_mask|=c>=chunk?4:c+n<chunk?2:2|4;
	}
	/*else if(DDRAW::vsB&&c>=DDRAW::vsB) //bool
	{
		c-=DDRAW::vsB; assert(c<16);
		
		dirty_mask|=1;
	}*/
	else{ assert(0); return; }

	memcpy(i+c,p,n*sizeof(int)); //4
}
void DDRAW::IDirect3DDevice::QueryGL::UniformBlock::dirty(int c, float *p, int n)
{
	if(c>=DDRAW::vsF) //float
	{
		c-=DDRAW::vsF; assert(c+n<=fmost); //assert(c<224);

		c*=4; n*=4;

		if(!memcmp(f+c,p,n*sizeof(float))) //4
		return;

		//bits 1~3 are for bool/int
		//bits are in groups of 8 (256/32)
		int rem = c%chunk;
		int m = 3+c/chunk, mm = (rem+n)/chunk;
		do{ dirty_mask|=1<<m++; }while(mm-->0);		
	}
	else{ assert(0); return; } 
	
	memcpy(f+c,p,n*sizeof(float)); //4
}
void DDRAW::IDirect3DDevice::QueryGL::UniformBlock::dirty() //UNUSED?
{
	//EXPERIMENTAL
	//just mark everything dirty without updating to
	//sync them on next the draw
	int c = 0, n = fmost;

//	if(c>=DDRAW::vsF) //float
	{
//		c-=DDRAW::vsF; assert(c<fmost); //assert(c<224);

		c*=4; n*=4;

//		if(!memcmp(f+c,p,n*sizeof(float))) //4
//		return;

		//bits 1~3 are for bool/int
		//bits are in groups of 8 (256/32)
		int rem = c%chunk;
		int m = 3+c/chunk, mm = (rem+n)/chunk;
		do{ dirty_mask|=1<<m++; }while(mm-->0);		
	}
//	else{ assert(0); return; } 
	
//	memcpy(f+c,p,n*sizeof(float)); //4
}
void DDRAW::IDirect3DDevice::QueryGL::UniformBlock::flush(int ubo)
{
	int m = dirty_mask; dirty_mask = 0;
	
	glBindBuffer(GL_UNIFORM_BUFFER,ubo);

	enum{ l6=0 }; //16 //bool? //UNIFORM_BUFFER_OFFSET_ALIGNMENT

	int os,sz; if(m&7) //int?
	{
		//there's 2.5 chunks here. 2 int chunks and 
		//a bool half chunk (at 0 offset)

		int *p = i; //i.data();

		switch(m&7) //2 is the only likely scenario
		{
		case 1: os = 0; sz = l6; assert(0); break; //bool?
		case 3: os = 0; sz = l6+chunk; break; //bool+low int?
		case 2: os = l6; sz = chunk; break; //low int?
		default: os = 0; sz = l6+chunk+chunk; break; 
		}
		
		//not trying to merge with float chunks
		glBufferSubData(GL_UNIFORM_BUFFER,os*4,sz*4,p+os);
	}

	//NOTE: from what I've read online small updates might be
	//fine, but I don't trust it for reading from the buffers
	//when dx_d3d9X_vs/psconstant_t update a single component

	os = l6+chunk+chunk; sz = 0; 

	float *p = f-os;

	for(m>>=3;m;m>>=1)
	{
		if(m&1)
		{
			sz+=chunk; continue;
		}
		else if(!sz) 
		{
			os+=chunk; continue;
		}
		
		glBufferSubData(GL_UNIFORM_BUFFER,os*4,sz*4,p+os);

		os+=sz+chunk; sz = 0;
	}
	if(sz) glBufferSubData(GL_UNIFORM_BUFFER,os*4,sz*4,p+os);
}		

extern void dx_d3d9X_resetdevice() //dx.d3d9c.cpp
{
	const bool X = DDRAW::gl;

	dx_d3d9c_vstransform(~0);

	if(dx_d3d9c_bltstate) //dx_d3d9X_bltstate?
	{
		if(!X) dx_d3d9c_bltstate->Release();

		dx_d3d9c_bltstate = 0;
	}	

	//2018: stereo/dx_d3d9c_immediate_mode (PlayStation VR)
	if(!X) for(int i=D3D9C::ibuffersN;i-->0;)
	{
		if(auto*x=dx_d3d9c_ibuffers[i]) x->Release(); 
	}
	else glDeleteBuffers(D3D9C::ibuffersN,dx_d3d9X_ibuffers);
	memset(dx_d3d9c_ibuffers,0x00,sizeof(dx_d3d9c_ibuffers));
	memset(dx_d3d9c_ibuffers_s,0x00,sizeof(dx_d3d9c_ibuffers_s));
	if(!X) for(int i=D3D9C::ibuffersN;i-->0;)
	{
		if(auto*x=dx_d3d9c_vbuffers[i]) x->Release();
	}
	else glDeleteBuffers(D3D9C::ibuffersN,dx_d3d9X_vbuffers);
	memset(dx_d3d9c_vbuffers,0x00,sizeof(dx_d3d9c_vbuffers));
	memset(dx_d3d9c_vbuffers_s,0x00,sizeof(dx_d3d9c_vbuffers_s));
	
	if(X)
	{
		auto &va = dx_d3d9X_varrays;		
		for(auto i=va.size();i-->0;)
		{
			GLuint vao = va[i]>>32;
			glDeleteVertexArrays(1,&vao);
		}
		va.clear();
	}
	else dx_d3d9c_stereoVD.clear_and_Release();
	
	dx_d3d9c_effectsshader_enable = false;

	dx_d3d9X_release_shaders(X); //dx_d3d9X_pipelines
		
	if(dx_d3d9c_alphasurface) dx_d3d9c_alphasurface->Release();
	if(dx_d3d9c_alphatexture) dx_d3d9c_alphatexture->Release();
	dx_d3d9c_alphasurface = 0; dx_d3d9c_alphatexture = 0;

	dx_d3d9c_anisotropy_max = 0; 

	//REMOVE US
	//dx_d3d9c_rendertarget_pitch = 0;
	//dx_d3d9c_rendertarget_format = D3DFMT_UNKNOWN;
	DDRAW::mrtargets9[0] = D3DFMT_UNKNOWN;

	dx_d3d9c_beginscene = dx_d3d9c_endscene = 0;

	DDRAW::inScene = false;

//	dx_d3d9c_alphablendenable = false; //???
	dx_d3d9c_colorkeyenable = true;

	for(int i=0;i<8;i++) DDRAW::TextureSurface7[i] = 0;

	/*EXPERIMENTAL
	if(auto*d12=DDRAW::D3D12Device)
	{
		DDRAW::D3D12Device = 0;
		auto &h12 = DDRAW::doD3D12_backbuffers;
		for(int i=0;i<DX_ARRAYSIZEOF(h12)-1;i++)
		if(h12[i].nt_handle)
		CloseHandle(h12[i].nt_handle);
		d12->Release();		
	}*/
	assert(!DDRAW::D3D12Device);
}

void DDRAW::IDirectDrawSurface::QueryX::_updateGL_delete()
{
	int compile[sizeof(void*)==sizeof(GLint)]; (void)compile;

	if(updateGL) glDeleteTextures(1,&updateGL); updateGL = 0;
}
void DDRAW::IDirect3DVertexBuffer::QueryX::_vboGL_delete()
{
	if(vboGL) glDeleteBuffers(1,&vboGL); vboGL = 0;
}
void DDRAW::IDirect3DDevice::QueryX::_delete_operatorGL()
{
	auto &gl = *(QueryGL*)this;

	//HACK: really just need to zero this memory?!?!

	auto &fx = *(Widgets95::xr::effects*)gl.effects;
	if(gl.xr) for(int i=4;i-->0;)
	if(auto&t=fx.gl_bind_texture[i])
	fx.reset_texture(gl.xr->first_surface(),i,nullptr);

	glDeleteTextures(DX_ARRAYSIZEOF(gl.textures),gl.textures);
	glDeleteRenderbuffers(1,&gl.depthstencil);
	glDeleteFramebuffers(2,&*gl.framebuffers); //1
	glDeleteSamplers(DX_ARRAYSIZEOF(gl.samplers),gl.samplers);
	glDeleteSamplers(DX_ARRAYSIZEOF(gl.uniforms),gl.uniforms);
	delete[] gl.uniforms2;

	if(gl.stereo) glDeleteBuffers(1,&gl.stereo);

	if(gl.stereo_fovea) glDeleteTextures(1,&gl.stereo_fovea);

	delete gl.xr; 
	if(gl.wgl) ReleaseDC(DDRAW::window,gl.wgldc); //wglGetCurrentDC()
	if(gl.wgl) wglDeleteContext(gl.wgl);
}
void DDRAW::IDirect3DDevice::QueryGL::StateBlock::apply_nanovg()
{
	//HACK: this is a state sync helper for Exselector->begin
	//it's mainly because OpenGL ES has no state management?!
	//https://github.com/memononen/nanovg#opengl-state-touched-by-the-backend
	
	//glUseProgram(prog);
	dx_d3d9X_pipeline = 0; //dirty cache

	//glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	srcblend = D3DBLEND_SRCALPHA;
	destblend = D3DBLEND_INVSRCALPHA;
	/*
	//GL_CULL_FACE is suspended and restored
	//(draw order could be changed instead)
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
	//glFrontFace(GL_CCW);	
	cullmode = D3DCULL_CCW;
	*/
	assert(cullmode>1);
	//glDisable(GL_CULL_FACE); //finally?
//	cullmode = D3DCULL_NONE;
	//glEnable(GL_BLEND);
	alphablendenable = 1;
	//glDisable(GL_DEPTH_TEST);	
//	zwriteenable = 0;

	//see som_hacks_early_disable_MRT (som.hacks.cpp)
	//assert(8==colorwriteenable);

	//glBindTexture(GL_TEXTURE_2D,0);
	if(DDRAW::TextureSurface7[0])
	{
		//dx_d3d9X_psconstant(DDRAW::psColorkey,0.0f,'w');
		DDRAW::TextureSurface7[0] = 0;
	}
}
extern void DDRAW::xr_depth_clamp(bool e)
{
	if(DDRAW::xr) (e?glEnable:glDisable)(GL_DEPTH_CLAMP);
	else assert(0);
}
static bool dx_d3d9X_SEPARATEALPHABLENDENABLE_00 = false;
void DDRAW::IDirect3DDevice::QueryGL::StateBlock::apply(const StateBlock &cmp)const
{
	//NOTE: I'd like some methods to set these directly
	//I just don't want to add to the methods right now

	StateBlock cp = *this;

	if(cp.sampler_mask!=cmp.sampler_mask)
	{
		int sam0 = DDRAW::Direct3DDevice7->queryGL->samplers[0];

		if(cp.minfilter!=cmp.minfilter
		 ||cp.mipfilter!=cmp.mipfilter)
		{
			//bool linear = cp.minfilter==D3DTEXF_LINEAR;
			bool linear = cp.minfilter!=0;
			GLint minf = linear?GL_LINEAR:GL_NEAREST;
			switch(cp.mipfilter)
			{
			case D3DTEXF_LINEAR: minf = GL_NEAREST_MIPMAP_LINEAR+linear; break;
			case D3DTEXF_POINT: minf = GL_NEAREST_MIPMAP_NEAREST+linear; break;
			}
			glSamplerParameteri(sam0,GL_TEXTURE_MIN_FILTER,minf);
		}
		if(cp.magfilter!=cmp.magfilter)
		{
			glSamplerParameteri(sam0,GL_TEXTURE_MAG_FILTER,cp.magfilter?GL_LINEAR:GL_NEAREST);
		}			
		static const int uv[4] = {GL_REPEAT,GL_REPEAT,GL_MIRRORED_REPEAT,GL_CLAMP_TO_EDGE};
		if(cp.addressu!=cmp.addressu)
		{
			glSamplerParameteri(sam0,GL_TEXTURE_WRAP_S,uv[cp.addressu]);
		}
		if(cp.addressv!=cmp.addressv)
		{
			glSamplerParameteri(sam0,GL_TEXTURE_WRAP_T,uv[cp.addressv]);
		}
		cp.sampler_mask = cmp.sampler_mask; //OPTIMIZING?
		if(cp.dword==cmp.dword) return;
	}
	if(cp.blend_mask!=cmp.blend_mask)
	{
		if(cp.alphablendenable!=cmp.alphablendenable)
		{
			(cp.alphablendenable?glEnable:glDisable)(0x0BE2); //GL_BLEND
		}
		if(cp.srcblend!=cmp.srcblend||cp.destblend!=cmp.destblend)
		{
			//this may be everything OpenGL ES supports
			#define GL_ZERO                           0
			#define GL_ONE                            1
			#define GL_SRC_COLOR                      0x0300
			#define GL_ONE_MINUS_SRC_COLOR            0x0301
			#define GL_SRC_ALPHA                      0x0302
			#define GL_ONE_MINUS_SRC_ALPHA            0x0303
			#define GL_DST_ALPHA                      0x0304
			#define GL_ONE_MINUS_DST_ALPHA            0x0305
			#define GL_DST_COLOR                      0x0306
			#define GL_ONE_MINUS_DST_COLOR            0x0307
			#define GL_SRC_ALPHA_SATURATE             0x0308

			GLenum s,d;
			//MSDN sas these are valid (may be old/bad advice)
			//GL_ZERO, GL_ONE, GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR,
			//GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, 
			//GL_ONE_MINUS_DST_ALPHA, and GL_SRC_ALPHA_SATURATE
			switch(cp.srcblend)
			{
			default: assert(0); //break;
			case D3DBLEND_ONE: s = GL_ONE; break;
			case D3DBLEND_ZERO: s = GL_ZERO; break;
			//case D3DBLEND_SRCCOLOR: s = GL_SRC_COLOR; break; //valid?
			//case D3DBLEND_INVSRCCOLOR: s = GL_ONE_MINUS_SRC_COLOR; break; //valid?
			case D3DBLEND_SRCALPHA: s = GL_SRC_ALPHA; break;
			case D3DBLEND_INVSRCALPHA: s = GL_ONE_MINUS_SRC_ALPHA; break;
			case D3DBLEND_DESTALPHA: s = GL_DST_ALPHA; break;
			case D3DBLEND_INVDESTALPHA: s = GL_ONE_MINUS_DST_ALPHA; break;
			case D3DBLEND_DESTCOLOR: s = GL_DST_COLOR; break;
			case D3DBLEND_INVDESTCOLOR: s = GL_ONE_MINUS_DST_COLOR; break;
			case D3DBLEND_SRCALPHASAT: s = GL_SRC_ALPHA_SATURATE; break;
				//obsolete since d3d6? D3DBLEND_SRCALPHA/D3DBLEND_INVSRCALPHA 
			//case D3DBLEND_BOTHSRCALPHA: s = GL_SRC_ALPHA; break; //goto?
				//no deprecation note? D3DBLEND_INVSRCALPHA/D3DBLEND_SRCALPHA 
			//case D3DBLEND_BOTHINVSRCALPHA: s = GL_ONE_MINUS_SRC_ALPHA; break; //goto?
			};	
			//MSDN sas these are valid (may be old/bad advice)
			//GL_ZERO, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, 
			//GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, and GL_ONE_MINUS_DST_ALPHA
			switch(cp.destblend)
			{
			default: assert(0); //break;			
			case D3DBLEND_ZERO: d = GL_ZERO; break;
			case D3DBLEND_ONE: d = GL_ONE; break;
			case D3DBLEND_SRCCOLOR: d = GL_SRC_COLOR; break; 
			case D3DBLEND_INVSRCCOLOR: d = GL_ONE_MINUS_SRC_ALPHA; break; 
			case D3DBLEND_SRCALPHA: d = GL_SRC_ALPHA; break;
			case D3DBLEND_INVSRCALPHA: d = GL_ONE_MINUS_SRC_ALPHA; break;
			case D3DBLEND_DESTALPHA: d = GL_DST_ALPHA; break;
			case D3DBLEND_INVDESTALPHA: d = GL_ONE_MINUS_DST_ALPHA; break;
			//case D3DBLEND_DESTCOLOR: d = GL_DST_COLOR; break; //valid?
			//case D3DBLEND_INVDESTCOLOR: d = GL_ONE_MINUS_DST_COLOR; break; //valid?
			//case D3DBLEND_SRCALPHASAT: d = GL_SRC_ALPHA_SATURATE; break; //valid?
				//obsolete since d3d6? D3DBLEND_SRCALPHA/D3DBLEND_INVSRCALPHA 
			//case D3DBLEND_BOTHSRCALPHA: d = GL_ONE_MINUS_SRC_ALPHA; break; //goto?
				//no deprecation note? D3DBLEND_INVSRCALPHA/D3DBLEND_SRCALPHA 
			//case D3DBLEND_BOTHINVSRCALPHA: d = GL_SRC_ALPHA; break; //goto?
			};	
			if(dx_d3d9X_SEPARATEALPHABLENDENABLE_00) //2024: som_scene_swing?
			{			
				glBlendFuncSeparate(s,d,GL_ZERO,GL_ZERO);
			}
			else glBlendFunc(s,d);
		}
		cp.blend_mask = cmp.blend_mask; //OPTIMIZING?
		if(cp.dword==cmp.dword) return;
	}
	if(cp.z_mask!=cmp.z_mask)
	{
		if(cp.zenable!=cmp.zenable)
		{
			(cp.zenable?glEnable:glDisable)(GL_DEPTH_TEST);
		}
		if(cp.zwriteenable!=cmp.zwriteenable)
		{
			glDepthMask(cp.zwriteenable); 
		}
		if(cp.zfunc!=cmp.zfunc)
		{
			/*these map neatly
			int f; switch(cp.zfunc)
			{
			case D3DCMP_NEVER: f = GL_NEVER; break; //1->0x0200
			case D3DCMP_LESS: f = GL_LESS; break;
			case D3DCMP_EQUAL: f = GL_EQUAL; break;
			case D3DCMP_LESSEQUAL: f = GL_LEQUAL; break;
			case D3DCMP_GREATER: f = GL_GREATER; break;
			case D3DCMP_NOTEQUAL: f = GL_NOTEQUAL; break;
			case D3DCMP_GREATEREQUAL: f = GL_GEQUAL; break;
			case D3DCMP_ALWAYS: f = GL_ALWAYS; break; //8->0x0207
			}*/
			glDepthFunc(0x0200-1+cp.zfunc); //f
		}
		cp.z_mask = cmp.z_mask; //OPTIMIZING?
		if(cp.dword==cmp.dword) return;
	}	
	if(cp.rare_mask!=cmp.rare_mask)
	{
		if(int diff=cp.colorwriteenable^cmp.colorwriteenable)
		{
			if(diff&8)
			{
				bool f = cp.colorwriteenable&8?1:0;
				//glColorMask(f,f,f,f);
				glColorMaski(0,f,f,f,f);
			}
			if(diff&7)
			{
				auto &mrt = DDRAW::Direct3DDevice7->queryGL->mrtargets;
				GLenum bufs[4] = {GL_COLOR_ATTACHMENT0};
				int i; for(i=0;i<3;i++)
				{
					if(!DDRAW::mrtargets9[i+1]) break;

					//NOTE: GL_NONE matches SetRenderTarget's
					//behavior. glClearBufferfv would need to
					//change as well if there's a concern for
					//having holes, and if clients need holes
					bool on = (cp.colorwriteenable&(1<<i))!=0;
					bufs[1+i] = on?GL_COLOR_ATTACHMENT0+1+i:0; //GL_NONE
					glActiveTexture(GL_TEXTURE0+DDRAW_TEX0+i);
					glBindTexture(GL_TEXTURE_2D,on?0:mrt[i]);
				}
				glActiveTexture(GL_TEXTURE0);
				//ANGLE throws an error here, maybe because there are
				//not attachments for the GL_NONE fields?
				//kFramebufferIncompleteAttachmentNotRenderable
				//glDrawBuffers(4,bufs);
				glDrawBuffers(1+i,bufs);
			}
			cp.colorwriteenable = cmp.colorwriteenable; //OPTIMIZING?
			if(cp.dword==cmp.dword) return;
		}
		if(cp.cullmode!=cmp.cullmode)
		{
			if(cp.cullmode>1) //NONE is 1
			{
				glCullFace(0x0404+(cp.cullmode==D3DCULL_CCW)); //GL_FRONT//GL_BACK
				glEnable(0x0B44); //GL_CULL_FACE
			}
			else glDisable(0x0B44); //GL_CULL_FACE
		}
		if(cp.fillmode!=cmp.fillmode)
		{
			//TODO: I think a "geometry shader" could fulfill this
			//would be an interesting experiment since I've never
			//used one. the following code is advised by this link
			//Widgets95::gl would work but is bound to be too slow
			//https://stackoverflow.com/questions/3539205/is-there-a-substitute-for-glpolygonmode-in-open-gl-es-webgl
			/*
			layout(triangles) in;
			layout(line_strip, max_vertices=4) out;
			void main()
			{
				for(int i=0; i<4; ++i)
				{
					// and whatever other attributes of course
					gl_Position = gl_in[i%3].gl_Position;
					EmitVertex();
				}
			}
			*/
			//OpenGL ES doesn't support this
			//#define GL_POINT                          0x1B00
			//#define GL_LINE                           0x1B01
			//#define GL_FILL                           0x1B02
			//#define GL_FRONT_AND_BACK                 0x0408
			glPolygonMode(0x0408,0x1B01+cp.fillmode); //GL_LINE/GL_FILL
		}
		//I think I remember this looking really bad with Direct3D
		//I'm pretty sure nothing uses it
		if(cp.shademode!=cmp.shademode) //UNUSED
		{
			assert(0);

			//OpenGL ES doesn't support this
			glShadeModel(0x1D00+cp.shademode); //GL_FLAT/GL_SMOOTH
		}
	}
}

extern void dx_d3d9X_SEPARATEALPHABLENDENABLE_knockout(bool e) //2024
{
	auto *d = DDRAW::Direct3DDevice7;

	dx_d3d9X_SEPARATEALPHABLENDENABLE_00 = e;

	if(DDRAW::gl) //call glBlendFuncSeparate
	{
		auto &st = d->queryGL->state, cmp = st;

		cmp.blend_mask = ~st.blend_mask; st.apply(cmp);
	}
	else if(DDRAW::compat=='dx9c')
	{
		//note: separate still writes to alpha when not blending
		d->proxy9->SetRenderState(D3DRS_COLORWRITEENABLE,e?0xf:7);
		//callers must do this through DDRAW::Direct3DDevice7
		//d->SetRenderState(DX::D3DRENDERSTATE_ALPHABLENDENABLE,e); //YUCK
		d->proxy9->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,e);
		d->proxy9->SetRenderState(D3DRS_SRCBLENDALPHA,D3DBLEND_ZERO);
		d->proxy9->SetRenderState(D3DRS_DESTBLENDALPHA,e?D3DBLEND_ZERO:D3DBLEND_DESTALPHA);
	}
}
