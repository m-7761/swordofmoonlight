				
#include "Sompaint.pch.h"

#include <vector>

SOMPAINT_MODULE_API SOMPAINT SOMPAINT_LIB(Connect)(const wchar_t v[4], const wchar_t *party)
{
	assert(0); return 0; //unimplemented
}

namespace Sompaint_cpp
{
	extern "C"{
	typedef struct 
	{
		void *vtable; int status;

	}c; //Sompaint.h
	}
}

SOMPAINT_API SOMPAINT SOMPAINT_LIB(Status)(SOMPAINT server)
{
	if(!server) return 0;

	Sompaint_cpp::c *c = 0;
	SOMPAINT cplusplus = 0;	
	
	const long long a = (long long)&c->status;
	const long long b = (long long)&cplusplus->status;

	return (SOMPAINT)((char*)server+b-a);
}

SOMPAINT_MODULE_API void SOMPAINT_LIB(Disconnect)(SOMPAINT server, bool host)
{
	void *debug = server->disconnect(host); assert(!debug);
}

SOMPAINT_API bool SOMPAINT_LIB(Source)(SOMPAINT server, void **io, const wchar_t string[MAX_PATH])
{
	return server->source(io,string);
}

SOMPAINT_API SOMPAINT_PAL SOMPAINT_LIB(Buffer)(SOMPAINT server, void **io)
{
	return server->buffer(io);
}

SOMPAINT_API bool SOMPAINT_LIB(Share)(SOMPAINT server, void **io, void **io2)
{
	return server->share(io,io2);
}

SOMPAINT_API void SOMPAINT_LIB(Discard)(SOMPAINT server, void **io)
{
	return server->discard(io);
}
 
SOMPAINT_API bool SOMPAINT_LIB(Expose2)(SOMPAINT server, void **io, const char *fstring, va_list va)
{
	return server->expose2(io,fstring,va);
}

SOMPAINT_API bool SOMPAINT_LIB(Format2)(SOMPAINT server, void **io, const char *fstring, va_list va)
{
	return server->format2(io,fstring,va);
}

SOMPAINT_API void *SOMPAINT_LIB(Lock)(SOMPAINT server, void **io, const char *mode, size_t inout[4], int plane)
{
	return server->lock(io,mode,inout,plane);
}

SOMPAINT_API void SOMPAINT_LIB(Unlock)(SOMPAINT server, void **io) 
{
	return server->unlock(io);
}

SOMPAINT_API const wchar_t *SOMPAINT_LIB(Assemble)(SOMPAINT server, const char *code, size_t codelen)
{
	return server->assemble(code,codelen);
}

SOMPAINT_API bool SOMPAINT_LIB(Run)(SOMPAINT server, const wchar_t *program)
{
	return server->run(program);
}

SOMPAINT_API int SOMPAINT_LIB(Load2)(SOMPAINT server, const char *fstring, va_list va)
{
	return server->load2(fstring,va);
}

SOMPAINT_API int SOMPAINT_LIB(Load3)(SOMPAINT server, const char *fstring, void **fargs, size_t fargs_s)
{
	return server->load3(fstring,fargs,fargs_s);
}

SOMPAINT_API void SOMPAINT_LIB(Reclaim)(SOMPAINT server, const wchar_t *handle)
{
	return server->reclaim(handle);
}

SOMPAINT_API void **SOMPAINT_LIB(Define)(SOMPAINT server, const char *d, const char *def, void **io)
{
	return server->define(d,def,io);
}

SOMPAINT_API const char *SOMPAINT_LIB(Ifdef)(SOMPAINT server, const char *d, void **io)
{
	return server->ifdef(d,io);
}

SOMPAINT_API bool SOMPAINT_LIB(Reset)(SOMPAINT server, const char *unused)
{
	return server->reset(unused);
}

SOMPAINT_API bool SOMPAINT_LIB(Frame)(SOMPAINT server, size_t inout[4])
{
	return server->frame(inout);
}

