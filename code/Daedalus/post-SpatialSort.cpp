#include "Daedalus.(c).h"
using namespace Daedalus;
struct SpatialSort //Assimp/SpatialSort.cpp
{	
    SpatialSort(){} struct Entry
    {
		//AI: entry in a spatially sorted position array. Consists of a vertex 
		//index, position, and precalculated distance from the reference plane
        size_t index; pre3D position; double distance; Entry(){}
        Entry(size_t i, const pre3D &p, double d):index(i),position(p),distance(d){}
        inline bool operator<(const Entry &e)const{ return distance<e.distance; }
    };
	std::vector<Entry> entries; static pre3D planeNormal;	
	void Fill(const pre3D *p, size_t pcount)
	{	
		if(!p||!pcount) return;
		entries.resize(pcount);
		for(size_t a=0;a<pcount;a++)
		entries[a] = Entry(a,p[a],p[a].DotProduct(planeNormal));
		std::sort(entries.begin(),entries.end());
	}
	void SpatialSort::FindPositions(const pre3D &find, double radius, std::vector<signed> &out)const
	{
		const double dist = find.DotProduct(planeNormal);
		const double minDist = dist-radius, maxDist = dist+radius;
		out.clear();
		if(entries.empty()||maxDist<entries.front().distance||minDist>entries.back().distance) 
		return;

		//AI: do bin-search for the minimal distance to start the iteration there
		size_t index = entries.size()/2, binaryStepSize = entries.size()/4;
		for(;binaryStepSize>1;binaryStepSize/=2)
		if(entries[index].distance<minDist) index+=binaryStepSize; else index-=binaryStepSize;
		//AI: depending on the direction of the last step, single step a bit back
		//or forth to find the actual beginning element of the range
		while(index>0&&entries[index].distance>minDist) index--;
		while(index<(entries.size()-1)&&entries[index].distance<minDist) index++;
		//AI: now iterate until the first position is outside of radius.
		//add all positions inside of radius to the result array
		const double radiusSquared = radius*radius;
		for(size_t i=index;i<entries.size()&&entries[i].distance<maxDist;i++)
		{
			if((entries[i].position-find).SquareLength()<radiusSquared)
			out.push_back(entries[i].index);			
		}
	}			  
};
//AI: define the reference plane, arbitrarily choosing a vector away from most
//axes in the hope that no model spreads all of its vertices along this plane.
pre3D SpatialSort::planeNormal = pre3D(0.8523f,0.34321f,0.5736f).Normalize();

static bool SpatialSort_Compute(Daedalus::post *p) //Assimp/ProcessHelper.h
{
	p->Verbose("Generate spatially-sorted vertex cache");
	auto &_counter = p->SpatialSort_Compute._counter = 0;
	PreConst(p->Scene())->MeshesCoList()^[&](const preMesh *ea){ _counter+=1+ea->Morphs(); };
	SpatialSort *q = new SpatialSort[_counter];
	p->SpatialSort_Compute._cleanup = q;
	PreConst(p->Scene())->MeshesCoList()^[&](const preMesh *ea)
	{
		q++->Fill(ea->positionslist,ea->positions);
		ea->MorphsCoList()^[&](const preMorph *ea2){ q++->Fill(ea2->positionslist,ea2->positions); };
	};
	return true;
}
void Daedalus::post::_ComputeSpatialSortProcess::FindCommonPositions(size_t sort, const pre3D &pos, std::vector<signed> &out)
{
	if(sort>=_counter){ assert(0); out.clear(); } //paranoia
	else ((SpatialSort*)_cleanup)[sort].FindPositions(pos,configSeparationEpsilon,out);
}
static bool SpatialSort_Destroy(Daedalus::post *p) //Assimp/ProcessHelper.h
{
	delete[] (SpatialSort*)p->SpatialSort_Compute._cleanup; return true;
}
Daedalus::post::_ComputeSpatialSortProcess::_ComputeSpatialSortProcess
(post *p):Base(p,p->steps&steps?::SpatialSort_Compute:0)
{
	_cleanup = 0; _counter = 0; //paranoia
	configSeparationEpsilon = std::numeric_limits<double>::epsilon(); 
}
Daedalus::post::_DestroySpatialSortProcess::_DestroySpatialSortProcess
(post *p):Base(p,p->steps&steps?::SpatialSort_Destroy:0)
{}