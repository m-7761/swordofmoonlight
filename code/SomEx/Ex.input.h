
#ifndef EX_INPUT_INCLUDED
#define EX_INPUT_INCLUDED

#ifndef EX_MICE
#define EX_MICE 1
#endif
#ifndef EX_JOYPADS
#define EX_JOYPADS 1
#endif
#ifndef EX_AFFECTS
#define EX_AFFECTS 3
#endif

#ifndef EX_MACRO_RUNAWAY_ALERT_ON
#define EX_MACRO_RUNAWAY_ALERT_ON 32
#endif

namespace DX
{
	class IDirectInputDeviceW;
	class IDirectInputDevice7W;
}		
namespace EX
{	
namespace INI{ struct Joypad; }
extern bool vk_pause_was_pressed;
extern EX::INI::Joypad universal_mouse;
static bool issue_macro_runaway_warning(int run=EX_MACRO_RUNAWAY_ALERT_ON)
{
	assert(run==EX_MACRO_RUNAWAY_ALERT_ON); //hardcoded for now

	assert(!"MACRO_RUNAWAY>32"); return true;
}}
			
namespace EX{	
extern class Keypad 
{										   
public:
		
	bool configured;

	enum{keystates_s=256,MAX_MACROS=16};

	const unsigned char *macros[MAX_MACROS]; 

	int macro_counters[MAX_MACROS]; //repetition
	int macro_runaways[MAX_MACROS]; //safety first
							   	
	bool keystates[keystates_s]; int contexts[keystates_s]; 

	//simulate: advance/union keypad state with x (fyi: x optional)
	//x should be initialized or in an intermediary state (not junk)
	//unx returns (upon success) with the raw untranslated key states
	bool simulate(unsigned char *x, size_t x_s, unsigned char *unx=0); 

	Keypad()
	{
		memset(this,0x00,sizeof(*this));
	}

}Keypad; //EX::
}

namespace EX{
extern class Syspad 
{										   
public:
	
	static const int MAX_BUFFER = 64;
	static const int MAX_REPEAT = 255;
	static const int MAX_MEMORY = 256;

	//send: holds down key for period of repeat
	//echo: via EX::broadcasting_direct_input_key(x,0)
	//spam: presses then releases key repeat times
	//Note: echo will broadcast every round regardless of key (ie. stick with n=1)
	inline EX::Syspad &send(unsigned char x, int n=1){ _repeat(2,x,n); return *this; }
	inline EX::Syspad &echo(unsigned char x, int n=1){ _repeat(3,x,n); return *this; }
	inline EX::Syspad &spam(unsigned char x, int n=9){ _repeat(4,x,n); return *this; }
		
	//+: tie additional keys to the previous send/echo/spam call
	//Usage: final key blocks, so you wanna do send(ALT)+(F4) vs. F4+ALT
	inline EX::Syspad &operator+(unsigned char x){ _tie(1,x); return *this; }

	//simulate: syspad will attempt to complete any
	//outstanding input via send/echo/spam as long as
	//that key has not be pressed/released since the 
	//previous round of simulation; eg. simulate()
	bool simulate(unsigned char *x, size_t x_s); 

	//memory: effectively memory represents the 
	//combined input via EX::Keypad and EX::Joypad 
	//for the previous round of simulation. Don't 
	//change it though or EX::Syspad will be unable
	//to cleanly merge input when the time comes
	unsigned char memory[MAX_MEMORY];

	Syspad()
	{
		memset(this,0x00,sizeof(*this));

		_buff = _swap[0];
	}

	void exit(int); //dicey

//private:

	void _repeat(int,unsigned char,int);

	unsigned char _swap[2][MAX_BUFFER*3];		
	
	unsigned char *_buff; int _fifo;

	void _tie(int,unsigned char);

}Syspad; //EX::
}

