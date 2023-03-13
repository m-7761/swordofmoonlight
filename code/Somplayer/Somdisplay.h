					 
#ifndef SOMDISPLAY_INCLUDED
#define SOMDISPLAY_INCLUDED

#include "Somthread.h"
#include "Somconsole.h"
#include "Somtexture.h"

class Somdisplay
{
public: int width, height;

	//This class represents a display
	//like an LCD or a virtual window
	//It's used to configure a user's
	//preferences and to fulfill some
	//trickier rendering requirements

	//// preferences ////////////////

	bool smooth; //shifts a half pixel over 
	bool dither; //todo: custom dither mask

	float brighter, dimmer; //small positive values

	int anisotropy; //eg. 4x, 8x, 16x, 0 to disable

	//// presentation ///////////////

	template<typename T>
	inline bool ready(T t)const{ return content==t&&picture; }

	inline bool ready()const{ return ready(&Somconsole::render); }

	inline bool present(HWND hwnd, const RECT *src, const RECT *dest)const
	{
		if(!picture) return false;

		if(!src) //TODO: zoom ought to be set to 0
		return picture->present(0,hwnd,src,1,dest); 
			
		RECT safe = {0,0,picture->width,picture->height}; //WM_SIZE		

		if(IntersectRect(&safe,src,&safe))
		return picture->present(0,hwnd,&safe,1,dest); 
		return false;			
	}

	//can call directly to keep the user's preferences
	~Somdisplay(){ picture->release(); picture = 0; };

	Somdisplay(){ memset(this,0x00,sizeof(Somdisplay)); }

private: friend Somconsole; //See Somconsole::display  

	mutable const Somtexture *picture; //the render target
		
	//This is one of 0, Somconsole::render, or Somconsole::select
	mutable bool(Somconsole::*content)(const Somenviron*,const Somdisplay*,int);

	mutable int renderops, selectops; //the previous render ops

	mutable int screen; //the current frame
};

#endif //SOMDISPLAY_INCLUDED