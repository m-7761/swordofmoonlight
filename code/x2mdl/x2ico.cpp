
#include "x2mdl.pch.h" //PCH

#include <vector> //REMOVE ME

#include <d3dx9shader.h>

//Added for snapshot transform
#include "../lib/swordofmoonlight.h"

static const float l_255 = 1/255.0f;

#define HLSL2(x) #x
#define HLSL(...) HLSL2(__VA_ARGS__) "\n"
#define HLSL_DEFINE(x,...)\
"#define " #x " " HLSL2(__VA_ARGS__) "\n"

#define COLORPLANES 4 //3
#define MULTIPLANES true

static const char x2ico_msm_vs[] = //HLSL
{	
	HLSL(
	float4x4 mvp:register(vs,c0);
	struct VS_INPUT
	{
		float4 pos : POSITION;
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
	
		Out.pos = mul(mvp,float4(In.pos.xyz,1.0f));
		Out.col = float4(In.uv0,In.pos.w,In.pos.y);

		return Out; 
	})
};
static const char x2ico_msm_ps[] = //HLSL
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
	float4 diffuse:register(ps,c1);
	float4 emissive:register(ps,c2);
	sampler2D sam0:register(s0);

	//https://www.chilliant.com/rgb2hsv.html
	float Epsilon = 1e-10;
	float3 HUEtoRGB(in float H)
	{
		float R = abs(H * 6 - 3) - 1;
		float G = 2 - abs(H * 6 - 2);
		float B = 2 - abs(H * 6 - 4);
		return saturate(float3(R,G,B));
	}
	float3 RGBtoHCV(in float3 RGB)
	{
		// Based on work by Sam Hocevar and Emil Persson
		float4 P = (RGB.g < RGB.b) ? float4(RGB.bg, -1.0, 2.0/3.0) : float4(RGB.gb, 0.0, -1.0/3.0);
		float4 Q = (RGB.r < P.x) ? float4(P.xyw, RGB.r) : float4(RGB.r, P.yzx);
		float C = Q.x - min(Q.w, Q.y);
		float H = abs((Q.w - Q.y) / (6 * C + Epsilon) + Q.z);
		return float3(H, C, Q.x);
	}
	float3 HSVtoRGB(in float3 HSV)
	{
		float3 RGB = HUEtoRGB(HSV.x);
		return ((RGB - 1) * HSV.y + 1) * HSV.z;
	}
	float3 RGBtoHSV(in float3 RGB)
	{
		float3 HCV = RGBtoHCV(RGB);
		float S = HCV.y / (HCV.z + Epsilon);
		return float3(HCV.x, S, HCV.z);
	}
	PS_OUTPUT f(PS_INPUT In)
	{
		//Note: g is doing lighting
	
		//TODO: might do this in SOM_MAP in case
		//map_icon_brightness is too much for it 
		//float lit = pow(1.0-In.col.z,1.5);
		
		PS_OUTPUT Out;

		//brighten blacks on walls?
		//-3 is about same as without mipmaps
		float4 c = tex2Dbias(sam0,float4(In.col.xy,0,-2.5));
	//	float4 c = tex2D(sam0,In.col.xy);
	//	float4 c = 1;

		float t = 0.25*(1-abs(In.col.z));
		c.rgb = RGBtoHSV(c.rgb);
		c.rgb = HSVtoRGB(float3(c.r,lerp(c.g,0.3,t),lerp(c.b,0.5,t)));

	//	c+=0.1*lit;

		Out.col = c;
		Out.col.a = 1.0;
		Out.col*=diffuse;
		Out.col.rgb+=emissive.rgb;	
	
		return Out; 
	}
	PS_OUTPUT g(PS_INPUT In)
	{
		PS_OUTPUT Out;

		float l = abs(In.col.z);
		float c = 0.15*clamp(ceiling-In.col.w,0,0.5)*(l-In.col.z);

		//this is shaping lighting for slopes
		//so that walls are bright like walls
		l = 0.25+l*l*l*0.75;
		//l = 0.75+0.25*cos(2*3.14159*l);
		//l = pow(0.4+0.6*l,1.5);
		{
			//0.5 grass slope?
			//this is sqashing the function to
			//get indication on shallow slopes

			l = 0.5+0.5*cos(2*3.14159*l);
			l = l = pow(l,12-11*pow(1-l,12));
			l = 0.8+0.2*l;
		}	

	//	Out.col = In.col*tex2D(sam0,In.col.xy);
	//	Out.col = In.col.z;
		Out.col.rgb = 0.0;
		//TODO: may want to use alpha-blend with sort here
	//	Out.col.a = 0.5+0.5*l*l-ceiling;		
		
		//Out.col.a = max(0.5,l-c);
		float o = l-c;
	//	o = o*(o*(o*0.305306011+0.682171111)+0.012522878); //linear
		//o = max(0.3,o*o);
	//	o = 0.1+0.9*o;
	//	o = max(0,1.055*pow(o,0.416666667)-0.055); //sRGB		
		//make everything slightly transparent to mark walls for
		//SOM_MAP to use as a stencil
		Out.col.a = min(o,0.996); //254

		return Out; 
	}
	PS_OUTPUT h(PS_INPUT In)
	{
		PS_OUTPUT Out;

		Out.col.rgb = 0; 
		Out.col.a = In.col.b;

		return Out; 
	})
};

extern PDIRECT3DSURFACE9 rt = 0, rs = 0; 
extern std::vector<IDirect3DTexture9*> icotextures;
static bool x2mhm_ico(bool border, BYTE *in, size_t sz, float margin, IDirect3DDevice9 *pd3Dd9, int symmetry)
{	
	namespace mhm = SWORDOFMOONLIGHT::mhm;		
	mhm::image_t img; mhm::maptorom(img,in,sz); 
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

	float eps = 1-margin*2, l_eps = 1-eps;

	std::vector<float> lines; lines.reserve(4*3*2*2);
	auto *pi = ip;
	unsigned i,iN = hd.facecount;
	for(i=0;i<iN;i++)	
	{
		int ic = fp[i].ndexcount;

		auto *pp = ip; ip+=ic;

		if(1==fp[i].clipmode)
		{
			mhm::vector_t &m = np[fp[i].normal];
			mhm::vector_t n = {m[0],m[1],-m[2]};

			int mx = 0, mz = 0;
			float mm[2] = { FLT_MAX,-FLT_MAX };
			for(auto *p=pp;p<ip;p++)
			{
				mhm::vector_t &v = vp[*p];
				if(fabsf(v[0])<0.001f) mx++;
				if(fabsf(v[2])<0.001f) mz++;

				mm[0] = std::min(mm[0],v[1]);
				mm[1] = std::max(mm[1],v[1]);
			}
			float h = mm[1]-mm[0];

			float an0 = fabsf(n[0]);
			float an2 = fabsf(n[2]);
			float diagonal = an0+an2;
			diagonal = 1+(diagonal-1)*0.5f;
			
			for(auto *p=pp;p<ip;p++)
			{
				mhm::vector_t &w = vp[*p];
				mhm::vector_t v = {w[0],w[1],-w[2]};
				mhm::vector_t &w2 = vp[p+1==ip?pp[0]:p[1]];
				mhm::vector_t v2 = {w2[0],w2[1],-w2[2]};
				
				float dx = v2[0]-v[0];
				float dz = v2[2]-v[2];
				float m = sqrtf(dx*dx+dz*dz);
				if(!m) continue;
				dx = dx/m*margin*2;
				dz = dz/m*margin*2;
				
				//NOTE: this has to clear walls
				float nx = n[0]*margin*2.1f;
				float nz = n[2]*margin*2.1f;
				if(!border&&h<=0.5f)
				{
					nx = -nx; nz = -nz;

					dx = dz = 0;
				}
				//maybe_add_wall?
				{
					//NOTE: I think !m does this as well
					if(fabsf(v[0]-v2[0])<l_eps
					 &&fabsf(v[2]-v2[2])<l_eps) continue;

					if(v[0]<-eps&&v2[0]<-eps)
					{
						if(v[0]>-1.01f&&v2[0]>-1.01f) //ii0017
						{
							v[0]+=l_eps; v2[0]+=l_eps;

							if(!border)
							{												
								if(n[0]<0&&h>0.5f) nx = -nx;
							}
						}
						else continue;
					}
					else if(v[0]>eps&&v2[0]>eps)
					{
						if(v[0]<1.01f&&v2[0]<1.01f)
						{
							v[0]-=l_eps; v2[0]-=l_eps;

							if(!border)
							{
								if(n[0]>0&&h>0.5f) nx = -nx;
							}
						}
						else continue;
					}
					if(v[2]<-eps&&v2[2]<-eps)
					{
						if(v[2]>-1.01f&&v2[2]>-1.01f)
						{
							v[2]+=l_eps; v2[2]+=l_eps;

							if(!border)
							{
								if(n[2]<0&&h>0.5f) nz = -nz;
							}
						}
						else continue;
					}
					else if(v[2]>eps&&v2[2]>eps)
					{
						if(v[2]<1.01f&&v2[2]<1.01f)
						{
							v[2]-=l_eps; v2[2]-=l_eps;

							if(!border)
							{
								if(n[2]>0&&h>0.5f) nz = - nz;
							}
						}
						else continue;
					}
				}
				v[0]+=nx-dx; v2[0]+=nx+dx; 
				v[2]+=nz-dz; v2[2]+=nz+dz;
				
				static const float sh = 0.6f;
				static const float sh2 = (1-sh)/2+sh;

				lines.insert(lines.end(),v,v+3);
				lines.push_back(sh);
				lines.insert(lines.end(),v2,v2+3);
				lines.push_back(sh);

				//v[0]+=nx-dx; v2[0]+=nx+dx; 
				//v[2]+=nz-dz; v2[2]+=nz+dz;
				v[0]+=nx/diagonal; v2[0]+=nx/diagonal; 
				v[2]+=nz/diagonal; v2[2]+=nz/diagonal;

				lines.insert(lines.end(),v,v+3);
				lines.push_back(sh2);
				lines.insert(lines.end(),v2,v2+3);
				lines.push_back(sh2);
			}

			pd3Dd9->SetFVF(D3DFVF_XYZW);
			pd3Dd9->DrawPrimitiveUP(D3DPT_LINELIST,lines.size()/8,lines.data(),4*sizeof(float));

			lines.clear();
		}
	}

	return true;
}
struct x2ico_vertex
{
	float v[6];
	float &operator[](int i){ return v[i]; }

