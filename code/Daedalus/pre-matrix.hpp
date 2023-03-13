
//union PreMatrix //PreServe.h
//{
	//PreMatrix::Matrix
	typedef PreQuaternion::Matrix MMtype;

#if M==3

	PRESERVE_LIST(Matrix)

	double n[9], m[3][3]; 
	typedef Matrix Mtype; Matrix
	(double a1, double a2, double a3
	,double b1, double b2, double b3
	,double c1, double c2, double c3)
	:a1(a1),a2(a2),a3(a3), b1(b1),b2(b2),b3(b3), c1(c1),c2(c2),c3(c3){}
	Matrix(double qNaN){ for(size_t i=0;i<N;i++) n[i] = qNaN; }
	Matrix(){ new(this)Matrix(1,0,0,0,1,0,0,0,1); }		
	Matrix(PreNoTouching){}

#elif M==4

	PRESERVE_LIST(PreMatrix)

	double n[16], m[4][4];
	typedef PreMatrix Mtype; PreMatrix
	(double a1, double a2, double a3, double a4
	,double b1, double b2, double b3, double b4
	,double c1, double c2, double c3, double c4
	,double d1, double d2, double d3, double d4)
	:a1(a1),a2(a2),a3(a3),a4(a4), b1(b1),b2(b2),b3(b3),b4(b4)
	,c1(c1),c2(c2),c3(c3),c4(c4), d1(d1),d2(d2),d3(d3),d4(d4){}
	PreMatrix(double qNaN){ for(size_t i=0;i<N;i++) n[i] = qNaN; }
	PreMatrix(){ new(this)PreMatrix(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1); }
	PreMatrix(const pre3D &scaling, const preQuaternion &rotation, const pre3D &position);
	PreMatrix(PreNoTouching){}	
	//used after PRESERVE_ZEROTHISAFTER
	inline void _SetDiagonal(double l){ for(size_t i=0;i<M;i++) m[i][i] = l; }
	
	inline operator MMtype()const{ return MMtype(a1,a2,a3,b1,b2,b3,c1,c2,c3); }

	inline void DecomposeNoScaling(preQuaternion &rotation, pre3D &position)const;
	inline void Decompose(pre3D &scaling, preQuaternion &rotation, pre3D &position)const;		

#else
#error
#endif
enum{ C2057=M };
#undef M
//enum{ M=sizeof(m)/sizeof(*m) }; //C2057
enum{ M=C2057, N=M*M }; 		

	DAEDALUS_DEPRECATED("please use m instead")
	//double *operator[](unsigned);  //not implementing
	const double *operator[](unsigned)const; //not implementing

	DAEDALUS_DEPRECATED("please use !EpsilonCompare instead")
	bool Equal(const Mtype&, double epsilon=1e-6)const; //not implementing		
	inline bool EpsilonCompare(const Mtype &o, double e/*=1e-6*/)const
	{ for(int i=0;i<N;i++) if(std::abs(n[i]-o.n[i])>e) return true; return false; }
	//FYI: Assimp never used an epsilon for these operators
	inline bool operator==(const Mtype &cmp)const //memcmp?
	{ for(size_t i=0;i<N;i++) if(n[i]!=cmp.n[i]) return false; return true; }		
	inline bool operator!=(const Mtype &cmp)const{ return !(*this==cmp); }
	//NOTICE: Assimp used a fixed 10e-3f epsilon. Not here!
	bool IsIdentity()const{ static const Mtype cmp; return *this==cmp; }

	DAEDALUS_DEPRECATED("when is *= ever appropriate? Use a=b*a instead")
	Mtype &operator*=(const Mtype&); //not implementing
	inline Mtype operator*(const Mtype&)const;
	
	double Determinant()const;
	DAEDALUS_DEPRECATED("please use TransposeMatrix instead")
	Mtype &Transpose(); //not implementing
	inline Mtype TransposeMatrix()const;
	DAEDALUS_DEPRECATED("please use InverseMatrix instead")
	Mtype &Inverse(); //not implementing
	inline Mtype InverseMatrix()const;			
	inline MMtype InverseTransposeMatrix()const
	{
		//Ie. non-orthogonal normal transformation matrix
		return ((MMtype)*this).InverseMatrix().TransposeMatrix();	
	}
	
	//WARNING: used to take an output/return by address
	static inline Mtype Rotation(double a, const pre3D &axis);
	static inline Mtype RotationX(double a),RotationY(double a),RotationZ(double a);	
	static inline Mtype Translation(const pre3D &v),Scaling(const pre3D &v);
	static inline Mtype FromEulerAnglesXYZ(double x, double y, double z);
	static inline Mtype FromEulerAnglesXYZ(const pre3D &xyz);
	template<class T> //AI: from/to should be unit length
	static inline void _FromToMatrix(const pre3D &from, const pre3D &to, T &out);
	static inline Mtype FromToMatrix(const pre3D &from, const pre3D &to);
//};