
#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

#include <set>
#include <deque>
#include <string>
#include <algorithm>

#include "Som.h" 

#ifdef NDEBUG //EX_UNFINISHED
#include "SomEx.h"
#endif

static size_t SOM_EDIT_cbsorted = 1;

static struct SOM_EDIT_tree //singleton
{
	//These helper routines don't return const
	//wchar_t pointers because they are mainly used
	//to set TVITEMEXW's const-unqualified pszText member
	
	typedef std::wstring stringbase;

	struct string : public stringbase
	{	
		string(){};
		string(const wchar_t *p):stringbase(p){};

		wchar_t *margin()
		{
			return (wchar_t*)c_str(); 
		}
		wchar_t *print()
		{
			wchar_t *out = margin();
			while(iswspace(*out)) out++; return out;
		}
		size_t printegral()
		{
			size_t out = 1;
			for(wchar_t *p=print();*p;p++) if(*p=='\n') out++;
			if(out>1&&at(size()-1)=='\n') out--;
			return out;
		}
		wchar_t *property()
		{
			wchar_t *out = uncut();
			return *out=='.'||*out=='#'?out+1:out;
		}
		wchar_t *uncut() //skip ; and blank lines
		{
			wchar_t *out = (wchar_t*)c_str();
			for(size_t i=0,j;npos!=(i=find(L'=',i));i++)			
			if(npos==(j=rfind_first_of(L";\n",i))) return out;
			else if(at(j)=='\n') return out+j+1;
			//void assignment & remarks
			size_t last = rfind(L'\n');
			if(npos==last) last = 0; else last++;
			if(iswspace(out[last])||out[last]==';') return out+size();
			return out+last; 
		}
		size_type rfind_first_of(const stringbase &str, size_type off)
		{
			if(off>size()) off = size(); //rfind
			reverse_iterator start = rend()-off-1, found = 
			std::find_first_of(start,rend(),str.begin(),str.end());
			return found==rend()?npos:off-(found-start);
		}
		void rename(wchar_t *name)
		{
			if(!*name) name = L"ERROR";
			size_t i,set = property()-margin();
			for(i=set;i<size()&&at(i)!='=';i++); 
			replace(set,i-set,name); 
		}
		void reveal()
		{
			size_t set = uncut()-margin();
			if(set<size()&&at(set)=='#'||at(set)=='.')
			replace(set,1,L"");
		}
		void conceal()
		{
			size_t set = uncut()-margin();
			replace(set,at(set)=='#'||at(set)=='.'?1:0,L"#");
		}
		void self_correct() //settings
		{
			size_t set = uncut()-margin();

			//repair margin with semicolons
			for(size_t ii=0,i=0;i<set;ii=i+=2) 
			{
				while(i<set&&at(i)!='\r'&&iswspace(at(ii))) i++;
				if(at(i)!='\r'&&at(ii)!=';'){ replace(ii,0,L";"); set++; }
				while(i<set&&at(i)!='\r') i++;
			}

			size_t i, sep = set; //= uncut()-margin();

			while(sep<size()&&at(sep)!='=') sep++; if(sep!=size())
			{
				for(i=sep;i&&iswspace(at(i-1))&&at(i-1)!='\n';i--);
				erase(i,sep-i); sep = i;
				for(i=sep+1;i<size()&&iswspace(at(i))&&at(i)!='\r';i++);
				erase(sep+1,i-(sep+1));
			}
									
			//remove any and all trailing whitespace 
			while(set<size()&&iswspace(at(size()-1))) resize(size()-1);

			if(set==size()&&set) //all comments
			{
				replace(set,0,L"\r\nERROR"); set+=2; sep+=7;
			}
			else if(sep==set) //zero sized name
			{
				replace(set,0,L"ERROR"); sep+=5;
			}

			if(sep>=size()-1||at(sep+1)!='\r') //single?
			{
				//there may still be more than one line
				for(i=sep+1;i<size()&&at(i)!='\r';i++); 

				if(i>=size()) //single, with separator?
				{						
					if(sep+1==size()) resize(size()-1); return; //!
				}
				else replace(sep+1,0,L"\r\n"); //multi!
			}
			append(L"\r\n"); //multi-line

			//repair blank lines with semicolons
			for(size_t ii=i=sep+3;i<size();ii=i+=2)
			{
				while(at(i)!='\r'&&iswspace(at(ii))) i++;
				if(at(i)=='\r') replace(ii,0,L";");				
				while(at(i)!='\r') i++;
			}
		}		
		
		struct sortcb 
		{
			size_t order; 

			sortcb(){ order = SOM_EDIT_cbsorted++; }

			sortcb(const sortcb&){ new (this) sortcb; }

		}sortcb;

	}temp; //must be left in an empty state

	struct less : public std::less<string> //std::wstring
	{			
		bool operator()(const string &a, const string &b)const
		{
			int i; for(i=0;a[i]==b[i]&&a[i]!='='&&b[i]!='=';i++);
			return a[i]<b[i];
		}
	};
										   
	typedef std::set<string,less> strings; 
	
	strings dict; std::set<std::wstring> userdict; 

	template<int N> void fill(wchar_t (&with)[N])
	{
		for(int i=0,j=0,k=0;i<N;i++) switch(with[i])
		{		
		case '\0': i = N-1; 
		case '\r': case '\n': with[i] = '\0';
					
			if(k>j) dict.insert(with+j); j = i+1;

		case '=': k = i;
		}		
	}

	wchar_t *operator[](const wchar_t *text)
	{
		text+=*text=='#'||*text=='.';
		wchar_t *out = (wchar_t*)translation(text);
		if(text==out) userdict.insert(text); 
		return out;
	}
	const wchar_t *translation(const wchar_t *text)
	{
		temp+=text; temp+='=';
		strings::iterator it = dict.find(temp); 
		temp.clear();

		if(it!=dict.end()) 
		{
			const wchar_t *x = it->c_str(); 

			while(*x!='=') x++; if(x[1]) return x+1;
		}
		return text; 
	}
	int dict_if_1_userdict_if_2(const wchar_t *text)
	{	
		temp+=L"-";	//assume variable
		if(text!=translation(text)) return 1;
		if(userdict.find(text)!=userdict.end()) return 2;
		return 0;
	}

	typedef std::deque<string> poolbase;
		
	//treeview memory pool
	struct pool : poolbase
	{
		//higher turnover pool
		poolbase vars_strings;

		inline void clear()
		{
			poolbase::clear(); 

			vars_strings.clear();

			eof = false; remarks = 0;
		}
		bool eof; LPARAM remarks; 

		inline LPARAM back_param() 
		{				
			return (LPARAM)&back();
		}
		inline LPARAM vars_strings_back_param() 
		{				
			return (LPARAM)&vars_strings.back();
		}

		//top level item 
		inline HTREEITEM file()
		{
			return TreeView_GetChild(treeview,TVI_ROOT);
		}
		inline HTREEITEM vars()
		{
			return TreeView_GetNextSibling(treeview,file());
		}
		inline HTREEITEM zone()
		{
			return TreeView_GetNextSibling(treeview,vars());
		}

		HWND treeview;		

	}left, right, *focus; //per treeview
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
	inline string &operator[](LPARAM lp) 
	{
		return lp?*(string*)lp:temp; //L""
	}
	inline void swap_params(LPARAM lp, LPARAM lp2) 
	{
		if(lp!=right.remarks&&lp2!=right.remarks)
		std::swap(((string*)lp)->sortcb.order,((string*)lp2)->sortcb.order);
	}

}SOM_EDIT_tree; //singleton

SOM_EDIT_tree::pool &SOM_EDIT_right = SOM_EDIT_tree.right;

static int CALLBACK SOM_EDIT_sortcb(LPARAM lp1, LPARAM lp2, LPARAM remarks)
{
	if(lp1==remarks) return 1; if(lp2==remarks) return -1; assert(lp1!=lp2);

	return SOM_EDIT_tree[lp1].sortcb.order<SOM_EDIT_tree[lp2].sortcb.order?-1:1;
}

static void SOM_EDIT_inicb(const wchar_t *kv[2], void *vp)
{
	SOM_EDIT_tree::pool &pool = *(SOM_EDIT_tree::pool*)vp;

	const wchar_t *Remarks = L"~File";

	if(!kv[1]) 
	{
		if(!kv[0]||*kv[0]=='[')
		{
			if(pool.eof=!kv[0])
			{
				kv[0] = Remarks; kv[1] = L""; 
			}
			else pool.push_back(kv[1]);		

			pool.remarks = (LPARAM)&pool.back();
		}
		else return; //don't expand environment variables
	}
	else if(pool.remarks) 
	{		
		if(kv[0]) //phantom section?
		{
			pool.back()+=kv[0]; 
		
			if(*kv[1]) pool.back()+L"=";
		}

		pool.back()+=kv[1]; return;
	}
	else if(!kv[0]) //margin
	{			
		pool.push_back(kv[1]); return;
	}

	SOM_EDIT_tree::string &body = pool.back();

	TVINSERTSTRUCTW tis = 
	{pool.file(),TVI_LAST,{TVIF_TEXT|TVIF_PARAM|TVIF_STATE,0,0,~0}};
		
	//hack: prepend - if not ~File
	int cut = *kv[0]=='.'||*kv[0]=='#';
	if(!pool.eof) SOM_EDIT_tree.temp = L"-";
	tis.item.pszText = (wchar_t*)SOM_EDIT_tree[kv[0]+cut];
	tis.item.lParam = pool.back_param(); 
	tis.item.state = cut?TVIS_CUT:0;
		
	if(pool.eof) //notes
	{
		if(Remarks==tis.item.pszText) //~File?
		tis.item.pszText = L"Remarks"; //better
	}
	else //key/value pair
	{
		body+=kv[0]; 
		
		if(*kv[1]) //void assignment?
		{
			body+=L"="; body+=kv[1];

			if(*kv[1]=='\r') body+=L"\r\n"; //block assignment
		}
	}

	tis.hParent = 
	TreeView_InsertItem(pool.treeview,&tis);	
	tis.item.pszText = L""; //0
	tis.itemex.mask|=TVIF_INTEGRAL; 
	tis.itemex.iIntegral = body.printegral();
	TreeView_InsertItem(pool.treeview,&tis);		
}

