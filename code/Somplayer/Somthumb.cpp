
#include "Somplayer.pch.h"

#include <Objbase.h>

//TODO: Vista way
//#include <Thumbcache.h> //IThumbnailProvider

#ifdef _DEBUG
#include <propsys.h>
#endif

#include "../Sompaste/Sompaste.h"
#include "../lib/swordofmoonlight.h" //MDL

#include "Somtexture.h"

static SOMPASTE Sompaste = 0;

#define SOMPLAYER_PREVIEW L"{CE56EE30-20F3-4C7B-911D-4B536A5938E9}"
#define SOMPLAYER_EXPLORE L"{210EF49F-05AF-418A-80DC-84BA28B3953F}"

//Thumbnail provider
static const GUID SOMPlayer_Preview = //4/24/2012
{ 0xce56ee30, 0x20f3, 0x4c7b, { 0x91, 0x1d, 0x4b, 0x53, 0x6a, 0x59, 0x38, 0xe9 } };

//Archive explorer
static const GUID SOMPlayer_Explore = //5/25/2012
{ 0x210ef49f, 0x5af, 0x418a, { 0x80, 0xdc, 0x84, 0xba, 0x28, 0xb3, 0x95, 0x3f } };

static int Somthumb_admin()
{
	static int out = 0;
	static int once = 0; if(once++) return out; 

	HKEY admin = 0; LONG err = 
	RegOpenKeyEx(HKEY_LOCAL_MACHINE,0,0,KEY_ALL_ACCESS,&admin);
	RegCloseKey(admin);

	return out = admin?1:0;
}

static HKEY Somthumb_root(int admin=1)
{
	return Somthumb_admin()&&admin?HKEY_LOCAL_MACHINE:HKEY_CURRENT_USER;
}

extern void Somthumb_clsid(bool repair=false) //extern: Somplayer.cpp (DllMain)
{
	static int once = 0; if(once++&&!repair) return; 

#ifdef _DEBUG

	return; //you might not want to go thru all this everytime you build anew

	SHChangeNotify(SHCNE_ASSOCCHANGED,SHCNF_IDLIST,0,0);

#endif

	wchar_t Somplayer[MAX_PATH] = L"", serving[MAX_PATH] = L"";
	if(!GetModuleFileNameW(Somplayer_dll(),Somplayer,MAX_PATH)) return;
		
	LONG max_path = sizeof(serving), err = RegQueryValueW(Somthumb_root(),
	L"Software\\Classes\\CLSID\\"SOMPLAYER_PREVIEW L"\\InProcServer32",serving,&max_path);

	if(err==ERROR_SUCCESS&&!wcscmp(Somplayer,serving)) return; //Assuming server is running
	
	//FYI: the reason this is not so straight forward is so that the 
	//server copy of the file can be updated without forcing the user
	//to close all instances of Explorer (be inconvenience in any way)
	//
	//PS: the reason this is necessary is because there is no facility
	//to unload the extension and the DLL is write-locked while loaded.

	wchar_t install[MAX_PATH] = L""; //assert(0); //testing

	wchar_t user[MAX_PATH] = L""; DWORD dword = MAX_PATH; GetUserNameW(user,&dword); 

	struct _stat t; t.st_mtime = 0; _wstat(Somplayer,&t); //paranoia...
	if(!swprintf_s(install,L"\\serving\\Somplayer_%s_%X\\Somplayer.dll",user,t.st_mtime)) return;
		
	//fallback position: not good; precludes instant update
	if(!Somfolder(install,0)) wcscpy(install,Somplayer); 

	if(wcscmp(serving,install))
	{	
		//delete any older copies of the server if able...
		{					
			swprintf_s(serving,L"\\serving\\Somplayer_%s_*",user);			
			wchar_t	*slash = wcsrchr(Somfolder(serving,0),'\\'); 	

			//NOTE: A single SHFileOperationW would not do :/
			SHFILEOPSTRUCTW op = {0,FO_DELETE,serving,0,FOF_NO_UI,0,0};
			
			//Despite documentation:
			//the glob above (Somplayer_*) does not find any folders...
			//SHFileOperationW(&op);

			WIN32_FIND_DATAW find; 
			HANDLE glob = FindFirstFileW(serving,&find); 

			if(glob!=INVALID_HANDLE_VALUE) 
			{	
				for(int safety=0;1;safety++)
				{	
					//Annoying: 00 terminate serving...
					wmemset(slash+1,0x00,wcslen(find.cFileName)+2);

					wcscpy(slash+1,find.cFileName); //serving
					int debug = SHFileOperationW(&op);

					if(!FindNextFileW(glob,&find)||safety>12)
					{				
						assert((debug=GetLastError())==ERROR_NO_MORE_FILES); 							
						break;
					}		
				}			  		

				FindClose(glob);
			}
		}
		
		SHPathPrepareForWriteW(0,0,install,SHPPFW_DIRCREATE|SHPPFW_IGNOREFILENAME);

		//REMINDER: the user needs to know that if they clean their disks
		//temporary files they will need to at least run Somplayer.dll to
		//get the Explorer shell extensions functionality to come back on

		CopyFileW(Somplayer,install,1); //TODO: log/perform in background

		wchar_t *slash = wcsrchr(wcscpy(serving,install),'\\'); 

		//// Dependencies ///////////////////////////////////////

		/*//Able to delay load Sompaste.dll for now
		HINSTANCE Sompaste_dll;	wchar_t Sompaste[MAX_PATH] = L""; 

		if(Sompaste_dll=GetModuleHandle("Sompaste.dll"))
		if(GetModuleFileNameW(Sompaste_dll,Sompaste,MAX_PATH)) 
		{
			wcscpy(slash+1,L"Sompaste.dll"); //serving
			CopyFileW(Sompaste,serving,0);
		}*/		

		HINSTANCE Sompaint_dll;	wchar_t Sompaint[MAX_PATH] = L""; 

		//GetModuleHandle: would be nice to delay loading
		if(Sompaint_dll=LoadLibrary("Sompaint_D3D9.dll"))
		{
			if(GetModuleFileNameW(Sompaint_dll,Sompaint,MAX_PATH)) 
			{
				wcscpy(slash+1,L"Sompaint_D3D9.dll"); //serving
				CopyFileW(Sompaint,serving,0);
			}
			FreeLibrary(Sompaint_dll);
		}
	}
	else if(!repair) return; //Assuming up to date
	
	for(int i=0;i<=Somthumb_admin();i++) //gotta be a better way??
	{
		HKEY root = Somthumb_root(i); const wchar_t tm[] = L"Apartment"; //L"Both"
				
		HKEY clsid = 0; LONG err = RegCreateKeyExW(root,
		L"Software\\Classes\\CLSID\\" SOMPLAYER_PREVIEW,0,0,0,KEY_WRITE,0,&clsid,0);

		if(err==ERROR_SUCCESS) //thumbnails CLSID
		{
			const wchar_t tag[] = L"Sword of Moonlight SOMPlayer Preview";

			RegSetValueW(clsid,0,REG_SZ,tag,sizeof(tag));
			RegSetValueW(clsid,L"InProcServer32",REG_SZ,install,sizeof(install));
			SHSetValueW(clsid,L"InProcServer32",L"ThreadingModel",REG_SZ,tm,sizeof(tm));

			//courtesy (Compressed Folder does this)
			RegSetValueW(clsid,L"ProgID",REG_SZ,L"SOMPlayer.Preview",sizeof(L"SOMPlayer.Preview"));
			RegCloseKey(clsid);
		}

		err = RegCreateKeyExW(root,
		L"Software\\Classes\\SOMPlayer.Preview",0,0,0,KEY_WRITE,0,&clsid,0);

		if(err==ERROR_SUCCESS) //thumbnails ProgID
		{
			const wchar_t friendly[] = 
			L"Sword of Moonlight Media Player Image Extractor";   
			RegSetValueW(clsid,0,REG_SZ,friendly,sizeof(friendly));
			RegSetValueW(clsid,L"CLSID",REG_SZ,SOMPLAYER_PREVIEW,sizeof(SOMPLAYER_PREVIEW));

			const wchar_t ShellEx_IExtractImage[] = 
			L"ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}";	 
			RegSetValueW(clsid,ShellEx_IExtractImage,REG_SZ,SOMPLAYER_PREVIEW,sizeof(SOMPLAYER_PREVIEW));
			RegCloseKey(clsid);
		}
		
		err = RegCreateKeyExW(root,
		L"Software\\Classes\\CLSID\\" SOMPLAYER_EXPLORE,0,0,0,KEY_WRITE,0,&clsid,0);

		if(err==ERROR_SUCCESS) //archive CLSID (ie. MDL browser)
		{				
			//copying cabview.dll under XP (could just SHCopyKey)

			const wchar_t tag[] = L"Sword of Moonlight SOMPlayer Explore";			
			
			//Redundant CLSID: necessary?
			RegSetValueW(clsid,L"CLSID",REG_SZ,SOMPLAYER_EXPLORE,sizeof(SOMPLAYER_EXPLORE));

			const DWORD sfgao = //0x200001a0|SFGAO_FILESYSANCESTOR; //testing (zip-like)
			SFGAO_STREAM| //"Some items can be flagged with both SFGAO_STREAM and SFGAO_FOLDER, 
			//such as a compressed file with a .zip file name extension. Some applications might 
			//include this flag when testing for items that are both files and containers."
			SFGAO_STORAGE|SFGAO_FOLDER|SFGAO_FILESYSTEM|SFGAO_FILESYSANCESTOR|  
			//SFGAO_CANDELETE|SFGAO_CANCOPY|SFGAO_CANMOVE|SFGAO_CANRENAME|SFGAO_CANLINK|
			SFGAO_DROPTARGET|SFGAO_BROWSABLE; 
			
			SHSetValueW(clsid, //necessary: "Browsable Shell Extension"
			L"Implemented Categories\\{00021490-0000-0000-C000-000000000046}",0,REG_SZ,0,0);  						   
			
			RegSetValueW(clsid,0,REG_SZ,tag,sizeof(tag));
			RegSetValueW(clsid,L"InProcServer32",REG_SZ,install,sizeof(install));
			SHSetValueW(clsid,L"InProcServer32",L"ThreadingModel",REG_SZ,tm,sizeof(tm));
			SHSetValueW(clsid,L"ShellFolder",L"Attributes",REG_DWORD,(BYTE*)&sfgao,sizeof(DWORD));

			//pretty common; maybe only under XP (may also have to be installed under .ext)
			const wchar_t Null_persistent_handler[] = L"{098F2470-BAE0-11CD-B579-08002B30BFEB}";			
			RegSetValueW(clsid,L"PersistentHandler",REG_SZ,Null_persistent_handler,sizeof(Null_persistent_handler));

			//courtesy (Compressed Folder does this)
			RegSetValueW(clsid,L"ProgID",REG_SZ,L"SOMPlayer.Explore",sizeof(L"SOMPlayer.Explore"));
			RegCloseKey(clsid);
		}

		err = RegCreateKeyExW(root,
		L"Software\\Classes\\SOMPlayer.Explore",0,0,0,KEY_WRITE,0,&clsid,0);		

		if(err==ERROR_SUCCESS) //archives ProgID
		{
			const wchar_t friendly[] = 
			L"Sword of Moonlight Media Player Archive Explorer";   
			RegSetValueW(clsid,0,REG_SZ,friendly,sizeof(friendly));
			RegSetValueW(clsid,L"CLSID",REG_SZ,SOMPLAYER_EXPLORE,sizeof(SOMPLAYER_EXPLORE));

			/*Spparently Vista has Open in new window
			if((GetVersion()&0xFF)>=6) //Vista
			{
			//\"%SystemRoot%\\Explorer.exe\" just doesn't find Explorer!!
			const wchar_t cmd[] = L"Explorer /e, ::{210EF49F-05AF-418A-80DC-84BA28B3953F}, %1";
			RegSetValueW(clsid,L"Shell\\Explore\\command",REG_SZ,cmd,sizeof(cmd));
			}*/

			//pretty common; maybe only under XP (may also have to be installed under .ext)
			const wchar_t Null_persistent_handler[] = L"{098F2470-BAE0-11CD-B579-08002B30BFEB}";			
			RegSetValueW(clsid,L"PersistentHandler",REG_SZ,Null_persistent_handler,sizeof(Null_persistent_handler));
			RegCloseKey(clsid);
		}
	}
}				 

