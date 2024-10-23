
#include "Sompaste.pch.h"

#include <map>
#include <string>
#include <vector>
#include <deque> //longnames

#include "../Somplayer/Somthread.h"

namespace Somproject_cpp
{	
	//NEW: allow for longish filenames
	std::deque<std::wstring> longnames(1);

	struct location	
	{
		enum{ meta=2 }; //???

		/*Assuming once set won't change*/
		wchar_t directory[MAX_PATH],_[meta]; //???
				
		location()
		{
			directory[0] = '\0';

			for(size_t i=0;i<meta;i++) directory[MAX_PATH+i] = 0; 
		}

		wchar_t directory_length() //optimization
		{
			if(directory[directory[MAX_PATH]]) 
			directory[MAX_PATH]+=wcslen(directory+directory[MAX_PATH]);				
			assert(directory[MAX_PATH]<MAX_PATH-1);
			return directory[MAX_PATH];
		}
	};

	enum{ ITEM=0, MY, OBJ, ENEMY, NPC, MAP, SFX, selectors };

	struct profile
	{
		int locator, selector;

		//10.3 filenames are probably best so UTF8 fits into profiles
		static const size_t title_s = 10+4+1; wchar_t title[title_s];

		inline const wchar_t *longname()const //NEW
		{
			assert(title[1]||*title>=256);
			return title[1]?title:longnames[*title-256].c_str(); 
		}
		inline bool operator<(const profile &than)const //std::less
		{
			return selector==than.selector?
			wcsicmp(longname(),than.longname())<0:selector<than.selector;
		}
	};	
	typedef std::pair<std::wstring,int> setfile; //locator
	
	//typedef long long unsigned int setmask; //64
	//static const size_t setmask_s = sizeof(setmask)*CHAR_BIT; //limits.h
	//static int setmask_compile[setmask_s>=64]; 
	//NEW: 
	struct setmask : private std::wstring //sparse bitset
	{
		inline bool meta()const
		{
			return get(0);
		}
		inline bool get(wchar_t setbit)const
		{
			return std::wstring::find(setbit)!=std::wstring::npos;
		}				
		inline void set(wchar_t setbit)
		{
			if(!get(setbit)) std::wstring::push_back(setbit);
		}
		inline setmask(int deprecated)
		{
			assert(deprecated==0); //assuming 0 initialization
		}
		inline void operator=(wchar_t setbit)
		{
			std::wstring::clear(); set(setbit);
		}
		inline void operator|=(const setmask &setbits)
		{
			for(size_t i=0;i<setbits.size();i++) set(setbits[i]);
		}
		//bitwise operators
		template<int _=0> struct bool_result
		{
			bool out; inline operator bool(){ return out; }

			inline operator int(){ int compile[-1]; }
		};
		inline bool_result<> operator&(const setmask &setbits)
		{
			bool_result<> r = {false};
			for(size_t i=0;i<setbits.size();i++) 
			if(get(setbits[i])){ r.out = true; break; }
			return r;
		}
		//superset operators
		inline bool operator>=(const setmask &subset)const
		{
			for(size_t i=0;i<subset.size();i++) 
			if(!get(subset[i])) return false; return true;
		}
		inline bool operator<(const setmask &subset)const
		{
			return !operator>=(subset);
		}
		inline bool operator!()const{ return empty(); }
	};
	typedef std::map<std::wstring,size_t> bitmap;
	typedef std::pair<std::wstring,size_t> bit;

	typedef std::vector<const profile*> matchset;
	typedef std::map<std::wstring,matchset> filter;		

	struct data //per project
	{
		filter::value_type *hit;

		std::vector<location> net;

		std::map<profile,setmask> set;

		std::multimap<profile*,int> shadow;

		std::map<std::wstring,matchset> filters;
		
		bool indexed; bitmap map;

		data()
		{
			hit = 0; indexed = false; 

			map[L"meta"] = 0; //2021: reserved for new my/arm profiles
		} 

