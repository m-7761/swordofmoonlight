
#include "directx.h" 
DX_TRANSLATION_UNIT

#include <mmsystem.h> //WAVEFORMATEX

#define DIRECTSOUND_VERSION 0x0800 //0x0700

#include "dx8.1/dsound.h"

#pragma comment(lib,"dsound.lib")

#include "dx.dsound.h"

namespace DDRAW
{
	extern HWND window;

	extern unsigned noFlips; //logging

	extern bool fullscreen;
}

static char *DSOUND::error(HRESULT err)
{
#define CASE_(E) case E: return #E;

	switch(err)
	{
	CASE_(DSOUND_UNIMPLEMENTED)

	CASE_(S_FALSE)
	CASE_(E_FAIL)
	}

	return "";

#undef CASE_
}

extern HINSTANCE DSOUND::dll = 0; 

extern int DSOUND::target = 'dx7';

extern DWORD DSOUND::listenedFor = 0;

extern IDirectSound8 *DSOUND::DirectSound8 = 0;
extern DSOUND::IDirectSound *DSOUND::DirectSound = 0;
extern DSOUND::IDirectSoundBuffer *DSOUND::PrimaryBuffer = 0;
extern DSOUND::IDirectSound3DListener *DSOUND::Listener = 0;

extern unsigned DSOUND::noStops = 0;
extern unsigned DSOUND::noLoops = 0;

extern bool DSOUND::doPiano = false;
extern bool DSOUND::doDelay = false;

extern bool DSOUND::doForward3D = false;
extern bool DSOUND::doReverb = false;
extern int DSOUND::doReverb_i3dl2[2] = {};
extern int DSOUND::doReverb_mask = 0;

static int dx_dsound_delays = 0;

static IDirectSoundBuffer **dx_dsound_delay7 = 0;

static DWORD *dx_dsound_delayz = 0;

static void dx_dsound_delay(::IDirectSoundBuffer *play, DWORD z=0)
{
	if(!play) return;

	for(int i=0;i<dx_dsound_delays;i++) 
	{
		if(play==dx_dsound_delay7[i]) return; 
	}

	static int dcap = 0;

	if(dx_dsound_delays==dcap) 
	{
		::IDirectSoundBuffer **swap = dx_dsound_delay7;

		dx_dsound_delay7 = new ::IDirectSoundBuffer*[dcap=dcap?dcap*2:32];

		DWORD *swapz = dx_dsound_delayz; dx_dsound_delayz = new DWORD[dcap];

		if(swap) memcpy(dx_dsound_delay7,swap,sizeof(void*)*dx_dsound_delays);
		if(swap) memcpy(dx_dsound_delayz,swapz,sizeof(DWORD)*dx_dsound_delays);

		delete[] swap; delete[] swapz;
	}

	dx_dsound_delay7[dx_dsound_delays] = play;
	dx_dsound_delayz[dx_dsound_delays] = z;

	dx_dsound_delays++;
}

static bool dx_dsound_delaying(::IDirectSoundBuffer *play)
{
	for(int i=0;i<dx_dsound_delays;i++) 
	{
		if(play==dx_dsound_delay7[i]) return true; 
	}

	return false;
}

static void DSOUND::releasing_buffer(::IDirectSoundBuffer *play)
{
	for(int i=0;i<dx_dsound_delays;i++)
	{
		if(play==dx_dsound_delay7[i]) dx_dsound_delay7[i] = 0;
	}
}

extern void DSOUND::playing_delays()
{
	for(int i=0;i<dx_dsound_delays;i++) if(dx_dsound_delay7[i]) 
	{
		HRESULT test = dx_dsound_delay7[i]->Play(0,0,dx_dsound_delayz[i]);
		assert(!test); //2022
	}

	dx_dsound_delays = 0;
}

static void dx_dsound_capture(DX::IDirectSoundBuffer *p, DX::IDirectSoundBuffer *q)
{
	if(!p||!q) return; 						  
	
	//assuming p identical to q except for buffer contents

	DX::DSBCAPS capsp = {sizeof(::DSBCAPS)};
	DX::DSBCAPS capsq = {sizeof(::DSBCAPS)};

	if(p->GetCaps(&capsp)!=DS_OK
	 ||q->GetCaps(&capsq)!=DS_OK) goto mismatch;

	if(capsp.dwSize!=capsq.dwSize
	 ||memcmp(&capsp,&capsq,capsp.dwSize)) goto mismatch;

	LPVOID lock1p, lock2p, lock1q, lock2q; 

	DWORD bytes1p, bytes2p, bytes1q, bytes2q; 

	DWORD n = capsp.dwBufferBytes, f = DSBLOCK_ENTIREBUFFER;

	if(p->Lock(0,n,&lock1p,&bytes1p,&lock2p,&bytes2p,f)==DS_OK)
	{
		if(q->Lock(0,n,&lock1q,&bytes1q,&lock2q,&bytes2q,f)==DS_OK)
		{
			if(lock1p&&lock1q&&bytes1p&&bytes1p==bytes1q)
			{
				memcpy(lock1q,lock1p,bytes1p);
			}
			else goto mismatch;

			//optional second pointer (unlikely for full copy)
			if(lock2p&&lock2q&&bytes2p&&bytes2p==bytes2q)
			{
				memcpy(lock2q,lock2p,bytes2p);
			}

			q->Unlock(lock1q,bytes1q,lock2q,bytes2q);	
		}
		else goto mismatch;
		
		p->Unlock(lock1p,bytes1p,lock2p,bytes2p);	
	}
	else goto mismatch; return;

mismatch: //debugging
	
	assert(0); 
}

static void *dx_dsound_enum_sets[32][3]; //warning: not thread safe

static void *dx_dsound_reserve_enum_set(void *callback, void *original_context)
{
	static bool init = true;

	if(init)
	{
		init = false;
		memset(dx_dsound_enum_sets,0x00,sizeof(void*)*3*32); 		
	}

	for(int i=0;i<32;i++) if(!dx_dsound_enum_sets[i][2])
	{
		dx_dsound_enum_sets[i][0] = callback;
		dx_dsound_enum_sets[i][1] = original_context;

		dx_dsound_enum_sets[i][2] = (void*)1;

		return (void*)i;		
	}

	return 0;
}

static void **dx_dsound_enum_set(void *new_context)
{
	if((int)new_context>=0&&(int)new_context<32) 
	if(dx_dsound_enum_sets[(int)new_context][2])
	{
		return dx_dsound_enum_sets[(int)new_context];
	}
	assert(0); return 0;
}

static void dx_dsound_return_enum_set(void *new_context)
{
	dx_dsound_enum_sets[int(new_context)][2] = 0;
}

static BOOL CALLBACK dx_dsound_enumeratecb(LPGUID guid, LPCSTR description, LPCSTR module, LPVOID passthru)
{
	DSOUND_LEVEL(7) << "Logging DirectSoundEnumerateA...\n";

	void **p = dx_dsound_enum_set(passthru); 
	
	if(!p) DSOUND_ERROR(0) << "ERROR! Passthru not setup correctly. Logging anyway.\n";
			
	DSOUND_LEVEL(7) << guid << description << '\n' << module  << '\n';

	if(p) return ((DX::LPDSENUMCALLBACKA)p[0])(guid,description,module,p[1]);

	return TRUE;
}

static DWORD dx_dsound_knockout = 0;

extern void DSOUND::knocking_out_delay(DWORD timeout_tick)
{
	dx_dsound_knockout = timeout_tick;
}

