
#include "Ex.h" 
EX_TRANSLATION_UNIT

//WARNING: this file is old/a bit of a mess
//it's improved a little bit but not enough

#include <map>
#include <string>

#include "Ex.regex.h"

#define EX_REGEX_OBJECTS 48

EX::Regex::Case EX::Regex::ZERO   = &EX::REGEX_ZERO;
EX::Regex::Case EX::Regex::LOST   = &EX::REGEX_LOST;
EX::Regex::Case EX::Regex::EXACT  = &EX::REGEX_EXACT;
EX::Regex::Case EX::Regex::IFANY  = &EX::REGEX_IFANY;
EX::Regex::Case EX::Regex::ALWAYS = &EX::REGEX_ALWAYS;
EX::Regex::Case EX::Regex::EMPTY  = &EX::REGEX_EMPTY;
EX::Regex::Case EX::Regex::NEVER  = &EX::REGEX_NEVER;
EX::Regex::Case EX::Regex::WHITE  = &EX::REGEX_WHITE;
EX::Regex::Case EX::Regex::ERROR  = &EX::REGEX_ERROR;
EX::Regex::Case EX::Regex::STRING = &EX::REGEX_STRING;
EX::Regex::Case EX::Regex::VALID  = &EX::REGEX_VALID;

//REMOVE ME?
static bool Ex_regex_ok = false;	
static void Ex_regex_iniatialize();
extern int EX::Regex_is_needed_to_initialize()
{
	static int one_off = 0; if(one_off++) return one_off; //???

	Ex_regex_iniatialize(); assert(Ex_regex_ok);

	return one_off; //???
}

static void Ex_regex_lock(EX::Regex::Case);
static void Ex_regex_unlock(EX::Regex::Case); 
static bool Ex_regex_finish_re(const char*&,int);

static const char *Ex_regex_decode_re(int,const char*,int*);
static const char *Ex_regex_decode_re(const wchar_t*,int*);

static const int *Ex_regex_simple_re(const char*,int);
static const int *Ex_regex_lookup_re(const char*,int,const int*);
static const int *Ex_regex_repair_re(const char*,int,const int*);

#define EX_REGEX_NONSIMPLE(RE) (Re&&*Re>EX::Regex::SIMPLE_CASES)

EX::Regex::Regex(int cp, const char *in, int in_s, const int *in_case_exact) 
{
	in = Ex_regex_ok?Ex_regex_decode_re(cp,in,&in_s):0;	 	
	Re = in?Ex_regex_repair_re(in,in_s,in_case_exact):EX::Regex::ERROR;	  
	if(EX_REGEX_NONSIMPLE(Re)) Ex_regex_lock(Re);
}
EX::Regex::Regex(const wchar_t *in, int in_s, const int *in_case_exact) 
{	
	const char *in8 = 
	Ex_regex_ok?Ex_regex_decode_re(in,&in_s):0;		
	Re = in8?Ex_regex_repair_re(in8,in_s,in_case_exact):EX::Regex::ERROR;	
	if(EX_REGEX_NONSIMPLE(Re)) Ex_regex_lock(Re);
}

void EX::Regex::lock(const int *re)const 
{
	if(EX_REGEX_NONSIMPLE(re)) Ex_regex_lock(re);
}				   
void EX::Regex::unlock(const int *re)const 
{
	if(EX_REGEX_NONSIMPLE(re)) Ex_regex_unlock(re);
}

typedef struct //NOT THREADSAFE
{
	static struct exact_t //TODO: EX::tls?
	{
		const int *re; int cp; 		
		const char *in; int in_s;
		inline void reset(){ re = 0; cp = 0; }

	}exact;

	static struct match_t //TODO: EX::tls?
	{
		const int *re; int cp, at;
		static const int out_s = 1024; 
		union
		{
			char out[out_s]; wchar_t wout[out_s];
		};
		char *tmp; int tmp_s, tmp_x;
		inline void reset(){ re = 0; cp = at = tmp_x = 0; }

	}match;

	static struct parse_t //TODO: EX::tls?
	{
		int out_x; static const int out_s = 16;		
		union
		{
			const char *out[out_s]; const wchar_t *wout[out_s]; 		
		};
		inline void reset(){ out_x = 0; }

	}parse;

}Ex_regex_thread;

Ex_regex_thread::exact_t Ex_regex_thread::exact = {0};
Ex_regex_thread::match_t Ex_regex_thread::match = {0};
Ex_regex_thread::parse_t Ex_regex_thread::parse = {0};