		void index(location in[], size_t in_s, const wchar_t *setlist)
		{
			while(iswspace(*setlist)) setlist++; if(!*setlist) setlist = 0; 

			if(indexed){ this->~data(); new (this) data(); }indexed = true;

			//MSVC'5 shows "???" in STL containers???
			struct rule{ setmask mask; wchar_t op; }; 

			std::vector<std::wstring> dottedruleset; //.

			std::vector<rule> ruleset; if(setlist)
			{
				bit rulebit; //optimization

				for(const wchar_t *p=setlist,*q;*p;p+=*p?1:0)
				if(*p=='+'||*p=='-'||*p=='|')
				while(*p&&*p!='\n')
				{	
					rule newrule = {0,*p++};

					if(q=wcspbrk(p,L" /\t\r\n")) //!
					rulebit.first.assign(p,q-p); else rulebit.first = p; 										
					p+=rulebit.first.size();
					
					if(rulebit.first.empty()) continue;
					if(rulebit.first.find(L'.')!=std::wstring::npos) 
					{
						//dotted filename rules cannot stand alone and
						//cannot be first and cannot be part of unions

						if(newrule.op!=' '
						&&newrule.op!='|') continue; //not supporting

						assert(!newrule.mask); //==0
						ruleset.push_back(newrule);
						dottedruleset.resize(ruleset.size());
						dottedruleset.back() = rulebit.first; continue;						
					}
										
					rulebit.second = map.size();					
					//newrule.mask = 1ULL<<map.insert(rulebit).first->second;
					newrule.mask = map.insert(rulebit).first->second; //NEW

					if(newrule.op=='/') //union
					{
						if(!!ruleset.back().mask) //undotted?
						{
							ruleset.back().mask|=newrule.mask;
						}
						else continue; //not supporting
					}
					else ruleset.push_back(newrule);
				}
				//NEW: allow to knockout lines
				else if(*p==';'||*p=='#') while(*p&&*p!='\n') p++;
				else break; //malformed
			}

			//2021: treat invalid/comment value as-if it was unset 
			//bool meta_only = !setlist;
			bool meta_only = ruleset.empty();

			if(in_s) net.push_back(in[0]); //!
			
			for(size_t i=0,j=1;i<net.size();i++) 
			{					
				size_t added = 0;
				size_t cat = net[i].directory_length();
				if(cat>MAX_PATH-3) continue; //paranoia

				WIN32_FIND_DATAW data; 
				wcscpy(net[i].directory+cat,L"\\*"); 
				HANDLE find = FindFirstFileW(net[i].directory,&data);
				net[i].directory[cat] = '\0';

				size_t reset = setfiles.size(); //hack...

				wchar_t *ext; 
				if(find!=INVALID_HANDLE_VALUE) do		
				if(data.cFileName[0]!='.' //., .., and .hidden
				&&~data.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN)
				if(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
				{
					net.push_back(net[i]);
					swprintf(net.back().directory+cat,L"\\%ls",data.cFileName);
				}
				else if((ext=wcsrchr(data.cFileName,'.'))&&ext++) //!
				{
					if(meta_only) //REMINDER: might want to build index (???)
					{				
						if(!wcsicmp(ext,L"prf")||!wcsicmp(ext,L"prt"))
						{
							if(!data.nFileSizeHigh) 
							add(i,data.cFileName,data.nFileSizeLow);
							added++;
						}
						else if(!wcsicmp(ext,L"set")) //meta/save for filtering
						{
							setfiles.push_back(std::make_pair(data.cFileName,i));

							//2021: need to gather data on "meta" set for exclusion
							for(wchar_t*pp=data.cFileName,*p=pp;p=wcsstr(p,L"meta");p+=4)							
							if((p[4]==' '||p[4]=='.')&&pp==p||p[-1]==' ')
							{
								addsetfile(setfiles.back()); break;
							}
						}
					}
					else if(!wcsicmp(ext,L"set")) //might want to do last
					{
						//save for filtering
						setfiles.push_back(std::make_pair(data.cFileName,i));
						added+=
						addsetfile(setfiles.back()); //shared implementation
					}
					
				}while(FindNextFileW(find,&data)); FindClose(find);

				if(!added) //memory optimization
				{
					setfiles.resize(reset); //hack: deallocates std::wstring

					if(i+1<net.size()) net[i] = net.back(); net.pop_back(); i--; 
				} 

				if(i==net.size()-1&&j<in_s) net.push_back(in[j++]);
			}

			//if(!setlist) return; ////////// pruning /////////////
			
			for(auto it=set.begin();it!=set.end();)
			{
				//2021: automatically reject meta set
				//NOTE: may want to expose meta in the future
				//but it shouldn't be mixed with the regular profiles
				bool keep = !it->second.meta();

				if(keep&&!meta_only)
				for(size_t i=0,top=0;i<ruleset.size();)
				{
					if(ruleset[i].op=='+'||ruleset[i].op=='-') top = i;

					bool match = ruleset[top].mask&it->second;

					size_t j = top; 
					while(++j<ruleset.size()&&ruleset[j].op==' ')
					{
						if(!(ruleset[j].mask&it->second))
						{
							if(match&&!ruleset[j].mask) //==0)							
							if(!wcsicmp(it->first.longname(),dottedruleset[j].c_str()))
							{
								continue; //matching literal file name
							}
							match = false;					
						}
					}
					if(i==top) i = j;

					if(i<ruleset.size()&&ruleset[i].op=='|') 
					{
						do if(!(ruleset[i].mask&it->second))
						{
							if(match&&!ruleset[i].mask) //==0)							
							if(!wcsicmp(it->first.longname(),dottedruleset[i].c_str()))
							{
								continue; //matching literal file name
							}
							match = false;					

						}while(++i<ruleset.size()&&ruleset[i].op==' ');
					}
					else if(i!=j) //unexpected behavior
					{
						assert(0); keep = false; break;
					}

					if(match) keep = ruleset[top].op=='+';
				}

				if(!keep)
				{	
					//shadow.erase(&it->first);
					shadow.erase(const_cast<profile*>(&it->first));	
					set.erase(it++);				
				}
				else it++;
			}
		}
		size_t setbit(const wchar_t *set, setmask *setmask)
		{
			bitmap::iterator it = map.find(set); if(it==map.end()) return wcslen(set);

			//*setmask|=1ULL<<it->second; return it->first.size();
			setmask->set(it->second); return it->first.size(); //NEW
		}
		bool add(int locator, const wchar_t *title, size_t filesize, setmask *setbits=0, bool insert=true)
		{
			profile prf; prf.locator = locator;			

			if(filesize<selectors)
			{
				prf.selector = filesize;
			}
			else switch(filesize)
			{																	   
			//+97: with extended balloon tip
			case 88: case 88+97: prf.selector = ITEM; break;
			case 40: case 40+97: prf.selector = MY; break;
			case 108: case 108+97: prf.selector = OBJ; break;

			//PRT already includes balloon tip
			case 228: prf.selector = MAP; break;

			default: 
				
				//2024: this is screening SFX profiles
				if(filesize%4==3)
				{
					prf.selector = SFX;
				}
				else
				{				
					assert(filesize>=384);
				
					if(filesize<384) return false; //reserved

					//NPCs go up to 416+97=513
					prf.selector = filesize>=564?ENEMY:NPC; 
				}
			}

			prf.title[prf.title_s-1] = '\0'; 
			wcsncpy(prf.title,title,prf.title_s); 
			if(prf.title[prf.title_s-1]!='\0')
			{
				prf.title[0] = 256+longnames.size()-1;
				prf.title[1] = 0; longnames.back() = title;
				/*static bool enough_already = false;	
				if(!enough_already)
				{
					wchar_t msg[MAX_PATH*2] = L"";
					swprintf_s(msg,L"Skipping profile. Title is more than 10 Unicode code points long:"
					L"         \n\n %ls\\%ls\n\n"
					L"Press CANCEL to disable this message box.",net[locator].directory,title);
					if(MessageBoxW(0,msg,L"Sompaste.dll",MB_OKCANCEL|MB_ICONERROR)==IDCANCEL)
					enough_already = true;	
				}
				return;*/
			}

			typedef std::map<profile,setmask>::iterator set_it;

			std::pair<set_it,bool> ins;
			
			if(!insert)
			{
				ins.second = false;
				ins.first = set.find(prf);
				if(ins.first==set.end()) return false;
			}
			else ins = set.insert(std::make_pair(prf,0));

			if(setbits) ins.first->second|=*setbits;

			if(ins.first->first.locator!=locator)
			{
				std::pair<profile*,int> kv
				(const_cast<profile*>(&ins.first->first),locator);
				shadow.insert(kv);
			}
			else if(longnames.size()-1==ins.first->first.title[0]-256)
			{
				//this longname is being kept, so cue up a new temporary
				if(!ins.first->first.title[1]) longnames.push_back(L"");
			}

			return ins.second;
		}	
								
		typedef std::pair<const profile*,int> id;

		typedef std::multimap<profile*,int>::iterator it;

		id select(const wchar_t *query, const wchar_t *match)
		{
			if(!wcsnicmp(query,L"data",4)&&(query+=5)) //legacy
			{
				if(query[-1]!='/'&&query[-1]!='\\') return id(/*0,0*/);
			}

			int selector = 0;

			switch(*query)
			{
			case '*': //broadest possible count

				return id(nullptr,query[1]?0:set.size());

			case 'i': case 'I': selector = ITEM; 
				
				if(!wcsnicmp(query,L"item",4)&&(query+=5)) break;

			case 'm': case 'M': selector = MY; 				
				
				if((query[1]=='y'||query[1]=='Y')&&(query+=3)) break;

			/*case 'm': case 'M':*/ selector = MAP;	
				
				if(!wcsnicmp(query,L"map",3)&&(query+=4)) break;

			case 'o': case 'O': selector = OBJ;

				if(!wcsnicmp(query,L"obj",3)&&(query+=4)) break;

			case 'e': case 'E':	selector = ENEMY;

				if(!wcsnicmp(query,L"enemy",5)&&(query+=6)) break;

			case 'n': case 'N': selector = NPC;

				if(!wcsnicmp(query,L"npc",3)&&(query+=4)) break;

			default: return id(/*0,0*/);
			}

			if(query[-1]!='/'&&query[-1]!='\\') return id(/*0,0*/);

			if(*query=='p'||*query=='P') //legacy
			{
				if(!wcsnicmp(query,L"prof",4)&&(query+=5))
				{
					if(query[-1]!='/'&&query[-1]!='\\') return id(/*0,0*/);
				}
				else if(!wcsnicmp(query,L"parts",5)&&(query+=6))
				{
					if(query[-1]!='/'&&query[-1]!='\\') return id(/*0,0*/);
				}
			}

			int enumerator = *query;

			if(query[+1]!='\0'
			 &&query[+1]!='/'&&query[+1]!='\\')
			{
				wchar_t *e = 0;	
				enumerator = wcstol(query,&e,10)+'0';

				switch(e==query?0:*e)
				{
				case '.': e++; //legacy

					if(!wcsnicmp(e,L"prf",3))
					{
						if(selector==MAP) return id(/*0,0*/);
					}
					else if(!wcsnicmp(e,L"prt",3))
					{
						if(selector!=MAP) return id(/*0,0*/);
					}
					else return id(/*0,0*/); e+=3;

					if(*e&&*e!='/'&&*e!='\\') return id(/*0,0*/);

				case '/': case '\\': break;

				default: return id(/*0,0*/);
				}

				query+=e-query;
			}
			else //single wchar_t mode
			{
				query++;
				
				if(enumerator<'0')
				{
					if(enumerator!='*') return id(/*0,0*/);
				}				
			}
			
			matchset *results = 0;

			if(!match) match = L"*";
						
			if(!hit||hit->first!=match
			||hit->second.empty() //paranoia??
			||hit->second[0]->selector!=selector)
			{
				results = miss(selector,match);
			}
			else results = &hit->second;
			
			if(enumerator=='*') //counting
			{
				return id(nullptr,results->size());
			}
			else enumerator-='0'; //selecting

			if(enumerator>=results->size()) return id(/*0,0*/);				

			const profile *out = results->at(enumerator);

			if(*query=='/'||*query=='\\')
			{
				wchar_t *e = 0;	
				enumerator = wcstol(query+1,&e,10); 

				if(e==query+1||*e) return id(/*0,0*/);

				it it = shadow.find((profile*)out); 
				
				while(enumerator&&it!=shadow.end()&&it->first==out)
				{
					enumerator--; it++; //map iteration is quite limited 
				}
				
				if(enumerator==0&&it!=shadow.end()) return *it;
			}
			else return id(out,out->locator);

			return id(/*0,0*/);
		}

		//so named because it holds . filters
		std::pair<std::wstring,matchset> dot;

		matchset *miss(int selector, const wchar_t *match)
		{				
			assert(match&&*match);

			matchset *empty = &dot.second;

			if(!dot.second.empty())			
			if(dot.first!=match||dot.second[0]->selector!=selector)
			{
				dot.first.clear(); dot.second.clear(); 
			}										  
			else return &dot.second; //singleton

			assert(empty->empty());

				dot.first = match; //REPURPOSING (2021)
			//filter::iterator fit = filters.find(match);
			filter::iterator fit = filters.find(dot.first);
				dot.first.clear(); //REPURPOSING (2021)

			if(fit!=filters.end()
			&&!fit->second.empty()
			&&fit->second[0]->selector==selector)
			{
				hit = &*fit; return &hit->second;
			}
			else hit = 0; addsets(match); //!
					
			size_t dotted = 0; dot.first = match;
						
			for(size_t i=0;match[i];i++) switch(match[i])
			{
			case ' ': case '\t': case '\r': case '\n': dot.first[i] = '\0'; break;

			case '.': dotted = i; default: /*dot.first[i] = match[i];*/ break;
			}

			setmask setbits = 0;
						
			dot.first.push_back('\0'); 

			const wchar_t *cdata = dot.first.data();

			for(size_t i=0,j;i<dot.first.size();i++) if(cdata[i])
			{
				if(i==dotted&&dotted)
				{
					j = wcslen(cdata+i); i+=j;

					continue; //longnames
					//if(j<profile::title_s) continue; return empty; 
				}
								
				if(j=setbit(cdata+i,&setbits)) i+=j; else return empty;				
			}

			//\0\1: empty short name workaround
			profile prf = {0,selector,L"\0\1"};

			std::map<profile,setmask>::iterator it;

			if(dotted) //a literal file name
			{	
				prf.title[prf.title_s-1] = '\0';
				wcsncpy(prf.title,cdata+dotted,prf.title_s);
				if(prf.title[prf.title_s-1]!='\0')
				{					
					prf.title[0] = 256+longnames.size()-1;
					prf.title[1] = 0; longnames.back() = cdata+dotted;
				}  
				it = set.find(prf); if(it==set.end()) return empty;

				//if(setbits&&(it->second&setbits)!=setbits) return empty;
				if(!!setbits&&it->second<setbits) return empty; //NEW

				dot.first = match;
				dot.second.push_back(&it->first); return &dot.second;
			}
			else it = set.lower_bound(prf);

			if(it==set.end()||it->first.selector!=selector) return empty;
			
			if(fit!=filters.end()) hit = &*fit; else hit =
			&*filters.insert(std::make_pair(match,matchset())).first; 			
			for(hit->second.clear();it!=set.end()&&it->first.selector==selector;it++)
			//if((it->second&setbits)==setbits) hit->second.push_back(&it->first); 
			if(it->second>=setbits) hit->second.push_back(&it->first); //NEW 
			return &hit->second;
		}

		std::vector<setfile> setfiles;

		std::vector<std::wstring> addsets_exploded;

		void addsets(const wchar_t *match)
		{	
			addsets_exploded.clear(); //static

			for(const wchar_t *p=match,*q=p;*p;q++) switch(*q)
			{	
			case ' ': case '\0': 
				
				wchar_t stackmem[32]; 
				wcsncpy_s(stackmem,p,q-p);
				if(map.find(stackmem)==map.end()) 
				addsets_exploded.push_back(stackmem);	  
			
			case '*': case '.': //dotted
				
				while(*q&&*q!=' ') q++; p = *q?q+1:q; 
			}			
			if(addsets_exploded.empty()) return; //!

			for(size_t i=0;i<addsets_exploded.size();i++)
			{
				map[addsets_exploded[i]] = map.size(); //insert
			}
			for(size_t i=0;i<setfiles.size();i++)
			for(size_t j=0;j<addsets_exploded.size();j++)
			{
				size_t pos = 
				setfiles[i].first.find(addsets_exploded[j]); 

				if(pos!=std::wstring::npos)
				if(pos==0||setfiles[i].first[pos-1]==' ')
				{
					addsetfile(setfiles[i],false); break; //!
				}
			}
		}		
		size_t addsetfile(setfile &in, bool insert=true)
		{
			setmask setbits = 0;

			const size_t ln_s = MAX_PATH*2; wchar_t ln[ln_s];
					
			wcscpy(ln,in.first.c_str()); //NEW: title is set list

			size_t len;
			for(len=0;ln[len];len++) switch(ln[len])
			{
			case '.': ln[len--] = '\0'; break; //--: end of the line

			case ' ': case '\t': case '\r': case '\n': ln[len] = '\0';
			}
			for(size_t i=0;i<len;i++) if(ln[i]) i+=setbit(ln+i,&setbits);
			
			size_t added = 0; if(!setbits) return 0;

			size_t cat = net[in.second].directory_length();		  

			swprintf_s(ln,L"%ls\\%ls",net[in.second].directory,in.first.c_str());
					
			//MSVC2005 extensions (ccs and wifstream)
			FILE *ccs = _wfopen(ln,L"rt, ccs=UTF-8"); 
			std::wifstream _set(ccs); while(_set.getline(ln+cat,ln_s-cat))
			{
				wchar_t *p = ln+cat; if(*p!='+') continue;

				for(*p++='\\';*p;p++); while(iswspace(p[-1])) p--; *p = '\0';

				WIN32_FIND_DATAW data; 
				//todo: if we had a local index here we could see if this
				//profile had been previously mentioned / skip this if so
				//Building a local index may also have better performance
				HANDLE find2 = FindFirstFileW(ln,&data);

				if(find2!=INVALID_HANDLE_VALUE) do //2021
				{
					if(!data.nFileSizeHigh) //paranoia
					add(in.second,data.cFileName,data.nFileSizeLow,&setbits,insert);
					added++;
				}while(FindNextFileW(find2,&data)); //2021

				FindClose(find2);
			}
			//fclose:
			//unclosed/locked by _set(css).~wifstream
			fclose(ccs); 
			return added;
		}
	};

