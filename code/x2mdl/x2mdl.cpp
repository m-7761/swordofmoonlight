
#include "x2mdl.pch.h" //PCH

#include <vector>
#include <string>
#include <regex> //C++11
#include <unordered_map> //C++11

#pragma comment(lib,"shlwapi.lib")

//Added for snapshot transform
#include "../lib/swordofmoonlight.h"

#include "x2mdl.h" //x2mdl_h //2021

const aiScene *X = nullptr, *Y = nullptr;

/*I don't understand this
the player's model matrix
is scaled by 1/1000 (1024 is from the PlayStation)
float scale = 1000; //to millimeters (nope)*/
float scale = 1024; 

static const short invert[3] = {+1,-1,-1};

int exit_status = 0;
const wchar_t *exit_reason = 0;

#include "cpgen.cpp" //2021: x2mdl.dll
#include "../SomEx/SoftMSM.cpp" //2022

const wchar_t *i18n_inputfilefailure =
L"No input was provided. Aborting.\r\n";

const wchar_t *i18n_externaltexturefailure =
L"A texture file could not be located or failed to load.\r\n"
L" Unable to determine dimensions of texture. Aborting.\r\n";

const wchar_t *i18n_direct3dfailure = 
L"Direct3D failed to load.\r\n"
L" Very likely Direct3D 9 is not available on this computer.\r\n";

const wchar_t *i18n_direct3dgeneralfailure = 
L"Direct3D failed in a general way.\r\n"
L" Very likely an interface reported something other than success.\r\n";

const wchar_t *i18n_assimpfailure = 
L"The Assimp library was unable to load the specified input.\r\n";

//const wchar_t *i18n_assimpmemoryfailure = 
//L"The Assimp library reported that it ran out of memory.\r\n";

const wchar_t *i18n_assimpgeneralfailure = 
L"The Assimp library failed or behaved in an unexpected way.\r\n"
L" Sorry, this is all the information you get.\r\n";

const wchar_t *i18n_unabletocomplete = 
L"X2mdl is unable to complete because of incompatible input.\r\n";

const wchar_t *i18n_unimplemented = 
L"Woops! X2mdl stumbled upon some unimplemented block of code (to be).\r\n"
L" This is of course not supposed to happen. Continue at your own risk.\r\n";

const wchar_t *i18n_compressedtexunsupported =
L"At least one embedded texture appears to be compressed.\r\n"
L" Decompression of this type is currently unsupported.\r\n"
L" Unable to determine dimensions of texture. Aborting.\r\n";

const wchar_t *i18n_materialsoverflow = //2021 
L"A model has more than 32 materials. This is more than there is memory.\r\n";

const wchar_t *i18n_generalwritefailure = 
L"X2mdl is unable to save a file somewhere on this system.\r\n";

LRESULT CALLBACK DummyWinProc(HWND,UINT,WPARAM,LPARAM)
{
	return 1;
};
HWND MakeHiddenWindow()
{
	WNDCLASSEXA dummy;

	memset(&dummy,0x00,sizeof(WNDCLASSEX));
	
	dummy.cbSize = sizeof(WNDCLASSEX);	
	dummy.lpfnWndProc = DummyWinProc;	
	dummy.lpszClassName = "dummy";
				   
	dummy.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);

	RegisterClassExA(&dummy);

	HWND wohs = CreateWindowA("dummy","X2MDL (offscreen)",0,-1,-1,-1,-1,0,0,0,0);

	if(!wohs) exit_status = 1;
	
	return wohs;
}

//this is just for testing theories
inline aiMatrix4x4 q2mat(aiQuaternion q, aiVector3D pos)
{		
	aiMatrix4x4 out(q.GetMatrix()); out.a4 = pos.x; out.b4 = pos.y; out.c4 = pos.z; return out;
}

//this is just for testing theories (seems buggy)
//http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToAngle/index.htm
inline void q2aa(aiQuaternion q, aiVector3D &x, float &v)
{
	if(q.w>1) q.Normalize();

	v = 2.0f*acosf(q.w);

	float s = sqrtf(1.0f-q.w*q.w);

	if(s<0.001) 
	{ 
		x.x = q.x; x.y = q.y; x.z = q.z;
	} 
	else 
	{
		x.x = q.x/s; x.y = q.y/s; x.z = q.z/s;
	}
}

bool integer(const wchar_t *string)
{
	if(*string=='-') string++;

	while(*string) if(*string<'0'||*string>'9') return false; else string++; return true;
}

inline int ani2i(const aiString &ani, int def)
{
	int a = 0;

	if(ani.data[ani.length-1]==')')
	{
		int i = ani.length-2;

		while(i>=0&&ani.data[i]!='(') i--;

		if(i<0||ani.data[i]!='('||ani.data[i+1]!='#') return def;

		a = i+2;

		if('-'==ani.data[a]) a++; //EXPERIMENTAL //2023
	}
		
	if(ani.data[a]<'0'||ani.data[a]>'9') return def;
		
	return atoi(ani.data+a);
}

inline aiVector3D q2xyz(const aiQuaternion &q)
{
	aiVector3D xyz;
	Assimp::EulerAnglesFromQuaternion<+1,1,2,3>(xyz,q);
	return xyz;
}

int setmask(int *mask, int sizeinbits, int set)
{
	if(set>sizeinbits)
	{
		assert(0); return 0;
	}

	int i = set/32; set-=i*32;

	//previous setting returned
	int out = mask[i]&1<<set?1:0;

	mask[i]|=1<<set; return out;
}

int getmask(int *mask, int sizeinbits, int get)
{
	if(get>sizeinbits) return 0;

	int i = get/32; get-=i*32;

	return mask[i]&1<<get?1:0;
}

int count(int *mask, int sizeinbits, int lo=0, int hi=-1)
{
	assert(sizeinbits%32==0);

	if(hi==-1) hi = sizeinbits;

	//count number of set bits in mask between lo and hi

	int out = 0, octets = sizeinbits/8;

	unsigned char *p = (unsigned char*)mask;

	for(int i=0;i<octets;i++)
	{
		int l = lo<i*8?0:(lo>i*8+8?8:lo-i*8); if(l==8) continue;		
		int h = hi<i*8?0:(hi>i*8+8?8:hi-i*8); if(h==0) continue;

		if(*p!=0xFF)
		{
			for(int j=l;j<h;j++) if((1<<j)&*p) out++;			
		}
		else out+=h-l;

		p++;
	}

	return out;
}

int count(aiNode *n)
{
	int out = 1; 
	for(int i=0;i<n->mNumChildren;i++)
	out+=count(n->mChildren[i]);
	return out;
}

int flatten(aiNode **flat, int sz, aiNode *n, int at = 0)
{
	if(at>=sz) return at;

	//flat[at++] = n; 

	for(int i=0;i<n->mNumChildren;i++) 
	{
		at = flatten(flat,sz,n->mChildren[i],at);	
	}
	flat[at++] = n; //ARM.MDL order (tail recursion)

	return at;
}

void quicksort(aiString **sort, int *refs, int sz, int lo, int hi)
{
	if(lo>=hi) return; 
	
	aiString *sswap = 0; int iswap = 0;
    	
	int r = rand(); r = lo<hi?lo+r%(hi-lo):hi+r%(lo-hi);

	sswap = sort[lo]; sort[lo] = sort[r]; sort[r] = sswap;
	iswap = refs[lo]; refs[lo] = refs[r]; refs[r] = iswap;

	int pivot = lo; aiString *p = sort[pivot];
	
	for(int unknown=pivot+1;unknown<=hi;unknown++) //...

	if(strcmp(sort[unknown]->data,p->data)<0)
	{   		
		pivot++; 
		
		sswap = sort[unknown]; sort[unknown] = sort[pivot]; sort[pivot] = sswap; 
		iswap = refs[unknown]; refs[unknown] = refs[pivot]; refs[pivot] = iswap; 
	}

	sswap = sort[lo]; sort[lo] = sort[pivot]; sort[pivot] = sswap; 
	iswap = refs[lo]; refs[lo] = refs[pivot]; refs[pivot] = iswap;

	if(pivot) quicksort(sort,refs,sz,lo,pivot-1); 
	
	if(pivot<=sz) quicksort(sort,refs,sz,pivot+1,hi);
}

void quicksort(aiNode **flat, aiString **sort, int *refs, int sz)
{
	for(int i=0;i<sz;i++) //TODO: randomize
	{
		sort[i] = &flat[i]->mName; refs[i] = i;
	}

	quicksort(sort,refs,sz,0,sz-1);
}

static bool //REMOVE US
tmd = false, 
tmd_1034 = false,
mm3d = false,
mdo = false, mdl = false,
msm = false, mhm = false;
int ico = 0;
enum{ ico_mipmaps=0 };
extern int max_anisotropy = 0;
int binlookup(const aiString &lookup, aiString **sorted, int sz)
{
	int cmp, first = 0, last = sz-1, stop = -1;

	int x; for(x=(last+first)/2;x!=stop;x=(last+first)/2)
	{
		cmp = strcmp(sorted[x]->data,lookup.data); if(!cmp) return x; 

		if(cmp>0) stop = last = x; else stop = first = x;
	}

	if(last==sz-1&&x==last-1) //round-off error
	{
		if(!strcmp(sorted[last]->data,lookup.data)) return last;
	}
	
	//NOTE: O384.MDL hits this, maybe because CP channels 
	//are removed, but it's called <Ani ch3> instead of a
	//(R0) style control point
	assert(mm3d&&'('==*lookup.data); return -1; //0 //woops: bogus catch all
}

int primitive(aiMesh *mesh, unsigned int face)
{
	if(mesh->mPrimitiveTypes!=aiPrimitiveType_TRIANGLE)
	{
		if(3!=mesh->mFaces[face].mNumIndices)
		return 0x00;
	}

	if(mesh->mColors[0])
	if(2!=mesh->mNumUVComponents[0]) //2022: UVs? (TMD)
	{
		unsigned int *q = mesh->mFaces[face].mIndices;

		aiColor4D c = mesh->mColors[0][q[0]]; 
		if(mesh->mColors[0][q[0]].a==1.0f&&c!=aiColor4D(1,1,1,1)
		&&c==mesh->mColors[0][q[1]]&&c==mesh->mColors[0][q[2]]
		/*&&!is_qnan(c.r)&&!is_qnan(c.g)&&!is_qnan(c.b)*/)
		{
			/*som_db.exe has to hide the primitive pack
			* routines as an optimization. 3 isn't treated
			* as a CP (ignored) so it's not worth hiding
			* more than 4 for the moment
			if(mesh->mNormals[q[0]]==mesh->mNormals[q[1]]
			 &&mesh->mNormals[q[0]]==mesh->mNormals[q[2]])
			return 0x00; 
			return 0x03; //???*/
			return 0x00;
		}
	}
	
	return 0x04; //default
}
bool primitive2(unsigned char (&o)[4], char *p)
{		
	int compile[sizeof(o)==sizeof(int)];
	*(int*)o = 0;	
	for(int i=1;i-->0;*(int*)o=0)
	{
		if(*p++!='(') continue; //i.e. "(R0)"
								
		auto *e = p;

		char P = toupper(*p);

		if(P=='C'&&p[1]=='0')
		{
			o[1] = o[2] = 255; //CYAN
									
			e = p+2;
		}
		else if(P=='R')
		{
			o[0] = 255; green: 
									
			long green = strtol(p+1,&e,10);

			if(green>=0&&green<=255)
			{
				o[1] = 0xff&green;
			}
			else continue;
		}
		else if(P=='P') //2022: x2msm?
		{
			o[0] = o[2] = 255; //PURPLE

			goto green;
		}
		else
		{
			int i = 0; do
			{
				o[i++] = strtol(p=e,&e,10);

			}while(')'!=*e&&','==*e++&&i<3);

			if(i!=3) continue;
		}
								
		if(*e==')') //closure?
		{
			o[3] = 255; //black?

			break;
		}
	}
	return *(int*)o!=0;
}

template<class S, class T>
unsigned pushback(S* &buf, T &len, S cp, int n=1)
{
	S *swap = buf; buf = new S[len+n];
	memcpy(buf,swap,(len+n)*sizeof(S));
	for(int i=n;i-->0;) buf[len+i] = cp;
	//YUCK: nullify ~Part destructor
	memset(swap,0x00,len*sizeof(S));
	delete[] swap; len+=n; return len-n;
}

enum{ hardn_fps=30*2, modern=1 }; //1||mm3d

int hardn(int i)
{
	//2023: increasing this to 60 and setting 0x10/16 flag
	//NOTE: this flag may also optimize the deltas' layout

	auto anim = X->mAnimations[i];
	int n = !modern+anim->mDuration/anim->mTicksPerSecond*hardn_fps;
	if(!i&&modern) n++; 
			
	//2022: SomLoader.cpp is now subtracting 1 from the duration
	//which som_db.exe throws out... it needs to be put back now
	//return n;
	return n+1;
};

enum{ cpmaterialsN=16+33 };
char texindex[cpmaterialsN]; //extern
WORD texmapsu[cpmaterialsN];
WORD texmapsv[cpmaterialsN]; //extern
typedef unsigned char Color[4];
Color controlpts[cpmaterialsN];
float materials[cpmaterialsN][4];
float materials2[cpmaterialsN][4]; 
const wchar_t *texnames_ext = 0;
std::unordered_map<std::string,int> texmap;
std::vector<std::wstring> texnames; //x2mm3d/mdo/etc.
std::vector<IDirect3DTexture9*> icotextures;

//YUCK: x2mdo needs this to be global
int snapshot = 0;
float *snapshotmats = 0;
float *bindposemats = 0;
aiNode **nodetable = 0;
char *chanindex = 0; 

#ifdef _CONSOLE
extern bool x2mm3d_lerp;			
//extern void x2mm3d_lerp_prep();
std::vector<aiNode*> x2mm3d_nodetable(0);
std::vector<std::pair<int,int>> x2mm3d_mats(0);
void x2mm3d_convert_points(aiMesh*,aiNode*);
#endif

//NOTE: X2MDL_API can't define these on site
void x2mdl_pt2tri(short*,short*,short*,short*);
bool x2mdo_txr(int,int,int,void*,const wchar_t*);
void x2mdl_animate(aiAnimation*,aiMatrix4x4*,float);
void x2mdo_split_XY();

extern bool x2msm_ico(int,WCHAR*,IDirect3DDevice9*,IDirect3DSurface9*[3]);
extern bool x2mdo_ico(int,WCHAR*,IDirect3DDevice9*,IDirect3DSurface9*[3]);

extern HWND X2MDL_MODAL = 0;
#ifdef _CONSOLE
int wmain(int argc, const wchar_t* argv[]) //wmain
#else
X2MDL_API 
int x2mdl(int argc, const wchar_t* argv[], HWND hwnd) //DLL?
#endif
{		
	std::vector<short> bindposeshot; //goto

	X2MDL_MODAL = 0;
	#ifdef _CONSOLE
	int in = 1; //x2mdl.exe?
	#else
	int in = 6; //x2mdl.dll?
	assert(argc>in);
	x2mdl_dll dll;
	{
		dll.cache_dir = argv[1];
		dll.image_suf = argv[2];	
		if(dll.image_suf&&*dll.image_suf)
		{
			assert(dll.image_suf[0]=='.');
		}
		else dll.image_suf = 0;
		//argv[3]; //reserved
		//argv[4]; //reserved
		//argv[5]; //reserved (0-separator?)
		dll.data_end = dll.data_begin = argv+in;
		while(in<argc&&PathIsDirectoryW(argv[in]))
		{
			dll.data_end++; in++;
		}
		
		dll.progress = 0; if(hwnd)
		{
			//NOTE: there's no way to retrieve control class
			//atoms via strings. only RegisterClass can make
			//such atoms
			char cn[32];
			if(GetClassNameA(hwnd,cn,sizeof(cn)))
			if(!strcmp(cn,"msctls_progress32"))
			{
				dll.progress = hwnd;
			}
		}

		dll.link = 0;
		if(CoCreateInstance(CLSID_ShellLink,0,CLSCTX_INPROC_SERVER,IID_IShellLinkW,(LPVOID*)&dll.link)
		||dll.link->QueryInterface(IID_IPersistFile,(void**)&dll.link2))
		{
			dll.link = 0; assert(0);
		}

		X2MDL_MODAL = hwnd;
	}
	#endif
	if(!X2MDL_MODAL) 
	{
		//YUCK: MessageBoxW(0) is "modeless"
		//X2MDL_MODAL = GetForegroundWindow();
		X2MDL_MODAL = GetActiveWindow();
	}

	using SWORDOFMOONLIGHT::mdl::round;

	static const float r2fpi = (float)(2048/AI_MATH_PI);

	#ifdef _CONSOLE
	if(argc<=1) help: //2019: trying to be more user-friendly this time around?
	{
	//	L"Convert X or some other model file formats to Sword of Moonlight's\n"
	//	L"MDL or other model file formats. This is intentionally vague since\n"
	//	L"this software isn't officially published. A program named cpgen is\n"
	//	L"needed to produce a CP file at this time. CP is for control-points.\n"
			//REMOVE THESE
	//	L"-x-abe=<NUM> 0 or 1 value to enable experimental blending override.\n"
	//	L"-x-abr=<NUM> 0,1,2,3 blending mode to use with -x-abe blend option.\n"

		std::wcout <<
		L"Sword of Moonlight: King's Field Making Tool\n"
		L"Since 2022 this program has three main uses (so far) that are switched\n"
		L"between by renaming its EXE file or using mklink to form a dummy clone.\n"
		L"x2mm3d mode makes an https://github.com/mick-p1982/mm3d MM3D file that\n"
		L"supports all of SOM's needs. The remaining ways aren't normally needed.\n"
		L"x2mdl mode generates a set of runtime model formats: MDL, MDO, CP & BP.\n"
		L"SOM can do this by itself now, so this is more of a niche option. Then\n"
		L"x2mdo mode just generates MDO files, much like the original x2mdo tool\n"
		L"only with other input formats and more optimized outputted model files.\n"
		L"  NOTES: This version of this program uses an old version of Assimp to\n"
		L"  load a lot of model formats to some extent, but full operation needs\n"
		L"  at least one way to represent control-points. Please request formats.\n"
		L"\n"
		L"Usage: [OPTION(S)] [POSE] INPUT(S)\n"
		L"\n"
		L"OPTION(S) and POSE can appear anywhere affecting subsequent inputs.\n"
		L"\n"
		L"POSE Numeric ID of animation to use to change skeleton for SOM_MAP.\n"
		L"\n"	
		L"--wipe-extension strips off the input file's extension before renaming.\n"
		L"--file-extension=[FORMAT] sets the format if not renaming the EXE file.\n"
		L"  NOTES: --wipe-extension is still required to prevent appending to it.\n"
		L"--update-texture forces textures to be outputted even if a file exists.\n"
		L"--prefix-texture=[PREFIX] prepends [PREFIX] to outputted texture names.\n"
		L"--apply-snapshot use snapshot [POSE] for bind pose.\n"
		L"--mdl-conversion=[MODE] changes how input MDL files are processed...\n"
		L"	MODES:\n"
		L"	-1: use 1 or 2 if file name looks like an obj MDL (default)\n"
		L"	0: make no changes (helpful to diagnose problems)\n"
		L"	1: keep first animation frame\n"
		L"	2: discard first animation frame\n"
		L"	3: old x2mdl compatibility mode for Debone processed files (Moratheia)\n"
		L"	4: can be for Node based files but I don't think any exist\n";
		L"--keep-materials disables a step that removes unused materials/textures.\n"
		L"  NOTES: --update-texture is needed to overwrite existing texture files.\n"
		L"--mm3d-lerp-keys outputs MM3D files with Lerp instead of Step keyframes.\n"
		L"	NOTES:\n"
		L"  Sword-of-Moonlight used a step model. This option seems to break loop\n"
		L"  around behavior and some animations aren't designed with lerp in mind\n"
		L"  but this option may be useful as a starting point.\n"
		L"--tmd-1034=1024 trims fringes from King's Field II level geometry tiles\n";
		L"--msm-subdivide fully tessellates input MSM files prior to a conversion\n";
		L"--ico=[25 or other Number] generate N x N icon for SOM_MAP or other use\n";
		return 0;
	}
	#endif

	#ifndef _CONSOLE
	mdl = mdo = true; //if(in==argc-1) //x2mdl.dll? //batch?
	{
		//2022: how to switch to MSM/MHM output???
		switch(argv[0][3])
		{
		default: assert(0); case 'd': msm = false;
			
			//WARNING: 100 is treated as not an icon
			//by Windows Explorer! maybe 128x128?
			//Note, 50 doesn't have defined bones for
			//skeleton (e105.mdl)

			ico = 96; //25 is a little too small

			assert(!wcscmp(argv[0],L"x2mdl.dll")); break; 

		case 's': msm = true; mhm = mdl = mdo = false; 
			
			ico = 25; //special value (21+2px border)

			assert(!wcscmp(argv[0],L"x2msm.dll")); break;
		}
	}
	#else
	const wchar_t *prog = PathFindFileNameW(argv[0]);
	//2020
	#if defined(X2MM3D_DEBUG) && defined(_DEBUG)
	mm3d = true;	
	#else
	mm3d = !wcsnicmp(prog,L"x2mm3d",6);
	#endif	
	//2021
	#if defined(X2MDO_DEBUG) && defined(_DEBUG)
	mdo = true;	
	#else
	mdo = !wcsnicmp(prog,L"x2mdo",5);
	#endif
	//2022
	#if defined(X2MSM_DEBUG) && defined(_DEBUG)
	msm = true;	
	#else
	msm = !wcsnicmp(prog,L"x2msm",5);
	#endif
	//#ifdef _DEBUG //TESTING
	{
		//explict x2mdo?
		mdl = mdo||msm?false:true;

		//x2mdl? output both?
		if(mdl&&!mm3d) mdo = true;

		if(mm3d) mdl = mdo = msm = false; //NEW
	}
	#endif

#ifdef _DEBUG
#ifdef _CONSOLE

	//Hmmm: have never needed this before??
	_set_error_mode(_OUT_TO_MSGBOX); 

#endif
#endif

	struct aiLogStream stream;

	// get a handle to the predefined STDOUT log stream and attach
	// it to the logging system. It will be active for all further
	// calls to aiImportFile(Ex) and aiApplyPostProcessing.
	stream = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT,NULL);
	aiAttachLogStream(&stream);

	// ... exactly the same, but this stream will now write the
	// log file to assimp_log.txt
	stream = aiGetPredefinedLogStream(aiDefaultLogStream_FILE,"assimp_log.txt");
	aiAttachLogStream(&stream);
	
	unsigned int postprocess = 
	aiProcess_JoinIdenticalVertices|
	aiProcess_GenUVCoords|
	aiProcess_RemoveRedundantMaterials|
	aiProcess_Debone;

	//hacks: development only 
	postprocess|=aiProcess_Triangulate; //in case of polygons
	postprocess|=aiProcess_GenNormals; //0x04 packets only for now

	//HACK! ComplexScene.cpp catches this
	//with this it's possible to reconstruct the original model's
	//vertices from the postprocess vertices
	//if there are 1D or 3D texture-coords the odd component must
	//be connectivity indices
	//
	// ATTENTION: this seems to be needed for deboning with MM3D
	// or else "isInterstitialRequired" comes up because of merged
	// vertices with the same attributes except for their origin
	//
	aiSetImportPropertyInteger("SP_GEN_CONNECTIVITY",1);
	
	#ifdef _CONSOLE
	{
		int i = modern?-1:0; //POINTLESS?
		//MODES:
		//-1: default to 1 or 2 if file name looks like an obj MDL
		//0: make no changes (bool like)
		//1: keep first animation frame (bool like)
		//2: default mode (discard first animation frame)
		//3: old x2mdl compatibility mode for Debone processed files
		//4: can be for Node based files but I don't think any exist
		char a[4];
		if(GetEnvironmentVariableA("SOM_MODERN_MDL_MODE",a,4))
		i = atoi(a);
		if(i!=-1) 
		aiSetImportPropertyInteger("SOM_MODERN_MDL_MODE",i);
	}
	#endif
	 	
	static bool d3d_available = false;
			
