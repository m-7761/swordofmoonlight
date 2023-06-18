
#include "Ex.h" 
EX_TRANSLATION_UNIT

#include "dx.dinput.h" //todo: graceful movie cancel
#include "Ex.window.h"
#include "Ex.movie.h"

static bool Ex_movie_playing = false;
static bool Ex_movie_stop_playing = false;

extern bool EX::is_playing_movie()
{
	return Ex_movie_playing;
}

extern void EX::stop_playing_movie()
{
	if(Ex_movie_playing) Ex_movie_stop_playing = true;
}

#pragma comment(lib,"mf.lib")
#pragma comment(lib,"mfplat.lib")
#pragma comment(lib,"mfuuid.lib")
#pragma comment(lib,"strmiids.lib")

//#include <mfmediaengine.h> //IMFMediaEngine?

//////////////////////////////////////////////////////////////////////////
// ContentEnabler.cpp: Manages content enabler action.
// 
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////

//#include "strsafe.h"
//#include "ProtectedPlayback.h"
//#include "common.h"
//{
//    #define WINVER 0x0501       
//    #define _WIN32_WINNT 0x0501

	// Windows Header Files:
//    #include <windows.h>

	// C RunTime Header Files
//    #include <tchar.h>
//    #include <commdlg.h> // OpenFile dialog
//    #include <assert.h>

	#undef QWORD //C2628
		// Media Foundation headers
	#include <mfapi.h>
	#include <mfobjects.h>
	#include <mfidl.h>
	#include <mferror.h>
	#include <nserror.h>  // More DRM errors.

	// EVR headers
	#include <evr.h>

	// Safe string functions
	#include <strsafe.h>

	/*#include "common.h"
	namespace MediaFoundationSamples
	{
		// IMPORTANT: No function here can return a NULL pointer - caller assumes
		// the return value is a valid null-terminated string. You should only
		// use these functions for debugging purposes.

		// Media Foundation event names (subset)
		inline const WCHAR* EventName(MediaEventType met)
		{
			switch(met)
			{
				#define _(x) case x: return L#x
				_(MEError);
				_(MEExtendedType);
				_(MESessionTopologySet);
				_(MESessionTopologiesCleared);
				_(MESessionStarted);
				_(MESessionPaused);
				_(MESessionStopped);
				_(MESessionClosed);
				_(MESessionEnded);
				_(MESessionRateChanged);
				_(MESessionScrubSampleComplete);
				_(MESessionCapabilitiesChanged);
				_(MESessionTopologyStatus);
				_(MESessionNotifyPresentationTime);
				_(MENewPresentation);
				_(MELicenseAcquisitionStart);
				_(MELicenseAcquisitionCompleted);
				_(MEIndividualizationStart);
				_(MEIndividualizationCompleted);
				_(MEEnablerProgress);
				_(MEEnablerCompleted);
				_(MEPolicyError);
				_(MEPolicyReport);
				_(MEBufferingStarted);
				_(MEBufferingStopped);
				_(MEConnectStart);
				_(MEConnectEnd);
				_(MEReconnectStart);
				_(MEReconnectEnd);
				_(MERendererEvent);
				_(MESessionStreamSinkFormatChanged);
				_(MESourceStarted);
				_(MEStreamStarted);
				_(MESourceSeeked);
				_(MEStreamSeeked);
				_(MENewStream);
				_(MEUpdatedStream);
				_(MESourceStopped);
				_(MEStreamStopped);
				_(MESourcePaused);
				_(MEStreamPaused);
				_(MEEndOfPresentation);
				_(MEEndOfStream);
				_(MEMediaSample);
				_(MEStreamTick);
				_(MEStreamThinMode);
				_(MEStreamFormatChanged);
				_(MESourceRateChanged);
				_(MEEndOfPresentationSegment);
				_(MESourceCharacteristicsChanged);
				_(MESourceRateChangeRequested);
				_(MESourceMetadataChanged);
				_(MESequencerSourceTopologyUpdated);
				_(MEStreamSinkStarted);
				_(MEStreamSinkStopped);
				_(MEStreamSinkPaused);
				_(MEStreamSinkRateChanged);
				_(MEStreamSinkRequestSample);
				_(MEStreamSinkMarker);
				_(MEStreamSinkPrerolled);
				_(MEStreamSinkScrubSampleComplete);
				_(MEStreamSinkFormatChanged);
				_(MEStreamSinkDeviceChanged);
				_(MEQualityNotify);
				_(MESinkInvalidated);
				_(MEAudioSessionNameChanged);
				_(MEAudioSessionVolumeChanged);
				_(MEAudioSessionDeviceRemoved);
				_(MEAudioSessionServerShutdown);
				_(MEAudioSessionGroupingParamChanged);
				_(MEAudioSessionIconChanged);
				_(MEAudioSessionFormatChanged);
				_(MEAudioSessionDisconnected);
				_(MEAudioSessionExclusiveModeOverride);
				_(MEPolicyChanged);
				_(MEContentProtectionMessage);
				_(MEPolicySet);
				#undef _

			default:
				return L"Unknown event";
			}
		}
	}
	using namespace MediaFoundationSamples;*/
	#ifndef SAFE_RELEASE
	template<class T> inline void SAFE_RELEASE(T *&p)
	{
		if(p) p->Release(); p = NULL;
	}
	#endif

	#define CHECK_HR(hr) IF_FAILED_GOTO(hr, done) //YUCK?
	// IF_FAILED_GOTO macro.
	// Jumps to 'label' on failure.
	#ifndef IF_FAILED_GOTO
	#define IF_FAILED_GOTO(hr, label) if (FAILED(hr)) { goto label; }
	#endif


//    #include "resource.h"
//    #include "webhelper.h"
//    #include "ContentEnabler.h"
//    #include "Player.h"

//    #define USE_LOGGING
//    #include "logging.h"
#define TRACE_INIT() 
#define TRACE(x) 
#define TRACE_CLOSE()
#define LOG_MSG_IF_FAILED(x, hr)
#define LOG_HRESULT(hr)
#define LOG_MSG_IF_FAILED(msg, hr)
//}

static void LogEnableType(const GUID &guidEnableType);
static void LogTrustStatus(MF_URL_TRUST_STATUS status);

const UINT WM_APP_PLAYER_EVENT = WM_APP+1; // wparam = IMFMediaEvent*

// WM_APP_CONTENT_ENABLER: Signals that the application must perform a
// content enabler action.
const UINT WM_APP_CONTENT_ENABLER = WM_APP+3; // no message parameters

// WM_APP_BROWSER_DONE: Signals that the user closed the browser window.
//const UINT WM_APP_BROWSER_DONE = WM_APP+4; // no message parameters

enum EnablerState
{
	Enabler_Ready,
	Enabler_SilentInProgress,
	Enabler_NonSilentInProgress,
	Enabler_Complete
};
enum EnablerFlags
{
	SilentOrNonSilent = 0,  // Use silent if supported, otherwise use non-silent.
	ForceNonSilent = 1      // Use non-silent.
};

//////////////////////////////////////////////////////////////////////////
//  ContentProtectionManager
//  Description: Manages content-enabler actions.
// 
//  This object implements IMFContentProtectionManager. The PMP media
//  session uses this interface to pass a content enabler object back
//  to the application. A content enabler in an object that performs some 
//  action needed to play a protected file, such as license acquistion.
//
//  For more information about content enablers, see IMFContentEnabler in
//  the Media Foundation SDK documentation.
//////////////////////////////////////////////////////////////////////////

/*struct DispatchCallback
{
	virtual void OnDispatchInvoke(DISPID  dispIdMember) = 0;
};*/

class ContentProtectionManager
	:
	public IMFAsyncCallback, public IMFContentProtectionManager //,
