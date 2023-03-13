
#ifndef SOMVECTOR_INCLUDED
#define SOMVECTOR_INCLUDED

#ifndef SOMVECTOR_MATH
#define SOMVECTOR_MATH std::
#endif
#ifndef SOMVECTOR_SIN
#define SOMVECTOR_SIN SOMVECTOR_MATH sin
#endif
#ifndef SOMVECTOR_TAN
#define SOMVECTOR_TAN SOMVECTOR_MATH tan
#endif
#ifndef SOMVECTOR_COS
#define SOMVECTOR_COS SOMVECTOR_MATH cos
#endif
#ifndef SOMVECTOR_ACOS
#define SOMVECTOR_ACOS SOMVECTOR_MATH acos
#endif
#ifndef SOMVECTOR_SQRT
#define SOMVECTOR_SQRT SOMVECTOR_MATH sqrt
#endif
#ifndef SOMVECTOR_PI
#define SOMVECTOR_PI 3.14159265358979323846
#endif

  /*///////////////// WARNING //////////////////

	Deriving from these classes requires Empty
	Base Optimization. Double check alignments
	It cannot use compilers that do not do EBO

  /*////////////////////////////////////////////

class Somvector //namespace
{	
public:	//matrices are column-major order
	   //So the rules you'll find in most
	  //books are going to be upside down

	template<class,int,int,int> class matrix;

	////WARNING for MSVC2005 developers //////////////
	//
	// See vector::premultiply for an example of a bug
	// that may lie dormant among the unused templates

