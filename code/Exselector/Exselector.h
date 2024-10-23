
#ifndef EXSELECTOR_VERSION
#define EXSELECTOR_VERSION 0,0,0,4
#define EXSELECTOR_VSTRING "1, 0, 0, 4"
#define EXSELECTOR_WSTRING L"1, 0, 0, 4"
#endif

#ifndef EXSELECTOR_INCLUDED
#define EXSELECTOR_INCLUDED

typedef int BOOL;

#include "Exselector.hpp"

#ifdef EXSELECTOR_PCH_INCLUDED //PCH
#define EXSELECTOR_API extern "C" __declspec(dllexport) 
#endif

#ifndef EXSELECTOR_API 
#define EXSELECTOR_API static 
#include "../Somversion/Somversion.hpp" 
#define EXSELECTOR_API_FINALE(type,API,prototype,arguments) \
SOMVERSION_HPP_INTERFACE(Exselector,type,API,prototype,arguments)
#elif !defined(EXSELECTOR_API_FINALE)					   
#define EXSELECTOR_API_FINALE(...) ;
#endif

/*SKETCHY 
//Load Exselector.dll with (Somversion) component update dialog?
*/
EXSELECTOR_API Exselector *Sword_of_Moonlight_Extension_Selector(BOOL system)
EXSELECTOR_API_FINALE(Exselector*,Sword_of_Moonlight_Extension_Selector,(BOOL),(system))

#endif //EXSELECTOR_INCLUDED
