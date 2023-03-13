	 
#ifndef EX_DATASET_INCLUDED
#define EX_DATASET_INCLUDED

		/*2021

		This was assisting som.keys.h ... it might have its
		uses, I'm not sure, but it's not being used for now

		//REMINDER
		DDRAW::IDirectDrawSurface7::Query::IDirectDrawSurface7::clientdata
		was passed to EX::DATA::Get/Set

		*/
		#error REMOVED FROM BUILD

namespace EX
{
	namespace DATA
	{
		struct Unknown{};

		template<typename T>
		struct Bit
		{	
			int type; T data;
			
			Bit<EX::DATA::Unknown> *next;			

			static int define_me;			
		};

		#define EX_DATA_DEFINE(T)\
		int EX::DATA::Bit<T>::define_me = 0; 

		template<typename T>
		inline void Gen(const T &t)
		{
			if(EX::DATA::Bit<T>::define_me) return;

			int compile[sizeof(T)>sizeof(Unknown)];

			EX::DATA::Bit<T>::define_me = ++
			EX::DATA::Bit<EX::DATA::Unknown>::define_me;
		}

		template<typename T>
		bool Set(const T &t, void* &set, bool unset=false)
		{				
			EX::DATA::Gen(t);

			EX::DATA::Bit<EX::DATA::Unknown> *p = 
			(EX::DATA::Bit<EX::DATA::Unknown>*)set, *q = 0;

			while(p&&p->type!=EX::DATA::Bit<T>::define_me)q=p,p=p->next;
			
			EX::DATA::Bit<T> *b = 0;

			if(!p)
			{
				if(unset) return true;

				if(b=(EX::DATA::Bit<T>*)new BYTE[sizeof(*b)])
				{
					b->type = EX::DATA::Bit<T>::define_me; 

					if(!q) set = b; else q->next = 
					(EX::DATA::Bit<EX::DATA::Unknown>*)b;
					
					b->next = 0; 
				}
				else return false;
			}
			else if(unset)
			{
				if(q) q->next = p->next; else set = p->next;

				delete[] (BYTE*)p;	return true;
			}

			memcpy(&b->data,&t,sizeof(T));

			return true;
		}

		template<typename T>
		bool Get(T &t, void* &set, bool create=true)
		{
			EX::DATA::Gen(t);

			if(!set&&!create) return false;

			EX::DATA::Bit<EX::DATA::Unknown> *p = 
			(EX::DATA::Bit<EX::DATA::Unknown>*)set, *q = 0;

			while(p&&p->type!=EX::DATA::Bit<T>::define_me)q=p,p=p->next;
						
			if(!p)
			{
				if(!create) return false;

				EX::DATA::Bit<T> *b = 0;

				if(b=(EX::DATA::Bit<T>*)new BYTE[sizeof(*b)])
				{
					b->type = EX::DATA::Bit<T>::define_me;

					memcpy(&b->data,&t,sizeof(T));

					if(!q) set = b; else q->next = 
					(EX::DATA::Bit<EX::DATA::Unknown>*)b;
					
					b->next = 0; return true;
				}
				else return false;
			}

			memcpy(&t,&p->data,sizeof(T));

			return true;
		}

		//REMOVE ME?
		void Nul(void* &set); //Ex.dataset.cpp
		//{				
		//	EX::DATA::Bit<EX::DATA::Unknown> *p = 
		//	(EX::DATA::Bit<EX::DATA::Unknown>*)set, *q = 0;
		//
		//	while(p)
		//	{
		//		q = p; p = p->next; delete[] (BYTE*)q;
		//	}
		//
		//	set = 0;
		//}
	}
}

#endif //EX_DATASET_INCLUDED