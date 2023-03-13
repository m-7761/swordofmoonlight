
DWORD_(dwSize)                 // size of the DDSURFACEDESC structure
DWORD_(dwFlags)                // determines what fields are valid
DWORD_AND(DDSD_CAPS)
DWORD_AND(DDSD_HEIGHT)
DWORD_AND(DDSD_WIDTH)
DWORD_AND(DDSD_PITCH)
DWORD_AND(DDSD_BACKBUFFERCOUNT)
DWORD_AND(DDSD_ZBUFFERBITDEPTH)
DWORD_AND(DDSD_ALPHABITDEPTH)
DWORD_AND(DDSD_LPSURFACE)
DWORD_AND(DDSD_PIXELFORMAT)
DWORD_AND(DDSD_CKDESTOVERLAY) 
DWORD_AND(DDSD_CKDESTBLT)
DWORD_AND(DDSD_CKSRCOVERLAY)
DWORD_AND(DDSD_CKSRCBLT)
DWORD_AND(DDSD_MIPMAPCOUNT)
DWORD_AND(DDSD_REFRESHRATE)
DWORD_AND(DDSD_LINEARSIZE)
DWORD_AND(DDSD_TEXTURESTAGE)
DWORD_AND(DDSD_FVF)
DWORD_AND(DDSD_SRCVBHANDLE)
DWORD_AND(DDSD_DEPTH)

DWORD_(dwHeight)               // height of surface to be created
DWORD_(dwWidth)                // width of input surface
//union{
    LONG_(lPitch)                 // distance to start of next line (return value only)
//    DWORD           dwLinearSize)           // Formless late-allocated optimized surface size
//} DUMMYUNIONNAMEN(1))
//union
//{
    DWORD_(dwBackBufferCount)      // number of back buffers requested
    DWORD_(dwDepth)                // the depth if this is a volume texture 
//} DUMMYUNIONNAMEN(5))
//union
//{
    DWORD_(dwMipMapCount)          // number of mip-map levels requestde
	                                         // dwZBufferBitDepth removed, use ddpfPixelFormat one instead
    DWORD_(dwRefreshRate)          // refresh rate (used when display mode is described)
//    DWORD           dwSrcVBHandle)          // The source used in VB::Optimize
//} DUMMYUNIONNAMEN(2))
DWORD_(               dwAlphaBitDepth)        // depth of alpha buffer requested
//DWORD               dwReserved)             // reserved
//LPVOID              lpSurface)              // pointer to the associated surface memory
//union
//{
DWORD_(ddckCKDestOverlay.dwColorSpaceLowValue)      // color key for destination overlay use
DWORD_(ddckCKDestOverlay.dwColorSpaceHighValue)
//    DWORD           dwEmptyFaceColor)       // Physical color for empty cubemap faces
//} DUMMYUNIONNAMEN(3))
DWORD_(ddckCKDestBlt.dwColorSpaceLowValue)          // color key for destination blt use
DWORD_(ddckCKDestBlt.dwColorSpaceHighValue)
DWORD_(ddckCKSrcOverlay.dwColorSpaceLowValue)       // color key for source overlay use
DWORD_(ddckCKSrcOverlay.dwColorSpaceHighValue)
DWORD_(ddckCKSrcBlt.dwColorSpaceLowValue)           // color key for source blt use
DWORD_(ddckCKSrcBlt.dwColorSpaceHighValue)
//union
//{
    //DDPIXELFORMAT   ddpfPixelFormat)        // pixel format description of the surface

    DWORD_(ddpfPixelFormat.dwFlags)                // pixel format flags
	DWORD_AND(DDPF_ALPHAPIXELS)
	DWORD_AND(DDPF_ALPHA)
	DWORD_AND(DDPF_FOURCC)
	DWORD_AND(DDPF_PALETTEINDEXED4)
	DWORD_AND(DDPF_PALETTEINDEXEDTO8)
	DWORD_AND(DDPF_PALETTEINDEXED8)
	DWORD_AND(DDPF_RGB)
	DWORD_AND(DDPF_COMPRESSED)
	DWORD_AND(DDPF_RGBTOYUV)
	DWORD_AND(DDPF_YUV)
	DWORD_AND(DDPF_ZBUFFER)
	DWORD_AND(DDPF_PALETTEINDEXED1)
	DWORD_AND(DDPF_PALETTEINDEXED2)
	DWORD_AND(DDPF_ZPIXELS)
	DWORD_AND(DDPF_STENCILBUFFER)
	DWORD_AND(DDPF_ALPHAPREMULT)
	DWORD_AND(DDPF_LUMINANCE)
	DWORD_AND(DDPF_BUMPLUMINANCE)
	DWORD_AND(DDPF_BUMPDUDV)
    DWORD_(ddpfPixelFormat.dwFourCC)               // (FOURCC code)