static bool Somthumb_enlarge() 
{
	static bool out = false;

	static DWORD a = 0; DWORD b = GetTickCount();

	DWORD d = b-a; a = b; if(d>1000&&a) return out;

	DWORD dword = 0, dword_s = 4; 	
	SHGetValueW(HKEY_CURRENT_USER,
	L"Software\\Classes\\CLSID\\"SOMPLAYER_PREVIEW,L"EnlargeThumbnails",0,&dword,&dword_s);
	
	if(out!=(bool)dword&&a)	SHChangeNotify(SHCNE_ASSOCCHANGED,SHCNF_IDLIST,0,0);

	return out = dword;
}

class Somthumb  
:
public IPersistFile, public IExtractImage2, public IRunnableTask
{
public:

	//IUnknown
	STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObj)
	{
		if(!ppvObj) return E_INVALIDARG; //paranoia

		*ppvObj = 0;
		if(riid==__uuidof(IExtractImage))
		*ppvObj = static_cast<IExtractImage*>(this);
		else if(riid==__uuidof(IExtractImage2))
		*ppvObj = static_cast<IExtractImage2*>(this);
		else if(riid==__uuidof(IPersistFile))
		*ppvObj = static_cast<IPersistFile*>(this);
		else if(riid==__uuidof(IRunnableTask)) 
		*ppvObj = static_cast<IRunnableTask*>(this);
		if(*ppvObj) AddRef(); 

		return *ppvObj?S_OK:E_NOINTERFACE;
	}
	STDMETHOD_(ULONG,AddRef)()
	{
		return InterlockedIncrement(&refs);
	}
	STDMETHOD_(ULONG,Release)()
	{
		LONG out = InterlockedDecrement(&refs);
		if(out==0) delete this; return out;
	}

	//IExtractImage
	STDMETHOD(GetLocation)(LPWSTR,DWORD,DWORD*,const SIZE*,DWORD,DWORD*);
	STDMETHOD(Extract)(HBITMAP*);

	//IExtractImage2
	STDMETHOD(GetDateStamp)(FILETIME*);

	//IPersist
	STDMETHOD(GetClassID)(CLSID *pClassID)
	{
		if(!pClassID) return E_INVALIDARG; //paranoia
		*pClassID = SOMPlayer_Preview;
		return S_OK;
	}

	//IPersistFile
	STDMETHOD(Load)(LPCOLESTR,DWORD);
	STDMETHOD(IsDirty)(VOID){ assert(0); return E_NOTIMPL; } 
	STDMETHOD(Save)(LPCOLESTR,BOOL){ assert(0); return E_NOTIMPL; }
	STDMETHOD(SaveCompleted)(LPCOLESTR){ assert(0); return E_NOTIMPL; }
	STDMETHOD(GetCurFile)(LPOLESTR*){ assert(0); return E_NOTIMPL; }

	//IRunnableTask
	STDMETHOD(Run)(void){ assert(0); return E_NOTIMPL; }      
    STDMETHOD(Kill)(BOOL){ assert(0); return E_NOTIMPL; }
    STDMETHOD(Suspend)(){ assert(0); return E_NOTIMPL; }
    STDMETHOD(Resume)(){ assert(0); return E_NOTIMPL; }
    STDMETHOD_(ULONG,IsRunning)(){ assert(0); return E_NOTIMPL; }

	ULONG refs; SIZE size;

	wchar_t path[MAX_PATH]; bool enlarge;
			
	static const Somtexture *pool;
	
	class Enough{ /*empty terminator*/ };

	static class Factory : public IClassFactory
	{
	public: 

		ULONG locks, refs, thumbs, folders;

		inline operator bool()
		{
			return locks||refs||thumbs||folders;
		}

		//IUnknown
		STDMETHOD(QueryInterface)(REFIID, LPVOID*p)
		{
			if(p) *p = 0; else return E_INVALIDARG; assert(0); 

			return E_NOINTERFACE; 
		}
		STDMETHOD_(ULONG,AddRef)(){ return InterlockedIncrement(&refs); };
		STDMETHOD_(ULONG,Release)()
		{
			if(refs==0) return 0; //paranoia
			ULONG out = InterlockedDecrement(&refs); 
			return out;
		
		};
		//IClassFactory
		STDMETHOD(CreateInstance)(IUnknown *pUnkOuter, REFIID riid, void **ppvObject)
		{
			if(pUnkOuter) return CLASS_E_NOAGGREGATION; if(!ppvObject) return E_INVALIDARG;
						
			if(riid==__uuidof(IExtractImage))
			{
				Somthumb *out = new Somthumb(); 
				*ppvObject = static_cast<IExtractImage*>(out); return S_OK; 
			}
			else if(riid==__uuidof(IShellFolder))
			{
				Somthumb::Folder *out = new Somthumb::Folder(); 
				*ppvObject = static_cast<IShellFolder*>(out); return S_OK; 
			} 
			else if(riid==__uuidof(IShellFolder2))
			{
				Somthumb::Folder *out = new Somthumb::Folder(); 
				*ppvObject = static_cast<IShellFolder2*>(out); return S_OK; 
			} 

			//// WARNING: Factory is sharing two CLSIDs (verclsid.exe) ////

			/*verclsid.exe on Win XP: seems interested in GetClassID*/
			else if(riid==__uuidof(IPersistFile))
			{
				Somthumb *out = new Somthumb(); 
				*ppvObject = static_cast<IPersistFile*>(out); return S_OK;
			}
			/*verclsid.exe on Win XP: seems interested in GetClassID*/
			else if(riid==__uuidof(IPersist))
			{
				Somthumb::Folder *out = new Somthumb::Folder(); 
				*ppvObject = static_cast<IPersist*>(static_cast<IPersistFolder*>(out)); 
				return S_OK; 
			} 

#ifdef _DEBUG

			else if(riid==__uuidof(IPropertySetStorage)){ return E_NOINTERFACE; }	
			else if(riid==__uuidof(IBrowserFrameOptions))
			{
				return E_NOINTERFACE; //// ATTENTION ////
			//	Somthumb::Folder *out = new Somthumb::Folder(); 
			//	*ppvObject = static_cast<IBrowserFrameOptions*>(out); 				
			//	return S_OK; 
			} 
			//////////////////////////////////////////////////////////////
			
			assert(0); 
#endif			
			return E_NOINTERFACE;
		}        
		STDMETHOD(LockServer)(BOOL lock)
		{
			if(lock) InterlockedIncrement(&locks); else InterlockedDecrement(&locks); 
			
			return S_OK;
		};

		Factory()
		{	
			locks = refs = thumbs = folders = 0; 
		}

	}factory;	 

	Somthumb() : refs(1), enlarge(false)
	{	
		InterlockedIncrement(&factory.thumbs);
		size.cx = size.cy = 0; path[0] = '\0';

		if(!pool) pool = Somtexture::open(0,0);
	}
	~Somthumb()
	{
		InterlockedDecrement(&factory.thumbs);
	}

	class Folder //eg. MDL
	:
	public IPersistFolder2, //3,
	public IPersistIDList,
	//public IShellDetails, //should not need
	//Reminder: a few sources say that 2 must
	//be implemented in order for the default 
	//shell view to be instantiated on one or
	//more versions of Windows' shell runtime
	public IShellFolder2, 
	//public IShellIcon,
	//public IDropTarget,
	//public IShellFolderContextMenuSink,
	//public IPerformedDropEffectSink,
	//public IBrowserFrameOptions,
	public Enough
	{
	public: 
				
		ULONG refs;

		wchar_t path[MAX_PATH], *name;

		ITEMIDLIST *idlist;

		swordofmoonlight_lib_image_t mdl;

		Folder() : refs(1), idlist(0), name(path)
		{	
			path[0] = '\0'; mdl.mode = 0;  

			InterlockedIncrement(&factory.folders);
		}
		~Folder()
		{			
			if(idlist) ILFree(idlist);

			InterlockedDecrement(&factory.folders);

			swordofmoonlight_lib_unmap(&mdl);
		}
	
		//IUnknown
		STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObj)
		{
			if(!ppvObj) return E_INVALIDARG; //paranoia

			*ppvObj = 0;
			if(riid==__uuidof(IUnknown))
			*ppvObj = static_cast<IUnknown*>
					 (static_cast<IPersistFolder*>(this));			
			else if(riid==__uuidof(IPersist))
			*ppvObj = static_cast<IPersist*>
					 (static_cast<IPersistFolder*>(this));			
			else if(riid==__uuidof(IPersistFolder))
			*ppvObj = static_cast<IPersistFolder*>(this);			
			else if(riid==__uuidof(IPersistIDList))
			*ppvObj = static_cast<IPersistIDList*>(this);					
			else if(riid==__uuidof(IShellFolder))
			*ppvObj = static_cast<IShellFolder*>(this);		
			else if(riid==__uuidof(IShellFolder2))
			*ppvObj = static_cast<IShellFolder2*>(this);		
						
#ifdef _DEBUG //surveying (roughly in order of encounter)

			if(riid==__uuidof(IBrowserFrameOptions))
			{
				return E_NOINTERFACE; //// ATTENTION ////
				//*ppvObj = static_cast<IBrowserFrameOptions*>(this);	
			} 

			if(riid==__uuidof(IPersistFolder3))	return E_NOINTERFACE;
			if(riid==__uuidof(IShellIcon))		 return E_NOINTERFACE;
			if(riid==__uuidof(IShellIconOverlay)) return E_NOINTERFACE; 

			//TODO: might want this one
			if(riid==__uuidof(IOleCommandTarget)) return E_NOINTERFACE; 
			//AND: this one too
			if(riid==__uuidof(IShellLinkW)) return E_NOINTERFACE; 

			//undocumented (apparently)
			// {C7264BF0-EDB6-11d1-8546-006008059368}
			static const GUID IID_IPersistFreeThreadedObject = 
			{0xc7264bf0,0xedb6,0x11d1,0x85,0x46,0x0,0x60,0x8,0x5,0x93,0x68};
			if(riid==IID_IPersistFreeThreadedObject) return E_NOINTERFACE; 
		
			//// Post-Vista ////////////////////////////

			if(riid==__uuidof(IObjectWithFolderEnumMode)) return E_NOINTERFACE;
			if(riid==__uuidof(INameSpaceTreeControlFolderCapabilities)) return E_NOINTERFACE;

			static const GUID Unknown0 = //{FDBEE76E-F12B-408E-93AB-9BE8521000D9}
			{0xFDBEE76E, 0xF12B, 0x408E, {0x93, 0xAB, 0x9B, 0xE8, 0x52, 0x10, 0x00, 0xD9}};
			if(riid==Unknown0) return E_NOINTERFACE;

			//IShellFolder3: un (or no longer) documented
			{
				static const GUID IID_IShellFolder3 = //{2EC06C64-1296-4F53-89E5-ECCE4EFC2189}
				//http://www.geoffchappell.com/studies/windows/shell/shell32/interfaces/ishellfolder3.htm
				{0x2EC06C64, 0x1296, 0x4F53, {0x89, 0xE5, 0xEC, 0xCE, 0x4E, 0xFC, 0x21, 0x89}};
				if(riid==IID_IShellFolder3) return E_NOINTERFACE;
			}
			//Alternative version (from my Windows 7 registry)
			{
				static const GUID IID_IShellFolder3 = //{2EC06C64-1296-4F53-89E5-ECCE4EFC2189}
				{0x711B2CFD, 0x93D1, 0x422b, {0xBD, 0xF4, 0x69, 0xBE, 0x92, 0x3F, 0x24, 0x49}};
				if(riid==IID_IShellFolder3) return E_NOINTERFACE;
			}
						
			//docs say system only
			if(riid==__uuidof(IShellItem)) return E_NOINTERFACE;
			if(riid==__uuidof(IParentAndItem)) return E_NOINTERFACE;
			if(riid==__uuidof(ICurrentItem)) return E_NOINTERFACE; 

			if(riid==__uuidof(IFolderView)) return E_NOINTERFACE; //customization

			static const GUID IID_IFolderProperties = //{7361EE29-5BAD-459D-A9F5-F655068982F0}
			//http://www.geoffchappell.com/studies/windows/shell/shell32/interfaces/ifolderproperties.htm
			{0x7361EE29, 0x5BAD, 0x459D, {0xA9, 0xF5, 0xF6, 0x55, 0x06, 0x89, 0x82, 0xF0}};
			if(riid==IID_IFolderProperties) return E_NOINTERFACE;
			static const GUID IID_IFolderType = //{053B4A86-0DC9-40A3-B7ED-BC6A2E951F48}
			//http://www.geoffchappell.com/studies/windows/shell/shell32/interfaces/ifoldertype.htm
			{0x053B4A86, 0x0DC9, 0x40A3, {0xB7, 0xED, 0xBC, 0x6A, 0x2E, 0x95, 0x1F, 0x48}};
			if(riid==IID_IFolderType) return E_NOINTERFACE;

			//Taken from Windows 7 registry
			static const GUID IID_IObjectWithGITCookie = //{527832F6-5FB1-414D-86CC-5BC1DA0F4ED9}
			{0x527832F6, 0x5FB1, 0x414D, {0x86, 0xCC, 0x5B, 0xC1, 0xDA, 0x0F, 0x4E, 0xD9}};
			if(riid==IID_IObjectWithGITCookie) return E_NOINTERFACE;
			static const GUID IID_INavigationRelatedItem = //{51737562-4F12-4B7A-A3A0-6E9EFE465E1E}
			{0x51737562, 0x4F12, 0x4B7A, {0xA3, 0xA0, 0x6E, 0x9E, 0xFE, 0x46, 0x5E, 0x1E}};
			if(riid==IID_INavigationRelatedItem) return E_NOINTERFACE;
			static const GUID IID_IViewResultRelatedItem = //{50BC72DA-9633-47CB-80AC-727661FB9B9F}
			{0x50BC72DA, 0x9633, 0x47CB, {0x80, 0xAC, 0x72, 0x76, 0x61, 0xFB, 0x9B, 0x9F}};
			if(riid==IID_IViewResultRelatedItem) return E_NOINTERFACE;
			static const GUID IID_IShellFolderPropertyInformation = //{124BAE2C-CB94-42CD-B5B8-4358789684EF}
			{0x124BAE2C, 0xCB94, 0x42CD, {0xB5, 0xB8, 0x43, 0x58, 0x78, 0x96, 0x84, 0xEF}};
			if(riid==IID_IShellFolderPropertyInformation) return E_NOINTERFACE;
			static const GUID IID_IInfoPaneProvider = //{38698B65-1CA7-458C-B4D6-E0A51379C1D2}
			{0x38698B65, 0x1CA7, 0x458C, {0xB4, 0xD6, 0xE0, 0xA5, 0x13, 0x79, 0xC1, 0xD2}};
			if(riid==IID_IInfoPaneProvider) return E_NOINTERFACE;

			//{E07010EC-BC17-44C0-97B0-46C7C95B9EDC}
			if(riid==__uuidof(IExplorerPaneVisibility)) return E_NOINTERFACE;

			//Taken from Windows 7 registry
			static const GUID IID_IRelocateFolderInNamespace = //{CD9617E6-E67F-4F7B-8B64-11B05F507868}
			{0xCD9617E6, 0xE67F, 0x4F7B, {0x8B, 0x64, 0x11, 0xB0, 0x5F, 0x50, 0x78, 0x68}};
			if(riid==IID_IRelocateFolderInNamespace) return E_NOINTERFACE;

			if(riid==__uuidof(IThumbnailHandlerFactory)) return E_NOINTERFACE;
			
			//Taken from Windows 7 registry
			static const GUID IID_ISearchBoxSettings = //{9838AAB6-32FD-455A-823D-83CFE06E4D48}
			{0x9838AAB6, 0x32FD, 0x455A, {0x82, 0x3D, 0x83, 0xCF, 0xE0, 0x6E, 0x4D, 0x48}};
			if(riid==IID_ISearchBoxSettings) return E_NOINTERFACE;
			static const GUID IID_IShellSearchTargetServices = //{DDA3A58A-43DA-4A43-A5F2-F7ABF6E3C026}
			{0xDDA3A58A, 0x43DA, 0x4A43, {0xA5, 0xF2, 0xF7, 0xAB, 0xF6, 0xE3, 0xC0, 0x26}};
			if(riid==IID_IShellSearchTargetServices) return E_NOINTERFACE;
			
			if(riid==__uuidof(IObjectProvider)) return E_NOINTERFACE; 
			
			//Taken from Windows 7 registry
			static const GUID IID_IContextMenuFactory = //{47D9E2B2-CBB3-4FE3-A925-F49978685982}
			{0x47D9E2B2, 0xCBB3, 0x4FE3, {0xA9, 0x25, 0xF4, 0x99, 0x78, 0x68, 0x59, 0x82}};
			if(riid==IID_IContextMenuFactory) return E_NOINTERFACE;

			if(riid==__uuidof(IObjectWithSite)) return E_NOINTERFACE; 
			if(riid==__uuidof(IInternetSecurityManager)) return E_NOINTERFACE;

			//follows IInternetSecurityManager in two places 
			static const GUID Unknown1 = //{924502A7-CC8E-4F60-AE1F-F70C0A2B7A7C}
			{0x924502A7, 0xCC8E, 0x4F60, {0xAE, 0x1F, 0xF7, 0x0C, 0x0A, 0x2B, 0x7A, 0x7C}};
			if(riid==Unknown1) return E_NOINTERFACE;			

			static const GUID Unknown2 = //{BE9DA82B-CC54-4B19-8C22-AD7762FF29EB}
			{0xBE9DA82B, 0xCC54, 0x4B19, {0x8C, 0x22, 0xAD, 0x77, 0x62, 0xFF, 0x29, 0xEB}};
			if(riid==Unknown2) return E_NOINTERFACE;			
			static const GUID Unknown3 = //{2F711B17-773C-41D4-93FA-7F23EDCECB66}
			{0x2F711B17, 0x773C, 0x41D4, {0x93, 0xFA, 0x7F, 0x23, 0xED, 0xCE, 0xCB, 0x66}};
			if(riid==Unknown3) return E_NOINTERFACE;			

			//Taken from Windows 7 registry
			static const GUID IID_ICacheableObject = //{771E5917-5788-4A36-A276-B0DDBF8E4ABF}
			{0x771E5917, 0x5788, 0x4A36, {0xA2, 0x76, 0xB0, 0xDD, 0xBF, 0x8E, 0x4A, 0xBF}};
			if(riid==IID_ICacheableObject) return E_NOINTERFACE;

			if(!*ppvObj) //assert(*ppvObj); 
			{
				wchar_t sticking[64] = L""; StringFromGUID2(riid,sticking,64);
				MessageBoxW(0,sticking,__WFILE__ L", ln:"__WLINE__,MB_RETRYCANCEL);
				assert(0);
			}			
#endif
			if(*ppvObj) AddRef(); 

			return *ppvObj?S_OK:E_NOINTERFACE;
		}
		STDMETHOD_(ULONG,AddRef)()
		{
			return InterlockedIncrement(&refs); 
		}
		STDMETHOD_(ULONG,Release)()
		{				
			ULONG out = InterlockedDecrement(&refs); 
			if(out==0) delete this;
			return out;		
		}
		
		//IPersist
		STDMETHOD(GetClassID)(CLSID *pClassID)
		{
			if(!pClassID) return E_INVALIDARG; //paranoia
			*pClassID = SOMPlayer_Explore;
			return S_OK;
		}

		//IPersistFolder
		STDMETHOD(Initialize)(PCIDLIST_ABSOLUTE);

		//IPersistFolder2
		STDMETHOD(GetCurFolder)(PIDLIST_ABSOLUTE*);

		//IPersistIDList 
		STDMETHOD(SetIDList)(PCIDLIST_ABSOLUTE);         
        STDMETHOD(GetIDList)(PIDLIST_ABSOLUTE*);

		//IShellFolder
		STDMETHOD(ParseDisplayName)(HWND,IBindCtx*,OLECHAR*,ULONG*,ITEMIDLIST**,ULONG*);
		STDMETHOD(EnumObjects)(HWND,DWORD,IEnumIDList**);
		STDMETHOD(BindToObject)(const ITEMIDLIST*,IBindCtx*,REFIID,void**);
		STDMETHOD(BindToStorage)(const ITEMIDLIST*,IBindCtx*,REFIID,void**)
		{ assert(0); return E_NOTIMPL; }
		STDMETHOD(CompareIDs)(LPARAM,const ITEMIDLIST*,const ITEMIDLIST*);
		STDMETHOD(CreateViewObject)(HWND,REFIID,void**);
		STDMETHOD(GetAttributesOf)(UINT,const ITEMIDLIST**,SFGAOF*);
		STDMETHOD(GetUIObjectOf)(HWND,UINT,const ITEMIDLIST**,REFIID,UINT*,void**);
		STDMETHOD(GetDisplayNameOf)(const ITEMIDLIST*,DWORD,STRRET*);
		STDMETHOD(SetNameOf)(HWND,const ITEMIDLIST*,const OLECHAR*,DWORD,ITEMIDLIST**);

		//IShellFolder2
		STDMETHOD(GetDefaultSearchGUID)(GUID*){ return E_NOTIMPL; }
        STDMETHOD(EnumSearches)(IEnumExtraSearch**){ return E_NOTIMPL; }
        STDMETHOD(GetDefaultColumn)(DWORD,ULONG*,ULONG*);        
        STDMETHOD(GetDefaultColumnState)(UINT,SHCOLSTATEF*);
		STDMETHOD(GetDetailsEx)(PCUITEMID_CHILD,const SHCOLUMNID*,VARIANT*);
		STDMETHOD(GetDetailsOf)(PCUITEMID_CHILD,UINT,SHELLDETAILS*);
		STDMETHOD(MapColumnToSCID)(UINT,SHCOLUMNID*);

		//IBrowserFrameOptions (maybe unnecessary)
		//STDMETHOD(GetFrameOptions)(BROWSERFRAMEOPTIONS dwMask,BROWSERFRAMEOPTIONS *pdwOptions)
		//{
		//	if(!pdwOptions)	return E_INVALIDARG; *pdwOptions = dwMask&BFO_NONE; return S_OK;
		//}

		class Enum : public IEnumIDList
		{
		public:	ULONG refs; int pos;

			Somthumb::Folder *folder;

			Enum(Somthumb::Folder *f) : refs(1)
			{ 
				folder = f; f->AddRef(); pos = -1; //name
			}
			~Enum()
			{
				folder->Release();
			}
		
			//IUnknown
			STDMETHOD(QueryInterface)(REFIID riid,LPVOID*p)
			{					
				if(p) *p = 0; else return E_INVALIDARG;				
#ifdef _DEBUG
				//going out on a limb and assuming unnecessary
				if(riid==__uuidof(IObjectWithSite)) return E_NOINTERFACE;

				//extension is strictly offline, so...
				if(riid==__uuidof(IInternetSecurityManager)) return E_NOINTERFACE;

				//follows IInternetSecurityManager in two places 
				static const GUID Unknown1 = //{924502A7-CC8E-4F60-AE1F-F70C0A2B7A7C}
				{0x924502A7, 0xCC8E, 0x4F60, {0xAE, 0x1F, 0xF7, 0x0C, 0x0A, 0x2B, 0x7A, 0x7C}};
				if(riid==Unknown1) return E_NOINTERFACE;

				assert(0); 
#endif				
				return E_NOINTERFACE; 		
			}
			STDMETHOD_(ULONG,AddRef)()
			{
				return InterlockedIncrement(&refs); 
			}
			STDMETHOD_(ULONG,Release)()
			{				
				ULONG out = InterlockedDecrement(&refs); 
				if(out==0) delete this;	return out;		
			}

			//IEnumIDList
			STDMETHOD(Next)(ULONG,PITEMID_CHILD*,ULONG*);
			STDMETHOD(Skip)(ULONG);
			STDMETHOD(Reset)(void);        
			STDMETHOD(Clone)(IEnumIDList**);
		};

		class View
		: 
		public IDataObject,
		public IDropTarget, 
		public Enough
		{
		public:	ULONG refs;

			Somthumb::Folder *folder;

			View(Somthumb::Folder *f) : refs(1)
			{ 
				folder = f; f->AddRef();
			}
			~View()
			{
				folder->Release(); 
			}
		
			//IUnknown
			STDMETHOD(QueryInterface)(REFIID riid,LPVOID*p)
			{					
				if(p) *p = 0; else return E_INVALIDARG; assert(0); 

				return E_NOINTERFACE; 		
			}
			STDMETHOD_(ULONG,AddRef)()
			{
				return InterlockedIncrement(&refs); 
			}
			STDMETHOD_(ULONG,Release)()
			{				
				ULONG out = InterlockedDecrement(&refs); 
				if(out==0) delete this;	return out;		
			}

			//IDataObject
			STDMETHOD(GetData)(FORMATETC*,STGMEDIUM*);        
			STDMETHOD(GetDataHere)(FORMATETC*,STGMEDIUM*);
			STDMETHOD(QueryGetData)(FORMATETC*);
			STDMETHOD(GetCanonicalFormatEtc)(FORMATETC*,FORMATETC*);
			STDMETHOD(SetData)(FORMATETC*,STGMEDIUM*,BOOL);
			STDMETHOD(EnumFormatEtc)(DWORD,IEnumFORMATETC**);
			STDMETHOD(DAdvise)(FORMATETC*,DWORD,IAdviseSink*,DWORD*);
			STDMETHOD(DUnadvise)(DWORD);
			STDMETHOD(EnumDAdvise)(IEnumSTATDATA**);

			//IDropTarget
			STDMETHOD(DragEnter)(IDataObject*,DWORD,POINTL,DWORD*);        
			STDMETHOD(DragOver)(DWORD,POINTL,DWORD*);        
			STDMETHOD(DragLeave)(void);
			STDMETHOD(Drop)(IDataObject*,DWORD,POINTL,DWORD*);
		};

		class Menu //unused
		: 
		public IContextMenu3,
		public Enough
		{
		public:	ULONG refs;

			IContextMenu *passthru;
			IContextMenu2 *passthru2;
			IContextMenu3 *passthru3;

			Somthumb::Folder *folder; ITEMIDLIST *idlists[1];

			//Release p/passthru (unless you need to keep it around)
			Menu(Somthumb::Folder *f, const ITEMIDLIST *cp, IContextMenu *p) : refs(1)
			{ 
				folder = f; f->AddRef(); idlists[0] = ILClone(cp); 
				
				passthru = p; passthru2 = 0; passthru3 = 0; if(p) p->AddRef(); else return;				

				p->QueryInterface(__uuidof(IContextMenu2),(void**)&passthru2);
				p->QueryInterface(__uuidof(IContextMenu3),(void**)&passthru3);				
			}
			~Menu()
			{
				folder->Release(); ILFree(idlists[0]); if(!passthru) return;
				
				passthru->Release(); if(passthru2)passthru2->Release();
				
				if(passthru3) passthru3->Release();
			}
		
			//IUnknown
			STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObj)
			{					
				if(!ppvObj) return E_INVALIDARG; //paranoia

				*ppvObj = 0;
				if(riid==__uuidof(IUnknown))
				*ppvObj = static_cast<IUnknown*>
						 (static_cast<IContextMenu*>(this));			
				else if(riid==__uuidof(IContextMenu))
				*ppvObj = static_cast<IContextMenu*>(this);	
				else if(riid==__uuidof(IContextMenu2))
				*ppvObj = static_cast<IContextMenu2*>(this);			
				else if(riid==__uuidof(IContextMenu3))
				*ppvObj = static_cast<IContextMenu3*>(this);			
#ifdef _DEBUG
				if(riid==__uuidof(IObjectWithSite)) return E_NOINTERFACE;
				if(riid==__uuidof(IInternetSecurityManager)) return E_NOINTERFACE;
				if(riid==__uuidof(IObjectWithSelection)) return E_NOINTERFACE;

				//follows IInternetSecurityManager in two places 
				static const GUID Unknown1 = //{924502A7-CC8E-4F60-AE1F-F70C0A2B7A7C}
				{0x924502A7, 0xCC8E, 0x4F60, {0xAE, 0x1F, 0xF7, 0x0C, 0x0A, 0x2B, 0x7A, 0x7C}};
				if(riid==Unknown1) return E_NOINTERFACE;			

				assert(*ppvObj); 
#endif
				if(*ppvObj) AddRef(); 

				if(!*ppvObj&&passthru) passthru->QueryInterface(riid,ppvObj);

				return *ppvObj?S_OK:E_NOINTERFACE;
			}
			STDMETHOD_(ULONG,AddRef)()
			{
				return InterlockedIncrement(&refs); 
			}
			STDMETHOD_(ULONG,Release)()
			{				
				ULONG out = InterlockedDecrement(&refs); 
				if(out==0) delete this;	return out;		
			}

			//IContextMenu
			STDMETHOD(QueryContextMenu)(HMENU,UINT,UINT,UINT,UINT);        
			STDMETHOD(InvokeCommand)(CMINVOKECOMMANDINFO*);
			STDMETHOD(GetCommandString)(UINT_PTR,UINT,UINT*,LPSTR,UINT);

			//IContextMenu2
			STDMETHOD(HandleMenuMsg)(UINT uMsg,WPARAM wParam,LPARAM lParam)
			{
				if(passthru2) return passthru2->HandleMenuMsg(uMsg,wParam,lParam); 
				return E_NOTIMPL;
			}

			//IContextMenu3
			STDMETHOD(HandleMenuMsg2)(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
			{
				if(passthru3) return passthru3->HandleMenuMsg2(uMsg,wParam,lParam,plResult); 
				return E_NOTIMPL;
			}
        
			//http://support.microsoft.com/kb/179911/r
			enum{ IDM_EXPLORE=0, IDM_OPEN, IDM_LAST=IDM_OPEN};
		};
	};
};

