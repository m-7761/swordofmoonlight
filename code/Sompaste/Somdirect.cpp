#include "Sompaste.pch.h"

#include <map>
#include <vector>
#include <string>	   

static bool Somdirect_failure = false; //TODO: paranoia

#define SOMDIRECT L"Software\\FROMSOFTWARE\\SOM\\DIRECT"

//Notice: this file was hastily adapted from code cut from Somplayer.dll

namespace Somdirect_cpp
{
	static struct _section 
	{
		CRITICAL_SECTION cs; //Winbase.h

		_section(){ InitializeCriticalSection(&cs); }
		~_section(){ DeleteCriticalSection(&cs); }

	}_section; struct section
	{
		section(){ EnterCriticalSection(&_section.cs); }
		~section(){ LeaveCriticalSection(&_section.cs); }
	};				  

	struct filter
	{
		enum { type_s = 64, path_s = MAX_PATH };

		bool hide; wchar_t type[type_s+1], path[path_s+1]; bool sel;

		void init(){ hide = sel = *type = *path = type[type_s] = path[path_s] = 0; }

		bool operator==(const filter &cmp){	return !memcmp(this,&cmp,sizeof(cmp)); }
	};															 

	struct prefilter //might setup a cache at some point
	{			
		wchar_t product[filter::type_s+1]; //cross filter

		prefilter(const wchar_t (&type)[filter::type_s+1], const wchar_t *filter);

		operator wchar_t*(){ return product; }
	};

	typedef std::vector<filter> folder;   

	static std::map<std::wstring,folder> folders;  
	
	static const filter new_filter = {1, L"", L""};

	static const filter defaults[] = 
	{
		{0, L"*", L"."},
		{0, L"mdl; mdo", L"..\\model"},
		{0, L"msm", L"..\\msm"},
		{0, L"pal; txr", L"..\\texture"},
		{0, L"bmp", L".\\bmp"},
		{1, L"", L""},
	};

	const size_t defaults_s = sizeof(defaults)/sizeof(filter);

	static folder results; //Somdirect_results

	struct multi_sz	//eg. REG_MULTI_SZ
	{
		wchar_t *out, *rw; DWORD cb; bool cp;	  
		multi_sz(wchar_t *home, DWORD read_cb=0, wchar_t *read=0);
		~multi_sz();	
	};
}					 

Somdirect_cpp::multi_sz::multi_sz(wchar_t *home, DWORD read_cb, wchar_t *read)
{	
	assert(!read||read_cb);

	out = read||read_cb?home:0; 

	rw = read; cb = read_cb; cp = !read;

	if(out&&cp) 
	{
		rw = (wchar_t*)new char[read_cb]; *rw = 0; 
	}

	if(out) return; //writing...
	
	Somdirect_cpp::section cs;

	folder &f = folders[home];

	using Sompaste_pch::array;

	//avoiding std::string
	static const int w_s = 512;
	static std::vector<array<wchar_t,w_s> >vw;

	array<wchar_t,w_s> ln; vw.clear();

	for(size_t i=0,n=f.size();i<n;i++)
	{
		filter &g = f[i]; if(!*g.path) continue;

		const wchar_t *f = L"N%c, %s, \"%s\"";
		ln[0] = -1+swprintf_s(ln._s,f,g.hide?'m':L'a',g.type,g.path);		
		vw.push_back(ln); 

		if(ln[0]>=w_s) ln[0] = 0; //paranoia
	}

	size_t sz = 0;
	for(size_t i=0,n=vw.size();i<n;i++)
	{
		sz+=vw[i][0]+1;
	}				  
	sz = sz?sz+1:2; 

	rw = new wchar_t[sz];
	cb = sz*sizeof(wchar_t);

	rw[0] = 0;

	size_t at = 0;
	for(size_t i=0,n=vw.size();i<n;i++)
	{
		wcsncpy(rw+at,vw[i]+1,sz-at);

		rw[at+=vw[i][0]] = 0; at++;
	}

	rw[sz-1] = 0;
}

Somdirect_cpp::multi_sz::~multi_sz()
{
	assert(rw&&cb);
			
	if(!out&&cp) delete rw;

	if(!out) return; //reading...
	
	Somdirect_cpp::section cs;

	folder &f = folders[out]; f.clear();

	size_t m = cb/sizeof(wchar_t);

	wchar_t *p, *b, *q, *d;
	for(p=rw,b=p+m;p<b&&*p;p++)
	{	
		filter g; g.init();
		
		size_t n = wcsnlen(p,b-p);

		int sep = 0, quote = 0;

		for(q=p,d=q+n;q<=d;q++) 
		{
			if(*q=='\"') quote = !quote; 

			if(quote||*q!=','&&*q!='\0') continue;
				
			switch(sep)
			{
			case 0:
			{
				while(p<q) switch(*p++)
				{
				case 'm': g.hide = true; break; //manual
				case 'a': g.hide = false; break; //automatic
				}

			}break;				
			case 1: case 2: 
			{	
				while(p<q&&*p==' ') p++; 
				
				if(quote=*p=='"') p++;

				int len = q-p; while(len&&p[n-1]==' ') len--;

				if(quote&&p[len-1]=='"') len--;

				len = std::min<size_t>(len,sep==1?g.type_s:g.path_s);

				wcsncpy(sep==1?g.type:g.path,p,len)[len] = 0;

				quote = 0;

			}break;
			}				

			p = q+1; sep++; 
		}

		f.push_back(g);

		p = d;
	}

	//ever present blank row
	f.push_back(new_filter);
				  
	if(cp) delete rw;
}

