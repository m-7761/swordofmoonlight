					
#ifndef SOMSCRIBE_INCLUDED
#define SOMSCRIBE_INCLUDED

#include "Somthread.h"
#include "Sominstant.h"

class Somscribe
:
public Sominstant
{
public:

	cast_and_crew *cg;
	cast_and_crew *chain_gang()const{ return cg; }

	Somscribe(){ cg = 0; }
		
	template<class U, class T> U detail(T *t, U u=0, size_t n=1) 
	{
		if(!t) return 0;

		if(!u) //if U is present return it; else add it
		{
			if(u=Sominstant::cast_support_magic<U>(t)) return u; 

			Somscribe::_place(t,u=(U)(new char*[sizeof(*u)*n]),n); //typeof

			u->countdown(u,n); //Somscribe_h::element::countdown
		}
				
		if(u->cg){ assert(0); return 0; }

		u->cg = t->cg; t->cg = dynamic_cast<cast_and_crew*>(u); 

		assert(u->complement()==n);

		return u; 
	}

	template<class U, class T> inline U detail(size_t n, T *t) 
	{
		return detail<U>(t,0,n); 
	}

	template<class T> void disturb(T *t) 
	{
		t->_disturb(t); assert((void*)t!=(void*)this);
	}
	template<class T> void disturb(T *t, int m, int n=0) 
	{
		t->_disturb(t,m,n); assert((void*)t!=(void*)this);
	}
	template<class T> void dismiss(T *t) 
	{
		t->_dismiss(t); assert((void*)t!=(void*)this);
	}

	template<class T>
	static cast_and_crew *_abandon(Sominstant *ship, T *_this)
	{
		cast_and_crew *out = _this->cg; 

		if(dynamic_cast<Sominstant*>(_this)!=ship) delete _this;

		return out;
	}	

	template<class U> void _place(Sominstant *p, U *q, size_t n=1) 
	{
		//MSVC2005: new (p) U[n]; compiles but does not seem to do anything

		for(size_t i=0;i<n;i++) (new (q+i) U)->place(p); //placement syntax
	}
};

template<class T> class Som	: public Somscribe, public T 
{		
public: typedef T T; //Ex. Som<object>[::T]

	using Somscribe::cg, Somscribe::chain_gang;  
		
	cast_and_crew *abandon_ship(Sominstant *ship)
	{	
		return Somscribe::_abandon(ship,this);
	}
};

namespace Somscribe_h
{	
	using Sominstant_h::scalar;
	using Sominstant_h::vector;

	//instant
	struct matrix 
	{
		//Note: matrix is actually hollow
		//until inherited by a Sominstant

		inline Sominstant *instant()const
		{
			 return dynamic_cast<Sominstant*>((matrix*)this);
		}

		//orient is pretty harmless and having a virtual
		//allows us to dynamic_cast up to Sominstant* or
		//Som<whatever>* at the cost of an added pointer

		//apply orientation to p and q relative to the other
		virtual	bool orient(Sominstant *p, Sominstant *q=0)const
		{
			return false; //Ex. if(!q) q = this->instant();
		}

		void place(Sominstant *i){} //initialization

		//placeholder: see element::countdown
		static inline void countdown(void*,size_t){} 

		//placeholder: see element::complement
		static inline int complement(){ return 1; }
	};
	
	//record
	struct appendix 
	:
	public Sominstant::cast_and_crew
	{	
		//inherit from appendix if you would like to use
		//Somscribe::detail with a Som<object>::T object

		cast_and_crew *cg;
		cast_and_crew *chain_gang()const{ return cg; }

		cast_and_crew *abandon_ship(Sominstant *ship)=0;

		appendix(){ cg = 0; }		
	};	

	//array
	struct element : public matrix 
	{
		int number, parity; 

		element(){ number = 1; parity = 0; } 					

		template<class T> static void countdown(T *t, size_t N)
		{
			for(size_t i=0;i<N;i++){ t[t[i].parity=i].number = N-i; }
		}

		inline int complement()
		{
			return number+parity; 
		}
	};
	
	//table
	struct wizard : public element 
	{	
		Som<element> *table; 

		inline bool hermit()
		{
			return number==1&&parity==0;
		}
		inline int minions()
		{
			return parity==0?number-1:0;
		}
		inline int elements()
		{
			return table?table->complement()/complement():0;
		}
		
		wizard(){ table = 0; }

		~wizard(){ assert(!table); } 

	protected: friend Somscribe;

		template<class T> void _disturb(T *_this, int m, int n)
		{	
			const size_t N = m*n+n; assert(!table);

			if(N) countdown(table=new Som<element>[N],N);

			if(N) for(int i=1;i<m;i++) _this[i].table = table+i*n;			

			countdown(this,m);
		}
		template<class T> void _dismiss(T *_this)
		{	
			if(table) for(int i=1;i<number;i++) _this[i].table = 0;

			delete [] table; table = 0;
		}
	};
		
	//memory
	struct sleeper : public element 
	{		
		//assuming zzZ is self explanatory
		bool zzZ; sleeper(){ zzZ = true; }

	protected: friend Somscribe;

		template<typename T> void _disturb(T *_this)
		{
			zzZ = false; 
		}		
		template<typename T> void _dismiss(T *_this)
		{
			element save = *this; //HERE IS A WEAK LINK

			Sominstant::cast_and_crew *_cg = _this->cg; //save

			_this->~T(); new (_this) T; 
			
			number = save.number; _this->cg = _cg; //restore
			parity = save.parity;
			
			assert(zzZ==true);
		}	
	};
	
	//signal
	struct beacon : public sleeper 
	{
		float light[3]; //unit red, green, and blue

		beacon(){ light[0] = light[1] = light[2] = 0.5; }		
	};

	//camera
	struct cyclops : public beacon
	{	
		scalar skyline; beacon lens; //0.5,0.5,0.5

		scalar fov, aspect, znear, zfar, zfog, fogline, tripod; //Eg. 1.5		

		//FYI: fov and aspect use Sword of Moonlight's defaults (unsure about near/far)
		cyclops():fov(50),aspect(1),znear(0.1),zfar(50){zfog=fogline=skyline=tripod=0;}		

		bool orient(Sominstant*p,Sominstant*q=0)const; //write locks blink

		mutable Somthread_h::interlock blink;
	};
	
	//display
	struct medusa : public sleeper
	{
		enum{ BLINDS=1 };

		int affects, display[4][4]; 
		
		medusa(){ affects = display[0][0] = 0; }
	};

	//posture
	struct skeleton : public matrix
	{
		//move: base control point
		//turn: forward quaternion
		//look: local Euler angles
		vector move, turn, look; //gaze, eyes; 
						
		skeleton():move(1),turn(1),look(0){}

		void standup(Sominstant *i=0)const
		{
			if(!i) i = instant(); if(!i) return;
			vector q; q.quaternion(look).postmultiply(turn);
			i->copy_quaternion<3,4>(q); i->o = move;
		}

		void place(Sominstant *i)
		{
			if(i){ move = i->o; turn.quaternion(*i); }

			standup(); assert(i);
		}
	};
	
	//environs
	struct cosmos : public matrix
	{
		int frame; cosmos(){ frame = 1; }
	};
}

#endif //SOMSCRIBE_INCLUDED