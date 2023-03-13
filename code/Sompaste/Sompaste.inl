
#ifndef SOMPASTE_INL
#define SOMPASTE_INL(x) x
#endif

#pragma push_macro("_")
#ifndef __cplusplus
#define _(default_argument)
#else
#define _(default_argument) = default_argument 
#endif
 
typedef SOMPASTE_LIB(module) SOMPASTE_INL(module);

inline HWND SOMPASTE_INL(Window)(HWND set=0)
{
	return SOMPASTE_LIB(Window)(set);
}

inline RECT SOMPASTE_INL(Pane)(HWND window, HWND client _(0))
{
	return SOMPASTE_LIB(Pane)(window,client,false);
}

inline RECT SOMPASTE_INL(Frame)(HWND window, HWND client _(0))
{
	return SOMPASTE_LIB(Pane)(window,client,true);
}

inline wchar *SOMPASTE_INL(Path)(wchar_t inout[MAX_PATH])
{
	return SOMPASTE_LIB(Path)(inout);
}

inline bool SOMPASTE_INL(Create)(HWND parent)
{
	return SOMPASTE_LIB(Create)(parent);
}

inline HWND *SOMPASTE_INL(Propsheet)(HWND owner, int tabs, const wchar_t *title= _(L"Properties"), HMENU menu _(0))
{
	return SOMPASTE_LIB(Propsheet)(owner,tabs,title,menu);
}

inline bool SOMPASTE_INL(Wallpaper)(HWND window=0, MSG *msg=0)
{
	return SOMPASTE_LIB(Wallpaper)(window,msg);
}

inline HWND SOMPASTE_INL(Folder)(SOMPASTE_INL(module) *p, HWND owner, wchar_t inout[MAX_PATH], const wchar_t *filter _(L"*"), const wchar_t *title _(L""), void *modeless _(0))
{
	return SOMPASTE_LIB(Folder)(p,HWND owner,inout,filter,title,modeless);
}

inline HWND SOMPASTE_INL(Database)(SOMPASTE p, HWND owner, wchar_t inout[MAX_PATH], const wchar_t *filter _(0), const wchar_t *title _(L""), void *modeless _(0))
{
	return SOMPASTE_LIB(Database)(p,HWND owner,inout,filter,title,modeless);
}

inline const wchar_t *SOMPASTE_INL(Longname)(wchar_t token)
{
	return SOMPASTE_LIB(Longname)(token);
}

inline HWND SOMPASTE_INL(Place)(SOMPASTE p, HWND owner, wchar_t out[MAX_PATH], const wchar_t *in _(0), const wchar_t *title _(L""), void *modeless _(0))
{
	return SOMPASTE_LIB(Place)(p,HWND owner,out,in,title,modeless);
}

inline const wchar_t *SOMPASTE_INL(Environ)(SOMPASTE p, const wchar_t *var _(0), const wchar_t *set _(0))
{
	return SOMPASTE_LIB(Environ)(p,var);
}

inline HWND SOMPASTE_INL(Clipboard)(SOMPASTE_INL(module) *p, HWND owner _(0), void *note _(0))
{
	return SOMPASTE_LIB(Clipboard)(p,owner,note);
}

inline HBITMAP SOMPASTE_INL(Clip)(int time _(0))
{
	return SOMPASTE_LIB(Clip)(time);
}

inline HBITMAP SOMPASTE_INL(Clip)(int time _(0))
{
	return SOMPASTE_LIB(Clip)(time);
}

void *SOMPASTE_INL(Choose)(wchar_t *args _(L"Color"), HWND window _(0), SOMPASTE_LIB(cccb) modeless _(0))
{
	return SOMPASTE_LIB(Choose)(args,window,modeless);
}

inline bool SOMPASTE_INL(Center)(HWND window, HWND reference=0, bool activate=true)
{
	return SOMPASTE_LIB(Center)(window,reference,activate);
}

inline bool SOMPASTE_INL(Cascade)(const HWND *windows, int count)
{
	return SOMPASTE_LIB(Cascade)(windows,count);
}

inline bool SOMPASTE_INL(Sticky)(const HWND *windows, int count, int *sticky _(0), RECT *memory _(0), bool zorder _(true))
{
	return SOMPASTE_LIB(Sticky)(windows,count,sticky,memory,zorder);
}

inline bool SOMPASTE_INL(Arrange)(const HWND *windows, int count, const int *sticky _(0), const RECT *memory _(0))
{
	return SOMPASTE_LIB(Arrange)(windows,count,sticky,memory);
}

inline void *SOMPASTE_INL(New)(size_t sizeinbytes)
{
	return SOMPASTE_LIB(New)(sizeinbytes);
}

inline void SOMPASTE_INL(Depart)(SOMPASTE_INL(module) *p)
{
	SOMPASTE_LIB(Depart)(p);
}

inline bool SOMPASTE_INL(Undo)(SOMPASTE_INL(module) *p, HWND hist _(0), void *add _(0))
{
	return SOMPASTE_LIB(Undo)(p,hist,add);
}

inline bool SOMPASTE_INL(Redo)(SOMPASTE_INL(module) *p, HWND hist _(0))
{
	return SOMPASTE_LIB(Redo)(p,hist);
}

inline void SOMPASTE_INL(Reset)(HWND hist _(0))
{
	return SOMPASTE_LIB(Reset)(hist);
}

#pragma pop_macro("_")
