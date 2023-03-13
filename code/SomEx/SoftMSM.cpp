
#ifndef X2MDL_INCLUDED //2022: x2mdl.cpp?

#include <string>
#include <fstream>
#include <iostream>
#include <assert.h>

#include <vector>
#include <algorithm>

#define WIN32_LEAN_AND_MEAN	
#include <windows.h> //FindFirstFile
#include <shlwapi.h> //PathFindFileNameW
#pragma comment(lib,"shlwapi.lib")

#include "../lib/swordofmoonlight.h"
//namespace msm = SWORDOFMOONLIGHT::msm;

#ifdef _CONSOLE
static int yes_no(char *q, char def, const char *opts="Y/N") 
{
	assert(def=='Y'||def=='N');
		
	std::cout << "\n\n";
	std::cout << q << " ("<<opts<<") [" << def <<"]:"; 
	
	char ok[] = {0,0}; std::cin.getline(ok,2); 	

	if(*ok>='a'&&*ok<='z') *ok-=32;

	return !*ok?def:*ok;
}
#endif

#endif //X2MDL_INCLUDED

struct SoftMSM_unique
{	
	//msm::vertex_t *v; int reduce,remove;

	swordofmoonlight_tnl_vertex_t *v;
	
	int sum,reduce,remove;

	bool operator<(const SoftMSM_unique &cmp)const
	{
		return sum<cmp.sum;
	}
	static bool compare(float *a, float *b, int n)
	{
		while(n-->0) if(fabsf(a[n]-b[n])>0.00001f)
		return true; return false;
	}	
	//REMINDER: this was a mistake (I think???)
	///*2022: I think this should've used memcmp to
	//*match operator< but now is not used since it
	//*compiles if knocked out
	bool operator!=(const SoftMSM_unique &cmp)const
	{
		return compare(v->pos,cmp.v->pos,8);
	}
	bool operator==(const SoftMSM_unique &cmp)const
	{
		return !operator!=(cmp);
	}//*/

	static bool reduce_pred(const SoftMSM_unique &a, const SoftMSM_unique &b)
	{
		return a.reduce<b.reduce;
	}
};