//	const D3DFORMAT rtf = D3DFMT_X8R8G8B8; //some drivers only do 888
	const D3DFORMAT rtf = D3DFMT_A8R8G8B8; //some drivers only do 888

	//TODO: I'd like to remove this completely to improve
	//performance with x2mdl.dll, however there is a small
	//problem of scaling images down to 512x512. originally
	//it converted to 16bpp for MDL (TIM)
	static IDirect3D9 *pd3D9 = 0; 
	static IDirect3DDevice9 *pd3Dd9 = 0;
	static IDirect3DSurface9 *rt = 0, *rs = 0, *ms = 0; //render target
	static IDirect3DStateBlock9 *sb = 0;
	static IDirect3DSurface9 *ds = 0;

	static HWND fake = MakeHiddenWindow(); //dll

	#ifdef _CONSOLE
	#define goto_0 goto _0;
	#define goto_1 goto _1;
	#else
	#define goto_0 goto dll_continue;
	#define goto_1 goto dll_continue;
	#endif

	//abr modes 
	//0: 50/50 blend
	//1: 100 additive (whiter)
	//2: 100 subtractive (funky)
	//3: 25 additive (fainter)
	//bool x_abe = false;
	//int abe = 0, abr = 0, x = 0;
	int x = 0;

	snapshot = 0; //-1; //_CONSOLE?

	bool has_snapshot = false; //2023
	int def_snapshot = 0; 

	const wchar_t *input = L""; //???

	#ifdef _CONSOLE
	const wchar_t *prefix_texture = 0;
	bool update_texture = false, wipe_extension = false;
	bool apply_snapshot = false;
	#else
	enum{ apply_snapshot=0 };
	#endif

	for(;in<argc;in++)
	#ifdef _CONSOLE
	if(*argv[in]=='-')
	{
		const wchar_t *o = argv[in]+1; //EXPERIMENTAL
		
		if('h'==o[*o=='-']) goto help; //ASSUMING

		/*
		if(o[0]=='x'&&o[1]=='-') //-x- (not --x-)
		{
			if(!wmemcmp(o+2,L"abr=",4))
			{
				abr = _wtoi(o+6);
			}
			else if(!wmemcmp(o+2,L"abe=",4))
			{
				abe = _wtoi(o+6); x_abe = true;
			}
		}*/
		if(o[0]=='-') //2021: switching SOM to MM3D
		{
			if(!wcscmp(o+1,L"wipe-extension"))
			{
				wipe_extension = true;
			}
			else if(!wcsncmp(o+1,L"file-extension=",15))
			{
				auto *ext = o+16;
				if(wcsstr(ext,L"mm3d")||wcsstr(ext,L"MM3D"))
				{
					mm3d = true; mdl = mdo = msm = false;
				}
				else if(wcsstr(ext,L"mdl")||wcsstr(ext,L"MDL"))
				{
					//only MDL isn't being offered.
					mm3d = msm = false; mdl = mdo = true;
				}
				else if(wcsstr(ext,L"mdo")||wcsstr(ext,L"MDO"))
				{
					mm3d = mdl = msm = false; mdo = true;
				}
				else if(wcsstr(ext,L"msm")||wcsstr(ext,L"MSM"))
				{
					mm3d = mdl = mdo = mhm = false; msm = true;
				}
				else if(wcsstr(ext,L"mhm")||wcsstr(ext,L"MHM"))
				{
					mm3d = mdl = mdo = false; msm = mhm = true;
				}
				else
				{
					std::wcerr << "Command-line error: \"" << o-1 << "\" didn't match mm3d, mdl, or mdo.\n";
					return 0;
				}
			}
			else if(!wcsncmp(o+1,L"ico=",3)) //4
			{
				ico = o[4]=='='?_wtoi(o+5):25;
			}
			else if(!wcscmp(o+1,L"update-texture"))
			{
				update_texture = true;
			}
			else if(!wcsncmp(o+1,L"prefix-texture=",15))
			{
				prefix_texture = o+1+15;
			}
			else if(!wcscmp(o+1,L"apply-snapshot"))
			{
				apply_snapshot = true;
			}			
			else if(!wcsncmp(o+1,L"mdl-conversion=",15))
			{
				aiSetImportPropertyInteger("SOM_MODERN_MDL_MODE",_wtoi(o+1+15));
			}
			else if(!wcscmp(o+1,L"msm-subdivide"))
			{
				SetEnvironmentVariableA("SOM_MSM_DEPTH_MAX","-1");
			}
			else if(!wcscmp(o+1,L"tmd-1034=1024"))
			{
				tmd_1034 = true; //one off setting
			}
			else if(!wcscmp(o+1,L"keep-materials"))
			{
				postprocess&=~aiProcess_RemoveRedundantMaterials;
			}
			else if(!wcscmp(o+1,L"mm3d-lerp-keys"))
			{
				x2mm3d_lerp = true;
			}
			else if('h'==o[1]) goto help;
		}
	}
	else if(integer(argv[in]))
	{
		has_snapshot = true; //2023
		def_snapshot = 
		snapshot = wcstol(argv[in],0,10);
	}
	else //2021: HUGE BLOCK (REFACTOR ME)
	#endif
	{	
		//2023: Direct3D is setup here for ico

		if(!pd3D9) //2021: dll?
		{
			exit_reason = i18n_direct3dfailure;

			pd3D9 = Direct3DCreate9(D3D_SDK_VERSION);

			if(!pd3D9) goto d3d_failure;

			std::wcout << "Status: Direct3D9 interface loaded\n";

			//IDirect3DDevice9 *pd3Dd9 = 0;

			D3DPRESENT_PARAMETERS null = 
			{
				X2MDL_TEXTURE_MAX, //256
				X2MDL_TEXTURE_MAX,D3DFMT_X8R8G8B8,1, //256,256,D3DFMT_X1R5G5B5,0,
				D3DMULTISAMPLE_NONE,0,
				D3DSWAPEFFECT_DISCARD,fake,1,
				0,D3DFMT_UNKNOWN, //1,D3DFMT_D16, //todo: enum/try 0		
				0,0,D3DPRESENT_INTERVAL_IMMEDIATE
			};
			HRESULT hr = !D3D_OK;
			/*#ifdef _CONSOLE
			hr = pd3D9->CreateDevice //REMOVE ME???
			(D3DADAPTER_DEFAULT,D3DDEVTYPE_REF, //D3DDEVTYPE_NULLREF
			1?0:hwnd, //HANGS APPLICATION WITH DUMMY WINDOW???
			D3DCREATE_SOFTWARE_VERTEXPROCESSING,&null,&pd3Dd9);
			if(hr!=D3D_OK)
			std::wcout << "Status: Reference device failed to load\n";
			#endif
			if(hr!=D3D_OK)*/
			{
				hr = pd3D9->CreateDevice
				(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,
				/*hwnd*/0, //HANGS APPLICATION WITH DUMMY WINDOW???
				D3DCREATE_HARDWARE_VERTEXPROCESSING,&null,&pd3Dd9);
			}
			if(hr!=D3D_OK) goto d3d_failure;

			D3DCAPS9 caps; 
			pd3Dd9->GetDeviceCaps(&caps);
			max_anisotropy = caps.MaxAnisotropy;

			auto mst = D3DMULTISAMPLE_16_SAMPLES;
			DWORD msq = 0;
			if(!hr&&ico&&!ms)
			{
				while(mst>2&&pd3D9->CheckDeviceMultiSampleType
				(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,D3DFMT_X8R8G8B8,1,mst,&msq))
				mst = (D3DMULTISAMPLE_TYPE)(mst/2);

				msq = msq?msq-1:0;

				if(mst>=4) pd3Dd9->CreateRenderTarget(X2MDL_TEXTURE_MAX,X2MDL_TEXTURE_MAX,rtf,mst,msq,0,&ms,0);
			}
			if(!hr&&ico) for(int i=3;i-->0;) //x2msm_ico?
			{
				D3DFORMAT f; switch(i)
				{
				case 2: f = D3DFMT_D24X8; break;
				case 1: f = D3DFMT_D24S8; break;
				case 0: f = D3DFMT_D16; break;
				}
				if(!pd3Dd9->CreateDepthStencilSurface
				(X2MDL_TEXTURE_MAX,X2MDL_TEXTURE_MAX,f,ms?mst:D3DMULTISAMPLE_NONE,ms?msq:0,0,&ds,0))
				{
					pd3Dd9->SetDepthStencilSurface(ds); break;
				}
			}	

			std::wcout << "Status: Direct3DDevice9 interface loaded\n";

			exit_reason = i18n_direct3dgeneralfailure;

			if(hr!=D3D_OK) d3d_failure:
			{
				int yesno = MessageBoxW(X2MDL_MODAL,
				L"Direct3D failed to initialize or experienced failure on one or more interfaces. \r\n" 
				"Proceed without textures?",X2MDL_EXE,X2MDL_YESNO);
			
				if(yesno!=IDYES) goto _1;

				std::wcerr << exit_reason;
			}
			else d3d_available = true;

			if(d3d_available)
			{
				pd3Dd9->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
				pd3Dd9->SetRenderState(D3DRS_ZENABLE,0);
				pd3Dd9->SetRenderState(D3DRS_ALPHATESTENABLE,0);			
				pd3Dd9->SetRenderState(D3DRS_LIGHTING,0);
			}	

			pd3Dd9->CreateStateBlock(D3DSBT_ALL,&sb);
		}
		#ifndef _CONSOLE
		dll.d3d9d = pd3Dd9; //let output convert textures
		#endif

		//options can reconfigure these
		if(mm3d) //2021
		{
			texnames_ext = L".bmp";
		}
		else if(mdo||msm) //TESTING
		{
			texnames_ext = L".txr";
		}

		//2021 (goto dll_continue?)		
		delete[] nodetable; nodetable = 0;
		aiString **nodenames = 0;
		int *nodeindex = 0;
		union{ char *mo; le32_t *mo32; };
		mo = 0;
		char *partindex = 0;
		float *bindposeinvs = 0;
		delete[] snapshotmats; snapshotmats = 0;
		delete[] bindposemats; bindposemats = 0;
		bindposeshot.clear(); //2020
		
		MDL::File mdl;

		if(x++>0)
		{
			aiReleaseImport(X); X = 0;
		}

		if(argc>1) input = argv[in];

		wchar_t absolute[MAX_PATH+4]; //REMOVE ME
		#ifdef _CONSOLE
		if(PathIsRelativeW(input))
		{
			int len = GetCurrentDirectoryW(MAX_PATH,absolute);

			absolute[len++] = '\\'; wcscpy_s(absolute+len,MAX_PATH-len,input);

			input = absolute;
		}
		#else
		//NOTE: this is a subroutine because it's a lot of code
		wchar_t *output = dll.output(absolute,input);
		{
			assert(dll.exit_code==0);

			if(dll.exit_code!=0)
			{
				exit_reason = dll.exit_reason; goto_1;
			}

			if(!output) continue; //copy mode?
		}
		#endif

		std::wcout << "x2mdl: Loading " << input << "...\n" << std::flush;

		exit_reason = i18n_inputfilefailure;

		#ifdef _CONSOLE
		if(!input||!*input) goto _1;
		#endif

		const wchar_t *ext = PathFindExtensionW(input); //DAE?

		//2021: need to use wchar_t so just switching to memory 
		//seems easiest and I think it will be helpful for KF2's
		//MO files to do memory anyway (later)
		//X = aiImportFile(input,postprocess);
		{	
			std::string buf; int rd;
			if(FILE*f=_wfopen(input,L"rb"))
			{
				fseek(f,0,SEEK_END);
				rd = ftell(f);
				fseek(f,0,SEEK_SET);

				#ifdef _CONSOLE
				if(!wcsicmp(ext,L".mo")) //King's Field II
				{
					ext = L".tmd";

					__int32 hd[3];
					if(!fread(hd,sizeof(hd),1,f))
					goto _1;
					int sz = hd[2]; if(sz>12)
					{
						memcpy(mo=new char[sz],hd,sizeof(hd));
						if(sz>=rd||sz%4||!fread(mo+12,sz-12,1,f))
						goto _1;
					}
					else if(sz<12) goto _1;

					rd-=sz;

					//REMOVE ME
					/*2020: no more just for "MO" files
					//see: consolidate (King's Field II)
					//hack: ComplexScene.cpp catches this
					//2021: no more but deboning needs it?
					aiSetImportPropertyInteger("SP_GEN_CONNECTIVITY",1);*/
				}
				if(tmd=!wcsicmp(ext,L".tmd"))
				{
					//is 1024 the scale?
					//if(tmd) scale = 1000/1024.0f; //0.9765625
					scale = 1;
				}
				#endif
				
				buf.resize(rd);
				if(rd&&!fread(&buf[0],rd,1,f)) 
				buf.clear(); //HACK...
				fclose(f);
			}
			if(buf.empty()) goto_1;

			for(int i=0;ext[i];i++) //HACK
			buf.push_back(ext[i]); 
			buf.push_back('\0');
			X = aiImportFileFromMemory(&buf[0],rd,postprocess,&buf[rd]);
		}

		if(!X) goto_1; exit_reason = 0; 

		std::wcout << "Hello: Converting " << input <<  "...\n" << std::flush;

		//2022: split MDO/MDL+MHM
		if(mdo) x2mdo_split_XY(); Y: 

		/*EXPERIMENTAL
		#ifdef _CONSOLE
		if(mm3d) x2mm3d_lerp_prep(); //REMOVE ME?
		#endif*/

		if(tmd_1034&&tmd) //2022: I was using x2mdl to do this in the past
		{
			for(int i=0;i<X->mNumMeshes;i++)
			{
				aiMesh *mesh = X->mMeshes[i];

				auto *vp = mesh->mVertices;
				for(int j=mesh->mNumVertices;j-->0;vp++)
				{
					int x = (int)vp->x, z = (int)vp->z;

					//NOTE: x2mdl was doing this to Y but that seems wrong
					if(x==+1034) vp->x = +1024;
					if(x==-1034) vp->x = -1024;
					if(z==+1034) vp->z = +1024;
					if(z==-1034) vp->z = -1024;
				}
			}
		}

		if(X->mNumMaterials>cpmaterialsN) //2021
		{
			exit_reason = i18n_materialsoverflow; goto_1;
		}
		for(int i=0;i<X->mNumMeshes;i++) //0~infinity
		{
			aiMesh *mesh = X->mMeshes[i];

			//excludes SP_GEN_CONNECTIVITY
			if(2!=mesh->mNumUVComponents[0]) //continue;
			{
				if(mm3d) //TESTING
				{
					//TODO: Maybe convert to point nodes?
					//mesh->mPrimitiveTypes = aiPrimitiveType_POINT; //???
					mesh->mPrimitiveTypes|=aiPrimitiveType_POINT;
				}

				continue;
			}

			float umin = FLT_MAX, vmin = FLT_MAX;

			unsigned int uvs = mesh->mNumVertices;

			for(int j=0;j<uvs;j++)
			{
				if(mesh->mTextureCoords[0][j].x<umin) umin = mesh->mTextureCoords[0][j].x;
				if(mesh->mTextureCoords[0][j].y<vmin) vmin = mesh->mTextureCoords[0][j].y;
			}

			float umov = -int(umin), vmov = -int(vmin);

			for(int j=0;j<uvs;j++) //hack: breaking constness
			{
				*(float*)&mesh->mTextureCoords[0][j].x+=umov;
				*(float*)&mesh->mTextureCoords[0][j].y+=vmov;
			}		
		}

		int nodecount = count(X->mRootNode);

		bool falseroot = false, cyancp = false;

		//2021: x2mdo needs this
		//aiNode **nodetable = new aiNode*[nodecount];
		nodetable = new aiNode*[nodecount];			
		nodenames = new aiString*[nodecount];
		nodeindex = new int[nodecount]; 

		flatten(nodetable,nodecount,X->mRootNode);

		//what's the use of this? MDL doesn't store names??
		quicksort(nodetable,nodenames,nodeindex,nodecount);

		#ifdef _CONSOLE
		{
			wchar_t *out = new wchar_t[MAX_PATH];
			wcscpy(out,input);
			wchar_t *ext2 = PathFindExtensionW(out);
			if(wipe_extension) *ext2 = '\0';
			wcscat_s(ext2,MAX_PATH-(ext2-out),mm3d?L".mm3d":L".mdl");			
			new(&mdl) MDL::File(out);
			delete[] out;
		}
		#else
		{
			//MDL::File mdl(output);
			new(&mdl) MDL::File(output); //goto dll_continue?
			mdl.dll = &dll; dll.input = input;
		}
		#endif

		mdl.mm3d = mm3d; //EXPERIMENTAL
		mdl.write = false; 
				
		//2021: som_db may have problems with empty 
		//animations. I'm not sure but it can't hurt
		for(int i=0,j=0,n=X->mNumAnimations;i<n;i++)
		{
			auto &a = X->mAnimations[i];

			if(!a->mNumChannels&&!a->mNumMeshChannels)
			{
				const_cast<unsigned&>(X->mNumAnimations)--;

				delete a; continue;
			}
			else a = X->mAnimations[j++];
		}

		bool containsbones = false;

		for(int i=0;i<X->mNumMeshes;i++)
		if(X->mMeshes[i]->mNumBones)
		{
			containsbones = true; //breakpoint
		}

		if(containsbones)
		{				
			int ok = 
			MessageBoxW(X2MDL_MODAL,
			//L"This type of animation (\"Scarecrow\") is not supported.\n"
			//L"\n Choose OK to continue without animation.",
			L"Animations with more than one vertex weight (100%) aren't supported, however "
			L"please request this feature and provide a good test model and it can be done."
			L"\n Choose OK to continue without animation.",
			X2MDL_EXE,MB_OKCANCEL|MB_ICONERROR);

			if(ok!=IDOK) goto_0;
		}

		//not well understood
		if(X->mNumAnimations)
		{
			//REMINDER: mdl.head.diffs overrides mdl.head.flags
			if(containsbones)
			{
				mdl.head.flags = 0x04;
			}
			else mdl.head.flags = 0x01;
		}
		else mdl.head.flags = 0x00;
		
		mdl.head.anims = 0;
		mdl.head.diffs = 0; //NOTE: depends on SP_GEN_CONNECTIVITY

		if(!containsbones) if(!mdo&&!msm||::mdl) //MSM?
		{
			for(int i=0;i<X->mNumAnimations;i++)
			{
				if(X->mAnimations[i]->mTicksPerSecond==0) //hack
				X->mAnimations[i]->mTicksPerSecond = 25; //Assimp_view default

				//NOTE: SOM won't accept MDL with both
				if(X->mAnimations[i]->mNumMeshChannels) mdl.head.diffs++;
				else if(X->mAnimations[i]->mNumChannels) mdl.head.anims++;
			}
			if(mdl.head.diffs) 
			{
				mdl.head.flags = 0x04; //HACK

				//NOTE: SOM won't accept MDL with both
				mdl.head.diffs+=mdl.head.anims; mdl.head.anims = 0; //2021
			}
		}
		if(!mdl.head.anims&&!mdl.head.diffs)
		{
			assert(!mdl.head.flags||msm||mdo);
			
			mdl.head.flags = 0; //2022

			//HACK
			//I'm not sure yet how to approach a model without any
			//animations with cpgen
			if(int x=X->mNumAnimations)
			{
				//mdl.head.flags = 1; mdl.head.anims = X->mNumAnimations;
				
				const_cast<unsigned&>(X->mNumAnimations) = 0; //2022

				while(x-->0) delete X->mAnimations[x];
			}
		}

		//NEW: mark modern? 
		//som_db.exe treats 3/2/1 as a hard animations, but I don't
		//know if it interprets them differently since there aren't
		//any MDL files that use 2/3
		//if(modern&&1==mdl.head.flags) //???
		if(modern)
		{
			//bit 2 produces junky vertices???
			//mdl.head.flags = 2;
			//bit 4 seems to work with som_db.exe so far
			mdl.head.flags|=8;
		}
		if(hardn_fps!=30&&mdl.head.flags&3) //2023 
		{
			mdl.head.flags|=16; //assuming 60 fps
		}

		memset(texmapsu,d3d_available?0xff:0,sizeof(texmapsu)); //???
		memset(texmapsv,d3d_available?0xff:0,sizeof(texmapsv)); //???
		memset(texindex,0xff,sizeof(texindex));

		texnames.clear(); texmap.clear();
		for(int i=0;i<X->mNumMaterials;i++)
		{
			int n = X->mMaterials[i]->GetTextureCount(aiTextureType_DIFFUSE);
			if(!n) continue;

			aiString path; 				
			switch(X->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE,0,&path))
			{
			case aiReturn_SUCCESS: break;

			//case aiReturn_OUTOFMEMORY: //???
					
				//exit_reason = i18n_assimpmemoryfailure; goto_1;

			default: assert(0);
					
				exit_reason = i18n_assimpgeneralfailure; goto_1;
			}	
			auto ins = texmap.insert(std::make_pair(path.data,texnames.size()));
			if(ins.second) texnames.push_back(L"");
			texindex[i] = ins.first->second;
		}
		for(size_t i=0;i<texnames.size();i++)
		{
			//aiString path;
			auto it = texmap.begin();
			while(it->second!=i) it++;
			const auto &path = it->first;

			UINT w = 1, h = 1;

			//TODO: I think this can be avoided unless writing
			//MDL data with textures
			
			//if(d3d_available)
			{
				if(path.empty()) //should not get here
				{
					exit_reason = i18n_assimpgeneralfailure; goto_1; 
				}
				else if(path[0]=='*') //ie. embedded texture
				{
					int num = atoi(path.c_str()+1);

					if(num<0||num>X->mNumTextures)
					{
						exit_reason = i18n_assimpgeneralfailure; goto_1; 
					}

					aiTexture *p = X->mTextures[num];

					w = p->mWidth; h = p->mHeight;

					if(!h)
					{
						exit_reason = i18n_compressedtexunsupported; goto_1;
					}
				}			
				else
				{
					std::string s;
					bool rel = path[0]=='.'&&(path[1]=='/'||path[1]=='\\');
					if(!rel&&PathIsRelativeA(path.c_str()))
					{
						rel = true; s.assign("./").append(path);
					}
					if(strstr(path.c_str(),"..")) //YUCK
					{
						//TODO: need to remove directories forward from relative paths
						//assert(!"..");
						
						//NOTE: PathCanoncalize is no help here... will probably have
						//to write/debug some custom parsing code (thanks Microsoft!)

						wchar_t buf[100+MAX_PATH]; 
						swprintf(buf,L"A texture has a ../ notation in its file system path.\n\n"
						L"This isn't yet supported, Sorry. Please request a fix.\n"
						L"\nModel file: %s\n\n Texture reference: %hs",input,path.c_str());
						int yesno = MessageBoxW(X2MDL_MODAL,buf,X2MDL_EXE,MB_OK|MB_ICONERROR);

						exit_reason = i18n_externaltexturefailure; goto_1;
					}					
					if(!wcsicmp(ext,L".dae")) //collada loader is weird
					{
						auto src = (s.empty()?path:s).c_str();
						s = std::regex_replace(src,std::regex("%20" )," ");				

						const char *p = s.c_str();
						if(p[0]=='/'&&p[1]&&p[2]==':') s.erase(0,1); //p++;
					}

					HRESULT hr; 

					wchar_t buf[MAX_PATH];
					int j = MultiByteToWideChar(CP_UTF8,0,
					(s.empty()?path:s).c_str(),-1,buf,MAX_PATH);
					if(j<=0)
					for(j=0;j<MAX_PATH&&(buf[j]=path[j]);j++);
					assert(j<=MAX_PATH);					

					bool retried = false; retry:

					//HACK: here this is being used to communicate the source
					//texture so that this path finding code isn't duplicated
					std::wstring &f = texnames[i];
					f = buf;
					if(buf[0]=='.'&&(buf[1]=='/'||buf[1]=='\\')) rel:
					{
						int fn = PathFindFileNameW(input)-input;
						f.assign(input,fn).append(buf+2);
					}
					else if(buf==PathFindFileNameW(buf))
					{
						wmemmove(buf+2,buf,MAX_PATH-2); goto rel;
					}

					#ifdef _CONSOLE
					if(prefix_texture)
					f.insert(PathFindFileNameW(f.c_str())-f.c_str(),prefix_texture);
					#endif

					const wchar_t *file = f.c_str();

					D3DXIMAGE_INFO info;

					hr = D3DXGetImageInfoFromFileW(file,&info);
																						
					exit_reason = i18n_externaltexturefailure;

					if(hr!=D3D_OK) 
					{
						std::wcout << "Warning: Failed to load " << file << '\n';

						#ifdef _CONSOLE
						if(!retried)
						{
							retried = true;

							file = PathFindFileNameW(buf);

							if(file-buf>2||*buf!='*')
							{
								swprintf(buf,L".\\%s",file); goto retry;
							}
						}
						#endif

						goto_1;
					}
					std::wcout << "Status: Loaded " << file << '\n';

					w = info.Width;	h = info.Height;
				}				
			
				if(w>h)
				{
					float r = float(h)/w;

					w = std::min<UINT>(X2MDL_TEXTURE_MAX,w); 
					
					h = r*w;
				}
				else if(h>w)
				{
					float r = float(w)/h;

					h = std::min<UINT>(X2MDL_TEXTURE_MAX,h);

					w = r*h;
				}
				else
				{
					w = std::min<UINT>(X2MDL_TEXTURE_MAX,w);
					h = std::min<UINT>(X2MDL_TEXTURE_MAX,h);
				}
			}

			for(int j=0;j<X->mNumMaterials;j++)
			if(i==texindex[j])
			{
				texmapsu[j] = w; texmapsv[j] = h;
			}
		}
		//2020: MOVING UP above x2mm3d_convert_points !!
		//2018: enabling partial blending/control points
	//	int x_abe_enabled = 0, x_abe_eligible = 0;
		for(int i=0;i<X->mNumMaterials;i++)
		{
			memset(controlpts+i,0x00,sizeof(controlpts[i]));

			auto *mat = X->mMaterials[i];

			aiColor4D diffuse(1,1,1,1); //RGBA
			mat->Get(AI_MATKEY_COLOR_DIFFUSE,diffuse);			
			float a = 1;
			mat->Get(AI_MATKEY_OPACITY,a);
			diffuse.a*=a;
			if(!mat->GetTextureCount(aiTextureType_DIFFUSE))
			{
				if(!mhm) //???
				if(diffuse.a==1)
				{
					controlpts[i][0] = diffuse.r*255;
					controlpts[i][1] = diffuse.g*255;
					controlpts[i][2] = diffuse.b*255;
					controlpts[i][3] = 255;
				}

				goto material;
			}
			else material:
			{
				materials[i][0] = diffuse.r;
				materials[i][1] = diffuse.g;
				materials[i][2] = diffuse.b;
				materials[i][3] = diffuse.a;

				/*REMOVE ME
				if(diffuse.a!=1) //!
				x_abe_enabled++; 
				x_abe_eligible++; //all or none*/
			}
			aiColor4D emmissive(0,0,0,0); //2021
			if(!mat->Get(AI_MATKEY_COLOR_EMISSIVE,emmissive))
			{
				materials2[i][0] = emmissive.r;
				materials2[i][1] = emmissive.g;
				materials2[i][2] = emmissive.b;
				materials2[i][3] = emmissive.a;
			}
			else memset(materials2+i,0x00,sizeof(materials2[i]));
		}
		/*2022: this looks like it could cause problems
		//now... I'm shocked it hasn't seemed to do so!
		//I thought the command-line switch was needed?
		if(!x_abe) 
		{
			abe = x_abe_enabled?1:0;
		}
		else if(!x_abe_enabled||x_abe_enabled==x_abe_eligible)
		{
			for(int i=0;i<X->mNumMaterials;i++) //all or none
			{
				//2021: I think this is wrong? But "abe" should
				//be removed anyway
				materials[i][3] = abe;
			}
		}*/

		mdl.head.parts = 0;
		
		for(int i=0;i<nodecount;i++)
		if(unsigned int&nm=nodetable[i]->mNumMeshes) //mdl.head.parts++;
		{
			#ifdef _CONSOLE
			//HACK: Convert MDL style triangles into point aiNodes?
			if(mm3d) for(int nn=nm,j=nm=0;j<nn;j++)
			{
				aiMesh *mesh = X->mMeshes[nodetable[i]->mMeshes[j]];
				if(mesh->mPrimitiveTypes&aiPrimitiveType_POINT)
				{
					x2mm3d_convert_points(mesh,nodetable[i]);
				}
				//this foils x2mm3d_gather_points for MDOs with CPs
				//if(mesh->mPrimitiveTypes>=aiPrimitiveType_TRIANGLE)
				nodetable[i]->mMeshes[nm++] = nodetable[i]->mMeshes[j];
			}
			#endif
			if(nm) mdl.head.parts++;
		}

		mdl.parts = new MDL::Part[mdl.head.parts]();
		
		int currentpart = 0, totalverts = 0, maxpacks = 0;
		
		//bool consolidate = false; //hack

		int dummy_mesh = -1, dummy_meshes = 0;

		for(int i=0;i<nodecount;i++) 
		if(nodetable[i]->mNumMeshes)
		{
			auto *nd = nodetable[i];

			if(dummy_mesh==nd->mMeshes[0]) //YUCK
			{
				maxpacks+=1; //2022

				currentpart++; continue; //???
			}

			bool controlpoints = false; //hack

			MDL::Part &part = mdl.parts[currentpart++];			

			bool sp_gen_connectivity = false; //HACK

			int realloc = 0; //YUCK

			for(int j=0;j<nd->mNumMeshes;j++)
			{	
				aiMesh *mesh = X->mMeshes[nd->mMeshes[j]];

				//assert(!mo||mesh->mTextureCoords[0]);
				for(int k=2;k-->0;) if(3==mesh->mNumUVComponents[k])
				{
					if(1==mesh->mTextureCoords[k][0].x
					 &&0==mesh->mTextureCoords[k][0].y)
					{
						sp_gen_connectivity = true;
					}
					else assert(0);
				}

				if(!mm3d) //NEW (x2mm3d_convert_points?)
				if(mesh->mPrimitiveTypes&aiPrimitiveType_POINT)
				{
					controlpoints = true;

					//2022: ignored unnamed points
					//(not accepting point clouds)
					bool named = false;

					for(int k=mesh->mNumFaces;k-->0;)
					if(1==mesh->mFaces[k].mNumIndices)
					{
						if(!named)
						{
							//without vertex colors the points can't be
							//differentiated as CPs
							assert(!mesh->mColors[0]);
							assert(1==mesh->mNumVertices);

							Color cp2;
							named|=primitive2(cp2,nd->mName.data);
							named|=primitive2(cp2,mesh->mName.data);
							if(!named) break;
						}

						part.extra+=2;
					}

					//HACK: generate normals for CPs?
					auto* &n = mesh->mNormals; if(!n)
					{
						aiVector3D z(0,0,1); //untested (or -1?)
						int k = mesh->mNumVertices;
						n = new aiVector3D[k];
						while(k-->0) n[k] = z;
					}
					/*auto* &c0 = mesh->mColors[0]; if(!c0)
					{
						//won't work because the nodes share the mesh
					}*/
				}
				else if(mesh->HasVertexColors(0)) 
				{
					controlpoints = true;
				}

				//HACK: ARM.MDL needs to consolidate these under their parent
				//plus it's the right thing to do
				if(part.extra) //dummy_mesh?
				if(part.extra==2*mesh->mNumFaces)
				{
					part.cextra = true;
					if(aiNode*p=nd->mParent)
					if(!p->mNumMeshes)
					{
						//HACK: mNumMeshes needs to be nonzero to be a part node
						if(-1==dummy_mesh)
						{
							aiScene *x = const_cast<aiScene*>(X);
							dummy_mesh = pushback(x->mMeshes,x->mNumMeshes,new aiMesh);
						}						
						realloc++; //pushback(mdl.parts,mdl.head.parts,cp)						
						dummy_meshes++;
						p->mMeshes = new unsigned int[1];
						p->mMeshes[0] = dummy_mesh;
						p->mNumMeshes = 1;
					}
				}

				part.verts+=mesh->mNumVertices;
				part.norms+=mesh->mNumVertices;
			//	part.faces+=mesh->mNumFaces;
			}
			totalverts+=part.verts;

				/*MEMORY OVERRUN (2022)
				//maxpacks is no longer including the many CP sources
			if(controlpoints) maxpacks+=2; //hack (WHY 2???) 03?
			maxpacks++; //hack: textured triangles
				*/
			maxpacks+=2; //00 or 04

			//see: consolidate (King's Field II)
			//if(mo) part.cverts = new short[part.verts];
			if(sp_gen_connectivity)
			{
				//TESTING (FIX ME!!!)
				//
				// multipart (nodes) models can't meet this
				// requirement because the consolidation is
				// in global "space" so the mapping will be
				// to higher indices than there are indices 
				//
			//	#ifdef NDEBUG
			//	#error :( //seems okay so far?
			//	#endif
				//if(mo&&mo32[2]>12||mdl.head.diffs)
				if(mo&&mo32[2]>12)
				{
				//	consolidate = true;
					part.cverts = new unsigned short[part.verts];					
				}
			}
			else assert(!mo);

			if(realloc) //part references mdl.parts
			{
				MDL::Part cp;
				mdl.parts[pushback(mdl.parts,mdl.head.parts,cp,realloc)];
			}
		}
		/*trying without (see above concerns)
		if(!consolidate) 
		{
			assert(!mdl.head.diffs); //aiAnimMesh?

			mdl.head.diffs = 0; 
		}*/
	//	else assert(!mdl.head.anims);
		
		//TODO? can totalchans be set equal to ctotalchans?
		int totalchans = 0, ctotalchans = 0; 

		//char *chanindex = 0; //2021: x2mdo needs this
		delete[] chanindex;
		//char *partindex = 0; //dll_continue?
		
		int channelmask[4] = {0,0,0,0};

		//process node hierarchy with or without animation data
		//if(mdl.head.anims)
		{		
			chanindex = new char[nodecount+1](); //for fake root
			memset(partindex=new char[nodecount],0xff,nodecount);

			//mm3d needs this for the code above that disables
			//removing nodes for x2mm3d_gather_points
			auto node_with_triangles = [&](aiNode *p)->bool
			{
				if(!mm3d) return p->mNumMeshes!=0; //cextra?

				for(int i=p->mNumMeshes;i-->0;)
				{
					auto *m = X->mMeshes[p->mMeshes[i]];
					if(m->mPrimitiveTypes&aiPrimitiveType_TRIANGLE)
					return true;
				}
				return false;
			};

			int part = 0;
			for(int i=0;i<nodecount;i++) 
			if(node_with_triangles(nodetable[i])) //mNumMeshes
			if(!mdl.parts[part++].cextra)
			{
				if(totalchans>=127) ch127: //spare room for fake root
				{
					MessageBoxW(X2MDL_MODAL,L"Number of channels exceeded 127.\r\n Aborting.",
					X2MDL_EXE,MB_OK|MB_ICONERROR);
					
					exit_reason = i18n_unabletocomplete; goto_1; 
				}
				
				chanindex[i] = totalchans++;

				setmask(channelmask,128,i);

				partindex[i] = part-1;
			}

			for(int i=0;i<nodecount;i++) 
			if(!node_with_triangles(nodetable[i])) //!mNumMeshes
			if(!nodetable[i]->mTransformation.IsIdentity())
			{	
				//2021: point nodes are getting into the 
				//channels and corrupting MDT_Joints. it
				//seems like if there's no children then
				//there's no reason to animate a channel
				//(this includes removed dummy channels)
				if(!nodetable[i]->mNumChildren) continue;

				if(totalchans>=127) goto ch127;

				chanindex[i] = totalchans++;

				setmask(channelmask,128,i);
			}

			if(mdl.head.anims)
			for(int i=0;i<X->mNumAnimations;i++)
			if(!X->mAnimations[i]->mNumMeshChannels)
			{
				aiAnimation *anim = X->mAnimations[i];

				for(int j=0;j<anim->mNumChannels;j++)
				{
					aiNodeAnim *chan = anim->mChannels[j];					

					int node = binlookup(chan->mNodeName,nodenames,nodecount);
					if(node==-1) continue; //error message?
					
					if(!getmask(channelmask,128,nodeindex[node]))
					{
						if(totalchans>=127) goto ch127;
						
						chanindex[nodeindex[node]] = totalchans++;

						setmask(channelmask,128,nodeindex[node]);
					}
				}
			}

			//these aren't outputted to animations but are needed
			//to process the geometry
			part = 0;
			for(int i=0;i<nodecount;i++) 
			if(node_with_triangles(nodetable[i])) //mNumMeshes
			if(mdl.parts[part++].cextra)
			{
				partindex[i] = part-1;

				if(!chanindex[i])
				if(!getmask(channelmask,128,i)) //2021 (NEW?)
				{
					if(totalchans>=127) goto ch127;

					chanindex[i] = totalchans++;

					//setmask(channelmask,128,i);
					ctotalchans++;
				}
			}
		}			
		if(totalchans) //channel hierarchy
		{
			//PROOF NEEDED
			//0 initializing??? I tried ff and it
			//broke the cyan CP detection... I worry
			//this can cause a loop in the hierarchy???
			mdl.chans = new short[totalchans+1](); 

			for(int i=0;i<nodecount;i++)
			{
				//HACK: set mdl.chans for cextra
				//if(getmask(channelmask,128,i))
				if(chanindex[i]||getmask(channelmask,128,i))
				{
					char ch = chanindex[i];

					short parent = 0xFF;

					aiNode *p = nodetable[i]->mParent;

					//TODO: I think this might want to skip levels?
					for(int j=0;j<nodecount;j++) if(nodetable[j]==p)
					{
						//if(getmask(channelmask,128,j))
						if(chanindex[j]||getmask(channelmask,128,j))
						{
							parent = chanindex[j];
							
							//HACK: piggybacking				
							if(char pi=~partindex[i])
							if(mdl.parts[~pi].cextra)
							{
								int pj = partindex[j];
								mdl.parts[~pi].cextra = pj;
							}
						}
						break;
					}

					mdl.chans[ch] = parent;
				}
			}

			//the extra root on the back isn't account for
			//by the channel count written to the MDL file
			//and whatever ctotalchans is for it's getting
			//in the way
			/*#if 0 //DISABLED
			int roots = 0; if(!mm3d) //not working :(
			{
				for(int i=0;i<nodecount;i++)
				{
					if(mdl.chans[i]==0xFF) roots++;
				}
				assert(roots>=1);
			}
			//FYI: this can happen even if there is a root node that's
			//unanimated, since unaminated nodes aren't retained above
			//2020
			//
			// actually O244.MDL (arrow trap) doesn't have a root node
			// MM3D requires one
			//
			if(roots>1) //there can be only one (fake root)
			{	
				std::wcout << "Warning: Multiple skeletons present...\n";
				std::wcout << "Creating a common root / fusing together!\n";

				falseroot = true;

				for(int i=0;i<nodecount;i++)
				{
					if(mdl.chans[i]==0xFF) mdl.chans[i] = totalchans;
				}
				
		//		//2020: seems wrong?
		//		if(chanindex)
		//		chanindex[totalchans] = 0xFF; //-1
				if(chanindex)
				chanindex[nodecount] = totalchans;

				getmask(channelmask,128,nodecount); //2021 (NEW?)

				mdl.chans[totalchans] = 0xFF;

				totalchans++;
			}
			#endif //DISABLED*/
		}
		ctotalchans = totalchans-ctotalchans;

		///// GEOMETRY /////

		mdl.packs = new MDL::Pack*[maxpacks+1]; 

		int currentpack = 0, currentnode = 0, currentvert = 0;		

		for(int i=0;i<mdl.head.parts;i++) //cextra?
		{
			MDL::Part *pt = mdl.parts+i;

			if(int ce=pt->cextra)
			{
				MDL::Part *cpt = mdl.parts+ce;

				pt->cstart = cpt->cstart; cpt->cstart+=pt->verts;
			}
		}
		for(int i=0;i<mdl.head.parts;i++) //note: assuming triangulated for now
		{
			while(nodetable[currentnode]->mNumMeshes==0) currentnode++;

			auto &pt = mdl.parts[i];

			pt.cnode = currentnode;

			int numpacks[0x11+1] = {};

			auto *cv = pt.cverts; //see: consolidate (King's Field II)

			Color cp2;
			bool p2 = primitive2(cp2,nodetable[currentnode]->mName.data);

			//first pass: tabulate primitives by type
			for(int m=0;m<nodetable[currentnode]->mNumMeshes;m++)
			{
				aiMesh *mesh = X->mMeshes[nodetable[currentnode]->mMeshes[m]];
				
				if(cv) //see: consolidate (King's Field II)
				{
					//HACK! SP_GEN_CONNECTIVITY?
					//Assuming not a 3D texture!
					for(int k=2;k-->0;) if(3==mesh->mNumUVComponents[k]) 
					{
						aiVector3D *cv2 = mesh->mTextureCoords[k];			
						assert(1==cv2->x); //single mesh?
						for(k=0;k<mesh->mNumVertices;k++) 
						{	
							auto z = (short)cv2->z; 
							
							//TODO? might check against summed count
							//but it still wouldn't be exact, but it
							//could catch a bug
							/*2021: I see no reason this should have
							held, ever... probably I was trying to
							get my barings. NOTE: z should include 
							everything in the original complex mesh
							assert(z<mesh->mNumVertices); //testing*/

							//cv2->y is not used because post-processing
							//might invert UV maps
							*cv++ = z; cv2++;
						}
						break;
					}
				}

				//see FLAT_CP below //ICKY
				{
					//2018: transparent?
					Color &c = controlpts[mesh->mMaterialIndex];

					//2018: diffuse based control point?
					if(c[3]||p2||primitive2(cp2,mesh->mName.data)) //2021
					if(2!=mesh->mNumUVComponents[0]) //2022: UVs? (TMD)
					{
						numpacks[0]+=mesh->mNumFaces;
						
						if(mm3d) for(int l=0;l<mesh->mNumFaces;l++)
						{
							//NEW: need to keep these for x2mm3d_gather_points
							if(3!=mesh->mFaces[l].mNumIndices) numpacks[0]--; 
						}
						
						continue;
					}
				}

				for(int k=0;k<mesh->mNumFaces;k++)
				{
					int pack = primitive(mesh,k); //TODO: what about 0 and 3?
					
					numpacks[pack]++;
				}
			}

			#ifdef _CONSOLE
			if(cv) pt.mm3d_cvertsz = cv-pt.cverts;
			#endif

			MDL::Pack **p = mdl.packs+currentpack;

			//2022: I think the code that iterates over the
			//packs has no way to account for parts without
			//any primitives, so this inserts 0xFFFFFFFF at
			//the end of the MDL part channel
			if(!numpacks[0]&&!numpacks[4]) numpacks[0] = 1;

			//second pass: allocate primitives by type
			for(int j=0;j<=0x11;j++) if(numpacks[j])
			{
				MDL::Pack *curr = 0;

				switch(j)
				{
				case 0x00: curr = *p++ = MDL::Pack::New<0x00>(numpacks[0x00]); break;
			//	case 0x03: curr = *p++ = MDL::Pack::New<0x03>(numpacks[0x03]); break;
				case 0x04: curr = *p++ = MDL::Pack::New<0x04>(numpacks[0x04]); break; 			

				default: assert(0);
					
					exit_reason = i18n_unimplemented; goto_1; 
				}

				curr->part = i; curr->type = j; curr->size = numpacks[j];		
			}
			
			//third pass: populate primitives by type
			for(int j=0;j<=0x11;j++) if(numpacks[j])
			{				
				MDL::Pack *curr = mdl.packs[currentpack]; 

				int currentprim = 0, vertexbase = pt.cstart; //0
											
				for(int m=0;m<nodetable[currentnode]->mNumMeshes;m++)
				{
					aiMesh *mesh = X->mMeshes[nodetable[currentnode]->mMeshes[m]];

					int mi = mesh->mMaterialIndex;	

					//excludes SP_GEN_CONNECTIVITY
					bool t2d = 2==mesh->mNumUVComponents[0]; //???
					
					#ifdef _CONSOLE				
					auto *mm3d_t = mesh->mTextureCoords[0];
					#endif

					short tcmp[2] = {texmapsu[mi],texmapsv[mi]};
					float tu = tcmp[0];
					float tv = tcmp[1];
					if(texmapsu[mi]==0xFFFF) tu = tv = 256; //???

					//2022: trying to add a little transparency to MDL?
					unsigned short abe = (1!=materials[mi][3])<<9&0x200;

					//DELICATE MACHINERY (DUPLICATE)
					//
					// numpacks must precisely match this logic.
					// 2022: actually I think it's fine as long
					// as enough memory is allocated (see below)

					bool p3 = p2||primitive2(cp2,mesh->mName.data);

					bool cp = p3; if(!cp) //diffuse color?
					{							
						Color &c = controlpts[mi];

						cp = c[3]!=0; memcpy(cp2,c,sizeof(c));
					}

					//2018: diffuse based control point?
					//primitive(mesh,l) doesn't have enough to
					//go on to detect this
					if(j==0&&!t2d)						
					for(int l=0;l<mesh->mNumFaces;l++)
					{
						//TODO? is primitive needed?
						if(!cp&&0!=primitive(mesh,l)) continue;

						auto ni = mesh->mFaces[l].mNumIndices;

						if(3!=ni)
						{
							//NEW: need to keep these for x2mm3d_gather_points
							if(mm3d) continue; 

							//NEW: ignore unnamed points (they run into errors)
							if(!p3||ni!=1) continue;
						}

						unsigned int *q = mesh->mFaces[l].mIndices;

						MDL::Pack::X00 &prim = curr->x00[currentprim];

						if(cp||!mesh->mColors[0])
						{
							prim.r = cp2[0];
							prim.g = cp2[1];
							prim.b = cp2[2];
							prim.c = 0x20;
						}
						else 
						{
							prim.r = int(mesh->mColors[0][q[0]].r*255+0.5f);
							prim.g = int(mesh->mColors[0][q[0]].g*255+0.5f);
							prim.b = int(mesh->mColors[0][q[0]].b*255+0.5f);
							prim.c = 0x20;
						}

						if(!cyancp)
						cyancp = prim.r==0&&prim.g==255&&prim.b==255;

						//x2mdl_pt2tri?
						// 
						// TODO: need bivector to match x2mm3d_convert_points
						// convention to roundtrip MM3D->MDL->MM3D 
						//
						if(3!=mesh->mFaces[l].mNumIndices)
						prim.verts[0] = prim.verts[1] = 
						prim.verts[2] = q[0]+vertexbase;
						else for(int k=0;k<3;k++)
						prim.verts[k] = q[2-k]+vertexbase;
						prim.norms[0] = q[0]+vertexbase;
							
						#ifdef _CONSOLE
						if(mm3d) 
						{							
							//prim.mm3d_norm = mm3d_n+q[0];

							x2mm3d_mats.push_back
							(std::make_pair(mi,(int)x2mm3d_mats.size()));
						}
						#endif

						pt.faces++; //2022

						currentprim++;						
					}
					else if(j!=0&&!cp)
					for(int l=0;l<mesh->mNumFaces;l++)
					{
						if(j!=primitive(mesh,l)) continue;

						unsigned int *q = mesh->mFaces[l].mIndices;

						#ifdef _CONSOLE
						if(mm3d) x2mm3d_mats.push_back
						(std::make_pair(mi,(int)x2mm3d_mats.size()));
						#endif

						switch(j)					
						{
						case 0x00: default: assert(0);
						{
							exit_reason = i18n_unimplemented; goto_1; 

							assert(0); break; //should be done above
						}
						/*case 0x03: 
						{
							MDL::Pack::X03 &prim =  curr->x03[currentprim];

							prim.r = int(mesh->mColors[0][q[0]].r*255+0.5f);
							prim.g = int(mesh->mColors[0][q[0]].g*255+0.5f);
							prim.b = int(mesh->mColors[0][q[0]].b*255+0.5f);
							prim.c = 0x30;
							
							for(int k=0;k<3;k++) 
							{
								unsigned int qK = q[2-k]+vertexbase;

								prim.verts[k] = qK;
								prim.norms[k] = qK;
							}

							#ifdef _CONSOLE
						//	for(int k=3;k-->0;) 
						//	prim.mm3d_norms[k] = mm3d_n+q[k];
							#endif

							break;
						}*/
						case 0x04:
						{	
							pt.faces++;

							MDL::Pack::X04 &prim =  curr->x04[currentprim];
							
							for(int k=0;k<3;k++) 
							{
								unsigned int qK = q[2-k]+vertexbase;

								prim.verts[k] = prim.norms[k] = qK;
								prim.flags[k] = 0x0000;
							}

							#ifdef _CONSOLE
							for(int k=3;k-->0;) 
							{
								//prim.mm3d_norms[k] = mm3d_n+q[k];
								prim.mm3d_uvs[k] = mm3d_t+q[k];
							}
							#endif
							 
							//TSB: texture specification byte
							prim.flags[1]|=0x1F&texindex[mi]; 																		
							prim.flags[1]|=X2MDL_TEXTURE_TPF<<7;
							//2022: assuming mode is 0 until I can
							//find time to try to decode/preserve it
							//if(a[3]) prim.flags[1]|=abr<<5&0x60; //ABR?

							//textured gouraud shaded triangle
							//NOTE: 0x400 should be added if there's a
							//texture present (t2d) ... I've added it below
							//(maybe it should always be one?)
							//(0x400 is "tme" and 0x200 is "tge"--lighting)
							prim.flags[2] = 0x34; 							
							//if(a[3]) prim.flags[2]|=abe<<9&0x200; //ABE?
							prim.flags[2]|=abe; //ABE?

							//2021: mark as texture mapped for roundtrip
							//TODO? I think SOM checks TPF too
							if(t2d) prim.flags[2]|=0x400;

							//this system is designed to move the UVs into
							//the 0~1 range based on the triangle's center
							//in order to gracefully handle UVs close to 1
							enum{ avg=1 };

							float avgu = 0, avgv = 0; if(t2d) if(avg) //2020
							{
								for(int k=0;k<3;k++)
								{
									avgu+=mesh->mTextureCoords[0][q[k]].x;
									avgv+=mesh->mTextureCoords[0][q[k]].y;
								}
								avgu = -floor(avgu/3);
								avgv = -floor(avgv/3);
							}
							for(int k=0;k<3;k++) if(t2d)
							{
								unsigned int qK = q[2-k];
						
								float x = mesh->mTextureCoords[0][qK].x+avgu;
								float y = mesh->mTextureCoords[0][qK].y+avgv;
								
								y = 1-y; //y is inverted vs. assimp

								x*=tu; y*=tv;

								if(avg) //2020
								{
									if(x<0) x = 0; if(x>tu) x = tu;
									if(y<0) y = 0; if(y>tv) y = tv;
								}
								else //old way (obsolete) //REMOVE ME
								{
									//removing wrapping for now////////

									if(fabs(x)>tu) x-=tu*(int(x/tu)); //x%=u
									if(fabs(y)>tv) y-=tv*(int(y/tv)); //y%=v
								
									///////////////////////////////////

									if(x<0) x+=tu; if(y<0) y+=tv; 

									//NOTE: this will fail since tu used to 
									//be width-1, etc.
									assert(x>=0&&x<256&&y>=0&&y<256); 
								}

								//EXTENSION
								//see swordofmoonlight_tmd_fields_t
								//mdl_edge_flags_ext notes
								short xx = (short)round(x);
								short yy = (short)round(y);
								if(xx==tcmp[0])
								{
									xx--; prim.flags[1]|=0x400<<k*2;
								}
								if(yy==tcmp[1])
								{
									yy--; prim.flags[1]|=0x400<<k*2+1;
								}

								prim.comps[k] = xx|yy<<8;
							}
							else prim.comps[k] = 0x0000;

							break;
						}} 

						currentprim++; 
					}

					vertexbase+=mesh->mNumVertices;
				}

				if(curr->size!=currentprim)
				{
					assert(!j&&curr->size>currentprim);

					curr->size = currentprim;
				}

				currentpack++;
			}

			currentnode++;
		}
		
		//NOTE: O512.mdl seems to be empty... was it a placeholder object?
		if(mdl.packs)
		mdl.packs[currentpack] = NULL; //0-terminated
		
		//yucky cextra business
		std::sort(mdl.packs,mdl.packs+currentpack,[&](MDL::Pack *a, MDL::Pack *b)
		{
			//put pure CP packs in front of the part they will belong to
			int ap = a->part, bp = b->part;
			if(int ce=mdl.parts[ap].cextra) ap = ce*2; else ap = ap*2+1;
			if(int ce=mdl.parts[bp].cextra) bp = ce*2; else bp = bp*2+1;
			return ap<bp;
		});		 
		for(int i=mdl.head.parts;i-->0;) 
		{
			mdl.parts[i].cpart = mdl.parts+i;
		}
		if(mdl.packs) //cpart?
		{
			int i = 0; currentpart = -1;

			//RFC: I think this is remapping the parts
			//according to the sort operation up above
			//(I think that simply reordering the part
			//array would pull the rug from under much
			//of the existing code) (x2mdl is a mess!)
			for(MDL::Pack**p=mdl.packs;*p;p++)
			{
				MDL::Pack *pack = *p;

				if(pack->part!=currentpart)
				{
					currentpart = pack->part;

					//I don't understand what this was
					//for, but now that all parts are
					//given a dummy pack, I think it's
					//wrong/safe to remove it
				//	while(!mdl.parts[i].verts) //???
				//	i++;

					//NEW: need to preserve parts
					//that don't have packs (CPs)
					auto p = mdl.parts+currentpart;
					auto q = mdl.parts[i].cpart;

					mdl.parts[i++].cpart = p;

					for(int j=mdl.head.parts;j-->i;)
					{
						if(p==mdl.parts[j].cpart)
						{
							mdl.parts[j].cpart = q;
						}
					}
				}
			}
				if(!mm3d) //MDO+x2mm3d_gather_points?
				{
			//2021: I keep failing the following assert... 
			//usually when adding a CP. this is a wild guess
			while(i<mdl.head.parts&&!mdl.parts[i].verts)
			{
				i++;
			}
			//hitting this now? does it include dummy_meshes?
			//assert(i+dummy_meshes==mdl.head.parts);
			assert(i==mdl.head.parts);
				}
		}

		///// ANIMATION /////

		if(mdl.head.diffs)
		mdl.diffs = new MDL::Diff*[mdl.head.diffs+1];
		if(mdl.head.anims)		
		mdl.anims = new MDL::Anim*[mdl.head.anims+1];

		int currentanim = 0;
		
		MDL::Anim *takesnapshot = 0;		
		
		//2023: -1 is default if unspecified
		snapshot = def_snapshot; if(!has_snapshot)
		{
			int min_type = 10000;
			for(int i=0;i<X->mNumAnimations;i++)
			{
				int type = ani2i(X->mAnimations[i]->mName,i);

				if(min_type&&type>=0) min_type = std::min(min_type,type);

				if(-1==type) snapshot = -1; //preferred pose identifier?
			}
			if(snapshot!=-1&&min_type<10000) snapshot = min_type;
		}

		if(mdl.head.anims)
		for(int i=0;i<X->mNumAnimations;i++)
		if(!X->mAnimations[i]->mNumMeshChannels)
		{
			aiAnimation *anim = X->mAnimations[i];
			
			//RECOMPUTING!!!
			//int n = 1+anim->mDuration/anim->mTicksPerSecond*30; //15
			int n = hardn(i);

			//2021: I can't remember what ctotalchans is exactly
			//however takesnapshot is used later with totalchans
			//so its buffer can't overflow
			int type = ani2i(anim->mName,i);
			int total = type==snapshot?totalchans:ctotalchans;

			mdl.anims[currentanim] = MDL::Anim::New(n*total); //ctotalchans

			MDL::Anim *curr = mdl.anims[currentanim];

			curr->size = n*ctotalchans; 

			curr->type = type;
			
			if(type==snapshot)
			{
				if(mdo&&::mdl) takesnapshot = curr;
			}

			if(type==1&&falseroot&&!cyancp)
			{				
				int yesno = MessageBoxW(X2MDL_MODAL,
				L"This input was fused and was not provided a cyan control point. "
				"It appears to want to walk but will not be able to walk as is. "
				"\n\n Proceed?",X2MDL_EXE,X2MDL_YESNO);
				
				#ifdef _CONSOLE
				if(yesno!=IDYES) goto_1; goto_0; 
				#else
				goto_1 //FIX ME (YUCK)
				#endif
			}

			curr->steps = n; //curr->setchans(0);

			memset(curr->info,0x00,n*total*6*sizeof(short)); //ctotalchans

			for(int j=anim->mNumChannels;j-->0;)
			{
				//DUPLICATE
				auto *cmp = anim->mChannels[j];
				if(cmp->mNumScalingKeys>1) lll: 
				{		
					int sz = 3*n*total;
					auto *s = new uint8_t[sz];
					memset(s,128,sizeof(uint8_t)*sz);

					(void*&)curr->scale = s; break;
				}
				else
				{
					auto&lll = cmp->mScalingKeys[0].mValue;
					if(lll.x!=1.0f||lll.y!=1.0f||lll.z!=1.0f)
					goto lll;
				}
			}

			currentanim++;
		}

