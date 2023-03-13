
#include "Ex.h" 
EX_TRANSLATION_UNIT

#include <vector> //REMOVE ME

#include "dx.ddraw.h"

#include "Ex.mipmap.h"

extern BYTE Ex_mipmap_sRGB(DWORD);

static void *Ex_mipmap(const DX::DDSURFACEDESC2*,DX::DDSURFACEDESC2*);

template <typename T, int N>
static void *Ex_mipmap_boxfilter
(const void *m, int mpitch, int mwidth, int mheight,
       void *n, int npitch, int nwidth, int nheight)
{
	switch(N)
	{
	case 888: //EXPERIMENTAL (2020)

	case 8888: if(sizeof(T)==4) break;
		
		assert(0); return 0;
	
	case 565: case 1555: if(sizeof(T)==2) break;

	default: assert(0); return 0;
	}

	if(!m||!n||nwidth<=0&&nheight<=0
	||nwidth%2&&nwidth!=1||nheight%2&&nheight!=1
	||nwidth>mwidth/2+mwidth%2||nheight>mheight/2+mheight%2)
	{
		assert(0); return 0;
	}

	const int bytes = sizeof(T); 

	if(mpitch%4||mwidth*bytes>mpitch){ assert(0); return 0; }
	if(npitch%4||nwidth*bytes>npitch){ assert(0); return 0; }

	static const int mask565[4] = {0x0,0xf800,0x07e0,0x001f};
	static const int shift565[4] = {0,11,5,0};
	static const int mask1555[4] = {0x8000,0x7c00,0x03e0,0x001f};
	static const int shift1555[4] = {15,10,5,0};

	const int *mask = N==565?mask565:mask1555;
	const int *shift = N==565?shift565:shift1555;
	
	bool sRBG = DDRAW::sRGB&&!DDRAW::shader; //2020

	for(int row=0;row<nheight;row++)
	{
		BYTE *above = (BYTE*)m+mpitch*row*2;
		BYTE *below = (BYTE*)m+mpitch*row*2+mpitch;

		if(row*2+1==mheight) below = above; //clamp border

		BYTE *sample = (BYTE*)n+npitch*row; 

		for(int col=0;col<nwidth;col++)
		{
			BYTE *kernel[2] = 
			{
				above+col*2*bytes,
				below+col*2*bytes
			};	

			int border = col*2+1==mwidth?0:bytes; //clamp
			
			switch(N)
			{
			case 8888: //EXPERIMENTAL
			{
				float sum = 0;

				sum+=kernel[0][3];
				sum+=kernel[0][border+3];
				sum+=kernel[1][border+3];
				sum+=kernel[1][3];

				//sum/=3.3333f; //assert(sum>=0&&sum<256); //4.0f

				//try to give alpha cutouts more body in mipmaps so
				//grass doesn't materialize from thin air
				if(1)
				{
					sum*=0.3f; //assert(sum>=0&&sum<256); 

					sample[3] = min((int)sum,255); //0xFF&int(sum);
				}
				else
				{
					sum*=0.25f; sample[3] = 0xFF&int(sum);
				}

				goto alpha;
			}
			case 888:
			{
				sample[3] = 1; alpha: //necessary?

				//for(int i=0;i<bytes;i++) 
				for(int i=0;i<bytes-1;i++) if(!sRBG)
				{
					float sum = 0;

					sum+=kernel[0][i];
					sum+=kernel[0][border+i];
					sum+=kernel[1][border+i];
					sum+=kernel[1][i];
					
					sum*=0.25f; //sum/=4.0f;

					assert(sum>=0&&sum<256);

					sample[i] = 0xFF&int(sum);
				}
				else if(0&&EX::debug) //testing point filter
				{
					sample[i] = kernel[0][i];
				}
				else
				{
					union{ DWORD dw; BYTE bt[4]; };
					bt[0] = kernel[0][i];
					bt[1] = kernel[0][border+i];
					bt[2] = kernel[1][border+i];
					bt[3] = kernel[1][i];
					sample[i] = Ex_mipmap_sRGB(dw);
				}

				break;
			}
			case 565:
			case 1555: 
			{	
				*(WORD*)sample = 0;

				for(int i=0;i<4;i++) if(mask[i])
				{
					float sum = 0; 

					sum+=(mask[i]&*(WORD*)(kernel[0]))>>shift[i];
					sum+=(mask[i]&*(WORD*)(kernel[0]+border))>>shift[i];
					sum+=(mask[i]&*(WORD*)(kernel[1]+border))>>shift[i];
					sum+=(mask[i]&*(WORD*)(kernel[1]))>>shift[i];

					sum*=0.25f; //sum/=4.0f;
					
					assert((int(sum)&mask[i]>>shift[i])==int(sum));

					*(WORD*)sample|=0xFFFF&int(sum)<<shift[i]&mask[i];
				}

				break;
			}
			default: assert(0); return 0;
			}

			sample+=bytes;
		}
    }

	//return true;
	return Ex_mipmap; //2021: this is just easier for Ex_mipmap
}

