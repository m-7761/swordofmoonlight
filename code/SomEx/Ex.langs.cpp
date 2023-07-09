
#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

#include "Ex.winnt.h" 

// additional codes from GNU sourcecode ////////////////////

#define SUBLANG_SINDHI_INDIA 0x00 
#define SUBLANG_TAMAZIGHT_ARABIC 0x01
#define SUBLANG_TAMAZIGHT_LATIN 0x02
#define SUBLANG_TIGRINYA_ETHIOPIA 0x00 
#define SUBLANG_TIGRIGNA_ETHIOPIA 0x00 //Microsoft spelling

////////////////////////////////////////////////////////////

#include "Ex.langs.h" 

EX::Locale EX::Locales[EX_MAX_LOCALES];

static const _locale_t 
Ex_langs_C = _create_locale(LC_ALL,"C");
const EX::Locale EX::Locale::C;

#define EX_LANGS_LL_CC_S 24 //ll_CC@variant
static const char *Ex_langs_ll_CC_from_LCID(LCID);		 
static bool Ex_langs_ll_CC_exists(const char**,const wchar_t**,const wchar_t**);
static LCID Ex_langs_LCID_from_ll_CC(const char *ll_CC);

EX::Locale::operator LCID()
{
	if(_LCID) return _LCID;	
	if(_locale_t!=Ex_langs_C) 
	_free_locale(_locale_t); _locale_t = 0; 
	*catalog = *language = '\0';

	const char *ll_CC = 0;
	const char **ll_CCs = (const char**)&_ll_CCs;
	const size_t desired = &_ll_CCs.desired-ll_CCs;
	const size_t instead = &_ll_CCs.instead-ll_CCs;
	const size_t program = &_ll_CCs.program-ll_CCs;
	for(size_t i=desired;i<=program;i++)
	{	
		if(i==desired&&!ll_CCs[i]&&this!=&C)
		{
			LCID conv = 0; //defaults						

			if(this==EX::Locales+EX::Locale::USR) //dates/figures 
			{
				conv = ConvertDefaultLocale(LOCALE_USER_DEFAULT);
			}
			else //if(!environment variables) //todo: LC_MESSAGES/LANG[UAGE]
			{
				conv = ConvertDefaultLocale(LOCALE_SYSTEM_DEFAULT);
			}
			
			ll_CC = Ex_langs_ll_CC_from_LCID(conv);
		}
		else if(ll_CCs[i])
		{
			if(i==instead
			 &&ll_CCs[program]&&ll_CC
			 &&!strcmp(ll_CCs[program],ll_CC)) continue;

			ll_CC = ll_CCs[i];
		}
		else continue;

		if(!languages) //20127: code for ASCII
		{
			EX::need_unicode_equivalent(20127,ll_CC,language,MAX_PATH);
			return _LCID = Ex_langs_LCID_from_ll_CC(ll_CC);
		}

		const wchar_t *lang = 0, *mo = messages;

		for(int i=0;*(lang=languages(i));i++)
		if(Ex_langs_ll_CC_exists(&ll_CC,&mo,&lang))
		{
			wcscpy(catalog,mo); wcscpy(language,lang);
			return _LCID = Ex_langs_LCID_from_ll_CC(ll_CC);
		}		
		if(i>=instead) //instead/program
		{
			for(int i=0;*(lang=languages(i));i++)
			if(FILE_ATTRIBUTE_DIRECTORY&~GetFileAttributesW(lang))
			{
				wcscpy(catalog,lang); break; //20127: code for ASCII
			}
			EX::need_unicode_equivalent(20127,ll_CC,language,MAX_PATH);
			return _LCID = Ex_langs_LCID_from_ll_CC(ll_CC);
		}
	}
	return _LCID = LANG_INVARIANT;
}

EX::Locale::operator _locale_t()
{
	if(_locale_t&&_LCID) return _locale_t;

	LCID lcid = operator LCID(); assert(!_locale_t);

	if(lcid==LANG_INVARIANT) return _locale_t = Ex_langs_C;

	char clc[96]; int cat = -1+ //inclue 0 terminator???
	GetLocaleInfoA(lcid,LOCALE_SENGLANGUAGE,clc,sizeof(clc)-1);
	if(cat<=0) return 0;

	clc[cat++] = '_'; cat+= -1+ //inclue 0 terminator???
	GetLocaleInfoA(lcid,LOCALE_SENGCOUNTRY,clc+cat,sizeof(clc)-cat);
	clc[cat] = '\0';

	return _locale_t = _create_locale(LC_ALL,clc);
}

