			  
#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

#include "Ex.output.h"
#include "Ex.memory.h"

//shared with Ex.output.cpp for now
extern int Ex_memory_depth = 0; //static
extern int Ex_memory_align = 4; //static

static const int Ex_memory_maxdepth = 7;
static const int Ex_memory_maxundos = 7;

struct //circular
{
	long long Ex_memory_pointers[8];
	EX::Section *Ex_memory_sections[8];
	void *Ex_memory_entrypoints[8];

}Ex_memory_memory[Ex_memory_maxundos];

static int Ex_memory_undo = 0;
static int Ex_memory_redo = 0;

//shared with Ex.output.cpp for now
extern long long Ex_memory_pointers[8] = {0,0,0,0,0,0,0,0}; //static
//shared with Ex.output.cpp for now
extern const EX::Section *Ex_memory_sections[8] = {0,0,0,0,0,0,0,0}; //static
//shared with Ex.output.cpp for now
extern void *Ex_memory_entrypoints[8] = {0,0,0,0,0,0,0,0}; //static

static const char *Ex_memory_camp_(const char *);
static const char *Ex_memory_find_(const char *);
static const char *Ex_memory_goto_(const char *, void *base=0);
static const char *Ex_memory_home_(const char *);
static const char *Ex_memory_redo_();
static const char *Ex_memory_undo_();

static char Ex_memory_found[32] = ""; //camp/find

static void *Ex_memory_camp = (void*)0x400000;
static void *Ex_memory_home = (void*)0x400000;

extern EX::Section *Ex_memory = 0; 
extern EX::Section *Ex_memory_nul = 0;

extern const wchar_t *EX::name_of_memory(const EX::Section *in)
{
	if(in->name) return in->name;

	static wchar_t out[64];

	_ltow(in->number,out,10); 
	
	int len = wcslen(out); out[len++] = ':';

	_ltow((DWORD)in->info.BaseAddress,out+len,16); 
		
	return out;
}

extern void EX::code_view_of_memory(const EX::Program *in)
{
	if(in&&in->text) in->text->code = in;
}

extern void EX::data_view_of_memory(const EX::Section *in)
{
	if(in) in->code = 0;
}

extern void EX::move_view_of_memory(int n, bool scroll)
{
	if(n==0) return;

	int d = Ex_memory_depth;
	
	if(!Ex_memory_sections[d]) return;

	if(scroll)
	if(GetKeyState(VK_SCROLL)&1)
	{
		//scroll lock safety enabled
		if(Ex_memory_pointers[d]>= //out of bounds
			Ex_memory_sections[d]->info.RegionSize) return;
	}
	else n = n>0?1:-1;
	
	//scroll lock safety disabled
	if(Ex_memory_pointers[d]>= //out of bounds
		Ex_memory_sections[d]->info.RegionSize)
	{			
		Ex_memory_pointers[d] = 0;
	}

	while(n)
	if(Ex_memory_sections[d]->code)
	{
		if(Ex_memory_pointers[d]+n>=
			Ex_memory_sections[d]->code->opcount)
		{
			if(!Ex_memory_sections[d]->next) break;

			n-=Ex_memory_sections[d]->code->opcount
				-Ex_memory_pointers[d];
			
			Ex_memory_sections[d] =
			Ex_memory_sections[d]->next;
			Ex_memory_pointers[d] = 0;
		}
		else if(Ex_memory_pointers[d]+n<0)
		{
			n+=Ex_memory_pointers[d];
						
			if(Ex_memory_sections[d]->prev)
			{
				Ex_memory_sections[d] =
				Ex_memory_sections[d]->prev;

				if(Ex_memory_sections[d]->code)
				{
					Ex_memory_pointers[d] = 
					Ex_memory_sections[d]->code->opcount-1;
				}
				else
				{
					Ex_memory_pointers[d] = 
					Ex_memory_sections[d]->info.RegionSize-Ex_memory_align;
				}
			}
			else 
			{
				Ex_memory_pointers[d] = 0;			

				break;
			}
		}
		else
		{
			Ex_memory_pointers[d]+=n;

			n = 0;
		}
	}
	else
	{
		n*=Ex_memory_align; 

		if(Ex_memory_pointers[d]+n>=
			Ex_memory_sections[d]->info.RegionSize)
		{
			if(!Ex_memory_sections[d]->next) break;

			n-=Ex_memory_sections[d]->info.RegionSize
				-Ex_memory_pointers[d];
						
			Ex_memory_sections[d] = 
			Ex_memory_sections[d]->next;
			Ex_memory_pointers[d] = 0;
		}
		else if(Ex_memory_pointers[d]+n<0)
		{
			if(!Ex_memory_sections[d]->prev) 
			{
				Ex_memory_pointers[d] = 0; break;
			}

			Ex_memory_sections[d] = Ex_memory_sections[d]->prev;

			n+=Ex_memory_pointers[d];

			if(Ex_memory_sections[d]->code)
			{
				Ex_memory_pointers[d] = 
				Ex_memory_sections[d]->code->opcount-1;
			}
			else
			{
				Ex_memory_pointers[d] = 
				Ex_memory_sections[d]->info.RegionSize-Ex_memory_align;
			}
		}
		else 
		{
			Ex_memory_pointers[d]+=n;

			n = 0;
		}

		n/=Ex_memory_align;
	}
}

