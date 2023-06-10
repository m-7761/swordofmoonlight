
#include "x2mdl.pch.h" //PCH

#include <vector> //REMOVE ME

#include <d3dx9shader.h>

//Added for snapshot transform
#include "../lib/swordofmoonlight.h"

/*UNUSED this is a failed experiment
static void x2mdl_sharpen(DWORD *pp, DWORD *qq, int sq)
{
	float sharpness = 0.25f;
	float sharpness_4 = sharpness*4;
	float l_9 = 1/9.0f;
	for(int sq_1=sq-1,row=0;row<sq;row++)
	{
		int s = row*sq;

		BYTE *p = (BYTE*)(pp+s), *above = row?(BYTE*)(p-s):p; 
		BYTE *q = (BYTE*)(qq+s), *below = row==sq_1?p:(BYTE*)(p+s);

		//TODO? Ex.mipmaps.cpp has Ex_mipmap_s8l16/Ex_mipmap_LinearToSRGB8 
		for(int col=0;col<sq;col++,p+=4,q+=4,above+=4,below+=4)
		{
			BYTE *k[9] = 
			{
				above-4,above,above+4,
				p-4,p,p+4,
				below-4,below,below+4,
			};

			if(row==0) k[0] = k[1] = k[2] = p;
			if(row==sq_1) k[6] = k[7] = k[8] = p;
			if(col==0) k[0] = k[3] = k[6] = p;
			if(col==sq_1) k[2] = k[5] = k[8] = p;
			
			int sum[3] = {};

			if(0)
			{
				//wrong: maybe matrix multiply? (too much)
				for(int i=9;i-->0;)
				for(int j=3;j-->0;) sum[j]-=k[i][j]; //-1
				for(int j=3;j-->0;) sum[j]+=p[j]*(17+1); //+1: undo middle
				for(int j=3;j-->0;) 
				q[j] = sum[j]<=0?0:sum[j]>=255?255:(BYTE)(sum[j]*l_9);
			}
			else
			{			
				//wrong: just doesn't make sense (255==1.0)
				for(int j=3;j-->0;)
				sum[j] = (int)(sharpness_4*p[j]+255+sharpness*(-k[1][j]-k[3][j]-k[5][j]-k[7][j]));
				for(int j=3;j-->0;) 
				q[j] = sum[j]<=0?0:sum[j]>=255?255:(BYTE)(sum[j]);
			}			
		}
	}
}*/

#define HLSL2(x) #x
#define HLSL(...) HLSL2(__VA_ARGS__) "\n"
#define HLSL_DEFINE(x,...)\
"#define " #x " " HLSL2(__VA_ARGS__) "\n"

static const char x2ico_vs[] = //HLSL
{	
	HLSL(
	float4x4 mvp:register(vs,c0);
	struct VS_INPUT
	{
		float4 pos : POSITION;
		float3 lit : NORMAL;
		float2 uv0 : TEXCOORD0;
	};
	struct VS_OUTPUT
	{
		float4 pos : POSITION;
		float4 col : COLOR0;
	};
	VS_OUTPUT f(VS_INPUT In)
	{
		VS_OUTPUT Out; 
	
	//	float lit = max(0.0,dot(In.lit,-float3(0.0,1.0,0.0)));
		float lit = 1?1.0:-In.lit.y; //same
		if(0) lit = 0.7+0.3*lit; //ambient
	//	if(1) lit*=1.0-clamp(In.pos.y*-0.3,0.0,0.7); //fog
		float brighten = 0.1; //ARBITRARY

		Out.pos = mul(mvp,float4(In.pos.xyz,1.0f));
		Out.col = float4(In.uv0,lit+brighten,In.pos.y);

		return Out; 
	}
	VS_OUTPUT g(VS_INPUT In)
	{
		VS_OUTPUT Out; 
	
	//	float lit = max(0.0,dot(In.lit,-float3(0.0,-1.0,0.0)));
		float lit = In.lit.y;

		Out.pos = mul(mvp,float4(In.pos.xyz,1.0));
		Out.col = float4(In.uv0,lit,In.pos.y); //alpha? //reserved

		return Out; 
	}
	VS_OUTPUT h(VS_INPUT In)
	{
		VS_OUTPUT Out; 
	
		Out.pos = mul(mvp,float4(In.pos.xyz,1.0));
		Out.col = float4(0,0,0,In.pos.w);

		return Out; 
	})
};
static const char x2ico_ps[] = //HLSL
{	
	HLSL(
	struct PS_INPUT
	{
		float4 col : COLOR0;
	};
	struct PS_OUTPUT
	{
		float4 col : COLOR0;
	};
	float ceiling:register(ps,c0);
	sampler2D sam0:register(s0);
	PS_OUTPUT f(PS_INPUT In)
	{
		PS_OUTPUT Out;

		Out.col = In.col.z*tex2D(sam0,In.col.xy);
		Out.col.a = 1.0;

		return Out; 
	}
	PS_OUTPUT g(PS_INPUT In)
	{
		PS_OUTPUT Out;

		float l = abs(In.col.z);
		float c = 0.15*clamp(ceiling-In.col.w,0,0.5)*(l-In.col.z);

	//	Out.col = In.col*tex2D(sam0,In.col.xy);
	//	Out.col = In.col.z;
		Out.col.rgb = 0.0;
		//TODO: may want to use alpha-blend with sort here
	//	Out.col.a = 0.5+0.5*l*l-ceiling;		
		
		//Out.col.a = max(0.5,l-c);
		float o = l-c;
		o = o*(o*(o*0.305306011+0.682171111)+0.012522878); //linear
		o = max(0.3,o*o);
		o = max(0,1.055*pow(o,0.416666667)-0.055); //sRGB		
		Out.col.a = o;

		return Out; 
	}
	PS_OUTPUT h(PS_INPUT In)
	{
		PS_OUTPUT Out;

		Out.col = In.col;

		return Out; 
	})
};