const Somtexture *Somthumb::pool = 0;

Somthumb::Factory Somthumb::factory;

HRESULT Somthumb::GetLocation(LPWSTR pszPathBuffer,DWORD cchMax, DWORD *pdwPriority,const SIZE *prgSize, DWORD dwRecClrDepth, DWORD *pdwFlags)
{
	//*pdwFlags &= ~IEIFLAG_ASYNC;
	// if (*pdwFlags & IEIFLAG_ASYNC)  MessageBox(0,"async",0,0);
	// if (*pdwFlags & IEIFLAG_ASPECT)  MessageBox(0,"aspect",0,0);
	//if (*pdwFlags & IEIFLAG_SCREEN)  MessageBox(0,"screen",0,0);

	//*pdwFlags &= IEIFLAG_REFRESH;

	if(pszPathBuffer&&cchMax) 
	{
		//docs are vague (at best) on this???
		size_t end = std::min<DWORD>(MAX_PATH-1,cchMax-1);
		wcsncpy(pszPathBuffer,path,end); pszPathBuffer[end] = '\0';
	}
			
	if(prgSize) size = *prgSize; 
		
	if(*pdwFlags&(IEIFLAG_ORIGSIZE|IEIFLAG_ASPECT))
	{
		enlarge = Somthumb_enlarge();
	}
	else enlarge = false;

	if(*pdwFlags&IEIFLAG_ASYNC)	return E_PENDING; 

	return NOERROR;
}
HRESULT Somthumb::Load(LPCOLESTR wszFile, DWORD dwMode)
{
	wcscpy(path,wszFile); //MessageBox(0,m_szFile,0,0);

	return S_OK;	
}
HRESULT Somthumb::Extract(HBITMAP *bitmap)
{
	if(!bitmap) return E_INVALIDARG; *bitmap = 0;

	//testing
	//*bitmap = LoadBitmap(Somplayer_dll(),MAKEINTRESOURCE(IDB_THUMBNAIL));
	//return NOERROR;

	const Somtexture *tmp = Somthumb::pool->open(path); 

	*bitmap = tmp->thumb(size.cx,size.cy,enlarge); tmp->release();

	if(!*bitmap) *bitmap = 
	LoadBitmap(Somplayer_dll(),MAKEINTRESOURCE(IDB_THUMBNAIL)); 

	return *bitmap?NOERROR:E_UNEXPECTED; 
}
HRESULT Somthumb::GetDateStamp(FILETIME *pDateStamp)
{
	FILETIME ftCreationTime,ftLastAccessTime,ftLastWriteTime;

	// open the file and get last write time
	HANDLE f = CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);

	if(!f) return E_FAIL;
	GetFileTime(f,&ftCreationTime,&ftLastAccessTime,&ftLastWriteTime);

	CloseHandle(f);	*pDateStamp = ftLastWriteTime;

	return NOERROR; 
}

