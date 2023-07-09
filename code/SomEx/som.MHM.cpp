#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

#include <vector>
#include <algorithm>

#include "Ex.output.h"
#include "SomEx.ini.h"

#include "som.game.h"
#include "som.state.h"

#define SOMVECTOR_MATH
#include "../Somplayer/Somvector.h"
#include "../lib/swordofmoonlight.h"

//2020: select KF2 variable ambient light level?
static float (*som_MHM_y)[2] = 0;
extern void som_MHM_y_init()
{
	delete[] som_MHM_y; som_MHM_y = 0;
	extern DWORD som_scene_ambient2;
	//som_logic_4079d0 needs this now
	//if(!som_scene_ambient2) return;

	som_MPX &mpx = *SOM::L.mpx->pointer;
	auto **m = (som_MHM**)mpx[SOM::MPX::mhm_pointer];
	int n = mpx[SOM::MPX::mhm_counter];
	(void*&)som_MHM_y = new float[n*2];

	while(n-->0)
	{
		som_MHM *p = m[n];
		int i = p->polies;
		float ymin = i?FLT_MAX:0, ymax = -ymin;
		while(i-->0)
		{
			ymin = min(ymin,p->poliesptr[i].box[0][1]);
			ymax = max(ymax,p->poliesptr[i].box[1][1]);
		}

		//KF2 dark village (zone 2) 16x,78y sits atop
		//a piece of tunnel that's default light wins
		//this logic is assuming if a tile is tall it
		//is a floor and ceiling and biases the floor
		float more = ymax-ymin>0.5f?-0.2f:1.0f;

		som_MHM_y[n][0] = ymin-0.1f; 
		som_MHM_y[n][1] = ymax+more; //0.1f
	}
}
//2022: som_MHM_layers_IMPRECISE_obj (below) uses this
//I'm not sure it has another use. I was thinking about
//applying it to the PC warp event since it historically
//needs a source reference layer. I'm not sure what to do
//about it :(
static int som_MHM_layers_IMPRECISE_(float pos[3], float height)
{
	assert(som_MHM_y); //crashed???

	int xz,xz3;
	if(((BYTE(__cdecl*)(FLOAT,FLOAT,int*,int*))0x415bc0)
	(pos[0],pos[2],&xz,&xz3)) 
	{
		xz+=xz3*100; xz3 = xz*3;
	}
	else return 0;

	DWORD ret = 0;

	float cmp = pos[1], cmp2 = pos[1]+height;

	som_MPX &mpx = *SOM::L.mpx->pointer;
	//auto **m = (som_MHM**)mpx[SOM::MPX::mhm_pointer];
	auto ll = (SOM::MPX::Layer*)&mpx[SOM::MPX::layer_0];
	for(int i=0;i<=6;i++)
	{
		auto &l = ll[-i];
		if(!l.tiles) continue;
		auto &tile = l.tiles[xz];
		if(tile.mhm==0xFFFF) continue;

		assert(tile.mhm<mpx[SOM::MPX::mhm_counter]); //crashed???

		//auto *p = m[tile.mhm];
		float *yy = som_MHM_y[tile.mhm];
		float y[2] = {yy[0]+tile.elevation,yy[1]+tile.elevation};

		if(y[0]<cmp&&y[1]>cmp //ideal?
		 ||y[0]>cmp&&y[0]<cmp2 //overlapping?
		 ||y[1]>cmp&&y[1]<cmp2) //overlapping?
		{
			ret|=1<<i;			
		}
	}

	return ret;
}
extern int som_MHM_layers_IMPRECISE_obj(int obj) //2021
{
	auto &ai = SOM::L.ai3[obj];
	float *pos = &ai[SOM::AI::xyz3];
	return som_MHM_layers_IMPRECISE_(pos,ai[SOM::AI::height3]);
}
extern DWORD som_MHM_ambient2(DWORD io, float pos[3+2])
{	
	int xz,xz3;
	if(((BYTE(__cdecl*)(FLOAT,FLOAT,int*,int*))0x415bc0)
	(pos[0],pos[2],&xz,&xz3)) 
	{
		xz+=xz3*100; xz3 = xz*3;
	}
	else return io;

	float cmp = pos[1], cmp2 = pos[1]+pos[4];

	float flr = -FLT_MAX; DWORD hi;
	som_MPX &mpx = *SOM::L.mpx->pointer;
	//auto **m = (som_MHM**)mpx[SOM::MPX::mhm_pointer];
	auto ll = (SOM::MPX::Layer*)&mpx[SOM::MPX::layer_0];
	for(int i=0;i<=6;i++) if(auto a=SOM::ambient2[i])
	{
		auto &l = ll[-i];
		auto &tile = l.tiles[xz];
		if(tile.mhm!=0xFFFF) //default?
		{
			//2022: lots of code is setting this to -1
			//to force refresh, which would be white if
			//alpha is set to 0xff
			//io = 0xFF000000|*(DWORD*)(a+xz3);
			io = *(DWORD*)(a+xz3);
		}
		else continue;

		//auto *p = m[tile.mhm];
		float *yy = som_MHM_y[tile.mhm];
		float y[2] = {yy[0]+tile.elevation,yy[1]+tile.elevation};

		if(y[0]<cmp&&y[1]>cmp //ideal?
		 ||y[0]>cmp&&y[0]<cmp2 //overlapping?
		 ||y[1]>cmp&&y[1]<cmp2) //overlapping?
		{
			return io;
		}		

		//default to heighest below feet? i.e. exterior?
		if(y[0]>flr&&y[0]<cmp)
		{
			flr = y[0]; hi = io;
		}
	}
	return flr==-FLT_MAX?io:hi;
}

//REMOVE ME
/*2021: DISABLING TO SUPPORT som_logic_4079d0 
//the exterior sets slopes are just below 0.5
//const double som_MHM_climb = //0.5 is 60 degrees
SOM::Climber::EXPERIMENT?0.33333:0.78539818525314331;
//const double som_MHM_climb2 = 0.78539818525314331; //4583F0
static DOUBLE &som_MHM_climbable()
{
	//this is the surface normal's Y component
	//38.242479460944857685992133288343 degrees
	//0.66745718071979400640308477328519 radians
	return *(DOUBLE*)0x4583F0; //0.78539818525314331
}*/
//2022: actually sin(0.785398) is always performed on this number
//static const float som_MHM_climb = 0&&EX::debug?0.5f:0.785398f;
//static const float som_MHM_climb = 0.70710679664085749076f; //sin(0.785398); //45 degrees
//try to give artists some leeway since full precision is 44.9999
static const float som_MHM_climb = 0.70710f;

