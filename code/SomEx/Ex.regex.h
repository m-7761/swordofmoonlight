
#ifndef EX_REGEX_INCLUDED
#define EX_REGEX_INCLUDED

#undef ERROR //wingdi.h
static const int ERROR = 0; 

namespace EX
{
	static const int REGEX_ZERO   = -0x1;
	static const int REGEX_LOST   = 0x00;
	static const int REGEX_EXACT  = 0x01;
	static const int REGEX_IFANY  = 0x02;
	static const int REGEX_ALWAYS = 0x03;
	static const int REGEX_EMPTY  = 0x04;
	static const int REGEX_NEVER  = 0x05;
	static const int REGEX_STRING = 0x06;
	static const int REGEX_WHITE  = 0x07;
	static const int REGEX_ERROR  = 0xF0;
	static const int REGEX_VALID  = 0xF1;	

	//REMOVE ME?
	extern int Regex_is_needed_to_initialize(); 
}

namespace EX{
//Regex: I feel like this is kind of a mess
//Do not try to keep instances of this object
//outside of any single scope. You can keep track
//of a shared Regex identifier via a const int
//pointer. If the int pointed to becomes zero
//then the regex is lost and must be rebuilt via
//one of the constructors. This will not happen
//within any given scope, ie. until destruction 
//occurs. Construction of global Regex objects 
//other than for simple cases, eg. Regex::EXACT, 
//will not result in a meaningful Regex object
//USAGE:
//Never make a reference (pointer) of a Regex!	
struct Regex //EX
{
private: const int *Re;

	void lock(const int*)const;
	void unlock(const int*)const;
		  
public: typedef const int *Case;

	static Case ZERO;   //-1: matches 0 pointer
	static Case LOST;   //00: reconstruction required
	static Case EXACT;  //01: matches user pattern exactly
	static Case IFANY;  //02: matches nonzero sized strings
	static Case ALWAYS; //03: always matches no matter what
	static Case EMPTY;  //04: matches zero sized strings
	static Case NEVER;  //05: never matches (ever)
	static Case STRING; //06: matches nonzero pointer
	enum{SIMPLE_CASES=7};
	static Case WHITE;  //07: explodes on whitespace boundaries
	static Case ERROR;  //F0: matches bad method parameters
	static Case VALID;  //F1: matches well-encoded input	
					 
	inline Regex(const int *in)
	{
		Re = in; if(Re&&*Re>SIMPLE_CASES) lock(Re); 
	}
	inline Regex(const Regex &cp)
	{
		Re = cp.Re; if(Re&&*Re>SIMPLE_CASES) lock(Re); 
	}

	Regex(const wchar_t *id, int id_s, const int *in_case_exact=0);	
	Regex(int cp, const char *id, int id_s, const int *in_case_exact=0);

	inline Regex(const wchar_t *id, const int *in_case_exact = 0)
	{
		new(this)Regex(id,wcslen(id),in_case_exact);
	}
	inline Regex(int cp, const char *id, const int *in_case_exact=0)
	{
		new(this)Regex(cp,id,strlen(id),in_case_exact);
	}

	inline ~Regex(){ if(Re&&*Re>SIMPLE_CASES) unlock(Re); }

	inline operator const int*(){ return Re; }

	inline int operator*()const{ return Re?*Re:0; }

	inline bool operator==(const int *in)const
	{ 
		return Re&&in?*Re==*in:!Re&&!in; 
	}
	inline bool operator!=(const int *in)const
	{ 
		return Re&&in?*Re!=*in:!(!Re&&!in); 
	}

	//NOTE THE wchar_t versions are not implemented//
	//Reminder: probably the easiest way to do that//
	//is to make them inline and pass 0 as their cp//

	//reset: should call before exact/match()
	//reset the entire exact/match/parse/place/fetch chain 
	//NOTE: match() resets itself if false or in_s reaches zero
	void reset();

	//exact: can call before match()
	//If Regex is of case EXACT match() will match against in/in_s only
	//Contents of in should remain in place until match() is no longer required
	void exact(int cp, const char *in, int in_s=0); 
	void exact(const wchar_t *in,int=0);

	  ////////////////////////////
	 //match->parse->place->fetch
	//
	//match/error: 
	//error() can be called instead of match to get regular expression
	//processing error output in human readable format. To get EX::Regex
	//error information use EX::Regex(EX::Regex::ERROR) with match()
	//The parameters work like iconv; in/out address a single pointer
	//match/error should be called multiple times until *in_s is zero
	//or false is returned
	bool match(int cp, const char **in, int *in_s, char **out=0, int *out_s=0);	
	bool match(const wchar_t **in,int*,wchar_t**_=0,int*_s=0);
	bool error(int cp, const char **in, int *in_s, char **out=0, int *out_s=0);	
	bool error(const wchar_t **in,int*,wchar_t**_=0,int*_s=0);

	//parse: can call after match()
	//out is a pointer to an array of pointers of count out_s. each out pointer
	//set will correspond to a 0-terminated string written to the match() out
	//buffer(s). The number of out pointers set is returned. partial if set 
	//will begin addressing partial number of strings into the match() out 
	//buffers. If out is 0, parse will return the total number of strings
	//available for addressing
	int parse(const char **out=0, int out_s=0, int partial=0); 
	int parse(const wchar_t **out,int,int=0); 

	//place: can call after match()
	//in is a pointer to an array of pointers of count in_x. in_s is the length
	//of the strings pointed to by in. place replaces the strings returned by
	//parse with the strings provided by in. partial can be set to offset into 
	//the number of parsed/replaceable strings. the number of strings replaced
	//is returned.
	int place(int cp, const char **in, int *in_s, int in_x, int partial=0); 
	int place(const wchar_t **in,int*,int,int=0);

	//fetch: can call after match/place()
	//fetch writes the original pattern to out with the parsed portions replaced
	//by the strings provided by place(). false is returned when finished
	bool fetch(int cp, char **out, int *out_s);	
	bool fetch(wchar_t **out,int*);

	inline bool match(int cp, const char *in, int in_s=0) //simple match
	{
		if(!in_s&&in&&*in) in_s = strlen(in); 
		
		bool out = match(cp,&in,&in_s);
		while(out&&in_s) out = match(cp,&in,&in_s);
		return out;
	}
	inline bool match(const wchar_t *in, int in_s=0) 
	{
		if(!in_s&&in&&*in) in_s = wcslen(in); 		
		bool out = match(&in,&in_s); while(out&&in_s) out = match(&in,&in_s);
		return out;
	}
	inline const char *fetch(int cp) //simple fetch
	{
		char *out = 0; while(fetch(cp,&out,0)); return out;
	}
	inline const wchar_t *fetch() 
	{
		wchar_t *out = 0; while(fetch(&out,0)); return out;
	}

	const char *pattern(int cp); 
	const wchar_t *pattern(); 

	bool is_printf_format_specification();

	//eg. "cCdiouxXeEfgGaAnPsS"
	const char *printf_type_field_characters();	 	

	//unused: currently only printf is required
	//bool is_perl_compatible_regular_expression();
};}

#endif EX_REGEX_INCLUDED