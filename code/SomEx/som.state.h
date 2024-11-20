
#ifndef SOM_STATE_INCLUDED
#define SOM_STATE_INCLUDED

#include <vector>

#include "Ex.memory.h"
#include "Ex.mipmap.h"

#include "som.game.h" //som.scene.h?

struct som_MHM; //som.MHM.cpp
struct som_MHM_ball;
struct som_MDL; //som.MDL.cpp
struct som_MDO; //2021
struct som_BSP; //2022

namespace SOM
{
	typedef som_MDL MDL;
	typedef som_MDO MDO;
	typedef som_MHM MHM;

	template<int N=1> struct Struct;
}
typedef SOM::Struct<132> som_MPX;
typedef SOM::Struct<63> som_EVT;
typedef SOM::Struct<43> som_NPC;
typedef SOM::Struct<149> som_Enemy;
typedef SOM::Struct<46> som_Obj;
				  
namespace SOM 
{	
	extern bool retail; //game
	extern int image(),tool,game; 
	extern DWORD image_rdata,image_data;

	extern DWORD thread(); //thread ID of WinMain
	
	extern const EX::Section *memory; //@400000
	extern const EX::Program *disasm; //disassembly
	extern const EX::Section *text,*rdata,*data; 

	//EVERYTHING BELOW IS GAME SPECIFIC//

	extern bool &f3; //EX::output_overlay_f[3]

	extern bool emu; //emulation (do_2000)
	extern const int &log; //EX::logging_onoff	

	//som.mocap.cpp 
	extern unsigned eventick;
	extern int event; 
	extern int eventapped,eventype;

	extern unsigned et,eticks; //elapsed time/ticks
	
	extern bool 
	play, //NEW: drive player below via event
	field, //true once in game new or continue
	player, //blocks player controls when false
	padtest, //blocks input in the Controls menu
	windowed; //there's screen realestate to spare
	extern bool &paused; //DDRAW::isPaused
	extern void reset(),warp(FLOAT[6],FLOAT[6]=0); //loading game

	//FYI these 3-letter paramers aren't any kind of
	//ideal design design. I've wanted to replace it
	//with something else for a long time, but can't
	//bother to
	extern float 
	fov[4], //resolution in x,y; near/far in z,w
	xyz[3], //mapcoords+compass (rotation in xz plane)	
	uvw[3], //2021: previously xyz[3] and pov[3] + roll?
	pov[4], //viewvector+plane distance
	pos[3], //2022: xyz from VR added to cam
	hmd[3], //2022: uvw with VR contribution
	eye[4], //camera relative to map position + bob
	cam[4], //2020: xyz+eye in world space (magic y)
	arm[3], //right arm swing tweaks
	err[4], //unbounded xyz+|xz|		
	xyz_past[3],xyz_step, //som.mocap.cpp
	analogcam[4][4], //tethered to SOM's digital camera	
	steadycam[4][4], //inverse of Ex adjustments to eye
	stereo344[3][4][4], //OpenXR
	heading,incline, //f6 overlay 
	slope,arch; //experimental

	//SOM_SYS
	extern int setup(); //1 or 2
	//SYS.DAT
	extern bool picturemenu;	
	//2020: I'm changing this so player_character_stride 
	//automatically scales walk and dash so underscoring
	//the const values
	extern const float &_walk,&_dash,&_turn; //SHORT
	extern float walk,dash;
	extern const BYTE &mode,&save,&gauge,&compass,&capacity; 	
	
	extern bool ezt[1024]; //true if an event is portable	

	extern BYTE mpx,mpx2;
	extern bool sky,skyswap; //nonzero if skydome present
	extern DWORD clear; //fogcolor
	//extern float fog[2]; //fogstart/end parameters		
	//extern BYTE *ambient2[7+1];
	extern BYTE **ambient2; //som_MPX_swap::mpx::ambient2

	struct mpx_defs_t //2022
	{
		//for now this needs to change when map transfers
		//reset material indices
		int sky; //som_MDO *sky;
		som_MDO *sky_instance();

		int _sky; som_MDO **_sky_instance;

		//read into continguous block
		float _fov,znear,zfar,fog; DWORD bg,ambient; //24B
		struct{ DWORD col; float dir[3]; }lights[3]; //48B

		DWORD _fogcolor;
		float _fogstart,_fogend,_fogmin[2];
		float _skystart,_skyend,_skyfloor,_skyflood;
	};
	extern mpx_defs_t &mpx_defs(int i); //som_MPX.cpp

	//extended values per BeginScene of each frame
	extern float fogstart,fogend, skystart,skyend;

	//2022: this blends the above values and skies
	extern float mpx2_blend,mpx2_blend_start;
		
	extern int colorkey(DX::DDSURFACEDESC2*,D3DFORMAT);

	//Options menu units
	//extern int volume[2]; //music/sound	   
	//should be negative
	extern float decibels[3]; //music/sound/master
	extern int millibels[3]; //DirectSound
	extern void config_bels(int=7); //2020
	//REMOVE ME?
	//NOTE: this is sometimes used as a velocity
	//but only with hacks.
	extern float doppler[3]; //listener velocity	
	
	//WARNING 
	// 
	// At one time these were to back the INI state. But they
	// no longer always do so. They were never actually used 
	// to write out the INI in the first place.
	//
	extern int//[config]	
	device,width,height,
	bpp,gamma,seVol,bgmVol, //filtering
	&filtering; //showItem
	extern BYTE &bob,&showGauge,&showCompass;
	//extended//
	extern int windowX,windowY,
	cursorX,cursorY,cursorZ,capture,
	analogMode,thumb1,thumb2,opening,
	buttonSwap,
	zoom,//zoom2(), //DDRAW::inStereo?X:SOM::zoom
	ipd,stereo,
	stereoMode, //2022
	masterVol,
	superMode, //2021
	map,mapX,mapY,mapZ,mapL,
	opengl,
	config(const char*,int def),rescue(const wchar_t*,int);
	extern float zoom2(),ipd_center();
	extern float reset_projection(float=SOM::zoom,float=SOM::skyend);
	//REMOVE ME?
	//flush ini (extended only)
	extern bool GAME_INI,config_ini(),record_ini(); 
	
	extern bool escape(int); //cycle analog mode
	extern bool thumbs(int,float[8]);
	extern unsigned int altf,altf_mask;
	extern bool altf1(),altf2(),altf3(int),altf4(int),altf5(),f10(),altf11(),altf12(int); 
		
	//mouse wheels
	extern bool z(bool downswing); //left mouse button down/up
	extern int tilt; 

	//the originals
	extern HWND window;	extern WNDPROC WindowProc;
	//REMOVE ME
	extern bool x480(); //640x480 title/movie screen mode?

	extern HICON icon;
	extern HFONT font; //system
	extern const char *fontface; 
	extern const wchar_t *wideface, *caption;
	extern int fontsize(int hres, int vres=0, int *width=0); 
	//NEW: passing 0 to face now produces the "status" font
	extern HFONT makefont(int hres, int vres, const char *face); 
	inline HFONT makefont(int hres, int vres=0)//system font
	{
		return makefont(hres,vres,SOM::fontface); 
	}

	extern int red;
	extern float red2; //2020
	extern void *hit; 
	extern unsigned invincible; 
			
	enum{ frame0=50 };
	extern unsigned 
	&frame, //DDRAW::noFlips	
	pcdown, //last PC died frame
	hpdown, //last hp drain frame
	newmap, //last .mpx open frame
	option, //last OPTION menu frame
	padcfg,	//last GAMEPAD menu frame
	dialog, //last non menu text frame
	taking, //last take this item frame
	saving, //last save game frame
	warped, //last teleport frame
	ladder, //pcstepladder frame	
	mapmap, automap, //maps frames	
	doubleblack, //0x64 tint frame
	black, //0x80 tint frame
	alt, //last alt key down frame
	alt2, //double alt down frame	
	altdown,
	ctrl,shift,space, //keys/mapping
	limbo, //buttonSwap
	crouched, //subtitles
	shoved,bopped, //TESTING //REMOVE ME
	swing,counteratk;
	extern void __cdecl frame_is_missing();
	/*{
		//for when a map/som_state_409af0
		//opens to a text event
		if(warped==frame) warped++;

		//in places SOM begins drawing a new// 
		//frame without having displayed the//
		//current frame. this could flip the//
		//frame; that hasn't been tried. but//
		//for now just keep up the heartbeat//

		SOM::frame++; //ie. DDRAW::noFlips++;
	}*/

	//reprogramming
	extern FLOAT *cone,*g,*u,u2[3],*v,*stool;
	
	//REMOVE ME?
	template<typename T> struct Select
	{
		typedef T selector;
	};
	typedef Select<WCHAR> F; //float
	typedef Select<WORD> S; //short
	typedef Select<BYTE> I; //int
	typedef Select<UINT> C;
	template<int N> struct Struct
	{
		union
		{
			FLOAT f[N]; SHORT s[N*2]; INT32 i[N];

			char c[N*4]; //probing MPX memory

			//NOTE: DWORD typedef is unsigned long?!
			WORD us[N*2]; BYTE uc[N*4]; DWORD ui[N]; //2021
		};

		inline FLOAT &operator[](SOM::F::selector n)
		{
			return f[int(n)]; //easy floating point access
		}			
		//cast to WORD if you need unsigned (65535)
		inline SHORT &operator[](SOM::S::selector n)
		{
			return s[int(n)]; //easy 16bit integral access
		}			
		inline INT32 &operator[](SOM::I::selector n)
		{
			return i[int(n)]; //easy 32bit integral access
		}
		inline CHAR &operator[](SOM::C::selector n)
		{
			return c[int(n)]; //MPX only so far
		}			
	};
	
	namespace MPX //598928
	{			
		//TEMPORARY
		//132 is likely exact. At most 143, at least
		//125. there are heap boundary bytes at 132
		//typedef SOM::Struct<132> Struct;
		typedef som_MPX Struct;

		struct Static //68416B 32+20000+21504*2+2688*2
		{
			//58a920 is ibuffer size

			DWORD unknown0; //341216

			//19aa968 holds the size
			DDRAW::IDirect3DVertexBuffer7 *vbuffer; //59892C

			DWORD tiles_to_display; //598930

			DWORD current_leaf; //0x20 (32)

			DWORD unknown2; //206904

			Struct *pointer; //59893C //som_MPX

			DWORD current_node; //598940

			BYTE tiles_on_display[10000][2]; //yx coords

			DWORD unknown4;	//59D764
							
			//WARNING: som_bsp_sort_and_draw is drawing
			//indices into this memory because the MDO
			//buffer can hold a lot more data than MPX
			//59d768 looks like a static vbuffer 896*24
			char _client_vbuffer[896*24]; //21504B
			//5A2B68
			//guessing? these are the exact same size??
			char _client_vbuffer2[896*24]; 					
			//5A7F68
			WORD ibuffer[2688];
			//5A9468
			
				BYTE _unk[0x1500]; //...

			//5aa968 (fairy map?) //4144e0
			//max seen is 6cf8e8-1
			//fvf is DIFFUSE|XYZRHW DrawPrimitive(TRIANGLE_LIST)
			//WARNING: I don't think boundaries are checked
			//here, and 100*100 is NOT hardcoded (layer #1 only)
			BYTE _debug_vbuffer[100*100*20]; //5A9468+119400=6C2868 
		};

		//som_db seems to a system that manages more 
		//than 1 layer via negative indices
		struct Layer
		{
			//NOTE: I think I've seen a world offset
			//somewhere, but it's not stored in here
			DWORD width,height;

			struct tile //16B
			{
				//guessing msm is second based this line from the layer loader code
				//(note, I can't see where this field is of any use, unless it's to
				//do with the BSP-tree thing... it's compared to FFFF)
				//00412DDF 66 8B 54 08 02       mov         dx,word ptr [eax+ecx+2]
				//00412DE4 81 FA FF FF 00 00    cmp         edx,0FFFFh 
				WORD mhm,msm; 

				float elevation;

