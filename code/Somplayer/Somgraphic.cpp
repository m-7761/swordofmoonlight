
#include "Somtexture.pch.h"

#include "../lib/swordofmoonlight.h"

#include "Somthread.h"

//private implementation
#include "Somgraphic.inl"

#ifndef SOMGRAPHIC_TEXTURES
#define SOMGRAPHIC_TEXTURES 8
#endif

namespace Somgraphic_cpp
{	
	struct pool
	:
	public Somthread_h::account	
	{
		const Somshader *vs[Somgraphic::PROFILES];
		const Somshader *ps[Somgraphic::PROFILES];

		pool(){ memset(this,0x00,sizeof(pool)); }

		~pool()
		{
			for(int i=0;i<Somgraphic::PROFILES;i++) 
			{
				if(vs[i]->release()||ps[i]->release()) assert(0); 
			}
		}
	};

	typedef std::map<int,pool>::iterator pool_it;

	std::map<int,pool> pools;

	struct poolhelper
	{			
		poolhelper(int &p) : pool(p)
		{
			Somthread_h::section cs; pools[pool];
		}

		~poolhelper() 
		{
			Somthread_h::section cs;

			if(!Somgraphic_cpp::pools[pool])
			{											
				Somgraphic_cpp::pools.erase(pool);
			}
		}

		inline operator int(){ return pool; } 

	private: //const
		
		const int &pool;
	};
}

//private
Somgraphic::Somgraphic(const Somgraphic &cp)
{
	/*do not enter*/ assert(0); 
}

//private
Somgraphic::Somgraphic(int pool) : Somgraphic_inl(pool) 
{
	width = height = 0; 

	Somthread_h::section cs; //Somtexture.cpp//

	Somgraphic_cpp::pools[pool]++; 
}

//private
Somgraphic::~Somgraphic() //called inplace by Somgraphic_open/edit
{	
	/*Somgraphic_cpp::pools[pool]--; //See Somgraphic::release()*/

	width = height = 0; //See ~Somgraphic_inl()
}

void Somgraphic_inl::construct::fit(size_t n)
{
	size_t picture_s = picture.size(); 

	if(n>picture_s) 
	{
		size_t x = n-picture_s;

		if(!picture_width)
		{
			if(!width)
			{
				width = 1;

				if(~featureset&SECTION) graphic->width = 1;
			}

			picture_width = width;
		}
		else assert(width);

		Sominstant **p = new Sominstant*[x*picture_width];

		Somthread_h::writelock wl = rwlock;

		memory.push_back(memset(p,0x00,sizeof(void*)*x*picture_width));

		for(size_t j=0;j<x;j++) picture.push_back(p+j*picture_width);
	}
	
	height = n; if(~featureset&SECTION) graphic->height = n;
}

wchar_t *Somgraphic_inl::construct::resource(int i, int type)
{
	Somthread_h::readlock rl = rwlock;

	for(size_t j=0,s=texture.size();j<s;j++) 
	{
		reference &r = texture[j]; if(r.type&type&&!i--) return r.resource;
	}
	return 0; 
}

const Somtexture *Somgraphic_inl::construct::image(int i, int type)
{
	Somthread_h::readlock rl = rwlock;

	for(size_t j=0,s=texture.size();j<s;j++) 
	{
		reference &r = texture[j]; if(r.type&type&&!i--) return r.image;
	}
	return 0;
}

int Somgraphic::count()const
{
	if(!this) return 0;

	int out = 0; 
	
	Somthread_h::readlock rl = Somgraphic_inl->rwlock;

	for(int i=0;i<height;i++) 
	{
		Sominstant **row = Somgraphic_inl->picture[i];

		if(row[0]&&!row[0]->collapsed()) out++;
	}

	return out;
}

