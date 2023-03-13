													   
#include "Sompaste.pch.h" 

extern HICON Somicon()
{		
	static HICON icon = //hack
	LoadIcon(Sompaste_dll(),MAKEINTRESOURCE(IDI_MOONLIGHT));
	return icon;
}