	//vector (zero sized container)
	template<class S, int N> class vector
	{		
	private: friend class Somvector; friend class vector;

		typedef S scalarN[N]; typedef vector<S,N> vectorN; 

		//these do not enforce const-ness and will conflict
		//with any new operators--which is npt wanted anyway
		//[enforcing const-ness will break builtin operators]
		inline operator scalarN&(){ return *(scalarN*)this; }
		inline operator scalarN&()const{ return *(scalarN*)this; }

	protected: vector<S,N>(){} vector<S,N>(const vector<S,N>&){}

	public:	typedef S scalar; static const size_t N = N; 
		
		//sub element (single) selector
		template<int a> inline scalar &se() //const 
		{
			int compile[a<N]; return (*this)[a];
		}
		template<int a> inline const scalar &se()const
		{
			int compile[a<N]; return (*this)[a];
		}	

		//sub element range (serial) selector
		template<int a, int z> inline vector<S,z-a+1> &ser(int len) //const
		{
			int compile[z-a<N]; assert(len==z-a+1); return *(vector<S,z-a+1>*)(*this+a);
		}
		template<int a, int z> inline const vector<S,z-a+1> &ser(int len)const
		{
			int compile[z-a<N]; assert(len==z-a+1); return *(vector<S,z-a+1>*)(*this+a);
		}	

		//sub vector (shorthand) selector
		template<int Z> inline vector<S,Z> &s()/*const*/{ return ser<0,Z-1>(Z); }

		template<int Z> inline const vector<S,Z> &s()const{ return ser<0,Z-1>(Z); }	
		
		//static method (map a vector to memory)
		template<int Z> static inline vectorN &map(scalar (&mem)[Z]) //const
		{
			int compile[Z>=N]; return *(vectorN*)mem; 
		}
		template<int Z> static inline const vectorN &map(const scalar (&mem)[Z])
		{
			int compile[Z>=N]; return *(vectorN*)mem; 
		}
		template<int Z> static inline vectorN &map(scalar*const &mem) //const
		{
			int compile[Z>=N]; return *(vectorN*)mem; 
		}
		template<int Z> static inline const vectorN &map(const scalar*const &mem)
		{
			int compile[Z>=N]; return *(vectorN*)mem; 
		} 	

		//map acquisition (non-static)
		template<int Z> inline scalarN &map() //const
		{
			int compile[Z==N]; return *this;  
		}
		template<int Z> inline const scalarN &map()const
		{
			int compile[Z==N]; return *this;			
		}

		//WARNING: Traditionally set is used to setup the values of the vector
		//That is NOT what is going on here. Unfortunately "objectifying" data 
		//introduces a question of semantics: Is the object copying the input?
		//Or is the input copying the object? Consistent language is important.

		//write to memory 
		template<int n, int Z, class T> inline void set(T (&v)[Z])const
		{
			int compile[n<=N&&n<=Z]; for(size_t i=0;i<n;i++) v[i] = (*this)[i]; 
		}	
		template<int n, int Z, class T> inline void set(T*const v)const
		{
			int compile[n<=N&&n<=Z]; for(size_t i=0;i<n;i++) v[i] = (*this)[i]; 
		}	
				
		//extra friendly copy, move & scale 
		//(also includes se_copy, se_move & se_scale)
		//NEW: now utilizing/exposing lambda expression based assignment operations
		template<int n, class T, class OP> inline vectorN &apply(const T &v, OP op)
		{
			int compile[n<=N&&n<=T::N]; for(size_t i=0;i<n;i++) op((*this)[i],v[i]); return *this;
		}
		template<int n, int Z, class T, class OP> inline vectorN &apply(const T (&v)[Z], OP op)
		{
			int compile[n<=N&&n<=Z]; for(size_t i=0;i<n;i++) op((*this)[i],v[i]); return *this;
		}
		template<int n, class T, class OP> inline vectorN &apply(const T*const &v, OP op)
		{
			int compile[n<=N]; for(size_t i=0;i<n;i++) op((*this)[i],v[i]); return *this;
		}
		template<int n, class T, class OP> inline vectorN &apply(T*const &v, OP op)
		{
			int compile[n<=N]; for(size_t i=0;i<n;i++) op((*this)[i],v[i]); return *this;
		}
		//se: scalar extension (the naming is deliberately misleading)
		template<int n, class T, class OP> inline vectorN &se_apply(const T &s, OP op)
		{
			int compile[n<=N]; for(size_t i=0;i<n;i++) op((*this)[i],s); return *this;
		}	
		#define SOMVECTOR_LOOP_OP_(func,op) \
		template<int n, class T> inline vectorN &func(const T (&v))\
		{ return apply<n>(v,[](scalar &x, const decltype(v[0]) (&y)){ op; }); }\
		template<int n, class T> inline vectorN &se_##func(const T (&s))\
		{ return se_apply<n>(s,[](scalar &x, const T (&y)){ op; }); }
		SOMVECTOR_LOOP_OP_(copy,x=y)
		SOMVECTOR_LOOP_OP_(move,x+=y)		
		SOMVECTOR_LOOP_OP_(scale,x*=y) //read: flip&move (reciprocal&scale)
		SOMVECTOR_LOOP_OP_(remove,x-=y) SOMVECTOR_LOOP_OP_(flip_move,x=y-x)
		SOMVECTOR_LOOP_OP_(receive,x/=y) SOMVECTOR_LOOP_OP_(recip_scale,x=y/x)
		#undef SOMVECTOR_LOOP_OP_

		//inversion
		template<int n> inline vectorN &flip()
		{
			int compile[n<=N]; for(size_t i=0;i<n;i++) (*this)[i] = -(*this)[i]; return *this;
		}
		template<int n> inline vectorN &recip()
		{
			int compile[n<=N]; for(size_t i=0;i<n;i++) (*this)[i] = 1/(*this)[i]; return *this;
		}

		//precision
		template<int n, typename T> inline vectorN &trim(const T &p)
		{
			int compile[n<=N]; for(size_t i=0;i<n;i++) (*this)[i] = scalar(T((*this)[i]*p))/p; return *this;
		}
		template<int n, class T> inline vectorN &limit(const T &min, const T &max)
		{
			int compile[n<=N]; for(size_t i=0;i<n;i++) 				
			if((*this)[i]<min) (*this)[i] = min; else if((*this)[i]>max) (*this)[i] = max;			
			return *this;
		}
		template<int n, class T> inline vectorN &limit(const T &symmetric_min_max)
		{
			return limit<n>(-symmetric_min_max,+symmetric_min_max);
		}

		//dot&cross product
		template<int n, class T> inline scalar dot(const T &v)const
		{
			scalar compile[n<=N&&n<=v.N], o = 0;
			
			for(size_t i=0;i<n;i++) o+=(*this)[i]*v[i]; return o;
		}
		template<int n, class T> inline vectorN &cross(const T &v)
		{
			//// unsure if this generalizes to other dimensions. 7D? ////
		
			scalar compile[n==3&&n<=N&&n<=v.N], w[3]; map(w).copy<n>(*this); 
			
			(*this)[0] = w[1]*v[2]-w[2]*v[1]; 
			(*this)[1] = w[2]*v[0]-w[0]*v[2]; 
			(*this)[2] = w[0]*v[1]-w[1]*v[0]; return *this;
		}		

		//normalization	
		template<int n> inline vectorN &unit()
		{		
			scalar rcp = length<n>(); if(rcp) rcp = 1/rcp;
			for(size_t i=0;i<n;i++) (*this)[i]*=rcp; return *this;
		}	
		template<int n> inline scalar length()const
		{
			return SOMVECTOR_SQRT(dot<n>(*this));
		}
								
		//transformation
		//
		// NOTICE: n here needs to be 3+1 to multiply a 3 vector
		// and 4x4 matrix or else the result won't be translated
		//
		template<int n, class T> inline vectorN &premultiply(const T &t)
		{
			scalar cp[N]; Somvector::multiply<n>(map(cp).copy<N>(*this),t,*this);
			return *this;
		}
		template<int n, class T> inline vectorN &postmultiply(const T &t)
		{
			int compile[n==N]; //TODO: implement homogeneous extension

			scalar cp[t.N][1]; matrix<S,t.N,1> &col = transpose<t.N>(); 

			 //MSVC2005 (bug)
			//Somvector::multiply<n,1>(t,col.map(cp).copy<t.N,1>(col),col); 
			Somvector::multiply<n,1>(t,col.map(cp).copy<T::N,1>(col),col); 
			return *this;
		}	
		
		//transposition
		template<int m> inline matrix<S,m,1,1> &transpose() //const
		{
			int compile[m<=N]; return matrix<S,m,1,1>::map<m,1>(*this);
		}
		template<int m> inline const matrix<S,m,1,1> &transpose()const
		{
			int compile[m<=N]; return matrix<S,m,1,1>::map<m,1>(*this);
		}

		/*//// quaternions (4D) ////////*/

		//standard construction methods
		template<class S, int Z> inline vectorN &quaternion(const vector<S,Z> &Euler)
		{
			int compile[N==4&&Z>=3];
			const scalar _5 = -.5, //.5; //2021: SOM seems to be inverted?
			c0(SOMVECTOR_COS(_5*Euler[0])), c1(SOMVECTOR_COS(_5*Euler[1])),	//in radians
			c2(SOMVECTOR_COS(_5*Euler[2])), s0(SOMVECTOR_SIN(_5*Euler[0])), c1_c2(c1*c2),
			s1(SOMVECTOR_SIN(_5*Euler[1])), s2(SOMVECTOR_SIN(_5*Euler[2])), s1_s2(s1*s2);			
			(*this)[0] = s0*c1_c2-c0*s1_s2;	(*this)[1] = c0*s1*c2+s0*c1*s2;
			(*this)[2] = c0*c1*s2-s0*s1*c2;	(*this)[3] = c0*c1_c2+s0*s1_s2; return *this;
		}				
		template<class S, int Z> inline vectorN &quaternion(const S (&Euler)[Z])
		{
			return quaternion(vector<S,Z>::map(Euler)); //treat scalar arrays like Euler angles
		}
		template<int A, int B, int C, class S, int Z>
		inline vectorN &quaternion(const vector<S,Z> &Euler) 
		{
			//2021: I set this up to experimentally determine some things
			//then implemented them in matrix form using matrix::rotation
			
			enum{ AA=A<0?-A:A,BB=B<0?-B:B,CC=C<0?-C:C };
			int compile[N==4&&Z>=3&&AA<4&&BB<4&&CC<4];

			const scalar _5 = -.5; //.5; //2021: SOM seems to be inverted?
			scalar x = SOMVECTOR_SIN(Euler[0]*_5); if(A==-1||B==-1||C==-1) x = -x;
			scalar y = SOMVECTOR_SIN(Euler[1]*_5); if(A==-2||B==-2||C==-2) y = -y;
			scalar z = SOMVECTOR_SIN(Euler[2]*_5); if(A==-3||B==-3||C==-3) z = -z;
			
			scalar qAbc[3][4] = 
			{ 
				{AA==1?x:0,AA==2?y:0,AA==3?z:0,AA?SOMVECTOR_COS(Euler[AA-1]*.5):1},
				{BB==1?x:0,BB==2?y:0,BB==3?z:0,BB?SOMVECTOR_COS(Euler[BB-1]*.5):1},
				{CC==1?x:0,CC==2?y:0,CC==3?z:0,CC?SOMVECTOR_COS(Euler[CC-1]*.5):1},
			};
			//note, postmultiply can be obtained by reversing ABC (i.e. CBA)
			copy<4>(qAbc[A?0:B?1:2]);	
			if(B&&A) premultiply(qAbc[1]);
			if(C&&(A||B)) premultiply(qAbc[2]); return *this;
		}
		template<int A, int B, int C, class S, int Z>
		inline vectorN &quaternion(const S (&Euler)[Z]) 
		{
			return quaternion<A,B,C>(vector<S,Z>::map(Euler)); //treat scalar arrays like Euler angles
		}
		template<class S, class T, int Z> inline vectorN &quaternion(const vector<S,Z> &unit_axis, const T angle)
		{
			int compile[N==4&&Z>=3]; 
			
			const scalar rad = scalar(-.5)*angle; //.5; //2021: SOM seems to be inverted?

			(*this)[3] = SOMVECTOR_COS(rad); return copy<3>(unit_axis).se_scale<3>(SOMVECTOR_SIN(rad));
		}
		template<int U, int V, int W, class T> inline vectorN &quaternion(const T angle)
		{
			int compile[(U+V+W)*(U+V+W)==1&&U*U|V*V|W*W==1]; //try to enforce two 0s and one 1 or -1

			int axis[3] = {U,V,W}; return quaternion(vector<int,3>::map(axis),angle);
		}
		//DOCUMENT ME (this looks like rotating from one vector to another?)
		template<class S, class T, int W, int Z> inline vectorN &quaternion(const vector<S,W> &s, const vector<T,Z> &t)
		{
			scalar cp[4]; return quaternion(map(cp).copy<3>(s).cross<3>(t).unit<3>(),SOMVECTOR_ACOS(s.dot<3>(t)));
		}		
		template<class T, int W, int Z, int ZZ> inline vectorN &quaternion(const matrix<T,W,Z,ZZ> &t) 
		{			
			int compile[N==4&&t.M>=3&&t.N>=3]; //TODO: non-trivial test

			scalar trace = t.trace<3>(), square = SOMVECTOR_SQRT(trace)*2;

			if(trace>0) //non-special case
			{
				(*this)[3] = square*scalar(0.25); square = scalar(1)/square;

				(*this)[0] = square*(t[1][2]-t[2][1]); (*this)[1] = square*(t[2][0]-t[0][2]);
				(*this)[2] = square*(t[0][1]-t[1][0]); 
			}
			else if(t[0][0]>t[1][1]&&t[0][0]>t[2][2])
			{	
				square = SOMVECTOR_SQRT(scalar(1)+t[0][0]-t[1][1]-t[2][2])*2;

				(*this)[0] = square*scalar(0.25); square = scalar(1)/square;

				(*this)[1] = square*(t[0][1]+t[1][0]); (*this)[2] = square*(t[2][0]+t[0][2]);
				(*this)[3] = square*(t[1][2]-t[2][1]);
			} 
			else if(t[1][1]>t[2][2]) 
			{ 
				square = SOMVECTOR_SQRT(scalar(1)+t[1][1]-t[0][0]-t[2][2])*2;

				(*this)[1] = square*scalar(0.25); square = scalar(1)/square;

				(*this)[0] = square*(t[0][1]+t[1][0]); (*this)[2] = square*(t[1][2]+t[2][1]);
				(*this)[3] = square*(t[2][0]-t[0][2]);
			} 
			else 
			{ 
				square = SOMVECTOR_SQRT(scalar(1)+t[2][2]-t[0][0]-t[1][1])*2;

				(*this)[2] = square*scalar(0.25); square = scalar(1)/square;

				(*this)[0] = square*(t[2][0]+t[0][2]); (*this)[1] = square*(t[1][2]+t[2][1]);
				(*this)[3] = square*(t[0][1]-t[1][0]);
			}
			return *this;
		}		

		//rotation about (a quaternion)
		template<int n, class T> inline vectorN &rotate(const vector<T,4> &q)
		{
			//REMINDER: this is 28 muls, 21 adds. it can be done in 17, 21 or vectorized

			//TODO: I Think {q[0],q[1],q[2], -q[3]} works??
			//TODO: I Think {q[0],q[1],q[2], -q[3]} works??
			//TODO: I Think {q[0],q[1],q[2], -q[3]} works??
			scalar cp[4], cq[4] = {-q[0],-q[1],-q[2], q[3]}; //conjugate
			int compile[n==3]; vector<S,3> &_this = ser<0,2>(n); vector<S,4> s4; //tricky
			Somvector::multiply(Somvector::multiply(q,_this,s4.map(cp)),s4.map(cq),_this);

			return *this;
		}
		template<int n, class T> inline vectorN &rotate(const T (&q)[4])
		{
			return rotate<n>(vector<T,4>::map(q));
		}

		//quaternion quaternion product
		template<class T> inline vectorN &premultiply(const vector<T,4> &t)
		{
			scalar cp[4]; return Somvector::multiply(map(cp).copy<4>(*this),t,*this);
		}
		template<class T> inline vectorN &postmultiply(const vector<T,4> &t)
		{				
			scalar cp[4]; return Somvector::multiply(t,map(cp).copy<4>(*this),*this);
		}
		template<class T> inline vectorN &premultiply(const T (&t)[4])
		{
			return premultiply(vector<T,4>::map(t));
		}
		template<class T> inline vectorN &postmultiply(const T (&t)[4])
		{				
			return postmultiply(vector<T,4>::map(t));
		}
	};

