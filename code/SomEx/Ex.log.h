											   			 
#ifndef EX_LOG_INCLUDED
#define EX_LOG_INCLUDED

namespace EX
{
	enum 
	{
	debug_log = 1,
	error_log = 2,
	panic_log = 4,
	alert_log = 8,
	hello_log = 16,
	sorry_log = 32,	  
	};

	//NOT THREAD-SAFE
	//2018: SOM_EX is always on/so has to change files
	extern void closing_log();

	//EX::log: 
	//This interface allows for per file/namespace  
	//runtime configuration of logging levels (seen below) 
	//
	//Each source file calls EX::log with _FILE_ for a given
	//set of static (per file) log level parameters when the 
	//source object is loaded (prior to the main entrypoint)
	//
	//file/ns pointers are expected to be persistent/const
	extern int *including_log(const wchar_t *object, const wchar_t *ns,
	//these addresses are expected to be persistent/static (sorry is under consideration)
	int debug[1], int error[1], int panic[1], int alert[1], int hello[1], int sorry[1]=0);
	
	//EX::logging:
	//Set logging level to int (neg/pos infinity)
	//Logs is a bitwise combination of EX::debug_log etc
	//Ns is matched against the namespace, and of the object
	//An empty string will match anything
	extern int logging(int bar, int logs=0xF, const wchar_t *ns=L"", const wchar_t *of=L"");
		
	extern int &logging_onoff(bool flip=true); //cycle master switch
	 	
	inline bool logging(){ return EX::logging_onoff(false); }

	inline void logging_off(){ if(EX::logging()) EX::logging_onoff(); }
	inline void logging_on(){ if(!EX::logging()) EX::logging_onoff(); }

	//is_log_object:
	//Greedily returns the last character matching any logged object
	//Partial: if false, returns 0 if the returned character is A-z0-9.
	extern const wchar_t *is_log_object(const wchar_t*, bool partial=0);

	//is_log_namespace:
	//Returns first non-alphanumerical character if 
	//preceeding string matches any logged namespace.
	extern const wchar_t *is_log_namespace(const wchar_t*);
	  	
	//Rationale: the plan is for this to work in
	//concert with EXLOG_LEVEL and friends towards
	//internationalizing the, to be, Ex console. Eg.
	/*
	  static EX::Format Ex;

	  char *World = "People of planet Earth";

	  //in other words: printf("Hello %s\n",World);
	  EXLOG_LEVEL(0) << "Hello " << Ex%World << '\n';

	  //in other words: World = gettex(World);
	  EXLOG_LEVEL(0) << "Hello " << Ex[World] << '\n';
	*/	
	struct monolog
	{			
		static std::wofstream log;
		template<typename T> inline
		std::wostream &operator<<(const T&t){ return log<<t; }

		#ifdef _WIN32
		static HANDLE handle; //REMOVE ME?		
		#endif
	};
	struct prefix 
	{											
		const char *prefix_; 
		prefix(const char *p=""):prefix_(p){} 
		prefix(const char*,const char*,size_t=sizeof(prefix)); 		

		template<typename T> 
		inline const T &operator[](const T& t){ return t; }	
	};	
	typedef struct : prefix
	{
#ifdef EX_PREFIX_PREFIXES
						
		prefix EX_PREFIX_PREFIXES;
#endif
	}prefixes;
	//template lets the linker generate unique layouts
	template<class prefixes> struct Prefix_ : prefixes
	{
		EX::monolog log;

		template<typename T> 
		inline const T &operator%(const T& t){ return t; }

#ifdef EX_PREFIX_PREFIXES
		
		#define _(x) #x
		Prefix_(){ new (this) prefix("",_(EX_PREFIX_PREFIXES)); } 
		#undef _
#endif		
	};	
	typedef Prefix_<prefixes> Prefix;
}

#endif //EX_LOG_INCLUDED