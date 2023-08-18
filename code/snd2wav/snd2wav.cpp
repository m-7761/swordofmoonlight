				  
#include <assert.h>
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <math.h> //fabs

//#include <WinDef.h> //UINT
//#include <MMsystem.h> //WAVEFORMATEX
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef struct tWAVEFORMATEX
{
    WORD        wFormatTag;         /* format type */
    WORD        nChannels;          /* number of channels (i.e. mono, stereo...) */
    DWORD       nSamplesPerSec;     /* sample rate */
    DWORD       nAvgBytesPerSec;    /* for buffer estimation */
    WORD        nBlockAlign;        /* block size of data */
    WORD        wBitsPerSample;     /* number of bits per sample of mono data */
    WORD        cbSize;             /* the count in bytes of the size of */
                                    /* extra information (after cbSize) */
} WAVEFORMATEX, *PWAVEFORMATEX, *NPWAVEFORMATEX, *LPWAVEFORMATEX;

int wmain(int argc, const wchar_t* argv[])
{	
	for(int i=1;i<argc;i++)
	{
		const wchar_t *input = argv[i];

		FILE *in = _wfopen(input,L"rb"); if(!in) continue;

		WAVEFORMATEX snd;

		fseek(in,0,SEEK_END);
		auto sz = ftell(in);
		fseek(in,0,SEEK_SET);
		fread(&snd,sizeof(snd),1,in);
		sz-=sizeof(WAVEFORMATEX);
		char *buf = new char[44+sz];
		fread(buf,sz,1,in);
		fclose(in);

		DWORD hd[11] = //44
		{
		0x46464952, //RIFF
		0x00000C45,
		0x45564157,	//WAVE
		0x20746D66, //fmt
		0x00000010,
		0x00010001,
		0x00002B11,
		0x00005622,
		0x00100002,
		0x61746164, //data
		0x0000E844,	
		};

		hd[1] = 44+sz-4;

		hd[4] = 16; //computed chunk size?

		memcpy(hd+5,&snd,hd[4]);

		hd[10] = sz-4;

		auto *ext = wcsrchr(input,'.');

		if(*ext) *const_cast<wchar_t*>(ext) = '\0';

		wchar_t output[260]; //MAX_PATH

		swprintf(output,L"%s.wav",input);

		FILE *out = _wfopen(output,L"wb");

		fwrite(hd,44,1,out);
		fwrite(buf,sz,1,out);
		fclose(out);
	}
	return 0;
}

