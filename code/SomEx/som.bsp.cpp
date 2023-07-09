
#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

#include <vector>
#include <algorithm>

#include "dx.ddraw.h"

#include "Ex.ini.h"
#include "Ex.output.h"

#include "som.state.h"
#include "som.game.h" //som_scene_state
#include "som.status.h" //Versus

	//WARNING: there's a lot of unused code in this
	//file. I'm hesitant to chuck it out in case it
	//can be used down the road. problem is BSP won't
	//work because of do_aa, but it might work with
	//a local knot tying strategy to prevent cuts from
	//proliferating, but this should be encoded in
	//som_BSP (MDO) and will require object partitions

/*
struct som_BSP //EXPERIMENTAL
{
	//NOTE: tris is index count plus a padding word
	WORD size,tris,verts,cuts;

	WORD *data(){ assert(size>1); return &cuts+1; }
	bool empty(){ assert(size>=1); return size<=1; }

	//NOTE: size covers 2 words and may not be enough
	//som_BSP *next(){ return (som_BSP*)(&size+size*2); }
	som_BSP *next(){ return (som_BSP*)(&size+size*4); }

	struct cut
	{
		WORD index, vertex;
		bool operator<(cut &cmp)const{ return index<cmp.index; }
	};
	struct vert{ WORD a,b; float t; };	
	
	//this is useful for the "tnl" mode but also works
	//for limiting scans as it's the last uncounted cut
	//index & vertex are the final index & vertex counts
	cut &terminator(){ return ((cut*)next())[-1]; }
};*/
extern void som_bsp_make_mdo_instance(SOM::MDO *o) //som.MDL.cpp
{
	int decal = 0;

	auto *d = o->mdo_data();		       
//	som_BSP *b = d->ext.bsp;
	if(d->textures&&d->textures[0]) //HACK: exclude shadows?
	for(int i=0,iN=d->chunk_count;i<iN;i++)
	{
		auto &ch = d->chunks_ptr[i];

		if(ch.blendmode||d->materials_ptr[ch.matnumber][3]!=1.0f)
		{
			o->elements[i].sort = 1; //sort?

			decal|=1<<ch.matnumber;

			#ifdef NDEBUG
//			#error item menu overwrites bsp
			#endif			
//			if(b||!EX::debug) //DEBUGGING (breakpoint)
//			o->elements[i].bsp = b->empty()?0:b;
		}
//		else assert(!o->elements[i].bsp);

//		if(b) b = b->next();
	}

	if(decal&&d->material_count>1) //HACK: KF2 wall decal?
	{
		//TODO: an EXTENSION here would allow for
		//more flexibility

		int i; for(i=0;decal;decal>>=1) 
		{
			if(decal&1) i++; //see if blending 2 materials?
		}
		if(i>1)
		for(int i=0,iN=d->chunk_count;i<iN;i++)
		if(o->elements[i].sort)
		{
			auto &ch = d->chunks_ptr[i];

			int j = ch.vertcount; //check for flatness?
			while(j-->1)
			if(memcmp(ch.vb_ptr[0].lit,ch.vb_ptr[j].lit,3*sizeof(float)))
			break;

			if(!j) o->elements[i].sort = 2;
		}
	}
}

/////////////////////////////////////

enum //memory block sizes 
{
	#ifdef NDEBUG
//	#error may want to profile this
	#endif
	som_bsp_v_m = 2056,
	som_bsp_t_m = 1024,
	som_bsp_y_m = som_bsp_t_m/2,
};
struct som_bsp_m //memory
{
	BYTE *b,*p,*e; som_bsp_m *next;
		
	__declspec(noinline) BYTE *a(int,int);
	__forceinline BYTE *get(int M, int sz)
	{
		BYTE *o = p; p+=sz; return p<=e?o:a(M,sz);
	}

	void clear()
	{
		for(auto *m=this;m;m=m->next) m->p = m->b;
	}

	size_t size(int sz)
	{
		size_t o = (p-b)/sz; if(next) o+=next->size(sz);
		return o;
	}
};
som_bsp_m som_bsp_mem[3] = {}; //1 would work too?
__declspec(noinline)
BYTE *som_bsp_m::a(int M, int sz)
{
	assert(p>e);

	for(auto*m=this;m=m->next;)	
	if(m->p==m->b)
	{
		std::swap(b,m->b);
		std::swap(p,m->p);
		std::swap(e,m->e); return get(M,sz);
	}

	int n = M; if(b)
	{
		auto *m = new som_bsp_m(*this);
		next = m;
		do n+=M; while(m=m->next);
	}
	p = b = new BYTE[n]; e = b+n; return get(M,sz);
}
#define som_bsp_new(i,m,s,t) \
((t*)som_bsp_mem[i].get(m*sizeof(s),sizeof(t)))
#define new_som_bsp_v som_bsp_new(0,som_bsp_v_m,som_bsp_v,som_bsp_v)
#define new_som_bsp_u som_bsp_new(0,som_bsp_v_m,som_bsp_v,som_bsp_u)
#define new_som_bsp_t som_bsp_new(1,som_bsp_t_m,som_bsp_t,som_bsp_t)
#define new_som_bsp_y som_bsp_new(2,som_bsp_y_m,som_bsp_y,som_bsp_y)
#define clear_som_bsp_y som_bsp_mem[2].clear()
#define count_som_bsp_t som_bsp_mem[1].size(sizeof(som_bsp_t))

struct som_bsp_v //vertex
{
	int d; //2023 //TESTING

	WORD batch,i; float pos[3]; //UNION

	float lit[3], uv[2];
};
struct som_bsp_u //unlit vertex
{
	int d; //2023 //TESTING

	WORD batch,i; float pos[3]; //UNION

	//WARNING: reordered for lerp ease?
	//DWORD vc; float uv[2];
	float uv[2]; DWORD vc; 
};
struct som_bsp_t //triangle
{	
	union
	{
		som_bsp_v *v[3];
		som_bsp_u *u[3]; 
	};

	union
	{
		som_MDL::vbuf *se; //v
		DWORD mpx_texture; //u
		DWORD texture_etc; //?
	};

	som_bsp_t *next; //linked-list

	void split(int,int,int,som_bsp_t*,float);
	void split2(int,int,int,som_bsp_t*,som_bsp_t*,float,float);

	int mode(){ return mpx_texture<=0xffff?0:se->mode; }

	bool unlit(){ return mode()<=1; }

	void draw();
};

struct som_bsp_e //enum
{
//	enum{ LEFT=-1,SAME,RIGHT,BOTH };
	int i0:4,i1:4,i2:4,result:4;
};
struct som_bsp_p //plane
{
	float abc[3],d;

	void init_abcd(float[3],float[3],float[3]);

	bool operator<(const som_bsp_p &cmp)const
	{
		for(int i=4;i-->0;)		
		if(float d=abc[i]-cmp.abc[i])
		return d<0;
		return false;
	}
	//MSVC2010 tries to mix == with < if debugging
	//bool operator==(const som_bsp_p &cmp)const
	static bool eq(const som_bsp_p &a, const som_bsp_p &b)
	{
		for(int i=4;i-->0;) 
		if(fabsf(a.abc[i]-b.abc[i])>0.001f)
		return false;
		return true;
	}
			
	enum{ LEFT=-1,SAME,RIGHT,BOTH };	
	som_bsp_e side(float[3],float[3],float[3]);
	float intersection(float[3],float[3]);
};
struct som_bsp_y : som_bsp_p //node
{
	som_bsp_y *left;
	som_bsp_y *right;
	som_bsp_t *first; //linked-list	
	void partition(bool split);

	som_bsp_t *split(som_bsp_t*&,som_bsp_e);

	som_bsp_y **sort(som_bsp_y**,float[3]); //repurposes left

	void aa_split(som_bsp_t*); //do_aa
};

#define som_bsp_dot(a,b) (a[0]*b[0]+a[1]*b[1]+a[2]*b[2])

static int som_bsp_d(float p[3], float ref[3]=SOM::cam)
{		
	double m[3] = {p[0]-ref[0],p[1]-ref[1],p[2]-ref[2]};	
	return (int)(100000*(double)som_bsp_dot(m,m));
}
static int som_bsp_d3(som_bsp_u *u[3])
{
	return min(min(u[0]->d,u[1]->d),u[2]->d);
}

