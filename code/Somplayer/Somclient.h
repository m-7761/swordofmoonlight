
#ifndef SOMCLIENT_INCLUDED
#define SOMCLIENT_INCLUDED

#include "Somscribe.h"
#include "Somtexture.h"
#include "Somconsole.h" 

class Somenviron;

namespace Somclient_h
{		
	class blacklist; //Somconsole
}

class Somclient 
:
public Somscribe,
public Somconsole_h::enums
{
	Somclient(Somconsole*);
	Somclient(const Somclient&);
	~Somclient();

	///// The console-client/server model //////
	//	
	//The client embodies the flow of time along
	//with the unfolding of events that comprise
	//a game for example. It is also responsible
	//for connecting to and dealing with servers
	//
	// Somconsole <-> Somclient <-> "Somserver"
	//
	//It is assumed that a Somclient maintains a
	//single execution thread that runs parallel 
	//to everything else. The thread may in turn
	//have additional threads coordinated by the
	//main thread. Once the client is terminated
	//it is understood that it no longer has use
	//of the console and needs to wrap things up
	//	
	//Unlike just about everything else a client
	//is in a unique position to work intimately
	//alongside a console due to its tight scope

public: //integration

	//// creation and termination //////////////
	//
	// Obviously this works like a C entry point

	//kick off a new Somclient (per Somconsole*)
	//The returned Somclient* is only a courtesy
	//The Somconsole's "client" member is set to
	//the same value (if non-zero the Somclient*
	//is first terminated if not soft resetting)
	static Somclient *main(Somconsole *console); 

	int exit(); //int: the Somplayer status code
	   
	//// Somconsole access points //////////////
	//
	// In general it is correct to think of both
	// Somconsole and Somclient as a single data
	// object. Meaning that they are to be coded
	// side by side as statically linked objects
	// The divide is chiefly about encapsulation 
	// but conceptually there is the one console 
	// while there may be many clients depending
	// upon the task. A client can even be truly
	// modular (dynamically linked) if necessary
	// just as long as nobody else needs to know

	//These should implement a mutual lock to be
	//used exclusively by the console and client 
	//A console will use the lock conservatively
	//A client will usually use the lock only if
	//it needs complete knowledge of a moment in
	//time. Ideally a client would never need to
	//pause itself and instead just get by being
	//accepting of a certain amount of fuzziness
	//stop_clock: if true the client should take
	//this as a cue to pause its real time clock
	//and return true if it does so. A client is
	//permitted to refuse to pause its clock and
	//can also continue functioning in a limited
	//capacity as long as it does not need to be
	//pause'd for any reason in the interim with
	//or without a real time clock functionality
	//A client might for example permit a player
	//to explore their world in its paused state
	bool pause(bool stop_clock=false), resume();

	//Somconsole::mpx defers to Somclient::watch
	const Somenviron *watch(const wchar_t*,int);
	
	void blind(const Somenviron*); //screensaver

	int frame(const Somenviron*); //refresh rate

	//adjust the projection matrix for rendering
	bool shape(const Somenviron*,const Somtexture*);		
	
	enum //Sword of Moonlight's composite layers 
	{
		THE_MAIN_CONTENT=00, THE_MENU_CONTENT=30, 	
		HEADS_UP_DISPLAY=10, THE_ITEM_DISPLAY=40,
		THE_TEXT_CONTENT=20, THE_MENU_OVERLAY=50,
	};
	bool layer(int,Somenviron*,const Somtexture*,int);
					 
protected: //implementation

	Somenviron *environ; //should there be more?

	wchar_t soft_reset[MAX_PATH]; //current file
	
	HANDLE event; DWORD id, (WINAPI *ep)(VOID*); 

	static DWORD WINAPI Moonlight(VOID*); //loop

	Somclient_h::blacklist *console; //...
};

namespace Somclient_h
{	
	class blacklist : public Somconsole
	{
		//calls thru to Somclient::watch
		const Somenviron *mpx(const wchar_t*,int);
		//calls thru to Somclient::layer
		int render(const Somenviron*,const Somtexture*,int,int);
		int select(const Somenviron*,const Somtexture*,int,int);
	};

	////including STL in a header////
	
	struct iless //case insensitivity 
	{			
	typedef std::wstring _Ty; //mainly for Windows file names

	bool operator()(const _Ty &_Left, const _Ty &_Right)const
	{	  
		//assuming symmetric with Windows file system
		return wcsicmp(_Left.data(),_Right.data())<0;
	}};

	struct listhelper 
	{
		wchar_t meta_type;

	friend class listhelper_st;

	private: int item, root, base;
		