extern void *Ex_mipmap_pointfilter(const DX::DDSURFACEDESC2 *in, DX::DDSURFACEDESC2 *out)
{	
	if(!in||!out) return false;
	
	int ww = in->dwWidth, hh = in->dwHeight;
	int w = out->dwWidth, h = out->dwHeight;

	if(ww!=w*2&&ww!=w&&w!=1) return 0;
	if(hh!=h*2&&hh!=h&&h!=1) return 0;

	void *src = in->lpSurface; int ppitch = in->lPitch;
	void *dst = out->lpSurface; int qitch = out->lPitch;

	int pitch = 2*ppitch;

	int bpp = in->ddpfPixelFormat.dwRGBBitCount;

	//UNFINISHED: ALPHA MASKS NEED SPECIAL LOGIC
	for(int i=0;i<h;i++)
	{
		//REMINDER: might want to be more "odd"
		//sometimes textures can develop stripes
		//when point filtering, this just mixes
		//things up so hopefully it's just noise

		int odd = i&1; //jittering a little bit

		int odd2 = odd?ppitch:0; //what if 1px?

		if(bpp==32)
		{
			DWORD *p = (DWORD*)((char*)src+pitch*i+odd2);
			DWORD *q = (DWORD*)((char*)dst+qitch*i);

			for(int j=0;j<w;j++){ *q++ = p[odd]; p+=2; }
		}
		else //16bit
		{
			WORD *p = (WORD*)((char*)src+pitch*i+odd2);
			WORD *q = (WORD*)((char*)dst+qitch*i);
			for(int j=0;j<w;j++){ *q++ = p[odd]; p+=2; }
		}				
	}

	return Ex_mipmap_pointfilter;
}

extern bool Ex_mipmap_point_filter = false;
static void *Ex_mipmap(const DX::DDSURFACEDESC2 *in, DX::DDSURFACEDESC2 *out)
{
	if(!in||!out) return 0;

	//2021: point filtering the mipmaps is slightly less blurry
	//and is stable unlike doing point-filtering in the sampler
	//TODO: MIGHT WANT TO TRY TO INCREASE THE RELATIVE CONTRAST
	if(Ex_mipmap_point_filter) return Ex_mipmap_pointfilter(in,out);

	switch(in->ddpfPixelFormat.dwGBitMask)
	{
	case 0x0000ff00: //X8/A8R8G8B8

		if(in->ddpfPixelFormat.dwRGBBitCount!=32)
		{
			assert(0); return 0;
		}

		//EXPERIMENTAL (2020)
		if(!in->ddpfPixelFormat.dwRGBAlphaBitMask)
		return Ex_mipmap_boxfilter<DWORD,888> //guint32
		(in->lpSurface,in->lPitch,in->dwWidth,in->dwHeight,
		out->lpSurface,out->lPitch,out->dwWidth,out->dwHeight);

		return Ex_mipmap_boxfilter<DWORD,8888> //guint32
		(in->lpSurface,in->lPitch,in->dwWidth,in->dwHeight,
		out->lpSurface,out->lPitch,out->dwWidth,out->dwHeight);

	case 0x07e0: //R5G6B5
		
		if(in->ddpfPixelFormat.dwRGBBitCount!=16)
		{
			assert(0); return 0;
		}

		return Ex_mipmap_boxfilter<WORD,565> //guint16
		(in->lpSurface,in->lPitch,in->dwWidth,in->dwHeight,
		out->lpSurface,out->lPitch,out->dwWidth,out->dwHeight);

	case 0x03e0: //X1/A1R5G5B5

		if(in->ddpfPixelFormat.dwRGBBitCount!=16)
		{
			assert(0); return 0;
		}
		
		return Ex_mipmap_boxfilter<WORD,1555> //guint16
		(in->lpSurface,in->lPitch,in->dwWidth,in->dwHeight,
		out->lpSurface,out->lPitch,out->dwWidth,out->dwHeight);

		assert(0); return 0; //unimplemented

	case 8: assert(0); return 0; //unimplemented

	default: assert(0);
	}	

	return 0;
}

