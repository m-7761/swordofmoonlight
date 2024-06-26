
//This program produces CP files for Sword of Moonlight.
//The results should be byte identical. I was surprised
//it was so for the few CP files for which I've checked.
//(Actually some have floating-point rounding error for
//what are most likely irrational numbers. A systematic
//scan of all of the old CP files would be reassuring.)

#ifndef X2MDL_INCLUDED //2021: x2mdl.cpp?

#include <assert.h>
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <math.h> //fabs

#include "../lib/swordofmoonlight.h"

//HACK: x2mdl.cpp defines these
int exit_status = 0;
const wchar_t *exit_reason = 0;

#endif //X2MDL_INCLUDED

#ifdef _DEBUG
const wchar_t *validate_against = 0; //L"O334.cp";
#else
const wchar_t *validate_against = 0;
#endif
#ifdef _CONSOLE //2021
bool validate(FILE *a, const wchar_t *b);
#else
bool validate(FILE*,const wchar_t*){ return true; }
#endif

#ifndef _CONSOLE
X2MDL_API
#endif
#ifdef X2MDL_INCLUDED
int cpgen(int argc, const wchar_t* argv[]) //2021
#else
int wmain(int argc, const wchar_t* argv[])
#endif
{
#ifdef _DEBUG

	//Hmmm: have never needed this before??
	_set_error_mode(_OUT_TO_MSGBOX); 

#endif

	//todo: consider exception handling
#define _or_exit ; if(!in||exit_status) goto _1 

	if(argc<2||!argv[1]||!*argv[1])	goto _1;

	for(int arg=1;arg<argc;arg++) //2022
	{
		const wchar_t *input = argv[arg];

		FILE *out = 0; float *cpfile = 0;

		namespace mdl = SWORDOFMOONLIGHT::mdl;

		mdl::image_t mdl_file; //readonly

		const mdl::image_t &in = mdl_file;

		mdl::maptofile(mdl_file,input,'r',0x1f)_or_exit;
	
		const mdl::header_t &hin = mdl::imageheader(in)_or_exit;

	//	if(hin.softanims) goto _1; //todo: support
	
	//	if(!hin.hardanims) goto _1; //meaningful??
	
		int cps = 0, cpchans[33], cpindex[33]; 

		for(int i=0;i<33;i++) cpindex[i] = -1; //new way
	
		float cpx[33], cpy[33], cpz[33];
	
		int cpmax = 1; int basecp = -1; bool basecpneeded = false;

		int cplog = 0; std::cout << "Logging control points (if any)\n...\n";

		int n = hin.primchans;

		//softanimation
		int soft = 0;
		int softindex[33][3]; 	
		typedef float softris_t[33][3][3];
		softris_t softris0;

		for(int i=0;i<n;i++)
		{		
			int prims = mdl::countprimitivesinset(in,i,mdl::controlprimset)_or_exit;

			if(prims==0) continue;

			const mdl::primch_t &ch = mdl::primitivechannel(in,i)_or_exit;
			const mdl::vertex_t *vs = mdl::pervertexlocation(in,i)_or_exit;

			if(!vs) goto _1; 

			for(int j=0;j<prims;j++)
			{
				mdl::triangle_t cp;

				cp = mdl::triangulateprimitive(in,i,mdl::controlprimset,j)_or_exit;
				
				if(!cp) goto _1; 

				printf("#%d %03dr%03dg%03db %+dx%+dy%+dz\n...\n",
					++cplog,(int)cp.rgb[0],(int)cp.rgb[1],cp.rgb[2],
					vs[cp.pos[0]].x,vs[cp.pos[0]].y,vs[cp.pos[0]].z);

				int rgb = cp.rgb[0]<<16|cp.rgb[1]<<8|cp.rgb[2];

				//CP files only include the cyan base CP and red CPs
				if(rgb>>16!=0xFF)
				{
					if(rgb!=0x00FFFF) continue; //cyan?
				}
				else if((rgb&0xFF)==0) //red?
				{
					//reds go up to 31
					if((rgb&0x00FF00)>>8>31) continue;


				}
				else continue; //other? MDLs do have others

				if(cps==33)	//maximum number of cps exceeded
				{
					i = n; break; //todo: warning message
				}

				for(int k=0;k<3;k++) 
				if(cp.pos[k]>=ch.vertcount) goto _1;

				if(hin.softanims) //2020
				{
					auto &si = softindex[soft];
					auto &st = softris0[soft];
					soft++;
					for(int k=3;k-->0;)
					{
						si[k] = cp.pos[k];
						st[k][0] = vs[cp.pos[k]].x*mdl::m;
						st[k][1] = vs[cp.pos[k]].y*mdl::m;
						st[k][2] = vs[cp.pos[k]].z*mdl::m;
					}
				}
			
				cpx[cps] = cpy[cps] = cpz[cps] = 0;

				for(int k=0;k<3;k++) //cp is centroid
				{
					cpx[cps]+=vs[cp.pos[k]].x;
					cpy[cps]+=vs[cp.pos[k]].y;
					cpz[cps]+=vs[cp.pos[k]].z;
				}
					   
				const float scale = 1.0/1024.0; //to meters

				cpx[cps]/=3.0f; cpx[cps]*=scale; 
				cpy[cps]/=3.0f; cpy[cps]*=scale; 
				cpz[cps]/=3.0f; cpz[cps]*=scale;

				if(rgb==0x00FFFF) basecp = cps; //cyan

				//cpindex[cps] = cps; //unsorted

				if(basecp!=cps)
				{
					int green = (rgb&0x00FF00)>>8;

					if(green>31) continue; //hack
				
					//dup: want cps to reflect total
					if(cpindex[1+green]!=-1) continue;

					cpindex[1+green] = cps; //assume red

					if(2+green>cpmax) cpmax = 2+green;
				}
				else cpindex[0] = cps;			

				cpchans[cps++] = i;
			}
		}

		if(!cps||basecp==-1) //cyan: add root cp
		{
			if(cps==33) cps--; //hack: delete final CP

			cpindex[0] = basecp = cps++; basecpneeded = true; 		
		
			cpx[basecp] = cpy[basecp] = cpz[basecp] = 0; //2020
		}

		int hardanis = hin.hardanims;
		int softanis = hin.softanims;

		int anis = hardanis+softanis;

		mdl::const_animation_t aniptrs[256];

		if(mdl::animations(in,aniptrs,256)!=anis||!in) goto _1; 
	
	#ifdef SWORDOFMOONLIGHT_CAN_VALIDATE

		exit_reason = i18n_anifailure;

		for(int i=0;i<anis;i++)
		{
			mdl::validate(in,aniptrs[i],i)_or_exit; //???
		}

	#endif

		int sumtimeofanis = 0;
		for(int i=0;i<hardanis;i++)
		sumtimeofanis+=aniptrs[i]->htime;

		if(anis>hardanis) //soft animations
		{
			assert(0==hardanis);

			void *e = 0; //unimplemented
		
			for(int i=0;i<softanis;i++)
			{
				sumtimeofanis+=1+mdl::softanimframestrtime(aniptrs[i]->frames,e);
			}
		}

		//Note: writing .cp file to memory first because a .cp
		// file is written in cp order. However it's easier to 
		// calculate in animation order. So to avoid accessing
		// the file in a very random way, we do this instead.

		int cpfilesize = 34+anis+sumtimeofanis*cps*3;

		cpfile = new float[cpfilesize]; assert(sizeof(float)==4);

	#ifdef _DEBUG

		memset(cpfile,0xFF,cpfilesize*sizeof(float));

	#endif

		int *cphead = (int*)cpfile; assert(sizeof(int)==4);
	
		cphead[0] = anis;

		for(int i=1;i<34;i++) cphead[i] = 0xFFFFFFFF;

		int cpgap = 0;
	
		//2020: is this required if there aren't animations present?
		if(anis) cphead[34] = 0; else assert(34==cpfilesize);

		for(int i=0;i<anis;)
		{
			int time; if(i>=hardanis) 
			{
				time = 1+mdl::softanimframestrtime(aniptrs[i]->frames);
			}
			else time = aniptrs[i]->htime;

			int accum = cphead[34+i]+time*12;

			(++i==anis?cpgap:cphead[34+i]) = accum;
		}
	
		cphead[1] = 34*4+anis*4;

		for(int i=1,j=0;i<cpmax;i++) 
		{		
			if(cpindex[i]!=-1) //gaps are free to exist
			{
				cphead[1+i] = cphead[1+j]+cpgap; j = i;
			}
		}

		float *cpbody = cpfile+34+anis;	

		int cpbodysize = cpfilesize-34-anis; //debugging

		float baseinv[3] = {};

		if(hardanis) //BYTE IDENTICAL FOR N032.CP
		{
			int anichans = mdl::animationchannels<mdl::hardanim_t>(in)_or_exit;

			mdl::hardanim_t animap[256];

			if(mdl::mapanimationchannels(in,animap,anichans)!=anichans||!in) goto _1;

			if(basecpneeded) 
			for(int i=0;i<anichans;i++) if(animap[i].map==-1) //root
			{
				//cpx[basecp] = cpy[basecp] = cpz[basecp] = 0;

				cpchans[basecp] = i; basecpneeded = false; //?

				break;
			}
			#ifdef _CONSOLE
			if(basecpneeded) goto _1; //paranoia
			#endif

			float xmats[256*16]; bool xmatsknown[256];

			for(int i=0;i<hardanis;i++)
			{
				int now = 0;

				for(int j=0;j<aniptrs[i]->htime;j++)
				{
					now = mdl::animate(aniptrs[i],animap,anichans,now,!i);

					float cpbase[3] = {0,0,0};

					for(int k=0,l=-1;k<33;k++) if(cpindex[k]!=-1) //k<cps
					{
						int iK = cpindex[k]; l++;

						float xyz[3] = { cpx[iK], cpy[iK], cpz[iK] };

						memset(xmatsknown,0x00,sizeof(bool)*anichans);

						if(!mdl::transform(animap,cpchans[iK],xyz,mdl::m3,xmats,xmatsknown))
						goto _1;
					
						int cur = cpgap/4*l+cphead[34+i]/4+j*3; 

						assert(cur<cpbodysize);

					//	assert(xyz[0]!=1); //??? //2020
					
						if(k==0) //base
						{
							if(j==0)
							{
								cpx[iK] = xyz[0];
								cpy[iK] = xyz[1];
								cpz[iK] = xyz[2];

								baseinv[0] = -cpx[iK];
								baseinv[1] = -cpy[iK];
								baseinv[2] = -cpz[iK];
							}

							for(int ii=3;ii-->0;) 
							{
								cpbase[ii] = xyz[ii]+=baseinv[ii];
							}
						}
						else for(int ii=3;ii-->0;)
						{
							xyz[ii]-=cpbase[ii];
						}

						cpbody[cur++] = +xyz[0];
						cpbody[cur++] = -xyz[1];
						cpbody[cur++] = +xyz[2];
					}				
				}
			}

			assert(!softanis);
		}
		else if(softanis)  //BYTE IDENTICAL FOR N033.CP
		{
			assert(!hardanis); //goto _1; //unimplemented

			softris_t softris1,softris2; 

			for(int i=0;i<softanis;i++)
			{
				int now = 0;

				auto *fp = aniptrs[i]->frames;
				int time = 1+mdl::softanimframestrtime(fp);
			
				auto pastris = &softris0;
				auto softris = &softris0;			

				for(int j=0,jt=1;j<time;j++)			
				{
					float t; if(!--jt)
					{	
						pastris = softris;
						softris = softris==&softris1?&softris2:&softris1;
						memcpy(softris,pastris,sizeof(softris0));
						//this reproduces byte identical original CP files
						//if(j>1) 
						if(j) 
						fp++; 
						//if(j) 
						mdl::translate(&mdl::softanimchannel(in,fp),soft*3,***softris,mdl::m3,(int*)softindex);
						jt = fp->time;
						t = 0;
					}
					else t = 1-float(jt)/fp->time;

					float cpbase[3] = {0,0,0};

					for(int k=0,l=-1;k<33;k++) if(cpindex[k]!=-1) //k<cps
					{
						int iK = cpindex[k]; l++;

						float xyz[3]; if(!j||!k&&basecpneeded)
						{
							xyz[0] = cpx[iK];
							xyz[1] = cpy[iK];
							xyz[2] = cpz[iK];
						}
						else 
						{
							auto &st1 = (*pastris)[iK];
							auto &st2 = (*softris)[iK];
							memset(xyz,0x00,sizeof(xyz));
							for(int ll=3;ll-->0;)
							for(int l=3;l-->0;)
							{
								float x = st1[ll][l];
								float y = st2[ll][l];

								xyz[l]+=x+(y-x)*t; //lerp
							}
							for(int l=3;l-->0;) xyz[l]/=3;
						}
		
						int cur = cpgap/4*l+cphead[34+i]/4+j*3; 

						assert(cur<cpbodysize);

						if(k==0) //base
						{
							if(j==0) //UNTESTED (SPECULATVE)
							{
								//2021: I had an old note that N033.CP seems
								//to have some extra logic that sounds a lot
								//like the "baseinv" system I set up for hard
								//animations above. I specualted it had to do
								//with smooth delta calculation but I think it
								//may be a way to ensure 0,0,0 is the center of
								//the model even if the base CP is off-centered
								//for soft-animations this is very simple given
								//that a base CP cannot be inferred from joints
								//still I've implmented it analogously in order
								//to be transparent

								//NOTE: should be identical to -cpx[iK], etc.
								for(int ii=3;ii-->0;) baseinv[ii] = -xyz[ii];
							}
							assert(!basecpneeded||0==xyz[1]); //removing branch

							for(int ii=3;ii-->0;) 
							{
								cpbase[ii] = xyz[ii]+=baseinv[ii];
							}
						}
						else for(int ii=3;ii-->0;)
						{
							xyz[ii]-=cpbase[ii];
						}

						cpbody[cur++] = +xyz[0];
						cpbody[cur++] = -xyz[1];
						cpbody[cur++] = +xyz[2];
					}
				}
			}
		}
			//make diff comparable to the original CP files?
			//the original CP files don't include -0 entries
			//#ifdef _DEBUG
			for(int i=cpbodysize;i-->0;)
			{
				if(!cpbody[i]) cpbody[i] = 0;
			}
			//#endif

		wchar_t output[_MAX_PATH];

		int inputlen = wcslen(input); 

		if(inputlen>_MAX_PATH-1) goto _1; //even possible??
	
		if(inputlen>4&&!wcscmp(input+inputlen-4,L".mdl"))
		{
			wcscpy(output,input); wcscpy(output+inputlen-4,L".cp");
		}
		else if(inputlen>4&&!wcscmp(input+inputlen-4,L".MDL"))
		{
			wcscpy(output,input); wcscpy(output+inputlen-4,L".CP");
		}
		else swprintf(output,_MAX_PATH,L"%s.cp",input);

		if(validate_against)
		{
			out = _wfopen(output,L"wb+"); //debugging 
		}
		else out = _wfopen(output,L"wb");
		
		if(!out||!fwrite(cpfile,cpfilesize*4,1,out)) goto _1;
		
		validate(out,input);

_0:		if(out) fclose(out);

		if(cpfile) delete[] cpfile;

		mdl::unmap(mdl_file)_or_exit;
	}

	//nuisance for real work
	//#ifdef _CONSOLE
	//extern "C" //#include <windows.h> //Sleep
	//__declspec(dllimport) void __stdcall Sleep(unsigned long);
	//Sleep(4000);
	//#endif

	return exit_status; //skipped in release build??

	//TODO: REMOVE ME!!

_1: static int once = 0; //compiler stupidity!?

	if(!once++) 
	{
		exit_status = 1;

		if(exit_reason)
		{
			//todo: translate/print reason

			std::wcerr << exit_reason;
		}
		else assert(0);
		
		goto _0;
	}

	#ifdef _CONSOLE
	exit(exit_status); //might as well try
	#endif
	return exit_status; //2021
}

