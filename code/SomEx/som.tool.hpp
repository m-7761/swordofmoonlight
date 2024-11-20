
#include <vector>
#include <string>
#include <algorithm>

//som.tool.hpp is for som.tool.cpp and
//its spinoffs
#ifndef SOM_TOOL_INCLUDED
#define SOM_TOOL_HPP //SOM_TOOL_INCLUDED
#define TOOL(X) \
enum{ X##_exe = 1+EX_COUNTOF(tools) };
#include "som.tool.h"
#define TOOL(X) \
static struct X{ enum{ exe=X##_exe }; }X;
TOOL(SOM_MAIN) TOOL(SOM_RUN) TOOL(SOM_PRM)
TOOL(SOM_EDIT) TOOL(SOM_SYS) //TOOL(SOM_MAP)
TOOL(MapComp) 
TOOL(PrtsEdit) TOOL(ItemEdit) TOOL(ObjEdit) 
TOOL(EneEdit) TOOL(NpcEdit) TOOL(SfxEdit)

#include "som.extra.h"

#include "../Sompaste/Sompaste.h"  
extern SOMPASTE Sompaste;

//don't use CBN_SELCHANGE with 
//WM_COMMAND to notify windows
#define C8N_SELCHANGE 1 //CBN_SELCHANGE
#undef CBN_SELCHANGE //9 //CBN_SELENDOK

//MFC
//not 100% certain which version of MFC
//these are. They are very close to VC6
struct SOM_CCmdTarget //7/28B
{
	//class CObject

	union
	{
		void **_vtable;
		char c[1]; //variable-length
	};

	//class CCmdTarget

	long m_dwRef;
	LPUNKNOWN m_pOuterUnknown;
	DWORD m_xInnerUnknown;  

	struct XDispatch
	{
		DWORD m_vtbl; 
		size_t m_nOffset;
	}m_xDispatch;
	BOOL m_bResultExpected;

	/*Something must go? (CWnd's base is 28B)
	struct XConnPtContainer
	{
		DWORD m_vtbl;
		size_t m_nOffset;
	}m_xConnPtContainer;*/	
};
struct SOM_CWnd : SOM_CCmdTarget //15/60B
{	
	HWND m_hWnd;

	HWND m_hWndOwner; 
	UINT m_nFlags;

	//(protected)
	WNDPROC m_pfnSuper;	
	int m_nModalResult;
	class COleDropTarget *m_pDropTarget;
	
	//#ifndef _AFX_NO_OCC_SUPPORT
	class COleControlContainer *m_pCtrlCont;
	class COleControlSite *m_pCtrlSite;  
};
struct SOM_CDialog : SOM_CWnd //22/88B
{
	//(protected)
	//this is not GetWindowContextHelpId
	//nor is it 0 as afxwin.h comments
	//it's MAKEINTRESOURCE
	UINT m_nIDHelp;
	//MAKEINTRESOURCE also
	LPCSTR m_lpszTemplateName; 
	HGLOBAL m_hDialogTemplate;
	LPCDLGTEMPLATE m_lpDialogTemplate; 
	void *m_lpDialogInit; 
	SOM_CWnd *m_pParentWnd;
	HWND m_hWndTop; 
};
struct SOM_CWinApp : SOM_CCmdTarget //192B
{	
	//class CWinThread

	typedef SOM_CDialog CWnd;
	CWnd *m_pMainWnd;
	CWnd *m_pActiveWnd;
	BOOL m_bAutoDelete; 

	HANDLE m_hThread;
	DWORD m_nThreadID;
	
	MSG m_msgCur;

	LPVOID m_pThreadParams; 
	UINT(*m_pfnThreadProc)(void*);

	void(__stdcall*m_lpfnOleTermOrFreeLib)(BOOL,BOOL);
	class COleMessageFilter *m_pMessageFilter;

	//last mouse position/mouse message (protected)
	POINT m_ptCursorLast; 
	UINT m_nMsgLast; 

	//class CWinApp

	HINSTANCE m_hInstance;
	HINSTANCE m_hPrevInstance;
	LPTSTR m_lpCmdLine;
	int m_nCmdShow;
	
	LPCSTR m_pszAppName;

	LPCSTR m_pszRegistryKey; 
	class CDocManager *m_pDocManager;

	BOOL m_bHelpMode;

	LPCSTR m_pszExeName;
	LPCSTR m_pszHelpFilePath;
	LPCSTR m_pszProfileName;

	//(protected)
	HGLOBAL m_hDevMode;
	HGLOBAL m_hDevNames; 
	DWORD m_dwPromptContext;

