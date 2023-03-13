
#include "Somtexture.pch.h"

#include <time.h> //import notes
#include <typeinfo.h> //Sompaint.h

#include "../lib/swordofmoonlight.h"

#include "Somthread.h"

//private implementation
#include "Somtexture.inl"

#include "Sominstant.h"
#include "Somenviron.h"
#include "Somgraphic.h"

#ifdef _D3D9_H_
#ifdef __D3DX9_H__
#define SOMTEXTURE_D3DX
//will need to implement eventually
static SOMPAINT Somtexture_D3D9 = 0;
#endif
#endif

namespace Somtexture_cpp
{	
	struct pool
	:
	public Somthread_h::account
	{			
		SOMPAINT server; 

		inline SOMPAINT operator->(){ return server; }
				
		struct thread{ DWORD id; void *pix, *src, *dst, *pal; }; 

		DWORD _tls; std::vector<thread*> threads;

		struct thread *tls()
		{
			if(!_tls) _tls = TlsAlloc(); assert(_tls);

			thread *out; if(out=(thread*)TlsGetValue(_tls)) return out;

			thread cp = {0,0,0,0,0}; if(!TlsSetValue(_tls,out=new thread(cp))) assert(0); 
						
			threads.push_back(out); out->id = GetCurrentThreadId();
			
			if(threads.size()>8) assert(0); //TODO: garbage collection

			return out;
		}		
			
		Somtexture_app application;

		const Somtexture *backbuffer;

		//using to bootstrap apply_state
		const wchar_t *dummy_vs, *dummy_ps;

		pool(SOMPAINT spool) : server(spool)
		{					 			
			_tls = 0; backbuffer = 0; 

			server->format(application,"state buffer: front;"); //with culling

			dummy_vs = server->assemble(Somshader::vs_2_0,Somshader::vs_2_0_s);
			dummy_ps = server->assemble(Somshader::ps_2_0,Somshader::ps_2_0_s);

			assert(dummy_vs&&!*dummy_ps);
		}
		~pool()
		{
			size_t i = threads.size(); while(i>0) delete threads[--i]; 			

			if(_tls) TlsFree(_tls);
		}

	private: pool(){ _tls = 0; } friend std::map<int,pool>;
	};

	typedef std::map<int,pool>::iterator pool_it;

	std::map<int,pool> pools;

	struct poolhelper
	{			
		poolhelper(int &p) : pool(p)
		{
			Somthread_h::section cs;

			if(pool>0) //validate
			{
				pool_it it = pools.find(pool); 

				if(it==pools.end()||!it->second) pool = 0; return; 
			}

			int item = -pool; pool = 0;
			
			//if(!menu.size()) Somtexture::menu(0); //shelving for now
			
			//if(item<menu.size())
			{	
				wchar_t v[4] = {SOMPAINT_VERSION}; 

				wchar_t Sompaint[MAX_PATH] = L"Sompaint_D3D9.dll";

				HMODULE Sompaint_dll = LoadLibraryW(Somtexture_pch::library(Sompaint));

				SOMPAINT (*Connect)(const wchar_t[4],const wchar_t*) = 0;

				*(void**)&Connect = 
				GetProcAddress(Sompaint_dll,"Sword_of_Moonlight_Raster_Library_Connect");
				SOMPAINT server = 0; //Sword_of_Moonlight_Raster_Library_Connect(v);

				if(Connect) server = Connect(v,0);				

				if(server) pools[pool=++counter] = Somtexture_cpp::pool(server);

				assert(pool);
			}
			//else assert(0);
		}

		~poolhelper() 
		{
			SOMPAINT server = 0;
			{
				Somthread_h::section cs;

				if(pool&&!Somtexture_cpp::pools[pool])
				{
					server = Somtexture_cpp::pools[pool].server;
							
					Somtexture_cpp::pools.erase(pool);
				}
			}

			if(server) server->disconnect(false); //SOMPAINT_DIRECT
			//Sword_of_Moonlight_Raster_Library_Disconnect(server);
		}

		inline operator int(){ return pool; } 

	private: static int counter;

		int &pool;
	};

	int poolhelper::counter = 0;

	struct wellhelper
	{			
		enum
		{				
		PAD = 1, //debugging

		ENUM = -1, SIZE = -2, TONE = -4, BACK = -3,

		HEAD = 4, NAME = 32, TAIL = 6, NEXT = HEAD+NAME+TAIL,		

		MAX = 32, NEW = MAX+NEXT*MAX+MAX,			
		};

		static wchar_t nul[NEXT], *unknown;
				
		wellhelper(wchar_t* &p) : wells(p)
		{
			if(!wells) 
			{
				wells = new wchar_t[PAD+NEW+PAD];
				wells+=PAD;
				memset(wells,0x00,sizeof(wchar_t)*NEW);

				wells[MAX+NEXT*0+HEAD+ENUM] = Somtexture::COLORKEY;
				wells[MAX+NEXT*1+HEAD+ENUM] = Somtexture::RESERVED;								
				for(int i=2;i<MAX;i++) 
				wells[MAX+NEXT*i+HEAD+ENUM] = Somtexture::UNTITLED+i-2;

				wells[MAX+NEXT*0+NEXT-1+ENUM] = Somtexture::RESERVED|Somtexture::COLORKEY;
				wells[MAX+NEXT*1+NEXT-1+ENUM] = Somtexture::REPLACED;
				for(int i=2;i<MAX;i++) 
				wells[MAX+NEXT*i+NEXT-1+ENUM] = Somtexture::RESERVED|Somtexture::UNTITLED+i-2;

				wchar_t *r = operator[](+Somtexture::RESERVED);

				r[TONE] = 0; r[BACK] = 255; r[SIZE] = 256;
			}
		}

		//TODO: implement the bulk of this elsewhere
		wellhelper(wchar_t* &p, const Sompalette q[256]) : wells(p)
		{
			int i, j, k; if(!q) return;
		
			if(!wells) new (this) wellhelper(p);

			int past = 0, mask; bool ok = true;

			wchar_t *r = operator[](+Somtexture::RESERVED);

			r[SIZE] = 0;
			for(i=j=k=0;i<256&&k<MAX-2;i=j,k++)
			{
				int id = q[i].peFlags; mask = 0;

				for(j=i+1;j<256&&q[j].peFlags==id;j++);

				switch(id)
				{
				case Somtexture::COLORKEY: ok = i==0&&j-i==1; break;
				
				default: ok = id>=Somtexture::UNTITLED&&id<=Somtexture::TITLED+(MAX-3);					

					mask = 1<<(id-Somtexture::UNTITLED); break;
												
				case Somtexture::RESERVED: ok = j==256; break;
				}

				if(!ok||mask&past) break; past|=mask;

				wchar_t *well = operator[](id); assert(well);
				
				well[SIZE] = j-i; well[TONE] = i;
				well[BACK] = j-1;

				wells[k] = id;
			}			
			wells[k++] = r[ENUM];

			while(k<MAX) wells[k++] = 0;

			if(i!=256) //bad palette
			{
				operator~(); assert(0);
			}
		}

		template<typename T> wchar_t *operator[](T n)
		{
			if(n<Somtexture::COLORKEY) //by ordinal
			{
				if(n<0) //reserve
				{
					n = n<-MAX?0:wells[NEW-MAX-n-1]; 
				}
				else n = n>=MAX?0:wells[n]; 
				
				if(!n) return unknown;			
			}			
			switch(n) //by constant
			{
			case Somtexture::COLORKEY: return wells+MAX+HEAD;
			case Somtexture::RESERVED: return wells+MAX+NEXT+HEAD;			
			case Somtexture::FINISHED: 

				for(int i=0;i<MAX&&wells[NEW-MAX+i];i++)
				{
					wchar_t *r = operator[](wells[NEW-MAX+i]); r[SIZE] = wells[NEW-MAX+i] = 0;
				}
				wells[MAX+NEXT+NEXT-1+SIZE] = 0; return unknown; //REPLACED

			case Somtexture::REPLACED: return wells+MAX+NEXT+NEXT-1;

			case Somtexture::RESERVED|Somtexture::COLORKEY:

				return wells+MAX+NEXT-1;
			}

			int r = n&Somtexture::RESERVED; n&=~r; n-=Somtexture::UNTITLED; 			
			
			assert(n>=0&&n<MAX-2); if(n<0||n>=MAX-2) return unknown;

			return wells+MAX+NEXT*2+NEXT*n+(r?NEXT-1:HEAD);			
		}
		inline wchar_t *operator[](const wchar_t *p)
		{
			if(p<wells+MAX+HEAD||p>=wells+MAX+NEXT*MAX) return unknown;

			return operator[](p[ENUM]); //paranoia
		}
		inline wchar_t *operator()(int) 
		{		
			wchar_t *tn = wells+MAX+NEXT*3+HEAD;

			for(int i=0;i<MAX-3;i++,tn+=NEXT) if(!tn[SIZE]) break;

			return !tn[SIZE]?tn:0; //new well
		}
		//inline wchar_t *operator+=(const wchar_t*)
		inline wchar_t *operator+=(const wchar_t (&well)[MAX])
		{
			wmemcpy(wells,well,MAX); return wells;
		}
		inline wchar_t *operator+=(int zero) //simulate 0 pointer
		{ 
			return wells; assert(zero==0);
		}
		//inline wchar_t *operator-=(const wchar_t*)
		inline wchar_t *operator-=(const wchar_t (&reserve)[MAX])
		{
			wmemcpy(wells+NEW-MAX,reserve,MAX); return wells+NEW-MAX;
		}
		inline wchar_t *operator-=(int zero) //simulate 0 pointer
		{ 
			return wells+NEW-MAX; assert(zero==0);
		}
		inline operator wchar_t*()
		{
			return wells; 
		}
		inline void operator~()
		{
			if(wells) delete [] (wells-PAD); 
			
			wells = 0;
		}

	private:

		wchar_t* &wells;
	};

	wchar_t wellhelper::nul[wellhelper::NEXT] = {0,0,0,0,0,0,0,0,0};		
	wchar_t *wellhelper::unknown = wellhelper::nul+wellhelper::HEAD;

	struct notehelper
	{
		enum
		{	
			HD = 2, WR = -1, SZ = -2, MAX = wchar_t(-1)
		};

		//TODO: migrate Somtexture::notes logic into here

		notehelper(wchar_t* &p):notes(p){};
	
		inline operator wchar_t*(){ return notes; } 
		
		inline void operator=(const notehelper &cp){ notes = cp.notes; }

		inline void operator~()
		{
			if(notes) delete [] (notes-HD);	notes = 0;
		}

	private:

		wchar_t* &notes;	
	};
}	

//private
Somtexture::Somtexture(const Somtexture &cp)
{
	/*do not enter*/ assert(0); 
}

//private
Somtexture::Somtexture(int pool) : Somtexture_inl(pool)
{				
	width = height = 0; mask = 0; palette = 0; pal = 0; //Somtexture.h	

	Somthread_h::section cs; //Somtexture.cpp//

	Sompaint = Somtexture_cpp::pools[pool].server; //Somtexture.inl

	Somtexture_cpp::pools[pool]++; 
}

//private
Somtexture::~Somtexture() //called inplace by Somtexture_open/edit
{	
	/*Somtexture_cpp::pools[pool]--; //See Somtexture::release()*/

	width = height = 0; mask->release(); mask = 0; delete[] palette; palette = 0; 
		
	Somtexture_inl->discard(this); Sompaint->discard(&pal); assert(pal==0);
}

void Somtexture_inl::editor::discard(const Somtexture *edited)
{
	if(this) for(size_t i=0;i<pals_s;i++) edited->Sompaint->discard(pals+i);
}

Somtexture_inl::editor::~editor()
{
	for(size_t i=0;i<pals_s;i++) assert(!pals[i]);

	delete[] rgba; delete[] index; //alpha
	
	~Somtexture_cpp::wellhelper(wells);
	~Somtexture_cpp::notehelper(notes);		
}