#ifdef _CONSOLE //2021
bool validate(FILE *a, const wchar_t *B)
{
	if(!a||!B||!validate_against) return false;

	FILE *b = _wfopen(validate_against,L"rb");	
	
	if(!b) //2020
	{
		wchar_t path[_MAX_PATH];

		int len = wcslen(B); while(len-->0)
		{
			if(B[len]=='\\'||B[len]=='/')
			{
				len++; break;
			}
		}
		if(len>0)
		{
			wcscpy(path,B);
			wcscpy(path+len,validate_against);
			b = _wfopen(validate_against,L"rb");
		}
		else assert(len>0);
	}

	assert(b);

	if(!b) return false;

	fseek(a,0,SEEK_END);
	fseek(b,0,SEEK_END);

	if(ftell(a)!=ftell(b)) 
	{
		fclose(b); assert(0); return false;
	}

	size_t fsize = ftell(a);

	assert(sizeof(float)==4);

	if(fsize%4!=0)
	{
		fclose(b); assert(0); return false;
	}

	fseek(a,0,SEEK_SET);
	fseek(b,0,SEEK_SET);

	int same = 0, diff = 0;

	std::wcout << std::hex << std::setprecision(4);

	for(int i=0;i<fsize;i+=4)
	{
		float f, g;

		if(!fread(&f,4,1,a)||!fread(&g,4,1,b))
		{
			fclose(b); assert(0); return false;
		}

		//NOTE: I think this was because the weird PlayStation style 1024 
		//scale factor was unknown at the time... I assumed it was 1000mm 
		//if(fabsf(f-g)>0.04f) //observed acceptable deviation
		if(f!=g&&fabsf(f-g)>0.000001f) //2020: arbitrary rounding epsilon 
		{
			int x = i%3;

			std::wcout << i << "! " << std::setw(8) << f << ' ' << std::setw(8) << g << '\n';

			diff++; //woops
		}
		else same++;
	}
		
	std::wcout << std::dec;

	if(diff)
	{
		std::wcout << diff << " different (out of " << same+diff << ")\n";
	}
	
	fclose(b);	

	return diff==0;
}
#endif //_CONSOLE
