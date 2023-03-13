																		  						  
#ifndef SOMCONTROL_INCLUDED
#define SOMCONTROL_INCLUDED

#ifndef SOMCONTROL_ACTIONS
#define SOMCONTROL_ACTIONS 12 //SIXAXIS
#endif
#ifndef SOMCONTROL_BUTTONS
#define SOMCONTROL_BUTTONS 256 //keyboard
#endif
#ifndef SOMCONTROL_CONTEXTS
#define SOMCONTROL_CONTEXTS 1 //moving about
#endif
#ifndef SOMCONTROL_DEVICES
#define SOMCONTROL_DEVICES 64 //multitap ports
#endif

class Somspeaker;

namespace Somcontrol_cpp
{
	struct thread;
	struct multitap;  
	struct internals;
	struct di8device;	   	
	struct ds8device;
	struct wmmdevice;
};		   

namespace Somcontrol_h
{		
	//An action here is just a couple
	//of mutually exclusive +/- buttons
	enum{ BUTTONS=SOMCONTROL_BUTTONS };
	enum{ ACTIONS=SOMCONTROL_ACTIONS };
	
	struct button
	{			
		unsigned short id; //ID_MOVE_FORWARD
		
		button(){} button(int ID){ id = ID; }

		operator unsigned short&(){ return id; } 

		operator unsigned short()const{ return id; }

		const wchar_t *text()const; //L"Move Forward"

		button(const wchar_t *text){ *this = text; }

		button &operator=(const wchar_t *text);				
	};

	struct context //union
	{	
		//buttons may runover into actions
		button buttons[BUTTONS-ACTIONS*2];

		//actions are positive, reactions negative
		button actions[ACTIONS], reactions[ACTIONS]; 
				
		inline unsigned short buttons_id(size_t i)const
		{
			return i<BUTTONS?buttons[i].id:0;
		}
		inline const wchar_t *buttons_text(size_t i)const
		{
			return i<BUTTONS?buttons[i].text():L"Out of Range";
		}

		inline const button &operator[](size_t i)const
		{
			return buttons[i];
		}
		inline button &operator[](size_t i)
		{
			return buttons[i];
		}
	};
}

class Somcontrol
{
public:
	
	//Somplayer::id 
	long portholder; 

	//not used internally
	long port, time, post;

	//See ~Somcontrol comments
	static void tapout(Somcontrol*&p)
	{
		delete p; p = 0; //! no new []
	}

	const int tap; enum{ DISCOVER=-1 };
	//enumerate discovered input devices
	//Pass DISCOVER to discover new devices
	//DISCOVER returns the first device found
	static const Somcontrol *multitap(int i);

	friend class Somcontrol_cpp::multitap;
	friend class Somcontrol_cpp::thread; 

	//// HOW THIS WORKS ////////////////
	//
	// There is one set of input devices
	// that are accessed via multitap(i)
	// Once allocated these will not get
	// delete'd as long as Somplayer.dll
	// is loaded into memory. You make a
	// a direct copy whenever you need a 
	// configurable controller. Doing so
	// will make the master come to life
	// At the end of the copy's lifetime
	// the master controller will retire
	// Of course multiple copies coexist

	//See Somconsole::export, import, and apply
	const wchar_t *persistent_identifier()const;

	//a guaranteed non-zero description string
	//device2 returns L"" if equivalent to device
	const wchar_t *device()const, *device2()const;
	 	
	inline const wchar_t *driver()const
	{
		if(this) switch(api)
		{
		case DI8: return L"Direct Input 8";
		case DS8: return L"Direct Sound 8";
		case WMM: return L"Multimedia API"; 
		
		default: assert(0); break;
		}
		return L"";
	}

	bool power()const; //think the SIXAXIS (PS) button

	bool system()const; //a system mouse/keyboard test
		
	float pixels_per_unit()const; //dpi: defaults to 1

	bool popup_control_panel(HWND)const; //calibration
	
	Somspeaker *speakers(); //does not return an array

	//// configuration ///////////////////////////////

	enum{ CONTEXTS=SOMCONTROL_CONTEXTS };

	//CAUTION: context is not used internally
	//Ex. BUTTONS does not limit button_count
	Somcontrol_h::context contexts[CONTEXTS];

	//map a generic button to contexts[N].[re]actions[i]
	inline int contexts_button(size_t i, size_t bts=0)const
	{
		if(!bts) bts = button_count(); if(i<bts) return i; //button

		if(i%2) return (i-bts)/2+(contexts[0].actions-contexts[0].buttons);

		return (i-bts)/2+(contexts[0].reactions-contexts[0].buttons);
	}
			
	//If n goes over the button count action
	//(and reaction) labels will be returned
	const wchar_t *button_label(int n)const;
	const wchar_t *action_label(int n)const;
		
	//button_state works like button_label with [re]actions
	//action_state amounts to actions()[n] with safety checks
	float button_state(int n)const, action_state(int n)const;

