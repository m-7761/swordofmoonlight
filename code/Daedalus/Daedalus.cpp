
#include "Daedalus.(c).h"

#ifdef _WIN32
#include "getopt.h"
#endif	
//'cli', 'gui'
static int operation = 0;
//commandline options
//(inherited from JSOM)
static int verbose = 0;
static int overwrite = 0;
static int externals = 1;
static char *file = "";
static char *directory = 0;
static char *overwrite2 = 0;
static char *prefix = "";
static char *salt = "";
static bool gzip = false;
//preservation modules
static char *prelib = 0;
static char *predia = "";
static char *presrc = "";  
#ifdef NDEBUG
#error write me (this is not JSOM)
#endif
static char *version = "Pre-Alpha";
static char *usage = "For usage: run command with --help.";
static char *help = "\n\
Usage: Daedalus [OPERATION] [OPTIONS] [INPUT FILE]\n\
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
	 '--cli' command-line interface model\n\
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
'-z, --gzip' Compression. Output is filtered through gzip\n\
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
int main(int argc, char *argv[]) 
{	
	struct option long_options[] = 
	{    	
	//JSOM inherited options
	//format based operations
	{ "cli", 0, &operation, 'cli' },
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
	//PreServer cli options
	{ "prelib",1,0,'pre'+'lib' },
	{ "predia",1,0,'pre'+'dia' },
	{ "presrc",1,0,'pre'+'src' },
	/*terminating 0 item*/
	{ 0, 0, 0, 0 } 
    };		 	
	for(int opt,long_opt_index=0;
	(opt=getopt_long(argc,argv,"f:hvVzC:",long_options,&long_opt_index))!=-1;)
	{
		switch(opt)
		{
		//case 0: //this getopt is misbehaving??? 
		//
		//	switch(long_options[long_opt_index].val)
		//	{
			case 1: break; //--overwrite

			case 2: //--overwrite-if-older-than

				overwrite2 = optarg; break;

			case 'pre'+'lib': prelib = optarg; break;
			case 'pre'+'dia': predia = optarg; break;
			case 'pre'+'src': presrc = optarg; break;
		//	}
		//	break;

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
			
	    default:
			
			printf("%s is not a recognized option. %s",optarg,usage);
			return 0;
	   }
	} 
	int err = 0; //scheduled obsolete
	if(prelib) //hand off to PreServe.cpp
	{		
		using namespace Daedalus;
		//not worth using PoolString here/could be an attack vector
		Daedalus::post post(preServer(prelib,predia,presrc,0,0),0); 
		if(PreServe(&post)) if(post.proceed())
		{
			if(!*file) file = 
			strdup((std::string(presrc)+=".dae").c_str());			
			typedef COLLADA_1_5_0::COLLADA COLLADA;
			if(!CollaDAPP(CollaDB.newDoc<COLLADA>(file),post.Scene()))
			err = fprintf(stderr,"Collada/DAE data entry failed unexpectedly\n");
			else if(!strcmp("-",file))			
			err = fprintf(stderr,"Write to stdout is not supported at this time\n");			
			else if(!CollaDB.write())		
			err = fprintf(stderr,"The Collada DOM library could not write to file\n");			
			//clear prior to exit()
			CollaDB.clear_of_content();
		}else err = fprintf(stderr,"post-PreServe step ended in failure. See earlier ouptut for details\n");
		else err = fprintf(stderr,"PreServe ended in failure. See earlier ouptut for details\n");
	}return err?1:0;
}
#ifdef _WIN32
int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
	#ifdef _DEBUG //compiler
	_set_error_mode(_OUT_TO_MSGBOX); 
	//_CRTDBG_LEAK_CHECK_DF floods output on debugger termination
	_crtDbgFlag = _CRTDBG_ALLOC_MEM_DF; //|_CRTDBG_LEAK_CHECK_DF;
	//_crtDbgFlag|=_CRTDBG_CHECK_ALWAYS_DF; //careful
	#endif	

	int argc = 0; //cannot use LPWSTR without hacking into getopt
	wchar_t **argw = CommandLineToArgvW(GetCommandLineW(),&argc);	
	char **argv = new char*[argc];
	std::vector<char> buffer(MAX_PATH);
	for(int i=0;i<argc;argv[i++]=_strdup(buffer.data())) top:
	{
		WideCharToMultiByte(CP_UTF8,0,argw[i],-1,buffer.data(),buffer.size(),0,0);
		if(GetLastError()!=ERROR_INSUFFICIENT_BUFFER) continue;
		buffer.resize(2*buffer.size()); goto top;
	}AttachConsole(GetCurrentProcessId());
	UINT cp = GetConsoleOutputCP(); SetConsoleOutputCP(65001);
	int exit_code = main(argc,argv); SetConsoleOutputCP(cp);
	return exit_code;
}
#endif