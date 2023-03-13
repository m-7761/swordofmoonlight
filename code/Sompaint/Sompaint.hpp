
#include <map>
#include <string>

//What's this doing here?
//
//This file has an inline preprocessor 
//to aid in parsing the various printf
//like methods. It's primarily for use
//by modules that implement the server 
//specification but may have more uses

#ifndef SOMPAINT_HPP_MAX
#define SOMPAINT_HPP_MAX 16
#endif

#ifndef SOMPAINT_HPP
#define SOMPAINT_HPP Sompaint_hpp
#endif

namespace SOMPAINT_HPP
{		
	 //_caution: all of this is subject to change_
	//Module maintainers will just have to keep up 
	//with Sompaint.hpp or keep an old copy around

	enum keyword
	{		
	UNSPECIFIED=-1, 
	UNRECOGNIZED=0,

	UNSPEC=-1, UNREC=0, //shorthand
	
	//these can overlap; just comment out extra occurences
	//Reminder: remember to add the tokens to coin (below)

	buffer, memory, raster, window, //format::spec		
		
	basic, frame, index, point, state, macro, //format::left
		
	_1d, _2d, ascii, blend, canvas, clear, depth, front, //format::lvalue
	
		mipmap, rgb, rgba, stencil, x, z, //ditto 
		
	_0, aaa, back, bgr, both, decal, mdl, mdo, range, reset, //format::rvalue

	_ellipsis, //format::_keyword

	#ifdef SOMPAINT_NONSTANDARD_KEYWORDS
			SOMPAINT_NONSTANDARD_KEYWORDS
	#undef SOMPAINT_NONSTANDARD_KEYWORDS
	#endif
	};

	typedef std::map<std::string,keyword>::iterator keyword_it;

	//format::key
	//bool return is just to simplify static initialization
	bool coin(std::map<std::string,keyword> &e, bool (*f)(keyword)=0)
	{
	#define C(a) if(!f||f(a)) e[#a] = a;
	#define C_(a) if(!f||f(_##a)) e[#a] = _##a;
	#define C2(b,a) if(!f||f(_##a)) e[#b] = _##a;

		C(buffer) C(memory) C(raster) C(window)
		C(basic) C(frame) C(index) C(point) C(state) C(macro) 
		C_(1d) C_(2d) C(ascii) C(blend) C(canvas) C(clear) C(depth) C(front)
		 C(mipmap) C(rgb) C(rgba) C(stencil) C(x) C(z)
		C_(0) C(aaa) C(back) C(bgr) C(both) C(decal) C(mdl) C(mdo) C(range) C(reset)
		C2(...,ellipsis)

	#undef C
	#undef C_
	#undef C2

		return true;
	}

	struct _rinput
	{			
		int datatype; //enum
		
		_rinput(){ datatype = 0; }

		template<int N> struct _{ char __[N]; };

		union
		{
		_<1> _1; _<2> _2; _<4> _4; _<8> _8;

		char c, *cp; int i, *ip; bool b, *bp; double d, *dp; float *fp; //f 

		const void *data;
		};		
		
		size_t datasize()
		{
			switch(datatype) //may want to optimize 
			{
			case 'c': return 1; default: assert(0); return 0;

			case 'i': return sizeof(int); case 'b': return sizeof(bool);

			case 'd': return sizeof(double); //case 'f': return sizeof(float);

			case 'cp': case 'dp': case 'fp': case 'ip': case 'bp': return sizeof(void*);

			case '_1': return 1; case '_2': return 2; case '_4': return 4; case '_8': return 8;
			}
		}

		inline bool datatype_is(int t)
		{
			if(datatype==t) return true; assert(0); return false;
		}

		inline operator int*(){	return datatype_is('ip')?ip:0; }
		inline operator bool*(){ return datatype_is('bp')?bp:0; }
		inline operator float*(){ return datatype_is('fp')?fp:0; }
		inline operator double*(){ return datatype_is('dp')?dp:0; }
		inline operator char*(){ return datatype_is('cp')?cp:0; }

		template<typename n> operator n() 
		{
			switch(datatype)
			{			
			case 'i': return i;	
			case 'd': return d;		 
			case 'c': return c;
			case 'cp': return !*cp?0:strtod(cp,0);

			default: assert(0); return -1; 
			}
		}	
		 		
		inline void empty()
		{
			datatype = 'cp'; data = "";
		}
		template<int N>	inline bool operator==(const char (&cmp)[N])
		{
			return datatype=='cp'?!strcmp(cp,cmp):false;
		}
		template<> inline bool operator==(const char (&cmp)[1]) //""
		{
			return datatype=='cp'?*cmp==*cp:false;
		}
		template<int N>	inline bool operator!=(const char (&cmp)[N])
		{
			return datatype=='cp'?strcmp(cp,cmp):true;
		}
		template<> inline bool operator!=(const char (&cmp)[1]) //""
		{
			return datatype=='cp'?*cmp!=*cp:true;
		}