extern int Ex_mipmap_colorkey = 0; //2021
extern int EX::colorkey(DX::DDSURFACEDESC2 *in, D3DFORMAT)
{	
	if(!in) return 0;
	
	int w = in->dwWidth, h = in->dwHeight;

	void *bits = in->lpSurface; int pitch = in->lPitch;

	int bpp = in->ddpfPixelFormat.dwRGBBitCount;

	int knockouts = 0; //tally colorkeyed texels

	//setup alpha channel for pitch black pixels
	for(int i=0;i<h;i++)
	{
		if(bpp==32)
		{
			DWORD *p = (DWORD*)((char*)bits+pitch*i);

			if(0==Ex_mipmap_colorkey) //2021
			{
				for(int j=0;j<w;j++)
				if((p[j]&~0xFF000000)==0) //yes, required
				{
					p[j] = 0x00000000;

					knockouts++;
				}
				else p[j]|=0xFF000000; //yes, required
			}
			else if(7==Ex_mipmap_colorkey)
			{
				for(int j=0;j<w;j++) if((p[j]&~0xFF070707)==0)
				{
					p[j] = 0x00000000; //p[j]&=0x00FFFFFF; 

					knockouts++;
				}
				else p[j]|=0xFF000000; //yes, required
			}
			else assert(0);
		}
		else if(bpp==16) //16bit
		{
			WORD *p = (WORD*)((char*)bits+pitch*i);

			for(int j=0;j<w;j++) if((p[j]&~0x8000)==0) 
			{
				p[j] = 0x0000; //p[j]&=~0x8000;

				knockouts++;
			}
			else p[j]|=0x8000; //yes, required
		}
		else assert(0);
	}

	return knockouts;
}

//UNUSED
//ultimately the kaiser filter didn't make a difference at all
//to my eye. what worked was the quasi-randomized point filter
//////////////////////////////////////////////////////////////
//http://number-none.com/product/Mipmapping, Part 1/index.html
//http://number-none.com/product/Mipmapping, Part 2/index.html
//https://github.com/dwilliamson/GDMagArchive/blob/master/dec01/blow/gdmag_mipmap_1/main.cpp
//2021: today I stumbled across the Wikipedia page for the "Mitchell" filter in this article
//the article doesn't seem to recommend it over the kaiser
//https://en.wikipedia.org/wiki/Mitchell–Netravali_filters

static float Ex_mipmap_bessel0(float x)
{
	float xh = x/2, sum = 1, pow = 1, ds = 1;
	//these type used to be double
	//can float be equal to 1E-16? (maybe just barely)
	//for(int k=1;ds>sum*(1E-16);k++) //epsilon_ratio
	for(int k=1;ds>sum*(1E-13);k++) //epsilon_ratio
	{
		pow*=xh/k; ds = pow*pow; sum+=ds;
	}
	return sum;
}
//this appears to be more correct
enum{ Ex_mipmap_kaiser_midstep=1 };
static void Ex_mipmap_fill_kaiser_filter(int width, float *filter_data, float stretch, float alpha=4) 
{
	//int width = filter->nsamples;
	assert(~width&1);

	static const float bessel0_alpha = Ex_mipmap_bessel0(alpha);

	float half_width = width/2;
	float offset = -half_width;
	float nudge = Ex_mipmap_kaiser_midstep?0.000001f:0.5f; //just don't zero-divide

	float sum = 0;
	for(int i=0;i<width;i++)
	{
		//float x = (i+offset)+nudge;
		float x = (i+offset+nudge)*stretch;

		float sinc_value = //sinc(x*stretch);
		//{			
			//x?sin(M_PI*x)/(M_PI*x):1;
			sin(M_PI*x)/(M_PI*x); assert(x!=0.0f);
		//}
		float window_value = //kaiser(alpha,half_width,x*stretch);
		//{
			Ex_mipmap_bessel0(sqrt(1-pow(x/half_width,2))*alpha)/bessel0_alpha;
		//}

		sum += filter_data[i] = sinc_value*window_value;
	}
	//filter->normalize();
	{
		assert(sum!=0); sum = 1/sum;

		for(int i=0;i<width;i++) filter_data[i]*=sum;
	}
}