RECT *Somtexture_inl::editor::dirty(int ch) 
{
	//todo: follow target::pixel's example

	switch(ch) //unsustainable
	{
	case Somtexture::RGB:   return _dirty+0;
	case Somtexture::PAL:
	case Somtexture::INDEX: return _dirty+1;
	case Somtexture::ALPHA: return _dirty+2;

	default: assert(0); //paranoia
		
		SetRectEmpty(_dirty+3); 
		
		return _dirty+3;
	}
}	  
bool Somtexture_inl::editor::clean(int ch, RECT *inout)
{
	RECT *p = dirty(ch), tmp;
	if(IsRectEmpty(p)) return true;

	if(!inout) return !SetRectEmpty(p);	//hack

	if(!IntersectRect(&tmp,p,inout)) return true;
	
	CopyRect(inout,&tmp); SubtractRect(&tmp,p,inout);
	
	//randomly expand until we can subtract
	for(int i=0;EqualRect(&tmp,p)&&i<4;i++)
	{
		((LONG*)inout)[i] = ((LONG*)p)[i];

		SubtractRect(&tmp,p,inout);
	}
	CopyRect(p,&tmp); return false;			
}

void Somtexture_inl::editor::fillout(const Somtexture *texture, 
	
	const BYTE *rin, size_t rin_pitch, size_t rin_stride, 			
	const BYTE *in, size_t in_pitch, size_t in_stride, const BYTE *a)
{	
	Somtexture_inl *init = texture?&texture->Somtexture_inl:0;

	const Sompalette *palette = texture?texture->palette:0;

	int w = texture?texture->width:0, h = texture?texture->height:0;

	rgba = 0; index = alpha = 0; black = 0; 

	if(size=w*h)
	{
		int ko = a?0:size;
		while(ko<size&&a[ko*in_stride]==0xFF) ko++;
		if(ko==size) a = 0;

		rgba = rin?new PALETTEENTRY[size]:0;
		index = in?new BYTE[size*(in&&a?2:1)]:0;
		if(in&&a) alpha = index+size;
	}	   
	if(rgba) for(int i=0;i<h;i++)
	{				
		BYTE *p = (BYTE*)(rgba+i*w);
		
		const BYTE *q = rin+rin_pitch*i;
		
		for(size_t j=0,k;j<w;j++) 
		{
			for(k=0;k<rin_stride&&k<4;k++) *p++ = *q++;

			while(k++<4) *p++ = 0xFF; //hack
		}
	}
	if(index) for(int i=0;i<h;i++)
	{
		BYTE *p = index+i*w, *r = alpha+i*w;

		const BYTE *q = in+i*in_pitch, *s =  a+i*in_pitch;					

		for(size_t j=0;j<w;j++) 
		{
			*p++ = *q; q+=in_stride; if(a) *r++ = *s; s+=in_stride;
		}				
	}
	for(int i=0;i<256;map[i].peFlags=i++) 
	{
		if(palette)
		{
			map[i] = palette[i]; 

			std::swap(map[i].peBlue,map[i].peRed);
		}
		else *(DWORD*)(map+i) = 0; //hack
	}

	if(!init) return;

	int use = init->current;

	if(rgba) init->supported|=Somtexture::RGB;
	if(!use) use = init->supported;
	if(palette&&index) init->supported|=Somtexture::PAL;
	if(!use) use = init->supported;
	if(palette) init->supported|=Somtexture::MAP;
	//if(!use) use = supported;
	if(black) init->supported|=Somtexture::BLACK;
	if(index) init->supported|=Somtexture::INDEX;
	if(!use) use = init->supported;
	if(alpha) init->supported|=Somtexture::ALPHA;
	if(!use) use = init->supported;

	if(rgba&&!alpha) for(int i=0,n=w*h;i<n;i++)
	{
		if(rgba[i].peFlags!=0xFF)
		{
			init->supported|=Somtexture::ALPHA; break;
		}
	}
					
	if(Somtexture::primary_candidate(use)) init->current = use;			

	init->supported|=Somtexture::NOTES; 
}		

int Somtexture_inl::editor::knockout(PALETTEENTRY pal[256])
{
	int ko = 0; //out
						
	if(rgba) for(int i=0;i<size;i++)
	if((*(DWORD*)(rgba+i)&0x00FFFFFF)==0)
	{
		rgba[i].peFlags = 0; ko++; assert(!alpha);

		if(alpha) alpha[i] = 0; //appropriate?
	}	
	int colorkey = -1;
	if(index&&pal) for(int i=0;i<size;i++)
	if((*(DWORD*)(map+index[i])&0x00FFFFFF)==0)
	{	
		if(alpha) alpha[i] = 0; 

		if(colorkey==-1) colorkey = index[i];

		index[i] = colorkey; ko++;
	}
	if(pal&&ko&&colorkey!=-1) //paranoia
	{				
		//rearrange palette so first entry is knocked out
		//and all knocked out indices are aimed at the one
					
		memmove(pal+1,pal,sizeof(DWORD)*colorkey);

		*(DWORD*)pal = 0x808000; //darkcyan 

		for(int i=0;i<size;i++)
		{	
			if(index[i]!=colorkey)
			{
				if(index[i]<colorkey) index[i]++;
			}
			else index[i] = 0; 
		}
			
		pal[0].peFlags = Somtexture::COLORKEY;			
						
		//just rewriting map for simplicity sake
		for(size_t i=0;i<256;map[i].peFlags=i++) 
		{
			map[i] = pal[i]; std::swap(map[i].peBlue,map[i].peRed);
		}
	}
	if(pal) for(int i=ko?1:0;i<256;i++) 
	{
		pal[i].peFlags = Somtexture::UNTITLED;
	}			

	return ko;		
}

void **Somtexture_inl::editor::cleanup(const Somtexture *texture, int ch, const RECT *src, int z)
{	
	size_t out = pals_s; 

	switch(ch)
	{
	case Somtexture::RGB: case Somtexture::ALPHA: out = 0; break;
	case Somtexture::PAL: case Somtexture::INDEX: out = 1; break;
	}

	SOMPAINT Sompaint = texture->server();

	if(!Sompaint||out>=pals_s) return 0;

	int w = texture->width, h = texture->height;

	if(w*h>size){ assert(0); return 0; } //unimplemented

	if(!pals[out])
	{	
		if(Sompaint->format(pals+out,"basic memory: 2d(%d,%d);",w,h))
		{
			src = 0; SetRect(dirty(ch),0,0,w,h); //hack: force full clean
		}
		else return 0;
	}
	
	RECT rect = {0,0,w,h}; if(src) CopyRect(&rect,src);

	if(src&&z>1) for(int i=0;i<4;i++) ((LONG*)&rect)[i]/=z; //zoom

	if(clean(ch,src?&rect:0)) return pals+out;

	size_t lock[4] = {rect.left,rect.top,rect.right,rect.bottom};
	 
	void *write = Sompaint->lock(pals+out,"w",lock); 
	
	if(!write){ assert(0); return 0; } //paranoia

	BYTE *mem = 0; switch(ch) 
	{
	case Somtexture::RGB: mem = &rgba->peRed; break;

	case Somtexture::PAL: case Somtexture::INDEX: mem = index; break;

	case Somtexture::ALPHA: mem = alpha?alpha:&rgba->peRed; break;	
	}
	size_t stride = mem==&rgba->peRed?4:1; assert(mem);
				
	size_t pitch = stride*w;
	size_t offset = pitch*rect.top+stride*rect.left;

	if(stride==4&&ch==Somtexture::ALPHA) offset+=3; //hack
		
	w = rect.right-rect.left; h = rect.bottom-rect.top; //yuck

	assert(!lock[0]&&lock[1]==32); //bits
	assert(lock[2]==4&&lock[3]>=w*4); //bytes

	if(mem) for(int i=0;i<h;i++)
	{			
		BYTE *p = (BYTE*)mem+offset+pitch*i;
		BYTE *q = (BYTE*)write+lock[3]*i; //pitch

		switch(ch)
		{
		case Somtexture::RGB: for(int k=0;k<w;k++)
		{
			*q++ = *p++; *q++ = *p++; *q++ = *p++; q++; p++; 
		}
		break;
		case Somtexture::ALPHA: for(int k=0;k<w;k++)
		{
			q+=3; *q++ = *p; p+=stride;
		}
		break;
		case Somtexture::PAL: for(int k=0;k<w;k++)
		{
			BYTE *r = (BYTE*)(map+*p++);

			*q++ = *r++; *q++ = *r++; *q++ = *r++; q++; 
		}
		break;
		case Somtexture::INDEX:	for(int k=0;k<w;k++)
		{
			q+=3; *q++ = *p++;
		}
		break;
		}
	}

	Sompaint->unlock(pals+out);

	return pals+out;
}

void Somtexture_inl::editor::change(const Somtexture *texture, int ch)
{		
	//assert(ch==texture->Somtexture_inl.current);

	int w = texture->width, h = texture->height;

	if(w*h>size){ assert(0); return; } //what to do?

	SOMPAINT Sompaint = texture->server();
			
	if(!texture->pal) //TODO: commandline arguments
	{	
		//rgb: Sompaint_D3D9.cpp's print function is still using GetDC
		Sompaint->format(&texture->pal,"basic buffer: 2d(%d,%d), rgb, mipmap;",w,h);
	}

	void *write = 0; size_t lock[4] = {0,0,w,h}; 

	if(write=Sompaint->lock(&texture->pal,"w",lock))
	{			
		assert(!lock[0]&&lock[1]==32); //bits
		assert(lock[2]==4&&lock[3]>=w*4); //bytes

		int pitch = lock[3]; 

		for(int i=0,j=0,k=0;i<h;i++,j=i*w)
		{
			bool picture = true;

			DWORD *p = (DWORD*)((BYTE*)write+pitch*i);
			
			switch(ch)
			{	
			case Somtexture::RGB: if(!(picture=(bool)rgba)) break;

				for(k=0;k<w;k++) p[k] = *(DWORD*)&rgba[j+k]; break;

			case Somtexture::PAL: if(!(picture=(bool)index)) break;

				for(k=0;k<w;k++) 
				{
					p[k] = 0xFF000000|*(DWORD*)&map[index[j+k]];
				}
				if(rgba||alpha) for(k=0;k<w;k++)
				{
					int a = rgba?rgba[j+k].peFlags:alpha[j+k]; p[k]|=a<<24;
				}
				break;

			case Somtexture::INDEX: if(!(picture=(bool)index)) break;
			
				for(k=0;k<w;k++)
				{
					int l = index[j+k];
					
					p[k] = 0xFF000000|l<<16|l<<8|l;
				}
				break;

			case Somtexture::ALPHA: if(!(picture=rgba||alpha)) break;

				for(k=0;k<w;k++)
				{
					int a = alpha?alpha[j+k]:rgba[j+k].peFlags;
					
					p[k] = 0xFF000000|a<<16|a<<8|a;
				}
				break;
			}

			if(!picture) for(k=0;k<w;k++) p[k] = 0xFF000000; 
		}

		Sompaint->unlock(&texture->pal);
	}	
	else assert(0);
}

