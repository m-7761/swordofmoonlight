					   
#include "Ex.h" 
EX_TRANSLATION_UNIT

//NOMINMAX
#undef min
#undef max

#include <set>
#include <map>
#include <deque> //REMOVE ME (VS2010 is probably safe)
#include <string>
#include <vector>
#include <cwchar>
#include <complex>

#include <limits> //numeric_limits

#include "Ex.number.h"	

#define EX_NUMBER_WS \
/*todo: honor Unicode white space*/\
' ': case '\t': case '\r': case '\n' 

#define EX_NUMBER_E 0.00001f

extern const float EX::NaN = 
std::numeric_limits<float>::quiet_NaN();				  
static const float Ex_number_NaN = std::log(-1.0f);

#ifdef _DEBUG
static int Ex_number_asserts()
{
	assert(!EX::isNaN(Ex_number_NaN)); return 0;
}
static int Ex_number_asserting = Ex_number_asserts();
#endif

namespace Ex_number_cpp
{	
	typedef EX::Calculator C;

	static float _E = EX_NUMBER_E; 
	
	typedef std::complex<float> value_type;

	struct value : value_type
	{	
		bool c; value(){ c = true; } 

		inline bool operator~(){ return !c; }		
		
		operator bool(){ return std::abs(*this)>_E; }

		//pow(-1,2) has a (small) imaginary component
		//operator float(){ return imag()?EX::NaN:real(); }
		operator float(){ return real(); }

		//REMOVE ME?
		//REMOVE ME?
		//REMOVE ME?
		//REMOVE ME?
		//SHOULD MATH FUNCTIONS USE THIS??????
		//(If so, what about += etc?)
		inline void operator=(const value &cp)
		{
			v() = cp.v(); if(!cp.c) c = false;
		}		

		//WTH MSVC? (_Complex_base::value_type)
		typedef std::complex<float> value_type;

		value_type &v(){ return *(value_type*)this; }

		const value_type &v()const{ return *(value_type*)this; }

		template<typename T> value(const T &cp, bool cp_c=true)
		{
			//I'M SLIGHTLY WORRIED complex WON'T PICK UP ON A
			//DERIVED TYPE.
			v() = cp; c = cp_c;
		}
	};	  
	static const value_type i_value(0,1);
	static const value_type _i_value(0,-1);

	typedef std::vector<value> value_vc;
	typedef value_vc::iterator value_it;	

	//out is *io unless io==io_s in which case io[-1]
	static void add_value(value_it io, value_it io_s)
	{
		for(value_it it=io+1;it<io_s;it++) *io+=*it;
	}
	static void sub_value(value_it io, value_it io_s)
	{
		for(value_it it=io+1;it<io_s;it++) *io-=*it;
	}
	static void mul_value(value_it io, value_it io_s)
	{
		for(value_it it=io+1;it<io_s;it++) *io*=*it;
	}
	static void div_value(value_it io, value_it io_s)
	{
		for(value_it it=io+1;it<io_s;it++) *io/=*it;
	}
	static void abs_value(value_it io, value_it io_s)
	{
		if(io!=io_s) *io = std::abs(*io); 
		for(value_it it=io+1;it<io_s;it++) *io+=std::abs(*it);
	}	
	static void if_value(value_it io, value_it io_s)
	{
		if(io_s-io==3) 
		{
			*io = *io?io[1]:io[2];
		}
		else if(io<io_s) *io = EX::NaN;
	}
	static void _E_value(value_it io, value_it io_s)
	{
		io[io==io_s?-1:0] = _E; 
	}
	static void and_value(value_it io, value_it io_s)
	{
		bool out = true; 
		for(value_it it=io;it<io_s&&out;it++) if(!*it) out = false;
		io[io==io_s?-1:0] = out;
	}
	static void nand_value(value_it io, value_it io_s)
	{
		bool out = true; 
		for(value_it it=io;it<io_s&&out;it++) if(!*it) out = false;
		io[io==io_s?-1:0] = !out;
	}
	static void or_value(value_it io, value_it io_s)
	{
		bool out = false; 
		for(value_it it=io;it<io_s&&!out;it++) if(*it) out = true;
		io[io==io_s?-1:0] = out;
	}
	static void nor_value(value_it io, value_it io_s)
	{
		bool out = false; 
		for(value_it it=io;it<io_s&&!out;it++) if(*it) out = true;
		io[io==io_s?-1:0] = !out;
	}
	static void xor_value(value_it io, value_it io_s)
	{
		switch(io_s-io)
		{
		case 0: return; 
		case 1: *io = EX::NaN; return;
		case 2: *io = !*io!=!io[1]; return;
		}
		bool out = !*io!=!io[1]; 
		for(value_it it=io+2;it<io_s;it++) out = !*it==out;
		*io = out;
	}
	static void xnor_value(value_it io, value_it io_s)
	{
		switch(io_s-io)
		{
		case 0: return; 
		case 1: *io = EX::NaN; return;
		case 2: *io = !*io==!io[1]; return;
		}
		bool out = !*io==!io[1]; 
		for(value_it it=io+2;it<io_s;it++) out = !*it!=out;
		*io = out;
	}
	static void x_value(value_it io, value_it io_s)
	{
		if(io!=io_s) *io = io->real();
	}
	static void y_value(value_it io, value_it io_s)
	{
		if(io!=io_s) *io = io->imag();
	}
	static void iy_value(value_it io, value_it io_s)
	{
		if(io!=io_s) *io = std::complex<float>(0,io->imag());
	}
	static void nan_value(value_it io, value_it io_s)
	{
		switch(io_s-io)
		{
		case 0: /*io[-1] = EX::NaN;*/ return;
		case 1: *io = _isnan(io->real())?1:0; return;
		}
		for(value_it it=io;it<io_s;it++) //coalesce
		{
			if(_isnan(it->real())) continue; *io = *it; return;
		}
		*io = io_s[-1]; //default to final argument
	}
	static void inf_value(value_it io, value_it io_s)
	{
		switch(io_s-io)
		{
		case 0: io[-1] = 			
		std::numeric_limits<float>::infinity(); return;		
		case 1: *io = !_finite(io->real())?1:0; return;
		}
		for(value_it it=io;it<io_s;it++) //coalesce			
		{
			if(!_finite(it->real())) continue; *io = *it; return;
		}
		*io = io_s[-1]; //default to final argument
	}
	static void not_value(value_it io, value_it io_s)
	{
		switch(io_s-io)
		{
		case 0: io[-1] = 0; return;		
		case 1: *io = !*io; return;		
		}
		for(value_it it=io;it<io_s;it++) //coalesce			
		{
			if(!*it) continue; *io = *it; return;
		}
		*io = io_s[-1]; //default to final argument
	}
	static void neg_value(value_it io, value_it io_s)
	{
		switch(io_s-io)
		{
		case 0: io[-1] = -_E; return; 
		case 1: *io = float(*io)<-_E; return;		
		}
		for(value_it it=io;it<io_s;it++) //coalesce			
		{
			if(float(*it)<-_E) continue; *io = *it; return;
		}
		*io = io_s[-1]; //default to final argument
	}
	static void int_value(value_it io, value_it io_s)
	{			
		if(io==io_s) return;

		if(_finite(io->real()))
		{
			*io = std::complex<float>((int)io->real(),(int)io->imag());
		}
		else *io = EX::NaN;
	}
	static void min_value(value_it io, value_it io_s)
	{
		if(io!=io_s)
		{
			float out = *io; value_it it=io+1;
			while(_isnan(out)&&it!=io_s) out = *it++;
			while(it!=io_s) if((float)*it++<out) out = it[-1];
			*io = out;
		}
		else io[-1] = std::numeric_limits<float>::min();
	}
	static void max_value(value_it io, value_it io_s)
	{
		if(io!=io_s)
		{
			float out = *io; value_it it=io+1;
			while(_isnan(out)&&it!=io_s) out = *it++;
			while(it!=io_s) if((float)*it++>out) out = it[-1];
			*io = out;
		}
		else io[-1] = std::numeric_limits<float>::max();
	}
	static void _value(value_it io, value_it io_s)
	{
		assert(io==io_s&&_isnan(io[-1].real()));
	}
	//Transcendentals	
	static void cos_value(value_it io, value_it io_s)
	{
		if(io!=io_s) *io = std::cos(*io);
	}
	static void cosh_value(value_it io, value_it io_s) //2018
	{
		if(io!=io_s) *io = std::cosh(*io);
	}
	static void exp_value(value_it io, value_it io_s)
	{
		if(io!=io_s) *io = std::exp(*io);
	}
	static void log_value(value_it io, value_it io_s)
	{
		switch(io_s-io)
		{		
		case 1: *io = std::log(*io); return;				
		case 2: *io = std::log10(*io);

			if(io[1].real()==10&&0==io[1].imag()) 
			return;
			*io/=std::log10(io[1]);

		case 0: return;	default: *io = EX::NaN;
		}
	}	
	static void pow_value(value_it io, value_it io_s)
	{
		//MSVC2013 gets stuck here... <double,double> is complex
		//but is it portable? Doesn't work with 2010!
		//for(value_it it=io+1;it<io_s;it++) *io = std::pow(*io,*it);
		//for(value_it it=io+1;it<io_s;it++) *io = std::pow<double,double>(*io,*it);
		for(value_it it=io+1;it<io_s;it++) *io = std::pow(io->v(),it->v());
	}	
	static void sin_value(value_it io, value_it io_s)
	{
		if(io!=io_s) *io = std::sin(*io);
	}
	static void sinh_value(value_it io, value_it io_s) //2018
	{
		if(io!=io_s) *io = std::sinh(*io);
	}
	static void tan_value(value_it io, value_it io_s)
	{
		if(io!=io_s) *io = std::tan(*io);
	}
	static void tanh_value(value_it io, value_it io_s) //2018
	{
		if(io!=io_s) *io = std::tanh(*io);
	}
	//2018
	//WARNING: Boost library says this is a naive implementation.
	//http://boost.sourceforge.net/doc/html/boost_math/inverse_complex.html
	//Boost's code is at least 600 lines to copy over.
	static void asin_value(value_it io, value_it io_s)
	{
		if(io!=io_s) if(io->imag()) 
		{			
			//-i*log(i*io+sqrt(1-io^2))
			*io = _i_value*std::log(i_value**io+std::sqrt(1.0f-*io**io));
		}
		else io->real(std::asin(io->real()));
	}
	static void asinh_value(value_it io, value_it io_s)
	{
		if(io!=io_s) if(io->imag())
		{
			//i*asin(-i*io)
			value_type iio = _i_value**io;
			value_type asin = _i_value*std::log(i_value*iio+std::sqrt(1.0f-iio*iio));
			*io = i_value*asin;
		}
		else //io->real(std::asinh(io->real()));
		{
			//en.wikipedia.org/wiki/Hyperbolic_function#Inverse_functions_as_logarithms
			float x = io->real(); io->real(std::log(x*std::sqrt(x*x+1)));
		}
	}
	static void acos_value(value_it io, value_it io_s)
	{
		if(io!=io_s) if(io->imag())
		{
			//pi/2+i*log(i*io+sqrt(1-io^2))
			*io = float(M_PI_2)+i_value*std::log(i_value**io+std::sqrt(1.0f-*io**io));			
		}
		else io->real(std::acos(io->real()));
	}
	static void acosh_value(value_it io, value_it io_s)
	{
		if(io!=io_s) if(io->imag())
		{
			//log(io+sqrt(io-1)*sqrt(io+1))
			*io = std::log(*io+std::sqrt(*io-1.0f)*std::sqrt(*io+1.0f));
		}
		else //io->real(std::acosh(io->real()));
		{
			//en.wikipedia.org/wiki/Hyperbolic_function#Inverse_functions_as_logarithms
			float x = io->real(); io->real(std::log(x*std::sqrt(x*x-1)));
		}
	}
	static void atan_value(value_it io, value_it io_s)
	{
		if(io!=io_s) if(io->imag())
		{
			//-i*atanh(i*io)
			value_type iio = i_value**io;
			*io = (std::log(1.0f+iio)-std::log(1.0f-iio))/2.0f*_i_value;
		}
		else io->real(std::atan(io->real()));
	}
	static void atan2_value(value_it io, value_it io_s)
	{
		if(io_s-io!=3/*||io->imag()*/)
		{
			*io = EX::NaN;
		}
		else *io = std::atan2(io[1].real(),io[2].real());
	}
	static void atanh_value(value_it io, value_it io_s)
	{
		if(io!=io_s) if(io->imag())
		{
			//1/2*(log(1+io)-log(1-io))
			*io = (std::log(1.0f+*io)-std::log(1.0f-*io))/2.0f;
		}
		else //io->real(std::atanh(io->real()));
		{
			//en.wikipedia.org/wiki/Hyperbolic_function#Inverse_functions_as_logarithms
			float x = io->real(); io->real(std::log((1+x)/(1-x))/2);
		}
	}	
	static const struct prime_pod
	{
		wchar_t gloss[8]; 
		
		int level, assoc, group; 
		
		void(*_value)(value_it io,value_it end); 

		void value(value_it io, value_it it)const
		{	
			_value(io,it);
			while(--it>io) if(~*it) io->c = false;	
		}
		operator wchar_t()const
		{
			return this?*gloss:'\0'; 
		}
						
	}primes[] = 
	//reverse precedence
	{{L"?",9,'l',':',0}, 
	 {L":",9,'l',  0,0},
	 {L"+",8,'l',  0,add_value},
	 {L"-",8,'l',  0,sub_value},
	 {L"*",7,'l',  0,mul_value},
	 //mul_ won't cut it for sets
	 {L"__of",7,'l', 0,mul_value}, 
	 {L"/",7,'l',  0,div_value},
	 {L"^",6,'l',  0,pow_value},
	 {L"abs",0,0,  0,abs_value},
	 {L"acos",0,0, 0,acos_value},
	 {L"acosh",0,0,0,acosh_value},
	 {L"asin",0,0, 0,asin_value},
	 {L"asinh",0,0,0,asinh_value},
	 {L"atan",0,0, 0,atan_value},
	 {L"atan2",0,0,0,atan2_value},
	 {L"atanh",0,0,0,atanh_value},
	 {L"|",0,'r','|',0}, 
	 {L"|",0,'r',  0,0}, 
     {L"[",0,'r',']',0},
     {L"]",0,  0,  0,0}, 
	 {L"(",0,'r',')',0}, 
	 {L")",0,  0,  0,0}, 
	 {L",",0,'l',  0,0},
	 {L";",0,'l',  0,0},	 	  	  
	 {L"if",0, 0,  0,if_value}, 
	 {L"_E", 0,0,  0,_E_value},
	 {L"and",0,0,  0,and_value}, 
	 {L"nand",0,0, 0,nand_value}, 
	 {L"or", 0,0,  0, or_value}, 
	 {L"nor",0,0,  0,nor_value}, 
	 {L"xor",0,0,  0,xor_value}, 
	 {L"xnor",0,0, 0,xnor_value},
	 {L"x",  0,0,  0,x_value}, 
	 {L"y",  0,0,  0,y_value}, 
	 {L"iy", 0,0,  0,iy_value}, 
	 {L"nan",0,0,  0,nan_value}, 
	 {L"inf",0,0,  0,inf_value}, 
	 {L"not",0,0,  0,not_value}, 
	 {L"neg",0,0,  0,neg_value}, 	 	 
	 {L"_",  0,0,  0,_value}, 
	 {L"_S", 0,0,  0,_value}, 
	 {L"_$", 0,0,  0,_value}, 
	 {L"_N", 0,0,  0,_value},  
	 {L"n",  0,0,  0,_value}, 
	 {L"int",0,0,  0,int_value}, 
	 {L"min",0,0,  0,min_value}, 
	 {L"max",0,0,  0,max_value},
	 {L"cos",0,0,  0,cos_value},
	 {L"cosh",0,0, 0,cosh_value},
	 {L"exp",0,0,  0,exp_value},
	 {L"log",0,0,  0,log_value},
	 {L"pow",0,0,  0,pow_value},
	 {L"sin",0,0,  0,sin_value},
	 {L"sinh",0,0, 0,sinh_value},
	 {L"tan",0,0,  0,tan_value},
	 {L"tanh",0,0, 0,tanh_value},
	};				 
	enum{primes_s =
	sizeof(primes)/sizeof(prime_pod)};			
	struct prime
	{
		const prime_pod *pod;
		
