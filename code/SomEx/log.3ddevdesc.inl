    

#if(D3DDEVICEDESC_VERSION==6)

DWORD_(dwFlags)		   
DWORD_(dcmColorModel)

#endif

DWORD_(dwDevCaps)              /* Capabilities of device */
	  
#if(D3DDEVICEDESC_VERSION==6)

DWORD_(dtcTransformCaps.dwCaps)

DWORD_(dlcLightingCaps.dwCaps)
DWORD_(dlcLightingCaps.dwLightingModel)
DWORD_(dlcLightingCaps.dwNumLights)

#endif

DWORD_(dpcLineCaps.dwSize)
DWORD_(dpcLineCaps.dwMiscCaps)                 /* Capability flags */
DWORD_(dpcLineCaps.dwRasterCaps)
DWORD_(dpcLineCaps.dwZCmpCaps)
DWORD_(dpcLineCaps.dwSrcBlendCaps)
DWORD_(dpcLineCaps.dwDestBlendCaps)
DWORD_(dpcLineCaps.dwAlphaCmpCaps)
DWORD_(dpcLineCaps.dwShadeCaps)
DWORD_(dpcLineCaps.dwTextureCaps)
DWORD_(dpcLineCaps.dwTextureFilterCaps)
DWORD_(dpcLineCaps.dwTextureBlendCaps)
DWORD_(dpcLineCaps.dwTextureAddressCaps)
DWORD_(dpcLineCaps.dwStippleWidth)             /* maximum width and height of */
DWORD_(dpcLineCaps.dwStippleHeight)            /* of supported stipple (up to 32x32) */

DWORD_(dpcTriCaps.dwSize)
DWORD_(dpcTriCaps.dwMiscCaps)                 /* Capability flags */
DWORD_(dpcTriCaps.dwRasterCaps)
DWORD_(dpcTriCaps.dwZCmpCaps)
DWORD_(dpcTriCaps.dwSrcBlendCaps)
DWORD_(dpcTriCaps.dwDestBlendCaps)
DWORD_(dpcTriCaps.dwAlphaCmpCaps)
DWORD_(dpcTriCaps.dwShadeCaps)
DWORD_(dpcTriCaps.dwTextureCaps)
DWORD_(dpcTriCaps.dwTextureFilterCaps)
DWORD_(dpcTriCaps.dwTextureBlendCaps)
DWORD_(dpcTriCaps.dwTextureAddressCaps)
DWORD_(dpcTriCaps.dwStippleWidth)             /* maximum width and height of */
DWORD_(dpcTriCaps.dwStippleHeight)            /* of supported stipple (up to 32x32) */

DWORD_(dwDeviceRenderBitDepth) /* One of DDBB_8, 16, etc. */
DWORD_(dwDeviceZBufferBitDepth)/* One of DDBD_16, 32, etc. */

#if(D3DDEVICEDESC_VERSION==6)

DWORD_(dwMaxBufferSize)
DWORD_(dwMaxVertexCount)

#endif

DWORD_(dwMinTextureWidth)
DWORD_(dwMinTextureHeight)
DWORD_(dwMaxTextureWidth)
DWORD_(dwMaxTextureHeight)

#if(D3DDEVICEDESC_VERSION==6)

DWORD_(dwMinStippleWidth)
DWORD_(dwMaxStippleWidth)
DWORD_(dwMinStippleHeight)
DWORD_(dwMaxStippleHeight)

#endif

DWORD_(dwMaxTextureRepeat)
DWORD_(dwMaxTextureAspectRatio)
DWORD_(dwMaxAnisotropy)

D3DVALUE_(dvGuardBandLeft)
D3DVALUE_(dvGuardBandTop)
D3DVALUE_(dvGuardBandRight)
D3DVALUE_(dvGuardBandBottom)

D3DVALUE_(dvExtentsAdjust)
DWORD_(dwStencilCaps)

DWORD_(dwFVFCaps)
DWORD_(dwTextureOpCaps)
WORD_(wMaxTextureBlendStages)
WORD_(wMaxSimultaneousTextures)

#if(D3DDEVICEDESC_VERSION==7)

DWORD_(dwMaxActiveLights)
D3DVALUE_(dvMaxVertexW)
GUID_(deviceGUID)

WORD_(wMaxUserClipPlanes)
WORD_(wMaxVertexBlendMatrices)

DWORD_(dwVertexProcessingCaps)

#endif