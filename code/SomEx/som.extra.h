
#ifndef SOM_EXTRA_INCLUDED
#define SOM_EXTRA_INCLUDED
				   
#include "SomEx.h" //courtesy

namespace som_scene_state
{
	extern void push(),pop();
}

namespace SOM //miscellaneous externals
{	
	//EXPERIMENTAL
	//setting to 1 to disable averaging
	//NOTE: it seems like this is definitely needed
	//even though it may be the source of spasms :(
	//(it's choppy without it and choppy without 
	//triple buffering enabled as well)
	extern DWORD onflip_triple_time[30];

	extern void launch_legacy_exe(); //som.exe.cpp	
	extern DWORD exe(const wchar_t*);
	extern HANDLE exe_process, exe_thread; //CloseHandle
	extern DWORD exe_threadId; //XP
	extern HWND splash();
	extern bool splashed(HWND,LPARAM=0);
	extern void initialize_taskbar(HWND);

	extern void zentai_init(HANDLE EZT); //som.zentai.cpp
	extern void zentai_split(HANDLE EVT); //postwrite
	extern void zentai_splice(HANDLE EVT); //preread

	//REMOVE ME?
	//extern void initialize_som_keys_cpp();
	extern void initialize_workshop_cpp(); 
	extern void initialize_som_hacks_cpp(); 
	extern void initialize_som_menus_cpp(); 
	extern void initialize_som_status_cpp();
			
	namespace HACKS //testing: som.hacks.cpp
	{
		//extern float compass_needle_fudgefactor; 

		//extern float player_character_height_adjust;

		extern HDC stretchblt;
	}
	extern void kickoff_somhacks_thread(); //REMOVE ME?
		
	//xtool/copy (language packs)
	//xtool extracts a tool EXE from the tool folder
	//xcopy extracts any file falling back to the install
	extern void xtool(wchar_t[MAX_PATH], const wchar_t*); //som.tool.h
	//dst is relative to the project folder
	//src is relative to the installation folder
	extern bool xcopy(const wchar_t *dst, const wchar_t *src); //som.tool.h	

	struct xxiix //EXTENSION //2018
	{
		unsigned x1:10,x3:10,ii:2,x2:10;
		
		operator int&(){ return *(int*)this; }
		
		int &operator=(int i){ return *(int*)this = i; }

		WORD &operator[](int i){ return ((WORD*)this)[i]; } //???

		int ryoku(int i){ i = i==2?x2:i==3?x3:x1; return i==0?50:i-1; }
	};
	
	//EXPERIMENTAL
	struct Thread //2022
	{
		void *job;
		int exited;
		int suspended;
		HANDLE handle;
		EX::critical *cs;
		DWORD(WINAPI*main)(Thread*);
		Thread(DWORD(WINAPI*)(Thread*));
		~Thread();

		bool create(),close(); 
		int suspend(),resume();
	};
	struct TextureFIFO //2022
	{
		//this is a circular queue that would jam if full
		//but there should always be some textures which
		//don't require processing, so it won't fill up

		TextureFIFO();
		void clear(){ new(this) TextureFIFO; }
		bool empty(){ return _front==_back; }
		void push_back(WORD);
		WORD remove_front();

		//might want to use InterlockedCompareExchange if
		//there were more than one consumers on each end
		//volatile LONG _front, _back;
		LONG _front, _back;
		WORD _buffer[1024];
	};
	extern void queue_up_game_map(int); //som.MPX.cpp
}

#endif //SOM_EXTRA_INCLUDED