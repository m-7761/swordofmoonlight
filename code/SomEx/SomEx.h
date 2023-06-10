
//Reminder: I think the last version
//number is never 0 but I can't recall
//why! odd is for demos, so 2 is lowest?
#ifndef SOMEX_VERSION
#define SOMEX_VERSION 1,2,4,6
#define SOMEX_VNUMBER 0x1020406UL
#define SOMEX_VSTRING "1, 2, 4, 6"
#define SOMEX_WSTRING L"1, 2, 4, 6"
extern int SOMEX_vnumber; //0x1020404UL
#endif

#ifndef SOMEX_INCLUDED
#define SOMEX_INCLUDED

/*These are virtual drives*/
#define SOMEX_(ABC) #ABC ":\\>"
#define SOMEX_L(ABC) L#ABC L":\\>"

#ifdef EX_INCLUDED //PCH
#define SOMEX_API extern "C" __declspec(dllexport) 
#endif

#ifndef SOMEX_API 
#define SOMEX_API static 
#include "../Somversion/Somversion.hpp" 
#define SOMEX_API_FINALE(type,API,prototype,arguments) \
SOMVERSION_HPP_INTERFACE(SomEx,type,API,prototype,arguments)
#elif !defined(SOMEX_API_FINALE)					   
#define SOMEX_API_FINALE(...) ;
#endif

/*Outside of DllMain initialization since 1.1.2.3 
//
//Sword_of_Moonlight is to allow GetOpenFileName to work
//so that if a project (SOM file) is not specified, and there
//is more than one file to choose from, then the user can be asked
//to intervene. HMDOULE is SomEx.dll. Usually you'd want to FreeLibrary
//
//cwd: must be the current working directory*/
SOMEX_API HMODULE Sword_of_Moonlight(const wchar_t *cwd)
SOMEX_API_FINALE(HMODULE,Sword_of_Moonlight,(const wchar_t*),(cwd))

#endif /*SOMEX_INCLUDED*/

/*RC1004*/
