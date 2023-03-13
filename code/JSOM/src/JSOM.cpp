
#define JSOM_PRINTF_MEMORY 256
#define JSOM_MSM_TEX_LIMIT 8
#define JSOM_MDO_TEX_LIMIT 8
#define JSOM_MDL_TEX_LIMIT 4

#include <assert.h>

#ifdef _WIN32 //2017: Trying to use vcpkg
#include <io.h>
#include <direct.h>
#include "Daedalus/getopt.h"
#define PATH_MAX 260 //MAX_PATH
#else
#include <unistd.h>	 
#include <getopt.h> //getopt_long
#endif

#include <sys/stat.h>
#include <cctype> //tolower
#include <limits>
#include <string>
#include <vector>
#include <map>
#include <utility>
#include <algorithm>

#include <png.h> 
#include <zlib.h> 
#include <openssl/md5.h>

extern "C"
{
#include "json.h"

//NONE enum is apparently not intended for json_print_x()
#define JSON_NONE compiler killer
}

#include "lib/swordofmoonlight.h"

using namespace SWORDOFMOONLIGHT;

#include "jsomap.h" //wraps around bmp2png 

//'prt', 'prt', etc.
static int operation = 0;

//commandline options
static int verbose = 0;
static int overwrite = 0;
static int externals = 1;
static char *file = 0;
static char *directory = 0;
static char *overwrite2 = 0;
static const char *prefix = "";
static const char *salt = "";
static bool gzip = false;

static const char *version = "Pre-Alpha";

//l10n messages
static const char *help = "\n\
Usage: jsom [OPERATION] [OPTIONS] [INPUT FILE]\n\
\n\
The JSOM program converts a Sword of Moonlight input file or stream\n\
into a JSON schema intended to be useful for WebGL applications. If not\n\
specified the OPERATION is taken from the INPUT FILE extension. External\n\
data such as textures and vertex buffer objects are cached in the current\n\
directory by default. Images are taken as textures to be converted to PNG\n\
format images consistent with Sword of Moonlight's 5bit black colorkey.\n\
\n\
OPERATION\n\
\n\
     One of the following:\n\
     '--prt' Sword of Moonlight .prt file format\n\
     '--prf' Sword of Moonlight .prf file format\n\
     '--mhm' Sword of Moonlight .mhm file format\n\
     '--msm' Sword of Moonlight .msm file format\n\
     '--mdo' Sword of Moonlight .mdo file format\n\
     '--mdl' Sword of Moonlight .mdl file format\n\
     '--txr' Sword of Moonlight .txr file format\n\
     '--bmp' Microsoft Windows .bmp image format\n\
\n\
INPUT FILE\n\
\n\
     The input file's format must match the current operation\n\
\n\
     If INPUT FILE is '-', stdin is read\n\
\n\
OPTIONS\n\
\n\
'-f, --file F'\n\
     The output file or device F (default \"-\", meaning stdout)\n\
\n\
'-C, --directory DIR' Change to directory DIR\n\
\n\
'-z, --gzip' Compression. JSON output is filtered through gzip\n\
\n\
     File extensions for external JSON will be json.gz\n\
\n\
'-h, --help' Display this help and exit\n\
\n\
'--overwrite' External files are forcibly overwritten\n\
\n\
'--overwrite-if-newer F' Conditional overwrite\n\
\n\
     Overwrite occurs if existing external file is older than F\n\
\n\
'--no-external-json' Do not externalize any of the JSON output\n\
\n\
'--external-prefix P' Prepend P to external file names.\n\
\n\
'--external-salt S' Add a salt to external file names.\n\
\n\
'-V, --version' Output version information and exit\n\
\n\
'-v, --verbose' Verbose output\n\
\n\
     If not writing to stdout, non-extenal JSON is printed\n\
\n";

static const char *usage = "For usage: run command with --help.";

static char printing[JSOM_PRINTF_MEMORY]; 
static json_printer spool[2]={0,0}, *job = spool;
	  
#ifdef _DEBUG 
#define JSOM_PRINTER json_print_pretty
#else
#define JSOM_PRINTER json_print_raw
#endif

