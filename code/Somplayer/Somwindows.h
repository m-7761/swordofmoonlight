
#ifndef SOMWINDOWS_INCLUDED
#define SOMWINDOWS_INCLUDED

#include "Somtexture.h"
#include "Somenviron.h"
#include "Somdisplay.h"

#define IDC_RESERVED (Somtexture::RESERVED-Somtexture::UNTITLED+IDC_UNTITLED)

namespace Somwindows_h
{
	struct wm_props
	{		
		HICON icon;
		HFONT font;
		HFONT logfont;
		HMENU context;

		long fontheight;
		long windowtype;
		long portholder;
		long controller;
		long resourceid;
		long consolekey;

		Somconsole *console;
		Somdisplay *display;
				
		const wchar_t *content; //windowtype
		const wchar_t *subject; //controller

		SUBCLASSPROC subclass;

		void *windowdata;

		void (*release)(void*);

		int references;

		wm_props()
		{
			memset(this,0x00,sizeof(wm_props));
		}
		wm_props(const wm_props &tearoff)
		{
			if(&tearoff) //paranoia
			{
				memcpy(this,&tearoff,sizeof(wm_props));
				
				icon = CopyIcon(tearoff.icon); font = 0;
			}
			else memset(this,0x00,sizeof(wm_props));
			
			windowdata = 0; display = 0;			
			references = 0;			
		}
		~wm_props()
		{
			//todo: context
			DestroyIcon(icon);
			DeleteObject((HGDIOBJ)font);

			if(release) release(windowdata);
						
			delete windowdata;
			delete display;
		}
	};

	struct windowman
	{	
		HWND window;

		wm_props *props;		
		
		windowman(HWND w, bool tearoff=false) 
		{		
			if(tearoff) //warning: at present you must delete props yourself
			{
				windowman wm(window=w); props = new wm_props(*wm.props); return; 
			}
			else *((HANDLE*)&props) = GetProp(window=w,"Somplayer_props"); 

			if(!props&&w&&!SetProp(w,"Somplayer_props",(HANDLE)(props=new wm_props)))
			{
				delete props; props = 0; window = 0; assert(!IsWindow(w)); return;
			}

			if(props) props->references++;
		}
		~windowman()
		{
			if(props) props->references--;
		}

		inline operator HWND(){ return window; }

		inline wm_props *operator->(){ return props; }
		
		LRESULT operator()(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam, UINT_PTR scid, DWORD_PTR)
		{
			if(props) 
			{
				if(Msg==WM_NCDESTROY) destroy(); 
			}	

			return DefSubclassProc(hwnd,Msg,wParam,lParam);
		}
		
		void usefont(HFONT cp)
		{
			//Reminder: WM_SETFONT docs say if font
			//is 0 the "system font" should be used 
			if(!props||cp==props->font||!cp) return;

			if(props->logfont==cp) return; //escewing redundancy

			LOGFONT lf; GetObject(props->logfont=cp,sizeof(lf),&lf); 

			DeleteObject(props->font); props->font = CreateFontIndirect(&lf);

			props->fontheight = abs(lf.lfHeight); //convenience

			HDC dc = GetDC(window); TEXTMETRIC tm;
			
			HGDIOBJ pop = SelectObject(dc,props->font);

			//preferring true height (ascender+descender) for now
			if(GetTextMetrics(dc,&tm)) props->fontheight = tm.tmHeight;

			SelectObject(dc,pop); ReleaseDC(window,dc);
		}
				
		inline void resize() //WM_SIZE does the bulk of our initialization
		{
			RECT client; GetClientRect(window,&client);
			SendMessage(window,WM_SIZE,SIZE_RESTORED,MAKELPARAM(client.right,client.bottom));
		}

private: friend Somconsole; //for Somconsole::view only for now

		bool subclass(const wchar_t*,int,int); //Somwindows.cpp

		bool attachto(Somconsole *c, bool lock=true)
		{
			if(!props||!props->subject&&!props->content) return false;
			
			bool out = false;
			if(lock&&!c->lock(c->key)) return false;			
			if(!props->subject||c->id(props->subject,props->controller,Somconsole::CONTROL))
			if(!props->content||c->id(props->content,props->resourceid,props->windowtype))
			{
				out = true; props->console = c; props->consolekey = c->key; 
			}				
			if(lock) c->unlock(); 			
			return out;
		}

public: Somconsole *consolelock(bool if_busy=true)
		{
			return props&&props->console->lock(props->consolekey,if_busy)?props->console:0;
		}
		