		#define NOP(op) \
		void operator op(const prime_pod*);
		inline operator const prime_pod*(){ return pod; }
		NOP(<) NOP(<=) NOP(==) NOP(!=) NOP(>=) NOP(>)
		#undef NOP	 								
		inline const prime_pod *operator->(){ return pod; }		
		inline const prime_pod &operator*(){ return *pod; }
				
		inline bool op(){ return pod&&pod->assoc&&pod->level; }

		//note: that there currently are no true right associated ops
		inline bool lop(){ return pod&&pod->assoc=='l'&&pod->level; }
		inline bool rop(){ return pod&&pod->assoc=='r'&&pod->level; }

		inline bool no(){ return !pod||!pod->assoc||pod->assoc=='r'; }
		inline bool mono(){ return !pod||!pod->assoc&&pod->_value; }	  

		inline bool closure(){ return pod&&!pod->assoc&&!pod->_value; }

		prime(const wchar_t *gloss)
		{
			pod = gloss>=primes[0].gloss
			&&gloss<primes[primes_s].gloss?(prime_pod*)gloss:0;
		}		
		inline bool operator<(prime p) //>
		{
			assert(op()); return pod->level>p.pod->level; 
		}	
		inline bool operator<=(prime p) //>=
		{
			assert(op()); return pod->level>=p.pod->level; 
		}		
	};

	static prime_pod subprimes[primes_s];

	static const bool subprimed = memcpy(subprimes,primes,sizeof(primes));

	static const wchar_t *subprime(const wchar_t *x)
	{
		if(prime(x)) return subprimes[(prime_pod*)x-primes].gloss;
		
		if(x<subprimes[0].gloss||x>=subprimes[primes_s].gloss) return x;

		return primes[(prime_pod*)x-subprimes].gloss; //subprime
	}

	struct glossless //std::less
	{
	bool operator()(const wchar_t *_Left, const wchar_t *_Right)const
	{
		return std::wcscmp(_Left,_Right)<0;
	}};
	static std::set<const wchar_t*,glossless> glossary;

	typedef std::set<const wchar_t*,glossless>::iterator glossary_it;
	typedef std::set<const wchar_t*,glossless>::reverse_iterator glossary_ti;

	static bool priming_glossary() 
	{
		for(size_t i=0;i<primes_s;i++) 
			
			glossary.insert(primes[i].gloss); return true;
	}
	static const bool primed = priming_glossary();
	
	static const wchar_t *seperator = *glossary.find(L",");
	static const wchar_t *parentheses = *glossary.find(L"(");
	static const wchar_t *__parentheses = *glossary.find(L")");		

	static bool pare(const wchar_t *greedy, const wchar_t *match)
	{
		if(*greedy==*match) while(*greedy) 		 			
		if(*greedy++!=*match++) return true; return false;
	}

	typedef unsigned short shortstack; //size_t
	
	static void push(shortstack t); //predeclaring

	static const wchar_t *__results = L"__results";
	
	typedef std::map<size_t,size_t>::iterator sparse_it;
	typedef std::multimap<std::wstring,size_t>::iterator label_it; 
	typedef std::multimap<std::wstring,size_t>::reverse_iterator label_ti; 
	typedef std::multimap<std::wstring,size_t>::value_type label_vt;	

	struct table //of items
	{	
		size_t n_number;

		EX::_sysnum _number;

		struct item //in table
		{	
			std::wstring assignment; 

			shortstack assignment_s, sum; value z;
			
			item *self_references; //new item(*this)

			//2022: should z be set to NaN?
			//NOTE: sum=0 is constant_sum below
			//NOTE: I've had itemize assign NaN to z
			item()
			{
				self_references = 0; assignment_s = 0;
				
				sum = constant_sum; z = EX::NaN; //2022
			}
			~item()
			{
				delete self_references; if(!EX::detached) push(sum); 
			}

			enum:shortstack{ unknown_sum=-1, constant_sum=0 };

			inline void operator=(const wchar_t *rv) 
			{
				push(sum); assignment = rv?rv:L"";

				sum = unknown_sum; assignment_s = 0;
			}				
		};	
			
		shortstack current;

		class/*key value*/pair 
		{	
		friend class table; 

			struct selection 
			{
				table &first; item &second; 

				selection(table &a, item &b):first(a),second(b){}

			}selected; shortstack selector, current; 
		
			pair(table &a, item &b, size_t c):selected(a,b)
			{
				selector = c; current = a.current;
			}

		public:	inline size_t _N(){ return selector; }
				
			inline selection *operator->()
			{
				if(current!=selected.first.current) 					
				*this = *this-0; return &selected; 
			}
			inline item &operator()(int i) //inspection
			{
				return selected.first.itemize(i+selector);
			}
			inline item &operator[](int i) //insertion
			{
				return selected.first.itemize(i+selector,true);
			}				  						
			inline pair operator-(int i) //summation
			{
				return selected.first(-i+selector);
			}	
			inline pair operator+(int i) //defaults
			{
				return selected.first(i+selector);
			}	
			inline pair operator++(int) //iteration
			{	
				current--; selector++; return *this-1;
			}
			inline void operator=(size_t __selection)
			{
				current--; selector = __selection;
			}	
			inline void operator=(const pair &cp)
			{
				new (this) pair(cp); //pointer
			}
		};	

		//relying on deque avoids
		//reallocation workarounds

	private: std::deque<item> items;		

		std::map<size_t,size_t> sparse;
		 		
		inline bool lightweight()
		{
			return keyname()==__results;
		}

	public: table(){ _number = 0; n_number = 0; }
						