		const wchar_t** &list; //out

		std::vector<std::wstring> vec; 

		std::map<std::wstring,int,iless> map; 
				
		typedef std::pair<std::wstring,int> map_vt;

		typedef std::map<std::wstring,int,iless>::iterator map_it;
		
	public:	~listhelper(){ makelist(); }

		listhelper(const wchar_t**&out, wchar_t type=-1UL) 
		: 
		list(out), meta_type(type) //See Somconsole_makelist
		{		
			item = root = -1; base = Somconsole::size(list); 
		}									
		inline void makelist()
		{
			assert(base==Somconsole::size(list));
			assert(base+vec.size()<wchar_t(-1UL)); 

			extern const wchar_t** Somconsole_makelist
			(std::vector<std::wstring>&,const wchar_t**,wchar_t);

			list = Somconsole_makelist(vec,list,meta_type);
			base = Somconsole::size(list);

			vec.clear();
		}		

		inline operator bool(){ return vec.size(); }

		wchar_t undefined;
		std::wstring::iterator::reference operator[](int head)
		{	 
			if(item<base||head>0) 			
			if(head<0&&item>=0&&-head<=list[item][Somconsole::HEAD])
			{
				return const_cast<wchar_t&>(list[item][Somconsole::HEAD]);
			}
			else return undefined = Somconsole::UNDEFINED;

			std::wstring &str = vec[item-base]; 

			size_t body = str[0], size = str.size(); assert(head<0);
						
			if(body-head>size) str.append(body-head-size,Somconsole::UNDEFINED);

			std::wstring::iterator it = str.begin();

			std::advance(it,body-head-1); 
			
			return *it;
		}						

		int &operator*(){ return item; }

		bool operator*=(const wchar_t *body)
		{	 
			const wchar_t *current;
			if(current=root<base?Somconsole::find(list,body,root):0)
			{				
				item = current[Somconsole::ITEM]; return false;
			}

			map_vt add(std::wstring(1,root-base),vec.size());			
			add.first+=body; 
			
			std::pair<map_it,bool> ins = map.insert(add);

			item = ins.first->second+base; if(!ins.second) return false; 

			size_t size = wcslen(body);	add.first[0] = size+1; 
			
			vec.push_back(add.first);
						
			operator[](Somconsole::PATH) = root;
			operator[](Somconsole::SIZE) = size;			
			operator[](Somconsole::ITEM) = item;			

			return true;
		}

		int &operator&(){ return root; }

		bool operator&=(wchar_t (&path)[MAX_PATH])
		{				
			root = -1; if(!path) return false;

			const wchar_t *current;
			if(current=Somconsole::find(list,path))
			{
				root = item = current[Somconsole::ITEM]; return false;
			}

			map_vt add(std::wstring(1,-1),vec.size()); 

			bool out = false;

			for(wchar_t *p=path,*q=p,i=0;*p&&i<MAX_PATH;q++,i++)
			{
				if(*q=='\\'||*q=='/'||*q=='\0')
				{		
					int op = 0;

					if(*p=='.')
					{
						if(p+1==q) op = '.';
						
						if(p[1]=='.'&&p+2==q) op = '..'; 
					}

					wchar_t d = *q; *q = '\0'; //ATTN #1

					switch(op)
					{
					case '.': break; case '..': 
						
						item = operator[](Somconsole::PATH); break;

					default: case 0: add.first+=p; 
					
						std::pair<map_it,bool> ins = map.insert(add);

						item = ins.first->second+base; if(!ins.second) break; //!

						size_t size = q-p; add.first[0] = size+1; 
						
						vec.push_back(add.first); add.second = vec.size();
						
						operator[](Somconsole::TYPE) = 0; 
						operator[](Somconsole::PATH) = root;						
						operator[](Somconsole::SIZE) = size;			
						operator[](Somconsole::ITEM) = item;
						
						out = true; break;		
					}
																
					if(*q=d) p = q+1; else p = q; //ATTN #2

					add.first[0] = (root=item)-base;

					add.first.resize(1);
				}
			}

			return out;
		}
	};

	struct listhelper_st
	{	
	private: int root, item; listhelper &lh; 

	public:	void pop(){	listhelper_st::~listhelper_st(); }

		void push(){ if(item==-2) new (this) listhelper_st(lh); }

		listhelper_st(listhelper &_lh):lh(_lh)
		{
			item = _lh.item; root = _lh.root;
		}
		~listhelper_st()
		{
			if(item<-1) return; 
			lh.item = item; lh.root = root;
			item = -2;			
		}
	};
} 

#endif //SOMCLIENT_INCLUDED