	//matrix (zero sized container)
	template<class S, int M, int N, int NN=N> class matrix
	{
	private: friend class Somvector; friend class matrix;

		typedef S scalarMxN[M][NN]; typedef matrix<S,M,N,NN> matrixMxN; 

		//these do not enforce const-ness and will conflict
		//with any new operators--which is not wanted anyway
		//[enforcing const-ness will break builtin operators]
		inline operator scalarMxN&(){ return *(scalarMxN*)this; }
		inline operator scalarMxN&()const{ return *(scalarMxN*)this; }

	protected: matrix<S,M,N,NN>(){ int compile[N<=NN]; } 
		
		matrix<S,M,N,NN>(const matrix<S,M,N,NN>&){ int compile[N<=NN]; }
		
	public:	typedef S scalar; static const size_t M = M, N = N, NN = NN;

		//sub element (single) selector
		template<int a, int aa> inline scalar &se() //const 
		{
			int compile[a<M&&aa<N]; return (*this)[a][aa];
		}
		template<int a, int aa> inline const scalar &se()const
		{
			int compile[a<M&&aa<N]; return (*this)[a][aa];
		}	

		//sub element range (serial) selector
		template<int a, int z, int aa, int zz> 
		inline  matrix<S,z-a+1,zz-aa+1,NN> &ser(int mm, int nn) //const
		{
			int compile[z-a<M&&zz-aa<N]; assert(mm==z-a+1&&nn==zz-aa+1); 
			
			return *(matrix<S,z-a+1,zz-aa+1,NN>*)&(*this)[a][aa];
		}
		template<int a, int z, int aa, int zz> 
		inline const matrix<S,z-a+1,zz-aa+1,NN> &ser(int mm, int nn)const 
		{
			int compile[z-a<M&&zz-aa<N]; assert(mm==z-a+1&&nn==zz-aa+1); 
			
			return *(matrix<S,z-a+1,zz-aa+1,NN>*)&(*this)[a][aa];
		}

		//sub matrix (shorthand) selector
		template<int W, int Z> inline matrix<S,W,Z,NN> &s() //const
		{
			return ser<0,W-1,0,Z-1>(W,Z); 
		}
		template<int W, int Z> inline const matrix<S,W,Z,NN> &s()const
		{
			return ser<0,W-1,0,Z-1>(W,Z); 
		}	

		//map a matrix to memory (static)
		#define SOMVECTOR_MATRIX_MAP_(T,Test,...) \
		template<int W, int Z> static inline __VA_ARGS__ &map(T)\
		{\
			int compile[W>=M&&Z>=N&&Test]; return *(__VA_ARGS__*)mem;\
		}\
		template<int W, int Z> static inline const __VA_ARGS__ &map(const T)\
		{\
			int compile[W>=M&&Z>=N&&Test]; return *(__VA_ARGS__*)mem;\
		}
		//(if you derive a subclass you may need to copy these)
		SOMVECTOR_MATRIX_MAP_(scalar (&mem)[W][Z],1,matrix<S,M,N,Z>)
		SOMVECTOR_MATRIX_MAP_(scalar (&mem)[W],N==1,matrix<S,M,N,Z>)
		SOMVECTOR_MATRIX_MAP_(scalar*const &mem,W*Z,matrix<S,M,N,Z>)
			
		//map acquisition (non-static)
		template<int W, int Z> inline scalarMxN &map() //const
		{
			int compile[W==M&&Z==NN]; return *this;  
		}
		template<int W, int Z> inline const scalarMxN &map()const
		{
			int compile[W==M&&Z==NN]; return *this; 
		}

		#define W_Z_T int W,int Z,class T //lazy: for legibility sake

		//WARNING: Traditionally set is used to setup the values of the matrix
		//That is NOT what is going on here. Unfortunately "objectifying" data 
		//introduces a question of semantics: Is the object copying the input?
		//Or is the input copying the object? Consistent language is important.

		//write matrix to memory 
		template<int m, int n, W_Z_T> inline void set(T (&t)[W][Z])const
		{
			int compile[m<=M&&m<=W&&n<=N&&n<=Z];		
			for(size_t i=0;i<m;i++) for(size_t j=0;j<n;j++) t[i][j] = (*this)[i][j]; 
		}	
		template<int m, int n, W_Z_T> inline void set(T t[W*Z])const
		{
			int compile[m<=M&&m<=W&&n<=N&&n<=Z];		
			for(size_t i=0;i<m;i++) for(size_t j=0;j<n;j++) t[Z*i+j] = (*this)[i][j]; 
		}	
		
		//matrix matrix copy assignment
		template<int m, int n, class T> inline matrixMxN &copy(const T &t)
		{	
			int compile[m<=M&&m<=t.M&&n<=N&&n<=t.N];
			for(size_t i=0;i<m;i++) for(size_t j=0;j<n;j++) (*this)[i][j] = t[i][j]; 		
			return *this;
		}	
		template<int m, int n, W_Z_T> inline matrixMxN &copy(const T (&t)[W][Z])
		{	
			int compile[m<=M&&m<=W&&n<=N&&n<=Z];
			for(size_t i=0;i<m;i++) for(size_t j=0;j<n;j++) (*this)[i][j] = t[i][j];
			return *this;
		}	
		template<int m, int n, W_Z_T> inline matrixMxN &copy(const T t[W*Z])
		{	
			int compile[m<=M&&m<=W&&n<=N&&n<=Z];
			for(size_t i=0;i<m;i++) for(size_t j=0;j<n;j++) (*this)[i][j] = t[W*i+j];
			return *this;
		}	
		
		//EXPERIMENTAL
		//standard construction methods		
		template<int m, int n, int how, class S, int Z> inline matrixMxN &rotation(const vector<S,Z> &Euler)
		{
			int compile1[M>=m&&N>=n&&Z==3&&m>=3&&n>=3]; //3x3, 3x4, 4x3
			int compile2[how=='xyz'||how=='zyx'||how=='yxz'||how=='zxy'];

			//SOM's angles seem inverted but it may very well be mistaken
			const scalar 
			cy = SOMVECTOR_COS(-Euler[0]), sy = SOMVECTOR_SIN(-Euler[0]),
			cp = SOMVECTOR_COS(-Euler[1]), sp = SOMVECTOR_SIN(-Euler[1]),
			cr = SOMVECTOR_COS(-Euler[2]), sr = SOMVECTOR_SIN(-Euler[2]);
			
			if(how=='zyx') //hard body inverse (-y) 
			{
				auto sysp = sy*sp;
				auto cysp = cy*sp;
				(*this)[0][0] = cp*cr;
				(*this)[0][1] = sysp*cr-cy*sr;
				(*this)[0][2] = cysp*cr+sy*sr;
				(*this)[1][0] = cp*sr;
				(*this)[1][1] = sysp*sr+cy*cr;
				(*this)[1][2] = cysp*sr-sy*cr;
				(*this)[2][0] = -sp;
				(*this)[2][1] = sy*cp;
				(*this)[2][2] = cy*cp;
			}
			else if(how=='xyz') //hard body animation (-y) 
			{
				/*WWW is no help, I'm (dyslexically) doing
				//these longhand

				Rx=| 1    0       0 | (449d80)
				   | 0  cosY  -sinY |
				   | 0  sinY   cosY |

				Ry=|  cosP  0  sinP | (449dc0)
				   |     0  1     0 |
				   | -sinP  0  cosP |

				Rz=| cosR  -sinR  0 | (449e00)
				   | sinR   cosR  0 |
				   |   0       0  1 |

				xy=|  cP      0       sP |
				   | -sY*-sP  cY  -sY*cP |
				   |  cY*-sP  sY   cY*cP |

				sYP = -sY*-sP
				-cYP = cY*-sP 
    
				xyz| cP*cR           cP*-sR             sP |
				   | sYP*cR+cY*sR    sYP*-sR+cY*cR  -sY*cP |
				   | -cYP*cR+sY*sR  -cYP*-sR+sY*cR   cY*cP |
				*/
				auto sysp = sy*sp; //-sy*-sp
				auto _cysp = cy*-sp;

				(*this)[0][0] = cp*cr;
				(*this)[0][1] = sysp*cr+cy*sr;
				(*this)[0][2] = _cysp*cr+sy*sr;
				(*this)[1][0] = cp*-sr;
				(*this)[1][1] = sysp*-sr+cy*cr;
				(*this)[1][2] = _cysp*-sr+sy*cr;
				(*this)[2][0] = sp;
				(*this)[2][1] = -sy*cp;
				(*this)[2][2] = cy*cp;
			}
			else if(how=='yxz') //object placement (449e40)
			{
				/*
				Ry=|...
				Rx=|...
				Rz=|...
				yx=| cP   sP*sY  sP*cY |
				   | 0    cY     -sY   |
				   | -sP  cP*sY  cP*cY |

				sYP = sY*sP
				cYP = cP*sY 
    
				yxz| cP*cR+sYP*sR  cP*-sR+sYP*cR   sP*cY |
				   | cY*sR         cY*cR            -sY   |
				   | -sP*cR+cYP*sR  -sP*-sR+cYP*cR  cP*cY |
				*/
				auto sysp = sy*sp; 
				auto cpsy = cp*sy;

				(*this)[0][0] = cp*cr+sysp*sr;
				(*this)[0][1] = cy*sr;
				(*this)[0][2] = -sp*cr+cpsy*sr;
				(*this)[1][0] = cp*-sr+sysp*cr;
				(*this)[1][1] = cy*cr;
				(*this)[1][2] = -sp*-sr+cpsy*cr;
				(*this)[2][0] = sp*cy;
				(*this)[2][1] = -sy;
				(*this)[2][2] = cy*cp;
			}
			else if(how=='zxy') //object placement inverse
			{
				/*
				Rz=|...
				Rx=|...
				Ry=|...
				zx=| cR  -sR*cY   -sR*-sY |
				   | sR  cR*cY  cR*-sY |
				   | 0   sY      cY |

				sRY = -sR*-sY
				cRY = cR*-sY 
    
				zxy| cR*cP+sRY*-sP  -sR*cY  cR*sP+sRY*cP |
				   | sR*cP+cRY*-sP  cr*cY  sR*sP+cRY*cP |
				   | cY*-sP  sY  cY*cP |
				*/
				auto srsy = sr*sy;
				auto crsy = cr*-sy;

				(*this)[0][0] = cr*cp+srsy*-sp;
				(*this)[0][1] = sr*cp+crsy*-sp;
				(*this)[0][2] = cy*-sp;
				(*this)[1][0] = -sr*cy;
				(*this)[1][1] = cy*cr;
				(*this)[1][2] = sy;
				(*this)[2][0] = cr*sp+srsy*cp;
				(*this)[2][1] = sr*sp+crsy*cp;
				(*this)[2][2] = cy*cp;
			}
			else assert(0);

			return _fill_4x4<m==4,n==4>();
		}				
		template<bool m, bool n> inline matrixMxN &_fill_4x4()
		{
			if(m) (*this)[3][0]=(*this)[3][1]=(*this)[3][2] = 0;
			if(n) (*this)[0][3]=(*this)[1][3]=(*this)[2][3] = 0;
			if(m&&n) (*this)[3][3] = 1; return *this;
		}
		template<int m, int n, int how, class S, int Z> inline matrixMxN &rotation(const S (&Euler)[Z])
		{
			return rotation<m,n,how>(vector<S,Z>::map(Euler)); //treat scalar arrays like Euler angles
		}

		//matrix quaternion copy conversion
		template<int m, int n, class T> inline matrixMxN &copy_quaternion(const vector<T,4> &q)
		{	
			int compile[m<=M&&m<=4&&n<=N&&n<=4/*2021*/&&m>=3&&n>=3];
			
			(*this)[0][0] = scalar(1)-scalar(2)*(q[1]*q[1]+q[2]*q[2]);
			(*this)[0][1] = scalar(2)          *(q[0]*q[1]+q[2]*q[3]);
			(*this)[0][2] = scalar(2)          *(q[0]*q[2]-q[1]*q[3]);			
			(*this)[1][0] = scalar(2)          *(q[0]*q[1]-q[2]*q[3]);
			(*this)[1][1] = scalar(1)-scalar(2)*(q[0]*q[0]+q[2]*q[2]);
			(*this)[1][2] = scalar(2)          *(q[1]*q[2]+q[0]*q[3]);
			(*this)[2][0] = scalar(2)          *(q[0]*q[2]+q[1]*q[3]);
			(*this)[2][1] = scalar(2)          *(q[1]*q[2]-q[0]*q[3]);			
			(*this)[2][2] = scalar(1)-scalar(2)*(q[0]*q[0]+q[1]*q[1]);

			return _fill_4x4<m==4,n==4>();
		}	
		template<int m, int n, class T> inline matrixMxN &copy_quaternion(const T (&q)[4])
		{
			return copy_quaternion<m,n>(Somvector::vector<T,4>::map(q));
		}

		//identity matrix
		template<int m, int n> inline matrixMxN &identity() 
		{				
			int compile[m<=M&&n<=N]; 
			for(size_t i=0;i<m;i++) for(size_t j=0;j<n;j++) (*this)[i][j] = i==j?1:0; 
			return *this;
		}
		template<int m> inline scalar trace()const
		{
			scalar sum = 0; for(size_t i=0;i<m;i++) sum+=(*this)[i][i]; return sum;
		}

		//matrix matrix product
		template<int m, int n, class T> inline matrixMxN &premultiply(const T &t)
		{
			scalar cp[m][N]; Somvector::multiply<m,n>(matrix<S,m,N>::map(cp).copy<m,N>(*this),t,*this);
			return *this;
		}
		template<int m, int n, class T> inline matrixMxN &postmultiply(const T &t)
		{
			scalar cp[M][n]; Somvector::multiply<m,n>(t,matrix<S,M,n>::map(cp).copy<M,n>(*this),*this);
			return *this;
		}

		//transpose matrix
		template<int m, int n> inline matrixMxN &transpose() 
		{
			scalar cp[n][m]; Somvector::transpose<m,n>(matrix<S,n,m>::map(cp).copy<n,m>(*this),*this);
			return *this;
		}

		//matrix inversion		
		template<int m, int n> inline matrixMxN &invert()
		{
			int compile[m<=M&&n<=N&&m==n]; scalar cof[m][n];
			
			for(size_t i=0;i<m;i++) for(size_t j=0;j<n;j++) cof[i][j] = cofactor<m,n>(i,j);
											   
			scalar rcpdet = vector<S,n>::map(cof[0]).dot<n>(vector<S,n>::map((*this)[0]));
			
			if(rcpdet) rcpdet = scalar(1)/rcpdet; //(there is a possible divide by zero here)

			for(size_t i=0;i<m;i++) for(size_t j=0;j<n;j++) (*this)[i][j] = cof[j][i]*rcpdet; 

			return *this;
		}
		template<int m, int n> inline scalar cofactor(int w, int z)const
		{
			int compile[m<=M&&n<=N&&m==n&&m>=2]; 

			scalar flip = w&1^z&1?-1:1; assert(w<m&&z<n);			

			if(m==2) return (*this)[!w][!z]*flip; //trivial
			
			const int l = m-1; //square matrix (m equals n)

			scalar sub[l][l]; int k = 0; //may not unroll so well

			for(size_t i=0;i<m;i++) for(size_t j=0;j<n;j++) 
			{
				if(i!=w&&j!=z) sub[0][k++] = (*this)[i][j]; //sub[l*l] 
			}			

			return matrix<S,l,l>::map(sub).determinant<l,l>()*flip;
		}
		template<int m, int n> inline scalar determinant()const
		{
			int compile[m<=M&&n<=N&&m==n&&m>=1&&m<=4]; 
			
			scalarMxN &_this = *this;
			switch(m) //column-major
			{
			case 1: return _this[0][0];
			case 2: return _this[0][0]*_this[1][1]
			              -_this[1][0]*_this[0][1];
			case 3: return _this[0][0]*_this[1][1]*_this[2][2]
						  			-_this[0][0]*_this[2][1]*_this[1][2]
			              +_this[1][0]*_this[2][1]*_this[0][2]
									  -_this[1][0]*_this[0][1]*_this[2][2]
			              +_this[2][0]*_this[0][1]*_this[1][2]
			              -_this[2][0]*_this[1][1]*_this[0][2]; 
			case 4: return _this[3][0]*_this[2][1]*_this[1][2]*_this[0][3]
			              -_this[2][0]*_this[3][1]*_this[1][2]*_this[0][3]
			              -_this[3][0]*_this[1][1]*_this[2][2]*_this[0][3]
			              +_this[1][0]*_this[3][1]*_this[2][2]*_this[0][3]
			              +_this[2][0]*_this[1][1]*_this[3][2]*_this[0][3]
			              -_this[1][0]*_this[2][1]*_this[3][2]*_this[0][3]
			              -_this[3][0]*_this[2][1]*_this[0][2]*_this[1][3]
			              +_this[2][0]*_this[3][1]*_this[0][2]*_this[1][3]
			              +_this[3][0]*_this[0][1]*_this[2][2]*_this[1][3]
			              -_this[0][0]*_this[3][1]*_this[2][2]*_this[1][3]
			              -_this[2][0]*_this[0][1]*_this[3][2]*_this[1][3]
			              +_this[0][0]*_this[2][1]*_this[3][2]*_this[1][3]
			              +_this[3][0]*_this[1][1]*_this[0][2]*_this[2][3]
			              -_this[1][0]*_this[3][1]*_this[0][2]*_this[2][3]
			              -_this[3][0]*_this[0][1]*_this[1][2]*_this[2][3]
			              +_this[0][0]*_this[3][1]*_this[1][2]*_this[2][3]
			              +_this[1][0]*_this[0][1]*_this[3][2]*_this[2][3]
			              -_this[0][0]*_this[1][1]*_this[3][2]*_this[2][3]
			              -_this[2][0]*_this[1][1]*_this[0][2]*_this[3][3]
			              +_this[1][0]*_this[2][1]*_this[0][2]*_this[3][3]
			              +_this[2][0]*_this[0][1]*_this[1][2]*_this[3][3]
			              -_this[0][0]*_this[2][1]*_this[1][2]*_this[3][3]
			              -_this[1][0]*_this[0][1]*_this[2][2]*_this[3][3]
									  +_this[0][0]*_this[1][1]*_this[2][2]*_this[3][3];

			default: return 0; //compiler
			}
		}
		
		//glFrustum
		template<int m, int n> inline matrixMxN &frustum
		(scalar left, scalar right, scalar bottom, scalar top, scalar znear, scalar zfar) 
		{
			int compile[M==4&&N==4&&m==4&&n==4];
						
			scalarMxN &_this = *this;
			//right handed (z-clipping is confused)
			_this[0][0] = (znear*2)/(right-left);
			_this[0][1] = 0;
			_this[0][2] = 0;
			_this[0][3] = 0;
			_this[1][0] = 0;
			_this[1][1] = (znear*2)/(top-bottom);
			_this[1][2] = 0;
			_this[1][3] = 0;
			_this[2][0] = (right+left)/(right-left);
			_this[2][1] = (top+bottom)/(top-bottom);
			//hmm: would prefer to go with OpenGL here???
			//_this[2][2] = -(zfar+znear)/(zfar-znear); //OpenGL
			_this[2][2] = zfar/(znear-zfar); //Direct3D
			_this[2][3] = -1;
			_this[3][0] = 0;
			_this[3][1] = 0;			
			//_this[3][2] = -(zfar*znear*2)/(zfar-znear); //OpenGL
			_this[3][2] = zfar*znear/(znear-zfar); //Direct3D
			_this[3][3] = 0;
			return *this;
		};
		template<int m, int n> //gluPerspective
		inline matrixMxN &perspective(scalar fovy, scalar aspect, scalar znear, scalar zfar) 
		{			
			scalar v = znear*SOMVECTOR_TAN(fovy>SOMVECTOR_PI+0.000003?fovy*SOMVECTOR_PI/360:fovy/2);

			scalar u = v*aspect; return frustum<m,n>(-u,u,-v,v,znear,zfar);
		};
	};
				