static HWND SOM_EDIT_151() //overload
{
	return GetParent(SOM_EDIT_right.treeview);
}
static LPARAM SOM_EDIT_param(HWND tv, HTREEITEM item)
{				
	TVITEMEXW ti = {TVIF_PARAM,item};	
	if(!TreeView_GetItem(tv,&ti)) return 0;
	return ti.lParam;
}
static HTREEITEM SOM_EDIT_param2(HWND lv, int i=-1)
{	
	if(i==-1) i = ListView_GetNextItem(lv,-1,LVNI_SELECTED);
	LVITEMW lvi = {LVIF_PARAM,i,0};			
	if(!ListView_GetItem(lv,&lvi)) return 0;
	return (HTREEITEM)lvi.lParam;
}
static void SOM_EDIT_saved();
//HWND: preventing unnecessary writes
static HWND SOM_EDIT_savefile_tv = 0;
//TODO? create som as tmp/rename som_tool extension
static bool SOM_EDIT_savefile(const wchar_t temp[MAX_PATH]=0, HWND tv=0)
{	
	if(!tv) tv = SOM_EDIT_right.treeview;

	//optimizing away redundant reads
	if(temp&&SOM_EDIT_savefile_tv==tv) return true; 
	if(temp) SOM_EDIT_savefile_tv = tv;

	wchar_t som[MAX_PATH] = L""; 
	int cbid = tv==SOM_EDIT_right.treeview?1013:1011;
	GetDlgItemTextW(SOM_EDIT_151(),cbid,som,MAX_PATH);

	if(!temp) temp = som; if(!*temp) return false; retry:

	HTREEITEM i = TreeView_GetChild(tv,SOM_EDIT_tree[tv].file());

	if(!i) return false; //assuming collapsed

	FILE *f = _wfopen(temp,L"wb"); if(!f) return false;

	//todo: preserve the file's Unicode encoding
	wchar_t bom = 0xfeff; bool out = fwrite(&bom,2,1,f);

	//note: assuming all strings are well-formed
	for(;i&&out;i=TreeView_GetNextSibling(tv,i))
	{
		if(temp==som) TreeView_SetItemState(tv,i,0,TVIS_BOLD);
		std::wstring &ws = SOM_EDIT_tree[SOM_EDIT_param(tv,i)];
		size_t ws_s = ws.size(); if(!ws_s) continue;
		if(out=fwrite(ws.c_str(),ws_s*sizeof(wchar_t),1,f))
		out = fwrite(L"\r\n",2*sizeof(wchar_t),1,f);
	}
	if(!out||fclose(f)) //disk error?
	if(IDRETRY==MessageBoxA(0,"EDIT202","SOM_EDIT",MB_RETRYCANCEL|MB_ICONERROR))
	goto retry; else return false;
		
	Som_h(som,Sompaste->get(L".NET"),temp);
	Sompaste->set(L".SOM",temp);

	if(som==temp&&tv==SOM_EDIT_right.treeview)
	SOM_EDIT_saved();
	return true;
}
static void SOM_EDIT_savefilereset(HWND tv)
{
	if(tv==SOM_EDIT_savefile_tv) SOM_EDIT_savefile_tv = 0;
}
static const wchar_t *SOM_EDIT_tempfile(HWND tv=0)
{
	static EX::temporary temp; SOM_EDIT_savefile(temp,tv);
	return temp;
}
static bool SOM_EDIT_program(wchar_t inout[MAX_PATH])
{
	wchar_t def[MAX_PATH+32] = L""; DWORD sizeof_def = sizeof(def);
	if(!SHGetValueW(HKEY_CLASSES_ROOT,inout,L"",0,def,&sizeof_def))
	{
		wcscat_s(def,L"\\shell\\open\\command"); 
		if(!SHGetValueW(HKEY_CLASSES_ROOT,def,L"",0,def,&(sizeof_def=sizeof(def))))
		{
			wchar_t *p = def; while(iswspace(*p)) p++; if(*p=='"')
			{
				for(wchar_t *q=++p;*q;q++) if(*q=='"') *q-- = '\0';
			}
			else for(wchar_t *q=p;*q;q++) if(iswspace(*q)) *q-- = '\0';
			ExpandEnvironmentStringsW(p,inout,MAX_PATH);
			return *inout;
		}
	}
	return false;
}
static void SOM_EDIT_preview()
{
	wchar_t prog[MAX_PATH] = L".INI";
	if(!SOM_EDIT_program(prog)) SOM_EDIT_program(wcscpy(prog,L".TXT"));
	ShellExecuteW(SOM_EDIT_151(),L"open",prog,SOM_EDIT_tempfile(),0,1);
}
static void SOM_EDIT_collapsereset(HWND tv)
{
	SOM_EDIT_savefilereset(tv); 
	HTREEITEM item = TreeView_GetChild(tv,TVI_ROOT);
	do TreeView_Expand(tv,item,TVE_COLLAPSE|TVE_COLLAPSERESET);
	while(item=TreeView_GetNextSibling(tv,item));
	SOM_EDIT_tree[tv].clear();
}
static void SOM_EDIT_changed(int change=1) //PSM_CHANGED
{	
	static int change_counter = 0;
	bool enable = (change_counter+=change)>0;
	windowsex_enable<12321>(SOM_EDIT_151(),enable); 
	if(change_counter<0) change_counter = 0;
	if(abs(change)<=1) //-1,0,1
	{
		//collapse the SOM file dependent items
		HWND r = SOM_EDIT_right.treeview; SOM_EDIT_savefilereset(r); 
		HTREEITEM item = TreeView_GetNextSibling(r,SOM_EDIT_right.file());
		do TreeView_Expand(r,item,TVE_COLLAPSE|TVE_COLLAPSERESET);
		while(item=TreeView_GetNextSibling(r,item));
		SOM_EDIT_right.vars_strings.clear();
	}	
}
static void SOM_EDIT_changed(HWND tv, HTREEITEM parent, HTREEITEM child=0)
{
	if(child) parent = TreeView_GetParent(tv,child); assert(parent);

	SOM_EDIT_changed(TVIS_BOLD&TreeView_GetItemState(tv,parent,~0)?0:1);	
}
static void SOM_EDIT_saved()
{
	return SOM_EDIT_changed(-100000);
}
static int SOM_EDIT_level(HWND tv, HTREEITEM item)
{
	int out = 0; while(item=TreeView_GetParent(tv,item)) out++; return out;
}
static void SOM_EDIT_center(HWND dialog, HWND over)
{		
	RECT a, b; GetWindowRect(over,&a);
	GetWindowRect(dialog,&b); OffsetRect(&b,-b.left,-b.top);
	int cx = a.left+(a.right-a.left)/2, cy = a.top+(a.bottom-a.top)/2;
	MoveWindow(dialog,cx-b.right/2,cy-b.bottom/2,b.right,b.bottom,0);
	SendMessage(dialog,DM_REPOSITION,0,0);
}
extern HWND som_tool_richinit(HWND,int);
extern const wchar_t *som_tool_richedit_class();
extern int som_tool_drawitem(DRAWITEMSTRUCT*,int,int=-1);
static void SOM_EDIT_refresh(HWND,HTREEITEM,HTREEITEM=0);
static LRESULT CALLBACK SOM_EDIT_RichEditProc(HWND,UINT,WPARAM,LPARAM,UINT_PTR,HLOCAL);
static INT_PTR CALLBACK SOM_EDIT_153(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//assumes right tree!
	LPARAM &remarks = SOM_EDIT_right.remarks;
	
	//mode 1 is for notes
	//mode 2 is for lines
	static LPARAM param = 0, modes = 0, cut = 0, v;

	//L" ": WHAT THE HELL??? IF EMPTY THE CURSORS
	//STOP BLINKING FOR THE _OTHER_ EDIT CONTROLS
	const wchar_t *wtf = L"\r";	//L" ";

	static CHARFORMAT2W cf; //NEW
	static HWND editor = 0; //Rich Edit (ID-less)
		
	const int eco = //NEW: Rich Edit
	ECO_AUTOVSCROLL|ECO_AUTOHSCROLL|ECO_SAVESEL;

	switch(uMsg)
	{	
	case WM_NOTIFY: //vertical scrollbar
	{
		REQRESIZE *rr = (REQRESIZE*)lParam;		
		if(rr->nmhdr.code==EN_REQUESTRESIZE)
		{
			RECT cr = {0,0,0,0};
			GetClientRect(editor,&cr);						
			if(cr.bottom<rr->rc.bottom-rr->rc.top) 
			{
				if(!v) v = 1; else break; 
			}
			else if(v) v = 0; else break;			
			ShowScrollBar(editor,SB_VERT,v);
			RECT refresh = Sompaste->pane(editor,hwndDlg);
			//this is the only thing that works	
			InvalidateRect(hwndDlg,&refresh,1);
			ValidateRect(editor,0);
			UpdateWindow(hwndDlg);
			/////////
			return 1; //override som_tool_subclassproc
		}
		break;
	}
	case WM_INITDIALOG:
	{
		param = lParam;	
		modes = cut = 0;
		SOM_EDIT_tree::string 
		&str = SOM_EDIT_tree[param];
		wchar_t *uncut = str.uncut();
		wchar_t *margin = str.margin();
		wchar_t *setting = L"";
		if(param==remarks)
		{				
			int dis[] = {1008,1009,1010,1011};			
			windowsex_enable(hwndDlg,dis,0);
			//remarks being if a section is encountered
			wchar_t *section = wcsstr(margin,L"\r\n[");
			modes = 1; //annotation mode
			uncut = setting = 
			*margin=='['?margin:section?section+2:uncut;			
		}
		else //standard operating procedure
		{
			if(*uncut=='#'||*uncut=='.') cut = *uncut;
			wchar_t *prop = uncut+!!cut; //property();
			while(iswspace(*prop)) prop++;
			wchar_t *sep = wcschr(prop,'=');
			if(sep) *sep = '\0';			
			SetWindowTextW(hwndDlg,*prop?prop:L"ERROR"); 
			if(sep) *sep = '=';	
			//2: single/wrapping mode
			modes = sep&&sep[1]=='\r'?0:2;
			setting = sep?sep+(modes&2?1:3):L"";
		}
		
		HWND window = GetDlgItem(hwndDlg,1000);		
		HWND macros = GetDlgItem(hwndDlg,1002);
		HWND status = GetDlgItem(hwndDlg,1003);
		
		const wchar_t *text = setting;
		wchar_t _uncut = *uncut; *uncut = '\0';
		const wchar_t *text2 = margin==uncut?L"":margin;
		if(modes&1) std::swap(text,text2);				
		//editor's text is set below
		//SetWindowTextW(editor,text); 
		//wtf! see remarks on Windows bug above
		SetWindowTextW(status,*text2?text2:wtf); 
		*uncut = _uncut; 
		//pull out semicolons 
		HLOCAL lock = (HLOCAL)
		SendMessageW(status,EM_GETHANDLE,0,0);				
		wchar_t *q = (wchar_t*)LocalLock(lock);
		for(wchar_t *p=q,*pp=p;*p;p++) 
		if(*p!=';'||p>pp&&p[-1]!='\n') *q++ = *p; *q = '\0';
		LocalUnlock(lock);

		//rewritten for Rich Edit approach
		int wr = ~modes&2?ECO_WANTRETURN:0; 
		int es = ES_MULTILINE|ES_DISABLENOSCROLL|ES_NOOLEDRAGDROP
		|WS_CHILD|WS_TABSTOP|WS_GROUP|WS_VISIBLE|WS_HSCROLL|WS_VSCROLL;
		/*HWND*/editor = CreateWindowExW(0,som_tool_richedit_class(),text,es,0,0,1,1,hwndDlg,0,0,0);										
		SetWindowSubclass(editor,(SUBCLASSPROC)SOM_EDIT_RichEditProc,0,0);
		SendMessage(editor,EM_SETOPTIONS,ECOOP_SET,eco|wr);
		SendMessage(editor,EM_SETTARGETDEVICE,0,modes&2?1:0);			
		som_tool_richinit(editor,ENM_REQUESTRESIZE); 				
						
		//scrollbars/macros window
		SetWindowTheme(editor,L"",L""); 
		ShowScrollBar(editor,SB_HORZ,1);
		ShowScrollBar(editor,SB_VERT,v=0);
		RedrawWindow(editor,0,0,RDW_INVALIDATE|RDW_FRAME); 
		//insert editor and macros panel into dummy window		
		WINDOWINFO wi = {sizeof(wi)}; GetWindowInfo(window,&wi);
		RECT rc; GetWindowRect(macros,&rc);	int cy = rc.bottom-rc.top;
		rc = wi.rcClient; rc.bottom-=cy; MapWindowRect(0,hwndDlg,&rc);
		SetWindowPos(editor,window,rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,0);
		//single: was a second/wrapping Edit control set above the horizontal scrollbar
		//SetWindowPos(single,editor,rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top-GetSystemMetrics(SM_CYHSCROLL),0);
		MoveWindow(macros,rc.left,rc.bottom,rc.right-rc.left,cy,0);	
		OffsetRect(&rc,-rc.left,-rc.top); InflateRect(&rc,-3,-2);
		Edit_SetRect(editor,&rc); Edit_SetRect(status,&rc);						
		EnableWindow(macros,0); //unimplemented									
		
		HWND gp = GetParent(hwndDlg);
		SOM_EDIT_center(hwndDlg,gp==SOM_EDIT_151()?SOM_EDIT_right.treeview:gp);
		return 1;
	}
	case WM_DRAWITEM: 
	{				 			  		
		int st = -1;
		switch(wParam)
		{
		case 1008: st = modes&2; break;
		case 1009: st = ~modes&2; break;
		case 1010: st = modes&1; break;
		case 1011: st = ~modes&1; break;
		}
		return som_tool_drawitem((DRAWITEMSTRUCT*)lParam,141,st);
	}
	case WM_COMMAND:
	{
		switch(wParam)
		{
		case MAKEWPARAM(0,EN_SETFOCUS):

			//this way if Enter is pressed in single
			//mode the single mode button is focused
			return SendMessage(hwndDlg,DM_SETDEFID,1008,0);

		case MAKEWPARAM(0,EN_KILLFOCUS):

			return SendMessage(hwndDlg,DM_SETDEFID,IDOK,0);

		//single / multi-line
		case 1008: case 1009: 
		{
			assert(~modes&1);

			int single = wParam&1?0:2;

			if(single==(modes&2)) //no change
			{
				//is default button pressed?
				if(GetKeyState(VK_RETURN)>>15)
				SetFocus(GetDlgItem(hwndDlg,wParam));
				break; 
			}

			HLOCAL lock = Edit_GetHandle(editor);			
			wchar_t *text = (wchar_t*)LocalLock(lock);
			if(single) //trim trailing whitespace
			{
				size_t i, len = wcslen(text);
				while(len&&iswspace(text[len-1])) len--;
				for(i=0;i<len;i++) if(text[i]=='\n')
				{	
					LocalUnlock(lock); //!!

					//note: it is still possible to Copy
					//line endings into single line mode
					
					MessageBeep(-1); //todo: MessageBoxA
					return 0; 
				}
				text[len] = '\0';
			}			
			LocalUnlock(lock);
			SendMessage(editor,EM_SETTARGETDEVICE,0,single?0:1);				
			int wr = single?0:ECO_WANTRETURN;
			SendMessage(editor,EM_SETOPTIONS,ECOOP_SET,eco|wr);			
			modes = modes&~2|single;
			RedrawWindow(GetDlgItem(hwndDlg,1008),0,0,RDW_INVALIDATE);
			RedrawWindow(GetDlgItem(hwndDlg,1009),0,0,RDW_INVALIDATE);
			break;
		}
		//annotations/setting
		case 1010: case 1011:
		{				
			int margin = wParam&1?0:1;

			if(margin==(modes&1)) break; //no change
			
			HWND status = GetDlgItem(hwndDlg,1003);
	
			LockWindowUpdate(hwndDlg);

			//NEW: using temp as swap buffer
			//(was using the single Edit control)
			HLOCAL lock = Edit_GetHandle(editor);
			SOM_EDIT_tree.temp = (wchar_t*)LocalLock(lock);
			LocalUnlock(lock);
			lock = (HLOCAL)Edit_GetHandle(status);
			wchar_t *text = (wchar_t*)LocalLock(lock);
			SetWindowTextW(editor,wcscmp(text,wtf)?text:L"");
			LocalUnlock(lock);
			text = (wchar_t*)SOM_EDIT_tree.temp.c_str();
			SetWindowTextW(status,*text?text:wtf);
			LocalUnlock(lock);
			SOM_EDIT_tree.temp.clear();				

			modes = modes&~1|margin;
			windowsex_enable(hwndDlg,1008,1009,!margin);
			RedrawWindow(GetDlgItem(hwndDlg,1010),0,0,RDW_INVALIDATE);			
			RedrawWindow(GetDlgItem(hwndDlg,1011),0,0,RDW_INVALIDATE);						
			RedrawWindow(hwndDlg,0,0,RDW_NOERASE);
			LockWindowUpdate(0);
			//break;			
			//scrollbars aren't being painted
			//SCROLLBARINFO sbi = {sizeof(sbi)};
			//if(GetScrollBarInfo(editor,OBJID_HSCROLL,&sbi))
			RedrawWindow(editor,0,0,RDW_INVALIDATE|RDW_FRAME);
			//else assert(0);
			break;
		}
		case IDOK: //IDCANCEL if unchanged 			
		{
			HWND setting = editor; 
			HWND margin = GetDlgItem(hwndDlg,1003);			
			if(modes&1) std::swap(margin,editor);

			HLOCAL lock = Edit_GetHandle(margin);
			HLOCAL lock2 = Edit_GetHandle(setting);

			wchar_t *text = (wchar_t*)LocalLock(lock);
			wchar_t *text2 = (wchar_t*)LocalLock(lock2);
			
			std::wstring &temp = SOM_EDIT_tree.temp;			
			temp.clear(); //paranoia

			//reinsert semicolons
			if(!wcscmp(text,wtf)) text = L"";
			for(wchar_t *next=text;next;text=next) 
			{
				next = wcschr(text,'\r');
				int i,len = next?next-text:wcslen(text);
				for(i=0;i<len&&iswspace(text[i]);i++);
				if(i<len) temp.push_back(';');
				temp.append(text,len); 
				if(!next) break;
				temp+=L"\r\n";
				next+=2;
			}
			//ensure margin is separate
			if(!temp.empty()&&temp[temp.size()-1]!='\n') temp+=L"\r\n";
			//insert title and separator
			if(param!=remarks)
			{
				if(cut) temp+=cut;
				wchar_t title[60] = L""; 			
				GetWindowTextW(hwndDlg,title,EX_ARRAYSIZEOF(title));
				temp+=title;	
				temp+=modes&2?L"=":L"=\r\n";			
			}
			//append value/sections for remarks
			temp+=text2;	
			
			LocalUnlock(lock); LocalUnlock(lock2);
			
			bool changed = 
			temp!=SOM_EDIT_tree[param];
			if(changed)
			{
				SOM_EDIT_tree[param] = temp.c_str();
				if(param!=remarks)
				SOM_EDIT_tree[param].self_correct();				
			}
			temp.clear();
			if(changed)
			{
				//REMOVE ME??
				//DestroyWindow(hwndDlg);
				//EndDialog(hwndDlg,IDOK);
				HWND tv = SOM_EDIT_tree->treeview;
				HTREEITEM item = TreeView_GetSelection(tv);
				SOM_EDIT_refresh(tv,item); SOM_EDIT_changed(tv,item);
				DestroyWindow(hwndDlg);
				return 1;
			}
			//break; //unchanged
		}
		case IDCANCEL: goto close;
		}
		break;
	}
	case WM_CLOSE: close:
	{
		//REMOVE ME??
		//DestroyWindow(hwndDlg);
		//EndDialog(hwndDlg,IDCANCEL); 
		DestroyWindow(hwndDlg);
		return 1;
	}}

	return 0;
}
static LRESULT CALLBACK SOM_EDIT_RichEditProc
(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM len, UINT_PTR scID, HLOCAL lh)
{	
	switch(uMsg)
	{	
	case WM_KEYUP:
	case WM_KEYDOWN: //ES_WANTRETURN

		if(wParam==VK_RETURN) 
		if(ECO_WANTRETURN&~SendMessage(hWnd,EM_GETOPTIONS,0,0))
		{
			HWND dlg = GetAncestor(hWnd,GA_ROOT);
			int defid = SendMessage(dlg,DM_GETDEFID,0,0);
			if(HIWORD(defid)==DC_HASDEFID) 
			SendDlgItemMessage(dlg,LOWORD(defid),BM_CLICK,0,0);
			return 1;
		}
		break;

	//REMOVE ME?
	case EM_GETHANDLE: //emulate Edit control 
		
		len = GetWindowTextLengthW(hWnd)+1;
		if(!lh||LocalSize(lh)<sizeof(WCHAR)*len)
		{
			if(!lh) lh = LocalAlloc(LMEM_MOVEABLE,0);			
			lh = LocalReAlloc(lh,sizeof(WCHAR)*len,LMEM_MOVEABLE);
			SetWindowSubclass(hWnd,(SUBCLASSPROC)SOM_EDIT_RichEditProc,scID,(DWORD_PTR)lh);
		}
		GetWindowTextW(hWnd,(WCHAR*)LocalLock(lh),len);
		LocalUnlock(lh);
		return (LRESULT)lh;
	 
	case WM_NCDESTROY: 

		RemoveWindowSubclass(hWnd,(SUBCLASSPROC)SOM_EDIT_RichEditProc,scID);		
		LocalFree(lh);
		break;
	}		

	return DefSubclassProc(hWnd,uMsg,wParam,len); //lParam
}
static void SOM_EDIT_relist(HWND lv, HWND tv, HTREEITEM i, int sel)
{
	int nsel = sel==-1?0:
	sel+ListView_GetSelectedCount(lv); //NEW
	if(nsel==sel) nsel++;

	HWND gp = GetParent(lv);
	SetWindowRedraw(gp,0); //important
	ListView_DeleteAllItems(lv);
	const int label_s = 60;	wchar_t label[label_s];
	LVITEMW lvi = {LVIF_TEXT|LVIF_PARAM|LVIF_STATE,0,0,0,~0};	 		
	TVITEMEXW tvi = {TVIF_PARAM|TVIF_TEXT|TVIF_STATE,0,0,~0};			
	tvi.pszText = label; tvi.cchTextMax = label_s;
	for(tvi.hItem=TreeView_GetChild(tv,i)
		;tvi.hItem&&TreeView_GetItem(tv,&tvi)
		;lvi.iItem++)
	{	
		//do first for remarks
		lvi.lParam = (LPARAM)tvi.hItem;
		tvi.hItem = TreeView_GetNextSibling(tv,tvi.hItem);
		if(!tvi.hItem) break; //remarks

		lvi.iSubItem = 0;			
		lvi.state = tvi.state&TVIS_CUT;
		if(lvi.iItem>sel&&lvi.iItem<nsel) lvi.state|=LVIS_SELECTED;
		lvi.pszText = LPSTR_TEXTCALLBACKW;			
		ListView_InsertItem(lv,&lvi);		
		lvi.iSubItem = 1; 
		//could be avoided if translated items were marked
		//Reminder: fix LVN_ENDLABELEDITW if this is fixed
		wchar_t *kv = SOM_EDIT_tree[tvi.lParam].property();			
		wchar_t *sep = wcschr(kv,'=');
		if(sep) *sep = '\0'; 
		if(!wcscmp(kv,label)) lvi.pszText = 0; //no description
		if(sep) *sep = '='; 
		SendMessageW(lv,LVM_SETITEMTEXTW,lvi.iItem,(LPARAM)&lvi);
	}	
	if(sel!=-1) //LVN_ITEMCHANGED
	{
		ListView_SetItemState(lv,sel,~0,LVIS_SELECTED|LVIS_FOCUSED);
		NMLISTVIEW geez = {{lv,1026,LVN_ITEMCHANGED},
		sel,0,LVIS_SELECTED,0,LVIF_STATE,{0,0},(LPARAM)SOM_EDIT_param2(lv,sel)};
		SendMessage(gp,WM_NOTIFY,1026,(LPARAM)&geez);
	}
	SetWindowRedraw(gp,1); //maintain selection
	InvalidateRect(gp,0,0);			
}
static void SOM_EDIT_refresh(HWND tv, HTREEITEM parent, HTREEITEM child)
{
	if(!child) child = TreeView_GetChild(tv,parent);
	if(!parent) parent = TreeView_GetParent(tv,child);
	
	int param = SOM_EDIT_param(tv,parent); 
	int state = TreeView_GetItemState(tv,parent,~0);

	TVITEMEXW ti = {TVIF_INTEGRAL,child};	
	TreeView_GetItem(tv,&ti); //testing
	int testing = ti.iIntegral;
	ti.iIntegral = SOM_EDIT_tree[param].printegral();
	TreeView_SetItem(tv,&ti);
	
	if(param!=SOM_EDIT_tree[tv].remarks)
	{
		wchar_t *label = 
		SOM_EDIT_tree[param].uncut();
		bool cut = *label=='.'|*label=='#'; 
		if(cut!=bool(state&TVIS_CUT))
		{
			TreeView_SetItemState(tv,parent,~state,TVIS_CUT);
			TreeView_SetItemState(tv,child,~state,TVIS_CUT);			
		}		
		label+=cut; //relabeling
		wchar_t *sep = wcschr(label,'=');		
		if(sep) *sep = '\0';		
		ti.hItem = parent; 
		ti.mask = TVIF_TEXT;		
		SOM_EDIT_tree.temp = L"-"; //hack
		ti.pszText = SOM_EDIT_tree[label];
		SendMessageW(tv,TVM_SETITEMW,0,(LPARAM)&ti);
		if(sep) *sep = '=';		
	}

	//Windows issues:
	//there are issues with the treeview if this 
	//is not done. Deleting/reinserting the item
	//will probably also work (SOM_MAIN.cpp says
	//so) but isn't implemented/requires sorting
	if(state&TVIS_EXPANDED) if(ti.iIntegral!=testing)
	{
		LockWindowUpdate(tv); //isn't flickering

		HTREEITEM sel = TreeView_GetSelection(tv);

		TreeView_Expand(tv,parent,TVE_COLLAPSE); 
		TreeView_Expand(tv,parent,TVE_EXPAND);

		if(sel==child) TreeView_SelectItem(tv,child);

		LockWindowUpdate(0); 
	}
}
static HTREEITEM SOM_EDIT_insert(HTREEITEM root, HTREEITEM before, LPARAM param)
{
	HTREEITEM out = 0;
	HWND tv = SOM_EDIT_right.treeview;
	LockWindowUpdate(tv); 
	TVINSERTSTRUCTW tvis = 
	{root,TVI_LAST,{TVIF_TEXT|TVIF_PARAM,0,0,0}};	
	SOM_EDIT_tree.temp = L"-";
	tvis.item.pszText = SOM_EDIT_tree[SOM_EDIT_tree[param].property()];
	wchar_t *sep = wcschr(tvis.item.pszText,'=');
	if(sep) *sep = '\0'; 	
	tvis.item.lParam = param;		
	//go ahead and insert as a means to get at the last item's handle
	out = TreeView_InsertItem(tv,&tvis);
	if(before) for(HTREEITEM i=out;i!=before&&i;)
	{	
		i = TreeView_GetPrevSibling(tv,i); assert(i);
		SOM_EDIT_tree.swap_params(SOM_EDIT_param(tv,i),param);
	}	 	
	if(sep) *sep = '='; 
	tvis.hParent = out; tvis.item.pszText = L""; //0; 
	TreeView_InsertItem(tv,&tvis);
	TVSORTCB scb = {root,SOM_EDIT_sortcb,SOM_EDIT_right.remarks};
	TreeView_SortChildrenCB(tv,&scb,0);
	TreeView_SelectItem(tv,out);
	LockWindowUpdate(0); 
	return out;
}
static INT_PTR CALLBACK SOM_EDIT_152(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HTREEITEM root;

	//assuming right tree!
	HWND &tv = SOM_EDIT_right.treeview;	

	switch(uMsg)
	{
	case WM_INITDIALOG: 
	{
		root = (HTREEITEM)lParam;
		HWND lv = GetDlgItem(hwndDlg,1026);
		int lvs = GetWindowStyle(lv);	
		SetWindowLong(lv,GWL_STYLE,lvs&~LVS_SINGLESEL);
		ListView_SetExtendedListViewStyle(lv,LVS_EX_FULLROWSELECT|LVS_EX_LABELTIP);
		RECT room; GetClientRect(lv,&room);
		LVCOLUMNW lvc = {LVCF_TEXT|LVCF_WIDTH,0,65,L"??"};
		ListView_InsertColumn(lv,0,&lvc);
		lvc.cx = room.right-=lvc.cx; 
		ListView_InsertColumn(lv,1,&lvc);
		SOM_EDIT_relist(lv,tv,root,0);		
		SOM_EDIT_center(hwndDlg,SOM_EDIT_right.treeview);
		return 1;
	}		
	case WM_DRAWITEM: 
	{
		int st = -1;
		if(wParam==1010||wParam==1011) //seesaw		
		{
			st = 0;
			HWND lv = GetDlgItem(hwndDlg,1026);
			int sel = ListView_GetNextItem(lv,-1,LVNI_SELECTED);
			if(sel!=-1)
			if(-1==ListView_GetNextItem(lv,sel,LVNI_SELECTED))
			{
				st = ListView_GetItemState(lv,sel,LVIS_CUT);
				if(wParam==1010) st = !st;
			}
			else st = -1; //multi-selection mode
		}
		return som_tool_drawitem((DRAWITEMSTRUCT*)lParam,141,st);
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
			TVITEMEXW tvi = {TVIF_TEXT|TVIF_PARAM};	
			tvi.pszText = p->item.pszText; 
			tvi.cchTextMax = p->item.cchTextMax;
			tvi.hItem = (HTREEITEM)p->item.lParam;
			if(TreeView_GetItem(tv,&tvi))
			if(!p->item.iSubItem)
			{
				wchar_t *kv = SOM_EDIT_tree[tvi.lParam].property();
				wchar_t *sep = wcschr(kv,'=');
				if(sep) *sep = '\0'; 
				wcscpy_s(p->item.pszText,p->item.cchTextMax,kv);
				if(sep) *sep = '='; 
			}
			break;			
		}
		case LVN_ITEMCHANGED:			
		{
			NMLISTVIEW *p = (NMLISTVIEW*)lParam;

			if(p->uChanged&LVIF_STATE)
			if(LVIS_SELECTED&(p->uNewState^p->uOldState))
			{	
				int sc = ListView_GetSelectedCount(lv);
				bool ms = sc>1, ss = !ms&&sc;
				if(!ms&&LVIS_SELECTED&p->uNewState)
				TreeView_SelectItem(tv,(HTREEITEM)p->lParam);  
				int raise = 1027, lower = 1028;							
				int first = ListView_GetNextItem(lv,-1,LVNI_SELECTED), last = first;
				for(int i=last;i!=-1;i=ListView_GetNextItem(lv,last=i,LVNI_SELECTED))
				if(i-last>1) raise = lower = 0; //non-contiguous	
				if(!raise||first==0) 
				windowsex_enable<1027>(hwndDlg,raise=0);
				if(!lower||last==ListView_GetItemCount(lv)-1)
				windowsex_enable<1028>(hwndDlg,lower=0);
				int single[] = {1009,1211}; 
				int multi[] = {1010,1011,raise,lower,1210}; 								
				windowsex_enable(hwndDlg,multi,sc);				
				windowsex_enable(hwndDlg,single,ss);
				windowsex_enable<1008>(hwndDlg,!ms);
				RedrawWindow(GetDlgItem(hwndDlg,1010),0,0,RDW_INVALIDATE);
				RedrawWindow(GetDlgItem(hwndDlg,1011),0,0,RDW_INVALIDATE);				
			}
			break;
		}
		case NM_CLICK: break; //unimplemented by som_tool_listviewproc

		case LVN_ITEMACTIVATE:
		{
			NMITEMACTIVATE *p = (NMITEMACTIVATE*)lParam;
			TreeView_SelectItem(tv,SOM_EDIT_param2(lv,p->iItem));
			SendMessage(SOM_EDIT_151(),WM_COMMAND,1211,0);							
			break;
		}		
		case LVN_ENDLABELEDITW:
		{	
			HTREEITEM param2 = SOM_EDIT_param2(lv);
			LPARAM param = SOM_EDIT_param(tv,param2);
			if(!p->item.pszText)
			if(SOM_EDIT_tree[param].size()) break;
			else p->item.pszText = L"ERROR";
			SOM_EDIT_tree[param].rename(p->item.pszText);
			SOM_EDIT_changed(tv,param2);
			SOM_EDIT_refresh(tv,param2);
			//in case description is filled/blanked 
			SOM_EDIT_relist(lv,tv,root,p->item.iItem);
			SetWindowLong(hwndDlg,DWL_MSGRESULT,0);
			return 1;
		}
		/*2018: LOOKS LIKE som_tool_subclassproc HAS THIS COVERED?
		case NM_CUSTOMDRAW:
		{
			NMCUSTOMDRAW *p = (NMCUSTOMDRAW*)lParam;			

			switch(p->dwDrawStage) //assuming listview
			{
			case CDDS_PREPAINT: //want per item behavior

				SetWindowLong(hwndDlg,DWL_MSGRESULT,CDRF_NOTIFYITEMDRAW);
				return 1;

			case CDDS_ITEMPREPAINT: 

				NMLVCUSTOMDRAW *q = (NMLVCUSTOMDRAW*)lParam;

				int state = //using LVIS_CUT mask doesn't work???
				ListView_GetItemState(lv,p->dwItemSpec,~0);					

				//SELECTED is always true for all!
				//if(CDIS_SELECTED&~p->uItemState)
				if(LVIS_SELECTED&~state)
				if(LVIS_CUT&state) //if(LVIS_CUT&p->uItemState)
				{	
					q->clrText = GetSysColor(COLOR_GRAYTEXT);	
					SetWindowLong(hwndDlg,DWL_MSGRESULT,CDRF_DODEFAULT);
					return 1;
				}
				break;				
			}
			break;
		}*/}
		break;
	}
	case WM_COMMAND:
	{
		switch(LOWORD(wParam))
		{	
		case IDCLOSE:
		case IDCANCEL: goto close;

		case 1012: SOM_EDIT_preview(); break;
		}

		int newselist = -1;
		LPARAM swapchain = 0;
		HWND lv = GetDlgItem(hwndDlg,1026);
		int sel = ListView_GetNextItem(lv,-1,LVNI_SELECTED);
		if(sel==-1)
		{
			//HACK: Skipping lvi initialiazing...
			__pragma(warning(disable:4533)) //2021
			if(1008==LOWORD(wParam)) goto insert; break;
		}
		else while(sel!=-1) //NEW: multi-selection
		{
			//todo: rewrite with SOM_EDIT_param/param2
			LVITEMW lvi = {LVIF_PARAM|LVIF_STATE,sel,0,0,~0};				
			ListView_GetItem(lv,(LPARAM)&lvi);
			TVITEMEXW tvi = {TVIF_PARAM|TVIF_STATE,(HTREEITEM)lvi.lParam,0,~0};
			if(TreeView_GetItem(tv,&tvi))		
			switch(wParam)
			{
			case 1008: insert:
			{
				SOM_EDIT_right.push_back(L"");			
				lvi.mask = LVIF_PARAM|LVIF_TEXT;
				lvi.pszText = LPSTR_TEXTCALLBACKW;
				int param = SOM_EDIT_right.back_param();
				lvi.lParam = (LPARAM)
				SOM_EDIT_insert(root,sel==-1?0:tvi.hItem,param);			
				lvi.iItem = sel==-1?100:sel; lvi.iSubItem = 0;
				sel = ListView_InsertItem(lv,&lvi);	
				lvi.iSubItem = 1;
				SendMessageW(lv,LVM_SETITEMTEXTW,sel,(LPARAM)&lvi);
				SOM_EDIT_changed();
				//break;
			}
			case 1009: //rename
			{
				SetFocus(lv); ListView_EditLabel(lv,sel);	 
				return 0;
			}		
			case 1010: case 1011: //show/hide
			{
				SOM_EDIT_changed(tv,tvi.hItem);
				if(wParam&1) SOM_EDIT_tree[tvi.lParam].conceal();
				if(~wParam&1) SOM_EDIT_tree[tvi.lParam].reveal();							
				ListView_SetItemState(lv,sel,wParam&1?LVIS_CUT:0,LVIS_CUT);				
				SOM_EDIT_refresh(tv,tvi.hItem);
				break;
			}			
			case 1027: case 1028: //raise/lower
			{
				bool raise = wParam&1;
				LPARAM swap = tvi.lParam;					
				if(!swapchain) swapchain = SOM_EDIT_param
				(tv,TreeView_GetNextItem(tv,tvi.hItem,raise?TVGN_PREVIOUS:TVGN_NEXT));
				else std::swap(swapchain,swap); //raising
				SOM_EDIT_tree.swap_params(swap,swapchain);
				if(!raise) swapchain = 0; //not chaining
				if(newselist==-1) newselist = sel+(raise?-1:+1);			
				SOM_EDIT_changed(tv,tvi.hItem);
				break;
			}			
			case 1210: case 1211: //delete/open
			{
				TreeView_SelectItem(tv,tvi.hItem);
				SendMessage(SOM_EDIT_151(),WM_COMMAND,LOWORD(wParam),0);				
				if(!TreeView_GetParent(tv,tvi.hItem)) 
				newselist = sel--; break; //deleted?
			}}
			sel = ListView_GetNextItem(lv,sel,LVNI_SELECTED);
		}
		if(newselist!=-1) //NEW: aftermath
		{				
			if(wParam==1027||wParam==1028) //raise/lower
			{
				TVSORTCB cb = {root,SOM_EDIT_sortcb,SOM_EDIT_right.remarks};
				TreeView_SortChildrenCB(tv,&cb,0);
			}
			SOM_EDIT_relist(lv,tv,root,newselist);
		}		
		RedrawWindow(GetDlgItem(hwndDlg,1010),0,0,RDW_INVALIDATE);
		RedrawWindow(GetDlgItem(hwndDlg,1011),0,0,RDW_INVALIDATE);
		break;
	}
	case WM_CLOSE: close:

		DestroyWindow(hwndDlg);
		return 1;
	}

	return 0;
}
static void SOM_EDIT_addfiles(HWND cb, const wchar_t *from, const wchar_t *ifnot)
{
	//CB_DIR with absolute paths
	wchar_t search[MAX_PATH] = L"";
	int cat = swprintf_s(search,L"%ls\\*.som",from)-5;
	if(cat<0) return;
	WIN32_FIND_DATAW found; 
	HANDLE glob = FindFirstFileW(search,&found);
	if(glob!=INVALID_HANDLE_VALUE) do		  
	{																			  
		wcscpy_s(search+cat,MAX_PATH-cat,found.cFileName);
		if(wcscmp(search,ifnot))
		SendMessageW(cb,CB_ADDSTRING,0,(LPARAM)search);		  
	}
	while(FindNextFileW(glob,&found)); FindClose(glob);
}
static bool SOM_EDIT_readerror(HWND tv=0)
{
	if(!tv) tv = SOM_EDIT_right.treeview;
	if(SOM_EDIT_tree[tv].eof) return false;
	MessageBoxA(0,"EDIT000","SOM_EDIT",MB_OK|MB_ICONWARNING);
	return true;
}
static wchar_t SOM_EDIT_undo[MAX_PATH];
static int SOM_EDIT_vars(HWND,HTREEITEM);
static int SOM_EDIT_zone(HWND,HTREEITEM);
static void SOM_EDIT_open(HWND,HTREEITEM);
static bool SOM_EDIT_label(HWND,HTREEITEM);
static void SOM_EDIT_cduser(wchar_t (&cd)[MAX_PATH], wchar_t (&user)[MAX_PATH])
{
	*cd = *user = '\0';
	Sompaste->place(0,cd,Sompaste->get(L"CD"),0); 
	Sompaste->place(0,user,Sompaste->get(L"USER"),0); 		
	DWORD cdrom = GetFileAttributesW(*user?user:cd);
	if(cdrom&FILE_ATTRIBUTE_READONLY
	||~cdrom&FILE_ATTRIBUTE_DIRECTORY) //EX::user
	{			   
		const wchar_t *game = Sompaste->get(L"GAME",0);	 
		if(!game) game = Sompaste->get(L"TITLE"); //fallback
		swprintf_s(user,L"%ls\\%ls",Sompaste->get(L"PLAYER"),game);
		Sompaste->place(0,user,user,0);
	}
}
static int SOM_EDIT_chkstkworkaround(HWND text) 
{
	enum{x_s=4096}; wchar_t x[x_s] = L"";				
	if(GetWindowTextW(text,x,x_s)>=x_s-1) assert(0);
	SOM_EDIT_tree.fill(x); return 0;
}
static VOID CALLBACK SOM_EDIT_activate(HWND tv, UINT, UINT_PTR id, DWORD)
{
	SOM_EDIT_tree = tv; //doesn't always focus on double-click?

	//todo: don't use Open button so the left tree can be activated
	KillTimer(tv,id); PostMessage(GetParent(tv),WM_COMMAND,1211,0); //open
}
static LRESULT CALLBACK SOM_EDIT_dblclkproc(HWND,UINT,WPARAM,LPARAM,UINT_PTR,DWORD_PTR);
extern INT_PTR CALLBACK SOM_EDIT_151(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{	
	switch(uMsg) 
	{
	case WM_INITDIALOG:
	{	
		//EX::cd/user are fixed
		wchar_t cd[MAX_PATH], user[MAX_PATH];
		SOM_EDIT_cduser(cd,user);

		//save SOM file going in
		wcscpy_s(SOM_EDIT_undo,Sompaste->get(L".SOM"));

		HWND text = //fill dictionaries
		GetWindow(GetDlgItem(hwndDlg,1022),GW_HWNDNEXT);
		static int one_off = SOM_EDIT_chkstkworkaround(text);		
		DestroyWindow(text); //prevent auto translation

		//treeviews
		HWND left = GetDlgItem(hwndDlg,1022);
		HWND right = GetDlgItem(hwndDlg,1023);
		SOM_EDIT_tree = //set focus nonzero
		SOM_EDIT_tree.left.treeview = left;
		SOM_EDIT_tree.right.treeview = right;		
		SetWindowSubclass(left,SOM_EDIT_dblclkproc,0,0);
		SetWindowSubclass(right,SOM_EDIT_dblclkproc,0,0);

		//left combobox
		HWND cb1 = GetDlgItem(hwndDlg,1011); 
		SendMessageW(cb1,CB_ADDSTRING,0,(LPARAM)L"\\\\.\\?.som");				
		wchar_t recent[MAX_PATH] = L"";
		ITEMIDLIST idl[1000]; char mru[32] = "0"; 
		for(DWORD i=0,idl_s;!SHGetValueA(HKEY_CURRENT_USER,
		"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ComDlg32\\OpenSavePidlMRU\\som",
		itoa(i,mru,10),0,idl,&(idl_s=sizeof(idl)));i++)
		{
			if(SHGetPathFromIDListW(idl,recent)) SendMessageW(cb1,CB_INSERTSTRING,1,(LPARAM)recent);
		}						 
		DWORD xp = sizeof(mru); wchar_t w[2] = L"a";
		if(!*recent&&!SHGetValueA(HKEY_CURRENT_USER,
		"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ComDlg32\\OpenSaveMRU\\som",
		"MRUList",0,mru,&xp))
		for(DWORD i=0,s;i<xp&&!SHGetValueW(HKEY_CURRENT_USER,
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ComDlg32\\OpenSaveMRU\\som",
		&(*w=mru[i]),0,recent,&(s=sizeof(recent)));i++)
		{
			SendMessageW(cb1,CB_ADDSTRING,0,(LPARAM)recent);
		}
		/*XP: keeping around until above way can be tested on XP
		else if(SHGetSpecialFolderPathW(0,recent,CSIDL_RECENT,0))
		{
			WIN32_FIND_DATAW found; 
			size_t cat = wcslen(recent)+1; 
			wcscpy(recent+cat-1,L"\\*.som.lnk");
			HANDLE glob = FindFirstFileW(recent,&found);
			if(glob!=INVALID_HANDLE_VALUE)
			{
				IShellLinkW *i = 0; IPersistFile *ii = 0; 
				CoCreateInstance(CLSID_ShellLink,0,CLSCTX_INPROC_SERVER,IID_IShellLinkW,(LPVOID*)&i);
				i->QueryInterface(IID_IPersistFile,(void**)&ii); 
				if(ii) do		  
				{																			  
					wcscpy_s(recent+cat,MAX_PATH-cat,found.cFileName);

					if(!ii->Load(recent,STGM_READ)
			//2021	 &&!i->Resolve(0,SLR_NO_UI|SLR_NOLINKINFO|SLR_NOUPDATE|SLR_NOSEARCH|SLR_NOTRACK)
					 &&!i->GetPath(target,MAX_PATH,0,0))
					{
						if(wcscmp(target,SOM_EDIT_undo))
						SendMessageW(cb1,CB_ADDSTRING,0,(LPARAM)target);		  
					}

				}while(FindNextFileW(glob,&found));
				if(ii) ii->Release();
				if(i) i->Release();
				FindClose(glob);
			}			
		}*/
		ComboBox_SetCurSel(cb1,0);
		
		//right combobox
		HWND cb2 = GetDlgItem(hwndDlg,1013); 
		SendMessageW(cb2,CB_ADDSTRING,0,(LPARAM)SOM_EDIT_undo);
		wchar_t *f = SOM_EDIT_undo, *fn = PathFindFileNameW(f);						
		if(*fn&&fn>f&&wcsncmp(cd,f,fn-1-f)&&wcsncmp(user,f,fn-1-f))
		{
			wmemcpy(recent,f,fn-1-f)[fn-1-f] = '\0';
			SOM_EDIT_addfiles(cb2,recent,SOM_EDIT_undo);
		}
		SOM_EDIT_addfiles(cb2,cd,SOM_EDIT_undo);
		if(wcscmp(cd,user)) SOM_EDIT_addfiles(cb2,user,SOM_EDIT_undo);
		ComboBox_SetCurSel(cb2,0);		
		
		//populate treeviews
		SendMessageW(hwndDlg,WM_COMMAND,MAKEWPARAM(1011,CBN_SELENDOK),(LPARAM)cb1);		
		SendMessageW(hwndDlg,WM_COMMAND,MAKEWPARAM(1013,CBN_SELENDOK),(LPARAM)cb2);		
		return 1;
	}
	case WM_DRAWITEM:
	
		return som_tool_drawitem((DRAWITEMSTRUCT*)lParam,141);
	
	case WM_COMMAND:

		switch(wParam)
		{		
		case MAKEWPARAM(1011,CBN_EDITCHANGE): //left combobox
		case MAKEWPARAM(1013,CBN_EDITCHANGE): //right combobox
		{	
			bool right = 1013==LOWORD(wParam);
			HWND tv = GetDlgItem(hwndDlg,1022+right); 
			if(right&&IsWindowEnabled(GetDlgItem(hwndDlg,12321)))
			{
				HWND cb = (HWND)lParam;	DWORD sel = ComboBox_GetEditSel(cb);
				int id = MessageBoxA(0,"EDIT1000","SOM_EDIT",MB_YESNOCANCEL|MB_ICONWARNING);
				if(id!=IDNO) //restore the file name
				{					
					ComboBox_SetCurSel(cb,ComboBox_GetCurSel(cb));
					if(id==IDCANCEL||!SOM_EDIT_savefile()
					&&IDCANCEL==MessageBoxA(0,"EDIT201","SOM_EDIT",MB_OKCANCEL|MB_DEFBUTTON2|MB_ICONWARNING))
					return 1; 		
				}
				else ComboBox_SetEditSel(cb,LOWORD(sel),HIWORD(sel));
			}
			const int dis[] = {1210,1211,12321};
			if(right) windowsex_enable(hwndDlg,dis,0);
			SOM_EDIT_collapsereset(tv);
			return 1;								 
		}			
		case MAKEWPARAM(1011,CBN_SELENDOK): //left tree
		case MAKEWPARAM(1013,CBN_SELENDOK): //right tree
		{
			bool right = 1013==LOWORD(wParam);

			HWND tv = GetDlgItem(hwndDlg,1022+right); 

			SOM_EDIT_tree::pool &pool = SOM_EDIT_tree[tv];
			
			//note: save on textual input is caught by EDITCHANGE
			if(right&&IsWindowEnabled(GetDlgItem(hwndDlg,12321)))
			switch(MessageBoxA(0,"EDIT1000","SOM_EDIT",MB_YESNOCANCEL|MB_ICONWARNING))
			{
			case IDYES:	if(SOM_EDIT_savefile()) break;

				if(IDOK==MessageBoxA(0,"EDIT201","SOM_EDIT",MB_OKCANCEL|MB_DEFBUTTON2|MB_ICONWARNING))
				break;

			case IDCANCEL: HWND cb = (HWND)lParam; 
				
				wchar_t text[MAX_PATH] = L""; GetWindowTextW(cb,text,MAX_PATH);
				ComboBox_SetCurSel(cb,SendMessageW(cb,CB_FINDSTRINGEXACT,-1,(LPARAM)text));
				return 1; 		
			}

			SetWindowRedraw(tv,0); //POINT OF NO RETURN

			SOM_EDIT_collapsereset(tv); 			

			HTREEITEM File = TreeView_GetChild(tv,TVI_ROOT);

			TVINSERTSTRUCTW tis = {TVI_ROOT,TVI_LAST,{TVIF_TEXT,0,0,~0}};

			if(!File) //first time
			{
				//TVIS_USERMASK
				tis.item.mask|=TVIF_STATE; //!
				tis.item.state = 0;
				tis.item.pszText = SOM_EDIT_tree[L"File"];
				File = TreeView_InsertItem(tv,&tis);
				tis.item.state = 0x1000;
				tis.item.pszText = SOM_EDIT_tree[L"Vars"];
				tis.itemex.mask|=TVIF_CHILDREN;
				tis.itemex.cChildren = 1;
				TreeView_InsertItem(tv,&tis);
				tis.item.state = 0x2000;
				tis.item.pszText = SOM_EDIT_tree[L"Zone"];
				TreeView_InsertItem(tv,&tis);
				tis.itemex.mask&=~TVIF_CHILDREN;
				//add some extra room for F2 editing
				tis.item.state = 0;
				tis.item.pszText = L"";				
				tis.itemex.mask|=TVIF_INTEGRAL;
				tis.itemex.iIntegral = 28;
				TreeView_InsertItem(tv,&tis);				
				tis.itemex.iIntegral = 1;
				tis.item.mask&=~TVIF_STATE; //!
			}

			HWND cb = (HWND)lParam; 
			int sel = ComboBox_GetCurSel(cb);

			if(!right&&!sel) //defaults?
			{
				static struct var
				{
					wchar_t *key; var(wchar_t *k):key(k){}

				}vars[] = 
				{
				L"-GAME=%TITLE%",
				L"-DISC=0",
				L"-CD=%CD%",
				L"-DATA=%DATA%",
				L"-DATASET",
				L"-USER",
				L"-SCRIPT",
				L"-FONT=%FONT%",
				L"-LANG=%LANG%",
				L"-INI=%TITLE%.ini",
				L"-TRIAL=00",
				L"-SETUP=1",
				L"-EX=%EX%",
				L"-ICON",
				L"-TEXT=%TEXT%",
				L"-TEMP=%TEMP%",
				L"-PLAYER=%PLAYER%",
				L"-TITLE=%TITLE%",
				L"-SOM=%SOM%",
				};
				tis.item.mask|=TVIF_PARAM; 
				for(size_t i=0;i<EX_ARRAYSIZEOF(vars);i++)
				{
					tis.hParent = File;					
					SOM_EDIT_tree.left.push_back(vars[i].key+1);
					tis.item.lParam = SOM_EDIT_tree.left.back_param();
					tis.item.pszText = (wchar_t*)SOM_EDIT_tree[vars[i].key];
					tis.hParent = TreeView_InsertItem(tv,&tis);					
					tis.item.pszText = 0;
					TreeView_InsertItem(tv,&tis);
				}  
				pool.eof = true;
			}
			else //opening a file
			{			
				wchar_t somfile[MAX_PATH] = L""; 				 
				if(sel==-1) GetWindowTextW(cb,somfile,MAX_PATH);
				if(sel!=-1) //the edit control is not yet up-to-date
				if(MAX_PATH>=SendMessageW(cb,CB_GETLBTEXTLEN,sel,0))
				SendMessageW(cb,CB_GETLBTEXT,sel,(LPARAM)somfile);				
				SWORDOFMOONLIGHT::ini::readfile(somfile,0x3,SOM_EDIT_inicb,&SOM_EDIT_tree[tv]);
				if(pool.eof)
				{
					if(sel==-1) 
					sel = SendMessageW(cb,CB_FINDSTRINGEXACT,-1,(LPARAM)somfile);
					if(sel==-1)	//todo: add to OpenSavePidlMRU ITEMIDLIST?
					sel = SendMessageW(cb,CB_INSERTSTRING,1,(LPARAM)somfile);
					ComboBox_SetCurSel(cb,sel);
				}
				else 
				{
					SOM_EDIT_collapsereset(tv);
					MessageBoxA(0,"EDIT000","SOM_EDIT",MB_OK|MB_ICONWARNING);					
				}
				if(right) SOM_EDIT_changed(-100000);
			}			
			if(pool.eof) TreeView_Expand(tv,File,TVE_EXPAND);

			SetWindowRedraw(tv,1);
			return 0;
		}}

		switch(LOWORD(wParam))
		{			
		case 1018: //->
		{
			//-> button is never greyed out

			HWND l = GetDlgItem(hwndDlg,1022); 
			HWND r = GetDlgItem(hwndDlg,1023); 

			HTREEITEM src = TreeView_GetSelection(l);			
			if(!TreeView_GetChild(l,src)) src = TreeView_GetParent(l,src);
			HTREEITEM file = SOM_EDIT_tree.left.file();
			HTREEITEM src_file = TreeView_GetParent(l,src);

			int param = SOM_EDIT_param(l,src);

			if(!param&&src!=file
			  ||src!=file&&src_file!=file
			  ||param==SOM_EDIT_tree.left.remarks&&param)
			{
				MessageBeep(-1); break;
			}

			if(!src_file) //deep copy
			{
				MessageBeep(-1); //unimplemented...

				//instead for now the left selection is moved up below
				//so the user can easily just keep pressing the <- button
			}
			else //TreeView_SelectItem: see deep copy notes above
			{					
				TreeView_SelectItem(l,TreeView_GetPrevSibling(l,src));

				HTREEITEM dst = TreeView_GetSelection(r);
				if(!TreeView_GetChild(r,dst)) dst = TreeView_GetParent(r,dst); 				
				HTREEITEM root = TreeView_GetParent(r,dst); 
				if(!root) root = dst; 
				
				file = SOM_EDIT_right.file();
				if(dst!=file&&root!=file) dst = root = file;

				HTREEITEM ins = root!=dst?dst:TreeView_GetChild(r,root);
				SOM_EDIT_right.push_back(SOM_EDIT_tree[param]);
				ins = SOM_EDIT_insert(root,ins,SOM_EDIT_right.back_param());
				if(TVIS_CUT&TreeView_GetItemState(l,src,~0))
				TreeView_SetItemState(r,ins,~0,TVIS_CUT);
				TreeView_SetItemState(r,ins,~0,TVIS_BOLD);
				SOM_EDIT_changed(+1);
			}			
			break;
		}
		case 1019: //<-
		{
			//<- button is never greyed out

			HWND r = GetDlgItem(hwndDlg,1023); 
			HTREEITEM ins = TreeView_GetSelection(r);			
			if(TVIS_BOLD&TreeView_GetItemState(r,ins,~0))
			if(SOM_EDIT_right.file()==TreeView_GetParent(r,ins))
			{
				SOM_EDIT_changed(-1); TreeView_DeleteItem(r,ins);
			}
			else MessageBeep(-1); break;
		}
		case 1010: //load
						
			if(IsWindowEnabled(GetDlgItem(hwndDlg,12321))) 
			switch(MessageBoxA(0,"EDIT1000","SOM_EDIT",MB_YESNOCANCEL|MB_ICONWARNING))
			{
			case IDYES:	if(SOM_EDIT_savefile()) break;

				if(IDOK==MessageBoxA(0,"EDIT201","SOM_EDIT",MB_OKCANCEL|MB_DEFBUTTON2|MB_ICONWARNING))
				break;

			case IDCANCEL: return 1; 		
			}
			//break;

		case 1009: //add
		{				
			static wchar_t som1[MAX_PATH] = L"";	
			static wchar_t som2[MAX_PATH] = L"";	
			
			wchar_t *som = wParam&1?som1:som2;
			if(!*som) wcscpy(som,SOM_EDIT_undo);			
			extern UINT_PTR CALLBACK som_tool_openhook(HWND,UINT,WPARAM,LPARAM);
			OPENFILENAMEW open = 
			{
				sizeof(open),hwndDlg,0,
				L"Sword of Moonlight (*.som)\0*.som\0",	
				0, 0, 0, som, MAX_PATH, 0, 0, 0, 0, 
				//this hook is just to change the look
				OFN_ENABLEHOOK|OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY,
				0, 0, 0, 0, som_tool_openhook, 0, 0, 0
			};
			if(wParam&1) open.Flags|=OFN_FILEMUSTEXIST;
			if(!GetOpenFileNameW(&open)) break;
			
			int id = wParam&1?1011:1013;
			HWND cb = GetDlgItem(hwndDlg,id);
			int sel = SendMessageW(cb,CB_FINDSTRINGEXACT,-1,(LPARAM)som);
			if(sel==CB_ERR) 
			sel = SendMessageW(cb,CB_INSERTSTRING,1,(LPARAM)som);	
			ComboBox_SetCurSel(cb,sel);
			//this line avoids double Keep changes? questioning 
			//and sidesteps the edit box's counterintuitive ways
			if(~wParam&1) windowsex_enable<12321>(hwndDlg,0); 
			SendMessage(hwndDlg,WM_COMMAND,MAKEWPARAM(id,CBN_SELENDOK),(LPARAM)cb);			
			break;
		}
		case 1210: //delete
		{
			SOM_EDIT_readerror();
			HWND bt = (HWND)lParam;
			if(!bt) bt = GetDlgItem(hwndDlg,1210);
			if(!IsWindowEnabled(bt)) break;
			HWND tv = SOM_EDIT_right.treeview;
			HTREEITEM ins = TreeView_GetSelection(tv);
			int bold = TVIS_BOLD&TreeView_GetItemState(tv,ins,~0);
			if(TreeView_DeleteItem(tv,ins))
			SOM_EDIT_changed(bold?-1:+1); 
			break;
		}
		case 1211: //open
		{
			SOM_EDIT_readerror();
			HWND bt = (HWND)lParam;
			if(!bt) bt = GetDlgItem(hwndDlg,1211);
			if(!IsWindowEnabled(bt)) break;

			LPARAM param = 0;
			HRSRC found = 0; DLGPROC dp = 0;
			HWND tv = SOM_EDIT_right.treeview;
			HTREEITEM item = TreeView_GetSelection(tv);
			switch(SOM_EDIT_level(tv,item))
			{
			case 0: dp = SOM_EDIT_152; 

				if(SOM_EDIT_right.file()==item)
				found = FindResource(0,MAKEINTRESOURCE(152),RT_DIALOG);
				param = (LPARAM)item;
				break;

			case 1: dp = SOM_EDIT_153;

				if(SOM_EDIT_right.file()==TreeView_GetParent(tv,item))
				found = FindResource(0,MAKEINTRESOURCE(153),RT_DIALOG);
				param = SOM_EDIT_param(tv,item);
				break;
			}				 
			if(!found) break;
			DLGTEMPLATE *locked = 
			(DLGTEMPLATE*)LockResource(LoadResource(0,found)); 
			if(!locked) break;	
			//A: call through installed hook
			CreateDialogIndirectParamA(GetModuleHandle(0),locked,hwndDlg,dp,param);
			break;
		}
		case 12321: //save
		{
			if(!SOM_EDIT_savefile())
			MessageBoxA(0,"EDIT201","SOM_EDIT",MB_OK|MB_ICONWARNING);
			break;
		}	
		case IDCLOSE: case IDCANCEL: 
			
			//if not handled in WM_CLOSE canceling will loop back???
			goto close; 		

		case IDOK: 

			if(!SOM_EDIT_savefile())
			switch(MessageBoxA(0,"EDIT201","SOM_EDIT",MB_OKCANCEL|MB_ICONWARNING))
			{
			case IDCANCEL: return 1;
			}
			Sompaste->set(L".MO",L""); //HACK? invalidate transcript
			//2018
			//HACK! EX::cd() and crew back most of the SOM file's env-variables
			//(I got the idea to do this from the SOM_EX launcher not resetting
			//a host of variables before launching a new "job.")
			extern void Ex_2018_reset_SOM_EX_or_SOM_EDIT_tool();
			Ex_2018_reset_SOM_EX_or_SOM_EDIT_tool();

			return DestroyWindow(hwndDlg); 
		}
		break;

	case WM_NOTIFY:
	{
		int out = 0;
				
		NMTVDISPINFOW *p = (NMTVDISPINFOW*)lParam;		

		HWND &tv = p->hdr.hwndFrom;
				  
		switch(p->hdr.code)
		{
		case TVN_GETDISPINFOW:
		{
			if(p->item.mask&TVIF_TEXT)
			{
				p->item.pszText = 
				SOM_EDIT_tree[p->item.lParam].print();
			}
			break;			
		}
		case TVN_GETINFOTIPW: 
		{
			NMTVGETINFOTIPW *p = (NMTVGETINFOTIPW*)lParam;

			if(1==SOM_EDIT_level(tv,p->hItem))
			{
				p->pszText = SOM_EDIT_tree[p->lParam].print();
			}
			break;
		}
		case NM_CUSTOMDRAW:
		{
			NMCUSTOMDRAW *p = (NMCUSTOMDRAW*)lParam;

			switch(p->dwDrawStage) //assuming treeview
			{
			case CDDS_PREPAINT: //want per item behavior

				out = CDRF_NOTIFYITEMDRAW; break;

			case CDDS_ITEMPREPAINT: //only level 2 items

				//subitem must be for ListView reports
				//out = CDRF_NOTIFYSUBITEMDRAW; break;
									
			//case CDDS_SUBITEM|CDDS_ITEMPREPAINT: //level 2? 

				NMTVCUSTOMDRAW *q = (NMTVCUSTOMDRAW*)lParam;
				
				HTREEITEM item = (HTREEITEM)p->dwItemSpec;
				TVITEMEXW ti = {TVIF_PARAM|TVIF_STATE|TVIF_TEXT,item,0,~0};	
				//5: CRLF/surrogate pairs
				wchar_t test[5] = L""; ti.pszText = test; ti.cchTextMax = 5;
				if(!TreeView_GetItem(tv,&ti)) assert(0);

				if(CDIS_SELECTED&~p->uItemState)
				if(TVIS_CUT&ti.state) //if(TVIS_CUT&p->uItemState)
				{	
					q->clrText = GetSysColor(COLOR_GRAYTEXT);					
					SetTextColor(p->hdc,q->clrText); 
				}

				//custom drawing...
				//if(q->iLevel!=2) break;
				//if(q->iLevel!=2&&ti.iIntegral<2) break;
				if(*test) break; 
				
				wchar_t *text = SOM_EDIT_tree[ti.lParam].print();

				if((ti.state&TVIS_USERMASK)==0x1000)
				{
					//vars: indent/merge with parent
					while(*text&&*text!='\r') text++; 
					TreeView_GetItemRect(tv,TreeView_GetParent(tv,item),&p->rc,1);
				}
				else TreeView_GetItemRect(tv,item,&p->rc,1);				
				RECT cr, rc = p->rc; GetClientRect(tv,&cr); 					
				DrawTextW(p->hdc,text,-1,&rc,DT_CALCRECT|DT_NOPREFIX|DT_NOCLIP);					
				//NEW: prefer integral over line height for now					
				TreeView_GetItemRect(tv,item,&p->rc,0);
				/*rc.bottom+=2;*/ rc.bottom = p->rc.bottom; //!
				int hack = rc.top; //hack: in case vars is split in two
				rc.right+=4; IntersectRect(&rc,&cr,&rc); rc.top = hack;
				int i; static POLYTEXTW poly[50];  
				int lh = TreeView_GetItemHeight(tv);
				for(i=0;i<EX_ARRAYSIZEOF(poly)&&text;i++)
				{
					poly[i].x = rc.left+2; poly[i].y = rc.top+1+lh*i;
					text = (wchar_t*)wcschr(poly[i].lpstr=text,'\n'); 
					poly[i].n = text?text++-poly[i].lpstr:wcslen(poly[i].lpstr); 
					poly[i].uiFlags = ETO_CLIPPED|ETO_OPAQUE;
					poly[i].rcl = rc; poly[i].rcl.top+=lh*i;
					poly[i].rcl.bottom = poly[i].rcl.top+lh;
					//the scrollbar setup won't clip the top
					if(poly[i].rcl.top>=rc.bottom) break;
				}
				//skip first line of environment var
				if((ti.state&TVIS_USERMASK)==0x1000)
				{
					PolyTextOutW(p->hdc,poly+1,i-1);
				}
				else PolyTextOutW(p->hdc,poly,i);
													   
				//ATTN: unfortunately this is the only option
				//it's all or nothing. Blanking the text still
				//produces an annoying empty placeholder thingy
				if(TVS_HASLINES&~GetWindowStyle(tv))
				out = CDRF_SKIPDEFAULT; 										
				if(!out) //Actually, this seems to do the trick
				ExcludeClipRect(p->hdc,rc.left,rc.top,rc.right,rc.bottom);					
				break;
			}
			break;
		}		
		case TVN_KEYDOWN:
		{
			HTREEITEM sel = TreeView_GetSelection(tv);

			switch(((NMTVKEYDOWN*)lParam)->wVKey)   
			{			
			case VK_DELETE:

				SendMessage(hwndDlg,WM_COMMAND,1210,0);
				break;

			case VK_SPACE: out = 1; //incremental search?
										
				switch(TVIS_USERMASK&TreeView_GetItemState(tv,sel,~0))
				{
				case 0x2000: SOM_EDIT_open(tv,sel); break;

				default: if(SOM_EDIT_readerror(tv)) break;

					if(IsWindowEnabled(GetDlgItem(hwndDlg,1211)))
					{
						SendMessage(hwndDlg,WM_COMMAND,1211,0);
					}
					else goto f2; //break;
				}
				break;
			
			case VK_F2: f2:	
			
				if(!SOM_EDIT_label(tv,sel))	MessageBeep(-1);
				break;
			}		
			break;
		}		
		case NM_CLICK: //simulate label edit
		{
			TVHITTESTINFO hti; 
			GetCursorPos(&hti.pt);
			MapWindowPoints(0,tv,&hti.pt,1);
			if(TreeView_HitTest(tv,&hti)
			  &&hti.flags&(TVHT_ONITEMRIGHT|TVHT_ONITEMLABEL)
			  &&hti.hItem==TreeView_GetSelection(tv))
			{
				 SOM_EDIT_label(tv,hti.hItem);
			}
			break;
		}
		case NM_DBLCLK: 
		{						
			HTREEITEM sel = TreeView_GetSelection(tv);
			switch(TVIS_USERMASK&TreeView_GetItemState(tv,sel,~0))
			{
			case 0x2000: SOM_EDIT_open(tv,sel); break;

			default: SetTimer(tv,'open',0,SOM_EDIT_activate); 
			}			
			out = 1; break; //1: prevent expansion
		}		
		case NM_SETFOCUS:
		{
			if(p->hdr.idFrom==1022) //left treeview
			{
				//disable delete/open buttons
				windowsex_enable(hwndDlg,1210,1211,0);
				SOM_EDIT_tree = p->hdr.hwndFrom; 
			}
			else 
			{
				SOM_EDIT_tree = p->hdr.hwndFrom; 
				//MSDN docs say this will work but it doesn't
				//else TreeView_SelectItem(tv,TreeView_GetSelection(tv));
				goto selchanged; 			
			}
			break;
		}
		case TVN_SELCHANGINGW:
		{
			//prevent selection of the scratch pad
			NMTREEVIEWW *p = (NMTREEVIEWW*)lParam;
			if(0==SOM_EDIT_level(tv,p->itemNew.hItem))			
			out = !TreeView_GetNextSibling(tv,p->itemNew.hItem);
			break;			
		}
		case TVN_SELCHANGEDW: selchanged: 
		{
			//enable delete/open buttons			
			if(tv==SOM_EDIT_right.treeview&&SOM_EDIT_right.eof)
			{
				//NMTREEVIEWW *p = (NMTREEVIEWW*)lParam;
				HTREEITEM sel = TreeView_GetSelection(tv);
				if(p->hdr.code==TVN_SELCHANGED)
				assert(sel==((NMTREEVIEWW*)p)->itemNew.hItem);
				int level = SOM_EDIT_level(tv,sel);	
				HWND del = GetDlgItem(hwndDlg,1210), open = GetDlgItem(hwndDlg,1211);
				//~0: the mask doesn't work as advertised
				if(!(TVIS_USERMASK&TreeView_GetItemState(tv,sel,~0)))
				{
					EnableWindow(del,level==1); 
					EnableWindow(open,level<2);
					if(SOM_EDIT_param(tv,sel)==SOM_EDIT_tree[tv].remarks)
					EnableWindow(del,0);
				}
				else //todo: support opening of top-level items
				{
					EnableWindow(del,0); EnableWindow(open,0);
				}
			}
			break;
		}
		case TVN_ITEMEXPANDINGW: 
		{
			NMTREEVIEW  *p = (NMTREEVIEW*)lParam;

			if(p->action!=TVE_EXPAND) break; 
			if(SOM_EDIT_readerror(tv)) break;			

			switch(TVIS_USERMASK&p->itemNew.state)
			{
			case 0x1000: return SOM_EDIT_vars(tv,p->itemNew.hItem);
			case 0x2000: return SOM_EDIT_zone(tv,p->itemNew.hItem);
			}
			break; 
		}}
		if(!out) break;
		SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,out);
		return 1;
	}
	case WM_CLOSE: close: 

		if(IsWindowEnabled(GetDlgItem(hwndDlg,12321))) 
		switch(MessageBoxA(0,"EDIT1000","SOM_EDIT",MB_YESNOCANCEL|MB_ICONWARNING))
		{
		case IDYES:	if(SOM_EDIT_savefile()) break;

			if(IDOK==MessageBoxA(0,"EDIT201","SOM_EDIT",MB_OKCANCEL|MB_DEFBUTTON2|MB_ICONWARNING))
			break;

		case IDCANCEL: return 1; 		
		}

		//restore original file going in
		if(wcscmp(SOM_EDIT_undo,Sompaste->get(L".SOM")))
		{
			Som_h(SOM_EDIT_undo,Sompaste->get(L".NET"));
			Sompaste->set(L".SOM",SOM_EDIT_undo);
		}
		return DestroyWindow(hwndDlg); 
	}

	return 0;
}		
static LRESULT CALLBACK SOM_EDIT_dblclkproc
(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scID, DWORD_PTR)
{
	switch(uMsg)
	{
	case WM_LBUTTONDBLCLK: //cascade 
	{	
		//done here since NM_DBLCLK ignores "whitespace"
		TVHITTESTINFO hti; POINTSTOPOINT(hti.pt,lParam);			
		if(TreeView_HitTest(hWnd,&hti)
		  &&hti.flags&(TVHT_ONITEMBUTTON|TVHT_ONITEMINDENT))
		{				
			//implements pain in the ass notification logic
			extern void som_tool_toggle(HWND,HTREEITEM,POINT,int);
			som_tool_toggle(hWnd,hti.hItem,hti.pt,TVE_EXPAND);

			int st = TreeView_GetItemState(hWnd,hti.hItem,~0);
			if(st&TVIS_USERMASK||1!=SOM_EDIT_level(hWnd,hti.hItem))
			return 0;
			SOM_EDIT_label(hWnd,TreeView_GetChild(hWnd,hti.hItem));
			return 0;
		}
		break;	
	}}

	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}