size_t Somgraphic::list(Sominstant ***inout, size_t inout_s, size_t skip, bool ro)const
{
	if(!this) return 0;

	size_t out = 0;

	Somthread_h::readlock rl = Somgraphic_inl->rwlock;

	for(out=0;out<inout_s;out++)
	{
		size_t row = skip+out; 

		//TODO: deal with dirty animations
		
		if(row>=height)
		{				
			if(!ro) 
			{
				//todo: establish somewhere
				//Reminder: must be less than Somconsole::UNKNOWN
				const size_t list_max = 4096;

				if(skip<list_max)
				{
					if(skip+inout_s>list_max)
					{
						inout_s = list_max-skip; assert(0);
					}

					rl.suspend();
					Somgraphic_inl->fit(skip+inout_s); 
					rl.resume();
				}
			}
			else break;
		}

		if(inout) inout[out] = Somgraphic_inl->picture[row];
	}

	return out;
}	

int Somgraphic::group(int m)const
{
	if(!this) return 0;

	if(Somgraphic_inl&ANIMATE&&Somgraphic_inl->groups)
	{
		assert(0); return m; //unimplemented
	}
	else if(m<width) //trivial 
	{
		if(m>=0) return m==0?2:1; 
	}
	
	return 0;
}

int Somgraphic::id(int n, int set)const
{
	assert(0); return n; //unimplemented
}

const double *Somgraphic::scale(int n, double set)const
{
	if(!this||n<0||n>=height) return 0; 

	static double *null = 0; static size_t null_s = 0;

	if(null_s<height)
	{
		Somtexture_pch::remember(null=new double[null_s=null_s?null_s*2:8]);		
		
		for(int i=0;i<null_s;i++) null[i] = 1;
	}

	if(!this||~Somgraphic_inl&SCALING) return null;

	double **out = &Somgraphic_inl->scaling;
	
	if(!*out&&height&&set>=0)
	{
		double *set = *out = new double[height]; 
		
		for(int i=0;i<width;i++) *set++ = 1.0;

		Somthread_h::writelock wl = Somgraphic_inl->rwlock;

		Somgraphic_inl->memory.push_back(*out);
	}
	
	if(set>=0&&*out) 
	{
		(*out)[n] = set; assert(0); //todo: update instances
	}

	return *out?*out+n:null;
}	 

size_t Somgraphic::place(int n, const Sominstant **inout, size_t inout_s, bool ro)const
{
	if(!this) return 0;

	size_t out = 0;

	for(out=0;out<inout_s;out++)
	{
		size_t row = n+out; 

		if(row>=height) return out;
		
		if(!inout) continue; //paranoia	 
		
		Sominstant *i = *Somgraphic_inl->picture[row];		

		if(!ro)
		{
			if(i&&inout[out])
			{
				//TODO: matrix processing

				i->copy<4,4>(*inout[out]);

				if(width>1) //hack: todo; dirty row
				{	
					//assert(~Somgraphic_inl&ANIMATE);

					Sominstant **p = Somgraphic_inl->picture[row]+1;		
					
					for(int j=1;j<width;j++,p++) if(p[0])
					{
						if(p[0]!=p[-1]) p[0]->copy<4,4>(*inout[out]);
					}					
				}
			}
		}
		else inout[out] = i;
	}

	return out;
}

double *Somgraphic::opacity(bool ro)const
{
	if(!this||~Somgraphic_inl&OPACITY) return 0;

	double **out = &Somgraphic_inl->opacity;
	
	if(!*out&&height&&!ro)
	{
		double *set = *out = new double[height]; 
		
		for(int i=0;i<height;i++) *set++ = 1.0;

		Somgraphic_inl->memory.push_back(*out);
	}
	
	return *out;
}	 

double *Somgraphic::shadows(bool ro)const
{
	if(!this||~Somgraphic_inl&SHADOWS) return 0;

	double **out = &Somgraphic_inl->shadows;
	
	if(!*out&&height&&!ro)
	{
		double *set = *out = new double[height]; 
		
		for(int i=0;i<height;i++) *set++ = 0.0;

		Somgraphic_inl->memory.push_back(*out);
	}
	
	return *out;
}	 

const wchar_t *Somgraphic::reference(int number, int type)const
{
	return this?Somgraphic_inl->resource(number,type):0;
}	   