HRESULT Somthumb::Folder::Initialize(PCIDLIST_ABSOLUTE pidl)
{
	if(!pidl) return E_INVALIDARG;

	IShellFolder *pDesktop = 0, *pFolder = 0; 

	::SHGetDesktopFolder(&pDesktop); if(!pDesktop) return E_UNEXPECTED;		
		
	STRRET temp = {STRRET_WSTR}; temp.uOffset = 0; //yikes...	
	if(pDesktop->GetDisplayNameOf(pidl,SHGDN_FORPARSING,&temp)==S_OK)
	{
		if(StrRetToBufW(&temp,pidl,path,MAX_PATH)!=S_OK) assert(0);
	}
	else assert(0);	pDesktop->Release(); assert(!idlist);

	if(idlist) ILFree(idlist); idlist = ILClone(pidl);

	name = PathFindFileNameW(path);

	return S_OK;
}
HRESULT Somthumb::Folder::GetCurFolder(PIDLIST_ABSOLUTE *ppidl)
{
	if(!ppidl) return E_INVALIDARG; if(!idlist) return E_ACCESSDENIED;
	
	*ppidl = ILClone(idlist); return S_OK;
}
HRESULT Somthumb::Folder::SetIDList(PCIDLIST_ABSOLUTE pidl)
{
	if(idlist) ILFree(idlist); idlist = 0;

	return Initialize(pidl);
}
HRESULT Somthumb::Folder::GetIDList(PIDLIST_ABSOLUTE *ppidl)
{		
	return GetCurFolder(ppidl);
}
HRESULT Somthumb::Folder::ParseDisplayName(HWND hwnd, IBindCtx* pbc, OLECHAR* pszName, ULONG* pchEaten, ITEMIDLIST** ppidl, ULONG* pdwAttributes)
{
	if(!pszName||!ppidl) return E_INVALIDARG; 
	
	int n = -1; //name

	if(!wcscmp(pszName,name))
	{
		n = 0; //_naked.mdl

		if(!wcsnicmp(pszName,L"_naked.mdl",11)) 
		{
			if(pszName[0]=='_') 
			{
				n = pszName[1]-'0'; //_n.tim

				if(pszName[1]>='1'&&pszName[1]<='9'&&pszName[2]=='.')
				{
					if(wcsnicmp(pszName+3,L"tim",4)) return E_ACCESSDENIED;	
				}
				else return E_ACCESSDENIED;
			}
			else return E_ACCESSDENIED;

			if(pchEaten) *pchEaten = 6;
		}
		else if(pchEaten) *pchEaten = 10;
	}
	else if(pchEaten) *pchEaten = wcslen(name);	
		
	SHITEMID ord[2] = {{sizeof(SHITEMID),n},{0,0}}; 

	if(pdwAttributes) GetAttributesOf(1,(LPCITEMIDLIST*)&ord,pdwAttributes);

	*ppidl = (ITEMIDLIST*)CoTaskMemAlloc(sizeof(ord));
	memcpy(*ppidl,&ord,sizeof(ord));	

	return S_OK;
}
HRESULT Somthumb::Folder::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList** ppenmIDList)
{
	if(!ppenmIDList) return E_INVALIDARG;

	SWORDOFMOONLIGHT::mdl::unmap(mdl);
	SWORDOFMOONLIGHT::mdl::maptofile(mdl,path,'r',0x1); //header only
	
	if(!mdl) return E_ACCESSDENIED; //Assuming Initialized

	Somthumb::Folder::Enum *out = new Somthumb::Folder::Enum(this);

	*ppenmIDList = static_cast<IEnumIDList*>(out); 

	return S_OK;
}
HRESULT Somthumb::Folder::BindToObject(const ITEMIDLIST* pidl, IBindCtx* pbc, REFIID riid, void** ppvObj)
{
	if(riid==__uuidof(IShellFolder)) return E_NOINTERFACE; //looking for subfolders

#ifdef _DEBUG

	//XP: internet down (could not lookup)
	//{10DF43C8-1DBE-11D3-8B34-006097DF5BD4}

	//// Post-Vista //////////////////////////////

	//propsys.h
	if(riid==__uuidof(IPropertyStoreFactory)) return E_NOINTERFACE; //looking for subfolders
	if(riid==__uuidof(IPropertyStore)) return E_NOINTERFACE; //looking for subfolders
	if(riid==__uuidof(IPropertyStoreCache)) return E_NOINTERFACE; //looking for subfolders

	if(riid==__uuidof(IPreviewItem)) return E_NOINTERFACE;
	if(riid==__uuidof(IIdentityName)) return E_NOINTERFACE;

	//Taken from Windows 7 registry
	static const GUID IID_ILibraryDescription = //{86187C37-E662-4D1E-A122-7478676D7E6E}
	{0x86187C37, 0xE662, 0x4D1E, {0xA1, 0x22, 0x74, 0x78, 0x67, 0x6D, 0x7E, 0x6E}};
	if(riid==IID_ILibraryDescription) return E_NOINTERFACE;

	static const GUID Unknown1 = //{D774A009-5A9A-4B52-80B9-DAA2DEAF9BB3}
	{0xD774A009, 0x5A9A, 0x4B52, {0x80, 0xB9, 0xDA, 0xA2, 0xDE, 0xAF, 0x9B, 0xB3}};
	if(riid==Unknown1) return E_NOINTERFACE;

	if(riid==__uuidof(ICurrentItem)) return E_NOINTERFACE;

	assert(0);

#endif

	return E_NOINTERFACE; //unimplemented
}
HRESULT Somthumb::Folder::CompareIDs(LPARAM lParam, const ITEMIDLIST* pidl1, const ITEMIDLIST* pidl2)
{
	if(!pidl1||!pidl2) return E_INVALIDARG; 

	LPCSHITEMID p = &pidl1->mkid, q = &pidl2->mkid;

	int n = *p->abID==255?-1:*p->abID; //255: name

	if(p[0].cb!=3||n>9/*||p[1].cb*/){ assert(0); return E_UNEXPECTED; }
	if(q[0].cb!=3||n>9/*||q[1].cb*/){ assert(0); return E_UNEXPECTED; }

	return int(*p->abID)-int(*q->abID);
}
HRESULT Somthumb::Folder::CreateViewObject(HWND hwndOwner, REFIID riid, void** ppvObj)
{
	if(!ppvObj) return E_INVALIDARG;

	*ppvObj = 0;

	if(riid==__uuidof(IDropTarget))
	{
		//return E_NOTIMPL; //// ATTENTION ////

		Somthumb::Folder::View *out = new Somthumb::Folder::View(this);

		*ppvObj = static_cast<IDropTarget*>(out);
	}
	else if(riid==__uuidof(IShellView))
	{
		SFV_CREATE sc = {sizeof(SFV_CREATE),0,0,0};

		sc.pshf = static_cast<IShellFolder*>(this);

		return SHCreateShellFolderView(&sc,(IShellView**)ppvObj);
	}
	
#ifdef _DEBUG //surveying

	if(riid==__uuidof(ICategoryProvider))
	{
		return E_NOINTERFACE; //docs don't explain what this is even for!!
	}
	else if(riid==__uuidof(IShellDetails))
	{
		return E_NOINTERFACE; //for pre Windows 2000 support 
	}	
	else if(riid==__uuidof(IContextMenu))
	{
		return E_NOINTERFACE; //a menu will be made for us :)
	}

	//// Post-Vista ///////////////////////////
			
	static const GUID IID_ITopViewAwareItem =
	{0x8279FEB8, 0x5CA4, 0x45C4, {0xBE, 0x27, 0x77, 0x0D, 0xCD, 0xEA, 0x1D, 0xEB}};
	if(riid==IID_ITopViewAwareItem) //looks totally obscure
	{
		return E_NOINTERFACE; //maybe not even Microsoft??
	}

	static const GUID Unknown0 = //{CAD9AE9F-56E2-40F1-AFB6-3813E320DCFD}
	{0xCAD9AE9F, 0x56E2, 0x40F1, {0xAF, 0xB6, 0x38, 0x13, 0xE3, 0x20, 0xDC, 0xFD}};
	if(riid==Unknown0) return E_NOINTERFACE;
	
	static const GUID IID_IConnectionFactory =
	{0X6FE2B64C, 0X5012, 0X4B88, {0XBB, 0X9D, 0X7C, 0XE4, 0XF4, 0X5E, 0X37, 0X51}};
	if(riid==IID_IConnectionFactory) return E_NOINTERFACE;

	static const GUID IID_IFrameLayoutDefinition = //{176C11B1-4302-4164-8430-D5A9F0EEACDB}
	//http://www.geoffchappell.com/studies/windows/shell/shell32/interfaces/iframelayoutdefinition.htm?ts=0,322
	{0x176C11B1, 0x4302, 0x4164, {0x84, 0x30, 0xD5, 0xA9, 0xF0, 0xEE, 0xAC, 0xDB}};
	if(riid==IID_IFrameLayoutDefinition) return E_NOINTERFACE;
	
	static const GUID Unknown1 = //{93F81976-6A0D-42C3-94DD-AA258A155470}
	{0x93F81976, 0x6A0D, 0x42C3, {0x94, 0xDD, 0xAA, 0x25, 0x8A, 0x15, 0x54, 0x70}};
	if(riid==Unknown1) return E_NOINTERFACE;

	//Taken from Windows 7 registry
	static const GUID IID_IItemSetOperations = //{32AE3A1F-D90E-4417-9DD9-23B0DFA4621D}
	{0x32AE3A1F, 0xD90E, 0x4417, {0x9D, 0xD9, 0x23, 0xB0, 0xDF, 0xA4, 0x62, 0x1D}};
	if(riid==IID_IItemSetOperations) return E_NOINTERFACE;
	static const GUID IID_IFrameLayoutDefinitionFactory = //{7E734121-F3B4-45F9-AD43-2FBE39E533E2}
	{0x7E734121, 0xF3B4, 0x45F9, {0xAD, 0x43, 0x2F, 0xBE, 0x39, 0xE5, 0x33, 0xE2}};
	if(riid==IID_IFrameLayoutDefinitionFactory) return E_NOINTERFACE;

	if(riid==__uuidof(IExplorerCommandProvider)) return E_NOINTERFACE;

	//operates upon IShellItem
	if(riid==__uuidof(ITransferSource)) return E_NOINTERFACE; 
	if(riid==__uuidof(ITransferDestination)) return E_NOINTERFACE; 
		
	static const GUID IID_INewItemAdvisor = //{24D16EE5-10F5-4DE3-8766-D23779BA7A6D}
	//http://www.geoffchappell.com/studies/windows/shell/shell32/interfaces/inewitemadvisor.htm
	{0x24D16EE5, 0x10F5, 0x4DE3, {0x87, 0x66, 0xD2, 0x37, 0x79, 0xBA, 0x7A, 0x6D}};
	if(riid==IID_INewItemAdvisor) return E_NOINTERFACE;

	assert(*ppvObj); //unimplemented

#endif

	return *ppvObj?S_OK:E_NOINTERFACE; 
}
HRESULT Somthumb::Folder::GetAttributesOf(UINT cidl, const ITEMIDLIST** ppidl, SFGAOF* prgfInOut)
{
	if(!ppidl||!prgfInOut) return E_INVALIDARG;

	//Not allowing SFGAO_CANMOVE|SFGAO_CANRENAME|SFGAO_CANLINK|SFGAO_DROPTARGET
	const DWORD sfgao = SFGAO_STREAM|SFGAO_FILESYSTEM|SFGAO_CANDELETE|SFGAO_CANCOPY; //|SFGAO_STORAGE
							
	for(UINT i=0;i<cidl;i++) prgfInOut[i] = sfgao; return S_OK;
}
static HRESULT CALLBACK Somthumb_CDefFolderMenu_Create2_cb(IShellFolder*, HWND, IDataObject*, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case DFM_MERGECONTEXTMENU: return S_OK;	
	case DFM_INVOKECOMMAND: return S_FALSE;

	default: return E_NOTIMPL;
	}
} 
HRESULT Somthumb::Folder::GetUIObjectOf(HWND hwndOwner, UINT cidl, const ITEMIDLIST** ppidl, REFIID riid, UINT* prgfInOut, void** ppvObj)
{
	if(prgfInOut||!ppvObj) return E_INVALIDARG;
								 
	if(riid==__uuidof(IContextMenu))
	{	
		const int keys_s = 4;
		//Assuming the menu is responsible for closing these
		HKEY all = 0, ext = 0, sfa = 0, cur = 0; //todo: perceived type?
		RegOpenKey(HKEY_CLASSES_ROOT,"*",&all);
		
		int mdl = 0, tim = 0;
		for(int i=0;i<cidl;i++)	if(*ppidl[i]->mkid.abID+1<=1) mdl++; else tim++;
		if((!mdl||!tim)&&(mdl||tim)) RegOpenKey(HKEY_CLASSES_ROOT,mdl?".mdl":".tim",&ext);

		HKEY tmp = 0; LONG err = 
		RegOpenKey(HKEY_CLASSES_ROOT,"SystemFileAssociations",&tmp);
		if(tmp&&!err) RegOpenKey(tmp,mdl?".mdl":".tim",&sfa); 
		RegCloseKey(tmp);

		err = //the users "Open with" preference
		RegOpenKey(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts",&tmp);
		if(tmp&&!err) RegOpenKey(tmp,mdl?".mdl":".tim",&cur); 
		RegCloseKey(tmp);

		HKEY keys[keys_s] = {all, ext, sfa, cur};

		HRESULT debug = 0;
		HRESULT (WINAPI *SHCreateDefaultContextMenu)(const DEFCONTEXTMENU*,REFIID,void**) = 0;
		*(void**)&SHCreateDefaultContextMenu = GetProcAddress(GetModuleHandle("shell32.dll"),"SHCreateDefaultContextMenu"); 

		if(1||!SHCreateDefaultContextMenu) //XP way (preferring for now)
		{	
			debug =	CDefFolderMenu_Create2
			(idlist,hwndOwner,cidl,ppidl,static_cast<IShellFolder*>(this),Somthumb_CDefFolderMenu_Create2_cb,keys_s,keys,(IContextMenu**)ppvObj);			
		}
		else //Vista way (funny; looks just like CDefFolderMenu_Create2)
		{
			DEFCONTEXTMENU dcm = { hwndOwner, 0, idlist, static_cast<IShellFolder*>(this), cidl, ppidl, 0, keys_s, keys	};

			debug = SHCreateDefaultContextMenu(&dcm,__uuidof(IContextMenu),ppvObj);
		}

		for(int i=0;i<keys_s;i++) RegCloseKey(keys[i]);

		if(cidl!=1||debug!=S_OK) return debug;
								
		Somthumb::Folder::Menu *out = 
		new Somthumb::Folder::Menu(this,ppidl[0],*(IContextMenu**)ppvObj);

		*ppvObj = static_cast<IContextMenu*>(out); out->passthru->Release(); //ppvObj

		return debug;
	}
	else if(riid==__uuidof(IDataObject)) //Reminder: required by CDefFolderMenu_Create2
	{
		//Vista way: SHCreateDataObject (todo: required it seems)
		HRESULT debug = CIDLData_CreateFromIDArray(idlist,cidl,ppidl,(IDataObject**)ppvObj);
		assert(!debug);
		return debug;
	}
	else if(riid==__uuidof(IExtractImage))
	{
		assert((GetVersion()&0xFF)<6); //not being retreived by Vista
		
		Somthumb *out = new Somthumb();
		*ppvObj = static_cast<IExtractImage*>(out);	 
		return S_OK;
	}	
	else if(riid==__uuidof(IDropTarget))
	{
		return E_NOINTERFACE;
	}
	else if(riid==__uuidof(IQueryInfo))
	{
		return E_NOINTERFACE;
	}
	else if(riid==__uuidof(IQueryAssociations))
	{			
		return E_NOINTERFACE; //{C46CA590-3C3F-11D2-BEE6-0000F805CA57}
	}
	else if(riid==__uuidof(IExtractIconW)||riid==__uuidof(IExtractIconA))
	{
		//{000214fa-0000-0000-c000-000000000046} {000214eb-0000-0000-c000-000000000046}
		return E_NOINTERFACE;
	}
	else if(riid==__uuidof(IShellLinkW)) return E_NOINTERFACE;
	
#ifdef _DEBUG

	assert(0); 

#endif

	return E_NOINTERFACE;
}
HRESULT Somthumb::Folder::GetDisplayNameOf(const ITEMIDLIST* pidl, DWORD dwFlags, STRRET* pstrName)
{
	if(!pidl||!pstrName) return E_INVALIDARG;

	LPCSHITEMID p = &pidl->mkid; //8 textures 

	int n = *p->abID==255?-1:*p->abID; //255: name

	if(p[0].cb!=3||n>9/*||p[1].cb*/){ assert(0); return E_UNEXPECTED; }
		
	if(n==-1) //name
	{
		pstrName->uType = STRRET_WSTR;	

		//DANGER! returning path here is something like a 
		//symlink pointing at the parent folder (may be unsafe)
		wchar_t *dup = !dwFlags||dwFlags&SHGDN_INFOLDER?name:path;
		
		return SHStrDupW(name,&pstrName->pOleStr);
	}

	pstrName->uType = STRRET_CSTR;

	strcpy(pstrName->cStr,n==0?"_naked.mdl":"_1.tim");
	
	if(n!=0) pstrName->cStr[1] = '0'+n;

	if(!dwFlags||dwFlags&SHGDN_INFOLDER) return S_OK; //relative path;

	wchar_t *out = (wchar_t*)CoTaskMemAlloc(sizeof(wchar_t)*MAX_PATH);

	swprintf_s(out,MAX_PATH,L"%s\\%S",path,pstrName->cStr);
		
	pstrName->uType = STRRET_WSTR;	
	pstrName->pOleStr = out;

	return S_OK;
	
}
HRESULT Somthumb::Folder::SetNameOf(HWND hwnd, const ITEMIDLIST* pidl, const OLECHAR* pszName, DWORD dwFlags, ITEMIDLIST** ppidlOut)
{
	return E_NOTIMPL; //pass thru
}
HRESULT Somthumb::Folder::GetDefaultColumn(DWORD dwRes,ULONG *pSort,ULONG *pDisplay)
{
	if(dwRes||!pSort||!pDisplay) return E_INVALIDARG; 

	*pSort = *pDisplay = 0; return S_OK;
}
HRESULT Somthumb::Folder::GetDefaultColumnState(UINT iColumn,SHCOLSTATEF *pcsFlags)
{
	if(!pcsFlags) return E_INVALIDARG; if(iColumn>0) return E_UNEXPECTED;

	*pcsFlags = SHCOLSTATE_TYPE_STR|SHCOLSTATE_ONBYDEFAULT|SHCOLSTATE_PREFER_VARCMP;

	return S_OK;
}

static const GUID Storage_property_set = //Ntquery.h
{ 0XB725F130, 0X47EF, 0X101A, { 0XA5, 0XF1, 0X02, 0X60, 0X8C, 0X9E, 0XEB, 0XAC } };

static const GUID Summary_property_set = //docs say Ntquery.h (but its not there)
{ 0XF29F85E0, 0X4FF9, 0X1068, { 0XAB, 0X91, 0X08, 0X00, 0X2B, 0X27, 0XB3, 0XD9 } };

HRESULT Somthumb::Folder::GetDetailsEx(PCUITEMID_CHILD pidl,const SHCOLUMNID *pscid,VARIANT *pv)
{
	if(!pidl||!pscid||!pv) return E_INVALIDARG;
		
	if(pscid->fmtid==Storage_property_set) switch(pscid->pid)
	{
	case 10: //display name (title)
				
		STRRET swap;	
		if(GetDisplayNameOf(pidl,0,&swap)!=S_OK) break;

		//assuming caller is responsible for deleting pv

		if(StrRetToStrW(&swap,pidl,&pv->bstrVal)!=S_OK) break;

		pv->vt = VT_BYREF|VT_BSTR; return S_OK;

	case 4: //TODO: type name

	default: return E_UNEXPECTED; 
	}
	if(pscid->fmtid==Summary_property_set) switch(pscid->pid)
	{			 
	default: //Properties->Summary (docs say VT_LPSTR???)
		
		return E_UNEXPECTED; 
	}

	//{F2275480-F782-4291-BD94-F13693513AEC}
	static const GUID XPDetailsPanel = //online docs say obsolete??
	{0xF2275480,0xF782,0x4291,{0xBD,0x94,0xF1,0x36,0x93,0x51,0x3A,0xEC}};
	if(pscid->fmtid==XPDetailsPanel) return E_UNEXPECTED;

	//{8D72ACA1-0716-419A-9AC1-ACB07B18DC32}
	//found 1 Japanese website listing this GUID???	
	static const GUID FileAttributesDisplay = 
	{0x8D72ACA1,0x0716,0x419A,{0x9A,0xC1,0xAC,0xB0,0x7B,0x18,0xDC,0x32}};
	if(pscid->fmtid==FileAttributesDisplay) return E_UNEXPECTED; 

	//ignoring anything else for now
	//assert(0); 
	
	return E_UNEXPECTED; 
}
HRESULT Somthumb::Folder::GetDetailsOf(PCUITEMID_CHILD pidl,UINT iColumn,SHELLDETAILS *psd)
{
	if(!psd) return E_INVALIDARG; 
	
	if(iColumn>0) return E_FAIL; //see IShellDetails::GetDetailsOf docs!
	
	psd->fmt = LVCFMT_LEFT; psd->cxChar = 32; //you guess???

	if(pidl==0) //column headings
	{
		strcpy(psd->str.cStr,"Name"); psd->str.uType = STRRET_CSTR;	
	}
	else return GetDisplayNameOf(pidl,0,&psd->str);

	return S_OK;
}
HRESULT Somthumb::Folder::MapColumnToSCID(UINT iColumn,SHCOLUMNID *pscid)
{
	if(!pscid) return E_INVALIDARG; if(iColumn>0) return E_UNEXPECTED;

	pscid->fmtid = Storage_property_set; pscid->pid = 10; //PID_STG_NAME 
	
	return S_OK;
}

HRESULT Somthumb::Folder::Enum::Next(ULONG celt, PITEMID_CHILD *rgelt, ULONG *pceltFetched)
{
	using namespace SWORDOFMOONLIGHT; 
	
	if(!rgelt||pceltFetched&&celt>1) return E_INVALIDARG;

	mdl::header_t &hd = mdl::imageheader(folder->mdl); if(!hd) return E_ACCESSDENIED;

	int out = 0, objects = 1+hd.timblocks; 

	for(out=0;out<celt&&pos+out<objects;out++) //1: name
	{
		SHITEMID ord[2] = {{sizeof(SHITEMID),pos++},{0,0}}; 

		rgelt[out] = (LPITEMIDLIST)CoTaskMemAlloc(sizeof(ord));

		memcpy(rgelt[out],&ord,sizeof(ord));

		if(pceltFetched) *pceltFetched++;
	}

	return out?S_OK:S_FALSE;
}
HRESULT Somthumb::Folder::Enum::Skip(ULONG celt)
{
	pos+=celt; return S_OK; 
}
HRESULT Somthumb::Folder::Enum::Reset(void)
{
	pos = -1; /*name*/ return S_OK;
}
HRESULT Somthumb::Folder::Enum::Clone(IEnumIDList **ppenum)
{
	if(!ppenum) return E_INVALIDARG; assert(0);

	Somthumb::Folder::Enum *out = new Somthumb::Folder::Enum(folder);

	out->pos = pos; *ppenum = static_cast<IEnumIDList*>(out); 
	
	return S_OK;
}

HRESULT Somthumb::Folder::View::GetData(FORMATETC *pformatetcIn,STGMEDIUM *pmedium)
{
	assert(0); return E_NOTIMPL; //unimplemented
}
HRESULT Somthumb::Folder::View::GetDataHere(FORMATETC *pformatetc, STGMEDIUM *pmedium)
{
	assert(0); return E_NOTIMPL; //unimplemented
}
HRESULT Somthumb::Folder::View::QueryGetData(FORMATETC *pformatetc)
{
	assert(0); return E_NOTIMPL; //unimplemented
}
HRESULT Somthumb::Folder::View::GetCanonicalFormatEtc(FORMATETC *pformatectIn,FORMATETC *pformatetcOut)
{
	assert(0); return E_NOTIMPL; //unimplemented
}
HRESULT Somthumb::Folder::View::SetData(FORMATETC *pformatetc,STGMEDIUM *pmedium,BOOL fRelease)
{
	assert(0); return E_NOTIMPL; //unimplemented
}
HRESULT Somthumb::Folder::View::EnumFormatEtc(DWORD dwDirection,IEnumFORMATETC **ppenumFormatEtc)
{
	assert(0); return E_NOTIMPL; //unimplemented
}
HRESULT Somthumb::Folder::View::DAdvise(FORMATETC *pformatetc,DWORD advf,IAdviseSink *pAdvSink,DWORD *pdwConnection)
{
	assert(0); return E_NOTIMPL; //unimplemented
}
HRESULT Somthumb::Folder::View::DUnadvise(DWORD dwConnection)
{
	assert(0); return E_NOTIMPL; //unimplemented
}
HRESULT Somthumb::Folder::View::EnumDAdvise(IEnumSTATDATA **ppenumAdvise)
{
	assert(0); return E_NOTIMPL; //unimplemented
}
HRESULT Somthumb::Folder::View::DragEnter(IDataObject *pDataObj,DWORD grfKeyState,POINTL pt,DWORD *pdwEffect)
{
	return S_OK; assert(0); return E_NOTIMPL; //unimplemented
}
HRESULT Somthumb::Folder::View::DragOver(DWORD grfKeyState,POINTL pt,DWORD *pdwEffect)
{
	return S_OK; assert(0); return E_NOTIMPL; //unimplemented
}
HRESULT Somthumb::Folder::View::DragLeave(void)
{
	return S_OK; assert(0); return E_NOTIMPL; //unimplemented
}
HRESULT Somthumb::Folder::View::Drop(IDataObject *pDataObj,DWORD grfKeyState,POINTL pt,DWORD *pdwEffect)
{
	return S_OK; assert(0); return E_NOTIMPL; //unimplemented
}

HRESULT Somthumb::Folder::Menu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
	//This is supposed to add Open to a shell file object
	//However, it seems like it's impossible to use along
	//with the default context menu way of doing business

	if(CMF_DEFAULTONLY&uFlags) return MAKE_HRESULT(SEVERITY_SUCCESS,0,USHORT(0));

	MENUITEMINFO mii; memset(&mii,0x00,sizeof(mii));
         
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_ID|MIIM_TYPE|MIIM_STATE;
	mii.wID = idCmdFirst+IDM_OPEN;
	mii.fType = MFT_STRING;
	mii.dwTypeData = "&Open";
	mii.fState = MFS_ENABLED|MFS_DEFAULT;
	InsertMenuItem(hmenu,indexMenu++,TRUE,&mii);

	if(!passthru) return MAKE_HRESULT(SEVERITY_SUCCESS,0,USHORT(IDM_LAST+1));   

	return passthru->QueryContextMenu(hmenu,indexMenu,idCmdFirst,idCmdLast,uFlags);
}
HRESULT Somthumb::Folder::Menu::InvokeCommand(CMINVOKECOMMANDINFO *pici)
{
	//the command is being sent via a verb
	if(HIWORD(pici->lpVerb)) //return NOERROR;
	{
		if(passthru) return passthru->InvokeCommand(pici);
	}
	else switch(LOWORD(pici->lpVerb))
	{
	case IDM_EXPLORE:
	case IDM_OPEN:
	{
	//TODO: try FindExecutable instead

		LPITEMIDLIST pidlFQ;
		SHELLEXECUTEINFOW sei;

		pidlFQ = ILCombine(folder->idlist,idlists[0]);

		wchar_t file[MAX_PATH] = L""; SHGetPathFromIDListW(pidlFQ,file);

		memset(&sei,0x00,sizeof(sei));
		sei.cbSize = sizeof(sei);
		sei.fMask = SEE_MASK_IDLIST|SEE_MASK_CLASSNAME;
		sei.lpIDList = pidlFQ;
		sei.lpClass = L"folder"; //might want to change this (SHGetFileInfo)
		sei.lpFile =  *file?file:0;
		sei.hwnd = pici->hwnd;
		sei.nShow = SW_SHOWNORMAL;

		sei.lpVerb = LOWORD(pici->lpVerb)==IDM_EXPLORE?L"explore":L"open";

		BOOL debug = ShellExecuteExW(&sei); //success code; but nothing

		ILFree(pidlFQ);

	}break;	  

	default: if(passthru) return passthru->InvokeCommand(pici);
	}

	return NOERROR;
}
HRESULT Somthumb::Folder::Menu::GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pReserved, LPSTR pszName, UINT cchMax)
{	
	if(passthru) return passthru->GetCommandString(idCmd,uType,pReserved,pszName,cchMax);

	return E_FAIL; //despite the name the docs suggest this method returns an infotip
}

