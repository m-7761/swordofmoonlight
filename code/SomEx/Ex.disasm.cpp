
#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

//TODO: See if it's not possible to use disasm.cpp from Detours 
//as a lightweight alternative to BeaEngine minus f11 disassembly.
//At a glance, it certaintly appears doable (DetourCopyInstructionEx)

#define BEA_USE_STDCALL
//#define BEA_ENGINE_STATIC

//why aren't we delay loading??
//#pragma comment(lib,"delayimp")
//#pragma comment(lib,"BeaEngine.lib") 

#include "beaengine/BeaEngine.h" 

#include "Ex.memory.h"

static DISASM Ex_disasm = {1}; //BeaEngine.h

static int (__bea_callspec__ *Ex_disasm_f)(LPDISASM) = 0;

static bool Ex_disasm_thunk() 
{
	Ex_disasm.SecurityBlock = 0; //paranoia

	if(Ex_disasm_f) return true;
	
	static HMODULE beaengine = LoadLibraryA("BeaEngine.dll");

	if(!beaengine) return false;

	typedef int (__bea_callspec__ *f)(LPDISASM);

	if(Ex_disasm_f=(f)GetProcAddress(beaengine,"_Disasm@4"))
	{
		memset(&Ex_disasm,0x00,sizeof(Ex_disasm));
	}

	return Ex_disasm_f;	
}

extern char Ex_disasm_lde(intptr_t lo, intptr_t ep, intptr_t hi)
{
	if(!Ex_disasm_thunk()) return 0;

	Ex_disasm.EIP = ep; Ex_disasm.SecurityBlock = hi-ep;

	int out = Ex_disasm_f(&Ex_disasm);

	if(out>0)
	{
		assert(out<16);

		return out<16?out:0;
	}
	else if(out==UNKNOWN_OPCODE)
	{
		assert(0);
	}
	else if(out==OUT_OF_BLOCK)
	{
		assert(0);
	}

	return out;
}

extern int Ex_disasm_sum(intptr_t lo, intptr_t ep, intptr_t hi, BYTE *out)
{
	if(!Ex_disasm_thunk()) return 0;

	int sum = 0; if(ep>=hi||ep<lo) return 0;

	while(ep<hi)
	{
		if(out) //been here before
		{
			if(out[ep-lo]) return sum;
		}
				
		Ex_disasm.EIP = ep; 
		Ex_disasm.SecurityBlock = hi-ep;

		int len = Ex_disasm_f(&Ex_disasm);

		if(len!=UNKNOWN_OPCODE)
		{	
			if(len>0)
			{
				assert(len<16);
				
				BYTE courtesy = len;

				while(ep<hi&&len--)
				{
					out[ep-lo] = courtesy; ep++;
				}

				sum++;
			}
			else //should not happen
			{
				assert(0); return 0;
			}			
		}
		else return sum; //junk

		if(out) //required for branching
		if(Ex_disasm.Instruction.AddrValue) 
		sum+=Ex_disasm_sum(lo,Ex_disasm.Instruction.AddrValue,hi,out);		
	}

	return sum;
}

extern const char *Ex_disasm_asm(intptr_t ep)
{
	if(!Ex_disasm_thunk()) return 0;

	Ex_disasm.EIP = ep; 
	Ex_disasm.SecurityBlock = 16;

	int len = Ex_disasm_f(&Ex_disasm);

	if(len>0)
	{
		Ex_disasm.CompleteInstr[-1] = len; 

		return Ex_disasm.CompleteInstr;
	}
	else return 0;
}