	//multiplication		
	template<int m, int n, class T, class U, class V> 
	static inline V &multiply(const T &t, const U &u, V &v) //matrix matrix product
	{
		return (V&)_multiply<m,n>(map(t),map(u),map(v));
	}
	template<int m, int n, class T, class U, class V> 
	static inline V &_multiply(const T &t, const U &u, V &v) 
	{
		int conform[u.M==t.N];
		int compile[m<=t.M&&n<=u.N&&m<=v.M&&n<=v.N]; 
		for(size_t i=0;i<m;i++) for(size_t j=0;j<n;j++) //row*column dot product
		{
			v[i][j] = 0; for(size_t k=0;k<u.N;k++) v[i][j]+=t[i][k]*u[k][j];
		}
		return v;
	}
	template<int n, class T, class U, class V> 
	static inline V &multiply(const T &t, const U &u, V &v) //vector matrix product
	{
		return (V&)_multiply<n>(map(t),map(u),map(v));
	}
	template<int n, class T, class U, class V> 
	static inline V &_multiply(const T &t, const U &u, V &v) 
	{	
		const int xx = n==v.N+1?v.N:n;	
		
		int conform[u.M-t.N<=1]; 
		int compile[xx<=v.N&&xx<=t.N&&u.M>=n]; 

		typedef typename T::scalar scalar;

		//homogeneous extension where t is 1 less u
		const scalar w = t.N==u.N?t[t.N-1]:scalar(1);

		for(size_t i=0;i<1;i++)
		for(size_t j=0;j<xx;j++) //row*column dot product
		{
			//2020: this isn't working... how has this
			//not been tested before now???
			//v[j] = xx!=n&&xx<t.N?w*u[xx][j]:0; //extended
			v[j] = xx!=n&&xx>=t.N?w*u[xx][j]:0; //extended
			
			for(size_t k=0;k<t.N;k++) v[j]+=t[k]*u[k][j];
		}

		//homogeneous extension where v is 1 more t
		if(xx==n&&t.N==v.N-1) v[n-1] = t.N==u.N?0:1; 
		
		return v;
	}	
	template<class T, class U, class V> 
	static inline V &multiply(const T &t, const U &u, V &v) //quaternions 
	{
		return (V&)_multiply(map(t),map(u),map(v));
	}
	template<class T, class U, class V> 
	static inline V &_multiply(const vector<T,4> &t, const U &u, V &v)
	{
		typedef typename U::scalar scalar;

		//u and v can be 3D vectors for implementing rotations
		int compile[u.N>=3]; const scalar u_3_ = u.N<4?0:u[3];

		if(v.N>=1) v[0] = t[3]*u[0]+t[0]*u_3_+t[1]*u[2]-t[2]*u[1];
		if(v.N>=2) v[1] = t[3]*u[1]+t[1]*u_3_+t[2]*u[0]-t[0]*u[2];
		if(v.N>=3) v[2] = t[3]*u[2]+t[2]*u_3_+t[0]*u[1]-t[1]*u[0];
		if(v.N>=4) v[3] = t[3]*u_3_-t[0]*u[0]-t[1]*u[1]-t[2]*u[2];

		return v;
	}

