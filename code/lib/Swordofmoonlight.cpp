
#include <assert.h> //assert
#include <stdio.h> //fopen
#include <stdlib.h> //bsearch?
#include <string.h> //memcpy
#include <math.h> //sin/cos

#ifdef _WIN32
#include <io.h>
#include <Windows.h>
#else
#include <unistd.h> //dup
#include <sys/mman.h>
#endif

//2017: In a hurry.
#include <ctype.h> //tolower
#include <wctype.h> //iswspace //TODO? isspace
#include <wchar.h> //getwc

#ifndef min
#define min(a,b) (((a)<(b))?(a):(b)) 
#define max(a,b) (((a)>(b))?(a):(b)) 
#endif

#define SWORDOFMOONLIGHT_INTERNAL
#include "swordofmoonlight.h" 
#undef environ

using namespace SWORDOFMOONLIGHT;
typedef mdl::int2 int2; //YUCK

static size_t //Swordofmoonlight_maptofile_format
Swordofmoonlight_mhm(FILE *f, int &mask, size_t &headbytes)
{
	size_t fsize = 0; headbytes = 24;

	fseek(f,0,SEEK_END); fsize = ftell(f);

	if(fsize%4||fsize<sizeof(mhm::header_t)) return 0;

	if(mask==1) return headbytes;

	int8_t buffer[sizeof(mhm::header_t)];
	size_t fwords = sizeof(mhm::header_t)/4;

	fseek(f,0,SEEK_SET);
	if(fread(buffer,sizeof(mhm::header_t),1,f))
	{
		mhm::header_t &peek = *(mhm::header_t*)buffer;

		fwords+=peek.vertcount*3;
		fwords+=peek.normcount*3;
		fwords+=peek.facecount*sizeof(mhm::face_t)/4;
	}
	else return 0;

	//REMINDER: THE INDEX COUNT IS VARIABLE LENGTH

	if(fwords*4>fsize) return 0; //probably not a MHM file

	return fsize;
}

static size_t //Swordofmoonlight_maptofile_format
Swordofmoonlight_msm(FILE *f, int &mask, size_t &headbytes)
{
	//REMINDER: similar to Swordofmoonlight_mdo() below
		
	size_t fsize = 0; 
	
	fseek(f,0,SEEK_END); fsize = ftell(f);

	const int headroom_s = 1024; 

	int8_t headroom[headroom_s]; //binary scanf?

	fseek(f,0,SEEK_SET);
		
	size_t rd = fread(&headroom,1,headroom_s,f);

	//2: MDO has 4 (32bit) here
	headbytes = 2; if(rd<headbytes) return 0;
		
	msm::textures_t &peek = *(msm::textures_t*)headroom; 

	int n = peek.count;
	for(int i=0;n&&headbytes<rd;i++)
	{
		headbytes++; if(!peek.refs[i]) n--;		
	}	

	if(headbytes==rd&&n||n) return 0; //woops
 
	//NOTE: unlike MDO, MSM is a totally unaligned format

	//align head with the start of -materials- 
	//if(headbytes%4) headbytes+=4-headbytes%4; 

	if(mask==1) return headbytes; return fsize;
}

static size_t //Swordofmoonlight_maptofile_format
Swordofmoonlight_mdo(FILE *f, int &mask, size_t &headbytes)
{
	//REMINDER: similar to Swordofmoonlight_msm() above

	//NOTE: unlike MSM, MDO is a 32bit unaligned format

	size_t fsize = 0; 
	
	fseek(f,0,SEEK_END); fsize = ftell(f);

	const int headroom_s = 1024; 

	int8_t headroom[headroom_s]; //binary scanf?

	fseek(f,0,SEEK_SET);
		
	size_t rd = fread(&headroom,1,headroom_s,f);

	//4: MSM has 2 (16bit) here
	headbytes = 4; if(rd<headbytes) return 0;
		
	mdo::textures_t &peek = *(mdo::textures_t*)headroom; 

	int n = peek.count;
	for(int i=0;n&&headbytes<rd;i++)
	{
		headbytes++; if(!peek.refs[i]) n--;		
	}	

	if(headbytes==rd&&n||n) return 0; //woops
 
	//NOTE: unlike MSM, MDO is 32bit aligned
	
	//align head with the start of materials 
	if(headbytes%4) headbytes+=4-headbytes%4; 

	if(mask==1) return headbytes;

	if((mask&~0x3)==0) 
	{
		fseek(f,headbytes,SEEK_SET);
		
		uint32_t mats = 0; 
		if(!fread(&mats,4,1,f)) return 0; 		

		assert(sizeof(float)==4);

		//header + materials + control points
		size_t out = headbytes+4+mats*32+4*12;

		return out>fsize?0:out;
	}

	return fsize;
}

static size_t Swordofmoonlight_mdl_mask(mdl::header_t &peek, int &mask, size_t fsize)
{
	size_t fwords = sizeof(mdl::header_t)/4;

	if(peek.primchanwords==0) mask&=~0x04;
	if(peek.hardanimwords==0) mask&=~0x08;
	if(peek.softanimwords==0) mask&=~0x10;
		
	if(peek.timblocks<1) mask&=~0x0100;
	if(peek.timblocks<2) mask&=~0x0200;
	if(peek.timblocks<3) mask&=~0x0400;
	if(peek.timblocks<4) mask&=~0x0800; 		
	if(peek.timblocks<5) mask&=~0x1000; 		
	if(peek.timblocks<6) mask&=~0x2000; 		
	if(peek.timblocks<7) mask&=~0x4000; 		
	if(peek.timblocks<8) mask&=~0x8000; 		

	if(mask) mask|=1; if(mask&~3) mask|=2;

	//2018: WTH was this about? mask<4???
	if(mask&2&&(unsigned)mask<4) 
	{
		//I think this excludes primchanwords
		assert(0==(mask&~0x3));

		fwords+=7*peek.primchans; 		
	}
	
	if(mask&~0x3) fwords+=peek.primchanwords;
	if(mask&~0x7) fwords+=peek.hardanimwords;
	if(mask&~0xF) fwords+=peek.softanimwords;		
				
	//TODO: size up TIM images on 8 10 20 40 

	if(mask&~0xFF) fwords = fsize/4+(fsize%4?1:0); return fwords;
}
static size_t //Swordofmoonlight_maptofile_format
Swordofmoonlight_mdl(FILE *f, int &mask, size_t &headbytes)
{
	size_t fsize = 0; headbytes = 16; 
	
	fseek(f,0,SEEK_END); fsize = ftell(f);

	if(fsize<sizeof(mdl::header_t)) return 0;
	
	int8_t buffer[sizeof(mdl::header_t)];
	size_t fwords = 0;

	fseek(f,0,SEEK_SET);
	if(fread(buffer,sizeof(mdl::header_t),1,f))
	{
		mdl::header_t &peek = *(mdl::header_t*)buffer;

		//copied by Swordofmoonlight_mapheader_format
		fwords = Swordofmoonlight_mdl_mask(peek,mask,fsize);

		if(fwords*4>fsize+4) return 0; //probably not a MDL file
	}
	else return 0; 
	
	assert(fsize>=fwords*4); //MDL should be multiple of 4 bytes
	
	return fwords*4;
}

struct Swordofmoonlight_tim_peek
{
	tim::header_t header;
	tim::pixmap_t pixmap;
	char buffer[sizeof(tim::highcolor_t)*256+sizeof(tim::pixmap_t)];
	tim::pixmap_t &pixmap2()
	{
		assert(header.cf);
		return *(tim::pixmap_t*)((char*)&pixmap+pixmap.bnum);
	}
};
static size_t Swordofmoonlight_tim_mask(Swordofmoonlight_tim_peek *peek, int &mask, size_t fsize)
{
	size_t fwords = sizeof(tim::header_t)/4; //2
			
	size_t bytesize = 0;
	tim::pixmap_t *pix = &peek->pixmap;

	if(peek->header.cf)
	{
		size_t bnum = pix->bnum;

		//require 16bpp 256/16 entry palette
		if(bnum<(peek->header.pmode?512:32)+sizeof(*pix)) 
		return 0; 

		if(pix->w*2*pix->h+sizeof(*pix)>bnum) 
		return 0;

		bytesize+=bnum;

		pix = &peek->pixmap2();
	}
	else return 0;

	if(pix->w*2*pix->h+sizeof(*pix)>pix->bnum) 
	return 0;

	bytesize+=pix->bnum;

	if(sizeof(tim::header_t)+bytesize>fsize) 
	return 0;

	int plus = bytesize/4*4!=bytesize?1:0;

	fwords+=bytesize/4+plus;

	mask&=peek->header.cf?3:1; return fwords;
}
static size_t //Swordofmoonlight_maptofile_format
Swordofmoonlight_tim(FILE *f, int &mask, size_t &headbytes)
{
	size_t fsize = 0; headbytes = 8;
	
	fseek(f,0,SEEK_END); fsize = ftell(f);

	if(fsize<sizeof(tim::header_t)) return 0;

	Swordofmoonlight_tim_peek peek;
	size_t fwords = 0;

	fseek(f,0,SEEK_SET);
	if(fread(&peek,sizeof(tim::header_t),1,f))
	{
		if(!peek.header)
		return 0; //assuming header is invalid

		if(fread(&peek.pixmap,sizeof(tim::pixmap_t),1,f))
		if(!peek.header.cf
		||fread(&peek.pixmap2(),sizeof(tim::pixmap_t),1,f))
		{
			//copied by Swordofmoonlight_mapheader_format
			fwords = Swordofmoonlight_tim_mask(&peek,mask,fsize); 

			if(fwords*4>fsize+4) return 0; //probably not a TIM file
		}
	}
	else return 0; return fwords*4;
}

static size_t //Swordofmoonlight_maptofile_format
Swordofmoonlight_txr(FILE *f, int &mask, size_t &headbytes)
{
	size_t fsize = 0; headbytes = 16;
	
	fseek(f,0,SEEK_END); fsize = ftell(f);

	if(fsize<sizeof(txr::header_t)) return 0;

	int8_t buffer[sizeof(txr::header_t)];
	size_t fwords = sizeof(txr::header_t)/4;

	fseek(f,0,SEEK_SET);
	if(fread(buffer,sizeof(txr::header_t),1,f))
	{
		txr::header_t &peek = *(txr::header_t*)buffer;

		if(!peek) return 0; //assuming header is invalid

		if(peek.depth<=8) fwords+=256; //palette 

	//Not clear how non-equal-non-pow2 dimensions work??

		int cols = peek.depth/8;
		int mips = peek.mipmaps, maps=0;

		int w = peek.width;
		int h = peek.height;

		int newmask = 0;

		while(mips--) if(mask&1<<maps)
		{	
			int rows = cols*w; 

			if(rows%4) rows+=4-rows%4; 

			fwords+=rows/4*h; newmask|=1<<maps++;

			if(w==1&&h==1) //final possible mipmap
			{	
				if(mips!=0)
				{
					//TODO: some kind of warning
				}

				break;
			}

			w = max(w/2,1);
			h = max(h/2,1);
		}
		else break;
				
		if(fwords*4>fsize) return 0;

		mask = newmask;
	}
	else return 0;

	return fwords*4;
}

static size_t //Swordofmoonlight_maptofile_format
Swordofmoonlight_evt(FILE *f, int &mask, size_t &headbytes)
{
	size_t fsize = 0;
	
	fseek(f,0,SEEK_END); fsize = ftell(f);

	if(fsize<4) return 0;

	uint32_t count;
	fseek(f,0,SEEK_SET);
	if(fread(&count,4,1,f))
	{
		headbytes = 0; //4+count*252;

		if(count!=1024 //?
		||4+count*252>fsize) return 0;
	}
	else return 0;

	return mask&2?fsize:4+count*252;
}

static size_t //Swordofmoonlight_maptofile_format
Swordofmoonlight_mo(FILE *f, int &mask, size_t &headbytes)
{										
	int8_t alias[sizeof(mo::header_t)];
	mo::header_t &peek = (mo::header_t&)alias; 
	if(!fread(alias,sizeof(alias),1,f)
	  ||peek.x950412de!=0x950412de
	  ||1<(peek.revisions>>16))
	  return 0;

	headbytes = peek.msgidndex;
	fseek(f,0,SEEK_END);
	return ftell(f);
}

static size_t //Swordofmoonlight_maptofile_format
Swordofmoonlight_nul(FILE *f, int &mask, size_t &headbytes)
{	
	size_t fsize = 0; 	
	fseek(f,0,SEEK_END); fsize = ftell(f);
	fseek(f,0,SEEK_SET);
	return fsize;
}

static void Swordofmoonlight_maptofile_format
(size_t (*format)(FILE*, int &mask, size_t &headbytes)
,swordofmoonlight_lib_image_t &in, swordofmoonlight_lib_file_t filename, int mode, int mask)
{	
	if(!format) format = Swordofmoonlight_nul; 
	assert(mask);
	memset(&in,0x00,sizeof(swordofmoonlight_lib_image_t)); 
	in.bad = 1; 

	FILE *f = 0;	
	if(filename)
	{
		switch(mode)
		{
		case 'v': 
		case 'r': f = SWORDOFMOONLIGHT_FOPEN(filename,"rb"); break;
		case 'w': f = SWORDOFMOONLIGHT_FOPEN(filename,"rb+"); break;

		default: assert(mode=='r'||mode=='w'); return;
		}
	}
	else //alternative approach (already open file)
	{			
		bool v = mode<0; //hack: writecopy 

		//want to be able to access 
		//file and mmap simultaneously
		int fdup = dup(v?-mode:mode);
		if(fdup==-1) return;

		//PORTABLE?
		//Turns out this doesn't work on Windows, but it is possible
		//to use CreateFileMapping the same way, instead of fdopen.
		//TODO? O_RDONLY==(fcntl(fd,F_GETFL)|O_ACCMODE) if POSIX.
		if(!(f=fdopen(fdup,"rb+")))
		{
			f = fdopen(fdup,"rb"); mode = v?'v':'r';
		}
		else mode = v?'v':'w';
	}

	bool w = mode=='w', v = mode=='v'; if(!f) return; 
		
	//todo: rewrite in terms of unaligned_size and PSX words
	size_t headbytes = 0, fbytes = format(f,mask,headbytes);
	//
	const size_t unaligned_size = fbytes; //NEW
	//
	int headalign = headbytes%4?4-headbytes%4:0; //4: PSX word size
	//
	if(fbytes%4) fbytes+=4-(fbytes%4); headbytes+=headalign;
	//	
	if(fbytes==0||headbytes/4>swordofmoonlight_lib_image_t::headmax) 
	{
		fclose(f); return;
	}
	assert(headbytes%4+fbytes%4==0);

	int8_t *map = 0;

#ifdef _WIN32 

	HANDLE g = 0; DWORD access = 0;

	/*if(filename) //Why not?
	{
		fclose(f); access = GENERIC_READ|(w||v?GENERIC_WRITE:0);
	
		g = CreateFileW(filename,access,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
	}
	else*/ g = (HANDLE)_get_osfhandle(fileno(f));

	if(!g) return; access = w?PAGE_READWRITE:PAGE_READONLY;
		
	//unaligned_size: fbytes should work for SOM's native formats but not Zip etc
	HANDLE h = CreateFileMapping(g,0,v?PAGE_WRITECOPY:access,0,unaligned_size,0);

	if(!h&&!filename) //HACK: Compensate for fdopen's failure to do this logic :(
	{
		//PORTABLE? Tested on Windows 10.
		w = false; access = PAGE_READONLY;
		h = CreateFileMapping(g,0,access,0,unaligned_size,0);
	}

	if(h) //NOTE: fbytes CAN BE 1~3Bs LARGER THAN THE FILE (so use unaligned_size)
	{
		access = w?FILE_MAP_WRITE:FILE_MAP_READ; 

		//file offset can't be negative. so a realign in memory won't work
		map = (int8_t*)MapViewOfFile(h,v?FILE_MAP_COPY:access,0,0,unaligned_size);

		if(map) in.file = (void*)h; else CloseHandle(h);
	}

	//NOTE: the CRT sometimes gets stuck on _fcloseall assert("_osfile(fh) & FOPEN")
	//I don't think it's related to this. it's probably something about file sharing
	//fclose(f); //CloseHandle(g); 	
	if(0!=fclose(f)) assert(0);

#else //NOTE: fbytes CAN BE 1~3 BYTES LARGER THAN THE FILE (so use unaligned_size)

	int fd = fileno(f), prot = PROT_READ|(w?PROT_WRITE:0);

	//hard to tell if negative offset is allowed, but probably not always
	map = (int8_t*)mmap(0,unaligned_size,prot,v?MAP_PRIVATE:MAP_SHARED,fd,0);

	if(map) in.file = f; else fclose(f);

#endif

	if(map) //copied by Swordofmoonlight_mapheader_format
	{			
		in.real = headalign;
		in.head = headbytes/4;
		in.set = (int32_t*)(map+headbytes-headalign);			    
		in.end = in.set+fbytes/4-in.head;
		in.mode = v?'v':mode; in.mask = mask; 
		in.bad = 0;
		in.size = unaligned_size; //NEW
	}
}

//HACK! THIS WAS JUST AN AFTERTHOUGHT
static void Swordofmoonlight_mapheader_format
(size_t(*format)(FILE*,int&,size_t&), //void*
swordofmoonlight_lib_image_t &in, swordofmoonlight_lib_ram_t _ram, size_t bytes)
{
	memset(&in,0x00,sizeof(swordofmoonlight_lib_image_t)); in.bad = 1; 

	//NEW: Swordofmoonlight_mapheader_format is used by the namespaced 
	//maptoram/rom helpers that don't receive a mask with their inputs.
	in.mask = ~0; 

	size_t headbytes = 0; 
	swordofmoonlight_lib_rom_t headroom = _ram;	
	if(format==Swordofmoonlight_mdo&&bytes>=8)
	{
		//copied from Swordofmoonlight_mdo
		headbytes = 4;
		mdo::textures_t &peek =	*(mdo::textures_t*)headroom; 
		int n,i; for(i=0,n=peek.count;n&&headbytes<bytes;i++)
		{
			headbytes++; if(0==peek.refs[i]) n--;	
		}
		if(headbytes==bytes&&n||n) return;
		//align head with the start of materials 
		if(headbytes%4) headbytes+=4-headbytes%4;
	}
	else if(format==Swordofmoonlight_mdl&&bytes>=16) 
	{
		headbytes = 16; //copied from Swordofmoonlight_mdl

		mdl::header_t &peek = *(mdl::header_t*)headroom; 
		size_t fwords = Swordofmoonlight_mdl_mask(peek,in.mask,bytes);		
		
		if(fwords*4>bytes+4) return; //probably not a MDL file
	}
	else if(format==Swordofmoonlight_tim&&bytes>8) 
	{
		//copied from Swordofmoonlight_tim
		headbytes = 8;

		Swordofmoonlight_tim_peek *peek;
		(const void*&)peek = headroom; 

		size_t fwords = Swordofmoonlight_tim_mask(peek,in.mask,bytes);

		if(fwords*4>bytes+4) return; //probably not a TIM file
	}	
	else if(format==Swordofmoonlight_msm&&bytes>=3)
	{
		//copied from Swordofmoonlight_msm
		headbytes = 2; 
		msm::textures_t &peek =	*(msm::textures_t*)headroom; 
		int n,i; for(i=0,n=peek.count;n&&headbytes<bytes;i++)
		{
			headbytes++; if(0==peek.refs[i]) n--;	
		}
		if(headbytes==bytes&&n||n) return; 
	}
	else if(format==Swordofmoonlight_mhm&&bytes>=24) 
	{
		//copied from Swordofmoonlight_mhm
		headbytes = 24;
		mhm::header_t &peek = *(mhm::header_t*)headroom; 		
		
		size_t fwords = sizeof(peek)/4
		+peek.vertcount*3
		+peek.normcount*3
		+peek.facecount*sizeof(mhm::face_t)/4;

		if(fwords*4>bytes) return; //probably not a MHM file
	}	
	else if(format==Swordofmoonlight_evt&&bytes>=4) 
	{
		//copied from Swordofmoonlight_evt		
		evt::header_t &peek = *(evt::header_t*)headroom; 		

		headbytes = 0; //4+peek.count*252;

		if(peek.count!=1024 //?
		||4+peek.count*252>bytes) return; //probably not an EVT file
	}
	else assert(0); //unexpected   
	if(headbytes) //TODO? fill out mask etc.
	{
		int headalign = headbytes%4?4-headbytes%4:0;
		headbytes+=headalign;

		//copied from Swordofmoonlight_maptofile_format
		SWORDOFMOONLIGHT::_memory_::maptoram(in,_ram,bytes,in.mask);		
		in.real = headalign;
		in.head = headbytes/4;
		int shift = headbytes-headalign;
		(char*&)in.set+=shift; //NEW way...
		bytes-=shift; if(bytes%4) 
		{
			bytes+=4-bytes%4;
			in.end = in.set+bytes/4; //align end pointer
		}
		in.bad = 0;
	}
}
static inline void Swordofmoonlight_mapheader_format
(size_t(*format)(FILE*,int&,size_t&), swordofmoonlight_lib_image_t &in, const swordofmoonlight_lib_rom_t rom, size_t bytes)
{
	Swordofmoonlight_mapheader_format(format,in,(swordofmoonlight_lib_ram_t)rom,bytes);
	in.mode = SWORDOFMOONLIGHT_READONLY;
}