extern void EX::next_view_of_memory(int n, bool scroll)
{
	if(n==0) return;

	int d = Ex_memory_depth;

	if(scroll)
	{
		scroll = GetKeyState(VK_CAPITAL)&1;

		//section navigation mode
		if(GetKeyState(VK_SCROLL)&1)
		{
			if(n>0)
			{
				if(Ex_memory_sections[d]->next)
				{
					Ex_memory_sections[d] =
					 Ex_memory_sections[d]->next;
				}
			}
			else if(Ex_memory_sections[d]->prev)
			{
				Ex_memory_sections[d] =
				 Ex_memory_sections[d]->prev;
			}

			return;
		}
	}

	if(n<0)
	{
		Ex_memory_depth+=n;

		if(Ex_memory_depth<0)

			Ex_memory_depth = 0;

		return;
	}		   

	while(n&&d<Ex_memory_maxdepth)
	{
		//unimplemented
		if(Ex_memory_sections[d]->code) return;

		if(!scroll) //try to advance pointer
		{
			DWORD ptr = Ex_memory_pointers[d]; 
			void *mem = Ex_memory_sections[d]->at(ptr);

			if(DWORD(*(void**)mem)%4==0)
			{
				mem = *(void**)mem;

				const EX::Section *next = Ex_memory->by(mem,&ptr);

				if(next)
				{						
					Ex_memory_sections[d+1] = next;
					Ex_memory_pointers[d+1] = ptr;

					Ex_memory_entrypoints[d+1] = mem;

					for(int i=d+2;i<=Ex_memory_maxdepth;i++)
					{
						Ex_memory_sections[i] = 0; //invalidate
					}
				}
			}
		}

		if(Ex_memory_sections[d+1])
		{
			Ex_memory_depth++; d++; n--;
		}
		else return;
	}
}

extern void EX::scan_view_of_memory(int n)
{
	if(n==0) return;

	int d = Ex_memory_depth;
	
	if(!Ex_memory_sections[d]) return;

	if(Ex_memory_pointers[d]>= //out of bounds
		Ex_memory_sections[d]->info.RegionSize)
	{			
		Ex_memory_pointers[d] = 0;
	}

	long long s = Ex_memory_pointers[d]; s-=s%Ex_memory_align;

	const EX::Section *p = Ex_memory_sections[d], *q = 0;
		
	while(n>0)
	{
		while(p)
		{
			if(p->code) //unimplemented
			{
				p = p->next; continue;
			}

			s+=Ex_memory_align;

			if(s>=p->info.RegionSize)
			{
				p = p->next; s = 0; if(!p) break;
			}

			void *mem = p->at(s); 

			DWORD pointer = DWORD(*(void**)mem);
						
			if(pointer%4==0) //potential pointer
			if(Ex_memory->by(*(void**)mem,&pointer)) break;			
		}

		n--;
	}

	while(n<0)
	{
		while(p)
		{
			if(p->code) //unimplemented
			{
				p = p->prev; continue;
			}

			s-=Ex_memory_align;

			if(s<0)
			{
				p = p->prev; if(!p) break;

				s = p->info.RegionSize-Ex_memory_align;
			}

			void *mem = p->at(s); 

			DWORD pointer = DWORD(*(void**)mem);
						
			if(pointer%4==0) //potential pointer
			if(Ex_memory->by(*(void**)mem,&pointer)) break;			
		}

		n++;
	}
	
	if(p)
	{
		s-=s%Ex_memory_align;

		Ex_memory_sections[d] = p;
		Ex_memory_pointers[d] = s;
	}
}

static const char *Ex_memory_camp_(const char *in) 
{
	*Ex_memory_found = '\0';

	int d = Ex_memory_depth;

	if(Ex_memory_sections[d]->code)
	{
		Ex_memory_camp = 
		Ex_memory_sections[d]->code->at(Ex_memory_pointers[d]);
	}
	else
	{
		Ex_memory_camp = 
		Ex_memory_sections[d]->at(Ex_memory_pointers[d]);
	}

	return "saved";
}

