				
#ifndef PRESERVE_HPP_INCLUDED
#define PRESERVE_HPP_INCLUDED

//Assimp/vector3.inl (etc.)
inline pre3D operator*(const PreMatrix::Matrix &m, const pre3D &v)
{ return pre3D(m.a1*v.x+m.a2*v.y+m.a3*v.z,m.b1*v.x+m.b2*v.y+m.b3*v.z,m.c1*v.x+m.c2*v.y+m.c3*v.z); }
inline pre3D operator*(const preMatrix &m, const pre3D &v)
{ return pre3D(m.a1*v.x+m.a2*v.y+m.a3*v.z+m.a4,m.b1*v.x+m.b2*v.y+m.b3*v.z+m.b4,m.c1*v.x+m.c2*v.y+m.c3*v.z+m.c4); }
inline double Pre3D::SquareLength()const{ return x*x+y*y+z*z; }
inline double Pre4D::SquareLength()const{ return x*x+y*y+z*z+w*w; }
inline double Pre3D::Length()const{ return std::sqrt(SquareLength()); }
inline double Pre2D::DotProduct(const pre2D &v2)const{ return x*v2.x+y*v2.y; }
inline double Pre3D::DotProduct(const pre3D &v2)const{ return x*v2.x+y*v2.y+z*v2.z; }
inline void Pre2D::Set(double X, double Y){ x = X; y = Y; }
inline void Pre3D::Set(double X, double Y, double Z){ x = X; y = Y; z = Z; }
inline void Pre4D::Set(double X, double Y, double Z, double W){ x = X; y = Y; z = Z; w = W; }
inline pre3D &Pre3D::Normalize(){ *this/=Length(); return *this; }
inline const pre3D &Pre3D::operator+=(const pre3D &o){ x+=o.x; y+=o.y; z+=o.z; return *this; }
inline const pre3D &Pre3D::operator-=(const pre3D &o){ x-=o.x; y-=o.y; z-=o.z; return *this; }
inline const pre3D &Pre3D::operator*=(double f){ x*=f; y*=f; z*=f; return *this; }
inline const pre3D &Pre3D::operator/=(double f){ x/=f; y/=f; z/=f; return *this; }
DAEDALUS_DEPRECATED("when is *= ever appropriate? Use v=m*v instead")
inline pre3D &operator*=(pre3D &v, const PreMatrix::Matrix &m); //not implementing
DAEDALUS_DEPRECATED("when is *= ever appropriate? Use v=m*v instead")
inline pre3D &operator*=(pre3D &v, const preMatrix& m); //not implementing
inline bool Pre3D::operator==(const pre3D &o)const{ return x==o.x&&y==o.y&&z==o.z; }
inline bool Pre3D::operator!=(const pre3D &o)const{ return x!=o.x||y!=o.y||z!=o.z; }
DAEDALUS_DEPRECATED("please avoid Assimp's specialization of std::min and std::max. Use PreMorph::GetContainer")
inline bool operator<(const Pre3D&,const pre3D&); //not implementing
inline pre3D Pre3D::SymMul(const pre3D &o)const{ return pre3D(x*o.x,y*o.y,z*o.z); }
inline pre2D operator+(const pre2D &v1, const pre2D &v2){ return pre2D(v1.x+v2.x,v1.y+v2.y); }
inline pre3D operator+(const pre3D &v1, const pre3D &v2){ return pre3D(v1.x+v2.x,v1.y+v2.y,v1.z+v2.z); }
inline pre4D operator+(const pre4D &v1, const pre4D &v2){ return pre4D(v1.x+v2.x,v1.y+v2.y,v1.z+v2.z,v1.w+v2.w); }
inline pre2D operator-(const pre2D &v1, const pre2D &v2){ return pre2D(v1.x-v2.x,v1.y-v2.y); }
inline pre3D operator-(const pre3D &v1, const pre3D &v2){ return pre3D(v1.x-v2.x,v1.y-v2.y,v1.z-v2.z); }
inline pre4D operator-(const pre4D &v1, const pre4D &v2){ return pre4D(v1.x-v2.x,v1.y-v2.y,v1.z-v2.z,v1.w-v2.w); }
DAEDALUS_DEPRECATED("please flip this around")
inline pre3D operator*(double f, const pre3D &v); //not implementing
inline pre3D operator*(const pre3D &v, double f){ return pre3D(f*v.x,f*v.y,f*v.z); }
inline pre3D operator/(const pre3D &v, double f){ return v*(1/f); }
inline pre3D operator/(const pre3D &v, const pre3D &v2){ return pre3D(v.x/v2.x,v.y/v2.y,v.z/v2.z); }
inline pre3D Pre3D::CrossProduct(const pre3D &v2)const{ return pre3D(y*v2.z-z*v2.y,z*v2.x-x*v2.z,x*v2.y-y*v2.x); }
DAEDALUS_DEPRECATED("please use CrossProduct instead")
inline pre3D operator^(const pre3D &v1, const pre3D &v2); //not implementing
inline pre3D operator-(const pre3D &v){ return pre3D(-v.x,-v.y,-v.z); }