	x2ico_vertex(int i, float *pverts)
	{
		auto *pv = pverts+8*i;
		v[0] = pv[0];
		v[1] = pv[1];
		v[2] = pv[2];
		v[3] = pv[4];
		v[4] = pv[6];
		v[5] = pv[7];
	}
};
extern bool x2msm_ico(int sq, WCHAR *in, IDirect3DDevice9 *pd3Dd9, IDirect3DSurface9 *surfs[3])
{
	auto *rt = surfs[0];
	auto *rs = surfs[1];
	auto *ms = surfs[2];

	assert(rt); if(!rt) return false;

	wcscpy(PathFindExtensionW(in),L".msm"); //YUCK
	FILE *f = _wfopen(in,L"rb");
	if(!f) return false;
	fseek(f,0,SEEK_END);
	std::vector<BYTE> msm(ftell(f));
	fseek(f,0,SEEK_SET);
	fread(msm.data(),msm.size(),1,f);
	fclose(f);
		
	wcscpy(PathFindExtensionW(in),L".mhm");
	f = _wfopen(in,L"rb");
	if(!f) return false;
	fseek(f,0,SEEK_END);
	std::vector<BYTE> mhm(ftell(f));
	fseek(f,0,SEEK_SET);
	fread(mhm.data(),mhm.size(),1,f);
	fclose(f);
	
	float margin = 0.5f/(sq/2.0f);

	float eps = 1-margin*2, l_eps = 1-eps;

	BYTE *m = msm.data();
	int i = *(short*)(m); //textures
	BYTE *pp = m+2; //textures names
	for(;i-->0;pp++) 
	while(*pp) pp++; //scanning past texture names

	int verts = *(WORD*)pp; pp+=2;

	auto pverts = (float*)pp; pp+=verts*8*sizeof(float); //polygon data
	
	float psc[4], &ceiling = psc[0];
	ceiling = -1000; //-FLT_MAX; //underflow?
	float mm[2] = {+1000,-1000}; //wireframe?
	for(float*p=pverts;p<(float*)pp;p+=8)
	{
	//	p[1] = -p[1]; p[4] = -p[4];
		p[2] = -p[2]; p[5] = -p[5]; //keep upright
		if(p[4]<-0.8)
		ceiling = std::max(ceiling,p[1]);
		
		mm[0] = std::min(mm[0],p[1]);
		mm[1] = std::max(mm[1],p[1]);
	}
	#ifdef _DEBUG
	enum{ wireframe=!1, grid=!10 }; 
	#else
	enum{ wireframe=0, grid=0 }; 
	#endif 
	if(wireframe) //TESTING
	{
		float yd = 1/(mm[1]-mm[0]);
		float x = margin/-10, y_x = -x-x;
		if(yd) for(float*p=pverts;p<(float*)pp;p+=8)
		{
			float t = (p[1]-mm[0])*yd;

			t = 1+x+y_x*t; p[0]*=t; p[2]*=t;
		}
	}

	auto hr = pd3Dd9->SetRenderTarget(0,ms?ms:rt);
	pd3Dd9->Clear(0,0,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1.0f,0);
	static IDirect3DPixelShader9 *ps[3] = {};
	static IDirect3DVertexShader9 *vs[1] = {};
	if(!vs[0]) for(int i=4;i-->0;)
	{
		char main[2] = "f"; *main+=i;

		DWORD cflags =
		D3DXSHADER_AVOID_FLOW_CONTROL|D3DXSHADER_OPTIMIZATION_LEVEL3;
		LPD3DXBUFFER obj,err = 0;
		if(i==3) //vs?
		{
			if(!D3DXCompileShader(x2ico_msm_vs,sizeof(x2ico_msm_vs)-1,0,0,"f","vs_3_0",cflags,&obj,&err,0))
			{
				pd3Dd9->CreateVertexShader((DWORD*)obj->GetBufferPointer(),&vs[0]); obj->Release();
			}
		}
		else if(!D3DXCompileShader(x2ico_msm_ps,sizeof(x2ico_msm_ps)-1,0,0,main,"ps_3_0",cflags,&obj,&err,0))
		{
			pd3Dd9->CreatePixelShader((DWORD*)obj->GetBufferPointer(),&ps[i]); obj->Release();
		}
		if(err)
		{
			char *e = (char*)err->GetBufferPointer(); 

			assert(!err); err->Release(); //breakpoint
		}
	}
	pd3Dd9->SetVertexShader(vs[0]);

//	pd3Dd9->CreateStateBlock(D3DSBT_ALL); //caller is responsible

	//transparency will form a cross in the middle
	//NOTE: walls must be drawn first in this case
	pd3Dd9->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESS); //LESSEQUAL

	int symmetry = 4, mode = 2;

	int sq2 = sq/(symmetry==4?2:1);

	bool odd = sq2*2<sq*2;

	for(int v=0;v<symmetry;v++) //rotation symmetry?
	{
		//NOTE: SetRenderTarget resets the viewport 
		D3DVIEWPORT9 vp = {0,0,sq2+odd,sq2+odd,0,1};
		if(symmetry==4) switch(v)
		{
		case 1: vp.X = sq2; break;
		case 3: vp.X = vp.Y = sq2; break;
		case 2: vp.Y = sq2; break;
		}
		if(odd&&mode==1) switch(v)
		{
		case 1: vp.X++; break;
		case 3: vp.X++; vp.Y++; break;
		case 2: vp.Y++; break;
		}		
		pd3Dd9->SetViewport(&vp);

		//I would guess margin*4, I don't understand
		//why 2 isn't 1 pixel???
		//10 works well with rounded outer corners
		//11 is almost perfect
		//10.5 is good for kf2
		//8.55-9.65 works for 1/2 diagonals
		float l = sq>=25?1+margin*4.5f:1; //4.0

		D3DXMATRIX mat,prj;
		if(symmetry==1)
		D3DXMatrixOrthoLH(&prj,2,2,25,-25); //-25,25 ???
		if(symmetry==4)
		D3DXMatrixOrthoOffCenterLH(&prj,-l,odd?margin*1:0,odd?margin*-1:0,l,25,-25);
		const float M_Pi_2 = 1.57079632679489661923f; // pi/2
		D3DXMatrixRotationX(&mat,M_Pi_2);
		if(1)
		{
			//these were because the tile didn't
			//get covered before
			mat._41 = -margin; 
			mat._42 = margin; //HACK?
		}
		float sx = 1, sy = sx;
		if(symmetry==4) if(mode==1) 
		{
			switch(v)
			{
			case 1: //mat._41-=margin*2;
				sx = -sx; break;		
			case 3: //mat._41-=margin*2; mat._42+=margin*2;
				sx = sy = -sx; break; 
			case 2: //mat._42+=margin*2;
				sy = -sx; break;
			}		
			mat._11*=sx; mat._21*=sx; mat._31*=sx;
			mat._12*=sy; mat._22*=sy; mat._32*=sy;
		}
		else if(mode==2)
		{
			switch(v)
			{
			case 1: mat._41-=l-margin; //margin*2?
					break;		
			case 3: mat._41-=l-margin; mat._42+=l-margin;
					break; 
			case 2: mat._42+=l-margin;
					break;
			}
		}
		D3DXMATRIX mvp = mat*prj;

		auto mirror = 1&&sx!=sy?D3DCULL_CW:D3DCULL_CCW;
	
		pd3Dd9->SetVertexShaderConstantF(0,(float*)&mvp,4);
		pd3Dd9->SetPixelShaderConstantF(0,(float*)&psc,1); //ymax

		pd3Dd9->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);
		pd3Dd9->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
		extern int max_anisotropy; //mipmaps?
		pd3Dd9->SetSamplerState(0,D3DSAMP_MIPFILTER,D3DTEXF_ANISOTROPIC); //UNUSED
		//NOTE: my higher-end card doesn't do 16, assuming 8 it universal		
		pd3Dd9->SetSamplerState(0,D3DSAMP_MAXANISOTROPY,std::min(16,max_anisotropy));

		pd3Dd9->BeginScene();

		int pass, sh = 0;
		std::vector<x2ico_vertex> w;
		std::vector<x2ico_vertex> wf; auto drawprims = [&]()
		{
			if(w.empty()&&wf.empty()) return;
			
			pd3Dd9->SetPixelShader(ps[sh]); //vs[sh]

			hr = pd3Dd9->SetFVF(D3DFVF_XYZW|D3DFVF_TEX1); //0x112
			if(!w.empty())
			{
			//	hr = pd3Dd9->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST,
			//	0,verts,w.size()/3,w.data(),D3DFMT_INDEX16,pverts,8*sizeof(float));
				hr = pd3Dd9->DrawPrimitiveUP(D3DPT_TRIANGLELIST,w.size()/3,w.data(),6*sizeof(float));
				w.clear();
			}
			if(!wf.empty())
			{
			//	hr = pd3Dd9->DrawIndexedPrimitiveUP(D3DPT_LINELIST,
			//	0,verts,wf.size()/2,wf.data(),D3DFMT_INDEX16,pverts,8*sizeof(float));
				hr = pd3Dd9->DrawPrimitiveUP(D3DPT_LINELIST,wf.size()/2,wf.data(),6*sizeof(float));
				wf.clear();
			}
		};
		auto maybe_add_wall = [&](int i, int j)
		{
			//float *pi = pverts+i*8;
			//float *pj = pverts+j*8;
			x2ico_vertex pi(i,pverts);
			x2ico_vertex pj(j,pverts);

			//HACK? remove near vertical lines?
			//NOTE: these may move vertices below
			//(which is a problem)
			if(fabsf(pi[0]-pj[0])<l_eps
			 &&fabsf(pi[2]-pj[2])<l_eps) return;

			//walls are only visible on 2 sides
			//so they create asymmetry when rotated
			//and paired side-by-side
			//they can be scooted over, but shadows
			//must be too, and it can't be done with
			//indices if so
			if(pi[0]<-eps&&pj[0]<-eps) //inverted
			{
				if(pi[0]>-1.01f&&pj[0]>-1.01f) //ii0017
				{
					pi[0]+=l_eps; pj[0]+=l_eps;
				}
				else return;
			}
			else if(pi[0]>eps&&pj[0]>eps)
			{
				if(pi[0]<1.01f&&pj[0]<1.01f)
				{
					pi[0]-=l_eps; pj[0]-=l_eps;
				}
				else return;
			}
			if(pi[2]<-eps&&pj[2]<-eps) //inverted
			{
				if(pi[2]>-1.01f&&pj[2]>-1.01f)
				{
					pi[2]+=l_eps; pj[2]+=l_eps;
				}
				else return;
			}
			else if(pi[2]>eps&&pj[2]>eps)
			{
				if(pi[2]<1.01f&&pj[2]<1.01f)
				{
					pi[2]-=l_eps; pj[2]-=l_eps;
				}
				else return;
			}
			wf.push_back(pi); wf.push_back(pj); 
		};
		
		pd3Dd9->SetRenderState(D3DRS_ALPHABLENDENABLE,1);
		pd3Dd9->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,1); 
		int passes = 3;
		for(pass=1;pass<=passes;pass++)
		{
			if(pass==4)
			{
				mat._43+=0.01f; //transparency?

				mvp = mat*prj;

				pd3Dd9->SetVertexShaderConstantF(0,(float*)&mvp,4);
			}
			else if(pass==2)
			{
				sh = 1;

				pd3Dd9->SetRenderState(D3DRS_ZENABLE,1);
				pd3Dd9->SetRenderState(D3DRS_ZWRITEENABLE,0);
				pd3Dd9->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);

				hr = pd3Dd9->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ZERO);
				hr = pd3Dd9->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_ONE);
				pd3Dd9->SetRenderState(D3DRS_BLENDOPALPHA,D3DBLENDOP_MIN);
				hr = pd3Dd9->SetRenderState(D3DRS_SRCBLENDALPHA,D3DBLEND_SRCALPHA);
				hr = pd3Dd9->SetRenderState(D3DRS_DESTBLENDALPHA,D3DBLEND_DESTALPHA);

				//sh = 2;
				pd3Dd9->SetPixelShader(ps[2]); //vs[2]
				if(!x2mhm_ico(sq>=25,mhm.data(),mhm.size(),margin,pd3Dd9,symmetry))
				{
					return false;
				}
			}
			else
			{
				sh = 0;

				pd3Dd9->SetRenderState(D3DRS_ZENABLE,1);
				pd3Dd9->SetRenderState(D3DRS_ZWRITEENABLE,1);
				pd3Dd9->SetRenderState(D3DRS_CULLMODE,mirror);

				pd3Dd9->SetRenderState(D3DRS_BLENDOPALPHA,D3DBLENDOP_ADD);
				hr = pd3Dd9->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
				hr = pd3Dd9->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
				hr = pd3Dd9->SetRenderState(D3DRS_SRCBLENDALPHA,D3DBLEND_ZERO);
				hr = pd3Dd9->SetRenderState(D3DRS_DESTBLENDALPHA,D3DBLEND_ONE);

				if(wireframe)
				pd3Dd9->SetRenderState(D3DRS_FILLMODE,pass==1?D3DFILL_WIREFRAME:D3DFILL_SOLID);
			}

			namespace msm = SWORDOFMOONLIGHT::msm;
			auto *p = (msm::polygon_t*)pp;
			int texture = -2; 
			bool keep = true;

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
			
					texture = (DWORD)p->texture;
			
					if((unsigned)texture<icotextures.size())
					{
						hr = pd3Dd9->SetTexture(0,icotextures[texture]);

						extern unsigned char(controlpts[])[4];
						extern float materials[][4],materials2[][4];

						int mi = 0;				 
						for(int i=texture;i>0;mi++) if(!*(DWORD*)controlpts[mi]) 
						{							
							i--;
						}
						float alpha = materials[mi][3];
										
						switch(pass)
						{
						case 1: //wall marker?
					
							//NOTE: the rasterizer may vary alpha? 253 is normal
							materials[mi][3] = 254*l_255;
							break;
					
						case 2: case 3:
						
							materials[mi][3] = 1.0f;
							break;
						}

						if(pass>=3) //TODO: accumulate?
						{
							keep = (pass==3)==(alpha>=1); 

							if(1&&!keep) passes = 4; //OPTIMIZING? //ILLUSTRATING
						}

						if(keep) pd3Dd9->SetPixelShaderConstantF(1,materials[mi],1);
						if(keep) pd3Dd9->SetPixelShaderConstantF(2,materials2[mi],1);

						materials[mi][3] = alpha;
					}
					else
					{
						keep = false; assert(0); //paranoia
					}
				}
			
				auto *pi = p->indices; if(keep)
				{
					bool y = false;
					for(int i=p->corners;i-->0;)
					{
						//0.05f is too high for ii0105
						if(fabs(pverts[pi[0]*8+4])>0.005f) y = true;
					}
					if(!y) //lines?
					{
						if(pass==1)
						if(!wireframe)
						for(int n=p->corners-1,j=1;j<n;j++)
						{
							maybe_add_wall(pi[0],pi[j]);
							maybe_add_wall(pi[j],pi[j+1]);
							maybe_add_wall(pi[j+1],pi[0]);							
						}
						else goto w;
					}
					else if(pass>=2) w: for(int n=p->corners-1,j=1;j<n;j++)
					{
						w.push_back(x2ico_vertex(pi[0],pverts));
						w.push_back(x2ico_vertex(pi[j],pverts));
						w.push_back(x2ico_vertex(pi[j+1],pverts));
					}							
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

		pd3Dd9->EndScene();
	}
	pd3Dd9->SetRenderState(D3DRS_CULLMODE,D3DCULL_CCW);
	
	//NOTE: has a black border problem with MSM drawing
	if(ms) pd3Dd9->StretchRect(ms,0,rt,0,D3DTEXF_POINT);

	RECT r = {0,0,sq,sq};
	D3DLOCKED_RECT lock;
	hr = pd3Dd9->GetRenderTargetData(rt,rs);
	hr = rs->LockRect(&lock,&r,D3DLOCK_READONLY);

	int sq1 = sq+odd, sq21 = sq2+odd;

	int lp4 = lock.Pitch/4; assert(lp4*4==lock.Pitch);

	if(symmetry==4&&mode==1)
	{
		for(int i=sq1;i-->0;)
		{
			BYTE *pp = (BYTE*)lock.pBits+lock.Pitch*i;

			DWORD *q = (DWORD*)pp+sq1-1;
			DWORD *p = (DWORD*)pp+sq21;
			for(int j=sq2/2;j-->0;p++,q--) std::swap(*p,*q);
		}
		for(int i=sq21,j=sq1-1,n=sq1-sq2/2;i<n;i++,j--)
		{
			BYTE *pp = (BYTE*)lock.pBits+lock.Pitch*i;
			BYTE *qq = (BYTE*)lock.pBits+lock.Pitch*j;
			
			DWORD *q = (DWORD*)qq;
			DWORD *p = (DWORD*)pp;
			for(int k=sq1;k-->0;p++,q++) std::swap(*p,*q);
		}
		if(odd) for(int i=0;i<sq1;i++)
		{
			DWORD *pp = (DWORD*)lock.pBits+lp4*i;
			DWORD *dst = pp+sq2, *src = dst+1;
			if(i>sq2)
			{
				memmove(pp,pp+lp4,sq21*4); src+=lp4;

				if(i==sq1-1) break;
			}
			memmove(dst,src,sq21*4);
		}
	}
		
	int lb = sq>=25?2:0;
	int rb = sq>=25?sq-2:sq;
	int center = sq/2;
	if(!lb) //OLD: can't preserve alpha this way
	{
		enum{ constrast=0 };
		if(1) for(int i=0;i<sq;i++)
		{
			BYTE *pp = (BYTE*)lock.pBits+lock.Pitch*i;
			BYTE *p = pp;
			for(int j=sq;j-->0;p+=4)
			{
				if(i>=lb&&i<rb&&j>=lb&&j<rb)
				{
					float a = p[3]*l_255;
					p[0]*=a;
					p[1]*=a;
					p[2]*=a;
					if(!constrast) //...
					p[3] = 255; 
				}
				else if(p[3]!=255)
				{
					p[0] = p[1] = p[2] = p[3]; //border
				}
				else p[0] = p[1] = p[2] = p[3] = 0;
			}
		}
		if(constrast) for(int i=0;i<sq;i++)
		{
			BYTE *pp = (BYTE*)lock.pBits+lock.Pitch*i;
			BYTE *p = pp;
			for(int j=sq;j-->0;p+=4)
			{
				float c = sqrt(2.1f-p[3]*l_255);

				for(int k=4;k-->0;)
				{
					float l = p[k]*l_255-0.5f;
					l = 0.5f+l*c;
					p[k] = (BYTE)std::min(255,std::max<int>(0,255*l));
				}
			}
		}
	}
	else for(int i=0;i<sq;i++) //black border?
	{
		BYTE *pp = (BYTE*)lock.pBits+lock.Pitch*i;
		BYTE *p = pp;
		for(int j=sq;j-->0;p+=4)
		{
			if(i<lb||i>=rb||j<lb||j>=rb)
			{
				p[0] = p[1] = p[2] = 0;
			}
			else if(grid) //HACK
			{
				if((j-center)%grid==0||(i-center)%grid==0)
				{
					p[3] = 200;					
				}				
			}
		}
	}

	wchar_t icon[MAX_PATH];
	wcscpy(icon,in);		
	BOOL o = 0; 
	if(sq>25) //WARNING: this is really a BMP file (it happens to work)
	{
		rs->UnlockRect();

		//wcscpy(PathFindExtensionW(icon),L".ico");	
		//o = !D3DXSaveSurfaceToFileW(icon,D3DXIFF_BMP,rs,0,&r); //rt
		wcscpy(PathFindExtensionW(icon),L"_big.png");	
		o = !D3DXSaveSurfaceToFileW(icon,D3DXIFF_PNG,rs,0,&r); //rt
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

			//lock.pBits is unaffected by r
		//	DWORD *buf = new DWORD[sq*sq*2], *buf2 = buf+sq*sq;
			DWORD *buf = new DWORD[sq*sq];
			for(int i=sq;i-->0;)
			memcpy(buf+sq*i,(char*)lock.pBits+lock.Pitch*i,sq*4);

			HDC dc = GetDC(0);
			HBITMAP bm = CreateCompatibleBitmap(dc,sq,sq);
			BOOL _ = SetBitmapBits(bm,4*sq*sq,buf); //buf2
			//BITMAPINFO bmi = {sizeof(bmi)};
			//GetDIBits(dc,bm,0,sq,0,&bmi,0);
			//int _ = SetDIBits(dc,bm,0,sq,lock.pBits,&bmi,0);
			rs->UnlockRect();

			HBITMAP bm2 = 0; if(0) //preview
			{
				int x2 = (rb-lb)*2;

				BYTE *buf2 = new BYTE[4*x2*x2];
				BYTE *p = buf2;
				for(int i=lb;i<rb;i++)
				{
					BYTE *q = (BYTE*)(buf+sq*i+lb);

					for(int j=rb;j-->lb;p+=8,q+=4)
					{
						p[0] = q[0];
						p[1] = q[1];
						p[2] = q[2];
						p[3] = 255;
						if(lb)
						{
							float a = q[3]*l_255;
							p[0]*=a;
							p[1]*=a;
							p[2]*=a;
						}
						p[4] = p[0];
						p[5] = p[1];
						p[6] = p[2];
						p[7] = 255;
					}
					int stride = 4*x2;
					memcpy(p,p-stride,stride);
					p+=stride;
				}

				bm2 = CreateCompatibleBitmap(dc,x2,x2);
				_ = SetBitmapBits(bm2,4*x2*x2,buf2); 

				delete[] buf2;
			}

			delete[] buf; 

			//https://forums.codeguru.com/showthread.php?441251-CBitmap-to-HICON-or-HICON-from-HBITMAP
		//	extern HICON HICONFromCBitmap(HBITMAP bitmap);
			//http://www.catch22.net/tuts/sysimg.asp
			extern BOOL SaveIcon(TCHAR *szIconFile, HBITMAP hIcon[], int nNumIcons);

			//HICON i[2] = {HICONFromCBitmap(bm),bm2?HICONFromCBitmap(bm2):0};
			HBITMAP i[2] = {bm,bm2};
			if(bm2) std::swap(i[0],i[1]); //Windows Explorer
			wcscpy(PathFindExtensionW(icon),L".ico");	
			o = SaveIcon(icon,i,bm2?2:1);
			DeleteObject(i[0]); //DestroyIcon

			if(i[1]) DeleteObject(i[1]); //DestroyIcon

			//rt->ReleaseDC(dc); 
		}
	//	else assert(0);
	}
	
	return o;
}