int swordofmoonlight_lib_unmap(swordofmoonlight_lib_image_t *in)
{
	if(!in) return 0;
	
	int out = in->bad; in->bad = 0; 

	if(in->file)
	{		
		#ifdef _WIN32
		//Assuming FlushViewOfFile implied
		UnmapViewOfFile(in->header<void>());
		CloseHandle((HANDLE)in->file);
		#else
		munmap(in->header<void>(),in->size);
		fclose((FILE*)in->file);
		#endif
	}
	#ifdef _DEBUG
	memset(in,0x00,sizeof(*in));
	#endif
	return out;
}

void mhm::maptofile(msm::image_t &in, msm::file_t filename, int mode)
{
	Swordofmoonlight_maptofile_format(Swordofmoonlight_mhm,in,filename,mode,~0);
}
void mhm::maptoram(mhm::image_t &in, mhm::ram_t ram, size_t bytes)
{
	Swordofmoonlight_mapheader_format(Swordofmoonlight_mhm,in,ram,bytes);
}
void mhm::maptorom(mhm::image_t &in, mhm::rom_t rom, size_t bytes)
{
	Swordofmoonlight_mapheader_format(Swordofmoonlight_mhm,in,rom,bytes);
}

int mhm::_imagememory(const mhm::image_t &in, const void* (&ptrs)[4])
{
	const mhm::header_t &hd = mhm::imageheader(in); if(!hd) return 0;

	mhm::vector_t *v = (mhm::vector_t*)(in+0);
	mhm::vector_t *n = v+hd.vertcount;
	mhm::face_t *f = (mhm::face_t*)(n+hd.normcount);
	mhm::index_t *i = (mhm::index_t*)(f+hd.facecount);

	int out = in.end-(int32_t*)i;

	if(out<0||in<=i+out) return 0;
	
	if(ptrs[0]) *(void**)ptrs[0] = v;
	if(ptrs[1]) *(void**)ptrs[1] = n;
	if(ptrs[2]) *(void**)ptrs[2] = f;
	if(ptrs[3]) *(void**)ptrs[3] = i; return out;
}

void msm::maptofile(msm::image_t &in, msm::file_t filename, int mode, int mask,...)
{
	Swordofmoonlight_maptofile_format(Swordofmoonlight_msm,in,filename,mode,mask&0x3);
}
void msm::maptoram(msm::image_t &in, msm::ram_t ram, size_t bytes)
{
	Swordofmoonlight_mapheader_format(Swordofmoonlight_msm,in,ram,bytes);
}
void msm::maptorom(msm::image_t &in, msm::rom_t rom, size_t bytes)
{
	Swordofmoonlight_mapheader_format(Swordofmoonlight_msm,in,rom,bytes);
}

msm::textures_t &msm::textures(msm::image_t &in)
{
	if(!in) return in.badref<msm::textures_t>();

	return *in.header<msm::textures_t>();
}

int msm::texturereferences(const msm::image_t &in, msm::reference_t *inout, int fill, int skip)
{		
	//REMINDER: same deal as mdo::texturereferences() down below

	if(!in) return 0; 

	const msm::textures_t &tex = msm::textures(in);

	int count = tex.count;
	int out = 0; msm::reference_t p = tex.refs;
	for(int i=0,j=skip;i<fill&&j<count;i++,j++,out++,p++)
	{
		inout[i] = p++; while(*p&&(int32_t*)p<in.set) p++; //paranoia
	}

	return out;
}

msm::vertices_t &msm::vertices(msm::image_t &in)
{
	msm::vertices_t &vrts = *(msm::vertices_t*)in.set;	
	if(in>=vrts.list&&in>=vrts.list+vrts.count) return vrts;
	else return in.badref<msm::vertices_t>();
}

msm::polygon_t *msm::firstpolygon(msm::image_t &in)
{	
	msm::vertices_t &vrts = msm::vertices(in);	
	return !vrts?0:(msm::polygon_t*)(vrts.list+vrts.count);
}

//2022: 0 is valid, but unfortunately requires walking
//the polygons to the end of the file in some cases
const msm::softvertex_t *msm::softvertex(const msm::image_t &img)
{
	size_t size = img.size; 
	const ule16_t *back16 = (const ule16_t*)(img.header<char>()+size);
	
	//old inline algorithm
	//return 0==back16[-2]&&back16[-1]!=0?back16-1:0;
	if(0==back16[-2]&&back16[-1]!=0) return back16-1;

	const msm::polygon_t *p = msm::terminator(img);

	//HACK: need to handle bug cases where the soft
	//vertex has been written more than once
	//return (void*)&p->subdivs==back16-2?back16-1:0;
	if(&p->subdivs<back16)
	{
		if(!p->subdivs) //0-terminator?
		{
			if(&p->subdivs<back16-1)
			{
				//this could help to clean up erroneous
				//files, assuming MSM is never extended
				if(&p->subdivs<back16-2)
				{
					return 0; //force reevaluation?
				}

				return &p->subdivs+1; //back16-1
			}
		}
		else assert(!p->subdivs);
	}

	return 0;
}
const msm::polygon_t *msm::terminator(const msm::image_t &img)
{
	size_t size = img.size; 
	const ule16_t *back16 = (const ule16_t*)(img.header<char>()+size);
	
	const msm::polygon_t *p = msm::firstpolygon(img);
	if(!p) return 0;

	#define MSM_SAFE_SHIFT \
	p+1<(void*)back16?(p=msm::shiftpolygon(p))->subdivs:0

	int i; for(i=p->subdivs;i-->0;)
	{
		do
		for(int j=MSM_SAFE_SHIFT;j-->0;)
		for(int k=MSM_SAFE_SHIFT;k-->0;)
		{
			MSM_SAFE_SHIFT;
		}
		while(i>0&&i--);
	}

	#undef MSM_SAFE_SHIFT

	return (void*)p<back16&&p->subdivs==0?p:0;
}
 
void mdo::maptofile(mdo::image_t &in, mdo::file_t filename, int mode, int mask,...)
{
	Swordofmoonlight_maptofile_format(Swordofmoonlight_mdo,in,filename,mode,mask&0x7);
}
void mdo::maptoram(mdo::image_t &in, mdo::ram_t ram, size_t bytes)
{
	Swordofmoonlight_mapheader_format(Swordofmoonlight_mdo,in,ram,bytes);
}
void mdo::maptorom(mdo::image_t &in, mdo::rom_t rom, size_t bytes)
{
	Swordofmoonlight_mapheader_format(Swordofmoonlight_mdo,in,rom,bytes);
}

mdo::textures_t &mdo::textures(mdo::image_t &in)
{
	if(!in) return in.badref<mdo::textures_t>(); 
	
	assert(!in.real); //MDO headers should be 32bit aligned
	
	if(!in.real) return *(mdo::textures_t*)(in.set-in.head);	
	
	return *in.header<mdo::textures_t>();
}

int mdo::texturereferences(const mdo::image_t &in, mdo::reference_t *inout, int fill, int skip)
{		
	//REMINDER: same deal as msm::texturereferences() up above

	if(!in) return 0;	 

	const mdo::textures_t &tex = mdo::textures(in);

	int count = tex?tex.count:0;
	int out = 0; mdo::reference_t p = tex.refs;
	for(int i=0,j=skip;i<fill&&j<count;i++,j++,out++,p++)
	{
		inout[i] = p++; while(*p&&(int32_t*)p<in.set) p++; //paranoia
	}
	return (le32_t*)p<in.set&&*p=='\0'?out:0; //NEW
}

mdo::materials_t &mdo::materials(mdo::image_t &in)
{
	mdo::materials_t &mats = *(mdo::materials_t*)in.set; 
	if(in>=mats.list&&in>=mats.list+mats.count) return mats;
	else return in.badref<mdo::materials_t>();
}

mdo::controlpoints_t &mdo::controlpoints(mdo::image_t &in)
{
	mdo::controlpoints_t *bad = 0; //C2101 (MSVC2005)

	if(!in) return *bad; //in.badref<mdo::controlpoints_t>();

	mdo::materials_t &mats = *(mdo::materials_t*)in.set;
	
	float *control_pts = mats?(float*)&mats[mats.count]:0;

	if(in<=control_pts+12) return *bad; //in.badref<mdo::controlpoints_t>();
		
	return *(mdo::controlpoints_t*)control_pts;
}

mdo::channels_t &mdo::channels(mdo::image_t &in)
{
	mdo::controlpoints_t &cp = mdo::controlpoints(in);	 
	mdo::channels_t &chans = *(mdo::channels_t*)(cp+4); 
	if(cp&&in>=chans.list&&in>=chans.list+chans.count) return chans;
	else return in.badref<mdo::channels_t>();
}

mdo::vertex_t *mdo::tnlvertexpackets(mdo::image_t &in, signed int ch)
{
	mdo::vertex_t *out = 0;
	mdo::channels_t &chans = mdo::channels(in); if(chans)
	{		  
		size_t base = chans[ch].vertblock;
		if(in.test(base%4,false)&&in>=out+chans[ch].vertcount)
		out = (mdo::vertex_t*)(in.header<char>()+base); 
	}
	return out;
}
	
mdo::triple_t *mdo::tnlvertextriples(mdo::image_t &in, signed int ch)
{
	mdo::triple_t *out = 0;
	mdo::channels_t &chans = mdo::channels(in); if(chans)
	{
		size_t base = chans[ch].ndexblock;
		if(in>=static_cast<ule16_t*>(*out)+chans[ch].ndexcount) 
		//2023
	//	if(in.test(base%4,false)&&in.test(chans[ch].ndexcount%3,false))		
		if(in.test(base%2,false)&&in.test(chans[ch].ndexcount%3,false))		
		out = (mdo::triple_t*)(in.header<char>()+base); 
	}
	return out;
}

void mdl::maptofile(mdl::image_t &in, mdl::file_t filename, int mode, int mask,...)
{
	Swordofmoonlight_maptofile_format(Swordofmoonlight_mdl,in,filename,mode,mask&0xFFFF);
}
void mdl::maptoram(mdl::image_t &in, mdl::ram_t ram, size_t bytes)
{
	Swordofmoonlight_mapheader_format(Swordofmoonlight_mdl,in,ram,bytes);
}
void mdl::maptorom(mdl::image_t &in, mdl::rom_t rom, size_t bytes)
{
	Swordofmoonlight_mapheader_format(Swordofmoonlight_mdl,in,rom,bytes);
}

mdl::header_t &mdl::imageheader(mdl::image_t &in)
{
	if(!in) return in.badref<mdl::header_t>();

	return psx::word_cast<mdl::header_t>(in.set-4);
}

mdl::primch_t &mdl::primitivechannel(mdl::image_t &in, int ch)
{
	if(in>ch*7+7) return psx::word_cast<mdl::primch_t>(in+ch*7);

	return in.badref<mdl::primch_t>();
}

mdl::primtable_t mdl::primitives(const mdl::image_t &img, int ch)
{
	mdl::primtable_t out = {};
	const mdl::primch_t &pc = mdl::primitivechannel(img,ch); 	
	if(!pc) return out;

	const psx::uword_t *p = &psx::word_cast<psx::uword_t>(img+pc.primsbase);
	if(img<p+1) return out;

	int i,hi; for(i=pc.primcount;i>0;i-=hi)
	{
		hi = p->hi;

		int x = 0; switch(p->lo)
		{
		case 0x00: x = 3; (const void*&)out.prims00 = p; break;
		case 0x01: x = 5; (const void*&)out.prims01 = p; break;
		case 0x03: x = 4; (const void*&)out.prims03 = p; break;
		case 0x04: x = 6; (const void*&)out.prims04 = p; break;
		case 0x06: x = 4; (const void*&)out.prims06 = p; break;
		case 0x07: x = 4; (const void*&)out.prims07 = p; break;
		case 0x0A: x = 5; (const void*&)out.prims0A = p; break;
		case 0x0D: x = 6; (const void*&)out.prims0D = p; break;
		case 0x11: x = 4; (const void*&)out.prims11 = p; break;

		//TODO: there is a chance this may be a new code.

		default: i = -1; continue;
		}
		p+=1+hi*x; if(img<p) break;
	}
	img.test(i==0); return out;
}

int mdl::countprimitives(const mdl::image_t &in, int ch, int *nout, int len, int firstprimcode, int accum)
{
	const mdl::primch_t &pc = mdl::primitivechannel(in,ch); if(!pc) return 0;

	const psx::uword_t *p = &psx::word_cast<psx::uword_t>(in+pc.primsbase);
	if(in<p+1) return 0;

	int out = firstprimcode+accum, hi, lo = 0; if(in<p) return 0;

	const int atleast = firstprimcode, atmost = firstprimcode+len-1;

	if(!accum&&nout&&len>0) memset(nout,0x00,len*sizeof(int));

	int i; for(i=pc.primcount;i>0;i-=hi)
	{	
		hi = p->hi; lo = p->lo;

		if(hi>i)
		{
			break; //breakpoint
		}
		int x = 0; switch(lo)
		{
		case 0x00: x = 3; break;
		case 0x01: x = 5; break;
		case 0x03: x = 4; break;
		case 0x04: x = 6; break;
		case 0x06: x = 4; break;
		case 0x07: x = 4; break;
		case 0x0A: x = 5; break;
		case 0x0D: x = 6; break;
		case 0x11: x = 4; break;

		//TODO: there is a chance this may be a new code.

		default: i = -1; continue;
		}
		p+=1+hi*x; if(in<p) break;

		if(nout&&lo>=atleast&&lo<=atmost)
		{
			nout[lo-firstprimcode]+=hi;

			out = max(lo+1,out);
		}		
	}
	in.test(i==0); return out-firstprimcode;
}

int mdl::countprimitivesinset(const mdl::image_t &in, int ch, int set)
{	
	int tmp[0x11+1], n = mdl::countprimitives(in,ch,tmp,0x11+1);	
	return mdl::primsum(set,tmp,n);
}

mdl::uvparts_t &mdl::partialprimitives(mdl::image_t &in)
{
	mdl::header_t &hd = mdl::imageheader(in);
	if(!hd) return in.badref<mdl::uvparts_t>();
	assert(1==hd.uvsblocks);
	int32_t *p = in+hd.primchans*7, out = in<p+1?0:psx::word_cast<psx::uword_t>(p).lo;	
	return psx::word_cast<mdl::uvparts_t>(out&&in>=p+1+out*3?p:0);	
}
mdl::uvparts_t &mdl::quadprimscomplement(mdl::image_t &in)
{
	//NEW: identical except for minor change
	mdl::header_t &hd = mdl::imageheader(in);	
	if(!hd) return in.badref<mdl::uvparts_t>();
	assert(1==hd.uvsblocks);
	int32_t *p = in+hd.primchans*7, out = in<p+1?0:psx::word_cast<psx::uword_t>(p).lo;	
	return psx::word_cast<mdl::uvparts_t>(/*out&&*/in>=p+1+out*3?p:0);	
}

mdl::vertex_t *mdl::pervertexlocation(mdl::image_t &in, int ch)
{
	mdl::primch_t &pc = mdl::primitivechannel(in,ch); if(!pc) return 0;
	mdl::vertex_t *p = &psx::word_cast<mdl::vertex_t>(in+pc.vertsbase);
	return in>p+pc.vertcount?p:0;
}

mdl::normal_t *mdl::pervertexlighting(mdl::image_t &in, int ch)
{
	mdl::primch_t &pc = mdl::primitivechannel(in,ch); if(!pc) return 0;
	mdl::normal_t *p = &psx::word_cast<mdl::normal_t>(in+pc.normsbase);
	return in>p+pc.normcount?p:0;
}

