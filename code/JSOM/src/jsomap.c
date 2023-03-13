
#include <assert.h> //2018
#include "jsomap.h"

//2017: vcpkg libpng does not include zlib.
#include <zlib.h>

#define main bmp2png_main_nonentrant
#include "bmp2png.c.inl"
#undef main

#ifdef _WIN32
//2017: Need uint16_t below. Requires MSVC2015.
//NOTE: Could use SWORDOFMOONLIGHT_NSTDINT but
//all that's desired is to debug it on Windows.
#include <stdint.h>
#endif
//MVSV2015 falls on its face???????????
//#include <algorithm> //2017: std::min

#define jsomap_max 512
static size_t jsomap_off = 0;
static jsomap_t jsomap_bad = {0,0,0,0,0};
static int jsomap_pow2[] = {1,2,4,8,16,32,64,128,256,jsomap_max};
static char jsomap_pad[jsomap_max*4];
static char jsomap_tmp[jsomap_max*4];
static BOOL jsomap_colorkey(IMAGE*);
static BOOL jsomap_powertwo(IMAGE*);

static BOOL jsomap_init()
{
	quietmode = 1; //bmp2png.c.inl

	IMAGE *nul = 0;
	if(sizeof(jsomap_t) //paranoia
	!=(size_t)((char*)&nul->sigbit-(char*)&nul->alpha))
	{
		//ideally this would be a compile time error

		assert(0); //2018: 64-bit had raised this
		
		return FALSE; //TODO: complain about something
	}
	else jsomap_off = (size_t)&nul->alpha;

	return TRUE;
}

extern jsomap_t *jsomap_bmp(int fd, int _)
{	
	if(!jsomap_init()) return &jsomap_bad;

	IMAGE tmp; jsomap_t *out = &jsomap_bad; 

	int oldin = dup(0); //stdin

	if(oldin!=-1&&!dup2(fd,0))
	{
		quietmode = 1; //bmp2png.c.inl

		if(read_bmp(0,&tmp)
		 &&jsomap_colorkey(&tmp))
		{
			IMAGE *cp = malloc(sizeof(tmp));
			
			if(cp) //always true??
			{
				memcpy(cp,&tmp,sizeof(tmp));
				out = (jsomap_t*)&cp->alpha;						
			}
		}	   
	}

	dup2(oldin,0);

	return out;
}

#ifndef _MSC_VER
#include <stdint.h>
#else
typedef unsigned __int8 uint8_t;
typedef __int32 int32_t;
#endif					  

extern jsomap_t *jsomap_txr(int fd, int _)
{		
	jsomap_t *out = &jsomap_bad;

	if(!jsomap_init()) return out;
			
	FILE *f = fdopen(fd,"rb"); if(!f) return out;

	size_t fsize = 0; 			
	fseek(f,0,SEEK_END); fsize = ftell(f);
	fseek(f,0,SEEK_SET);

	int32_t hd[4]; //header
	size_t rd = fread(hd,16,1,f);
	
	//minima
	if(!rd||hd[0]<1||hd[1]<1||hd[3]<1) return out;

	//maxima (arbitrary)
	if(hd[0]>jsomap_max||hd[1]>jsomap_max) return out;

	//implemented depths
	if(hd[2]!=8&&hd[2]!=24) return out;
	   
	int rowsz = hd[0]*hd[2]/8; if(rowsz%4) rowsz+=4-rowsz%4;
		
	int totalsz = 16+hd[1]*rowsz; if(hd[2]==8) totalsz+=256*4;

	int imgsz = hd[1]*rowsz; if(fsize<totalsz) return out;

	IMAGE tmp;
	tmp.alpha = 0;
	tmp.bmpbits = malloc(imgsz); 
	tmp.height = hd[1];
	tmp.imgbytes = imgsz;
	//+1:make room in back for 32bit cast below
	tmp.palette = hd[2]==8?malloc(256*4+1):0;
	tmp.palnum = hd[2]==8?256:0;
	tmp.pixdepth = hd[2];
	tmp.rowbytes = rowsz;
	tmp.rowptr = malloc(sizeof(void*)*hd[1]);
	int i; for(i=0;i<hd[1];i++) tmp.rowptr[i] = tmp.bmpbits+rowsz*i;
	memset(&tmp.sigbit,8,sizeof(tmp.sigbit));
	tmp.topdown = FALSE;
	tmp.width = hd[0];
	
	if(tmp.pixdepth==8)
	{
		int32_t buff[256];
		if(fread(buff,256*4,1,f)) for(i=0;i<256;i++)
		{
			*(int32_t*)(tmp.palette+i) = 
			buff[i]<<16&0xFF0000|buff[i]&0xFF00|buff[i]>>16;
		}
	}

	rd = fread(tmp.bmpbits,imgsz,1,f);		

	if(rd&&jsomap_colorkey(&tmp))
	{
		IMAGE *cp = malloc(sizeof(tmp));
				
		if(cp) //always true??
		{
			memcpy(cp,&tmp,sizeof(tmp));
			out = (jsomap_t*)&cp->alpha;						
		}
	}
	else imgbuf_free(&tmp); 

	return out;
}

