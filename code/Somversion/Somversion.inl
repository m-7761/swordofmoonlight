
#ifndef SOMVERSION_INL
#define SOMVERSION_INL(x) x
#endif

typedef SOMVERSION_LIB(quartet) SOMVERSION_INL(quartet);

inline SOMVERSION_INL(quartet) SOMVERSION_INL(Version)(HWND parent, wchar_t PE[MAX_PATH], int timeout=-1)
{
	return SOMVERSION_LIB(Version)(parent,PE,timeout);
}

inline SOMVERSION_INL(quartet) SOMVERSION_INL(Assist)(HWND parent, wchar_t PE[2][MAX_PATH], int timeout=-1)
{
	return SOMVERSION_LIB(Assist)(parent,PE,timeout);
}

inline SOMVERSION_INL(quartet) SOMVERSION_INL(Recover)(wchar_t PE[MAX_PATH])
{
	return SOMVERSION_LIB(Recover)(PE);
}