		const wchar_t *resource(bool lock, int filter=0)
		{
			if(!props||!props->content) return 0;			
			
			if(lock&&!props->console->lock(props->consolekey)) return false;			

			if(!filter) filter = props->windowtype;

			const wchar_t *out = props->console->id(props->content,props->resourceid,filter);

			if(lock) props->console->unlock(); return out;
		}

		inline const wchar_t *texture(bool lock){ return resource(lock,Somconsole::TEXTURE); }
		inline const wchar_t *palette(bool lock){ return resource(lock,Somconsole::PALETTE); }
		inline const wchar_t *picture(bool lock){ return resource(lock,Somconsole::PICTURE); }
		inline const wchar_t *control(bool lock){ return resource(lock,Somconsole::CONTROL); }
		inline const wchar_t *graphic(bool lock){ return resource(lock,Somconsole::GRAPHIC); }

		static BOOL CALLBACK adopter(HWND hello, LPARAM parent)
		{
			SetParent(hello,(HWND)parent); return TRUE;
		}
		static BOOL CALLBACK typesetter(HWND youagain, LPARAM font)
		{
			SendMessage(youagain,WM_SETFONT,(WPARAM)font,MAKELPARAM(1,0)); return TRUE;
		}		
		HWND adopt(HWND doner) //doesn't handled nested parenting
		{				
			EnumChildWindows(doner,adopter,(LPARAM)window);	

			if(!props->font) usefont((HFONT)SendMessage(doner,WM_GETFONT,0,0));

			EnumChildWindows(window,typesetter,(LPARAM)props->font);		  			

			return doner;
		}

		static BOOL CALLBACK destroyer(HWND byebye, LPARAM)
		{
			DestroyWindow(byebye); return TRUE;
		}
		void destroy(wm_props *newprops=0)
		{	
			EnumChildWindows(window,destroyer,0);			

			if(props) RemoveWindowSubclass(window,props->subclass,0); 
			if(props) props->subclass = 0;

			if(newprops==props) return;

			if(props&&props->references<=1) 
			{
				delete props; RemoveProp(window,"Somplayer_props"); 
			}	 

			if(newprops)
			{
				SetProp(window,"Somplayer_props",(HANDLE)newprops);
				newprops->references++;
			}
			
			props = newprops; 
		}

		HMENU context(int sub=-1)
		{
			if(!props) return 0;

			switch(props->windowtype)
			{
			case Somconsole::PICTURE: 
			{
				static HMENU menu = //hack: one menu for all for now
				LoadMenu(Somplayer_dll(),MAKEINTRESOURCE(IDR_PICTURE_MENU)); 				
				props->context = menu; 

				switch(sub){ case 'main': sub = 0; break; }

			}break;
			case Somconsole::TEXTURE: 
			{
				static HMENU menu = //hack: one menu for all for now
				LoadMenu(Somplayer_dll(),MAKEINTRESOURCE(IDR_TEXTURE_MENU)); 				
				props->context = menu; 

				switch(sub){ case 'main': sub = 0; break; }

			}break;
			case Somconsole::PALETTE: 
			{
				static HMENU menu = //hack: one menu for all for now
				LoadMenu(Somplayer_dll(),MAKEINTRESOURCE(IDR_PALETTE_MENU)); 				
				props->context = menu; 

				switch(sub){ case 'main': sub = 0; break; }

			}break;
			}

			if(sub!=-1) return GetSubMenu(props->context,sub);

			return props->context;
		}
	};

	struct scrollbar : public SCROLLINFO
	{			
		scrollbar() //No additional data members...
		{
			assert(sizeof(scrollbar)==sizeof(SCROLLINFO));	
			memset(this,0x00,sizeof(scrollbar));
			cbSize = sizeof(SCROLLINFO); 
		}

		bool operator()(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam) 
		{				
			extern bool Sommctrl_scroll(WPARAM,SCROLLINFO&,bool,HWND);
			return Sommctrl_scroll(wParam,*this,Msg==WM_HSCROLL,hwnd);
		}
	};

