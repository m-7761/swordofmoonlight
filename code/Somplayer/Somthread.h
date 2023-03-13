
#ifndef SOMTHREAD_INCLUDED
#define SOMTHREAD_INCLUDED

//WARNING:
//This header employs an anonymous namespace
//so don't include it in precompiled headers

#ifndef SOMTHREAD_HOLLOW
#ifndef  _MSC_VER
#	error Compiler not supported
#endif
#endif

namespace Somthread_h
{	
   //_todo list_    
   //*thread local storage (TlsAlloc)

	struct account //atomic counter
	{	
		account(long i=0){ counter = i;}

		inline operator long(){	return counter; }

	#ifndef SOMTHREAD_HOLLOW
		
		inline void operator=(long i){ InterlockedExchange(&counter,i); }
		
		inline long operator++(){ return InterlockedIncrement(&counter); }
		inline long operator--(){ return InterlockedDecrement(&counter); }		

		inline void operator++(int){ InterlockedIncrement(&counter); }
		inline void operator--(int){ InterlockedDecrement(&counter); }		

		private: volatile LONG counter; 

	#else

		inline void operator=(long i){ counter = i; }
		
		inline long operator++(){ return ++counter; }
		inline long operator--(){ return --counter; }		

		inline void operator++(int){ counter++; }
		inline void operator--(int){ counter--; }		

		private: long counter; 

	#endif
	};

    //// locks: cannot be nested /////////////////////
		
	struct interlock; //shared read (fast STL wrapper)

	struct intralock //intralock hides in an interface
	{	
		inline operator interlock&()
		{
			return (interlock&)*this; //RAII
		}

	#ifndef SOMTHREAD_HOLLOW

	//http://stackoverflow.com/questions/4203467/
	//multiple-readers-single-writer-locks-in-boost/4205717#4205717

		volatile LONG readers, writers;

		intralock(){ readers = writers = 0; }

		~intralock(){ assert(!readers&&!writers); }

		void readlock()
		{
			while(1)
			{        
				if(writers!=0){ Sleep(0); continue; }

				InterlockedIncrement(&readers);	if(writers==0) break;
				InterlockedDecrement(&readers); Sleep(0);
			}
		}			  
		LONG readunlock_if_any() //do not mix with readunlock
		{
			return readers?InterlockedDecrement(&readers):0; 
		}
		void readunlock()
		{ 
			InterlockedDecrement(&readers); 
		}

		void writelock()
		{
			while(InterlockedExchange(&writers,1)==1) Sleep(0);

			while(readers!=0) Sleep(0);
		}	
		void writeunlock()
		{
			writers = 0; 
		}

	#else

		void readlock(){}			  
		LONG readunlock_if_any(){ return 0; }
		void readunlock(){}
		void writelock(){}	
		void writeunlock(){}

	#endif
	};

	struct readlock; 
	struct writelock; //RAII

	struct interlock : private intralock
	{
		friend struct readlock; friend struct writelock; 
	};

	struct readlock
	{
	#ifndef SOMTHREAD_HOLLOW
		
		inline void resume(){ lock.readlock(); }
				
		readlock(interlock &i):lock(i){ resume(); }

		inline void suspend(){ lock.readunlock(); }		

		~readlock(){ suspend(); }

	private: interlock &lock;

	#else

		inline void resume(){}

		readlock(interlock &i){}		
				
		inline void suspend(){}
		
	#endif
	};

	struct writelock
	{
	#ifndef SOMTHREAD_HOLLOW
				
		inline void resume(){ lock.writelock(); }
				
		writelock(interlock &i):lock(i){ resume(); }

		inline void suspend(){ lock.writeunlock(); }

		~writelock(){ suspend(); }		

	private: interlock &lock;

	#else

		inline void resume(){}

		writelock(interlock &i){}		
				
		inline void suspend(){}
		
	#endif
	};					
		
	struct await //atomic wait
	{	
	#ifdef SOMTHREAD_HOLLOW

		template<typename LONG> await(LONG &a){}

	#else

		volatile LONG &core; LONG b; //plan B

		template<typename LONG> await(volatile LONG &a)
		:
		core((unsigned long long)&a>512?(long&)a:b),b(0) 
		{
			while(InterlockedIncrement(&core)!=1)
			{
				InterlockedDecrement(&core); Sleep(0); 				
			}
		}
		~await(){ InterlockedDecrement(&core); }

	#endif
	};

	template<typename LONG> void await_lock(volatile LONG &a)
	{
	#ifndef SOMTHREAD_HOLLOW

		InterlockedIncrement(&(long&)a); 				

	#endif
	}
	template<typename LONG> void await_unlock(volatile LONG &a)
	{
	#ifndef SOMTHREAD_HOLLOW

		InterlockedDecrement(&(long&)a); 	

	#endif
	}

    //// locks: CAN be nested ////////////////////////
	
	namespace //anonymous
	{	
		struct section;			  

		struct intersect
		{			
		#ifndef SOMTHREAD_HOLLOW

			intersect(){ InitializeCriticalSection(&cs); }
			~intersect(){ DeleteCriticalSection(&cs); }

		private: friend struct section;

			CRITICAL_SECTION cs;

		#endif
		};
		
		//one per translation unit
		static intersect intrasect; 
		
		struct section //RAII
		{	
		#ifdef SOMTHREAD_HOLLOW

			section(intersect &is=intrasect){}
			
		#else

			~section(){ LeaveCriticalSection(&cs); }

			section(intersect &is=intrasect):cs(is.cs)
			{
				EnterCriticalSection(&cs); 
			}

		private: CRITICAL_SECTION &cs;

		#endif 
		};
	}	
}

#endif //SOMTHREAD_INCLUDED