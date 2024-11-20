													   
#include "Exselector.pch.h"

#include "../SomEx/Ex.h"

//#ifndef NO_EX_H 
extern int EX::context(){ assert(0); return 0; }
extern const wchar_t *EX::log(){ assert(0); return L"Exselector"; }
extern const wchar_t *EX::ini(int){ assert(0); return nullptr; }
extern const wchar_t *EX::user(int){ assert(0); return nullptr; }
extern void EX::numbers(){ assert(0); };
extern void EX::numbers_mismatch(bool,const wchar_t*){ assert(0); }
extern bool EX::attached = true;
extern bool EX::detached = false;
extern bool EX::ok_generic_failure(const wchar_t *one_liner,...)
{
	assert(0); return true;
}
extern void EX::is_needed_to_shutdown_immediately(int exitcode, const char *caller)
{
	assert(0); return;
}