static const char *Ex_memory_find_(const char *in)
{
	if(!in) return "error";

	switch(Ex_memory_align) //paranoia
	{
	default: return "error";

	case 4: case 8: break;
	}

	if(!*in) //pointer back trace
	{
		return "unimplemented";
	}
		
	long long head = 0;

	const EX::Section *seek = Ex_memory;
		
	//trailing backspace signals reverse lookup
	int bs = 0; while(bs<32&&in[bs]&&in[bs]!='/') bs++;

	bool reverse = in[bs]=='/'; //in reverse mode

	if(!*Ex_memory_found
	   ||!strncmp(Ex_memory_found,in,bs))
	{
		seek = Ex_memory->by(Ex_memory_camp,&head);
	}

	assert(head<INT_MAX);

	const EX::Section *start = 0; 
	
	signed wrap = head;

	int d = Ex_memory_depth; bool found = false;

	const char *p = in;

	bool num = true;
	bool neg = *p=='-'; 
	bool hex = *p=='0'; 

	if(neg){ in++; p++; }

	if(neg&&hex) return "unsupported"; 

	for(const char*a=p;*p;p++) if(*p!='.')
	{
		if(*p=='-') break; //2020: range?

		if(*p>='a'&&*p<='f') 
		{
			hex = true;

			if(neg) return "unsupported";
		}
		else if(*p<'0'&&*p>'9') num = false;
	}

	bool neg2 = false;
	const char *in2 = 0; if(*p=='-') //2020: range?
	{
		in2 = ++p; num = true;

		neg2 = *p=='-';

		if(neg2){ in2++; p++; }

		if(*p=='0') hex = true; 

		if(neg2&&hex) return "unsupported"; 

		for(const char*a=p;*p;p++) if(*p!='.')
		{
			if(*p>='a'&&*p<='f') 
			{
				hex = true;

				if(neg2) return "unsupported"; 
			}
			else if(*p<'0'&&*p>'9') num = false;
		}
	}

	if(!num) return "unsupported";

	if(*p) //floating point
	{			
		head-=head%Ex_memory_align;

		double f = neg?-1:1; f*=strtod(in,0);

		double f2; if(in2)
		{
			f2 = neg2?-1:1; f2*=strtod(in2,0);
		}

		while(seek)		
		{				
			//TODO: look in code for constants

			int dwords = seek->info.RegionSize/4;

			if(Ex_memory_align==8) dwords-=dwords%2;

			DWORD *cmp = (DWORD*)seek->info.BaseAddress;

			//respect section boundaries
			int safe = dwords-Ex_memory_align/4-1;

			if(!reverse)
			for(int i=head/4;i<safe;i++)
			{						
				if(start==seek
				  &&head>=wrap-Ex_memory_align)
				{
					return "not found";
				}

				if(!in2)
				{
					if(Ex_memory_align==4
					&&fabs(*(float*)(cmp+i)-f)<0.00003
					||Ex_memory_align==8
					&&fabs(*(double*)(cmp+i)-f)<0.00003) 
					{
						found = true; break;
					}			
					else head+=4;
				}
				else //2020
				{
					if(Ex_memory_align==4
					&&f<=*(float*)(cmp+i)+0.00003
					&&f2>=*(float*)(cmp+i)-0.00003
					||Ex_memory_align==8
					&&f<=*(double*)(cmp+i)+0.00003
					&&f2>=*(double*)(cmp+i)-0.00003)
					{
						found = true; break;
					}			
					else head+=4;
				}
			}
			if(reverse&&dwords>head/4) //paranoia
			for(int i=head/4;i>=0;i--)
			{	
				if(start==seek
				  &&head<=wrap+Ex_memory_align)
				{
					return "not found";
				}

				if(!in2)
				{
					if(Ex_memory_align==4
					&&fabs(*(float*)(cmp+i)-f)<0.00003
					||Ex_memory_align==8
					&&fabs(*(double*)(cmp+i)-f)<0.00003) 
					{
						found = true; break;
					}			
					else head-=4;
				}
				else //2020
				{
					if(Ex_memory_align==4
					&&f<=*(float*)(cmp+i)+0.00003
					&&f2>=*(float*)(cmp+i)-0.00003
					||Ex_memory_align==8
					&&f<=*(double*)(cmp+i)+0.00003
					&&f2>=*(double*)(cmp+i)-0.00003)
					{
						found = true; break;
					}			
					else head-=4;
				}
			}

			if(!start) start = seek;

			if(!found) 
			{	
				if(reverse)
				{
					if(!seek->prev)
					{
						seek = Ex_memory->wrap();
					}
					else seek = seek->prev;
										
					head = seek->info.RegionSize-Ex_memory_align;
				}
				else
				{
					if(seek->next)
					{
						 seek = seek->next;
					}
					else seek = Ex_memory;

					head = 0;
				}
			}
			else break;
		}
	}
	else
	{					
		int f = strtol(in,0,10);

		unsigned u,h = strtoul(in,0,16);

		int f2; unsigned u2,h2; if(in2) 
		{
			f2 = strtol(in2,0,10);
			h2 = strtoul(in2,0,16);
		}

		int size = 1; bool dec = false;

		if(hex)
		{			
			if(h>0xFF) size = 2;  
			if(h>0xFFFF) size = 4;

			if(in2&&h2>0xFF) size = max(size,2);
			if(in2&&h2>0xFFFF) size = max(size,4);
		}
		else if(!neg&&!neg2)
		{	
			dec = hex = true; //ambiguous

			u = f; u2 = f2; //"signed/unsigned mismatch"

			if(f>255&&h>0xFF) size = 2;	
			if(f>65535&&h>0xFFFF) size = 4;

			if(in2&&f2>255&&h2>0xFF) size = max(size,2);
			if(in2&&f2>65535&&h2>0xFFFF) size = max(size,4);
		}
		else
		{
			if(f>128) size = 2;
			if(f>32768) size = 4;

			if(neg) f = -f;

			if(in2)
			{
				if(f2>128) size = size = max(size,2);
				if(f2>32768) size = size = max(size,4);

				if(neg2) f2 = -f2;
			}
		}		

		while(seek)		
		{
			int align = seek->code?1:4;

			int bytes = seek->info.RegionSize;

			BYTE *cmp = (BYTE*)seek->info.BaseAddress;

		  //TODO: combine these loops into one

			if(!reverse)
			for(int i=head;i<bytes;i+=align)
			{	
				if(start==seek&&head>wrap-align) //=
				{
					return "not found";
				}

				if(size==1)
				{
					BYTE *b = (BYTE*)(cmp+i);

					if(!in2)
					{
						for(int i=0;i<4;i++) //shadowing
						if(neg&&f==*(signed __int8*)(b+i)
						||dec&&f==*(unsigned __int8*)(b+i)	
						||hex&&h==*(unsigned __int8*)(b+i))						 
						{
							found = true; break;
						}
					}
					else
					{
						for(int i=0;i<4;i++) //shadowing
						if(neg&&f<=*(signed __int8*)(b+i)
						&&neg&&f2>=*(signed __int8*)(b+i)
						||dec&&f<=*(unsigned __int8*)(b+i)	
						&&dec&&f2>=*(unsigned __int8*)(b+i)	
						||hex&&h<=*(unsigned __int8*)(b+i)
						&&hex&&h2>=*(unsigned __int8*)(b+i))	
						{
							found = true; break;
						}
					}

					if(found) break;
				}
				if(size<=2)
				{
					WORD *w = (WORD*)(cmp+i);

					if(!in2)
					{
						for(int i=0;i<2;i++) //shadowing
						if(neg&&f==*(signed __int16*)(w+i)
						||dec&&f==*(unsigned __int16*)(w+i)
						||hex&&h==*(unsigned __int16*)(w+i))					 				 
						{
							found = true; break;
						}
					}
					else
					{
						for(int i=0;i<4;i++) //shadowing
						if(neg&&f<=*(signed __int16*)(w+i)
						&&neg&&f2>=*(signed __int16*)(w+i)
						||dec&&f<=*(unsigned __int16*)(w+i)	
						&&dec&&f2>=*(unsigned __int16*)(w+i)	
						||hex&&h<=*(unsigned __int16*)(w+i)
						&&hex&&h2>=*(unsigned __int16*)(w+i))	
						{
							found = true; break;
						}
					}

					if(found) break;
				}
				if(size<=4) //should be always
				{
					if(!in2)
					{
						if(neg&&f==*(signed __int32*)(cmp+i)
						||dec&&f==*(unsigned __int32*)(cmp+i)					 
						||hex&&h==*(unsigned __int32*)(cmp+i))
						{
							found = true; break;
						}

						if(hex&&align==1) //code: jump pointers
						{
							if(h==int(cmp+i+1)+*(__int8*)(cmp+i)
							||h==int(cmp+i+2)+*(__int16*)(cmp+i)
							||h==int(cmp+i+4)+*(__int32*)(cmp+i))
							{
								found = true; break;
							}
						}
					}
					else
					{
						if(neg&&f<=*(signed __int32*)(cmp+i)
						&&neg&&f2>=*(signed __int32*)(cmp+i)
						||dec&&u<=*(unsigned __int32*)(cmp+i)	
						&&dec&&u2>=*(unsigned __int32*)(cmp+i)	
						||hex&&h<=*(unsigned __int32*)(cmp+i)
						&&hex&&h2>=*(unsigned __int32*)(cmp+i))	
						{
							found = true; break;
						}
					}					
				}		  				

				head+=align;
			}
			if(reverse&&bytes>head) //paranoia
			for(int i=head;i>=0;i-=align)
			{	
				if(start==seek
				  &&head<wrap+align) //=
				{
					return "not found";
				}

				if(size==1)
				{
					BYTE *b = (BYTE*)(cmp+i);

					if(!in2) //DUPLICATE CODE
					{
						for(int i=0;i<4;i++) //shadowing
						if(neg&&f==*(signed __int8*)(b+i)
						||dec&&f==*(unsigned __int8*)(b+i)	
						||hex&&h==*(unsigned __int8*)(b+i))						 
						{
							found = true; break;
						}
					}
					else
					{
						for(int i=0;i<4;i++) //shadowing
						if(neg&&f<=*(signed __int8*)(b+i)
						&&neg&&f2>=*(signed __int8*)(b+i)
						||dec&&f<=*(unsigned __int8*)(b+i)	
						&&dec&&f2>=*(unsigned __int8*)(b+i)	
						||hex&&h<=*(unsigned __int8*)(b+i)
						&&hex&&h2>=*(unsigned __int8*)(b+i))	
						{
							found = true; break;
						}
					}

					if(found) break;
				}
				if(size<=2)
				{
					WORD *w = (WORD*)(cmp+i);

					if(!in2) //DUPLICATE CODE
					{
						for(int i=0;i<2;i++) //shadowing
						if(neg&&f==*(signed __int16*)(w+i)
						||dec&&f==*(unsigned __int16*)(w+i)
						||hex&&h==*(unsigned __int16*)(w+i))					 				 
						{
							found = true; break;
						}
					}
					else
					{
						for(int i=0;i<4;i++) //shadowing
						if(neg&&f<=*(signed __int16*)(w+i)
						&&neg&&f2>=*(signed __int16*)(w+i)
						||dec&&f<=*(unsigned __int16*)(w+i)	
						&&dec&&f2>=*(unsigned __int16*)(w+i)	
						||hex&&h<=*(unsigned __int16*)(w+i)
						&&hex&&h2>=*(unsigned __int16*)(w+i))	
						{
							found = true; break;
						}
					}

					if(found) break;
				}
				if(size<=4) //should be always
				{
					if(!in2) //DUPLICATE CODE
					{
						if(neg&&f==*(signed __int32*)(cmp+i)
						||dec&&f==*(unsigned __int32*)(cmp+i)					 
						||hex&&h==*(unsigned __int32*)(cmp+i))
						{
							found = true; break;
						}

						if(hex&&align==1) //code: jump pointers
						{
							if(h==int(cmp+i+1)+*(__int8*)(cmp+i)
							||h==int(cmp+i+2)+*(__int16*)(cmp+i)
							||h==int(cmp+i+4)+*(__int32*)(cmp+i))
							{
								found = true; break;
							}
						}
					}
					else
					{
						if(neg&&f<=*(signed __int32*)(cmp+i)
						&&neg&&f2>=*(signed __int32*)(cmp+i)
						||dec&&u<=*(unsigned __int32*)(cmp+i)	
						&&dec&&u2>=*(unsigned __int32*)(cmp+i)	
						||hex&&h<=*(unsigned __int32*)(cmp+i)
						&&hex&&h2>=*(unsigned __int32*)(cmp+i))	
						{
							found = true; break;
						}
					}
				}
					
				head-=align;
			}

			if(!start) start = seek;

			if(!found) 
			{	
				if(reverse)
				{
					if(!seek->prev)
					{
						seek = Ex_memory->wrap();
					}
					else seek = seek->prev;
										
					head = seek->info.RegionSize-4;
				}
				else
				{
					if(seek->next)
					{
						 seek = seek->next;
					}
					else seek = Ex_memory;

					head = 0;
				}
			}
			else break;
		}
	}

	if(!found) return "error"; //should not be so

	head-=head%Ex_memory_align;

	Ex_memory_sections[d] = seek;
	Ex_memory_pointers[d] = head;

	if(reverse) 
	{
		if(Ex_memory_pointers[d]-Ex_memory_align>=0)
		{
			Ex_memory_camp = Ex_memory_sections[d]->
			at(Ex_memory_pointers[d]-Ex_memory_align);
		}
		else if(Ex_memory_sections[d]->prev)
		{
			Ex_memory_camp = Ex_memory_sections[d]->prev->
			at(Ex_memory_sections[d]->prev->info.RegionSize-Ex_memory_align);
		}			
		else //wrap around to back of memory
		{
			const EX::Section *p = Ex_memory->wrap();

			Ex_memory_camp = 
			p->at(p->info.RegionSize-Ex_memory_align);
		}
	}
	else
	{
		if(Ex_memory_pointers[d]+Ex_memory_align
		   <Ex_memory_sections[d]->info.RegionSize)
		{
			Ex_memory_camp = Ex_memory_sections[d]->
			at(Ex_memory_pointers[d]+Ex_memory_align);
		}
		else if(Ex_memory_sections[d]->next)
		{
			Ex_memory_camp = Ex_memory_sections[d]->next->at(0);
		}
		else Ex_memory_camp = (void*)0x400000; //hack
	}

	if(seek->code) //hack
	{	
		void *mem = seek->at(head);

		head = seek->code->by(mem); //opcode at address
		
		Ex_memory_pointers[d] = 0; //hack...

		EX::move_view_of_memory(head,false);
	}

	strncpy(Ex_memory_found,in,bs);

	Ex_memory_found[bs] = '\0';

	return "";
}