static bool Somdirect_register(wchar_t *home)
{
	if(!home) home = L""; //places

	LONG err; Somdirect_cpp::multi_sz wr(home);

	if(wr.cb>sizeof(wchar_t)*2||!*home)	
	{
		err = SHSetValueW(HKEY_CURRENT_USER,SOMDIRECT,home,REG_MULTI_SZ,wr.rw,wr.cb);		
	}
	else err = SHDeleteValueW(HKEY_CURRENT_USER,SOMDIRECT,home);		

	return !err;
}

static Somdirect_cpp::folder &Somdirect_folders(wchar_t *home)
{
	if(!home) home = L""; //places
	
	Somdirect_cpp::folder &out = Somdirect_cpp::folders[home];
	
	static Somdirect_cpp::folder &places = Somdirect_folders(L""); //initialize
		
	if(out.size()) return out; //pull from cache
	
	DWORD cb = 0; LONG err = 
	SHGetValueW(HKEY_CURRENT_USER,SOMDIRECT,home,0,0,&cb);

	size_t test = Somdirect_cpp::defaults_s;

	if(!cb) //initialize registry
	{	
		if(!*home) for(size_t i=0;i<Somdirect_cpp::defaults_s;i++) 
		{
			out.push_back(Somdirect_cpp::defaults[i]); //load default places
		}

		Somdirect_cpp::multi_sz wr(L""); //initialize with places
		
		if(!*home) err = //then delay write to registry (for instance the MDL archives)
		SHSetValueW(HKEY_CURRENT_USER,SOMDIRECT,home,REG_MULTI_SZ,wr.rw,wr.cb);		

		Somdirect_cpp::multi_sz(home,wr.cb,wr.rw); //rd
	}	
	else //pull from registry
	{
		Somdirect_cpp::multi_sz rd(home,cb); err = 
		SHGetValueW(HKEY_CURRENT_USER,SOMDIRECT,home,0,rd.rw,&rd.cb);
	}

  //// ensuring fully qualified paths for now ////

	if(*home) //assuming home is not relative
	{
		Somdirect_cpp::section cs;

		wchar_t absolute[MAX_PATH] = L""; 

		for(size_t i=0,n=out.size();i<n;i++)
		{			
			Somdirect_cpp::filter &g = out[i]; if(!*g.path) continue;

			SOMPASTE_LIB(Path)(PathCombineW(absolute,home,g.path));
			wcscpy_s(g.path,absolute);
		}
	}

  ////////////////////////////////////////////////

	return out;	
}							  

Somdirect_cpp::prefilter::
prefilter(const wchar_t (&type)[filter::type_s+1], const wchar_t *filter)
{
	size_t i = 0, j = 0, k = 0;
	size_t s = wcsnlen(filter,filter::type_s); 

	for(i=0,j=0;i<=s;i++) switch(filter[i])
	{		
	case ' ': continue; //white space

	default: product[j++] = filter[i]; break;

	case '\0': case ';': 
		
		product[j++] = ';';  
			   
		if(*type!='*')
		for(const wchar_t *p=type,*q=p,*d=p+filter::type_s;q<d;q++)
		{
			if(*q==';'||*q=='\0')
			{
				while(*p==' ') p++; 
				
				if(j-k-1==q-p&&!wcsnicmp(product+k,p,q-p)) k = j; 
								
				if(*q=='\0'||k==j) break;

				p =	q+1; //continue			
			}
		}
		else k = j;	if(k!=j) j = k;

		if(!filter[i]) break;
	}

	product[j] = '\0';
}

static size_t Somdirect_results2(Somdirect_cpp::folder&,wchar_t*,wchar_t*,const wchar_t*,wchar_t[MAX_PATH]);

static size_t Somdirect_results(wchar_t *home, wchar_t *name, const wchar_t *filter, wchar_t result[MAX_PATH]=0)
{
	Somdirect_cpp::section cs; assert(home&&*home);

	return !home||!*home?0:Somdirect_results2(Somdirect_folders(home),home,name,filter,result);
}

