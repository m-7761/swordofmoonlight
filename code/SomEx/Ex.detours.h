
#ifndef EX_DETOURS_INCLUDED
#define EX_DETOURS_INCLUDED

namespace EX
{	
	//user handler: attach/detach Microsoft Detours hooks
	extern void (*detours)(LONG (WINAPI*)(PVOID*,PVOID)); 
	//int: primarily a courtesy to assist static initialization
	extern int detouring(void(*)(LONG(WINAPI*)(PVOID*,PVOID))); 
	
	extern void setting_up_Detours(ULONG threadID=0);

	extern bool is_Detoured();

	extern void cleaning_up_Detours(ULONG=0); 	
}
	  
#endif //EX_DETOURS_INCLUDED
