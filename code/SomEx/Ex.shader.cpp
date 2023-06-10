			  
#include "Ex.h" 
EX_TRANSLATION_UNIT

#include <vector> //NEW

#include <d3dx9shader.h>

#include "Ex.output.h"
#include "Ex.shader.h"

//REMOVE ME
//2021: the debugger won't inspect "asm"
//#define asm _asm_ //spasm

static char *Ex_shader_macros = 0;

static size_t Ex_shader_sizeof_macros = 0;

static D3DXMACRO *Ex_shader_d3dxmacros = 0;

static int Ex_shader_defined_macros = 0;
static int Ex_shader_compare_macros = 0;

#undef EX_SHADER_USING_FXC //using D3DDisassembleShader instead 

static const char *Ex_shader_profile(int type);

//UNUSED: launch fxc.exe in subprocess / generate spasm for hlsl code or file
//static char *Ex_shader_fxc(const char*,const char*,const char*,const wchar_t*);

//compile hlsl / disassemble via D3DX libraries
static char *Ex_shader_hlsl(int,const char*,const char*,const wchar_t*);

//copy spasm with contant register references padded by n
static char *Ex_shader_pad(const char*, int n, char *spasm);

//the workhorse of this file: link n padded spasm units
static const int *Ex_shader_link(char **spasm, int n, const int *arraytable);

int &EX::defining_shader_macro(const char *D)
{	
	if(!D) return Ex_shader_compare_macros;

	size_t length = strlen(D);

	int v = 0; while(D[v]&&D[v]!=' ') v++; 

	const char *define = D, *value = D+v+(v!=length);

	size_t required = length+1;

#ifdef EX_SHADER_USING_FXC

	required+=sizeof("#define");

#endif

	//TODO: use std container
	static size_t definechars = 0; 
	if(Ex_shader_sizeof_macros+required>definechars)
	{
		char *swap = Ex_shader_macros;
		definechars+=max(definechars?definechars:512,required);
		Ex_shader_macros = new char[definechars+1];

		if(Ex_shader_d3dxmacros)
		for(int i=0;i<Ex_shader_defined_macros;i++)
		{
			Ex_shader_d3dxmacros[i].Name = 
			Ex_shader_macros+(Ex_shader_d3dxmacros[i].Name-swap);
			Ex_shader_d3dxmacros[i].Definition = 
			Ex_shader_macros+(Ex_shader_d3dxmacros[i].Definition-swap);
		}
		memcpy(Ex_shader_macros,swap,Ex_shader_sizeof_macros);
		delete[] swap;
	}
	
	char *p = Ex_shader_macros+Ex_shader_sizeof_macros;
	
#ifdef EX_SHADER_USING_FXC

	assert(0); //unimplemented

#else 

	define = (char*)memcpy(p,define,v); p[v] = '\0';

	value  = (char*)memcpy(p+v+1,value,length-v); p[length+1] = '\0';

	static int d3dxmacros = 0;

	int d = Ex_shader_defined_macros++;

	if(Ex_shader_defined_macros>=d3dxmacros)
	{
		d3dxmacros = d?d*2+2:16;	 
		D3DXMACRO *swap = Ex_shader_d3dxmacros;
		Ex_shader_d3dxmacros = new D3DXMACRO[d3dxmacros+1];	
		memcpy(Ex_shader_d3dxmacros,swap,sizeof(D3DXMACRO)*d);	
		delete[] swap;
	}

	Ex_shader_d3dxmacros[d].Name = define;
	Ex_shader_d3dxmacros[d].Definition = value;

	Ex_shader_d3dxmacros[d+1].Definition = 0; //paranoia	
	Ex_shader_d3dxmacros[d+1].Name = 0; //terminal

#endif

	Ex_shader_macros[Ex_shader_sizeof_macros+=required] = '\0';

	return ++Ex_shader_compare_macros;
}

int EX::undefining_shader_macros()
{
	Ex_shader_sizeof_macros = 0;

	if(Ex_shader_macros) Ex_shader_macros[0] = '\0';

	if(Ex_shader_d3dxmacros) 
	{
		Ex_shader_d3dxmacros[0].Name = 0;
		Ex_shader_d3dxmacros[0].Definition = 0;
	}

	Ex_shader_defined_macros = 0;

	return ++Ex_shader_compare_macros;
}

