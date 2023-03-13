
#ifndef EX_WINDOW_INCLUDED
#define EX_WINDOW_INCLUDED

namespace EX
{
	extern HWND window;
	extern HWND client;
	extern RECT adjust;
	extern RECT coords;
	extern RECT screen; //virtual display
		
	inline HWND display(){ return window?window:client; }

	extern DWORD multihead; //display identifier

	extern bool fullscreen;

	extern DWORD style[2]; //style[fullscreen]

	extern const wchar_t *caption;

	inline bool is_visible()
	{	
		HWND hwnd = display();

		return hwnd?GetWindowStyle(hwnd)&WS_VISIBLE:true;

		//return EX::style[EX::fullscreen]&WS_VISIBLE;
	}

	//letterbox: 
	//based on current aspect ratio of hmonitor:
	//add mattes to w/h where such a display mode is available
	extern void letterbox(HMONITOR hmonitor, DWORD &w, DWORD &h); 

	//capturing_screen: 
	//nonzero hmonitor maps EX:multihead
	extern void capturing_screen(HMONITOR hmonitor=0);

	extern void creating_window(HMONITOR screen=0, int x=CW_USEDEFAULT, int y=CW_USEDEFAULT); 

	extern void showing_window();

	extern void adjusting_window(int w, int h);

	//x/y is in case returning from fullscreen
	extern void styling_window(bool fullscreen, int x, int y);

	extern void destroying_window();
}

#endif //EX_WINDOW_INCLUDED