static HWND SOM_EDIT_f2 = 0;
static bool SOM_EDIT_f2check = false;
static TOOLINFOW SOM_EDIT_f2tip = //sizeof(ti)
{TTTOOLINFOW_V1_SIZE,TTF_IDISHWND|TTF_SUBCLASS,
0,0,{0,0,0,0},0,L"[v] Enter->Ctrl+Down    Tab",0,0};
static HHOOK SOM_EDIT_f2next = 0; //REMOVE ME??
static LRESULT CALLBACK SOM_EDIT_f2hook(int,WPARAM,LPARAM);
static LRESULT CALLBACK SOM_EDIT_f2proc(HWND,UINT,WPARAM,LPARAM,UINT_PTR,DWORD_PTR);

static bool SOM_EDIT_label(HWND tv, HTREEITEM item)
{
	if(SOM_EDIT_f2) return true;

	LPARAM param = SOM_EDIT_param(tv,item);	

	if(!param||2!=SOM_EDIT_level(tv,item)) return false;

	switch(TVIS_USERMASK&TreeView_GetItemState(tv,item,~0))
	{
	case 0: break; default: return false; //filter by group
	}

	RECT ir = {0,0,20,20}; TreeView_GetItemRect(tv,item,&ir,1);

	long readonly = GetDlgCtrlID(tv)==1022?ES_READONLY:0;
	long checkbox = readonly?BS_3STATE:BS_AUTOCHECKBOX;

	const int bt_size = 13, es = //EN_UPDATE requires auto v/hscroll
	ES_MULTILINE|ES_WANTRETURN|ES_AUTOHSCROLL|ES_AUTOVSCROLL|WS_BORDER;	
	//Reminder: not using Rich Edit since Windows doesn't for its labels
	//But if we ever change our minds use EM_SETEDITSTYLE+SES_EMULATESYSEDIT
	HWND em = CreateWindowExW(WS_EX_TRANSPARENT,L"EDIT",0,WS_CHILD|es|readonly,0,0,1,1,tv,0,0,0);	
	HWND bt = CreateWindowExW(WS_EX_TRANSPARENT,L"BUTTON",0,WS_CHILD|checkbox|BS_FLAT,0,0,1,1,tv,0,0,0);								
	SetWindowFont(em,GetWindowFont(tv),0);
	SetWindowPos(em,0,ir.left,ir.top-1,0,0,SWP_NOSIZE|SWP_SHOWWINDOW); 								
	SetWindowPos(bt,em,ir.left-bt_size-3,ir.top,bt_size,bt_size,SWP_SHOWWINDOW);
		
	SOM_EDIT_f2 = em; //important this is set here!
	SOM_EDIT_f2next = //how we are cancelling input
	//(scrollbars are especially hard, this is the only way that works)
	SetWindowsHookExW(WH_MOUSE,SOM_EDIT_f2hook,0,GetCurrentThreadId());		
	
	SetWindowTextW(em,SOM_EDIT_tree[param].margin());				
	Edit_SetModify(em,0); Edit_SetSel(em,0,-1);
	SetWindowSubclass(em,SOM_EDIT_f2proc,(UINT_PTR)tv,(DWORD_PTR)item);
	SetWindowSubclass(tv,SOM_EDIT_f2proc,(UINT_PTR)tv,(DWORD_PTR)item);
	//initialize edit label dimensions (depends on subclass above)
	SendMessage(tv,WM_COMMAND,MAKEWPARAM(0,EN_UPDATE),(LPARAM)em); 		
	Button_SetCheck(bt,readonly?BST_INDETERMINATE:SOM_EDIT_f2check);	
	//hack: focus mid click (Posting to WM_SETFOCUS)
	PostMessage(em,WM_SETFOCUS,0,0); //SetFocus(em);
					
	HWND tt = TreeView_GetToolTips(tv);	//checkbox tootip
	SOM_EDIT_f2tip.hwnd = tv; SOM_EDIT_f2tip.uId = (UINT_PTR)bt; 
	if(!SendMessageW(tt,TTM_ADDTOOLW,0,(LPARAM)&SOM_EDIT_f2tip)) 
	assert(0);

	return true;
}