static IDirect3DPixelShader9 *ps[3] = {};
static IDirect3DVertexShader9 *vs[3] = {};
extern PDIRECT3DSURFACE9 rt = 0, rs = 0; 
extern std::vector<IDirect3DTexture9*> icotextures;
static bool x2mhm_ico(WCHAR *in, float margin, IDirect3DDevice9 *pd3Dd9)
{
	wmemcpy(PathFindExtensionW(in),L".mhm",4);
	FILE *f = _wfopen(in,L"rb");
	if(!f) return false;
	fseek(f,0,SEEK_END);
	std::vector<BYTE> buf(ftell(f));
	fseek(f,0,SEEK_SET);
	fread(buf.data(),buf.size(),1,f);
	fclose(f);

	BYTE *m = buf.data();
	
	namespace mhm = SWORDOFMOONLIGHT::mhm;		
	mhm::image_t img; mhm::maptorom(img,m,buf.size()); 
	if(!img) bad:
	{
		assert(0); return false;
	}
	mhm::face_t *fp;	
	mhm::index_t *ip;
	mhm::vector_t *vp,*np;
	mhm::header_t &hd = mhm::imageheader(img);
	int npN = mhm::imagememory(img,&vp,&np,&fp,&ip);
	if(!npN&&!img) goto bad;

	for(auto i=hd.vertcount;i-->0;) 
	{
		vp[i][2] = -vp[i][2];
	}
	for(auto i=hd.normcount;i-->0;) 
	{
		np[i][2] = -np[i][2];
	}
	
	std::vector<float> lines; lines.reserve(4*3*2);
	auto *pi = ip;		
	float lmargin = (1+margin)/1;	
	float marginx = margin+margin*0.05f; //FUDGE
	float marginz = margin-margin*0.05f; //FUDGE
	float margin2 = margin+margin;
	float margin2x = margin2+margin*0.05f; //FUDGE
	float margin2z = margin2-margin*0.05f; //FUDGE
	float magic = 1.5f; //HACK: diagonals
	unsigned i,iN = hd.facecount;
	for(i=0;i<iN;i++)	
	{
		int ic = fp[i].ndexcount;

		auto *pp = ip; ip+=ic;

		if(1==fp[i].clipmode)
		{
			mhm::vector_t &n = np[fp[i].normal];

			int mx = 0, mz = 0;
			float mm[2] = {FLT_MAX,-FLT_MAX};
			for(auto *p=pp;p<ip;p++)
			{
				mhm::vector_t &v = vp[*p];
				if(fabsf(v[0])<0.001f) mx++;
				if(fabsf(v[2])<0.001f) mz++;

				mm[0] = std::min(mm[0],v[1]);
				mm[1] = std::max(mm[1],v[1]);
			}
			float h = mm[1]-mm[0];

			for(auto *p=pp;p<ip;p++)
			{
				mhm::vector_t &w = vp[*p];
				mhm::vector_t v = {w[0],w[1],w[2]};
				
				//mvp?
				{
					v[0]*=lmargin; v[2]*=lmargin;
				}
				if(1) //NEW WAY?
				{
					float nx = n[0]*margin2x*magic, nz = n[2]*margin2z*magic;

					v[0]+=h<=0.5f?-nx:nx; v[2]+=h<=0.5f?-nz:nz;
				}
				else if(mx==ic) //vertical through middle?
				{
					v[0]+=n[0]*margin2x; //margin2x
				}
				else if(mz==ic) //horizontal through middle?
				{
					v[2]+=n[2]*margin2z; //margin2z
				}
				else 
				{
					v[0]+=(v[0]<0?n[0]*margin2x:-n[0]*margin2x); //margin2x
					v[2]+=(v[2]<0?n[2]*margin2z:-n[2]*margin2z); //margin2z
				}
				//mvp?
				{ 
					v[0]-=marginx; v[2]-=marginz;
				}

				lines.insert(lines.end(),v,v+3);
				lines.push_back(0.5f);
			}
			
			for(int alpha=0;alpha<=1;alpha++)
			{
				if(alpha) for(size_t i=0;i<lines.size();i+=4)
				{					
					if(1) //NEW WAY?
					{
						float nx = n[0]*margin2x*magic, nz = n[2]*margin2z*magic;

						lines[i]+=h<=0.5f?-nx:nx; lines[i+2]+=h<=0.5f?-nz:nz;
					}
					else if(mx==ic) //vertical through middle?
					{
						lines[i]+=n[0]*margin2x*2; //FUDGE 
					}
					else if(mz==ic) //horizontal through middle?
					{
						lines[i+2]+=n[2]*margin2z*2; //FUDGE
					}
					else
					{
						lines[i]+=(lines[i]<0?n[0]*margin2x*2:-n[0]*margin2x*2);
						lines[i+2]+=(lines[i+2]<0?n[2]*margin2z*2:-n[2]*margin2z*2);
					}

					lines[i+3] = 0.75f;
				}

				pd3Dd9->SetFVF(D3DFVF_XYZW);
				pd3Dd9->DrawPrimitiveUP(D3DPT_LINESTRIP,lines.size()/4-1,lines.data(),4*sizeof(float));
			}

			lines.clear();
		}
	}

	return true;
}
extern bool x2msm_ico(int sq, WCHAR *in, IDirect3DDevice9 *pd3Dd9, IDirect3DSurface9 *rt, IDirect3DSurface9 *rs)
{
	assert(rt); if(!rt) return false;

	FILE *f = _wfopen(in,L"rb");
	if(!f) return false;
	fseek(f,0,SEEK_END);
	std::vector<BYTE> buf(ftell(f));
	fseek(f,0,SEEK_SET);
	fread(buf.data(),buf.size(),1,f);
	fclose(f);

	BYTE *m = buf.data();
	
	int i = *(short*)(m); //textures
	BYTE *pp = m+2; //textures names
	for(;i-->0;pp++) 
	while(*pp) pp++; //scanning past texture names

	int verts = *(WORD*)pp; pp+=2;

	void *pverts = pp; pp+=verts*8*sizeof(float); //polygon data
	
	float psc[4], &ceiling = psc[0];
	ceiling = -1000; //-FLT_MAX; //underflow?
	for(float*p=(float*)pverts;p<(float*)pp;p+=8)
	{
	//	p[1] = -p[1]; p[4] = -p[4];		
		p[2] = -p[2]; p[5] = -p[5]; //keep upright
		if(p[4]<-0.8)
		ceiling = std::max(ceiling,p[1]);
	}

	auto hr = pd3Dd9->SetRenderTarget(0,rt);
	pd3Dd9->Clear(0,0,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1.0f,0);
	if(!vs[0]) for(int i=3;i-->0;)
	{
		char main[2] = "f"; *main+=i;

		DWORD cflags =
		D3DXSHADER_AVOID_FLOW_CONTROL|D3DXSHADER_OPTIMIZATION_LEVEL3;
		LPD3DXBUFFER obj,err;
		if(!D3DXCompileShader(x2ico_vs,sizeof(x2ico_vs)-1,0,0,main,"vs_3_0",cflags,&obj,&err,0))
		{
			pd3Dd9->CreateVertexShader((DWORD*)obj->GetBufferPointer(),&vs[i]); obj->Release();
		}
		if(err)
		{
			char *e = (char*)err->GetBufferPointer(); 

			assert(!err); err->Release(); //breakpoint
		}
				
		if(!D3DXCompileShader(x2ico_ps,sizeof(x2ico_ps)-1,0,0,main,"ps_3_0",cflags,&obj,&err,0))
		{
			pd3Dd9->CreatePixelShader((DWORD*)obj->GetBufferPointer(),&ps[i]); obj->Release();
		}
		if(err)
		{
			char *e = (char*)err->GetBufferPointer(); 

			assert(!err); err->Release(); //breakpoint
		}
	}

//	pd3Dd9->CreateStateBlock(D3DSBT_ALL); //caller is responsible

	D3DXMATRIX mat,mvp;
	D3DXMatrixOrthoRH(&mat,2,2,-50,50);
	mvp = mat;
	const float M_Pi_2 = 1.57079632679489661923f; // pi/2
	D3DXMatrixRotationX(&mat,M_Pi_2);
	float margin = 0.5f/sq;
	mat._41 = -margin; mat._42 = margin; //HACK?
	mvp = mat*mvp;
	//NOTE: SetRenderTarget resets the viewport 
	D3DVIEWPORT9 vp = {0,0,sq,sq,0,1};
	pd3Dd9->SetViewport(&vp);
	pd3Dd9->SetVertexShaderConstantF(0,(float*)&mvp,4);
	pd3Dd9->SetPixelShaderConstantF(0,(float*)&psc,1); //ymax

	pd3Dd9->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);
	pd3Dd9->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
	//pd3Dd9->SetSamplerState(0,D3DSAMP_MIPFILTER,D3DTEXF_ANISOTROPIC);
	//pd3Dd9->SetSamplerState(0,D3DSAMP_MAXANISOTROPY,8);

	pd3Dd9->BeginScene();

	int sh = 0;
	std::vector<WCHAR> w; auto drawprims = [&]()
	{
		if(!w.empty())
		{
			pd3Dd9->SetVertexShader(vs[sh]); pd3Dd9->SetPixelShader(ps[sh]);
			hr = pd3Dd9->SetFVF(0x112);
			hr = pd3Dd9->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST,
			0,verts,w.size()/3,w.data(),D3DFMT_INDEX16,pverts,8*sizeof(float));
			w.clear();
		}
	};
		
	//DOCUMENT ME
	pd3Dd9->SetRenderState(D3DRS_ALPHABLENDENABLE,1);
	pd3Dd9->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,1); 
	for(int pass=1;pass<=2;pass++)
	{
		if(pass==1)
		{
			sh = 1;

			pd3Dd9->SetRenderState(D3DRS_ZENABLE,0);
			pd3Dd9->SetRenderState(D3DRS_ZWRITEENABLE,0);
			pd3Dd9->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);

			pd3Dd9->SetRenderState(D3DRS_BLENDOPALPHA,D3DBLENDOP_MIN);
			hr = pd3Dd9->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ZERO);
			hr = pd3Dd9->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_ONE);
			hr = pd3Dd9->SetRenderState(D3DRS_SRCBLENDALPHA,D3DBLEND_SRCALPHA);
			hr = pd3Dd9->SetRenderState(D3DRS_DESTBLENDALPHA,D3DBLEND_DESTALPHA);

			//sh = 2;
			pd3Dd9->SetVertexShader(vs[2]); pd3Dd9->SetPixelShader(ps[2]);
			if(!x2mhm_ico(in,margin,pd3Dd9))
			{
				return false;
			}
		}
		else
		{
			sh = 0;

			pd3Dd9->SetRenderState(D3DRS_ZENABLE,1);
			pd3Dd9->SetRenderState(D3DRS_ZWRITEENABLE,1);
			pd3Dd9->SetRenderState(D3DRS_CULLMODE,D3DCULL_CCW);

			pd3Dd9->SetRenderState(D3DRS_BLENDOPALPHA,D3DBLENDOP_ADD);
			hr = pd3Dd9->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_DESTALPHA);
			hr = pd3Dd9->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVDESTALPHA);
			hr = pd3Dd9->SetRenderState(D3DRS_SRCBLENDALPHA,D3DBLEND_ZERO);
			hr = pd3Dd9->SetRenderState(D3DRS_DESTBLENDALPHA,D3DBLEND_ONE);			
		}

		namespace msm = SWORDOFMOONLIGHT::msm;
		auto *p = (msm::polygon_t*)pp;
		int texture = -2;

		//NOTE: this is SOM_MAP's algorithm. I'd never
		//seen MSM processing code before work on this
		for(i=p->subdivs;i-->0;)
		{
			//NOTE: it really looks like SOM_MAP
			//calls SetTexture for every polygon
			if(texture!=p->texture)
			{
				//HACK: need to resolve transparency
				//texture = p->texture;

				drawprims(); //OPTIMIZING
			
				auto t = (DWORD)p->texture; //texture
				
				texture = t;						
			
				hr = pd3Dd9->SetTexture(0,icotextures[t]);
			}
			
			auto *pi = p->indices;
			for(int n=p->corners-1,j=1;j<n;j++)
			{
				w.push_back(pi[0]); w.push_back(pi[j]); w.push_back(pi[j+1]);
			}		

			//DUPLICATE
			//I'm not sure this makes sense but it's what
			//the code here seems to be doing to skip the
			//subdivisions
			for(int j=(p=msm::shiftpolygon(p))->subdivs;j-->0;)
			for(int k=(p=msm::shiftpolygon(p))->subdivs;k-->0;)
			{
				p = msm::shiftpolygon(p);
			}
		}
		drawprims(); //OPTIMIZING
	}
	if(1) //fill in alpha mask?
	{
		pd3Dd9->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);

		//WTH? I feel like ZERO/ONE shouldn't require this?
	//	pd3Dd9->SetRenderState(D3DRS_COLORWRITEENABLE,0x8);

		pd3Dd9->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_MAX);
		pd3Dd9->SetRenderState(D3DRS_BLENDOPALPHA,D3DBLENDOP_MAX);
		hr = pd3Dd9->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ZERO);
		hr = pd3Dd9->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_ONE);
		hr = pd3Dd9->SetRenderState(D3DRS_SRCBLENDALPHA,D3DBLEND_ONE);
		hr = pd3Dd9->SetRenderState(D3DRS_DESTBLENDALPHA,D3DBLEND_ZERO);

		pd3Dd9->SetVertexShader(0); pd3Dd9->SetPixelShader(0);

		float a; *(D3DCOLOR*)&a = 0xff000000;
		float fan[4*5] = {0,0,0,1,a, sq,0,0,1,a, sq,sq,0,1,a, 0,sq,0,1,a};			

		pd3Dd9->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);
		pd3Dd9->DrawPrimitiveUP(D3DPT_TRIANGLEFAN,2,fan,5*sizeof(float));

	//	pd3Dd9->SetRenderState(D3DRS_COLORWRITEENABLE,0xf);
	}

	pd3Dd9->EndScene();
	
	wchar_t icon[MAX_PATH];
	wcscpy(icon,in);		
	RECT r = {0,0,sq,sq};
	BOOL o = 0; 
	if(1) //WARNING: this is really a BMP file (it happens to work)
	{
	//	hr = pd3Dd9->GetRenderTargetData(rt,rs);
		wcscpy(PathFindExtensionW(icon),L".ico");	
		o = !D3DXSaveSurfaceToFileW(icon,D3DXIFF_BMP,rt,0,&r); //rs
	}
	else //??? this was working but stopped???
	{
		//this dc is special and only GetCurrentObject
		//works. BitBlt can't use it as a source. SelectObject
		//doesn't work
		//HDC dc;
		//if(!rt->GetDC(&dc)) //fails (unfinished)
		{
		//	HBITMAP bm = (HBITMAP)GetCurrentObject(dc,OBJ_BITMAP);

			D3DLOCKED_RECT lock;
			hr = pd3Dd9->GetRenderTargetData(rt,rs);
			hr = rs->LockRect(&lock,&r,D3DLOCK_READONLY);

			//lock.pBits is unaffected by r
		//	DWORD *buf = new DWORD[sq*sq*2], *buf2 = buf+sq*sq;
			DWORD *buf = new DWORD[sq*sq];
			for(int i=sq;i-->0;)
			memcpy(buf+sq*i,(char*)lock.pBits+lock.Pitch*i,sq*4);

		//	x2mdl_sharpen(buf,buf2,sq);

			HDC dc = GetDC(0);
			HBITMAP bm = CreateCompatibleBitmap(dc,sq,sq);
			BOOL _ = SetBitmapBits(bm,4*sq*sq,buf); //buf2
			//BITMAPINFO bmi = {sizeof(bmi)};
			//GetDIBits(dc,bm,0,sq,0,&bmi,0);
			//int _ = SetDIBits(dc,bm,0,sq,lock.pBits,&bmi,0);
			rs->UnlockRect();

			delete[] buf;

			//https://forums.codeguru.com/showthread.php?441251-CBitmap-to-HICON-or-HICON-from-HBITMAP
			extern HICON HICONFromCBitmap(HBITMAP bitmap);
			//http://www.catch22.net/tuts/sysimg.asp
			extern BOOL SaveIcon(TCHAR *szIconFile, HICON hIcon[], int nNumIcons);

			HICON i[1] = {HICONFromCBitmap(bm)};
			wcscpy(PathFindExtensionW(icon),L".ico");	
			o = SaveIcon(icon,i,1);
			DeleteObject(i[0]);

			DeleteObject(bm); //rt->ReleaseDC(dc); 
		}
	//	else assert(0);
	}
	
	return o;
}