//2018: basic layer system
#ifdef _DEBUG
static bool som_MHM_layer_aware = false;
#else
#define som_MHM_layer_aware true
#endif
static VOID __cdecl som_MHM_416cd0(FLOAT*,FLOAT*,FLOAT*,DWORD,DWORD,FLOAT,DWORD);
static BYTE __cdecl som_MHM_416a50(som_MHM*,DWORD,FLOAT*,FLOAT,FLOAT,FLOAT*,FLOAT*);
static BYTE __cdecl som_MHM_416860(som_MHM*,DWORD,FLOAT*,FLOAT,FLOAT,FLOAT*,FLOAT*);
static BYTE __cdecl som_MHM_4169a0(som_MHM*,DWORD,FLOAT*,FLOAT,FLOAT,FLOAT*,FLOAT*);
static BYTE som_MHM_415450_inner(FLOAT _1[3], FLOAT _2, FLOAT _3, DWORD _4, FLOAT _5[3], FLOAT _6[3], DWORD *_7)
{
	//2020: this complements the new clip test logic added to
	//som_MHM_416860 since the changes are maybe too accurate
	//whichever polygon goes first pushes the clip shapes out 
	//of reach. ideally the stronger influence would go first
	//instead this code iterates over the polygons in reverse
	//order every other frame

	//divide by the width of tiles (two meters)
	//NOTE: 415450 does the division as a float
	int i4 = (int)_3/2+1;

	int x,y = -100;
	if(!((BYTE(__cdecl*)(FLOAT,FLOAT,int*,int*))0x415bc0)
	(_1[0],_1[2],&x,&y)) 
	{
		//NOTE: this is failing lately??? _finite?
		//(maybe because arm roll?)
		//(it fails if out of hard range 0,0 200,200)
		//
		//I tracked this down degenerate MSM polygons
		//that are probably thanks to x2msm :(
		//these need to be eliminated at the source
		if(y==-100) //2021
		{
			assert(0); return 0; //NOTE: som_db does this
		}

		//2021: maybe this is happening at the map's edge
		//or on an empty tile, but the result looks legit
		//NOTE: the max/min below don't check both limits
		x = max(0,min(x,99)); y = max(0,min(y,99));
	}

	//NOTE: 415450 tests the map size that's fixed at 100x100
	som_MPX &mpx = *SOM::L.mpx->pointer;
	int ls = mpx[SOM::MPX::layer_selector];
	auto &l = ((SOM::MPX::Layer*)&mpx[SOM::MPX::layer_0])[ls];
	auto *m = (som_MHM**)mpx[SOM::MPX::mhm_pointer];

	//Ghidra names
	const int l2c = max(0,x-i4), v5 = min(99,x+i4);
	const int l20 = max(0,y-i4), v11 = min(99,y+i4);
	
		//this is the whole point of reimplementing
		//this algorithm (the change)
		//
		// 2022: this is no longer so necessary since
		// changing the vertical wall clipping code to
		// shrink its radius depending on how close the 
		// character is to the edge of the wall
		//
		// I'm not reverting it just yet however, though
		// in theory going backward won't necessary solve
		// things if edges consist of more than 2 polygons
		//
		/* Alternating seems to hold a slight advantage
		// but maybe the vertical wall clipping code can
		// be improved to tip the tide the other direction
		// (som_MHM_416860)
		#if 0 && defined(_DEBUG)
		bool caps = GetKeyState(VK_CAPITAL)&1;
		const bool odd = caps?1:SOM::frame&1; //reverse?
		#else
		#error convinced?
		enum{ odd=1 };
		#endif*/
		const bool odd = SOM::frame&1; //reverse?
		const int inc = odd?1:-1;
		const int b1 = odd?l2c:v5;
		const int e1 = odd?v5+1:l2c-1;
		const int b2 = odd?l20:v11;
		const int e2 = odd?v11+1:l20-1;

	for(int pass=1;pass<=4;pass<<=1)
	{
		//all of 1 must be processed first, etc.
		if(~_4&pass) continue;

		//for(i4=l20;i4<=v11;i4++) for(int v10=l2c;v10<=v5;v10++)
		for(i4=b2;i4!=e2;i4+=inc) for(int v10=b1;v10!=e1;v10+=inc)
		{
			auto &tile = l.tiles[i4*100+v10];

			if(tile.mhm==0xFFFF) continue;
		
			auto *p = m[tile.mhm];

			float e = tile.elevation;
			int r = tile.rotation;

			FLOAT pos[3] = {_1[0],_1[1],_1[2]};
			som_MHM_416cd0(pos+0,pos+1,pos+2,v10,i4,e,r);

			const int n = p->polies;
			const int b3 = odd?0:n-1;
			const int e3 = odd?n:0-1;
			//for(int i=0;i<n;i++)
			for(int i=b3;i!=e3;i+=inc)
			{
				auto *pp = p->poliesptr+i;

				//if(!(pp->type&_4)) continue;
				if(pp->type!=pass) continue;

				BYTE ret = 0; switch(pp->type)
				{
				case 1: ret = som_MHM_416860(p,i,pos,_2,_3,_5,_6); break;
				case 2: ret = som_MHM_4169a0(p,i,pos,_2,_3,_5,_6); break;
				case 4: ret = som_MHM_416a50(p,i,pos,_2,_3,_5,_6); break;
				}
				if(ret)
				{
					if(_5) 
					((VOID(__cdecl*)(FLOAT*,FLOAT*,FLOAT*,DWORD,DWORD,FLOAT,DWORD))0x416da0)
					(_5+0,_5+1,_5+2,v10,i4,e,r);
					assert(!_5||_finite(_5[0])); //x2msm?
					if(_6) 
					((VOID(__cdecl*)(FLOAT*,FLOAT*,FLOAT*,DWORD,DWORD,FLOAT,DWORD))0x416da0)
					(_6+0,_6+1,_6+2,0,0,0,r);
					assert(!_6||(_6[0]||_6[1]||_6[2])); //x2msm?
					if(_7)
					*_7 = pp->type;

					return 1;
				}
			}
		}
	}
	return 0;
}
extern BYTE _cdecl som_MHM_415450(FLOAT _1[3], FLOAT _2, FLOAT _3, DWORD _4, FLOAT _5[3], FLOAT _6[3], DWORD *_7)
{
	#ifdef _DEBUG
	som_MHM_layer_aware = true;
	#endif

	if(_2<=0||_3<=0) return 0;

	som_MPX &mpx = *SOM::L.mpx->pointer;
	int &ls = mpx[SOM::MPX::layer_selector];
	int *p = &mpx[SOM::MPX::layer_0];
	BYTE ret = 0;
	goto layer_0;
	for(;ls>=-6;ls--,p-=10) if(*p) layer_0:
	{
		if(ret=(0?((BYTE(__cdecl*)
		(FLOAT[3],FLOAT,FLOAT,DWORD,FLOAT[3],FLOAT[3],DWORD*))0x415450)
		:som_MHM_415450_inner)
		(_1,_2,_3,_4,_5,_6,_7))
		{
			//assert(ls==0||_4==10);
			break;
		}
	}
	ls = 0;
	
	#ifdef _DEBUG
	som_MHM_layer_aware = false;
	#endif
	
	return ret;
}//SAME ALGORITHM, FOR MAGIC (ALL?)
extern BYTE __cdecl som_MHM_414f20(FLOAT _1[3], FLOAT _2, FLOAT _3[3], FLOAT _4[3], void **_5)
{
	#ifdef _DEBUG
	som_MHM_layer_aware = true;
	#endif

	//this is a small value so seams aren't leaky
	//I think using _2 may add some, but it's not
	//bad
	float fix = _2*0.5f;

	//what's this? (it's the polygon pointer...
	//which makes no sense since there's no way 
	//to know which MHM it belongs to)
	//0040BA7B 6A 00                push        0 
	assert(_3&&_4&&!_5);

	BYTE ret = 0;

	//DISABLING FOR NOW, since these things
	//explode on contact (tornados could be
	//enabled to climb slopes, etc.)
	//HACK: this is the outermost subroutine
	//for magic (that I know of)
	//if(!SOM::emu) som_MHM_4159A0_mask = 0x10000; //SFX
	{	
		som_MPX &mpx = *SOM::L.mpx->pointer;
		int &ls = mpx[SOM::MPX::layer_selector];
		int *p = &mpx[SOM::MPX::layer_0];
		goto layer_0;
		for(;ls>=-6;ls--,p-=10) if(*p) layer_0:
		{
			/*if(0) //CLEAN OUT SOME OF THIS STUFF SOMETIME?
			{
				if(ret=(((BYTE(__cdecl*)
				(FLOAT*,FLOAT,FLOAT*,FLOAT*,void**))0x414F20)
				(_1,_2,_3,_4,_5)))
				{
					break;
				}
			}
			else*/ //2020: the test is buggy and full of bad code
			{
				//besides simplifying this code there's a bug in
				//the hit detection that misses collisions on the
				//seams between polygons

				//THIS CODE IS COPIED FROM som_MHM_415450_inner 

				int i4 = (int)_2/2+1;

				int x,y;
				if(!((BYTE(__cdecl*)(FLOAT,FLOAT,int*,int*))0x415bc0)
				(_1[0],_1[2],&x,&y)) 
				{
					assert(0); continue; //return 0;
				}

				//NOTE: 414F20 tests the map size that's fixed at 100x100
				auto &l = ((SOM::MPX::Layer*)&mpx[SOM::MPX::layer_0])[ls];
				auto *m = (som_MHM**)mpx[SOM::MPX::mhm_pointer];

				//Ghidra names
				const int l2c = max(0,x-i4), v5 = min(99,x+i4);
				const int l20 = max(0,y-i4), v11 = min(99,y+i4);
	
				for(i4=l20;i4<=v11;i4++) for(int v10=l2c;v10<=v5;v10++)
				{
					auto &tile = l.tiles[i4*100+v10];

					if(tile.mhm==0xFFFF) continue;

					auto *p = m[tile.mhm];

					float e = tile.elevation;
					int r = tile.rotation;

					//weird??? why is this a subroutine??
					//4165b0(p,v10,i4,e,r,_1,_2,_3,_4,_5)
					{
						FLOAT pos[3] = {_1[0],_1[1],_1[2]};
						som_MHM_416cd0(pos+0,pos+1,pos+2,v10,i4,e,r);

						for(int i=p->polies;i-->0;)
						{
							auto *pp = p->poliesptr+i;
							
							//2022: NaN warning information... when there
							//is NaN data "continue" via negativa like
							//so has real consequences. I should probably
							//reverse the expressions, especially since
							//x2mdl.dll now roots out NaN normals... from
							//degenerate triangles most likely... but that
							//could change... having SFX effects explode
							//is not such a bad way to discover NaN data
							
							//NaN warning #1
							float d1 = pp->distance(p,pos);
							if(d1<0) continue;

							float *n = p->normsptr+3*pp->normal;

							//NOTE: only place _2 is considered???
							float n2[3], pos2[3]; for(int i=3;i-->0;)
							{
								n2[i] = -n[i]*_2;
								pos2[i] = pos[i]+n2[i];
							}

							//NaN warning #2
							float d2 = pp->distance(p,pos2);
							if(d2>=0) continue;

							float *v1 = p->vertsptr+3*pp->vertsptr[0];
							float *v2 = p->vertsptr+3*pp->vertsptr[1];
							float *v3 = p->vertsptr+3*pp->vertsptr[2];

							//REMOVE ME (TEMPORARY)
							//
							// this one is ridiculous too
							//
							/*if(0) //mostly pointless
							{							
								if(!((BYTE(__cdecl*)(FLOAT*,FLOAT*,FLOAT*,
								FLOAT,FLOAT,FLOAT, FLOAT,FLOAT,FLOAT,
								FLOAT,FLOAT,FLOAT, FLOAT,FLOAT,FLOAT, FLOAT,FLOAT,FLOAT))0x446310)
								(_3,_3+1,_3+2,pos[0],pos[1],pos[2],pos2[0],pos2[1],pos2[2],
								v1[0],v1[1],v1[2],v2[0],v2[1],v2[2],v3[0],v3[1],v3[2]))
								{
									assert(0); continue; //impossible
								}
							}
							else*/ //_3 = lerp(pos,pos2,t) besides a load of crap
							{
								/*this can be n if normalization is acceptable?
								FLOAT nn[3] = {v1[0]-v3[0],v1[1]-v3[1],v1[2]-v3[2]};
								FLOAT mm[3] = {v2[0]-v3[0],v2[1]-v3[1],v2[2]-v3[2]};
								Somvector::map(nn).cross<3>(Somvector::map(mm));*/
								float *nn = n;

								//not sure how to interpret this... the point is
								//projecting the collision point onto the polygon
								float t = 
								 (nn[0]*pos[0]+nn[1]*pos[1]+nn[1]*pos[1]
								-(nn[0]*v1[0]+nn[1]*v1[1]+nn[2]*v1[2]))
								/(nn[0]*n2[0]+nn[1]*n2[1]+nn[2]*n2[2]);
								//lerp like? is t positive? (no)
								for(int i=3;i-->0;) _3[i] = t*n2[i]+pos[i];
							}

							//what does this achieve exactly?
							//without it the effects explodes midair
							/*if(0)
							{
								//REMOVE ME (TEMPORARY)
								//
								// misuses 446260 subroutine
								//
								// returns true if it makes it past the last vertex
								// (it tests all vertices)
								//
								if(!((BYTE(__cdecl*)(void*,DWORD,FLOAT,FLOAT,FLOAT))0x417470)
								(p,i,_3[0],_3[1],_3[2])) 
								continue;
							}
							else*/ //doesn't seem to be the problem
							{
								for(int i=3;i-->0;)
								pos2[i] = _3[i]+n[i];
								int j; for(v3=_3,j=pp->verts;j-->0;)
								{
									v2 = p->vertsptr+3*pp->vertsptr[j];
									{
										float cmp; /*
										{
											//this is the same? what's with the loop?
											cmp = ((FLOAT(__cdecl*)(FLOAT,FLOAT,FLOAT,
											FLOAT,FLOAT,FLOAT, FLOAT,FLOAT,FLOAT, FLOAT,FLOAT,FLOAT))0x446260)
											(pos2[0],pos2[1],pos2[2],v3[0],v3[1],v3[2],
											v1[0],v1[1],v1[2],v2[0],v2[1],v2[2]);
										}
										if(1)*/
										{
											//here I was having problems reproducing 446260
											//because I repurposed pos instead of pos2
											/*equivalent to 446260
											//(B-v3y*C-Bz)-(B-v3z*C-By)
											//(B-v3z*C-Bx)-(B-v3x*C-Bz)
											//(B-v3x*C-By)-(B-v3y*C-Bx)
											FLOAT nn[3] = {v1[0]-v3[0],v1[1]-v3[1],v1[2]-v3[2]};
											FLOAT mm[3] = {v2[0]-v1[0],v2[1]-v1[1],v2[2]-v1[2]};											
											*/
											// the rest of this algorithm would like
											// float nn[3] = normal(v1,v2,v3);
											//
											FLOAT nn[3] = {v1[0]-v3[0],v1[1]-v3[1],v1[2]-v3[2]};
											FLOAT mm[3] = {v2[0]-v3[0],v2[1]-v3[1],v2[2]-v3[2]};
											Somvector::map(nn).cross<3>(Somvector::map(mm));
											//som_MHM::Polygon::distance
											{
												float d = nn[0]*pos2[0]+nn[1]*pos2[1]+nn[2]*pos2[2];
												float dd = nn[0]*v3[0]+nn[1]*v3[1]+nn[2]*v3[2];
												//if(dd-d<0) break;
												//if(d-dd<0) break;
												float cmp2 = d-dd; //breakpoint
										//		assert(fabs(cmp-cmp2)<0.0001f);
												cmp = cmp2;												
											}
										}
										//using _2 wasn't working before but it does now?
										//if(cmp>0) break; 
										if(cmp>fix) break; //_2
									}
									v1 = v2;
								}
								if(j!=-1) continue;
							}

							//if(_3)
							{
								((VOID(__cdecl*)(FLOAT*,FLOAT*,FLOAT*,DWORD,DWORD,FLOAT,DWORD))0x416da0)
								(_3+0,_3+1,_3+2,v10,i4,e,r);
							}
							//if(_4)
							{
								_4[0] = n[0];
								_4[1] = n[1];
								_4[2] = n[2];
								((VOID(__cdecl*)(FLOAT*,FLOAT*,FLOAT*,DWORD,DWORD,FLOAT,DWORD))0x416da0)
								(_4+0,_4+1,_4+2,0,0,0,r);
							}
							//if(_5) *_5 = pp; //SENSELESS

							ret = 1; break;
						}
					}
				}
			}
		}
		ls = 0;
	}	
	//som_MHM_4159A0_mask = 0; som_MHM_current.clear();

	#ifdef _DEBUG
	som_MHM_layer_aware = false;
	#endif

	if(ret)	//NEW: same as som_logic_40C8E0/som_logic_40DC70
	{
		extern void som_logic_40C8E0_offset(float[3],float,const float[3]);
		som_logic_40C8E0_offset(_3,_2,_1);
	}
	
	return ret;
}