static const char *Ex_memory_goto_(const char *in, void *base)
{
	if(!in||!Ex_memory) return "error";
		
	char *out = "bad syntax";

	int d = Ex_memory_depth;

	const EX::Section *to = 0;

	if(!*in) //naked
	{
		if(Ex_memory_sections[d]->code) 
		{
			return "unimplemented"; //unimplemented
		}

		to = Ex_memory_sections[d];

		void *mem = to->at(Ex_memory_pointers[d]); 

		DWORD pointer = DWORD(*(void**)mem);
					
		if(pointer%4==0) //potential pointer
		{
			to = Ex_memory->by(*(void**)mem,&pointer);

			if(to) //todo: undo
			{
				Ex_memory_sections[d] = to;
				Ex_memory_pointers[d] = pointer;

				return "";
			}
		}

		return "bad context";
	}
	
	const char *a = in, *sep = a, *z = 0;

	while(*sep&&*sep!='+'&&*sep!=':') sep++; 
	
	z = sep; while(*z) z++;
	
	to = Ex_memory; DWORD off = 0;

	if(*sep=='+')
	{
		char *ok = (char*)sep+1;
		
		off = strtoul(sep+1,&ok,16);

		if(sep-a==0) to = Ex_memory_sections[d];

		if(ok!=z||sep==z) return "bad syntax";	

		off-=off%Ex_memory_align; out = "";		
	}	

	char *ok = (char*)a; 
	
	DWORD num = strtoul(a,&ok,16); 
	
	if(ok==sep&&sep-a) //number
	{
		if(sep-a<=2)
		{
			if(*sep==':') //line
			{	
				if(Ex_memory_sections[d]->code) 
				{
					return "unimplemented"; //unimplemented
				}

				num-=num%Ex_memory_align;

				if(base)
				{
					DWORD at = 0;

					to = Ex_memory->by(base,&at);

					off = (at&0xFFFFFF00)+num;
				}
				else
				{
					to = Ex_memory_sections[d]; 

					off = (Ex_memory_pointers[d]&0xFFFFFF00)+num;
				}

				out = "";
			}	
			else //section
			{
				size_t i = 0; 
				for(i;i<num&&to;i++) to = to->next;
				out = i==num?"":"out of range";
			}
		}
		else if(*sep!='+') //address
		{
			DWORD at = 0; 

			to = Ex_memory->by((void*)(0x400000+num),&at);

			if(!to||!to->code) at-=at%Ex_memory_align;

			off+=at; out = "";
		}
	}
	else if(*a=='h'&&!strcmp(a,"home"))
	{
		DWORD at = 0;

		to = Ex_memory->by(Ex_memory_home,&at);

		off+=at; out = "";
	}	
	else if(*a=='c'&&!strcmp(a,"camp"))
	{
		DWORD at = 0;

		to = Ex_memory->by(Ex_memory_camp,&at);

		off+=at; out = "";
	}	
	else if(*a=='.') switch(a[1]) //hack: expected names
	{
	case 't': //hack: transitional

		if(!memcmp(a,".text",sep-a))
		{
			to = Ex_memory->next; out = ""; //HACK!!!
		}
		break;

	case 'r': //hack: transitional

		if(!memcmp(a,".rdata",sep-a))
		{
			to = Ex_memory->next->next; out = ""; //HACK!!!
		}
		break;

	case 'd': //hack: transitional

		if(!memcmp(a,".data",sep-a))
		{
			to = Ex_memory->next->next->next; out = ""; //HACK!!!
		}
		break;
	}

	if(!*out&&to) //todo: undo
	{			
		if(to->code) 
		{
			void *mem = to->at(off);

			off = to->code->by(mem); //opcode at address
		}
		else off/=Ex_memory_align; //hack...
		
		Ex_memory_sections[d] = to;
		Ex_memory_pointers[d] = 0; //hack...

		EX::move_view_of_memory(off,false);
	}
	else out = "error";

	return out;
}