void DSOUND::IDirectSoundMaster::playfwd7(IDirectSoundBuffer *p, DWORD x, DWORD y, DWORD z)
{		
	if(target!='dx7'||!proxy7) return;
	
	if(!p||p->target!='dx7'||p->master!=this) return;

	if(!DSOUND::DirectSound||DSOUND::DirectSound->target!='dx7') return;
					  
	if(dx_dsound_knockout) //mute?
	{
		if(DX::tick()<dx_dsound_knockout) 
		{
			//DSBCAPS_CTRLVOLUME?
			//muting with SetVolume didn't work
			//anyway, ignoring the sound is even better			
			return; //forward7[i]->SetVolume(DSBVOLUME_MIN); 
		}
		else dx_dsound_knockout = 0; //!!
	}
		
	//EXPERIMENTAL
	/*"""In the DirectSound implementation, only the signal sent 
	//to the room effect is heard. To hear the direct path, play 
	//the sound simultaneously in another buffer that does not have 
	//the environmental reverb effect."""*/
	//int xx = 1&&DX::debug&&DSOUND::DirectSound8?2:1;
	int xx = DSOUND::doReverb&&DSOUND::DirectSound8?2:1;

	if(!forwarding)
	{
		forwarding = 8*xx; 
		forward7 = new ::IDirectSoundBuffer*[forwarding*2];	
		int xxx = p->in3D?2:1;
		if(xxx==2)
		forward3D7 = (::IDirectSound3DBuffer**)forward7+forwarding;	
		else assert(!forward3D7);
		memset(forward7,0x00,forwarding*xxx*sizeof(void*));
	}
		
	int i,ii = doReverb_mask&1; assert(!ii||xx==2);

	for(i=0;i<forwarding;i+=xx,ii+=xx) if(forward7[i])
	{
		DWORD status; if(isPaused&&isPaused&1<<i) continue;

		//REMIDNER: it looks like i here is the FX buffer
		//(assuming it plays longer)
		if(forward7[ii]->GetStatus(&status)==DS_OK&&status==0) 
		{
			if(!DSOUND::doDelay||!dx_dsound_delaying(forward7[ii])) break;
		}
	}
	else
	{
		bool fx = xx==2&&i%2==0; reverb_1:

		if(fx) //DuplicateSoundBuffer doesn't work on FX buffers
		{
			DSBUFFERDESC desc = {sizeof(desc)};
			WAVEFORMATEX wavf;
			DX::DSBCAPS caps = {sizeof(caps)};
			auto a = p->proxy->GetCaps(&caps);
			auto b = p->proxy->GetFormat(&wavf,sizeof(wavf),0);
			assert(!p->in3D==(caps.dwFlags&DSBCAPS_CTRL3D?0:1));
			desc.dwFlags = caps.dwFlags;
			desc.dwBufferBytes = caps.dwBufferBytes;
			desc.lpwfxFormat = &wavf;
			if(fx) //TESTING
			{
				desc.dwFlags|=DSBCAPS_CTRLFX;
				//how to calculate this? may not be enough?
				DWORD ms = wavf.wBitsPerSample*wavf.nSamplesPerSec*DSBSIZE_FX_MIN/1000;
				ms+=ms%wavf.nBlockAlign;
				//does it need extra room?
				if(1) desc.dwBufferBytes = max(ms,desc.dwBufferBytes);
				else desc.dwBufferBytes+=ms;
			}
			if(DSOUND::DirectSound8->CreateSoundBuffer(&desc,&forward7[i],0))
			{
				assert(0); return;
			}
			VOID *r,*w; DWORD rb,wb;
			if(!p->proxy->Lock(0,0,&r,&rb,0,0,DSBLOCK_ENTIREBUFFER))
			{
				if(!forward7[i]->Lock(0,0,&w,&wb,0,0,DSBLOCK_ENTIREBUFFER))
				{
					assert(rb<=wb&&wb==desc.dwBufferBytes);
					memcpy(w,r,rb);
					if(rb<wb) //DSBSIZE_FX_MIN?
					memset((char*)w+rb,0x00,wb-rb);
					forward7[i]->Unlock(w,wb,0,0);
				}
				else assert(0);
				p->proxy->Unlock(r,rb,0,0);
			}
			else assert(0); if(fx)
			{
				//REMINDER: this is really the second buffer DS reqires for reverb
				//the first buffer doesn't have DSBCAPS_CTRLFX
				auto f8 = (IDirectSoundBuffer8*)forward7[i];
				auto sz = sizeof(DSEFFECTDESC);
				DSEFFECTDESC rev[2] = {{sz},{sz}};
				rev[0].guidDSFXClass = GUID_DSFX_STANDARD_I3DL2REVERB;				
				//inaudible
				//rev[1].guidDSFXClass = GUID_DSFX_STANDARD_CHORUS;
				#ifdef NDEBUG
			//	#error testing KF2 effect (either or?)
				#endif
			//	rev[1].guidDSFXClass = GUID_DSFX_WAVES_REVERB;
				f8->SetFX(1,rev,0);
				IDirectSoundFXI3DL2Reverb *i;
				if(!f8->GetObjectInPath(GUID_DSFX_STANDARD_I3DL2REVERB,0,IID_IDirectSoundFXI3DL2Reverb,(void**)&i))
				{					
					//EXTENSION? 
					//some presets like forest and carpted hallway are virtually
					//silent at 2 but can be heard clearly at 3
					i->SetQuality(DSFX_I3DL2REVERB_QUALITY_MAX); //3 (2 is default)
					i->Release();
				}/*
				IDirectSoundFXChorus *j;
				if(!f8->GetObjectInPath(GUID_DSFX_STANDARD_CHORUS,0,IID_IDirectSoundFXChorus,(void**)&j))
				{					
					DSFXChorus p;
					j->GetAllParameters(&p);
					p.fWetDryMix = 100;
					p.fFeedback = 100;
					j->SetAllParameters(&p);
					j->Release();
				}*/
			}
		}
		else
		{
			if(DSOUND::DirectSound->proxy7->
			DuplicateSoundBuffer(proxy7,&forward7[i])!=DS_OK)
			{
				assert(0); return;
			}			

			LONG vol; //genius!!
			//Quote docs:
			//There is a known issue with volume levels of duplicated buffers. 
			//The duplicated buffer will play at full volume unless you change 
			//the volume to a different value than the original buffer's volume 
			//setting. If the volume stays the same (even if you explicitly set 
			//the same volume in the duplicated buffer with a SetVolume  call), 
			//the buffer will play at full volume regardless. To work around this
			//problem, immediately set the volume of the duplicated buffer to some-
			//thing slightly different than what it was, even if you change it one
			//millibel. The volume may then be immediately set back again to the 
			//original desired value.		
			if(p->proxy7->GetVolume(&vol)==DS_OK)
			{
				forward7[i]->SetVolume(vol-100<DSBVOLUME_MIN?vol+100:vol-100);
				forward7[i]->SetVolume(vol); 
			}
		}
		
		if(forward3D7)
		if(forward7[i]->QueryInterface(IID_IDirectSound3DBuffer,(LPVOID*)&forward3D7[i])!=DS_OK)
		{
			forward7[i]->Release(); forward7[i] = 0; assert(0); return;
		}

		if(xx==2) if(fx)
		{
			fx = false;
			i++;
			goto reverb_1;
		}
		else i--; break;
	}
	//else return;

	if(i==forwarding)
	{
		//NOTE: hitting this changing volume in som_game_11_menu41e100
		//(Options menu???)
		//also enemy sound effects in Moratheia (407f30) end game area
		//assert(0); 
		
		return; 
	}

	DSOUND_LEVEL(7) << "forwarding #" << i << '\n';

	if(xx==2) 
	{
		auto f8 = (IDirectSoundBuffer8*)forward7[i];
		IDirectSoundFXI3DL2Reverb *i;

		//TODO: blend these together
		if(~doReverb_mask&1)
		if(doReverb_i3dl2[0]
		&&!f8->GetObjectInPath(GUID_DSFX_STANDARD_I3DL2REVERB,0,IID_IDirectSoundFXI3DL2Reverb,(void**)&i))
		{
			//"In the DirectSound implementation, only the signal sent 
			//to the room effect is heard. To hear the direct path, play 
			//the sound simultaneously in another buffer that does not have 
			//the environmental reverb effect."																				 
			//DSFX_I3DL2_ENVIRONMENT_PRESET_ROOM 
			//what is DSFX_I3DL2_MATERIAL_PRESET_SINGLEWINDOW, etc?
			//NOTE: look for "DSFXI3DL2Reverb Structure" doc
			i->SetPreset(DSOUND::doReverb_i3dl2[0]);

			/*flRoomRolloffFactor seems important? if it has an effect?
			if(0&&DX::debug)
			{
				DSFXI3DL2Reverb p;
				i->GetAllParameters(&p);
				{
					//this seems to not taper off?
					//beyond 2 will clip the "hangar" preset
					//p.flDecayTime = min(p.flDecayTime*2,DSFX_I3DL2REVERB_DECAYTIME_MAX);
					//PlayStation (KF2) reverb has a very long tail compared to DS
					//MULTIPLY OR DIVIDE??? 
					//can't tell the difference???
					//p.lReverb = max(min(p.lReverb/4,DSFX_I3DL2REVERB_REVERB_MAX),DSFX_I3DL2REVERB_REVERB_MIN);
					//-1000 is almost always used
					//this seems to make a longer trail more like KF2 but with "room effect"
					//p.lRoom/=3;
					//all of the presets set this to 0? no rolloff? (room rolloff)
					if(forward3D7)
					p.flRoomRolloffFactor = 1;
				}
				i->SetAllParameters(&p);
			}*/

			i->Release();		

			goto reverb_2;
		}
	}
	else reverb_2:
	{
		assert(forward7[i]&&forward3D7[i]&&p->in3D);

		if(forward3D7)
		if(forward3D7[i])
		if(p->in3D&&p->in3D->proxy7) 
		{
			DS3DBUFFER params = {sizeof(params)};
			if(p->in3D->proxy7->GetAllParameters(&params)==DS_OK)
			forward3D7[i]->SetAllParameters(&params,DS3D_IMMEDIATE); 		
		}
		
		LONG vol; DWORD freq;
	
		if(p->proxy7->GetVolume(&vol)==DS_OK)
		{
			if(23==doReverb_i3dl2[0]) //DSFX_I3DL2_ENVIRONMENT_PRESET_UNDERWATER
			{
				//underwater? how to handle this? the reverb is very quiet? if 
				//the volume is lowered it will be more quiet... the medium is
				//different from air

				if(xx==2&&i%2) vol-=2000; //2000~3000? SetRolloff???
			}

			/*2020: shouldn't playing at the same volume not matter?
			//HACK: see above comments
			forward7[i]->SetVolume(vol-100<DSBVOLUME_MIN?vol+100:vol-100); //geez*/
			forward7[i]->SetVolume(vol);
		}
		if(p->proxy7->GetFrequency(&freq)==DS_OK)
		{
			forward7[i]->SetFrequency(freq);
		}

		if(DSOUND::doDelay)
		{
			dx_dsound_delay(forward7[i]);
		}
		else forward7[i]->Play(x,y,z);
	}
	if(xx==2&&++i%2)
	{
		if(~doReverb_mask&2) goto reverb_2;
	}
}												 

void DSOUND::IDirectSoundMaster::stopfwd7(IDirectSoundBuffer*)
{
	//Note: only seems to happen when 
	//Sword of Moonlight is shutting down
	//commenting out for now
	
	//Note: dialog box fails to materialize
	//Note: hard to trace/annoying beep
	//assert(0); //unimplemented
}

void DSOUND::IDirectSoundMaster::movefwd7(float delta[3])
{
	for(int i=forwarding;i-->0;) if(auto*fwd=forward3D7[i])
	{
		D3DVECTOR v; 
		if(!fwd->GetPosition(&v))
		fwd->SetPosition(v.x+delta[0],v.y+delta[1],v.z+delta[2],DS3D_DEFERRED);
	}
}

static void **dx_dsound_hacks = 0; 