#ifdef SOMTEXTURE_D3DX
static IDirect3DTexture9 *Somtexture_import
(IDirect3DDevice9 *dev, const wchar_t *src, PALETTEENTRY pal[256], D3DXIMAGE_INFO *inf)
{				
	if(!dev){ assert(0); return 0; } //TODO: Somtexture_D3D9 

	int x = D3DX_DEFAULT_NONPOW2, y = x, ck = 0; //0xFF000000;

	//Note: scratch is the only for anything not D3DFMT_A8R8G8B8
	//You will get back a palette, but there will be no index data!
	D3DPOOL pool = D3DPOOL_SCRATCH; D3DFORMAT fmt = D3DFMT_UNKNOWN;

	if(inf)
	{	
		if(D3DXGetImageInfoFromFileW(src,inf)!=D3D_OK) return 0;	
		
		x = inf->Width; y = inf->Height;
		if(x>SOMTEXTURE_MAX||y>SOMTEXTURE_MAX)
		{
			if(x>y)
			{
				float r = float(y)/x;

				x = std::min<int>(SOMTEXTURE_MAX,x); y = r*x;
			}
			else if(y>x)
			{
				float r = float(x)/y;

				y = std::min<int>(SOMTEXTURE_MAX,y); x = r*y;
			}
			else x = y = SOMTEXTURE_MAX;
		}

		if(x==inf->Width)
		{
			switch(fmt=inf->Format)
			{
			case D3DFMT_A4L4:
			case D3DFMT_L16:
			case D3DFMT_L8: case D3DFMT_A8: 
				
				fmt = D3DFMT_A8L8; break;

			case D3DFMT_P8: case D3DFMT_A8P8:

				fmt = D3DFMT_A8P8; break;

			default: fmt = D3DFMT_A8R8G8B8; 
			}
		}
		else fmt = D3DFMT_A8R8G8B8; 
	}

	DWORD f = D3DX_FILTER_NONE;	

	if(inf&&x!=inf->Width) f = D3DX_FILTER_MIRROR|D3DX_FILTER_TRIANGLE; 

	IDirect3DTexture9 *out = 0; 
	
	HRESULT err = D3DXCreateTextureFromFileExW(dev,src,x,y,1,0,fmt,pool,f,D3DX_DEFAULT,ck,inf,pal,&out);		

	switch(err)
	{
	case D3DERR_INVALIDCALL: err=err; break;	
	case D3DERR_NOTAVAILABLE: err=err; break;
	case D3DERR_OUTOFVIDEOMEMORY: err=err; break;
	}

	return err==D3D_OK?out:0;
}
#endif //SOMTEXTURE_D3DX

static int Somtexture_type(const wchar_t *file)
{
	if(!file) return 0;

	//if(!PathFileExistsW(file)) return 0;

	wchar_t ext[5] = {0,0,0,0,0}; 
	wcslwr(wcsncpy(ext,PathFindExtensionW(file),4)); if(*ext!='.') return 0;

	switch(ext[1])
	{
	case 'p': if(!wcscmp(ext,L".pal")) return 'pal';
	case 't': if(!wcscmp(ext,L".txr")) return 'txr';
			  if(!wcscmp(ext,L".tim")) return 'tim';

#ifdef SOMTEXTURE_D3DX

	//case 'd': if(!wcscmp(ext,L".dds")) return 'dds';

	default: return 'dds'; //hack??

#endif	
	}

	return 0;
}

Somtexture *Somtexture_edit(int pool, const wchar_t *file, Somtexture *out=0) 
{		
	if(out) out->~Somtexture();  
	if(out) out->Somtexture_inl.current = 0;
	
	Somtexture_cpp::poolhelper ph(pool); if(!ph) return out;
	
	if(out) assert(out->Somtexture_inl.pool==ph);
	
	if(!file)
	{
		if(!out) out = new Somtexture(ph);

		out->Somtexture_inl.edit = new Somtexture_inl::editor;
		
		return out;
	}

	SOMPAINT Sompaint = 0;
	{
		Somthread_h::section cs;

		Sompaint = Somtexture_cpp::pools[ph].server;
	}

#ifdef SOMTEXTURE_D3DX

	IDirect3DTexture9 *p = 0; 

#endif

	PALETTEENTRY *palette = 0;	   	

	size_t pitch = 0, ipitch = 0;
	size_t stride = 0, istride = 0;	
	size_t width = 0, height = 0, ko = 0; 

	const BYTE *rgb = 0, *index = 0, *alpha = 0;	

	BYTE *tmp = 0; //careful not to leak

	int use = 0, type = Somtexture_type(file);

	swordofmoonlight_lib_image_t img;

	switch(type)
	{
	//Reminder: neglecting Big-Endian
	using namespace SWORDOFMOONLIGHT;

	case 'pal':
	{
	pal: assert(0); //unimplemented

	}break;
	case 'txr':
	{
		//TODO: determine that TXR is not a PAL
		if(0) goto pal;

		txr::maptofile(img,file); 
		
		txr::header_t &hd = txr::imageheader(img); if(!hd) break;

		const txr::mipmap_t &map = txr::mipmap(img,0);
		const txr::pixrow_t &pix = txr::pixrow(map,0);

		txr::palette_t *cp = txr::palette(img); 
						
		if(cp) palette = new PALETTEENTRY[256];
		if(palette)	for(int i=0;i<256;i++) ((DWORD*)palette)[i] = !cp[i];

		width = hd.width; height = hd.height; 

		if(hd.depth==8&&pix)
		{
			index = pix.indices; ipitch = map.rowlength; istride = 1;
		}
		if(hd.depth==24&&pix)
		{
			rgb = (BYTE*)&pix.truecolor; pitch = map.rowlength; stride = 3; 
		}		

	}break;
	case 'tim':
	{
		int sub = -1; //hack...

		const swordofmoonlight_lib_image_t &in2 = img; 

		const wchar_t *name = PathFindFileNameW(file);

		if(name[0]=='_' //Assuming embedded MDL notation
		 &&name[1]>='1'&&name[1]<='9'&&!wcsnicmp(name+2,L".tim",5))
		{
			wchar_t root[MAX_PATH]; wcscpy_s(root,file); 

			root[name>file?name-file-1:0] = '\0'; //name>file: paranioa
			
			mdl::maptofile(img,root,'r'); //TODO: access TIM directly

			sub = name[1]-'1'; //timsubimageblock...
		}
		else tim::maptofile(img,file); if(!img) break;
		
		tim::image_t img = //shadowing (hack)
		sub<0?in2:mdl::maptotimblock(img,in2,sub); //#1

		tim::header_t &hd = tim::imageheader(img);

		tim::pixmap_t &clut = tim::clut(img), &data = tim::data(img);

		if(sub>=0) tim::unmap(img); if(!hd) break; //#2

		tim::pixrow_t &pix = tim::pixrow(data,0);

		const tim::highcolor_t *cp = tim::pixrow(clut,0).highcolor;

		if(cp) palette = new PALETTEENTRY[256];

		if(palette&&hd.pmode==0)
		memset(palette+16,0x00,sizeof(PALETTEENTRY)*(256-16));

		if(palette) for(int i=0,n=hd.pmode?256:16;i<n;i++) 
		{
			*(tim::truecolor_t*)(palette+i) = cp[i]; //palette[i].peFlags = 0;
		}

		width = data(hd.pmode); height = data?data.h:0;

		size_t mode = hd.pmode, size = width*height; if(size==0) break;
				
		tmp = new BYTE[size*(mode>1?3:1)]; 
		
		if(mode>1) rgb = tmp; else index = tmp;

		switch(mode)	
		{
		case tim::codexmode: //4bpp
		case tim::indexmode: //8bpp
		
			ipitch = width; istride = 1; break;

		case tim::highcolormode: //16bpp
		case tim::truecolormode: //24bpp
			
			pitch = width*3; stride = 3; break;
		}
				
		BYTE *idst = tmp;
		tim::truecolor_t *dst = (tim::truecolor_t*)tmp;

		for(int i=0;i<height;i++) 
		{
			switch(mode)	
			{
			case tim::codexmode: //4bpp
			{
				const tim::codex_t *src = data[i].codices;

				for(int j=0;j<width;j+=2,src++)
				{
					*idst++ = src->lo; *idst++ = src->hi;
				}

			}break;
			case tim::indexmode: //8bpp
			{
				const tim::index_t *src = data[i].indices;

				for(int j=0;j<width;j++,src++) *idst++ = *src;

			}break;
			case tim::highcolormode: //16bpp
			{
				const tim::highcolor_t *src = data[i].highcolor;

				for(int j=0;j<width;j++,src++) *dst++ = !*src;

			}break;
			case tim::truecolormode: //24bpp
			{
				const tim::truecolor_t *src = data[i].truecolor;

				for(int j=0;j<width;j++,src++) *dst++ = !*src;

			}break;
			}
		}		

	}break;
	default:
	{
	#ifdef SOMTEXTURE_D3DX

		IDirect3DDevice9 *dev = 0; 
				
		D3DXIMAGE_INFO inf;	PALETTEENTRY cp[256];

		Sompaint->expose(0,"IDirect3DDevice9*",&dev);
				
		p = Somtexture_import(dev,file,cp,&inf); 
		
		if(dev) dev->Release(); if(!p) break;

		D3DSURFACE_DESC desc; D3DLOCKED_RECT lock;

		if(p->GetLevelDesc(0,&desc)==D3D_OK)
		{			
			width = desc.Width; height = desc.Height;

			switch(desc.Format)
			{
			case D3DFMT_A8P8: //case D3DFMT_P8: 
				
				memcpy(palette=new PALETTEENTRY[256],cp,sizeof(cp));
			}

			if(p->LockRect(0,&lock,0,0)==D3D_OK)
			{
				switch(desc.Format)
				{
				case D3DFMT_A8P8: istride = 2; ipitch = lock.Pitch;
			
					index = (BYTE*)lock.pBits; alpha = index+1; break;

				case D3DFMT_A8R8G8B8: stride = 4; pitch = lock.Pitch;
					
					rgb = (BYTE*)lock.pBits; break;
				}
			}
		}		
	#endif //SOMTEXTURE_D3DX
	}
	}//switch

	if(pitch&&stride||ipitch&&istride)
	{		
		if(!out) out = new Somtexture(pool);

		out->width = width; out->height = height; 
		
		out->palette = (Sompalette*)palette; assert(width&&height);		
		
		Somtexture_inl::editor *ed = 
		out->Somtexture_inl.edit = new Somtexture_inl::editor;

		out->Somtexture_inl.current = use;
		ed->fillout(out,rgb,pitch,stride,index,ipitch,istride,alpha);
		use = out->Somtexture_inl.current;

		if(type!='pal') ko = ed->knockout(palette);		

		//scheduled obsolete
		Somtexture_cpp::wellhelper(ed->wells,out->palette); 

		ed->change(out,use);
	}
	
#ifdef SOMTEXTURE_D3DX

	if(p) p->UnlockRect(0); 
	if(p) p->Release();	

#endif

	if(type!='pal'&&out) //importing...
	{
		out->title(out->well(Somtexture::COLORKEY),L"COLORKEY",9);
		out->title(out->well(Somtexture::UNTITLED),L"UNTITLED",9);
		out->title(out->well(Somtexture::RESERVED),L"RESERVED",9);

		const wchar_t *logf =
		L"~PalEdit import log [%s] ...\n"
		L"~%s\n"
		L"~%d x %d [%d pixels]\n"
		L"~Palette found [%s]\n"
		L"~Colorkey [%d pixels]\n"				
		L"~^Z\n";

		const int notetext_s = MAX_PATH*2; wchar_t notetext[notetext_s];

		wchar_t day[32]; time_t t = ::time(0); wcsftime(day,32,L"%B %#d %Y",localtime(&t));
		if(swprintf_s(notetext,logf,day,file,width,height,width*height,palette?L"yes":L"no",ko)>0)
		{
			out->notes(0,notetext_s,0,notetext);
		}			
	}
		
	if(type!='dds') 
	swordofmoonlight_lib_unmap(&img); 

	delete [] tmp;
	
	return out;
}										  
  
static Somtexture*
Somtexture_txr(int pool, const SWORDOFMOONLIGHT::txr::image_t &in, Somtexture *out=0)
{	
	//Reminder: neglecting Big-Endian
	using namespace SWORDOFMOONLIGHT;

	const txr::header_t &head = txr::imageheader(in); 

	const txr::mipmap_t top = txr::mipmap(in,0); if(!head||!top) return out;

	const txr::palette_t *pal = txr::palette(in);

	int w = top.width, h = top.height; if(top.depth!=24&&!pal) return out;

	if(!out) out = new Somtexture(pool);

	out->Somtexture_inl.current = Somtexture::RGB;
	
	SOMPAINT Sompaint = out->server(); assert(Sompaint);
	
	//rgb: Sompaint_D3D9.cpp's print function is still using GetDC
	Sompaint->format(&out->pal,"basic buffer: 2d(%d,%d), rgb;",w,h);

	void *write = 0; size_t lock[4] = {0,0,w,h}; 
		
	if(write=Sompaint->lock(&out->pal,"w",lock))
	{					
		out->width = w; out->height = h;

		out->Somtexture_inl.supported = Somtexture::RGB; 

		assert(!lock[0]&&lock[1]==32); //bits
		assert(lock[2]==4&&lock[3]>=w*4); //bytes

		int i, depth = top.depth, pitch = lock[3]; 

		for(i=0;i<h;i++) 
		{
			DWORD *dst = 
			(DWORD*)((BYTE*)write+pitch*i); 

			switch(depth)	
			{
			case 8: //8bpp
			{
				const txr::index_t *src = top[i].indices;

				for(int j=0;j<w;j++,src++) *dst++ = pal[*src]; 

			}break;
			case 24: //24bpp
			{
				const txr::truecolor_t *src = top[i].truecolor;

				for(int j=0;j<w;j++,src++) *dst++ = *src;

			}break;
			}
		}

		Sompaint->unlock(&out->pal);		
	}
	else assert(0);

	return out;
}
		