const Somtexture *Somgraphic::reference_texture(int number, int type, bool ro)const
{
	return this?Somgraphic_inl->image(number,type):0;
}

const Somshader *Somgraphic::reference_shader(int number, int type, bool ro)const
{
	if(!this) return 0;

	const Somshader *out = 0;

	switch(type) //simple: per profile for now
	{
	case RGB_VS: out = Somgraphic_inl->vs; break; 
	case RGB_PS: out = Somgraphic_inl->ps; break;

	default: assert(0); return 0;
	}

	if(!ro&&!out->assembly()) 
	{
		const wchar_t *main = profile_string();

		out->compile(type==RGB_VS?L"vs":L"ps",main);

		assert(out->assembly());
	}

	return out;
}

bool Somgraphic::freeze()const
{
	if(!this) return false;

	Somgraphic_inl->freezer.readlock(); return true;
}	

bool Somgraphic::frozen()const
{
	return this?Somgraphic_inl->freezer.readers:false;
}	

bool Somgraphic::thaw()const
{
	return this?!Somgraphic_inl->freezer.readunlock_if_any():false;
}

bool Somgraphic::play(int n, int id, int mode, double time)const
{
	if(!this||~Somgraphic_inl&ANIMATE) return false;

	assert(0); return 0; //unimplemented
}	   

size_t Somgraphic::pose(int n, int *id, double *t, size_t t_s)const
{
	if(!this||~Somgraphic_inl&ANIMATE) return 0;

	assert(0); return 0; //unimplemented
}	   
	
bool Somgraphic::step(int n, int id, double time)const
{
	if(!this||~Somgraphic_inl&ANIMATE) return false;

	assert(0); return 0; //unimplemented
}	     

bool Somgraphic::stop(int n, int id, double time)const
{
	if(!this||~Somgraphic_inl&ANIMATE) return false;

	assert(0); return 0; //unimplemented
}	

const Somgraphic *Somgraphic::addref()const
{
	if(!this) return 0;	

	if(Somgraphic_inl&SECTION)
	return Somgraphic_inl->graphic->addref();

	Somgraphic_inl.refs++;
	
	return this;
}	

const Somgraphic *Somgraphic::release()const
{		
	if(!this) return 0;

	if(Somgraphic_inl&SECTION)
	return Somgraphic_inl->graphic->release();

	if(--Somgraphic_inl.refs>0) return this;
	
	Somthread_h::section cs;

	int pool = Somgraphic_inl.pool; //tearoff
		
	Somgraphic_cpp::poolhelper ph(pool); delete this;

	Somgraphic_cpp::pools[ph]--; return 0;	
}	
 
int Somgraphic::refcount()const
{
	if(!this) return 0;

	if(Somgraphic_inl&SECTION)
	return Somgraphic_inl->graphic->refcount();
	return Somgraphic_inl.refs;
}

int Somgraphic::pool()const
{
	if(!this) return 0;

	if(Somgraphic_inl&SECTION)
	return Somgraphic_inl->graphic->pool();
	return Somgraphic_inl.pool;
}

size_t Somgraphic::channel(int ch[1], const Sominstant **in, int *i, size_t s, size_t skip)const
{
	if(!this) return 0; assert(ch); //unimplemented

	size_t out = 0;
	size_t col = Somgraphic_inl->tune(*ch); if(!col) return 0;

	for(size_t row=0,x=0;out<s;row++) //x: courtesy
	{
		if(row>=height) //==
		{	
			size_t cmp = Somgraphic_inl->tune(++col); 

			if(!x||!cmp||Somgraphic_inl->primitives[col]!=Somgraphic_inl->primitives[cmp]) break;

			*ch++; col = cmp; row-=height; //similar
		}

		Sominstant **n = Somgraphic_inl->picture[row];
		
		if(!n[0]||!n[0]->u.x) continue; x++;
			
		if(!n[col]||!n[col]->u.x||skip&&skip--) continue;

		if(i) i[out] = row*width+col;

		if(in) in[out] = n[col];

		out++;
	}

	return out;
}

