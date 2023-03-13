
const char *SOM::T::header(SOM::Shader x)
{
	switch(model)
	{
	case 'vs_3': case 'ps_3':
	if(x==SOM::Shader::classic)
	return som_shader_source(model=='ps_3',x);
	}
	assert(0); return 0;
}
const char *SOM::T::shader(SOM::Shader x)
{
	switch(model)
	{
	case 'vs_3': case 'ps_3':
	if(x!=SOM::Shader::classic)
	return som_shader_source(model=='ps_3',x);
	}
	assert(0); return 0;
}

void SOM::T::define(SOM::Define x,...)
{
	va_list va; va_start(va,x);	
				
	F(som_shader_define)(x,va,model); 
}

void SOM::T::undef(SOM::Define x)
{
	som_shader_undef(x,model); 
}

bool SOM::T::ifdef(SOM::Define x)
{
	return som_shader_ifdef(x,model); 
}

char *SOM::T::compile(SOM::Shader x, int index)
{
	return (char*)F(som_shader_compile)(EX::compiling_shader,model,x,index); 
}

void SOM::T::configure(int count, char *x,...)
{
	int *at = 0; va_list va; va_start(va,x);	
	
	switch(model)
	{
	case 'vs_3': case 'vs_2': at = som_shader_arraytable_vs;
	}

	som_shader_configure(count,x,va,at); 
}

void SOM::T::release(int count, char *x,...)
{
	va_list va; va_start(va,x);	

	som_shader_release(count,x,va); 
}

const DWORD *SOM::T::assemble(const char *x)
{
	return EX::assembling_shader(x,'dx9c');
}

const DWORD *SOM::T::debug(SOM::Shader x, int index)
{
	return (DWORD*)F(som_shader_compile)(EX::debugging_shader,model,x,index); 
}