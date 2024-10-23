	
#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

#include <vector>
#include <algorithm>

#include "SomEx.ini.h"

#include "som.state.h"
#include "som.tool.hpp"

#include "../lib/swordofmoonlight.h"
#include "../x2mdl/x2mdl.h"

//2024: MapComp_40c40a_etc_xyz
//MapComp_40c40a_obj_z(char *a) assume
//stack address of a
#pragma optimize("",off)

extern DWORD(&MapComp_memory)[6];

static struct Mapcomp_SoftMSM //SoftMSM
{
	enum{ m=1024, m2=m*2 };

	typedef struct MapComp_43e638::msm::vert vert;

	struct soft_double
	{
		int ir; vert *v; 
		bool operator<(const soft_double &cmp)const
		{
			return ir<cmp.ir;
		}

		void norm(float (&o)[3])
		{
			float (&n)[3] = v->norm; o[1] = n[1];
			switch(ir&3) //MapComp_LYT
			{
			case 0: o[0] = +n[0]; o[2] = +n[2]; break;
			case 1: o[0] = -n[2]; o[2] = +n[0]; break;
			case 2: o[0] = -n[0]; o[2] = -n[2]; break;
			case 3: o[0] = +n[2]; o[2] = -n[0]; break;
			}			
		}
	};	
	struct soft_record : soft_double
	{
		int sum,x,y,z;

		bool operator<(const soft_record &cmp)const
		{
			return sum<cmp.sum;
		}
	};
	
	std::vector<soft_record> srec;

	struct soft_result{ FLOAT *dst,src[3]; };	
	
	//NOTE: refactored for layers

	void operator()(int layer=0) //SoftMSM
	{
		layer*=100*100; 

		//4: implicates SoftMSM, 5: light		
		if(!MapComp_memory[4]||~MapComp_memory[5]&1) 
		{
			//TODO: add MAP (SOM_MAP) option to disable 
			return;
		}
		else assert(!EX::INI::Editor()->do_subdivide_polygons);

		srec.reserve(4096); //a bunch
		
		std::vector<soft_result> sres;
		std::vector<soft_double> sdub; //could be fixed

		//if there are not layers this is a
		//two pass algorithm. layers fill up
		//the lookup table with just one pass 
		int passN = -1!=MapComp_memory[2]?2:1;
		int pass = layer?passN:1;
		for(;pass<=passN;pass++)
		{
			//NOTE: this is for layers, since the
			//MSM instances memory is unfilled out
			int x = 0, z = 0;

			for(int i=0;i<100*100;i++,x+=m2)
			{	
				if(x==100*m2){ x = 0; z+=m2; }

				struct MapComp_43e638 &tile = MapComp_43e638[i];		

				int part_number = tile.prt;
				if(part_number==0xFFFF) continue;

				assert(part_number==tile.mhm||!tile.msm);

				typedef char part_record[228];
				part_record &prt = *((part_record**)0x486FF0)[part_number];
						
				//MapComp_LYT needs to use msm because it
				//doesn't fill out the memory where dst_msm is
				typedef struct MapComp_43e638::msm mesh_record;
				mesh_record &msm = *((mesh_record**)0x489000)[part_number];
				mesh_record &dst_msm = *tile.msm;
				assert(!tile.msm||msm.vertsN==dst_msm.vertsN);

				//SOM_MAP_map::read/Mapcomp_SoftMSM stash this in 
				//the part description
				DWORD softvertex = (DWORD&)prt[131+1]; 

				DWORD lastvertex = msm.vertsN;

				if(softvertex>=lastvertex) 
				{
					assert(softvertex==lastvertex);
					continue;
				}

				float y = m*tile.elevation;

				vert *it,*iit,*itt = msm.verts;
				int r = tile.rotation, ir = i+layer<<2|r;
				for(iit=it=itt+softvertex,itt+=lastvertex;it<itt;it++)
				{
					soft_record sr;
					sr.x = m*it->wpos[0];
					sr.y = m*it->wpos[1]+y;
					sr.z = m*it->wpos[2];
					switch(r) //MapComp_LYT
					{
					case 1: std::swap(sr.x,sr.z); sr.x = -sr.x; break; 
					case 2: sr.x = -sr.x; sr.z = -sr.z; break;
					case 3: std::swap(sr.x,sr.z); sr.z = -sr.z; break; 
					}
					sr.x+=x; sr.z+=z;
					sr.sum = sr.x+sr.y+sr.z;
					sr.ir = ir; 				
					sr.v = it;								

					if(pass==1)
					{
						srec.push_back(sr); continue;
					}

					std::vector<soft_record>::iterator b,e,lb,ub;
					lb = ub = std::lower_bound(b=srec.begin(),e=srec.end(),sr);				
					while(lb>b&&sr.sum-lb[-1].sum<=3) lb--;
					if(ub!=e)
					do ub++;while(ub<e&&ub->sum-sr.sum<=3);
				
					//float *n = it->norm;
					float *n = dst_msm.verts[it-msm.verts].norm;

					float lbn[3],srn[3] = {n[0],n[1],n[2]};
					soft_result res = {n}; for(;lb<ub;lb++) 
					{
						//NOTE: currently SoftMSM doesn't screen out edges
						//that look like boundary edges because of UV maps
						//this excludes them, but is needed in either case
						if(ir==lb->ir) 
						continue;

						int dx = sr.x-lb->x; 
						int dy = sr.y-lb->y; 
						int dz = sr.z-lb->z; 
						if(dx<-1||dx>1||dy<-1||dy>1||dz<-1||dz>1)
						continue;					
											
						sdub.push_back(*lb);				
					}
					if(sdub.empty()) continue;
				
					//COMPLICATED: sdub tries to deal with artificial 
					//cases where a vertex is loaded with multiple UV
					//coordinates
					std::sort(sdub.begin(),sdub.end());
					for(size_t i=0;i<sdub.size();)
					{
						size_t ii = i; do
						{
							i++;
						}while(i<sdub.size()&&sdub[ii].ir==sdub[i].ir);
						
						float w = 1; if(i-ii>1) w/=i-ii;
						
						for(;ii<i;ii++)
						{
							const float (&n2)[3] = sdub[ii].v->norm;														
							lbn[1] = n2[1];
							switch(sdub[ii].ir&3) //MapComp_LYT
							{
							case 0: lbn[0] = +n2[0]; lbn[2] = +n2[2]; break;
							case 1: lbn[0] = -n2[2]; lbn[2] = +n2[0]; break;
							case 2: lbn[0] = -n2[0]; lbn[2] = -n2[2]; break;
							case 3: lbn[0] = +n2[2]; lbn[2] = -n2[0]; break;
							}	

							//somehow two-sided faces must be ruled out
							float a = 0; for(int j=0;j<3;j++) a+=srn[j]*lbn[j];
							a = acosf(a);  
						
							if(a>M_PI/2.727272727272) continue; //66deg
							//if(a>M_PI/2.5) continue; //72deg					
							//if(a>M_PI_2) continue; //90deg
						
							for(int j=0;j<3;j++)
							{
								res.src[j]+=w*lbn[j];
							}
						}
					}	
					for(int i=0;i<3;i++) res.src[i]+=srn[i];

					float dp = 0; //normalize
					for(int i=0;i<3;i++) dp+=res.src[i]*res.src[i];
					dp = 1/sqrt(dp); //zero divide
					for(int i=0;i<3;i++) res.src[i]*=dp;

					sdub.clear();

					sres.push_back(res);
				}
			}

			if(passN==2) 
			if(pass==1) std::sort(srec.begin(),srec.end());
		}
		for(size_t i=0;i<sres.size();i++)
		{
			memcpy(sres[i].dst,sres[i].src,sizeof(sres[i].src));
		}	
	}

}Mapcomp_SoftMSM;