static size_t Somdirect_results2(Somdirect_cpp::folder &f, wchar_t *home, wchar_t *name, const wchar_t *filter, wchar_t result[MAX_PATH]=0)
{	
	Somdirect_cpp::section cs;	

	size_t out = 0; if(!result) Somdirect_cpp::results.clear();

	//Returning the number of results matched 
	//If result is nonzero at most one match is returned via result
	//If result is 0, all results must be obtained from Somdirect_cpp::results
	//(in which case Somdirect_results should be called within a Somdirect_cpp::section block)

	if(!f.size()||!home||!*home||!name||!*name||!filter||!*filter)
	{
		assert(0); return 0; //let's be clear here (unsupported)
	}	
		
	wchar_t *dot = PathFindExtensionW(name);

	if(*dot=='.') *dot = '\0'; //IMPORTANT #1
		
	for(size_t i=0,m=f.size();i<m;i++)
	{			
		const Somdirect_cpp::filter &g = f[i]; 
		
		if(result&&g.hide||!*g.path||!*g.type) continue; 					

		Somdirect_cpp::prefilter pre(g.type,filter); 
		
		if(!*pre) continue; //may want to cache eventually

	//TODO: search _should_ include (non-file) shell items	

		wchar_t find[MAX_PATH] = L""; 
		int n = swprintf_s(find,L"%s\\%s.*",g.path,name);

		if(n>1) while(find[n-1]!='.'&&n) n--; else continue; 

		WIN32_FIND_DATAW found; //optimization?
		HANDLE glob = FindFirstFileW(find,&found); //*				
		if(glob==INVALID_HANDLE_VALUE) continue; //early out
		FindClose(glob); 		

		for(const wchar_t *p=pre,*q=p,*d=p+g.type_s;q<d;q++)
		{
			if(*q==';'||*q=='\0') //filtering types only for now
			{
				const int safe_limit = 32;

				if(q-p) //else assume double delimiters
				{
					wcsncpy_s(find+n,MAX_PATH-n-1,p,q-p); find[q-p+n] = '\0';
									
					glob = FindFirstFileW(find,&found); 
				}
				else glob = INVALID_HANDLE_VALUE;

				if(glob!=INVALID_HANDLE_VALUE) 
				for(int safety=0;safety<safe_limit;safety++) 
				{	
					if(result) //take the first "automatic" match 
					{	
						FindClose(glob);

						if(*dot=='\0') *dot = '.'; //IMPORTANT #1.5

						if(!wcsicmp(name,found.cFileName))
						{
							if(!wcsicmp(home,g.path)) return 0; //input matches output
						}
						
						n = swprintf_s(result,MAX_PATH,L"%s\\%s",g.path,found.cFileName);

						result[n>0?n:0] = '\0'; return 1; //paranoia
					}
										
					Somdirect_cpp::filter dup = g; 

					wcsncpy_s(dup.type,dup.type_s,p,q-p); dup.type[q-p] = '\0';

					for(size_t j=0;j<=out;j++) if(j==out) 
					{
						out++; Somdirect_cpp::results.push_back(dup); break;
					}
					else if(dup==Somdirect_cpp::results[j]) break; //case sensitive

					if(!FindNextFileW(glob,&found))
					{				
						assert(GetLastError()==ERROR_NO_MORE_FILES); 							
						break;
					}		
				}	
				FindClose(glob);

				if(*q=='\0') break;

				p =	q+1;
			}
		}		
	}

	if(*dot=='\0') *dot = '.'; //IMPORTANT #2

	return out;
}

static LRESULT CALLBACK //forward declaring
SomdirectDetailsProc(HWND,UINT,WPARAM,LPARAM,UINT_PTR,DWORD_PTR); 

namespace Somdirect_cpp
{
	struct tabprops 
	{		
		HWND propsheet;

		tabprops(HWND dlg){ propsheet = dlg; }

		int tab() //propsheet
		{	
			HWND tabs = PropSheet_GetTabControl(propsheet);
			return TabCtrl_GetCurSel(tabs);
		}
	};

	struct details //work in progress
	{
		HWND listview, edit; RECT editrect; 

		bool dragdrop; int dragging, editlabel, edits;

		//return false to cancel default behavior
		virtual	bool drop(int target, int handle, int button=0){ return true; }

		details(HWND lv=0, SUBCLASSPROC proc=0)
		{
			listview = lv; edit = 0; editing = -1; 

			dragdrop = false; dragging = editlabel = -1; edits = 0; 

			SetWindowSubclass(lv,proc,0,(DWORD_PTR)this);
		}
				
		bool endlabeledit(NMLVDISPINFOW *p)
		{
			//a FALSE return is mostly for the purpose of Apply activation
			if(!p->item.pszText||(p->item.iSubItem=editing)<0) return FALSE; 
						
			if(wcslen(p->item.pszText)<MAX_PATH) //consider an optimization			
			{
				wchar_t diff[MAX_PATH] = L"";
				LVITEMW lvi = {LVIF_TEXT,p->item.iItem,editing,0,0,diff,MAX_PATH};
				if(MAX_PATH>=SendMessageW(listview,LVM_GETITEMTEXTW,p->item.iItem,(LPARAM)&lvi))
				{
					if(!wcsncmp(p->item.pszText,diff,MAX_PATH)) return FALSE;
				}
			}		

			LRESULT out = 
			SendMessageW(listview,LVM_SETITEMTEXTW,p->item.iItem,(LPARAM)&p->item);
			if(out) edits++; 
			return out;
		}

		int editing; //private
	}; 