//	public DispatchCallback // To get callbacks from the browser control.
{
public:

	static HRESULT CreateInstance(HWND hNotify,ContentProtectionManager **ppManager)
	{
		if(!hNotify||!ppManager) return E_INVALIDARG;

		auto *pManager = new ContentProtectionManager(hNotify);
		if(pManager) (*ppManager=pManager)->AddRef();

		return !pManager?E_OUTOFMEMORY:S_OK;
	}

	// IUnknown
	STDMETHODIMP QueryInterface(REFIID iid,void **ppv)
	{
		if(!ppv) return E_POINTER;
		else if(iid==IID_IUnknown)
		*ppv = static_cast<IUnknown*>(static_cast<IMFAsyncCallback*>(this));
		else if(iid==IID_IMFAsyncCallback)
		*ppv = static_cast<IMFAsyncCallback*>(this);
		else if(iid==IID_IMFContentProtectionManager)
		*ppv = static_cast<IMFContentProtectionManager*>(this);
		else *ppv = NULL;

		if(*ppv) AddRef(); return *ppv?S_OK:E_NOINTERFACE;
	}

	STDMETHODIMP_(ULONG) AddRef()
	{
		return InterlockedIncrement(&m_nRefCount);
	}
	STDMETHODIMP_(ULONG) Release()
	{
		ULONG uCount = InterlockedDecrement(&m_nRefCount);
		if(!uCount) delete this;
		// For thread safety, return a temporary variable.
		return uCount;
	}

	// IMFAsyncCallback methods
	STDMETHODIMP GetParameters(DWORD *,DWORD *)
	{
		return E_NOTIMPL; //optional        
	}

	// IMFContentProtectionManager methods
	STDMETHODIMP BeginEnableContent(IMFActivate *pEnablerActivate,
	IMFTopology *pTopo,IMFAsyncCallback *pCallback,IUnknown *punkState)
	{
		TRACE((L"ContentProtectionManager::BeginEnableContent"));

		HRESULT hr = S_OK;

		if(m_pEnabler!=NULL)
		{
			TRACE((L"A previous call is still pending."));
			return E_FAIL;
		}

		// Create an async result for later use.
		hr = MFCreateAsyncResult(NULL,pCallback,punkState,&m_pResult);
		LOG_MSG_IF_FAILED(L"MFCreateAsyncResult",hr);

		// Create the enabler from the IMFActivate pointer.
		if(SUCCEEDED(hr))
		{
			hr = pEnablerActivate->ActivateObject(IID_IMFContentEnabler,(void**)&m_pEnabler);
			LOG_MSG_IF_FAILED(L"ActivateObject",hr);
		}

		// Notify the application. The application will call DoEnable from the app thread.
		if(SUCCEEDED(hr))
		{
			m_state = Enabler_Ready; // Reset the state.
			PostMessage(m_hwnd,WM_APP_CONTENT_ENABLER,0,0);
		}

		return hr;
	}

	STDMETHODIMP EndEnableContent(IMFAsyncResult *pResult)
	{
		TRACE((L"ContentProtectionManager::EndEnableContent"));
		if(!pResult) return E_POINTER;

		// Release interfaces, so that we're ready to accept another call
		// to BeginEnableContent.
		SAFE_RELEASE(m_pResult);
		SAFE_RELEASE(m_pEnabler);
		SAFE_RELEASE(m_pMEG); return m_hrStatus;
	}

	STDMETHODIMP Invoke(IMFAsyncResult *pAsyncResult);

	/*// DispatchCallback
	void OnDispatchInvoke(DISPID  dispIdMember)
	{
		if (dispIdMember == DISPID_ONQUIT)
		{
			// The user closed the browser window. Notify the application.
			TRACE((TEXT("DISPID_ONQUIT")));
			PostMessage(m_hwnd, WM_APP_BROWSER_DONE, 0, 0);
			m_webHelper.Exit();
		}
	}*/

	// Public methods for the application.

	HRESULT DoEnable(EnablerFlags flags=SilentOrNonSilent);
	HRESULT CancelEnable();
	HRESULT CompleteEnable();

	EnablerState GetState()const{ return m_state; }
	HRESULT GetStatus()const{ return m_hrStatus; }

private:

	ContentProtectionManager(HWND hNotify)
		:
		m_nRefCount(0),m_pMEG(),m_pResult(),m_pEnabler(),m_hwnd(hNotify),
		m_state(Enabler_Ready),m_hrStatus(S_OK){}

	virtual ~ContentProtectionManager()
	{
		TRACE((L"~ContentEnabler\n"));
		SAFE_RELEASE(m_pMEG);
		SAFE_RELEASE(m_pResult);
		SAFE_RELEASE(m_pEnabler);
	}

	HRESULT DoNonSilentEnable();

	long m_nRefCount;        // Reference count.
	EnablerState m_state;
	HRESULT m_hrStatus;         // Status code from the most recent event.
	HWND m_hwnd;

	IMFContentEnabler *m_pEnabler;        // Content enabler.
	IMFMediaEventGenerator *m_pMEG;            // The content enabler's event generator interface.
	IMFAsyncResult *m_pResult;         // Asynchronus result object.

//    WebHelper               m_webHelper;        // For non-silent enable
};
///////////////////////////////////////////////////////////////////////
//  Name: Invoke
//  Description:  Callback for asynchronous BeginGetEvent method.
//  
//  pAsyncResult: Pointer to the result.
/////////////////////////////////////////////////////////////////////////
HRESULT ContentProtectionManager::Invoke(IMFAsyncResult *pAsyncResult)
{
	HRESULT hr = S_OK;
	IMFMediaEvent *pEvent = NULL;
	MediaEventType meType = MEUnknown;  // Event type
	PROPVARIANT varEventData;           // Event data

	PropVariantInit(&varEventData);

	// Get the event from the event queue.
	hr = m_pMEG->EndGetEvent(pAsyncResult,&pEvent);
	LOG_MSG_IF_FAILED(L"IMediaEventGenerator::EndGetEvent",hr);

	// Get the event type.
	if(SUCCEEDED(hr)) hr = pEvent->GetType(&meType);

	// Get the event status. If the operation that triggered the event did
	// not succeed, the status is a failure code.
	if(SUCCEEDED(hr)) hr = pEvent->GetStatus(&m_hrStatus);

	// Get the event data.
	if(SUCCEEDED(hr)) hr = pEvent->GetValue(&varEventData);

	if(SUCCEEDED(hr))
	{
		// For the MEEnablerCompleted action, notify the application.
		// Otehrwise, request another event.
		TRACE((L"Content enabler event: %s",EventName(meType)));

		if(meType==MEEnablerCompleted)
		{
			PostMessage(m_hwnd,WM_APP_CONTENT_ENABLER,0,0);
		}
		else
		{
			if(meType==MEEnablerProgress)
			{
				if(varEventData.vt==VT_LPWSTR)
				{
					TRACE((L"Progress: %s",varEventData.pwszVal));
				}
			}
			m_pMEG->BeginGetEvent(this,NULL);
		}
	}

	// Clean up.
	PropVariantClear(&varEventData); 
	
	SAFE_RELEASE(pEvent); return S_OK;
}
///////////////////////////////////////////////////////////////////////
//  Name: DoEnable
//  Description:  Does the enabler action.
//
//  flags: If ForceNonSilent, then always use non-silent enable.
//         Otherwise, use silent enable if possible.
////////////////////////////////////////////////////////////////////////
HRESULT ContentProtectionManager::DoEnable(EnablerFlags flags)
{
	TRACE((L"ContentProtectionManager::DoEnable (flags =%d)",flags));

	HRESULT hr = S_OK;
	BOOL bAutomatic = FALSE;
	GUID guidEnableType;

	// Get the enable type. (Just for logging. We don't use it.)
	hr = m_pEnabler->GetEnableType(&guidEnableType);
	LOG_MSG_IF_FAILED(L"GetEnableType",hr);

	if(SUCCEEDED(hr)) LogEnableType(guidEnableType);

	// Query for the IMFMediaEventGenerator interface so that we can get the
	// enabler events.
	if(SUCCEEDED(hr))
	hr = m_pEnabler->QueryInterface(IID_IMFMediaEventGenerator,(void**)&m_pMEG);	

	// Ask for the first event.
	if(SUCCEEDED(hr)) hr = m_pMEG->BeginGetEvent(this,NULL);

	// Decide whether to use silent or non-silent enabling. If flags is ForceNonSilent,
	// then we use non-silent. Otherwise, we query whether the enabler object supports 
	// silent enabling (also called "automatic" enabling).
	if(SUCCEEDED(hr))	
	if(flags==ForceNonSilent)
	{
		TRACE((L"Forcing non-silent enable."));
		bAutomatic = FALSE;
	}
	else
	{
		hr = m_pEnabler->IsAutomaticSupported(&bAutomatic);
		TRACE((L"IsAutomatic: auto = %d",bAutomatic));
	}

	// Start automatic or non-silent, depending.
	if(SUCCEEDED(hr))	
	if(bAutomatic)
	{
		m_state = Enabler_SilentInProgress;
		TRACE((L"Content enabler: Automatic is supported"));
		hr = m_pEnabler->AutomaticEnable();
	}
	else
	{
		m_state = Enabler_NonSilentInProgress;
		TRACE((L"Content enabler: Using non-silent enabling"));
		hr = DoNonSilentEnable();
	}
	

	if(FAILED(hr)) m_hrStatus = hr; return hr;
}
///////////////////////////////////////////////////////////////////////
//  Name: CancelEnable
//  Description:  Cancels the current action.
//  
//  During silent enable, this cancels the enable action in progress.
//  During non-silent enable, this cancels the MonitorEnable thread.
/////////////////////////////////////////////////////////////////////////
HRESULT ContentProtectionManager::CancelEnable()
{
	HRESULT hr = S_OK;
	if(m_state!=Enabler_Complete)
	{
		hr = m_pEnabler->Cancel();
		LOG_MSG_IF_FAILED(L"IMFContentEnabler::Cancel",hr);

		if(FAILED(hr))
		{
			// If Cancel fails for some reason, queue the MEEnablerCompleted
			// event ourselves. This will cause the current action to fail.
			m_pMEG->QueueEvent(MEEnablerCompleted,GUID_NULL,hr,NULL);
		}
	}
	return hr;
}
///////////////////////////////////////////////////////////////////////
//  Name: CompleteEnable
//  Description:  Completes the current action.
//  
//  This method invokes the PMP session's callback. 
/////////////////////////////////////////////////////////////////////////
HRESULT ContentProtectionManager::CompleteEnable()
{
	m_state = Enabler_Complete;

	// m_pResult can be NULL if the BeginEnableContent was not called.
	// This is the case when the application initiates the enable action, eg 
	// when MFCreatePMPMediaSession fails and returns an IMFActivate pointer.
	if(m_pResult)
	{
		TRACE((L"ContentProtectionManager: Invoking the pipeline's callback. (status = 0x%X)",m_hrStatus));
		m_pResult->SetStatus(m_hrStatus);
		MFInvokeCallback(m_pResult);
	}
	return S_OK;
}
///////////////////////////////////////////////////////////////////////
//  Name: DoNonSilentEnable
//  Description:  Performs non-silent enable.
/////////////////////////////////////////////////////////////////////////
HRESULT ContentProtectionManager::DoNonSilentEnable()
{
	// Trust status for the URL.
	MF_URL_TRUST_STATUS trustStatus = MF_LICENSE_URL_UNTRUSTED;

	WCHAR *sURL = NULL;       // Enable URL
	DWORD cchURL = 0;         // Size of enable URL in characters.

	BYTE *pPostData = NULL;  // Buffer to hold HTTP POST data.
	DWORD cbPostDataSize = 0; // Size of buffer, in bytes.

	HRESULT hr = S_OK;

	// Get the enable URL. This is where we get the enable data for non-silent enabling.
	hr = m_pEnabler->GetEnableURL(&sURL,&cchURL,&trustStatus);
	LOG_MSG_IF_FAILED(L"GetEnableURL",hr);

	if(SUCCEEDED(hr))
	{
		TRACE((L"Content enabler: URL = %s",sURL));
		LogTrustStatus(trustStatus);
	}

	if(trustStatus!=MF_LICENSE_URL_TRUSTED)
	{
		TRACE((L"The enabler URL is not trusted. Failing."));
		hr = E_FAIL;
	}

	// Start the thread that monitors the non-silent enable action. 
	if(SUCCEEDED(hr))
	{
		hr = m_pEnabler->MonitorEnable();
	}

	// Get the HTTP POST data
	if(SUCCEEDED(hr))
	{
		hr = m_pEnabler->GetEnableData(&pPostData,&cbPostDataSize);
		LOG_MSG_IF_FAILED(L"GetEnableData",hr);
	}

	// Initialize the browser control.
//	if(SUCCEEDED(hr)) hr = m_webHelper.Init((DispatchCallback*)this);

	// Open the URL and send the HTTP POST data.
//	if(SUCCEEDED(hr))
//	hr = m_webHelper.OpenURLWithData(sURL, pPostData, cbPostDataSize);

	CoTaskMemFree(pPostData); CoTaskMemFree(sURL); return hr;
}

