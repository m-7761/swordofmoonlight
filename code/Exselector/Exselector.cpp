		
#include "Exselector.pch.h" //PCH

#include <map>

#include "../SomEx/Ex.h"
#include "../SomEx/SomEx.ini.h"

//TEMPORARY
//these just repackage dllexport defintions
#undef interface
#include <widgets95.h>
//NOTE: have to fake call JslConnectDevices 
//in DllMain for some reason to export this
#include "JoyShockLibrary/JoyShockLibrary/JoyShockLibrary.h"

static HMODULE Exselector_hmodule = 0;

extern HMODULE Exselector_dll()
{
	return Exselector_hmodule;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
		//HACK: for some reason something 
		//like /OPT:REF is in play unless
		//the LIB is "used"
		if(0) JslConnectDevices();

	if(ul_reason_for_call==DLL_PROCESS_ATTACH)
	{	
		Exselector_hmodule = hModule;

		//Somfolder(0); //hack: initialize temporary folder
	}
	else if(ul_reason_for_call==DLL_PROCESS_DETACH)
	{

	}

	return TRUE;
}

static class Exselector Exselector_cpp;
extern class Exselector*const Exselector = &Exselector_cpp;
class Exselector *Sword_of_Moonlight_Extension_Selector(BOOL system)
{
	return &Exselector_cpp;
}

	////// TODO: MOVE TO OWN FILE //////

#define GL_NO_ERROR				0
#define GL_INVALID_ENUM			0x050
#define GL_FALSE				0
#define GL_TRUE					1
#define GL_POINTS				0x0000
#define GL_LINES				0x0001
#define GL_LINE_LOOP			0x0002
#define GL_LINE_STRIP			0x0003
#define GL_TRIANGLES			0x0004
#define GL_TRIANGLE_STRIP		0x0005
#define GL_TRIANGLE_FAN			0x0006
#define GL_ZERO					0
#define GL_ONE					1
#define GL_SRC_COLOR			0x0300
#define GL_ONE_MINUS_SRC_COLOR	0x0301
#define GL_SRC_ALPHA			0x0302
#define GL_ONE_MINUS_SRC_ALPHA	0x0303
#define GL_DST_ALPHA			0x0304
#define GL_ONE_MINUS_DST_ALPHA	0x0305
#define GL_DST_COLOR			0x0306
#define GL_ONE_MINUS_DST_COLOR	0x0307
#define GL_SRC_ALPHA_SATURATE	0x0308
#define GL_TEXTURE_2D			0x0DE1
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
#define GL_UNPACK_ALIGNMENT		0x0CF5
#define GL_UNPACK_ROW_LENGTH	0x0CF2 //???
#define GL_UNPACK_SKIP_ROWS		0x0CF3 //???
#define GL_UNPACK_SKIP_PIXELS	0x0CF4 //???
#define GL_RGBA					0x1908
#define GL_UNSIGNED_BYTE		0x1401
#define GL_FLOAT				0x1406
#define GL_R8					0x8229
#define GL_RED					0x1903
#define GL_BLEND				0x0BE2
#define GL_CULL_FACE			0x0B44
#define GL_CW					0x0900
#define GL_CCW					0x0901
#define GL_STENCIL_TEST			0x0B90
#define GL_DEPTH_TEST			0x0B71
#define GL_SCISSOR_TEST			0x0C11
#define GL_EQUAL				0x0202
#define GL_NOTEQUAL				0x0205
#define GL_ALWAYS				0x0207
#define GL_FRONT				0x0404
#define GL_BACK					0x0405
#define GL_KEEP					0x1E00
#define GL_REPLACE				0x1E01
#define GL_INCR					0x1E02
#define GL_DECR					0x1E03
#define GL_INVERT				0x150A
#define GL_INCR_WRAP			0x8507
#define GL_DECR_WRAP			0x8508
#define GL_TEXTURE0				0x84C0
#define GL_ARRAY_BUFFER			0x8892
#define GL_UNIFORM_BUFFER		0x8A11
#define GL_STREAM_DRAW			0x88E0
#define GL_FRAGMENT_SHADER		0x8B30
#define GL_VERTEX_SHADER		0x8B31
#define GL_COMPILE_STATUS		0x8B81
#define GL_LINK_STATUS			0x8B82
#define GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT	0x8A34