static const char *Ex_memory_home_(const char *)
{
	int d = Ex_memory_depth;

	if(Ex_memory_sections[d]->code)
	{
		Ex_memory_home = 
		Ex_memory_sections[d]->code->at(Ex_memory_pointers[d]);
	}
	else
	{
		Ex_memory_home = 
		Ex_memory_sections[d]->at(Ex_memory_pointers[d]);
	}

	return "saved";
}

static const char *Ex_memory_redo_()
{
	return "unimplemented";
}

static const char *Ex_memory_undo_()
{
	return "unimplemented";
}

static const char *Ex_memory_match(const char *in, const char *against)
{
	while(*in&&*in!=' '&&*against&&*in++==*against++); 
	
	if(!*in||*in==' ') while(*in==' ') in++; else return false;	

	return in;
}

extern const char *EX::command_view_of_memory(const char *in)
{
	const char *f = 0;

	switch(*in)
	{
	case 'c': //camp

		if(f=Ex_memory_match(in,"camp")) 
			return Ex_memory_camp_(in); 
				break;

	case 'd': //double

		if(f=Ex_memory_match(in,"double")) 
		{	
			int d = Ex_memory_depth;

			if(!Ex_memory_sections[d]->code)
			{
				Ex_memory_pointers[d]-=Ex_memory_pointers[d]%8;
			}

			Ex_memory_align = 8; 
			
			return "";
		}
		break;

	case 'f': //find
	
		if(f=Ex_memory_match(in,"find")) 
			return Ex_memory_find_(f);
				break;

	case 'g': //goto

		if(f=Ex_memory_match(in,"goto")) 
		{
			int d = Ex_memory_depth;

			void *base = 
			Ex_memory_sections[d]->
			at(Ex_memory_pointers[d]);

			if(base)
			if(int(base)%4==0 //paranoia
			  &&!(GetKeyState(VK_CAPITAL)&1))
			{
				DWORD pointer = DWORD(*(void**)base);
								
				if(pointer%4==0) 
				if(Ex_memory->by(*(void**)base))
				{
					base = *(void**)base;
				}
			}
			else base = 0;

			return Ex_memory_goto_(f,base); 			
		}		
		break;

	case 'h': //home

		if(f=Ex_memory_match(in,"home")) 
		return Ex_memory_home_(in); 
		break;

	case 'u': //undo

		if(f=Ex_memory_match(in,"undo")) 
		return Ex_memory_undo_();
		break;

	case 'r': //redo

		if(f=Ex_memory_match(in,"redo")) 
		return Ex_memory_undo_();
		break;

	case 's': //single

		if(f=Ex_memory_match(in,"single")) 
		{	
			int d = Ex_memory_depth;
			if(!Ex_memory_sections[d]->code)
			Ex_memory_pointers[d]-=Ex_memory_pointers[d]%4;
			Ex_memory_align = 4; 			
			return "";
		}
		break;
	}

	return "bad syntax";
}