typedef struct
{
	uint16_t r:5,g:5,b:5,_:1;

}jsomap_555_t;

extern jsomap_t *jsomap_tim(int fd, int seek)
{	
	jsomap_t *out = &jsomap_bad;

	if(!jsomap_init()) return out;
			
	FILE *f = fdopen(fd,"rb"); if(!f) return out;

	if(seek) fseek(f,seek,SEEK_SET);

	int16_t hd[4]; //header
	size_t rd = fread(hd,8,1,f);

	if(!rd||hd[0]!=0x00000010) return out;

	int pmode = hd[2]&0x7, cf = hd[2]>>3&1;

	if(pmode>(cf?1:2)) return out; //supporting 0,1,2

	unsigned char *img=0; jsomap_555_t pal[256]; 
	
	int palsz=0, imgsz=0, rowsz=0;

	int32_t bnum; int16_t xywh[4];
		
	int i,j,k;
	for(i=0;i<(cf?2:1);i++)
	{
		enum{ buff_s = 4096 };

		rd = fread(&bnum,4,1,f)+fread(xywh,8,1,f);

		if(rd!=2||12+xywh[2]*2*xywh[3]>bnum) return out;

		int n = xywh[2]*xywh[3], bneed = bnum-12; 
				
		if(cf&&i==0) //palette
		{
			if(bneed>256*2) return out;

			rd = fread(pal,bneed,1,f); if(!rd) return out;

			palsz = bneed/2; continue;
		}

		int pixsz = (pmode==2?3:1);
		int rowpixs = xywh[2]*(pmode?3-pmode:4);
				
		//minima
		if(rowpixs<1||xywh[3]<1) return out;

		//maxima (arbitrary)
		if(rowpixs>jsomap_max||xywh[3]>jsomap_max) return out;
				
		rowsz = rowpixs*pixsz; 
		
		char *p = img = malloc(imgsz=rowsz*xywh[3]);

		uint16_t buff[buff_s]; //int16_t //>> can be sign extended.
		
		if(p) for(j=0;j<n;j+=rd)
		{
			//MSVC2015 can't include <algorithm> today????
			//std::min(buff_s,n-j)
			size_t buff_n = n-j; if(buff_n>buff_s) buff_n = buff_s;

			rd = fread(buff,2,buff_n,f);
						 						
			switch(rd?pmode:-1)
			{
			case 0: for(k=0;k<rd;k++)
					{
						*p++ = buff[k]&0xF; *p++ = buff[k]>>4&0XF;
						*p++ = buff[k]>>8&0xF; *p++ = buff[k]>>12;
					}
					break;

			case 1: memcpy(p,buff,rd*2); p+=rd*2; break;

			case 2: for(k=0;k<rd;k++)
					{
						jsomap_555_t *q = (jsomap_555_t*)buff+k;

						//bmp2png is hardwired for BGR when not paletted??
						*p++ = q->b*8+7; *p++ = q->g*8+7; *p++ = q->r*8+7; 
					}
					break;

			default: free(img); return out;
			}
		}
	}

	if(!img||!imgsz||!rowsz) return out; //sanity checks

	IMAGE tmp;
	tmp.alpha = 0;
	tmp.bmpbits = img; 
	tmp.height = xywh[3];
	tmp.imgbytes = imgsz;
	tmp.palette = palsz?malloc(palsz*4):0;
	tmp.palnum = palsz;
	tmp.pixdepth = pmode==2?24:8;
	tmp.rowbytes = rowsz;
	tmp.rowptr = malloc(sizeof(void*)*xywh[3]);
	for(i=0;i<xywh[3];i++) tmp.rowptr[i] = tmp.bmpbits+rowsz*i;
	memset(&tmp.sigbit,8,sizeof(tmp.sigbit));
	tmp.topdown = FALSE;
	tmp.width = xywh[2]*(pmode?3-pmode:4);
	
	if(palsz)
	for(i=0;i<palsz;i++)
	{
		tmp.palette[i].red = pal[i].r*8+7;
		tmp.palette[i].green = pal[i].g*8+7;
		tmp.palette[i].blue = pal[i].b*8+7;		
	}

	if(rd&&jsomap_colorkey(&tmp))
	{
		IMAGE *cp = malloc(sizeof(tmp));
				
		if(cp) //always true??
		{
			memcpy(cp,&tmp,sizeof(tmp));
			out = (jsomap_t*)&cp->alpha;						
		}
	}
	else imgbuf_free(&tmp); 

	return out;
}