	//The button count will be rounded up to be even
	size_t button_count()const, action_count()const;

	inline size_t button_count_s(size_t max=SOMCONTROL_BUTTONS)const
	{
		 return std::min(button_count(),max);
	}
	inline size_t action_count_s(size_t max=SOMCONTROL_ACTIONS)const
	{
		 return std::min(action_count(),max);
	}

	//// registration ////////////////////////////////
	//
	// These write contexts (above) and other things
	// (advanced configuration bits) to the registry
	//
	// MISSING
	// *A control points (curve filter) dialog/setup
	// *Etc.

	//open the IDD_DEVICE_TAB Export... child dialog
	//bool: don't mind it; it's to be used by apply (below) 
	//owner: MUST be non-zero otherwise export behaves like apply 
	//player: write below HKCU\Software\FROMSOFTWARE\SOM\CONFIG\player
	bool export(const wchar_t *player, HWND owner);

	//store the player's configuration in the registry 
	//player: write below HKCU\Software\FROMSOFTWARE\SOM\PLAYER\player
	inline bool apply(const wchar_t *player){ return export(player,0); }

	//open the IDD_DEVICE_TAB Import... child dialog
	//bool: don't mind it; it's to be used by reset (below) 
	//owner: MUST be non-zero otherwise import behaves like reset
	bool import(const wchar_t *player, HWND owner);

	//load the player's configuration from the registry
	//player: if 0 the settings are taken from the CONFIG\.database 
	inline bool reset(const wchar_t *player=0){ return import(player,0); }

	//// information /////////////////////////////////
	//
	// MISSING
	// *A buffered API should be around here somewhere

	//buttons state per the (above) button_count
	//buttons range from 0 to 1, actions -1 to 1
	//returns 0 whenever the _count (above) is 0
	const float *buttons()const, *actions()const;

	enum{ ANALOG=1, BINARY=2 };
	//The ANALOG bit means states are fractional
	//The BINARY bit means states are 1, 0, or -1	
	int *buttons_traits()const, *actions_traits()const;

	//// notification ////////////////////////////////
	//
	// FYI: these are more about dialogs than games

	//msg is posted to HWND with wparam set to this
	//approximately whenever the controller is moved
	int post_messages_to(HWND, int msg, void *lparam)const;
	void stop_messages_to(HWND, int post_messages_to)const;

	//// psuedo buttons //////////////////////////////
	
	//stateful: eg. See power (above)   //stateless
	enum{ IF_OFF=1, IF_ON=2, POWER=4 }; enum{ PAUSE=8, BELL=16 }; 
	
	//returns true if the button comes up
	bool press(int, int current_state=-1); //-1: predicate mode

	inline bool press(int i, bool down){ return press(i,down?1:0); }
		
	static void focus(HWND); //set focus
		
	//call for system mice and keyboards
	void capture(int ms_timeout=0)const;

	//// construction ////////////////////////////////
		
	Somcontrol(const Somcontrol &cp) : tap(0)
	{
		cp.keepalive();
		memcpy(this,&cp,sizeof(Somcontrol));
	}
	Somcontrol &operator=(const Somcontrol &cp)
	{
		cp.keepalive(); 
		Somcontrol::~Somcontrol();		
		memcpy(this,&cp,sizeof(Somcontrol));
		return *this;
	}

	//Think .~Somcontrol() with extras
	//tap is set to SOMCONTROL_DEVICES
	static const Somcontrol unplugged;

	//should return false if unplugged
	inline operator bool(){ return api; }

	inline bool unplug() //true if unplugged
	{
		return this&&*this?!(*this=unplugged):false;
	}

	//delete _pointers_ with tapout
	//instantiate with copy ctor and or =	
	protected: ~Somcontrol(); Somcontrol(int); 
		
	void keepalive()const; //monitoring

	//// implementation //////////////////////////////

	int some_internal_states;

	Somcontrol_cpp::internals *cpp;
	Somcontrol_cpp::di8device *di8;

	//placeholders 
	static Somcontrol_cpp::wmmdevice *wmm; 
	static Somcontrol_cpp::ds8device *ds8;

	//workaround: IDirectInput8::EnumDevices 
	static BOOL CALLBACK di8_callback(LPCDIDEVICEINSTANCEW,LPVOID);

	enum{ _=0, DI8, DS8, WMM }api;

	//See Somcontrol_h::stack
	Somcontrol():tap(-1){api=_;}
};

namespace Somcontrol_h
{
	//stack/member instantiation 
	struct stack : public Somcontrol
	{			
		stack(){} //See Somcontrol::Somcontrol above
		stack(const Somcontrol &cp):Somcontrol(cp){}

		inline Somcontrol &operator=(const Somcontrol &cp)
		{
			return *(Somcontrol*)this = cp;
		}	
	};
}

#endif //SOMCONTROL_INCLUDED