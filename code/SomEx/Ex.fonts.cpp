
#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

#include <map>
#include <string>

#include "Ex.fonts.h"

//REMOVE ME?
const wchar_t *EX::Font::u_0001_ffff = L"\1\uffff";

struct EX::Font::grapheme_t
{
	ULONG refs; //0th member (apparently)

	ID3DXFont *d3dxfont;
	ID3DXSprite *d3dxsprite;
	int glfont; //Exselector

	const wchar_t *defaults; //selection

	LOGFONTW lfont; HFONT hfont;		

	grapheme_t(){ memset(this,0x00,sizeof(*this)); }
};	
static ULONG *Ex_fonts_f(const wchar_t *in, size_t in_s)
{
	wchar_t key[64]; //NEW: rewritten (64: purely arbitrary)
	wmemcpy_s(key,EX_ARRAYSIZEOF(key),in,in_s);	key[in_s] = '\0';
	static std::map<std::wstring,EX::Font::grapheme_t> out;
	return &out[key].refs;
}
static EX::Font::grapheme_t &Ex_fonts_g(const ULONG *f)
{
	return *(EX::Font::grapheme_t*)f;
}

extern const wchar_t *Ex_describing_font(const LOGFONTW &log, const wchar_t *f, va_list va)
{
	static wchar_t in[2056];
	static wchar_t out[2056+64]; 

	if(vswprintf_s(in,2056,f,va)<0) return 0;

	if(!f)
	{
		if(0>=swprintf_s(out,in,L"%d %d %d %s",log.lfHeight,log.lfWidth,log.lfWeight,log.lfFaceName))
		return 0; return out;
	}
	
	int i = 0;

	float numbers[2];
		
	bool whitespace = false;
	bool glyphsets = false;
	
	int faces = 0; //allow leading ranges

	bool terminal = false;

	wchar_t *p = in;	
	wchar_t *q = out; *q++ = ' '; 

	while(1) switch(*p)
	{
	case ' ': case '\t':
		
	case L'\u3000': //TODO: full Unicode

	case '\r': case '\n':
		
		whitespace = false;

		if(q[-1]!=' ') *q++ = *p;
		
		p++; break; 

	default:	

		terminal = *p=='\0';

		if(whitespace) return 0;

		if(*p==','&&glyphsets)
		{
			if(q[-1]==' ')
			{
				q[-1] = ','; p++;
			}
			else *q++ = *p++; *q++ = ' ';

			glyphsets = false; 
		}
		else if(p[*p=='-']>='0'&&p[*p=='-']<='9') //numbers
		{
			if(glyphsets)
			{
				q[-1] = ','; *q++ = ' ';

				glyphsets = false;
			}

			wchar_t *b; 
			
			float n = wcstod(p,&b); 

			if(p==b) return 0; else p = b;			

			if(*p=='%') 
			{
				n/=100.0f; p++; 
			}

			if(i<2) numbers[i++] = n;

			whitespace = *p!=',';
		}
		else if(*p=='U'&&p[1]=='+') //Unicode range
		{
			if(i||faces)
			if(!glyphsets) return 0;

			if(!faces) //apply ranges to logical font
			{
				assert(q==out+1);

				if(q!=out+1) return 0;

				int n = swprintf_s(q,2056,L"%d %d %d %s ",log.lfHeight,log.lfWidth,log.lfWeight,log.lfFaceName);

				if(n>0) q+=n; else return 0;

				glyphsets = true;
			}

			p[0] = 'U'; p[1] = '+';

			wchar_t *b; wcstol(p+2,&b,16);

			if(b==p+2) return 0; 

			if(*b=='-') //range
			{
				wchar_t *p; //shadowing

				wcstol(b+1,&p,16);

				if(p==b+1) return 0;

				b = p;
			}						

			if(q-out+b-p>=2056) return 0;

			memcpy(q,p,sizeof(wchar_t)*(b-p)); 

			q+=b-p; p+=b-p;

			whitespace = *p!=',';
		}
		else //assuming the font face in question
		{				
			if(glyphsets)
			{
				if(terminal)
				{
					*q = '\0'; 
					
					return out+1; //sans leading space
				}
				else if(q[-1]==' ') 
				{
					q[-1] = ',';
				}
				else *q++ = ','; *q++ = ' ';

				glyphsets = false;				
			}

			wchar_t quote = 0; 

			if(*p=='\''||*p=='"') quote = *p;

			if(quote) p++; *q = '\0';

			int bold = log.lfWeight;

			if(i>0) bold = numbers[i-1];

			float x = i>1?numbers[i-2]:1.0f;

			int h = x*log.lfHeight;
			int w = x*log.lfWidth;
			int n = swprintf_s(q,2056-int(q-out),L"%d %d %d ",h,w,bold);

			if(n<0) return 0; else q+=n;

			if(*p==',') //NEW: 12/10/2012
			{
				*q++ = ' '; *q++ = '@'; //dummy face
			}

			while(!glyphsets)
			{
				switch(*p)
				{					
				case '\'': case '"':
					
					if(*p!=quote) return 0;

					p++; //FALLS THRU
							  
				case ',': case '\0': 
							
					terminal = *p=='\0';			

					glyphsets = true; break;
															
				case ' ': case '\t':

				case L'\u3000': //TODO: full Unicode
				
				case '\r': case '\n':

					if(p[1]>='0'&&p[1]<'9')
					{
						if(quote) return 0;

						glyphsets = true; break;
					}
					else if(p[1]=='U'&&p[2]=='+')
					{
						if(quote) return 0;

						glyphsets = true; break;
					}
					
					if(*p!='\r'&&*p!='\n')
					{
						if(q[-1]!=' ') *q++ = ' '; 
					}
					
					p++; break;

				default: *q++ = *p++;	
				}
			}

			assert(glyphsets); 
						
			if(terminal)
			{
				*q = '\0'; 
				
				return out+1; //sans leading space
			}

			faces++;

			i = 0;
		}
	}

	return 0;
}

