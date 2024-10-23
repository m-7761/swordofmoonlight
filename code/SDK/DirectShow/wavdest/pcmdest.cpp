
#include <streams.h>
#include "pcmdest.h"
#include <aviriff.h>
#include <malloc.h>


//SOM_SDK
// {3C78B8E2-6C4D-11d1-ADE2-0000F8754B99}
static const GUID &CLSID_WavDest =
//{ 0x3c78b8e2, 0x6c4d, 0x11d1, { 0xad, 0xe2, 0x0, 0x0, 0xf8, 0x75, 0x4b, 0x99 } };
GUID_NULL;


const AMOVIESETUP_FILTER sudWavDest =
{
    &CLSID_WavDest,           // clsID
    L"WAV Dest",              // strName
    MERIT_DO_NOT_USE,         // dwMerit
    0,                        // nPins
    0                         // lpPin
};


// Global data
CFactoryTemplate g_Templates[]= {
    {L"PCM Dest", &CLSID_WavDest, PCM_DestFilter::CreateInstance, NULL, &sudWavDest},
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


// ------------------------------------------------------------------------
// filter constructor

#pragma warning(disable:4355)


PCM_DestFilter::PCM_DestFilter(LPUNKNOWN pUnk, HRESULT *phr) :
                CTransformFilter(NAME("WavDest filter"), pUnk, CLSID_WavDest),
                m_cbWavData(0),
                m_cbHeader(0)
{
    ASSERT(m_pOutput == 0);
    ASSERT(phr);

    if(SUCCEEDED(*phr))
    {
        // Create an output pin so we can have control over the connection
        // media type.
        PCM_DestOutputPin *pOut = new PCM_DestOutputPin(this, phr);

        if(pOut)
        {
            if(SUCCEEDED(*phr))
            {
                m_pOutput = pOut;
            }
            else
            {
                delete pOut;
            }
        }
        else
        {
            *phr = E_OUTOFMEMORY;
        }

        //
        // NOTE!: If we've created our own output pin we must also create
        // the input pin ourselves because the CTransformFilter base class 
        // will create an extra output pin if the input pin wasn't created.        
        //
        CTransformInputPin *pIn = new CTransformInputPin(NAME("Transform input pin"),
	                                    this,              // Owner filter
                                        phr,               // Result code
                                        L"In");            // Pin name

        // a failed return code should delete the object
        if(pIn)
        {
            if(SUCCEEDED(*phr))
            {
                m_pInput = pIn;
            }
            else
            {
                delete pIn;
            }
        }
        else
        {
            *phr = E_OUTOFMEMORY;
        }
    }
}


// ------------------------------------------------------------------------
// destructor

PCM_DestFilter::~PCM_DestFilter()
{
}


CUnknown * WINAPI PCM_DestFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
    return new PCM_DestFilter(pUnk, phr);
}


//
// PCM_DestFilter::CheckTransform
//
// To be able to transform, the formats must be identical
//
HRESULT PCM_DestFilter::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
    HRESULT hr;

    if(FAILED(hr = CheckInputType(mtIn)))
    {
        return hr;
    }

    return NOERROR;

} // CheckTransform


// overridden because we need to know if Deliver() failed.

HRESULT PCM_DestFilter::Receive(IMediaSample *pSample)
{
    ULONG cbOld = m_cbWavData;
    HRESULT hr = CTransformFilter::Receive(pSample);

    // don't update the count if Deliver() downstream fails.
    if(hr != S_OK)
    {
        m_cbWavData = cbOld;
    }

    return hr;
}

//
// PCM_DestFilter::Transform
//
//
HRESULT PCM_DestFilter::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
    REFERENCE_TIME rtStart, rtEnd;

    // If there is a media type attached to the sample, reject the sample.
    AM_MEDIA_TYPE *pMediaType = NULL;
    if (S_OK == pIn->GetMediaType(&pMediaType))
    {
        DeleteMediaType(pMediaType);
        return VFW_E_INVALIDMEDIATYPE;
    }


    // First just copy the data to the output sample
    HRESULT hr = Copy(pIn, pOut);
    if(FAILED(hr))
    {
        return hr;
    }

    // Prepare it for writing    
    LONG lActual = pOut->GetActualDataLength();

    if(m_cbWavData + m_cbHeader + lActual < m_cbWavData + m_cbHeader)
    { 
        return E_FAIL;      // overflow
    }
			//CLSID_FileWriter (File Writer Filter)
			//"The file writer filter uses the input stream's time stamps
			//as file offsets and provides random access to the file. It 
			//supports IStream to allow reading and writing the file header
			//after the graph is stopped. To improve performance it also
			//supports unbuffered overlapped writes and handles the corre-
			//sponding buffer negotiation. "
    rtStart = m_cbWavData + m_cbHeader;
    rtEnd   = rtStart + lActual;
    m_cbWavData += lActual;
    EXECUTE_ASSERT(pOut->SetTime(&rtStart, &rtEnd) == S_OK);

			//flush file?
			EXECUTE_ASSERT(m_pOutput->BeginFlush());

    return S_OK;
}