#define JSOM_PRINT(t) \
JSOM_PRINTER(job,JSON_##t,0,0);
#define JSOM_PRINTF(t,...) \
JSOM_PRINTER(job,JSON_##t,printing,snprintf(printing,JSOM_PRINTF_MEMORY,__VA_ARGS__));

#ifdef _DEBUG 
#define JSOM_DEBUGF(...){ if(file||!gzip) printf("Debug: " __VA_ARGS__); }
#else
#define JSOM_DEBUGF(...){}
#endif

static int json(FILE *f, const char *s, uint32_t len)
{
	if(len>=JSOM_PRINTF_MEMORY) return 1;

	if(verbose&&file) fwrite(s,len,1,stdout);

	while(!fwrite(s,len,1,f)) if(ferror(f)) return 1;

	return 0; 
}

typedef struct _ZFILE : public z_stream_s
{
	_ZFILE(FILE *f, bool close=0)
	{
		gz = 0; close_gz = 0; //!ok()

		zalloc = Z_NULL; zfree = Z_NULL; opaque = Z_NULL;

		//16: need a gzip header here.
		if(deflateInit2(this,-1,Z_DEFLATED,15+16,8,Z_DEFAULT_STRATEGY)!=Z_OK)
		{
			fprintf(stderr,"Failure on deflateInit2.\n");

			return; //gz is 0
		}

		gz = f; close_gz = close;
	}

	~_ZFILE()
	{		
		def(Z_FINISH); deflateEnd(this);

		if(gz&&close_gz) fclose(gz);
	}

	unsigned char chunk[131072]; //128k

	inline void def(int flush)
	{
		if(gz) do //...
		{
			next_out = chunk;
			avail_out = sizeof(chunk);		 

			if(deflate(this,flush)!=Z_STREAM_ERROR)
			{
				unsigned len = sizeof(chunk)-avail_out;

				if(len) while(gz&&!fwrite(chunk,len,1,gz)) 
				{
					if(ferror(gz)) gz = 0; //!ok()
				}
			}
			else gz = 0; //!ok()
		}
		while(avail_out==0);

		if(!gz) 
		fprintf(stderr,"Failure on deflate.\n");		
	}
	
	inline bool ok(){ return gz?true:false; }
	
private: FILE *gz; bool close_gz;

}ZFILE;

static int json_gz(ZFILE *z, const char *s, uint32_t len)
{
	if(len>=JSOM_PRINTF_MEMORY) return 1;

	if(verbose&&file) fwrite(s,len,1,stdout);

	z->avail_in = len; z->next_in = (Bytef*)s;

	z->def(Z_NO_FLUSH); if(!z->ok()) return 1;
		
	return 0; 
}

int JSOM_rename(const char *src, const char *dst)
{
	int o = rename(src,dst); if(o)
	{
		//Win32 docs say EACCESS
		//in reality says EEXIST
		int e = errno;	
		if(e==EEXIST||e==EACCES) //Win32 
		{
			remove(dst); o = rename(src,dst);
		}
	}
	return o;
}

static int fd = 0;

static int main_prt();
static int main_prf();
static int main_mhm();
static int main_msm();
static int main_mdo();
static int main_mdl();
static int main_txr();
static int main_bmp();

int status = 0;
int substatus = 0;

int main(int argc, char *argv[]) 
{
//Reminder: options before everything else!

    struct option long_options[] = 
	{    
	//format based operations
	{ "prt", 0, &operation, 'prt' },
	{ "prf", 0, &operation, 'prf' },
	{ "mhm", 0, &operation, 'mhm' },
	{ "msm", 0, &operation, 'msm' },
	{ "mdo", 0, &operation, 'mdo' },
	{ "mdl", 0, &operation, 'mdl' },
	{ "txr", 0, &operation, 'txr' },
	{ "bmp", 0, &operation, 'bmp' },

	//tar like options		
	{ "file", 1, 0, 'f' }, //output file name (no effect on external file names)
	{ "help", 0, 0, 'h' },
	{ "verbose", 0, 0, 'v' },
	{ "version", 0, 0, 'V' },
    { "gzip", 0, 0, 'z' },  //filter JSON through gzip
	{ "directory", 1, 0, 'C' }, //change to directory (will be created if necessary) 
	{ "overwrite", 0, &overwrite, 1  }, //overwrite external files (default is to skip external files that exist)	

	//tar inspired options
	{ "overwrite-if-newer", 1, &overwrite, 2  }, //overwrite existing external files if older than provided file		
	
	//unique options
	{ "no-external-json", 0, &externals, 0 }, //roll one json file (mainly for debugging)
	{ "external-prefix", 1, 0, 'fix'},
	{ "external-salt", 1, 0, 'salt'},

	{ 0, 0, 0, 0 } /* terminating 0 item */
    };	
	
	for(int opt,long_opt_index=0;
	(opt=getopt_long(argc,argv,"f:hvVzC:",long_options,&long_opt_index))!=-1;)
	{
		switch(opt)
		{
		case 0: 

			switch(long_options[long_opt_index].val)
			{
			case 1: break; //--overwrite

			case 2: //--overwrite-if-older-than

				overwrite2 = optarg; break;
			}

			break;

		case 'f': file = optarg; break;

		case 'v': verbose = 1; break;

		case 'z': gzip = true; break;

		case 'C': directory = optarg; break;

		case 'fix': //--external-prefix

			prefix = optarg; break;

		case 'salt': //--external-salt

			salt = optarg; break;

		case 'V': printf("%s",version); return 0;
	    case 'h': printf("%s",help); return 0;
			
	    default: abort();
	   }
	}
	
	//Note: it looks like DDD is buggy (3.3.12/cygwin)
	/*The DDD Execution Window packs on 5 additional arguments...
	* So it may be that the shell passes along args intended for
	* other commands or whatever??? Needs research...
	*UPDATE: Found bug in a tracker (don't use the exec window)
	if(optind<argc-1)
	{
		fprintf(stderr,"%s %s\n","More than one input provided.",usage);

		return 1;
	}
	const char *in = optind!=argc?argv[argc-1]:0;
	*/
	const char *in = optind!=argc?argv[optind++]:0; //try this instead

	JSOM_DEBUGF("Top-level i/o is %s/%s\n",in?in:"-",file?file:"-")

	ZFILE *zout = 0; //compressed output

	char *incomplete = file?new char[strlen(file)+sizeof(".incomplete")+1]:0;
		 
	if(incomplete) sprintf(incomplete,"%s.incomplete",file);

	FILE *fout = file?fopen(incomplete,"w"):stdout; 

	if(!fout)
	{
		if(file)
		{
			fprintf(stderr,"%s %s\n","Failed to open file: ",incomplete);
		}
		else fprintf(stderr,"Unable to write to stdout\n");

		return 1;
	}

	FILE *f = in?fopen(in,"rb"):stdin;
						
	if(in&&!f)
	{
		fprintf(stderr,"Unable to open %s. %s\n",in,usage);

		goto _1;
	}

	if(!operation&&in&&f) //try to deduce operation
	{
		const char *ext = strrchr(in,'.');

		if(ext&&strlen(ext)==4)
		{
			char cmp[4] = {tolower(ext[1]),tolower(ext[2]),tolower(ext[3]),0};

			JSOM_DEBUGF("Deducing operation. Extensions is %s\n",cmp)

			if(!strcmp(cmp,"prt")) operation = 'prt'; else
			if(!strcmp(cmp,"prf")) operation = 'prf'; else 
			if(!strcmp(cmp,"mhm")) operation = 'mhm'; else
			if(!strcmp(cmp,"msm")) operation = 'msm'; else
			if(!strcmp(cmp,"mdo")) operation = 'mdo'; else
			if(!strcmp(cmp,"mdl")) operation = 'mdl'; else
			if(!strcmp(cmp,"txr")) operation = 'txr'; else
			if(!strcmp(cmp,"bmp")) operation = 'bmp';
		}
	}

	if(!operation) //failed to deduce operation
	{
		fprintf(stderr,"%s %s\n","Unable to deduce input format.",usage);

		goto _1;
	}

	//buffer stdin unless bmp/txr
	if(!in&&operation!='bmp'&&operation!='txr') 
	{
		//TODO: maximum limit?

		const size_t tmp_s = 4096;

		FILE *g = tmpfile(); char tmp[tmp_s]; 

		for(size_t rd=0;rd=fread(tmp,1,tmp_s,f);!rd&&feof(f)||!rd&&ferror(f))
		{ 
			for(size_t wr=0;wr<rd&&!ferror(g);wr+=fwrite(tmp+wr,1,rd-wr,g));
		}		

		if(!feof(f)||ferror(f)||ferror(g))
		{
			fprintf(stderr,"Unable to fully buffer stdin. This is a requirement.\n");

			goto _1;
		}

		f = g; fseek(g,0,SEEK_SET);

		if(0&&fread(tmp,1,tmp_s,g)) //debugging
		{
			tmp[64] = 0; printf("%s",tmp);
		}		
	}

	if(gzip) zout = new ZFILE(fout); 

	if(gzip)
	{
		if(!zout->ok())
		{
			fprintf(stderr,"Unable to instantiate zlib stream.\n"); 

			goto _1;
		}
		else if(json_print_init(job,(json_printer_callback)json_gz,zout)) 
		{
			fprintf(stderr,"Unable to initialize libjson print API.\n"); 

			goto _1;
		}
	}
	else if(json_print_init(job,(json_printer_callback)json,fout)) 
	{
		fprintf(stderr,"Unable to initialize libjson print API.\n"); 

		goto _1;
	}

	fd = fileno(f);
	
	char cd[PATH_MAX];

	if(directory)
	if(!getcwd(cd,PATH_MAX)||chdir(directory))
	{
		fprintf(stderr,"Unable to change directory to %s. %s\n",directory,usage);

		goto _1;
	}

	JSOM_DEBUGF("Proceeding with operation.\n")
		
	switch(operation)
	{
	case 'ptr': substatus = main_prt(); break;
	case 'prf': substatus = main_prf(); break;
	case 'mhm': substatus = main_mhm(); break;
	case 'msm': substatus = main_msm(); break;
	case 'mdo': substatus = main_mdo(); break;
	case 'mdl': substatus = main_mdl(); break;
	case 'txr': substatus = main_txr(); break;
	case 'bmp': substatus = main_bmp(); break;
	}

	if(status==0) status = substatus;

	if(directory&&chdir(cd))
	{
		fprintf(stderr,"Unable to change directory back to %s. %s\n",cd,usage);

		goto _1;
	}

	if(zout) delete zout; //flushes output

_0:	if(fout!=stdout) fclose(fout);

	if(f!=stdin) fclose(f); 
	
	if(incomplete&&status==0)
	{
		if(JSOM_rename(incomplete,file)) status = 1;
		
		delete incomplete;
	}

	if(status!=0)
	fprintf(stderr,"%s %s\n","Operation terminated (1)\nJSOM version is",version); 
	
	json_print_free(job);
	
    return status;

_1:	status = 1;

	goto _0;
}

bool external_exists_or_error(const char *fname)
{	
	if(overwrite&&!overwrite2) return false;

	struct stat st;
	if(!stat(fname,&st))
	{
		if(overwrite2)
		{
			static struct stat cmp; 
			
			static int onetime = stat(overwrite2,&cmp);

			if(onetime)
			{
				static int onetime2 = 0;

				if(!onetime2++)	fprintf(stderr,"%s %s\n","Unable to stat file argument of option --overwrite-if-newer.",usage);

				status = 1; return true; 
			}

			if(cmp.st_mtime>st.st_mtime) return false;
		}

		JSOM_DEBUGF("External file of this name exists: %s\n",fname);

		return true;
	}

	return false;
}

bool print_job(const char *fname, const char *depends=0) 
{	
	JSOM_DEBUGF("New print job initiated: %s\n",fname)

	if(external_exists_or_error(fname))
	if(!depends||external_exists_or_error(depends))
	{
		JSOM_DEBUGF("Job terminated.\n"); return false; 
	}

	if(job!=spool) //can handle only 1 external job at a time
	{
		fprintf(stderr,"%s\n","print_job: Unexpected behavior.");

		exit(1); //fatal error
	}
	else job++;
	
	std::string in(fname); in+=".incomplete";

	FILE *fout = fopen(in.c_str(),"w");
		
	if(gzip)
	{
		ZFILE *zout = new ZFILE(fout,"close me"); 

		if(!zout->ok())
		{
			fprintf(stderr,"Unable to instantiate zlib stream.\n"); 

			status = 1;
		}
		else if(json_print_init(job,(json_printer_callback)json_gz,zout)) 
		{
			fprintf(stderr,"Unable to initialize libjson print API.\n"); 

			status = 1;
		}
		else return true; //complete_job...

		delete zout;
	}
	else if(json_print_init(job,(json_printer_callback)json,fout)) 
	{
		fprintf(stderr,"Unable to initialize libjson print API.\n"); 

		status = 1;
	}
	else return true; //complete_job...

	memset(job--,0x00,sizeof(json_printer)); 
	
	return false;
}

bool complete_job(const char *fname, bool complete=true) //of print_job
{		
	if(job!=spool+1||!job->callback)
	{
		fprintf(stderr,"complete_job: Unexpected behavior.\n");

		exit(1); //fatal error
	}
			
	if(gzip)
	{
		delete (ZFILE*)job->userdata;
	}
	else fclose((FILE*)job->userdata);
	
	json_print_free(job); 

	memset(job--,0x00,sizeof(json_printer)); 

	std::string in(fname); in+=".incomplete"; 
	
	if(!complete||JSOM_rename(in.c_str(),fname)) return false; 

	JSOM_DEBUGF("Print job completed: %s\n",fname)

	return true;
}

static const char *hex_md5(unsigned char *md5=0, int force=0)
{
	static char out[32+1] = ""; if(!md5&&!force) return out;
 
	const uint32_t *_ = (uint32_t*)md5;	
	if(_&&sprintf(out,"%08x%08x%08x%08x",_[0],_[1],_[2],_[3])==32) return out;

	fprintf(stderr,"hex_md5: Internal MD5 failure.\n");

	*out = '\0'; status = 1; return 0;
}

/*generic JSOM schema reference
{
 "externals"
 [
  {"png":"http://..."}, //image
  {"json":"http://..."}, //buffer objects
 ],
   
 "images":
 [
  {"ref":"tex"},     //texture reference
  {"url":0},         //external texture
  {"rgb":[1,0,0,1]}, //1 texel texture   
 ],
 
 "arrays":
 [
  {"url":1}, //vertex buffer objects
  {"data":[1.0f,...]}, 
 ],
 
 "attributes": //override array attributes
 [ 
  {
   "semantic":"position",
   "identifier":"position", //defaults to semantic
   "array":1,
   "size":4,
   "offset":0,
   "stride":4,
  },
 ], 
 
 "elements":
 [
  {//GL_TRIANGLES 0x0004
   "mode":4,
   "array":1, //arrays[0.0] (nesting) 
   "start":0,
   "count":368,
  },
  {//GL_LINES 0x0001
   "mode":1,
   "array":2; //arrays[0.1]
   "start":0,
   "count":220,
  },
 ],

 "scripts": //shaders
 [
  {"url":2},
  [ //compose...
   "#define _DEBUG\n",
   {"url":3},
  ],  
 ],
 
 "semantics"
 [
  {"position":
   {glsl:"mix(position,tweenpos,tween)"}
  }					   
 ]					   

 "materials"
 [
  {   
   "semantics":[0,null];   

   "texture":[0,1,2], //images (multi-texture)
   
   "shaders":[], //scripts (vertex, fragment, geometry)   
   
   "ambient":{"rgb":[]}, 
   "diffuse":{"rgb":[]},     
   "emissive":{"rgb":[]},
  },
 ],
 
 "graphics"
 [
  {
   "sides":[0,null], //materials
     
   "state":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1], //matrix
   
   "attributes":[], //default is to use all arrays as attributes

   "primitives":[0], //1st set of elements   
   "wireframes":[1], //2nd set of elements
   "controller":[3], //3rd set of elements

   "clearance":[[-1,1],[-1,1],[-1,1]];
         
   "graphics": //nesting
   [
   ],     
  },
 ],
 
 "tracks"
 [
  {"rot":[,[0,2],]}, //turn table animation 
  {"url":2}, //external animations
 ],
 
 "listing"
 [
  {"html":"textual file info etc..."},
 ],
}
*/

static void print_attribute(const char *id, int size, int stride, int start, int array=0)
{
	JSOM_PRINT(OBJECT_BEGIN)
	JSOM_PRINTF(KEY,"array") 
	if(array==0) JSOM_PRINTF(INT,"0") 
	if(array!=0) JSOM_PRINTF(INT,"%d",array) 
	JSOM_PRINTF(KEY,"semantic") //identifier
	JSOM_PRINTF(STRING,"%s",id)	
	if(size!=3) JSOM_PRINTF(KEY,"size")
	if(size!=3) JSOM_PRINTF(INT,"%d",size)	
	if(stride) JSOM_PRINTF(KEY,"stride")
	if(stride) JSOM_PRINTF(INT,"%d",stride)	
	if(start) JSOM_PRINTF(KEY,"start")
	if(start) JSOM_PRINTF(INT,"%d",start)	
	JSOM_PRINT(OBJECT_END)
}

struct jsom_imageinfo
{
	bool alpha;

	int width, height;

	jsom_imageinfo()
	{
		alpha = false; width = height = 0;
	}

	inline operator bool()
	{
		return width>0&&height>0;
	}
};

static void print_imageinfo(jsom_imageinfo &info)
{
	JSOM_PRINTF(KEY,"alpha")
	if(!info.alpha) JSOM_PRINT(FALSE)
	if(info.alpha) JSOM_PRINT(TRUE)
	if(info.width>0&&info.height>0)
	{
		JSOM_PRINTF(KEY,"width")
		JSOM_PRINTF(INT,"%d",info.width)
		JSOM_PRINTF(KEY,"height")
		JSOM_PRINTF(INT,"%d",info.height)
	}
}

template<typename T> struct jsom_clearance
{	
	T min[3], max[3];

	int stride, width; const int *filters;

	template<typename Int>
	inline jsom_clearance<T> &operator<<(Int w)
	{
		width = int(w); return *this;
	}

	inline jsom_clearance<T> &operator<<(T *v)
	{
		if(!v) return *this;

		for(int i=0;i<width;i++)
		{
			for(int j=0;j<3;j++) if(v[j]<min[j]&&f(i,j)) min[j] = v[j];
			for(int j=0;j<3;j++) if(v[j]>max[j]&&f(i,j)) max[j] = v[j];

			v+=stride;
		}

		return *this;
	}

	jsom_clearance(int s, const int *f=0):stride(s),width()
	{			
		min[0] = min[1] = min[2] = std::numeric_limits<T>::max();
		max[0] = max[1] = max[2] = std::numeric_limits<T>::min();

	//Assuming we want to include the ground level at Y=0

		min[1] = max[1] = 0; filters = f; //filter out CPs
	}

	inline operator bool()
	{ 
		return min[0]!=std::numeric_limits<T>::max(); 
	}

	inline int f(int i, int j)
	{
		if(filters) 
		for(int k=1;k<=*filters;k++)
		{
			if(filters[k]==i) return false;
		}
		return true;
	}
};

template<typename T>
static void print_clearance(jsom_clearance<T> &clr,float x=1,float y=1,float z=1)
{
	if(!clr) return;

	//TODO: "clearance":[[-1,1],[-1,1],[-1,1]];
	JSOM_PRINTF(KEY,"clearance")
	JSOM_PRINT(ARRAY_BEGIN)
	 JSOM_PRINT(ARRAY_BEGIN)
	 if(x>0) JSOM_PRINTF(FLOAT,"%f",x*clr.min[0])
	 if(x>0) JSOM_PRINTF(FLOAT,"%f",x*clr.max[0])
	 if(x<0) JSOM_PRINTF(FLOAT,"%f",x*clr.max[0])
	 if(x<0) JSOM_PRINTF(FLOAT,"%f",x*clr.min[0])
	 JSOM_PRINT(ARRAY_END)
	 JSOM_PRINT(ARRAY_BEGIN)
	 if(y>0) JSOM_PRINTF(FLOAT,"%f",y*clr.min[1])
	 if(y>0) JSOM_PRINTF(FLOAT,"%f",y*clr.max[1])
	 if(y<0) JSOM_PRINTF(FLOAT,"%f",y*clr.max[1])
	 if(y<0) JSOM_PRINTF(FLOAT,"%f",y*clr.min[1])
	 JSOM_PRINT(ARRAY_END)
	 JSOM_PRINT(ARRAY_BEGIN)
	 if(z>0) JSOM_PRINTF(FLOAT,"%f",z*clr.min[2])
	 if(z>0) JSOM_PRINTF(FLOAT,"%f",z*clr.max[2])
	 if(z<0) JSOM_PRINTF(FLOAT,"%f",z*clr.max[2])
	 if(z<0) JSOM_PRINTF(FLOAT,"%f",z*clr.min[2])
	 JSOM_PRINT(ARRAY_END)
	JSOM_PRINT(ARRAY_END)
}

static const char *sum_md5(jsomap_t *map)
{
	if(!map||!map->imgbytes)
	{
		fprintf(stderr,"sum_md5: Zero size image.\n");

		status = 1; return 0;
	}

	MD5_CTX c; 

	size_t row = 0;
	size_t rows = map->imgbytes/map->rowbytes;

	if(MD5_Init(&c))
	for(row=0;row<rows;row++)
	{
		if(!map->palette)
		{
			if(!MD5_Update(&c,map->rowptr[row],map->rowbytes)) break;
		}
		else for(size_t i=0;i<map->rowbytes;i++) //hmm: would a buffer help?
		{
			unsigned char p = map->rowptr[row][i]; 

			if(!MD5_Update(&c,map->palette+p,sizeof(*map->palette))) break;
		}
	}

	unsigned char tmp[16];
	if(row!=rows||!MD5_Final(tmp,&c))
	{
		fprintf(stderr,"sum_md5: OpenSSL MD5 failure.\n");

		status = 1; return 0;
	}

	return hex_md5(tmp);
}

static bool print_mhm_external(mhm::image_t &in, std::string &inout)
{
	if(!in) return false; 
		
	mhm::header_t &hd = mhm::imageheader(in);
	mhm::face_t *fp;	
	mhm::index_t *ip;
	mhm::vector_t *vp,*np;
	int npN = mhm::imagememory(in,&vp,&np,&fp,&ip);
	if(!in) return false;
	
	if(inout=="0") //arrays
	{	
		typedef std::pair<int,int> key;		
		typedef std::map<key,int>::iterator map_it;		
		std::map<key,int> map;
		std::pair<key,int> vt;
		std::pair<map_it,bool> ins;		
		std::vector<int> els,poly;

		if(externals) 
		{		
		//WARNING: assuming msm after texture header 32bit aligned...

		    //compile if uin8_t is of char type
			if(hex_md5(MD5((uint8_t*)in.set,4*(in.end-in.set),0),'f'))
			{
				inout = std::string("mhm_")+salt+hex_md5()+".json"+(gzip?".gz":"");
				
				//todo: returning true here is a little too ambiguous

				if(!print_job(inout.c_str())) return true; 
			}
			else return false;

			//.json wrapper
			JSOM_PRINT(OBJECT_BEGIN)
			JSOM_PRINTF(KEY,"arrays")
			JSOM_PRINT(ARRAY_BEGIN)
		}

		//attributes
		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"name")
		JSOM_PRINTF(STRING,"vertices")
		JSOM_PRINTF(KEY,"data")
		JSOM_PRINT(ARRAY_BEGIN)		
		unsigned i,j,iN = hd.facecount,jN,kN;
		for(i=j=0;i<iN;i++,j=0)	
		{
			vt.first.first = fp[i].normal;			
			for(jN=fp[i].ndexcount;j<jN;j++)
			{		
				vt.first.second = *ip++;
				ins = map.insert(vt);
				poly.push_back(ins.first->second);
				if(!ins.second) continue;				
				vt.second++;
				mhm::vector_t &n = np[vt.first.first];
				mhm::vector_t &v = vp[vt.first.second];  
				JSOM_PRINTF(FLOAT,"%f",-v[0])
				JSOM_PRINTF(FLOAT,"%f",+v[1])
				JSOM_PRINTF(FLOAT,"%f",+v[2]) 		
				JSOM_PRINTF(FLOAT,"%f",-n[0])
				JSOM_PRINTF(FLOAT,"%f",+n[1])
				JSOM_PRINTF(FLOAT,"%f",+n[2])					
			}
			for(int i=1,iN=(int)poly.size()-1;i<iN;)
			{
				//SOM_MAP flips XZ axes
				els.push_back(poly[i]);
				els.push_back(poly[0]);				
				els.push_back(poly[++i]);
			}
			poly.clear();
		}		
		JSOM_PRINT(ARRAY_END)
		JSOM_PRINT(OBJECT_END)
				
		//elements
		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"name")
		JSOM_PRINTF(STRING,"triangles")
		JSOM_PRINTF(KEY,"index")
		JSOM_PRINT(TRUE)
		JSOM_PRINTF(KEY,"data")
		JSOM_PRINT(ARRAY_BEGIN)
		int elements = els.size();
		for(int i=0;i<elements;i++)
		JSOM_PRINTF(INT,"%d",els[i]);
		JSOM_PRINT(ARRAY_END)
		JSOM_PRINT(OBJECT_END)

		if(externals) 
		{
			//.json wrapper 
			JSOM_PRINT(ARRAY_END)
			JSOM_PRINT(OBJECT_END)
			
			return complete_job(inout.c_str());
		}
	}
	else return false; return true;
}
static int main_mhm() //2018: copying main_msm
{
	JSOM_DEBUGF("Entering main_mhm()\n")

	mhm::image_t in; mhm::maptofile(in,0,fd);
	
	mhm::face_t *fp;
	mhm::vector_t *vp;
	mhm::imagememory(in,&vp,0,&fp);
	if(!in) return mhm::unmap(in);
	mhm::header_t &hd = mhm::imageheader(in);
	
	JSOM_PRINT(OBJECT_BEGIN)

//externals: //MHM	
	
	int elements = 0;
	std::string external_0 = "0";

	JSOM_PRINTF(KEY,"externals")
	JSOM_PRINT(ARRAY_BEGIN)	

	if(externals)
	if(print_mhm_external(in,external_0))
	{
		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"json")
		JSOM_PRINTF(STRING,"%s%s",prefix,external_0.c_str())
		JSOM_PRINT(OBJECT_END)	
	}		
	JSOM_PRINT(ARRAY_END)
	