static int som_MHM_4159A0_mask = 0;
//TODO: THIS WILL BE NEEDED TO EXTRACT INFORMATION ABOUT
//THE SURROUNDING POLYGONS TO IMPLEMENT A SUPPORT-FOOT &
//OTHER ADVANCED FEATURES SHORTLY
static struct : std::vector<int> 
{
	//this designed to avoid getting stuck on a polygon
	//4159A0 just goes through the list determistically
	//if two polygons ping-pong between each other then
	//no polygons beyond those are ever evaluated. THIS
	//IS A FUNDAMENTAL DESIGN FLAW. this is a temporary
	//and partial fix
	
	int xy; //this is the current map tile

	int _layer() //2018
	{
		som_MPX &mpx = *SOM::L.mpx->pointer;
		return -mpx[SOM::MPX::layer_selector]<<13;
	}
	void push_back(int p)
	{
		//at first one polygon was kept (the last) but when
		//a model has many walls it's important to keep all
		//of them. Sloped walls are unpredicatable and will
		//not work in very basic cases 
	//	if(som_MHM_4159A0_mask) //SOM::emu/NPCs
		if(som_MHM_4159A0_mask&0x10000)
		vector::push_back(xy|_layer()|p);
	}
	bool find(int p)
	{
		return end()!=std::find(begin(),end(),xy|_layer()|p); 
	}

}som_MHM_current;