static som_bsp_t *som_bsp_top = 0;
typedef SOM::MPX::Layer::tile som_bsp_tile;
extern void som_bsp_add_tiles(som_bsp_tile::scenery_ext *ses, size_t sz)
{
	auto swap = som_bsp_top;

	DWORD txr,tex = ~0, tuv = 0; float tu,tv; 

	som_MPX &mpx = *SOM::L.mpx->pointer;
	float *vp = (float*)mpx[SOM::MPX::vertex_pointer];
	//REPURPOSING
	//this should max out at 2/3rds this buffer's size
	auto up = (som_bsp_u**)(WORD*)SOM::L.mpx->ibuffer;

	float r[3]; memcpy(r,SOM::cam,sizeof(r));

	while(sz-->0)
	{
		auto *se2 = ses+sz;

		if(tex!=se2->model->texture)
		{
			tex = se2->model->texture; 
						
			assert(tex<1024); //65535?

			txr = ((WORD*)mpx[SOM::MPX::texture_pointer])[tex];

			assert(txr<1024); //65535?

			tuv = 0;

		//	DWORD blend = DX::D3DBLEND_INVSRCALPHA;

			if(SOM::material_textures)
			{
		//		DWORD txr = ((WORD*)mpx[SOM::MPX::texture_pointer])[tex];
				//assert(txr==som_scene_state::texture);

		//		if(txr<1024)
				if(SOM::MT *mt=(*SOM::material_textures)[txr])
				{
					if(tuv=mt->mode&(8|16))
					{
						extern float som_scene_413F10_uv;

						tu = mt->data[7]*som_scene_413F10_uv;
						tv = mt->data[8]*som_scene_413F10_uv;
					}
				
		//			if(mt->mode&0x100) blend = DX::D3DBLEND_ONE;
				}
			}
			
		//	if(destblend!=blend) //NOTE: not relying on sss::destblend
			{
		//		destblend = blend;

		//		DDRAW::Direct3DDevice7->SetRenderState
		//		(DX::D3DRENDERSTATE_DESTBLEND,sss::destblend=destblend);
			}
		}

		int a = se2->angle;
		float l[3],*cp = se2->locus; //???
		l[0] = cp[0];
		l[1] = cp[1];
		l[2] = cp[2];
		DWORD *vc = *se2->model->vcolor_indices;
		int i,iN = se2->model->vcolor_indicesN;
		for(i=0;i<iN;i++,vc+=2)
		{
			auto *u = up[i] = new_som_bsp_u; 
			
			u->batch = 0; u->vc = vc[1];

			float *p = u->pos; 

			float *v = vp+5*vc[0];
			p[0] = l[0];
			p[1] = l[1]+v[1];
			p[2] = l[2];
			switch(a) //might want to unroll this part 
			{
			case 0: p[0]+=v[0]; p[2]+=v[2]; break; //S
			case 1: p[0]-=v[2]; p[2]+=v[0]; break; //W 
			case 2: p[0]-=v[0]; p[2]-=v[2]; break; //N
			case 3: p[0]+=v[2]; p[2]-=v[0]; break; //E
			}
			//(DWORD&)p[3] = vc[1]; //lerp?
			//p[4] = v[3];
			//p[5] = v[4];
			(DWORD&)p[5] = vc[1];
			p[3] = v[3];
			p[4] = v[4];
			if(tuv) switch(tuv&16?a:0) //2020
			{				
			//case 0: p[4]+=tu; p[5]+=tv; break; //KF2...
			case 0: p[3]+=tu; p[4]+=tv; break; //KF2
			case 1: p[3]-=tu; p[4]+=tv; break; //UNTESTED
			case 2: p[3]-=tu; p[4]-=tv; break; //UNTESTED
			case 3: p[3]+=tu; p[4]-=tv; break; //UNTESTED
			}

			u->d = som_bsp_d(p,r);
		}
		
		//transparent triangles are hidden like so
		//int i = 2*se2->model->triangle_indicesN;
		i = se2->model->_transparent_indicesN; if(!i) 
		{
			//paranoia: relying on !ibN to lock
			assert(0); continue; 
		}

		auto *ip = se2->model->triangle_indices;
		for(i/=3;i-->0;)
		{
			auto *t = new_som_bsp_t; 

			t->mpx_texture = txr;

			t->u[0] = up[*ip++];
			t->u[1] = up[*ip++];
			t->u[2] = up[*ip++];

			t->next = swap; swap = t;
		}
	}

	som_bsp_top = swap;
}
static void som_bsp_add_sprite(som_MDL::vbuf *se, som_bsp_t* &swap, float ref[3])
{
	assert(se->material==0); //???

	auto *v = (DX::D3DLVERTEX*)se->worldxform;

	som_bsp_u *up[4]; for(int i=4;i-->0;v++)
	{
		som_bsp_u *u = up[i] = new_som_bsp_u;
				
		u->batch = 0;

		u->pos[0] = v->x;
		u->pos[1] = v->y;
		u->pos[2] = v->z;
		u->uv[0] = v->tu;
		u->uv[1] = v->tv;
		u->vc = v->color; 
				
		u->d = som_bsp_d(u->pos,ref);
	}	

	for(int i=2;i-->0;)
	{
		auto *t = new_som_bsp_t; 

	//	t->texture_etc = se->texture; //material?
		t->se = se;

		if(i)
		{
			t->u[0] = up[2];
			t->u[1] = up[1];
			t->u[2] = up[0];
		}
		else //UNTESTED
		{
			t->u[0] = up[3];
			t->u[1] = up[2];
			t->u[2] = up[0];
		}

		t->next = swap; swap = t;
	}
}
static void som_bsp_lerp(int i, float *result, float *a, float *b, float t)
{
	while(i-->0) result[i] = a[i]+(b[i]-a[i])*t; 
}	
extern void som_bsp_add_vbufs(som_MDL::vbuf **ses, size_t sz)
{
	auto swap = som_bsp_top;
	
	//REPURPOSING
	//this should max out at 2/3rds this buffer's size
	//auto up = (som_bsp_u**)(WORD*)SOM::L.mpx->ibuffer;
	//HACK: this has greater memory capacity beyond 4096
	auto vp = (som_bsp_v**)(WORD*)SOM::L.mpx->_client_vbuffer;

	int decal = 0;
	som_MDL::vbuf *decal2 = 0;

	float r[3] = {};
	extern unsigned som_hacks_tint;
	bool item = som_hacks_tint==SOM::frame;
	extern float som_hacks_inventory[4][4];
	extern DX::D3DMATRIX som_hacks_item; //world	
	if(item)
	{
		for(int i=3;i-->0;) r[i] = -som_hacks_inventory[3][i];
	}
	else memcpy(r,SOM::cam,sizeof(r));

	while(sz-->0)
	{
		auto *se = ses[sz];

		DWORD mode = se->mode;

		if(!se->sort) continue; //sort?

	//	if(!se->texture) continue; //shadow?

		assert(se->texture);

		switch(se->mode)
		{
		default: assert(0); continue; //2? 4?

		case 1:	som_bsp_add_sprite(se,swap,r); continue;

		case 3: break;
		}

		int i,iN = se->vcount;
		//float *p = (float*)se->vdata;
		//for(i=0;i<iN;i++,p+=8)
		for(i=0;i<iN;i++)
		{
			vp[i] = new_som_bsp_v; 

			vp[i]->batch = 0; //...
		}
		//HACK: avoid Somvector.h dependency
		extern void som_scene_xform_bsp_v(void*,float**,size_t,bool);
		som_scene_xform_bsp_v(se,(float**)vp,iN,item);

		if(se->sort==2) 
		{
			int m = 0;
			for(i=0;i<iN;i++) m+=som_bsp_d(vp[i]->pos,r);
			m/=iN; //average?

			if((size_t)(decal2-se)<4) //same MDO?
			if(0==((intptr_t)decal2-(intptr_t)se)%sizeof(*se))
			{
				if(abs(decal-m)<500) //DICEY			
				{
					m = decal+10; //1 is likely fine
				}
			}

			for(i=0;i<iN;i++) vp[i]->d = m;

			decal = m; decal2 = se;
		}
		else for(i=0;i<iN;i++)
		{
			vp[i]->d = som_bsp_d(vp[i]->pos,r);
		}		
		
		/*#ifdef _DEBUG
		if(som_BSP*b=se->bsp)
		{
			WORD *bw = b->data();
			auto *bv = (som_BSP::vert*)(bw+b->tris);
			auto *bc = (som_BSP::cut*)(bv+b->verts);
			
			for(iN+=b->verts;i<iN;i++,bv++)
			{
				assert(bv->a<i&&bv->b<i);
				auto *v = vp[i] = new_som_bsp_v;
				auto *va = vp[bv->a], *vb = vp[bv->b];
				som_bsp_lerp(8,v->pos,va->pos,vb->pos,bv->t);

				v->batch = 0;
			}
			int iM = iN; //debugging
			
			auto *ip = se->idata;
			for(i=0,iN=se->icount;i<iN;)
			{
				auto *t = new_som_bsp_t; 

				t->se = se;

				for(int j=0;j<3;j++,ip++)			
				if(bc->index==i++)
				{
					assert(bc->vertex<iM);

					t->v[j] = vp[bc->vertex];

					bc++;
				}
				else t->v[j] = vp[*ip];

				t->next = swap; swap = t;
			}

			for(int i=b->tris/3;i-->0;)
			{
				auto *t = new_som_bsp_t; 

				t->se = se;

				assert(bw[0]<iM&&bw[1]<iM&&bw[2]<iM);

				t->v[0] = vp[*bw++];
				t->v[1] = vp[*bw++];
				t->v[2] = vp[*bw++];

				t->next = swap; swap = t;
			}
		}
		else
		#endif*/
		{
			auto *ip = se->idata;
			for(i=se->icount/3;i-->0;)
			{
				auto *t = new_som_bsp_t; 

				t->se = se;

				t->v[0] = vp[*ip++];
				t->v[1] = vp[*ip++];
				t->v[2] = vp[*ip++];

				t->next = swap; swap = t;
			}
		}
	}

	som_bsp_top = swap;
}