//arrays: //MHM

	JSOM_PRINTF(KEY,"arrays")
	JSOM_PRINT(ARRAY_BEGIN)
	
	if(externals)
	{		
		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"url")
		JSOM_PRINTF(INT,"0")
		JSOM_PRINT(OBJECT_END)
	}
	else print_mhm_external(in,external_0);
	
	JSOM_PRINT(ARRAY_END)
	
//attributes: //MHM

	JSOM_PRINTF(KEY,"attributes")
	JSOM_PRINT(ARRAY_BEGIN)
	{
		print_attribute("position",3,6,0);
		print_attribute("normal",3,6,3);
	}
	JSOM_PRINT(ARRAY_END)

//elements: //MHM
		
	JSOM_PRINTF(KEY,"elements")
	JSOM_PRINT(ARRAY_BEGIN)
	{
		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"mode")
		JSOM_PRINTF(STRING,"TRIANGLES")
		JSOM_PRINTF(KEY,"array")
		JSOM_PRINTF(INT,"1")
		JSOM_PRINTF(KEY,"start")
		JSOM_PRINTF(INT,"0")
		JSOM_PRINTF(KEY,"count")
		//Must compute if externals is available.
		int elements = 0;
		for(int i=0,iN=hd.facecount;i<iN;i++)			
		elements+=(fp[i].ndexcount-2)*3;
		JSOM_PRINTF(INT,"%d",elements)
		JSOM_PRINT(OBJECT_END)
	}
	JSOM_PRINT(ARRAY_END)

//images: //MHM

	JSOM_PRINTF(KEY,"images")
	JSOM_PRINT(ARRAY_BEGIN)	
	JSOM_PRINT(OBJECT_BEGIN)
	JSOM_PRINTF(KEY,"rgb")
		JSOM_PRINT(ARRAY_BEGIN)
		JSOM_PRINTF(FLOAT,"0")
		JSOM_PRINTF(FLOAT,"1")
		JSOM_PRINTF(FLOAT,"1")
		JSOM_PRINTF(FLOAT,"0.75")
		JSOM_PRINT(ARRAY_END)
	JSOM_PRINT(OBJECT_END)
	JSOM_PRINT(ARRAY_END)