void EX::Regex::reset()
{
	Ex_regex_thread::exact.reset();
	Ex_regex_thread::match.reset();
	Ex_regex_thread::parse.reset();
}

void EX::Regex::exact(int cp, const char *in, int in_s)
{
	Ex_regex_thread::exact.re = Re;	
	Ex_regex_thread::exact.cp = cp;
	Ex_regex_thread::exact.in = in; 
	
	if(!in_s&&in&&*in) in_s = strlen(in);

	Ex_regex_thread::exact.in_s = in_s;
}

static bool Ex_regex_exact(int,const char**,int*,char**,int*);
static bool Ex_regex_white(int,const char**,int*,char**,int*); 
static bool Ex_regex_match(EX::Regex::Case,int,const char**,int*,char**,int*);

bool EX::Regex::match(int cp, const char **in, int *in_s, char **out, int *out_s)
{
	bool ok = false; if(!Re||!cp) goto mismatch; 
	 
	if(!in) //Reset a la iconv
	{
		if(Re&&*Re==EX::REGEX_ZERO) ok = true; goto reset;
	}
	
	if(Ex_regex_thread::match.at==0)
	{
		if(Ex_regex_thread::exact.re!=Re) Ex_regex_thread::exact.reset();

		Ex_regex_thread::match.re = Re;	Ex_regex_thread::match.tmp_x = 0;
		
		Ex_regex_thread::parse.out_x = 0;
	}

	for(int in_x=0;!in_s;in_s=&in_x) in_x = strlen(*in);

	if(!out) //provide an internal buffer
	{
		out = &Ex_regex_thread::match.tmp; 

		Ex_regex_thread::match.tmp = Ex_regex_thread::match.out; 

		if(!out_s) out_s = &Ex_regex_thread::match.tmp_s;
			
		*out_s = Ex_regex_thread::match.out_s;

		if(Ex_regex_thread::match.tmp_x)
		{
			*out_s-=Ex_regex_thread::match.tmp_x;

			*out+=Ex_regex_thread::match.tmp_x;
		}
	}

	if(*Re<=EX::Regex::SIMPLE_CASES) switch(*Re)
	{  	   
	case EX::REGEX_ZERO: if(!in) goto match; break;	 
	case EX::REGEX_STRING: if(in) goto match; break;
	case EX::REGEX_EXACT: 
		
		if(Ex_regex_thread::exact.re!=Re) goto mismatch; 
		
		ok = Ex_regex_exact(cp,in,in_s,out,out_s); break;

	case EX::REGEX_IFANY:
		
		if(in&&*in&&in_s&&in_s) ok = true; break;

	case EX::REGEX_ALWAYS: goto match;	 
	case EX::REGEX_EMPTY:
		
		if(in&&(!*in||(in_s&&!in_s))) ok = true; break;

	case EX::REGEX_NEVER: case EX::REGEX_LOST: 

	default: goto mismatch;

	case EX::REGEX_WHITE: 

		ok = Ex_regex_white(cp,in,in_s,out,out_s); break;
	}
	else if(*Re<=0xFF&&*Re>=0xF0) switch(*Re)
	{
	case EX::REGEX_ERROR: //unimplemented
	case EX::REGEX_VALID: assert(0); break; 

	default: goto mismatch;
	}
	else ok = Ex_regex_match(Re,cp,in,in_s,out,out_s);
	
	if(!ok||!*in_s) goto reset;

	return ok; 

match: ok = true;

mismatch: //FALL THRU

reset: //Reset a la iconv()

	Ex_regex_thread::match.at = 0;

	return ok;	
}

int EX::Regex::parse(const char **out, int out_s, int partial)
{
	if(Ex_regex_thread::match.re!=Re)
	{
		assert(0); return 0; //wanna know
	}

	if(!out) return max(Ex_regex_thread::parse.out_x-partial,0);

	int i;
	for(i=0;i<out_s&&i+partial<Ex_regex_thread::parse.out_x;i++)
	{
		out[i] = Ex_regex_thread::parse.out[partial+i];
	}
	return i;
}

static int Ex_regex_match(EX::Regex::Case,const wchar_t*,int*,wchar_t*,int*);

static const char *Ex_regex_pattern(EX::Regex::Case,int);