//		std::wcout << "Debug: front loading animations. \n" << std::flush;

		//process node hierarchy with or without animation data
		//if(mdl.head.anims)
		for(int i=0;i<nodecount;i++)
		if(!nodetable[i]->mTransformation.IsIdentity())
		{							
			int currentchan = chanindex[i];
								
			if(currentchan==-1) 
			{
				assert(0); continue; //fake root (should not occur)
			}

			if(!chanindex[i]&&!getmask(channelmask,128,i)) //HACK?
			{
				//2022: MM3D->MDL CP rotation is being assigned to
				//soft animation mesh (KF2 slime)
				continue;  
			}

			assert(currentchan<totalchans);
			
//			std::wcout << "Debug: animation " << currentchan << " of " << totalchans << ".\n" << std::flush;

			aiVector3D pos, rot, scl; aiQuaternion q;
 
			nodetable[i]->mTransformation.Decompose(scl,q,pos);
			
			rot = q2xyz(q)*r2fpi; pos = pos.SymMul(scale);

			short p[9];
			p[0] = round(rot.x)*invert[0];
			p[1] = round(rot.y)*invert[1];
			p[2] = round(rot.z)*invert[2];
			p[3] = round(pos.x)*invert[0];
			p[4] = round(pos.y)*invert[1];
			p[5] = round(pos.z)*invert[2];
			short *p6 = p+6;
			for(int i=3;i-->0;)
			{
				p6[i] = (int)(128*scl[i]+0.5f);
			}
			if(p6[0]!=128||p6[1]!=128||p6[2]!=128)
			{
				for(int i=3;i-->0;)
				p6[i] = std::max<int>(0,std::min<int>(255,p6[i]));
			}
			else p6[0] = -1;
							
			//preload animations with static transform state (???)
						
			if(bindposeshot.empty()) 
			{
				bindposeshot.assign(totalchans*9,0);
				auto *s = &bindposeshot[0];
				for(int i=totalchans*9-3;i>0;i-=9) s[i] = -1;
			}

			if(!mdl.head.anims //NEW
			||currentchan>=ctotalchans)
			{
				memcpy(&bindposeshot[currentchan*9],p,9*sizeof(*p));
			}
			else if(currentchan||getmask(channelmask,128,i)) //???
			{
				currentanim = 0;

				for(int j=0;j<X->mNumAnimations;j++)
				if(!X->mAnimations[j]->mNumMeshChannels) 
				{
					aiAnimation *anim = X->mAnimations[j];

					MDL::Anim *curr = mdl.anims[currentanim++];
			
					//RECOMPUTING!!!
					//int n = 1+anim->mDuration/anim->mTicksPerSecond*30; //15
					int n = hardn(j);
				
					short *d = curr->info[n*currentchan];
					memcpy(d,p,6*sizeof(*d));

					if(p6[0]!=-1) if(auto*s=curr->scale) s:
					{
						s+=n*currentchan;
						for(int i=3;i-->0;) (*s)[i] = (uint8_t)p6[i];
					}
					else //DUPLICATE
					{
						int sz = 3*n*totalchans; //ctotalchans
						(void*&)s = new uint8_t[sz];
						memset(s,128,sizeof(uint8_t)*sz);

						curr->scale = s; goto s;
					}
					
					if(j==0)
					{
						memcpy(&bindposeshot[currentchan*9],p,9*sizeof(*p));
					}
				}
			}
		}