		int write(char *dst, size_t dst_s)
		{
			switch(datatype)
			{
			case 'i': return sprintf_s(dst,dst_s,"%d",i);
			case 'd': return sprintf_s(dst,dst_s,"%f",d);
			case 'c': return sprintf_s(dst,dst_s,"%c",c);
			case 'cp': return sprintf_s(dst,dst_s,"%s",cp);

			default: assert(0); return 0;
			}
		}
	};
		
	struct _lexer : public _rinput 
	{	
		const char *type; //% 
						
		int width; int precision;		
		
		const char *fstring, *rstring; //recursion
		
		inline size_t type_s(){ return fstring-type; }
				
		union{ va_list arguments; const void **varguments; };				

		char identifier[SOMPAINT_HPP_MAX]; size_t varguments_s;
				
		_lexer(const char *f, va_list v) : syntax_errors(0)
		{
			fstring = f?f:""; rstring = 0; arguments = v; varguments_s = 0;
		}
		_lexer(const char *f, const void **v, size_t v_s) : syntax_errors(0)
		{
			fstring = f?f:""; rstring = 0; varguments = v; varguments_s = v_s;
		}

		inline bool digit(size_t i)
		{
			return fstring[i]>='0'&&fstring[i]<='9';
		}		
		inline bool white(size_t i) //trimmed
		{
			return fstring[i]==' '; //'\n'||'\t'||'\r';
		}
		inline bool space(size_t i) //seperators
		{
			switch(fstring[i]) //TODO: customizable lookup string 
			{
			case '%': case ' ': case 0: //white(i)
			case ':': case ';': case ',': case '=': case '(': case ')': return true; 
			}
			return false;
		}
		inline size_t print(size_t i) //after the % character
		{	
			if(digit(i)) //width
			{
				width = atoi(fstring+i++); while(digit(i)) i++;
			}
			else if(fstring[i]=='*'&&++i)
			{
				if(arguments)
				{
					if(varguments_s)
					{
						width = (int)*varguments++; 
						
						if(!--varguments_s) arguments = 0;
					}
					else width = va_arg(arguments,int); 
				}
				else assert(0);
			}  	
			else width = 0;

			if(fstring[i]=='.') //precision
			{
				if(digit(++i))
				{
					precision = atoi(fstring+i++); while(digit(i)) i++;
				}
				else if(fstring[i]=='*'&&++i)
				{
					if(arguments)
					{
						if(varguments_s)
						{
							precision = (int)*varguments++;
							
							if(!--varguments_s) varguments = 0;
						}
						else precision = va_arg(arguments,int);	
					}
					else assert(0);
				}
			}
			else precision = 0;

			return i;
		}
		  
		//treats % as printf-like directives
		//trims white space down to a single seperator
		//pulls out alpha numeric identifiers; returns seperator
		inline char operator()(int(*types)(const char*,_rinput&)=0)
		{
			datatype = 0; 
			identifier[0] = '\0';

			int i = -1, j = 0;  

		r:	//recursion recovery point

			while(fstring[++i]) if(!space(i))
			{
				if(j<sizeof(identifier)-1) 
				{
					identifier[j++] = fstring[i];
				}
				else assert(0);
			}
			else //important
			{
				switch(fstring[i])
				{
				case '%': //printf
					
					//raw string?
					if(rstring||!types) break; 

					if(fstring[i+1]!='%')
					{
						i = print(i+1); //width etc.
						   						   
						i+=types(type=fstring+i,*this);
																		
						if(arguments)
						{
							if(varguments_s)
							{
								data = *varguments++; 
								
								if(!--varguments_s) varguments = 0;
							}
							else
							{
								switch(datasize())
								{
								case 1: _1 = va_arg(arguments,_<1>); break;
								case 2: _2 = va_arg(arguments,_<2>); break; 
								case 4: _4 = va_arg(arguments,_<4>); break;
								case 8: _8 = va_arg(arguments,_<8>); break;

								default: data = 0; assert(0);
								}

								switch(datatype) //substitution
								{
								default: //assuming numerical argument
								{
									if(!*identifier&&space(i)) break; //optimized case
									
									int wr = write(identifier+j,sizeof(identifier)-1-j);

									identifier[j+wr] = '\0'; datatype = 0; //fall thru

									assert(0); //untested
								}
								case 'cp': rstring = fstring+i; //assert(0); //untested
									
									fstring = datatype=='cp'?cp:identifier+j; 

									i = -1; assert(fstring); continue;
								}
							}
						}
						else assert(0);
					}
					else fstring++; break;

				case '\r': case '\n': case '\t': assert(0); break;
				case ' ': 

					if(!i) //left trim
					{
						fstring++; i--; continue; 
					}
					else while(white(i+1)) 
					{
						fstring++; //right trim
					}

				break;
				}	  

				break; //seperator
			}

			char out = *(fstring+=i);

			if(!out&&rstring) //recursion
			{
				fstring = rstring; rstring = 0; i = -1; goto r;
			}

			if(white(0)&&space(1))
			{
				if(*++fstring!='%'||fstring[1]=='%') 
				{
					out = *fstring; if(out) fstring++; 
				}
			}
			else if(out) fstring++;
					   			
			identifier[j] = '\0';	

			if(!datatype)
			{
				data = identifier; datatype = 'cp'; 
				
				type = ""; type = fstring;
			}

			return out;
		}	   
		inline char operator*() 
		{
			return *fstring; 
		}	  	