//SOURCE
//http://www.catch22.net/tuts/sysimg.asp
//http://www.catch22.net/tuts/zips/sysimg.zip
//https://groups.google.com/g/comp.os.ms-windows.programmer.win32/c/vRpGcRldrAk?pli=1

#define WIN32_LEAN_AND_MEAN
#define STRICT

#include <windows.h>

// turn off boring warnings
#pragma warning(disable: 4244)	// conversion (long->byte, loss of data)
#pragma warning(disable: 4013)	// undefined function, assuming...

#pragma pack(push, 1)

//
//	ICONS (.ICO type 1) are structured like this:
//
//	ICONHEADER											 (just 1)
//	ICONDIR										[1...n]  (an array, 1 for each image)
//	[BITMAPINFOHEADER+COLOR_BITS+MASK_BITS]		[1...n]	 (1 after the other, for each image)
//
//	CURSORS (.ICO type 2) are identical in structure, but use
//	two monochrome bitmaps (real XOR and AND masks, this time).
//

typedef struct
{
	WORD	idReserved;		// must be 0
	WORD	idType;			// 1 = ICON, 2 = CURSOR
	WORD	idCount;		// number of images (and ICONDIRs)

	// ICONDIR   [1...n] 
	// ICONIMAGE [1...n]

} ICONHEADER;