		//using & with 0 references
		inline pair query(size_t i)
		{
			return pair(*this,itemize(i),i);
		}				
		inline pair insert(size_t i)
		{
			return pair(*this,itemize(i,true),i);
		}		
		item &itemize(size_t i, bool insert=false)
		{
			if(sparse.size()) sparse_insert: //sparse model
			{
				sparse_it it = sparse.find(i);

				if(it!=sparse.end()) return items[it->second];

				if(!insert) return *(item*)0; current++;

				if(i>=item::unknown_sum) return *(item*)0;

				size_t item = sparse[i] = items.size();

				items.resize(item+1); return items[item]; 
			}

			if(i<items.size()) return items[i];
			
			if(!insert) return *(item*)0; current++;

			if(i>=item::unknown_sum) return *(item*)0;

			if(i-items.size()>16) //use sparse model
			{
				size_t j,k;
				for(j=0,k=0;j<items.size();j++)
				{
					if(items[j].assignment_s
					||!items[j].assignment.empty())
					{
						if(k<j) items[k] = items[j];

						sparse[j] = k++;
					}
				}
				if(k<j) items.resize(k);
				//2020: stack overflow on items.empty()
				//return itemize(i,true);
				goto sparse_insert;
			}
			items.resize(i+1); return items[i];
		}		
		size_t next_assignment(size_t out)
		{
			if(sparse.size()) //using sparse model
			{
				sparse_it it = sparse.upper_bound(out);	
				while(it!=sparse.end())					
				if(!items[it->second].assignment.empty()) 
				return it->first; 
			}
			else while(++out<items.size())
			{
				if(!items[out].assignment.empty()) return out;
			}
			return -2; //unsigned
		}
		inline pair assignments()
		{
			return query(next_assignment(-1));
		}
		inline bool items_empty()
		{
			return items.empty();
		}	  
		inline void items_clear() 
		{
			if(lightweight()) return items.clear();

			labels.clear(); sparse.clear(); items.clear(); current++; 
		}	  
		inline size_t n()
		{
			if(lightweight()) return items.size();

			if(!sparse.size()) 
			return std::max(n_number,items.size());
			return std::max(n_number,sparse.rbegin()->first+1);
		}
		inline pair operator[](size_t i)
		{
			return insert(i);
		}
		inline pair operator()(size_t i)
		{
			return query(i);
		}

		std::multimap<std::wstring,size_t> labels;

		inline void apply_label(size_t i, const wchar_t *trimmed)
		{
			labels.insert(label_vt(trimmed,i));
		}		
		size_t follow_label(size_t i, const wchar_t* &trimmed, int s, int t, int u=0)
		{
			//todo: figure out how to do the same with equal_range
			label_ti ti = label_ti(labels.upper_bound(trimmed+s)); 

			while(ti!=labels.rend()&&pare(ti->first.c_str(),trimmed+s)) ti++; 

			while(ti!=labels.rend()) 
			if(ti->first.size()==t-s&&!ti->first.compare(0,t-s,trimmed+s,t-s)) 
			{
				if(ti->second>=i){ trimmed+=u; return ti->second; }else ti++; //match
			}
			else return i; return i; //mismatch		
		}	

		inline const wchar_t *keyname()
		{
			std::pair<const wchar_t*,table> *nul = 0;
			return *(const wchar_t**)((size_t)this-(size_t)&nul->second);
		}
	};
	typedef table::item item; typedef table::pair pair;

	typedef std::map<const wchar_t*,table>::iterator table_it;
	typedef std::map<const wchar_t*,table>::value_type table_vt;
	
	//map is required to not trigger reallocation
	static std::map<const wchar_t*,table> tables;		
				
	//// evaluation /////////////////////
	
	//__coff is short for computed offset
	static const wchar_t *__coff = L"__coff", *__X = L"__%";	
	static EX::_sysenum _sysoff(C*,const wchar_t*,size_t,float*);
	static EX::_sysenum _sysnan(C*,const wchar_t*,size_t,float*out)
	{
		*out = EX::NaN; return EX::constant; 
	}

	struct term //bytecode
	{		
		const wchar_t *gloss;		
				
		//0: complex or "prime" number w/ real
		//1: system number w/ precomputed subscript 
		//2: custom number w/ precomputed subexpression
		int mode(){	return !gloss||prime(gloss)?0:mode_1?1:2; }
		
		union
		{	
			float mode_0_real;

			EX::_sysnum mode_1;
		};
		union
		{
			float mode_0_complex;

			shortstack mode_0_parameter; 
			shortstack mode_1_subscript; 

			struct 
			{
				shortstack recursing;
				shortstack arguments; 
			};				   			
		};
		
		shortstack remaining; //...

		inline operator value() //std::complex<float>
		{
			return std::complex<float>(mode_0_real,mode_0_complex);
		}
		inline void operator=(const std::complex<float> &value)
		{
			mode_0_real = value.real(); 
			mode_0_complex = value.imag(); gloss = 0; //!
		}
		term(){} explicit term(int args)
		{
			gloss = L"__nul"; mode_1 = _sysnan; 
			arguments = args; remaining = 0;
		}

		shortstack stack; //...
	};
	static shortstack stack = 0; //!
						   
	static const shortstack temporary = 0;
	static const shortstack incomplete = 1;

	static std::vector<term> terms(2,term(0));
	
	//REMOVE ME?
	//(invalidates terms)
	static shortstack pop() //recycling out
	{
		if(stack) return terms[--stack].stack;

		terms.push_back(term()); return terms.size()-1;
	}
	static void push(shortstack t) //recycling bin
	{	 		
		while(++t>2) //NEW: descend into __results numbers
		//while(++t>2) t = terms[terms[stack++].stack=t-1].remaining;
		{
			if(terms[--t].gloss==__results) push(terms[t].recursing);
			t = terms[terms[stack++].stack=t].remaining;
		}		
	}	

	static const wchar_t *__unfolded = L"__unfolded";

	static const EX::_sysenum side_effect = EX::_sysenum(EX::mismatch+1);

	static EX::_sysenum _sysnop(C*,const wchar_t*,size_t,float*)
	{
		return side_effect; //marker (produces no output)
	}			
	static void mark(shortstack after, const wchar_t *marker)
	{			
		//hmm: invalidates terms
		shortstack soda = pop(); 
		shortstack before = terms[after].remaining;
		term &tag = terms[terms[after].remaining=soda];		
		tag.gloss = marker;     tag.mode_1 = _sysnop;
		tag.remaining = before;	tag.mode_1_subscript = after; 
		tag.arguments = 0;		
	}	

#define END values.end() 
	
	static value_vc values;
	//data (assuming float is portable)
	static std::basic_string<float> _values(1,0); 		

	static const wchar_t *__default = L"__default";
	static const wchar_t *parameter = *glossary.find(L"_");	

	static size_t parameters = 0; //d: -1 yields variable
	static bool variable = false; //local input parameters
	static bool evaluate(size_t t, size_t n=0, size_t d=0) 
	{
		parameters = n;

		if(++d>32) //todo: maximum depth preference
		{
			if(EX::ok_generic_failure(L"Number recursion overflow (%d)",d))
			{
				EX::is_needed_to_shutdown_immediately(); t = item::unknown_sum;
			}	
		}
		if(t==item::unknown_sum) //lightweight NaN
		{
			values.push_back(EX::NaN); return variable = false;
		}
		
		//hack: temporaries cleanup after themselves
		const bool out = t||!terms[temporary].gloss;

		assert(out||terms[temporary].arguments==parameters);

		int coff_ = 0, _ = values.size()-n, m = n;

		#define COFF_(_1) \
		(term.recursing==item::unknown_sum\
		&&term.gloss?END[coff_=_1].real():term.recursing)
		#define COFF_SUBSCRIPT COFF_(-1)
		#define COFF_SUBTERM COFF_(-term.arguments-1)

		do{	term &term = terms[t];

		switch(term.mode())
		{
		case 0:	//prime number
		{
		if(term.gloss)
		{
			shortstack param = 
			term.mode_0_parameter?COFF_(-1)-1:-1;
			if(term.gloss!=parameter) //_
			{
				if(!EX::isNaN(term.mode_0_real))
				values.push_back(term.mode_0_real);			
				prime(term.gloss)->value(END-term.arguments,END);
			}
			else values.push_back //d: local parameters
			(param<m?values[_+param]:value(EX::NaN,d)); 
			if(term.arguments>1)
			values.erase(END-term.arguments+1,END);										
		}
		else values.push_back(term); break; //!
		}
		case 1: //system number
		{
		bool c = true;
		if(_values[0]=term.arguments)
		{
			_values.resize(1+term.arguments);
			value_it it = END-term.arguments;
			for(size_t i=1;i<=term.arguments;it++)
			{
				_values[i++] = *it; if(~*it) c = false;
			}	   			
			values.erase(END-term.arguments,END);
		}
		switch(term.mode_1(0/*this*/,term.gloss, 
			   COFF_SUBSCRIPT,(float*)_values.data()))
		{
		case EX::variable: c = false; //falling thru
		case EX::constant:
			
			values.push_back(value(_values[0],c)); break;

		case EX::mismatch: //todo: suppressable dialog

			values.push_back(EX::NaN); break;

		case side_effect: //introduce variablity
			
			variable = !c; continue; 

		}break;
		}
		case 2: //custom number 
		{
		if(term.gloss==__default) 
		{
			while(m<term.arguments) 
			values.push_back(EX::NaN),m++;
			if(m==term.arguments)
			evaluate(term.recursing,m++,d); 
		}
		else 
		{		
			evaluate(COFF_SUBTERM,term.arguments,d);
			if(term.arguments)
			values.erase(END-1-term.arguments,END-1);	

		}parameters = n; //restore
		}}
		if(coff_) //computed offset 
		{
			coff_ = 0; //swap and pop coff value 
			END[-2] = END[-1]; values.pop_back();
		}
		assert(!variable);
		//if(variable) values.back().c = variable = false;
		}while(t=terms[t].remaining); 
		if(m-=n) values.erase(END-1-m,END-1);
		#undef COFF_SUBSCRIPT
		#undef COFF_SUBTERM
		#undef COFF_
		return out;
	}	

#undef END	
			
	//static const wchar_t *__coff = L"__coff";
	static void _sum(pair);
	EX::_sysenum _sysoff(C*, const wchar_t *in, size_t off, float *out)
	{
		size_t i = 0, n = *out; 
		while(i<n&&out[++i]>=0) off+=out[i]; 		

		if(i!=n) //negative/NaN input
		{
			*out = item::unknown_sum;
		}
		else if(*in) //todo: warnings
		{	
			term &temp = terms[temporary];

			//WARNING (2022)
			//query returns uninitialized values
			//where numbers aren't sparse arrays
			//and values are unset
			pair kv = tables[in].query(off);
			auto &v = kv->second;

			if(!&v) //conservative 
			{
				if(temp.mode_1=kv->first._number) 
				{
					temp.gloss = in;
					temp.mode_1_subscript = off;										
					temp.arguments = parameters; //!!!

					*out = temporary; 
				}
				else *out = item::unknown_sum;
			}
			else 
			{
				//2020: not yet computed???
				if(v.sum==item::unknown_sum)
				{
					_sum(kv);
				}	
				if(v.sum==item::constant_sum) 
				{
					/*2022? should be NaN? can v.z
					//be initialized to NaN?
					if(v.assignment.empty())
					{
						temp = EX::NaN;
					}
					else temp = v.z;*/

					temp = v.z; 
					
					*out = temporary;
				}
				else *out = v.sum; 
			}
		}
		else *out = off; return EX::constant;
	}		
	float staticoff(size_t args, float staticout=0)
	{
		typedef value_vc::reverse_iterator value_ti;
		for(value_ti ti=values.rbegin();args--;ti++)
		
		if(!(ti->c&&ti->real()>=0&&!ti->imag())) return EX::NaN;

		else staticout+=size_t(ti->real()); return staticout;
	}
	static size_t count(const wchar_t *in, size_t out=0)
	{
		if(!*in||out==item::unknown_sum) return 0;
		
		if(in==parameter) return parameters+1; //!!!

		return prime(in)?1:tables[in].n();
	}
	static EX::_sysenum _sysnth(C*, const wchar_t *in, size_t off, float *out)
	{	
		size_t n = count(subprime(in),off); *out = off>n?EX::NaN:n-off;

		return in==parameter?EX::variable:EX::constant;
	}
	static EX::_sysenum _sysNth(C*, const wchar_t *in, size_t off, float *out)
	{
		size_t n = count(subprime(in),off); *out = off>=n?EX::NaN:off;
		
		return in==parameter?EX::variable:EX::constant;
	}

