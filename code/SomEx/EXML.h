							
#ifndef EXML_INCLUDED
#define EXML_INCLUDED

namespace EXML //experimental
{				 
	//note: EXML will be case sensitive, like XML. Here
	//we use lowercase always to avoid confusion and so
	//user/provisional tags and attributes can use caps

	//EX_ENUMSPACE allows these to overlap
	struct Tags{enum Type
	{
	_0=0,_1,exml, //1 is an unrecognized tag
	}
	EX_ENUMSPACE(Tags,Type)};
	struct Tokens{enum Type
	{
	_0=0,_1, //0 indicates value only, 1 boolean (sans =)
	}
	EX_ENUMSPACE(Tokens,Type)};
	struct Attribs{enum Type
	{
	_0=0,_1, //1 is an unrecognized attribute	
	#define EXML_ATTRIBS \
	alt,c,cc,hl,il,l,lh,li,tc,tm,
	EXML_ATTRIBS
	_,_s, //these indicate parsing status (see tag below)	
	}
	EX_ENUMSPACE(Attribs,Type)};

	struct Attrib
	{ 
		inline operator bool(){ return key; }
		inline bool operator!(){ return !key; }
		inline bool operator~(){ return key==Attribs::_; }
		Attribs key; Tags tag; Tokens tok; const char *val, *keyname; 			
	};
	template<size_t N> struct Attribuf
	{
		static const size_t N = N; 
		
		Attrib buf[N]; const Attrib terminator; 

		typedef Attrib AttribN[N]; 
		inline operator AttribN&(){ return buf; }
		inline Attrib *operator->(){ return buf; }
		inline size_t operator-(const char *start)
		{
			return buf->val-start;
		}
		void poscharwindow(const char *pos, size_t lim) 
		{
			((Attrib&)terminator).val = pos+lim;
			buf->val = pos; buf->tag = (Tags)0;
			buf->key = Attribs::_;
			buf[1].key = (Attribs)0;
		}
		inline const char *charpos()
		{
			return buf->val; 
		}
		inline size_t charlim()
		{
			return terminator.val-buf->val;
		}		
		Attribuf(const char *str=0, size_t str_s=0)
		{
			int compile[N>=2];
			((Attrib&)terminator).key = (Attribs)0;
			poscharwindow(str,str_s);
		}				
		template<int str_s>
		inline Attribuf(const char (&str)[str_s])
		{
			new(this)Attribuf<N>(str,str_s);
		}		
		inline Attribuf(const char *str, const char *str_s)
		{
			new(this)Attribuf<N>(str,str_s-str);
		}		
	};
	//The first attribute is always either _ or _s and its val
	//contains the position after the scan. If _s tag is stuck
	//If _ tag is mid-tag, probably at the >. If at the end of
	//the tag its tag will be 0 and if mid tag contain the tag
	//when resuming tag will hand back the first tag of Attrib	
	//its keyname is set to the tag's name after the opening <
	//its token is 1 if at the closing >. tag happily skips ><
	//so that it isn't necessary to adjust between consecutive
	//tags, however tag halts (returning 0) on any other input
	//Note: to tag there isn't much difference between opening
	//and closing tags. You must watch for the / in the output
	//ATTN: IF STUCK AT _s WITH 0 ATTRIBUTES tag WILL RETURN 0
	extern Tags tag(const char *_, size_t _s, EXML::Attrib*,size_t N);
	template<int N>
	inline Tags tag(const char *_, size_t _s, EXML::Attrib (&attr)[N])
	{
		return tag(_,_s,attr,N);
	}
	template<int N> inline Tags tag(EXML::Attribuf<N> &attr)
	{
		return tag(attr.charpos(),attr.charlim(),attr,attr.N);
	}

	Attribs attribs(const char *keyname, size_t keyname_s=-1);

	struct Quote //unquote values
	{
		const char *value, *delim; 
		inline size_t length(){ return delim-value; }
	
		Quote(const char *val) 
		{
			value = delim = val;

			if(val&&*val) if(*val=='\''||*val=='"') 
			{
				delim++; while(*delim&&*delim!=*val) delim++; value++;
			}
			else while(*delim&&*delim!='>'&&!isspace(*delim)) delim++;
		}
		inline Quote(Attrib &val){ new(this)Quote(val.val); }
		inline operator const char*(){ return (char*)value; }
				
		//hairy alternative to Token below
		inline void unquote(char &sentinel)
		{
			if(delim) std::swap((char&)*delim,sentinel); 
		}
		template<typename T> inline Quote(char &sentinel, T t)
		{
			new(this)Quote(t); unquote(sentinel);
		}

		Quote(){} 
		inline void operator=(const char *val){ new(this)Quote(val); }
	};
	struct Quoted : Quote
	{
		Quoted(){}
		inline void operator=(const char *val){ new(this)Quoted(val); }

		Quoted(const char *val) : Quote(val)
		{
			if(val) if(*delim=='"'||*delim=='\''){ value--; delim++; }
		}
		template<typename T> inline Quoted(char &sentinel, T t)
		{
			new(this)Quoted(t); unquote(sentinel);
		}
	};
	struct Token : Quote //unquote with 0 terminator
	{
		Token(Attrib &val) : Quote(val){ (char&)*delim = '\0'; }
		Token(const char *val) : Quote(val){ (char&)*delim = '\0'; }		
		inline operator char*(){ return (char*)value; }
	};
}				

#endif //EXML_INCLUDED