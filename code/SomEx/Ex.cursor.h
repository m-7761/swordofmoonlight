
#ifndef EX_CURSOR_INCLUDED
#define EX_CURSOR_INCLUDED

namespace EX
{
	extern int pointer; //0 'wait' 'z' 'hand'
	
	extern bool cursor;
	extern bool inside; //TODO: readonly
	extern bool active;
	extern bool onhold;
	extern bool crosshair; //2020

	extern HWND window; //Ex.window.h

	extern void (*timeout)(); //handler
					
	extern int x, y; //relative to client area

	inline bool is_waiting()
	{
		return EX::pointer=='wait'; 
	}

	inline bool is_captive() //assuming active
	{			
		return EX::window&&GetCapture()==EX::window;
	}

	extern bool is_cursor_hidden(); //GetCursorInfo

	//relative to virtual screen coordinates
	extern void following_cursor(int vx=-1, int vy=-1);

	extern void showing_cursor(int pointer='same');
	extern void hiding_cursor(int wait=0); //'wait'

	inline void holding_cursor(){ EX::onhold = true; }

	extern void capturing_cursor();
	extern void releasing_cursor();

	extern void clipping_cursor(int x=-1, int y=-1, int w=0, int h=0);
	extern void unclipping_cursor();

	extern void activating_cursor();
	extern void deactivating_cursor(bool hold=true);

	inline void alternating_cursor(bool hold=true)		
	{
		if(EX::active)
		{
			EX::deactivating_cursor(hold);
		}
		else EX::activating_cursor();
	}

	inline void abandoning_cursor()
	{
		EX::unclipping_cursor();
		EX::releasing_cursor();
	}
}

#endif //EX_CURSOR_INCLUDED