	int m_nWaitCursorCount;
	HCURSOR m_hcurWaitCursorRestore; 

	class CRecentFileList *m_pRecentFileList;

	class CCommandLineInfo *m_pCmdInfo;

	ATOM m_atomApp, m_atomSystemTopic;
	UINT m_nNumPreviewPages; 

	size_t m_nSafetyPoolSize;

	void (__stdcall*m_lpfnDaoTerm)();
};

//AFXTEMPL.H
//SOM_PRM seems to abuse (use) this. Not sure where the lists
//start though
template<class TYPE, class ARG_TYPE>
class SOM_CList //: public CObject
{
	//class CObject

	void *_vtable;

	//template class CList

protected:
	struct CNode
	{
		CNode *pNext,*pPrev;
		TYPE data;
	};	
	//AFXPLEX_.H
	struct CPlex //VARIABLE-LENGTH
	{
		CPlex *pNext;
	/*#if (_AFX_PACKING >= 8)
		DWORD dwReserved[1];    // align on 8 byte boundary
	#endif*/
		//BYTE data[maxNum*elementSize];

		void *data(){ return this+1; }
	};
public:

	//Read only should do, if this is to be used at all.
	//(Storing things this way is pretty ridiculous.)
	int GetCount(){ return m_nCount }
	const TYPE &GetHead(){ return m_pNodeHead->data; }
	const TYPE &GetTail(){ return m_pNodeTail->data; }
	void *GetHeadPosition(){ return (void*)m_pNodeHead; }
	void *GetTailPosition(){ return (void*)m_pNodeTail; }
	const TYPE &GetNext(void* &rPosition)
	{
		CNode *pNode = (CNode*)rPosition;
		rPosition = pNode->pNext; return pNode->data
	}
	const TYPE &GetPrev(void* &rPosition)
	{
		CNode *pNode = (CNode*)rPosition;
		rPosition = pNode->pPrev; return pNode->data;
	}
	const TYPE &GetAt(void *position)
	{
		return ((CNode*)position)->data;
	}
	void *Find(ARG_TYPE searchValue, void *startAfter=0)
	{
		CNode *pNode = (CNode*)startAfter;
		pNode = !pNode?m_pNodeHead:pNode->pNext;
		for(;pNode;pNode=pNode->pNext) if(pNode->data==searchValue)
		return pNode; return 0;
	}
	void *FindIndex(int nIndex)
	{
		if(nIndex>=m_nCount||nIndex<0) return 0; 
		CNode *pNode = m_pNodeHead;
		while(nIndex--) pNode = pNode->pNext; return pNode;
	}

protected:// Implementation
	CNode *m_pNodeHead,*m_pNodeTail;
	int m_nCount;
	CNode *m_pNodeFree;
	CPlex *m_pBlocks;
	int m_nBlockSize;
};

struct SOM_CImageList //8B
{
	void *_vtable; HIMAGELIST hil;
};

union SOM_MAP_model //SOM::Struct?
{
	int i[0x81]; float f[0x81]; char c[0x204];
};

//4921ac is derived from the following pattern
//suggesting there is more in front of it, and
//another structure containing a pointer to it
//[46D538()+4] is 4920E8
//004200C1 E8 75 D4 04 00       call        0046D53B
//004200C6 8B 40 04             mov         eax,dword ptr [eax+4]
//004200D3 8D 98 C4 00 00 00    lea         ebx,[eax+0C4h] 
//SOM_MAP_4920E8+0xC4
static struct SOM_MAP_app : SOM_CWinApp //196B header? CWinApp is 192B?
{
	//addresses to lighting globals are taken from here (421F8C)

	DWORD unknown0_0; //part of CWinApp?

	//0x466fe2
	static SOM_CWnd *CWnd(HWND win)
	{
		//BOTH OF THESE WORK??? SIDE EFFECTS?
		//0x468441 calls GetParent??
		return ((SOM_CWnd*(__stdcall*)(HWND))0x468468)(win);
		//return ((SOM_CWnd*(__stdcall*)(HWND))0x468441)(win);
	}

//NOTE: historically 4921ac came first
}&SOM_MAP_4920e8 = *(SOM_MAP_app*)0x4920E8; //CONTIGUOUS...
static struct SOM_MAP_4921ac 
{
	DWORD unknown0_1;

	DWORD modified[3];
		
	inline DWORD modified_flags() //map/evt/sys.dat?
	{
		//return ((BYTE(__thiscall*)(void*))0x413970)(this);
		return modified[0]|modified[1]<<1|modified[2]<<2;
	}