	//// summation ////////////////////////////////
	////                                            
	//// The following 3 letter functions work step 
	//// by step to prepare numbers for evaluation. 
	//// Refer to sum for an illustration of usage.
	////
	//// WARNING: lexemes returned by lex may point
	//// into the assignment string input argument.
	//// (for labels based on constant expressions)

	static const wchar_t *n = *glossary.find(L"n");
	static const wchar_t *_N = *glossary.find(L"_N");	
	static const wchar_t *nan = *glossary.find(L"nan");

	static bool nth(const wchar_t *f){ return f==n||f==_N; }

	//__of is an operator that yields left of right
	static const wchar_t *__of = *glossary.find(L"__of");

	static const wchar_t *_S = *glossary.find(L"_S");
	static const wchar_t *_dollars = *glossary.find(L"_$");		

	//Reminder: labels shove two integers in second
	struct lexeme:std::pair<const wchar_t*,float>
	{
		template<class T>
		lexeme(const wchar_t*f,T s):pair(f,(float)s){}
		lexeme(){}
		template<class S, class T>
		lexeme(const std::pair<S,T> &cp):pair(cp){}
	};

	typedef std::vector<lexeme> lexeme_vc;	 
	typedef lexeme_vc::const_iterator lexeme_it;

	static lexeme_vc &nul = *(lexeme_vc*)0, pad; //private