size_t Somgraphic::texture(int ch[1], const Somtexture **in, int *i, size_t s, size_t skip)const
{
	if(!this) return 0; assert(ch); //unimplemented

	size_t col = Somgraphic_inl->tune(*ch); if(!col) return 0;

	if(skip==0&&s)
	{
		if(i) i[0] = 0; //multi-texture index
		in[0] = Somgraphic_inl->texture[Somgraphic_inl->primitives[col].texture].image; return 1;
	}
	else return 0;
}

const Somtexture *Somgraphic::model(int ch[1], const Somtexture **t, size_t s, size_t skip)const
{
	if(!this) return 0; assert(ch); //unimplemented
	
	size_t col = Somgraphic_inl->tune(*ch); if(!col) return 0;

	if(t) for(size_t i=0;i<s;i++) t[i] = 0; //unimplemented

	return Somgraphic_inl->texture[Somgraphic_inl->primitives[col].model].image;
}

const Somtexture *Somgraphic::index(int ch[1], size_t range[2], size_t cache[2], size_t s)const
{
	if(!this) return 0; assert(ch); //unimplemented
	
	size_t col = Somgraphic_inl->tune(*ch); if(!col) return 0;

	Somgraphic_inl::primitive &prim = Somgraphic_inl->primitives[col];

	if(range)
	{
		if(s>0) range[0] = prim.start;
		if(s>1) range[1] = prim.count;
		if(s>2) for(size_t i=2;i<s;i++) range[i] = 0; //paranoia
	}
	if(cache) 
	{
		if(s>0) cache[0] = prim.vstart;
		if(s>1) cache[1] = prim.vcount;
		if(s>2) for(size_t i=2;i<s;i++) cache[i] = 0; //paranoia
	}

	return Somgraphic_inl->texture[Somgraphic_inl->primitives[col].index].image;
}

const double *Somgraphic::tween()const
{
	if(!this||~Somgraphic_inl&ANIMATE) return 0;

	return Somgraphic_inl->tween;
}

int Somgraphic::vertex(int unused)const
{
	return this?Somgraphic_inl->vertex:0;
}

int Somgraphic::blendmode(int m, float f[8])const
{
	if(!this) return 0;
		
	if(m<0||m>=width)
	{			
		if(f) memset(f,0x00,sizeof(float)*8); assert(0);

		return 0;
	}

	if(f)
	if(~Somgraphic_inl&FACTORS||!Somgraphic_inl->factors)
	{
		f[0]=f[1]=f[2]=f[3]=1.0; f[4]=f[5]=f[6]=f[7]=0.0;
	}
	else 
	{
		size_t g = 8*Somgraphic_inl->primitives[m].factors;

		memcpy(f,Somgraphic_inl->factors+g,sizeof(float)*8);
	}

	return Somgraphic_inl->primitives[m].blendop;
}

int Somgraphic::material(int m)const
{
	if(!this||m<0||m>=width){ assert(0); return 0; }

	//assuming this will be sufficient for now
	return Somgraphic_inl->primitives[m].factors;
}

int Somgraphic::profile(int m)const 
{
	return this?Somgraphic_inl->profile:0; 
}		

int Somgraphic::shaders(int m)const 
{
	if(!this||m<0||m>=width){ assert(0); return 0; }

	return Somgraphic_inl->primitives[m].texture-Somgraphic_inl->shaders;
}

const Somgraphic *Somgraphic::section(int i)const
{
	if(!this||~Somgraphic_inl&ANIMATE) return 0;

	return Somgraphic_inl->section(i);
}

const Somgraphic *Somgraphic::bisect(int)const
{
	if(!this||~Somgraphic_inl&ANIMATE) return 0;

	assert(0); return 0; //unimplemented
}