	struct Icon //reverse linked list???
	{
		char icon_bmp[32];

		DWORD il_index; //HIMAGELIST

		Icon *previous; //40F971
	};
	Icon *icons_llist;

	SOM_CImageList *icons[2]; //1 is rotated
	
	DWORD partsN; //PRT count

	struct Part //59*4B //236B
	{
		WORD part_number;
		
		//WORD unused; //high 16-bits (0)
		BYTE angle,rot;

		Icon *icon;

	  ////same as a PRT file////

		//HACK: std::swap was working in debug builds
		struct{ char file[32]; }msm,mhm;
		
		DWORD bytes; //same as PRT file (byte fields)
		
		char icon_bmp[32], description[128];

	}*parts;

	Part *find_part_number(WORD); //2022

		//36B mark

	//00410D3C 66 89 44 D3 48       mov         word ptr [ebx+edx*8+48h],ax	
	//DWORD unknown1[20016]; //80064	
	WORD unknown1_0;
	char title[32]; //1 byte likely padding
	BYTE width,height;
	struct Tile //8B //0x4921ac+72=0x4921F4
	{
		WORD part;
		BYTE spin,ev; //'e' or 'v' (2023: E or V for BSP pin)
		//
		//NOTE: this value is not truncated
		//
		//00410D63 D9 5F 4C             fstp        dword ptr [edi+4Ch]
		FLOAT z; 

		Tile():part(0xFFFF),spin(),ev('e'),z(){}

	}grid[100*100]; 
	DWORD unknown1_1[7];

		//80100B mark

	struct Start //formerly Base
	{
		void elevate(Tile*,Tile*);

		void record_z_index(HWND);

		inline int xyindex()const
		{
			return 100*tile[1]+tile[0];
		}

		//NOTE: this byte is an object-type 
		//for objects, however it seems to
		//not be used. SOM_MAP/MapComp both
		//use the PRM/PR2 file to determine
		//the type, and events do not seem
		//to consult it either
		//AFTER READING the file, if it is 
		//10 or more it is converted to 0
		BYTE ex_zindex; //exension
		BYTE tile[2];
		BYTE ex_original_zindex; //exension
		FLOAT xzy[3]; 
		SHORT uwv[3],_pad;
	};

		/*beginning SOM_MAP_contents_cmp_t*/

	//80100
	struct Object : Start //40 bytes
	{
		FLOAT size; WORD props;

		BYTE layer, shown, profsetup[8];

	}objects[512]; //20480
	
	struct Enemy_NPC : Start //36 bytes
	{
		FLOAT size; WORD props; 
		
		BYTE frequency, entrance; 		
		BYTE instances, prize, shadow,_pad;

		Enemy_NPC()
		{			
			memset(this,0x00,sizeof(*this));
			//0040ec82 66 89 68 fe     MOV        word ptr [EAX + -0x2],BP			
			props = 0xFFFF;
			frequency = 100;
			instances = shadow = 1;
		}
	};

	/*both branches accomplish the same exact thing?
	0041146E 66 3D 80 00          cmp         ax,80h  
	00411472 66 89 83 E4 88 01 00 mov         word ptr [ebx+188E4h],ax  
	00411479 74 12                je          0041148D  
	0041147B 8B 7C 24 18          mov         edi,dwor
	00411482 66 C7 87 E4 88 01 00 80 00 mov         word ptr [edi+188E4h],80h  
	0041148B EB 09                jmp         00411496  
	0041148D 8B 7C 24 18          mov         edi,dword ptr [esp+18h]  
	00411491 BB 80 00 00 00       mov         ebx,80h  
	00411496 33 ED                xor         ebp,ebp
	*/
	DWORD enemies_size; //0x4aaa90

	//100584
	#if 0
	Enemy_NPC enemies[128]; //4608
	#else
	//this is to port King's Field II
	#define SOM_TOOL_enemies2
	//don't allocate if not SOM_MAP!!
	//static Enemy_NPC enemies2[512];
	static std::vector<Enemy_NPC> enemies2;
	struct Enemies
	{
		Enemy_NPC _pad[128];
		Enemy_NPC &operator[](int i)
		{
			return enemies2[i];
		}
		/*operator Enemy_NPC*()
		{
			return enemies2.data();
		}*/
	}enemies;	
	static std::vector<SOM_MAP_model> enemies3;
	#endif
	
	//NOTE: this code overuns if a MAP file has 129 or more
	//enemies
	//00411541 88 46 01             mov         byte ptr [esi+1],al
	//
	DWORD NPCs_size; //used?

