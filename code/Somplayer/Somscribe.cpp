
#include "Somplayer.pch.h"

#include "Somscribe.h"
#include "Somenviron.h"

using namespace Somscribe_h;

bool Somscribe_h::cyclops::orient(Sominstant *wiz, Sominstant *eye)const
{
	Somthread_h::writelock wr = blink;

	if(!eye) eye=instant(); if(!wiz||!eye) return false;
	
	Som<skeleton> *pos = wiz->cast_support_magic(pos); //posture

	eye->copy<4,4>(pos?*pos:*wiz).se<3,1>()+=tripod; //upright

	Sominstant::matrix proj(fov,aspect,znear,zfar); //zfar

	eye->normalize().affinvert().premultiply<4,4>(proj);

	return true;
}

template<typename T>
inline size_t Somscribe_t(Sominstant *in, int *q, size_t q_s, T *get, size_t get_s)
{
	size_t out = 0; T *set = 0, *get_x = get+get_s;

#define GET(x) if(get+x<=get_x){ set = get; get+=x; }else return out;

	typedef Somvector::vector<T,4> scalar4;
	typedef Somvector::matrix<T,4,4> scalar4x4;

	Som<cyclops> *eye = dynamic_cast<Som<cyclops>*>(in); 
					 
	if(eye) for(out;out<q_s;out++) switch(q[out])
	{
	case Somenviron::SPHERE: GET(1)	*set = eye->znear; break;		
	case Somenviron::HORIZON: GET(1) *set = eye->zfar; break;	
	
	case Somenviron::EVENT: assert(0); return out; //wants "eye" location?

	case Somenviron::GLOBAL: case Somenviron::STATIC: GET(1) *set = 0; break;

	case Somenviron::ADDED: case Somenviron::REMOVED: assert(get_s==0); return 0;

	case Somenviron::SIGNAL: GET(4)

		scalar4::map<4>(set).copy<3>(eye->lens.light).se<3>()=1; break;

	case Somenviron::FRUSTUM: GET(16)

		scalar4x4::map<4,4>(set)
		.perspective<4,4>(eye->fov,eye->aspect,eye->znear,eye->zfar); break;	

	default: assert(0); return out;
	}

	return out;	
}

extern size_t Somscribe_f(Sominstant *in, int *q, size_t q_s, float *get, size_t get_s)
{
	return Somscribe_t<float>(in,q,q_s,get,get_s);
}

extern size_t Somscribe_d(Sominstant *in, int *q, size_t q_s, double *get, size_t get_s)
{
	return Somscribe_t<double>(in,q,q_s,get,get_s);
}