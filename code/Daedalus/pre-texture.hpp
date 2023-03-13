
//struct PreTexture //PreServe.h
//{	
	PRESERVE_LIST(PreTexture*)
	PRESERVE_LISTS(preID,MetaData,metadata)

	DAEDALUS_DEPRECATED("porting Assimp? Try adding ,0")
	PreTexture(const PreTexture &cp); //not implementing
	PreTexture(const PreTexture &cp, PreX::Pool *p):PRESERVE_VERSION
	{
		PRESERVE_COPYTHISAFTER(_size)	
		PRESERVE_COPYLIST(metadata)
		PRESERVE_COPYLIST(colordata) PRESERVE_COPYLIST(colors)		
	}
	PreTexture():PRESERVE_VERSION{ PRESERVE_ZEROTHISAFTER(_size) }
	~PreTexture()
	{
		MetaDataList().Clear();
		delete[] colordatalist; delete[] colorslist; 		
	}    
	
	struct Color
	{
		unsigned char b,g,r,a;

		inline bool operator==(const Color &cmp)const
		{
			return b==cmp.b&&r==cmp.r&&g==cmp.g&&a==cmp.a;
		}
		inline bool operator!=(const Color& cmp)const
		{
			return b!=cmp.b||r!=cmp.r||g!=cmp.g||a!=cmp.a;
		}
		inline operator pre4D()const
		{
			const double _255 = (double)1/255;
			return pre4D(r*_255,g*_255,b*_255,a*_255);
		}
	};

	inline bool CheckFormat(const char *s)const
	{
		return (0==strncmp(assimp3charcode_if0height,s,3));
	}	

	//NEW: not sure if really necessary
	PRESERVE_LISTS(Color,Colors,colors)
	PRESERVE_LISTS(unsigned char,ColorData,colordata)
//};