//    union
//    {
        DWORD_(ddpfPixelFormat.dwRGBBitCount)          // how many bits per pixel
//        DWORD   dwYUVBitCount)          // how many bits per pixel
//        DWORD   dwZBufferBitDepth)      // how many total bits/pixel in z buffer (including any stencil bits)
//        DWORD   dwAlphaBitDepth)        // how many bits for alpha channels
//        DWORD   dwLuminanceBitCount)    // how many bits per pixel
//        DWORD   dwBumpBitCount)         // how many bits per "buxel", total
//        DWORD   dwPrivateFormatBitCount)// Bits per pixel of private driver formats. Only valid in texture
                                        // format list and if DDPF_D3DFORMAT is set
//    } DUMMYUNIONNAMEN(1))
//    union
//    {
        DWORD_(ddpfPixelFormat.dwRBitMask)             // mask for red bit
//        DWORD   dwYBitMask)             // mask for Y bits
//        DWORD   dwStencilBitDepth)      // how many stencil bits (note: dwZBufferBitDepth-dwStencilBitDepth is total Z-only bits)
//        DWORD   dwLuminanceBitMask)     // mask for luminance bits
//        DWORD   dwBumpDuBitMask)        // mask for bump map U delta bits
//        DWORD   dwOperations)           // DDPF_D3DFORMAT Operations
//    } DUMMYUNIONNAMEN(2))
//    union
//    {
        DWORD_(ddpfPixelFormat.dwGBitMask)             // mask for green bits
//        DWORD   dwUBitMask)             // mask for U bits
//        DWORD   dwZBitMask)             // mask for Z bits
//        DWORD   dwBumpDvBitMask)        // mask for bump map V delta bits
//        struct
//        {
//            WORD    wFlipMSTypes)       // Multisample methods supported via flip for this D3DFORMAT
//            WORD    wBltMSTypes)        // Multisample methods supported via blt for this D3DFORMAT
//        } MultiSampleCaps)

//    } DUMMYUNIONNAMEN(3))
//    union
//    {
        DWORD_(ddpfPixelFormat.dwBBitMask)             // mask for blue bits
//        DWORD   dwVBitMask)             // mask for V bits
//        DWORD   dwStencilBitMask)       // mask for stencil bits
//        DWORD   dwBumpLuminanceBitMask) // mask for luminance in bump map
//    } DUMMYUNIONNAMEN(4))
//    union
//    {
        DWORD_(ddpfPixelFormat.dwRGBAlphaBitMask)      // mask for alpha channel
//        DWORD   dwYUVAlphaBitMask)      // mask for alpha channel
//        DWORD   dwLuminanceAlphaBitMask)// mask for alpha channel
//        DWORD   dwRGBZBitMask)          // mask for Z channel
//        DWORD   dwYUVZBitMask)          // mask for Z channel
//    } DUMMYUNIONNAMEN(5))

//    DWORD           dwFVF)                  // vertex format description of vertex buffers
//} DUMMYUNIONNAMEN(4))
DWORD_(ddsCaps.dwCaps)                // direct draw surface capabilities
DWORD_(ddsCaps.dwCaps2)
DWORD_(ddsCaps.dwCaps3)
DWORD_(ddsCaps.dwCaps4)
DWORD_(dwTextureStage)         // stage in multitexture cascade