static const char *Ex_langs_ll_CC_from_LCID(LCID in)
{
	const int out_s = EX_LANGS_LL_CC_S, iso_s = 10; 
	
	static char out[out_s];

	char iso639[iso_s], iso3166[iso_s]; 

	int out_x = 0, iso639_x = 0, iso3166_x = 0;

	iso639_x = GetLocaleInfoA(in,LOCALE_SISO639LANGNAME,iso639,iso_s);
	iso3166_x = GetLocaleInfoA(in,LOCALE_SISO3166CTRYNAME,iso3166,iso_s);

	if(iso639_x) iso639_x--; if(iso3166_x) iso3166_x--; //null terminals

	if(iso639_x+(iso3166_x?1:0)+iso3166_x+1>out_s) return 0;

	if(iso639_x) memcpy(out,iso639,out_x=iso639_x);

	if(iso3166_x) 
	{
		out[out_x++] = '_';
		memcpy(out+out_x,iso3166,iso3166_x);
		out_x+=iso3166_x;
	}

	switch(LANGIDFROMLCID(in)) //known modifiers
	{
	#define MODIFER(x) if(out_x+sizeof("@"#x)>out_s) return 0;\
	\
		memcpy(out+out_x,"@"#x,sizeof("@"#x)); out_x+=sizeof("@"#x)-1;

	case MAKELANGID(LANG_AZERI,SUBLANG_AZERI_CYRILLIC):
	case MAKELANGID(LANG_BOSNIAN,SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_CYRILLIC):
	case MAKELANGID(LANG_SERBIAN,SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_CYRILLIC):
	case MAKELANGID(LANG_SERBIAN,SUBLANG_SERBIAN_CYRILLIC):
	case MAKELANGID(LANG_MONGOLIAN,SUBLANG_MONGOLIAN_CYRILLIC_MONGOLIA):
	case MAKELANGID(LANG_UZBEK,SUBLANG_UZBEK_CYRILLIC):

		MODIFER(cyrillic) break;

	case MAKELANGID(LANG_TAMAZIGHT,SUBLANG_TAMAZIGHT_LATIN): 
	case MAKELANGID(LANG_INUKTITUT,SUBLANG_INUKTITUT_CANADA_LATIN):
	case MAKELANGID(LANG_AZERI,SUBLANG_AZERI_LATIN):
	case MAKELANGID(LANG_BOSNIAN,SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_LATIN):
	case MAKELANGID(LANG_SERBIAN,SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_LATIN):
	case MAKELANGID(LANG_SERBIAN,SUBLANG_SERBIAN_LATIN):
	case MAKELANGID(LANG_UZBEK,SUBLANG_UZBEK_LATIN):

		MODIFER(latin) break;

	case MAKELANGID(LANG_TAMAZIGHT,SUBLANG_TAMAZIGHT_ARABIC): 

		MODIFER(arabic) break;

	case MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_MODERN):

		MODIFER(modern) break;

#undef MODIFER
	}

	out[out_x] = '\0';

	return out;
}