void EX::undefining_shader_macro(const char *D)
{
	if(!D||!*D||!D[1]){ assert(0); return; }

	int i,j;
	for(i=j=0;i<Ex_shader_defined_macros;i++)
	{
		auto &d = Ex_shader_d3dxmacros[i];

		if(D[1]==d.Name[1]) 
		{
			int len = strlen(d.Name);
			if(!memcmp(D,d.Name,len))
			if(!D[len]||isspace(D[len]))
			{
				continue;
			}
		}

		Ex_shader_d3dxmacros[j++] = d;
	}
	Ex_shader_defined_macros = j;
	Ex_shader_d3dxmacros[j].Definition = 0; //paranoia	
	Ex_shader_d3dxmacros[j].Name = 0; //terminal
}

static char *Ex_shader_prep(const char *code)
{
	LPD3DXBUFFER obj = 0, err = 0;
	if(code)
	D3DXPreprocessShader(code,strlen(code),Ex_shader_d3dxmacros,0,&obj,EX::debug?&err:0);
	if(err)
	{
		auto *e = err->GetBufferPointer(); assert(0); 
		
		err->Release();
	}
	char *o = 0; if(obj)
	{
		auto sz = obj->GetBufferSize();
		o = new char[sz+1];
		memcpy(o,obj->GetBufferPointer(),sz);
		o[sz] = '\0';
		obj->Release();
	}
	return o;
}
char *EX::preprocessing_shader(const char *code) 
{
	return Ex_shader_prep(code);
}

char *EX::compiling_shader(int type, const char *main, const char *code, const wchar_t *file, int lang)
{
	switch(lang)
	{
	case 'hlsl': return Ex_shader_hlsl(type,main,code,file);

	default: assert(lang=='hlsl'); return 0;
	}
}

const int *EX::linking_shaders(char **spasm, int count, const int *arraytable)
{
	return Ex_shader_link(spasm,count,arraytable);
}

const DWORD *EX::assembling_shader(const char *spasm, int target)
{
	if(!spasm) return 0;
	assert(target=='dx9c');							 
	LPD3DXBUFFER out = 0, err = 0;
	if(!D3DXAssembleShader(spasm,strlen(spasm),0,0,0,&out,EX::debug?&err:0))
	{
		size_t copy = out->GetBufferSize();
		return (DWORD*)memcpy(new char[copy],out->GetBufferPointer(),copy);
	}
	const char *debug = err?(const char*)err->GetBufferPointer():0; if(err) err->Release();
	EX_BREAKPOINT(som_shader_assemble)
	return 0;
}

const DWORD *EX::debugging_shader(int type, const char *main, const char *code, void*, int)
{
	if(!code||!main) return 0;

	DWORD cflags =
	D3DXSHADER_AVOID_FLOW_CONTROL|
	D3DXSHADER_OPTIMIZATION_LEVEL3;
//  |D3DXSHADER_USE_LEGACY_D3DX9_31_DLL;

	LPD3DXBUFFER obj = 0, err = 0;

	const char *profile = Ex_shader_profile(type); 
	
	if(!profile) return 0;

	D3DXMACRO *d = Ex_shader_d3dxmacros;

	HRESULT ok = D3DXCompileShader(code,strlen(code),d,0,main,profile,cflags,&obj,&err,0);
	
	if(ok!=D3D_OK)
	{
		const char *debug = err?(const char*)err->GetBufferPointer():0;

		EX_BREAKPOINT(som_shader_compile)

		err->Release(); return 0;
	}	  		

	return (DWORD*)obj->GetBufferPointer();
}

void EX::releasing_shaders()
{
	assert(0); return; //unimplemented
}

static const char *Ex_shader_profile(int type)
{
	switch(type)
	{
	case 'vs_1': return "vs_1_1";
	case 'vs_2': return "vs_2_a";
	case 'vs_3': return "vs_3_0";

	case 'ps_1': return "ps_1_1";
	case 'ps_2': return "ps_2_0";
	case 'ps_3': return "ps_3_0";

	default: assert(0); //woops
		
		return "";
	}
}

