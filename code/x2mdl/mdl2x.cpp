
#include "mdl2x.h" //precompiled header

#include <Shellapi.h> //CommandLineToArgvW

#define X2MDL_TEXTURE_MAX 256
#define X2MDL_TEXTURE_TPF 0x02 //16bit
//#define X2MDL_TEXTURE_TPF 0x03 //32bit

int exit_status = 0;

const wchar_t *exit_reason = 0;

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

const wchar_t *i18n_assimpmemoryfailure = 
L"The Assimp library reported that it ran out of memory.\r\n";

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

#define _or_exit ; if(exit_status) goto _1

//may not be required (not using d3d)
LRESULT CALLBACK DummyWinProc(HWND,UINT,WPARAM,LPARAM)
{
	return 1;
};

HWND MakeHiddenWindow()
{
	WNDCLASSEX dummy;

	memset(&dummy,0x00,sizeof(WNDCLASSEX));
	
	dummy.cbSize = sizeof(WNDCLASSEX);	
	dummy.lpfnWndProc = DummyWinProc;	
	dummy.lpszClassName = "dummy";
				   
	RegisterClassEx(&dummy);

	HWND wohs = CreateWindow
	("dummy","MDL2X (offscreen)",0,-1,-1,-1,-1,0,0,0,0);

	if(!wohs) exit_status = 1;
	
	return wohs;
}

#ifdef _DEBUG

bool skip_textures = false; //true; //???

#else

bool skip_textures = false;

#endif

//lazy: copied from x2mdl.cpp...

bool d3d_available = false;

IDirect3D9 *pd3D9 = 0;

IDirect3DDevice9 *pd3Dd9 = 0;

PDIRECT3DSURFACE9 rt = 0, rs = 0; //render target

void release_d3d()
{		
	if(rt) rt->Release(); rt = 0; //render target
	if(rs) rs->Release(); rs = 0; //render surface

	if(pd3Dd9) pd3Dd9->Release(); pd3Dd9 = 0;
	if(pd3D9) pd3D9->Release(); pd3D9 = 0;
}