//materials: //MHM
	
	JSOM_PRINTF(KEY,"materials")
	JSOM_PRINT(ARRAY_BEGIN)
	JSOM_PRINT(OBJECT_BEGIN)
	JSOM_PRINTF(KEY,"texture")
	JSOM_PRINT(ARRAY_BEGIN)
	JSOM_PRINTF(INT,"0")
	JSOM_PRINT(ARRAY_END)		
	JSOM_PRINT(OBJECT_END)
	JSOM_PRINT(ARRAY_END)

//graphics: //MHM
	
	JSOM_PRINTF(KEY,"graphics")
	JSOM_PRINT(ARRAY_BEGIN)		
	JSOM_PRINT(OBJECT_BEGIN)
	{
		JSOM_PRINTF(KEY,"sides")
		JSOM_PRINT(ARRAY_BEGIN)
		JSOM_PRINTF(INT,"0")
		JSOM_PRINT(NULL)
		JSOM_PRINT(ARRAY_END)		
		JSOM_PRINTF(KEY,"primitives")
		JSOM_PRINT(ARRAY_BEGIN)
		JSOM_PRINTF(INT,"0")
		JSOM_PRINT(ARRAY_END)

		jsom_clearance<float> clear(3);

		clear << hd.vertcount << *vp;

		//TODO: JSOM_PRINTF(KEY,"state")

		print_clearance(clear,-1,+1,-1);
	}
	JSOM_PRINT(OBJECT_END)
	JSOM_PRINT(ARRAY_END)

//listing: //MHM
	
	JSOM_PRINTF(KEY,"listing")	
	JSOM_PRINT(ARRAY_BEGIN)
	{
		JSOM_PRINT(OBJECT_BEGIN)
		//JSOM_PRINTF(KEY,"html")
		//JSOM_PRINTF(STRING,"")
		JSOM_PRINT(OBJECT_END)    
	}
	JSOM_PRINT(ARRAY_END)
	
//closure: 

	JSOM_PRINT(OBJECT_END)

	return mhm::unmap(in); 
}

static bool print_jsomap_external
(
 jsom_imageinfo &info, jsomap_format_t jsomap_f, std::string &inout, size_t seek=0
)
{	if(jsomap_f==jsomap_bmp) JSOM_DEBUGF("Buffering Microsoft bitmap (BMP) image.\n")	
	if(jsomap_f==jsomap_txr) JSOM_DEBUGF("Buffering Sword of Moonlight TXR image.\n")
	if(jsomap_f==jsomap_tim) JSOM_DEBUGF("Buffering Sword of Moonlight TIM image.\n")

	jsomap_t *in = jsomap_f(fd,seek); 
	
	const char *md5 = sum_md5(in); if(!md5) jsomap_free(in);

	if(in&&md5) 
	{
		info.alpha = in->alpha;	  
		info.height = in->imgbytes/in->rowbytes; 
		info.width = in->rowbytes/(in->palette?1:3);

		size_t pow2[] = {1,2,4,8,16,32,64,128,256,512},i,j; //hack

		for(i=0;i<sizeof(pow2);i++) if((size_t)info.width<=pow2[i]) break; 
		for(j=0;j<sizeof(pow2);j++) if((size_t)info.height<=pow2[j]) break; 

		info.width = pow2[i]; info.height = pow2[j];
	}
	else return 1; //error status
		
	inout = std::string("tex_")+salt+md5+".png";

	const char *png = inout.c_str();

	JSOM_DEBUGF("New Portable Network Graphic (PNG) job initiated: %s\n",png)

	if(external_exists_or_error(png))
	{
		JSOM_DEBUGF("Job terminated. Freeing image buffer.\n")

		png = 0; //free without write
	}
	
	status = jsomap_free(in,png); 

	JSOM_DEBUGF("PNG job %s: %s\n",status?"failure":"completed",inout.c_str())

	return status?false:true;
}

static bool print_msm_external(msm::image_t &in, std::string &inout, int n, msm::polyinfo_t pinfo[JSOM_MDO_TEX_LIMIT+1])
{
	if(!in) return false; 
		
	msm::vertices_t &vrts = msm::vertices(in);
	msm::polygon_t *poly0 = msm::firstpolygon(in);

	msm::polyinfo_t &ptotal = pinfo[JSOM_MDO_TEX_LIMIT];
	
	if(inout=="0") //arrays
	{		
		if(externals) 
		{		
		//WARNING: assuming msm after texture header 32bit aligned...

		    //compile if uin8_t is of char type
			if(hex_md5(MD5((uint8_t*)in.set,4*(in.end-in.set),0),'f'))
			{
				inout = std::string("msm_")+salt+hex_md5()+".json"+(gzip?".gz":"");
				
				//todo: returning true here is a little too ambiguous

				if(!print_job(inout.c_str())) return true; 
			}
			else return false;

			//.json wrapper
			JSOM_PRINT(OBJECT_BEGIN)
			JSOM_PRINTF(KEY,"arrays")
			JSOM_PRINT(ARRAY_BEGIN)
		}

		//attributes
		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"name")
		JSOM_PRINTF(STRING,"vertices")
		JSOM_PRINTF(KEY,"data")
		JSOM_PRINT(ARRAY_BEGIN)
		
		float *p = vrts.list[0].pos;
		for(int i=0;i<vrts.count*8;i++)
		{
			switch(i%8) //SOM_MAP flips XZ axes
			{
			case 0: case 3: //case 2: case 5:

				JSOM_PRINTF(FLOAT,"%f",-*p++); break;

			default: JSOM_PRINTF(FLOAT,"%f",*p++);
			}
		}

		JSOM_PRINT(ARRAY_END)
		JSOM_PRINT(OBJECT_END)

		//elements
		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"name")
		JSOM_PRINTF(STRING,"triangles")
		JSOM_PRINTF(KEY,"index")
		JSOM_PRINT(TRUE)
		JSOM_PRINTF(KEY,"data")
		JSOM_PRINT(ARRAY_BEGIN)

		//TODO: could easily stream around a fixed size buffer
		msm::triple_t *q = new msm::triple_t[ptotal.triangles];

		for(int i=0;i<n;i++)
		{
			pinfo[i] = //triangulate polygons per texture
			msm::triangulatepolygons(poly0,ptotal.polygons,q,ptotal.triangles,1<<i);
			 			
			if(!in.test(msm::polystack)) //something has gone "horribly" wrong 
			{
				fprintf(stderr,"MSM: Corruption detected inside msm::polystack.\n");				
			}
			else for(int j=0;j<pinfo[i].triangles;j++)
			{					
				//SOM_MAP flips XZ axes
				JSOM_PRINTF(INT,"%d",q[j][1]);
				JSOM_PRINTF(INT,"%d",q[j][0]);
				JSOM_PRINTF(INT,"%d",q[j][2]);
			}
		}

		delete[] q;

		JSOM_PRINT(ARRAY_END)
		JSOM_PRINT(OBJECT_END)

		if(externals) 
		{
			//.json wrapper 
			JSOM_PRINT(ARRAY_END)
			JSOM_PRINT(OBJECT_END)
			
			return complete_job(inout.c_str());
		}
	}
	else return false; return true;
}
static int main_msm()
{
	JSOM_DEBUGF("Entering main_msm()\n")

	msm::image_t in; msm::maptofile(in,0,fd);

	msm::textures_t &tex = msm::textures(in);
	msm::vertices_t &vrts = msm::vertices(in);

	//1 per possible texture + 1 for totals
	msm::polyinfo_t pinfo[JSOM_MDO_TEX_LIMIT+1];

	memset(pinfo,0x00,sizeof(pinfo)); //paranoia

	pinfo[JSOM_MDO_TEX_LIMIT] = msm::polyinfo(in);

	in.test(vrts.count); //require some vertices
	in.test(pinfo[JSOM_MDO_TEX_LIMIT].triangles); 

	msm::reference_t refs[JSOM_MDO_TEX_LIMIT];

	int refs_s = msm::texturereferences(in,refs,JSOM_MSM_TEX_LIMIT);

	JSOM_DEBUGF("MSM image %s\n",!in?"was BAD...":"looks good.")

	if(!in) msm::unmap(in);

	int n = std::min<int>(std::min<int>(tex.count,refs_s),JSOM_MSM_TEX_LIMIT);

	JSOM_PRINT(OBJECT_BEGIN)
		
//externals: //MSM	
	
	std::string external_0 = "0";

	JSOM_PRINTF(KEY,"externals")
	JSOM_PRINT(ARRAY_BEGIN)	

	if(externals)
	if(print_msm_external(in,external_0,n,pinfo))
	{
		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"json")
		JSOM_PRINTF(STRING,"%s%s",prefix,external_0.c_str())
		JSOM_PRINT(OBJECT_END)	
	}		
	JSOM_PRINT(ARRAY_END)
	
//images: //MSM

	JSOM_PRINTF(KEY,"images")
	JSOM_PRINT(ARRAY_BEGIN)
	
	for(int i=0;i<n;i++)
	{
		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"ref")
		JSOM_PRINTF(STRING,"tex%d",i)
		JSOM_PRINT(OBJECT_END)
	}

	JSOM_PRINT(ARRAY_END)

//arrays: //MSM
	
	JSOM_PRINTF(KEY,"arrays")
	JSOM_PRINT(ARRAY_BEGIN)
	
	if(externals)
	{		
		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"url")
		JSOM_PRINTF(INT,"0")
		JSOM_PRINT(OBJECT_END)
	}
	else print_msm_external(in,external_0,n,pinfo);
	
	JSOM_PRINT(ARRAY_END)

//attributes: //MSM

	JSOM_PRINTF(KEY,"attributes")
	JSOM_PRINT(ARRAY_BEGIN)
	{
		print_attribute("position",3,8,0);
		print_attribute("normal",3,8,3);
		print_attribute("texcoord0",2,8,6);
	}
	JSOM_PRINT(ARRAY_END)

//elements: //MSM
		
	JSOM_PRINTF(KEY,"elements")
	JSOM_PRINT(ARRAY_BEGIN)

	for(int i=0,j=0;i<n;i++)
	{
		if(!pinfo[i].triangles) 
		{
			//assuming externals were not printed
			msm::polygon_t *p = msm::firstpolygon(in); 
			msm::polyinfo_t &ptotal = pinfo[JSOM_MDO_TEX_LIMIT];
			pinfo[i] = msm::triangulatepolygons(p,ptotal.polygons,0,ptotal.triangles,1<<i);
		}

		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"mode")
		JSOM_PRINTF(STRING,"TRIANGLES")
		JSOM_PRINTF(KEY,"array")
		JSOM_PRINTF(INT,"1")
		JSOM_PRINTF(KEY,"start")
		JSOM_PRINTF(INT,"%d",j)
		JSOM_PRINTF(KEY,"count")
		JSOM_PRINTF(INT,"%d",pinfo[i].triangles*3)
		JSOM_PRINT(OBJECT_END)

		j+=pinfo[i].triangles*3;
	}

	JSOM_PRINT(ARRAY_END)