static char *Ex_shader_hlsl
(int type, const char *main, // /T /E 
const char *code, const wchar_t *file) 
{
	if(!code&&!file||!main) return 0;

	static HMODULE legacy = 0;

#ifdef EX_SHADER_USING_FXC

	assert(0); return 0; //unimplemented //REMOVE ME

#else

	DWORD cflags =
	//D3DXSHADER_AVOID_FLOW_CONTROL|
	D3DXSHADER_OPTIMIZATION_LEVEL3;
	//D3DXSHADER_USE_LEGACY_D3DX9_31_DLL;

	if(type=='vs_1'||type=='ps_1')
	{
		//Note: this doesn't work...
		cflags|=D3DXSHADER_USE_LEGACY_D3DX9_31_DLL; 
		//And so we do this instead
		if(!legacy) legacy = LoadLibraryA("d3dx9_31.dll"); 
		if(!legacy) return 0;
		//But it won't fly either!?
		return 0; 
	}

	LPD3DXBUFFER spasm = 0, obj = 0, err = 0;

	LPD3DXBUFFER *_err = 0;
	#ifdef _DEBUG
	_err = &err; //cflags|=D3DXSHADER_DEBUG; //no extra output
	#endif

	const char *profile = Ex_shader_profile(type);

	if(!profile) return 0;

	D3DXMACRO *d = Ex_shader_d3dxmacros;

	HRESULT ok = !D3D_OK;
	
	if(file)
	{
		if(type=='vs_1'||type=='ps_1') //fallback to legacy compiler
		{
			typedef HRESULT (WINAPI *C)(LPCWSTR,CONST D3DXMACRO*,LPD3DXINCLUDE,LPCSTR,LPCSTR,DWORD,LPD3DXBUFFER*,LPD3DXBUFFER*,LPD3DXCONSTANTTABLE*);

			C c = (C)GetProcAddress(legacy,"D3DXCompileShaderFromFileW"); if(!c) return 0;

			if(c) ok = c(file,d,0,main,profile,cflags,&obj,_err,0);
		}
		else ok = D3DXCompileShaderFromFileW(file,d,0,main,profile,cflags,&obj,_err,0);
	}
	else if(code)
	{
		if(type=='vs_1'||type=='ps_1') //fallback to legacy compiler
		{
			typedef HRESULT (WINAPI *C)(LPCSTR,UINT,CONST D3DXMACRO*,LPD3DXINCLUDE,LPCSTR,LPCSTR,DWORD,LPD3DXBUFFER*,LPD3DXBUFFER*,LPD3DXCONSTANTTABLE*);

			C c = (C)GetProcAddress(legacy,"D3DXCompileShader"); if(!c) return 0;

			if(c) ok = c(code,strlen(code),d,0,main,profile,cflags,&obj,_err,0);
		}
		else ok = D3DXCompileShader(code,strlen(code),d,0,main,profile,cflags,&obj,_err,0);
	}
	else return 0;

	if(ok!=D3D_OK)
	{
		const char *debug = err?(const char*)err->GetBufferPointer():0;

		const char *debug2[32]; 
		for(int i=0;i<EX_ARRAYSIZEOF(debug2)&&debug;i++) 
		{
			const char *n = strchr(debug,'\n');
			debug2[i] = strchr(debug,'('); //hack: beyond the file name
			if(debug2[i]>n) debug2[i] = debug;
			debug = n?n+1:0;
		}

		const char *code2[256]; 
		code2[0] = "";
		code2[1] = code;
		for(int i=2;code=strchr(code,'\n');)
		{
			code2[i] = ++code; if(++i==EX_ARRAYSIZEOF(code2)) break;
		}

		EX_BREAKPOINT(som_shader_compile)

		if(err) err->Release(); return 0;
	}
	if(err) err->Release(); //PARANOIA?
		
	ok = D3DXDisassembleShader((DWORD*)obj->GetBufferPointer(),0,0,&spasm);

	obj->Release();	if(ok!=D3D_OK) return 0;

	//return (char*)spasm->GetBufferPointer(); //debugging

	char *out = Ex_shader_pad(main,5,(char*)spasm->GetBufferPointer());

	spasm->Release();

	return out;

#endif
}

