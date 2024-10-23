#include <streams.h>
#include "pcmdest.h"

//WARNING //WARNING //WARNING //WARNING //WARNING

	//The WavDest sample had __stdcall set as its
	//calling convention. It seemed to change how
	//Microsoft Detours worked, but even after it
	//is fixed, GetProcAddress must be called for 
	//the hook to be installed within the DLL????

//WARNING //WARNING //WARNING //WARNING //WARNING

extern IBaseFilter *SOM_SDK_CreateInstance_pcmdest(HRESULT*phr)
{
	//return (IBaseFilter*)PCM_DestFilter::CreateInstance(0,phr);
	PCM_DestFilter *o = new PCM_DestFilter(0,phr);
	if(1!=o->AddRef()) KASSERT(0); //assert(0);
    return static_cast<IBaseFilter*>(o);
}