static bool Ex_langs_ll_CC_exists
(const char **inout, const wchar_t **mo, const wchar_t **lang)
{	
	if(!inout||!*inout) return false; 

	if(!mo||!*mo||!lang||!*lang) return true;

	HANDLE glob = 0; //err
	
	//Yes the way this works is a mess
	//it's evolved to be this way, yet
	//still it'd probably benefit from
	//a complete and thorough overhaul

	const char *in = *inout;
	static char out[EX_LANGS_LL_CC_S+1];
	static wchar_t path[MAX_PATH], text[MAX_PATH];

	if(wcscpy_s(path,MAX_PATH-1,*lang)) goto err;

	if(!PathIsDirectoryW(path)) return false; //goto err;

	int	pathlen = wcslen(path); path[pathlen++] = '\\';
	int textlen = swprintf_s(text,MAX_PATH,L"\\LC_MESSAGES\\%s.mo",*mo);
	if(textlen<0) goto err;
		
	int i; wchar_t *win = path+pathlen;
	for(i=0;in[i]&&pathlen+i<MAX_PATH;i++) 
	{
		if(unsigned(in[i])<=127) win[i] = in[i]; 		
		else goto err; 
	}
	if(in[i]||wcscpy_s(win+i,MAX_PATH-pathlen-i,text))
	goto err;

	if(PathFileExistsW(path)) //perfect match
	{	   
		*mo = wcscpy(text,*lang=path); path[pathlen+i] = '\0'; 
		return true;
	}

	int _ = 0; while(_<i&&win[_]!='_'&&win[_]!='@') _++; 	
	if(!win[_]) return false;
			
	WIN32_FIND_DATAW data; 
	const wchar_t *name = data.cFileName;
	path[pathlen+_] = '*'; path[pathlen+_+1] = '\0'; //globbing	
	glob = FindFirstFileW(path,&data);
	while(glob!=INVALID_HANDLE_VALUE)
	{
		if(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
		{
			switch(name[_])
			{
			default: goto next; //not an ll_CC@variant filename

			case '\0': case '_': case '@': break; //good enough
			}

			if(wcscpy_s(path+pathlen,MAX_PATH-pathlen,name)
			  ||wcscat_s(path+pathlen,MAX_PATH-pathlen,text)) 
			  goto next;

			if(PathFileExistsW(path))
			{	
				int i;
				for(i=0;name[i]&&i<EX_LANGS_LL_CC_S;i++)
				{
					if(unsigned(name[i])<=127) out[i] = name[i];
					else goto next;						
				}						
				if(name[i]) goto next; 
								
				*mo = wcscpy(text,*lang=path); path[pathlen+i] = '\0'; 

				FindClose(glob); *inout = out; out[i] = '\0';

				return true;
			}
		}

next:	if(!FindNextFileW(glob,&data)) break;
	}
err:FindClose(glob); return false;
}

static LCID Ex_langs_LCID_from_ll_CC(const char *in)										   
{	
	if(!in||!*in||!memcmp(in,"C",2)) return LANG_INVARIANT; 

	if(!in[1]||(in[2]!='_'&&in[3]!='_')) return LANG_INVARIANT;

	char ll[4] = {0,0,0,0}, cc[4] = {0,0,0,0}; 

	if(in[2]=='_') //two letter code
	{
		ll[0] = in[1]; ll[1] = in[0]; in+=3;
	}
	else //3 letter code
	{
		ll[0] = in[2]; ll[1] = in[1]; ll[2] = in[0]; in+=4;
	}

	if(*in) cc[1] = *in++; else return LANG_INVARIANT;
	if(*in) cc[0] = *in++; else return LANG_INVARIANT;
	
	cc[2] = '\0'; 
		
	long long ll_CC = 
	(long long(*(int*)cc)<<32LL)+*(int*)ll;

	const char *at = *in?in+1:0; 
	if(at&&at[-1]!='@') return LANG_INVARIANT;
	int tag = 0; //@modifier

	if(at) //lame: fourccs are taken from MacOS X
	if(!strcmp(at,"latin"))    tag = 'Latn'; else 
	if(!strcmp(at,"cyrillic")) tag = 'Cyrl'; else
	if(!strcmp(at,"modern"))   tag = 'Modn'; else //made this one up
	if(!strcmp(at,"arabic"))   tag = 'Arab';	

#define LCID(PRIM,_,SUB) MAKELCID(MAKELANGID(LANG_##PRIM,SUBLANG_##PRIM##_##SUB),0x0) //SORT_DEFAULT

#define LSUB(PRIM,_,SUB) MAKELCID(MAKELANGID(LANG_##PRIM,SUBLANG_##SUB),0x0) //see: FARSI

#define LANG(PRIM) MAKELCID(LANG_##PRIM,0x0) //primary language only

#define LL_CC(ll,_,CC) ((long long(#@CC)<<32LL)+#@ll) //Microsoft Specific

//#define LL(ll) (#@ll) //Microsoft Specific

	switch(ll_CC)
	{									  
	default: return 0;

	case LL_CC(af,_,ZA): return LCID(AFRIKAANS,_,SOUTH_AFRICA);
	case LL_CC(sq,_,AL): return LCID(ALBANIAN,_,ALBANIA);

	case LL_CC(gsw,_,FR): return LCID(ALSATIAN,_,FRANCE);

	case LL_CC(am,_,ET): return LCID(AMHARIC,_,ETHIOPIA);	  

	case LL_CC(ar,_,DZ): return LCID(ARABIC,_,ALGERIA); 
	case LL_CC(ar,_,BH): return LCID(ARABIC,_,BAHRAIN); 
	case LL_CC(ar,_,EG): return LCID(ARABIC,_,EGYPT); 
	case LL_CC(ar,_,IQ): return LCID(ARABIC,_,IRAQ); 
	case LL_CC(ar,_,JO): return LCID(ARABIC,_,JORDAN); 
	case LL_CC(ar,_,KW): return LCID(ARABIC,_,KUWAIT); 
	case LL_CC(ar,_,LB): return LCID(ARABIC,_,LEBANON); 
	case LL_CC(ar,_,LY): return LCID(ARABIC,_,LIBYA); 
	case LL_CC(ar,_,MA): return LCID(ARABIC,_,MOROCCO); 
	case LL_CC(ar,_,OM): return LCID(ARABIC,_,OMAN); 
	case LL_CC(ar,_,QA): return LCID(ARABIC,_,QATAR); 
	case LL_CC(ar,_,SA): return LCID(ARABIC,_,SAUDI_ARABIA); 
	case LL_CC(ar,_,SY): return LCID(ARABIC,_,SYRIA); 
	case LL_CC(ar,_,TN): return LCID(ARABIC,_,TUNISIA); 
	case LL_CC(ar,_,AE): return LCID(ARABIC,_,UAE); 
	case LL_CC(ar,_,YE): return LCID(ARABIC,_,YEMEN); 

	case LL_CC(hy,_,AM): return LCID(ARMENIAN,_,ARMENIA); 
	case LL_CC(as,_,IN): return LCID(ASSAMESE,_,INDIA); 

	case LL_CC(az,_,AZ): //Azerbaijan
		
		if(tag=='Cyrl') return LCID(AZERI,_,CYRILLIC);
						return LCID(AZERI,_,LATIN); 

	case LL_CC(ba,_,RU): return LCID(BASHKIR,_,RUSSIA); 
	case LL_CC(eu,_,ES): return LCID(BASQUE,_,BASQUE); 
	case LL_CC(be,_,BY): return LCID(BELARUSIAN,_,BELARUS);

	case LL_CC(bn,_,BD): return LCID(BENGALI,_,BANGLADESH); 
	case LL_CC(bn,_,IN): return LCID(BENGALI,_,INDIA); 

	case LL_CC(bs,_,BA): //Bosnia and Herzegovina 
		
		if(tag=='Cyrl') return LCID(BOSNIAN,_,BOSNIA_HERZEGOVINA_CYRILLIC); 
						return LCID(BOSNIAN,_,BOSNIA_HERZEGOVINA_LATIN); 
	
	case LL_CC(br,_,FR): return LCID(BRETON,_,FRANCE); 
	case LL_CC(bg,_,BG): return LCID(BULGARIAN,_,BULGARIA); 
	case LL_CC(ca,_,ES): return LCID(CATALAN,_,CATALAN); 

	case LL_CC(zh,_,HK): return LCID(CHINESE,_,HONGKONG); 
	case LL_CC(zh,_,MO): return LCID(CHINESE,_,MACAU); //Macao SAR
	case LL_CC(zh,_,SG): return LCID(CHINESE,_,SINGAPORE); 
	case LL_CC(zh,_,CN): return LCID(CHINESE,_,SIMPLIFIED);
	case LL_CC(zh,_,TW): return LCID(CHINESE,_,TRADITIONAL);

	case LL_CC(co,_,FR): return LCID(CORSICAN,_,FRANCE);

	case LL_CC(hr,_,BA): return LCID(CROATIAN,_,BOSNIA_HERZEGOVINA_LATIN);
	case LL_CC(hr,_,HR): return LCID(CROATIAN,_,CROATIA);

	case LL_CC(cs,_,CZ): return LCID(CZECH,_,CZECH_REPUBLIC);
	case LL_CC(da,_,DK): return LCID(DANISH,_,DENMARK);

	case LL_CC(prs,_,AF): return LCID(DARI,_,AFGHANISTAN);

	case LL_CC(dv,_,MV): return LCID(DIVEHI,_,MALDIVES);

	case LL_CC(nl,_,BE): return LCID(DUTCH,_,BELGIAN);
	case LL_CC(nl,_,NL): return LCID(DUTCH);

	case LL_CC(en,_,AU): return LCID(ENGLISH,_,AUS);
	case LL_CC(en,_,BE): return LCID(ENGLISH,_,BELIZE);
	case LL_CC(en,_,CA): return LCID(ENGLISH,_,CAN);
	case LL_CC(en,_,GD): return LCID(ENGLISH,_,CARIBBEAN);
	case LL_CC(en,_,IN): return LCID(ENGLISH,_,INDIA);
	case LL_CC(en,_,IE): return LCID(ENGLISH,_,IRELAND);
	case LL_CC(en,_,JM): return LCID(ENGLISH,_,JAMAICA);
	case LL_CC(en,_,MY): return LCID(ENGLISH,_,MALAYSIA);	
	case LL_CC(en,_,NZ): return LCID(ENGLISH,_,NZ);
	case LL_CC(en,_,PH): return LCID(ENGLISH,_,PHILIPPINES);
	case LL_CC(en,_,SG): return LCID(ENGLISH,_,SINGAPORE);
	case LL_CC(en,_,ZA): return LCID(ENGLISH,_,SOUTH_AFRICA);
	case LL_CC(en,_,TT): return LCID(ENGLISH,_,TRINIDAD);
	case LL_CC(en,_,GB): return LCID(ENGLISH,_,UK);	   
	case LL_CC(en,_,US): return LCID(ENGLISH,_,US);
	case LL_CC(en,_,ZW): return LCID(ENGLISH,_,ZIMBABWE);

	case LL_CC(et,_,EE): return LCID(ESTONIAN,_,ESTONIA);
	case LL_CC(fo,_,FO): return LCID(FAEROESE,_,FAROE_ISLANDS);

	case LL_CC(fil,_,PH): return LCID(FILIPINO,_,PHILIPPINES);

	case LL_CC(fi,_,FI): return LCID(FINNISH,_,FINLAND);

	case LL_CC(fr,_,BE): return LCID(FRENCH,_,BELGIAN);
	case LL_CC(fr,_,CA): return LCID(FRENCH,_,CANADIAN);
	case LL_CC(fr,_,FR): return LCID(FRENCH);
	case LL_CC(fr,_,LU): return LCID(FRENCH,_,LUXEMBOURG);
	case LL_CC(fr,_,MC): return LCID(FRENCH,_,MONACO);
	case LL_CC(fr,_,CH): return LCID(FRENCH,_,SWISS);

	case LL_CC(fy,_,NL): return LCID(FRISIAN,_,NETHERLANDS);
	case LL_CC(gl,_,ES): return LCID(GALICIAN,_,GALICIAN);
	case LL_CC(ka,_,GE): return LCID(GEORGIAN,_,GEORGIA);

	case LL_CC(de,_,AT): return LCID(GERMAN,_,AUSTRIAN);
	case LL_CC(de,_,DE): return LCID(GERMAN);
	case LL_CC(de,_,LI): return LCID(GERMAN,_,LIECHTENSTEIN);
	case LL_CC(de,_,LU): return LCID(GERMAN,_,LUXEMBOURG);
	case LL_CC(de,_,CH): return LCID(GERMAN,_,SWISS);

	case LL_CC(el,_,GR): return LCID(GREEK,_,GREECE);
	case LL_CC(kl,_,GL): return LCID(GREENLANDIC,_,GREENLAND);
	case LL_CC(gu,_,IN): return LCID(GUJARATI,_,INDIA);
	case LL_CC(ha,_,NG): return LCID(HAUSA,_,NIGERIA_LATIN);
	case LL_CC(he,_,IL): return LCID(HEBREW,_,ISRAEL);
	case LL_CC(hi,_,IN): return LCID(HINDI,_,INDIA);
	case LL_CC(hu,_,HU): return LCID(HUNGARIAN,_,HUNGARY);
	case LL_CC(is,_,IS): return LCID(ICELANDIC,_,ICELAND);
	case LL_CC(ig,_,NG): return LCID(IGBO,_,NIGERIA);
	case LL_CC(id,_,ID): return LCID(INDONESIAN,_,INDONESIA);

	case LL_CC(iu,_,CA): //Canadian Syllabics	
											 
		if(tag=='Latn') return LCID(INUKTITUT,_,CANADA_LATIN); 
						return LCID(INUKTITUT,_,CANADA); 						

	case LL_CC(ga,_,IE): return LCID(IRISH,_,IRELAND);
	case LL_CC(xh,_,ZA): return LCID(XHOSA,_,SOUTH_AFRICA); 
	case LL_CC(zu,_,ZA): return LCID(ZULU,_,SOUTH_AFRICA); 

	case LL_CC(it,_,IT): return LCID(ITALIAN); 
	case LL_CC(it,_,CH): return LCID(ITALIAN,_,SWISS); 

	case LL_CC(ja,_,JP): return LCID(JAPANESE,_,JAPAN); 
	case LL_CC(kn,_,IN): return LCID(KANNADA,_,INDIA); 

	case LL_CC(ks,_,IN): return LCID(KASHMIRI,_,INDIA); 
	case LL_CC(ks,_,PK): return LCID(KASHMIRI,_,SASIA); //PK?

	case LL_CC(kk,_,KZ): return LCID(KAZAK,_,KAZAKHSTAN);
	case LL_CC(kh,_,KH): return LCID(KHMER,_,CAMBODIA);

	case LL_CC(qut,_,GT): return LCID(KICHE,_,GUATEMALA); //K'iche

	case LL_CC(rw,_,RW): return LCID(KINYARWANDA,_,RWANDA); //Kinyarwanda

	case LL_CC(kok,_,IN): return LCID(KONKANI,_,INDIA); 

	case LL_CC(ko,_,KR): return LCID(KOREAN); 
	case LL_CC(ky,_,KG): return LCID(KYRGYZ,_,KYRGYZSTAN); 
	case LL_CC(lo,_,LA): return LCID(LAO,_,LAO); 
	case LL_CC(lv,_,LV): return LCID(LATVIAN,_,LATVIA); 
	case LL_CC(lt,_,LT): return LCID(LITHUANIAN,_,LITHUANIA); 

	case LL_CC(dsb,_,DE): return LCID(LOWER_SORBIAN,_,GERMANY); 

	case LL_CC(lb,_,LU): return LCID(LUXEMBOURGISH,_,LUXEMBOURG); 
	case LL_CC(mk,_,MK): return LCID(MACEDONIAN,_,MACEDONIA); 

	case LL_CC(ms,_,BN): return LCID(MALAY,_,BRUNEI_DARUSSALAM); 
	case LL_CC(ms,_,MY): return LCID(MALAY,_,MALAYSIA); 

	case LL_CC(ml,_,IN): return LCID(MALAYALAM,_,INDIA); 
	case LL_CC(mt,_,MT): return LCID(MALTESE,_,MALTA); 

	case LL_CC(mni,_,IN): return LANG(MANIPURI); //GNU cc

	case LL_CC(mi,_,NZ): return LCID(MAORI,_,NEW_ZEALAND); 

	case LL_CC(arn,_,CL): return LCID(MAPUDUNGUN,_,CHILE); 

	case LL_CC(mr,_,IN): return LCID(MARATHI,_,INDIA); 

	case LL_CC(moh,_,CA): return LCID(MOHAWK,_,MOHAWK); //Canada
		
	case LL_CC(mn,_,MN): //Mongolian
		
		if(tag=='Cyrl') return LCID(MONGOLIAN,_,CYRILLIC_MONGOLIA); 
						return LCID(MONGOLIAN,_,PRC); //default?

	case LL_CC(ne,_,NP): return LCID(NEPALI,_,NEPAL); 
	case LL_CC(ne,_,IN): return LCID(NEPALI,_,INDIA); 

	case LL_CC(nb,_,NO): return LCID(NORWEGIAN,_,BOKMAL); 
	case LL_CC(nn,_,NO): return LCID(NORWEGIAN,_,NYNORSK); 
	case LL_CC(no,_,NO): return LANG(NORWEGIAN); //requiring cc?
	
	case LL_CC(oc,_,FR): return LCID(OCCITAN,_,FRANCE);
	case LL_CC(or,_,IN): return LCID(ORIYA,_,INDIA);
	case LL_CC(ps,_,AF): return LCID(PASHTO,_,AFGHANISTAN);

	case LL_CC(fa,_,IR): return LSUB(FARSI,_,PERSIAN_IRAN);

	case LL_CC(pl,_,PL): return LCID(POLISH,_,POLAND);
	case LL_CC(pt,_,BR): return LCID(PORTUGUESE,_,BRAZILIAN);
	case LL_CC(pt,_,PT): return LCID(PORTUGUESE);
	case LL_CC(pa,_,IN): return LCID(PUNJABI,_,INDIA);

	case LL_CC(quz,_,BO): return LCID(QUECHUA,_,BOLIVIA);
	case LL_CC(quz,_,EC): return LCID(QUECHUA,_,ECUADOR);
	case LL_CC(quz,_,PE): return LCID(QUECHUA,_,PERU);

	case LL_CC(ro,_,RO): return LCID(ROMANIAN,_,ROMANIA);
	case LL_CC(rm,_,CH): return LCID(ROMANSH,_,SWITZERLAND);
	case LL_CC(ru,_,RU): return LCID(RUSSIAN,_,RUSSIA);

	case LL_CC(smn,_,FI): return LCID(SAMI,_,INARI_FINLAND);
	case LL_CC(smj,_,NO): return LCID(SAMI,_,LULE_NORWAY);
	case LL_CC(smj,_,SE): return LCID(SAMI,_,LULE_SWEDEN);

	case LL_CC(se,_,FI): return LCID(SAMI,_,NORTHERN_FINLAND);
	case LL_CC(se,_,NO): return LCID(SAMI,_,NORTHERN_NORWAY);
	case LL_CC(se,_,SE): return LCID(SAMI,_,NORTHERN_SWEDEN);

	case LL_CC(sms,_,FI): return LCID(SAMI,_,SKOLT_FINLAND);
	case LL_CC(sma,_,NO): return LCID(SAMI,_,SOUTHERN_NORWAY);
	case LL_CC(sma,_,SE): return LCID(SAMI,_,SOUTHERN_SWEDEN);

	case LL_CC(sa,_,IN): return LCID(SANSKRIT,_,INDIA);
	case LL_CC(sr,_,BA):

		if(tag=='Cyrl') return LCID(SERBIAN,_,BOSNIA_HERZEGOVINA_CYRILLIC); 
						return LCID(SERBIAN,_,BOSNIA_HERZEGOVINA_LATIN);

	case LL_CC(sr,_,HR): return LCID(SERBIAN,_,CROATIA);	
	case LL_CC(sr,_,CS):
									
		if(tag=='Cyrl') return LCID(SERBIAN,_,CYRILLIC); 
						return LCID(SERBIAN,_,LATIN);

	case LL_CC(nso,_,ZA): return LCID(SOTHO,_,NORTHERN_SOUTH_AFRICA);	

	case LL_CC(tn,_,ZA): return LCID(TSWANA,_,SOUTH_AFRICA);	

	case LL_CC(sd,_,IN): return LCID(SINDHI,_,INDIA);
	case LL_CC(sd,_,AF): return LCID(SINDHI,_,AFGHANISTAN); //AF?	
	case LL_CC(sd,_,PK): return LCID(SINDHI,_,PAKISTAN);	

	case LL_CC(si,_,LK): return LCID(SINHALESE,_,SRI_LANKA);	
	case LL_CC(sk,_,SK): return LCID(SLOVAK,_,SLOVAKIA);	

	case LL_CC(sl,_,SI): return LCID(SLOVENIAN,_,SLOVENIA);	

	case LL_CC(es,_,AR): return LCID(SPANISH,_,ARGENTINA);	
	case LL_CC(es,_,BO): return LCID(SPANISH,_,BOLIVIA);
	case LL_CC(es,_,CL): return LCID(SPANISH,_,CHILE);
	case LL_CC(es,_,CO): return LCID(SPANISH,_,COLOMBIA);
	case LL_CC(es,_,CR): return LCID(SPANISH,_,COSTA_RICA);
	case LL_CC(es,_,DO): return LCID(SPANISH,_,DOMINICAN_REPUBLIC);
	case LL_CC(es,_,EC): return LCID(SPANISH,_,ECUADOR);
	case LL_CC(es,_,SV): return LCID(SPANISH,_,EL_SALVADOR);
	case LL_CC(es,_,GT): return LCID(SPANISH,_,GUATEMALA);
	case LL_CC(es,_,HN): return LCID(SPANISH,_,HONDURAS);
	case LL_CC(es,_,MX): return LCID(SPANISH,_,MEXICAN);
	case LL_CC(es,_,NI): return LCID(SPANISH,_,NICARAGUA);
	case LL_CC(es,_,PA): return LCID(SPANISH,_,PANAMA);
	case LL_CC(es,_,PY): return LCID(SPANISH,_,PARAGUAY);
	case LL_CC(es,_,PE): return LCID(SPANISH,_,PERU);
	case LL_CC(es,_,PR): return LCID(SPANISH,_,PUERTO_RICO);
	case LL_CC(es,_,ES): 
		
		if(tag=='Modn') return LCID(SPANISH,_,MODERN);
						return LCID(SPANISH); 

	case LL_CC(es,_,US): return LCID(SPANISH,_,US); 
	case LL_CC(es,_,UY): return LCID(SPANISH,_,URUGUAY); 	
	case LL_CC(es,_,VE): return LCID(SPANISH,_,VENEZUELA);

	case LL_CC(sw,_,KE): return LCID(SWAHILI);
	case LL_CC(sv,_,FI): return LCID(SWEDISH,_,FINLAND);
	case LL_CC(sv,_,SE): return LCID(SWEDISH,_,SWEDEN);

	case LL_CC(syr,_,SY): return LCID(SYRIAC);

	case LL_CC(tg,_,TJ): return LCID(TAJIK,_,TAJIKISTAN); //Cyrillic

	case LL_CC(ber,_,MA): //GNU...
	
		if(tag=='Arab') return LCID(TAMAZIGHT,_,ARABIC);
						return LCID(TAMAZIGHT,_,LATIN);

	case LL_CC(tzm,_,DZ): return LCID(TAMAZIGHT,_,ALGERIA_LATIN);

	case LL_CC(ta,_,IN): return LCID(TAMIL,_,INDIA);
	case LL_CC(tt,_,RU): return LCID(TATAR,_,RUSSIA);
	case LL_CC(te,_,IN): return LCID(TELUGU,_,INDIA);
	case LL_CC(th,_,TH): return LCID(THAI,_,THAILAND);	
	case LL_CC(bo,_,CN): return LCID(TIBETAN,_,PRC);

	case LL_CC(ti,_,ET): return LCID(TIGRIGNA,_,ETHIOPIA);
	case LL_CC(ti,_,ER): return LCID(TIGRIGNA,_,ERITREA);

	case LL_CC(tr,_,TR): return LCID(TURKISH,_,TURKEY);
	case LL_CC(tk,_,TM): return LCID(TURKMEN,_,TURKMENISTAN);
	case LL_CC(uk,_,UA): return LCID(UKRAINIAN,_,UKRAINE);

	case LL_CC(hsb,_,DE): return LCID(UPPER_SORBIAN,_,GERMANY);

	case LL_CC(ur,_,IN): return LCID(URDU,_,INDIA);
	case LL_CC(ur,_,PK): return LCID(URDU,_,PAKISTAN);

	case LL_CC(ug,_,CN): return LCID(UIGHUR,_,PRC);

	case LL_CC(uz,_,UZ): //Uzbek
		
		if(tag=='Cyrl') return LCID(UZBEK,_,CYRILLIC);
						return LCID(UZBEK,_,LATIN);

	case LL_CC(vi,_,VN): return LCID(VIETNAMESE,_,VIETNAM);
	case LL_CC(cy,_,GB): return LCID(WELSH,_,UNITED_KINGDOM);
	case LL_CC(wo,_,SN): return LCID(WOLOF,_,SENEGAL);

	case LL_CC(sah,_,RU): return LCID(YAKUT,_,RUSSIA);

	case LL_CC(ii,_,CN): return LCID(YI,_,PRC);
	case LL_CC(yo,_,NG): return LCID(YORUBA,_,NIGERIA);
	}
}