extern bool MapComp_LYT(WCHAR map[2]) //som.tool.cpp
{
	MapComp_memory[1] = -1; //assume basic	

	assert(!SOM_MAP.lyt);
	extern void som_LYT_open(WCHAR mapcomp[2]);
	som_LYT_open(map);
	int i = _wtoi(map);
	if(i<0||i>63||!SOM_MAP.lyt[i])
	return true;			 

	bool out = true;
	char a[MAX_PATH] = SOMEX_(B)"\\data\\map\\";
	int cat = strlen(a);
	int lyr = 1;
	for(som_LYT **lyt=SOM_MAP.lyt[i];*lyt;lyt++)
	{
		MapComp_memory[2] = -1; //MapComp_LYT

		som_LYT &ln = **lyt; 
		sprintf(a+cat,*ln.w2?"%ls":"%s.map",*ln.w2?(void*)ln.w2:ln.map);
		const wchar_t *w = SOM::Tool.project(a);
		extern bool som_tool_file_appears_to_be_missing(const char*,const wchar_t*,wchar_t*);
		if(som_tool_file_appears_to_be_missing(a,w,0))
		return false;		
		HANDLE h = CreateFile(w,SOM_TOOL_READ);
		if(h==INVALID_HANDLE_VALUE)
		continue;
	
		//mark base-map/existence of layer
		//REMINDER: enables to Mapcomp_SoftMSM/Mapcomp_MHM to load vbuffer 
		MapComp_memory[1] = i; 

		//SoftMSM/Mapcomp_SoftMSM
		for(int j=100*100;j-->0;) MapComp_43e638[j].prt = 0xFFFF; 

		//mimicking som_tool_CreateFileA
		SOM_MAP.map.current = ln.legacy(); //YUCK
		SOM_MAP.map = h; 
		while(!SOM_MAP.map.finished)
		{
			//read loads the profiles into MapComp (486FF0) via 408EA0
			char buf[2048]; DWORD rd = sizeof(buf);			
			if(!SOM_MAP.map.read(buf,rd,&rd)) 
			{
				out = false; break;
			}
		}
		CloseHandle(h); assert(!SOM_MAP.map);

		Mapcomp_SoftMSM(lyr++); //SoftMSM/Mapcomp_SoftMSM
	}	
	MapComp_memory[2] = 0; //SoftMSM/Mapcomp_SoftMSM

	if(!out) MapComp_memory[1] = -1; return out;
}

inline BYTE Mapcomp_MT_vclamp(int i)
{
	return (BYTE)max(0,min(0xFF,i));
}
static void Mapcomp_MT_vcolor(BYTE *vc, int m, float *d)
{		
	if(m&1) //darkened?
	for(int i=0;i<3;i++)
	vc[i] = Mapcomp_MT_vclamp(d[i]*vc[i]+0.5f);
	if(m&2) //transparent?
	vc[3] = Mapcomp_MT_vclamp(d[3]*255+0.5f);
	if(m&4) //glowing?
	for(int i=0;i<3;i++)
	vc[i] = Mapcomp_MT_vclamp(vc[i]+d[i+4]*255+0.5f);
}
static void Mapcomp_MT() //mdo texture?
{	
	SOM::MT::lookup("");
	auto &mt = *SOM::material_textures;
	if(!&mt) return;	

	typedef MapComp_container<char*> txrC;
	txrC &txr = *(txrC*)0x43E2CC;
	for(int i=txr.size();i-->0;)
	mt[i] = SOM::MT::lookup(txr[i]);

	typedef struct MapComp_43e638::mpx mpx;
	for(int i=0;i<100*100;i++) if(mpx*ptr=MapComp_43e638[i].mpx)
	{
		//00408419 E8 52 A3 FF FF       call        00402770
		mpx::per_texture *p = ptr->pointer;
		mpx::per_texture *d = p+ptr->textures;
		for(;p<d;p++) if(p->texture<1024) //untextured?
		{	
			SOM::MT *mdo = mt[p->texture];
			if(!mdo) continue;

			int mm = mdo->mode; if(mm&7)
			{
				float *md = mdo->data; if(mdo->data[3])
				{
					mpx::vertex *itt,*it;
					itt = p->vertices.end;
					it = p->vertices.begin;
					for(;it<itt;it++)
					Mapcomp_MT_vcolor(it->bgra,mm,md);
				}
				else //fully transparent? (dummy.txr MDO?)
				{
					//NOTE: 0 is treated as opaque, so 
					//the only way is to empty
					//TODO: remove p (without a memory leak)

					//CRASHES (also crashes game... with initial picture)
					//0040E7C4 8B 44 8E F8          mov         eax,dword ptr [esi+ecx*4-8]
					//p->indices.clear(); p->vertices.clear();

					//HACK: degenerate :(
					for(int i=0;i<p->indices.size();i++)
					p->indices[i] = p->indices[0];
				}
			}
		}
	}
}

