																		  						  
#ifndef SOMSPEAKER_INCLUDED
#define SOMSPEAKER_INCLUDED

#include "Somcontrol.h"

class Somspeaker
{
public:
	
	//Somplayer::id 
	inline long &headphones()
	{
		static long zero = 0; //compiler

		const Somcontrol *p = controls(); assert(p);
		
		if(!p) return zero = 0;
		
		return p->portholder;
	}
	
	Somcontrol *controls(); //does not return an array

	//unimplemented (todo: things w/ sound)
};

#endif //SOMSPEAKER_INCLUDED