static void LogEnableType(const GUID &guidEnableType)
{
	if(guidEnableType==MFENABLETYPE_WMDRMV1_LicenseAcquisition)
	{
		TRACE((L"MFENABLETYPE_WMDRMV1_LicenseAcquisition"));
	}
	else if(guidEnableType==MFENABLETYPE_WMDRMV7_LicenseAcquisition)
	{
		TRACE((L"MFENABLETYPE_WMDRMV7_LicenseAcquisition"));
	}
	else if(guidEnableType==MFENABLETYPE_WMDRMV7_Individualization)
	{
		TRACE((L"MFENABLETYPE_WMDRMV7_Individualization"));
	}
	else if(guidEnableType==MFENABLETYPE_MF_UpdateRevocationInformation)
	{
		TRACE((L"MFENABLETYPE_MF_UpdateRevocationInformation"));
	}
	else if(guidEnableType==MFENABLETYPE_MF_UpdateUntrustedComponent)
	{
		TRACE((L"MFENABLETYPE_MF_UpdateUntrustedComponent"));
	}
	else TRACE((L"Unknown content enabler type."));
}
static void LogTrustStatus(MF_URL_TRUST_STATUS status)
{
	switch(status)
	{
	case MF_LICENSE_URL_UNTRUSTED:
		TRACE((L"MF_LICENSE_URL_UNTRUSTED"));
		break;
	case MF_LICENSE_URL_TRUSTED:
		TRACE((L"MF_LICENSE_URL_TRUSTED"));
		break;
	case MF_LICENSE_URL_TAMPERED:
		TRACE((L"MF_LICENSE_URL_TAMPERED"));
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
// 
// player.cpp : Playback helper class.
// 
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////

//#include "ProtectedPlayback.h"
//#include "ContentEnabler.h"

// Forward declarations
static HRESULT CreateSourceStreamNode
(IMFMediaSource*,IMFPresentationDescriptor*,IMFStreamDescriptor*,IMFTopologyNode**);
static HRESULT CreateOutputNode(IMFStreamDescriptor*,HWND,IMFTopologyNode**);
//const UINT WM_APP_PLAYER_EVENT = WM_APP+1; // wparam = IMFMediaEvent*
enum PlayerState
{
	Closed = 0,     // No session.
	Ready,          // Session was created, ready to open a file. 
	OpenPending,    // Session is opening a file.
	Started,        // Session is playing a file.
	Paused,         // Session is paused.
	Stopped,        // Session is stopped (ready to play). 
	Closing         // Application has closed the session, but is waiting for MESessionClosed.
};
class CPlayer : public IMFAsyncCallback
{
public:

	static HRESULT CreateInstance(HWND hVideo,HWND hEvent,CPlayer **ppPlayer);

	// IUnknown methods
	STDMETHODIMP QueryInterface(REFIID iid, void **ppv)
	{
		if(!ppv) return E_POINTER;
		else if(iid==IID_IUnknown)
		*ppv = static_cast<IUnknown*>(this);
		else if(iid==IID_IMFAsyncCallback)
		*ppv = static_cast<IMFAsyncCallback*>(this);
		else *ppv = NULL;

		if(*ppv) AddRef(); return *ppv?S_OK:E_NOINTERFACE;
	}
	STDMETHODIMP_(ULONG) AddRef()
	{
		return InterlockedIncrement(&m_nRefCount);
	}
	STDMETHODIMP_(ULONG) Release()
	{
		ULONG uCount = InterlockedDecrement(&m_nRefCount);
		if(uCount==0) delete this;
		// For thread safety, return a temporary variable.
		return uCount;
	}

	// IMFAsyncCallback methods
	STDMETHODIMP GetParameters(DWORD *,DWORD *)
	{
		return E_NOTIMPL; //optional
	}

	STDMETHODIMP Invoke(IMFAsyncResult *pAsyncResult);

	// Playback
	HRESULT OpenURL(const WCHAR *sURL);
	HRESULT Play();
	HRESULT Pause();
	HRESULT Shutdown();
	HRESULT HandleEvent(UINT_PTR pUnkPtr);
	PlayerState GetState()const{ return m_state; }
	// Video functionality
	HRESULT Repaint()
	{
		HRESULT hr = !m_pVideoDisplay?S_OK:m_pVideoDisplay->RepaintVideo();
		return hr; //breakpoint
	}
	HRESULT MoveVideo(long x, long y, long width, long height) //WM_SIZE
	{
		//Set the destination rectangle.
		// Leave the default source rectangle (0,0,1,1).
		RECT rcDest = { x,y,x+width,y+height };
		HRESULT hr = !m_pVideoDisplay?S_OK:m_pVideoDisplay->SetVideoPosition(NULL,&rcDest);
		return hr; //breakpoint
	}
	BOOL HasVideo()const { return m_pVideoDisplay!=nullptr; }

	// Content protection manager
	HRESULT GetContentProtectionManager(ContentProtectionManager **ppManager);

	void SetMasterVolume(float percent) //2023
	{
		UINT32 count;
		if(m_pVolumeControl&&!m_pVolumeControl->GetChannelCount(&count))
		{
			float vol = percent;
			float vols[20];
			for(int i=count;i-->0;) vols[i] = vol;
			m_pVolumeControl->SetAllVolumes(count,vols);
		}
		m_volume = percent;
	}

protected:

	// Constructor is private. Use static CreateInstance method to instantiate.
	CPlayer(HWND hVideo,HWND hEvent)
		:
		m_pSession(),
		m_pSource(),
		m_pVideoDisplay(),
		m_pVolumeControl(),m_volume(1.0f),
		m_hwndVideo(hVideo),
		m_hwndEvent(hEvent),
		m_state(Ready),
		m_hCloseEvent(),
		m_nRefCount(1),
		m_pContentProtectionManager()
	{}
	// Destructor is private. Caller should call Release.
	virtual ~CPlayer()
	{
		assert(m_pSession==NULL);  // If FALSE, the app did not call Shutdown().

		// Note: The application must call Shutdown() because the media 
		// session holds a reference count on the CPlayer object. (This happens
		// when CPlayer calls IMediaEventGenerator::BeginGetEvent on the
		// media session.) As a result, there is a circular reference count
		// between the CPlayer object and the media session. Calling Shutdown()
		// breaks the circular reference count.

		// Note: If CreateInstance failed, the application will not call 
		// Shutdown(). To handle that case, we must call Shutdown() in the 
		// destructor. The circular ref-count problem does not occcur if
		// CreateInstance has failed. Also, calling Shutdown() twice is
		// harmless.
		Shutdown();
	}

	HRESULT Initialize();
	HRESULT CreateSession();
	HRESULT CloseSession();
	HRESULT StartPlayback();
	HRESULT CreateMediaSource(const WCHAR *sURL);
	HRESULT CreateTopologyFromSource(IMFTopology **ppTopology);

	HRESULT AddBranchToPartialTopology
	(IMFTopology *pTopology, IMFPresentationDescriptor *pSourcePD, DWORD iStream);

	// Media event handlers
	HRESULT OnTopologyReady(IMFMediaEvent *pEvent);
	HRESULT OnSessionStarted(IMFMediaEvent *pEvent);
	HRESULT OnSessionPaused(IMFMediaEvent *pEvent);
	HRESULT OnSessionClosed(IMFMediaEvent *pEvent);
	HRESULT OnPresentationEnded(IMFMediaEvent *pEvent);

	long m_nRefCount;        // Reference count.

	IMFMediaSession *m_pSession;
	IMFMediaSource *m_pSource;
	IMFVideoDisplayControl *m_pVideoDisplay;
	IMFAudioStreamVolume *m_pVolumeControl; //2023
	float m_volume;

	HWND m_hwndVideo;        // Video window.
	HWND m_hwndEvent;        // App window to receive events.
	PlayerState m_state;            // Current state of the media session.
	HANDLE m_hCloseEvent;      // Event to wait on while closing

	ContentProtectionManager *m_pContentProtectionManager;
};
///////////////////////////////////////////////////////////////////////
//  Name: CreateInstance
//  Description:  Static class method to create the CPlayer object.
//  
//  hVideo:   Handle to the video window.
//  hEvent:   Handle to the window to receive notifications.
//  ppPlayer: Receives an AddRef's pointer to the CPlayer object. 
//            The caller must release the pointer.
/////////////////////////////////////////////////////////////////////////
HRESULT CPlayer::CreateInstance(HWND hVideo, HWND hEvent, CPlayer **ppPlayer)
{
	TRACE((L"CPlayer::Create\n"));

	assert(hVideo);
	assert(hEvent);

	if(!ppPlayer) return E_POINTER;

	CPlayer *pPlayer = new CPlayer(hVideo,hEvent);
	HRESULT hr = pPlayer?S_OK:E_OUTOFMEMORY;

	hr = pPlayer->Initialize();
	if(SUCCEEDED(hr))
	{
		*ppPlayer = pPlayer; (*ppPlayer)->AddRef();
	}

	// The CPlayer constructor sets the ref count to 1.
	// If the method succeeds, the caller receives an AddRef'd pointer.
	// Whether the method succeeds or fails, we must release the pointer.
	SAFE_RELEASE(pPlayer); return hr;
}
//////////////////////////////////////////////////////////////////////
//  Name: Initialize
//  Initializes the CPlayer object. This method is called by the
//  CreateInstance method.
/////////////////////////////////////////////////////////////////////////
HRESULT CPlayer::Initialize()
{
	HRESULT hr = S_OK;

	if(m_hCloseEvent) return MF_E_ALREADY_INITIALIZED;

	// Start up Media Foundation platform.
	CHECK_HR(hr=MFStartup(MF_VERSION));

	m_hCloseEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	if(!m_hCloseEvent)
	CHECK_HR(hr = HRESULT_FROM_WIN32(GetLastError()));

done: return hr;
}
///////////////////////////////////////////////////////////////////////
//  Name: OpenURL
//  Description:  Opens a URL for playback.
/////////////////////////////////////////////////////////////////////////
HRESULT CPlayer::OpenURL(const WCHAR *sURL)
{
	TRACE((L"CPlayer::OpenURL\n"));
	TRACE((L"URL = %s\n",sURL));

	// 1. Create a new media session.
	// 2. Create the media source.
	// 3. Create the topology.
	// 4. Queue the topology [asynchronous]
	// 5. Start playback [asynchronous - does not happen in this method.]

	HRESULT hr = S_OK;
	IMFTopology *pTopology = NULL;

	// Create the media session.
	CHECK_HR(hr = CreateSession());

	// Create the media source.
	CHECK_HR(hr = CreateMediaSource(sURL));

	// Create a partial topology.
	CHECK_HR(hr = CreateTopologyFromSource(&pTopology));

	// Set the topology on the media session.
	CHECK_HR(hr = m_pSession->SetTopology(0,pTopology));

	// Set our state to "open pending"
	m_state = OpenPending;

	// If SetTopology succeeded, the media session will queue an 
	// MESessionTopologySet event.

done: if(FAILED(hr)) m_state = Closed;

	SAFE_RELEASE(pTopology); return hr;
}
///////////////////////////////////////////////////////////////////////
//  Name: Play
//  Description:  Starts playback from paused state.
/////////////////////////////////////////////////////////////////////////
HRESULT CPlayer::Play()
{
	TRACE((L"CPlayer::Play\n"));

	if(m_state!=Paused&&m_state!=Stopped) return MF_E_INVALIDREQUEST;

	if(!m_pSession||!m_pSource) return E_UNEXPECTED;

	HRESULT hr = StartPlayback();
	return hr; //breakpoint
}
///////////////////////////////////////////////////////////////////////
//  Name: Pause
//  Description:  Pauses playback.
/////////////////////////////////////////////////////////////////////////
HRESULT CPlayer::Pause()
{
	TRACE((L"CPlayer::Pause\n"));

	if(m_state!=Started) return MF_E_INVALIDREQUEST;

	if(!m_pSession||!m_pSource) return E_UNEXPECTED;

	HRESULT hr = m_pSession->Pause();
	if(SUCCEEDED(hr)) m_state = Paused; return hr;
}
///////////////////////////////////////////////////////////////////////
//  Name: Invoke
//  Description:  Callback for asynchronous BeginGetEvent method.
//  
//  pAsyncResult: Pointer to the result.
/////////////////////////////////////////////////////////////////////////
HRESULT CPlayer::Invoke(IMFAsyncResult *pResult)
{
	HRESULT hr = S_OK;
	MediaEventType meType = MEUnknown;  // Event type

	IMFMediaEvent *pEvent = NULL;

	// Get the event from the event queue.
	CHECK_HR(hr = m_pSession->EndGetEvent(pResult,&pEvent));

	// Get the event type. 
	CHECK_HR(hr = pEvent->GetType(&meType));

	// If the session is closed, the application is waiting on the
	// m_hCloseEvent event handle. Also, do not get any more 
	// events from the session.

	if(meType==MESessionClosed)
	{
		SetEvent(m_hCloseEvent);
	}
	else
	{
		// For all other events, ask the media session for the
		// next event in the queue.
		CHECK_HR(hr = m_pSession->BeginGetEvent(this,NULL));
	}

	// For most events, we post the event as a private window message to the
	// application. This lets the application process the event on it's
	// main thread.

	// However, if call to IMFMediaSession::Close is pending, it means the 
	// application is waiting on the m_hCloseEvent event handle. (Blocking 
	// call.) In that case, we simply discard the event.

	// NOTE: When IMFMediaSession::Close is called, MESessionClosed is NOT
	// necessarily the next event that we will receive. We may receive
	// any number of other events before receiving MESessionClosed.
	if(m_state!=Closing)
	{
		// Leave a reference count on the event.
		auto safe = pEvent->AddRef(); assert(safe<4); //paranoia
		
		//WM_APP_PLAYER_EVENT is not being received when dragging/resizing
		//the window. possibly the messages are getting backedup/discarded
//		PostMessage(m_hwndEvent,WM_APP_PLAYER_EVENT,(WPARAM)pEvent,(LPARAM)0);
		HandleEvent((WPARAM)pEvent);
	}

done: SAFE_RELEASE(pEvent); return S_OK;
}
//-------------------------------------------------------------------
//  HandleEvent
//
//  Called by the application when it receives a WM_APP_PLAYER_EVENT
//  message. 
//
//  This method is used to process media session events on the
//  application's main thread.
//
//  pUnkPtr: Pointer to the IUnknown interface of a media session 
//  event (IMFMediaEvent).
//-------------------------------------------------------------------
HRESULT CPlayer::HandleEvent(UINT_PTR pUnkPtr)
{
	HRESULT hr = S_OK;
	HRESULT hrStatus = S_OK;            // Event status
	MediaEventType meType = MEUnknown;  // Event type
	MF_TOPOSTATUS TopoStatus = MF_TOPOSTATUS_INVALID; // Used with MESessionTopologyStatus event.    

	IUnknown *pUnk = NULL;
	IMFMediaEvent *pEvent = NULL;

	// pUnkPtr is really an IUnknown pointer.
	pUnk = (IUnknown *)pUnkPtr;

	if(!pUnk) return E_POINTER;

	CHECK_HR(hr = pUnk->QueryInterface(__uuidof(IMFMediaEvent),(void**)&pEvent));

	// Get the event type.
	CHECK_HR(hr = pEvent->GetType(&meType));

	// Get the event status. If the operation that triggered the event did
	// not succeed, the status is a failure code.
	CHECK_HR(hr = pEvent->GetStatus(&hrStatus));
	assert(!hr);

	TRACE((L"Media event: %s\n",EventName(meType)));

	// Check if the async operation succeeded.
	if(!hr&&SUCCEEDED(hrStatus))
	{
		// Switch on the event type. Update the internal state of the CPlayer as needed.
		switch(meType)
		{
		case MESessionTopologyStatus:
			// Get the status code.
			CHECK_HR(hr = pEvent->GetUINT32(MF_EVENT_TOPOLOGY_STATUS,(UINT32 *)&TopoStatus));
			switch(TopoStatus)
			{
			case MF_TOPOSTATUS_READY:
				hr = OnTopologyReady(pEvent);
				break;
			default:
				// Nothing to do.
				break;
			}
			break;

		case MEEndOfPresentation:

			CHECK_HR(hr = OnPresentationEnded(pEvent));
			break;
		}
	}
	else hr = hrStatus;

	done: SAFE_RELEASE(pUnk); SAFE_RELEASE(pEvent); return hr;
}
///////////////////////////////////////////////////////////////////////
//  Name: ShutDown
//  Description:  Releases all resources held by this object.
/////////////////////////////////////////////////////////////////////////
HRESULT CPlayer::Shutdown()
{
	TRACE((L"CPlayer::ShutDown\n"));

	// Close the session
	HRESULT hr = CloseSession();

	// Shutdown the Media Foundation platform
	MFShutdown();

	if(m_hCloseEvent) CloseHandle(m_hCloseEvent);

	m_hCloseEvent = NULL; return hr;
}
///////////////////////////////////////////////////////////////////////
//  Name: GetContentProtectionManager
//  Description:  Returns the content protection manager object.
//
//  This is a helper object for handling IMFContentEnabler operations.
/////////////////////////////////////////////////////////////////////////
HRESULT  CPlayer::GetContentProtectionManager(ContentProtectionManager **ppManager)
{
	if(ppManager==NULL) return E_INVALIDARG;
	else if(m_pContentProtectionManager==NULL)
	return E_FAIL; // Session wasn't created yet. No helper object;
	else *ppManager = m_pContentProtectionManager;

	(*ppManager)->AddRef(); return S_OK; //E_OUTOFMEMORY?
}
///////////////////////////////////////////////////////////////////////
//  Name: OnTopologyReady
//  Description:  Handler for MESessionTopologyReady event.
//
//  Note: 
//  - The MESessionTopologySet event means the session queued the 
//    topology, but the topology is not ready yet. Generally, the 
//    applicationno need to respond to this event unless there is an
//    error.
//  - The MESessionTopologyReady event means the new topology is
//    ready for playback. After this event is received, any calls to
//    IMFGetService will get service interfaces from the new topology.
/////////////////////////////////////////////////////////////////////////
HRESULT CPlayer::OnTopologyReady(IMFMediaEvent *pEvent)
{
	TRACE((L"CPlayer::OnTopologyReady\n"));

	// Ask for the IMFVideoDisplayControl interface.
	// This interface is implemented by the EVR and is
	// exposed by the media session as a service.

	// Note: This call is expected to fail if the source
	// does not have video.
	MFGetService(m_pSession,MR_VIDEO_RENDER_SERVICE,__uuidof(IMFVideoDisplayControl),(void**)&m_pVideoDisplay);
	MFGetService(m_pSession,MR_STREAM_VOLUME_SERVICE,__uuidof(IMFAudioStreamVolume),(void**)&m_pVolumeControl);
	SetMasterVolume(m_volume);	

	HRESULT hr = StartPlayback();
	return S_OK; //breakpoint
}
HRESULT CPlayer::OnPresentationEnded(IMFMediaEvent *pEvent)
{
	TRACE((L"CPlayer::OnPresentationEnded\n"));

	// The session puts itself into the stopped state autmoatically.
	m_state = Stopped; return S_OK;
}
///////////////////////////////////////////////////////////////////////
//  Name: CreateSession
//  Description:  Creates a new instance of the media session.
/////////////////////////////////////////////////////////////////////////
HRESULT CPlayer::CreateSession()
{
	TRACE((L"CPlayer::CreateSession\n"));

	HRESULT hr = S_OK;

	IMFAttributes *pAttributes = NULL;
	IMFActivate *pEnablerActivate = NULL;

	// Close the old session, if any.
	CHECK_HR(hr = CloseSession());

	assert(m_state==Closed);

	// Create a new attribute store.
	CHECK_HR(hr = MFCreateAttributes(&pAttributes,1));

	// Create the content protection manager.
	assert(m_pContentProtectionManager==NULL); // Was released in CloseSession

	CHECK_HR(hr = ContentProtectionManager::CreateInstance(
		m_hwndEvent,
		&m_pContentProtectionManager
	));

	// Set the MF_SESSION_CONTENT_PROTECTION_MANAGER attribute with a pointer
	// to the content protection manager.
	CHECK_HR(hr = pAttributes->SetUnknown(
		MF_SESSION_CONTENT_PROTECTION_MANAGER,
		(IMFContentProtectionManager*)m_pContentProtectionManager
	));

	// Create the PMP media session.
	CHECK_HR(hr = MFCreatePMPMediaSession(
	//	0, // Can use this flag: MFPMPSESSION_UNPROTECTED_PROCESS
	!	MFPMPSESSION_UNPROTECTED_PROCESS,
		pAttributes,
		&m_pSession,
		&pEnablerActivate
 	));


	// TODO:

	// If MFCreatePMPMediaSession fails it might return an IMFActivate pointer.
	// This indicates that a trusted binary failed to load in the protected process.
	// An application can use the IMFActivate pointer to create an enabler object, which 
	// provides revocation and renewal information for the component that failed to
	// load. 

	// This sample does not demonstrate that feature. Instead, we simply treat this
	// case as a playback failure. 


	// Start pulling events from the media session
	CHECK_HR(hr = m_pSession->BeginGetEvent((IMFAsyncCallback *)this,NULL));

	done:
	SAFE_RELEASE(pAttributes);
	SAFE_RELEASE(pEnablerActivate); return hr;
}
///////////////////////////////////////////////////////////////////////
//  Name: CloseSession
//  Description:  Closes the media session. 
//
//  Note: The IMFMediaSession::Close method is asynchronous, but the
//        CPlayer::CloseSession method waits on the MESessionClosed event.
//        The MESessionClosed event is guaranteed to be the last event 
//        that the media session fires.
/////////////////////////////////////////////////////////////////////////
HRESULT CPlayer::CloseSession()
{
	HRESULT hr = S_OK;

	SAFE_RELEASE(m_pVideoDisplay);

	if(m_pSession)
	{
		DWORD dwWaitResult = 0;

		m_state = Closing;

		CHECK_HR(hr = m_pSession->Close());

		// Wait for the close operation to complete

		dwWaitResult = WaitForSingleObject(m_hCloseEvent,5000);

		if(dwWaitResult==WAIT_TIMEOUT)
		TRACE((L"CloseSession timed out!\n"));
		
		// Now there will be no more events from this session.
	}

	// Complete shutdown operations.

	// Shut down the media source. (Synchronous operation, no events.)
	if(m_pSource) m_pSource->Shutdown();	

	// Shut down the media session. (Synchronous operation, no events.)
	if(m_pSession) m_pSession->Shutdown();

	SAFE_RELEASE(m_pSource);
	SAFE_RELEASE(m_pSession);
	SAFE_RELEASE(m_pContentProtectionManager);

	m_state = Closed;

done: return hr;
}
///////////////////////////////////////////////////////////////////////
//  Name: StartPlayback
//  Description:  Starts playback from the current position. 
/////////////////////////////////////////////////////////////////////////
HRESULT CPlayer::StartPlayback()
{
	TRACE((L"CPlayer::StartPlayback\n"));

	assert(m_pSession!=NULL);

	HRESULT hr = S_OK;

	PROPVARIANT varStart;
	PropVariantInit(&varStart);

	varStart.vt = VT_EMPTY;

	hr = m_pSession->Start(&GUID_NULL,&varStart);

	if(SUCCEEDED(hr))
	{
		// Note: Start is an asynchronous operation. However, we
		// can treat our state as being already started. If Start
		// fails later, we'll get an MESessionStarted event with
		// an error code, and we will update our state then.
		m_state = Started;
	}

	PropVariantClear(&varStart); return hr;
}
///////////////////////////////////////////////////////////////////////
//  Name: CreateMediaSource
//  Description:  Create a media source from a URL.
//
//  sURL: The URL to open.
/////////////////////////////////////////////////////////////////////////
HRESULT CPlayer::CreateMediaSource(const WCHAR *sURL)
{
	TRACE((L"CPlayer::CreateMediaSource\n"));

	HRESULT hr = S_OK;
	MF_OBJECT_TYPE ObjectType = MF_OBJECT_INVALID;

	IMFSourceResolver *pSourceResolver = NULL;
	IUnknown *pSource = NULL;

	SAFE_RELEASE(m_pSource);

	// Create the source resolver.
	CHECK_HR(hr = MFCreateSourceResolver(&pSourceResolver));

	// Use the source resolver to create the media source.

	// Note: For simplicity this sample uses the synchronous method on
	// IMFSourceResolver to create the media source. However, creating a 
	// media source can take a noticeable amount of time, especially for
	// a network source. For a more responsive UI, use the asynchronous
	// BeginCreateObjectFromURL method.

	CHECK_HR(hr=pSourceResolver->CreateObjectFromURL
	(
		sURL,                       // URL of the source.
		MF_RESOLUTION_MEDIASOURCE,  // Create a source object.
		NULL,                       // Optional property store.
		&ObjectType,                // Receives the created object type. 
		&pSource                    // Receives a pointer to the media source.
	));

	// Get the IMFMediaSource interface from the media source.
	CHECK_HR(hr = pSource->QueryInterface(__uuidof(IMFMediaSource),(void**)&m_pSource));

done: SAFE_RELEASE(pSourceResolver); SAFE_RELEASE(pSource); return hr;
}
///////////////////////////////////////////////////////////////////////
//  CreateTopologyFromSource
//  Description:  Create a playback topology from the media source.
//
//  Pre-condition: The media source must be created already.
//                 Call CreateMediaSource() before calling this method.
/////////////////////////////////////////////////////////////////////////
HRESULT CPlayer::CreateTopologyFromSource(IMFTopology **ppTopology)
{
	TRACE((L"CPlayer::CreateTopologyFromSource\n"));

	assert(m_pSession!=NULL);
	assert(m_pSource!=NULL);

	HRESULT hr = S_OK;

	IMFTopology *pTopology = NULL;
	IMFPresentationDescriptor *pSourcePD = NULL;
	DWORD cSourceStreams = 0;

	// Create a new topology.
	CHECK_HR(hr = MFCreateTopology(&pTopology));
	// Create the presentation descriptor for the media source.
	CHECK_HR(hr = m_pSource->CreatePresentationDescriptor(&pSourcePD));
	// Get the number of streams in the media source.
	CHECK_HR(hr = pSourcePD->GetStreamDescriptorCount(&cSourceStreams));

	TRACE((L"Stream count: %d\n",cSourceStreams));

	// For each stream, create the topology nodes and add them to the topology.
	for(DWORD i=0;i<cSourceStreams;i++)
	CHECK_HR(hr=AddBranchToPartialTopology(pTopology,pSourcePD,i));

	// Return the IMFTopology pointer to the caller.
	if(SUCCEEDED(hr))
	{
		*ppTopology = pTopology; (*ppTopology)->AddRef();
	}

done: SAFE_RELEASE(pTopology); SAFE_RELEASE(pSourcePD); return hr;
}
///////////////////////////////////////////////////////////////////////
//  Name:  AddBranchToPartialTopology 
//  Description:  Adds a topology branch for one stream.
//
//  pTopology: Pointer to the topology object.
//  pSourcePD: The source's presentation descriptor.
//  iStream: Index of the stream to render.
//
//  Pre-conditions: The topology must be created already.
//
//  Notes: For each stream, we must do the following:
//    1. Create a source node associated with the stream. 
//    2. Create an output node for the renderer. 
//    3. Connect the two nodes.
//  The media session will resolve the topology, so we do not have
//  to worry about decoders or other transforms.
/////////////////////////////////////////////////////////////////////////
HRESULT CPlayer::AddBranchToPartialTopology
(IMFTopology *pTopology, IMFPresentationDescriptor *pSourcePD, DWORD iStream)
{
	TRACE((L"CPlayer::AddBranchToPartialTopology\n"));

	assert(pTopology!=NULL);

	IMFStreamDescriptor *pSourceSD = NULL;
	IMFTopologyNode *pSourceNode = NULL;
	IMFTopologyNode *pOutputNode = NULL;
	BOOL fSelected = FALSE;

	HRESULT hr = S_OK;

	// Get the stream descriptor for this stream.
	CHECK_HR(hr = pSourcePD->GetStreamDescriptorByIndex(iStream,&fSelected,&pSourceSD));

	// Create the topology branch only if the stream is selected.
	// Otherwise, do nothing.
	if(fSelected)
	{
		// Create a source node for this stream.
		CHECK_HR(hr = CreateSourceStreamNode(m_pSource,pSourcePD,pSourceSD,&pSourceNode));

		// Create the output node for the renderer.
		CHECK_HR(hr = CreateOutputNode(pSourceSD,m_hwndVideo,&pOutputNode));

		// Add both nodes to the topology.
		CHECK_HR(hr = pTopology->AddNode(pSourceNode));
		CHECK_HR(hr = pTopology->AddNode(pOutputNode));

		// Connect the source node to the output node.
		CHECK_HR(hr = pSourceNode->ConnectOutput(0,pOutputNode,0));
	}
done:
	// Clean up.
	SAFE_RELEASE(pSourceSD); SAFE_RELEASE(pSourceNode);
	SAFE_RELEASE(pOutputNode); return hr;
}

/// Static functions

//-------------------------------------------------------------------
//  Name: CreateSourceStreamNode
//  Description:  Creates a source-stream node for a stream.
// 
//  pSource: Pointer to the media source that contains the stream.
//  pSourcePD: Presentation descriptor for the media source.
//  pSourceSD: Stream descriptor for the stream.
//  ppNode: Receives a pointer to the new node.
//-------------------------------------------------------------------

static HRESULT CreateSourceStreamNode
(IMFMediaSource *pSource, IMFPresentationDescriptor *pSourcePD, IMFStreamDescriptor *pSourceSD,
IMFTopologyNode **ppNode)
{
	if(!pSource||!pSourcePD||!pSourceSD||!ppNode) return E_POINTER;

	HRESULT hr = S_OK; IMFTopologyNode *pNode = NULL;

	// Create the source-stream node. 
	CHECK_HR(hr=MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE,&pNode));
	// Set attribute: Pointer to the media source.
	CHECK_HR(hr=pNode->SetUnknown(MF_TOPONODE_SOURCE,pSource));
	// Set attribute: Pointer to the presentation descriptor.
	CHECK_HR(hr=pNode->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR,pSourcePD));
	// Set attribute: Pointer to the stream descriptor.
	CHECK_HR(hr=pNode->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR,pSourceSD));

	// Return the IMFTopologyNode pointer to the caller.
	*ppNode = pNode; (*ppNode)->AddRef();