static int __cdecl MapComp_main(int argc, char *argv[], char *envp[]) 
{
	//REMINDER: THIS SUBROUTINE IS REENTRANT

	SOM::splashed(0);

	//2019: Notes to self I wish I made for my future self in 2018	
	//1) 489000 may be MSM records 
	//2) 486FF0 is PRT records. I think the initial pointer stored
	//in them may be an MHM record
	//3) I think PRT/MSM/MHM are treated as one entity even though
	//there are two containers for tracking the association. these
	//containers just count 1,2,3,etc. they are used to locate the
	//MSM/MHM records. MHM models can overlap a lot, but an MSM is
	//unlikely to see reuse	
	
	//ANNOYANCE
	//4322B0/432298 are 4 DWORD containers (the first word is 0?)
	//that effectively simply count the MHM/MSM records that will
	//go into the MPX. MSM are not even required
	bool base; if(base=!*(DWORD*)0x4322B4)
	{
		//TODO: MapComp doesn't do it but these can be manipulated
		//to emit fewer MHM files where /PRTs (486FF0) share MHM files

		//SOM_MAP_map::read allocates and fills these
		auto _malloc = (void*(__cdecl*)(DWORD))0x40b6a3;
		WORD *p = (WORD*)_malloc(sizeof(WORD)*1024); //405af7 //msm
		WORD *q = (WORD*)_malloc(sizeof(WORD)*1024); //40573d //mhm
		*(DWORD*)0x43229C = *(DWORD*)0x4322A0 = (DWORD)p;
		*(DWORD*)0x4322A4 = (DWORD)(p+1024);
		*(DWORD*)0x4322B4 = *(DWORD*)0x4322B8 = (DWORD)q;				
		*(DWORD*)0x4322BC = (DWORD)(q+1024);
	}
	
	//0040C784 E8 07 6C FF FF       call        00403390
	int ret = ((int(*)(int,char*[],char*p[]))0x403390)(argc,argv,envp);
	//this may be the main processing routine?
	//0040360E E8 5D 1E 00 00       call        00405470

	//IS THIS MEMORY ZEROED?
	//00407CBE 6A 20                push        20h  
	//00407CC0 68 D0 6F 41 00       push        416FD0
	assert(0!=*(DWORD*)0x416FD0);
	//what are these MPX fields?
	//00407CB1 68 88 72 41 00       push        417288h
//	assert(0==*(DWORD*)0x417288); //bsp something	
	//00407D1D 68 20 E5 43 00       push        43E520h
	assert(0==*(QWORD*)0x43E520);
	//00407DB3 6A 01                push        1
	//00407DB5 68 D0 8F 47 00       push        478FD0h 
	//assert(0==(BYTE*)0x478FD0);
	//00407DC0 6A 03                push        3  
	//00407DC2 6A 01                push        1  
	//00407DC4 68 D1 8F 47 00       push        478FD1h 
	assert(0==*(DWORD*)0x478FD0);

	//NOTE: the subroutines return error codes
	//that are forwarded, i.e. loading MSM/MMH
	//files or PRT files  have error codes too
	switch(ret)
	{
	default: assert(0); //gathering failure codes

	case 0x24: //PR2/MAP file object format mismatch

	case 0x14: //20: no MAP file?

	case 0x15: //21: version isn't 12

		//(NOTE: PRT isn't a failure but should be)

	case 0x16: //MSM failure
	case 0x17: //MHM failure

	case 0x18: //24: 40c2e3 didn't find a delimiter

	case 0x19: //2022: new PRT failure (MapComp_408ea0)

		//WARNING: 0x19 may already be a code somewhere
		//in which case the PRT code needs to be changed
		//if found to be so

	//31: MPX doesn't open for writing
	//00407CA0 B8 1F 00 00 00       mov         eax,1Fh
	case 0x1f: 

		goto ret;

	case 0: //success

		if(-1==MapComp_memory[1]) //no layer table/reentrance
		return 0;
		break;
	}
	MapComp_memory[2] = 1; //MPY mode/LYT line number
	char *swap[2] = {argv[1],argv[2]};
	char arg1[MAX_PATH],arg2[] = SOMEX_(B)"\\data\\map\\??.mpy";
	enum{ cat=sizeof(arg2)-7 };	
	argv[1] = arg1; argv[2] = arg2;
	{
		//HACK: assuming not writing more MHMs		
		HANDLE mpy = SOM::Tool.Mapcomp_MHM(0);
		SetFilePointer(mpy,0,0,FILE_BEGIN);
		SetEndOfFile(mpy);

		memcpy(arg1,arg2,cat);
		arg2[cat+0] = '0'+MapComp_memory[1]/10; //expects 2 digits
		arg2[cat+1] = '0'+MapComp_memory[1]%10; //expects 2 digits

		som_LYT **lyt = SOM_MAP.lyt[MapComp_memory[1]]; //LYT file
		MapComp_memory[1] = -1; //prevent reentry

		for(;*lyt;lyt++,MapComp_memory[2]++) //next LYT line number
		{
			//these have to be cleared after MPX write 
			//but best to never fill them for MPY
			//(0x48A10C seems to get reset???)
			DWORD *p,containers[4] = {0x415F74,0x48A10C,0x415F84,0x43E514};			 			
			//(assuming POD)
			for(int i=0;i<4;i++){ p = (DWORD*)containers[i]; p[1] = p[0]; }

			som_LYT &ln = **lyt; 
			sprintf(arg1+cat,*ln.w2?"%ls":"%s.map",*ln.w2?(void*)ln.w2:ln.map);
			
			//RECURSIVE

			if(ret=MapComp_main(argc,argv,envp))
			{
				break; //TODO: delete MPX/MPY files?				
			}
		}

		if(0==ret)
		{
			if(0) //some processing may be necessary
			{
				//SetFilePointer(mpy,0,0,FILE_BEGIN);
			}
			if(!CopyFile(SOM::Tool.Mapcomp_MHM_temporary(),SOM::Tool::project(arg2),0))
			{
				assert(0); //breakpoint
				ret = 20; //most appropriate error code?
			}
		}
		CloseHandle(mpy);
	}
	argv[1] = swap[0]; argv[2] = swap[1]; 
	
	if(base) //ANNOYANCE
	{
		*(DWORD*)0x43229C = *(DWORD*)0x4322A0 =
		*(DWORD*)0x4322B4 = *(DWORD*)0x4322B8 = 
		*(DWORD*)0x4322BC = *(DWORD*)0x4322A4 = 0;
	}

	ret: return ret;
}

extern const wchar_t *x2mdl_dll; //HACK
static DWORD __cdecl MapComp_405470(char *a)
{
	assert(!MapComp_memory[3]||SOM_MAP.lyt);
	DWORD ret = ((DWORD(__cdecl*)(char*))0x405470)(a);
	if(ret) return ret;
	int cp[] = //copy everything layer related 
	{
		//00407CB1 68 88 72 41 00       push        417288h
		0x417288, //light/bsp flags?
		//00407CF0 68 D8 E2 43 00       push        43E2D8h
		//00407CFF 68 DC E2 43 00       push        43E2DCh
		//00407D0E 68 E0 E2 43 00       push        43E2E0h
		0x43E2D8, //light cutoff?
		0x43E2DC, //near plane? fog?
		0x43E2E0, //draw distance?

		//2 bytes... one is ambient? assuming rest is padding/related
		//00407D27 8A 0D 09 70 41 00    mov         cl,byte ptr ds:[417009h]
		0x417008,		
		//ambient color, and?
		//00407D2D A1 04 70 41 00       mov         eax,dword ptr ds:[00417004h] 
		0x417004,
	};
	DWORD &mem = MapComp_memory[3];	
	enum{ direction0=0x415F28 }; //0x415F18 //not sure?!		
	typedef MapComp_container<DWORD> lampsC;
	typedef MapComp_container<WORD[34]> objectC;		
	lampsC &lamps = *(lampsC*)0x417074;
	objectC &objects = *(objectC*)0x415F74;
	if(1) if(!mem) //save base's parameters?
	{	
		int *p = new int[(sizeof(cp)+24*3)/4+1+lamps.size()*11];
		mem = (DWORD)p;

		for(int i=0;i<EX_ARRAYSIZEOF(cp);i++)
		*p++ = *(int*)cp[i];

		//directional lights?
		memcpy(p,(void*)direction0,24*3);
		p+=6*3;

		//lamp lights
		*p++=lamps.size();
		//object loop
		//00406B15 0F 8C 44 F6 FF FF    jl          0040615F
		//first char
		//00406181 E8 8F 62 00 00       call        0040C415
		for(DWORD*it=lamps.begin;it<lamps.end;it++)
		{
			//light code is extensive... running from 4063ce to 406717
			//parsing ends at 40649F
			//417074 looks like lights container

			//DISREGARDS MAP's OBJECT-TYPE CODES
			//0x00478FF0 holds OBJ.PRM
			//00406398 66 8B 04 CD 14 90 47 00 mov         ax,word ptr [ecx*8+479014h]
			//0x00417290 hodls OBJ.PR2
			//004063AB 66 8B 04 95 E2 72 41 00 mov         ax,word ptr [edx*4+4172E2h]			
			//subtracts 10 (light code)
			//004063B3 83 C0 F6             add         eax,0FFFFFFF6h
			//compares to 1f+0xa (0x29... highest code: receptacle)
			//004063B6 83 F8 1F             cmp         eax,1Fh 
			//error? or inert object?
			//004063B9 0F 87 43 04 00 00    ja          00406802 
			//jump table (lights are next instruction)
			//004063C1 8A 88 58 7C 40 00    mov         cl,byte ptr [eax+407C58h]  
			//004063C7 FF 24 8D 38 7C 40 00 jmp         dword ptr [ecx*4+407C38h] 
			memcpy((p+=11)-11,&objects[*it],44); 
		}

		//EXPERIMENTAL
		//Follow up on MapComp_reprogram's removed instances
		//OBJECTS		
		//NOTE: can be non-512 if PR2 file changes... but if
		//so ret should be nonzero (it's code 0x24)
		assert(512==objects.size());
		int objsz = objects.size(); //2020
		typedef WORD obj_prm[1024][28];
		obj_prm *op = (obj_prm*)0x478FF0;
		for(int i=0;i<objsz;i++)
		{
			WORD &prm = objects[i][0];
			if(0xFFFF!=prm)
			if(0xFFFF==(*op)[prm][18])
			prm = 0xFFFF;
		}
		//SAME FOR NPCS (MAY WANT TO FIND OUT WHY SOM_DB IS
		//UNABLE TO HANDLE NPCS LIKE ENEMIES
		//TODO: MapComp doesn't read NPC.PRM
		//should probably let SOM_MAP.read filter
		//the instance tables
		typedef MapComp_container<WORD[26]> npcC;	
		npcC &NPCs = *(npcC*)0x415F84;
		assert(128==NPCs.size());		
		typedef WORD npc_prm[1024][160];
		if(FILE*f=_wfopen(SOM::Tool.project(SOMEX_(B)"\\param\\npc.prm"),L"rb"))
		{
			union //C++???
			{
				npc_prm *np; char *cpp;
			};
			//npc_prm *np = new npc_prm;
			cpp = new char[sizeof(*np)];
			if(fread(np,sizeof(*np),1,f))
			{			
				for(int i=0;i<128;i++)		
				{
					WORD &prm = NPCs[i][16];
					if(0xFFFF!=prm)
					if(0xFFFF==(*np)[prm][0])
					{
						memset(&NPCs[i],0x00,0x34);
						prm = 0xFFFF;			
					}
				}
			}
			fclose(f);
		}
	}
	else //copy base's parameters over layer's
	{
		int *p = (int*)mem;
		for(int i=0;i<EX_ARRAYSIZEOF(cp);i++)
		{
			*(int*)cp[i] = *p++;
		}

		//directional lights?
		memcpy((void*)direction0,p,24*3);
		p+=6*3;

		//lamp lights
		int lampsN = *p++;
		lamps.end = lamps.begin+lampsN;
		objects.end = objects.begin+lampsN;
		for(DWORD*it=lamps.begin;it<lamps.end;it++)
		memcpy(&objects[*it=it-lamps.begin],(p+=11)-11,44);
	}

		//SoftMSM: opportune time to do this
		Mapcomp_SoftMSM(MapComp_memory[2]); 

	return 0;
}