				//or rotation:2???
				//msb seems to be a visibility test of some kind
				//test        dword ptr [ecx+edx+8],80000000h //address???
				//2020: 407d21 identifies 0x40 as checkpoint bit
				//(0 if entry isn't possible. MAP files use 'e')
				//unsigned rotation:2,nonzero:14,icon:8,unknown2:7,msb:1;
				//unsigned rotation:2,_uk1:4,ev:1,_uk2:8,nobsp:1,icon:8,msb:8; //:1
			//	unsigned rotation:2,_uk1:4,ev:8,_bsp:1,nobsp:1,icon:8,msb:8; //:1
				unsigned rotation:2,box:4,ev:1,xxx:1,hit:1,_5:5,_bsp:1,nobsp:1,icon:8,msb:8;

				//TRULY WEIRD
				//WTH?? from som.scene.cpp (rendering)
				//maybe MSM data... looks individually allocated
				struct scenery
				{
					//TEXTURE COUNT (MOST LIKELY)
					//skipped if 0
					//004140BE 83 38 00             cmp         dword ptr [eax],0 
					//suggest count... probably packs of 8 consecutive WORD values
					//004143A8 3B CA                cmp         ecx,edx 
					DWORD counter;

					//[4] is interpreted as a DWORD
					//00414161 8B 76 08             mov         esi,dword ptr [esi+8]
					struct per_texture
					{
						//skipped if [0] is 0? or nonzero? (seems nonzero)
						//0041403A 33 ED                xor         ebp,ebp 
						//004140D4 3B D5                cmp         edx,ebp (0)
						//always 2 or 4?
						//004140DE 66 8B 46 02          mov         ax,word ptr [esi+2]
						//...
						//004140FB 66 8B 56 04          mov         dx,word ptr [esi+4]
						//processed 8 at a time?
						//004143A4 83 C6 10             add         esi,10h						
						WORD texture;
						//indices limit? (0x380 is 896S)
						//004140EB 81 F9 80 03 00 00    cmp         ecx,380h
						WORD vcolor_indicesN;
						//3*indices limit? (0xa80 is 2688)
						//00414101 81 FA 80 0A 00 00    cmp         edx,0A80h 
						WORD triangle_indicesN; 
						//WORD zero1; //418230 (can use this for TRANSPARENCY?)
						WORD _transparent_indicesN;
						DWORD (*vcolor_indices)[2];
						WORD *triangle_indices;
					}*pointer;
				//always a DWORD/WORD* pair?
				//004140B4 8B 47 0C             mov         eax,dword ptr [edi+0Ch]  
				//004140B7 8B 70 04             mov         esi,dword ptr [eax+4]
				//...
				//004140D1 66 8B 16             mov         dx,word ptr [esi]
				}*pointer;
				
				struct scenery_ext //som_scene_element2
				{
					float locus[3]; int angle; 
					
					scenery::per_texture *model;

					float depth_44D5A0; //som_scene_44d7d0
				
					//REMOVE ME??
					//TODO: sort pointers (according to texture groups?)
					bool operator<(const scenery_ext &cmp)const
					{
						//less-than because the tiles are almost sorted
						//after the front-to-back solid tile processing
						return depth_44D5A0<cmp.depth_44D5A0;
					}
				};

			}*tiles; //width*height 

			//this all has to do with the maze feature
			//that doesn't render tiles that should be
			//hidden behind other tiles
			DWORD count1;
			struct struct1 //32B (9)
			{
				//11.0 99.0 21.0 171.0 (map coords)
				float xy1[2],xy2[2];

				//5~46 2~1512 (int)
				DWORD unknown1; //int->pointer?

				DWORD unknown2_count; //pointer?

				//00412579 E8 82 EF FE FF       call        00401500
				DWORD(*unknown2_pointer)[2];

				BYTE *pointer_into_pointer4;

			}*pointer1;
			DWORD count2;
			struct struct2 //44B (11)
			{
				//-1/0~41
				//-1/1~45
				//-1/2~45 //bitfield?
				//-1/0~19 //ptr? 32B
				DWORD unknown1[1]; //maybe int->pointers
				int right;
				int left;
				int leaf;

				//-1.0 171.0 21.0 199.0 (more map coords)
				float xy1[2]; //0x10
				float xy2[2]; //0x18

				//-1/1~17
				//-1/85~99
				DWORD x,z; //0x20

				//0~8 (1,2,4,8)
				DWORD blinders;

			}*pointer2;
			DWORD count3;
			struct struct3 //24B (6)
			{			
				//35.0 99.0 35.0 171.0 (map coords)
				float xy1[2],xy2[2];

				//0~20 (int)
				//2~23
				DWORD unknown1[2];

			}*pointer3;					
			//looks completely random??
			//struct1 has a pointer into this array
			BYTE *pointer4; 
		};

		const //enum:
		SOM::C::selector 	
//	    {
			map=0, //e.g. 0 if name is "00"

			name=1, //maybe two-digits only 

			title=312, //long name (unused)

			bgm=344, //WAV file name

			maps=376; //408 //440 (1 2 & 3)

			//411d03 reads this... I think
			//it was probably once a layer
			//for the starting coordinates
			//?=480; //481-483 is 0-padded
//	    }

		const //enum:
		SOM::I::selector 	
//	    {
			//this looks an awful lot like
			//KF's cemetery
			//PER LAYER DATA (40B apiece)
			//
			// 2022: maybe it's just extra
			// space to hold the file name					 m
			// it seems too big but 411ac2
			// copies an unrestrained long
			// file name at SOM::MPX::name
			//
			layer_6=6,
			layer_5=16,
			layer_4=26,
			layer_3=36,
			layer_2=46,
			layer_1=56,
			layer_0=66,
			//(see MPX::Layer)
			width=66,height=67, //0x108
			tile=68,			
			maze=69, //69~74 (see MPX::Layer)
						
			//this is truly bizarre... I think once the
			//Layer structure was an array, that wasn't
			//compiled into the final product... little
			//optimization must have been done
			//it looks like it could have used negative
			//subscripts (cute) however the memory that
			//appears to not be used that is accessible
			//when negative is likely a coincidence, as
			//patterns that use this code make no sense
			//if it were signed
			//when unsigned, it variously points either
			//to itself, or the BGM field
			//everything in som.scene.cpp tests against
			//this selector, but it compares bytes with
			//it as a dword, without sign-extending the
			//byte, and so therefor, it can't be signed 
			/*
			00413F16 8B 0D 3C 89 59 00    mov         ecx,dword ptr ds:[59893Ch]  
			00413F1C 8B 81 30 01 00 00    mov         eax,dword ptr [ecx+130h]  
			00413F22 53                   push        ebx  
			00413F23 55                   push        ebp  
			00413F24 8D 04 80             lea         eax,[eax+eax*4]  
			00413F27 56                   push        esi  
			00413F28 8D B4 C1 08 01 00 00 lea         esi,[ecx+eax*8+108h] 
			//...
			0041408B 8B 3E                mov         edi,dword ptr [esi]
			//...
			004140AC 8B 46 08             mov         eax,dword ptr [esi+8]
			*/
			layer_selector=76, //0x130

			//this precedes the title, like in the MPX
			//file. it includes light enabled flags in
			//1 and 2 (off if 0,0,0 or lighting is off)
			//ambient appears to be 2. direction 1 is 1
			flags=77,

			mhm_counter=118, //count?
			mhm_pointer=119, //pointer to som_MHM pointers
			//0? //120 I think this is in the MAP file
			//start x y z //121-123
			//start turn //124...

			//FIXED?
			/*
			00414034 8B 88 F4 01 00 00    mov         ecx,dword ptr [eax+1F4h]  
			0041403A 33 ED                xor         ebp,ebp  
			0041403C 3B CF                cmp         ecx,edi  
			0041403E 89 6C 24 30          mov         dword ptr [esp+30h],ebp  
			00414042 0F 86 0B 04 00 00    jbe         00414453  
			00414048 8B 88 F8 01 00 00    mov         ecx,dword ptr [eax+1F8h] 
			*/
			texture_counter=125,
			texture_pointer=126, //16-bit SOM::L.textures index
			vertex_counter=127,
			//vbuffer filling loop?
			//004142F3 8B AD 00 02 00 00    mov         ebp,dword ptr [ebp+200h] 
			vertex_pointer=128, //5 floats apiece (pos+uv)

			sky_index=129,
			//note: 445870 unloads
			sky_model2=130, //this is mdo data too (value returned by 445660)
			//note: 445ad0 unloads
			sky_model=131; //MDO (som.scene.cpp) (value returned by 4458d0)

			//baadfood (END OF STRUCTURE)
//	    }

		const //enum:
		SOM::F::selector 	
//	    {
			startpos=121, //starting x,y,z //122 //123
			starturn=124; //starting rotation (inverted)
//	    }
	}
		
	struct Animation //2024: som.kage.cpp
	{
		float t;

		float bbox[6];

		int w,h,data_s; BYTE *data;

		DDRAW::IDirectDrawSurface7 *texture;

		Animation(),~Animation();

		void upload(),release();
	};
	typedef std::vector<Animation> Kage,Face;
	  
	struct Texture //160B
	{
		//TODO: seems 0x448660 might leak inputted image
		//when LoadImage_handle is nonzero

		//0x448660 is used by static/known textures only
		//(MDL TIM images seem to use it via 444c80 but 
		//maybe 444b20 screens it out)
		//
		// I think it might leak its inputted HANDLE if
		// the texture had already existed
		//
		//0x448660 loads/increases ref count by 1
		//0x448780 frees/decreases possibly
		//0x448660 subroutine
		//0x448725 E8 06 0E 00 00       call        00449530
		//0x448780 subroutine
		//004487AD E8 1E 11 00 00       call        004498D0
		//DWORD ref_counter;
		INT32 ref_counter; //som_MPX_swap 

		//mode is an argument to 0x448660
		//mipmap_counter is an argument to 00449530
		//(it must be a byte)
		WORD mode; BYTE _something,mipmap_counter; 
		
		//unique ID including " TIM 000" pattern for MDL files
		//and not necessarily corresponding to the image file
		//in menu/hud cases
		char path_identifier[80]; //maybe smaller?

		//stock textures use LoadImage
		//the rest use CreateDIBSection to hold images in
		//system memory for DirectDraw. but I think these
		//are never used, and so are just gobbling memory
		//Probably the loading code needs to be rewritten
		//10 is 512/256/128/64/32/16/8/4/2/1 
		HGDIOBJ mipmaps[10];
		
		union
		{
			HGDIOBJ maybe_mipmaps_beyond_512[6]; 

			float kage_bbox[6];
		};
		
		//NOTE: Ex.mipmap.h defines these types
		DDRAW::IDirectDrawSurface7 *texture;
		DDRAW::IDirectDrawPalette *_palette;
		
		bool update_texture(int),uptodate(); //som.MPX.cpp
	};
	typedef Texture Textures[1024];
	
	//2023: because depth sorting transparent polygons
	//ends up scanning back-to-front almost like drawing
	//a 2D image there tends to be a lot of draw calls
	//using an "atlas" is the only way to reduce these 
	//calls apart from putting more attributes into the
	//vertex stream
	struct TextureAtlasRec
	{
		//x/y is scale, s/t is origin		
		float s,t,x,y;
		
		DDRAW::IDirectDrawSurface7 *texture;

		BYTE pending;
	};
	//2023: [1] is transparency and
	//[0] for is static map geometry
	extern TextureAtlasRec TextureAtlas[1024][2];

	extern struct VT
	{
		int group,sides; float depth,power,fog[4];

		unsigned frame; //HACK: last in aggregate

		//TEMPORARY
		//this is DDRAW::psColorkey's original setting
		//since it's disabled. if other shaders need a
		//general purpose register it needs to be made
		//available another way
		static int fog_register; 

	}*(*volume_textures)[1024]; //EXTENSION

	extern struct MT
	{
		int mode; float data[9];

		static SOM::MT *lookup(const char*);

	}*(*material_textures)[1024]; //EXTENSION
	
	struct Item //36B
	{
		BYTE nonempty; //bool? (yes)
		BYTE prm; //item_prm_file
		union{ BYTE mdo,pr2; }; //items_mdo_table
		BYTE layer; //compared to MPX::x130?
		FLOAT x,y,z,u,v,w;
		DWORD time; //world tick
		DWORD item; //map item or FFFF if drop
	};
	typedef SOM::Item Items[256];

	struct Shop
	{
		WORD stock[250],price[250],price2[250];

		char title[31], unknown[3]; //flag+padding?
	};
	typedef SOM::Shop Shops[128];

	struct Setup //SOM_SYS
	{
		WORD str,mag,hp,mp;
		DWORD gold,experience; 			
		BYTE level,equipment[8],items[250];
	};