//materials: //MSM
	
	JSOM_PRINTF(KEY,"materials")
	JSOM_PRINT(ARRAY_BEGIN)

	for(int i=0;i<n;i++)
	{	
		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"texture")
		JSOM_PRINT(ARRAY_BEGIN)
		JSOM_PRINTF(INT,"%d",i)
		JSOM_PRINT(ARRAY_END)		
		JSOM_PRINT(OBJECT_END)
	}

	JSOM_PRINT(ARRAY_END)
	
//graphics: //MSM
	
	JSOM_PRINTF(KEY,"graphics")
	JSOM_PRINT(ARRAY_BEGIN)
		
	JSOM_PRINT(OBJECT_BEGIN)

	jsom_clearance<float> clear(8);

	clear << vrts.count << vrts.list[0].pos;

	//TODO: JSOM_PRINTF(KEY,"state")

	print_clearance(clear,-1,+1,-1);

	//filing under subgraphics
	JSOM_PRINTF(KEY,"graphics")
	JSOM_PRINT(ARRAY_BEGIN)

	for(int i=0;i<n;i++)
	{
		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"sides")
		JSOM_PRINT(ARRAY_BEGIN)
		JSOM_PRINTF(INT,"%d",i)
		JSOM_PRINT(NULL)
		JSOM_PRINT(ARRAY_END)		
		JSOM_PRINTF(KEY,"primitives")
		JSOM_PRINT(ARRAY_BEGIN)
		JSOM_PRINTF(INT,"%d",i)
		JSOM_PRINT(ARRAY_END)
		JSOM_PRINT(OBJECT_END)
	}

	JSOM_PRINT(ARRAY_END)
	JSOM_PRINT(OBJECT_END)
	JSOM_PRINT(ARRAY_END)

//listing: //MSM
	
	JSOM_PRINTF(KEY,"listing")	
	JSOM_PRINT(ARRAY_BEGIN)
	{
		JSOM_PRINT(OBJECT_BEGIN)
		//JSOM_PRINTF(KEY,"html")
		//JSOM_PRINTF(STRING,"")
		JSOM_PRINT(OBJECT_END)    
	}
	JSOM_PRINT(ARRAY_END)
	
//closure: 

	JSOM_PRINT(OBJECT_END)

	return msm::unmap(in); 
}

static bool print_mdo_external(mdo::image_t &in, std::string &inout)
{
	if(!in) return false; 

	if(inout=="0") //arrays
	{	
		mdo::channels_t &ch = mdo::channels(in);

		if(!ch||!ch.count) return false;

		if(externals) 
		{				
			void *sum = mdo::tnlvertextriples(in,0);
			void *eof = mdo::tnlvertexpackets(in,-1)+ch[-1].vertcount;

			size_t sum_s = size_t(eof)-size_t(sum);

			if(in<=(int8_t*)sum+sum_s) return false;
			
			 //compile if uin8_t is of char type
			if(hex_md5(MD5((uint8_t*)sum,sum_s,0),'f'))
			{
				inout = std::string("mdo_")+salt+hex_md5()+".json"+(gzip?".gz":"");
				
				//todo: returning true here is a little too ambiguous

				if(!print_job(inout.c_str())) return true; 
			}
			else return false;

			//.json wrapper
			JSOM_PRINT(OBJECT_BEGIN)
			JSOM_PRINTF(KEY,"arrays")
			JSOM_PRINT(ARRAY_BEGIN)
		}

		//attributes
		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"name")
		JSOM_PRINTF(STRING,"vertices")
		JSOM_PRINTF(KEY,"data")
		JSOM_PRINT(ARRAY_BEGIN)

		for(unsigned i=0;i<ch.count;i++)
		{
			float *p = (float*)mdo::tnlvertexpackets(in,i);

			if(p) for(int j=0,n=ch[i].vertcount*8;j<n;j++)
			{
				JSOM_PRINTF(FLOAT,"%f",*p++);
			}
		}

		JSOM_PRINT(ARRAY_END)
		JSOM_PRINT(OBJECT_END)

		//elements
		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"name")
		JSOM_PRINTF(STRING,"triangles")
		JSOM_PRINTF(KEY,"index")
		JSOM_PRINT(TRUE)
		JSOM_PRINTF(KEY,"data")
		JSOM_PRINT(ARRAY_BEGIN)

		int start = 0;
		for(unsigned i=0;i<ch.count;i++)
		{
			uint16_t *p = (uint16_t*)mdo::tnlvertextriples(in,i);

			if(p) for(int j=0,n=ch[i].ndexcount;j<n;j++)
			{
				JSOM_PRINTF(INT,"%d",start+*p++);
			}

			start+=ch[i].vertcount;
		}

		JSOM_PRINT(ARRAY_END)
		JSOM_PRINT(OBJECT_END)

		if(externals) 
		{
			//.json wrapper 
			JSOM_PRINT(ARRAY_END)
			JSOM_PRINT(OBJECT_END)
			
			return complete_job(inout.c_str());
		}
	}
	else return false;

	return true;
}

static int main_mdo()
{
	JSOM_DEBUGF("Entering main_mdo()\n")

	mdo::image_t in; mdo::maptofile(in,0,fd);

	mdo::textures_t &tex = mdo::textures(in);
	mdo::materials_t &mat = mdo::materials(in);
	mdo::channels_t &chan = mdo::channels(in);

	mdo::reference_t refs[JSOM_MDO_TEX_LIMIT];

	int refs_s = mdo::texturereferences(in,refs,JSOM_MDO_TEX_LIMIT);

	JSOM_DEBUGF("MDO image %s\n",!in?"was BAD...":"looks good.")

	if(!in) return mdo::unmap(in);

	JSOM_PRINT(OBJECT_BEGIN)
		
//externals: //MDO	
	
	std::string external_0 = "0";

	JSOM_PRINTF(KEY,"externals")
	JSOM_PRINT(ARRAY_BEGIN)	

	if(externals)
	if(print_mdo_external(in,external_0))
	{
		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"json")
		JSOM_PRINTF(STRING,"%s%s",prefix,external_0.c_str())
		JSOM_PRINT(OBJECT_END)	
	}		
	JSOM_PRINT(ARRAY_END)
	
//images: //MDO

	JSOM_PRINTF(KEY,"images")
	JSOM_PRINT(ARRAY_BEGIN)
	
	for(int i=0;i<refs_s;i++)
	{
		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"ref")
		JSOM_PRINTF(STRING,"tex%d",i)
		JSOM_PRINT(OBJECT_END)
	}

	JSOM_PRINT(ARRAY_END)

//arrays: //MDO
	
	JSOM_PRINTF(KEY,"arrays")
	JSOM_PRINT(ARRAY_BEGIN)
	
	if(externals)
	{		
		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"url")
		JSOM_PRINTF(INT,"0")
		JSOM_PRINT(OBJECT_END)
	}
	else print_mdo_external(in,external_0);
	
	JSOM_PRINT(ARRAY_END)

//attributes: //MDO

	JSOM_PRINTF(KEY,"attributes")
	JSOM_PRINT(ARRAY_BEGIN)
	{
		print_attribute("position",3,8,0);
		print_attribute("normal",3,8,3);
		print_attribute("texcoord0",2,8,6);
	}
	JSOM_PRINT(ARRAY_END)

//elements: //MDO
		
	JSOM_PRINTF(KEY,"elements")
	JSOM_PRINT(ARRAY_BEGIN)

	for(unsigned i=0,n=0;i<chan.count;i++)
	{
		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"mode")
		JSOM_PRINTF(STRING,"TRIANGLES")
		JSOM_PRINTF(KEY,"array")
		JSOM_PRINTF(INT,"1")
		JSOM_PRINTF(KEY,"start")
		JSOM_PRINTF(INT,"%d",n)
		JSOM_PRINTF(KEY,"count")
		JSOM_PRINTF(INT,"%d",chan[i].ndexcount)
		JSOM_PRINT(OBJECT_END)

		n+=chan[i].ndexcount;
	}

	JSOM_PRINT(ARRAY_END)

//materials: //MDO
	
	std::vector<std::pair<int,int> >materials;

	JSOM_PRINTF(KEY,"materials")
	JSOM_PRINT(ARRAY_BEGIN)

	for(unsigned i=0;i<tex.count;i++)
	for(unsigned j=0;j<mat.count;j++)
	for(unsigned k=0;k<chan.count;k++)
	if(chan[k].texnumber==i&&chan[k].matnumber==j)
	{	
		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"texture")
		JSOM_PRINT(ARRAY_BEGIN)
		JSOM_PRINTF(INT,"%d",i)
		JSOM_PRINT(ARRAY_END)
		JSOM_PRINTF(KEY,"diffuse")
		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"rgb")
		 JSOM_PRINT(ARRAY_BEGIN)
		 JSOM_PRINTF(FLOAT,"%f",mat[j].r)
		 JSOM_PRINTF(FLOAT,"%f",mat[j].g)
		 JSOM_PRINTF(FLOAT,"%f",mat[j].b)
		 JSOM_PRINTF(FLOAT,"%f",mat[j].a)
		 JSOM_PRINT(ARRAY_END)
		JSOM_PRINT(OBJECT_END)
		JSOM_PRINTF(KEY,"emissive")
		JSOM_PRINT(OBJECT_BEGIN)
		 JSOM_PRINTF(KEY,"rgb")
		 JSOM_PRINT(ARRAY_BEGIN)
		 JSOM_PRINTF(FLOAT,"%f",mat[j].r2)
		 JSOM_PRINTF(FLOAT,"%f",mat[j].g2)
		 JSOM_PRINTF(FLOAT,"%f",mat[j].b2)
		 JSOM_PRINTF(FLOAT,"%f",mat[j]._)
		 JSOM_PRINT(ARRAY_END)
		JSOM_PRINT(OBJECT_END)		
		JSOM_PRINT(OBJECT_END)

		materials.push_back(std::pair<int,int>(i,j));

		break;
	}

	JSOM_PRINT(ARRAY_END)
	
//graphics: //MDO
	
	JSOM_PRINTF(KEY,"graphics")
	JSOM_PRINT(ARRAY_BEGIN)

	for(int i=materials.size()-1;i>=0;i--)
	{
		int m = materials[i].first;
		int n = materials[i].second;

		for(int j=0,modes=1;j<modes;j++)
		{	
			bool moonlight = chan[0].blendmode;

#ifdef _DEBUG //just scouting for new blendmodes
			
			if(moonlight) assert(chan[0].blendmode==1);
#endif
			if(j) moonlight = !moonlight;
	
			int pass = moonlight||mat[n].a<1.0?1:0;

			JSOM_PRINT(OBJECT_BEGIN)
			JSOM_PRINTF(KEY,"pass") //1: transparent
			JSOM_PRINTF(INT,"%d",pass)
			JSOM_PRINTF(KEY,"sides")
			JSOM_PRINT(ARRAY_BEGIN)
			JSOM_PRINTF(INT,"%d",i)
			JSOM_PRINT(NULL)
			JSOM_PRINT(ARRAY_END)
			
			JSOM_PRINTF(KEY,"primitives")
			JSOM_PRINT(ARRAY_BEGIN)

			jsom_clearance<float> clear(8);
			
			for(unsigned k=0;k<chan.count;k++) 
			if(chan[k].texnumber==m&&chan[k].matnumber==n)
			{
				//is there more than one kind?
				if(moonlight!=chan[k].blendmode) 
				{
					modes = 2; continue; //must be
				}
				
				clear << chan[k].vertcount << mdo::tnlvertexpackets(in,k)->pos;

				JSOM_PRINTF(INT,"%d",k)
			}

			JSOM_PRINT(ARRAY_END)

			print_clearance(clear);
			
			if(pass==1)
			{
				JSOM_PRINTF(KEY,"blend")
				JSOM_PRINT(ARRAY_BEGIN)
				if(moonlight) JSOM_PRINTF(STRING,"ONE") 
				if(moonlight) JSOM_PRINTF(STRING,"ONE") 
				if(!moonlight) JSOM_PRINTF(STRING,"SRC_ALPHA")
				if(!moonlight) JSOM_PRINTF(STRING,"ONE_MINUS_SRC_ALPHA")				
				JSOM_PRINT(ARRAY_END)
			}

			JSOM_PRINT(OBJECT_END)

		}
	}

	JSOM_PRINT(ARRAY_END)