bool initialize_d3d()
{	
	static bool init = false, out = false;

	if(init) return out; init = true;

	if(skip_textures) return out = d3d_available = true;

	HWND hwnd = GetConsoleWindow();
	HWND fake = MakeHiddenWindow();

//	bool d3d_available = false;

	exit_reason = i18n_direct3dfailure;

	//TODO: dynamically load interface
//	IDirect3D9 *pd3D9 = Direct3DCreate9(D3D_SDK_VERSION);
	pd3D9 = Direct3DCreate9(D3D_SDK_VERSION);

	if(!pd3D9) goto d3d_failure;

	std::wcout << "Status: Direct3D9 interface loaded\n";

	D3DFORMAT d3d9f = D3DFMT_UNKNOWN; int modes = 0;

	modes = pd3D9->GetAdapterModeCount(D3DADAPTER_DEFAULT,d3d9f);

	while(modes==0) 
	{
		switch(d3d9f) //doesn't matter, just find one that works
		{
		case D3DFMT_UNKNOWN:  d3d9f = D3DFMT_R5G6B5; break;
		case D3DFMT_R5G6B5:   d3d9f = D3DFMT_X1R5G5B5; break;
		case D3DFMT_X1R5G5B5: d3d9f = D3DFMT_A1R5G5B5; break;
		case D3DFMT_A1R5G5B5: d3d9f = D3DFMT_X8R8G8B8; break;
		case D3DFMT_X8R8G8B8: d3d9f = D3DFMT_A8R8G8B8; break;
		case D3DFMT_A8R8G8B8: d3d9f = D3DFMT_A2R10G10B10; break;

		default: goto d3d_failure;
		}

		modes = pd3D9->GetAdapterModeCount(D3DADAPTER_DEFAULT,d3d9f);
	}

	int tiny = 0x7FFFFFFF; D3DDISPLAYMODE mode, best;

	for(int i=0;i<modes;i++)
	{
		pd3D9->EnumAdapterModes(D3DADAPTER_DEFAULT,d3d9f,i,&mode);

		if(mode.Width>=X2MDL_TEXTURE_MAX&&mode.Height>=X2MDL_TEXTURE_MAX)
		if(mode.Height<tiny){ tiny = mode.Height; best = mode; }
	}

//	IDirect3DDevice9 *pd3Dd9 = 0;

	D3DPRESENT_PARAMETERS null = 
	{
		best.Width,best.Height,best.Format,1, //256,256,D3DFMT_X1R5G5B5,0,
		D3DMULTISAMPLE_NONE,0,
		D3DSWAPEFFECT_DISCARD,fake,1,
		0,D3DFMT_UNKNOWN, //1,D3DFMT_D16, //todo: enum/try 0		
		0,0,D3DPRESENT_INTERVAL_IMMEDIATE
	};

//	PDIRECT3DSURFACE9 rt = 0, rs = 0; //render target

	HRESULT hr = pd3D9->CreateDevice
	(D3DADAPTER_DEFAULT,D3DDEVTYPE_REF,hwnd, //D3DDEVTYPE_NULLREF
	D3DCREATE_SOFTWARE_VERTEXPROCESSING,&null,&pd3Dd9);

	if(hr!=D3D_OK)
	{
		std::wcout << "Status: Reference device failed to load\n";

		hr = pd3D9->CreateDevice
		(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,hwnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,&null,&pd3Dd9);
	}

	if(hr!=D3D_OK) goto d3d_failure;

	std::wcout << "Status: Direct3DDevice9 interface loaded\n";

	exit_reason = i18n_direct3dgeneralfailure;

	D3DFORMAT tpf = D3DFMT_X1R5G5B5;

	switch(X2MDL_TEXTURE_TPF)
	{
	case 0x02: tpf = D3DFMT_X1R5G5B5; break; //16bit
	case 0x03: tpf = D3DFMT_X8R8G8B8; break; //32bit??

	default: assert(0);
	}

	hr = pd3Dd9->CreateRenderTarget 
	(X2MDL_TEXTURE_MAX,X2MDL_TEXTURE_MAX,tpf,D3DMULTISAMPLE_NONE,0,!TRUE,&rt,0);
	
	if(hr!=D3D_OK) goto d3d_failure;

	hr = pd3Dd9->CreateOffscreenPlainSurface
	(X2MDL_TEXTURE_MAX,X2MDL_TEXTURE_MAX,tpf,D3DPOOL_SYSTEMMEM,&rs,0);

	if(hr!=D3D_OK) goto d3d_failure;

	hr = pd3Dd9->SetRenderTarget(0,rt);

	if(hr!=D3D_OK) goto d3d_failure;

	if(hr!=D3D_OK) 
	{
	d3d_failure: 

		int yesno = MessageBoxA
		(0,"Direct3D failed to initialize or experienced failure on one or more interfaces. \r\n" 
		"Proceed without textures?","x2mdl.exe",MB_YESNO|MB_ICONERROR);
			
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

_0: return out;

_1: exit_status = 1;

	return out;
}

const char *save_d3dsurface(const char *p, aiTexture *q)
{
	if(!d3d_available) return "";

	static char out[MAX_PATH]; *out = '\0';

	static int numdumped = 0;

	sprintf_s(out,"%s.texdump%d.bmp",p,numdumped++);
	
	if(skip_textures) return out;

	UINT w = q->mWidth, h = q->mHeight;

	if(!h) 
	{
		exit_reason = i18n_compressedtexunsupported; goto _1;
	}
	
	IDirect3DTexture9 *pt = 0;

	HRESULT	hr = pd3Dd9->CreateTexture
	(w,h,1,D3DUSAGE_DYNAMIC,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&pt,0);

	exit_reason = i18n_direct3dgeneralfailure;

	if(hr!=D3D_OK) goto tex_failure;

	D3DLOCKED_RECT lock;

	hr = pt->LockRect(0,&lock,0,0);

	if(hr!=D3D_OK) goto tex_failure;

	assert(sizeof(int)==sizeof(char)*4);

	if(lock.Pitch!=4*w)
	{
		void *info = q->pcData;

		for(int i=0;i<h;i++) //note: shadowing
		{
			int *p = (int*)info+w*i;
			int *q = (int*)lock.pBits+lock.Pitch/4*i;

			for(int j=0;j<w;j++) *q++ = *p++;
		}
	}
	else memcpy(lock.pBits,q->pcData,w*h*4);

	pt->UnlockRect(0);
	
	pd3Dd9->Clear(0,0,D3DCLEAR_TARGET,0xFF0000FF,1.f,0);
			
	pd3Dd9->BeginScene();
	pd3Dd9->SetTexture(0,pt);

	pd3Dd9->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);

	if(w>h)
	{
		float r = float(h)/w;

		w = min(X2MDL_TEXTURE_MAX,w); 
		
		h = r*w;
	}
	else if(h>w)
	{
		float r = float(w)/h;

		h = min(X2MDL_TEXTURE_MAX,h);

		w = r*h;
	}
	else
	{
		w = min(X2MDL_TEXTURE_MAX,w);
		h = min(X2MDL_TEXTURE_MAX,h);
	}

	float vb[24] = 
	{
		0.f,0.f,0.f,1.0f,0.f,0.f,
		  w,0.f,0.f,1.0f,1.f,0.f,
		  w,  h,0.f,1.0f,1.f,1.f,
		0.f,  h,0.f,1.0f,0.f,1.f,   
	};

	pd3Dd9->DrawPrimitiveUP(D3DPT_TRIANGLEFAN,2,vb,24);
	pd3Dd9->EndScene();
	
	pt->Release();	 

	if(pd3Dd9->GetRenderTargetData(rt,rs)!=D3D_OK)
	{
		exit_reason = i18n_direct3dgeneralfailure;

		goto tex_failure;
	}

	//TODO: allow for other D3DXIMAGE_FILEFORMATs
	hr = D3DXSaveSurfaceToFile(out,D3DXIFF_BMP,rs,0,0);

	if(hr!=D3D_OK) return ""; //TODO: warning
		
	goto _0;

tex_failure: //allow user to discard texture if desired
			
	if(pt) pt->Release();

	int yesno = 
	MessageBoxA(0,"A texture failed to load or otherwise could not be processed.\r\n Discard texture and proceed?","x2mdl.exe",MB_YESNO|MB_ICONERROR);
	
	if(yesno!=IDYES) goto _1;

_0: return out;

_1: exit_status = 1;

	return "";
}