//Assimp/color4.inl (etc.)
inline bool Pre3D::IsBlack()const{ return !EpsilonCompare(pre3D(),10e-3); };
inline bool Pre4D::IsBlack()const{ return !EpsilonCompare(pre4D(0,0,0,a),10e-3); };

//Assimp/matrix4x4.inl (etc.)
inline preMatrix PreMatrix::operator*(const preMatrix &m)const
{
	return preMatrix(
    m.a1 * a1 + m.b1 * a2 + m.c1 * a3 + m.d1 * a4,
    m.a2 * a1 + m.b2 * a2 + m.c2 * a3 + m.d2 * a4,
    m.a3 * a1 + m.b3 * a2 + m.c3 * a3 + m.d3 * a4,
    m.a4 * a1 + m.b4 * a2 + m.c4 * a3 + m.d4 * a4,
    m.a1 * b1 + m.b1 * b2 + m.c1 * b3 + m.d1 * b4,
    m.a2 * b1 + m.b2 * b2 + m.c2 * b3 + m.d2 * b4,
    m.a3 * b1 + m.b3 * b2 + m.c3 * b3 + m.d3 * b4,
    m.a4 * b1 + m.b4 * b2 + m.c4 * b3 + m.d4 * b4,
    m.a1 * c1 + m.b1 * c2 + m.c1 * c3 + m.d1 * c4,
    m.a2 * c1 + m.b2 * c2 + m.c2 * c3 + m.d2 * c4,
    m.a3 * c1 + m.b3 * c2 + m.c3 * c3 + m.d3 * c4,
    m.a4 * c1 + m.b4 * c2 + m.c4 * c3 + m.d4 * c4,
    m.a1 * d1 + m.b1 * d2 + m.c1 * d3 + m.d1 * d4,
    m.a2 * d1 + m.b2 * d2 + m.c2 * d3 + m.d2 * d4,
    m.a3 * d1 + m.b3 * d2 + m.c3 * d3 + m.d3 * d4,
    m.a4 * d1 + m.b4 * d2 + m.c4 * d3 + m.d4 * d4);
}
inline preMatrix PreMatrix::TransposeMatrix()const
{
	return preMatrix(a1,b1,c1,d1,a2,b2,c2,d2,a3,b3,c3,d3,a4,b4,c4,d4);
}
inline PreMatrix::Matrix PreMatrix::Matrix::TransposeMatrix()const
{
	return PreMatrix::Matrix(a1,b1,c1,a2,b2,c2,a3,b3,c3);
}
inline double PreMatrix::Matrix::Determinant()const
{
    return a1*b2*c3 - a1*b3*c2 + a2*b3*c1 - a2*b1*c3 + a3*b1*c2 - a3*b2*c1;
}
inline double PreMatrix::Determinant()const
{
    return a1*b2*c3*d4 - a1*b2*c4*d3 + a1*b3*c4*d2 - a1*b3*c2*d4
         + a1*b4*c2*d3 - a1*b4*c3*d2 - a2*b3*c4*d1 + a2*b3*c1*d4
         - a2*b4*c1*d3 + a2*b4*c3*d1 - a2*b1*c3*d4 + a2*b1*c4*d3
         + a3*b4*c1*d2 - a3*b4*c2*d1 + a3*b1*c2*d4 - a3*b1*c4*d2
         + a3*b2*c4*d1 - a3*b2*c1*d4 - a4*b1*c2*d3 + a4*b1*c3*d2
         - a4*b2*c3*d1 + a4*b2*c1*d3 - a4*b3*c1*d2 + a4*b3*c2*d1;
}
inline PreMatrix::Matrix PreMatrix::Matrix::InverseMatrix()const
{
	double det = Determinant(); if(det==0)
    {
        //AI: not really correct math but easy to see in a debugger 
		return PreMatrix::Matrix(std::numeric_limits<double>::quiet_NaN());
	}
    double invdet = 1/det; return PreMatrix::Matrix
	(invdet*(b2*c3-b3*c2),-invdet*(a2*c3-a3*c2), invdet*(a2*b3-a3*b2),
	-invdet*(b1*c3-b3*c1), invdet*(a1*c3-a3*c1),-invdet*(a1*b3-a3*b1),
	 invdet*(b1*c2-b2*c1),-invdet*(a1*c2-a2*c1), invdet*(a1*b2-a2*b1));
}
inline preMatrix PreMatrix::InverseMatrix()const
{
    double det = Determinant(); if(det==0)
    {
        //AI: not really correct math but easy to see in a debugger 
		return preMatrix(std::numeric_limits<double>::quiet_NaN());
	}
    double invdet = 1/det; return preMatrix
	(invdet*(b2*(c3*d4-c4*d3)+b3*(c4*d2-c2*d4)+b4*(c2*d3-c3*d2)),
	-invdet*(a2*(c3*d4-c4*d3)+a3*(c4*d2-c2*d4)+a4*(c2*d3-c3*d2)),
	 invdet*(a2*(b3*d4-b4*d3)+a3*(b4*d2-b2*d4)+a4*(b2*d3-b3*d2)),
	-invdet*(a2*(b3*c4-b4*c3)+a3*(b4*c2-b2*c4)+a4*(b2*c3-b3*c2)),
	-invdet*(b1*(c3*d4-c4*d3)+b3*(c4*d1-c1*d4)+b4*(c1*d3-c3*d1)),
	 invdet*(a1*(c3*d4-c4*d3)+a3*(c4*d1-c1*d4)+a4*(c1*d3-c3*d1)),
	-invdet*(a1*(b3*d4-b4*d3)+a3*(b4*d1-b1*d4)+a4*(b1*d3-b3*d1)),
	 invdet*(a1*(b3*c4-b4*c3)+a3*(b4*c1-b1*c4)+a4*(b1*c3-b3*c1)),
	 invdet*(b1*(c2*d4-c4*d2)+b2*(c4*d1-c1*d4)+b4*(c1*d2-c2*d1)),
	-invdet*(a1*(c2*d4-c4*d2)+a2*(c4*d1-c1*d4)+a4*(c1*d2-c2*d1)),
	 invdet*(a1*(b2*d4-b4*d2)+a2*(b4*d1-b1*d4)+a4*(b1*d2-b2*d1)),
	-invdet*(a1*(b2*c4-b4*c2)+a2*(b4*c1-b1*c4)+a4*(b1*c2-b2*c1)),
	-invdet*(b1*(c2*d3-c3*d2)+b2*(c3*d1-c1*d3)+b3*(c1*d2-c2*d1)),
	 invdet*(a1*(c2*d3-c3*d2)+a2*(c3*d1-c1*d3)+a3*(c1*d2-c2*d1)),
	-invdet*(a1*(b2*d3-b3*d2)+a2*(b3*d1-b1*d3)+a3*(b1*d2-b2*d1)),
	 invdet*(a1*(b2*c3-b3*c2)+a2*(b3*c1-b1*c3)+a3*(b1*c2-b2*c1)));
}
template<class T> //AI: from/to should be unit length
inline void PreMatrix::_FromToMatrix(const pre3D &from, const pre3D &to, T &out)
{
	PreSuppose(T::M>=3&&T::N>=3);

	const double e = from.DotProduct(to), f = (e<0)?-e:e;

    if(f>1-0.00001) //AI: "from" and "to"-vector almost parallel
    {
        pre3D u,v; //AI: temporary storage vectors
        //AI: vector most nearly orthogonal to "from"
		pre3D x(from.x>0?from.x:-from.x,from.y>0?from.y:-from.y,from.z>0?from.z:-from.z);
        if(x.x<x.y) if(x.x<x.z){ x.x = 1; x.y = x.z = 0; }else{ x.z = 1; x.y = x.z = 0; }
        else if(x.y<x.z){ x.y = 1; x.x = x.z = 0; }else{ x.z = 1; x.x = x.y = 0; }        
		u = x-from; v = x-to;
		const double c1 = 2/u.DotProduct(u);
        const double c2 = 2/v.DotProduct(v);
		const double c3 = u.DotProduct(v)*c1*c2;
        for(size_t i=0;i<3;out.m[i][i]+=1,i++)
		for(size_t j=0;j<3;j++) out.m[i][j] = -c1*u.n[i]*u.n[j]-c2*v.n[i]*v.n[j]+c3*v.n[i]*u.n[j];
    }
    else //AI: the most common case, unless "from"="to", or "from"=-"to"
    {
		const pre3D v = from.CrossProduct(to);
        //AI: optimization by Gottfried Chen (9 mults less)
        const double h = 1/(1+e); 
        const double hvx = h*v.x, hvxy = hvx*v.y;
        const double hvz = h*v.z, hvxz = hvx*v.z, hvyz = hvz*v.y;
        out.m[0][0] = e+hvx*v.x; out.m[0][1] = hvxy-v.z; out.m[0][2] = hvxz+v.y;
        out.m[1][0] = hvxy+v.z; out.m[1][1] = e+h*v.y*v.y; out.m[1][2] = hvyz-v.x;
        out.m[2][0] = hvxz-v.y; out.m[2][1] = hvyz+v.x; out.m[2][2] = e+hvz*v.z;
    }
}
inline PreMatrix::Matrix PreMatrix::Matrix::FromToMatrix(const pre3D &from, const pre3D &to)
{
	PreMatrix::Matrix out; PreMatrix::_FromToMatrix(from,to,out); return out;
}
inline preMatrix PreMatrix::FromToMatrix(const pre3D &from, const pre3D &to)
{
	preMatrix out; PreMatrix::_FromToMatrix(from,to,out);
	out.a4 = out.b4 = out.c4 = out.d1 = out.d2 = out.d3 = 0; out.d4 = 1; return out;
}
inline void PreMatrix::Decompose(pre3D &scaling, preQuaternion &rotation, pre3D &position)const
{	
    pre3D rows[3] = //will be modified below
	{pre3D(m[0][0],m[1][0],m[2][0]),pre3D(m[0][1],m[1][1],m[2][1]),pre3D(m[0][2],m[1][2],m[2][2])};
    scaling.Set(rows[0].Length(),rows[1].Length(),rows[2].Length());
    if(Determinant()<0) scaling*=-1;
	position.Set(m[0][3],m[1][3],m[2][3]);
    rows[0]/=scaling.x; rows[1]/=scaling.y; rows[2]/=scaling.z;
    rotation = preQuaternion(PreQuaternion::Matrix
	(rows[0].x,rows[1].x,rows[2].x,rows[0].y,rows[1].y,rows[2].y,rows[0].z,rows[1].z,rows[2].z));
}
inline PreMatrix::PreMatrix(const pre3D &scaling, const preQuaternion &rotation, const pre3D &position)
{
	rotation.GetMatrix(*this);	  
    a1*=scaling.x; a2*=scaling.x; a3*=scaling.x; a4 = position.x;
    b1*=scaling.y; b2*=scaling.y; b3*=scaling.y; b4 = position.y;
	c1*=scaling.z; c2*=scaling.z; c3*=scaling.z; c4 = position.z;	
}

