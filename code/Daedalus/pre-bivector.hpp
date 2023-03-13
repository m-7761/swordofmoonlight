
//template<class T> union PreBi //PreServe.h
//{	
	PRESERVE_LIST(PreBi)

	enum{ V=2 };
	double m[T::N][V],n[V*T::N]; 
	struct{ T v[V]; }; //C2621
	typedef PreBi<T> Wtype; 	
	PreBi():a(),b()
	{ PreSuppose(sizeof(*this)==2*sizeof(f)); }
	PreBi(const T &f, const T &s):a(f),b(s)
	{ PreSuppose(sizeof(*this)==2*sizeof(f)); }
	PreBi(const Wtype &cp):a(cp.a),b(cp.b){}				
	Wtype &operator=(const Wtype &cp){ a = cp.a; b = cp.b; return *this; }

	inline void AddToContainer(const T&);
//};