STDAPI DllInstall(BOOL I, LPCWSTR cmd);

static INT_PTR CALLBACK SomthumbPreviewMoreProc(HWND,UINT,WPARAM,LPARAM);

static INT_PTR CALLBACK SomthumbPreviewProps(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	static ULONG pvmore = -1UL;

	switch(Msg)
	{
	case WM_INITDIALOG: pvmore = -1UL;
	{			
		HWND dialog = GetParent(hWndDlg);

		EnableThemeDialogTexture(hWndDlg,ETDT_ENABLETAB);
		
		HKEY classes = 0, options = 0; LONG err =  
		RegOpenKeyExW(Somthumb_root(),L"Software\\Classes",0,KEY_READ,&classes);
		RegOpenKeyExW(classes,L"CLSID\\"SOMPLAYER_PREVIEW,0,KEY_READ,&options);
		
		const int checks[] = {IDC_TXR,IDC_TIM,IDC_DDS,IDC_MDO,IDC_MDL,IDC_MSM,0};

		const wchar_t *formats[] = {L".txr",L".tim",L".dds",L".mdo",L".mdl",L".msm"};

		wchar_t extractf[MAX_PATH] = L""; 

		wchar_t clsid[MAX_PATH];
		for(int i=0;checks[i];i++) if(*formats[i])
		{
			swprintf_s(extractf,
			L"SystemFileAssociations\\%s\\ShellEx\\"
			L"{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}",formats[i]); //IExtractFile

			LONG max_path = sizeof(extractf), err = 
			RegQueryValueW(classes,extractf,clsid,&max_path);
			int check = err==ERROR_SUCCESS&&!wcsicmp(clsid,SOMPLAYER_PREVIEW)?1:0;
			SendDlgItemMessage(hWndDlg,checks[i],BM_SETCHECK,check,0);
		}

		DWORD dword = 0, dword_s = 4; 		
		SHGetValueW(options,0,L"EnlargeThumbnails",0,&dword,&dword_s);
		
		SendDlgItemMessage(hWndDlg,IDC_ENLARGE,BM_SETCHECK,dword,0);

		RegCloseKey(classes); RegCloseKey(options);

		//rc text limited to 256 characters
		SetDlgItemText(hWndDlg,IDC_WARNING, 
		"Warning: By choosing OK you are installing these\r\n"
		"features into Windows Explorer. A temporary copy of\r\n"
		"Somplayer.dll is used by Explorer (it is lost when temporary\r\n"
		"files are routinely discarded. Run any software that uses\r\n"
		"Somplayer.dll to prepare a new copy for Explorer.)");
						
		ShowWindow(hWndDlg,SW_SHOW); 

		SendDlgItemMessage(hWndDlg,IDC_I_AGREE,BM_SETCHECK,1,0);
			
		RECT pos = Sompaste->pane(hWndDlg); //hack: adjust margins
		SetWindowPos(hWndDlg,0,pos.left+8,pos.top,pos.right-pos.left-8,pos.bottom-pos.top,SWP_NOZORDER|SWP_NOACTIVATE);

		return FALSE;
	}	
	case WM_NOTIFY:
	{
		PSHNOTIFY *p = (PSHNOTIFY*)lParam;

		switch(p->hdr.code) //how a property sheet would work
		{				
		case PSN_SETACTIVE: case PSN_KILLACTIVE: 
		{
			int show = p->hdr.code==PSN_SETACTIVE?SW_SHOW:SW_HIDE;

			switch(p->hdr.idFrom)
			{
			case 0: //should be Channels

				//ShowWindow(GetDlgItem(hWndDlg,IDC_LISTVIEW),show); 
				break;

			case 1: //should be Notepad
				
				//ShowWindow(GetDlgItem(hWndDlg,IDC_NOTEPAD),show); 
				break;

			default: assert(0);
			}

			SetWindowLong(hWndDlg,DWL_MSGRESULT,FALSE); return TRUE;
		}
		case PSN_APPLY:
		{
			//if(p->hdr.idFrom==1) break; //avoid double apply

			bool txr = SendDlgItemMessage(hWndDlg,IDC_TXR,BM_GETCHECK,0,0);
			bool tim = SendDlgItemMessage(hWndDlg,IDC_TIM,BM_GETCHECK,0,0);			
			bool dds = SendDlgItemMessage(hWndDlg,IDC_DDS,BM_GETCHECK,0,0);
			bool mdo = SendDlgItemMessage(hWndDlg,IDC_MDO,BM_GETCHECK,0,0);
			bool mdl = SendDlgItemMessage(hWndDlg,IDC_MDL,BM_GETCHECK,0,0);			
			bool msm = SendDlgItemMessage(hWndDlg,IDC_MSM,BM_GETCHECK,0,0);

			bool enlarge = SendDlgItemMessage(hWndDlg,IDC_ENLARGE,BM_GETCHECK,0,0);

			std::wstring i = L"", u = L""; //(((: string pooling
			(((txr?i:u)+=L" ")+=L".txr")+=L"\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}";
			(((tim?i:u)+=L" ")+=L".tim")+=L"\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}";
			(((dds?i:u)+=L" ")+=L".dds")+=L"\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}";
			(((mdo?i:u)+=L" ")+=L".mdo")+=L"\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}";
			(((mdl?i:u)+=L" ")+=L".mdl")+=L"\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}";
			(((msm?i:u)+=L" ")+=L".msm")+=L"\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}";
			((enlarge?i:u)+=L" ")+=L"EnlargeThumbnails";

			if(pvmore!=-1UL) //(((: string pooling
			{
			(((pvmore&1<<(IDC_BMP-IDC_BMP)?i:u)+=L" ")+=L".bmp")+=L"\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}";
			(((pvmore&1<<(IDC_DIB-IDC_BMP)?i:u)+=L" ")+=L".dib")+=L"\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}";
			(((pvmore&1<<(IDC_PNG-IDC_BMP)?i:u)+=L" ")+=L".png")+=L"\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}";
			(((pvmore&1<<(IDC_JPEG-IDC_BMP)?i:u)+=L" ")+=L".jpeg")+=L"\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}";
			(((pvmore&1<<(IDC_JPG-IDC_BMP)?i:u)+=L" ")+=L".jpg")+=L"\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}";
			(((pvmore&1<<(IDC_TGA-IDC_BMP)?i:u)+=L" ")+=L".tga")+=L"\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}";
			(((pvmore&1<<(IDC_HDR-IDC_BMP)?i:u)+=L" ")+=L".hdr")+=L"\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}";
			(((pvmore&1<<(IDC_PPM-IDC_BMP)?i:u)+=L" ")+=L".ppm")+=L"\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}";
			(((pvmore&1<<(IDC_PFM-IDC_BMP)?i:u)+=L" ")+=L".pfm")+=L"\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}";
			pvmore = -1UL;
			}

			//contrary to documentation, regsvr32 /i producess "" rather than 0

			const wchar_t *c_i = i.c_str(); if(*c_i) DllInstall(1,c_i);	
			const wchar_t *u_i = u.c_str(); if(*u_i) DllInstall(0,u_i);

		}break;		
		}

	}break;
	case WM_COMMAND:
	{			 	
		switch(LOWORD(wParam))
		{		
		case IDC_RUN_EXPLORER: 
		{
			wchar_t temp[MAX_PATH] = L""; Somfolder(temp);

			ShellExecuteW(Sompaste->window(),L"explore",temp,0,0,SW_SHOWNORMAL); 			

		}break;
		case IDC_BMP_ETC: 
		{
			pvmore = DialogBoxParamW(Somplayer_dll(),MAKEINTRESOURCEW(IDD_DLLINSTALL_BMP_ETC),hWndDlg,SomthumbPreviewMoreProc,pvmore);			

			if(pvmore==-1UL) break; //FALLS THRU
		}
		default: //turn on the Apply button
			
			SendMessage(GetParent(hWndDlg),PSM_CHANGED,(WPARAM)hWndDlg,0);
		}

	}break;
	case WM_DESTROY: 
	{
	close:;	//DestroyWindow(hWndDlg); 
	}
	}//switch

	return 0;
}