static char *__cdecl MapComp_40c415(char *buf, int sz, FILE *f) //fgets
{
	static bool alt = false; alt = !alt; //HACK
	char *p = ((char*(__cdecl*)(char*,int,FILE*))0x40c415)(buf,sz,f);
	if(!p) return 0;

	//I'm VERBATIM copying this from SOM_MAP_map::read where it was
	//originally developed

	//overriding light/mask mode?
	assert(p[1]==/*'\r'*/'\n');
	assert(*p=='0'||*p=='1');
	//wchar_t ch = 1136==line?'m':'l';
	wchar_t ch = alt?'m':'l';
	const wchar_t *mapcomp_165 =
	Sompaste->get(L".mapcomp.165");						
	if(wcschr(mapcomp_165,ch))
	{	
		if(*p=='0') (char&)*p = '1';
	}
	else if(wcschr(mapcomp_165,toupper(ch)))
	{
		if(*p=='1') (char&)*p = '0';
	}						 						
	/*can't hurt, but MapComp_405470 is
	//working now without it
	else if(current==MapComp_memory[1]) //base map?
	{
		//HACK: injecting into layers
		//MapComp_405470 is having trouble copying the
		//flags at 0x417288 
		wchar_t ex2[8];
		swprintf(ex2,L"%s%c",mapcomp_165,*p=='1'?ch:toupper(ch));
		Sompaste->set(L".mapcomp.165",ex2);	
	}*/

	//NEW: Mapcomp_SoftMSM would like to know about lighting 
	//before the base map is read
	int soft = ch=='m'?2:1; 
	if(!*mapcomp_165) //command-line?
	{
		//NOTE LIGHTING IS ENABLED
		//HACK: running MapComp from command-line it's not
		//possible to know this without parsing a base map
		if(-1==MapComp_memory[2])
		{
			MapComp_memory[5] = 1; //force Mapcomp_SoftMSM job
		}
		else if(0==MapComp_memory[2])
		{
			MapComp_memory[5]&=~soft; goto mapcomp_165;
		}
		else //inherit layer settings
		{
			(char&)*p = MapComp_memory[5]&soft?'1':'0';
		}
	}
	else mapcomp_165:
	{
		if(*p=='1') MapComp_memory[5]|=soft;
	}

	return p;
}

static DWORD __cdecl MapComp_407c80(char *a) //MPX
{
	/*2022: adding clip? (might better go in reverse?)
	//REMOVE ME (clip?)
	//MapComp_40c40a_etc_xyz should make this obsolete
	//#ifdef _DEBUG*/
	{
		//need to keep layers out of the MPX file
		//(assuming unwanted for time being)
		//  415F74 (512 objects, 0x44B each)
		//  48A10C (128 enemies? 0x34 each)
		//  415F84 (128 NPCs? 0x34 each)
		//  43E514 (256 items, 0x28 each)	
		typedef BYTE(*obj)[0x44];
		typedef BYTE(*npc)[0x34],(*itm)[0x28];
		int clip = -1;
		obj *op = (obj*)0x415F74;
		for(auto p=*op;p<op[1];p++)
		{
			//**p = 0;

			if(0xFFFF!=(WORD&)((*p)[0])) clip = p-*op; //2022
		}
		op[1] = op[0]+clip+1;
		clip = -1;
		npc *ep = (npc*)0x48A10C;
		for(auto p=*ep;p<ep[1];p++)
		{
			assert(!**p); //**p = 0; //zindex?

			if(0xFFFF!=(WORD&)((*p)[32])) clip = p-*ep; //2022
		}		
		ep[1] = ep[0]+clip+1;
		clip = -1;
		npc *np = (npc*)0x415F84;
		for(auto p=*np;p<np[1];p++)
		{
			assert(!**p); //**p = 0; //zindex?

			if(0xFFFF!=(WORD&)((*p)[32])) clip = p-*np; //2022
		}
		np[1] = np[0]+clip+1;
		clip = -1;
		itm *ip = (itm*)0x43E514;
		for(auto p=*ip;p<ip[1];p++)
		{
			assert(!**p); //**p = 0; //zindex?

			if(0xFFFF!=(WORD&)((*p)[28])) clip = p-*ip; //2022
		}
		ip[1] = ip[0]+clip+1;

		//2022: the textures section is unaligned, and
		//this throws off the rest of the MPX file making
		//it impractical to map the file to memory
		clip = 0;
		auto tp = (char***)0x43E2CC;	
		for(auto p=*tp;p<tp[1];p++)
		{
			clip+=strlen(*p)+1;
		}	
		if(int x=clip%4) //pad with "" strings?
		{
			x = 4-x;
			int cmp = tp[1]-tp[0];
			char dummy[] = "\x80"; //assuming unique!!
			while(x-->0)
			{
				if(cmp!=((WORD(__cdecl*)(const char*))0x4092f0)(dummy))
				{
					assert(0); break; //dummy collied?
				}
				else cmp++;

				char **p = tp[1]-1;

				**p = '\0'; //store as empty string
			}
		}
	}
	//#endif

	//INCOMPLETE
	//this selectively disables parts of the MPX file
	//so that an MPY is really just MPX without parts	
	DWORD containers[] = 
	{
		//NOTE: something clears 0x48A10C
		//if this doesn't restore it after
		//it will need to be reprogrammed
		//
		0x415F74,0x48A10C,0x415F84,0x43E514, //objects, etc.

		0x43E2CC, //textures

		0x415FB4, //MSM vbuffer

		//2B each
		//per MHM data doubling as 487FF0 (PRT) record counter
		//0040847E 7E 3F                jle         004084BF
		0x4322B4, 

		//43229C is an equivalent container for MSM, but
		//it seems like MPX doesn't actually require MSM
	};	

	Mapcomp_MT(); //add MDO properties to MSM?

	DWORD *p,restore[EX_ARRAYSIZEOF(containers)];
	for(int i=0;i<EX_ARRAYSIZEOF(containers);i++)
	{
		p = (DWORD*)containers[i];

		restore[i] = p[1]; 
		
		if(MapComp_memory[2]) //MPY?
		{
			p[1] = p[0]; //don't emit
		}
	}

	DWORD ret = ((DWORD(__cdecl*)(char*))0x407c80)(a); //write
	
	for(int i=0;i<EX_ARRAYSIZEOF(containers);i++)
	{
		p = (DWORD*)containers[i];

		p[1] = restore[i]; 
	}

	return ret;
}

