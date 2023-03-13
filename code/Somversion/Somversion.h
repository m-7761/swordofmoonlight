
#ifndef SOMVERSION_VERSION
#define SOMVERSION_VERSION 1,0,4,8
#define SOMVERSION_VSTRING "1, 0, 4, 8"
#define SOMVERSION_WSTRING L"1, 0, 4, 8"
#endif

#ifndef SOMVERSION_INCLUDED
#define SOMVERSION_INCLUDED
															 
#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#pragma push_macro("_")
#ifndef __cplusplus
#define _(default_argument)
#else
#define _(default_argument) = default_argument 
#endif					  
													  
#define SOMVERSION_LIB(x) Sword_of_Moonlight_Subversion_Library_##x

#ifdef SOMVERSION_API_ONLY
#pragma push_macro("SOMVERSION")
#endif											

/*#define SOMVERSION SOMVERSION_LIB(quartet)*/
#define SOMVERSION struct SOMVERSION_LIB(quartet_pod)

extern "C" struct SOMVERSION_LIB(quartet_pod)
{
	wchar_t v[4]; /*todo: 32-bit?*/

	enum /*Version timeout argument*/
	{ 
	noupdate  =  0, /*just return version*/
	notimeout = -1, /*wait on GUI forever*/
	queueup   = -2, /*add to force update*/
	doupdate  = -3, /*force update dialog*/
	};
};

#ifndef SOMVERSION_API 
#define SOMVERSION_API static 
#include "../Somversion/Somversion.hpp" 
#define SOMVERSION_API_FINALE(type,API,prototype,arguments) \
SOMVERSION_HPP_INTERFACE(Somversion,type,SOMVERSION_LIB(API),prototype,arguments)
#elif !defined(SOMVERSION_API_FINALE)
#define SOMVERSION_API_FINALE(...) ;
#endif

SOMVERSION_API
/*Version: obtain version of a PE file with update
//HWND: parent window for modal dialog (if necessary)
//PE will be converted to an abosolute path if successful
//if PE is a not a binary (executable) file it is treated as a CSV file
//if treated thus, PE will be overwritten with the product of the CSV dialog 
//timeout: number of milliseconds for dialog to wait for any manual intervention
//Or any of SOMVERSION's enum values above (noupdate, notimeout, queueup, doupdate)
//will return ?.?.?.0 if the PE file cannot be resolved or lacks version information*/
SOMVERSION SOMVERSION_LIB(Version)(HWND owner, wchar_t pe[MAX_PATH], int timeout _(-1)) 
SOMVERSION_API_FINALE(SOMVERSION,Version,(HWND,wchar_t[MAX_PATH],int),(owner,pe,timeout))

SOMVERSION_API
/*Assist: assist in the update of problem files (eg. Somversion.dll)
//Same deal as Version except the new PE file is stored in a staging folder
//PE[1] should contain the name of the assisting application...
//On a nonzero return PE[1] will be empty or contain the path to the staged PE file
//The role of the assistee is to copy PE[1] over PE[0] (and delete PE[1])*/
SOMVERSION SOMVERSION_LIB(Assist)(HWND owner, wchar_t pe[2][MAX_PATH], int timeout _(-1))
SOMVERSION_API_FINALE(SOMVERSION,Assist,(HWND,wchar_t[2][MAX_PATH],int),(owner,pe,timeout))

SOMVERSION_API //1.0.4.2
/*Temporary: exposes Somfolder() internal API.
//I feel like this is rushed and unideal. It's needed to hook up COLLADA-SOM
//with the temporary folder. And come back to locate the fruits of its labor.
//If components do this independently they're liable to be on the wrong page.
//Somlegalize is applied to all but for slash.
//if inout does not begin with a slash it is not appended to the TEMP folder.
//Somlegalize is applied to it, and no folder is made.
//If inout is "" the temporary folder is retrieved. Historically this is the
//%TEMP%/Swordofmoonlight.net folder. SOM files can change the TEMP variable.
//Any initial slash is not passed-through Somleagalize.*/
wchar_t *SOMVERSION_LIB(Temporary)(wchar_t inout[MAX_PATH], int slash)
SOMVERSION_API_FINALE(wchar_t*,Temporary,(wchar_t[MAX_PATH],int),(inout,slash))

#ifdef __cplusplus
#undef SOMVERSION
#define SOMERSION SOMVERSION_LIB(quartet_oop)
struct SOMVERSION:SOMVERSION_LIB(quartet_pod)
{		
	inline operator bool()const{ return v[3]; }	
	inline int operator[](int i)const{ return v[i]; }
	inline bool operator==(const SOMVERSION &cmp)const
	{
		if(!*this||!cmp) return false;
		for(int i=0;i<4;i++) if(v[i]!=cmp[i]) return false;
		return true;
	}
	inline bool operator>=(const SOMVERSION &cmp)const
	{
		if(!*this||!cmp) return false;
		for(int i=0;i<3;i++) if(v[i]>cmp[i]) return true;
		return v[3]>=cmp[3];
	}
	inline bool operator<(const SOMVERSION &cmp)const
	{
		return !(*this>=cmp); /*true if !cmp||!*this*/
	}	
	template<typename i> inline 
	SOMVERSION(i a,i b,i c,i d)
	{
		v[0] = a; v[1] = b; v[2] = c; v[3] = d;
	}
	SOMVERSION(){ v[3] = 0; }
	SOMVERSION(const SOMVERSION_LIB(quartet_pod) &C)
	{
		int compile[sizeof(*this)==sizeof(C)];
		*this = (SOMVERSION&)C;
	}
	SOMVERSION &version(HWND a,wchar_t(&b)[MAX_PATH],int c=-1)
	{
		return *this = SOMVERSION_LIB(Version)(a,b,c);
	}
	template<typename pe> inline 
	SOMVERSION &version(HWND owner,pe cp,int to=-1)
	{
		wchar_t pe[MAX_PATH] = L""; if(cp) wcscpy(pe,cp);
		return version(owner,pe,to); 
	}
};	  
#endif /*__cplusplus*/
typedef SOMVERSION SOMVERSION_LIB(quartet);

#ifdef SOMVERSION_API_ONLY
#pragma pop_macro("SOMVERSION")
#endif

#pragma pop_macro("_")

#endif /*SOMVERSION_INCLUDED*/

/*RC1004*/