//
//	An array of ICONDIRs immediately follow the ICONHEADER
//
typedef struct
{
	BYTE	bWidth;
	BYTE	bHeight;
	BYTE	bColorCount;
	BYTE	bReserved;
	WORD	wPlanes;		// for cursors, this field = wXHotSpot
	WORD	wBitCount;		// for cursors, this field = wYHotSpot
	DWORD	dwBytesInRes;
	DWORD	dwImageOffset;	// file-offset to the start of ICONIMAGE
	
} ICONDIR;

//
//	After the ICONDIRs follow the ICONIMAGE structures - 
//	consisting of a BITMAPINFOHEADER, (optional) RGBQUAD array, then
//	the color and mask bitmap bits (all packed together
//
typedef struct
{
	BITMAPINFOHEADER	biHeader;      // header for color bitmap (no mask header)
	//RGBQUAD			rgbColors[1...n];
	//BYTE				bXOR[1];      // DIB bits for color bitmap
	//BYTE				bAND[1];      // DIB bits for mask bitmap
	
} ICONIMAGE;

#pragma pack(pop)

//
//	Write the ICO header to disk
//
static UINT WriteIconHeader(HANDLE hFile, int nImages)
{
	ICONHEADER	iconheader;
	DWORD		nWritten;	

	// Setup the icon header
	iconheader.idReserved		= 0;		// Must be 0
	iconheader.idType			= 1;		// Type 1 = ICON  (type 2 = CURSOR)
	iconheader.idCount			= nImages;	// number of ICONDIRs 
	
	// Write the header to disk
	WriteFile(hFile, &iconheader, sizeof(iconheader), &nWritten, 0);

	// following ICONHEADER is a series of ICONDIR structures (idCount of them, in fact)
	return nWritten;
}