extern const EX::Section *EX::memory(const void *ext, size_t extsz)
{
	if(!Ex_memory_nul)
	{
		static EX::Section nul;
		memset(&nul.info,0x00,sizeof(nul.info));		
		Ex_memory_nul = &nul;
	}

	Ex_memory_nul->number = -1; 
	Ex_memory_nul->name = L"nul"; 

	if(ext||extsz) //unimplemented
	{
		assert(0); return Ex_memory_nul; 
	}

	if(Ex_memory) return Ex_memory;

	DWORD pid = GetCurrentProcessId(); //som's pid	   
	HANDLE hid = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,0,pid); //note: inheritable for now

	EX::Section *p = Ex_memory = new EX::Section; p->number = 0;
	
	LPCVOID guess = (LPCVOID*)0x400000; //hack

	int max_loops = 24; //arbitrary	maximum

	while(VirtualQueryEx(hid,guess,&p->info,sizeof(p->info)))
	{
		assert(p->info.RegionSize);

		if(!p->info.RegionSize) break;

		if(p->info.AllocationBase!=(void*)0x400000		
			||p->info.State!=MEM_COMMIT				
			||p->info.Protect==PAGE_NOACCESS) 
		{
			guess = (LPCVOID)(int(p->info.BaseAddress)+p->info.RegionSize);
								
			if(!max_loops--) break; else continue;
		}

		EXLOG_LEVEL(5) << "Virtual Memory Page mapped at " << p->info.BaseAddress << '\n';
		EXLOG_LEVEL(5) << " ^Protected status: " << std::hex << p->info.AllocationProtect << '\n';
				
		if(p->prev) //simple sanity check  		
		assert(p->info.BaseAddress>p->prev->info.BaseAddress);

		EX::Section *swap = new EX::Section;

		p->next = swap; swap->prev = p; swap->number = p->number+1;

		guess = (LPCVOID)(int(p->info.BaseAddress)+p->info.RegionSize);

		switch(p->number) //assuming!!
		{
		case 1: p->name = L".text"; break;
		case 2: p->name = L".rdata"; break;
		case 3: p->name = L".data"; break;
		}

		p = swap; if(!max_loops--) break;
	}

	if(p->prev)	p->prev->next = 0; delete p;

	EXLOG_LEVEL(5) << "Virtual Memory mapped\n";

	CloseHandle(hid);

	return Ex_memory;
}