done: SAFE_RELEASE(pNode); return hr;
}
//-------------------------------------------------------------------
//  Name: CreateOutputNode
//  Description:  Create an output node for a stream.
//
//  pSourceSD: Stream descriptor for the stream.
//  ppNode: Receives a pointer to the new node.
//
//  Notes:
//  This function does the following:
//  1. Chooses a renderer based on the media type of the stream.
//  2. Creates an IActivate object for the renderer.
//  3. Creates an output topology node.
//  4. Sets the IActivate pointer on the node.
//-------------------------------------------------------------------
static HRESULT CreateOutputNode
(IMFStreamDescriptor *pSourceSD, HWND hwndVideo, IMFTopologyNode **ppNode)
{
	IMFTopologyNode *pNode = NULL;
	IMFMediaTypeHandler *pHandler = NULL;
	IMFActivate *pRendererActivate = NULL;

	GUID guidMajorType = GUID_NULL;
	HRESULT hr = S_OK;

	// Get the stream ID.
	DWORD streamID = 0;
	pSourceSD->GetStreamIdentifier(&streamID); // Just for debugging, ignore any failures.

	// Get the media type handler for the stream.
	CHECK_HR(hr = pSourceSD->GetMediaTypeHandler(&pHandler));
	// Get the major media type.
	CHECK_HR(hr = pHandler->GetMajorType(&guidMajorType));
	// Create a downstream node.
	CHECK_HR(hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE,&pNode));

	// Create an IMFActivate object for the renderer, based on the media type.
	if(MFMediaType_Audio==guidMajorType)
	{
		// Create the audio renderer.
		TRACE((L"Stream %d: audio stream\n",streamID));
		CHECK_HR(hr = MFCreateAudioRendererActivate(&pRendererActivate));
	}
	else if(MFMediaType_Video==guidMajorType)
	{
		// Create the video renderer.
		TRACE((L"Stream %d: video stream\n",streamID));
		CHECK_HR(hr = MFCreateVideoRendererActivate(hwndVideo,&pRendererActivate));
	}
	else
	{
		TRACE((L"Stream %d: Unknown format\n",streamID));
		CHECK_HR(hr = E_FAIL);
	}

	// Set the IActivate object on the output node.
	CHECK_HR(hr = pNode->SetObject(pRendererActivate));

	// Return the IMFTopologyNode pointer to the caller.
	*ppNode = pNode; (*ppNode)->AddRef();