//Assimp/quaternion.inl
inline PreQuaternion::PreQuaternion(const PreQuaternion::Matrix &qm)
{
    double s,t = qm.a1+qm.b2+qm.c3; if(t>0) //AI: large enough?
    { s = std::sqrt(1+t)*2; Set(s/4,(qm.c2-qm.b3)/s,(qm.a3-qm.c1)/s,(qm.b1-qm.a2)/s); } 
    else if(qm.a1>qm.b2&&qm.a1>qm.c3) //AI: Column 0?
    { s = std::sqrt(1+qm.a1-qm.b2-qm.c3)*2; Set((qm.c2-qm.b3)/s,s/4,(qm.b1+qm.a2)/s,(qm.a3+qm.c1)/s); }
    else if(qm.b2>qm.c3) //AI: Column 1?
    { s = std::sqrt(1+qm.b2-qm.a1-qm.c3)*2; Set((qm.a3-qm.c1)/s,(qm.b1+qm.a2)/s,s/4,(qm.c2+qm.b3)/s); } 
	else //AI: Column 2?
    { s = std::sqrt(1+qm.c3-qm.a1-qm.b2)*2; Set((qm.b1-qm.a2)/s,(qm.a3+qm.c1)/s,(qm.c2 + qm.b3)/s,s/4); }
}
inline void PreQuaternion::Set(double W, double X, double Y, double Z){ w = W; x = X; y = Y; z = Z; }
template<class T> inline void PreQuaternion::_GetMatrix(T &m)const
{
    m.a1 = 1-2*(y*y+z*z); m.a2 =   2*(x*y-z*w); m.a3 =   2*(x*z+y*w);
    m.b1 =   2*(x*y+z*w); m.b2 = 1-2*(x*x+z*z); m.b3 =   2*(y*z-x*w);
    m.c1 =   2*(x*z-y*w); m.c2 =   2*(y*z+x*w); m.c3 = 1-2*(x*x+y*y);
}
inline void PreQuaternion::GetMatrix(Matrix &m)const{ _GetMatrix(m); }
inline void PreQuaternion::GetMatrix(PreMatrix &m)const
{ _GetMatrix(m); m.a4 = m.b4 = m.c4 = m.d1 = m.d2 = m.d3 = 0; m.d4 = 1; }