int mdl::triangulateprimitives(const mdl::image_t &in, int ch, int set, mdl::triangle_t *nout, int len, int restart)
{	
	const mdl::primch_t &pc = mdl::primitivechannel(in,ch); 	
	const mdl::uvparts_t &parts = partialprimitives(in,set); if(!pc) return 0;

	const psx::uword_t *p = &psx::word_cast<psx::uword_t>(in+pc.primsbase);

	int out = 0, hi = 0, lo = 0; 
	for(int i=pc.primcount;i>0;i-=hi)
	{
		int x = 0; if(in<p+1) return 0;

		hi = p->hi; lo = p->lo;

		//tris per code (this list may be incomplete)
		static const int N[] = {1,1,0,1,1,0,2,2,0,0,2,1,2};

		if(hi>i||lo>=(int)sizeof(N)||N[lo]==0) return(in.bad=1,0);		
					
		if(out+N[lo]>len) return out; //partials are not allowed
				
		if(N[lo]==2&&!parts //required
		&&(lo!=0x7&&lo!=0xA&&lo!=0x11)&&hi) return(in.bad=1,0);

		switch(lo)
		{
		case 0x00: x = 3; break;
		case 0x01: x = 5; break;
		case 0x03: x = 4; break;
		case 0x04: x = 6; break;
		case 0x06: x = 4; break;
		case 0x07: x = 4; break;
		case 0x0A: x = 5; break;
		case 0x0D: x = 6; break;
		case 0x11: x = 4; break;

		default: //should be unreachable
			
			return(in.bad=1,0);
		}

		if(!x||in<p+1+hi*x) return(in.bad=1,0);

		if(set<0?set&1<<lo:lo==set)
		{
			p++;

			int j; if(restart)
			{
				j = min(restart,hi);

				p+=x*j; restart-=j;
			}
			else j = 0; for(;j<hi;j++)
			{
				mdl::triangle_t &t = nout[out];
				mdl::triangle_t &t2 = nout[out+1];
								 
			//PLEASE: Do not make this stuff any more succint.

				//?????: Is there a pattern in the set bits??

				switch(lo)
				{		   
				//00000:    //3 words
				case 0x00: //flat colored triangle 

					t.rgb[0] = p[0].r;
					t.rgb[1] = p[0].g;
					t.rgb[2] = p[0].b;
					t.rgb[3] = p[0].c; t.tmd.bits = 0;

					t.lit[0] = t.lit[1] = t.lit[2] = p[1].lo;

					t.pos[0] = p[1].hi;
					t.pos[1] = p[2].lo; 
					t.pos[2] = p[2].hi;
			
					break;

				//00001:    //5 words
				case 0x01: //flat triangle
					
					t.uvs[0][0] = p[0].u;
					t.uvs[0][1] = p[0].v; //p[0].hi??
					t.uvs[1][0] = p[1].u;
					t.uvs[1][1] = p[1].v; t.tmd.tsb = p[1].hi;
					t.uvs[2][0] = p[2].u;
					t.uvs[2][1] = p[2].v; t.tmd.modeflags = p[2].hi;

					t.lit[0] = t.lit[1] = t.lit[2] = p[3].lo;

					t.pos[0] = p[3].hi;
					t.pos[1] = p[4].lo; 
					t.pos[2] = p[4].hi;
			
					break; 

				//00011:    //4 words
				case 0x03: //smooth colored triangle
					
					t.rgb[0] = p[0].r;
					t.rgb[1] = p[0].g;
					t.rgb[2] = p[0].b;
					t.rgb[3] = p[0].c; t.tmd.bits = 0;

					t.lit[0] = p[1].lo;
					t.pos[0] = p[1].hi;
					t.lit[1] = p[2].lo;
					t.pos[1] = p[2].hi; 
					t.lit[2] = p[3].lo;
					t.pos[2] = p[3].hi; 
			
					break;

				//00100:    //6 words
				case 0x04: //smooth shaded triangle
					
					t.uvs[0][0] = p[0].u;
					t.uvs[0][1] = p[0].v; //p[0].hi??

				//NOTE: 0001.MDL (SFX) has 0x7da0 in p[0].hi. It's possibly
				//junk... where is the drawing code? EneEdit (403A66) maybe?

					t.uvs[1][0] = p[1].u;
					t.uvs[1][1] = p[1].v; t.tmd.tsb = p[1].hi;
					t.uvs[2][0] = p[2].u;
					t.uvs[2][1] = p[2].v; t.tmd.modeflags = p[2].hi;

					t.lit[0] = p[3].lo;
					t.pos[0] = p[3].hi;
					t.lit[1] = p[4].lo;
					t.pos[1] = p[4].hi; 
					t.lit[2] = p[5].lo;
					t.pos[2] = p[5].hi;

					break;

				//00110:    //4 words
				case 0x06: //flat colored quadrangle
					
					t.rgb[0] = p[0].r;
					t.rgb[1] = p[0].g;
					t.rgb[2] = p[0].b;
					t.rgb[3] = p[0].c; t.tmd.bits = 0;

					t.lit[0] = t.lit[1] = t.lit[2] = p[1].lo;

					t.pos[0] = p[1].hi;
					t.pos[1] = p[2].lo; 
					t.pos[2] = p[2].hi;
			
					t2.pos[0] = t.pos[1];
					t2.pos[1] = p[3].lo; 
					t2.pos[2] = t.pos[2];
			
					break;

				//00111:    //4 words
				case 0x07: //flat shaded quadrangle (indexed UVs)
				{
					if(p[0].lo>=parts.count) return(in.bad=1,0);

					const mdl::uvpart_t &uvs = parts[p[0].lo];

					t.uvs[0][0] = uvs.u0;
					t.uvs[0][1] = uvs.v0; //p[0].hi??
					t.uvs[1][0] = uvs.u1;
					t.uvs[1][1] = uvs.v1; t.tmd.tsb = uvs.tsb;
					t.uvs[2][0] = uvs.u2;
					t.uvs[2][1] = uvs.v2; t.tmd.modeflags = p[0].hi;
					
					t.lit[0] = t.lit[1] = t.lit[2] = p[1].lo;

					t.pos[0] = p[1].hi;
					t.pos[1] = p[2].lo; 
					t.pos[2] = p[2].hi;
					
					t2.lit[0] = t2.lit[1] = t2.lit[2] = t.lit[0];

					t2.uvs[0][0] = uvs.u1;
					t2.uvs[0][1] = uvs.v1;
					t2.uvs[1][0] = uvs.u3;
					t2.uvs[1][1] = uvs.v3;
					t2.uvs[2][0] = uvs.u2;
					t2.uvs[2][1] = uvs.v2;
					
					t2.pos[0] = t.pos[1];
					t2.pos[1] = p[3].lo; 
					t2.pos[2] = t.pos[2];

					break;
				}
				//01010:    //5 words
				case 0x0A: //smooth shaded quadrangle (indexed UVs)
				{	
					if(p[0].lo>=parts.count) return(in.bad=1,0);
					
					const mdl::uvpart_t &uvs = parts[p[0].lo];

					t.uvs[0][0] = uvs.u0;
					t.uvs[0][1] = uvs.v0; 
					t.uvs[1][0] = uvs.u1;
					t.uvs[1][1] = uvs.v1; t.tmd.tsb = uvs.tsb;
					t.uvs[2][0] = uvs.u2; //0x003C vs 0x0034
					t.uvs[2][1] = uvs.v2; t.tmd.modeflags = p[0].hi;
					
					t.lit[0] = p[1].lo;
					t.pos[0] = p[1].hi;
					t.lit[1] = p[2].lo;
					t.pos[1] = p[2].hi; 
					t.lit[2] = p[3].lo;
					t.pos[2] = p[3].hi; 

					t2.uvs[0][0] = uvs.u1;
					t2.uvs[0][1] = uvs.v1; 
					t2.uvs[1][0] = uvs.u3;
					t2.uvs[1][1] = uvs.v3; 
					t2.uvs[2][0] = uvs.u2;
					t2.uvs[2][1] = uvs.v2; 
					
					t2.lit[0] = t.lit[1];
					t2.pos[0] = t.pos[1];
					t2.lit[1] = p[4].lo;
					t2.pos[1] = p[4].hi; 
					t2.lit[2] = t.lit[2];
					t2.pos[2] = t.pos[2]; 

					break;
				}
				//01101:    //6 words
				case 0x0D: //unlit triangle
					
					t.uvs[0][0] = p[0].u;
					t.uvs[0][1] = p[0].v; //p[0].hi??
					t.uvs[1][0] = p[1].u;
					t.uvs[1][1] = p[1].v; t.tmd.tsb = p[1].hi;
					t.uvs[2][0] = p[2].u;
					t.uvs[2][1] = p[2].v; t.tmd.modeflags = p[3].hi; //2

				//TODO: Figure out if SOM uses these colors for anything
					/*2021: RGB is decoded but 4435a0 ignores them*/

					/*2021: seems this is wrong
									   //could store in lit
					t.lit[0] = p[3].r; //t.rgb[0] = p[3].r;
					t.lit[1] = p[3].g; //t.rgb[1] = p[3].g;
					t.lit[2] = p[3].b; //t.rgb[2] = p[3].b;
									   //t.rgb[3] = p[3].c;
					*/
					t.lit[0] = ((uint8_t*)p)[10];
					t.lit[1] = ((uint8_t*)p)[11];
					t.lit[2] = ((uint8_t*)p)[12];

					t.pos[0] = p[4].lo;
					t.pos[1] = p[4].hi; 
					t.pos[2] = p[5].lo;

					break;

				//10001:    //4 words
				case 0x11: //unlit quadrangle (indexed UVs)
				{
					if(p[0].lo>=parts.count) return(in.bad=1,0);

					const mdl::uvpart_t &uvs = parts[p[0].lo];

					t.uvs[0][0] = uvs.u0;
					t.uvs[0][1] = uvs.v0; //p[0].hi??
					t.uvs[1][0] = uvs.u1;
					t.uvs[1][1] = uvs.v1; t.tmd.tsb = uvs.tsb;
					t.uvs[2][0] = uvs.u2;
					t.uvs[2][1] = uvs.v2; t.tmd.modeflags = p[1].hi; //0
					
				//TODO: Figure out if SOM uses these colors for anything
					/*2021: RGB is decoded but 4435a0 ignores them*/

					/*2021: seems this is wrong
												   //could store in lit
					t.lit[0] = t2.lit[0] = p[1].r; //t.rgb[0] = p[1].r;
					t.lit[1] = t2.lit[1] = p[1].g; //t.rgb[1] = p[1].g;
					t.lit[2] = t2.lit[2] = p[1].b; //t.rgb[2] = p[1].b;
												   //t.rgb[3] = p[1].c;
					*/
					t.lit[0] = ((uint8_t*)p)[2];
					t.lit[1] = ((uint8_t*)p)[3];
					t.lit[2] = ((uint8_t*)p)[4];

					t.pos[0] = p[2].lo;
					t.pos[1] = p[2].hi; 
					t.pos[2] = p[3].lo;
			
					t2.uvs[0][0] = uvs.u1;
					t2.uvs[0][1] = uvs.v1;
					t2.uvs[1][0] = uvs.u3;
					t2.uvs[1][1] = uvs.v3; 
					t2.uvs[2][0] = uvs.u2;
					t2.uvs[2][1] = uvs.v2; 
					
					t2.pos[0] = t.pos[1];
					t2.pos[1] = p[3].hi; 
					t2.pos[2] = t.pos[2]; 
					
					break;
				}
				default: //should be unreachable
					
					return(in.bad=1,0);
				}

				t.edges = 4-N[t.set=lo];

				if(t.edges==2) //quad
				{
					t2.tmd.bits = t.tmd.bits;

					t2.set = lo; t2.edges = 2; out++;
				}

				//early out
				if(++out==len) return out; 
				
				p+=x;
			}
		}
		else p+=1+hi*x;
	}

	return out;
}

int2 mdl::fillvertexbuffer
(mdl::vbuffer_t &v, int2 v_s, mdl::triangle_t *t, int t_s, int2 t_o, 
const mdl::vertex_t *pos, const mdl::normal_t *lit, bool uvs, int2 *inout)
{		
	if(!v||v_s<2) return v_s; //illegitimate 

	int2 vs = v[v_s-1].hash; //sizeofvertexbuffer

	if(vs>v_s-2) return v_s; //buffer seems to be broken??

	for(int i=0;i<t_s;i++) if(t[i].set!=-1) for(int j=0;j<3;j++)
	{
		if(vs==v_s-2) //time to grow the buffer
		{
			mdl::vbuffer_t swap; int swap_s = mdl::newvertexbuffer(swap,vs,1);

			if(!swap||swap_s<=v_s||mdl::sizeofvertexbuffer(swap,swap_s)!=vs)
			{
				mdl::deletevertexbuffer(swap); return v_s; //paranoia: should be unreachable
			}

			memcpy(swap,v,sizeof(*v)*vs); v_s = swap_s;

			mdl::deletevertexbuffer(v); v = swap; 
		}

		int m = t[i].pos[j], n = t[i].lit[j], uv[2] = {t[i].uvs[j][0],t[i].uvs[j][1]};
		
		int2 vm = t_o+m; mdl::v_t &cmp = v[vm].hash?v[vs]:v[vm];
		
		cmp.pos[0] = pos[m].x; cmp.pos[1] = pos[m].y; cmp.pos[2] = pos[m].z;

		switch(t[i].set)
		{
		case 1: case 4: case 7: case 0xA:

		if(lit){ cmp.lit[0] = lit[n].x; cmp.lit[1] = lit[n].y; cmp.lit[2] = lit[n].z; }

		case 0xD: case 0x11: if(uvs){ cmp.uvs[0] = uv[0]; cmp.uvs[1] = uv[1]; }
		}
		
		if(v[vm].hash)
		{
			while(v[vm]!=cmp)
			{
				if(v[vm].hash==-1)
				{
					vm = v[vm].hash = vs++; cmp.hash = -1; break;
				}
				else vm = v[vm].hash;

				if(vm<=0||vm>=v_s-2) return v_s; //corrupted buffer
			}
		}
		else v[vm].hash = -1;

		if(!inout)
		{
			 t[i].pos[j] = vm; //16bit: overwrite triangle positions			
		}
		else inout[i*3+j] = vm; //32bit: no overwrite
	}

	v[v_s-1].hash = vs; return v_s;
}

int mdl::animations(mdl::image_t &in, mdl::animation_t *nout, int len, int *IDs, int skip)
{
	mdl::header_t &hd = mdl::imageheader(in); if(!hd) return 0;

	size_t pcwords = hd.primchanwords, hawords = hd.hardanimwords;

	psx::uword_t *p = &psx::word_cast<psx::uword_t>(in+pcwords);

	int out = 0, i=0, n = hd.hardanims;

	if(skip<n&&n)
	{
		//if(in<p+1) return 0; else p++; //???? 
		//I guess this an extension mechanism. The code that reads it
		//just jumps to the very next bytes to position the read head
		//WILL NEED TO SEE IF ALL OF THE PROGRAMS RESPECT IT SOMETIME 
		//00445066 8D 74 8E FE          lea         esi,[esi+ecx*4-2]
		p+=p->hi; if(in<p) return 0;

		for(/*i=0*/;i<n&&out<len;i++)
		{			
			bool keep = IDs?false:true;

			mdl::animation_t q = reinterpret_cast<mdl::animation_t>(p);

			if(IDs&&!skip) for(int i=1;i<*IDs;i++) if(keep=IDs[i]==q->id) break;

			if(skip) skip--; else nout[out++] = keep?q:0;

			if(in<p+1) return 0; 

			int time = p->hi; p++;

			for(int j=0;j<time;j++)
			{
				if(in<p+1) return 0;

				int words = p->lo; 

				//Notes: basically there are unaligned blocks that are
				//guarded by an aligned header and trailer one word each.
				//The low half of the header is the size of the block in 
				//words, and the trailer mirrors the low half of the head.
				//Can't remember if the trailer could facilitate reverse
				//traversal or not. Or it could just be a sanity check.

				//int: signed/unsigned mismatch warning (may be wrong)
				if(in<=p+words||(int)p[words-1]!=words)
				{
					return(in.bad=1,0);
				}

				p+=words;
			}
		}
	}
	else skip-=n;

	if(!hd.softanims) return out;

	//there is a 4 word header at the top of the soft animation block
	uint16_t *d=0, *q = &psx::word_cast<uint16_t>(in+pcwords+hawords); 

	//unclear: q[0] here may be required to be equal to hd.softanims.
	//If so out should not exceed q[0]/hd.softanims. Examples are needed.

	//q[1] is the word offset to the next section of the block
	if(in>q+4) d = q+(2*q[1]); if(!d||in<d) return 0;

	//I don't know what the significance of this value
	//is. The game crashes without it. EneEdit/NpcEdit
	//become unstable/crash if the profile is unloaded.
	assert(((uint32_t*)q)[1]==0xFFFEFE00);

	//align q with the first mdl::animation_t
	for(q+=4,n+=hd.softanims;i<n&&out<len&&q<d;i++)
	{
		bool keep = IDs?false:true;

		mdl::animation_t p = reinterpret_cast<mdl::animation_t>(q);

		if(IDs&&!skip) for(int i=1;i<*IDs;i++) if(keep=IDs[i]==p->id) break;

		if(skip) skip--; else nout[out++] = keep?p:0;

		//align q with first mdl::softanimframe_t and increment
		//q until the 0-terminator is encountered and once more
		q++; while(q<d&&*q) q++; if(!*q) q++; else return 0;
	}

	return out;
}

int mdl::animationchannels(const mdl::image_t &in, bool soft)
{	
	const mdl::header_t &hd = mdl::imageheader(in); if(!hd) return 0;

	if(!soft) 
	{
		if(!hd.hardanims||!hd.hardanimwords) return 0;

		const psx::uword_t *p = &psx::word_cast<psx::uword_t>(in+hd.primchanwords);

		if(in>p+1) return min(p->lo,127); return 0;
	}

	if(!hd.softanims||!hd.softanimwords) return 0;

	size_t pcwords = hd.primchanwords, hawords = hd.hardanimwords;

	//there is a 4 word header at the top of the soft animation block
	const uint16_t *d=0, *q = &psx::word_cast<uint16_t>(in+pcwords+hawords); 

	if(in>q+4) d = q+(2*q[1]); if(!d||in<d+1) return 0;

	//assuming first channel starts after the last channel offset 

	q = d+d[0]*2; //advance q to the start of the first channel

	return in>=q?min(q-d,127):0; //number of channel offsets
}

int mdl::mapanimationchannels(const mdl::image_t &in, mdl::hardanim_t *out, int len)
{	
	const mdl::header_t &hd = mdl::imageheader(in); if(!hd) return 0;

	const psx::uword_t *p = &psx::word_cast<psx::uword_t>(in+hd.primchanwords);

	if(!out||hd.hardanims==0||in<p+2) return 0; 
	
	int channels = p->lo; 

	//if(len!=channels) return 0;
	if(len<channels) return 0;

	//p++;
	//I guess this an extension mechanism. The code that reads it
	//just jumps to the very next bytes to position the read head
	//WILL NEED TO SEE IF ALL OF THE PROGRAMS RESPECT IT SOMETIME 
	//00445066 8D 74 8E FE          lea         esi,[esi+ecx*4-2]
	p+=p->hi; if(in<p) return 0;

	int time = p->hi; if(time) p++; else return(in.bad=1,0);

	assert(p->hi==channels); //2020: using diffs below?

	if(p->hi!=channels||channels>=255) return(in.bad=1,0);

	int words = p->lo;	
		
	int diffs = p->hi; //2020?

	//WHAT IS THIS? (2020)
	//mdl::animations EXPLAINS THIS PATTERN
	//int: signed/unsigned mismatch warning (may be wrong)
	if(in>p+words&&(int)p[words-1]==words&&diffs<=words-2)
	{
		p++; 
	}
	else return(in.bad=1,0);
	
	const uint8_t *q = reinterpret_cast<const uint8_t*>(p), *cmp = q;

	for(int i=0;i<len;i++) out[i].map = 256; //sentinel...

	//2020: these are probably equal but technically diffs seems correct
	//for(int i=0;i<channels;i++)
	for(int i=0;i<diffs;i++)
	{
		int channel  = *q++;
		int diffmask = *q++;
		int bitsmask = *q++;
		int mapping  = *q++;

		out[channel].map = mapping==255?-1:mapping;

		//if(diffmask)	
		for(int j=0;j<6;j++) if(diffmask&1<<j)
		{
			if(~bitsmask&1<<j) q++; q++;
		}		
		if(diffmask&1<<6) q++; //scale x?
		if(diffmask&1<<7) q++; //scale y?
		if(bitsmask&1<<6) q++; //scale z?

		//enough granularity?
		//just one frame? what about the others?
		if(q-cmp>=words*4) return(in.bad=1,0);
	}

	for(int i=0;i<channels;i++) //len
	{
		if(out[i].map>channels) return(in.bad=1,0);
	}

	return channels;
}

int mdl::animate(mdl::const_animation_t in, mdl::hardanim_t *out, int clr, int now, bool first, int steps)
{		
	int16_t sign = steps>=0?1:-1; steps*=sign;

	if(!out||!in||now+steps<0||now+steps>in->time||!steps) return 0;  

	if(now==0) while(clr-->0) 
	{
		int swap = out[clr].map; 		
		memset(out+clr,0x00,sizeof(mdl::hardanim_t));
		memset(out[clr].cs,128,4);
		out[clr].map = swap;
	}

	const psx::uword_t *p = in->words;

	for(int i=0;i<in->time;i++)
	{			
		int words = p->lo;
		
		if(i==now)
		{
			int diffs = p->hi;
			
			const uint8_t *q = reinterpret_cast<const uint8_t*>(p+1);

			for(int j=0;j<diffs;j++)
			{
				int channel  = *q++;
				int diffmask = *q++; 
				int bitsmask = *q++; 

				if(i==0&&first) q++; //parent mapping?

				if(diffmask&0x07) 
				for(int k=0;k<3;k++) if(diffmask&1<<k)
				{
					if(~bitsmask&1<<k)
					{
						//assuming a mistake (untested?)
						//out[channel].cv[k]+=*(int16_t*)q*sign;
						out[channel].cv[k]+=*(int16_t*)q*sign; q+=2;
					}
					else out[channel].cv[k]+=*(int8_t*)(q++)*sign;	
				}	

				if(diffmask&0x38) 
				for(int k=3;k<6;k++) if(diffmask&1<<k)
				{
					if(~bitsmask&1<<k)
					{
						//assuming a mistake (untested?)
						//out[channel].ct[k-3]+=*(uint16_t*)q*sign;
						out[channel].ct[k-3]+=*(int16_t*)q*sign; q+=2;
					}
					else out[channel].ct[k-3]+=*(int8_t*)(q++)*sign;				
				}	

				//SFX?
				//REMINDER: 445120 seems to include some
				//scaling capability by 3 bytes that can
				//range from 0 to 1.9921875 and override
				//rather than accumulate. the top 2 bits
				//in diffmask mark two of the dimensions
				//the 3rd is marked by bit 7 in bitsmask
				if(diffmask&1<<6) //scale x?
				{
					out[channel].cs[0] = *q++;
				}
				if(diffmask&1<<7) //scale y?
				{
					out[channel].cs[1] = *q++;
				}
				if(bitsmask&1<<6) //scale z?
				{
					out[channel].cs[2] = *q++;

					diffmask|=1<<8; //...
				}
				assert(~bitsmask&0x80);		

				//2020: clr must set to 0 if |= is undesired
				//out[channel].dvt = diffmask;
				out[channel].dvt|=diffmask;
			}

			if(--steps==0) return now+sign;

			now+=sign;
		}
		
		p+=words;
	}

	return 0;
}

//For use by Transform only (they make assumptions about inputs and outputs)
static void Multiply(const float *mL, const float *mR, float *mout, bool homo);
static void Eulerxform3x3(const float r3[4], float mout[4*4]);

//bool mdl::transform(const mdl::hardanim_t *n, int ch, float *vinout, const float scale[4], float *nmats, bool *nknown)
static bool Transform(const mdl::hardanim_t *n, int ch, float *nmats, bool *nknown, const float *scl, float *rad, bool homo)
{
	if(ch<0||ch>=254) return false; //paranoia

	auto &chn = n[ch];

	//NEW: prevent stack overflow (for better or worse?)
	if(nknown)
	{
		if(!nknown[ch]) nknown[ch] = true; else return true; 
	}
	assert(ch!=chn.map);

	if(chn.map!=-1) //root
	{
		if(chn.map<0||chn.map>254) return false; //paranoia

		if(!nknown||!nknown[chn.map])
		{
			if(!Transform(n,chn.map,nmats,nknown,scl,rad,homo)) return false;
		}		
	}

	//if(!nknown||!nknown[ch])
	{
		float *m = nmats+ch*16;

		float rot[3] = { chn.cv[0]*rad[0], chn.cv[1]*rad[1], chn.cv[2]*rad[2] }; 

		//2021: optimizing this
		//Eulerxform(rot,pos,m,homo);
		Eulerxform3x3(rot,m); if(homo) //what was I thinking?
		{
			//2021: I guess "homo" is just for rotation, so scaling would just complicate
			//it??? I think this should be removed but I don't want to think about it now
			if(*(uint32_t*)chn.cs!=0x80808080)
			{
				auto &m44 = *(float(*)[4][4])m;

				float s3[3] = {chn.cs[0]*0.0078125f,chn.cs[1]*0.0078125f,chn.cs[2]*0.0078125f};

				for(int i=3;i-->0;) 
				for(int j=3;j-->0;) m44[j][i]*=s3[i];
			}
			
			m[12] = chn.ct[0]*scl[0]; m[13] = chn.ct[1]*scl[1]; m[14] = chn.ct[2]*scl[2];
		}
		else m[12] = m[13] = m[14] = 0;

		m[3] = m[7] = m[11] = 0; m[15] = 1; //identity

		if(chn.map!=-1) Multiply(nmats+chn.map*16,m,m,homo);

	//	if(nknown) nknown[ch] = true;
	}

	return true;
}

