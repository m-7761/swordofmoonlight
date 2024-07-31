#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

//https://github.com/mohabouje/WinToast (I want to thank mohabouje somehow)

/* * Copyright (C) 2016-2023 Mohammed Boujemaoui <mohabouje@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

	#ifndef WINTOASTLIB_H
	#define WINTOASTLIB_H

	#include <Windows.h>
	#include <sdkddkver.h>
	#include <WinUser.h>
	#include <ShObjIdl.h>
	#include <wrl/implements.h>
	#include <wrl/event.h>
	#include <windows.ui.notifications.h>
	#include <strsafe.h>
	#include <Psapi.h>
	#include <ShlObj.h>
	#include <roapi.h>
	#include <propvarutil.h>
	#include <functiondiscoverykeys.h>
	#include <iostream>
	#include <winstring.h>
	#include <string.h>
	#include <vector>
	#include <map>
	#include <memory>

	using namespace Microsoft::WRL;
using namespace ABI::Windows::Data::Xml::Dom;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::UI::Notifications;
using namespace Windows::Foundation;

namespace WinToastLib
{

	class IWinToastHandler
	{
	public:
		enum WinToastDismissalReason
		{
			UserCanceled = ToastDismissalReason::ToastDismissalReason_UserCanceled,
			ApplicationHidden = ToastDismissalReason::ToastDismissalReason_ApplicationHidden,
			TimedOut = ToastDismissalReason::ToastDismissalReason_TimedOut
		};

		virtual ~IWinToastHandler() = default;
		virtual void toastActivated() const = 0;
		virtual void toastActivated(int actionIndex) const = 0;
		virtual void toastDismissed(WinToastDismissalReason state) const = 0;
		virtual void toastFailed() const = 0;
	};

	class WinToastTemplate
	{
	public:
		enum class Scenario { Default,Alarm,IncomingCall,Reminder };
		enum Duration { System,Short,Long };
		enum AudioOption { Default = 0,Silent,Loop };
		enum TextField { FirstLine = 0,SecondLine,ThirdLine };

		enum WinToastTemplateType
		{
			ImageAndText01 = ToastTemplateType::ToastTemplateType_ToastImageAndText01,
			ImageAndText02 = ToastTemplateType::ToastTemplateType_ToastImageAndText02,
			ImageAndText03 = ToastTemplateType::ToastTemplateType_ToastImageAndText03,
			ImageAndText04 = ToastTemplateType::ToastTemplateType_ToastImageAndText04,
			Text01 = ToastTemplateType::ToastTemplateType_ToastText01,
			Text02 = ToastTemplateType::ToastTemplateType_ToastText02,
			Text03 = ToastTemplateType::ToastTemplateType_ToastText03,
			Text04 = ToastTemplateType::ToastTemplateType_ToastText04
		};

		enum AudioSystemFile
		{
			DefaultSound,
			IM,
			Mail,
			Reminder,
			SMS,
			Alarm,
			Alarm2,
			Alarm3,
			Alarm4,
			Alarm5,
			Alarm6,
			Alarm7,
			Alarm8,
			Alarm9,
			Alarm10,
			Call,
			Call1,
			Call2,
			Call3,
			Call4,
			Call5,
			Call6,
			Call7,
			Call8,
			Call9,
			Call10,
		};

		enum CropHint
		{
			Square,
			Circle,
		};

		WinToastTemplate(_In_ WinToastTemplateType type = WinToastTemplateType::ImageAndText02);
		~WinToastTemplate();

		void setFirstLine(_In_ std::wstring const &text);
		void setSecondLine(_In_ std::wstring const &text);
		void setThirdLine(_In_ std::wstring const &text);
		void setTextField(_In_ std::wstring const &txt,_In_ TextField pos);
		void setAttributionText(_In_ std::wstring const &attributionText);
		void setImagePath(_In_ std::wstring const &imgPath,_In_ CropHint cropHint = CropHint::Square);
		void setHeroImagePath(_In_ std::wstring const &imgPath,_In_ bool inlineImage = false);
		void setAudioPath(_In_ WinToastTemplate::AudioSystemFile audio);
		void setAudioPath(_In_ std::wstring const &audioPath);
		void setAudioOption(_In_ WinToastTemplate::AudioOption audioOption);
		void setDuration(_In_ Duration duration);
		void setExpiration(_In_ INT64 millisecondsFromNow);
		void setScenario(_In_ Scenario scenario);
		void addAction(_In_ std::wstring const &label);

		std::size_t textFieldsCount() const;
		std::size_t actionsCount() const;
		bool hasImage() const;
		bool hasHeroImage() const;
		std::vector<std::wstring> const &textFields() const;
		std::wstring const &textField(_In_ TextField pos) const;
		std::wstring const &actionLabel(_In_ std::size_t pos) const;
		std::wstring const &imagePath() const;
		std::wstring const &heroImagePath() const;
		std::wstring const &audioPath() const;
		std::wstring const &attributionText() const;
		std::wstring const &scenario() const;
		INT64 expiration() const;
		WinToastTemplateType type() const;
		WinToastTemplate::AudioOption audioOption() const;
		Duration duration() const;
		bool isToastGeneric() const;
		bool isInlineHeroImage() const;
		bool isCropHintCircle() const;

	private:
		std::vector<std::wstring> _textFields{};
		std::vector<std::wstring> _actions{};
		std::wstring _imagePath{};
		std::wstring _heroImagePath{};
		bool _inlineHeroImage{ false };
		std::wstring _audioPath{};
		std::wstring _attributionText{};
		std::wstring _scenario{ L"Default" };
		INT64 _expiration{ 0 };
		AudioOption _audioOption{ WinToastTemplate::AudioOption::Default };
		WinToastTemplateType _type{ WinToastTemplateType::Text01 };
		Duration _duration{ Duration::System };
		CropHint _cropHint{ CropHint::Square };
	};

	class WinToast
	{
	public:
		enum WinToastError
		{
			NoError = 0,
			NotInitialized,
			SystemNotSupported,
			ShellLinkNotCreated,
			InvalidAppUserModelID,
			InvalidParameters,
			InvalidHandler,
			NotDisplayed,
			UnknownError
		};

		enum ShortcutResult
		{
			SHORTCUT_UNCHANGED = 0,
			SHORTCUT_WAS_CHANGED = 1,
			SHORTCUT_WAS_CREATED = 2,

			SHORTCUT_MISSING_PARAMETERS = -1,
			SHORTCUT_INCOMPATIBLE_OS = -2,
			SHORTCUT_COM_INIT_FAILURE = -3,
			SHORTCUT_CREATE_FAILED = -4
		};

		enum ShortcutPolicy
		{
			/* Don't check, create, or modify a shortcut. */
			SHORTCUT_POLICY_IGNORE = 0,
			/* Require a shortcut with matching AUMI, don't create or modify an existing one. */
			SHORTCUT_POLICY_REQUIRE_NO_CREATE = 1,
			/* Require a shortcut with matching AUMI, create if missing, modify if not matching. This is the default. */
			SHORTCUT_POLICY_REQUIRE_CREATE = 2,
		};

		WinToast(void);
		virtual ~WinToast();
		static WinToast *instance();
		static bool isCompatible();
		static bool isSupportingModernFeatures();
		static bool isWin10AnniversaryOrHigher();
		static std::wstring configureAUMI(_In_ std::wstring const &companyName,_In_ std::wstring const &productName,
										  _In_ std::wstring const &subProduct = std::wstring(),
										  _In_ std::wstring const &versionInformation = std::wstring());
		static std::wstring const &strerror(_In_ WinToastError error);
		virtual bool initialize(_Out_opt_ WinToastError *error = nullptr);
		virtual bool isInitialized() const;
		virtual bool hideToast(_In_ INT64 id);
		virtual INT64 showToast(_In_ WinToastTemplate const &toast,_In_ IWinToastHandler *eventHandler,
								_Out_opt_ WinToastError *error = nullptr);
		virtual void clear();
		virtual enum ShortcutResult createShortcut();

		std::wstring const &appName() const;
		std::wstring const &appUserModelId() const;
		void setAppUserModelId(_In_ std::wstring const &aumi);
		void setAppName(_In_ std::wstring const &appName);
		void setShortcutPolicy(_In_ ShortcutPolicy policy);

	protected:
		struct NotifyData
		{
			NotifyData() {};
			NotifyData(_In_ ComPtr<IToastNotification> notify,_In_ EventRegistrationToken activatedToken,
					   _In_ EventRegistrationToken dismissedToken,_In_ EventRegistrationToken failedToken) :
				_notify(notify),_activatedToken(activatedToken),_dismissedToken(dismissedToken),_failedToken(failedToken)
			{}

			~NotifyData()
			{
				RemoveTokens();
			}

			void RemoveTokens()
			{
				if(!_readyForDeletion)
				{
					return;
				}

				if(_previouslyTokenRemoved)
				{
					return;
				}

				if(!_notify.Get())
				{
					return;
				}

				_notify->remove_Activated(_activatedToken);
				_notify->remove_Dismissed(_dismissedToken);
				_notify->remove_Failed(_failedToken);
				_previouslyTokenRemoved = true;
			}

			void markAsReadyForDeletion()
			{
				_readyForDeletion = true;
			}

			bool isReadyForDeletion() const
			{
				return _readyForDeletion;
			}

			IToastNotification *notification()
			{
				return _notify.Get();
			}

		private:
			ComPtr<IToastNotification> _notify{ nullptr };
			EventRegistrationToken _activatedToken{};
			EventRegistrationToken _dismissedToken{};
			EventRegistrationToken _failedToken{};
			bool _readyForDeletion{ false };
			bool _previouslyTokenRemoved{ false };
		};

		bool _isInitialized{ false };
		bool _hasCoInitialized{ false };
		ShortcutPolicy _shortcutPolicy{ SHORTCUT_POLICY_REQUIRE_CREATE };
		std::wstring _appName{};
		std::wstring _aumi{};
		std::map<INT64,NotifyData> _buffer{};

		void markAsReadyForDeletion(_In_ INT64 id);
		HRESULT validateShellLinkHelper(_Out_ bool &wasChanged);
		HRESULT createShellLinkHelper();
		HRESULT setImageFieldHelper(_In_ IXmlDocument *xml,_In_ std::wstring const &path,_In_ bool isToastGeneric,bool isCropHintCircle);
		HRESULT setHeroImageHelper(_In_ IXmlDocument *xml,_In_ std::wstring const &path,_In_ bool isInlineImage);
		HRESULT setBindToastGenericHelper(_In_ IXmlDocument *xml);
		HRESULT
			setAudioFieldHelper(_In_ IXmlDocument *xml,_In_ std::wstring const &path,
								_In_opt_ WinToastTemplate::AudioOption option = WinToastTemplate::AudioOption::Default);
		HRESULT setTextFieldHelper(_In_ IXmlDocument *xml,_In_ std::wstring const &text,_In_ UINT32 pos);
		HRESULT setAttributionTextFieldHelper(_In_ IXmlDocument *xml,_In_ std::wstring const &text);
		HRESULT addActionHelper(_In_ IXmlDocument *xml,_In_ std::wstring const &action,_In_ std::wstring const &arguments);
		HRESULT addDurationHelper(_In_ IXmlDocument *xml,_In_ std::wstring const &duration);
		HRESULT addScenarioHelper(_In_ IXmlDocument *xml,_In_ std::wstring const &scenario);
		ComPtr<IToastNotifier> notifier(_In_ bool *succeded) const;
		void setError(_Out_opt_ WinToastError *error,_In_ WinToastError value);
	};
} // namespace WinToastLib
#endif // WINTOASTLIB_H

