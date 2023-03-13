
#ifndef SOMPAINT_INL
#define SOMPAINT_INL(x) x
#endif

#pragma push_macro("_")
#ifdef SOMPAINT_CPLUSPLUS
#define _(default_argument) = default_argument 
#else
#define _(default_argument)
#endif

typedef SOMPAINT_LIB(server) SOMPAINT_INL(server);
typedef SOMPAINT_LIB(buffer) SOMPAINT_INL(buffer);
typedef SOMPAINT_LIB(raster) SOMPAINT_INL(raster);
typedef SOMPAINT_LIB(window) SOMPAINT_INL(window);

inline SOMPAINT_INL(server) *SOMPAINT_INL(Connect)(const wchar_t v[4], const wchar_t *party _(0))
{
	return SOMPAINT_LIB(Connect)(v,party);
}

#ifndef SOMPAINT_CPLUSPLUS
inline SOMPAINT_INL(server) *SOMPAINT_INL(Status)(SOMPAINT_INL(server) *server)
{
	return SOMPAINT_LIB(Status)(server);
}
#endif

inline void SOMPAINT_INL(Disconnect)(SOMPAINT_INL(server) *server, bool host _(false))
{
	SOMPAINT_LIB(Disconnect)(server,host);
}

inline bool SOMPAINT_INL(Source)(SOMPAINT_INL(server) *server, void **io, const wchar_t string[MAX_PATH])
{
	return SOMPAINT_LIB(Source)(server,io,string);
}

inline SOMPAINT_INL(buffer) *SOMPAINT_INL(Buffer)(SOMPAINT_INL(server) *server, void **io)
{
	return SOMPAINT_LIB(Buffer)(server,io);
}

inline bool SOMPAINT_INL(Share)(SOMPAINT_INL(server) *server, void **io, void **io2)
{
	return SOMPAINT_LIB(Share)(server,io,io2);
}

inline void SOMPAINT_INL(Discard)(SOMPAINT_INL(server) *server, void **io)
{
	SOMPAINT_LIB(Discard)(server,io);
}

inline bool SOMPAINT_INL(Format)(SOMPAINT_INL(server) *server, void **io, const char *fstring,...)
{
	va_list va; va_start(va,fstring); bool out = SOMPAINT_LIB(Format2)(server,io,fstring,va); va_end(va); return out;
}

inline bool SOMPAINT_INL(Format2)(SOMPAINT_INL(server) *server, void **io, const char *fstring, va_list va)
{
	return SOMPAINT_LIB(Format2)(server,io,fstring,va);
}

inline bool SOMPAINT_INL(Expose)(SOMPAINT_INL(server) *server, void **io, const char *fstring,...)
{
	va_list va; va_start(va,fstring); bool out = SOMPAINT_LIB(Expose2)(server,io,fstring,va); va_end(va); return out;
}

inline bool SOMPAINT_INL(Expose2)(SOMPAINT_INL(server) *server, void **io, const char *fstring, va_list va)
{
	return SOMPAINT_LIB(Expose2)(server,io,fstring,va);
}

inline void *SOMPAINT_INL(Lock)(SOMPAINT_INL(server) *server, void **io, const char *mode, size_t inout[4], int plane _(0))
{
	return SOMPAINT_LIB(Lock)(server,io,mode,inout,plane);
}

inline void SOMPAINT_INL(Unlock)(SOMPAINT_INL(server) *server, void **io) 
{
	SOMPAINT_LIB(Unlock)(server,io);
}

inline const wchar_t *SOMPAINT_INL(Assemble)(SOMPAINT_INL(server) *server, const char *code, size_t codelen _(-1))
{
	return SOMPAINT_LIB(Assemble)(server,code,codelen);
}

inline bool SOMPAINT_INL(Run)(SOMPAINT_INL(server) *server, const wchar_t *program)
{
	return SOMPAINT_LIB(Run)(server,program);
}

inline int SOMPAINT_INL(Load)(SOMPAINT_INL(server) *server, const char *fstring,...)
{
	va_list va; va_start(va,fstring); int out = SOMPAINT_LIB(Load2)(server,va); va_end(va); return out;
}

inline int SOMPAINT_INL(Load2)(SOMPAINT_INL(server) *server, const char *fstring, va_list va)
{
	return SOMPAINT_LIB(Load2)(server,fstring,va);
}

inline int SOMPAINT_INL(Load3)(SOMPAINT_INL(server) *server, const char *fstring, void **fargs, size_t fargs_s)
{
	return SOMPAINT_LIB(Load3)(server,fstring,fargs,fargs_s);
}

