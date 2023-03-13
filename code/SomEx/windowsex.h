
//reminder: what's up with SetDlgMsgResult???
//#define UNICODE
#include <windowsx.h>

#if _MSC_VER==1600 //2010
//C4258: warns about conforming for-loop scope
__pragma(warning(disable:4258))
//C4305: truncation from int to bool (literal)
__pragma(warning(disable:4305))
#endif

#define QWORD unsigned __int64

typedef void CALLBACK windowsex_TIMERPROC(HWND,UINT,UINT,DWORD);
typedef INT_PTR CALLBACK windowsex_DIALOGPROC(HWND,UINT,WPARAM,LPARAM);
typedef LRESULT CALLBACK windowsex_SUBCLASSPROC(HWND,UINT,WPARAM,LPARAM,UINT_PTR,DWORD_PTR);

#undef GetCharWidth //C4005
#define GetCharWidth32W __pragma(message("error: use GetCharWidthI instead (API says GetCharWidth32 doesn't support TrueType fonts)"))
#define GetCharWidth GetCharWidth32W

#undef ComboBox_FindItemData
#define ComboBox_FindItemData __pragma(message("error: use CB_FINDSTRING... ComboBox_FindItemData is (bizarrely) owner-drawn only??"))

inline ATOM GetClassAtom(HWND hw)
{
	return GetClassLong(hw,GCW_ATOM);
}

static float GetWindowFloat(HWND hw)
{
	WCHAR d[66],*e; float f;
	GetWindowTextW(hw,d,66);		
	f = wcstod(d,&e); if(*e) f = 0; return f;
}
inline float GetDlgItemFloat(HWND hw, int id)
{
	return GetWindowFloat(GetDlgItem(hw,id));
}
static void windowsex_trimf(WCHAR *w)
{
	if(auto*o=wcsrchr(w,'0')) if(!o[1])
	{
		if(auto*dp=wcschr(w,'.')) while(--o>dp)
		{
			o[1] = '\0'; if(o[0]!='0') break;
		}
	}
}
static BOOL SetWindowFloat(HWND hw, float f, PCWSTR fmt=0)
{
	WCHAR w[66]; swprintf_s(w,fmt?fmt:L"%g",f);
	if(fmt) windowsex_trimf(w);
	return SetWindowText(hw,w);	
}
inline BOOL SetDlgItemFloat(HWND hw, int id, float f, PCWSTR fmt=0)
{
	return SetWindowFloat(GetDlgItem(hw,id),f,fmt);	
}

inline int GetDlgRadioID(HWND hw, int a, int z)
{
	for(;a<=z;a++) if(IsDlgButtonChecked(hw,a)) return a; return 0;
}

inline void SetWindowStyle(HWND hw, int mask, int ws=0xFFFFFFFF)
{
	SetWindowLong(hw,GWL_STYLE,~mask&GetWindowStyle(hw)|mask&ws);
}
inline void SetWindowExStyle(HWND hw, int mask, int ws=0xFFFFFFFF)
{
	SetWindowLong(hw,GWL_EXSTYLE,~mask&GetWindowExStyle(hw)|mask&ws);
}

inline void ListBox_SetItem(HWND lb, int i, LPCWSTR string, int data=0)
{
	ListBox_DeleteString(lb,i);
	ListBox_InsertString(lb,i,string);
	if(data) ListBox_SetItemData(lb,i,data);
}
inline void ListBox_SetString(HWND lb, int i, LPCWSTR string)
{
	ListBox_SetItem(lb,i,string,ListBox_GetItemData(lb,i));
}
inline int ListBox_SetCurItem(HWND lb, LPCWSTR string, int data=0)
{
	int i = ListBox_GetCurSel(lb); ListBox_SetItem(lb,i,string,data);
	return ListBox_SetCurSel(lb,i);
}
inline int ListBox_SetCurString(HWND lb, LPCWSTR string)
{
	int i = ListBox_GetCurSel(lb); ListBox_SetString(lb,i,string);
	return ListBox_SetCurSel(lb,i);
}

//EXPERIMENTAL //radio/checkbox?
inline void windowsex_click(HWND bt) 
{
	//Button is focus when BM_CLICK is received???
	HWND gf = GetFocus();
	//BLACK MAGIC
	//BM_CLICK works without SetFocus, but not with
	//SetFocus(gf) unless SetFocus(bt) goes first?!
	SetFocus(bt);
	//MORE PROBLEMS: BM_CLICK is sometimes sending
	//BN_CLICKED before it does BM_SETCHECK inside
	//of initialization.
	SendMessage(bt,BM_CLICK,0,0);
	SetFocus(gf);
}
inline void windowsex_click(HWND dlg, int ID)
{
	windowsex_click(GetDlgItem(dlg,ID)); 
}

inline void windowsex_notify(HWND dlg, WORD ID, WORD hiword=0)
{
	HWND ch = GetDlgItem(dlg,ID);
	SendMessage(dlg,WM_COMMAND,MAKEWPARAM(ID,hiword),(LPARAM)ch);
}

template<int N, typename I>
inline void windowsex_enable(HWND dlg, const I (&ch)[N], int ew=1)
{
	for(int i=0;i<N;i++) EnableWindow(GetDlgItem(dlg,ch[i]),ew);
}
inline void windowsex_enable(HWND dlg, int a, int z, int ew=1)
{
	while(a<=z) if(HWND ch=GetDlgItem(dlg,a++)) EnableWindow(ch,ew);
}
template<int ID>
inline void windowsex_enable(HWND dlg, int ew=1)
{
	windowsex_enable(dlg,ID,ID,ew);
}