//listing: //MDO
	
	JSOM_PRINTF(KEY,"listing")	
	JSOM_PRINT(ARRAY_BEGIN)
	{
		JSOM_PRINT(OBJECT_BEGIN)
		//JSOM_PRINTF(KEY,"html")
		//JSOM_PRINTF(STRING,"")
		JSOM_PRINT(OBJECT_END)    
	}
	JSOM_PRINT(ARRAY_END)
	
//closure: 

	JSOM_PRINT(OBJECT_END)

	return mdo::unmap(in); 
}

static bool print_mdl_external(mdl::image_t &in, std::string &inout,
							   mdl::triangle_t *t, int tris, int verts)
{	if(!in) return false; 
	
	bool complete = true;

	std::string inout_2; //animations

	int ok = 0, vbsize = 0; mdl::vbuffer_t vb;
	
	mdl::header_t &head = mdl::imageheader(in);

	int anims = head.hardanims+head.softanims;

	if(inout=="0") //arrays
	{	
		if(externals) 
		{							
			size_t primandanimwords = (size_t)
			head.primchanwords+head.hardanimwords+head.softanimwords;
			
			if(in<=primandanimwords) return false;

			//compile if uin8_t is of char type
			if(hex_md5(MD5((uint8_t*)in.set,4*primandanimwords,0),'f'))
			{
				inout = std::string("mdl_")+salt+hex_md5()+".json"+(gzip?".gz":"");
				inout_2 = std::string("mdl_")+salt+hex_md5()+"_2.json"+(gzip?".gz":"");
				
				//todo: returning true here is a little too ambiguous

				if(!print_job(inout.c_str(),inout_2.c_str())) 
				{
					return true; 
				}
			}
			else return false;

			//.json wrapper
			JSOM_PRINT(OBJECT_BEGIN)
		}
		 		
		if(anims&&externals)
		{
			JSOM_PRINTF(KEY,"externals")
			JSOM_PRINT(ARRAY_BEGIN)
			JSOM_PRINT(OBJECT_BEGIN)
				JSOM_PRINTF(KEY,"json") 
				JSOM_PRINTF(STRING,"%s%s",prefix,inout_2.c_str())
				JSOM_PRINTF(KEY,"priority")
				JSOM_PRINTF(INT,"2")
			JSOM_PRINT(OBJECT_END)
			JSOM_PRINT(ARRAY_END)

			JSOM_PRINTF(KEY,"keyframes")
			JSOM_PRINT(ARRAY_BEGIN)
			JSOM_PRINT(OBJECT_BEGIN)
				JSOM_PRINTF(KEY,"url") 
				JSOM_PRINTF(INT,"0") 
			JSOM_PRINT(OBJECT_END)
			JSOM_PRINT(ARRAY_END)

			JSOM_PRINTF(KEY,"keygroups")
			JSOM_PRINT(ARRAY_BEGIN)
			JSOM_PRINT(OBJECT_BEGIN)
				JSOM_PRINTF(KEY,"url") 
				JSOM_PRINTF(INT,"0")
			JSOM_PRINT(OBJECT_END)
			JSOM_PRINT(ARRAY_END)
		}
				
		ok = mdl::newvertexbuffer(vb,verts);

		int tris2 = 0, verts2 = 0;
		for(int i=0;i<head.primchans;i++)
		{
			mdl::vertex_t *pos =  mdl::pervertexlocation(in,i);
			mdl::normal_t *lit =  mdl::pervertexlighting(in,i);

			int primcounts[16], n = mdl::countprimitives(in,i,primcounts,16);
			mdl::priminfo_t pinfo = mdl::priminfo(mdl::displayprimset,primcounts,n);

			ok = mdl::fillvertexbuffer(vb,ok,t+tris2,pinfo.triangles,verts2,pos,lit); 

			verts2+=mdl::primitivechannel(in,i).vertcount;							
			tris2+=pinfo.triangles; 		
		}

		vbsize = mdl::sizeofvertexbuffer(vb,ok);

		complete = in&&tris==tris2&&vbsize>=verts;

		JSOM_PRINTF(KEY,"arrays")
		JSOM_PRINT(ARRAY_BEGIN)

		//attributes
		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"name")
		JSOM_PRINTF(STRING,"vertices")
		JSOM_PRINTF(KEY,"data")
		JSOM_PRINT(ARRAY_BEGIN)

		if(complete) for(int i=0;i<vbsize;i++)
		{
		JSOM_PRINTF(FLOAT,"%f",-mdl::m*vb[i].pos[0])
		JSOM_PRINTF(FLOAT,"%f",-mdl::m*vb[i].pos[1])
		JSOM_PRINTF(FLOAT,"%f",+mdl::m*vb[i].pos[2]) 		
		JSOM_PRINTF(FLOAT,"%f",-mdl::n*vb[i].lit[0])
		JSOM_PRINTF(FLOAT,"%f",-mdl::n*vb[i].lit[1])
		JSOM_PRINTF(FLOAT,"%f",+mdl::n*vb[i].lit[2])

		//note: this requires a texture matrix setup
		JSOM_PRINTF(FLOAT,"%f",float(vb[i].uvs[0])/256.0f)
		JSOM_PRINTF(FLOAT,"%f",float(vb[i].uvs[1])/256.0f)
		}					  
		
		JSOM_PRINT(ARRAY_END)
		JSOM_PRINT(OBJECT_END)
				
		//elements
		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"name")
		JSOM_PRINTF(STRING,"triangles")
		JSOM_PRINTF(KEY,"index")
		JSOM_PRINT(TRUE)
		JSOM_PRINTF(KEY,"data")
		JSOM_PRINT(ARRAY_BEGIN)

		if(complete) for(int k=0;k<tris;k++)
		{
			JSOM_PRINTF(INT,"%d",t[k].pos[1]);
			JSOM_PRINTF(INT,"%d",t[k].pos[0]);
			JSOM_PRINTF(INT,"%d",t[k].pos[2]);
		}

		JSOM_PRINT(ARRAY_END)
		JSOM_PRINT(OBJECT_END)
		
		if(externals) 
		{
			if(head.softanims)
			{					
				JSOM_PRINT(OBJECT_BEGIN)
				JSOM_PRINTF(KEY,"url") 
				JSOM_PRINTF(INT,"0") 
				JSOM_PRINT(OBJECT_END)				
			}
			JSOM_PRINT(ARRAY_END) //arrays

			//.json wrapper 
			JSOM_PRINT(OBJECT_END)
			
			complete = complete_job(inout.c_str(),complete);

			if(!anims) return complete;
		}
		else if(!head.softanims||!complete)
		{
			JSOM_PRINT(ARRAY_END) //arrays
		}
	}
	else return false;

	if(inout_2.size()&&anims&&complete) //animations
	{
		if(externals) 
		{							
			//paranoia: should never hold true
			if(!print_job(inout_2.c_str())) return true; 				

			//.json wrapper
			JSOM_PRINT(OBJECT_BEGIN)

			if(head.softanims)
			{
			JSOM_PRINTF(KEY,"arrays")
			JSOM_PRINT(ARRAY_BEGIN)
			}
		}

		int soft0 = head.hardanims;

		mdl::animation_t *animations = new mdl::animation_t[anims];

		if(anims&&complete)
		complete = anims==mdl::animations(in,animations,anims);

		int totalunique = 0, *uniqueframes = 0;
		int totalframes = mdl::softanimationframes(in);		

		if(totalframes&&complete)
		{
			uniqueframes = new int[totalframes];
			totalunique = mdl::softanimationframes(in,0,uniqueframes,totalframes);

			complete = in&&totalunique>0;
		}

		if(totalunique&&complete)
		{
			mdl::vbuffer_t accum = 0;

			std::vector<bool> seen(totalunique+1);

			seen[totalunique] = true; //initial frame

			//lazy: assuming everything is kosher...
			mdl::softanimframe_t *p = animations[soft0]->frames;
			
			JSOM_PRINT(OBJECT_BEGIN)
			JSOM_PRINTF(KEY,"name")
			JSOM_PRINTF(STRING,"tweenpos")
			JSOM_PRINTF(KEY,"data")
			JSOM_PRINT(ARRAY_BEGIN)
		
			int frame = 0;
			for(int i=0;i<totalframes;i++) 
			{					
				//TODO: iterate over animations instead

				mdl::softanim_t *ch = &mdl::softanimchannel(in,p);
				ok = mdl::copyvertexbuffer(accum,frame==0?vb:0,ok,ch);
				 
				if(ok&&!seen[uniqueframes[i]])
				{
					seen[uniqueframes[i]] = true;

					for(int j=0;j<vbsize;j++)
					{
					JSOM_PRINTF(FLOAT,"%f",-mdl::m*accum[j].pos[0])
					JSOM_PRINTF(FLOAT,"%f",-mdl::m*accum[j].pos[1])
					JSOM_PRINTF(FLOAT,"%f",+mdl::m*accum[j].pos[2]) 					
					}
				}
				
				if(*++p) frame++; else frame = 0;

				//hack: hop over 0 terminator/next ID
				if(frame==0) p+=2;
			}
			
			JSOM_PRINT(ARRAY_END)
			JSOM_PRINT(OBJECT_END)
			
			complete = in&&ok;
		}

		if(externals&&head.softanims)
		{
			JSOM_PRINT(ARRAY_END) //arrays
		}
	
		if(anims&&complete)
		{	
			int firstanimationgroup = 0;
			
			if(head.hardanims) firstanimationgroup+=head.primchans;
			if(head.softanims) firstanimationgroup+=1;

			int softgraphicsgroup = firstanimationgroup-1;

			int keys = 0;						
			std::vector<int> animationtimes; animationtimes.reserve(anims);
			std::vector<std::vector<int> >groups(firstanimationgroup+anims);			

			JSOM_PRINTF(KEY,"keyframes")
			JSOM_PRINT(ARRAY_BEGIN)
			
			int hardchans = 
			mdl::animationchannels<mdl::hardanim_t>(in);

			if(head.hardanims&&hardchans)
			{	
				//NOTE: This data would probably be a lot less bulky
				//if the transforms / still frames are in local space.
				//Would need to arrange the graphics skeletally to see.

				mdl::hardanim_t *p = new mdl::hardanim_t[hardchans];

				if(hardchans==mdl::mapanimationchannels(in,p,hardchans))
				{
					float scale[4] = {-mdl::m,-mdl::m,+mdl::m,1};

					int m = hardchans, n = head.primchans;

					float *mats = new float[m*16]; bool *knowns = new bool[m];

					for(int i=0;i<head.hardanims;i++)
					{	
						int time = animations[i]->time;

						for(int j=0;j<time;j++)
						{							
							mdl::animate(animations[i],p,m,j,!i);

							memset(knowns,0x00,sizeof(bool)*m);
							
							//first frame is not used
							if(j) for(int k=0;k<n;k++)
							{
								mdl::transform(p,k,0,scale,mats,knowns);
			
								JSOM_PRINT(OBJECT_BEGIN)

								JSOM_PRINTF(KEY,"timerange")
								JSOM_PRINT(ARRAY_BEGIN)
								JSOM_PRINTF(INT,"%d",j)
								JSOM_PRINTF(INT,"%d",j+1)
								JSOM_PRINT(ARRAY_END)

								JSOM_PRINTF(KEY,"graphics")
								JSOM_PRINT(ARRAY_BEGIN)
								JSOM_PRINT(OBJECT_BEGIN) 
								JSOM_PRINTF(KEY,"state")
								{
									JSOM_PRINT(ARRAY_BEGIN)
									for(int l=0;l<16;l++)
									JSOM_PRINTF(FLOAT,"%f",mats[16*k+l])									
									JSOM_PRINT(ARRAY_END)
								}
								JSOM_PRINT(OBJECT_END)
								JSOM_PRINT(ARRAY_END)
								
								JSOM_PRINT(OBJECT_END)

								groups[firstanimationgroup+i].push_back(keys);
								groups[k].push_back(keys++); 
							}

						}					

						animationtimes.push_back(time);
					}

					delete[] knowns;
					delete[] mats; 
				}

				delete[] p;
			}

			if(head.softanims) //redundant
			for(int i=head.hardanims,t=0,s=0;i<anims;i++) 
			{
				int time = 0;
				int len = mdl::softanimframestrlen(animations[i]->frames);				

				for(int j=0;j<len;j++,s=t++)
				{
					JSOM_PRINT(OBJECT_BEGIN)

					JSOM_PRINTF(KEY,"timerange")
					JSOM_PRINT(ARRAY_BEGIN)
					JSOM_PRINTF(INT,"%d",time)
					time+=animations[i]->frames[j].time;
					JSOM_PRINTF(INT,"%d",time)					
					JSOM_PRINT(ARRAY_END)

					JSOM_PRINTF(KEY,"graphics")
					JSOM_PRINT(ARRAY_BEGIN)
					JSOM_PRINT(OBJECT_BEGIN) 
					{
					int strides[2] = {0,0}, starts[2] = {0,0};
					
					if(!j //first frame is bind pose
					 ||uniqueframes[s]==totalunique) strides[0] = 8;
					if(uniqueframes[t]==totalunique) strides[1] = 8;
					
					if(strides[0]!=8) starts[0] = vbsize*3*uniqueframes[s];
					if(strides[1]!=8) starts[1] = vbsize*3*uniqueframes[t];

					JSOM_PRINTF(KEY,"attributes")
					JSOM_PRINT(ARRAY_BEGIN)
					print_attribute("position",3,strides[0],starts[0],strides[0]?0:2);					
					print_attribute("tweenpos",3,strides[1],starts[1],strides[1]?0:2);
					JSOM_PRINTF(INT,"1") //normals
					JSOM_PRINTF(INT,"2") //texcoords
					JSOM_PRINT(ARRAY_END)
					}
					JSOM_PRINT(OBJECT_END) 
					JSOM_PRINT(ARRAY_END)

					groups[firstanimationgroup+i].push_back(keys);
					groups[softgraphicsgroup].push_back(keys++); 

					JSOM_PRINT(OBJECT_END)
				}

				animationtimes.push_back(time);
			}

			JSOM_PRINT(ARRAY_END) //keyframes
			
			JSOM_PRINTF(KEY,"keygroups")
			JSOM_PRINT(ARRAY_BEGIN)
			
			for(int i=0,m=groups.size();i<m;i++)
			{
				JSOM_PRINT(OBJECT_BEGIN)
				JSOM_PRINTF(KEY,"keyframes")
				JSOM_PRINT(ARRAY_BEGIN)
				for(int j=0,n=groups[i].size();j<n;j++)
				{
					JSOM_PRINTF(INT,"%d",groups[i][j])
				}
				JSOM_PRINT(ARRAY_END)
				if(i>=firstanimationgroup)
				{
					int time = 
					animationtimes[i-firstanimationgroup];

					JSOM_PRINTF(KEY,"timeframe")
					JSOM_PRINT(ARRAY_BEGIN)
					JSOM_PRINTF(INT,"1")
					JSOM_PRINTF(INT,"%d",time)
					JSOM_PRINT(ARRAY_END)				
					JSOM_PRINTF(KEY,"timescale")
					JSOM_PRINTF(FLOAT,"%f",1.0/30)
				}				
				JSOM_PRINT(OBJECT_END)
			}			
			
			JSOM_PRINT(ARRAY_END) //keygroups
		}

		delete[] uniqueframes;
		delete[] animations;

		if(externals) 
		{
			//.json wrapper 
			JSOM_PRINT(OBJECT_END)
			
			complete = complete_job(inout_2.c_str(),complete);
		}
	}

	mdl::deletevertexbuffer(vb);

	return complete;
}