extern wchar_t *som_art_CreateFile_w; 
extern int som_art(const wchar_t*, HWND);
extern int som_art_model(const char*, wchar_t[MAX_PATH]);
static BYTE __cdecl MapComp_40ae50(float *cps, char *a)
{
//	assert(x2mdl_dll[3]=='d'); //2022: x2mdl.dll //403820

	x2mdl_dll = L"x2mdl.dll"; //PARANOID

	wchar_t art[MAX_PATH]; 	
	int e = som_art_model(a,art); //2021
	using namespace x2mdl_h;
	if(e&_art&&~e&_lnk)	
	if(!som_art(art,0)) //x2mdl exit code?
	{
		//NOTE: MapComp_405470 is setting x2mdl_dll
		//to "x2msm.dll" after the map file is read

		e = som_art_model(a,art); //retry?
	}
	if(0==(e&(_mdo|_bp|_mdl))) return 0; //2021

	som_art_CreateFile_w = art;

	char *ext = PathFindExtensionA(a);
	char *fmt = ".mdl";
	if(e&(_mdo|_bp))
	fmt = e&_mdo?".mdo":".bp"; //BP??
	if(fmt) memcpy(ext,fmt,5);

	BYTE ret; if(e&(_mdo|_bp)) //mdo?
	{
		ret = ((BYTE(__cdecl*)(float*,char*))0x40ae50)(cps,a);
	}
	else if(void*mdl=((void*(__cdecl*)(char*))0x409C70)(a))
	{
		float *mdo_cps = (float*)0x4322c0;
		void **mdl_cps = (void**)0x415fd0;
		ret = 1; mdl_cps[(cps-mdo_cps)/12] = mdl;
	}
	else ret = 0;

	som_art_CreateFile_w = 0; return ret;
}
static WORD __cdecl MapComp_4092f0(const char *a) //2022 (x2mdl)
{
	if(!*a) return 0xffff; //remove "" padding textures #1

	return ((WORD(__cdecl*)(const char*))0x4092f0)(a);
}
static WORD *__cdecl MapComp_409570(void *_1, void *_2, WORD *_3, WORD **_4, WORD *_5)
{
	auto *msm = (struct MapComp_43e638::msm*)_1;

	for(int i=msm->texturesN;i-->0;) //remove "" textures #2
	{
		if(msm->textures[i]==0xFFFF) msm->texturesN--; else break;
	}

	return ((WORD*(__cdecl*)(void*,void*,void*,void*,void*))0x409570)(_1,_2,_3,_4,_5);
}
extern DWORD MapComp_408ea0(DWORD pn) //SOM_MAP.cpp
{
	//NOTE: I thought setting this in MapComp_405470
	//would fix this but I'm still seeing leaks that
	//output MDO instead of MSM/MHM in Sep. 2022

//	assert(x2mdl_dll[3]=='s'); //2022: x2msm.dll

	x2mdl_dll = L"x2msm.dll"; //PARANOID

	if(pn>=1024){ assert(0); return 0; }

	typedef char part_record[228];
	part_record* &prt_ptr = ((part_record**)0x486FF0)[pn];

	struct mhm_pair{ DWORD sz; void *file; };

	mhm_pair* &mhm_ptr = ((mhm_pair**)0x487ff0)[pn];

	typedef struct MapComp_43e638::msm mesh_record;
	mesh_record* &msm_ptr = ((mesh_record**)0x489000)[pn];

	if(prt_ptr){ assert(0); return 1; }

	extern UINT som_map_tileviewmask;
	bool mhm = 0x100&som_map_tileviewmask;

	char a[64], *b = a+20;

	auto _malloc = (void*(__cdecl*)(DWORD))0x40b6a3;

	///////////////// PRT ////////////////////

	//0x415248->"%s%4.4d.prt"
	//0x417080->"A:\>\data\map\parts\"
	sprintf(a,(char*)0x415248,0x417080,pn);

	//FILE *f = fopen(a,"rb");
	HANDLE h = CreateFileA(a,SOM_TOOL_READ);
	
	//if(!f) //return 0; //???
	if(h==INVALID_HANDLE_VALUE)
	{
		//doesn't work
		//(void*&)prt_ptr = _malloc(228); //TESTING
		//memset(prt_ptr,0,228);
		
		//MapComp can't work if a PRT file fails to read
		//so I guess I have to invent an error code here
		return 0x19;
	}

	void *p = _malloc(228);

	DWORD rd;
	//fread(p,228,1,f); fclose(f); 
	ReadFile(h,p,228,&rd,0); CloseHandle(h);
	
	(void*&)prt_ptr = p;
	auto *prt = *prt_ptr;
	if(mhm&&prt[32]) prt+=32;
	auto *ext = strrchr(prt,'.');
	if(!ext) ext = prt+strlen(prt);

	///////////////// MSM ////////////////////

	//0x417184->"A:\>\data\map\msm\"->model
	sprintf(a,"%s%s",SOMEX_(A)"\\data\\map\\model\\",prt);

	x2mdl_dll = L"x2msm.dll"; //PARANOID

	wchar_t art[MAX_PATH]; 	
	int e = som_art_model(a,art); //2021
	using namespace x2mdl_h;
	if(e&_art&&~e&_lnk)	
	if(!som_art(art,0)) //x2mdl exit code?
	{
		//NOTE: MapComp_405470 is setting x2mdl_dll
		//to "x2msm.dll" after the map file is read
		//2022: I saw a leak yesterday prior to the
		//new release that will add MSM/MHM to DATA

		e = som_art_model(a,art); //retry?
	}	
	auto cmp = mhm?".mhm":".msm";
	int _mxm = mhm?_mhm:_msm;
	if(0==(e&_mxm))
	if(*ext&&!stricmp(ext,cmp))
	{			
		b-=2; sprintf(a+14,"%s\\%s",cmp+1,prt);
	}

	som_art_CreateFile_w = 0==(e&_mxm)?0:art;
	{
		memcpy(b+(ext-prt),cmp,5);

		//f = fopen(a,"rb");
		h = CreateFileA(a,SOM_TOOL_READ);
	}
	som_art_CreateFile_w = 0; //if(f) //4090a0
	if(h!=INVALID_HANDLE_VALUE)
	{
		//fseek(f,0,SEEK_END);	
		//auto os = ftell(f);	
		auto os = GetFileSize(h,0);
		p = new char[os]; //p = _malloc(os);		
		//fseek(f,0,SEEK_SET);		
		//fread(p,os,1,f); //fclose(f);
		ReadFile(h,p,os,&rd,0); CloseHandle(h);
		namespace msm = SWORDOFMOONLIGHT::msm;
		msm::image_t img;
		msm::maptorom(img,p,os);
		auto &t = msm::textures(img);
		auto &v = msm::vertices(img);
		auto fp = msm::firstpolygon(img);
		if(!img.bad)
		{
			(void*&)msm_ptr = _malloc(32);

			auto *mp = msm_ptr;
			DWORD tn = t.count, vn = v.count;
			mp->texturesN = tn;
			(void*&)mp->textures = _malloc(2*tn);
			mp->vertsN = vn;
			(void*&)mp->verts = _malloc(60*vn);			 
			mp->facesN = 0; 
			mp->faces = 0; 

			const char *a = t.refs;
			for(DWORD i=0;i<tn;i++)
			{
				mp->textures[i] = MapComp_4092f0(a);
				while(*a) a++; a++;
			}

			auto *vp = v.list;
			for(DWORD i=0;i<vn;i++,vp++)
			memcpy(mp->verts+i,vp,8*sizeof(float));

			auto pp = &fp->subdivs;
			MapComp_409570(mp,&mp->faces,&mp->facesN,&pp,mp->textures);

			/*NOT WORKING OUT?
			//HACK: trying not to trigger som_MPX_411a20_ltd asserts
			//on empty models (debugging)
			if(!fp->subdivs) 
			{
				mp->texturesN = 0;

				//MEMORY LEAK
				//this seems reasonable, but I'm hitting an error in
				//som_MPX_411a20_ltd now. this might invalidate dummy
				//tiles too
				msm_ptr = 0; 
			}*/
		}
		//else f = 0;
		else h = INVALID_HANDLE_VALUE;
		delete[] p; //operator_delete(p); //040b620
	}
	//if(!f) return 23; //assert(0); should raise MessageBox now
	if(h==INVALID_HANDLE_VALUE) return 23;

	///////////////// MHM ////////////////////
	
	if(a[15]=='s') //don't use art system?
	{
		a[15] = 'h'; //msm->mhm?
	}			

	if(!mhm||prt!=*prt_ptr+32)
	if(!prt[32]||!memcmp(prt,prt+32,ext-prt+1))
	{
		if(!prt[32]) memcpy(prt+32,prt,32); 

		ext+=32;
		prt+=32;
	}
	else 
	{
		prt+=32;

		memcpy(b,prt,32);

		ext = strrchr(prt,'.');
		if(!ext) ext = prt+strlen(prt);

		if(b==a+20)
		{
			e = som_art_model(a,art); //2021
		//	using namespace x2mdl_h;
			if(e&_art&&~e&_lnk)	
			if(!som_art(art,0)) //x2mdl exit code?
			{
				e = som_art_model(a,art); //retry?
			}
			if(0==(e&_mhm))
			{
				if(*ext&&!stricmp(ext+1,"mhm"))
				{			
					b-=2; sprintf(a+14,"mhm\\%s",prt);
				}
			}
		}
	}
	
	som_art_CreateFile_w = 0==(e&_mhm)?0:art;
	{
		memcpy(b+(ext-prt),".mhm",5);

		//HACK: som_tool_CreateFileA needs a
		//way to prevent MHM->MSM conversion
		//it used to check for "map/msm" but
		//it cannot do that with "map/model"
		if(mhm) som_map_tileviewmask&=~0x100;

		//f = fopen(a,"rb");
		h = CreateFileA(a,SOM_TOOL_READ);

		if(mhm) som_map_tileviewmask|=0x100;
	}	
	som_art_CreateFile_w = 0; //if(f) //408fc0
	if(h!=INVALID_HANDLE_VALUE)
	{
		//fseek(f,0,SEEK_END);	
		//auto os = ftell(f);
		auto os = GetFileSize(h,0);
		p = _malloc(os);		
		//fseek(f,0,SEEK_SET);		
		//fread(p,os,1,f); //fclose(f);
		ReadFile(h,p,os,&rd,0); CloseHandle(h);

		(void*&)mhm_ptr = _malloc(8);
		mhm_ptr->sz = os;
		mhm_ptr->file = p;

		//EXTENSION?
		if(os<24) //f = 0; //TODO: further validation?
		h = INVALID_HANDLE_VALUE;
	}
	//if(!f) return 22; //assert(0); should raise MessageBox now
	if(h==INVALID_HANDLE_VALUE) return 22;
	
	return 0;
}