namespace EX{
extern class Joypad 
{
public: GUID instance;
	
	GUID product; //NEW
	int isXInputDevice;
	int isDualShock;
	EX::INI::Joypad ini;	

	DX::IDirectInputDeviceW *dx;
	DX::IDirectInputDevice7W *dx7;

	//NEW: thumbs is a user provided
	//procedure for reshaping/rotating
	//axis pairs
	//this formula makes square inputs to 
	//fall within a circle
	//xx = x*sqrt(1-y*y/2);
	//yy = y*sqrt(1-x*x/2);
	static bool (*thumbs)(int,float[8]);

	//2020: test if a trigger was fully
	//released before the current press
	static int trigger_full_release(int);
	static int trigger_fast_release(int);
		
	struct Configuration //aggregate (pod)
	{
		int analog_mode[8][EX::contexts]; //9

		//TODO: neg_ should come first / rethink
		//(maybe don't interleave them at all!!)
		unsigned short pos_pos[3][EX::contexts];
		unsigned short neg_pos[3][EX::contexts];
		unsigned short pos_rot[3][EX::contexts];
		unsigned short neg_rot[3][EX::contexts];
		unsigned short pos_aux[2][EX::contexts];
		unsigned short neg_aux[2][EX::contexts];
		unsigned short pov_hat[8][EX::contexts];
		/*UNUSED
		//TODO? don't interleave neg_pos/rot/aux
		static int map_axis_to_pos_pos(int i)
		{
			if(i<3) return i; 
			if(i<6) return 6+i-3; return 12+i-6;
		}
		static int map_pos_pos_to_axis(int i)
		{
			if(i<6) return i%3; i-=6;
			if(i<6) return 3+i%3; i-=6; return 6+i%2;
		}*/
				
		unsigned short buttons[32][EX::contexts];
		
		unsigned short menu[EX::contexts]; //mice

		//REMOVE ME?
		unsigned short deadzone[8][EX::contexts];
		unsigned short saturate[8][EX::contexts];

		inline unsigned short *a(){	return pos_pos[0]; }

		inline size_t x()const{ return deadzone[0]-pos_pos[0]; } 
		inline size_t z()const{ return (unsigned short*)&this[1]-pos_pos[0]; }

		void operator+=(const Configuration &c)
		{				
			for(size_t i=0,n=z();i<n;i++) //!
			{
				if(!pos_pos[0][i]) pos_pos[0][i] = c.pos_pos[0][i];
			}
			for(size_t i=0;i<8*EX::contexts;i++)
			{
				analog_mode[0][i]+=c.analog_mode[0][i];
			}
		}

		void operator=(int zero) 
		{
			memset(this,zero,sizeof(*this));
		}		

	}cfg;

	typedef Configuration Config;

	//the default configurations
	static Configuration padscfg; 
	static Configuration micecfg; 
		
	float position[8], range[8][2];

	int povhat, buttons[32], contexts[32]; 

	int menu[EX::contexts]; //mice

	int triggers[2]; //XInput/DualShock4

	enum{ mice_smoothing_max=4 };
	static int wm_input[EX_MICE*2];
	static float absolute[EX_MICE*3]; 
	static float smoothing[EX_MICE*2][mice_smoothing_max];

	static const int MAX_MACROS = 16;

	const unsigned char *macros[MAX_MACROS]; 

	int macro_counters[MAX_MACROS]; //repetition
	int macro_runaways[MAX_MACROS]; //safety first

	bool issue_macro_runaway_warning(int run=EX_MACRO_RUNAWAY_ALERT_ON);

	bool active;
	bool activate(EX::INI::Joypad,const GUID&,const GUID&XInputProductID=GUID_NULL);
	void deactivate();

	//WARNING: call to IWbemServices::CreateInstanceEnum is incurring
	//a very long delay (today) so it's recommended to not do this at
	//start up, or in a real-time thread	
	//sets isXInputDevice for all active joypads
	static void detectXInputDevices(); 

	//anolog input is saved in the lower 7 bits in the
	//form of 8 binary gaits: 0, 1, 11, 111, 1111, etc
	bool simulate(unsigned char *x, size_t x_s);
		
	//REMOVE ME?: implements do_escape details
	void assign_gaits_if_default_joypad(const float gaits[7]);

	int analog[8]; //for visualization

	static float analog_dilation; //TESTING
	
	static unsigned &analog_clock; //REMOVE ME?
		
	static int analog_dpad(int i)
	{
		//0x07: approximate SOM axis deadzone
		if(i&1) return (i&0x07)==0x07?0x80:0;

		return i==0x80?i:0; //lower ranges
	}

	static int gaitcode(int bytecode)
	{
		if(bytecode<0) //REMOVE ME?
		{
			assert(0); //phasing out
			return EX::Joypad::bytecode(-bytecode);
		}
		else switch(bytecode&0x7F)
		{
		case 0x01: return 1; //1/5
		case 0x03: return 2; //1/3
		case 0x07: return 3; //1/2
		case 0x0f: return 4; //2/3
		case 0x1f: return 5; //3/4
		case 0x3f: return 6; //7/8
		case 0x7f: return 7;

		case 0x00: return bytecode==0x80?7:0; 

		default: //lower ranges
			
			//2017: Jumpstart hit this with 0x15 on key 0x47
			//even if |= there couldn't be two analog inputs
			//NOTE: mouse+controller (sticky stick) can be a
			//source of two analog inputs. som.hacks.cpp has
			//to filter half gaits (10<<1) since they do not
			//work with the built-in |= operator
			assert(~bytecode&1&&bytecode<256);			
			return -(bytecode>>1&0x3F);
		}
	}
	static unsigned char bytecode(int gaitcode)
	{
		if(gaitcode>0) switch(gaitcode)
		{		
		case 1: return 0x1;
		case 2: return 0x3;
		case 3: return 0x7;
		case 4: return 0xf;
		case 5: return 0x1f;
		case 6: return 0x3f;
		case 7: return 0x7f;
		}
		else if(gaitcode<0) 
		return -gaitcode<<1&0x3F; //lower ranges
		else assert(!gaitcode);
		return 0;
	}

	static int analog_dither(int i)
	{	
		if(i==0x80) i = 0x7F;		

		if(i&1) switch(i&0x7f) 
		{		
		case 0x01: i=analog_clock%5?0x01:0x81; break;
		case 0x03: i=analog_clock%3?0x03:0x83; break;
		case 0x07: i=analog_clock%2?0x07:0x87; break;
		case 0x0f: i=analog_clock%3?0x8f:0x0f; break;
		case 0x1f: i=analog_clock%4?0x9f:0x1f; break;
		case 0x3f: i=analog_clock%7?0xBf:0x3f; break;		
		case 0x7f: i=0xFF;
		}
		else if(i) //lower ranges
		{
			int n = i>>1&0x3f;
			i = analog_clock%n?0:0x80;
			i|=n<<1;			
		}
				
		return i;
	}
	static int analog_divide(int i, int div)
	{	
		if(!i||div<=0) return i; assert(div<20);
				
		if(i==0x80) i = 0x7F; i&=0x7F; 
				
		if(i&1) switch(i>>div) 
		{
		case 0x01: i=analog_clock%5?0x01:0x81; break;
		case 0x03: i=analog_clock%3?0x03:0x83; break;
		case 0x07: i=analog_clock%2?0x07:0x87; break;
		case 0x0f: i=analog_clock%3?0x8f:0x0f; break;
		case 0x1f: i=analog_clock%4?0x9f:0x1f; break;
		case 0x3f: i=analog_clock%7?0xBf:0x3f; break;

		default: int n = 0;
			
			switch(i)
			{
			case 0x01: n = 5+div; break;
			case 0x03: n = 4+div; break;
			case 0x07: n = 3+div; break;
			case 0x0f: n = 2+div; break;
			case 0x1f: n = 1+div; break;
			case 0x3f: n = 0+div; break;
			case 0x7f: n = div-1; break;
			}
		    if(n<=0) return 0; //paranoia

			i = analog_clock%n?0:0x80;
			i|=n<<1;
		}		
		else //lower ranges
		{
			int n = (i>>1)+div;		
			i = analog_clock%n?0:0x80;
			i|=n<<1; 
		}

		return i;
	}  	 
	static int analog_divide(int i, float div)
	{
		return analog_divide(i,int(div+0.5f)); //round up
	}

	//WARNING: works for controller input ONLY
	//TODO? this works too, and supports lower gaits
	//return bytecode(max(gaitcode(a),gaitcode(b)));
	static int analog_select(int a, int b) 
	{
		//2021: som.hacks.cpp uses this to normalize
		//digital inputs. 0xfc and 0xfe are a hack to
		//identify XInput analog trigger buttons
		if(a>=0xFC) a = 0x80;

		//a|b works if a and b are full gaits
		if(!a||(b&~0x80)!=10<<1)
		{
			if(a^1&&b) a = 0; a|=b; //a^1??? 
		}
		return a;
	}

	//HACK: reducing 5 helps a lot with do_u2 transitions
	//from gait 2 to 3
	static float analog_scale_3; //0.5
	static float analog_scale_4; //0.666666
	static float analog_scale(int i)
	{
		switch(i&0x7F)
		{
		case 0x00: return i==0x80?1:0; 									 
		//reminder: these are tied to digital
		//screen intervals, and cannot simply
		//be changed. refer to EX::INI::GAITS
		case 0x01: return 0.2f;      //1/5 ^u2=.09
		case 0x03: return 0.333333f; //1/3 ^u2=.192+.1
		//case 0x07: return 0?0.5f:0.475f;      //1/2 ^u2=.353+.16
		case 0x07: return analog_scale_3;
		//case 0x0f: return 0.666666f; //2/3 ^u2=.544+.19
		case 0x0f: return analog_scale_4;
		case 0x1f: return 0.75f;     //3/4 //0.707106f?	
		case 0x3f: return 0.875f;    //7/8
		case 0x7f: return 1.0f;		 				
		default: //lower ranges
			
			return 1.0f/(i>>1&0x3F);		
		}
	}
	//NOT USING. USEFUL FRACTION REFERENCE?
	/*//3bit numerator and 4bit denominator 
	static float analog_scale(int i)
	{
		switch(i&0x7F)
		{
		default: assert(0);

		case 0x00: return i==0x80?1:0; 

		case 0x01: return 0.0625;    //1/16
		case 0x01: return 0.111111f; //1/9
		case 0x01: return 0.125f;    //1/8
		case 0x01: return 0.142857f; //1/7
		case 0x01: return 0.166666f; //1/6
		case 0x01: return 0.2f;      //1/5
		case 0x03: return 0.25f;     //1/4
		case 0x03: return 0.285714f; //2/7
		case 0x03: return 0.333333f; //1/3
		case 0x03: return 0.4f;      //2/5
		case 0x03: return 0.428571f; //3/7
		case 0x07: return 0.5f;      //1/2
		case 0x07: return 0.571428f; //4/7
		case 0x07: return 0.6f;      //3/5
		case 0x0f: return 0.666666f; //2/3
		case 0x0f: return 0.714285f; //5/7
		case 0x1f: return 0.75f;     //3/4
		case 0x1f: return 0.8f;      //4/5
		case 0x1f: return 0.833333f; //5/6
		case 0x1f: return 0.857141f; //6/7
		case 0x3f: return 0.875f;    //7/8
		case 0x3f: return 0.888888f; //8/9
		case 0x7f: return 1.0f;		 
		}
	}*/
	
	Joypad() : ini(0) //important
	{
		memset(this,0x00,sizeof(*this));
	}

}Joypads[EX_JOYPADS+EX_MICE]; //EX::

extern EX::Joypad &Mouse; //EX::
}