	struct dirprops 
	:
	public tabprops, public details
	{	
		wchar_t filter[MAX_PATH];
		wchar_t _select[MAX_PATH];

		wchar_t home[MAX_PATH], *name, *type;

		Somdirect_cpp::folder snapshot; //tab switching

		inline int tab() //shadowing
		{			
			//with or without Search Results?
			return tabprops::tab()+(*filter?0:1);
		}

		dirprops(HWND dlg, HWND lv, const Somdirect_cpp::filter &f)
		:
		tabprops(dlg), details(lv,SomdirectDetailsProc) //hack
		{				
			wcscpy_s(filter,f.type); wcscpy_s(home,f.path);
			
			*_select = 0; name = type = L""; if(!*f.type) return;

			name = PathFindFileNameW(home);	type = PathFindExtensionW(name); 
			
			if(name!=home) name[-1] = 0; else assert(0); //not good

			if(*type=='.') type++; else assert(0); //not good
		}

		void refresh()
		{
			HWND self = GetParent(listview); //hack						

			PSHNOTIFY pn = { {propsheet, tab(), PSN_SETACTIVE}, 0 };

			SendMessage(self,WM_NOTIFY,0,(WPARAM)&pn);
			
			if(pn.hdr.idFrom!=1) return; //hack: convenient

			PropSheet_Changed(propsheet,self);

			edits++;
		}

		void select(int i)
		{				
			HWND self = GetParent(listview); //hack

			size_t ext = type-name;	assert(tab()==0);

			wchar_t folder[MAX_PATH] = L""; 
			LVITEMW lvi = {LVIF_TEXT,i,2,0,0,folder,MAX_PATH};
			SendMessageW(listview,LVM_GETITEMTEXTW,i,(LPARAM)&lvi);

			wchar_t title[MAX_PATH] = L""; lvi.iSubItem = 1;
			lvi.pszText = wcscpy(title,name)+ext; lvi.cchTextMax-=ext; 				
			SendMessageW(listview,LVM_GETITEMTEXTW,i,(LPARAM)&lvi);

			if(*folder&&title[ext]&&IsWindowEnabled(listview))
			{
				swprintf_s(_select,L"%s\\%s",folder,title);
			}
			else *_select = '\0';

			SetProp(propsheet,"Sompaste_select",(HANDLE)_select);

			PropSheet_Changed(propsheet,self);
		}

		bool drop(int target, int handle, int button=0) //virtual
		{					
			if(target==handle||target==handle+1)
			{
				if(target>=0&&handle>=0)
				if(ListView_GetSelectedCount(listview)<2) return false;
			}

			size_t m = snapshot.size();

			if(target>=int(m)) //yikes 
			{					
				target = m-1; assert(!*snapshot[target].path);
			}

			Somdirect_cpp::folder out, mov; 
			
			for(size_t i=0;i<m;i++)
			{
				Somdirect_cpp::filter &g = snapshot[i];

				if(!g.sel||button==IDC_USE&&g.hide) continue; 

				mov.push_back(g); *g.path = 0; //mark for removal
			}			
			for(size_t i=0;i<m;i++)
			{
				if(i==target) //drop move in here
				for(size_t j=0,n=mov.size();j<n;j++) out.push_back(mov[j]);
			
				if(*snapshot[i].path) out.push_back(snapshot[i]);			
			}

			out.push_back(Somdirect_cpp::new_filter);

			snapshot = out;	refresh(); 			
			
			return false; 
		}
	};
}

static LRESULT CALLBACK SomdirectDetailsEdit(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scid, DWORD_PTR p) 
{		
	switch(uMsg) 
    { 	
	case WM_WINDOWPOSCHANGING: 
	{
		//pin edit rectangle to subitem

		WINDOWPOS *wp = (WINDOWPOS*)lParam;

		RECT rect = ((Somdirect_cpp::details*)p)->editrect;

		wp->x = rect.left; wp->cx = rect.right-rect.left;

	}break;
	case WM_KEYDOWN: case WM_KEYUP: 	
	{
		if(wParam!=VK_RETURN) break; //key is sticky

		if(uMsg==WM_KEYDOWN) return 1; //discard
												 
		DefSubclassProc(hwnd,WM_KEYDOWN,wParam,0);
		DefSubclassProc(hwnd,WM_KEYUP,wParam,1<<30);		

	}break;
	case WM_NCDESTROY:

		((Somdirect_cpp::details*)p)->editing = -1;

		RemoveWindowSubclass(hwnd,SomdirectDetailsEdit,scid);

		break;
	}

	return DefSubclassProc(hwnd,uMsg,wParam,lParam);
}