#if 0 //UNUSED
static char *Ex_shader_fxc(const char *args, // /T /E /LD
						    const char *head, // #defines etc
						     const char *code, const wchar_t *file)
{	
	static size_t outsz = 65535;

  /*// NOTE THAT THIS WILL UNLIKELY EVER BE USED ////

	started working on this before discovering the
	D3DXDisassembleShader interface. Never tested.

  /*/////////////////////////////////////////////////

	static char *out = new char[outsz+1];

#define NAMEPIPE(xxx) "\\\\.\\pipe\\SomEx-fxc-"#xxx"-pipe_"

#define NAMELEN (sizeof(NAMEPIPE(xxx))-1)

	static char srcpipename[64] = NAMEPIPE(src); //hlsl
	static char asmpipename[64] = NAMEPIPE(spasm); //fxc /Fc
	static char outpipename[64] = NAMEPIPE(out); //stdout
	static char errpipename[64] = NAMEPIPE(err); //stderr

	if(!srcpipename[NAMELEN]) //append PID
	{
		itoa(GetCurrentProcessId(),srcpipename+NAMELEN,10);

		strcpy(asmpipename+NAMELEN,srcpipename+NAMELEN);
		strcpy(outpipename+NAMELEN,srcpipename+NAMELEN);
		strcpy(errpipename+NAMELEN,srcpipename+NAMELEN);
	}

	char argscat[MAX_PATH]; 
	
	const char *options = "/nologo /03";
	
	sprintf_s(argscat,MAX_PATH,
	"%s %s /Fc %s %s",args,options,asmpipename,srcpipename);
	
	if(!*args) return 0; //too long

	const DWORD TIMEOUT = 3000, BUFSIZE = 4096;

	PROCESS_INFORMATION pi; memset(&pi,0,sizeof(pi));

    STARTUPINFOA si; memset(&si,0,sizeof(si)); si.cb = sizeof(si);	

	si.dwFlags = STARTF_USESHOWWINDOW|STARTF_USESTDHANDLES; 
	
	si.wShowWindow = SW_HIDE; 

#define LAYSOMEPIPE(pipe,dir) CreateNamedPipeA\
	(\
		pipe##name,\
		PIPE_ACCESS_##dir##BOUND|FILE_FLAG_FIRST_PIPE_INSTANCE,\
		0,1, /*mode/number of instances permitted*/\
		#dir=="OUT"?BUFSIZE:0, /*string pooling*/\
		#dir=="IN"?BUFSIZE:0, /*string pooling*/\
		TIMEOUT,0\
	);

	//NOTE: these are probably optional
	si.hStdOutput = LAYSOMEPIPE(outpipe,OUT)
	si.hStdError = LAYSOMEPIPE(errpipe,OUT)

	int src = 0; HANDLE hSrc = LAYSOMEPIPE(srcpipe,IN)
	int spasm = 0; HANDLE hAsm = LAYSOMEPIPE(asmpipe,OUT)

    if(si.hStdOutput&&si.hStdError&&hSrc&&hAsm&&
		CreateProcessA("fxc",argscat,0,0,1,CREATE_NO_WINDOW,0,0,&si,&pi))
	{
		const int n = 5;

		HANDLE WaitHandles[n] = 
		{
			pi.hProcess, si.hStdOutput, si.hStdError, hSrc, hAsm
		};

		BYTE buff[BUFSIZE];	DWORD dwWaitResult;

		while(1)
		{
			DWORD dwBytesRead, dwBytesAvailable;

			dwWaitResult = WaitForMultipleObjects(n,WaitHandles,0,TIMEOUT);

			while(PeekNamedPipe(si.hStdOutput,0,0,0,&dwBytesAvailable,0)&&dwBytesAvailable)
			{
				ReadFile(si.hStdOutput,buff,BUFSIZE-1,&dwBytesRead, 0);
			}

			while(PeekNamedPipe(si.hStdError,0,0,0,&dwBytesAvailable,0)&&dwBytesAvailable)
			{
				ReadFile(si.hStdError,buff,BUFSIZE-1,&dwBytesRead,0);
			}
			
			while(PeekNamedPipe(hAsm,0,0,0,&dwBytesAvailable,0)&&dwBytesAvailable)
			{
				ReadFile(si.hStdError,buff,BUFSIZE-1,&dwBytesRead,0);

				if(spasm+dwBytesRead>outsz)
				{
					char *swap = out; 

					outsz*=2; out = new char[outsz+1];

					memcpy(out,swap,spasm);

					delete[] swap;
				}

				memcpy(out+spasm,buff,dwBytesRead);

				spasm+=dwBytesRead;
			}

			//source code pipe block would go here
			{
				//it would either feed Ex_shader_macros
				//followed by the inlined hlsl, or followed by
				//#include "<path to hlsl file>"

				assert(0); //unimplemented
			}

			if(dwWaitResult==WAIT_OBJECT_0||dwWaitResult==WAIT_TIMEOUT)	
				
				break;
		} 

		CloseHandle(pi.hProcess);
	}
	else return 0; //todo: error message
					  
	if(si.hStdOutput) CloseHandle(si.hStdOutput);
	if(si.hStdError) CloseHandle(si.hStdError);

	if(hSrc) CloseHandle(hSrc);
	if(hAsm) CloseHandle(hAsm);

	out[spasm] = '\0';

	return out;
}
#endif //UNUSED