namespace x2ico //EXPERIMENTAL
{
	namespace mdl = SWORDOFMOONLIGHT::mdl;
	namespace mdo = SWORDOFMOONLIGHT::mdo;

	struct weight_t
	{
		BYTE pt,ch,wt,ct; WORD first[1];

		weight_t *next()
		{
			return (weight_t*)(first+ct);
		}
	};

	struct animator_base
	{
		std::vector<mdl::animation_t> anims;
		std::vector<mdl::hardanim_t> hard_chs;
		bool *knowns; float *mats;

		int cbsz; float *cb,*vb; 

		mdl::softanimframe_t *soft_fr;

		int anim; float time;

		int time_rounded, time_remaining;
	};
	struct animator : private animator_base
	{
		mdl::image_t &in;
		mdo::image_t &out;
		animator(mdo::image_t &jn, mdl::image_t &in)
		:in(in),out(jn),wt(){ cb = 0; _init(); }
		~animator(){ clear(); }

		weight_t *wt; size_t wt_sz;

		void weight(void *w, size_t sz)
		{
			(void*&)wt = w; wt_sz = sz;
		}

		void begin(int anim);
		
		void advance(float step);

		void clear(),_init();

		bool empty(){ return anims.empty(); }

		const animator_base &cc()
		{
			return static_cast<animator&>(*this);
		}
	};
}