//		std::wcout << "Debug: compiling animations. \n" << std::flush;

		currentanim = 0;

		if(mdl.head.anims)
		for(int i=0;i<X->mNumAnimations;i++)
		if(!X->mAnimations[i]->mNumMeshChannels)
		{
//			std::wcout << "Debug: animation " << i << " of " << X->mNumAnimations << ".\n" << std::flush;

			aiAnimation *anim = X->mAnimations[i];
			
			//RECOMPUTING!!!
			//int n = 1+anim->mDuration/anim->mTicksPerSecond*30; //15
			int n = hardn(i);

			//note: starting one pseudo (pose) step before time zero
			float step,time; if(!modern)
			{
				//step = anim->mDuration/(n-1); time = -step; //???
				step = anim->mDuration/(n-2); time = -step; //???
			}
			else 
			{
				//2022: SomLoader.cpp now subtracts 1 from the duration
				//which som_db.exe throws away... -1 here gets the step
				//size back to 1.0
				//step = anim->mDuration/(n-!i);
				step = anim->mDuration/(n-!i-1);
			}

			MDL::Anim *curr = mdl.anims[currentanim];

			/*OBSOLETE? the "modern" format typically must animate
			//every channel to undo the bind pose
			if(modern||takesnapshot) 
			for(int i=0;i<ctotalchans;i++)
			setmask(curr->chans,128,i);*/

			for(int k=0;k<anim->mNumChannels;k++)
			{
				time = 0;

				int keys[3] = {}; //rotation/position/scale 
							
				aiNodeAnim *chan = anim->mChannels[k];

				int node = binlookup(chan->mNodeName,nodenames,nodecount);
				if(node==-1) continue; //error message?

				int currentchan = chanindex[nodeindex[node]];

				assert(currentchan<ctotalchans);

				if(currentchan==-1) 
				{
					assert(0); continue; //fake root (should not occur)
				}

				/*OBSOLETE? the "modern" format typically must animate
				//every channel to undo the bind pose
				if(!modern)
				setmask(curr->chans,128,currentchan);*/

				short *p = curr->info[n*currentchan], delta[6] = {}; 

				auto *s = curr->scale; if(s) s+=n*currentchan;
					
				aiVector3D pos0, scl0; aiQuaternion qrot0;
				aiNode *nd = nodetable[nodeindex[node]];
				nd->mTransformation.Decompose(scl0,qrot0,pos0);
				aiQuaternion qrot = qrot0;
				aiVector3D rot = q2xyz(qrot);
				aiVector3D pos = pos0;
				aiVector3D scl = scl0;

				int j = 0; if(!i&&modern)
				{
					j = 1;
					
					memcpy(delta,p,sizeof(delta)); p+=6;
					
					if(s) s+=1;
				}
				for(;j<curr->steps;j++)			
				{
					int breakout = 0; float a,b,t;
					
					if(chan->mRotationKeys[chan->mNumRotationKeys-1].mTime>=time)
					{
						aiQuaternion *src;
						while(keys[0]<chan->mNumRotationKeys-1
						&&chan->mRotationKeys[keys[0]].mTime<=time) 
						keys[0]++;						
						b = chan->mRotationKeys[keys[0]].mTime;
						if(keys[0]&&b>time)
						{
							src = &chan->mRotationKeys[keys[0]-1].mValue;

							a = chan->mRotationKeys[keys[0]-1].mTime;

							qrot0: //decompose?

							t = (time-a)/(b-a); //zero divide

							if(t==0) //TESTING
							{
								qrot = *src;
							}
							else if(t==1)
							{
								qrot = chan->mRotationKeys[keys[0]].mValue;
							}
							else
							{
								aiQuaternion::Interpolate
								(qrot,*src,chan->mRotationKeys[keys[0]].mValue,t);
							}

							rot = q2xyz(qrot);
						}
						else if(b==0||chan->mPreState==aiAnimBehaviour_CONSTANT)
						{
							rot = q2xyz(qrot=chan->mRotationKeys[keys[0]].mValue);
						}
						else
						{
							a = 0; src = &qrot0; goto qrot0;
						}
					}
					else //breakout++;
					{
						//2022: the last key can slip through the cracks
						//(I learned this manipulating scale to hide coinciding geometry)
						if(memcmp(&qrot,&chan->mRotationKeys[chan->mNumRotationKeys-1].mValue,sizeof(qrot)))
						rot = q2xyz(qrot=chan->mRotationKeys[chan->mNumRotationKeys-1].mValue);

						if(0&&chan->mPostState!=aiAnimBehaviour_CONSTANT) //!!
						{
							//assert(0); //IMPLEMENT ME
						}

						breakout++;
					}

					if(chan->mPositionKeys[chan->mNumPositionKeys-1].mTime>=time)
					{
						while(keys[1]<chan->mNumPositionKeys-1
						&&chan->mPositionKeys[keys[1]].mTime<=time) 
						keys[1]++;
						b = chan->mPositionKeys[keys[1]].mTime;
						if(keys[1]&&b>time)
						{
							pos = chan->mPositionKeys[keys[1]-1].mValue;

							a = chan->mPositionKeys[keys[1]-1].mTime;

							pos0: //decompose?

							t = (time-a)/(b-a); //lerp //zero divide

							pos+=(chan->mPositionKeys[keys[1]].mValue-pos)*t;							
						}
						else if(b==0||chan->mPreState==aiAnimBehaviour_CONSTANT)
						{
							pos = chan->mPositionKeys[keys[1]].mValue;
						}
						else
						{
							a = 0; pos = pos0; goto pos0;
						}
					}
					else //breakout++; 
					{
						//2022: the last key can slip through the cracks
						//(I learned this manipulating scale to hide coinciding geometry)
						pos = chan->mPositionKeys[chan->mNumPositionKeys-1].mValue;

						if(0&&chan->mPostState!=aiAnimBehaviour_CONSTANT) //!!
						{
							//assert(0); //IMPLEMENT ME
						}

						breakout++;
					}

					if(chan->mScalingKeys[chan->mNumScalingKeys-1].mTime>=time)
					{
						while(keys[2]<chan->mNumScalingKeys-1
						&&chan->mScalingKeys[keys[2]].mTime<=time) 
						keys[2]++;
						b = chan->mScalingKeys[keys[2]].mTime;
						if(keys[2]&&b>time)
						{
							scl = chan->mScalingKeys[keys[2]-1].mValue;

							a = chan->mScalingKeys[keys[2]-1].mTime;

							scl0: //decompose?

							t = (time-a)/(b-a); //lerp //zero divide

							scl+=(chan->mScalingKeys[keys[2]].mValue-scl)*t;
						}
						else if(b==0||chan->mPreState==aiAnimBehaviour_CONSTANT)
						{
							scl = chan->mScalingKeys[keys[2]].mValue;
						}
						else
						{
							a = 0; scl = scl0; goto scl0;
						}
					}
					else //breakout++;
					{
						//2022: the last key can slip through the cracks
						scl = chan->mScalingKeys[chan->mNumScalingKeys-1].mValue;

						if(0&&chan->mPostState!=aiAnimBehaviour_CONSTANT) //!!
						{
							//assert(0); //IMPLEMENT ME
						}

						breakout++;
					}
			
					if(breakout==3) //break; //OPTIMZING?
					{
						if(s) //YUCK: //scaling is stored in absolute terms
						{
							if(j==0) 
							{
								for(int k=3;k-->0;)
								{
									int si = (int)(scl[k]*128+0.5f);
									(*s)[k] = (uint8_t)std::min(255,std::max(0,si));
								}

								j = 1; s++;
							}
							for(;j<curr->steps;j++,s++)	
							{								
								for(int k=3;k-->0;) s[0][k] = s[-1][k];
							}
						}

						break; //breakpoint //OPTIMZING?
					}

					//if(snapshotmats)
					{	
						//Reminder: inserting snapshot transformations
						//at this point (into relative reference frames)
						//ammounted to a dead end / proved impossible.
					}

					//2021: invert is applied below to simplify the normalization
					p[0] = round(rot.x*r2fpi); p[3] = round(pos.x*scale)*invert[0];
					p[1] = round(rot.y*r2fpi); p[4] = round(pos.y*scale)*invert[1];
					p[2] = round(rot.z*r2fpi); p[5] = round(pos.z*scale)*invert[2];
					
					for(int i=0;i<3;i++)
					{
						p[i]*=invert[i];

						//2021: copying shortest path logic from som.MDL.cpp
						//this makes interframe interpolation more efficient
						if(1)
						{
							int a = delta[i];
							int b = a+p[i];

							//shortest path?
							int c = (b-a+2048)%4096-2048;
							if(c<-2048) c+=4096;

							p[i] = (short)c;
						}
					}
					for(int i=0;i<6;i++) 
					{
						short diff = p[i]-delta[i]; delta[i] = p[i]; p[i] = diff;
					}

					if(s) for(int k=3;k-->0;)
					{
						int si = (int)(scl[k]*128+0.5f);
						(*s)[k] = (uint8_t)std::min(255,std::max(0,si));
					}

					p+=6; if(s) s+=1; time+=step; 
				}
			}
			/*if(modern&&!mm3d) //TESTING (REMOVE ME)
			{
				if(!i&&n>1)
				for(int k=ctotalchans;k-->0;)
				if(!getmask(curr->chans,128,k))
				{
					//if not animated the bind pose needs to be undone by the 
					//first frame
					short (*p)[3+3] = &curr->info[n*k];				
					for(int j=3+3;i-->0;) p[1][j] = p[0][j]; //good enough?
				}
			}*/

			currentanim++;
		}	
				
		if(mdl.anims)
		mdl.anims[currentanim] = NULL; //0-terminated

		//YUCK: x2mdo needs this to be global
		//float *snapshotmats = 0;
		//float *bindposeinvs = 0; //dll_continue?

		if(takesnapshot||!bindposeshot.empty())
		{
			//Reminder: discovered that it was 
			//necessary to implement a global solution
			//to this problem (one which considers the 
			//entire transformation chain top to bottom)
			
			SWORDOFMOONLIGHT::mdl::hardanim_t 
			*hardanimpose = new SWORDOFMOONLIGHT::mdl::hardanim_t[totalchans],
			*frameclear = 0;

			bool *known = new bool[totalchans];
			
			//delete[] snapshotmats; //x2mdo?
			if(takesnapshot)
			snapshotmats = new float[totalchans*16];

			if(!bindposeshot.empty())
			{
				bindposemats = new float[totalchans*16];
				bindposeinvs = new float[totalchans*16];
				frameclear = new SWORDOFMOONLIGHT::mdl::hardanim_t[totalchans*3];
			}

			float meters[4] = {invert[0]/1024.0f,invert[1]/1024.0f,invert[2]/1024.0f,1}; //scale?

			//2021: this used to be either takesnapshot or the other, but now
			//the MDL data uses the generic/bindpose pose (also for art files)
			//and MDO data uses the expressive pose
			int passes = snapshotmats?2:1;
			//snapshotmats is coming out wrong on second pass... I think the lower
			//code may be modifying the animation deltas... need to decouple these
			//for(int pass=1;pass<=passes;pass++)
			for(int pass=passes;pass>=1;pass--) 
			{
				if(pass==1&&bindposeshot.empty()) continue;

				for(int i=0;i<totalchans;i++)
				{	
					//it looks like mdl::transform will access
					//framemats beyond ctotalchans
					//int map = mdl.chans[i]==0xFF?-1:mdl.chans[i];
					int map = mdl.chans[i]>=ctotalchans?-1:mdl.chans[i];

					if(pass==1)
					{
						frameclear[i].set();
						frameclear[i].map = map;
					}
					hardanimpose[i].map = map;

					if(pass==2) //takesnapshot
					{
						//note: haven't confirmed it but EneEdit seems
						//to pick a frame deeper into the animation than
						//the first frame (not including first dummy frame)
						hardanimpose[i].set(takesnapshot->info[takesnapshot->steps*i]);

						//EneEdit seems to pose by the second frame...
						//though I haven't seen its code that does this
						enum{ n=0 }; //DUPLICATE (x2mdo.cpp)

						//late 2021... something isn't working... the
						//bind pose is being captured in x2mdo... this
						//is copied from where takesnapshot is grabbed?						
						int deeper = 1+std::min(!snapshot+n,takesnapshot->steps-1);
						if(auto*s=takesnapshot->scale)
						hardanimpose[i].scale(s[takesnapshot->steps*i+deeper-1]);						
						while(deeper-->1)
						hardanimpose[i].add(takesnapshot->info[takesnapshot->steps*i+deeper]);
					}
					else //2020
					{
						auto &a = *(short(*)[3+3])&bindposeshot[9*i];
						hardanimpose[i].set(a);
						if(a[6]!=-1)
						hardanimpose[i].scale(*(short(*)[3])(&a+1));
					}
				}

				memset(known,0x00,sizeof(*known)*totalchans);				

				float *mats = pass==2?snapshotmats:bindposemats;

				if(apply_snapshot&&snapshotmats&&pass==1) //REMOVE ME?
				{
					//I thought doors were ajar some so their vertices didn't 
					//get stuck together... but that only happens on the first
					//animation frame... in theory this may still come in handy
					memcpy(bindposemats,snapshotmats,sizeof(float)*totalchans*16);
				}
				else for(int i=0;i<totalchans;i++)
				SWORDOFMOONLIGHT::mdl::transform(hardanimpose,i,0,meters,mats,known);			

					if(pass==2) continue;

				SWORDOFMOONLIGHT::mdl::hardanim_t
				*frameinputs = frameclear+totalchans, *frameoutputs = frameclear+totalchans*2;
				
				SWORDOFMOONLIGHT::mdl::inverse(bindposeinvs,bindposemats,totalchans);

				/*REMOVE ME
				//this was for if pretransforming Assimp's vertices as an optimization
				//but it didn't work out because of intancing (e.g. control points)
				if(snapshotmats) if(apply_snapshot) //REMOVE ME?
				{
					delete[] snapshotmats; snapshotmats = 0;
				}
				else SWORDOFMOONLIGHT::mdl::product(snapshotmats,snapshotmats,bindposeinvs,ctotalchans);*/
								
				float *framemats = new float[ctotalchans*16];
				float *frameinvs = new float[ctotalchans*16];

				if(mdl.anims&&!mm3d)
				for(MDL::Anim **curr=mdl.anims;*curr;curr++)
				{
					MDL::Anim *p = *curr;

					memcpy(frameinputs,frameclear,sizeof(*frameclear)*ctotalchans);
					memcpy(frameoutputs,frameclear,sizeof(*frameclear)*ctotalchans);

					int i = modern&&curr==mdl.anims;
					if(i) for(int j=0;j<ctotalchans;j++)
					{	
						frameinputs[j].set(p->info[p->steps*j]);
						if(auto*s=p->scale)
						frameinputs[j].scale(s[p->steps*j]);
						frameoutputs[j].set(frameinputs[j]);
					}
					for(;i<p->steps;i++)			
					{	
						for(int j=0;j<ctotalchans;j++)
						{
							known[j] = false;
							frameinputs[j].add(p->info[p->steps*j+i]);
							if(auto*s=p->scale)
							frameinputs[j].scale(s[p->steps*j+i]);
						}

						for(int j=0;j<ctotalchans;j++)
						SWORDOFMOONLIGHT::mdl::transform(frameinputs,j,0,meters,framemats,known);

						//memcpy(framemats,bindposeinvs,sizeof(float)*16*ctotalchans); //testing 
						SWORDOFMOONLIGHT::mdl::product(framemats,framemats,bindposeinvs,ctotalchans);
						SWORDOFMOONLIGHT::mdl::inverse(frameinvs,framemats,ctotalchans);
												
						for(int j=0;j<ctotalchans;j++)
						{	
							short delta[6]; frameoutputs[j].get(delta);

							//o332.mdl (a door) is outside this range??? fixed above?
							int parent = hardanimpose[j].map; assert(parent<ctotalchans);

							float prod[16], *inv = frameinvs+parent*16;

							if(inv>=frameinvs) SWORDOFMOONLIGHT::mdl::product(prod,inv,framemats+j*16);
							if(inv<frameinvs) memcpy(prod,framemats+j*16,sizeof(float)*16); //root

							aiMatrix4x4 &ai = *(aiMatrix4x4*)prod; ai.Transpose();

							aiQuaternion q; aiVector3D rot, pos, scl;
							
							ai.Decompose(scl,q,pos); rot = q2xyz(q);

							SWORDOFMOONLIGHT::mdl::hardanim_t diff;
							diff.cv[0] = round(rot.x*r2fpi)*invert[0]; 
							diff.cv[1] = round(rot.y*r2fpi)*invert[1];
							diff.cv[2] = round(rot.z*r2fpi)*invert[2];
							diff.ct[0] = round(pos.x*scale)*invert[0];
							diff.ct[1] = round(pos.y*scale)*invert[1];
							diff.ct[2] = round(pos.z*scale)*invert[2];
							if(p->scale) for(int i=3;i-->0;)
							{
								int s = (int)(scl[i]*128+0.5f);
								diff.cs[i] = (uint8_t)std::min(255,std::max(0,s));
							}
							else diff.unscale(); frameoutputs[j].set(diff);

							diff.sub(delta); diff.get(p->info[p->steps*j+i]);

							if(auto*s=p->scale) memcpy(s+p->steps*j+i,diff.cs,3);
						}					
					}
				}

				delete[] framemats;
				delete[] frameinvs;
			}

			//delete[] bindposeinvs;
			delete[] hardanimpose;
			delete[] frameclear;				
			delete[] known;
		}

		mdl.verts = new short[totalverts*3];
		mdl.norms = new short[totalverts*3];

		currentnode = 0; currentvert = 0;

		for(int i=0;i<mdl.head.parts;i++) 
		{				
			/*2020: vertices may be in different order if 
			//converting/consolidating points to triangle
			//control points
			while(nodetable[currentnode]->mNumMeshes==0)
			currentnode++;*/
			currentnode = mdl.parts[i].cpart->cnode;

			int currentchan = chanindex?chanindex[currentnode]:0; 

			for(int j=0;j<nodetable[currentnode]->mNumMeshes;j++)
			{
				aiMesh *mesh = X->mMeshes[nodetable[currentnode]->mMeshes[j]];

				if(mm3d&&!mesh->mNormals) //point mesh?
				{
					assert(1==mesh->mNumVertices);

					currentvert+=mesh->mNumVertices;
				}
				else for(int k=0;k<mesh->mNumVertices;k++) 
				{
					//NOTE: for MDO it would be good to transform the vertices
					//in place. the mAnimMeshes code needs -1 indices to match
					//plus technically Assimp's meshes can be instanced clones
					//
					// REMINDER: I implemented this forgetting about instances
					//
					aiVector3D v = mesh->mVertices[k];
					aiVector3D n = mesh->mNormals[k].Normalize(); //???

					if(bindposemats) //snapshotmats
					{
						SWORDOFMOONLIGHT::mdl::multiply(bindposemats+currentchan*16,&v.x,&v.x,1);
						SWORDOFMOONLIGHT::mdl::multiply(bindposemats+currentchan*16,&n.x,&n.x,0);
					}

					for(int l=0;l<3;l++)
					{
						mdl.verts[currentvert*3+l] = round(v[l]*invert[l]*scale);
						mdl.norms[currentvert*3+l] = round(n[l]*invert[l]*4096);
					}

					currentvert++;
				}
			}		

			currentnode++;
		}

		#ifdef _CONSOLE
		if(mm3d)
		{
			x2mm3d_nodetable.assign(totalchans,nullptr);

			for(int i=0;i<nodecount;i++)
			if(getmask(channelmask,128,i))
			{
				char ch = chanindex[i];
				if(ch!=-1) x2mm3d_nodetable[ch] = nodetable[i];
			}
		}
		#endif
		 	 
		//if(consolidate) //MM3D?
		{
			mdl.consolidate(); //SP_GEN_CONNECTIVITY
		}		
		if(mo) //King's Field II
		{				
			//mdl.consolidate(); //SP_GEN_CONNECTIVITY
			mdl.mo(mo32);
		}
		bool xposed = false;
		if(mdl.head.diffs)
		for(int w,we,i=currentanim=0;i<X->mNumAnimations;i++)
		if(X->mAnimations[i]->mNumMeshChannels)
		{
			aiAnimation *anim = X->mAnimations[i];			
			aiMeshAnim *cmp = anim->mMeshChannels[0];

			if(!currentanim)
			{
				//assuming single mesh
				//int w = 3*mdl.parts[0].verts; //FIX ME?
				w = we = 0;
				for(int i=mdl.head.parts;i-->0;) //CPs?
				{
					w+=3*mdl.parts[i].verts;
					we+=3*mdl.parts[i].extra; //EXPERIMENTAL
				}
				w+=we;
			}

			int n = cmp->mNumKeys; if(n) //2022
			{
				int pre = cmp->mKeys[0].mTime>0;
				int post = anim->mDuration>cmp->mKeys[n-1].mTime;			
				if(pre+post)
				{
					n+=pre+post;

					for(int k=anim->mNumMeshChannels;k-->0;)
					{
						aiMeshAnim *ext = anim->mMeshChannels[k];

						ext->mNumKeys = n;

						auto *swap = ext->mKeys;
						memcpy((ext->mKeys=new aiMeshKey[n])+pre,swap,sizeof(*swap)*(n-pre-post));
						delete[] swap;
						swap = ext->mKeys;

						if(pre) switch(cmp->mPreState)
						{
						case aiAnimBehaviour_CONSTANT:
					
							swap[0].mValue = swap[1].mValue; break;

						default: swap[0].mValue = -1; break;
						}
						if(post) switch(cmp->mPostState)
						{
						case aiAnimBehaviour_CONSTANT:

							swap[n-1].mValue = swap[n-2].mValue; break;

						default: swap[n-1].mValue = -1; break;
						}
						if(pre) swap[0].mTime = 0;
						if(post) swap[n-1].mTime = anim->mDuration;
					}
				}
			}
			else assert(0);

			mdl.diffs[currentanim] = MDL::Diff::New(w,n);

			MDL::Diff *curr = mdl.diffs[currentanim];

			curr->type = ani2i(anim->mName,i);
										
		//	if(curr->type==snapshot) takesnapshot = curr;

			if(curr->type==1&&falseroot&&!cyancp)
			{				
				int yesno = MessageBoxW(X2MDL_MODAL,
				L"This input was fused and was not provided a cyan control point. "
				"It appears to want to walk but will not be able to walk as is. "
				"\n\n Proceed?",X2MDL_EXE,X2MDL_YESNO);
						
				#ifdef _CONSOLE
				if(yesno!=IDYES) goto_1; goto_0; 
				#else
				goto_1 //FIX ME (YUCK)
				#endif
			}

			currentanim++;
			
			short total = round(anim->mDuration/anim->mTicksPerSecond*30);
			short *times = curr->times();
			for(int j=0;j<n;j++)
			times[j] = round(cmp->mKeys[j].mTime/anim->mTicksPerSecond*30);				
			for(int j=n;j-->1;) 
			times[j]-=times[j-1];
			for(int j=0;j<n;j++) if(times[j]<1) //HACK
			{
				//first frame shouldn't contribute to running time
				//this will cause the length be 2 frames less than
				//the input
				if(!j) continue; //establishing frame?

				//NOTE: SOM's original decoder divides by the
				//times
				times[j] = 1; 

				//NOTE: better to do this with floating-point
				if(j<n-1&&times[j+1]>1) times[j+1]--;
				else if(j&&times[j-1]>1) times[j-1]--;
			}
			//FIX??? (FOR ABOVE)
			for(int j=0;j<n;j++)
			total-=times[j]; assert(total<=1); //TESTING
			assert(total==0);
			// FIX? I think I may have forgotten about this
			//
			// TODO: might ignore this in the future if the
			// first frame interpolates the base mesh
			// 
			// NOTE: x2mm3d.cpp has some code that ignores
			// this frame... this is critical to maintain
			// the frame count on round-trips 
			//
			times[0]++; //establishing frame?
							
			static std::unordered_map<aiNode*,int> xmap;
			xmap.clear();
			static std::vector<aiMatrix4x4> x;
			x.resize(anim->mNumChannels);				
			for(int k=anim->mNumChannels;k-->0;)
			{
				aiNodeAnim *chan = anim->mChannels[k];

				int node = binlookup(chan->mNodeName,nodenames,nodecount);
				if(node==-1) continue; //error message? mm3d point?

				aiNode *nd = nodetable[nodeindex[node]];
				xmap[nd] = k;
				x[k] = nd->mTransformation;
			}
			//UNTESTED: need to subtract the bind poses (I think)
			//
			// NOT WORKING FOR CPS... WAS THIS FOR King's Field II? WHY?
			//
			//#ifdef _DEBUG
			if(bindposeinvs)
			{
				if(!xposed)
				for(int i=totalchans*16;(i-=16)>=0;)
				((aiMatrix4x4*)(bindposeinvs+i))->Transpose();
				xposed = true;
			}
			//#else
			//#error resolved? why do cps not require this? I'm stupid?
			//#endif

			aiMatrix4x4 xm;

			short *lb = curr->verts, *ub = lb+w*n;

			for(int k=anim->mNumMeshChannels;k-->0;)
			{
				aiMeshAnim *ch = anim->mMeshChannels[k];

				if(n!=ch->mNumKeys) keyframe_error:
				{
					MessageBoxW(X2MDL_MODAL,
					L"A vertex-animation section has different keyframes than the rest.",
					X2MDL_EXE,MB_OK|MB_ICONERROR);
					goto_1;
				}					
				for(int j=0;j<n;j++)
				{
					if(ch->mKeys[j].mTime!=cmp->mKeys[j].mTime) 
					goto keyframe_error;					
				}
			}

			for(int j=0;j<n;j++) 
			{
				if(!x.empty())
				{
					//TODO: will need to do better for missing first and last frames					
					//x2mdl_animate(anim,&x[0],s,t);
					x2mdl_animate(anim,&x[0],cmp->mKeys[j].mTime);
				}

				int v0 = 0, v00 = 0; //YUCK

				for(int i=0;i<mdl.head.parts;i++)
				{
					//MDL::Part &pt = mdl.parts[i];
					MDL::Part &pt = *mdl.parts[i].cpart; //NEW

					int currentnode = pt.cnode;

					if(!pt.cstart) //YUCK
					{
						v0 = v00; v00 = 0; //this is merging for single part diffs
					}
					v00+=3*(pt.verts+pt.extra);

						pt.diff_v0 = v0; //REMOVE ME

					if(!x.empty())
					{
						aiNode *nd = nodetable[pt.cnode];
						auto it = xmap.find(nd);
						xm = it!=xmap.end()?x[it->second]:nd->mTransformation;
						for(aiNode*p=nd->mParent;p;p=p->mParent)
						{									
							it = xmap.find(p);
							auto &mul = it!=xmap.end()?x[it->second]:p->mTransformation;
							xm = mul*xm;
						}
						if(xposed) //bindposeinvs
						{
							//I added this to enable complex models that add a
							//skeleton to drive the CPs for my KFII project's
							//skeleton (monster) model to be easier to animate
							//
							//there's more code like this below, but it's not
							//working. I'm not sure why, perhaps because the 
							//MM3D loader inverse transforms geometry but not
							//points (since they're nodes)

							int currentchan = chanindex?chanindex[currentnode]:0;
							xm = xm*(*(aiMatrix4x4*)(bindposeinvs+currentchan*16));
						}
					}
						
					//see: consolidate (King's Field II)
					auto *cv = pt.cverts, *cv2 = cv; int cst3 = v0+3*pt.cstart;

					assert(cv||!pt.verts); //2021: is this guaranteed?

					for(int m=0;m<nodetable[currentnode]->mNumMeshes;m++,cv=cv2)
					{
						aiMesh *mesh = X->mMeshes[nodetable[currentnode]->mMeshes[m]];
 							
						cv2 = cv+mesh->mNumVertices;

						short *v = lb+cst3+j*w; 
							
						for(int k=anim->mNumMeshChannels;k-->0;)
						{
							cmp = anim->mMeshChannels[k];

							if(cmp->mName!=mesh->mName) continue;

							aiAnimMesh *am = 0;
							int kv = cmp->mKeys[j].mValue;
							if(kv!=-1)
							if(kv>=mesh->mNumAnimMeshes //REMOVE US
							||(am=mesh->mAnimMeshes[kv])->mNumVertices!=mesh->mNumVertices) //!
							{ 
								MessageBoxW(X2MDL_MODAL,
								L"A vertex-animation index is out-of-bounds. (Programmer Error.)",
								X2MDL_EXE,MB_OK|MB_ICONERROR);						
								goto_1; 
							}
							aiVector3D *av = kv==-1?mesh->mVertices:am->mVertices; 
														
							//s: running out of subscripts :(
							for(int s=mesh->mNumVertices;s-->0;)
							{
								//REDUNDANT (NEED REVERSE consolidate MAPPING?)

								aiVector3D src = av[s]; //am->mVertices
								if(!x.empty()) src*=xm;
								short *dst = v+3*cv[s];
								assert(dst>=lb&&dst<ub);
								for(int l=0;l<3;l++)
								dst[l] = round(src[l]*invert[l]*scale);
							}
						}
					}

					currentnode++;
				}
			}
				
			//2020: expand Mm3dLoader.cpp like points?
			if(mm3d||!anim->mNumChannels) continue;
												
			short *v = lb;

			//this was wrong??? what was I think?
			//float s = 0, t = -1;
				
			for(int j=0;j<n;j++,v+=w)
			{
				//t+=times[j];

				bool animated = false;

				//s = t;

				int e = w-we;

				for(int i=0;i<mdl.head.parts;i++)
				{
					MDL::Part &pt = *mdl.parts[i].cpart;
					
					if(!pt.extra) continue;
					aiNode *nd = nodetable[pt.cnode];

					int v0 = pt.diff_v0; //REMOVE ME

					//see: consolidate (King's Field II)
					auto *cv = pt.cverts, *cv2 = cv; int cst3 = v0+3*pt.cstart;						

					for(int m=0;m<nd->mNumMeshes;m++,cv=cv2)
					{
						aiMesh *mesh = X->mMeshes[nd->mMeshes[m]];
 							
						cv2 = cv+mesh->mNumVertices;
							
						//HACK: concerned with expanding Mm3dLoader.cpp
						//style point data only
						if(aiPrimitiveType_POINT!=mesh->mPrimitiveTypes)
						{
							assert(~mesh->mPrimitiveTypes&aiPrimitiveType_POINT);
							continue;
						}
							
						if(!animated)
						{
							animated = true;

							//TODO: will need to do better for missing first and last frames
							//x2mdl_animate(anim,&x[0],s,t);
							x2mdl_animate(anim,&x[0],cmp->mKeys[j].mTime);
						}

						for(int k=0;k<mesh->mNumVertices;k++)
						{
							aiVector3D src = mesh->mVertices[k];
							aiVector3D nml(0,0,1);							
							for(aiNode*p=nd;p;p=p->mParent)
							{									
								auto it = xmap.find(p);

								//HACK: Assimp doesn't have a nice way to multiply
								//normals
								auto &m = it!=xmap.end()?x[it->second]:p->mTransformation;
								auto m3 = aiMatrix3x3(m);
								nml*=m3;
								src*=m3; 
								src.x+=m.a4; src.y+=m.b4; src.z+=m.c4;
							}
							/*REFERENCE //???
							*
							*  this was added to match the code above but it's not working
							*  I'm not sure why exactly. one difference is that code deals
							*  with geometry that's inverse transformed to be centered on
							*  the node, whereas these are coming directly from nodes, and
							*  their mesh data is expected to be 1 shared vertex at 0,0,0
							*  anyway, enabling this generates crazy CPs in the cpgen step
							*  I guess it makes some sense since the goal is to get all of
							*  the data into global space (how MDL and MM3D work) which is
							*  already the case for nodes
							* 
							if(xposed) //bindposeinvs
							{
								int currentchan = chanindex?chanindex[pt.cnode]:0;
								auto &m = (*(aiMatrix4x4*)(bindposeinvs+currentchan*16));
								auto m3 = aiMatrix3x3(m);
								nml*=m3;
								src*=m3; 
								src.x+=m.a4; src.y+=m.b4; src.z+=m.c4;
							}*/

							short *dst = v+cst3+3*cv[k];
														
							assert(dst>=lb&&dst<ub);

							for(int l=0;l<3;l++)
							dst[l] = round(src[l]*invert[l]*scale);

							//convert points to CP triangles?
							if(pt.extra) if(e+6<=w)
							{
								short nm2[3]; for(int i=3;i-->0;)
								{
									nm2[i] = nml[i]*invert[i]*4096;
								}

								assert(v+e+6<=ub);

								x2mdl_pt2tri(dst,nm2,v+e,v+e+3); e+=6;
							}
							else assert(e+6<=w);
						}
					}

				//	currentnode++;
				}

				assert(e==w);
			}			
		}
		if(mdl.diffs&&!mo)
		mdl.diffs[currentanim] = NULL; //0-terminated