static INT_PTR CALLBACK SomthumbPreviewMoreProc(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	static ULONG ok = -1UL, cancel = ok;

	switch(Msg)
	{
	case WM_INITDIALOG: 

		cancel = lParam;
		
		if((ok=lParam)==-1UL) //pull initial state from registry
		{
			ok = 0x00000000;

			HKEY classes = 0, options = 0; LONG err =  
			RegOpenKeyExW(Somthumb_root(),L"Software\\Classes",0,KEY_READ,&classes);

			wchar_t ext[6], key[MAX_PATH], clsid[MAX_PATH];
			if(IDC_BMP<IDC_PFM) for(int i=IDC_BMP;i<=IDC_PFM;i++) 
			{
				if(GetDlgItemTextW(hWndDlg,i,ext,6)&&*ext=='.')
				{
					ext[ext[4]==' '?4:5] = '\0'; *key = 0; //IExtractFile
					swprintf_s(key,L"SystemFileAssociations\\%s\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}",ext);

					LONG max_path = sizeof(clsid), err = RegQueryValueW(classes,key,clsid,&max_path);

					if(err==ERROR_SUCCESS&&!wcsncmp(clsid,SOMPLAYER_PREVIEW,sizeof(clsid)))
					{
						ok|=1<<i-IDC_BMP; //we are in ownership of this file format
					}
				}
			}

			RegCloseKey(classes); 
		}

		if(ok) SendDlgItemMessage(hWndDlg,IDC_CHECK_ALL,BM_SETCHECK,1,0);

		if(IDC_BMP<IDC_PFM) for(int i=IDC_BMP;i<=IDC_PFM;i++) 
		if(ok&1<<i-IDC_BMP)	SendDlgItemMessage(hWndDlg,i,BM_SETCHECK,1,0);

		return TRUE;
	
	case WM_COMMAND:
	
		int lo = LOWORD(wParam);

		bool check = //Assuming checkbox
		SendDlgItemMessage(hWndDlg,LOWORD(wParam),BM_GETCHECK,0,0);

		if(lo>=IDC_BMP&&lo<=IDC_PFM)
		{
			int shift = LOWORD(wParam)-IDC_BMP;
			if(check) ok|=1<<shift; else ok&=~(1<<shift);  
			SendDlgItemMessage(hWndDlg,IDC_CHECK_ALL,BM_SETCHECK,ok?1:0,0);
		}
		else switch(LOWORD(wParam))
		{
		case IDC_CHECK_ALL:
		{
			if(IDC_BMP<IDC_PFM)
			for(int i=IDC_BMP;i<=IDC_PFM;i++)
			{
				int shift = i-IDC_BMP;
				if(check) ok|=1<<shift; else ok&=~(1<<shift);
				SendDlgItemMessage(hWndDlg,i,BM_SETCHECK,check,0);
			}
			break;
		}
		case IDCANCEL: 

			EndDialog(hWndDlg,cancel); break;

		case IDOK: 

			EndDialog(hWndDlg,ok); break;
		}
		break;
	}

	return 0;
}

