
#ifndef EXSELECTOR_INCLUDED
#error #include Exselector.h?
#endif

typedef struct tagRECT RECT; 
typedef struct HFONT__ *HFONT;

/**SINGLETON
 * There is a single global pointer to an object of this
 * type returned by Sword_of_Moonlight_Extension_Library.
 */
class Exselector
{
public:
	
	//REMINDER: these methods can't be reordered since
	//this is mainly a way to avoid individual exports

	//call when destroying/changing the OpenGL context
	//if canvas is nonzero a new canvas is created for
	//the current OpenGL context and get_ANGLE_enabled
	virtual void svg_reset(bool canvas=false);	

	//this just returns true if a canvas object exists
	virtual bool svg_canvas();

	//2022: this just calls glViewport without callers
	//having to have a dependency on OpenGL. it's here
	//to work make it easy to implement svg_end's user
	//callback, which should also ccall svg_view below
	virtual void svg_viewport(int,int,int,int);

	//2022: this must be called before svg_end or from
	//inside svg_end via its callback parameter. if 0
	//is passed it uses glGetIntegerv(GL_VIEWPORT,&vp)
	//to create a 1-to-1 default matrix like this:
	//
	//		2/w,0,0,0,
	//		0,-2/h,0,0,
	//		0,0,0,0,
	//		-1,1,0,1,
	//
	//upside-down isn't supported but is equivalent to
	//negating the 2nd values in the 2nd and 3rd rows 
	virtual void svg_view(float wvp[4*4]=0);
	
	//allocate a font resource for svg_draw
	virtual int svg_font(HFONT);

	//2022: to allow for OpenXR svg_view has to set up
	//a transformation matrix
	virtual void svg_begin();
//	{
		//this overload draws text if it's not obvious
		virtual int svg_draw(int font, const wchar_t *str, int len, RECT *rect, int format, unsigned color);
//	}
	//2022: the xr_f callback return type is bool but
	//must be int because it's implemented by C which
	//doesn't support bool type. return false to stop
	//enemurating OpenXR views. each call should call
	//svg_viewport and svg_view for the view's number.
	virtual void svg_end(BOOL(*xr_f)(int view)=0);

public: //END CURRENT VERSION

		  //// IMPLEMENTATION ////

		#ifdef EXSELECTOR_PCH_INCLUDED 

		bool _lazy_init_nvgc();

		Exselector():_nvgc(){}

		NVGcontext *_nvgc;

		#endif
};