extern const wchar_t *EX::describing_font(const LOGFONTW &log, const wchar_t *format,...)
{
	va_list va; va_start(va,format); return Ex_describing_font(log,format,va);
}

extern const EX::Font *EX::creating_font(const LOGFONTW &in, const wchar_t *description)
{	
	int n = 1; const wchar_t *p = description; if(!p) p = L"";

	for(int paranoia=0;*p&&paranoia<2056;paranoia++,p++) if(*p==',') n++;

	assert(!*p); if(*p) return 0; //this is not good
	
	if(!description)
	{
		static wchar_t tmp[256]; description = tmp;
		if(0>=swprintf_s(tmp,256,L"%d %d %d %s",in.lfHeight,in.lfWidth,in.lfWeight,in.lfFaceName)) 
		return 0; 
	}

	p = description;

	//REMOVE ME?
	EX::Font *out = EX::Font::construct(in,n);

	out->glyphs[n].selection = 0; //null terminate

	HDC dc = EX::hdc(); //assuming this is safe

	for(int et=0;et<n;et++) //et spells glyphset?
	{
		out->glyphs[et].refs = 0; //for error dtor 
		out->glyphs[et].selection = EX::Font::u_0001_ffff;
	
		const wchar_t *a = p, *z = a; while(*z&&*z!=',') z++; 
		
		int stats[3]; if(*p==' ') p++;

		for(int i=0;i<3;i++)
		if(p[*p=='-']>='0'&&p[*p=='-']<='9') //want numbers
		{	
			stats[i] = wcstol(p,0,10); 

			while(*p&&*p!=' ') p++; if(*p) p++;

			if(!*p&&i!=2) goto err;
		}
		else goto err;

		const wchar_t *u = p;

		while(u!=z&&(u[0]!='U'||u[1]!='+')) u++;

		ULONG *f = Ex_fonts_f(a,u-a);

		EX::Font::grapheme_t &g = Ex_fonts_g(f);

		out->glyphs[et].d3dxfont = &g.d3dxfont;
		out->glyphs[et].d3dxsprite = &g.d3dxsprite;
		out->glyphs[et].glfont = &g.glfont;
				
		if(in.lfHeight)
		{
			out->glyphs[et].scale = 
			float(stats[0])/float(in.lfHeight);
		}
		else out->glyphs[et].scale = 1.0f;

		const wchar_t *rz = 0;

		if(!g.hfont)
		{										
			g.lfont.lfHeight = stats[0];
			g.lfont.lfWidth  = stats[1];
			g.lfont.lfWeight = stats[2];

			//seems safe with Unicode API
			g.lfont.lfCharSet = DEFAULT_CHARSET; 
			g.lfont.lfQuality = PROOF_QUALITY; 

			const wchar_t *l = p, *r = u; 
			
			while(*l&&*l==' '&&l<r) l++; //ltrim
			while(r[-1]==' '&&r>l) r--; //rtrim

			if(*l=='@'&&r==l+1) //dummy
			{
				wcscpy_s(g.lfont.lfFaceName,in.lfFaceName);
			}
			else wcsncpy(g.lfont.lfFaceName,l,min(LF_FACESIZE-1,r-l));
						
			g.lfont.lfFaceName[LF_FACESIZE-1] = '\0';

			g.hfont = CreateFontIndirectW(&g.lfont); 

			if(!g.hfont) goto err;
		}

		if(u!=z&&et!=n-1) //have explicit unicode ranges
		{			
			int i,n;
			for(i=0,n=0;u+i!=z;i++) if(u[i]=='+') n++;

			wchar_t *rz = new wchar_t[n*2+2];

			for(i=0;i<n*2&&*u!=' ';i+=2)
			{
				if(u[0]!='U'||u[1]!='+') break;

				wchar_t *v;

				rz[i] = wcstol(u+2,&v,16);

				if(v!=u+2) u = v; else break;				

				if(*u=='-')
				{
					rz[i+1] = wcstol(u+1,&v,16);

					if(v!=u+1) u = v; else break;
				}
				else rz[i+1] = rz[i];

				if(rz[i]==0) rz[i] = 1;

				if(i&&int(rz[i])-int(rz[i-1])<1) break;

				if(int(rz[i+1])-int(rz[i])<0) break;

				//2018: I think this was missing to enable
				//multiple ranges
				if(*u==' '&&u[1]=='U') u++;
			}	   
			if(i==n*2) //looks good
			{
				rz[i] = rz[i+1] = '\0';

				out->glyphs[et].selection = rz;
			}
			else delete[] rz; //bad ranges
		}

		if(et!=n-1)
		if(!g.defaults) //Note: could store by face
		if(out->glyphs[et].selection==EX::Font::u_0001_ffff)
		{						
			GLYPHSET gs = {64,0};

			SelectObject(dc,g.hfont);

			DWORD required = GetFontUnicodeRanges(dc,0);

			if(required<=64 //impractical for huge fonts
			  &&GetFontUnicodeRanges(dc,&gs)&&gs.cRanges)
			{
				wchar_t *rz = new wchar_t[gs.cRanges*2+2];

			  //Hmmm: what to do about surrogate pairs??//

				//hack: skirt 0th codepoint
				WCRANGE *gs_ranges = gs.ranges;

				if(gs.ranges[0].wcLow==0)
				{
					if(gs.ranges[0].cGlyphs==1)
					{
						gs_ranges++; gs.cRanges--;
					}
					else gs.ranges[0].wcLow = 1;
				}

				size_t i;
				for(i=0;i<gs.cRanges;i++)
				{
					rz[i*2] = rz[i*2+1] = gs_ranges[i].wcLow; 

					rz[i*2+1]+=gs_ranges[i].cGlyphs;
				}

				rz[i*2] = rz[i*2+1] = '\0';

				g.defaults = rz;
			}
			else g.defaults = EX::Font::u_0001_ffff;
		}

		if(et!=n-1)
		if(out->glyphs[et].selection==EX::Font::u_0001_ffff)
		{
			out->glyphs[et].selection = g.defaults;
		}		

		out->glyphs[et].lfont = &g.lfont; 
		out->glyphs[et].hfont = g.hfont;  		
		out->glyphs[et].refs = f;

		p = z+1;
	}
	for(int et=0;et<n;et++)
	{
		*out->glyphs[et].refs++; //et spells glyphset?
	}

	int _ = out->select(L"_");
	int __ = out->select(L"\u3000");

	SetMapMode(dc,MM_TEXT); 
	SetGraphicsMode(dc,GM_ADVANCED);
	ModifyWorldTransform(dc,0,MWT_IDENTITY);

	SelectObject(dc,out->glyphs[_].hfont); 

	ABCFLOAT abc; //GetCharWidthFloat is bunk

	if(GetCharABCWidthsFloatW(dc,' ',' ',&abc))
	{
		out->space = abc.abcfA+abc.abcfB+abc.abcfC;
	}

	SelectObject(dc,out->glyphs[__].hfont);

	if(GetCharABCWidthsFloatW(dc,L'\u3000',L'\u3000',&abc))
	{
		out->u3000 = abc.abcfA+abc.abcfB+abc.abcfC;
	}	
		
	EX::hdc(&dc);

	return out;

err: assert(0);
	
	out->destruct();

	return 0;
}