static int main_mdl()
{
	JSOM_DEBUGF("Entering main_mdl()\n")

	mdl::image_t in; mdl::maptofile(in,0,fd);

	JSOM_DEBUGF("MDL image %s\n",!in?"was BAD...":"looks good.")

	if(!in) return mdl::unmap(in);

	mdl::header_t &head = mdl::imageheader(in);
		
	int textures = std::min<int>(head.timblocks,JSOM_MDL_TEX_LIMIT);

	char framebuffer[32]; memset(framebuffer,0x00,sizeof(framebuffer));

	//first: number of triangles
	//second: 1st 24 TMD bits | ch<<8 (hack)
	std::vector<std::pair<int,int> >elements;
	
	//note: assuming mutual exclusivity
	assert(!head.hardanims||!head.softanims);
	int keys = head.hardanims+head.softanims;

	const int cp_s = 32; int cp[1+cp_s*3] = {0};

	mdl::triangle_t cp_tris[cp_s]; jsom_clearance<int16_t> clear(4,cp);

	int tmp = 0, total = 0;		   
	for(int i=0;i<head.primchans;i++)	
	{
		int primcounts[16], n = mdl::countprimitives(in,i,primcounts,16);
		mdl::priminfo_t pinfo = mdl::priminfo(mdl::displayprimset,primcounts,n);

		tmp = std::max(pinfo.triangles,tmp); total+=pinfo.triangles;
	}
	
	mdl::triangle_t *triangles = new mdl::triangle_t[total+tmp];
	
	int tris = 0, verts = 0;
	for(int i=0;i<head.primchans;i++)
	{
		mdl::primch_t &chan = mdl::primitivechannel(in,i);	

		*cp = 3*mdl::triangulateprimitives(in,i,mdl::controlprimset,cp_tris,cp_s);

		for(int j=1,k,l=0;j<=*cp;j+=3,l++) 
		{
			//include cyan control point in clearance
			bool cyan = cp_tris[l].rgb[1]==255&&cp_tris[l].rgb[2]==255&&cp_tris[l].rgb[0]==0;

			for(k=0;k<3;k++) cp[j+k] = cyan?-1:cp_tris[l].pos[k]; 			
		}

		clear << chan.vertcount << mdl::pervertexlocation(in,i)->xyz;
	
		int n = mdl::triangulateprimitives(in,i,mdl::displayprimset,triangles+total,tmp);

		int abr[4]; mdl::trianglematsort(triangles+tris,triangles+total,n,abr);

		const unsigned int mask = mdl::tpnbits|mdl::abrbits|mdl::abebits;
		
		int cmp = 0; mdl::triangle_t *p = triangles+tris;

		for(int j=0;j<n;j++)
		if((p[j].tmd.bits&mask)!=(p[cmp].tmd.bits&mask))
		{
			elements.push_back(std::pair<int,int>(j-cmp,p[cmp].tmd.bits&mask|i<<8)); cmp = j; 			
		}		

		if(cmp!=n) elements.push_back(std::pair<int,int>(n-cmp,p[cmp].tmd.bits&mask|i<<8)); 

		verts+=chan.vertcount; tris+=n;		
	}

	JSOM_DEBUGF("There were %d vertices in total.\n",verts)

	if(tris!=total)	//paranoia
	{
		delete[] triangles; return(mdl::unmap(in),1);
	}

	JSOM_PRINT(OBJECT_BEGIN)

//externals: //MDL	
	
	std::string external_0 = "0";	
	std::string external_n[JSOM_MDL_TEX_LIMIT];

	jsom_imageinfo image_n[JSOM_MDL_TEX_LIMIT];
	
	int ext = //top heavy: arrays keyframes & keygroups...
	print_mdl_external(in,external_0,triangles,tris,verts)?1:0;

	JSOM_PRINTF(KEY,"externals")
	JSOM_PRINT(ARRAY_BEGIN)	

	if(ext&&externals)
	{
		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"json")
		JSOM_PRINTF(STRING,"%s%s",prefix,external_0.c_str())
		JSOM_PRINT(OBJECT_END)	
	}	
	if(externals) 
	for(int i=0;i<textures;i++)
	{
		tim::image_t tex; mdl::maptotimblock(tex,in,i);		

		size_t seek = tex.header<char>()-in.header<char>();

		if(print_jsomap_external(image_n[i],jsomap_tim,external_n[i],seek))
		{
			JSOM_PRINT(OBJECT_BEGIN)
			JSOM_PRINTF(KEY,"png")
			JSOM_PRINTF(STRING,"%s%s",prefix,external_n[i].c_str())
			JSOM_PRINT(OBJECT_END)	

			framebuffer[tim::tpn(tim::data(tex))] = i;
		}

		tim::unmap(tex);
	}
	JSOM_PRINT(ARRAY_END)
	
//arrays/keyframes/keygroups: //MDL
		
	if(externals)
	for(int i=0;i<(keys?3:1);i++)
	{
		const char *turn = "arrays";

		if(i) turn = i==1?"keyframes":"keygroups";

		JSOM_PRINTF(KEY,turn)
		JSOM_PRINT(ARRAY_BEGIN)
		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"url")
		JSOM_PRINTF(INT,"0")
		JSOM_PRINT(OBJECT_END)
		JSOM_PRINT(ARRAY_END)
	}