	#define return(x){ assert(x); return x; }
	//step 1) translate/validate/collate input	
	static size_t lex(pair io, size_t sep, lexeme_vc &out=nul, lexeme *c=0)
	{			
		//c: entering subexpression (closure)
		int _s = c?io->second.assignment_s:0;

		if(sep&&!c) _s = 1+io(-1).assignment_s;

		const wchar_t *in = //REMOVE ME?
		io(-_s).assignment.c_str(), *p = in+sep; 
				
		if(!*p) return 0; //signaling end of series

		if(!c) io[0].assignment_s = _s; //new series

		bool wr = &out!=&nul; lexeme l; 		
		const wchar_t* &f = l.first = c?c->first:0; 		
		float &s = l.second = c?c->second:0;		
		size_t closure = 0, ws = 0, sub = c?s:-1, sub0 = 0;		
		size_t subscript = 0; table *subscript_e = 0;		
		size_t subscript__ = 0;
		for(const wchar_t *e=p,*home=p,*q;*p;e=p) 
		{	
			bool _sub0 = true;		
			
			#define SUB0 \
			{\
				sub0++; _sub0 = false;\
				if(wr) out.push_back(lexeme(parentheses,++sub));\
				if(wr) out.push_back(lexeme(/*0,0*/));\
			}
			#define _SUB0 for(;sub0;--sub0)\
			{\
				if(wr) out.push_back(lexeme(__parentheses,sub--));\
			}
			#define SEP(pos) \
			{\
				_SUB0 if(!c) return(pos-in); /*!*/ \
			\
				if(wr) out.push_back(lexeme(seperator,sub));\
			\
				home = p = pos; ws = 0; continue;\
			}
			#define SEPX(pos) \
			{\
				if(ws) SEP(pos)\
				else if(wr) out.push_back(lexeme(__of,0));\
			}
			//caution: important to implementation
			#define NOSCRIPT(S) ((S)&&(e==n||e==_N))
			#define SUBSCRIPT \
			{wchar_t*e,l=wcstod(p+1,&e);if(*e==']')s+=l,p=e;}			

			switch(*p) //white space
			{
			case ',': case ';': //cosmetic

			case EX_NUMBER_WS: p++; //consuming

				if(*p=='\r'||*p=='\n') //line ending
				{
					for(q=p;*p;p++) switch(*p)
					{
					default: goto home; case EX_NUMBER_WS:;
					}
					home: home+=q-p; continue;
				}
				if(!ws&&prime(f).op()) //eg. 1+ 2
				{
					if(wr) out.push_back(lexeme(/*0,0*/)); //1+0 2

					SEP(p) //series
				}				
				ws++; continue; 

			//NEW: suppress default parameters//

			case '%': //todo: raise bad symbol
								
				if(c||!ws&&p!=home) return(0); 

				if(p-ws!=home) SEP(p) //series

				if(wr) out.push_back(lexeme(__X,0));

				p++; ws++; continue;
			}

			const wchar_t *d = p; //wcstod

			//plain number
			if(d[*d=='.']>='0'&&d[*d=='.']<='9') 
			{
				s = std::wcstod(p,(wchar_t**)&e);  
			}
			else s = 0;

			//named number
			switch(e==p?-1:*e) //including suffixes
			{
			default: //yuck: ensure e is set on all paths
			{	
				//hmm: the pare loop is pretty heavy on _ numbers
				//todo: optimize with an underscore only glossary??
				glossary_ti ti = glossary_ti(glossary.upper_bound(e)); 

				while(ti!=glossary.rend()&&pare(*ti,e)) ti++; //greedy

				if(ti==glossary.rend()) return(0); //todo: raise bad symbol
				 
				if(e==p||prime(*ti).mono()){ p = e; e = *ti; break; } 
			}
			case 0: case ',': case ';': //sans suffix
			
			case EX_NUMBER_WS: p = e; e = 0; //!
			}
			
			prime fp(f), ep(e); //wcstod

			//2017: Adapting c to !wr scenario. (Found a bug.)
			enum{ GROUP=sizeof(prime_pod)/sizeof(wchar_t) };

			//blacklist
			if(ep.closure())
			{	   
				if(!c||*e!=c->first[GROUP]||e[1]) //eg. ) or |4)
				{
					return(0); //todo: raise grouping mismatch
				}
				if(fp.lop()||d-ws<=home) //eg. 1+) or ()
				{
					if(wr) out.push_back(lexeme(/*0,0*/)); //1+0) or (0)
				}
			}

			if(d-ws<=home) //home
			{
				assert(d-ws==home);

				if(ep.lop()) SUB0 //eg. -1				
			}
			else if(ep.op()==fp.op())
			{	
				if(!ep.op()) //series?
				{	
					//hack: subscript_e is 
					//being repurposed as a 
					//flag so to extend mono
					//to include numbers with
					//a subscript. Eg. f[X](Y)

					if(fp.mono()||subscript_e)
					{	
						if(ep.mono()) //eg. 2 3 
						{	
							if(!ws) //X Y good; XY sorry
							{
								return(0); //todo: raise bad symbol
							}
							else SEP(d) //series
						}
						else if((!f||ws)&&ep->group) //eg. 1( or f (
						{
							//!e||!c||*e!=c->first[GROUP]: eg. |a|
							if(!e||!c||*e!=c->first[GROUP]) SEPX(d) //series
						}
					}
					else if(!fp->group) //eg. )1 or )( or ))
					{						
						//HACK: 2020 patch for ))
						if(ep->gloss==__parentheses&&fp->gloss==__parentheses)
						{
							p = p; //breakpoint
						}
						else if(!ws&&e&&ep.mono()) //eg. )c
						{	
							//rpn sets up __coff (next step)
							//this is really just a pad which
							//allows rpn to be single buffered
							if(wr) out.push_back(lexeme(__coff,sub+1));

							if(s) return(0); //todo: raise bad subscript
							
							s = -int(sub+2); //look ahead for coff subject

							if(NOSCRIPT(1)) return(0); 

							subscript__ = 1; //hack							
						}
						else SEPX(d) //series
					}
				}
				else if(ws&&d[-1]==*f) //eg. 2 +-3
				{
					if(wr) out.pop_back(); //pop operator
				
					SEP(d-1) //series
				}
				else SUB0 //eg. 1+-2
			}
			else if(!fp.no()&&ep.lop()||fp.rop()&&!ep.no())
			{
				assert(0); //should this even be possible?
				return(0); //todo: raise dissociated operator
			}
			else if(ws&&fp.op()&&d[-1]==*f) //eg. 1 -2
			{
				if(wr) out.pop_back(); //pop operator
				
				SEP(d-1) //series
			}
						
			if(ep.lop()&&_sub0) _SUB0			

		  //// f is discarded from here on out /////

			if(!ep.op()||fp.op()) ws = 0; //!

			if(f=e) while(*e) //hack: saving e
			{
				if(*e++!=*p++) return(0); //todo: raise bad symbol
			}	
			e = f; //hack: restoring e
			
			typedef void f; //shadowing
			
			closure = 0; //!

			//grouping
			if(ep&&c&&*e==c->first[GROUP]) //)
			{
				_SUB0 l.second = sub;

				if(wr) out.push_back(l); assert(!e[1]);

				return(p-in); //closure
			}			
			else if(ep&&ep->group) //(
			{
				lexeme ll = l; //2020

				if(*ep!='(') 
				if(subscript) //__coff series
				{
					if(subscript==1) //[ to __coff(
					{
						int off = wr?out.back().second:0; 

						//-(sub+2): look ahead for coff subject
						if(wr) out.back().second = -int(sub+2); 
												
						if(off>0) //prepend static offset
						{
							out.push_back(lexeme(__coff,sub+1));
							out.push_back(lexeme(parentheses,sub+1));
							out.push_back(lexeme(nullptr,off));

							subscript = 2; //look out below...
						}
						else if(wr) out.push_back(lexeme(__coff,sub+1));
					}
					else if(wr) out.pop_back(); //][ to ,

					l.first = subscript>1?seperator:parentheses;
				}
				else if(ep->group=='|') //| to abs(
				{
					l.first = parentheses;

					//static: assuming "abs" will not be overloaded
					static const wchar_t *abs = *glossary.find(L"abs");

					if(wr) out.push_back(lexeme(abs,0)); 
				}

				l.second = sub+1;
				if(wr) out.push_back(l); assert(!e[1]);

				//2017: passing l because out can't be relied
				//on if wr is false
				//2020: l is changed from [ and | to ( 
				//if(closure=lex(io,p-in,out,&l/*ep*/)) //!
				ll.second = l.second;
				if(closure=lex(io,p-in,out,&ll/*ep*/)) //!
				{
					assert((int)closure>p-in); 

					p = in+closure; //continue; //match					

					if(wr) out.back().first = __parentheses;

					l.first = __parentheses;
				}
				else return(0); //mismatch
			}

			//subscript
			if(p[0]!='[')
			{
				//hack: using as flag above
				//if(!subscript) subscript_e; //???
				if(!subscript) subscript_e = 0; //2020 (correct?)

				subscript__ = subscript = 0;
			}
			else if(subscript||e&&ep.mono())
			{
				const wchar_t *label = 0;

				if(!subscript)
				{
					if(e==parameter) 
					{
						//_N: entering label space
						subscript_e = &io->first; s+=io._N();
					}
					else //subscript_e = tables.find(e);
					{
						table_it it = tables.find(e);
						subscript_e = it==tables.end()?0:&it->second;
					}
				}
				if(subscript__) //eg. f(X)g[Y]
				{
					subscript__ = 0; //NEW: conformant labels 
				}
				else for(int i=1,j=1;j;i++) switch(p[i])
				{
				case 0: return(0); 				
				case '[': j++; continue; 				
				case ']': if(--j) continue; 

					//2020: is this backward?
					label = subscript?p:0;

					if(p[i-1]=='[') return(0); //ie. []				
					
					if(p[i]==']'&&p[i+1]=='(') //debugging
					{
						e = e; //breakpoint
					}

					//0: __coff series must accept any label 
					//and then sort out the difference later on
					//Users just have to workaround this shortfall
					assert(!subscript||s==0);
					
					if(subscript_e) //!=tables.end()
					s = subscript_e->follow_label(s,p,+1,i,i);

					if(subscript) //continuing subscript
					{
						if(*p==']') //the label was matched
						{
							/*a desperate yet effective hack*/
							float coff_number = float(sub+1)/10; 
							coff_number+=subscript__+subscript++;

							if(wr) out.back() = lexeme(seperator,sub+1);
							if(wr) out.push_back(lexeme(label,coff_number));
							if(wr) out.push_back(lexeme(__parentheses,sub+1)); 
						}
						else label = 0; //...
					}
					else if(*p=='['&&p[1]!='-') SUBSCRIPT //[+1]		

					if(*p==']'&&*++p=='[') j = i = 1; 
				}	  
				if(!subscript) //leaving label space
				{
					if(e==parameter) s-=io._N();
				}
				if(*p=='['&&!label) subscript++;	

				if(NOSCRIPT(1)) return(0); 
			}	
			if(NOSCRIPT(s)) return(0); 

			if(closure) //continue
			{
				continue; //breakpoint
			}

			if(e==_S||e==_dollars) //_$
			{
				if(subscript
				 ||e==_S&&(int)s>_s
				 ||e==_dollars&&(int)s!=_s) return(0);

				//todo: raise bad subscript

				if(e==_S) //simple with keyname
				{
					l.first = io->first.keyname();
					l.second = io._N()-_s+s;
				}
				else l.second = _s; //_$
			}

			if(wr) out.push_back(l); 
		}
		if(&out) //NEW
		switch(out.size())
		{
		case 1: //falling thru
			
			if(l.first==__X) return(0); //%

		default: if(prime(l.first).op()) //eg. 1+
				
			if(wr) out.push_back(lexeme(/*0,0*/)); //0 

		break; case 0: //white space (undefined item)
			
			if(wr) out.push_back(lexeme(nullptr,EX::NaN)); 
		}
		if(c) return(0); //todo: raise bad closure

		_SUB0 return(p-in);	

	#undef SUBSCRIPT
	#undef NOSCRIPT
	#undef SEPX
	#undef SEP
	#undef _SUB0
	#undef SUB0	
	}
	#undef return
	//step 2) transform into RPN (shunting yard)
	static lexeme_vc &rpn(lexeme_vc &io, size_t io_i=0) 
	{
		size_t opargs = 1, chain = 0;		
		size_t fargs[10] = {0,0,0,0,0,0,0,0,0,0};
		static const wchar_t *ss = L"0123456789"; //hack

		io.push_back(lexeme(/*0,0*/)); //io may be pad
		lexeme_vc &stack = pad; size_t empty = pad.size(); //!
		
		bool separated = true; //NEW
		size_t x = io[io_i].first==__X;	io_i+=x; //%
		size_t out = io_i; 
		for(size_t i=io_i,io_s=io.size()-1;i<io_s;i++)
		{
			prime fp(io[i].first);

			if(fp.mono()) //number
			{	
				//move __coff to front
				if(io[i].first==__coff)
				{
					stack.push_back(io[--out]); 
				}
				io[out++] = io[i]; 
			}
			else if(fp.op()) //operator
			{
				while(stack.size()!=empty)
				{
					prime tp(stack.back().first);

					if(tp.op()&&(fp.lop()&&fp<=tp||fp<tp))
					{
						io[out++] = stack.back(); stack.pop_back();
						io[out-1].second+=opargs; opargs = 1;
					}
					else break;
				}

				io[i].second = opargs; opargs = 1;

				stack.push_back(io[i]);
			}
			else //grouping
			{
				size_t sub = io[i].second;

				switch(*fp) 
				{	
				default: assert(0);
				break;
				case '(': 

					if(!fargs[sub]) 
					{	
						size_t sub = io[i].second;

						if(sub>9)
						if(EX::ok_generic_failure(L"Number subexpression over 9 deep"))
						{
							EX::is_needed_to_shutdown_immediately();
						}			
												
						//+1: lets us do !fargs[sub]
						fargs[sub] = stack.size()+1;						

						//chain: appending args to front
						stack.push_back(lexeme(ss+sub,chain)); chain = 0;

						//function?
						if(!separated) //NEW
						if(out>io_i) //2020: f((0-10)) //i.e. f(-10) (out-of-bounds)
						{
							auto &f = io[out-1];

							//DICIER AND DICIER
							//2017: This isn't good enough. 1/(2) has 1 at io[out-1]
							//but / is on the back of stack.
							//I think it wants to look at io[i-1] but only if it is
							//outputted; i.e. it's the function name. Note that any
							//number can be a function; though I can't remember how
							//plain numbers are treated.
							//if(prime(io[out-1].first).mono()) 
							//2020: this isn't picking up x[1_](y)
							//if(io[i-1]==io[out-1]&&prime(io[i-1].first).mono())
							if(prime(f.first).mono()) 
							{
								//NOTE: i-1==out-1 doesn't cut it, but I worry that
								//io[i-1]==io[out-1] could perhaps be a coincidence
								
								bool a = f==io[i-1];
								bool b = f.first&&f.second<0; //__coff?

								if(a||b) 
								{
									stack.push_back(io[--out]);
								}
							}
						}
						
						stack.push_back(io[i]);

						break;
					}
					else; //fall thru
				
				case ')': //closure
				case ',': //courtesy lex
					
					separated = true; //NEW

					stack[fargs[sub]-1].second++; 
				   
					while(stack.size()!=empty
					&&stack.back().first!=parentheses)    
					{   
						assert(prime(stack.back().first).op());
						io[out++] = stack.back(); stack.pop_back();						
						io[out-1].second+=opargs; opargs = 1;
					}  

					if(*fp==',') continue; 

					stack.pop_back(); //) 
					
					if(stack.size()==fargs[sub]) 
					{
						opargs = stack.back().second;

						//If an operator does not exist to the left
						//or to the right, then an implicit + exists
						//static: assuming "+" will not be overloaded
						static const wchar_t *plus = *glossary.find(L"+");

						if(stack.size()==1 //check for operators
						||!prime(stack[stack.size()-2].first).op()) //left?
						{	
							if(!prime(io[i+1].first).op()&&opargs>1) //right?
							{	
								io[out++] = lexeme(plus,opargs); opargs = 1; 
							}
						}
					}
					else //function 
					{
						const wchar_t *f = stack.back().first;

						io[out++] = stack.back(); stack.pop_back();

						if(f!=n&&f!=_N //special?
						  ||stack.back().second!=1
						  ||out<2||!io[out-2].first
						  ||isdigit(*io[out-2].first))
						{		
							//then a regular function
							io[out++] = stack.back(); 

							//reverse for sum (next step)
							std::swap(io[out-2],io[out-1]);
						}
					}
					stack.pop_back();

					//subscripts
					if(io[out-1].first==__coff)
					{
						//reversing position of __coff 
						io[out++] = stack.back(); stack.pop_back();
					}
					else if(io[i+1].first==__coff) //f(X)g form 
					{
						i++; //this __coff is a pad (see previous step)
											  
						if(io[i+2].first!=__coff) //f(X)g 
						{
							io[out++] = lexeme(ss+sub,1); 
							io[out++] = lexeme(__coff,sub);
						}
						else chain = 1; //f(X)g[Y] 
					}

					fargs[sub] = 0; 
				}
			}
			separated = false; //NEW
		}
		while(stack.size()!=empty)
		{
			assert(prime(stack.back().first).op());
			io[out++] = stack.back(); stack.pop_back();
			io[out-1].second+=opargs; opargs = 1;
		}
		io.resize(out); return io;
	}
	//step 3) convert RPN to unoptimized bytecode
	static item &sum(pair io, const lexeme_vc &in=nul, size_t in_i=~0) //0
	{
		if(&in==&nul) //run the gamut
		{
			//2021: simplfy default arguments to avoid errors, etc.
			if(in_i==~0) in_i = pad.size(); 

			pad.resize(in_i); 
			pair kv = io-io->second.assignment_s;
			//NOTE: I think this is because the definitions are tied
			//together into a single string so it must be reparsed from
			//the beginning to find the separators
			for(size_t sep=0;sep=lex(kv,sep,pad);pad.resize(in_i))
			{
				//2021: there are cycle issues with default arguments
				//consider 2_*0.5,0_S will try to use the next subscript
				//as a default, which _S will then refer back to the first
				//(Note, I don't know if it's important here to force sum to
				//be recomputed. if so it's going to be a problem)
				//sum(kv++,rpn(pad,in_i),in_i); 
				if(kv->second.sum==item::unknown_sum)
				sum(kv,rpn(pad,in_i),in_i); 
				kv++;
			}
			return io->second;
		}
		else if(in.size()<=in_i) //paranoia
		{
			assert(in_i!=~0); //2021

			io->second.sum = incomplete; return io->second;
		}
		else if(io->second.sum!=item::unknown_sum)		
		{
			//NEW: keep the top most term in place
			push(terms[io->second.sum].remaining);
		}				  		

		size_t params = 0;
		size_t clear = values.size();
		size_t x = in[in_i].first==__X;				
		item &out = io(0); push(out.sum);
		for(size_t i=in_i+x,t=0,in_s=in.size();i<in_s;i++)
		{
			const void *test = &in; //breakpoint

			size_t fargs = 0;
			const wchar_t *f = 0;
			prime fp(f=in[i].first);

			if(f) //argument list?
			{
				if(fp.op())
				{
					fargs = in[i].second;
				}
				else if(*f>='0'&&*f<='9') 
				{
					fargs = in[i].second; //i++
					
					fp = prime(f=in[++i].first);
				}
			}
			
			//hmm: invalidates terms
			shortstack soda = pop(); 

			terms_invalidated:
			term &term = terms 
			[t=(t?terms[t].remaining:out.sum)=soda];
			term.remaining = incomplete;

			if(term.gloss=f) //!
			{
				if(!fp) //non-prime
				{
					if(*f=='[') //label
					{
						term = EX::NaN; //!

						int args = in[i].second;
						int sub = (in[i].second-args)*10; //!

						float base = staticoff(args);

						lexeme_it it = in.begin()+i+1;
						for(float s=-sub-1;it->second!=s;it++); 

						if(!EX::isNaN(base))
						for(int i=1,j=1;j;i++) switch(f[i])
						{					
						case 0: j = 0; break;
						case '[': j++; continue; 				
						case ']': if(--j) continue; 

							if(it->first==parameter) 
							{	
								base+=io._N();
								term.mode_0_real = 
								io->first.follow_label(base,f,+1,i);
							}
							else term.mode_0_real = 
							tables[it->first].follow_label(base,f,+1,i);
							term.mode_0_real-=base;							
						}			
					}
					else if(f==__coff) //subscript
					{	
						term.gloss = L"";
						term.mode_1 = _sysoff;
						term.mode_1_subscript = 0;

						//nth: n and _N special functions
						if(i+2>=in_s||!nth(in[i+2].first))
						{						
							int sub = in[i].second;
							lexeme_it it = in.begin()+i+1;
							for(float s=-sub-1;it->second!=s;it++); 
						 								
							if(it->first==parameter) //_
							{				
								term.mode_1_subscript = io._N(); 
							}
							else //if(!prime(it->first))
							{
								//shadowing
								const wchar_t *first = it->first;
								table_it jt = tables.find(first); 

								if(jt==tables.end()
								 ||jt->second._number&&!&jt->second)
								{
									term.gloss = L""; //pure system table
								}
								else if(term.gloss=jt->first) //mixed table
								{
									float coffset = staticoff(fargs);

									if(!EX::isNaN(coffset)) //dynamic?
									{	
										pair kv = jt->second(coffset);

										if(&kv->second
										 &&kv->second.sum==item::unknown_sum)
										{
											sum(kv); //!
										}	
									}
								}
							}
						}
					}
					else //modes 1 and 2
					{
						term.mode_1 = 0;
						table &table = //hack
						f==__results?io->first:tables[f];
						pair kv = table.query(in[i].second);

						if(in[i].second<0) //coff
						{								
							if(kv->first.items_empty()) 
							term.mode_1 = kv->first._number; 
							term.recursing = item::unknown_sum;
						}
						else if(!&kv->second) //mode 1
						{
							term.mode_1 = kv->first._number;
							term.mode_1_subscript = in[i].second;

							if(term.mode_1==0
							 ||term.mode_1_subscript>=item::unknown_sum)
							{
								if(fargs) term.mode_1 = _sysnan;
								if(!fargs) term = EX::NaN;
							}
						}
						else //mode 2
						{
							if(term.recursing=kv->second.sum)						
							if(term.recursing==item::unknown_sum)
							{
								//term.recursing = 
								terms[t].recursing =
								sum(kv).sum; //!
								goto terms_invalidated;
							}
							if(term.recursing==item::constant_sum)
							{
								//todo: wraparound value
								if(fargs) term.recursing = incomplete;
								if(!fargs) term = kv->second.z;
							}
						}
					}
				}		
				else if(f==_dollars) //_$
				{
					item *refs = out.self_references;

					if(refs) //mode 2
					{							
						term.gloss = 
						subprime(io->first.keyname());
						term.mode_1 = 0;

						if(refs->sum!=item::constant_sum)
						{
							term.recursing = refs->sum;
						}
						else if(fargs)
						{
							//todo: wraparound value
							term.recursing = incomplete;
						}
						else term = refs->z;
					}
					else if(prime(term.gloss=io->first.keyname()))
					{
						term.mode_0_parameter = 0;

						if(io._N()!=0) term.gloss = nan; 

						if(!fargs) 
						{	
							term.mode_0_real = Ex_number_NaN;
						}
						else term.mode_0_real = EX::NaN;							
					}
					else if(term.mode_1=io->first._number)
					{
						term.mode_1_subscript = io._N();
					}
					else if(fargs) //shouldn't be here
					{
						term.recursing = incomplete;
					}
					else term = EX::NaN;
				}
				else //prime 
				{	
					if(in[i].second<0) //coff
					{
						//it's too late to help this
						//if(in[i].first==parameter)
						term.mode_0_parameter = item::unknown_sum;					
						if(!x&&in[i].first==parameter&&values.back().c)
						params = std::max<size_t>(params,values.back().real());
					}
					else if(in[i].first==parameter) //>=0
					{
						term.mode_0_parameter = in[i].second;

						if(!x) params = std::max<size_t>
						(params,term.mode_0_parameter=in[i].second);
					}
					else term.mode_0_parameter = 0;					

					if(!fargs) 
					{	
						term.mode_0_real = Ex_number_NaN;
					}
					else term.mode_0_real = EX::NaN;	

					if(f==n||f==_N) //errors
					{
						term.gloss = L"";
					}
				}

				if(term.gloss)
				term.arguments = fargs;
			}
			else term = in[i].second; 
							
			const wchar_t *g = 0; //nth
			
			if(i<in_s-1) g = in[i+1].first;
			
			if(g==n||g==_N) //special functions
			{	
				//NEW: ensure seen as mode 1
				term.gloss = subprime(term.gloss);

				if(g==n) term.mode_1 = _sysnth;
				if(g==_N) term.mode_1 = _sysNth;
				term.mode_1_subscript = item::unknown_sum;
				if(in[i].second>=0) 
				term.mode_1_subscript = in[i].second;

				/*skipping lexeme of g*/ i++;
			}

			term.remaining = 0;

			evaluate(t,0,-1); 
		}
		if(!x) //todo: maximum defaults preference
		for(size_t i=std::min<size_t>(params,8);i>0;i--)
		{
			item &def = io(i); if(!&def) continue;

			if(def.sum==item::unknown_sum)
			{
				sum(io+i); //!
			}

			//hmm: invalidates terms
			shortstack soda = pop(); 
			term &prep = terms[soda]; prep.remaining = out.sum;
			prep.gloss = __default; prep.recursing = def.sum;	
			prep.arguments = i-1; prep.mode_1 = 0; 
			out.sum = soda; //prepend to front
		}
		if(terms[out.sum].remaining)
		{
			mark(out.sum,__unfolded);
		}	 
		out.z = values.back();
		values.resize(clear); return out;
	}
	static void _sum(pair kv){ sum(kv); } //_sysoff
	//step 4) calculate a result
	static float run(pair in, const float *f=0, size_t f_s=0) 
	{
		if(!&in->second)
		{
			float out = EX::NaN;
			if(in->first._number) //NEW 
			in->first._number(0,in->first.keyname(),in._N(),&out); 
			return out;	//paranoia
		}

		if(in->second.sum==item::unknown_sum) sum(in); 

		if(f_s||in->second.sum!=item::constant_sum) 
		{
			size_t o = values.size(), of_s = o+f_s; 

			for(size_t i=0;i<f_s;i++) values.push_back(f[i]);

			evaluate(in->second.sum,f_s); assert(o==0); //wanna know
			
			float out = values[of_s]; values.resize(o);

			return _isnan(out)?EX::NaN:out;
		}
		else return in->second.z;
	}		 				
	//step $) wrappers and clobbering
	static void preassign(table &lv, size_t lv_s, const wchar_t *rv)
	{		
		//todo: if part of a series is reassigned
		//then bust up the rest of the series into
		//multiple standalone assignments (clobber)

		for(const wchar_t *p=rv,*q,*d;*p;p++) if(*p=='$')
		{
			if(p==rv||p[-1]!='_') continue; //paranoia

			long _s = lv_s;

			if(p-2!=rv&&isdigit(p[-2])) //2_$
			{
				for(q=p-2;q!=rv&&isdigit(q[-1]);q--);
				
				_s+=wcstol(q,0,10);
			}
			else if(p[1]=='[') //_$[2]
			{
				_s+=wcstol(p+2,(wchar_t**)&d,10);

				if(*d!=']') _s = lv_s;
			}

			item &self = lv.itemize(_s);

			if(&self&&self.sum!=incomplete)
			{	
				item *refs = new item;

				if(self.sum==item::unknown_sum) sum(lv(_s));				

				item &base = lv.itemize(_s-self.assignment_s);

				refs->assignment = base.assignment;
				refs->assignment_s = self.assignment_s;				

				refs->sum = self.sum; self.sum = incomplete; refs->z = self.z; 

				refs->self_references = self.self_references;
				self.self_references = refs;
			}
		}
	}