#define DSOUND_PUSH_HACK(h,...)\
\
	void *hP,*hK;\
	for(hP=0,hK=dx_dsound_hacks?dx_dsound_hacks[DSOUND::h##_HACK]:0;hK;hK=0)hP=((void*(*)(HRESULT*,DSOUND::__VA_ARGS__))hK)

#define DSOUND_POP_HACK(h,...)\
\
	pophack: if(hP) ((void*(*)(HRESULT*,DSOUND::__VA_ARGS__))hP)

bool DSOUND::hack_interface(DSOUND::Hack hack, void *f)
{	
	if(hack<0||hack>=DSOUND::TOTAL_HACKS) return false;

	if(!dx_dsound_hacks)
	{
		if(!f) return true;
		
		dx_dsound_hacks = new void*[DSOUND::TOTAL_HACKS];

		memset(dx_dsound_hacks,0x00,sizeof(void*)*DSOUND::TOTAL_HACKS);
	}

	dx_dsound_hacks[hack] = f; return true;
}

extern void DSOUND::Yelp(const char *Interface)
{
	DSOUND_LEVEL(7) << '~' << Interface << "()\n";
}

////////////////////////////////////////////////////////
//            DIRECTSOUND INTERFACES                  //
////////////////////////////////////////////////////////

#define DSOUND_IF_NOT_TARGET_RETURN(...) if(target!='dx7') return(assert(0),__VA_ARGS__);

HRESULT DSOUND::IDirectSound::QueryInterface(REFIID riid, LPVOID FAR *ppvObj)
{
	DSOUND_LEVEL(7) << "IDirectSound::QueryInterface()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	LPOLESTR p; if(StringFromIID(riid,&p)==DS_OK) DSOUND_LEVEL(7) << ' ' << p << '\n';

	assert(0);
	
	return proxy->QueryInterface(riid,ppvObj);
}	
ULONG DSOUND::IDirectSound::AddRef() 
{
	DSOUND_LEVEL(7) << "IDirectSound::AddRef()\n";

	DSOUND_IF_NOT_TARGET_RETURN(0);

	return proxy->AddRef(); 
}
ULONG DSOUND::IDirectSound::Release()
{
	DSOUND_LEVEL(7) << "IDirectSound::Release()\n";

	DSOUND_IF_NOT_TARGET_RETURN(0);

	ULONG out = proxy->Release(); 

	if(out==0) 
	{
		DSOUND::DirectSound = 0;
		
		if(DSOUND::DirectSound8)
		{
			if(proxy!=(void*)DSOUND::DirectSound8)
			DSOUND::DirectSound8->Release();
			DSOUND::DirectSound8 = 0;
		}

		delete this; 
	}

	return out;
}
HRESULT DSOUND::IDirectSound::CreateSoundBuffer(DX::LPCDSBUFFERDESC x, DX::LPDIRECTSOUNDBUFFER *y, LPUNKNOWN z)
{
	DSOUND_LEVEL(7) << "IDirectSound::CreateSoundBuffer()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	//return proxy->CreateSoundBuffer(x,y,z); 
									
	DX::LPDIRECTSOUNDBUFFER q = 0;
	
	/*REMOVE ME
	const DX::DSBUFFERDESC *in = 0; 	
	//software mode: some experimenting 	
	if(0&&x&&x->dwFlags&DSBCAPS_CTRL3D)
	{
	  //FYI: THESE ARE IDENTICAL TO som_db'S FLAGS//

		static DX::DSBUFFERDESC X;			
		memcpy(&X,x,x->dwSize); //defaults

		//SOM_SYS.exe reports 20 (minus guid3DAlgorithm)
		//assert(x->dwSize==sizeof(DX::DSBUFFERDESC));		
		memset(&X.guid3DAlgorithm,0x00,sizeof(GUID));

		//X.dwSize = sizeof(X);		
		X.dwFlags&=~DSBCAPS_LOCHARDWARE;
		X.dwFlags|=DSBCAPS_LOCSOFTWARE;
		if((X.dwFlags&DSBCAPS_PRIMARYBUFFER)==0)
		{											
			X.dwFlags|=DSBCAPS_MUTE3DATMAXDISTANCE;

			//X.guid3DAlgorithm = DS3DALG_HRTF_FULL; //better quality

			//if(x->dwSize>=36)
			//assert(x->guid3DAlgorithm==DS3DALG_DEFAULT);			
		}		

		in = x; x = &X;
	}*/

	//many of the buffers are failing under DS8?!
	auto &flags = const_cast<DWORD&>(x->dwFlags);
	if(DSOUND::DirectSound8)
	{
		flags&=~(DSBCAPS_LOCSOFTWARE|DSBCAPS_LOCHARDWARE);
	}
	if(flags&DSBCAPS_CTRL3D)
	{
		//DS8 doesn't allow this flag since stereo isn't allowed
		//assert(1==x->lpwfxFormat->nChannels);
		flags&=~DSBCAPS_CTRLPAN;
		
		//trying things because the sound cuts out at 
		//max-distance?

		//if(DX::debug)
		//flags&=~DSBCAPS_MUTE3DATMAXDISTANCE; //testing

		//if(DX::debug) //E_INVALIDARG (okay?)
		//memcpy((void*)&x->guid3DAlgorithm,1?&DS3DALG_HRTF_FULL:&DS3DALG_HRTF_LIGHT,sizeof(GUID));
	}
	/*ADDING THIS IN playfwd7 FOR NOW
	//DuplicateSoundBuffer refuses to cooperatee with FX buffers
	bool fx = false; 
	if(0&&DX::debug&&DSOUND::DirectSound8)
	{
		if(~x->dwFlags&DSBCAPS_PRIMARYBUFFER)
		{
			fx = true;
			flags|=DSBCAPS_CTRLFX;
		}
	}*/

	HRESULT out = proxy->CreateSoundBuffer(x,y?&q:0,z);
	/*REMOVE ME
	if(out&&in) out = proxy->CreateSoundBuffer(in,y?&q:0,z);*/

	if(out) DSOUND_LEVEL(7) << "IDirectSound::CreateSoundBuffer() Failed\n"; 		
	if(out) return out; 
	
	if(DSOUND::doIDirectSoundBuffer)
	{	
		DX::LPDIRECTSOUNDBUFFER dsb = q;
		DSOUND::IDirectSoundMaster *p = new DSOUND::IDirectSoundMaster('dx7');
		
		if(x->dwFlags&DSBCAPS_PRIMARYBUFFER)
		{
			DSOUND_LEVEL(7) << " Created primary buffer\n";

			p->isPrimary = true; //don't wanna monkey with this buffer

			DSOUND::PrimaryBuffer = p;
		}

		*y = p; p->proxy = dsb;
	}
	else *y = q; 

	return out;
}
HRESULT DSOUND::IDirectSound::GetCaps(DX::LPDSCAPS x)
{
	DSOUND_LEVEL(7) << "IDirectSound::GetCaps()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->GetCaps(x); 
}
HRESULT DSOUND::IDirectSound::DuplicateSoundBuffer(DX::LPDIRECTSOUNDBUFFER x, DX::LPDIRECTSOUNDBUFFER *y)
{
	DSOUND_LEVEL(7) << "IDirectSound::DuplicateSoundBuffer()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	DSOUND::IDirectSoundBuffer *source = 0;

	if(DSOUND::doIDirectSoundBuffer) 
   	{
		source = DSOUND::is_proxy<IDirectSoundBuffer>(x);
		
		if(source) x = source->proxy;
	}

	DX::LPDIRECTSOUNDBUFFER q = 0;

	HRESULT out = proxy->DuplicateSoundBuffer(x,&q);

	if(out!=DS_OK){ DSOUND_LEVEL(7) << "IDirectSound::DuplicateSoundBuffer() Failed\n"; return out; }

	if(DSOUND::doIDirectSoundBuffer)
	{
		DX::LPDIRECTSOUNDBUFFER dsb = q;
		
		DSOUND::IDirectSoundBuffer *p = new DSOUND::IDirectSoundBuffer('dx7');

		p->master = source->master; p->isDuplicate = true; assert(p->master);

		p->master->duplicates++; DSOUND::u_dlists(source,p); 

		p->proxy = dsb; *y = p;
	}
	else *y = q; 

	LONG vol; //genius!!

	//Quote docs:
	//There is a known issue with volume levels of duplicated buffers. 
	//The duplicated buffer will play at full volume unless you change 
	//the volume to a different value than the original buffer's volume 
	//setting. If the volume stays the same (even if you explicitly set 
	//the same volume in the duplicated buffer with a SetVolume  call), 
	//the buffer will play at full volume regardless. To work around this
	//problem, immediately set the volume of the duplicated buffer to some-
	//thing slightly different than what it was, even if you change it one
	//millibel. The volume may then be immediately set back again to the 
	//original desired value.
	
	if(x&&q&&x->GetVolume(&vol)==DS_OK)
	{
		q->SetVolume(vol-100<DSBVOLUME_MIN?vol+100:vol-100); //geez		
		q->SetVolume(vol);
	}
	else assert(0); //this would be a problem!!	

	return out;
}
HRESULT DSOUND::IDirectSound::SetCooperativeLevel(HWND x, DWORD y)
{
	DSOUND_LEVEL(7) << "IDirectSound::SetCooperativeLevel()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK); 
		
	DSOUND_PUSH_HACK(DIRECTSOUND_SETCOOPERATIVELEVEL,IDirectSound*,
	HWND&,DWORD&)(0,this,x,y);
	
	//if(!x) x = DDRAW::window; assert(x); //helpful?
	if(!x||x==DDRAW::window)
	x = GetAncestor(DDRAW::window,GA_ROOT); assert(x); //helpful?
							
	assert(y==3); //Sword of Moonlight
	//trying to fix CommitDeferredSettings taking too long
	//y = 2; //no difference (1 is silent)
	//#define DSSCL_NORMAL                0x00000001
	//#define DSSCL_PRIORITY              0x00000002
	//#define DSSCL_EXCLUSIVE             0x00000003
	//#define DSSCL_WRITEPRIMARY          0x00000004
	HRESULT out = proxy->SetCooperativeLevel(x,y); assert(!out);
		
	DSOUND_POP_HACK(DIRECTSOUND_SETCOOPERATIVELEVEL,IDirectSound*,
	HWND&,DWORD&)(&out,this,x,y);

	return out;
}
HRESULT DSOUND::IDirectSound::Compact()
{
	DSOUND_LEVEL(7) << "IDirectSound::Compact()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->Compact(); 
}
HRESULT DSOUND::IDirectSound::GetSpeakerConfig(LPDWORD x)
{
	DSOUND_LEVEL(7) << "IDirectSound::GetSpeakerConfig()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->GetSpeakerConfig(x); 
}
HRESULT DSOUND::IDirectSound::SetSpeakerConfig(DWORD x)
{
	DSOUND_LEVEL(7) << "IDirectSound::SetSpeakerConfig()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->SetSpeakerConfig(x); 
}
HRESULT DSOUND::IDirectSound::Initialize(LPCGUID x)
{
	DSOUND_LEVEL(7) << "IDirectSound::Initialize()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->Initialize(x); 
}





DSOUND::IDirectSoundBuffer *DSOUND::IDirectSoundBuffer::get_head = 0; //static

HRESULT DSOUND::IDirectSoundBuffer::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	DSOUND_LEVEL(7) << "IDirectSoundBuffer::QueryInterface()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	LPOLESTR p; if(StringFromIID(riid,&p)==DS_OK) DSOUND_LEVEL(7) << ' ' << p << '\n';

	if(riid==IID_IDirectSound3DListener)
	{
		assert(isPrimary);

		DSOUND_LEVEL(7) << "(IDirectSound3DListener)\n";

		::IDirectSound3DListener *q = 0; 

		HRESULT out = proxy->QueryInterface(riid,(LPVOID*)&q); 
		
		if(out!=S_OK){ DSOUND_LEVEL(7) << "QueryInterface() Failed\n"; return out; }

		if(DSOUND::doIDirectSound3DListener)
		{							
			DSOUND::IDirectSound3DListener *p = new DSOUND::IDirectSound3DListener('dx7');

			p->proxy7 = q; *ppvObj = p; DSOUND::Listener = p;
		}
		else *ppvObj = q;

		return out;
	}
	else if(riid==IID_IDirectSound3DBuffer)
	{
		assert(!in3D);

		if(in3D) 
		{
			in3D->AddRef();	*ppvObj = in3D; return S_OK;
		}

		DSOUND_LEVEL(7) << "(IDirectSound3DBuffer)\n";

		::IDirectSound3DBuffer *q = 0; 

		HRESULT out = proxy->QueryInterface(riid,(LPVOID*)&q); 
		
		if(out!=S_OK){ DSOUND_LEVEL(7) << "QueryInterface() Failed\n"; return out; }

		if(DSOUND::doIDirectSound3DBuffer)
		{							
			DSOUND::IDirectSound3DBuffer *p = new DSOUND::IDirectSound3DBuffer('dx7');

			p->source = this; in3D = p; DSOUND::u_dlists(this,p);

			if(DSOUND::doForward3D) isForwarding = true;
			
			p->proxy7 = q; *ppvObj = p;
		}
		else *ppvObj = q;

		return out;
	}
	else assert(0);
	
	return proxy->QueryInterface(riid,ppvObj);
}	
ULONG DSOUND::IDirectSoundBuffer::AddRef() 
{
	DSOUND_LEVEL(7) << "IDirectSoundBuffer::AddRef()\n";

	DSOUND_IF_NOT_TARGET_RETURN(0);

	return proxy->AddRef(); 
}
ULONG DSOUND::IDirectSoundBuffer::Release()
{
	DSOUND_LEVEL(7) << "IDirectSoundBuffer::Release()\n";

	DSOUND_IF_NOT_TARGET_RETURN(0);

	ULONG out = proxy->Release(); 

	if(out==0)
	{			
		DSOUND::releasing_buffer(proxy7);

		master->duplicates--;

		if(master->duplicates==0)
		{			   
			if(DSOUND::doPiano) master->killpiano();

			if(DSOUND::PrimaryBuffer==master) DSOUND::PrimaryBuffer = 0;

			if(master->capture3D) master->capture3D->Release();
			if(master->capture) master->capture->Release();	   			
						
			for(int i=0;i<master->forwarding;i++)
			{
				if(master->forward3D7)
				if(master->forward3D7[i]) master->forward3D7[i]->Release();
				if(master->forward7[i]) master->forward7[i]->Release();

				DSOUND::releasing_buffer(master->forward7[i]);
			}

			delete master; 
		}	
	}

	return out;
}
HRESULT DSOUND::IDirectSoundBuffer::GetCaps(DX::LPDSBCAPS x)
{
	DSOUND_LEVEL(7) << "IDirectSoundBuffer::GetCaps()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->GetCaps(x); 
}
HRESULT DSOUND::IDirectSoundBuffer::GetCurrentPosition(LPDWORD x, LPDWORD y)
{
	DSOUND_LEVEL(7) << "IDirectSoundBuffer::GetCurrentPosition()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);
		
	HRESULT out = proxy->GetCurrentPosition(x,y); 

	if(x&&out==DS_OK) DSOUND_LEVEL(7) << ' ' << *x << '\n';	
	if(y&&out==DS_OK) DSOUND_LEVEL(7) << ' ' << *y << '\n';	

	return out;	
}
HRESULT DSOUND::IDirectSoundBuffer::GetFormat(LPWAVEFORMATEX x, DWORD y, LPDWORD z)
{
	DSOUND_LEVEL(7) << "IDirectSoundBuffer::GetFormat()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->GetFormat(x,y,z); 
}
HRESULT DSOUND::IDirectSoundBuffer::GetVolume(LPLONG x)
{
	DSOUND_LEVEL(7) << "IDirectSoundBuffer::GetVolume()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->GetVolume(x); 
}
HRESULT DSOUND::IDirectSoundBuffer::GetPan(LPLONG x)
{
	DSOUND_LEVEL(7) << "IDirectSoundBuffer::GetPan()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->GetPan(x); 
}
HRESULT DSOUND::IDirectSoundBuffer::GetFrequency(LPDWORD x)
{
	DSOUND_LEVEL(7) << "IDirectSoundBuffer::GetFrequency()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->GetFrequency(x); 
}
HRESULT DSOUND::IDirectSoundBuffer::GetStatus(LPDWORD x)
{
	DSOUND_LEVEL(7) << "IDirectSoundBuffer::GetStatus()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	if(isForwarding) 
	{
		if(x) //appear to be finished playing
		{
			*x = 0; return DS_OK;
		}
		else return !DS_OK;
	}

	if(isPaused) //appear to be playing
	{
		if(x) *x = DSBSTATUS_PLAYING;

		if(x&&isLooping) *x = DSBSTATUS_LOOPING;

		return x?DS_OK:!DS_OK;
	}

	HRESULT out = proxy->GetStatus(x); 
	
#ifdef _DEBUG
#define OUT(X) if(out==DS_OK&&x) if(*x&X) DSOUND_LEVEL(7) << ' ' << #X << '\n';

	OUT(DSBSTATUS_LOOPING)
	OUT(DSBSTATUS_PLAYING)
	OUT(DSBSTATUS_BUFFERLOST)

#undef OUT
#endif

	return out;
}
HRESULT DSOUND::IDirectSoundBuffer::Initialize(DX::LPDIRECTSOUND x, DX::LPCDSBUFFERDESC y)
{
	DSOUND_LEVEL(7) << "IDirectSoundBuffer::Initialize()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	if(DSOUND::doIDirectSound)
   	{
		DSOUND::IDirectSound *p = 0; 

		p = DSOUND::is_proxy<IDirectSound>(x);
		
		if(p) x = p->proxy;
	}

	return proxy->Initialize(x,y); 
}
HRESULT DSOUND::IDirectSoundBuffer::Lock(DWORD x, DWORD y, LPVOID *z, LPDWORD w, LPVOID *q, LPDWORD r, DWORD s)
{
	DSOUND_LEVEL(7) << "IDirectSoundBuffer::Lock()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	if(doPiano&&master) master->killpiano();

	HRESULT out = proxy->Lock(x,y,z,w,q,r,s); 

	if(out==DS_OK) isLocked = true;

	return out;
}
HRESULT DSOUND::IDirectSoundBuffer::Play(DWORD x, DWORD y, DWORD z)
{
	DSOUND_LEVEL(7) << "IDirectSoundBuffer::Play()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	if(y&DSBPLAY_LOOPING)
	{
		y = y; //breakpoint
	}

	if(master) master->wasPlayed = true;

	if(isForwarding) 
	{
		assert(master);

		if(master)
		{
			master->playfwd7(this,x,y,z);

			return DS_OK;
		}
	}

	if(DSOUND::doDelay)
	{
		dx_dsound_delay(proxy7,z); return DS_OK; //assuming
	}
	else return proxy->Play(x,y,z); 
}
HRESULT DSOUND::IDirectSoundBuffer::SetCurrentPosition(DWORD x)
{
	DSOUND_LEVEL(7) << "IDirectSoundBuffer::SetCurrentPosition()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->SetCurrentPosition(x); 
}
HRESULT DSOUND::IDirectSoundBuffer::SetFormat(LPCWAVEFORMATEX x)
{
	DSOUND_LEVEL(7) << "IDirectSoundBuffer::SetFormat()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	HRESULT ret = proxy->SetFormat(x); 

	return ret;
}
HRESULT DSOUND::IDirectSoundBuffer::SetVolume(LONG x)
{
	DSOUND_LEVEL(7) << "IDirectSoundBuffer::SetVolume()\n";

	DSOUND_LEVEL(7) << ' ' << x << '\n';

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	DSOUND_PUSH_HACK(DIRECTSOUNDBUFFER_SETVOLUME,IDirectSoundBuffer*,
	LONG&)(0,this,x);

	HRESULT out = proxy->SetVolume(x); 

	DSOUND_POP_HACK(DIRECTSOUNDBUFFER_SETVOLUME,IDirectSoundBuffer*,
	LONG&)(&out,this,x);

	return out;
}
HRESULT DSOUND::IDirectSoundBuffer::SetPan(LONG x)
{
	DSOUND_LEVEL(7) << "IDirectSoundBuffer::SetPan()\n";

	DSOUND_LEVEL(7) << ' ' << x << '\n';

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->SetPan(x); 
}
HRESULT DSOUND::IDirectSoundBuffer::SetFrequency(DWORD x)
{
	DSOUND_LEVEL(7) << "IDirectSoundBuffer::SetFrequency()\n";

	DSOUND_LEVEL(7) << ' ' << x << '\n';

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	DSOUND_PUSH_HACK(DIRECTSOUNDBUFFER_SETFREQUENCY,IDirectSoundBuffer*,
	DWORD&)(0,this,x);

	if(master) //??? //DSOUND::Piano only?
	{	
		float current = (int)master->frequency;
		
		float samples = master->frequency-current;
		
		if(samples==0.0f)
		{
			if(current==0.0f)
			{	
				master->frequency = x; //first sample			
			}
			else master->frequency = 0.5f+int((current+x)/2.0f); //second sample
		}
		else //subsequent samples
		{
			samples = floorf(1.0f/samples+0.5f); //round about way of rounding
			
			if(samples<100) 
			{
				master->frequency = 1.0f/(samples+1)+int((current*samples+x)/(samples+1));	
			}
			else master->frequency = current; //reset
		}
	}
	
	HRESULT out = proxy->SetFrequency(x);

	DSOUND_POP_HACK(DIRECTSOUNDBUFFER_SETFREQUENCY,IDirectSoundBuffer*,
	DWORD&)(&out,this,x);

	return out;
}
HRESULT DSOUND::IDirectSoundBuffer::Stop()
{
	DSOUND_LEVEL(7) << "IDirectSoundBuffer::Stop()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	if(isForwarding) 
	{
		assert(master);

		if(master)
		{
			master->stopfwd7(this);

			return DS_OK;
		}
	}

	return proxy->Stop(); 
}
HRESULT DSOUND::IDirectSoundBuffer::Unlock(LPVOID x, DWORD y, LPVOID z, DWORD w)
{
	DSOUND_LEVEL(7) << "IDirectSoundBuffer::Unlock()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	if(DSOUND::doPiano&&master) master->killpiano(); //should never occur

	HRESULT out = proxy->Unlock(x,y,z,w); 

	if(out==DS_OK)
	{	
		if(master)
		if(DSOUND::doPiano)		
		if(!isPrimary&&DSOUND::DirectSound)		
		{	
			if(!master->capture)
			{									
				DX::DSBCAPS caps = {sizeof(caps)};

				WAVEFORMATEX wav; wav.cbSize = sizeof(wav);

				if(proxy->GetCaps(&caps)==DS_OK
				  &&proxy->GetFormat(&wav,wav.cbSize,0)==DS_OK)
				{
					DX::DSBUFFERDESC desc = {sizeof(desc)};

					//assuming meaningful
					desc.dwFlags = caps.dwFlags;
					desc.dwBufferBytes = caps.dwBufferBytes;
					desc.lpwfxFormat = &wav;
					desc.guid3DAlgorithm = DS3DALG_DEFAULT;
					desc.dwReserved = 0;	 
									
					if(DSOUND::DirectSound->proxy->
					CreateSoundBuffer(&desc,&master->capture,0)==DS_OK)
					if(master->capture->QueryInterface
					(IID_IDirectSound3DBuffer,(LPVOID*)&master->capture3D)==DS_OK)
					{
						master->capture3D->SetMinDistance(1000,DS3D_IMMEDIATE);
						master->capture3D->SetMaxDistance(DS3D_DEFAULTMAXDISTANCE,DS3D_IMMEDIATE);
						master->capture3D->SetMode(DS3DMODE_DISABLE,DS3D_IMMEDIATE);						
					}
				}
			}

			if(master->capture)
			dx_dsound_capture(proxy,master->capture); 			
		}

		isLocked = false;		
	}

	return out;
}
HRESULT DSOUND::IDirectSoundBuffer::Restore()
{
	DSOUND_LEVEL(7) << "IDirectSoundBuffer::Restore()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->Restore(); 
}






HRESULT DSOUND::IDirectSound3DListener::QueryInterface(REFIID riid, LPVOID FAR *ppvObj)
{
	DSOUND_LEVEL(7) << "IDirectSound3DListener::QueryInterface()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	LPOLESTR p; if(StringFromIID(riid,&p)==DS_OK) DSOUND_LEVEL(7) << ' ' << p << '\n';

	assert(0);
	
	return proxy->QueryInterface(riid,ppvObj);
}	
ULONG DSOUND::IDirectSound3DListener::AddRef() 
{
	DSOUND_LEVEL(7) << "IDirectSound3DListener::AddRef()\n";

	DSOUND_IF_NOT_TARGET_RETURN(0);

	return proxy->AddRef(); 
}
ULONG DSOUND::IDirectSound3DListener::Release()
{
	DSOUND_LEVEL(7) << "IDirectSound3DListener::Release()\n";

	DSOUND_IF_NOT_TARGET_RETURN(0);

	ULONG out = proxy->Release(); 

	if(out==0)
	{
		DSOUND::Listener = 0;

		delete this; 
	}

	return out;
}
HRESULT DSOUND::IDirectSound3DListener::GetAllParameters(DX::LPDS3DLISTENER x)
{
	DSOUND_LEVEL(7) << "IDirectSound3DListener::GetAllParameters()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->GetAllParameters(x);
}
HRESULT DSOUND::IDirectSound3DListener::GetDistanceFactor(DX::D3DVALUE *x)
{
	DSOUND_LEVEL(7) << "IDirectSound3DListener::GetDistanceFactor()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->GetDistanceFactor(x);
}
HRESULT DSOUND::IDirectSound3DListener::GetDopplerFactor(DX::D3DVALUE *x)
{
	DSOUND_LEVEL(7) << "IDirectSound3DListener::GetDopplerFactor()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->GetDopplerFactor(x);
}
HRESULT DSOUND::IDirectSound3DListener::GetOrientation(DX::D3DVECTOR *x, DX::D3DVECTOR *y)
{
	DSOUND_LEVEL(7) << "IDirectSound3DListener::GetOrientation()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->GetOrientation(x,y);
}
HRESULT DSOUND::IDirectSound3DListener::GetPosition(DX::D3DVECTOR *x)
{
	DSOUND_LEVEL(7) << "IDirectSound3DListener::GetPosition()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->GetPosition(x);
}
HRESULT DSOUND::IDirectSound3DListener::GetRolloffFactor(DX::D3DVALUE *x)
{
	DSOUND_LEVEL(7) << "IDirectSound3DListener::GetRolloffFactor()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->GetRolloffFactor(x);
}
HRESULT DSOUND::IDirectSound3DListener::GetVelocity(DX::D3DVECTOR *x)
{
	DSOUND_LEVEL(7) << "IDirectSound3DListener::GetVelocity()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->GetVelocity(x);
}
HRESULT DSOUND::IDirectSound3DListener::SetAllParameters(DX::LPCDS3DLISTENER x, DWORD y)
{
	DSOUND_LEVEL(7) << "IDirectSound3DListener::SetAllParameters()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	if(deaf>deaf_s) DSOUND_RETURN(S_OK)

	return proxy->SetAllParameters(x,y);
}
HRESULT DSOUND::IDirectSound3DListener::SetDistanceFactor(DX::D3DVALUE x, DWORD y)
{
	DSOUND_LEVEL(7) << "IDirectSound3DListener::SetDistanceFactor()\n";

	DSOUND_LEVEL(7) << ' ' << x << '\n';

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	if(deaf>deaf_s) DSOUND_RETURN(S_OK)

	assert(0);

	return proxy->SetDistanceFactor(x,y);
}
HRESULT DSOUND::IDirectSound3DListener::SetDopplerFactor(DX::D3DVALUE x, DWORD y)
{
	DSOUND_LEVEL(7) << "IDirectSound3DListener::SetDopplerFactor()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	if(deaf>deaf_s) DSOUND_RETURN(S_OK)

	assert(0);

	return proxy->SetDopplerFactor(x,y);
}
HRESULT DSOUND::IDirectSound3DListener::SetOrientation(DX::D3DVALUE x, DX::D3DVALUE y, DX::D3DVALUE z, DX::D3DVALUE w, DX::D3DVALUE q, DX::D3DVALUE r, DWORD s)
{
	DSOUND_LEVEL(7) << "IDirectSound3DListener::SetOrientation()\n";

	DSOUND_LEVEL(7) << ' ' << x << ' ' << y << ' ' << z << ' ' << w << ' ' << q << ' ' << r << '\n';

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);
		
	DSOUND_PUSH_HACK(DIRECTSOUND3DLISTENER_SETORIENTATION,IDirectSound3DListener*,
	DX::D3DVALUE&,DX::D3DVALUE&,DX::D3DVALUE&,DX::D3DVALUE&,DX::D3DVALUE&,DX::D3DVALUE&,DWORD&)(0,this,x,y,z,w,q,r,s);

	HRESULT out = S_OK;

	if(deaf>deaf_s) DSOUND_FINISH(S_OK)

	out = proxy->SetOrientation(x,y,z,w,q,r,s);

	DSOUND_POP_HACK(DIRECTSOUND3DLISTENER_SETORIENTATION,IDirectSound3DListener*,
	DX::D3DVALUE&,DX::D3DVALUE&,DX::D3DVALUE&,DX::D3DVALUE&,DX::D3DVALUE&,DX::D3DVALUE&,DWORD&)(&out,this,x,y,z,w,q,r,s);

	return out;
}
HRESULT DSOUND::IDirectSound3DListener::SetPosition(DX::D3DVALUE x, DX::D3DVALUE y, DX::D3DVALUE z, DWORD w)
{
	DSOUND_LEVEL(7) << "IDirectSound3DListener::SetPosition()\n";

	DSOUND_LEVEL(7) << ' ' << x << ' ' << y << ' ' << z << '\n';

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	DSOUND_PUSH_HACK(DIRECTSOUND3DLISTENER_SETPOSITION,IDirectSound3DListener*,
	DX::D3DVALUE&,DX::D3DVALUE&,DX::D3DVALUE&,DWORD&)(0,this,x,y,z,w);

	HRESULT out = S_OK;

	if(deaf>deaf_s) DSOUND_FINISH(S_OK)

	out = proxy->SetPosition(x,y,z,w);

	DSOUND_POP_HACK(DIRECTSOUND3DLISTENER_SETPOSITION,IDirectSound3DListener*,
	DX::D3DVALUE&,DX::D3DVALUE&,DX::D3DVALUE&,DWORD&)(&out,this,x,y,z,w);

	return out;
}
HRESULT DSOUND::IDirectSound3DListener::SetRolloffFactor(DX::D3DVALUE x, DWORD y)
{
	DSOUND_LEVEL(7) << "IDirectSound3DListener::SetRolloffFactor()\n";

	DSOUND_LEVEL(7) << ' ' << x << '\n';

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	if(deaf>deaf_s) DSOUND_RETURN(S_OK)

	return proxy->SetRolloffFactor(x,y);
}
HRESULT DSOUND::IDirectSound3DListener::SetVelocity(DX::D3DVALUE x, DX::D3DVALUE y, DX::D3DVALUE z, DWORD w)
{
	DSOUND_LEVEL(7) << "IDirectSound3DListener::SetVelocity()\n";

	DSOUND_LEVEL(7) << ' ' << x << ' ' << y << ' ' << z << '\n';

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);
		
	DSOUND_PUSH_HACK(DIRECTSOUND3DLISTENER_SETVELOCITY,IDirectSound3DListener*,
	DX::D3DVALUE&,DX::D3DVALUE&,DX::D3DVALUE&,DWORD&)(0,this,x,y,z,w);

	HRESULT out = S_OK;

	if(deaf>deaf_s) DSOUND_FINISH(S_OK)

	out = proxy->SetVelocity(x,y,z,w);

	DSOUND_POP_HACK(DIRECTSOUND3DLISTENER_SETVELOCITY,IDirectSound3DListener*,
	DX::D3DVALUE&,DX::D3DVALUE&,DX::D3DVALUE&,DWORD&)(&out,this,x,y,z,w);

	return out;
}
static DWORD WINAPI dx_dsound_CommitDeferredSettingsProc(LPVOID listener) 
{
	HRESULT error =
	((IDirectSound3DListener*)listener)->CommitDeferredSettings();
	if(error==DSERR_BUFFERLOST) //2021???
	{
		//default device was unplugged... what can be done?
		//anyway, don't assert
	}
	else if(error)
	{
		assert(0);
		DSOUND_LEVEL(7) << "dx_dsound_CommitDeferredSettingsProc() Failed: "<<error<<"\n";	
	}
	return 0;
}
HRESULT DSOUND::IDirectSound3DListener::CommitDeferredSettings()
{		
	DSOUND_LEVEL(7) << "IDirectSound3DListener::CommitDeferredSettings()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);
	
	//DSOUND::listenerInterval?
	//static int x = 0; if(x++%8) return DS_OK;

	if(deaf>deaf_s) DSOUND_RETURN(S_OK)

	if(DSOUNDLOG::debug<=7)
	{
		static unsigned debug = DDRAW::noFlips;

		if(debug!=DDRAW::noFlips)
		{
			DSOUND_LEVEL(7) << "=========================================================\n";

			debug = DDRAW::noFlips;
		}
	}	 

	//EXPERIMENTAL
	//2017: Call CommitDeferredSettings in a separate thread. For whatever
	//reason this API has always taken multiple milliseconds, sometimes it
	//has more time than anything else (up to 8 or more millisecons for 60
	//frames-per-second is more than half of the frame.)
	//This (naive) approach seems to work. I haven't confirmed if it makes
	//a different. It could make a big difference. An extension would help
	//to benchmark or disable it if it acts up (some synchronization might
	//be helpful.)
	//(NOTE: The frame rate may be GPU bound. Especially for slimmer GPUs.)
	if(1!=DX::central_processing_units) //Give players an out if necessary.
	{
		HANDLE h = 
		CreateThread(0,0,dx_dsound_CommitDeferredSettingsProc,proxy,CREATE_SUSPENDED,0);
		SetThreadPriority(h,THREAD_PRIORITY_TIME_CRITICAL);
		ResumeThread(h);
		CloseHandle(h);
		return DS_OK;
	}

	DWORD test = timeGetTime();

	HRESULT out = proxy->CommitDeferredSettings();
	//			
	//TODO: It seems like DirectSound should happen in a different thread.
	//CommitDeferredSettings is basically working on the CPU at this time.
	//
	DWORD ms = timeGetTime()-test; if(ms>8) 
	{
		if(deaf==5)	//0: spamming the log
		{
			DSOUND_ALERT(0) << "ALERT! Sound card is unresponsive around IDirectSound3DListener::CommitDeferredSettings\n";
		}

		static int db = MB_DEFBUTTON3; //NEW

		if(db) if(deaf++==deaf_s) 
		{			
			wchar_t app[MAX_PATH] = L""; 
			GetWindowTextW(DDRAW::window,app,MAX_PATH);									 
			switch(MessageBoxW(DDRAW::window, //MB_OK
			L"3D location is being disabled for your Default sound device because it has been determined to be unresponsive.\n"
			L"\n"			
			L"This can be a known Windows bug. The device and or Windows is refusing to operate in software mode. Which is generally recommended.\n"
			L"\n"			
			L"You are advised to use another device or run \"dxdiag\" to see if Windows will allow you to force the device into software mode.\n"			
			L"\n"		
			L"It may be neccessary to restart the game for a new sound setup to take effect.\n"		
			,app,MB_CANCELTRYCONTINUE|MB_ICONWARNING|db))
			{
			case IDCANCEL: deaf = 0; db = 0; break; 
			case IDTRYAGAIN: deaf = 0; db = MB_DEFBUTTON2; break;
			}
		}
	}
	else deaf = 0;

	DSOUND::listenedFor = ms;

	return out;
}						