STDAPI DllInstall(BOOL I, LPCWSTR cmd)
{
	//XP
	//TODO: {3F30C968-480A-4C6C-862D-EFC0897BB84B}
	//regsvr32 %windir%\system32\shimgvw.dll

	Somthumb_clsid(I); //repair
	
	//Reminder:
	//contrary to documentation, regsvr32 /i producess "" rather than 0

	if(!cmd||!*cmd) //TODO: parse command
	{
		int tabs_s = 1; //todo: accumulate
				
		HWND *sheet = 0, owner = 0;

		HMODULE Sompaste_dll = LoadLibrary("Sompaste.dll");

		if(Sompaste_dll)
		{
			//Optimization:
			//increment module refcount /DELAYLOAD
			//to work around a free->load scenario
			owner = Sompaste->window(); 		
			FreeLibrary(Sompaste_dll);		
		}
		else //returns NOERROR
		{
			const char caption[] = 
			"Somplayer.dll - DllInstall is missing Sompaste.dll";
			const char text[] = 
			"The menu dialog requires Sompaste.dll.\r\n"
			"\r\n"
			"It is enough to copy Sompaste.dll into the same folder.\r\n"
			"\r\n"
			"Alternatively supply one or more of the following arguments.\r\n"
			"\r\n"
			"Thumbnail shell extensions take the form of:\r\n"
			"\r\n"
			".<ext>\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}\r\n"
			"\r\n"
			"Where <ext> may be one of; txr, tim, dds.\r\n"
			"\r\n"
			"Additional options; EnlargeThumbnails";

			MessageBoxA(GetConsoleWindow(),text,caption,IDOK|MB_ICONINFORMATION);

			return NOERROR;
		}

		if(sheet=Sompaste->propsheet(owner,tabs_s,L"Somplayer.dll - DllInstall"))
		{	
			HWND modal = sheet[tabs_s];	 			
			HWND preview = CreateDialogW(Somplayer_dll(),MAKEINTRESOURCEW(IDD_DLLINSTALL),modal,SomthumbPreviewProps);		
			HWND tabs = (HWND)SendMessage(modal,PSM_GETTABCONTROL,0,0);

			TCITEMW tab = { TCIF_TEXT,0,0,L"Preview",8,0};
			SendMessageW(tabs,TCM_SETITEMW,0,(LPARAM)&tab);

			SetWindowLong(modal,GWL_EXSTYLE,WS_EX_APPWINDOW|GetWindowLong(modal,GWL_EXSTYLE));

			if(modal!=Sompaste->wallpaper(modal)) return NOERROR; 
			   
			//modal loop is for regsvr32
			for(MSG msg;IsWindow(modal);)
			{
				if(GetMessageW(&msg,0,0,0)<=0)
				{
					DestroyWindow(modal); exit(0); return NOERROR;
				}
				else if(Sompaste->wallpaper(0,&msg)) continue;
		
				if(IsDialogMessageW(modal,&msg)) continue;
				
				TranslateMessage(&msg); 
				DispatchMessageW(&msg); 
			}
		}

		return NOERROR; //it's in the dialog's hands now
	}

	HRESULT hr = E_UNEXPECTED; 
			
	HKEY admin = 0; LONG err = 
	RegOpenKeyEx(HKEY_LOCAL_MACHINE,0,0,KEY_ALL_ACCESS,&admin);
	RegCloseKey(admin);
		
	int argc = 0; wchar_t **argv = CommandLineToArgvW(cmd,&argc);
	
	DWORD rights = I?KEY_WRITE:KEY_READ|KEY_SET_VALUE;

	size_t errors = 0;

	for(int i=0;i<=Somthumb_admin();i++) //gotta be a better way??
	{
		HKEY root = Somthumb_root(i);
		HKEY classes = 0, options = 0; LONG err =  
		RegOpenKeyExW(root,L"Software\\Classes",0,rights,&classes);
		RegOpenKeyExW(classes,L"CLSID\\"SOMPLAYER_PREVIEW,0,rights,&options);

		if(err!=ERROR_SUCCESS) errors++;

		if(!err) for(int i=0;i<argc;i++)
		{
			if(!*argv[i]) continue;

			bool complete = true, option = false;

			switch(*argv[i])
			{
			case '.': //file extensions

				//IExtractImage (thumbs)			
				//TODO: look into how these interact with the newer IthumbnailProvier iface
				if(!_wcsicmp(argv[i],L".tim\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}")) break;
				if(!_wcsicmp(argv[i],L".txr\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}")) break;				
				if(!_wcsicmp(argv[i],L".dds\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}")) break;
				if(!_wcsicmp(argv[i],L".mdo\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}")) break;
				if(!_wcsicmp(argv[i],L".mdl\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}")) break;
				if(!_wcsicmp(argv[i],L".msm\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}")) break;

				//Reminder: these formats are not supplied by DllRegisterServer on purpose
				//Adding support for D3DXCreateTextureFromFile formats because it's easy to do
				//And because some formats such as BMP demonstrate misbehaving Shell Extensions
				if(!_wcsicmp(argv[i],L".bmp\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}")) break;
				if(!_wcsicmp(argv[i],L".dib\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}")) break;
				if(!_wcsicmp(argv[i],L".png\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}")) break;
				if(!_wcsicmp(argv[i],L".jpeg\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}")) break;
				if(!_wcsicmp(argv[i],L".jpg\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}")) break;
				if(!_wcsicmp(argv[i],L".tga\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}")) break;
				if(!_wcsicmp(argv[i],L".hdr\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}")) break;
				if(!_wcsicmp(argv[i],L".ppm\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}")) break;
				if(!_wcsicmp(argv[i],L".pfm\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}")) break;				

			default: //options?
				
				if(!_wcsicmp(argv[i],L"EnlargeThumbnails"))
				{
					option = true; break;
				}
				complete = false;
			}

			if(option)
			{
				BYTE dword[] = {1,0,0,0};

				//Annoying: must see that value exists before deleting
				if(!I&&!SHGetValueW(options,0,argv[i],0,0,0))
				{				
					complete = !RegDeleteValueW(options,argv[i]);
				}			
				if(I) complete = !RegSetValueExW(options,argv[i],0,REG_DWORD,dword,4);
			}
			else if(complete)
			{
				wchar_t expand[512] = L"", *slash = wcschr(argv[i],'\\'); 

				if(slash) *slash = 0; 
				if(slash) swprintf_s(expand,L"SystemFileAssociations\\%s\\%s",argv[i],slash+1); 

				//TODO: backup/restore preexisting values??

				if(!*expand) errors++; else if(I) 
				{
					static wchar_t SOMPLayer_Preview[] = L"SOMPLayer.Preview";
					complete = !RegSetValueW(classes,expand,REG_SZ,SOMPLAYER_PREVIEW,sizeof(SOMPLAYER_PREVIEW))
					&&!RegSetValueW(classes,argv[i],REG_SZ,SOMPLayer_Preview,sizeof(SOMPLayer_Preview)); 					
				}
				else //removing
				{
					//avoid deleting unrelated shell extensions
					wchar_t nice[MAX_PATH]; LONG max_path = sizeof(nice);			
					LONG err = RegQueryValueW(classes,expand,nice,&max_path);
					if(err==ERROR_SUCCESS&&!wcscmp(nice,SOMPLAYER_PREVIEW))
					{	
						complete = !SHDeleteValueW(classes,expand,0) 
						; //&&!SHDeleteValueW(classes,argv[i],0);
					}
					max_path = sizeof(nice); err = 
					RegQueryValueW(classes,argv[i],nice,&max_path);
					if(err==ERROR_SUCCESS&&!wcscmp(nice,SOMPLAYER_PREVIEW))
					{	
						complete&=!SHDeleteValueW(classes,argv[i],0);
					}
				}

				if(slash) *slash = '\\'; //pass 2
			}
			if(!complete) errors++;
		}

		RegCloseKey(classes); RegCloseKey(options);
	}

	if(!errors) hr = NOERROR; assert(errors==0);

	if(!hr) SHChangeNotify(SHCNE_ASSOCCHANGED,SHCNF_IDLIST,0,0);

	LocalFree(argv);

	return hr;
}

