	
#include "Ex.h" 
EX_TRANSLATION_UNIT

#include <vector>
#include <algorithm>

#include "som.tool.hpp" //som_LYT

extern HWND &som_tool;

//som_LYT_open is called at the top of SOM_MAP and whenever the
//map menu is opened (it is refreshed) and MapComp uses it also
static int som_LYT_open2(wchar_t *w, int i, WIN32_FIND_DATA &fd)
{
	//these will need proper error codes
	enum{ ok=0,eopen,eover,eline,ename,eparse,ecoord,ewide }; 

	som_LYT** &lyt = 
	SOM_MAP.lyt[_wtoi(fd.cFileName)]; if(lyt)
	{
		som_LYT::Place &cmp = lyt[0]->data();
		if(cmp.i==i&&!memcmp(&cmp.time,&fd.ftLastWriteTime,sizeof(FILETIME)))
		return 0;
		delete[] (void*)lyt; lyt = 0;
	}

	QWORD legacy = 0;

	//these constants are abitrary/will
	//be increased upon request
	//6: SOM::MPX::layer_6 is the limit
	//for right now
	short ii=0,jj=0;
	enum{ bufN=2056,tblN=6/*4*/ };
	BYTE buf[bufN+33]; short tbl[tblN];	

	//LYT lines look like the following
	//only the file name is nonoptional
	//%s is the drop-list string, where
	//%s is replaced with the MAP title
	//
	//	*1;10.map;%s;0.0;0*0;0*0*100*100
	//
	//or with 90 degree turn
	//	
	//	*1;10.map;%s;0.0;0*0*90;0*0*100*100
	//
	FILE *f = _wfopen(w,L"rt, ccs=UTF-8");
	if(!f) return eopen;
	struct RAII //HACK
	{ FILE *f; ~RAII(){ fclose(f); }
	}_ = {f};	
	enum{ N=512 };
	wchar_t *pp,*p,ln[N];
	for(BYTE lno=1;pp=p=fgetws(ln,N,f);lno++)
	{
		if(jj==tblN) return eover;
		tbl[jj++] = ii;

		som_LYT *ll,l = {lno};
		ll = (som_LYT*)(buf+ii);
		ii+=sizeof(l);		
		if(ii>bufN) return eover;

		for(;;) switch(*p++)
		{
		case '\0':
			if(!feof(f)) return eline;			
			//break;
		case '\r': case '\n': case ';':

			if(pp==p-1) return eparse;

			bool eol = ';'!= p[-1];
			p[-1] = '\0'; std::swap(pp,p);
			{
				wchar_t *e,*x;
				unsigned long v[4],vN = 0;
				if(x=wcschr(p,'*')) do
				{	
					if(vN==4||x==pp-2) 
					return eparse;

					if(p==x) p++;
					v[vN++] = wcstoul(p,&e,10);
					if(e!=x&&e!=pp-1)
					return eparse;

					//get value on right side of final *
					if(x) p = x+1;
					if(x) x = wcschr(p,'*');
				}while(e!=pp-1);

				if(vN) switch(vN)
				{
				case 1: //group 
					
					//limiting to 7 so it is possible
					//to convert object-type IDs into
					//0 (7 is a free layer)
					if(*v>6) return eparse;

					l.group = 0xFF&*v; break;

				case 3: //destination+turn
					
					switch(v[2])
					{
					case 0: v[2] = 0; break; //S
					case 90: v[2] = 1; break; //E
					case 180: v[2] = 2; break; //N
					case 270: v[2] = 3; break; //W
					default: return ecoord;
					}
					l.pos[2] = v[2]; //break;

				case 2: //destination

					if(v[0]>99||v[1]>99) return ecoord;

					l.pos[0] = 0xFF&v[0]; 
					l.pos[1] = 0xFF&v[1]; break;

				case 4: //source

					if(v[0]>99||v[1]>99
					||v[2]>100||v[3]>100
					||v[0]>=v[2]||v[1]>=v[3]) return ecoord;
					else for(int i=0;i<4;i++)
					l.box[i] = 0xFF&v[i]; break;
				}
				else if(x=wcschr(p,'.')) //0.0 or 00.map
				{					
					float elev = wcstod(p,&e);
					if(e!=pp-1)
					{
						if(!wcsicmp(x,L".map")
						&&2==x-p&&elev<64&&elev>=0
						&&!l.map[0])
						sprintf(l.map,"%02d",(int)elev);
						else goto map2;
					}
					else l.elev = elev;
				}
				else map2:
				{
					if(pp-p==3&&isdigit(p[0])&&isdigit(p[1]))
					{
						if(_wtoi(p)<64&&!l.map[0]) //e.g. 00
						{
							l.map[0] = p[0]; l.map[1] = p[1];
						}
						else return eparse;
					}
					else
					{
						int len0 = pp-p;
						if(len0>127) return ename;

						//convert 00 file name to longer form?
						if(l.map[1]&&!*l.w2
						&&(ii+=6*sizeof(WCHAR))<=bufN)
						{
							swprintf(ll->w2,L"%hs.map",l.map);
							*l.w2 = *ll->w2;
							l.map[0] = 7; l.map[1] = 0;
						}
						ii+=len0*sizeof(WCHAR);
						if(ii>=bufN) return eover;

						wmemcpy(ll->w2+l.map[0],p,len0);

						if(!l.map[0]) //file name?
						{
							ii--; //w2
							*l.w2 = *ll->w2;
							l.map[0] = 0xFF&len0;
							for(int k=0;k<len0;k++)
							if(ll->w2[k]>127) return ewide;
						}
						else if(!l.map[1]) //dropdown name?
						{
							l.map[1] = 0xFF&len0;
						}
						else return eparse;
					}
				}
			}
			p = pp; 			
			if(eol) goto eol;
		}eol:

		if(!l.box[2]) l.box[2] = 100-l.pos[0];
		if(!l.box[3]) l.box[3] = 100-l.pos[1];
		
		if(l.pos[0]+int(l.box[2]-l.box[0])>100
		 ||l.pos[1]+int(l.box[3]-l.box[1])>100)
		return ecoord;

		if(!*l.map&&!*l.w2)
		return eparse;

		*ll = l;
		
		int xx = -1; if(*ll->w2)
		{
			if(!ll->w2[2]||!wcsicmp(ll->w2+2,L".map"))
			{
				xx = wcstol(ll->w2,&p,10);
				if(p-ll->w2!=2) xx = -1;
			}
		}
		else xx = atoi(ll->map);

		if(xx>=0&&xx<64) legacy|=1ULL<<xx;
	}
	//fclose(f); //HACK: RAII is closing in error case

	//empty? whitespace? BOM?
	if(!jj) return eparse; 

	size_t head = sizeof(void*)*(jj+1);
	head+=sizeof(som_LYT::Place);
	char *mem = new char[head+ii];	
	(void*&)lyt = mem; 
	char *buf2 = mem+head; 
	for(int j=0;j<jj;j++) (void*&)lyt[j] = buf2+tbl[j];	
	(void*&)lyt[jj] = 0;
	lyt[0]->data().i = i; 
	lyt[0]->data().time = fd.ftLastWriteTime; 	
	lyt[0]->data().legacy = legacy;
	memcpy(buf2,buf,ii); return 0;	
}
extern void som_LYT_open(WCHAR mapcomp[2])
{
	if(!SOM_MAP.lyt) 
	SOM_MAP.lyt = new som_LYT**[64]();
		
	WIN32_FIND_DATA fd; 
	wchar_t w[MAX_PATH+8]; QWORD mask = ~0ULL;

	//WARNING: som_tool_FindFirstFileA fails
	//to implement this correctly/needs work
	int iN = wcscmp(EX::user(0),EX::cd())?2:1; //HACK
	for(int map,i=0;i<iN;i++)	
	{
		int cat = swprintf(w,L"%s\\map\\",EX::data(i));		
		wmemcpy(w+cat,L"??.lyt",7);
		if(mapcomp) w[cat+0] = mapcomp[0];
		if(mapcomp) w[cat+1] = mapcomp[1];
		HANDLE ff = FindFirstFileW(w,&fd);
		if(ff!=INVALID_HANDLE_VALUE) do
		{
			WCHAR *fn = fd.cFileName;
			if((map=_wtoi(fn))<64
			&&isdigit(w[cat+0]=fn[0])
			&&isdigit(w[cat+1]=fn[1])
			&&'.'==fn[2]
			&&mask&1ULL<<map)
			{
				mask&=~(1ULL<<map);
				if(int err=som_LYT_open2(w,i,fd))
				{
					EX::messagebox(MB_OK, //RUSH SOLUTION
					"LYT%03d: Error parsing layer table file:\n\n%ls",err,w);
				}
			}

		}while(FindNextFile(ff,&fd)); FindClose(ff);
	}
	for(int i=0;i<64;i++) if(SOM_MAP.lyt[i]&&mask&1ULL<<i)
	{
		delete[] (void*)SOM_MAP.lyt[i]; SOM_MAP.lyt[i] = 0;
	}
}