HRESULT DSOUND::IDirectSound3DBuffer::QueryInterface(REFIID riid, LPVOID FAR *ppvObj)
{
	DSOUND_LEVEL(7) << "IDirectSound3DBuffer::QueryInterface()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	LPOLESTR p; if(StringFromIID(riid,&p)==DS_OK) DSOUND_LEVEL(7) << ' ' << p << '\n';

	assert(0);

	return proxy->QueryInterface(riid,ppvObj);
}	
ULONG DSOUND::IDirectSound3DBuffer::AddRef() 
{
	DSOUND_LEVEL(7) << "IDirectSound3DBuffer::AddRef()\n";

	DSOUND_IF_NOT_TARGET_RETURN(0);

	return proxy->AddRef(); 
}
ULONG DSOUND::IDirectSound3DBuffer::Release()
{
	DSOUND_LEVEL(7) << "IDirectSound3DBuffer::Release()\n";

	DSOUND_IF_NOT_TARGET_RETURN(0);

	ULONG out = proxy->Release(); 

	if(out==0)
	{
		source->in3D = 0; source = 0;
	}

	return out;
}
HRESULT DSOUND::IDirectSound3DBuffer::GetAllParameters(DX::LPDS3DBUFFER x)
{
	DSOUND_LEVEL(7) << "IDirectSound3DBuffer::GetAllParameters()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->GetAllParameters(x);
}
HRESULT DSOUND::IDirectSound3DBuffer::GetConeAngles(LPDWORD x, LPDWORD y)
{
	DSOUND_LEVEL(7) << "IDirectSound3DBuffer::GetConeAngles()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->GetConeAngles(x,y);
}
HRESULT DSOUND::IDirectSound3DBuffer::GetConeOrientation(DX::D3DVECTOR* x)
{
	DSOUND_LEVEL(7) << "IDirectSound3DBuffer::GetConeOrientation()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->GetConeOrientation(x);
}
HRESULT DSOUND::IDirectSound3DBuffer::GetConeOutsideVolume(LPLONG x)
{
	DSOUND_LEVEL(7) << "IDirectSound3DBuffer::GetConeOutsideVolume()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->GetConeOutsideVolume(x);
}
HRESULT DSOUND::IDirectSound3DBuffer::GetMaxDistance(DX::D3DVALUE* x)
{
	DSOUND_LEVEL(7) << "IDirectSound3DBuffer::GetMaxDistance()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->GetMaxDistance(x);
}
HRESULT DSOUND::IDirectSound3DBuffer::GetMinDistance(DX::D3DVALUE* x)
{
	DSOUND_LEVEL(7) << "IDirectSound3DBuffer::GetMinDistance()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->GetMinDistance(x);
}
HRESULT DSOUND::IDirectSound3DBuffer::GetMode(LPDWORD x)
{
	DSOUND_LEVEL(7) << "IDirectSound3DBuffer::GetMode()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->GetMode(x);
}
HRESULT DSOUND::IDirectSound3DBuffer::GetPosition(DX::D3DVECTOR* x)
{
	DSOUND_LEVEL(7) << "IDirectSound3DBuffer::GetPosition()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->GetPosition(x);
}
HRESULT DSOUND::IDirectSound3DBuffer::GetVelocity(DX::D3DVECTOR* x)
{
	DSOUND_LEVEL(7) << "IDirectSound3DBuffer::GetVelocity()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->GetVelocity(x);
}
HRESULT DSOUND::IDirectSound3DBuffer::SetAllParameters(DX::LPCDS3DBUFFER x, DWORD y)
{
	DSOUND_LEVEL(7) << "IDirectSound3DBuffer::SetAllParameters()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->SetAllParameters(x,y);
}
HRESULT DSOUND::IDirectSound3DBuffer::SetConeAngles(DWORD x, DWORD y, DWORD z)
{
	DSOUND_LEVEL(7) << "IDirectSound3DBuffer::SetConeAngles()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->SetConeAngles(x,y,z);
}
HRESULT DSOUND::IDirectSound3DBuffer::SetConeOrientation(DX::D3DVALUE x, DX::D3DVALUE y, DX::D3DVALUE z, DWORD w)
{
	DSOUND_LEVEL(7) << "IDirectSound3DBuffer::SetConeOrientation()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->SetConeOrientation(x,y,z,w);
}
HRESULT DSOUND::IDirectSound3DBuffer::SetConeOutsideVolume(LONG x, DWORD y)
{
	DSOUND_LEVEL(7) << "IDirectSound3DBuffer::SetConeOutsideVolume()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->SetConeOutsideVolume(x,y);
}
HRESULT DSOUND::IDirectSound3DBuffer::SetMaxDistance(DX::D3DVALUE x, DWORD y)
{
	DSOUND_LEVEL(7) << "IDirectSound3DBuffer::SetMaxDistance()\n";

	DSOUND_LEVEL(7) << ' ' << x << '\n';

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->SetMaxDistance(x,y);
}
HRESULT DSOUND::IDirectSound3DBuffer::SetMinDistance(DX::D3DVALUE x, DWORD y)
{
	DSOUND_LEVEL(7) << "IDirectSound3DBuffer::SetMinDistance()\n";

	DSOUND_LEVEL(7) << ' ' << x << '\n';

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->SetMinDistance(x,y);
}
HRESULT DSOUND::IDirectSound3DBuffer::SetMode(DWORD x, DWORD y)
{
	DSOUND_LEVEL(7) << "IDirectSound3DBuffer::SetMode()\n";

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

#ifdef _DEBUG
#define OUT(X) if(x==X) DSOUND_LEVEL(7) << ' ' << #X << '\n';

	OUT(DS3DMODE_NORMAL)
	OUT(DS3DMODE_HEADRELATIVE)
	OUT(DS3DMODE_DISABLE)

#undef OUT
#endif

	return proxy->SetMode(x,y);
}
HRESULT DSOUND::IDirectSound3DBuffer::SetPosition(DX::D3DVALUE x, DX::D3DVALUE y, DX::D3DVALUE z, DWORD w)
{
	DSOUND_LEVEL(7) << "IDirectSound3DBuffer::SetPosition()\n";

	DSOUND_LEVEL(7) << ' ' << x << ' ' << y << ' ' << z << '\n';

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK); 

	DSOUND_PUSH_HACK(DIRECTSOUND3DBUFFER_SETPOSITION,IDirectSound3DBuffer*,
	DX::D3DVALUE&,DX::D3DVALUE&,DX::D3DVALUE&,DWORD&)(0,this,x,y,z,w);

	HRESULT out = proxy->SetPosition(x,y,z,w);

	DSOUND_POP_HACK(DIRECTSOUND3DBUFFER_SETPOSITION,IDirectSound3DBuffer*,
	DX::D3DVALUE&,DX::D3DVALUE&,DX::D3DVALUE&,DWORD&)(&out,this,x,y,z,w);

	return out;
}
HRESULT DSOUND::IDirectSound3DBuffer::SetVelocity(DX::D3DVALUE x, DX::D3DVALUE y, DX::D3DVALUE z, DWORD w)
{
	DSOUND_LEVEL(7) << "IDirectSound3DBuffer::SetVelocity()\n";

	DSOUND_LEVEL(7) << ' ' << x << ' ' << y << ' ' << z << '\n';

	DSOUND_IF_NOT_TARGET_RETURN(!DS_OK);

	return proxy->SetVelocity(x,y,z,w);
}