	///// lightweight numbers //////////////	
	
	//static const wchar_t *__results = L"__results";  
	static table_vt table_vt_of_results(__results,table());	

	static table &table_of_results = table_vt_of_results.second;

	struct result{ size_t sum, len; float min, *nan, max; };

	static std::map<const float*,result> results; 

	typedef std::map<const float*,result>::iterator result_it; 
	typedef std::map<const float*,result>::value_type result_vt;

	static size_t read_result_hit = 0; //private//

	static const result_vt missing_result(nullptr,result());

	static std::deque<const result_vt*> cache_of_results(5,&missing_result);

	static const result_vt *find_result(const float *key) 
	{
		result_it it = results.find(key);
		
		if(it==results.end()) return &missing_result;

		cache_of_results.pop_back();
		cache_of_results.push_front(&*it);
		
		read_result_hit = 0; return &*it;
	}
	static const result_vt *need_result(const float *key) 
	{			
		if(cache_of_results[0]->first==key) return cache_of_results[0];

		return key==&EX::NaN?&missing_result:find_result(key);
	}
	static const result_vt *read_result(const float *key) 
	{
		if(cache_of_results[read_result_hit]->first==key)
		{
			return cache_of_results[read_result_hit];
		}
		for(size_t i=read_result_hit==0;i<cache_of_results.size();i++)
		{
			if(cache_of_results[i]->first==key)
			{
				return cache_of_results[read_result_hit=i];
			}
		}
		return find_result(key);
	} 	
	static void void_result()
	{
		if(cache_of_results[0]!=&missing_result) 
		for(size_t i=0;i<cache_of_results.size();i++)
		{
			cache_of_results[i] = &missing_result;
		}
	}
}
namespace EX{
namespace cpp = Ex_number_cpp;
typedef class EX::Calculator
{
public: 	
#ifndef EX_CALCULATORS
	
	static EX::critical _cs; //2022: som.MPX.cpp

	static float &_E;
	static cpp::value_vc &values;
	static cpp::shortstack &stack;
	static std::vector<cpp::term> &terms;		
	static std::map<const wchar_t*,cpp::table> &tables;
	
	//REMOVE ME?
	inline cpp::shortstack pop(){ return cpp::pop(); } 
	//REMOVE ME?	
	inline void push(cpp::shortstack t){ return cpp::push(t); }	
	inline void evaluate(size_t t, size_t _=0){ cpp::evaluate(t,_); }

	//todo: these should be private
	static cpp::lexeme_vc &nul, &pad;
	//todo: move body of these methods into EX::Calculator
	inline size_t lex(cpp::pair io, size_t sep, cpp::lexeme_vc &out=nul, cpp::lexeme *c=0)
	{
		return cpp::lex(io,sep,out,c);
	}								  
	inline cpp::lexeme_vc &rpn(cpp::lexeme_vc &io, size_t io_i=0) 
	{
		return cpp::rpn(io,io_i);
	}
	inline cpp::item &sum(cpp::pair io, const cpp::lexeme_vc &in=nul, size_t in_i=0)
	{
		return cpp::sum(io,in,in_i);
	}
	inline float run(cpp::pair io, const float *f=0, size_t f_s=0)
	{
		return cpp::run(io,f,f_s);
	}
	inline void preassign(cpp::pair kv, const wchar_t *rv)
	{
		return cpp::preassign(kv->first,kv._N(),rv);
	}

	static cpp::table &table_of_results;	
	static std::map<const float*,cpp::result> &results;

	inline const cpp::result_vt *need_result(const float *key)
	{
		return cpp::need_result(key);
	}
	inline const cpp::result_vt *read_result(const float *key)
	{
		return cpp::read_result(key);
	}
	inline void void_result()
	{
		return cpp::void_result();
	}

#else

	todo: Ex_number_cpp as prototype

#endif
	
	typedef cpp::result_vt result_vt; 
	typedef cpp::result_it result_it; 
	typedef cpp::result result; 
	typedef cpp::value_it value_it; 	
	typedef cpp::table_it table_it; 	
	typedef cpp::table table; 
	typedef cpp::item item;	
	typedef cpp::pair pair;	
	typedef cpp::term term;	
}C;

#ifndef EX_CALCULATORS	
EX::critical C::_cs; //2022: som.MPX.cpp
float &C::_E = cpp::_E;
cpp::value_vc &C::values = cpp::values;
cpp::shortstack &C::stack = cpp::stack;
std::vector<cpp::term> &C::terms = cpp::terms;
std::map<const wchar_t*,cpp::table> &C::tables = cpp::tables;
std::map<const float*,cpp::result> &C::results = cpp::results; 
cpp::lexeme_vc &C::pad = cpp::pad, &C::nul = *(cpp::lexeme_vc*)0;
cpp::table &C::table_of_results = cpp::table_of_results;	
#endif

}typedef EX::Calculator C;

