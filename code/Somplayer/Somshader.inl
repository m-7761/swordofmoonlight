
//private implementation
#ifndef SOMSHADER_INCLUDED
#define SOMSHADER_INTERNAL

struct Somshader_inl
{		
	Somthread_h::account refs; 
	
	void *media; size_t length; bool expired;

	Somshader_inl() : refs(1)
	{
		media = 0; length = 0; expired = false;
	}	
};

#define private public 
#include "Somshader.h"
#undef private

#else
#ifdef SOMSHADER_INTERNAL //"pimpl"//
	
	mutable Somshader_inl Somshader_inl;

#endif //SOMSHADER_INTERNAL
#endif //SOMSHADER_INCLUDED