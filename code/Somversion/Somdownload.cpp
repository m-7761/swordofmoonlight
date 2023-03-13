
#include "Somversion.pch.h"

typedef struct 
{
	bool connected;
	HINTERNET internet; 
	HINTERNET connection;
	DWORD connection_timeout;	

	HANDLE request_complete;
	HANDLE thread; DWORD id;
	char resume[MAX_PATH*4];
	char resume_s[INTERNET_RFC1123_BUFSIZE];		
	wchar_t incomplete[MAX_PATH];		

	Somdownload_progress progress; void *dialog;
	enum{ WinInet,WinSock }method; SOCKET socket; //NEW

}Somdownload_instance;

#define SOMDOWNLOAD_AGENT "Sword of Moonlight Subversion Client"

//NEW: timeout/resume framework
static const DWORD Somdownload_flags = 
INTERNET_FLAG_RELOAD|INTERNET_FLAG_PRAGMA_NOCACHE|INTERNET_FLAG_NO_CACHE_WRITE;
static void Somdownload_connect(Somdownload_instance *in)
{	
	int resume = 0; if(in->connection) //NEW
	{
		FILE *f = _wfopen(in->incomplete,L"rb");
		fseek(f,0,SEEK_END); resume = ftell(f); fclose(f);		
	}
	else
	{
		SYSTEMTIME st; GetSystemTime(&st);
		InternetTimeFromSystemTime(&st,INTERNET_RFC1123_FORMAT,in->resume_s,INTERNET_RFC1123_BUFSIZE);
	}
	char headers[1024] = "";
	int headers_s = sprintf_s(headers,
	"If-Unmodified-Since: %s\r\nRange: bytes=%d-\r\n",in->resume_s,resume);	
	if(in->method==in->WinInet)
	{
		in->connected = 
		InternetOpenUrlA(in->internet,in->resume,headers,headers_s,Somdownload_flags,(DWORD_PTR)in);			
		if(!in->connected&&ERROR_IO_PENDING==GetLastError())
		while(!in->connected) Sleep(100);		
	}
	else if(in->method==in->WinSock)
	{
		char *file,*host,head[4096];		
		host = in->resume+sizeof("http://")-1;
		assert(in->resume==strstr(in->resume,"http://"));
		file = strchr(host,'/'); assert(file);		
		if(file) *file = '\0';
		WSADATA data;
		int err = WSAStartup(0x202,&data);
		hostent *he = gethostbyname(host);	
		sockaddr_in	sa = {AF_INET,htons(80),{0},{0,0,0,0,0,0,0,0}};
		if(he) sa.sin_addr = *(IN_ADDR*)he->h_addr;
		sprintf_s(head,"GET /%s HTTP/1.1\r\nHost: %s\r\n%s" //headers
		"User-Agent: "SOMDOWNLOAD_AGENT"\r\n"/**/"\r\n",file?file+1:"",host,headers);
		if(file) *file = '/';	
		in->connected = !err;
		in->socket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP); 
		assert(in->socket>=0);   
		if(connect(in->socket,(SOCKADDR*)&sa,sizeof(sa))>=0)
		{
			int paranoia = strlen(head);
			int sent = send(in->socket,head,paranoia,0);
			assert(sent==paranoia);
		}
		else assert(0);
	}
	assert(in->connected);
}
static void Somdownload_disconnect(Somdownload_instance *in)
{
	if(in->method==in->WinInet)
	InternetCloseHandle(in->connection);	
	if(in->method==in->WinSock&&in->connected)
	{	
		closesocket(in->socket); WSACleanup(); in->connected = false;
	}
}		  
static bool Somdownload_resume(Somdownload_instance *in)
{
	Somdownload_disconnect(in);
	while(in->connected) Sleep(100); 
	Somdownload_connect(in); return in->connected; 
}

//200 was used when I had a slower Internet connection
//(30 may lock for some users. We probably just need a bette API)
//http://social.msdn.microsoft.com/Forums/en-US/25da2c43-5f8c-4627-a8f9-caed0810b7ff/wininet-hangs-in-internetreadfileinternetreadfileex?forum=vssmartdevicesnative
extern DWORD Somdownload_sleep = 30; //200;