	struct Attack //68B
	{
		WORD strength, magic; 
		
		union //???
		{		
			struct
			{
				BYTE damage[8]; 
		
				BYTE side_effects, side_effects_potency;

				WORD power_gauge; //5000
			};

			float explosion_origin[3]; //437cd0
		};
		
		//1 monster direct
		//2 may implicate the player character
		//8 player indirect
		//9 monster indirect and traps direct!
		//14 player direct
		//4C28A0 (player) has many more modes?
		//NOTE: this code is just copying bits (painfully)
		/*
		00404239 F6 C1 01             test        cl,1  
		0040423C 74 05                je          00404243  
		0040423E B8 01 00 00 00       mov         eax,1  
		00404243 F6 C1 02             test        cl,2  
		00404246 74 03                je          0040424B  
		00404248 83 C8 02             or          eax,2  
		0040424B F6 C1 04             test        cl,4  
		0040424E 74 03                je          00404253  
		00404250 83 C8 04             or          eax,4  
		00404253 F6 C1 08             test        cl,8  
		00404256 74 03                je          0040425B  
		00404258 83 C8 08             or          eax,8  				
		*/
		DWORD attack_mode;
		
		union //19C1A18 is player
		{
			//NOTE: NPCs aren't viable attack sources
			som_NPC *source;  
			som_Obj *source_trap;
			som_Enemy *source_enemy;
		};

		///////// LOOKS SKETCHY /////////////////

			//WARNING: 427815 sets this BYTE to 0

				//NEEDS CONFIRMING!
		SOM::Struct<> *_source_SFX;
		//I think this may be for homing SFX attacks
		union
		{
			//WARNING: 4277d3 SEEMS TO SET THIS TO 1
			//will need to set to 2 to indicate shield
			//or heavy attack
			//WARNING! SETTING TO 2 MAKES THE ATTACK
			//ALWAYS HIT
			BYTE unknown_pc1;

			//som_SFX_0a_42ed50_needle sets this to 3
			DWORD unknown_sfx;

				//NEEDS CONFIRMING!
			som_Enemy *_dest_enemy; //sfx destination?
			som_NPC *_dest_NPC;
		};

		/////////////////////////////////////////

		FLOAT attack_origin[3]; //2020: CP origin
		FLOAT radius,height;
		FLOAT aim,pie; //turn direction/hit wedge

		//I think this is filled out on hit?

		//1 for player
		//2 for enemies
		//4 for NPCs
		DWORD target_type; 

		union //0x19C1A18 is player
		{
			som_NPC *target; //NPC
			som_Enemy *target_enemy;
		};
	};
	
	struct sfx_init_params //104B (26 dwords) 
	{
		WORD sfx,_unused; //I think?
		float scale; //set to 1
		union //???
		{			
			struct //som_SFX_128a_438b10
			{
				WORD cp,cp2;
				som_MDL *mdl_cp;
				som_MDO *mdo_cp;
			};

			float xyz[3]; //add 1.2 to y
		};
		float look_vec[3]; //computed
		float duration; //magic_prm_file+0xe*4?
		Attack dmg_src; //17
	};

	struct sfx_dat_rec //Struct<12>
	{
		BYTE procedure;
		BYTE model;
		BYTE unk2[6];
		//-1,-1 disables explosions
		float width;
		float height;
		float radius;
		float speed;
		float scale;
		float scale2;
		float unk5[2];
		WORD snd,chainfx;
		char pitch,_pad2;
		SHORT _;
	};

	struct sfx_pro_rec //EXTENSION
	{
		WORD slot;
		char model[31],sound[31];
	};

	//2024: these allocate sound slots for non-numeric
	//sound files
	WORD SND(WORD),SND(char*a),SND(const wchar_t*a);

	struct SFX //SOM::Struct<126>
	{
			/*FUN_0042eeb0_sfx_shared_sub?_allocating?*/

		union
		{
			char c[1]; //variable-array
			BYTE uc[1];
			BYTE on1; //1 if active? BYTE?			
		};
		BYTE inst; //allocated inst number?
		WORD sfx; //copied from 1st word in sfx.dat
		float scale; //2nd DWORD in sfx.dat data (may be heterogeneous)
		void(*func2)(int); //2 //PTR_FUN_0042efe0_sfx_proc_0_func2_0045e64c
		SOM::MDL *mdl; //3 //DAT_01cdcd44_sfx_ref_table?_16_mdl_instances
		WORD txr,_pad; //4 //DAT_01cdcd40_sfx_ref_table?_mdl_or_txr_field

		float xyz[3]; //5-7

		union //8-10
		{
			float pitch;

			struct
			{
				float incline; //-y component of look vector
				float yaw; //atan computed from look vector
				float roll; //set to 0???
			};
			
			float uvw[3]; //w is set to 0

			float look_vec[3]; //unused
		};
		
		//43a297 sets this (4B) to a single byte om the sfx.dat record
		DWORD duration; //11 //snd

		int unk1; //magic_something; //12 //???

		Attack dmg_src; //13-29
		
		   /*43a243 does this and copies cp1 into xyz*/

		  //WARNING: this may be general purpose memory//

		union //30-32
		{
			struct //candle type
			{
				float (*cp1)[3]; //candles get a (green) CP from somewhere
				float (*cp2)[3]; //candles get a (green) CP from somewhere
			//	float (*cp3)[3];
			};

			float const_vel[3]; //needle type

			BYTE uc30[12]; float f30[3];
		};

		float unk33[18]; //33-50

		float offset[3]; //51-53 //subtracted

		float unk54[24]; //54-77

		float randomizer; //78
		float explode_sz[2]; //79-80
		float unk3[45]; //81-125
	};

	////////////////////////////////////////////////

	template<DWORD addr,class T> struct State
	{
		inline T *operator&(){ return (T*)addr; }
		inline operator T&(){ return *(T*)addr; }
		//NEW: pointer type support (very rare)
		inline T &operator->(){ return *(T*)addr; } 
		inline T &operator=(const T &cp){ return *(T*)addr = cp; }
	};
	template<DWORD addr,class T> struct State<addr,T[]>
	{
		inline operator T*(){ return (T*)addr; }
		inline T *operator&(){ return (T*)addr; }
		inline T *operator->(){ return (T*)addr; }	
	};
	template<DWORD addr,class T,int N> struct State<addr,T[N]>
	{
		typedef T TN[N]; inline operator TN&()
		{
			//bug? for SOM::Items & SOM::Shops 
			TN *C2101 = (TN*)addr; return *C2101; 
		}
		//inline T *operator&(){ return (T*)addr; }
		inline TN&operator&(){ return operator TN&(); }
		inline T *operator->(){ return (T*)addr; }
	};
		
	//REMOVE ME??
	//NOTE: shops use a different memory
	//location (who knows why?) for this
	//same purpose 
	//when shopping the value is changed
	//to 0x1d11ca8
	//
	//REMINDER: there's a flashing pulse
	//at DWORD #46.
	//
	//REMINDER: 8Bs in front is 2D scale
	//values... it could be part of this
	//structure
	//TODO
	//think probably this is 0x19AA978 (+A8 incl. shops)
	//004183F8 68 78 A9 9A 01       push        19AA978h
	//
	// SOM::menupcs = (DWORD*)_19AA978+0xA8/4;
	//
	// size is 7Ah*4 (30)
	// 004212E2 B9 7A 00 00 00       mov         ecx,7Ah
	//
	extern DWORD *menupcs; //menu textures