//		std::wcout << "Debug: Embedding textures.";

		//TODO: need to disable this for MDL+MDO output
		//as well, except for clipping geometry
		if(::mdl&&(!mdo||mdl.anims||mdl.diffs))
		{
			mdl.head.skins = texnames.size(); //2022
			if(mdl.head.skins)
			mdl.skins = new MDL::Skin*[mdl.head.skins+1];
		}
		else mdl.head.skins = 0;

		int currentskin = 0;

		assert(icotextures.empty());
		icotextures.clear(); //goto Y?

		for(size_t i=0;i<texnames.size();i++)
		{
			//aiString path;
			auto it = texmap.begin();
			while(it->second!=i) it++;
			const auto &path = it->first;
			
			if(path.empty()) //TODO: faux texture?
			{
				int yesno = MessageBoxW(X2MDL_MODAL,
				L"Material to texture conversion has not been implemented.\r\n Proceed?",X2MDL_EXE,X2MDL_YESNO);
				
				exit_reason = i18n_unabletocomplete;

				if(yesno!=IDYES) goto_1; goto tex_failure; 
			}
			
			aiTexture *embed = 0; if('*'==path[0])
			{				
				//does scene-preprocessor not handle this?
				int num = atoi(path.c_str()+1);					
				if(num<0||num>=X->mNumTextures||!X->mTextures[num]) //???
				{
					exit_reason = i18n_assimpgeneralfailure; assert(0);
					goto tex_failure;
				}
				embed = X->mTextures[num];
			}
			wchar_t file[MAX_PATH]; if(!embed)
			{
				//HACK: save source for makelink below
				//(and also opening the source file if
				//unable to avoid writing the texture)
				wcscpy(file,texnames[i].c_str());
			}
			else *file = '\0'; //...

			if(texnames_ext)
			{
				std::wstring &f = texnames[i];

				if(embed) //ie. embedded texture
				{	
					f.assign(mdl.name);

					while(f.back()!='.')
					f.pop_back(); f.pop_back();

					#ifdef _CONSOLE
					if(prefix_texture)
					f.insert(PathFindFileNameW(f.c_str())-f.c_str(),prefix_texture);
					#endif
					
					if(X->mNumTextures>1)
					{
						//NOTE: x2mdo_txr will strip image_suf
						//if '.' is used as a separator
						f.push_back('_'); //'.'
						f.push_back('1'+(wchar_t)i); //'0'
					}
				}
				else
				{
					//NOTE: recomputing from scratch since the source may be modified
					//to grope around for nearby textures (i.e. for convenience sake)
					wchar_t buf[MAX_PATH];
					int zt = MultiByteToWideChar(CP_UTF8,0,path.c_str(),-1,buf,MAX_PATH);
					assert(zt>0);
					if(zt<0) zt = 0;
					buf[zt++] = 0;
					while(zt-->0) 
					if(buf[zt]=='\\') buf[zt] = '/'; //???

					if(buf[0]=='.'&&buf[1]=='/') rel2: //???
					{
						int fn = PathFindFileNameW(input)-input;
						f.assign(input,fn).append(buf+2);
					}
					else if(buf==PathFindFileNameW(buf))
					{
						wmemmove(buf+2,buf,MAX_PATH-2); goto rel2;
					}
					else
					{
						//x2mdo.cpp and x2mm3d.cpp handle absolute
						//paths differently
						f.assign(buf); 
					}

					#ifdef _CONSOLE
					if(prefix_texture)
					f.insert(PathFindFileNameW(f.c_str())-f.c_str(),prefix_texture);
					#endif

					#ifndef _CONSOLE
					int fn = PathFindFileNameW(output)-output;
					f.assign(output,fn);					
					if(*buf!='.') //this is generally not a good scenario!
					{
						assert(0); //TODO? should this be disallowed?

						//YUCK: have to convert to relative path (ideally)
						const wchar_t *p = input, *q = buf;
						while(*p==*q&&*p){ p++; q++; }
						while(q>buf&&*q!='/'&&*q!='\\'){ p--; q--; }

						if(*q=='/'||*q=='\\')
						{
							//I feel like there's got to be a better way?
							p++; q++;
							int fn = PathFindFileNameW(input)-input;
							fn = p-input-fn;
							if(fn>=0)
							{
								f.append(q-fn);
							}
							else //backward relative path?
							{
								assert(0);
								f.append(PathFindFileNameW(buf));
							}							
						}
						else //old way? this is not reliable!
						{
							assert(0);
							f.append(PathFindFileNameW(buf));
						}
					}
					else f.append(buf+2);
					#endif

					auto np = f.rfind('.');
					if(!~np) np = 0;
					f.erase(np);

					#ifndef _CONSOLE
					//extensions can modify images in the cache
					if(dll.image_suf)
					f.append(dll.image_suf);
					#endif					
				}
				f.append(texnames_ext);
			}
			
			#ifndef _CONSOLE
			bool write = !dll.havelink(texnames[i].c_str());
			#else
			bool write = update_texture||!PathFileExistsW(texnames[i].c_str());
			#endif
			
			D3DLOCKED_RECT lock = {};

			IDirect3DTexture9 *dt = 0;

			UINT w = 1, h = 1;

			//can avoid reading if not writing			
			if(write||mdl.skins||ico)
			if(d3d_available) if(embed)
			{
				w = embed->mWidth; h = embed->mHeight; if(!h) 
				{
					exit_reason = i18n_compressedtexunsupported;
					goto tex_failure;
				}
			}
			else
			{
				D3DXIMAGE_INFO info;				
				if(D3DXGetImageInfoFromFileW(file,&info))
				{
					exit_reason = i18n_direct3dgeneralfailure;	
					goto tex_failure;
				}
				w = info.Width;	h = info.Height;				
			}			

			//OPTIMIZING
			//might need StretchRect?
			D3DPOOL pool = D3DPOOL_SYSTEMMEM;
			//TODO: omit MDL+MDO skins?
			if(mdl.skins)
			if(w>X2MDL_TEXTURE_MAX||h>X2MDL_TEXTURE_MAX)
			pool = D3DPOOL_DEFAULT;
			if(ico)
			pool = D3DPOOL_DEFAULT; //2023

			if(write||mdl.skins||ico) if(!embed)
			{	
				auto hr = D3DXCreateTextureFromFileExW			
				(pd3Dd9,file,w,h,ico?!ico_mipmaps:1, //2022
				D3DUSAGE_DYNAMIC,//|D3DUSAGE_AUTOGENMIPMAP, //2022
				D3DFMT_X8R8G8B8,pool,
				D3DX_FILTER_NONE,D3DX_FILTER_LINEAR, //D3DX_DEFAULT,0 //2022
				0,0,0,&dt);
				
				icotextures.push_back(dt);

				exit_reason = i18n_direct3dgeneralfailure;
				if(hr) goto tex_failure;
			}
			else
			{
				auto hr = pd3Dd9->CreateTexture
				(w,h,ico?!ico_mipmaps:1,D3DUSAGE_DYNAMIC|D3DUSAGE_AUTOGENMIPMAP,//|D3DUSAGE_AUTOGENMIPMAP,
				D3DFMT_X8R8G8B8,pool,&dt,0);

				icotextures.push_back(dt);

				if(!hr) hr = dt->LockRect(0,&lock,0,0);
				else goto tex_failure;
					
				assert(sizeof(int)==sizeof(char)*4);

				if(lock.Pitch!=4*w)
				{
					void *info = embed->pcData;

					for(int i=0;i<h;i++)
					{
						int *p = (int*)info+w*i;
						int *q = (int*)lock.pBits+lock.Pitch/4*i;

						for(int j=0;j<w;j++) *q++ = *p++;
					}
				}
				else memcpy(lock.pBits,embed->pcData,w*h*4);

				//2022: avoid unlock->lock if possible?
				//dt->UnlockRect(0);
					
				#ifdef _CONSOLE
				if(mm3d) //write BMP file?
				{
					assert(write); write = false; 

					dt->UnlockRect(0); lock.Pitch = 0; //2022

					//TODO: PRESERVE SOURCE FORMAT
					assert(texnames_ext==L".bmp");
					D3DXIMAGE_FILEFORMAT ff = D3DXIFF_BMP;

					auto *f = texnames[i].c_str();

					if(D3DXSaveTextureToFileW(f,ff,dt,0))
					if(x2mdo_makedir((wchar_t*)f))
					if(D3DXSaveTextureToFileW(f,ff,dt,0))
					{
						exit_reason = i18n_generalwritefailure;
						goto tex_failure;
					}

					dt->Release(); dt = 0; //2022
				}
				#endif
			}

			MDL::Skin *curr = 0;

			bool resample = false; //2022

			if(dt) //d3d_available?
			{
				if(mdo||msm) //txr?
				{
					assert(texnames_ext==L".txr");

					#ifndef _CONSOLE
					auto swap = dll.input; //YUCK
					dll.input = file;
					#endif
					if(write)
					{
						//maybe locked from writing embedded texture data?
						HRESULT hr = 0; if(!lock.Pitch)
						hr = dt->LockRect(0,&lock,0,D3DLOCK_READONLY);
						if(hr||!x2mdo_txr(w,h,lock.Pitch,lock.pBits,texnames[i].c_str()))
						{
							exit_reason = 
							hr?i18n_direct3dgeneralfailure:i18n_generalwritefailure;
							goto tex_failure;					
						}
					}
					#ifndef _CONSOLE
					if(write) dll.makelink(texnames[i].c_str());
					dll.input = swap; //YUCK
					#endif
				}

				if(mdl.skins) //256x256?
				{
					UINT ww = w, hh = h;

					if(w>h)
					{
						float r = float(h)/w;

						w = std::min<UINT>(X2MDL_TEXTURE_MAX,w);				
						h = r*w;
					}
					else if(h>w)
					{
						float r = float(w)/h;

						h = std::min<UINT>(X2MDL_TEXTURE_MAX,h);
						w = r*h;
					}
					else
					{
						w = std::min<UINT>(X2MDL_TEXTURE_MAX,w);
						h = std::min<UINT>(X2MDL_TEXTURE_MAX,h);
					}

					resample = ww!=w||hh!=h;

					HRESULT hr = 0; if(resample) //UNTESTED
					{
						if(lock.Pitch)
						{
							lock.Pitch = 0; //UnlockRect? (below)

							dt->UnlockRect(0);
						}

						//TODO: classically SOM converts to POWER-TWO
						//at runtime. it could be done here and also
						//with D3DXCreateTextureFromFileExW however 
						//in my opinion this is ugly and so should be
						//discouraged harshly, especially as it might
						//go unnoticed

						if(!rt) //2022: defer?
						{
							hr = pd3Dd9->CreateRenderTarget 
							(X2MDL_TEXTURE_MAX,X2MDL_TEXTURE_MAX,rtf,D3DMULTISAMPLE_NONE,0,0,&rt,0); 
							if(!hr)
							hr = pd3Dd9->CreateOffscreenPlainSurface
							(X2MDL_TEXTURE_MAX,X2MDL_TEXTURE_MAX,rtf,D3DPOOL_SYSTEMMEM,&rs,0);
							//if(hr) goto d3d_failure;
							if(!hr) hr = pd3Dd9->SetRenderTarget(0,rt);
						}
						if(!hr)
						{
							RECT rect = {0,0,w,h};

							PDIRECT3DSURFACE9 ds;
							dt->GetSurfaceLevel(0,&ds);
							hr = pd3Dd9->StretchRect(ds,0,rt,0,D3DTEXF_LINEAR);
							ds->Release();
						}
						if(!hr) hr = pd3Dd9->GetRenderTargetData(rt,rs);
						if(!hr) hr = rs->LockRect(&lock,0,D3DLOCK_READONLY);
					}
					else if(!lock.Pitch)
					{
						hr = dt->LockRect(0,&lock,0,D3DLOCK_READONLY);
					}

					exit_reason = i18n_direct3dgeneralfailure;
					if(hr) goto tex_failure;
				}

				if(mdl.skins) if(X2MDL_TEXTURE_TPF==0x02) //16bit
				{
					curr = mdl.skins[currentskin] = MDL::Skin::New(w*h*2);
				
					if(rtf==D3DFMT_X8R8G8B8) //new way: support more drivers
					{
						for(int i=0;i<h;i++)
						{
							uint16_t *p = (uint16_t*)curr->info+w*i;
							uint32_t *q = (uint32_t*)lock.pBits+lock.Pitch/4*i;

							for(int j=0;j<w;j++,p++,q++)
							{
								uint8_t *bgrx = (uint8_t*)q;
							
								*p = (bgrx[0]/8)<<10|(bgrx[1]/8)<<5|(bgrx[2]/8);
							}
						}
					}
					else if(lock.Pitch!=w*2)
					{	
						for(int i=0;i<h;i++)
						{
							le16_t *p = (le16_t*)curr->info+w*i;
							le16_t *q = (le16_t*)lock.pBits+lock.Pitch/2*i;

							for(int j=0;j<w;j++) *p++ = *q++;
						}
					}
					else memcpy(curr->info,lock.pBits,w*h*2);
				}
				//incompatible with som runtimes
				else if(X2MDL_TEXTURE_TPF==0x03) //32bit 
				{
					curr = mdl.skins[currentskin] = MDL::Skin::New(w*h*3);

					for(int i=0;i<h;i++)
					{
						char *p = (char*)curr->info+w*i*3;
						char *q = (char*)lock.pBits+lock.Pitch*i;

						for(int j=0;j<w;j++) 
						{
							p[0] = q[2]; p[1] = q[1]; p[2] = q[0]; p+=3; q+=4;
						}
					}
				}
				else assert(0);
			}
			else if(mdl.skins) //EXPERIMENTAL
			{
				w = h = 1;

				curr = mdl.skins[currentskin] = MDL::Skin::New(w*h*3);

				char *p = curr->info;

				p[0] = p[1] = p[2] = 0; p[currentskin%3] = 0xFF; //RGB
			}

			if(curr)
			{	
				curr->index = i;
				curr->width = w; 
				curr->height = h;		
				curr->image = curr->info;

				if(rtf!=D3DFMT_X8R8G8B8)
				if(X2MDL_TEXTURE_TPF==0x02) 
				{
					curr->X1R5G5B5toS1B5G5R5(); 		
				}
			}

			if(lock.Pitch)
			{
				if(rs&&resample) rs->UnlockRect(); 
				if(dt&&!resample) dt->UnlockRect(0);
			}
		//	if(dt) dt->Release();	

			currentskin++; continue;			
			 			
tex_failure: //allow user to discard texture if desired

			assert(!lock.Pitch); //2022

		//	if(dt) dt->Release(); dt = 0; //2022

			int yesno = MessageBoxW(X2MDL_MODAL,
			L"A texture failed to load or otherwise could not be processed.\r\n Discard texture and proceed?",
			X2MDL_EXE,X2MDL_YESNO);
			
			#ifdef _CONSOLE
			if(yesno!=IDYES) goto_1; goto_0; 
			#else
			goto_1 //FIX ME (YUCK)
			#endif

			if(exit_reason)
			{
				//todo: translate/print reason

				std::wcerr << exit_reason;	
			}
			else assert(0);

			if(mdl.head.skins) mdl.head.skins--; 
		}	

		if(mdl.skins)
		mdl.skins[currentskin] = NULL; //0-terminated
		 
		mdl.write = true; 

		std::wcout << "Hello: Saving " << mdl.name <<  "...\n" << std::flush;
		
		#ifndef _CONSOLE
		dll_continue:
		if(exit_reason)
		{
			std::wcerr << exit_reason; //UNFINISHED

			exit_reason = 0;
		}
		#endif
		
		//YUCK: x2mdo will need this :(
		//delete[] snapshotmats;
		delete[] bindposeinvs;		
		//delete[] chanindex; //x2mdo		
		delete[] partindex;

		//delete[] nodetable; //x2mdo
		delete[] nodenames;
		delete[] nodeindex;
		delete[] mo; 

		if(tmd) //2022
		if(mm3d||mdo||msm) //HACK: divide by 1024?
		{
			if(!bindposemats)
			{
				bindposemats = new float[totalchans*16];
				memset(bindposemats,0x00,4*16*totalchans);
				for(int i=totalchans;i-->0;x+=16)
				{
					auto &xx = (float(&)[4][4])*bindposemats;
					xx[0][0] = xx[1][1] = xx[2][2] = xx[3][3] = 1;
				}
			}

			for(int pass=1;pass<=2;pass++)
			if(auto*x=pass==1?bindposemats:snapshotmats)
			{
				for(int i=totalchans;i-->0;x+=16)
				{
					auto &xx = (float(&)[4][4])*x;
					for(int m=4;m-->0;)
					for(int n=3;n-->0;) xx[m][n]*=0.0009765625f;
				}
			}
		}

		bool YY = Y!=0; //HACK

		mdl.flush(); if(Y) //Y? (x2mdo_split_XY)
		{
			(void*&)X->mMaterials = 0; //can Assimp dup aiMaterial?

			delete X; //aiReleaseImport(X);
			
			X = Y; Y = 0; //goto Y; //...
		}
		else if(ico&&!icotextures.empty())
		{
			//if(PathFileExistsW(mdl.name)) //HACK: not mhm only?
			{
				//assert(PathFileExistsW(mdl.name)); //may be mdo only

				if(!rt) pd3Dd9->CreateRenderTarget //2022: defer?
				(X2MDL_TEXTURE_MAX,X2MDL_TEXTURE_MAX,rtf,D3DMULTISAMPLE_NONE,0,0,&rt,0);
				if(!rs)
				pd3Dd9->CreateOffscreenPlainSurface
				(X2MDL_TEXTURE_MAX,X2MDL_TEXTURE_MAX,rtf,D3DPOOL_SYSTEMMEM,&rs,0);

				IDirect3DSurface9 *ss[3] = {rt,rs,ms};

				(msm?x2msm_ico:x2mdo_ico)(ico,mdl.name,pd3Dd9,ss);

				sb->Apply(); //D3DSBT_ALL
			}
		}		

		for(auto&dt:icotextures) dt->Release();
		
		icotextures.clear();

			if(YY) goto Y; //Y
	}	