//
//	Return the number of BYTES the bitmap will take ON DISK
//
static UINT NumBitmapBytes(BITMAP *pBitmap)
{
	int nWidthBytes = pBitmap->bmWidthBytes;

	// bitmap scanlines MUST be a multiple of 4 bytes when stored
	// inside a bitmap resource, so round up if necessary
	if(nWidthBytes & 3)
		nWidthBytes = (nWidthBytes + 4) & ~3;

	return nWidthBytes * pBitmap->bmHeight;
}

//
//	Return number of bytes written
//
static UINT WriteIconImageHeader(HANDLE hFile, BITMAP *pbmpColor, BITMAP *pbmpMask)
{
	BITMAPINFOHEADER biHeader;
	DWORD	nWritten;
	DWORD    nImageBytes;

	// calculate how much space the COLOR and MASK bitmaps take
	nImageBytes = NumBitmapBytes(pbmpColor) + NumBitmapBytes(pbmpMask);

	// write the ICONIMAGE to disk (first the BITMAPINFOHEADER)
	ZeroMemory(&biHeader, sizeof(biHeader));

	// Fill in only those fields that are necessary
	biHeader.biSize				= sizeof(biHeader);
	biHeader.biWidth			= pbmpColor->bmWidth;
	biHeader.biHeight			= pbmpColor->bmHeight * 2;		// height of color+mono
	biHeader.biPlanes			= pbmpColor->bmPlanes;
	biHeader.biBitCount			= pbmpColor->bmBitsPixel;
	biHeader.biSizeImage		= nImageBytes;

	// write the BITMAPINFOHEADER
	WriteFile(hFile, &biHeader, sizeof(biHeader), &nWritten, 0);

	// write the RGBQUAD color table (for 16 and 256 colour icons)
	if(pbmpColor->bmBitsPixel == 2 || pbmpColor->bmBitsPixel == 8)
	{
		
	}

	return nWritten;
}