namespace Ex_number_cpp
{
#ifdef EX_CALCULATORS

	static inline float *new_key(C *c, size_t size=1)
	{
		char *out = new char[sizeof(c)+sizeof(float)*size];

		*(C**)&out = c; return (float*)(out+sizeof(C*));
	}	   		
	static inline void delete_key(float *key){ delete[] ((C**)key-1);	}		

	static inline C *claim_key(const float *key){ return *((C**)key-1); }

#else //compare and contrast

	static inline C *claim_key(const float *key){ return 0; }

	static inline float *new_key(C *c, size_t size=1){ return new float[size]; }	   	

	static inline void delete_key(float *key){ delete[] key; }	

#endif
}
extern void EX::resetting_numbers_to_scratch(C *c)
{
	c->tables.clear(); c->_E = EX_NUMBER_E;
}
extern void EX::refreshing_numbers_en_masse(C *c)
{		
	C::table_it end = c->tables.end();

	for(C::table_it it=c->tables.begin();it!=end;it++)
	{
		C::pair kv = it->second.assignments();

		while(&kv->second) 
		{
			//2021: I think this is correct since I've changed the default
			//behavior to ignore already computed sums
			//c->sum(kv);
			c->sum(kv,c->pad);
			
			kv = it->second.next_assignment(kv._N());
		}
	}
}
static EX::_sysenum Ex_number_cpp_r(C *C, const wchar_t*, size_t r, float *out)
{
	int rout = rand(); switch(r)
	{
	case 0: *out = rout; break;
	
	case 1: *out = rout/32767.0f; break; //2021

	default: *out = r<32767?rout%r:EX::NaN; break; //RAND_MAX
	}
	return EX::variable;
}
extern void EX::including_number_r(C *c, const wchar_t *keyname)
{
	EX::assigning_number(c,keyname,Ex_number_cpp_r,32767); //RAND_MAX
}
static EX::_sysenum Ex_number_cpp_id(C *C, const wchar_t*, size_t base, float *out)
{		
	if(!base) switch(int(*out)) 
	{ 
	case 8: base = 8; break;
	case 6: base = 16; break;
	case 4: base = 64; break;
	case 3: base = 256; break;
	case 2: base = 4096; break;
	case 1: base = 16777216; break;
	}			 
	size_t shift = 0; switch(base)
	{
	case 2: shift = 1; break;
	case 4: shift = 2; break;			
	case 8: shift = 3; break;
	case 16: shift = 4; break;
	case 64: shift = 6; break;
	case 256: shift = 8; break;
	case 4096: shift = 12; break;
	case 16777216: shift = 24; break;
	}
	if(shift)		
	{
		size_t args = *out;
		size_t bits = shift*args;	
		if(bits<=24) //24 bit mantissa
		{
			size_t id = 0;
			for(size_t i=1;bits;bits-=shift,i++) 
			{
			broadcasted: //REMOVE ME?

				size_t unit = out[i];				
				if(unit<base) //within radix/positive
				{
					id|=unit<<bits-shift;
				}
				else if(out[i]<0) //broadcast?
				{
					size_t j = -out[i];
					if(j!=i&&j&&j<=args) out[i] = out[j];
					else shift = 0; 
					if(shift) goto broadcasted;
				}
				else shift = 0;

				if(!shift) break; //2020 (infinite loop?)
			}
			*out = id;
		}
		else shift = 0; 
	}		
	if(!shift) *out = EX::NaN; //todo: user error
	return EX::constant; 
}
static EX::_sysenum Ex_number_cpp_di(C *C, const wchar_t*, size_t subscript, float *out)
{
	size_t base = 0, shift = 0;

	if(subscript<64)
	{
		if(subscript<8)
		{
			if(subscript<4)
			{
				base = 2; shift = 1;
			}
			else 
			{
				base = 4; shift = 2;
			}
		}
		else if(subscript<16) 
		{
			base = 8; shift = 3;
		}
		else 
		{
			base = 16; shift = 4;
		}
	}
	else if(subscript<4096)
	{
		if(subscript<256) 
		{
			base = 64; shift = 6;
		}
		else
		{
			base = 256; shift = 8;
		}
	}
	else 
	{
		base = 4096; shift = 12;
	}
	 	
	if(*out>1) //supplying offset
	{
		if(subscript!=base) 
		{
			base = 0; //todo: user error
		}
		else subscript = out[2]; 
	}
	//todo: could handle in binary search
	if(subscript<base||subscript>=base+base)
	{
		base = 0; //todo: user error
	}
	else if(*out&&out[1]>=0&&out[1]<16777216) //2^24
	{
		size_t mask = (1<<shift)-1;
		*out = mask&size_t(out[1])>>(subscript-base)*shift;
	}
	else base = 0; //todo: user error 

	if(!base) *out = EX::NaN; return EX::constant;		
}
extern void EX::including_number_id(C *c, const wchar_t *id, const wchar_t *di)
{
	EX::assigning_number(c,id,Ex_number_cpp_id,4096+1);
	EX::assigning_number(c,di,Ex_number_cpp_di,4096+4096);
}
					  
static const wchar_t *Ex_number_key(const wchar_t *gloss)
{	
	//hack: 0~9 is used by arg count lexemes
	if(*gloss<='0'&&*gloss<='9') gloss = L""; 

	Ex_number_cpp::glossary_it it = 
	Ex_number_cpp::glossary.find(gloss);

	if(it==Ex_number_cpp::glossary.end()) 
	{
		wchar_t *dup = //MEMORY LEAK
		std::wcscpy(new wchar_t[std::wcslen(gloss)+1],gloss);

		return *Ex_number_cpp::glossary.insert(dup).first;
	}
	//hack: thwart overloading of prime numbers (for now)
	if(Ex_number_cpp::prime(*it)) return Ex_number_key(L"");

	return *it; 
}	   
static C::pair Ex_number_pair(C *c, const wchar_t *lv, const wchar_t **rv=0)
{	
	size_t id = 0, len = 0; const wchar_t *label = 0;

	const size_t name_s = 30; wchar_t name[name_s+1] = L"";

	for(size_t i=0;i<=name_s;i++) switch(*lv)
	{		
	case '[': //2020
	case '_': if(lv[+1]<'0'||lv[+1]>'9') goto _; //!

		id = std::wcstol(lv+1,(wchar_t**)&label,10); 

		if(*lv=='['&&*label!=']') //2020
		if(EX::ok_generic_failure(L"Number key subscript awry: %s",name))
		{
			EX::is_needed_to_shutdown_immediately();
		}

	case '=': //hack?
	case 0: case EX_NUMBER_WS: name[i] = 0; i = name_s; //!

	break; default: _: name[i] = *lv++; len++;
	}

	name[name_s] = 0; assert(name[len]==0); //detecting overflow for debug

	if(len==name_s)
	if(EX::ok_generic_failure(L"Number key over %d chars: %s",name_s,name))
	{
		EX::is_needed_to_shutdown_immediately();
	}		   		

	C::table &table = c->tables[Ex_number_key(name)];

	if(label&&*label=='_') 
	{
		switch(*++label)
		{
		default: rv = &label; break;

		case 0: case EX_NUMBER_WS: //_ = 

			if(!rv) assert(0);
		}
	}
	else rv = 0; //label only

	if(rv&&*rv&&**rv) //sheesh
	{
		wchar_t trim[name_s+1] = L"";

		for(size_t i=len=0;i<name_s;*rv+=1) 
		{
			switch(**rv) //todo: fix this mess
			{		
			case '=': if(rv==&label) goto rtrim;

			default: trim[i++] = **rv; break;

			case 0: *rv-=1; //I guess: break out

				rtrim: if(i&&trim[i-1]==' ') len = --i; 
				
				trim[len=i] = 0; i = name_s; continue; //!

			case EX_NUMBER_WS:

				//ltrim/collapse to single ASCII space
				if(i&&trim[i-1]!=' ') trim[i++] = ' ';
			}
			len = i; //!
		}
		trim[name_s] = 0;

		if(len==name_s||**rv&&rv!=&label)
		if(EX::ok_generic_failure(L"Number label over %d chars: %s",name_s,trim))
		{
			EX::is_needed_to_shutdown_immediately();
		}	

		table.apply_label(id,trim);
	}

	return table(id);
}