#define X2ICO_SHADOW_EXPAND 0.15
static const char x2ico_mdo_vs[] = //HLSL
{	
	HLSL(
	float4x4 mvp:register(vs,c0);
	float4 ceiling:register(vs,c4);
	struct VS_INPUT
	{
		float3 pos : POSITION;
		float3 lit : NORMAL;
		float2 uv0 : TEXCOORD0;
	};
	struct VS_OUTPUT
	{
		float4 pos : POSITION;
		float3 lit : NORMAL;
		float2 uv0 : TEXCOORD0;
	};
	VS_OUTPUT f(VS_INPUT In)
	{
		VS_OUTPUT Out; 
	
		Out.pos = mul(mvp,float4(In.pos.xyz,1.0f));
		Out.lit = In.lit;
		Out.uv0 = In.uv0;

		return Out; 
	}
	VS_OUTPUT g(VS_INPUT In)
	{
		VS_OUTPUT Out; 
	
		Out.lit = (ceiling.y-In.pos.y)*ceiling.x*0.996;

		float3 pos = In.pos;
		pos.xz-=ceiling.zw;
		pos.xz*=1+(1-Out.lit)*X2ICO_SHADOW_EXPAND;
		pos.xz+=ceiling.zw;

		Out.pos = mul(mvp,float4(pos,1.0f));
		Out.lit = 1-Out.lit;
		Out.uv0 = In.uv0;

		return Out; 
	})
};
static const char x2ico_mdo_ps[] = //HLSL
{	
	HLSL(
	struct PS_INPUT
	{
		float3 lit : NORMAL;
		float2 uv0 : TEXCOORD;
	};
	struct PS_OUTPUT
	{
		float4 col : COLOR0;
	};
	float4 diffuse:register(ps,c1);
	float4 emissive:register(ps,c2);
	//int i:register(ps,i0);
	float4 i:register(ps,c3);
	sampler2D sam0:register(s0);

	float Epsilon = 1e-10;
	float3 HUEtoRGB(in float H)
	{
		float R = abs(H * 6 - 3) - 1;
		float G = 2 - abs(H * 6 - 2);
		float B = 2 - abs(H * 6 - 4);
		return saturate(float3(R,G,B));
	}
	float3 RGBtoHCV(in float3 RGB)
	{
		// Based on work by Sam Hocevar and Emil Persson
		float4 P = (RGB.g < RGB.b) ? float4(RGB.bg, -1.0, 2.0/3.0) : float4(RGB.gb, 0.0, -1.0/3.0);
		float4 Q = (RGB.r < P.x) ? float4(P.xyw, RGB.r) : float4(RGB.r, P.yzx);
		float C = Q.x - min(Q.w, Q.y);
		float H = abs((Q.w - Q.y) / (6 * C + Epsilon) + Q.z);
		return float3(H, C, Q.x);
	}
	float3 HSVtoRGB(in float3 HSV)
	{
		float3 RGB = HUEtoRGB(HSV.x);
		return ((RGB - 1) * HSV.y + 1) * HSV.z;
	}
	float3 RGBtoHSV(in float3 RGB)
	{
		float3 HCV = RGBtoHCV(RGB);
		float S = HCV.y / (HCV.z + Epsilon);
		return float3(HCV.x, S, HCV.z);
	}
	PS_OUTPUT f(PS_INPUT In)
	{
		PS_OUTPUT Out;

		float lit = sqrt(In.lit.z);

		float4 c = tex2D(sam0,In.uv0);
		//c = c*0.5+0.5*c*In.lit.z; 
		c = 0.5*c+0.5*c*lit;
		c+=0.25*lit; //faux specular
		c = pow(c,1.5);

		//TODO: make yellow,green,red,blue?
	//	float t = 0.25*(1-abs(In.col.z));
		c.rgb = RGBtoHSV(c.rgb);
		c.rgb = HSVtoRGB(float3(c.r,min(1,c.g+0.10),max(0,c.b-0.10)));

		Out.col = c;
		Out.col.a = 1.0;
		Out.col*=diffuse;
		Out.col.rgb+=emissive.rgb;	
	
		return Out; 
	}
	PS_OUTPUT g(PS_INPUT In)
	{
		PS_OUTPUT Out;

		float1 c = In.lit.y+(1-In.lit.y)*(1-diffuse.a);

		Out.col = c.rrrr*i;
		
		if(3==COLORPLANES) Out.col.a = 1;

		return Out; 
	})
};
static IDirect3DPixelShader9 *x2ico_mdo_pshaders[2] = {};
static IDirect3DVertexShader9 *x2ico_mdo_vshaders[2] = {};
extern bool x2mdo_ico
(int sq, WCHAR *in, IDirect3DDevice9 *pd3Dd9, IDirect3DSurface9 *surfs[4])
{
	auto *rt = surfs[0];
	auto *rs = surfs[1];
	auto *ms = surfs[2];
	auto *ds = surfs[3];

	assert(rt); if(!rt) return false;

	auto *ext = PathFindExtensionW(in);

	std::vector<BYTE> buf1,buf2,buf3;

	auto mopen = [&](int b, const wchar_t *ext2)->bool
	{
		auto &buf = b==1?buf1:b==2?buf2:buf3;

		wcscpy(ext,ext2);
	
		FILE *f = _wfopen(in,L"rb");
		if(!f) return false;
		fseek(f,0,SEEK_END);
		buf.resize(ftell(f));
		fseek(f,0,SEEK_SET);
		fread(buf.data(),buf.size(),1,f);
		fclose(f); return true;
	};
	if(!mopen(1,L".mdo")) 
	{
		assert(0); return false;
	}

	namespace mdo = SWORDOFMOONLIGHT::mdo;
	mdo::image_t img;
	mdo::maptorom(img,buf1.data(),buf1.size());

	std::vector<HBITMAP> kage; //NEW: MDL file?

	namespace mdl = SWORDOFMOONLIGHT::mdl;
	mdl::image_t jmg;
	bool hard = false, soft = false;
	if(mopen(2,L".mdl"))
	{
		mdl::maptorom(jmg,buf2.data(),buf2.size());
	}
	else jmg.bad = 1;

	x2ico::animator animator(img,jmg);
	auto &anims = animator.cc().anims;		

	if(mopen(3,L".wt"))
	animator.weight(buf3.data(),buf3.size());

	//NOTE: shadows source from the MDL's positions
	{
		auto &chans = mdo::channels(img);
		
		for(int i=chans.count;i-->0;)
		{		
			auto &ch = chans[i]; 
			auto *verts = mdo::tnlvertexpackets(img,i);
			for(int i=ch.vertcount;i-->0;)
			{
				auto *p = verts[i].pos;
					
				p[2] = -p[2]; p[5] = -p[5]; //normals
			}
		}
	}

	auto &ps = x2ico_mdo_pshaders;
	auto &vs = x2ico_mdo_vshaders;
	if(!vs[0]) for(int i=2;i-->0;)
	{
		char main[2] = "f"; *main+=i;

		DWORD cflags =
		D3DXSHADER_AVOID_FLOW_CONTROL|D3DXSHADER_OPTIMIZATION_LEVEL3;
		LPD3DXBUFFER obj = 0, err = 0;
		if(!D3DXCompileShader(x2ico_mdo_vs,sizeof(x2ico_mdo_vs)-1,0,0,main,"vs_3_0",cflags,&obj,&err,0))
		{
			pd3Dd9->CreateVertexShader((DWORD*)obj->GetBufferPointer(),&vs[i]); obj->Release();
		}
		if(err) //warning X3571: pow(f, e) will not work for negative f
		{
			char *e = (char*)err->GetBufferPointer(); 

			assert(!err||obj); err->Release(); //breakpoint
		}
		if(!D3DXCompileShader(x2ico_mdo_ps,sizeof(x2ico_mdo_ps)-1,0,0,main,"ps_3_0",cflags,&obj,&err,0))
		{
			pd3Dd9->CreatePixelShader((DWORD*)obj->GetBufferPointer(),&ps[i]); obj->Release();
		}
		if(err) //warning X3571: pow(f, e) will not work for negative f
		{
			char *e = (char*)err->GetBufferPointer(); 

			assert(!err||obj); err->Release(); //breakpoint
		}
	}

//	pd3Dd9->CreateStateBlock(D3DSBT_ALL); //caller is responsible

	pd3Dd9->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);

	//alpha looks funny (e.g. green slime)
	//NOTE: multisample forces alphablend! so
	//instead the material alpha is set to 1...
	pd3Dd9->SetRenderState(D3DRS_ALPHABLENDENABLE,1);
	pd3Dd9->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);
	pd3Dd9->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	pd3Dd9->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
	
	pd3Dd9->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);
	pd3Dd9->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
	//pd3Dd9->SetSamplerState(0,D3DSAMP_MIPFILTER,D3DTEXF_ANISOTROPIC);
	//pd3Dd9->SetSamplerState(0,D3DSAMP_MAXANISOTROPY,16);
		
	pd3Dd9->SetRenderState(D3DRS_ZENABLE,1);
	pd3Dd9->SetRenderState(D3DRS_ZWRITEENABLE,1);

	int fr = 0; //debugging

	float sh_sq = 0;

	int sh = 0; shadow: //!
			
	auto hr = 
	pd3Dd9->SetRenderTarget(0,ms?ms:rt);
	pd3Dd9->SetVertexShader(vs[sh?1:0]);
	pd3Dd9->SetPixelShader(ps[sh?1:0]);

	int sqx,sqy;
	int xmax = 0, ymax = 0;
	int yinv[4] = {};
	float animation_step;
	float strip[4][9] = {};	
	float xr,zr,yr, xc,zc,yc;
	float mmx,mmy[2],mmz;
	float mm[6] = {1000,-1000,1000,-1000,1000,-1000};
	auto calc_margins = [&]()
	{
		for(int i=6;i-->0;) mm[i] = i&1?-10000:10000;
		auto &chans = mdo::channels(img);
		for(int i=chans.count;i-->0;)
		{		
			auto &ch = chans[i]; 
			auto *verts = mdo::tnlvertexpackets(img,i);

			for(int i=ch.vertcount;i-->0;)
			{
				auto *p = verts[i].pos;
					
				mm[0] = std::min(mm[0],p[0]);
				mm[1] = std::max(mm[1],p[0]);
				mm[2] = std::min(mm[2],p[1]);
				mm[3] = std::max(mm[3],p[1]);
				mm[4] = std::min(mm[4],p[2]);
				mm[5] = std::max(mm[5],p[2]);
			}
		}		
	};
	int symmetry = sh&&!anims.empty()?COLORPLANES:1; //REPURPOSING
	for(int i,v=0;v<symmetry;v++,fr++)
	{
		sqx = sqy = sq;

		float marginx = 0.5f/(sq/2.0f); //...
		float marginy = marginx;
		calc_margins(); 
		xr = (mm[1]-mm[0])/2;
		xc = (mm[1]+mm[0])/2;
		yr = (mm[3]-mm[2])/2;
		yc = (mm[3]+mm[2])/2;
		zr = (mm[5]-mm[4])/2;
		zc = (mm[5]+mm[4])/2;
		mmx = std::max(fabsf(mm[0]),fabsf(mm[1]));
		mmy[0] = mm[2];
		mmy[1] = mm[3];
		mmz = std::max(fabsf(mm[4]),fabsf(mm[5]));	
		if(sh) 
		{
			float dx = mm[1]-mm[0];
			float dy = mm[5]-mm[4];

			float s = std::max(dx,dy)/sh_sq;
			sq = (int)(sh_sq/2*X2MDL_SHADOWS_MAX);			
			if(sq>X2MDL_SHADOWS_MAX)
			s*=X2MDL_SHADOWS_MAX/(float)sq;
			sq*=s;
			if(sq%2) sq++;

			if(dy<dx)
			{
				sqx = sq;
				sqy = (int)(sq*(dy/dx));
				if(sqy%2) sqy++;
			}
			else 
			{
				sqy = sq;
				sqx = (int)(sq*(dx/dy));
				if(sqx%2) sqx++;

				//HACK: strip needs sizeof(strip)
				//pixels on the last row
				sqx = std::max(60,sqx);
			}			
			marginx = 0.5f/(sqx/2.0f); 
			marginy = 0.5f/(sqy/2.0f);
			
			//this is put into the projection
			//matrix to try to make it easier
			//to compute variable size images
			xr*=1+X2ICO_SHADOW_EXPAND;
		//	xr+=marginx*4;
			zr*=1+X2ICO_SHADOW_EXPAND;
		//	zr+=marginy*4;

			//unused?
			mm[0] = -xr+xc; mm[1] = +xr+xc;
			mm[4] = -zr+zc; mm[5] = +zr+zc;
		}
		else //aspect ratio?
		{
			float dy = mmy[1]-mmy[0];
			float dx = mmx*2;

			if(dy<dx)
			{
				if(mmy[1]>0)
				{
					mmy[1] = mmy[0]+dx;
				}
				else
				{
					mmy[0] = mmy[1]-dx;
				}

				marginx*=dx/2;
				marginy*=dx/2;
					
				if(dx/dy>=2) //optimizing?
				{
					sqy/=2; mmz/=2;
				}
			}
			else 
			{
				dy*=1+marginx; //heads are cutoff???

				mmx = dy/2;

				if(mmy[1]>0)
				{
					mmy[1] = mmy[0]+dy;
				}
				else
				{
					mmy[0] = mmy[1]-dy;
				}

				marginx*=dy/2;
				marginy*=dy/2;

				if(dy/dx>=2) //optimizing?
				{
					sqx/=2; mmx/=2;
				}
			}
		}
		sqx+=4*2; //marginx*8?
		sqx+=4*2; //marginy*8?
		xmax = std::max(xmax,sqx);
		ymax = std::max(ymax,sqy);
		yinv[v] = sqy;

		//note, advance() uses nearest frame
		int time = animator.cc().time_rounded;
		strip[v][0] = (float)time;
		strip[v][1] = xc; strip[v][2] = xr;
		strip[v][3] = yc; strip[v][4] = yr;
		strip[v][5] = zc; strip[v][6] = zr; //mdo
		//I can't seem to reliably compute 
		//these from the bounding box data
		strip[v][7] = sqx/2; 
		strip[v][8] = sqy/2;

		float psc[4] = {}, psi[4] = {};

		if(sh) //shadows: convert to shadow space?
		{
			psc[0] = sqrtf(mmx*mmx+mmz*mmx);
			psc[0] = 1/std::max(psc[0],mmy[1]-mmy[0]);
		//	psc[1] = 1/psc[0];
			psc[1] = mmy[1];			
			psc[2] = xc;
			psc[3] = zc;

			int cc = ~v&1?v?0:2:v; //swap blue/red for ico format

			if(v==0)
			pd3Dd9->Clear(0,0,D3DCLEAR_TARGET,~0,0,0);
			psi[0] = cc==0;
			psi[1] = cc==1;
			psi[2] = cc==2;
			psi[3] = cc==3;
			pd3Dd9->SetRenderState(D3DRS_COLORWRITEENABLE,1<<cc);
		}
		else pd3Dd9->Clear(0,0,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0x00ffffff,1.0f,0);

		//NOTE: SetRenderTarget resets the viewport 
		D3DVIEWPORT9 vp = {0,0,sqx,sqy,0,1};
		pd3Dd9->SetViewport(&vp);

		D3DXMATRIX mat,prj;
		if(sh)
		{
			//float xw = xr*(1+X2ICO_SHADOW_EXPAND)+marginx*8;
			//float zh = zr*(1+X2ICO_SHADOW_EXPAND)+marginy*8;
			float xw = xr+marginx*8;
			float zh = zr+marginy*8;

			//D3DXMatrixOrthoOffCenterLH(&prj,mm[0],mm[1],mm[5],mm[4],25,-25);
			D3DXMatrixOrthoLH(&prj,xw*2,zh*2,25,-25);
			const float M_Pi_2 = 1.57079632679489661923f; // pi/2
			D3DXMatrixRotationYawPitchRoll(&mat,M_Pi_2*2,-M_Pi_2,0);
			mat._41 = xc;
			mat._42 = -zc;
		}
		else
		{
			D3DXMatrixOrthoOffCenterLH(&prj,-mmx,mmx,mmy[0],mmy[1],25,-25);
			D3DXMatrixIdentity(&mat);
		}

		if(1)
		{
			//these were because the tile didn't
			//get covered before
			//this throws the shadow off-center in game
		//	mat._41-=marginx; 
		//	mat._42+=marginy; //HACK?
		}
		D3DXMATRIX mvp = mat*prj;

		pd3Dd9->SetVertexShaderConstantF(0,(float*)mvp,4);
		pd3Dd9->SetVertexShaderConstantF(4,(float*)psc,1); //ymax
		pd3Dd9->SetPixelShaderConstantF(3,(float*)psi,1); //shadow plane

		pd3Dd9->BeginScene();

		int passes = 1;
		for(int pass=1;pass<=passes;pass++)
		{
			if(0&&pass==2)
			{
				mat._43+=0.01f; //transparency?

				mvp = mat*prj;

				pd3Dd9->SetVertexShaderConstantF(0,(float*)&mvp,4);
			}

			int texture = -2; 
			int material = -2;
			bool keep = true;
			auto &chans = mdo::channels(img);
			for(i=chans.count;i-->0;)
			{
				auto &ch = chans[i]; 

				//NOTE: it really looks like SOM_MAP
				//calls SetTexture for every polygon
				if(texture!=ch.texnumber)
				{
					texture = (DWORD)ch.texnumber; //texture

					if((unsigned)texture>=icotextures.size())
					{
						assert(0); continue;
					}
					
					hr = pd3Dd9->SetTexture(0,icotextures[texture]);
				}

				extern int8_t x2mdo_rmatindex2[33];

				if(material!=x2mdo_rmatindex2[ch.matnumber])
				{
					material = x2mdo_rmatindex2[ch.matnumber];

					if((unsigned)material>=33) 
					{
						assert(0); continue;
					}
								
					extern float materials[][4],materials2[][4];

					//if(pass>=1) //TODO: accumulate?
					{
						//ignoring alpha?
						float alpha = materials[material][3];

						//FORCE FOR MULTISAMPLE (see D3DRS_ALPHABLENDENABLE note)
						if(alpha!=1) 
						{
							//slight white alpha?
							//materials[material][3] = 1;
							materials[material][3] = sh?alpha:alpha+(1-alpha)*0.75f;							
						}
						keep = (pass==1)==(alpha>=1); 

						if(1&&!keep)
						{
							passes = 2; //OPTIMIZING? //ILLUSTRATING
						}
						else
						{	
							pd3Dd9->SetPixelShaderConstantF(1,materials[material],1);
							pd3Dd9->SetPixelShaderConstantF(2,materials2[material],1);
						}

						materials[material][3] = alpha;
					}
				}
			
				if(!keep) continue;
				
				auto *verts = mdo::tnlvertexpackets(img,i);
				auto *trips = mdo::tnlvertextriples(img,i);

				pd3Dd9->SetFVF(D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1); //0x112

				pd3Dd9->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST,
				0,ch.vertcount,ch.ndexcount/3,trips,D3DFMT_INDEX16,verts,8*sizeof(float));
			}
		}

		pd3Dd9->EndScene(); skip:

		auto &cc = animator.cc();
		auto &anim = const_cast<int&>(cc.anim); //YUCK
		if(!cc.anims.empty())
		if(!sh||cc.time_rounded>=mdl::animtime(jmg,anims[anim]))
		{
			static const float delta = 3; //frames at 30 fps

			if(!sh)
			{
				//HACK: pass to caculate bounds				
				while(++anim<(int)anims.size()) //DUPLICATE
				{						
					animator.begin(anim);
				//	int t = anims[anim]->time;
					int t = mdl::animtime(jmg,anims[anim]);
					int frames = (int)std::max(1.0f,t/delta);
		
					calc_margins();
					sh_sq = std::max(sh_sq,mm[1]-mm[0]);
					sh_sq = std::max(sh_sq,mm[5]-mm[4]);

					animation_step = (float)t/frames;
					for(fr++;cc.time_rounded<t;fr++)
					{	
						animator.advance(animation_step);

						calc_margins();
						sh_sq = std::max(sh_sq,mm[1]-mm[0]);
						sh_sq = std::max(sh_sq,mm[5]-mm[4]);
					}
				}
				anim = -1; fr = 0; //debugging
			}

			if(++anim<(int)anims.size()&&MULTIPLANES)
			{						
				animator.begin(anim);
				int t = mdl::animtime(jmg,anims[anim]);
				int frames = (int)std::max(1.0f,t/delta);
		
				animation_step = (float)t/frames;
			}
			else break; //!
		}
		else if(MULTIPLANES)
		{
			animator.advance(animation_step);
		}
	}
	pd3Dd9->SetRenderState(D3DRS_COLORWRITEENABLE,0xf);
		
	//https://forums.codeguru.com/showthread.php?441251-CBitmap-to-HICON-or-HICON-from-HBITMAP