	static size_t places(SOMPASTE p, HWND owner, location outs[], size_t outs_s, const wchar_t *in, const wchar_t *cat=0)
	{
		size_t out = 0; 

		while(*in&&out<outs_s) 
		{
			in+=(int)Somplace(p,owner,outs[out].directory,in,L"?",0);

			if(*outs[out].directory) if(cat)
			{	
				wcscat_s(outs[out].directory,cat);

				if(PathFileExistsW(outs[out].directory)) out++;
			}			
			else out++;
		}
		return out; 
	}
}

static Somproject_cpp::data Somproject_data; 

extern HWND Somproject(SOMPASTE p, HWND owner, wchar_t inout[MAX_PATH], const wchar_t *filter, const wchar_t *title, void *modeless)
{	
	Somproject_cpp::data *db = 0;

	if(!p) db = &Somproject_data; else assert(0); //unimplemented

	if(!db) return 0; //unfinished (SOMPASTE is nonzero)

	if(!db->indexed) /*this is not intended to be thread-safe*/
	{
		Somthread_h::section cs; //todo: one section per disk drive?

		assert(!db->indexed); //again: not intended to be thread safe...

		if(db->indexed) return Somproject(p,owner,inout,filter,title,modeless);

		const size_t database_s = 16;
		Somproject_cpp::location database[database_s]; size_t s = 0;
		s+=Somproject_cpp::places(p,owner,database+s,1,p->get(L"USER"),L"\\data");
		s+=Somproject_cpp::places(p,owner,database+s,1,p->get(L"CD"),L"\\data");
		s+=Somproject_cpp::places(p,owner,database+s,database_s-s,p->get(L"DATA"));
		db->index(database,s,p->get(L"DATASET"));
	}																   

	assert(!modeless); //unimplemented (work in progress)
	
	Somproject_cpp::data::id id = db->select(inout,filter);

	inout[0] = '\0'; if(!id.first) return (HWND)id.second; //count

	int out = db->net[id.second].directory_length();

	if(MAX_PATH-out<Somproject_cpp::profile::title_s) return 0; //paranoia

	wmemcpy(inout,db->net[id.second].directory,out);

	inout[out++] = '\\'; wcscpy(inout+out,id.first->title);

	return (HWND)out;
}

extern const wchar_t *Somproject_longname(long name)
{
	return name<Somproject_cpp::longnames.size()?Somproject_cpp::longnames[name].c_str():0;
}
extern wchar_t Somproject_name(const wchar_t *longname) //EXPERIMENTAL
{
	auto &b = Somproject_cpp::longnames.back();
	assert(b.empty());

	if(b.empty()) b.assign(longname); 
	else Somproject_cpp::longnames.push_back(longname);

	size_t ret = Somproject_cpp::longnames.size()-1;
	
	Somproject_cpp::longnames.push_back(L""); //add expects an empty one on the back
	
	return (wchar_t)ret;
}

extern bool Somproject_inject(SOMPASTE p, HWND owner, wchar_t path[MAX_PATH], size_t kind_or_filesize)
{
	wchar_t *longname = PathFindFileNameW(path);
	if(!*longname) return false;

	wchar_t swap = longname[-1]; longname[-1] = '\0';

	wchar_t place[MAX_PATH]; Somplace(p,owner,place,path,L"",0);

	int l = 0;
	for(auto&ea:Somproject_data.net)
	{
		if(!wcsicmp(ea.directory,place)) break;
		else l++; 
	}
	if(l==Somproject_data.net.size())
	{
		Somproject_cpp::location ll;
		wcscpy(ll.directory,place);
		Somproject_data.net.push_back(ll);
	}

	bool ret = Somproject_data.add(l,longname,kind_or_filesize);

	longname[-1] = swap;

	if(ret) Somproject_data.hit = 0;
	if(ret) Somproject_data.filters.clear(); //OVERKILL

	return ret;
}

extern HWND Somplace(SOMPASTE p, HWND owner, wchar_t out[MAX_PATH], const wchar_t *in, const wchar_t *title, void *modeless)
{	
	const wchar_t *ltrim = in;

	//todo: Unicode	white space??
	while(iswspace(*ltrim)) *ltrim++;

	const wchar_t *sep = ltrim;	
	while(*sep&&*sep!=';'&&*sep!='\r'&&*sep!='\n') sep++;

	const wchar_t *rtrim = sep;				
	while(rtrim>ltrim&&iswspace(rtrim[-1])) rtrim--;

	size_t outlen = rtrim-ltrim; 

	if(outlen<MAX_PATH)
	{
		wmemcpy(out,ltrim,outlen)[outlen] = '\0'; 
	}
	else *out = '\0';

	//#ifdef NDEBUG
	//todo: follow notes up top and Sompaste.h docs
	//#endif

	if(*out&&PathIsRelativeW(out))
	{
		wchar_t swap[MAX_PATH]; wcscpy_s(swap,out);	 

		swprintf_s(out,MAX_PATH,L"%ls/%ls",p->get(L"CD"),swap);
	}

	//if(!PathFileExistsW(out)) *out = '\0'; //testing

	assert(!modeless); //unimplemented

	if(*out) SOMPASTE_LIB(Path)(out);

	return (HWND)(sep-in+(*sep?1:0)); 
}

namespace Somproject_cpp
{
	typedef std::vector<wchar_t> value;