	//105196:
	Enemy_NPC NPCs[128]; //4608

	DWORD unknown4;

	//109808
	struct Item : Start //32 bytes
	{
		WORD props; 		
		BYTE frequency, entrance;
		BYTE instances, available,_pad0,_pad1;

	}items[256]; //8192

		/*ending SOM_MAP_contents_cmp_t*/
					   
	//118000  
	BYTE masking;
	BYTE lighting;
	BYTE unknown5_1[2];
	DWORD unknown5_2[2];
	float draw_distance;
	float fog;
	DWORD rgb; 
	//118024
	struct Ambient
	{
		//NOTE: for some reason both lit bytes
		//must be one (are they kept in sync?)
		DWORD color;
		BYTE lit[2],_pad[2];
	}ambient;
	struct Light
	{
		//NOTE: for some reason both lit bytes
		//must be one (are they kept in sync?)
		BYTE color[4];
		WORD ray[2];
		BYTE lit[2],_pad[2];
	}lights[3];
	//118068	
	/*WARNING: using Start for SOM_MAP_foreach
	//
	// I think start_angle does not conform to
	// the other layouts, and don't know what 
	// comes after it!
	//
	BYTE start_z_index; //assuming layer selector
	BYTE start_xy[2];
	BYTE unknown5_4; //pad //ex_original_zindex
	float start_center[3];
	WORD start_angle;
	WORD unknown5_5; //pad (likely)
	WORD Start_requirement[2];*/
	Start start; //HACK?
	char *start_bgm(){ return (char*)(start.uwv+2); }
	char _start_bgm_4[28];
	char worldmaps[3][32];

		//‭118216‬B mark
		
	//assuming it's all here
	DWORD unknown6;
	//118220
	BYTE SYS_DAT_file[33760];
	//‭151980
	BYTE shop_DAT_file[196352];
			
	struct Prof //32 bytes
	{
		//NOTE: items store the 32nd byte in axes
		//it's difficult to see how this could be 
		//useful for SOM_MAP 
		char model[31], axes;
	};

	struct Prop //34 bytes 
	{
		WORD profile; char name[31],_pad; 

		Prop(){ int compile[sizeof(*this)==34]; }
	};

	//348332
	struct ItemProf : Prof //44 bytes
	{
		WORD v,_pad0; FLOAT pivot; 
		
		//76: no clue when this is used
		BYTE byte76, _pad1,_pad2,_pad3;

	}itemprofs[256]; //11264 
		
	//359596	
	typedef Prop ItemProp;
	ItemProp itemprops[250]; //8500

	//368096
	struct ObjProf : Prof //36 bytes
	{
		//kind2 is the receptacle key ID
		//it seems like the receptacle setup
		//won't work if it is not filled out??
		BYTE kind, _pad0,_pad1, kind2;

	}objectprofs[1024]; //36864

	//404960
	struct SizingProp : Prop //40 bytes
	{			  		
		WORD _pad0; FLOAT size;	
	};
	
	typedef SizingProp ObjProp;
	ObjProp objectprops[1024]; //40960 		

	//445920 
	typedef Prof NPCProf;
	NPCProf NPCprofs[1024]; //32768	
	
	//478688 
	typedef SizingProp NPCProp;
	NPCProp NPCprops[1024]; //40960	

	//519648
	typedef Prof EnemyProf;
	EnemyProf enemyprofs[1024]; //32768	

	//552416
	typedef SizingProp EnemyProp;
	EnemyProp enemyprops[1024]; //40960	

	struct Skin //2021
	{
		char skin, file[31];
	};
	static Skin *npcskins; //1024+1024

	//593376
	BYTE magicprofs[256]; //256 //UNFINISHED
	
	//593632
	typedef Prop MagicProp;
	MagicProp magicprops[250]; //8500 //UNFINISHED

	DWORD unknown7;

	struct Event
	{
		union zr //EXTENSION
		{
			float f; int i; struct
			{
				int flag:8,xy:12,z:12;
			};
		};

		char *name;
		BYTE type,_pad1;
		WORD subject;
		BYTE protocol,item;
		WORD cone;
		zr sx,sy; //float
		float cr;
		WORD test,text_i,test_v;
		BYTE test_logic,_pad2;
	};

	//602136 events pointers (672B ea.)
	//00414FDD 8B 85 18 30 09 00    mov         eax,dword ptr [ebp+93018h]
	Event **eventptrs;