_0:	//wchar_t ok; std::wcin >> ok; //debugging
		
	for(auto&dt:icotextures) dt->Release();

	if(Y) (void*&)Y->mMaterials = 0; //can Assimp dup aiMaterial?

	// cleanup - calling 'aiReleaseImport' is important, as the library 
	// keeps internal resources until the scene is freed again. Not 
	// doing so can cause severe resource leaking.
	if(X){ aiReleaseImport(X); X = 0; }
//	if(Y){ aiReleaseImport(Y); Y = 0; }
	if(Y){ delete Y; Y = 0; }

	// we added a log stream to the library, it's our job to disable it
	// again. this will definitely release the last resources allocated
	// by Assimp.
	aiDetachAllLogStreams();

	#ifdef _CONSOLE //static?
	if(rt) rt->Release(); //render target
	if(rs) rs->Release(); //render surface
	if(ms) ms->Release(); //render surface
	if(pd3Dd9) pd3Dd9->Release();
	if(pd3D9) pd3D9->Release();
	#else
	if(dll.link) //TODO: static?
	{
		dll.link->Release();
		dll.link2->Release();
	}
	#endif

	//nuisance for real work
	//#ifdef _CONSOLE	
	//Sleep(4000);
	//#endif

	return exit_status;

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

void MDL::File::consolidate()
{
	for(int i=0;i<head.parts;i++) 
	{
		//MO may be using cverts
		//if(parts[i].cverts) return; //already consolidated
		if(parts[i].cnorms) return; //already consolidated
	}				 
	
	short *p = verts, *q = norms, *b = p, *d = q, *cp;
	
	unsigned short *cv;

	for(int i=0;i<head.parts;i++) 
	{
		//2020: vertices may be in different order if 
		//converting/consolidating points to triangle
		//control points
		MDL::Part *pt = parts[i].cpart;

		int v = 0, n = 0;
		
		int v3 = pt->verts*3;		
		int n3 = pt->norms*3;
		
		if(!pt->faces) goto nf; //2022

		if(cv=pt->cverts) //repurposing 
		{					
			//This is to reconstruct the MO file from
			//Assimp's "verbose" format. It's just to
			//be expediant to work on King's Field II.			
			memcpy(cp=new short[v3],b,v3*sizeof(short));
			for(int j=0,k=0;j<v3;j+=3,k++)
			{
				int cvk = cv[k]*3;
				if(cvk>v) v = cvk;
				p[cvk+0] = cp[j+0];
				p[cvk+1] = cp[j+1];
				p[cvk+2] = cp[j+2];
			}
			delete[] cp; if(v3) v+=3; else assert(0);
			goto mo;
		}

		cv = pt->cverts = v3?new unsigned short[pt->verts]:0;		

		//I can't figure out a way to not fuse soft
		//animation vertices without relying on the
		//SP_GEN_CONNECTIVITY hack. at least if the
		//mesh is different artists can get control
		aiNode *nd = nodetable[pt->cnode];

		int jbase = 0, jnext = 0, vbase = 0;
		for(int m=0;m<nd->mNumMeshes;m++,jbase=jnext,vbase=v)
		{
			jnext+=3*X->mMeshes[nd->mMeshes[m]]->mNumVertices;

			//for(int j=0;j<v3;j+=3)
			for(int j=jbase;j<jnext;j+=3)
			{
				//int k; for(k=0;k<v;k+=3)
				int k; for(k=vbase;k<v;k+=3)
				{
					if(p[k+0]!=b[j+0]) continue; 
					if(p[k+1]!=b[j+1]) continue; 
					if(p[k+2]!=b[j+2]) continue; 

					break; //duplicate
				}

				if(k==v)
				{
					p[v++] = b[j+0]; 
					p[v++] = b[j+1]; 
					p[v++] = b[j+2]; 
				}

				*cv++ = k/3; 
			}
		}
		#ifdef _CONSOLE
		if(v3) pt->mm3d_cvertsz = cv-pt->cverts;
		if(v3) assert(pt->verts==pt->mm3d_cvertsz);
		#endif
				
	mo:	//2021: some code assumes if these pointers
		//are nonzero their size is nonzero... they
		//should be dummy_mesh if 0
		assert(pt->verts==pt->norms);

		cv = pt->cnorms = n3?new unsigned short[pt->norms]:0;

		for(int j=0;j<n3;j+=3)
		{
			int k; for(k=0;k<n;k+=3)
			{
				if(q[k+0]!=d[j+0]) continue; 
				if(q[k+1]!=d[j+1]) continue; 
				if(q[k+2]!=d[j+2]) continue; 

				break; //duplicate
			}

			if(k==n)
			{
				q[n++] = d[j+0]; 
				q[n++] = d[j+1]; 
				q[n++] = d[j+2]; 
			}

			*cv++ = k/3; 
		}
				
		//TESTING (FIX ME!!!)
		//
		// multipart (nodes) models can't meet this
		// requirement because the consolidation is
		// in global "space" so the mapping will be
		// to higher indices than there are indices 
		//
	//	#ifdef NDEBUG
	//	#error :(	//I think this is fixed/working?
	//	#endif
		assert(pt->verts>=v/3);
		assert(pt->norms>=n/3);

	nf:	pt->verts = v/3;
		pt->norms = n/3;		
		
		p+=v; b+=v3; 
		q+=n; d+=n3; 
	}
}

void x2mdl_pt2tri(short *v, short *n, short *v1, short *v2)
{
	//todo: should try to get more information from
	//the node if possible (somehow)
	aiVector3D nn(n[0],n[1],n[2]); 
	nn.Normalize();
	aiVector3D up = nn^aiVector3D(1,0,0);
	//up.Normalize();
	float len = up.Length();
	if(len!=0) up = up.SymMul(1/len);
	if(len==0) up.Set(0,-1,0);
	aiQuaternion q120(nn,AI_DEG_TO_RAD(120));

	//1/32nd of a meter radius?	~1.25 inches?
	float scale = 1024/32;

	//reusing normal for now				
	for(int i=0;i<3;i++) 
	v1[i] = round(v[i]+up[i]*scale);
	up = q120.Rotate(up);
	for(int i=0;i<3;i++)
	v2[i] = round(v[i]+up[i]*scale);
	up = q120.Rotate(up);
	for(int i=0;i<3;i++)
	v[i] = round(v[i]+up[i]*scale);
}

