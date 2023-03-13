
#ifndef EX_MEMORY_INCLUDED
#define EX_MEMORY_INCLUDED

namespace EX
{	
	struct Program; 

	extern const struct Section 
	{
		int number;

		mutable const wchar_t *name;

		mutable const EX::Program *code;

		Section *prev, *next; 

		MEMORY_BASIC_INFORMATION info;

		inline Section()
		{
			prev = next = 0; number = -1; name = 0; code = 0;
		};

		inline void *at(DWORD offset)const
		{
			return this?(void*)(DWORD(info.BaseAddress)+offset):0;
		}

		inline const EX::Section *no(int num)const
		{		
			const EX::Section *p = this;
			while(p&&p->number!=num) p = p->next;
			return p&&p->number==num?p:0;
		}

		inline const EX::Section *by(const void *ptr, DWORD *at=0)const
		{		
			if(at&&this)
			{
				*at = ptr?DWORD(ptr):0; 
				*at-=(DWORD)info.BaseAddress;
			}

			for(const EX::Section *p=this;p;p=p->next)
			{
				if(ptr>=p->info.BaseAddress)
				{
					if(ptr>=(BYTE*)p->info.BaseAddress
							   	  +p->info.RegionSize)
					{
						if(at&&p->next)
						*at-=(DWORD)p->next->info.BaseAddress
						    -(DWORD)p->info.BaseAddress;							
					}
					else return p;
				}
				else break;
			}

			return 0;
		}

		template<typename T>
		inline const EX::Section *by(const void *ptr, T *at)const
		{
			DWORD tmp; const EX::Section *out = by(ptr,&tmp); 
			
			if(out&&at) *at = tmp; return out;
		}

		inline const EX::Section *wrap()const
		{
			const EX::Section *p = this;
			while(p&&p->next) p = p->next; return p;
		}

	}*memory(const void *extend=0,size_t=0);
	
	template<typename T> struct Pointer
	{
		static T *null; T **t; 

		inline T **operator&(){ return t; }

		inline T *&operator*(){ return *t; }

		inline T *operator+(int n){ return *t+n; }

		inline operator T*&(){ return t?*t:null; };

		inline operator DWORD(){ return DWORD(t?*t:null); }; //hack

		inline Pointer(void *p){ *(void**)&t = p; }

		inline void operator=(void *p)
		{
			*(void**)&t = p;
		}
	};

	extern const struct Program
	{			
		size_t opcount;

		const int *opcodes;
		const char *opchars;

		intptr_t loboundary; 
		intptr_t entrypoint;
		intptr_t hiboundary; 
		
		mutable size_t *scratch;
		
		template<typename T> 
		inline T *scratchmem(size_t)const;

		inline void scratchmem()const
		{
			if(scratch) delete[] --scratch; scratch = 0;
		}

		const EX::Section *text;		
		//const EX::Section *reloc()const;

		inline void *at(int op)const
		{
			return this&&op>=0&&size_t(op)<opcount? (void*)(loboundary+opcodes[op]):0;
		}

		int by(const void*)const; //binary lookup

		const char *x(int op, const char *ln=0)const;

		const struct x86
		{
			union
			{
			int mnemonic; //eg. 'add'

			char debug[4]; //for visual debugger
			};

			char _; //debugging: always 0

			char runlength;
			char reserved0;
			char immediate;			

			unsigned address; //branch
			
			struct Argument
			{
				union
				{
				int mnemonic; //eg. 'eax'

				char debug[4]; //for visual debugger
				};

				char _; //debugging: always 0

				char mode; //r/w
				char size;
				char sreg; 

			}args[3];

		}&x86(int op, int count=1)const;

		Program()
		{
			memset(this,0x00,sizeof(*this));
		}

		~Program()
		{
			delete[] opcodes;
			delete[] opchars;
			
			scratchmem();
		}

	}*program(const EX::Section *text=0, intptr_t ep=0);

	typedef struct EX::Program::x86 x86_t;

	#pragma pack(1) 
	static const struct Headers 
	{			
		DWORD pe;
		IMAGE_FILE_HEADER file;
		IMAGE_OPTIONAL_HEADER32 optional;

	}*header()
	#pragma pack()
	{
		const EX::Section *pe = EX::memory();

		if(pe&&unsigned(pe->info.BaseAddress)==0x400000)
		if(pe&&unsigned(pe->next->info.BaseAddress)==0x401000)
		{
			IMAGE_DOS_HEADER *dos = 
			(IMAGE_DOS_HEADER*)pe->info.BaseAddress;

			if(dos->e_magic==*(WORD*)"MZ")
			{
				EX::Headers *out = (EX::Headers*)
				((BYTE*)pe->info.BaseAddress+dos->e_lfanew);

				if(out->pe==*(DWORD*)"PE\0\0"
				 &&out->file.Machine==0x14c //i386
				 &&out->optional.Magic==0x10B) //?
				{
					return out; //looks good
				}
			}
		}

		return 0;
	} 

	//// stuff hastily transplanted from Ex.output.cpp ////
		   	
	extern const wchar_t *name_of_memory(const EX::Section *in);

	extern void code_view_of_memory(const EX::Program*);
	extern void data_view_of_memory(const EX::Section*);
	extern void move_view_of_memory(int n, bool scroll=true);
	extern void next_view_of_memory(int n, bool scroll=true);
	extern void scan_view_of_memory(int n);

	extern const char *command_view_of_memory(const char*);

	///////////////////////////////////////////////////////

	//this is provided to determine if a caller is the EXE or not
	//NEW: returns def if frame pointers are not present (just risk it)
	__declspec(noinline) static PVOID return_address(DWORD def=0x401000)
	{		 
		//2021: What about this?
		//https://web.archive.org/web/20050907224522/http://msdn.microsoft.com/library/en-us/vclang/html/vclrf_returnaddress.asp
		//_ReturnAddress,_AddressOfReturnAddress 
		//https://stackoverflow.com/questions/57621889/getting-the-callers-return-address

		//CaptureStackBackTrace 
		static USHORT(WINAPI*RtlCaptureStackBackTrace)(ULONG,ULONG,PVOID*,PULONG) = 
		(USHORT(WINAPI*)(ULONG,ULONG,PVOID*,PULONG))GetProcAddress(GetModuleHandleA("Kernel32.dll"),"RtlCaptureStackBackTrace");
		assert(RtlCaptureStackBackTrace);
		PVOID out = 0;
		if(!RtlCaptureStackBackTrace(2,1,&out,0)||!out) //2: determined experimentally 
		out = (PVOID)def;
		return out;		
	}
}

template<typename T>
inline T *EX::Program::scratchmem(size_t atleast)const
{
	size_t sz = atleast*sizeof(T);

	if(scratch)
	if(scratch[-1]>=sz)
	return (T*)scratch; else delete[] --scratch;	     

	scratch = new size_t[1+sz/sizeof(size_t)+1];

	if(!scratch) return 0; *scratch++ = sz;
	
	return (T*)scratch;
}

#endif //EX_MEMORY_INCLUDED