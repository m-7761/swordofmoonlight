
#ifndef SOM_ATLENC_INCLUDED
#define SOM_ATLENC_INCLUDED

//REMOVE ME?
//TODO: IMPLEMENT BASE64 BY WAY OF lib/swordofmoonlight
//Copyright: this file is based on Microsoft's atlenc.h
//=====================================================
// Base64Encode/Base64Decode compliant with RFC 2045
//=====================================================

#ifdef _WIN32 //REMOVE ME?
inline int som_atlenc_Base64EncodeGetRequiredLength(int nSrcLen)
{
	__int64 nSrcLen4 = static_cast<__int64>(nSrcLen)*4; assert(nSrcLen4<=INT_MAX);

	int nRet = static_cast<int>(nSrcLen4/3);

	//if(~dwFlags&ATL_BASE64_FLAG_NOPAD) nRet+=nSrcLen%3;

	int nCRLFs = nRet/76+1, nOnLastLine = nRet%76;

	if(nOnLastLine&&nOnLastLine%4) nRet+=4-nOnLastLine%4;

	//nCRLFs*=2;
	//if(~dwFlags&ATL_BASE64_FLAG_NOCRLF) nRet+=nCRLFs;
	return nRet;
}
inline bool som_atlenc_Base64Encode(const BYTE *pbSrcData, int nSrcLen, LPSTR szDest, int *pnDestLen) 
{
	static const char s_chBase64EncodingTable[64+1] = 	
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789@$";

	if(!pbSrcData||!szDest||!pnDestLen
	 ||*pnDestLen<som_atlenc_Base64EncodeGetRequiredLength(nSrcLen)){ assert(0); return false; }

	int nWritten = 0, nLen1 = nSrcLen/3*4, nLen2 = nLen1/76, nLen3 = 19;

	for(int i=0;i<=nLen2;i++)
	{
		if(i==nLen2) nLen3 = (nLen1%76)/4;

		for(int j=0;j<nLen3;j++)
		{
			DWORD dwCurr = 0;
			for(int n=0;n<3;n++,dwCurr<<=8)	
			dwCurr|=*pbSrcData++; 
			for(int k=0;k<4;k++,dwCurr<<=6)
			*szDest++ = s_chBase64EncodingTable[dwCurr>>26];
		}
		nWritten+=nLen3*4;

		//if(~dwFlags&ATL_BASE64_FLAG_NOCRLF)
		{
		//	*szDest++ = '\r'; *szDest++ = '\n';	nWritten+=2;
		}
	}
	//if(nWritten&&~dwFlags&ATL_BASE64_FLAG_NOCRLF)
	{
	//	szDest-=2; nWritten-=2;
	}
	int nSrcLenX3 = nLen2 = nSrcLen%3; if(nLen2++)
	{
		DWORD dwCurr = 0;
		for(int n=0;n<3;n++,dwCurr<<=8)		
		if(n<nSrcLenX3)	dwCurr|=*pbSrcData++;
		for(int k=0;k<nLen2;k++,dwCurr<<=6)		
		*szDest++ = s_chBase64EncodingTable[dwCurr>>26];
		nWritten+=nLen2;
		/*if(~dwFlags&ATL_BASE64_FLAG_NOPAD)
		{
			nLen3 = nLen2?4-nLen2:0;
			for(int j=0;j<nLen3;j++) *szDest++ = '=';
			nWritten+=nLen3;
		}*/
	}
	*pnDestLen = nWritten;
	return true;
}

inline int som_atlenc_DecodeBase64Char(unsigned int ch)
{
	//returns -1 if the character is invalid or should be skipped
	//otherwise, returns the 6-bit code for the character from the encoding table
	if(ch>='A'&&ch<='Z') return ch-'A'+00; //00 range starts at 'A'
	if(ch>='a'&&ch<='z') return ch-'a'+26; //26 range starts at 'a'
	if(ch>='0'&&ch<='9') return ch-'0'+52; //52 range starts at '0'
	if(ch=='@')	return 62;
	if(ch=='$')	return 63; return -1;
}
inline bool som_atlenc_Base64Decode(LPCSTR szSrc, int nSrcLen, BYTE *pbDest, int *pnDestLen)
{
	//walk the source buffer
	//each four character sequence is converted to 3 bytes
	//CRLFs and =, and any characters not in the encoding table are skiped

	if(!szSrc||!pnDestLen){	assert(0); return false; }
	
	LPCSTR szSrcEnd = szSrc+nSrcLen; 
	int nWritten = 0, bOverflow = !pbDest;	
	while(szSrc<szSrcEnd&&*szSrc)
	{
		int nBits = 0; DWORD dwCurr = 0;		
		for(int i=0;i<4&&szSrc<szSrcEnd;i++)
		{
			int nCh = som_atlenc_DecodeBase64Char(*szSrc++);

			if(nCh==-1){ i--; continue; } //skip this char

			dwCurr<<=6;	dwCurr|=nCh; nBits+=6;
		}

		bOverflow = bOverflow||nWritten+nBits/8>*pnDestLen;

		//left to right
		//dwCurr has the 3 bytes to write to the output buffer		
		dwCurr<<=24-nBits;
		for(int i=0,n=nBits/8;i<n;i++,dwCurr<<=8,nWritten++)		
		if(!bOverflow)
		*pbDest++ = (BYTE)((dwCurr&0x00ff0000)>>16);
	}	
	*pnDestLen = nWritten;
	
	if(bOverflow)
	{
		assert(!pbDest); return false;
	}
	else return true;
}
#endif //_WIN32
#endif //SOM_ATLENC_INCLUDED