//REMOVE ME??
//note this is mainly for scrollbars but backgrounds too and maybe more
static LRESULT CALLBACK SOM_EDIT_f2hook(int code, WPARAM wParam, LPARAM lParam)
{
	int out = 0;

	MOUSEHOOKSTRUCT *p = (MOUSEHOOKSTRUCT*)lParam;
			
	if(code==HC_ACTION)
	{	
		switch(wParam) //clicks
		{
		case WM_LBUTTONUP: case WM_RBUTTONUP: 
		case WM_NCLBUTTONUP: case WM_NCRBUTTONUP: 
		case WM_MOUSEMOVE: case WM_MOUSEHOVER: case WM_MOUSELEAVE: 
		case WM_NCMOUSEMOVE: case WM_NCMOUSEHOVER: case WM_NCMOUSELEAVE: 

			break; default: 

			int ht = p->hwnd==SOM_EDIT_f2?1:
			SendMessage(SOM_EDIT_f2,WM_NCHITTEST,0,POINTTOPOINTS(p->pt));
			if(!ht)
			{
				HWND bt = GetWindow(SOM_EDIT_f2,GW_HWNDNEXT);
				ht = p->hwnd==bt?1:
				SendMessage(bt,WM_NCHITTEST,0,POINTTOPOINTS(p->pt));
				//if(ht) p->hwnd = bt;
			}
			//else p->hwnd = SOM_EDIT_f2;		
			
			//kill focus on the edit label
			if(!ht) SetFocus(WindowFromPoint(p->pt));
			/*this is no longer needed and doesn't work
			//with the edit label KILLFOCUS->COLLAPSE->EXPAND
			//bits of f2proc, which gets the same results for free
			//(reminder: these "bits" were moved into SOM_EDIT_refresh)
			if(!ht)	//NEW: permit click on item to cancel 
			{
				HWND newfocus = WindowFromPoint(p->pt);				
				if(newfocus==GetParent(SOM_EDIT_f2)) //treeview?
				{
					TreeView_SelectItem(newfocus,0); //NM_CLICK
					if(wParam==WM_LBUTTONDOWN)
					newfocus = 0;
				}
				SetFocus(newfocus);
			}*/
		}
	}

	if(!out||code<0) //code<0: paranoia
	return CallNextHookEx(SOM_EDIT_f2next,code,wParam,lParam);
	return out;
}
static LRESULT CALLBACK SOM_EDIT_f2proc
(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scID, DWORD_PTR treeitem)
{
	HWND &tv = (HWND&)scID;
	HTREEITEM &item = (HTREEITEM&)treeitem;	 

	if(hWnd==tv) //resizing
	{
		if(uMsg==WM_COMMAND)
		if((HWND)lParam==SOM_EDIT_f2)
		if(EN_UPDATE==HIWORD(wParam))
		{	
			HWND em = (HWND)lParam;	HDC dc = GetDC(em);	
			SelectObject(dc,(HGDIOBJ)SendMessage(em,WM_GETFONT,0,0));			
			RECT rect, calc = {0,0,4096,4096}; //Non-ClientRect
			GetWindowRect(em,&rect); OffsetRect(&rect,-rect.left,-rect.top); 
			HLOCAL lock = (HLOCAL)SendMessageW(em,EM_GETHANDLE,0,0);
			wchar_t *text = (wchar_t*)LocalLock(lock); 
			//DT_EXPANDTABS is better than nothing for now
			//DT_EDITCONTROL ignores trailing blank lines triggering auto-vscroll issues
			DrawTextW(dc,*text?text:L" ",-1,&calc,DT_CALCRECT|DT_NOPREFIX|DT_NOCLIP|DT_EXPANDTABS); 
			LocalUnlock(lock);
				
			//2: WS_BORDER
			//4: there must be room for the last line to fit			
			calc.right+=4; calc.bottom+=4; assert(!calc.left&&!calc.top);
			if(calc.right>rect.right||calc.bottom>rect.bottom)
			{	
				RECT vp; GetClientRect(hWnd,&vp); MapWindowRect(hWnd,em,&vp);
				UnionRect(&rect,&calc,&rect); IntersectRect(&rect,&vp,&rect);
				SetWindowPos(em,HWND_TOP,0,0,rect.right,rect.bottom,SWP_NOMOVE|SWP_NOREDRAW|SWP_NOZORDER|SWP_NOACTIVATE);			
				RedrawWindow(em,0,0,RDW_FRAME|RDW_INVALIDATE); 	
			}
		}
	}
	else switch(uMsg) //hwnd==em
	{		
	case WM_GETDLGCODE: //VK_TAB
	{
		LRESULT out = DefSubclassProc(hWnd,uMsg,wParam,lParam);
		return out|DLGC_WANTTAB;
	}	
	case WM_KEYDOWN:

		switch(wParam)
		{	
		case VK_TAB: //Enter key mode

			SendMessage(GetWindow(hWnd,GW_HWNDNEXT),BM_CLICK,0,0);
			SetFocus(hWnd); //???
			return 1;

		case VK_DOWN: //insert line break?
						
			if(ES_READONLY&GetWindowStyle(hWnd)) break;

			if(GetKeyState(VK_CONTROL)>>15)
			{
				SendMessageW(hWnd,EM_REPLACESEL,1,(LPARAM)L"\r\n");
				return 1;
			}
			else
			{
				POINT pt, qt; GetCaretPos(&pt);

				LRESULT out = DefSubclassProc(hWnd,uMsg,wParam,lParam);

				GetCaretPos(&qt); if(qt.y>pt.y) return out; //nope

				int len = GetWindowTextLength(hWnd);			
				SendMessageW(hWnd,EM_SETSEL,len,len);
				SendMessageW(hWnd,EM_REPLACESEL,1,(LPARAM)L"\r\n");
				return 1;
			}
			break;

		case VK_ESCAPE: goto escape;

		case VK_RETURN: //falls through
			
			if(Button_GetCheck(GetWindow(hWnd,GW_HWNDNEXT)))
			if(~GetKeyState(VK_F2)>>15) //hack: som_tool_msgfilterproc
			break;

		case VK_F2: //falling through

			SetFocus(tv); //goto killfocus; //prevent double pass		
			break;
		}
		break;		

	//SetFocus: allows PostMessage to focus
	case WM_SETFOCUS: SetFocus(hWnd); break; //hack
	case WM_KILLFOCUS:
	{
		if((HWND)wParam==GetWindow(hWnd,GW_HWNDNEXT))
		{
			SendMessage(GetWindow(hWnd,GW_HWNDNEXT),BM_CLICK,0,0);
			PostMessage(hWnd,WM_SETFOCUS,0,0); //hack
			break;
		}		
		else if(~GetWindowStyle(hWnd)&ES_READONLY)
		{
			if(!Edit_GetModify(hWnd)) goto escape;
			LONG_PTR param = SOM_EDIT_param(tv,item);
			HLOCAL lock = (HLOCAL)SendMessageW(hWnd,EM_GETHANDLE,0,0);		
			SOM_EDIT_tree[param] = (wchar_t*)LocalLock(lock);			
			LocalUnlock(lock);						
			if(param!=SOM_EDIT_tree[tv].remarks) 			
			SOM_EDIT_tree[param].self_correct();
			SOM_EDIT_refresh(tv,0,item); 
			SOM_EDIT_changed(tv,0,item);
		}

	escape:	UnhookWindowsHookEx(SOM_EDIT_f2next);

		SOM_EDIT_f2check = 
		Button_GetCheck(GetWindow(hWnd,GW_HWNDNEXT));
		DestroyWindow(GetWindow(hWnd,GW_HWNDNEXT));
		if(!SendMessageW(TreeView_GetToolTips(tv),TTM_DELTOOLW,0,(LPARAM)&SOM_EDIT_f2tip))
		;//assert(0); //fails sometimes?
		//DestroyWindow(hWnd);  
		PostMessage(hWnd,WM_CLOSE,0,0);

		if(uMsg==WM_KILLFOCUS) break;

		SetFocus(tv); //treeview
		
		return 0;
	}
	case WM_CLOSE: DestroyWindow(hWnd); break;
	case WM_NCDESTROY: SOM_EDIT_f2 = 0; 

		RemoveWindowSubclass(hWnd,SOM_EDIT_f2proc,scID);		
		RemoveWindowSubclass(tv,SOM_EDIT_f2proc,scID); 
		break;
	}	

	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}

