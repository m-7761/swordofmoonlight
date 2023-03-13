					 
//private implementation
#ifndef SOMGRAPHIC_INCLUDED
#define SOMGRAPHIC_INTERNAL

#define SOMGRAPHIC_FEATURES	\
\
	enum\
	{\
	EDITING=1,\
	SCALING=2, /*MDO*/\
	OPACITY=4,\
	FACTORS=8,\
	ANIMATE=16, /*MDL*/\
	SECTION=32,\
	SHADOWS=64,\
	};

#include "../Sompaint/Sompaint.h"

#include "Somshader.h" 
#include "Somtexture.h" 
#include "Sominstant.h"

struct Somgraphic_inl
{	
	SOMGRAPHIC_FEATURES
				
	struct reference
	{
		int type;
				
		union //reserved
		{
		const Somtexture *image;
		};

		wchar_t *resource;		

		reference(int t=0)
		{
			type = t; image = 0;
			
			resource = L"";
		}
	};	

	//union
	struct loopback //hack
	{
		////Somgraphic////
		int width, height; 

		//Somgraphic_inl//
		loopback *section; 

		loopback() : section(this)
		{
			width = height = 0; 
		}

		inline operator Somgraphic*()
		{
			return (Somgraphic*)this;
		}
	};

	//abstract
	struct construct : public loopback
	{			
		int featureset; 

		Somgraphic *graphic;

		std::vector<void*> memory;
		
		std::vector<reference> texture;	

		std::vector<Sominstant**> picture;					
		
		size_t picture_width; //refresh
							   
		Somthread_h::intralock freezer;
		Somthread_h::interlock rwlock;

		//assuming monolithic for now
		int vertex, profile, shaders; 

		const Somshader *vs, *ps;

		void fit(size_t n);

		wchar_t *resource(int i, int type);

		const Somtexture *image(int i, int type);

	protected: //abstract

		//texture[0] should always hold an empty value 
		construct(int f=0) : featureset(f), texture(1)
		{	
			graphic = 0; picture_width = vertex = profile = shaders = 0; 
			vs = ps = 0;
		}				  
		~construct()
		{
			size_t i = texture.size(); 
			while(i>0) texture[--i].image->release();

			i = memory.size(); while(i>0) delete [] memory[--i];

			vs->release(); ps->release();
		}
	};

	union primitive 
	{	
		long long compare;
				
		struct
		{
		//compare//////////
		unsigned texture:4; 
		unsigned blendop:4;
		unsigned factors:8;
		unsigned        :8;
		unsigned model  :4;
		unsigned index  :4; 		
		unsigned start :16;		
		unsigned count :16;		
		//vcaching/////////
		unsigned vstart:16;		
		unsigned vcount:16;
		unsigned	   :16;
		///////////////////
		unsigned channel:8;
		unsigned		:8;
		};

		primitive()
		{
			(&compare)[0] = (&compare)[1] = 0;

			int compile[sizeof(compare)*2==sizeof(primitive)]; 			
		}
		
		inline bool operator==(const primitive &cmp)
		{
			return compare==cmp.compare;
		}
		inline bool operator!=(const primitive &cmp)
		{
			return compare!=cmp.compare;
		}
	};

	//MSM
	struct permanent : public construct 
	{	
		primitive *primitives; //per width

		permanent(int f=0) : construct(f)
		{
			primitives = 0;	  
		}
		~permanent(){ /*construct::memory*/ }
			
		inline size_t tune(int ch)const
		{
			return ch<0||ch>=width-1?0:primitives[ch].channel;
		}
	};	

	//MDO
	struct transient : public permanent
	{	 
		double *scaling, *opacity; float *factors; //per height

		transient(int f=0) : permanent(f|SCALING|OPACITY|FACTORS)
		{
			scaling = 0; opacity = 0; factors = 0;
		}
		~transient(){ /*construct::memory*/ }
	};	  	   

	//MDL
	struct animation : public transient 
	{
		int *groups; //per width
		
		double *tween, *shadows; //per height

		int *playlist; double *timetable; //per animation 

		//sectioning requires the highest level graphic//
		//structure because sections cannot be switched//
		//if and when a graphic is refreshed. A section//
		//makes little sense if a model is not animated//
		//But it is a technical detail worth mentioning//

		animation *sections;
		Somgraphic *section(int i)
		{
			if(i<0) return 0; //paranoia

			if(featureset&ANIMATE&&~featureset&SECTION)
			{
				if(i==0) return *this;

				animation *out = sections;
				for(i--;i>0&&out;i--) out = out->sections;
				return *out;
			}
			else return i?0:*this; 
		}

		animation(int f=0) : transient(f|ANIMATE|SHADOWS)
		{
			playlist = groups = 0; timetable = tween = 0; sections = 0; shadows = 0;
		}	
		~animation() 
		{
			if(featureset&ANIMATE) delete sections;	//construct::memory
		}
	};	

	construct *section; //loopback
			 
	//careful: returning sub class
	inline animation *operator->()
	{
		return (animation*)section;
	}

	inline operator int()
	{
		return section->featureset;
	}

	Somthread_h::account refs; const int pool; 

	Somgraphic_inl(int p=0) : refs(1), pool(p){	section = 0; }

	~Somgraphic_inl() //See ~Somgraphic()
	{
		delete (animation*)section; section = 0; //hack			
	}
};

#define private public 
#include "Somgraphic.h"
#undef private

#else
#ifdef SOMGRAPHIC_INTERNAL //"pimpl"//
		
	SOMGRAPHIC_FEATURES //lazy

	mutable Somgraphic_inl Somgraphic_inl;

#endif //SOMGRAPHIC_INTERNAL
#endif //SOMGRAPHIC_INCLUDED