//
//	Wrapper around GetIconInfo and GetObject(BITMAP)
//
static BOOL GetIconBitmapInfo(HICON hIcon, ICONINFO *pIconInfo, BITMAP *pbmpColor, BITMAP *pbmpMask)
{
	if(!GetIconInfo(hIcon, pIconInfo))
		return FALSE;

	if(!GetObject(pIconInfo->hbmColor, sizeof(BITMAP), pbmpColor))
		return FALSE;

	if(!GetObject(pIconInfo->hbmMask,  sizeof(BITMAP), pbmpMask))
		return FALSE;

	return TRUE;
}

//
//	Write one icon directory entry - specify the index of the image
//
static UINT WriteIconDirectoryEntry(HANDLE hFile, int nIdx, HICON hIcon, UINT nImageOffset)
{
	ICONINFO	iconInfo;
	ICONDIR		iconDir;

	BITMAP		bmpColor;
	BITMAP		bmpMask;

	DWORD		nWritten;
	DWORD		nColorCount;
	DWORD		nImageBytes;

	GetIconBitmapInfo(hIcon, &iconInfo, &bmpColor, &bmpMask);
		
	nImageBytes = NumBitmapBytes(&bmpColor) + NumBitmapBytes(&bmpMask);

	if(bmpColor.bmBitsPixel >= 8)
		nColorCount = 0;
	else
		nColorCount = 1 << (bmpColor.bmBitsPixel * bmpColor.bmPlanes);

	// Create the ICONDIR structure
	iconDir.bWidth				= bmpColor.bmWidth;
	iconDir.bHeight				= bmpColor.bmHeight;
	iconDir.bColorCount			= nColorCount;
	iconDir.bReserved			= 0;
	iconDir.wPlanes				= bmpColor.bmPlanes;
	iconDir.wBitCount			= bmpColor.bmBitsPixel;
	iconDir.dwBytesInRes		= sizeof(BITMAPINFOHEADER) + nImageBytes;
	iconDir.dwImageOffset		= nImageOffset;

	// Write to disk
	WriteFile(hFile, &iconDir, sizeof(iconDir), &nWritten, 0);

	// Free resources
	DeleteObject(iconInfo.hbmColor);
	DeleteObject(iconInfo.hbmMask);

	return nWritten;
}

