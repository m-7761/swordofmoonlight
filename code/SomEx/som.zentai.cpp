		 
#include "Ex.h" 
EX_TRANSLATION_UNIT

#include "som.extra.h"

//31/34: class/protocol bytes
//classes       protocols
//0:NPC		    0:0~9 (SYSTEM)
//1:Enemy	    1:Button
//2:Object	    2:Key item
//FC:conflicted 4/8:Square/Circle
//FD:Systemwide 10:Defeat (16)
//FE:System     20:Continuous (32)
//FF:deleted    40:Anywhere item (64)
static const WORD som_zentai_void[252/2] = //252 bytes
{ 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //0 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFF00, //16
	0x0000, 0xFF00, 0x0168, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //32 //0xFF01
//	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	//48
//	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //64
//	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
//	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
//	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
//	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
//	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
//	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
//	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
//	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
//	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
//	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
//	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};

static bool som_zentai_conflicts = false;
static bool som_zentai_conflict(int id, const BYTE *conflict, const BYTE *systemwide)
{
	som_zentai_conflicts = true; //optimization
	wchar_t a[31],b[31];
	EX::need_unicode_equivalent(932,(char*)conflict,a,31);
	EX::need_unicode_equivalent(932,(char*)systemwide,b,31); 	
	return IDCANCEL!=EX::messagebox(MB_OKCANCEL,
	"Event #%04d \"%ls\" is in the way of systemwide event \"%ls\".\n\n"
	"In order to resolve this conflict you will need to move (or remove) the event.\n\n"
	"Afterward the systemwide event will not reappear until this map is reloaded.\n\n"
	"Choosing CANCEL will disable these warnings on this map until it is reloaded."
	,id,a,b);
}
static void som_zentai_troubles()
{
	assert(0); //UNIMPLEMENTED
}

extern HANDLE som_zentai_ezt = 0;

extern void SOM::zentai_init(HANDLE ezt)
{
	if(ezt==som_zentai_ezt) return;		

	som_zentai_ezt = ezt; if(!ezt) return;

	if(ezt==INVALID_HANDLE_VALUE)
	{
		assert(ezt!=INVALID_HANDLE_VALUE);
	}
	else if(!GetFileSize(ezt,0))
	{
		static DWORD header = 1024, writ;
		WriteFile(ezt,&header,sizeof(header),&writ,0);
		for(DWORD i=0;i<header;i++) 
		WriteFile(ezt,som_zentai_void,sizeof(som_zentai_void),&writ,0);
		assert(writ==252);
	}
}

extern void SOM::zentai_split(HANDLE v) //evt
{
	DWORD sz = GetFileSize(v,0);
	HANDLE z = som_zentai_ezt, conflicts = 0;	
	//THIS BLOCK OF CODE CONCERNS CONFLICT RESOLUTION
	DWORD csz = GetFileSize(z,0), c1024 = 0, rd; BYTE c = 0;
	if(som_zentai_conflicts&&!SetFilePointer(z,0,0,FILE_BEGIN)
	&&ReadFile(z,&c1024,4,&rd,0)&&SetFilePointer(z,31,0,FILE_CURRENT))
	{	
		if(rd==4) //0xFC: spells conflict
		for(DWORD i=0;i++<c1024&&c!=0xFC; 
		ReadFile(z,&c,1,&rd,0),SetFilePointer(z,252-rd,0,FILE_CURRENT));
		if(c==0xFC) //backup data section
		{				
			static EX::temporary cp;			   
			SetFilePointer(cp,0,0,FILE_BEGIN);
			SetFilePointer(z,4+c1024*252,0,FILE_BEGIN);
			if(cp.copy<4096>(z)==csz-4-c1024*252) conflicts = cp; 
			assert(conflicts);
		}
	}
	if(sz==INVALID_FILE_SIZE
	||csz==INVALID_FILE_SIZE||csz<4+c1024*252)
	{
		return som_zentai_troubles();
	}
	//s2 IS sz WITH EXTRA PADDING TO FIT IN CONFLICTS
	DWORD s2 = sz; if(conflicts) s2+=csz-4-c1024*252;

	//memory-mapped-files here works wonders for clarity sake
	HANDLE vm = CreateFileMapping(v,0,PAGE_READWRITE,0,s2,0);
	HANDLE zm = CreateFileMapping(z,0,PAGE_READWRITE,0,s2,0);

	BYTE *vv = (BYTE*)MapViewOfFile(vm,FILE_MAP_WRITE,0,0,s2);
	BYTE *zv = (BYTE*)MapViewOfFile(zm,FILE_MAP_WRITE,0,0,s2);

	if(!vv||!zv) return som_zentai_troubles();

	int header = *(DWORD*)vv; 

	BYTE *p = vv+4, *pp = p;
	BYTE *dd = p+header*252, *q = zv+4;
	int pdata = 4+header*252, qdata = 4+header*252;

	*(DWORD*)zv = header;
	for(int i=0,j,k;i<header;i++,p+=252,q+=252)
	{			
		//look ahead for next data
		if(p==pp) while((pp+=252)<dd)
		{	
			int *epp = (int*)(pp+60);
			for(k=0;k<16&&!*epp;k++,epp+=3);		
			if(k<16) break;
		}
		else assert(p<pp);

		int data = 0;
		int *ep = (int*)(p+60);		
		for(j=0;j<16&&!*ep;j++,ep+=3);
		if(j<16) //should have data
		{
			if(pp<dd) //same as above
			{
				int *epp = (int*)(pp+60);
				for(k=0;k<16&&!*epp;k++,epp+=3);		
				if(k<16) data = *epp-*ep; 
			}
			else data = sz-*ep; assert(data);
		}

		bool conflicted = 
		conflicts&&q[31]==0xFC&&i<int(c1024);
		bool systemwide = p[31]==0xFD;

		if(systemwide&&!conflicted)		 
		{
			if(data) //move data from v to z
			{			
				memcpy(zv+qdata,vv+*ep,data);

				int diff = qdata-*ep; 
				for(;j<16;j++,ep+=3) if(*ep) *ep+=diff;

				qdata+=data; //!
			}			

			memcpy(q,p,252);
			memcpy(p,som_zentai_void,252);
		}
		else //record/data stays in EVT file
		{				
			if(!conflicted)
			memcpy(q,som_zentai_void,252);
			//memcpy(p,p,252);

			if(data) //move data within v
			{
				if(*ep!=pdata) //optimization
				{	
					memcpy(vv+pdata,vv+*ep,data);

					int diff = pdata-*ep;
					for(;j<16;j++,ep+=3) if(*ep) *ep+=diff;
				}
				pdata+=data;
			}
		}

		if(conflicted) //preserve data
		{
			data = 0;
			ep = (int*)(q+60);		
			for(j=0;j<16&&!*ep;j++,ep+=3);
			if(j<16) //should have data
			{
				BYTE *qq = q+252;
				BYTE *qq_s = zv+4+c1024*252;
				for(;qq<qq_s&&!data;qq+=252) 
				{
					int *epp = (int*)(qq+60);
					for(k=0;k<16&&!*epp;k++,epp+=3);		
					if(k<16) data = *epp-*ep; 
				}
				if(qq==qq_s&&!data) data = csz-*ep;

				if(data) //restore to z from data backup file
				{	
					LONG fp = *ep-4-c1024*252; 
					SetFilePointer(conflicts,fp,0,FILE_BEGIN);
					ReadFile(conflicts,zv+qdata,data,&rd,0);

					int diff = qdata-*ep; 
					for(;j<16;j++,ep+=3) if(*ep) *ep+=diff;

					qdata+=data; //!
				}			
				else assert(0);
			}
		}
	}

	UnmapViewOfFile(vv);
	CloseHandle(vm);
	SetFilePointer(v,pdata,0,FILE_BEGIN);
	SetEndOfFile(v);
	SetFilePointer(v,0,0,FILE_BEGIN);

	UnmapViewOfFile(zv);
	CloseHandle(zm);
	SetFilePointer(z,qdata,0,FILE_BEGIN);
	SetEndOfFile(z);
	SetFilePointer(z,0,0,FILE_BEGIN);

	FlushFileBuffers(v);
	FlushFileBuffers(z);
}

extern void SOM::zentai_splice(HANDLE v) //evt
{
	HANDLE z = som_zentai_ezt;
	DWORD vsz = GetFileSize(v,0);
	DWORD zsz = GetFileSize(z,0);
		
	DWORD vheader = 0, zheader = 0, rw;

	SetFilePointer(v,0,0,FILE_BEGIN);
	SetFilePointer(z,0,0,FILE_BEGIN);
	if(!ReadFile(v,&vheader,4,&rw,0)||4+vheader*252>vsz
	 ||!ReadFile(z,&zheader,4,&rw,0)||4+zheader*252>zsz)
	{
		SetFilePointer(v,0,0,FILE_BEGIN); //courtesy
		SetFilePointer(z,0,0,FILE_BEGIN); //(peeking)

		return som_zentai_troubles();
	}
	
	int header = max(vheader,zheader);		
	
	DWORD sz = 4+header*252+vsz-4-vheader*252+zsz-4-zheader*252; 

	//memory-mapped-files here works wonders for clarity sake
	HANDLE vm = CreateFileMapping(v,0,PAGE_READWRITE,0,sz,0);
	HANDLE zm = CreateFileMapping(z,0,PAGE_READONLY,0,zsz,0);

	BYTE *vv = (BYTE*)MapViewOfFile(vm,FILE_MAP_WRITE,0,0,sz);
	BYTE *zv = (BYTE*)MapViewOfFile(zm,FILE_MAP_READ,0,0,zsz);

	assert(vv&&zv); 
	
	if(header>(int)vheader) //untested
	{
		//Likely incompatible with SOM
		//(implemented for completeness sake)
		memmove(vv+4+header,vv+4+vheader,vsz-4-vheader*252);
		int diff = (header-vheader)*252;
		for(DWORD i=0;i<vheader;i++)
		{
			BYTE *p = vv+4+i*252;
			int *ep = (int*)(p+60);
			for(int j=0;j<16;j++,ep+=3) if(*ep) ep+=diff;
		}
		for(int i=vheader;i<header;i++)
		memcpy(vv+4+i*252,som_zentai_void,252);
		vheader = *(DWORD*)vv = header;
	}
	assert((int)vheader==header); //!

	int odata = sz, cdata = 0;
	int pdata = vsz, qdata = zsz;
	BYTE *p = vv+4+header*252-252;
	BYTE *q = zv+4+zheader*252-252;

	if((int)zheader<header) //preprocess
	{
		for(int i=zheader,j;i<header;i++)
		{
			BYTE *p = vv+4+i*252;
			int *ep = (int*)(p+60);
			for(j=0;j<16&&!*ep;j++,ep+=3); //!
			if(j>=16) continue;
			
			int diff = sz-(vsz-*ep);
			memmove(vv+*ep+diff,vv+*ep,vsz-*ep);
			pdata-=vsz-*ep;
			
			for(;i<header;i++,p+=252)
			{
				int *ep = (int*)(p+60);
				for(int j=0;j<16;j++,ep+=3) if(*ep) ep+=diff;
			}
		}
		p-=252*(header-zheader); header = zheader;
	}
	assert((int)zheader==header); //!

	bool conflict_warning = true;

	//Ps & Qs: this algorithm works backwards
	for(int i=0,j;i<header;i++,p-=252,q-=252)
	{	
		bool deletion = p[31]==0xFF;
		bool systemwide = q[31]==0xFD||q[31]==0xFC;		

		if(deletion&&!systemwide) continue; //trivial
		
		if(systemwide) //conflict/resolution
		{	
			BYTE mark = 0xFD;
			int c = header-i-1; 
			
			if(!deletion) //conflict?
			{
				if(c<10) //special uses
				{
					//effectively deleted?
					int *ep = (int*)(p+60);
					for(j=0;j<16&&!*ep;j++,ep+=3);								
					if(j<16) mark = 0xFC;
				}
				else mark = 0xFC;
			}

			if(mark!=q[31]) //try to swap 0xFD and 0xFC
			{
				SetFilePointer(z,4+c*252+31,0,FILE_BEGIN);
				if(WriteFile(z,&mark,1,&rw,0)) //readonly?
				assert(q[31]==mark);
			}			

			if(mark==0xFC) //conflict
			{
				if(conflict_warning) 
				{
					conflict_warning = 
					som_zentai_conflict(c,p,q); //MB_OKCANCEL 
				}

				//exclude this EZT data
				int *ep = (int*)(q+60);
				for(j=0;j<16&&!*ep;j++,ep+=3);	
				if(j<16) 
				{					
					cdata+=qdata-*ep;
					qdata-=qdata-*ep;
				}

				systemwide = false; //!
			}			
		}
		
		//first things first: copy over
		if(systemwide) memcpy(p,q,252);
		   
		int data = 0;
		int *ep = (int*)(p+60);
		for(j=0;j<16&&!*ep;j++,ep+=3);		
		if(j>=16) continue;
		data = (systemwide?qdata:pdata)-*ep;
		odata-=data; assert(data);

		int diff = odata-*ep;
		if(diff) for(;j<16;j++,ep+=3) if(*ep) *ep+=diff;

		if(systemwide) //moved data from z to v
		{	
			qdata-=data; 
			memcpy(vv+odata,zv+qdata,data);			
		}
		else //record/data stayed in EVT file
		{
			pdata-=data;
			if(odata!=pdata)
			memmove(vv+odata,vv+pdata,data);			
		}		
	}
	if(odata>4+header*252) //close gap in data
	{			
		//expected data due to confliction
		assert(cdata==odata-4-header*252);

		int diff = 4+header*252-odata; 		
		memmove(vv+odata+diff,vv+odata,sz-odata);		
		for(int i=0;i<header;i++)
		{
			BYTE *p = vv+4+i*252;
			int *ep = (int*)(p+60);
			for(int j=0;j<16;j++,ep+=3) if(*ep) *ep+=diff;			
		}
		odata+=diff; assert(diff<0);
		sz+=diff; 
	}
	//todo: verify file integrity
	assert(odata==4+ header*252);
	assert(pdata==4+vheader*252);
	assert(qdata==4+zheader*252);

	UnmapViewOfFile(vv);
	CloseHandle(vm);
	SetFilePointer(v,sz,0,FILE_BEGIN);
	SetEndOfFile(v);
	SetFilePointer(v,0,0,FILE_BEGIN);

	UnmapViewOfFile(zv);
	CloseHandle(zm);
	//SetFilePointer(z,qdata,0,FILE_BEGIN);
	//SetEndOfFile(z);
	SetFilePointer(z,0,0,FILE_BEGIN);

	FlushFileBuffers(v);
	FlushFileBuffers(z);
}