enum{ som_MHM_416a50_renormalize=0 };
static float som_MHM_416a50_slopetop;
static BYTE som_MHM_4159A0(SOM::Clipper &c)
{
	//if(EX::debug) c.mask&=~8; //no effect???

	memcpy(c.pclipos,c.pcstate,3*sizeof(float));
//	assert(_finite(c.pclipos[0]));

	//NOTE: c.height includes c.goingup. it's antigravity
	float max_height = c.pcstate[1]+c.height;
	
	float slopestop,slopestop2; int slopestop2x;

	//gathering information to be used by extensions
	c.ceiling = FLT_MAX; c.floor = c.slopefloor = -FLT_MAX; 
	c.elevator = 1;

	c.slide = c.cling = false; const float tol = 0.001f; //4583E8
	
	//004159A0 A1 3C 89 59 00       mov         eax,dword ptr ds:[0059893Ch]  
	//004159A5 83 EC 24             sub         esp,24h  
	//004159A8 85 C0                test        eax,eax  
//	DWORD eax = *(DWORD*)0x59893C; //MPX pointer
//	assert(eax); //illustrating
	//004159AA 0F 84 BA 01 00 00    je          00415B6A  
	//004159B0 8B 4C 24 28          mov         ecx,dword ptr [esp+28h]  
	//004159B4 85 C9                test        ecx,ecx  
	//004159B6 0F 84 AE 01 00 00    je          00415B6A  
//	assert(c.pcstate); //illustrating
	//004159BC D9 44 24 2C          fld         dword ptr [esp+2Ch]  
	//004159C0 D8 1D 38 82 45 00    fcomp       dword ptr ds:[458238h]  
	//004159C6 DF E0                fnstsw      ax  
	//004159C8 F6 C4 41             test        ah,41h  
	//004159CB 0F 85 99 01 00 00    jne         00415B6A  
	//som_state_haircut
	if(c.height<=0) return false; //assert(c.height>0);	
	//004159D1 D9 44 24 30          fld         dword ptr [esp+30h]  
	//004159D5 D8 1D 38 82 45 00    fcomp       dword ptr ds:[458238h]  
	//004159DB DF E0                fnstsw      ax  
	//004159DD F6 C4 41             test        ah,41h  
	//004159E0 0F 85 84 01 00 00    jne         00415B6A  
	assert(c.radius>0);

	assert(!c.goingup||c.mask&8); //2022

	//POINT-OF-NO-RETURN
	if(!SOM::emu) som_MHM_4159A0_mask = c.mask|0x10000;

	//backing pcstate[0~2] and radius/mask (esp+38/44h)
	//004159E6 8B 01                mov         eax,dword ptr [ecx]  
	//004159E8 8B 51 04             mov         edx,dword ptr [ecx+4]  
	//004159EB 53                   push        ebx  
	//004159EC 55                   push        ebp  
	//004159ED 8B 6C 24 38          mov         ebp,dword ptr [esp+38h]  
	//004159F1 56                   push        esi  
	//004159F2 89 44 24 18          mov         dword ptr [esp+18h],eax  
	//004159F6 8B 41 08             mov         eax,dword ptr [ecx+8]  
	//004159F9 57                   push        edi  
	//004159FA 8B 7C 24 44          mov         edi,dword ptr [esp+44h]  
	//bl IS returnED
	//004159FE 32 DB                xor         bl,bl  
	//00415A00 89 54 24 20          mov         dword ptr [esp+20h],edx  
	//00415A04 89 44 24 24          mov         dword ptr [esp+24h],eax  
	float height = c.height;
	float radius = c.radius; //ebp 	
	DWORD edi = c.mask; BYTE bl = 0; 
	/*this lets the current climbable slope push away from itself
	//NOTE: "elevator" pushes the clip position up, so that slopes
	//never touch each other, so that normal clip tests won't work
	if(c.mask==5) bl = SOM::climber.push_back(c.pclipos,c.radius);*/
	//00415A08 33 F6                xor         esi,esi
	int esi = 0; do //for(int esi=0;esi<8;esi++,bl=1)
	{
		/*call 415450 (black box for now)
		00415A0A 8D 4C 24 38          lea         ecx,[esp+38h]  
		00415A0E 51                   push        ecx  
		00415A0F 8B 4C 24 40          mov         ecx,dword ptr [esp+40h]  
		00415A13 8D 54 24 14          lea         edx,[esp+14h]  
		00415A17 52                   push        edx  
		00415A18 8D 44 24 30          lea         eax,[esp+30h]  
		00415A1C 50                   push        eax  
		00415A1D 57                   push        edi  
		00415A1E 55                   push        ebp  
		00415A1F 51                   push        ecx  
		00415A20 8D 54 24 34          lea         edx,[esp+34h]  
		00415A24 52                    push        edx  
		00415A25 E8 26 FA FF FF       call        00415450  
		00415A2A 83 C4 1C             add         esp,1Ch  
		00415A2D 84 C0                test        al,al  
		00415A2F 0F 84 13 01 00 00    je          00415B48  
		*/
		DWORD polytype; FLOAT polycoord[3],polynorm[3]; 

		if(c.goingup) //extension
		{
			if(esi==0)
			{
				edi = 10; 
				
				slopestop = -FLT_MAX; slopestop2x = 0;
			}

			//REMINDER: if this is moved outside of this loop
			//the results are bumpy if running at high speeds

			//2017: this is trying to cope with stepping down 
			//off of a flat tile, onto a slope, below it
			//technically the flat tile is a platform that is
			//not stepped off of until the clip radius clears
			//it. but hill sets have flat pieces too, and that
			//is just not their expected behavior

			//REMINDER: relying on gravity to push down into a
			//slope below a platform. could need an adjustment
			//otherwise
			if(!som_MHM_415450
			(c.pclipos,height,radius/*0.001f*/,12,polycoord,polynorm,&polytype))
			{
				if(!bl&&slopestop2x) //2022
				{
					bl = 1; 

					slopestop2/=slopestop2x; //average?
					
					//NOTE: treating polytype as 2
					//assume perched on tip of a slope?
					c.pclipos[1] = slopestop2+tol;
					
					c.floor = slopestop2; //extensions
			
					if(c.pclipos[1]+height>max_height) //extension
					{
						//limit floors to the original height
						height = max_height-c.pclipos[1];
					}
				}

				goto slopeless;
			}
				
			if(polynorm[1]>som_MHM_climb)
			{
				float st = som_MHM_416a50_slopetop;
				float st2 = st+polycoord[1];

				if(st<0) //out-of-bounds?
				{
					slopestop2 = slopestop2x++?slopestop2+st2:st2;

					//need this if taking average instead of max(slopetop2)
					c.elevator = min(c.elevator,polynorm[1]);

					continue;
				}
				
				//this approach has issues with platforms above the top of the
				//slope; so som_MHM_416a50_slopetop is added
				//radius = 0.001f;
				slopestop = max(slopestop,st2);
			}
			else assert(0);
		}
		else slopeless: if(!som_MHM_415450
		(c.pclipos/*const*/,height,radius,edi,polycoord,polynorm,&polytype))
		break;

		//00415A35 8B 44 24 38          mov         eax,dword ptr [esp+38h]  
		//00415A39 48                   dec         eax  
		//00415A3A 83 F8 07             cmp         eax,7  
		//00415A3D 0F 87 F9 00 00 00    ja          00415B3C  
		//00415A43 FF 24 85 70 5B 41 00 jmp         dword ptr [eax*4+415B70h]  		
		switch(polytype)
		{
		case 2: //ceiling/floor

			/*
			00415A4A D9 44 24 14          fld         dword ptr [esp+14h]  
			00415A4E D8 1D B4 82 45 00    fcomp       dword ptr ds:[4582B4h]  
			00415A54 D9 44 24 2C          fld         dword ptr [esp+2Ch]  
			00415A58 DF E0                fnstsw      ax  
			00415A5A F6 C4 40             test        ah,40h  
			00415A5D 74 0F                je          00415A6E  
			00415A5F D8 05 E8 83 45 00    fadd        dword ptr ds:[4583E8h]  
			00415A65 D9 5C 24 20          fstp        dword ptr [esp+20h]  
			00415A69 E9 CE 00 00 00       jmp         00415B3C  
			00415A6E D8 64 24 3C          fsub        dword ptr [esp+3Ch]  
			00415A72 D8 25 E8 83 45 00    fsub        dword ptr ds:[4583E8h]  
			00415A78 D9 5C 24 20          fstp        dword ptr [esp+20h]  
			00415A7C E9 BB 00 00 00       jmp         00415B3C 
			*/
			//NOTE: this held until I added MHM to x2mdl
			//in early 2022. I'm not sure if it would be
			//an improvement to allow a slight tolerance
			//especially for ceilings (415a4e has ==1.0)
			//if(polynorm[1]!=1) //2022: guaranteed?
			if(polynorm[1]<0)
			{
				if(c.goingup) //extension
				{
					//hack: keep from repeating?
					//OBSOLETE? som_MHM_current should cover this
					//IT doesn't cover arches yet. not sure what to do now
		//			height = polycoord[1]-c.pclipos[1]-tol;
					continue; //ignore ceilings
				}

				c.pclipos[1] = polycoord[1]-c.height-tol;

				if(polycoord[1]<c.ceiling) //extensions 
				{
					c.ceiling = polycoord[1]; c.ceilingarch = -1;
				}
			}
			else if(!c.goingup||fabs(polycoord[1]-slopestop)>tol) 
			{	 
				c.pclipos[1] = polycoord[1]+tol;
				 
				if(polycoord[1]>c.floor) c.floor = polycoord[1]; //extensions

				if(c.goingup)				
				if(c.pclipos[1]+height>max_height) //extension
				{
					//limit floors to the original height
					height = max_height-c.pclipos[1];
				}
			}
			break;

		case 4: //slope/arch
		
			/*parts of this code was modifed beforehand
			00415A81 D9 44 24 14          fld         dword ptr [esp+14h]  
			00415A85 D8 1D 38 82 45 00    fcomp       dword ptr ds:[458238h]  
			00415A8B DF E0                fnstsw      ax  
			00415A8D F6 C4 41             test        ah,41h  
			00415A90 75 28                jne         00415ABA  
			00415A92 D9 44 24 14          fld         dword ptr [esp+14h]  
			00415A96 DD 05 F0 83 45 00    fld         qword ptr ds:[4583F0h]  
			00415A9C D9 FE                fsin  
			00415A9E DE D9                fcompp  
			00415AA0 DF E0                fnstsw      ax  
			00415AA2 F6 C4 01             test        ah,1  
			00415AA5 74 13                je          00415ABA  
			00415AA7 D9 44 24 2C          fld         dword ptr [esp+2Ch]  
			00415AAB D8 05 E8 83 45 00    fadd        dword ptr ds:[4583E8h]  
			00415AB1 D9 5C 24 20          fstp        dword ptr [esp+20h]  
			00415AB5 E9 82 00 00 00       jmp         00415B3C  
			00415ABA 8D 4C 24 18          lea         ecx,[esp+18h]  
			00415ABE 51                   push        ecx  
			00415ABF 8D 54 24 18          lea         edx,[esp+18h]  
			00415AC3 52                   push        edx  
			00415AC4 8D 44 24 18          lea         eax,[esp+18h]  
			00415AC8 50                   push        eax  
			00415AC9 C7 44 24 20 00 00 00 00 mov         dword ptr [esp+20h],0  
			//normalize? See "som_state_4466C0."
			00415AD1 E8 EA 0B 03 00       call        004466C0  
			00415AD6 83 C4 0C             add         esp,0Ch  
			*/
			if(polynorm[1]<0) //arch
			{
				if(c.goingup) //extension
				{	
					continue; //ignore ceilings
				}

				//SOM doesn't do this but it probably should
				//NOTE: these are approximately equal but still helps
				//2022: edi&2 is to prevent getting stuck on normal steps???
				//(I don't really understand it) (kf2 arch stairwell)
				float projection = polycoord[1]-height-tol;					
				if(projection<c.pclipos[1])
				{
					if(edi&2) c.pclipos[1] = projection; //???
				}
				
				if(polycoord[1]<c.ceiling) //extensions 
				{
					c.ceiling = polycoord[1]; c.ceilingarch = polynorm[1];
				}

				if(polynorm[1]<-0.95f) //extension
				{
					//som_MHM_current should keep from repeating
					//height-=0.01f; //hack: keep from repeating

					continue; //nearly flat arches are weird
				}
			}
			else //slope
			{
				/*0.785398 (*(double*)0x4583f0)
				//if(polynorm[1]>som_MHM_climbable()) //climbable slope
				if(polynorm[1]>sin(som_MHM_climbable())) //2022*/
				if(polynorm[1]>som_MHM_climb)
				{						
					if(polycoord[1]>c.slopefloor) //extensions
					{
						c.slopefloor = polycoord[1];
						memcpy(c.slopenormal,polynorm,3*sizeof(float));
					}

					//2017: addressing cliff side abutting slopes
					c.elevator = min(c.elevator,polynorm[1]);
					   										
					if(c.goingup) //extensions
					{
						float y = polycoord[1];
						float dy = c.pclipos[1]-y;

						//2022: going down prevents climbing
						//TODO: increase precision in 416a50
						if(!bl||dy<0)
						{
							c.pclipos[1] = y+tol;

							//2017: a flat tile beside a slope can push pclipos
							//down 0.5 meters; which is far enough to not find 
							//the flat tile's floor, and so falls through the
							//flat tile. this applies to starting points too
							if(dy>0) height+=dy;

							if(y+height>max_height)
							{
								//limit floors to the original height
								//AND correct for the above adjustments
								height = max_height-y;
							}
						}
					}
					else if(1==(~4&c.mask)) //HACK?
					{
						goto no_hit; //2022: don't return false positive?
					}

					assert(polynorm[1]>som_MHM_climb);
					break; //!!
				}
				else c.slide = true; //extensions
			}

			//som_db/rt do walls here. It doesn't make much sense. even if
			//climbing tests were to move along X/Z, the height is not the
			//same, so it seems pointless
			if(~c.mask&1) break;

			//Reminder: the goal here was (I believe) to keep the cavern
			//ceilings from pushing the player around so much
			//Note: it's now possible to remove the arches from the caves
			//but really a shallow arch should not propel with such force
			//(especially when jumping up into it)

/*somehow the 4th set's arched bridge is forming a barrier in the opposite
//direction, as if there's a wall in the middle or being moved to the side
//(NOTE: I think it could stopped by shrinking the radius in the clip test
but that's a chicken and egg problem)

			if(polynorm[1]>0) //SOM normally does this to arches too
*/			{	
				if(som_MHM_416a50_renormalize)
				{
					float zero = 0; //renormalize? 
					//Somvector.h isn't used elsewhere...
					//Somvector::map(polynorm).unit<3>();
					((void(*)(float*,float*,float*))0x4466c0)(polynorm,&zero,polynorm+2);
				}
			} 			
			//break;
		
		case 1: //walls			

			/*
			00415AD9 D9 44 24 40          fld         dword ptr [esp+40h]  
			00415ADD D8 05 EC 83 45 00    fadd        dword ptr ds:[4583ECh]  
			00415AE3 D9 C0                fld         st(0)  
			00415AE5 D8 4C 24 10          fmul        dword ptr [esp+10h]  
			00415AE9 D8 44 24 28          fadd        dword ptr [esp+28h]  
			00415AED D9 5C 24 1C          fstp        dword ptr [esp+1Ch]  
			00415AF1 D8 4C 24 18          fmul        dword ptr [esp+18h]  
			00415AF5 D8 44 24 30          fadd        dword ptr [esp+30h]  
			00415AF9 D9 5C 24 24          fstp        dword ptr [esp+24h]
			00415AFD EB 3D                jmp         00415B3C 
			*/			
			c.pclipos[0] = (radius+0.002f)*polynorm[0]+polycoord[0];
			c.pclipos[2] = (radius+0.002f)*polynorm[2]+polycoord[2];
//			assert(_finite(c.pclipos[0]));

			//TODO? if a normal test is ever done for clinging, then change
			//the standing up while falling behavior to use cling instead of
			//the limited/hack landing_2017 test
			//The arch may be at knee level
			//if(!c.cling) c.cling = polynorm[1]>=-0.1f; //0
			c.cling = true; 

			break;

	/////POSSIBLE? 415450 doesn't output 8 that I can see/////

			//NOTE: 8 is passed when climbing but I don't
			//think it's processed anywhere, except it is
			//processed elsewhere via som_MHM_4159A0_mask
			//in this file (extensions) to alter behavior
			//for climbing

		case 8: //default: //unexpected
			
			//2020: is this MAP file's checkpoint system?
			//(doesn't apply to player if so)
			/*Ghidra's code:

				// UNUSED? FUN_00415450 doesn't return 8 //
			    fVar1 = param_3 + 0.00100000;
				local_18 = fVar1 * local_24 + local_c;
				local_14 = fVar1 * local_20 + local_8;
				local_10 = fVar1 * local_1c + local_4;
				OutputDebugStringA("Hit Edge!!");
			*/

			assert(0); continue; //ja 00415B3C

			/*possible mystery case 			
			00415AFF D9 44 24 40          fld         dword ptr [esp+40h]  
			00415B03 68 08 AB 45 00       push        45AB08h  
			00415B08 D8 05 E8 83 45 00    fadd        dword ptr ds:[4583E8h]  
			00415B0E D9 C0                fld         st(0)  
			00415B10 D8 4C 24 14          fmul        dword ptr [esp+14h]  
			00415B14 D8 44 24 2C          fadd        dword ptr [esp+2Ch]  
			00415B18 D9 5C 24 20          fstp        dword ptr [esp+20h]  
			00415B1C D9 C0                fld         st(0)  
			00415B1E D8 4C 24 18          fmul        dword ptr [esp+18h]  
			00415B22 D8 44 24 30          fadd        dword ptr [esp+30h]  
			00415B26 D9 5C 24 24          fstp        dword ptr [esp+24h]  
			00415B2A D8 4C 24 1C          fmul        dword ptr [esp+1Ch]  
			00415B2E D8 44 24 34          fadd        dword ptr [esp+34h]  
			00415B32 D9 5C 24 28          fstp        dword ptr [esp+28h]
			//OutputDebugStringA("Hit Edge!!")
			00415B36 FF 15 6C 80 45 00    call        dword ptr ds:[45806Ch]  
			*/

		default: //unexpected
			
			assert(0); continue;
		} 
		//00415B3C 46                   inc         esi  
		//00415B3D 83 FE 08             cmp         esi,8  
		//00415B40 B3 01                mov         bl,1  
		//00415B42 0F 8C C2 FE FF FF    jl          00415A0A  
		bl = 1; no_hit:; //2022

	}while(++esi<16&&height>0); //8 
	
	//extensions
	if(c.ceiling==FLT_MAX) c.ceilingarch = -1; if(c.slopefloor==-FLT_MAX)
	{
		c.slopenormal[0] = 0; c.slopenormal[1] = 1; c.slopenormal[2] = 0;
	}
	if(som_MHM_4159A0_mask) //emu?
	{
		som_MHM_4159A0_mask = 0; if(bl)
		{
			bl = 0; //2021: som_logic_4079d0?

			//2021: gather layer data?
			for(auto it=som_MHM_current.begin();it<som_MHM_current.end();it++)
			bl|=1<<(*it>>13&7); assert(bl!=0);
		}
	}
	som_MHM_current.clear(); return bl;
}