extern void DSOUND::Stop(bool looping)
{
	DSOUND::noStops++;

	if(looping) DSOUND::noLoops++;

	if(!DSOUND::doIDirectSoundBuffer) return;

	DSOUND::IDirectSoundBuffer *head = 0, *stop = 0, *p = 0;
	
	head = DSOUND::IDirectSoundBuffer::get_head; 
		
	DWORD status; if(!head) return;

	for(p=head;p&&p!=stop;p=p->get_next) 
	{
		stop = head;

		if(!p->isPrimary)
		if(DSOUND::doForward3D&&p->isForwarding)
		{
			if(p->isDuplicate) continue;

			assert(p->master==p);

			if(p->master!=p||!p->master->forwarding) continue;
			
			for(int i=0;i<p->master->forwarding;i++) if(p->master->forward7[i])
			{
				HRESULT ok = DS_OK;

				if(p->isPaused&1<<i)
				{
					status = DSBSTATUS_PLAYING;

					if(p->isLooping&1<<i) status|=DSBSTATUS_LOOPING;					
				}
				else ok = p->master->forward7[i]->GetStatus(&status); 				

				if(ok==DS_OK)
				if(status&DSBSTATUS_PLAYING) 		
				if(looping||(status&DSBSTATUS_LOOPING)==0)
				{
					if(p->isPaused&1<<i)					
					{
						if(!p->master->pausefwds)
						{
							p->master->pausefwds = new char[p->master->forwarding];

							for(int i=0;i<p->master->forwarding;i++)
							{
								p->master->pausefwds[i] = p->isPaused&1<<i?1:0;
							}
						}

						p->master->pausefwds[i]++;
					}
					else if(p->master->forward7[i]->Stop()==DS_OK)
					{
						p->isPaused|=1<<i;

						if(status&DSBSTATUS_LOOPING)
						{
							p->isLooping|=1<<i;
						}
					}									
				}
			}			
		}
		else 
		{
			if(p->pauserefs)
			{
				status = DSBSTATUS_PLAYING;

				if(p->isLooping) status|=DSBSTATUS_LOOPING;
			}

			if(p->pauserefs||S_OK==p->proxy->GetStatus(&status))
			{
				if(status&DSBSTATUS_PLAYING)	
				if(looping||(status&DSBSTATUS_LOOPING)==0)
				{
					if(!p->pauserefs)
					{
						if(p->proxy->Stop()==DS_OK) 
						{
							p->isPaused = true;	p->pauserefs = 1;
						}
					}
					else p->pauserefs++;
				}

				if(p->isPaused) p->isLooping = status&DSBSTATUS_LOOPING;
			}
		}
	}
}
extern void DSOUND::Play(bool looping)
{
	assert(DSOUND::noStops);

	if(!DSOUND::noStops) return;

	DSOUND::noStops--;		

	if(looping)
	{
		assert(DSOUND::noLoops);

		DSOUND::noLoops--;
	}

	if(!DSOUND::doIDirectSoundBuffer) return;

	DSOUND::IDirectSoundBuffer *head = 0, *stop = 0, *p = 0;
	
	head = DSOUND::IDirectSoundBuffer::get_head; if(!head) return;

	for(p=head;p&&p!=stop;p=p->get_next)
	{	
		stop = head;

		if(!p->isPrimary)
		if(DSOUND::doForward3D&&p->isForwarding)
		{
			if(p->isDuplicate) continue;

			assert(p->master==p);

			if(p->master!=p||!p->master->forwarding) continue;
						
			for(int i=0;i<p->master->forwarding;i++) if(p->master->forward7[i])
			{
				if(p->isLooping&&looping) continue; 

				//Woops: shadowing looping argument
				int looping = p->isLooping&(1<<i)?DSBPLAY_LOOPING:0;
				
				if(p->isPaused&(1<<i))
				if(!p->master->pausefwds||p->master->pausefwds[i]<2)
				{
					if(p->master->pausefwds) p->master->pausefwds[i]--;

					if(p->master->forward7[i]->Play(0,0,looping)==DSERR_BUFFERLOST)
					{
						DSOUND_ALERT(0) << "DirectSoundBuffer contents were lost!!\n";

						p->master->forward7[i]->Restore(); 					
						p->master->forward7[i]->Play(0,0,looping);
					}

					p->isLooping&=~(1<<i);
					p->isPaused&=~(1<<i);
				}
				else p->master->pausefwds[i]--;				
			}	
			
			if(!DSOUND::noStops) //paranoia
			{
				assert(!p->isPaused&&!p->isLooping);

				if(p->master->pausefwds) 
				{
					memset(p->master->pausefwds,0x00,p->master->forwarding);
				}

				p->isPaused = p->isLooping = 0x00000000;
			}
		}
		else 
		{
			if(p->isLooping&&!looping) continue; 

			//Woops: shadowing looping argument
			int looping = p->isLooping?DSBPLAY_LOOPING:0;			

			if(p->isPaused)
			if(p->pauserefs<2)
			{
				p->pauserefs--; assert(p->pauserefs==0); 
				
				if(p->proxy->Play(0,0,looping)==DSERR_BUFFERLOST)
				{
					DSOUND_ALERT(0) << "DirectSoundBuffer contents were lost!!\n";

					p->proxy->Restore(); p->proxy->Play(0,0,looping);
				}

				p->isPaused = p->isLooping = false; 

				p->pauserefs = 0; //paranoia
			}
			else p->pauserefs--;
		}
	}	
}
extern void DSOUND::Sync(LONG mBs, LONG mBs3D)
{
	LONG vol, add; if(!mBs&&!mBs3D) return;
	
	DSOUND::IDirectSoundBuffer *head = 0, *stop = 0, *p = 0;
	
	head = DSOUND::IDirectSoundBuffer::get_head; if(!head) return;

	for(p=head;p&&p!=stop;p=p->get_next)
	{	
		stop = head; if(!p->isPaused) continue;

		add = p->in3D?mBs3D:mBs; if(add==0) continue;		

		if(DSOUND::doForward3D&&p->isForwarding)
		{
			if(p->isDuplicate) continue; assert(p->master==p);

			if(p->master!=p||!p->master->forwarding) continue;
						
			for(int i=0;i<p->master->forwarding;i++) 				
			if(p->isPaused&(1<<i)&&p->master->forward7[i])
			{								
				if(p->master->forward7[i]->GetVolume(&vol)==DS_OK)
				{	
					p->master->forward7[i]->SetVolume(min(max(vol+add,DSBVOLUME_MIN),0));		
				}
			}	
		}
		else if(p->proxy7->GetVolume(&vol)==DS_OK)
		{	
			p->proxy7->SetVolume(min(max(vol+add,DSBVOLUME_MIN),0));		
		}
	}	
}

