
//struct PreQuaternion //PreServe.h
//{
	PRESERVE_LIST(PreQuaternion)

	PreQuaternion():w(1),x(),y(),z(){}
    PreQuaternion(double pw, double px, double py, double pz):w(pw),x(px),y(py),z(pz){}	    
    explicit PreQuaternion(const PreQuaternion::Matrix&);
    PreQuaternion(double rotx, double roty, double rotz);
    PreQuaternion(const pre3D &axis, double angle);
	explicit PreQuaternion(const pre3D &normalized);
	PreQuaternion(PreNoTouching){}
	
	inline void Set(double w,double,double,double); 

    PreQuaternion::Matrix GetMatrix()const;

    bool operator==(const PreQuaternion&)const;
    bool operator!=(const PreQuaternion&)const;

	DAEDALUS_DEPRECATED("please use !EpsilonCompare instead")
    bool Equal(const PreQuaternion&, double epsilon=1e-6)const;

    PreQuaternion &Normalize();
    PreQuaternion &Conjugate();
    pre3D Rotate(const pre3D&);
	    
    PreQuaternion operator*(const PreQuaternion &mulitply)const;

    static void Interpolate(PreQuaternion &slerp, const PreQuaternion&, const PreQuaternion&, double);

	template<class T> inline void _GetMatrix(T&)const;
	inline void GetMatrix(Matrix &m)const, GetMatrix(PreMatrix &m)const;
//};