//UNUSED
static void som_MHM_rotate(float &x, float &z, int aim)
{
	switch(aim)
	{
	case 1: std::swap(x,z); x = -x; break;

	case 2: x = -x; z = -z; break;

	case 3: std::swap(x,z); z = -z; break;
	}
}
/*BYTE SOM::Climber::push_back(float pos[3], float r)
{
	if(!SOM::Climber::EXPERIMENT
	||-1==SOM::climber.current) return 0;

	assert(som_MHM_current.empty());
	som_MHM_current.vector::push_back(SOM::climber.current);

	som_MHM::Polygon &p = current_MHM->poliesptr[current&0x3FFF];
	
	float *n = current_MHM->normsptr+3*p.normal;

	float nx = n[0], nz = n[2];	
	som_MHM_rotate(nx,nz,current>>14&0x3);
	
	float dist = reach(n[1]);
	r = r*pow(dist,2);
		
	float pc[3],pn[3];
	if(0==((BYTE(__cdecl*)(som_MHM*,DWORD,FLOAT*,FLOAT,FLOAT,FLOAT*,FLOAT*))0x416A50)		
	(current_MHM,current&0x3FFF,current_coords,1,r,pc,pn))
	return 0;
	som_MHM_rotate(pc[0],pc[2],current>>14&0x3);	
	pc[0]+=2*(current>>16&0xFF); 
	pc[2]+=2*(current>>24&0xFF);

	//EX::dbgmsg("push back: %f %f\n%f\n%f",r,dist,pc[0],pc[2]); 

	float zero = 0; //renormalize? 
	((void(*)(float*,float*,float*))0x4466c0)(&nx,&zero,&nz);
	
	r+=0.002f;
	pos[0] = pc[0]+r*nx; 
	pos[1]-=r*n[1];
	pos[2] = pc[2]+r*nz; return 1;
}*/

BYTE SOM::Clipper::clip(float out[3]) //PC
{		
	BYTE hit;

	/*2021: want to extend to NPC use
	if(mask&4&&mask&8) //EXPERIMENTAL //8?
	if(SOM::Climber::EXPERIMENT&&!SOM::emu)
	//som_MHM_climbable() = som_MHM_climb;
	som_MHM_climbable() = asin(som_MHM_climb);*/
	{			
	//	if(!SOM::emu)
	//	som_MHM_4159A0_mask = mask; //mask|0x8000; 
		{
			hit = som_MHM_4159A0(*this);				
		}
	//	som_MHM_4159A0_mask = 0; 
	//	som_MHM_current.clear();
	}
	/*if(SOM::Climber::EXPERIMENT)
	som_MHM_climbable() = som_MHM_climb2; //38.24 degrees
	SOM::climber.prior = SOM::climber.current;*/

	if(!hit) return false;
	if(out) memcpy(out,pclipos,sizeof(pclipos));
	return hit;	
}

//2017 //2017 //2017 //2017 //2017 //2017 //2017 //2017 //2017 //2017

//TEMPORARY
//non-PC entrypoint. 1.2.1.8 rush release
extern BYTE __cdecl som_MHM_4159A0(FLOAT *_1, FLOAT _2, FLOAT _3, DWORD _4, FLOAT *_5)
{
	//assert(som_MHM_layer_aware);

	//2021: som_logic_4079d0 depends on this to return layer data 
	if(!SOM::emu) som_MHM_4159A0_mask = _4|0x10000;
	BYTE out = ((BYTE(__cdecl*)(FLOAT*,FLOAT,FLOAT,DWORD,FLOAT*))0x4159A0)(_1,_2,_3,_4,_5);
	if(!SOM::emu) 
	{
		som_MHM_4159A0_mask = 0; if(out)
		{
			out = 0; //2021: gather layer data?
			for(auto it=som_MHM_current.begin();it<som_MHM_current.end();it++)
			out|=1<<(*it>>13&7); assert(out!=0);
		}
	}
	som_MHM_current.clear(); return out;
}

//eavesdropping 
static VOID __cdecl som_MHM_416cd0(FLOAT *_1, FLOAT *_2, FLOAT *_3, DWORD _4, DWORD _5, FLOAT _6, DWORD _7)
{
	assert(som_MHM_layer_aware);

	//_4 & _5 are 0 based
	//_6 is 0? _7 is 1/0/2? _1~_3 are xyz, transformed to local space?
	//_6 is tile elevation. _7 is rotation, 0~3
	som_MHM_current.xy = _5<<24|_4<<16; //_7<<14; //_7 is just for SOM::climber

	/*REMOVE ME? _layer owns bits 13,14,15!
	int compile[!SOM::Climber::EXPERIMENT];
	if(SOM::Climber::EXPERIMENT) som_MHM_current.xy|=_7<<14;*/

	((void(__cdecl*)(FLOAT*,FLOAT*,FLOAT*,DWORD,DWORD,FLOAT,DWORD))0x416CD0)(_1,_2,_3,_4,_5,_6,_7);				
}

//4169A0 is the type 2 (horizontal/flat) test
static BYTE __cdecl som_MHM_4169a0(som_MHM *edi, DWORD _2, FLOAT *_3, FLOAT _4, FLOAT _5, FLOAT *_6, FLOAT *_7)
{
	//assert(som_MHM_layer_aware); //MHM::clip?

	//this is the last identified type subroutine. by process of elimination

	som_MHM::Polygon &esi = edi->poliesptr[_2];
	assert(esi.type==2);	

	BYTE out = ((BYTE(__cdecl*)(som_MHM*,DWORD,FLOAT*,FLOAT,FLOAT,FLOAT*,FLOAT*))0x4169A0)(edi,_2,_3,_4,_5,_6,_7);		
	if(out) //extensions
	{			   
		if(som_MHM_current.find(_2)) 
		return 0;

		//climbing onto a floor necessarily means being on its wrong side
		//should the sense be reversed?
		//maybe better to simply filter out ceilings if below waist level?
		//if(0>esi.distance(edi,_3))
		//return 0;
		som_MHM_current.push_back(_2);
		//YUCK: I'm seeing a degenerate triangle 
		//in my KF2 project that's probably made
		//by x2msm's subdivision feature
		float *n = edi->normsptr+3*esi.normal;
		if(!n[0]&&!n[1]&&!n[2]) return 0;
	}
	return out;
}

//2017: 416A50 tests a vertical face (type 1)
static BYTE __cdecl som_MHM_416860(som_MHM *edi, DWORD _2, FLOAT *_3, FLOAT _4, FLOAT _5, FLOAT *_6, FLOAT *_7)
{
	//assert(som_MHM_layer_aware); //MHM::clip?

	//_3 is the coords
	//_4 is the height
	//_5 is the radius
	
	som_MHM::Polygon &esi = edi->poliesptr[_2]; //assert(esi.type==1);

	//need to do this for 0x416860
	if(_3[1]>esi.box[1][1]||_3[1]+_4<esi.box[0][1])
	{
		return 0; //why not X/Z???
	}

	float d = esi.distance(edi,_3);

	if(d>_5||0>d) return 0; //NOTE: 0>d is an extension
	
	float r = _5; 

	//REEVALUATE ME
	//2022: I think the arch case for this may be fixed 
	//in som_MHM_416a50, but maybe the smoothing filter
	//is optimized for this behavior 
	//enum{ by_half=1||!EX::debug }; //TESTING DISABLED
	enum{ by_half=1 };

	//emu is designed to reproduce the original behavior
	//for testing purposes
	enum{ emu=0&&EX::debug }; if(!emu&&!SOM::emu)
	{
		//TODO: it might help to disable alternating test
		//order in som_MHM_415450_inner to further refine 
		//this code

		if(d<r)
		{	
			float test = cos(asin(d/r));
			assert(test>0&&_finite(d));
			r*=test;
			r+=0.0001f; //2021 (TESTING: worry too precise?)
		}
		else assert(d==r||!_finite(d)); //what causes this?

		//REMOVE ME
		//
		// this is designed to emulate the old behavior when 
		// right on the clip plane because of approaching from
		// beneath an archway for example. currently the radius
		// is too unpleasant
		//
		if(by_half) r = min(r,_5/2);
	}
	else
	{
		//this code cuts the radius in half (it could be disabled)
		//00416E84 D8 0D 90 82 45 00    fmul        dword ptr ds:[458290h]  
		r/=2;
	}

	//WARNING
	//
	// I think maybe this subroutine expects a diameter instead of a 
	// radius, but it's called with the radius. if not it seemes like
	// some kind of hack to halve the radius
	//
	//cutout the middle man? (this mainly just computes d)
	//BYTE out = ((BYTE(__cdecl*)(som_MHM*,DWORD,FLOAT*,FLOAT,FLOAT,FLOAT*,FLOAT*))0x416860)(edi,_2,_3,_4,r,_6,_7);		
	if(!((BYTE(__cdecl*)(som_MHM*,DWORD,FLOAT*,FLOAT,FLOAT))0x416E70)(edi,_2,_3,_4,r)
	||som_MHM_current.find(_2))
	return 0;			
	som_MHM_current.push_back(_2); 
	
	float *n = edi->normsptr+3*esi.normal;
	//YUCK: I'm seeing a degenerate triangle 
	//in my KF2 project that's probably made
	//by x2msm's subdivision feature
	if(!n[0]&&!n[1]&&!n[2]) return 0;

	if(_6&&_7) //som_MHM_416E70 can't do this
	{
		int iN = esi.verts; 
		float *v = edi->vertsptr;	
		
		//need to do this for 0x416860
		float *vp = v+3*esi.vertsptr[0];
		{
			//Ghidra's variable names
			float fv8 = -n[2];
			float fv5 = n[0];
			float fv7 = ((_3[2]-vp[2])*fv5+(_3[0]-vp[0])*fv8)/(fv8*fv8+fv5*fv5);
			_6[0] = fv7*fv8+vp[0];
			_6[1] = _3[1];
			_6[2] = fv7*fv5+vp[2];
			//assert(!_6||_finite(_6[0])); //x2msm?
		}
		for(int i=3;i-->0;) 
		{
			_7[i] = n[i];
		}
		//assert(!_7||(_7[0]||_7[1]||_7[2])); //x2msm?
		if(emu||SOM::emu) return 1;

		//Ghidra's variable names
		float v20 = atan2f(n[0],-n[2]); 
		float fv8 = _3[1];
		float fv18 = sinf(v20);
		float fv19 = cosf(v20);
		float xx = -fv19*_3[0]-fv18*_3[2];
		//not dividing by 2
		//float fv11 = xx-_5, fv12 = xx+_5; 

		//IMPROVE ME (archways)
		//
		// TODO: this doesn't handle triangular holes in walls
		// including walls that aren't a right-angle quad. to
		// go to the next level it should clip to the limits
		// of the disc along y and deal with diagonal walls
		//
		float xmin = 1000, xmax = -1000;
		for(int i=0;i<iN;i++)
		{
			vp = v+3*esi.vertsptr[i];
			float x = -fv19*vp[0]-fv18*vp[2];
			//float y = vp[1];
			if(x<xmin) xmin = x;
			if(x>xmax) xmax = x;
		}
		xmin-=xx; xmax-=xx; 
		float t = xmax<0?-xmax:xmin>0?xmin:_5;
		if(t<_5)
		{
			t = sin(acos(t/_5));

			/*if(0) //TODO: should move _6 instead?
			{
				//HACK: NO LONGER NORMALIZED!
				for(int i=3;i-->0;) _7[i]*=t;
			}
			else*/
			{
				t = (1-t)*_5;

				//REMOVE ME
				//
				// see r = min(r,_5/2); notes above
				//
				if(by_half) t = min(0.5,t);

				for(int i=3;i-->0;) _6[i]-=_7[i]*t;
			}
		}
		else t = 0; //dbgmsg?

		//if(som_MHM_4159A0_mask&0x8000)
		{
			//EX::dbgmsg("%f %f (%x)",r,t,som_MHM_current.back());
		}
	}
	else assert(0); return 1;
}