namespace EX{
extern class Pedals
{
public: bool active;

	struct Configuration //aggregate (pod)
	{
		int analog_mode[6][1]; //EX::contexts

		unsigned char pos_pos[3][1], neg_pos[3][1];	//short
		unsigned char pos_rot[3][1], neg_rot[3][1]; //short
	
		inline unsigned char *a(){	return pos_pos[0]; }

		inline size_t x()const{ return z(); }
		inline size_t z()const{ return (unsigned char*)&this[1]-pos_pos[0]; }

		void operator+=(const Configuration &c)
		{				
			int compile[sizeof(analog_mode)==6*sizeof(int)];

			for(size_t i=0,n=z();i<n;i++) //!
			{
				if(!pos_pos[i][0]) pos_pos[i][0] = c.pos_pos[i][0];
			}
			for(size_t i=0;i<6;i++)
			{
				analog_mode[i][0]+=c.analog_mode[i][0];
			}
		}

		void operator=(int zero) 
		{
			memset(this,zero,sizeof(*this));
		}		

	}cfg; 

	typedef Configuration Config;
								
	//the default configurations
	static Configuration pedscfg;

	//fyi: correction is a quaternion
	float position[6], correction[4];

	bool activate(bool _=true, bool reset=false)
	{
		if(_) cfg = pedscfg; 
		
		if(reset) memset(position,0x00,sizeof(position));
		if(reset) memset(correction,0x00,sizeof(correction));

		correction[3] = 1; return active = true;
	}
	void deactivate(){ active = false; }

	void clear(){ memset(position,0x00,sizeof(position)); }

	bool simulate(unsigned char *x, size_t x_s, float t=0, float mod=1);

	int analog[8]; //for visualization
	
	 //REMOVE ME?
	static unsigned &analog_clock;
	static const float analog_least;

	Pedals()
	{
		memset(this,0x00,sizeof(*this));

		correction[3] = 1;
	}

	static const float *calibration();

	//2020: this is the conversion logic to go from a float 
	//to a "bytecode" for external usage
	static int byteencode_scale(float, int *x80=0);
	static int gaitencode_scale(float f)
	{
		return EX::Joypad::gaitcode(byteencode_scale(f));
	}
	static int selectcode_scale(float f, int blend)
	{
		blend = EX::Joypad::gaitcode(blend);
		blend = max(blend,gaitencode_scale(f));
		return EX::Joypad::bytecode(blend);
	}

}Affects[EX_AFFECTS]; //EX::
}

#endif //EX_INPUT_INCLUDED