static DWORD WINAPI Somdownload_threadmain(Somdownload_instance *in);
static void CALLBACK Somdownload_handler(HINTERNET,DWORD_PTR,DWORD,LPVOID,DWORD);
extern bool Somdownload(wchar_t inout[MAX_PATH], Somdownload_progress progress, void *dialog)
{
	if(!progress) return false;

	int i, j; char urlencode[MAX_PATH*4] = ""; //InternetOpenUrlW
	for(i=0,j=0;i<MAX_PATH&&inout[i]&&j<sizeof(urlencode)-40;i++) if(inout[i]>=128)
	{
		unsigned char *utf8 = (unsigned char*)urlencode+j+30;
		int len = WideCharToMultiByte(CP_UTF8,0,inout+i,1,(char*)utf8,10,0,0);
		for(int k=0;k<len;itoa(utf8[k],urlencode+j+1,16),k++,j+=3) urlencode[j] = '%'; 
	}
	else urlencode[j++] = inout[i]; urlencode[j] = '\0';	

	//TODO: ask for permission
	if(InternetAttemptConnect(0)!=ERROR_SUCCESS
    ||!InternetCheckConnectionW(inout,FLAG_ICC_FORCE_CONNECTION,0))
	{		
		assert(0); return false; //unimplemented
	}
	else //see if file is there...
	{
		HINTERNET test2 = 0;
		HINTERNET test1 = InternetOpenA(SOMDOWNLOAD_AGENT,0,0,0,0);
		if(test1)
		{
			test2 =	InternetOpenUrlA(test1,urlencode,0,0,Somdownload_flags,0);				
			InternetCloseHandle(test2);
			InternetCloseHandle(test1);
		}
		if(!test2) return false;
	}

	wchar_t out[MAX_PATH+1] = L"\\connection";	
	size_t cat = wcslen(Somfolder(out)); out[cat++] = '\\';
	wcscpy_s(out+cat,MAX_PATH-cat,inout); Somlegalize(out+cat);
	DeleteFileW(out); wcscat_s(out,L".incomplete"); //cache
	
	HANDLE test = CreateFileW(out,
	GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);

	if(test==INVALID_HANDLE_VALUE) //TODO: messagebox
	{			
		assert(0); return false;
	}		
	else CloseHandle(test);	
	
	Somdownload_instance *in = new Somdownload_instance;

	memset(in,0x00,sizeof(*in));		
	wcscpy_s(in->incomplete,out);
	strcpy_s(in->resume,urlencode);	
	in->progress = progress; in->dialog = dialog; 
	DWORD DownlodMethod = 0, cb = sizeof(DWORD); SHGetValueW
	(HKEY_CURRENT_USER,L"SOFTWARE\\FROMSOFTWARE\\SOM",L"?DownloadMethod",0,&DownlodMethod,&cb);
	in->method = DownlodMethod==1?in->WinInet:in->WinSock;
	if(in->method==in->WinInet)
	{
		in->connection = 0; //setup by asynchronous callback
		in->internet = InternetOpen(SOMDOWNLOAD_AGENT,0,0,0,INTERNET_FLAG_ASYNC);	
		//default seems excessive considering how often ISPs timeout!
		unsigned long timeout = 5000; //5s (current default is 30ish)
		InternetSetOption(in->internet,INTERNET_OPTION_RECEIVE_TIMEOUT,&timeout,sizeof(long));
		InternetSetStatusCallback(in->internet,(INTERNET_STATUS_CALLBACK)Somdownload_handler);					
	}
	Somdownload_connect(in);
	if(in->method==in->WinSock) in->thread =
	CreateThread(0,0,(LPTHREAD_START_ROUTINE)Somdownload_threadmain,in,0,&in->id);

	//*: strip off .incomplete
	*PathFindExtensionW(wcscpy(inout,out)) = '\0';
	return true;
}

