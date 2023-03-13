				
#ifndef SOMINSTANT_INCLUDED
#define SOMINSTANT_INCLUDED

#include "Somvector.h"

#ifdef _DEBUG
#define SOMINSTANT_DOUBLE
#endif

#ifdef SOMINSTANT_DOUBLE
#define SOMINSTANT_SCALAR double
#else 
#define SOMINSTANT_SCALAR float
#endif
 
class Sominstant;

namespace Sominstant_h
{		
typedef SOMINSTANT_SCALAR scalar;

	class vector //homogeneous
	:
	public Somvector::vector<scalar,4> 
	{
	public:	scalar x, y, z, w; vector(){} 
				
		vector(scalar h){ x = y = z = 0; w = h;	}

		vector(scalar X, scalar Y, scalar Z, scalar W) 
		{
			//compiler: Empty Base Optimization assertion
			int compile[sizeof(*this)==sizeof(scalar)*4];

			x = X; y = Y; z = Z; w = W;
		}
	};		

	class matrix //column major
	:
	public Somvector::matrix<scalar,4,4>
	{	
	public:	vector u, v, n, o; matrix(){}
						
		matrix(int i){ if(i==1) identity<4,4>(); else u.x = 0; }

		inline matrix(scalar fov, scalar aspect, scalar znear, scalar zfar)
		{
			perspective<4,4>(fov,aspect,znear,zfar); //projection matrix
		}							  	
		inline matrix &normalize() //othonormal matrices
		{
			u.unit<3>(); v.unit<3>(); n.unit<3>(); return *this; 
		}
		inline matrix &affinvert() //invert affine matrix
		{
			invert<3,3>(); o.flip<3>();
			o.ser<0,2>(3).premultiply<3>(ser<0,2,0,2>(3,3)); 
			return *this;
		} 
	};

#ifndef _CPPRTTI //MSVC++
#define SOMINSTANT_OLDHAND
#else

	struct cast_and_crew 
	{		  	
		//// we need to implement one virtual function ////
		//// to achieve polymorphism with dynamic_cast ////
		//// so why not have some fun with downcasting ////
	
		virtual cast_and_crew *chain_gang()const{ return 0; }

		virtual cast_and_crew *abandon_ship(Sominstant *ship)
		{
			return chain_gang(); //time to reclaim your maties
		}
	};

#endif	
}

class Sominstant 
: 
public Sominstant_h::matrix

#ifndef SOMINSTANT_OLDHAND
,
public Sominstant_h::cast_and_crew
#endif
{
public: Sominstant(int i=1){ u.x = i; } 	

	inline bool collapsed()const{ return !u.x; } 
		
#ifndef SOMINSTANT_OLDHAND
	
	////in theory this can be used to chain together any number of////
	////compatible classes at runtime; there just can't be overlap////

	template<typename T> static T cast_support_magic(const Sominstant *i)
	{
		T ptr; if(ptr=dynamic_cast<T>(const_cast<Sominstant*>(i))); else if(i)
		for(cast_and_crew*cg=i->chain_gang();cg;cg=(ptr=dynamic_cast<T>(cg))?0:cg->chain_gang());
		return ptr;
	}
	template<typename T> inline T cast_support_magic(T &t)const //short hand
	{
		return t = cast_support_magic<T>(this);
	}

	~Sominstant() 
	{
		for(cast_and_crew*cg=abandon_ship(this);cg;cg=cg->abandon_ship(this)); 

	#ifdef SOMINSTANT_DTOR
	
		SOMINSTANT_DTOR

	#endif
	}
	
#endif

#ifdef SOMINSTANT_INLINE

#include "Sominstant.inl"

#endif

#ifdef SOMINSTANT_EXTEND
	
	SOMINSTANT_EXTEND

#endif

};

#endif //SOMINSTANT_INCLUDED