//
// PCM_DestFilter::Copy
//
// Make destination an identical copy of source
//
HRESULT PCM_DestFilter::Copy(IMediaSample *pSource, IMediaSample *pDest) const
{
    CheckPointer(pSource,E_POINTER);
    CheckPointer(pDest,E_POINTER);

    // Copy the sample data

    BYTE *pSourceBuffer, *pDestBuffer;
    long lSourceSize = pSource->GetActualDataLength();

#ifdef DEBUG    
    long lDestSize = pDest->GetSize();
    ASSERT(lDestSize >= lSourceSize);
#endif

    pSource->GetPointer(&pSourceBuffer);
    pDest->GetPointer(&pDestBuffer);

    CopyMemory((PVOID) pDestBuffer,(PVOID) pSourceBuffer,lSourceSize);

    // Copy the sample times

    REFERENCE_TIME TimeStart, TimeEnd;
    if(NOERROR == pSource->GetTime(&TimeStart, &TimeEnd))
    {
        pDest->SetTime(&TimeStart, &TimeEnd);
    }

    LONGLONG MediaStart, MediaEnd;
    if(pSource->GetMediaTime(&MediaStart,&MediaEnd) == NOERROR)
    {
        pDest->SetMediaTime(&MediaStart,&MediaEnd);
    }

    // Copy the actual data length
    long lDataLength = pSource->GetActualDataLength();
    pDest->SetActualDataLength(lDataLength);

    return NOERROR;

} // Copy


//
// CheckInputType
//
HRESULT PCM_DestFilter::CheckInputType(const CMediaType* mtIn)
{
    if (m_cbWavData > 0)
    {
        // No new input types after we start getting data.
        return S_FALSE;
    }
    else if (mtIn->formattype == FORMAT_WaveFormatEx)
    {
			//SOM_SDK
			WAVEFORMATEX &f = *(WAVEFORMATEX*)mtIn->pbFormat;
			if(f.wFormatTag==WAVE_FORMAT_PCM)
			{
				//if(16==pmt->FormatLength())
				//(ULONG&)mtIn->cbFormat = 16; //BUG... usually 40, even w/ PCM
				return S_OK;
			}
    }
  
	return S_FALSE;
}

//
// GetMediaType
//
HRESULT PCM_DestFilter::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    ASSERT(iPosition == 0 || iPosition == 1);

    if(iPosition == 0)
    {
        CheckPointer(pMediaType,E_POINTER);

        pMediaType->SetType(&MEDIATYPE_Stream);
        pMediaType->SetSubtype(&MEDIASUBTYPE_WAVE);
        return S_OK;
    }

    return VFW_S_NO_MORE_ITEMS;
}

//
// DecideBufferSize
//
// Tell the output pin's allocator what size buffers we
// require. Can only do this when the input is connected
//
HRESULT PCM_DestFilter::DecideBufferSize(IMemAllocator *pAlloc,
                                         ALLOCATOR_PROPERTIES *pProperties)
{
    HRESULT hr = NOERROR;

    // Is the input pin connected
    if(m_pInput->IsConnected() == FALSE)
    {
        return E_UNEXPECTED;
    }

    CheckPointer(pAlloc,E_POINTER);
    CheckPointer(pProperties,E_POINTER);

    pProperties->cBuffers = 1;
    pProperties->cbAlign  = 1;

    // Get input pin's allocator size and use that
    ALLOCATOR_PROPERTIES InProps;
    IMemAllocator * pInAlloc = NULL;

    hr = m_pInput->GetAllocator(&pInAlloc);
    if(SUCCEEDED(hr))
    {
        hr = pInAlloc->GetProperties(&InProps);
        if(SUCCEEDED(hr))
        {
            pProperties->cbBuffer = InProps.cbBuffer;
        }
        pInAlloc->Release();
    }

    if(FAILED(hr))
        return hr;

    ASSERT(pProperties->cbBuffer);

    // Ask the allocator to reserve us some sample memory, NOTE the function
    // can succeed (that is return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);
    if(FAILED(hr))
    {
        return hr;
    }

    ASSERT(Actual.cBuffers == 1);

    if(pProperties->cBuffers > Actual.cBuffers ||
        pProperties->cbBuffer > Actual.cbBuffer)
    {
        return E_FAIL;
    }

    return NOERROR;

} // DecideBufferSize