	#define SOMCONSOLE_SUPERMAN(s) \
	\
		s##man(HWND window):superman(windowman(window)){}\
		s##man(windowman &wm):superman(wm){}

	//superclass for texture/paletteman
	template <typename T> struct palfileman 
	{
		HWND window; T *image;

		static const int N = T::N;		

		palfileman<T>(windowman &wm)
		{	
			window = wm.window;

			if(wm&&wm->windowtype==N) 
			{					
				image = (T*)wm->windowdata;

				if(!image||!image->pal) open();
			}
			else image = 0;	
		}

		inline T *operator->(){ return image; }

		inline operator T*(){ return image; }
		
		void open()
		{
			close();
					
			windowman wm(window);

			Somconsole *c;					
			if(c=wm.consolelock())
			{	
				//dodge a modal deadlock
				ShowWindow(window,SW_HIDE);

				const Somtexture *pal = 
				c->pal(wm->content,wm->resourceid);

				if(!image)
				{
					wm->windowdata = image = new T(pal);
					wm->release = T::release;
				}
				else image->pal = pal;

				*image = pal->primary();

				ShowWindow(window,SW_SHOW);

				c->unlock();
			}
		}
		void close()
		{
			if(image) image->release();
		}

		typedef palfileman<T> superman;
	};

	//// textureman ////////////////////////////
		
	enum
	{	
	SQUARE=0,CIRCLE,FOURPT, //nibs  
	CURSOR=0,ERASER,PENCIL,STAMP,STENCIL,MARKER, //tools
	};

	struct tm_image
	{	
		int space;

		scrollbar scrollbars[2];

		int zoom;	
		int tool; //ERASER, PENCIL, etc
		int mode; //Somtexture::RGB...
		int file; //ID_FILE_RGB...
		
		bool wet; //debug: presenting

		struct : POINT
		{
			char shape[8][8]; 

			inline char operator()(unsigned x, unsigned y)
			{
				return x>=8||y>=8?'\0':shape[x][y]; 
			}
			
		}nib;

		POINT edit; //for Edit in context menu		
				
		RECT crop, fill; //image/client space
		
		const Somtexture *pal;

		static void release(void *me)
		{
			if(me) ((tm_image*)me)->release();
		}
		void release()
		{
			pal->release(); pal = 0;
		}

		tm_image(){}
		tm_image(const Somtexture *p)
		{
			zoom = 1;
			
			edit.x = edit.y = -1;

			space = 7; tool = CURSOR; 

			nib.x = 1; nib.shape[0][0] = 'o';
			nib.y = 1; 			
			
			wet = false; 

			SetRect(&crop,0,0,0,0); fill = crop;

			pal = p; 
		}

		void operator=(HWND window) 
		{	
			int height = pal?pal->height*zoom:0, width = pal?pal->width*zoom:0;
						
			extern void Sommctrl_texture(int,int,RECT&,int,RECT&,SCROLLINFO*,HWND);

			if(file==ID_FILE_16X16) width = height = 16*zoom;
			return Sommctrl_texture(width,height,crop,space,fill,scrollbars,window);			
		}
		int operator=(int filemode) 
		{
			switch(filemode)
			{
			case pal->RGB:	 filemode = ID_FILE_RGB; break;
			case pal->PAL:	 filemode = ID_FILE_PAL; break;
			case pal->MAP:   filemode = ID_FILE_16X16; break;
			case pal->INDEX: filemode = ID_FILE_INDEX; break;
			case pal->ALPHA: filemode = ID_FILE_ALPHA; break;
			}
			switch(file=filemode)
			{
			case ID_FILE_RGB: return mode = pal->RGB; 
			case ID_FILE_PAL: return mode = pal->PAL; 
			case ID_FILE_16X16: return mode = pal->MAP|16<<pal->MAP; 
			case ID_FILE_INDEX: return mode = pal->INDEX; 
			case ID_FILE_ALPHA: return mode = pal->ALPHA; 

			default: return mode = file = 0;
			}
		}
		operator int(){ return mode; }

		bool operator!()
		{
			return crop.left==crop.right||crop.top==crop.bottom;
		}