//UNUSED
//this was an experiment to try to produce less blurry mipmaps
//ultimately I could see a difference and went with randomized
//point filter to preserve color for use with pixel art styles
static std::vector<float> Ex_mipmap_buffer;
extern int Ex_mipmap_kaiser_sinc_levels = 0;
extern void *Ex_mipmap2(const DX::DDSURFACEDESC2 *in, DX::DDSURFACEDESC2 *out)
{
	if(!in||!out)
	{
		Ex_mipmap_buffer.swap(std::vector<float>());
		return 0; 
	}

	if((int)out->dwDepth>Ex_mipmap_kaiser_sinc_levels)
	{
		if(0&&EX::debug)
		{
			memset(out->lpSurface,0xff,out->dwHeight*out->lPitch);
			return Ex_mipmap;
		}
		return Ex_mipmap(in,out);
	}

	//the filter_width size below is prohibitive for large depths
	//I'm not sure it's not a mistake so I've written its author
	if(0&&out->dwDepth>3||in->dwHeight==1||in->dwWidth==1) 
	{
		if(0&&EX::debug)
		{
			memset(out->lpSurface,0xff,out->dwHeight*out->lPitch);
			return Ex_mipmap;
		}
		return Ex_mipmap(in,out);
	}

	//NOTE: sRGB seems to be assumed below. it's up to the client
	//to manage this prior to CreateDevice and DirectDrawCreateEx
	//will turn it off since shaders are needed to implement sRGB
	assert(DDRAW::sRGB);

	if(in->dwWidth!=out->dwWidth*2
	 ||in->dwHeight!=out->dwHeight*2
	 ||in->ddpfPixelFormat.dwRGBBitCount!=32)
	{
		assert(0); return Ex_mipmap(in,out);
	}

	int w = in->dwWidth<<in->dwDepth;
	int h = in->dwHeight<<in->dwDepth;

	if(!in->dwDepth) //2021: top-level?
	{
		Ex_mipmap_buffer.clear();
		Ex_mipmap_buffer.resize(w*h*4); //YUCK: zero-initialize (C++)

		//TODO: probably don't need float here
		extern const WORD Ex_mipmap_s8l16[256];

		float l_ffff = 1/65535.0f;
		BYTE *p = (BYTE*)in->lpSurface;
		float *data = &Ex_mipmap_buffer[0];
		for(int i=h,s=in->lPitch;i-->0;p+=s)
		for(int j=0,n=4*w;j<n;j++)
		*data++ = Ex_mipmap_s8l16[p[j]]*l_ffff;
	}
	else assert(w*h*4==Ex_mipmap_buffer.size());

	size_t sz = Ex_mipmap_buffer.size();

	float *filter; int window;
	//for(int i=0;i<NUM_WIDE_FILTERS;i++)
	{
		int i = out->dwDepth-1;

		//values slightly below or above this seem to experience
		//problems on texture borders 
		enum{ GOOD_FILTER_WIDTH=14 };

		//WARNING: this balloons to 1792 at 1x1 as near as I can
		//see. I've written the author for a response. for now I
		//think the lowest levels can just use a box filter? see
		//the check at the top (also see below)
		int levels_to_reduce = i+1;
		int filter_width = GOOD_FILTER_WIDTH<<levels_to_reduce-1;
		if(GOOD_FILTER_WIDTH&1)
		{
			if(~filter_width&1) filter_width++;
		}
		if(1) //GOOD_FILTER_WIDTH<<levels_to_reduce-1?
		{
			filter_width = min(max(w,h)*2-2,filter_width);
		}
		if(!GOOD_FILTER_WIDTH||filter_width>=w*2||filter_width>=h*2)
		{
			//HACK: I have the same concern here, but this could
			//pretty easily happen for small textures... I don't
			//want to have to use modulus or loops to wrap below
			return Ex_mipmap(in,out);
		}

		Ex_mipmap_buffer.resize(sz+filter_width);

		filter = &Ex_mipmap_buffer[sz];
		window = filter_width; 

		//I don't know if this is better or not, to be honest it's 
		//hard to tell definitively... it's easier to tell when a
		//change makes things worse and do the opposite
		if(1)
		{	
			//in theory I think I'd like this to devolve into a point
			//filter, but I'm not sure how to do it. GOOD_FILTER_WIDTH
			//can't really be used and I think it would demand higher
			//values to be more like a point filter, but I'm not sure
			float extra_scale = powf(0.66666f,powf(levels_to_reduce,0.33333f));
			Ex_mipmap_fill_kaiser_filter(filter_width,filter,extra_scale,3+powf(levels_to_reduce,4));
		}
		else //recommended setting (looks like box filter)
		{
			//NOTE: border artifacts are appearing if GOOD_FILTER_WIDTH
			//varies far from 14

			float extra_scale = pow(0.5,levels_to_reduce);
			Ex_mipmap_fill_kaiser_filter(filter_width,filter,extra_scale);
		}
	}

	int step = 1<<out->dwDepth;
	int ww = w/step, hh = h/step;

	//DISABLED
	//this is to sample in the middle of the band, not sure if it
	//makes a difference, but you know, it seems to make sense to
	//keep to the center
	//
	// Notes: when using this single pixels have less ghosting if
	// using mipmaps_pixel_art_power_of_two and so they look more
	// clear. other things seem worse however and the fountain in
	// the lobby is ghostly
	//
	int midstep = Ex_mipmap_kaiser_midstep?step/2:0;

	//NOTE: want zero-intialization for tmp
	Ex_mipmap_buffer.resize(sz+window+ww*h*4); 
	filter = &Ex_mipmap_buffer[sz];
	float *tmpp = &Ex_mipmap_buffer[sz+window];
	float *tmp = tmpp;
	float *oob = &Ex_mipmap_buffer.back()+1;

	assert(~window&1);
	int filter_offset = -(window/2-!Ex_mipmap_kaiser_midstep);

	float *data = &Ex_mipmap_buffer[0];
	for(int j=0;j<h;j++,data+=4*w)
	{
		for(int i=midstep;i<w;i+=step,tmp+=4)
		{
			for(int k=0;k<window;k++)
			{
				int src_i = i+k+filter_offset;
				//int src_i = get_index_with_reflecting(src_i,j);
				//{
					//NOTE: not more than once
						#if 0 //clamp?
					if(src_i<0) src_i = -src_i;
					else 
					if(src_i>=w) src_i = w+w-src_i-1; //-1?
						#else //wrap?
					if(src_i<0) src_i+=w;
					else 
					if(src_i>=w) src_i-=w;
						#endif
				//}
				float fk = filter[k];
				src_i*=4; 
				assert(data+src_i+4<=filter);
				for(int l=0;l<4;l++) 
				tmp[l]+=fk*data[src_i+l];
			}
		}
	}

	int s = out->lPitch;
	BYTE *pp = (BYTE*)out->lpSurface;
	BYTE *oopp = pp+s*hh;
	//for(int ww4=ww*4,i=0;i<ww;i++)
	for(int ww4=ww*4,i=0;i<ww4;i+=4)
	{
		tmp = tmpp+i;
		BYTE *p = pp+i;
		for(int j=midstep;j<h;j+=step,p+=s)
		{	
			float sum[4] = {};
			for(int k=0;k<window;k++)
			{
				int src_j = j+k+filter_offset;
				//int src_j = tmp->get_index_with_reflecting(i/4,src_j);
				//{
					//NOTE: not more than once
						#if 0 //clamp?
					if(src_j<0) src_j = -src_j;
					else 
					if(src_j>=h) src_j = h+h-src_j-1; //-1?
						#else //wrap?
					if(src_j<0) src_j+=h;
					else 
					if(src_j>=h) src_j-=h;
						#endif
				//}				
				float fk = filter[k];
				src_j*=ww4; 
				assert(tmp+src_j+4<=oob);
				for(int l=0;l<4;l++) 
				sum[l]+=fk*tmp[src_j+l];
			}
			assert(p+4<=oopp);
			extern BYTE Ex_mipmap_LinearToSRGB8(WORD);
			for(int l=0;l<4;l++)
			{
				//texture_from_image_buffer clamps these
				float clamp = sum[l]*65535;
				clamp = min(65535,max(0,clamp));
				p[l] = Ex_mipmap_LinearToSRGB8((WORD)clamp);
			}

		}
	}

	Ex_mipmap_buffer.resize(sz); return Ex_mipmap;
}