static char *Ex_shader_pad(const char *ep, int m, char *q)
{
	EXLOG_LEVEL(-1) << "PADDING("<<ep<<")\n" << q << "#####\n";

	char *log = q, *line = q; //error reporting

	static int constants = 32; //system wide floor

	size_t n = strlen(q)+m*constants;

	char *p,*out = 0; if(0) redouble:
	{			
		n+=m*constants; delete[] out; 	
		constants*=2; assert(constants<128);		
	}
	p = out = new char[n+1];
	
	bool seperated = true, newline = true; 

	size_t i;
	for(i=0;*q&&i<n;i++) 
	{
		while(*q&&*q=='/') //comments
		{
			assert(*q++=='/');

			while(*q&&*q!='\r'&&*q!='\n') q++;
			while(*q&&(*q=='\r'||*q=='\n')) q++;
		}
				
		switch(*q)
		{
		case '\0': break;

		case '\r': 
			
			//assuming not required

			q++; i--; break;

		case '\n':

			line = q+1;

			if(newline)
			{
				q++; i--; break; //double spaced
			}
			
			seperated = newline = true;

			*p++ = *q++;

			break;

		case '-': case ',': case ' ': case '\t':
					
			if(*q=='\t') *q = ' '; //tabs to spaces

			if(seperated&&*q==' ') 
			{
				q++; i--; break; //extra space
			}

			seperated = true; newline = false;

			*p++ = *q++;

			break;

		//NOTE: turns out shader spasm is case sensitive
		//case 'C': case 'I': case 'B': //paranoia

			//*q+=32; //paranoia: make lowercase...

		case 'c': case 'i':	case 'b':		
						
			if(seperated)
			if(q[1]>='0'&&q[1]<='9')
			{
				//assuming constant register

				*p++ = *q++; i++; //eg. c
				*p++ = *q++; i++; //eg. 0

				bool padding = false;

				for(int j=m-2;j;j--,i++)
				{
					if(!padding) padding = *q<'0'||*q>'9';

					*p++ = padding||!*q?' ':*q++;
				}

				if(!padding)
				{
					EXLOG_ALERT(-1) << "Alert! failed while padding shader:" << line << '\n';

					assert(0); //need more padding??
					return 0; //TODO: error message
				}

				seperated = newline = false;

				i--; break;
			} 

		default: //FALLING THRU 
				
			seperated = newline = false;

			//NOTE: turns out shader spasm is case sensitive
			//if(*q>='A'&&*q<='Z') *q+=32; //paranoia: make lowercase

			*p++ = *q++;
		}
	}		
	if(*q) goto redouble; *p = '\0'; 	
	return out;
}