//DeboneProcess, PretransformVertices & OptimizeGraph
inline void PreMesh::_ApplyTransform(const preMatrix &x)
{
	//BEWARE! ApplyTransform should multiply the bone
	//matrices by x.InverseMatrix, however that seems
	//to be undesirable in most if not every use case	
	//if(!x.IsIdentity()) //optimizing?
	if(HasPositions()) 
	{
		auto &f = [&](preMorph *p) //lambda
		{ for(size_t i=0;i<p->positions;i++) 
		p->positionslist[i] = x*p->positionslist[i];
		};f(this); if(HasMorphs()) for(size_t i=0;i<morphs;i++) f(morphslist[i]);
	}
	if(HasNormals()||HasTangentsAndBitangents()) 
	{
		auto xNormal = x.InverseTransposeMatrix();
		auto &f = [&](preMorph *p) //lambda
		{ if(p->HasNormals()) for(size_t i=0;i<p->positions;i++) 			
		p->normalslist[i] = (xNormal*p->normalslist[i]).Normalize();				
		if(p->HasTangentsAndBitangents()) for(size_t i=0;i<p->positions;i++) 
		{ p->tangentslist[i] = (xNormal*p->tangentslist[i]).Normalize();
		p->bitangentslist[i] = (xNormal*p->bitangentslist[i]).Normalize();
		}};f(this); if(HasMorphs()) for(size_t i=0;i<morphs;i++) f(morphslist[i]);
	}
}
inline void PreMesh::ApplyTransform(const preMatrix &x, int id)
{
	if(id==-1) id = x.IsIdentity(); if(!id) _ApplyTransform(x); 
}
//PROBABLY DUE TO BE OBSOLETE
//FindInstancesProcess, OptimizeMeshes & PretransformVertices
//PLEASE DO NOT RELY ON THESE BITS (as PretransformVertices had)
inline unsigned PreMesh::GetMeshVFormatUnique()const
{
    unsigned out = 1; //nonzero (call implies this mesh exists)
    if(HasNormals()) out|=0x2;
    if(HasTangentsAndBitangents()) out|=0x4;
	PreSuppose(8>=texturecoordslistsN&&8>=colorslistsN);
    for(size_t p=0;HasTextureCoords(p);p++)
    { out|=(0x100<<p); /*if(3==texturedimensions[p]) out|=(0x10000<<p);*/ }
    for(size_t p=0;HasVertexColors(p);p++) out|=(0x1000000<<p);
    return out;
}

