			 
#ifndef EX_MACRO_INCLUDED
#define EX_MACRO_INCLUDED

namespace EX{
//Macro: Work in progress
//
struct Macro
{
private:

	const char *At;

public: 

	//macro from xmacro
	Macro(unsigned short x);

	//variadic definition
	Macro(const char*,...);

	Macro(int n, unsigned char, int=0,...);

	//new macro spliced from many
	Macro(unsigned short*, size_t n);

	unsigned short x()const; //xmacro

	inline operator unsigned short()const
	{
		return At?x():0;
	}
};}