	//transposition 
	template<int m, int n, class T, class U> 
	static inline U &transpose(const T &t, U &u) //transpose matrix
	{
		int compile[m<=t.N&&n<=t.M&&m<=u.M&&n<=u.N]; 
		for(size_t i=0;i<m;i++) for(size_t j=0;j<n;j++) u[i][j] = t[j][i]; 
		return u;
	}
					  	
	//measurements
	//2018: using const so temporary pointer can be passed via T/U
	template<int m, typename T, class U>
	static inline typename T::scalar measure(const T &t, const U &u)
	{
		typename T::scalar cp[m]; return 
		Somvector::map(cp).copy<m>(t).remove<m>(u).length<m>();
	}
	template<int m, typename T, int N, class U>
	static inline T measure(const T (&t)[N], const U &u)
	{
		T cp[m]; return 
		Somvector::map(cp).copy<m>(t).remove<m>(u).length<m>();
	}	
	template<int m, typename T, class U>
	static inline T measure(const T*const &t, const U &u)
	{
		T cp[m]; return 
		Somvector::map(cp).copy<m>(t).remove<m>(u).length<m>();
	}	

	template<int N, class T>
	static inline vector<T,N> &map(T* &t){ return vector<T,N>::map<N>(t); } //const 
	template<int N, class T>
	static inline const vector<T,N> &map(const T* &t){ return vector<T,N>::map<N>(t); }
	template<int N, class T>
	static inline vector<T,N> &map(T (&t)[N]){ return vector<T,N>::map(t); } //const 	
	template<int N, class T>
	static inline const vector<T,N> &map(const T (&t)[N]){ return vector<T,N>::map(t); }