//	extern HICON HICONFromCBitmap(HBITMAP bitmap);
	//http://www.catch22.net/tuts/sysimg.asp
	extern BOOL SaveIcon(TCHAR *szIconFile, HBITMAP hIcon[], int nNumIcons);

	//NOTE: has a black border problem with MSM drawing
	if(ms) pd3Dd9->StretchRect(ms,0,rt,0,D3DTEXF_POINT);

	sqx = xmax; sqy = ymax; //!!
	RECT r = {0,0,sqx,sqy};
	D3DLOCKED_RECT lock;
	hr = pd3Dd9->GetRenderTargetData(rt,rs);
	hr = rs->LockRect(&lock,&r,D3DLOCK_READONLY);

	BOOL o = 0; 
	if(0&&sq>25) //WARNING: this is really a BMP file (it happens to work)
	{
		rs->UnlockRect();

		//wcscpy(PathFindExtensionW(in),L".ico");	
		//o = !D3DXSaveSurfaceToFileW(in,D3DXIFF_BMP,rs,0,&r); //rt
		wcscpy(ext,L"_big.png");	
		o = !D3DXSaveSurfaceToFileW(in,D3DXIFF_PNG,rs,0,&r); //rt
	}
	else if(0&&sh)
	{
		rs->UnlockRect();

		wcscpy(ext,L"_kage.png");	
		o = !D3DXSaveSurfaceToFileW(in,D3DXIFF_PNG,rs,0,&r); //rt
	}
	else if(sqx*sqy)//??? this was working but stopped???
	{
		//this dc is special and only GetCurrentObject
		//works. BitBlt can't use it as a source. SelectObject
		//doesn't work
		//HDC dc;
		//if(!rt->GetDC(&dc)) //fails (unfinished)
		{
		//	HBITMAP bm = (HBITMAP)GetCurrentObject(dc,OBJ_BITMAP);

			//lock.pBits is unaffected by r
		//	DWORD *buf = new DWORD[sq*sq*2], *buf2 = buf+sq*sq;			
			DWORD *buf; if(sh) 
			{
				buf = new DWORD[sqx*sqy];
				memset(buf,0xff,sizeof(DWORD)*sqx*sqy);

				//YUCK: shift to bottom because ico is upside down?
				for(int i=sqy;i-->0;)
				{
					int ii[4];
					for(int k=4;k-->0;)
					ii[k] = i-(ymax-yinv[k]);
					BYTE *pp = (BYTE*)&buf[i*sqx];
					for(int jN=sqx*4,j=0;j<jN;j++) if(ii[j%4]>=0)
					{						
						pp[j] = ((BYTE*)lock.pBits)[lock.Pitch*ii[j%4]+j];
					}
				}
			}
			else
			{
				int sq2 = std::max(sqx,sqy);
				buf = new DWORD[sq2*sq2];
				memset(buf,0xff,sizeof(DWORD)*sq2*sq2);
				int mx = (sq2-sqx)/2;
				int my = (sq2-sqy)/2;
				DWORD *p = buf+sq2*my+mx;
				for(int i=0;i<sqy;i++,p+=sq2)			
				memcpy(p,(char*)lock.pBits+lock.Pitch*i,sqx*4);

				sqx = sqy = sq2;
			}

			enum{ preprocess=1 };

			if(sh&&preprocess) //blur pass 
			{
				//memset(buf,0xff,4*sqx*sqy); //2 passes

				//first pass, writing back into lock.pBits
				//this pass blurs the white region so it isn't
				//a sharp cutoff, the blur is some constant amount
				for(int i=sqy;i-->0;) for(int j=sqx;j-->0;)
				{	
					DWORD *dst = (DWORD*)((BYTE*)lock.pBits+lock.Pitch*i+j*4);
					DWORD *src = buf+sqx*i+j;

					constexpr int ir = 4;
					constexpr int irr = ir*ir;
					constexpr int len = ir+1+ir;
					const int pitch = sqx;
					const int mid = pitch*ir+ir;					
					DWORD *pp = src-mid;
					DWORD *dd = src+1+mid;

					for(int cp=0;cp<COLORPLANES;cp++)
					{
						float s = 0; int total = 0;

						for(int k=len;k-->0;)
						{
							DWORD *p = pp+k*pitch+len-1;

							BYTE *c = (BYTE*)p+cp; //color plane?

							for(int l=len;l-->0;c-=4) //p--
							{
								int kk = k-ir;
								int ll = l-ir;
								if(kk*kk+ll*ll<=irr)
								{
									total++;

									kk+=i; ll+=j;
									if(ll<0||kk<0||ll>=sqx||kk>=sqy)
									{
										s+=1; //white
									}
									else if(255==*c)
									{
										s+=1; //white
									}
									else
									{
										//darkening and reducing contrast to something
										//resembling the original shadows

										//NOTE: this new formula is for D3DBLENDOP_MIN
										//it's just approximating the old one...
										//its math isn't identical to it
										if(MULTIPLANES)
										s+=0.5f+(powf(*c*l_255,0.25))*0.4f; //new?
										else
										s+=0.5f+(1-powf(*c*l_255,0.75))*0.4f; //old
									}
								}
							}
						}
						
						float ss = s/(float)total; 
					
						((BYTE*)dst)[cp] = total?(BYTE)(255.0f*ss):255;
					}
				}
				//2nd pass (SAME BASIC ALGORITHM)
				for(int i=sqy;i-->0;) for(int j=sqx;j-->0;)
				{	
					DWORD *src = (DWORD*)((BYTE*)lock.pBits+lock.Pitch*i+j*4);
					DWORD *dst = buf+sqx*i+j;

					for(int cp=0;cp<COLORPLANES;cp++)
					{
						BYTE blur = ((BYTE*)src)[cp]; //2 passes

						if(blur==255)
						{
							((BYTE*)dst)[cp] = blur; continue; //255
						}

						int sv = ((BYTE*)dst)[cp]; //src //2 passes

					//	float r = 1-pow(sv*l_255,2); //0?
						float r = sqrtf(1-sv*l_255)*(1-blur*l_255);
						//r = 8*(0.2+0.8*r); 
						//r = 8; //looks like classic shadow (first pass?)
						r = 10*r;

						int ir = (int)(r);
						int irr = (int)(r*r);
						int pitch = lock.Pitch/4;
						int mid = pitch*ir+ir;
						int len = ir+1+ir;
						DWORD *pp = src-mid;
						DWORD *dd = src+1+mid;

						float s = 0; int total = 0;

						for(int k=len;k-->0;)
						{
							DWORD *p = pp+k*pitch+len-1;

							BYTE *c = (BYTE*)p+cp; //color plane?

							for(int l=len;l-->0;c-=4) //p--
							{
								int kk = k-ir;
								int ll = l-ir;
								if(kk*kk+ll*ll<=irr)
								{
									total++;

									kk+=i; ll+=j;
									if(ll<0||kk<0||ll>=sqx||kk>=sqy)
									{
										s+=1; //white
									}
									else if(255==*c)
									{
										s+=1; //white
									}
									else
									{
										s+=*c*l_255;
									}
								}
							}
						}
						
						float ss = s/(float)total; 
					
						if(1) if(0)
						{
							//ss*=ss;
							ss = powf(ss,1.5f);
						}
						else if(1) if(ss<1)
						{
							ss = ss*ss;
						//	float contrast = 1.5f;
						//	float l = ss-0.5f;
						//	l = 0.5f+l*contrast;
						//	ss = l;
							ss = std::max(0.0f,ss);
							ss = std::min(1.0f,ss);
						}
						else assert(ss==1);

						((BYTE*)dst)[cp] = total?(BYTE)(255.0f*ss):255;
					}
				}
			}

			HDC dc = GetDC(0); int half = sh?2:1;
			HBITMAP bm = CreateCompatibleBitmap(dc,sqx/half,sqy/half);
			
			//REMINDER: this is giving priority to the 
			//"colorize" images because they're big and
			//maybe because they're second in the icon
			//list/array
			HBITMAP bm2 = 0; if(0&&!sh) //preview (pixel art style)
			{
			  //NOTE: I'M RENABLING THIS BECAUSE EXPLORER TURNS RECTANGLE//
			  //ICONS INTO SQUARE ICONS WHEN SEEN THROUGH A SHORTCUT ICON//

				int sq2 = std::max(sqx,sqy)*2;
				int mx = (sq2-sqx*2)/2;
				int my = (sq2-sqy*2)/2;
				BYTE *buf2 = new BYTE[4*sq2*sq2];
				memset(buf2,0xff,4*sq2*sq2);
				BYTE *p = buf2+4*sq2*my;
				for(int i=0;i<sqy;i++)
				{
					BYTE *q = (BYTE*)(buf+sqx*i);

					p+=4*mx;
					for(int j=sqx;j-->0;p+=8,q+=4)
					{
						p[0] = q[0];
						p[1] = q[1];
						p[2] = q[2];
						p[3] = 255;
						p[4] = p[0];
						p[5] = p[1];
						p[6] = p[2];
						p[7] = 255;

						//if(colorize) //colorizable?
						{
							//leaving details to SOM_MAP
							static const float lum[3] = { 0.00087058632f,0.00277254292f,0.00027843076f};
							float l =
							q[2]*lum[0]+ //R=0.222
							q[1]*lum[1]+ //G=0.707
							q[0]*lum[2]; //B=0.071
							q[0] = q[1] = q[2] = l*255;
						}
					}
					p+=4*mx;
					memcpy(p,p-4*sq2,4*sq2);
					p+=4*sq2;
				}

				bm2 = CreateCompatibleBitmap(dc,sq2,sq2);
				BOOL _ = SetBitmapBits(bm2,4*sq2*sq2,buf2); 

				delete[] buf2;
			}

			if(sh)
			{
				//downsampling makes a smaller footprint
				//and removes some glitches
				auto *buf2 = new DWORD[sqx*sqy/4];
				BYTE *p = (BYTE*)buf2;
				BYTE *a = (BYTE*)buf;
				for(int i=sqy/2;i-->0;a+=sqx*4)
				for(int j=sqx/2;j-->0;p+=4,a+=8)
				{
					BYTE *b = a+4;
					BYTE *c = a+4*sqx;
					BYTE *d = c+4;
					for(int k=4;k-->0;)
					p[k] = (BYTE)((a[k]+b[k]+c[k]+d[k])/4);
				}

				DWORD *b = buf2+sqx/2*(sqy/2-1); //upside-down?
				
				if(anims.empty()) //old way //remove me
				{
					(float&)b[0] = xc; (float&)b[1] = xr;
					(float&)b[2] = yc; (float&)b[3] = yr;
					(float&)b[4] = zc; (float&)b[5] = zr; //mdo*/
				}
				else memcpy(b,strip,COLORPLANES*sizeof(*strip)); //NEW

				SetBitmapBits(bm,sqx*sqy,buf2);

				delete[] buf2;
			}
			else SetBitmapBits(bm,4*sqx*sqy,buf);

			rs->UnlockRect();

			delete[] buf; 

			if(!sh||anims.empty())
			{
				//HICON i[2] = {HICONFromCBitmap(bm),bm2?HICONFromCBitmap(bm2):0};
				HBITMAP i[2] = {bm,bm2};
				if(bm2) std::swap(i[0],i[1]); //Windows Explorer
			
			//	wcscpy(ext,L"_kage.ico"+(sh?0:5));
				wcscpy(ext,L".ico");
				o = SaveIcon(in,i,bm2?2:1);	
				DeleteObject(i[0]); //DestroyIcon

				if(i[1]) DeleteObject(i[1]); //DestroyIcon
			}
			else kage.push_back(bm);

			//rt->ReleaseDC(dc); 
		}
	//	else assert(0);
	}

	if(!anims.empty())
	{
		//multisample does z-buffer or blending wrong???
		//it's probably not right to aa the height data
		//anyway
		//FIX ME? NEED THIS FOR COMPATIBLE DEPTH BUFFER
		//ms = 0;	
		if(!sh&&MULTIPLANES) //YUCK: reopen bind-pose if necessary
		{
		//	ms = 0;
			
			pd3Dd9->SetDepthStencilSurface(0);

			pd3Dd9->SetRenderState(D3DRS_ZENABLE,0);
			pd3Dd9->SetRenderState(D3DRS_ZWRITEENABLE,0);
			pd3Dd9->SetRenderState(D3DRS_ALPHABLENDENABLE,1); 
			pd3Dd9->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_MIN);
			pd3Dd9->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCCOLOR);
			pd3Dd9->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_DESTCOLOR);

			if(COLORPLANES==4)
			{
				pd3Dd9->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,1); 
				pd3Dd9->SetRenderState(D3DRS_BLENDOPALPHA,D3DBLENDOP_MIN);
				pd3Dd9->SetRenderState(D3DRS_SRCBLENDALPHA,D3DBLEND_SRCCOLOR);
				pd3Dd9->SetRenderState(D3DRS_DESTBLENDALPHA,D3DBLEND_DESTCOLOR);
			}
		}

		//TODO: XZ rotated items and objects will need
		//their shadows generated at runtime	
		if(animator.cc().anim<(int)anims.size())
		{
			sh++;

			if(MULTIPLANES||sh<2) //debugging
				
				goto shadow; //limiting to NPCs for now 
		}
	}

	pd3Dd9->SetDepthStencilSurface(ds);
	pd3Dd9->SetRenderState(D3DRS_ZENABLE,1);
	pd3Dd9->SetRenderState(D3DRS_ZWRITEENABLE,1);
	pd3Dd9->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,0); 
	pd3Dd9->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);
	pd3Dd9->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	pd3Dd9->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);

	if(!kage.empty())
	{
		wcscpy(ext,L"_kage.ico");	
		o = SaveIcon(in,kage.data(),(int)kage.size());	

		for(auto&bm:kage) DeleteObject(bm);
	}
	
	return o;
}