static int Ex_shader_zero = 0;

static bool *Ex_shader_constants_c = 0;
static bool *Ex_shader_constants_i = 0;
static bool *Ex_shader_constants_b = 0;

static int Ex_shader_largest_constant_c = -1;
static int Ex_shader_largest_constant_i = -1;
static int Ex_shader_largest_constant_b = -1;

static int *Ex_shader_def_constants_c = &Ex_shader_zero;
static int *Ex_shader_def_constants_i = &Ex_shader_zero;
static int *Ex_shader_def_constants_b = &Ex_shader_zero;

static const int *Ex_shader_arrays_c = &Ex_shader_zero;
static const int *Ex_shader_arrays_i = &Ex_shader_zero;
static const int *Ex_shader_arrays_b = &Ex_shader_zero;

static bool Ex_shader_constant_overflow = false;

static void Ex_shader_build_arrays(const int *p, int filter)
{
	Ex_shader_arrays_c = &Ex_shader_zero;
	Ex_shader_arrays_i = &Ex_shader_zero;
	Ex_shader_arrays_b = &Ex_shader_zero;

	if(!p||p[0]==1||p[1]==0) return;

	int m = p[0], n = p[1];
		
	for(int i=2,j=0;i<m&&j<n;j++)
	{
		switch(p[i])
		{
		case 'vs_c': if(filter==0) Ex_shader_arrays_c = p+i; break;
		case 'vs_i': if(filter==0) Ex_shader_arrays_i = p+i; break;
		case 'vs_b': if(filter==0) Ex_shader_arrays_b = p+i; break;
		case 'ps_c': if(filter==1) Ex_shader_arrays_c = p+i; break;
		case 'ps_i': if(filter==1) Ex_shader_arrays_i = p+i; break;
		case 'ps_b': if(filter==1) Ex_shader_arrays_b = p+i; break;

		default: assert(0); 
		}

		i+=2+p[i+1]*2;
	}
}
 
static void Ex_shader_begin_shader() //per shader
{
	*Ex_shader_def_constants_c = 0;
	*Ex_shader_def_constants_i = 0;
	*Ex_shader_def_constants_b = 0;
}

static void Ex_shader_clear_constants() //per profile
{		
	Ex_shader_largest_constant_c = -1;
	Ex_shader_largest_constant_i = -1;
	Ex_shader_largest_constant_b = -1;

	Ex_shader_constant_overflow = false;

	Ex_shader_build_arrays(0,0); //paranoia

	Ex_shader_begin_shader(); //paranoia
}

static int Ex_shader_constant_count(char type, int nth)
{
	const int *arrays = 0;

	switch(type)
	{
	case 'c': arrays = Ex_shader_arrays_c; break;
	case 'i': arrays = Ex_shader_arrays_i; break;
	case 'b': arrays = Ex_shader_arrays_b; break;
	}
	 	 
	if(arrays) for(int i=0;i<arrays[1];i++)
	{
		if(nth==arrays[2+i*2]) return arrays[2+i*2+1];
	}

	return 1;
}

#define CASE_CDATA(type) \
	_a_ = &allocated_##type;\
	_b_ = &Ex_shader_constants_##type;\
	_c_ = &Ex_shader_largest_constant_##type;\
	_d_ = &Ex_shader_def_constants_##type;
#define SWITCH_CDATA(type) \
	switch(type)\
	{\
	case 'c': CASE_CDATA(c) break;\
	case 'i': CASE_CDATA(i) break;\
	case 'b': CASE_CDATA(b) break;\
	}
#define GATHER_CDATA(type) \
	static int allocated_c = 0;\
	static int allocated_i = 0;\
	static int allocated_b = 0;\
\
	int *_a_; bool **_b_; int *_c_, **_d_;\
\
	SWITCH_CDATA(type)\
\
	int   &alloc = *_a_;\
	bool* &bools = *_b_;\
	int   &large = *_c_;\
	int*  &defs  = *_d_;