	//NOTE: 1e9b000 is the end of the module
	extern struct L 
	{	
		L(){ (DWORD&)SOM::L.ai = 0x4C77C8; }

		static const void *zero; //HACK: som.MPX.cpp

		//.text
		//REMOVE ME?...
		//TODO: tie to tap_or_hold_ms_timeout
		//.text
		State<0x42486B,const DWORD> hold; //750 
		
		//.rdata
		//458310 is 4 for death animation id
		//before it is a table of which it's
		//the last?
		//
		//for some reason when monsters interact with the PC the have
		//a hit radius stored in a different location
		//som_logic_reprogram is reassigning them to the 0x458380 one
		//State<0x4582c8,FLOAT> height; //1.8
		State<0x4582cc,FLOAT> hitbox2; //0.25
		//NOTE: #8 is unidentified (deactivate?)
		//0,1,2,1,8,9,a,b,c,f,10,11,6,7,14,16,4
		State<0x4582d0,DWORD[17]> animation_id_table;
		//State<0x458318,FLOAT> //pi/180
		//safe to modify? (som.MDL.cpp does so)
		State<0x45831C,FLOAT> rate; //1/30 (frame rate?) 0.0333
		State<0x458324,FLOAT> fade; //0.0166666675 (1/60s)
		//this is typically used to multiply/divide but 406ab0
		//adds it to the activation radius suggesting it's the
		//player's diameter
		//State<0x458290,FLOAT> half; //0.5
		//FIGURE ME OUT
		//som_state_426D60 manipulates this (0.9 or 1.0?)
//		State<0x458334,FLOAT> ???;
		//this is height but also lower if crouched
		State<0x45837C,FLOAT> duck; //1.8
		State<0x458380,FLOAT> hitbox; //0.25
		State<0x458448,FLOAT> fps; //30
		//I think SomEx treats this as constant WRT [Number]
		State<0x4584FC,FLOAT> height; //1.8
		State<0x458500,FLOAT> shape; //0.25
		State<0x45851C,FLOAT> bob;   //0.075 //pi/42?		
		State<0x458520,FLOAT[2]> nod;   //-0.785398,0.785398
		State<0x458528,FLOAT> abyss; //-10
		State<0x45852C,FLOAT> bob2;   //0.0075 //2024 //pi/420
		State<0x458554,FLOAT> fence; //0.51	
		State<0x4585d4,FLOAT> rate2; //0.666667 1/15
		//rdata?
		State<0x45A10C,DWORD> fill; //D3DRENDERSTATE_FILLMODE

		//NOTE: these are replaced by som_state_instructions!
		//State<0x45A700,void*[160]> event_function_pointers;

		//44F713 generates random numbers (SOM::rng)
		//State<0x45F580,DWORD> rng;

		//triggers 447480
		//calls IDirectDraw7::RestoreAllSurfaces
		//can't find any references
		//0040240C A0 10 04 4C 00       mov         al,byte ptr ds:[004C0410h]
		//State<0x4C0410,BYTE> restore_all_surfaces_trigger;
		//padding?
		//is SOMEX_(B) represented?
		//State<0x4C0414,MAX_PATH> "...\" //install path
		//State<0x4C0518,MAX_PATH> "data\map\"; //relative
		//State<0x4C0620,MAX_PATH> ini_file_path; //absolute
		//State<0x4C0728,MAX_PATH> "...\data\obj\model" //absolute
		//
		// not sure why this data is between these path objects 
		//		 
		//0 is bob
		//1 is gauges
		//2 is compass
		//3 is items		
		State<0x4C0852,BYTE[]> on_off; //15+?
		State<0x4C085F,BYTE> f3; //on_off[13]
		State<0x4C0860,BYTE> f4; //on_off[14]
		//		
		//State<0x4c0868,MAX_PATH> "data\map\texture\" //relative
		//
		// several more paths follow...

		//prevents 402240 (step PC)		
		//40224E A0 6C 09 4C 00       mov         al,byte ptr ds:[004C096Ch]
		//see game_over_sequence_selector below (0x4c16f8)
		State<0x4C096C,BYTE> game_over_status;

		//-1 is debug?
		//0 is device
		//1 is width
		//2 is height
		//3	is bpp
		//2018: what is config2? it once
		//created a font, but didn't seem
		//to be repeatable
		State<0x4C0970,DWORD[]> config;
		State<0x4C0830,DWORD[]> config2;

		//pretty sure about this
		//0042337D A1 84 09 4C 00       mov         eax,dword ptr ds:[004C0984h] 
		State<0x4C0984,/*DWORD*/int> filter;

		//controls map loading
		//401ab3 suggests this begins a 300B (75 DWORDS) 
		//structure ending at 4C0CC4
		struct Corridor
		{
			BYTE lock; //1: prevents reentry to map event
			BYTE fade[2];
			BYTE map;
			char som_db[260]; //NOTE: 1d131a8 may be equivalent to this?
			BYTE nosetting; //1 WHEN LOADING SAVE FILES!
			BYTE dbsetting; //0: use command-line or SOM_SYS to load map
			BYTE map_event; //1: triggers saving event data on closing map
			BYTE setting[2]; //tile (not WORD aligned)
			//BYTE _padding[3]; 
			BYTE zindex,_padding[2]; //EXTENSION (assuming unused)
			FLOAT offsetting[3];
			FLOAT heading[3]; //[0] and [2] ignored
			BYTE settingmask; //bit 1 is heading[1]. 2,3,4 is offsetting

			  //41B so far?
		};
		State<0x4C0B98,Corridor[1]> corridor;

		//see game_over_status above (0x4C096C)
		State<0x4c16f8,DWORD> game_over_sequence_selector;

		//this is currently limited to 30fps in 60fps mode
		//to limit poison timing and HP regeneration
		//0x19c1d48 governs the damage flash and is also
		//limited to 30fps by som_game_4023c0
		State<0x4c2244,DWORD> world_counter;

		//402400 writes to logs (printf?) if this is nonzero:
		//2: config information
		//3: memory usage (TPF/VPF)
		//4: VRAM/TEXMEM totals
		//5: other information (fps, etc.)
		State<0x4C234C,DWORD> logging;

		//this causes code at 402503 to be entered
		//that doesn't draw the sky and draws tiles
		//with wireframes in two passes... probably
		//white black wireframes (2) on polygons (1)
		State<0x4C2350,BYTE> wireframes_debug_mode;
		//0040255B 38 1D 51 23 4C 00    cmp         byte ptr ds:[4C2351h],bl  
		//00402561 74 05                je          00402568  
		//00402563 E8 78 1F 01 00       call        004144E0
		//
		// NOTE: som_hacks_DrawPrimitive has code that works with this
		// but nothing is appearing onscreen (hmmm, it might be because
		// dx_d3d9x_drawprims_static is destructively transforming the
		// vertex data because it can't pass D3DFVF_XYZRHW to a shader)
		// 
		// 2024: this visualizes some possibly temporary tile flags
		// and only seems to be used in development to draw the map
		// in 3 2d passes. its subroutine might have memory overrun
		//
		State<0x4C2351,BYTE> maybe_fairy_map_feature; //bsp debug maybe?

		//00401C8F 88 15 53 23 4C 00    mov         byte ptr ds:[4C2353h],dl
		State<0x4C2353,BYTE> suspends_message_pump;
		
		//xyzuvw is __thiscall associated
		//xyzuvw is __thiscall associated
		//xyzuvw is __thiscall associated
		//WARNING: not necessarily the player
		//u is about the x axis (look up/down)
		//u is divided by a 4:3 aspect ratio???
		//v is about the y axis (turn left/right)
		//w is presumably unused by som_rt/_db.exe
		//State<0X4C2358,FLOAT[6]> xyzuvw;		
		//Ghidra can't seem to refer to this and 
		//applies it the wrong address wherever it
		//appears (all thiscall is this way in Ghidra)
		//0040A745 B9 58 23 4C 00       mov         ecx,4C2358h
		//0040A74A E8 61 69 FF FF       call        004010B0
		// 
		// I think this has to be intimately related to
		// the model view matrix and BSP system 
		// 
		State<0x4C2358,float[3+3]> view_matrix_xyzuvw; //__thiscall?			

		State<0X4c23ec,FLOAT> fov; //0.872664630 (50 degrees)

		//see som_hacks_updatefrustum for diagram
		//the depth is always 300, but far is used
		//(the 300 is stored in *(double*)0x458298)
		State<0x4C23f0,FLOAT[2+8]> frustum; //near/far/4 2D/topdown points
				
		//"GOLD"
		//the save file stores this with memcpy
		//0042D564 B9 00 01 00 00       mov         ecx,100h  
		//0042D569 BE 18 24 4C 00       mov         esi,4C2418h  
		//0042D56E F3 A5                rep movs    dword ptr es:[edi],dword ptr [esi]
		//initialization zeroes their first BYTE
		State<0x4C2418,SOM::Struct<8>[32]> gold; 
		//00403E3C A1 18 28 4C 00       mov         eax,dword ptr ds:[004C2818h]
		State<0x4C2818,BYTE*> gold_MDO_file;
		//gold_MDO_file instances
		State<0x4C281C,som_MDO*[32]> gold_MDO;

		//DAMAGE CALCULUS
		//00404210 A1 A0 28 4C 00       mov         eax,dword ptr ds:[004C28A0h]
		State<0x4C28A0,DWORD> damage_sources_size;
		//som_state_404470 knows about this structure
		//404210 starts at 0x4c28b8
		//4041d0 starts at 0x4c28a8
		State<0x4c28a8,SOM::Attack[64]> damage_sources;

		//images table? not sure?
		//not sure what this is for... maybe SOM_SYS slideshows? they're shown
		//at 640x480 ... they have textures
		State<0x4c3bf0,DWORD> images_size;
		State<0x4c3cfc,SOM::Struct<339>[32]> images; //1356B

		//00406635 45                   inc         ebp
		State<0x4C39C4,DWORD> active_ai_counter; //?
		//004065BB C7 05 C4 39 4C 00 00 00 00 00 mov         dword ptr ds:[4C39C4h],0  
		//004065C5 C7 05 A8 39 4C 00 00 00 00 00 mov         dword ptr ds:[4C39A8h],0 
		//State<0x4C39A8,DWORD> unknown_ai_counter; //???

		//data is the file's header converted
		//to pointers into a a dynamic memory
		State<0x4C67C8,BYTE*[1024]> enemy_pr2_data;
		//2020: going dynamic?
		//WARNING: & operator IS DANGEROUS
		//State<0x4C77C8,SOM::Struct<149>*> ai; //128/ai_size
			som_Enemy *ai;
		State<0x4DA1C8,SOM::Struct<122>[1024]> enemy_prm_file;
		//2020: KF2 has 200 enemy upper limit
		//00405DE0 A1 CC 41 55 00       mov         eax,dword ptr ds:[005541CCh]
		//00405DEB 3D 80 00 00 00       cmp         eax,80h
		State<0x5541C8,BYTE*> enemy_pr2_file;
		State<0x5541CC,DWORD> ai_size;

		//SOM::MDL::data
		State<0x5541d0,BYTE*[1024]> enemy_mdl_files;

		//events?
		//00408A80 A1 D0 51 55 00       mov         eax,dword ptr ds:[005551D0h] 
		//5551D0 is event table?
		//event id is forwarded to 408AE0
		//00408AC7 E8 14 00 00 00       call        00408AE0		
		State<0x5551d0,som_EVT*> events; //1024
		State<0x5553c0,DWORD> events_size;		
		State<0x5555C4,WORD[1024]> counters;
		State<0x555E44,BYTE[1024]> leafnums;

		//this is a bit field that's set after a trip zone based event clears its
		//predicate so that the event won't fire until that trip zone is left and
		//subsequently reentered
		State<0x555dc4,BYTE[128]> evt_trip_zone_bits;

		State<0x556244,BYTE*> evt_file; //408499

		//damage hit tests pass this value to the last argument... it looks quite 
		//large
		//
		// it looks like this holds up to 8 monster hit candidates and 8 separate
		// NPC candidates... player hit is stored in the front... projectile hits
		// may be stored separately too
		// 
		State<0x556250,FLOAT[]> hit_buf_pos; //(40bf80)
		State<0x55625c,FLOAT[]> hit_buf_look_vec;
		//I guess 40b600 sets this global?
		State<0x55626c,DWORD> target_type; //state from 1 2 4 8 16
		State<0x556270,void*> target;
		State<0x556274,BYTE> hit_buf_pc_hit;
		State<0x556294,DWORD> hit_buf_enemies_hit;
		State<0x556298,SOM::Struct<8>[]> hit_buf_enemies;
		State<0x556398,DWORD> hit_buf_NPCs_hit;
		State<0x55639c,SOM::Struct<8>[]> hit_buf_NPCs;

		State<0X5565c4,WORD> keyboard16; //16 keys
		State<0X5565C8,BYTE[]> controls; //8 buttons (assignments?)
		//this is the effective controller
		BYTE &controller(){ return controls[9]; }
			
		  //IN-WRONG-ORDER
		  //this is the prospective controller
		  State<0x19AAB74,BYTE[]> control_;

		//this is copied to the per map save data as is???
		//there's a 64-bit mask corresponding to instances
		 //on a per type basis		
		State<0x5567D0,QWORD[256]> items_64bit_masks;				  
		//appears to be 64 pointers per item profile to a data structure
		//that includes call 446010 3D render stuffs
		State<0x556fd0,som_MDO*[256][64]> items_MDO_table;

		//Reminder: item_pr2_file has 6 more records than item_prm_file
		State<0x566ff0,SOM::Struct<84>[250]> item_prm_file;
				//just 8B here
		//there's more per item info here that's saved to the save file
		//it starts with 6dof coordinates (are they saved?)
		//the save data is 3B plus pad byte. it's not dropped item data
		//includes instances counter
		//(it corresponds to the bitmask)
		//why 3 copies of item coordinates?
		//
		//  -FLOAT 0-5 (24B) is 6DOF coordinates
		//  -unknown 4B
		//	-WORD at byte 28 is the PRM record (40fe2c)
		//  -unknown 4B
		//  -BYTE 34-35 are loaded from save data (42d240)
		//  -unknown 4B
		//
		State<0x57B818,SOM::Struct<10>[256]> MPX_items;
		//this data (2304B) is saved directly to the save file as it is
		//State<0x57E038,SOM::Item[64]> items;
		State<0x57E038,SOM::Item[256]> items;
		
		//items use this for timestamps, gauges pulse on it, blindness
		//pulses on it, Ghidra doesn't show that many references to it
		//it runs twice as fast at 60 fps
		State<0x57e054,DWORD> world_step;

		//Reminder: item_pr2_file has 6 more records than item_prm_file
		//Ex uses #250 for magic entries in a unified/expanded PRM table
		//2021: the som_game_nothing items really should use these too to
		//make it easier to detect them (starting at #251 I guess)
		State<0x580438,DWORD> item_pr2_size;
		//State<0x58043C,DWORD> item_pr2_size; //???
		State<0x580440,SOM::Struct<22>[256]> item_pr2_file;

		//this seems to be a copy of items_MDO_table holding the files
		//rather than the instances
		State<0x585c40,BYTE*[250]> item_MDO_files;

	//	State<0x58605C,BYTE*> take_MDO_file; 
	//	State<0x586060,som_MDO*> take_MDO;		

		//start is unverified 
		State<0x5861a0,SOM::Struct<28>[128]> lamps;
		//411400 sets this to *589bc0 that's in turn  set on map load
		//by 589bc0
		//State<0x5899a8,DWORD> lamps???
		//GUESSING (411050)
		State<0x5899a8,DWORD> lamps_size; //128

		State<0x58a920,DWORD> mpx_ibuffer_size;
		//this pointer does change, even though
		//its size doesn't
		State<0x598928,SOM::MPX::Static[1]> mpx; //68416B
		//		 68416
		// 
		// 
		// WHAT'S IN THIS HUGE GULF???
		// WHAT'S IN THIS HUGE GULF???
		// WHAT'S IN THIS HUGE GULF???
		//
		// 
		// 
		//WARNING: even though the hard limit is 896 the real size
		//of this vertex buffer is 4096 just like the MDO vbuffer!
		State<0x19aa968,DWORD> mpx_vbuffer_size;


		//MAIN MENU region?
		//
		//4183f0 drives this model (would be be nice to control its
		//viewing angle, etc. with a controller)
		State<0x19aa9b4,som_MDO*> menu_MDO;
		//this is returned by 4183f0 (4253a2) via the third parameter
		//
		// I guess there's no easy way to use an item? 4085d0 uses it
		// but doesn't remove it from inventory or handle failures
		//
		//State<0x19aab60,BYTE> used_item_id; //defered menu item action?

		State<0x19aab64,DWORD> main_menu_selector; //jump table at 418700

		State<0x19AB530,LONG> subtitle_timer;

		State<0x19AB950,SOM::Struct<80>[250]> magic_prm_file;		
		State<0x19BF1CC,DWORD> magic_pr2_size;
		State<0x19BF1D0,SOM::Struct<10>[256]> magic_pr2_file;
		
		//42464c calls 440f30 and 440520 to initialize
		//these (they seem quite large for model data)
		//SOM::MDL::data
		State<0x19c19ec,SOM::Struct<622>*> arm_MDL_file;
		State<0x19c19f0,SOM::MDL*> arm_MDL;
		//42734c loads two arm equipment pieces (MDO) into
		//19c1a08 and the MDO records go into 19C19F4 (-16)
		//it looks like 19c1a04 is the weapon MDO and 19c1a10
		//is also drawn, maybe it's body armor?
		State<0x19C19F4,SOM::Struct<>*[4]> arm_MDO_files;
		State<0x19c1a04,som_MDO*[4]> arm_MDO;
		som_MDO *arm2_MDO[4]; //EXTENSION

		//high byte is inventory. FF for empty
		//low byte and 0 then means??? no clue
		//(0 means empty. Not sure about FF??)
		//(0x80 seems to mean it's unpacked)
		//also constant implicating the player
		//(probably just the start of PC data)
		State<0x19C1A18,WORD[]> pcstore; 
		//0: weapon
		//1: helmet
		//2: upperbody
		//3: arms
		//4: legs
		//5: shield
		//6: accessory
		//7: magic via Sys.dat
		State<0x19C1C0C,BYTE[8]> pcequip;
		BYTE pcequip2_in_waiting[8],pcequip2; //2021
		//stating with 19c1c14 there are 13 of 
		//these (427a40)
		State<0x19c1c15,BOOL> equip_based_blind; //blind?
		State<0x19c1c16,BOOL> equip_based_curse; //curse?
		//State<0x19c1c19,BOOL> equip_based_427d70; //427d70?
		//State<0x19c1c1c,BOOL> equip_based_427d70; //427d70?
		State<0x19c1c1d,BYTE> equip_based_str_up;
		State<0x19c1c1e,BYTE> equip_based_mag_up;
		//???
		//See PC below
		State<0x19C1C24,WORD[]> pcstatus;
		State<0x19C1C40,DWORD> pcmagic;
		//1) weapon, 2) magic, 3) sword-magic
		State<0x19C1C48,SOM::Attack[3]> pcattack;		
		State<0x19C1D14,SHORT[8]> pcdefense;
		State<0x19c1d2a,WORD> pcmagic_refill;
		State<0x19C1D34,BYTE[8]> pcmagic_shield_ratings;
		//State<0x19C1D3c,BYTE[8]> pcmagic_shield_timers; //BYTE is too small
		DWORD pcmagic_shield_timers[8] = {};
		State<0x19C1D44,INT32> pcdamage_display; //hp
		State<0x19c1d48,DWORD> damage_flash; //0-10
		State<0x19c1d4c,DWORD> bad_status_mask;
		State<0x19c1d50,DWORD[5]> status_timers;
		State<0x19c1d64,DWORD[5]> status_timers2; //???
		State<0x19c1d78,DWORD> damage_taken; //404375->425bc7
		//like xyzuvw above except
		//y is situated on the ground or base CP
		//This copy is updated by GetDeviceState
		//Another copy exists right behind the w
		//that is not current (probably a delta)	
		State<0x19C1D98,FLOAT[6+6]> pcstate;
		State<0x19C1DB0,FLOAT[6]> pcstate2; //pcstate+6 
		State<0x19C1DC8,BYTE> pcstep; //experimental
		//00426E73 D9 1D CC 1D 9C 01    fstp        dword ptr ds:[19C1DCCh]
		//00426EFC D9 1D CC 1D 9C 01    fstp        dword ptr ds:[19C1DCCh]
		State<0x19C1DCC,FLOAT> pcstepladder;		
		//State<0x19c1dd0,FLOAT> bob_sinewave; 
		State<0x19c1dd0,FLOAT> bobbing;
		State<0x19c1dd4,FLOAT> bob_counting_down_from_420; //??? //2024
		State<0x19C1DD8,DWORD> dashing; //0~750+
		State<0x19C1DDC,BYTE> swinging;		
		//flags in save data
		State<0x19C1DDE,BYTE> mode2;
		State<0x19C1DDF,BYTE> save2;		
		State<0x19c1de0,INT32> pcmagic_support_index;
		State<0x19c1de4,DWORD> pcmagic_support_timer;				
		//SOM::MDL::data
		State<0x19c1de8,BYTE*[1024]> NPC_mdl_files;

		State<0x19C2DE8,SOM::Struct<80>[1024]> NPC_prm_file;		
		State<0x1A12dE8,DWORD> ai2_size;
		State<0x1A12DF0,SOM::Struct<43>[128]> ai2;	
		State<0x1A183F0,BYTE*[1024]> NPC_pr2_data;
		State<0x1a193f0,BYTE*> NPC_pr2_file;

		State<0x1a193fc,BYTE*[1024]> obj_MDL_files;
		State<0x1a1a3fc,BYTE*[1024]> obj_MDO_files; //don't match mdl3???
		State<0x1A1B3FC,DWORD> ai3_size;		
		//State<0x1A1B3FC,DWORD> obj_pr2_size; //ai3_size???
		State<0x1A1B400,SOM::Struct<27>[1024]> obj_pr2_file; //108B
		State<0x1A36400,SOM::Struct<14>[1024]> obj_prm_file; //56B
		State<0x1A44400,SOM::Struct<46>[512]> ai3; //512?

		//-1: in world
		//0: company title
		//1: opening movie/sequence
		//2: press START prompt
		//3: start menu
		//9: continue menu
		State<0x1A5B5EC,INT32> startup;
		//42c730
		State<0x1a5b604,void*[8]> startup_images;

		//31264B (7A20) per map save data 
		//0042D332 81 C5 48 B6 A5 01    add         ebp,1A5B648h 
		//the first byte is a marker that is either equal to the 
		//map, or 255 if it's never been saved to the save file
		//(or closed in the game)
		//0042D33B B9 88 1E 00 00       mov         ecx,1E88h  
		State<0x1A5B648,SOM::Struct<7816>[64]> mapsdata;
		State<0x1a5f868,SOM::Struct<7816>[64]> mapsdata_MPX_items; //4*256B //42d240
		State<0x1a5fc68,SOM::Struct<7816>[64]> mapsdata_items; //4*0x900B
		State<0x1a62068,SOM::Struct<7816>[64]> mapsdata_items_64bit_masks; //4*512B
		State<0x1a62868,SOM::Struct<7816>[64]> mapsdata_gold; //4*256B
		State<0x1A62C68,SOM::Struct<7816>[64]> mapsdata_events; //1024B
		State<0x1a5b668,SOM::Struct<7816>[64]> mapsdata_enemies; //2B each
		State<0x1a5b768,SOM::Struct<7816>[64]> mapsdata_npcs; //2B each
		State<0x1a5b868,SOM::Struct<7816>[64]> mapsdata_objects; //32B each
		//
		//// 1A63068 ends first map data /////

		//5 flashes?
		State<0x1c43e48,SOM::Struct<398>[5]> fullscreen_quads; 
		//fullscreen_quads ends at 1C4460E
		State<0x1c45d60,void*[4]> fullscreen_SFX_images;
		void *fullscreen_SFX_images_ext[220-211] = {}; //2024: more textures
		//EXPLORATORY
		//
		// this includes "flames" and explosion images
		// 42ec40 draws both. the images are fed to the
		// transparency buffer and are probably using a
		// data structure identical to the MDL sections
		//
		State<0x1c45d70,som_scene_picture[512]> SFX_images; //512?
		State<0x1c8dd70,SOM::MDL*[512]> SFX_models; //512?
		//workshop.cpp has sfx_record
		State<0x1C91D30,sfx_dat_rec[1024]> SFX_dat_file; //48B apiece (1024)
		State<0x1c9dd30,DWORD> SFX_images_size;
		State<0x1c9dd34,DWORD> SFX_models_size;
		//
		// 1c9dd38-to-1cdcd38 7e*4B (512)
		//
		// 42ebe0 passes these to the second procedure proc
		// they're quite large (their back 94*4 are all 0s)
		//
		// the second WORD selects the SFX.dat table record
		// the third dword is an SFX_models pointer assigned
		// to SFX_models by 42ebe0 anew each world frame
		//
		// 0043A35E 8B 45 2C             mov         eax,dword ptr [ebp+2Ch]
		// DWORD PTR[0x2C] is a timer that ticks down important
		// to controlling the frame rate. the second SFX function
		// decrements it at its end
		//
		// it adds models to 1c8dd70 without bound checking 
		// it clears 1c8dd70 before its loop
		//
		State<0x1c9dd38,SOM::SFX[512]> SFX_instances; //512		
		//byte 4~8 is counter 
		// 
		// 2020: these are instances. 42ebe0 iterates over them
		// calling the second procedure proc
		//
		// 42e5c0 loads the MDL data. 42e970 has unloading code
		//
		// 0: SFX type (1B)
		// 4: ref count (4B)
		// 8: MDL or TXR (4B)
		// 16: 16 MDL instances (64B)
		//
		struct SFX_ref //80B
		{
			BYTE type, _pad[3];
			DWORD ref_count;
			BYTE *mdl_or_txr; //MDL::data
			SOM::MDL *mdl_instances[16];
			DWORD _unknown; //UNUSED?
		};
		State<0x1CDCD38,SFX_ref[255]> SFX_refs;
		
		State<0x1ce1cf4,BYTE*> kage_mdl_file;
		State<0x1CE1CF8,BYTE> kage_dev_caps; //bool 43cda0 GetCaps

		State<0x1CE1D02,SOM::Shop[128]> shops;

		//NOTE: NPC and enemy SNDs assigned by PRF
		//files are never unloaded from what I see
		State<0x1D11E54,WORD[1024]> SND_ref_counts; //1024
		
		State<0x1d11e51,BYTE> snd_volume; //SOM::seVol?
		State<0x1d12654,BYTE> bgm_volume; //SOM::bgmVol?

		//2 is MIDI (43f7e4) 1 is wav (43f83f)
		State<0x1d1265c,DWORD> bgm_mode;

		//43f9f0 is used to copy this into save data
		//
		// NOTE: only the 32B file name is copied... 
		// I'm not sure if the save data contains 32
		// of 260 bytes (need to find out)
		//
		State<0x1d12670,char[MAX_PATH]> bgm_file_name;

		//lvUP table
		//every other is 4 bytes
		State<0x1D12778,DWORD[]> lvtable;

		State<0x1d12a90,BYTE[33760]> sys_dat;
		//
		//dash flag
		State<0x1D12B8E,BYTE> mode;		
		//meters per second
		State<0x1D12B90,FLOAT> walk;
		State<0x1D12B94,FLOAT> dash;
		//degrees per second
		State<0x1D12B98,SHORT> turn; 
		//next 32 are levels
		//(0 for event based acquisition)
		State<0x1D12D22,BYTE[32]> magic32;
		State<0x1D12D62,BYTE> save;
		//0-2: seal? lock? wrong key?
		//3-5: no effect? no mp? lv up?
		//6-8: gnosis? str? mag? 
		//9-11: empty? dead? opened? sys_dat_messages_9_11
		State<0x1D12D68,char[9][41]> sys_dat_messages_0_8;		
		/*
		struct Setup //SOM_SYS
		{
			WORD str,mag,hp,mp;
			DWORD gold,experience; 			
			BYTE level,equipment[8],items[250];
		};*/
		//+sizeof(Setup) is som_db.exe's setup #2
		State<0x1D12F80+sizeof(SOM::Setup),SOM::Setup[1]> setup;
		//
		State<0x1d131a8,char> debug_map; //map supplied to som_db (overwritten?)
		State<0x1D131A9,char[1024][31]> counter_names;
		//
		State<0x1D1ADAA,WORD[16]> sys_dat_16_sound_effects;
		State<0x1D1ADF0,char[3][41]> sys_dat_messages_9_11; //empty? dead? opened?
		State<0x1d1ae6b,BYTE> sys_data_menu_sounds_suite;

		//0043FC85 66 8B 04 45 70 AE D1 01 mov         ax,word ptr [eax*2+1D1AE70h]
		//
		// this is 4 WORD values that are written to the save file prior to writing
		// the level data. this code comes from subroutine 43FC70 that is only used
		// in one other place that suggests these are the 4 event timers
		//
		State<0x1D1AE70,WORD[4]> timers; //4

		//set before/after menus (may freeze animation?)
		//
		// NOTE: COULDN'T USE IT IN SOM.RECORD.CPP TO ELIMINATE GHOSTS IN ANIMATION
		//
		State<0X1D1AE78,BYTE> timers_stopped; 

		State<0x1D1AE7C,DWORD> et; //43fc60 returns this

		//FULLSCREEN SPRITES EFFECT? MAYBE FLASH EFFECTS???
		//4471b0 gets (4471f0 sets)
		State<0x1D3D208,DWORD> vp_bpp;
		State<0x1D3D20C,DWORD> vp_width;	
		State<0x1D3D218,DWORD> vp_height;

		//4476c0???
		//State<0x1D3D21C,DWORD> ;
		

		//State<0x1d3d224,IDirectDraw7*>; //447690 returns this
		//State<0X1d3d228,IDirect3D7*>; //4476a0 returns this
		//State<0X1D3D23C,IDirect3DDevice7*>; //4476b0 returns this?

		//struct Material //D3DMATERIAL7 is undefined here
		//{
		//  //shared materials look mostly vestigial. models
		//  //don't share
		//	DWORD extant; D3DMATERIAL7 d3d7; DWORD nonshared;
		//};
		//these are 0x4c, which is 8B more than sizeof(D3DMATERIAL7)
		//447f8e sets the first dword to 1 and the last dword to 0
		//4480db sets the last dword to 1, could mean not unique?
		//448290 grows the buffer 4096 at a time up to ~65535
		State<0X1d3d248,SOM::Struct<19>*> materials;
			//0X1d3d24c?
		State<0X1d3d250,DWORD> materials_count;
		State<0X1d3d254,DWORD> materials_capacity;

		//guessing where this starts based on this
		//004487E5 8B 80 F0 D2 D3 01    mov         eax,dword ptr [eax+1D3D2F0h]
		//(1D3D2F0 is the first IDirectDrawSurface7)
		State<0x1D3D258,Textures> textures;
		State<0x1d65270,DWORD> textures_counter;
		//...
		//textures_out_of_1024
		struct _struct_1D6525c //UNUSED (April 2019)
		{
			void *interface1; //other dx9c interface
			DWORD unknown16; //16?
			void *interface2; //other dx9c interface
			void *device3d; //DDRAW::IDirect3DDevice7
			DWORD unknown1; //1?
			//after this 0x448660 returns -1 (0xffffffff)
			DWORD textures_out_of_1024;
			DWORD unknown7[7]; //0?
			void *interface3; //other dx9c interface
			DWORD unknown17[17]; //0?
			DWORD unknown50902; //0x50902 (329986)
			void *keyboard_interface; //dx7 keyboard  (DirectInput)
			//looks like keys, but more down than you'd think
			char kbstate[256], kbstate2[256];
			DWORD maybe_zero; //maybe off here
			void *joypad_interface; //dx7 joypad (DirectInput)

		  //JOYPAD DATABASE FOLLOWS... maybe it can be updated 
		  //to hot plug controllers!!!
		};
		State<0x1D6525C,_struct_1D6525c[1]> direct_x_cluster;

		State<0X1D654A0,DWORD> controller_count;

		//these are Direct Sound buffer objects and accessory data
		//State<0x1d685f8, > //12B each

		//MIDI?
		//0044c8ce a1 48 9d d6 01        mov        eax,[1d69d48h]
		State<0x1D69D4C,void*> bgm_HMMIO; 
		struct WAVEFORMATEX //FIRST 
		{
			WORD        wFormatTag;         /* format type */
			WORD        nChannels;          /* number of channels (i.e. mono, stereo...) */
			DWORD       nSamplesPerSec;     /* sample rate */
			DWORD       nAvgBytesPerSec;    /* for buffer estimation */
			WORD        nBlockAlign;        /* block size of data */
			WORD        wBitsPerSample;     /* number of bits per sample of mono data */
			WORD        cbSize; 
		};
		struct BGM
		{
			WAVEFORMATEX fmt;

			//0044C887 8B 35 60 9D D6 01    mov         esi,dword ptr ds:[1D69D60h]  
			//0044C88D 8B 0D 64 9D D6 01    mov         ecx,dword ptr ds:[1D69D64h]  
			//0044C893 A1 68 9D D6 01       mov         eax,dword ptr ds:[01D69D68h]
			INT32 size; //1D69D60
			INT32 head; //1D69D64 (file read head)
			//0044C864 A1 68 9D D6 01       mov         eax,dword ptr ds:[01D69D68h]
			INT32 rate; //1D69D68 (2*alignment*sample rate)

			//44c790 deals with this
			//each one it progresses from 1 to 2 back to 1 or 0 if stopped
			//if both are 0 it stops rewinds but doesn't play
			//I think the dsound buffer is split so that only one half can
			//be locked since the other half is mid-play but I'm uncertain
			//it's just a theory. that seems super low-level TBH. both are
			//set when it's first played
			DWORD dual_buffer_status[2]; //1D69D6c

			BYTE loop; //1D69D75 //MIDI is at 1d11e50
		};		
		State<0x1D69D4C,BGM[1]> bgm;

		//fixed in two places to different values:
		//43f31c fixes to 0x41700000 (15.0) (used value)
		//44af9b fixes to 0x4e6e6b28 (1000000000) (direct sound default)
		State<0x1D69D78,FLOAT> snd_max_dist;
		//why are these far apart in memory?
		//43f31c fixes to 2.5
		State<0x1D685fc,FLOAT> snd_min_dist;

		//44b990 loops over these (44bdf0 assigns a WAV buffer)
		State<0X1D69D80,DWORD> snd_bank_size;
		//the first so many of these are general use (working) buffers
		//I think it's 1+4, meaning BGM+4 SFX buffers (sound is originally
		//limited to 4 mixed sounds)
		//
		struct WAV
		{
			WAVEFORMATEX fmt;

			INT32 unknown5;
			INT32 unknown6;

			void **sb; //DSOUND::IDirectSoundMaster
			void **sb3d; //DSOUND::IDirectSound3DBuffer
		};
		State<0X1D69D84,WAV*> snd_bank;

		State<0X1D69D88,void*> dsound_listener;

		//44bea0 relates this to DirectSound something (buffers?) at 1d685f8
		//there's currently 5 (seems to map to snd_bank/1d69d84?)
		//the first so many of snd_bank must be general purpose buffers
		//(44AF8d is 5 in program memory)
		State<0X1D69D8C,DWORD> snd_dups_limit;

		//NOTE: 44cf30 clears a structure at 1D69Da8 and another at 1d6a248
		//2022: 1D69Da8 is render-state stack memory
		//2022: 1d6a248 is 17 render-states (som_scene_state) 
		State<0x1d69da4,DDRAW::IDirect3DVertexBuffer7*> vbuffer; //4096 //D3DFVF_VERTEX		
		//NOTE: these are allocated but I don't think they're ever used (44d01b/44d035)
		/*2022: som_hacks_CreateVertexBuffer prevents this memory waste
		State<0x1d6a28c,DDRAW::IDirect3DVertexBuffer7*> sprite_vbuffer; //4096 //D3DFVF_LVERTEX
		State<0x1d6a22c,DDRAW::IDirect3DVertexBuffer7*> screen_vbuffer; //4096 //D3DFVF_TLVERTEX*/

		//FYI argc/argv are name of C's "main" function parameters
		State<0x1d6a2b4,DWORD> argc; State<0x1d6a2b8,char**> argv;

	//  0x01d6c000 is write access violation
	//	State<01d67960,WORD[]> mpx_client_vbuffer;
	// 
	}L;