const char *EX::Regex::pattern(int cp)
{
	return cp&&EX_REGEX_NONSIMPLE(Re)?Ex_regex_pattern(Re,cp):0;
}

static const wchar_t *Ex_regex_pattern(EX::Regex::Case);

const wchar_t *EX::Regex::pattern()
{
	return EX_REGEX_NONSIMPLE(Re)?Ex_regex_pattern(Re):0;
}

static const char *Ex_regex_syntax(EX::Regex::Case);

bool EX::Regex::is_printf_format_specification() 
{
	if(!EX_REGEX_NONSIMPLE(Re)) return false;

	const char *syntax = Ex_regex_syntax(Re);

	return syntax=="printf";
}

static const char *Ex_regex_printf(EX::Regex::Case);

const char *EX::Regex::printf_type_field_characters()
{
	return Re?Ex_regex_printf(Re):0;
}			 
/*unused/never used
bool EX::Regex::is_perl_compatible_regular_expression()
{
	if(!EX_REGEX_NONSIMPLE(Re)) return false; 
	const char *syntax = Ex_regex_syntax(Re); 
	return syntax=="pcre";
}*/

static bool Ex_regex_finish_re(const char* &in, int in_s)
{
	if(!in[in_s]) return true;
		
	const int out_s = 1024; if(in_s>out_s) return false; //woops!
	
	static char out[out_s+1]; //NOT THREADSAFE

	memcpy(out,in,in_s); out[in_s] = '\0'; 
	
	in = out; return true;
}

static const int *Ex_regex_simple_re(const char *in, int in_s)
{		
	if(!in) return EX::Regex::ZERO;

	if(!*in||!in_s) return EX::Regex::EMPTY;

	//assuming printf syntax
	{
		int i;
		for(i=0;i<in_s&in[i];i++) if(in[i]=='%'&&in[i+1]!='%') break;
		
		if(i==in_s) return EX::Regex::EXACT;
	}
	
	return 0; //catch all
}	

static int *Ex_regex_lookup_re(const char* &inout) 
{	
	typedef std::map<std::string,int> hash_table;  	
	//NOT THREADSAFE
	static hash_table ht; 
	static std::pair<std::string,int> temp("",0);
	temp.first = inout; 
	hash_table::value_type &vt = *ht.insert(temp).first;
	inout = vt.first.c_str(); 
	return &vt.second;
}
inline const int *Ex_regex_lookup_re
(const char *in, int in_s, const int *in_case_exact, int* (*f)(const char*&)=Ex_regex_lookup_re) 
{
	const int *out = Ex_regex_simple_re(in,in_s);
	
	if(out&&*out<=EX::Regex::SIMPLE_CASES) 
	{
		if(*out==EX::REGEX_EXACT) 
		{
			if(in_case_exact) return in_case_exact;
		}
		else return out; 
	}

	return Ex_regex_finish_re(in,in_s)?f(in):0;
}
static int *Ex_regex_prep(const char* &in);
inline const int *Ex_regex_repair_re(const char *in, int in_s, const int *in_case_exact)
{
	//NEW: rewritten so to reuse the identical code up above
	return Ex_regex_lookup_re(in,in_s,in_case_exact,Ex_regex_prep);
}

static const char *Ex_regex_decode_re(int cp, const char *in, int *in_s)
{
	if(!in) return 0; assert(in_s); //paranoia

	if(cp==65001||EX::Decode(cp,&in,in_s)==65001) return in;
	
	assert(0); return 0; //unimplemented
}				  
static const char *Ex_regex_decode_re(const wchar_t *in, int *in_s)
{		
	const char *in8 = 0; if(!in) return 0; assert(in_s); //paranoia

	if(EX::Decode(in,&in8,in_s)==65001) return in8;
	
	assert(0); return 0; //unimplemented
}

typedef struct //Ex_regex
{
	int *re, lock; //ref counter

	//unused
	unsigned born, seen, died; //EX::tick 
	//unused
	int load, work; //times used and times matched

	const char *syntax; //"printf" or "pcre"

	enum{printf_s=16};
	char printf[printf_s+1]; //eg. "cCdiouxXeEfgGaAnPsS"	

	const char *pattern; size_t pattern_s; 
	const wchar_t *widened; size_t widened_s;	 	
	const wchar_t *widen()
	{
		if(widened) return widened; 
		if(!pattern) return 0;	
		const char *u = pattern; int u_s = pattern_s;
		const wchar_t *w = 0; EX::Convert(65001,&u,&u_s,&w); 		
		widened = wcsdup(w); widened_s = wcslen(w);
		return widened;
	}		 
	void zero()
	{
		if(widened) free((void*)widened);
		memset(this,0x00,sizeof(*this));
	}

}Ex_regex;