void som_bsp_p::init_abcd(float *p0, float *p1, float *p2)
{
	float v[3] = {p1[0]-p0[0],p1[1]-p0[1],p1[2]-p0[2]};
	float w[3] = {p2[0]-p0[0],p2[1]-p0[1],p2[2]-p0[2]};
	//cross product
	abc[0] = w[1]*v[2]-w[2]*v[1]; 
	abc[1] = w[2]*v[0]-w[0]*v[2]; 
	abc[2] = w[0]*v[1]-w[1]*v[0];
	//normalize
	if(float len=som_bsp_dot(abc,abc))
	{
		len = 1/sqrtf(len);
		for(int i=3;i-->0;) abc[i]*=len;
	}
	
	d = som_bsp_dot(abc,p0);
}
som_bsp_e som_bsp_p::side(float *p0, float *p1, float *p2)
{
	float d0 = som_bsp_dot(abc,p0);
	float d1 = som_bsp_dot(abc,p1);
	float d2 = som_bsp_dot(abc,p2);

	int e0,e1,e2,er;
	if(e0=fabsf(d0-d)>0.001f) e0 = d0<d?LEFT:RIGHT;
	if(e1=fabsf(d1-d)>0.001f) e1 = d1<d?LEFT:RIGHT;
	if(e2=fabsf(d2-d)>0.001f) e2 = d2<d?LEFT:RIGHT;

	if(e0<=0&&e1<=0&&e2<=0)
	{
		er = e0|e1|e2?LEFT:SAME;
	}
	else er = e0>=0&&e1>=0&&e2>=0?RIGHT:BOTH;

	som_bsp_e e = {e0,e1,e2,er}; return e;
}
void som_bsp_y::partition(bool splitting)
{
	left = right = 0;

	//plane normal/distance?
	init_abcd(first->v[0]->pos,first->v[1]->pos,first->v[2]->pos);

	som_bsp_t *_l[3] = {}, **l = _l-LEFT; //YUCK!

	//NOTE: Sometimes code will try to pick a polygon
	//to divide the half-spaces more evenly. A random
	//pick might help but is messier with linked list.
	for(som_bsp_t*q,*p=first->next;p;p=q)
	{
		q = p->next;

		som_bsp_e e = side(p->v[0]->pos,p->v[1]->pos,p->v[2]->pos);

		int i = e.result; 
		
		if(i==BOTH) if(splitting) //split? //IN DISUSE (ILLUSTRATING)
		{
			p->next = 0; //2 on left?

			if(som_bsp_t*pp=split(p,e)) //DICEY
			{
				if(p->next) //2 on left?
				{
					p->next->next = l[LEFT]; l[LEFT] = p->next;
				}
				else if(pp->next) //2 on right?
				{
					pp->next->next = l[RIGHT]; l[RIGHT] = pp->next;
				}

				pp->next = l[RIGHT]; l[RIGHT] = pp;
			}
			else assert(0);

			i = LEFT;
		}
		else if(1) //no split (do_aa requirement too hard?)
		{
			float *p0 = p->v[0]->pos;
			float *p1 = p->v[1]->pos;
			float *p2 = p->v[2]->pos;
			float avg[3];
			avg[0] = (p0[0]+p1[0]+p2[0])*0.3333333f;
			avg[1] = (p0[1]+p1[1]+p2[1])*0.3333333f;
			avg[2] = (p0[2]+p1[2]+p2[2])*0.3333333f;

			float d0 = som_bsp_dot(abc,avg);

			//if(i=fabsf(d0-d)>0.0001f) i = d0<d?LEFT:RIGHT;
			//if(i=fabsf(d0-d)>0.01f) i = d0<d?LEFT:RIGHT;
			i = d0<d?LEFT:RIGHT;
		}
		else i = LEFT;
		
		p->next = l[i]; l[i] = p;
	}

	first->next = l[SAME];

	for(int i=LEFT;i<=RIGHT;i+=RIGHT-LEFT)
	{
		if(!l[i]) continue;
		
		auto y = new_som_bsp_y;

		(i==LEFT?left:right) = y;

		y->first = l[i]; y->partition(splitting);
	}
}

///////////////////////////////////////////

static DWORD som_bsp_lerp_vc(DWORD ac, DWORD bc, float t)
{
	if(ac==bc) return ac;
	BYTE *a = (BYTE*)&ac; BYTE *b = (BYTE*)&bc;
	DWORD c; BYTE *result = (BYTE*)&c;
	for(int i=4;i-->0;)
	result[i] = a[i]+(b[i]-a[i])*t; return c;
}
void som_bsp_t::split(int i0, int i1, int i2, som_bsp_t *n1, float t1)
{
	if(unlit())
	{
		n1->u[2] = new_som_bsp_u;
		som_bsp_lerp(5,n1->u[2]->pos,u[i1]->pos,u[i2]->pos,t1);	
		n1->u[2]->vc = som_bsp_lerp_vc(u[i1]->vc,u[i2]->vc,t1);
	}
	else
	{
		n1->v[2] = new_som_bsp_v;
		som_bsp_lerp(8,n1->v[2]->pos,v[i1]->pos,v[i2]->pos,t1);
	}
	n1->v[2]->batch = 0;
	
	n1->v[0] = v[i0];
	n1->v[1] = v[i1];
	
	n1->texture_etc = texture_etc;

	v[i1] = n1->v[2];
}
void som_bsp_t::split2(int i0, int i1, int i2, som_bsp_t *n1, som_bsp_t *n2, float t1, float t2)
{
	if(unlit())
	{
		n1->u[0] = new_som_bsp_u; n2->u[2] = new_som_bsp_u;
		som_bsp_lerp(5,n1->u[0]->pos,u[i0]->pos,u[i1]->pos,t1);
		n1->u[0]->vc = som_bsp_lerp_vc(u[i0]->vc,u[i1]->vc,t1);		
		som_bsp_lerp(5,n2->u[2]->pos,u[i0]->pos,u[i2]->pos,t2);
		n2->u[2]->vc = som_bsp_lerp_vc(u[i0]->vc,u[i2]->vc,t2);
	}
	else
	{
		n1->v[0] = new_som_bsp_v; n2->v[2] = new_som_bsp_v;
		som_bsp_lerp(8,n1->v[0]->pos,v[i0]->pos,v[i1]->pos,t1);
		som_bsp_lerp(8,n2->v[2]->pos,v[i0]->pos,v[i2]->pos,t2);
	}
	n1->v[0]->batch = 0;
	n2->v[2]->batch = 0;

	n1->v[1] = v[i1];
	n1->v[2] = v[i2];

	n1->texture_etc = texture_etc;

	n2->v[0] = n1->v[0];
	n2->v[1] = v[i2];

	n2->texture_etc = texture_etc;

	v[i1] = n1->v[0];
	v[i2] = n2->v[2];
}
/*void som_bsp_y::aa_split(som_bsp_t *p) //do_aa? //UNUSED
{
	//HACK: to ensure do_aa doesn't crack split
	//triangles on both sides of the half-space
	for(som_bsp_t*q;p;p=q)
	{
		q = p->next;

		som_bsp_e e = side(p->v[0]->pos,p->v[1]->pos,p->v[2]->pos);

		if(e.result!=BOTH) continue;

		p->next = 0; //2 on left?

		auto *f = p;  //DICEY

		if(som_bsp_t*pp=split(p,e)) //DICEY
		{
			if(p!=f) std::swap(p,pp); //YUCK

			assert(p==f);
			
			if(p->next) p = p->next; p->next = pp;

			if(pp->next) pp = pp->next; pp->next = q;
		}
		else 
		{
			p->next = q; assert(0);
		}
	}
}*/