static UINT WriteIconData(HANDLE hFile, HBITMAP hBitmap)
{
	BITMAP	bmp;
	int		i;
	BYTE *  pIconData;
	
	DWORD	nBitmapBytes;
	DWORD	nWritten;
	
	GetObject(hBitmap, sizeof(BITMAP), &bmp);

	nBitmapBytes = NumBitmapBytes(&bmp);

	pIconData = (BYTE *)malloc(nBitmapBytes);

	GetBitmapBits(hBitmap, nBitmapBytes, pIconData);

	// bitmaps are stored inverted (vertically) when on disk..
	// so write out each line in turn, starting at the bottom + working
	// towards the top of the bitmap. Also, the bitmaps are stored in packed
	// in memory - scanlines are NOT 32bit aligned, just 1-after-the-other
	for(i = bmp.bmHeight - 1; i >= 0; i--)
	{
		// Write the bitmap scanline
		WriteFile(
			hFile, 
			pIconData + (i * bmp.bmWidthBytes),		// calculate offset to the line 
			bmp.bmWidthBytes,						// 1 line of BYTES
			&nWritten,
			0);
		
		// extend to a 32bit boundary (in the file) if necessary
		if(bmp.bmWidthBytes & 3)
		{
			DWORD padding = 0;
			WriteFile(hFile, &padding, 4 - bmp.bmWidthBytes, &nWritten, 0);
		}
	}

	free(pIconData);

	return nBitmapBytes;
}