done:

	SAFE_RELEASE(pNode);
	SAFE_RELEASE(pHandler);
	SAFE_RELEASE(pRendererActivate); return hr;
}

static void Ex_movie_OnContentEnablerMessage(CPlayer *p) //winmain.cpp
{
	ContentProtectionManager *pManager = NULL;

	HRESULT hr = p->GetContentProtectionManager(&pManager);

	if(SUCCEEDED(hr))
	{
		EnablerState state = pManager->GetState();
		HRESULT hrStatus = pManager->GetStatus();   // Status of the last action.

		// EnablerState is defined for this application; it is not a standard
		// Media Foundation enum. It specifies what action the 
		// ContentProtectionManager helper object is requesting.

		switch(state)
		{
		case Enabler_Ready:
			// Start the enable action.
			hr = pManager->DoEnable();
			break;

		case Enabler_SilentInProgress:
			// We are currently in the middle of silent enable.

			// If the status code is NS_E_DRM_LICENSE_NOTACQUIRED,
			// we need to try non-silent enable.
			if(hrStatus==NS_E_DRM_LICENSE_NOTACQUIRED)
			{
				TRACE((L"Silent enabler failed, attempting non-silent.\n"));
				hr = pManager->DoEnable(ForceNonSilent); // Try non-silent this time;
			}
			else
			{
				// Complete the operation. If it succeeded, the content will play.
				// If it failed, the pipeline will queue an event with an error code.
				pManager->CompleteEnable();
			}
			break;

		case Enabler_NonSilentInProgress:
			// We are currently in the middle of non-silent enable. 
			// Either we succeeded or an error occurred. Either way, complete
			// the operation.
			pManager->CompleteEnable();
			break;

		case Enabler_Complete:
			// Nothing to do.
			break;

		default:
			TRACE((L"Unknown EnablerState value! (%d)\n",state));
			assert(false);
			break;
		}

		// If a previous call to DoEnable() failed, complete the operation
		// so that the pipeline will get the correct failure code.
		if(FAILED(hr)) pManager->CompleteEnable();
	}

	SAFE_RELEASE(pManager);
}