SOMPAINT_API bool SOMPAINT_LIB(Clip)(SOMPAINT server, size_t inout[4])
{
	return server->clip(inout);
}

SOMPAINT_API bool SOMPAINT_LIB(PAL_Apply)(SOMPAINT_PAL buffer, int i)
{
	return buffer->apply(i);
}

SOMPAINT_API bool SOMPAINT_LIB(PAL_Setup)(SOMPAINT_PAL buffer, int i)
{
	return buffer->setup(i);
}

SOMPAINT_API bool SOMPAINT_LIB(PAL_Clear)(SOMPAINT_PAL buffer, int mask)
{
	return buffer->clear(mask);
}

SOMPAINT_API bool SOMPAINT_LIB(PAL_Sample)(SOMPAINT_PAL buffer, int i)
{
	return buffer->sample(i);
}

SOMPAINT_API bool SOMPAINT_LIB(PAL_Layout)(SOMPAINT_PAL buffer, const wchar_t *layout)
{
	return buffer->layout(layout);
}

SOMPAINT_API int SOMPAINT_LIB(PAL_Stream)(SOMPAINT_PAL buffer, int n, const void *up, size_t up_s)
{
	return buffer->stream(n,up,up_s);
}

SOMPAINT_API bool SOMPAINT_LIB(PAL_Draw)(SOMPAINT_PAL buffer, int start, int count, int vstart, int vcount)
{
	return buffer->draw(start,count,vstart,vcount);
}

SOMPAINT_API SOMPAINT_PIX SOMPAINT_LIB(Raster)(SOMPAINT server, void **io, void *local, const char *typeid_local_raw_name)
{
	return server->raster(io,local,typeid_local_raw_name);
}

SOMPAINT_API SOMPAINT_BOX SOMPAINT_LIB(Window)(SOMPAINT server, void **io, void *local, const char *typeid_local_raw_name)
{
	return server->window(io,local,typeid_local_raw_name);
}

SOMPAINT_API bool SOMPAINT_LIB(PAL_Present)(SOMPAINT_PAL buffer, int zoom, SOMPAINT_BOX src, SOMPAINT_BOX dst, SOMPAINT_PIX raster)
{
	return buffer->present(zoom,src,dst,raster);
}

SOMPAINT_API bool SOMPAINT_LIB(PAL_Fill)(SOMPAINT_PAL buffer, int m, int n, int wrap, int zoom, SOMPAINT_BOX src, SOMPAINT_BOX dst, SOMPAINT_PIX raster)
{
	return buffer->fill(m,n,wrap,zoom,src,dst,raster);
}

SOMPAINT_API bool SOMPAINT_LIB(PAL_Print)(SOMPAINT_PAL buffer, int width, int height, SOMPAINT_PIX raster)
{
	return buffer->print(width,height,raster);
}							   

static HMODULE Sompaint_hmodule = 0;

static bool Sompaint_detached = false;

extern HMODULE Sompaint_dll()
{
	//has DllMain been entered?
	assert(Sompaint_hmodule||Sompaint_detached); 

	return Sompaint_hmodule;
}

static std::vector<void*> Sompaint_delete;

extern void Somdelete(void *del)
{
	Sompaint_delete.push_back(del);	
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{							   
	if(ul_reason_for_call==DLL_PROCESS_ATTACH)
	{	
		Sompaint_hmodule = hModule; 
	}	
	else if(ul_reason_for_call==DLL_THREAD_DETACH)
	{

	}
	else if(ul_reason_for_call==DLL_PROCESS_DETACH)
	{
		Sompaint_detached = true; //Sompaint_dll

		size_t i = Sompaint_delete.size();
		while(i>0) delete Sompaint_delete[--i];

		if(lpReserved) Sompaint_hmodule = 0; //...

		//interlocking DLLs 
		//semi-documented behavior
		//http://blogs.msdn.com/b/larryosterman/archive/2004/06/10/152794.aspx
		if(lpReserved) return TRUE; //terminating
	}

    return TRUE;
} 