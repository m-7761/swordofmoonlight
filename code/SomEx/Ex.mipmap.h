
#ifndef EX_MIPMAP_INCLUDED
#define EX_MIPMAP_INCLUDED

enum _D3DFORMAT; //D3D9

typedef enum _D3DFORMAT D3DFORMAT;

namespace DDRAW
{
	class IDirectDrawSurface7; 
	class IDirectDrawPalette; //2022: som.state.h?
	class IDirect3DVertexBuffer7;
}

namespace DX
{
	typedef struct _DDSURFACEDESC2 DDSURFACEDESC2; 
}

namespace EX
{
	extern void *mipmap(const DX::DDSURFACEDESC2*, DX::DDSURFACEDESC2*);

	extern int colorkey(DX::DDSURFACEDESC2*,D3DFORMAT);
}

#endif //EX_MIPMAP_INCLUDED