//SOURCE
//http://www.catch22.net/tuts/sysimg.asp
//http://www.catch22.net/tuts/zips/sysimg.zip
//https://groups.google.com/g/comp.os.ms-windows.programmer.win32/c/vRpGcRldrAk?pli=1
//WriteIconData needed &3 edit

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
static BOOL GetBitmapInfo(HBITMAP hIcon, ICONINFO *pIconInfo, BITMAP *pbmpColor, BITMAP *pbmpMask)
{
	//if(!GetIconInfo(hIcon, pIconInfo))
		//return FALSE;
	memset(pIconInfo,0x00,sizeof(*pIconInfo));
	pIconInfo->fIcon = true;
	pIconInfo->hbmColor = hIcon;
	
	if(!GetObject(pIconInfo->hbmColor, sizeof(BITMAP), pbmpColor))
		return FALSE;

	pIconInfo->hbmMask = CreateBitmap(pbmpColor->bmWidth,pbmpColor->bmHeight,1,1,0);

	if(!GetObject(pIconInfo->hbmMask,  sizeof(BITMAP), pbmpMask))
		return FALSE;

	return TRUE;
}
//
//	Write one icon directory entry - specify the index of the image
//
static UINT WriteIconDirectoryEntry(HANDLE hFile, int nIdx, HBITMAP hIcon, UINT nImageOffset)
{
	ICONINFO	iconInfo;
	ICONDIR		iconDir;

	BITMAP		bmpColor;
	BITMAP		bmpMask;

	DWORD		nWritten;
	DWORD		nColorCount;
	DWORD		nImageBytes;

	//GetIconBitmapInfo(hIcon, &iconInfo, &bmpColor, &bmpMask);
	GetBitmapInfo(hIcon, &iconInfo, &bmpColor, &bmpMask);
		
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
	//DeleteObject(iconInfo.hbmColor);
	assert(hIcon==iconInfo.hbmColor);
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
			WriteFile(hFile, &padding, 4 - bmp.bmWidthBytes&3, &nWritten, 0);
		}
	}

	free(pIconData);

	return nBitmapBytes;
}