static void CALLBACK dx_dsound_piano(UINT id, UINT, DWORD_PTR userp, DWORD_PTR, DWORD_PTR)
{
	assert(DSOUND::IDirectSoundBuffer::get_head);

	if(!DSOUND::IDirectSoundBuffer::get_head) return;

	DSOUND::IDirectSoundBuffer *p = 
	DSOUND::IDirectSoundBuffer::get_head->get((void*)userp); 
	
	assert(p&&p->master&&p->master->piano==id);

	if(!p||!p->master||p->master->piano!=id) return;

	p->master->killpiano();
}

extern void DSOUND::Piano(int key)
{	
	if(key<0) return;

#define OK (p->master==p&&p->master->capture)

	if(!DSOUND::IDirectSoundBuffer::get_head) return;

	DSOUND::IDirectSoundBuffer *p = DSOUND::IDirectSoundBuffer::get_head;	
	DSOUND::IDirectSoundBuffer *q = p; 

	int safety = 0; static const int paranoia = 26*2*4;

	while(p&&p->get_next!=q&&p->isPrimary&&p->isDuplicate||!(OK)) 
	{	
		p = p->get_next; if(++safety==paranoia) break;
	}
	
	if(!p||safety==paranoia) return;

	if(key!=0)
	{
		for(p=p->get_next;key&&p&&p!=q;p=key?p->get_next:p) 
		{			
			if(!p->isPrimary&&!p->isDuplicate&&OK) key--;

			if(++safety==paranoia) break;
		}

		if(key||!p||safety==paranoia) return;
	}

	if(p->master==p)
	if(!p->master->piano)
	if(p->master->capture)
	{
//		DWORD status; 
		
//		if(p->capture
//		||p->proxy->GetStatus(&status)==DS_OK
//		&&(status&DSBSTATUS_PLAYING)==0)
		{			
			DX::DSBCAPS caps = {sizeof(caps)};

			if(p->proxy->GetCaps(&caps)!=DS_OK) return;

			WAVEFORMATEX wav; wav.cbSize = sizeof(wav);
			
			if(p->proxy->GetFormat(&wav,wav.cbSize,0)==DS_OK)
			{
				float secs = float(caps.dwBufferBytes)/wav.nAvgBytesPerSec;

				p->master->piano = timeSetEvent(secs*1000,secs*100,dx_dsound_piano,(DWORD_PTR)p->proxy,TIME_ONESHOT);
			}
			else assert(0);
			
			if(!p->master->piano) return;

			DX::IDirectSoundBuffer *buffer = p->proxy;	
			if(p->master->capture) buffer = p->master->capture;

			if(buffer->SetVolume(0)!=DS_OK //MAX
			 ||buffer->SetFrequency(p->master->frequency)!=DS_OK 
			 ||buffer->SetCurrentPosition(0)!=DS_OK
			 ||buffer->Play(0,0,0)!=DS_OK)
			{
				p->master->killpiano(); assert(0);
			}									
		}
	}

#undef OK
}

