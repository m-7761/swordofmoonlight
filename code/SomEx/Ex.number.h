		  			  
#ifndef EX_NUMBER_INCLUDED
#define EX_NUMBER_INCLUDED

//#define EX_CALCULATORS

#include "Ex.langs.h" //isdigit/isalpha issues

namespace EX
{	
	class Calculator;

	#define C Calculator

	//_don't use these_
	extern const float NaN;

	inline bool isNaN(const float &f)
	{
		int compile[sizeof(float)==sizeof(int)];

		return *(int*)&EX::NaN==*(int*)&f;
	}
	//2021: test for negative zeroes!
	inline bool isNeg(const float &f)
	{
		int compile[sizeof(float)==sizeof(int)];

		return 0!=(0x80000000&*(int*)&f);
	}
	
	 //A word of warning:
	//
	// The first two ways to assign numbers
	// introduce hidden variables. These are
	// capital N numbers. The third assigning
	// procedure assigns to "little n" numbers 
	// Which is admittedly confusing since they
	// use EX::Number big N insted of EX::number
	
	enum _sysenum{ variable, constant, mismatch };
	//using to query system numbers; eg. c _WALK etc.
	//The number of parameters is passed through float*
	//(could add a bool complex flag to assigning_number)
	typedef _sysenum (*_sysnum)(C*,const wchar_t*,size_t,float*);

	//key/value pair. The key must be = or 0 terminated	
	extern void assigning_number(C*,const wchar_t*,_sysnum,size_t);
	extern void assigning_number(C*,const wchar_t*,const wchar_t*);
	extern void debugging_number(C*,const wchar_t*,const wchar_t*);						
	//NEW: adds keynames and labels prior to assignment
	extern void declaring_number(C*,const wchar_t*,const wchar_t*);
	//NEW: may not be done correctly???
	extern float reevaluating_number(C*,const wchar_t*);

	//These were designed to work with EX::Number
	template<int,int,int,int,int,int> struct Number; 
	//const wchar_t *expression, float min, float max	
	//returns an internal buffer for serial assignment
	extern const wchar_t *assigning_number(C*,const float**,const wchar_t*,float,float);	
	//float* must come from assigning_number(&f)
	extern bool reevaluating_number(const float *f);
	//2020: returns true if f is constant
	extern bool evaluating_number(const float *f,unsigned of,/*float*/...);		
	/*float*'s must be "resigned" from memory*/
	extern void resigning_number(const float*);
	 				 	
	struct Calculation{ C *calculator; const wchar_t *expression; };

	/*CAUTION: must cast to float with printf*/
	//(really we need a compiler that gives warnings about ... args)
	//WARNING: does not act like float inside of ?: operators (solution??)
	template<int mi,int n, int Na,int N, int ma,int x> struct Number
	{	
	private: const float *_result; public: Number(){ _result = &EX::NaN; }

		~Number(){ if(_result!=&EX::NaN) EX::resigning_number(_result); }

		bool operator&()const{ return _result!=&EX::NaN; } //assigned
																		
		static inline float o(int a=Na, int b=N){ return float(b)/100000+a; }		

		//2021: This bypasses NaN test/conversion.
		float operator*()const{ return *_result; }
		operator float()const{ return EX::isNaN(*_result)?o(Na,N):*_result; }

		Number &equal(C *c, const wchar_t *expression, const wchar_t **serial=0)
		{
			if(!expression) //NEW: support Ex.ini file extended .extension hiding syntax
			{ this->~Number(); _result = &EX::NaN; if(serial) *serial = 0; return *this; }

			const wchar_t *sep = EX::assigning_number(c,&_result,expression,o(mi,n),o(ma,x));

			if(serial) *serial = sep; else if(sep) EX::numbers_mismatch(true,expression); return *this;
		}						
		Number &operator=(const EX::Calculation &c){ return equal(c.calculator,c.expression); }
		Number &operator=(const wchar_t *expression){ return equal(0,expression); }		

		private:void operator=(const Number&);Number(const Number&);public: //nix

		//returns the next expression in serial assignments
		const wchar_t *operator<<=(const wchar_t *expression) 
		{
			const wchar_t *out; equal(0,expression,&out); return out;
		}						
		EX::Calculation operator<<=(EX::Calculation c) 
		{
			equal(c.calculator,c.expression,&c.expression); return c;
		}						

		//returns the result of evaluation with parameters
		inline const Number &operator()(float _1, float _2, float _3)const
		{
			EX::evaluating_number(_result,3,_1,_2,_3); return *this;
		}	
		inline const Number &operator()(float _1, float _2)const
		{
			EX::evaluating_number(_result,2,_1,_2); return *this;
		}	
		inline const Number &operator()(float _1)const
		{
			EX::evaluating_number(_result,1,_1); return *this;
		}	
		inline const Number &operator()(bool *constant=0)const
		{
			bool ret = EX::reevaluating_number(_result);
			
			if(constant) *constant = ret; return *this;
		}	

		//NEW: single input adjustment functor
		inline void operator()(float *_1)const
		{
			if(!operator&()) return;
			EX::evaluating_number(_result,1,*_1); 
			if(!EX::isNaN(*_result)) *_1 = *_result;
		}	
	};	
	
