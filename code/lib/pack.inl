
#ifdef SWORDOFMOONLIGHT_PACK
#if defined(_WIN32)
#ifdef SWORDOFMOONLIGHT_PACK_POP
#	pragma pack(pop)
#	undef SWORDOFMOONLIGHT_PACK_POP
#else
#	pragma pack(push,1)
#	define SWORDOFMOONLIGHT_PACK_POP
#endif
#pragma warning(disable:4103) //suppress 
#endif //_WIN32
#endif