#define EXSELECTOR_GL_PROCS_X \
X(glActiveTexture)\
X(glAttachShader)\
X(glBindAttribLocation)/*???*/\
X(glBindBuffer)\
X(glBindBufferRange)\
X(glBindSampler)\
X(glBindTexture)\
X(glBindVertexArray)\
X(glBindVertexBuffer)\
X(glBlendFunc)\
X(glBufferData)\
X(glBufferSubData)\
X(glColorMask)\
X(glCompileShader)\
X(glCreateProgram)\
X(glCreateShader)\
X(glCreateShaderProgramv)\
X(glCullFace)\
X(glDeleteBuffers)\
X(glDeleteProgram)\
X(glDeleteShader)\
X(glDeleteTextures)\
X(glDeleteVertexArrays)\
X(glDepthFunc)\
X(glDepthMask)\
X(glDepthRangef)\
X(glDisable)\
X(glDrawArrays)\
X(glDrawBuffers)\
X(glDrawRangeElements)\
X(glEnable)\
X(glEnableVertexAttribArray)\
X(glFrontFace)\
X(glGenBuffers)\
X(glGenerateMipmap)\
X(glGenTextures)\
X(glGenVertexArrays)\
X(glGetError)\
X(glGetFloatv)\
X(glGetIntegerv)\
X(glGetProgramiv)\
X(glGetProgramInfoLog)\
X(glGetShaderiv)\
X(glGetShaderInfoLog)\
X(glGetString)\
X(glGetUniformBlockIndex)/*???*/\
X(glGetUniformLocation)/*???*/\
X(glLinkProgram)\
X(glPixelStorei)\
X(glScissor)\
X(glShaderSource)\
X(glStencilFunc)\
X(glStencilMask)\
X(glStencilOp)\
X(glStencilOpSeparate)\
X(glTexImage2D)\
X(glTexParameteri)\
X(glTexStorage2D)\
X(glTexSubImage2D)\
X(glUseProgram)\
X(glUniform1i)\
X(glUniform2fv)\
X(glUniformBlockBinding)/*???*/\
X(glVertexAttribBinding)\
X(glVertexAttribFormat)\
X(glVertexAttribPointer)/*???*/\
X(glViewport)\
//
#define X(f) static auto *f = 0?Widgets95::gle::f:0;
EXSELECTOR_GL_PROCS_X

#define glnvg__checkError(...) //glDebugMessageCallback?
#define NANOVG_GLES3_IMPLEMENTATION
#include "src/nanovg/nanovg_gl.h"

