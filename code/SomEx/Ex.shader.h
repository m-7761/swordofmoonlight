			  
#ifndef EX_SHADER_INCLUDED
#define EX_SHADER_INCLUDED

struct ID3D10Blob;

namespace EX
{
	//defining_shader_macro:
	//define a preprocessor macro; undecorated name, space, and definition
	//returned number is incremented each time called with nonzero argument
	int &defining_shader_macro(const char *name_definition=0);

	//2020: new, remove individual macro?
	void undefining_shader_macro(const char *name_definition);

	//undefining_shader_macros:
	//call before compiling with fresh macros
	//returned number is same as defining_shader_macro's incremented
	int undefining_shader_macros(); 

	//D3DXPreprocessShader?
	char *preprocessing_shader(const char *code); //2022 

	//compiling_shader:
	//type may be 'vs_1' 'ps_1' 'vs_2' and so on. 
	char *compiling_shader(int type, const char *main, const char *code, const wchar_t *file=0, int lang='hlsl');

	//linking_shaders:
	//link two or more shaders with overlapping constant registers
	//each char* is assumed to be 0 terminated and from compiling_shader()
	//memory pointed to is modified in preparation for assembling_shader()
	//returns linkage of constant registers via an internal buffer
	//the first int returned is the number of ints. The next int is the
	//number of register types. The next int is a register type. Either
	//'vs_c', 'vs_i', 'vs_b' and so on. A combination of vs_/ps_ and c, i, or b.
	//the next int is the number of register ranges of that type which follow.
	//the next n*2 ints are the ranges for that type, where each even int begins
	//a range, and each odd is the number of registers within that range. 
	//disregard any range with a register count of zero. 
	//the returned pointer is good until the next call.
	//arrays:
	//where constant arrays are accessed dynamically it's impossible to
	//discern the bounds of the arrays from the disassembly. Therefore the
	//bounds should be passed via arrays in the form of an array table, which
	//follows the same format as the memory pointed to by the return value.
	//Ie. each range provided describes a constant array.
	const int *linking_shaders(char **compilations, int count, const int *arrays=0);
	
	//assembling_shader:
	//compilation assumed to be from compiling_shader() and 0 terminated
	const DWORD *assembling_shader(const char *compilation, int target='dx9c'); 

	template<typename T> inline T *assembling_shader(const char *linked);

	template<> inline ID3D10Blob *assembling_shader(const char *linked)
	{
		return (ID3D10Blob*)assembling_shader(linked,'dx10');
	}

	//debugging_shader: hlsl code only
	//NOTE: this just goes straight to the compiled shader
	//the final two args are ignored/exist so to match compiling_shader signature
	const DWORD *debugging_shader(int type, const char *main, const char *code, void*_=0, int=0);

	//releasing_shaders:
	//invalidates all pointers returned by compiling/assembling_shader
	void releasing_shaders(); 
}

#endif //EX_SHADER_INCLUDED
