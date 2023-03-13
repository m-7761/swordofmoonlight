
#ifndef SOMPLAYER_INL
#define SOMPLAYER_INL(x) x
#endif

#pragma push_macro("_")
#ifdef SOMPLAYER_CPLUSPLUS
#define _(default_argument) = default_argument 
#else
#define _(default_argument)
#endif

typedef SOMPLAYER_LIB(console) SOMPLAYER_INL(console);

inline SOMPLAYER_INL(console) SOMPLAYER_INL(Connect)(const wchar_t v[4], wchar_t *party _(0))
{
	return SOMPLAYER_LIB(Connect)(v,party);
}

#ifndef SOMPLAYER_CPLUSPLUS
inline SOMPLAYER_INL(console) SOMPLAYER_INL(Status)(SOMPLAYER_INL(console) *console)
{
	return SOMPLAYER_LIB(Status)(console);
}
#endif

inline void SOMPLAYER_INL(Disconnect)(SOMPLAYER_INL(console) *console, bool host _(false))
{
	return SOMPLAYER_LIB(Disconnect)(console,host);
}

inline size_t SOMPLAYER_INL(Open)(SOMPLAYER_INL(console) *console, const wchar_t path[MAX_PATH], bool play _(true))
{
	return SOMPLAYER_LIB(Open)(console,path,play);
}

inline size_t SOMPLAYER_INL(Listing)(SOMPLAYER_INL(console) *console, const wchar_t **inout _(0), size_t inout_s _(0), size_t skip _(0))
{
	return SOMPLAYER_LIB(Listing)(console,inout,inout_s,skip);
}

inline bool SOMPLAYER_INL(Current)(SOMPLAYER_INL(console) *console, const wchar_t *item, void *reserved _(0))
{
	return SOMPLAYER_LIB(Listing)(console,item,reserved);
}

inline const wchar_t *SOMPLAYER_INL(Change)(SOMPLAYER_INL(console) *console, const wchar_t *play, double skip _(0))
{
	return SOMPLAYER_LIB(Change)(console,play,time);
}

inline bool SOMPLAYER_INL(Capture)(SOMPLAYER_INL(console) *console, const wchar_t *surrounding _(0))
{
	return SOMPLAYER_LIB(Capture)(console,surrounding);
}

inline const wchar_t *SOMPLAYER_INL(Captive)(SOMPLAYER_INL(console) *console)
{
	return SOMPLAYER_LIB(Captive)(console);
}

inline const wchar_t * SOMPLAYER_INL(Release)(SOMPLAYER_INL(console) *console)
{
	return SOMPLAYER_LIB(Release)(console);
}

inline bool SOMPLAYER_INL(Priority)(SOMPLAYER_INL(console) *console, const wchar_t **inout, size_t inout_s)
{
	return SOMPLAYER_LIB(Priority)(console,inout,inout_s);
}

inline double SOMPLAYER_INL(Vicinity)(SOMPLAYER_INL(console) *console, double meters _(-1), bool visibility _(false))
{
	return SOMPLAYER_LIB(Vicinity)(console,meters,visibility);
}

inline size_t SOMPLAYER_INL(Surrounding)(SOMPLAYER_INL(console) *console, const wchar_t **inout _(0), size_t inout_s _(0))
{
	return SOMPLAYER_LIB(Surrounding)(console,inout,inout_s);
}

inline size_t SOMPLAYER_INL(Perspective)(SOMPLAYER_INL(console) *console, const wchar_t **inout _(0), size_t inout_s _(0))
{
	return SOMPLAYER_LIB(Perspective)(console,inout,inout_s);
}

inline size_t SOMPLAYER_INL(Control)(SOMPLAYER_INL(console) *console, HWND window _(0), size_t N _(0))
{
	return SOMPLAYER_LIB(Control)(console,window,N);
}

inline size_t SOMPLAYER_INL(Picture)(SOMPLAYER_INL(console) *console, HWND window _(0), size_t N _(0))
{
	return SOMPLAYER_LIB(Picture)(console,window,N);
}

inline size_t SOMPLAYER_INL(Texture)(SOMPLAYER_INL(console) *console, HWND window _(0), size_t N _(0))
{
	return SOMPLAYER_LIB(Texture)(console,window,N);
}

inline size_t SOMPLAYER_INL(Palette)(SOMPLAYER_INL(console) *console, HWND window _(0), size_t N _(0))
{
	return SOMPLAYER_LIB(Palette)(console,window,N);
}

inline HMENU SOMPLAYER_INL(Context)(SOMPLAYER_INL(console) *console, const wchar_t *item _(0), HWND window _(0), const char *menutext _(0), size_t *ID _(0))
{
	return SOMPLAYER_LIB(Context)(console,item,window,menutext,ID);
}

#pragma pop_macro("_")