extern void *EX::mipmap(const DX::DDSURFACEDESC2 *in, DX::DDSURFACEDESC2 *out)
{
	return (Ex_mipmap_kaiser_sinc_levels?Ex_mipmap2:Ex_mipmap)(in,out);
}

/////////////////////////////////////////////////////////////////////////////
//
//	SOURCE: https://github.com/ncruces/go-image/blob/master/imageutil/srgb.go
//
/*MIT License

Copyright (c) 2019 Nuno Cruces

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/
extern const WORD Ex_mipmap_s8l16[256] =
{
	0x0000, 0x0014, 0x0028, 0x003c, 0x0050, 0x0063, 0x0077, 0x008b,
	0x009f, 0x00b3, 0x00c7, 0x00db, 0x00f1, 0x0108, 0x0120, 0x0139,
	0x0154, 0x016f, 0x018c, 0x01ab, 0x01ca, 0x01eb, 0x020e, 0x0232,
	0x0257, 0x027d, 0x02a5, 0x02ce, 0x02f9, 0x0325, 0x0353, 0x0382,
	0x03b3, 0x03e5, 0x0418, 0x044d, 0x0484, 0x04bc, 0x04f6, 0x0532,
	0x056f, 0x05ad, 0x05ed, 0x062f, 0x0673, 0x06b8, 0x06fe, 0x0747,
	0x0791, 0x07dd, 0x082a, 0x087a, 0x08ca, 0x091d, 0x0972, 0x09c8,
	0x0a20, 0x0a79, 0x0ad5, 0x0b32, 0x0b91, 0x0bf2, 0x0c55, 0x0cba,
	0x0d20, 0x0d88, 0x0df2, 0x0e5e, 0x0ecc, 0x0f3c, 0x0fae, 0x1021,
	0x1097, 0x110e, 0x1188, 0x1203, 0x1280, 0x1300, 0x1381, 0x1404,
	0x1489, 0x1510, 0x159a, 0x1625, 0x16b2, 0x1741, 0x17d3, 0x1866,
	0x18fb, 0x1993, 0x1a2c, 0x1ac8, 0x1b66, 0x1c06, 0x1ca7, 0x1d4c,
	0x1df2, 0x1e9a, 0x1f44, 0x1ff1, 0x20a0, 0x2150, 0x2204, 0x22b9,
	0x2370, 0x242a, 0x24e5, 0x25a3, 0x2664, 0x2726, 0x27eb, 0x28b1,
	0x297b, 0x2a46, 0x2b14, 0x2be3, 0x2cb6, 0x2d8a, 0x2e61, 0x2f3a,
	0x3015, 0x30f2, 0x31d2, 0x32b4, 0x3399, 0x3480, 0x3569, 0x3655,
	0x3742, 0x3833, 0x3925, 0x3a1a, 0x3b12, 0x3c0b, 0x3d07, 0x3e06,
	0x3f07, 0x400a, 0x4110, 0x4218, 0x4323, 0x4430, 0x453f, 0x4651,
	0x4765, 0x487c, 0x4995, 0x4ab1, 0x4bcf, 0x4cf0, 0x4e13, 0x4f39,
	0x5061, 0x518c, 0x52b9, 0x53e9, 0x551b, 0x5650, 0x5787, 0x58c1,
	0x59fe, 0x5b3d, 0x5c7e, 0x5dc2, 0x5f09, 0x6052, 0x619e, 0x62ed,
	0x643e, 0x6591, 0x66e8, 0x6840, 0x699c, 0x6afa, 0x6c5b, 0x6dbe,
	0x6f24, 0x708d, 0x71f8, 0x7366, 0x74d7, 0x764a, 0x77c0, 0x7939,
	0x7ab4, 0x7c32, 0x7db3, 0x7f37, 0x80bd, 0x8246, 0x83d1, 0x855f,
	0x86f0, 0x8884, 0x8a1b, 0x8bb4, 0x8d50, 0x8eef, 0x9090, 0x9235,
	0x93dc, 0x9586, 0x9732, 0x98e2, 0x9a94, 0x9c49, 0x9e01, 0x9fbb,
	0xa179, 0xa339, 0xa4fc, 0xa6c2, 0xa88b, 0xaa56, 0xac25, 0xadf6,
	0xafca, 0xb1a1, 0xb37b, 0xb557, 0xb737, 0xb919, 0xbaff, 0xbce7,
	0xbed2, 0xc0c0, 0xc2b1, 0xc4a5, 0xc69c, 0xc895, 0xca92, 0xcc91,
	0xce94, 0xd099, 0xd2a1, 0xd4ad, 0xd6bb, 0xd8cc, 0xdae0, 0xdcf7,
	0xdf11, 0xe12e, 0xe34e, 0xe571, 0xe797, 0xe9c0, 0xebec, 0xee1b,
	0xf04d, 0xf282, 0xf4ba, 0xf6f5, 0xf933, 0xfb74, 0xfdb8, 0xffff,
};
// 8-bit linear to 16-bit sRGB LUT (see srgb_gen.py)
static const WORD Ex_mipmap_l8s16[256] = 
{
	0x0000, 0x0cfc, 0x15f9, 0x1c6b, 0x21ce, 0x2671, 0x2a93, 0x2e53,
	0x31c6, 0x34fb, 0x37fd, 0x3ad3, 0x3d83, 0x4013, 0x4286, 0x44e0,
	0x4722, 0x4950, 0x4b6b, 0x4d75, 0x4f6f, 0x515b, 0x5339, 0x550a,
	0x56d0, 0x588b, 0x5a3c, 0x5be3, 0x5d82, 0x5f17, 0x60a5, 0x622b,
	0x63a9, 0x6521, 0x6692, 0x67fd, 0x6962, 0x6ac1, 0x6c1a, 0x6d6f,
	0x6ebe, 0x7008, 0x714e, 0x7290, 0x73cc, 0x7505, 0x763a, 0x776b,
	0x7898, 0x79c1, 0x7ae7, 0x7c0a, 0x7d29, 0x7e45, 0x7f5e, 0x8074,
	0x8187, 0x8297, 0x83a4, 0x84af, 0x85b7, 0x86bd, 0x87c0, 0x88c0,
	0x89be, 0x8aba, 0x8bb4, 0x8cab, 0x8da1, 0x8e94, 0x8f85, 0x9074,
	0x9161, 0x924d, 0x9336, 0x941e, 0x9503, 0x95e7, 0x96ca, 0x97aa,
	0x9889, 0x9967, 0x9a42, 0x9b1d, 0x9bf5, 0x9ccc, 0x9da2, 0x9e76,
	0x9f49, 0xa01b, 0xa0eb, 0xa1b9, 0xa287, 0xa353, 0xa41e, 0xa4e7,
	0xa5b0, 0xa677, 0xa73d, 0xa802, 0xa8c5, 0xa988, 0xaa49, 0xab09,
	0xabc8, 0xac87, 0xad44, 0xae00, 0xaebb, 0xaf75, 0xb02d, 0xb0e5,
	0xb19d, 0xb253, 0xb308, 0xb3bc, 0xb46f, 0xb522, 0xb5d3, 0xb684,
	0xb734, 0xb7e3, 0xb891, 0xb93e, 0xb9ea, 0xba96, 0xbb41, 0xbbeb,
	0xbc94, 0xbd3d, 0xbde4, 0xbe8b, 0xbf32, 0xbfd7, 0xc07c, 0xc120,
	0xc1c3, 0xc266, 0xc308, 0xc3a9, 0xc44a, 0xc4ea, 0xc589, 0xc628,
	0xc6c6, 0xc763, 0xc800, 0xc89c, 0xc937, 0xc9d2, 0xca6d, 0xcb06,
	0xcb9f, 0xcc38, 0xccd0, 0xcd67, 0xcdfe, 0xce94, 0xcf2a, 0xcfbf,
	0xd053, 0xd0e7, 0xd17b, 0xd20e, 0xd2a0, 0xd332, 0xd3c3, 0xd454,
	0xd4e5, 0xd574, 0xd604, 0xd693, 0xd721, 0xd7af, 0xd83c, 0xd8c9,
	0xd956, 0xd9e2, 0xda6d, 0xdaf8, 0xdb83, 0xdc0d, 0xdc97, 0xdd20,
	0xdda9, 0xde32, 0xdeba, 0xdf41, 0xdfc8, 0xe04f, 0xe0d6, 0xe15b,
	0xe1e1, 0xe266, 0xe2eb, 0xe36f, 0xe3f3, 0xe477, 0xe4fa, 0xe57c,
	0xe5ff, 0xe681, 0xe702, 0xe784, 0xe804, 0xe885, 0xe905, 0xe985,
	0xea04, 0xea83, 0xeb02, 0xeb80, 0xebfe, 0xec7c, 0xecf9, 0xed76,
	0xedf3, 0xee6f, 0xeeeb, 0xef67, 0xefe2, 0xf05d, 0xf0d8, 0xf152,
	0xf1cc, 0xf246, 0xf2bf, 0xf338, 0xf3b1, 0xf429, 0xf4a1, 0xf519,
	0xf591, 0xf608, 0xf67f, 0xf6f6, 0xf76c, 0xf7e2, 0xf858, 0xf8cd,
	0xf942, 0xf9b7, 0xfa2c, 0xfaa0, 0xfb14, 0xfb88, 0xfbfc, 0xfc6f,
	0xfce2, 0xfd54, 0xfdc7, 0xfe39, 0xfeab, 0xff1d, 0xff8e, 0xffff,
};
/* Fast 8-bit sRGB to 16-bit linear conversion.
// Returns the correctly rounded result.
extern WORD Ex_mipmap_SRGB8ToLinear(BYTE srgb)
{
	return Ex_mipmap_s8l16[srgb]; // direct lookup
}*/
// Fast 16-bit linear to 8-bit sRGB conversion.
// Returns the correctly rounded result for 99.8% of inputs,
// error within -1 and +1.
extern BYTE Ex_mipmap_LinearToSRGB8(WORD lin)
{
	// piecewise linear
	//div, mod := divmod257(uint32(lin))
	DWORD div,mod;
	{
		// valid for x=[0..256*65535[
		QWORD mul = QWORD(lin)*0xff0100; 
		//truncating!
		//mod = mul*257>>32;
		mod = QWORD((DWORD)mul)*257>>32;
		div = mul>>32;
	}
	DWORD l0 = Ex_mipmap_l8s16[(BYTE)(div)];
	DWORD l1 = Ex_mipmap_l8s16[(BYTE)(div+1)];
	DWORD li = 257*l0+mod*(l1-l0);
	//return uint8(divsqr257rnd(li))
	{
		// valid for x=[0..257*65535[
		QWORD mul = QWORD(li+0x8100)*0x1fc05f9;
		div = mul>>41;
		return (BYTE)div;
	}
}

extern BYTE Ex_mipmap_sRGB(DWORD b4)
{
	DWORD sum = 0;
	for(int i=4;i-->0;b4>>=8)
	sum+=Ex_mipmap_s8l16[b4&0xFF];
	sum/=4;
	return Ex_mipmap_LinearToSRGB8((WORD)sum);
}
extern BYTE Ex_mipmap_sRGB(DWORD b4, float w)
{
	DWORD sum = 0;
	for(int i=4;i-->0;b4>>=8)
	sum+=Ex_mipmap_s8l16[b4&0xFF];
	sum*=w;
	return Ex_mipmap_LinearToSRGB8((WORD)sum);
}