////2017: 416A50 tests a slope face
//static int som_MHM_4159A0_mask = 0;
static BYTE __cdecl som_MHM_416a50(som_MHM *edi, DWORD _2, FLOAT *_3, FLOAT _4, FLOAT _5, FLOAT *_6, FLOAT *_7)
{
	//assert(som_MHM_layer_aware); //MHM::clip?

	//_3 is the face??
	//_4 is the height
	//_5 is the radius
	
	int mask = som_MHM_4159A0_mask;

	som_MHM::Polygon &esi = edi->poliesptr[_2];		
		 	
	float *n = edi->normsptr+3*esi.normal;
	float ny = n[1];
	/*0.785398 (*(double*)0x4583f0)
	//bool climbable = ny>som_MHM_climbable();
	bool climbable = ny>sin(som_MHM_climbable());*/
	bool climbable = ny>som_MHM_climb;
	if(mask&1)
	{	
		if(climbable) return 0;
		
		//if(mask&0x8000) //HACK: PC? 
		{
			//UNUSED?
			//arches could benefit by shrinking the radius too
		//	if(ny>som_MHM_climb)
			{
				//_5*=SOM::climber.x(ny);
				//return 0;
			}
		}
	}
	else if(mask&8) //8??? //8 is climbing?
	{
		//REMINDER: in theory steep slopes could dampen a fall
		//by pushing up. but I'm doubtful som_db.exe positions
		//the new Y clip position up higher than where it's at
		//TESTS SHOULD BE DONE
		//this interferes with quickly recovering from falling
		//off a slope
		// 
		// 2022: it doesn't, but should it?
		// 
		//if(~mask&2) 		
		//if(!climbable&&ny>0) return 0; //2022: ny>0? must be old???
		if(!climbable) return 0;
	}
		
	//apparently this subroutine changes behavior according to 4583F0
	//double test = som_MHM_climbable();
	//som_MHM_climbable() = som_MHM_climb2;

	//2022: there's always been a bug in 416a50 that glitches where 2
	//cliffs connect side-by-side
	BYTE out = 0; if(som_MHM_416a50_renormalize)
	{
		out = ((BYTE(__cdecl*)(som_MHM*,DWORD,FLOAT*,FLOAT,FLOAT,FLOAT*,FLOAT*))0x416A50)(edi,_2,_3,_4,_5,_6,_7);
	}
	else //416A50(edi,_2,_3,_4,_5,_6,_7)
	{
		float yb = _3[1], y, yt = yb+_4;
		float bb = esi.box[0][1], bt = esi.box[1][1]; 
		if(yt>=bb&&yb<=bt) if(climbable)
		{
			//NOTE: this just fowards to 446310
			if(out=((BYTE(__cdecl*)(som_MHM*,DWORD,FLOAT,FLOAT,FLOAT))0x417130)(edi,_2,_3[0],_3[2],_5)
			&&((BYTE(__cdecl*)(som_MHM*,DWORD,FLOAT,FLOAT,FLOAT,FLOAT,FLOAT,FLOAT,FLOAT*,FLOAT*,FLOAT*))0x417340)
			(edi,_2,_3[0],_3[1],_3[2],0,1,0,_6+0,_6+1,_6+2))
			memcpy(_7,n,3*sizeof(float));
		}
		else //som_MHM_416a50_renormalize?
		{
			//NOTE: the original code tried both yt and yb and 
			//doesn't clamp to bt,bb
			float y; if(ny<0) //arch?
			{
				y = min(yt,bt);
			}
			else y = max(yb,bb);

			_3[1] = y;
			float d = esi.distance(edi,_3);
		//	_3[1] = yb;
			if(0<=d) //NEW
			{
				float hyp = d/fabsf(ny); //trig: opp/sin=hyp

				//NOTE: 416a50 only calls this once, but
				//it's called here to avoid calling it
				//twice	or calculating this new r up top
				if(((BYTE(__cdecl*)(som_MHM*,DWORD,FLOAT,FLOAT,FLOAT))0x417130)(edi,_2,_3[0],_3[2],min(hyp,_5))) //r
				{	
					//TODO: might do this when loading the models? 417130
					//memcpy(_7,n,3*sizeof(float));
					_7[0] = n[0]; _7[1] = 0; _7[2] = n[2]; //renormalize?
					((void(*)(float*,float*,float*))0x4466c0)(_7+0,_7+1,_7+2);
					
					//EXPERIMENTAL: 32 is a secret code
					float r = _5; if(ny<0&&mask&(32|2)) //arch?
					{
						//HACK: assuming PC/NPC have a human like
						//body above the shoulders so climbing an
						//arched stairwell in kf2 won't get stuck

						//NOTE: I implemented this to try to solve
						//a glitch, but it turned out to be something
						//else ("pclipos[1] = projection")

						float head = (y-yb)/_4-0.8571429f; //1-1/7

						if(head>0) r*=1-head*6; assert(r>0); //7
					}
					float rx = _7[0]*r, rz = _7[2]*r;
					
					//NOTE: this just fowards to 4464c0
					if(((BYTE(__cdecl*)(som_MHM*,DWORD,FLOAT,FLOAT,FLOAT,FLOAT,FLOAT,FLOAT,FLOAT*,FLOAT*,FLOAT*))0x4173e0)
					(edi,_2,_3[0]-rx,y,_3[2]-rz,_3[0]+rx,y,_3[2]+rz,_6+0,_6+1,_6+2))
					{
						//HACK: this is not ideal. the vertical wall test (which this is) does a cutout
						//around the actual polygons when not square, whereas this slope test just tests
						//the bbox height and a top-down cutout test. but it needs work because I think
						//it does extra stuff that's not need and may be incorrect. it also would be more
						//correct to use _7 instead of n, but it will have to be rewritten/decomposed to
						//do that without juggling memory
						//NOTE: this is for the "elevator" system to not have to clear the entire polygon
						if(((BYTE(__cdecl*)(som_MHM*,DWORD,FLOAT*,FLOAT,FLOAT))0x416E70)(edi,_2,_3,_4,d))
						{
							out = 1; //break;

							_7[1] = ny; //HACK: signal arch or not		

							_6[1] = ny<0?yt:yb; //HACK: som_MHM_4159A0 expects this

							if(r!=_5) //head?
							{
								r/=_5; _7[0]*=r; _7[2]*=r;
							}
						}
					}
				}
			}
			_3[1] = yb; //416E70 needs this
		}
	}

	//som_MHM_climbable() = test;

	if(out)
	{	
		if(som_MHM_current.find(_2))
		return 0;

		som_MHM_current.push_back(_2);
		//YUCK: I'm seeing a degenerate triangle 
		//in my KF2 project that's probably made
		//by x2msm's subdivision feature
		//float *n = edi->normsptr+3*esi.normal;
//		if(!n[0]&&!n[1]&&!n[2]) return 0;

//		if(mask&1) //extensions
		//if(!climbable)
		{		
			/*if(0x8000&mask) //PC?
			if(SOM::Climber::EXPERIMENT)
			//if(ny>som_MHM_climb&&ny<som_MHM_climb2) 
			if(ny>som_MHM_climb&&ny<som_MHM_climb) 
			{
				if(SOM::climber.prior!=SOM::climber.current
				 ||SOM::climber.prior==-1)
				{
					SOM::climber.current = som_MHM_current.xy|_2;
					SOM::climber.current_MHM = edi;
					memcpy(SOM::climber.current_coords,_6,3*sizeof(float));
				}
			}*/
		}

		som_MHM_416a50_slopetop = esi.box[1][1]-_6[1];
	}
	return out;
}