//SUBROUTINE
//NOTE: this will eventually be needed to make
//fonts for for the mediabar
static int Exselector_ttf(HFONT h, NVGcontext *_nvgc)
{
	if(!h){ assert(0); return 0; }
	LOGFONTA l; GetObjectA(h,sizeof(l),&l);

	int f = nvgFindFont(_nvgc,l.lfFaceName);
	if(f!=-1) return f<<24|l.lfHeight;

	HDC hdc = CreateCompatibleDC(0);
	HGDIOBJ old = SelectObject(hdc,h);
	//from HFONT to TTF image
	//http://archives.miloush.net/michkap/archive/2007/07/07/3746794.html
	{
		//#include <stdio.h>
		//#include <windows.h>

		//NOTE: TTF was developed at Apple, so is BE
		auto ReadUshort = [](BYTE *p)
		{
			return ((USHORT)p[0]<<8)+((USHORT)p[1]); 
		};
		auto ReadDword = [](BYTE* p)
		{
			return ((LONG)p[0]<<24)+
			((LONG)p[1]<<16)+((LONG)p[2]<<8)+((LONG)p[3]);
		};
		auto ReadTag = [](BYTE *p)
		{
			return ((LONG)p[3]<<24)+
			((LONG)p[2]<<16)+((LONG)p[1]<<8)+((LONG)p[0]);
		};
		auto WriteDword = [](BYTE *p, DWORD dw)
		{
			p[0] = (BYTE)((dw>>24)&0xFF);
			p[1] = (BYTE)((dw>>16)&0xFF);
			p[2] = (BYTE)((dw>>8)&0xFF);
			p[3] = (BYTE)((dw)&0xFF);
		};
		auto RoundUpToDword = [](DWORD val)
		{
			return (val+3)&~3;
		};

		//#define TTC_FILE 0x66637474

		static const DWORD SizeOfFixedHeader       = 12;
		static const DWORD OffsetOfTableCount      = 4;
		static const DWORD SizeOfTableEntry        = 16;
		static const DWORD OffsetOfTableTag        = 0;
		static const DWORD OffsetOfTableChecksum   = 4;
		static const DWORD OffsetOfTableOffset     = 8;
		static const DWORD OffsetOfTableLength     = 12;

		DWORD pcbFontDataSize = NULL;
		BYTE *ppvFontData = 0;		
		/*HRESULT ExtractFontDataFromTTC(HDC hdc, 
		DWORD *pcbFontDataSize, void **ppvFontData)*/
		{
			//*ppvFontData = NULL;
			//*pcbFontDataSize = 0;

			// Check if font is really in ttc
		//	if(GetFontData(hdc,TTC_FILE,0,NULL,0)==GDI_ERROR)
			{
		//		goto ttf; //return GetLastError();
			}

			// 1. Read number of tables in the font (ushort value at offset 2)

			USHORT nTables;
			BYTE UshortBuf[2];
			if(GetFontData(hdc,0,4,UshortBuf,2)==GDI_ERROR)
			{
				goto err; //return GetLastError();
			}
			nTables = ReadUshort(UshortBuf);

			// 2. Calculate memory needed for the whole font header and read it into buffer

			DWORD cbHeaderSize = SizeOfFixedHeader+nTables*SizeOfTableEntry;
			BYTE *pbFontHeader = new BYTE[cbHeaderSize];

			if(GetFontData(hdc,0,0,pbFontHeader,cbHeaderSize)==GDI_ERROR)
			{
				delete[] pbFontHeader; goto err;
			}

			// 3. Go through tables and calculate total font size.
			//    Don't forget that tables should be padded to 4-byte
			//    boundaries, so length should be rounded up to dword.

			DWORD cbFontSize = cbHeaderSize;

			for(int i=0;i<nTables;i++)
			{
				DWORD cbTableLength = 
				ReadDword(pbFontHeader+
				SizeOfFixedHeader+i*SizeOfTableEntry+OffsetOfTableLength);
				if(i<nTables-1)
				cbFontSize += RoundUpToDword(cbTableLength);
				else
				cbFontSize += cbTableLength;
			}

			// 4. Copying header into target buffer. Offsets are incorrect,
			//    we will patch them with correct values while copying data.

			BYTE *pbFontData = new BYTE[cbFontSize];
			memcpy(pbFontData,pbFontHeader,cbHeaderSize);

			// 5. Get table data from GDI, write it into known place
			//    inside target buffer and fix offset value.

			DWORD dwRunningOffset = cbHeaderSize;

			for(int i=0;i<nTables;i++)
			{
				BYTE *pEntryData = pbFontHeader+
				SizeOfFixedHeader+i*SizeOfTableEntry;

				DWORD dwTableTag = ReadTag(pEntryData+OffsetOfTableTag);
				DWORD cbTableLength = ReadDword(pEntryData+OffsetOfTableLength);

				// Write new offset for this table.
				WriteDword(pbFontData+
				SizeOfFixedHeader+i*SizeOfTableEntry+OffsetOfTableOffset,dwRunningOffset);

				//Get font data from GDI and place it into target buffer
				if(GetFontData(hdc,dwTableTag,0,pbFontData+dwRunningOffset,cbTableLength)==GDI_ERROR)
				{
					delete[] pbFontData; //MEMORY LEAK

					delete[] pbFontHeader; goto err;
				}

				dwRunningOffset += cbTableLength;

				// Pad tables (except last) with zero's
				if(i<nTables-1)
				while(dwRunningOffset%4!=0)
				{
					pbFontData[dwRunningOffset] = 0;
					dwRunningOffset++;
				}				
			}

			delete[] pbFontHeader;

			ppvFontData = pbFontData;
			pcbFontDataSize = cbFontSize;

		//	return S_OK;
		}
		if(!ppvFontData) err:
		{
			DWORD err = GetLastError();

			f = 0; assert(0);
		}
		else f = nvgCreateFontMem(_nvgc,l.lfFaceName,ppvFontData,pcbFontDataSize,true);	
	}
	SelectObject(hdc,old);
	DeleteDC(hdc);

	return f<<24|l.lfHeight;
}

