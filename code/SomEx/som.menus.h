	 
#ifndef SOM_MENUS_INCLUDED
#define SOM_MENUS_INCLUDED
				
namespace SOM //old-style menus
{
	struct Menu; 
	extern Menu getMenuByNs(int);
	extern Menu getMenuById(const char *id, int id_s=0);
	
	extern void initialize_som_menus_cpp(); 

	//internals: 
	struct Term; //formerly of som.terms.h
	extern const Term &getTermById(const char *id, int id_s=0);
	struct Term{ const char *nn, *id, *en; int sz, ns; };	

	//employing DOM-like language 

	struct Menu //this is the document
	{
		int ns;	const char *id;			
		class Node; class Text; 
		class Element; typedef Element Elem; 
		
		//REMOVE ME?
		void (*on)(int,int); 

		const Node **nodes; 
		const Elem **elements; 
		const Text **text; 

		enum
		{
		SORT_UNSORTED,
		SORT_NODENAME, 
		SORT_UNIQUEID,
		TOTAL_SORTING,
		};
		/*void order(int); //unused
		inline void sortNodesById()
		{
			order(SORT_UNIQUEID);
		}
		inline void sortNodesByName()
		{
			order(SORT_NODENAME);
		}
		inline void unsortNodes()
		{
			order(SORT_UNSORTED);
		}*/

		inline Menu(int NS, const char *ID, void *o, Node **n, Elem **e, Text **t)
		{
			ns = NS; id = ID; on = (void(*)(int,int))o; nodes = (const Node**)n;
			
			elements = (const Elem**)e; text = (const Text**)t;
		}
		inline Menu(const Menu &cp)
		{
			memcpy(this,&cp,sizeof(*this)); 
		}
		inline Menu()
		{
			memset(this,0x00,sizeof(*this)); 
		}

		inline operator bool()const{ return ns; }

		struct Namespace //class
		{
			Node **nodes; Element **elements; Text **text;

			//int count(const Node**)const; //unused
		};
		
		static const Namespace *namespaces;

		struct Node //class
		{
			const int ns,cc; //namespace/control code
			
			const char *nn; //OLD: node name in menu graph		
			const char *id;	//original Japanese (or author text)
			const char *pf; //NEW: now an optional disambiguating msgctxt	

			int sz; //optimization: sizeof(id)-1 or strlen(id)			
			Node(int c=0):ns(0),cc(c?c:'none'),nn(""),id(""),sz(0){}
			Node(int c, const char *n, const char *i, int z=0, int s=0)
			:ns(s),cc(c),nn(n),id(i),sz(z),pf(0)
			{
				if(ns==0) return; //temp "nodes" use namespace 0
				if(*id) pf = strchr(id,'\4'); if(pf) std::swap(id,++pf);
				if(pf) sz-=id-pf;
			}			

			inline operator Node*(){ return this; }	 			
		};

		struct Text : Node //class
		{			
			//todo: change to 65001
			//along with som.menus.inl
			static const int cp = 932;

			const int op; //0,1:TextOut,2:DrawText

			const char *en;	//built-in ASCII/English

			Text(int c=0):Node(c),op(0),en(0),cb(0),re(0){}
			Text(int c, int o, const char *n, int s, const char *i, size_t z, const char *e)
			:Node(c,n,i,z,s),op(o),en(e),cb(0),re(0){ re = &re_lost; }

			inline operator Text*(){ return this; }

			bool irregular(void *procA)const;  

			bool match(void *procA, const char*, int=0)const; //by id

			const char **parse()const; //last matched 			
			const char *format()const; //printf format string...			
			const char *fchars()const; //printf format field characters

			int pixels()const; //%s: may want to extend to Use? like boxes?

			const char *translate(const char*,int=0)const;
			const char *translate2(const char*,int=0)const;

		private: friend void SOM::initialize_som_menus_cpp();

			//2017: format() was rejecting titles that use cb.
			//A whiteliest has been added to it for right now.
			//
			const char *(*cb)(const char*,int); //translator
			static const int re_lost = 0;
			mutable const int *re; 
		};
		
		struct Element : Node //class
		{	
			const Element **top()const;

			const Text *text; void (*on)(int ns, int exit); 

			Element(int c, int o, const char *n, int s, 
			const char *i, size_t z, const char *e, int m) 
			:Node(c,n,i,z,s),menu(m),text(&alt),alt('text',o,n,s,i,z,e),on(0){}
			Element(int c):Node(c),menu(0),alt(c),text(0),on(0){}

			inline Text *operator->(){ return &alt; }

			inline operator Text*(){ return &alt; }
			inline operator Elem*(){ return this; }			

		private: Text alt; const int menu;
		};
	};	
}

namespace som_menus_h
{	
	class validator //slash-translator
	{
		int is_case,in_case,is_cond;
		int is_item,in_item,is_more;
		const SOM::Menu::Text** text;				
		public: const SOM::Menu menu;		
		bool operator()(void*,const char*,int);		
		validator(const char *id=0, int id_s=0)
		: menu(SOM::getMenuById(id,id_s))
		{ memset(this,0x00,offsetof(validator,menu)); }			
		const SOM::Menu::Text *operator->(){ return *text; }				
		operator bool(){ return text; }
	};
}

#endif //SOM_MENUS_INCLUDED