//unallocated objects stack
static int Ex_regex_top = 0;
static int Ex_regex_stack[EX_REGEX_OBJECTS];
static int Ex_regex_pop()
{
	if(Ex_regex_top==EX_REGEX_OBJECTS) return 0;

	return Ex_regex_stack[Ex_regex_top++];
}
static void Ex_regex_push(int i) //unused
{
	if(i==0||i>EX_REGEX_OBJECTS-1) return;

	if(Ex_regex_top<=1) //paranoia
	{
		assert(0); return; //unimplemented
	}
	else Ex_regex_stack[--Ex_regex_top] = i;
}
static const int Ex_regex_zero = 65535;
static Ex_regex Ex_regex_pool[EX_REGEX_OBJECTS];
static void Ex_regex_iniatialize()
{
	if(Ex_regex_ok++) return; 
	memset(Ex_regex_pool,0x00,sizeof(Ex_regex)*EX_REGEX_OBJECTS);
	for(int i=0;i<EX_REGEX_OBJECTS;i++) Ex_regex_stack[i] = i;
	Ex_regex_top = 1;	
}
//REMOVE ME?
int Ex_regex_release_mode_recover() //hack
{
	EX_BREAKPOINT(Ex_regex_release_mode_recover)
	#ifdef _DEBUG
	assert(0); return 0; //Release mode only emergency strategy!!
	#endif
	for(int i=0;i<EX_REGEX_OBJECTS;i++)
	{
		if(Ex_regex_pool[i].re) *Ex_regex_pool[i].re = EX::REGEX_LOST;	
	}
	Ex_regex_ok = false; Ex_regex_iniatialize();
	return Ex_regex_pop();
}

static int *Ex_regex_prep(const char* &in)
{
	int *re = Ex_regex_lookup_re(in); if(!re) return 0;

	if(*re==EX::REGEX_LOST)
	{
		int c = 0; char printf[Ex_regex::printf_s] = ""; 

		for(const char *p=in;*p;p++)
		{
			if(*p=='%')
			{					
				if(p[1]=='%')
				{
					p++; //%% escape sequence
				}
				else while(*++p&&!printf[c]) switch(*p)
				{
				case 'c': //int
				case 'C': //wint_t
				case 'd': //signed
				case 'i': 					
				case 'o': //unsigned
				case 'u': 
				case 'x':
				case 'X': 					
				case 'e': //double
				case 'E':
				case 'f':
				case 'g':
				case 'G':
				case 'a':
				case 'A':					
				case 'n': //feedback
				case 'p': //void*
				case 's': //char*
				case 'S': //wchar_t*

					if(c>=Ex_regex::printf_s)
					{
						assert(0); return 0; //unimplemented
					}
					else printf[c] = *p--;					
				}

				printf[++c] = '\0';
			}
		}

		int x = Ex_regex_pop();		
		if(!x) x = Ex_regex_release_mode_recover(); //hack!! 
		if(!x) return 0;

		Ex_regex &ok = Ex_regex_pool[x]; 
		ok.zero(); ok.born = EX::tick(); 
		
		ok.syntax = "printf"; //hack
		ok.pattern = in; ok.pattern_s = strlen(in);
		strcpy_s(ok.printf,ok.printf_s,printf); 
						
		ok.re = re;	*re = Ex_regex_zero+x;			
		return re;
	}
	else if(*re>=Ex_regex_zero&&*re<Ex_regex_zero+EX_REGEX_OBJECTS)
	{
		//return Ex_regex_pool[*re-Ex_regex_zero].id==in_q?re:0;
		return Ex_regex_pool[*re-Ex_regex_zero].pattern==in?re:0;
	}
	else return 0;
}

#define EX_REGEX_OK_CASE(RE,OK,...) if(!Ex_regex_ok) return __VA_ARGS__;\
if(!RE||*RE<Ex_regex_zero||*RE>Ex_regex_zero+EX_REGEX_OBJECTS) return __VA_ARGS__;\
Ex_regex &OK = Ex_regex_pool[*RE-Ex_regex_zero]; if(OK.re!=RE) return __VA_ARGS__;