//TODO: Big Endian
int8_t  _S8_;
le16_t _S16_;
le32_t _S32_;
uint8_t  _U8_;
ule16_t _U16_;
ule32_t _U32_;	  
#define U8(i) &(_U8_=i),1,1
#define U16(i) &(_U16_=i),2,1
#define U32(i) &(_U32_=i),4,1
#define S8(i) &(_S16_=i),1,1
#define S16(i) &(_S16_=i),2,1
#define S32(i) &(_S32_=i),4,1
bool MDL::File::flush_diffs(FILE *f, std::vector<short> &extra) //2019
{
	int sizer = 4;

	//command line option?
	const bool dummy = false; //TESTING

	//REMOVE ME?
	//
	// e102.MDL starts at t=10. there must be a
	// way to do this?
	//
	// SOM has an off-by-one bug in these cases
	// so that models without a first frame are
	// interpolated as if the first keyframe is
	// at T=0
	//
	//MDL doesn't seem to have a way to represent
	//the base model. KF2's "MO" files default to
	//the base model.
	int dummy_i = -1; 

	int w = diffs[0]->width;
	int cmpN = w*sizeof(short);
	std::vector<short*> unique;
	for(MDL::Diff **p=diffs;*p;p++)
	{
		MDL::Diff *diff = *p;

		if(w!=diff->width)
		{
			assert(w==diff->width); return false; 
		}

		sizer++; //type

		if(dummy) //TESTING
		{
			if(diff->times()[0]>1)
			{
				sizer++; dummy_i = 0; //REMOVE ME
			}
		}

		short *vert1 = diff->verts;		
		short *index = diff->flush();
		for(int i=diff->steps;i-->0;)
		{			 
			short *v = vert1+i*w;
			short *v2 = i?v-w:verts;
			if(v2==verts&&!extra.empty()) //YUCK
			{
				int j,we = w-(int)extra.size();
				for(j=0;j<we;j++) v[j]-=v2[j];
				for(;j<w;j++) v[j]-=extra[j-we];
			}
			else for(int j=0;j<w;j++) v[j]-=v2[j];
		}
		for(int i=0;i<diff->steps;i++,index++,vert1+=w)
		{			 		
			size_t j; for(j=0;j<unique.size();j++)
			{
				short *cmp = unique[j];

				if(0==memcmp(cmp,vert1,cmpN))
				{
					*index = short(j+1); break;
				}

				int k; for(k=0;k<w;k++)
				{
					if(cmp[k]!=-vert1[k]) break;
				}
				if(k==w)
				{
					*index = -short(j+1); break;
				}				
			}
			if(j==unique.size())
			{
				*index = short(j+1); 

				unique.push_back(vert1);
			}

			sizer++; //frame
		}

		sizer++; //0-terminator
	}	
	if(dummy_i==0) 		 
	for(int &i=dummy_i;i<(int)unique.size();i++)
	{
		if(0==*(le32_t*)unique[i])
		if(0==memcmp(unique[i],unique[i]+2,w-4))
		break;
	}
	if(dummy_i==(int)unique.size()) 
	{
		unique.push_back(0);
	}

	fwrite(U16(head.diffs),f);

	bool pad = sizer%2;

	if(pad) sizer++; sizer/=2; //word align

	fwrite(U16(sizer),f);

		//What is this?
		//What is this?
		//What is this?

	//I don't know what the significance of this value
	//is. The game crashes without it. EneEdit/NpcEdit
	//become unstable/crash if the profile is unloaded.
	//
	// The loop at 444353 in som_db.exe looks like it
	// uses both FE and FF as sentinels. I think this
	// may be wrong.
	//
	// FF is the last, FE is padding, but 0??
	//
	fwrite(U32(0xFFFEFE00),f);

	for(MDL::Diff **p=diffs;*p;p++)
	{	
		MDL::Diff *diff = *p;

		fwrite(S16(diff->type),f); 

		short *index = diff->flush();
		short *times = diff->times();

		if(dummy) //TESTING
		{
			if(times[0]>1) //dummy_i
			{
				//NOTE: time starts at 1?
				fwrite(S16(1<<8|dummy_i+1),f); //REMOVE ME
			}
		}

		for(int i=0;i<diff->steps;i++,index++,times++)
		{
			int lo = *index; unsigned int hi = *times;

			if(lo!=(char)lo||hi!=(unsigned char)hi)
			{
				assert(lo==(char)lo);
				assert(hi==(unsigned char)hi);
				return false;
			}

			short frame = (short)(hi<<8|lo&0xff);

			fwrite(S16(frame),f);
		}
		fwrite(S16(0),f);
	}

	if(pad) fwrite(U16(0),f);

	auto pos = ftell(f); 	
	int add = unique.size();
	if(add&1) add++; add*=2;
	fseek(f,add,SEEK_CUR);
	for(int i=0,iN=(int)unique.size();i<iN;i++)
	{
		fseek(f,pos+i*2,SEEK_SET);
		fwrite(U16(add/4),f);
		fseek(f,pos+add,SEEK_SET);

		//pcm is the size of a bitfield holding
		//1 bit per xyz component
		int pcm = w/3/8; 
		if(w/3%8) pcm++;	
		//this is the distance beyond the field
		fwrite(U16(3+pcm*3+1),f);
		//this is unknown... unsure always zero
		fwrite(U8(0),f);
		{	
			//1 in 3+pcm*3+1 is the U8(0) below

			//separating these seems hard on the cache 
			//I can't remember if it makes the algorithm
			//easier to implement. I mean why even have 3?	
			for(int x=0;x<3;x++)
			{
				short *p = unique[i]+x;
				short *d = p+w;
				int word = 0, B = 0;
				if(dummy_i==i)
				{
					//just illustrating. the U8
					//loop would amount to this
					while((p+=3*32)<d)
					{
						fwrite(U32(0),f);
						B+=4;
					}
				}
				else for(int j=0;p<d;p+=3)
				{
					if(*p) word|=1<<j;

					if(++j==32)
					{
						fwrite(U32(word),f);
						j = word = 0; 
						B+=4;
					}
				}

				for(;B++<pcm;word>>=8)
				{
					fwrite(U8(word&0xFF),f); 					
				}
			}
		}
		//this is crazy. it's needed to make MDL work
		//I don't know if it houses anything. but the
		//offset math doesn't work without it and the
		//1B above also
		fwrite(U8(0),f);

		short *p = unique[i];
		short *d = p+w;
		if(p) for(;p<d;p++) //dummy_i may be null
		{			
			//the LSB is just noise... there's no way
			//to compensate. this introduces error if
			//converted to another format and back to
			//MDL again

			if((char)*p==*p)
			{
				if(*p) //breakpoint
				{
					fwrite(S8(*p|1),f);
				}
			}
			else fwrite(S16(*p&~1),f);
		}

		while((add=ftell(f))%4) fwrite(U8(0),f);

		add-=pos;
	}

	return true;
}
void MDL::File::flush()
{
	if(!write||!*name) return;

	write = false;

	//REMOVE ME
	#ifdef NDEBUG
	consolidate(); //always consolidate
	#else //debugging
	consolidate(); //testing
	#endif

	if(Y) //split mdo/mhm?
	{
		if(!x2msm(true))
		{
			#ifndef _CONSOLE
			if(x2mdo_makedir(name)) x2msm(mhm);
			#endif
		}
		return;
	}
	else if(msm) //2022
	{
		if(!x2msm(mhm)) 
		{
			#ifndef _CONSOLE
			if(x2mdo_makedir(name)) x2msm(mhm);
			#endif
		}
		return;
	}
	else if(mdo) //2021
	{
		if(!x2mdo()) //TESTING
		{
			#ifndef _CONSOLE
			if(x2mdo_makedir(name)) x2mdo();
			#endif
		}
		
		#ifndef _CONSOLE
		//need to be sure there's not a scenario where
		//an old MDL file is wrongly paired with a MDO
		if(!mdl) if(wchar_t*o=wcsrchr(name,'.'))
		{
			wcscpy(o,L".*");
			WIN32_FIND_DATAW found;
			HANDLE glob = INVALID_HANDLE_VALUE;
			glob = FindFirstFileW(name,&found);
			if(glob!=INVALID_HANDLE_VALUE)
			{
				do if(~found.dwFileAttributes
				&FILE_ATTRIBUTE_DIRECTORY)
				{
					wchar_t *ext =
					PathFindExtensionW(found.cFileName);

					#ifdef SWORDOFMOONLIGHT_BIGEND
					char mc[4] = {ext[0],ext[1],ext[2],ext[3]};
					#else
					char mc[4] = {ext[3],ext[2],ext[1],ext[0]};
					#endif
					for(int i=4;i-->0;) mc[i] = tolower(mc[i]);

					switch(wcslen(ext)<=4?*(DWORD*)mc:0)
					{
					case '.bp\0': //NOTE: x2mdo removes bp files

					default: continue;

						//note: for SFX where a model file takes
						//precedence over a txr file there isn't
						//a bi-directional solution, so a manual
						//deletion is the only option

					case '.mhm': //if(mhm) continue;
					case '.msm':
					case '.mdl': case '.cp\0':

						wcscpy(o,ext); DeleteFileW(name); break;
					}

				}while(FindNextFileW(glob,&found));

				FindClose(glob);
			}
			wcscpy(o,L".mdl");
		}
		else assert(0);
		#endif

		if(!mdl) return; //x2mdo?

		//don't make MDL unless animated
		//#ifndef _CONSOLE
		if(0==(7&head.flags))
		{
			assert(!anims&&!diffs);
			return;		
		}		
		//#endif
	}
	else assert(mdl||mm3d);
		
	std::vector<short> extra; //goto err?

	//SEEMS MAY NOT MATTER (REMOVE ME?)
	//NOTE: the diff format I think can support multiple parts
	//but it's not well understand and has never been utilized
	MDL::Part diffpt,*pts = diffs?&diffpt:parts;
		
	FILE *f = _wfopen(name,L"wb");

	if(!f) goto err;

	if(mm3d) //DESTRUCTIVE
	{
		//REMINDER: this is modifying cverts so that it can't
		//be later (here) used to output a MDL file since the
		//CP vertices are removed
		x2mm3d(f);
		fclose(f); return;
	}

	using SWORDOFMOONLIGHT::mdl::round;

	assert(0==ftell(f)); //???
	fseek(f,0,SEEK_SET); //???
	
	char real_parts = 0;
	for(int i=0,v=0,n=0;i<head.parts;i++)
	{
		auto &pt = parts[i];

		if(!pt.cextra) real_parts++;

		pt.cextrav = verts+v; v+=3*pt.verts;
		pt.cextran = norms+n; n+=3*pt.norms;
	}
	if(pts!=parts) real_parts = 1; //2021

	fwrite(U8(head.flags),f); //head: 16 bytes total
	fwrite(U8(head.anims),f);
	fwrite(U8(head.diffs),f);
	fwrite(U8(head.skins),f);
	fwrite(U8(real_parts),f); //head.parts

	fwrite(U8(1),f); //1~3 for next 3... (multi-resolution?)
	fwrite(S16(-1),f); //filled in later
	fwrite(U16(0),f); //unclear 16bits (multi-resolution?)
	fwrite(U16(0),f); //unknown 16bits (multi-resolution?)

	fwrite(S16(head.anims?-1:0),f); //filled in later
	fwrite(S16(head.diffs?-1:0),f); //filled in later

	for(int i=0;i<head.parts;i++) //2020
	if(int ce=parts[i].cextra)
	{
		assert(ce>i);

		parts[ce].verts+=parts[i].verts;
		parts[ce].extra+=parts[i].extra;
		parts[ce].norms+=parts[i].norms;
		parts[ce].faces+=parts[i].faces;
	}
	int ptsN; if(pts!=parts) //diffs
	{
		ptsN = 1;

		//REMINDER: x2mdo.cpp assumes merger

		for(int i=0;i<head.parts;i++) //2021
		if(!parts[i].cextra)
		{
			diffpt.verts+=parts[i].verts;
			diffpt.extra+=parts[i].extra;
			diffpt.norms+=parts[i].norms;
			diffpt.faces+=parts[i].faces;
		}
	}
	else ptsN = head.parts;		
	for(int i=0;i<ptsN;i++) //parts: 28 bytes each
	{
		//if(parts[i].cextra) continue; //ptsN?
		if(pts[i].cextra) continue;

		fwrite(S32(-1),f); //filled in later
		fwrite(U32(pts[i].verts+pts[i].extra),f); 
		fwrite(S32(-1),f); //filled in later
		fwrite(U32(pts[i].norms),f);
		fwrite(S32(-1),f); //filled in later
		fwrite(U32(pts[i].faces),f); 
		fwrite(U32(0),f); //unknown 32bits
	}
	//HACK: pts!=parts makes this extremely
	//dicey and hard to insert the logic in
	//the loops (it can be done but's error
	//prone looking)
	auto _write_offset = [&](int at, int part)
	{
		auto pos = ftell(f); 
		
		assert(pos%4==0); //paranoia

		//seek to "at" offset for this part
		fseek(f,16+part*28+at,SEEK_SET);

		fwrite(U32(pos/4-4),f); //subtract head

		fseek(f,pos,SEEK_SET);
	};

	if(tiles)
	{
		assert(0); //unimplemented thus far
	}
	else
	{
		fwrite(U16(0),f); 
		fwrite(U16(0),f);
	}

	//if(!packs) goto err; //???

	int part = 0, cmpart = -1, extraxyz;

	int verts0 = 0, norms0 = 0; //YUCK //diffpt

	if(packs) for(MDL::Pack **p=packs;*p;p++)
	{
		MDL::Pack *pack = *p;

		int part2 = pack->part;

		MDL::Part *pt = parts+part2;

		if(int ce=parts[part2].cextra)
		{
			part2 = ce;
		}

		if(cmpart!=part2)
		{
			//I think the packs should be
			//sorted to simplify animation
			assert(part2>cmpart);

			bool first = cmpart==-1;

			if(pts!=parts&&!first) //YUCK //diffpt
			{
				verts0+=parts[cmpart].verts;
				norms0+=parts[cmpart].norms;
			}

			cmpart = part2;

			assert(cmpart<head.parts); //paranoia

			if(pts==parts||first)
			{				
				extraxyz = parts[cmpart].verts;

				if(first)
				{
					if(pts!=parts) extraxyz = pts->verts;
				}
				else //part face blocks end FFFFFFFF
				{				
					fwrite(S16(-1),f); fwrite(S16(-1),f);
				}

				_write_offset(16,part++);
			}
		}

		fwrite(U16(pack->type),f); 
		fwrite(U16(pack->size),f);
		
		int cst = pt->cstart;
		#define O16(x,vn,i)	U16(vn##0+(pt->c##vn?pt->c##vn[x.vn[i]-cst]+cst:x.vn[i]))

		for(int i=0;i<pack->size;i++) switch(pack->type)
		{
		case 0x00: //3x32bits
		{
			fwrite(U8(pack->x00[i].r),f);
			fwrite(U8(pack->x00[i].g),f);
			fwrite(U8(pack->x00[i].b),f);
			fwrite(U8(pack->x00[i].c),f);

			//EXPERIMENTAL
			//build an old school CP triangle out of point data
			auto *cp = pack->x00[i].verts;
			if(cp[0]==cp[1]&&cp[0]==cp[2])
			{
				//run vertex through the consolidate?
				int ccp0 = pt->cverts?pt->cverts[cp[0]-cst]+cst:cp[0];
				int ccpN = pt->cnorms?pt->cnorms[cp[0]-cst]+cst:cp[0];

				short *v = pt->cextrav+ccp0-cst;
				short *n = pt->cextran+ccpN-cst;

				//TODO: need bivector to match x2mm3d_convert_points
				//convention to roundtrip MM3D->MDL->MM3D
				extra.insert(extra.end(),6,0);
				x2mdl_pt2tri(v,n,&extra.back()-5,&extra.back()-2);

				fwrite(U16(norms0+ccpN),f);
				fwrite(U16(verts0+ccp0),f);
				fwrite(U16(extraxyz++),f);
				fwrite(U16(extraxyz++),f);
			}
			else
			{
				fwrite(O16(pack->x00[i],norms,0),f);
				fwrite(O16(pack->x00[i],verts,0),f);
				fwrite(O16(pack->x00[i],verts,1),f);
				fwrite(O16(pack->x00[i],verts,2),f);
			}
			break;
		}
		case 0x03: //4x32bits
		{
			fwrite(U8(pack->x03[i].r),f);
			fwrite(U8(pack->x03[i].g),f);
			fwrite(U8(pack->x03[i].b),f);
			fwrite(U8(pack->x03[i].c),f);

			fwrite(O16(pack->x03[i],norms,0),f);
			fwrite(O16(pack->x03[i],verts,0),f);
			fwrite(O16(pack->x03[i],norms,1),f);
			fwrite(O16(pack->x03[i],verts,1),f);
			fwrite(O16(pack->x03[i],norms,2),f);
			fwrite(O16(pack->x03[i],verts,2),f);

			break;
		}
		case 0x04: //6x32bits
		{
			auto &f1 = pack->x04[i].flags[1];
			//assert((f1&0x1F)<8);
			//int tpn = framebuffer[f1&0x1F]; //2022
			int tpn = f1&0x1F;
			f1&=~0x1F; f1|=tpn; //hack! //???			

			fwrite(U16(pack->x04[i].comps[0]),f);
			fwrite(U16(pack->x04[i].flags[0]),f); //UNUSED
			fwrite(U16(pack->x04[i].comps[1]),f);
			fwrite(U16(pack->x04[i].flags[1]),f);			
			fwrite(U16(pack->x04[i].comps[2]),f);
			fwrite(U16(pack->x04[i].flags[2]),f);
			fwrite(O16(pack->x04[i],norms,0),f);
			fwrite(O16(pack->x04[i],verts,0),f);
			fwrite(O16(pack->x04[i],norms,1),f);
			fwrite(O16(pack->x04[i],verts,1),f);
			fwrite(O16(pack->x04[i],norms,2),f);
			fwrite(O16(pack->x04[i],verts,2),f);	

			break;
		}
		default: assert(0); goto err; //unimplemented
		}

		#undef O16
	}	
	
	//part face blocks end FFFFFFFF
	fwrite(S16(-1),f); fwrite(S16(-1),f);

	int currentvertxyz = extraxyz = 0;

	if(pts!=parts) //diffpt?
	{
		//it's clearn to get this out of the way
		_write_offset(0,0);
	}
	for(int i=part=0;i<head.parts;i++)
	{
		if(parts[i].cextra) continue;
	
		if(pts==parts)
		{
			//seek to vertex offset for this part
			_write_offset(0,part++);
		}		

		for(int j=parts[i].verts;j-->0;)
		{
			fwrite(S16(verts[currentvertxyz++]),f);
			fwrite(S16(verts[currentvertxyz++]),f);
			fwrite(S16(verts[currentvertxyz++]),f);
			fwrite(S16(0),f); 
		}

		if(pts==parts)
		for(int j=parts[i].extra;j-->0;)
		{
			fwrite(S16(extra[extraxyz++]),f);
			fwrite(S16(extra[extraxyz++]),f);
			fwrite(S16(extra[extraxyz++]),f);
			fwrite(S16(0),f); 
		}
	}
	if(pts!=parts)
	for(int j=0;j<extra.size();j+=3)
	{
		fwrite(S16(extra[extraxyz++]),f);
		fwrite(S16(extra[extraxyz++]),f);
		fwrite(S16(extra[extraxyz++]),f);
		fwrite(S16(0),f); 
	}
		
	int currentnormxyz = 0;

	if(pts!=parts) //diffpt?
	{
		//it's cleaner to get this out of the way
		_write_offset(8,0);
	}
	for(int i=part=0;i<head.parts;i++)
	{
		if(parts[i].cextra) continue;

		if(pts==parts)
		{
			//seek to normal offset for this part
			_write_offset(8,part++);
		}

		for(int j=0;j<parts[i].norms;j++)
		{
			fwrite(S16(norms[currentnormxyz++]),f);
			fwrite(S16(norms[currentnormxyz++]),f);
			fwrite(S16(norms[currentnormxyz++]),f);
			fwrite(S16(0),f);
		}
	}

	std::wcout << "Statistics: "
	<< "Total vertices/normals out (" << currentvertxyz/3 << "/" << currentnormxyz/3 << ")\n";

	auto pos = ftell(f);

	assert(pos%4==0); //paranoia

	//seek to animation offset
	fseek(f,6,SEEK_SET);

	fwrite(U16(pos/4-4),f);

	fseek(f,pos,SEEK_SET);
	
	if(anims)
	{
		bool first = true;

		int maxchannels = -1;
		for(Anim **p=anims;*p;p++)
		{
			Anim *anim = *p;
			int cmp = std::max(anim->size/anim->steps,maxchannels);

			//I'm thinking of removing this
			assert(cmp==maxchannels||maxchannels==-1);
			maxchannels = cmp;
		}

		//NOTE: this is read as a byte
		//00445050 8A 06                mov         al,byte ptr [esi]
		fwrite(U16(maxchannels),f); 		
		//NOTE: this is read as a word
		//unknown 16bits (always 1)
		//I guess this an extension mechanism. The code that reads it
		//just jumps to the very next bytes to position the read head
		//WILL NEED TO SEE IF ALL OF THE PROGRAMS RESPECT IT SOMETIME
		//(som_db.exe appears to fail this test. arm.mdl animates but
		//its geometry in invisible. the weapon is visible. it may be
		//some subroutine expects to find data here.)
		//00445066 8D 74 8E FE          lea         esi,[esi+ecx*4-2]
		fwrite(U16(1),f);

		for(MDL::Anim **p=anims;*p;p++)
		{
			MDL::Anim *anim = *p;

			fwrite(S16(anim->type),f); //U16 //EXPERIMENTAL //2023 
			fwrite(U16(anim->steps),f);
			
			int stride = anim->steps;
			int nchans = anim->size/stride;

			for(int i=0;i<stride;i++)
			{
				int top = ftell(f);

				fwrite(S16(-1),f); //words (filled in later)
				fwrite(S16(-1),f); //diffs (filled in later)

				int diffs = 0; 
				
				uint8_t (*s)[3], (*s2)[3];
				uint8_t s111[3] = {128,128,128};

				for(int j=0;j<maxchannels;j++)
				{
					/*I think is not longer useful...
					//the first frames have to undo
					//the bind pose
					if(i&&!getmask(anim->chans,128,j))
					continue;*/

					//if(i&&j>=nchans) continue; //???
					if(i&&j>=nchans) break; 

					short *p = anim->info[j*stride+i];

					if(s=anim->scale)
					{
						s+=j*stride+i; if(i!=0)
						{
							s2 = s-1;

							if((*(_int32*)s&0xFFFFFF)
							==(*(_int32*)s2&0xFFFFFF)) s = 0;
						}
						else if(0x808080==(*(_int32*)s&0xFFFFFF))
						{
							s = 0; 
						}
						else s2 = &s111;
					}

					if(i&&!p[0]&&!p[1]&&!p[2] //i???
					&&!p[3]&&!p[4]&&!p[5]&&!s) continue;

					diffs++;

					fwrite(U8(j),f);

					int diffmask = 0, bitsmask = 0;

					if(j<nchans)
					{	
						for(int k=0;k<6;k++) if(p[k])
						{
							diffmask|=1<<k; 
							bitsmask|=abs(p[k])<127?1<<k:0;
						}
						if(s) //scaling?
						{
							if((*s)[0]!=(*s2)[0]) diffmask|=64;
							if((*s)[1]!=(*s2)[1]) diffmask|=128;
							if((*s)[2]!=(*s2)[2]) bitsmask|=64;
						}

						fwrite(U8(diffmask),f);
						fwrite(U8(bitsmask),f);
					}
					else fwrite(U16(0),f);

					if(first) //include hierarchy info
					{
						fwrite(U8(chans[j]),f);
					}	

					if(diffmask)
					for(int k=0;k<6;k++) if(diffmask&1<<k)
					{
						if(bitsmask&1<<k)
						fwrite(S8(p[k]),f);
						else fwrite(S16(p[k]),f);
					}
					if(s)
					{
						if(diffmask&64) fwrite(U8((*s)[0]),f);
						if(diffmask&128) fwrite(U8((*s)[1]),f);
						if(bitsmask&64) fwrite(U8((*s)[2]),f);
					}
				}

				int end = ftell(f);

				int align = end%4?4-end%4:0; end+=align;

				while(align-->0) fwrite(U8(0),f);
				
				int words = (end-top)/4+1;

				fseek(f,top,SEEK_SET);

				fwrite(U16(words),f);
				fwrite(U16(diffs),f);

				fseek(f,end,SEEK_SET);

				fwrite(U32(words),f); //reverse iteration

				first = false;
			}
		}

		assert(!diffs); //2019
	}
	else if(diffs) //2019
	{
		if(!flush_diffs(f,extra)) goto err; //subroutine
	}

	auto add = ftell(f)-pos; pos = ftell(f);
				
	assert(add%4==0); //paranoia
	assert(pos%4==0); //paranoia

	//seek to sizeof animation block
	fseek(f,12,SEEK_SET);

	//SOM doesn't support both. were it to soft-animations seem to 
	//work out of a common buffer
	int animwords = diffs?0:add/4;
	int diffwords = diffs?add/4:0;

	fwrite(U16(animwords),f); 
	fwrite(U16(diffwords),f); 

	fseek(f,pos,SEEK_SET);
		  
	if(skins)
	for(MDL::Skin**p=skins;*p;p++)
	{
		MDL::Skin *skin = *p;

		if(skin->index>31) continue;

		fwrite(U8(0x10),f); //always 0x10
		fwrite(U8(0),f); //version: always 0
		fwrite(U16(0),f); //reserved
		fwrite(U8(X2MDL_TEXTURE_TPF),f); //TIM pmode/clut S1B5G5R5
		fwrite(U8(0),f); //reserved
		fwrite(U16(0),f); //reserved

		fwrite(U32(12+skin->width*skin->height*X2MDL_TEXTURE_TPF),f); //bnum

		//REMINDER: once UVs that were more than 1 caused EneEdit, 
		//SOM_MAP, and som_db to change the texture. this must mean
		//they are not using the TPN by itself
		//
		//TODO? eliminate untextured materials
		//2020: why 12,28,8,24?
		//int framebuffer[32] = {12,28,8,24}; //todo: not good enough
		//const int framebuffer[32] = {0,4,8,12,16,20,24,28}; 
		//2022: REMOVE ME
		//int tpn = framebuffer[skin->index];
		int tpn = skin->index;

		int dx = (tpn>15?tpn-16:tpn)*64;
		int dy =  tpn>15?256:0;

		fwrite(U16(dx),f); //horizontal position in framebuffer
		fwrite(U16(dy),f); //vertical position in framebuffer

		fwrite(U16(skin->width*X2MDL_TEXTURE_TPF/2),f);
		fwrite(U16(skin->height),f);
			
		fwrite(skin->image,X2MDL_TEXTURE_TPF,skin->width*skin->height,f);	

		while((add=ftell(f))%4) fwrite(U8(0),f); //HACK
	}

	fclose(f);

	//2021: always outputting a cp file for now since there
	//will be BP and MDO files outputted unless some special
	//mode appears
	if(mdo)
	{
		const wchar_t *args[2] = {L"x2mdl",name}; //I guess
		cpgen(2,args);
	}	
	else assert(0);

	return;

err: error = true;

	return;
}
#undef U8
#undef U16
#undef S8
#undef S16 
#undef U32