extern int jsomap_free(jsomap_t *in, const char *png)
{
	if(in==&jsomap_bad||!jsomap_off||!in) return 1;
	
	IMAGE *img = (IMAGE*)((char*)in-jsomap_off); img->alpha = FALSE;
	
	int out = 0; if(!png) imgbuf_free(img);

	//make pure black transparent
	trans_type = B2P_TRANSPARENT_RGB; //bmp2png.c.inl
	trans_values.red = trans_values.green = trans_values.blue = 0;

	quietmode = 1; //bmp2png.c.inl
	if(png&&jsomap_powertwo(img)) 
	out = write_png((char*)png,img)?0:1; //char: warning

	free(img); return out;
}

extern BOOL jsomap_colorkey(IMAGE *in)
{
	//crop row/imgbytes to final pixel for jsomap_t interface.

	switch(in->pixdepth)
	{
	case 1: case 4:

		if(in->rowbytes>=in->width)
		{
			//unimplemented (falling thru)
		}
		//else 
		{
			fprintf(stderr,"Unable to increase color depth: %d.\n",in->pixdepth);

			return FALSE;
		}

	//hack: seems safe enough bmp2png wise
	case 8: in->rowbytes = in->width; break;
	case 24: in->rowbytes = 3*in->width; break;

	case 16: case 32: //assuming should not be

	default: 

		fprintf(stderr,"Unexpected color depth: %d.\n",in->pixdepth);

		return FALSE;
	}
		
	//hack: seems safe enough bmp2png wise
	in->imgbytes = in->rowbytes*in->height; 
	
	size_t row = 0, i;
	size_t rows = in->imgbytes/in->rowbytes;

	//saturating Sword of Moonlight's 5bit black colorkey

	if(in->palette)
	{
		//colorkey
		PALETTE *p = in->palette; 
		for(i=0;i<in->palnum;i++,p++) 
		{
			if(p->red<8&&p->green<8&&p->blue<8) 
			{
				p->red = p->green = p->blue = 0; 
			}				
		}
		
		//per index alpha
		PALETTE *pal =in->palette;
		for(row=0;row<rows;row++)
		{			
			unsigned char *p = in->rowptr[row];

			for(i=0;i<in->rowbytes;i++,p++)
			{
				if((*(uint32_t*)(pal+*p)&0xFFFFFF)==0)
				{
					return in->alpha = TRUE;
				}
			}
		}

		return TRUE;
	}

	for(row=0;row<rows;row++)
	{			
		unsigned char *p = in->rowptr[row];

		for(i=0;i<in->rowbytes;i+=3,p+=3)
		{
			if(p[0]<8&&p[1]<8&&p[2]<8) 
			{
				p[0] = p[1] = p[2] = 0;	in->alpha = TRUE;
			}
		}
	}

	return TRUE;
}

static BOOL jsomap_powertwo(IMAGE *in)
{
	int i, w, h;
	for(w=0;w<sizeof(jsomap_pow2);w++) if(in->width<=jsomap_pow2[w]) break;
	for(h=0;h<sizeof(jsomap_pow2);h++) if(in->height<=jsomap_pow2[h]) break;
	
	int trueheight = in->height;
	if(jsomap_pow2[h]!=in->height)
	{
		free(in->rowptr);

		in->rowptr = malloc(sizeof(void*)*jsomap_pow2[h]);

		for(i=0;i<in->height;i++) 
		in->rowptr[i] = in->bmpbits+in->rowbytes*i;

		for(in->height=jsomap_pow2[h];i<in->height;i++)
		in->rowptr[i] = jsomap_pad;
	}
	
	if(jsomap_pow2[w]!=in->width)
	{
		memcpy(jsomap_tmp,in->rowptr[trueheight],in->rowbytes);

		in->rowbytes = in->rowbytes/in->width*jsomap_pow2[w];  
		
		in->rowptr[trueheight] = jsomap_tmp;

		in->width = jsomap_pow2[w];
	}

	return TRUE;
}