bool mdl::transform(const mdl::hardanim_t *n, int ch, float *vinout, const float scl[4], float *nmats, bool *nknown)
{
	static float *xmats = 0; 

	if(!nmats) //not thread safe
	{
		if(ch>127) return false; 

		if(!xmats) xmats = new float[128*16]; 

		nmats = xmats; assert(!nknown);

		if(nknown) return false;
	}

	if(!scl) scl = mdl::mm3; bool homo = scl[3];

	//C4838 (narrowing) //2021
	//const double pi = 3.141592653589793238462643383279, f2rads = pi/2048.0; 		
	const float pi = 3.141592653589793238462643383279f, f2rads = pi/2048.0f;

	float rad[3] = {scl[0]>0?f2rads:-f2rads,scl[1]>0?f2rads:-f2rads,scl[2]>0?f2rads:-f2rads};

	if(!Transform(n,ch,nmats,nknown,scl,rad,homo)) return false;

	if(vinout) mdl::multiply(nmats+ch*16,vinout,vinout,homo,3,1);

	return true;
}

static void Multiply(const float *m1, const float *m2, float *mout, bool homo)
{
	//Assuming 3rd row/column contains identity values!

	float 
	a00 = m1[0], a01 = m1[1], a02 = m1[2], //a03 = m1[3],
    a10 = m1[4], a11 = m1[5], a12 = m1[6], //a13 = m1[7],
    a20 = m1[8], a21 = m1[9], a22 = m1[10], //a23 = m1[11],
    
    b00 = m2[0], b01 = m2[1], b02 = m2[2], //b03 = m2[3],
    b10 = m2[4], b11 = m2[5], b12 = m2[6], //b13 = m2[7],
    b20 = m2[8], b21 = m2[9], b22 = m2[10]; //b23 = m2[11],

    mout[0] = b00 * a00 + b01 * a10 + b02 * a20; // + b03 * a30;
    mout[1] = b00 * a01 + b01 * a11 + b02 * a21; // + b03 * a31;
    mout[2] = b00 * a02 + b01 * a12 + b02 * a22; // + b03 * a32;
//  mout[3] = b00 * a03 + b01 * a13 + b02 * a23 + b03 * a33;
    mout[4] = b10 * a00 + b11 * a10 + b12 * a20; // + b13 * a30;
    mout[5] = b10 * a01 + b11 * a11 + b12 * a21; // + b13 * a31;
    mout[6] = b10 * a02 + b11 * a12 + b12 * a22; // + b13 * a32;
//  mout[7] = b10 * a03 + b11 * a13 + b12 * a23 + b13 * a33;
    mout[8] = b20 * a00 + b21 * a10 + b22 * a20; // + b23 * a30;
    mout[9] = b20 * a01 + b21 * a11 + b22 * a21; // + b23 * a31;
    mout[10] = b20 * a02 + b21 * a12 + b22 * a22; // + b23 * a32;
//  mout[11] = b20 * a03 + b21 * a13 + b22 * a23;  + b23 * a33;

	if(!homo) return;

	float
	a30 = m1[12], a31 = m1[13], a32 = m1[14], //a33 = m1[15],
    b30 = m2[12], b31 = m2[13], b32 = m2[14]; //b33 = m2[15];

    mout[12] = b30 * a00 + b31 * a10 + b32 * a20 + /*b33 **/ a30;
    mout[13] = b30 * a01 + b31 * a11 + b32 * a21 + /*b33 **/ a31;
    mout[14] = b30 * a02 + b31 * a12 + b32 * a22 + /*b33 **/ a32;
//  mout[15] = b30 * a03 + b31 * a13 + b32 * a23; //+ b33 * a33;
}

//static void Eulerxform(const float *euler, const float *xyz, float *mout, bool homo)
static void Eulerxform3x3(const float euler[3], float mout[4*4])
{		
	//2021: Somvector::rotation('xyz')
	float cy = cosf(euler[0]);
	float sy = sinf(euler[0]);
	float cp = cosf(euler[1]);
	float sp = sinf(euler[1]);
	float cr = cosf(euler[2]);
	float sr = sinf(euler[2]);
	float sysp = sy*sp; //-sy*-sp
	float _cysp = cy*-sp;
	auto &m44 = *(float(*)[4][4])mout;		
	m44[0][0] = cp*cr;
	m44[0][1] = sysp*cr+cy*sr;
	m44[0][2] = _cysp*cr+sy*sr;
	m44[1][0] = cp*-sr;
	m44[1][1] = sysp*-sr+cy*cr;
	m44[1][2] = _cysp*-sr+sy*cr;
	m44[2][0] = sp;
	m44[2][1] = -sy*cp;
	m44[2][2] = cy*cp;
}

void mdl::inverse(float *dst, const float *src, int m)
{
	for(int i=0;i<m;i++,dst+=16,src+=16)
	{
		float 
		a00 = src[ 0], a01 = src[ 1], a02 = src[ 2], a03 = src[ 3],
        a10 = src[ 4], a11 = src[ 5], a12 = src[ 6], a13 = src[ 7],
        a20 = src[ 8], a21 = src[ 9], a22 = src[10], a23 = src[11],
        a30 = src[12], a31 = src[13], a32 = src[14], a33 = src[15],

        b00 = a00 * a11 - a01 * a10,
        b01 = a00 * a12 - a02 * a10,
        b02 = a00 * a13 - a03 * a10,
        b03 = a01 * a12 - a02 * a11,
        b04 = a01 * a13 - a03 * a11,
        b05 = a02 * a13 - a03 * a12,
        b06 = a20 * a31 - a21 * a30,
        b07 = a20 * a32 - a22 * a30,
        b08 = a20 * a33 - a23 * a30,
        b09 = a21 * a32 - a22 * a31,
        b10 = a21 * a33 - a23 * a31,
        b11 = a22 * a33 - a23 * a32;

		float det = 1.0f/(b00*b11-b01*b10+b02*b09+b03*b08-b04*b07+b05*b06);

		dst[ 0] = (+a11*b11 - a12*b10 + a13*b09)*det;
		dst[ 1] = (-a01*b11 + a02*b10 - a03*b09)*det;
		dst[ 2] = (+a31*b05 - a32*b04 + a33*b03)*det;
		dst[ 3] = (-a21*b05 + a22*b04 - a23*b03)*det;
		dst[ 4] = (-a10*b11 + a12*b08 - a13*b07)*det;
		dst[ 5] = (+a00*b11 - a02*b08 + a03*b07)*det;
		dst[ 6] = (-a30*b05 + a32*b02 - a33*b01)*det;
		dst[ 7] = (+a20*b05 - a22*b02 + a23*b01)*det;
		dst[ 8] = (+a10*b10 - a11*b08 + a13*b06)*det;
		dst[ 9] = (-a00*b10 + a01*b08 - a03*b06)*det;
		dst[10] = (+a30*b04 - a31*b02 + a33*b00)*det;
		dst[11] = (-a20*b04 + a21*b02 - a23*b00)*det;
		dst[12] = (-a10*b09 + a11*b07 - a12*b06)*det;
		dst[13] = (+a00*b09 - a01*b07 + a02*b06)*det;
		dst[14] = (-a30*b03 + a31*b01 - a32*b00)*det;
		dst[15] = (+a20*b03 - a21*b01 + a22*b00)*det;
	}
}

void mdl::product(float *result, const float *left, const float *right, int m)
{
	for(int i=0;i<m;i++,result+=16,left+=16,right+=16)
	{
		float
		a00 = left[0], a01 = left[1], a02 = left[2], a03 = left[3],
		a10 = left[4], a11 = left[5], a12 = left[6], a13 = left[7],
		a20 = left[8], a21 = left[9], a22 = left[10], a23 = left[11],
		a30 = left[12], a31 = left[13], a32 = left[14], a33 = left[15],

		b00 = right[0], b01 = right[1], b02 = right[2], b03 = right[3],
		b10 = right[4], b11 = right[5], b12 = right[6], b13 = right[7],
		b20 = right[8], b21 = right[9], b22 = right[10], b23 = right[11],
		b30 = right[12], b31 = right[13], b32 = right[14], b33 = right[15];

		result[0] = b00 * a00 + b01 * a10 + b02 * a20 + b03 * a30;
		result[1] = b00 * a01 + b01 * a11 + b02 * a21 + b03 * a31;
		result[2] = b00 * a02 + b01 * a12 + b02 * a22 + b03 * a32;
		result[3] = b00 * a03 + b01 * a13 + b02 * a23 + b03 * a33;
		result[4] = b10 * a00 + b11 * a10 + b12 * a20 + b13 * a30;
		result[5] = b10 * a01 + b11 * a11 + b12 * a21 + b13 * a31;
		result[6] = b10 * a02 + b11 * a12 + b12 * a22 + b13 * a32;
		result[7] = b10 * a03 + b11 * a13 + b12 * a23 + b13 * a33;
		result[8] = b20 * a00 + b21 * a10 + b22 * a20 + b23 * a30;
		result[9] = b20 * a01 + b21 * a11 + b22 * a21 + b23 * a31;
		result[10] = b20 * a02 + b21 * a12 + b22 * a22 + b23 * a32;
		result[11] = b20 * a03 + b21 * a13 + b22 * a23 + b23 * a33;
		result[12] = b30 * a00 + b31 * a10 + b32 * a20 + b33 * a30;
		result[13] = b30 * a01 + b31 * a11 + b32 * a21 + b33 * a31;
		result[14] = b30 * a02 + b31 * a12 + b32 * a22 + b33 * a32;
		result[15] = b30 * a03 + b31 * a13 + b32 * a23 + b33 * a33;
	}
}

/*REFERENCE (scale is so rare I'm not sure it's worth decomposing)
//Probably a dead end precision/applicability wise (not thoroughly tested)
//void mdl::decompose(mdl::hardanim_t *out, const float *mat, const float scale[4], int n)
void mdl_decompose(mdl::hardanim_t *out, const float *mat, const float scale[4], int n)
{
	//2020: doesn't round away
	//const float round = 0.5f; //0.5f

	int sign[3] = {scale[0]>0?1:-1,scale[1]>0?1:-1,scale[2]>0?1:-1};

	const double pi = 3.141592653589793238462643383279, f2rads = 1.0/(pi/2048.0); 

	float scl[3] = {float(sign[0])/scale[0],float(sign[1])/scale[1],float(sign[2])/scale[2]};

	for(int i=0;i<n;i++,mat+=16,out++)
	{
		//UNFINISHED: IMPLEMENT NEW mdl::hardanim_t::cs SCALE FACTOR?

		if(mat[1]>0.998) //singularity at north pole
		{ 
			out->cv[0] = 0;
			out->cv[1] = mdl::round(f2rads*atan2f(mat[8],mat[10]));
			out->cv[2] = mdl::round(f2rads*pi*0.5f);
		}
		if(mat[1]<-0.998) //singularity at south pole
		{
			out->cv[0] = 0;			
			out->cv[1] = mdl::round(f2rads*atan2f(mat[8],mat[10]));
			out->cv[2] = mdl::round(f2rads*-pi*0.5f);
		}
		else
		{
			out->cv[0] = mdl::round(f2rads*atan2f(-mat[9],mat[5]));
			out->cv[1] = mdl::round(f2rads*atan2f(-mat[2],mat[0]));
			out->cv[2] = mdl::round(f2rads*asinf(mat[1]));			
		}

		out->cv[0]*=sign[0]; out->cv[1]*=sign[1]; out->cv[2]*=sign[2];

		out->ct[0] = mdl::round(mat[12]*scl[0])*sign[0]; 
		out->ct[1] = mdl::round(mat[13]*scl[1])*sign[1]; 
		out->ct[2] = mdl::round(mat[14]*scl[2])*sign[2]; 
	}
}*/

int mdl::softanimframes(const mdl::image_t &in, const int *IDs, int *unique, int len)
{
	const mdl::header_t &hd = mdl::imageheader(in); 
	
	if(!hd||!hd.softanimwords||!hd.softanimwords) return 0;

	size_t pcwords = hd.primchanwords, hawords = hd.hardanimwords;

	//there is a 4 word header at the top of the soft animation block
	//
	//  2021: SEE softanimchannels FOR NOTES ON THIS HEADER
	//
	const uint16_t *d=0, *q = &psx::word_cast<uint16_t>(in+pcwords+hawords); 

	if(in>q+4) d = q+(2*q[1]); if(!d||in<d) return 0;

	int out = 0; 
	for(const uint16_t *p=q+4;p<d&&out<255;p++) //255: paranoia
	{
		bool count = IDs?false:true;

		if(IDs) for(int i=1;i<*IDs;i++) if(count=IDs[i]==*p) break;

		while(++p<d) if(*p){ if(count) out++; }else break;
	}

	if(!unique) return out; 

  //// The rest is an algorithm for detecting/indexing unique frames ////

	int chans = mdl::animationchannels<mdl::softanim_t>(in);

	if(!chans||!out||len!=out) return 0; //must match exactly

	int total = out; out = 0;
	int *accum = new int[total*chans+chans];

	memset(accum,0x00,sizeof(int)*total*chans+chans);

	int frame = 0, top = 0; 	
	for(const uint16_t *p=q+4;p<d;p++)
	{
		bool count = IDs?false:true;

		if(IDs) for(int i=1;i<*IDs;i++) if(count=IDs[i]==*p) break;

		while(++p<d) if(*p)
		{ 
			if(!count||frame>=total) continue; //paranoia

			//TODO: optimize by looking ahead and applying deltas forward

			if(top!=frame) //hack: carry over from previous frame
			memcpy(accum+frame*chans,accum+frame*chans-chans,sizeof(int)*chans);
			
			int ch = mdl::softanimframechannel((const mdl::softanimframe_t*)p);
			int delta = mdl::softanimframedelta((const mdl::softanimframe_t*)p);

			accum[frame*chans+ch]+=delta; frame++;			
		
		}else break;

		top = frame;
	}

	//sanity check
	if(frame==total) for(int i=0;i<total;i++)
	{
		//back buffer is all zeroes (ie. the initial frame
		if(memcmp(accum+i*chans,accum+total*chans,sizeof(int)*chans))
		{ 
			bool fresh = true;
			for(int j=i-1;j>=0;j--) if(unique[j]!=-1) //minor optimization
			{
				if(!memcmp(accum+i*chans,accum+j*chans,sizeof(int)*chans))
				{
					unique[i] = unique[j]; fresh = false; break; //then it's not unique
				}
			}

			if(fresh) unique[i] = out++;
		}
		else unique[i] = -1; //an initial frame
	}

	//initial frames are marked by the highest unique frame+1
	for(int i=0;i<total;i++) if(unique[i]==-1) unique[i] = out;

	delete[] accum; return out;
}

mdl::softanim_t *mdl::softanimchannels
(const mdl::image_t &in, const mdl::softanimframe_t *deltas, mdl::softanim_t *out, int n, const mdl::softanim_t *defaults)
{		  
	if(!out) return 0; 
	
	if(defaults) for(int i=0;i<n;i++)
	{
		out[i] = *defaults;
	}
	else memset(out,0x00,sizeof(mdl::softanim_t)*n);

	const mdl::header_t &hd = mdl::imageheader(in); if(!hd) return out;

	if(!hd.softanims||!hd.softanimwords) return out;

	size_t pcwords = hd.primchanwords, hawords = hd.hardanimwords;

	//there is a 4 word header at the top of the soft animation block
	//
	// 2021: from x2mdl the first 2B is a "diff" count, the next is
	// a relative pointer. then there is an FFFEFE00 sequence which
	// I think is variable. FF is the end, FE is padding, and 00 is
	// important. I think all existing MDL files have this sequence
	// but 444353 in som_db.exe loops over it suggesting the format
	// is more expressive than what's known
	//
	const uint16_t *d=0, *q = &psx::word_cast<uint16_t>(in+pcwords+hawords); 

	if(in>q+4) d = q+(2*q[1]); if(!d||in<d+1) return out;

	//assuming first channel starts after the last channel offset 

	const uint16_t *p = d+d[0]*2, *b=0; //p: start of the first channel

	int chans = in>=p?min(p-d,127):0; //number of channel offsets

	if(0==d[chans-1]){ chans--; assert(chans!=0); } //padding

	size_t sahalfs = hd.softanimwords*2;
	
	for(int i=0;i<n;i++) 
	{
		int ch = mdl::softanimframechannel(deltas+i);
		int delta = mdl::softanimframedelta(deltas+i);

		if(ch<chans)
		{
			p = d+d[ch]*2;
			b = ch==chans-1?q+sahalfs:d+d[ch+1]*2;

			if(b>p&&in>p&&in>=b) 
			{
				out[i].firststep = (mdl::softanimcodec_t*)p;
				out[i].overstep = (mdl::softanimcodec_t*)b;								
				out[i].delta = delta;
			}			
		}
	}

	return out;
}

int2 mdl::copyvertexbuffer(mdl::vbuffer_t &dst, mdl::const_vbuffer_t src, int2 m, mdl::softanim_t *ch, int n, int sign)
{		
	int2 szsrc = mdl::sizeofvertexbuffer(src,m);
	int2 szdst = mdl::sizeofvertexbuffer(dst,m);
		
	bool writing = szdst||szsrc||dst&&m;

	if(writing)
	{
		if(m<=0) return 0;

		if(dst&&src) 
		{
			for(int2 i=0,iN=min(szdst,szsrc);i<iN;i++) 
			{
				~dst[i] = ~src[i]; //YUCK
			}
		}
		else if(!dst&&src&&szsrc)
		{
			memcpy(dst=new mdl::v_t[m],src,sizeof(*dst)*m);
		}
		
		if(!dst||!ch||!n) return dst?m:0;

		if(szdst<szsrc) szdst = szsrc; 
	}
	else 
	{
		if(!ch||!n) return 0;

		szdst = m-1; //2020: stops loop below (translate)
	}

	for(int i=0;i<n;i++) if(ch[i])
	{
		//INCOMPLETE
		//
		// there's a system here that iterates
		// over the multiple primitive channels
		//

		mdl::softanim_t &t = ch[i];

		int16_t delta = sign==0?(ch->delta>=0?1:-1):sign;

		bool write = writing&&!t.nowrite, dup = !t.nodup;

		//FIX ME
		// 
		// som_db calculates this from the vertex
		// count on the primitive channel divided
		// by 8 plus some rounding logic
		//
		//assuming this will always do the job
		//NOTE: 4 is 3 in front, and 1 in back (crazy)
		int2 bytes = (t.firststep->diffdataoffset-4)/3;		
				
		//separating these seems hard on the cache 
		//I can't remember if it makes the algorithm
		//easier to implement. I mean why even have 3?		
		const uint8_t *x = 
		t.firststep->diffmask, *y = x+bytes, *z = y+bytes;

		const mdl::softanimdatum_t *verts, *start = 
		t.firststep->diffdata+t.firststep->diffdataoffset;
			
		if(t.dec!=(unsigned)t.vrestart)
		{
			t.dec = t.vec = 0;

			if(t.vrestart<0||t.vrestart>=szdst&&write) 
			continue;

			//RECURSIVE
			//reset the vertex pointer to vrestart?			
			auto swap = t.vrestart; 
			auto swap2 = t.vwindow;
			t.vwindow = swap;			
			t.vrestart = 0;			
			mdl::vbuffer_t _ = 0;
			mdl::copyvertexbuffer(_,0,swap+1,ch,1,1);
			t.vrestart = swap; 
			t.vwindow = swap2;
			if(t.dec!=swap) //???
			{
				assert(t.dec==swap);
				continue;
			}
		}		
		verts = start+t.vec; //2020
		
		int2 bits = bytes*8;

		if(t.vwindow) bits = min(bits,t.vwindow);
				
		int2 v = t.vrestart, w = v;	   
		int2 B = t.dec/8,b = t.dec%8; //Bytes+bits
		
		bool wr = false, mod = false;		
	
		//TODO: might want to implement an alternative
		//version of this loop that minimizes branching
		//THIS IS A REFERENCE IMPLEMENTATION
		for(int2 i=t.dec;i<bits&&v<szdst;i++)
		{				
			uint8_t mask = 1<<b;
			if(x[B]&mask) 
			{
				if(wr=write)
				dst[v].pos[0]+=delta*verts->magnitude();
				verts+=verts->footprint();
				mod = true;
			}
			if(y[B]&mask) 
			{
				if(wr=write)
				dst[v].pos[1]+=delta*verts->magnitude();
				verts+=verts->footprint();
				mod = true;
			}
			if(z[B]&mask) 
			{
				if(wr=write)
				dst[v].pos[2]+=delta*verts->magnitude();
				verts+=verts->footprint();
				mod = true;
			}

			if(b==7){ b = 0; B++; }else b++;
	
			if(wr&&dup)
			for(w=dst[v].hash;w>0&&w<m;w=dst[w].hash)
			{
				dst[w].pos[0] = dst[v].pos[0];
				dst[w].pos[1] = dst[v].pos[1];
				dst[w].pos[2] = dst[v].pos[2];
			}
                         
			v++; wr = false;
		}

		t.vrestart = t.vwindow?v:0;
		t.changed = mod?1:0;

		t.vec = verts-start;
		t.dec = bits;		
	}

	return m;
}
static int Swordofmoonlight_translate_compare(int32_t *a, int32_t *b)
{
	int aa = *a&0xFF, bb = *b&0xFF; //being big endian friendly
	if(aa==bb) return 0; //not expecting but don't want problems
	return aa<bb?-1:1;
}
int mdl::translate(mdl::softanim_t *ch, int2 m, float vinout[3], const float scale[4], int2 *mverts, int n, int sign)
{
	bool unsort = m<0;
	bool sorted = false; if(mverts)
	{	
		if(unsort) m = -m;

		int cmp = -1;

		for(int i=0;i<m;i++) 
		{	
			int ffff = mverts[i]&0xffff; 
			
			if(ffff!=mverts[i])
			{
				//if the high bits are set then assume already
				//sorted by an earlier call to mdl::translate
				goto sorted;
			}
			else if(cmp>ffff)
			{
				if(ch||!unsort)
				{
					for(int i=m;i-->0;)
					{
						//being big endian friendly
						ffff = mverts[i]&0xffff; mverts[i] = i<<16|ffff;
					}
					::qsort(mverts,m,sizeof(*mverts),
					(int(*)(const void*,const void*))Swordofmoonlight_translate_compare);
				}
				sorted: sorted = true; break;
			}
			cmp = ffff;
		}
	}
	int i = 0; if(!ch) ret: //0 is used to just sort/unsort
	{
		if(unsort) if(sorted)
		{
			//reverse sort to avoid managing memory I guess
			for(int i=m;i-->0;)
			{
				int a = mverts[i]&0xffff; //being big endian friendly
				int b = (unsigned)mverts[i]>>16; mverts[i] = a<<16|b;
			}
			::qsort(mverts,m,sizeof(*mverts),
			(int(*)(const void*,const void*))Swordofmoonlight_translate_compare);
			for(int i=m;i-->0;) 
			{
				mverts[i] = (unsigned)mverts[i]>>16;
			}
		}	

		return i;
	}

	mdl::v_t vb[2] = {}; vb[1].hash = m+1;

	//copyvertexbuffer wants vwindow to be nonzero
	//to use vrestart
	int hack = ch->vwindow;

	int cmp = -1; 

	int v; if(!mverts)
	{
		v = m; m = 0; goto one_off;
	}
	for(;i<m;i++)
	{
		v = mverts[i]&0xffff; one_off:

		if(cmp!=v)
		{
			//HACK! copyvertexbuffer WAS NEVER DESIGNED TO 
			//WORK LIKE THIS
			mdl::vbuffer_t vp = vb-v, _ = 0;
			vb[1].hash = v+1;
			ch->vwindow = v;
			if(cmp<v-1)
			if(!mdl::copyvertexbuffer(_,0,v+1,ch,n,sign))
			break;
			cmp = v;
			ch->vwindow = v+1;
			memset(vb->pos,0x00,sizeof(vb->pos));			
			if(!mdl::copyvertexbuffer(vp,0,v+2,ch,n,sign))		
			break;
		}

		float *f = vinout+3*(sorted?mverts[i]>>16:i);

		for(int j=3;j-->0;) f[j]+=vb->pos[j]*scale[j];
	}

	ch->vwindow = hack; goto ret; //return i;
}