	template<int M, int N, class T>
	static inline matrix<T,M,N> &map(T* &t){ return matrix<T,M,N>::map<M,N>(t); } //const
	template<int M, int N, class T>
	static inline const matrix<T,M,N> &map(const T* &t){ return matrix<T,M,N>::map<M,N>(t); }
	template<int M, int N, class T>
	static inline matrix<T,M,N> &map(T (&t)[M][N]){ return matrix<T,M,N>::map(t); } //const
	template<int M, int N, class T>
	static inline const matrix<T,M,N> &map(const T (&t)[M][N]){ return matrix<T,M,N>::map(t); }
	template<int M, int N, int NN, class T>
	static inline matrix<T,M,N,NN> &map(T* &t){ return matrix<T,M,N>::map<M,NN>(t); } //const
	template<int M, int N, int NN, class T>
	static inline const matrix<T,M,N,NN> &map(const T* &t){ return matrix<T,M,N>::map<M,NN>(t); } 		
	template<int M, int N, int NN, class T, int U>
	static inline matrix<T,M,N,NN> &map(T (&t)[U][NN]){ return matrix<T,M,N>::map(t); } //const
	template<int M, int N, int NN, class T, int U>
	static inline const matrix<T,M,N,NN> &map(const T (&t)[U][NN]){ return matrix<T,M,N>::map(t); } 
		