	void compile(){ int c[sizeof(*this)==602140]; }

}&SOM_MAP_4921ac = *(struct SOM_MAP_4921ac*)0x4921ac; 

//476000 is .data
//479A18 is?
//47A7CC is?
//482C80 is?
static struct SOM_PRM_47a708 : SOM_CWinApp
{
	//0x2b8
	//0x268. Before it is 0x200,0, but it's
	//possible that is part of SOM_CWinApp.
	DWORD _unknown1;

	//Tends to be 0?
	//0043F63D 83 3D CC A7 47 00 01 cmp         dword ptr ds:[47A7CCh],1 
	//...
	//0043F7B3 33 C0                xor         eax,eax  
	//0043F7B5 5E                   pop         esi  
	//0043F7B6 C3                   ret  
	DWORD _unknown2;

	//These are expanded at start up. They're
	//DATA folder paths, etc.
	char *_folder_paths[25];
	
	struct Dlg102 : SOM_CDialog 
	{
		DWORD _0; //could this be CDialog
		DWORD tab;	//only one is nonzero

		//NOTE: SOM_PRM_this has some methods and
		//container data members of these objects
		SOM_CDialog *pages[6];
		SOM_CDialog &page()
		{
			return *pages[tab]; //breakpoint 
		}
	};
	
	//FYI: This is a stack object. 19fe30?
	Dlg102 &main(){ return *(Dlg102*)m_pMainWnd; }

	//FYI: Each of these is more or less a CWnd array 
	//up to the pointer returned by current_prm_table.
	SOM_CDialog &page(){ return main().page(); }

	//CAN FORGO THIS ALTOGETHER SINCE THE TABS ARE 
	//STORING THIS IN THE USER-DATA NOW
	//this is the main listbox's item number. however
	//not so in the enemy screens's case
	int &item()
	{
		switch(main().tab) //UNCONFIRMED
		{
		case 1: /*case 2:*/ assert(0); break;
		case 4: static int fixme = 0; return fixme; //return by address
		}
		//WARNING: the enemy screen sets this to 1 if
		//one of the wizard pages are open and 0 when
		//they are not (note Ex always keeps one open)
		//for now som_tool.cpp manages this exception
		return ((int*)(&page()+1))[1]; 
	}
	//[eax+60h] is 1?
	//[eax+64h] is the save prompt flag
	//[eax+68h] is? 0?
	//0043F668 8B 48 64             mov         ecx,dword ptr [eax+64h]  
	//0043F66B 85 C9                test        ecx,ecx  
	//0043F66D 75 0B                jne         0043F67A  
	//0043F66F 8B 48 68             mov         ecx,dword ptr [eax+68h]  
	//0043F672 85 C9                test        ecx,ecx  
	//0043F674 0F 84 39 01 00 00    je          0043F7B3 
	int &save()
	{
		//REMINDER: the enemy screen's header is smaller
		//it ends after 2
		int i = 4; switch(main().tab)
		{
		case 0: i = 3; break;
		//Note: 4 has a secondary save flag at 47F838???
		//this save flag is forgotten when changing tabs 
		//som.tool.cpp corrects for this. SOM_PRM prefers
		//the 47F838 flag
		case 4: i = 2; break;
		case 1: case 2: assert(0); //UNCONFIRMED
		}
		return ((int*)(&page()+1))[/*4*/i]; 
	}

	typedef BYTE view[1024];
	//this happens to be the exact same buffer that
	//som_tool_prm would read the PRM files into if
	//SOM_PRM was to hook into it
	view &data()
	{
		int os = 0;
		SOM_CDialog *p = &page();
		switch((int)p->m_lpszTemplateName)
		{
		case 129: os = 0x870; break; //NPCs
		case 138: os = 0x104c; break; //items
		case 140: os = 0x860; break; //magics
		case 141: os = 0x14994; break; //shops
		case 142: os = 0x190; break; //enemies
		case 143: os = 0x634; break; //objects
		}
		//this seems reliable, even though there is some
		//indication that this isn't part of the dialog
		//object (LocalAlloc baadf00d appears inbetween)
		//0762D06C-762c020=104C //items (tried twice)
		//55508A8-5550048 //magic
		//555067C-5550048 //objects
		//55501D8-5550048 //enemies
		//55508B8-5550048 //NPCs
		return *(view*)(os>0?(char*)p+os:0);
	}
}&SOM_PRM_47a708 = *(struct SOM_PRM_47a708*)0x47A708;