//images: //MDL

	JSOM_PRINTF(KEY,"images")
	JSOM_PRINT(ARRAY_BEGIN)

	if(0==textures)
	{
		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"rgb")
		JSOM_PRINT(ARRAY_BEGIN)
		JSOM_PRINTF(FLOAT,"%f",0.0f)
		JSOM_PRINTF(FLOAT,"%f",1.0f)
		JSOM_PRINTF(FLOAT,"%f",1.0f)
		JSOM_PRINT(ARRAY_END)
		JSOM_PRINT(OBJECT_END)
	}
	else for(int i=0;i<textures;i++)
	{	
		if(!image_n[i]) continue;

		JSOM_PRINT(OBJECT_BEGIN)
				
		if(!externals) 
		{
			JSOM_PRINTF(KEY,"rgb")
			JSOM_PRINT(ARRAY_BEGIN)
			JSOM_PRINTF(FLOAT,"%f",!i||i!=1?0.8f:0.0f)
			JSOM_PRINTF(FLOAT,"%f",!i||i!=3?0.8f:0.0f)
			JSOM_PRINTF(FLOAT,"%f",!i||i!=2?0.8f:0.0f)
			JSOM_PRINT(ARRAY_END)
		}
		else
		{
			JSOM_PRINTF(KEY,"url")
			JSOM_PRINTF(INT,"%d",ext)	

			print_imageinfo(image_n[i]);

			if(image_n[i].width!=256||image_n[i].height!=256)
			{
			JSOM_PRINTF(KEY,"scale")
			JSOM_PRINT(ARRAY_BEGIN)
			JSOM_PRINTF(FLOAT,"%f",256.0f/image_n[i].width)		
			JSOM_PRINTF(FLOAT,"%f",256.0f/image_n[i].height)
			JSOM_PRINT(ARRAY_END)
			}
		}

		JSOM_PRINT(OBJECT_END)

		ext++;
	}

	JSOM_PRINT(ARRAY_END)

//attributes: //MDL

	JSOM_PRINTF(KEY,"attributes")
	JSOM_PRINT(ARRAY_BEGIN)
	{
		print_attribute("position",3,8,0);
		print_attribute("normal",3,8,3);
		print_attribute("texcoord0",2,8,6);
	}
	JSOM_PRINT(ARRAY_END)

//elements: //MDL
		
	JSOM_PRINTF(KEY,"elements")
	JSOM_PRINT(ARRAY_BEGIN)

	int m = elements.size();
	for(int i=0,n=0;i<m;i++)
	{
		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"mode")
		JSOM_PRINTF(STRING,"TRIANGLES")
		JSOM_PRINTF(KEY,"array")
		JSOM_PRINTF(INT,"1")
		JSOM_PRINTF(KEY,"start")
		JSOM_PRINTF(INT,"%d",n)
		JSOM_PRINTF(KEY,"count")
		JSOM_PRINTF(INT,"%d",elements[i].first*3)
		JSOM_PRINT(OBJECT_END)

		n+=elements[i].first*3;
	}

	JSOM_PRINT(ARRAY_END)

//semantics: //MDL

	if(head.softanims)
	{
	JSOM_PRINTF(KEY,"semantics")
	JSOM_PRINT(ARRAY_BEGIN)
	JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"position")
		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"glsl")
		JSOM_PRINTF(STRING,"vec4(mix(position,tweenpos,tween),1)")
		JSOM_PRINT(OBJECT_END)
		//optimize away state multiplication
		JSOM_PRINTF(KEY,"state0(X)")
		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"glsl")
		JSOM_PRINTF(STRING,"X")
		JSOM_PRINT(OBJECT_END)
	JSOM_PRINT(OBJECT_END)
	JSOM_PRINT(ARRAY_END)
	}

//materials: //MDL
	
	JSOM_PRINTF(KEY,"materials")
	JSOM_PRINT(ARRAY_BEGIN)

	int texture = 0;
	if(0==textures) goto texture;
	while(texture<textures) texture:
	{	
		JSOM_PRINT(OBJECT_BEGIN)
		if(head.softanims)
		{
		JSOM_PRINTF(KEY,"semantics")
		JSOM_PRINT(ARRAY_BEGIN)
		JSOM_PRINTF(INT,"0")
		JSOM_PRINT(NULL)
		JSOM_PRINT(ARRAY_END)
		}
		JSOM_PRINTF(KEY,"texture")
		JSOM_PRINT(ARRAY_BEGIN)
		JSOM_PRINTF(INT,"%d",texture++)
		JSOM_PRINT(ARRAY_END)		
		JSOM_PRINT(OBJECT_END)
	}

	JSOM_PRINT(ARRAY_END)
	
//graphics: //MDL
	
	JSOM_PRINTF(KEY,"graphics")
	JSOM_PRINT(ARRAY_BEGIN)
		
	JSOM_PRINT(OBJECT_BEGIN)

  //impractical to do clearance per graphic

    float s = 1.0f/1000.0f;
	print_clearance(clear,-s,-s,+s);

	//filing under subgraphics
	JSOM_PRINTF(KEY,"graphics")
	JSOM_PRINT(ARRAY_BEGIN)

	for(int i=0;i<m;i++)
	{
		int pass = elements[i].second&mdl::abebits?1:0;

		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"pass") //1: transparent
		JSOM_PRINTF(INT,"%d",pass)
		JSOM_PRINTF(KEY,"sides")
		JSOM_PRINT(ARRAY_BEGIN)
		JSOM_PRINTF(INT,"%d",framebuffer[elements[i].second&mdl::tpnbits])
		JSOM_PRINT(NULL)
		JSOM_PRINT(ARRAY_END)
		
		JSOM_PRINTF(KEY,"primitives")
		JSOM_PRINT(ARRAY_BEGIN)		
		JSOM_PRINTF(INT,"%d",i)
		JSOM_PRINT(ARRAY_END)

		if(pass==1) 
		{
			int abr = elements[i].second>>5&3;
			
			//PlayStation blend modes:
			//00  50%back + 50%polygon
			//01 100%back + 100%polygon
			//10 100%back - 100%polygon 
			//11 100%back + 25%polygon

			if(abr!=1) JSOM_PRINTF(KEY,"opacity")			
			if(abr==0) JSOM_PRINTF(FLOAT,"0.5") 
			if(abr==2) JSOM_PRINTF(FLOAT,"-1.0") 
			if(abr==3) JSOM_PRINTF(FLOAT,"0.25") 

			JSOM_PRINTF(KEY,"blend")
			JSOM_PRINT(ARRAY_BEGIN)
			if(abr==1) JSOM_PRINTF(STRING,"ONE") //src
			if(abr!=1) JSOM_PRINTF(STRING,"SRC_ALPHA") //src
			if(abr==0) JSOM_PRINTF(STRING,"ONE_MINUS_SRC_ALPHA") //dest
			if(abr!=0) JSOM_PRINTF(STRING,"ONE") //dest
			JSOM_PRINT(ARRAY_END)
		}

		if(keys)
		{
		JSOM_PRINTF(KEY,"keygroups")
		JSOM_PRINT(ARRAY_BEGIN)				
		if(head.hardanims) JSOM_PRINTF(INT,"%d",elements[i].second>>8&0xFF) //hack
		if(head.softanims) JSOM_PRINTF(INT,"%d",head.hardanims?head.primchans:0)
		JSOM_PRINT(ARRAY_END)
		}

		JSOM_PRINT(OBJECT_END)
	}

	JSOM_PRINT(ARRAY_END)
	JSOM_PRINT(OBJECT_END)
	JSOM_PRINT(ARRAY_END)

//tracks: //MDL
	
	JSOM_PRINTF(KEY,"tracks")
	JSOM_PRINT(ARRAY_BEGIN)	
	{		
		int key0 = head.softanims?1:0;				
		if(head.hardanims) key0+=head.primchans; 

		mdl::animation_t aniIDs[16];		  
		int ids = mdl::animations(in,aniIDs,16);

		for(int i=0;i<keys;i++)
		{
			JSOM_PRINT(OBJECT_BEGIN)
			JSOM_PRINTF(KEY,"key")
			JSOM_PRINT(ARRAY_BEGIN)		
			JSOM_PRINTF(INT,"%d",key0+i)
			JSOM_PRINT(ARRAY_END)		
			if(i<ids) JSOM_PRINTF(KEY,"title")
			if(i<ids) JSOM_PRINTF(STRING,"Animation ID #%d",aniIDs[i]->id)
			JSOM_PRINT(OBJECT_END)  
		}
	}
	JSOM_PRINT(ARRAY_END)

//listing: //MDL
	
	JSOM_PRINTF(KEY,"listing")	
	JSOM_PRINT(ARRAY_BEGIN)
	{
		JSOM_PRINT(OBJECT_BEGIN)
		//JSOM_PRINTF(KEY,"html")
		//JSOM_PRINTF(STRING,"")
		JSOM_PRINT(OBJECT_END)    
	}
	JSOM_PRINT(ARRAY_END)
	
//closure: 
	
	JSOM_PRINT(OBJECT_END)

	delete[] triangles;

	return mdl::unmap(in); 
}

static int print_jsomap(jsomap_format_t format)
{			  
	JSOM_PRINT(OBJECT_BEGIN)
		
//externals: //BMP/TXR
		
	std::string external_0 = "0";

	jsom_imageinfo image_0;

	JSOM_PRINTF(KEY,"externals")
	JSOM_PRINT(ARRAY_BEGIN)	

	//if(externals) //TODO: replace with rgb?
	if(print_jsomap_external(image_0,format,external_0))
	{
		JSOM_PRINT(OBJECT_BEGIN)
		JSOM_PRINTF(KEY,"png")
		JSOM_PRINTF(STRING,"%s%s",prefix,external_0.c_str())
		JSOM_PRINT(OBJECT_END)	
	}		
	JSOM_PRINT(ARRAY_END)
	
//images: //BMP/TXR

	JSOM_PRINTF(KEY,"images")
	JSOM_PRINT(ARRAY_BEGIN)
	JSOM_PRINT(OBJECT_BEGIN)
	JSOM_PRINTF(KEY,"url")
	JSOM_PRINTF(INT,"0")  

	if(image_0)	print_imageinfo(image_0);

	JSOM_PRINT(OBJECT_END)
	JSOM_PRINT(ARRAY_END)
	JSOM_PRINT(OBJECT_END)

	return 0;
}

static int main_txr()
{
	JSOM_DEBUGF("Entering main_txr()\n")

	return print_jsomap(jsomap_txr);
}

static int main_bmp()
{
	JSOM_DEBUGF("Entering main_bmp()\n")
		
	return print_jsomap(jsomap_bmp);
}

static int main_prt()
{
	/*PRT JSOM schema reference
	{	 
	 "graphics"
	 [
	  {"ref":"model"}
	 ],

	 "listing"
	 [
	  {"html":"textual file info etc..."},
	 ],
	}
	*/

	return 1; //unimplemented
}

static int main_prf()
{	
	/*PRF JSOM schema reference
	{	 
	 "graphics"
	 [
	  {"ref":"model"}
	 ],
	 
	 "tracks"
	 [
	  //not clear what to do here//
	 ],
	 
	 "listing"
	 [
	  {"html":"textual file info etc..."},
	 ],
	}
	*/

	return 1; //unimplemented
}