static int SOM_EDIT_vars(HWND tv, HTREEITEM item)
{
	SOM_EDIT_tree::pool &pool = SOM_EDIT_tree[tv]; 
	
	HTREEITEM vars = pool.vars(); 	
	if(item!=vars||TreeView_GetChild(tv,vars)) return 0;

	if(!SOM_EDIT_tempfile(tv)) return 1; //beep?

	TVINSERTSTRUCTW tvis = 
	{vars,TVI_LAST,{TVIF_PARAM|TVIF_TEXT|TVIF_STATE,0,0,~0}};				

	wchar_t *env = GetEnvironmentStringsW();

	for(wchar_t *p=env,*sep;*p;p++)
	{
		sep = wcschr(p,'='); if(sep) *sep = '\0';
		//SOM: includes Som.h and som.exe.cpp and SOM_EX.cpp
		if(sep&&*p&&*p!='.'&&(*p!='S'||wcsncmp(p,L"SOM",3)))
		{
			switch(SOM_EDIT_tree.dict_if_1_userdict_if_2(p))
			{			
			case 1: tvis.item.state = 0; break;
			case 2: tvis.item.state = TVIS_BOLD; break;
			default: tvis.item.state = TVIS_CUT; break;			
			}
			tvis.item.state|=0x1000; //TVIS_USERMASK

			//pool.vars_strings.push_back(p); 
			//tvis.item.lParam = pool.vars_strings_back_param();
			//SOM_EDIT_tree::string &var = pool.vars_strings.back();
			wchar_t *top = p;

			for(p=sep+1;*p&&*p!='\n';p++); 

			if(*p!='\n') //single line
			{
				/*if(sep[1])
				{
					var+=L"="; var+=sep+1;
				}*/
				tvis.item.lParam = 0; 
				tvis.item.pszText = top; //LPSTR_TEXTCALLBACKW; 
				if(sep[1]) *sep = '=';
				TreeView_InsertItem(tv,&tvis);
				*sep = '\0';				
			}
			else //multi-line: add [+] button
			{
				pool.vars_strings.push_back(top); 
				tvis.item.lParam = pool.vars_strings_back_param();
				SOM_EDIT_tree::string &var = pool.vars_strings.back();

				var+=L"=";
				tvis.item.pszText = var.print();
				tvis.hParent = TreeView_InsertItem(tv,&tvis);
				var+=L"\r\n"; var+=sep+1; 				
				tvis.item.pszText = L""; //0;
				tvis.itemex.mask|=TVIF_INTEGRAL;
				//-1: the first line is not displayed
				tvis.itemex.iIntegral = var.printegral()-1;
				TreeView_InsertItem(tv,&tvis);
				tvis.itemex.iIntegral = 1;
				tvis.hParent = vars;
			}
		}
		if(sep) *sep = '='; while(*p) p++;
	}
	FreeEnvironmentStringsW(env); return 0;
}
static bool SOM_EDIT_deep(const wchar_t *dir)
{
	wchar_t spec[MAX_PATH] = L"";
	swprintf_s(spec,L"%ls\\*",dir);
	WIN32_FIND_DATAW found;
	HANDLE glob = FindFirstFileW(spec,&found);
	if(glob!=INVALID_HANDLE_VALUE) do
	{												
		if(*found.cFileName=='.') continue; //./..				
		if(found.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
		{
			FindClose(glob); return true;
		}
	}
	while(FindNextFileW(glob,&found)); 
	FindClose(glob); return false;
}
static int SOM_EDIT_zone2(HWND,HTREEITEM);
static int SOM_EDIT_zone(HWND tv, HTREEITEM item)
{
	SOM_EDIT_tree::pool &pool = SOM_EDIT_tree[tv];

	HTREEITEM zone = pool.zone();
	if(item!=zone) return SOM_EDIT_zone2(tv,item);

	if(TreeView_GetChild(tv,zone)) return 0;
	
	if(!SOM_EDIT_tempfile(tv)) return 1; //beep?
	
	TVINSERTSTRUCTW tvis = 
	{zone,TVI_LAST,{TVIF_PARAM|TVIF_TEXT|TVIF_STATE,0,0,~0}};			
	
	tvis.item.lParam = 0;
	tvis.item.state = 0x2000; //TVIS_USERMASK

	wchar_t cd[MAX_PATH], user[MAX_PATH];
	SOM_EDIT_cduser(cd,user);

	TVINSERTSTRUCTW tvis2 = tvis; 
	tvis.item.pszText = SOM_EDIT_tree[L"-CD"];
	pool.vars_strings.push_back(cd); //balloon tip
	tvis.item.lParam = pool.vars_strings_back_param();
	tvis2.item.pszText = cd; 
	tvis2.hParent = TreeView_InsertItem(tv,&tvis);
	TreeView_InsertItem(tv,&tvis2);		

	bool use = *user&&wcscmp(user,cd);

	if(use) //bold USER files/folders
	{		
		tvis.item.state|=TVIS_BOLD;
		tvis2.item.state|=TVIS_BOLD;
		tvis.item.pszText = SOM_EDIT_tree[L"-USER"];
		pool.vars_strings.push_back(user); //balloon tip
		tvis.item.lParam = pool.vars_strings_back_param();
		tvis2.hParent = TreeView_InsertItem(tv,&tvis);
		tvis2.item.pszText = user; 		
		TreeView_InsertItem(tv,&tvis2);
		tvis2.item.state&=~TVIS_BOLD;
		tvis.item.state&=~TVIS_BOLD;
	}
	else wcscpy(user,cd);	

	union{struct //MSVC extension
	{
		wchar_t rn[2], place[MAX_PATH];	  
	};};
	wchar_t *rnplace = wcscpy(place-2,L"\r\n");
	tvis.item.pszText = SOM_EDIT_tree[L"-DATA"];	
	swprintf_s(place,L"%ls\\DATA",user);
	pool.vars_strings.push_back(place); //balloon tip
	tvis.item.lParam = pool.vars_strings_back_param();
	tvis2.item.pszText = place; 
	tvis2.hParent = TreeView_InsertItem(tv,&tvis);
	HTREEITEM bold = TreeView_InsertItem(tv,&tvis2);
	{
		if(use)
		{
			swprintf_s(place,L"%ls\\DATA",cd); 
			pool.vars_strings.back()+=rnplace;
			TreeView_SetItemState(tv,bold,~0,TVIS_BOLD);			
			TreeView_InsertItem(tv,&tvis2);
		}					
		const wchar_t *data = Sompaste->get(L"DATA"); 
		for(int j=0;j=Sompaste->place(0,place,data,0);data+=j)
		{				
			pool.vars_strings.back()+=rnplace;
			TreeView_InsertItem(tv,&tvis2);
		}
	}
	tvis.item.pszText = SOM_EDIT_tree[L"-SCRIPT"];	
	swprintf_s(place,L"%ls\\lang",user);
	pool.vars_strings.push_back(place); //balloon tip
	tvis.item.lParam = pool.vars_strings_back_param();
	tvis2.hParent = TreeView_InsertItem(tv,&tvis);
	bold = TreeView_InsertItem(tv,&tvis2);		
	{
		if(use)
		{
			swprintf_s(place,L"%ls\\lang",cd);
			pool.vars_strings.back()+=rnplace;
			TreeView_SetItemState(tv,bold,~0,TVIS_BOLD);
			TreeView_InsertItem(tv,&tvis2);
		}			 
		const wchar_t *script = Sompaste->get(L"SCRIPT"); 
		for(int j=0;j=Sompaste->place(0,place,script,0);script+=j)
		{
			pool.vars_strings.back()+=rnplace;
			TreeView_InsertItem(tv,&tvis2);
			wcscat_s(place,L"\\lang");
			pool.vars_strings.back()+=rnplace;
			TreeView_InsertItem(tv,&tvis2);
		}		
	}
	tvis.item.pszText = SOM_EDIT_tree[L"-FONT"];	
	swprintf_s(place,L"%ls\\font",user);
	pool.vars_strings.push_back(place); //balloon tip
	tvis.item.lParam = pool.vars_strings_back_param();
	tvis2.hParent = TreeView_InsertItem(tv,&tvis);
	bold = TreeView_InsertItem(tv,&tvis2);
	{			 	
		if(use)
		{
			swprintf_s(place,L"%ls\\font",cd);
			pool.vars_strings.back()+=rnplace;
			TreeView_SetItemState(tv,bold,~0,TVIS_BOLD);
			TreeView_InsertItem(tv,&tvis2);
		}			   		
		const wchar_t *font = Sompaste->get(L"FONT");	
		for(int j=0;j=Sompaste->place(0,place,font,0);font+=j)
		{
			pool.vars_strings.back()+=rnplace;
			TreeView_InsertItem(tv,&tvis2);			
		}											 		
		const wchar_t *script = Sompaste->get(L"SCRIPT");	
		for(int j=0;j=Sompaste->place(0,place,script,0);script+=j)
		{			
			wcscat_s(place,L"\\font"); 
			pool.vars_strings.back()+=rnplace;
			TreeView_InsertItem(tv,&tvis2);
		}
	}	
	
	const wchar_t *ex = Sompaste->get(L"EX"); 
	const wchar_t *ini = Sompaste->get(L"INI");	 	
	const wchar_t *icon = Sompaste->get(L"ICON");
	const wchar_t *text = Sompaste->get(L"TEXT");

	if(*ini)
	{
		tvis.item.pszText = SOM_EDIT_tree[L"-INI"];
		pool.vars_strings.push_back(L""); //balloon tip
		tvis.item.lParam = pool.vars_strings_back_param();
		tvis2.hParent = TreeView_InsertItem(tv,&tvis);
		
		if(PathIsRelativeW(ini))
		{
			swprintf_s(place,L"%ls\\%s",user,ini);
			pool.vars_strings.back()+=place;
			bold = TreeView_InsertItem(tv,&tvis2);			

			if(use)
			{
				if(PathFileExistsW(place))
				tvis2.item.state|=TVIS_CUT;				
				swprintf_s(place,L"%ls\\%s",cd,ini);
				pool.vars_strings.back()+=rnplace;
				TreeView_SetItemState(tv,bold,~0,TVIS_BOLD);
				TreeView_InsertItem(tv,&tvis2);			
				tvis2.item.state&=~TVIS_CUT;				
			}			
		}
		else 
		{
			wcscpy_s(place,ini);
			pool.vars_strings.back()+=place;
			TreeView_InsertItem(tv,&tvis2);			
		}
	}
	if(*ex)
	{
		tvis.item.pszText = SOM_EDIT_tree[L"-EX"];
		pool.vars_strings.push_back(L""); //balloon tip
		tvis.item.lParam = pool.vars_strings_back_param();
		tvis2.hParent = TreeView_InsertItem(tv,&tvis);		
		for(int j=0,_=0;j=Sompaste->place(0,place,ex,0);ex+=j)
		{
			pool.vars_strings.back()+=rnplace+(_++?0:2);
			bold = TreeView_InsertItem(tv,&tvis2); 
			if(use&&PathIsPrefixW(cd,place)&&wcsicmp(cd,user)) 
			{	
				if(PathFileExistsW(place))
				tvis2.item.state|=TVIS_CUT;
				wchar_t rel[MAX_PATH] = L"";
				wcscpy_s(rel,place+wcslen(cd)+1);
				swprintf_s(place,L"%ls\\%ls",user,rel);				
				pool.vars_strings.back()+=rnplace;
				TreeView_InsertItem(tv,&tvis2);
				tvis2.item.state&=~TVIS_CUT;
			}			
		}
	}
	if(*icon)
	{
		tvis.item.pszText = SOM_EDIT_tree[L"-ICON"];
		pool.vars_strings.push_back(L""); //balloon tip
		tvis.item.lParam = pool.vars_strings_back_param();
		tvis2.hParent = TreeView_InsertItem(tv,&tvis);
		
		size_t _ = wcscspn(icon,L"\\/");

		if(icon[_])
		{
			wcscpy_s(place,icon);
			pool.vars_strings.back()+=place;
			TreeView_InsertItem(tv,&tvis2);	
		}
		if(PathIsRelativeW(icon))
		{
			swprintf_s(place,L"%ls\\%s",cd,icon);
			pool.vars_strings.back()+=rnplace+(icon[_]?2:0);
			TreeView_InsertItem(tv,&tvis2);	
		}
		else 
		{
			wcscpy_s(place,icon);
			pool.vars_strings.back()+=rnplace;
			TreeView_InsertItem(tv,&tvis2);	
		}	   
	}
	if(*text)
	{
		tvis.item.pszText = SOM_EDIT_tree[L"-TEXT"];
		pool.vars_strings.push_back(L""); //balloon tip
		tvis.item.lParam = pool.vars_strings_back_param();
		tvis2.hParent = TreeView_InsertItem(tv,&tvis);						
		const wchar_t *text = Sompaste->get(L"TEXT");
		for(int j=0,_=0;j=Sompaste->place(0,place,text,0);text+=j)
		{
			pool.vars_strings.back()+=rnplace+(_++?0:2);
			TreeView_InsertItem(tv,&tvis2);
		}
	}
	
	TVITEMEXW &tvi = tvis.itemex; tvi.lParam = 0;
	tvi.pszText = place; tvi.cchTextMax = MAX_PATH;
	HTREEITEM i = TreeView_GetChild(tv,zone); while(i)
	{
		bool cut = true; 
		tvi.hItem = TreeView_GetChild(tv,i);			
		while(tvi.hItem)
		{
			*place = '\0'; //paranoia
			TreeView_GetItem(tv,&tvi);
			DWORD attribs = GetFileAttributesW(place);
			if(attribs!=INVALID_FILE_ATTRIBUTES)
			{
				cut = false;

				//continued by SOM_EDIT_zone2 below
				if(attribs&FILE_ATTRIBUTE_DIRECTORY)
				{	
					if(SOM_EDIT_deep(place)) //courtesy
					{
						//TVIF_CHILDREN/cChildren doesn't work
						tvis.hParent = tvi.hItem; *place = '\0';
						TreeView_InsertItem(tv,(LPARAM)&tvis);
					}
				}
			}
			else if(place[wcscspn(place,L"\\/")])
			{
				TreeView_SetItemState(tv,tvi.hItem,~0,TVIS_CUT);
			}
			else cut = false; //todo: search PATH variable etc?

			tvi.hItem = TreeView_GetNextSibling(tv,tvi.hItem);
		}			  
		if(cut) TreeView_SetItemState(tv,i,~0,TVIS_CUT); 		
		i = TreeView_GetNextSibling(tv,i);
	}

	return 0;
}
static size_t SOM_EDIT_path(HWND tv, HTREEITEM i, int depth, wchar_t path[MAX_PATH])
{
	size_t cat = depth==0?0:
	SOM_EDIT_path(tv,TreeView_GetParent(tv,i),depth-1,path);
	TVITEMEXW tvi = {TVIF_TEXT,i}; 
	tvi.pszText = path+cat; tvi.cchTextMax = MAX_PATH-cat-1;
	if(cat&&++cat) *tvi.pszText++ = '\\'; 	
	*tvi.pszText = '\0'; TreeView_GetItem(tv,&tvi);
	return cat+wcslen(tvi.pszText); 
}
static int SOM_EDIT_zone2(HWND tv, HTREEITEM item) 
{	
	wchar_t path[MAX_PATH] = L"";
	HTREEITEM gc = TreeView_GetChild(tv,item); 	
	TVINSERTSTRUCTW tvis = {item,TVI_LAST,{TVIF_TEXT,gc,0,~0}};
	tvis.item.pszText = path; tvis.item.cchTextMax = MAX_PATH;
	if(gc&&!TreeView_GetItem(tv,&tvis.item))
	{
		*path = '!'; assert(0); //paranoia
	}
	if(*path) return 0; //previously expanded

	int lv = SOM_EDIT_level(tv,item); 
	
	if(lv<2) return 0; //theoretical possibile

	if(gc) TreeView_DeleteItem(tv,gc); //dummy

	//not required to use the dummy technique
	tvis.item.mask|=TVIF_STATE|TVIF_CHILDREN;
	tvis.item.state = 0x2000; tvis.item.cChildren = 1;

	SOM_EDIT_path(tv,item,lv-2,path); 
	size_t cat = SOM_EDIT_path(tv,item,lv-2,path); 	
	wcscpy_s(path+cat,MAX_PATH-cat,L"\\*");

	WIN32_FIND_DATAW found; 					
	HANDLE glob = FindFirstFileW(path,&found);
	wchar_t *p = tvis.item.pszText = found.cFileName;
	if(glob!=INVALID_HANDLE_VALUE) do	
	if(found.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
	{
		if(!p[wcsspn(p,L".")]) continue; 
		wcscpy_s(path+cat+1,MAX_PATH-cat-1,p);
		tvis.item.cChildren = SOM_EDIT_deep(path); 
		TreeView_InsertItem(tv,&tvis);
	}
	while(FindNextFileW(glob,&found)); FindClose(glob);	
	TreeView_SortChildren(tv,item,0); 	
	return 0;
}
static void SOM_EDIT_open(HWND tv, HTREEITEM item) 
{
	wchar_t open[MAX_PATH] = L"";
	int lv = SOM_EDIT_level(tv,item); 	
	if(lv>1) SOM_EDIT_path(tv,item,lv-2,open); 
	if(!*open //if empty the current directory is opened!
	 ||(HINSTANCE)33>ShellExecuteW(GetParent(tv),L"openas",open,0,0,1)
     &&(HINSTANCE)33>ShellExecuteW(GetParent(tv),L"open",open,0,0,1))
	MessageBeep(-1);
}