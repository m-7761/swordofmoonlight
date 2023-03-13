																		  						  
#ifndef SOMREGIS_INCLUDED
#define SOMREGIS_INCLUDED

#include "Somthread.h"

/*Somregis.h is a repository of registry business*/
//
// TODO: how about making Somregis its own project?

namespace Somregis_h
{	
	//pattern for reading and writing REG_MULTI_SZ
	template<typename T, size_t N> struct multi_sz
	{	
		//T will have to be a reference
		//or pointer or a smart pointer
		T lv; typedef T T; enum{ N=N };
							
		//you will have to implement these yourself
		inline int f(T, wchar_t (&)[N], size_t &i); //writer
		inline int g(T, wchar_t (&)[N], size_t &i); //reader

		bool rd, cp; wchar_t *sz; DWORD cb; //size_t

		//TODO: try to make new / delete more easily optional
		multi_sz(T rvalue, size_t read_cb=0, wchar_t *read=0)
		{
			assert(!read||read_cb);

			lv = rvalue; rd = read||read_cb;

			sz = read; cb = read_cb; cp = !read;

			if(rd&&cp) 
			{
				sz = (wchar_t*)new char[read_cb]; 
				*sz = 0; 
			}

			if(rd) return; //writing
			
			Somthread_h::section cs;

			using Somplayer_pch::array;

			//avoiding std::string
			static std::vector<array<wchar_t,N> >vw;

			array<wchar_t,N> ln; vw.clear();
			
			for(size_t i=0;ln.size=f(lv,ln._s,i);i++)
			{	
				assert(ln.size<N); 

				if(ln.size<N) vw.push_back(ln); else break;				
			}

			size_t len = 0;
			for(size_t i=0,n=vw.size();i<n;i++)
			{
				len+=vw[i].size+1;
			}				  
			len = len?len+1:2; 

			sz = new wchar_t[len];
			cb = len*sizeof(wchar_t);

			sz[0] = '\0';

			size_t pos = 0;
			for(size_t i=0,n=vw.size();i<n;i++)
			{
				wcsncpy(sz+pos,vw[i],vw[i].size);

				sz[pos+=vw[i].size] = 0; pos++;
			}

			sz[len-1] = 0;
		}
		~multi_sz()
		{
			assert(sz&&cb);
			
			if(!rd&&cp) delete sz;

			if(!rd) return; //reading
			
			Somthread_h::section cs;

			using Somplayer_pch::array;
												
			wchar_t *p = sz, *b = p+cb/sizeof(wchar_t);

			array<wchar_t,N> ln; 

			for(size_t i=0,j;p<b;i++)
			{	
				for(j=0;p<b&&*p&&j<N;j++) ln[j] = *p++;

				if(j<N&&p<b) ln[j] = *p++; else break;

				if(ln[j]!=0) break; //extreme paranoia

				//! the last line is an empty string
				int f = g(lv,ln._s,i); assert(f>=0);
				
				if(f<=0) break; 
			}
			  
			if(cp) delete sz;
		}
	};
}
	
#endif //SOMREGIS_INCLUDED