extern int som_LYT_base(int ll)
{
	if(!SOM_MAP.lyt) return ll;

	//NOTE: this workflow is limited to standard two-digit
	//MAP files in DATA/MAP. it would be nice if only the
	//base layer needs to be one of these files but that
	//can't be achieved without a major overhaul/audit
	if(ll<0||ll>63) return -1;

	int j = -1;
	QWORD l = 1ULL<<ll;	
	for(int i=0;i<64;i++) 
	if(SOM_MAP.lyt[i]&&l&SOM_MAP.lyt[i][0]->data().legacy)
	{
		if(j!=-1){ j = -1; break; }else j = i;
	}
	return j;
}

static void som_LYT_maybe_update_label(int base, int layer)
{
	//NICE: keep layer menu up-to-date?
	HWND cb = GetDlgItem(som_tool,1133);
	int top = ComboBox_GetItemData(cb,0);		
	if(top==-1) top = layer; if(base==top)
	{	
		char buf[64]; sprintf(buf,"[%02d] %s",
		ComboBox_GetItemData(cb,ComboBox_GetCurSel(cb)),
		SOM_MAP_4921ac.title);
		extern bool som_LYT_label(HWND,const char*);
		if(!som_LYT_label(cb,buf))
		SetWindowTextW(cb,EX::need_unicode_equivalent(932,buf));
	}
}
static void som_LYT_compose2(WCHAR *p, som_LYT &ln, bool group)
{
	if(group) p+=swprintf(p,L"*%d;",(int)ln.group);	
	bool pos = ln.pos[0]||ln.pos[1]||ln.pos[2];
	bool box = ln.box[0]||ln.box[1]||ln.box[2]!=100||ln.box[3]!=100;	
	WCHAR *pp = p;
	if(!*ln.w2) 	
	p+=swprintf(p,L"%hs",ln.map);
	else p+=swprintf(p,L"%s",ln.w2);
	if(pos||box||ln.elev)	
	if(p-pp==2&&isdigit(p[-1])&&isdigit(p[-2])) //e.g 00
	p = wmemcpy(p,L".map",4)+4;
	if(*ln.w2&&ln.map[1]) 
	p+=swprintf(p,L";%s",ln.w2+ln.map[0]);	
	if(ln.elev)
	p+=swprintf(p,(int)ln.elev==ln.elev?L";%g.0":L";%g",ln.elev);
	if(pos||box) 
	p+=swprintf(p,L";%d*%d*%d;%d*%d*%d*%d",
	(int)ln.pos[0],(int)ln.pos[1],(int)ln.pos[2],
	(int)ln.box[0],(int)ln.box[1],(int)ln.box[2],(int)ln.box[3]);	
	//wmemcpy(p,L"\r\n",3); //fputs \r\r\n :(
	wmemcpy(p,L"\n",2);
}
extern void som_LYT_compose(HWND hw)
{
	HWND lb = GetDlgItem(hw,1239);

	int sel[2],selN = ListBox_GetSelItems(lb,2,sel);
	
	int i,j; if(1==selN) //decomposing?
	{
		j = som_LYT_base(sel[0]);
		if(j==-1) goto programmer_error;
	}
	else if(2==selN)
	{
		j = sel[0];
	}
	else programmer_error: //MessageBox
	{
		assert(0); MessageBeep(-1); return;
	}
	
	const int base = j;
	som_LYT **lyt = SOM_MAP.lyt[j];	
	if(!lyt&&1==selN) goto programmer_error;

	//WARNING: som_tool_FindFirstFileA fails
	//to implement this correctly/needs work
	char a[] = SOMEX_(B)"\\data\\map\\00.lyt";	
	a[sizeof(a)-7]+=j/10;
	a[sizeof(a)-6]+=j%10;
	const wchar_t *w = SOM::Tool::project(a);

	if(1==selN) //decomposing?
	{
		if(!lyt[1]&&!_wremove(w)) //single line?
		{
			delete[] (void*)SOM_MAP.lyt[j]; SOM_MAP.lyt[j] = 0;
			refresh:
			windowsex_enable<1075>(hw,0);
			//nice: refresh menu and reload if [de]composing
			//the currently open map, but after closing menu
			extern char SOM_MAP_layer(WCHAR[]=0,DWORD=0);	
			int layer = SOM_MAP_layer();
			if(sel[2==selN]==layer) //delay changing menu
			{
				//HACK: som_tool_subclassproc checks this on
				//IDCANCEL and converts it to IDOK
				SetWindowLong(hw,DWL_USER,~layer); 
			}								  
			else som_LYT_maybe_update_label(base,layer);
			return;
		}
	}
	
	FILE * f = _wfopen(w,L"wt, ccs=UTF-8");
	if(!f) MessageBeep(-1); //MessageBox		
	if(!f) return;	
	rewind(f); //(Microsoft's) UTF-8 BOM
		
	//max expected size + some extra
	wchar_t buf[3+128+128+6+8+14 +33];
	
	//BEING VERY CAREFUL TO PRESERVE
	//GROUPS AND KEEP MENUS IN ORDER

	som_LYT ins = {}; if(selN==2)
	{
		ins.box[2] = ins.box[3] = 100;
		swprintf(buf,L"%02d",sel[1]);
		ins.map[0] = 0xFF&buf[0]; 
		ins.map[1] = 0xFF&buf[1];
		//finding a free group
		BYTE groups[8]; i = 0;
		if(lyt) for(som_LYT **p=lyt;*p;p++)
		{
			assert(i<sizeof(groups));
			if((*p)->group<sizeof(groups))
			groups[i++] = (*p)->group;
		}
		std::sort(groups,groups+i);
		ins.group = 1; 
		for(j=0;j<i;j++)
		{
			if(groups[j]==ins.group) 
			ins.group = groups[j]+1;
		}
	}

	//WORTHWHILE? use group notation unless
	//groups are sequential and on the back
	//TODO? might be better to always write 
	//groups in case the LYT file is edited
	//by hand without knowledge of grouping
	bool grouping = false; if(lyt)
	{
		//tells if sel is not the last item
		grouping = (1ULL<<sel[selN-1]+1)-1<lyt[0]->data().legacy;
		//grouping defaults to line numbers
		if(1==lyt[0]->group)
		{
			for(som_LYT **p=lyt;*p;p++)
			if(p[1]&&p[0]->group!=p[1]->group+1)		
			{				
				//this last test defers to the last item test if
				//removing layers from the back
				if(selN==2||p[2]) grouping = true; 
			}
		}
		else grouping = true;
	}

	if(lyt) while(*lyt)
	{
		som_LYT &ln = **lyt; lyt++;

		i = ln.legacy();

		if(1==selN) //decomposing?
		{
			if(*sel==i) continue;
		}
		else if(i>sel[1]) //inserting?
		{
			if(ins.group)
			{
				som_LYT_compose2(buf,ins,grouping);		
				ins.group = 0; //mark inserted
				lyt--; goto ins;
			}
		}

		som_LYT_compose2(buf,ln,grouping);		
		ins:fputws(buf,f);
	}
	if(2==selN&&ins.group) //appending?
	{
		som_LYT_compose2(buf,ins,grouping);		
		fputws(buf,f);
	}

	fclose(f);

	som_LYT_open(0); //refresh SOM_MAP.lyt

	//can't use the compose button after decomposing
	//without first selecting a base map
	if(selN==1) goto refresh;

	assert(selN==2);
	//compose+open?
	//DICEY: som_LYT_label is using the label text
	//when the file inevitably fails to open
	if(0xFF==ListBox_GetItemData(lb,sel[1]))
	{
		ListBox_SetCurSel(lb,sel[1]);
		SendMessage(hw,WM_COMMAND,IDOK,0);
		/*instead, the map will be marked modifed
		//to force the Save prompt...
		////force save just so it's airtight? or
		////let the LYT file be out of sync?
		//PostMessage(som_tool,WM_COMMAND,57603,0);
		*/
		return;
	}
	goto refresh;
}
const wchar_t *som_LYT_title(int i, const char *C=0)
{
	char a[32];
	char b[] = SOMEX_(B)"\\data\\map\\00.map";	
	b[sizeof(b)-7]+=i/10;
	b[sizeof(b)-6]+=i%10;
	const wchar_t *w = SOM::Tool::project(b);
	FILE *f = _wfopen(w,L"rb");
	if(!f) goto err;
	for(i=0;i<2;i++)
	{
		if(!fgets(a,32,f)) err:
		{
			//if C assuming som_LYT_compose is
			//on the compose+open path that is
			//currently saving after open, but
			//not before the label text is set
			strcpy(a,C?C:"MAP I/O ERROR 0000");
			if(C) *SOM_MAP_4921ac.modified = 1;
			break;
		}
	}
	if(f) fclose(f);	
	if(char*n=strchr(a,'\n'))
	{
		if(n[-1]=='\r') n--; *n = '\0';
	}	
	return EX::need_unicode_equivalent(932,a);
}