static LRESULT CALLBACK SomdirectDetailsProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR scid, DWORD_PTR p) 
{		
	HWND &hWndDlg = hwnd, &lv = hwnd; 

	Somdirect_cpp::details &view = *(Somdirect_cpp::details*)p;
	
	static bool arrow = false; //hack??

	switch(uMsg) 
    { 		
	case WM_LBUTTONDBLCLK: 
	{
		LVHITTESTINFO ht = 
		{
			{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)}
		}; 		
		ListView_SubItemHitTest(lv,&ht);

		if(ht.iItem==-1) ht.iItem = ListView_GetItemCount(lv)-1;

		view.editlabel = ht.iSubItem;
		SendMessage(hwnd,LVM_EDITLABELW,ht.iItem,0);						
				
	}return 1;
	case LVM_EDITLABELA: 
	case LVM_EDITLABELW: 
	{			
		if(view.editlabel==-1) return 0;

		RECT rect; int label = view.editlabel; 		
		ListView_GetSubItemRect(lv,wParam,label,LVIR_BOUNDS,&rect);			
				
		if(view.edit=(HWND)DefSubclassProc(hwnd,uMsg,wParam,lParam))
		{
			view.editrect = rect; //Reminder: timing is important here

			wchar_t text[MAX_PATH] = L"";  
			LVITEMW lvi = {LVIF_TEXT,wParam,label,0,0,text,MAX_PATH};
			SendMessageW(lv,LVM_GETITEMTEXTW,wParam,(LPARAM)&lvi);

			//Reminder: SomwindowsListViewEdit locks in position
			SetWindowPos(view.edit,0,rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top,0);		
			SetWindowSubclass(view.edit,SomdirectDetailsEdit,0,(DWORD_PTR)&view); 
			SetWindowTextW(view.edit,text);
		}
				
		view.editing = label; //keepsake
		view.editlabel = -1; //invalidate
		
		return (LRESULT)view.edit;
	}
	case WM_LBUTTONDOWN: 
	{	  
		//hack: should not be necessary???
		//(not done by DefSubClassProc either)
		if(GetFocus()!=hwnd) SetFocus(hwnd);
		
		LVHITTESTINFO ht = 
		{
			{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)}
		}; 		
		
		ListView_SubItemHitTest(lv,&ht);

		if(ht.iItem==-1)
		{
			ht.iItem = ListView_GetItemCount(lv)-1;

			POINT pt; //hack: DefSubclassProc...
			ListView_GetItemPosition(lv,ht.iItem,&pt);
			lParam = LOWORD(lParam)|WORD(pt.y+4)<<16;
		}

		//hack: avoid interference from DragDetect 
		LRESULT out = DefSubclassProc(hwnd,uMsg,wParam,lParam);
		
		if(view.dragdrop) 
		if(ClientToScreen(lv,&ht.pt)&&DragDetect(lv,ht.pt))
		if(ht.iItem||!ChildWindowFromPoint(ListView_GetHeader(lv),ht.pt))
		{
			//TODO: ListView_CreateDragImage?

			DWORD lvs_ex_style = LVS_EX_INFOTIP;	  
			ListView_SetExtendedListViewStyleEx(lv,lvs_ex_style,0);

			SetCapture(hWndDlg);				  
			SetCursor(LoadCursor(0,IDC_SIZENS));

			view.dragging = ht.iItem;
		}					

		return out;		
	
	}break;
	case WM_CAPTURECHANGED: break;
	{
		////Doesn't seem to behave as you might expect???////
		//if((HWND)lParam!=hWndDlg) SetCursor(LoadCursor(0,IDC_ARROW));

		return 1;

	}break;
	case WM_LBUTTONUP:
	{	
		if(view.dragging!=-1)
		{	
			int target = 0;
			int handle = view.dragging;

			view.dragging = -1;

			LVHITTESTINFO ht = 
			{
				{0, GET_Y_LPARAM(lParam)}
			}; 								
			ListView_HitTest(lv,&ht);
			target = ht.iItem!=-1?ht.iItem:ListView_GetItemCount(lv);				

			if(view.drop(target,handle))
			{
				assert(0); //TODO: default implementation (shuffle the listview)
			}

			DWORD lvs_ex_style = LVS_EX_INFOTIP;	  
			ListView_SetExtendedListViewStyleEx(lv,lvs_ex_style,-1UL);		
					
			SetCursor(LoadCursor(0,IDC_ARROW));			

			ReleaseCapture(); return 1;
		}

	}break;
	case WM_KEYDOWN: case WM_KEYUP: 
	{
		//lazy: easier than subclassing keyboard input for now

		//WARNING: seems to always return 0 (ie. effect or not)
		LRESULT out = DefSubclassProc(hwnd,uMsg,wParam,lParam);

		//hack: allow parent to process keyboard inputs (eg. X delete)
		if(!out) return SendMessage(GetParent(hwnd),uMsg,wParam,lParam);

		return out;

	}break;
	case WM_NCDESTROY:
	
		view.dragging = -1;

		RemoveWindowSubclass(hwnd,SomdirectDetailsProc,scid);
		break;
	}

	return DefSubclassProc(hwnd,uMsg,wParam,lParam);
}