//2017: msm_clamp eliminates small inaccuracies
//introduced by Blender so they do not manifest
//as shadow-casting artifacts when MPX compiled.
inline float msm_clamp(float x)
{
	//Note: 15 is not decimal places. But the 
	//larger the more precise. 5 worked for a
	//while, but irrational numbers were very
	//far away. There are 8 exponent bits. So
	//maybe 256 is a limit? Or is 8 a limit??
	//In any case, 15 doesn't overlap shadows.

	//MSVC2010 doesn't have round().
	//return ldexp(round(ldexp(x,15)),-15);
	//
	// FIX? floor isn't equivalent to round
	//
	return ldexp(floor(ldexp(x,18)+0.5),-18);
}

bool tmd = false;
inline float tmd_scale(float x)
{
	if(!tmd) return x;
	
	//KING'S FIELD 2
	//NOTE: It looks like 1034 but it seems to be a trick
	//to deal with cracks where positioning is not precise
	//enough on the PlayStation. Transparent tiles use 1024.
	//(these lips/wings will have to be trimmed by hand.)
	//return tmd?x/1034:x;
	//1034: Since this is done to climbing pieces also, while
	//their height retains the value it would have at 1, it is
	//best to do this here, and deal with the fallout afterward
	if(abs((int)x)==1034) return x<0?-1:1; return x/1024;
}