extern int SoftMSM(const wchar_t *path, int mode=0)
{	
	namespace msm = SWORDOFMOONLIGHT::msm;

	//OBJECTIVE
	//a "soft MSM" has vertices lying on a boundary
	//edge of a soft lit polygon partitioned to the
	//back of the vertex list. the first soft index
	//is appended to the back of the MSM, making it
	//a soft MSM. MapComp is extended to blend soft
	//MSM

	int err = 0;
	enum{ e1=1,e2,e3,e4,e5,e6,e7,e8,e9 };
	#define goto_close(e) { err = e##e; assert(!e); goto close; }

	msm::image_t in = {};

	std::vector<char> buf;

	HANDLE f = CreateFileW(path,GENERIC_READ|GENERIC_WRITE,0,0,OPEN_EXISTING,0,0);	
	if(f==INVALID_HANDLE_VALUE) 
	return false;
	size_t fs = GetFileSize(f,0);
	if(!fs) goto_close(1);

	bool unique = false;
	buf.resize(fs*2/4*4,0);	
	DWORD rw;
	ReadFile(f,&buf[0],fs,&rw,0);
	if(rw!=fs) goto_close(2);
	top:
	char *eof = &buf[fs];
	msm::maptoram(in,&buf[0],fs);
	if(in.bad) goto_close(3);
	
	typedef msm::softvertex_t index_t;

	enum{ index_msb=0x8000, index_max=0xFFFE };

	const index_t *sv = msm::softvertex(in);
	
	auto *t = msm::terminator(in); //2022

	//HACK: handle cases where softvertex had to walk the
	//polygon tree and found extra (probably garbage from
	//errors unless MSM has ben extended)
	if(sv&&(char*)sv<eof) 
	{
		eof = (char*)sv;
	}
	else if(t)
	{
		eof = (char*)(&t->subdivs+1); assert(!sv);
	}	
	if(!t) //error?
	{
		std::wcout << "SoftMSM failure: file looks bad" << std::endl;

		assert(t); goto close;
	}

	auto debug = eof-&buf[fs];

	if(sv)
	{
		#ifdef _CONSOLE
		#ifndef X2MDL_INCLUDED 
		static const char *opts = "Y/N"; 
		static int disp = 'N'; if(disp>0)
		{
			int y = yes_no("This MSM feels \"soft\". Skip?",disp,opts);
			opts = "Y/N/Don't ask again";
			disp = y=='D'?-disp:y;
		}
		if(disp!='N'&&disp!=-'N') goto close;
		#endif
		#endif
	}

	msm::vertices_t &v = msm::vertices(in);
	int vc = v.count;
	msm::polygon_t *fp = msm::firstpolygon(in);

	//msm::polygon_t *p,*d = (msm::polygon_t*)(eof-sizeof(*p));
	msm::polygon_t *p,*d = const_cast<msm::polygon_t*>(t);

	/**** unexpected: duplicate vertex removal */

	//Reminder: this ALSO helps to remove duplicates that
	//were in the past generated by this process prior to
	//applying the process anew

	if(1&&vc>1&&!unique) //YUCK: remove x2msm duplicates?
	{
		unique = true;

//		//it might simplify x2mdl to lean on SoftMSM.cpp?
//		#if !defined(X2MDL_INCLUDED) || defined(_DEBUG)

		//IN A WAY THIS MORE COMPLICATED THAN THE EDGE DETECTION
		//SOFT-MSM WAS DESIGNED TO ACCOMPLISH

		SoftMSM_unique *u = (SoftMSM_unique*)(&buf.back()+1)-vc;

		for(int i=0;i<vc;i++)
		{
			auto &ui = u[i];
			auto &vi = v[i];

			ui.v = &vi; ui.reduce = i;

			//2022: the existing sorting algorithm
			//was inadequate. this is the strategy
			//Mapcomp_SoftMSM uses
			//NOTE: it was using memcmp. maybe the
			//old x2msm rounded its values somehow
			//it seemed to be fine
			for(int i=ui.sum=0;i<3;i++)
			ui.sum+=(int)(2048*vi.pos[i]);
		}
		std::sort(u,u+vc); //NOTE: using memcmp (2022)

		int reduced = 1;
		u[0].remove = -1;
		for(int i=1;i<vc;i++) 
		{
			//NOTE: 3 is to account for rounding
			SoftMSM_unique *lb = u+i;
			while(lb>u&&u[i].sum-lb[-1].sum<=3)
			lb--;

			for(;lb<u+i;lb++)
			{
				if(*lb==u[i]) break; //compare
			}

			if(lb<u+i)
			{
				if(-1==lb->remove)
				{
					u[i].remove = lb->reduce;
				}
				else u[i].remove = lb->remove;
			}
			else
			{
				u[i].remove = -1; reduced++;
			}
		}

//		#ifdef X2MDL_INCLUDED 
//		assert(reduced==vc); //x2mdl shouldn't rely on this
//		#endif
		if(reduced!=vc) 
		{
			std::sort(u,u+vc,SoftMSM_unique::reduce_pred);

			for(int i=0,j=0;i<vc;i++)
			{
				if(-1==u[i].remove)
				{
					v[j] = *u[i].v; u[i].remove = j++;
				}
				else u[i].reduce = -1; //...
			}
			for(int i=0;i<vc;i++) if(-1==u[i].reduce)
			{				
				u[i].remove = u[u[i].remove].remove;
				assert(-1!=u[i].remove);
			}
	
			for(p=fp;p<d;p=msm::shiftpolygon(p))
			{	
				for(int i=p->corners;i-->0;)
				{
					p->indices[i] = u[p->indices[i]].remove;
				}
			}
			
			char *src = (char*)&v[vc];
			memmove(&v[reduced],src,fs-(src-&buf[0]));
			v.count = reduced;
			fs-=(vc-reduced)*sizeof(msm::vertex_t);
			
			memset(&buf[fs],0x00,buf.size()-fs);

			msm::unmap(in); //unnecessary
			goto top;
		}
		else memset(u,0x00,vc*sizeof(SoftMSM_unique));

//		#endif //X2MDL_INCLUDED
	}	
	  	
	//SOM's models aren't normalized??? that makes it hard
	//to compare angles
	//NOTE: I think x2mdl should not renormalize to get an
	//accurate interplation, but I'm not 100% certain here
	//searching for this topic turns up smooth normal help
	for(int i=0;i<vc;i++) 
	{
		float n = 0, *l = v[i].lit;
		for(int j=0;j<3;j++) n+=l[j]*l[j];
		n = n?1/sqrtf(n):1;
		for(int j=0;j<3;j++) l[j]*=n;
	}

	/**** resuming soft-msm algorithm ****/

	int hard = mode==1?0:vc;

	struct vert{ index_t hard,soft; };

	struct edge
	{
		//NOTE: sum is v1+v2. It's stored as a
		//sum so it can be partitioned in such
		//a way to have to compare fewer edges

		index_t sum,v2; bool soft;

		bool operator<(const edge &cmp)
		{
			return sum<cmp.sum;
		}
	};	
	
	vert *hardsoft = (vert*)(&buf.back()+1)-vc;

	edge *ee,*e=ee=(edge*)hardsoft;

	for(p=fp;p<d;p=msm::shiftpolygon(p))
	{	
		int v2 = p->indices[0];
		if(v2>=vc) goto_close(4);

		float *lit = v[v2].lit;

		int i,iN; bool soft = mode==1;
		for(i=0,iN=p->corners;i<iN;i++)
		{
			if((char*)--e<eof) //just in case
			{
				//assert(0); //aborts console
				std::wcout << "SoftMSM restarting: edge buffer too small" << std::endl;
				restart: 
				msm::unmap(in); //unnecessary
				size_t sz = buf.size()*3/2/4*4; 
				buf.resize(fs);
				buf.resize(sz);
				goto top;
			}
			
			e->v2 = (index_t)v2;
			
			int pi = p->indices[(i+1)%iN];
			
			if(pi>=vc) goto_close(5);
			
			float *cmp = v[pi].lit;

			if(lit!=cmp&&!soft&&!mode)
			{
				//if(memcmp(lit,v[pi].lit,3*sizeof(float)))
				if(SoftMSM_unique::compare(lit,cmp,3))
				{
					soft = true; hard = 0;
				}
			}

			e->sum = (index_t)pi; v2 = pi;
		}
		for(i=0;i<iN;i++)
		{
			if(soft) e[i].soft = true;

			vert &hs = hardsoft[e[i].v2];
			vert &sh = hardsoft[e[i].sum];
			(soft?hs.soft:hs.hard)++;
			(soft?sh.soft:sh.hard)++;

			e[i].sum+=e[i].v2;
		}

		if(soft) //HACK: marking to resolve an ambiguity
		{
			p->texture|=index_msb; //repurposing texture
		}
		else p->texture&=~index_msb; //in case restarted
	}

	if(0!=p->subdivs) //0-terminator
	{ 
		goto_close(6);
	}
	else d = p;

	if(!hard) //soft?
	{
		std::sort(e,ee);

		int eN = int(ee-e); 				
		for(int i=0;++i<=eN;)
		{
			int ii = i-1;
			index_t cmp = e[ii].sum;
			e[ii].sum-=e[ii].v2;
			while(i<eN&&cmp==e[i].sum)
			{
				e[i].sum-=e[i].v2;
				i++;				
			}

			//ALGORITHM
			//if edges x,y and y,x exist then the
			//edge has two polygons, in which case
			//it's not a candidate for MPX blending
			for(int j,jj=ii;jj<i;jj++)
			{
				bool x = false;
				index_t v1 = e[jj].sum;
				index_t v2 = e[jj].v2;
				if(!v1&&!v2) continue;
				for(j=ii;j<i;j++)
				if(e[j].sum==v2&&e[j].v2==v1)
				{
					e[j].sum = e[j].v2 = 0;

					x = true; if(e[j].soft)
					{
						//e[j].soft = 0;

						hardsoft[v1].soft--;
						hardsoft[v2].soft--;
					}
				}
				if(x) for(j=ii;j<i;j++)
				if(e[j].sum==v1&&e[j].v2==v2)
				{
					e[j].sum = e[j].v2 = 0;

					if(e[j].soft)
					{
						//e[j].soft = 0;

						hardsoft[v1].soft--;
						hardsoft[v2].soft--;
					}
				}
			}
		}

		int soft = 0; assert(!hard);

		//if both the vertex must be duplicated
		//to preserve the hard faces
		for(int i=0;i<vc;i++)
		{
			vert &hs = hardsoft[i];
			assert(~hs.soft&index_msb);
			assert(~hs.hard&index_msb);

			if(hs.soft)
			{
				hs.soft = soft++;
			}
			else
			{
				hs.soft = index_max+1;

				if(!hs.hard) hs.hard = 1; //internal
			}

			hs.hard = hs.hard?hard++:index_max+1;
		}
		if(!soft) goto hard;
		for(int i=0;i<vc;i++) //soft come after hard
		{
			if(hardsoft[i].soft<index_max)
			{
				hardsoft[i].soft+=hard;
			}
		}		
		
		sv = (msm::softvertex_t*)(&d->subdivs+1);

		int shift = (hard+soft-v.count)*sizeof(msm::vertex_t);

		//move edge vertices to back
		msm::vertex_t *swap = (msm::vertex_t*)(sv+1);
		(char*&)swap+=shift;
		if((char*)hardsoft-(char*)swap<soft*sizeof(*swap))
		{
			//assert(0); //aborts console
			std::wcout << "SoftMSM restarting: swap buffer too small" << std::endl;
			
			goto restart;
		}
		swap-=hard;

	  /** "goto restart" forbidden beyond this point **/

		for(p=fp;p<d;p=msm::shiftpolygon(p))
		{
			int softpoly = p->texture&index_msb;
			if(softpoly) p->texture&=~index_msb; //unmark

			for(int i=p->corners;i-->0;)
			{
				index_t &pi = p->indices[i]; //MODIFYING
				
				vert &hs = hardsoft[pi];
				index_t pj = softpoly?hs.soft:hs.hard;
				if(pj>index_max)
				{
					pj = softpoly?hs.hard:hs.soft;
				}		
				pi = pj;
			}
		}

		if(shift) //make room for duplications
		{
			assert(shift>=0);
			if(shift<0) goto_close(8); //programmer error

			char *p = (char*)fp;			
			memmove(p+shift,p,(char*)sv-p);

			(char*&)sv+=shift;
		}		
				
		for(int i=0;i<vc;i++)
		{
			vert &hs = hardsoft[i];
			if(hs.soft<=index_max) swap[hs.soft] = v[i];
			if(hs.hard<=index_max) v[hs.hard] = v[i];
		}
		memcpy(&v[hard],swap+hard,sizeof(*swap)*soft);

		v.count = hard+soft; fs = (char*)sv-&buf[0]+sizeof(*sv);
	}
	else hard:
	{	
		if(!sv)
		{
			sv = (msm::softvertex_t*)eof; fs+=sizeof(*sv);
		}
		else assert(*sv==vc); assert(hard==vc);
	}
	*const_cast<msm::softvertex_t*>(sv) = (msm::softvertex_t)hard;
		
	SetFilePointer(f,0,0,FILE_BEGIN);
	WriteFile(f,&buf[0],fs,&rw,0);	
	SetEndOfFile(f);

	close: //skip? goto_close?

	msm::unmap(in);
	CloseHandle(f); return err;
}

