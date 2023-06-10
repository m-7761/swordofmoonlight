				  
#ifndef EX_FONTS_INCLUDED
#define EX_FONTS_INCLUDED

class ID3DXFont;
class ID3DXSprite;

namespace EX{

class Font
{		
public: 

	mutable int refs;

	const LOGFONTW log;
		
	float space; //width of u+0020
	float u3000; //ideographic space

	//OBSOLETE?
	//why is this the maximum again???
	//static const int MAX_SELECT = 128;

	//select: selects a glyphset for 
	//up to size_t characters and stores
	//number of selected characters in
	//that glyphset's selected member.
	//negative for right justification
	//
	//midline: this has to be false to 
	//select more than 1 newline, which
	//is needed to manage hybrid fonts
	int select(const wchar_t*,int=0,bool midline=0)const;
	
	struct glyphset_t
	{
		const ULONG *refs;

		//null terminated
		//Unicode range pairs
		const wchar_t *selection;	

		//nselected counts \n characters
		mutable int selected, nselected;

		inline operator bool()const
		{ 
			return selection?true:false; 
		}

		ID3DXFont **d3dxfont;
		ID3DXSprite **d3dxsprite;
		int *glfont; //Exselector

		float scale; //vertical

		const LOGFONTW *lfont;

		HFONT hfont;

	private: //optimizations

	friend class EX::Font;

		mutable DWORD hits;

		mutable int index; 

	}glyphs[1];

  ////used internally///////////

	struct grapheme_t; //Ex.fonts.cpp

	//REMOVE ME?
	static const wchar_t *u_0001_ffff; //L"\1\uffff";

private: //yuck

	~Font() //REMOVE ME?
	{
		for(int i=0;glyphs[i].selection||i==0;i++)
		{
			if(glyphs[i].selection)
			if(glyphs[i].selection!=EX::Font::u_0001_ffff)
			delete[] glyphs[i].selection;

			if(glyphs[i].refs)
			{
				*(ULONG*)glyphs[i].refs--; assert(*glyphs[i].refs>=0);
			}

			if(!glyphs[i].selection) break;
		}
	}

	//REMOVE ME?
	Font(const LOGFONTW &cp):log(cp){ /*that's it*/ }

public: 

	//REMOVE ME?
	static EX::Font *construct(const LOGFONTW &cp, int n)
	{			
		size_t sz = sizeof(Font)+sizeof(glyphset_t)*(n); //-1)+sizeof(void*);
		
		//if(sz!=size_t(&((EX::Font*)0)->glyphs[n].refs+1)){ assert(0); return 0; } 		

		return new (memset(new BYTE[sz],0x00,sz)) EX::Font(cp); 
	}
	void destruct()const 
	{
		if(this) this->~Font(); delete[] ((BYTE*)this);
	}
};

extern const wchar_t *describing_font(const LOGFONTW&, const wchar_t*,...);

extern const EX::Font *creating_font(const LOGFONTW&, const wchar_t *description=0);

} //namespace EX

#endif //EX_FONTS_INCLUDED
