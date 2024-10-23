
	/*/////////////////////////////

	This interface code modifies the "WavDest" DirectShow sample
	to negotiate for an uncompressed, plain PCM media type/input.
	It's intended for private use. If the output pin is unfilled
	it has a nonstanard set of methods for pulling directly from
	the input pin.

	/*/////////////////////////////

#pragma warning(disable: 4097 4511 4512 4514 4705)

class PCM_DestOutputPin : public CTransformOutputPin
{
public:
    PCM_DestOutputPin(CTransformFilter *pFilter, HRESULT * phr);

    STDMETHODIMP EnumMediaTypes( IEnumMediaTypes **ppEnum );
    HRESULT CheckMediaType(const CMediaType* pmt);
};

class PCM_DestFilter : public CTransformFilter
{

public:

    DECLARE_IUNKNOWN;
  
    PCM_DestFilter(LPUNKNOWN pUnk, HRESULT *pHr);
    ~PCM_DestFilter();
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *pHr);

    HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);
    HRESULT Receive(IMediaSample *pSample);

    HRESULT CheckInputType(const CMediaType* mtIn) ;
    HRESULT CheckTransform(const CMediaType *mtIn,const CMediaType *mtOut);
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType) ;

    HRESULT DecideBufferSize(IMemAllocator *pAlloc,
                             ALLOCATOR_PROPERTIES *pProperties);

    HRESULT StartStreaming();
    HRESULT StopStreaming();

    HRESULT CompleteConnect(PIN_DIRECTION direction,IPin *pReceivePin)
	{
		return S_OK; 
	}

private:

    HRESULT Copy(IMediaSample *pSource, IMediaSample *pDest) const;
    HRESULT Transform(IMediaSample *pMediaSample);
    HRESULT Transform(AM_MEDIA_TYPE *pType, const signed char ContrastLevel) const;

    ULONG m_cbWavData;
    ULONG m_cbHeader;

private: //SOM_SDK

	HRESULT WriteHeader()const;
};