static Somtexture*
Somtexture_tim(int pool, const SWORDOFMOONLIGHT::tim::image_t &in, Somtexture *out=0)
{
	//Reminder: neglecting Big-Endian
	using namespace SWORDOFMOONLIGHT; 

	const tim::header_t &head = tim::imageheader(in); if(!head) return out;

	const tim::pixmap_t &clut = tim::clut(in);
	const tim::pixmap_t &data = tim::data(in); if(!in||!data) return out;
	
	int w = data(head.pmode), h = data.h;

	const tim::highcolor_t *pal = clut?clut[0].highcolor:0;

	if(!out) out = new Somtexture(pool); 

	out->Somtexture_inl.current = Somtexture::RGB;
	
	SOMPAINT Sompaint = out->server(); assert(Sompaint);
	
	//rgb: Sompaint_D3D9.cpp's print function is still using GetDC
	Sompaint->format(&out->pal,"basic buffer: 2d(%d,%d), rgb;",w,h);

	void *write = 0; size_t lock[4] = {0,0,w,h}; 
		
	if(write=Sompaint->lock(&out->pal,"w",lock))
	{
		out->width = w; out->height = h;

		out->Somtexture_inl.supported = Somtexture::RGB; 
		
		assert(!lock[0]&&lock[1]==32); //bits
		assert(lock[2]==4&&lock[3]>=w*4); //bytes 

		tim::truecolor_t tmp;

		int i, mode = head.pmode, pitch = lock[3];

		for(i=0;i<h;i++) 
		{
			DWORD *dst = 
			(DWORD*)((BYTE*)write+pitch*i);

			switch(mode)	
			{
			case tim::codexmode: //4bpp
			{
				const tim::codex_t *src = data[i].codices;

				for(int j=0;j<w;j+=2,src++)
				{
					int m = src->lo, n = src->hi;

					if(pal) tmp = pal[m]; else tmp = m*17; *dst++ = !tmp; 
					if(pal) tmp = pal[n]; else tmp = n*17; *dst++ = !tmp; 
				}

			}break;
			case tim::indexmode: //8bpp
			{
				const tim::index_t *src = data[i].indices;

				for(int j=0;j<w;j++,src++)
				{
					if(pal) tmp = pal[*src]; else tmp = *src; *dst++ = !tmp; 
				}

			}break;
			case tim::highcolormode: //16bpp
			{
				const tim::highcolor_t *src = data[i].highcolor;

				for(int j=0;j<w;j++,src++)
				{
					tmp = *src; *dst++ = !tmp; 
				}

			}break;
			case tim::truecolormode: //24bpp
			{
				const tim::truecolor_t *src = data[i].truecolor;

				for(int j=0;j<w;j++,src++) *dst++ = !*src;

			}break;
			}
		}

		Sompaint->unlock(&out->pal);		
	}
	else assert(0);

	return out;
}

#ifdef SOMTEXTURE_D3DX

static Somtexture*
Somtexture_dds(int pool, const wchar_t in[MAX_PATH], Somtexture *out=0)
{
	D3DXIMAGE_INFO inf;

	if(D3DXGetImageInfoFromFileW(in,&inf)!=D3D_OK) return out;	
										   	
	int w = 0, h = 0;

	if(inf.Width>inf.Height)
	{
		float r = float(inf.Height)/inf.Width;

		w = std::min<int>(SOMTEXTURE_MAX,inf.Width); h = r*w;
	}
	else if(inf.Height>inf.Width)
	{
		float r = float(inf.Width)/inf.Height;

		h = std::min<int>(SOMTEXTURE_MAX,inf.Height); w = r*h;
	}
	else w = h = std::min<int>(SOMTEXTURE_MAX,inf.Width);

	if(!out) out = new Somtexture(pool);

	out->Somtexture_inl.current = Somtexture::RGB;

	SOMPAINT Sompaint = out->server(); assert(Sompaint);

	//rgb: Sompaint_D3D9.cpp's print function is still using GetDC
	Sompaint->format(&out->pal,"basic buffer: 2d(%d,%d), rgb;",w,h);

	IDirect3DSurface9 *top = 0;
	
	if(Sompaint->expose(&out->pal,"IDirect3DSurface9*",&top))
	{
		RECT dest = {0,0,w,h};

		DWORD f = D3DX_FILTER_MIRROR|D3DX_FILTER_TRIANGLE;
		
		if(top&&D3DXLoadSurfaceFromFileW(top,0,&dest,in,0,f,0,0)==D3D_OK)
		{
			out->Somtexture_inl.supported = Somtexture::RGB; 

			out->width = w; out->height = h; 
		}
		else assert(0);
	}
	else //TODO: Somtexture_D3D9
	{
		assert(0); //TODO: if(D3DXCreateTextureFromFile) Sompaint->lock;
	}

	if(top) top->Release();
	
	return out;
}

#endif //SOMTEXTURE_D3DX

static Somtexture*
Somtexture_open(int pool, const wchar_t *file, Somtexture *out=0) 
{
	if(out) out->~Somtexture();  
	if(out) out->Somtexture_inl.current = Somtexture::RGB;
	
	Somtexture_cpp::poolhelper ph(pool); if(!ph) return 0;
	
	if(out) assert(out->Somtexture_inl.pool==ph);
		
	if(!file) return out?out:new Somtexture(ph);
		
	int filetype = Somtexture_type(file);

#ifdef SOMTEXTURE_D3DX

	if(filetype=='dds')
	{
		return out = Somtexture_dds(ph,file,out);
	}

#endif
			
	using namespace SWORDOFMOONLIGHT;

	swordofmoonlight_lib_image_t img; 

	switch(filetype)
	{
	case 'pal': assert(0); break; //unimplemented

			  //pal::maptofile(img,file); break;
	case 'txr': txr::maptofile(img,file); break;
	case 'tim': tim::maptofile(img,file); break;
	}

	if(img) switch(filetype)
	{
	//case 'pal': out = Somtexture_pal(ph,img,out); break;
	case 'txr': out = Somtexture_txr(ph,img,out); break;
	case 'tim': out = Somtexture_tim(ph,img,out); break;
	}

	swordofmoonlight_lib_unmap(&img);

	return out;
}

//static
const Somtexture *Somtexture::open(int pool, const wchar_t *file, size_t file_s) 
{
	assert(file_s==size_t(-1)); //unimplemented

	return Somtexture_open(pool,file,0);
}

//static
const Somtexture *Somtexture::edit(int pool, const wchar_t *file, size_t file_s) 
{
	assert(file_s==size_t(-1)); //unimplemented

	return Somtexture_edit(pool,file,0);
}

bool Somtexture::refresh(const wchar_t *file, size_t file_s, bool alt)const
{
	if(!this) return false;

	assert(file_s==size_t(-1)); //unimplemented

	if(readonly()||alt) 
	Somtexture_open(pool(),file,const_cast<Somtexture*>(this));
	Somtexture_edit(pool(),file,const_cast<Somtexture*>(this));

	return !file||Somtexture_inl.supported;
}	 

//static
const wchar_t *Somtexture::menu(int item, int mode)
{		
	return item>1||item<-1?0:L"Direct3D9"; /*shelving for now

	Somthread_h::section cs;

	if(!Somtexture_cpp::menu.size()) //intialize
	{
		//TODO: synchronize menu with the registry

		wchar_t Somplayer[MAX_PATH] = L"";
		if(GetModuleFileNameW(Somplayer_dll(),Somplayer,MAX_PATH))
		{
			const wchar_t *(*proc)(int) = 0;
			*(void**)&proc = GetProcAddress(Somplayer_dll(),"Enumerate_Somshader_h_applets");

			if(proc)
			{
				const wchar_t *e = proc(0); assert(e);
			
				//hack: menu[0] holds the default PAL implementation
				Somtexture_cpp::menu.push_back(Somtexture_cpp::item(Somplayer,0,e));

				for(int i=0;e=proc(i);i++)
				Somtexture_cpp::menu.push_back(Somtexture_cpp::item(Somplayer,i,e));				
			}
			else assert(0);
		}
		else assert(0);

		//TODO: check for available PAL modules
	}

	if(item<0) item = -item; //either way works

	if(item>=Somtexture_cpp::menu.size()) return 0;

	switch(mode)
	{	
	case Somtexture::LOCAL: //localized (unimplemented)

	default: return Somtexture_cpp::menu[item].longname.c_str();

	case Somtexture::SHORT: //undecorated unlocalized description

		return Somtexture_cpp::menu[item].shortname.c_str();
	}
	*/
}

namespace Somtexture_cpp
{
	struct pixel
	{
		unsigned hash;

		std::vector<int> subformat;

		size_t bits_per_pixel; int format;

		wchar_t *description, descriptionlen;

		static unsigned djb2(const int *formats, size_t s)
		{
			unsigned out = 5381; //unsigned
			for(size_t i=0;i<s;i++) out = ((out<<5)+out)+formats[s];
			return out;	
		}
				
		pixel(const int *f, size_t f_s,
				
			const wchar_t *text, wchar_t text_s) : subformat(f_s)
		{				
			hash = djb2(f,f_s); bits_per_pixel = 0;
			
			if(f_s>0) format = f[0]; else assert(0);
			for(size_t i=1;i<f_s;i++) if(f[i]!=f[0]) format = Somtexture::MIXED;

			for(size_t i=0;i<f_s;i++) switch(subformat[i]=f[i]) //!
			{	
			default: if(f[i]>=1&&f[i]<=128) bits_per_pixel+=f[i]; 

			else if(f[i]<=-2&&f[i]>=-128) bits_per_pixel+=-f[i]; else assert(0); break;

			case Somtexture::RGB8: case Somtexture::RGBA8: bits_per_pixel+=8*4; break;

			case Somtexture::HIGHP: bits_per_pixel+=32; break; //32bit floating point
			case Somtexture::MEDP: bits_per_pixel+=16; break; //16bit floating point
			}		

			description = L"no description provided"; descriptionlen = 0; 

			if(text&&text_s) 
			{
				wcsncpy(description=new wchar_t[text_s+1],text,text_s);				
				description[descriptionlen=text_s] = 0;

				Somtexture_pch::remember(description);
			}
			else assert(!text&&!text_s);
		}
		
	private: pixel(){} friend std::map<int,pixel>;
	};

	typedef std::map<int,pixel>::iterator pixel_it;
	
	std::map<int,pixel> pixels;
}