extern void som_MHM_reprogram() //som.state.cpp
{
	/*//2017: investigating inside the clipper....
	004169F9 52                   push        edx  
	004169FA 8B 16                mov         edx,dword ptr [esi]  
	004169FC 50                   push        eax  
	004169FD 52                   push        edx  
	004169FE 53                   push        ebx  
	004169FF 51                   push        ecx  
	00416A00 E8 2B 07 00 00       call        00417130  
	00416A05 83 C4 14             add         esp,14h  
	00416A08 84 C0                test        al,al  
	*/
	//floors/ceilings
//	*(DWORD*)0x416A01 = (DWORD)som_MHM_417130-0x416A05;			
	//00416AB1 E8 7A 06 00 00       call        00417130 
	//arches? second pass?
//	*(DWORD*)0x416AB2 = (DWORD)som_MHM_417130-0x416AB6;			

	/////walls/////
	//004155C7 E8 04 17 00 00       call        00416CD0 
	*(DWORD*)0x4155C8 = (DWORD)som_MHM_416cd0-0x4155CC;	
	//004155F3 E8 68 12 00 00       call        00416860
	*(DWORD*)0x4155F4 = (DWORD)som_MHM_416860-0x4155F8;	
	/////floors/////
	//004156CD E8 FE 15 00 00       call        00416CD0
	*(DWORD*)0x4156CE = (DWORD)som_MHM_416cd0-0x4156D2;	
	//00415707 E8 94 12 00 00       call        004169A0
	*(DWORD*)0x415708 = (DWORD)som_MHM_4169a0-0x41570C;	
	/////slopes/////
	//004158A6 E8 25 14 00 00       call        00416CD0  
	*(DWORD*)0x4158A7 = (DWORD)som_MHM_416cd0-0x4158AB;	
	//004158E6 E8 65 11 00 00       call        00416A50 
	*(DWORD*)0x4158E7 = (DWORD)som_MHM_416a50-0x4158EB;	
	
  //2018  //2018  //2018  //2018  //2018  //2018  //2018  //2018

	//2018: layers for NPCs
	//0040BED0 E8 7B 95 00 00       call        00415450
	*(DWORD*)0x40BED1 = (DWORD)som_MHM_415450-0x40BED5;
	//00415A25 E8 26 FA FF FF       call        00415450 
	*(DWORD*)0x415A26 = (DWORD)som_MHM_415450-0x415A2A;
	//2018: layers for SFXs
	//0040BA8D E8 8E 94 00 00       call        00414F20
	*(DWORD*)0x40BA8E = (DWORD)som_MHM_414f20-0x40BA92;

	//if(EX::debug)
	{
	//looking for magic v MHM tests
	//som_MHM_416cd0 (translates MHM)
	//???
	//004162B9 E8 12 0A 00 00       call        00416CD0	
	*(DWORD*)0x4162ba = (DWORD)som_MHM_416cd0-0x4162be;
	//???
	//004162D4 E8 F7 09 00 00       call        00416CD0
	*(DWORD*)0x4162d5 = (DWORD)som_MHM_416cd0-0x4162d9;
	//MAGIC 4165B0 (fireball)
	//004165F2 E8 D9 06 00 00       call        00416CD0	
	*(DWORD*)0x4165f3 = (DWORD)som_MHM_416cd0-0x4165f7;

	/*runs with or without magic/NPCs present??? SEE ABOVE
	//som_MHM_417130
	//00416A00 E8 2B 07 00 00       call        00417130	
	*(DWORD*)0x416a01 = (DWORD)som_MHM_417130-0x416a05;
	//00416AB1 E8 7A 06 00 00       call        00417130	
	*(DWORD*)0x416ab2 = (DWORD)som_MHM_417130-0x416ab6;
	*/
	}

  //2020  //2020  //2020  //2020  //2020  //2020  //2020  //2020

	//RADIUS OR DIAMETER?
	//this is designed to eliminate a problem owing to the clip
	//logic being a bit of a hack. the original code extends the
	//clip plane beyond the edge of the polygon. this can cause
	//artificial obstacles if for example two diagonal walls form
	//a / \ shape that is narrower to enter than it appears. it's
	//also what causes funky cornering...
	//so this change limits the clip width in 2D to the section 
	//of the the column's circle and the plane, and also adjusts 
	//the distance the cylinder is pushed away so that the plane
	//isn't extended beyond the polygon
	/*letting som_MHM_416860 takeover for som_MHM_416E70
	//0041694F E8 1C 05 00 00       call        00416E70
	*(DWORD*)0x416950 = (DWORD)som_MHM_416E70-0x416954;*/
	//remove hack (don't divide in half)
	//00416E84 D8 0D 90 82 45 00    fmul        dword ptr ds:[458290h]  
	memset((void*)0x416E84,0x90,6);
}

extern som_MHM *som_MHM_417630(FILE *f, bool mpx)
{
	DWORD sz; if(!mpx) //2022: object MHM extension
	{
		fseek(f,0,SEEK_END); sz = (size_t)ftell(f);
		fseek(f,0,SEEK_SET);
	}
	else fread(&sz,4,1,f);

	char _buf[4096];
	char *buf = sz<=sizeof(buf)?_buf:new char[sz];
	fread(buf,sz,1,f); 

	namespace mhm = SWORDOFMOONLIGHT::mhm;
	mhm::image_t in;
	mhm::maptorom(in,buf,sz);
	mhm::header_t &hd = mhm::imageheader(in);
	mhm::vector_t *v,*n;
	mhm::face_t *p;
	mhm::index_t *i;	
	som_MHM *o; 
	SOM::Game.malloc_401500(o);
	if(int iN=mhm::imagememory(in,&v,&n,&p,&i))
	{
	//	o->zero = 0; //this isn't set... maybe for sz?
		o->refs = 0;

		o->verts = hd.vertcount;
		o->norms = hd.normcount;
		o->polies = hd.facecount;
		o->types124[0] = hd.sidecount;
		o->types124[1] = hd.flatcount;
		o->types124[2] = hd.cantcount;

		SOM::Game.malloc_401500(o->poliesptr,o->polies);
		SOM::Game.malloc_401500(o->vertsptr,3*o->verts);
		SOM::Game.malloc_401500(o->normsptr,3*o->norms);

		memcpy(o->vertsptr,v,3*o->verts*sizeof(float));
		memcpy(o->normsptr,n,3*o->norms*sizeof(float));

		for(int *k,kN,j=0,jN=o->polies;j<jN;j++,i+=kN,iN-=kN)
		{
			kN = p[j].ndexcount;
			SOM::Game.malloc_401500(k,kN); memcpy(k,i,4*kN); //int

			//UNUSED //EXTENSION?
			//
			// TODO: I mean to stash some extra data in this
			// memory in the MHM format to annotate polygons
			// 
			// WARNING: som_MHM_416a50_slopetop is using this
			//
			memcpy(o->poliesptr[j].box,p[j].box,sizeof(p[j].box));		

			o->poliesptr[j].type = p[j].clipmode;
			o->poliesptr[j].normal = p[j].normal;
			o->poliesptr[j].verts = kN;
			o->poliesptr[j].vertsptr = k;
		}		
		assert(iN==0);
	}
	else //NOTE: not returning 0 if MHM is invalid
	{
		memset(o,0x00,sizeof(*o)); assert(sz==24);
	}
	mhm::unmap(in);

	if(buf!=_buf) delete[] buf; return o; 
}