static int Somgraphic_type(const wchar_t *file)
{
	if(!file) return 0;

	//if(!PathFileExistsW(file)) return 0;

	wchar_t ext[5] = {0,0,0,0,0}; 
	wcslwr(wcsncpy(ext,PathFindExtensionW(file),4)); if(*ext!='.') return 0;

	switch(ext[1])
	{
	case 'm': if(!wcscmp(ext,L".mdo")) return 'mdo';
			  if(!wcscmp(ext,L".mdl")) return 'mdl';
			  if(!wcscmp(ext,L".msm")) return 'msm';
	case 't': if(!wcscmp(ext,L".tmd")) return 'tmd';

	default: assert(0); 
	}

	return 0;
}

static int Somgraphic_pixel(int vertex)
{
	static int mdo = 0, mdl = 0, mpx = 0;

	switch(vertex)
	{
	case Somgraphic::MDO: if(mdo) return mdo; break;
	case Somgraphic::MDL: if(mdl) return mdl; break;
	case Somgraphic::MPX: if(mpx) return mpx; break;
	}

	const int hp = Somtexture::HIGHP; //32bit

	//TODO: text descriptions
	if(vertex==Somgraphic::MDO)
	{
		//          vertex    normal    coords
		int f[] = { hp,hp,hp, hp,hp,hp,	hp,hp };

		return mdo = Somtexture::register_pixel(f,L"");
	}
	if(vertex==Somgraphic::MDL) //16 stores the edit index
	{
		//           vertex          normal         coords
		int f[] = { -16,-16,-16,16, -16,-16,-16,16, 8,8,16 }; 

		return mdl = Somtexture::register_pixel(f,L"");
	}
	if(vertex==Somgraphic::MPX)
	{
		//          vertex    colour             coords
		int f[] = { hp,hp,hp, Somtexture::RGBA8, hp,hp };

		return mpx = Somtexture::register_pixel(f,L"");
	}

	assert(0); //???

	return 0;
}

static Somgraphic_inl::permanent*
Somgraphic_msm(int pool, const wchar_t *file, Somgraphic_inl::permanent *inout, bool del=true)
{
	if(!inout) return inout;

	using namespace SWORDOFMOONLIGHT;

	msm::image_t img; msm::maptofile(img,file);

	assert(0); //unimplemented

	if(!img&&del) delete inout;
	
	if(!img) inout = 0;

	msm::unmap(img);

	return inout;
}

static Somgraphic_inl::transient*
Somgraphic_box(int pool, const wchar_t *file, Somgraphic_inl::transient *inout, bool del=true)
{
	if(!inout) return inout;

	inout->vertex = Somgraphic::MDO;
	inout->profile = Somgraphic::BLENDED; //SPRITE?

	int px = Somgraphic_pixel(inout->vertex);	

	bool ro = inout->featureset&~Somgraphic_inl::EDITING;

	float box [8][3] =
	{
		{-1,+1,+1}, {+1,+1,+1}, //0 1
		 {-1,+1,-1}, {+1,+1,-1}, //2 3
				
		{-1,-1,+1}, {+1,-1,+1}, //4 5
		 {-1,-1,-1},  {+1,-1,-1}, //6 7  
	};

	int pos[6*4] = 
	{
		1,3,7,5, //right
		0,1,3,2, //top
		0,1,5,4, //back			
		0,2,6,4, //left
		4,5,7,6, //bottom
		2,3,7,6, //front
	};

	float uvs [4][2] =
	{
		{0,0}, {1,0}, {0,1}, {1,1},
	};

	short tri[6][6];

	float tnl[6*4][8]; 	

	for(int i=0,j=0;i<6;i++)
	{
		tri[i][0] = j;
		tri[i][1] = j+1;
		tri[i][2] = j+2;
		tri[i][3] = j+2;
		tri[i][4] = j+3;
		tri[i][5] = j;

		float sign = i<3?1:-1;

		for(int k=0;k<4;k++,j++)
		{
			tnl[j][0] = box[pos[j]][0];
			tnl[j][1] = box[pos[j]][1]; 
			tnl[j][2] = box[pos[j]][2]; 

			tnl[j][3] = sign*(i%3==0);
			tnl[j][4] = sign*(i%3==1);
			tnl[j][5] = sign*(i%3==2);

			tnl[j][6] = uvs[k][0];
			tnl[j][7] = uvs[k][1];
		}		
	}

	Somgraphic_inl::reference model, index;
	
	model.type = Somgraphic::XYZ; 
	index.type = Somgraphic::INDEX;
	model.image = Somtexture::open_or_edit(ro,pool,0);		
	index.image = Somtexture::open_or_edit(ro,pool,0);
	model.image->format(model.type,6*4,1,0,0,px);
	index.image->format(index.type,6*6,1,16,16,0);
	inout->texture.push_back(model);
	inout->texture.push_back(index);

	size_t p1 = 6*4; RECT r1 = {0,0,p1,1}; p1*=32; //8 floats
	size_t p2 = 6*6; RECT r2 = {0,0,p2,1}; p2*=2; //16bit integer

	model.image->buffer(model.type,tnl,p1,&r1);
	index.image->buffer(index.type,tri,p2,&r2);
		
	inout->shaders = inout->texture.size(); //hack

	inout->texture.push_back(Somgraphic_inl::reference(Somgraphic::RGB));

	inout->primitives = new Somgraphic_inl::primitive[2];
	inout->memory.push_back(inout->primitives); //lazy
	
	inout->primitives[0].channel = 1;

	Somgraphic_inl::primitive *p = inout->primitives+1;

	p->model = 1; assert(inout->texture[1].type==Somgraphic::XYZ); 
	p->index = 2; assert(inout->texture[2].type==Somgraphic::INDEX); 
		
	p->texture+=inout->shaders;

	p->vstart = 0; p->vcount = 6*4;
	p->start = 0; p->count = 6*6;

	inout->width = 2;

	return inout;
}