//auto generate pixels for unit and homogeneous formats
static int Somtexture_pixel(int ch, int format=0, int depth=0)
{
	if(format==Somtexture::MIXED) return 0;

	switch(ch) //defaults and pruning
	{
	case Somtexture::MAP: //palette
		
		if(!format) format = Somtexture::RGB8;

		if(format!=Somtexture::RGB8) return 0; break;
		
	case Somtexture::PAL: case Somtexture::RGB: //colour
		
		if(!format) format = Somtexture::RGB8;

		if(format!=Somtexture::RGB8&&format!=Somtexture::RGBA8) return 0; break;
	
	case Somtexture::ALPHA: if(!format) format = Somtexture::GREY8;

		if(format!=Somtexture::GREY8) return 0; break;
	
	case Somtexture::DEPTH: if(!format) format = 32; 

		if(format!=16&&format!=32) return 0; break; //depth buffer formats
	
	case Somtexture::STENCIL: if(format>1) return 0; //bit limited for now

		if(format==0) format = 1; break; //default to 1

	case Somtexture::INDEX: if(!format) format = 8; //palette / vertex index

		if(format!=8&&format!=16&&format!=32) return 0; break;
	
	case Somtexture::XYZ: break; //vertex buffer

	//// for completeness sake /////////////////////

	case Somtexture::NOTES: //whatever
		
		if(!format) format = sizeof(wchar_t)/sizeof(char)*CHAR_BIT;

		if(format!=sizeof(wchar_t)/sizeof(char)*CHAR_BIT) return 0; break;
	
	case Somtexture::BLACK: if(!format) format = 9; //9bit

		if(format!=9) return 0; break;

	default: assert(0); return 0;
	}		

	if(!Somtexture::known_format(format)) return 0;
		
	int px = 0, x = 1;	

	int unit = Somtexture::bits_per_pixel(format);

	if(depth&&depth!=unit)
	{
		if(depth%unit||depth/unit>32) return 0; 
		
		px = format|(depth/unit)<<8;
	}
	else px = format; if(!px) return 0; //paranoia

	Somthread_h::section cs;

	Somtexture_cpp::pixel_it it = Somtexture_cpp::pixels.find(px);	

	if(it!=Somtexture_cpp::pixels.end()) return px;

	int homo[32]; for(int i=0;i<x;i++) homo[i] = format;

	//TODO: generate a description per Somtexture::describe_pixel
	Somtexture_cpp::pixels[px] = Somtexture_cpp::pixel(homo,x,0,0);

	return px;
}

//static
int Somtexture::register_pixel(const int *subformat, size_t s, const wchar_t *description, size_t t) 
{
	static const int user_pixels = 0x10000;

	if(!subformat||!s||s>32||t>1024){ assert(0); return 0; } //paranoia
	
	for(int i=0;i<s;i++) if(!known_format(subformat[i])||subformat[i]==MIXED) return 0;

	Somthread_h::section cs;

	unsigned hash = Somtexture_cpp::pixel::djb2(subformat,s);

	Somtexture_cpp::pixel_it it = Somtexture_cpp::pixels.find(user_pixels);
	
	if(!description) t = 0; else if(!t) description = 0; //paranoia

	for(;it!=Somtexture_cpp::pixels.end();it++)
	{
		if(it->second.hash!=hash) continue;

		if(it->second.descriptionlen!=t) continue;

		size_t n = it->second.subformat.size(); if(n!=s) continue;			

		for(size_t i=0;i<n;i++) if(it->second.subformat[i]!=subformat[i]) continue;

		if(wcsncmp(it->second.description,description,t)) continue;			

		return it->first; //was previously registered
	}

	int out = user_pixels+Somtexture_cpp::pixels.size();

	Somtexture_cpp::pixels[out] = Somtexture_cpp::pixel(subformat,s,description,t);

	return out;
}

//static
size_t Somtexture::bits_per_pixel(int px) 
{		
	switch(px) //optimization 
	{
	case 0: case MIXED: return 0;

	default: if(px>=1&&px<=128) return px; 
		
		if(px<=-2&&px>=-128) return -px; /*signed notation*/ break;

	case RGB8: case RGBA8: case HIGHP: return 32; case MEDP: return 16;
	}

	Somthread_h::section cs;

	Somtexture_cpp::pixel_it it = Somtexture_cpp::pixels.find(px);

	if(it==Somtexture_cpp::pixels.end()) return 0;

	return it->second.bits_per_pixel;
}

//static
size_t Somtexture::describe_pixel(int px, int *subformat, size_t s, const wchar_t **description) 
{
	Somthread_h::section cs;

	Somtexture_cpp::pixel_it it = Somtexture_cpp::pixels.find(px); assert(subformat);

	if(it==Somtexture_cpp::pixels.end()) 
	{
		//hack: expecting XYZ to accept all known formats
		if(known_format(px)) px = Somtexture_pixel(XYZ,px); else return 0;

		it = Somtexture_cpp::pixels.find(px); if(it==Somtexture_cpp::pixels.end()) return 0;
	}

	if(subformat)
	for(size_t i=0,n=it->second.subformat.size();i<s&&i<n;i++) subformat[i] = it->second.subformat[i];

	if(description) *description = it->second.description;

	return it->second.subformat.size();
}

int Somtexture::pixel(int ch)const
{
	if(!this) return 0;

	if(ch&~Somtexture_inl.supported||!known_channel(ch)) return 0;

	int &out = Somtexture_inl.pixel(ch);

	if(!out) out = Somtexture_pixel(ch); assert(out);

	return out;
}

int Somtexture::primary()const
{
	if(!this) return 0;

	return Somtexture_inl.current;
}

int Somtexture::channel(int ch)const 
{
	if(!this) return 0;

	//todo: should be defering to pixel(ch)

	//baking in formats per channel for now
	switch(Somtexture_inl.supported&ch?ch:-1) 
	{
	case -1: return 0; //not supported

	case RGB: case PAL: case MAP: return RGB8;
	
	case INDEX: case ALPHA: return GREY8;

	case BLACK: return 9; //GREY9

	case NOTES: return sizeof(wchar_t)*CHAR_BIT;

	default: assert(0); return 0; 
	}
}

size_t Somtexture::memory(int ch, int spec)const
{
	if(!this) return 0;

	//todo: should be defering to pixel(ch)

	ch = ch&Somtexture_inl.supported;
	ch|=spec&=~Somtexture_inl.supported;

	switch(ch) //pixel(ch)
	{
	case RGB: return width*height*3;

	case ALPHA: return width*height;

	case PAL: case MAP: return 256*4;

	case INDEX: return width*height;

	case NOTES: if(!Somtexture_inl.edit) return 0;
	{
		Somtexture_cpp::notehelper nh(Somtexture_inl->notes);
		
		return nh?nh[nh.SZ]*sizeof(wchar_t):0;
	}
	case BLACK: //unimplemented...
	
	case 0: return 0;
	}

	size_t out = 0;

	for(int i=0;i<32;i++) 
	{
		int bit = ch&(1<<i);

		if(bit==ch) //unrecognize format
		{
			assert(0); return 0;
		}
				
		out+=memory(bit);
	}

	return out;
}

bool Somtexture::format(int ch, int x, int y, int depth, int format, int pixel, int tx, int ty)const
{
	if(!this) return false;	

	if(Somtexture_inl.locks&ch) return false; 

	if(!pixel) pixel = Somtexture_pixel(ch,format,depth); assert(pixel);
	if(!pixel) return false;

	int paranoia = format;

	//TODO: access Somtexture_cpp::pixels directly
	size_t bpp = Somtexture::bits_per_pixel(pixel); 
	size_t len = Somtexture::describe_pixel(pixel,&format,1);

	if(!len||depth&&depth!=bpp //some sanity checks
	||paranoia&&format!=paranoia&&paranoia!=MIXED){	assert(0); return false; }

	if(tx<x) tx = x; if(ty<y) ty = y; if(x<=0||y<=0) return false;

	//Are we merely resizing the buffer?
	if(Somtexture_inl.width==tx&&Somtexture_inl.height==ty)
	{
		if(Somtexture_inl.supported==ch&&Somtexture_inl.current==ch) 
		{
			if(Somtexture_inl.pixel(ch)==format) 
			{					
				const_cast<int&>(width) = x; const_cast<int&>(height) = y;

				return true; //yes we are
			}
		}
	}

	switch(ch)
	{
	default: assert(0); return false;

	case Somtexture::XYZ: //assuming vertex buffer
	{
		Somtexture_inl.supported&=~ch;

		if(ty!=1||!Sompaint->format(&pal,"point buffer: 1d(%d,%d);",tx,bpp)) return false;

	}break;
	case Somtexture::INDEX: //assumind index buffer
	{		
		Somtexture_inl.supported&=~ch;
		
		if(len!=1||bpp%8||bpp>32) return false; //paranoia

		if(ty!=1||!Sompaint->format(&pal,"index buffer: 1d(%d,%d);",tx,bpp)) return false;
		
	}break;
	case Somtexture::RGB: //assuming render target
	{
		Somtexture_inl.supported&=~ch;

		if(len!=1||bpp!=32) return false; //paranoia

		char *a = ""; 
		
		switch(format)
		{
		case Somtexture::RGBA8: a = "a"; case Somtexture::RGB8: break;
		
		default: assert(0); return false;
		}

		//TODO: commandline arguments
		if(!Sompaint->format(&pal,"frame buffer: 2d(%d,%d,32), rgb%s(32);",tx,ty,a)) return false;
		
	}break;
	}	

	Somtexture_inl.width = tx; Somtexture_inl.height = ty;

	const_cast<int&>(width) = x; const_cast<int&>(height) = y;

	Somtexture_inl.supported|=ch; assert(Somtexture_inl.supported==ch);

	if(Somtexture_inl.supported==ch) Somtexture_inl.current = ch;	

	Somtexture_inl.pixel(ch) = pixel; 
	
	return true;
}

namespace Somtexture_cpp
{		
	struct key
	{
		const void *texture; int channel; 

		bool operator<(const key &cmp)const
		{
			return texture==cmp.texture?channel<cmp.channel:texture<cmp.texture;
		}
	};

	struct lock{ char *mem; size_t box[4]; };
			 
	typedef std::map<key,lock>::iterator lock_it;
	
	//Why not std::vector? //Why not std::vector? //Why not std::vector?
	std::map<key,lock> locks;
}

bool Somtexture::buffer(int ch, void *set, size_t set_pitch, const RECT *dst, bool locking)const
{
	if(!this) return false; //!set
	
	if(ch&~Somtexture_inl.supported) return false;
			
	int locked = Somtexture_inl.locks&ch; assert(readonly());
		
	RECT rect; size_t bpp = depth(ch), Bpp = bpp/CHAR_BIT; assert(Bpp*CHAR_BIT==bpp);

	if(dst&&!locked&&!locking) CopyRect(&rect,dst); else SetRect(&rect,0,0,width,height);	

	int x = dst?dst->left:0, y = dst?dst->top:0;
	
	int w = dst?dst->right-x:width, h = dst?dst->bottom-y:height;

	if(x<0||y<0||w<0||h<0){ assert(0); return false; } //battery of tests!

	if(!set_pitch&&w>1||x+w>width||y+h>height){ assert(0); return false; }

	if(set_pitch&&set_pitch*CHAR_BIT<w*bpp&&set){ assert(0); return false; }

	Somtexture_cpp::key key = {this,ch};
	Somtexture_cpp::lock lock = {0,{rect.left,rect.top,rect.right,rect.bottom}};
	
	if(locked)
	{
		Somthread_h::section cs;

		Somtexture_cpp::lock_it it = Somtexture_cpp::locks.find(key);

		if(it==Somtexture_cpp::locks.end()){ assert(0); return false; }

		lock = it->second;
	}
	else if(lock.mem=(char*)Sompaint->lock(&pal,"w",lock.box))
	{
		//todo: select the appropriate plane
		assert(ch==Somtexture_inl.supported); 
	}
	else assert(0);
	
	if(!lock.mem) return false;

	bool out = true;
		
	//Reminder: may have to do conversion

	if(lock.box[1]==bpp&&lock.box[0]==0
	 &&lock.box[2]==Bpp&&lock.box[3]>=Bpp*width)
	{			
		if(h==1) //testing
		{
			if(x!=rect.left)
			{
				assert(rect.left==0);

				if(set) memcpy(lock.mem+x*Bpp,set,w*Bpp);
			}
			else if(set) memcpy(lock.mem,set,w*Bpp);
		}
		else assert(0); //unimplemented
	}
	else //unsafe
	{
		assert(0); //unimplemented??

		out = locking = false; 	
	}	

	if(!locking)
	{
		Sompaint->unlock(&pal);

		Somtexture_inl.locks&=~ch;		

		Somthread_h::section cs;

		if(locked) Somtexture_cpp::locks.erase(key);
	}
	else if(!locked)
	{
		Somthread_h::section cs;

		Somtexture_cpp::locks[key] = lock;

		Somtexture_inl.locks|=ch;
	}
					
	return out;
}