#ifdef _CONSOLE
#ifndef X2MDL_INCLUDED 
static int wmain(int argc, const wchar_t* argv[])
{	
	const wchar_t *def[2] = {argv[0],L"*.msm"};

	int mode = 0; if(argc==1)
	{
		argc = 2; argv = def; 

		std::cout <<
		"Usage: SoftMSM [new mode] [\"glob\"] ...\n"
		"This program modifies MSM files to make them blend in your games.\n"
		"Input can be multiple \"glob\" patterns that are file names with\n"
		"wildcards; or a mode number to use on subsequent input files:\n"
		"0: Default mode; edge vertices must belong to \"soft\" polygons.\n"
		"1: Soft mode; edge vertices are unfiltered, as-if polygons are soft.\n"
		"2: Hard mode; edge vertices aren't blended, as-if polygons are hard.";

		switch(int y=yes_no("Convert MSM files in this directory?",'N',"Y/N/0/1/2"))
		{
		default: return 0;
		case '1': case '2': mode = y-'0';
		case '0': case 'Y': break;
		}
	}

	for(int j=0,i=1;i<argc;i++)
	{
		int f = *argv[i];
		if(f>='0'&&f<='2'&&!argv[i][1])
		{
			mode = f-'0'; continue;
		}

		WIN32_FIND_DATAW found;
		HANDLE glob = FindFirstFileW(argv[i],&found);

		if(glob==INVALID_HANDLE_VALUE) continue;

		wchar_t *cat = PathFindFileNameW(argv[i]);
		size_t len = cat-argv[i];

		wchar_t path[MAX_PATH];
		wmemcpy(path,argv[i],len); cat = path+len;

		do //FindNextFile
		{
			wcscpy(cat,found.cFileName);
			
			std::wcout << "Softening " << path << std::endl;

			int err = SoftMSM(path,mode);

			std::wcout << j++;			
			std::wcout << (err?": ERROR #":": Success");
			if(err)
			std::wcout << err;			
			std::wcout << std::endl;			

		}while(FindNextFileW(glob,&found));

		FindClose(glob);
	}

	return 0;
}
#endif X2MDL_INCLUDED 
#endif