static int Ex_shader_new_constant(char type, int nth=-1, int x=1)
{
	GATHER_CDATA(type)

	bool add = nth!=-1;	

	if(nth==-1) nth = large+1;

	if(add)	
	{
		int i = defs[0]; while(i&&defs[i]!=nth) i--;

		if(i) add = false; //a def constant
	}

	if(nth+x>alloc)
	{
		bool *swap = bools;
		alloc+=alloc?alloc:96;
		alloc = max(nth+x,alloc);		
		bools = new bool[alloc];
		memcpy(bools,swap,large+1);
		delete[] swap;
	}

	if(nth+x-1>large)
	{
		memset(bools+large+1,0x00,sizeof(bool)*(nth-large));

		large = nth+x-1;
	}

	if(add&&!bools[nth])
	{
		for(int i=0;i<x;i++) bools[nth+i] = true;
	}

	return nth;
}

static int Ex_shader_sub_constant(char type, int nth)
{
	GATHER_CDATA(type)
		
	int i = defs[0]; while(i&&defs[i]!=nth) i--;

	if(i) nth = defs[-i];

	return nth;
}

static int Ex_shader_add_constant(char *c, char **sub=0)
{
	int out = 2; 
	
	if(!c||!*c||c[1]<'0'||c[1]>'9') goto overflow;

	while(c[out]>='0'&&c[out]<='9'||c[out]==' ') out++;

	bool passthru = false;

	switch(*c)
	{
	default: passthru = true;

		assert(c[4]>='a'&&c[4]<='z'); //register syntax

	case 'c': case 'i': case 'b':

		char d = c[out]; c[out] = '\0';

		int nth = atoi(c+1); c[out] = d;
		
		if(nth<0) goto overflow;
		
		if(!passthru)
		{	
			if(!sub) //collecting constants
			{
				int count = 1; //dynamically accessed arrays
				
				if(d=='[') count = Ex_shader_constant_count(*c,nth);

				Ex_shader_new_constant(*c,nth,count);				
			}
			else nth = Ex_shader_sub_constant(*c,nth); 			
		}

		if(sub) //perform substitution
		{
			if(!*sub||*sub>c) goto overflow;

			char *p = *sub; *p++ = *c; 		
						
			if(passthru)
			{
				for(int i=1;i<out;i++) *p++ = c[i];
			}
			else
			{
				char b[32]; _itoa_s(nth,b,32,10);

				int i, j;
				for(i=1,j=0;i<out;i++)
				{
					if(b[j]) *p++ = b[j++];
				}	 
				if(b[j]) goto overflow;
			}
			
			*sub = p;
		}	

		if(d>='a'&&d<='z') out--; 
	}

	return out;
	
overflow: assert(0); //paranoia

	Ex_shader_constant_overflow = true;

	c[out=0] = '\0'; 
	
	return out; 
}

static int Ex_shader_def_constant(char *c, char **sub=0)
{	
	int out = 0; 

	while(c[out]&&out<4) out++;
	
	if(out!=4||memcmp(c,"def ",4)) goto overflow;

	bool passthru = false;

	while(c[out]&&c[out]!='\n') out++; if(c[out]) out++; 

	switch(c[4])
	{
	default: passthru = true;

		assert(c[4]>='a'&&c[4]<='z'); //register syntax

	case 'c': case 'i': case 'b': //FALLING THRU

		int i = 5; while(c[i]>='0'&&c[i]<='9') i++;

		if(i==5) goto overflow;

		char d = c[i]; c[i] = '\0';

		int nth = atoi(c+5); c[i] = d;

		if(!passthru)
		{		
			GATHER_CDATA(c[4])

			if(!alloc||*defs==alloc)
			{
				int pivot = alloc;
				int *swap = defs-pivot;				
				alloc = alloc?alloc*2:16;
				defs = new int[alloc*2+1];
				defs+=alloc; 

				if(pivot) 
				{						
					memcpy(defs-pivot,swap,sizeof(int)*pivot*2+1);	  
					delete[] swap;
				}	
				else *defs = 0;
			}

			//confused?	YES!
			//defs[0] is the number of "def" constants
			//defs[+1] is the first def constant register
			//defs[-1] is the substitution for def[+1]

			defs[*defs+1] = nth;

			if(sub&&*defs)
			{
				int begin = defs[-*defs];

				int i;
				for(i=begin;i<=large;i++) if(!bools[i])
				{
					nth = defs[-*defs-1] = i; break; //a free register
				}			

				if(i>large) //then add a register to the mix
				{
					nth = defs[-*defs-1] = Ex_shader_new_constant(*c);
				}
			}
			else if(!sub)
			{
				defs[-*defs-1] = nth; //paranoia

				*defs+=1;
			}
		}

		if(sub) //perform substitution
		{
			if(!*sub||*sub>c) goto overflow;

			int i = 5; char *p = *sub;
			
			memcpy(p,"def ",4); p+=4; *p++ = c[4]; 		

			if(!passthru)
			{
				char b[32]; _itoa_s(nth,b,32,10);

				int j;
				for(j=0;c[i]==' '||c[i]>='0'&&c[i]<='9';i++)
				{
					if(b[j]) *p++ = b[j++];
				}

				if(b[j]) goto overflow;
			}

			for(c+=i;i<out;i++) *p++ = *c++;

			*sub = p;
		}	
	}

	return out;

overflow: assert(0); //paranoia

	Ex_shader_constant_overflow = true;

	c[out=0] = '\0'; 
	
	return out; 
}

