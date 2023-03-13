
#include "Daedalus.(c).h"

using namespace Daedalus;

HMODULE PreServe_dll(preX &io)
{
	wchar_t w[MAX_PATH]; int len =
	MultiByteToWideChar(65001,0,io.String(),io.length+1,w,MAX_PATH);
	if(len<=0) return 0;
	int slash = 0;
	for(int i=0;i<len;i++) if(w[i]=='/') slash = w[i] = '\\';	
	HMODULE out = 0;
	//prepending to make commandline simpler
	const wchar_t *PreServe_ = L"PreServe_";
	if(slash!='\\'&&wcsnicmp(PreServe_,w,9)&&len<MAX_PATH-10)
	{
		wmemmove(w+9,w,len); len+=9;
		out = LoadLibrary(PreServe_=wmemcpy(w,PreServe_,9));
	}
	if(!out&&w==PreServe_) //liklihood is less
	out = LoadLibrary(wmemmove(w,w+9,len-=9));
	if(!out) return 0;
	len = GetModuleFileName(out,w,MAX_PATH)+1;
	for(int i=0;i<len;i++) if(w[i]=='\\') slash = w[i] = '/';
	char u[MAX_PATH*4]; if(len>1) len = 
	WideCharToMultiByte(65001,0,w,len,u,sizeof(u),0,0);
	if(len<=0){	FreeLibrary(out); return 0; } //extreme paranoia	
	io.SetString(u,len-1); return out;
};

bool PreServe_mmap(preServer *p)
{
	preX &x = const_cast<preX&>(p->resourcenameIn);	
	wchar_t w[MAX_PATH]; int len =
	MultiByteToWideChar(65001,0,x.String(),x.length+1,w,MAX_PATH);
	if(len<=0) return false;
	wchar_t *scheme = wcschr(w,':'); if(scheme&&scheme-w>1)
	{
		//todo: permit HTTP:// over?
		if(!wcsnicmp(w,L"file:/",5)) //5:6: UNC or volume?
		{				
			int mv = w[6]=='/'?5:6; if(len>mv+1) wmemmove(w,w+mv,len-=mv);
		}
	}
	HANDLE g = CreateFileW(w,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
	const_cast<size_t&>(p->amountofcontentIn) = GetFileSize(g,0);
	HANDLE h = CreateFileMapping(g,0,PAGE_WRITECOPY,0,p->amountofcontentIn,0);
	(void*&)p->contentIn = MapViewOfFile(h,FILE_MAP_COPY,0,0,p->amountofcontentIn);
	CloseHandle(h);	CloseHandle(g);
	return !!p->contentIn;
}
void PreServe_munmap(void* &v)
{
	UnmapViewOfFile(v); 
}

DAEDALUS_API int Daedalus::PreServe(preServer *p)
{	
	HMODULE lib = PreServe_dll(const_cast<preX&>(p->pathtomoduleIn));
	int(*proc)(preServer*) = 0;
	if(lib) proc = (int(*)(preServer*))GetProcAddress(lib,"PreServe");

	int test = !p->amountofcontentIn; assert(!p->contentIn);
	if(proc!=PreServe) //extreme paranoia: is it even possible??
	if(proc&&PreServe_mmap(p)) test = proc(p); 
		
	int exit_code = test==p->amountofcontentIn;
	const_cast<size_t&>(p->amountofcontentIn) = 0;
	PreServe_munmap((void*&)p->contentIn);
		
	#ifdef NDEBUG
	#error must validate strings
	#endif
	struct : PreX::Pool 
	{
		PreNew<char*[]>::Vector out;
		typedef std::pair<char*,size_t> newcopy;
		//dunno if using std::string here is the best idea or not
		std::unordered_map<const char*,newcopy,std::hash<std::string>> in;

		newcopy inout(const char *x)
		{
			auto &ins = in[x]; if(!ins.first)
			{
				ins.second = strlen(x);
				memcpy(ins.first=new char[ins.second+1],x,ins.second+1);
				out.PushBack(ins.first);

			}return ins;
		}
		virtual bool PoolString(PreX &x)
		{
			newcopy io = inout(x.String());
			x.PoolString(io.first,io.second); return true;
		}
		virtual bool PoolMaterialString(const char* &x)
		{
			assert(x); x = inout(x?x:"").first; return true; 
		}
		virtual void GetDeleteBuffers(preN &x, char** &xlist)
		{
			x = out.Size(); xlist = out.MovePointer();
		}
	}pool; auto &f = [&](preScene* &x)
	{ if(auto y=x){ x = new preScene(*y,&pool); y->serverside_delete(y); } };
	f(p->simpleSceneOut); 
	f(p->complexSceneOut);
	f(p->terrainSceneOut);
	FreeLibrary(lib);
	return exit_code;
}


