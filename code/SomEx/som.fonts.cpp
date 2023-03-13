		
#include "Ex.h" 
EX_TRANSLATION_UNIT
			
#include "som.state.h"

//These simulate SOM's behavior including flaws!
extern int SOM::fontsize(int x, int y, int *out)
{		
	//observed values (bracketed nums = repeating fractions)
	//0x20:   640x480  (32x24)
	//0x22:   720x576  (32.[7272]x26.[1818]) 
	//0x25:   800x600  (32x24)
	//0x40:  1280x720  (32x18!)
	//0x32:  1024x768  (32x24) 
	//0x45:  1440x900  (32x20!)
	//0x50:  1600x1200 (32x24)
	//0x52:  1680x1050 (32.[307692]...x20.1[923076])
	//0x60:  1920x1080 (32x18)

	if(out) *out = 0; //this is apparently how SOM works

	return int(float(x)/32.0f); //y
}

extern HFONT SOM::makefont(int x, int y, const char *face)
{		
	//CreateFontA relies upon Ex.detours.cpp
	if(GetCurrentThreadId()!=EX::threadmain)
	{
		assert(0); return 0; //todo: CreateFontW
	}	

	int h = SOM::fontsize(x,y); assert(x); //SOM::fov[0] 

	//observed values (for "‚l‚r –¾’©" ie. MS Mincho)	  													  
	//e0 o0 wt1000 it0 ul0 so0 cs128 op4 cp0 qu2 pf1 
	return CreateFontA(h,0,0,0,1000,0,0,0,128,4,0,2,1,face);	
}