enum{ disable_nanovg=0 }; //TESTING

static void *Exselector_wgl(HMODULE dll, const char *fn)
{
	void *f = wglGetProcAddress(fn);
	if(!f) f = GetProcAddress(dll,fn); assert(f); return f;
}
bool Exselector::_lazy_init_nvgc()
{
	if(_nvgc) return true;

	if(Widgets95::glut::get_ANGLE_enabled())
	{
		if(!glActiveTexture)
		{
			#undef X
			#define X(f) f = Widgets95::gle::f;
			EXSELECTOR_GL_PROCS_X
		}
		assert(glActiveTexture==Widgets95::gle::glActiveTexture);		
	}
	else //opengl32.dll? 
	{
		//HACK: assuming if pointer is identical that there's
		//no call to reload procedures
		if(glBindSampler)
		if(glBindSampler!=(void*)wglGetProcAddress("glBindSampler"))
		glBindSampler = nullptr;
		if(!glBindSampler)
		{
			HMODULE dll = GetModuleHandleA("opengl32.dll");
			#undef X
			#define X(f) (void*&)f = Exselector_wgl(dll,#f);
			EXSELECTOR_GL_PROCS_X
		}
	}
	
	_nvgc = nvgCreateGLES3(1?0:NVG_ANTIALIAS|NVG_STENCIL_STROKES);

	return true;
}
void Exselector::svg_reset(bool canvas)
{
	if(_nvgc) nvgDeleteGLES3(_nvgc); _nvgc = nullptr;

	if(canvas) _lazy_init_nvgc(); //2022
}
bool Exselector::svg_canvas()
{
	return _nvgc!=0;
}
void Exselector::svg_viewport(int x, int y, int w, int h)
{
	_lazy_init_nvgc(); //assert(_nvgc); //REMOVE ME?
	
	glViewport(x,y,w,h);
}
void Exselector::svg_view(float wvp[4*4])
{
	_lazy_init_nvgc(); //assert(_nvgc); //REMOVE ME?

	if(!wvp) //DOCUMENTATION
	{
		int vp[4] = {};
		glGetIntegerv(0x0BA2,vp); //GL_VIEWPORT
		//nvgBeginFrame(_nvgc,(float)vp[2],(float)vp[3],1.0f);

		float tmp[4*4] = 
		{
//	"	gl_Position = vec4(2.0*vertex.x/viewSize.x - 1.0, 1.0 - 2.0*vertex.y/viewSize.y, 0, 1);\n"

			2.0f/vp[2],0,0,0,
			0,-2.0f/vp[3],0,0,
			0,0,0,0,
			-1,1,0,1,
		};
		
		//NEW WAY
	//	if(upsidedown) tmp[5] = -tmp[5]; 
	//	if(upsidedown) tmp[13] = -tmp[13];

	//	nvgBeginFrame(_nvgc,wvp,1.0f);
				
		/*OLD WAY
		//NOTE: relies on removing "scale/invscale" business
		if(upsidedown) nvgScale(_nvgc,1,-1); 
		if(upsidedown) nvgTranslate(_nvgc,0,(float)-vp[3]);
		//_upsidedown = upsidedown;
		*/

		nvgView(_nvgc,tmp);
	}
	else nvgView(_nvgc,wvp);
}
void Exselector::svg_begin()
{
	_lazy_init_nvgc();

	if(disable_nanovg) return; //TESTING

	//NOTE: 1.0 (2, etc.) does nothing I
	//can see. it would be nice to raise
	//the resolution since VR is spatial
	nvgBeginFrame(_nvgc,1.0f);

	//NOTE: without this nothing renders
	//at all... I'm not sure what's wrong
	//with the caller's sampler that it's
	//not drawing (the bigger problem is
	//svg_end is doing glBindSampler(0,1))
	#ifdef NDEBUG
//	#error remove me?
	#endif
	glBindSampler(0,0); //TESTING #1
}
int Exselector::svg_font(HFONT h)
{
	_lazy_init_nvgc(); //assert(_nvgc);

	return Exselector_ttf(h,_nvgc);	
}
static std::vector<char> Exselector_utf8;
int Exselector::svg_draw(int font, const wchar_t *str, int len, RECT *rect, int format, unsigned color)
{
	assert(_nvgc); //_lazy_init_nvgc();

	int h = font&0xFF; 	
	int f = (unsigned&)font>>=24;

	if(disable_nanovg) return h; //TESTING

	nvgFontFaceId(_nvgc,f);
	nvgFontSize(_nvgc,(float)h);

	int a; switch(format&3)
	{
	default: assert(0); //not being initialized?
	case DT_LEFT: a = NVG_ALIGN_LEFT; break;
	case DT_CENTER: a = NVG_ALIGN_CENTER; break;
	case DT_RIGHT: a = NVG_ALIGN_RIGHT; break;
	}
	#if 1
	#define VALIGNED
	//these don't affect layout, but may still
	//be relevant. refer to fons__getVertAlign
	switch(format&(4|8)) //NVG_ALIGN_BASELINE?
	{
	case DT_TOP: a|=NVG_ALIGN_TOP; break;
	case DT_VCENTER: a|=NVG_ALIGN_MIDDLE; break;
	case DT_BOTTOM: a|=NVG_ALIGN_BOTTOM; break;
	}
	#endif
	nvgTextAlign(_nvgc,a);

	NVGcolor c; //OPTIMIZE ME? //nvgRGBA?
	c.r = float(0xFF&(color>>16))*0.00392156f; //1/255
	c.g = float(0xFF&(color>>8 ))*0.00392156f;
	c.b = float(0xFF&(color    ))*0.00392156f;
	c.a = float(0xFF&(color>>24))*0.00392156f; 
	nvgFillColor(_nvgc,c);

	//REMINDER: not 0 termianted
	auto &v = Exselector_utf8; retry:
	int i = WideCharToMultiByte(65001,0,str,len,v.data(),(int)v.size(),0,0);
	if(!i||i>=(int)v.size()) 
	{
		if(i||ERROR_INSUFFICIENT_BUFFER==GetLastError())
		{
			i = WideCharToMultiByte(65001,0,str,len,0,0,0,0);
			i = i/64*64+64;
			v.resize((size_t)i); goto retry;
		}
		assert(0); return 0;
	}
	int x = rect->left;	
	int w = rect->right-x;
	int t = rect->top;
	int b = rect->bottom;	
		//NVG_ALIGN_TOP approximates h/5
		//(I had missed I'd disabled it)
		//
		// MIGHT NEED TO GO BACK TO THIS
		//
		#ifndef VALIGNED
	//+h: nanovg seems to put the origin
	//at the bottom or baseline of lines
	//-h/5: nanovg positions its letters
	//different from Windows. horizontal
	//is 1px off too, but I'm unsure why
	//BOTTOM is UNTESTED
	if(~format&DT_BOTTOM) b = t+h;
	//7.8 is a refinement for "-h/5" for
	//large fonts. NOTE: 8.0 is too much
	//b-=h/5; 
	float y = b-(float)h/7.8f;
		#else
	int y = format&DT_BOTTOM?b:t;
		#endif
	if(format&DT_VCENTER) y+=(b-t)/2; //YUCK

	//nvgTextAlign doesn't apply to this API
	//nvgText(_nvgc,x,y,v.data(),v.data()+i);
	float yy = nvgTextBox(_nvgc,(float)x,(float)y,(float)w,v.data(),v.data()+i);

	//return h; //WIP
	return std::max(h,(int)yy-y);
}
void Exselector::svg_end(int(*xr_f)(int))
{
	if(disable_nanovg) return; //TESTING

	if(xr_f) //HACK: the text can't use regular mipmaps
	{
		glDisable(0x9563); //GL_SHADING_RATE_IMAGE_NV
	}

	nvgEndFrame(_nvgc,xr_f); assert(_nvgc);

	if(xr_f) //HACK: assuming no-op if no mask is bound
	{
		glEnable(0x9563); //GL_SHADING_RATE_IMAGE_NV
	}

	glBindSampler(0,1); //TESTING #2

	//REMOVE ME
	//REFERENCE (I may need to restore this)
	// 	   
	// the black tint is coming out deep blue
	// 
	//NOTE: source is actually GL_ONE... nvgReset
	//changes to NVG_SOURCE_OVER which looks like
	//ID3DXFont. GL_SRC_ALPHA is faint
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
}