static INT_PTR CALLBACK SomdirectFolderProc(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	BOOL shortcut = 0; //hack

	Somdirect_cpp::dirprops *props = 
	(Somdirect_cpp::dirprops*)GetWindowLongPtrW(hWndDlg,GWLP_USERDATA);		

	if(!&props&&Msg!=WM_INITDIALOG) return 0; //paranoia

	switch(Msg)
	{
	case WM_INITDIALOG: 
	{			
		HWND propsheet = GetParent(hWndDlg);

		EnableThemeDialogTexture(hWndDlg,ETDT_ENABLETAB);
		
		HWND lv = GetDlgItem(hWndDlg,IDC_LISTVIEW);		
		DWORD lvs_ex_style = LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_INFOTIP;	  
		ListView_SetExtendedListViewStyle(lv,lvs_ex_style);
				
		Somdirect_cpp::filter &f = *(Somdirect_cpp::filter*)lParam;

		props = new Somdirect_cpp::dirprops(propsheet,lv,f);
		SetWindowLongPtrW(hWndDlg,GWLP_USERDATA,(LONG_PTR)props);
		
		HWND tabs = PropSheet_GetTabControl(propsheet);
				
		int t = 0; TCITEMW tab = 
		{ TCIF_TEXT,0,0,L"Search Results"};
		if(*props->filter) //including Search Results
		SendMessageW(tabs,TCM_SETITEMW,t++,(LPARAM)&tab); 
		tab.pszText = L"Searched Locations";
		SendMessageW(tabs,TCM_SETITEMW,t++,(LPARAM)&tab); 
		tab.pszText = L"Home Location";
		SendMessageW(tabs,TCM_SETITEMW,t++,(LPARAM)&tab); 

		LVCOLUMNW lvc = {LVCF_TEXT|LVCF_FMT|LVCF_WIDTH};  
		lvc.fmt = LVCFMT_LEFT;

		RECT room; LONG &remaining = room.right; 			
		GetClientRect(lv,&room);

		lvc.pszText = L""; remaining-=lvc.cx = 20; 
		SendMessage(lv,LVM_INSERTCOLUMNW,0,(LPARAM)&lvc);
		lvc.pszText = L"Type"; remaining-=lvc.cx = 60; 
		SendMessage(lv,LVM_INSERTCOLUMNW,1,(LPARAM)&lvc);
		lvc.pszText = L"Folder"; lvc.cx = remaining; 
		SendMessage(lv,LVM_INSERTCOLUMNW,2,(LPARAM)&lvc); 		
				
		Somdirect_cpp::section cs;

		props->snapshot = Somdirect_folders(props->home); 
		props->refresh(); 

		ShowWindow(hWndDlg,SW_SHOW); 

		return FALSE;
	}	
	case WM_NOTIFY:
	{
		PSHNOTIFY *p = (PSHNOTIFY*)lParam;

		switch(p->hdr.code) //how a property sheet would work
		{	
		case LVN_BEGINLABELEDITA: assert(0);
		case LVN_BEGINLABELEDITW:
		{
			BOOL out = TRUE; //deny edit

			switch(props->tab())
			{
			case 0: //Search Results
				
				PostMessage(hWndDlg,WM_COMMAND,ID_SELECT,0);

				out = TRUE; break; //deny edit
			
			case 1: //Searched Locations

				out = props->editlabel<1; break; 

			case 2: //Home Location

				out = TRUE; break; //deny edit
			}

			SetWindowLong(hWndDlg,DWL_MSGRESULT,out); 
			return TRUE;
		}
		case LVN_ENDLABELEDITA: assert(0); break; 
		case LVN_ENDLABELEDITW:
		{
			NMLVDISPINFOW *p = (NMLVDISPINFOW*)lParam;		
						    
			if(!props->endlabeledit(p)) break;
						
			Somdirect_cpp::filter &g = props->snapshot[p->item.iItem];
						
			switch(p->item.iSubItem)
			{
			case 1: wcsncpy(g.type,p->item.pszText,g.type_s); break;
			case 2: wcsncpy(g.path,p->item.pszText,g.path_s); break;
			}

			if(!*g.type) wcscpy(g.type,L"*"); 
			if(!*g.path) wcscpy(g.path,L".");

			if(p->item.iItem==props->snapshot.size()-1) //a new row
			{	
				assert(p->item.iItem==ListView_GetItemCount(props->listview)-1);
				props->snapshot.push_back(Somdirect_cpp::new_filter);
			}
						
			props->refresh(); 

			SetWindowLong(hWndDlg,DWL_MSGRESULT,FALSE); 
			return TRUE;

		}break;		
		case PSN_SETACTIVE: 
		{				
			HWND lv = props->listview;

			ListView_DeleteAllItems(lv); 
					
			int tab = props->tab(); //p->hdr.idFrom
						
			Somdirect_cpp::folder home, *tabs[3] = 
			{
				&Somdirect_cpp::results, &props->snapshot, &home

			}, &f = *tabs[tab]; 
			
			Somdirect_cpp::section cs;
			
			if(props->dragdrop=tab==1) 
			{			
				SetWindowLong(lv,GWL_STYLE,~LVS_SINGLESEL&GetWindowLong(lv,GWL_STYLE));				
			}
			else SetWindowLong(lv,GWL_STYLE,LVS_SINGLESEL|GetWindowLong(lv,GWL_STYLE));
						
			switch(tab)
			{
			case 0: //Search Results
			{
				//populate Somdirect_cpp::results
				if(!Somdirect_results2(props->snapshot,props->home,props->name,props->filter))
				{
					Somdirect_cpp::filter g; g.init(); 

					//TODO: offer assistance (of some kind ???)

					wcscpy(g.type,L"missing"); wcscpy_s(g.path,props->home);

					Somdirect_cpp::results.clear();
					Somdirect_cpp::results.push_back(g);

					EnableWindow(lv,FALSE);
				}
				else EnableWindow(lv,TRUE);

			}break;
			case 1: //Searched Locations
			{
				EnableWindow(lv,TRUE);

				if(f.size()) break; //already have snapshot

				Somdirect_cpp::folders[props->home].clear(); //flush

				props->snapshot = Somdirect_folders(props->home); 

			}break;
			case 2: //Home Location
			{
				EnableWindow(lv,TRUE);

				Somdirect_cpp::filter g; g.init();

				wcscpy_s(g.type,props->filter); 
				wcscpy_s(g.path,props->home);

				home.push_back(g);

			}break;
			default: assert(0);
			}
					
			LVITEMW lvi = {LVIF_TEXT|LVIF_STATE};
						
			for(size_t i=0,a=0,n=f.size();i<n;i++) 
			{
				lvi.iItem = i;		

				Somdirect_cpp::filter g = f[i];

				lvi.stateMask = -1UL;
				lvi.state = g.sel?LVIS_SELECTED|LVIS_FOCUSED:0;
				
				if(tab!=2) if(!g.hide)
				{	
					lvi.pszText = a++==0?L"Default":L"";
				}
				else lvi.pszText = *g.path?L"Hidden":L"";		
						
				lvi.iSubItem = 0; 
				SendMessage(lv,LVM_INSERTITEMW,0,(LPARAM)&lvi);	
				lvi.iSubItem = 1; lvi.pszText = g.type;
				SendMessage(lv,LVM_SETITEMTEXTW,i,(LPARAM)&lvi);
				lvi.iSubItem = 2; lvi.pszText = g.path;
				SendMessage(lv,LVM_SETITEMTEXTW,i,(LPARAM)&lvi);				
			} 
			//FALL THRU
		}
		case PSN_KILLACTIVE: 
		case LVN_ITEMCHANGED: //FALLING THRU
		{
			int tab = props->tab();

			bool use = false, add = false, remove = false;

			switch(p->hdr.code)
			{
			case PSN_KILLACTIVE: break;

			case LVN_ITEMCHANGED: //FALLS THRU
			{
				NMLISTVIEW *q = (NMLISTVIEW*)p; 
				
				if(q->uChanged&LVIF_STATE)
				{
					//// Note: NMLISTVIEW reports state incorrectly ////

					assert(q->iItem!=-1); //must apply state to all items
																 
					//const int selected = LVIS_SELECTED|LVIS_FOCUSED; //ok???

					//props->snapshot[q->iItem].sel = q->uNewState&selected;

					UINT st = ListView_GetItemState(q->hdr.hwndFrom,q->iItem,-1UL);

					//assert(st==q->uNewState); //fails: some Microsoft B.S.

					switch(tab)
					{
					case 0: //Search Results
						
						if(st&LVIS_SELECTED) props->select(q->iItem); break;

					case 1: //Searched Locations
						
						props->snapshot[q->iItem].sel = st&LVIS_SELECTED; break;
					}
				}		
			}							
			case PSN_SETACTIVE: //FALLING THRU
			
				if(tab!=1) break;

				for(size_t i=0,n=props->snapshot.size();i<n;i++)
				{			
					Somdirect_cpp::filter &g = props->snapshot[i]; 

					if(!g.sel||!*g.path) continue; 

					if(i&&!g.hide) use = true; //TODO: type filter

					if(g.hide) add = true; else remove = true;
				}
			}
			EnableWindow(GetDlgItem(hWndDlg,IDC_USE),use); 
			EnableWindow(GetDlgItem(hWndDlg,IDC_ADD),add); 				
			EnableWindow(GetDlgItem(hWndDlg,IDC_REMOVE),remove); 

		}break;
		case PSN_APPLY:
		{				
			if(p->hdr.idFrom!=0) break; //managing tabs 1~3

			if(props->edits) //assuming Search Locations edited
			{
				props->edits = 0;

				Somdirect_cpp::section cs;
				Somdirect_cpp::folders[props->home] = props->snapshot;				

				if(!Somdirect_register(props->home))
				{
					assert(0); //TODO: notify the end user
				}				
			}			

		}break;	
		case PSN_QUERYCANCEL:
		
			RemoveProp(props->propsheet,"Sompaste_select");
			break;
		}

	}break;		
	case WM_CONTEXTMENU:
	{
	shortcut: //hack

		HWND lv = props->listview;

		LVHITTESTINFO ht = 
		{
			{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)}

		}; ScreenToClient(lv,&ht.pt);
		
		int tab = props->tab();

		if(shortcut)
		{
			ht.iItem = ListView_GetNextItem(lv,-1,LVNI_SELECTED);

			ht.iSubItem = 2; if(ht.iItem==-1) break; //ID_OPEN_LOCATION
		}
		else if(ListView_SubItemHitTest(lv,&ht)==-1&&tab!=1) break;
		
		bool app = ht.iItem==-1||tab==1&&ht.iItem==ListView_GetItemCount(lv)-1;

		if(app) ht.iItem = ListView_GetItemCount(lv)-1;

		bool X = tab==1&&(!app||ListView_GetSelectedCount(lv)>1);
					
		if(ht.iItem==0) //first row is hit when clicking header
		if(ChildWindowFromPoint(ListView_GetHeader(lv),ht.pt)) break;

		static HMENU menu = 
		LoadMenu(Sompaste_dll(),MAKEINTRESOURCE(IDR_FOLDER_POPUP));

		HMENU popup = GetSubMenu(menu,0); 			
		SetMenuDefaultItem(popup,ID_SELECT,0);	
				
		switch(tab)
		{
		case 0: //Search Results
		{
			int missing = IsWindowEnabled(lv)?0:MF_GRAYED;
			ModifyMenuW(menu,ID_SELECT,missing,ID_SELECT,L"&Select");		
			//ModifyMenuW(menu,ID_OPEN_LOCATION,0,ID_OPEN_LOCATION,L"&Open file location\t\\");			
			
		}break;
		case 1: case 2: //Searched Locations //Home Location
		{
			ModifyMenuW(menu,ID_SELECT,tab==1?0:MF_GRAYED,ID_SELECT,L"&Edit");
			//ModifyMenuW(menu,ID_OPEN_LOCATION,0,ID_OPEN_LOCATION,L"&Open folder location \t\\");

		}break;		
		}

		if(ht.iSubItem==0&&tab!=0)
		EnableMenuItem(menu,ID_SELECT,MF_GRAYED);			
		EnableMenuItem(menu,ID_DELETE,X?0:MF_GRAYED);		

		ClientToScreen(lv,&ht.pt);

		if(!shortcut) shortcut = 
		TrackPopupMenu(popup,TPM_RETURNCMD,ht.pt.x,ht.pt.y,0,hWndDlg,0);
				
		switch(shortcut)
		{	 
		case ID_SELECT: 
		{		
			if(tab==0) //Select
			{
				props->select(ht.iItem);

				switch(props->edits?Somprops_confirm(hWndDlg,L"Folder"):IDYES) 
				{
				case IDNO: props->edits = 0; case IDYES: break; case IDCANCEL: return 0;
				}

				PropSheet_PressButton(props->propsheet,PSBTN_OK);
			}
			else //Edit
			{
				SetFocus(lv); //docs say required				

				props->editlabel = ht.iSubItem;

				ListView_EditLabel(lv,ht.iItem);
			}

		}break;
		case ID_OPEN_LOCATION:
		{
			wchar_t text[MAX_PATH] = L"";  
			LVITEMW lvi = {LVIF_TEXT,ht.iItem,ht.iSubItem,0,0,text,MAX_PATH};
			SendMessageW(lv,LVM_GETITEMTEXTW,ht.iItem,(LPARAM)&lvi);				

			//TODO: verify directory exists
			ShellExecuteW(hWndDlg,L"explore",text,0,0,SW_SHOWNORMAL);			

		}break;
		case ID_DELETE:
		{
			props->drop(-1,ht.iItem);

		}break;		
		}

	}break;
	case WM_KEYDOWN: 
	{
		switch(wParam) //accelerators
		{
		default: if(props->tab()!=1) break;
			
			break; //TODO: shortcut = ID_SELECT?

		case VK_OEM_5: case VK_OEM_102: //slash
										   
			shortcut = ID_OPEN_LOCATION; goto shortcut;

		case VK_DELETE: shortcut = ID_DELETE; goto shortcut;		
		}

	}break;
	case WM_COMMAND:
	{			 	
		switch(LOWORD(wParam))
		{		
		case ID_SELECT:

			shortcut = ID_SELECT; goto shortcut;

		case ID_FILE_OPEN:
		{
			const wchar_t *item = 0;

			wchar_t text[MAX_PATH] = L"", file[MAX_PATH] = L"", name[MAX_PATH] = L"";   
							
			swprintf_s(text,L"Find %s Folder",wcscpy(file,props->name));		

			OPENFILENAMEW open = 
			{
				sizeof(open),GetParent(hWndDlg),0,
				0, //All Files
				0, MAX_PATH, 0, file, MAX_PATH, name, MAX_PATH, 0,
				//OFN_NOVALIDATE: the MDL browsing extension for some reason
				//causes an ugly "Path does not exist" message box to appear 
				//in Vista. No clue why; everything seems in order. The docs
				//for OFN_NOVALIDATE do not suggest that it is of any use to
				//us, however the docs for FOS_NOVALIDATE are way more clear
				text, OFN_NOVALIDATE, 0, 0, 0, 0, 0, 0, 0, 0
			};

			if(!GetOpenFileNameW(&open)) break;			

			assert(0); //unfinished
		
		}break;
		case IDC_USE: 
		{
			HWND lv = props->listview;
			props->drop(0,ListView_GetNextItem(lv,-1,LVNI_SELECTED),IDC_USE); 

		}break;		
		case IDC_ADD: case IDC_REMOVE:
		{
			HWND lv = props->listview; int next = -1;
			while(-1!=(next=ListView_GetNextItem(lv,next,LVNI_SELECTED)))
			{			
				Somdirect_cpp::filter &g = props->snapshot[next]; 

				if(*g.path) g.hide = LOWORD(wParam)==IDC_REMOVE;
			}		  

			props->refresh();

		}break;
		}

		switch(LOWORD(wParam)) //buttons
		{		
		case IDC_USE: case IDC_ADD: case IDC_REMOVE: //return focus to listview 

			SendMessage(props->propsheet,WM_NEXTDLGCTL,(WPARAM)props->listview,1);
		}

	}break;
	case WM_NCDESTROY:
	{	
	close: RemoveProp(hWndDlg,"Sompaste_select");
		
		SetWindowLongPtrW(hWndDlg,GWLP_USERDATA,0); delete props;
				
		if(Msg!=WM_NCDESTROY) DestroyWindow(hWndDlg);
	}
	}//switch

	return 0;
}