/* * Copyright (C) 2016-2023 Mohammed Boujemaoui <mohabouje@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

 //#include "wintoastlib.h"

#include <memory>
#include <assert.h>
#include <unordered_map>
#include <array>
#include <functional>

#pragma comment(lib, "shlwapi")
#pragma comment(lib, "user32")

#ifdef NDEBUG
#    define DEBUG_MSG(str)                                                                                                                 \
        do {                                                                                                                               \
        } while (false)
#else
#    define DEBUG_MSG(str)                                                                                                                 \
        do {                                                                                                                               \
            std::wcout << str << std::endl;                                                                                                \
        } while (false)
#endif

#define DEFAULT_SHELL_LINKS_PATH L"\\Microsoft\\Windows\\Start Menu\\Programs\\"
#define DEFAULT_LINK_FORMAT      L".lnk"
#define STATUS_SUCCESS           (0x00000000)

// Quickstart: Handling toast activations from Win32 apps in Windows 10
// https://blogs.msdn.microsoft.com/tiles_and_toasts/2015/10/16/quickstart-handling-toast-activations-from-win32-apps-in-windows-10/
using namespace WinToastLib;
namespace DllImporter
{

	// Function load a function from library
	template <typename Function>
	HRESULT loadFunctionFromLibrary(HINSTANCE library,LPCSTR name,Function &func)
	{
		if(!library)
		{
			return E_INVALIDARG;
		}
		func = reinterpret_cast<Function>(GetProcAddress(library,name));
		return (func!=nullptr)?S_OK:E_FAIL;
	}

	typedef HRESULT(FAR STDAPICALLTYPE *f_SetCurrentProcessExplicitAppUserModelID)(__in PCWSTR AppID);
	typedef HRESULT(FAR STDAPICALLTYPE *f_PropVariantToString)(_In_ REFPROPVARIANT propvar,_Out_writes_(cch) PWSTR psz,_In_ UINT cch);
	typedef HRESULT(FAR STDAPICALLTYPE *f_RoGetActivationFactory)(_In_ HSTRING activatableClassId,_In_ REFIID iid,
																  _COM_Outptr_ void **factory);
	typedef HRESULT(FAR STDAPICALLTYPE *f_WindowsCreateStringReference)(_In_reads_opt_(length+1) PCWSTR sourceString,UINT32 length,
																		_Out_ HSTRING_HEADER *hstringHeader,
																		_Outptr_result_maybenull_ _Result_nullonfailure_ HSTRING *string);
	typedef PCWSTR(FAR STDAPICALLTYPE *f_WindowsGetStringRawBuffer)(_In_ HSTRING string,_Out_opt_ UINT32 *length);
	typedef HRESULT(FAR STDAPICALLTYPE *f_WindowsDeleteString)(_In_opt_ HSTRING string);

	static f_SetCurrentProcessExplicitAppUserModelID SetCurrentProcessExplicitAppUserModelID;
	static f_PropVariantToString PropVariantToString;
	static f_RoGetActivationFactory RoGetActivationFactory;
	static f_WindowsCreateStringReference WindowsCreateStringReference;
	static f_WindowsGetStringRawBuffer WindowsGetStringRawBuffer;
	static f_WindowsDeleteString WindowsDeleteString;

	template <class T>
	__inline _Check_return_ HRESULT _1_GetActivationFactory(_In_ HSTRING activatableClassId,_COM_Outptr_ T **factory)
	{
		return RoGetActivationFactory(activatableClassId,IID_INS_ARGS(factory));
	}

	template <typename T>
	inline HRESULT Wrap_GetActivationFactory(_In_ HSTRING activatableClassId,_Inout_ Details::ComPtrRef<T> factory) noexcept
	{
		return _1_GetActivationFactory(activatableClassId,factory.ReleaseAndGetAddressOf());
	}

	inline HRESULT initialize()
	{
		HINSTANCE LibShell32 = LoadLibraryW(L"SHELL32.DLL");
		HRESULT hr =
			loadFunctionFromLibrary(LibShell32,"SetCurrentProcessExplicitAppUserModelID",SetCurrentProcessExplicitAppUserModelID);
		if(SUCCEEDED(hr))
		{
			HINSTANCE LibPropSys = LoadLibraryW(L"PROPSYS.DLL");
			hr = loadFunctionFromLibrary(LibPropSys,"PropVariantToString",PropVariantToString);
			if(SUCCEEDED(hr))
			{
				HINSTANCE LibComBase = LoadLibraryW(L"COMBASE.DLL");
				bool const succeded =
					SUCCEEDED(loadFunctionFromLibrary(LibComBase,"RoGetActivationFactory",RoGetActivationFactory))&&
					SUCCEEDED(loadFunctionFromLibrary(LibComBase,"WindowsCreateStringReference",WindowsCreateStringReference))&&
					SUCCEEDED(loadFunctionFromLibrary(LibComBase,"WindowsGetStringRawBuffer",WindowsGetStringRawBuffer))&&
					SUCCEEDED(loadFunctionFromLibrary(LibComBase,"WindowsDeleteString",WindowsDeleteString));
				return succeded?S_OK:E_FAIL;
			}
		}
		return hr;
	}
} // namespace DllImporter

class WinToastStringWrapper
{
public:
	WinToastStringWrapper(_In_reads_(length) PCWSTR stringRef,_In_ UINT32 length) noexcept
	{
		HRESULT hr = DllImporter::WindowsCreateStringReference(stringRef,length,&_header,&_hstring);
		if(!SUCCEEDED(hr))
		{
			RaiseException(static_cast<DWORD>(STATUS_INVALID_PARAMETER),EXCEPTION_NONCONTINUABLE,0,nullptr);
		}
	}

	WinToastStringWrapper(_In_ std::wstring const &stringRef) noexcept
	{
		HRESULT hr =
			DllImporter::WindowsCreateStringReference(stringRef.c_str(),static_cast<UINT32>(stringRef.length()),&_header,&_hstring);
		if(FAILED(hr))
		{
			RaiseException(static_cast<DWORD>(STATUS_INVALID_PARAMETER),EXCEPTION_NONCONTINUABLE,0,nullptr);
		}
	}

	~WinToastStringWrapper()
	{
		DllImporter::WindowsDeleteString(_hstring);
	}

	inline HSTRING Get() const noexcept
	{
		return _hstring;
	}

private:
	HSTRING _hstring;
	HSTRING_HEADER _header;
};

class InternalDateTime : public IReference<DateTime>
{
public:
	static INT64 Now()
	{
		FILETIME now;
		GetSystemTimeAsFileTime(&now);
		return ((((INT64)now.dwHighDateTime)<<32)|now.dwLowDateTime);
	}

	InternalDateTime(DateTime dateTime) : _dateTime(dateTime) {}

	InternalDateTime(INT64 millisecondsFromNow)
	{
		_dateTime.UniversalTime = Now()+millisecondsFromNow*10000;
	}

	virtual ~InternalDateTime() = default;

	operator INT64()
	{
		return _dateTime.UniversalTime;
	}

	HRESULT STDMETHODCALLTYPE get_Value(DateTime *dateTime)
	{
		*dateTime = _dateTime;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE QueryInterface(const IID &riid,void **ppvObject)
	{
		if(!ppvObject)
		{
			return E_POINTER;
		}
		if(riid==__uuidof(IUnknown)||riid==__uuidof(IReference<DateTime>))
		{
			*ppvObject = static_cast<IUnknown *>(static_cast<IReference<DateTime>*>(this));
			return S_OK;
		}
		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE Release()
	{
		return 1;
	}

	ULONG STDMETHODCALLTYPE AddRef()
	{
		return 2;
	}

	HRESULT STDMETHODCALLTYPE GetIids(ULONG *,IID **)
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE GetRuntimeClassName(HSTRING *)
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE GetTrustLevel(TrustLevel *)
	{
		return E_NOTIMPL;
	}

protected:
	DateTime _dateTime;
};

namespace Util
{

	typedef LONG NTSTATUS,*PNTSTATUS;
	typedef NTSTATUS(WINAPI *RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
	inline RTL_OSVERSIONINFOW getRealOSVersion()
	{
		HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
		if(hMod)
		{
			RtlGetVersionPtr fxPtr = (RtlGetVersionPtr)::GetProcAddress(hMod,"RtlGetVersion");
			if(fxPtr!=nullptr)
			{
				RTL_OSVERSIONINFOW rovi = { 0 };
				rovi.dwOSVersionInfoSize = sizeof(rovi);
				if(STATUS_SUCCESS==fxPtr(&rovi))
				{
					return rovi;
				}
			}
		}
		RTL_OSVERSIONINFOW rovi = { 0 };
		return rovi;
	}

	inline HRESULT defaultExecutablePath(_In_ WCHAR *path,_In_ DWORD nSize = MAX_PATH)
	{
		DWORD written = GetModuleFileNameExW(GetCurrentProcess(),nullptr,path,nSize);
		DEBUG_MSG("Default executable path: "<<path);
		return (written>0)?S_OK:E_FAIL;
	}

	inline HRESULT defaultShellLinksDirectory(_In_ WCHAR *path,_In_ DWORD nSize = MAX_PATH)
	{
		DWORD written = GetEnvironmentVariableW(L"APPDATA",path,nSize);
		HRESULT hr = written>0?S_OK:E_INVALIDARG;
		if(SUCCEEDED(hr))
		{
			errno_t result = wcscat_s(path,nSize,DEFAULT_SHELL_LINKS_PATH);
			hr = (result==0)?S_OK:E_INVALIDARG;
			DEBUG_MSG("Default shell link path: "<<path);
		}
		return hr;
	}

	inline HRESULT defaultShellLinkPath(_In_ std::wstring const &appname,_In_ WCHAR *path,_In_ DWORD nSize = MAX_PATH)
	{
		HRESULT hr = defaultShellLinksDirectory(path,nSize);
		if(SUCCEEDED(hr))
		{
			const std::wstring appLink(appname+DEFAULT_LINK_FORMAT);
			errno_t result = wcscat_s(path,nSize,appLink.c_str());
			hr = (result==0)?S_OK:E_INVALIDARG;
			DEBUG_MSG("Default shell link file path: "<<path);
		}
		return hr;
	}

	inline PCWSTR AsString(_In_ ComPtr<IXmlDocument> &xmlDocument)
	{
		HSTRING xml;
		ComPtr<IXmlNodeSerializer> ser;
		HRESULT hr = xmlDocument.As<IXmlNodeSerializer>(&ser);
		hr = ser->GetXml(&xml);
		if(SUCCEEDED(hr))
		{
			return DllImporter::WindowsGetStringRawBuffer(xml,nullptr);
		}
		return nullptr;
	}

	inline PCWSTR AsString(_In_ HSTRING hstring)
	{
		return DllImporter::WindowsGetStringRawBuffer(hstring,nullptr);
	}

	inline HRESULT setNodeStringValue(_In_ std::wstring const &string,_Out_opt_ IXmlNode *node,_Out_ IXmlDocument *xml)
	{
		ComPtr<IXmlText> textNode;
		HRESULT hr = xml->CreateTextNode(WinToastStringWrapper(string).Get(),&textNode);
		if(SUCCEEDED(hr))
		{
			ComPtr<IXmlNode> stringNode;
			hr = textNode.As(&stringNode);
			if(SUCCEEDED(hr))
			{
				ComPtr<IXmlNode> appendedChild;
				hr = node->AppendChild(stringNode.Get(),&appendedChild);
			}
		}
		return hr;
	}

	template<typename FunctorT>
	inline HRESULT setEventHandlers(_In_ IToastNotification *notification,_In_ std::shared_ptr<IWinToastHandler> eventHandler,
									_In_ INT64 expirationTime,_Out_ EventRegistrationToken &activatedToken,
									_Out_ EventRegistrationToken &dismissedToken,_Out_ EventRegistrationToken &failedToken,
									_In_ FunctorT &&markAsReadyForDeletionFunc)
	{
		HRESULT hr = notification->add_Activated(
			Callback<Implements<RuntimeClassFlags<ClassicCom>,ITypedEventHandler<ToastNotification *,IInspectable *>>>(
			[eventHandler,markAsReadyForDeletionFunc](IToastNotification *notify,IInspectable *inspectable)
		{
			ComPtr<IToastActivatedEventArgs> activatedEventArgs;
			HRESULT hr = inspectable->QueryInterface(activatedEventArgs.GetAddressOf());
			if(SUCCEEDED(hr))
			{
				HSTRING argumentsHandle;
				hr = activatedEventArgs->get_Arguments(&argumentsHandle);
				if(SUCCEEDED(hr))
				{
					PCWSTR arguments = Util::AsString(argumentsHandle);
					if(arguments&&*arguments)
					{
						eventHandler->toastActivated(static_cast<int>(wcstol(arguments,nullptr,10)));
						DllImporter::WindowsDeleteString(argumentsHandle);
						markAsReadyForDeletionFunc();
						return S_OK;
					}
					DllImporter::WindowsDeleteString(argumentsHandle);
				}
			}
			eventHandler->toastActivated();
			markAsReadyForDeletionFunc();
			return S_OK;
		})
				.Get(),
			&activatedToken);

		if(SUCCEEDED(hr))
		{
			hr = notification->add_Dismissed(
				Callback<Implements<RuntimeClassFlags<ClassicCom>,ITypedEventHandler<ToastNotification *,ToastDismissedEventArgs *>>>(
				[eventHandler,expirationTime,markAsReadyForDeletionFunc](IToastNotification *notify,IToastDismissedEventArgs *e)
			{
				ToastDismissalReason reason;
				if(SUCCEEDED(e->get_Reason(&reason)))
				{
					if(reason==ToastDismissalReason_UserCanceled&&expirationTime&&
						InternalDateTime::Now()>=expirationTime)
					{
						reason = ToastDismissalReason_TimedOut;
					}
					eventHandler->toastDismissed(static_cast<IWinToastHandler::WinToastDismissalReason>(reason));
				}
				markAsReadyForDeletionFunc();
				return S_OK;
			})
					.Get(),
				&dismissedToken);
			if(SUCCEEDED(hr))
			{
				hr = notification->add_Failed(
					Callback<Implements<RuntimeClassFlags<ClassicCom>,ITypedEventHandler<ToastNotification *,ToastFailedEventArgs *>>>(
					[eventHandler,markAsReadyForDeletionFunc](IToastNotification *notify,IToastFailedEventArgs *e)
				{
					eventHandler->toastFailed();
					markAsReadyForDeletionFunc();
					return S_OK;
				})
						.Get(),
					&failedToken);
			}
		}
		return hr;
	}

	inline HRESULT addAttribute(_In_ IXmlDocument *xml,std::wstring const &name,IXmlNamedNodeMap *attributeMap)
	{
		ComPtr<ABI::Windows::Data::Xml::Dom::IXmlAttribute> srcAttribute;
		HRESULT hr = xml->CreateAttribute(WinToastStringWrapper(name).Get(),&srcAttribute);
		if(SUCCEEDED(hr))
		{
			ComPtr<IXmlNode> node;
			hr = srcAttribute.As(&node);
			if(SUCCEEDED(hr))
			{
				ComPtr<IXmlNode> pNode;
				hr = attributeMap->SetNamedItem(node.Get(),&pNode);
			}
		}
		return hr;
	}

	inline HRESULT createElement(_In_ IXmlDocument *xml,_In_ std::wstring const &root_node,_In_ std::wstring const &element_name,
								 _In_ std::vector<std::wstring> const &attribute_names)
	{
		ComPtr<IXmlNodeList> rootList;
		HRESULT hr = xml->GetElementsByTagName(WinToastStringWrapper(root_node).Get(),&rootList);
		if(SUCCEEDED(hr))
		{
			ComPtr<IXmlNode> root;
			hr = rootList->Item(0,&root);
			if(SUCCEEDED(hr))
			{
				ComPtr<ABI::Windows::Data::Xml::Dom::IXmlElement> audioElement;
				hr = xml->CreateElement(WinToastStringWrapper(element_name).Get(),&audioElement);
				if(SUCCEEDED(hr))
				{
					ComPtr<IXmlNode> audioNodeTmp;
					hr = audioElement.As(&audioNodeTmp);
					if(SUCCEEDED(hr))
					{
						ComPtr<IXmlNode> audioNode;
						hr = root->AppendChild(audioNodeTmp.Get(),&audioNode);
						if(SUCCEEDED(hr))
						{
							ComPtr<IXmlNamedNodeMap> attributes;
							hr = audioNode->get_Attributes(&attributes);
							if(SUCCEEDED(hr))
							{
								for(auto const &it:attribute_names)
								{
									hr = addAttribute(xml,it,attributes.Get());
								}
							}
						}
					}
				}
			}
		}
		return hr;
	}
} // namespace Util

WinToast *WinToast::instance()
{
	//DEFEATING ~WinToast CRASH
	//static WinToast instance;
	//return &instance;
	static WinToast *instance = new WinToast;
	return instance;
}

WinToast::WinToast() : _isInitialized(false),_hasCoInitialized(false)
{
	if(!isCompatible())
	{
		DEBUG_MSG(L"Warning: Your system is not compatible with this library ");
	}
}

WinToast::~WinToast()
{
	clear();

	if(_hasCoInitialized)
	{
		CoUninitialize();
	}
}

void WinToast::setAppName(_In_ std::wstring const &appName)
{
	_appName = appName;
}

void WinToast::setAppUserModelId(_In_ std::wstring const &aumi)
{
	_aumi = aumi;
	DEBUG_MSG(L"Default App User Model Id: "<<_aumi.c_str());
}

void WinToast::setShortcutPolicy(_In_ ShortcutPolicy shortcutPolicy)
{
	_shortcutPolicy = shortcutPolicy;
}

bool WinToast::isCompatible()
{
	DllImporter::initialize();
	return !((DllImporter::SetCurrentProcessExplicitAppUserModelID==nullptr)||(DllImporter::PropVariantToString==nullptr)||
			 (DllImporter::RoGetActivationFactory==nullptr)||(DllImporter::WindowsCreateStringReference==nullptr)||
			 (DllImporter::WindowsDeleteString==nullptr));
}

bool WinToastLib::WinToast::isSupportingModernFeatures()
{
	constexpr auto MinimumSupportedVersion = 6;
	return Util::getRealOSVersion().dwMajorVersion>MinimumSupportedVersion;
}

bool WinToastLib::WinToast::isWin10AnniversaryOrHigher()
{
	return Util::getRealOSVersion().dwBuildNumber>=14393;
}

std::wstring WinToast::configureAUMI(_In_ std::wstring const &companyName,_In_ std::wstring const &productName,
									 _In_ std::wstring const &subProduct,_In_ std::wstring const &versionInformation)
{
	std::wstring aumi = companyName;
	aumi += L"."+productName;
	if(subProduct.length()>0)
	{
		aumi += L"."+subProduct;
		if(versionInformation.length()>0)
		{
			aumi += L"."+versionInformation;
		}
	}

	if(aumi.length()>SCHAR_MAX)
	{
		DEBUG_MSG("Error: max size allowed for AUMI: 128 characters.");
	}
	return aumi;
}

std::wstring const &WinToast::strerror(WinToastError error)
{
	static const std::unordered_map<WinToastError,std::wstring> Labels = {
		{ WinToastError::NoError,L"No error. The process was executed correctly" },
		{ WinToastError::NotInitialized,L"The library has not been initialized" },
		{ WinToastError::SystemNotSupported,L"The OS does not support WinToast" },
		{ WinToastError::ShellLinkNotCreated,L"The library was not able to create a Shell Link for the app" },
		{ WinToastError::InvalidAppUserModelID,L"The AUMI is not a valid one" },
		{ WinToastError::InvalidParameters,L"Invalid parameters, please double-check the AUMI or App Name" },
		{ WinToastError::NotDisplayed,L"The toast was created correctly but WinToast was not able to display the toast" },
		{ WinToastError::UnknownError,L"Unknown error" }
	};

	auto const iter = Labels.find(error);
	assert(iter!=Labels.end());
	return iter->second;
}

enum WinToast::ShortcutResult WinToast::createShortcut()
{
	if(_aumi.empty()||_appName.empty())
	{
		DEBUG_MSG(L"Error: App User Model Id or Appname is empty!");
		return SHORTCUT_MISSING_PARAMETERS;
	}

	if(!isCompatible())
	{
		DEBUG_MSG(L"Your OS is not compatible with this library! =(");
		return SHORTCUT_INCOMPATIBLE_OS;
	}

	if(!_hasCoInitialized)
	{
		HRESULT initHr = CoInitializeEx(nullptr,COINIT::COINIT_MULTITHREADED);
		if(initHr!=RPC_E_CHANGED_MODE)
		{
			if(FAILED(initHr)&&initHr!=S_FALSE)
			{
				DEBUG_MSG(L"Error on COM library initialization!");
				return SHORTCUT_COM_INIT_FAILURE;
			}
			else
			{
				_hasCoInitialized = true;
			}
		}
	}

	bool wasChanged;
	HRESULT hr = validateShellLinkHelper(wasChanged);
	if(SUCCEEDED(hr))
	{
		return wasChanged?SHORTCUT_WAS_CHANGED:SHORTCUT_UNCHANGED;
	}

	hr = createShellLinkHelper();
	return SUCCEEDED(hr)?SHORTCUT_WAS_CREATED:SHORTCUT_CREATE_FAILED;
}

bool WinToast::initialize(_Out_opt_ WinToastError *error)
{
	_isInitialized = false;
	setError(error,WinToastError::NoError);

	if(!isCompatible())
	{
		setError(error,WinToastError::SystemNotSupported);
		DEBUG_MSG(L"Error: system not supported.");
		return false;
	}

	if(_aumi.empty()||_appName.empty())
	{
		setError(error,WinToastError::InvalidParameters);
		DEBUG_MSG(L"Error while initializing, did you set up a valid AUMI and App name?");
		return false;
	}

	if(_shortcutPolicy!=SHORTCUT_POLICY_IGNORE)
	{
		if(createShortcut()<0)
		{
			setError(error,WinToastError::ShellLinkNotCreated);
			DEBUG_MSG(L"Error while attaching the AUMI to the current proccess =(");
			return false;
		}
	}

	if(FAILED(DllImporter::SetCurrentProcessExplicitAppUserModelID(_aumi.c_str())))
	{
		setError(error,WinToastError::InvalidAppUserModelID);
		DEBUG_MSG(L"Error while attaching the AUMI to the current proccess =(");
		return false;
	}

	_isInitialized = true;
	return _isInitialized;
}

bool WinToast::isInitialized() const
{
	return _isInitialized;
}

std::wstring const &WinToast::appName() const
{
	return _appName;
}

std::wstring const &WinToast::appUserModelId() const
{
	return _aumi;
}

HRESULT WinToast::validateShellLinkHelper(_Out_ bool &wasChanged)
{
	WCHAR path[MAX_PATH] = { L'\0' };
	Util::defaultShellLinkPath(_appName,path);
	// Check if the file exist
	DWORD attr = GetFileAttributesW(path);
	if(attr>=0xFFFFFFF)
	{
		DEBUG_MSG("Error, shell link not found. Try to create a new one in: "<<path);
		return E_FAIL;
	}

	// Let's load the file as shell link to validate.
	// - Create a shell link
	// - Create a persistant file
	// - Load the path as data for the persistant file
	// - Read the property AUMI and validate with the current
	// - Review if AUMI is equal.
	ComPtr<IShellLink> shellLink;
	HRESULT hr = CoCreateInstance(CLSID_ShellLink,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&shellLink));
	if(SUCCEEDED(hr))
	{
		ComPtr<IPersistFile> persistFile;
		hr = shellLink.As(&persistFile);
		if(SUCCEEDED(hr))
		{
			hr = persistFile->Load(path,STGM_READWRITE);
			if(SUCCEEDED(hr))
			{
				ComPtr<IPropertyStore> propertyStore;
				hr = shellLink.As(&propertyStore);
				if(SUCCEEDED(hr))
				{
					PROPVARIANT appIdPropVar;
					hr = propertyStore->GetValue(PKEY_AppUserModel_ID,&appIdPropVar);
					if(SUCCEEDED(hr))
					{
						WCHAR AUMI[MAX_PATH];
						hr = DllImporter::PropVariantToString(appIdPropVar,AUMI,MAX_PATH);
						wasChanged = false;
						if(FAILED(hr)||_aumi!=AUMI)
						{
							if(_shortcutPolicy==SHORTCUT_POLICY_REQUIRE_CREATE)
							{
								// AUMI Changed for the same app, let's update the current value! =)
								wasChanged = true;
								PropVariantClear(&appIdPropVar);
								hr = InitPropVariantFromString(_aumi.c_str(),&appIdPropVar);
								if(SUCCEEDED(hr))
								{
									hr = propertyStore->SetValue(PKEY_AppUserModel_ID,appIdPropVar);
									if(SUCCEEDED(hr))
									{
										hr = propertyStore->Commit();
										if(SUCCEEDED(hr)&&SUCCEEDED(persistFile->IsDirty()))
										{
											hr = persistFile->Save(path,TRUE);
										}
									}
								}
							}
							else
							{
								// Not allowed to touch the shortcut to fix the AUMI
								hr = E_FAIL;
							}
						}
						PropVariantClear(&appIdPropVar);
					}
				}
			}
		}
	}
	return hr;
}

HRESULT WinToast::createShellLinkHelper()
{
	if(_shortcutPolicy!=SHORTCUT_POLICY_REQUIRE_CREATE)
	{
		return E_FAIL;
	}

	WCHAR exePath[MAX_PATH]{ L'\0' };
	WCHAR slPath[MAX_PATH]{ L'\0' };
	Util::defaultShellLinkPath(_appName,slPath);
	Util::defaultExecutablePath(exePath);
	ComPtr<IShellLinkW> shellLink;
	HRESULT hr = CoCreateInstance(CLSID_ShellLink,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&shellLink));
	if(SUCCEEDED(hr))
	{
		hr = shellLink->SetPath(exePath);
		if(SUCCEEDED(hr))
		{
			hr = shellLink->SetArguments(L"");
			if(SUCCEEDED(hr))
			{
				hr = shellLink->SetWorkingDirectory(exePath);
				if(SUCCEEDED(hr))
				{
					ComPtr<IPropertyStore> propertyStore;
					hr = shellLink.As(&propertyStore);
					if(SUCCEEDED(hr))
					{
						PROPVARIANT appIdPropVar;
						hr = InitPropVariantFromString(_aumi.c_str(),&appIdPropVar);
						if(SUCCEEDED(hr))
						{
							hr = propertyStore->SetValue(PKEY_AppUserModel_ID,appIdPropVar);
							if(SUCCEEDED(hr))
							{
								hr = propertyStore->Commit();
								if(SUCCEEDED(hr))
								{
									ComPtr<IPersistFile> persistFile;
									hr = shellLink.As(&persistFile);
									if(SUCCEEDED(hr))
									{
										hr = persistFile->Save(slPath,TRUE);
									}
								}
							}
							PropVariantClear(&appIdPropVar);
						}
					}
				}
			}
		}
	}
	return hr;
}

INT64 WinToast::showToast(_In_ WinToastTemplate const &toast,_In_ IWinToastHandler *eventHandler,_Out_ WinToastError *error)
{
	std::shared_ptr<IWinToastHandler> handler(eventHandler);
	setError(error,WinToastError::NoError);
	INT64 id = -1;
	if(!isInitialized())
	{
		setError(error,WinToastError::NotInitialized);
		DEBUG_MSG("Error when launching the toast. WinToast is not initialized.");
		return id;
	}
	if(!handler)
	{
		setError(error,WinToastError::InvalidHandler);
		DEBUG_MSG("Error when launching the toast. Handler cannot be nullptr.");
		return id;
	}

	ComPtr<IToastNotificationManagerStatics> notificationManager;
	HRESULT hr = DllImporter::Wrap_GetActivationFactory(
		WinToastStringWrapper(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(),&notificationManager);
	if(SUCCEEDED(hr))
	{
		ComPtr<IToastNotifier> notifier;
		hr = notificationManager->CreateToastNotifierWithId(WinToastStringWrapper(_aumi).Get(),&notifier);
		if(SUCCEEDED(hr))
		{
			ComPtr<IToastNotificationFactory> notificationFactory;
			hr = DllImporter::Wrap_GetActivationFactory(
				WinToastStringWrapper(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(),&notificationFactory);
			if(SUCCEEDED(hr))
			{
				ComPtr<IXmlDocument> xmlDocument;
				hr = notificationManager->GetTemplateContent(ToastTemplateType(toast.type()),&xmlDocument);
				if(SUCCEEDED(hr)&&toast.isToastGeneric())
				{
					hr = setBindToastGenericHelper(xmlDocument.Get());
				}
				if(SUCCEEDED(hr))
				{
					for(UINT32 i = 0,fieldsCount = static_cast<UINT32>(toast.textFieldsCount()); i<fieldsCount&&SUCCEEDED(hr); i++)
					{
						hr = setTextFieldHelper(xmlDocument.Get(),toast.textField(WinToastTemplate::TextField(i)),i);
					}

					// Modern feature are supported Windows > Windows 10
					if(SUCCEEDED(hr)&&isSupportingModernFeatures())
					{
						// Note that we do this *after* using toast.textFieldsCount() to
						// iterate/fill the template's text fields, since we're adding yet another text field.
						if(SUCCEEDED(hr)&&!toast.attributionText().empty())
						{
							hr = setAttributionTextFieldHelper(xmlDocument.Get(),toast.attributionText());
						}

						std::array<WCHAR,12> buf;
						for(std::size_t i = 0,actionsCount = toast.actionsCount(); i<actionsCount&&SUCCEEDED(hr); i++)
						{
							_snwprintf_s(buf.data(),buf.size(),_TRUNCATE,L"%zd",i);
							hr = addActionHelper(xmlDocument.Get(),toast.actionLabel(i),buf.data());
						}

						if(SUCCEEDED(hr))
						{
							hr = (toast.audioPath().empty()&&toast.audioOption()==WinToastTemplate::AudioOption::Default)
								?hr
								:setAudioFieldHelper(xmlDocument.Get(),toast.audioPath(),toast.audioOption());
						}

						if(SUCCEEDED(hr)&&toast.duration()!=WinToastTemplate::Duration::System)
						{
							hr = addDurationHelper(xmlDocument.Get(),
												   (toast.duration()==WinToastTemplate::Duration::Short)?L"short":L"long");
						}

						if(SUCCEEDED(hr))
						{
							hr = addScenarioHelper(xmlDocument.Get(),toast.scenario());
						}

					}
					else
					{
						DEBUG_MSG("Modern features (Actions/Sounds/Attributes) not supported in this os version");
					}

					if(SUCCEEDED(hr))
					{
						bool isWin10AnniversaryOrAbove = WinToast::isWin10AnniversaryOrHigher();
						bool isCircleCropHint = isWin10AnniversaryOrAbove?toast.isCropHintCircle():false;
						hr = toast.hasImage()
							?setImageFieldHelper(xmlDocument.Get(),toast.imagePath(),toast.isToastGeneric(),isCircleCropHint)
							:hr;
						if(SUCCEEDED(hr)&&isWin10AnniversaryOrAbove&&toast.hasHeroImage())
						{
							hr = setHeroImageHelper(xmlDocument.Get(),toast.heroImagePath(),toast.isInlineHeroImage());
						}
						if(SUCCEEDED(hr))
						{
							ComPtr<IToastNotification> notification;
							hr = notificationFactory->CreateToastNotification(xmlDocument.Get(),&notification);
							if(SUCCEEDED(hr))
							{
								INT64 expiration = 0,relativeExpiration = toast.expiration();
								if(relativeExpiration>0)
								{
									InternalDateTime expirationDateTime(relativeExpiration);
									expiration = expirationDateTime;
									hr = notification->put_ExpirationTime(&expirationDateTime);
								}

								EventRegistrationToken activatedToken,dismissedToken,failedToken;

								GUID guid;
								HRESULT hrGuid = CoCreateGuid(&guid);
								id = guid.Data1;
								if(SUCCEEDED(hr)&&SUCCEEDED(hrGuid))
								{
									hr = Util::setEventHandlers(notification.Get(),handler,expiration,activatedToken,dismissedToken,
																failedToken,[this,id]() { markAsReadyForDeletion(id); });
									if(FAILED(hr))
									{
										setError(error,WinToastError::InvalidHandler);
									}
								}

								if(SUCCEEDED(hr))
								{
									if(SUCCEEDED(hr))
									{
										_buffer.emplace(id,NotifyData(notification,activatedToken,dismissedToken,failedToken));
										DEBUG_MSG("xml: "<<Util::AsString(xmlDocument));
										hr = notifier->Show(notification.Get());
										if(FAILED(hr))
										{
											setError(error,WinToastError::NotDisplayed);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return FAILED(hr)?-1:id;
}

ComPtr<IToastNotifier> WinToast::notifier(_In_ bool *succeded) const
{
	ComPtr<IToastNotificationManagerStatics> notificationManager;
	ComPtr<IToastNotifier> notifier;
	HRESULT hr = DllImporter::Wrap_GetActivationFactory(
		WinToastStringWrapper(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(),&notificationManager);
	if(SUCCEEDED(hr))
	{
		hr = notificationManager->CreateToastNotifierWithId(WinToastStringWrapper(_aumi).Get(),&notifier);
	}
	*succeded = SUCCEEDED(hr);
	return notifier;
}

void WinToast::markAsReadyForDeletion(_In_ INT64 id)
{
	// Flush the buffer by removing all the toasts that are ready for deletion
	for(auto it = _buffer.begin(); it!=_buffer.end();)
	{
		if(it->second.isReadyForDeletion())
		{
			it->second.RemoveTokens();
			it = _buffer.erase(it);
		}
		else
		{
			++it;
		}
	}

	// Mark the toast as ready for deletion (if it exists) so that it will be removed from the buffer in the next iteration
	auto const iter = _buffer.find(id);
	if(iter!=_buffer.end())
	{
		_buffer[id].markAsReadyForDeletion();
	}
}

bool WinToast::hideToast(_In_ INT64 id)
{
	if(!isInitialized())
	{
		DEBUG_MSG("Error when hiding the toast. WinToast is not initialized.");
		return false;
	}

	auto iter = _buffer.find(id);
	if(iter==_buffer.end())
	{
		return false;
	}

	auto succeded = false;
	auto notify = notifier(&succeded);
	if(!succeded)
	{
		return false;
	}

	auto &notifyData = iter->second;
	auto result = notify->Hide(notifyData.notification());
	if(FAILED(result))
	{
		DEBUG_MSG("Error when hiding the toast. Error code: "<<result);
		return false;
	}

	notifyData.RemoveTokens();
	_buffer.erase(iter);
	return SUCCEEDED(result);
}

void WinToast::clear()
{
	auto succeded = false;
	auto notify = notifier(&succeded);
	if(!succeded)
	{
		return;
	}

	for(auto &data:_buffer)
	{
		auto &notifyData = data.second;
		notify->Hide(notifyData.notification());
		notifyData.RemoveTokens();
	}
	_buffer.clear();
}

//
// Available as of Windows 10 Anniversary Update
// Ref: https://docs.microsoft.com/en-us/windows/uwp/design/shell/tiles-and-notifications/adaptive-interactive-toasts
//
// NOTE: This will add a new text field, so be aware when iterating over
//       the toast's text fields or getting a count of them.
//
HRESULT WinToast::setAttributionTextFieldHelper(_In_ IXmlDocument *xml,_In_ std::wstring const &text)
{
	Util::createElement(xml,L"binding",L"text",{ L"placement" });
	ComPtr<IXmlNodeList> nodeList;
	HRESULT hr = xml->GetElementsByTagName(WinToastStringWrapper(L"text").Get(),&nodeList);
	if(SUCCEEDED(hr))
	{
		UINT32 nodeListLength;
		hr = nodeList->get_Length(&nodeListLength);
		if(SUCCEEDED(hr))
		{
			for(UINT32 i = 0; i<nodeListLength; i++)
			{
				ComPtr<IXmlNode> textNode;
				hr = nodeList->Item(i,&textNode);
				if(SUCCEEDED(hr))
				{
					ComPtr<IXmlNamedNodeMap> attributes;
					hr = textNode->get_Attributes(&attributes);
					if(SUCCEEDED(hr))
					{
						ComPtr<IXmlNode> editedNode;
						if(SUCCEEDED(hr))
						{
							hr = attributes->GetNamedItem(WinToastStringWrapper(L"placement").Get(),&editedNode);
							if(FAILED(hr)||!editedNode)
							{
								continue;
							}
							hr = Util::setNodeStringValue(L"attribution",editedNode.Get(),xml);
							if(SUCCEEDED(hr))
							{
								return setTextFieldHelper(xml,text,i);
							}
						}
					}
				}
			}
		}
	}
	return hr;
}

HRESULT WinToast::addDurationHelper(_In_ IXmlDocument *xml,_In_ std::wstring const &duration)
{
	ComPtr<IXmlNodeList> nodeList;
	HRESULT hr = xml->GetElementsByTagName(WinToastStringWrapper(L"toast").Get(),&nodeList);
	if(SUCCEEDED(hr))
	{
		UINT32 length;
		hr = nodeList->get_Length(&length);
		if(SUCCEEDED(hr))
		{
			ComPtr<IXmlNode> toastNode;
			hr = nodeList->Item(0,&toastNode);
			if(SUCCEEDED(hr))
			{
				ComPtr<IXmlElement> toastElement;
				hr = toastNode.As(&toastElement);
				if(SUCCEEDED(hr))
				{
					hr = toastElement->SetAttribute(WinToastStringWrapper(L"duration").Get(),WinToastStringWrapper(duration).Get());
				}
			}
		}
	}
	return hr;
}

HRESULT WinToast::addScenarioHelper(_In_ IXmlDocument *xml,_In_ std::wstring const &scenario)
{
	ComPtr<IXmlNodeList> nodeList;
	HRESULT hr = xml->GetElementsByTagName(WinToastStringWrapper(L"toast").Get(),&nodeList);
	if(SUCCEEDED(hr))
	{
		UINT32 length;
		hr = nodeList->get_Length(&length);
		if(SUCCEEDED(hr))
		{
			ComPtr<IXmlNode> toastNode;
			hr = nodeList->Item(0,&toastNode);
			if(SUCCEEDED(hr))
			{
				ComPtr<IXmlElement> toastElement;
				hr = toastNode.As(&toastElement);
				if(SUCCEEDED(hr))
				{
					hr = toastElement->SetAttribute(WinToastStringWrapper(L"scenario").Get(),WinToastStringWrapper(scenario).Get());
				}
			}
		}
	}
	return hr;
}

HRESULT WinToast::setTextFieldHelper(_In_ IXmlDocument *xml,_In_ std::wstring const &text,_In_ UINT32 pos)
{
	ComPtr<IXmlNodeList> nodeList;
	HRESULT hr = xml->GetElementsByTagName(WinToastStringWrapper(L"text").Get(),&nodeList);
	if(SUCCEEDED(hr))
	{
		ComPtr<IXmlNode> node;
		hr = nodeList->Item(pos,&node);
		if(SUCCEEDED(hr))
		{
			hr = Util::setNodeStringValue(text,node.Get(),xml);
		}
	}
	return hr;
}

HRESULT WinToast::setBindToastGenericHelper(_In_ IXmlDocument *xml)
{
	ComPtr<IXmlNodeList> nodeList;
	HRESULT hr = xml->GetElementsByTagName(WinToastStringWrapper(L"binding").Get(),&nodeList);
	if(SUCCEEDED(hr))
	{
		UINT32 length;
		hr = nodeList->get_Length(&length);
		if(SUCCEEDED(hr))
		{
			ComPtr<IXmlNode> toastNode;
			hr = nodeList->Item(0,&toastNode);
			if(SUCCEEDED(hr))
			{
				ComPtr<IXmlElement> toastElement;
				hr = toastNode.As(&toastElement);
				if(SUCCEEDED(hr))
				{
					hr = toastElement->SetAttribute(WinToastStringWrapper(L"template").Get(),WinToastStringWrapper(L"ToastGeneric").Get());
				}
			}
		}
	}
	return hr;
}

HRESULT WinToast::setImageFieldHelper(_In_ IXmlDocument *xml,_In_ std::wstring const &path,_In_ bool isToastGeneric,
									  _In_ bool isCropHintCircle)
{
	assert(path.size()<MAX_PATH);

	wchar_t imagePath[MAX_PATH] = L"file:///";
	HRESULT hr = StringCchCatW(imagePath,MAX_PATH,path.c_str());
	if(SUCCEEDED(hr))
	{
		ComPtr<IXmlNodeList> nodeList;
		HRESULT hr = xml->GetElementsByTagName(WinToastStringWrapper(L"image").Get(),&nodeList);
		if(SUCCEEDED(hr))
		{
			ComPtr<IXmlNode> node;
			hr = nodeList->Item(0,&node);

			ComPtr<IXmlElement> imageElement;
			HRESULT hrImage = node.As(&imageElement);
			if(SUCCEEDED(hr)&&SUCCEEDED(hrImage)&&isToastGeneric)
			{
				hr = imageElement->SetAttribute(WinToastStringWrapper(L"placement").Get(),WinToastStringWrapper(L"appLogoOverride").Get());
				if(SUCCEEDED(hr)&&isCropHintCircle)
				{
					hr = imageElement->SetAttribute(WinToastStringWrapper(L"hint-crop").Get(),WinToastStringWrapper(L"circle").Get());
				}
			}
			if(SUCCEEDED(hr))
			{
				ComPtr<IXmlNamedNodeMap> attributes;
				hr = node->get_Attributes(&attributes);
				if(SUCCEEDED(hr))
				{
					ComPtr<IXmlNode> editedNode;
					hr = attributes->GetNamedItem(WinToastStringWrapper(L"src").Get(),&editedNode);
					if(SUCCEEDED(hr))
					{
						Util::setNodeStringValue(imagePath,editedNode.Get(),xml);
					}
				}
			}
		}
	}
	return hr;
}

HRESULT WinToast::setAudioFieldHelper(_In_ IXmlDocument *xml,_In_ std::wstring const &path,
									  _In_opt_ WinToastTemplate::AudioOption option)
{
	std::vector<std::wstring> attrs;
	if(!path.empty())
	{
		attrs.push_back(L"src");
	}
	if(option==WinToastTemplate::AudioOption::Loop)
	{
		attrs.push_back(L"loop");
	}
	if(option==WinToastTemplate::AudioOption::Silent)
	{
		attrs.push_back(L"silent");
	}
	Util::createElement(xml,L"toast",L"audio",attrs);

	ComPtr<IXmlNodeList> nodeList;
	HRESULT hr = xml->GetElementsByTagName(WinToastStringWrapper(L"audio").Get(),&nodeList);
	if(SUCCEEDED(hr))
	{
		ComPtr<IXmlNode> node;
		hr = nodeList->Item(0,&node);
		if(SUCCEEDED(hr))
		{
			ComPtr<IXmlNamedNodeMap> attributes;
			hr = node->get_Attributes(&attributes);
			if(SUCCEEDED(hr))
			{
				ComPtr<IXmlNode> editedNode;
				if(!path.empty())
				{
					if(SUCCEEDED(hr))
					{
						hr = attributes->GetNamedItem(WinToastStringWrapper(L"src").Get(),&editedNode);
						if(SUCCEEDED(hr))
						{
							hr = Util::setNodeStringValue(path,editedNode.Get(),xml);
						}
					}
				}

				if(SUCCEEDED(hr))
				{
					switch(option)
					{
					case WinToastTemplate::AudioOption::Loop:
						hr = attributes->GetNamedItem(WinToastStringWrapper(L"loop").Get(),&editedNode);
						if(SUCCEEDED(hr))
						{
							hr = Util::setNodeStringValue(L"true",editedNode.Get(),xml);
						}
						break;
					case WinToastTemplate::AudioOption::Silent:
						hr = attributes->GetNamedItem(WinToastStringWrapper(L"silent").Get(),&editedNode);
						if(SUCCEEDED(hr))
						{
							hr = Util::setNodeStringValue(L"true",editedNode.Get(),xml);
						}
					default:
						break;
					}
				}
			}
		}
	}
	return hr;
}

HRESULT WinToast::addActionHelper(_In_ IXmlDocument *xml,_In_ std::wstring const &content,_In_ std::wstring const &arguments)
{
	ComPtr<IXmlNodeList> nodeList;
	HRESULT hr = xml->GetElementsByTagName(WinToastStringWrapper(L"actions").Get(),&nodeList);
	if(SUCCEEDED(hr))
	{
		UINT32 length;
		hr = nodeList->get_Length(&length);
		if(SUCCEEDED(hr))
		{
			ComPtr<IXmlNode> actionsNode;
			if(length>0)
			{
				hr = nodeList->Item(0,&actionsNode);
			}
			else
			{
				hr = xml->GetElementsByTagName(WinToastStringWrapper(L"toast").Get(),&nodeList);
				if(SUCCEEDED(hr))
				{
					hr = nodeList->get_Length(&length);
					if(SUCCEEDED(hr))
					{
						ComPtr<IXmlNode> toastNode;
						hr = nodeList->Item(0,&toastNode);
						if(SUCCEEDED(hr))
						{
							ComPtr<IXmlElement> toastElement;
							hr = toastNode.As(&toastElement);
							if(SUCCEEDED(hr))
							{
								hr = toastElement->SetAttribute(WinToastStringWrapper(L"template").Get(),
																WinToastStringWrapper(L"ToastGeneric").Get());
							}
							if(SUCCEEDED(hr))
							{
								hr = toastElement->SetAttribute(WinToastStringWrapper(L"duration").Get(),
																WinToastStringWrapper(L"long").Get());
							}
							if(SUCCEEDED(hr))
							{
								ComPtr<IXmlElement> actionsElement;
								hr = xml->CreateElement(WinToastStringWrapper(L"actions").Get(),&actionsElement);
								if(SUCCEEDED(hr))
								{
									hr = actionsElement.As(&actionsNode);
									if(SUCCEEDED(hr))
									{
										ComPtr<IXmlNode> appendedChild;
										hr = toastNode->AppendChild(actionsNode.Get(),&appendedChild);
									}
								}
							}
						}
					}
				}
			}
			if(SUCCEEDED(hr))
			{
				ComPtr<IXmlElement> actionElement;
				hr = xml->CreateElement(WinToastStringWrapper(L"action").Get(),&actionElement);
				if(SUCCEEDED(hr))
				{
					hr = actionElement->SetAttribute(WinToastStringWrapper(L"content").Get(),WinToastStringWrapper(content).Get());
				}
				if(SUCCEEDED(hr))
				{
					hr = actionElement->SetAttribute(WinToastStringWrapper(L"arguments").Get(),WinToastStringWrapper(arguments).Get());
				}
				if(SUCCEEDED(hr))
				{
					ComPtr<IXmlNode> actionNode;
					hr = actionElement.As(&actionNode);
					if(SUCCEEDED(hr))
					{
						ComPtr<IXmlNode> appendedChild;
						hr = actionsNode->AppendChild(actionNode.Get(),&appendedChild);
					}
				}
			}
		}
	}
	return hr;
}

HRESULT WinToast::setHeroImageHelper(_In_ IXmlDocument *xml,_In_ std::wstring const &path,_In_ bool isInlineImage)
{
	ComPtr<IXmlNodeList> nodeList;
	HRESULT hr = xml->GetElementsByTagName(WinToastStringWrapper(L"binding").Get(),&nodeList);
	if(SUCCEEDED(hr))
	{
		UINT32 length;
		hr = nodeList->get_Length(&length);
		if(SUCCEEDED(hr))
		{
			ComPtr<IXmlNode> bindingNode;
			if(length>0)
			{
				hr = nodeList->Item(0,&bindingNode);
			}
			if(SUCCEEDED(hr))
			{
				ComPtr<IXmlElement> imageElement;
				hr = xml->CreateElement(WinToastStringWrapper(L"image").Get(),&imageElement);
				if(SUCCEEDED(hr)&&isInlineImage==false)
				{
					hr = imageElement->SetAttribute(WinToastStringWrapper(L"placement").Get(),WinToastStringWrapper(L"hero").Get());
				}
				if(SUCCEEDED(hr))
				{
					hr = imageElement->SetAttribute(WinToastStringWrapper(L"src").Get(),WinToastStringWrapper(path).Get());
				}
				if(SUCCEEDED(hr))
				{
					ComPtr<IXmlNode> actionNode;
					hr = imageElement.As(&actionNode);
					if(SUCCEEDED(hr))
					{
						ComPtr<IXmlNode> appendedChild;
						hr = bindingNode->AppendChild(actionNode.Get(),&appendedChild);
					}
				}
			}
		}
	}
	return hr;
}

void WinToast::setError(_Out_opt_ WinToastError *error,_In_ WinToastError value)
{
	if(error)
	{
		*error = value;
	}
}

WinToastTemplate::WinToastTemplate(_In_ WinToastTemplateType type) : _type(type)
{
	constexpr static std::size_t TextFieldsCount[] = { 1,2,2,3,1,2,2,3 };
	_textFields = std::vector<std::wstring>(TextFieldsCount[type],L"");
}

WinToastTemplate::~WinToastTemplate()
{
	_textFields.clear();
}

void WinToastTemplate::setTextField(_In_ std::wstring const &txt,_In_ WinToastTemplate::TextField pos)
{
	auto const position = static_cast<std::size_t>(pos);
	if(position>=_textFields.size())
	{
		DEBUG_MSG("The selected template type supports only "<<_textFields.size()<<" text lines");
		return;
	}
	_textFields[position] = txt;
}

void WinToastTemplate::setImagePath(_In_ std::wstring const &imgPath,_In_ CropHint cropHint)
{
	_imagePath = imgPath;
	_cropHint = cropHint;
}

void WinToastTemplate::setHeroImagePath(_In_ std::wstring const &imgPath,_In_ bool inlineImage)
{
	_heroImagePath = imgPath;
	_inlineHeroImage = inlineImage;
}

void WinToastTemplate::setAudioPath(_In_ std::wstring const &audioPath)
{
	_audioPath = audioPath;
}

void WinToastTemplate::setAudioPath(_In_ AudioSystemFile file)
{
	static const std::unordered_map<AudioSystemFile,std::wstring> Files = {
		{ AudioSystemFile::DefaultSound,L"ms-winsoundevent:Notification.Default" },
		{ AudioSystemFile::IM,L"ms-winsoundevent:Notification.IM" },
		{ AudioSystemFile::Mail,L"ms-winsoundevent:Notification.Mail" },
		{ AudioSystemFile::Reminder,L"ms-winsoundevent:Notification.Reminder" },
		{ AudioSystemFile::SMS,L"ms-winsoundevent:Notification.SMS" },
		{ AudioSystemFile::Alarm,L"ms-winsoundevent:Notification.Looping.Alarm" },
		{ AudioSystemFile::Alarm2,L"ms-winsoundevent:Notification.Looping.Alarm2" },
		{ AudioSystemFile::Alarm3,L"ms-winsoundevent:Notification.Looping.Alarm3" },
		{ AudioSystemFile::Alarm4,L"ms-winsoundevent:Notification.Looping.Alarm4" },
		{ AudioSystemFile::Alarm5,L"ms-winsoundevent:Notification.Looping.Alarm5" },
		{ AudioSystemFile::Alarm6,L"ms-winsoundevent:Notification.Looping.Alarm6" },
		{ AudioSystemFile::Alarm7,L"ms-winsoundevent:Notification.Looping.Alarm7" },
		{ AudioSystemFile::Alarm8,L"ms-winsoundevent:Notification.Looping.Alarm8" },
		{ AudioSystemFile::Alarm9,L"ms-winsoundevent:Notification.Looping.Alarm9" },
		{ AudioSystemFile::Alarm10,L"ms-winsoundevent:Notification.Looping.Alarm10" },
		{ AudioSystemFile::Call,L"ms-winsoundevent:Notification.Looping.Call" },
		{ AudioSystemFile::Call1,L"ms-winsoundevent:Notification.Looping.Call1" },
		{ AudioSystemFile::Call2,L"ms-winsoundevent:Notification.Looping.Call2" },
		{ AudioSystemFile::Call3,L"ms-winsoundevent:Notification.Looping.Call3" },
		{ AudioSystemFile::Call4,L"ms-winsoundevent:Notification.Looping.Call4" },
		{ AudioSystemFile::Call5,L"ms-winsoundevent:Notification.Looping.Call5" },
		{ AudioSystemFile::Call6,L"ms-winsoundevent:Notification.Looping.Call6" },
		{ AudioSystemFile::Call7,L"ms-winsoundevent:Notification.Looping.Call7" },
		{ AudioSystemFile::Call8,L"ms-winsoundevent:Notification.Looping.Call8" },
		{ AudioSystemFile::Call9,L"ms-winsoundevent:Notification.Looping.Call9" },
		{ AudioSystemFile::Call10,L"ms-winsoundevent:Notification.Looping.Call10" },
	};
	auto const iter = Files.find(file);
	assert(iter!=Files.end());
	_audioPath = iter->second;
}

void WinToastTemplate::setAudioOption(_In_ WinToastTemplate::AudioOption audioOption)
{
	_audioOption = audioOption;
}

void WinToastTemplate::setFirstLine(_In_ std::wstring const &text)
{
	setTextField(text,WinToastTemplate::FirstLine);
}

void WinToastTemplate::setSecondLine(_In_ std::wstring const &text)
{
	setTextField(text,WinToastTemplate::SecondLine);
}

void WinToastTemplate::setThirdLine(_In_ std::wstring const &text)
{
	setTextField(text,WinToastTemplate::ThirdLine);
}

void WinToastTemplate::setDuration(_In_ Duration duration)
{
	_duration = duration;
}

void WinToastTemplate::setExpiration(_In_ INT64 millisecondsFromNow)
{
	_expiration = millisecondsFromNow;
}

void WinToastLib::WinToastTemplate::setScenario(_In_ Scenario scenario)
{
	switch(scenario)
	{
	case Scenario::Default:
		_scenario = L"Default";
		break;
	case Scenario::Alarm:
		_scenario = L"Alarm";
		break;
	case Scenario::IncomingCall:
		_scenario = L"IncomingCall";
		break;
	case Scenario::Reminder:
		_scenario = L"Reminder";
		break;
	}
}

void WinToastTemplate::setAttributionText(_In_ std::wstring const &attributionText)
{
	_attributionText = attributionText;
}

void WinToastTemplate::addAction(_In_ std::wstring const &label)
{
	_actions.push_back(label);
}

std::size_t WinToastTemplate::textFieldsCount() const
{
	return _textFields.size();
}

std::size_t WinToastTemplate::actionsCount() const
{
	return _actions.size();
}

bool WinToastTemplate::hasImage() const
{
	return _type<WinToastTemplateType::Text01;
}

bool WinToastTemplate::hasHeroImage() const
{
	return hasImage()&&!_heroImagePath.empty();
}

std::vector<std::wstring> const &WinToastTemplate::textFields() const
{
	return _textFields;
}

std::wstring const &WinToastTemplate::textField(_In_ TextField pos) const
{
	auto const position = static_cast<std::size_t>(pos);
	assert(position<_textFields.size());
	return _textFields[position];
}

std::wstring const &WinToastTemplate::actionLabel(_In_ std::size_t position) const
{
	assert(position<_actions.size());
	return _actions[position];
}

std::wstring const &WinToastTemplate::imagePath() const
{
	return _imagePath;
}

std::wstring const &WinToastTemplate::heroImagePath() const
{
	return _heroImagePath;
}

std::wstring const &WinToastTemplate::audioPath() const
{
	return _audioPath;
}

std::wstring const &WinToastTemplate::attributionText() const
{
	return _attributionText;
}

std::wstring const &WinToastLib::WinToastTemplate::scenario() const
{
	return _scenario;
}

INT64 WinToastTemplate::expiration() const
{
	return _expiration;
}

WinToastTemplate::WinToastTemplateType WinToastTemplate::type() const
{
	return _type;
}

WinToastTemplate::AudioOption WinToastTemplate::audioOption() const
{
	return _audioOption;
}

WinToastTemplate::Duration WinToastTemplate::duration() const
{
	return _duration;
}

bool WinToastTemplate::isToastGeneric() const
{
	return hasHeroImage()||_cropHint==WinToastTemplate::Circle;
}

bool WinToastTemplate::isInlineHeroImage() const
{
	return _inlineHeroImage;
}

bool WinToastTemplate::isCropHintCircle() const
{
	return _cropHint==CropHint::Circle;
}

//#include "wintoastlib.h"
#include <string>
#include <windows.h>

#define _NO_exit_ //exit

using namespace WinToastLib;

class CustomHandler : public IWinToastHandler
{
public:
	void toastActivated() const
	{
		std::wcout<<L"The user clicked in this toast"<<std::endl;
		_NO_exit_(0);
	}

	void toastActivated(int actionIndex) const
	{
		std::wcout<<L"The user clicked on action #"<<actionIndex<<std::endl;
		_NO_exit_(16+actionIndex);
	}

	void toastDismissed(WinToastDismissalReason state) const
	{
		switch(state)
		{
		case UserCanceled:
			std::wcout<<L"The user dismissed this toast"<<std::endl;
			_NO_exit_(1);
			break;
		case TimedOut:
			std::wcout<<L"The toast has timed out"<<std::endl;
			_NO_exit_(2);
			break;
		case ApplicationHidden:
			std::wcout<<L"The application hid the toast using ToastNotifier.hide()"<<std::endl;
			_NO_exit_(3);
			break;
		default:
			std::wcout<<L"Toast not activated"<<std::endl;
			_NO_exit_(4);
			break;
		}
	}

	void toastFailed() const
	{
		std::wcout<<L"Error showing current toast"<<std::endl;
		_NO_exit_(5);
	}
};

enum Results
{
	ToastClicked,             // user clicked on the toast
	ToastDismissed,           // user dismissed the toast
	ToastTimeOut,             // toast timed out
	ToastHided,               // application hid the toast
	ToastNotActivated,        // toast was not activated
	ToastFailed,              // toast failed
	SystemNotSupported,       // system does not support toasts
	UnhandledOption,          // unhandled option
	MultipleTextNotSupported, // multiple texts were provided
	InitializationFailure,    // toast notification manager initialization failure
	ToastNotLaunched          // toast could not be launched
};

#define COMMAND_ACTION     L"--action"
#define COMMAND_AUMI       L"--aumi"
#define COMMAND_APPNAME    L"--appname"
#define COMMAND_APPID      L"--appid"
#define COMMAND_EXPIREMS   L"--expirems"
#define COMMAND_TEXT       L"--text"
#define COMMAND_HELP       L"--help"
#define COMMAND_IMAGE      L"--image"
#define COMMAND_SHORTCUT   L"--only-create-shortcut"
#define COMMAND_AUDIOSTATE L"--audio-state"
#define COMMAND_ALARMAUDIO L"--alarm-audio"
#define COMMAND_ATTRIBUTE  L"--attribute"

void print_help()
{
	std::wcout<<"WinToast Console Example [OPTIONS]"<<std::endl;
	std::wcout<<"\t"<<COMMAND_ACTION<<L" : Set the actions in buttons"<<std::endl;
//	std::wcout<<"\t"<<COMMAND_AUMI<<L" : Set the App User Model Id"<<std::endl; //same as COMMAND_APPID
	std::wcout<<"\t"<<COMMAND_APPNAME<<L" : Set the default appname"<<std::endl;
	std::wcout<<"\t"<<COMMAND_APPID<<L" : Set the App Id"<<std::endl;
	std::wcout<<"\t"<<COMMAND_EXPIREMS<<L" : Set the default expiration time"<<std::endl;
	std::wcout<<"\t"<<COMMAND_TEXT<<L" : Set the text for the notifications"<<std::endl;
	std::wcout<<"\t"<<COMMAND_IMAGE<<L" : set the image path"<<std::endl;
	std::wcout<<"\t"<<COMMAND_ATTRIBUTE<<L" : Set the attribute for the notification"<<std::endl;
	std::wcout<<"\t"<<COMMAND_SHORTCUT<<L" : Create the shortcut for the app"<<std::endl;
	std::wcout<<"\t"<<COMMAND_AUDIOSTATE<<L" : Set the audio state: Default = 0, Silent = 1, Loop = 2"<<std::endl;
	std::wcout<<"\t"<<COMMAND_ALARMAUDIO<<L" : Set alarm audio: No-alarm = 0, Alarm = 1"<<std::endl;
	std::wcout<<"\t"<<COMMAND_HELP<<L" : Print the help description"<<std::endl;
}

extern int Ex_toast_wmain(int argc, LPWSTR *argv)
{
	if(argc==1)
	{
		print_help();
		return 0;
	}

	if(!WinToast::isCompatible())
	{
		std::wcerr<<L"Error, your system in not supported!"<<std::endl;
		return Results::SystemNotSupported;
	}

	std::wstring appName = L"Console WinToast Example";
	std::wstring appUserModelID = L"WinToast Console Example";
	std::wstring text = L"";
	std::wstring imagePath = L"";
	std::wstring attribute = L"default";
	std::vector<std::wstring> actions;
	INT64 expiration = 0;

	bool onlyCreateShortcut = false;
	WinToastTemplate::AudioOption audioOption = WinToastTemplate::AudioOption::Default;

	int alarm = 0;

	int i;
	for(i = 1; i<argc; i++)
	{
		if(!wcscmp(COMMAND_IMAGE,argv[i]))
		{
			imagePath = argv[++i];
		}
		else if(!wcscmp(COMMAND_ACTION,argv[i]))
		{
			actions.push_back(argv[++i]);
		}
		else if(!wcscmp(COMMAND_EXPIREMS,argv[i]))
		{
			expiration = wcstol(argv[++i],NULL,10);
		}
		else if(!wcscmp(COMMAND_APPNAME,argv[i]))
		{
			appName = argv[++i];
		}
//		else if(!wcscmp(COMMAND_AUMI,argv[i])||!wcscmp(COMMAND_APPID,argv[i]))
		else if(!wcscmp(COMMAND_APPID,argv[i]))
		{
			appUserModelID = argv[++i];
		}
		else if(!wcscmp(COMMAND_TEXT,argv[i]))
		{
			text = argv[++i];
		}
		else if(!wcscmp(COMMAND_ATTRIBUTE,argv[i]))
		{
			attribute = argv[++i];
		}
		else if(!wcscmp(COMMAND_SHORTCUT,argv[i]))
		{
			onlyCreateShortcut = true;
		}
		else if(!wcscmp(COMMAND_AUDIOSTATE,argv[i]))
		{
			audioOption = static_cast<WinToastTemplate::AudioOption>(std::stoi(argv[++i]));
		}		
		else if(!wcscmp(COMMAND_ALARMAUDIO,argv[i])) //2024
		{
			alarm = std::stoi(argv[++i]);
		}
		else if(!wcscmp(COMMAND_HELP,argv[i]))
		{
			print_help();
			return 0;
		}
		else
		{
			std::wcerr<<L"Option not recognized: "<<argv[i]<<std::endl;
			return Results::UnhandledOption;
		}
	}

	WinToast::instance()->setAppName(appName);
	WinToast::instance()->setAppUserModelId(appUserModelID);

	if(onlyCreateShortcut)
	{
		if(!imagePath.empty()||!text.empty()||actions.size()>0||expiration)
		{
			std::wcerr<<L"--only-create-shortcut does not accept images/text/actions/expiration"<<std::endl;
			return 9;
		}
		enum WinToast::ShortcutResult result = WinToast::instance()->createShortcut();
		return result?16+result:0;
	}

	if(text.empty())
	{
		text = L"Hello, world!";
	}

	if(!WinToast::instance()->initialize())
	{
		std::wcerr<<L"Error, your system in not compatible!"<<std::endl;
		return Results::InitializationFailure;
	}

	WinToastTemplate templ(!imagePath.empty()?WinToastTemplate::ImageAndText02:WinToastTemplate::Text02);
	templ.setTextField(text,WinToastTemplate::FirstLine);
	templ.setAudioOption(audioOption);
	templ.setAttributionText(attribute);
	templ.setImagePath(imagePath);

	for(auto const &action:actions)
	{
		templ.addAction(action);
	}

	if(expiration)
	{
		templ.setExpiration(expiration);
	}

	//TODO: choose audio type
	//NOTE: this is to override Focus Assist (doesn't seem to work)
	if(alarm) 
	{
		templ.setScenario(WinToastTemplate::Scenario::Alarm);
	//	templ.setAudioPath(WinToastTemplate::AudioSystemFile::Alarm); //1 off alarm clock
	}

	if(WinToast::instance()->showToast(templ,new CustomHandler())<0)
	{
		std::wcerr<<L"Could not launch your toast notification!";
		return Results::ToastFailed;
	}

	// Give the handler a chance for 15 seconds (or the expiration plus 1 second)
//	Sleep(expiration?(DWORD)expiration+1000:15000);

	_NO_exit_(2); return 0;
}