 //som.tool.cpp //som.tool.cpp //som.tool.cpp //som.tool.cpp

extern struct SOM_PRM_arm //SINGLETON
{
	BYTE *file;

	FILETIME filetime; 

	//NOTE: clear forces a save, save
	//writes only if filetime changes
	void init(),clear(DWORD=1),save();

	private: bool read(bool);

}SOM_PRM_arm;

//SINGLETON
//MIGRATING from som.tool.cpp to SOM_MAP.cpp
extern struct SOM_MAP_intellisense
{	
	enum{ exe=SOM_MAP_exe };
		
	//AfxWnd42s
	//made by WM_INITDIALOG in this order 
	HWND model,painting,palette;

	struct SOM_MAP_prt &prt; //som_map
	struct SOM_MAP_map &map; //som_map	
	struct SOM_MAP_ezt &ezt; //som_map_zentai

	struct som_LYT ***lyt; //layer table file

	union //EXPERIMENTAL
	{
		//NOTE: SOM_MAP_201_open deletes this
		//2021: SOM_MAP_map2 allocates memory
		SOM_MAP_4921ac::Tile *base,*layer; 
		SOM_MAP_4921ac::Tile(*base100x100)[100][100];
		//2021
		//WARNING: the current map is a donut
		//hole so that if(base) remains valid
		SOM_MAP_4921ac::Tile *layers[7];
	};

	//IntelliSense is not so intelligent when
	//structs have the same name as variables
	SOM_MAP_intellisense(int);

	struct evt_descriptor
	{
		//the start of this is the instruction 
		//name, but I'm not sure how long it goes
		//(256 seems like a lot)
		char name_etc[256];
		WORD code,_; //pad?
		//these are 3 of the same thing...
		//if 0 ignored, if 1 strlen is used...
		//otherwise malloc makes data of its size
		DWORD data[3]; 
	};
	static evt_descriptor *evt_code(unsigned short);
	static evt_descriptor *evt_desc(unsigned short);
	struct evt_instruction
	{
		WORD code, size; char *data[3];
	};
	static evt_instruction *evt_data(HWND,int=-1);

	EX::critical *_cs; //undo

}SOM_MAP;
struct som_LYT //VARIABLE-LENGTH
{	
	//if w2 is nonzero map[0] and map[1] 
	//hold the size of two WCHAR strings 
	//the sizes include the 0-terminator
	BYTE group;
	BYTE pos[3];
	BYTE box[4];
	float elev;
	char map[2]; //2 letter name if !w2[0]
	WCHAR w2[1]; //file name+dropdown name 	

	int legacy() //FIX ME
	{
		//ASSUMING w2 HAS LEGACY FILE NAME
		return *w2?_wtoi(w2):atoi(map);
	}

	struct Place //VALID FOR 1ST LINE ONLY
	{
		//i is the LYT's EX::data(i) folder
		int i; FILETIME time; QWORD legacy;
	};
	Place &data(){ return ((Place*)this)[-1]; }
};
struct SOM_MAP_prt
{
	//REFACTORING
	int indexed; void clear_index()
	{		
		indexed = 0;
		for(size_t i=0,iN=keys();i<iN;i++)
		blob[i].index = missing;		
	}
	SOM_MAP_prt():indexed(),missing(){}

	struct key 
	{
		//2023: just the UUID part
		//wchar_t icon[31];
		wchar_t iconID[31];
		wchar_t profile[15];		
		wchar_t description[31];		
		//HACK: refactoring
		inline size_t missing()const
		{
			return SOM_MAP.prt.missing;
		}
		const wchar_t *longname()const
		{
			return Sompaste->longname(profile);
		}				
		size_t index;
		size_t *index2()const //layer index
		{
			assert(SOM::tool==MapComp.exe);
			return (size_t*)description; 
		}

		mutable FILETIME writetime; //2022
		
		key(const wchar_t *p)
		{
			index = missing(); //!			
			*iconID = *description = '\0'; 
			wcscpy_s(profile,p);
		}		
		inline bool indexed()const //ugly
		{
			if(index!=missing()) return true;
			//HACK: refactoring
			//const_cast<size_t&>(index) = som_map.index++;
			const_cast<size_t&>(index) = SOM_MAP.prt.indexed++;
			return false;
		}
		inline bool reverse_indexed(size_t uglier)const
		{
			//HACK: refactoring
			//key &r = blob[som_map.index++];
			size_t i = SOM_MAP.prt.indexed++;
			if(i>=missing()) return false; //MAP/DATA mismatch?
			key &r = SOM_MAP.prt.blob[i];
			if(r.index!=missing()||r.number()!=uglier)
			return true;			
			r.index = number(); return false;
		}
							  	