//x2msm like many X loaders only uses the single mesh API.
//Or the old retained-mode's D3DXLoadMeshFromX equivalent.
void uni_model(X::File &x)
{	
	X::Data mesh(x,TID_D3DRMMesh,"D3DXLoadMeshFromX");			

	aiMesh sum,*p; bool vcolors = false;
	
	for(int i=0;i<mdl->mNumMeshes;i++)
	{
		p = mdl->mMeshes[i];

		sum.mNumVertices+=p->mNumVertices;

		sum.mNumFaces+=p->mNumFaces;

		if(p->HasVertexColors(0)) vcolors = true;
	}
			
	mesh.write((DWORD)sum.mNumVertices);

	for(int i=0;i<mdl->mNumMeshes;i++)
	{
		p = mdl->mMeshes[i];

		for(unsigned int j=0;j<p->mNumVertices;j++)
		{
			//2017: msm_clamp eliminates small inaccuracies
			//introduced by Blender so they do not manifest
			//as shadow-casting artifacts when MPX compiled.
			mesh.write(msm_clamp(tmd_scale(p->mVertices[j].x)));
		//	mesh.write(msm_clamp(tmd_scale(p->mVertices[j].y))); //???
			mesh.write(msm_clamp(p->mVertices[j].y));
			mesh.write(msm_clamp(tmd_scale(p->mVertices[j].z)));
		}
	}
	mesh.write((DWORD)sum.mNumFaces);
	DWORD sumoff = 0;
	for(int i=0;i<mdl->mNumMeshes;i++,sumoff+=p->mNumVertices)
	{
		p = mdl->mMeshes[i];

		for(unsigned int j=0;j<p->mNumFaces;j++)
		{
			mesh.write(p->mFaces[j].mNumIndices);
			//mesh.write(p->mFaces[j].mIndices,p->mFaces[j].mNumIndices);
			for(int k=0;k<p->mFaces[j].mNumIndices;k++)
			mesh.write(sumoff+p->mFaces[j].mIndices[k]);
		}
	}

	X::Data nmls(mesh,TID_D3DRMMeshNormals);

	nmls.write((DWORD)sum.mNumVertices);

	for(int i=0;i<mdl->mNumMeshes;i++)
	{	
		p = mdl->mMeshes[i];

		if(p->HasNormals())
		{	
			for(unsigned int j=0;j<p->mNumVertices;j++)
			{
				nmls.write(p->mNormals[j].x);
				nmls.write(p->mNormals[j].y);
				nmls.write(p->mNormals[j].z);
			}
		}
		else
		{	
			for(unsigned int j=0;j<p->mNumVertices;j++)
			{
				nmls.write(1);
				nmls.write(0);
				nmls.write(0);
			}
		}
	}
	nmls.write((DWORD)sum.mNumFaces);
	sumoff = 0;
	for(int i=0;i<mdl->mNumMeshes;i++,sumoff+=p->mNumVertices)
	{
		p = mdl->mMeshes[i];

		for(unsigned int j=0;j<p->mNumFaces;j++)
		{
			nmls.write(p->mFaces[j].mNumIndices);
			//mesh.write(p->mFaces[j].mIndices,p->mFaces[j].mNumIndices);
			for(int k=0;k<p->mFaces[j].mNumIndices;k++)
			nmls.write(sumoff+p->mFaces[j].mIndices[k]);
		}					
	}					

	X::Data crds(mesh,TID_D3DRMMeshTextureCoords);

	crds.write((DWORD)sum.mNumVertices);

	for(int i=0;i<mdl->mNumMeshes;i++)
	{	
		p = mdl->mMeshes[i];

		if(p->HasTextureCoords(0)) 
		{
			for(unsigned int j=0;j<p->mNumVertices;j++)
			{
				crds.write(p->mTextureCoords[0][j].x);		
				crds.write(p->mTextureCoords[0][j].y);
			}	
		}
		else
		{
			for(unsigned int j=0;j<p->mNumVertices;j++)
			{
				crds.write(0.0f); crds.write(0.0f);
			}	
		}
	}

	if(vcolors)
	{
		X::Data vclr(mesh,TID_D3DRMMeshVertexColors);

		vclr.write((DWORD)sum.mNumVertices);

		for(int i=0;i<mdl->mNumMeshes;i++)
		{	
			p = mdl->mMeshes[i];

			if(p->HasVertexColors(0))
			{
				for(unsigned int j=0;j<p->mNumVertices;j++)
				{
					vclr.write((DWORD)j);
					vclr.write(p->mColors[0][j]);		
				}	
			}
			else
			{
				for(unsigned int j=0;j<p->mNumVertices;j++)
				{
					vclr.write((DWORD)j);
					vclr.write(aiColor4D(1,1,1,1)); 
				}	
			}
		}
	}

	X::Data mlst(mesh,TID_D3DRMMeshMaterialList);

	mlst.write((DWORD)mdl->mNumMeshes);
	mlst.write((DWORD)sum.mNumFaces);

	for(int i=0;i<mdl->mNumMeshes;i++)
	{
		p = mdl->mMeshes[i];

		for(unsigned int j=0;j<p->mNumFaces;j++)
		{						
			mlst.write((DWORD)i); //TODO: fill
		}			
	}
	for(int i=0;i<mdl->mNumMeshes;i++)
	{
		p = mdl->mMeshes[i];

		aiString name; //2021
		aiMaterial *q = mdl->mMaterials[p->mMaterialIndex];
		if(q->Get(AI_MATKEY_NAME,name))
		name.Set("Mat%d");

		//mlst.refer("Mat%d",p->mMaterialIndex);
		mlst.refer(name.data,p->mMaterialIndex);
	}

	memset(&sum,0x00,sizeof(sum));
}