		enum{ N=Somconsole::TEXTURE };
	};

	struct textureman 
	:
	public palfileman<tm_image>
	{	
	SOMCONSOLE_SUPERMAN(texture)

		inline bool mote()
		{
			return image?!*image:false;
		}
		inline void crop(bool repaint=false)
		{
			if(image) *image = window; if(!repaint||!image||!window) return;

			RECT paranoia = {0,0,0,0}; //unsure if reading the docs right???
			InvalidateRect(window,0,TRUE); ValidateRect(window,&image->fill);
			RedrawWindow(window,&paranoia,0,RDW_INTERNALPAINT); 
			UpdateWindow(window);
		}	
		inline void show(int filemode)
		{
			if(!image) return;

			int a = *image, b = *image = filemode;

			crop(a||b);	//!a&&!b==stack+overflow
		}
	};

	//// paletteman ////////////////////////////

	struct pm_image
	{	
		int xor; 
		int xorselect[2];
		int selection[2];		
		int selcancel[2];

		//tonemap focus status
		bool infocus, floating;
				
		//mouse capture states
		int hit; bool dragging, x;

		scrollbar scrollbars[2]; //1
		
		const Somtexture *pal, *fresh;

		static void release(void *me)
		{
			if(me) ((pm_image*)me)->release();
		}
		void release()
		{
			pal->release(); pal = fresh = 0; 
		}

		pm_image(){}
		pm_image(const Somtexture *p)
		{ 
			memset(this,0x00,sizeof(pm_image));

			pal = p; 
		}

		void operator=(int filemode){}

		enum{ N=Somconsole::PALETTE };
	};

	struct paletteman 
	:
	public palfileman<pm_image>
	{			
	SOMCONSOLE_SUPERMAN(palette)

		bool edit(bool selection=false)
		{
			const Somtexture *pal = image?image->pal:0;

			if(selection) //tonemap selection required
			{
				if(!pal||!image->selection[0]) return false;
				if(!image->floating&&GetFocus()
				   !=GetDlgItem(window,IDC_TONE_MAP)) return false;
			}
			if(pal&&!pal->palette)
			{
				HWND tm = GetDlgItem(window,IDC_TONE_MAP);
				COLORREF c = GetWindowLong(GetDlgItem(tm,1000),GWL_USERDATA);
				pal->paint(pal->well(pal->COLORKEY),0,0,(PALETTEENTRY*)&c,0xFFFFFF);
				pal->reserve(pal->FINISHED);
				assert(pal->palette);
			}
			return pal&&pal->palette;
		}

	    void text()
		{
			wchar_t text[16] = L"0 / 0"; if(image)
			{
				const wchar_t *t = 
				image->pal->tones(image->pal->well(-1));
				int ck = image->pal->colorkey(1)?1:0, tz = t?t[1]+1:0;
				swprintf_s(text,L"%d / %d",tz?tz-ck:0,256-ck);
			}
			SetDlgItemTextW(window,IDC_TONE_MAP,text); 
		}

		void xor(int a=-1, int z=-1)
		{
			if(a>-1&&image)
			{
				image->selcancel[0] = image->selection[0] = a;
				image->selcancel[1] = image->selection[1] = z>-1?z:a;
			}

			HWND tm = GetDlgItem(window,IDC_TONE_MAP);
			
			if(GetFocus()==tm)
			{
				HWND bg = GetDlgItem(tm,IDC_BACKGROUND);
				if(bg) RedrawWindow(bg,0,0,RDW_ERASE|RDW_INVALIDATE|RDW_UPDATENOW);
			}
			else SetFocus(tm);
		}
		inline void xor(int (&az)[2])
		{
			return xor(az[0],az[1]); 
		}

		//effectively obsolete
		HWND match(HWND tone, HWND set=0); 

		bool clear(bool ok=true); //after paint/map

		static size_t clipboard_s;
		//TODO: favor win32 clipboard
		static PALETTEENTRY clipboard[256];
				
		void copy(HWND tone=0, bool cut=false); 

		inline void cut(HWND tone=0){ return copy(tone,"cut"); }

		void paste(HWND tone=0, bool ontoendofwell=false); 

		bool test(int t, bool focus=true) 
		{
			if(!image||!image->selection[0]) return false;
			
			if(focus&&!image->infocus&&!image->floating) return false;

			int a = image->selection[0], z = image->selection[1]; if(z<a) std::swap(a,z);

			if(t<1000) t+=1000; return t>=a&&t<=z;
		}

		RECT clip(int t=-1, bool focus=true)
		{							  
			RECT out = {0,0,0,0}; if(!image) return out; 

			int a = image->selection[0]-1000; if(t>=1000) t-=1000; 
			int z = image->selection[1]-1000; if(a>z) std::swap(a,z); 

			if((a<0||z<0)||t>=0&&(t<a||t>z)) a = z = t; //if outside a~z  

			if(t==a&&t==z) focus = false; if(a<0||z<0||a>255||z>255) return out; 

			out.bottom = !focus||image->infocus||image->floating?1:0; 

			out.left = a; out.right = z+1; return out;
		}
	};
	