bool Somtexture::present(int ch, HWND window, const RECT *src, int zoom, const RECT *dst, int mask)const
{
	if(!this||!window||zoom<1) return false; if(!ch) ch = primary();
		
	RECT stmp; if(src) stmp = *src; else SetRect(&stmp,0,0,width*zoom,height*zoom); 
		
	RECT dtmp; if(dst) dtmp = *dst; else if(!GetClientRect(window,&dtmp)) return false;
		
	Somtexture_cpp::pool::thread *tls = 0;
	{
		Somthread_h::section cs;

		tls = Somtexture_cpp::pools[Somtexture_inl.pool].tls();
	}

	SOMPAINT_BOX sbox = Sompaint->window(&tls->src,&stmp);
	SOMPAINT_BOX dbox = Sompaint->window(&tls->dst,&dtmp);
	SOMPAINT_PIX dpix = Sompaint->raster(&tls->pix,&window);

	if(ch&MAP&&ch!=PAL) //MAP mode
	{
		PALETTEENTRY *p = 0;

		int len = 256, wrap = ch>>MAP, mod; 
		
		p = palette?*palette:0; if(!p||wrap>256) return false; 		
				
		if(!wrap) //TODO: solve wrap/len (see comments)
		{				
			assert(wrap); return false; //unimplemented
		}
		else //TODO: clamp src to destination (palette and all)
		{
			if(!src) SetRect(&stmp,0,0,wrap*zoom,len/wrap*zoom);
		}		

		if(!tls->pal)
		{
			Sompaint->format(&tls->pal,"basic memory: 1d(256), rgb = bgr(32);");			
		}
		
		void *wr = 0; size_t lock[4] = {0,0,len,1};

		if(wr=Sompaint->lock(&tls->pal,"w",lock))
		{	
			assert(!lock[0]&&lock[1]==32); //bits
			assert(lock[2]==4&&lock[3]>=len*4); //bytes

			memcpy(wr,p,sizeof(PALETTEENTRY)*len);

			Sompaint->unlock(&tls->pal);
		}
		else assert(0);

		if(mod=len%wrap) //unimplemented
		{
			assert(0); //TODO: two pass fill with irregular bit in the last row

			return false;
		}
		else //first pass 
		{
			return Sompaint->pal(tls)->fill(len,len,wrap,zoom,sbox,dbox,dpix);
		}
	}
	else
	{
		void **pal = &this->pal;
		
		if(ch!=Somtexture_inl.current&&!readonly()) //paranoia
		{
			const int greyscale = Somtexture::ALPHA|Somtexture::INDEX;

			pal = Somtexture_inl->cleanup(this,ch,&stmp,zoom); assert(pal);

			if(pal) Sompaint->format(pal,"rgba = %s;",ch&greyscale?"aaa":"rgb");
		}

		return Sompaint->buffer(pal)->present(zoom,sbox,dbox,dpix);
	}
}

bool Somtexture::readonly()const
{
	return !this||!Somtexture_inl.edit;
}

bool Somtexture::use(int ch)const
{	
	if(readonly()) return false;

	if(!primary_candidate(ch)) return false;

	if(ch==Somtexture_inl.current) return false; //already primary
			
	Somtexture_inl->change(this,ch);

	Somtexture_inl.current = ch; 
	
	return true;
}

bool Somtexture::add(int ch)const
{
	if(readonly()) return false;

	int cmp = Somtexture_inl.supported;

	if(ch<0) Somtexture_inl.supported&=~(-ch); //remove 
	if(ch>=0) Somtexture_inl.supported|=ch;   //add

	Somtexture_inl.supported&=_ADD; //all channels

	if(cmp==Somtexture_inl.supported) return false; //no change
	
	if(Somtexture_inl.supported&PAL&&!palette)
	{
		Sompalette *hack = new Sompalette[256];
		memset(hack,0x00,sizeof(Sompalette)*256);
		for(int i=0;i<256;i++) hack[i].peFlags = RESERVED;
		const_cast<Sompalette*&>(palette) = hack;
	}

	if(~Somtexture_inl.supported&Somtexture_inl.current) 
	{
		Somtexture_inl->change(this,Somtexture_inl.current=0);		
	}

	return true; 
}

bool Somtexture::paste(int ch, const Somtexture*, const RECT *src, const RECT *dest)const
{
	if(readonly()) return false;

	return false; //unimplemented
}

bool Somtexture::copy(int ch, HWND, const RECT *src, const RECT *dest)const
{
	if(readonly()) return false;

	return false; //unimplemented
}

bool Somtexture::load(int ch, void *nib, int x, int y, int z, int w, const PALETTEENTRY *p)const
{
	if(readonly()) return false;

	return false; //unimplemented
}

bool Somtexture::boxselect(int op, RECT *box, bool not)const
{
	if(readonly()) return false;

	return false; //unimplemented
}

bool Somtexture::copyselect(int op, const Somtexture*, RECT *src, RECT *dst, bool not)const
{
	if(readonly()) return false;

	return false; //unimplemented
}

bool Somtexture::mapselect(int op, int a, int z, bool not)const
{
	if(readonly()) return false;

	return false; //unimplemented
}

const wchar_t *Somtexture::well(int n)const
{
	if(readonly()) return 0;
	
	Somtexture_cpp::wellhelper wh(Somtexture_inl->wells);

	if(n<0) //reverse iteration
	{
		wchar_t *queue = wh+=0;

		int sz = wcsnlen(queue,wh.MAX); 
		
		if(sz&&queue[sz-1]==RESERVED) sz--;
		
		n+=sz; if(n<0) return 0;
	}
						 
	wchar_t *out = wh[n]; 

	//enumeration termination condition
	if(out[wh.ENUM]==RESERVED&&n<COLORKEY) return 0;

	return out[wh.ENUM]?out:0;
}

const wchar_t *Somtexture::tones(const wchar_t *well)const
{
	if(readonly()) return 0;
	
	Somtexture_cpp::wellhelper wh(Somtexture_inl->wells);

	wchar_t *out = wh[well];
	
	if(out[wh.SIZE]==0) return 0;

	return out+wh.TONE;
}

size_t Somtexture::palette_index(size_t tone)const
{
	if(readonly()||tone>255) return 0;

	return Somtexture_inl->map[tone].peFlags;
}

const wchar_t *Somtexture::title(const wchar_t *well, const wchar_t *name, size_t namelen)const
{
	if(readonly()) return 0;
	
	Somtexture_cpp::wellhelper wh(Somtexture_inl->wells);

	wchar_t *rename = wh[well]; if(!rename[wh.ENUM]) return 0;

	if(rename[wh.ENUM]&RESERVED) switch(rename[wh.ENUM])
	{													
	case REPLACED: rename = wh[+RESERVED]; case RESERVED: break;

	default: rename = wh[rename[wh.ENUM]&~RESERVED]; break;
	}

	if(name&&rename)
	{
		if(readonly()) return 0;

		size_t n = std::min<size_t>(wcsnlen(name,namelen),wh.NAME-1);

		wmemcpy(rename,name,n); rename[n] = 0;
	}
	
	return rename; 
}

const wchar_t *Somtexture::reserve(int n)const
{
	if(readonly()) return 0;
	
	Somtexture_cpp::wellhelper wh(Somtexture_inl->wells);

	if(n<0) //reverse iteration
	{
		wchar_t *queue = wh-=0;

		int sz = wcsnlen(queue,wh.MAX); 
		
		if(sz&&queue[sz-1]==REPLACED) sz--;

		n+=sz; if(n<0) return 0;
	}							

	if(n<COLORKEY) 
	{
		if(n>=0) n = -n-1; else return 0;
	}
	else n|=RESERVED;
						 
	wchar_t *out = wh[n]; 

	if(out[wh.ENUM]==REPLACED&&n<COLORKEY) return 0;

	return out[wh.SIZE]?out:0;
}

const wchar_t *Somtexture::paint(const wchar_t *well, int a, int z, PALETTEENTRY *p, COLORREF mask, int op)const	
{ 
	if(readonly()) return 0;
	
	//TODO: will need to cache reserve for undo

	//TODO: abstract most of this logic into one member of
	//either Somshader_h::target or Somtexture_PAL::editor

	Somtexture_cpp::wellhelper wh(Somtexture_inl->wells); wh[+FINISHED]; 
	
	wchar_t *w = wh[+RESERVED];

	int r0 = w[wh.SIZE]?w[wh.TONE]:256;

	if(a>255) a = a-256+r0; if(z>255) z = z-256+r0; 

	if(z<a||a<0||a>255||z>255) return 0;
		
	if(!well) //establishing a new well
	{	
		wchar_t *title = wh('new'); if(title) well = title; else return 0; 
		
		if(!*title) swprintf(title,wh.NAME,L"New Well (%d)",title[wh.ENUM]-Somtexture::TITLED);		
	}
	
	if(!palette&&!add(MAP)||!palette) return 0;

	w = wh[well]; if(w!=well) return 0;
	
	if(!well[wh.ENUM]) return 0; //||well[wh.ENUM]&RESERVED

	Sompalette *map = const_cast<Sompalette*>(palette); assert(map);

	bool undo = op<0; if(undo) op = -op; assert(!undo); //unimplemented

	int x = z; //cutoff point for painting
	
	if(w[wh.SIZE]==0) //new well
	{
		int left = a?palette[a-1].peFlags:-1;
		int right = z<255&&z+1<r0?palette[z+1].peFlags:-2;

		//painting has caused a well to be split into two: right half belongs to well
		if(left==right) while(palette[z+1].peFlags==right) if(++z+1==r0||z==255) break;
	}
	else if(a>w[wh.BACK]+1||z<(int)w[wh.TONE]-1) return 0;	

	int f, g, minf = 256, maxf = -1; //TODO: update image somehow

	wchar_t r[wh.MAX], q[wh.MAX], r_s = 0, q_s = 0; 

	for(int i=a,j=i;i<=z;j=i,r_s++) //brute force seems fine
	{
		for(r[r_s]=map[i].peFlags;i<=z&&r[r_s]==map[i].peFlags;i++)
		{
			if(p&&i<=x)
			{
				PALETTEENTRY swap = map[i];			

				f = Somtexture_inl->map[i].peFlags; if(f<minf) minf = f;
				g = Somtexture_inl->map[f].peFlags; if(f>maxf) maxf = f;

				switch(op)
				{
				default: return 0;

	/////////// point of no return ///////////////////////////////////

				case 0:
				case 1: if(mask==0) break;

					assert(mask==0xFFFFFF);

					PALETTEENTRY rgba = map[i] = p[i-a];

					std::swap(rgba.peBlue,rgba.peRed);
										
					Somtexture_inl->map[f] = rgba;					
				}

				Somtexture_inl->map[f].peFlags = g;

				if(op) p[i-a] = swap;
			}			

			map[i].peFlags = well[wh.ENUM];

			if(i==r0) r0++;
		}
				
		if(r[r_s]!=RESERVED) //Assume deleted
		{
			w = wh[r[r_s]]; w[wh.SIZE] = 0; r[r_s]|=RESERVED;			
		}
		else r[r_s] = REPLACED;

		w = wh[r[r_s]]; w[wh.TONE] = j; w[wh.BACK] = i-1;  

		w[wh.SIZE] = i-j; assert(r_s<wh.MAX-1);
	}

	// identical logic to map ///////////////////////////

	wh[+RESERVED][wh.SIZE] = 0;
	for(int i=0,j=i;i<256;j=i,q_s++) //brute force seems fine
	{
		for(q[q_s]=palette[i].peFlags;i<256&&q[q_s]==palette[i].peFlags;i++);	

		w = wh[q[q_s]]; w[wh.TONE] = j; w[wh.BACK] = i-1;  

		w[wh.SIZE] = i-j; assert(q_s<wh.MAX-2);
	}

	while(q_s<wh.MAX) q[q_s++] = 0; wh+=q; 
	while(r_s<wh.MAX) r[r_s++] = 0; wh-=r;	

	/////////////////////////////////////////////////////

	return well;
}