#define DSOUND_TABLES(Interface)\
void *DSOUND::Interface::vtables[DSOUND::MAX_TABLES] = {0,0,0};\
void *DSOUND::Interface::dtables[DSOUND::MAX_TABLES] = {0,0,0};
	
DSOUND_TABLES(IDirectSound)
DSOUND_TABLES(IDirectSoundBuffer)
DSOUND_TABLES(IDirectSound3DListener)
DSOUND_TABLES(IDirectSound3DBuffer)		   

#undef DSOUND_TABLES

//REMOVE ME?
extern int DSOUND::is_needed_to_initialize()
{
	static int one_off = 0; if(one_off++) return one_off; //??? 

	DX::is_needed_to_initialize(); 
	
#define DSOUND_TABLES(Interface) DSOUND::Interface().register_tables('dx7');

	DSOUND_TABLES(IDirectSound)
	DSOUND_TABLES(IDirectSoundBuffer)
	DSOUND_TABLES(IDirectSound3DListener)
	DSOUND_TABLES(IDirectSound3DBuffer)		  

#undef DSOUND_TABLES

	DSOUND_HELLO(0) << "DirectSound initialized\n";

	return 1; //for static thunks //???
}  
extern void DSOUND::multicasting_dinput_key_engaged(unsigned char keycode) //extern
{
	switch(keycode)
	{
	case 0xC5: case 0xA2: //DIK_PAUSE/DIK_PLAYPAUSE

//		if(DSOUND::doPause) DSOUND::isPaused = !DSOUND::isPaused;
		
		break;
	}
}

