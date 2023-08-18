	
#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

#include <set>
#include <map>
#include <deque>
#include <string>
#include <vector>
#include <hash_map>
#include <algorithm>

#include "Ex.langs.h"
#include "som.932w.h"
#include "som.tool.h"
#include "som.extra.h"
#include "som.title.h" //EXML
#include "som.atlenc.h" //Base64

#include "SomEx.h"
#include "../Sompaste/Sompaste.h"
#include "../lib/swordofmoonlight.h" 
namespace mo = SWORDOFMOONLIGHT::mo;

extern SOMPASTE Sompaste; //path
namespace SOM{ int image(); } //optimizing

//falsify to build in rich text
//DDRAW::fxInterlaced (can't extern?)
static const bool SOM_MAIN_poor = true;

extern SHORT som_tool_VK_CONTROL;

//todo: setup extension for this
//not sure if it can be increased
//once set. The EN response doesn't 
//fire when the limit is reached, but
//presumably only if it is unfulfilled
const int SOM_MAIN_undos = 1000;

//how deep to scan for msgctxt \4 marker
static const int SOM_MAIN_msgctxt_s = 8;
static const int SOM_MAIN_levels_s = 32;

//REMOVE ME??
extern HWND som_tool_dialog(HWND=0);		
extern void som_tool_recenter(HWND,HWND,int=0);
extern HWND &som_tool, som_tool_taskbar;
//menu, 3rd button screen, compact view
static HWND SOM_MAIN_modes[3] = {0,0,0};
//taken from SOM_EDIT.cpp
inline HWND SOM_MAIN_151(){ return SOM_MAIN_modes[1]; }
inline HWND SOM_MAIN_152(){ return SOM_MAIN_modes[2]; }

static bool SOM_MAIN_quiet = false;
#define SOM_MAIN_error(a,b) SOM_MAIN_code(a,b|MB_ICONERROR)
#define SOM_MAIN_report(a,b) SOM_MAIN_code(a,b|MB_ICONINFORMATION)
#define SOM_MAIN_warning(a,b) SOM_MAIN_code(a,b|MB_ICONEXCLAMATION)
static int SOM_MAIN_code(const char *code, int mb)
{						
	if(SOM_MAIN_quiet) return 0;
	//A/0: calling som_tool_MessageBoxA
	return MessageBoxA(0,code,"SOM_MAIN",mb);
}

namespace SOM_MAIN_std
{
	static const size_t _res = 256;
	template<typename T> static std::vector<T> &vector()
	{static std::vector<T>out(_res);out.clear();return out;}
	static std::string &string()
	{static std::string out(_res,0);out.clear();return out;}
	static std::wstring &wstring()
	{static std::wstring out(_res,0);out.clear();return out;}
}
static wchar_t *SOM_MAIN_utf8to16(const char *a, int a_s=-1)
{
	if(!a) return 0;
	std::vector<wchar_t> &out = SOM_MAIN_std::vector<wchar_t>(); 
	out.resize(MultiByteToWideChar(65001,0,a,a_s,0,0)+1);
	MultiByteToWideChar(65001,0,a,a_s,&out[0],out.size());
	out[out.size()-1] = '\0'; return &out[0];
}
static char *SOM_MAIN_utf16to8(const wchar_t *w, int w_s=-1)
{
	if(!w) return 0;
	std::vector<char> &out = SOM_MAIN_std::vector<char>(); 
	out.resize(WideCharToMultiByte(65001,0,w,w_s,0,0,0,0)+1);
	WideCharToMultiByte(65001,0,w,w_s,&out[0],out.size(),0,0);
	out[out.size()-1] = '\0'; return &out[0];
}
static wchar_t *SOM_MAIN_WindowText(HWND win, int dlgid=0)
{
	if(dlgid) win = GetDlgItem(win,dlgid);
	std::vector<wchar_t> &text = SOM_MAIN_std::vector<wchar_t>();
	text.resize(GetWindowTextLengthW(win)+1); 
	if(!GetWindowTextW(win,&text[0],text.size())) text[0] = '\0';
	return &text[0];
}		
static long SOM_MAIN_strntol(char *p, char **e, int r, int n)
{
	long out = strtol(p,e,r); if(*e-p<n) return out;			
	*e = p+n; char del = **e; **e = '\0'; 	
	out = strtol(p,e,r); **e = del; return out;
}
static long SOM_MAIN_wcsntol(wchar_t *p, wchar_t **e, int r, int n)
{
	long out = wcstol(p,e,r); if(*e-p<n) return out;			
	*e = p+n; wchar_t del = **e; **e = '\0'; 	
	out = wcstol(p,e,r); **e = del; return out;
}
static int SOM_MAIN_WideCharEntities(LPWSTR lpWC, int cchWC)
{
	int i,n,&out=cchWC;	assert(cchWC>=0);
	for(i=0;i<out&&lpWC[i]!='&';i++); if(i>=out) return out;
	for(n=out,out=i;i<n;lpWC[out++]=lpWC[i++]) 
	if(lpWC[i]=='&'&&i<n-3) switch(lpWC[i+1]+lpWC[i+2])
	{
	#define CASE(a,b,c) case L##a:\
	if(n-i>=sizeof(b)-1&&!wcsncmp(lpWC+i,L##b,sizeof(b)-1))\
	lpWC[i+=sizeof(b)-2] = c; continue;
	//just the five (todo? add DTD support)
	CASE('l'+'t',"&lt;",'<')CASE('g'+'t',"&gt;",'>')
	CASE('a'+'p',"&apos;",'\'')CASE('q'+'u',"&quot;",'"')CASE('a'+'m',"&amp;",'&')
	#undef CASE
	default: if(lpWC[i+1]!='#') continue;
		int x = lpWC[i+2]=='x'; wchar_t *e; 
		switch(lpWC[i+2+x]) //strtol issues
		{case '+': case '-': case 'x': case 'X': continue;}
		x = SOM_MAIN_wcsntol(lpWC+i+2+x,&e,x?16:10,n-i);
		if(e<lpWC+n&&*e==';') lpWC[i=e-lpWC] = x;
	}
	return out;
}
inline int SOM_MAIN_AttribToWideChar(LPCSTR in, int in_s, LPWSTR out, int out_s)
{
	return SOM_MAIN_WideCharEntities(out,MultiByteToWideChar(65001,0,in,in_s,out,out_s));
}		

namespace SOM_MAIN_cpp
{	
	enum
	{ 
	wm_drawclipboard=WM_APP, 
	wm_wavefunctioncollapse,
	};

	struct _layout //private parts 
	{
		HWND cp, gp, z; 
		LONG ws, wes, nc, lm,tm,rm,bm;		
	};
	struct layout : RECT, private _layout
	{
		inline const _layout *operator->()const
		{
			return &static_cast<const _layout&>(*this);
		}		
		layout &operator=(HWND hw)
		{
			gp = GetParent(cp=hw);
			GetClientRect(hw,this);	
			nc = right;
			GetWindowRect(hw,this);			
			MapWindowRect(0,gp,this);						
			nc = right-left-nc;
			if(nc>GetSystemMetrics(SM_CXVSCROLL))
			nc-=GetSystemMetrics(SM_CXVSCROLL);
			z = GetWindow(hw,GW_HWNDPREV);
			ws = GetWindowStyle(hw);
			wes = GetWindowExStyle(hw);				
			//todo: replace "sep" static 
			//variables with these margins			
			GetClientRect(gp,(RECT*)&lm); 
			lm = left; tm = top; assert(&lm+3==&bm);
			rm-=right; bm-=bottom;
			return *this;
		}
	};
}

static struct SOM_MAIN_tree //singleton
{
	static SOM_MAIN_tree &tree;

	struct{ mo::image_t mo; }nihongo;

	SOM_MAIN_tree(){ nihongo.mo.bad = 1; }

	struct stringbase
	{		
		mutable const char *id, *str;
		mutable size_t ctxt_s, id_s, str_s;
	};
	struct string : stringbase 
	{	
		stringbase &base(){ return *this; }		
		string &nc()const{ return (string&)*this; }
		inline string(const char *b_id){ id=b_id; } 
		inline bool operator<(const string &b)const
		{
			return strcmp(id,b.id)<0;
		}			
		inline bool operator!=(const string &b)const
		{
			return (operator==(b))==false;
		}
		inline bool operator==(const string &b)const
		{
			if(id_s!=b.id_s) return false; 
			return ctxt_s==b.ctxt_s&&!strcmp(id,b.id);
		}		
		mutable HTREEITEM left, right;
		inline HTREEITEM operator[](HWND tv)const
		{
			return tv==tree.left.treeview?left:right;
		}		
		mutable HWND undo;
		mutable size_t outline, mmap; inline string()
		{
			memset(this,0x00,sizeof(*this)); mmap = -1;
		}
		inline string(const mo::range_t &r)
		{
			int compile[sizeof(*this)==4*10];

			new(this)string; if(r.n) //debugging
			{
				ctxt_s = r.ctxtlog-r.catalog;
				id = mo::msgctxt(r); id_s = r.idof->clen; 				
				str = mo::msgstr(r); str_s = r.strof->clen;								
				mmap = r.lowerbound;
			}
			else assert(0);
		}
		inline string &operator=(const mo::range_t &r)
		{
			return *(new(this)string(r));
		}		

		inline bool rewrote()const
		{
			return id+id_s+1==str;
		}
		inline bool serifed()const 
		{				
			return itemized();
		}
		inline bool itemized()const 
		{			
			return !ctxt_s||id[ctxt_s]; 
		}				
		inline bool unlisted()const
		{
			return !(temporary()?right||left:itemized());
		}
		inline bool unoriginal()const 
		{
			return ctxt_s==0&&tree.nihongo.mo.claim(id);
		}		
		inline bool bilingual()const 
		{
			mo::image_t &jmo = tree.nihongo.mo;
			return !ctxt_s&&jmo.claim(id)&&!jmo.claim(str);
		}
		inline size_t bilingual_gettext(const char* &jstr)const
		{
			assert(bilingual()); //optimizing
			const string &j = *tree.jtable.find(*this); 
			jstr = j.str; return j.str_s; //mo::gettext
		}
		inline const char *bilingual_id()const
		{
			if(ctxt_s) return id; 
			mo::range_t r; mo::range(tree.nihongo.mo,r);
			size_t i = mo::find(r,id);
			if(i!=r.n) return mo::msgid(r,i); return id;
		}
		inline const char *mmapid(int _=1)const
		{
			mo::range_t r; mo::nrange(tree.right.mo,r);
			const char *_out = mmap&&mmap<r.n?mo::msgid(r,mmap):0;
			if(!_out) assert(_); return _out;
		}		
		inline int nocontext()const 
		{				
			return ctxt_s==1&&(unsigned char)*id<'\4'?*id:0;
		}
		inline bool gettext_header_entry()const
		{
			assert(ctxt_s!=1||nocontext());	return ctxt_s==1;			
		}
		const char *exml_attribute(EXML::Attribs at)const
		{
			EXML::Attribuf<8> attribs(str,str_s);
			const char *out = SOM::exml_attribute(at,attribs);
			if(!out&&bilingual())
			{
				const char *jstr; size_t jstr_s =
				//mo::gettext(tree.nihongo.mo,id,jstr);
				bilingual_gettext(jstr); //optimizing
				attribs.poscharwindow(jstr,jstr_s); assert(jstr_s);
				out = SOM::exml_attribute(at,attribs);
			}
			return out;
		}		
		const char *exml_text_component()const
		{
			if(ctxt_s||!tree.nihongo.mo.claim(str))
			{
				EXML::Attribuf<8> attribs(str,str_s); 
				return SOM::exml_text_component(attribs);
			}
			else assert(0);	return 0; //use SOM_MAIN_libintl
		}		
		inline bool temporary()const
		{
			return &tree.temp==this;
		}

	}temp, clip; //SOM_MAIN_temparam/cliparam
	
	inline LPARAM string_param(const string &a){ return (LPARAM)&a; }

	inline string &operator[](LPARAM lp) 
	{
		assert(lp>65535); return *(string*)lp;
	}

	//REMOVE ME??
	//this was to save the msgid
	//but now mmap is being used for that
	//now it is only used to quickly look up msgstr
	struct jstring : string //nihongo.mo
	{	
		//==: Dinkumware validation vagary
		inline jstring(const mo::range_t &r):string(r){}
		inline bool operator<(const string &b)const{ return id<b.id; }
		inline bool operator==(const string &b)const{ return id==b.id; }	
	};
	struct jtable : std::vector<const jstring> //singleton
	{
		inline const_iterator find(const string &val)const
		{
			return std::find(begin(),end(),val);
		}

	}jtable;

	//dictionary	
	struct group : std::string
	{			
		group(const char *p):std::string(p){};
		inline bool operator<(const std::string &b)const
		{
			const std::string &a = *this;
			int i; for(i=0;a[i]>'\4'&&b[i]>'\4'&&a[i]==b[i];i++);
			return a[i]<b[i];
		}
	};		
	struct groups : std::set<group> //singleton 
	{	
		template<int N> 
		void fill(char (&with)[N])
		{
			//adapted from SOM_EDIT.cpp
			for(int i=0,j=0,k=-1,l=1;i<N;i++) switch(with[i])
			{		
			default: if(k<j&&with[i]<0) return; break; //paranoia

			case '\0': i = N-1; 			
			case '\r': case '\n': with[i] = '\0';	
				
				//contexts end with \4 and can be up to 8 bytes
				//\1, \2, \3 are used by Metadata, Messages, and Japanese
				if(i>j&&k>=j&&k-j<=SOM_MAIN_msgctxt_s) //8
				{
					with[k] = k==j?l++:'\4'; insert(with+j); 
				}
				j = i+1;

			case '=': k = i;
			}
		}	
		__declspec(noinline) //chkstk workaround
		void fill(HWND with) 
		{	
			enum{ x_s=4096 }; HWND language = GetWindow
			(GetDlgItem(with,1023),GW_HWNDNEXT); if(language) 
			{
				wchar_t x[x_s] = L""; char u[4*x_s] = "";
				if(GetWindowTextW(language,x,x_s)>=x_s-1) assert(0);			
				EX::need_ansi_equivalent(65001,x,u,sizeof(u));
				fill(u); DestroyWindow(language);
			}			
		}

	}groups; //singleton
	
	//dead links workaround 
	typedef std::pair<LPARAM,unsigned> link;
	static struct links:stdext::hash_map<LPARAM,unsigned>{}links;

	//multi: allowing for msgid collisions
	typedef std::multiset<string> poolbase;	
		
	//treeview memory pool
	struct pool : poolbase
	{
		mo::image_t mo; int copy(),paste();

		pool(){ memset(&mo,0x00,sizeof(mo)); }

		inline LPARAM insert_param(const string &cp)
		{
			return tree.string_param(*insert(cp));
		}
		link link_param(LPARAM lp)
		{
			if(mo.readonly()) return link(lp,0); static unsigned n = 1;
			unsigned &out = links[lp]; if(!out) out = n++; return link(lp,out);
		}		
		void erase_param(LPARAM lp)
		{
			const string &i = tree[lp];
			iterator it = find(i); while(it!=end()&&&*it!=&i) it++;
			if(&*it==&i) erase(it); else assert(0);	if(mo.readonly()) return;
			links::iterator jt = links.find(lp); if(jt!=links.end()) links.erase(jt);
		}
		LPARAM follow_link(const link &safe)
		{
			if(!safe.second) return safe.first;
			links::iterator it = links.find(safe.first);
			return it!=links.end()&&it->second==safe.second?safe.first:0;
		}
		template<class T> inline LPARAM follow_link(const T &t)
		{
			int compile[sizeof(T)==sizeof(link)]; return follow_link((const link&)t);
		}
		void redraw(HTREEITEM i)
		{
			RECT ir; do{ TreeView_GetItemRect(treeview,i,&ir,0)&&InvalidateRect(treeview,&ir,1);
			}while(TVIS_CUT&TreeView_GetItemState(treeview,i=TreeView_GetParent(treeview,i),~0));
		}		

		//top level items
		LPARAM metadata, messages, japanese;

		struct outlinestats
		{
			outlinestats():levels(0){}
			int levels, countperlevels[SOM_MAIN_levels_s]; int count()
			{
				int out = 0; for(int i=0;i<levels;i++) out+=countperlevels[i]; return out;
			}
			void addtolevel(int treeview_level, int n=1)
			{
				int lv = treeview_level-1; assert(lv>=0);
				if(lv<SOM_MAIN_levels_s) countperlevels[lv]+=n;	if(lv>=levels) levels = lv+1;
				if(!countperlevels[lv]&&levels==lv+1) levels = lv;
			}
		};
		std::vector<outlinestats> outlines;

		std::deque<std::wstring> contexts; LPARAM context;

		void clear() //0 is directory listing mode			
		{			
			if(mo.writable()) links.clear();
			mo.mode = 0; metadata = messages = japanese = 0; 
			poolbase::clear(); outlines.clear(); contexts.clear(); 
		}				
		void add_context(SOM_MAIN_tree::string &msg)
		{
			mo::range_t r; size_t _4 = strcspn(msg.id,"\n\4");			
			if(msg.id[_4]=='\4'&&!mo::range(mo,r,msg.id,_4+1))
			contexts.push_back(SOM_MAIN_utf8to16(msg.id,_4+1));			
		}

		SOM_MAIN_cpp::layout layouts[2]; //fixed & compact

		void reparent(HWND p) //&&: flickers
		{ 
			if(p!=GetParent(treeview)&&SetParent(treeview,p))
			EnableWindow(treeview,IsWindowEnabled(p));
		}
		HWND treeview; inline pool *operator->() 
		{
			return &(&tree.left)[this==&tree.left];
		}		

	}left, right, *focus;
	inline pool *operator->(){ return focus; }
	inline pool &operator*(){ return *focus; }
	inline HWND operator=(HWND tv)
	{
		focus = &operator[](tv); return tv;
	}		
	inline pool &operator[](HWND tv) 
	{
		return tv==right.treeview?right:left;
	}	
	
}SOM_MAIN_tree; //singleton	 

struct SOM_MAIN_tree::links SOM_MAIN_tree::links;								 
struct SOM_MAIN_tree &SOM_MAIN_tree::tree = ::SOM_MAIN_tree;

static SOM_MAIN_tree::pool *SOM_MAIN_rtl = 0; 
static SOM_MAIN_tree::pool &SOM_MAIN_right = SOM_MAIN_tree.right;
static HTREEITEM SOM_MAIN_insertitem(HWND tv, TVINSERTSTRUCTW *tis)
{
	if(tis->item.lParam<400000||!tis->item.cChildren) 
	return TreeView_InsertItem(tv,tis);
	SOM_MAIN_tree::string &msg = SOM_MAIN_tree[tis->item.lParam];
	if(tv==SOM_MAIN_right.treeview)	
	return msg.right = TreeView_InsertItem(tv,tis);	
	return msg.left = TreeView_InsertItem(tv,tis);
}
static void SOM_MAIN_deleteitem(HWND tv, HTREEITEM ti)
{
	HTREEITEM gp = TreeView_GetParent(tv,ti); //winsanity
	if(TreeView_DeleteItem(tv,ti)&&!TreeView_GetChild(tv,gp))
	{
		TreeView_Expand(tv,gp,TVE_COLLAPSE|TVE_COLLAPSERESET);
		TreeView_SetItemState(tv,gp,0,TVIS_EXPANDED);
	}
}
inline bool SOM_MAIN_left(HWND tv)
{
	return tv==SOM_MAIN_tree.left.treeview;
}
static HWND SOM_MAIN_escaped = 0;
static void SOM_MAIN_escape(int r=-1);
static VOID CALLBACK SOM_MAIN_escape(HWND win, UINT, UINT_PTR id, DWORD)
{
	KillTimer(win,id); SOM_MAIN_escape(id);
}
static bool SOM_MAIN_leftonly()
{
	if(som_tool==SOM_MAIN_152())
	if(SOM_MAIN_escaped!=SOM_MAIN_right.treeview)
	return !IsWindowVisible(SOM_MAIN_right.treeview); 
	return false;
}		  
#define SOM_MAIN_RTLPOS \
HWND r = SOM_MAIN_right.treeview;\
HWND l = SOM_MAIN_tree.left.treeview;\
if(SOM_MAIN_rtl!=&SOM_MAIN_right) std::swap(l,r);\
RECT &lpos = SOM_MAIN_tree[l].layouts[1];\
RECT &rpos = SOM_MAIN_tree[r].layouts[1];\
if(SOM_MAIN_leftonly()) std::swap(l,r);

inline bool SOM_MAIN_readonly() 
{
	return SOM_MAIN_right.mo.readonly();
}
inline bool SOM_MAIN_readonly(HWND tv) 
{
	return SOM_MAIN_tree[tv].mo.readonly();
}
inline bool SOM_MAIN_same() //transcript mirror mode
{
	return SOM_MAIN_right.mo.set==SOM_MAIN_tree.left.mo.set
	&&SOM_MAIN_tree.right.mo.set;
}
inline bool SOM_MAIN_lang(HWND tv=0) //directory listing mode
{
	return !SOM_MAIN_tree.left.mo.mode&&(!tv||SOM_MAIN_left(tv));
}
static VOID CALLBACK SOM_MAIN_focus(HWND win, UINT, UINT_PTR id, DWORD)
{
	KillTimer(win,id); if(GetFocus()!=win&&IsWindow(win)) SetFocus(win);
}
static struct //SOM_MAIN_before
{struct state{void operator()(HWND tv)
{
	if(!tv) //double dragon
	{
		if(SOM_MAIN_same())	
		tv = SOM_MAIN_tree.left.treeview;
		(this+1)->operator()(SOM_MAIN_right.treeview);		
		if(!tv) return;
	}

	if(count++) //NEW: prevent re-entry
	{
		if(count>=8) //suggests something's amiss
		{
			if(!EX::debug) //not good enough
			SOM_MAIN_error("MAIN199",MB_OK); assert(count<8);
		}
		return;
	}
	else focus = GetFocus(); 
	if(1) //testing
	{
		//probably best to do this this way 
		//until the kinks can be worked out

		if(!blank++) //optimizing
		{
			HWND ch = GetWindow(tv,GW_CHILD); 
			for(;ch;ch=GetWindow(ch,GW_HWNDNEXT)) ShowWindow(ch,0);			
		}
		return; //!
	}
	TVITEMEXW tvi = {TVIF_PARAM|TVIF_CHILDREN};	
	HTREEITEM &i = tvi.hItem = TreeView_GetFirstVisible(tv);
	for(LPARAM pre=0;i;i=TreeView_GetNextVisible(tv,i))
	if(TreeView_GetItem(tv,&tvi)&&!tvi.cChildren&&pre!=tvi.lParam)
	ShowWindow((HWND)(pre=tvi.lParam),0);
}HWND focus; int count; bool blank; 
state(){ memset(this,0,sizeof(*this)); }
}states[2]; void operator()(HWND tv=0){ operator[](tv)(tv); }
state &operator[](HWND tv){ return states[tv==SOM_MAIN_right.treeview]; }
}SOM_MAIN_before; // = {{0,0,false};
static VOID CALLBACK SOM_MAIN_and_after(HWND tv, UINT, UINT_PTR id, DWORD)
{
	KillTimer(tv,id); SOM_MAIN_before[tv].blank = false;
	TVITEMEXW tvi = {TVIF_PARAM|TVIF_CHILDREN};	RECT ir;
	HTREEITEM &i = tvi.hItem = TreeView_GetFirstVisible(tv); 
	TreeView_GetItemRect(tv,i,&ir,0);
	LONG ih = TreeView_GetItemHeight(tv); 
	for(LPARAM pre=0;i;i=TreeView_GetNextVisible(tv,i))
	if(TreeView_GetItem(tv,&tvi)&&!tvi.cChildren&&tvi.lParam!=pre)	
	{			
		HWND re = (HWND)(pre=tvi.lParam); if(!re) continue;
		HTREEITEM ipos = (HTREEITEM)GetWindowLong(re,GWL_USERDATA);
		if(TreeView_GetItemRect(tv,ipos,&ir,0)) //collapsed?
		{
			SetWindowPos(re,0,0,ir.top+ih,0,0,SWP_NOZORDER|SWP_NOSIZE);
			ShowWindow(re,1); //SWP_SHOWWINDOW (docs suggest don't???)
			InvalidateRect(re,0,0); //NEW
		}
	}
}
static void SOM_MAIN_and_after(HWND tv=0)	
{
	if(!tv) //double dragon
	{
		if(SOM_MAIN_same())	
		tv = SOM_MAIN_tree.left.treeview;
		SOM_MAIN_and_after(SOM_MAIN_right.treeview);		
		if(!tv) return;
	}
	int &count = SOM_MAIN_before[tv].count;
	assert(count>0); if(--count) return;
	SetTimer(tv,'b&a',0,SOM_MAIN_and_after);
	HWND &focus = SOM_MAIN_before[tv].focus;
	if(tv==GetParent(focus)) 
	SetTimer(focus,'b&a',50,SOM_MAIN_focus); 
	focus = 0;
}
struct SOM_MAIN_before_and_after //RAII
{
	HWND hwnd; //bool ifso;
	//=0: BEWARE THE "MOST VEXING PARSE"
	SOM_MAIN_before_and_after(HWND tv=0) 
	{
		/*if(ifso=so)*/ SOM_MAIN_before(hwnd=tv); 
	}
	~SOM_MAIN_before_and_after()
	{
		/*if(ifso)*/ SOM_MAIN_and_after(hwnd); 
	}
};

static void SOM_MAIN_reintegrate(HWND tv, HWND em, int i)
{
	SOM_MAIN_before_and_after raii(tv);

	int scroll_stops = 6; //const
	//mainly for when divider is closing zoomed text
	DWORD n,d = 0; SendMessage(em,EM_GETZOOM,(WPARAM)&n,(LPARAM)&d);
	if(d&&n>d) scroll_stops = scroll_stops*n/d; assert(scroll_stops>=6);

	TVINSERTSTRUCTW tis = 
	{0,TVI_LAST,{TVIF_INTEGRAL|TVIF_PARAM|TVIF_TEXT}};
	tis.item.lParam = (LPARAM)em; tis.item.pszText = L"";
	tis.hParent = (HTREEITEM)GetWindowLong(em,GWL_USERDATA);
	if(!tis.hParent) return;
	HTREEITEM &it = tis.item.hItem = TreeView_GetChild(tv,tis.hParent);
	for(;i>0;i-=scroll_stops) 
	{
		int ii = i<scroll_stops?i:scroll_stops;
		//see SOM_EDIT_refresh for bug description
		//tvi.iIntegral = i<scroll_stops?i:scroll_stops;				
		if(it) //TreeView_SetItem(tv,&tvi);
		{
			TreeView_GetItem(tv,&tis.item);		
			if(ii!=tis.itemex.iIntegral) //important			
			{
				tis.hInsertAfter = it;
				tis.itemex.iIntegral = ii;
				it = TreeView_InsertItem(tv,&tis);				
				TreeView_DeleteItem(tv,tis.hInsertAfter);
				tis.hInsertAfter = TVI_LAST;
			}
			it = TreeView_GetNextSibling(tv,it);
		}
		else
		{
			tis.itemex.iIntegral = ii;
			TreeView_InsertItem(tv,&tis);
		}	
	}	
	for(HTREEITEM i;i=it;TreeView_DeleteItem(tv,i))
	{
		it = TreeView_GetNextSibling(tv,i);
	}
}
//REMOVE ME? if EM_REQUESTRESIZE works 
static void SOM_MAIN_requestresize(HWND tv, HWND ta, int h=-1)
{	
	if(h==-1) //testing
	{
		//KEEPING metadata (top item) CLOSED UP
		//SendMessage(ta,EM_REQUESTRESIZE,0,0);
		//return;
	}

	//adapted from EN_REQUESTRESIZE
	RECT cr; GetClientRect(ta,&cr);
	bool hack = h<0; if(hack) h = cr.bottom; 
	int ih = TreeView_GetItemHeight(tv);
	int i = h/ih; if(!i||h%ih>ih/6) i++;	
	if(hack||cr.bottom!=ih*i) //spamming on change
	{
		GetClientRect(tv,&cr);
		SetWindowPos(ta,0,0,0,cr.right,ih*i,SWP_NOMOVE|SWP_NOZORDER);			
		SOM_MAIN_reintegrate(tv,ta,i);			
	}
}
static VOID CALLBACK SOM_MAIN_sizeup(HWND tv, UINT, UINT_PTR id, DWORD)
{
	KillTimer(tv,id); SOM_MAIN_before_and_after raii(tv);
	
	//200: assuming divider is closing
	RECT r; GetClientRect(tv,&r); int w = max(r.right,200);
	for(HWND ch=GetWindow(tv,GW_CHILD);ch;ch=GetWindow(ch,GW_HWNDNEXT))
	if(!IsIconic(ch)&&GetWindowRect(ch,&r)&&MapWindowRect(0,tv,&r))
	MoveWindow(ch,0,r.top,w,r.bottom-r.top,0);
} 
static VOID CALLBACK SOM_MAIN_cleanup(HWND tv, UINT, UINT_PTR id, DWORD)
{
	KillTimer(tv,id); 
	HWND x, ch = GetWindow(tv,GW_CHILD); while(x=ch) 
	{
		ch = GetWindow(ch,GW_HWNDNEXT);
		if(TVIS_EXPANDED&~TreeView_GetItemState
		(tv,(HTREEITEM)GetWindowLong(x,GWL_USERDATA),~0))
		{
			SOM_MAIN_reintegrate(tv,x,0); 
			DestroyWindow(x); 
		}
	}
}
 
static LPARAM SOM_MAIN_param(HWND tv, HTREEITEM item)
{							
	TVITEMEXW tvi = {TVIF_PARAM,item};	
	if(!TreeView_GetItem(tv,&tvi)) return 0;
	return tvi.lParam;
}
static TVITEMEXW SOM_MAIN_label_ti = {TVIF_TEXT};
static wchar_t *SOM_MAIN_label(HWND tv, HTREEITEM item, TVITEMEXW &ti=SOM_MAIN_label_ti)
{
	const int tabstop = SOM_MAIN_msgctxt_s+1; //173

	static wchar_t out[tabstop+260]; //260: display limit
	ti.hItem = item; ti.pszText = out+tabstop; ti.cchTextMax = 260;
	*out = '\0'; TreeView_GetItem(tv,&ti); return out+tabstop;
}
struct SOM_MAIN_textarea //the leaves
{	
	HWND hwnd; HTREEITEM item; LPARAM param; 
	SOM_MAIN_textarea(HWND tv, HTREEITEM ti) //leaf?
	{		
		TVITEMEXW tvi = {TVIF_CHILDREN|TVIF_PARAM,ti};	
		if(!TreeView_GetItem(tv,&tvi)) tvi.cChildren = 0; 
		new(this)SOM_MAIN_textarea(tvi.cChildren?0:(HWND)tvi.lParam);		
	}
	SOM_MAIN_textarea():hwnd(0){}
	SOM_MAIN_textarea(HWND ta):hwnd(ta)
	{
		item = (HTREEITEM)GetWindowLong(hwnd,GWL_USERDATA);
		param = GetWindowID(hwnd);
	}	
	inline operator HWND(){ return hwnd; }
	inline HWND treeview(){ return GetParent(hwnd); }
	
	int menu(), close(int f5=0);
};	  
static DWORD SOM_MAIN_select_menu(HWND ed)
{
	DWORD out = Edit_GetSel(ed)+1;
	Edit_SetSel(ed,0,SOM_MAIN_textarea(ed).menu());
	return out;
}
static DWORD SOM_MAIN_select_text(HWND ed, size_t len=0)
{
	SOM_MAIN_textarea ta(ed); 
	assert(ta.param); if(!ta.param) return 0;	
	DWORD out = Edit_GetSel(ed)+1, menu = ta.menu(); 
	Edit_SetSel(ed,menu+1,len?menu+1+len:-1);
	return out;
}
static void SOM_MAIN_and_unselect(HWND ed, DWORD sel)
{
	if(sel--) Edit_SetSel(ed,LOWORD(sel),HIWORD(sel));
}

static bool SOM_MAIN_changed_new = true;
static void SOM_MAIN_changed(int change=1) //PSM_CHANGED
{	
	if(!SOM_MAIN_changed_new||change<0) 
	{
		static int change_counter = 0;	
		bool enable = (change_counter+=change)>0;	
		windowsex_enable<12321>(SOM_MAIN_151(),enable); 
		if(change_counter<0) change_counter = 0;
	}
}
inline void SOM_MAIN_changed(HWND tv, HTREEITEM item)
{
	assert(!SOM_MAIN_readonly(tv));
	SOM_MAIN_changed(TVIS_BOLD&TreeView_GetItemState(tv,item,~0)?0:1);	
}
inline void SOM_MAIN_changed(HTREEITEM item)
{
	SOM_MAIN_changed(SOM_MAIN_right.treeview,item);	
}
static int SOM_MAIN_level(HWND tv, HTREEITEM item)
{
	int out = 0; while(item=TreeView_GetParent(tv,item)) out++; return out;
}
static HTREEITEM SOM_MAIN_toplevelitem(HWND tv, HTREEITEM item, int *lv=0)
{
	if(lv) *lv = 0; for(HTREEITEM i;i=TreeView_GetParent(tv,item);item=i)
	if(lv) (*lv)++; return item;
}
inline LPARAM SOM_MAIN_toplevelparam(HWND tv, HTREEITEM item, int *lv=0)
{
	return SOM_MAIN_param(tv,SOM_MAIN_toplevelitem(tv,item,lv));
}
static bool SOM_MAIN_7bitshortcode(const char *ctxt, size_t ctxt_s, HWND tv=0)
{
	if(!ctxt_s||ctxt_s>SOM_MAIN_msgctxt_s+1) return false;
	for(size_t i=0;i<ctxt_s;i++) if(ctxt[i]<'\4') return false;
	char test[SOM_MAIN_msgctxt_s+1]; strncpy_s(test,ctxt,ctxt_s);
	return (tv?SOM_MAIN_tree[tv]:SOM_MAIN_right).count(test);	
}

inline LPARAM SOM_MAIN_temparam()
{
	return SOM_MAIN_tree.string_param(SOM_MAIN_tree.temp);
}
static LPARAM SOM_MAIN_temparam(HWND tv, HTREEITEM ti)
{
	SOM_MAIN_tree::string &out = SOM_MAIN_tree.temp;
	SOM_MAIN_tree::string &msgctxt = SOM_MAIN_tree[SOM_MAIN_toplevelparam(tv,ti)];
	new(&out)SOM_MAIN_tree::string; if(!msgctxt.nocontext())
	{ out.id = msgctxt.id; out.id_s = msgctxt.id_s; out.ctxt_s = msgctxt.ctxt_s; }
	else out.id = msgctxt.id+msgctxt.id_s; 
	(SOM_MAIN_left(tv)?out.left:out.right) = ti; //placeholder
	return SOM_MAIN_tree.string_param(out);
}  
#define SOM_MAIN_msgfmt(x,y) SOM_MAIN_po(x,y,'load')
static bool SOM_MAIN_po(const char*,const char*,int=0);
static int SOM_MAIN_po_unquote(const char*,const char*eof=0,char*pp=0);
static char *SOM_MAIN_po_keyword(char *p, const char *kw)
{
	return strstr(p,kw); //TODO: STAY OUTSIDE QUOTES
}
static struct SOM_MAIN_clip : private std::vector<char> //singleton
{	
	const char *po; bool fresh; int format; 
	SOM_MAIN_clip(){ po = 0; fresh = format = 0; }

	//c_str()[-1] is = if not empty
	inline bool empty(){ return vector::size()<=1; }
	inline size_t size()
	{
		return empty()?0:vector::size()-1-!vector::back();
	}	
	inline const char *c_str()
	{
		if(empty()) return " "+1; //boolean
		if(vector::back()) vector::push_back('\0'); 
		vector::at(0) = '='; //assignment
		return &vector::at(1);
	}
	bool refresh(HWND hwndDlg)
	{
		if(fresh++) return po; 		
		if(format=OpenClipboard(hwndDlg)) 
		{		
			format = CF_UNICODETEXT;
			HANDLE got = GetClipboardData(format);
			if(!got) format = 0;
			WCHAR *lock = (WCHAR*)GlobalLock(got);
			if(lock&&*lock==0xfeff) lock++; //BOM?
			if(!lock) lock = L""; 
			vector::resize(1+WideCharToMultiByte(65001,0,lock,-1,0,0,0,0));
			if(size())
			WideCharToMultiByte(65001,0,lock,-1,&vector::at(1),size(),0,0);
			if(got) GlobalUnlock(got);
			CloseClipboard();		
		}
		else vector::resize(1);
		po = empty()?0:c_str();
		if(po&&!SOM_MAIN_po(po,po+size())) po = 0; //!!
		return po;
	}	

}SOM_MAIN_clip; //singleton	 

static LPARAM SOM_MAIN_cliparam(HWND tv)
{
	SOM_MAIN_clip.refresh(GetParent(tv));
	SOM_MAIN_tree::string &out = SOM_MAIN_tree.clip;
	new(&out)SOM_MAIN_tree::string; out.id = ""; 
	if(SOM_MAIN_clip.po)
	{
		static std::vector<char> buf;
		buf.resize(SOM_MAIN_clip.size()+1);
		char *p = strcpy(&buf[0],SOM_MAIN_clip.po);		
		char *id = SOM_MAIN_po_keyword(p,"msgid");
		char *str = SOM_MAIN_po_keyword(p,"msgstr");
		char *ctxt = SOM_MAIN_po_keyword(p,"msgctxt");
		//if(ctxt>id) ctxt = 0;

		char *eof = p+buf.size();
		if(ctxt) ctxt = strchr(ctxt,'"')+1;
		out.ctxt_s = ctxt?SOM_MAIN_po_unquote(ctxt,eof)+1:0;
		if(ctxt)
		{
			out.id = p; memmove(p,ctxt,out.ctxt_s);
			p[out.ctxt_s-1] = '\4';	p[out.ctxt_s] = '\0';
		}
		if(id) id = strchr(id,'"')+1;
		out.id_s = out.ctxt_s+(id?SOM_MAIN_po_unquote(id,eof):0);
		if(id)
		{
			out.id = p; memmove(p+out.ctxt_s,id,out.id_s-out.ctxt_s);
			p[out.id_s] = '\0';
		}
		if(str) str = strchr(str,'"')+1;
		out.str_s = str?SOM_MAIN_po_unquote(str,eof):0;
		if(str)
		{
			out.str = str; str[out.str_s] = '\0';
		}
		if(!SOM_MAIN_7bitshortcode(out.id,out.ctxt_s,tv)) 
		out.ctxt_s = 0;
	}
	else //default to filled out msgid
	{
		out.id = SOM_MAIN_clip.c_str();
		out.id_s = SOM_MAIN_clip.size();
	}
	return SOM_MAIN_tree.string_param(out);
}
inline LPARAM SOM_MAIN_cliparam()
{
	return SOM_MAIN_tree.string_param(SOM_MAIN_tree.clip);
}

namespace SOM_MAIN_cpp //MRU
{
	typedef std::multimap<time_t,std::wstring> mru_t;
	typedef mru_t::iterator mru_it;
	typedef mru_t::value_type mru_vt; static mru_t mru; 
}
//all separates combobox 1011
static size_t SOM_MAIN_all = 0;
static void SOM_MAIN_mru(HWND cb1, HWND cb2, int filter=0)
{
	static bool one_off = false; if(!one_off++) //???
	{	
		HKEY hk = 0; 
		if(RegOpenKeyEx(HKEY_CURRENT_USER,L"SOFTWARE\\FROMSOFTWARE\\SOM_MAIN",0,KEY_READ,&hk))
		return;
		wchar_t path[MAX_PATH]; time_t t; 
		path[MAX_PATH-1] = '\0'; //paranoia
		for(DWORD i=0,type,r,s,t_s;ERROR_NO_MORE_ITEMS!=
		(r=RegEnumValue(hk,i,path,&(s=MAX_PATH),0,&type,(BYTE*)&(t=0),&(t_s=sizeof(t))));i++)
		if(!r&&type==REG_BINARY&&!path[MAX_PATH-1]) //paranoia
		SOM_MAIN_cpp::mru.insert(std::make_pair(t,path));
		RegCloseKey(hk);
	}

	//all is the first line of the left combobox
	int sep = ComboBox_GetCount(cb1), all = sep;
	int ins = 1013==GetWindowID(cb2); //NEW: New
		
	HWND ex = //Common Dialog's ComboBoxEx
	(HWND)SendMessage(cb1?cb1:cb2,CBEM_GETCOMBOCONTROL,0,0);
	if(ex&&!IsWindow(ex)) ex = 0;
	
	//reminder: !ex could still be Common Dialog?
	COMBOBOXEXITEM cbi={CBEIF_TEXT|CBEIF_LPARAM,0};
	LPARAM &l = cbi.lParam; INT_PTR &i = cbi.iItem;
	const wchar_t* &c = (const wchar_t*&)cbi.pszText;	
	for(SOM_MAIN_cpp::mru_it it=SOM_MAIN_cpp::mru.begin();it!=SOM_MAIN_cpp::mru.end();it++)
	{			
		c = it->second.c_str();
		size_t c_s = it->second.size(); 		
		bool mo = c_s>=3&&!wcsicmp(c+c_s-3,L".mo"); l = (LPARAM)&*it;
		if(cb1&&(!filter||filter==1+mo))
		{				
			if(!mo) sep++; i = mo?sep:all; 			
			if(ex) SendMessage(cb1,CBEM_INSERTITEM,0,(LPARAM)&cbi);
			if(!ex) ComboBox_SetItemData(cb1,ComboBox_InsertString(cb1,i,c),l);
		}
		if(cb2&&mo)	
		{
			if(ex) SendMessage(cb2,CBEM_INSERTITEM,0,(LPARAM)&cbi); 
			if(!ex) ComboBox_SetItemData(cb2,ComboBox_InsertString(cb2,ins,c),l);
		}
	}

	//hack: keeping track of the lang folder/mo file separator
	if(all==1&&!filter) SOM_MAIN_all = sep; else assert(!all);
}
static int SOM_MAIN_use(wchar_t path[MAX_PATH], HWND cb)
{
	if(!*path) return -1;
	Sompaste->path(path);
	//lang folder/mo file separator
	bool all = 1011==GetWindowID(cb);
	bool mo = !wcsicmp(L".mo",PathFindExtension(path));
	time_t now = time(0);	
	if(all&&mo) //stop auto-opening left on right 
	{
		HWND cb2 = GetDlgItem(SOM_MAIN_151(),1013);
		if(ComboBox_GetCurSel(cb2)>0)
		{
			wchar_t path2[MAX_PATH] = L"";
			if(GetWindowTextW(cb2,path2,MAX_PATH))			
			if(wcsicmp(path,path2)&&PathFileExists(path))
			now-=LLONG_MAX; int compile[sizeof(now)==8];
		}
	}
	SHSetValueW(HKEY_CURRENT_USER,
	L"SOFTWARE\\FROMSOFTWARE\\SOM_MAIN",path,REG_BINARY,&now,sizeof(now));		
	int found = ComboBox_FindStringExact(cb,-1,path);
	if(found!=CB_ERR)
	{
		SOM_MAIN_cpp::mru.erase
		(((SOM_MAIN_cpp::mru_vt*)
		ComboBox_GetItemData(cb,found))->first);
		ComboBox_DeleteString(cb,found);
	}		
	int out = all&&mo?SOM_MAIN_all:1; //1: All/New
	{
		SOM_MAIN_cpp::mru_it ins = 
		SOM_MAIN_cpp::mru.insert(std::make_pair(now,path));
		ComboBox_SetItemData(cb,out=
		ComboBox_InsertString(cb,out,path),(LPARAM)&*ins);	
	}
	if(all&&!mo) //hack: separator
	if(out!=CB_ERR&&found==CB_ERR) SOM_MAIN_all++;	
	return out;
}
static void SOM_MAIN_selendok(int cbid, const wchar_t *path)
{
	//simulate typing into combobox
	HWND cb = GetDlgItem(SOM_MAIN_151(),cbid);
	ComboBox_SetCurSel(cb,-1); SetWindowTextW(cb,path); 
	SendMessageW(SOM_MAIN_151(),WM_COMMAND,MAKEWPARAM(cbid,CBN_SELENDOK),(LPARAM)cb);
}
static void SOM_MAIN_reset(HWND tv)
{	
	SetWindowRedraw(tv,0); //cleanup?
	if(SOM_MAIN_same()&&SOM_MAIN_left(tv)
	  &&TreeView_GetCount(SOM_MAIN_right.treeview)) 
	{
		HTREEITEM item = TreeView_GetChild(tv,TVI_ROOT);
		do TreeView_Expand(tv,item,TVE_COLLAPSE|TVE_COLLAPSERESET);
		while(item=TreeView_GetNextSibling(tv,item));
		SOM_MAIN_cleanup(tv,0,0,0);		
	}
	TreeView_DeleteAllItems(tv);	
	for(HWND x,ch=GetWindow(tv,GW_CHILD);x=ch;DestroyWindow(x))
	ch = GetWindow(ch,GW_HWNDNEXT);	
	SetWindowRedraw(tv,1); //sends TVN_GETDISPINFO!
}
static void SOM_MAIN_mirror()
{	
	SOM_MAIN_reset(SOM_MAIN_tree.left.treeview);
	SetWindowRedraw(SOM_MAIN_tree.left.treeview,0);
	SOM_MAIN_tree.left.mo = SOM_MAIN_right.mo; //SOM_MAIN_same
	SOM_MAIN_tree.left.outlines.clear();
	TVINSERTSTRUCTW tis = {TVI_ROOT,TVI_LAST,
	{TVIF_TEXT|TVIF_PARAM|TVIF_CHILDREN|TVIF_STATE,0,0,TVIS_CUT}};
	//0: seems to work like LVIF_NORECOMPUTE
	tis.item.pszText = LPSTR_TEXTCALLBACKW; tis.item.cchTextMax = 0; 
	HTREEITEM &i = tis.item.hItem;
	if(i=TreeView_GetChild(SOM_MAIN_right.treeview,TVI_ROOT))
	do if(TreeView_GetItem(SOM_MAIN_right.treeview,&tis.item)) 
	SOM_MAIN_insertitem(SOM_MAIN_tree.left.treeview,&tis); else assert(0);
	while(i=TreeView_GetNextSibling(SOM_MAIN_right.treeview,i)); else assert(0);					
	SetWindowRedraw(SOM_MAIN_tree.left.treeview,1);
}
static bool SOM_MAIN_doublewide(int w, int h)
{
	int out = w>640; 		
	int bts[] = {1082,1018,1019};
	*bts+=!out&&SOM_MAIN_leftonly();
	windowsex_enable(SOM_MAIN_152(),bts,out);
	*bts^=1;
	windowsex_enable(SOM_MAIN_152(),*bts,*bts);
	return out;
}

static WNDPROC SOM_MAIN_opendir2 = 0;
static OPENFILENAMEW *SOM_MAIN_open = 0;
static wchar_t SOM_MAIN_opendirfilter[90] = L"";
static LRESULT CALLBACK SOM_MAIN_opendir(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{		
	switch(uMsg)
	{		
	case WM_COMMAND:

		//taken from Somtext.cpp
		//(todo: share Sompaste)
		if(LOWORD(wParam)==IDOK) //select directory
		if(SOM_MAIN_opendirfilter==SOM_MAIN_open->lpstrFilter)
		{
			if(GetFocus()!=(HWND)lParam&&!SOM_MAIN_open->nFileOffset)
			{
				//5: CRLF/surrogate pairs
				wchar_t test[5]; if(GetDlgItemTextW(hWnd,1148,test,5))
				break; //assuming navigating to folder via the combobox
			}

			//0: finishing up
			OPENFILENAMEW *ofn = SOM_MAIN_open;
			SOM_MAIN_open = 0;

			if(!ofn->nFileOffset)
			{					
				wchar_t *p = ofn->lpstrFile; 
				if(!GetDlgItemTextW(hWnd,1148,p,MAX_PATH)||PathIsRelativeW(p))
				{
					size_t cat = SendMessageW(hWnd,CDM_GETFILEPATH,MAX_PATH,(LPARAM)p);				
					if(GetDlgItemTextW(hWnd,1148,p+cat+1,MAX_PATH-cat-1)) p[cat] = '\\';
				}			
				ofn->nFileOffset = PathFindFileNameW(p)-p;
				ofn->nFileExtension = PathFindExtensionW(p)-p;					
			}
			else assert(0);
			
			EndDialog(hWnd,1); return 0; 
		}
				
		switch(wParam) 
		{
		case MAKEWPARAM(1148,CBN_DROPDOWN): //ComboBoxEx
		{
			ComboBox_ResetContent((HWND)lParam);	
			if(SOM_MAIN_opendirfilter==SOM_MAIN_open->lpstrFilter)
			{
				SOM_MAIN_mru((HWND)lParam,0,SOM_MAIN_open->nFilterIndex);
			}
			else if(SOM_MAIN_open->nFilterIndex==1)
			{
				SOM_MAIN_mru(0,(HWND)lParam); 
			}
			else break; //POT/PO or SOM_RUN options

			return 0; 
		}
		case MAKEWPARAM(1137,CBN_DROPDOWN): //ComboBox scrollbars
		{
			COMBOBOXINFO cbi = {sizeof(cbi)};
			if(GetComboBoxInfo((HWND)lParam,&cbi))
			SetWindowTheme(cbi.hwndList,L"",L""); else assert(0);
			break;
		}}
		break; 

	case WM_DESTROY: //the dialog crashes without this
		
		SendDlgItemMessage(hWnd,1148,CB_RESETCONTENT,0,0);
		break;
	}	
	return CallWindowProcW(SOM_MAIN_opendir2,hWnd,uMsg,wParam,lParam);
}
static UINT_PTR CALLBACK SOM_MAIN_openhook(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
				
		SOM_MAIN_open = (OPENFILENAMEW*)lParam;
		return 1;

	case WM_NOTIFY:
	{
		OFNOTIFYW *p = (OFNOTIFYW*)lParam;
				
		switch(p->hdr.code)
		{
		case CDN_INITDONE:
				
			//WHY NOT SetWindowSubclass?!
			SOM_MAIN_opendir2 = (WNDPROC)			
			SetWindowLongPtrW(p->hdr.hwndFrom,GWL_WNDPROC,(LONG_PTR)SOM_MAIN_opendir);			
			
			//NEW: auto-select default (also applies style)
			extern UINT_PTR CALLBACK SomEx_OpenHook(HWND,UINT,WPARAM,LPARAM);			
			return SomEx_OpenHook(hWnd,uMsg,wParam,lParam);

			//TODO: NEARLY IDENTICAL TO som_tool_openhook
			//TODO: NEARLY IDENTICAL TO som_tool_openhook
			//TODO: NEARLY IDENTICAL TO som_tool_openhook
			//TODO: NEARLY IDENTICAL TO som_tool_openhook
			//TODO: NEARLY IDENTICAL TO som_tool_openhook
								
		case CDN_SELCHANGE:

			//see Somtext.cpp for original implementation
			if(SOM_MAIN_opendirfilter==SOM_MAIN_open->lpstrFilter)
			{
				HWND SHELLDLL_DefView = 
				GetDlgItem(p->hdr.hwndFrom,1121);
				HWND lv = GetDlgItem(SHELLDLL_DefView,1);
				int sel = ListView_GetNextItem(lv,-1,LVNI_SELECTED);
				wchar_t buffer[MAX_PATH] = L"";				
				LVITEMW lvi = {LVIF_TEXT,0,0,0,0,buffer,MAX_PATH};
				if(sel!=-1) SendMessageW(lv,LVM_GETITEMTEXTW,sel,(LPARAM)&lvi);
				SetDlgItemTextW(p->hdr.hwndFrom,1148,buffer);
			}
			break;

		case CDN_FOLDERCHANGE: //not sure why this is necessary???
						
			if(SOM_MAIN_opendirfilter==SOM_MAIN_open->lpstrFilter)
			SetDlgItemTextW(p->hdr.hwndFrom,1148,L"");
		}
		break;
	}}	 
	return 0;
}
static HWND SOM_MAIN_clipboard = 0;
static HWND SOM_MAIN_clipboardview = 0;
static void SOM_MAIN_readytoserveview(HWND=0);
static bool SOM_MAIN_mo(const wchar_t*,HWND=0);
#define SOM_MAIN_save(...) SOM_MAIN_save_as(0,0)
static bool SOM_MAIN_save_as(const wchar_t*,int);
template<int N> //support.microsoft.com/kb/180077
static void SOM_MAIN_buttonup(HWND,const int(&)[N],int,int);
static bool SOM_MAIN_langdir(const wchar_t*,HWND,HTREEITEM=0,const wchar_t*spec=0);
static LRESULT CALLBACK SOM_MAIN_enrichproc(HWND,UINT,WPARAM,LPARAM,UINT_PTR,DWORD_PTR);
static LRESULT CALLBACK SOM_MAIN_richedproc(HWND,UINT,WPARAM,LPARAM,UINT_PTR,DWORD_PTR);
static LRESULT CALLBACK SOM_MAIN_commonproc(HWND,UINT,WPARAM,LPARAM,UINT_PTR,DWORD_PTR);
extern INT_PTR CALLBACK SOM_MAIN_151(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{	
	switch(uMsg) //clipboard
	{	
	case WM_INITDIALOG:
		
		if(wParam) //initializing
		{
			//WM_DRAWCLIPBOARD is generated?
			HWND cbv = SetClipboardViewer(hwndDlg);
			//assert(cbv==SOM_MAIN_clipboardview);
			SOM_MAIN_clipboardview = cbv;
		}
		break; //!!

	case WM_DRAWCLIPBOARD:
		
		SOM_MAIN_clip.fresh = false;
		SendMessage(SOM_MAIN_clipboardview,uMsg,wParam,lParam); 		
		SendMessage(SOM_MAIN_clipboard,SOM_MAIN_cpp::wm_drawclipboard,0,0);
		break; 
		
	case WM_CHANGECBCHAIN: 

		if((HWND)wParam!=SOM_MAIN_clipboardview) 
		SendMessage(SOM_MAIN_clipboardview,uMsg,wParam,lParam); 		
		else SOM_MAIN_clipboardview = (HWND)lParam; 
		//happens when more than one instance is opened
		//assert(SOM_MAIN_clipboardview!=(HWND)lParam);
		break;

    case WM_DESTROY: 		

        ChangeClipboardChain(hwndDlg,SOM_MAIN_clipboardview); 
		SOM_MAIN_clipboardview = 0; 
		break;
	}

	static wchar_t path[MAX_PATH+1] = L""; //chkstk

	switch(uMsg) //not the clipboard
	{
	case WM_INITDIALOG:
	{			
		ShowWindow(som_tool,0);
		EnableWindow(som_tool,0);
		SOM_MAIN_modes[0] = som_tool;		
		SOM_MAIN_modes[1] = som_tool = hwndDlg;		

		HICON icon = //128: the ID used by SOM's tools	
		LoadIcon(GetModuleHandle(0),MAKEINTRESOURCE(128));
		SendMessage(hwndDlg,WM_SETICON,ICON_SMALL,(LPARAM)icon);
		SendMessage(hwndDlg,WM_SETICON,ICON_BIG,(LPARAM)icon);

		SOM_MAIN_tree.groups.fill(hwndDlg); //chkstk workaround		

		//enable transitional help button
		SetWindowLong(hwndDlg,GWL_EXSTYLE,
		WS_EX_CONTEXTHELP|GetWindowExStyle(hwndDlg));

		//NEW/HACK
		//path: Sompaste->path crashes in Release mode
		const wchar_t *compact = Sompaste->get(L".MO");	  		
		if(*compact) compact = wcscpy(path,compact);
		typedef void path;

		//treeviews
		HWND left = GetDlgItem(hwndDlg,1022);
		HWND right = GetDlgItem(hwndDlg,1023);
		SOM_MAIN_tree = //set focus nonzero
		SOM_MAIN_tree.left.treeview = left;
		SOM_MAIN_tree.right.treeview = right;
		SOM_MAIN_tree.left.layouts[0] = left;
		SOM_MAIN_tree.right.layouts[0] = right;	
		SetWindowSubclass(left,SOM_MAIN_enrichproc,0,0); 
		SetWindowSubclass(right,SOM_MAIN_enrichproc,0,0); 

		SOM_MAIN_rtl = 
		SOM_MAIN_right.layouts[0].right
		<SOM_MAIN_tree.left.layouts[0].right
		?&SOM_MAIN_tree.left:&SOM_MAIN_right;

		//left combobox
		HWND cb1 = GetDlgItem(hwndDlg,1011); 
		ComboBox_AddString(cb1,som_932w_All);
		ComboBox_SetCurSel(cb1,0);

		//right combobox 
		HWND cb2 = GetDlgItem(hwndDlg,1013); 
		ComboBox_AddString(cb2,som_932w_New); 
		
		SOM_MAIN_mru(cb1,cb2); if(*compact) 
		ComboBox_SetCurSel(cb2,SOM_MAIN_use((wchar_t*)compact,cb2));		
		const wchar_t *lang =
		SOM::Tool::install(SOMEX_(A)"\\lang");		
		if(CB_ERR==ComboBox_FindStringExact(cb1,-1,lang))
		SOM_MAIN_use((wchar_t*)lang,cb1);
		ComboBox_AddString(cb1,L""); //mirror mode

		const int bts[] = {1009,1010,1211,1213};
		SOM_MAIN_buttonup(hwndDlg,bts,1080,1084);
		SetWindowSubclass(hwndDlg,SOM_MAIN_commonproc,0,0);	   
		
		bool slippery = *compact;

		//populate treeviews
		if(ComboBox_SetCurSel(cb2,1)<0) ComboBox_SetCurSel(cb2,0); //New
		SendMessageW(hwndDlg,WM_COMMAND,MAKEWPARAM(1011,CBN_SELENDOK),(LPARAM)cb1);		
		SendMessageW(hwndDlg,WM_COMMAND,MAKEWPARAM(1013,CBN_SELENDOK),(LPARAM)cb2);		

		if(slippery) //*compact WAS nonzero
		{
			SOM_MAIN_escape(1); 
			//SOM_MAIN_readytoserveview();	//moved to 152
		}
		else SetFocus((HWND)wParam);
		return 0;
	}	
	case WM_SYSCOMMAND:

		//REMOVE ME?
		if(wParam==SC_CONTEXTHELP)
		{
			const wchar_t *help = 
			L"This development tool is new to Sword of Moonlight. It was developed\n"
			L"over the course of five months and first released during February, 2015.\n"
			L"\n"
			L"The original plan called for two months, so there are unfinished pieces.\n"
			L"\n"
			L"To run SOM_RUN choose the .SOM file option in the \"Save As...\" menu.\n"
			L"\n"			
			L"The following keyboard combinations work in most cases:\n"
			L"\n"
			L"Delete. Ctrl+X to Cut. Ctrl+C to Copy. Ctrl+V to Paste.\n"
			L"Ctrl+B to Paste Below. Ctrl+Y, Ctrl+Shift+Z to Redo text edits.\n"
			L"Space to Open items. Space is equivalent to clicking with a \"mouse\".\n"
			L"Enter to Begin inputting a new item removed of inititial information.\n"
			L"Enter can also activate hyperlink-like areas within text.\n"
			L"Insert to Push an outline level out, creating a new level above.\n"
			L"F2 to Toggle the advanced editor screen from inside the item's text.\n"
			L"\n"
			L"These work in the flattened-out, column-based views only:\n"
			L"F2 to Open the text editor. Multiple selections will be queued up.\n"
			L"Shift+Backspace, Shift+Esc,\n"
			L"Backslash, Grave-accent to Go inside the primary selection's level.\n"
			L"Backspace, Escape to Exit to the outline level one level up.\n"
			L"1~9, 0 to Set the slider, limiting the number of levels on display.\n"
			L"Shift+Enter to Read without closing (this is subject to change.)\n"
			L"\n"
			L"F5 updates text in the game without having to save the script file.\n"
			/*L"\n" //sticks out below the initial release's layout
			L"Ctrl can enlarge text provided your \"mouse\" has a wheel!"*/;
			MessageBox(hwndDlg,help,L"SOM_MAIN",MB_OK|MB_ICONQUESTION);
			return 1;
		}
		break;

	case WM_COMMAND:
	
		switch(wParam)
		{		
		case MAKEWPARAM(1011,CBN_EDITCHANGE): //left combobox
		case MAKEWPARAM(1013,CBN_EDITCHANGE): //right combobox
		{	
			bool right = 1013==LOWORD(wParam);
			HWND tv = GetDlgItem(som_tool,1022+right); 
			if(right&&IsWindowEnabled(GetDlgItem(hwndDlg,12321)))
			{
				HWND cb = (HWND)lParam;	DWORD sel = ComboBox_GetEditSel(cb);
				int id = SOM_MAIN_warning("MAIN1000",MB_YESNOCANCEL);
				if(id!=IDNO) //restore the file name
				{					
					ComboBox_SetCurSel(cb,ComboBox_GetCurSel(cb));
					if(id==IDCANCEL||!SOM_MAIN_save())
					if(IDCANCEL==SOM_MAIN_warning("MAIN201",MB_OKCANCEL|MB_DEFBUTTON2))
					return 1; 		
				}
				else ComboBox_SetEditSel(cb,LOWORD(sel),HIWORD(sel));
			}
			int disabling[] = {1210,1211,12321};
			if(right) windowsex_enable(hwndDlg,disabling,0);
			SOM_MAIN_reset(tv);
			return 1;								 
		}	
		case MAKEWPARAM(1011,CBN_SELENDOK): //left tree
		case MAKEWPARAM(1013,CBN_SELENDOK): //right tree
		{
			bool right = 1013==LOWORD(wParam);

			HWND tv = GetDlgItem(som_tool,1022+right); 
			
			SetWindowRedraw(tv,0); //POINT OF NO RETURN

			SOM_MAIN_reset(tv); //DeleteAllItems and RichEdit boxes

			HWND cb = (HWND)lParam; 
			int sel = ComboBox_GetCurSel(cb);				
			
			if(sel||right) //opening a file or folder
			{
				if(right) SOM_MAIN_changed_new = !sel;

				if(sel==-1) GetWindowTextW(cb,path,MAX_PATH);
				if(sel!=-1) //the edit control is not yet up-to-date
				if(MAX_PATH>=SendMessageW(cb,CB_GETLBTEXTLEN,sel,0))
				SendMessageW(cb,CB_GETLBTEXT,sel,(LPARAM)path);
				
				if(!right&&wcsicmp(L".mo",PathFindExtensionW(path)))
				{
					SOM_MAIN_tree.left.clear();

					if(!*path)
					{
						if(sel==-1) 
						ComboBox_SetCurSel(cb,ComboBox_GetCount(cb)-1);
					}
					else if(SOM_MAIN_langdir(path,tv))
					{
						sel = SOM_MAIN_use(path,cb);
					}
				}			
				else if(*path||!sel) //allow New to be blank
				{
					if(!sel) //New file
					{
						char po[] = 
						"msgid \"\"\n"
						"msgstr \"\"\n"
						"\"Content-Type: text/plain; charset=UTF-8\\n\"\n"
						"\"Content-Transfer-Encoding: 8bit\\n\"\n";
						SOM_MAIN_msgfmt(po,po+sizeof(po)-1);
					}
					else if(SOM_MAIN_mo(path,tv))
					{				
						if(sel) sel = SOM_MAIN_use(path,cb);	
					}
				}
							
				if(!right&&!*path
				||right&&!GetWindowTextLengthW(GetDlgItem(som_tool,1011)))
				SOM_MAIN_mirror();
			}
			else if(!right&&!sel) //list lang folders
			{
				SOM_MAIN_tree.left.clear();

				TVINSERTSTRUCTW tis = 
				{TVI_ROOT,TVI_LAST,{TVIF_TEXT|TVIF_STATE|TVIF_CHILDREN,0,0,~0}};
				tis.item.pszText = path;
				for(size_t i=1;i<SOM_MAIN_all;i++,tis.item.state=0)
				{
					ComboBox_GetLBText(cb,i,path); 					
					tis.item.cChildren = PathFileExistsW(path);
					if(!tis.item.cChildren) tis.item.state|=TVIS_CUT;
					TreeView_InsertItem(tv,&tis);
				}
			}			
			else assert(0);	//paranoia

			TreeView_SelectItem(tv,TreeView_GetChild(tv,TVI_ROOT));

			if(sel!=-1) ComboBox_SetCurSel(cb,sel);

			if(right) SOM_MAIN_changed(-10000);

			//LockWindowUpdate(0);	
			SetWindowRedraw(tv,1);
			return 0;
		}}

		switch(wParam)
		{
		case 1010: //load

			if(IsWindowEnabled(GetDlgItem(hwndDlg,12321))) 
			switch(SOM_MAIN_warning("MAIN1000",MB_YESNOCANCEL))
			{
			case IDYES:	if(SOM_MAIN_save()) break;

				if(IDOK==SOM_MAIN_warning("MAIN201",MB_OKCANCEL|MB_DEFBUTTON2))
				break;

			case IDCANCEL: return 1; 		
			}
			//break;

		case 1009: //add
		{				
			static size_t msgfmt = 0;
			static wchar_t title[40] = L""; //chkstk
			if(!*title&&!GetDlgItemTextW(hwndDlg,1002,title,EX_ARRAYSIZEOF(title)))
			wcscpy_s(title,L"Add a lang directory folder");
			if(!*SOM_MAIN_opendirfilter) //initialize
			{
				SHFILEINFOW si; //install: any folder will do here
				SHGetFileInfoW(SOM::Tool::install(),0,&si,sizeof(si),SHGFI_TYPENAME);		
				if(!*si.szTypeName) wcscpy(si.szTypeName,L"Folder");
				//qqqq... is a tip from the link below
				//http://www.codeproject.com/Articles/20853/Win-Tips-and-Tricks		
				wchar_t *cat = SOM_MAIN_opendirfilter;
				int cat_s,n = swprintf(cat,L"%ls%lcqqqqqqqqqqqqqqq.qqqqqqqqq%lc",si.szTypeName,'\0','\0');				
				cat+=n; cat_s = EX_ARRAYSIZEOF(SOM_MAIN_opendirfilter)-n;	
				int len = GetDlgItemTextW(hwndDlg,1001,cat,cat_s);
				if(!len) wcscpy_s(cat,cat_s,L"Script");
				wchar_t *_n = wcschr(cat,'\n'); if(_n) len = _n-cat; else _n = cat+len;
				const wchar_t mo[] = L" (*.mo)\0*.mo\0"; 
				msgfmt = _n-SOM_MAIN_opendirfilter+EX_ARRAYSIZEOF(mo)-1;
				wmemcpy_s(_n,cat_s-len,mo,EX_ARRAYSIZEOF(mo));
			}
			*path = '\0';
			OPENFILENAMEW open = 
			{
				sizeof(open),som_tool,0,				
				SOM_MAIN_opendirfilter,0,0,0,path,MAX_PATH,0,0,0,title, 
				OFN_ENABLEHOOK|OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_FILEMUSTEXIST,
				0,0,0,0,SOM_MAIN_openhook,0,0,0
			};
			if(wParam==1010) //skip the folder filter
			{
				open.lpstrTitle = 0;				
				open.lpstrFilter+=1+wcslen(open.lpstrFilter);
				open.lpstrFilter+=1+wcslen(open.lpstrFilter);
				open.Flags&=~OFN_FILEMUSTEXIST;

				//NEW: msgfmt programs have lousy Unicode file name support 
				const wchar_t ext[] = L"msgfmt (*.pot;*.po)\0*.pot;*.po\0";
				wmemcpy_s(SOM_MAIN_opendirfilter+msgfmt, //xgettext (*.som)
				EX_ARRAYSIZEOF(SOM_MAIN_opendirfilter)-msgfmt,ext,EX_ARRAYSIZEOF(ext));
			}
			else SOM_MAIN_opendirfilter[msgfmt] = '\0'; //NEW
			
			if(GetOpenFileNameW(&open))
			if(wParam==1010&&open.nFilterIndex>1) //NEW: msgfmt
			{
				SOM_MAIN_reset(SOM_MAIN_right.treeview);
				SetWindowRedraw(SOM_MAIN_right.treeview,0);
				SOM_MAIN_changed_new = true; SOM_MAIN_changed(-10000);
				SendDlgItemMessage(SOM_MAIN_151(),1013,CB_SETCURSEL,0,0);
				HANDLE fm,f = CreateFileW(path,SOM_TOOL_READ); //_not_ testing if non-UTF8
				fm = CreateFileMapping(f,0,PAGE_READONLY,0,0,0);			
				char *view = (char*)MapViewOfFile(fm,FILE_MAP_READ,0,0,0);					
				SOM_MAIN_msgfmt(view,view+GetFileSize(f,0)); 				
				UnmapViewOfFile(view); CloseHandle(fm); CloseHandle(f);				
				SetWindowRedraw(SOM_MAIN_right.treeview,1);
				if(SOM_MAIN_same()) SOM_MAIN_mirror();
			}
			else SOM_MAIN_selendok(wParam==1009?1011:1013,path);
			break;
		}		
		case IDOK: //save as...
		{
			//hack: disabling default button
			if(GetKeyState(VK_RETURN)>>15) break; 
			
			const wchar_t *filters[][2] = 
			{
				{L"Script",L"*.mo"},
				{L"UTF-8 gettext document",L"*.pot;*.po"},
				{L"Run SOM_RUN program sold by From Software",L"*.som"},
			};
			static wchar_t xfilter[200] = L"", xfilter_s = //chkstk
			GetDlgItemTextW(hwndDlg,1001,xfilter,EX_ARRAYSIZEOF(xfilter));
			wchar_t *x = xfilter, *n, *_0;
			std::vector<wchar_t> &f = SOM_MAIN_std::vector<wchar_t>();
			for(size_t i=0;i<EX_ARRAYSIZEOF(filters);i++,x=n+(*n=='\n'))
			{
				n = _0 = wcschr(x,'\n'); if(!n) n = xfilter+xfilter_s;
				size_t x_s = n-x; if(!*x) x_s = wcslen(x=(wchar_t*)filters[i][0]);				
				size_t cat = f.size(); f.resize(cat+x_s+sizeof(" ()")+wcslen(filters[i][1])*2+1);
				if(_0) *_0 = '\0';
				swprintf_s(&f[cat],f.size()-cat,L"%ls (%ls)%lc%ls",x,filters[i][1],'\0',filters[i][1]);
				if(_0) *_0 = '\n';
			}
			f.push_back('\0');			

			*path = '\0'; 
			OPENFILENAMEW save = 
			{
				sizeof(save),som_tool,0,				
				&f[0],0,0,0,path,MAX_PATH,0,0,0,0, 
				OFN_ENABLEHOOK|OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY,
				0,0,0,0,SOM_MAIN_openhook,0,0,0
			};			
			if(GetSaveFileNameW(&save)) switch(save.nFilterIndex)
			{
			case 1: case 2:

				SOM_MAIN_save_as(path,save.nFilterIndex);
				break;

			case 3: //SOM_RUN
			
				SetEnvironmentVariableW(L".SOM",path);
				if(SOM::exe(L"SOM_RUN.exe \""SOMEX_L(A)L"\""))
				{
					//WaitForSingleObject(SOM::exe_process,INFINITE);
					CloseHandle(SOM::exe_process);
					CloseHandle(SOM::exe_thread);
				}
				else //SOM_RUN failure (prints location of TOOL folder)
				{
					SOM_MAIN_error("MAIN087",MB_OK);
				}
				break;
			}			
			break;
		}
		case 12321: //save
		{
			if(!SOM_MAIN_save())
			SOM_MAIN_warning("MAIN201",MB_OK);
			break;
		}
		case IDCANCEL: //via Esc key
		{
			int side = GetKeyState(VK_SHIFT)>>15?0:1;
			//closes this window and the next one behind it
			while(GetAsyncKeyState(VK_ESCAPE)>>15) EX::sleep(100);
			SOM_MAIN_escape(side);
			return 1; //beeping
		}
		case IDCLOSE: goto close;
		}
		break;
	
	case WM_CLOSE: close: //Close or Exit/X

		if(IsWindowEnabled(GetDlgItem(hwndDlg,12321))) 
		switch(SOM_MAIN_warning("MAIN1000",MB_YESNOCANCEL))
		{
		case IDYES:	if(SOM_MAIN_save()) break;

			if(IDOK==SOM_MAIN_warning("MAIN201",MB_OKCANCEL|MB_DEFBUTTON2))
			break; 
			
			MessageBeep(-1); return 1; //2023

		case IDCANCEL: return IDCANCEL; 		
		}
		som_tool = SOM_MAIN_modes[0]; DestroyWindow(hwndDlg);		
		if(uMsg==WM_CLOSE) return SendMessage(som_tool,uMsg,wParam,lParam);
		EnableWindow(som_tool,1); ShowWindow(som_tool,1);
		return 1;

	case WM_NCDESTROY:

		SOM_MAIN_mo(0); //important
		DestroyWindow(SOM_MAIN_modes[2]);
		SOM_MAIN_modes[1] = SOM_MAIN_modes[2] = 0;
		break;
	} 
	return 0;
}     
inline bool SOM_MAIN_152_divider(RECT &div)
{
	if(IsWindowVisible(SOM_MAIN_right.treeview))
	if(IsWindowVisible(SOM_MAIN_tree.left.treeview))
	{
		SOM_MAIN_RTLPOS
		return SetRect(&div,lpos.right,lpos.top,rpos.left,lpos.bottom);
	}
	return false;
}
extern void som_tool_highlight(HWND,int=0);
extern void SOM_MAIN_updowns(HWND ch, RECT &rc);
static INT_PTR CALLBACK SOM_MAIN_152(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{	
	//schdeduled obsolete
	static RECT sep; static int left_of_findbox, x; 
	const int bts[] = {1,1018,1019,1082,1083,1200,1213,1081};		

	switch(uMsg)
	{		
	case WM_INITDIALOG:
	{	
		SOM_MAIN_modes[2] = hwndDlg;

		HICON icon = //128: the ID used by SOM's tools	
		LoadIcon(GetModuleHandle(0),MAKEINTRESOURCE(128));
		SendMessage(hwndDlg,WM_SETICON,ICON_SMALL,(LPARAM)icon);
		SendMessage(hwndDlg,WM_SETICON,ICON_BIG,(LPARAM)icon);

		//todo? maybe frame the treeview itself
		//place in top-right corner (presuming ltr)
		RECT rc; GetWindowRect(SOM_MAIN_151(),&rc);
		POINT tr = {rc.right,rc.top}; 
		GetWindowRect(hwndDlg,&rc);	OffsetRect(&rc,-rc.left,-rc.top);
		SetWindowPos(hwndDlg,0,tr.x-rc.right,tr.y,rc.right,rc.bottom,SWP_NOZORDER);

		HWND left = GetDlgItem(hwndDlg,1022);
		HWND right = GetDlgItem(hwndDlg,1023);
		SOM_MAIN_tree.left.layouts[1] = left?left:right;
		SOM_MAIN_tree.right.layouts[1] = right;
		
		GetClientRect(hwndDlg,&rc); 
		sep.left = SOM_MAIN_rtl->layouts[1].left;
		if(left) if(SOM_MAIN_rtl==&SOM_MAIN_right)
		sep.left-=SOM_MAIN_tree.left.layouts[1].right;
		else sep.left-=SOM_MAIN_right.layouts[1].right;
		sep.top = SOM_MAIN_rtl->layouts[1].top;
		sep.right = rc.right-SOM_MAIN_rtl->layouts[1].right;
		sep.bottom = rc.bottom-SOM_MAIN_rtl->layouts[1].bottom;

		HWND hw = GetDlgItem(hwndDlg,1014); //findbox
		RECT fb; GetWindowRect(hw,&fb); left_of_findbox = fb.left;
		for(int i=0;i<EX_ARRAYSIZEOF(bts);i++) 
		if(GetWindowRect(hw=GetDlgItem(hwndDlg,bts[i]),&rc))
		{
			if(rc.left<left_of_findbox) left_of_findbox = rc.left;
		}
		left_of_findbox = fb.left-left_of_findbox;
				
		const int bts[] = {1,1213}, bts2[] = {0};
		SOM_MAIN_buttonup(hwndDlg,bts,1081,1083);		
		if(DLGC_BUTTON&~SendDlgItemMessage(hwndDlg,1018,WM_GETDLGCODE,0,0))
		SOM_MAIN_buttonup(hwndDlg,bts2,1018,1019);		
		SetWindowSubclass(hwndDlg,SOM_MAIN_commonproc,0,0);		
 
		hw = GetDlgItem(hwndDlg,1081);		
		HWND escb = GetDlgItem(hwndDlg,1200);
		//manually reposition this checkbox?
		if(BS_PUSHLIKE&GetWindowStyle(escb))
		{
			//workaround: ResEdit size restrictions
			tr.y = GetDlgItemInt(hwndDlg,1200,0,0); 
			if(tr.y||0==GetWindowTextLengthW(escb))
			{
				if(tr.y) SetWindowTextW(escb,L""); 
				if(!tr.y) GetWindowRect(escb,&rc);
				if(!tr.y) tr.y = rc.bottom-rc.top;
				SOM_MAIN_updowns(hw,rc=Sompaste->frame(hw,hwndDlg)); 
				rc.top+=rc.bottom-rc.top; rc.bottom = rc.top+tr.y;
				MoveWindow(escb,rc.left,rc.top,rc.right-rc.left,tr.y,0);
			}
		}		
		Button_SetCheck(escb,IsWindowEnabled(hw));
		//replace the treeviews with the real deal
		DestroyWindow(left); DestroyWindow(right); 		
		SOM_MAIN_escape(lParam);					
		if(!IsWindowVisible(hwndDlg))
		SOM_MAIN_readytoserveview(escb); //moved from 151		
		return 0;
	}	
	case WM_SHOWWINDOW:

		if(wParam) //enabled/disable buttons
		{
			RECT cr; GetClientRect(hwndDlg,&cr);
			SOM_MAIN_doublewide(cr.right,cr.bottom);
		}
		break;

	case WM_SIZE: //todo: somehow merge 152/154/172
	{
		if(wParam!=SIZE_RESTORED&&wParam!=SIZE_MAXIMIZED) break;
		
		int w = LOWORD(lParam), h = HIWORD(lParam), z = SWP_NOZORDER;

		HDWP dwp = BeginDeferWindowPos(9);
		
		bool dw = SOM_MAIN_doublewide(w,h); //reparent: flickers
		SOM_MAIN_tree[SOM_MAIN_escaped]->reparent(SOM_MAIN_modes[1+dw]);
		SOM_MAIN_RTLPOS	if(dw) 
		{
			int room = w-lpos.left;
			int cy = h-sep.top-sep.bottom;
			int cx = room-sep.left-sep.right; cx/=2;		
			lpos.right = lpos.left+cx; lpos.bottom = lpos.top+cy;
			DeferWindowPos(dwp,l,0,lpos.left,lpos.top,cx,cy,z);
			rpos.left = lpos.left+cx+sep.left; 
		}
		else rpos.left = lpos.left; 

		int xdelta = rpos.right, ydelta = rpos.bottom;
		rpos.right = w-sep.right; rpos.bottom = h-sep.bottom;
		xdelta = rpos.right-xdelta; ydelta = rpos.bottom-ydelta;
		DeferWindowPos(dwp,r,0,rpos.left,rpos.top,rpos.right-rpos.left,rpos.bottom-rpos.top,z);		
		
		HWND hw, fw = GetDlgItem(hwndDlg,1014); //findbox
		RECT rc, fb; GetWindowRect(fw,&fb); MapWindowRect(0,hwndDlg,&fb);
		fb.right+=xdelta;
		int xdelta2 = fb.left;		
		int room = fb.right-lpos.left-left_of_findbox;
		fb.left = fb.right-max(75,min(300,room));
		xdelta2 = fb.left-xdelta2; 
		fb.right-=xdelta; fb.left-=xdelta;
		  
		for(int i=0;i<EX_ARRAYSIZEOF(bts);i++) if(hw=GetDlgItem(hwndDlg,bts[i]))
		{	
			GetWindowRect(hw,&rc); MapWindowRect(0,hwndDlg,&rc);
			OffsetRect(&rc,rc.left>fb.right?xdelta:xdelta2,ydelta);
			DeferWindowPos(dwp,hw,0,rc.left,rc.top,0,0,z|SWP_NOSIZE);
		}

		OffsetRect(&fb,xdelta,ydelta);
		DeferWindowPos(dwp,fw,0,fb.left,fb.top,fb.right-fb.left,fb.bottom-fb.top,z);

		if(!EndDeferWindowPos(dwp)) assert(0);

		if(xdelta) //update the wrapping/widths of the textareas
		{
			if(IsWindowVisible(r)) SetTimer(r,'<->',250,SOM_MAIN_sizeup);
			if(IsWindowVisible(l)) SetTimer(l,'<->',250,SOM_MAIN_sizeup);

			RECT div = {lpos.right,lpos.top,rpos.left,lpos.bottom};
			InvalidateRect(hwndDlg,&div,1);
		}	 
		break;
	}
	case WM_SETCURSOR: case WM_LBUTTONDOWN: //todo: somehow merge 152/154/172
	{
		POINT pt; GetCursorPos(&pt);
		MapWindowPoints(0,hwndDlg,&pt,1);
		RECT &lpos = SOM_MAIN_tree.left.layouts[1];
		RECT &rpos = SOM_MAIN_tree.right.layouts[1];
		if(pt.x>lpos.right&&pt.x<rpos.left
		  &&pt.y>lpos.top&&pt.y<lpos.bottom) switch(uMsg)
		{	  
		case WM_SETCURSOR:
		
			SetCursor(LoadCursor(0,IDC_SIZEWE));
			return 1;

		case WM_LBUTTONDOWN: 
			
			SetCapture(hwndDlg); x = pt.x; 
			break;
		}
		break;
	}
	case WM_LBUTTONUP: case WM_MOUSEMOVE: //todo: somehow merge 152/154/172
	
		if(GetCapture()==hwndDlg) switch(uMsg)
		{
		case WM_LBUTTONUP: ReleaseCapture(); break;		
		case WM_MOUSEMOVE:

			POINT pt; GetCursorPos(&pt);
			MapWindowPoints(0,hwndDlg,&pt,1);
			int delta = pt.x-x;	x = pt.x; if(!delta) break; 
			SOM_MAIN_RTLPOS
			if(delta<0)
			{
				lpos.right = max(lpos.right+delta,
				lpos.left+SOM_MAIN_tree.left.layouts[1]->nc);
				rpos.left = lpos.right+sep.left;
			}
			else
			{
				rpos.left = min(rpos.left+delta,
				rpos.right-SOM_MAIN_tree.right.layouts[1]->nc);
				lpos.right = rpos.left-sep.left;
			}
			RECT div = {lpos.right,lpos.top,rpos.left,lpos.bottom};
			InvalidateRect(hwndDlg,&div,1);
			HWND fp,f = GetFocus(); 
			if((fp=GetParent(f))==l||r==fp) f = fp;
			for(int i=0;i<2;i++)
			{
				HWND &lr = i?l:r; RECT &lrc = i?lpos:rpos;				
				MoveWindow(lr,lrc.left,lrc.top,lrc.right-lrc.left,lrc.bottom-lrc.top,1);
				SetTimer(lr,'<->',250,SOM_MAIN_sizeup);
				EnableWindow(lr,lrc.right-lrc.left>SOM_MAIN_tree[lr].layouts[1]->nc);
			}
			if(!IsWindowEnabled(f)) 
			SetFocus(SOM_MAIN_tree[f]->treeview);			
			break;
		}	
		break;
	 		
	case WM_SYSCOMMAND:

		if(SC_MINIMIZE==wParam&&som_tool_taskbar)
		{
			//isn't replacing foreground???
			//CloseWindow(som_tool_taskbar);
			SendMessage(som_tool_taskbar,uMsg,wParam,lParam);
			return 1;
		}
		break;	  
	
	case WM_COMMAND:

		switch(LOWORD(wParam))
		{
		case 1200: //Esc key mode 

			som_tool_highlight(hwndDlg,IDOK); //hide
			windowsex_enable<1081>(hwndDlg,Button_GetCheck((HWND)lParam));
			break;

		case IDOK: //minimize button
			
			if(~GetKeyState(VK_RETURN)>>15)
			PostMessage(hwndDlg,WM_SYSCOMMAND,SC_MINIMIZE,0);
			break;

		case IDCANCEL: //Esc key
		{
			//closes this window and the next one behind it
			while(GetAsyncKeyState(VK_ESCAPE)>>15) EX::sleep(100);
			HWND escb = GetDlgItem(hwndDlg,1200);
			if(escb&&!Button_GetCheck(escb)) 
			PostMessage(hwndDlg,WM_SYSCOMMAND,SC_MINIMIZE,0);
			else SOM_MAIN_escape(); 
			return 1; //beeping
		}}
		break;

	case WM_CLOSE: 

		SendMessage(SOM_MAIN_151(),uMsg,wParam,lParam);
		return 1;
	} 
	return 0;
}
static ITextDocument *SOM_MAIN_tom(HWND re)
{
	IRichEditOle *ole = 0;
	SendMessage(re,EM_GETOLEINTERFACE,0,(LPARAM)&ole);
	ITextDocument *tom = 0;	const IID IID_ITextDocument = 
	{0x8CC497C0,0xA1DF,0x11ce,0x80,0x98,0x00,0xAA,0x00,0x47,0xBE,0x5D};		
	ole->QueryInterface(IID_ITextDocument,(void**)&tom);
	ole->Release();	return tom;
}
static ITextRange *SOM_MAIN_range(HWND re, long lb=0, long ub=LONG_MAX)
{
	ITextRange *out = 0; ITextDocument *tom = SOM_MAIN_tom(re);
	tom->Range(lb,ub,&out); tom->Release();	return out;
}
static ITextSelection *SOM_MAIN_selection(HWND re)
{
	ITextSelection *out = 0; ITextDocument *tom = SOM_MAIN_tom(re);
	tom->GetSelection(&out); tom->Release(); return out;
}
static void SOM_MAIN_format(HWND em, int start, const wchar_t *face=0, int indent=-1)
{
	DWORD sel = Edit_GetSel(em)+1; Edit_SetSel(em,start,-1);

	CHARFORMAT2W cf; cf.cbSize = sizeof(cf);  					
	cf.dwEffects = 0;
	cf.dwMask = CFM_BACKCOLOR|CFM_LINK|CFM_HIDDEN|CFM_PROTECTED; 
	//todo: extension to change the highlight
	cf.crBackColor = GetSysColor(COLOR_WINDOW);
	if(ES_READONLY&GetWindowStyle(em))
	cf.crBackColor = GetSysColor(COLOR_3DFACE);

	int compile[SOM_MAIN_poor]; if(face) 
	{
		//Mincho: for 2014 New Years Eve release only
		//todo: find out if this requires SCF_ASSOCIATEFONT or not
		//(not setting CFM_CHARSET seems to prevent pasting other fonts)
		cf.dwMask|=CFM_FACE; wcscpy_s(cf.szFaceName,face);
	}
	extern char som_main_richinit_pt[32];	
	cf.dwMask|=CFM_SIZE|CFM_COLOR|CFM_ITALIC|CFM_BOLD|CFM_STRIKEOUT|CFM_UNDERLINE|CFM_SUBSCRIPT|CFM_SMALLCAPS|CFM_ALLCAPS;	
	cf.yHeight = 20*strtod(som_main_richinit_pt,0);	cf.dwEffects|=CFE_AUTOCOLOR;

	SendMessage(em,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM)&cf);

	PARAFORMAT2 pf; pf.cbSize = sizeof(pf); 
	pf.dwMask = PFM_STARTINDENT|PFM_OFFSET|PFM_SPACEAFTER|PFM_SPACEBEFORE;
	pf.wEffects = 0; 
	if(indent<0) indent = 160;
	pf.dxStartIndent = indent; pf.dxOffset = 0; 
	pf.dySpaceBefore = pf.dySpaceAfter = 20;
	SendMessage(em,EM_SETPARAFORMAT,0,(LPARAM)&pf);					

	Edit_SetSel(em,LOWORD(sel),HIWORD(sel));
}
static void SOM_MAIN_copyrichtext(HWND copy, HWND text, int bc=0)
{
	ITextRange *src = SOM_MAIN_range(text);			
	ITextRange *dst = SOM_MAIN_range(copy);
	dst->SetFormattedText(src);	if(bc) //important done first
	{
		ITextFont *cf; dst->GetFont(&cf); cf->SetBackColor(GetSysColor(bc)); 
		cf->Release();
	}
	if(GetWindowTextLengthW(copy)>GetWindowTextLengthW(text))
	{
		long wtf; //really? remove tacked on line that wasn't asked for			
		dst->GetEnd(&wtf); dst->SetStart(wtf-1); dst->Delete(1,0,&wtf); 			
	}	
	src->Release(); dst->Release(); 
}
static DWORD SOM_MAIN_copyrichtextext(HWND copy, HWND text)
{
	SOM_MAIN_copyrichtext(copy,text);
	DWORD sel = SOM_MAIN_select_menu(text);	
	DWORD del = 1+HIWORD(Edit_GetSel(text));
	SOM_MAIN_and_unselect(text,sel); 
	sel = Edit_GetSel(text); ((WORD*)&sel)[0]-=del; ((WORD*)&sel)[1]-=del;
	if(del>1) Edit_SetSel(copy,0,del);
	if(del>1) Edit_ReplaceSel(copy,L""); return sel;
}
static void SOM_MAIN_editrichtextext(HWND undo, HWND text)
{
	DWORD sel = Edit_GetSel(text); if(!undo) return;		
	SOM_MAIN_textarea ta(undo); WORD menu = ta.menu();
	if(Edit_GetModify(text)) //keeping 172's selection
	{
		int compile[SOM_MAIN_poor];
		const wchar_t *p = SOM_MAIN_WindowText(text); 
		SOM_MAIN_select_text(undo);	   										
		SETTEXTEX st = {ST_KEEPUNDO|ST_SELECTION,1200};
		//hack: not the best way to do this
		ITextDocument *tom = SOM_MAIN_tom(undo); 							
		SendMessage(undo,EM_SETTEXTEX,(WPARAM)&st,(LPARAM)p); 
		tom->Undo(tomSuspend,0);					
		SOM_MAIN_format(undo,menu+1, 
		SOM_MAIN_tree[ta.param].serifed()?som_932w_MS_Mincho:0);	
		tom->Undo(tomResume,0);
		tom->Release();					
	}
	Edit_SetSel(undo,LOWORD(sel)+menu+1,HIWORD(sel)+menu+1);
}
static void SOM_MAIN_justlines(std::string &urtf, const char *p)
{
	if(p) while(*p) switch(*p)
	{
	default: urtf+=*p++; break;
	case '\r': p++; break; case '\n': p++; urtf+="\\line "; 
	}		
}
static void SOM_MAIN_plaintextwithlines(std::string &urtf, const char *p)
{
	if(p) while(*p) switch(*p)
	{
	case '\r': p++; break;
	case '\n': p++; urtf+="\\line "; break;
	case '\\':case'{':case'}':urtf+='\\'; default:urtf+=*p++;
	}		
}
template<class vectorlike>
static void SOM_MAIN_plaintext(vectorlike &urtf, const char *p, size_t p_s=-1)
{
	if(p) while(*p&&p_s--) switch(*p)
	{
	case '\r': if(*++p=='\n') break; else urtf.push_back('\n'); break;
	case '\\':case'{':case'}':urtf.push_back('\\'); default:urtf.push_back(*p++);
	}		
}
static std::string &SOM_MAIN_libintl(std::string &urtf, const char *msg_id)
{
	const char *p, *pp, *_4 = strchr(msg_id,'\4'); 
	p = pp = _4?_4+1:msg_id; //skip screen context 
	for(int s=-1;*p;p++) if(*p=='%') if(p[1]!='%') switch(++s)
	{
	case 0: urtf.append(pp,p-pp+1); pp = p+1; continue;			
	case 1: urtf+='0'+s; urtf+='$'; 
	default: urtf.append(pp,p-pp+1); pp = p+1; urtf+='1'+s; urtf+='$'; 
	}
	else p++; /*%%*/ return urtf.append(pp,p-pp); //todo? SOM_MAIN_plaintext for \{}
}
static const char *SOM_MAIN_libintl(const char *id)
{
	return !strchr(id,'%')?id:SOM_MAIN_libintl(SOM_MAIN_std::string(),id).c_str();	
}
static bool SOM_MAIN_fillrichtext(HWND riched, const char *with, const char *plain=0)
{	
	int compile[SOM_MAIN_poor]; 
	std::string &urtf = SOM_MAIN_std::string();
	urtf = "{\\urtf1 "; if(with) //todo: inject tables here
	if(with==plain) SOM_MAIN_plaintextwithlines(urtf,with); else SOM_MAIN_justlines(urtf,with); 
	urtf+="}"; SETTEXTEX st = {0,CP_UTF8};
	SendMessage(riched,EM_SETTEXTEX,(WPARAM)&st,(LPARAM)urtf.c_str()); return with;
}
static DWORD CALLBACK SOM_MAIN_EditStreamCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
	int plain = dwCookie&1; dwCookie-=plain;
	std::vector<char> &rtf = *(std::vector<char>*)dwCookie; 
	if(!plain){ size_t _ = rtf.size(); rtf.resize(_+cb); memcpy(&rtf[_],(char*)pbBuff,cb); }
	else SOM_MAIN_plaintext(rtf,(char*)pbBuff,cb); *pcb = cb; return 0;
}
static size_t SOM_MAIN_peelrichtext(std::vector<char> &urtf, HWND riched, int sf=SF_RTF)
{	
	size_t sizein = urtf.size();
	EDITSTREAM es = {(DWORD_PTR)&urtf,0,&SOM_MAIN_EditStreamCallback};
	if(~sf&SF_RTF&&SOM_MAIN_poor) es.dwCookie+=1;
	SendMessage(riched,EM_STREAMOUT,MAKEWPARAM(sf|SF_USECODEPAGE,CP_UTF8),(LPARAM)&es);
	if(~sf&SF_RTF) return 0;

	const char closure[] = "}\r\n";
	if(urtf.size()-sizein<=sizeof("{\\urtf1}\r\n")
	 ||strcmp(&urtf.back()-sizeof(closure)+1,closure)) return(urtf.resize(sizein),0);
	
	char *head = &urtf[sizein];
	size_t headlen = sizeof("{\\urtf1")-1;
	char *trim = strstr(head,"\\highlight"); if(!trim) return(urtf.resize(sizein),0); 	
	
	strtol(trim+=sizeof("\\highlight")-1,&trim,10);
	if(!strncmp(trim,"\\lang",5)&&isdigit(trim[5]))
	strtol(trim+5,&trim,10); if(*trim==' ') trim++;		

	headlen = trim-head; 	
	size_t trimlen = urtf.size()-sizeof(closure); 
	
	urtf[trimlen] = '\0'; 	
	trim = strrchr(head,'\\');
	if(trim&&!strcmp(trim,"\\par\r\n"))
	{
		trimlen = trim-head;

		*trim = '\0'; 
		trim = strrchr(head,'\\');
		if(trim&&!strcmp(trim,"\\highlight0")) 
		{
			trimlen = trim-head;
		}
	}
	urtf.resize(trimlen); return headlen;
}
inline char *SOM_MAIN_peelpoortext(std::vector<char> &urtf, HWND riched, int sff=0)
{
	size_t sizein = urtf.size();
	size_t out = SOM_MAIN_peelrichtext(urtf,riched,(SOM_MAIN_poor?SF_TEXT:SF_RTF)|sff); 	
	urtf.push_back('\0'); assert(!out);
	return &urtf[sizein+out];
}
static char *SOM_MAIN_peelpoortextext(std::vector<char> &urtf, HWND riched)
{
	DWORD sel = SOM_MAIN_select_text(riched);			
	char *out = SOM_MAIN_peelpoortext(urtf,riched,SFF_SELECTION);
	SOM_MAIN_and_unselect(riched,sel); return out;
}
static WORD SOM_MAIN_l = 120, SOM_MAIN_s = 120;
static void SOM_MAIN_rainbowtab(HWND text, bool select=false)
{
	static bool one_off = false; if(!one_off++) //???
	{
		BYTE ls[2]; DWORD two = 2;	  
		if(!SHGetValueW(HKEY_CURRENT_USER,
		L"Software\\FROMSOFTWARE\\SOM_MAIN\\STORY\\<exml hl>",L"UserSet",0,ls,&two)
		||two>2){ SOM_MAIN_l = ls[0]; SOM_MAIN_s = ls[1]; }
	}

	SOM_MAIN_textarea ta(text);	if(!ta) return;
	COLORREF bg = GetSysColor(select?COLOR_HIGHLIGHT:COLOR_3DFACE);
	if(!select||ta.param)
	{
		LPARAM lp = ta.param; 
		if(!lp) lp = SOM_MAIN_param(ta.treeview(),ta.item);
		const char *hl = EXML::Quote(SOM_MAIN_tree[lp].exml_attribute(EXML::Attribs::hl));
		if(hl) bg = ColorHLSToRGB(atoi(hl),SOM_MAIN_l,SOM_MAIN_s);
		if(hl&&bg==GetSysColor(COLOR_HIGHLIGHT)) bg+=1; //paranoia
	}
	int mf = Edit_GetModify(text); //POINT OF NO RETURN
	{
		if(bg==SendMessage(text,EM_SETBKGNDCOLOR,0,bg)) goto mf;

		if(ta.param) goto mf; //todo? contrast menu text somehow

		CHARFORMAT cf = {sizeof(cf)};
		cf.dwEffects = 0; cf.dwMask = CFM_COLOR; 
		cf.crTextColor = GetSysColor(select?COLOR_HIGHLIGHTTEXT:COLOR_WINDOWTEXT);
		SendMessage(text,EM_SETCHARFORMAT,SCF_ALL,(LPARAM)&cf);	
	}
mf: Edit_SetModify(text,mf);
}
inline void	SOM_MAIN_selectab(HWND text)
{
	if(!GetWindowID(text)) 
	if(GetKeyState(VK_LBUTTON)>>15)
	{
		//interferes (people know what they are clicking)
		//POINT pt; GetCursorPos(&pt);
		//SOM_MAIN_rainbowtab(text,!DragDetect(text,pt));
	}
	else
	{
		DWORD sel = Edit_GetSel(text); 
		SOM_MAIN_rainbowtab(text,LOWORD(sel)==HIWORD(sel));
	}		
}

static int SOM_MAIN_delete(HWND,HTREEITEM,int=0);
static int SOM_MAIN_expand(HWND,HTREEITEM,int='+');
static int SOM_MAIN_insert(HWND,HTREEITEM,HTREEITEM);
static int SOM_MAIN_create(HWND,HTREEITEM,HTREEITEM);
#define SOM_MAIN_edit() SOM_MAIN_expand(0,0,'edit')
#define SOM_MAIN_retract() SOM_MAIN_delete(0,0,'<-')
#define SOM_MAIN_open(x,y) SOM_MAIN_expand(x,y,'open')
#define SOM_MAIN_collapse(x,y) SOM_MAIN_expand(x,y,'-')
static VOID CALLBACK SOM_MAIN_activate(HWND tv, UINT one, UINT_PTR id, DWORD)
{
	KillTimer(tv,id); HTREEITEM i = TreeView_GetSelection(tv);

	SOM_MAIN_tree = tv; //doesn't always focus on double-click?

	SOM_MAIN_textarea ta(tv,i); if(ta) //NEW: assuming inserting 
	{	
		const int sep = 3; //todo: extension?
		char url[32+8], *e; LPARAM l = 0, r = 0;
		long cp, pt = LOWORD(Edit_GetSel(ta)), menu = ta.menu(); 
		if(pt<menu) //-sep doesn't seem to reach far enough
		{
			int n = one==1?1:2;
			ITextRange *p = SOM_MAIN_range(ta,pt-sep,pt-sep); 
			if(p) for(int side=1;side<=n;side++,p->SetRange(pt+sep,pt+sep))
			{
				url[0] = '\0'; e = 0;
				LPARAM &lr = side==1?l:r;			
				if(!p->StartOf(tomLink,0,&cp))
				for(int i=0;i<sizeof(url)&&!p->GetChar(&cp);url[i++]=cp) p->Move(1,1,0); 			
				if(!strncmp(url,"HYPERLINK \"",11)&&url[19+8]=='"') 
				{
					lr = //strtoul(url+11,&e,16);
					SOM_MAIN_tree[tv].follow_link(_strtoui64(url+11,&e,16));
					if(!lr) //link is no longer viable
					SOM_MAIN_report("MAIN214",MB_OK);
				}
				//+1: black magic if last URL is bracketed
				//+2? black magic if last URL is bracketed
				if(e!=url+19+8||p->GetStart(&cp)||cp>menu+2) lr = 0; //safety first
			}
			if(p) p->Release();
		}
		else l = SOM_MAIN_param(tv,ta.item); //must be last

		if(one==1) r = l;
		SOM_MAIN_insert(tv,l?SOM_MAIN_tree[l][tv]:0,r?SOM_MAIN_tree[r][tv]:0); 
	}
	else SOM_MAIN_open(tv,i); 
}
static HTREEITEM SOM_MAIN_collapsed(HWND tv, HTREEITEM parent)
{		
	assert(TreeView_GetChild(tv,parent));
	if(TVIS_EXPANDED&~TreeView_GetItemState(tv,parent,~0)) return parent;		
	parent = TreeView_GetParent(tv,parent); if(!parent) return 0;	
	return SOM_MAIN_collapsed(tv,parent);
}
template<typename RT>
static HWND SOM_MAIN_(RT rt, HWND parent, DLGPROC dlgproc, LPARAM param=0)
{
	if(rt==(RT)153||rt==(RT)154) 
	{
		INITCOMMONCONTROLSEX icex =
		{sizeof(icex),ICC_DATE_CLASSES|ICC_INTERNET_CLASSES};
		if(!InitCommonControlsEx(&icex)) assert(0);
	}	
	void *locked = LockResource(LoadResource(0,FindResourceW(0,(LPCWSTR)rt,(LPCWSTR)RT_DIALOG))); 	
	if(!locked) return(MessageBeep(-1),0); 
	//A: calling som_tool_CreateDialogIndirectParamA
	return CreateDialogIndirectParamA(GetModuleHandle(0),(DLGTEMPLATE*)locked,parent,dlgproc,param);
}
static void SOM_MAIN_ellipsize(wchar_t *p, size_t p_s)
{
	//const wchar_t ellipsis[] = L"[...]";
	//const size_t ellipsis_s = sizeof(ellipsis)-1;
	wchar_t *d = p+p_s-1;
	wchar_t *n = wcschr(p,'\n'); if(!n||n>=d) return; 
	do{ *n = '/'; }while(n=wcschr(p,'\n')); return; /*NEW
	while(d-n<ellipsis_s&&n!=p) n--; if(d-n>=ellipsis_s) wcscpy(n,ellipsis);
	else wmemcpy(n,ellipsis,d-n); *d = '\0';*/
}
static int SOM_MAIN_onedge = 0;
extern HWND som_tool_richcode(HWND);
extern HWND som_tool_richinit(HWND,int); 
extern const wchar_t *som_tool_richedit_class();
static DLGTEMPLATE *SOM_MAIN_dlgtemplate(int id);
extern int som_tool_drawitem(DRAWITEMSTRUCT*,int,int=-1);
static INT_PTR CALLBACK SOM_MAIN_150(HWND,UINT,WPARAM,LPARAM);
static INT_PTR CALLBACK SOM_MAIN_153b(HWND,UINT,WPARAM,LPARAM);
static HWND SOM_MAIN_message_text(LPARAM,HWND,HTREEITEM,HWND=0);
static LRESULT CALLBACK SOM_MAIN_commonproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scID, DWORD_PTR)
{
	switch(uMsg)
	{
	case WM_ACTIVATE:

		uMsg = uMsg; //breakpoint

		break;

	case WM_ACTIVATEAPP: 
		
		if(wParam) SOM_MAIN_readytoserveview(); 
		break;

	case WM_SHOWWINDOW: //continued...

		if(lParam==SW_PARENTOPENING) SOM_MAIN_readytoserveview(); 
		break;

	case WM_SIZE: case WM_MOVE: 
		
		if(SOM_MAIN_onedge&&IsWindowVisible(hWnd)) SOM_MAIN_onedge = 0;
		break;

	case WM_DRAWITEM: //REMOVE ME?
	
		return som_tool_drawitem((DRAWITEMSTRUCT*)lParam,156);

	case WM_NOTIFY: 
	{
		//(for TVN messages)
		//hWnd will be SOM_MAIN_modes[1] both ways
		//https://support2.microsoft.com/kb/104069
		//should probably just set hWnd = som_tool
		//assert(hWnd==som_tool);
		
		NMTVDISPINFOW *p = (NMTVDISPINFOW*)lParam;		

		HWND &tv = p->hdr.hwndFrom;

		switch(p->hdr.code)
		{
		case TVN_GETDISPINFOW:
		{
			if(p->item.mask&TVIF_TEXT)
			if(p->item.lParam>65535)
			{
				//shouldn't be necessary but seems to be so
				if(SOM_MAIN_textarea(tv,p->item.hItem)) break;

				const char *utf8 = 0; 

				SOM_MAIN_tree::string &msg = SOM_MAIN_tree[p->item.lParam];
				
				if(p->item.state&TVIS_CUT)
				if(msg[tv]!=p->item.hItem)
				if(utf8=msg.exml_attribute(EXML::Attribs::lh))
				{
					EXML::Quote qt(utf8); 		
					const char *q,*d = qt;
					HTREEITEM gp = p->item.hItem; do
					{for(q=d;q<qt.delim&&iswspace(*q);d=++q); 			
					for(gp=TreeView_GetParent(tv,gp);d<qt.delim&&*d!='\n';d++); 
					}while(SOM_MAIN_param(tv,gp)==p->item.lParam);					
					//permit half-width space indent/alignment
					while(q>utf8&&q<qt.delim&&q[-1]==' ') q--; 
					int zt = q>=qt.delim?0:
					SOM_MAIN_AttribToWideChar(q,d-q,p->item.pszText,p->item.cchTextMax-1);
					p->item.pszText[zt] = '\0';
					if(zt) return 0;
				}
				if(utf8=msg.exml_attribute(EXML::Attribs::l))
				{
					EXML::Quote qt(utf8); 
					int zt = SOM_MAIN_AttribToWideChar(qt.value,qt.length(),p->item.pszText,p->item.cchTextMax-1);
					p->item.pszText[zt] = '\0';
					return 0;
				}
				if(!TreeView_GetParent(tv,p->item.hItem))
				{
					SOM_MAIN_tree::groups::iterator it;
					it = SOM_MAIN_tree.groups.find(msg.id);
					if(it==SOM_MAIN_tree.groups.end())
					{			 
						if(msg.nocontext()) //for completeness sake
						{
							p->item.pszText = (wchar_t*)som_932w_NoContext[*msg.id];
							return 0;
						}
						int zt = MultiByteToWideChar(65001,0,msg.id,msg.id_s-1,p->item.pszText,p->item.cchTextMax);
						p->item.pszText[zt] = '\0';
						return 0;
					}
					else utf8 = it->c_str()+msg.ctxt_s;				
				}
				else utf8 = msg.id+msg.ctxt_s;

				int zt = MultiByteToWideChar(65001,0,utf8,-1,p->item.pszText,p->item.cchTextMax);
				p->item.pszText[zt] = '\0';
				SOM_MAIN_ellipsize(p->item.pszText,p->item.cchTextMax);
			}
			break;			
		}
		case TVN_GETINFOTIPW: 
		{					
			NMTVGETINFOTIPW *p = (NMTVGETINFOTIPW*)lParam;		

			if(p->lParam)
			if(p->lParam<65535) 
			{
				wchar_t *q = p->pszText, *d = q+p->cchTextMax;

				LCTYPE types[] = 
				{
					LOCALE_SLANGUAGE, //LOCALE_SCOUNTRY,
					LOCALE_SENGLANGUAGE, //LOCALE_SENGCOUNTRY,
					LOCALE_SNATIVELANGNAME, //LOCALE_SNATIVECTRYNAME,
				};
				for(int i=0,got;i<EX_ARRAYSIZEOF(types);i++)				
				if(got=GetLocaleInfoW(p->lParam,types[i],q,d-q))
				{
					q+=got; q[-1] = '\n'; //i&1?'\n':'_';
				}
				if(q>p->pszText) q[-1] = '\0';
			}
			else if(!SOM_MAIN_textarea(tv,p->hItem))
			{
				SOM_MAIN_tree::string &msg = SOM_MAIN_tree[p->lParam];

				if(!TreeView_GetParent(tv,p->hItem)) switch((int)*msg.id)
				{
				case 1: case 2: case 3: 
					
					break; //todo? explanation of metadata/messages/japanese

				default: 
					
					int zt = MultiByteToWideChar
					(65001,0,msg.id,msg.ctxt_s-1,p->pszText,p->cchTextMax-1);	
					p->pszText[zt] = '\0';
				}
				else EX::need_unicode_equivalent(65001,msg.id+msg.ctxt_s,p->pszText,p->cchTextMax);
			}
			if(*p->pszText)
			//needed to show in mode 2 (tooltip is WS_POPUP style)
			SetWindowPos(TreeView_GetToolTips(tv),HWND_TOP,0,0,0,0,
			SWP_NOMOVE|SWP_NOSIZE|SWP_NOOWNERZORDER|SWP_NOACTIVATE);			
			break;
		}  	
		case NM_CUSTOMDRAW:
		{
			NMCUSTOMDRAW *p = (NMCUSTOMDRAW*)lParam;

			switch(p->dwDrawStage) //assuming treeview
			{
			case CDDS_PREPAINT: //want per item behavior

				return CDRF_NOTIFYITEMDRAW;

			case CDDS_ITEMPREPAINT: //drawing grey if cut
				
				HTREEITEM item = (HTREEITEM)p->dwItemSpec;
				int st = TreeView_GetItemState(tv,item,~0);
				NMTVCUSTOMDRAW *q = (NMTVCUSTOMDRAW*)lParam;
				if(TVIS_CUT&st) //if(TVIS_CUT&p->uItemState)
				{	
					if(CDIS_SELECTED&~p->uItemState) //needed
					q->clrText = GetSysColor(COLOR_GRAYTEXT);
				}
				break;
			}
			break;
		}	
		case NM_RETURN:
			
			assert(0); //unsent: using IDOK for now
			return 1;			

		case TVN_KEYDOWN:
		{
			if(GetKeyState(VK_CONTROL)>>15) 
			switch(((NMTVKEYDOWN*)lParam)->wVKey)   
			{	
			case 'X': case 'C': 

				SOM_MAIN_tree[tv].copy(); break;

			case 'V': case 'B': //B: paste below

				SOM_MAIN_tree[tv].paste(); break;

			case VK_DELETE: goto Ctrl_Delete;
			}
			else switch(((NMTVKEYDOWN*)lParam)->wVKey) 			
			{			
			case VK_SPACE: 
							
				SOM_MAIN_open(tv,TreeView_GetSelection(tv));
				return 1; //incremental search?

			case VK_INSERT: 
			{
				HTREEITEM r = TreeView_GetSelection(tv);			
				SOM_MAIN_insert(tv,TreeView_GetParent(tv,r),r);
				break;
			}
			case VK_DELETE: Ctrl_Delete:

				SOM_MAIN_delete(tv,TreeView_GetSelection(tv),'del');
				break;
			}			
			break;
		}
		case NM_DBLCLK: //NM_CLICK: see som.tool.cpp
		{
			//cascade on +/- happens in SOM_MAIN_enrichproc
			SetTimer(tv,'open',0,SOM_MAIN_activate);
			return 1; //prevent expansion
		}	
		case NM_SETFOCUS:
		{
			if(p->hdr.idFrom==1022) //left treeview
			{
				//disable delete/edit buttons
				windowsex_enable(hWnd,1210,1211,0);
				SOM_MAIN_tree = p->hdr.hwndFrom; 
			}
			else if(p->hdr.idFrom==1023) //right treeview
			{
				SOM_MAIN_tree = p->hdr.hwndFrom; 
				//MSDN docs say this will work but it doesn't
				//else TreeView_SelectItem(tv,TreeView_GetSelection(tv));
				goto selchanged; 			
			}
			break;
		}
		case TVN_SELCHANGEDW: selchanged: 
		{
			HTREEITEM sel = TreeView_GetSelection(tv);
			if(p->hdr.code==TVN_SELCHANGED)
			assert(sel==((NMTREEVIEWW*)p)->itemNew.hItem);				
			SOM_MAIN_textarea ta(tv,sel);	

			int del = 0, open = 0, insert = 0;			
			//note: could enable for the left view
			//in mirror mode but it would be confusing
			if(tv==SOM_MAIN_right.treeview&&sel)
			{				
				open = 1; if(!ta)
				{
					if(!SOM_MAIN_readonly(tv)&&TreeView_GetParent(tv,sel)) 
					{
						SOM_MAIN_tree::string &msg = 
						SOM_MAIN_tree[SOM_MAIN_param(tv,sel)];
						if(msg.unoriginal()) del = msg.bilingual(); 
						else insert = del = 1;
					}
				}
				else if(ta.param!=SOM_MAIN_right.metadata)
				{
					if(!ta.param) //2019: unexpanded text menu?
					{
						insert = !SOM_MAIN_tree[SOM_MAIN_param(tv,ta.item)].unoriginal();
					}
					else insert = !SOM_MAIN_tree[ta.param].unoriginal();					
				}
			}

			//hWnd is SOM_MAIN_151()			
			windowsex_enable<1210>(hWnd,del);
			windowsex_enable<1211>(hWnd,open);
			windowsex_enable<1084>(hWnd,insert);
				
			//entering richedit crossbar?
			if(p->hdr.code==TVN_SELCHANGEDW)
			{
				if(ta) //NEW: smooth paging
				{
					//TODO: ensure items are seen

					int vks = 
					GetKeyState(VK_HOME)|GetKeyState(VK_END)
					|GetKeyState(VK_NEXT)|GetKeyState(VK_PRIOR);				
					if(vks&0x8000) 
					{
						//these do different things
						TreeView_SelectItem(tv,ta.item);
						TreeView_SelectSetFirstVisible(tv,ta.item);
						break;
					}
				}						
				HWND f = ta?ta:tv; 
				if(GetFocus()!=f) SetFocus(f);
			}
			break;
		}				
		case TVN_SELCHANGINGW: //hack: anti-fighting magic
		{
			//REMOVE ME??
			NMTREEVIEW *p = (NMTREEVIEW*)lParam;
			SOM_MAIN_textarea ta(tv,p->itemNew.hItem); if(ta) 
			{
				HTREEITEM parent = SOM_MAIN_collapsed(tv,ta.item);
				if(parent) return TreeView_SelectItem(tv,parent);
			}
			break;
		}
		case TVN_ITEMEXPANDEDW:
		case TVN_ITEMEXPANDINGW: 
		{	
			NMTREEVIEW  *p = (NMTREEVIEW*)lParam;

			assert(p->action!=TVE_TOGGLE);

			if(p->action==TVE_COLLAPSE)
			if(p->hdr.code==TVN_ITEMEXPANDEDW)
			if(SOM_MAIN_collapse(tv,p->itemNew.hItem))
			return 1;
			
			if(p->action==TVE_EXPAND)
			if(p->hdr.code==TVN_ITEMEXPANDINGW)
			if(SOM_MAIN_expand(tv,p->itemNew.hItem)) 
			return 1; 
			if(p->hdr.code==TVN_ITEMEXPANDINGW) 
			SOM_MAIN_before(tv);
			else SOM_MAIN_and_after(tv); 
			break; 
		}}
		break;
	}	
	case WM_COMMAND:
	{		
		switch(wParam)
		{		
		//->       //<-       
		case 1018: case 1019: 
		{
			int r = wParam&1; if(r) //<-
			{
				if(SOM_MAIN_lang()) //mirror mode
				{
					SOM_MAIN_selendok(1011,L""); return 0;
				}
				else if(!SOM_MAIN_same()) //retract?
				{
					SOM_MAIN_retract();	return 0;
				}
			}
			else if(SOM_MAIN_lang()) //->
			{
				if(!SOM_MAIN_edit()) MessageBeep(-1); //script?
				return 0;
			}					
			HWND tv,dstv; HTREEITEM ti,dsti; //move?
			tv = (r?SOM_MAIN_right:SOM_MAIN_tree.left).treeview;
			dstv =(!r?SOM_MAIN_right:SOM_MAIN_tree.left).treeview;
			if(TreeView_GetParent(tv,ti=TreeView_GetSelection(tv)))
			if(TreeView_GetParent(dstv,dsti=TreeView_GetSelection(dstv)))
			{
				if(SOM_MAIN_textarea(tv,ti))
				return SOM_MAIN_error("MAIN240",MB_OK); //textarea->
				SOM_MAIN_textarea dsta(dstv,dsti); 
				SOM_MAIN_tree::string &dst =
				SOM_MAIN_tree[SOM_MAIN_param(dstv,dsta?dsta.item:dsti)];
				if(dst.nocontext())
				return SOM_MAIN_error("MAIN241",MB_OK); //->metadata
				SOM_MAIN_tree::string &src =
				SOM_MAIN_tree[SOM_MAIN_param(tv,ti)];
				if(dst.unoriginal()) //japanese?
				{
					if(!src.unoriginal()) //same origin???
					return SOM_MAIN_error("MAIN231",MB_OK); //->japanese
					return SOM_MAIN_report("MAIN1001",MB_OK); //unimplemented
				}
				if(src.unoriginal()&&!dst.ctxt_s)
				return SOM_MAIN_error("MAIN242",MB_OK); //japanese->messages
				//SOM_MAIN_150 will call SOM_MAIN_move
				if(dsta||TreeView_GetNextSibling(dstv,dsti))
				{
					SOM_MAIN_150(0,WM_INITDIALOG,0,r);
					SOM_MAIN_150(0,WM_CLOSE,0,0);				
				}
				else SOM_MAIN_(150,som_tool,SOM_MAIN_150,r); 
				return 0;
			}
			return MessageBeep(-1); 
		}
		case 1210: //delete
		{
			HWND tv = SOM_MAIN_right.treeview;			
			HTREEITEM i = TreeView_GetSelection(tv);
			if(!SOM_MAIN_textarea(tv,i)) SOM_MAIN_delete(tv,i);
			else assert(0); //paranoia
			break;
		}
		case 1211: //open
		{
			HWND tv = SOM_MAIN_right.treeview;
			HTREEITEM item = TreeView_GetSelection(tv);					
			SOM_MAIN_textarea ta(tv,item); if(ta)
			{
				int _ = LOWORD(Edit_GetSel(ta));

				if(_>ta.menu()) //SOM_MAIN_172
				return SendMessage(ta,WM_KEYDOWN,VK_F2,0);
				POINTL pt; 				
				SendMessage(ta,EM_POSFROMCHAR,(WPARAM)&pt,_);
				//WM_LBUTTONDBLCLK alone isn't working
				LPARAM lparam = MAKELPARAM(pt.x,pt.y);
				SendMessage(ta,WM_LBUTTONDOWN,0,lparam);
				SendMessage(ta,WM_LBUTTONUP,0,lparam);
				SendMessage(ta,WM_LBUTTONDBLCLK,0,lparam);
				SendMessage(ta,WM_LBUTTONUP,0,lparam);
				break;
			}
			SOM_MAIN_open(tv,item);
			break;
		}
		case 1084: //insert
		{
			HWND tv = SOM_MAIN_right.treeview;
			HTREEITEM l,r = TreeView_GetSelection(tv);			
			SOM_MAIN_textarea ta(tv,r); if(ta)
			{
				int _ = LOWORD(Edit_GetSel(ta));

				if(!ta.param||_<=ta.menu())				
				return SendMessage(ta,WM_KEYDOWN,VK_INSERT,0);	
				SOM_MAIN_(153,hWnd,SOM_MAIN_153b,ta.param);
				break;
			}
			SOM_MAIN_insert(tv,TreeView_GetParent(tv,r),r);
			break;
		}
		case IDOK: //Enter key?
		{
			HWND f = GetFocus();
			HWND tv = SOM_MAIN_right.treeview;
			if(f!=tv) tv = SOM_MAIN_tree.left.treeview;
			if(f!=tv) break;
			HTREEITEM ti = TreeView_GetSelection(tv);					
			return SOM_MAIN_create(tv,TreeView_GetParent(tv,ti),ti);
		}}
		break;
	}
	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,SOM_MAIN_commonproc,scID);
		break;
	}
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
extern int som_tool_et(wchar_t *p);
static void SOM_MAIN_swap(HWND,HTREEITEM,HTREEITEM=0);
static HTREEITEM SOM_MAIN_move(HWND,HTREEITEM,HWND,HTREEITEM);
static INT_PTR CALLBACK SOM_MAIN_150(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{		
	static HWND dstv;

	switch(uMsg)
	{
	case WM_INITDIALOG:
	{
		dstv = (lParam?SOM_MAIN_tree.left:SOM_MAIN_right).treeview;
		if(!wParam) return 0; //SOM_MAIN_commonproc

		static wchar_t title[30] = L""; //chkstk
		if(!GetWindowTextW(hwndDlg,title,8))
		if(GetDlgItemTextW(SOM_MAIN_151(),1018+lParam,title,EX_ARRAYSIZEOF(title)))
		if(som_tool_et(title)) //&
		SetWindowTextW(hwndDlg,title);

		if(som_tool==SOM_MAIN_151())
		{
			som_tool_recenter(hwndDlg,som_tool);
			RECT wr; if(GetWindowRect(hwndDlg,&wr))
			if(OffsetRect(&wr,0,-GetSystemMetrics(SM_CYCAPTION)))
			MoveWindow(hwndDlg,wr.left,wr.top,wr.right-wr.left,wr.bottom-wr.top,0);
		}
		else som_tool_recenter(hwndDlg,GetDlgItem(SOM_MAIN_152(),1018+lParam));
		return 1;
	}
	case WM_DRAWITEM: //REMOVE ME?
	{				 			  		
		return som_tool_drawitem((DRAWITEMSTRUCT*)lParam,156);
	}
	case WM_COMMAND:

		switch(wParam)
		{
		case IDOK: 
		{
			HTREEITEM i = 
			(HTREEITEM)SOM_MAIN_150(hwndDlg,WM_CLOSE,0,0);
			if(i) SOM_MAIN_swap(dstv,i);
			if(i) TreeView_EnsureVisible(dstv,i);
			return 0;
		}
		case IDCANCEL: goto close;
		}
		break;

	case WM_CLOSE: close: 

		DestroyWindow(hwndDlg);	
		HWND tv = SOM_MAIN_tree[dstv]->treeview;
		HTREEITEM ti = TreeView_GetSelection(tv);
		HTREEITEM tip = TreeView_GetPrevSibling(tv,ti);
		HTREEITEM i = SOM_MAIN_move(tv,ti,dstv,TreeView_GetSelection(dstv));
		if(!i) return 0;
		TreeView_SelectItem(dstv,i); if(tip) TreeView_SelectItem(tv,tip);		  
		return (INT_PTR)i; //IDOK
	}
	return 0;	
}

extern void SOM_MAIN_updowns(HWND ch, RECT &rc)
{	
	int uds = GetWindowStyle(ch); 
	if(~uds&UDS_HORZ) return;
	LONG split = (rc.right-rc.left)/2;
	if(UDS_ALIGNRIGHT&uds) //accuracy 				
	rc.right-=split; else rc.left = rc.right-split;
}
static LRESULT CALLBACK SOM_MAIN_updownproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scID, DWORD_PTR)
{
	switch(uMsg)
	{	
	case WM_ERASEBKGND:

		return 1; //helping?

	case WM_PAINT:
		
		//Surprised the compiler is okay with this :(
		goto paint;

	case WM_LBUTTONUP:
	case WM_LBUTTONDOWN: 
	{	
		HWND gp = GetParent(hWnd);
		int id = GetDlgCtrlID(hWnd), hl = 0; 			
		int hid = GetWindowContextHelpId(gp);
		if(uMsg==WM_LBUTTONDOWN) 
		{
			switch(id+hid)
			{
			case 1018+10300: case 1019+10300: //<<>>
			case 1082+10200: case 1083+10200: 
			case 1082+10300: case 1083+10300: //find 
				
				hl = 1213; break; //unimplemented

			case 1084+10200: //split-button-like ticker
				
				hl = 1211; break;

			case 1082+10400: //move up/down cut ticker

				hl = 1226; break; //unimplemented

			case 1083+10400: //dpad-like close tickers
			case 1084+10400: case 1085+10400:
				
				hl = 8;	break; 

			case 1080+10500: case 1081+10500: //scoping

				hl = 1212; break; 

			case 1082+10500: case 1083+10500: //reading

				hl = 1; break; 
			}			
		}	
		paint: if(UDS_HORZ&GetWindowStyle(hWnd))
		{
			RECT cr; if(GetClientRect(hWnd,&cr))
			{
				//give theme pack some control
				bool right = UDS_ALIGNRIGHT&GetWindowStyle(hWnd);

				LONG split = (cr.right-cr.left)/2;
				if(right==(uMsg==WM_PAINT)) //accuracy 
				cr.left = cr.right-split; else cr.right-=split;

				switch(uMsg) //cut the updown in half
				{
				case WM_PAINT: //hack: could clip instead
					
					ValidateRect(hWnd,&cr); break;

				case WM_LBUTTONUP: 
				
					InflateRect(&cr,3,3); //whatever works...

				case WM_LBUTTONDOWN:

					POINT pt = {GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};					

					if(!PtInRect(&cr,pt)) 
					{				
						//1019: the < and > buttons on 152
						if(1019==(id|1)&&hWnd!=GetCapture())
						return SendMessage(GetDlgItem(gp,id^1),uMsg,wParam,lParam);
						//prevent clicks
						//MessageBeep: SOM_MAIN_buttonproc
						if(uMsg&1) return MessageBeep(-1); 
						//prevent release
						lParam = cr.right*4; 
					}					
					break; 
				}
			}
		}
		if(uMsg==WM_LBUTTONDOWN)
		if(hl) som_tool_highlight(gp,hl);
		if(uMsg==WM_LBUTTONUP&&hWnd==GetCapture()) 
		{	
			RECT cr; GetClientRect(hWnd,&cr); 
			POINT pt = {GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
			InflateRect(&cr,3,3); //whatever works
			if(!PtInRect(&cr,pt)) break;
		
			switch(id+hid)
			{
			case 1080+10200: //ear tickers
			case 1081+10200: case 1081+10300: 
				
				//50: give button enough time to spring up
				SetTimer(som_tool,1&id,50,SOM_MAIN_escape); 
				break;

			case 1082+10200: case 1083+10200: 
			case 1082+10300: case 1083+10300: //find 
			case 1082+10400: //move up/down cut ticker
			case 1084+10200: //split-button-like ticker
			case 1083+10400: //dpad-like close tickers
			case 1084+10400: case 1085+10400:
			case 1080+10500: case 1081+10500: //scoping
			case 1082+10500: case 1083+10500: //reading
			case 1084+10500: //columns / outline flyout

				if(UDS_HORZ&GetWindowStyle(hWnd))
				{
					if(pt.x<cr.right/2) (SHORT&)id = -id;
				}
				else if(pt.y<cr.bottom/2) (SHORT&)id = -id;

			case 1018+10300: case 1019+10300: //<<>>

				SendMessage(gp,WM_COMMAND,id,0); 
				break;
			}
		}
		break;
	}
	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,SOM_MAIN_updownproc,scID);
		break;
	}					  	
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}     
static VOID CALLBACK SOM_MAIN_control(HWND win, UINT, UINT_PTR id, DWORD)
{
	if(~GetKeyState(id)>>15) KillTimer(win,id);	else return;
	
	//send back thru SOM_MAIN_buttonproc to pick which side to push
	if(GetKeyState(VK_CONTROL)>>15) SendMessage(win,WM_KEYUP,id,0);
}
static LRESULT CALLBACK SOM_MAIN_buttonproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scID, DWORD_PTR)
{
	static WPARAM repeat = 0;

	switch(uMsg) 
	{
	case WM_KEYUP: if(repeat!=wParam) break; //double beeps
	case WM_KEYDOWN: 			
	if(GetKeyState(VK_CONTROL)>>15
	&&wParam!=VK_CONTROL&&wParam!=VK_SHIFT
	&&!iswalpha(wParam)&&!iswdigit(wParam)) //&Mneumonics
	{				
		if(repeat!=wParam) 
		if(!GetCapture()) repeat = wParam; //driving ticker
		else break; 		

		//VK_LBUTTON: som_tool_buttonproc masking VK_SPACE
		bool space = wParam==VK_SPACE||wParam==VK_LBUTTON;
		bool u = wParam==VK_UP, d = wParam==VK_DOWN, l = wParam==VK_LEFT, r = wParam==VK_RIGHT;

		HWND gp = GetParent(hWnd), checkbox = 0;  
		int ud = 0, ud2 = 0, hid = GetWindowContextHelpId(gp);		
		switch(GetWindowID(hWnd)+hid)
		{ 
		case 1211+10200: //split-button ticker
			
			if(r||space) ud = 1084; break; //insert

		case 1226+10400: //move up/down cut ticker

			if(u||d) ud = 1082; break;

		case 1+10300: //hide saftey-lock checkbox
		case 1211+10400: //create direction checkbox

			if(uMsg==WM_KEYDOWN)
			if(space||r||l||d||u)												
			checkbox = GetDlgItem(gp,1200);
			break;

		case 8+10400: //dpad-like close tickers		
			
			ud = u|d?1083:l?1084:r?1085:0; break;

		case 1212+10500: //scoping

			//trackbar
			if(r||l) return SendDlgItemMessage(gp,1201,uMsg,wParam,lParam);
			ud2 = 1080; ud = 1081; break; 
		
		case 1+10500: //reading

			//hack: this belongs to scoping (todo: more general solution)
			if(r||l) return SendDlgItemMessage(gp,1201,uMsg,wParam,lParam);
			//break;

		case 1213+10200: case 1213+10300: //find 
			
			if(r||l)
			{
				ud = 1018|(int)l; //bogus C4806
			}
			else if(u||d)
			{
				ud2 = 1082; ud = 1083;

				if(SOM_MAIN_leftonly()) std::swap(ud2,ud);
			}
			break; 
		}		
		if(!ud) if(r||l||space) 
		{
			for(int side=1;side<=2;side++) 
			{
				switch(hid+GetWindowID(GetWindow(hWnd,side+1)))
				{
				case 10200+1080: ud = 1080; break;
				case 10200+1081: ud = 1081; break;
				case 10300+1081: ud = 1081; break;
				case 10500+1084: ud = 1084; break;
				}
			}		
		}
		if(checkbox) 
		if(!ud||GetKeyState(VK_SHIFT)>>15)
		{
			//som_tool_highlight(hWnd);
			Button_SetCheck(checkbox,!Button_GetCheck(checkbox));				
			SendMessage(gp,WM_COMMAND,GetWindowID(checkbox),(LPARAM)checkbox);			
			return repeat = 0;
		}
		HWND tick = ud?GetDlgItem(gp,ud):0;
		HWND tick2 = ud2?GetDlgItem(gp,ud2):0;
		if(!IsWindowEnabled(tick)) tick = 0;
		if(ud2&&!IsWindowEnabled(tick2)) tick2 = 0;
		if(tick&&tick2)
		tick = GetKeyState(VK_SHIFT)>>15?tick2:tick;
		else tick = tick?tick:tick2; if(!tick)
		{
			repeat = 0;	return MessageBeep(-1);
		}
		if(space) r = UDS_ALIGNRIGHT&~GetWindowStyle(tick);
		LPARAM lp = 0; if(r||d)
		{
			RECT cr = {0,0,0,0}; GetClientRect(tick,&cr);

			lp = r?cr.right/2+4:(cr.bottom/2+4)<<16;
		}
		else if(!l&&!u) return MessageBeep(-1);
		uMsg = uMsg==WM_KEYDOWN?WM_LBUTTONDOWN:WM_LBUTTONUP;		
		SendMessage(tick,uMsg,0,lp);
		if(uMsg==WM_KEYDOWN)
		SetTimer(hWnd,wParam,100,SOM_MAIN_control);
		return 0;
	}
	else break;
	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,SOM_MAIN_buttonproc,scID);
		break;
	}					  	
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
template<int N> //KB180077
static void SOM_MAIN_buttonup(HWND dlg, const int (&bts)[N], int ud0, int udN)
{
	for(int i=0;i<N;i++)
	SetWindowSubclass(GetDlgItem(dlg,bts[i]),SOM_MAIN_buttonproc,0,0);		
	for(int i=ud0;i<=udN;i++)
	{
		HWND ud = GetDlgItem(dlg,i); if(!ud) continue;
		SetWindowSubclass(ud,SOM_MAIN_updownproc,0,0);
		//PRB: Cannot Change Width of Vertical Up-Down Control 
		if(GetWindowStyle(ud)&(UDS_HORZ|UDS_AUTOBUDDY)) continue;
		RECT wr; GetWindowRect(ud,&wr); //undo user theme setting
		SetWindowPos(ud,0,0,0,16,wr.bottom-wr.top,SWP_NOMOVE|SWP_NOZORDER);
	}
}

static LRESULT CALLBACK SOM_MAIN_timeoutproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scID, DWORD_PTR calendar)
{
	switch(uMsg)
	{	
	case WM_CHAR:

		if(isalpha(wParam)||isdigit(wParam)) return MessageBeep(-1);
		break;

	case WM_KEYDOWN: 

		if(!calendar) switch(wParam)
		{
		case 'A': case VK_CONTROL: break; //control beeps, allow Ctrl+A

		default: if(GetKeyState(VK_CONTROL)>>15) return MessageBeep(-1);
		}
		else switch(wParam)
		{
		case VK_ESCAPE: case VK_RETURN: case VK_SPACE: case VK_F4: break;

		default: return MessageBeep(-1);
		}
		break;
	
	case WM_LBUTTONDOWN: case WM_RBUTTONDOWN:
		
		if(!calendar) break; return MessageBeep(-1);
	
	case WM_MOUSEHOVER: case WM_MOUSEMOVE: return 0; 

	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,SOM_MAIN_timeoutproc,scID);
		break;
	}					  	
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
static bool SOM_MAIN_links(HWND ta, int c)
{	
	DWORD sel = Edit_GetSel(ta);
	CHARFORMAT2W cfe; cfe.cbSize = sizeof(cfe); 
	Edit_SetSel(ta,c-1,c-1);
	bool first = 
	SendMessageW(ta,EM_GETCHARFORMAT,SCF_SELECTION,(LPARAM)&cfe)
	&&cfe.dwEffects&CFE_LINK;
	Edit_SetSel(ta,c,c);			
	bool second = 
	SendMessageW(ta,EM_GETCHARFORMAT,SCF_SELECTION,(LPARAM)&cfe)
	&&cfe.dwEffects&CFE_LINK;
	Edit_SetSel(ta,LOWORD(sel),HIWORD(sel));				
	return first&&second;
}
static LRESULT SOM_MAIN_pastext(HWND hWnd, bool serifed, int indent=-1)
{
	if(ES_READONLY&~GetWindowStyle(hWnd))
	{
		static HWND rtf = som_tool_richinit
		(CreateWindowExW(0,som_tool_richedit_class(),0,ES_MULTILINE,0,0,0,0,0,0,0,0),0);		
		//0: shouldn't let OLE objects in without IRichEditOleCallback
		SendMessage(rtf,EM_PASTESPECIAL,SOM_MAIN_poor?CF_UNICODETEXT:0,0);
		if(Edit_GetLineCount(rtf))
		{
			const wchar_t *face = serifed?
			som_932w_MS_Mincho:som_932w_MS_Gothic;
			SOM_MAIN_format(rtf,0,face,indent);
			ITextRange *src = SOM_MAIN_range(rtf);								
			ITextSelection *dst = SOM_MAIN_selection(hWnd);	
			//removing tacked on line that wasn't asked for			
			long wtf; src->GetEnd(&wtf); src->SetEnd(wtf-1); 
			if(dst->SetFormattedText(src)) MessageBeep(-1);
			dst->Collapse(0);
			src->Delete(1,0,0); //empty out
			src->Release(); dst->Release();
			return 0;
		}					
	}	
	return MessageBeep(-1);
}

static void SOM_MAIN_172(HWND,LPARAM,HWND=0);
static VOID CALLBACK SOM_MAIN_editest(HWND win, UINT, UINT_PTR id, DWORD)
{									 
	KillTimer(win,id); if(Edit_GetModify(win)) //SOM_MAIN_changed(1);
	{
		SOM_MAIN_changed(1); //is reversed in SOM_MAIN_textarea::close

		//NEW: don't want to do in SOM_MAIN_save_as
		LPARAM param = GetWindowID(win); if(!param) return; //paranoia
		if(!SOM_MAIN_tree[param].mmapid())
		if(!SOM_MAIN_tree[param].nocontext())
		if(!SOM_MAIN_textarea(win).close('f5')) //insufficient memory?
		{
			SetWindowTextW(win,L""); SendMessage(win,EM_EMPTYUNDOBUFFER,0,0);				
			Edit_SetModify(win,0);
		}
	}
}				
static LRESULT CALLBACK SOM_MAIN_richedproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scID, DWORD_PTR)
{
	static HWND hactivating = 0;

	switch(uMsg)
	{
	case WM_SETFOCUS: 
	{	
		HWND tv = GetParent(hWnd);		
		SOM_MAIN_textarea ta(hWnd); 
		if(!ta.param) SOM_MAIN_selectab(hWnd);
		HTREEITEM i = TreeView_GetChild(tv,ta.item);		
		if(i==TreeView_GetSelection(tv)) 
		{
			//grey buttons and set SOM_MAIN_tree.focus
			NMHDR nm = {tv,GetWindowID(tv),NM_SETFOCUS};
			SendMessage(som_tool,WM_NOTIFY,nm.idFrom,(LPARAM)&nm);		
		}
		else TreeView_SelectItem(tv,i);	SOM_MAIN_tree = tv; //!
		break;
	}
	case WM_KILLFOCUS: 
	{
		HWND tv = GetParent(hWnd);
		if(SOM_MAIN_before[tv].focus==hWnd) return 0;
		
		SOM_MAIN_textarea ta(hWnd); 
		if(!ta.param) SOM_MAIN_rainbowtab(hWnd);		
		break;
	}
	case WM_SIZE: //get icon out of the way

		//fyi: this doesn't really work in Win2k classic mode
		if(wParam==SIZE_MINIMIZED) return ShowWindow(hWnd,0);
		break;

	case WM_MOUSEWHEEL: //disable <1 zoom
		
		if(MK_CONTROL&wParam) 
		{
			int z = GET_WHEEL_DELTA_WPARAM(wParam); if(z<0)
			{
				DWORD zn=0,zd=0;
				SendMessage(hWnd,EM_GETZOOM,(WPARAM)&zn,(LPARAM)&zd);
				if(zn==1&&zd==1) return 0;
				DefSubclassProc(hWnd,uMsg,wParam,lParam);
				SendMessage(hWnd,EM_GETZOOM,(WPARAM)&zn,(LPARAM)&zd);
				if(zn<zd)
				SendMessage(hWnd,EM_SETZOOM,1,1);
				return 0;
			}
		}
		break;

	case WM_PASTE: goto paste;

	case WM_KEYDOWN: 
		
		//if(!ta.param)
		if(!GetWindowID(hWnd)) SOM_MAIN_rainbowtab(hWnd);

		if(!Edit_GetModify(hWnd))
		if(ES_READONLY&~GetWindowStyle(hWnd))		
		SetTimer(hWnd,'edit',0,SOM_MAIN_editest); //SOM_MAIN_changed(1);

		switch(wParam)
		{			
		case 'V': //paste 

			if(GetKeyState(VK_CONTROL)>>15)
			{
				paste: SOM_MAIN_textarea ta(hWnd);
				return SOM_MAIN_pastext(hWnd,ta.param&&SOM_MAIN_tree[ta.param].serifed());
			}
			break;		

		case VK_ESCAPE:
		
			return SendMessage(som_tool,WM_COMMAND,IDCANCEL,0);
		
		case VK_TAB:

			if(~GetKeyState(VK_CONTROL)>>15) 
			PostMessage(GetParent(hWnd),uMsg,wParam,lParam);			
			break;

		case VK_DOWN: case VK_UP: 

			if(GetKeyState(VK_SHIFT)>>15) break;	
			if(Edit_LineFromChar(hWnd,-1)
			 ==(wParam==VK_UP?0:Edit_GetLineCount(hWnd)-1))
			{
				HWND tv = GetParent(hWnd); 
				HTREEITEM x, i = (HTREEITEM)GetWindowLong(hWnd,GWL_USERDATA);
				if(wParam==VK_DOWN) //select next item  
				for(;i&&!(i=TreeView_GetNextSibling(tv,x=i));i=TreeView_GetParent(tv,x));
				if(!i) return MessageBeep(-1);
				//0: workaround SOM_MAIN_focus timer
				SetFocus(0); TreeView_SelectItem(tv,i); 
				SetFocus(tv); 
				return 0;
			}
			break;

		case VK_RIGHT: case VK_RETURN: //expand menu

			if(!GetWindowID(hWnd)&&~GetKeyState(VK_SHIFT)>>15)
			if(HIWORD(Edit_GetSel(hWnd))==GetWindowTextLengthW(hWnd))
			{
				HWND tv = GetParent(hWnd);
				HTREEITEM i = (HTREEITEM)GetWindowLong(hWnd,GWL_USERDATA);				
				SetFocus(SOM_MAIN_message_text(SOM_MAIN_param(tv,i),tv,i));
				return 0;
			}
			break;

		case 'Z': //undo

			if(GetKeyState(VK_CONTROL)>>15)		  
			if(GetKeyState(VK_SHIFT)>>15) wParam = 'Y'; //redo
			break;

		case VK_SPACE: case VK_INSERT: //activate?
		{
			HWND tv = GetParent(hWnd);
			SOM_MAIN_textarea ta(hWnd);
			DWORD sel = Edit_GetSel(hWnd);			
			if(LOWORD(sel)>ta.menu()) break;

			//winsanity: SOM_MAIN_links returns true
			//on the front of a link if the caret was
			//moved into position by arrowing backward
			if(!hactivating)
			{	
				keybd_event(VK_LEFT,0,0,0);
				keybd_event(VK_LEFT,0,KEYEVENTF_KEYUP,0);
				keybd_event(VK_RIGHT,0,0,0);
				keybd_event(VK_RIGHT,0,KEYEVENTF_KEYUP,0);
				keybd_event(wParam,0,0,0);
				keybd_event(wParam,0,KEYEVENTF_KEYUP,0);
				hactivating = GetFocus();
				if(hactivating!=hWnd) SetFocus(hWnd);
				return 0;
			}
			else 
			{	
				if(GetFocus()!=hactivating)
				SetFocus(hactivating);
				hactivating = 0;				
			}
				
			if(wParam==VK_SPACE)
			if(SOM_MAIN_links(hWnd,LOWORD(sel)))
			{					
				wParam = VK_RETURN; break; //pass to EN_LINK
			}			
			int one = 0;
			if(wParam==VK_INSERT)
			if(SOM_MAIN_links(hWnd,LOWORD(sel)))
			{
				one = 1; //SOM_MAIN_activate for one				
			}
			SOM_MAIN_activate(tv,one,'open',0);
			return 0;
		}
		case VK_F2: //open "advanced" editor
		{
			SOM_MAIN_textarea ta(hWnd);
			if(ta.param&&LOWORD(Edit_GetSel(hWnd))>ta.menu())
			{	
				if(GetKeyState(VK_CONTROL)>>15) //better than nothing
				{
					SOM_MAIN_(153,som_tool,SOM_MAIN_153b,ta.param);
				}
				else SOM_MAIN_172(som_tool,ta.param,ta);
			}
			else MessageBeep(-1); 
			break;
		}
		case VK_F5: //refresh memory-mapped-file
			
			return SOM_MAIN_textarea(hWnd).close('f5');
		}
		break;
	
	case WM_LBUTTONDOWN: 
		
		//if(!ta.param)
		if(!GetWindowID(hWnd)) SOM_MAIN_rainbowtab(hWnd);
		//no clue why this is necessary
		//(is some code in this file to blame?)
		//but it only becomes a problem when switching modes???		
		SetFocus(hWnd);	break;

	case WM_LBUTTONDBLCLK: //activate?
	{
		POINT pt = {GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
		int c = SendMessage(hWnd,EM_CHARFROMPOS,0,(LPARAM)&pt);
		if(c<=SOM_MAIN_textarea(hWnd).menu()) 
		{
			if(!SOM_MAIN_links(hWnd,c))
			return SetTimer(GetParent(hWnd),'open',0,SOM_MAIN_activate);			
		}
		break;
	}
	case WM_CHAR: case WM_UNICHAR:
				
		//!=: suppresses beeps during activation
		if(!GetWindowID(hWnd)||hWnd!=GetFocus()||hactivating) 
		{
			switch(wParam) //hack: act like the back is protected 
			{
			case '\r': case ' ': break;	default: MessageBeep(-1);
			}
			return 0;
		}
		break;

	case WM_NCDESTROY:
	{
		LPARAM param = GetWindowID(hWnd);
		if(param&&hWnd==SOM_MAIN_tree[param].undo)
		SOM_MAIN_tree[param].undo = 0;
		break;
	}}					  
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}

static const POINT SOM_MAIN_upcascade_pt = {0,0};
extern void som_tool_toggle(HWND,HTREEITEM,POINT,int);
static void SOM_MAIN_upcascade(HWND tv, HTREEITEM hti, const POINT &pt)
{
	HTREEITEM gp = TreeView_GetParent(tv,hti); if(!gp) return;
	SOM_MAIN_upcascade(tv,gp,pt); som_tool_toggle(tv,gp,pt,TVE_EXPAND); 
}
static HWND SOM_MAIN_cascade(HWND tv, HTREEITEM hti, LPARAM url=0, const POINT &pt=SOM_MAIN_upcascade_pt)
{
	if(!pt.x&&!pt.y) SOM_MAIN_upcascade(tv,hti,pt);

	som_tool_toggle(tv,hti,pt,TVE_EXPAND);
	LPARAM param = url?url:SOM_MAIN_param(tv,hti);
	while(hti=TreeView_GetChild(tv,hti)) 
	{	
		SOM_MAIN_textarea ta(tv,hti); if(ta)
		{
			if(ta.param!=param)	return SOM_MAIN_message_text(param,tv,ta.item);
		}
		else som_tool_toggle(tv,hti,pt,TVE_EXPAND);
	}	
	return 0;
}
static void SOM_MAIN_clearaway(HWND tv, HTREEITEM ti)
{
	HTREEITEM placeholder = TreeView_GetChild(tv,ti);
	if(!placeholder) return; 
	SOM_MAIN_textarea ta(tv,placeholder);
	if(!ta||!ta.param) return; 
	LPARAM param = SOM_MAIN_param(tv,ti);
	if(ta.param!=param) return;
	if(SOM_MAIN_tree[param].undo==ta) return;
	HWND test = SOM_MAIN_message_text(param,tv,ta.item); 
	assert(test&&!SOM_MAIN_textarea(test).param);
}
static VOID CALLBACK SOM_MAIN_enrichlink(HWND win, UINT, UINT_PTR id, DWORD)
{
	KillTimer(win,id); SOM_MAIN_textarea ta(win); SOM_MAIN_message_text(id,ta.treeview(),ta.item);
}						
static LRESULT CALLBACK SOM_MAIN_enrichproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scID, DWORD_PTR)
{	 
	switch(uMsg)
	{	
	case TVM_SELECTITEM:
	{
		RECT ir, cr; GetClientRect(hWnd,&cr);
		TreeView_GetItemRect(hWnd,(HTREEITEM)lParam,&ir,0);		
		if(ir.top>0&&ir.top+TreeView_GetItemHeight(hWnd)<cr.bottom)
		break; 
		goto scrolling; //break;
	}
	scrolling:
	case WM_VSCROLL: 
	//case WM_MOUSEWHEEL: //disabled
	{
		switch(LOWORD(wParam)) //moving?
		{
		case SB_THUMBPOSITION: case SB_THUMBTRACK:
		if(HIWORD(wParam)==GetScrollPos(hWnd,SB_VERT)) 
		return DefSubclassProc(hWnd,uMsg,wParam,lParam); //no
		}	 
		SOM_MAIN_before_and_after raii(hWnd);
		LRESULT out = DefSubclassProc(hWnd,uMsg,wParam,lParam);	
		return out;
	}		
	case WM_KEYDOWN: //scrolling

		switch(wParam) //these don't generate WM_VSCROLL
		{		
		case VK_HOME: case VK_END: case VK_PRIOR: case VK_NEXT: 

		case VK_LEFT: goto scrolling; 

		//optimizing/scheduled obsolete
		//todo: goto scrolling if/when flicker is not an issue
		//(the goal is to not flicker unless neccessitated/masked by scolling)
		case VK_UP: case VK_DOWN: case VK_RIGHT: 
		
			if(0) goto scrolling; //testing

			if(!GetWindow(hWnd,GW_CHILD)) break;
			if(~GetKeyState(VK_CONTROL)>>15)
			{
				//jumps to selection 
				RECT offscreen, crud;
				if(!TreeView_GetItemRect(hWnd,
					TreeView_GetSelection(hWnd),&offscreen,0)
				  ||offscreen.top<0||!GetClientRect(hWnd,&crud)
				  ||offscreen.top>=crud.bottom) goto scrolling;
			}
			int before = GetScrollPos(hWnd,SB_VERT);
			bool up = wParam==VK_UP;
			HWND lv = 0, lv2 = 0, fv = 0, fv2 = 0;
			if(up) lv = SOM_MAIN_textarea(hWnd,TreeView_GetLastVisible(hWnd));
			if(!up) fv = SOM_MAIN_textarea(hWnd,TreeView_GetFirstVisible(hWnd));
			LRESULT out = DefSubclassProc(hWnd,uMsg,wParam,lParam);							
			if(before==GetScrollPos(hWnd,SB_VERT)) 
			return out;			
			if(up) lv2 = SOM_MAIN_textarea(hWnd,TreeView_GetLastVisible(hWnd));
			if(!up) fv2 = SOM_MAIN_textarea(hWnd,TreeView_GetFirstVisible(hWnd));
			//these are beyond before's reach
			if(fv!=fv2&&fv) ShowWindow(fv,0);
			if(lv!=lv2&&lv) ShowWindow(lv,0);			
			//invalidating the old positions
			SOM_MAIN_before_and_after(hWnd);
			return out;
		}
		break;

	case WM_NOTIFY: 
	{
		ENLINK  *p = (ENLINK*)lParam;

		switch(p->nmhdr.code)
		{		
		case EN_REQUESTRESIZE:
		{
			REQRESIZE *p = (REQRESIZE*)lParam;

			if(p->rc.right) //icon?
			SOM_MAIN_requestresize(hWnd,p->nmhdr.hwndFrom,p->rc.bottom-p->rc.top);
			break;
		}
		case EN_PROTECTED:
		{
			switch(p->msg)
			{
			case WM_COPY: case EM_SETCHARFORMAT: return 0;	
			}
			return 1;
		}
		case EN_LINK:
		{
			//the link may be extractable as CFE_HIDDEN? Ah it contains just the link
			//http://blogs.msdn.com/b/murrays/archive/2009/09/24/richedit-friendly-name-hyperlinks.aspx
			//EM_GETTEXTRANGE?
			
			switch(p->msg)
			{
			case WM_SETCURSOR: 

				//4 is 10 in som.tool.cpp
				//supposed to be a WM_MOUSE code
				//Windows 10?
				//assert(HIWORD(lParam)==4); 
				SetCursor(LoadCursor(0,IDC_ARROW)); 
				return 1;

			//case WM_LBUTTONUP: assert(0);
			//UP is sent for keys but not for mice???
			case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK: 
			{
				WCHAR url[9+8] = L"", *e;
				HWND ta = p->nmhdr.hwndFrom;
				TEXTRANGE tr = { p->chrg,url }; 				
				if(8+8==p->chrg.cpMax-p->chrg.cpMin)
				if(8+8==SendMessageW(ta,EM_GETTEXTRANGE,0,(LPARAM)&tr))				
				for(unsigned __int64 link=_wcstoui64(url,&e,16);8+8==e-url;)
				{		
					LPARAM param = //NEW: guard against deadlinks
					SOM_MAIN_tree[GetParent(ta)].follow_link(link);
					if(param)
					if(p->msg==WM_LBUTTONDBLCLK
					 ||GetKeyState(VK_SPACE)>>16) //masquerating as VK_RETURN
					{
						KillTimer(ta,param);
						SOM_MAIN_open(hWnd,SOM_MAIN_tree[param][hWnd]);
					}
					else if(p->msg==WM_LBUTTONDOWN)
					{
						if(GetFocus()==ta)
						SetTimer(ta,param,
						GetDoubleClickTime(),SOM_MAIN_enrichlink);						
						POINT pt; 
						if(GetCursorPos(&pt)&&DragDetect(ta,pt))
						{
							KillTimer(ta,param);
							return 0;
						}
					}									
					//makes the double-click wait feel more responsive
					ITextSelection *sel = SOM_MAIN_selection(ta);
					sel->SetRange(p->chrg.cpMin,p->chrg.cpMin);
					sel->Expand(tomLink,0); sel->Release();		
					if(!param) //link is no longer viable
					SOM_MAIN_report("MAIN214",MB_OK);
					return 1;
				}		
				else if(p->msg!=WM_LBUTTONDOWN) break;

				POINT pt; 
				if(GetCursorPos(&pt)&&!DragDetect(ta,pt))
				{
					WCHAR autourl[1024] = L""; tr.lpstrText = autourl;
					if(EX_ARRAYSIZEOF(autourl)-1>tr.chrg.cpMax-tr.chrg.cpMin)
					SendMessageW(ta,EM_GETTEXTRANGE,0,(LPARAM)&tr);				
					if(!*autourl
					 ||(HINSTANCE)33>ShellExecuteW(hWnd,L"openas",autourl,0,0,1)
					 &&(HINSTANCE)33>ShellExecuteW(hWnd,L"open",autourl,0,0,1))
					MessageBeep(-1);
					return 1;
				}
				break; 
			}}
			break;
		}
		case EN_STOPNOUNDO: assert(0); 
		{
			//1: no clue what the docs are trying to say???
			//(unlikely this is related to EM_SETUNDOLIMIT)
			SOM_MAIN_error("MAIN213",MB_OK); return 1;
		}}
		break;
	}		  
	case WM_LBUTTONDBLCLK: //cascade 
	{	
		//done here since NM_DBLCLK ignores "whitespace"
		TVHITTESTINFO hti; POINTSTOPOINT(hti.pt,lParam);			
		if(TreeView_HitTest(hWnd,&hti)
		  &&hti.flags&(TVHT_ONITEMBUTTON|TVHT_ONITEMINDENT))
		{
			SOM_MAIN_cascade(hWnd,hti.hItem,0,hti.pt);
			return 0; //prevent NM_DBLCLK			
		}
		break;
	}	
	case WM_SETFOCUS: 
	{
		if(!SOM_MAIN_before[hWnd].focus) //optimizing
		{
			//the intent here is to return focus on navigation
			SOM_MAIN_textarea ta(hWnd,TreeView_GetSelection(hWnd));
			if(ta) SetFocus(ta); else break;		
		}
		return 0; //NM_SETFOCUS
	}}					  
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}

static LPARAM SOM_MAIN_message_menu(std::string &urtf, HWND tv, HTREEITEM i, LPARAM sel)
{
	HTREEITEM ii = i;
	LPARAM param = SOM_MAIN_param(tv,i);
	HTREEITEM gp = TreeView_GetParent(tv,i);
	if(TVIS_CUT&TreeView_GetItemState(tv,gp,~0))
	while(param==SOM_MAIN_param(tv,gp)) gp = TreeView_GetParent(tv,i=gp);
	LPARAM out = //recursively building up the menu
	gp?SOM_MAIN_message_menu(urtf,tv,gp,sel):param;	
		
	SOM_MAIN_tree::string &msg = SOM_MAIN_tree[param];

	char url[8+8+1] = "_"; 
	int compile[4+4==sizeof(SOM_MAIN_tree::link)];
	sprintf_s(url,"%016llx",SOM_MAIN_tree[tv].link_param(param));
	urtf+=sel==param?"<":"\\'20"; 
	urtf+="{\\field{\\*\\fldinst HYPERLINK \"";
	urtf+=url; 
	urtf+="\"}{\\fldrslt {\\ul0 "; 
	//link's "friendly name"	   	
	size_t sz = urtf.size();
	urtf+=EX::need_ansi_equivalent(65001,SOM_MAIN_label(tv,i)); 	
	//NEW: string together any phantom headings
	if(TVIS_CUT&TreeView_GetItemState(tv,i,~0)) while(i!=ii) 
	{ 
		i = TreeView_GetChild(tv,i);
		const char *phantom = EX::need_ansi_equivalent(65001,SOM_MAIN_label(tv,i));
		//hack: assuming repeating if equal
		if(strcmp(urtf.c_str()+sz,phantom))
		{
			urtf+="\\'20\\'20";	sz = urtf.size();
			urtf+=phantom;
		}
	}
	urtf+="}}}";
	urtf+=sel==param?">":"\\'20"; return out;
}	
static int SOM_MAIN_message_text_beheader = 0;
static HWND SOM_MAIN_message_text(LPARAM url, HWND tv, HTREEITEM i, HWND move)
{	
	bool behead = move;
	TVITEMEXW tvi = {TVIF_PARAM}; 
	HTREEITEM &ii = tvi.hItem = TreeView_GetChild(tv,i);			
	HWND text = move, copy = 0, tv2 = tv; 
	HWND prev = move?0:(HWND)SOM_MAIN_param(tv,ii); 
	bool focus = prev==GetFocus();
	if(!move){ //manual
	ShowWindow(prev,0); //Win2k classic
	if(url) if(url==GetDlgCtrlID(prev))
	{
		url = 0; CloseWindow(prev); //minimize
	}
	else if(IsWindow(SOM_MAIN_tree[url].undo)) 
	{
		text = SOM_MAIN_tree[url].undo; tv2 = GetParent(text);
		HTREEITEM j = (HTREEITEM)GetWindowLong(text,GWL_USERDATA);
		behead = SOM_MAIN_param(tv,i)!=SOM_MAIN_param(tv2,j);						
		if(!IsIconic(text)&&(tv!=tv2||i!=j)) 
		std::swap(copy,text); //read-only copy		
	}}
	bool empty = !copy&&!text;	

	if(!text) //create a new text or make a copy
	{	
		int ex = GetWindowExStyle(tv)&(WS_EX_RIGHT|WS_EX_RTLREADING);
		int es = ES_MULTILINE|ES_WANTRETURN|ES_NOHIDESEL|ES_NOOLEDRAGDROP;		
		text = CreateWindowEx(ex,som_tool_richedit_class(),0,es|WS_CHILD,0,0,0,0,tv2,(HMENU)url,0,0);				
		SetWindowSubclass(text,SOM_MAIN_richedproc,0,0);	
		SendMessage(text,EM_SETTARGETDEVICE,0,0); //wrap					
		SendMessage(text,EM_SETUNDOLIMIT,url?SOM_MAIN_undos:0,0);				
				
		assert(!Edit_GetModify(text));		
		Edit_SetModify(som_tool_richinit(text,0),0);
			
		if(copy) //slight of hand
		{
			std::swap(copy,text); 
			LONG i; DWORD zn=0,zd=0;
			SetWindowLong(copy,GWL_USERDATA,
			i=GetWindowLong(text,GWL_USERDATA));			
			TVITEMEXW tvi = {TVIF_PARAM};
			HTREEITEM &ii = tvi.hItem = 
			TreeView_GetChild(tv2,(HTREEITEM)i);
			tvi.lParam = (LPARAM)copy;
			for(;ii;ii=TreeView_GetNextSibling(tv2,ii))
			TreeView_SetItem(tv2,&tvi);
			DWORD sel = Edit_GetSel(text);
			SOM_MAIN_copyrichtext(copy,text,COLOR_3DFACE);			
			Edit_SetReadOnly(copy,1); 
			SendMessage(copy,EM_SETEVENTMASK,0,ENM_LINK|ENM_REQUESTRESIZE);	
			SendMessage(text,EM_GETZOOM,(WPARAM)&zn,(LPARAM)&zd);
			SendMessage(copy,EM_SETZOOM,zn,zd);
			RECT pos; GetWindowRect(text,&pos); MapWindowRect(0,tv2,&pos);
			SetWindowPos(copy,text,0,pos.top,pos.right-pos.left,pos.bottom-pos.top,0);
			ShowWindow(text,0); Edit_SetSel(text,LOWORD(sel),HIWORD(sel));
			ShowWindow(copy,1); Edit_SetSel(copy,LOWORD(sel),HIWORD(sel));			
			//SOM_MAIN_rainbowtab(copy);
			SendMessage(copy,EM_SETBKGNDCOLOR,0,GetSysColor(COLOR_3DFACE));
			Edit_SetModify(copy,0);
		}		
	}
	SetParent(text,tv);
	SetWindowLong(text,GWL_USERDATA,(LONG)i); 	
	SOM_MAIN_rainbowtab(text);		

	SETTEXTEX st = {ST_KEEPUNDO|ST_SELECTION,CP_UTF8};			
	
	if(SOM_MAIN_right.metadata==url)
	{
		if(empty) //readonly view of metadata
		{	
			//trim: easier to do before EM_SETTEXTEX
			char *trim = (char*)SOM_MAIN_tree[url].str;
			char &n = trim&&*trim?trim[strlen(trim)-1]:(char&)""[0];			
			char trimmed = n; if(n=='\n') n = '\0'; 
			SendMessage(text,EM_SETEVENTMASK,0,ENM_REQUESTRESIZE|ENM_LINK); 
			SendMessage(text,EM_SETTEXTEX,(WPARAM)&st,(LPARAM)trim);				
			if(trimmed) n = trimmed;
			Edit_SetReadOnly(text,1); SOM_MAIN_format(text,0); 			
			Edit_SetModify(text,0); SOM_MAIN_requestresize(tv,text); 
			Edit_SetSel(text,0,0);
			ShowWindow(text,1);
		}
		return text;
	}

	//backup in order to restore later on
	bool modified = Edit_GetModify(text);
	DWORD sel = Edit_GetSel(text), zn=0,zd=0;
	SendMessage(prev,EM_GETZOOM,(WPARAM)&zn,(LPARAM)&zd);
	//disabling EN_PROTECTED notifications
	SendMessage(text,EM_SETEVENTMASK,0,0); //EN_REQUESTRESIZE	

	if(!move) //manual
	{
		if(IsWindow(prev))
		if(Edit_GetModify(prev))
		CloseWindow(prev); else DestroyWindow(prev);	
		tvi.lParam = (LPARAM)text; 
		for(prev=0;ii;ii=TreeView_GetNextSibling(tv,ii))			
		TreeView_SetItem(tv,&tvi);	

		if(modified) OpenIcon(text);
		HWND z = url?GetDlgItem(tv,url):0;		
		RECT ir;TreeView_GetItemRect(tv,i,&ir,0);
		ir.top+=TreeView_GetItemHeight(tv);	
		RECT h = {0,0,0,1}; GetClientRect(text,&h);
		//SetWindowPos silently fails if z==text!
		SetWindowPos(text,z==text?0:z,ir.left,ir.top,ir.right-ir.left,h.bottom,0);	
	}
	
	int _head; LPARAM ctxt; if(behead) 
	{
		#define _HEAD ".HEAD"  //neither \n
		#define L_HEAD L".HEAD" //nor \r works		
		_head = SOM_MAIN_textarea(text).menu(); 
		Edit_SetSel(text,0,_head); //off with it...
	}
	std::string &urtf = SOM_MAIN_std::string(); if(behead||empty)
	{
		urtf = "{\\urtf1\\protect ";
		ctxt = SOM_MAIN_message_menu(urtf,tv,i,url);
		urtf+="{\\v" _HEAD "}}";	
		//Reminder: EM_STREAMIN discards the undo state
		SendMessage(text,EM_SETTEXTEX,(WPARAM)&st,(LPARAM)urtf.c_str());
	}
	
	int _ = LOWORD(Edit_GetSel(text));		

	if(url&&SOM_MAIN_tree[url].undo!=text) //the text
	{
		SOM_MAIN_tree[url].undo = text;		  
		SOM_MAIN_tree::string &msg = SOM_MAIN_tree[url];
		
		urtf = "{\\urtf1";
		//todo: insert the script's color table here?
		urtf+="\\protect\\line\\protect0 ";				
		if(ctxt==SOM_MAIN_right.japanese
		  &&ctxt!=url&&!msg.bilingual()) //show example
		{	
			//reminder: doesn't escape \{}
			SOM_MAIN_libintl(urtf,msg.id); 
		}
		else //todo: don't assume 0 terminated
		{
			const char *etc = msg.exml_text_component();
			if(SOM_MAIN_poor)
			SOM_MAIN_plaintextwithlines(urtf,etc?etc:msg.str);			
			else if(!etc)
			SOM_MAIN_plaintextwithlines(urtf,msg.str);
			else urtf+=etc;
		}
		urtf+="}"; 
		SendMessage(text,EM_SETTEXTEX,(WPARAM)&st,(LPARAM)urtf.c_str());
		SOM_MAIN_format(text,_+1,ctxt==url?0:som_932w_MS_Mincho);	
	}
	if(behead) //adjust the selection after beheading
	{
		if(LOWORD(sel)>=_head) ((WORD*)&sel)[0]+=_-_head;
		if(HIWORD(sel)>=_head) ((WORD*)&sel)[1]+=_-_head;		
	}
	if(!SOM_MAIN_message_text_beheader) //initialize it
	{
		//strlen(HEAD) now but it wasn't always and isn't guaranteed to be
		SOM_MAIN_message_text_beheader = _-SOM_MAIN_textarea(text).menu();	
	}
	
	//reenabling EN_PROTECTED notifications after replacing the protected regions
	SendMessage(text,EM_SETEVENTMASK,0,ENM_PROTECTED|ENM_LINK|ENM_REQUESTRESIZE);	
	Edit_SetSel(text,sel?LOWORD(sel):_+1,sel?HIWORD(sel):url&&focus?-1:_+1);	
	SendMessage(text,EM_SETZOOM,zn,zd);
	if(!modified) Edit_EmptyUndoBuffer(text);
	Edit_SetModify(text,modified);
	if(!move||!IsIconic(text))
	SOM_MAIN_requestresize(tv,text);	
	if(!url||SOM_MAIN_readonly(tv)) Edit_SetReadOnly(text,1);	
	if(focus) SetFocus(text);
	if(!move) ShowWindow(text,1); return text;
}
int SOM_MAIN_textarea::menu()
{	
	FINDTEXTW ft = {{0,-1},L_HEAD}; 
	//NEW: was caching in the Help ID after beheading
	//this solution is to work around undo operations	
	//note: ITextRange::FindText doesn't seem to work
	//HOWEVER the documentation says the selection is
	//used it isn't actually: FINDTEXT's CHARRANGE is
	//and so although dismaying! FindText is unneeded	
	int paranoia = SendMessageW(hwnd,EM_FINDTEXTW,FR_DOWN|FR_MATCHCASE|FR_WHOLEWORD,(LPARAM)&ft);											
	assert(paranoia>=0); //beheader may depend on RTF
	return paranoia<0?0:paranoia+SOM_MAIN_message_text_beheader;
}	
static bool SOM_MAIN_message_leftitem(HWND tv, HTREEITEM msgti, LPARAM param=0)
{
	if(SOM_MAIN_same()) //copy other side?
	if(tv!=SOM_MAIN_right.treeview) //present regime
	{	
		//obsolete: now _right is dominant
		HWND vt = SOM_MAIN_right.treeview;
		if(vt==tv) vt = SOM_MAIN_tree.left.treeview;	
		if(!param) param = SOM_MAIN_param(tv,msgti);
		SOM_MAIN_tree::string &msg = SOM_MAIN_tree[param];
		HTREEITEM twin = msg[vt]; if(twin)
		{				
			TVINSERTSTRUCTW tis = {msgti,TVI_LAST,
			{TVIF_TEXT|TVIF_PARAM|TVIF_CHILDREN|TVIF_STATE,0,0,TVIS_CUT|TVIS_BOLD}};	
			//0: seems to work like LVIF_NORECOMPUTE
			tis.item.pszText = LPSTR_TEXTCALLBACKW; tis.item.cchTextMax = 0;
			HTREEITEM &i = tis.item.hItem;

			int phantoms = //hack: grey/intermediary items
			SOM_MAIN_level(vt,twin)-SOM_MAIN_level(tv,msgti);
			while(phantoms-->0) twin = TreeView_GetParent(vt,twin);		
			if(phantoms==-1) if(i=TreeView_GetChild(vt,twin))
			{				
				do if(TreeView_GetItem(vt,&tis.item)) 
				if(!tis.item.cChildren||!SOM_MAIN_insertitem(tv,&tis)) break; 				
				while(i=TreeView_GetNextSibling(vt,i));
				if(!i) return false;
			}
		}
	}			
	else assert(0); return true; //previously expanded
}
inline void SOM_MAIN_message(HWND tv, HTREEITEM msgti, LPARAM param=0)
{
	//gutted SOM_MAIN_message_leftitem
	if(!SOM_MAIN_left(tv)||SOM_MAIN_message_leftitem(tv,msgti,param)) 
	SOM_MAIN_message_text(0,tv,msgti); 
}

static bool SOM_MAIN_outline(HWND tv, HTREEITEM ctxti, LPARAM param=0)
{	
	if(TreeView_GetChild(tv,ctxti)) return true;
	if(!param) param = SOM_MAIN_param(tv,ctxti);
	SOM_MAIN_tree::pool &pool = SOM_MAIN_tree[tv];
	SOM_MAIN_tree::string &ctxt = SOM_MAIN_tree[param], msg;

	TVINSERTSTRUCTW tis = {ctxti,TVI_LAST,
	{TVIF_TEXT|TVIF_PARAM|TVIF_CHILDREN|TVIF_STATE,0,0,TVIS_CUT}};		
	tis.item.cChildren = 1; //0: seems to work like LVIF_NORECOMPUTE
	tis.item.pszText = LPSTR_TEXTCALLBACKW; tis.item.cchTextMax = 0;
	
	if(SOM_MAIN_same()&&SOM_MAIN_left(tv)) //copy other side
	{
		//note: we are loading everything up on the right side
		//because the left side can change and it would be too
		//much of a headache to copy its stuff over if it does

		HTREEITEM &i = tis.item.hItem;
		HWND vt = SOM_MAIN_right.treeview;
		i = TreeView_GetChild(vt,ctxt.right);
		if(!i&&SOM_MAIN_outline(vt,ctxt.right,param))
		i = TreeView_GetChild(vt,ctxt.right);
		if(i&&!SOM_MAIN_textarea(vt,i))
		do if(TreeView_GetItem(vt,&tis.item)) 
		SOM_MAIN_insertitem(tv,&tis); else assert(0);
		while(i=TreeView_GetNextSibling(vt,i));			
		else return false; return true; //!!
	}
	
	struct outline
	{
		LPARAM param;
		const char *exml_li;
		outline(LPARAM l):param(l),exml_li(0)
		{							  		
			if(SOM_MAIN_tree[l].str_s) exml_li =
			SOM_MAIN_tree[l].exml_attribute(EXML::Attribs::li);
		}
		inline bool operator<(const outline &ol)const
		{
			const char *a = exml_li, *b = ol.exml_li;
			if(!a||!b) return b; 
			if(*a=='\''||*a=='"') a++; if(*b=='\''||*b=='"') b++;
			double cmp = 
			strtod(a,(char**)&a)-strtod(b,(char**)&b);
			while(cmp==0&&*a=='-'&&*b=='-')
			cmp = strtod(++a,(char**)&a)-strtod(++b,(char**)&b);
			if(cmp==0) return *b=='-';
			return cmp<0;			
		}
	};
	static std::vector<outline> ol;
		
	switch(*ctxt.id)
	{	
	default: //outline with msgctxt
	{
		mo::range_t ri;
		mo::range(pool.mo,ri,ctxt.id,strlen(ctxt.id));
		if(ri.n&&mo::msgctxt(ri)==ctxt.id) for(size_t i=1;i<ri.n;i++)		
		ol.push_back(pool.insert_param(ri[i]));
	}
	case 1: break; //metadata	
	case 2: case 3: //messages/japanese
	{
		bool japanese = 3==*ctxt.id;
		bool readonly = SOM_MAIN_readonly(tv);
		
		//try to open the nihongo.mo file
		static wchar_t nihongo_mo[MAX_PATH] = L""; 
		static bool one_off = false; if(!one_off++) //???
		{
			namespace zip = SWORDOFMOONLIGHT::zip;

			static zip::inflate_t titles; //32K (+16K that goes to waste)

			zip::mapper_t z; size_t sz;
			for(size_t i=sz=0;!sz&&*EX::text(i);i++)										
			if(PathIsDirectoryW(EX::text(i))) 
			{
				swprintf_s(nihongo_mo,L"%ls\\tool\\nihongo.mo",EX::text(i));		
				FILE *f = _wfopen(nihongo_mo,L"rb");
				if(f) sz = fread(titles,1,sizeof(titles.dictionary),f);
				if(f) fclose(f);					
			}
			else if(zip::open(z,EX::text(i),L"tool\\nihongo.mo"))
			{	
				zip::image_t unzip;
				zip::maptolocalentry(unzip,z,L"tool\\nihongo.mo");
				sz = zip::inflate(unzip,titles);
				zip::unmap(unzip); 
				zip::close(z); 
			}		
			if(!sz) //!*nihongo_mo) //fallback to the TOOL directory
			{
				swprintf_s(nihongo_mo,L"%ls\\tool\\nihongo.mo",SOM::Tool::install());
				FILE *f = _wfopen(nihongo_mo,L"rb");
				if(f) sz = fread(titles,1,sizeof(titles.dictionary),f);
				if(f) fclose(f);					
			}
			mo::maptorom(SOM_MAIN_tree.nihongo.mo,titles,sz);
			mo::validate00(SOM_MAIN_tree.nihongo.mo);

			//REMOVE ME??
			//this was to save the msgid
			//but now mmap is being used for that
			//now it is only used to quickly lookup msgstr
			SOM_MAIN_tree.jtable.clear();
			mo::range_t rj; mo::range(SOM_MAIN_tree.nihongo.mo,rj);		
			for(size_t j=0;j<rj.n;j++)
			SOM_MAIN_tree.jtable.push_back(rj[j]);
		}

		mo::range_t ri,rj; size_t j;
		mo::range(pool.mo,ri); mo::range(SOM_MAIN_tree.nihongo.mo,rj);		
		if(*nihongo_mo) //opening nihongo.mo?
		{			
			wchar_t path[MAX_PATH] = L"";
			GetDlgItemTextW(SOM_MAIN_151(),readonly?1011:1013,path,MAX_PATH);
			if(!wcsicmp(path,nihongo_mo)) 
			mo::range(SOM_MAIN_tree.nihongo.mo,rj,0,0);			
		}
		for(size_t i=j=0;i<ri.n;i++)
		{
			const char *jid,*id = mo::msgid(ri,i);
			while(j<rj.n&&0<strcmp(id,jid=mo::msgid(rj,j))) 
			{
				if(japanese&&!readonly)
				{
					msg = rj[j]; msg.mmap = -1; 
					ol.push_back(pool.insert_param(msg));					
				}
				j++;
			}

			size_t _4; //see if beginning of outline
			for(_4=0;_4<=SOM_MAIN_msgctxt_s&&id[_4]>'\4';_4++) 
			if(id[_4]=='\4') break;
			if(id[_4]=='\4'&&id[_4+1]=='\0') //exclude by outline
			{
				//note: there is no mechanism for setting up a message
				//that appears to belong to an outline for which there
				//is no outline (msgctxt with a blank msgid) available

				mo::range_t skip; 
				i+=mo::range(pool.mo,skip,ri.lowerbound+i,mo::upperbound(pool.mo,id,_4+1));
				if(skip.n) i--; else assert(0);
			}
			else if(japanese==(j<rj.n&&!strcmp(id,jid))) //exclude by filter
			{
				msg = ri[i];
				LPARAM lp = pool.insert_param(msg);				
				ol.push_back(lp); 

				if(japanese) //marking/overriding ordering
				{
					msg = rj[j++]; 			
					SOM_MAIN_tree[lp].id = msg.id;
					ol.back().exml_li = msg.exml_attribute(EXML::Attribs::li);					
				}
			}
		}		
		if(japanese&&!readonly) while(j<rj.n) //wrapup
		{
			msg = rj[j++]; msg.mmap = -1; 
			ol.push_back(pool.insert_param(msg));
		}
		break;
	}}

	if(!ol.size()) return false; 

	int (&oli)[SOM_MAIN_levels_s] = 
	pool.outlines[ctxt.outline].countperlevels;
	memset(oli,0x00,sizeof(oli));

	std::sort(ol.begin(),ol.end());		
	int li[SOM_MAIN_levels_s] = {1};
	const char *curr, *cend, *prev = "", *pend = prev; 	
	for(size_t i=0,n=ol.size(),l=0,ll=0;i<n;i++,prev=curr,pend=cend)
	{			
		LPARAM lp = ol[i].param;
		curr = ol[i].exml_li; if(!curr) curr = ""; 
		cend = curr; //EXML::Quote(curr).delim;		
		while(*cend>='0'&&*cend<='9'||*cend=='-'||*cend=='.') 
		cend++;
		
		tis.item.lParam = lp;
		const char *p = prev, *q = curr;
		for(;p<pend&&q<cend&&*p==*q;p++,q++);	 
		if(tis.hParent!=ctxti) 
		if(p<pend||q>=cend||*q!='-') 
		tis.hParent = TreeView_GetParent(tv,tis.hParent),l--;
		else q++;		
		while(p<pend) if(*p++=='-') 
		tis.hParent = TreeView_GetParent(tv,tis.hParent),l--;	
		if(l<SOM_MAIN_levels_s)
		{
			while(ll<l) li[++ll] = 1;
			SOM_MAIN_tree[lp].outline = li[l]++;
			while(ll>l) li[ll--] = 1;
		}
		else SOM_MAIN_tree[lp].outline = 0;
		while(q<cend) if(*q++=='-') 
		{
			if(l<SOM_MAIN_levels_s) oli[l]++;
			if(++l<SOM_MAIN_levels_s) li[ll=l] = 2;
			tis.hParent = SOM_MAIN_insertitem(tv,&tis);
			TreeView_SetItemState(tv,tis.hParent,TVIS_CUT,TVIS_CUT);
		}				
		if(l<SOM_MAIN_levels_s) oli[l]++;
		tis.hParent = SOM_MAIN_insertitem(tv,&tis),l++;		
	}
	int i = 0; while(i<SOM_MAIN_levels_s&&oli[i]) i++;	
	pool.outlines[ctxt.outline].levels = i;
	ol.clear();	return true;
}
static int CALLBACK SOM_MAIN_outline_sort(LPARAM a, LPARAM b, LPARAM)
{
	return strcmp(SOM_MAIN_tree[a].id,SOM_MAIN_tree[b].id);
}
template<typename T>
static LPARAM SOM_MAIN_outline_param(HWND tv, const T *msgctxt)
{
	int i; char sc[SOM_MAIN_msgctxt_s+2] = ""; if(msgctxt) 
	{
		for(i=0;i<sizeof(sc)-1;i++) 
		if((sc[i]=msgctxt[i])<='\4') break; switch(sc[i]) //!
		{
		default: sc[i=0] = '\0'; break;
		case '\0': sc[i] = '\4'; case '\4': sc[++i] = '\0'; break;
		}	
	}
	SOM_MAIN_tree::pool &pool = 
	SOM_MAIN_same()?SOM_MAIN_right:SOM_MAIN_tree[tv];	
	SOM_MAIN_tree::pool::iterator it = pool.find(sc);
	return it==pool.end()?pool.messages:SOM_MAIN_tree.string_param(*it);
}
static std::wstring &SOM_MAIN_outline_item(HWND tv=0, HTREEITEM i=0)
{
	static std::wstring out; if(!tv) return out;
	static LPARAM gparam; LPARAM param = SOM_MAIN_param(tv,i);
	HTREEITEM gp = TreeView_GetParent(tv,i); if(gp)
	{
		SOM_MAIN_outline_item(tv,gp); if(!out.empty()) out+='-';
		wchar_t w[32] = L"1";
		if(gparam!=param) _itow(SOM_MAIN_tree[SOM_MAIN_param(tv,i)].outline,w,10);
		out+=w;
	}
	else out.clear(); gparam = param; return out;
}
static int CALLBACK SOM_MAIN_outline_item_sort(LPARAM a, LPARAM b, LPARAM first)
{
	if(a==first) return -1; if(b==first) return 1; //phantom business
	return (int)SOM_MAIN_tree[a].outline-(int)SOM_MAIN_tree[b].outline;
}
static std::vector<size_t> &SOM_MAIN_outline_item_vector()
{
	static std::vector<size_t> out; out.clear();
	const wchar_t *p = SOM_MAIN_outline_item().c_str();
	do out.push_back(abs(wcstol(p,(wchar_t**)&p,10))); while(out.back());
	out.pop_back(); if(out.empty()) out.push_back(1); return out;
}
static std::pair<HTREEITEM,bool> SOM_MAIN_outline_item_pair(HWND tv, const char *ol)
{
	TVITEMEXW tvi = {TVIF_CHILDREN/*|TVIF_PARAM*/};	
	std::vector<size_t> &iv = SOM_MAIN_outline_item_vector();
	std::pair<HTREEITEM,bool> out(SOM_MAIN_tree[SOM_MAIN_outline_param(tv,ol)][tv],false);	
	if(!SOM_MAIN_outline(tv,out.first)) SOM_MAIN_message_text(0,tv,out.first);
	for(size_t i=0,j=1;i<iv.size()&&!out.second;out.second=!tvi.hItem,i++,j=1)	
	{
		tvi.hItem = TreeView_GetChild(tv,out.first);
		if(!tvi.hItem) SOM_MAIN_message(tv,out.first); 
		if(!tvi.hItem) tvi.hItem = TreeView_GetChild(tv,out.first);
		if(!tvi.hItem) assert(0); else do 
		{
			if(!TreeView_GetItem(tv,&tvi)){ assert(0); return out; }
			out.first = tvi.hItem; if(!tvi.cChildren) return out; //textarea?
			//if(SOM_MAIN_tree[tvi.lParam].outline>=iv[i]) break;
			if(j++>=iv[i]) break; //phantom business
		}
		while(tvi.hItem=TreeView_GetNextSibling(tv,tvi.hItem));
	}
	return out;
}
static DWORD SOM_MAIN_outline_item_octets()
{
	int ip[4] = {0,0,0,0};
	std::vector<size_t> &v = SOM_MAIN_outline_item_vector();
	for(size_t i=1;i<5&&i<v.size();i++) ip[i-1] = v[i];
	return MAKEIPADDRESS(ip[0],ip[1],ip[2],ip[3]);
}

static INT_PTR CALLBACK SOM_MAIN_149(HWND,UINT,WPARAM,LPARAM);
static INT_PTR CALLBACK SOM_MAIN_153(HWND,UINT,WPARAM,LPARAM);
static INT_PTR CALLBACK SOM_MAIN_154(HWND,UINT,WPARAM,LPARAM);
static bool SOM_MAIN_langdir(const wchar_t *dir, HWND tv, HTREEITEM i, const wchar_t *spec)
{
	if(!PathIsDirectory(dir))
	return !SOM_MAIN_error("MAIN083",MB_OK);
		
	bool foundout = false;
	wchar_t find[MAX_PATH] = L"";
	swprintf_s(find,L"%ls\\%ls",dir,spec?spec:L"*");
	WIN32_FIND_DATAW found;
	HANDLE glob = FindFirstFileW(find,&found);

	TVINSERTSTRUCTW tis = 
	{i,TVI_LAST,{TVIF_TEXT|TVIF_PARAM|TVIF_CHILDREN}};	
	tis.item.cChildren = 1;	   
	tis.item.pszText = found.cFileName;

	bool empty = true;
	EX::Locale lc; char ll_CC[32]; 
	if(glob!=INVALID_HANDLE_VALUE) do
	{												
		if(*found.cFileName=='.') continue; //./..
		empty = false;

		if(!spec)
		if(found.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
		{
			int i,_,c;
			for(i=_=0;c=found.cFileName[i];i++) 			
			if(c=='_'&&!_++||c<127&&!ispunct(c))
			{
				if(i<sizeof(ll_CC)-1) ll_CC[i] = c; 
				else break;
			}
			else break;	ll_CC[i] = '\0';	  			
			lc->desired = ll_CC; tis.item.lParam = lc;			
			if(TreeView_InsertItem(tv,&tis)) foundout = true;
		}
		else break; //safe to assume directories are first?

		if(spec)
		if(~found.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
		{
			if(TreeView_InsertItem(tv,&tis)) foundout = true;
		}

	}while(FindNextFileW(glob,&found)); 
	FindClose(glob);
	if(!foundout) //nothing to expand to
	if(empty) SOM_MAIN_report("MAIN1011",MB_OK);
	else if(IDYES==SOM_MAIN_report("MAIN1010",MB_YESNOCANCEL))
	SOM_MAIN_open(tv,i);
	return true;
}
static int SOM_MAIN_expand(HWND tv, HTREEITEM i, int how)
{	
	if(how=='+') //expanding
	{
		if(TreeView_GetChild(tv,i)) return 0;
	}
	else if(how=='-') //collapsing
	{
		if(SOM_MAIN_lang(tv)) return 0; 
		SOM_MAIN_textarea ta(tv,TreeView_GetChild(tv,i));
		if(ta) if(!ta.close()) return 1;
		if(ta) SetTimer(tv,'-',500,SOM_MAIN_cleanup);
		return 0;
	}
	else if(how=='edit') //-> to open script
	{
		i = TreeView_GetSelection(tv=SOM_MAIN_tree.left.treeview); 
	}

	LPARAM param = SOM_MAIN_param(tv,i);	
	HTREEITEM gp = TreeView_GetParent(tv,i);
	HTREEITEM gpp = TreeView_GetParent(tv,gp);	

	if(how=='edit'&&(!gp||param||!SOM_MAIN_lang())) return 0;
	
	if(param<65535) //directory
	{		
		wchar_t path[MAX_PATH] = L"";
		TVITEMW tvi = {TVIF_TEXT,gpp?gpp:gp?gp:i}; 
		tvi.pszText = path; tvi.cchTextMax = MAX_PATH;
		
		if(param||gp)
		{						
			if(param?!gp:!gpp)  
			GetDlgItemTextW(SOM_MAIN_151(),1011,path,MAX_PATH);						
			else TreeView_GetItem(tv,&tvi);
			size_t cat = wcslen(path);
			if(tvi.cchTextMax=MAX_PATH-cat-1)
			{
				path[cat++] = '\\';
				tvi.pszText = path+cat;	
				tvi.hItem = param?i:gp;
				if(TreeView_GetItem(tv,&tvi))
				{
					if(how=='+'||!param)
					wcscat_s(path+cat,MAX_PATH-cat-1,L"\\LC_MESSAGES");

					if(!param)
					{
						cat+=wcslen(path+cat);
						path[cat++] = '\\';
						tvi.pszText = path+cat;	
						if(tvi.cchTextMax=MAX_PATH-cat)
						{
							tvi.hItem = i;
							if(TreeView_GetItem(tv,&tvi)) switch(how)
							{
							case '+': case 'edit':

								int cbid = how=='edit'?1013:1011;

								//don't want to empty tree
								if(!PathFileExistsW(path))
								{
									//200: cannot load file
									SOM_MAIN_error("MAIN200",MB_OK);
									return 1;
								}
								SOM_MAIN_selendok(cbid,path);
								return 1;
							}
						}
					}
					else if(how=='+') 
					{
						SOM_MAIN_langdir(path,tv,i,L"*.mo");
					}
				}
			}
		}
		else if(TreeView_GetItem(tv,&tvi)&&how=='+') 
		{
			SOM_MAIN_langdir(path,tv,i);
		}

		if(how=='open'&&(!*path
		||(HINSTANCE)33>ShellExecuteW(GetParent(tv),L"openas",path,0,0,1)
		&&(HINSTANCE)33>ShellExecuteW(GetParent(tv),L"open",path,0,0,1)))
		MessageBeep(-1);
	}
	else //script
	{		
		if(how=='+') //expand
		{
			if(!gp) //show scratchpad?
			{
				if(!SOM_MAIN_outline(tv,i,param)) 
				SOM_MAIN_message_text(param,tv,i);
			}
			else SOM_MAIN_message(tv,i,param);
		}
		else if(how=='open')
		{	
			int id = 0; DLGPROC dp = 0; if(gp) //item
			{
				dp = SOM_MAIN_153; id = 153; 
			}
			else if(param!=SOM_MAIN_tree[tv].metadata) //outline
			{
				dp = SOM_MAIN_154; id = 154;
			}
			else //metadata (should say feature is missing from release)
			{
				SOM_MAIN_report("MAIN1001",MB_OK);	
				return 1;
			}			 
			SOM_MAIN_(id,som_tool,dp,param);
			return 1;
		}
		else assert(0);
	}
	return 0;
}
static void SOM_MAIN_insertafter(HWND dlg153,bool app=true);
static INT_PTR CALLBACK SOM_MAIN_insertproc(HWND,UINT,WPARAM,LPARAM);
static HWND SOM_MAIN_insertbefore(HWND tv, HTREEITEM ti, HTREEITEM tj, LPARAM(*f)(HWND,HTREEITEM))
{
	DLGPROC g = //hack?
	f==SOM_MAIN_param?SOM_MAIN_insertproc:SOM_MAIN_153;
	if(ti) if(!SOM_MAIN_readonly(tv))		
	switch(SOM_MAIN_tree[SOM_MAIN_toplevelparam(tv,ti)].nocontext())
	{
	case 1: MessageBeep(-1); break; case 0: case 2: 	
	SetFocus(tv); //NEW: SOM_MAIN_readytoserveviewtoo
	return SOM_MAIN_(153,som_tool_dialog(),g,f(tv,tj?tj:ti));
	default: SOM_MAIN_report("MAIN231",MB_OK); break;
	}
	else SOM_MAIN_report("MAIN230",MB_OK); else MessageBeep(-1);
	return 0;
}
static int SOM_MAIN_insert(HWND tv, HTREEITEM after, HTREEITEM inplaceof)
{
	if(!TreeView_GetParent(tv,after)) //top-level?
	{
		if(inplaceof==after) return(MessageBeep(-1),0);
		if(!inplaceof) return SOM_MAIN_create(tv,after,inplaceof); //NEW
	}
	HWND dlg153 = SOM_MAIN_insertbefore(tv,after,inplaceof,SOM_MAIN_param);
	if(!dlg153) return 0;
	if(!inplaceof) 
	{
		SOM_MAIN_insertafter(dlg153); 
		windowsex_enable<12321>(dlg153); //enable apply button
	}
	return 1;
}
static int SOM_MAIN_create(HWND tv, HTREEITEM within, HTREEITEM ontopof)
{
	HWND dlg153 = SOM_MAIN_insertbefore(tv,within,ontopof,SOM_MAIN_temparam);
	if(!dlg153) return 0;
	SOM_MAIN_outline_item(tv,ontopof?ontopof:within);
	SOM_MAIN_insertafter(dlg153,!ontopof);
	return 1;
}		   
static void SOM_MAIN_cutupitem(LPARAM);
static void SOM_MAIN_moveupitem(HWND,HTREEITEM);
static bool SOM_MAIN_removeitem(LPARAM,LPARAM=0);
static void SOM_MAIN_delete_subitems(HWND,HTREEITEM);
static HTREEITEM SOM_MAIN_produceitem(HWND,HTREEITEM);
static HTREEITEM SOM_MAIN_reproduceitem(HWND,HWND,HTREEITEM);
static int SOM_MAIN_delete(HWND tv, HTREEITEM i, int mb)
{	
	if(SOM_MAIN_readonly(tv)) 
	return !SOM_MAIN_report("MAIN230",MB_OK);			
	if(SOM_MAIN_lang(tv)) return(MessageBeep(-1),0);
	if(mb=='<-') i = //SOM_MAIN_retract
	TreeView_GetSelection(tv=SOM_MAIN_right.treeview);
	if(!TreeView_GetParent(tv,i)) //hollow?
	if(mb!='del'||~GetKeyState(VK_SHIFT)>>15) 
	return !SOM_MAIN_report("MAIN1001",MB_OK);	
	LPARAM param = SOM_MAIN_param(tv,i);
	int out = !SOM_MAIN_tree[param].unoriginal();
	int st = TreeView_GetItemState(tv,i,~0);	
	if(TVIS_CUT&st&&out) //pullout phantoms?
	if(mb=='del'&&~GetKeyState(VK_SHIFT)>>15)
	switch(GetKeyState(VK_CONTROL)>>15?IDYES:
	SOM_MAIN_warning("MAIN1002",MB_YESNOCANCEL))
	{		 
	case IDNO: break; case IDCANCEL: return 0;
	case IDYES:

		SOM_MAIN_moveupitem(tv,i);
		if(TVIS_BOLD&~st) SOM_MAIN_changed(1); return out;
	}
	if(tv==SOM_MAIN_tree.left.treeview) //illustrating
	{
		i = SOM_MAIN_produceitem(tv,i),tv = SOM_MAIN_right.treeview;
	}
	if(mb=='<-') //retracting
	if(st&TVIS_BOLD) mb = IDYES; 
	else return(MessageBeep(-1),0);
	else if(!mb||mb=='del'&&~GetKeyState(VK_CONTROL)>>15)
	{
		HTREEITEM gc = TreeView_GetChild(tv,i);
		if(gc&&SOM_MAIN_textarea(tv,gc)) gc = 0;
		if(mb=='del'&&GetKeyState(VK_SHIFT)>>15) //hollowing
		{
			if(!gc) //nothing to delete
			return !SOM_MAIN_report("MAIN1003",MB_OK);
			SOM_MAIN_before();
			if(TVIS_CUT&st&&out)
			{
				SOM_MAIN_moveupitem(tv,i);
				if(TVIS_BOLD&~st) SOM_MAIN_changed(1);
			}
			SOM_MAIN_delete_subitems(tv,i);
			SOM_MAIN_and_after();
			if(!out) goto collapse_for_effect;
			return out;
		}	
		if(!out) //cannot fully delete these items
		if(IDCANCEL==SOM_MAIN_report("MAIN1004",MB_OKCANCEL)) 
		return 0;
		mb = !gc?IDNO:SOM_MAIN_warning("MAIN1005",MB_YESNOCANCEL);		
		if(IDCANCEL==mb) return 0;
	}
	else if(mb=='del') mb = GetKeyState(VK_SHIFT)>>15?IDYES:IDNO;	
	if(TVIS_CUT&st&&mb!=IDYES)
	return !SOM_MAIN_report("MAIN1006",MB_OK);
	SOM_MAIN_before();	
	if(mb==IDYES) SOM_MAIN_delete_subitems(tv,i);
	if(TVIS_CUT&~st) SOM_MAIN_removeitem(param);
	SOM_MAIN_and_after();		
	SOM_MAIN_changed(TVIS_BOLD&st?-1:1); 
	if(!out) collapse_for_effect: 
	{
		POINT pt = {0,0};
		som_tool_toggle(tv,i,pt,TVE_COLLAPSE);
		if(SOM_MAIN_tree[param].left)
		{
			HWND vt = SOM_MAIN_tree.left.treeview;
			HTREEITEM ii = SOM_MAIN_reproduceitem(vt,tv,i);
			som_tool_toggle(vt,ii,pt,TVE_COLLAPSE);
		}
	}
	return out;
}
void SOM_MAIN_delete_subitems(HWND tv, HTREEITEM i)
{
	TVITEMEXW tvi = {TVIF_PARAM|TVIF_CHILDREN|TVIF_STATE,i=TreeView_GetChild(tv,i),0,~0};
	if(!TreeView_GetItem(tv,&tvi)||!tvi.cChildren) return; //textarea
	for(tvi.mask&=~TVIF_CHILDREN;i&&TreeView_GetItem(tv,&tvi);tvi.hItem=i)
	{
		i = TreeView_GetNextSibling(tv,tvi.hItem); 
		SOM_MAIN_delete_subitems(tv,tvi.hItem);	if(tvi.state&TVIS_CUT) continue;
		SOM_MAIN_removeitem(tvi.lParam); SOM_MAIN_changed(TVIS_BOLD&tvi.state?-1:+1);
	}
}
static void SOM_MAIN_escape(int r)
{		
	SOM_MAIN_onedge = 0;
	if(!SOM_MAIN_152()) return //initialize
	(void)SOM_MAIN_(152,som_tool=0,SOM_MAIN_152,r);	
		
	int i = 1, ii = 2;
	if(IsWindowVisible(SOM_MAIN_152())) std::swap(i,ii);							
	//ShowWindow(SOM_MAIN_modes[i],SW_HIDE);
	som_tool = SOM_MAIN_modes[ii];	

	HWND lr[2] = {SOM_MAIN_tree.left.treeview,SOM_MAIN_right.treeview};
	HWND lo[2] = {SOM_MAIN_tree.left.treeview,SOM_MAIN_right.treeview};

	bool doublewide = false; if(ii==2)
	{
		if(r==0) std::swap(lo[0],lo[1]);										   	
		RECT cr; GetClientRect(SOM_MAIN_152(),&cr);
		doublewide = SOM_MAIN_doublewide(cr.right,cr.bottom);
	} 
	bool sp[2] = {false,false}; for(int j=0;j<2;j++)
	{	
		if(sp[j]=j==r||doublewide||ii!=2) //swapping parentage		
		SetWindowRedraw(lr[j],0);		
	}	
		
	//before relies on TreeView_GetFirstVisible/visibility
	for(int j=0;j<2;j++) if(sp[j]) SOM_MAIN_before(lr[j]);

	int sw = //compact?
	IsWindowVisible(SOM_MAIN_modes[i]); 
	ShowWindow(SOM_MAIN_modes[i],0); 	
	EnableWindow(SOM_MAIN_modes[i],0); EnableWindow(som_tool,1);
	for(int j=0;j<2;j++) 
	{	
		HWND tv = lr[j]; //swapping styles in advance
		SOM_MAIN_cpp::layout &layout = SOM_MAIN_tree[lo[j]].layouts[ii-1];
		SetWindowLong(tv,GWL_STYLE,layout->ws);	SetWindowLong(tv,GWL_EXSTYLE,layout->wes);

		if(sp[j]) //swapping parentage		
		{	
			SetWindowRedraw(lr[j],1);
			SOM_MAIN_tree[tv].reparent(som_tool); RECT &lp = layout; 
			SetWindowPos(tv,layout->z,lp.left,lp.top,lp.right-lp.left,lp.bottom-lp.top,SWP_FRAMECHANGED);												
			SOM_MAIN_sizeup(tv,0,0,0);
			//winsanity: trick the scrollbar into showing up
			TVINSERTSTRUCTW wontbelong = {TVI_ROOT,TVI_LAST,{0}};
			TreeView_DeleteItem(lr[j],TreeView_InsertItem(lr[j],&wontbelong));								
		}		
	}				
	if(ii==2) SOM_MAIN_escaped = lr[r];
	ShowWindow(som_tool,sw); 

	//SOM_MAIN_before.focus = 0;
	for(int j=0;j<2;j++) if(sp[j]) SOM_MAIN_and_after(lr[j]); 
	//NOTE: EM_GETOLEINTERFACE can fail if the focus is wrong
	SetFocus(0); //hack: coming out focused yet also greyed??

	SetFocus(SOM_MAIN_escaped);	
}

const char *SOM_MAIN_void = "=\v"+1; 
const char *SOM_MAIN_exml_li = "=exml.li"+1;
static void SOM_MAIN_153_loopback(HWND,const wchar_t*,int);
inline bool SOM_MAIN_153_unlocked(int i, HWND lv=0)
{
	if(lv) i = ListView_GetItemState(lv,i,~0);
	return LVIS_CUT!=((LVIS_GLOW|LVIS_CUT)&i); //dicey
}
static struct SOM_MAIN_list //singleton
{	
	static SOM_MAIN_list &list;

	class aspects; //friendship

	struct aspect : std::string
	{
		const char *convention; 
		inline bool XML_like()const{ return *convention; }
		inline bool gettext_like()const{ return !*convention; }
		inline bool EXML_important()const
		{ return convention==list.aspects.exml.important; }
		mutable std::wstring description;
		aspect(const char *a, int b, const wchar_t *c=L"")
		:convention(a),std::string(1,(char)b),description(c){}
		aspect(const char *a, const std::string &b, const wchar_t *c=L"")
		:convention(a),std::string(b),description(c){}
		mutable HWND loopback153;
		inline int dialog()const{ return XML_like()?179:170+at(0); }						
		//select in the database query sense
		//highlight means copied like Ctrl+C
		mutable int id153, row153, column154; 
		mutable const char *selected, *highlighted;
		inline void inserted()const //constructor
		{
			id153 = row153 = column154 = 0;
			begin_editing(); selected = highlighted = 0;
		}
		inline bool selected_is_void()const
		{
			return selected&&selected[0]=='\v';
		}
		inline bool selected_is_bool()const
		{
			return selected&&selected[-1]!='=';
		}		
		void make_edit(const char *rval)const
		{
			making_edit(rval,'rtf');
			if(loopback153) SOM_MAIN_153_loopback
			(loopback153,rval?SOM_MAIN_utf8to16(rval):L"",dialog());
		}				
		void make_edit(const wchar_t *rval)const
		{
			making_edit(SOM_MAIN_utf16to8(rval));
			SOM_MAIN_153_loopback(loopback153,rval?rval:L"",dialog());
		}
		void make_edit(int rval_is_0)const
		{
			make_edit((char*)0); assert(!rval_is_0);
		}		
		void make_edit(HWND rval)const
		{
			return make_edit(SOM_MAIN_WindowText(rval));			
		}
		void fake_edit(HWND lb153)const //hack
		{
			char d = '\0'; EXML::Quote qt(d,selected); 
			if(loopback153=lb153) SOM_MAIN_153_loopback
			(lb153,qt?SOM_MAIN_utf8to16(qt):L"",dialog()); qt.unquote(d);
		}	
		void take_snapshot()const
		{
			snapshot = edit;
		}
		bool edited_versus_snapshot()const
		{
			return edited()&&edit!=snapshot;
		}
		inline bool edited()const{ return edit.size()!=1; }
		inline bool deleted()const{ return edit.size()==0; }
		inline bool assigned()const{ return edit.size()>=2; }

	private: friend class aspects;

		mutable std::string edit, snapshot; 
		void begin_editing()const
		{
			edit = "="; loopback153 = 0;
		}
		inline const char *selected_or_edit()const
		{
			return deleted()?0:
			assigned()||selected_is_void()?edit.c_str()+1:selected;
		}
		void making_edit(const char *rval, int rtf=0)const
		{
			list.aspects.unselect(); //void cache

			if(!rval) return edit.clear(); //delete
			edit = *rval?"=":" ";
			if(!*rval) return edit.push_back('\0'); //boolean	
			else assert(*rval!='\v');
			int compile[SOM_MAIN_poor]; 
			if(at(0)==2&&!rtf) SOM_MAIN_plaintext(edit,rval);
			else if(XML_like()&&strpbrk(rval," \r\n\t\'>")) 
			{
				edit+='\'';	edit+=rval;
				for(size_t i=2;npos!=(i=edit.find('\'',i));i+=6) 
				edit.replace(i,1,"&apos;");	edit+='\''; 
			}
			else edit+=rval; 
			if((at(0)==1||at(0)==3)&&edit.end()[-1]!='\4') 
			edit.push_back('\4');			
		}
	};								    
	struct aspectset : std::set<aspect>
	{
		using std::set<aspect>::find;
		inline iterator find(const char *b){ return find(aspect(0,b,L"")); }
		iterator insert_aspect(const char *xml, const char *tag, const char *key)
		{
			//todo: EX::INI::Editor()->aspect_(xml,tag,key);
			std::pair<iterator,bool> out = insert(aspect(tag,key));			
			out.first->description = EX::need_unicode_equivalent(65001,key);
			if(!out.second) assert(0); //there can be only one
			else out.first->inserted(); return out.first;
		}
		void print_selected(std::string &buf)const
		{
			const_iterator it = begin(); int uno = 0; EXML::Quoted qt;						
			for(;it!=end();it++) if(it->selected&&*it->selected!='\v') 
			{	
				if(!uno++){	buf+='<'; buf+=it->convention; } 
				buf+=' '; buf+=*it; if(it->selected_is_bool()) continue;
				buf+='='; qt = it->selected; buf.append(qt,qt.length());
			}
			if(uno) buf+='>'; assert(!uno||begin()->XML_like()); 
		}
	};
	struct aspects : std::map<std::string,aspectset> //singleton
	{	
		class foreach : std::pair<iterator,aspectset::iterator>
		{
			inline iterator end(){ return list.aspects.end(); }
			inline iterator begin(){ return list.aspects.begin(); }
			public:	inline void operator++(int) 
			{
				if(++second!=first->second.end()) return;				
				while(++first!=end()&&first->second.empty());
				if(operator bool()) second = first->second.begin();
			}			
			inline operator bool(){ return first!=end(); }
			foreach():pair(begin(),begin()->second.begin()){}			
			inline const aspect &operator*(){ return *second; }
			inline const aspect *operator->(){ return &*second; }
		};

		struct exml : std::vector<const aspect*> 
		{
			char important[5],__; 
			const aspect &li,&l,&lh,&alt,&c,&tc,&tm,&cc,&hl,&il; 						
			const aspect &_exml(aspectset &set, const char *x, const wchar_t *y)
			{ push_back(&*set.insert(aspect(important,x,y)).first); return *back(); }
			exml(aspectset &set) 
			#define _(x) x(_exml(set,#x,som_932w_EXML::x)),
			:_(li)_(l)_(lh)_(alt)_(c)_(tc)_(tm)_(cc)_(hl)_(il)__(0)
			{ strcpy(important,"exml"); push_back(0); }exml()
			#define _(x) x(x), //truly annoying optimization			 
			:_(li)_(l)_(lh)_(alt)_(c)_(tc)_(tm)_(cc)_(hl)_(il)__(0){}
			#undef _

		}exml; //easy access//
		const aspect &msgid,&msgctxt,&msgstr_body,&msgctxt_short_code;
		const aspect &_gettext(size_t i)
		{ return *operator[]("").insert(aspect("",i,som_932w_Message[i])).first; }
		aspects(int):
		msgid(_gettext(0)),
		msgctxt(_gettext(1)),
		msgstr_body(_gettext(2)),
		msgctxt_short_code(_gettext(3)),exml(operator[]("exml"))
		{
			//reminder: SOM_MAIN_list::list isn't yet intialized
			edited = selected = highlighted = 0; 
		}		
		aspects():msgid(msgid),msgctxt(msgid),msgstr_body(msgid),msgctxt_short_code(msgid)
		{
			if('main'==SOM::image()) new(this)aspects(1); //optimizing
		}
		static std::string &token(const char *p)
		{
			static std::string out; out.clear();
			while(*p&&!isspace(*p)&&*p!='='&&*p!='/'&&*p!='>') out.push_back(*p++);
			return out;
		}		

		LPARAM edited;
		LPARAM selected; 				
		LPARAM highlighted; 
		inline void unselect()
		{
			selected = 0; //invalidate
		}
		inline void reselect(LPARAM param)
		{
			if(selected!=param) select(param);
		}
		inline void edit(LPARAM param)
		{
			edited = param;
			if(edited==selected) unselect();
			for(foreach ea;ea;ea++)	ea->begin_editing();
		}
		inline void highlight(LPARAM param)
		{
			if(!SOM_MAIN_clip.po)
			if(param==SOM_MAIN_cliparam()) 
			param = 0; else assert(!param); //msgid

			highlighted = param;			
			select(param,&aspect::highlighted,0);			

			if(msgctxt.highlighted)
			msgctxt_short_code.highlighted = msgctxt.highlighted;
		}
		void select(LPARAM param, std::vector<LPARAM> *cols=0)
		{
			selected = param;
			select(param,&aspect::selected,cols);
			if(!param) return;
			if(!exml.li.selected) 
			if(!SOM_MAIN_tree[param].unlisted())
			exml.li.selected = SOM_MAIN_exml_li; //placeholder
			if(edited==param) 
			for(foreach ea;ea;ea++)	ea->selected = ea->selected_or_edit();
		}
		void select(LPARAM param, const char *aspect::*member, std::vector<LPARAM> *cols)
		{		
			//(aspect&)
			//The MSVC2005 implementation of std::set doesn't
			//return const elements as it should. furthermore
			//its compiler ignores mutable pointer-to-members

			iterator it; aspectset::iterator jt; 
			if(!cols) for(it=begin();it!=end();it++)
			for(jt=it->second.begin();jt!=it->second.end();jt++)				
			(aspect&)(*jt).*member = 0; 

			if(!param) return;
			SOM_MAIN_tree::string &msg = SOM_MAIN_tree[param];		

			EXML::Tags tag;
			EXML::Attribuf<8> attribs;
			bool bi = msg.bilingual();
			int pass = bi?1:2; if(pass==1)
			{
				const char *jstr = 0; size_t jstr_s = 
				//mo::gettext(SOM_MAIN_tree.nihongo.mo,msg.id,jstr);				
				msg.bilingual_gettext(jstr); //optimizing
				if(jstr_s) attribs.poscharwindow(jstr,jstr_s);
				else assert(0);
			}
			else pass2: attribs.poscharwindow(msg.str,msg.str_s); 			
			while(tag=EXML::tag(attribs))
			{	
				it = insert(std::make_pair
				(token(attribs->keyname),aspectset())).first;
				for(int i=1;attribs[i];i++) 
				{	
					const char *key;
					switch(attribs[i].key) //quirks?
					{
					case EXML::Attribs::l: key = "l"; break;					
					case EXML::Attribs::li: key = "li"; break;
					default: key = token(attribs[i].keyname).c_str();
					}
					jt = it->second.find(key); 
					if(jt==it->second.end()) //will generate a description 
					if(tag==EXML::Tags::exml||strstr(attribs->val,"<exml")) 					
					jt = it->second.insert_aspect("xml",it->first.c_str(),key);
					else break; //likely a legacy/unmarked message catalog
					(aspect&)(*jt).*member = attribs[i].val;
					if(cols&&!jt->column154&&&*jt!=&exml.li) //column 0	
					if(jt->column154=cols->size()) 
					cols->push_back(list.aspect_param(*jt));
					else assert(0);
				}
				//hack: stop at the exml tag's text/body
				//note: also notes the prescence of EXML
				if(tag==EXML::Tags::exml&&!attribs->tag)
				{
					EXML::tag(attribs); break;
				}
			}
			if(2==++pass) goto pass2;
			assert(msg.id);
			const char *_4 = strchr(msg.id,'\4');
			(aspect&)msgid.*member = _4?_4+1:msg.id;
			if((unsigned char&)*(msgid.*member)<'\4') 
			(aspect&)msgid.*member = 0; //SOM_MAIN_create
			(aspect&)msgctxt.*member = !msg.ctxt_s&&_4?msg.id:0;
			const char *body = tag==EXML::Tags::exml?attribs->val:msg.str;
			if(msg.ctxt_s||!SOM_MAIN_tree.nihongo.mo.claim(body))			
			(aspect&)msgstr_body.*member = body&&*body?body:0;
			(aspect&)msgctxt_short_code.*member = msg.ctxt_s>1?msg.id:0;			 
		}		
		void record(SOM_MAIN_tree::string &rec, std::string &buf)
		{
			//TODO: probably should use SOM_MAIN_copy//
			//so the shared memory file will be alike//
			//Reminder: SOM_MAIN_save_as/copy_machine//

			new(&rec)SOM_MAIN_tree::string; size_t id = buf.size(); 

			const char *_4 = 0, *ctxt = msgctxt.selected; 
			if(!ctxt) ctxt = msgctxt_short_code.selected;
			if(ctxt&&*ctxt!='\v') //append crashes if nul
			{_4 = strchr(ctxt,'\4');
			rec.ctxt_s = _4?_4-ctxt+1:0; buf.append(ctxt,rec.ctxt_s);}
			if(!SOM_MAIN_7bitshortcode(ctxt,rec.ctxt_s)) 
			rec.ctxt_s = 0;
			if(msgid.selected&&*msgid.selected!='\v') 
			buf+=msgid.selected; 
			rec.id_s = buf.size()-id; buf+='\0'; size_t str = buf.size();
			aspectset &exml = operator[]("exml");
			for(iterator it=++begin();it!=end();it++)
			if(&exml!=&it->second) it->second.print_selected(buf);
			exml.print_selected(buf);
			if(msgstr_body.selected&&*msgstr_body.selected!='\v')
			buf+=msgstr_body.selected;
			rec.str_s = buf.size()-str;
			rec.id = buf.c_str()+id; rec.str = buf.c_str()+str; 
		}

		//dictionary
		template<int N> void fill(char (&with)[N])
		{   
			for(int i=0,j=0,l=0;i<N;i++) switch(with[i])
			{		
			case '\0': i = N-1; /*case '\r':*/ case '\n': 
				
				if(with[j]=='<') //leveraging XML parser
				{
					EXML::Attribuf<16> attribs(with+j,N-j); 					
					while(EXML::tag(attribs)) if(attribs[1])
					{
						iterator it = insert
						(std::make_pair(token(attribs->keyname),aspectset())).first;
						for(int i=1;attribs[i];i++) it->
						second.insert(aspect(it->first.c_str(),token(attribs[i].keyname))).first
						->description = EX::need_unicode_equivalent(65001,EXML::Token(attribs[i]));						
					}
					if(!~*attribs) return;
					for(i=attribs-with;i<N&&with[i]!='\n';i++); 
				}
				else //assuming unnammed (gettext-likes)
				{
					with[i] = '\0'; 
					if(i&&with[i-1]=='\r') with[i-1] = '\0'; 
					begin()->second.insert(aspect("",l++)).first
					->description = EX::need_unicode_equivalent(65001,with+j);
				}
				j = i+1;
			}
		}
		__declspec(noinline) //chkstk workaround
		void fill() 
		{	
			enum{x_s=4096}; HWND language =	
			GetDlgItem(SOM_MAIN_151(),1000); if(language) 
			{
				wchar_t x[x_s] = L""; char u[4*x_s] = "";
				if(GetWindowTextW(language,x,x_s)>=x_s-1) assert(0);			
				EX::need_ansi_equivalent(65001,x,u,sizeof(u));
				fill(u); DestroyWindow(language);
			}
		}	 		

	}aspects; //singleton
	
	inline aspect &operator[](LPARAM l){ return *(aspect*)l; }

	inline LPARAM aspect_param(const aspect &a){ return (LPARAM)&a; }  	

	struct format //dictionary
	{
		format(int i=CB_ERR):item(i){}
		std::wstring resource, itemlabel; int item;
	};
	struct formats : std::vector<format> //singleton
	{
		using std::vector<format>::operator[];

		std::map<std::string,size_t> byaspect;

		format &operator[](const aspect &a)
		{
			char key[30]; sprintf_s(key,"<%s %s>",a.convention,a.c_str());
			std::map<std::string,size_t>::iterator it = byaspect.find(key);
			if(it!=byaspect.end()) return at(it->second);
			static format cberr; return cberr;
		}
		template<int N> void fill(char (&with)[N])
		{   
			for(int i=0,j=0,l=0;i<N-1;i++) switch(with[i])
			{		
			case '\0': i = N-1; case '\n': with[i] = '\0'; 
				
				if(with[j]=='<') //leveraging XML parser
				{
					push_back(size()); 
					format &f = back(); 
					iterator set; const char *key = 0;
					EXML::Attribuf<16> attribs(with+j,N-j); 					
					while(EXML::tag(attribs)) if(attribs[1])
					{	
						char *key = (char*)attribs->keyname-1;
						char *cat = key+1; while(*cat&&!iswspace(*cat)) cat++;
						*cat++ = ' ';	
						for(int i=1;attribs[i];i++)
						if(attribs[i].val==attribs[i].keyname) //paranoia
						{
							char *val = (char*)attribs[i].val, *len = val; 
							while(*len&&!iswspace(*len)&&*len!='>') len++;
							if(len-val) //delim: only matters if the first
							{
								char delim = len[1]; 
								strcpy(len,">"); memcpy(cat,val,len-val+2); 
								if(f.resource.empty())
								{
									f.resource = EX::need_unicode_equivalent(65001,key);

									//REMOVE ME??
									//hack: FindResource requires ALL CAPS
									for(size_t i=0;i<f.resource.size();i++)
									{
										f.resource[i] = towupper(f.resource[i]);
									}
								}
								byaspect[key] = size()-1;
								len[1] = delim;
							}
							else assert(0);
						}
						else assert(0);
					}

					if(!f.resource.empty()&&*attribs->keyname=='>') 
					f.itemlabel = EX::need_unicode_equivalent(65001,attribs->val);
					else pop_back();
				}
				j = i+1;
			}
		}
		__declspec(noinline) //chkstk workaround
		void fill(HWND with) 
		{	
			HWND cb = GetDlgItem(with,1013);
			HWND language = GetWindow(cb,GW_HWNDNEXT);
			static bool one_off = false; if(!one_off++) //???
			{
				enum{ x_s=4096 };
				wchar_t x[x_s] = L""; char u[4*x_s] = "";
				if(GetWindowTextW(language,x,x_s)>=x_s-1) assert(0);			
				EX::need_ansi_equivalent(65001,x,u,sizeof(u));
				fill(u);
			}
			DestroyWindow(language);
		}	 

	}formats; //singleton
		
	struct cell //like pool
	{	
		//REMOVE ME?
		cell(){ locked = false; } //154 

		//WYSIWYG caching mechanism
		const wchar_t *quoted; bool locked; 
		const wchar_t *quote(int i, int j=0)
		{
			NMLVGETINFOTIPW lvit = 
			{{listview,0,LVN_GETINFOTIPW},0,0,0,i,j,0};
			lvit.hdr.idFrom = GetWindowID(listview);
			assert(this==&list.item); //making
			static std::vector<wchar_t> out(260); //static
			for(out.resize(out.capacity());;out.resize(1.5*lvit.cchTextMax))
			{
				lvit.pszText = &out[0];	lvit.cchTextMax = out.size();
				*lvit.pszText = '?'; //hack: simulate an ellipsis popup?
				SendMessageW(GetParent(listview),WM_NOTIFY,lvit.hdr.idFrom,(LPARAM)&lvit);
				if(lvit.cchTextMax) //hack: 0 signals no msgctxt (short code)
				{
					int len = wcslen(lvit.pszText); if(lvit.cchTextMax<=len-1) continue;
					lvit.pszText[len] = '\0';
				}
				else out.resize(1); break;
			}
			if(this==&list.item)
			locked = !SOM_MAIN_153_unlocked(i,listview);
			quoted_i = i; //for redraw() below
			return quoted = &out[0]; 
		}
		int quoted_i; inline void redraw(int i=-1)
		{
			if(i==-1) i = quoted_i; ListView_RedrawItems(listview,i,i);
		}						
		void redraw_affected_rows(HTREEITEM i) //154
		{
			LVFINDINFOW lfi = {LVFI_PARAM,0}; 
			do{lfi.lParam = (LPARAM)i; int j = //phantom business
			ListView_FindItem(listview,-1,&lfi); if(j!=-1) redraw(j);
			if(TreeView_GetPrevSibling(SOM_MAIN_right.treeview,i)) return;
			else i = TreeView_GetParent(SOM_MAIN_right.treeview,i);
			}while(i&&TVIS_CUT&TreeView_GetItemState(SOM_MAIN_right.treeview,i,~0));
		}

		void clear() //order is important here
		{
			entrypoint = bookmark = 0; 
			entrylevel = entrylimit = sortedby = 0;
			ListView_DeleteAllItems(listview); 
		}
		inline bool empty(){ return !entrypoint; }
		
		HTREEITEM bookmark;
		HTREEITEM entrypoint; int entrylevel, entrylimit;		

		HWND sidebyside;
		HWND corresponding_treeview()
		{
			if(this==&list.item)
			{
				assert(0); return 0; //corresponding to 154 params
			}
			if(sidebyside) return sidebyside;
			bool r = this==&list.right||SOM_MAIN_same();
			return (r?SOM_MAIN_right:SOM_MAIN_tree.left).treeview;
		}
		HWND underlying_treeview()
		{
			if(this==&list.item) 
			return SOM_MAIN_tree->treeview;
			if(!SOM_MAIN_same()) return corresponding_treeview();
			assert(!sidebyside);
			if(this==&list.left) 
			return SOM_MAIN_tree.left.treeview;
			return SOM_MAIN_right.treeview;			
		}
		
		std::vector<LPARAM> columns154;	
		int sortedby; void sort(int by=-1) //154
		{
			if(by!=-1) sortedby = by; HWND lv = listview;
			NMLISTVIEW nm = {{lv,GetWindowID(lv),LVN_COLUMNCLICK},-1,sortedby};
			SendMessage(GetParent(lv),WM_NOTIFY,nm.hdr.idFrom,(LPARAM)&nm);		
		}			
		void swapped(HTREEITEM i, HTREEITEM ii)
		{
			if(sortedby==0) return sort(0);	
			redraw_affected_rows(i); redraw_affected_rows(ii); 
		}
		void introduced(int lv, HTREEITEM i, int cut, bool final)
		{
			if(lv<entrylevel||lv>entrylimit||!i) return;
			if(this==&list.left&&!SOM_MAIN_same()) return;
			LVITEMW lvi = {LVIF_PARAM|LVIF_TEXT|LVIF_STATE,0,0,cut,LVIS_CUT};
			lvi.pszText = LPSTR_TEXTCALLBACKW; lvi.lParam = (LPARAM)i;
			ListView_InsertItem(listview,&lvi);	if(final||lv==entrylimit) sort();
		}
		inline void removed(HTREEITEM i) //cutting
		{
			if(!empty()) removed(SOM_MAIN_level(SOM_MAIN_right.treeview,i),i,TVIS_CUT);
		}	
		void removed(int lv, HTREEITEM i, int cut=0)
		{
			if(lv<entrylevel||lv>entrylimit||!i) return;			
			if(this==&list.left&&!SOM_MAIN_same()) return;
			LVFINDINFOW lfi = {LVFI_PARAM,0,(LPARAM)i};
			int j = ListView_FindItem(listview,-1,&lfi);
			if(j!=-1&&cut){ListView_SetItemState(listview,j,~0,LVIS_CUT);}
			else if(j!=-1) ListView_DeleteItem(listview,j);
		}		

		SOM_MAIN_cpp::layout layout;

		HWND listview; inline cell *operator->() 
		{
			return &(&list.left)[this==&list.left];
		}

	}item, left, right, *focus;
	SOM_MAIN_list(){ focus = &item; }
	inline cell *operator->(){ return focus; }
	inline cell &operator*(){ return *focus; }
	inline HWND operator=(HWND lv)
	{
		if(lv==focus->listview) return lv; //debugging

		if(lv!=item.listview)
		{
			focus = &operator[](lv); 
			SOM_MAIN_tree = focus->underlying_treeview();
		}
		else focus = &item;	return lv;
	}
	inline cell &operator[](HWND lv)
	{
		return lv==right.listview?right:left;	 
	}	

}SOM_MAIN_list; //singleton

struct SOM_MAIN_list &SOM_MAIN_list::list = ::SOM_MAIN_list;

#undef SOM_MAIN_RTLPOS
#define SOM_MAIN_RTLPOS \
HWND l = SOM_MAIN_list.left.listview;\
HWND r = SOM_MAIN_list.right.listview;\
if(SOM_MAIN_rtl!=&SOM_MAIN_right) std::swap(l,r);\
RECT &lpos = SOM_MAIN_list.left.layout;\
RECT &rpos = SOM_MAIN_list.right.layout;\
//if(SOM_MAIN_leftonly()) std::swap(l,r);
				
static LPARAM SOM_MAIN_153_listparam(HWND lv, int i)
{
	LVITEMW lvi = {LVIF_PARAM,i,0};
	return ListView_GetItem(lv,&lvi)?lvi.lParam:0;
}
inline HTREEITEM SOM_MAIN_154_treeitem(HWND lv, int i=-1)
{
	if(i==-1) i = ListView_GetNextItem(lv,-1,LVNI_FOCUSED);
	return (HTREEITEM)SOM_MAIN_153_listparam(lv,i);
}
static INT_PTR CALLBACK SOM_MAIN_insertproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{		
	if(uMsg==WM_INITDIALOG) //use insert button
	{
		HWND create = GetDlgItem(hwndDlg,1211);
		HWND insert = GetDlgItem(hwndDlg,1212);
		if(!insert)
		{
			insert = create; create = 0;
			SetWindowLong(insert,GWL_ID,1212);
			SetWindowTextW(insert,som_932w_Insert);
		}
		if(!IsWindowVisible(insert))
		{
			RECT rc = {0,0,0,0}; 
			GetWindowRect(create,&rc); MapWindowRect(0,hwndDlg,&rc); 
			SetWindowPos(insert,create,rc.left,rc.top,0,0,SWP_NOSIZE);
			ShowWindow(insert,1);			
			DestroyWindow(create);
		}
		else EnableWindow(create,0);
		windowsex_enable<1200>(hwndDlg,0); //checkbox
		SetWindowLongPtrW(hwndDlg,DWLP_DLGPROC,(LONG_PTR)SOM_MAIN_153);
		return SOM_MAIN_153(hwndDlg,uMsg,wParam,lParam);
	}
	return 0;	
}
static void SOM_MAIN_insertafter(HWND dlg153, bool app)
{
	if(app) SOM_MAIN_outline_item()
	+=L"-1"+SOM_MAIN_outline_item().empty();
	int row = SOM_MAIN_list.aspects.exml.li.row153;
	SendDlgItemMessage(dlg153,1022,LVM_REDRAWITEMS,row,row);
	SendDlgItemMessage(dlg153,1023,IPM_SETADDRESS,0,SOM_MAIN_outline_item_octets());
}
		
inline FILETIME SOM_MAIN_kbQ167296(time_t t)
{	
	int compile[sizeof(t)==sizeof(FILETIME)];
	return reinterpret_cast<FILETIME&>(t=t*10000000+116444736000000000);
}
inline time_t SOM_MAIN_kbQ167296(FILETIME ft)
{	
	return (reinterpret_cast<time_t&>(ft)-116444736000000000)/10000000;
}
static double SOM_MAIN_clockdifference(const time_t &ref)
{
	struct tm utc,lt;
	//LocalFileTimeToFileTime says DST is off
	if(gmtime_s(&utc,&ref)||localtime_s(&lt,&ref)) return 0;
	//0: clockdifference (not timedifference)
	int isdst = lt.tm_isdst; lt.tm_isdst = 0;
	return difftime(mktime(&utc),mktime(&lt));
}								
static time_t SOM_MAIN_universaltime(SYSTEMTIME *st)
{
	FILETIME ft; 
	if(!SystemTimeToFileTime(st,&ft)) return 0; //1970?	
	time_t out = SOM_MAIN_kbQ167296(ft);
	return out+SOM_MAIN_clockdifference(out);
}
static bool SOM_MAIN_windowstime(SYSTEMTIME *dst, time_t t) 
{	
	t-=SOM_MAIN_clockdifference(t);
	if(t<0||t>32503679999i64) return false; //crashes
	FILETIME ft = SOM_MAIN_kbQ167296(t);
	return FileTimeToSystemTime(&ft,dst);
}
static bool SOM_MAIN_windowstime(SYSTEMTIME *dst, const wchar_t *p)
{
	if(!*p) return false;
	char a[12]; int i; for(i=0;i<11&&*p;a[i++]=*p++)
	if(*p>='0'&&*p<='9'||*p>='a'&&*p<='z'||*p>='A'||*p<='Z') continue;
	else if(*p!='@'&&*p!='$') return false; if(*p) return false;
	while(i<11) a[i++] = 'A'; a[11] = '=';
	time_t time = 0; int time_s = sizeof(time);
	if(!som_atlenc_Base64Decode(a,12,(BYTE*)&time,&time_s)) return false;
	return SOM_MAIN_windowstime(dst,time); 
}
static const char *SOM_MAIN_timencode(time_t t)
{	
	static char out64[11+1]; int len64 = 12;	
	som_atlenc_Base64Encode((BYTE*)&t,sizeof(t),out64,&len64);
	out64[11] = '\0'; assert(len64==11);
	for(int i=10;i&&out64[i]=='A';i--) out64[i] = '\0'; return out64;
}
static const char *SOM_MAIN_timencode(SYSTEMTIME *dst)
{
	return SOM_MAIN_timencode(SOM_MAIN_universaltime(dst));
}

//2000-2999/12/31/a second until midnight
static SYSTEMTIME SOM_MAIN_millennium[2]; 
static bool SOM_MAIN_millennium_initialized = 
SOM_MAIN_windowstime(SOM_MAIN_millennium+0,L"gENtO")&&
SOM_MAIN_windowstime(SOM_MAIN_millennium+1,L"$8tekQc");

static HWND SOM_MAIN_loopingback = 0;
static void SOM_MAIN_153_loopback(HWND lb, const wchar_t *rv, int dialog17X)
{
	if(rv==SOM_MAIN_list.item.quoted) return; //WYSIWYG? 
	//173 must come before SOM_MAIN_utf8to16 (WM_SETTEXT)
	if(dialog17X==173) //short code combobox (not pretty)
	{
		HWND cb = GetDlgItem(lb,1013); if(!cb) return;
		LPARAM lp = SOM_MAIN_outline_param(SOM_MAIN_right.treeview,rv);
		ComboBox_ResetContent(cb); ComboBox_SetItemData(cb,ComboBox_SetCurSel(cb,
		ComboBox_AddString(cb,SOM_MAIN_label(SOM_MAIN_right.treeview,SOM_MAIN_tree[lp].right))),lp);
	}
	if(SOM_MAIN_loopingback=lb) //!
	{
		switch(dialog17X)
		{
		case 179:
		{
			const wchar_t *nice = wcschr(rv,'&'); if(nice) 
			SOM_MAIN_WideCharEntities((wchar_t*)rv,nice-rv+wcslen(nice)+1);
			break;
		}
		case 172: int compile[SOM_MAIN_poor]; //todo: RTF?
		}
		if(DLGC_HASSETSEL&SendMessage(lb,WM_GETDLGCODE,0,0))
		{
			Edit_SetSel(lb,0,-1);
			SendMessage(lb,EM_REPLACESEL,1,(LPARAM)rv);
		}
		else SetWindowTextW(lb,rv);
	}
	SOM_MAIN_loopingback = 0; //!
}
static HWND SOM_MAIN_153_richtext() 
{
	static HWND out = som_tool_richinit //SOM_MAIN_modes[0]: XP crashes on exit if 0
	(CreateWindowExW(0,som_tool_richedit_class(),0,ES_MULTILINE,0,0,0,0,SOM_MAIN_modes[0],0,0,0),0);
	return out; 
}
static int CALLBACK SOM_MAIN_sort153cb(LPARAM lParam1, LPARAM lParam2, LPARAM)
{
	return SOM_MAIN_list[lParam1].id153-SOM_MAIN_list[lParam2].id153;
}
static void SOM_MAIN_enumerate153(HWND lv, int from)
{
	LVITEMW lvi = {LVIF_PARAM,ListView_GetItemCount(lv),0}; 
	while(--lvi.iItem>=from)
	if(ListView_GetItem(lv,&lvi)) SOM_MAIN_list[lvi.lParam].row153 = lvi.iItem;
}
static VOID CALLBACK SOM_MAIN_sort153(HWND win) //was timer
{
	ListView_SortItems(win,SOM_MAIN_sort153cb,0); SOM_MAIN_enumerate153(win,0);	
}
static HWND SOM_MAIN_153_1009_clientext = 0;
static const wchar_t *SOM_MAIN_153_1009_text = 0;
static VOID CALLBACK SOM_MAIN_153_1009_pastespecial(HWND win, UINT, UINT_PTR id, DWORD)
{
	KillTimer(win,id);
	if(SOM::MO::view&&SOM_MAIN_153_1009_clientext)
	if(IsWindow(SOM::MO::view->pastespecial_2ED_clipboard))
	SendMessage(SOM_MAIN_153_1009_clientext,EM_PASTESPECIAL,0x2ED,0);
}
static VOID CALLBACK SOM_MAIN_153_1009(HWND win, UINT=0, UINT_PTR id=0, DWORD=0)
{
	KillTimer(win,id); SendMessage(win,WM_COMMAND,1009,0); //slight improvement
}

static int SOM_MAIN_153_sanstext;
static void SOM_MAIN_153_paste();
static void SOM_MAIN_153_drawclipboard(LPARAM);
static void SOM_MAIN_153_WhatYouSeeIsWhatYouGet(int=0);
static bool SOM_MAIN_153_autoascribe(HWND,const void*);
static bool SOM_MAIN_copytoclipboard(LPARAM,HWND lv153);
static HTREEITEM SOM_MAIN_reproduceitem(HWND,HWND,HTREEITEM);
static HTREEITEM SOM_MAIN_introduceitem(LPARAM,HWND,HTREEITEM,bool,HTREEITEM=0,int=0);
static INT_PTR CALLBACK SOM_MAIN_153b(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{		
	//SOM_MAIN_153_sanstext
	if(uMsg==WM_INITDIALOG) //make 153 suitable for outlines 
	{
		SetWindowLongPtrW(hwndDlg,DWLP_DLGPROC,(LONG_PTR)SOM_MAIN_153);

		INT_PTR out = SOM_MAIN_153(hwndDlg,uMsg,wParam,lParam);

		SOM_MAIN_153_sanstext = -1;	
		HWND lv = GetDlgItem(hwndDlg,1022);		
		int n = //pull out gettext aspects/<exml li>
		SOM_MAIN_list.aspects.msgctxt_short_code.row153;
		for(int i=0;i<=n;i++) ListView_DeleteItem(lv,0); 
		if(SOM_MAIN_list.aspects.exml.li.selected) 
		ListView_DeleteItem(lv,0); SOM_MAIN_enumerate153(lv,0);
		SendDlgItemMessage(hwndDlg,1011,CB_SETCURSEL,5,0);
		SendDlgItemMessage(hwndDlg,1023,IPM_CLEARADDRESS,0,0);
		int dis[] = {1013,1023,1200,1211,1212,1082,1083,1084,1085};
		windowsex_enable(hwndDlg,dis,0);				
		HWND tv = SOM_MAIN_tree->treeview;
		SetWindowTextW(hwndDlg,SOM_MAIN_label(tv,SOM_MAIN_tree[lParam][tv]));
		return out;
	}
	return 0;	
}
static LPARAM SOM_MAIN_17X_continue(HWND,WPARAM,LPARAM);
static INT_PTR CALLBACK SOM_MAIN_17X(HWND,UINT,WPARAM,LPARAM);
static INT_PTR CALLBACK SOM_MAIN_180(HWND,UINT,WPARAM,LPARAM);
static INT_PTR CALLBACK SOM_MAIN_153(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{		
	//these values don't matter
	static HWND tv = 0, lv = 0;		
	static HTREEITEM sel = 0; static LPARAM param = 0;	
	static bool readonly = true, inserting = false, creating = false;

	static int drophilited; static HWND refocus; 
	
	static struct //const void *autoascribing = 0;
	{
		const SOM_MAIN_list::aspect *aspect;

		void operator=(int)
		{
			if(!aspect||!on) return; 		
			if(!aspect->loopback153) assert(0); else
			SOM_MAIN_153_autoascribe(aspect->loopback153,aspect);	
			aspect = 0;
		}
		void operator=(const SOM_MAIN_list::aspect &a)
		{
			if(!on||&a==aspect) return;	
			operator=(0); aspect = &a; assert(a.loopback153);
			//appear responsive
			if(!SOM_MAIN_tree[SOM_MAIN_list.aspects.edited].temporary())			
			windowsex_enable<12321>(on); 
		}		
		HWND on; void off(){ on = 0; aspect = 0; }
		
	}autoascribing = {0,0};

	switch(uMsg)
	{
	case WM_INITDIALOG: 
	{	
		autoascribing.off();

		HWND gp = GetParent(hwndDlg);
		windowsex_enable<12321>(hwndDlg,0);
						
		if(wParam) //initializing
		{
			SOM_MAIN_153_sanstext = 0;

			som_tool_recenter(hwndDlg,gp,SOM_MAIN_onedge);

			param = lParam;
			tv = SOM_MAIN_tree->treeview;
			SOM_MAIN_list.aspects.fill(); //chkstk workaround
			
			refocus = SOM_MAIN_list->listview;
			SOM_MAIN_list = lv = 
			SOM_MAIN_list.item.listview = GetDlgItem(hwndDlg,1022);					

			if(gp==som_tool&&!SOM_MAIN_153_1009_text) 
			{
				sel = TreeView_GetSelection(tv); //phantom business
				if(SOM_MAIN_textarea(tv,sel)) sel = SOM_MAIN_tree[param][tv];
			}
			else if(!SOM_MAIN_tree[param].temporary())
			{
				sel = SOM_MAIN_154_treeitem(refocus);
				tv = SOM_MAIN_list[refocus].corresponding_treeview();
			}
			else if(sel=SOM_MAIN_tree[param].right)
			{
				TreeView_SelectItem(tv,sel);
			}
			else assert(0);

			readonly = SOM_MAIN_readonly(tv);

			SOM_MAIN_clipboard = hwndDlg; 
			SendMessage(hwndDlg,SOM_MAIN_cpp::wm_drawclipboard,0,0);			
		}
		else //moving with dpad or recording
		{
			sel = TreeView_GetSelection(tv);
			assert(!SOM_MAIN_textarea(tv,sel));
			param = SOM_MAIN_param(tv,sel);
		}	

		SOM_MAIN_tree::pool &pool = SOM_MAIN_tree[tv];		
		SOM_MAIN_tree::string &msg = SOM_MAIN_tree[param];			
		
		if(!readonly)
		SOM_MAIN_list.aspects.edit(param);
		if(SOM_MAIN_153_1009_text) //experimental		
		SOM_MAIN_list.aspects.msgstr_body.make_edit(SOM_MAIN_153_1009_text);
		SOM_MAIN_list.aspects.select(param);
		//set caption (0: it doesn't matter)
		SendMessage(hwndDlg,WM_SETTEXT,0,0);
		SOM_MAIN_list.aspects.msgid.loopback153 = 
		SOM_MAIN_list.aspects.msgctxt.loopback153 = 
		SOM_MAIN_list.aspects.msgctxt_short_code.loopback153 = hwndDlg;
		SOM_MAIN_outline_item(tv,msg[tv]);
		HWND ip = GetDlgItem(hwndDlg,1023); //fake_edit sets loopback153
		SendMessage(ip,IPM_SETADDRESS,0,SOM_MAIN_outline_item_octets());	
		SOM_MAIN_list.aspects.exml.li.loopback153 = GetWindow(ip,GW_CHILD);
		SOM_MAIN_list.aspects.exml.l.fake_edit(GetDlgItem(hwndDlg,1000));
		SOM_MAIN_list.aspects.exml.alt.fake_edit(GetDlgItem(hwndDlg,1001));
		for(int i=1024;i<=1026;i++)
		SendDlgItemMessage(hwndDlg,i,DTM_SETRANGE,3,(LPARAM)SOM_MAIN_millennium);
		SOM_MAIN_list.aspects.exml.tc.fake_edit(GetDlgItem(hwndDlg,1024));
		SOM_MAIN_list.aspects.exml.tm.fake_edit(GetDlgItem(hwndDlg,1026));				
				
		creating = msg.temporary();
		HWND create = GetDlgItem(hwndDlg,1211);		
		if(wParam) //can be disabled if japanese
		inserting = !create||!IsWindowEnabled(create);	
		HWND rt = SOM_MAIN_153_richtext(); if(!msg.undo) 
		{
			SETTEXTEX st = {0,CP_UTF8};	   						
			SOM_MAIN_fillrichtext(rt,SOM_MAIN_list.aspects.msgstr_body.selected,msg.str);						
		}
		else SOM_MAIN_copyrichtextext(SOM_MAIN_153_richtext(),msg.undo);
		SendMessage(rt,EM_SETTARGETDEVICE,0,1);
		SendMessage(rt,EM_SETUNDOLIMIT,0,0);		
		SendMessage(rt,EM_SETMODIFY,0,0);
			
		//left combobox/listview afterward
		HWND cb1 = GetDlgItem(hwndDlg,1011); 						
		SOM_MAIN_list::aspects::iterator it;
		SOM_MAIN_list::aspectset::iterator jt;
		SOM_MAIN_list::aspects::exml::iterator kt; if(wParam) 
		{
			it = SOM_MAIN_list.aspects.begin();
			for(jt=it->second.begin();jt!=it->second.end();jt++)
			{	
				ComboBox_SetItemData(cb1,jt->id153 = 
				ComboBox_AddString(cb1,jt->description.c_str()),
				SOM_MAIN_list.aspect_param(*jt));
			}	
			for(kt=SOM_MAIN_list.aspects.exml.begin();*kt;kt++)
			{			
				ComboBox_SetItemData(cb1,(*kt)->id153 = 
				ComboBox_AddString(cb1,(*kt)->description.c_str()),
				SOM_MAIN_list.aspect_param(**kt));
			}
			for(it++;it!=SOM_MAIN_list.aspects.end();it++)
			for(jt=it->second.begin();jt!=it->second.end();jt++)			
			if(!jt->EXML_important()) 
			{			
				ComboBox_SetItemData(cb1,jt->id153 = 
				ComboBox_AddString(cb1,jt->description.c_str()),
				SOM_MAIN_list.aspect_param(*jt));
			}		
			ComboBox_SetCurSel(cb1,0);
	
			int lvs = GetWindowStyle(lv);
			SetWindowLong(lv,GWL_STYLE,lvs&~LVS_SINGLESEL);
			ListView_SetExtendedListViewStyle
			(lv,LVS_EX_FULLROWSELECT|LVS_EX_INFOTIP|LVS_EX_LABELTIP|LVS_EX_HEADERDRAGDROP);
			RECT room; GetClientRect(lv,&room);
			//92/-1: lines up separator on the combobox button
			//and pushes the 3rd column offscreen sized as the 1st-1
			//per som_tool_dialogpixels: 111-vCol1|51+v-2Col2|111-v-1Col3
			LVCOLUMNW lvc = {LVCF_TEXT|LVCF_WIDTH,0,room.right-92,L"??"};
			ListView_InsertColumn(lv,0,&lvc);
			int swap = lvc.cx; lvc.cx = room.right-=lvc.cx-1;
			ListView_InsertColumn(lv,1,&lvc);
			lvc.cx = swap-1;
			ListView_InsertColumn(lv,2,&lvc);
		}
		else ListView_DeleteAllItems(lv); 		
		it = SOM_MAIN_list.aspects.begin();		
		LVITEMW lvi = {LVIF_TEXT|LVIF_PARAM,0,0};
		lvi.iItem = 0; lvi.pszText = LPSTR_TEXTCALLBACKW;
		for(jt=it->second.begin();jt!=it->second.end();jt++)
		{					
			if('\1'==jt->at(0)&&msg.ctxt_s) continue;
			lvi.lParam = SOM_MAIN_list.aspect_param(*jt);				
			ListView_InsertItem(lv,&lvi); lvi.iItem++;
			if(jt->highlighted) 
			ListView_SetItemState(lv,lvi.iItem-1,~0,LVIS_GLOW); 
		}			
		for(it++;it!=SOM_MAIN_list.aspects.end();it++)
		for(jt=it->second.begin();jt!=it->second.end();jt++)				
		{
			if(!jt->selected) //continue
			if(readonly||!jt->highlighted) continue;
			lvi.lParam = SOM_MAIN_list.aspect_param(*jt);
			ListView_InsertItem(lv,&lvi); lvi.iItem++;
			if(jt->highlighted) ListView_SetItemState
			(lv,lvi.iItem-1,~0,LVIS_GLOW|LVIS_CUT*!jt->selected);
		}			
		lvi.mask = LVIF_TEXT; //really?
		for(int i=ListView_GetItemCount(lv)-1;i>=0;i--)
		{
			lvi.iItem = i; //description/aspect columns
			lvi.iSubItem = 1; ListView_SetItem(lv,&lvi);
			lvi.iSubItem = 2; ListView_SetItem(lv,&lvi);
		}
		SOM_MAIN_sort153(lv); 					
		
		if(readonly)
		{
			ListView_SetItemState(lv,-1,LVIS_CUT,~0);

			extern COLORREF som_tool_gray();
			for(int i=1014;i<=1015;i++) Edit_SetReadOnly(GetDlgItem(hwndDlg,i),1);
			int dis[] = {1013,1023,1082,1226,1047,12321,1211,1212,1013};
			windowsex_enable(hwndDlg,dis,0);									

			for(int i=1024;i<=1027;i++) //semi-disable DateTimes (note: custom draw isn't possible)
			{
				HWND dt = GetDlgItem(hwndDlg,i);
				DateTime_SetMonthCalColor(dt,MCSC_TITLEBK,som_tool_gray());
				for(HWND ch=GetWindow(dt,GW_CHILD);ch;ch=GetWindow(ch,GW_HWNDNEXT)) 
				EnableWindow(ch,0);
				SetWindowSubclass(dt,SOM_MAIN_timeoutproc,0,0);		
			}			
		}			

		const int bts[] = {8,1211,1226};
		SOM_MAIN_buttonup(hwndDlg,bts,1082,1085);

		//BN_KILLFOCUS to nix drop highlight
		HWND cut = GetDlgItem(hwndDlg,1226);
		SetWindowLong(cut,GWL_STYLE,BS_NOTIFY|GetWindowStyle(cut));
			   
		TVITEMEXW tvi = {TVIF_TEXT|TVIF_PARAM};
		const wchar_t *outline = SOM_MAIN_label(tv,SOM_MAIN_toplevelitem(tv,sel),tvi);
		HWND cb2 = GetDlgItem(hwndDlg,1013); ComboBox_ResetContent(cb2);
		ComboBox_SetItemData(cb2,ComboBox_SetCurSel(cb2,ComboBox_AddString(cb2,outline)),tvi.lParam);		
		if(msg.unoriginal()) //disable location elements
		{
			int dis[] = {1013,1023,1082,1200,1211,1212};
			windowsex_enable(hwndDlg,dis,0);
			for(int i=0;i<5;i++) if(i!=2) 
			ListView_SetItemState(lv,i,LVIS_CUT,~0);
		}		
		else if(som_tool!=gp) //154? (todo: relax this)
		{
			EnableWindow(cb2,0);
			ListView_SetItemState(lv,msg.ctxt_s?2:3,LVIS_CUT,~0);
		}

		if(!readonly) //NEW: record/create/insert
		SOM_MAIN_153_WhatYouSeeIsWhatYouGet(153);
		SOM_MAIN_list.aspects.exml.li.take_snapshot();
		SOM_MAIN_list.aspects.msgctxt_short_code.take_snapshot();

		if(wParam) //initializing
		{
			//Reminder: want it to be super easy to set the Text
			SetFocus(cb1); SendMessage(hwndDlg,DM_SETDEFID,1009,0);
			if(creating) //PostMessage(hwndDlg,WM_COMMAND,1009,0);
			{
				EnableWindow(hwndDlg,0);
				SetTimer(hwndDlg,'1009',300,SOM_MAIN_153_1009);
			}
		}
		else assert(!creating); //SEE BELOW
		windowsex_enable<1211>(hwndDlg,!creating&&!readonly&&!msg.unoriginal());
		if(!readonly)
		autoascribing.on = hwndDlg;
		return 0;
	}	
	case WM_ENABLE: if(creating) //SEE ABOVE
		if(SOM_MAIN_list.aspects.msgid.edited())
		windowsex_enable<1211>(hwndDlg);
		break;
	
	case WM_SETTEXT: //display [DUPLICATE] indicator
	{
		if(SOM_MAIN_153_sanstext) break;

		enum{ title_s=90 }; 
		static size_t duplicate_s = 0;
		static wchar_t duplicate[title_s/2] = L""; 
		static wchar_t title[title_s+1+sizeof("\4")]; //chkstk

		if(lParam==(LPARAM)title) break; //!!

	  //the text is likely sent by SOM_MAIN_153_loopback//
	  //and so can be anything and so is of no use below//

		if(!*duplicate) //one time initialize
		{
			if(!GetWindowTextW(hwndDlg,duplicate,EX_ARRAYSIZEOF(duplicate)))
			wcscpy(duplicate,som_932w_Duplicate); duplicate_s = wcslen(duplicate);
		}		
	
		SOM_MAIN_tree::pool &pool = SOM_MAIN_tree[tv];		
		SOM_MAIN_tree::string cmp = SOM_MAIN_tree[param];	

		const char *id = cmp.id;
		SOM_MAIN_list.aspects.reselect(param);
		const char *id2 = SOM_MAIN_list.aspects.msgid.selected;
		const char *ctxt = SOM_MAIN_list.aspects.msgctxt.selected;
		if(!ctxt||!*ctxt) ctxt = SOM_MAIN_list.aspects.msgctxt_short_code.selected;				
		//ceiling can be 0 or 1 depending on if the msgctxt+id differ
		size_t ceiling = id2+cmp.ctxt_s==ctxt+cmp.ctxt_s; if(!ceiling) 
		{
			std::string &cat = SOM_MAIN_std::string();
			const char *_4 = ctxt?strchr(ctxt,'\4'):0; assert(!ctxt==!_4);
			if(ctxt&&_4) cat.assign(ctxt,_4-ctxt+1); cmp.ctxt_s = cat.size(); 
			if(id2) cat+=id2; cmp.id = cat.c_str(); ceiling = !strcmp(cmp.id,id);
		}
		size_t dup = 0; if(!cmp.id[cmp.ctxt_s]||pool.count(cmp)>ceiling)
		{
			dup = duplicate_s; wcscpy(title,duplicate)[dup++] = ' ';
		}
		const char *_4 = strchr(cmp.id,'\4');
		const wchar_t *wid = SOM_MAIN_utf8to16(_4?_4+1:cmp.id);
		wcsncpy(title+dup,wid,title_s-dup); title[title_s-1] = '\0';
		SOM_MAIN_ellipsize(title+dup,title_s-dup);	
		//lParam = (LPARAM)title; break; //doesn't work???		
		SetWindowLong(hwndDlg,DWL_MSGRESULT,1);
		return SetWindowTextW(hwndDlg,title);		
	}
	case WM_DRAWITEM: //REMOVE ME?
	{				 			  		
		return som_tool_drawitem((DRAWITEMSTRUCT*)lParam,156);
	}
	case WM_NOTIFY:
	{
		NMHDR *p = (NMHDR*)lParam; HWND &lv = p->hwndFrom;
		
		switch(p->code)
		{
		case DTN_DROPDOWN: if(!readonly) break;
		
			SetWindowSubclass((HWND)
			SendMessage(p->hwndFrom,DTM_GETMONTHCAL,0,0),
			SOM_MAIN_timeoutproc,0,1); break;
		
		case DTN_CLOSEUP: if(!readonly) break;

			PostMessage(p->hwndFrom,WM_KILLFOCUS,0,0);
			break;

		case DTN_USERSTRING: 
		{
			NMDATETIMESTRINGW *p = (NMDATETIMESTRINGW*)lParam;

			//served by som_tool_datetimeproc
			if(p->nmhdr.hwndFrom==SOM_MAIN_loopingback)
			if(!SOM_MAIN_windowstime(&p->st,p->pszUserString))
			{	
				GetLocalTime(&p->st); //todo: some kind of error?
			}
			break;
		}
		case DTN_DATETIMECHANGE:
			if((p->idFrom|1)==1025) autoascribing = SOM_MAIN_list.aspects.exml.tc;
			if((p->idFrom|1)==1027) autoascribing = SOM_MAIN_list.aspects.exml.tm;
			break;	
		case NM_KILLFOCUS:			
			if((p->idFrom|1)==1025) autoascribing = 0;
			if((p->idFrom|1)==1027) autoascribing = 0;
			break; 

		case IPN_FIELDCHANGED: break; //unwieldy. see EN_CHANGE
		
		case LVN_DELETEALLITEMS: 
			SetWindowLong(hwndDlg,DWL_MSGRESULT,1);
			return 1;
		case LVN_DELETEITEM: //suppressed above
		{
			NMLISTVIEW *p = (NMLISTVIEW*)lParam;
			if(LVIS_CUT&~ListView_GetItemState(lv,p->iItem,~0))
			{	
				LPARAM lp = SOM_MAIN_153_listparam(lv,p->iItem);
				if(!SOM_MAIN_153_sanstext||IsWindowVisible(hwndDlg))
				{
					switch(SOM_MAIN_list[lp][0]) //\2: assuming msgstr_body
					{
					default: SOM_MAIN_list[lp].make_edit(0); break;
					case '\2': SetWindowTextW(SOM_MAIN_153_richtext(),L""); break;
					}
					if(!creating) windowsex_enable<12321>(hwndDlg);
				}
			}
			break;
		}
		case LVN_GETDISPINFOW: 
		{
			getdispinfo: //point of no return
			SOM_MAIN_list.aspects.reselect(param);
			NMLVDISPINFO *p = (NMLVDISPINFO*)lParam;			
			SOM_MAIN_tree::string &msg = SOM_MAIN_tree[param];		
			SOM_MAIN_list::aspect &aspect = SOM_MAIN_list[p->item.lParam];
			switch(p->item.iSubItem)
			{
			case 1: 
				
				p->item.pszText = (wchar_t*)aspect.description.c_str(); 
				if(!*p->item.pszText) //fall back onto attribute keyname
				p->item.pszText = (wchar_t*)EX::need_unicode_equivalent(65001,aspect.c_str());
				break;

			default: if(p->item.iSubItem>2) break; //paranoia

				if(aspect.gettext_like()) 
				{
					if(p->item.iSubItem)
					{
						switch(aspect[0])
						{
						case '\0': p->item.pszText = L"msgid"; break;
						case '\1': p->item.pszText = L"msgctxt"; break;
						case '\2': p->item.pszText = L"msgstr body"; break;
						case '\3': p->item.pszText = L"msgctxt short code"; break;						
						}
					}
					else switch(aspect[0])
					{										
					case '\0': //Text
					
						if(aspect.selected) 
						EX::need_unicode_equivalent(65001,aspect.selected,p->item.pszText,p->item.cchTextMax);
						break;
					
					case '\1': //Context
					{
						if(!aspect.selected) break;
						const char *_4 = strchr(aspect.selected,'\4');
						int zt = MultiByteToWideChar(65001,0,aspect.selected,_4-aspect.selected,p->item.pszText,p->item.cchTextMax-1);
						p->item.pszText[zt] = '\0';
						break;
					}	
					case '\2': //Subtext
					{
						HWND rt = SOM_MAIN_153_richtext();
						int zt = Edit_GetLine(rt,0,p->item.pszText,p->item.cchTextMax-3);
						if(Edit_GetLineCount(rt)>1) wcscpy(p->item.pszText+zt,L"..."); 
						else p->item.pszText[zt] = '\0';
						break;
					}
					case '\3': //Outline
					{
						if(GetDlgItemTextW(hwndDlg,1013,p->item.pszText,p->item.cchTextMax))
						break;							
						else if(aspect.selected) //show short code as a fall back?
						{
							//todo: convert to SOM_MAIN_tree::string and lookup label?
							const char *_4 = strchr(aspect.selected,'\4'); if(_4) p->item.pszText
							[MultiByteToWideChar(65001,0,msg.id,_4-msg.id,p->item.pszText,p->item.cchTextMax-1)] = '\0';							
						}
						break;
					}}
					break;
				}
				else if(aspect.EXML_important())
				{
					if(!p->item.iSubItem) 
					{
						int id = 0; HWND di = 0;
						switch(EXML::attribs(aspect.c_str()))
						{
						case EXML::Attribs::li: 
						{
							int zt = SOM_MAIN_outline_item().copy(p->item.pszText,p->item.cchTextMax-1);
							p->item.pszText[zt] = '\0'; goto important;
						}
						case EXML::Attribs::l: id = 1000; break;
						case EXML::Attribs::alt: id = 1001; break;
						case EXML::Attribs::tc:	id = 1024; break;
						case EXML::Attribs::tm: id = 1026; break;
						}
						if(id) di = GetDlgItem(hwndDlg,id);	if(!di) goto unimportant;						
						int cat = GetDlgItemTextW(hwndDlg,id,p->item.pszText,p->item.cchTextMax-1);
						switch(id)
						{
						case 1024: case 1026: //append dates

							p->item.pszText[cat++] = ' ';
							GetDlgItemTextW(hwndDlg,id+1,p->item.pszText+cat,p->item.cchTextMax-cat);
						}		
						important: break;
					}
				}																
				if(p->item.iSubItem)
				{
					std::string &utf8 = SOM_MAIN_std::string(); utf8 = "XML <"; 
					utf8+=aspect.convention; utf8+=" "; utf8+=aspect.c_str(); utf8+=">";
					EX::need_unicode_equivalent(65001,utf8.c_str(),p->item.pszText,p->item.cchTextMax);
				}
				else unimportant: if(aspect.selected) 
				{
					EXML::Quote qt(aspect.selected); p->item.pszText
					[SOM_MAIN_AttribToWideChar(qt,qt.length(),p->item.pszText,p->item.cchTextMax-1)] = '\0';
				}
			} 
			if(p->hdr.code!=LVN_GETINFOTIPW)
			{
				 //list views flatten lines
				if(!p->item.iSubItem) SOM_MAIN_ellipsize(p->item.pszText,p->item.cchTextMax);			
			}
			else goto gotinfotip;
			break;
		}
		case LVN_GETINFOTIPW:
		{
			NMLVGETINFOTIPW *p; int ellipsis; //2021: C4533
			{
			SOM_MAIN_list.aspects.reselect(param);
			p = (NMLVGETINFOTIPW*)lParam;
			NMLVDISPINFOW lvdi = {p->hdr,LVIF_PARAM,p->iItem,0,0,0,p->pszText,p->cchTextMax};
			if(!ListView_GetItem(lv,&lvdi.item)) break; 

			ellipsis = *p->pszText;
			//formats: would have to preload dialog 179?
			SOM_MAIN_list::aspect &aspect = SOM_MAIN_list[lvdi.item.lParam];
			//SOM_MAIN_list::format &format = SOM_MAIN_list.formats[aspect];
			if(aspect.gettext_like()) switch(aspect[0]) 
			{				
			case '\2': //full Subtext?

				GetWindowTextW(SOM_MAIN_153_richtext(),p->pszText,p->cchTextMax);
				goto gotinfotip;
				
			case '\3': //Outline short code
			{
				const char *sc = aspect.selected;
				HWND cb = GetDlgItem(hwndDlg,1013);				
				LPARAM lp = ComboBox_GetItemData(cb,ComboBox_GetCurSel(cb));
				if(lp) sc =  SOM_MAIN_tree[lp].id;
				//\0: signal SOM_MAIN_list::cell::quote
				if(*sc<'\4') return p->cchTextMax = *p->pszText = '\0'; 
				const char *_4 = strchr(sc,'\4'); if(_4) p->pszText
				[MultiByteToWideChar(65001,0,sc,_4-sc,p->pszText,p->cchTextMax-1)] = '\0';
				return 0;
			}}
			else if(aspect.EXML_important())
			{
				int dt = 0;
				if(aspect.size()==2&&aspect[0]=='t') switch(aspect[1])
				{
				case 'c': dt = 1024; break;	case 'm': dt = 1026; break; 
				}  
				SYSTEMTIME d,t; if(dt) //base64 code
				if(!SendDlgItemMessage(hwndDlg,dt+0,DTM_GETSYSTEMTIME,0,(LPARAM)&d))
				if(!SendDlgItemMessage(hwndDlg,dt+1,DTM_GETSYSTEMTIME,0,(LPARAM)&t))
				{
					memcpy(&d.wHour,&t.wHour,sizeof(WORD)*3); 
					EX::need_unicode_equivalent(65001,SOM_MAIN_timencode(&d),p->pszText,p->cchTextMax);
					return 0;
				}
			}
			*p->pszText = '\0';
			lParam = (LPARAM)&lvdi; goto getdispinfo; 
			}//C4533
			gotinfotip: 
			if(!ellipsis&&!wcschr(p->pszText,'\n')) *p->pszText = '\0';
			break;
		}
		case LVN_ITEMCHANGED: //highlight combobox
		{
			NMLISTVIEW *p = (NMLISTVIEW*)lParam;

			if(p->uChanged&LVIF_STATE&&p->iItem!=-1)
			if(LVIS_SELECTED&(p->uNewState^p->uOldState))
			{
				size_t sc = ListView_GetSelectedCount(lv); 				
				if(sc==1&&LVIS_SELECTED&p->uNewState&&~p->uOldState&LVIS_SELECTED)			
				SendDlgItemMessage(hwndDlg,1011,CB_SETCURSEL,SOM_MAIN_list[p->lParam].id153,0);
			}
			break;
		}
		case LVN_ITEMACTIVATE: goto ascribe;

		case LVN_KEYDOWN:

			if(GetKeyState(VK_CONTROL)>>15)
			switch(((NMLVKEYDOWN*)lParam)->wVKey)
			{
			case 'X': goto cut;
			case 'C': goto copy;
			case 'V': goto paste;
			}
			else if(VK_DELETE==((NMLVKEYDOWN*)lParam)->wVKey)
			{
				if(readonly) return MessageBeep(-1);
				int i,from = ListView_GetNextItem(lv,-1,LVNI_SELECTED);
				if((i=from)!=-1) do 
				if(SOM_MAIN_153_unlocked(i,lv)&&i!=SOM_MAIN_153_sanstext)				
				ListView_DeleteItem(lv,i--); else MessageBeep(-1);
				while(-1!=(i=ListView_GetNextItem(lv,i,LVNI_SELECTED)));
				SOM_MAIN_enumerate153(lv,from);
				if(SOM_MAIN_clip.po)
				SOM_MAIN_153_drawclipboard(param);
			}
			break;
		
		case LVN_BEGINLABELEDITW:

			//REMOVE ME?
			Edit_SetReadOnly(ListView_GetEditControl(lv),1);
			break;
		}	
		break;
	}	
	case WM_MOUSEMOVE: //not pixel perfect

		autoascribing = 0; //about to click listview?
		break;

	case WM_COMMAND:		
		
		if(!HIWORD(wParam)) 		
		switch((SHORT&)wParam) //ticker?
		{
		case -1082: case +1082:
		sel = SOM_MAIN_tree[param][tv]; 
		case -1083:	case +1083: case -1084:	case +1085:
		SendMessage(hwndDlg,SOM_MAIN_cpp::wm_wavefunctioncollapse,0,0);
		switch((SHORT&)wParam) //ticker!!
		{
		case -1082: case +1082: //elevator: going up?
		{
			HTREEITEM i = sel, ii = 
			TreeView_GetNextItem(tv,i,TVGN_NEXT+(wParam>>15));
			if(!ii) MessageBeep(-1);
			else if(!SOM_MAIN_tree[param].temporary())
			{
				SOM_MAIN_swap(tv,i,ii);

				if(som_tool!=GetParent(hwndDlg)) 
				{
					LPARAM param2 = SOM_MAIN_param(tv,ii); 
					if(refocus) SOM_MAIN_list[refocus].swapped(i,ii);
					if(refocus) SOM_MAIN_list[refocus]->swapped(i,ii);
				}
				SOM_MAIN_outline_item(tv,i);
			}
			else SOM_MAIN_outline_item(tv,sel=ii);
			SendDlgItemMessage(hwndDlg,1023,IPM_SETADDRESS,0,SOM_MAIN_outline_item_octets());							
			TreeView_SelectItem(tv,sel); TreeView_EnsureVisible(tv,sel);			
			SOM_MAIN_153_autoascribe(lv,0); //0: drophilite li
			drophilited = SOM_MAIN_list.aspects.exml.li.row153;
			ListView_SetItemState(lv,drophilited,~0,LVIS_DROPHILITED);
			ListView_EnsureVisible(lv,drophilited,1);
			break;
		}
		case -1083:	case +1083: case -1084:	case +1085: //dpad 
		{	
			HTREEITEM stuck = 
			TreeView_GetSelection(tv); stuck:			
			int pos = GetScrollPos(tv,SB_VERT);
			int vk = 0; switch((SHORT&)wParam)
			{			
			case +1083: vk = VK_DOWN; break; case -1083: vk = VK_UP; break;
			case -1084: vk = VK_LEFT; break; case +1085: vk = VK_RIGHT; break;
			}			
		//	assert(0); //??? //2023
			//should this be 0xff80? //is it suppressing or adding?
			som_tool_VK_CONTROL = 1; SendMessage(tv,WM_KEYDOWN,vk,0);
			som_tool_VK_CONTROL = 0; 
			HTREEITEM i = TreeView_GetSelection(tv);			
			if(i&&TreeView_GetParent(tv,i)) 
			{
				SOM_MAIN_textarea ta(tv,i);	if(ta) //skip unless scrolling
				{
					if(stuck==i&&vk==VK_RIGHT) //pushing right?
					{
						if(ta.param) return MessageBeep(-1);
												
						LPARAM url = SOM_MAIN_param(tv,ta.item);

						SOM_MAIN_message_text(url,tv,ta.item); return 0;
					}
							
					if(pos!=GetScrollPos(tv,SB_VERT)||vk==VK_RIGHT) break;
					
					stuck = i; goto stuck; //scrolling/skipping to next item
				}
				else if(stuck==i) break; //expanding?
			}
			else //cannot leave the outline 
			{
				TreeView_SelectItem(tv,sel); return MessageBeep(-1); 
			}
			SetWindowRedraw(hwndDlg,0);	SendMessage(hwndDlg,WM_INITDIALOG,0,0);
			SetWindowRedraw(hwndDlg,1);	RedrawWindow(hwndDlg,0,0,RDW_INVALIDATE);
			break;		
		}}}
		switch(wParam)
		{
		case MAKEWPARAM(1023,EN_CHANGE): //IP control
		{
			std::wstring &oli = SOM_MAIN_outline_item();
			if((HWND)lParam==GetParent(SOM_MAIN_loopingback))
			{
				SOM_MAIN_list.aspects.reselect(param);
				const char *li = SOM_MAIN_list.aspects.exml.li.selected;
				oli.clear(); if(!li) li = ""; if(*li=='-') li++; 
				while(isdigit(*li)||*li=='-'&&li[1]!='-'&&li[1]) //li=l
				oli.push_back(*li++); if(oli.empty()) oli = '1';
				PostMessage((HWND)lParam,IPM_SETADDRESS,0,SOM_MAIN_outline_item_octets());				
			}
			else //record the change for WYSIWYG retrieval
			{				
				size_t first = {oli.find_first_of('-')}, last = first;
				if(first==oli.npos) first = oli.size(); if(!first) break; //WM_INITDIALOG???
				for(size_t i=0;i<4&&last!=oli.npos;i++) last = oli.find_first_of('-',last+1);
				BYTE ip[4] = {0,0,0,0}; SendMessage((HWND)lParam,IPM_GETADDRESS,0,(LPARAM)ip);
				wchar_t dashed[17]; swprintf(dashed,L"-%d-%d-%d-%d",ip[3],ip[2],ip[1],ip[0]);
				oli.replace(first,last-first,dashed); 
				if(last==oli.npos) for(int i=0;i<4&&!ip[i];i++) oli.resize(max(oli.size(),2)-2);
				autoascribing = SOM_MAIN_list.aspects.exml.li;					
			}
			break;
		}
		case MAKEWPARAM(1000,EN_CHANGE): autoascribing = SOM_MAIN_list.aspects.exml.l;
			break;
		case MAKEWPARAM(1001,EN_CHANGE): autoascribing = SOM_MAIN_list.aspects.exml.alt;
			break;
		case MAKEWPARAM(1000,EN_KILLFOCUS): case MAKEWPARAM(1001,EN_KILLFOCUS): 
		case MAKEWPARAM(1023,EN_KILLFOCUS): autoascribing = 0;
			break;
		case MAKEWPARAM(1013,CBN_SELENDOK):			
			SOM_MAIN_153_autoascribe((HWND)lParam,&SOM_MAIN_list.aspects.msgctxt_short_code);
			break;

		case MAKEWPARAM(1226,BN_KILLFOCUS):
		
			ListView_SetItemState(lv,drophilited,0,LVIS_DROPHILITED);
			break;

		case 1009: ascribe:
		case MAKEWPARAM(1011,CBN_SELENDOK):	
		{ 
			HWND cb = GetDlgItem(hwndDlg,1011);
			int sel = ComboBox_GetCurSel(cb);
			if(sel==-1) //adding custom aspect?
			{
				wchar_t name[60];
				if(ComboBox_GetText(cb,name,EX_ARRAYSIZEOF(name)))
				sel = ComboBox_FindStringExact(cb,-1,name);
				if(sel==-1) sel =  
				ComboBox_SetCurSel(cb,ComboBox_AddString(cb,name));
			}			
			int id = ComboBox_GetItemData(cb,sel); if(!id) //custom?
			{
				//future feature
				return !SOM_MAIN_report("MAIN1001",MB_OK); 
				if(readonly)
				return !SOM_MAIN_report("MAIN230",MB_OK); 
				//unimplemented
				assert(0); MessageBeep(-1); return 0;
			}
						
			SOM_MAIN_list::aspect &aspect = SOM_MAIN_list[id];
			//0: add temporarily to listview if not there, beeps if readonly
			if(!SOM_MAIN_153_autoascribe(0,&aspect)) return 0;

			int d = aspect.dialog(); if(d==172)
			{
				SOM_MAIN_list.item.locked = readonly;
				SOM_MAIN_172(hwndDlg,param,SOM_MAIN_153_richtext());
			}
			else if(SOM_MAIN_list.item.quote(aspect.row153))
			{		
				if(SOM_MAIN_153_1009_text) //170
				SOM_MAIN_list.item.quoted = SOM_MAIN_153_1009_text;
				SOM_MAIN_(d,hwndDlg,SOM_MAIN_17X,id);
			}
			else assert(0);	break;
		}
		case MAKEWPARAM(1013,CBN_DROPDOWN):

			if(1==ComboBox_GetCount((HWND)lParam))
			{
				TVITEMEXW tvi = {TVIF_TEXT|TVIF_PARAM};
				tvi.hItem = TreeView_GetChild(tv,TVI_ROOT);	
				LPARAM blindspot = SOM_MAIN_right.japanese;								
				LPARAM id = ComboBox_GetItemData((HWND)lParam,0);
				while(tvi.hItem=TreeView_GetNextSibling(tv,tvi.hItem))
				if(*SOM_MAIN_label(tv,tvi.hItem,tvi)&&tvi.lParam!=blindspot) 
				if(id==tvi.lParam) id = 0; else ComboBox_SetItemData((HWND)lParam,
				ComboBox_InsertString((HWND)lParam,id?0:-1,tvi.pszText),tvi.lParam);
			}
			break;
		 		
		case 1010: //preview
		
			if(!readonly)
			SOM_MAIN_153_WhatYouSeeIsWhatYouGet(180);			
			SOM_MAIN_(180,hwndDlg,SOM_MAIN_180,param);
			break;
	
		case 1200: //checkbox
			
			som_tool_highlight(hwndDlg,1211); 
			break;

		//record/create/insert
		case 12321: case 1211: case 1212: 
		{
			//TODO: use SOM_MAIN_copy (notes in record)
			int todolist[SOMEX_VNUMBER<=0x1020504UL];

			//REMOVE ME??
			sel = SOM_MAIN_tree[param][tv];
			SendMessage(hwndDlg,SOM_MAIN_cpp::wm_wavefunctioncollapse,0,0);	
			TreeView_SelectItem(tv,sel); 
			SOM_MAIN_153_WhatYouSeeIsWhatYouGet(wParam);			
			{			
				time_t t = time(0);
				static unsigned tt = t; 
				if(tt>=t) t = tt+1; tt = t;
				const char *te = SOM_MAIN_timencode(t);
				//todo: MessageBox if edited_versus_snapshot
				SOM_MAIN_list.aspects.exml.tm.make_edit(te);
				if(wParam!=12321)				
				SOM_MAIN_list.aspects.exml.tc.make_edit(te);				
			}		
			std::pair<HTREEITEM,bool> itp = 			
			SOM_MAIN_list.aspects.exml.li.edited_versus_snapshot()||
			SOM_MAIN_list.aspects.msgctxt_short_code.edited_versus_snapshot()
			?SOM_MAIN_outline_item_pair(tv,SOM_MAIN_list.aspects.msgctxt_short_code.selected)
			:std::make_pair(sel //todo? tri-sate checkbox
			,wParam==1211&&!IsDlgButtonChecked(hwndDlg,1200));
			SOM_MAIN_list.aspects.reselect(param);
			SOM_MAIN_list.aspects.record(SOM_MAIN_tree.temp,SOM_MAIN_std::string());
			HTREEITEM &i = itp.first; i = 
			SOM_MAIN_introduceitem(SOM_MAIN_temparam(),tv,i,itp.second,
			wParam==12321?sel:0,SOM_MAIN_outline_item_vector().size());
			if(!i) break;
			//fyi: introduceitem keeps 154 uptodate
			if(SOM_MAIN_list.aspects.msgstr_body.edited())
			SOM_MAIN_editrichtextext(SOM_MAIN_tree
			[SOM_MAIN_param(tv,i)].undo,SOM_MAIN_153_richtext());
			if(SOM_MAIN_list.aspects.exml.hl.edited())
			SOM_MAIN_rainbowtab(SOM_MAIN_tree[SOM_MAIN_param(tv,i)].undo);				
			SOM_MAIN_changed(tv,i); 
			if(SOM_MAIN_153_sanstext) //issues
			if(SOM_MAIN_153_autoascribe(lv,&SOM_MAIN_list.aspects.exml.tm)) //hack
			return EnableWindow((HWND)lParam,0); if(wParam==1212)
			SOM_MAIN_move(tv,TreeView_GetNextSibling(tv,i),tv,i); 
			if(i!=TreeView_GetSelection(tv)) //these do different things
			TreeView_SelectItem(tv,i)&&TreeView_SelectSetFirstVisible(tv,i);						
			SetWindowRedraw(hwndDlg,0);	SendMessage(hwndDlg,WM_INITDIALOG,0,0);
			SetWindowRedraw(hwndDlg,1);	RedrawWindow(hwndDlg,0,0,RDW_INVALIDATE);								
			break;
		}		
		case 1226: cut:

			if(readonly) return MessageBeep(-1);
			ListView_SetItemState(lv,drophilited,0,LVIS_DROPHILITED);
			//break;

		case 1213: copy:
		
			if(!readonly)
			SOM_MAIN_153_WhatYouSeeIsWhatYouGet();
			if(SOM_MAIN_copytoclipboard(param,lv))
			{
				if(wParam==1226) //cutting
				SendMessage(lv,WM_KEYDOWN,VK_DELETE,0);
			}
			break;

		case 1047: paste:

			if(!readonly&&SOM_MAIN_clip.format) 
			SOM_MAIN_153_paste(); else MessageBeep(-1);
			break;
		
		case IDCANCEL:
		case IDCLOSE: goto close;
		}
		break;
	
	case WM_CLOSE: close:
		
		DestroyWindow(hwndDlg); 	

	case WM_NCDESTROY:

		autoascribing.off();
		SOM_MAIN_153_1009_text = 0;
		SOM_MAIN_153_1009_clientext = 0;
		SOM_MAIN_list = refocus;
		SOM_MAIN_list.item.listview = 0;
		SOM_MAIN_list.aspects.edit(0);
		SOM_MAIN_clipboard = 0; 
		break;

	case SOM_MAIN_cpp::wm_wavefunctioncollapse:

		if(SOM_MAIN_same()&&som_tool!=GetParent(hwndDlg))
		{
			HWND tv2 = //collapse the wave function?
			SOM_MAIN_list[refocus].underlying_treeview();
			if(tv2!=tv)
			{
				sel = SOM_MAIN_reproduceitem(tv2,tv,sel);
				tv = tv2; assert(SOM_MAIN_left(tv2));					
			}
		}				
		break;

	case SOM_MAIN_cpp::wm_drawclipboard: 

		if(readonly) break;		
		SOM_MAIN_list.aspects.highlight(SOM_MAIN_cliparam(tv));
		if(IsWindowVisible(hwndDlg)) SOM_MAIN_153_drawclipboard(param);
		windowsex_enable<1047>(hwndDlg, //paste?
		!SOM_MAIN_clip.empty()&&(SOM_MAIN_clip.po||ListView_GetSelectedCount(lv)));
        break;
	}
	return 0;
}
static void SOM_MAIN_153_drawclipboard(LPARAM param)
{
	bool sort = false;
	HWND lv = SOM_MAIN_list.item.listview;
	SOM_MAIN_list.aspects.reselect(param);		
	LVITEMW lvi = {LVIF_PARAM|LVIF_STATE,0,0};
	lvi.pszText = LPSTR_TEXTCALLBACKW;				
	for(SOM_MAIN_list::aspects::foreach ea;ea;ea++)
	{
		lvi.iItem = ea->row153;
		lvi.stateMask = LVIS_GLOW|LVIS_CUT;
		LPARAM lp = SOM_MAIN_list.aspect_param(*ea);
		if(!ListView_GetItem(lv,&lvi)||lp!=lvi.lParam)
		{
			if(ea->highlighted)			
			if(ea->XML_like()||!SOM_MAIN_153_sanstext)
			{
				sort = true; lvi.iItem = 10000;
				lvi.lParam = lp; lvi.mask|=LVIF_TEXT;
				lvi.state = LVIS_GLOW|LVIS_CUT; 
				lvi.iItem = ListView_InsertItem(lv,&lvi); 
				lvi.iSubItem = 1; ListView_SetItem(lv,&lvi);
				lvi.iSubItem = 2; ListView_SetItem(lv,&lvi);			
				lvi.iSubItem = 0; lvi.mask&=~LVIF_TEXT;
			}
		}
		else if(SOM_MAIN_153_unlocked(lvi.state))
		{
			if(!ea->highlighted)
			{
				if(!ea->selected&&ea->XML_like()) 
				{
					ListView_DeleteItem(lv,ea->row153);
					SOM_MAIN_enumerate153(lv,ea->row153);
				}
				else if(lvi.state&LVIS_GLOW)
				{
					lvi.state = 0; lvi.stateMask = LVIS_GLOW;
					SendMessage(lv,LVM_SETITEMSTATE,ea->row153,(LPARAM)&lvi);
				}
			}
			else if(~lvi.state&LVIS_GLOW)
			{					
				lvi.state = lvi.stateMask = LVIS_GLOW;
				SendMessage(lv,LVM_SETITEMSTATE,ea->row153,(LPARAM)&lvi);
			}
		}
	}
	if(sort) SOM_MAIN_sort153(lv); 
}
static void SOM_MAIN_153_WhatYouSeeIsWhatYouGet(int preview)
{
	assert(SOM_MAIN_list.aspects.edited);
	HWND lv = SOM_MAIN_list.item.listview;
	struct SOM_MAIN_list::aspects::exml &exml = SOM_MAIN_list.aspects.exml;
	const SOM_MAIN_list::aspect *wysiwyg[] = 
	{&SOM_MAIN_list.aspects.msgctxt_short_code,&exml.li,&exml.tm,&exml.tc,&exml.l,&exml.alt};
	//2: may want to increase to 3 to include exml.tm (edited_versus_snapshot)
	size_t n = preview==153?2:EX_ARRAYSIZEOF(wysiwyg); for(size_t i=0;i<n;i++)	
	if(preview||LVIS_SELECTED&ListView_GetItemState(lv,wysiwyg[i]->row153,~0))
	if(SOM_MAIN_list.aspect_param(*wysiwyg[i])==SOM_MAIN_153_listparam(lv,wysiwyg[i]->row153))
	if(LVIS_CUT&~ListView_GetItemState(lv,wysiwyg[i]->row153,~0))
	{
		//0: if empty, delete if gettext-like, else emit as boolean type
		const wchar_t *quoted = SOM_MAIN_list->quote(wysiwyg[i]->row153);
		wysiwyg[i]->make_edit(!*quoted&&wysiwyg[i]->gettext_like()?0:quoted);
	}
	HWND rt = SOM_MAIN_153_richtext(); if(!Edit_GetModify(rt)) return;
	SOM_MAIN_list.aspects.msgstr_body.make_edit
	(SOM_MAIN_peelpoortext(SOM_MAIN_std::vector<char>(),rt));
	Edit_SetModify(rt,0);
}
static int SOM_MAIN_17X_cancel = -1;			
static bool SOM_MAIN_153_autoascribe(HWND text, const void *aspect)
{		
	bool drophilite = !aspect;
	HWND lv = SOM_MAIN_list->listview;	
	//WARNING: this subroutine is used at least four different ways
	//depending on if text is 0, nonzero, or lv, and if aspect is 0
	const SOM_MAIN_list::aspect *p = (SOM_MAIN_list::aspect*)aspect;		
	if(drophilite) p = &SOM_MAIN_list.aspects.exml.li; 
	LPARAM edited = 0; if(text&&!drophilite)
	{
		edited = SOM_MAIN_list.aspects.edited;			
		SOM_MAIN_list.aspects.reselect(edited);
		if(edited&&edited!=SOM_MAIN_temparam())
		windowsex_enable<12321>(GetParent(lv)); //obsolete? 
	}			
	int add = lv==text||GetWindowTextLengthW(text);	
	LVITEMW lvi = {LVIF_PARAM|LVIF_STATE,p->row153,0,0,~0};		
	if(ListView_GetItem(lv,&lvi)&&lvi.lParam==SOM_MAIN_list.aspect_param(*p))
	{
		if(text) if(!add)
		{
			p->make_edit(0); ListView_DeleteItem(lv,p->row153);
			SOM_MAIN_enumerate153(lv,p->row153);
		}
		else if(add)
		{
			if((LVIS_CUT|LVIS_GLOW)==((LVIS_CUT|LVIS_GLOW)&lvi.state))
			{
				lvi.state&=~LVIS_CUT; //was a placeholder
				SendMessage(lv,LVM_SETITEMSTATE,p->row153,(LPARAM)&lvi);
			}
			ListView_RedrawItems(lv,p->row153,p->row153);	
		}
	}
	else if(add||!text)
	{
		if(SOM_MAIN_153_sanstext)		
		if(p->gettext_like()||p==&SOM_MAIN_list.aspects.exml.li)
		{
			//aspect is unavailable on this screen
			return !SOM_MAIN_report("MAIN1020",MB_OK);
		}
		if(SOM_MAIN_readonly(SOM_MAIN_list->underlying_treeview()))
		{
			assert(!text); //MessageBeep(-1);
			return !SOM_MAIN_report("MAIN230",MB_OK);
		}		

		if(p->highlighted||!text)
		lvi.state = LVIS_GLOW+LVIS_CUT*!text;
		lvi.mask|=LVIF_TEXT; lvi.pszText = LPSTR_TEXTCALLBACKW;
		lvi.iItem = '?'; lvi.lParam = SOM_MAIN_list.aspect_param(*p); 		
		lvi.iItem = ListView_InsertItem(lv,&lvi);					
		lvi.mask = LVIF_TEXT;
		lvi.iSubItem = 1; ListView_SetItem(lv,&lvi);
		lvi.iSubItem = 2; ListView_SetItem(lv,&lvi);				

		SOM_MAIN_sort153(lv);		
				
		//hack: don't let SOM_MAIN_153_drawclipboard
		//delete the row if its highlight is removed
		if(text&&text==p->loopback153&&!p->selected)					
		p->make_edit(SOM_MAIN_list->quote(p->row153));

		//mark as temporary so deleted if canceled
		if(!text) SOM_MAIN_17X_cancel = p->row153;
	}
	return true;
}
static void SOM_MAIN_153_paste()
{
	LVITEMW lvi = 
	{LVIF_PARAM|LVIF_STATE,0,0,0,~0}; 
	int &i = lvi.iItem, lp, beep = 1;
	HWND lv = SOM_MAIN_list->listview;
	bool masked = ListView_GetSelectedCount(lv); 
	const char *c_str = SOM_MAIN_clip.c_str(), *hl, *op;
	for(SOM_MAIN_list::aspects::foreach ea;ea;ea++)
	{
		hl = ea->highlighted; if(!hl&&!masked) continue;
		i = ea->row153; lp = SOM_MAIN_list.aspect_param(*ea);		
		if(!ListView_GetItem(lv,&lvi)||lvi.lParam!=lp) continue;
		else if(masked&&~lvi.state&LVIS_SELECTED) continue;
		else if(SOM_MAIN_153_unlocked(lvi.state))
		{		
			char d = '\0'; EXML::Quote qt; 
			{
				if(SOM_MAIN_clip.po) if(hl&&ea->XML_like()) 
				{
					//-1: if not = then emit a boolean attribute
					c_str = hl[-1]!='='?"":qt=EXML::Quote(d,hl);
				}
				else c_str = hl; op = !hl||*c_str!='\v'?c_str:0;

				if(ea->at(0)||*c_str>='!') //protecting msgid
				{
					//?: placeholder for msgstr body
					if(ea->at(0)=='\2'&&SOM_MAIN_fillrichtext
					(SOM_MAIN_153_richtext(),op,SOM_MAIN_clip.po?0:op))
					op = "?"; ea->make_edit(op);
					if(op) lvi.state&=~LVIS_CUT; else lvi.state|=LVIS_CUT;
					ListView_SetItem(lv,&lvi);
					ListView_RedrawItems(lv,ea->row153,ea->row153);
				}
				else MessageBeep(-1); beep = 0;
			}
			if(d) qt.unquote(d);
		}		 
	}	 
	if(beep) MessageBeep(-1);
}

struct SOM_MAIN_sort
{
	int tie; LPARAM a, b; SOM_MAIN_sort(LPARAM lva, LPARAM lvb)
	{
		HWND tv = SOM_MAIN_list->corresponding_treeview();
		a = SOM_MAIN_param(tv,(HTREEITEM)lva); b = SOM_MAIN_param(tv,(HTREEITEM)lvb); 
		tie = a!=b?0:SOM_MAIN_level(tv,(HTREEITEM)lva)-SOM_MAIN_level(tv,(HTREEITEM)lvb);
	}
};
static int CALLBACK SOM_MAIN_sortbyText(LPARAM a, LPARAM b, LPARAM c)
{
	SOM_MAIN_sort ab(a,b); if(ab.tie) return ab.tie;
	static std::vector<wchar_t> abuf(24*16), bbuf(24*16);
	struct sortby
	{
		sortby(std::vector<wchar_t> &buf, LPARAM lp, LPARAM c)
		{
			SOM_MAIN_tree::string &msg = SOM_MAIN_tree[lp];
			const char *mb = msg.id; int mb_s = msg.id_s; switch(c)
			{
			case 1: mb+=msg.ctxt_s; mb_s-=msg.ctxt_s; case 0: break; default: 
			EXML::Quote qt(msg.exml_attribute((EXML::Attribs)c)); if(!qt||!*qt)
			{
				wmemset(&buf[0],0xFFFF,3)[3] = '\0'; return; //assuming ascending order
			}
			else mb = qt; mb_s = qt.length();
			}
			while(MultiByteToWideChar(65001,0,mb,mb_s,&buf[0],buf.size())>=(int)buf.size()-1)			
			buf.resize(MultiByteToWideChar(65001,0,mb,mb_s,0,0)+10); //+1	
		}

	}sa(abuf,ab.a,c), sb(bbuf,ab.b,c); return wcscmp(&abuf[0],&bbuf[0]);
}
static int CALLBACK SOM_MAIN_sortbyReal(LPARAM a, LPARAM b, LPARAM c)
{
	SOM_MAIN_sort ab(a,b); if(ab.tie) return ab.tie;
	const char *sa = SOM_MAIN_tree[ab.a].exml_attribute((EXML::Attribs)c);
	const char *sb = SOM_MAIN_tree[ab.b].exml_attribute((EXML::Attribs)c);
	if(!sa) return sb?1:0; if(!sb) return -1;
	char *ea, *eb; double diff = strtod(sa,&ea)-strtod(sb,&eb);
	if(ea==sa) return eb==sb?1:0; if(eb==sb) return -1;
	return diff<0?-1:diff?1:0;
}
static int CALLBACK SOM_MAIN_sortbyTime(LPARAM a, LPARAM b, LPARAM c)
{
	SOM_MAIN_sort ab(a,b); if(ab.tie) return ab.tie;
	struct sortby
	{
		time_t t; sortby(LPARAM lp, LPARAM c) : t(-1)
		{
			int i = 0, t_s = sizeof(t); char a[12]; 
			EXML::Quote qt(SOM_MAIN_tree[lp].exml_attribute((EXML::Attribs)c));
			if(!qt||!*qt) return;
			for(int n=qt.length();i<sizeof(a)&&i<n;i++) a[i] = qt[i];
			while(i<11) a[i++] = 'A'; a[11] = '=';
			som_atlenc_Base64Decode(a,12,(BYTE*)&t,&t_s);
		}

	}sa(ab.a,c), sb(ab.b,c); return sb.t-sa.t; //backwards in time order
}
static int CALLBACK SOM_MAIN_sortbyItemEx(LPARAM i, LPARAM ii, HWND lv)
{
	wchar_t a[30] = L"", *p = a, b[30] = L"", *q = b;
	ListView_GetItemText(lv,i,0,a,30); ListView_GetItemText(lv,ii,0,b,30);	
	//ListView_SortItemsEx reverses the order!
	//(note: that the MSDN docs say otherwise)
	long cmp = wcstol(q,&q,10)-wcstol(p,&p,10);
	while(!cmp&&*p=='-'&&*q=='-')	
	cmp = wcstol(++q,&q,10)-wcstol(++p,&p,10);	
	return !cmp?*p=='-'?+1:0:cmp<0;
}					  

inline bool SOM_MAIN_154_divider(RECT &div)
{
	if(!SOM_MAIN_list.left.empty())
	if(!SOM_MAIN_list.right.empty()) 
	{
		SOM_MAIN_RTLPOS
		return SetRect(&div,lpos.right,lpos.top,rpos.left,lpos.bottom);			
	}
	return false;
}
static void SOM_MAIN_154_bookmark(HWND,int);
static bool SOM_MAIN_154_enter(HWND,HTREEITEM);
#define SOM_MAIN_154_slide(lv) SOM_MAIN_154_enter(lv,TVI_ROOT)
static INT_PTR CALLBACK SOM_MAIN_154(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{	
	//schdeduled obsolete
	static RECT sep; static int left_of_trackbar, x; 
	const int bts[] = {1,8,1212, 1080,1081,1082,1083,1084};		
	//todo: reconsider/rename these 
	static const int doublewide = 640; static HWND singlewide = 0;

	switch(uMsg)
	{
	case WM_INITDIALOG:
	{	
		bool matched = SOM_MAIN_same();
		HWND tv = SOM_MAIN_tree->treeview;		
		if(!matched&&!SOM_MAIN_leftonly())
		{
			matched = SOM_MAIN_tree[tv]->end()!=
			SOM_MAIN_tree[tv]->find(SOM_MAIN_tree[lParam]);
		}		
		SOM_MAIN_list.aspects.fill(); //chkstk workaround		
		
		//copying SOM_MAIN_152
		RECT tb, rc, cr; HWND 
		lv = GetDlgItem(hwndDlg,1023),
		hw = GetDlgItem(hwndDlg,1201); //trackbar accordion
		SOM_MAIN_list.left.layout =
		SOM_MAIN_list.right.layout = singlewide = lv;
		GetWindowRect(lv,&sep);		
		MapWindowRect(0,hwndDlg,&sep); 
		GetClientRect(hwndDlg,&cr);
		sep.right = cr.right-sep.right;
		sep.bottom = cr.bottom-sep.bottom;				
		GetWindowRect(hw,&tb); left_of_trackbar = tb.left;
		for(int i=0;i<EX_ARRAYSIZEOF(bts);i++) 
		if(GetWindowRect(hw=GetDlgItem(hwndDlg,bts[i]),&rc))
		if(rc.left<left_of_trackbar) left_of_trackbar = rc.left;
		left_of_trackbar = tb.left-left_of_trackbar;
				
		SOM_MAIN_list.left.listview = SOM_MAIN_left(tv)?lv:0;
		SOM_MAIN_list.right.listview = SOM_MAIN_left(tv)?0:lv;
		SOM_MAIN_list[lv].sidebyside = 0;
		SOM_MAIN_list[lv]->sidebyside = 
		matched?0:SOM_MAIN_list[lv].corresponding_treeview(); //tv
		SOM_MAIN_list.left.clear();
		SOM_MAIN_list.right.clear();
		
		SOM_MAIN_RTLPOS
		SOM_MAIN_tree::pool &pool =
		lv==r?SOM_MAIN_right:SOM_MAIN_tree.left;	
		if(som_tool==SOM_MAIN_151())
		som_tool_recenter(hwndDlg,som_tool==SOM_MAIN_152()?som_tool:tv); 
		GetWindowRect(hwndDlg,&rc);
		int cx = rc.right-rc.left; 
		if(IsWindowVisible(pool->treeview))
		{
			GetWindowRect(pool->treeview,&cr);
			rc.left = lv==r^som_tool==SOM_MAIN_151()?cr.right-cx:cr.left;			
		}
		else //mode 2 w/single panel
		{
			GetWindowRect(som_tool,&cr); rc.left = cr.left-cx; 
		}
		if(som_tool==SOM_MAIN_152())
		{
			GetWindowRect(som_tool,&cr); rc.top = cr.top; rc.bottom = cr.bottom;
		}
		MoveWindow(hwndDlg,rc.left,rc.top,cx,rc.bottom-rc.top,0);
		SendMessage(hwndDlg,DM_REPOSITION,0,0);

		HTREEITEM outline = SOM_MAIN_tree
		[lParam][SOM_MAIN_list[lv].corresponding_treeview()];
		SOM_MAIN_154_enter(lv,outline);			

		int bts2[] = {1,8,1212};
		SOM_MAIN_buttonup(hwndDlg,bts2,1080,1084);	

		//make reading all easy
		SetFocus((HWND)wParam);
		ListView_SetItemState(lv,0,1,LVIS_FOCUSED|LVIS_SELECTED);		
		return 0;
	}
	case WM_SIZE: //todo: somehow merge 152/154/172
	{
		if(wParam!=SIZE_RESTORED&&wParam!=SIZE_MAXIMIZED) break;
		
		int w = LOWORD(lParam), h = HIWORD(lParam), z = SWP_NOZORDER;

		HDWP dwp = BeginDeferWindowPos(10);		

		SOM_MAIN_RTLPOS

		HWND &created = singlewide==r?l:r; 
		int sw = w>doublewide?SW_SHOWNORMAL:SW_HIDE;		
		if(sw) if(!l||!r) 
		{
			SOM_MAIN_cpp::layout &lo = SOM_MAIN_list.right.layout;	
			SOM_MAIN_list::cell &cell = (r?SOM_MAIN_list.left:SOM_MAIN_list.right);			
			cell.listview =	CreateWindowExW(lo->wes,WC_LISTVIEW,0,lo->ws&~WS_VISIBLE,lo.left,lo.top,lo->nc,lo.bottom-lo.top,hwndDlg,(HMENU)1022,0,0);
			created = cell.listview; 
			extern ATOM som_tool_listview(HWND);
			som_tool_listview(created);
		}

		if(sw) //doublewide
		{
			//hack? reset to zero width client area
			if(!IsWindowVisible(created)) if(singlewide==r) 
			lpos.right = lpos.left+SOM_MAIN_list.left.layout->nc;
			else rpos.left = rpos.right-SOM_MAIN_list.right.layout->nc;

			int cy = h-sep.top-sep.bottom;
			int cx = &created==&l?lpos.right-lpos.left:
			w-lpos.left-sep.left-sep.right-(rpos.right-rpos.left);
			lpos.right = lpos.left+cx; lpos.bottom = lpos.top+cy;
			DeferWindowPos(dwp,l,0,lpos.left,lpos.top,cx,cy,z);
			rpos.left = lpos.left+cx+sep.left; 
		}	
		else rpos.left = lpos.left; 		
		
		int xdelta = rpos.right, ydelta = rpos.bottom;
		rpos.right = w-sep.right; rpos.bottom = h-sep.bottom;
		xdelta = rpos.right-xdelta; ydelta = rpos.bottom-ydelta;		
		DeferWindowPos(dwp,sw?(r?r:l):singlewide,0,rpos.left,rpos.top,rpos.right-rpos.left,rpos.bottom-rpos.top,z);				
		
		HWND hw, tw = GetDlgItem(hwndDlg,1201); //trackbar
		RECT rc, tb; GetWindowRect(tw,&tb); MapWindowRect(0,hwndDlg,&tb);
		tb.right+=xdelta;
		int xdelta2 = tb.left;		
		int room = tb.right-sep.left-left_of_trackbar;
		tb.left = tb.right-max(75,min(200,room));
		xdelta2 = tb.left-xdelta2; 
		tb.right-=xdelta; tb.left-=xdelta;
		  
		for(int i=0;i<EX_ARRAYSIZEOF(bts);i++) if(hw=GetDlgItem(hwndDlg,bts[i]))
		{	
			GetWindowRect(hw,&rc); MapWindowRect(0,hwndDlg,&rc);
			OffsetRect(&rc,rc.left>tb.right?xdelta:xdelta2,ydelta);
			DeferWindowPos(dwp,hw,0,rc.left,rc.top,0,0,z|SWP_NOSIZE);
		}

		OffsetRect(&tb,xdelta,ydelta);
		DeferWindowPos(dwp,tw,0,tb.left,tb.top,tb.right-tb.left,tb.bottom-tb.top,z);

		if(!EndDeferWindowPos(dwp)) assert(0);

		if(xdelta) 
		{
			RECT div = {lpos.right,lpos.top,rpos.left,lpos.bottom};
			InvalidateRect(hwndDlg,&div,1);
		}

		InvalidateRect(tw,0,0); //assuming transparent background
		ShowWindow(created,sw);
		break;
	}
	case WM_NCLBUTTONDBLCLK: //todo: somehow merge 152/154/172 
	{
		int sw = IsZoomed(hwndDlg)?SW_RESTORE:SW_MAXIMIZE;
		//ShowWindow immediately bounces back to its restored size 
		ShowWindowAsync(hwndDlg,IsZoomed(hwndDlg)?SW_RESTORE:SW_MAXIMIZE);
		break;
	}	
	case WM_SETCURSOR: case WM_LBUTTONDOWN: //todo: somehow merge 152/154/172
	{
		if(!SOM_MAIN_list[singlewide]->listview) break;

		POINT pt; GetCursorPos(&pt);
		MapWindowPoints(0,hwndDlg,&pt,1);
		RECT &lpos = SOM_MAIN_list.left.layout;
		RECT &rpos = SOM_MAIN_list.right.layout;
		if(pt.x>lpos.right&&pt.x<rpos.left
		 &&pt.y>lpos.top&&pt.y<lpos.bottom) switch(uMsg)
		{	  
		case WM_SETCURSOR:
		
			SetCursor(LoadCursor(0,IDC_SIZEWE));
			return 1;

		case WM_LBUTTONDOWN: 
			
			SetCapture(hwndDlg); x = pt.x; 
			break;
		}
		break;
	}
	case WM_LBUTTONUP: case WM_MOUSEMOVE: //todo: somehow merge 152/154/172
	
		if(GetCapture()==hwndDlg) switch(uMsg)
		{
		case WM_LBUTTONUP: ReleaseCapture(); break;		
		case WM_MOUSEMOVE:

			POINT pt; GetCursorPos(&pt);
			MapWindowPoints(0,hwndDlg,&pt,1);
			int delta = pt.x-x;	x = pt.x; if(!delta) break; 			
			SOM_MAIN_RTLPOS
			if(delta<0)
			{
				lpos.right = max(lpos.right+delta,
				lpos.left+SOM_MAIN_list.left.layout->nc);
				rpos.left = lpos.right+sep.left;
			}
			else
			{
				rpos.left = min(rpos.left+delta,
				rpos.right-SOM_MAIN_list.right.layout->nc);
				lpos.right = rpos.left-sep.left;
			}			
			RECT div = {lpos.right,lpos.top,rpos.left,lpos.bottom};
			InvalidateRect(hwndDlg,&div,1);
			for(int i=0;i<2;i++) 
			{
				HWND &lr = i?l:r,&rl=i?r:l; RECT &lrc = i?lpos:rpos;				
				MoveWindow(lr,lrc.left,lrc.top,lrc.right-lrc.left,lrc.bottom-lrc.top,1);
				if(lrc.right-lrc.left>SOM_MAIN_list.right.layout->nc)
				{
					if(SOM_MAIN_list[lr].empty())
					{
						HWND f = GetFocus(); SetFocus(rl);
						SendMessage(hwndDlg,WM_LBUTTONDBLCLK,0,MAKELPARAM(div.left,div.top));
						ListView_SetItemState(lr,0,~0,LVIS_FOCUSED); //|LVIS_SELECTED
						SetFocus(f);
					}
				}	
				else if(!SOM_MAIN_list[lr].empty())
				{
					SOM_MAIN_list[lr].clear();
					SetWindowTextW(hwndDlg,SOM_MAIN_label(rl,SOM_MAIN_list[rl].entrypoint));				
					int dis[] = {1080,1082}; 
					windowsex_enable(hwndDlg,dis,0);
					if(GetFocus()==lr) SetFocus(rl);
					EnableWindow(lr,0);
				}
			}
			break;
		}	
		break;

	case WM_LBUTTONDBLCLK:
	{
		HWND lv = //open up?
		SOM_MAIN_list[singlewide]->listview;
		if(!lv) break;
		RECT &lpos = SOM_MAIN_list.left.layout;
		RECT &rpos = SOM_MAIN_list.right.layout;
		POINT pt = {GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};		
		if(pt.x<lpos.right||pt.x>rpos.left
		 ||pt.y<lpos.top||pt.y>lpos.bottom) break;
		if(lv!=GetFocus()) lv = singlewide;
		HWND vl = SOM_MAIN_list[lv]->listview;
		RECT cr; GetWindowRect(vl,&cr);
		if(!cr.right) return MessageBeep(-1);
		HWND tv = SOM_MAIN_list[lv].corresponding_treeview();
		HWND vt = SOM_MAIN_list[vl].corresponding_treeview();
		LPARAM lp = SOM_MAIN_param(tv,SOM_MAIN_list[lv].entrypoint);
		LPARAM tlp = SOM_MAIN_toplevelparam(tv,SOM_MAIN_list[lv].entrypoint);				
		SOM_MAIN_tree::pool::iterator match = SOM_MAIN_tree[vt].find(SOM_MAIN_tree[tlp]);
		if(lp!=tlp&&!TreeView_GetChild(vt,(*match)[vt]))
		SOM_MAIN_outline(vt,(*match)[vt],SOM_MAIN_tree.string_param(*match));
		SOM_MAIN_tree::pool::iterator match2 = SOM_MAIN_tree[vt].find(SOM_MAIN_tree[lp]);
		HTREEITEM gp, ep = (match2!=SOM_MAIN_tree[vt].end()?*match2:*match)[vt];
		while(lp==SOM_MAIN_param(vt,gp=TreeView_GetParent(vt,ep))) ep = gp;
		if(ep==SOM_MAIN_list[vl].entrypoint||!SOM_MAIN_154_enter(vl,ep))
		if(GetCapture()!=hwndDlg) MessageBeep(-1);
		int en[] = {1080,1082}; 
		windowsex_enable(hwndDlg,en);
		EnableWindow(vl,1);
		break;
	}
	case WM_DRAWITEM: //REMOVE ME?
	{				 			  		
		return som_tool_drawitem((DRAWITEMSTRUCT*)lParam,156);
	}
	case WM_NOTIFY:
	{
		NMLVDISPINFOW *p = (NMLVDISPINFOW*)lParam;

		HWND &lv = p->hdr.hwndFrom;

		switch(p->hdr.code)
		{
		case LVN_GETDISPINFOW:
		{
			if(~p->item.mask&LVIF_TEXT) break;						
			bool r = lv==SOM_MAIN_list.right.listview;
			HWND tv = SOM_MAIN_list[lv].corresponding_treeview();
			LPARAM lp = SOM_MAIN_param(tv,(HTREEITEM)p->item.lParam);
			SOM_MAIN_list.aspects.reselect(lp);
			SOM_MAIN_list::aspect &aspect = 
			SOM_MAIN_list[SOM_MAIN_list[lv].columns154[p->item.iSubItem]];
			if(aspect.selected) 
			if(aspect.gettext_like()) switch(aspect[0])
			{
			case '\0': //Text
					
				EX::need_unicode_equivalent(65001,aspect.selected,p->item.pszText,p->item.cchTextMax);
				break;
			
			case '\1': //Context

				const char *_4 = strchr(aspect.selected,'\4');
				int zt = MultiByteToWideChar(65001,0,aspect.selected,_4-aspect.selected,p->item.pszText,p->item.cchTextMax-1);
				p->item.pszText[zt] = '\0';
				break;
			}
			else if(&aspect==&SOM_MAIN_list.aspects.exml.li)
			{
				HTREEITEM i = (HTREEITEM)p->item.lParam;
				static std::vector<int> ol(5); ol.clear();				
				for(LPARAM cut=lp;lp=SOM_MAIN_param(tv,i=TreeView_GetParent(tv,i));cut=lp)
				ol.push_back(cut==lp?1:SOM_MAIN_tree[cut].outline); 
				ol.back() = -ol.back(); 
				for(size_t _s,i=ol.size(),j=0;i;i--)
				{
					_s = p->item.cchTextMax-j-1; if(_s<32) //NEW
					{
						wchar_t w[32] = L""; _ltow(-ol[i-1],w,10);
						wcsncpy(p->item.pszText+j,w,_s)[_s] = '\0';
					}
					else _ltow_s(-ol[i-1],p->item.pszText+j,_s,10);
					while(p->item.pszText[j]) j++;
				}				
			}
			else //todo: interperet data types
			{
				EXML::Quote q(aspect.selected); 
				int zt = SOM_MAIN_AttribToWideChar(q.value,q.length(),p->item.pszText,p->item.cchTextMax-1);
				p->item.pszText[zt] = '\0';

				//hack: assume clock data type
				if(&aspect==&SOM_MAIN_list.aspects.exml.tm
				 ||&aspect==&SOM_MAIN_list.aspects.exml.tc) 
				{
					static HWND dt = //GetWindowTextW
					CreateWindowExW(0,L"SysDateTimePick32",0,0,0,0,0,0,SOM_MAIN_modes[0],0,0,0);
					SYSTEMTIME st; if(SOM_MAIN_windowstime(&st,p->item.pszText))
					{
						DateTime_SetSystemtime(dt,0,&st);
						SetWindowLong(dt,GWL_STYLE,DTS_SHORTDATECENTURYFORMAT);
						zt = GetWindowTextW(dt,p->item.pszText,p->item.cchTextMax-2);
						if(zt) p->item.pszText[zt++] = ' ';
						if(zt) SetWindowLong(dt,GWL_STYLE,DTS_TIMEFORMAT);
						if(zt) zt+=GetWindowTextW(dt,p->item.pszText+zt,p->item.cchTextMax-zt-1);
						if(zt) p->item.pszText[zt] = '\0';
					}
				}
			}
			break;			
		}
		case NM_SETFOCUS:

			SOM_MAIN_list = lv;
			break;

		case LVN_COLUMNCLICK:
		{
			//POINT OF NO RETURN
			//save focus in case background sorting
			HWND refocus = SOM_MAIN_list->listview;
			SOM_MAIN_list = lv; 
			int col = ((NMLISTVIEW*)lParam)->iSubItem;
			SOM_MAIN_list::aspect &aspect = 
			SOM_MAIN_list[SOM_MAIN_list->columns154[col]];
			//could check sign for ascending/descending order
			SOM_MAIN_list->sortedby = col;
			if(aspect.gettext_like())
			ListView_SortItems(lv,SOM_MAIN_sortbyText,aspect[0]);
			else //this solution is for one release only
			{
			int todolist[SOMEX_VNUMBER<=0x1020504UL];			
			EXML::Attribs lp = EXML::attribs(aspect.c_str()); 
			switch(lp)
			{
			case 0: case 1: //unimplemented				
				assert(0); MessageBeep(-1); 
				break;
			case EXML::Attribs::li:
				ListView_SortItemsEx(lv,SOM_MAIN_sortbyItemEx,(LPARAM)lv);
				break;
			case EXML::Attribs::hl:
				ListView_SortItems(lv,SOM_MAIN_sortbyReal,(LPARAM)lp);
				break;
			case EXML::Attribs::tc: case EXML::Attribs::tm:
				ListView_SortItems(lv,SOM_MAIN_sortbyTime,(LPARAM)lp);
				break;
			default: ListView_SortItems(lv,SOM_MAIN_sortbyText,(LPARAM)lp);
			}}			
			if(!IsWindowEnabled(hwndDlg))
			SOM_MAIN_list = refocus;
			break;
		}
		case LVN_ITEMACTIVATE: 
		{
			HTREEITEM ti = //full row implementation
			SOM_MAIN_154_treeitem(lv,((NMITEMACTIVATE*)lParam)->iItem);
			if(!ti) return MessageBeep(-1); //paranoia?			
			HWND tv = SOM_MAIN_list[lv].underlying_treeview();
			HWND tv2 = SOM_MAIN_list[lv].corresponding_treeview();
			TreeView_SelectItem(tv,SOM_MAIN_reproduceitem(tv,tv2,ti));
			SOM_MAIN_(153,hwndDlg,SOM_MAIN_153,SOM_MAIN_param(tv2,ti));
			return 0;
		}
		case LVN_BEGINLABELEDIT: f2: 
		{				
			if(GetFocus()==lv) //f2: prevent double firing?
			{
				SOM_MAIN_172(hwndDlg,0); //will queue things up
			}
			SetWindowLong(hwndDlg,DWL_MSGRESULT,1);
			return 1;
		}
		case NM_RETURN: 

			assert(0); //unsent: using IDOK for now
			SetWindowLong(hwndDlg,DWL_MSGRESULT,1);
			return 1;

		case LVN_KEYDOWN: 
		{
			int vk = ((NMLVKEYDOWN*)lParam)->wVKey;

			if(vk>='0'&&vk<='9')
			{
				int pos = vk=='0'?1000:vk-'1';
				SendDlgItemMessage(hwndDlg,1201,TBM_SETPOS,1,pos);
			}
			else switch(vk)
			{
			case VK_F2: goto f2; 

			case VK_ESCAPE:	case VK_BACK: up:
			{	
				//shift here is just in case `~ and \| are unavailable
				bool r = lv==GetFocus()&&!SOM_MAIN_list.right.empty();
				if(GetKeyState(VK_SHIFT)>>15) return 
				SendMessage(hwndDlg,WM_COMMAND,MAKEWPARAM(+1080+r,0),0);
				SendMessage(hwndDlg,WM_COMMAND,MAKEWPARAM(-1080-r,0),0);
				break;
			}
			case VK_OEM_3: //grave accent/tilde (left of 1 on US keyboards)
			case VK_OEM_5: //backslash/vertical bar (often below baskspace)
			{
				bool r = lv==GetFocus()&&!SOM_MAIN_list.right.empty();
				if(GetKeyState(VK_SHIFT)>>15) return
				SendMessage(hwndDlg,WM_COMMAND,MAKEWPARAM(-1080-r,0),0);
				SendMessage(hwndDlg,WM_COMMAND,MAKEWPARAM(+1080+r,0),0);
				break;
			}
			case VK_INSERT:
			{
				HWND tv = SOM_MAIN_list[lv].corresponding_treeview();
				HTREEITEM inplaceof = SOM_MAIN_154_treeitem(lv), after =
				inplaceof?TreeView_GetParent(tv,inplaceof):SOM_MAIN_list[lv].entrypoint;
				return SOM_MAIN_insert(tv,after,inplaceof);
			}
			case VK_DELETE: final_cut: //'X'
			{
				HWND vl = SOM_MAIN_list[lv]->listview;
				LVFINDINFO lfi = {vl?LVFI_PARAM:0,0,0};				
				HWND tv = SOM_MAIN_list[lv].corresponding_treeview();
				if(SOM_MAIN_readonly(tv)) return MessageBeep(-1); int beep = 1;
				if(tv!=SOM_MAIN_list[vl].corresponding_treeview()) lfi.flags = 0;				
				for(int i=-1,j;-1!=(i=ListView_GetNextItem(lv,i,LVNI_SELECTED));)
				{
					if(LVIS_CUT&ListView_GetItemState(lv,i,~0)) 
					continue; else beep = 0;
					HTREEITEM ti = SOM_MAIN_154_treeitem(lv,i);
					if(vk==VK_DELETE) //cutting?
					if(!SOM_MAIN_delete(tv,ti,IDNO)) continue; //japanese?
					if(lfi.flags) //side-by-side
					{ lfi.lParam = (LPARAM)ti; j = ListView_FindItem(vl,j,&lfi); }
					for(int l=0;l<=int(lfi.flags&&j>-1);l++)
					{
						HWND lvl = l?vl:lv; int &ijl = l?j:i; 
						if(SOM_MAIN_param(tv,ti))
						{ListView_SetItemState(lvl,ijl,~0,LVIS_CUT); //{}: broken macro???
						}else do ListView_DeleteItem(lvl,ijl--); 
						while(ijl>-1&&!SOM_MAIN_param(tv,SOM_MAIN_154_treeitem(lvl,ijl)));
					}
				}
				if(beep) return MessageBeep(-1);
				SOM_MAIN_list[lv].sort(); if(lfi.flags) SOM_MAIN_list[vl].sort();
				break;
			}
			case 'X': case 'C': case 'V': case 'B':
			
				if(GetKeyState(VK_CONTROL)>>15)
				if(1==ListView_GetSelectedCount(lv))
				{
					int i = ListView_GetNextItem(lv,-1,LVNI_SELECTED);
					HWND tv = SOM_MAIN_list[lv].underlying_treeview();
					HWND tv2 = SOM_MAIN_list[lv].corresponding_treeview();					
					HTREEITEM ti = SOM_MAIN_reproduceitem(tv,tv2,SOM_MAIN_154_treeitem(lv,i));
					if(TreeView_SelectItem(tv,ti))
					{
						if(vk=='V'||vk=='B') SOM_MAIN_tree[tv].paste();
						if(vk!='V'&&vk!='B'&&SOM_MAIN_tree[tv].copy()&&vk=='X') goto final_cut;
					}
					else SOM_MAIN_error("MAIN199",MB_OK); //not expecting this
				}
				else SOM_MAIN_report(vk=='X'||vk=='C'?"MAIN215":"MAIN216",MB_OK);
				break;
			}
			break;
		}
		case LVN_ITEMCHANGED:
		{
			NMLISTVIEW *p = (NMLISTVIEW*)lParam;

			if(p->uChanged&LVIF_STATE)
			if((p->uNewState&LVIS_SELECTED)!=(p->uOldState&LVIS_SELECTED))
			SOM_MAIN_list[lv].bookmark = 0;
			break;
		}}
		break;
	}	
	case WM_HSCROLL: //trackbar
	{
		if(1201==GetWindowID((HWND)lParam))
		if(!SOM_MAIN_154_slide(SOM_MAIN_list.right.listview)
		  &!SOM_MAIN_154_slide(SOM_MAIN_list.left.listview)) MessageBeep(-1);
		break;
	}
	case WM_COMMAND:

		if(!HIWORD(wParam)) 
		switch((SHORT&)wParam) //tickers
		{
		case -1080: case +1080: case -1081: case +1081: 
		{
			bool r = wParam&1&&!SOM_MAIN_list.right.empty();
			HWND lv = (r?SOM_MAIN_list.right:SOM_MAIN_list.left).listview;
			HWND tv = SOM_MAIN_list[lv].corresponding_treeview();
			HTREEITEM ep = SOM_MAIN_list[lv].entrypoint;
			ep = ~wParam&0x8000?SOM_MAIN_154_treeitem(lv):
			TreeView_GetParent(tv,SOM_MAIN_list[lv].entrypoint);
			if(!SOM_MAIN_154_enter(lv,ep)) MessageBeep(-1);
			break;
		}
		case -1082: case +1082: case -1083: case +1083: 
		{
			bool closing = (HWND)lParam==hwndDlg; //hack
			bool r = wParam&1&&!SOM_MAIN_list.right.empty();
			HWND lv = (r?SOM_MAIN_list.right:SOM_MAIN_list.left).listview;
			HWND tv = SOM_MAIN_list[lv].underlying_treeview();
			HWND tv2 = SOM_MAIN_list[lv].corresponding_treeview();
			HTREEITEM &bookmark = SOM_MAIN_list[lv].bookmark;
			if(!bookmark)
			{
				if(0x8000&wParam) //f2
				{
					SOM_MAIN_list = lv;
					SOM_MAIN_172(hwndDlg,0); break;
				}
				int sc = ListView_GetSelectedCount(lv); if(closing)
				{
					SetWindowRedraw(lv,0);
					if(!sc&&ListView_GetSelectedCount(SOM_MAIN_list[lv]->listview))
					return 0; //ignoring
					if(SOM_MAIN_list[lv].sortedby&&sc!=1) SOM_MAIN_list[lv].sort(0);
				}
				if(!sc) ListView_SetItemState(lv,-1,~0,LVIS_SELECTED);
				int bookmark = -1;
				LVITEMW lvi = {LVIF_PARAM,-1,0}; 
				HTREEITEM &ti = (HTREEITEM&)lvi.lParam; 
				for(int &i=lvi.iItem;(i=ListView_GetNextItem(lv,i,LVNI_SELECTED))>-1;)
				{
					ListView_GetItem(lv,&lvi); 
					SOM_MAIN_cascade(tv,SOM_MAIN_reproduceitem(tv,tv2,ti));					
					if(bookmark==-1) bookmark = i;
				}				
				if(bookmark!=-1)
				SOM_MAIN_154_bookmark(lv,bookmark);
				else if(!closing) MessageBeep(-1); 
				SetWindowLong(hwndDlg,DWL_MSGRESULT,bookmark); return 1;
			}
			else if(!closing) //scrolling with tickers
			{
				int pos = GetScrollPos(tv,SB_VERT);	
				int neg = 0x8000&wParam?LVNI_ABOVE:0;
				SendMessage(tv,WM_VSCROLL,neg?SB_LINEUP:SB_LINEDOWN,0);
				if(pos!=GetScrollPos(tv,SB_VERT))
				{
					HTREEITEM gp,gc,fv = 
					gc=gp=TreeView_GetFirstVisible(tv);
					while(gp!=bookmark&&gp)	gp = TreeView_GetParent(tv,gp);
					if(gp==bookmark)				
					while(fv=TreeView_GetChild(tv,fv)) gc = fv;
					if(SOM_MAIN_textarea(tv,gc).param==SOM_MAIN_param(tv,bookmark))
					break;
				}
				int i = ListView_GetNextItem(lv,-1,LVNI_FOCUSED);				
				i = ListView_GetNextItem(lv,i,neg|LVNI_SELECTED);
				if(i==-1) if(MessageBeep(-1),neg) //winsanity
				{
					i = ListView_GetItemCount(lv);
					if(~ListView_GetItemState(lv,--i,~0)&LVIS_SELECTED)
					i = ListView_GetNextItem(lv,i,neg|LVNI_SELECTED);
				}
				else i = ListView_GetNextItem(lv,-1,LVNI_SELECTED);
				SOM_MAIN_154_bookmark(lv,i);
			}
			break;
		}}

		switch(wParam)
		{
		case 1212: begin: //IDOK
		{
			SOM_MAIN_list::cell *p = &SOM_MAIN_list.right;
			if(SOM_MAIN_list->listview==GetFocus()) p = SOM_MAIN_list.focus;
			HWND tv = (p->empty()?p=&SOM_MAIN_list.left:p)->corresponding_treeview();
			HTREEITEM ti = SOM_MAIN_154_treeitem(p->listview);			
			return SOM_MAIN_create(tv,p->entrypoint,ti);
		}
		case IDOK: //Enter key?
		{				
			//treeviews don't have DLGC_WANTALLKEYS
			if(SOM_MAIN_list->listview==GetFocus())
			{
				if(~GetKeyState(VK_SHIFT)>>15) goto begin;			
				bool read = SOM_MAIN_list.focus==&SOM_MAIN_list.right;
				return SendMessage(hwndDlg,WM_COMMAND,1082+read,0); 
			}
			else //hack: disabling default button
			{
				if(GetKeyState(VK_RETURN)>>15) break; 
			}
			int beep = -1;
			if(IsWindowEnabled(GetDlgItem(hwndDlg,1082)))
			beep&=SendMessage(hwndDlg,WM_COMMAND,1082,(LPARAM)hwndDlg);
			beep&=SendMessage(hwndDlg,WM_COMMAND,1083,(LPARAM)hwndDlg);
			if(beep==-1) return MessageBeep(-1);
			goto close;
		}
		case IDCANCEL: //Esc key

			if(SOM_MAIN_list->listview==GetFocus()) escape:
			{
				HWND tv = SOM_MAIN_list->corresponding_treeview();
				HTREEITEM ep = TreeView_GetParent(tv,SOM_MAIN_list->entrypoint);
				if(ep||wParam!=IDCANCEL) 
				{
					if(ep) SOM_MAIN_154_enter(SOM_MAIN_list->listview,ep);
					else MessageBeep(-1);
					break;
				}
			}	
			//break;

		case IDCLOSE: goto close;
		}
		break;
	
	case WM_NCDESTROY:

		SOM_MAIN_list.left.clear();
		SOM_MAIN_list.left.listview = 0;
		SOM_MAIN_list.right.clear();
		SOM_MAIN_list.right.listview = 0;
		break;

	case WM_CLOSE: close:
		
		DestroyWindow(hwndDlg); 	
	}
	return 0;
}
static bool SOM_MAIN_154_enter(HWND lv, HTREEITEM ep)
{
	if(!lv) return false;	
	SOM_MAIN_list[lv].bookmark = 0;	
	SOM_MAIN_list::cell &cell = SOM_MAIN_list[lv];		
	if(ep==TVI_ROOT) ep = cell.entrypoint; //154_slide
	if(!ep) return false;	

	bool out = false, sort = false;
	bool visible = IsWindowVisible(lv);		
	bool empty = //cell.empty(); //hmm??
	!Header_GetItemCount(ListView_GetHeader(lv));
						
	//note: if initializing the outline is opened fully
	//both to ensure all columns appear and for sorting
	//by things like the modification time conveniently

	HWND gp = GetParent(lv);	
	HWND tb = GetDlgItem(gp,1201);
	HWND tv = SOM_MAIN_list[lv].corresponding_treeview();
	int limit = SendMessage(tb,TBM_GETPOS,0,0);	
	
	if(empty) //initializing
	{		
		if(visible) //must be opening opposite panel
		{
			static int round = 1; if(1==round++)
			{
				SOM_MAIN_154_enter(lv,SOM_MAIN_toplevelitem(tv,ep));	
				SOM_MAIN_154_enter(lv,ep);
				return round = 1;
			}
		}
		else SendMessage(tb,TBM_SETRANGE,1,0);
		LPARAM param = SOM_MAIN_param(tv,ep);
		SOM_MAIN_outline(tv,ep,param); 
		limit = SOM_MAIN_tree[tv].outlines[SOM_MAIN_tree[param].outline].levels-1;		
		if(limit>SendMessage(tb,TBM_GETRANGEMAX,0,0)) 
		SendMessage(tb,TBM_SETRANGE,1,MAKEWPARAM(0,limit));
		ListView_SetExtendedListViewStyle(lv,LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT|LVS_EX_LABELTIP|LVS_EX_HEADERDRAGDROP);								
		for(SOM_MAIN_list::aspects::foreach ea;ea;ea++)
		ea->column154 = 0; cell.columns154.clear(); 
		const SOM_MAIN_list::aspect *basics[] = 
		{
		&SOM_MAIN_list.aspects.exml.li, //0		
		&SOM_MAIN_list.aspects.msgid,
		&SOM_MAIN_list.aspects.msgctxt, //last
		};
		size_t basics_s = EX_ARRAYSIZEOF(basics);
		basics_s-=!SOM_MAIN_tree[param].nocontext(); 
		for(size_t i=0;i<basics_s;basics[i]->column154=i,i++) 
		cell.columns154.push_back(SOM_MAIN_list.aspect_param(*basics[i]));			
		out = true;
	}
	
	int level = SOM_MAIN_level(tv,ep);	
	int entrylevel = 1+level;
	int entrylimit = entrylevel+limit;
	//hack: trackbar noises
	if(cell.entrypoint==ep)
	if(cell.entrylevel==entrylevel)
	if(cell.entrylimit==entrylimit) return true;
	
	LVITEMW lvi = {LVIF_PARAM,0,0,0,~0}; 
	for(int i=0,j,n=ListView_GetItemCount(lv);i<n;lvi.iItem=++i)
	{
		ListView_GetItem(lv,&lvi);
		HTREEITEM item = (HTREEITEM)lvi.lParam;
		for(j=0;item=TreeView_GetParent(tv,item);j++)
		if(item==ep) 
		lvi.lParam = 0;
		if(!lvi.lParam&&j>=entrylevel&&j<=entrylimit) 
		continue;
		ListView_DeleteItem(lv,i--); n--;
		out = true;
	}	
	lvi.iItem = 0; //initializing
	lvi.mask|=LVIF_TEXT|LVIF_STATE;
	lvi.pszText = LPSTR_TEXTCALLBACKW;
	TVITEMEXW tvi = {TVIF_CHILDREN|TVIF_STATE,0,0,~0};
	HTREEITEM &i = tvi.hItem, ii;
	if(ii=TreeView_GetChild(tv,ep)) 
	if(cell.entrylevel>entrylevel||cell.entrylimit<entrylimit)
	for(i=ii;ii!=ep;i=level>=entrylimit?0:TreeView_GetChild(tv,ii))
	{
		if(i) ii=i,level++;
		else while(!i&&ii!=ep) textarea:
		if(i=TreeView_GetNextSibling(tv,ii)) ii = i;
		else ii	= TreeView_GetParent(tv,ii),level--;
		//if true this item should already be represented
		//(does that mean our walk could be simplified??)
		if(level>=cell.entrylevel&&level<=cell.entrylimit) 
		{
			HTREEITEM j = i; //TODO: OPTIMIZE BY TRACKING
			//WHETHER OR NOT WE ARE BELOW cell.entrypoint
			while((j=TreeView_GetParent(tv,j))&&j!=cell.entrypoint);
			if(j==cell.entrypoint) continue;
		}
		if(level>=entrylevel) assert(i); else break;
		if(TreeView_GetItem(tv,&tvi)&&!tvi.cChildren) //textarea?
		{
			ii = TreeView_GetParent(tv,i),level--; goto textarea;
		}
		if(empty) SOM_MAIN_list.aspects.select
		(SOM_MAIN_param(tv,i),&cell.columns154); //initializing
		lvi.lParam = (LPARAM)i; 		
		lvi.state = tvi.state&LVIS_CUT;
		lvi.iItem = ListView_InsertItem(lv,&lvi)+1;
		out = sort = true;
	}
	SOM_MAIN_list.aspects.unselect(); //paranoia

	if(empty) //initializing
	{
		LVCOLUMNW lvc = {LVCF_TEXT|LVCF_WIDTH,0,0};		
		size_t c = 2==SOM_MAIN_list.aspects.msgctxt.column154;
		for(size_t i=0;i<cell.columns154.size();i++)
		{
			lvc.cx = i<=2+c?90:80; //hack: fine tuning
			lvc.pszText = (wchar_t*)
			SOM_MAIN_list[cell.columns154[i]].description.c_str();
			ListView_InsertColumn(lv,i,&lvc);
		}						
		//todo: work in dialog units/etc?
		ListView_SetColumnWidth(lv,0,70);
		//todo: measure to the width of description[0]
		if(SOM_MAIN_list.aspects.msgctxt.column154==2)		
		ListView_SetColumnWidth(lv,2,19);

		int lc = //hack: promoting to column 2
		SOM_MAIN_list.aspects.exml.l.column154;
		SOM_MAIN_list.aspects.exml.l.column154 = 0;
		static std::vector<int> coa; coa.clear();
		coa.push_back(0); if(lc) coa.push_back(lc);
		if(SOM_MAIN_list.aspects.msgctxt.column154==2)
		coa.push_back(2); coa.push_back(1); //ctxt/id
		SOM_MAIN_list::aspects::iterator it;
		SOM_MAIN_list::aspectset::iterator jt;
		SOM_MAIN_list::aspects::exml::iterator kt;		
		it = SOM_MAIN_list.aspects.begin();		
		for(kt=SOM_MAIN_list.aspects.exml.begin();*kt;kt++)
		if((*kt)->column154) coa.push_back((*kt)->column154);
		for(it++;it!=SOM_MAIN_list.aspects.end();it++)
		for(jt=it->second.begin();jt!=it->second.end();jt++)			
		if(!jt->EXML_important())
		if(jt->column154) coa.push_back(jt->column154);
		ListView_SetColumnOrderArray(lv,coa.size(),&coa[0]);
		SOM_MAIN_list.aspects.exml.l.column154 = lc;
		if(!visible)
		SendMessage(tb,TBM_SETPOS,1,limit);
	}
	if(ep==TreeView_GetParent(tv,cell.entrypoint))
	{
		//focus on the item being escaped from
		LVFINDINFO lfi = {LVFI_PARAM,0,(LPARAM)cell.entrypoint};
		int i = ListView_FindItem(lv,-1,&lfi);
		ListView_SetSelectionMark(lv,i); //thwart multi-selection
		ListView_SetItemState(lv,i,1,LVIS_FOCUSED|LVIS_SELECTED);
		ListView_EnsureVisible(lv,i,0);
	}
	cell.entrylevel = entrylevel; 
	cell.entrylimit = entrylimit; 
	cell.entrypoint = ep; 
	
	SOM_MAIN_list::cell &l = 
	SOM_MAIN_list.left, &r = SOM_MAIN_list.right;	
	enum{sep_s=20}; static wchar_t sep[sep_s] = L"";
	if(!*sep&&!GetWindowTextW(gp,sep,sep_s)) wcscpy(sep,L"  ][  ");
	if(!l.empty()&&!r.empty())
	{	
		wchar_t lr[260+sep_s+260]; 
		wcscpy(lr,SOM_MAIN_label(l.corresponding_treeview(),l.entrypoint));		
		wchar_t *opposite = SOM_MAIN_label(r.corresponding_treeview(),r.entrypoint);
		size_t cat = wcslen(lr); 
		if(wmemcmp(lr,opposite,cat+1)) 
		{
			int ltrim = 0, rtrim = 0;
			size_t seplen = wcslen(sep);
			//could try to determine if proportional
			//NONCLIENTMETRICSW ncm = {sizeof(ncm)};
			//SystemParametersInfoW(SPI_GETNONCLIENTMETRICS,sizeof(ncm),ncm,0);
			if(cat) switch(lr[cat-1]) //full-width brackets
			{
			case 0xFF63: case 0x3009: case 0x300B: case 0x300D: case 0x300F: 
			case 0x3011: case 0xFF09: case 0xFF3D: case 0xFF1E:	case 0xFF5D:

				if(*sep==' ') ltrim = 1;			
			}
			switch(*opposite) //opening full-width brackets
			{
			case 0xFF62: case 0x3008: case 0x300A: case 0x300C: case 0x300E: 
			case 0x3010: case 0xFF08: case 0xFF3B: case 0xFF1C:	case 0xFF5B:

				if(sep[seplen-1]==' ') rtrim = 1;
			}
			wcscpy(wcscpy(lr+cat,sep+ltrim)+seplen-rtrim-ltrim,opposite);		
		}
		SetWindowTextW(gp,lr);
	}
	else SetWindowTextW(gp,SOM_MAIN_label(tv,ep));	

	if(sort&&!empty) cell.sort(); return out;
}
static void SOM_MAIN_154_bookmark(HWND lv, int i)
{	 	
	//would prefer it end up on top
	//if/when scrolling is required
	ListView_EnsureVisible(lv,i,0);
	HTREEITEM ti = SOM_MAIN_154_treeitem(lv,i);
	ListView_SetItemState(lv,i,~0,LVIS_FOCUSED);
	
	HWND tv = SOM_MAIN_list[lv].underlying_treeview();
	HWND tv2 = SOM_MAIN_list[lv].corresponding_treeview();
	ti = SOM_MAIN_reproduceitem(tv,tv2,ti); 
	SOM_MAIN_list[lv].bookmark = ti;

	//these do different things
	TreeView_SelectItem(tv,ti);
	TreeView_SelectSetFirstVisible(tv,ti);
}

extern const char *SOM_MAIN_generate()
{
	GUID guid; CoCreateGuid(&guid); 
	static char base64[24]; int len64 = 24;			
	som_atlenc_Base64Encode((BYTE*)&guid,sizeof(guid),base64,&len64);
	base64[22] = '\0'; assert(len64==22); return base64;
}
			
static HWND SOM_MAIN_17X_textarea;
extern ATOM som_tool_richtext(HWND);
extern void som_tool_extendtext(HWND,int=0);
static LRESULT CALLBACK SOM_MAIN_170(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scID, DWORD_PTR readonly)
{	
	HWND &ta = SOM_MAIN_17X_textarea;

	switch(uMsg)
	{
	case WM_INITDIALOG: ta = 0;
	{	
		int ro[] = {1000,1080};
		windowsex_enable(hWnd,ro,!readonly);
		int guess = 0, guess2 = 0;
		int compile[sizeof(long)==4];
		const char *id = SOM_MAIN_tree[lParam].id;
		if(strchr(id,'\4')==id+4)
		switch(_byteswap_ulong(*(long*)id))
		{
		case 'MAP0': //Maps	
		case 'MAP1': //Events

			guess = 1211; break;

		case 'MAP2': //Programs

			guess = 1212; guess2 = 1214; break;

		case 'MAP3': //Counters

			guess = 1211; break;

		case 'PRM0': //Shop Items
		case 'PRM1': //Shop Memory
		case 'PRM2': //Techniques
		case 'PRM3': //Components
		case 'PRM4': //Opponents
		case 'PRM5': //Complements

			guess = 1211; guess2 = 1213; break;

		case 'SYS0': //Titles

			guess = 1210; break;

		case 'SYS1': //Captions

			guess = 1212; break;			

		case 'SYS2': //Segments

			guess = 1215; break;
		}		
		if(*SOM_MAIN_list->quoted
		  ||SOM_MAIN_153_1009_clientext) 
		{
			if(!guess) guess = 1215; 
			if(IsWindow(SOM_MAIN_153_1009_clientext)
			 &&ES_MULTILINE&GetWindowStyle(SOM_MAIN_153_1009_clientext)
			 &&guess2) guess = guess2; else
			if(guess2&&wcschr(SOM_MAIN_list->quoted,'\n'))
			guess = guess2;
			//SetFocus(GetDlgItem(hWnd,guess));				
			SetTimer(GetDlgItem(hWnd,guess),'init',0,SOM_MAIN_focus);
		}
		else if(!readonly) //generate
		som_tool_highlight(hWnd,1000);			
		else assert(0);				
		return 0;
	}	
	case WM_COMMAND:
		
		if(lParam==(LPARAM)ta)
		{
			switch(HIWORD(wParam))
			{
			case EN_CHANGE:

				//5: CRLF/surrogate pairs
				wchar_t test[5]; if(Edit_GetModify(ta))
				windowsex_enable<IDOK>(hWnd,GetWindowTextW(ta,test,5));

				if(SOM::MO::view)
				if(SOM_MAIN_153_1009_clientext)
				if(SOM::MO::view->pastespecial_2ED_clipboard)
				SetTimer(hWnd,'2ED',500,SOM_MAIN_153_1009_pastespecial);
				break;
			}
		}
		else switch(wParam)
		{
		case IDOK: //change to edited text
			
			SOM_MAIN_153_1009_text = 0; 			
			if(ta&&SOM_MAIN_153_1009_clientext)
			{
				KillTimer(hWnd,'2ED');
				SOM_MAIN_153_1009_pastespecial(hWnd,0,0,0);
			}
			break;

		case IDCANCEL:
		case WM_CLOSE: //trigger EN_CHANGE

			if(ta&&SOM_MAIN_153_1009_clientext)
			{					
				Edit_SetText(ta,SOM_MAIN_list->quoted);
				KillTimer(hWnd,'2ED');
				SOM_MAIN_153_1009_pastespecial(hWnd,0,0,0);
			}
			break;

		case 1000: //generate
		{
			if(!ta) 
			{
				SetFocus(GetDlgItem(hWnd,1215)); assert(ta);
				SetFocus((HWND)lParam);
			}
			if(ES_READONLY&GetWindowStyle(ta)) return MessageBeep(-1);
			SendMessageA(ta,EM_REPLACESEL,1,(LPARAM)SOM_MAIN_generate());
			int sel = LOWORD(Edit_GetSel(ta));
			Edit_SetSel(ta,sel-22,sel);
			break;
		}
		case 1200: case 1201: case 1202: case 1203: case 1204: case 1205: 
		{
			//what on earth is happening here?
			if(!Button_GetCheck((HWND)lParam)) break; //winsanity
			SetWindowRedraw(hWnd,0);
			HWND box = GetDlgItem(hWnd,wParam+10); if(!ta)
			{
				int ex = GetWindowExStyle(box)&
				(WS_EX_CLIENTEDGE|WS_EX_WINDOWEDGE
				|WS_EX_RIGHT|WS_EX_RTLREADING|WS_EX_LEFTSCROLLBAR);
				const wchar_t *text = SOM_MAIN_list->quoted;
				int es = GetWindowStyle(box)&ES_RIGHT;
				es|=WS_CHILD|WS_TABSTOP|WS_HSCROLL|WS_VSCROLL|WS_VISIBLE;
				es|=ES_MULTILINE|ES_WANTRETURN|ES_SAVESEL|ES_NOOLEDRAGDROP|ES_NOHIDESEL;								
				ta = CreateWindowExW(ex,som_tool_richedit_class(),0,es,0,0,1,1,hWnd,0,0,0);	
				som_tool_richtext(ta); 
				if(readonly) Edit_SetReadOnly(ta,1);				
				Edit_SetText(ta,text); //w/URLs
				if(SOM_MAIN_153_1009_clientext)
				windowsex_enable<IDOK>(hWnd,*text?1:0);
				Edit_SetSel(ta,0,-1);
												
				if(SOM::MO::view) 
				if(SOM_MAIN_153_1009_clientext)
				SOM::MO::view->pastespecial_2ED_clipboard = ta;		
			}
			else ShowWindow(GetWindow(ta,GW_HWNDPREV),1);
			ShowWindow(box,0); 
			RECT rc; GetWindowRect(box,&rc); MapWindowRect(0,hWnd,&rc);
			SetWindowPos(ta,box,rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,0);			
			//10000: start wrapping over from the top
			SendMessage(ta,EM_SETTARGETDEVICE,0,10000);
			SendMessage(ta,EM_SETTARGETDEVICE,0,0);
			SetFocus(ta);
			SetWindowRedraw(hWnd,1); RedrawWindow(hWnd,0,0,RDW_INVALIDATE);
			break;
		}
		case MAKEWPARAM(1210,EN_SETFOCUS): case MAKEWPARAM(1211,EN_SETFOCUS): 
		case MAKEWPARAM(1212,EN_SETFOCUS): case MAKEWPARAM(1213,EN_SETFOCUS): 
		case MAKEWPARAM(1214,EN_SETFOCUS): case MAKEWPARAM(1215,EN_SETFOCUS): 
		{
			HWND bubble = GetDlgItem(hWnd,LOWORD(wParam)-10);
			if(!Button_GetCheck(bubble))
			SendDlgItemMessage(hWnd,LOWORD(wParam)-10,BM_CLICK,0,0);
			SetFocus(ta);
			break;
		}}
		break;

	case WM_NCDESTROY:

		if(SOM::MO::view)
		SOM::MO::view->pastespecial_2ED_clipboard = 0;
		break;
	}		  	
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
} 
static LRESULT CALLBACK SOM_MAIN_171(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scID, DWORD_PTR readonly)
{		
	static int seesaw;

	switch(uMsg)
	{
	case WM_INITDIALOG: 
	{
		HWND ta = GetDlgItem(hWnd,1000); 		
		SetWindowTextW(ta,SOM_MAIN_list->quoted);				
		seesaw = wcschr(SOM_MAIN_list->quoted,'\n')?0:1;		
		if(readonly&&Edit_SetReadOnly(ta,readonly))
		windowsex_enable(hWnd,1008,1010,!readonly);
		return 1;
	}
	case WM_DRAWITEM: //REMOVE ME? 

		if(wParam==1008||wParam==1009) //seesaw
		return som_tool_drawitem((DRAWITEMSTRUCT*)lParam,156,seesaw^wParam&1);
		break;

	case WM_COMMAND:

		switch(wParam)
		{
		case 1008: case 1009: if(!readonly) //seesaw
		{			
			seesaw = ~wParam&1; 			
			RedrawWindow(GetDlgItem(hWnd,1008),0,0,RDW_INVALIDATE);
			RedrawWindow(GetDlgItem(hWnd,1009),0,0,RDW_INVALIDATE);
		}
		break;
		case MAKEWPARAM(1000,EN_CHANGE):
		case MAKEWPARAM(1013,CBN_SELENDOK):
		case MAKEWPARAM(1013,CBN_EDITCHANGE):		
		{
			int ok = 0; if(!readonly)
			{
				wchar_t peek[SOM_MAIN_msgctxt_s+2] = L""; 
				if(GetWindowTextW((HWND)lParam,peek,EX_ARRAYSIZEOF(peek)))
				{
					while(ok<SOM_MAIN_msgctxt_s&&peek[ok]&&peek[ok]<127) ok++; 				
					ok = peek[ok]>=127||ok==SOM_MAIN_msgctxt_s&&peek[ok];				
				}
				else ok = 1; //clearing the context doesn't require validation

				assert(ok||HIWORD(wParam)!=CBN_SELENDOK);
			}			
			if(!ok||IsWindowVisible(hWnd))
			EnableWindow(GetDlgItem(hWnd,wParam&1?1010:IDOK),ok);
			break;
		}
		case MAKEWPARAM(1013,CBN_DROPDOWN):
		{
			HWND cb = (HWND)lParam;
			COMBOBOXINFO cbi = {sizeof(cbi)};
			if(ComboBox_GetCount(cb)||!GetComboBoxInfo(cb,&cbi)) break;

			if(SOM_MAIN_tree->contexts.empty())
			{
				SOM_MAIN_tree->context = 0;
				SOM_MAIN_tree->contexts.push_back(L""); //marks checked

				const char *_4, *curr, *prev, *p; 
				mo::range_t ri; mo::range(SOM_MAIN_tree->mo,ri);
				for(size_t i=0,curr_s,prev_s=0;i<ri.n;i++)
				if(_4=strchr(curr=mo::msgctxt(ri,i),'\4'))
				{
					curr_s = _4-curr;
					if(prev_s==curr_s&&!memcmp(prev,curr,prev_s)) continue;
					prev_s = 0;

					if(!_4[1]&&curr_s<=SOM_MAIN_msgctxt_s)
					{
						for(p=curr;*p>'\4';p++); if(p==_4)
						{
							mo::range_t skip; size_t ub =
							mo::upperbound(SOM_MAIN_tree->mo,curr,curr_s+1);
							i+=mo::range(SOM_MAIN_tree->mo,skip,ri.lowerbound+i,ub);
							if(skip.n) i--; else assert(0);	continue;
						}					
					}

					if(curr_s&&_4[-1]!='\n')
					SOM_MAIN_tree->contexts.push_back(SOM_MAIN_utf8to16(curr,curr_s));			
					prev_s = curr_s; prev = curr;
				}
				else prev_s = 0;
			}

			for(size_t i=0;i<SOM_MAIN_tree->contexts.size();i++)
			{
				LPARAM id = (LPARAM)SOM_MAIN_tree->contexts[i].c_str();
				ComboBox_SetItemData(cb,ComboBox_AddString(cb,(wchar_t*)id),id);
			}
			if(SOM_MAIN_tree->context) SendMessage(cb,CB_SETTOPINDEX,
			ComboBox_FindStringExact(cb,-1,(wchar_t*)SOM_MAIN_tree->context),0);
			break;
		}
		case 1010: //select
		{
			HWND cb = GetDlgItem(hWnd,1013); 
			SOM_MAIN_tree->context = ComboBox_GetItemData(cb,ComboBox_GetCurSel(cb));
			break;
		}
		case IDOK: try_once_more:
		{
			HWND ta = GetDlgItem(hWnd,1000);
			size_t lc = Edit_GetLineCount(ta);
			wchar_t ln[4], ln_s = Edit_GetLine(ta,lc-1,ln,4);
			if(ln_s==1==seesaw) //\r for richedit50 
			{
				int sel = !seesaw?
				Edit_GetTextLength(ta):Edit_LineIndex(ta,lc-1)-1;
				Edit_SetSel(ta,sel,-1);Edit_ReplaceSel(ta,seesaw?L"":L"\r"); 
				if(seesaw&&lc>1) goto try_once_more;
			}
			break;
		}}
		break;
	}		
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}

static HWND SOM_MAIN_172_f2 = 0;
static RECT SOM_MAIN_172_divider;
static void SOM_MAIN_172_fillwordbar(HWND);
static VOID CALLBACK SOM_MAIN_172_setmargins(HWND,UINT=0,UINT_PTR=0,DWORD=0);
static LRESULT CALLBACK SOM_MAIN_172_wordpadproc(HWND,UINT,WPARAM,LPARAM,UINT_PTR,DWORD_PTR);
static LRESULT CALLBACK SOM_MAIN_172(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scID, DWORD_PTR readonly)
{	
	static HWND preview = 0, rul;	
	static SOM_MAIN_cpp::layout lo[2];
	static RECT sep, minsz; static int x, y;	
	
	switch(uMsg)
	{
	case WM_INITDIALOG: preview = 0;
	{	
		HWND gp = GetParent(hWnd);
		HWND ta = GetDlgItem(hWnd,1023); 
		HWND tb = GetDlgItem(hWnd,1022); 
		
		DWORD nz,dz; if(!wParam) //why is this necessary???
		SendMessage(ta,EM_GETZOOM,(WPARAM)&nz,(LPARAM)&dz);

		if(gp==som_tool) //151/152
		{	
			HWND tv = GetParent(SOM_MAIN_172_f2);
			SetWindowTextW(hWnd,SOM_MAIN_label(tv,SOM_MAIN_tree[lParam][tv]));
		}
		else if(&*SOM_MAIN_list!=&SOM_MAIN_list.item) //154
		{
			lParam = SOM_MAIN_17X_continue(hWnd,wParam,lParam);
			if(!lParam) return 0; //cancelled
			SOM_MAIN_172_f2 = SOM_MAIN_tree[lParam].undo;
		}	

		if(wParam) //initializing
		{
			som_tool_extendtext(ta,ENM_SELCHANGE|ENM_CHANGE);
			SendMessage(ta,EM_SETUNDOLIMIT,SOM_MAIN_undos,0);
			SetWindowSubclass(ta,SOM_MAIN_172_wordpadproc,0,0);				
			
			som_tool_extendtext(tb); 		
			SendMessage(tb,EM_SETUNDOLIMIT,0,0);
			Edit_SetReadOnly(tb,1);				

			lo[0] = tb; lo[1] = ta;
			GetWindowRect(hWnd,&minsz);
			minsz.bottom-=lo[1].bottom-lo[1].top-lo[1]->nc;
			//todo: move to subproc common to 152/154/172
			RECT &tpos = lo[lo[0].top>lo[1].top];
			RECT &bpos = lo[lo[0].top<lo[1].top];
			GetClientRect(hWnd,&sep);
			sep.top = bpos.top-tpos.bottom; //divider
			sep.left = lo[1].left; //not used
			sep.right-=lo[1].right;
			sep.bottom-=bpos.bottom;

			SOM_MAIN_172_fillwordbar(tb);

			const int bts[] = {1213};
			SOM_MAIN_buttonup(hWnd,bts,1083,1083);
		}
		else SetWindowRedraw(ta,0); 		
		SOM_MAIN_tree::string &msg = SOM_MAIN_tree[lParam];
		DWORD sel = 0; if(SOM_MAIN_172_f2)
		{
			if(SOM_MAIN_172_f2==msg.undo)					
			sel = SOM_MAIN_copyrichtextext(ta,SOM_MAIN_172_f2);
			else SOM_MAIN_copyrichtext(ta,SOM_MAIN_172_f2);
		}
		else if(SOM_MAIN_list[scID].selected)
		{
			SOM_MAIN_fillrichtext(ta,SOM_MAIN_list[scID].selected,msg.str);
		}
		else if(SOM_MAIN_tree.nihongo.mo.claim(msg.id)) //show example
		{
			std::string &utf8 = SOM_MAIN_std::string();
			SETTEXTEX st = {0,CP_UTF8}; SOM_MAIN_libintl(utf8,msg.id);
			SendMessage(ta,EM_SETTEXTEX,(WPARAM)&st,(LPARAM)utf8.c_str()); 		
		}	 		

		if(readonly) Edit_SetReadOnly(ta,1);			
		SendMessage(ta,EM_SETBKGNDCOLOR,0,GetSysColor(COLOR_3DFACE));
		const wchar_t *fface = //hack: SOM_MAIN_153_richtext
		msg.serifed()?som_932w_MS_Mincho:som_932w_MS_Gothic;
		SOM_MAIN_format(ta,0,SOM_MAIN_172_f2==msg.undo?0:fface,0);
		SendMessage(ta,EM_EMPTYUNDOBUFFER,0,0);
		Edit_SetModify(ta,0);
		//gp: allow selection change when working in tree		
		windowsex_enable(hWnd,IDOK,IDOK,gp==som_tool);
		Edit_SetSel(ta,LOWORD(sel),HIWORD(sel));
		if(wParam) 
		{
			SendMessage(ta,EM_SETZOOM,5,4);
			SOM_MAIN_172_setmargins(hWnd);
		}
		else //why must zoom be set?
		{
			SendMessage(ta,EM_SETZOOM,nz,dz);		
			SetWindowRedraw(ta,1);
			InvalidateRect(ta,0,0);
		}
		SetFocus(ta);		
		//formerly of WM_ERASEBKND
		SOM_MAIN_cpp::layout &t = lo[lo[0].top>lo[1].top];
		SOM_MAIN_cpp::layout &b = lo[lo[0].top<lo[1].top];
		SetRect(&SOM_MAIN_172_divider,t.left,t.bottom,b.right,b.top);
		return 0;
	}
	case WM_GETMINMAXINFO:
	{
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = minsz.right-minsz.left;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = minsz.bottom-minsz.top;
		break;
	}
	case WM_SIZE: //todo: somehow merge 152/154/172
	{
		if(wParam!=SIZE_RESTORED&&wParam!=SIZE_MAXIMIZED) break;
		
		int w = LOWORD(lParam), h = HIWORD(lParam), z = SWP_NOZORDER;
						
		HDWP dwp = BeginDeferWindowPos(11);	
		int xdelta = w-sep.right-lo[1].right;
		bool bottom = lo[0].top<lo[1].top;
		int ydelta = h-sep.bottom-lo[bottom].bottom;
		lo[1].bottom+=ydelta;
		if(!bottom)	lo[0].top+=ydelta;
		if(!bottom)	lo[0].bottom+=ydelta;
		for(int i=0;i<2;i++)
		{
			RECT &pos = lo[i]; pos.right+=xdelta;
			DeferWindowPos(dwp,lo[i]->cp,0,pos.left,pos.top,pos.right-pos.left,pos.bottom-pos.top,z);		
			if(preview&&lo[i]->cp==GetDlgItem(hWnd,1023))
			DeferWindowPos(dwp,preview,0,pos.left,pos.top,pos.right-pos.left,pos.bottom-pos.top,z);		
		}
		for(HWND ch=GetWindow(hWnd,GW_CHILD);ch;ch=GetWindow(ch,GW_HWNDNEXT))
		{
			RECT rc; GetWindowRect(ch,&rc); 

			switch(GetWindowID(ch))
			{
			case 1022: case 1023: continue; //boxes
			case 1082: if(bottom) continue; //rulers
				
				OffsetRect(&rc,0,ydelta); break;

			default: if(ch==preview) continue;
				
				OffsetRect(&rc,xdelta,ydelta);
			}
			MapWindowRect(0,hWnd,&rc); 
			DeferWindowPos(dwp,ch,0,rc.left,rc.top,0,0,z|SWP_NOSIZE);
		}						  
		
		if(!EndDeferWindowPos(dwp)) assert(0);

		//reminder: wrapping changes regardless
		if(xdelta) SOM_MAIN_172_setmargins(hWnd); 						
		SetRect(&SOM_MAIN_172_divider,0,lo[1].bottom,w,lo[0].top);
		if(ydelta&&!bottom) 
		InvalidateRect(hWnd,&SOM_MAIN_172_divider,1);
		break;
	}	
	case WM_NCLBUTTONDBLCLK: //todo: somehow merge 152/154/172 
	{
		int sw = IsZoomed(hWnd)?SW_RESTORE:SW_MAXIMIZE;
		//ShowWindow immediately bounces back to its restored size 
		ShowWindowAsync(hWnd,IsZoomed(hWnd)?SW_RESTORE:SW_MAXIMIZE);
		break;
	}
	case WM_SETCURSOR: case WM_LBUTTONDOWN: //todo: somehow merge 152/154/172
	{
		RECT &ta = lo[1], &tb = lo[0];	  
		POINT pt; GetCursorPos(&pt);
		MapWindowPoints(0,hWnd,&pt,1);
		rul = ChildWindowFromPoint(hWnd,pt);
		if(GetWindowID(rul)!=1082) rul = 0; 
		if(rul
		||(pt.x>ta.left&&pt.x>tb.left&&pt.x<ta.right&&pt.x<tb.right
		&&(pt.y<ta.top&&pt.y>tb.bottom||pt.y>ta.top&&pt.y<tb.bottom)))
		{
			switch(uMsg)
			{	  
			case WM_SETCURSOR:
			
				SetCursor(LoadCursor(0,rul?IDC_SIZEWE:IDC_SIZENS));
				return 1;
			
			case WM_LBUTTONDOWN: 
				
				SetCapture(hWnd); x = pt.x; y = pt.y; 
				break;
			}
		}
		break;
	}
	case WM_LBUTTONUP: case WM_MOUSEMOVE: //todo: somehow merge 152/154/172
	
		if(GetCapture()==hWnd) switch(uMsg)
		{
		case WM_LBUTTONUP: ReleaseCapture(); break;		
		case WM_MOUSEMOVE:
		{
			POINT pt; GetCursorPos(&pt);
			MapWindowPoints(0,hWnd,&pt,1);
			int xdelta = pt.x-x; x = pt.x;
			int ydelta = pt.y-y; y = pt.y;

			if(rul)
			{
				if(!xdelta) break;
				
				HWND cp = rul; bool ctrl;
				if(ctrl=GetKeyState(VK_CONTROL)>>15)
				{
					cp = GetDlgItem(hWnd,1082); if(xdelta>0)
					for(HWND t=cp;1082==GetWindowID(t=GetWindow(t,GW_HWNDNEXT));cp=t);
				}

				HWND prev = GetWindow(cp,GW_HWNDPREV);
				HWND next = GetWindow(cp,GW_HWNDNEXT);
				
				RECT rc; int left = lo[1].left, right = lo[1].right; 

				if(GetWindowID(prev)==1082)
				if(GetWindowRect(prev,&rc)&&MapWindowRect(0,hWnd,&rc)) 
				left = rc.right;
				if(GetWindowID(next)==1082)				
				if(GetWindowRect(next,&rc)&&MapWindowRect(0,hWnd,&rc)) 
				right = rc.left;
				
				GetWindowRect(cp,&rc); MapWindowRect(0,hWnd,&rc);
						  				
				if(rc.left+xdelta<left)	xdelta = left-rc.left;
				if(rc.right+xdelta>right) xdelta = right-rc.right;				
				if(!xdelta) break;

				MoveWindow(cp,rc.left+xdelta,rc.top,rc.right-rc.left,rc.bottom-rc.top,1);
								
				if(ctrl) //moving all margins/tabstops at once
				{
					int gw = xdelta>0?GW_HWNDPREV:GW_HWNDNEXT;	
					for(HWND t=cp;1082==GetWindowID(t=GetWindow(t,gw));)
					{
						GetWindowRect(t,&rc); MapWindowRect(0,hWnd,&rc);
						MoveWindow(t,rc.left+xdelta,rc.top,rc.right-rc.left,rc.bottom-rc.top,1);
					}	 					
				}
				
				if(ctrl||GetWindowID(next)!=1082) minsz.right+=xdelta;
				
				SetTimer(hWnd,'|--|',16,SOM_MAIN_172_setmargins);
				
				break;
			}
			else if(!ydelta) break; 
			
			bool bottom = lo[0].top>lo[1].top;
			SOM_MAIN_cpp::layout &t = lo[bottom], &b = lo[!bottom];

			if(t.bottom+ydelta<t.top+t->nc) ydelta = t.top+t->nc-t.bottom;
			if(b.top+ydelta>b.bottom-b->nc) ydelta = b.bottom-b->nc-b.top;

			if(ydelta<0)
			{
				t.bottom+=ydelta; b.top = t.bottom+sep.top;
			}
			else if(ydelta>0)
			{				
				b.top+=ydelta; t.bottom = b.top-sep.top;
			}
			else break;

			if(!bottom) //resizing toolbar
			{
				GetWindowRect(hWnd,&minsz);
				minsz.bottom-=lo[1].bottom-lo[1].top-lo[1]->nc;
			}

			HWND ruler = GetDlgItem(hWnd,1082); if(ruler) do
			{
				RECT rc; GetWindowRect(ruler,&rc); MapWindowRect(0,hWnd,&rc);
				MoveWindow(ruler,rc.left,rc.top+ydelta,rc.right-rc.left,rc.bottom-rc.top,1);

			}while(1082==GetWindowID(ruler=GetWindow(ruler,GW_HWNDNEXT)));
			MoveWindow(t->cp,t.left,t.top,t.right-t.left,t.bottom-t.top,1);
			MoveWindow(b->cp,b.left,b.top,b.right-b.left,b.bottom-b.top,1);			
			if(preview) //hack
			{
				RECT cp = Sompaste->frame(GetDlgItem(hWnd,1023),hWnd);
				MoveWindow(preview,cp.left,cp.top,cp.right-cp.left,cp.bottom-cp.top,1);	
			}
			SetRect(&SOM_MAIN_172_divider,t.left,t.bottom,b.right,b.top);
			InvalidateRect(hWnd,&SOM_MAIN_172_divider,1);			
			break;
		}}	
		break;

	case WM_LBUTTONDBLCLK: 
	{
		POINT pt; POINTSTOPOINT(pt,lParam);
		HWND rul = ChildWindowFromPoint(hWnd,pt);
		if(1082!=GetWindowID(rul)) break;
		HWND ta = GetDlgItem(hWnd,1023);
		HWND z = GetWindow(rul,GW_HWNDPREV);
		RECT wr, rc; GetWindowRect(ta,&wr); GetWindowRect(rul,&rc);			
		if(1082==GetWindowID(z))
		{
			int xdelta = wr.right-(rc.right-rc.left)-rc.left;			
			rc.left+=xdelta; minsz.right+=xdelta;
		}
		else rc.left = wr.left; 
		MapWindowRect(0,hWnd,&rc);
		SetWindowPos(rul,z,rc.left,rc.top,0,0,SWP_NOSIZE);
		SOM_MAIN_172_setmargins(hWnd);
		break;
	}
	case WM_NOTIFY:
	{
		ENLINK  *p = (ENLINK*)lParam;

		switch(p->nmhdr.code)
		{
		case EN_LINK: 
		{			
			if(p->nmhdr.idFrom==1022)
			if(p->msg==WM_LBUTTONDOWN)
			{
				POINT pt; HWND ta = p->nmhdr.hwndFrom; 
				if(GetCursorPos(&pt)&&!DragDetect(ta,pt))
				{
					wchar_t url[20];
					TEXTRANGE tr = {p->chrg,url}; *url = '\0';
					if(EX_ARRAYSIZEOF(url)-1>tr.chrg.cpMax-tr.chrg.cpMin)
					SendMessageW(ta,EM_GETTEXTRANGE,0,(LPARAM)&tr);
					//TODO: carryout ITextFont transformation
					if(*url) SOM_MAIN_report("MAIN1001",MB_OK);
					else MessageBeep(-1);
					return 1;
				}				
			}
			break;
		}		
		case EN_SELCHANGE:
		{				
			int compile[SOM_MAIN_poor]; //todo: update toolbar
			break;
		}}
		break;
	}
	case WM_COMMAND: 
		
		switch(wParam)
		{
		case MAKEWPARAM(1023,EN_CHANGE): //enable Preview?
		{
			wchar_t test[5]; //5: CRLF/surrogate pairs
			windowsex_enable(hWnd,1010,1010,GetWindowText((HWND)lParam,test,5));
			if(Edit_GetModify((HWND)lParam))
			windowsex_enable(hWnd,IDOK,IDOK);
			break;
		}		
		case IDOK: case IDCONTINUE: 
			
			if(preview) //todo: reload preview?
			DestroyWindow(preview); preview = 0;				
			break;

		case 1010: //preview (see IDOK above)
		{
			SetWindowRedraw(hWnd,0); 									
			HWND ta = GetDlgItem(hWnd,1023);
			//static: we don't want it to be auto clear'd
			static std::vector<char> &rtf = SOM_MAIN_std::vector<char>(); 
			if(preview)
			{
				if(Edit_GetModify(preview))
				{					
					GETTEXTLENGTHEX gtl = 
					{GTL_NUMBYTES,CP_UTF8}; GETTEXTEX gt = 
					{SendMessage(preview,EM_GETTEXTLENGTHEX,(WPARAM)&gtl,0),0,CP_UTF8,0,0};
					size_t headlen = rtf.size(); rtf.resize(headlen+gt.cb+sizeof("}"));
					SendMessage(preview,EM_GETTEXTEX,(WPARAM)&gt,(LPARAM)&rtf[headlen]);

					strcpy(&rtf.back()-1,"}");				
					SETTEXTEX st = {ST_KEEPUNDO,CP_UTF8}; 
					//todo: pull out added last line
					SendMessage(ta,EM_SETTEXTEX,(WPARAM)&st,(LPARAM)&rtf[0]);
					rtf.clear();
				}
				DestroyWindow(preview); preview = 0;
			}
			else //previewing
			{
				size_t headlen; rtf.clear();
				if(headlen=SOM_MAIN_peelrichtext(rtf,ta))
				{
					int ex = GetWindowExStyle(ta)&
					(WS_EX_CLIENTEDGE|WS_EX_WINDOWEDGE
					|WS_EX_RIGHT|WS_EX_RTLREADING|WS_EX_LEFTSCROLLBAR);
					int ws = GetWindowStyle(ta)&ES_RIGHT;
					ws|=WS_CHILD|WS_TABSTOP|WS_VSCROLL|WS_VISIBLE; //|WS_HSCROLL
					ws|=ES_MULTILINE|ES_WANTRETURN|ES_SAVESEL|ES_NOOLEDRAGDROP|ES_NOHIDESEL;
					preview = CreateWindowExW(ex,som_tool_richedit_class(),0,ws,0,0,1,1,hWnd,0,0,0);	
					som_tool_richcode(preview);	
					RECT rc; GetWindowRect(ta,&rc); MapWindowRect(0,hWnd,&rc);
					SetWindowPos(preview,ta,rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,0);			
					SETTEXTEX st = {0,CP_UTF8}; 
					//todo: pull out added line
					SendMessage(preview,EM_SETTEXTEX,(WPARAM)&st,(LPARAM)&rtf[headlen]);
					Edit_SetModify(preview,0);
					Edit_SetReadOnly(preview,readonly);
					rtf.resize(headlen);
				}
				else SOM_MAIN_error("MAIN199",MB_OK);				
			}				
			ShowWindow(ta,preview?0:1);
			SetWindowRedraw(hWnd,1); 
			InvalidateRect(preview?preview:ta,0,0);
			Button_SetCheck((HWND)lParam,preview?1:0);
			break;
		}}
		break;

	case WM_NCDESTROY:

		SOM_MAIN_172_f2 = 0; break;
	}
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
static void SOM_MAIN_172_fillwordbar(HWND tb)
{
	//REMOVE ME?
	static std::string with; if(with.empty())
	{
		with = "{\\urtf1"; char temp[1000] = "";
		GETTEXTEX gt = {sizeof(temp)-1,0,CP_UTF8,0,0};
		temp[SendMessage(tb,EM_GETTEXTEX,(WPARAM)&gt,(LPARAM)temp)+1] = 00;		 
		for(char *p=temp,*pp=p,*ff=0;*pp;p++) switch(*p)
		{
		case '\r': *p = '\0'; case '\0': //\n is omitted
			
			char *rtf = strchr(pp,'{'); if(rtf)
			{
				if(!ff) //hack: would be better to insert afterward
				{
				with+="{\\field{\\*\\fldinst HYPERLINK \"_ff_\"}{\\fldrslt {";
				with+=ff=(char*)EX::need_ansi_equivalent(65001,som_932w_MS_Mincho);
				with+="}}}"; 
				}
				with+="\\'20";
				*rtf = '\0'; //!
				//_: something like ul{\ul ul} has no effect
				with+="{\\field{\\*\\fldinst HYPERLINK \"_";
				with+=pp; 				
				with+="_\"}{\\fldrslt {";
				extern char som_main_richinit_pt[32];
				if(*pp=='_'&&!strcmp(pp,"_pt")) with+=som_main_richinit_pt;
				*rtf = '{'; //!!
				with+=rtf;
				with+="}}}";
			}
			else with+=pp; pp = p+1;
		}
		with+="}";
	}
	SETTEXTEX st = {0,CP_UTF8}; 
	SendMessage(tb,EM_SETTEXTEX,(WPARAM)&st,(LPARAM)with.c_str());
}
static VOID CALLBACK SOM_MAIN_172_setmargins(HWND win, UINT, UINT_PTR id, DWORD)
{
	KillTimer(win,id); //-3*d/n: tries to keep cursor centered when zooming
	HWND ta = GetDlgItem(win,1023), left = GetDlgItem(win,1082), right = 0;
	for(HWND t=left;1082==GetWindowID(t=GetWindow(t,GW_HWNDNEXT));right=t);
	DWORD n, d; if(!SendMessage(ta,EM_GETZOOM,(WPARAM)&n,(LPARAM)&d)) assert(0);
	RECT wr, lr, rr; GetWindowRect(ta,&wr); GetWindowRect(left,&lr); GetWindowRect(right,&rr);	
	int baseoffset = (lr.right-lr.left)/2-3*d/n; lr.left+=baseoffset; rr.right-=baseoffset;
	SendMessage(ta,EM_SETMARGINS,3,MAKELPARAM(d*(lr.left-wr.left)/n,d*(wr.right-rr.right)/n));
}
static LRESULT CALLBACK SOM_MAIN_172_wordpadproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scID, DWORD_PTR readonly)
{	
	switch(uMsg)
	{
	case WM_MOUSEWHEEL: 

		if(MK_CONTROL&wParam) 
		//setting timer because of som_tool_textareaproc
		SetTimer(GetParent(hWnd),'|--|',0,SOM_MAIN_172_setmargins);
		break;

	case WM_PASTE: goto paste;
	case WM_KEYDOWN: 
		
		switch(wParam)
		{			
		case 'V': //paste 

			if(GetKeyState(VK_CONTROL)>>15)
			{
				paste: SOM_MAIN_textarea f2(SOM_MAIN_172_f2);
				return SOM_MAIN_pastext(hWnd,!f2.param||SOM_MAIN_tree[f2.param].serifed(),0);
			}
			break;	

		case VK_F2: 

			SendMessage(GetParent(hWnd),WM_COMMAND,IDOK,0);
			break;
		}
		break;

	case WM_NCDESTROY:

		RemoveWindowSubclass(hWnd,SOM_MAIN_172_wordpadproc,scID);
		break;
	}
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
static LRESULT CALLBACK SOM_MAIN_173(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scID, DWORD_PTR readonly)
{	
	static bool matched;
	HWND &tv = SOM_MAIN_tree->treeview;
	//LB_SETTABSTOPS (support proportional fonts)
	const int tabstop = SOM_MAIN_msgctxt_s*4+4+3;
		
	switch(uMsg)
	{
	case WM_INITDIALOG: 
	{	
		HWND sc = GetDlgItem(hWnd,1011);
		HWND ta = GetDlgItem(hWnd,1013);
		HWND lb = GetDlgItem(hWnd,1022);
		ListBox_SetTabStops(lb,1,&tabstop);	
		Edit_LimitText(sc,SOM_MAIN_msgctxt_s);				
		SetWindowTextW(sc,SOM_MAIN_list->quoted);				
		for(HTREEITEM i=TreeView_GetChild(tv,TVI_ROOT);i=TreeView_GetNextSibling(tv,i);)
		{		
			LPARAM param = SOM_MAIN_param(tv,i);
			SOM_MAIN_tree::string &msgctxt = SOM_MAIN_tree[param];		
			if(msgctxt.nocontext()) continue;
			int j = msgctxt.ctxt_s-1; 			
			wchar_t *p = SOM_MAIN_label(tv,i), *pp = p; 
			if(j>SOM_MAIN_msgctxt_s){ assert(0); continue; } //paranoia
			*--p = '\t'; while(j-->0) *--p = msgctxt.id[j];
			j = ListBox_AddString(lb,p); ListBox_SetItemData(lb,j,param);
			if(!wcsncmp(p,SOM_MAIN_list->quoted,msgctxt.ctxt_s-1))
			if(!SOM_MAIN_list->quoted[msgctxt.ctxt_s])
			{
				ListBox_SetCurSel(lb,j); 
				SendMessage(hWnd,WM_COMMAND,MAKEWPARAM(1022,LBN_SELCHANGE),(LPARAM)lb);	
			}
		}	
		matched = -1!=ListBox_GetCurSel(lb);		
		windowsex_enable<IDOK>(hWnd,0);
		windowsex_enable<1003>(hWnd,matched&&!readonly); 		
		HWND f = GetWindowTextLengthW(ta)?ta:sc;
		Edit_SetSel(f,0,-1); //not automatic
		SetFocus(f);
		return 0;
	}	
	case WM_DRAWITEM: //REMOVE ME? 

		if(wParam==1003&&!matched) //delete
		return som_tool_drawitem((DRAWITEMSTRUCT*)lParam,156,1);
		break;

	case WM_COMMAND:

		switch(wParam)
		{
		case MAKEWPARAM(1022,LBN_SELCHANGE): 
		{
			SOM_MAIN_tree::string &msgctxt = SOM_MAIN_tree //never LB_ERR?
			[ListBox_GetItemData((HWND)lParam,ListBox_GetCurSel((HWND)lParam))];
			SetDlgItemTextW(hWnd,1011,SOM_MAIN_utf8to16(msgctxt.id,msgctxt.ctxt_s-1));
			SetDlgItemTextW(hWnd,1013,SOM_MAIN_label(tv,msgctxt[tv]));			
			if(!readonly) windowsex_enable<IDOK>(hWnd);
			break;
		}
		case MAKEWPARAM(1022,LBN_DBLCLK): 

			if(readonly) return MessageBeep(-1);
			return SendMessage(hWnd,WM_COMMAND,IDOK,0);		
		}
		break;
	}
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
namespace{namespace //bool
SOM_MAIN_179_uptodate{static struct
{inline void operator=(bool update) //all so ok appears responsive
{ if(!update&&uptodate&&ok) EnableWindow(ok,1); uptodate = update; }		
inline operator bool&(){ return uptodate; }bool uptodate; HWND ok; }uptodate = {true,0};}} 
static INT_PTR CALLBACK SOM_MAIN_179_formatsproc(HWND,UINT,WPARAM,LPARAM);
static LRESULT CALLBACK SOM_MAIN_179(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scID, DWORD_PTR readonly)
{		
	switch(uMsg)
	{
	case WM_INITDIALOG: 
	{
		SOM_MAIN_list.formats.fill(hWnd); //chkstk				
		SetWindowTextW(hWnd,SOM_MAIN_list[scID].description.c_str());		

		HWND cb1 = GetDlgItem(hWnd,1013);				
		for(size_t i=0;i<SOM_MAIN_list.formats.size();i++)
		ComboBox_AddString(cb1,SOM_MAIN_list.formats[i].itemlabel.c_str());		
		ComboBox_SetCurSel(cb1,SOM_MAIN_list.formats[SOM_MAIN_list[scID]].item);		
		SendMessage(hWnd,WM_COMMAND,MAKEWPARAM(1013,CBN_SELENDOK),(LPARAM)cb1);		
		return 1;	
	}
	case WM_COMMAND: using namespace SOM_MAIN_179_uptodate;

		switch(wParam)
		{
		case MAKEWPARAM(1013,CBN_SELENDOK): 
		{	
			//todo: add text to combobox
			HWND ok = GetDlgItem(hWnd,IDOK);
			HWND ch = GetWindow(hWnd,GW_CHILD);			
			EnableWindow(ok,SOM_MAIN_17X_cancel!=-1);
			SOM_MAIN_17X_textarea = GetDlgItem(hWnd,1014);			
			SetWindowTextW(SOM_MAIN_17X_textarea,SOM_MAIN_list->quoted);										
			const int trans = WS_EX_TRANSPARENT|WS_EX_CONTROLPARENT;
			if(trans==(trans&GetWindowExStyle(ch))) 
			{
				if(!DestroyWindow(ch)) break; //old data type
			}
			int sel = ComboBox_GetCurSel((HWND)lParam);
			if(sel<0) break; //paranoia			
			const wchar_t *rsrc = 
			SOM_MAIN_list.formats[sel].resource.c_str();
			uptodate.ok = 0; //hack: WM_INITDIALOG
			ch = SOM_MAIN_(rsrc,hWnd,SOM_MAIN_179_formatsproc);				
			SetWindowLong(ch,GWL_EXSTYLE,trans|GetWindowExStyle(ch));
			SetWindowPos(ch,0,0,0,0,0,SWP_NOSIZE); //winsanity	
			//restore OK button post WM_INITDIALOG
			uptodate = true; uptodate.ok = readonly?0:ok;
			break;
		}
		case MAKEWPARAM(1014,CBN_EDITCHANGE): 

			if(!readonly) EnableWindow(uptodate.ok,1); //IDOK
			break;
		}	
		break;
	}
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
template<typename T>
static void SOM_MAIN_179_change(HWND ch, T *t)
{	
	HWND gp = GetParent(ch); 
	if(!IsWindowVisible(ch))
	if(!IsWindowEnabled(GetDlgItem(gp,IDOK))) return;	
	int compile[sizeof(wchar_t)==2]; switch(sizeof(*t))
	{
	case 4: SetDlgItemInt(gp,1014,*(int*)t,1); break;
	case 1: SetDlgItemTextA(gp,1014,(char*)t); break;
	case 2: SetDlgItemTextW(gp,1014,(wchar_t*)t); break; 
	}	
	SendMessage(gp,WM_COMMAND,MAKEWPARAM(1014,CBN_EDITCHANGE),0);
}					  
static LRESULT CALLBACK SOM_MAIN_179_authproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
{
	switch(uMsg)
	{
	case WM_INITDIALOG: 
	{
		SetDlgItemText(hWnd,1000,SOM_MAIN_list->quoted);
		return 1;
	}
	case WM_COMMAND:

		switch(wParam)
		{		
		case MAKEWPARAM(1000,CBN_SELENDOK):

			SOM_MAIN_179_change(hWnd,SOM_MAIN_WindowText((HWND)lParam));
			break;
		}
		break;
	}
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
static void SOM_MAIN_179_dayofweek(HWND hWnd, SYSTEMTIME *st)
{	
	static int baselen = 0;
	HWND ga = GetAncestor(hWnd,GA_ROOT);	
	const wchar_t *p = L""; if(st)
	{
		static wchar_t days[500] = L""; 
		if(!IsWindowVisible(hWnd)) baselen = GetWindowTextLengthW(ga); 		
		if(!*days) GetDlgItemTextW(hWnd,1007,days,EX_ARRAYSIZEOF(days));	
		int day = 2; //reminder: days & months are not intervals
		struct tm now; time_t t; localtime_s(&now,&(t=time(0)));
		switch(12*(1900+now.tm_year)+now.tm_mon+1-(12*st->wYear+st->wMonth))
		{
		case 1: if(now.tm_mday--!=1) break;

			localtime_s(&now,&(t=mktime(&now)));

			if(now.tm_mday==st->wDay) day = 1; break; //yesterday
				
		case 0: day = now.tm_mday-st->wDay; break; //today or yesterday
		}
		if(day&&day>1) day = st->wDayOfWeek+2;

		p = days; while(day&&*p) if(*p++=='\n') day--;		
	}
	wchar_t text[50];
	if(baselen<=GetWindowTextW(ga,text,EX_ARRAYSIZEOF(text)-1))
	wcsncpy(text+baselen,p,EX_ARRAYSIZEOF(text)-baselen);
	wchar_t *delim = wcschr(text,'\n');
	//\r: XP has a scrunched up 2 line caption title
	if(delim>text&&delim[-1]=='\r') delim[-1] = ' ';
	if(delim) *delim = '\0';
	SetWindowTextW(ga,text);
	if(delim) *delim = '\n';	
}
static LRESULT CALLBACK SOM_MAIN_179_timeproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
{
	using namespace SOM_MAIN_179_uptodate; char a[32]; 

	switch(uMsg) 
	{
	case WM_INITDIALOG:
	{
		HWND mos = GetDlgItem(hWnd,1000);
		for(int i=1;i<=12;i++) 
		SendMessageA(mos,CB_ADDSTRING,0,(LPARAM)_itoa(i,a,10));							
		for(int i=1023;i<=1025;i++) SendDlgItemMessage
		(hWnd,i,i==1023?MCM_SETRANGE:DTM_SETRANGE,3,(LPARAM)SOM_MAIN_millennium);
		NMDATETIMECHANGE dtc = {{0,0,DTN_DATETIMECHANGE},0};
		if(!SOM_MAIN_windowstime(&dtc.st,SOM_MAIN_list->quoted)
		||!SendDlgItemMessage(hWnd,1024,DTM_SETSYSTEMTIME,0,(LPARAM)&dtc.st))
		{
			MessageBeep(-1); memcpy(&dtc.st,SOM_MAIN_millennium,sizeof(dtc.st));	
			SendDlgItemMessage(hWnd,1024,DTM_SETSYSTEMTIME,0,(LPARAM)&dtc.st);
		}		
		SendMessage(hWnd,WM_NOTIFY,1024,(LPARAM)&dtc);			
		SendMessage(hWnd,WM_NOTIFY,1025,(LPARAM)&dtc);
		//same as som_tool_datetime 
		HWND calendar = GetDlgItem(hWnd,1023);
		SetWindowFont(calendar,GetWindowFont(mos),1);
		SendMessage(calendar,MCM_SETCOLOR,MCSC_TITLEBK,GetSysColor(COLOR_HIGHLIGHT));
		//registry based settings
		CheckDlgButton(hWnd,3000,1); //DST
		HWND tz = GetDlgItem(hWnd,3001);
		ComboBox_SetCurSel(tz,ComboBox_AddString(tz,som_932w_LocalTime));
		return 1;
	}
	case WM_NOTIFY:
	{
		NMDATETIMECHANGE *p = (NMDATETIMECHANGE*)lParam;
		
		switch(p->nmhdr.code)
		{
		case DTN_DATETIMECHANGE: 
		{	
			if(IsWindowVisible(hWnd))
			SOM_MAIN_179_change(hWnd,SOM_MAIN_timencode(&p->st));
			SendDlgItemMessage //synchronizing
			(hWnd,wParam^1,DTM_SETSYSTEMTIME,0,(LPARAM)&p->st);			
			if(wParam&1) //1025
			{
				int pm,hr = p->st.wHour; 
				if(pm=IsWindowEnabled(GetDlgItem(hWnd,1005)))
				{
					if(hr>12) hr-=12; else pm = 0; if(!hr) hr = 12;
				}
				SetDlgItemInt(hWnd,1003,hr,0);
				sprintf(a,"%02d",p->st.wMinute);
				SetDlgItemTextA(hWnd,1004,a);
				CheckDlgButton(hWnd,1005,pm?1:0);
				SendDlgItemMessage(hWnd,1006,TBM_SETRANGE,1,MAKELPARAM(0,59));
				SendDlgItemMessage(hWnd,1006,TBM_SETPOS,1,p->st.wSecond);				
			}
			else //1024
			{				
				//synchronizing (calendar isn't keeping time?)
				SendDlgItemMessage(hWnd,1023,MCM_SETCURSEL,0,(LPARAM)&p->st);
				SendDlgItemMessage(hWnd,1000,CB_SETCURSEL,p->st.wMonth-1,0);
				SetDlgItemInt(hWnd,1001,p->st.wDay,0);
				SetDlgItemInt(hWnd,1002,p->st.wYear,1);			
				SOM_MAIN_179_dayofweek(hWnd,&p->st);
				break;
			}
			return 0;	
		}		
		case MCN_SELECT: SetFocus(p->nmhdr.hwndFrom); //winsanity				 			
		{
			//the calendar's clock component isn't keeping time
			NMDATETIMECHANGE dtc = {{0,0,DTN_DATETIMECHANGE},0};
			SendDlgItemMessage(hWnd,1025,DTM_GETSYSTEMTIME,0,(LPARAM)&dtc.st);			
			memcpy(&dtc.st,&((NMSELCHANGE*)p)->stSelStart,8);
			SendDlgItemMessage(hWnd,1024,DTM_SETSYSTEMTIME,0,(LPARAM)&dtc.st);			
			SendMessage(hWnd,WM_NOTIFY,1024,(LPARAM)&dtc);
			break;
		}}
		break;
	}
	case WM_HSCROLL:

		if(GetWindowID((HWND)lParam)==1006) 
		SendMessage(hWnd,WM_COMMAND,1005,0); 
		break;

	case WM_MOUSEMOVE: if(uptodate) break;
	{
		HWND f = GetFocus(); int fid = GetWindowID(f);
		SendMessage(hWnd,WM_COMMAND,MAKEWPARAM(fid,EN_KILLFOCUS),(LPARAM)f);
		break;	
	}	
	case WM_COMMAND:

		switch(wParam)
		{
		case MAKEWPARAM(1001,EN_CHANGE):
		case MAKEWPARAM(1002,EN_CHANGE):
		case MAKEWPARAM(1003,EN_CHANGE):
		case MAKEWPARAM(1004,EN_CHANGE): 
			
			if(GetFocus()==(HWND)lParam) uptodate = false; 
			break;

		case MAKEWPARAM(1001,EN_KILLFOCUS):		
		case MAKEWPARAM(1003,EN_KILLFOCUS):
		case MAKEWPARAM(1004,EN_KILLFOCUS):	if(uptodate) break;
		{
			int id = LOWORD(wParam);
			int i = GetDlgItemInt(hWnd,id,0,0);
			switch(id)
			{
			case 1001: //day

				if(i>28)
				{
					struct tm lt = 
					{
					0,0,0,i,
					SendDlgItemMessage(hWnd,1000,CB_GETCURSEL,0,0),
					GetDlgItemInt(hWnd,1002,0,1)-1900,0,0,-1 
					};	
					time_t time = mktime(&lt);
					time-=SOM_MAIN_clockdifference(time);
					localtime_s(&lt,&time);
					if(i!=lt.tm_mday)
					SetDlgItemInt(hWnd,id,i-lt.tm_mday-1,0);	
				}
				else if(i<1) SetDlgItemInt(hWnd,id,1,0);	
				break;
						
			case 1003: //hour 
			{
				int pm = IsWindowEnabled(GetDlgItem(hWnd,1005)); 
				int low = pm?1:0, high = pm?12:23;
				if(i<low) SetDlgItemInt(hWnd,id,low,0);	
				if(i>high) SetDlgItemInt(hWnd,id,high,0);	
				break;
			}
			case 1004: //minute

				if(i<0) SetDlgItemInt(hWnd,id,0,0);	
				if(i>59) SetDlgItemInt(hWnd,id,59,0);	
				break;
			}
			//break;
		}
		case MAKEWPARAM(1002,EN_KILLFOCUS): if(uptodate) break;
		case MAKEWPARAM(1000,CBN_SELENDOK):	case 1005: uptodate = true; 
		{			
			HWND xpm = GetDlgItem(hWnd,1005);
			int hr = GetDlgItemInt(hWnd,1003,0,0); 
			if(IsWindowEnabled(xpm)) hr%=12; 
			if(Button_GetCheck(xpm)) hr+=12;
			struct tm lt = 
			{
			SendDlgItemMessage(hWnd,1006,TBM_GETPOS,0,0),
			GetDlgItemInt(hWnd,1004,0,0),hr,
			GetDlgItemInt(hWnd,1001,0,0),
			SendDlgItemMessage(hWnd,1000,CB_GETCURSEL,0,0),
			GetDlgItemInt(hWnd,1002,0,1)-1900,0,0,-1 
			};
			time_t time = mktime(&lt);
			NMDATETIMECHANGE dtc = {{0,0,DTN_DATETIMECHANGE},0};
			if(!SOM_MAIN_windowstime(&dtc.st,time)
			||!SendDlgItemMessage(hWnd,1023,MCM_SETCURSEL,0,(LPARAM)&dtc.st)
			||!SendDlgItemMessage(hWnd,1024,DTM_SETSYSTEMTIME,0,(LPARAM)&dtc.st)
			||!SendDlgItemMessage(hWnd,1025,DTM_SETSYSTEMTIME,0,(LPARAM)&dtc.st))
			{				
				SendDlgItemMessage(hWnd,1024,DTM_GETSYSTEMTIME,0,(LPARAM)&dtc.st);
				SendMessage(hWnd,WM_NOTIFY,1024,(LPARAM)&dtc);			
				SendMessage(hWnd,WM_NOTIFY,1025,(LPARAM)&dtc);
				return MessageBeep(-1);
			}
			else SOM_MAIN_179_dayofweek(hWnd,&dtc.st); 			
			SOM_MAIN_179_change(hWnd,SOM_MAIN_timencode(time));			
			break;
		}}
		break;

	case WM_NCDESTROY:

		SOM_MAIN_179_dayofweek(hWnd,0); break;
	}
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
static LRESULT CALLBACK SOM_MAIN_179_textproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
{
	switch(uMsg)
	{
	case WM_INITDIALOG: //freeze combobox
	{
		COMBOBOXINFO cbi = {sizeof(cbi)};
		if(GetComboBoxInfo(GetDlgItem(GetParent(hWnd),1014),&cbi))
		Edit_SetReadOnly(cbi.hwndItem,1); else assert(0);
		SOM_MAIN_17X_textarea = GetDlgItem(hWnd,1000);
		SetWindowTextW(SOM_MAIN_17X_textarea,SOM_MAIN_list->quoted);
		SetDlgItemTextW(hWnd,1001,SOM_MAIN_list->quoted);		
		return 1;
	}
	case WM_COMMAND:

		if(wParam==MAKEWPARAM(1000,EN_CHANGE)&&Edit_GetModify((HWND)lParam))		
		SendMessage(GetParent(hWnd),WM_COMMAND,MAKEWPARAM(1014,CBN_EDITCHANGE),0);
		break;

	case WM_NCDESTROY: //restore combobox

		COMBOBOXINFO cbi = {sizeof(cbi)};
		if(GetComboBoxInfo(GetDlgItem(GetParent(hWnd),1014),&cbi))
		Edit_SetReadOnly(cbi.hwndItem,0); else assert(0);
		break;
	}
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
static LRESULT CALLBACK SOM_MAIN_179_itemproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
{
	using namespace SOM_MAIN_179_uptodate;

	wchar_t val[8+1]; const int val_s = EX_ARRAYSIZEOF(val)-1; 

	switch(uMsg)
	{
	case WM_INITDIALOG:
	{			
		HWND item; int set = 0; 
		wchar_t *e, *p = (wchar_t*)SOM_MAIN_list->quoted; 	
		for(int i=2000;item=GetDlgItem(hWnd,i);i++)
		if(!p) EnableWindow(item,0); else
		{			
			e = p; while(iswdigit(*e)) e++;
			if(*e=='.') while(iswdigit(*++e));
			wchar_t delim = *e; *e = '\0'; 
			if(p!=e&&SetWindowTextW(item,p)&&++set)
			if(delim=='-'&&iswdigit(e[1])) p = e+1; 
			else p = 0;	else p = 0;
			if(delim&&!p) MessageBeep(-1);
			*e = delim;			
		}
		EnableWindow(GetDlgItem(hWnd,2000+set),1);
		return 1;							  
	}
	case WM_MOUSEMOVE: if(uptodate) break;
	{
		HWND f = GetFocus(); 
		int fid = GetWindowID(f); if(fid>2000) //paranoia
		SendMessage(hWnd,WM_COMMAND,MAKEWPARAM(fid,EN_KILLFOCUS),(LPARAM)f);
		break;
	}
	case WM_COMMAND:

		if(LOWORD(wParam)>=2000) switch(HIWORD(wParam))
		{
		case EN_CHANGE: uptodate = false; break;
		case EN_UPDATE: //uptodate = false;
		{
			int dt = 0, n = 
			GetWindowTextW((HWND)lParam,val,val_s+1);
			int i,j; for(i=j=0;i<n;i++) if(i<val_s) switch(val[i])
			{
			case '.': if(!dt++) val[j++] = '.'; break;
			default: if(val[i]>='0'&&val[i]<='9') val[j++] = val[i];				
			}
			else break;	val[j] = '\0'; if(j!=n||i==val_s) 
			{
				int _ = 0xFF&Edit_GetSel((HWND)lParam);
				MessageBeep(-1); SetWindowTextW((HWND)lParam,val);
				_-=n-j; Edit_SetSel((HWND)lParam,_,_);
			}
			else //enabled/disable next in line
			{
				HWND next = GetDlgItem(hWnd,LOWORD(wParam)+1);

				if(next) if(*val&&0==_wtoi(val))
				{
					SetWindowText(next,L"0"); EnableWindow(next,0);
				}
				else EnableWindow(next,1);
			}
			break;
		}			
		case EN_KILLFOCUS: if(uptodate++) break;
		{	
			if(!GetWindowTextLengthW((HWND)lParam))
			SetWindowTextW((HWND)lParam,L"0"); 
			std::wstring &ws = SOM_MAIN_std::wstring();			
			for(int i=2000;GetDlgItemTextW(hWnd,i,val,val_s)&&wcstod(val,0);i++)			
			ws.append(L"-",!ws.empty()).append(val);
			SOM_MAIN_179_change(hWnd,ws.c_str());			
			break;
		}}
		break;
	}
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
static LRESULT CALLBACK SOM_MAIN_179_realproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
{
	using namespace SOM_MAIN_179_uptodate;

	switch(uMsg)
	{
	case WM_INITDIALOG:
	{
		wchar_t *e, *p = (wchar_t*)SOM_MAIN_list->quoted; 
		SetDlgItemInt(hWnd,1000,wcstol(p,&e,10),1);
		if(*e=='.'&&e[1]!='-') 
		{
			p = e+1; 
			SetDlgItemInt(hWnd,1001,wcstol(p,&e,10),0);
		}
		if(*e) MessageBeep(-1);
		return 1;
	}
	case WM_MOUSEMOVE: if(uptodate) break; 
	{
		uptodate = true; HWND f = GetFocus(); 
		int fid = GetWindowID(f); if(fid>=1000&&fid<=1003) //paranoia
		SendMessage(hWnd,WM_COMMAND,MAKEWPARAM(fid,EN_KILLFOCUS),(LPARAM)f);
		break;
	}
	case WM_COMMAND:

		switch(HIWORD(wParam))
		{
		case EN_CHANGE: uptodate = false; break;
		case EN_UPDATE: //uptodate = false;
		{
			//todo: just subclass these
			wchar_t val[8+1]; int c = 0;
			const int val_s = EX_ARRAYSIZEOF(val)-1;
			int n = GetWindowTextW((HWND)lParam,val,val_s+1);
			int i,j; for(i=j=0;i<n;i++) if(i<val_s) switch(val[i])
			{	
			case'-': if(!i&&~wParam&1) val[j++] = '-'; break;
			case'/':c++;case'+':case'=':c++;case'.':c+=1001; break;			
			default: if(val[i]>='0'&&val[i]<='9') val[j++] = val[i];				
			}
			else break;	val[j] = '\0'; 			
			if(j!=n||i==val_s)
			{
				int _ = 0xFF&Edit_GetSel((HWND)lParam);
				SetWindowTextW((HWND)lParam,val);			
				_-=n-j; Edit_SetSel((HWND)lParam,_,_);
			}
			if(c&&n--&&c<=1003) SetFocus(GetDlgItem(hWnd,c));
			if(j!=n||i==val_s) MessageBeep(-1);
			break;
		}			
		case EN_KILLFOCUS: if(uptodate++) break;
		{
			int dp; 
			enum{w_s=16}; wchar_t w[w_s] = L""; 
			if(dp=GetDlgItemTextW(hWnd,1000,w,w_s))
			if(GetDlgItemTextW(hWnd,1001,w+dp+1,w_s-dp-1))
			w[dp] = '.';
			double d = wcstod(w,0);
			double n = GetDlgItemInt(hWnd,1002,0,1);
			if(n) d+=n/GetDlgItemInt(hWnd,1003,0,0);
			swprintf_s(w,L"%f",d);
			wchar_t *p,*pp = wcschr(w,'.'); if(pp) pp++;
			if(pp) for(p=pp+wcslen(pp);--p!=pp&&*p=='0';*p='\0');
			SOM_MAIN_179_change(hWnd,w);
			break;			
		}}
		break;
	}
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
extern HDC som_tool_dc;
static HBITMAP SOM_MAIN_spectrum240(WORD l=0, WORD s=0)
{
	static WORD oldl = 120, olds = 120;
	static HBITMAP out = 0; if(!l&&!s&&out) return out;
	if(!out) out = CreateCompatibleBitmap(GetDC(0),240,1); //som_tool_dc //2023???
	if(!l) l = oldl; if(!s) s = olds;
	HGDIOBJ so = SelectObject(som_tool_dc,(HGDIOBJ)out);
	for(WORD h=0;h<240;h++) SetPixel(som_tool_dc,h,0,ColorHLSToRGB(h,l,s));		
	SelectObject(som_tool_dc,so);
	oldl = l; olds = s; return out;
}
static LRESULT CALLBACK SOM_MAIN_179_tabsproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
{
	//todo? make into globals
	BYTE us[10]; DWORD us_s; wchar_t w[32];

	switch(uMsg)
	{
	case WM_INITDIALOG: 
	{			
		wchar_t *e, *lp = e = 
		(wchar_t*)SOM_MAIN_list->quoted; 
		int hue = *lp?wcstoul(lp,&e,10)%240:120; //cyan
		SetDlgItemInt(hWnd,1000,hue,0);
		if(e!=lp) SetDlgItemInt(hWnd,1001,hue,0); if(*e) MessageBeep(-1);

		us_s = sizeof(us); 
		if(!SHGetValueW(HKEY_CURRENT_USER,
		L"SOFTWARE\\FROMSOFTWARE\\SOM_MAIN\\STORY\\<exml hl>",L"UserSet",0,us,&us_s)
		||us_s>sizeof(us)&&(us_s=sizeof(us)))
		{
			if(us_s>0) SOM_MAIN_l = us[0];	
			if(us_s>1) SOM_MAIN_s = us[1]; HWND h = GetDlgItem(hWnd,3030); 
			for(DWORD i=2;i<us_s&&DLGC_HASSETSEL&SendMessage(h,WM_GETDLGCODE,0,0);i++)
			{
				SetWindowText(h,_itow(us[i],w,10));
				if(WS_GROUP&GetWindowStyle(h=GetWindow(h,GW_HWNDNEXT))) break;
			}
		}
		SOM_MAIN_spectrum240(SOM_MAIN_l,SOM_MAIN_s);
		
		int tbs[] = {1005,3001,3002};
		int pos[] = {hue,SOM_MAIN_l,SOM_MAIN_s};
		for(int i=0;i<EX_ARRAYSIZEOF(tbs);i++)
		{	
			SendDlgItemMessage(hWnd,tbs[i],TBM_SETRANGE,1,MAKELPARAM(-120,120));
			SendDlgItemMessage(hWnd,tbs[i],TBM_SETPOS,1,pos[i]-120);
		}
		SendDlgItemMessage(hWnd,1080,UDM_SETRANGE32,0,240);
		return 1;
	}
	case WM_CTLCOLOREDIT:

		if(DLGC_HASSETSEL&SendMessage((HWND)lParam,WM_GETDLGCODE,0,0))
		{
			int id; switch(id=GetWindowID((HWND)lParam))
			{
			case 1000: case 1001: break; default:
				
				SetBkMode((HDC)wParam,TRANSPARENT);
				WORD h = GetDlgItemInt(hWnd,id,0,0);
				ExtSelectClipRgn((HDC)wParam,0,RGN_COPY);
				static LRESULT hb = 0; DeleteObject((HGDIOBJ)hb);
				return hb = (LRESULT)CreateSolidBrush(ColorHLSToRGB(h,120,120));
			}
		}
		break;

	case WM_DRAWITEM:
	{
		DRAWITEMSTRUCT *p = (DRAWITEMSTRUCT*)lParam;
		if(p->CtlType!=ODT_STATIC) break;	

		auto so = SelectObject(som_tool_dc,(HGDIOBJ)SOM_MAIN_spectrum240()); 		

		switch(wParam)
		{
		case 1002: case 1003:
		{
			extern HBRUSH som_tool_graybrush();
			static HBRUSH hb = 0; DeleteObject((HGDIOBJ)hb); hb = 0;
			BOOL ok; int h = GetDlgItemInt(hWnd,wParam-2,&ok,0)%240;
			if(ok) hb = CreateSolidBrush(GetPixel(som_tool_dc,h,0));
			FillRect(p->hDC,&p->rcItem,ok?hb:som_tool_graybrush());
			break;		
		}
		case 1004:
			
			SetStretchBltMode(p->hDC,HALFTONE); SetBrushOrgEx(p->hDC,0,0,0);
			StretchBlt(p->hDC,0,0,p->rcItem.right,p->rcItem.bottom,som_tool_dc,0,0,240,1,SRCCOPY);	
			break;
		}	 

		SelectObject(som_tool_dc,so); //2023

		break;
	}
	case WM_HSCROLL: //trackbars
	{
		int id, pos = 120+SendMessage((HWND)lParam,TBM_GETPOS,0,0);
		switch(id=GetWindowID((HWND)lParam))
		{		
		case 1005: SetDlgItemInt(hWnd,1000,pos,0);
			break;

		case 3001: case 3002:
		
			SOM_MAIN_spectrum240(id&1?pos:0,id&1?0:pos);
			for(int i=1002;i<=1004;i++)	InvalidateRect(GetDlgItem(hWnd,i),0,0);
			//break;
			return 1;
		}
		break;
	}
	case WM_COMMAND:
	
		switch(wParam)
		{
		case MAKEWPARAM(1000,EN_CHANGE):
		{
			HWND tb = GetDlgItem(hWnd,1005);
			BOOL ok; int h = GetDlgItemInt(hWnd,1000,&ok,0);			
			if(ok&&GetFocus()!=tb&~GetKeyState(VK_CONTROL)>>15)
			SendDlgItemMessage(hWnd,1005,TBM_SETPOS,1,h%240-120);			
			if(ok) SOM_MAIN_179_change(hWnd,&h);
			InvalidateRect(GetDlgItem(hWnd,1002),0,0);
			break;
		}
		case 12321:
		{
			us_s = 2; HWND h = GetDlgItem(hWnd,3030);
			us[0] = 120+SendDlgItemMessage(hWnd,3001,TBM_GETPOS,0,0);
			us[1] = 120+SendDlgItemMessage(hWnd,3002,TBM_GETPOS,0,0);			
			while(us_s<sizeof(us)&&DLGC_HASSETSEL&SendMessage(h,WM_GETDLGCODE,0,0))
			{
				us[us_s++] = !GetWindowText(h,w,4)?0:_wtoi(w);
				if(WS_GROUP&GetWindowStyle(h=GetWindow(h,GW_HWNDNEXT))) break;
			}
			SHSetValueW(HKEY_CURRENT_USER,
			L"SOFTWARE\\FROMSOFTWARE\\SOM_MAIN\\STORY\\<exml hl>",L"UserSet",REG_BINARY,us,us_s);		
			SOM_MAIN_l = us[0];	SOM_MAIN_s = us[1];
		}}
		break;
	}
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}
static INT_PTR CALLBACK SOM_MAIN_179_formatsproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{	   	
	case WM_INITDIALOG: case WM_NCDESTROY: 
	{
		static SUBCLASSPROC sc = 0; if(uMsg==WM_NCDESTROY)
		{
			RemoveWindowSubclass(hwndDlg,sc,0);	sc = 0;	break;
		}

		switch(GetWindowContextHelpId(hwndDlg))
		{
		case 12000: sc = SOM_MAIN_179_authproc; break;
		case 12100: sc = SOM_MAIN_179_timeproc; break;
		case 12200: sc = SOM_MAIN_179_textproc; break;
		case 12300: sc = SOM_MAIN_179_itemproc; break;
		case 12400: sc = SOM_MAIN_179_realproc; break;
		case 12500: sc = SOM_MAIN_179_tabsproc; break;

		default: MessageBeep(-1); //unfinished?
			
			DestroyWindow(hwndDlg); 
		}
		SetWindowSubclass(hwndDlg,sc,0,0);
		return SendMessage(hwndDlg,WM_INITDIALOG,wParam,lParam);
	}
	case WM_DRAWITEM: //REMOVE ME? 
	{				 			  		
		return som_tool_drawitem((DRAWITEMSTRUCT*)lParam,156);
	}}
	return 0;
}
static void SOM_MAIN_172(HWND parent, LPARAM param, HWND f2)
{
	SOM_MAIN_172_f2 = f2; LPARAM lParam = 
	SOM_MAIN_list.aspect_param(SOM_MAIN_list.aspects.msgstr_body);
	
	if(parent==som_tool) //151/152
	{
		//hack: simulate opening through 153
		SOM_MAIN_list.aspects.select(param);
		SOM_MAIN_list.focus = &SOM_MAIN_list.item;	
		SOM_MAIN_list.item.locked = GetWindowStyle(f2)&ES_READONLY;					
	}
	else if(!param&&f2) //154 (scheduled obsolete?)
	{
		if(!ListView_GetItemCount(SOM_MAIN_list->listview))
		return (void)MessageBeep(-1); 

		//todo: determine which column is selected depending on selection model
		lParam = SOM_MAIN_list.aspect_param(SOM_MAIN_list.aspects.msgstr_body);
	}
	SOM_MAIN_(172,parent,SOM_MAIN_17X,lParam);
}	
static UINT_PTR SOM_MAIN_17X_scID; 
static LPARAM SOM_MAIN_17X_selected;
static INT_PTR CALLBACK SOM_MAIN_17X(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UINT_PTR &scID = SOM_MAIN_17X_scID;
		
	switch(uMsg)
	{	
	case WM_INITDIALOG: case WM_NCDESTROY: 
	{
		//static UINT_PTR scID; 
		static SUBCLASSPROC sc = 0; if(uMsg==WM_NCDESTROY)
		{
			SOM_MAIN_17X_cancel = -1; RemoveWindowSubclass(hwndDlg,sc,scID); 
			scID = 0; sc = 0; return 0;
		}
		else scID = lParam; lParam = 0;

		switch(SOM_MAIN_list[scID].dialog())
		{
		case 170: sc = SOM_MAIN_170; break;
		case 171: sc = SOM_MAIN_171; break;
		case 172: sc = SOM_MAIN_172; break;
		case 173: sc = SOM_MAIN_173; break;
		case 179: sc = SOM_MAIN_179; break;

		default: MessageBeep(-1); //unfinished?
			
			DestroyWindow(hwndDlg); 
		}

		som_tool_recenter(hwndDlg,GetParent(hwndDlg),SOM_MAIN_onedge);
		windowsex_enable<IDOK>(hwndDlg,SOM_MAIN_17X_cancel!=-1);

		SOM_MAIN_17X_textarea = 0; DWORD lp = 
		SOM_MAIN_17X_selected =	SOM_MAIN_list.aspects.selected;
		bool readonly = !SOM_MAIN_same()&&SOM_MAIN_tree[lp].left;
		SetWindowSubclass(hwndDlg,sc,scID,SOM_MAIN_list->locked||readonly);
		return SendMessage(hwndDlg,WM_INITDIALOG,wParam,lp);
	}
	case WM_DRAWITEM: //REMOVE ME? 
					 			  		
		return som_tool_drawitem((DRAWITEMSTRUCT*)lParam,156);
	
	case WM_COMMAND: //finish up for 170~179
	{
		if(!scID) return 0; //pre-WM_INITDIALOG?

		SOM_MAIN_tree::string &msg = 
		SOM_MAIN_tree[SOM_MAIN_17X_selected];

		switch(wParam)
		{	
		case 1003: //Delete (173)

			if(173==SOM_MAIN_list[scID].dialog()) 
			{
				SOM_MAIN_list[scID].make_edit(0);
				assert(SOM_MAIN_list.aspects.edited);								
				goto apply173;
			}
			break;

		case 1010: //Select (171)
			if(171!=SOM_MAIN_list[scID].dialog()) 
			break;
		case IDOK:
		{
			HWND result = 0;
			int d = SOM_MAIN_list[scID].dialog(); 
			switch(d)
			{
			case 170: result = SOM_MAIN_17X_textarea; break;
			case 171: result = GetDlgItem(hwndDlg,wParam&1?1000:1013); break;
			case 172: result = GetDlgItem(hwndDlg,1023); break;
			case 173: result = GetDlgItem(hwndDlg,1011); break;
			case 179: result = SOM_MAIN_17X_textarea; break;
			}
			assert(result);
				
			int compile[SOM_MAIN_poor];			
			if(d==172&&SOM_MAIN_172_f2==msg.undo)
			SOM_MAIN_editrichtextext(msg.undo,result);
			else if(SOM_MAIN_list.aspects.edited) //153
			if(d==172) SOM_MAIN_copyrichtext(SOM_MAIN_172_f2,result);
			else SOM_MAIN_list[scID].make_edit(result);
			else assert(0); //154?

apply173:	HWND gp = GetParent(hwndDlg);	
			HWND apply = GetDlgItem(gp,12321);
			//todo? don't do unless modified
			if(!apply||gp==som_tool) SOM_MAIN_changed(msg.right);
			else if(!msg.temporary()) EnableWindow(apply,1);
			if(SOM_MAIN_17X_cancel>=0)
			{
				int glow = SOM_MAIN_list[scID].highlighted?LVIS_GLOW:0;
				ListView_SetItemState
				(SOM_MAIN_list->listview,SOM_MAIN_17X_cancel,glow,LVIS_GLOW|LVIS_CUT);		
			}
			SOM_MAIN_list->redraw();
			if(!IsWindowVisible(GetDlgItem(hwndDlg,IDCONTINUE)))
			return DestroyWindow(hwndDlg); 
			//break;
		}
		case IDCONTINUE: 
			
			return SendMessage(hwndDlg,WM_INITDIALOG,0,0); 			

		case IDCANCEL: goto close;
		}
		break;
	}
	case WM_CLOSE: close:
	{
		if(SOM_MAIN_17X_cancel>=0) 
		ListView_DeleteItem(SOM_MAIN_list->listview,SOM_MAIN_17X_cancel);
		DestroyWindow(hwndDlg); 
		break;
	}}
	return 0;
}	
static LPARAM SOM_MAIN_17X_continue(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	if(SOM_MAIN_list.focus==&SOM_MAIN_list.item) return lParam;

	HWND lv = SOM_MAIN_list->listview;

	static LPARAM phantom; phantom: //nice (phantom business)

	if(wParam) //initializing
	{
		phantom = 0;

		int sc = ListView_GetSelectedCount(lv); if(!sc)
		{
			sc = ListView_GetItemCount(lv);
			ListView_SetItemState(lv,-1,~0,LVNI_SELECTED);					
		}
		if(sc>1) //display IDCONTINUE button
		{
			HWND cont = GetDlgItem(hWnd,IDCONTINUE); 
			HWND cancel = GetDlgItem(hWnd,IDCANCEL); 
			if(!cont) return(MessageBeep(-1),lParam); //bad theme package
			RECT rc; GetWindowRect(cancel,&rc);	MapWindowRect(0,hWnd,&rc);
			MoveWindow(cont,rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,0);
			ShowWindow(cont,1);	ShowWindow(cancel,0);					
		}
		ListView_SetItemState(lv,
		ListView_GetNextItem(lv,-1,LVNI_SELECTED),~0,LVNI_FOCUSED);
	}

	int f = ListView_GetNextItem(lv,-1,LVNI_FOCUSED);			
	if(wParam) f--;
	int i = ListView_GetNextItem(lv,f,LVNI_SELECTED);		
	if(i==-1) return 0&DestroyWindow(hWnd);
	//must increment here so phantom can work its magic
	if(!wParam) ListView_SetItemState(lv,i,~0,LVNI_FOCUSED);
	HWND tv = SOM_MAIN_list->corresponding_treeview();
	lParam = SOM_MAIN_param(tv,SOM_MAIN_154_treeitem(lv,i));
	if(lParam==phantom) goto phantom; phantom = lParam;
	SetWindowTextW(hWnd,SOM_MAIN_label(tv,SOM_MAIN_tree[lParam][tv]));				
	//would prefer it end up on top
	ListView_EnsureVisible(lv,i,0);			
	SOM_MAIN_17X_selected = lParam;
	if(!wParam) 
	SOM_MAIN_list.aspects.select(lParam);
	return lParam;
}

static struct SOM_MAIN_copy //singleton
{
	//SOM_MAIN_copy()	
	inline char *operator()(LPARAM lp, HWND hw=0)
	{
		return operator()(SOM_MAIN_std::string(),lp,hw);
	}
	//SOM_MAIN_save_as
	bool nostringsattached, flushing;
	SOM_MAIN_copy(){ memset(this,0x00,sizeof(*this)); }
	char *operator()(std::string&,LPARAM,HWND=som_tool);			

	struct options
	{
		//reserved: this space is for formatting 
		//preferences like quirk/strict XML mode
	};

}SOM_MAIN_copy; //singleton

static INT_PTR CALLBACK SOM_MAIN_180(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg) 
	{	
	case WM_INITDIALOG: 
	{
		SETTEXTEX st = {0,CP_UTF8};
		som_tool_recenter(hwndDlg,GetParent(hwndDlg));
		HWND ta = som_tool_richcode(GetDlgItem(hwndDlg,1000));			
		SendMessage(ta,EM_SETTEXTEX,(WPARAM)&st,(LPARAM)SOM_MAIN_copy(lParam)); 
		return 1;
	}
	case WM_DRAWITEM: //REMOVE ME? 
	{				 			  		
		return som_tool_drawitem((DRAWITEMSTRUCT*)lParam,156);
	}
	case WM_COMMAND:

		switch(wParam)
		{
		case IDOK: //wrap

			//Enter doesn't actually check the checkbox???
			if(GetKeyState(VK_RETURN)>>15) SendMessage((HWND)lParam,BM_CLICK,0,0);
			SendDlgItemMessage(hwndDlg,1000,EM_SETTARGETDEVICE,0,!Button_GetCheck((HWND)lParam));
			break;

		case IDCANCEL: goto close;
		}
		break;

	case WM_CLOSE: close:
	{
		DestroyWindow(hwndDlg); 	
		break;
	}}

	return 0;
}

struct SOM_MAIN_mmap //singleton
{	
	//CONSIDER inline MEMBERS private

	//after an afternoon of research there
	//doesn't seem to be any off the shelf
	//dynamic memory solutions for a fixed
	//buffer (memory mapped file) scenario

	//since the size of our memory pool is
	//a multiple of megabytes this manager
	//just agressively fills up that space
	//not really concerned with whether it
	//gets dynamically parceled out or not
	//(it's basically a garbage collector)

	struct fragment : std::pair<char*,int> 
	{
		fragment(){}
		fragment(char*f,int s):pair(f,s){}
		inline bool operator<(const fragment &b)const
		{
			return first<b.first;
		}
		inline char *sum(){ return first+second; }

		inline bool mergeleft(char *middle, fragment &f)
		{
			if(first!=f.sum()) return false;
			if(middle<f.sum()) 
			{
				int half = middle-f.first;
				second+=half; f.first = middle; f.second-=half;
				return false;
			}
			else second+=f.second; return true;
		}
		inline bool mergeright(char *middle, fragment &f)
		{
			if(f.sum()!=first) return false;
			if(middle>f.first) 
			{
				int half = f.sum()-middle;
				second+=half; f.first = middle; f.second-=half;
				return false;
			}
			first = f.first; 
			second+=f.second; return true;
		}

	}left, right; std::vector<fragment> fragments;	
	void letout()
	{
		left.first-=8; left.second+=8; right.second+=8;
	}	
	bool takein()
	{
		if(left.second<8||right.second<8) 
		return false;
		left.first+=8; left.second-=8; right.second-=8; 
		return true;
	}
	inline char *middle()
	{
		return left.first+(right.sum()-left.first)/2;
	}	
	void clear(char *front, int back)
	{
		fragments.clear(); 	
		const mo::header_t &hd = 
		mo::imageheader(SOM_MAIN_right.mo);
		left.first = (char*)&hd+hd.msgidndex+hd.nmessages*8;
		left.second = front-left.first;
		right.first = front+back;
		right.second = (char*)&hd+hd.msgstrndex-right.first;
	}

	bool reallocate(SOM_MAIN_tree::string &string)
	{
		if(string.bilingual())
		if(string.mmap!=size_t(-1)) //cheating
		{
			assert(0); //not expecting to be used

			const char *id = string.id;			
			string.id = string.mmapid(0);
			bool out = reallocate(string,left,right);
			string.id = id; return out;
		}
		return reallocate(string,left,right);
	}		
	void deallocate(SOM_MAIN_tree::string &string)
	{	
		const char *id = string.id;
		if(string.bilingual()) string.id = string.mmapid(0);
		deallocate(string,INT_MAX); string.id = id;		
	}
	inline int deallocate(SOM_MAIN_tree::string &string, int s)
	{
		int out = string.id_s+1+string.str_s+1;	if(out>s) return 0;
		if(SOM_MAIN_right.mo.claim(string.str)) if(!string.rewrote())
		{
			assert(SOM_MAIN_right.mo.claim(string.id));
			fragments.push_back(fragment((char*)string.id,string.id_s));
			fragments.push_back(fragment((char*)string.str,string.str_s));
		}
		else fragments.push_back(fragment((char*)string.id,out)); return out;
	}		
	inline bool reallocate(SOM_MAIN_tree::string &string, fragment &l, fragment &r)
	{
		fragment &f = l.second>r.second?l:r;		
		int less = deallocate(string,f.second);
		if(!less) return false; bool right = &f==&r;
		const char *id = right?f.first:f.sum()-less; f.second-=less;
		const char *str = id+string.id_s+1; if(right) f.first+=less; 
		memmove((void*)id,string.id,string.id_s+1);		
		memmove((void*)str,string.str,string.str_s+1);
		if(string.mmapid()==string.id) //coallocating?
		mo::move(SOM_MAIN_right.mo,string.mmap,id,str);
		string.id = id; string.str = str; return true;		
	}	

	bool reverse; HWND progress149;
	bool coalesce(SOM_MAIN_tree::string *firstring=0)
	{
		//set to 0 before calling coalesce if unwanted
		if(progress149)	assert(IsWindow(progress149));

		const int n = defragment(); 
		const int m = std::lower_bound
		(fragments.begin(),fragments.end(),
		fragment(middle(),0))-fragments.begin();
		int l = m, r = l+1; if(!n) return false;		
		if(firstring) coallocate(*firstring,l,r,n);
		SOM_MAIN_tree::pool &strings = SOM_MAIN_right;				
		if(reverse=!reverse) 
		{
			SOM_MAIN_tree::pool::iterator it;
			for(it=strings.begin();it!=strings.end();it++)
			if(!coallocate(it->nc(),l,r,n)&&--l<0&&++r>=n) break;
		}
		else //randomizing (coallocate also does some)
		{		
			//tying --l&&++r also introduces randomness

			SOM_MAIN_tree::pool::reverse_iterator it;
			for(it=strings.rbegin();it!=strings.rend();it++)
			if(!coallocate(it->nc(),l,r,n)&&--l<0&&++r>=n) break;
		}
		if(progress149)
		if(!IsWindow(progress149)) return true; //cancelled?
		if(n==fragments.size()) return false;
		else defragment(); return true;
	}	
	inline bool coalesced(const char *p, int s, fragment &l, fragment &r)
	{
		return p>=l.sum()&&p+s<r.first;
	}
	inline bool coallocate(SOM_MAIN_tree::string &string, int l, int r, int n)
	{
		if(progress149)
		if(!SendDlgItemMessage(progress149,1053,PBM_STEPIT,0,0))
		return false; //cancelled?

		fragment &lf = l<0?left:fragments[l];
		fragment &rf = r<n?fragments[r]:right; if(!string.rewrote())
		{
			if(string.gettext_header_entry()
			 ||!SOM_MAIN_right.mo.claim(string.str)) return true;
			if(string.bilingual()
			 ||coalesced(string.id,string.id_s+1,lf,rf))
			if(coalesced(string.str,string.str_s+1,lf,rf)) return true;
			if(string.bilingual())
			{
				const char *id = string.id;
				string.id = string.mmapid();
				bool out = coallocate(string,l,r,n); string.id = id;
				return out;
			}
		}
		else if(coalesced(string.id,string.id_s+1+string.str_s+1,lf,rf)) 
		return true;
		if(reallocate(string,lf,rf)) return true;
		//reallocate(string); //randomizing
		while(--l>0&&++r<n) reallocate(string,fragments[l],fragments[r]);
		return false;
	}
	size_t defragment() //probably just want coalesce
	{
		size_t i = 0, out = 0;
		size_t n = fragments.size(); if(!n) return 0;		
		std::sort(fragments.begin(),fragments.end());
		char *p = middle(); 
		while(i<n&&left.mergeleft(p,fragments[i])) i++;
		while(n>i&&right.mergeright(p,fragments[n-1])) n--;
		if(i<n) for(fragments[0]=fragments[i++];i<n;i++)
		if(fragments[out].sum()==fragments[i].first)			
		fragments[out].second+=fragments[i].second;
		else fragments[++out] = fragments[i];
		fragments.resize(out); return out;
	}

	bool allocate //if this looks backward it was more an afterthought
	(SOM_MAIN_tree::string &string, const SOM_MAIN_tree::string &base)
	{
		size_t ctxt_s = base.ctxt_s;   
		if(base.nocontext()) ctxt_s = 0;
		if(string.nocontext()) //paranioa
		return false; //gettext header-entry 
		//roundabout: ensure not deallocated
		SOM_MAIN_tree::string safe = string;		
		if(SOM_MAIN_right.mo.claim(string.id)
		 ||SOM_MAIN_right.mo.claim(string.str)
		 ||ctxt_s!=safe.ctxt_s||safe.id[0]=='\v'
		 ||strncmp(base.id,safe.id,safe.ctxt_s))
		{
			static std::string buf(256,'\0'); 
			buf.assign(base.id,ctxt_s).append //plural?
			(safe.id+safe.ctxt_s,safe.id_s-safe.ctxt_s);
			safe.id_s = buf.size();
			safe.ctxt_s = ctxt_s; buf+='\0'; //relocating?
			buf.append(safe.str?safe.str:"",safe.str_s)+='\0';			
			safe.id = buf.c_str(); safe.str = safe.id+safe.id_s+1;
		}
		else if(!safe.str) safe.str = ""; safe.mmap = -1;

		//0: enable to safely force/debug coalesce below
		bool testing = 0&&EX::debug&&!fragments.empty();

		//POINT OF NO RETURN
		int tookin = takein();
		if(!tookin||testing||!reallocate(safe))
		{			
			SOM_MAIN_(149,som_tool_dialog(),SOM_MAIN_149);		
			EX::sleep(500); 
			//220: memory is critically low
			if(!coalesce(&safe)||!(tookin+=takein())) 
			SOM_MAIN_warning("MAIN220",MB_OK);
			EX::sleep(500); 
			DestroyWindow(progress149);
			//if(testing) //can't hurt
			if(tookin&&!SOM_MAIN_right.mo.claim(safe.str)) 
			reallocate(safe);
			while(tookin--) letout();
			//allocate must guarantee takein is going to work
			if(!SOM_MAIN_right.mo.claim(safe.str)||!takein())
			{ deallocate(safe); return false; }
		}		
		letout(); string.base() = safe; //return true;
		SOM::MO::view->numberofalterationsinceopening++;		
		SOM_MAIN_right.add_context(string);	return true;
	}
	bool takein(SOM_MAIN_tree::string &string)
	{	
		//there is a problem here
		//mo::lowerbound returns 1 for empty strings
		assert(*string.id); 

		//assert: just allocate before calling
		if(!takein()){ assert(0); return false;	}//paranoia?
		if(!SOM_MAIN_right.mo.claim(string.str))//&&!reallocate(string))		
		{ letout(); assert(0); return false; } //paranoia???
		mo::header_t &hd = mo::imageheader(SOM_MAIN_right.mo);
		size_t lb = mo::lowerbound(SOM_MAIN_right.mo,string.id,string.id_s+1);
		//tricky: rsh left-shifts. recall: already taken in
		uint32_t rsh = lb*8, lsh = hd.nmessages-lb; lsh*=8;	
		ule32_t *idndex = (ule32_t*)memmove(left.first-lsh,left.first-lsh-8,lsh)-2;
		ule32_t *strndex = (ule32_t*)memmove(right.sum(),right.sum()+8,rsh)+rsh/4;	
		idndex[0] = string.id_s; idndex[1] = string.id-(char*)&hd;
		strndex[0] = string.str_s; strndex[1] = string.str-(char*)&hd;		
		hd.nmessages+=1; hd.msgstrndex-=8;
		SOM_MAIN_tree::pool::iterator it; assert(string.mmap==size_t(-1));
		for(it=SOM_MAIN_right.begin();it!=SOM_MAIN_right.end();it++)
		if(it->mmap>=lb&&it->mmap<hd.nmessages) it->mmap++;
		string.mmap = lb; 
		string.id = string.bilingual_id(); 
		return true;
	}
	bool letout(SOM_MAIN_tree::string &string)
	{
		if(!SOM_MAIN_right.mo.claim(string.str)) return false;
		const char *id = string.mmapid(); if(!id) return false;		
		size_t lb = string.mmap; 
		mo::range_t r; mo::entry(SOM_MAIN_right.mo,r,lb);		
		if(r.n&&mo::msgctxt(r)==id) //paranoia
		{
			mo::header_t &hd = 
			mo::imageheader(SOM_MAIN_right.mo);
			uint32_t rsh = lb*8, lsh = hd.nmessages-lb; lsh*=8;
			memmove(left.first-lsh,left.first-lsh+8,lsh-8);
			memmove(right.sum()+8,right.sum(),rsh);
			hd.nmessages-=1; hd.msgstrndex+=8;			
			SOM_MAIN_tree::pool::iterator it;
			for(it=SOM_MAIN_right.begin();it!=SOM_MAIN_right.end();it++)
			if(it->mmap>lb&&it->mmap<hd.nmessages) it->mmap--;			
			string.mmap = -1;
			if(id!=string.id)		
			string.str_s = string.bilingual_gettext(string.str);			
			letout(); return true;
		}
		else assert(0); return false; //not good
	}	

}SOM_MAIN_mmap; //singleton

static INT_PTR CALLBACK SOM_MAIN_149(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg) 
	{
	case WM_INITDIALOG:
	
		SOM_MAIN_mmap.progress149 = hwndDlg;
		SendDlgItemMessage(hwndDlg,1053,PBM_SETSTEP,1,0);
		//1: so PBM_STEPIT returns nonzeo
		//2: so PBM_SETPIT won't rollover and firstring
		SendDlgItemMessage(hwndDlg,1053,PBM_SETRANGE32,1,SOM_MAIN_right.size()+2);		
		SendDlgItemMessage(hwndDlg,1053,PBM_SETPOS,1,0);
		som_tool_recenter(hwndDlg,GetParent(hwndDlg));
		return 1;

	case WM_CLOSE: DestroyWindow(hwndDlg); break;
	}
	return 0;
}		 

static bool SOM_MAIN_gottextheaderentry(const char *str, size_t str_s)
{
	SOM_MAIN_tree::pool &pool =
	SOM_MAIN_right.mo.claim(str)?
	SOM_MAIN_right:SOM_MAIN_tree.left;
	for(size_t i=0,ii=0,form=0;i<=str_s;i++) if(!str[i]) 
	{	
		LPARAM plural = 0; switch(form++)
		{
		case 0: plural = pool.metadata; break; 
		case 1: plural = pool.messages; break;
		case 2: plural = pool.japanese; break; 
		}
		if(!plural) return false; 		
		SOM_MAIN_tree[plural].str = str+ii;
		SOM_MAIN_tree[plural].str_s = i-ii; ii = i+1;
	}
	return true; //todo? parse header-entry
}
static std::string &SOM_MAIN_gettextheaderentry(LPARAM cp=0, LPARAM rp=0)
{	
	static std::string out(1024,'\0'); 
	if(!cp&&!rp) return out;	
	out.clear(); for(int i=0;i<3;i++)
	{
		LPARAM plural = 0; switch(i)
		{
		case 0: plural = SOM_MAIN_right.metadata; break;
		case 1: plural = SOM_MAIN_right.messages; break;
		case 2: plural = SOM_MAIN_right.japanese; break; 
		}
		if(rp==plural) plural = cp;
		if(i) out+='\0'; if(!plural) continue; //japanese?
		if(const char *s=SOM_MAIN_tree[plural].str)
		{
			//2019: HAVING PROBLEMS
			assert(strlen(s)==SOM_MAIN_tree[plural].str_s);

			out.append(s,SOM_MAIN_tree[plural].str_s);			
		}
	}
	return out;
}  
static bool SOM_MAIN_mo(const wchar_t *script, HWND tv)
{		
	//reminder: msgfmt has -a --alignment
	//todo: detect alignment and set to 8
	//SOM_MAIN_mmap has to be aligned too

	HANDLE cp; //2021: C4533
	{
	assert(script||!tv);
	bool closing = !script; if(closing) 
	{ script = L""; tv = SOM_MAIN_tree.right.treeview; }			
	SOM_MAIN_tree::pool &pool = SOM_MAIN_tree[tv]; pool.clear();
	retry: cp = 0; if(!closing)
	{
		DWORD sh = FILE_SHARE_READ;
		DWORD fa = GetFileAttributes(script);	
		if(fa&FILE_ATTRIBUTE_TEMPORARY)
		sh|=FILE_SHARE_WRITE;
		if(fa==INVALID_FILE_ATTRIBUTES)
		if(tv==SOM_MAIN_right.treeview)
		switch(SOM_MAIN_warning("MAIN1030",MB_CANCELTRYCONTINUE))
		{
		case IDCANCEL: return false; case IDTRYAGAIN: goto retry; case IDCONTINUE:
		
			static const char new_mo[] = 
			"\xde\x12\4\x95\0\0\0\0\1\0\0\0\x1c\0\0\0"
			"\x24\0\0\0\3\0\0\0\x2c\0\0\0\0\0\0\0"
			"\x38\0\0\0\x48\0\0\0\x39\0\0\0\1\0\0\0"
			"\0\0\0\0" "\0\0\0\0" "\0" 
			"Content-Type: text/plain; charset=UTF-8\n"
			"Content-Transfer-Encoding: 8bit\n";
			DWORD wr; cp = //try to create
			CreateFileW(script,GENERIC_READ|GENERIC_WRITE,0,0,CREATE_NEW,0,0);				
			if(cp==INVALID_HANDLE_VALUE||!WriteFile(cp,new_mo,sizeof(new_mo),&wr,0)||wr!=sizeof(new_mo))
			{
				CloseHandle(cp);
				if(cp!=INVALID_HANDLE_VALUE) DeleteFileW(script); //right?
				SOM_MAIN_error("MAIN201",MB_OK);
				goto close;
			}
			else SetFilePointer(cp,0,0,FILE_BEGIN);	
		}		
		if(!cp) cp = CreateFileW(script,GENERIC_READ,sh,0,OPEN_EXISTING,0,0);
		if(cp==INVALID_HANDLE_VALUE&&SOM_MAIN_error("MAIN200",MB_OK))
		goto close;		
	}	
		
	//todo: Editor extension
	//below: view->megabytes>1
	const size_t megabytes = 1; 
	size_t bytes = megabytes*1024*1024;
	DWORD read=0,size = GetFileSize(cp,&read);
	if(size!=INVALID_FILE_SIZE)
	{
		if(size>bytes||read) //suggest a larger buffer?	
		if(IDCANCEL==SOM_MAIN_error("MAIN223",MB_OKCANCEL))
		goto close;
	}
	else if(closing) size = 0;
	else goto close;
	
	//odd: OpenFileMapping wants MapViewOfFile's flags
	const DWORD access = FILE_MAP_WRITE|FILE_MAP_READ;

	if(&pool==&SOM_MAIN_right)
	{	
		using SOM::MO::view; 
		Sompaste->set(L".MO",script);
		SOM::MO::memoryfilename name(script);		
		if(view&&view->server==SOM_MAIN_151())
		{
			view->server = 0; view->closedbyserver = 1;
		}			
		UnmapViewOfFile(view); view = 0;
		HANDLE same = !name?0:OpenFileMapping(access,0,name); 
		static HANDLE fm = 0; if(fm) CloseHandle(fm); 		

		fm = 0; if(closing) return true; //whatever

		if(!(fm=same)) fm = CreateFileMapping
		(INVALID_HANDLE_VALUE,0,PAGE_READWRITE,0,sizeof(*view)+bytes,name);			
		(void*&)view = MapViewOfFile(fm,access,0,0,0);	
		int mb = MB_OK|MB_ICONERROR;
		const char *error = "MAIN199"; //???
		if(same&&view
		&&view->mobase>=sizeof(*view)
		&&!wcscmp(view->writetofilename,script)) //canonicalize?
		{
			if(IsWindow(view->server)
			&&view->server!=SOM_MAIN_151()) //there can be only one
			{
				error = "MAIN224";				
				mb = MB_OK|MB_ICONINFORMATION;
				//tv: change MessageBox parent 
				if(SetForegroundWindow(view->server)) tv = view->server;				
			}
			else if(!view->megabytes||view->megabytes>1)
			{
				//TODO: VirtualQuery actual size
			}
			else mb = 0; //preexisting 			
		}
		if(!same&&view) mb = 0; //newly created

		if(mb||!view) //inaccessible
		{		
			SOM_MAIN_code(error,mb);
			if(mb==(MB_OK|MB_ICONINFORMATION)) //???
			SetForegroundWindow(tv);
			UnmapViewOfFile(view); view = 0; CloseHandle(fm); fm = 0; 
			return(CloseHandle(cp),false);
		}
		else if(!same) //initializing
		{	
			new(view)SOM::MO::memoryfilebase;
			wcscpy(view->writetofilename,script);
			view->megabytes = megabytes;
		}
		else //todo? VirtualQuery actual size 
		{
			bytes = view->megabytes*1024*1024;		
			view->closedbyserver = 0;	
		}
		view->server = SOM_MAIN_151(); 
		mo::maptoram(pool.mo,(char*)view+view->mobase,bytes);
	}
	else if(*script) //read only
	{	
		//static: pool.mo can be set to be same as SOM_MAIN_right.mo
		pool.mo.mode = SWORDOFMOONLIGHT_READONLY;
		static HANDLE fm = CreateFileMapping(INVALID_HANDLE_VALUE,0,PAGE_READWRITE,0,bytes,0);			
		static void *readonly = MapViewOfFile(fm,access,0,0,bytes);		
		mo::maptorom(pool.mo,readonly,size);
	}	
	else assert(0);

	bool partial = size>bytes; if(partial) 
	{	
		size = bytes; mo::maptorom(pool.mo,pool.mo+0,size); 
		SOM_MAIN_warning("MAIN225",MB_OK);
	}	

	//28: byte size of a ver. 0 MO file header
	DWORD read_s = pool.mo.readonly()?size:28;
	mo::header_t &hd = mo::imageheader(pool.mo); 
	//&& if not readonly rearrange so that:
	//A) the translation index is at the back of the image
	//B) the identifiers index is at the front in case not already
	//C) the character data is centered between the two indexes		
	if(!ReadFile(cp,pool.mo+0,read_s,&read,0)
	  ||read!=read_s||hd.x950412de!=0x950412de
	  ||hd.msgidndex%4||hd.msgidndex+8*hd.nmessages>size
	  ||hd.msgstrndex%4||hd.msgstrndex+8*hd.nmessages>size
	  ||pool.mo.writable()&&
	  (hd.msgidndex!=SetFilePointer(cp,hd.msgidndex,0,FILE_BEGIN)
	  ||!ReadFile(cp,(int8_t*)pool.mo.set+read_s,8*hd.nmessages,&read,0)
	  ||hd.msgstrndex!=SetFilePointer(cp,hd.msgstrndex,0,FILE_BEGIN)
	  ||!ReadFile(cp,(int8_t*)pool.mo.end-8*hd.nmessages,8*hd.nmessages,&read,0)))
	{
		SOM_MAIN_error("MAIN200",MB_OK); goto close;
	}

	if(pool.mo.writable()) //ABC	
	{	
		hd.msgidndex = read_s;
		hd.msgstrndex = pool.mo.size-8*hd.nmessages;
		hd.revisions = hd._nhashes = hd._hashndex = 0; //paranoia

		int hi = 0, lo = size;
		int32_t *p = pool.mo+hd.msgidndex/4;
		int32_t *q = pool.mo+hd.msgstrndex/4;

		for(size_t i=0,j=1;i<hd.nmessages;i++,j+=2)
		{
			if(p[j]<lo) lo = p[j]; if(p[j]>hi) hi = p[j]+p[j-1];
			if(q[j]<lo) lo = q[j]; if(q[j]>hi) hi = q[j]+q[j-1];			
		}
		hi+=1; //add final terminating 0 character

 		int middle = (pool.mo.size-read_s)/2+read_s;
		int space = pool.mo.size-read_s-16*hd.nmessages;		
		int hi_lo = hi-lo;
		int delta = middle-hi_lo/2-lo;		

		if(hi_lo>space||lo<28||hi<28||hi>(int)size
		 ||lo!=(int)SetFilePointer(cp,lo,0,FILE_BEGIN)
		 ||!ReadFile(cp,(int8_t*)pool.mo.set+middle-hi_lo/2,hi_lo,&read,0))
		{
			SOM_MAIN_error("MAIN199",MB_OK); goto close;
		}		
		for(size_t i=0,j=1;i<hd.nmessages;i++,j+=2)
		{
			p[j]+=delta; q[j]+=delta;
		}

		//reset the dynamic memory manager
		SOM_MAIN_mmap.clear((char*)pool.mo.set+lo+delta,hi_lo);		
	}				  

	CloseHandle(cp); //that's that

	const char *set = (char*)pool.mo.set;
	const int32_t *p = pool.mo+hd.msgidndex/4;
	const int32_t *q = pool.mo+hd.msgstrndex/4;

	TVINSERTSTRUCTW tis = 
	{0,TVI_ROOT,{TVIF_TEXT|TVIF_PARAM|TVIF_CHILDREN}};
	tis.item.pszText = LPSTR_TEXTCALLBACKW;
	tis.item.cChildren = 1;

	SOM_MAIN_tree::string msg;	
	msg.ctxt_s = msg.id_s = 1;
	msg.id = "\1"; pool.metadata = 
	tis.item.lParam = pool.insert_param(msg); msg.outline++;
	SOM_MAIN_insertitem(tv,&tis); 
	msg.id = "\2"; pool.messages = 
	tis.item.lParam = pool.insert_param(msg); msg.outline++;
	SOM_MAIN_insertitem(tv,&tis);
	msg.id = "\3"; pool.japanese = 
	tis.item.lParam = pool.insert_param(msg); msg.outline++;
	SOM_MAIN_insertitem(tv,&tis);	
	if(hd.nmessages&&set[p[1]]=='\0')
	if(!SOM_MAIN_gottextheaderentry(set+q[1],q[0]))
	{
		//too many plural forms
		SOM_MAIN_error("MAIN203",MB_OK);
		pool.mo.mode = SWORDOFMOONLIGHT_READONLY;		
	}

	//todo? make use of mo::range_t
	char ctxt[SOM_MAIN_msgctxt_s] = "\4"; 		
	for(size_t i=0,j=1,k,l;i<hd.nmessages;i++,j+=2)
	{
		const char *id = set+p[j]; //msgid
		const char *str = set+q[j]; //msgstr

		//validating
		if(pool.mo<id+p[j-1]||id[p[j-1]]) id = 0;
		if(pool.mo<str+q[j-1]||str[q[j-1]]) str = 0;
			
		if(!id||!str) //failed to validate
		{
			if(!partial) 
			{
				partial = true;
				SOM_MAIN_error("MAIN204",MB_OK);
				pool.mo.mode = SWORDOFMOONLIGHT_READONLY; 
			}
			pool.mo.bad = 0; if(!id) continue;
		}
		
		for(k=0;k<sizeof(ctxt)&&id[k]>'\4'&&id[k]==ctxt[k];k++);

		if(id[k]==ctxt[k]) continue; //must be same or less context

		while(k<sizeof(ctxt)&&id[k]>'\4') k++;

		if(k&&id[k]=='\4'&&!id[k+1]) //an outline?
		{
			memcpy(ctxt,id,k); 						
			msg.id = (char*)id;
			msg.id_s = p[j-1]; msg.ctxt_s = k+1;
			msg.str = (char*)str; msg.str_s = str?q[j-1]:0;
			msg.mmap = i; 
			msg.outline++;
			tis.item.lParam = pool.insert_param(msg);
			SOM_MAIN_insertitem(tv,&tis);
		}		
	}	
	
	if(pool.mo.writable())
	{
		tis.item.state = 
		tis.item.stateMask = TVIS_CUT;
		tis.item.mask|=TVIF_STATE;
		static char stdctxts[][SOM_MAIN_msgctxt_s+1] = 
		{
		"MAP0\4",//=Maps
		"MAP1\4",//=Events
		"MAP2\4",//=Programs
		"MAP3\4",//=Counters
		"PRM0\4",//=Shop Items
		"PRM1\4",//=Shop Memory
		"PRM2\4",//=Techniques
		"PRM3\4",//=Components
		"PRM4\4",//=Opponents
		"PRM5\4",//=Complements
		"SYS0\4",//=Titles
		"SYS1\4",//=Captions
		"SYS2\4",//=Segments
		};
		msg.mmap = -1;
		msg.str = 0; msg.str_s = 0; 
		for(int i=0;i<EX_ARRAYSIZEOF(stdctxts);i++)
		{
			msg.id = stdctxts[i];
			if(pool.find(msg)==pool.end())
			{
				msg.ctxt_s = 
				msg.id_s = strlen(msg.id);
				msg.outline++;
				tis.item.lParam = pool.insert_param(msg); 
				SOM_MAIN_insertitem(tv,&tis);
			}
		}		
	}
	pool.outlines.clear();
	pool.outlines.resize(msg.outline+1);
	TVSORTCB cb = {TVI_ROOT,SOM_MAIN_outline_sort,0};
	TreeView_SortChildrenCB(tv,&cb,0);
	return true;
	}//C4533
	close:CloseHandle(cp); assert(tv);
	if(!SOM_MAIN_left(tv)) SOM_MAIN_mo(0,0); //closing
	return false; 
}
static int SOM_MAIN_po_string(const char *p, const char *eof)
{
	int out = 0; while(p<eof) switch(*p++)
	{
	case '\\':	
		
		switch(*p)
		{
		case '\\': case '"': p++; out++;
		}
		//break;

	default: out++; break;

	case '"': const char *pp = p;
		
		while(pp<eof&&isspace(*pp)) pp++;
		
		if(pp==eof||*pp!='"') return out+1; 
		
		out+=pp-p+2; p = pp+1; continue; 
	}
	return p==eof||*p!='"'?0:out+1;
}
static int SOM_MAIN_isspace(const char *p, int s)
{
	const unsigned char*&u = 
	(const unsigned char*&)p; //alias
	if(s>=1) switch(u[0])
	{
	case ' ':  
	case '\t': case '\n': case '\r':
	case '\v': case '\f':
		
		return 1;
	
	case 0xC2:
		
		if(s>=2) switch(u[1])
		{
		case 0x85: //0085 //C2 85 NEXT LINE
		case 0xA0: //00A0 //C2 A0 NO-BREAK SPACE

			return 2;
		}
		break;	

	case 0xE1:

		if(s>=3) switch(u[1])
		{
		case 0x9A: if(u[2]!=0x80) break;

			return 3; //1680 //E1 9A 80 OGHAM SPACE MARK

		case 0xA0: if(u[2]!=0x8E) break;

			return 3; //180E //E1 A0 8E //Monglian
		}
		break;

	case 0xE2:

		if(s>=3) switch(u[1])
		{
		case 0x80:

			//2000-200D //E2 80 80-8D //various sizes
			if(u[2]<=0x8) return u[2]>=0x80?3:0;

			//2028-2029 //E2 80 A8-A9 //separators	
			if(u[2]==0xA8||u[2]==0xA9) return 3;

			//202F //E2 80 AF //NARROW-NO-BREAK
			if(u[2]==0xAF) return 3; return 0;

		case 0x81:
			
			if(u[2]==0x9F //205F //E2 81 9F //mathematical
			 ||u[2]==0xA0) //2060 //E2 81 A0 //WORD JOINER 
			return 3;
		}
		break;
	
	//3000 //E3 80 80 //full-width
	case 0xE3: return s>=3&&u[1]==0x80&&u[2]==0x80?3:0;	
	//FEFF //EF BB BF //BOM
	case 0xEF: return s>=3&&u[1]==0xBB&&u[2]==0xBF?3:0;	
	}
	return 0;
}
inline int SOM_MAIN_skipspace(const char* &u, int &s)
{
	int out = SOM_MAIN_isspace(u,s); u+=out; s-=out; return out;
}
static bool SOM_MAIN_po(const char *p, const char *eof, int msgfmt)
{		
	//todo: SOM_MAIN_skipspace
	int todolist[SOMEX_VNUMBER<=0x1020504UL];

	const char bom[4] = "\xef\xbb\xbf";
	if(eof-p>=3&&!strncmp(p,bom,3)) p+=3;
	if(msgfmt&&eof-p==0) p = "#",eof = p+1;

	struct msg //msgfmt?
	{
		char *id_s, *id, *str_s, *str; 
		msg(char *_):id(_){ id_s=str=str_s=0; }
		inline bool operator<(const msg &b)const
		{ return strcmp(id,b.id)<0; }
		inline bool operator==(const msg &b)const
		{ return strcmp(id,b.id)==0; }
	};
	struct o : std::vector<msg>
	{	
		void push_back()
		{
			msg &b = back();
			b.id_s = b.id;
			if(ctxt) b.id_s = write(b.id_s,ctxt);
			if(ctxt) *b.id_s++ = '\4';
			b.id_s = write(b.id_s,id); 
			if(id_plural) b.id_s = write(b.id_s+=1,id_plural);
			b.str = b.id_s+1; b.str_s = write(b.str,str); 			
			vector::push_back(b.str_s+1);
		}
		void push_form()
		{
			msg &b = *(end()-2); b.str_s = write(b.str_s+1,str); 
			back().id = b.str_s+1;
		}
		const char *ctxt, *id, *id_plural, *str;
		static char *write(char *to, const char *from)
		{
			while(isspace(*from)) from++; if(*from++!='"') assert(0);
			return to+SOM_MAIN_po_unquote(from,0,to);
		}		

		~o() //msgfmt -o
		{
			if(!(HANDLE)f) return;			
			int compile[sizeof(void*)==4];			
			char *pos = back().id; 
			while((pos-view)%4) pos++; pop_back();
			size_t n = size();
			std::sort(begin(),end());
			mo::header_t &hd = *(mo::header_t*)view;
			hd.x950412de = 0x950412de;
			hd.nmessages = n;	
			hd.msgidndex = pos-view;
			hd.msgstrndex = pos-view+n*8;
			ule32_t *p = (ule32_t*)pos, *q = p+n*2;
			for(size_t i=0;i<n;i++,p+=2,q+=2)
			{
				p[1] = at(i).id-view; p[0] = at(i).id_s-view-p[1];
				q[1] = at(i).str-view; q[0] = at(i).str_s-view-q[1];				
			}			
			CloseHandle(fm); UnmapViewOfFile(view);
			SOM_MAIN_mo(f,SOM_MAIN_right.treeview);
			if(!out) SOM_MAIN_warning("MAIN205",MB_OK);
			if(!fm||!view) SOM_MAIN_error("MAIN202",MB_OK);
		}		
		EX::temporary f; HANDLE fm; char *view;	bool out;
		o(size_t s):f(s!=0),fm(0),view(0),ctxt(0),id(0),id_plural(0),str(0)
		{
			if(out=!s) return;
			if(!(HANDLE)f) SOM_MAIN_error("MAIN199",MB_OK);
			fm = CreateFileMapping(f,0,PAGE_READWRITE,0,s+32+4,0);
			view = (char*)MapViewOfFile(fm,FILE_MAP_READ|FILE_MAP_WRITE,0,0,0);	
			vector::push_back(view+32);	assert(fm&&view);
		}		
	
	}o(msgfmt?eof-p:0);	
	int msgstr_i_ = -1;	
	while(p<eof) switch(*p) //based on GLib msgfmt runs
	{
	case '#': while(p<eof&&*++p!='\n'); continue;

	case ' ': case '\r': case '\n': case '\t': p++; break; 
			
	default: if(eof-p<8) return false;
		
		if(*p++!='m'||*p++!='s'||*p++!='g') return false;

		switch(*p++)
		{
		default: return false;

		case 'c': //msgctxt
			
			if(*p++!='t'||*p++!='x'||*p++!='t') return false;
									
			if(o.ctxt||o.id) return false;

			o.ctxt = p; break;					

		case 'i': //msgid/msgid_plural
			
			if(*p++!='d') return false; 
					
			if(*p=='_') //msgid_plural
			{
				if(eof-p<10) return false;
				
				if(strncmp(p+1,"plural",6)) return false;				

				p+=7; o.id_plural = p;
				
				if(!o.id) return false;
			}
			else 
			{
				if(o.id) return false;

				o.id = p; 
			}
			
			msgstr_i_ = -1; break;

		case 's': //msgstr
			
			if(*p++!='t'||*p++!='r') return false;
						
			if(*p=='[')
			{
				if(msgstr_i_<0)
				if(!o.id_plural) return false;

				const char *e = p+1;
				while(e<eof&&isdigit(*e)) e++; 
				if(e>=eof||*e!=']') return false;
				int i = atoi(p+1); p = e+1;
				if(i!=1+msgstr_i_) return false;
				msgstr_i_ = i;
			}
			else 
			{
				if(o.id_plural) return false; 
				if(msgstr_i_>=0) return false; 				
				if(!o.id&&msgfmt) return false;
			}

			o.str = p; break;
		}

		if(p>=eof||!isspace(*p++)) return false;

		while(p<eof&&isspace(*p)) p++;

		if(p>=eof-1||*p++!='"') return false;

		int po_s = SOM_MAIN_po_string(p,eof);

		p+=po_s; if(!po_s) return false; 
		
		if(o.str) if(msgstr_i_<=0)
		{
			if(msgfmt) o.push_back(); o.ctxt = o.id = o.id_plural = 0; 
		}
		else if(msgfmt) o.push_form(); o.str = 0;		
	}			 
	return o.out = true;
}
static void SOM_MAIN_po_innerquote(std::string &utf, const char *p)
{
	while((unsigned char&)*p>'\4') switch(*p)
	{
	case '\r': case '\n':
	utf+="\\n\"\n\""; if(*p++=='\r'&&*p=='\n') p++; break;
	case '\v': utf+="\\v"; p++; break; //SOM_MAIN_void
	case '\\': case '"': 
		
		utf+='\\'; default: utf+=*p++; //todo? isprint
	}		
}
static void SOM_MAIN_po_quote(std::string &utf, const char *p)
{
	utf+='"'; SOM_MAIN_po_innerquote(utf,p); utf+="\"\n";
}
static int SOM_MAIN_po_unquote(const char *p, const char *eof, char *pp)
{
	if(!pp) pp = (char*)p; //NEW
	int out = 0, n = eof?SOM_MAIN_po_string(p,eof):INT_MAX; 	
	for(const char *d=p+n;p<d;p++) switch(*p)
	{
	default: pp[out++] = *p; break;

	case '\\': p++;
		
		if(*p>='0'&&*p<'8') //\000
		{
			char *e; memcpy(pp,p,3); //non-const
			pp[out] = SOM_MAIN_strntol(pp,&e,8,3);
			p+=e-p-1;
		}
		else switch(*p) //just the basics
		{
		case 'r': pp[out] = '\r'; break;
		case 'n': pp[out] = '\n'; break;		
		case 't': pp[out] = '\t'; break;		
		case 'v': pp[out] = '\v'; break; //SOM_MAIN_void 

		default: pp[out] = *p; break;

		case 'x': //\xhh

			char *e; memcpy(pp,p+1,2); //non-const
			pp[out] = SOM_MAIN_strntol(pp,&e,16,2);
			p+=e-p;
		}
		out++; break;

	case '"': p++; while(p<d&&isspace(*p)) p++;

		assert(*p=='"'||p>=d||!eof);

		if(eof||*p=='"') continue;

	case '\0': pp[out] = '\0'; return out;
	}
	return out;
}

//THESE SUBROUTINES MUSTN'T SOM_MAIN_changed
static void SOM_MAIN_cutupitem(LPARAM first)
{
	//phantom business: that's the whole point of this
	SOM_MAIN_tree::string &msg = SOM_MAIN_tree[first];	
	HWND l = SOM_MAIN_tree.left.treeview, r = SOM_MAIN_right.treeview;	
	if(SOM_MAIN_same()&&!msg.left) SOM_MAIN_reproduceitem(l,r,msg.right);	
	TVITEMEXW tvi = {TVIF_PARAM,msg.right}; 	
	while(TreeView_GetItem(r,&tvi)&&tvi.lParam==first) 
	tvi.hItem = TreeView_GetParent(r,tvi.hItem);
	HTREEITEM lp,rp = tvi.hItem; tvi.lParam = first;	
	lp = !msg.left?0:SOM_MAIN_reproduceitem(l,r,rp); 	
	while(TVIS_CUT&TreeView_GetItemState(r,tvi.hItem=rp,~0)) //!
	{	
		TreeView_SetItem(r,&tvi); if(tvi.hItem=lp) TreeView_SetItem(l,&tvi); 
		if(TreeView_GetPrevSibling(r,rp)) break; //that's far enough phantom
		rp = TreeView_GetParent(r,rp); if(lp) lp = TreeView_GetParent(l,lp);
	}
	SOM_MAIN_tree[r].redraw(msg.right);
	if(msg.left) SOM_MAIN_tree[l].redraw(msg.left); 
}
static bool SOM_MAIN_removeitem(LPARAM param, LPARAM rparam)
{	
	SOM_MAIN_before_and_after raii;
	HWND r = SOM_MAIN_right.treeview;
	HWND l = SOM_MAIN_tree.left.treeview;
	SOM_MAIN_tree::string &msg = SOM_MAIN_tree[param];
	
	HTREEITEM gc = 
	TreeView_GetChild(r,msg.right);	
	if(gc&&SOM_MAIN_textarea(r,gc)) gc = 0; 
	const bool out = gc, original = !msg.unoriginal();
	if(original) if(!gc) //greedily deleting
	{
		int lv; SOM_MAIN_tree::string &top =
		SOM_MAIN_tree[SOM_MAIN_toplevelparam(r,msg.right,&lv)];
		lv++; cut_and_not_first: lv--;
		size_t ol = msg.outline; bool cut = false;		
		HTREEITEM i = TreeView_GetNextSibling(r,msg.right);
		LPARAM first = i?SOM_MAIN_param(r,i):0;
		if(!TreeView_GetPrevSibling(r,msg.right)) //phantom business
		{
			if(first) SOM_MAIN_tree[first].outline = ol; 
			HTREEITEM gp = TreeView_GetParent(r,msg.right);
			if(cut=TVIS_CUT&TreeView_GetItemState(r,gp,~0))
			if(first) SOM_MAIN_cutupitem(first);			
			if(i) i = TreeView_GetNextSibling(r,i); ol = 2;
		}
		for(;i;i=TreeView_GetNextSibling(r,i))
		SOM_MAIN_tree[SOM_MAIN_param(r,i)].outline = ol++;						
		SOM_MAIN_right.outlines[top.outline].addtolevel(lv,-1);
		if(rparam==param) SOM_MAIN_list.left.removed(lv,msg.right);
		if(rparam==param) SOM_MAIN_list.right.removed(lv,msg.right);
		if(i=msg.left) 
		if(cut&&!first) msg.left = TreeView_GetParent(l,msg.left);		
		if(i) SOM_MAIN_deleteitem(l,i);
		i = msg.right;
		if(cut&&!first) msg.right = TreeView_GetParent(r,msg.right);		
		SOM_MAIN_deleteitem(r,i);
		if(cut&&!first) goto cut_and_not_first;		
	}
	else //greying, or converting to phantom
	{
		LPARAM first = SOM_MAIN_param(r,gc);
		SOM_MAIN_tree[first].outline = msg.outline;
		TreeView_SetItemState(r,msg.right,~0,TVIS_CUT);
		if(rparam==param) SOM_MAIN_list.left.removed(msg.right);
		if(rparam==param) SOM_MAIN_list.right.removed(msg.right);
		if(msg.left) TreeView_SetItemState(l,msg.left,~0,TVIS_CUT); SOM_MAIN_cutupitem(first);				
	}	

	if(!rparam) //deleting for good
	{
		//todo? repair menus below item

		HWND lr[2] = {l,r}; //text business
		for(int i=0;i<2;i++) if(msg[lr[i]])
		for(HWND x,ch=GetDlgItem(lr[i],param);x=ch;)
		{
			HTREEITEM item = (HTREEITEM)
			GetWindowLong(x,GWL_USERDATA);
			ch = GetWindow(ch,GW_HWNDNEXT);	
			if(item==msg[lr[i]]) !original&& //optimizing
			TreeView_Expand(lr[i],msg[lr[i]],TVE_COLLAPSE|TVE_COLLAPSERESET);
			else if(IsIconic(x)&&TreeView_GetParent(lr[i],item))
			SOM_MAIN_message_text(0,lr[i],item); //minimize			
			DestroyWindow(x); 
			if(param!=GetWindowID(ch)) break;
		}	

		if(original||msg.bilingual())
		{
			//note: the order matters here
			SOM_MAIN_mmap.deallocate(msg); 
			if(!SOM_MAIN_mmap.letout(msg)) assert(0);
		}		
	}		

	if(!original) //protecting
	{
		assert(!rparam);
		SOM_MAIN_tree[r].redraw(msg.right);
		if(msg.left) SOM_MAIN_tree[l].redraw(msg.left);
	}
	else if(param!=rparam) SOM_MAIN_right.erase_param(param);
	else msg.left = msg.right = 0;
	return out;
}
static void SOM_MAIN_movetext(HWND text, HTREEITEM item=0)
{
	if(!IsWindow(text)) return;
	if(ES_READONLY&GetWindowStyle(text))
	{
		//readonly text mainly just props up scrollbars
		DestroyWindow(text); return;
	}	
	SOM_MAIN_textarea ta(text); HWND tv = ta.treeview();
	if(!item) item = ta.item; //assuming previously set
	SOM_MAIN_textarea tb(tv,TreeView_GetChild(tv,item));					
	if(tb&&!tb.param&&!ta.param)
	{
		DestroyWindow(ta); return; //one menu is enough
	}
	if(tb&&ES_READONLY&GetWindowStyle(tb))
	{
		DestroyWindow(tb); tb = 0; assert(0); //paranoia
	}
	if(!ta.param) //refresh menu?
	{
		DestroyWindow(ta); if(!tb)
		SOM_MAIN_message_text(0,tv,item); goto expand;
	}
	else if(!tb.param&&!IsIconic(ta)) 
	{
		DestroyWindow(tb); tb = 0;
	}
	SOM_MAIN_message_text(ta.param,tv,item,text);
	if(tb&&!IsIconic(tb)) CloseWindow(ta);	
	if(IsIconic(ta)) return; expand:	
	POINT pt = {0,0};
	som_tool_toggle(tv,item,pt,TVE_EXPAND);
}
static HTREEITEM SOM_MAIN_produceitem(HWND tv, HTREEITEM i)
{
	if(!SOM_MAIN_left(tv)||!SOM_MAIN_same()) return i;
	SOM_MAIN_tree::string &msg = SOM_MAIN_tree[SOM_MAIN_param(tv,i)];	
	int diff = -SOM_MAIN_level(tv,i); 
	diff+=SOM_MAIN_level(tv=SOM_MAIN_right.treeview,i=msg.right);
	while(diff-->0) i = TreeView_GetParent(tv,i); return i;
}
static HTREEITEM SOM_MAIN_reproduceitem(HWND l, HWND r, HTREEITEM ri)
{
	if(l==r||!SOM_MAIN_same()) return ri;

	//this has evolved into a big mess
	static int depth = 0; //optimizing
	LPARAM param = SOM_MAIN_param(r,ri);
	SOM_MAIN_tree::string &msg = SOM_MAIN_tree[param];
	HTREEITEM lp = 0; if(msg.left) goto midcut;	
	depth++;
	lp = SOM_MAIN_reproduceitem(l,r,TreeView_GetParent(r,ri));
	depth--;
	if(TreeView_GetParent(l,lp))
	{
		SOM_MAIN_message_leftitem(l,lp,0); midcut:
		while(TVIS_CUT&TreeView_GetItemState(l,msg.left,~0))
		{
			SOM_MAIN_message_leftitem(l,lp=msg.left,param);
			assert(lp!=msg.left); //if(lp==msg.left) break;
		}
	}
	else SOM_MAIN_outline(l,lp); assert(msg.left);
	lp = msg.left;
	if(depth==0&&msg.right!=ri) 
	{
		int delta = SOM_MAIN_level(l,lp)-SOM_MAIN_level(r,ri);
		while(delta-->0) lp = TreeView_GetParent(l,lp);
	}
	return lp;
}
static HTREEITEM SOM_MAIN_deduceitem(HWND vt, HTREEITEM it)
{
	return SOM_MAIN_left(vt)?SOM_MAIN_produceitem(vt,it):
	SOM_MAIN_reproduceitem(SOM_MAIN_tree.left.treeview,vt,it);
}
static bool SOM_MAIN_replaceitem_ghe(LPARAM,LPARAM,HWND);
static bool SOM_MAIN_replaceitem(LPARAM cp, LPARAM rp, HWND closing=0)
{
	SOM_MAIN_before_and_after raii;
	if(SOM_MAIN_tree[rp].nocontext()) 
	return SOM_MAIN_replaceitem_ghe(cp,rp,closing);

	SOM_MAIN_tree::string &str = SOM_MAIN_tree[cp], &msg = SOM_MAIN_tree[rp];
	SOM_MAIN_tree::string swap = msg; msg.str = str.str; msg.str_s = str.str_s;	

	//workaround: SOM_MAIN_replaceitem_ghe
	if(msg!=str) if(strcmp(msg.id,str.id)) //changing msgid_plural?
	return !SOM_MAIN_error("MAIN199",MB_OK); else msg.base() = str; 

	if(!SOM_MAIN_mmap.allocate(msg,msg)) 
	{
		if(closing) //collapsing textarea? 
		{
			SOM_MAIN_textarea ta(closing);
			TreeView_SelectItem(ta.treeview(),ta.item);
			TreeView_SelectSetFirstVisible(ta.treeview(),ta.item);
		}
		//there is insufficient memory
		SOM_MAIN_error("MAIN221",MB_OK); return false;
	}
	else if(!*swap.id) //SOM_MAIN_replaceitem_ghe?
	{
		//should this be entered? takein and mo::move both
		//trigger their asserts? doing this (nothing) here
		//seems to save the MO file correctly

		//2020: I think changing to str from id omits the ""
		//header string that's hitting an assert(string.id)?
		
		rp = rp; //breakpoint
	}
	else if(!SOM_MAIN_right.mo.claim(swap.str)) //swap.id
	{
		if(SOM_MAIN_mmap.takein(msg)&&!msg.itemized())
		{
			if(msg.left) //SOM_MAIN_mo stdctxts (outline suggestions)
			TreeView_SetItemState(SOM_MAIN_tree.left.treeview,msg.left,0,TVIS_CUT);
			TreeView_SetItemState(SOM_MAIN_tree.right.treeview,msg.right,0,TVIS_CUT); 
		}
	}
	else if(!mo::move(SOM_MAIN_right.mo,msg.mmap,msg.id,msg.str))
	assert(0);
	SOM_MAIN_mmap.deallocate(swap);	
	if(msg.left)
	SOM_MAIN_tree.left.redraw(msg.left); 
	SOM_MAIN_tree.right.redraw(msg.right); return true;		
}
static bool SOM_MAIN_replaceitem_ghe(LPARAM cp, LPARAM rp, HWND closing)
{				
	//these scratchpads are packaged into msgstr[0,2]
	SOM_MAIN_tree::string &temp = SOM_MAIN_tree.temp;
	temp.ctxt_s = 0; temp.id_s = 1; temp.id = "\0\0"; 	
	temp.str = SOM_MAIN_gettextheaderentry(cp,rp).c_str();
	temp.str_s = SOM_MAIN_gettextheaderentry().size();
	SOM_MAIN_tree::string &del = SOM_MAIN_tree[rp], el = del;
	if(!mo::nmetadata(SOM_MAIN_right.mo)) del = temp; else
	{
		mo::range_t e; mo::entry(SOM_MAIN_right.mo,e,0); del = e;
	}
	//MEMORY LEAK?
	//WHAT IS THIS DOING EXACTLY? CURRENTLY THE MAIN BODY OF
	//SOM_MAIN_replaceitem FAILS IF THE MSGID IS "" WHICH IS
	//USED BY THE HEADER (I.E. ALL STRINGS THAT ENTERE HERE)
	bool out = SOM_MAIN_replaceitem(SOM_MAIN_temparam(),rp,closing);	
	if(out) temp = del; SOM_MAIN_tree[rp] = el; 
	if(out) SOM_MAIN_gottextheaderentry(temp.str,temp.str_s); 	
	if(out) SOM_MAIN_tree.left.redraw(el.left); 
	if(out) SOM_MAIN_tree.right.redraw(el.right); return out;
}
static HTREEITEM SOM_MAIN_introduceitem(LPARAM cp, HWND tv, HTREEITEM on, bool bh, HTREEITEM rp, int lw)
{
	int lv;
	SOM_MAIN_tree::string &top =
	SOM_MAIN_tree[SOM_MAIN_toplevelparam(tv,on,&lv)];	
	if(lw<lv) lw = lv;
	SOM_MAIN_tree::string *replace = 0;
	LPARAM rparam = SOM_MAIN_param(tv,rp);
	if(rp) replace = &SOM_MAIN_tree[rparam];
	if(!lv||rp==on&&lv==lw&&*replace==SOM_MAIN_tree[cp])
	return SOM_MAIN_replaceitem(cp,SOM_MAIN_param(tv,rp))?rp:0;

	TVINSERTSTRUCTW tis = 
	{TreeView_GetParent(tv,on),0,
	{TVIF_TEXT|TVIF_STATE|TVIF_PARAM|TVIF_CHILDREN,0,0,~0}};		
	SOM_MAIN_textarea ta(tv,on);
	tis.hInsertAfter = ta?0:bh?on:TreeView_GetPrevSibling(tv,on);	
	if(!tis.hInsertAfter) tis.hInsertAfter = TVI_FIRST;	
	if(lv<lw) tis.item.state = TVIS_CUT;
	tis.item.cChildren = 1; tis.item.pszText = LPSTR_TEXTCALLBACKW;		
	if(!replace||*replace!=SOM_MAIN_tree[cp])
	tis.item.lParam = SOM_MAIN_right.insert_param(SOM_MAIN_tree[cp]);
	else tis.item.lParam = rparam;
	
	SOM_MAIN_tree::string del; //C4533
	LPARAM parent_param = SOM_MAIN_param(tv,tis.hParent),
	placement_param = ta?parent_param:SOM_MAIN_param(tv,on);
	SOM_MAIN_tree::string &msg = SOM_MAIN_tree[tis.item.lParam];
	SOM_MAIN_tree::string &parent = SOM_MAIN_tree[parent_param]; 	
	SOM_MAIN_tree::string placement = SOM_MAIN_tree[placement_param];
	assert(!placement.unoriginal());
	if(!parent.itemized()) //grey?
	if(TVIS_CUT&TreeView_GetItemState(tv,parent[tv],~0))
	if(!SOM_MAIN_replaceitem(parent_param,parent_param))
	goto out_of_order;

	//SOM_MAIN_tree::string del; //C4533
	if(rparam==tis.item.lParam) del = msg; //optimizing
	if(!SOM_MAIN_mmap.allocate(msg,parent)) 
	{
		//there is insufficient memory
		SOM_MAIN_error("MAIN221",MB_OK); out_of_order:
		if(rparam!=tis.item.lParam) SOM_MAIN_right.erase_param(tis.item.lParam);
		return 0;
	}
	else if(!del.id&&SOM_MAIN_mmap.takein(msg)&&rp)
	{
		if(SOM_MAIN_mmap.letout(*replace)) SOM_MAIN_mmap.deallocate(*replace);
		else assert(0);
	}
	else if(del.id&&mo::move(SOM_MAIN_right.mo,msg.mmap,msg.id,msg.str))
	{
		SOM_MAIN_mmap.deallocate(del);
	}
	else assert(!del.id);
	
	SOM_MAIN_before_and_after raii;

	HWND vt = SOM_MAIN_tree[tv]->treeview; 
	HTREEITEM pr = replace?(*replace)[vt]:0;
	if(SOM_MAIN_same()) SOM_MAIN_before(vt);	
	SOM_MAIN_textarea at; POINT pt = {0,0}; if(parent[vt])
	at = SOM_MAIN_textarea(vt,TreeView_GetChild(vt,parent[vt]));	
	if(at) TreeView_Expand(vt,parent[vt],TVE_COLLAPSE|TVE_COLLAPSERESET);
	if(ta) TreeView_Expand(tv,parent[tv],TVE_COLLAPSE|TVE_COLLAPSERESET);	
	
	if(rp) //delete old item	
	if(!SOM_MAIN_removeitem(rparam,tis.item.lParam))
	if(tis.hInsertAfter==rp) tis.hInsertAfter = on; replace = 0; 
	HTREEITEM out = SOM_MAIN_insertitem(tv,&tis);
	
	if(placement[vt]) 
	if(!ta||SOM_MAIN_left(tv)) //reproduce item
	{	
		tis.hParent = SOM_MAIN_deduceitem(tv,tis.hParent);		
		if(tis.hInsertAfter!=TVI_FIRST)
		tis.hInsertAfter = SOM_MAIN_deduceitem(tv,tis.hInsertAfter);
		SOM_MAIN_insertitem(vt,&tis);
	}
	else assert(!SOM_MAIN_left(tv));

	HTREEITEM i = out; 
	if(tis.hInsertAfter==TVI_FIRST)
	if(TVIS_CUT&TreeView_GetItemState(tv,tis.hParent,~0)) //phantom business
	{
		msg.outline = parent.outline; SOM_MAIN_cutupitem(tis.item.lParam); 
	}
	else msg.outline = 1; //complicated logic to avoid counting to placement
	else msg.outline = parent_param==placement_param?2:placement.outline+bh;	
	size_t ol = tis.hInsertAfter==TVI_FIRST?2:msg.outline+1;
	if(!ta) for(HTREEITEM i = out;i=TreeView_GetNextSibling(tv,i);ol++)	
	SOM_MAIN_tree[SOM_MAIN_param(tv,i)].outline = ol; 	
	SOM_MAIN_right.outlines[top.outline].addtolevel(lv); assert(top.outline);
	SOM_MAIN_list.left.introduced(lv,msg.right,tis.item.state,lv==lw);
	SOM_MAIN_list.right.introduced(lv,msg.right,tis.item.state,lv==lw);
	if(lv<lw) //self-nesting: eg. li=1-1-1-1...
	{	
		tis.hParent = msg.right; while(lv++<lw) 
		{
			msg.right = tis.hParent = 
			SOM_MAIN_insertitem(SOM_MAIN_right.treeview,&tis);
			SOM_MAIN_right.outlines[top.outline].addtolevel(lv); 
			SOM_MAIN_list.left.introduced(lv,msg.right,LVIS_CUT,0);
			SOM_MAIN_list.right.introduced(lv,msg.right,LVIS_CUT,0);
		}
		tis.item.state = 0; tis.item.hItem = msg.right; 		
		TreeView_SetItem(SOM_MAIN_right.treeview,&tis.item);	
		SOM_MAIN_list.left.introduced(lv,msg.right,0,'sort');
		SOM_MAIN_list.right.introduced(lv,msg.right,0,'sort');
		out = SOM_MAIN_reproduceitem(tv,SOM_MAIN_right.treeview,msg.right);
	}	

	if(rparam) //replaced textarea
	{	
		//todo? repair menus below item

		HWND tvs[2] = {tv,vt}; 
		LONG uds[2] = {(LONG)rp,(LONG)pr};
		for(int i=0;i<2;i++) if(msg[tvs[i]])
		for(HWND ch=GetDlgItem(tvs[i],rparam);ch;)
		{
			if(rparam!=tis.item.lParam) 
			SetWindowLong(ch,GWL_ID,tis.item.lParam);
			if(uds[i]==GetWindowLong(ch,GWL_USERDATA))
			SOM_MAIN_movetext(ch,msg[tvs[i]]);
			if(rparam!=GetWindowID(ch=GetWindow(ch,GW_HWNDNEXT)))
			break;
		}
	}          
	if(at) SOM_MAIN_movetext(at,msg[vt]); //displaced textarea
	if(ta) SOM_MAIN_movetext(ta,msg[tv]);
	return out;
}
static void SOM_MAIN_moveupitem(HWND tv, HTREEITEM onto)
{
	//introduceitem won't work here
	SOM_MAIN_before_and_after raii;
	HWND r = SOM_MAIN_right.treeview;
	HWND l = SOM_MAIN_tree.left.treeview;
	LPARAM param = SOM_MAIN_param(tv,onto);		
	HTREEITEM ri = SOM_MAIN_produceitem(tv,onto);	
	SOM_MAIN_tree::string &msg = SOM_MAIN_tree[param]; if(msg.left)
	{
		HTREEITEM li = SOM_MAIN_reproduceitem(l,r,ri);
		for(;msg.left!=li;msg.left=li) SOM_MAIN_deleteitem(l,msg.left); 
		TreeView_SetItemState(l,msg.left,0,TVIS_CUT);
	}
	int lv; SOM_MAIN_tree::string &top =
	SOM_MAIN_tree[SOM_MAIN_toplevelparam(r,msg.right,&lv)];		
	for(;msg.right!=ri;msg.right=ri) 
	{	
		HTREEITEM i = msg.right;
		while(i=TreeView_GetNextSibling(r,i))
		SOM_MAIN_tree[SOM_MAIN_param(r,i)].outline--;
		SOM_MAIN_right.outlines[top.outline].addtolevel(lv--,-1);
		SOM_MAIN_deleteitem(r,msg.right); 
	}
	TreeView_SetItemState(r,msg.right,0,TVIS_CUT);
}

//static char *SOM_MAIN_copy(...) //in a previous life
char *SOM_MAIN_copy::operator()(std::string &po, LPARAM param, HWND from)
{
	HWND lv153 = from==som_tool?0:from; 	

	SOM_MAIN_tree::string &msg = SOM_MAIN_tree[param];
	
	if(from!=som_tool)
	{
		SOM_MAIN_list.aspects.reselect(param); 
		if(lv153&&ListView_GetSelectedCount(lv153))
		{
			SOM_MAIN_list.aspects.unselect();
			LVITEMW lvi = {LVIF_PARAM|LVIF_STATE,0,0,0,~0};	
			for(int &i=lvi.iItem,n=ListView_GetItemCount(lv153);i<n;i++)
			{
				ListView_GetItem(lv153,&lvi);
				if(~lvi.state&LVIS_SELECTED)
				SOM_MAIN_list[lvi.lParam].selected = 0;
				else if(!SOM_MAIN_list[lvi.lParam].selected)
				SOM_MAIN_list[lvi.lParam].selected = SOM_MAIN_void;
			}		
		}
	}
	else //SOM_MAIN_list.aspects.select(param); 
	{
		if(flushing) //must come before select
		if(msg.undo&&Edit_GetModify(msg.undo)) 
		if(flushing=SOM_MAIN_textarea(msg.undo).close('f5')) //!
		Edit_SetModify(msg.undo,0);

		SOM_MAIN_list.aspects.select(param); 
	}

	int nocontext = msg.nocontext();	

	if(!nostringsattached) 
	{
		const char *p,*q=0,*_4; 

		if(!nocontext)
		if((p=SOM_MAIN_list.aspects.msgctxt.selected)
		 ||(q=SOM_MAIN_list.aspects.msgctxt_short_code.selected)) 
		{
			p = !p||p&&q&&*p=='\v'?q:p;

			po+="msgctxt "; 
			if(_4=strchr(p,'\4'))
			{
				//(char&)*_4 = '\0';
				const char *_n = strchr(p,'\n');
				if(_n&&_n<_4) po+="\"\"\n";
				SOM_MAIN_po_quote(po,p);
				//(char&)*_4 = '\4';
			}
			else po+="\"\\v\"\n"; //SOM_MAIN_void???
		}
		if(p=SOM_MAIN_list.aspects.msgid.selected)
		{
			po+="msgid "; 
			_4 = strchr(p,'\4'); assert(*p);
			if(strchr(_4?_4+1:p,'\n')) po+="\"\"\n";
			SOM_MAIN_po_quote(po,_4?_4+1:p);
		}
		else if(!lv153) po+="msgid \"\"\n"; 
		
		if(nocontext) po+="msgid_plural \"\"\n"; 
	}
	size_t before = po.size();

	po+="msgstr"; bool head = false;

	size_t and_after = po.size();	

	switch(nocontext) //partial plural form
	{
	default: po+='['; po+='0'+nocontext-1; po+=']';
	case 0: po+=' ';	
	}
	
	SOM_MAIN_list::aspects::iterator it;
	SOM_MAIN_list::aspectset::iterator jt;
	struct //REMOVE ME?
	{inline void operator()(std::string &utf, const char *val)
	{
		char del = '\0'; EXML::Quoted qt(del,val);
		
		SOM_MAIN_po_innerquote(utf,qt); qt.unquote(del);
	}	
	}_qt; it = SOM_MAIN_list.aspects.begin();
	for(it++;it!=SOM_MAIN_list.aspects.end();it++)
	{
		if(!it->first.compare("exml")) continue;
		
		bool tagged = false;
		for(jt=it->second.begin();jt!=it->second.end();jt++)			
		{
			if(!jt->selected) continue;

			if(!head++) po+="\"\"\n";
			if(!tagged++){ po+="\"<"; po+=it->first; }

			po+=' '; po+=*jt; 
			if(jt->selected_is_bool()) continue; 
			po+='=';_qt(po,jt->selected);
		}
		if(tagged) po+=">\"\n";
	}
	if(nocontext==1)
	{
		po+="\"\"\n"; goto header_entry;
	}
	else po+="\"<exml"; 
	const char *li,*l=0; 
	if(li=SOM_MAIN_list.aspects.exml.li.selected)	
	{
		l = SOM_MAIN_list.aspects.exml.l.selected;

		po+=l?" ":" li="; //quirks: li=l

		if(!msg.unlisted())
		if(li==SOM_MAIN_exml_li 
		||param!=SOM_MAIN_list.aspects.edited
		||!SOM_MAIN_list.aspects.exml.li.edited())
		{
			HTREEITEM i = msg.right;
			HWND tv = (i?SOM_MAIN_right:SOM_MAIN_tree.left).treeview;
			if(!i) i = msg.left; assert(i);			
			//fyi: copied from/similar to SOM_MAIN_154
			static std::vector<int> ol(5); ol.clear(); char a[32];
			for(LPARAM cut=param,lp;lp=SOM_MAIN_param(tv,i=TreeView_GetParent(tv,i));cut=lp)
			ol.push_back(cut==lp?1:SOM_MAIN_tree[cut].outline); 
			ol.back() = -ol.back();       
			for(size_t i=ol.size();i;i--) po+=itoa(-ol[i-1],a,10);
		}
		else _qt(po,li); else //saving li=l quirk?
		{
			for(const char *p=li,*e;strtod(p,(char**)&e);p=e)
			if(p==li||*p=='-') po.append(p,e-p);
		}
		if(l) //quirks: li=l
		{
			po+='=';_qt(po,l); 
		}
	}	
	SOM_MAIN_list::aspectset
	&exml = SOM_MAIN_list.aspects["exml"];
	for(jt=exml.begin();jt!=exml.end();jt++)			
	{
		if(!jt->selected
		||jt->selected==li
		||jt->selected==l&&li) continue; 

		po+=' '; po+=*jt; 
		if(jt->selected_is_bool()) continue; 
		po+='=';_qt(po,jt->selected);
	}
	po+=">\"\n"; header_entry:

	//todo: don't assume body is 0 terminated
	const char *body = SOM_MAIN_list.aspects.msgstr_body.selected;		
	//NEW: use undo if 0 unless it's modified
	if(!body&&SOM_MAIN_list.aspects.msgstr_body.edited()) body = "";

	if(!body&&!msg.undo&&from!=som_tool)	
	{
		//hack: exclude msgstr if not selected
		if(po.size()-and_after<sizeof("\"<exml>\"\n")) po.resize(before);  
	}
	else if(!body&&msg.undo||msg.undo&&from==som_tool&&Edit_GetModify(msg.undo)) 
	{	
		SOM_MAIN_po_quote(po,SOM_MAIN_peelpoortextext(SOM_MAIN_std::vector<char>(),msg.undo)); 
	}
	else if(body==msg.str&&body&&strpbrk(body,"\\{}"))
	{
		static std::string plain(512,'\0'); plain.clear(); //"<exml>"
		SOM_MAIN_plaintext(plain,body); SOM_MAIN_po_quote(po,plain.c_str());				
	}
	else if(body) SOM_MAIN_po_quote(po,body); 
	return (char*)po.c_str();
}
static bool SOM_MAIN_copytoclipboard(LPARAM param, HWND from)
{
	assert(from); //MessageBox
	char *po = SOM_MAIN_copy(param,from);
	if(!OpenClipboard(som_tool))
	return !SOM_MAIN_error("MAIN217",MB_OK);
	//POINT OF NO RETURN
	SIZE_T len = MultiByteToWideChar(65001,0,po,-1,0,0)+1;
	HANDLE c2c = GlobalAlloc(GMEM_MOVEABLE,len*sizeof(WCHAR));	
	MultiByteToWideChar(65001,0,po,-1,(WCHAR*)GlobalLock(c2c),len);
	GlobalUnlock(c2c);
	EmptyClipboard();
	bool out = //winsanity: c2c is freed and then returned!
	c2c==SetClipboardData(CF_UNICODETEXT,c2c); assert(out);
	CloseClipboard(); if(!out)
	SOM_MAIN_error("MAIN217",MB_OK);
	return out;
}
int SOM_MAIN_tree::pool::copy()
{	
	if(SOM_MAIN_lang(treeview)) 
	return(MessageBeep(-1),0);
	bool x = GetKeyState('X')>>15;
	if(x&&SOM_MAIN_readonly(treeview))
	return !SOM_MAIN_report("MAIN230",MB_OK);
	assert(GetKeyState(VK_CONTROL)>>15);
	HTREEITEM i = TreeView_GetSelection(treeview);
	HTREEITEM gp = TreeView_GetParent(treeview,i);	
	int st = TreeView_GetItemState(treeview,i,~0);
	if(!gp||TVIS_CUT&st) return(MessageBeep(-1),0);
	if(!SOM_MAIN_copytoclipboard(SOM_MAIN_param(treeview,i),som_tool))
	return 0;
	if(x) SOM_MAIN_removeitem(SOM_MAIN_param(treeview,i));
	if(x) SOM_MAIN_changed(TVIS_BOLD&st?-1:+1); 	
	return 1;
}
int SOM_MAIN_tree::pool::paste()
{
	if(SOM_MAIN_lang(treeview)) 
	return(MessageBeep(-1),0);
	if(SOM_MAIN_readonly(treeview))
	return !SOM_MAIN_report("MAIN230",MB_OK);
	assert(GetKeyState(VK_CONTROL)>>15);
	HTREEITEM i = TreeView_GetSelection(treeview);
	HTREEITEM gp = TreeView_GetParent(treeview,i);
	if(!gp) return(MessageBeep(-1),0);
	if(tree[SOM_MAIN_param(treeview,i)].unoriginal())
	return !SOM_MAIN_error("MAIN231",MB_OK);
	LPARAM cp = SOM_MAIN_cliparam(treeview);
	switch(tree[cp].id[tree[cp].ctxt_s])
	{case '\0': case '\v': tree[cp].id_s = 
	strlen(tree[cp].id=SOM_MAIN_generate());}
	if(i=SOM_MAIN_introduceitem(cp,treeview,i,GetKeyState('B')>>15))
	{
		TreeView_SelectItem(treeview,i);
		LPARAM lp = SOM_MAIN_param(treeview,i);
		if(count(tree[lp])>1)
		SOM_MAIN_(153,som_tool_dialog(),SOM_MAIN_153,lp);		
		SOM_MAIN_changed(); return 1;
	}
	return 0; 
}
int SOM_MAIN_textarea::close(int f5)
{
	if(!hwnd||!Edit_GetModify(hwnd)) return -1;
	HWND tv; if(!param||SOM_MAIN_readonly(tv=treeview()))
	{ assert(0); return -1;	} //paranoia
	SOM_MAIN_tree::string &temp = //aliasing
	SOM_MAIN_tree.temp = SOM_MAIN_tree[param]; assert(hwnd==temp.undo); 	
	bool libintl = temp.unoriginal()&&!temp.bilingual();
	const char *etc = libintl?0:temp.exml_text_component();	
	const char *before = etc?etc:libintl?SOM_MAIN_libintl(temp.id):"";	

	size_t headlen = etc?etc-temp.str:temp.str_s;
	std::vector<char> &str = SOM_MAIN_std::vector<char>();	
	if(headlen) str.resize(headlen);
	if(headlen) memcpy(&str[0],temp.str,headlen);	
	if(etc==temp.str) SOM_MAIN_plaintext(str,"<exml>"); 
	const char *after = SOM_MAIN_peelpoortextext(str,hwnd);	
	temp.str = &str[0]; 
	temp.str_s = str.size()-1; assert(str.back()=='\0'); 

	int out = !strcmp(before,after)?-1
	:SOM_MAIN_replaceitem(SOM_MAIN_temparam(),param,f5?0:hwnd);
	if(!f5) //-1: reverse SOM_MAIN_changed(1) added by SOM_MAIN_editest 
	if(out<0||out>0&&TVIS_BOLD&TreeView_GetItemState(tv,temp.right,~0)) 
	SOM_MAIN_changed(-1); return out;
}
static void SOM_MAIN_swap(HWND tv, HTREEITEM i, HTREEITEM ii)
{
	SOM_MAIN_before_and_after raii;
	HTREEITEM gp = TreeView_GetParent(tv,i);
	if(!ii) ii = TreeView_GetNextSibling(tv,i); 
	if(!ii||SOM_MAIN_readonly(tv)){ assert(0); return; } //paranoia
	SOM_MAIN_changed(tv,i); SOM_MAIN_changed(tv,ii); 
	LPARAM first = 0, param = SOM_MAIN_param(tv,i), param2 = SOM_MAIN_param(tv,ii);	
	if(TVIS_CUT&TreeView_GetItemState(tv,gp,~0)) //phantom business
	{
		first = SOM_MAIN_param(tv,TreeView_GetChild(tv,gp));
		if(param==first) SOM_MAIN_cutupitem(first=param2); //!!
		else if(param2==first) SOM_MAIN_cutupitem(first=param);
	}				
	std::swap(SOM_MAIN_tree[param].outline,SOM_MAIN_tree[param2].outline);								
	TVSORTCB cb = {gp,SOM_MAIN_outline_item_sort,first};
	TreeView_SortChildrenCB(tv,&cb,0);								
	HWND vt = SOM_MAIN_tree[tv]->treeview;
	if(cb.hParent=TreeView_GetParent(vt,SOM_MAIN_tree[param][vt]))
	TreeView_SortChildrenCB(vt,&cb,0);	
}
static VOID CALLBACK SOM_MAIN_moverror(HWND win, UINT, UINT_PTR id, DWORD)
{
	KillTimer(win,id); TreeView_EnsureVisible(win,(HTREEITEM)id);
									 
	SOM_MAIN_error("MAIN221",MB_OK); //there is insufficient memory					
}
static HTREEITEM SOM_MAIN_move(HWND tv, HTREEITEM ti, HWND dstv, HTREEITEM dsti)
{
	static std::vector<HWND> text; 
	struct pairam : std::pair<LPARAM,LPARAM>
	{
		pairam(LPARAM f):pair(f,0){} //==: Dinkumware validation vagary
		inline bool operator<(const pairam &b)const{ return first<b.first; }
		inline bool operator==(const pairam &b)const{ return first==b.first; }	
	};
	static std::vector<pairam> repairs; 

	static bool from153; //tv==dstv
							
	bool ta, ro = SOM_MAIN_readonly(tv);	
	HWND l = SOM_MAIN_tree.left.treeview;
	HWND r = SOM_MAIN_right.treeview; HTREEITEM ri = dsti;
	TVINSERTSTRUCTW tis = {ri,TVI_LAST, //defaults
	{TVIF_TEXT|TVIF_PARAM|TVIF_CHILDREN|TVIF_STATE,0,0,TVIS_CUT|TVIS_BOLD}};
	static int depth = 0; if(ta=depth==0) //default
	{
		if(!ti||!dsti) return 0; //nice
		if(ro&&SOM_MAIN_readonly(dstv)) //write-lock
		return (SOM_MAIN_report("MAIN230",MB_OK),0); 

		//POINT OF NO RETURN///////////////////////////		
		if(!ro) SOM_MAIN_before(l); SOM_MAIN_before(r);		

		text.push_back(SOM_MAIN_textarea(dstv,dsti));
		from153 = tv==dstv, ta = !from153&&text.back(); if(ta)
		{
			tis.hInsertAfter = TVI_FIRST;
			tis.hParent = dsti = TreeView_GetParent(dstv,dsti);
			TreeView_Expand(dstv,dsti,TVE_COLLAPSE|TVE_COLLAPSERESET);
		}				
		ri = SOM_MAIN_produceitem(dstv,dsti); if(!ro) 
		{
			//using temp to hack/leverage SOM_MAIN_removeitem
			SOM_MAIN_tree::string &temp = SOM_MAIN_tree.temp;
			temp = SOM_MAIN_tree[SOM_MAIN_param(tv,ti)]; temp.right =
			ti = SOM_MAIN_produceitem(tv,ti); tv = r; assert(SOM_MAIN_same());						
			if(temp.left) temp.left = SOM_MAIN_reproduceitem(l,tv,ti);			
		}			
		if(!ta&&!from153) tis.hParent = TreeView_GetParent(r,ri);
		if(!ta) tis.hInsertAfter = from153?0:TreeView_GetPrevSibling(r,ri);
	}			
	//0: seems to work like LVIF_NORECOMPUTE
	if(!tis.hInsertAfter) tis.hInsertAfter = TVI_FIRST;	
	tis.item.pszText = LPSTR_TEXTCALLBACKW; tis.item.cchTextMax = 0; 			
	bool original = !ro&&!SOM_MAIN_tree.temp.unoriginal();
		
	size_t ol = 1; 
	LPARAM parent_param = 
	SOM_MAIN_param(r,tis.hParent); if(ta) //grey?
	{
		if(!SOM_MAIN_tree[parent_param].itemized())
		if(TVIS_CUT&TreeView_GetItemState(dstv,dsti,~0))
		if(!SOM_MAIN_replaceitem(parent_param,parent_param)) goto out_of_order; 
	}	
	static int tol,tolv, dstol,dstolv; if(depth==0) 
	{
		tol = SOM_MAIN_tree[SOM_MAIN_toplevelparam(tv,ti,&tolv)].outline;
		dstol = SOM_MAIN_tree[SOM_MAIN_toplevelparam(dstv,dsti,&dstolv)].outline;		
		if(ta) dstolv++;
	}			
	//ii/i/ti: left/right/multiuse with tv
	HTREEITEM ii,&i = tis.item.hItem = ti;
	HTREEITEM err = 0; ti = TreeView_GetParent(tv,ti);
	bool x = !ro&&original, y = ro||tol!=dstol||!original;		
	bool first_cut = TVIS_CUT&TreeView_GetItemState(tv,ti,~0);
	do if(TreeView_GetItem(tv,&tis.item)) 
	{	
		if(ol==1&&x) //x: xferring/deleting
		if(SOM_MAIN_tree[tis.item.lParam].left)
		ii = SOM_MAIN_reproduceitem(l,r,i); else ii = 0; 
		if(y) if(!first_cut||depth==0) //y: copying
		{
			static SOM_MAIN_tree::string dup; 
			dup.base() = SOM_MAIN_tree[tis.item.lParam]; 			
			if(!SOM_MAIN_mmap.allocate(dup,SOM_MAIN_tree[parent_param])) 
			{
				err = ri; //there is insufficient memory
				if(depth==0) SOM_MAIN_error("MAIN221",MB_OK); 
				if(x&&ol>1)	for(size_t l=1;i;i=TreeView_GetNextSibling(tv,i))
				SOM_MAIN_tree[SOM_MAIN_param(tv,i)].outline = l++; 
				goto out_of_order; 
			}
			else dup.mmap = -1; //paranoia
			if(!SOM_MAIN_mmap.takein(dup)) assert(0);
			if(x) repairs.push_back(tis.item.lParam); 
			if(x) SOM_MAIN_right.erase_param(tis.item.lParam);
			tis.item.lParam = SOM_MAIN_right.insert_param(dup);
			if(x) repairs.back().second = tis.item.lParam;			
		}
		else if(first_cut&&depth>0) //phantom business
		{
			first_cut = false; tis.item.lParam = parent_param;
		}		

		SOM_MAIN_tree::string &dst = SOM_MAIN_tree[tis.item.lParam];

		dst.outline = ol++;		
		if(tis.item.lParam==parent_param) 
		dst.outline = SOM_MAIN_tree[parent_param].outline; //phantom
		else if(!ro) dst.left = dst.right = 0;		
		if(depth==0) tis.item.state&=~TVIS_CUT;
		if(TVIS_BOLD&~tis.item.state) SOM_MAIN_changed(1);
		if(ro) tis.item.state|=TVIS_BOLD;		
		ri = SOM_MAIN_insertitem(r,&tis);		
		ti = TreeView_GetChild(tv,i); if(ti) //recursive? 
		{
			HWND t = SOM_MAIN_textarea(tv,ti); if(!t)			
			{
				depth++; err = SOM_MAIN_move(tv,ti,r,ri);
				depth--;				
				//SetTimer: let the caller do its business
				if(err&&depth==0) SetTimer(dstv,(UINT_PTR)
				SOM_MAIN_reproduceitem(dstv,r,err),0,SOM_MAIN_moverror);			
			}
			else if(x) //replacing textarea
			{
				SetWindowLongPtr(t,GWLP_USERDATA,(LONG_PTR)ri);
				text.push_back(t);				
			}
		}	

		if(depth==0) goto remove; //phantom business
		
		ti = i,i = TreeView_GetNextSibling(tv,i); if(x)
		{
			if(err) if(depth==0) assert(0); //one & done
			else for(size_t l=1;i=TreeView_GetNextSibling(tv,i);)
			SOM_MAIN_tree[SOM_MAIN_param(tv,i)].outline = l++;
			if(from153)	
			{
				SOM_MAIN_list.left.removed(tolv+depth,ti);
				SOM_MAIN_list.right.removed(tolv+depth,ti);
				SOM_MAIN_list.left.introduced(dstolv+depth,ri,tis.item.state,0);
				SOM_MAIN_list.right.introduced(dstolv+depth,ri,tis.item.state,0);
			}
			TreeView_DeleteItem(tv,ti); if(ii) //do left?
			{
				ti = ii,ii = TreeView_GetNextSibling(l,ii); 
				TreeView_DeleteItem(l,ti); 
			}
		}

	}while(i); out_of_order: 
	
	//remove: leave top item's outline stats to SOM_MAIN_removeitem below	
	if(x) SOM_MAIN_right.outlines[tol].addtolevel(tolv+depth,-int(ol-1));	
	remove: SOM_MAIN_right.outlines[dstol].addtolevel(dstolv+depth,ol-1);
	
	if(depth==0&&ol==2) //delete top item
	{
		if(x) SOM_MAIN_removeitem(SOM_MAIN_temparam(),SOM_MAIN_temparam());					
		//reminder: has to follow SOM_MAIN_removeitem
		if(from153)	SOM_MAIN_list.left.introduced(dstolv,ri,tis.item.state,'sort');
		if(from153)	SOM_MAIN_list.right.introduced(dstolv,ri,tis.item.state,'sort');
		if(x) for(size_t i=ta,repairsorted=0;i<text.size();i++) if(text[i])
		{
			if(!repairsorted++) std::sort(repairs.begin(),repairs.end());

			std::vector<pairam>::iterator it;
			LPARAM oldparam = SOM_MAIN_textarea(text[i]).param;			
			it = std::find(repairs.begin(),repairs.end(),oldparam);
			if(it!=repairs.end()) if(it->second) //paranoia
			SetWindowLong(text[i],GWL_ID,it->second); else assert(0);

			SOM_MAIN_movetext(text[i]); //replaced textarea
		}
		HWND tb = 0; if(!ro&&!from153) //reproduce item
		{
			SOM_MAIN_tree::string &dstop =
			SOM_MAIN_tree[SOM_MAIN_param(dstv,dsti)]; if(dstop.left)
			{							
				tis.hParent = SOM_MAIN_reproduceitem(l,r,tis.hParent);
				if(!ta||TVIS_EXPANDED&TreeView_GetItemState(l,tis.hParent,~0))
				{
					if(tis.hInsertAfter!=TVI_FIRST)
					tis.hInsertAfter = SOM_MAIN_reproduceitem(l,r,tis.hInsertAfter);
					if(ta) tb = SOM_MAIN_textarea(l,TreeView_GetChild(l,tis.hParent));
					if(tb) TreeView_Expand(l,tis.hParent,TVE_COLLAPSE|TVE_COLLAPSERESET);
					SOM_MAIN_insertitem(l,&tis);								
				}
			}
		}					
		if(tb) SOM_MAIN_movetext //displaced textarea
		(tb,SOM_MAIN_reproduceitem(l,r,SOM_MAIN_tree[tis.item.lParam].right)); 
		if(ta) SOM_MAIN_movetext(text[0],SOM_MAIN_tree[tis.item.lParam].right); 				
	}
	if(depth==0) ////////////////////////POINT OF NO RETURN//
	{		
		if(!ro) SOM_MAIN_and_after(l); SOM_MAIN_and_after(r);		
		repairs.clear(); text.clear();	

		return ol!=2?0:SOM_MAIN_reproduceitem(dstv,r,ri);
	}
	else return err;
}

//TODO? create as tmp/rename som_tool extension
static bool SOM_MAIN_save_as(const wchar_t *path, int format)
{
	if(SOM_MAIN_readonly())
	return !SOM_MAIN_report("MAIN230",MB_OK);
	if(!path&&!format)
	path = Sompaste->get(L".MO"); /*save*/retry:
	FILE *f = _wfopen(path,L"wb"); if(!f) return false;
	//POINT OF NO RETURN//
	SOM_MAIN_quiet = true;
	SOM_MAIN_copy.flushing = !format;
	SOM_MAIN_copy.nostringsattached = true;		
	char *str; size_t str_s;	
	struct{void operator()(LPARAM param)
	{str = SOM_MAIN_copy(SOM_MAIN_std::string(),param);
	//todo? handle plural strings
	while(*str!='"'&&*str) str++; assert(*str);
	str_s = *str?SOM_MAIN_po_unquote(str+=1):0; 
	}char* &str; size_t &str_s;
	}copy_machine = {str,str_s};
	//reminder: msgfmt has -a --alignment
	//todo: may want to align the strings
	//not sure. however for SOM_MAIN_mmap
	//it assumes the strings are airtight	
	mo::range_t r; mo::range(SOM_MAIN_right.mo,r);
	static std::vector<ule32_t> header(8+4+r.n*4);
	size_t i = 10; if(format<2) //MO
	{	
		//2020: copy_machine modifies (flushes) ghe, double appending?
		//std::string &ghe = 
		SOM_MAIN_gettextheaderentry(); //borrowing
		std::string ghe;
		copy_machine(SOM_MAIN_right.metadata),ghe.assign(str,str_s+1);
		copy_machine(SOM_MAIN_right.messages),ghe.append(str,str_s+1);
		copy_machine(SOM_MAIN_right.japanese),ghe.append(str,str_s);

		//rounding to 64 bits
		const int head64 = 8; //7
		header.resize(head64+4+r.n*4);	
		header[0] = 0x950412de; 
		header[2] = 1+r.n;
		header[3] = head64*4;
		header[4] = head64*4+8+r.n*8;
		header[8] = 1; 
		header[9] = head64*4+8*2+r.n*8*2;				
		for(size_t e=0;e<r.n;e++,i+=2)
		{
			//msgfmt --alignment=1
			header[i] = r.idof[e].clen;
			header[i+1] = header[i-1]+header[i-2]+1;
		}
		header[i] = ghe.size();
		header[i+1] = header[i-1]+header[i-2]+1; i+=2;
		fwrite(&header[0],4*header.size(),1,f);
		fwrite("\0",2,1,f); 
		for(size_t e=0;e<r.n;e++)
		fwrite(mo::msgctxt(r,e),r.idof[e].clen+1,1,f);
		fwrite(ghe.c_str(),ghe.size()+1,1,f);
	}
	else //assuming PO
	{
		std::string &po = SOM_MAIN_std::string();
		po+="msgid \"\"\n" "msgid_plural \"\"\n"; 
		SOM_MAIN_copy(po,SOM_MAIN_right.metadata);
		SOM_MAIN_copy(po,SOM_MAIN_right.messages);
		SOM_MAIN_copy(po,SOM_MAIN_right.japanese);
		po+='\n'; 
		fwrite(po.c_str(),po.size(),1,f);
	}

	//at this point the header and header-entry are// 
	//filled out, including the indices, except for//
	//the msgstr index is 0s after its header entry//

	struct output //cache locality
	{			
		LPARAM entry, group, score;
		output(){ entry = group = score = 0; }
		//==: Dinkumware validation vagary
		inline bool operator<(const output &b)const
		{
			return group!=b.group?group<b.group:score<b.score;
		}
		inline bool operator==(const output &b)const
		{
			return group==b.group&&score==b.score;
		}
	};
	static std::vector<output> outvec(r.n); //sorting
	outvec.resize(r.n); //final size

	LPARAM highscore = 1;
	HWND tv = SOM_MAIN_right.treeview;	
	HTREEITEM gc,ti,tli = TreeView_GetChild(tv,TVI_ROOT); 
	//-1/+=2: this is a trick so 0 includes outline-less
	//strings that are not opened up into the tree, where
	//as -1 will include "messages" if they've been opened
	//and 1 include "japanese" if open (overriding group 0)			
	for(LPARAM param,g=-1;ti=tli;g+=2,tli=TreeView_GetNextSibling(tv,tli))	
	if(TVIS_CUT&~TreeView_GetItemState(tv,tli,~0)) 
	if(gc=TreeView_GetChild(tv,tli)) do //walking?
	{			
		param = SOM_MAIN_param(tv,ti);
		SOM_MAIN_tree::string &msg = SOM_MAIN_tree[param];
		size_t e = msg.mmap-r.lowerbound; if(e<r.n) 
		{
			outvec[e].group = g;
			outvec[e].entry = param; outvec[e].score = highscore++; 
		}
		if(gc&&!SOM_MAIN_textarea(tv,gc))
		{
			ti = gc; gc = TreeView_GetChild(tv,ti);
		}
		else while(ti&&ti!=tli) if(gc=TreeView_GetNextSibling(tv,ti))
		{
			ti = gc; gc = TreeView_GetChild(tv,ti); break;
		}
		else ti = TreeView_GetParent(tv,ti);

	}while(ti&&ti!=tli); else //optimizing
	{
		SOM_MAIN_tree::string &msgctxt = 
		SOM_MAIN_tree[SOM_MAIN_param(tv,tli)]; if(!msgctxt.nocontext())
		{
			mo::range_t rr; 				
			if(mo::range(SOM_MAIN_right.mo,rr,msgctxt.id,msgctxt.ctxt_s))
			for(size_t e=rr.lowerbound-r.lowerbound,e_s=e+rr.n;e<e_s;e++)
			outvec[e].group = g;
		}
	}		
	//optimizing: default to stored cache locality
	for(size_t e=0;e<r.n;e++) if(!outvec[e].score)
	outvec[outvec[e].entry=e].score = highscore+(LPARAM)mo::msgstr(r,e);
	std::sort(outvec.begin(),outvec.end());	
	LPARAM lp, fp = ftell(f);	
	new(&SOM_MAIN_tree.temp) SOM_MAIN_tree::string; //optimizing
	for(size_t e=0;e<r.n;e++)
	{
		output &o = outvec[e]; if(o.score>=highscore) //optimizing
		{	
			lp = SOM_MAIN_temparam(); SOM_MAIN_tree.temp = r[o.entry];			
		}
		else lp = o.entry,o.entry = SOM_MAIN_tree[lp].mmap-r.lowerbound; 

		if(format<2) //MO
		{
			copy_machine(lp);
			header[i+2*o.entry] = str_s;
			header[i+2*o.entry+1] = fp; fp+=str_s+1;
			fwrite(str,str_s+1,1,f);
		}
		else //assuming PO
		{
			std::string &po = SOM_MAIN_std::string();
			const char *_4,*_n,*id = SOM_MAIN_tree[lp].id;
			if(_4=strchr(id,'\4'))
			{
				po+="msgctxt ";
				_n = strchr(id,'\n');
				if(_n&&_n<_4) po+="\"\"\n";
				SOM_MAIN_po_quote(po,id); id = _4+1;
			}
			po+="msgid "; 			
			if(strchr(id,'\n')) po+="\"\"\n";
			SOM_MAIN_po_quote(po,id);
			SOM_MAIN_copy(po,lp);
			po+='\n';
			fwrite(po.c_str(),po.size(),1,f);
		}
	}
	if(format<2) //MO
	if(!fseek(f,header[4],SEEK_SET))
	fwrite((char*)&header[0]+header[4],8+r.n*8,1,f);	
	outvec.clear();
	header.clear();
	if(!format&&SOM_MAIN_copy.flushing) //flushed?
	SOM_MAIN_changed(-10000); 
	SOM_MAIN_copy.nostringsattached = false;	
	SOM_MAIN_copy.flushing = false;
	SOM_MAIN_quiet = false;
	if(ferror(f)|fclose(f)) //disk error?
	if(IDRETRY!=SOM_MAIN_error("MAIN202",MB_RETRYCANCEL))
	return false; else goto retry;
	return true;
}

static VOID CALLBACK 
SOM_MAIN_bewithviewshortly(HWND win,UINT,UINT_PTR id,DWORD)
{
	KillTimer(win,id); if(!win) //fake modal flash
	{
		if(win=som_tool_dialog()) MessageBeep(-1); 		
		FLASHWINFO f = {sizeof(f),win,FLASHW_ALL,5,75};
		if(win) FlashWindowEx(&f),SetForegroundWindow(win);
	}
	else SOM_MAIN_create(win,TreeView_GetSelection(win),0);	
}
static void SOM_MAIN_readytoserveview(HWND escb)
{
	if(!SOM::MO::view||!SOM::MO::view->clientext) return;

	if(!IsWindowEnabled(som_tool)) //fake a flash
	{
		SOM::MO::view->clientext = 0;	
		SetTimer(0,'view',0,SOM_MAIN_bewithviewshortly); return;
	}

	HWND ct = //note: setting clientext to 0
	SOM::MO::view->clientext; SOM::MO::view->clientext = 0;	
	HWND gp = GetParent(ct); SOM_MAIN_onedge = 0;
		
	//positioning requires singular compact model
	int iwv = IsWindowVisible(som_tool); if(!iwv) 
	{
		RECT rc,cr,wr; //place on whichever side has more room
		if(GetClientRect(som_tool,&rc)&&rc.right-rc.left<=640)
		{		
			GetClientRect(gp,&cr);
			GetClientRect(ct,&rc); MapWindowRect(ct,gp,&rc);
			som_tool_recenter(som_tool,ct); GetWindowRect(som_tool,&wr);
			int onright = rc.right+8;
			int onleft = rc.left-8-(wr.right-wr.left); swapsides:			
			POINT lp = {rc.left<cr.right-rc.right?onright:onleft,0}; 
			SOM_MAIN_onedge = lp.x==onright?1:-1;
			MapWindowPoints(gp,0,&lp,1);
			SetWindowPos(som_tool,0,lp.x,wr.top,0,0,SWP_NOSIZE|SWP_NOZORDER);
			SendMessage(som_tool,DM_REPOSITION,0,0);	
			if(onleft<onright&&GetWindowRect(som_tool,&wr)&&wr.left!=lp.x)
			{
				std::swap(onleft,onright); goto swapsides;
			}			
			if(escb) //put in Esc minimizes mode
			{
				Button_SetCheck(escb,0);
				windowsex_enable<1081>(som_tool,0);
			}
		}
	}

	HWND tv = SOM_MAIN_right.treeview; 
	HWND tv2 = SOM_MAIN_tree.left.treeview;
	if(SOM_MAIN_leftonly()) tv = tv2;
	else if(SOM_MAIN_same()) 
	if(!IsWindowEnabled(tv)) tv = tv2;
	else if(IsWindowEnabled(tv2))
	{
		int i=0,x[3]; HWND w[3]={ct,tv,tv2};
		for(RECT wr;i<3&&GetWindowRect(w[i],&wr);i++) 
		x[i] = wr.left+(wr.right-wr.left)/2; //centers
		if(abs(x[0]-x[2])<abs(x[0]-x[1])) tv = tv2;
	}
	SetFocus(tv); //important	
	SOM_MAIN_tree::pool &pool = 
	SOM_MAIN_same()?SOM_MAIN_right:SOM_MAIN_tree[tv];
	
	const char *ctxt = ""; 	
	DWORD hlp = GetWindowContextHelpId(gp); 
	switch(hlp)
	{
	case 31100: ctxt = "PRM0\4"; break; //items
	case 32100: ctxt = "PRM1\4"; break; //shops
	case 33100: ctxt = "PRM2\4"; break; //magic
	case 34100: ctxt = "PRM3\4"; break; //objects
	case 35200:
	case 35300: ctxt = "PRM4\4"; break; //enemies
	case 36100: ctxt = "PRM5\4"; break; //NPCs	
	case 44000:	ctxt = "MAP0\4"; break; //maps
	case 45100: ctxt = "MAP1\4"; break; //events
	case 46100: 
	case 47700: ctxt = "MAP2\4"; break; //programs
	case 45200: ctxt = "MAP3\4"; break; //counters   		
	case 53100: ctxt = "SYS0\4"; break; //classes
	case 55100: ctxt = "SYS1\4"; break; //messages
	case 56200: ctxt = "SYS2\4"; break; //slideshows
	}
	assert(*ctxt); if(!*ctxt) return;	
	int lim = 30, lim2 = 40*6; 
	int compile[sizeof(long)==4];
	switch(_byteswap_ulong(*(long*)ctxt))
	{	
	case 'MAP2': lim = 40; lim2 = 40*7; break;
	case 'SYS0': lim = 14; break;
	case 'SYS1': lim = 40; break;
	case 'SYS2': lim2 = 48*16; break;
	}		
	//WM_GETTEXT is cross-process	
	char text932[5+48*16+1] = "";
	//2* for CRLF filled scenario
	//static is for SOM_MAIN_153_1009_text
	static wchar_t text[2*sizeof(text932)]; 
	int n; for(n=0;ctxt[n];n++) text[n] = ctxt[n];
	n+=SendMessageW(ct,WM_GETTEXT,EX_ARRAYSIZEOF(text)-n,(WPARAM)(text+n));
	for(int i=0,j=0;i<=n;i++) if(text[i]!='\r') text[j++] = text[i];	
	EX::need_ansi_equivalent(932,text,text932,sizeof(text932));
	text932[5+(ES_MULTILINE&GetWindowStyle(ct)?lim2:lim)] = '\0';
	EX::need_unicode_equivalent(932,text932,text,sizeof(text932));			
	char *utf8 = SOM_MAIN_utf16to8(text); retry:
	SOM_MAIN_tree::pool::iterator it = pool.find(utf8);
	if(utf8[5]&&it!=pool.end())
	{
		HWND f = it->undo; DWORD _=0;
		if(f&&GetParent(f)==tv) _=1; else f = 0;
		if(!f) SOM_MAIN_clearaway(tv,(*it)[tv]);
		if(!f) f = SOM_MAIN_cascade 
		(tv,SOM_MAIN_reproduceitem(tv,SOM_MAIN_right.treeview,it->right)); 		
		SOM_MAIN_textarea ta(f); assert(ta);
		if(_) _ = ta.menu()+1; if(_) Edit_SetSel(f,_,_);
		TreeView_SelectSetFirstVisible(tv,ta.item);
		//if(f) SetFocus(f); 
		SetTimer(f,'view',0,SOM_MAIN_focus); return;
	}
	if(utf8[5]) it = pool.find(ctxt); if(it!=pool.end())
	{
		if(!SOM_MAIN_readonly(tv))
		{			
			if(!TreeView_GetChild(tv,(*it)[tv]))		
			{
				const char *fast = 0; //optimizing
				if(mo::gettext(pool.mo,utf8,fast))
				if(SOM_MAIN_outline(tv,(*it)[tv])) goto retry;
			}								
			SOM_MAIN_153_1009_text = text+5;		
			SOM_MAIN_153_1009_clientext = ct;
			//SOM_MAIN_create(tv,(*it)[tv],0);
			TreeView_SelectItem(tv,(*it)[tv]); 
			SetTimer(tv,'view',iwv?0:300,SOM_MAIN_bewithviewshortly);
			return;
		}
		SOM_MAIN_report("MAIN230",MB_OK); 
	}	
	SOM_MAIN_error("MAIN199",MB_OK);
}

//extern: som_tool_erasebkgnd assistant 
extern RECT *SOM_MAIN_divider(HWND win)
{
	static RECT out; switch(GetWindowContextHelpId(win))
	{
	default: return 0;
	case 10300: return SOM_MAIN_152_divider(out)?&out:0;
	case 10500: return SOM_MAIN_154_divider(out)?&out:0;
	case 11200: out = SOM_MAIN_172_divider; return &out;
	}
}

extern void SOM_MAIN_reprogram()
{
	//New Project sometimes loads a COM-like interface 
	//to get the amount of /free space on the drive volume??? 
	//this force it to fall back to going through GetDiskFreeSpaceA
	//saw change in a single login session after be asked to look into it
	//(REMINDER: this is only required because SOMEX_(B) is given to SOM_MAIN)*	
	*(BYTE*)0x4015D1 = 0xEB; //je->jmp

	//2018 splash screen finding
	//DOING THIS SO som_main_LoadBitmapA is unnecessary
	//
	//for years the splash screen wouldn't appear, until
	//one day late in 2018 it began to appear only when
	//opening the "SOM_MANU" script editor from within a
	//hosting tool???
	/*
	00407820 83 EC 18             sub         esp,18h  
	00407823 56                   push        esi  
	00407824 8B F1                mov         esi,ecx  
	00407826 E8 DB DF 01 00       call        00425806  
	0040782B 8B 40 0C             mov         eax,dword ptr [eax+0Ch]  
	0040782E 68 90 00 00 00       push        90h  
	00407833 50                   push        eax  
	00407834 FF 15 CC B4 42 00    call        dword ptr ds:[42B4CCh]
	*/
	//here is the test/jump that bypasses the splash screen
	/*
	00407726 A1 34 8D 43 00       mov         eax,dword ptr ds:[00438D34h]  
	0040772B 85 C0                test        eax,eax  
	0040772D 74 7A                je          004077A9
	*/
	//407700 is the subroutine that sets this memory (to 1)
	//(this code is unentered if the splash screen doesn't appear)
	/*
	00402FC0 8B 4C 24 10          mov         ecx,dword ptr [esp+10h]  
	00402FC4 51                   push        ecx  
	00402FC5 E8 36 47 00 00       call        00407700  
	00402FCA 83 C4 04             add         esp,4  
	*/
	//clearly? this must be to suppress the splash screen
	//for SOM_EDIT. It must think that SOM_EX is SOM_EDIT
	*(BYTE*)0x40772D = 0xEB; //je->jmp
}