	template<class T, int N> 
	static inline vector<T,N> &map(vector<T,N> &passthru){ return passthru; } //const 
	template<class T, int N> 
	static inline const vector<T,N> &map(const vector<T,N> &passthru){ return passthru; } 	
	template<class T, int M, int N, int NN> 
	static inline matrix<T,M,N,NN> &map(matrix<T,M,N,NN> &passthru){ return passthru; } //const 
	template<class T, int M, int N, int NN> 
	static inline const matrix<T,M,N,NN> &map(const matrix<T,M,N,NN> &passthru){ return passthru; } 

	//serialization (define as many as you need)
	template<int N, class T, class X, class Y, class Z> 
	static inline vector<T,3> &series(T (&t)[N], const X &m, const Y &n, const Z &z)
	{int compile[N>=3]; vector<T,3> &s = s.map(t); s[0] = m; s[1] = n; s[2] = z; return s;}
	template<int N, class T, class X, class Y, class Z> 
	static inline vector<T,3> &series(T t[N], const X &m, const Y &n, const Z &z)
	{int compile[N>=3]; vector<T,3> &s = s.map<N>(t); s[0] = m; s[1] = n; s[2] = z; return s;}
	template<int N, class T, class X, class Y, class Z, class W> 
	static inline vector<T,4> &series(T (&t)[N], const X &m, const Y &n, const Z &z, const W &w)
	{int compile[N>=4]; vector<T,4> &s = s.map(t); s[0] = m; s[1] = n; s[2] = z; s[3] = w; return s;}
	template<int N, class T, class X, class Y, class Z, class W> 
	static inline vector<T,4> &series(T t[N], const X &m, const Y &n, const Z &z, const W &w)
	{int compile[N>=4]; vector<T,4> &s = s.map<N>(t); s[0] = m; s[1] = n; s[2] = z; s[3] = w; return s;}
	//maybe this is cause for a recursive macro?
};

#endif //SOMVECTOR_INCLUDED
