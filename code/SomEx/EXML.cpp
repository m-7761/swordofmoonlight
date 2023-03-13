																		   
#include "Ex.h" 
EX_TRANSLATION_UNIT

#include <string>
#include <vector>
#include <algorithm>

#include "EXML.h" //experimental

extern EXML::Tags EXML::tag(const char *_, size_t _s, EXML::Attrib *out, size_t out_s)
{
	if(out_s<2||!_) 
	{
		if(out_s) out->key = Attribs::_0; return Tags::_0;
	}
		
	Tags tag = out->tag; out->key = Attribs::_s; //assuming

	//TODO: REWRITE SO _ IS INCREMENTED IN ORDER TO EASE DEBUGGING
	
	size_t i = 0, at = 1; out[1].key = Attribs::_0; //in case of error

	switch(_s?*_:0) 
	{
	case '>': //courtesy
		 
		if(_s<2||_[1]!='<') //text
		{
			out->tok = Tokens::_0;
			out->keyname = _;	out->val = _+1;
			return out->tag = Tags::_0;
		}
		_++; _s--; //break;

	case '<': 
			
		tag = Tags::_1; //unknown
		out->val = _; out->keyname = _+1; out->tok = Tokens::_0;
		for(i=1;i<_s&&_[i]&&_[i]!='/'&&_[i]!='>'&&!isspace(_[i]);i++);
		if(i>=_s||!_[i]) return Tags::_0;

		out->key = Attribs::_;

		if(_[1]=='/') //assuming closing
		{
			_++; _s--;
		}

		switch(_[1]) //toy implementation
		{
		case 'e':
			
			if(i>=5&&!strncmp(_+1,"exml",4))
			{
				if(i>5) //quirk (exml=Title)
				if(_[5]=='=') i = 5; else break;
				tag = Tags::exml;
			}
			break;
		}
		//break;

	default: //mid-tag?
		
		if(!tag) //untagged text?
		{
			//_: better than nothing!!
			out->keyname = _;	break; 
		}
				
		while(i<_s&&isspace(_[i])) i++;
		for(size_t ii=i;i<_s&&_[i]!='>';ii=i)
		{				
			//FORBIDDING SPACE AROUND =
			while(i<_s&&_[i]!='='&&_[i]!='>'&&!isspace(_[i])) i++;
			if(i>=_s){ i = ii; break; }

			out[at].tag = tag;
			out[at].keyname = _+ii;
			out[at].key = Attribs::_1; //unknown

			switch(_[ii])
			{			
			case '=': //quirk: <exml=example>
				
				out[at].key = Attribs::l; break;
			
			default: //quirk: <exml 20-1> 
				
				if(tag==Tags::exml)
				if(_[ii]>='0'&&_[ii]<='9')
				{					
					//treated as boolean below
					out[at].key = Attribs::li; 					
					if(_[i]!='=') break;					
					//permit li=l construction
					out[at].tok = Tokens::_1;
					out[at].val = _+ii;
					goto li_l;					
				}
				
				out[at].key = EXML::attribs(_+ii,i-ii);
			}
					
			bool empty = false;

			if(_[i]!='=') 
			{
				//no = sign 
				out[at].tok = Tokens::_1;
				out[at].val = _+ii;
			}
			else if(i+1<_s)	//= sign
			{
				out[at].val = _+i+1; 
				out[at].tok = Tokens::_0;				
				char quote = *out[at].val;
				if(quote!='\''&&quote!='"')
				{
					while(i<_s&&_[i]!='>'&&!isspace(_[i])) i++;
					empty = out[at].val>=_+i-(_[i]!='>'); //-1
				}
				else //match closing quote
				{
					i++; while(++i<_s&&_[i]!=quote); 					
					if(i<_s&&_[i]!=quote) break; i++;
					empty = out[at].val>=_+i-2;
				}

				//TODO: CONVERT VALUE INTO A KNOWN TOKEN
			}
			else i++; if(i>=_s)
			{	
				i = ii;				
				out->key = Attribs::_s; 
				break;			
			}
			else out->key = Attribs::_;
			
			while(i<_s&&isspace(_[i])) i++;

			if(i<_s-1&&_[i]=='/'&&_[i+1]=='>') i++;

			if(!empty) li_l: //li_l: quirk
			{
				if(at>=out_s-1)
				{
					at++; break; //buffer is full
				}
				else out[++at].key = Attribs::_0;
			}
		}		 
		//catch all 0 terminator
		if(at<out_s) out[at].key = Attribs::_0; 
	}			

	out->tag = _[i]=='>'?Tags::_0:tag;	
	out->tok = Tokens(out->tag?0:_[i]=='>'); 	
	out->val = _+i; 
	//_0: courtesy since this pattern indicates stuck
	if(at==1&&out->key==Attribs::_s) return Tags::_0;
	return tag;
}
extern EXML::Attribs EXML::attribs(const char *keyname, size_t keyname_s)
{
	std::vector<std::string>::iterator found;
	static std::vector<std::string> av;	if(av.empty())		
	for(const char *p=EX_CSTRING(EXML_ATTRIBS),*pp=p;*p;p++)
	if(*p==','){ av.push_back(""); av.back().assign(pp,p-pp); pp=p+1; }		

	if(keyname_s!=size_t(-1))
	{
		static std::string tmp; tmp.assign(keyname,keyname_s);		
		found = std::find(av.begin(),av.end(),tmp.c_str());
	}
	else found = std::find(av.begin(),av.end(),keyname);
	
	if(found!=av.end()) 
	return Attribs(found-av.begin()+2);
	return Attribs::_1;
}