//Fyi: quarantining BeaEngine stuff in Ex.disasm.cpp
extern int Ex_disasm_sum(intptr_t,intptr_t,intptr_t,BYTE*);
extern char Ex_disasm_lde(intptr_t,intptr_t,intptr_t);
extern const char *Ex_disasm_asm(intptr_t ep);

const EX::Program *EX::program(const EX::Section *in, intptr_t ep)
{
	if(!in||!ep)
	{
		if(!EX::header()) return 0;

		ep = 0x400000+EX::header()->optional.AddressOfEntryPoint;

		if(in&&ep<(long long)in->info.BaseAddress
			||ep>=(long long)in->info.BaseAddress+in->info.RegionSize) return 0;
	}

	if(!in)
	{
		in = EX::memory();

		while(in&&ep<(long long)in->info.BaseAddress) in = in->next; 

		if(!in) return 0;
	}

	EX::Program *out = new EX::Program();

	out->text = in;	out->entrypoint = ep;

	size_t n = in->info.RegionSize;

	intptr_t a = (DWORD)in->at(0), z = a+n;
	
	BYTE *code = out->scratchmem<BYTE>(n);

	memset(code,0x00,sizeof(BYTE)*n);

	int c = Ex_disasm_sum(a,ep,z,code);

	if(c>0) out->opcount = c; else goto woops;
	
	int *p = new int[c+2]; p[0] = p[c] = 0;
	char *q = new char[c+2]; q[0] = q[c] = 0;
						
	out->opcodes = ++p; out->opchars = ++q; 

	for(size_t i=0;i<n;i++) if(code[i])
	{	
		//optimization:
	    //Ex_disasm_sum stores run length as a courtesy

		*q = code[i]; //Ex_disasm_lde(a,a+i,z);

		if(*q<=0||p-out->opcodes>=c) goto woops; //paranoia
			 
		*p++ = i; i+=*q++-1;
	}	

	out->loboundary = a; 
	out->hiboundary = z;

	return out;

woops: //something went wrong
		
	delete out; //deletes code/scratch	 

	return 0;
}