inline void PreLight::ApplyTransform(const preMatrix &x)
{
	position = x*position;
	auto xNormal = x.InverseTransposeMatrix();
	direction = (xNormal*direction).Normalize();
}
inline void PreCamera::ApplyTransform(const preMatrix &x)
{
	position = x*position;
	auto xNormal = x.InverseTransposeMatrix();
	lookat = (xNormal*lookat).Normalize();
	up = (xNormal*up).Normalize();
}

//avoid Assimp's specialization of std::min and std::max
template<class T> inline void PreBi<T>::AddToContainer(const T &vec)
{
	for(int i=0;i<T::N;i++)
	{ 
		minima.n[i] = std::min(vec.n[i],minima.n[i]);
		maxima.n[i] = std::max(vec.n[i],maxima.n[i]); 
	}
};
inline Pre3D::Container PreMorph::GetContainer(Pre3D::Container minmax)const
{
	PositionsCoList()^[&](const pre3D &ea){ minmax.AddToContainer(ea); };
	return minmax;
}
inline Pre3D::Container PreMesh::GetMorphsContainer(Pre3D::Container minmax)const
{
	MorphsCoList()^[&minmax](const PreMorph *ea){ minmax = ea->GetContainer(minmax); };
	return GetContainer(minmax);
}

#endif //PRESERVE_HPP_INCLUDED