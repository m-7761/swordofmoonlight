
#ifndef X2MDL_VERSION
#define X2MDL_VERSION 1,0,0,4
#define X2MDL_VSTRING "1, 0, 0, 4"
#define X2MDL_WSTRING L"1, 0, 0, 4"
#endif

#ifndef X2MDL_INCLUDED
#define X2MDL_INCLUDED

#ifdef X2MDL_PCH_INCLUDED //PCH
#define X2MDL_API extern "C" __declspec(dllexport) 
#endif

#ifndef X2MDL_API 
#define X2MDL_API static 
#include "../Somversion/Somversion.hpp" 
#define X2MDL_API_FINALE(type,API,prototype,arguments) \
SOMVERSION_HPP_INTERFACE(x2mdl,type,API,prototype,arguments)
#elif !defined(X2MDL_API_FINALE)					   
#define X2MDL_API_FINALE(...) ;
#endif

namespace x2mdl_h
{
	//SomEx.dll uses these to copy runtime files that are
	//up-to-date but are at a different location.
	enum
	{
		_lnk=1, //i.e. "shortcut"
		_mdl=2,_mdo=4,_bp=8,_cp=16,
		_msm=64,
		_mhm=128,
		_txr=256, //SFX
		_runtime_mask = _mdl|_mdo|_msm|_mhm|_txr,
		_art_shift = 23,
		_art = 1<<_art_shift,
		_data_shift = 24, //high 8 bits		
	};
}

/*SKETCHY 
//Load x2mdl.dll with (Somversion) component update dialog?
*/
X2MDL_API int x2mdl(int argc, const wchar_t* argv[], HWND hwnd)
X2MDL_API_FINALE(int,x2mdl,(int,const wchar_t*[],HWND),(argc,argv,hwnd))

X2MDL_API int cpgen(int argc, const wchar_t* argv[])
X2MDL_API_FINALE(int,cpgen,(int,const wchar_t*[]),(argc,argv))

#endif //X2MDL_INCLUDED
