
DWORD_(dwDDFX)
DWORD_(dwROP)                          // Win32 raster operations
DWORD_(dwDDROP)                        // Raster operations new for DirectDraw
DWORD_(dwRotationAngle)                // Rotation angle for blt
DWORD_(dwZBufferOpCode)                // ZBuffer compares
DWORD_(dwZBufferLow)                   // Low limit of Z buffer
DWORD_(dwZBufferHigh)                  // High limit of Z buffer
DWORD_(dwZBufferBaseDest)              // Destination base value
DWORD_(dwZDestConstBitDepth)           // Bit depth used to specify Z constant for destination
//	union
//    {
DWORD_(dwZDestConst)					// Constant to use as Z buffer for dest
//LPDIRECTDRAWSURFACE lpDDSZBufferDest)   // Surface to use as Z buffer for dest
//	  } 
DWORD_(dwZSrcConstBitDepth)            // Bit depth used to specify Z constant for source
//    union
//    {
DWORD_(dwZSrcConst)                    // Constant to use as Z buffer for src
//		LPDIRECTDRAWSURFACE lpDDSZBufferSrc)    // Surface to use as Z buffer for src
//    } 
DWORD_(dwAlphaEdgeBlendBitDepth)       // Bit depth used to specify constant for alpha edge blend
DWORD_(dwAlphaEdgeBlend)               // Alpha for edge blending
DWORD_(dwAlphaDestConstBitDepth)       // Bit depth used to specify alpha constant for destination
//    union
//    {
DWORD_(dwAlphaDestConst)               // Constant to use as Alpha Channel
//        LPDIRECTDRAWSURFACE lpDDSAlphaDest)     // Surface to use as Alpha Channel
//    } 
DWORD_(dwAlphaSrcConstBitDepth)        // Bit depth used to specify alpha constant for source
//    union
//    {
DWORD_(dwAlphaSrcConst)                // Constant to use as Alpha Channel
//        LPDIRECTDRAWSURFACE lpDDSAlphaSrc)      // Surface to use as Alpha Channel
//    } 
//    union
//    {
DWORD_(dwFillColor)                    // color in RGB or Palettized
//        DWORD   dwFillDepth)                    // depth value for z-buffer
//        DWORD   dwFillPixel)                    // pixel value for RGBA or RGBZ
//        LPDIRECTDRAWSURFACE lpDDSPattern)       // Surface to use as pattern
//    } 
DWORD_(ddckDestColorkey.dwColorSpaceLowValue)               // DestColorkey override
DWORD_(ddckDestColorkey.dwColorSpaceHighValue)
DWORD_(ddckSrcColorkey.dwColorSpaceLowValue)
DWORD_(ddckSrcColorkey.dwColorSpaceHighValue)