static void Ex_regex_lock(EX::Regex::Case re)
{
	EX_REGEX_OK_CASE(re,ok) ok.lock++;
}				   
static void Ex_regex_unlock(EX::Regex::Case re)
{
	EX_REGEX_OK_CASE(re,ok) ok.lock--; assert(ok.lock>=0);
}

static bool Ex_regex_matchf(const char *c, int c_s, const char *in, int in_s, char **out, int *out_s)
{
	if(!in||!c_s) return false; //paranoia

	while(c_s--) //HACK: for side by side formats, eg %6d%s
	{
		if(!c[c_s]) return false;

		char *in_x = (char*)in;

		int spec = *c++;

		switch(spec)
		{
		case 'c': //char
			
			if(in_s!=1||*out_s<1) return false;

			in_x++; break;

		case 'C': //wint_t (hack: assuming locale setup)
		
			assert(0); return false; //unimplemented

			//int mb = _mbtowc_l(0,in,in_s,l); 

			//if(mb!=in_s||*out_s<mb) return false;

			break;
		
		case 'd':
		case 'i': //signed (base 10)

			strtol(in,&in_x,10); break;

		case 'o': //unsigned (base 8)

			strtol(in,&in_x,8); break;

		case 'u': //unsigned (base 10)

			strtol(in,&in_x,10); break;

		case 'x': 
		case 'X': //unsigned (base 16)
			
			strtol(in,&in_x,16); break;
		
		case 'e':
		case 'E':
		case 'f':
		case 'g':
		case 'G':
		case 'a':
		case 'A': //double
		
			strtod(in,&in_x); break;
		
		case 'n': //feedback
			
			if(in_s) return false; break; 

		case 'p': //address (hexidecimal)
		
			strtol(in,&in_x,16); break;
		
		case 's': //string
			
			in_x+=in_s; break; //DOESN'T SEEM TO BE GOOD ENOUGH!?
		
		case 'S': //wide string

			assert(0); return false; //unimplemented
			
		case '\0': assert(0); return false; //wanna know

		default: return false; //???
		}		

		if(!in_x) return false;

		int x = int(in_x-in); if(x>in_s) return false;

		in_s-=x; //HACK: shadowing
		{
			int in_s = x; 
				
			switch(spec)
			{
			case 's': case 'c': case 'C': case 'S': 
				
				break; //take as is (eg. may be space[s])

			default: //trim

				int i = 0; while(i<in_s&&in[i]==' ') i++; in+=i; in_s-=i; //ltrim

				for(i=in_s-1;i&&in[i]==' ';i--) in_s--; //rtrim;
			}

			if(*out_s<in_s) return false; //hack??

			if(Ex_regex_thread::parse.out_x==Ex_regex_thread::parse.out_s) 
			return false;

			Ex_regex_thread::parse.out
			[Ex_regex_thread::parse.out_x++] = *out; 

			if(*out==Ex_regex_thread::match.tmp) //using internal buffer
			{
				Ex_regex_thread::match.tmp_x+=in_s+1; 
			}
			*out_s-=in_s+1; //warning! iterating in_s below

			while(in_s--) *(*out)++ = *in++;

			*(*out)++ = '\0'; //hack??
		}

		in = in_x;	
	}
	
	return true;
}
static bool Ex_regex_match
(EX::Regex::Case re, int cp, const char **in, int *in_s, char **out, int *out_s) 
{
	EX_REGEX_OK_CASE(re,ok,false) 

	if(!cp||!in||!in_s||!out||!out_s) return false; //paranoia

	ok.load++; ok.seen = EX::tick();

	if(ok.syntax=="printf") 
	{		
		/*///////////////////////////////////////////////////

		For now the format string is treated as if a wildcard
		glob, where each % sequence is reduced to a * character
		This approach is highly susceptible to gross ambiguity
		However, because human readible print is inclined toward 
		unambiguously separating terms, this is not an entirely 
		unreasonable starting point. Bearing in mind the printf 
		format is not and can never be completely reversible!
		
		/*///////////////////////////////////////////////////

		const wchar_t *wild = 0; int w = 0;
		const wchar_t *p = ok.widen(), *b = p+ok.widened_s; 
		if(!p) return false; 
			
		const char *q = *in, *d = q+*in_s; int q_s = *in_s;
		const char *q_a = 0, *q_z = 0; //match delimiters		
		
		char *c = ok.printf; int x = 0;
		wchar_t q_wc = 0; int q_mb = 0;

		//this implementation greatly simplifies this
		int wctmb = cp==65001?0:WC_NO_BEST_FIT_CHARS;

		#define P_WILD(X)\
		if(*p=='%'||!*p) if(!*p||p[1]!='%')\
		{\
			if(q_z&&!Ex_regex_matchf(c,x,q_a,int(q_z-q_a),out,out_s))\
			goto mismatch;\
			q_z = 0; if(!*p) goto match;\
			c+=x; x = 0;\
x##X:		if(c[x]=='\0'||w+x>=ok.printf_s) goto mismatch;\
			while(p!=b&&*p!=c[x]) p++;\
			if(p==b||*p=='\0') goto mismatch;\
			x++; p++;\
			if(*p=='%') if(p[1]=='%') p++; else goto x##X;\
			q_a = q-q_mb; wild = p; w++;\
		}\
		else p++; /*double %% escape sequence*/
		P_WILD(0)			
		#define Q_TO_WC /*q_mb=_mbtowc_l*/\
		if((MultiByteToWideChar(cp,0,q,q_s,&(q_wc=0),1)||q_wc)\
		&&(q_mb=WideCharToMultiByte(cp,wctmb,&q_wc,1,0,0,0,0)))\
		{ q+=q_mb; q_s-=q_mb; }else q_mb = 0; 
		Q_TO_WC
		while(p!=b&&q-q_mb<d&&q_wc!='\0')
		{
			if(*p==q_wc)
			{									
				Q_TO_WC p++; P_WILD(1)
			}
			else if(wild)
			{					
				//q_mb: just in case of conversion error
				while(q_mb&&*q&&q<d&&q_wc!=*wild) Q_TO_WC 

				if(q_wc==*wild)
				{
					p = wild; if(q_a) q_z = q-q_mb; 
				}
				else goto mismatch;				
			}
			else goto mismatch;
		}

		if(q_a&&p==b)
		{
			if(!Ex_regex_matchf(c,x,q_a,int(d-q_a),out,out_s)) goto mismatch;			
		}
		else if(q_a) goto mismatch;

		if(wild&&(!*wild||wild==b)) goto match; 

		if((!*q||q==d)&&(!*p||p==b)) goto match;

#undef Q_TO_WC
#undef P_WILD
#undef PARSE

		goto mismatch;
	}
	else 
	{
		assert(0); return 0; //unimplemented
	}

match: *in_s = 0; //hack

	ok.work++; return true;

mismatch: return false;
}		