extern HWND Somdirect(SOMPASTE, HWND owner, wchar_t inout[MAX_PATH], const wchar_t *filter, const wchar_t *title, void *modeless)
{	
	if(!inout) return 0;

	if(PathIsRelativeW(inout)) return 0; //what to do?	

	SOMPASTE_LIB(Path)(inout); //ensuring canonicalized for now

	Somdirect_cpp::filter param; param.init(); //passing args onto SomdirectFolderProc
	wcscpy_s(param.path,param.path_s,inout); if(filter) wcsncpy(param.type,filter,param.type_s);

	if(!title) //windowless mode 
	{
		assert(!modeless); //modal is implied

		Somdirect_cpp::dirprops hack(0,0,param);

		if(!Somdirect_results(hack.home,hack.name,hack.filter,inout)) return 0;

		return (HWND)INVALID_HANDLE_VALUE; //success
	}

	if(!modeless) return 0; //unfinished (modal loop)
	
	INITCOMMONCONTROLSEX cc = //hack
	{sizeof(cc),ICC_LISTVIEW_CLASSES};
	static BOOL hack1 = InitCommonControlsEx(&cc); 
	
	HMENU menu = //static (apparently destroyed with window)
	LoadMenu(Sompaste_dll(),MAKEINTRESOURCE(IDR_FOLDER_MENU));
		
	const int tabs = 3;
	HWND *sheet = SOMPASTE_LIB(Propsheet)(owner,tabs,title,menu); if(!sheet) return 0;

	HWND props = CreateDialogParamW(Sompaste_dll(),MAKEINTRESOURCEW(IDD_FOLDER),sheet[tabs],SomdirectFolderProc,(LPARAM)&param);			

	if(!*title)
	{
		wchar_t text[MAX_PATH+10] = L"";
		swprintf_s(text,L"%s Folder",!filter?inout:PathFindFileNameW(inout));
		SetWindowTextW(sheet[tabs],text);
	}
		
	if(sheet[0]==props) sheet[1] = props; //Searched Locations
	if(sheet[0]==props) sheet[2] = props; //Home Location

	if(modeless) return sheet[tabs];

	//Reminder: modal loop will have to catch Sompaste_select

	assert(0); //unfinished	 
	return 0; //compile
}