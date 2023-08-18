
#include "x2mdl.pch.h" //PCH

#include <vector> //REMOVE ME

#include <d3dx9shader.h>

//Added for snapshot transform
#include "../lib/swordofmoonlight.h"

#define HLSL2(x) #x
#define HLSL(...) HLSL2(__VA_ARGS__) "\n"
#define HLSL_DEFINE(x,...)\
"#define " #x " " HLSL2(__VA_ARGS__) "\n"

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

				pd3Dd9->SetRenderState(D3DRS_BLENDOPALPHA,D3DBLENDOP_MIN);
				hr = pd3Dd9->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ZERO);
				hr = pd3Dd9->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_ONE);
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

						extern float materials[][4],materials2[][4];

						float alpha = materials[texture][3];
										
						switch(pass)
						{
						case 1: //wall marker?
					
							//NOTE: the rasterizer may vary alpha? 253 is normal
							materials[texture][3] = 254/255.0f;
							break;
					
						case 2: case 3:
						
							materials[texture][3] = 1.0f;
							break;
						}

						if(pass>=3) //TODO: accumulate?
						{
							keep = (pass==3)==(alpha>=1); 

							if(1&&!keep) passes = 4; //OPTIMIZING? //ILLUSTRATING
						}

						if(keep) pd3Dd9->SetPixelShaderConstantF(1,materials[texture],1);
						if(keep) pd3Dd9->SetPixelShaderConstantF(2,materials2[texture],1);

						materials[texture][3] = alpha;
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
					float a = p[3]/255.0f;
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
				float c = sqrt(2.1f-p[3]/255.0f);

				for(int k=4;k-->0;)
				{
					float l = p[k]/255.0f-0.5f;
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
							float a = q[3]/255.0f;
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
			DeleteObject(i[0]);

			if(i[1]) DeleteObject(i[1]);

			//rt->ReleaseDC(dc); 
		}
	//	else assert(0);
	}
	
	return o;
}

static const char x2ico_mdo_vs[] = //HLSL
{	
	HLSL(
	float4x4 mvp:register(vs,c0);
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
	})
};
extern bool x2mdo_ico(int sq, WCHAR *in, IDirect3DDevice9 *pd3Dd9, IDirect3DSurface9 *surfs[3])
{
	auto *rt = surfs[0];
	auto *rs = surfs[1];
	auto *ms = surfs[2];

	assert(rt); if(!rt) return false;

	wcscpy(PathFindExtensionW(in),L".mdo");
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

	namespace mdo = SWORDOFMOONLIGHT::mdo;
	mdo::image_t img;
	mdo::maptorom(img,m,buf.size());

	auto &chans = mdo::channels(img);

	float margin = 0.5f/(sq/2.0f); //...
	float mmx = 0, mmy[2] = {+1000,-1000};
	{		
		for(i=chans.count;i-->0;)
		{		
			auto &ch = chans[i]; 
			auto *verts = mdo::tnlvertexpackets(img,i);

			for(int i=ch.vertcount;i-->0;)
			{
				auto *p = verts[i].pos;
					
			//	p[1] = -p[1]; p[4] = -p[4];
				p[2] = -p[2]; p[5] = -p[5]; //keep upright
		
				mmx = std::max(mmx,fabsf(p[0]));
				mmy[0] = std::min(mmy[0],p[1]);
				mmy[1] = std::max(mmy[1],p[1]);		
			}
		}

		//aspect ratio?
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

				margin*=dx/2;
			}
			else 
			{
				dy*=1+margin; //heads are cutoff???

				mmx = dy/2;

				if(mmy[1]>0)
				{
					mmy[1] = mmy[0]+dy;
				}
				else
				{
					mmy[0] = mmy[1]-dy;
				}

				margin*=dy/2;
			}
		}		
	}	

	auto hr = pd3Dd9->SetRenderTarget(0,ms?ms:rt);
	pd3Dd9->Clear(0,0,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0x00ffffff,1.0f,0);
	static IDirect3DPixelShader9 *ps[1] = {};
	static IDirect3DVertexShader9 *vs[1] = {};
	if(!vs[0]) for(int i=2;i-->0;)
	{
		//char main[2] = "f"; *main+=i;

		DWORD cflags =
		D3DXSHADER_AVOID_FLOW_CONTROL|D3DXSHADER_OPTIMIZATION_LEVEL3;
		LPD3DXBUFFER obj = 0, err = 0;
		if(i==1) //vs?
		{
			if(!D3DXCompileShader(x2ico_mdo_vs,sizeof(x2ico_mdo_vs)-1,0,0,"f","vs_3_0",cflags,&obj,&err,0))
			{
				pd3Dd9->CreateVertexShader((DWORD*)obj->GetBufferPointer(),&vs[0]); obj->Release();
			}
		}
		else if(!D3DXCompileShader(x2ico_mdo_ps,sizeof(x2ico_mdo_ps)-1,0,0,"f","ps_3_0",cflags,&obj,&err,0))
		{
			pd3Dd9->CreatePixelShader((DWORD*)obj->GetBufferPointer(),&ps[0]); obj->Release();
		}
		if(err) //warning X3571: pow(f, e) will not work for negative f
		{
			char *e = (char*)err->GetBufferPointer(); 

			assert(!err||obj); err->Release(); //breakpoint
		}
	}
	pd3Dd9->SetVertexShader(vs[0]); pd3Dd9->SetPixelShader(ps[0]);