void mdl::maptotimblock(tim::image_t &out, mdl::image_t &in, int tim)
{		
	memset(&out,0x00,sizeof(out)); out.bad = 1;

	if(!in||(in.mask&1<<8+tim)==0) return;

	mdl::header_t &hd = mdl::imageheader(in);
	
	if(!hd||tim>=hd.timblocks) return;

	size_t start = hd.primchanwords
	+hd.hardanimwords+hd.softanimwords, at = start;

	tim::header_t *th = 0; tim::pixmap_t *pm = 0;

	for(int i=0;i<=tim;i++)
	{			
		th = &psx::word_cast<tim::header_t>(in+at); 
		pm = &psx::word_cast<tim::pixmap_t>(in+at+2); 

		at+=2; if(in<=at+psx::word_size(*pm)) return;

		if(th->cf)
		{
			size_t bnum = pm->bnum;
			if(th->pmode>1||bnum<(th->pmode?512:32)+sizeof(tim::pixmap_t)) 
			{
				return; //require 256/16 entry 16bpp palette
			}
			size_t align = bnum; if(align%4) align++; //paranoia?

			pm = &psx::word_cast<tim::pixmap_t>(in+at+align/4); at+=align/4;

			if(in<=at+psx::word_size(*pm)) return;
		}

		size_t align = pm->bnum; if(align%4) align++; //paranoia?

		at+=align/4; if(in<=at) return;
	}

	if(!th||!pm) return;

	//pre-validate the entire image
	if(12u+pm->w*2u*pm->h<=pm->bnum
	 &&th->sixteen==16&&th->version==0&&th->pmode<3)
	{
		out.mode = in.mode; 
		out.mask = th->cf?3:1; 
		//out.file = in.file;
		out.set = (int32_t*)th+2; 
		out.end = out.set+at-start; 
		out.head = 2; 
		out.size = 4*(out.end-out.set+out.head); //NEW
		out.bad = 0; 
	}
}

void tim::maptofile(tim::image_t &in, tim::file_t filename, int mode, int mask,...)
{
	Swordofmoonlight_maptofile_format(Swordofmoonlight_tim,in,filename,mode,mask&0x2);
}
void tim::maptoram(tim::image_t &in, tim::ram_t ram, size_t bytes)
{
	Swordofmoonlight_mapheader_format(Swordofmoonlight_tim,in,ram,bytes);
}
void tim::maptorom(tim::image_t &in, tim::rom_t rom, size_t bytes)
{
	Swordofmoonlight_mapheader_format(Swordofmoonlight_tim,in,rom,bytes);
}

tim::header_t &tim::imageheader(tim::image_t &in)
{
	return in?*(tim::header_t*)(in.set-2):in.badref<tim::header_t>();
}

tim::pixmap_t &tim::clut(tim::image_t &in)
{
	if((in.mask&2)==0) return in.badref<tim::pixmap_t>();

	tim::header_t &hd =
	in?*(tim::header_t*)(in.set-2):in.badref<tim::header_t>();
	
	if(!hd||!hd.cf||in<=(int8_t*)(&hd+1)+sizeof(tim::pixmap_t)) 
	return in.badref<tim::pixmap_t>();

	tim::pixmap_t &out = *(tim::pixmap_t*)(&hd+1);
		
	if(in<=(int8_t*)&out+sizeof(tim::pixmap_t)+out.bnum)
	return in.badref<tim::pixmap_t>();

	return out;
}

tim::pixmap_t &tim::data(tim::image_t &in)
{
	tim::header_t &hd =
	in?*(tim::header_t*)(in.set-2):in.badref<tim::header_t>();

	tim::pixmap_t &clut = tim::clut(in); //paranoia...

	size_t align = clut?clut.bnum:0; if(align%4) align+=4-align%4; 

	int8_t *at = clut?(int8_t*)&clut+align:(int8_t*)(&hd+1);

	if(!hd||in<=at+sizeof(tim::pixmap_t)) return in.badref<tim::pixmap_t>();

	tim::pixmap_t &out = *(tim::pixmap_t*)at;

	if(in<=(int8_t*)&out+out.bnum) return in.badref<tim::pixmap_t>();

	return out;
}

void txr::maptofile(txr::image_t &in, txr::file_t filename, int mode, int mask,...)
{
	Swordofmoonlight_maptofile_format(Swordofmoonlight_txr,in,filename,mode,mask);
}

txr::header_t &txr::imageheader(txr::image_t &in)
{
	if(!in) return in.badref<txr::header_t>();

	return psx::word_cast<txr::header_t>(in.set-4);
}

txr::palette_t *txr::palette(txr::image_t &in)
{
	txr::header_t &head = txr::imageheader(in);

	return in&&head.depth<=8&&in>=in.set+256?(txr::palette_t*)in.set:0;
}

const txr::mipmap_t txr::mipmap(const txr::image_t &in, int n)
{
	const txr::header_t &head = txr::imageheader(in);

	txr::mipmap_t out; memset(&out,0x00,sizeof(out));

	//16: an arbitrary upward limit
	if(n<0||n>16||!in||(in.mask&1<<n)==0) return out; 

	int cols = head.depth/8;

	//mask should preempt this possibility
	if(cols<1||n>head.mipmaps) return out;

	//256: leap over the 256 entry palette
	int32_t *first = in.set; if(cols==1) first+=256; 

	int w = head.width, h = head.height;

	int rows = cols*w; if(rows%4) rows+=4-rows%4; 

	for(int i=0;i<n;i++)
	{	
		first+=rows/4*h;

		if(w==1&&h==1) break; //final possible mipmap
				
		w = max(w/2,1); h = max(h/2,1); rows = cols*w;

		if(rows%4) rows+=4-rows%4; 
	}

	if(in>=first+rows/4*h) 
	{
		out.depth = 8*cols;
		out.width = w; out.height = h;

		*(void**)&out.readonly = first;

		out.rowlength = rows;
	}

	return out;
}

mpx::base_t &mpx::base(mpx::image_t &img)
{
	void *end = img.end;
	auto &b = *(mpx::base_t*)img.set;
	if(!img.bad) for(;;)
	{
		auto &o = *(mpx::objects_t*)(&b+1);
		if(end<=o.list) break;
		auto &e = *(mpx::enemies_t*)(o.list+o.count);
		if(end<=e.list) break;
		auto &n = *(mpx::npcs_t*)(e.list+e.count);
		if(end<=n.list) break;			
		auto &i = *(mpx::items_t*)(n.list+n.count);
		if(end<=i.list) break;
		auto &t = *(mpx::tiles_t*)(i.list+i.count);
		if(end<=t.list) break;
		if(end<=t.list+t.count) break; return b;
	}
	img.bad = 1; return *(mpx::base_t*)0;
}
void mpx::maptofile(mpx::image_t &in, mpx::file_t filename, int mode)
{
	Swordofmoonlight_maptofile_format(0,in,filename,mode,1);
} 
mpx::body_t::body_t(mpx::base_t &b)
:
objects(*(mpx::objects_t*)(&b+1)),
enemies(*(mpx::enemies_t*)(objects.list+objects.count)),
npcs(*(mpx::npcs_t*)(enemies.list+enemies.count)),
items(*(mpx::items_t*)(npcs.list+npcs.count)),
tiles(*(mpx::tiles_t*)(items.list+items.count))
{
	/*NOP*/ 
}
mpx::data_t mpx::data(mpx::image_t &img, mpx::body_t &body, int mask)
{
	mpx::bspdata_t *bsp; 
	mpx::textures_t *tp; 
	mpx::vertices_t *vp; if(!img.bad) for(;mask&&body;)
	{
		if(img<body.tiles.list) goto bad;
		bsp = (mpx::bspdata_t*)(body.tiles.list+body.tiles.count);

		if(~mpx::base_cast(img).flags&1) //disabled?
		{
			tp = (mpx::textures_t*)bsp; bsp = 0;			
		}
		else 
		{
			auto *s1 = (mpx::bspdata_t::struct1*)bsp;
			auto *vi = s1->var_item_1;
			if(img<vi) goto bad;
			for(int i=s1->count;i-->0;)
			{
				(void*&)vi = vi->list+vi->count;
				if(img<vi) goto bad;
			}
			auto *s2 = (mpx::bspdata_t::struct2*)vi;
			if(img<s2->list) goto bad;
			auto *s3 = (mpx::bspdata_t::struct3*)(s2->list+s2->count);
			if(img<s3->list) goto bad;
			auto *s4 = (mpx::bspdata_t::struct4*)(s3->list+s3->count);
			if(img<s4->list) goto bad;
			//tp = (mpx::textures_t*)(s4->list+s4->count);
			tp = (mpx::textures_t*)(s4->list+(s1->count+7)/8*s1->count);
		}
			
		if(mask<2) break;

		//UNALIGNED READ
		auto *p = tp->refs;
		for(int i=tp->count;i-->0;p++)
		{
			for(;;) if(img>p) //!
			{
				if(*p) p++;
				else break;
			}else goto bad;
		}
		if(img<=p) goto bad;

		if(mask<4) break;

		vp = (mpx::vertices_t*)p;

		//UNALIGNED READ
		if(img<=vp->list+vp->count) goto bad;
		
		break; //!
	}
	else bad:
	{
		mask = 0; img.bad = 1;
	}
	if(~mask&1) bsp = 0;
	if(~mask&2) tp = 0;
	if(~mask&4) vp = 0;

	mpx::data_t dt = {*bsp,*tp,*vp}; return dt;
};

void evt::maptofile(evt::image_t &in, evt::file_t filename, int mode, int mask)
{
	Swordofmoonlight_maptofile_format(Swordofmoonlight_evt,in,filename,mode,mask);
}
void evt::maptoram(evt::image_t &in, evt::ram_t ram, size_t bytes)
{
	Swordofmoonlight_mapheader_format(Swordofmoonlight_evt,in,ram,bytes);
}
void evt::maptorom(evt::image_t &in, evt::rom_t rom, size_t bytes)
{
	Swordofmoonlight_mapheader_format(Swordofmoonlight_evt,in,rom,bytes);
}

static bool Swordofmoonlight_cd(const char *key, som::text_t val, som::char_t *cmp)
{
	som::char_t *p = cmp; while(*p&&*p!='=') p++;

	if(*p=='=') //case insensitive comparison
	{
		while(*key) if(tolower(*cmp++)!=tolower(*key++)) return false;

		cmp = p+1; //after =
	}

	while(iswspace(*cmp)) cmp++; //ltrim

	while(*val==*cmp&&*val){ val++; cmp++; } //literal comparison

	if(!*val&&*cmp) //rtrim
	{
		p = cmp; while(iswspace(*p)) p++; 

		if(!*p) *cmp = '\0';
	}

	return !*val&&!*cmp;
}

//note: was originally som::readfile
//http://en.swordofmoonlight.org/wiki/INI_file
static void Swordofmoonlight_ini(som::file_t in, int mask, som::environ_t environ, void *env)
{
	bool somfile = mask==0;	
	bool emitting_errors = somfile;
	bool emitting_margins = mask&2;	

	const int ln_s = 260*8; //MAX_PATH*8
	som::char_t ln[ln_s+1], var[ln_s+40]; 

	FILE *f = SWORDOFMOONLIGHT_FOPEN(in,"rt"); 		

	if(emitting_errors) 
	{
		som::text_t error[2] = {ln,ln+6};
		SWORDOFMOONLIGHT_SPRINTF(ln,"ERROR%c",'\0');
		if(!f) //setting _and_ clearing the ERROR variable
		SWORDOFMOONLIGHT_SNPRINTF(ln+6,ln_s-6,"Failed to open %s",in);
		environ(error,env);
	}

	if(!f) return;
	
	//NEW: interleaving margins
	som::char_t margin[ln_s+40]; 
	#define MARGIN if(emitting_margins)\
	{\
		margin[jj] = '\0'; jj = 0;\
		som::text_t lines[2] = {0,margin};\
		environ(lines,env);\
	}
	//NEW: get line ending from variable
	som::char_t newline[] = {'\n','\0'};	
	som::text_t expand[2] = {newline,0};
	environ(expand,env);
	som::char_t endline[8] = {'\r','\n','\0'};
	if(expand[1]&&*expand[1])
	SWORDOFMOONLIGHT_SPRINTF(endline,"%s",expand[1]);	
	#define ENDLINE(x,i) if(i&&x[i-1]=='\n')\
	{int ii = 0; while(x[i++-1]=endline[ii++]); i-=2;}\
	if(i>=ln_s) goto toolong;
	#ifdef _WIN32
	#define FINISHLINE (WEOF==ungetwc(getwc(f),f)) //feof(f)
	#else
	#define FINISHLINE (EOF==ungetc(getc(f),f))
	#endif 
	//FYI: vestigial bits of this algorithm ahead
	int i,j,jj,k; som::char_t *sep = 0, *end = 0; 
	for(i=0,j=0,jj=0,k=0;SWORDOFMOONLIGHT_FGETS(ln,ln_s+1,f);i++,k=0) 
	{			
		bool blank = true;		
		bool manyline = j;		
		if(!manyline) sep = 0;	 
		if(*ln=='['&&!sep) //section
		{
			while(ln[k]&&ln[k]!=']') k++;
			if(ln[k]!=']') ln[k] = ']'; ln[k+1] = '\0';
			som::text_t section_head[2] = {ln,0};
			MARGIN environ(section_head,env); 			
			if(section_head[1]!=ln) //skip it
			while(k&&'['!=(k=ungetwc(getwc(f),f)))
			k=!!SWORDOFMOONLIGHT_FGETS(ln,ln_s,f);
			continue; //it's a trap!
		}
		//parse left of the separator
		for(k=0;ln[k];k++) switch(ln[k])
		{
		case '\r': assert(0); //fgets removes \r

		case ' ': case '\t': case '\n':

			break;

		case '=': //separator?
			
			if(!manyline)
			{
				if(blank=!sep) sep = var+j++;

				break;
			}
			//break;

		default: 
						
			if(blank=!sep) var[j++] = ln[k]; 			
		}
		if(!k||ln[k-1]!='\n')
		{
			//2020: SOM's save games are now binary files with
			//a SOM file at their top, so it's important to bail
			//out on a 0-terminator. the only other reason for one
			//is the end of the file, so the seek to the end doesn't
			//matter

			if(k>=ln_s) toolong:
			{		
				//TODO: std::getline should be used to avoid arbitrary
				//limit. it will have to fall back to arbitrary limits
				//on Windows XP if it can't be made to work

				if(emitting_errors)
				{				
					som::text_t error[2] = {ln,ln+6};				
					SWORDOFMOONLIGHT_SPRINTF(ln,"ERROR%cLine %d exceeds built in character limit: %d",0,i,ln_s);				
					environ(error,env); 
				}				
			}
			fseek(f,0,SEEK_END);
		}

		som::text_t p = ln;					
		
		if(*ln==';') //comments
		{
			if(emitting_margins)
			{
				if(manyline) //embedded
				{
					while(var[j]=*p++) j++;					
					ENDLINE(var,j) 
					end = var+j;
				}
				else //garden_variety_margin:
				{
					while(margin[jj]=*p++) jj++;
					ENDLINE(margin,jj) 
				}
			}
			if(!manyline) j = 0; 
			continue;
		}
		else if(manyline&&blank) //end of the line(s)
		{	 			
			//trim last line ending
			while(iswspace(end[-1])) end--;
			*sep = '\0'; *end = '\0';			
			som::text_t lines[2] = {var,sep+1};
			MARGIN environ(lines,env); 
			j = 0; 
			//there should be an implicit blank line afterward
			//if(emitting_margins) goto garden_variety_margin; 
			continue;
		}				 			
		else if(!manyline)
		{
			//old two line SOM file?
			//files can also do this for looks
			if(i<2&&!sep) if(somfile) somfile:
			{
				som::text_t line[2] = {var,var+5};
				while(k&&iswspace(ln[k-1])) ln[--k] = '\0'; //rtrim				
				SWORDOFMOONLIGHT_SPRINTF(var,"%ls%c%s",i?L"DISC":L"GAME",0,ln);			
				MARGIN environ(line,env); 
				j = 0; continue;
			}
			else //does in end in .SOM?
			{
				int i=0; while(in[i++]);
				if(i>=5) if(in[i-5]=='.')				
				if(in[i-4]=='s'||in[i-4]=='S')
				if(in[i-3]=='o'||in[i-3]=='O')
				if(in[i-2]=='m'||in[i-2]=='M')
				goto somfile;
			}

			end = var+j; //new assignment line
			
			if(sep==end-1&&blank) //beginning block text
			{
				//NEW: herald with a line break
				var[j++] = '\n'; ENDLINE(var,j) 

				if(!FINISHLINE) continue; 
			}
			else if(!sep) //NEW: no separator? no problem!
			{	
				int ws = 0;	while(ws<j&&iswspace(var[ws])) ws++;
					
				if(ws<j) //void assignment
				{
					sep = end; sep[1] = '\0'; //void separator
				}
				else //fully blank line
				{						
					if(emitting_margins)
					{
						while(margin[jj]=*p++) jj++;
						ENDLINE(margin,jj)
					}
					continue;
				}
			}

			//position to right of separator
			while(*p&&*p!='=') p++;	if(*p=='=') p++;
			while(iswspace(*p)) p++; //ltrim
		}		
		
		//parse right of the separator
		while(*p&&j<ln_s) switch(*p++)
		{	
		case '%': //variable expansion?
						
			if(*p=='%') //%% built in variable
			{
				p++; //expand to nothing

				if(*p=='%') //%%% built in variable
				{
					p++; goto noexpand; //keep the last %
				} 
			}
			else //expand variable
			{
				som::text_t q = p+1; 
				
				while(*q&&*q!='%') q++;

				if(*q!='%') goto noexpand;

				const_cast<som::char_t&>(*q) = '\0'; 

				som::text_t expand[2] = {p,0}; 
				environ(expand,env); 

				const_cast<som::char_t&>(*q) = '%'; 

				if(!expand[1]) goto noexpand;

				ini::text_t pp = expand[1];

				while(j<ln_s&&*pp)
				switch(var[j++]=*pp++)
				{
				case '\r': j--; break;
				case '\n': ENDLINE(var,j) 
				}
				
				p = q+1; goto rtrim;
			}
			break;

		default: noexpand: 
			
			if(!iswspace(var[j++]=p[-1]))
			{
				rtrim: end = var+j;
			}
		}
		
		if(j>=ln_s) goto toolong;

		if(!manyline||FINISHLINE)
		{
			*sep = '\0'; *end = '\0';
			som::text_t lines[2] = {var,sep+1};
			MARGIN environ(lines,env); 
			j = 0; 
		}
		else ENDLINE(var,j)
	}
	if(!somfile) 
	{
		//trim final margin
		if(emitting_margins)
		while(jj&&iswspace(margin[jj-1])) jj--;
		som::text_t eof[2] = {0,0};
		MARGIN environ(eof,env); 
	}
	#undef FINISHLINE
	#undef ENDLINE
	#undef MARGIN
	fclose(f);
}

