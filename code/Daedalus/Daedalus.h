
#ifndef DAEDALUS_H_INCLUDED
#define DAEDALUS_H_INCLUDED
							 
//BE MINIMALISTIC
#include <string> 
#include <limits> 
#include <cassert>
//std::min/std::max
#include <algorithm>

#ifdef _
#error please push/pop _ before/after these headers
#endif
#ifdef WIN32
#define DAEDALUS_API extern "C" __declspec(dllexport)
//_SCL_SECURE_NO_WARNINGS (or _CRT_SECURE_NO_WARNINGS?)
//#pragma warning(error:4996) 
#define DAEDALUS_DEPRECATED(x) __declspec(deprecated(x))
#define DAEDALUS_CALLMETHOD __declspec(noinline)
#define DAEDALUS_SUPPRESS_C(xxxx) __pragma(warning(suppress:xxxx))
//possible loss of data
#pragma warning(disable:4244) 
//i of for-loop ignored
#pragma warning(disable:4258) 
//redundant cv qualifier
#pragma warning(disable:4114) 
#else
#error POSIX
#endif

#endif //DAEDALUS_H_INCLUDED