//EXTENSION
//these inject tile coordinates into 
//the data structures that the new map 
//map change event extension requires
//to be user-friendly
//
//WEIRD (Ghidra thinks __thiscall?)
//the structure of 40c40a looks like this???
//0040C40A FF 74 24 04          push        dword ptr [esp+4]  
//0040C40E E8 6C FF FF FF       call        0040C37F  
//0040C413 59                   pop         ecx 
#pragma optimize("",off)
static void __cdecl MapComp_40c40a_obj_z(char *a)
{
	//004061C3 C6 84 24 9A 00 00 00 00 mov         byte ptr [esp+9Ah],0 
	BYTE *obj = (BYTE*)&a+0x9A-6; //0x0068ea9c

	obj[5] = (BYTE)atoi(a);
}
static int __cdecl MapComp_40c40a_obj_xy(char *a)
{
	int i = atoi(a);
	
	BYTE *obj = (BYTE*)&a+0x9A-6; //0x0068ea9c 
	
	//004061E8 89 44 24 4C          mov         dword ptr [esp+4Ch],eax 
	BYTE *x = (BYTE*)&a+0x4c-8;

	obj[6] = *x;
	obj[7] = (BYTE)i; return i; //y
}
static int __cdecl MapComp_40c40a_etc_xyz(char *a)
{
	int i = atoi(a); //0x0068ea08 

	BYTE *etc = (BYTE*)&a+0x54; //0x0068ea5c

	BYTE *x = (BYTE*)&a+0x28; //0x0068ea30

	etc[1] = etc[0]; //zindex
	etc[0] = 0; //hide zindex from som_db.exe
	etc[2] = *x;
	etc[3] = (BYTE)i; return i; //y
}
//#pragma optimize("",on)