		short part_number()const
		{
			auto *p = profile; p:

			if(isdigit(*p))
			{
				wchar_t *e;					
				long l = wcstol(p,&e,10);

				if(l>=0&&l<1024)
				if(e-p==4&&!wcsicmp(e,L".prt"))				
				{
					return l; //legacy piece
				}
			}
			if(!p[1]&&p==profile) //som_map_append_prt?
			{
				p = longname(); goto p; //2024
			}
			return 1024+number();
		}
		inline size_t number()const
		{
			return this-&SOM_MAP.prt.blob[0];
		}
		inline operator bool()const
		{
			//return *profile;
			return number()<missing();			
		}
	};

	std::vector<key> blob;
		
	//indicates SOM_MAP's profiles have begun loading up
	inline bool loaded(){ return !blob.empty(); }

	size_t missing;

	size_t keys()
	{	
		if(blob.empty()) //feed the blob
		{
			wchar_t query[MAX_PATH] = L"map/*";
			missing = Sompaste->database(0,query,0,0);			
			blob.reserve(missing+1);
			for(size_t i=0;i<missing;i++)
			{
				wcscpy(query,L"map/0")[4]+=i;
				int sep = Sompaste->database(0,query,0,0); 
				blob.push_back(query+sep);
			}		
			if(SOM::tool==MapComp.exe)
			for(size_t i=0;i<missing;i++)
			{
				*blob[i].index2() = missing; //layer support
			}
			blob.push_back(L""); 			
		}
		return missing;
	}	
	template<typename T>
	const key &operator[](T i)
	{
		return blob[(size_t)i>=keys()?missing:i]; 
	}		 
	const key &operator[](const wchar_t *prof)
	{
		if(prof>=operator[](0).profile&&prof<=blob.rbegin()->profile)
		{
			return blob[((char*)prof-(char*)blob[0].profile)/sizeof(key)];
		}	
		std::vector<key>::iterator it = 
		std::lower_bound(blob.begin(),blob.begin()+keys(),prof,predicate());	

		if(it!=blob.end()&&!wcsicmp(it->longname(),prof)) return *it; //find

		return blob[missing];
	}
	inline const key &operator[](wchar_t *prof)
	{
		return operator[]((const wchar_t*)prof);
	}
	struct predicate
	{
		inline bool operator()(const key &a, const key &b) 
		{
			return wcsicmp(a.longname(),b.longname())<0; //compiler
		}
		inline bool operator()(const key &a, const wchar_t *b) 
		{
			return wcsicmp(a.longname(),b)<0;
		}
		inline bool operator()(const wchar_t *a, const key &b) 
		{
			return wcsicmp(a,b.longname())<0;
		}
	};			
};
struct SOM_MAP_map 
{	
	//todo: DON'T memset
	static bool legacy;
	static int current;
	static char versions[64];
	std::vector<char> buffer;
	
	char version; 
	bool finished;
	int error_line; //2022
	int line,tile;	
	int width,height,area;	
	HANDLE in,in2; 	
	BOOL read(LPVOID,DWORD,LPDWORD);
	BOOL write(LPCVOID,DWORD,LPDWORD); 		
	inline operator HANDLE&(){ return in; }

	int rw; enum //monitoring
	{	
	read_only = GENERIC_READ,
	write_only = GENERIC_WRITE,
	read_and_write = 0xC0000000, //monitoring
	write_only2 = GENERIC_WRITE|1, //wrinkle
	};	
	void operator=(HANDLE reset)
	{	
		assert(!reset||!in);	
		assert(rw!=read_and_write);		
		//todo: DON'T memset
		//memset(this,0x00,sizeof(*this)); 		
		line=tile=width=height=area=rw=0;
		error_line = 0;
		finished = false; 
		version = 0;		
		//REMINDER: may be called with 0
		//before buffer is initialized
		if(in=in2=reset) 
		{				
			version = versions[current];
			buffer.clear();
			SOM_MAP.prt.clear_index();
		}
	}
	SOM_MAP_map(){ *this = in = 0; }		
};
struct SOM_MAP_ezt 
{
	BOOL read(LPVOID,DWORD,LPDWORD);
	BOOL write(LPCVOID,DWORD,LPDWORD);

	//2021: "sizeof(this)" fix (may break something)
	SOM_MAP_ezt(){ memset(this,0x00,sizeof(*this)); }
	~SOM_MAP_ezt(){ CloseHandle(ezt); assert(!evt); }
	
	inline char *operator[](int i){ return names[i]; }

	inline operator HANDLE(){ return evt; }