	namespace PC
	{	
		enum
		{
		//_: base
		str=0,_str=1, //Strength
		mag=2,_mag=3, //Magic
		
		//_: max
		hp=4,_hp=5, //HP
		mp=6,_mp=7, //MP

		//pts: 32bit
		c=8, pts=10, //Gold, Exp.

		lv=12, //level

		//magic table: 32bit
		//?=14,

		//power gauge of last attack
		attack_power=17,
		
		//gauges
		p=128,m=129, //0~5000
		o=130,a=131, //refill speed
		w=132,g=134, //???
			
		//8 bytes
		//magic cooldowns?
		//???=140

		//32bit	
		//set on hit (hp loss)
		//???=144
		//countdown: 32bit	
		//set on hit (invincibility?)
		//???=146

		//status: 8bit
		//REMINDER: damage inflicted only
		//(equipment status is separated)
		xxx=148, //bitmask				
		//32-bit?
		//countdown: 
		//1000 centisecs
		xxx_timers = 150,
		poison_timer=150,
		palsy_timer=152, 
		dark_timer=154,
		curse_timer=156, 
		slow_timer=158,
	
		last_word //below
		};
	}	

	//REMOVE ME?
	namespace AI
	{	
	//compiler is not always
	//respecting enum typing

		const //enum:
		SOM::C::selector
//		{
			//_: initial state
			_obj_animated = 3,
			_obj_shown = 4, 

