
#ifndef DAEDALUS_PCH_INCLUDED
#define COLLADA_DOM_MAKER \
"Daedalus 3-D (Collada.pch.cpp)"
#endif 
////////////////////////////
//HACK? Daedalus.(c).h does:
//#include "Collada.pch.cpp"
#undef VOID
#define IMPORTING_COLLADA_DOM
#define COLLADA_NOLEGACY
#include <ColladaDOM.inl>
#define COLLADA__inline__
#define COLLADA__http_www_collada_org_2008_03_COLLADASchema__namespace \
COLLADA_DOM_NICKNAME(COLLADA_1_5_0,http_www_collada_org_2008_03_COLLADASchema)
#undef RELATIVE
#undef CONST
#undef RGB
#include <COLLADA/COLLADA.h>
////////////////////////////
#ifndef DAEDALUS_PCH_INCLUDED
extern COLLADA::daePShare COLLADA::DOM_process_share = 0;
extern COLLADA::daeClientString ColladaAgent(COLLADA::XS::Schema *xs)
{
	return COLLADA_DOM_MAKER;
}
#endif