static Widgets95::ui *Exselector_main_win = nullptr;
static DWORD WINAPI Exselector_main_thread(void*)
{	
	Widgets95::glut::set_wxWidgets_enabled();

	using namespace Widgets95::glute;

	int argc = 1;
	char *bogus = "Exselector";
	glutInit(&argc,&bogus);
	glutInitWindowSize(100,100);
	glutInitDisplayMode(GLUT_RGB|GLUT_DOUBLE|GLUT_DEPTH);
	Exselector_main_win = new Widgets95::ui("Watch",100,100);
	Exselector_main_win->hide();
	glutMainLoop();
	return 0;
}
static void Exselector_kickoff_main_thread()
{
	if(!Exselector_main_win)
	{
		CreateThread(0,0,Exselector_main_thread,0,0,0);
		while(!Exselector_main_win) 
		Sleep(1);
		Sleep(0);
	}
}
static void Exselector_slider_cb(Widgets95::ui::control *c)
{
	auto *b = (Widgets95::ui::bar*)c;
	auto *a = b->associated_object;
	double d = c->float_val();
	d==(int)d?a->set_int_val((int)d):a->set_float_val(d);
}
Widgets95::node *Exselector::watch(const char *name, const type_info &type, void *var, bool reg)
{
	if(reg) return nullptr; //UNIMPLEMENTED (may want to provide a menu of watch items)

	auto* &ui = Exselector_main_win; //REPURPOSING (for now)

	//NOTE: if a new window were to be created here it
	//would have to be injected into the glutMainLoop
	//thread somehow (glutIdleFunc? I don't know)
	if(!ui)
	{
		Exselector_kickoff_main_thread(); Sleep(250);
	}

	for(auto*ch=ui->main_panel()->first_child();ch;ch=ch->next())
	{
		if(ch->live_ptr()==var) return ch; //already live?
	}

	Widgets95::ui::control *ret = nullptr;
	
	//EXTENSION
	//this should call xcv_widgets->Yield()
	Widgets95::glute::glutMainLoop();

	HDC dc = wglGetCurrentDC();
	HGLRC glrc = wglGetCurrentContext();
	{
		if(type==typeid(float))
		{
			new Widgets95::ui::spinbox(ui,name,(float*)var);
		}
		else if(type==typeid(double))
		{
			new Widgets95::ui::spinbox(ui,name,(double*)var);
		}
		else if(type==typeid(int))
		{
			new Widgets95::ui::spinbox(ui,name,(int*)var);
		}
		else if(type==typeid(std::string))
		{
			new Widgets95::ui::wordproc(ui,name,true,(std::string*)var);
		}
		else 
		{
			assert(0); return nullptr;
		}

		ret = ui->main_panel()->last_child();

		//ret->name().append("\t"); ret->expand();

		auto *p = strchr(name,'(');
		if(!p) p = strchr(name,'[');
		if(p) if(auto*sb=dynamic_cast<Widgets95::ui::spinbox*>(ret))
		{
			bool slider = *p=='[';

			p++; char *e;		
			double d = strtod(p,&e);
			double dd = strtod(e+1,0);
			if('~'==*e) sb->limit(d,dd);

			if(slider)
			{
				auto *b = new Widgets95::ui::bar(ui,"",1);

				b->expand();

				b->set_object_callback(Exselector_slider_cb,sb);

				if('~'==*e) b->set_range(d,dd);

				if(type==typeid(int))
				{
					b->spin(ret->int_val());
				}
				else b->spin(ret->float_val());
			}
		}

		ui->show(); 	
	}
	wglMakeCurrent(dc,glrc); //guarding

	return ret;
}