//
//	Create a .ICO file, using the specified array of HICON images
//
BOOL SaveIcon(TCHAR *szIconFile, HBITMAP hIcon[], int nNumIcons)
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
		
	//	GetIconBitmapInfo(hIcon[i], &iconInfo, &bmpColor, &bmpMask);
		GetBitmapInfo(hIcon[i], &iconInfo, &bmpColor, &bmpMask);

		// record the file-offset of the icon image for when we write the icon directories
		pImageOffset[i] = SetFilePointer(hFile, 0, 0, FILE_CURRENT);
		
		// bitmapinfoheader + colortable
		WriteIconImageHeader(hFile, &bmpColor, &bmpMask);
		
		// color and mask bitmaps
		WriteIconData(hFile, iconInfo.hbmColor);
		WriteIconData(hFile, iconInfo.hbmMask);

		//DeleteObject(iconInfo.hbmColor);
		assert(hIcon[i]==iconInfo.hbmColor);
		DeleteObject(iconInfo.hbmMask);
	}

	// 
	//	Lastly, skip back and write the icon directories.
	//
	SetFilePointer(hFile, sizeof(ICONHEADER), 0, FILE_BEGIN);

	for(i = 0; i < nNumIcons; i++)
	{
		//WriteIconDirectoryEntry(hFile, i, hIcon[i], pImageOffset[i]);
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

////////////////////////////////////////////////////

void x2ico::animator::_init()
{
	if(cb!=0) clear(); //cp

	anim = -1;

	cb = vb = 0; mats = 0; knowns = 0;

	mdl::header_t &head = mdl::imageheader(in);
	mdl::header_t hd = {}; 	
	if(head) memcpy(&hd,&head,sizeof(hd));

	int n = hd.hardanims+hd.softanims;
	anims.resize(n);
	mdl::animations(in,anims.data(),n);

	int hardchans = mdl::animationchannels<mdl::hardanim_t>(in);
	int softchans = mdl::animationchannels<mdl::softanim_t>(in);

	if(hardchans)
	{	
		hard_chs.resize(hardchans);

		if(hardchans==mdl::mapanimationchannels(in,hard_chs.data(),hardchans))
		{			
			mats = new float[hardchans*16]; knowns = new bool[hardchans];
		}
		else assert(0);
	}
	
	cbsz = 0;
	for(int i=0;i<hd.primchans;i++)
	{
		auto &pc = mdl::primitivechannel(in,i);

		pc._unknown0 = cbsz; //REPURPOSING #1

		cbsz+=3*pc.vertcount;
	}
	if(cbsz)
	{
		if(softchans) vb = new float[cbsz];
	
		if(mats||!vb)
		{
			cb = new float[cbsz];
		}
		else cb = vb;
	}
}
void x2ico::animator::begin(int a)
{
	if(!cb) _init();

	if(vb&&(unsigned)a<anims.size())
	{
		soft_fr = anims[a]->frames;

		mdl::header_t &hd = mdl::imageheader(in);
		int n = hd.primchans;

		for(int j=0,k=0;k<n;k++)
		{
			auto &pc = mdl::primitivechannel(in,k);
			auto *vp = mdl::pervertexlocation(in,k);
			for(int n=pc.vertcount,i=0;i<n;i++,j+=3)
			{
				vb[j+0] = vp[i].x;
				vb[j+1] = vp[i].y;
				vb[j+2] = vp[i].z;
			}
		}
	}

	anim = a; 	
	time = 0.0f; 
	time_rounded = 0;
	time_remaining = 0;
	
	advance(0); //show initial pose?
}
void x2ico::animator::clear()
{
	anims.clear(); hard_chs.clear();

	delete[] knowns; knowns = 0;
	delete[] mats; mats = 0;

	if(cb==vb) cb = 0;
	delete[] vb; cbsz = 0;
	delete[] cb; cb = 0; anim = -1; 
}
void x2ico::animator::advance(float step)
{
	if(anims.empty()) return;

	if(anim==-1) begin(0);

	time+=step;
	int top = time_rounded;
	int stop = (int)(time+0.5f);
	if(!stop) stop = 1;
	int end = mdl::animtime(in,anims[anim]);	
	stop = std::min(stop,end);
	assert(!top&&!stop||top<stop);
   			
	mdl::header_t &hd = mdl::imageheader(in);

	int m = (int)hard_chs.size(), n = hd.primchans;

	if(vb) //softchans?
	{	
		mdl::softanimframe_t *p = soft_fr;

		for(int t=top;t<stop;)
		{
			auto &sac = mdl::softanimchannel(in,p);

			int tr = time_remaining;
			int pt = tr?tr:p->time;

			if(t+pt>stop)
			{
				tr = pt;
				pt = stop-t;
				tr-=pt;
			}
			else tr = 0;

			mdl::accumulate(&sac,vb,cbsz,3,(float)pt/p->time);
				
			t+=pt; time_remaining = tr;

			if(tr) break; //begin?
			
			//hop over 0 terminator/next ID
			if(!*++p) p+=2;
		}

		soft_fr = p;

		assert(mats||cb==vb); //memcpy(cb,vb);
	}
	
	if(mats) //hardchans?
	{
		mdl::hardanim_t *p = hard_chs.data();

		int now = !anim&&top?top+1:top;
		int to = std::max(stop,1+!anim);
		mdl::animate(anims[anim],p,m,now,!anim,to-top);

		/*if(0) //can't do weights here anymore
		{
			memset(knowns,0x00,sizeof(bool)*m);

			for(int j=0,k=0;k<n;k++)
			{
				mdl::transform(p,k,0,0,mats,knowns);
			
				auto &pc = mdl::primitivechannel(in,k);
				auto *vp = mdl::pervertexlocation(in,k);

				for(int n=pc.vertcount,i=0;i<n;i++,j+=3)
				{
					float v[3];

					v[0] = vp[i].x;
					v[1] = vp[i].y;
					v[2] = vp[i].z;

					if(vb)
					{
						v[0]+=vb[j+0];
						v[1]+=vb[j+1];
						v[2]+=vb[j+2];
					}
				
					mdl::multiply(mats+16*k,v,v);
					cb[j+0] = (int16_t)v[0];
					cb[j+1] = (int16_t)v[1];
					cb[j+2] = (int16_t)v[2];
				}
			}
		}*/
	}
	if(mats)
	{
		mdl::hardanim_t *p = hard_chs.data();

		memset(knowns,0x00,sizeof(bool)*m);
			
		for(int j=0,k=0;k<m;k++) //n
		mdl::transform(p,k,0,0,mats,knowns);
	}
	for(int j=0,k=0;k<n;k++)
	{
		auto &pc = mdl::primitivechannel(in,k);
		auto *vp = mdl::pervertexlocation(in,k);
		for(int n=pc.vertcount,i=0;i<n;i++,j+=3)
		{
			cb[j+0] = vb?vb[j+0]:vp[i].x;
			cb[j+1] = vb?vb[j+1]:vp[i].y;
			cb[j+2] = vb?vb[j+2]:vp[i].z;
		}
	}
   
	const float l_1024 = 1/1024.0f;

	void *hd2 = out.header<void>();
	auto &chans = mdo::channels(out);
		
	for(int vo=0,n=chans.count,i=0;i<n;i++)
	{
		auto &ch = chans[i]; 
		auto &ext = ch.extra();
		auto &pc = mdl::primitivechannel(in,ext.part);
		WORD *pi = (WORD*)((DWORD)hd2+ext.part_index);
		auto *vp = mdo::tnlvertexpackets(out,i);

		int jN = ch.vertcount;

		auto *cbp = cb+pc._unknown0; //REPURPOSING #2

		/*
		for(int j=0;j<jN;j++)
		{
			float *p = vp[j].pos;
			float *q = &cbp[3*pi[j]];
			p[0] = +l_1024*q[0];
			p[1] = -l_1024*q[1];
			p[2] = +l_1024*q[2];
		}*/

		int k = ext.part;

		if(ext.skin&1&&wt)
		{
			for(int j=0;j<jN;j++)
			{
				float *p = vp[j].pos;
				memset(p,0x00,3*sizeof(float));
			}
		}
		else for(int j=0;j<jN;j++)
		{
			float *p = vp[j].pos;
			float *q = &cbp[3*pi[j]];
			if(mats) 
			mdl::multiply(mats+16*k,q,p);
			else
			memcpy(p,q,3*sizeof(float));
			p[0]*=+l_1024;
			p[1]*=-l_1024;
			p[2]*=+l_1024;
		}
	}
	if(wt&&mats)
	{
		void *e = (BYTE*)wt+wt_sz;

		for(auto*w=wt;w<e;w=w->next())
		{
			auto &ch = chans[w->ch]; 
			auto &ext = ch.extra();			
			if(ext.skin&1)
			{
				WORD *pi = (WORD*)((DWORD)hd2+ext.part_index);
				auto *vp = mdo::tnlvertexpackets(out,w->ch);
				auto &pc = mdl::primitivechannel(in,ext.part);

				auto *cbp = cb+pc._unknown0;

				int jN = w->ct;
				int k = w->pt;

				float ww = w->wt/200.0f;

				for(int j=0;j<jN;j++)
				{
					int i = w->first[j];

					float *p = vp[i].pos;
					float *q = &cbp[3*pi[i]];
					float x[3];
					mdl::multiply(mats+16*k,q,x);

					p[0]+=ww*l_1024*x[0];
					p[1]-=ww*l_1024*x[1];
					p[2]+=ww*l_1024*x[2];					
				}
			}
			else assert(0);
		}
	}

	time_rounded = stop;
}