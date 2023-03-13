
//union PreND //PreServe.h
//{	
#if N==2

	PRESERVE_LIST(Pre2D)

	Pre2D():x(),y(){};
    Pre2D(double x, double y):x(x),y(y){}
    explicit Pre2D(double xy):x(xy),y(xy){}
    Pre2D(const Pre2D &cp):x(cp.x),y(cp.y){}
	Pre2D(PreNoTouching){}
	
	double n[2]; typedef Pre2D Ntype;

	//Ntype &operator=(double);
	inline void Set(double,double); 

#elif N==3

	PRESERVE_LIST(Pre3D)

	Pre3D():x(),y(),z(){}
    Pre3D(double x, double y, double z):x(x),y(y),z(z){}
    explicit Pre3D(double xyz):x(xyz),y(xyz),z(xyz){}
    Pre3D(const Pre3D &cp):x(cp.x),y(cp.y),z(cp.z){}
	Pre3D(PreNoTouching){}	
	double n[3]; typedef Pre3D Ntype;

	inline void Set(double,double,double);    
	inline Pre3D CrossProduct(const Pre3D&)const;

#elif N==4

	PRESERVE_LIST(Pre4D)

	Pre4D():x(),y(),z(),w(){}
    Pre4D(double x, double y, double z, double w):x(x),y(y),z(z),w(w){}
	explicit Pre4D(double xyzw):x(xyzw),y(xyzw),z(xyzw),w(xyzw){}
	explicit Pre4D(double rgb, double a):r(rgb),g(rgb),b(rgb),a(a){}
    Pre4D(const Pre4D &cp):x(cp.x),y(cp.y),z(cp.z),w(cp.w){}
	Pre4D(PreNoTouching){}	
	double n[4]; typedef Pre4D Ntype;

	inline void Set(double,double,double,double); 	

#else
#error
#endif
enum{ C2057=N };
#undef N
//enum{ N=sizeof(n)/sizeof(*n) }; //C2057
enum{ N=C2057 }; 

	//SymMul nomenclature is inherited from Assimp
	Ntype SymMul(const Ntype&)const, &Normalize();	

	DAEDALUS_DEPRECATED("please use !EpsilonCompare instead")
	bool Equal(const Ntype&, double epsilon=1e-6)const; //not implementing
	inline bool EpsilonCompare(const Ntype &o, double e/*=1e-15*/)const
	{ for(int i=0;i<N;i++) if(std::abs(n[i]-o.n[i])>e) return true; return false; }

	inline double Length()const, SquareLength()const, DotProduct(const Ntype&)const;    

	DAEDALUS_DEPRECATED("please use DotProduct instead (or SymMul)")
	inline double operator*(const Ntype &v1); //not implementing
		
	inline Ntype &operator+=(const Ntype&);
	inline Ntype &operator-=(const Ntype&);
	inline Ntype &operator*=(double);
	inline Ntype &operator/=(double s){ return *this*=(1/s); }
	inline friend Ntype operator*(const Ntype&,double);
	inline Ntype operator/(double s)const{ return *this*(1/s); }

	bool operator==(const Ntype&)const;
	bool operator!=(const Ntype&)const;

	DAEDALUS_DEPRECATED("please use n instead")
	//inline double &operator[](unsigned);  
	inline double operator[](unsigned)const;

	//ValidateDataStructure.cpp
	inline bool IsBlack()const;

	//avoid Assimp's specialization of std::min and std::max
	static inline Ntype Min(){ return Ntype(+std::numeric_limits<double>::max()); }
	static inline Ntype Max(){ return Ntype(-std::numeric_limits<double>::max()); }
	static inline PreBi<Ntype> MinMax(){ return PreBi<Ntype>(Min(),Max()); }
//};