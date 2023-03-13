
#ifndef JSOMAP_INCLUDED
#define JSOMAP_INCLUDED

#ifdef __cplusplus
extern "C" {			 
#define _0 =0
#else
#define _0
#endif

//wraps around bmp2png.c

//2018 HACK: jsomap_init is
//having 64-bit build issues
#include "lib/pack.inl" /*push...*/	

	//from common.h (bmp2png)
	typedef struct //tagIMAGE 
	{
	//	LONG    width;
	//	LONG    height;
	//	UINT    pixdepth;
	//	UINT    palnum; 
	//	BOOL    topdown;
		int alpha; //BOOL
		/* ----------- */
		unsigned long rowbytes;	//DWORD
		unsigned long imgbytes;	//DWORD
		struct
		{
			unsigned char r, g, b; //png_color

		}             *palette; //PALETTE
		unsigned char **rowptr; //BYTE
		unsigned char *bmpbits; //BYTE
		/* ----------- */
	//	png_color_8 sigbit;

	}
#if defined(__GNUC__)
__attribute__((packed))
#endif
	jsomap_t; //IMAGE;
	
//2018 HACK: jsomap_init is
//having 64-bit build issues
#include "lib/pack.inl" /*pop...*/	

typedef jsomap_t *(*jsomap_format_t)(int,int);

//fd: file descriptor
//seek: from beginning of file (tim only)
extern jsomap_t *jsomap_bmp(int fd _0, int seek _0);
extern jsomap_t *jsomap_txr(int fd _0, int seek _0);
extern jsomap_t *jsomap_tim(int fd _0, int seek _0);

//png: bitmap will be written to png before freed
extern int jsomap_free(jsomap_t *free, const char *png _0);

#undef _0
#ifdef __cplusplus
} //extern "C"
#endif

#endif