	typedef std::map<std::wstring,value> getenv; 

	typedef std::map<std::wstring,value>::iterator variable;
};

static Somproject_cpp::getenv Somproject_getenv;

extern const wchar_t *Somenviron(SOMPASTE p, const wchar_t *var, const wchar_t *set)
{
	Somproject_cpp::getenv *getenv = 0;

	if(!p) getenv = &Somproject_getenv; //todo: p->environ

	if(!getenv||!var){ assert(0); return L""; } //unimplemented

	DWORD n = 1; 

	Somproject_cpp::variable it = getenv->find(var);
	
	if(it==getenv->end()) 
	{
		if(!set)
		{
			n = GetEnvironmentVariableW(var,0,0);

			//http://stackoverflow.com/questions/20436735/
			//intended to fix an XP bug around "" assignment
			if(n==0&&GetLastError()==ERROR_MORE_DATA) n = 1;

			if(!n) return 0; //!
		}

		const Somproject_cpp::value nul(1,'\0');
		it = getenv->insert(std::make_pair(var,nul)).first; 		
		it->second.resize(n); 
	}

	Somproject_cpp::value &val = it->second; 
	
	if(set)	if(p) //untested
	{					 
		val.clear();
		for(const wchar_t *p=set;*p;p++)
		val.push_back(*p); 
		val.push_back('\0'); return &val[0];
	}	
	else SetEnvironmentVariableW(var,set); 
	
	val[0] = '\0';
	
	n = GetEnvironmentVariableW(var,&val[0],val.size());

	//http://stackoverflow.com/questions/20436735/
	//intended to fix an XP bug around "" assignment	
	if(n==0&&GetLastError()==ERROR_MORE_DATA) n = 1;
	
	if(n>val.size()) 
	{
		val.resize(n); 
	
		n = GetEnvironmentVariableW(var,&val[0],val.size());

		if(n!=val.size()-1) val[0] = '\0'; 
		
		assert(val[0]);
	}	
	return &val[0]; 	
}