inline void SOMPAINT_INL(Reclaim)(SOMPAINT_INL(server) *server, const wchar_t *handle)
{
	SOMPAINT_LIB(Reclaim)(server,handle);
}

inline void **SOMPAINT_INL(Define)(SOMPAINT_INL(server) *server, const char *d, const char *definition _(""), void **io _(0))
{
	SOMPAINT_LIB(Define)(server,d,definition,io);
}

inline const char *SOMPAINT_INL(Ifdef)(SOMPAINT_INL(server) *server, const char *d, void **io _(0))
{
	return SOMPAINT_LIB(Ifdef)(server,d,io);
}

inline bool SOMPAINT_INL(Reset)(SOMPAINT_INL(server) *server, const char *unused _(0))
{
	return SOMPAINT_LIB(Reset)(server,unused);
}

inline bool SOMPAINT_INL(Frame)(SOMPAINT_INL(server) *server, size_t inout[4])
{
	return SOMPAINT_LIB(Frame)(server,inout);
}

inline bool SOMPAINT_INL(Clip)(SOMPAINT_INL(server) *server, size_t inout[4])
{
	return SOMPAINT_LIB(Clip)(server,inout);
}

inline bool SOMPAINT_INL(PAL_Apply)(SOMPAINT_INL(buffer) *buffer, int i _(0))
{
	return SOMPAINT_LIB(PAL_Apply)(buffer,i);
}

inline bool SOMPAINT_INL(PAL_Setup)(SOMPAINT_INL(buffer) *buffer, int i _(0))
{
	return SOMPAINT_LIB(PAL_Setup)(buffer,i);
}

inline bool SOMPAINT_INL(PAL_Clear)(SOMPAINT_INL(buffer) *buffer, int mask _(~0))
{
	return SOMPAINT_LIB(PAL_Clear)(buffer,mask);
}

inline bool SOMPAINT_INL(PAL_Sample)(SOMPAINT_INL(buffer) *buffer, int i _(0))
{
	return SOMPAINT_LIB(PAL_Sample)(buffer,i);
}

inline bool SOMPAINT_INL(PAL_Layout)(SOMPAINT_INL(buffer), const wchar_t *layout)
{
	return SOMPAINT_LIB(PAL_Layout)(buffer,layout);
}

inline int SOMPAINT_INL(PAL_Stream)(SOMPAINT_INL(buffer) *buffer, int n, const void *up, size_t up_s)
{
	return SOMPAINT_LIB(PAL_Stream)(buffer,n,up,up_s);
}

inline bool SOMPAINT_INL(PAL_Draw)(SOMPAINT_INL(buffer) *buffer, int start, int count, int vstart, int vcount)
{
	return SOMPAINT_LIB(PAL_Draw)(server,start,count,vstart,vcount);
}

inline SOMPAINT_INL(raster) *SOMPAINT_INL(Raster)(SOMPAINT_INL(server) *server, void **io, void *local _(0), const char *typeid_local_raw_name _(0))
{
	return SOMPAINT_LIB(Raster)(server,io,local,typeid_local_raw_name);
}

inline SOMPAINT_INL(window) *SOMPAINT_INL(Window)(SOMPAINT_INL(server) *server, void **io, void *local _(0), const char *typeid_local_raw_name _(0))
{
	return SOMPAINT_LIB(Window)(server,io,local,typeid_local_raw_name);
}

inline bool SOMPAINT_INL(PAL_Present)(SOMPAINT_INL(buffer) *buffer, int zoom, SOMPAINT_INL(window) *src, SOMPAINT_INL(window) *dst, SOMPAINT_INL(raster) *raster)
{
	return SOMPAINT_LIB(PAL_Present)(buffer,zoom,src,dst,raster);
}

inline bool SOMPAINT_INL(PAL_Fill)(SOMPAINT_INL(buffer) *buffer, int m, int n, int wrap, int zoom, SOMPAINT_INL(window) *src, SOMPAINT_INL(window) *dst, SOMPAINT_INL(raster) *raster)
{
	return SOMPAINT_LIB(PAL_Fill)(buffer,m,n,wrap,zoom,src,dst,raster);
}

inline bool SOMPAINT_INL(PAL_Print)(SOMPAINT_INL(buffer) *buffer, int width, int height, SOMPAINT_INL(raster) *raster)
{
	return SOMPAINT_LIB(PAL_Print)(buffer,width,height,raster);
}

#pragma pop_macro("_")