som_MHM::~som_MHM() //models_free
{		
	for(int i=polies;i-->0;)
	SOM::Game.free_401580(poliesptr[i].vertsptr);
	SOM::Game.free_401580(poliesptr);
	SOM::Game.free_401580(vertsptr);
	SOM::Game.free_401580(normsptr);
}
int som_MHM::clip(int cm, float in[5], float out[6], int restart)
{
	const bool odd = SOM::frame&1;

	Polygon *p = poliesptr;

	int inc = odd?1:-1; 
	int iit = odd?0:polies-1;
	int itt = odd?polies:-1;

	//assert(!som_MHM_4159A0_mask);
	assert(~som_MHM_4159A0_mask&0x10000);

	int i; if(restart)
	{
		switch(int m=restart>>16) //YUCK
		{
		case 4: cm = 4; break;

		default: assert(cm!=3);
		}

		i = (restart&0xffff)+inc;
	}
	else i = iit;

	do switch(cm)
	{
	default: assert(0); break;

	case 0: //ball+all? //4165b0

		for(float fix=in[3]*0.5f;i!=itt;i+=inc) //som_MHM_414f20
		{
			p = poliesptr+i;

			//NOTE: this code is adapted from SOM's code
			//which had some leaky bugs that it addressed

			//2022: NaN warning information... when there
			//is NaN data "continue" via negativa like
			//so has real consequences. I should probably
			//reverse the expressions, especially since
			//x2mdl.dll now roots out NaN normals... from
			//degenerate triangles most likely... but that
			//could change... having SFX effects explode
			//is not such a bad way to discover NaN data
							
			//NaN warning #1
			float d1 = p->distance(this,in);
			if(d1<0) continue;

			float *n = normsptr+3*p->normal;

			//NOTE: only place in[3] is considered???
			float n2[3], pos2[3]; for(int i=3;i-->0;)
			{
				n2[i] = -n[i]*in[3];
				pos2[i] = in[i]+n2[i];
			}

			//NaN warning #2
			float d2 = p->distance(this,pos2);
			if(d2>=0) continue;

			float *v1 = vertsptr+3*p->vertsptr[0];
			float *v2 = vertsptr+3*p->vertsptr[1];
			float *v3 = vertsptr+3*p->vertsptr[2];

			//not sure how to interpret this... the point is
			//projecting the collision point onto the polygon
			float hit[3],t = 
			 (n[0]*in[0]+n[1]*in[1]+n[1]*in[1]
			-(n[0]*v1[0]+n[1]*v1[1]+n[2]*v1[2]))
			/(n[0]*n2[0]+n[1]*n2[1]+n[2]*n2[2]);
			//lerp like? is t positive? (no)
			for(int i=3;i-->0;) hit[i] = t*n2[i]+in[i];
			for(int i=3;i-->0;) pos2[i] = hit[i]+n[i];
			int j; for(v3=hit,j=p->verts;j-->0;)
			{
				v2 = vertsptr+3*p->vertsptr[j];
				{
					float cmp; 
					FLOAT nn[3] = {v1[0]-v3[0],v1[1]-v3[1],v1[2]-v3[2]};
					FLOAT mm[3] = {v2[0]-v3[0],v2[1]-v3[1],v2[2]-v3[2]};
					Somvector::map(nn).cross<3>(Somvector::map(mm));
					//som_MHM::Polygon::distance
					{
						float d = nn[0]*pos2[0]+nn[1]*pos2[1]+nn[2]*pos2[2];
						float dd = nn[0]*v3[0]+nn[1]*v3[1]+nn[2]*v3[2];
						//if(dd-d<0) break;
						//if(d-dd<0) break;
						float cmp2 = d-dd; //breakpoint
						//assert(fabs(cmp-cmp2)<0.0001f);
						cmp = cmp2;												
					}
					if(cmp>fix) break; 
				}
				v1 = v2;
			}
			if(j==-1)
			{
				memcpy(out,hit,sizeof(hit));
				memcpy(out+3,n,sizeof(hit));
				return true;
			}
		}
		break;

	case 5: //1|4
	case 1: cm&=~1; //disc+wall //416860

		if(types124[0])
		for(;i!=itt;i+=inc)
		if(p[i].type==1&&som_MHM_416860(this,i,in,in[3],in[4],out,out+3))
		return 1<<16|i; break;

	case 6: //2|4
	case 2: cm&=~2; //disc+flat //4169a0

		if(types124[1])
		for(;i!=itt;i+=inc)
		if(p[i].type==2&&som_MHM_4169a0(this,i,in,in[3],in[4],out,out+3))
		return 2<<16|i; break;

	case 4: cm&=~4; //disc+slope //416a50

		if(types124[2])
		for(;i!=itt;i+=inc)
		if(p[i].type==4&&som_MHM_416a50(this,i,in,in[3],in[4],out,out+3))
		return 4<<16|i; break;

	}while(i=iit,cm); return false;
}
float som_MHM::Polygon::distance(som_MHM *edi, float v2[3])
{
	//NOTE: this is signed "distance" with negative values
	//being behind the polygon/plane
	float *n = edi->normsptr+3*normal;
	float *v = edi->vertsptr+3*vertsptr[0];
	float d = /*-*/(n[0]*v[0]+n[1]*v[1]+n[2]*v[2]);
	float dd = /*-*/(n[0]*v2[0]+n[1]*v2[1]+n[2]*v2[2]);
	return dd-d; //d-dd;
}
bool SOM::Clipper::clip(som_Obj &ai, float hit[3], float dir[3])
{
	auto *l = (som_MDL*)ai[SOM::AI::mdl3];
	auto *o = (som_MDO*)ai[SOM::AI::mdo3];
	auto *h = o?o->mdo_data()->ext.mhm:l->mdl_data->ext.mhm;
	obj_had_mhm = h!=0;
	return h?clip(h,o,l,hit,dir):true;
}
bool SOM::Clipper::clip(som_MHM *h, som_MDO *o, som_MDL *l, float hit[3], float dir[3])
{
	assert(h&&(o||l));
		
	assert(radius>0);
	assert(!goingup||mask&8); //2022

	if(height<=0) return false;

	float *x,l_x;
	float *xyz,*uvw; if(l) //MDL?
	{	
		x = l->scale;
		xyz = l->xyzuvw; uvw = l->xyzuvw+3; 		
	}
	else //MDO?
	{
		x = o->f+25;		
		xyz = o->f+13; uvw = o->f+19;
	}
	if(*x!=1) //OVERKILL?
	{
		l_x = 1/x[0];		
	}
	else x = 0;

	float out[6];
	for(int i=3;i-->0;)
	pclipos[i] = pcstate[i]-xyz[i];
	pclipos[3] = height;
	pclipos[4] = radius;
			
	bool v,uw; 
	if(uw=uvw[0]||uvw[2])
	{
		#ifdef NDEBUG
//		#error need to build a matrix
		#endif 
		assert(0); //UNFINISHED
	}

	float c,s; //if(v=uvw[1]!=0)
	{
		c = cosf(-uvw[1]);
		s = sinf(-uvw[1]);
		//SOM::rotate(pclipos,0,-uvw[1]);
		{
			float swap = pclipos[0];
			pclipos[0] = swap*c-pclipos[2]*s;
			pclipos[2] = swap*s+pclipos[2]*c;
		}
	}

	if(x) //assuming uniform
	{
		for(int i=5;i-->0;) pclipos[i]*=l_x;
	}

	bool ret = false;

	//som_MHM_416a50?
	//NOTE: not using som_MHM_current (16)
	//TODO? should 0 this before returning
	som_MHM_4159A0_mask = mask;

	int cm = mask&7; if(!cm)
	{
		ret = h->clip(cm,pclipos,out)!=0;
	}
	else //som_MHM_4159A0 like?
	{
		//NOTATION
		auto &c = *this;
		float *polycoord = out;
		float *polynorm = out+3; 
		float &height = pclipos[3]; //SHADOWING
		float &radius = pclipos[4]; //SHADOWING

		const float tol = 0.001f;

		//NOTE: c.height includes c.goingup. it's antigravity
		float max_height = c.pcstate[1]+c.height;

		float slopestop,slopestop2; int slopestop2x;

		//gathering information to be used by extensions
		c.ceiling = FLT_MAX; c.floor = c.slopefloor = -FLT_MAX; 
		c.elevator = 1;

		c.slide = c.cling = false; 

		bool sloped = h->types124[2]&&c.goingup;

		//NOTE: restart avoids using som_MHM_current
		for(int polytype,restart2,restart=0,i=0;i<16;i++) //8
		{	
			if(sloped) //extension
			{
				if(!i)
				{
					restart2 = 0; cm = 2; 

					slopestop = -FLT_MAX; slopestop2x = 0;
				}

				//REMINDER: if this is moved outside of this loop
				//the results are bumpy if running at high speeds

				//2017: this is trying to cope with stepping down 
				//off of a flat tile, onto a slope, below it
				//technically the flat tile is a platform that is
				//not stepped off of until the clip radius clears
				//it. but hill sets have flat pieces too, and that
				//is just not their expected behavior

				//REMINDER: relying on gravity to push down into a
				//slope below a platform. could need an adjustment
				//otherwise
				restart2 = h->clip(4,pclipos,out,restart2);
				if(!restart2)
				{
					if(!ret&&slopestop2x) //2022
					{
						ret = true;

						slopestop2/=slopestop2x; //average?

						//slopestop2+tol
						//assume perched on tip of a slope?
						c.pclipos[1] = slopestop2+tol;

						c.floor = slopestop2; //extensions
			
						if(c.pclipos[1]+height>max_height) //extension
						{
							//limit floors to the original height
							height = max_height-c.pclipos[1];
						}
					}

					sloped = false; goto slopeless; 
				}
				else polytype = restart2>>16;
				
				if(polynorm[1]>som_MHM_climb)
				{
					float st = som_MHM_416a50_slopetop;
					float st2 = st+polycoord[1];

					if(st<0) //out-of-bounds?
					{
						slopestop2 = slopestop2x++?slopestop2+st2:st2;

						//need this if taking average instead of max(slopetop2)
						c.elevator = min(c.elevator,polynorm[1]);

						continue;
					}
				
					//this approach has issues with platforms above the top of the
					//slope; so som_MHM_416a50_slopetop is added
					//radius = 0.001f;
					slopestop = max(slopestop,st2);
				}
				else assert(0);
			}
			else slopeless:
			{
				restart = h->clip(cm,pclipos,out,restart); //mask
				if(!restart) break;
				
				polytype = restart>>16;
			}

			switch(polytype) //TODO? breakout this logic
			{
			case 2: //ceiling/floor

				//INCOMPLETE?

				if(polynorm[1]<0)
				{
					if(c.goingup) continue; //ignore ceilings

					c.pclipos[1] = polycoord[1]-height-tol;

					if(polycoord[1]<c.ceiling) //extensions 
					{
						c.ceiling = polycoord[1]; c.ceilingarch = -1;
					}
				}
				else if(!c.goingup||fabs(polycoord[1]-slopestop)>tol)
				{
					c.pclipos[1] = polycoord[1]+tol;

					if(polycoord[1]>c.floor) c.floor = polycoord[1]; //extensions

					if(c.goingup)				
					if(c.pclipos[1]+height>max_height) //extension
					{
						//limit floors to the original height
						height = max_height-c.pclipos[1];
					}
				}
				break;

			case 4: //slope/arch

				if(polynorm[1]<0) //arch
				{
					if(c.goingup) continue; //ignore ceilings

					//SOM doesn't do this but it probably should
					//NOTE: these are approximately equal but still helps
					//2022: cm&2 is to prevent getting stuck on normal steps???
					//(I don't understand it) (kf2 arch stairwell)
					float projection = polycoord[1]-height-tol;					
					if(projection<c.pclipos[1])
					{
						if(cm&2) c.pclipos[1] = projection; //???
					}
				
					if(polycoord[1]<c.ceiling) //extensions 
					{
						c.ceiling = polycoord[1]; c.ceilingarch = polynorm[1];
					}

					if(polynorm[1]<-0.95f) //extension
					{
						//som_MHM_current should keep from repeating
						//height-=0.01f; //hack: keep from repeating

						continue; //nearly flat arches are weird
					}
				}
				else
				{
					/*0.785398 (*(double*)0x4583f0)
					//if(polynorm[1]>som_MHM_climbable()) //climbable slope
					//if(polynorm[1]>sin(som_MHM_climbable())) //2022*/
					if(polynorm[1]>som_MHM_climb)
					{
						if(polycoord[1]>c.slopefloor) //extensions
						{
							c.slopefloor = polycoord[1];
							memcpy(c.slopenormal,polynorm,3*sizeof(float));
						}

						//2017: addressing cliff side abutting slopes
						c.elevator = min(c.elevator,polynorm[1]);
					   										
						if(c.goingup) //extensions
						{
							float y = polycoord[1];
							float dy = c.pclipos[1]-y;

							//2022: going down prevents climbing
							//TODO: increase precision in 416a50
							if(!ret||dy<0)
							{
								c.pclipos[1] = y+tol;

								//2017: a flat tile beside a slope can push pclipos
								//down 0.5 meters; which is far enough to not find 
								//the flat tile's floor, and so falls through the
								//flat tile. this applies to starting points too
								if(dy>0) height+=dy;
							
								if(y+height>max_height)
								{
									//limit floors to the original height
									//AND correct for the above adjustments
									height = max_height-y;
								}
							}
						}
						else if(1==(~4&c.mask)) //HACK?
						{
							goto no_hit; //2022: don't return false positive?
						}

						assert(polynorm[1]>som_MHM_climb);
						break; //!!
					}
					else c.slide = true; //extensions
				}

				if(c.mask&1) //renormalize?
				{
					if(som_MHM_416a50_renormalize)
					{
						float zero = 0; 
						((void(*)(float*,float*,float*))0x4466c0)(polynorm,&zero,polynorm+2);
						//break;
					}
				}
				else break;

			case 1: //wall

				c.pclipos[0] = (radius+0.002f)*polynorm[0]+polycoord[0];
				c.pclipos[2] = (radius+0.002f)*polynorm[2]+polycoord[2];

				c.cling = true; break;
			}

			ret = true; no_hit:; //2022
		}
				
		//extensions
		if(c.ceiling==FLT_MAX) c.ceilingarch = -1; if(c.slopefloor==-FLT_MAX)
		{
			c.slopenormal[0] = 0; c.slopenormal[1] = 1; c.slopenormal[2] = 0;
		}	
	}

	if(!ret) return false;
	
	if(uw)
	{
		#ifdef NDEBUG
//		#error need to build a matrix
		#endif 
		assert(0); //UNFINISHED
	}
	//if(v)
	{
		//c = -c; s = -s;
		c = cosf(uvw[1]);
		s = sinf(uvw[1]);
		//SOM::rotate(out,0,uvw[1]);
		{
			float swap = out[0];
			out[0] = swap*c-out[2]*s;
			out[2] = swap*s+out[2]*c;
		}
		//SOM::rotate(out+3,0,uvw[1]);
		{
			float swap = out[3];
			out[3] = swap*c-out[5]*s;
			out[5] = swap*s+out[5]*c;
		}
		//SOM::rotate(pclipos,0,uvw[1]);
		{
			float swap = pclipos[0];
			pclipos[0] = swap*c-pclipos[2]*s;
			pclipos[2] = swap*s+pclipos[2]*c;
		}
		if(slopefloor!=-FLT_MAX)
		{
			float swap = slopenormal[0];
			slopenormal[0] = swap*c-slopenormal[2]*s;
			slopenormal[2] = swap*s+slopenormal[2]*c;
		}
	}
	
	//if(x)
	{
		float yy = xyz[1], xx = x?x[0]:1;

		for(int i=3;i-->0;) out[i]*=xx;

		for(int i=3;i-->0;) pclipos[i]*=xx;

		if(floor!=-FLT_MAX) floor = floor*xx+yy;

		if(slopefloor!=-FLT_MAX) slopefloor = slopefloor*xx+yy;

		if(ceiling!=FLT_MAX) ceiling = ceiling*xx+yy;
	}

	for(int i=3;i-->0;)
	{
		hit[i] = out[i]+xyz[i];
		dir[i] = out[3+i];

		pclipos[i]+=xyz[i];
	}
	
	return true;
}