static DWORD WINAPI Somdownload_threadmain(Somdownload_instance *in)
{	
	FILE *f = in->thread?_wfopen(in->incomplete,L"wb"):0;	

	char buff[8192]; //observed	
	INTERNET_BUFFERSA rw; //annoying...
	memset(&rw,0x00,sizeof(rw)); rw.dwStructSize = sizeof(rw);
	rw.lpvBuffer = (BYTE*)buff;

	int statistics[4] = {0,0,0,0};
	
	if(in->WinSock==in->method) //NEW METHOD
	{
		int lim = sizeof(buff)-1;
		//2017: MSG_WAITALL isn't really supported by TCP and 
		//failed on XP (maybe without updates or drivers) and
		//I believe being "blocking" means it isn't necessary.
		int len = recv(in->socket,buff,lim,0/*MSG_WAITALL*/); 
		if(len<0) len = 0; buff[len] = '\0';
		const char tok[] = "\r\nContent-Length: ";
		char *p = strstr(buff,tok);
		if(p) statistics[1] = atoi(p+sizeof(tok)-1);
		p = strstr(buff,"\r\n\r\n"); 		
		if(!p) statistics[1] = 0; 
		if(p) statistics[0] = buff+len-p-4;
		if(p) fwrite(p+4,statistics[0],1,f);
	}
	else if(in->WinInet==in->method) //OBSOLETE?
	{
		Sleep(500); //winsanity: helps on some systems

		DWORD four = 4; assert(sizeof(int)==4);
		HttpQueryInfo(in->connection,HTTP_QUERY_CONTENT_LENGTH|HTTP_QUERY_FLAG_NUMBER,statistics+1,&four,0);	
	}

	std::wclog << "\n\nDebugging...\n";
	std::wclog << "\n\nDOWNLOADING " << statistics[1] << " TOTAL\n\n";

	DWORD began = GetTickCount(), Bs = 0, per = began;

	bool canceled = false;
	bool complete = statistics[0]>=statistics[1]; 
		
	size_t remaining = statistics[1]; //WinSock
	in->connection_timeout = GetTickCount(); //testing
	for(DWORD wr=0,error=0;!complete;rw.dwBufferLength=sizeof(buff)) 
	{	
		error = 0; const char *func = "";
								   		
		if(in->WinSock==in->method)
		{
			int len = //hurry up the last bit
			std::min(remaining,sizeof(buff));
			len = recv(in->socket,buff,len,0); //MSG_WAITALL
			if(len==SOCKET_ERROR)
			switch(len=WSAGetLastError()) //!!
			{
			case WSAEMSGSIZE: assert(0); len = sizeof(buff); break;

			default: assert(0); len = 0; break;				
			}
			statistics[0]+=len;	if(!len)
			{
				if(statistics[0]<statistics[1])
				complete = !Somdownload_resume(in);
			}
			else fwrite(buff,len,1,f); remaining-=len;
		}
		else if(in->WinInet==in->method)
		{			
			//black magic: WinInet wants to rest??
			//http://social.msdn.microsoft.com/Forums/en-US/25da2c43-5f8c-4627-a8f9-caed0810b7ff/wininet-hangs-in-internetreadfileinternetreadfileex?forum=vssmartdevicesnative
			Sleep(Somdownload_sleep); 

			if(!wr)
			{
				if(!f) break; //paranoia			   
				func = " (InternetQueryDataAvailable)";	   
				if(!InternetQueryDataAvailable(in->connection,&wr,0,0))
				error = GetLastError();	
			}												
			else if(InternetReadFileExA(in->connection,&rw,IRF_ASYNC,(DWORD_PTR)in))
			{
				func = " (InternetReadFileExA)";

				if(!rw.dwBufferLength) 
				{	
					std::wclog << "\n\nDebugging...\n";
					std::wclog << "\n\nBREAKING ON !rw.dwBufferLength\n\n";

					//NEW: assuming below comment is so
					//complete = true; break; //timeout 				
					complete = !Somdownload_resume(in);				
				}
				else if(!fwrite(buff,rw.dwBufferLength,1,f)) 
				{
					std::wclog << "\n\nDebugging...\n";
					std::wclog << "\n\nBREAKING ON !fwrite(buff,rw.dwBufferLength,1,f)\n\n";
					assert(0); break;
				}

				statistics[0]+=rw.dwBufferLength; 		
			
				wr-=rw.dwBufferLength;	   
			}	
			else 
			{
				func = " (InternetReadFileExA)";

				error = GetLastError();
			}

			//event trigger is initial connection on resume
			if(error==ERROR_INTERNET_INCORRECT_HANDLE_STATE)
			if(!complete) //paranoia
			continue;
		}

		if(statistics[0]>=statistics[1])
		{
			assert(statistics[0]==statistics[1]);

			std::wclog << "\n\nDebugging...\n";
			std::wclog << "\n\nBREAKING ON statistics[0]==statistics[1]\n\n";

			complete = true; break; //finish up outside loop
		}
		else //update the timing stats?
		{
			DWORD now = GetTickCount();

			/*#ifdef _DEBUG //testing resume
			if(now-in->connection_timeout>4000)
			{
				in->connection_timeout = now;
				if(!complete) Somdownload_resume(in);
			}
			#endif*/
						
			if(now-per>250) //report progress
			{	
				float seconds = float(now-per)/1000;
				statistics[2] = float(statistics[0]-Bs)/seconds;
				per = now; Bs = statistics[0];
				statistics[3] = float(now-began)/1000;
				if(!in->progress(statistics,in->dialog))
				{
					canceled = true; break; 
				}
			}
		}

		if(in->WinInet==in->method)
		{
			if(error)
			if(error==ERROR_IO_PENDING)							   
			{
				while(!(canceled=!in->progress(statistics,in->dialog)))			
				if(WAIT_TIMEOUT!=
				WaitForSingleObject(in->request_complete,1000))
				break;
				if(canceled) 
				break;
			}
			else 
			{
				//irregular ISP/network?
				//QUICKFIX: assuming connecting 
				//if(error==ERROR_INVALID_HANDLE) continue;

				std::wclog << "\n\nDebugging...\n";
				std::wclog << "\n\nBREAKING ON error!=ERROR_IO_PENDING\n\n";

				wchar_t *ferror = 0;

				FormatMessageW(
				FORMAT_MESSAGE_ALLOCATE_BUFFER| 
				FORMAT_MESSAGE_FROM_SYSTEM,0,error,
				MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
				(LPWSTR)&ferror,0,0);

				if(ferror)
				{
					std::wclog << "\n\nTHE ERROR: " << ferror << func << "\n\n";

					LocalFree(ferror);
				}

				assert(0); break; //TODO: more		
			}
		}
	}
	
	if(f) fclose(f); 

	if(statistics[1]>0)
	{
		if(statistics[0]>=statistics[1])
		{
			statistics[1] = complete?statistics[0]:0;
		}
		else statistics[1] = 0; //incomplete...
	}
	else if(!complete) statistics[1] = 0; //assuming??
	
	if(complete&&statistics[1])
	{
		wchar_t rename[MAX_PATH];
		wcscpy(rename,in->incomplete);
		wchar_t *_incomplete = wcsrchr(rename,'.');
		if(_incomplete&&!wcscmp(_incomplete,L".incomplete")) //paranoia
		{
			*_incomplete = '\0'; //strip .incomplete from file name
		}
		if(!MoveFileW(in->incomplete,rename)) assert(0);
	}

	if(!canceled)
	while(!(canceled=!in->progress(statistics,in->dialog)))
	Sleep(250); //keeping alive

	Somdownload_disconnect(in);
	if(in->method==in->WinInet)
	{
		InternetSetStatusCallback(in->internet,0);	
		InternetCloseHandle(in->internet);
		CloseHandle(in->request_complete);
	}	
	delete in; return S_OK;
}