static Somgraphic_inl::transient*
Somgraphic_mdo(int pool, const wchar_t *file, Somgraphic_inl::transient *inout, bool del=true)
{
	if(!inout) return inout;

	inout->vertex = Somgraphic::MDO;
	inout->profile = Somgraphic::BLENDED; //SPRITE?

	int px = Somgraphic_pixel(inout->vertex);	

	bool ro = ~inout->featureset&Somgraphic_inl::EDITING;

	using namespace SWORDOFMOONLIGHT;

	//TODO: consider adding some INDEX manipulation
	//methods to Somtexture so to avoid a situation
	//like using private write (see 'v' note below)

	//v: it is necessary to overwrite the indices
	mdo::image_t img; mdo::maptofile(img,file,'v'); 
		
	mdo::channels_t &chan = mdo::channels(img);

	int m = 0, n = 0; 
	if(chan) for(int i=0;i<chan.count;i++) 
	{	
		mdo::triple_t *t = mdo::tnlvertextriples(img,i);

		if(i) for(int j=0,o=chan[i].ndexcount;j<o;j++) t[0][j]+=m; //v

		m+=chan[i].vertcount; n+=chan[i].ndexcount;
	}

	Somgraphic_inl::reference model, index;
	
	model.type = Somgraphic::XYZ; 
	index.type = Somgraphic::INDEX;
	if(m) model.image = Somtexture::open(pool,0);		
	if(n) index.image = Somtexture::open(pool,0);
	model.image->format(model.type,m,1,0,0,px);
	index.image->format(index.type,n,1,16,16,0);
	inout->texture.push_back(model);
	inout->texture.push_back(index);

	m = 0; n = 0; 
	if(chan) for(int i=0;i<chan.count;i++) 
	{
		size_t p1 = chan[i].vertcount; RECT r1 = {m,0,m+=p1,1};	p1*=32; //8 floats
		size_t p2 = chan[i].ndexcount; RECT r2 = {n,0,n+=p2,1}; p2*=2; //16bit integer
		model.image->buffer(model.type,mdo::tnlvertexpackets(img,i),p1,&r1,i!=chan.count-1);
		index.image->buffer(index.type,mdo::tnlvertextriples(img,i),p2,&r2,i!=chan.count-1);
		assert(p2%6==0);
	}	

	mdo::textures_t &tex = mdo::textures(img);	
	mdo::reference_t refs[SOMGRAPHIC_TEXTURES];

	int refs_s = mdo::texturereferences(img,refs,SOMGRAPHIC_TEXTURES);
		
	int sz = refs_s?strlen(refs[refs_s-1])+1:0;

	for(int i=1;i<refs_s;i++) sz+=refs[i]-refs[i-1];

	wchar_t *w = sz?new wchar_t[sz]:0;
	for(int i=0;i<sz;i++) w[i] = refs[0][i]; if(sz) w[sz-1] = '\0';
	if(w) inout->memory.push_back(w);

	inout->shaders = inout->texture.size(); //hack

	if(!refs_s) //dummy texture
	{
		inout->texture.push_back(Somgraphic_inl::reference(Somgraphic::RGB));
	}
	else for(int i=0;i<refs_s;i++)
	{
		Somgraphic_inl::reference texture; 
		texture.type = Somgraphic::RGB; texture.resource = w+(refs[i]-refs[0]);
		inout->texture.push_back(texture);
	}

	mdo::materials_t &mat = mdo::materials(img);	 	

	if(mat) inout->factors = mat.count?new float[mat.count*8]:0;
	if(inout->factors) memcpy(inout->factors,mat.list,mat.count*8*sizeof(float));	 
	if(inout->factors) inout->memory.push_back(inout->factors); //lazy

	if(chan) inout->primitives = new Somgraphic_inl::primitive[1+chan.count];
	if(chan) inout->memory.push_back(inout->primitives); //lazy
	
	//todo: disable from commandline 
	Somgraphic_inl::primitive merge; //hack

	int j = 1; m = 0; n = 0; 
	if(chan) for(int i=0;i<chan.count;i++)
	{
		//TODO: sort channels
		inout->primitives[j-1].channel = j;

		//primitives[0] should be empty
		Somgraphic_inl::primitive *p = inout->primitives+j;

		assert(!inout->texture[0].type); //expected to be empty

		p->model = 1; assert(inout->texture[1].type==Somgraphic::XYZ); 
		p->index = 2; assert(inout->texture[2].type==Somgraphic::INDEX); 
		
		p->texture = refs_s?chan[i].texnumber:0; 
		p->factors = mat.count?chan[i].matnumber:0;
		
		if(refs_s) assert(p->texture<refs_s+3);		
		if(mat.count) assert(p->factors<mat.count);

		if(p->texture>=refs_s) p->texture = 0;
		if(p->factors>=mat.count) p->factors = 0;

		p->texture+=inout->shaders;

		p->blendop = chan[i].blendmode<<1; 

		if(*p==merge)
		{
			Somgraphic_inl::primitive *q = p-1;

			q->vcount+=chan[i].vertcount; 
			q->count+=chan[i].ndexcount;	 				

			m+=chan[i].vertcount;
			n+=chan[i].ndexcount;

			continue; //merged
		}
		else merge = *p;
		
		p->vstart = m;
		p->vcount = chan[i].vertcount;
		m+=p->vcount;

		p->start = n;
		p->count = chan[i].ndexcount;
		n+=p->count;

		j++;
	}	
		
	if(!img&&del) delete inout;
	
	if(!img) inout = 0;

	mdo::unmap(img);

	if(inout) inout->width = j;

	return inout;
}

