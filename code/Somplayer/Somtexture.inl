
//private implementation
#ifndef SOMTEXTURE_INCLUDED
#define SOMTEXTURE_INTERNAL

class Somtexture;
class Somenviron;

struct Somtexture_inl
{ 
	struct editor
	{	
	public: size_t size, black; 

		PALETTEENTRY *rgba, map[256]; 

		BYTE *index, *alpha; wchar_t *wells, *notes; 	

		RECT *dirty(int ch); bool clean(int ch, RECT *inout=0);

		void **cleanup(const Somtexture*, int ch, const RECT *area=0, int zoom=1);		

		editor(){ memset(this,0x00,sizeof(editor));	}		
		~editor();
		                               //rgba                    //index                   //alpha
		void fillout(const Somtexture*,const BYTE*,size_t,size_t,const BYTE*,size_t,size_t,const BYTE*);
		
		int knockout(PALETTEENTRY palette[256]=0); //import colorkey w/ palette 

		void change(const Somtexture*, int ch); //primary
			
		void discard(const Somtexture*); //discard pals

	  //Storing 2 additional RGBA textures: RGB+ALPHA PAL+INDEX//
	  //A greyscale shader is used to visualize ALPHA and INDEX// 

	private: static const size_t pals_s = 2; void *pals[pals_s];	   

		RECT _dirty[4]; //implementation dependent

	}*edit;

	inline editor *operator->(){ return edit; }
	
	Somthread_h::account refs; const int pool; 
	
	int current, supported, locks, *pixels, pixels_s; 	

	int width, height; //internal width height

	Somtexture_inl(int p=0) : refs(1), pool(p), edit(0)
	{
		current = supported = locks = pixels_s = 0; pixels = 0;

		width = height = 0;
	}		
	~Somtexture_inl() //See ~Somtexture()
	{	
		delete edit; edit = 0; supported = 0; assert(locks==0);
		
		delete[] pixels; pixels = 0; pixels_s = 0;		
	} 
	
	int &pixel(int ch) 
	{	
		/*optimization of single channel textures*/
		if(!pixels&&ch==supported) return pixels_s;  

		if(pixels) for(int i=0;i<pixels_s;i++) if(pixels[i]==ch) return pixels[pixels_s+i];
		if(pixels) for(int i=0;i<pixels_s;i++) if(pixels[i]==0) return pixel(pixels[i]=ch);

		int x = pixels?pixels_s*2:8, *swap = pixels; 
		
		memset(pixels=new int[x*2],0x00,sizeof(int)*x*2);

		if(swap) memcpy(pixels,swap,pixels_s*sizeof(int)); 
		if(swap) memcpy(pixels+x,swap+pixels_s,pixels_s*sizeof(int)); 

		pixels_s = x; delete[] swap; return pixel(ch);
	} 
};

class Somtexture_app;

#define SOMPAINT_DIRECT

#include "../Sompaint/Sompaint.h"

#define private public 
#include "Somtexture.h"
#undef private

#include "Somshader.h"

struct Somtexture_app
{
	int operations; 

	const Somenviron *environment;

	void *pal; //Sompaint state buffer

	inline operator void**(){ return this?&pal:0; }

	typedef int(*f)(const Somgraphic*,int,int,void*);

	f filter; void *passthru;

	Somshader::vs_c vs_c; Somshader::vs_i vs_i;
	Somshader::ps_c ps_c; 	

	int blendmode; //current
	
	Somtexture_app()
	{
		memset(this,0x00,sizeof(Somtexture_app));
	}

	template<class Somtexture>
	static inline void unset(const Somtexture *p) 
	{
		assert(p&&p->Somtexture_app);

		Somtexture_app *app = 0; std::swap(app,p->Somtexture_app);						

		app->environment->release(); app->environment = 0;

		app->passthru = app->filter = 0;		
	}
};

#else
#ifdef SOMTEXTURE_INTERNAL

	mutable SOMPAINT Sompaint; //"pimpl"//
	
	mutable Somtexture_inl Somtexture_inl;

	mutable Somtexture_app *Somtexture_app;

#endif //SOMTEXTURE_INTERNAL
#endif //SOMTEXTURE_INCLUDED