static void CALLBACK Somdownload_handler
(
  HINTERNET hInternet,
  DWORD_PTR dwContext,
  DWORD dwInternetStatus,
  LPVOID lpvStatusInformation,
  DWORD dwStatusInformationLength
)
{	if(!dwContext) return;

	Somdownload_instance *in = 
	(Somdownload_instance*)dwContext;

	#define OUT(X) \
	if(GetCurrentThreadId()==in->id) std::wcout\
	<< in->resume << " -> " << in->incomplete << '\n'\
	<< " " #X " (" << dwStatusInformationLength << ')'
	#define CASE(X) \
	break; case INTERNET_STATUS_##X: OUT(X)  << '\n';

	switch(dwInternetStatus)
	{
	default: OUT(UNKNOWN) << '\n';
	CASE(CLOSING_CONNECTION)
	CASE(CONNECTED_TO_SERVER)
	
		in->connected = true;

	CASE(CONNECTING_TO_SERVER)
	CASE(CONNECTION_CLOSED)
	CASE(HANDLE_CLOSING)
	
		if(*(HANDLE*)lpvStatusInformation==in->connection)
		in->connected = false; //NEW: resuming
		else assert(0); 

	CASE(HANDLE_CREATED)
	CASE(INTERMEDIATE_RESPONSE)
	CASE(NAME_RESOLVED)
	CASE(RECEIVING_RESPONSE)
	CASE(RESPONSE_RECEIVED)
	CASE(REDIRECT)	
	CASE(REQUEST_COMPLETE)
	{
		SetEvent(in->request_complete);
		auto i = (LPINTERNET_ASYNC_RESULT)lpvStatusInformation;
		if(!i->dwError)
		{
			bool resuming = in->connection; //NEW
			if(i->dwResult) in->connection = (HINTERNET)i->dwResult; 
			else assert(0); if(!resuming) //hack: once per session
			{					
				in->request_complete = CreateEvent(0,0,0,0);

				in->thread = CreateThread
				(0,0,(LPTHREAD_START_ROUTINE)Somdownload_threadmain,
				in,0,&in->id);				
			}					
		}
		else 
		{
			DWORD error = GetLastError(); assert(0);

			std::wcout << "Error (" << error << ") encountered\n";
		}		
		break;
	}
	CASE(REQUEST_SENT)
	CASE(RESOLVING_NAME)
	CASE(SENDING_REQUEST)
	CASE(STATE_CHANGE) //documentation looks screwy

	assert(0); //INTERNET_STATE_DISCONNECTED?  	
	}	
}