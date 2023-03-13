					
#ifndef SOM_STATUS_INCLUDED
#define SOM_STATUS_INCLUDED 

namespace SOM
{	
	extern void Map(void*vb),Map2(void*);

	extern void Clear(DWORD color, float z=0);   
	inline void Black(){ Clear(0x64000000,0); } 
	inline void Tint(){	Clear(EX::context()==0?0x80000000:0x64000000,0.0f); }	
	extern void Pause(int chan='user'),Unpause(int chan='user'); 	
	//TODO: setup a multi-job Spool API for the likes of do_st under DirectDraw 7
	extern int Print(const wchar_t*, LPRECT=0, UINT=DT_CENTER|DT_VCENTER|DT_SINGLELINE, UINT *etc=0);
	//NEW: Print text start menu with custom tint and system font and no contrast
	extern int Start(const wchar_t*, DWORD tint, LPRECT=0, UINT=DT_CENTER|DT_SINGLELINE);
	
	//Retrieve controller button status with their labels
	extern bool *Buttons(bool *down=0, size_t downsz=32);
	extern const wchar_t *Joypad(int,const wchar_t*prefix=0); 
	
	//Shadow Tower: elem can be 'pts', 'bar', 'bad', or 'hit'
	extern const wchar_t *Status(RECT*, UINT*, int elem, size_t n);			
	extern struct Versus 
	{
		enum{ timeout=1000 };
		//NOTE: damage_sources is 64
		static const size_t x_s = 8;
		//history: adapted from f7 function overlay
		struct x
		{
			int npc, enemy, timer, drain, multi, combo;
			float red; //2020
			DWORD EDI; WORD HP;
		}x[x_s]; int f7; //2017		
		inline struct x &operator[](int i){ return x[i]; }
		struct{ int timer, multi, combo; }all; //new
		Versus(){ memset(this,0xFF,sizeof(*this)); }		
		int operator()(); //SOM::vs()
		//2017
		bool hit(int enemy,int,int,float),hit2(int npc,int,int,float);

	}Versus; 

	//REMOVE US
	//this now needs to be based on critical_hit_point_quantifier
	//it really doesn't belong here but it's hard to deal with 
	//event based damage and poison
	//extern float Red(WORD hit, WORD max);
	extern float Red(WORD hit, int src=12288, int npc=12288, WORD max=0);
	extern bool Stun(float red);

	//Commands: EnableCinematicMode/EnableVRMode
	//https://github.com/gusmanb/PSVRFramework/wiki/Toolbox-network-commands-and-command-line-parameters
	struct Sixaxis
	{	
		int time; //0~FFFFFF
		short gyro[3],motion[3]; //Yaw,Pitch,Roll;
		int button;
	};
	extern void PSVRToolbox(const char *Command=0);

	extern unsigned &frame;

	//https://www.nolovr.com/hardwareDevice
	extern struct NoloClientLib
	{
		HMODULE dll;
		
		int started;

		NoloClientLib(){ memset(this,0x00,sizeof(*this)); }

	}*NoloVR;
}

#endif //SOM_STATUS_INCLUDED