			frequency = 34, //1~100
			entry = 35,
			instances = 36,

			item3 = 36,
			obj_item = 36,
			obj_key = 37,

			item = 38, //item2 = 38

			//this is recorded in the
			//save file if _save below
			//is nonzero
			instance = 52, //?

			attack = 116, //3*68B SOM::Attack[3]

			//these are save data for
			//NPCs
			instance2 = 120,
			//3 post-spawn, like stage
			stage2 = 121,
			standing_up2 = 122,

			//objects from MPX loader?
			//121 = 1
			//122 = byte 4 from MapComp? (visibility)
			//123 = 0
			//124 = 1
			obj_valid = 121, //??? //
			obj_shown = 122,
			obj_move_evt = 123,

			stage3 = 124, //42afb0

			standing_turning2 = 160, //???

			//compared to PRM to delay 
			//attack and honor cyan CP???
			//
			// REMINDER: no animation plays during this
			// time (it couldn't hurt to improve that!)
			//
			attack_recovery = 551;

		/////GENERAL USE MEMORY?///

			//? = 588; //4B
			//? = 592; //4B
//		}

		const //enum:
		SOM::F::selector 	
//		{
			//2020: looks like _xyz=15?
			//2022: NPCs use _xyz also
			//but _xyz2 below exists
			//for an unknown reason
			_xyzuvw=1,_xyz=1,
			_xyz3=2, _xyzuvw3=2,
			_uvw=4, _uvw3=5,

			//2020: looks like 25 is full
			//scale
			_scale = 7, _scale3 = 8, 
			//_: station
			//NOTE: this only goes to xyz
			//(not uvw) and I don't know
			//why it's here (but it is)
			//_xyz2 = 13, //why not _xyz?
			xyz2 = 16, y2 = 17,
			xyzuvw2 = 16,
			xyz3 = 17, y3 = 18,
			xyzuvw3 = 17,
			xyzuvw = 18,
			xyz = 18, y = 19,
			//_xyz=15, //2020

			//v is about vertical axis
			//(u/w may not be anything)
			uvw = 21,

			//f24 = 24, //turn speed?
			turning_rate = 24,

			scale2 = 23, //2023
			scale3 = 23, //2021
			scale = 25, //2020

			//NPC shape
			//todo: confirm what's what
			height2 = 26, 
			height3 = 26, 
			//height,x-width,z,xz-radius
			box3 = 26, radius3 = 29,
			shadow2 = 27,
			diameter2 = 28, radius2 = 29,			

			//40a564 multiplies by 30 fps (60 fps?)			
			obj_move_evt_goal_xyzuvw = 34, //6
			obj_move_evt_step_xyzuvw = 40, //6

			//for "sniper" types this is
			//the maximum attack range or
			//FLT_MAX if none exist
			//for "hunter" types it's the
			//minimum attack range or
			//FLT_MAX
			perimeter = 138,

			//todo: check SOM_MAP scaled
			height = 139, 
			//som_MPX_405de0 reassigns these
			//to something more appropriate so
			//406ab0 and 40b1d0 can work better
			//without repurposing the shadow size
			//
			// it make shadow equal radius 
			// and diameter equal to the max
			// attack range
			shadow = 140, diameter = 141, //RENAME US?
			radius = 142,

			//this is used to test if the
			//monster has data in the save
			//file. it's 1.0 and not scaled
			//NPCs use bool for this purpose
			_save = 143, //?

		/////GENERAL USE MEMORY?///