static int *Ex_shader_model = &Ex_shader_zero;

static void Ex_shader_reset_model()
{
	*Ex_shader_model = 0; 
}

static void Ex_shader_update_model(int filter)
{
	//assert(0); //unimplemented
}

static const int *Ex_shader_link(char **spasm, int n, const int *arrays)
{
	//warning: 
	//assuming processed by Ex_shader_pad()
	//meaning:
	//uncommented 
	//no tabs/carriage returns
	//single seperated (where not padded)
	//single spaced

	//spasm is converted to _asm_ and debugger can't
	//see into it
	if(!spasm) return 0;

	Ex_shader_reset_model();

	for(int i=0;i<2;i++) //profiles
	{
		char *profile;

		switch(i)
		{
		case 0: profile = "vs"; break;
		case 1:	profile = "ps"; break;
		}

		Ex_shader_clear_constants(); //clear

		Ex_shader_build_arrays(arrays,i);
				
		for(int pass=1;pass<=2;pass++)
		{
			//1st pass: collect registers
			//2nd pass: perform substitutions

			for(int j=0;j<n;j++) //inputs
			{
				if(!spasm[j]) continue; //return 0;
				
				int k = 0; while(spasm[j][k]==profile[k]) k++;

				if(profile[k]) continue; //mismatch

				Ex_shader_begin_shader();

				char *q = spasm[j], *p = q;
				
				char **sub = pass==2?&p:0; //2nd pass
			
				bool seperated = true;

				while(*q)
				{	
					int paranoia = 0;

					switch(*q)
					{
					case '\0': //paranoia
						
						assert(0); break;

					case '-': //negates swizzle 
					case ',': 
					case ' ':
					case '\t':
					case '\r': 
					case '\n':		
					
						seperated = true; 

						if(sub) *p++ = *q; 

						q++; break;

					case 'd': //def
					case 'c': case 'i':	case 'b':
									
						if(seperated)
						if(q[0]=='d') //def
						{
							if(q[1]=='e'&&q[2]=='f'&&q[3]==' ') 
							{
								paranoia = Ex_shader_def_constant(q,sub);
								break;
							}
						}
						else if(q[1]>='0'&&q[1]<='9')
						{
							//assuming constant register
							paranoia = Ex_shader_add_constant(q,sub);
							break;
						} 

					default: //FALLING THRU 
							
						seperated = false;
						if(sub) *p++ = *q; 						
						q++;
					}

					int k;
					for(k=0;*q&&k<paranoia;k++) q++;

					if(k!=paranoia) 
					{
						*q = '\0'; Ex_shader_constant_overflow = true;
					}
				}

				if(Ex_shader_constant_overflow) 
				{
					spasm[j][0] = '\0'; return 0;
				}

				if(sub) *p = '\0';
			}
		}

		Ex_shader_update_model(i);		
	}

	return Ex_shader_model;
}