	//// pictureman ////////////////////////////

	struct pm_video
	{
		bool wet;

		MMRESULT timer;
				
		/*is not delete'd*/
		Somdisplay *target;

		const Somenviron *camera;

		int refresh_rate; //scheduled obsolete

		static void release(void *me)
		{
			if(me) ((pm_video*)me)->release();
		}
		void release()
		{
			camera->release(); camera = 0;

			target->~Somdisplay(); 
		}

		pm_video(Somdisplay *p)
		{ 
			memset(this,0x00,sizeof(pm_video)); 

			target = p; refresh_rate = 30; //default fps			
		}

		bool focus(windowman &wm, bool lock=true)
		{
			if(!wm||!wm->portholder
			 ||lock&&!wm.consolelock()) return false;
			if(!camera->point_of_view())
			{
				camera->release();
				camera = wm->console->mpx(wm->content,wm->resourceid);
			}
			wm->console->tap(0,wm->portholder,wm->subject,wm->controller);
			bool out = wm->console->use(camera,wm->portholder);			
			if(lock) wm->console->unlock();
			return true;
		}
				
		bool refresh(windowman &wm, bool lock=true)
		{
			if(!target||wet
			  ||IsIconic(wm)
			  ||!IsWindowVisible(wm)
			  ||lock&&!wm.consolelock()) return false;
			if(!camera->point_of_view())
			{
				camera->release();
				camera = wm->console->mpx(wm->content,wm->resourceid);
			}
			bool out = wm->console->render(camera,target);
			if(out) InvalidateRect(wm,0,0);
			if(lock) wm->console->unlock();
			return out;
		}

		enum{ N=Somconsole::PICTURE };
	};

	struct pictureman
	{
		HWND window; 
		
		pm_video *video;		

		pictureman(windowman &wm)
		{	
			window = wm.window;

			if(wm&&wm->windowtype==pm_video::N) 
			{					
				video = (pm_video*)wm->windowdata;

				if(!video) open();
			}
			else video = 0;	
		}
		pictureman(HWND window)
		{					   
			windowman wm(window);

			new (this) pictureman(wm);
		}

		inline pm_video *operator->(){ return video; }

		inline operator pm_video*(){ return video; }

		bool play(int fps); //Somwindows.cpp

		inline bool pause(){ return play(0); };
		inline bool resume()
		{
			return video?play(video->refresh_rate):false; 
		};
		inline bool stop(){	return play(-1); };

		inline bool focus(bool lock=true)		
		{
			return video?video->focus(windowman(window),lock):false;
		}					   
		inline bool refresh(bool lock=true)
		{
			return video?video->refresh(windowman(window),lock):false;
		}

		void open()
		{
			close();
					
			windowman wm(window);

			Somconsole *c;					
			if(c=wm.consolelock())
			{	
				//dodge a modal deadlock
				ShowWindow(window,SW_HIDE);

				if(!video)
				{
					if(!wm->display)
					wm->display = c->display(window);
					wm->windowdata = video = new pm_video(wm->display);
					wm->release = pm_video::release;
				}

				ShowWindow(window,SW_SHOW);

				c->unlock();
			}

			play(video->refresh_rate);
		}
		void close()
		{
			if(video) video->release();

			stop();
		}	
	};

} //namespace Somwindows_h
 
#endif //SOMWINDOWS_INCLUDED