bool som::readfile(som::file_t in, som::environ_t environ, void *env)
{	
	if(!in||!*in||!environ) return true;

	//hack: 0 implicates som::readfile
	Swordofmoonlight_ini(in,0,environ,env);	
	som::text_t err = som::readerror(environ,env);
	if(err&&*err) return true; //error

	///////////////// disc detection /////////////////////

	const int ln_s = 260; //MAX_PATH 
	som::char_t ln[ln_s], var[ln_s+1];

	som::text_t cd[2] = {ln,0};
	SWORDOFMOONLIGHT_SPRINTF(ln,"CD"); environ(cd,env);
	
	if(!cd[1]||!*cd[1]) return true;

	//see if CD differs from in before verifying
	int i = 0; while(cd[1][i]==in[i]&&in[i]) i++;
	if(!cd[1][i]&&in[i]=='/'||in[i]=='\\') 
	{
		for(i++;in[i]&&in[i]!='/'&&in[i]!='\\';i++);

		if(!in[i]) return true; //cd contains in
	}

	som::text_t game[2] = {ln,0};
	SWORDOFMOONLIGHT_SPRINTF(ln,"GAME"); environ(game,env);
	som::text_t disc[2] = {ln,0};
	SWORDOFMOONLIGHT_SPRINTF(ln,"DISC"); environ(disc,env);
	som::text_t title[2] = {ln,0};
	SWORDOFMOONLIGHT_SPRINTF(ln,"TITLE"); environ(title,env);

	if(!game[1]||!*game[1])
	if(!disc[1]||!*disc[1]) return true;

    /*returning false on failure from here on out*/

	FILE *f = 0; 

	if(title[1])
	{	
		*ln = '\0';
		SWORDOFMOONLIGHT_SPRINTF(ln,"%s/%s.som",cd[1],title[1]);
		if(*ln)	f = SWORDOFMOONLIGHT_FOPEN(ln,"rt"); 
	}
	if(!f)
	{
		*ln = '\0';
		SWORDOFMOONLIGHT_SPRINTF(ln,"%s/CD.som",cd[1]);
		if(*ln) f = SWORDOFMOONLIGHT_FOPEN(ln,"rt"); 
	}

	if(!f) return false; 
	*ln = '\0'; SWORDOFMOONLIGHT_FGETS(ln,ln_s,f); //game
	*var = '\0'; SWORDOFMOONLIGHT_FGETS(var,ln_s,f); //disc
	fclose(f);

	if(game[1]&&*game[1])
	if(!Swordofmoonlight_cd("GAME",game[1],ln)) return false;		
	if(disc[1]&&*disc[1])
	if(!Swordofmoonlight_cd("DISC",disc[1],var)) return false;

	return true; 
}

void ini::readfile(ini::file_t in, int mask, ini::environ_t environ, void *env)
{
	//hack: 0 implicates som::readfile
	if(mask!=0&&in&&*in&&environ) Swordofmoonlight_ini(in,mask,environ,env);	
}

struct Swordofmoonlight_ini_sections
{
	int mask; bool eof; size_t count; ini::char_t ***text; 	

	Swordofmoonlight_ini_sections(){ eof = mask = count = 0; text = 0; }
};
struct Swordofmoonlight_ini_compiler : private Swordofmoonlight_ini_sections
{	
	ini::char_t *stack;
	ini::char_t *buffer; size_t size, fill, strings; ini::environ_t vars; void *varsp;
	template<size_t N>
	Swordofmoonlight_ini_compiler(char (&stackmem)[N], int m, ini::environ_t v, void *vp)
	{
		stack = buffer = (ini::char_t*)stackmem; mask = m; vars = v; varsp = vp;		
		size = N/sizeof(ini::char_t); 
		//section zero
		buffer[0] = '['; buffer[1] = ']'; buffer[2] = '\0'; //wcscpy(buffer,L"[]");
		fill = 3; count = 1; strings = 2; 
	}	
	static void environ_f(ini::text_t kv[2], void *me)
	{
		Swordofmoonlight_ini_compiler &c = 
		*(Swordofmoonlight_ini_compiler*)me;

		switch((kv[0]?1:0)|(kv[1]?2:0))
		{		
		case 0: c.eof = true; return;

		case 1: //environment variable?

			if(*kv[0]!='[') //section head?
			{
				if(c.vars) c.vars(kv,c.varsp); return;
			}
			else if(c.mask&4) //section filtering mode
			{
				c.vars(kv,c.varsp);	if(kv[1]!=kv[0]) return; 
			}
			else kv[1] = kv[0]; //normalize

			c.count++; c.strings+=2; break; 

		case 2: (const void*&)kv[0] = L""; 
			
			if(c.mask&2) break; //margin

			assert(0); return; //shouldn't be happening

		case 3: if(c.mask&1) break; //property

			kv[1] = kv[0] = (ini::text_t)L""; //dummy
		}
		c.strings++; assert(kv[0]&&kv[1]);

		for(ini::text_t p=kv[0],q=kv[1];;)
		{
			if(c.fill+2>=c.size) //reallocate?
			{
				ini::char_t *swap = c.buffer; 
				c.buffer = new ini::char_t[c.size=c.size*3/2];
				memcpy(c.buffer,swap,c.fill*sizeof(ini::char_t));
				if(swap!=c.stack) delete[] swap;
			}

			if(*p) //property key or section head
			{
				c.buffer[c.fill++] = *p++;
				
				if(!*p)	if(*kv[0]=='[') //section head
				{
					c.buffer[c.fill++] = '\0'; break; 
				}
				else if(*q) c.buffer[c.fill++] = '='; //separator
			}
			else if(!*q) //property value or margin
			{
				c.buffer[c.fill++] = '\0'; break;
			}
			else c.buffer[c.fill++] = *q++; 
		}
	}	
	//for one-time use (by ini::open) only!
	size_t finalize(ini::sections_t &inout) 
	{	
		if(!eof||fill<=3) return 0; //trivial		
		
		size_t count2 = mask&2?count:count*2;
		Swordofmoonlight_ini_sections *fin = 0;
		size_t table = count2*2+strings, fin_s = 
		sizeof(*fin)+table*sizeof(void*)+fill*sizeof(ini::char_t);
		fin = (Swordofmoonlight_ini_sections*)(inout=new char[fin_s]);
		memcpy(fin,this,sizeof(*fin)); 
		memset(fin->text=(ini::char_t***)(fin+1),0x00,table*sizeof(void*));
		ini::char_t ***margins = mask&2?fin->text+count:0;
		ini::char_t *p = (ini::char_t*)(fin->text+table), *p_fill = p+fill;		
		ini::char_t **paranoia = (ini::char_t**)p;
		memcpy(p,buffer,fill*sizeof(ini::char_t));		
		
		if(buffer!=stack) delete[] buffer; //bye!		
				
		//initialize string table pointers
		fin->text[0] = (ini::char_t**)fin->text+count2;
		if(margins) margins[0] = fin->text[0]+strings/2+count;

		size_t i,j; //paranoia
		for(i=-1,j=0;p<p_fill;j++) //fill out table
		{
			if(*p=='[') //new section
			{
				if(++i) //these are already terminated
				{	
					fin->text[i] = fin->text[i-1]+j+1;

					if(margins)	margins[i] = margins[i-1]+j+2; 

					j = 0; assert(i<count);
				}								
				if(margins) margins[i][j] = p;
			}
			else if(margins)
			{
				margins[i][j] = p; while(*p++);
			}
			fin->text[i][j] = p; while(*p++);
		}		
		if(margins) //paranoia
		assert(margins[i]+j+1<paranoia);
		assert(fin->text[i]+j<paranoia);
		assert(count-1==i&&p<=p_fill);		
		return count;
	}
};	
size_t ini::open(ini::sections_t &inout, ini::file_t inifile, int mask, ini::environ_t environ, void *env)
{	
	inout = 0; 	
	//reminder: 0 implicates som::readfile
	if(mask==0||!inifile||!*inifile) return 0; 

	char stackmem[32*1024];
	Swordofmoonlight_ini_compiler c(stackmem,mask,environ,env);
	Swordofmoonlight_ini(inifile,mask,c.environ_f,&c);
	return c.finalize(inout);
}

size_t ini::count(const ini::sections_t &in)
{
	return !in?0:((Swordofmoonlight_ini_sections*)in)->count;
}

bool ini::error(const ini::sections_t &in)
{
	return !in?true:!((Swordofmoonlight_ini_sections*)in)->eof;
}

void ini::close(ini::sections_t &in)
{
	delete[] in; in = 0;
}

const ini::text_t *ini::section(const ini::sections_t &in, size_t i)
{
	if(i>=ini::count(in)) return 0;
		
	return ((Swordofmoonlight_ini_sections*)in)->text[i];
}

const ini::text_t *ini::sectionmargins(const ini::sections_t &in, size_t i)
{
	if(i>/*=*/ini::count(in)) return 0; //>: there should be one extra margin
		
	Swordofmoonlight_ini_sections &iin = *(Swordofmoonlight_ini_sections*)in;	

	return iin.mask&2?iin.text[iin.count+i]:0;
}
	