float som_bsp_p::intersection(float p1[3], float p2[3])
{
	float p2_p1[3]; for(int i=3;i-->0;) p2_p1[i] = p2[i]-p1[i];

	return (d-som_bsp_dot(abc,p1))/som_bsp_dot(abc,p2_p1);
}
som_bsp_t *som_bsp_y::split(som_bsp_t* &t, som_bsp_e e)
{
	som_bsp_t *r = new_som_bsp_t;

//	bool rtl; if(!e.i0||!e.i1||!e.i2) //triangles
	bool rtl; if(0==(e.i0&e.i1&e.i2)) //triangles
	{
		int i0,i1,i2; if(!e.i0)
		{
			i0 = 0; i1 = 1; i2 = 2; rtl = e.i1<0;
		}
		else if(!e.i1)
		{
			i0 = 1; i1 = 2; i2 = 0; rtl = e.i0>=0;
		}
		else if(!e.i2)
		{
			i0 = 2; i1 = 0; i2 = 1; rtl = e.i0<0;
		}
		else //MEMORY LEAK?
		{
			assert(0); return 0; //2022
		}

		r->next = 0;
		t->split(i0,i1,i2,r,intersection(t->v[i1]->pos,t->v[i2]->pos));
	}
	else //triangle and quadrangle (or 3 triangles IOW)
	{
		int i0,i1,i2; if(e.i0==e.i1)
		{
			i0 = 2; i1 = 0; i2 = 1; rtl = e.i2>=0;
		}
		else if(e.i0==e.i2)
		{
			i0 = 1; i1 = 2; i2 = 0; rtl = e.i1>=0;
		}
		else if(e.i1==e.i2)
		{
			i0 = 0; i1 = 1; i2 = 2; rtl = e.i0>=0;
		}
		else //MEMORY LEAK?
		{
			assert(0); return 0; //2022
		}
		
		r->next = new_som_bsp_t;
		r->next->next = 0;
		t->split2(i0,i1,i2,r,r->next,
		intersection(t->v[i0]->pos,t->v[i1]->pos),intersection(t->v[i0]->pos,t->v[i2]->pos));
	}
	if(rtl) std::swap(t,r); return r;
}


/////////////////////////////////////////


extern void som_scene_state::setss(DWORD cop) //2022
{	
	namespace sss = som_scene_state;
	#define _(x,y,z) if(sss::x!=z)\
	DDRAW::Direct3DDevice7->SetTextureStageState(0,DX::D3DTSS_##y,sss::x=z);
	switch(cop)
	{
	default: assert(0); break;

	case 0: //NEW: I think this is what tiles use

		_(tex0colorop,COLOROP,4) //D3DTOP_MODULATE
		_(tex0colorarg1,COLORARG1,2) //D3DTA_TEXTURE
		_(tex0colorarg2,COLORARG2,1) //D3DTA_CURRENT
		_(tex0alphaop,ALPHAOP,1) //D3DTOP_DISABLE
		break;

	case 1: case 7:
	
		_(tex0colorop,COLOROP,2) //D3DTOP_SELECTARG1
		_(tex0colorarg1,COLORARG1,2) //D3DTA_TEXTURE
		_(tex0alphaop,ALPHAOP,2) //D3DTOP_SELECTARG1
		_(tex0alphaarg1,ALPHAARG1,2) //D3DTA_TEXTURE
		break;

	case 2:

		_(tex0colorop,COLOROP,4) //D3DTOP_MODULATE
		_(tex0colorarg1,COLORARG1,2) //D3DTA_TEXTURE
		_(tex0colorarg2,COLORARG2,0) //D3DTA_DIFFUSE
		_(tex0alphaop,ALPHAOP,3) //D3DTOP_SELECTARG2
		_(tex0alphaarg2,ALPHAARG2,0) //D3DTA_DIFFUSE
		break;

	case 3:

		_(tex0colorop,COLOROP,13) //D3DTOP_BLENDTEXTUREALPHA
		_(tex0colorarg1,COLORARG1,2) //D3DTA_TEXTURE
		_(tex0colorarg2,COLORARG2,0) //D3DTA_DIFFUSE
		_(tex0alphaop,ALPHAOP,3) //D3DTOP_SELECTARG2
		_(tex0alphaarg2,ALPHAARG2,0) //D3DTA_DIFFUSE
		break;

	case 4:

		_(tex0colorop,COLOROP,4) //D3DTOP_MODULATE
		_(tex0colorarg1,COLORARG1,2) //D3DTA_TEXTURE
		_(tex0colorarg2,COLORARG2,0) //D3DTA_DIFFUSE
		_(tex0alphaop,ALPHAOP,4) //D3DTOP_MODULATE
		_(tex0alphaarg1,ALPHAARG1,2) //D3DTA_TEXTURE
		_(tex0alphaarg2,ALPHAARG2,0) //D3DTA_DIFFUSE
		break;

	case 8:

		_(tex0colorop,COLOROP,7) //D3DTOP_ADD
		_(tex0colorarg1,COLORARG1,2) //D3DTA_TEXTURE
		_(tex0colorarg2,COLORARG2,0) //D3DTA_DIFFUSE
		_(tex0alphaop,ALPHAOP,3) //D3DTOP_SELECTARG2
		_(tex0alphaarg2,ALPHAARG2,0) //D3DTA_DIFFUSE
		break;
	}	
	#undef _
}

extern bool som_scene_gamma_n;
extern float som_scene_lit5[10];
extern float *som_scene_lit;
extern float som_hacks_skyconstants[4];
extern int som_scene_volume;
extern void som_scene_lighting(bool);

som_bsp_y **som_bsp_y::sort(som_bsp_y **yy, float ref[3])
{
	bool r = som_bsp_dot(abc,ref)<d;

	//NOTE: recursively this would look like
	//this, but it builds a flat list instead
	//(repurposing left)
	//if(y=r?right:left) y->draw();
	//draw();
	//if(y=r?left:right) y->draw();

	auto *y = r?right:left; if(y) yy = y->sort(yy,ref);
	
	*yy = this; yy = &left;
	
	if(y=r?left:right) yy = y->sort(yy,ref); 
	
	return yy;
}
extern void som_bsp_sort_and_draw(bool item)
{
	if(!som_bsp_top) return; //important           

	namespace sss = som_scene_state;

	if(item)
	{
		sss::push();
		extern void som_scene_alphaenable(int);
		som_scene_alphaenable(1);				
		DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_ZWRITEENABLE,sss::zwriteenable=0);
		DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_COLORKEYENABLE,sss::colorkeyenable=1);
	}
	if(sss::worldtransformed)
	{
		sss::worldtransformed = 0;
		DDRAW::Direct3DDevice7->SetTransform(DX::D3DTRANSFORMSTATE_WORLD,&DDRAW::Identity);
	}
	assert(sss::alphaenable&&!sss::worldtransformed);
	assert(sss::colorkeyenable&&!sss::zwriteenable);

	//is LEQUAL inadequate for decals?
	DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_ZBIAS,-1);

	extern float som_hacks_inventory[4][4];	
	float r[3]; if(item)
	{
		for(int i=3;i-->0;) r[i] = -som_hacks_inventory[3][i];
	}
	else memcpy(r,SOM::cam,sizeof(r));
	
	bool fog = !item;
	som_bsp_t *fogt = 0;
	som_scene_lighting(0); som_scene_lit = 0; //fog?
	sss::texture = 0xffff;
	DDRAW::Direct3DDevice7->SetTexture(0,0);

	/*#if 0
	#define SOM_BSP_Y 1
	#else
	#define SOM_BSP_Y 0*/
	
		typedef std::pair<int,som_bsp_t*> v_t;
		static std::vector<v_t> dt; dt.clear();
		dt.reserve(count_som_bsp_t);
		auto *t = som_bsp_top, *tt = t;
		for(;t;t=t->next)				
		dt.push_back(v_t(som_bsp_d3(t->u),t));

		auto it = dt.begin(), iit=it, itt = dt.end();
		//NOTE: stable_sort just for sort mode 2?
		//YUCK: sort mode 2 looks fine with plain sort
		auto pred = [](const v_t &a, const v_t &b)
		{
			return a.first>b.first; //<
		};
		std::sort(iit=it,itt,pred);
	//	std::stable_sort(iit=it,itt,pred);
		if(fog)
		{
			int fog2 = (int)(100000*SOM::fogend*SOM::fogend);
			auto fit = std::lower_bound(it,itt,v_t(fog2,t),pred);
			if(fit!=itt) 
			{
				//NOTE: the lower/upper_bound may be 
				//way far from the fog line
				if(fit>itt)
				{
					fit--;
				
					fogt = fit->second;
				}
				else if(fit->first>=fog2)
				{
					fogt = fit->second;
				}
			}
		}

		if(!fogt) fog = false;

		//iterator debugging slowdown
		auto *pp = &dt[0], *p = pp, *d = p+dt.size(); 
	/*
	#endif
	#if SOM_BSP_Y
	
		auto y = new_som_bsp_y;		
	
		y->first = som_bsp_top; 

		y->partition(); 

		//WARNING: repurposing y->left!
		*y->sort(&y) = 0;
	
		if(0) for(auto*p=y;p;p=p->left) //do_aa?
		{
			//this seems prohibitively costly :(

			for(auto*q=y;q;q=q->left)
			{
				if(p!=q) q->aa_split(p->first);
			}
		}
	
	#endif*/
		
  //DRAWING/////////////////////////

	som_MPX &mpx = *SOM::L.mpx->pointer;	

	#ifdef NDEBUG