void MDL::File::mo(le32_t *mo)
{
	#ifdef _CONSOLE //2021

	using SWORDOFMOONLIGHT::mdl::round;

	int tmd = mo[2]; 

	if(12>=tmd||!parts||!parts[0].verts) 
	{
		return;
	}
	
	int *anims = mo+5; assert(0x14==mo[4]);
	
	int m = mo[1]; 
	int w = 3*parts[0].verts;
	
	//I guess this is just the current maximum???
	//(Dias has 14. 10 was the previous top out.)
	if(!m||m>14){ assert(m&&m<=14); return; }
	
	diffs = new MDL::Diff*[m+1](); //0-terminated

	void *b = mo+m, *e = mo+tmd/4;
	#define goto_err_if(x) \
	if(x<b||x>=e){ assert(x<b&&x>=e); goto err; }

	int *dataoff = mo+mo[3]/4; goto_err_if(dataoff)

		//int test = 0;

	for(int i=0;i<m;i++)
	{
		int *anim = mo+anims[i]/4; goto_err_if(anim)
		 
		int n = anim[0]; //30?
		if(!n||n>30){ assert(n&&n<=30); goto err; }

		int compile[sizeof(__int16)==sizeof(short)];

		//hack: not sure how to interpret this???
		//note: the fisherman NPC (Jose?) requires
		//this interpretation
		short *ending = (short*)mo+anim[n]/2;		
		bool reversed = ending[0]==1;
		short endtime = short(ending[1]/4096.0f*30+0.5f);
		if(reversed&&n==1)
		{
			ending[1] = 0; n++; //add frame to beginning?
		}

		MDL::Diff *diff = MDL::Diff::New(w,n);
		diffs[i] = diff;

		//hack: this makes no sense???
		if(reversed) n--; 

		diff->type = i; //diff->time = 10; 
		
		short *times = diff->times()-1; 

		short *v = diff->verts; for(int j=1;j<=n;j++)
		{
			short *frame = (short*)mo+anim[j]/2; goto_err_if(frame)

			//I don't know what the real timing is here!!
			//NOTE: 30 is way too fast for the stone door
			//NOTE: 60 seems too slow for most everything
			times[j] = round(frame[1]/4096.0f*30); //30 //round

			/*NOT WORKING FOR KRAKEN'S IDLE ANIMATION (it uses 0,0)
			//this is pure madness???
			int not_a_clue = frame[2]; //3
			*/
			//int not_a_clue = test++; //frame[4]; //0
			//int not_a_clue = std::max(frame[0],frame[2]);
			//this gets the kraken hit and idles animations but has
			//problems with the end of the attack animations
			//int not_a_clue = test++;
			//
			//HACK??? this works for the kraken hit and idle
			//but it doesn't make any sense
			//int not_a_clue = frame[0]+frame[2]; //death doesn't work
			//int not_a_clue = frame[0]?frame[0]:frame[2]; //works?
			int not_a_clue = frame[2]; //doesn't work for fisherman NPC
			//how can this work for both hit and idle???
			//it has multiple -32768 entries but maybe they're vertices
			//that don't move in either case
			
			short *q = (short*)mo+dataoff[not_a_clue]/2; goto_err_if(q)

			/*how the heck can this work???
			short *reverse = 0; if(frame[0])
			{
				assert(1==frame[0]);

				reverse = verts; //I don't know, relative to what else?
			}*/

			short *prev = j==1?verts:diff->verts+w*(j-2);

			int w2 = 0; q++;
			for(int k=q[-1];k-->0&&w2<w;)
			{
				if(q[0]==-32768) //0x8000 //SHRT_MIN
				{
					//short *v2 = verts+w2;
					short *v2 = prev+w2;
					int w3 = w2+3*q[1]; 
					assert(w3<=w);
					for(w3=std::min(w,w3);w2<w3;w2++)
					{
						*v++ = *v2++; 
					}
					q+=2; 
				}
				else
				{					
					for(int i=0;i<3;i++)
					{
						short x = *q++; 
						
				//		if(reverse) x = prev[w2+i]-(x-reverse[w2+i]);

						*v++ = round(x*scale);
					}

					w2+=3; //k is in sets of 3
				}

				if(q>e){ assert(q<=e); goto err; } //goto_err_if(q)
			}
			//assert(w2==w); //secret door hits this, looks legit
			short *v2 = verts+w2;
			for(;w2<w;w2++) *v++ = *v2++; 
		}		

		if(reversed)
		{
			times[n+1] = endtime;

			memcpy(v,verts,w*sizeof(*v));
		}
	}
	#undef goto_err_if
	if(0) err:
	{
		for(int i=0;i<m;i++)
		delete[] diffs[i];
		delete[] diffs; diffs = 0; return;
	}  
	head.flags = 4; head.diffs = m;	

	#endif //_CONSOLE
}

void x2mdl_animate(aiAnimation *anim, aiMatrix4x4 *x, float t)
{
	//THIS CODE IS BASED ON AnimEvaluator.cpp FROM THE assimview PROJECT

		//TODO: will need to do better for missing first and last frames

	// calculate the transformations for each animation channel
	for( unsigned int a = 0; a < anim->mNumChannels; a++)
	{
		const aiNodeAnim* channel = anim->mChannels[a];

		// ******** Position *****
		aiVector3D presentPosition( 0, 0, 0);
		if( channel->mNumPositionKeys > 0)
		{
			// Look for present frame number. Search from last position if time is after the last time, else from beginning
			// Should be much quicker than always looking from start for the average use case.
		//	unsigned int frame = (t >= s) ? mLastPositions[a].get<0>() : 0;
			unsigned int frame = 0;
			while( frame < channel->mNumPositionKeys - 1)
			{
				if( t < channel->mPositionKeys[frame+1].mTime)
					break;
				frame++;
			}

			// interpolate between this frame's value and next frame's value
			unsigned int nextFrame = (frame + 1) % channel->mNumPositionKeys;
			const aiVectorKey& key = channel->mPositionKeys[frame];
			const aiVectorKey& nextKey = channel->mPositionKeys[nextFrame];
			double diffTime = nextKey.mTime - key.mTime;
			if( diffTime < 0.0)
				diffTime += anim->mDuration;
			if( diffTime > 0)
			{
				float factor = float( (t - key.mTime) / diffTime);
				presentPosition = key.mValue + (nextKey.mValue - key.mValue) * factor;
			} 
			else
			{
				presentPosition = key.mValue;
			}
			
			// Hack: just undoing interpolation work
			if(channel->mInterpolate==aiAnimInterpolate_NONE) 
			{
		//		presentPosition = key.mValue;
			}

		//	mLastPositions[a].get<0>() = frame;
		}

		// ******** Rotation *********
		aiQuaternion presentRotation( 1, 0, 0, 0);
		if( channel->mNumRotationKeys > 0)
		{
		//	unsigned int frame = (t >= s) ? mLastPositions[a].get<1>() : 0;
			unsigned int frame = 0;
			while( frame < channel->mNumRotationKeys - 1)
			{
				if( t < channel->mRotationKeys[frame+1].mTime)
					break;
				frame++;
			}

			// interpolate between this frame's value and next frame's value
			unsigned int nextFrame = (frame + 1) % channel->mNumRotationKeys;
			const aiQuatKey& key = channel->mRotationKeys[frame];
			const aiQuatKey& nextKey = channel->mRotationKeys[nextFrame];
			double diffTime = nextKey.mTime - key.mTime;
			if( diffTime < 0.0)
				diffTime += anim->mDuration;
			if( diffTime > 0)
			{
				float factor = float( (t - key.mTime) / diffTime);
				aiQuaternion::Interpolate( presentRotation, key.mValue, nextKey.mValue, factor);
			}
			else
			{
				presentRotation = key.mValue;
			}

			// HACK: just undoing interpolation work
			if(channel->mInterpolate==aiAnimInterpolate_NONE) 
			{
		//		presentRotation = key.mValue;
			}

		//	mLastPositions[a].get<1>() = frame;
		}

		// ******** Scaling **********
		aiVector3D presentScaling( 1, 1, 1);
		if( channel->mNumScalingKeys > 0)
		{
		//	unsigned int frame = (t >= s) ? mLastPositions[a].get<2>() : 0;
			unsigned int frame = 0;
			while( frame < channel->mNumScalingKeys - 1)
			{
				if( t < channel->mScalingKeys[frame+1].mTime)
					break;
				frame++;
			}

			/*
			#error nothing?
			// TODO: (thom) interpolation maybe? This time maybe even logarithmic, not linear
			presentScaling = channel->mScalingKeys[frame].mValue;
		//	mLastPositions[a].get<2>() = frame;
			*/
			// interpolate between this frame's value and next frame's value
			unsigned int nextFrame = (frame + 1) % channel->mNumScalingKeys;
			const aiVectorKey& key = channel->mScalingKeys[frame];
			const aiVectorKey& nextKey = channel->mScalingKeys[nextFrame];
			double diffTime = nextKey.mTime - key.mTime;
			if( diffTime < 0.0)
				diffTime += anim->mDuration;
			if( diffTime > 0)
			{
				float factor = float( (t - key.mTime) / diffTime);
				presentScaling = key.mValue + (nextKey.mValue - key.mValue) * factor;
			} 
			else
			{
				presentScaling = key.mValue;
			}
		}

		// build a transformation matrix from it
		aiMatrix4x4& mat = x[a];
		mat = aiMatrix4x4( presentRotation.GetMatrix());
		mat.a1 *= presentScaling.x; mat.b1 *= presentScaling.x; mat.c1 *= presentScaling.x;
		mat.a2 *= presentScaling.y; mat.b2 *= presentScaling.y; mat.c2 *= presentScaling.y;
		mat.a3 *= presentScaling.z; mat.b3 *= presentScaling.z; mat.c3 *= presentScaling.z;
		mat.a4 = presentPosition.x; mat.b4 = presentPosition.y; mat.c4 = presentPosition.z;
		//mat.Transpose();
	}
}

#ifndef _CONSOLE
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	if(ul_reason_for_call==DLL_PROCESS_ATTACH)
	{	
		//NOP
	}
	else if(ul_reason_for_call==DLL_PROCESS_DETACH)
	{
		//NOP
	}

	return TRUE;
}
wchar_t *x2mdl_dll::output(wchar_t(&p)[MAX_PATH+4], const wchar_t *input)
{
	//NOTE: step is before the job runs (no big deal)
	if(progress) SendMessage(progress,WM_USER+5,0,0); //PBM_STEPIT

	exit_code = -1; 
	exit_reason = L"x2mdl_dll::output generic error reason.\r\n";

	const wchar_t *a,*b,*c;

		//detect copy mode
			
	c = *input=='"'?input+1:0;

	int e = c?_wtoi(c):0;

		//find data folder

	auto data = data_begin; if(c) //copy mode?
	{
		while(isdigit(*c)) c++;

		data+=e>>x2mdl_h::_data_shift;

		if(data<data_end)
		{
			for(a=*data;*a;) a++;
		}
		else return 0;
	}
	else
	{
		while(data<data_end)
		{
			a = *data; b = input;
			while(*a==*b&&*a){ a++; b++; }
			if(*a) data++; else break;
		}

		if(data==data_end) return 0;
		if(*b!='\\'&&*b!='/') return 0;
	}

		//start with "art" cache folder

	int i,j;
	for(i=0;i<MAX_PATH;i++)
	if(!(p[i]=cache_dir[i]))
	break;
	p[i++] = '\\';

		//append singular data folder

	int n = a-*data; a-=n;
	for(j=0;i<MAX_PATH&&j<n;i++,j++) switch(a[j])
	{
	default: p[i] = a[j]; break;
		//NOTE: this code comes from Somversion ("Somlegalize")
		//(they don't technically have to match)
	case '/': case '\\': 
	case '<': case '>': case ':': case '"': case '|':
	case '*': case '?': p[i] = '-'; break;
	case ' ': p[i] = '_'; break; //preference
	}
		//append path past data folder

	if(c) //copy mode?
	{
		if(*c++!='|') return 0;

		p[i++] = '\\';

		while(i<MAX_PATH&&*c&&*c!='|')
		{
			p[i++] = *c++;
		}

		if(*c++!='|') return 0;
	}
	else for(;i<MAX_PATH&&input[j];i++,j++)
	{
		p[i] = input[j];
	}
	p[i] = '\0';

		//append file name/extension

	const wchar_t *model = L".mdl";
	const wchar_t *ext = model;

	if(!c) for(a=p+i-1;a>p;a--) if(*a=='/'||*a=='\\')
	{
		while(*a&&*a!='.') a++;

		//YUCK: SFX data has some models that are
		//just TXR files, but BMP should work too
		if(*a) switch(tolower(a[1]))
		{
		case 't': //SFX?				
		if(!wcsicmp(a,L".txr")) ext = L".txr"; break;
		case 'b': //SFX?				
		if(!wcsicmp(a,L".bmp")) ext = L".txr"; break;
		case 'p':
		if(!wcsicmp(a,L".png")) ext = L".txr"; break;
		case 'd':
		if(!wcsicmp(a,L".dib")) ext = L".txr"; break; //SOM?
		}			
		break;
	}

	if(i>=MAX_PATH-1) return 0;

		//choose how to handle job

	this->input = input; //YUCK //makelink?

	if(c) //copy?
	{
		copy(e,p,c); return 0;
	}
	else if(ext!=model&&!wcsicmp(a,L".txr")) //txr?
	{
		//could check filetimes, but can't guarantee synchronization
		if(CopyFileW(input,p,0))
		{
			makelink(p);

			exit_code = 0;
		}
		return 0;
	}
	else if(ext!=model) //x2mdo_txr?
	{
		wmemcpy(const_cast<wchar_t*>(a),ext,5);

		IDirect3DTexture9 *pt = 0; D3DXIMAGE_INFO info;

		HRESULT hr = D3DXGetImageInfoFromFileW(input,&info);

		UINT w = info.Width, h = info.Height;

		//TODO? passing 0,0 converts to POWER-TWO dimension
		//however in my opinion this is ugly and so should
		//be discouraged, even though SOM did so at runtime
		//originally harshly, especially since it might go
		//unnoticed (NOTE NPT cannot be mipmapped normally
		//by SomEx.dll)
		if(!hr&&!D3DXCreateTextureFromFileExW			
		(d3d9d,input,w,h,1,0,D3DFMT_X8R8G8B8,D3DPOOL_SYSTEMMEM,
		D3DX_FILTER_NONE, //D3DX_DEFAULT, //distorting color!
		0,0,0,0,&pt))
		{
			D3DLOCKED_RECT lock;

			if(!(hr=pt->LockRect(0,&lock,0,0)))
			{
				if(x2mdo_txr(w,h,lock.Pitch,lock.pBits,p))
				{
					makelink(p);
				}
				else hr = !S_OK; //HACK

				pt->UnlockRect(0);
			}
		}

		if(pt) pt->Release();

		if(!hr) exit_code = 0; return 0;
	}
	else wmemcpy(const_cast<wchar_t*>(a),ext,5);

	exit_code = 0; return p;
}
void x2mdl_dll::copy(int e, wchar_t p[MAX_PATH], const wchar_t *b)
{
	wchar_t w[MAX_PATH+4];
	wchar_t *pf = PathFindFileNameW(p);
	wchar_t *pe = PathFindExtensionW(pf);
	wchar_t *be = w+(PathFindExtensionW(b)-b);
	b = wmemcpy(w,b,be-w);

	int err = 0;
	auto f = [&](const wchar_t *ext)
	{
		wcscpy(be,ext); wcscpy(pe,ext);

		//could check filetimes, but can't guarantee synchronization
		if(!CopyFileW(b,p,0))
		if(x2mdo_makedir(p)&&!CopyFileW(b,p,0))
		{
			err = 1; //should it try to generate the texture or not?
		}
	};

	using namespace x2mdl_h;

	//need to be sure there's not a scenario where
	//an old MDL file is wrongly paired with a MDO
	//if(e&(_mdo|_bp))
	{
		wcscpy(pe,L".*");
		WIN32_FIND_DATAW found;
		HANDLE glob = INVALID_HANDLE_VALUE;
		glob = FindFirstFileW(p,&found);
		if(glob!=INVALID_HANDLE_VALUE)
		{
			do if(~found.dwFileAttributes
			&FILE_ATTRIBUTE_DIRECTORY)
			{
				wchar_t *ext = found.cFileName+(pe-pf);

				#ifdef SWORDOFMOONLIGHT_BIGEND
				char mc[4] = {ext[0],ext[1],ext[2],ext[3]};
				#else
				char mc[4] = {ext[3],ext[2],ext[1],ext[0]};
				#endif
				for(int i=4;i-->0;) mc[i] = tolower(mc[i]);

				switch(wcslen(ext)<=4?*(DWORD*)mc:0)
				{
				default: continue;

				case '.mdl': if(e&_mdl) continue;
				case '.mdo': if(e&_mdo) continue;
				case '.mhm': if(e&_mhm) continue;
				case '.msm': if(e&_msm) continue;
				case '.txr': if(e&_txr) continue;
				case '.bp\0': if(e&_bp); continue;
				case '.cp\0': if(e&_cp); continue;
				}

				wcscpy(pe,ext); DeleteFileW(p);

			}while(FindNextFileW(glob,&found));

			FindClose(glob);
		}
	}

	if(e&_mdl) f(L".mdl");
	if(e&_mdo) f(L".mdo");
	if(e&_bp) f(L".bp");
	if(e&_cp) f(L".cp");
	if(e&_mhm) f(L".mhm");
	if(e&_msm) f(L".msm");
	//this is just to prevent double copying
	//below (assuming model has the texture)
	if(e&_txr&&0==(e&(_mdo|_msm))) f(L".txr");
	if(e&(_mdo|_msm))
	{
		namespace mdo = SWORDOFMOONLIGHT::mdo;
		namespace msm = SWORDOFMOONLIGHT::msm;

		mdo::image_t img1; msm::image_t img2;

		img1.file = img2.file = 0; //HACK
		
		if(e&_mdo) //DUPLICATE
		{
			wcscpy(be,L".mdo");
			mdo::maptofile(img1,b,'r',1);
		}
		if(e&_msm) //DUPLICATE
		{
			wcscpy(be,L".msm");
			msm::maptofile(img2,b,'r',1);
		}
		
		wchar_t txr[64];

		while(pe>p&&*pe!='\\'&&*pe!='/') pe--; pe++;
		while(be>b&&*be!='\\'&&*be!='/') be--; be++;

		if(img1.file) //DUPLICATE
		{				
			if(mdo::textures_t&t=mdo::textures(img1))
			{
				const char *q = t.refs;
				if(int rem=t.count)
				for(int i=0;(void*)q<(void*)img1.end;)
				if(!(txr[i++]=*q++))
				{
					while(i&&txr[i-1]!='.') i--;

					if(txr[i-1]=='.') wmemcpy(txr+i,L"txr",4);

					i = 0; f(txr); if(!--rem) break;
				}
			}
			else err = 1; mdo::unmap(img1);
		}
		if(img2.file) //DUPLICATE
		{
			if(msm::textures_t&t=msm::textures(img2))
			{
				const char *q = t.refs;
				if(int rem=t.count)
				for(int i=0;(void*)q<(void*)img2.end;)
				if(!(txr[i++]=*q++))
				{
					while(i&&txr[i-1]!='.') i--;

					if(txr[i-1]=='.') wmemcpy(txr+i,L"txr",4);

					i = 0; f(txr); if(!--rem) break;
				}
			}
			else err = 1; msm::unmap(img2);
		}
	}

	if(!err) exit_code = 0;
}
void x2mdl_dll::makelink(const wchar_t *dest)
{
	HANDLE h = CreateFileW(input,GENERIC_READ,0,0,OPEN_EXISTING,0,0);
	FILETIME ft;
	if(GetFileTime(h,0,0,&ft))
	{
		//NEEDS CLEAN UP
		wchar_t *fn = PathFindFileNameW(input);
		wchar_t *ext = wcschr(fn,'.');
		if(!ext) ext = fn+wcslen(fn);
		link->SetPath(dest);
		enum{ descN=96 };
		wchar_t desc[descN];
		DWORD df = FDTF_LONGDATE|FDTF_LONGTIME;
		int cat = SHFormatDateTimeW(&ft,&df,desc,descN);
		//the time feels the entire comment box in English
		//wcsncat(desc,L" (x2mdl.dll timestamped)",descN);
		link->SetDescription(desc);

		if(!wcscmp(L".txr",PathFindExtensionW(dest)))
		{
			link->SetIconLocation(input,0);
		}
		else if(ico) //NOTE: also set for TXR shortcuts
		{
			wchar_t icon[MAX_PATH];
			wcscpy(icon,dest);
			wcscpy(PathFindExtensionW(icon),L".ico");
			link->SetIconLocation(icon,0);
		}
		else link->SetIconLocation(L"",0); //carries over

		wchar_t swap[32];
		wcscpy(swap,ext);
		wmemcpy(ext,L".lnk",5);
		{			
			link2->Save(input,false);

			//thinking of this because GetOpenFileName is
			//very slow to open when there are shortcuts
			//probably it's using IShellLink on every one
			//
			// TODO: maybe environment variable?
			//
			if(0) SetFileAttributesW(input,FILE_ATTRIBUTE_HIDDEN);

			//2023: X2MDL_UPTODATE?
			FILETIME ct; SYSTEMTIME st;
			GetSystemTime(&st);
			SystemTimeToFileTime(&st,&ct);

			HANDLE h2 = CreateFileW(input,GENERIC_WRITE,0,0,OPEN_EXISTING,0,0);
			if(h2!=INVALID_HANDLE_VALUE)
			if(!SetFileTime(h2,&ct,0,&ft))
			{
				DWORD err = GetLastError();
				
				assert(0);
			}
			CloseHandle(h2);
		}
		wcscpy(ext,swap);
	}			
	CloseHandle(h);
}
static bool x2mdl_shortcut(const wchar_t *i, wchar_t o[MAX_PATH])
{
	//DUPLICATE som_art_shortcut (w/ comments, etc.)
	FILE *f = _wfopen(i,L"rb"); if(!f) return false;
	char buf[4096];	
	int sz = fread(buf,1,sizeof(buf),f); fclose(f);
	struct lnk_header
	{
		char magic[4];
		char guid[16];
		DWORD idlist:1,path:1;
		char _unspecified[52];
		WORD idlist_sz;
	};
	auto &l = *(lnk_header*)buf;
	if(sz<sizeof(l)||!l.idlist||78+l.idlist_sz>sz)
	{
		assert(0); return false;
	}
	return SHGetPathFromIDListW((PCIDLIST_ABSOLUTE)(buf+78),o);
}
bool x2mdl_dll::havelink(const wchar_t *dest)
{
	//this is a (potential) optimization to avoid making
	//copies of textures when they're shared or when the
	//model is changed but the texture remains unchanged
	wchar_t path[MAX_PATH];
	wchar_t *fn = PathFindFileNameW(input);
	wchar_t *ext = wcschr(fn,'.');
	if(!ext) ext = fn+wcslen(fn);
	bool ret = false;
	wchar_t swap[32];
	wcscpy(swap,ext);
	wmemcpy(ext,L".lnk",5);
		#if 0 //runs very slow (and is avoidable)
	if(!link2->Load(input,STGM_READ))			
	if(!link->GetPath(path,MAX_PATH,0,0))
		#else
	if(x2mdl_shortcut(input,path)) //OPTIMZING
		#endif
	{
		//WARNING: this relies on the client software to
		//check every texture individually to update the
		//links in case the texture is change indpendent
		//of the model (which they should to be correct)
		/*links use \ whereas x2mdl prefers / so there's
		//no way to use strcmp here
		if(!wcsicmp(dest,path)) ret = PathFileExistsW(path);*/
		for(int i=0;;i++) if(dest[i]!=path[i])
		{
			switch(path[i])
			{
			case '\\': if(dest[i]=='/') continue;
			case '/': if(dest[i]=='\\') continue;
			default:				
			if(tolower(path[i])==tolower(dest[i]))
			continue;
			}
			break;			
		}
		else if(!path[i])
		{
			ret = PathFileExistsW(path); break;
		}
	}
	wcscpy(ext,swap); return ret;
}
#endif