int EX::Font::select(const wchar_t *in, int sz, bool midline)const
{
	if(sz<0) //TODO: right justification
	{
		assert(0); return 0;
	}

	if(!in) in = L"";

	/*OBSOLETE?
	cutting up new lines
	if(sz==0) sz = MAX_SELECT; //hack...
	else if(sz>MAX_SELECT) sz = MAX_SELECT;					  
	sz = wcsnlen(in,sz);*/
	
	if(!*in||!glyphs[0].selection) 
	{
		glyphs[0].selected = sz; return 0;
	}

	int out = -1;

	//et spells glyphset?
	while(*in&&out<0||glyphs[out].selected<sz)
	for(int et=0;et>=0&&glyphs[et].selection;et++)
	{
		const wchar_t *rz = glyphs[et].selection;
				
		while(*rz) if(*in>=rz[0]&&*in<=rz[1])
		{
			if(out==-1)
			{
				glyphs[et].selected = 0;
				glyphs[et].nselected = 0; //NEW
			}
			else if(out!=et) 
			{
				return out; //breakpoint
			}

			glyphs[out=et].selected++;

			switch(*in++) //in++;
			{
			case '\n': //NEW

				glyphs[out].nselected++;
				
				if(midline) goto out; break;
			}

			et = -2; break; //hack
		}
		else rz+=2; //2018: was rz++
	}

	out: assert(out>-1); return out; 
}