const char *EX::Program::x(int op, const char *ln)const
{
	if(size_t(op)>opcount||op<0||!opcodes) return "";
	
	const char *out = Ex_disasm_asm(loboundary+opcodes[op]);

	if(!out) return "???";

	if(out[-1]!=opchars[op]) //assuming code edited
	{			
		//assert(0);
		//Reminder: happens when debugging the disassembly with MSVC

		return "???"; //unimplemented
	}

	if(ln) //with line prefix
	{
		static char tmp[128];
		if(sprintf_s(tmp,ln,out)>0) return tmp; 
	}

	return out;
}

const EX::x86_t &EX::Program::x86(int op, int count)const
{
	static EX::x86_t out; 

	assert(0); //unimplemented;	
	
	return out;
}

int EX::Program::by(const void *pin)const
{
	long long in = (long long)pin-loboundary;

	if(in<0||!opcount||!opcodes||!text) return 0;

	if(in>=text->info.RegionSize) return opcount;

	int cmp, first = 0, last = opcount-1, stop = -1;

	int x = (last+first)/2;
	for(x;x!=stop;x=(last+first)/2)
	{
		cmp = opcodes[x]>in?+1:-1;

		if(cmp<0&&in-opcodes[x]<opchars[x]) return x;

		if(cmp>0) stop = last = x; else stop = first = x;
	}			   
	if(last==opcount-1&&x==last-1) //round-off error
	{
		if(opcodes[last]<=in&&
		   opcodes[last]+opchars[last]>in) 
		    return last;

		x = last; 
	}		
	return opcodes[x]>in?x:x+1; 
}		