extern void EX::assigning_number(C *c, const wchar_t *lv, EX::_sysnum rv, size_t n)
{
	if(!lv||!*lv||!rv){ assert(0); return; } //paranoia

	C::pair kv = Ex_number_pair(c,lv);

	kv->first._number = rv; kv->first.n_number = n;
}	
extern void EX::assigning_number(C *c, const wchar_t *lv, const wchar_t *rv)
{
	if(!lv||!*lv||!rv){ assert(0); return; } //paranoia

	C::pair kv = Ex_number_pair(c,lv,&rv); 

	if(!*rv) return; //assuming pretty label layout

	c->preassign(kv,rv); //_$		
	
	kv[0] = rv; //insert and assign	
	
	while(1) //validate and serialize
	{
		//todo: what about encapsulation?

		size_t sep = 0; 	
		//2020: _sysoff needs sum set to unknown_sum
		//while(sep=c->lex(kv++,sep)) if(!rv[sep]) return;
		do
		{
			sep = c->lex(kv,sep);
			
			//HACK: random access into the sparse array
			//would need to rewind to assignment_s to
			//see if the first in the assingment is set
			//to unknown_sum (which isn't hard to do 
			//apparently... kv-assignment_s seems to do)
			if(kv->second.assignment_s)
			{
				assert(!kv->second.sum);
				kv->second.sum = kv->second.unknown_sum;
			}

			kv++;

			if(!rv[sep]) return;

		}while(sep);
	 
		//todo: configurable error handling
		if(EX::ok_generic_failure(L"Number error in expression: %s",rv))
		{
			EX::is_needed_to_shutdown_immediately(); //user error
		}		
		//todo: permit user to correct syntax
		return;
	}
}	
extern void EX::declaring_number(C *c, const wchar_t *lv, const wchar_t *rv)
{
	if(!lv||!*lv||!rv){ assert(0); return; } //paranoia

	Ex_number_pair(c,lv,&rv); 
}
extern void EX::debugging_number(C *c, const wchar_t *lv, const wchar_t *rv)
{
	/*this is here to let programmers trace code*/

	C::pair kv = Ex_number_pair(c,lv,&rv); 

	if(!*rv) return; //assuming pretty label layout

	c->preassign(kv,rv); //_$		
	
	kv[0] = rv; //insert and assign	

	for(size_t sep=0;sep=c->lex(kv,sep,c->pad);c->pad.clear())
	{
		c->rpn(c->pad); c->sum(kv++,c->pad); //more testing	
	}	
	if(c->pad.size()) c->pad.clear(); //error
}
static float Ex_number_clamp(float in, float min, float nan, float max)
{
	return in>=min&&in<=max?in:in<min?min:_isnan(in)?nan:max; 
}
extern const wchar_t *EX::assigning_number
(C *c, const float **lv, const wchar_t *rv, float min, float max)
{
	size_t sep = 0, _ = 0;
	float *key = (float*)*lv;

	//clear table if not continuing series
	if(!c->table_of_results.items_empty())
	{																	 
		C::item &lw = //lightweight
		c->table_of_results.itemize(0);

		std::wstring &a = lw.assignment;
		if(rv<=a.c_str()||rv>=a.c_str()+a.length())
		{
			//if variable then thwart recycling
			if(~lw.z) lw.sum = lw.constant_sum;
			size_t n = c->table_of_results.n();
			for(size_t i=1;i<n;i++) //shadowing
			{
				C::item &lw = c->table_of_results.itemize(i);
				
				if(~lw.z) lw.sum = lw.constant_sum;
			}
			c->table_of_results.items_clear();
		}
		else //continuing series
		{
			sep = rv-a.c_str(); rv = a.c_str();
		}
	}	

	size_t _s = c->table_of_results.n();
	C::pair kv = c->table_of_results[_s]; 

	C::item &lw = kv->second; //lightweight
	if(_s==0) lw = rv; else lw.assignment_s = _s;
		
	//__X: suppress default params
	//todo: c->pad should be private	
	c->pad.push_back(std::make_pair(Ex_number_cpp::__X,0.0f));

	sep = c->lex(kv,sep,c->pad);
		  
	if(!sep) //indicates error
	{
		//todo: configurable error handling
		if(EX::ok_generic_failure(L"Number error in expression: %s",rv))
		{
			EX::is_needed_to_shutdown_immediately(); //user error
		}		

		EX::resigning_number(key); *lv = &EX::NaN; 
		
		c->pad.clear(); return 0; //!
	}
	else c->rpn(c->pad);

	for(size_t i=0;i<c->pad.size();i++)
	if(c->pad[i].first==Ex_number_cpp::_dollars) //_$
	{		
		if(c->pad[i].second>_s) break; 
		if(c->pad[i].second<_s) continue;

		lw.self_references = new C::item;
		lw.self_references->sum = c->need_result(key)->second.sum; 		
		lw.self_references->z = *key; break;
	}
	else if(c->pad[i].first==Ex_number_cpp::parameter) _++;

	c->sum(kv,c->pad); c->pad.clear(); 
									 
	if(lw.self_references) //thwart recycling
	lw.self_references->sum = lw.constant_sum;

	if(c->need_result(key)->first) 
	{		
		c->push(c->need_result(key)->second.sum);

		c->results.erase(key); c->void_result();
	}

	if(key&&key!=&EX::NaN) //reuse key
	{
		*lv = key; //Ex_number_cpp::delete_key(key);
	}
	else *lv = Ex_number_cpp::new_key(c);

	if(~lw.z) //variable?
	{
		C::result var = {lw.sum,!_,min,(float*)&EX::NaN,max};
		c->results[*lv] = var;
	}
	
	const_cast<float&>(**lv) = 
	Ex_number_clamp(lw.z,min,EX::NaN,max); 
	
	const wchar_t *out = rv+sep; 
	if(_s==0&&*out) out = lw.assignment.c_str()+sep;	

	while(*out) switch(*out)
	{
	default: return out; case EX_NUMBER_WS: out++;
	}
	return 0;			 
}			 
extern int EX::assigning_numbering
(C *c, const float **lv, size_t len, const wchar_t *rv, float min, const float *nan, float max)
{
	if(!len) return *rv;

	bool variable = false;

	float *key = (float*)*lv;

	if(key==nan)
	{
		key = Ex_number_cpp::new_key(c,len);
		for(size_t i=0;i<len;i++) key[i] = nan[i];
		*lv = key; key = 0; //_$
	}
	else; //reusing key (assuming len works)

	size_t sep = 0, _s = -1, _ = 0;
	size_t sum = c->need_result(key)->second.sum;

	//c->table_of_results.items_clear();
	//clear table if not continuing series
	if(!c->table_of_results.items_empty())
	{										
		//if variable then thwart recycling
		size_t n = c->table_of_results.n();
		for(size_t i=0;i<n;i++) //shadowing
		{
			C::item &lw = c->table_of_results.itemize(i);
				
			if(~lw.z) lw.sum = lw.constant_sum;
		}
		c->table_of_results.items_clear();
	}

	while(rv[sep]&&++_s<len)
	{
		C::pair kv = c->table_of_results[_s]; 

		C::item &lw = kv->second; //lightweight
		if(_s==0) lw = rv; else lw.assignment_s = _s;			
		lw.z = EX::NaN; //in case of errors

		sep = c->lex(kv,sep,c->pad);

		//errors handled afterwards by !sep 
		if(sep) c->rpn(c->pad); else break; 

		for(size_t i=0;i<c->pad.size();i++)
		if(c->pad[i].first==Ex_number_cpp::_dollars) //_$
		{		
			if(c->pad[i].second>_s) break; 
			if(c->pad[i].second<_s) continue;

			lw.self_references = new C::item;
			lw.self_references->z = key?key[_s]:EX::NaN;
			lw.self_references->sum = sum; break;
		}
		else if(c->pad[i].first==Ex_number_cpp::parameter) _++;
		
		if(_){ sep = 0; break; } //not allowed

		c->sum(kv,c->pad); c->pad.clear(); 
										 
		if(lw.self_references) //thwart recycling
		lw.self_references->sum = lw.constant_sum;
		
		if(~lw.z) variable = true;
		
		const_cast<float*>(*lv)[_s] = 
		Ex_number_clamp(lw.z,min,nan[_s],max); 

		sum = c->terms[sum].remaining;
	}		
	
	if(key&&c->need_result(key)->first) 
	{		
		c->push(c->need_result(key)->second.sum);

		c->results.erase(key); c->void_result();
	}

	if(!sep) //indicates error
	{
		c->pad.clear(); //important!
				
		//todo: configurable error handling
		if(EX::ok_generic_failure(L"Number error in expression: %s",rv))
		{
			EX::is_needed_to_shutdown_immediately(); //user error
		}		

		if(!_s) //too conservative??
		{
			key = (float*)*lv; *lv = nan;
			Ex_number_cpp::delete_key(key); 
			return -1;
		}		
		else; //honoring the ok bits
	}

	if(variable) //serialize
	{
		C::pair kv = c->table_of_results(0);		

		for(sum=0;&kv->second;kv++)
		{
			C::item &lw = kv->second;

			size_t soda = c->pop();
			c->terms[soda].remaining = 0;

			if(~lw.z) //variable?
			{
				if(c->terms[lw.sum].remaining)
				{
					//in order for _$ to work
					//each item in the series
					//has to be one term wide
					C::term &term = c->terms[soda];
					term.gloss = Ex_number_cpp::__results;
					term.mode_1 = 0;
					term.recursing = lw.sum;
					term.arguments = 0;
				}
				else //singleton
				{
					c->push(soda); soda = lw.sum;
				}
				//thwart recycling
				lw.sum = lw.constant_sum;
			}
			else c->terms[soda] = lw.z;

			if(sum==0) //first item in series
			{
				sum = soda;
				C::result var = {sum,len,min,(float*)nan,max};	
				c->results[*lv] = var;
			}
			else c->terms[sum].remaining = soda; 
		}
	}

	return _s<len?-1:rv[sep]?+1:0;
}

extern bool EX::evaluating_number(const float *f, unsigned in,...)
{	
	if(f==&EX::NaN) return true;

	C *c = Ex_number_cpp::claim_key(f);
	
	EX::section raii(c->_cs); //2022: som.MPX.cpp
	
	if(!c->read_result(f)->first) return true;

	const C::result &var = c->read_result(f)->second; 

		//diverging from reevaluating_number//

	if(in) debug: //function
	{
		va_list va; va_start(va,in);
		for(size_t i=0;i<in;i++) c->values.push_back(va_arg(va,double));
		va_end(va);
	}
	c->evaluate(var.sum,in);

	if(c->values.size()!=in+1) 
	if(EX::ok_generic_failure(L"Number was %d; expected %d",c->values.size(),in+1))
	{
		EX::is_needed_to_shutdown_immediately(); //programmer error
	}			
	else if(1&&EX::debug) //DEBUGGING
	{
		c->values.clear(); goto debug;
	}

	const_cast<float&>(*f) = 
	Ex_number_clamp(c->values.back(),var.min,EX::NaN,var.max);

	bool ret = c->values.back().c;

	c->values.clear(); return ret;
}
extern bool EX::reevaluating_number(const float *f)
{
	if(f==&EX::NaN) return true;

	C *c = Ex_number_cpp::claim_key(f);

	EX::section raii(c->_cs); //2022: som.MPX.cpp
	
	if(!c->read_result(f)->first) return true;

	const C::result &var = c->read_result(f)->second; 

		//diverging from evaluating_number//

	if(var.len==0) return evaluating_number(f,0); //function

	c->evaluate(var.sum); 

	if(c->values.size()!=var.len) 
	if(EX::ok_generic_failure(L"Number was %d; expected %d",c->values.size(),var.len))
	{
		EX::is_needed_to_shutdown_immediately(); //programmer error
	}			

	for(size_t i=0;i<var.len;i++)
	{
		const_cast<float&>(f[i]) = 
		Ex_number_clamp(c->values[i],var.min,var.nan[i],var.max);
	}

	bool ret = c->values.back().c;

	c->values.clear(); return ret;
}
extern float EX::reevaluating_number(C *c, const wchar_t *lv)
{
	EX::section raii(c->_cs); //2022: som.MPX.cpp

	return c->run(Ex_number_pair(c,lv));
}
extern void EX::reevaluating_numbers(C *c)
{
	EX::section raii(c->_cs); //2022: som.MPX.cpp

	C::result_it it = c->results.begin(), end = c->results.end();

	for(;it!=end;it++) if(it->second.len) //variables without parameters
	{
		const C::result &var = it->second; 
			
		if(var.len==0) continue; //function
		
		c->evaluate(var.sum); 

		if(c->values.size()!=var.len) 
		if(EX::ok_generic_failure(L"Number was %d; expected %d",c->values.size(),var.len))
		{
			EX::is_needed_to_shutdown_immediately(); //programmer error
		}			

		for(size_t i=0;i<var.len;i++)
		{
			const_cast<float&>(it->first[i]) = 
			Ex_number_clamp(c->values[i],var.min,var.nan[i],var.max);
		}
		c->values.clear();
	}
}

extern void EX::resigning_number(const float *lv)
{	
	float *key = (float*)lv; 
	
	if(!key||key==&EX::NaN) return; 

	C *c = Ex_number_cpp::claim_key(key);

	Ex_number_cpp::delete_key(key);
					  
#ifndef EX_CALCULATORS //hack: static globals

	if(EX::detached) return; //assuming detaching

#endif

	C::result_it it = c->results.find(key);

	if(it!=c->results.end())
	{
		c->push(it->second.sum); c->results.erase(it);

		c->void_result();
	}
}