/////////////////////////////////////////////////////////////////////////////////////////////

static CPlayer *Ex_movie_player;
void EX::is_playing_movie_WM_SIZE(long w, long h)
{	
	RECT cr; GetClientRect(EX::client,&cr);
	Ex_movie_player->MoveVideo((w-cr.right)/2,(h-cr.bottom)/2,cr.right,cr.bottom);
	InvalidateRect(EX::window,0,1);
	//SendMessage(EX::window,WM_ERASEBKGND,0,0); //HACK
}
void EX::playing_movie(const wchar_t *filename, float volume)
{
	CoInitialize(0);

	Ex_movie_playing = true; //hmmm: best timing??

	HWND hwnd = EX::window; //TESTING

	//REMINDER: there's no picture in the D3D flip-ex window
	if(hwnd!=EX::client) ShowWindow(EX::client,SW_HIDE);

	CPlayer *p = nullptr;
	HRESULT hr = CPlayer::CreateInstance(hwnd,hwnd,&p);
	Ex_movie_player = p;

	RECT cr; GetClientRect(hwnd,&cr);
	EX::is_playing_movie_WM_SIZE(cr.right,cr.bottom);
	GetClientRect(EX::client,&cr);
	if(!hr) //void OnOpenURL(HWND hwnd)
	{
		// Pass in an OpenUrlDialogInfo structure to the dialog. The dialog 
		// fills in this structure with the URL. The dialog proc allocates
		// the memory for the string. 
	//	OpenUrlDialogInfo url = {}; 
		// Show the Open URL dialog.
	//	if(IDOK==DialogBoxParam(hInst,MAKEINTRESOURCE(IDD_OPENURL),hwnd,OpenUrlDialogProc,(LPARAM)&url))
		{
			// Open the file with the playback object.
	//		hr = p->OpenURL(url.pszURL);
			hr = p->OpenURL(filename);

			if(!SUCCEEDED(hr)) //OpenPending?
			{
				//NotifyError(hwnd,L"Could not open this URL.",hr);
				//UpdateUI(hwnd,Ready);
				assert(p->GetState()==Ready);
			}
			else assert(p->GetState()==OpenPending);
		}

		// The app must release the string.
	//	CoTaskMemFree(url.pszURL);
	}

	p->SetMasterVolume(volume);

	DWORD ticks = EX::tick();

	int sleeping = 0, sleep = 200; DWORD test = 0;

	char dinput[256]; for(;;)
	{
		auto st = p->GetState();

		if(!st||st==Stopped) Ex_movie_stop_playing = true;

		if(Ex_movie_stop_playing) break;

	//	g_mediaSeeking->GetCurrentPosition(&position);	
		//REMINDER: Nvidia requires manually prompting repaint
		//after the first play (subsequent plays) so I've stuck
		//InvalidateRect inside PresentImage

	  ////From som.status.cpp////////////

		MSG msg; //VK_PAUSE/courtesy
		while(PeekMessage(&msg,0,0,0,PM_REMOVE))
		{ 
//			if(msg.hwnd==DDRAW::window)
//			if(lpIVMRWindowlessControl9)
			switch(msg.message)
			{
			case WM_PAINT:
			
				if(msg.hwnd!=hwnd) break;

//				if(g_bRepaintClient)
				switch(p->GetState())
				{					
				case Started:				
				case Stopped: case Paused: 
					
					p->Repaint(); 
					
					if(msg.wParam!='clr') break;

				default: //Ex.window.cpp?

					/*PAINTSTRUCT ps; 
					HDC	hdc = BeginPaint(hwnd,&ps); 

					RECT rc;
					GetClientRect(hwnd,&rc);

					if(msg.wParam=='clr') //HACK
					ExcludeClipRect(hdc,(rc.right-cr.right)/2,(rc.bottom-cr.bottom)/2,cr.right,cr.bottom);

					// The video is not playing, so we must paint the application window.					
					FillRect(hdc,&rc,(HBRUSH)COLOR_BACKGROUND);

					EndPaint(msg.hwnd,&ps);*/
					break;
				}
				break;
						
			case WM_SIZE:
			//case WM_SIZING: //TESTING
			//	p->MoveVideo((LOWORD(msg.lParam)-cr.right)/2,(HIWORD(msg.lParam)-cr.bottom)/2,cr.right,cr.bottom);

				assert(0); //EX::is_playing_movie_WM_SIZE
				break;

			case WM_ERASEBKGND:
				// Suppress window erasing, to reduce flickering while the video is playing.
				continue;

			case WM_APP_PLAYER_EVENT:
				
				//PostMessage is unreliable when dragging/resizing
				//assert(0);
				break;

				//OnPlayerEvent(hwnd,wParam);
				{
					hr = p->HandleEvent(msg.wParam); //pUnkPtr

					if(FAILED(hr))
					{
						//NotifyError(hwnd,L"An error occurred.",hr);
						assert(!"Media Foundation error");
					}
				}
				continue;

			case WM_APP_CONTENT_ENABLER:

				Ex_movie_OnContentEnablerMessage(p);
				continue;
			}

			//if(!TranslateAccelerator(EX::window,haccel,&msg))
			{
				TranslateMessage(&msg);	DispatchMessage(&msg); 
			}
		}		

		DWORD delta = EX::tick();

		sleeping+=delta-ticks; ticks = delta;

		if(sleeping>sleep) 
		{
			sleeping = 0;

			if(DINPUT::Keyboard) //mimic SOM
			{
				char dinput[256];   
				DINPUT::Keyboard->Acquire(); 
				DINPUT::Keyboard->GetDeviceState(256,dinput); 
			}
			if(DINPUT::Joystick) //mimic SOM
			{
				DX::DIJOYSTATE st; DINPUT::Joystick->Acquire(); 
				if(DINPUT::Joystick->as2A) DINPUT::Joystick->as2A->Poll();	 
				DINPUT::Joystick->GetDeviceState(sizeof(st),&st);
			}
		}		
	}

	if(hwnd!=EX::client) ShowWindow(EX::client,SW_SHOW);

	Ex_movie_playing = false; //hmmm: best timing??

	Ex_movie_stop_playing = false; //reset

	if(p)
    {
        p->Shutdown(); SAFE_RELEASE(p);
    }

	CoUninitialize();
}