//de-DEFLATE (inflate) 
//http://unlicense.org/
//http://code.google.com/p/miniz/source/browse/trunk/tinfl.c
//(changes are made for readability/maintainability)
//REMINDER: MAY NEED TO SUPPORT CRC-32 CHECKSUMS
//2017: I've updated the code a little after working on COLLADA-DOM
//which has to wrestle with the partial-input, wrapping-buffer deal.
//https://github.com/uroni/miniz/blob/master/miniz_tinfl.c now says,
/**************************************************************************
 *
 * Copyright 2013-2014 RAD Game Tools and Valve Software
 * Copyright 2010-2014 Rich Geldreich and Tenacious Software LLC
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **************************************************************************/
#define TINFL_BITBUF_SIZE 32
typedef uint32_t tinfl_bit_buf_t; //uint64_t
enum // Internal/private bits
{
  TINFL_MAX_HUFF_TABLES = 3, 
  TINFL_MAX_HUFF_SYMBOLS_0 = 288,
  TINFL_MAX_HUFF_SYMBOLS_1 = 32,
  TINFL_MAX_HUFF_SYMBOLS_2 = 19,
  TINFL_FAST_LOOKUP_BITS = 10, 
  TINFL_FAST_LOOKUP_SIZE = 1 << TINFL_FAST_LOOKUP_BITS,
};
typedef struct _tinfl_huff_table //compiler
{
	uint8_t m_code_size[TINFL_MAX_HUFF_SYMBOLS_0];
	int16_t m_look_up[TINFL_FAST_LOOKUP_SIZE], m_tree[TINFL_MAX_HUFF_SYMBOLS_0*2];

}tinfl_huff_table;
typedef struct _tinfl_decompressor //compiler
{
	uint32_t m_state, m_num_bits, m_zhdr0, m_zhdr1, m_z_adler32, m_final, m_type,
	m_check_adler32, m_dist, m_counter, m_num_extra, m_table_sizes[TINFL_MAX_HUFF_TABLES];

	tinfl_bit_buf_t m_bit_buf;
	size_t m_dist_from_out_buf_start;
	tinfl_huff_table m_tables[TINFL_MAX_HUFF_TABLES];
	uint8_t m_raw_header[4], m_len_codes[TINFL_MAX_HUFF_SYMBOLS_0+TINFL_MAX_HUFF_SYMBOLS_1+137];

}tinfl_decompressor;
enum 
{	//If set, the input has a valid zlib header and ends with an adler32 checksum (it's a valid zlib stream). Otherwise, the input is a raw deflate stream.	
	//TINFL_FLAG_PARSE_ZLIB_HEADER = 1,	
	//If set, there are more input bytes available beyond the end of the supplied input buffer. If clear, the input buffer contains all remaining input.
	TINFL_FLAG_HAS_MORE_INPUT = 2,	
	//If set, the output buffer is large enough to hold the entire decompressed stream. If clear, the output buffer is at least the size of the dictionary (typically 32KB).
	TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF = 4,	
	//Force adler-32 checksum computation of the decompressed bytes.
	//TINFL_FLAG_COMPUTE_ADLER32 = 8,		
};
typedef enum _tinfl_status //compiler
{
	TINFL_STATUS_FAILED_CANNOT_MAKE_PROGRESS = -4,
	TINFL_STATUS_BAD_PARAM = -3,
	TINFL_STATUS_ADLER32_MISMATCH = -2,
	TINFL_STATUS_FAILED = -1,
	TINFL_STATUS_DONE = 0,
	TINFL_STATUS_NEEDS_MORE_INPUT = 1,
	TINFL_STATUS_HAS_MORE_OUTPUT = 2

}tinfl_status;
static tinfl_status tinfl_decompress(tinfl_decompressor *r, 
const uint8_t*pIn_buf_next,size_t*pIn_buf_size,uint8_t*pOut_buf_start,uint8_t*pOut_buf_next,size_t*pOut_buf_size, 
const uint32_t decomp_flags)
{
	tinfl_status status = TINFL_STATUS_FAILED; //out

	static const int s_length_base[31] = 
	{ 3,4,5,6,7,8,9,10,11,13, 15,17,19,23,27,31,35,43,51,59, 67,83,99,115,131,163,195,227,258,0,0 };
	static const int s_length_extra[31] = 
	{ 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0,0,0 };
	static const int s_dist_base[32] = 
	{ 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193, 257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577,0,0};
	static const int s_dist_extra[32] = 
	{ 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};
	static const uint8_t s_length_dezigzag[19] = 
	{ 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };
	static const int s_min_table_sizes[3] = { 257,1,4 };
		
	uint32_t num_bits, dist, counter, num_extra; tinfl_bit_buf_t bit_buf;	
	const uint8_t *pIn_buf_cur = pIn_buf_next, *const pIn_buf_end = pIn_buf_next+*pIn_buf_size;
	uint8_t *pOut_buf_cur = pOut_buf_next, *const pOut_buf_end = pOut_buf_next+*pOut_buf_size; 
	size_t out_buf_size_mask = decomp_flags&TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF?size_t(-1):((pOut_buf_next-pOut_buf_start)+*pOut_buf_size)-1;
	size_t dist_from_out_buf_start;

	//Ensure the output buffer's size is a power of 2, unless the output buffer is large enough to hold the entire output file (in which case it doesn't matter).
	if((out_buf_size_mask+1)&out_buf_size_mask||pOut_buf_next<pOut_buf_start){ *pIn_buf_size = *pOut_buf_size = 0; return TINFL_STATUS_BAD_PARAM; }

	num_bits = r->m_num_bits; bit_buf = r->m_bit_buf; dist = r->m_dist; counter = r->m_counter; num_extra = r->m_num_extra; 
	dist_from_out_buf_start = r->m_dist_from_out_buf_start;
	
	#define TINFL_CR_BEGIN switch(r->m_state){ default: assert(0); case 0:
	#define TINFL_CR_RETURN(state_index,result) \
	{ status = result; r->m_state = state_index; goto common_exit; case state_index:; }
	#define TINFL_CR_RETURN_FOREVER(state_index,result) \
	for(;;){ TINFL_CR_RETURN(state_index,result); }
	#define TINFL_CR_FINISH } 	
	#define TINFL_GET_BYTE(state_index,c)\
	{\
		while(pIn_buf_cur>=pIn_buf_end)\
		TINFL_CR_RETURN(state_index,decomp_flags&TINFL_FLAG_HAS_MORE_INPUT?TINFL_STATUS_NEEDS_MORE_INPUT:TINFL_STATUS_FAILED_CANNOT_MAKE_PROGRESS)\
		c = *pIn_buf_cur++;\
	}
	#define TINFL_NEED_BITS(state_index,n) \
	do{ unsigned c; TINFL_GET_BYTE(state_index,c); bit_buf|=tinfl_bit_buf_t(c)<<num_bits; num_bits+=8;\
	}while(num_bits<unsigned(n));
	#define TINFL_SKIP_BITS(state_index,n) \
	{ if(num_bits<unsigned(n)) TINFL_NEED_BITS(state_index,n) bit_buf>>=(n); num_bits-=(n); }
	#define TINFL_GET_BITS(state_index,b,n) \
	{ if(num_bits<unsigned(n)) TINFL_NEED_BITS(state_index,n) b = bit_buf&((1<<(n))-1); bit_buf>>=(n); num_bits-=(n); } 
	//TINFL_HUFF_BITBUF_FILL() is only used rarely, when the number of bytes remaining in the input buffer falls below 2.
	//It reads just enough bytes from the input stream that are needed to decode the next Huffman code (and absolutely no more). It works by trying to fully decode a
	//Huffman code by using whatever bits are currently present in the bit buffer. If this fails, it reads another byte, and tries again until it succeeds or until the
	//bit buffer contains >=15 bits (deflate's max. Huffman code size).
	#define TINFL_HUFF_BITBUF_FILL(state_index,pHuff) \
	do{\
		temp = (pHuff)->m_look_up[bit_buf&(TINFL_FAST_LOOKUP_SIZE-1)]; \
		if(temp>=0)\
		{\
			code_len = temp>>9;\
			if(code_len&&num_bits>=code_len) break; \
		}\
		else if(num_bits>TINFL_FAST_LOOKUP_BITS)\
		{\
			code_len = TINFL_FAST_LOOKUP_BITS;\
			do{ temp = (pHuff)->m_tree[~temp+((bit_buf>>code_len++)&1)];\
			}while(temp<0&&num_bits>=code_len+1);\
			if(temp>=0) break;\
		}\
		TINFL_GET_BYTE(state_index,c)\
		bit_buf|=tinfl_bit_buf_t(c)<<num_bits;\
		num_bits+=8;\
	}while(num_bits<15);
	//TINFL_HUFF_DECODE() decodes the next Huffman coded symbol. It's more complex than you would initially expect because the zlib API expects the decompressor to never read
	//beyond the final byte of the deflate stream. (In other words, when this macro wants to read another byte from the input, it REALLY needs another byte in order to fully
	//decode the next Huffman code.) Handling this properly is particularly important on raw deflate (non-zlib) streams, which aren't followed by a byte aligned adler-32.
	//The slow path is only executed at the very end of the input buffer.
	#define TINFL_HUFF_DECODE(state_index,sym,pHuff) \
	{\
		int temp; unsigned code_len, c;\
		if(num_bits<15)\
		if(pIn_buf_end-pIn_buf_cur>=2)\
		{\
			bit_buf|=((tinfl_bit_buf_t)pIn_buf_cur[0]<<num_bits)|((tinfl_bit_buf_t)pIn_buf_cur[1]<<num_bits+8);\
			pIn_buf_cur+=2; num_bits+=16;\
		}\
		else TINFL_HUFF_BITBUF_FILL(state_index,pHuff)\
		temp = (pHuff)->m_look_up[bit_buf&(TINFL_FAST_LOOKUP_SIZE-1)];\
		if(temp<0)\
		{\
			code_len = TINFL_FAST_LOOKUP_BITS;\
			do{ temp = (pHuff)->m_tree[~temp+((bit_buf>>code_len++)&1)];\
			}while(temp<0);\
		}\
		else{ code_len = temp>>9; temp&=511; }\
		sym = temp;\
		bit_buf>>=code_len;	num_bits-=code_len;\
	}
	#define TINFL_ZERO_MEM(obj) memset(&(obj),0,sizeof(obj));
	#if 1 //MINIZ_USE_UNALIGNED_LOADS_AND_STORES && MINIZ_LITTLE_ENDIAN
	#define TINFL_READ_LE16(p) *((const uint16_t*)(p))
	#define TINFL_READ_LE32(p) *((const uint32_t*)(p))
	#else
	#define TINFL_READ_LE16(p) ((uint32_t)(((const uint8_t*)(p))[0])|((uint32_t)(((const uint8_t*)(p))[1])<<8U))
	#define TINFL_READ_LE32(p) ((uint32_t)(((const uint8_t*)(p))[0])|((uint32_t)(((const uint8_t*)(p))[1])<<8U)|\
					           ((uint32_t)(((const uint8_t*)(p))[2])<<16U)|\
							   ((uint32_t)(((const uint8_t*)(p))[3])<<24U))
	#endif
	TINFL_CR_BEGIN //switch(r->m_state){ case 0:
	//
	bit_buf = num_bits = dist = counter = 0;
	num_extra = r->m_zhdr0 = r->m_zhdr1 = 0; 
	r->m_z_adler32 = r->m_check_adler32 = 1;
	//
	/*if(0!=(decomp_flags&TINFL_FLAG_PARSE_ZLIB_HEADER))
	{
		TINFL_GET_BYTE(1,r->m_zhdr0); TINFL_GET_BYTE(2,r->m_zhdr1)
		counter = (r->m_zhdr0*256+r->m_zhdr1)%31!=0||(r->m_zhdr1&32)!=0||(r->m_zhdr0&15)!=8?1:0;
		if(0==(decomp_flags&TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF)) 
		counter|=1U<<8U+(r->m_zhdr0>>4)>32768U||out_buf_size_mask+1<size_t(1U<<8U+(r->m_zhdr0>>4));
		if(counter) TINFL_CR_RETURN_FOREVER(36,TINFL_STATUS_FAILED)
	}*/
	do
	{	TINFL_GET_BITS(3,r->m_final,3); 
		r->m_type = r->m_final>>1;
		if(r->m_type==0)
		{
			TINFL_SKIP_BITS(5,num_bits&7)
			for(counter=0;counter<4;++counter) 
			{
				if(num_bits) 
					TINFL_GET_BITS(6,r->m_raw_header[counter],8) 
				else TINFL_GET_BYTE(7,r->m_raw_header[counter]) 
			}

			counter = r->m_raw_header[0]|(r->m_raw_header[1]<<8);
			if(counter!=(unsigned)(0xFFFF^(r->m_raw_header[2]|(r->m_raw_header[3]<<8)))) 
			TINFL_CR_RETURN_FOREVER(39,TINFL_STATUS_FAILED)

			while(counter&&num_bits)
			{
				TINFL_GET_BITS(51,dist,8)
				while(pOut_buf_cur>=pOut_buf_end)
				TINFL_CR_RETURN(52,TINFL_STATUS_HAS_MORE_OUTPUT)
				*pOut_buf_cur++ = (uint8_t)dist;
				counter--;
			}

			while(counter)
			{
				while(pOut_buf_cur>=pOut_buf_end)
				TINFL_CR_RETURN(9,TINFL_STATUS_HAS_MORE_OUTPUT)

				while(pIn_buf_cur>=pIn_buf_end)
				TINFL_CR_RETURN(38,decomp_flags&TINFL_FLAG_HAS_MORE_INPUT?TINFL_STATUS_NEEDS_MORE_INPUT:TINFL_STATUS_FAILED_CANNOT_MAKE_PROGRESS)				

				size_t n = min(counter,min(size_t(pOut_buf_end-pOut_buf_cur),size_t(pIn_buf_end-pIn_buf_cur)));
				memcpy(pOut_buf_cur,pIn_buf_cur,n);				
				pIn_buf_cur+=n; pOut_buf_cur+=n; 
				counter-=(unsigned)n;
			}
		}
		else if(r->m_type==3)
		{
			TINFL_CR_RETURN_FOREVER(10,TINFL_STATUS_FAILED)
		}
		else
		{
			if(r->m_type==1)
			{
				uint8_t *p = r->m_tables[0].m_code_size; 
				r->m_table_sizes[0] = 288; r->m_table_sizes[1] = 32; 
				memset(r->m_tables[1].m_code_size,5,32); 
				unsigned i=0;
				for(;i<=143;++i) *p++ = 8; 
				for(;i<=255;++i) *p++ = 9;
				for(;i<=279;++i) *p++ = 7; 
				for(;i<=287;++i) *p++ = 8;
			}
			else
			{
				for(counter=0;counter<3;counter++)
				{ 
					TINFL_GET_BITS(11,r->m_table_sizes[counter],"\05\05\04"[counter])
					r->m_table_sizes[counter]+=s_min_table_sizes[counter]; 
				}
				TINFL_ZERO_MEM(r->m_tables[2].m_code_size)
				for(counter=0;counter<r->m_table_sizes[2];counter++) 
				{
					unsigned s; TINFL_GET_BITS(14,s,3)
					r->m_tables[2].m_code_size[s_length_dezigzag[counter]] = (uint8_t)s; 
				}
				r->m_table_sizes[2] = 19;
			}

			for(;(int)r->m_type>=0;r->m_type--)
			{					
				tinfl_huff_table *pTable;
				pTable = &r->m_tables[r->m_type]; //C2360
				TINFL_ZERO_MEM(pTable->m_look_up) TINFL_ZERO_MEM(pTable->m_tree)
				unsigned i,used_syms,total,sym_index,next_code[17],total_syms[16];
				TINFL_ZERO_MEM(total_syms)
				for(i=0;i<r->m_table_sizes[r->m_type];i++) 
				total_syms[pTable->m_code_size[i]]++;
				used_syms = total = next_code[0] = next_code[1] = 0;
				for(i=1;i<=15;used_syms+=total_syms[i++]) 
				next_code[i+1] = total = total+total_syms[i]<<1; 
				if(65536!=total&&used_syms>1)
				TINFL_CR_RETURN_FOREVER(35,TINFL_STATUS_FAILED)
				
				int tree_next, tree_cur; 
				for(tree_next=-1,sym_index=0;sym_index<r->m_table_sizes[r->m_type];sym_index++)
				{
					unsigned rev_code = 0, l, cur_code;
					unsigned code_size = pTable->m_code_size[sym_index]; 
					if(!code_size) continue;
					cur_code = next_code[code_size]++; 
					for(l=code_size;l>0;l--,cur_code>>=1) 
					rev_code = (rev_code<<1)|(cur_code&1);
					if(code_size <= TINFL_FAST_LOOKUP_BITS)
					{
						int16_t k = (int16_t)((code_size<<9)|sym_index); 
						while(rev_code<TINFL_FAST_LOOKUP_SIZE)
						{
							pTable->m_look_up[rev_code] = k; rev_code+=(1<<code_size); 
						} 
						continue; 
					}
					tree_cur = pTable->m_look_up[rev_code&(TINFL_FAST_LOOKUP_SIZE-1)];
					if(tree_cur==0)
					{
						pTable->m_look_up[rev_code&(TINFL_FAST_LOOKUP_SIZE-1)] = (int16_t)tree_next;
						tree_cur = tree_next; tree_next-=2; 
					}
					rev_code>>=(TINFL_FAST_LOOKUP_BITS-1);
					for(unsigned j=code_size;j>(TINFL_FAST_LOOKUP_BITS+1);j--)
					{
						tree_cur-=((rev_code>>=1)&1);
						if(!pTable->m_tree[-tree_cur-1])
						{ 
							pTable->m_tree[-tree_cur-1] = (int16_t)tree_next;
							tree_cur = tree_next; tree_next-=2;
						}
						else tree_cur = pTable->m_tree[-tree_cur-1];
					}
					tree_cur-=(rev_code>>=1)&1;
					pTable->m_tree[-tree_cur-1] = (int16_t)sym_index;
				}
				if(r->m_type==2)
				{
					for(counter=0;counter<r->m_table_sizes[0]+r->m_table_sizes[1];)
					{
						TINFL_HUFF_DECODE(16,dist,&r->m_tables[2])
						if(dist<16)
						{
							r->m_len_codes[counter++] = (uint8_t)dist; 
							continue; 
						}
						if(dist==16&&0==counter)
						TINFL_CR_RETURN_FOREVER(17,TINFL_STATUS_FAILED)
						unsigned s; 
						num_extra = "\02\03\07"[dist-16]; 
						TINFL_GET_BITS(18,s,num_extra) s+="\03\03\013"[dist-16];
						memset(r->m_len_codes+counter,(dist==16)?r->m_len_codes[counter-1]:0,s); 
						counter+=s;
					}
					if((r->m_table_sizes[0]+r->m_table_sizes[1])!=counter)
					{
						TINFL_CR_RETURN_FOREVER(21,TINFL_STATUS_FAILED)
					}
					memcpy(r->m_tables[0].m_code_size,r->m_len_codes,r->m_table_sizes[0]);
					memcpy(r->m_tables[1].m_code_size,r->m_len_codes+r->m_table_sizes[0],r->m_table_sizes[1]);
				}
			}

			for(uint8_t *pSrc;;)
			{
				for(;;)
				{
					if(pIn_buf_end-pIn_buf_cur<4||pOut_buf_end-pOut_buf_cur<2)
					{
						TINFL_HUFF_DECODE(23,counter,&r->m_tables[0])
						if(counter>=256) break;
						while(pOut_buf_cur>=pOut_buf_end)
						TINFL_CR_RETURN(24,TINFL_STATUS_HAS_MORE_OUTPUT)
						*pOut_buf_cur++ = (uint8_t)counter;
					}
					else
					{							
						#if 64==TINFL_BITBUF_SIZE //TINFL_USE_64BIT_BITBUF
						if(num_bits<30){ bit_buf|=(((tinfl_bit_buf_t)TINFL_READ_LE32(pIn_buf_cur))<<num_bits); pIn_buf_cur+=4; num_bits+=32; }
						#else
						if(num_bits<15){ bit_buf|=(((tinfl_bit_buf_t)TINFL_READ_LE16(pIn_buf_cur))<<num_bits); pIn_buf_cur+=2; num_bits+=16; }
						#endif

						unsigned code_len;
						int sym2 = r->m_tables[0].m_look_up[bit_buf&(TINFL_FAST_LOOKUP_SIZE-1)];
						if(sym2<0) 
						{
							code_len = TINFL_FAST_LOOKUP_BITS; 							
							do{ sym2 = r->m_tables[0].m_tree[~sym2+((bit_buf>>code_len++)&1)]; 							
							}while(sym2<0);
						}
						else code_len = sym2>>9;
						counter = sym2; bit_buf>>=code_len; num_bits-=code_len;
						if(counter&256) break;

						#if 64!=TINFL_BITBUF_SIZE //!TINFL_USE_64BIT_BITBUF
						if(num_bits<15){ bit_buf|=(((tinfl_bit_buf_t)TINFL_READ_LE16(pIn_buf_cur))<<num_bits); pIn_buf_cur+=2; num_bits+=16; }
						#endif

						sym2 = 
						r->m_tables[0].m_look_up[bit_buf&(TINFL_FAST_LOOKUP_SIZE-1)];
						if(sym2<0)
						{
							code_len = TINFL_FAST_LOOKUP_BITS; 
							do{ sym2 = r->m_tables[0].m_tree[~sym2+((bit_buf>>code_len++)&1)];
							}while(sym2<0);
						}
						else code_len = sym2>>9;
						bit_buf>>=code_len; num_bits-=code_len;
						pOut_buf_cur[0] = (uint8_t)counter;
						if(sym2&256)
						{
							pOut_buf_cur++;	counter = sym2;
							break;
						}
						pOut_buf_cur[1] = (uint8_t)sym2;
						pOut_buf_cur+=2;
					}
				}
				counter&=511;
				if(counter==256) break;
				num_extra = s_length_extra[counter-257];
				counter = s_length_base[counter-257];
				if(num_extra)
				{
					unsigned extra_bits; 
					TINFL_GET_BITS(25,extra_bits,num_extra)
					counter+=extra_bits; 
				}
				TINFL_HUFF_DECODE(26,dist,&r->m_tables[1])
				num_extra = s_dist_extra[dist]; 
				dist = s_dist_base[dist];
				if(num_extra)
				{
					unsigned extra_bits;
					TINFL_GET_BITS(27,extra_bits,num_extra)
					dist+=extra_bits; 
				}
				dist_from_out_buf_start = pOut_buf_cur-pOut_buf_start;
				if(dist>dist_from_out_buf_start&&decomp_flags&TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF)				
				TINFL_CR_RETURN_FOREVER(37,TINFL_STATUS_FAILED)

				pSrc = pOut_buf_start+((dist_from_out_buf_start-dist)&out_buf_size_mask);

				if(max(pOut_buf_cur,pSrc)+counter>pOut_buf_end)
				{
					while(counter--)
					{
						while(pOut_buf_cur>=pOut_buf_end)
						TINFL_CR_RETURN(53,TINFL_STATUS_HAS_MORE_OUTPUT)						
						*pOut_buf_cur++ = pOut_buf_start[(dist_from_out_buf_start-dist)&out_buf_size_mask];
						dist_from_out_buf_start++;
					}
					continue;
				}
				#if 1 //MINIZ_USE_UNALIGNED_LOADS_AND_STORES
				else if(counter>=9&&counter<=dist)
				{
					const uint8_t *pSrc_end = pSrc+(counter&~7);

					do
					{
						((uint32_t*)pOut_buf_cur)[0] = ((const uint32_t*)pSrc)[0];
						((uint32_t*)pOut_buf_cur)[1] = ((const uint32_t*)pSrc)[1];
						pSrc+=8; pOut_buf_cur+=8;

					}while(pSrc<pSrc_end);

					counter&=7;
					if(counter<3)
					{
						if(counter!=0)
						{
							pOut_buf_cur[0] = pSrc[0];
							if(counter>1)
							pOut_buf_cur[1] = pSrc[1];
							pOut_buf_cur+=counter;
						}
						continue;
					}
				}
				#endif
				do
				{
					pOut_buf_cur[0] = pSrc[0];
					pOut_buf_cur[1] = pSrc[1];
					pOut_buf_cur[2] = pSrc[2];
					pOut_buf_cur+=3; pSrc+=3;
					counter-=3;

				}while((int)counter>2);

				if((int)counter>0)
				{
					pOut_buf_cur[0] = pSrc[0];
					if((int)counter>1)
					pOut_buf_cur[1] = pSrc[1];
					pOut_buf_cur+=counter;
				}
			}
		}
	}while(0==(r->m_final&1));

	/* Ensure byte alignment and put back any bytes from the bitbuf if we've looked ahead too far on gzip, or other Deflate streams followed by arbitrary data. */
    /* I'm being super conservative here. A number of simplifications can be made to the byte alignment part, and the Adler32 check shouldn't ever need to worry about reading from the bitbuf now. */
    TINFL_SKIP_BITS(32,num_bits&7);
    while(pIn_buf_cur>pIn_buf_next&&num_bits>=8)
    {
        pIn_buf_cur--; num_bits-=8;
    }
    bit_buf&=tinfl_bit_buf_t((uint64_t(1)<<num_bits)-uint64_t(1));
    assert(0==num_bits); /* if this assert fires then we've read beyond the end of non-deflate/zlib streams with following data (such as gzip streams). */

	/*if(0!=(decomp_flags&TINFL_FLAG_PARSE_ZLIB_HEADER))
	{
		for(counter=0;counter<4;counter++) 
		{
			unsigned s; 
			if(0!=num_bits) TINFL_GET_BITS(41,s,8) else TINFL_GET_BYTE(42,s)
			r->m_z_adler32 = (r->m_z_adler32<<8)|s; 
		}
	}*/
	TINFL_CR_RETURN_FOREVER(34,TINFL_STATUS_DONE)
	TINFL_CR_FINISH //}

common_exit: /////////////////////////////////////////////////////////////////

	/* As long as we aren't telling the caller that we NEED more input to make forward progress: */
    /* Put back any bytes from the bitbuf in case we've looked ahead too far on gzip, or other Deflate streams followed by arbitrary data. */
    /* We need to be very careful here to NOT push back any bytes we definitely know we need to make forward progress, though, or we'll lock the caller up into an inf loop. */
    if(status!=TINFL_STATUS_NEEDS_MORE_INPUT
	 &&status!=TINFL_STATUS_FAILED_CANNOT_MAKE_PROGRESS)
    {
        while(pIn_buf_cur>pIn_buf_next&&num_bits>=8)
        {
            pIn_buf_cur--; num_bits-=8;
        }
    }

	r->m_num_bits = num_bits; 
	r->m_bit_buf = bit_buf&tinfl_bit_buf_t((uint64_t(1)<<num_bits)-uint64_t(1));
	r->m_dist = dist; 
	r->m_counter = counter; 
	r->m_num_extra = num_extra;
	r->m_dist_from_out_buf_start = dist_from_out_buf_start;

	*pIn_buf_size = pIn_buf_cur-pIn_buf_next; 
	*pOut_buf_size = pOut_buf_cur-pOut_buf_next;

	/*if(status>=0&&0!=(decomp_flags&(TINFL_FLAG_PARSE_ZLIB_HEADER|TINFL_FLAG_COMPUTE_ADLER32)))
	{			
		const uint8_t *ptr = pOut_buf_next; 
		size_t buf_len = *pOut_buf_size, block_len = buf_len%5552;
		uint32_t s1 = r->m_check_adler32&0xffff, s2 = r->m_check_adler32>>16; 
		while(0!=buf_len)
		{
			uint32_t i = 0;
			for(;i+7<block_len;i+=8,ptr+=8)
			{
				s2+=s1+=ptr[0]; s2+=s1+=ptr[1]; s2+=s1+=ptr[2]; s2+=s1+=ptr[3];
				s2+=s1+=ptr[4]; s2+=s1+=ptr[5]; s2+=s1+=ptr[6]; s2+=s1+=ptr[7];
			}
			for(;i<block_len;i++)
			{
				s2+=s1+=*ptr++;
			}

			s1%=65521U; s2%=65521U; buf_len-=block_len; block_len = 5552;
		}
		r->m_check_adler32 = (s2<<16)+s1; 
		if(status==TINFL_STATUS_DONE
		&&decomp_flags&TINFL_FLAG_PARSE_ZLIB_HEADER&&r->m_check_adler32!=r->m_z_adler32)
		status = TINFL_STATUS_ADLER32_MISMATCH;
	}*/
	return status;
}

size_t zip::inflate(const zip::image_t &in, zip::inflate_t &inf, size_t out)
{
	uint32_t f = 0;

	if(!out)
	{
		out = sizeof(inf.dictionary);
	}
	else if(out>sizeof(inf.dictionary))
	{
		assert(out<=sizeof(inf.dictionary)); return 0;
	}
	else f = TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF;

	zip::header_t &hd = zip::imageheader(in); 

	if(!hd||hd.approach!=8&&hd.approach) return 0; //8: DEFLATE
	
	if(inf.restart>=hd.bodysize)
	{
		assert(inf.restart==hd.bodysize); return 0; //finished 
	}

	if(!hd.approach) //courtesy: uncompressed
	{
		if(!in.test(hd.bodysize==hd.filesize)) return 0; 
		out = min(out,hd.bodysize-inf.restart);
		memcpy(inf,(uint8_t*)in.set+inf.restart,out); 
		inf.restart+=out; 
	}	
	else //based on tinfl_decompress_mem_to_callback example
	{	//http://code.google.com/p/miniz/source/browse/trunk/tinfl.c 			
		int compile[sizeof(inf.Swordofmoonlight_cpp)>sizeof(tinfl_decompressor)];
		tinfl_decompressor &decomp = (tinfl_decompressor&)inf.Swordofmoonlight_cpp;
		if(inf.restart==0) decomp.m_state = 0; //tinfl_init(&decomp);

		const uint8_t *src = (uint8_t*)in.set+inf.restart;		
		size_t src_remaining_in_inflated_out = hd.bodysize-inf.restart;
		tinfl_status status = tinfl_decompress(&decomp,src,&src_remaining_in_inflated_out,inf,inf,&out,f);		
		inf.restart+=src_remaining_in_inflated_out;	 

		switch(status)
		{
		case TINFL_STATUS_DONE: break;
		
		case TINFL_STATUS_HAS_MORE_OUTPUT: 
			
			assert(out==sizeof(inf.dictionary)); break;

		default: in.bad = 1; return 0; //indicate error
		}
	}

	return out;
}
 