//
// StartStreaming
//
// Compute the header size to allow space for us to write it at the end.
//
// 00000000    RIFF (00568BFE) 'WAVE'
// 0000000C        fmt  (00000010)
// 00000024        data (00568700)
// 0056872C
//
HRESULT PCM_DestFilter::StartStreaming()
{
			CMediaType &pcm = m_pInput->CurrentMediaType();
			if(WAVE_FORMAT_PCM!=*pcm.Format())
			return S_FALSE;

    // leave space for the header
    m_cbHeader = sizeof(RIFFLIST) + 
                 sizeof(RIFFCHUNK) + 
			16 + //pcm.FormatLength() + //BUG
                 sizeof(RIFFCHUNK);

    m_cbWavData = 0;
	
			//lay down incomplete header so file can play in 
			//an incomplete state
			WriteHeader();

	return S_OK;
}


//
// StopStreaming
//
// Write out the header
//
HRESULT PCM_DestFilter::StopStreaming()
{	    
    if(m_pOutput->IsConnected() == FALSE)
        return E_FAIL;

	return WriteHeader();
}
HRESULT PCM_DestFilter::WriteHeader()const
{		
    IPin * pDwnstrmInputPin = m_pOutput->GetConnected();

    if(!pDwnstrmInputPin)
        return E_FAIL;

	IStream *pStream; HRESULT hr = 
	((IMemInputPin*)pDwnstrmInputPin)->QueryInterface(IID_IStream,(void**)&pStream);
    if(SUCCEEDED(hr))
    {
        BYTE *pb = (BYTE *)_alloca(m_cbHeader);

				CMediaType &pcm = m_pInput->CurrentMediaType();
				if(WAVE_FORMAT_PCM!=*pcm.Format())
				return E_FAIL;

        RIFFLIST  *pRiffWave = (RIFFLIST *)pb;
        RIFFCHUNK *pRiffFmt  = (RIFFCHUNK *)(pRiffWave + 1);
        RIFFCHUNK *pRiffData = (RIFFCHUNK *)(((BYTE *)(pRiffFmt + 1)) + 
				16); //m_pInput->CurrentMediaType().FormatLength()); //BUG

        pRiffData->fcc = FCC('data');
        pRiffData->cb = m_cbWavData;

        pRiffFmt->fcc = FCC('fmt ');
        pRiffFmt->cb = //m_pInput->CurrentMediaType().FormatLength(); //BUG
				16;
        CopyMemory(pRiffFmt + 1, pcm.Format(), pRiffFmt->cb);

        pRiffWave->fcc = FCC('RIFF');
        pRiffWave->cb = m_cbWavData + m_cbHeader - sizeof(RIFFCHUNK);
        pRiffWave->fccListType = FCC('WAVE');

        LARGE_INTEGER li;
        ZeroMemory(&li, sizeof(li));

					union
					{
						ULARGE_INTEGER repair;
						LARGE_INTEGER seriously; //???
					};
					pStream->Seek(li,STREAM_SEEK_CUR,&repair);
        hr = pStream->Seek(li, STREAM_SEEK_SET, 0);
        if(SUCCEEDED(hr))
        {
            hr = pStream->Write(pb, m_cbHeader, 0);
			
					pStream->Commit(0); //flush?
					pStream->Seek(seriously,STREAM_SEEK_SET,0);
        }
        pStream->Release();
    }

	return hr;
}

//
// PCM_DestOutputPin::PCM_DestOutputPin 
//
PCM_DestOutputPin::PCM_DestOutputPin(CTransformFilter *pFilter, HRESULT * phr) :
        CTransformOutputPin(NAME("WavDest output pin"), pFilter, phr, L"Out")
{
    // Empty
}


//
// PCM_DestOutputPin::EnumMediaTypes
//
STDMETHODIMP PCM_DestOutputPin::EnumMediaTypes( IEnumMediaTypes **ppEnum )
{
    return CBaseOutputPin::EnumMediaTypes(ppEnum);
}

//
// PCM_DestOutputPin::CheckMediaType
//
// Make sure it's our default type
//
HRESULT PCM_DestOutputPin::CheckMediaType(const CMediaType* pmt)
{
    CheckPointer(pmt,E_POINTER);

    if(pmt->majortype == MEDIATYPE_Stream && pmt->subtype == MEDIASUBTYPE_WAVE)
        return S_OK;
    else
        return S_FALSE;
}


////////////////////////////////////////////////////////////////////////
//
// Exported entry points for registration and unregistration 
// (in this case they only call through to default implementations).
//
////////////////////////////////////////////////////////////////////////

		#ifdef _USRDLL //SOM_SDK

STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2(FALSE);
}

//
// DllEntryPoint
//
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, 
                      DWORD  dwReason, 
                      LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}

		#endif //_USRDLL //SOM_SDK