static Somgraphic_inl::animation*
Somgraphic_mdl(int pool, const wchar_t *file, Somgraphic_inl::animation *inout, bool del=true)
{
	if(!inout) return inout;

	using namespace SWORDOFMOONLIGHT;

	mdl::image_t img; mdl::maptofile(img,file);

	//assert(0); //unimplemented

	mdl::unmap(img);

	if(!img&&del) delete inout;
	
	if(!img) inout = 0;

	return inout;
}

static Somgraphic*
Somgraphic_open(int pool, const wchar_t *file, int edit, Somgraphic *out=0)
{	
	if(pool<=0) return 0; //TODO: relax??
		
	if(out) out->~Somgraphic();  		
	if(out) assert(out->Somgraphic_inl.pool==pool);

	edit = edit?Somgraphic_inl::EDITING:0;

	Somgraphic_inl::construct *p = file?0:new Somgraphic_inl::animation(edit);
	
	int type = file?Somgraphic_type(file):0;

	if(edit) switch(type)
	{
	case 'msm': p = Somgraphic_msm(pool,file,new Somgraphic_inl::animation(edit)); break;
	case 'mdo': p = Somgraphic_mdo(pool,file,new Somgraphic_inl::animation(edit)); break;
	case 'mdl': p = Somgraphic_mdl(pool,file,new Somgraphic_inl::animation(edit)); break;
	}		 
	if(!edit) switch(type)
	{
	case 'msm': p = Somgraphic_msm(pool,file,new Somgraphic_inl::permanent); break;
	case 'mdo': p = Somgraphic_mdo(pool,file,new Somgraphic_inl::transient); break; 
	case 'mdl': p = Somgraphic_mdl(pool,file,new Somgraphic_inl::animation); break; 
	}
	
	if(!p) return out; 

	if(!p->vs&&file)
	{
		SOMPAINT Sompaint = 0;
				
		for(size_t i=p->texture.size();i>0;i--)
		{
			if(Sompaint=p->texture[i-1].image->server()) break;
		}

		if(Sompaint)
		{
			Somthread_h::section cs;

			Somgraphic_cpp::pool &el = Somgraphic_cpp::pools[pool];

			if(!el.vs[p->profile]) //setup the builtin compiler
			{
				(el.vs[p->profile]=Somshader::open())->compile_with(0,Sompaint);
				(el.ps[p->profile]=Somshader::open())->compile_with(0,Sompaint);				
			}

			p->vs = el.vs[p->profile]->addref();
			p->ps = el.ps[p->profile]->addref();			
		}
		else assert(0||type=='mdl'); //2017: Was extracting SFX textures (TIMs.)
	}		
		
	if(!out) out = new Somgraphic(pool); 

	if(!file) p->width = 1;	out->width = p->width; 

	out->Somgraphic_inl.section = p; out->height = p->height;
	
	return p->graphic = out;
}