bool Somtexture::map(int a, int z, int aa, int op)const
{
	if(readonly()||!palette||!op) return false;

	//TODO: will need to cache reserve for undo

	//TODO: abstract most of this logic into one member of
	//either Somshader_h::target or Somtexture_PAL::editor

	Somtexture_cpp::wellhelper wh(Somtexture_inl->wells); wh[+FINISHED];

	wchar_t *w = wh[+RESERVED]; int r0 = w[wh.SIZE]?w[wh.TONE]:256;

	if(a>255) a = a-256+r0; if(z>255) z = z-256+r0; if(aa>255) aa = aa-256+r0; 

	if(aa>=r0&&a<r0){ aa = r0; z = std::min(z,aa-1); } //hack: effective but limiting

	//DANGER! aa can be 256, which is outside the bounds of a 256 palette
	//Probably the implementation logic should be reworked to work around this dilemma

	if(z<a||a<0||aa<0||a>255||z>255||aa>255+1) return false;
		
	Sompalette *map = const_cast<Sompalette*>(palette); assert(map);

	bool undo = op<0; if(undo) op = -op; if(op>2) return 0; assert(!undo);
	
	if(undo) if(aa>z) //untested
	{
		int swap = a; a = aa-(z-a+1); z = aa-1; aa = swap; assert(0);
	}
	else
	{
		int swap = z; z = aa+(z-a+1); a = aa; aa = swap+1; assert(0);
	}		

	bool cutting = aa>=r0+(op==BEHIND?1:0);

	int behind = map[aa?aa-1:0].peFlags; 
	int before = aa<256?map[aa].peFlags:RESERVED;

	int map_aa__peFlags = aa<256?map[aa].peFlags:RESERVED; //quick bugfix
	
	if(!a||map[a].peFlags!=map[a-1].peFlags) if(a<r0) behind = map[a].peFlags;
	
	if(aa>=a&&aa<=z) //should not affect change in most circumstances
	{			
		if(a==aa&&(op==BEFORE||map[a].peFlags==behind)) return false;

		//TODO: consider lifting requirement if map changes membership of a

		if(op!=BEHIND||!aa||map[aa].peFlags==map[aa-1].peFlags) return false;
	}
	else if(aa>a&&z==aa-1) //observed: same deal as above (slight variant)
	{
		if(op==BEHIND||map[z].peFlags==map_aa__peFlags) return false;
	}
	if(aa==0&&op==BEHIND) //fail if a~z is the subsection of a well
	{
		if(a<r0&&map[a].peFlags==map[a?a-1:0].peFlags) 
		{
			if(map[a].peFlags!=map[aa].peFlags) return false;
		}
	}

	int A = aa<a?aa:a, Q = aa>z?aa-1:z, X = Q; //Alpha y Qmega

	int left = map[Q!=z?Q:a-1].peFlags;
	int right = Q<255&&Q+1<r0?map[Q+1].peFlags:-2;

	if(left==right&&left!=RESERVED&&right!=RESERVED) 
	{
		while(map[Q+1].peFlags==right) if(++Q+1==r0||Q==255) break;
	}

	wchar_t r[wh.MAX], q[wh.MAX], r_s = 0, q_s = 0; 
	
	// identical logic to paint ///////////////////////////

	for(int i=A,j=i;i<=Q;j=i,r_s++) //brute force seems fine
	{					
		for(r[r_s]=map[i].peFlags;i<=Q&&r[r_s]==map[i].peFlags;i++);
		
		if(r[r_s]!=RESERVED) //Assume deleted
		{
			w = wh[r[r_s]]; w[wh.SIZE] = 0; r[r_s]|=RESERVED;			
		}
		else r[r_s] = REPLACED;

		w = wh[r[r_s]]; w[wh.TONE] = j; w[wh.BACK] = i-1;  

		w[wh.SIZE] = i-j; assert(r_s<wh.MAX-1);
	}

	///////////////////////////////////////////////////////

	int last = A?map[A-1].peFlags:0; 

	for(int i=A;i<=X;i++) //dissolve memberships
	{
		int swap = map[i].peFlags; 
		
		if(swap!=last) 
		{
			if(cutting&&i>=a&&i<=z) 
			{
				//move front of the well to the end of the cutting area
				while(map[i].peFlags==swap&&i<=z) map[i++].peFlags = 0; 
				continue;
			}
		}
		else map[i].peFlags = 0; 
		
		last = swap;
	}
		
	if(map_aa__peFlags||a>=r0) switch(op)
	{
	case BEFORE: map[a].peFlags = before; break;
	case BEHIND: map[a].peFlags = behind; break;

	default: assert(0); //paranoia
	}
		
	// leap frogging the palette entry memory //////////////

	int m = X-A+1, n = z-a+1, hole = m-n;
				
	PALETTEENTRY swap[256]; 
	memcpy(swap,map+A,sizeof(PALETTEENTRY)*m);

	unsigned char edit[256];
	for(int i=0;i<m;i++) edit[i] = Somtexture_inl->map[A+i].peFlags;

	if(aa>z)
	{			
		memcpy(map+A+hole,swap,sizeof(PALETTEENTRY)*n);		
		memcpy(map+A,swap+n,sizeof(PALETTEENTRY)*hole);

		for(int i=0;i<n;i++) Somtexture_inl->map[A+hole+i].peFlags = edit[i];
		for(int i=0;i<hole;i++) Somtexture_inl->map[A+i].peFlags = edit[n+i];
	}
	else
	{
		memcpy(map+A,swap+hole,sizeof(PALETTEENTRY)*n);
		memcpy(map+A+n,swap,sizeof(PALETTEENTRY)*hole); 

		for(int i=0;i<n;i++) Somtexture_inl->map[A+i].peFlags = edit[hole+i];
		for(int i=0;i<hole;i++) Somtexture_inl->map[A+n+i].peFlags = edit[i];
	}

	////////////////////////////////////////////////////////

	last = map[A?A-1:0].peFlags;

	for(int i=A;i<=X;i++) //reconstitute memberships
	{
		if(map[i].peFlags) last = map[i].peFlags; map[i].peFlags = last;
	}

	for(int i=X+1;i<=Q;i++) map[i].peFlags = map[X].peFlags;

	// identical logic to paint ///////////////////////////

	wh[+RESERVED][wh.SIZE] = 0;
	for(int i=0,j=i;i<256;j=i,q_s++) //brute force seems fine
	{
		for(q[q_s]=palette[i].peFlags;i<256&&q[q_s]==palette[i].peFlags;i++);	

		w = wh[q[q_s]]; w[wh.TONE] = j; w[wh.BACK] = i-1;  

		w[wh.SIZE] = i-j; assert(q_s<wh.MAX-2);
	}  

	while(q_s<wh.MAX) q[q_s++] = 0; wh+=q; 
	while(r_s<wh.MAX) r[r_s++] = 0; wh-=r;	

	////////////////////////////////////////////////////////

	return true;
}

size_t Somtexture::index(size_t toner, PALETTEENTRY *paint, int index)const
{
	if(readonly()) return 0;

	assert(0); return true; //unimplemented
}

bool Somtexture::sort(int ch, int op, int a, int z)const
{
	if(readonly()) return 0;

	if(ch!=INDEX||op!=0) return false;

	if(ch&Somtexture_inl.locks) return false; 

	size_t size = width*height;
			
	if(size>Somtexture_inl->size) //paranoia
	{
		assert(0); return false;
	}

	BYTE unmap[256]; int unsorted = 0;
	
	for(int i=0;i<256;i++)
	{
		if(i!=Somtexture_inl->map[i].peFlags) unsorted++;
	
		unmap[Somtexture_inl->map[i].peFlags] = i; 

		Somtexture_inl->map[i].peFlags = i; 
	}
		
	if(unsorted==0) return false;

	if(!Somtexture_inl->index) return true;
	 	
	for(size_t i=0;i<size;i++)
	{
		Somtexture_inl->index[i] = unmap[Somtexture_inl->index[i]];
	}

	//hack: a bit heavy handed
	if(Somtexture_inl.current==INDEX)
	{
		Somtexture_inl->change(this,Somtexture_inl.current); 
	}
	else Somtexture_inl->cleanup(this,INDEX); 

	return true;
}


wchar_t Somtexture::notes(wchar_t *read, wchar_t size, wchar_t skip, const wchar_t *write)const
{	
	if(readonly()) return 0;
		
	if(~Somtexture_inl.supported&NOTES) return 0;

	//TODO: migrate logic into notehelper
	Somtexture_cpp::notehelper nh(Somtexture_inl->notes);

	if(write&&size) //writing notes
	{
		size_t sz = nh?nh[nh.SZ]:0; 
		
		if(skip>sz||skip+size+1>nh.MAX) return 0;

		const size_t cap_x = 1024;
		
		const size_t szcap = sz?(sz/cap_x+1)*cap_x:0;

		const size_t newsz = std::max<size_t>(skip+size+1,sz);

		const size_t newcap = (newsz/cap_x+1)*cap_x;
								 
		if(newcap>szcap) //this is a mess
		{
			wchar_t *swap = new wchar_t[newcap+nh.HD]+nh.HD;

			Somtexture_cpp::notehelper cp(swap);

			if(nh) //this is a bit of a mess because notehelper needs more thought
			{
				wmemcpy(cp-cp.HD,nh-nh.HD,nh[nh.SZ]+nh.HD); ~Somtexture_cpp::notehelper(nh); 
			}
			else cp[cp.WR] = 0; nh = cp;

			Somtexture_inl->notes = swap;
		}

		wcsncpy(nh+skip,write,size); nh[newsz-1] = 0;

		nh[nh.SZ] = skip+wcsnlen(write,size)+1;
		nh[nh.WR]++;
	}

	if(read) //reading notes 
	{	
		if(size<1) return 0;

		if(nh)
		{
			if(skip>=nh[nh.WR]) return 0;

			wcsncpy(read,nh+skip,size-1); 		

			read[size-1] = 0;
		}
		else *read = 0;
	}

	return nh?nh[nh.WR]:1;		
}			

const Somtexture *Somtexture::addref()const
{
	if(!this) return 0;
	
	Somtexture_inl.refs++;
	
	return this;
}

const Somtexture *Somtexture::release()const
{
	if(!this) return 0;
	
	if(Somtexture_inl.locks)
	{
		assert(0); //TODO: unlock but leave assert
	}

	if(--Somtexture_inl.refs>0) return this;

	Somthread_h::section cs;

	int pool = Somtexture_inl.pool; //tearoff
		
	Somtexture_cpp::poolhelper ph(pool); delete this;

	Somtexture_cpp::pools[ph]--; return 0;	
}

int Somtexture::refcount()const
{
	return this?Somtexture_inl.refs:0;
}

int Somtexture::pool()const
{
	return this?Somtexture_inl.pool:0;
}

SOMPAINT Somtexture::server()const
{
	return this?Sompaint:0;
}