static const char *Ex_regex_pattern(EX::Regex::Case re, int cp)
{
	EX_REGEX_OK_CASE(re,ok,0) 

	if(cp!=65001) //TODO: EX::Encode()
	{
		assert(0); return 0; //unimplemented
	}
	else return ok.pattern;
}			 
static const wchar_t *Ex_regex_pattern(EX::Regex::Case re)
{
	EX_REGEX_OK_CASE(re,ok,0) return ok.widen();
}						 
static const char *Ex_regex_syntax(EX::Regex::Case re)
{
	EX_REGEX_OK_CASE(re,ok,0) return ok.syntax;
}					   
static const char *Ex_regex_printf(EX::Regex::Case re)
{
	EX_REGEX_OK_CASE(re,ok,0) return *ok.printf?ok.printf:0;
}				 
static bool Ex_regex_exact(int cp, const char **in, int *in_s, char **out, int *out_s)
{
	if(!cp||!in||!in_s||!out||!out_s) return false; //paranoia

	if(!Ex_regex_thread::exact.cp) return false;
	
	if(Ex_regex_thread::exact.cp==cp)
	{
		if(Ex_regex_thread::exact.in_s!=*in_s) return false;

		if(!memcmp(in,Ex_regex_thread::exact.in,*in_s))
		{
			if(*out_s<*in_s+1) //partial match??
			{
				memcpy(out,in,*out_s); out[*out_s] = '\0';

				*in_s-=*out_s; return false;
			}

			memcpy(out,in,*in_s); //match
			
			out[*in_s] = out[*in_s+1] = '\0';

			*out_s=*in_s; return true;
		}
		else return false;
	}
	else assert(0); //unimplemented

	return false; //catch all	
}	   
static bool Ex_regex_white(int cp, const char **in, int *in_s, char **out, int *out_s)
{
	if(!cp||!in||!in_s||!out||!out_s) return false; //paranoia

	assert(0); return false; //unimplmented
}