static const wchar_t Somplayer_regsvr32[] =
L".tim\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1} "
L".txr\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1} "
L".dds\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1} "
L".mdo\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1} "
L".mdl\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1} "
L".msm\\ShellEx\\{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1} ";

STDAPI DllRegisterServer()
{
	return DllInstall(TRUE,Somplayer_regsvr32);
}

STDAPI DllUnregisterServer()
{
	return DllInstall(FALSE,Somplayer_regsvr32);
}

STDAPI DllGetClassObject(const CLSID &rclsid, const IID &riid, void **ppv)
{		
	if(!ppv) return E_INVALIDARG;

	assert(rclsid==SOMPlayer_Preview
		 ||rclsid==SOMPlayer_Explore);

	*ppv = 0;
	if(riid==__uuidof(IClassFactory))
	{
//	static int once = 0; assert(once++); //debugging

		Somthumb::factory.AddRef();

		*ppv = static_cast<IClassFactory*>(&Somthumb::factory);
	}
	else assert(0);

	return *ppv?S_OK:CLASS_E_CLASSNOTAVAILABLE;
} 

STDAPI DllCanUnloadNow()
{
	if(Somthumb::factory) return S_FALSE;

	Somthumb::pool->release(); Somthumb::pool = 0; 

	return S_OK;
}