	extern void reevaluating_numbers(C*); //includes numberings below

	//int balance, size_t length, wchar_t *expressions, float min *nan max 
	//WARNING: when a float* is reassigned it's assumed size_t remains the same
	//This one is used to setup an array based series that cannot include parameters 
	extern int assigning_numbering(C*,const float**,size_t,const wchar_t*,float,const float*,float);

	template<size_t len, int mi,int n, const float (&NaN)[len], int ma,int x> struct Numbers
	{			
	private: const float *_result; public: Numbers(){ _result = NaN; }

		~Numbers(){ if(_result!=NaN) EX::resigning_number(_result); }

		bool operator&()const{ return _result!=NaN; } //assigned
		
		static inline float o(int a, int b){ return float(b)/100000+a; }		

		typedef const float numbering[len];
		inline operator numbering&()const{ return *(numbering*)_result; }

		Numbers &equal(C *c, const wchar_t *expressions, bool underflow=false)
		{
			if(!expressions){ this->~Numbers(); _result = NaN; return *this; } //.syntax

			int cmp = EX::assigning_numbering(c,&_result,len,expressions,o(mi,n),NaN,o(ma,x));

			if(cmp>0||cmp<0&&underflow) EX::numbers_mismatch(cmp>0,expressions); return *this;
		}						
		Numbers &operator=(const EX::Calculation &c){ return equal(c.calculator,c.expression); }
		Numbers &operator=(const wchar_t *expressions){ return equal(0,expressions); }	

		private:void operator=(const Numbers&);Numbers(const Numbers&);public: //nix
								 
		inline const Numbers &operator()()const //cannot include parameters
		{
			EX::reevaluating_number(_result); return *this; 
		}			
	};

	//built in random number generator
	extern void including_number_r(C*, const wchar_t *keyname=L"r"); 
	//built in base encoding facilities with decoder
	extern void including_number_id(C*, const wchar_t *id=L"id", const wchar_t *di=L"di"); 

	 //Ex.output.cpp
	//returns editing/testing_number key    //closest match//next//filter eg. "_"
	extern const wchar_t *finding_number(C*,const wchar_t*,int=0,const wchar_t*f=0);
	//returns next number part      //finding key  //part	   //expression or _TOO_LONG
	extern size_t editing_number(C*,const wchar_t*,size_t=-1UL,wchar_t*ed=0,size_t ed_s=0);
	//true if not literal         //finding key  //part //default yield of function 
	extern bool testing_number(C*,const wchar_t*,size_t,wchar_t*of=0,size_t of_s=0);

	//Exselector
	extern void resetting_numbers_to_scratch(C*);

	//undocumented
	extern void refreshing_numbers_en_masse(C*);

	#undef C //Calculator
}

#endif //EX_NUMBER_INCLUDED