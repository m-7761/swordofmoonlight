
#include "Ex.h" 
EX_TRANSLATION_UNIT //(C)

#include "SomEx.ini.h"
#include "som.state.h"

extern std::vector<SOM::MDL*> som_kage(64);

SOM::Animation *SOM::MDL::find_first_kage()
{
	if((unsigned)mdl_data->ext.kage+1u<=4096)
	return nullptr;
	auto &k = *mdl_data->ext.kage2;
	auto a = 0u, sz = k.size();
	for(int i=c;i-->0;a++)
	{
		assert(a==sz-1||k[a].t<=1);
		while(a<sz-1&&k[a+1].t>1) a++;
	}
	while(a>=sz-1)
	{
		if(a) a--; else return 0;
	}
	while(a<sz-1&&k[a+1].t<f)
	a++;	
	if(a==sz-1||k[a+1].t<=1) 
	a--;

	k[a].upload();
	k[a+1].upload(); return &k[a]; 
}

struct ico_dentry_t
{
	//NOTE: can't exceed 255
	//or will be 0 or less
	BYTE w,h,palette,_; //reserved

	WORD planes,bpp;

	int sz,os;
};
extern SOM::Kage *som_kage_ico(wchar_t *w, char *a)
{
	assert(EX::INI::Option()->do_shadow);

	int i = 0;

	auto &tc = SOM::L.textures_counter;

	SOM::Kage *v = nullptr;

	if(tc<1024) if(FILE*f=_wfopen(w,L"rb"))
	{
		fseek(f,0,SEEK_END);
		int sz = ftell(f);
		fseek(f,0,SEEK_SET);
		WORD *buf = new WORD[sz/2];
		int res = fread(buf,sz,1,f);
		fclose(f);

		int n = buf[2]; //0/1/n

		//TODO? consolidate memory
		if(n>1) v = new SOM::Kage;
		if(v) v->reserve(n*4);

		auto *p = (ico_dentry_t*)(buf+3);
		for(;n-->0;p++)
		{
			if(p->os+p->sz>sz) break;

			auto *bi = (BITMAPINFO*)((BYTE*)buf+p->os);
			BYTE *bp = (BYTE*)bi->bmiColors;
			int isz = p->sz-sizeof(bi->bmiHeader);

			bi->bmiHeader.biHeight/=2; //remove mask?

			assert(p->w==bi->bmiHeader.biWidth||!p->w); //256?
			assert(p->h==bi->bmiHeader.biHeight||!p->h); //256?
			assert(p->bpp==bi->bmiHeader.biBitCount);

			isz = bi->bmiHeader.biSizeImage = //p->w*p->h*p->bpp/8;
			bi->bmiHeader.biWidth*bi->bmiHeader.biHeight*p->bpp/8;
						
			void *cp;
			HBITMAP h = CreateDIBSection(0,bi,0,&cp,0,0);
			assert(h);
			if(!h) continue;
				
			if(v) //2024 (June)
			{
				//this is 4 images stored in rgba data

				v->resize(v->size()+4);
				auto *a = &v->back()-3;

				for(int j=4;j-->0;)
				for(int i=7;i-->0;)
				{
					RGBQUAD &px = bi->bmiColors[7*j+i];
					(i?a[j].bbox[i-1]:a[j].t) = (float&)px; 
					memset(&px,0xffffffff,sizeof(px));
				}

				int w = bi->bmiHeader.biWidth;
				int h = bi->bmiHeader.biHeight; //remove mask

				//HACK: removing excess bytes
				float xm = 0, zm = 0;
				for(int j=0;j<4;j++)
				{
					xm = max(xm,a[j].bbox[1]);
					zm = max(zm,a[j].bbox[5]);

					if(a[j].t>1) a[j].t =
					1+(a[j].t-1)*SOM::MDL::fps;
				}				
				for(int j=0;j<4;j++)
				{
					auto &aj = a[j];
					aj.w = aj.bbox[1]/xm*(w-8)+8;
					aj.h = aj.bbox[5]/zm*(h-8)+8;
					assert(aj.w<=w&&aj.h<=h);
					aj.data_s = aj.w*aj.h;
					a[j].data = aj.data_s?new BYTE[aj.data_s]:nullptr;
				}
				for(int y=h;y-->0;)				
				for(int x=w;x-->0;)
				{
					RGBQUAD &px = bi->bmiColors[y*w+x];

					for(int j=0;j<4;j++) if(a[j].w>x&&a[j].h>y)
					{
						a[j].data[(a[j].h-1-y)*a[j].w+x] = 255-(&px.rgbBlue)[j];
					}
				}
			}
			else //old way (reference)
			{
				float kage_bbox[6];
				for(int i=6;i-->0;)
				{
					RGBQUAD &px = bi->bmiColors[i];
					kage_bbox[i] = (float&)px; 
					memset(&px,0xffffffff,sizeof(px));
				}

				if(1)
				{
					//int _ = SetDIBits(0,h,0,bi->bmiHeader.biHeight,bp,bi,0);
					memcpy(cp,bi->bmiColors,isz);	
					if(0<(i=((int(__cdecl*)(char*,DWORD,DWORD,DWORD,HGDIOBJ))0x448660)(a,0,0,0,h)))
					{
						auto &t = SOM::L.textures[i];
						memcpy(t.kage_bbox,kage_bbox,sizeof(kage_bbox));					
					}
					else{ i = 0; assert(0); DeleteObject(h); break; }
				}
				/*else //I think this didn't work out?
				{
					//TODO: take current texture first
					auto &tc = SOM::L.textures_counter;
				
					i = SOM::L.textures[tc].ref_counter?0:tc; //optimizing
					
					for(;i<1024;i++) if(!SOM::L.textures[i].ref_counter)
					{
						//int _ = SetDIBits(0,h,0,bi->bmiHeader.biHeight,bp,bi,0);
						memcpy(cp,bi->bmiColors,isz);				

						auto &t = SOM::L.textures[i];
						extern BYTE __cdecl som_game_449530(SOM::Texture*,char*,DWORD*,DWORD,HGDIOBJ*,DWORD,HGDIOBJ*);
						if(som_game_449530(&t,a,0,0,0,1,(HGDIOBJ*)&h))
						{
							cp = 0; //!

							tc++; t.ref_counter = 1;

						//	t.mipmap_counter = 1; //TESTING

						//	t.mode = 1; //TESTING

							memcpy(t.kage_bbox,kage_bbox,sizeof(kage_bbox));						
						}
						break; //!
					}
				}*/				
			}
		}

		delete[] buf;
	}

	if(v&&!v->empty()) return v; delete v;

	return (SOM::Kage*)(i<1024?i:0); //-1 //deprecated
}