			//UNION?
			//float? bool? int?
			//LIKELY a "union" that depends 
			//on ai_state			
			//f146 = 146; //set to f24 or f24/2
			turning_rate_goal = 146,
			//f147 = 147; //target angle to PC?
			turning_rads_goal = 147;
			//148 is char (new 4-aligned block)
//		}

		const //enum:
		SOM::S::selector 	
//		{
			object = 0, npc = 16, enemy = 16, //id			

			//2020: yes, mp? I don't know what 
			//else it could be, but it doesn't
			//look to be used
			hp = 56, mp = 57, //enemy
			hp2 = 63; //npc
//		}

		const //enum:
		SOM::I::selector 	
//	    {
			//a bitmask that encodes the enemy's
			//perception of the world around it
			//e.g. is it aware of the PC, etc.
			//(only 4 and 8 look interesting)
			//(it may not extend to 32 bits)
			//4: in vision cone?
			//8: spotted?
			ai_bits = 14, //TENTATIVE NAME

			//what is obj_MDO_files/obj_MDL_files
			mdl3 = 24, 
			mdl2 = 24,
			kage_mdl2 = 25, //0x64
			mdo3 = 25, 
			mdl = 26, //0x68
			kage_mdl = 27, //0x6c

			obj_move_evt_frames = 33, //1

			//passed to 427d70 //vampyrism?
			//compared to AI::hp (406d0c)
			//this must be set by the damage
			//step and processed by the enemy
			//step?
			pending_damage = 135,

			//som_logic_4066d0
			//1: pre-spawn? 
			//2: failed to spawn?
			//3: post-spawn?
			//4: dead
			stage = 143,

			//index into animation_id_table?
			//40707d begins a jump table for
			//these
			//5: set trigger
			ai_state = 144,

			idling_time_goal = 146;

		/////GENERAL USE MEMORY?///

//	    }
	}	

	extern FLOAT f3state[3];
	inline void die(bool f3=SOM::f3)
	{
		if(f3) //NEW	
		{
			for(int i=0;i<3;i++) 			
			SOM::L.pcstate[i] = SOM::L.pcstate2[i] = SOM::f3state[i];
			SOM::L.pcstate[1]+=0.1f; SOM::reset(); //black magic
			SOM::warp(SOM::L.pcstate,SOM::L.pcstate); SOM::pcdown-=20;
			((void(*)(DWORD,DWORD,DWORD))0x42d6f0)(4,0,0); //red flash
		}
		else SOM::L.pcstatus[SOM::PC::hp] = 0;
	}
	static void __cdecl die_4257DB(){ die(); } 

	extern bool record();
	extern int recording;
	extern struct Context
	{
		enum belief
		{ 
		limbo=0,	 
		playing_game,  
		browsing_menu, 
		browsing_shop, 
		reading_text,  
		reading_menu,
		viewing_picture, 
		playing_movie,
		taking_item,
		saving_game,
		game_over,
		}which, swhich; //switch

		inline Context(belief one=limbo)
		{
			which = swhich = one;
		}
		inline Context(int one)
		{
			which = swhich = (belief)one;
		}		   
		inline int operator=(belief one)
		{ 
			if(one!=swhich)
			{
				swhich = one; goto limbo;
			}
			else if(one==limbo) limbo:
			{
				SOM::limbo = SOM::frame; //2020
			}
			else which = one; return one;
		}		   
		/*2021: this ignores limbo status?
		inline bool operator==(belief one)
		{ 
			return which==one;
		}*/		   
		inline operator int()
		{
			return which==swhich?which:limbo; 
		}		   
		operator const wchar_t*()
		{
			if(SOM::paused) //NEW
			return L"press S to begin recording";
			if(SOM::recording) return L"recording";
			switch(//which //_2017_delay
			swhich!=limbo?swhich:which)
			{
			default:			  return L"start of game";
			case playing_game:    if(!SOM::player)				
								  return L"watching game";
								  return L"playing game";
			case browsing_shop:   return L"browsing shop";
			case browsing_menu:   return L"browsing menu";
			case reading_text:    return L"reading text";
			case reading_menu:    return L"reading menu";
			case viewing_picture: return L"viewing picture";
			case playing_movie:   return L"watching movie"; 
			case taking_item:     return L"taking item";
			case saving_game:     return L"saving game";			
			}			
		}

	}context; 

	extern DWORD(*movesnds)[4]; //2023
	extern DWORD shield_or_glove_sound_and_pitch(int=5);	
	extern const SOM::Struct<22>*(*movesets)[4]; //2021	
	extern const SOM::Struct<22> *shield_or_glove(int=5);
	extern char move; //2024
	extern BYTE *move_damage_ratings(int i=SOM::move);

	extern struct Motions //som.mocap.cpp 
	{	
		unsigned tick,diff,frame; float step; //other
		
		bool cling; float ceiling; //inputs

		int floor_instance;
		float floor_object,ceiling_object; //for sound effects

		unsigned swung_tick,swung_id; //arm_ms_windup2

		Motions(int=0){ /*reset_config();*/ }
		void reset_config(),ready_config(DWORD ticks, BYTE *keys);
		float place_camera //returns sky base point along vertical axis
		(float(&analogcam)[4][4],float(&steadycam)[4][4],float swing[6]=0);				
		float *place_shadow(float tmp[3]);

		int aloft; float leanness; //outputs

		float clinging,clinging2,arm_clear;

		int swing_move; float swing, shield,shield2[2],shield3;

		bool freefall;

		WORD fallheight,fallimpact; //for fall damage events

		float dash; //0~1 for pc constant

		float cornering; //EXPERIMENTAL

		MOTION_STATE sixaxis_calibration; //Joyshock

		float dx, dz; //EXPERIMENTAL

	}motions; //singleton

	/*HIGHLY EXPERIMENTAL //UNUSED
	extern struct Climber
	{			
		enum{ EXPERIMENT=0 }; //ENABLED

		int current,prior;

		som_MHM *current_MHM;

		float current_coords[3];

		float starting_point[3];

		Climber(int):current(-1)
		{
			starting_point[1] = FLT_MAX; 
		}

		BYTE push_back(float[3],float);
		
		float reach(float y);

	}climber;*/

	extern struct Clipper //som.MHM.cpp
	{
		const float *pcstate; //inputs
		
		float height, radius; //inputs

		//NEW: goingup doubles as a bugfix but
		//is really a boolean value for climbing
		//it's antigravity. but I prefer "going up"
		int mask; float goingup; //inputs

		float pclipos[3]; float _pclipos2[2]; //height/radius?

		float ceiling, floor,slopefloor,slopenormal[3]; //outputs

		float ceilingarch,elevator; bool slide, cling, obj_had_mhm;

		BYTE clip(float out[3]=0); 
		inline BYTE clip(const float pc[3], float h, float r, int m, float up=0, float out[3]=0)
		{
			new(this)Clipper(pc,h,r,m,up); return clip(out);
		}
		inline Clipper &init(const float pc[3], float h, float r, int m, float up=0)
		{
			new(this)Clipper(pc,h,r,m,up); return *this;
		}
		Clipper(int){}
		inline Clipper(const float pc[3], float h, float r, int m, float up=0)
		{
			pcstate = pc; height = h; radius = r; mask = m; goingup = up;
		}

		//WARNING: THIS RETURNS true IF NO MHM EXISTS, AND DOESN'T 
		//DOESN'T TOUCH THE float FIELDS IF SO
		bool clip(som_Obj&,float[3],float[3]);
		bool clip(som_MHM*,som_MDO*,som_MDL*,float[3],float[3]); //2022
		inline BYTE clip(const float pc[3], float h, float r, int m, float up, som_Obj&o, float out[3]=0, float out2[3]=0)
		{
			new(this)Clipper(pc,h,r,m,up); return clip(o,out,out2);
		}

	}clipper; //OSBOLETE

	//2022: MHM
	extern bool clipper_40dff0(float[3],float,float,int,int,float[3],float[3]=0);

	//REMOVE ME?
	extern bool enteringwallspace(float futurepos[3]); 
	extern bool enteringcrawlspace(float futurepos[3]); 	
	extern bool surmountableobstacle(float futurepos[3]);
	extern bool surmountedcrawlspace(float futurepos[3]); //UNUSED

	//REMOVE ME?
	extern int se_volume,se_looping;
	
	//2022: these are now implemented in som.MPX.cpp
	extern void se(int snd, int pitch=0, int vol=0);
	extern float se3D(float pos[3], int snd, int pitch=0, int vol=0);
	inline void menuse(int snd)
	{
		//19aab0C doesn't include shops?!
		//int *bank = (int*)(0x019aab0C);
		int *bank = (INT*)(&SOM::menupcs[59]);

		if(!bank[snd]) return; //silence?
		else assert(bank[snd]==bank[0]+snd); //Shopping?

		//423480 plays menus sounds with mute option
		//se(bank[snd]);		
		((void(__cdecl*)(DWORD))0x423480)(bank[snd]);		
	}	
	static BYTE bgm(const char *filename, BYTE loop=1)
	{
		return ((BYTE(__cdecl*)(const char*,BYTE))0x43f6d0)(filename,loop);
	}

	static LONG rng(){ return ((LONG(__cdecl*)())0x44F713)(); }

	extern void rotate(float[3], float x, float y);

	extern void rumble();

	extern void subtitle(const char*);
}  

struct som_scene_element; //som.scene.cpp

struct som_MDO : SOM::Struct<1> //31
{
	//data *mdo_data; //<1>

	float transl1[3];
	float _cmp_transl1[3];
	float rotate1[3];
	float _cmp_rotate1[3];
	float transl2[3];
	float _cmp_transl2[3];
	float rotate2[3];
	float _cmp_rotate2[3];
	float scale2[3];
	float _cmp_scale2[3];

	float fade,fade2;

	float controlpoints[4][3];

	WORD *materials;

	som_scene_element *elements;

	typedef som_scene_element::v v;

	struct uv_extra //EXPERIMENTAL
	{
		WORD size,procedure;
		
		float tu,tv; //procedure 0
	};

	struct data //32B
	{
		char *file;
		DWORD texture_count;
		DWORD *textures;
		DWORD material_count;
		float(*materials_ptr)[8];
		float(*cpoints_ptr)[3];
		DWORD chunk_count;
		struct chunk
		{
			//swordofmoonlight_mdo_channel_t
			bool blendmode;
			BYTE extrasize; //EXTENSION
			WORD extradata; //EXTENSION
			WORD texnumber;
			WORD matnumber;
			WORD ndexcount;
			WORD vertcount; 		
			union{ UINT ndexblock; WORD *ib_ptr; };
			union{ UINT vertblock; v *vb_ptr; };

			//EXTENSION
			//struct extra //2021
			struct ext
			{
				//this is used to combine a MDO
				//and MDL file
				BYTE part, skin; //reserving
				WORD part_verts; //safety check
				WORD*part_index; //relocated
				//VERSION 2 (8B)
				uv_extra *uv_fx; //EXPERIMENTAL
			};
			/*SKETCHY
			struct ext *operator->()
			{
				assert(extrasize);
				return (ext*)((DWORD*)this+extradata);
			}*/
			ext &extra() //2022
			{
				assert(extrasize);
				return *(ext*)((DWORD*)this+extradata);
			}

		}*chunks_ptr;

		struct ext_t //debugger
		{
			int refs; //2022

			unsigned uv_tick,uv_fmod;

			som_MHM *mhm; //2022

			som_BSP *bsp; //TEMPORARY?

			BYTE *wt; DWORD wt_size;

		}ext;

		void ext_uv_uptodate();

		DWORD _instance_mem_estimate();
	};

	struct ext_t //debugger
	{
		som_MHM_ball *mhm; //2024: fully transformed

	}ext;

	data *mdo_data(){ return (data*)i[0]; }
	data *operator->(){ return (data*)i[0]; }
};
struct som_MDL //SOM::Struct<250> 
{
	static int fps; //som_MDL_fps

	//NOTE: SOM::MDL is preferred. defining inside SOM
	//causes its global variables to be pulled into any
	//method definitions that can lead to mistakes
	typedef som_MDL MDL;

