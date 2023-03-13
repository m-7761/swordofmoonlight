				  
#ifndef EX_LANGS_INCLUDED
#define EX_LANGS_INCLUDED

#ifndef EX_MAX_LOCALES
#define EX_MAX_LOCALES EX::Locale::TOTAL
#endif
	  
//HACK: turns out these functions return
//nonsense given greater than 7-bit ints
//can't remember to always use isw forms
//WHY'S THIS NOT EASIER?
#define isalpha iswalpha
#define isdigit iswdigit
#define isspace iswspace
#define ispunct iswpunct
#define isprint iswprint
#define isupper iswupper
#define islower iswlower
#define toupper towupper
#define tolower towlower
#define ispunct iswpunct

namespace EX{
extern struct Locale 
{	
	//_how this works_
	//Locale represents an LCID/_locale_t
	//which is retrieved according to its
	//various const members acting like a
	//kind of configuration space that is
	//accessed by operator LCID/_locale_t
	//LCID cannot return 0/LOCALE_NEUTRAL
	//so it defaults to 7f/LANG_INVARIANT 
	//which _locale_t will convert into C
	operator LCID(); operator _locale_t();			
	//hack: populate catalog and language
	inline void ready(){ operator LCID(); }
	//NEW: these get filled out automatically 
	//by operator LCID and or operator _locale_t
	wchar_t catalog[MAX_PATH], language[MAX_PATH];
		
	//at present Locale uses ConvertDefaultLocale
	//it is to be extended by way of a user handler
	//it can use to get at LC_MESSAGES and LANG[UAGE]

	//you must supply these two
	//messages is the LC_MESSAGES textdomain
	//languages enumerates "lang" folders and MO catalogs
	const wchar_t *messages, *(*languages)(int);
		
	//you must supply these too
	//language_territory@variant only "ll_CCs"
	//
	// these get used to determine the most
	// appropriate available locale if any.
	//
	struct ll_CCs
	{
		const char *desired; //over GetThreadLocale
		const char *missing; //GetThreadLocale that is
		const char *install; //the distribution locale
		const char *others0; //...
		const char *instead; //locale of last resort
		const char *program; //locale of the program

	}_ll_CCs; LCID _LCID; _locale_t _locale_t;
	ll_CCs *operator->() //NEW: keep it simple
	{
		_LCID = 0; return &_ll_CCs;
	}	
	Locale(){ memset(this,0x00,sizeof(*this)); }

	static const EX::Locale C; //internals
	inline operator LCID()const
	{ return ((Locale*)this)->operator LCID(); }
	inline operator _locale_t()const
	{ return ((Locale*)this)->operator _locale_t(); }

	//recommended EX::Locales[EX_MAX_LOCALES] selectors
	enum{ APP=0, SYS, USR, INI, CON, ERR, ADD, TOTAL };

}Locales[EX_MAX_LOCALES];} //EX::

#endif //EX_LANGS_INCLUDED