bool Somtexture::apply_state(const Somenviron *env, int ops)const
{
	if(!this) return false;
	
	const char *z = "z = reset"; 

	if(ops&Z) z = ops&BLEND?"z = decal":"z(0,1)"; 

	const wchar_t *vs = 0, *ps = 0;

	const Sominstant *pov = env->point_of_view(); 
	{
		Somthread_h::section cs;

		Somtexture_cpp::pool &pool = 
		Somtexture_cpp::pools[Somtexture_inl.pool];
		
		if(pool.backbuffer==this) 
		{
			if(Somtexture_app&&env
			 &&env==Somtexture_app->environment)
			{
				//switching operations
				Somtexture_app->blendmode = -1;
				Somtexture_app->operations = ops;
				Sompaint->format(*Somtexture_app,"blend(1,0), %s;",z);				
				return true;
			}
			else Sompaint->reset();

			Somtexture_app::unset(this);

			release(); pool.backbuffer = 0;
		}
		else if(!env||env&&pool.backbuffer) 
		{
			assert(0); return false; //collision
		}
		else Sompaint->reset(); //paranoia

		if(!env) return true; //closure	 
		if(!pov) return false; //required

		assert(RGB==Somtexture_inl.current);
		assert(RGB&Somtexture_inl.supported);

		if(RGB&~Somtexture_inl.supported) return false;
		if(!Sompaint->pal(this)->setup()) return false;

		size_t vp[4] = {0,0,width,height};
		if(!Sompaint->frame(vp)) return false;

		Sompaint->pal(pool.application)->apply(0);

		Somtexture_app = &pool.application;	

		Somtexture_app->environment = env->addref();
		Somtexture_app->operations = ops;		

		vs = pool.dummy_vs; ps = pool.dummy_ps;

		pool.backbuffer = addref(); 				
	}		 		
	
	struct //caching just to be safe
	Somtexture_app &app = *Somtexture_app; 

	if(!&app) return false;
	
	//hack: scheduled obsolete
	#define vs_c app.vs_c
	#define vs_c_s Somshader::vs_c_s
	#define vs_i app.vs_i
	#define vs_i_s Somshader::vs_i_s
	#define ps_c app.ps_c
	#define ps_c_s Somshader::ps_c_s	

	const Sominstant *fog = 0; //clear colour

	if(env->list(0,env->FOG,&fog,1))
	{
		//assuming fog surrounds our point of view
		env->describe(fog,env->SIGNAL,ps_c.fogColor,4);
	}
	else Somvector::series(ps_c.fogColor,0,0.5,0.5,1); //cyan

	float *f = ps_c.fogColor; 
	Sompaint->format(&pal,"clear(%f,%f,%f,1);",f[0],f[1],f[2]);

	typedef Somvector::matrix<float,4,4> float4x4;

	//the W is being calculated per instance inside the shaders
	float4x4 &v = v.map(vs_c._x4mWV), &vp = vp.map(vs_c._x4mWVP);

	if(!env->describe(0,env->FRUSTUM,*vs_c._x4mWV,16)) assert(0);

	v.invert<4,4>().premultiply<4,4>(vp.copy<4,4>(*pov)); 	
	
	if(!Sompaint->run(vs)) assert(0); //move to foreground	

	int ld = Sompaint->load("%*f %*i",vs_c_s,&vs_c,vs_i_s,&vs_i);
			
	if(!Sompaint->run(ps)) assert(0); assert(ld==vs_c_s+vs_i_s);

	ld = Sompaint->load("%*f",ps_c_s,&ps_c); assert(ld==ps_c_s);
		 
	app.blendmode = -1;
	Sompaint->format(app,"blend(1,0), %s;",z);

	return true;
}

bool Somtexture::apply_filter(Somtexture_app::f f, void *userpass)const
{
	if(!this||!Somtexture_app) return false;

	Somtexture_app->filter = f; Somtexture_app->passthru = userpass;

	return true;
}

bool Somtexture::apply(const Somgraphic *gin, int in)const
{
	if(!this) return false;
	
	struct //caching just to be safe
	Somtexture_app &app = *Somtexture_app; 

	if(!&app) return false;

	///// todo: check that texture is setup ////////////

	if(!gin) //// clear mode /////
	{
		assert(in==-1); //all channels

		return Sompaint->pal(this)->clear(); //todo: mask
	}				 

	const int &ops = app.operations;

	const bool solid = ops&SOLID, blend = ops&BLEND;		 	

	const size_t h_s = 32; 
	const size_t h_up_s = 16+1; //world matrix + opacity (for now)

	const Somgraphic *g = 0; 
	const Sominstant *h[h_s]; int h_i[h_s]; float h_up[h_s*h_up_s];

	for(int i=0;g=gin->section(i);i++) 
	{	
		bool frozen = g->freeze();
		
		double *fade = g->opacity('ro');

		for(int j=0,k;k=g->channel(&j,h,h_i,h_s,0);j++) 
		{				
			int m = h_i[0]%g->width; 

			float factors[8]; //material

			int mode = g->blendmode(m,factors);

			bool blended = mode||factors[3]!=1;

			if(!solid) if(blend) //2nd pass								
			{
				if(!blended&&!fade) continue;
			}
			else if(blended) continue;	
		
			size_t r[2], c[2], skip = 0; 

			const Somtexture *image = 0;
			const Somtexture *model = g->model(&j,0,0,0);
			const Somtexture *index = g->index(&j,r,c,2);			 

			if(model&&index&&g->texture(&j,&image,0,1,0)) do //!
			{
				int up = 0; //upload

				for(int l=0;l<k;l++)
				{	
					int n = h_i[l]/g->width;

					if(in>=0&&n!=in) continue;

					int compile[h_up_s==16+1]; 					
										
					float opacity = fade?fade[n]:1;

					if(!solid) if(blend) //2nd pass
					{
						if(opacity==1&&!blended) continue;
					}
					else if(opacity!=1) continue;								

					h[up] = h[l]; 

					h[up]->set<4,4,4,4>(h_up+up*h_up_s); //16 floats

					h_up[up*h_up_s+16] = opacity; //1 float

					up++; //...
				}

				if(up==0) continue;				

				int f = KEEP;

				if(app.filter) 
				{
					f = app.filter(g,m,0,app.passthru); 
					
					if(!f) continue; //discarded
				}

				int rgb = g->shaders(m);
				
				const Somshader
				*vs = g->reference_shader(rgb,g->RGB_VS), 
				*ps = g->reference_shader(rgb,g->RGB_PS); 

				if(!Sompaint->run(vs->assembly())) assert(0); 

				Sompaint->load("%8.*f",vs_c[vs_c.matDiffuse],factors);

				if(!Sompaint->run(ps->assembly())) assert(0);

				if(blend&&app.blendmode!=mode)
				{
					//Reminder: if mode==0 then per instance opacity is in play
					Sompaint->format(app,"blend = %s(%d);",mode&1?"mdl":"mdo",mode>>1); 

					app.blendmode = mode;
				}
				
				if(f&POST&&!app.filter(g,m,f,app.passthru)) continue; 
								
				Sompaint->pal(image)->sample(0);
				Sompaint->pal(model)->layout(g->vertex_string(m));
				Sompaint->pal(model)->stream(up,h_up,up*h_up_s*sizeof(float));
				Sompaint->pal(index)->draw(r[0],r[1],c[0],c[1]);

			}while(k=g->channel(&j,h,h_i,h_s,skip+=k));
		}

		if(frozen) g->thaw(); 
	}

	return true; //try harder 
}

HBITMAP Somtexture::thumb(int thumbwidth, int thumbheight, bool enlarge)const
{
	if(!this||width<1||height<1) return 0; 
		
	if(thumbwidth<1) thumbwidth = width; 	
	if(thumbheight<1) thumbheight = height;

	int w = width, h = height;

	int tw = enlarge?thumbwidth:std::min<UINT>(thumbwidth,w);
	int th = enlarge?thumbheight:std::min<UINT>(thumbheight,h);

	RECT fit = aspect(tw,th);

	Somtexture_cpp::pool::thread *tls = 0;
	{
		Somthread_h::section cs;		

		tls = Somtexture_cpp::pools[Somtexture_inl.pool].tls();
	}

	HBITMAP out = 0;

	SOMPAINT_PIX pix = Sompaint->raster(&tls->pix,&out);

	Sompaint->pal(this)->print(fit.right,fit.bottom,pix);

	return out;
}

RECT Somtexture::aspect(int fitx, int fity, int x, int y)const
{		
	int w = this?width:0;
	int h = this?height:0;

	if(w>h)
	{
		float r = float(h)/w; w = fitx; h = r*w+0.5f;
	}
	else if(h>w)
	{
		float r = float(w)/h; h = fity; w = r*h+0.5f;
	}
	else w = h = fitx<fity?fitx:fity;

	if(!w) w = fitx?1:0; if(!h) h = fity?1:0;
		
	RECT out = { x, y, w+x, h+y };

	assert(w<=fitx&&h<=fity);

	return out;
}

//SCHEDULED FOR REMOVAL
bool Somtexture::where_BLACK(int ch, COLORREF f(COLORREF))const
{
	if(readonly()||ch&~Somtexture_inl.supported) return false;

	if(Somtexture_inl.locks&ch) return false; //Sufficient?

	//2017: I don't if this is how I intended this to work :(
	PALETTEENTRY *buf; int i = 0, iN = 256; switch(ch)
	{
	case PAL: buf = Somtexture_inl->map; i = colorkey()?1:0; break;
	case RGB: buf = Somtexture_inl->rgba; iN = width*height; break;
	default: return false;
	}											

	int out = 0; for(;i<iN;i++)
	{
		COLORREF &e = (COLORREF&)buf[i];
		if(0==(~0xFF070707&e))
		{
			e = e&0xFF000000|f(e&0xFFFFFF); out++; 
		}
	}

	//HACK?	Assuming end-user is editing the texture.
	//TODO? Count changes and report/dirty on change.
	SetRect(Somtexture_inl->dirty(ch),0,0,width,height); 

	//SCHEDULED FOR REMOVAL: THIS SHOULD'T BE NECESSARY.
	//(And why is cleanup insufficient?)
	if(Somtexture_inl.current==ch)
	{
		Somtexture_inl->change(this,ch); 
	}
	else Somtexture_inl->cleanup(this,ch); return out!=0;
}

/*~~~~~~~~~~references: keeping around a copy of these for now ~~~~~~~~~~~~~~~~~~~*/

//class Somtexture //Somtexture.h
//{
	/*UNIMPLEMENTED (thought might be necessary at one time)
	//
	//Associate some meta data which will persist until released
	//
	//N is the 1 based version of the key desired... unless key is 0; in
	//which case N enumerates the available keys... unless value is nonzero
	//in which case if N is nonzero N is the size of value, otherwise value is
	//0 terminated, and a new version of key set to value is created and returned
	//
	//FYI: when enumerating N is 0 (or -1) based (and in no particular order)
	//Note: for the newest version N can be 0 or -1 (-2, -3, etc for reverse)
	const wchar_t *meta(int N, const wchar_t *key=0, const wchar_t *value=0);
	//
	//Disclaimer: assuming value was returned by meta with outstanding references
	inline wchar_t meta_version(const wchar_t *value){ return value?value[-1]:0; }
	//
	//This is provided to more safely deal with binary values (if you must do so) 
	//Notice: this size is in wchar_t units and does not include the 0 terminator
	inline wchar_t meta_characters(const wchar_t *value){ return value?value[-2]:0; }
	//
	//Figure the total number of keys (one per key; regardless of any versioning)
	inline wchar_t meta_keys(){ const wchar_t *n = meta(-1); return n?n[-1]+1:0; }
	*/

	//// shared editing/feedback APIs ///////////////////////////
	/*
	//Get a loop of windows that are being 
	//notified about changes to the texture
	//
	//add: add a window to the loop...
	//post: after a change 0x8000+post is posted to
	//the windows in the loop. LOWORD(WPARAM) is the 
	//affected channel/well; 0 when a well is affected...
	//
	//For channels LPARAM is a pointer to a RECT defining
	//the modified region. Width/height should be checked
	//
	//For wells HIWORD(WPARAM) is the well number
	//LOBYTE(LOWORD(LPARAM)) is the new start of the well
	//HIBYTE(LOWORD(LPARAM)) is the new count for the well
	//LOBYTE(HIWORD(LPARAM)) is the first affected tone
	//HIBYTE(HIWORD(LPARAM)) is the affected tone count
	//
	//pre: if greater than -1 a symmetric message is
	//sent out before any changes will be carried out
	//WARNING: pre messages will block post messages
	//
	//Returns add on success unless add is 0
	//The first window is returned if add is 0
	//While post is -1 the loop can be iterated 
	//over by passing the return back through add 
	HWND loop(HWND add, int post, int pre=-1)const;

	//see iteration explanation above
	inline HWND loop(HWND add=0)const 
	{
		return loop(add,-1,-1); 
	}
	
	void lock(HWND, int notify=0)const;

	inline void unlock()const
	{
		return lock(0);
	}
	*/
//}