//
//	Create a .ICO file, using the specified array of HICON images
//
BOOL SaveIcon(TCHAR *szIconFile, HICON hIcon[], int nNumIcons)
{
	HANDLE	hFile;
	int		i;
	int	*	pImageOffset;
	
	if(hIcon == 0 || nNumIcons < 1)
		return FALSE;

	// Save icon to disk:
	hFile = CreateFile(szIconFile, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

	if(hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	//
	//	Write the iconheader first of all
	//
	WriteIconHeader(hFile, nNumIcons);

	//
	//	Leave space for the IconDir entries
	//
	SetFilePointer(hFile, sizeof(ICONDIR) * nNumIcons, 0, FILE_CURRENT);

	pImageOffset = (int *)malloc(nNumIcons * sizeof(int));

	//
	//	Now write the actual icon images!
	//
	for(i = 0; i < nNumIcons; i++)
	{
		ICONINFO	iconInfo;
		BITMAP		bmpColor,  bmpMask;
		
		GetIconBitmapInfo(hIcon[i], &iconInfo, &bmpColor, &bmpMask);

		// record the file-offset of the icon image for when we write the icon directories
		pImageOffset[i] = SetFilePointer(hFile, 0, 0, FILE_CURRENT);
		
		// bitmapinfoheader + colortable
		WriteIconImageHeader(hFile, &bmpColor, &bmpMask);
		
		// color and mask bitmaps
		WriteIconData(hFile, iconInfo.hbmColor);
		WriteIconData(hFile, iconInfo.hbmMask);

		DeleteObject(iconInfo.hbmColor);
		DeleteObject(iconInfo.hbmMask);
	}

	// 
	//	Lastly, skip back and write the icon directories.
	//
	SetFilePointer(hFile, sizeof(ICONHEADER), 0, FILE_BEGIN);

	for(i = 0; i < nNumIcons; i++)
	{
		WriteIconDirectoryEntry(hFile, i, hIcon[i], pImageOffset[i]);
	}

	free(pImageOffset);

	// finished!
	CloseHandle(hFile);

	return TRUE;
}

//https://forums.codeguru.com/showthread.php?441251-CBitmap-to-HICON-or-HICON-from-HBITMAP
HICON HICONFromCBitmap(HBITMAP bitmap)
{
   BITMAP bmp;
   GetObject((HANDLE)bitmap,sizeof(bmp),&bmp);
   
   HBITMAP hbmMask = CreateCompatibleBitmap(GetDC(NULL),bmp.bmWidth,bmp.bmHeight);

   ICONINFO ii = {};
   ii.fIcon    = TRUE;
   ii.hbmColor = bitmap;
   ii.hbmMask  = hbmMask;

   HICON hIcon = CreateIconIndirect(&ii); DeleteObject(hbmMask); return hIcon;
}