
#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

		/*2021

		This was assisting som.keys.h ... it might have its
		uses, I'm not sure, but it's not being used for now

		*/
		#error REMOVED FROM BUILD

#include "Ex.dataset.h"

EX_DATA_DEFINE(EX::DATA::Unknown)
	
extern void EX::DATA::Nul(void* &set)
{				
	EX::DATA::Bit<EX::DATA::Unknown> *p = 
	(EX::DATA::Bit<EX::DATA::Unknown>*)set, *q = 0;

	while(p)
	{
		q = p; p = p->next; delete[] (BYTE*)q;
	}

	set = 0;
}