static std::map<size_t,void*> Exselector_sections;

bool Exselector::_section(size_t ti, size_t sz, void **o)
{
	EX_CRITICAL_SECTION
	void* &p = Exselector_sections[ti];
	bool ret;
	if(ret=!p) 
	p = new BYTE[sz](); *o = p;
	return ret;
}

static Widgets95::ui::ti *Exselector_new_edit(Widgets95::ui::rollout*p, const char *l, int total)
{
	if(total%10==0) new Widgets95::ui::column(p);

	auto *tb = new Widgets95::ui::wordproc(p,l);
	
	tb->expand(); tb->lock(1,0); tb->span(250); tb->drop(10); 
	
	return tb;
}
static void Exselector_set_text(Widgets95::ui::ti *tb, EX::INI::Section &s, const char *l)
{
	const wchar_t *w = s.get(l); 
	
	int nl = *w?1:0; 

	if(!*w) w = L"\n"; //HACK

	std::string ss;
	
	while(*w)
	{
		if(*w=='\n') nl++;

		ss.push_back(*w++);
	}
	
	tb->lim_max() = nl; 

	if(nl<=1) tb->scrollbar->set_hidden();

	tb->set_text(ss.c_str());
}

void Exselector::extensions()
{
	return; //DISABLING

	static bool one_off = true; //!
	if(one_off) return; one_off = true; //EXPERIMENTAL

	auto* &ui = Exselector_main_win; //REPURPOSING (for now)

	//NOTE: if a new window were to be created here it
	//would have to be injected into the glutMainLoop
	//thread somehow (glutIdleFunc? I don't know)
	if(!ui)
	{
		Exselector_kickoff_main_thread(); Sleep(250);
	}
	HDC dc = wglGetCurrentDC();
	HGLRC glrc = wglGetCurrentContext();

	int total = 0;
	Widgets95::ui::ti *c;
	EX::INI::Adjust ad;
	Widgets95::ui::rollout *p;
	#define _(A) \
	p = new Widgets95::ui::rollout(ui,#A,false);		
	#define LOOKUP(A,B,C) \
	if(strcmp(#C,"total_failures"))\
	{\
		c = Exselector_new_edit(p,#C,total++);\
		Exselector_set_text(c,*sx._section,#C);\
	}
	_(Adjust)
	{
		total = 0;
		EX::INI::Adjust ad, &sx = ad;
		#define SOMEX_INI_ADJUST_SECTION
		#include "../SomEx/ini.lookup.inl"
		#undef SOMEX_INI_ADJUST_SECTION
	}
	_(Analog)
	{
		total = 0;
		EX::INI::Analog an, &sx = an;
		#define SOMEX_INI_ANALOG_SECTION
		#include "../SomEx/ini.lookup.inl"
		#undef SOMEX_INI_ANALOG_SECTION
	}
	_(Author)
	{
		total = 0;
		EX::INI::Author au, &sx = au;
		#define SOMEX_INI_AUTHOR_SECTION
		#include "../SomEx/ini.lookup.inl"
		#undef SOMEX_INI_AUTHOR_SECTION
	}
//	_(Bitmap)
	_(Boxart)
	{
		total = 0;
		EX::INI::Boxart ba, &sx = ba;
		#define SOMEX_INI_BOXART_SECTION
		#include "../SomEx/ini.lookup.inl"
		#undef SOMEX_INI_BOXART_SECTION
	}
	_(Bugfix)
	{
		total = 0;
		EX::INI::Bugfix bf, &sx = bf;
		#define SOMEX_INI_BUGFIX_SECTION
		#include "../SomEx/ini.lookup.inl"
		#undef SOMEX_INI_BUGFIX_SECTION
	}
	_(Damage)
	{
		total = 0;
		EX::INI::Damage hp, &sx = hp;
		#define SOMEX_INI_DAMAGE_SECTION
		#include "../SomEx/ini.lookup.inl"
		#undef SOMEX_INI_DAMAGE_SECTION
	}
	_(Detail)
	{
		total = 0;
		EX::INI::Detail dt, &sx = dt;
		#define SOMEX_INI_DETAIL_SECTION
		#include "../SomEx/ini.lookup.inl"
		#undef SOMEX_INI_DETAIL_SECTION
	}
	_(Editor)
	{
		total = 0;
		EX::INI::Editor ed, &sx = ed;
		#define SOMEX_INI_EDITOR_SECTION
		#include "../SomEx/ini.lookup.inl"
		#undef SOMEX_INI_EDITOR_SECTION
	}
//	_(Engine)
//	_(Joypad)
	_(Launch)
	{
		total = 0;
		EX::INI::Launch la, &sx = la;
		#define SOMEX_INI_LAUNCH_SECTION
		#include "../SomEx/ini.lookup.inl"
		#undef SOMEX_INI_LAUNCH_SECTION
	}
//	_(Number)
	_(Option)
	{
		total = 0;
		EX::INI::Option op, &sx = op;
		#define SOMEX_INI_OPTION_SECTION
		#include "../SomEx/ini.lookup.inl"
		#undef SOMEX_INI_OPTION_SECTION
	}
	_(Output)
	{
		total = 0;
		EX::INI::Output tt, &sx = tt;
		#define SOMEX_INI_OUTPUT_SECTION
		#include "../SomEx/ini.lookup.inl"
		#undef SOMEX_INI_OUTPUT_SECTION
	}
	_(Player)
	{
		total = 0;
		EX::INI::Player pc, &sx = pc;
		#define SOMEX_INI_PLAYER_SECTION
		#include "../SomEx/ini.lookup.inl"
		#undef SOMEX_INI_PLAYER_SECTION
	}
	_(Sample)
	{
		total = 0;
		EX::INI::Sample sa, &sx = sa;
		#define SOMEX_INI_SAMPLE_SECTION
		#include "../SomEx/ini.lookup.inl"
		#undef SOMEX_INI_SAMPLE_SECTION
	}
	_(Script)
	{
		total = 0;
		EX::INI::Script gt, &sx = gt;
		#define SOMEX_INI_SCRIPT_SECTION
		#include "../SomEx/ini.lookup.inl"
		#undef SOMEX_INI_SCRIPT_SECTION
	}
	_(Stereo)
	{
		total = 0;
		EX::INI::Stereo st, &sx = st;
		#define SOMEX_INI_STEREO_SECTION
		#include "../SomEx/ini.lookup.inl"
		#undef SOMEX_INI_STEREO_SECTION
	}
	_(System)
	{
		total = 0;
		EX::INI::System sy, &sx = sy;
		#define SOMEX_INI_SYSTEM_SECTION
		#include "../SomEx/ini.lookup.inl"
		#undef SOMEX_INI_SYSTEM_SECTION
	}
	//_(Volume)
	_(Window)
	{
		total = 0;
		EX::INI::Window wn, &sx = wn;
		#define SOMEX_INI_WINDOW_SECTION
		#include "../SomEx/ini.lookup.inl"
		#undef SOMEX_INI_WINDOW_SECTION
	}
	#undef _
	#undef LOOKUP
		
	ui->show();

	wglMakeCurrent(dc,glrc); //guarding
}