namespace DSOUND
{
	static HRESULT (WINAPI *DirectSoundCreate)(LPCGUID,LPDIRECTSOUND*,LPUNKNOWN) = 0;
	static HRESULT (WINAPI *DirectSoundEnumerateA)(LPDSENUMCALLBACKA,LPVOID) = 0;
}
static HRESULT WINAPI dx_dsound_DirectSoundCreate(LPCGUID lpGuid, LPDIRECTSOUND *lplpDS, LPUNKNOWN pUnkouter)
{				
	DSOUND::is_needed_to_initialize();

	DSOUND_LEVEL(7) << "dx_dsound_DirectSoundCreate()\n";
	
	//experimental: movies?
	if(DSOUND::DirectSound)
	{	
		//hack: first come first serve
		return DSOUND::DirectSoundCreate(lpGuid,lplpDS,pUnkouter);
		//DSOUND_LEVEL(7) << " Proxy exists. Adding reference...\n"; 	
		//DSOUND::DirectSound->AddRef(); *lplpDS = (LPDIRECTSOUND)DSOUND::DirectSound; 
		//return DS_OK;
	}

	static OLECHAR g[64]; if(lpGuid) StringFromGUID2(*lpGuid,g,64); //,,,
	
	if(lpGuid) DSOUND_LEVEL(7) << ' ' << g << '\n'; //the display device??

	IDirectSound *q = 0;
		
	IDirectSound8 *ds8 = 0; //NECESSARY?
		
	//NOTE: DS8 seems to work with MIDI files so that it doesn't freeze up
	//when there's no BGM but it still doesn't work when there's no BGM at
	//all. I guess that means it's sharing a common subsytem with the MIDI
	//player
	HRESULT out; if(DSOUND::doIDirectSound8) //2020: reverb?
	{
		out = DirectSoundCreate8(0,&ds8,0); 
		/*if(!out)
		{
			assert(!pUnkouter);
			out = ds8->QueryInterface(IID_IDirectSound,(void**)&q);
			if(out) ds8->Release();
			if(out) ds8 = 0;
		}*/
		(void*&)q = ds8;
	}
	if(!ds8) out = DSOUND::DirectSoundCreate(lpGuid,&q,pUnkouter); 

	if(out!=DS_OK)
	{ 
		DSOUND_LEVEL(7) << "DirectSoundCreate() Failed\n"; 
		
		return out; 
	}

	DSOUND_LEVEL(7) << "(IDirectSound)\n";		

	if(DSOUND::doIDirectSound)
	{	
		DSOUND::DirectSound8 = ds8; //can't use QueryInterface?

		DSOUND::IDirectSound *p = new DSOUND::IDirectSound('dx7');

		p->proxy = (DX::LPDIRECTSOUND)q; *lplpDS = (LPDIRECTSOUND)p;

		DSOUND::DirectSound = p;
	}
	else *lplpDS = (::LPDIRECTSOUND)q;

	return out;
}			  
static HRESULT WINAPI dx_dsound_DirectSoundEnumerateA(LPDSENUMCALLBACKA lpCallback, LPVOID lpContext)
{						 
	DSOUND::is_needed_to_initialize();

	DSOUND_LEVEL(7) << "dx_dsound_DirectSoundEnumerateA()\n";

	void *passthru = dx_dsound_reserve_enum_set(lpCallback,lpContext);

	HRESULT out = DSOUND::DirectSoundEnumerateA(dx_dsound_enumeratecb,passthru);

	dx_dsound_return_enum_set(passthru); return out;
}
static void dx_dsound_detours(LONG (WINAPI *f)(PVOID*,PVOID))
{	
		//this was not necessary prior to adding the DirectShow (wavdest)
		//static libraries to the mix. it seemed to fail even then, until
		//I noticed the wavdest project was using __stdcall by way of its
		//MSVC project file. could be just a coincidence
		HMODULE dll = GetModuleHandleA("dsound.dll");
		if(!dll) return; //not a game
		//#define _(x) x
		#define _(x) GetProcAddress(dll,#x)		
	if(!DSOUND::DirectSoundCreate)
	{
		(void*&)DSOUND::DirectSoundCreate = _(DirectSoundCreate);
		(void*&)DSOUND::DirectSoundEnumerateA = _(DirectSoundEnumerateA);
	}
		#undef _
	f(&(PVOID&)DSOUND::DirectSoundCreate,dx_dsound_DirectSoundCreate);	
	f(&(PVOID&)DSOUND::DirectSoundEnumerateA,dx_dsound_DirectSoundEnumerateA);	

}//register dx_dsound_detours
static int dx_dsound_detouring = DX::detouring(dx_dsound_detours);

