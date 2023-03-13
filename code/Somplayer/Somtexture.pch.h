					
#ifndef SOMTEXTURE_PCH_INCLUDED
#define SOMTEXTURE_PCH_INCLUDED

//// Somtexture.pch.h is to be included by /////
//Somtexture/Somgraphic/Somshader/Somenviron.cpp
//mainly so that SomEx.dll can easily replace it

#define SOMSHADER_C
#define SOMSHADER_ASM
#define SOMSHADER_HLSL

#define SOMTEXTURE_MAX 512

#include <cmath> 
#include <cassert>

//precompiling
#include <map>
#include <vector>

#define NOMINMAX 
#define WIN32_LEAN_AND_MEAN	

#include <windows.h>
#include <shlwapi.h>

#pragma comment(lib,"shlwapi.lib")	

//import w/ D3DX
#include <d3d9.h>
#include <d3dx9.h>

#ifdef _DEBUG
#pragma comment(lib,"d3dx9d.lib")
#else
#pragma comment(lib,"d3dx9.lib")
#endif

//Somplayer.pch.h
extern void Somdelete(void*); 

//Somplayer.pch.h
extern wchar_t *Somlibrary(wchar_t[MAX_PATH]);

namespace Somtexture_pch
{
	inline void remember(void *mem)
	{
		Somdelete(mem);
	}				
	const wchar_t *library(wchar_t inout[MAX_PATH])
	{
		return Somlibrary(inout);		
	}
}

#endif //SOMTEXTURE_PCH_INCLUDED