//	pd3Dd9->CreateStateBlock(D3DSBT_ALL); //caller is responsible

	pd3Dd9->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);

	//for(int v=0;v<symmetry;v++) //rotation symmetry?
	{
		//NOTE: SetRenderTarget resets the viewport 
		D3DVIEWPORT9 vp = {0,0,sq,sq,0,1};
		pd3Dd9->SetViewport(&vp);

		D3DXMATRIX mat,prj;
		D3DXMatrixOrthoOffCenterLH(&prj,-mmx,mmx,mmy[0],mmy[1],25,-25);
	//	const float M_Pi_2 = 1.57079632679489661923f; // pi/2
	//	D3DXMatrixRotationX(&mat,M_Pi_2);
		D3DXMatrixIdentity(&mat);
		if(1)
		{
			//these were because the tile didn't
			//get covered before
			mat._41 = -margin; 
			mat._42 = margin; //HACK?
		}
		D3DXMATRIX mvp = mat*prj;

		auto mirror = D3DCULL_CCW;
	
		pd3Dd9->SetVertexShaderConstantF(0,(float*)&mvp,4);

		pd3Dd9->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);
		pd3Dd9->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
		//pd3Dd9->SetSamplerState(0,D3DSAMP_MIPFILTER,D3DTEXF_ANISOTROPIC);
		//pd3Dd9->SetSamplerState(0,D3DSAMP_MAXANISOTROPY,16);
		
		pd3Dd9->BeginScene();

		pd3Dd9->SetRenderState(D3DRS_ZENABLE,1);
		pd3Dd9->SetRenderState(D3DRS_ZWRITEENABLE,1);
		pd3Dd9->SetRenderState(D3DRS_CULLMODE,mirror);

		//alpha looks funny (e.g. green slime)
		//NOTE: multisample forces alphablend! so
		//instead the material alpha is set to 1...
		pd3Dd9->SetRenderState(D3DRS_ALPHABLENDENABLE,0);		
		pd3Dd9->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);
		pd3Dd9->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
		pd3Dd9->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);

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
							materials[material][3] = alpha+(1-alpha)*0.75f;							
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

		pd3Dd9->EndScene();
	}
	
	//NOTE: has a black border problem with MSM drawing
	if(ms) pd3Dd9->StretchRect(ms,0,rt,0,D3DTEXF_POINT);

	RECT r = {0,0,sq,sq};
	D3DLOCKED_RECT lock;
	hr = pd3Dd9->GetRenderTargetData(rt,rs);
	hr = rs->LockRect(&lock,&r,D3DLOCK_READONLY);

	wchar_t icon[MAX_PATH];
	wcscpy(icon,in);		
	BOOL o = 0; 
	if(0&&sq>25) //WARNING: this is really a BMP file (it happens to work)
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
			//colorizing...
		//	BOOL _ = SetBitmapBits(bm,4*sq*sq,buf); //buf2
			//BITMAPINFO bmi = {sizeof(bmi)};
			//GetDIBits(dc,bm,0,sq,0,&bmi,0);
			//int _ = SetDIBits(dc,bm,0,sq,lock.pBits,&bmi,0);
			rs->UnlockRect();

			//REMINDER: this is giving priority to the 
			//"colorize" images because they're big and
			//maybe because they're second in the icon
			//list/array
			HBITMAP bm2 = 0; if(0) //preview (pixel art style)
			{
				int x2 = sq*2;

				BYTE *buf2 = new BYTE[4*x2*x2];
				BYTE *p = buf2;
				for(int i=0;i<sq;i++)
				{
					BYTE *q = (BYTE*)(buf+sq*i);

					for(int j=sq;j-->0;p+=8,q+=4)
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
					int stride = 4*x2;
					memcpy(p,p-stride,stride);
					p+=stride;
				}

				bm2 = CreateCompatibleBitmap(dc,x2,x2);
				BOOL _ = SetBitmapBits(bm2,4*x2*x2,buf2); 

				delete[] buf2;
			}

			//colorizing?
			BOOL _ = SetBitmapBits(bm,4*sq*sq,buf); //buf2

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
			DeleteObject(i[0]);

			if(i[1]) DeleteObject(i[1]);

			//rt->ReleaseDC(dc); 
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
			WriteFile(hFile, &padding, 4 - bmp.bmWidthBytes, &nWritten, 0);
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