struct Swordofmoonlight_zip_mapper
:
public swordofmoonlight_lib_image_t 
{
	size_t count, ibm437; 
	//trailercar:
	//using the directory at the 
	//end of the file for lookup
	//because the names are in a
	//row and so should behave a
	//little better with caching
	//(and we don't want to pull
	//in the front of the file!)
	//2017
	//TECHNICALLY THIS IS NEEDED
	//BECAUSE A FILE CAN BE JUNK
	//ACCORDING TO WIKIPEDIA/ZIP
	#include "pack.inl" /*push*/	
	struct thecaboose
	{
		const uint32_t x6054b50;
		const uint32_t diskettes;
		const uint16_t trailercars;
		const uint16_t trailercars2;
		const uint32_t sizeoftrailer;
		const uint32_t startoftrailer;
		const uint16_t comments;  

	}SWORDOFMOONLIGHT_PACK;	
	struct trailercar
	{
		const uint32_t x2014b50, versions;
		const uint16_t modemask, approach;
		const uint32_t datetime, crc32sum;
		const uint32_t bodysize, filesize;
		const uint16_t namesize, datasize;
		const uint16_t comments, diskette, pkattrib;
		const uint32_t fsattrib, locentry; 

		const uint8_t name[SWORDOFMOONLIGHT_N(260)];

	}SWORDOFMOONLIGHT_PACK 
	//POD w/ variable length array!
	*index[SWORDOFMOONLIGHT_N(30)]; 
	#include "pack.inl" /*pop*/
	inline void sort()
	{	
		if(*this) ::qsort(index,count,sizeof(*index),
		(int(*)(const void*,const void*))sort_compare);
		else count = ibm437 = 0; 
		if(count&&!(index[0]->modemask&0x400))
		for(ibm437=count;ibm437&&index[ibm437-1]->modemask&0x400;ibm437--);
		else ibm437 = 0;
	}		
	static int sort_compare(trailercar **aa, trailercar **bb)
	{
		trailercar *a=*aa,*b=*bb;
		int i = //IBM437 vs UTF8
		int(a->modemask&0x400)-int(b->modemask&0x400);
		if(i) return i;
		int n = min(a->namesize,b->namesize);
		while(i<n&&a->name[i]==b->name[i]) i++;
		if(i<n) return a->name[i]<b->name[i]?-1:1;
		if(a->namesize==b->namesize) return 0;
		return a->namesize<b->namesize?-1:1;
	}
	inline trailercar *searchIBM437(const uint8_t *key)const
	{
		if(!ibm437||!*key) return 0; 
		void *out = ::bsearch(key,index,ibm437,
		sizeof(void*),(int(*)(const void*,const void*))search_compare);
		return out?*(trailercar**)out:0;
	}
	inline trailercar *searchUTF8(const uint8_t *key)const
	{
		if(!utf8()||!*key) return 0;
		void *out = ::bsearch(key,index+ibm437,utf8(),
		sizeof(void*),(int(*)(const void*,const void*))search_compare);
		return out?*(trailercar**)out:0;
	}
	static int search_compare(const uint8_t *a, trailercar **bb)
	{
		size_t i,n; trailercar *b = *bb; 		
		for(i=0,n=b->namesize;i<n&&a[i]&&a[i]==b->name[i];i++);
		if(i>=n) return a[i]?1:0; //==
		if(a[i]) return a[i]<b->name[i]?-1:1;
		if(i&&a[i-1]=='/'&&b->name[i-1]=='/') return 0;
		return b->name[i]?-1:0;	
	}
	inline size_t utf8()const{ return count-ibm437; }
};

struct Swordofmoonlight_zmask
{	
	struct characters
	{			
		uint8_t codeunits[512]; size_t count;		
		characters(){ codeunits[0] = count = 0; }		
		void initialize(zip::file_t mask, int cp, uint8_t app=0) 
		{
			codeunits[sizeof(codeunits)-2] = 0; //paranoia
			int compile[sizeof(char)==sizeof(*codeunits)];
			#ifdef _WIN32			
			WideCharToMultiByte(cp,0,mask,-1,(char*)codeunits,sizeof(codeunits),0,0);
			#else //POSIX (not targeting consumers)
			SWORDOFMOONLIGHT_SNPRINTF((char*)codeunits,sizeof(codeunits),"%s",mask);
			#endif
			if(*codeunits&&!codeunits[sizeof(codeunits)-2]) 
			{
				for(count=0;codeunits[count];count++) 
				if(codeunits[count]=='\\') codeunits[count] = '/'; 
				if(app&&codeunits[count-1]!=app) codeunits[count++] = app;				
				//hack: keep trailing slashes away from zmap::search_compare
				if(app!='/'&&codeunits[count-1]=='/') *codeunits = errorcode;
			}
			else *codeunits = errorcode;
			if(*codeunits==errorcode) count = 1; 
			codeunits[count] = '\0';
		}
		inline bool match(const uint8_t *name, size_t namesize)
		{
			if(namesize<count) return false;
			//optimization: exact match case
			if(codeunits[count-1]!=name[count-1]
			  ||memcmp(name,codeunits,count)) return false;			
			if(namesize==count) return true; //file?
			size_t slash = codeunits[count-1]=='/'; //directory?
			return name[count-slash]=='/';
		}
		inline operator int(){ return codeunits[0]; }	 

	}codepages[2]; //IBM437 & UTF8

	enum{ errorcode=' ' }; bool error()
	{
		return codepages[0]==errorcode||codepages[1]==errorcode;
	}
};

size_t zip::open(zip::mapper_t &in, zip::file_t zipfile, zip::file_t mask)
{		
	in = 0; if(mask&&!*mask) mask = 0; 

	//REMINDER: DON'T READ FROM THE FRONT OF
	//THE FILE AS WE DON'T WANT ALL OF IT TO
	//BE LOADED UP INTO MEMORY UNNECESSARILY		

	Swordofmoonlight_zip_mapper zmap; 
	Swordofmoonlight_maptofile_format(0,zmap,zipfile,'r',1);

	if(!zmap||zmap<25) //25*4=100
	{
		swordofmoonlight_lib_unmap(&zmap); return 0; //cannot possibly be a zip file
	}

	int8_t *set = (int8_t*)zmap.set;
	//hack: zmap.end is aligned on four byte boundary
	int8_t *true_end = set+zmap.size;

	typedef Swordofmoonlight_zip_mapper::trailercar trailercar_t;
	typedef Swordofmoonlight_zip_mapper::thecaboose thecaboose_t;

	thecaboose_t *eof = (thecaboose_t*)(true_end-sizeof(*eof));

	//scan for 6054b50 in back to front order
	while(eof->x6054b50!=0x6054b50&&(int8_t*)&eof->x6054b50>set) 
	{
		((char*&)eof)--; //scan past possibile comments in trailer
	}	
	if(eof->x6054b50!=0x6054b50
	  ||eof->diskettes||eof->trailercars!=eof->trailercars2
	 /*Explorer modified files have stuff tacked onto the end
	  ||((int8_t*)(&eof->comments+1))+eof->comments!=true_end*/)
	{
		swordofmoonlight_lib_unmap(&zmap); return 0; //won't support
	}

	trailercar_t *sot = 
	(trailercar_t*)(set+eof->startoftrailer);
	trailercar_t *p = sot, *d = 
	(trailercar_t*)(set+eof->startoftrailer+eof->sizeoftrailer);
	if(zmap<d||(void*)d>(void*)eof)
	{
		swordofmoonlight_lib_unmap(&zmap); return 0; //corrupted?
	}
		
	//discarding zmap
	size_t baselen = (char*)zmap.index-(char*)&zmap;
	size_t varlen = sizeof(*zmap.index)*eof->trailercars;
	Swordofmoonlight_zip_mapper *inout = 
	(Swordofmoonlight_zip_mapper*) new char[baselen+varlen];
	in = memcpy(inout,&zmap,baselen); 
	{
		int bad = 0;
		typedef void zmap;
		Swordofmoonlight_zmask zmask;
		for(inout->count=0;p<d;p=(trailercar_t*)
		(p->name+p->namesize+p->datasize+p->comments))
		{
			if(++bad>eof->trailercars) break; //paranoia

			//we'll undo this down below
			inout->index[inout->count++] = p; if(mask)
			{
				int i = p->modemask&0x400?1:0; //Unicode?

				if(!zmask.codepages[i])
				{
					zmask.codepages[i].initialize(mask,i?65001:437);
				}
				if(!zmask.codepages[i].match(p->name,p->namesize))
				{
					inout->count--; //rejecting
				}
			}
		}
		inout->test(p==d&&!zmask.error()&&bad<=eof->trailercars);
		inout->sort();		
	}	
	//it's all or nothing
	if(!*inout) inout->count = 0;	
	return inout->count;
}

size_t zip::count(const zip::mapper_t &in)
{
	return !in?0:((Swordofmoonlight_zip_mapper*)in)->count;
}

void zip::close(zip::mapper_t &in)
{
	//Swordofmoonlight_zip_mapper are variable length arrays
	swordofmoonlight_lib_unmap((Swordofmoonlight_zip_mapper*)in); 
	delete[] in; in = 0;
}

bool zip::directory(const zip::mapper_t &in, zip::file_t mask)
{
	if(!zip::count(in)) return false;

	Swordofmoonlight_zip_mapper &zin = *(Swordofmoonlight_zip_mapper*)in;	
	Swordofmoonlight_zmask zmask;
	
	if(zin.ibm437) zmask.codepages[0].initialize(mask,437,'/');
	if(zin.utf8()) zmask.codepages[1].initialize(mask,65001,'/');

	if(zin.searchIBM437(zmask.codepages[0].codeunits)) 
	return true;		
	return zin.searchUTF8(zmask.codepages[1].codeunits); 
}
 
zip::entries_t zip::entries(const zip::mapper_t &in)
{
	return !in?0:(zip::entries_t)((Swordofmoonlight_zip_mapper*)in)->index;
}

size_t zip::firstunicodeentry(const zip::mapper_t &in)
{
	return !in?0:((Swordofmoonlight_zip_mapper*)in)->ibm437;
}

zip::entry_t &zip::find(const zip::mapper_t &in, zip::file_t mask)
{
	if(!zip::count(in)) return image_t::badref<zip::entry_t>();

	Swordofmoonlight_zip_mapper &zin = *(Swordofmoonlight_zip_mapper*)in;	
	Swordofmoonlight_zmask zmask;

	if(zin.ibm437) zmask.codepages[0].initialize(mask,437);
	if(zin.utf8()) zmask.codepages[1].initialize(mask,65001);

	Swordofmoonlight_zip_mapper::trailercar *tc = 0;
	int compile[sizeof(*tc)==sizeof(zip::entry_t)];	 
	tc = zin.searchIBM437(zmask.codepages[0].codeunits);
	if(!tc) tc = zin.searchUTF8(zmask.codepages[1].codeunits); 
	return *(zip::entry_t*)tc;
}

void zip::maptolocalentry(zip::image_t &out, const zip::mapper_t &in, zip::entry_t &e)
{
	memset(&out,0x00,sizeof(out)); out.bad = 1; 
	
	if(!zip::count(in)) return;

	Swordofmoonlight_zip_mapper &zin = *(Swordofmoonlight_zip_mapper*)in;	

	const size_t sizeof_zip__header_t_ = 30; 

	if(!e //Reminder: it's not an error to not find e!
	  ||zin<(int8_t*)e.locentry+sizeof_zip__header_t_) return;
	
	zip::header_t *hd = (zip::header_t*)((int8_t*)zin.set+e.locentry);
		
	if(!zin.test(!memcmp(&hd->bodysize,&e.bodysize,10))
	  ||zin<hd->name+hd->namesize+hd->datasize+hd->bodysize
	  ||!zin.test(!memcmp(hd->name,e.name,hd->namesize))) return;

	const size_t hd_s = sizeof_zip__header_t_+hd->namesize+hd->datasize;	
		
	out.size = hd_s+hd->bodysize;
	out.real = hd_s%4?4-hd_s%4:0; //32bit align
	out.head = (hd_s+out.real)/4;
	out.set = (int32_t*)((int8_t*)hd+hd_s);
	out.end = (int32_t*)((int8_t*)hd+hd_s+hd->bodysize);
	out.bad = 0; out.mode = 'r'; out.mask = 1;
}

struct Swordofmoonlight_res_mapper
:
public swordofmoonlight_lib_image_t 
{
	size_t count;

	res::resource_t index[SWORDOFMOONLIGHT_N(30)]; 
};

size_t res::open(res::mapper_t &in, res::file_t resfile, int mask)
{		
	in = 0; 

	Swordofmoonlight_res_mapper rmap; 
	Swordofmoonlight_maptofile_format(0,rmap,resfile,'r',1);

	if(!rmap) 
	{
		swordofmoonlight_lib_unmap(&rmap); return 0; 
	}

	size_t out = 0;
	
	union //v/w/d
	{ 
		int32_t *v; uint32_t *w; res::unicode_t d; 
	};

	for(v=rmap+0;v<rmap.end;out++)
	{
		size_t data_dwords = w[0]/4;
		size_t header_dwords = w[1]/4;

		if(w[0]%4) data_dwords++;
		if(w[1]%4) header_dwords++; 

		v+=header_dwords+data_dwords;
	}

	if(v>rmap.end) out--;

	if(!out) 
	{
		swordofmoonlight_lib_unmap(&rmap); return 0; 
	}

	//discarding rmap
	size_t baselen = (char*)rmap.index-(char*)&rmap;
	size_t varlen = sizeof(*rmap.index)*out;
	Swordofmoonlight_res_mapper *inout = 
	(Swordofmoonlight_res_mapper*) new char[baselen+varlen];
	in = memcpy(inout,&rmap,baselen); 
	{
		typedef void rmap;								
		inout->count = out;
		res::resource_t *r=inout->index, *s=r+out;
		for(v=*inout+0;r<s;r++)
		{
			size_t data_dwords = w[0]/4;
			size_t header_dwords = w[1]/4;

			if(w[0]%4) data_dwords++;
			if(w[1]%4) header_dwords++; 

			r->size = w+2; //union'ed with r->type

			v+=header_dwords+data_dwords;

			if(*r->type!=0xFFFF) //ordinal?
			{
				res::unicode_t p = r->type; while(p<d&&*p++); 

				r->name = p;
			}
			else r->name = r->type+2;
			
			if(!inout->test(r->name<d)) break;

			if(*r->name!=0xFFFF) //ordinal?
			{
				res::unicode_t p = r->type; while(p<d&&*p++); 

				r->lcid = (p-r->type)%2?p+4:p+3;
			}
			else r->lcid = r->name+5;

			if(!inout->test(r->lcid+5<=d)) break;			
		}
	}
	//it's all or nothing
	if(!*inout) inout->count = 0;	
	return inout->count;
}

size_t res::count(const res::mapper_t &in)
{
	return !in?0:((Swordofmoonlight_res_mapper*)in)->count;
}

void res::close(res::mapper_t &in)
{
	//Swordofmoonlight_res_mapper are variable length arrays
	swordofmoonlight_lib_unmap((Swordofmoonlight_res_mapper*)in); 
	delete[] in; in = 0;
}

res::resources_t res::resources(const res::mapper_t &in)
{
	return !in?0:(res::resources_t)((Swordofmoonlight_res_mapper*)in)->index;
}
 
void mo::maptofile(mo::image_t &in, mo::file_t filename)
{
	Swordofmoonlight_maptofile_format(Swordofmoonlight_mo,in,filename,'r',1);
	mo::header_t &hd = mo::imageheader(in); 
	in.test(hd&&hd.x950412de!=0x950412de);
} 

int mo::metadata(const mo::image_t &in, const char* &msgstr)
{
	const mo::header_t &hd = mo::imageheader(in); 
	if(!hd) return 0;

	const char *base = (char*)&hd;
	int32_t *id = (int32_t*)(base+hd.msgidndex);
	int32_t *str = (int32_t*)(base+hd.msgstrndex);

	if(in<id+1||*id //msgid=""?
	  ||in<=base+id[0]+id[1]) return 0;	

	if(in<str+1||!*str //translated?
	  ||in<=base+str[0]+str[1]) return 0;	

	msgstr = base+str[1]; return *str+1;
}

static int Swordofmoonlight_mo_strncmp(const char *a, const char *b, int n)
{
	for(int i=0;i<n;i++)
	if((uint8_t&)a[i]<='\r'&&(uint8_t&)b[i]<='\r'
	&&(a[i]=='\r'||a[i]=='\n')&&(b[i]=='\r'||b[i]=='\n'))
	{
		a+=i; b+=i; n-=i+1; if(n<2) return 0;
		a+=a[0]=='\r'&&a[1]=='\n'?2:1; b+=b[0]=='\r'&&b[1]=='\n'?2:1;
		return Swordofmoonlight_mo_strncmp(a,b,n);
	}
	else if(a[i]!=b[i]) return int((uint8_t&)a[i])-int((uint8_t&)b[i]);
	else if(a[i]=='\0') return 0; return 0; 
}
static int Swordofmoonlight_mo_bcompare(const char *a[3], uint32_t b[2])
{
	const char *p = a[0], *q = a[1]+b[1];
	size_t n = b[0]; if(q+n>=a[2]){ a[2] = 0; return 0; } //OOB
	return Swordofmoonlight_mo_strncmp(p,q,n+1);
};	  
int mo::gettext(const mo::image_t &in, const char *msgid, const char* &msgstr)
{
	if(!*msgid) return 0;
	const mo::header_t &hd = mo::imageheader(in); 
	if(!hd) return 0;

	const char *base = (char*)&hd;
	const void *key[3] = {msgid,base,in.end};
	int32_t *idbase = (int32_t*)(base+hd.msgidndex);
	int (*bcompare)(const void*,const void*);	
	(void*&)bcompare = Swordofmoonlight_mo_bcompare;
	int32_t *id = (int32_t*)::bsearch(key,idbase,hd.nmessages,8,bcompare);
	if(!id||!in.test(key[2])) return 0;

	int32_t *strbase = (int32_t*)(base+hd.msgstrndex);
	int32_t *str = strbase+(id-idbase);

	if(in<str+1||!*str //translated?
	||in<=base+str[0]+str[1]) return 0;

	msgstr = base+str[1]; return *str+1;
}
size_t mo::find(const mo::range_t &r, const char *msgid)
{
	int (*bcompare)(const void*,const void*);	
	(void*&)bcompare = Swordofmoonlight_mo_bcompare;
	const void *key[3] = {msgid,r.ctxtlog,(void*)-1};
	int32_t *out = (int32_t*)::bsearch(key,r.idof,r.n,8,bcompare);
	return out?(out-(int32_t*)r.idof)/2:r.n;
}

namespace Swordofmoonlight_mo_bcompare2
{
	struct key //mo::upperbound/lowerbound 
	{ 
		uint32_t *res; 
		const char *cat, *end, *substr; int n; 
	}; 
	static int lowerbound_predicate(key *a, uint32_t b[2])
	{
		const char *p = a->cat+b[1];
		if(p+b[0]>=a->end){ a->end = 0; return 0; } //OOB
		int out = Swordofmoonlight_mo_strncmp(a->substr,p,a->n); 
		if(out<=0) a->res = b; 
		return out?out:-1;
	}
	static int upperbound_predicate(key *a, uint32_t b[2])
	{
		const char *p = a->cat+b[1];
		if(p+b[0]>=a->end){ a->end = 0; return 0; } //OOB
		int out = Swordofmoonlight_mo_strncmp(a->substr,p,a->n);
		if(out<0) a->res = b; else if(!out) out = 1;
		return out;
	}
	static size_t bsearch //err will select the predicate
	(size_t err, const mo::image_t &in, const char *ctxt, int clen)
	{	
		struct key key = 
		{0,mo::catalog(in),(char*)in.end,ctxt,clen}; 
		if(!key.cat) return err;
													
		const mo::header_t &hd = mo::imageheader(in); 
		int (*bcompare)(const void*,const void*);	
		(void*&)bcompare = err?lowerbound_predicate:upperbound_predicate;
		uint32_t *idbase = (uint32_t*)(key.cat+hd.msgidndex);
		::bsearch(&key,idbase,hd.nmessages,8,bcompare);

		if(!in.test(key.end)) return err; //OOB
		if(!key.res) return hd.nmessages;		
		size_t out = (key.res-idbase)/2;  				
		//may want to just return out right here?		
		assert(out); 
		//return 1: exclude metadata/header-entry 
		//(assuming lower/upperbound)
		if(!out&&!key.cat[idbase[1]]) return 1; 
		return out;
	}
};	  
size_t mo::lowerbound(const mo::image_t &in, const char *ctxt, int clen)
{	
	return Swordofmoonlight_mo_bcompare2::bsearch(-1,in,ctxt,clen);
}
size_t mo::upperbound(const mo::image_t &in, const char *ctxt, int clen)
{
	return Swordofmoonlight_mo_bcompare2::bsearch(0,in,ctxt,clen);
}

size_t mo::nrange(const mo::image_t &in, range_t &r, size_t lb, size_t ub)
{
	r.n = 0; const mo::header_t &hd = mo::imageheader(in); if(!hd) return 0;

	if(ub>hd.nmessages) ub = hd.nmessages; if(lb>=ub) return 0; 
	
	const char *base = (char*)&hd;
	(int32_t*&)r.idof = (int32_t*)(base+hd.msgidndex);
	(int32_t*&)r.strof = (int32_t*)(base+hd.msgstrndex);	

	r.idof+=lb; r.strof+=lb;
	r.lowerbound = lb; r.catalog = r.ctxtlog = base; 
	size_t out = ub-lb;
	if(in<=r.idof+out||in<=r.strof+out) return 0;
	return r.n = out; 	
}
size_t mo::range(const mo::image_t &in, range_t &r, size_t lb, size_t ub)
{
	if(mo::nrange(in,r,lb,ub)&&!r.lowerbound&&!r.catalog[r.idof[0].coff])
	{
		r.lowerbound++; r.n--; r.idof++; r.strof++; 
	}
	return r.n;
}