	int animation(int id);
	int animation_id(int c);
	int animation_id(){ return animation_id(c); }
	int running_time(int c);
	bool ending_soon(int f);
	bool ending_soon2(int f);
	bool control_point(float avg[3], int c, int f, int cp=-1, bool s2=true);
	//2021: update_animation_post separates
	//the copy to the vbuffer so the arm.mdl
	//has a chance to adjust the animation
	//it would be better to draw directly to
	//the vbuffer. draw calls this according
	//to the ext.mdo_uptodate flag
	void update_animation();
	void update_animation_post();
	void update_transform();	
	void draw(void *transparent_elements);
	//2023: speed
	bool advance(int dir=1),advance2(int dir=1);
	void rewind()
	{
		f = -1; ext.dir = 1; if(e!=c) ext.t2 = 0; ext.s = 0;
	}
	void rewind2()
	{
		ext.f2 = -1; ext.dir2 = 1; if(e!=c) ext.t2 = 0; ext.s2 = 0;
	}
	SOM::Animation *find_first_kage();

	struct part //file_head+16
	{
		//swordofmoonlight_mdl_primch_t
		DWORD vertsbase; 
		DWORD vertcount; 
		DWORD normsbase;
		DWORD normcount; 		
		DWORD primsbase;
		DWORD primcount; 
		DWORD _unknown0;
	};
	struct data //2488 bytes (622 dwords)
	{
		char *file_name; //0
		BYTE *file_data; //1 deleted?
		DWORD file_size; //2
		BYTE *file_head; //3 equal to file_data?

		//"uvsblocks" seems to allow up to 3
		//after that the load loop overflows
		//multi-resolution? multi-pass? clip
		//geometry?
		struct layer //same numbers of parts
		{
			//4-11 are set in a loop according
			//to "uvsblocks" ... I guess there
			//must be at least 1 or there will
			//be no setting these that seem to
			//extend to the regular primitives
			part *first_prims_ptr; //4 28B?
			part *ditto_prims_ptr; //5 mutable?
			DWORD shared_uvs_size; //6
			DWORD *shared_uvs_ptr; //7
			short *file_vertex_ptr; //8
			short *file_normal_ptr; //9
			DWORD file_vertex_size; //10
			DWORD file_normal_size; //11
	
			//UNUSED: there might be reserved
			//data for these repeated after the 
			//following data, but 440030 doesn't
			//set them up if so
			//
		}l[3]; //12-27

		BYTE *tim_block_ptr; //28
		BYTE *hard_anim_ptr; //29
		WORD *soft_anim_ptr; //30

		//SKETCHY
		//31 is prims pointer (12B)
		struct primitive //12B
		{
			DWORD unknown1;
			DWORD unknown2;
			DWORD vstart;
		};
		primitive *primitive_buf; //31

		struct hard_anim //8B
		{
			//this seems a bit silly, but the
			//second pointer probably is just
			//pointing to 4B beyond the first
			//see code starting at 4450ad

			WORD *file_ptr; //animation_t
			WORD *file_ptr2; //animation_t+4B
		};
		hard_anim *hard_anim_buf; //32 

		DWORD skeleton_size; //33

		//MYSTERY
		//see soft_anim_byte_buf
		DWORD soft_anim_byte_buf_size; //34
		//MYSTERY
		//1B per soft animation???
		//see code at 44432b
		//
		// this relates to the mysterious FFFEFE00
		// pattern in the 8B header... it looks like
		// it can be variable-length, and assign
		// up to the total number animations??? at
		// which point it would overflow the malloc
		// region. FE is no-op alignment padding, FF
		// is the stop sentinel. 00 is equal to no
		// assignment, but soft_anim_byte_buf_size
		// will be set to 0 instead of 1 if not for
		// it (I don't know how these are used yet)
		//
		BYTE *soft_anim_byte_buf; //35

		//set equal to soft_anim_ptr (intially?)
		//see code at 4442df
		WORD *soft_anim_ptr2; //36

		struct soft_anim //12B
		{
			//WARNING: file_ptr points at an ID
			//but it's universally treated as a
			//4B word, even though file_ptr2 is
			//pointing inside of it. IDs should
			//be 2B... file_ptr2[0] must always
			//be 0 for this to work. see 44438c

			//not animation_t?
			//441340 returns 4B ID
			//this points to after the FFFEFE00
			//pattern, and so on (frame pointers)
			//note, subsequent ptrs are deeper in
			DWORD *file_ptr;
			//441430 adds 1 to this (get_runtime)
			//but it seems like hard animations may
			//exclude the final frame... what about
			//soft ones then?
			DWORD runlength;
			//this points to 2B after the FFFEFE00
			//(like hard_anim, seems pretty silly)
			WORD *file_ptr2;
		};
		soft_anim *soft_anim_buf; //37

		BYTE (*tim_buf)[36]; //38
		DWORD vertex_count; //39 (max per 3 layers)
		float (*vertex_buf)[3]; //40 //soft resets?
		DWORD normal_count; //41 (max per 3 layers)
		float (*normal_buf)[3]; //42

		DWORD element_count; //43
		struct element //44
		{
			DWORD body_part;
			DWORD blend_mode;
			DWORD _unknown1;

		  //// rest is soft animation only ////

			DWORD vindex_count;
			WORD *vindex_buf;
			DWORD vertex_count;			
			DWORD _mode_4_if_0; //0 for mode 4 else 3
			FLOAT *vertex_buf;
			WORD *soft_vindex_buf;

		}elements[64]; //2304B

		DWORD *cp_file; //620

		BYTE _tim_atlas_mode, pad[3]; //621 final

		struct ext_t //debugger
		{
			int refs; //2022

			som_MDO::data *mdo;

			som_MHM *mhm; //2022

			union
			{
				int kage; //deprecated

				SOM::Kage *kage2; //som.kage.cpp	
			};

		}ext;

		DWORD _instance_mem_estimate();
	};
	data *mdl_data;
	data *operator->(){ return mdl_data; }

	float xyzuvw[6],scale[3]; //1-9

	float fade,fade2; //10-11 {1,-1}

	//c: target animation?
	//d: target frame?
	//e: current animation?
	//f: current frame?
	INT32 c,d,e,f;

	DWORD _unknown2; //16			
	WORD *hardanim_read_head; //17

	struct bone //272B 0x110
	{
		//bool? why is this? what to call 
		//it?
		BYTE mapped; //0
		BYTE _unknown1[3]; //1-3B
		int parent; //4-7B

		float uvw[3]; //8-19B
		float scale[3]; //20-31B
		float xyz[3]; //32-43B
		float nonlocal[4][4]; //44-107B
		float local[4][4]; //108-171B //scale?
		float xform[4][4]; //172-235B

		//36 bytes 236-271
		//a mystery alternative animation
		//mode uses this... it just 
		//adds a fraction of these values
		//to the real values and the 
		//regular animations can't play
		//the enemy state routine checks
		//for the mode
		//_mystery_mode coordinates it
		//
		//it may have been an abandoned
		//interpolation system
		FLOAT _uvw_scale_xyz2[3*3]; 
	};

	bone *skeleton; //18

	//swordofmoonlight_softanimframe_t
	//(444438)
	//@20 counts up to the time in @19
	//until @19 is incremented
	WORD *softanim_read_head; //19
	DWORD softanim_read_tick; //20

	//EXPLANATION?
	//
	// I think maybe this system was for
	// blending animation transition from
	// the cancellation frame to the start
	// frame of the next animation. I think
	// KF2 may do something like that
	//
	struct //UNUSED (PROBABLY)
	{	//
		// som_MDL_4418b0 is turning this
		// on to let MDLs without CP files
		// to not crash. I didn't know what
		// I was doing when I set that up
		//
		//DISABLED FUNCTIONALITY?
		//this is an alternative animation
		//mode. it may be an unused spawn
		//animation. (it may blidly blend 
		//between two animation frames)
		//it might look like a
		//Salamander coming up out of the
		//ground if anim_vertices where
		//initalized to 0,0,0
		//@21 is a bool that enables it
		//441510 turns off @21
		//@22 is tested by 441531 (441510)
		//@23 is a target frame
		BYTE _mystery_mode[4]; //21
		DWORD _mystery_mode2; //22
		DWORD _mystery_mode3; //23
		//
		// these may be related
		//
		//WASTED SPACE?
		//another set of vertices in addition
		//to anim_vertices (41)	 ... it looks
		//like these are used to reset 
		//anim_vertices, but not at the start
		//of a soft animation.
		//
		// som_MDL_440f30_440ab0 is preventing
		// the allocation of this memory since
		// it seems it's not used and is large
		//
		float (*_soft_vertices)[3]; //24
	};

	float xform[4][4]; //25-40 (0x19)

	//animated vertices... allocated 
	//when first byte is nozero
	float (*anim_vertices)[3]; //41

	typedef som_MDO::v v; //pos/lit/uv

	typedef som_scene_element vbuf; //104B

	//these correspond to data::elements
	struct element2 //8B
	{
		data::element *data; 
		
		union //NOTE: these are the same type now
		{
			vbuf *buf; //8B

			som_scene_element *se; //som.scene.cpp
		};
	};		
	element2 *elements; //42

	//I guess this is narrowed to 
	//fill vbuf::material
	DWORD materials[3]; //43-45
				
	DWORD _unknown4[6]; //46-51

	float cp_frame[32][3]; //52-147
	//
	//this is 0,0,0 transformed by xform()
	//so it's probably identical to xyz and
	//the translation part of xform()... I
	//guess it could be an internal object
	//(weird)
	float xyz2[3]; //148-150 //44176e
		/*
	//396B UNUSED?
	//I think this may be a duplicate of
	//cp_frame/xyz2 from the previous frame, 
	//but I've not found it in code (it's
	//the same size, could be different kind,
	//unused if so)
	//??? //151-249
	//
	// this is a wild guess, it fits exactly
	//
	float _cp_frame2[32][3],_xyz3[3];
		*/
	struct //EXTENSIONS
	{
		som_scene_element *mdo_elements;

		WORD *mdo_materials;

		int anim_mask;

		int c2,f2;

		int d2; //for arm.mdl only for now

		//this is just for moving sideways
		//for now. it could go up to 4
		float anim_weights[2];

		bool inverse_lateral;

		bool reverse_d; //REMOVE ME

		bool mdo_uptodate; //som_scene_swing

		bool subtract_base_control_point;

		//there's ample vacant memory here
		//to stuff things without changing
		//the allocated size (could reduce
		//the allocated size)

		WORD *anim_read_head2;
		DWORD anim_read_tick2;

		unsigned pc_clip; //frame number

		struct //EXPERIMENTAL
		{
			operator bool(){ return true; }

			int layers;
			int noentry; //debugging
			bool footing;
			bool cling;
			bool stopped; //UNUSED
			float clinging;
			float falling;
			float falling2; //debugging
			//pcstep/pcstepladder
			float npcstep,npcstep2;
			float reshape,reshape2;			
			//soft is very good and is very
			//similar to how the PC behaves
			//soft2 is where problems begin
			//unfortunately NPCs have needs
			//the PC doesn't soft2 attempts
			//to smooth over
			float softolerance;
			float soft[3],stem[2];
			float soft2[3+1],soft23;

			/*REMOVE ME
			som_MHM *mhm; //2022
			//NOTE: must recompute if [3][3] is 0
			float(*inverse)[4][4];*/

			float cp_accum[2][3]; //2024

			int feelers; //2024

		}clip;

		som_MDL *kage; //2023: som.kage.cpp

		float s,t,s2,t2,speed,speed2; //2023
		int dir,dir2;

		som_MHM_ball *mhm; //2024: fully transformed?

	}ext;
};
struct som_MHM //40B
{
	//0? //unused?
	//maybe the size in the MPX record?
	//TODO: maybe use this to not delete
	//shared/synthetic models
	//int zero; 
	int refs; 

	int verts,polies; //4
	int types124[3]; //12
	float *vertsptr; //24
	struct Polygon
	{
		int type;
		//WARNING: som_MHM_416a50_slopetop
		//is using this. 416a50 uses the Y
		//component too
		float box[2][3]; //Y only?
		int normal; //28
		int verts; //32
		int *vertsptr; //36
		float distance(som_MHM*,float[3]);
	}*poliesptr; //28
	int norms; //32
	float *normsptr; //36

	~som_MHM(); //models_free

	int clip(int,float[5],float[6],int=0); //415450
};
struct som_MHM_ball : som_MHM
{
	float radius,pos[3]; void init_ball();

	bool hit_ball_cylinder(float[3],float h,float r);
	bool hit_ball_ball(float[3],float r);
};

#endif //SOM_STATE_INCLUDED