		int syntax_errors;
		
		inline void error_if(bool test)
		{
			if(test) syntax_errors++; assert(!test);
		}
		inline void error()
		{
			error_if(0);
		}	    
	};
		
	#ifndef SOMPAINT_LOADF		
	inline int loadf(const char *t, _rinput &i)
	{			
		bool L = *t=='L'; 

		switch(t[L?1:0])
		{
		case 'f': i.datatype = L?'dp':'fp'; return L?2:1;
		case 'i': i.datatype = L?'?p':'ip'; return L?0:1;
		case 'b': i.datatype = L?'ip':'bp'; return L?2:1;

		default: assert(0); return 0;
		}
	}
	#else
			SOMPAINT_LOADF			
	#undef SOMPAINT_LOADF 	
	#endif

	//SOMPAINT::load parser
	struct load : public _lexer 
	{	   
		load(const char *f, va_list v) : _lexer(f,v){ }

		load(const char *f, const void **v, size_t v_s) : _lexer(f,v,v_s){ }

		bool operator++(){ return (*this)(loadf); } //lexer
	};	 

	#ifndef SOMPAINT_PRINTF
	inline int printf(const char *t, _rinput &i)
	{
		switch(*t)
		{
		case 'd': 
		case 'i': i.datatype = 'i'; return 1;
		case 'e':
		case 'f':		
		case 'g': i.datatype = 'd'; return 1;
		case 'c': i.datatype = 'c'; return 1;			
		case 's': i.datatype = 'cp'; return 1;

		default: assert(0); return 0;
		}
	}
	#else
			SOMPAINT_PRINTF
	#undef SOMPAINT_PRINTF
	#endif
		
	//SOMPAINT::format parser
	struct format : public _lexer
	{
		keyword left, spec; //Ex. basic buffer:

		keyword lvalue, rvalue, _keyword; //Ex. blend = mdo(1) 
				
		inline operator keyword()const{ return _keyword; }

		static std::map<std::string,keyword> key; //coin 
		
		format(const char *f, va_list v) : _lexer(f,v)
		{ 
			left = lvalue = rvalue = UNREC; spec = UNSPEC; scope = 1;

			if(f) while(*f) if(*f++==':') scope = 0; //hack?? 
		}		

		void empty(){ _lexer::empty(); _keyword = UNREC; }		

		bool closure(); //true if the final function parameter

		inline bool operator++(){ return closure(); }
				
		int scope; //private:
	};

	std::map<std::string,keyword> format::key;

	bool format::closure() //format parser
	{	
		if(scope==1) //hack??
		{
			lvalue = rvalue = UNREC; 
		}

		while(*fstring)
		{
			char sep = (*this)(printf); //lexer
					 
			if(*identifier)
			{
				keyword_it it = key.find(identifier);

				_keyword = it==key.end()?UNREC:it->second;
			}
			else _keyword = UNREC;

			switch(scope)
			{
			case 0: //left of colon
			{					
				switch(sep)
				{
				case ' ': error_if(left!=UNREC);
					
					left = _keyword; break;

				case ':': error_if(left==UNREC); 
					
					spec = _keyword; break;

				default: error();
				}

				if(spec==UNSPEC) continue;
				
				scope = 1; return closure();				

			}break;
			case 1: //right of colon
			{			
				if(*identifier)
				if(lvalue!=UNREC)
				{
					error_if(rvalue!=UNREC); 

					rvalue = _keyword; //specialization
				}
				else lvalue = rvalue = _keyword; 
						   
				switch(sep)
				{
				case '=': error_if(lvalue==UNREC);
					
					rvalue = UNREC; break;

				case '(': error_if(rvalue==UNREC); 
					
					scope = 2; break; //function

				case ',': case ';':	case '\0': 
					
					if(rvalue==UNREC) break; 
										
					empty(); //no arguments

					return true; //closure

				default: error();
				}

			}break;
			case 2: //inside function
			{
				switch(sep)
				{
				case ',': return false;

				case ')': scope = 1; return true;

				default: error();
				}

			}break;
			default: error();
			}
		}		 

		return false;
	}
}