//TODO: could just use mdl global from mdl2x.h!
void and_scene(X::Data &x, const aiScene *mdl, const aiNode *in)
{
	X::Data xfrm(x,TID_D3DRMFrameTransformMatrix);

	//this may require converting handedness 
	assert(in->mTransformation.IsIdentity());

	xfrm.write((float*)&in->mTransformation,16);
		
	for(int i=0;i<in->mNumMeshes;i++)
	{
		x.refer("Mesh%d",in->mMeshes[i]);
	}

	for(int i=0;i<in->mNumChildren;i++)
	{
		X::Data child(x,TID_D3DRMFrame,"%s",in->mChildren[i]->mName.data);

		and_scene(child,mdl,in->mChildren[i]);
	}
}	

int main(int argc, char** argv)
//int wmain(int argc, const wchar_t* argv[])
{			
	#ifdef _DEBUG
	_set_error_mode(_OUT_TO_MSGBOX); 
	#endif

	//wchar_t **winput = 
	//CommandLineToArgvW(GetCommandLineW(),&argc);

	struct aiLogStream stream;

	if(argc<3)
	{
#ifdef GL_DEBUG //Nearly verbatim from Sample_SimpleOpenGL.c

	glutInitWindowSize(800,600);
	glutInitWindowPosition(100,100);
	glutInitDisplayMode(GLUT_RGB|GLUT_DOUBLE|GLUT_DEPTH);
	glutInit(&argc,argv);

	glutCreateWindow("mdl2x - OpenGL Visual Debugger");
	glutDisplayFunc(GL::display);
	glutReshapeFunc(GL::reshape);

#endif
	}

	// get a handle to the predefined STDOUT log stream and attach
	// it to the logging system. It will be active for all further
	// calls to aiImportFile(Ex) and aiApplyPostProcessing.
	stream = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT,NULL);
	aiAttachLogStream(&stream);

	// ... exactly the same, but this stream will now write the
	// log file to assimp_log.txt
	stream = aiGetPredefinedLogStream(aiDefaultLogStream_FILE,"assimp_log.txt");
	aiAttachLogStream(&stream);
	
	wchar_t top[MAX_PATH] = L"";		
	GetCurrentDirectoryW(MAX_PATH,top);

	//HACK: Assimp's loaders all output exploded triangles only
	//and every postprocessing step expects this behavior.
	//Preserve polygons (quadrangles, etc.)
	aiSetImportPropertyInteger("SOM_NOT_POSTPROCESSING",1);		
	unsigned int postprocess = 
	aiProcess_ConvertToLeftHanded|
	aiProcess_PreTransformVertices; //x2msm is a single mesh only :(

	for(int in=1;in<argc;in++)
	{	
		char *input = "";
		char *debug = "dwarf.x"; //TODO: wchar_t

		if(argc>1) debug = argv[in];
		if(argc>1) input = argv[in];
		
#ifdef _DEBUG

		input = debug;
#endif
		std::wcout << "Hello: Loading " << input << "...\n" << std::flush;

#ifdef _DEBUG
#ifdef GL_DEBUG
		//disabling visuals for multi-file input
		if(argc<3) GL::loadasset(debug,postprocess)
		else //...		
#else
		mdl = aiImportFile(debug,postprocess);		
#endif
#else //NDEBUG

		if(!input||!*input) 
		{
			exit_reason = i18n_inputfilefailure; goto _1;
		}

		mdl = aiImportFile(input,postprocess);
#endif
		if(!mdl) 
		{
			exit_reason = i18n_assimpfailure; goto _1;
		}

		std::wcout << "Hello: Converting " << input <<  "...\n" << std::flush;
				
		char out[MAX_PATH]; //hack

		char *ext = out+sprintf_s(out,MAX_PATH,"%s.%s",input,x)-strlen(x)-2;
		while(ext>out&&ext[-1]!='.') ext--;
		tmd = 0==strnicmp(ext,"tmd",3);

		//prepare to write file...
		SetCurrentDirectoryW(top);
					
		std::wcout << "Hello: Saving " << out <<  "...\n" << std::flush;

		if(!stricmp(x,"x"))
		{	
			char o[3] = "wb";
			if(GetEnvironmentVariableA("D3DXF_FILEFORMAT",o+1,2)) switch(o[1])
			{		    
			case '1': o[1] = 't'; break; case '2': o[1] = 'c'; break; default: o[1] = 'b';
			}
			X::File x(out,o); //hack: shadowing

			static GUID header = 
			{0x3D82AB43,0x62DA,0x11cf,0xAB,0x39,0x00,0x20,0xAF,0x71,0xE4,0x33};
			{
				X::Data head(x,header);

				head.write((DWORD)1);
				head.write((DWORD)0);
				head.write((DWORD)1);
			}

			DWORD tps = 0;

			//ticks per second
			if(mdl->mNumAnimations)
			{
				//TODO: sum over all / timeshift animation keys
				tps = mdl->mAnimations[0]->mTicksPerSecond;

				if(!tps) tps = 25; //AssimpView default

				X::Data tic(x,DXFILEOBJ_AnimTicksPerSecond);

				tic.write(tps);
			}

			assert(sizeof(aiColor4D)==sizeof(D3DCOLORVALUE));			
									
			//all materials are global			
			assert(0!=mdl->mNumMaterials);
			for(int i=0;i<mdl->mNumMaterials;i++)
			{
				aiMaterial *p = mdl->mMaterials[i];

				aiString name; //2021
				if(p->Get(AI_MATKEY_NAME,name))
				name.Set("Mat%d");

				//2021: what the???
				//X::Data mat(x,TID_D3DRMMaterial,"Mat%d",i);
				X::Data mat(x,TID_D3DRMMaterial,name.data,i);

				aiColor4D diffuse(1,1,1,1); //RGBA
				p->Get(AI_MATKEY_COLOR_DIFFUSE,diffuse);
				float a = 1;
				p->Get(AI_MATKEY_OPACITY,a);
				diffuse.a*=a;

				float power = 0; 
				p->Get(AI_MATKEY_SHININESS,power);

				aiColor3D specular(0,0,0); //RGB
				p->Get(AI_MATKEY_COLOR_SPECULAR,specular);

				aiColor3D emissive(0,0,0); //RGB								
				p->Get(AI_MATKEY_COLOR_EMISSIVE,emissive);

				mat.write(diffuse);
				mat.write(power);
				mat.write(specular);
				mat.write(emissive);

				//This is for MSMs so SOM_MAP doesn't crash
				//if(p->GetTextureCount(aiTextureType_DIFFUSE))
				{	
					aiString path;
					p->GetTexture(aiTextureType_DIFFUSE,0,&path);
					
					//dump embedded texture
					if(path.length&&*path.data=='*')
					{
						initialize_d3d()_or_exit;
						
						if(!d3d_available) break;

						int num = atoi(path.data+1);
													
						aiTexture *p = mdl->mTextures[num];

						//D3DXSaveSurfaceToFile
						path = save_d3dsurface(out,p);
					} 

					//if(path.length) //hack
					{	
						X::Data tex(mat,TID_D3DRMTextureFilename);

						if(path.length)
						{
							//This is mainly for non-SOM inputs.
							const char *trunc = path.data+path.length;
							while(trunc>path.data&&'\\'!=trunc[-1]&&'/'!=trunc[-1])
							trunc--;

							tex.write(trunc,path.length-(trunc-path.data)+1);
						}
						else //This is for MSMs so SOM_MAP doesn't crash
						{
							tex.write("atari.bmp",10);												
						}
					}
				}
			}

			assert(sizeof(float)*3==sizeof(aiVector3D));
			assert(sizeof(DWORD)==sizeof(unsigned int));

			//x2msm like many X loaders only uses the single mesh API.
			//Or the old retained-mode's D3DXLoadMeshFromX equivalent.
			uni_model(x);			

			//X::Data root(x,TID_D3DRMFrame,"%s",mdl->mRootNode->mName.data);
			X::Data root(x,TID_D3DRMFrame,"%s",PathFindFileName(input));

			and_scene(root,mdl,mdl->mRootNode);

			for(int i=0;i<mdl->mNumAnimations;i++)
			{
				aiAnimation *p = mdl->mAnimations[i];
				
				if(p->mNumMeshChannels) continue; //not supporting (yet)

				X::Data aset(x,TID_D3DRMAnimationSet,"%s",p->mName.data);
			
				for(int j=0;j<p->mNumChannels;j++)
				{
					aiNodeAnim *q = p->mChannels[j];

					X::Data anim(aset,TID_D3DRMAnimation);

					anim.refer("%s",q->mNodeName.data);

					//X can handle non-negative keys only
					int k; for(k=0;k<q->mNumRotationKeys;k++)
					{
						if(q->mRotationKeys[k].mTime>=0) break;
					}

					if(q->mNumRotationKeys-k>0)
					{
						X::Data rkey(anim,TID_D3DRMAnimationKey);

						rkey.write((DWORD)0); //rotations
						rkey.write((DWORD)q->mNumRotationKeys-k);
												
						for(;k<q->mNumRotationKeys;k++)
						{								
							rkey.write((DWORD)q->mRotationKeys[k].mTime);

							rkey.write((DWORD)4); //quaternion

							aiQuaternion tmp = q->mRotationKeys[k].mValue;

							tmp.z = -tmp.z; //tmp.Conjugate();

							//aiVector3D euler;

							//Assimp::EulerAnglesFromQuaternion<+1,1,2,3>(euler,tmp);

							//euler.y = -euler.y;
							
							//euler.z = -euler.z;

							//Assimp::EulerAnglesToQuaternion<-1,1,2,3>(euler,tmp);

							rkey.write((float*)&tmp,4);
						}
					}

					//X can handle non-negative keys only
					for(int k=0;k<q->mNumPositionKeys;k++)
					{
						if(q->mPositionKeys[k].mTime>=0) break;
					}

					if(q->mNumPositionKeys-k>0)
					{
						X::Data pkey(anim,TID_D3DRMAnimationKey);

						pkey.write((DWORD)2); //positions
						pkey.write((DWORD)q->mNumPositionKeys-k);
						
						for(k;k<q->mNumPositionKeys;k++)						
						{	
							pkey.write((DWORD)q->mPositionKeys[k].mTime);

							pkey.write((DWORD)3); //xyz
							pkey.write(q->mPositionKeys[k].mValue.x);
							pkey.write(q->mPositionKeys[k].mValue.y);
							pkey.write(-q->mPositionKeys[k].mValue.z);
						}
					}
				}	  
			}
		}
		else if(!stricmp(x,"dae"))
		{
			aiExportScene(mdl,"collada",out);
		}
		else aiExportScene(mdl,x,out);

#ifdef GL_DEBUG 

		if(argc>2)
#endif
		{
			aiReleaseImport(mdl);

			mdl = 0;
		}
	}	

	if(argc<3)
	{
#ifdef GL_DEBUG  //Nearly verbatim from Sample_SimpleOpenGL.c

		glClearColor(0.1f,0.1f,0.1f,1.f);

		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0); //Uses default lighting parameters

		glEnable(GL_DEPTH_TEST);

		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);
		glEnable(GL_NORMALIZE);

		// XXX docs say all polygons are emitted CCW, but tests show that some aren't.
		if(getenv("MODEL_IS_BROKEN"))  
		glFrontFace(GL_CW);

		glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);

		glutGet(GLUT_ELAPSED_TIME);
		glutMainLoop();

#endif _DEBUG
	}

_0:	//wchar_t ok; std::wcin >> ok; //debugging
	
	// cleanup - calling 'aiReleaseImport' is important, as the library 
	// keeps internal resources until the scene is freed again. Not 
	// doing so can cause severe resource leaking.
	if(mdl) aiReleaseImport(mdl);

	// We added a log stream to the library, it's our job to disable it
	// again. This will definitely release the last resources allocated
	// by Assimp.
	aiDetachAllLogStreams();

	release_d3d();

	//nuisance for real work
	//Sleep(4000); //leave console open for four seconds 

	return exit_status;

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

	exit(exit_status); //might as well try
}