const Somgraphic *Somgraphic::open(int pool, const wchar_t *file, size_t file_s) //static
{
	assert(file_s==size_t(-1)); //unimplemented

	return Somgraphic_open(pool,file,!'edit'); 
}

const Somgraphic *Somgraphic::edit(int pool, const wchar_t *file, size_t file_s) //static
{
	assert(file_s==size_t(-1)); //unimplemented

	return Somgraphic_open(pool,file,'edit'); 
}
	 
bool Somgraphic::refresh(const wchar_t *file, size_t file_s, bool alt)const
{	
	if(!this) return false;

	assert(file_s==size_t(-1)); //unimplemented

	//// todo: transfer colour textures and Sominstant* table ////

		assert(0); return false; 

	//////////////////////////////////////////////////////////////

	Somgraphic_open(pool(),file,!readonly()||alt,const_cast<Somgraphic*>(this));

	return width;
}

bool Somgraphic::readonly()const
{
	return !this||~Somgraphic_inl&EDITING;
}

size_t Somgraphic::link(const Somtexture **in, size_t in_s, int type, bool ro)const
{
	if(!this||!in) return false;

	if(type!=Somgraphic::RGB&&!ro){ assert(0); return false; } //???

	size_t out = 0;

	Somthread_h::readlock rl = Somgraphic_inl->rwlock;

	std::vector<Somgraphic_inl::reference> &refs = Somgraphic_inl->texture;

	for(size_t i=0,n=refs.size();out<in_s&&i<n;out++,i++) 
	{
		while(i<n) if(refs[i].type==type)
		{
			if(!ro) //readonly
			{					
				const Somtexture *swap = refs[i].image;

				if(refs[i].image=in[out]) refs[i].image->addref(); if(swap) swap->release();
			}
			else in[out] = refs[i].image; break;
		}
		else i++;
	}

	return out;
}