//	#error do_red (npc) (lights)
	#endif
	extern void som_scene_red(DWORD);
	extern int som_scene_volume_select(DWORD,int);

	const DWORD lockf = //VS2010 enum fails in lambda
	DDLOCK_DISCARDCONTENTS|DDLOCK_WAIT|DDLOCK_WRITEONLY;

	int draw_calls = 0;
	int draw_tries = 0;

	//this has way more memory that's unused as far as
	//as I know, so that the vbN isn't limited by the
	//tile buffer limit (it can hold 4096 although
	//historically chunks were always 128)
//	WORD *ib = SOM::L.mpx->ibuffer; //2688
	WORD *ib = (WORD*)SOM::L.mpx->_client_vbuffer; //21504+2688
	DX::IDirect3DVertexBuffer7 *ub = SOM::L.mpx->vbuffer;	
	DX::IDirect3DVertexBuffer7 *vb = SOM::L.vbuffer;
//	DWORD &ubN = SOM::L.mpx_vbuffer_size; //896
//	DWORD &ibM = SOM::L.mpx_ibuffer_size; //2688
//	DWORD ibN = 0; assert(!ibM);
	DWORD ubN = 0, vbN = 0;
	WORD *q = ib;
	int batch = 1, b = 0;
	float *up,*upp,*vp,*vpp;	
	int mode = -1, ui = 0, vi = 0;
	auto flush = [&]()
	{
		draw_calls++;

		if(ui)
		{
			#define SOM_BSP_VB 1
			#if SOM_BSP_VB
			{
				//WARNING: slower without dueling vbuffers
			//	ub->Unlock();	
				DDRAW::Direct3DDevice7->DrawIndexedPrimitiveVB
				(DX::D3DPT_TRIANGLELIST,ub,0,ubN,q,ui,0);	
			//	ub->Lock(lockf,(void**)&up,0);
			}
			#else
			{
				DDRAW::Direct3DDevice7->DrawIndexedPrimitive
				//WARNING: this change may upload unnecessary vertex data
			//	(DX::D3DPT_TRIANGLELIST,0x142,up-=ubN*6,ubN,q,ibN,0); 
				(DX::D3DPT_TRIANGLELIST,0x142,upp,ubN,q,ui,0);
			}
			#endif

			q+=ui; ui = 0;

		//	ubN = 0; 

		//	assert(!vbN);
		}
		if(vi)
		{
		//	assert(vbN);

			#if SOM_BSP_VB
			{					 
				//WARNING: slower without dueling vbuffers
			//	vb->Unlock();	
				DDRAW::Direct3DDevice7->DrawIndexedPrimitiveVB
				(DX::D3DPT_TRIANGLELIST,vb,0,vbN,q,vi,0);	
			//	vb->Lock(lockf,(void**)&vp,0);
			}
			#else
			{
				DDRAW::Direct3DDevice7->DrawIndexedPrimitive
				//WARNING: this change may upload unnecessary vertex data
			//	(DX::D3DPT_TRIANGLELIST,D3DFVF_VERTEX,vp-=vbN*8,vbN,q,ibN,0);
				(DX::D3DPT_TRIANGLELIST,D3DFVF_VERTEX,vpp,vbN,q,vi,0);
			}
			#endif

			q+=vi; vi = 0;

		//	vbN = 0; 
		}
	};
	
//	DDRAW::IDirectDrawSurface7 *tar = 0; //atlas

	DWORD msz = SOM::L.materials_count; //DEBUGGING
	DWORD txr = ~0, mat = ~0, cop = ~0;
	DWORD vs = 0, npc = ~0;
	DWORD cmp = ~0, blend = DX::D3DBLEND_INVSRCALPHA;
	assert(sss::srcblend==DX::D3DBLEND_SRCALPHA);
//	DDRAW::Direct3DDevice7->SetRenderState
//	(DX::D3DRENDERSTATE_DESTBLEND,sss::destblend=blend);

	/*UNUSED (EXAMPLE)
	* 
	* this is example code for using 
	* SOM_BSP_Y
	* 
	//WARNING: repurposing y->left!
	for(*y->sort(&y,r)=0;y;y=y->left)
	for(;y;y=y->left)
	for(t=y->first;t;t=t->next)
	{
		//same rendering code goes here
	}
	*/

	ub = som_scene::ubuffers[b];
	vb = som_scene::vbuffers[b];	
//	ub->Lock(lockf,(void**)&up,0); upp = up;
//	vb->Lock(lockf,(void**)&vp,0); vpp = vp;

	extern DWORD som_hacks_primode;
	som_hacks_primode = fog?4:0;

	//2-pass: draw back-faces for 2-sided volume 
	//textures
	if(1||!EX::debug)
	{
	//	som_scene_volume = 4; //HACK

		bool v2 = false, used = false;

		npc = 0;
		som_hacks_skyconstants[2] = npc?-1:1; 
		DDRAW::pset9(som_hacks_skyconstants);
		som_scene_gamma_n = false;
		sss::setss(0);

		for(p=pp;p<d;p++)
		{
			t = p->second;

			if(cmp!=t->texture_etc)
			{
				cmp = t->texture_etc;

				DWORD cmp2 = cmp<0xffff?cmp:t->se->texture;

				if(cmp2!=txr)
				{
					txr = cmp2;

					v2 = false;

					if(SOM::volume_textures)
					if(SOM::VT*v=(*SOM::volume_textures)[txr])
					{
						if(v->frame!=SOM::frame)
						{
							extern void som_scene_volume_select2(SOM::VT*);
							som_scene_volume_select2(v);
						}
						if(2==(int)v->sides) //draw back side only (2-pass)
						{
							if(mode==-1) //used
							{
								mode = 0;

								//190: D3DRS_COLORWRITEENABLE1 (reenable depth-texture write)
								//168: D3DRS_COLORWRITEENABLE
								DDRAW::Direct3DDevice7->SetRenderState((DX::D3DRENDERSTATETYPE)190,0xF);
								DDRAW::Direct3DDevice7->SetRenderState((DX::D3DRENDERSTATETYPE)168,0);
								DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_ALPHABLENDENABLE,sss::alphaenable=0);
								DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_CULLMODE,DX::D3DCULL_CW);

								ub->Lock(lockf,(void**)&up,0); upp = up;
							}

							v2 = true;
						}
					}
				}
			}

			if(v2)
			{
				//vbuffer is really 4096
				//if(ubN>896-3||ibN>2688-3)
				if(ubN>som_scene::vbuffer_size-3||ui>4096*3-3)
				{
					ub->Unlock(); 

					q = ib; //YUCK //REPURPOSING

					flush(); //lambda //REMOVE ME				

					ub->Lock(lockf,(void**)&up,0); upp = up;

					ubN = 0; q = ib; batch++; //YUCK
				}

				if(t->mode()<=1) //tile/sprite?
				{
					for(int j=0;j<3;j++)
					{
						auto *u = t->u[j];

						if(u->batch!=batch)
						{
							u->batch = batch;

							up[0] = u->pos[0];
							up[1] = u->pos[1];
							up[2] = u->pos[2];
							(DWORD&)up[3] = ~0;
							up[4] = u->uv[0];
							up[5] = u->uv[1];
							up+=6;

							u->i = ubN++;
						}
						*q++ = u->i;
					}
					ui+=3;
				}
				else //pack v into u (fog)
				{
					for(int j=0;j<3;j++)
					{
						auto *v = t->v[j];

						if(v->batch!=batch)
						{
							v->batch = batch;

							up[0] = v->pos[0];
							up[1] = v->pos[1];
							up[2] = v->pos[2];
							(DWORD&)up[3] = ~0;
							up[4] = v->uv[0];
							up[5] = v->uv[1];
							up+=6;

							v->i = ubN++;
						}
						*q++ = v->i;
					}
					ui+=3;
				}
			}
		}							

		if(ui)
		{
			q = ib; //YUCK //REPURPOSING

			ub->Unlock(); 

			flush(); //lambda //REMOVE ME

			ubN = 0; q = ib;
		}				

		if(mode!=-1) //used
		{
			mode = -1; 

			som_scene_volume = 0;

			DDRAW::Direct3DDevice7->SetRenderState((DX::D3DRENDERSTATETYPE)190,0);
			DDRAW::Direct3DDevice7->SetRenderState((DX::D3DRENDERSTATETYPE)168,0xF);
			DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_ALPHABLENDENABLE,sss::alphaenable=1);
			DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_CULLMODE,DX::D3DCULL_CCW);
		}

		cmp = ~0;
	}
	draw_calls = 0; //UNUSED