extern void MapComp_reprogram()
{
	//main() entrypoint
	//0040C76D A1 E8 A1 48 00       mov         eax,dword ptr ds:[0048A1E8h]  
	//0040C772 A3 EC A1 48 00       mov         dword ptr ds:[0048A1ECh],eax
	//0040C777 50                   push        eax  
	//0040C778 FF 35 E0 A1 48 00    push        dword ptr ds:[48A1E0h]  
	//0040C77E FF 35 DC A1 48 00    push        dword ptr ds:[48A1DCh]
	//main( int argc, char *argv[ ], char *envp[ ] ) 
	//0040C784 E8 07 6C FF FF       call        00403390
	*(DWORD*)0x40C785 = (DWORD)MapComp_main-0x40C789;
	
	//reads MAP file
	//0040360E E8 5D 1E 00 00       call        00405470
	*(DWORD*)0x40360F = (DWORD)MapComp_405470-0x403613;
								 
	if(1||EX::debug) //EXPERIMENTAL (NEEDS MORE WORK)
	{
	//TODO? Items/monsters don't seem to encounter problems	
	//
	//try to hide references to empty PRM records (and filter out afterward)
	//OBJECTS (change to movsx eax,...)
	//00406398 66 8B 04 CD 14 90 47 00 mov         ax,word ptr [ecx*8+479014h]  	
	*(WORD*)0x406398 = 0xbf0f;		
	//NPCs don't crash MapComp like objects, but do SOM_DB
	}

	//objects don't have a z-index field (it must have been
	//lazily replaced with a profile type/format identifier)
	//0040682D D8 04 D5 3C E6 43 00 fadd        dword ptr [edx*8+43E63Ch]
	// 
	// 2022: 4061c3 moves 0 into place where the zindex would go
	// it uses an 8B instruction (4 for 0s) so there should be room
	// for a call to restore it
	// 
	//disable enemy z-index
	//00406E1A 25 FF 00 00 00       and         eax,0FFh
	*(BYTE*)0x406E1B = 0;
	//disable NPC z-index
	//0040716D 25 FF 00 00 00       and         eax,0FFh
	*(BYTE*)0x40716E = 0;
	//disable items z-index
	//00407469 25 FF 00 00 00       and         eax,0FFh 
	*(BYTE*)0x40746A = 0;	
	//disable PC z-index
	//00407AD7 25 FF 00 00 00       and         eax,0FFh
	*(BYTE*)0x407AD8 = 0;
	//2022
	//this new code puts the zindex and tile x/z coordinates
	//into spare memory for new map change event extensions
	//for consistency with objects just doing this the same
	//way in all cases (object container insertion is inline)
	//004061FD E8 08 62 00 00       call        0040C40A //objects
	//00406C17 E8 EE 57 00 00       call        0040C40A //enemy
	//00406F6D E8 98 54 00 00       call        0040C40A //npc
	//004072C0 E8 45 51 00 00       call        0040C40A //items	
	//objects have atoi removed... unfortunately because
	//their data starts at 0x80 on the stack their store 
	//are much larger and so won't fit
	//004061BC 8D 44 24 14          lea         eax,[esp+14h]  
	//004061C0 50                   push        eax  
	//004061C1 6A 00                push        0
	//004061C3 C6 84 24 9A 00 00 00 00 mov         byte ptr [esp+9Ah],0 
	/*the lea bit needs to be reordered :(
	*(DWORD*)0x4061C3 = 0xe8909090; //call
	*(DWORD*)0x4061C7 = (DWORD&)MapComp_40c40a_obj_z-0x4061Cb;*/
	memcpy((void*)0x4061c4,(void*)0x4061BC,7);
	*(WORD*)0x4061Bc = 0xe850; //push eax, call
	*(DWORD*)0x4061be = (DWORD)MapComp_40c40a_obj_z-0x4061c2;
	*(WORD*)0x4061c2 = 0x9058; //pop eax
	*(DWORD*)0x4061Fe = (DWORD)MapComp_40c40a_obj_xy-0x406202;
	*(DWORD*)0x406C18 = (DWORD)MapComp_40c40a_etc_xyz-0x406C1c;
	*(DWORD*)0x406F6e = (DWORD)MapComp_40c40a_etc_xyz-0x406F72;
	*(DWORD*)0x4072C1 = (DWORD)MapComp_40c40a_etc_xyz-0x4072C5;

	//486FF0 IS DEALLOCATED
	//489000 AND 487FF0 SEEM NOT TO BE DEALLOCATED
	//this call ultimately wipes out 486FF0. 408F80 does 
	//so, being passed each pointer, one at a time
	//hoping (fingers crossed) this subroutine tears down
	//all memory (which is unnecessary for one-off processes)
	//004036FC E8 DF 4D 00 00       call        004084E0
	//memset((void*)0x4036FC,0x90,5);
	{
		//PRT records (486FF0)
		//need to wipe some memory away, but keep the model data
		//0040880E E8 6D 07 00 00       call        00408F80
		memset((void*)0x40880E,0x90,5);

		//NOTE: this doesn't change anything, since NPCs don't have
		//anything to do with lighting, but it's at least consistent
		//enemy (or NPC?) container
		//it is cleared, whereas its peers are not
		//00408731 74 1A                je          0040874D
		*(BYTE*)0x408731 = 0xEB; //JMP

		//texture container
		//00408805 89 0D D0 E2 43 00    mov         dword ptr ds:[43E2D0h],ecx
		//memset((void*)0x408805,0x90,6);
		//004087BB 74 4E                je          0040880B
		*(BYTE*)0x4087BB = 0xEB; //JMP

		//MHM and MSM tables?
		//00408674 89 35 B8 22 43 00    mov         dword ptr ds:[4322B8h],esi
		//004086AC 89 35 A0 22 43 00    mov         dword ptr ds:[4322A0h],esi
		memset((void*)0x408674,0x90,6);
		memset((void*)0x4086ac,0x90,6);
	}

	//407C80 writes the MPX calling 40BD7C with fwrite signature
	//004036C9 E8 B2 45 00 00       call        00407C80
	*(DWORD*)0x4036CA = (DWORD)MapComp_407c80-0x4036CE;
	//flags? bit 1 is BSP-tree, 2 is lighting?
	//00407CAF 6A 04                push        4 
	//00407CB1 68 88 72 41 00       push        417288h
	//title
	//00407CBE 6A 20                push        20h  
	//00407CC0 68 D0 6F 41 00       push        416FD0h
	//BGM
	//00407CCD 6A 20                push        20h  
	//00407CCF 68 90 5F 41 00       push        415F90h
	//BMP (3)
	//00407CDC 6A 60                push        60h  
	//00407CDE 68 10 70 41 00       push        417010h
	//stored back-to-back yet written separately
	//50,0.1,30 ... I think 50 may be light cutoff
	//30 is draw distance? ... 0.1 the near plane? fog?
	//00407CEE 6A 04                push        4  
	//00407CF0 68 D8 E2 43 00       push        43E2D8h
	//00407CFF 68 DC E2 43 00       push        43E2DCh
	//00407D0E 68 E0 E2 43 00       push        43E2E0h
	//8 bytes? 0? fog color? and?
	//00407D1B 6A 08                push        8  
	//00407D1D 68 20 E5 43 00       push        43E520h
	//ambient (code converts map record to DWORD)
	//00407D3E 6A 04                push        4 
	//00407D40 52                   push        edx
	//directional lighting follows in form of loop
	//0x00415F34-1c is 3 floats+DWORD 24B apiece
	//there is a 1.0 (intensity? ambient contribution?) 
	//field SOM_MAP can't access
	//00407DA2 83 C7 18             add         edi,18h
	//stored back-to-back yet written separately
	//1 byte?
	//00407DB3 6A 01                push        1  
	//00407DB5 68 D0 8F 47 00       push        478FD0h 
	//3 bytes?
	//00407DC0 6A 03                push        3  
	//00407DC2 6A 01                push        1  
	//00407DC4 68 D1 8F 47 00       push        478FD1h 
	//starting position (Y is 0.05 less?)
	//00407DD1 6A 04                push        4  
	//00407DD3 68 D4 8F 47 00       push        478FD4h 
	//00407DE2 68 D8 8F 47 00       push        478FD8h
	//00407DF4 68 DC 8F 47 00       push        478FDCh
	//270? starting rotation? but 90 becomes 270!
	//game convention?
	//00407E01 6A 04                push        4  
	//00407E03 68 E0 8F 47 00       push        478FE0h
	//
	//	CONTAINERS
	//  note, these are not deallocated... probably the
	//  destructors fire after main returns
	//  415F74 (512 objects, 0x44B each)
	//  48A10C (128 enemies? 0x34 each)
	//  415F84 (128 NPCs? 0x34 each)
	//  43E514 (256 items, 0x28 each)
	//
	//OBJECTS? contiguous array? 415F7C holds capacity?
	//(2 more containers follow... the 3rd isn't full?)
	//00407E0D A1 74 5F 41 00       mov         eax,dword ptr ds:[415F74h]
	//00407E1F 8B 0D 78 5F 41 00    mov         ecx,dword ptr ds:[415F78h] 
	//computes/outputs 512 (objects?) or 0 if empty	   00407E43 6A 04                push        4  
	//00407E45 51                   push        ecx  
	//00407E46 E8 31 3F 00 00       call        0040BD7C
	//
	// END-CONTAINERS?
	//
	//0? light count? (just after 32B title/416FD0 in memory)
	//00407FC4 6A 04                push        4  
	//00407FC6 68 F0 6F 41 00       push        416FF0h 
	//DISABLED?
	//EBP? 0? disabled feature? or is EBP not zero
	//00407FD7 6A 04                push        4  
	//00407FD9 52                   push        edx
	//100x100
	//00407FE6 6A 04                push        4  
	//00407FE8 68 30 E6 43 00       push        43E630h
	//00407FF7 68 34 E6 43 00       push        43E634h
	//
	//  MAP DATA 24B each, outputting 12 bytes (2+2+4+4) in loop
	//
	//BSP-Tree?
	//0040807F F6 05 88 72 41 00 01 test        byte ptr ds:[417288h],1
	//
	//478FC0 looks like CONTAINER (BSP data?)
	//0040809B 8B 0D C0 8F 47 00    mov         ecx,dword ptr ds:[478FC0h]
	//...
	//...
	//resumes post-BSP...
	//
	//another CONTAINER (textures)
	//00408320 A1 CC E2 43 00       mov         eax,dword ptr ds:[0043E2CCh]
	//must be VBUFFER (nothing else is flat/20B eaach)
	//00408384 A1 B4 5F 41 00       mov         eax,dword ptr ds:[00415FB4h]
	//
	//100x100 again? (not outputting)
	//004083FC 8B 0D 30 E6 43 00    mov         ecx,dword ptr ds:[43E630h]
	//
	//MHM related (2B per element, count counts outputted MHM)
	//00408447 A1 B4 22 43 00       mov         eax,dword ptr ds:[004322B4h]
	//
	// MHM?
	//487FF0? just after PRT table in memory... points to 16B ahead of each
	//PRT records memory (memory manager coincidence)
	//ii0026.prt: 384   40703856 (ptr)         17        241
	//ii0026.prt: 480   40709648 (ptr)         17        241
	//488FF0+16 also has such a table (489000)
	//00408490 8B 0C BD F0 7F 48 00 mov         ecx,dword ptr [edi*4+487FF0h]
	//
	//END OF SUBROUTINE (fclose?)
	//004084C0 E8 2C 33 00 00       call        0040B7F1

	//LIGHTS/LIGHTS INDEX CONTAINER
	//417074 is populated with lamps as objects are read from the MAP file
	//004064C4 8B 15 74 70 41 00    mov         edx,dword ptr ds:[417074h]
	
	//2020
	//extending the enemy limit makes the BSP and lighting flags fall on
	//variable line numbers, so this code goes directly to the source of
	//the problem
	//read 1136 (mask)
	//004074F4 E8 1C 4F 00 00       call        0040C415
	//read 1137 (light)
	//00407579 E8 97 4E 00 00       call        0040C415
	*(DWORD*)0x4074F5 = (DWORD)MapComp_40c415-0x4074F9;
	*(DWORD*)0x40757a = (DWORD)MapComp_40c415-0x40757e;

	//2021
	//loading lamp models to extract control points
    //00403820 E8 2B 76 00 00       call        0040AE50 //mdo
	//004037EA E8 81 64 00 00       call        00409C70 //mdl
	//force down mdo path
	*(WORD*)0x4037c1 = 0x45EB; //jmp 69
	*(DWORD*)0x403821 = (DWORD)MapComp_40ae50-0x403825;

	#ifdef NDEBUG
//	#error MapComp_408ea0 should have this covered
	#endif
	//2022
	//reject "" texture references in new MSM files where used to introduce
	//padding in order to align MSM model data
	//00409197 E8 54 01 00 00       call        004092F0
	*(DWORD*)0x409198 = (DWORD)MapComp_4092f0-0x40919c;
	//remove empty fields from per texture MSM records
	//004092C1 E8 AA 02 00 00       call        00409570
	*(DWORD*)0x4092C2 = (DWORD)MapComp_409570-0x4092C6;

	//2023
	//layers "BSP" system
	//remove treatment of blank tiles as occluders for layers
	//004046F9 00 99 B9 64 00 00    add         byte ptr [ecx+64B9h],bl
	memset((void*)0x4046f9,0x90,5);
	//treat all tiles as dummies
	//0040480A 0D 78 09 4C 00       or          eax,4C0978h
	memset((void*)0x40480a,0x90,2);
	//store v/e/V/E
	//00405f4c 85 c0           TEST       EAX,EAX
    //00405f4e 0f 84 6c 01 00 00   JZ     LAB_004060c0
	//00405f54 80 38 65        CMP        byte ptr [EAX],0x65
    //00405f57 75 04           JNZ        LAB_00405f5d
    //00405f59 83 4d 10 40     OR         dword ptr [EBP + 0x10],0x40
	{
		//NOTE: low bit is still 1 for 'e' and 0 for 'v'
		//xor ecx,ecx
		//mov cl, byte ptr [eax]
		//shl ecx,6
		//or dword ptr [ebp+10h],ecx
		memcpy((void*)0x405f4c,"\x31\xc9\x8a\x08\xc1\xe1\x06\x09\x4d\x10",10);
		//2024 clear final 2 bits? doesn't work. there's a bug
		//where some tiles have their "hit" (pit) bit set when
		//'E' is set in bits 6~13
		//(this bug was in reading the MPX file using a union)
		//and dword ptr [ebp+10h],3fffh
		//memcpy((void*)0x405f56,"\x81\x65\x10\xff\x3f\x00\x00",7);
		memset((void*)0x405f56,0x90,7);
	}
	//2024
	//recover pit/poison data
	//00405faa 81 4d 10 00 01 00 00       OR         dword ptr [EBP + 0x10],0x100
    //00405fbe 81 4d 10 80 00 00 00       OR         dword ptr [EBP + 0x10],0x80
	*(DWORD*)0x405fad = 0x8000;
	*(DWORD*)0x405fc1 = 0x4000;
}
