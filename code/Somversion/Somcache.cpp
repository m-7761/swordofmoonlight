
#include "Somversion.pch.h" 

#include "../lib/swordofmoonlight.h"

typedef struct 
{
	SWORDOFMOONLIGHT::zip::mapper_t z;

	Somcache_progress progress; size_t sz;

	void *dialog; HANDLE thread; DWORD threadid;
	
	wchar_t source[MAX_PATH], destination[MAX_PATH+1];	

}Somcache_instance;

static DWORD WINAPI Somcache_threadmain(Somcache_instance*);

extern bool Somcache(wchar_t inout[MAX_PATH], Somcache_progress progress, void *dialog)
{
	if(!inout||!*inout) return false;

	bool out = false;
	namespace zip = SWORDOFMOONLIGHT::zip;
	zip::mapper_t z;	
	if(out=zip::open(z,inout))
	{	
		Somcache_instance 
		*in = new Somcache_instance;
		in->z = z;
		in->sz = 0;
		zip::entries_t e = zip::entries(z);
		for(size_t i=0,n=zip::count(z);i<n;i++) 
		in->sz+=e[i]->filesize;
		in->progress = progress;
		in->dialog = dialog;
		wcscpy_s(in->source,inout);
		wchar_t *d = wcsrchr(inout,'\\');
		wchar_t caching[MAX_PATH] = L"";
		swprintf(caching,L"\\caching\\%s",d?d+1:inout);			
		wcscpy_s(in->destination,MAX_PATH,Somfolder(caching));	
		in->thread = 
		CreateThread(0,0,
		(LPTHREAD_START_ROUTINE)Somcache_threadmain,
		in,0,&in->threadid);			
		if(!in->thread)
		{
			delete in; out = false;
		}
		else wcscpy(inout,in->destination);
	}
	if(!out) zip::close(z);
	return out;
}

static DWORD WINAPI Somcache_threadmain(Somcache_instance *in)
{
	namespace zip = SWORDOFMOONLIGHT::zip;
	
	int statistics[6] = {0,in->sz,0,0,0,0};

	DWORD t0 = GetTickCount(), Bs = 0, per = t0;

	wchar_t x[MAX_PATH] = L"";
	size_t xcat = swprintf(x,L"%ls\\",in->destination);
	size_t xcat_s = xcat>MAX_PATH?0:MAX_PATH-xcat;

	bool complete = false, canceled = false;	

	zip::inflate_t b;
	zip::entries_t p = zip::entries(in->z);
	size_t fue = zip::firstunicodeentry(in->z);
	for(size_t i=0,n=zip::count(in->z);i<n;i++) 
	{	
		zip::entry_t &e = **p++;
		if(!e.filesize) continue; //directory?

		FILE *f = 0;
		if(MultiByteToWideChar(i<fue?437:65001,0,(char*)e.name,e.namesize,x+xcat,xcat_s))
		f = _wfopen(x,L"wb"); 
		if(!f) break;

		zip::image_t le;
		zip::maptolocalentry(le,in->z,e);  
		size_t remaining = e.filesize; 		
		for(b.restart=0;remaining;)
		{
			size_t wr = zip::inflate(le,b);	
			
			if(!wr||wr>remaining||!fwrite(b,wr,1,f)) break;		

			statistics[0]+=wr; remaining-=wr;
			
			if(statistics[0]==statistics[1])
			{
				complete = true; break; //finish up outside loop
			}
			else
			{
				DWORD now = GetTickCount();

				if(now-per>250
				||!remaining&&i<n-1&&b.restart==e.bodysize) //2021
				{
					float seconds = float(now-per)/1000;
					statistics[2] = float(statistics[0]-Bs)/seconds;
					per = now; Bs = statistics[0]; 
					statistics[3] = float(now-t0)/1000;
					if(!in->progress(x+xcat,statistics,in->dialog))
					{
						canceled = true; break;
					}
				}
			}
		}
		zip::unmap(le); fclose(f); 		
		if(!remaining&&b.restart==e.bodysize)
		{
			if(i!=n-1) statistics[4]++;
		}
		else break;
	}
	
	if(statistics[1]>0)
	{
		if(statistics[0]>=statistics[1])
		{
			statistics[1] = complete?statistics[0]:0;
		}
		else statistics[1] = 0; //incomplete...
	}
	else if(!complete) statistics[1] = 0; //assuming??
	
	if(!canceled)
	while(in->progress(x+xcat,statistics,in->dialog))
	{
		Sleep(250); assert(0); //keeping alive?
	}	

	zip::close(in->z);
	delete in; 
	return S_OK;
}