//goto Debug;

//	ub->Unlock(); vb->Unlock();

	//2-pass: transparency and front side of
	//2-side volume textures
	for(p=pp,tt=nullptr;p<d;tt=t)
	{	
		auto q = p+1; //SHADOWING
		while(q<d&&q->first==p->first)
		q++;

		if(q<=p+1)
		{
			t = p++->second; 

			if(tt) tt->next = t;
			else som_bsp_top = t; //REPURPOSING
		}
		else
		{
			auto y = new_som_bsp_y; 

			y->first = p->second;
			
			for(p++;p<q;p++)			
			p[-1].second->next = p->second;			
			p[-1].second->next = nullptr;
			
				y->partition(false/*item*/);
						
				*y->sort(&y,r)=0; //YUCK			

			if(!tt) som_bsp_top = y->first; //REPURPOSING

			for(t=tt;y;y=y->left)
			{
				if(t) t->next = y->first; t = y->first;

				while(t->next) t = t->next;
			}
				clear_som_bsp_y;
		}
	}
	t->next = nullptr;
	
	tt = som_bsp_top; //batch?

	resume: 
	
	batch++; 
	
	vbN = ubN = 0; q = ib;

	b = ++b%som_scene::vbuffersN;
	ub = som_scene::ubuffers[b];
	vb = som_scene::vbuffers[b];

	ub->Lock(lockf,(void**)&up,0); upp = up;
	vb->Lock(lockf,(void**)&vp,0); vpp = vp;

	int vg = -1; //volume group

	for(t=tt;t;t=t->next)
	{
		mode = t->mode();

		if(mode<=1) //tile/sprite?
		{
			//vbuffer is really 4096
			//if(ubN>896-3||ibN>2688-3)
			if(ubN>som_scene::vbuffer_size-3||ui>4096*3-3)
			{
				break; //flush(); //lambda
			}

			for(int j=0;j<3;j++)
			{
				auto *u = t->u[j];

				if(u->batch!=batch)
				{
					u->batch = batch;

					up[0] = u->pos[0];
					up[1] = u->pos[1];
					up[2] = u->pos[2];
					(DWORD&)up[3] = u->vc;
					up[4] = u->uv[0];
					up[5] = u->uv[1];
					up+=6;

					u->i = ubN++;
				}
				*q++ = u->i;
			}
			ui+=3;
		}
		else
		{
			if(vbN>som_scene::vbuffer_size-3||vi>4096*3-3)
			{
				break; //flush(); //lambda //EXTEND ME? //som_scene::vbuffer_size?
			}

			for(int j=0;j<3;j++)
			{
				auto *v = t->v[j];

				if(v->batch!=batch)
				{
					v->batch = batch;

					memcpy(vp,v->pos,8*sizeof(float));
					vp+=8;

					v->i = vbN++;
				}
				*q++ = v->i;
			}
			vi+=3;
		}
	}
	q = ib;
	
	ub->Unlock(); vb->Unlock();

	//if(EX::debug) //BLACK MAGIC (REMOVE ME?)
	{
		txr = ~0; 
		mat = ~0, cop = ~0;
		vs = 0, npc = ~0;
		cmp = ~0; 
		mode = -1;
		som_scene_red(0); som_scene_gamma_n = false;
		som_scene_lit = 0;
		som_scene_volume = 0;		
		som_hacks_primode = fog?4:0;
	}

	auto *tn = t;
	for(ui=vi=0,t=tt;t!=tn;t=t->next,(mode<=1?ui:vi)+=3)
	{	
		draw_tries++;
				
		if(t==fogt) //kill fog? 	
		{
			flush();

			fog = false;
			
			som_hacks_primode = 0;

			mode = -1; cmp = txr = ~0;
		}

		if(mode!=t->mode())
		{
			flush(); //lambda

			mode = t->mode();

			if(mode) //lit 
			{
				mat = ~0; cop = ~0;

				//2022: this should be off (not on?) does som.hacks.cpp manage this?
				//assert(sss::lighting_current);
			//	DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_LIGHTING,1); //1?
				som_scene_lighting(!fog);
			}
			else //emulating som_scene_413F10?
			{
				sss::setss(cop=0);

				//2022: this should be off (not on?) does som.hacks.cpp manage this?
				//assert(sss::lighting_current);
			//	DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_LIGHTING,0); //1?
				som_scene_lighting(0);

				som_scene_lit = 0;
												
				if(npc)
				{
					npc = 0;

					som_hacks_skyconstants[2] = npc?-1:1; 
					DDRAW::pset9(som_hacks_skyconstants);

					som_scene_gamma_n = false;
				}												
			}

			if(!fog) som_hacks_primode = mode==1?5:0; //sprite
		}

		if(cmp!=t->texture_etc)
		{
			cmp = t->texture_etc;
			
			if(!mode)
			{
				blend = DX::D3DBLEND_INVSRCALPHA;

				if(SOM::material_textures)
				{
					//assert(cmp==sss::texture);
										
					if(SOM::MT*mt=(*SOM::material_textures)[cmp])
					{
				//		if(tuv=mt->mode&(8|16))
						{
				//			tu = mt->data[7]*som_scene_413F10_uv;
				//			tv = mt->data[8]*som_scene_413F10_uv;
						}
				
						if(mt->mode&0x100) blend = DX::D3DBLEND_ONE;
					}
				}

				if(txr!=t->mpx_texture)
				{				
					txr = t->mpx_texture; txr: //GOTO 
					
					auto *txr2 = SOM::L.textures[txr].texture;
					
					if(fog)
					{
						if(!txr2->queryX->knockouts)
						{
							if(sss::texture!=0xffff)
							{
								flush(); //colorkey?

								sss::texture = 0xffff;
								DDRAW::Direct3DDevice7->SetTexture(0,0);
							}
						}
						else goto txr2;
					}
					else if(1||!EX::debug) //profiling
					{
						flush(); //lambda					

						vg = som_scene_volume_select(txr,vg);

						//draw front side only (2-pass)
						som_scene_volume = som_scene_volume?1:0;

					txr2: sss::texture = txr; //!

						DDRAW::Direct3DDevice7->SetTexture(0,txr2);
					}
				}
			}
			else
			{
				auto *se = t->se;

				blend = (se->flags>>8)&0xf;

				if(!fog)
				{
					DWORD op = (se->flags>>12)&0xf;

					if(cop!=op)
					{
						flush(); //lambda

						assert(op);

						sss::setss(cop=op);
					}

					if(vs!=se->vs)
					{
						flush(); //lambda

						vs = se->vs;

						som_scene_red(vs?SOM::Versus[vs-1].EDI:0);
					}

					if(npc!=se->npc)
					{
						flush(); //lambda

						npc = se->npc;

						som_hacks_skyconstants[2] = npc?-1:1; 
						DDRAW::pset9(som_hacks_skyconstants);
					
						//WTH???
						//shouldn't matter with batching logic in 44d810
						//except som_scene_413F10 has troubles 
						som_scene_gamma_n = npc==3&&DDRAW::vshaders9[8];
					}
										
					if(se->mode!=3||!se->lit
					||!sss::lighting_desired) //2022: sky?
					{
						som_scene_lit = 0;
					}
					else if(se->worldxform[0][3]) //hack: repurposing
					{
						//this was designed for transparent elements
						//som_scene_batchelements_hold is using lit5
						//som_scene_lit = som_scene_lit5;
						som_scene_lit = som_scene_lit5+5;
						som_scene_lit[0] = se->lightselector[0];
						som_scene_lit[1] = se->lightselector[1];
						som_scene_lit[2] = se->lightselector[2];
						som_scene_lit[3] = se->worldxform[0][3]; se->worldxform[0][3] = 0;
						som_scene_lit[4] = se->worldxform[1][3]; se->worldxform[1][3] = 0;

						extern DWORD som_scene_ambient2_vset9(float[3+2]);
					//	DWORD debug = //having batching troubles
						som_scene_ambient2_vset9(som_scene_lit); //2020
					}
				}												
			
				if(mat!=se->material) 
				{
					auto &mm = SOM::L.materials[mat]; assert(mat==~0||mat<msz);

					mat = se->material; sss::material = mat;					

					auto &m = SOM::L.materials[mat]; assert(mat<msz);

					if(memcmp(&m,&mm,sizeof(m)))
					{
						flush(); //lambda

						DDRAW::Direct3DDevice7->SetMaterial((DX::D3DMATERIAL7*)(m.f+1));
					}
				}

				if(txr!=se->texture) //FINAL
				{
					txr = se->texture; goto txr; //!
				}
			}

			if(sss::destblend!=blend)
			{
				flush();

				DDRAW::Direct3DDevice7->SetRenderState
				(DX::D3DRENDERSTATE_DESTBLEND,sss::destblend=blend);
			}
		}

	//	buffer(); //lambda			
	}
	flush(); //lambda

	/*if(EX::debug) //BLACK MAGIC (REMOVE ME?)
	{
		txr = ~0; 
		mat = ~0, cop = ~0;
		vs = 0, npc = ~0;
		cmp = ~0; 
		mode = -1;
		som_scene_red(0); som_scene_gamma_n = false;
		som_scene_lit = 0;
		som_scene_volume = 0;		
		som_hacks_primode = fog&&tn?4:0;
	}*/

	if(tt=tn) goto resume; //!
		
	Debug:;

	//ub->Unlock();
	//vb->Unlock();	

	if(vs) som_scene_red(0); som_scene_gamma_n = false;

	som_scene_lit = 0;

	som_scene_volume = 0;
		
	som_hacks_primode = 0;

  //ASSUMING REBUILDING EACH FRAME//

	som_bsp_top = 0; //reset?

	for(int i=_countof(som_bsp_mem);i-->0;) //reset?
	som_bsp_mem[i].clear();

	DDRAW::Direct3DDevice7->SetRenderState(DX::D3DRENDERSTATE_ZBIAS,0);

	if(item) sss::pop();

	EX::dbgmsg("bsp draw calls: %d/%d",draw_calls,draw_tries);
}