extern bool som_LYT_label(HWND A, const char *C)
{
	const wchar_t *_map = Sompaste->get(L".MAP");	
	assert(isdigit(_map[0])&&isdigit(_map[1])&&!_map[2]);

	if(C) //old behavior from som_tool_SetWindowTextA
	{
		//historically SOM_MAP always opened to 00.map		
		(char&)C[1] = _map[0]; (char&)C[2] = _map[1];
	}

	//REMINDER: DROPLIST CANNOT CONTROL ITS BACKGROUND

	HWND ch = GetDlgItem(A,1001); //GetFirstChild(A);	
	if(!ch) return false; //static label?
	SetWindowStyle(ch,ES_READONLY|ES_NOHIDESEL,ES_READONLY);
	Edit_SetReadOnly(ch,1);

	int xx = _wtoi(_map); 
	if(xx<0||xx>63){ assert(0); return false; }
	
	int i,j; if(!C) //cancel?
	{		
		//stupid macro
		//int test = ComboBox_FindItemData(A,-1,xx);
		for(i=0;-1!=(j=ComboBox_GetItemData(A,i));i++) if(j==xx)
		{
			ComboBox_SetCurSel(A,i); return true; 
		}
		return false; //assert(0);
	}
	ComboBox_ResetContent(A);

	j = som_LYT_base(xx);	
	int base = j!=-1?j:xx;
	som_LYT **lyt = SOM_MAP.lyt[base];
	if(!lyt) return false;

	wchar_t buf[128];
	const wchar_t *title; if(xx!=base)
	{
		swprintf(buf,L"[%02d] %s",base,som_LYT_title(base));
		title = buf;
	}
	else title = EX::need_unicode_equivalent(932,C);

	for(i=base;;lyt++,title=buf)
	{		
		j = ComboBox_AddString(A,title);
		ComboBox_SetItemData(A,j,i);
		if(i==xx) ComboBox_SetCurSel(A,j);
		if(!*lyt) break;

		som_LYT &ln = **lyt; 		
		i = ln.legacy();
		if(*ln.w2&&ln.map[1])
		{
			//PARANOIA
			//allowing one unformatted %s or %%
			title = 0;
			WCHAR *p,*format = ln.w2+ln.map[0];
			for(p=format;p=wcschr(p,'%');p++) switch(p[1])
			{
			case '%': p++; continue;

			case 's': //%s
				
				if(title)
				{
					default: goto paranoia;
				}
				title = som_LYT_title(i);
			}
			swprintf(buf,format,title);
		}
		else paranoia: 
		{
			swprintf(buf,L"[%02d] %s",i,som_LYT_title(i,xx==i?C+5:0));
		}
	}

	return true;
}
extern void som_LYT_tooltip(NMTTDISPINFO &di)
{
	extern std::vector<WCHAR> som_tool_wector; 
	auto &w = som_tool_wector; w.clear();
	
	HWND cb = GetDlgItem(som_tool,1133);
	//YIKES: prevents the text from being displayed :(
	//HWND tb = di.hdr.hwndFrom;
	HWND tb = (HWND)di.hdr.idFrom; //what is hwndFrom?
	//multiline?
	SendMessage(di.hdr.hwndFrom,TTM_SETMAXTIPWIDTH,0,500);
	int pos = SendMessage(tb,TBM_GETPOS,0,0);
	pos-=SendMessage(tb,TBM_GETRANGEMIN,0,0);
	int base = ComboBox_GetItemData(cb,0);
	som_LYT **lyt = 0;
	if(base>=0&&base<64) lyt = SOM_MAP.lyt[base];	

	//numbers are shifted by TBM_GETRANGEMIN 
	extern wchar_t som_tool_text[MAX_PATH];		
	w.assign(som_tool_text,som_tool_text+swprintf(som_tool_text,L"%d\n",pos));

	//assuming 2-digit codes :(	
	int len,i,iN = ComboBox_GetCount(cb);
	if(!iN&&pos==0) 
	{
		len = GetWindowText(cb,som_tool_text,MAX_PATH);
		i = 0; goto demo;
	}
	for(i=0;i<iN;i++)
	{
		if(i==0&&pos==0)
		goto base;
		int l = ComboBox_GetItemData(cb,i);		
		for(som_LYT**p=lyt;*p;p++)
		{
			if(pos==(*p)->group)
			if(l==(*p)->legacy()) //not pretty
			{	
				base:
				len = ComboBox_GetLBText(cb,i,som_tool_text);		
				demo:
				w.insert(w.end(),som_tool_text,som_tool_text+len);
				w.push_back('\n');
				break;
			}
		}
	}	
	w.push_back('\0'); di.lpszText = &w[0]; 
}