	void operator=(HANDLE v) //using EX::temporary if reading
	{
		//READ is needed for SOM_MAP to launch som_db.exe
		//WRITE will be needed too if the mutex lock ever goes
		const DWORD share = FILE_SHARE_READ;

		if(!ezt) SOM::zentai_init(ezt= 
		CreateFileW(SOM::Tool.project(SOMEX_(B)"\\DATA\\MAP\\sys.ezt"),
		GENERIC_READ|GENERIC_WRITE,share,0,OPEN_ALWAYS,0,0));

		//A harebrain scheme for sure
		if(EX::is_temporary(v?v:evt)) //assuming reading if so
		{
			if(v&&!evt) SOM::zentai_splice(v); else assert(evt&&!v);
		}
		else if(!v&&evt) SOM::zentai_split(evt); else assert(v&&!evt);

		evt = v; buffered = recorded = header = 0;
	}

	void create_evt_for_splicing(const wchar_t *filename)
	{
		HANDLE hack = //som_zentai_init fills out blank files
		CreateFileW(filename,GENERIC_WRITE,0,0,CREATE_NEW,0,0);
		SOM::zentai_init(hack); 
		SOM::zentai_init(ezt); //restore rightful file handle
		CloseHandle(hack);
	}

private: HANDLE ezt, evt; 
		
	enum //interesting bytes	
	{ 
		NAMED=30, //name delimiter 
		CLASS=31, //subject class 
		PROTO=34, //object protocol
	};
	BYTE buffer[252];
	LONG buffered, recorded;	
	CHAR names[1024][31];
	LONG header;
};

template<class T>
struct MapComp_container //MFC?
{
	typedef T value_type;

	T *begin, *end, *capacity;

	void clear(){ begin = capacity; }

	int size(){ return end-begin; }

	T &operator[](int i){ return begin[i]; }	
};
static struct MapComp_43e630
{
	DWORD width, height; //2024
};
static struct MapComp_43e638 //array
{
	//iterate over MSM data
	//004083FC 8B 0D 30 E6 43 00    mov         ecx,dword ptr ds:[43E630h]
	struct mpx
	{
		//00402784 E8 F3 95 00 00       call        0040BD7C
		DWORD textures;

		struct vertex
		{
			DWORD vindex; BYTE bgra[4]; //or rgba?
		};

		struct per_texture //36B
		{
			//004027B5 E8 C2 95 00 00       call        0040BD7C 
			WORD texture;

			WORD unknown1; //nonzero

			DWORD unknown2; //4B? (2) part of the container?
			
			MapComp_container<vertex> vertices; 

			DWORD unknown3; //4B? (2) part of the container?

			MapComp_container<WORD> indices; 

		}*pointer;		
	};
	struct msm //size unknown
	{
		WORD texturesN, *textures;

		WORD vertsN; struct vert //15
		{
			//transformed pos,norm,UV?,pos?,transparency?,color
			FLOAT wpos[3],norm[3],uv[2],pos[3],badf00d,color[3]; 
		}*verts;

		//409570 generates this data (recursively)
		WORD facesN; struct face
		{
			WORD texture, indicesN, *indices;

			//SUBDIVISIONS?
			//tend to be all zero, except final is 0x40000
			//or 0x30000 (maybe SomEx is forcing for do_aa)

			DWORD unknown1[3]; //0,0,0
			DWORD unknown2;    //0
			DWORD unknown3[3]; //0,0,0x40000/0x30000

		}*faces;
	};

	WORD prt,mhm; //msm? what's what???
	
	float elevation; 
	//unsigned rotation:2,box:4,ev:xxx:1,pit:1,:7,icon:8,msb:8;
	unsigned rotation:2,box:4,ev:8,xxx:1,pit:1,icon:8,msb:8;

	DWORD zero_or_8; //0,8 (mostly 0)

	struct msm *msm;
	//populates individual msm pointers
	//NOTE: largely uninteresting... just translates the
	//lighting data into the MPX format
	//00401DCA E8 D1 00 00 00       call        00401EA0
	struct mpx *mpx; //...

	//MSVC2010 won't declare a reference to an array
	MapComp_43e638 &operator[](int i){ return this[i]; }

}&MapComp_43e638 = *(struct MapComp_43e638*)0x43e638; //[100*100]

//2024: these are accessed always with i==0, but maybe
//there is memory here for layers
static struct MapComp_layer: MapComp_43e630 //UNUSED
{
	struct MapComp_43e638 map[100*100];

	MapComp_layer &operator[](int i){ return this[i]; }

}&MapComp_layers = *(struct MapComp_layer*)0x43e630;

#endif //SOM_TOOL_INCLUDED