/*////// test code for presplitting (x2mdl should do this) ////////

	#ifdef NDEBUG 
	#error remove me
	#endif
struct som_bsp_vert
{
	union
	{
		DWORD ab;

		struct{ WORD a,b; };
	};

	float t,p[3]; WORD i,j;

	void set(WORD aa, WORD bb, float x[3], float y[3], float tt, WORD tri=0xffff)
	{
		a = aa; b = bb; t = tt; j = tri;

		if(b<a&&tri==0xffff)
		{
			std::swap(a,b); std::swap(x,y); t = 1-t; 
		}

		for(int i=3;i-->0;) p[i] = x[i]+(y[i]-x[i])*t; //lerp
	}

	bool operator<(const som_bsp_vert &cmp)const
	{
		if(int d=(int)j-(int)cmp.j) return d<0;

		if(j==0xffff)
		{
			if(DWORD d=ab-cmp.ab) return d<0; return t<cmp.t;
		}
		else
		{
			for(int i=3;i-->0;)		
			if(float d=p[i]-cmp.p[i]) return d<0; return false;
		}
	}
	bool operator==(const som_bsp_vert &cmp)const
	{
		if(j!=cmp.j) return false;
		
		if(j==0xffff) return ab==cmp.ab&&fabsf(t-cmp.t)<0.0001f; //DICEY

		for(int i=3;i-->0;) 
		if(fabsf(p[i]-cmp.p[i])>0.001f) return false; return true;
	}

	static bool i_pred(const som_bsp_vert &a, const som_bsp_vert &b)
	{
		return a.i<b.i;
	}
};
static struct som_bsp_ext_t
{
	int split(int,som_bsp_p*,WORD,int);

	som_bsp_ext_t()
	{
		word.reserve(1024);
		cuts.reserve(256);
		abcd.reserve(1024);
		vert.reserve(256);
	}
	std::vector<WORD> word;
	std::vector<som_bsp_vert> vert;
	std::vector<som_BSP::cut> cuts;
	std::vector<som_bsp_p> abcd;
	som_BSP *f(som_MDO::data &mdo);
}*som_bsp_ext_buf = 0;
extern void som_bsp_ext_clear()
{
	delete som_bsp_ext_buf; som_bsp_ext_buf = 0;
}
extern som_BSP *som_bsp_ext(som_MDO::data &mdo)
{
	if(!som_bsp_ext_buf) som_bsp_ext_buf = new som_bsp_ext_t;

	return som_bsp_ext_buf->f(mdo);
}
som_BSP *som_bsp_ext_t::f(som_MDO::data &mdo)
{
	//ALGORITHM
	//the goal is to split every triangle 
	//along every triangle's plane (yeah)

	int k,kN = mdo.chunk_count;

	auto *mats = mdo.materials_ptr;

	int sz; for(sz=0,k=kN;k-->0;)
	{
		auto &el = mdo.chunks_ptr[k];
		if(!el.blendmode&&mats[el.matnumber][3]>=1.0f) //SAME		
		continue;

		sz+=el.ndexcount/3;
	}
	abcd.resize((size_t)sz);
	for(k=kN;k-->0;)
	{
		auto &el = mdo.chunks_ptr[k];
		if(!el.blendmode&&mats[el.matnumber][3]>=1.0f) //SAME		
		continue;

		auto *vb = el.vb_ptr;		
		auto *ip = el.ib_ptr;
		for(int i=el.ndexcount/3;i-->0;ip+=3)
		{
			auto &p = abcd[--sz];
			p.init_abcd(vb[ip[0]].pos,vb[ip[1]].pos,vb[ip[2]].pos);
			//removes more on sort/unique
			if(p.d<0) for(int i=4;i-->0;) p.abc[i] = -p.abc[i];
		}
	}
	std::sort(abcd.begin(),abcd.end());
	abcd.erase(std::unique(abcd.begin(),abcd.end(),&som_bsp_p::eq),abcd.end());
	if(abcd.size()<=1) return 0;

	bool keep = false; word.clear();
	
	auto *pp = abcd.data(), *d = pp+abcd.size();
	for(k=kN;k-->0;)
	{
		auto &el = mdo.chunks_ptr[k];
		if(!el.blendmode&&mats[el.matnumber][3]>=1.0f) //SAME
		{
			concave:
			too_big:
			//WORD placeholder[2] = {1}; //32-bit
			WORD placeholder[4] = {1}; //64-bit
			//word.insert(word.end(),placeholder,placeholder+2);
			word.insert(word.end(),placeholder,placeholder+4);
			continue;
		}

		size_t os = word.size();
		size_t ts = os+sizeof(som_BSP)/sizeof(WORD);
		word.resize(ts); 

		//NOTE: all this memory is thrown away but it
		//represents preexisting vertices and dummies
		size_t vc = el.vertcount;
		vert.resize(vc);

		//gotta do this somewhere
		cuts.clear();

		auto *vb = el.vb_ptr;		
		auto *ip = el.ib_ptr;
		for(int i=el.ndexcount/3;i-->0;ip+=3)
		{
			float *p0 = vb[ip[0]].pos;
			float *p1 = vb[ip[1]].pos;
			float *p2 = vb[ip[2]].pos;

			int t = 0;
			for(auto*p=pp;p<d;p++)
			{
				som_bsp_e e = p->side(p0,p1,p2);

				if(e.result!=p->BOTH) continue;

				if(!t) //HACK: make a dummy
				{
					t = 3;

					auto *vp = vert.data();
					vp[ip[0]].set(ip[0],ip[1],p0,p1,0);
					vp[ip[1]].set(ip[1],ip[2],p1,p2,0);
					vp[ip[2]].set(ip[2],ip[0],p2,p0,0);

					word.insert(word.end(),ip,ip+3);
				}
				t+=split(t,p,(WORD)i,vc);

				if(vert.size()>10000) //65535?
				{
					//assert(0);

					word.resize(os); goto too_big;
				}
			}
			if(t>3)
			{
				WORD *bp = &word[word.size()-t];
			
				#ifdef NDEBUG
				#error x2mdl?
				#endif
				//TODO: x2mdl should search for triangle with
				//the most original indices
				for(int j=3;j-->0;) if(bp[j]>=vc)
				{
					som_BSP::cut c = {i*3+j,(WORD)bp[j]};
					cuts.push_back(c);
				}

				//move last triangle into the hole
				WORD *ep = bp+t-3;
				for(int j=3;j-->0;) bp[j] = ep[j];
				word.resize(word.size()-3);
			}
			else assert(t==0);
		}
		if(ts==word.size())
		{
			word.resize(os); goto concave;
		}
		keep = true;

		//removing duplication
		auto *v = vert.data()+vc;
		sz = (int)vert.size()-vc;
		for(int i=sz;i-->0;)
		v[i].i = (WORD)i;
		if(1) //DEBUGGING
		{
			//NOTE: stable_sort could be avoided by factoring i into
			//operator< but either way the earlier vertices need to
			//be preserved with unsorting
			std::stable_sort(vert.begin()+vc,vert.end());
			for(int i=sz-1;i-->0;)
			if(v[i]==v[i+1]) v[i+1].ab = 0;
			//have to restore them now so interpolation
			//happens on existing vertices
			std::sort(vert.begin()+vc,vert.end(),&som_bsp_vert::i_pred);
		}
		int j = 0;
		for(int i=0;i<sz;i++) 
		{
			//with unsort these should be equal
			//v[v[i].i].j = (WORD)(vc+j);
			v[i].j = (WORD)(vc+j);

			if(v[i].ab)
			{
				auto &vj = v[j++];
				auto &vi = v[i];

				assert(vi.a<vc+i&&vi.b<vc+i);

				vj.a = vi.a<vc?vi.a:v[vi.a-vc].j;
				vj.b = vi.b<vc?vi.b:v[vi.b-vc].j;
				vj.t = vi.t;

				assert(vj.a<vc+j&&vj.b<vc+j);
			}
			else v[i].j-=1; //j is off by 1
		}

		int ti = word.size()-ts; //!
		if(ti&1) word.push_back(0); //ALIGNMENT
		size_t vs = word.size();
		word.resize(vs+j*sizeof(som_BSP::vert)/sizeof(WORD));
		//cuts is last because of a dummy terminator
		size_t cs = word.size();
		std::sort(cuts.begin(),cuts.end());
		WORD *wp = (WORD*)cuts.data();		
		word.insert(word.end(),wp,wp+2*cuts.size());
		WORD term[2] = {(WORD)(el.ndexcount+ti),(WORD)vc+vert.size()};
		word.insert(word.end(),term,term+2);
		sz = word.size()-os; //hd.size		
		if(sz%4) //YUCK
		{
			sz+=2; word.insert(word.end(),term,term+2);
		}
		//assert(sz%2==0);
		assert(sz%4==0);
		//if(sz/2>65535) //fix me in x2mdl?
		if(sz/4>65535) //fix me in x2mdl?
		{
			assert(sz/4<=65535);
			word.resize(os);
			goto too_big; 
		}

		auto &hd = (som_BSP&)word[os];
		
		//hd.size = (WORD)(sz/2); //32-bit
		hd.size = (WORD)(sz/4); //64-bit?
		hd.tris = (WORD)(ti+(ti&1));		
		hd.verts = (WORD)j;
		hd.cuts = (WORD)cuts.size();

		auto *vp = (som_BSP::vert*)&word[vs];
		for(int i=0,sz=j;i<sz;i++)
		{
			vp[i].a = v[i].a;
			vp[i].b = v[i].b;
			vp[i].t = v[i].t;
		}
			v-=vc; //!!

		wp = &word[ts];
		while(ti-->0) if(wp[ti]>=vc)
		wp[ti] = v[wp[ti]].j;
		wp = &word[cs];
		for(int i=1,iN=cuts.size()*2;i<iN;i+=2)
		wp[i] = v[wp[i]].j;		
	}

	return !keep?0:(som_BSP*)memcpy
	(new WORD[word.size()],word.data(),sizeof(WORD)*word.size());
}
int som_bsp_ext_t::split(int t, som_bsp_p *p, WORD tri, int vc)
{
	int o = 0, tt = (int)word.size()-t;


		//REMINDER: there are bugs in this code I couldn't
		//work out in time

//	#error why isn't this working? //I GIVE UP??? //REMOVE ME


	for(t+=tt;tt<t;tt+=3)
	{
		auto *vp = vert.data();
		WORD *tp = word.data()+tt;

		som_bsp_vert v[3] = {vp[tp[0]],vp[tp[1]],vp[tp[2]]};

		som_bsp_e e = p->side(v[0].p,v[1].p,v[2].p);

		if(e.result!=p->BOTH) continue; //!

	//	if(!e.i0||!e.i1||!e.i2) //triangles
		if(0==(e.i0&e.i1&e.i2)) //triangles
		{
			int i0,i1,i2; if(!e.i0)
			{
				i0 = 0; i1 = 1; i2 = 2;
			}
			else if(!e.i1)
			{
				i0 = 1; i1 = 2; i2 = 0;
			}
			else if(!e.i2)
			{
				i0 = 2; i1 = 0; i2 = 1;
			}
			else //MEMORY LEAK?
			{
				assert(0); continue; //2022
			}

			word.resize(word.size()+3);
			WORD *tp = word.data()+tt;
			WORD *n1 = &word.back()-2;

			float t1 = p->intersection(v[i1].p,v[i2].p);

			//t->split(i0,i1,i2,n1,t1);
			{
				//som_bsp_lerp(3,n1[2],v[i1],v[i2],t1);
				n1[2] = vert.size();
				vert.resize(vert.size()+1);
				WORD tri2 = tp[i1]<vc&&tp[i2]<vc?0xffff:tri;
				vert.back().set(tp[i1],tp[i2],v[i1].p,v[i2].p,t1,tri2);
		
				n1[0] = tp[i0]; n1[1] = tp[i1];
	
				tp[i1] = n1[2];
			}

			o+=3;
		}
		else //triangle and quadrangle (or 3 triangles IOW)
		{
			int i0,i1,i2; if(e.i0==e.i1)
			{
				i0 = 2; i1 = 0; i2 = 1;
			}
			else if(e.i0==e.i2)
			{
				i0 = 1; i1 = 2; i2 = 0;
			}
			else if(e.i1==e.i2)
			{
				i0 = 0; i1 = 1; i2 = 2;
			}
			else //MEMORY LEAK?
			{
				assert(0); continue; //2022
			}

			word.resize(word.size()+6);
			WORD *tp = word.data()+tt;
			WORD *n1 = &word.back()-5;
			WORD *n2 = n1+3;

			float t1 = p->intersection(v[i0].p,v[i1].p);
			float t2 = p->intersection(v[i0].p,v[i2].p);

			//t->split2(i0,i1,i2,n1,n2,t1,t2);
			{
				//som_bsp_lerp(3,n1[0],v[i0],v[i1],t1);
				//som_bsp_lerp(3,n2[2],v[i0],v[i2],t2);
				n1[0] = vert.size();
				n2[2] = vert.size()+1;
				vert.resize(vert.size()+2);
				WORD tri2 = tp[i0]<vc&&tp[i1]<vc?0xffff:tri;
				WORD tri3 = tp[i0]<vc&&tp[i2]<vc?0xffff:tri;
				auto *b_1 = &vert.back()-1;
				b_1[0].set(tp[i0],tp[i1],v[i0].p,v[i1].p,t1,tri2);
				b_1[1].set(tp[i0],tp[i2],v[i0].p,v[i2].p,t2,tri3);
	
				n1[1] = tp[i1]; n1[2] = tp[i2];

				n2[0] = n1[0]; n2[1] = tp[i2];

				tp[i1] = n1[0]; tp[i2] = n2[2];
			}

			o+=3+3;
		}
	}

	return o;
}*/