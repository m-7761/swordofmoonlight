#include "Daedalus.(c).h"
using namespace Daedalus;
class FindDegenerates //Assimp/FindDegenerates.cpp
{
	Daedalus::post &post;		 

	std::vector<bool> removing;
	void Process(preMesh *mesh)
	{	
		int degens[2] = {0,0}; 				
		mesh->_polytypesflags = 0;		
		auto il = mesh->FacesCoList();
		auto pl = PreConst(mesh)->PositionsCoList();						
		if(post.FindDegenerates.configRemoveDegenerated)
		removing.assign(il.Size(),false);				
		for(size_t i=il;i<il.Size();i++)
		{	
			preFace &coface = il[i]; 
			bool degenerated = false;					
			auto pr = mesh->IndicesSubList(coface.PositionsRange());
			for(size_t j=0;j<coface.polytypeN;j++)
			{
				//AI: Polygons with more than 4 points are allowed to have double 
				//points, that is simulating polygons with holes just with concave 
				//polygons. However, double points cannot be directly after another
				size_t limit = coface.polytypeN;
				if(coface.polytypeN>4) limit = std::min(limit,j+2);

			///////////////////////////////////////////////////////////////
			//WON'T DO: REQUIRES A GLOBAL STORE OF TRULY UNIQUE POSITIONS//
			//OR WITHOUT VERBOSE FORMAT SHOULD JUST COMPARE THEIR INDICES//
			///////////////////////////////////////////////////////////////

				for(size_t k=j+1;k<limit;k++) if(pl[pr[j]]==pl[pr[k]])
				{	
					if(!degenerated) 
					{
						degenerated = true;
						degens[coface.line==coface.polytype]++;
						if(post.FindDegenerates.configRemoveDegenerated) 
						{
							removing[i] = true; goto remove_degenerated; 					
						}											 
					}                           

					//AI: upong finding a matching vertex position
					//remove the corresponding index from the array
					coface.polytypeN--; limit--;					
					for(size_t l=k--;l<coface.polytypeN;l++) pr[l] = pr[l+1];													
					PreSuppose(!PreMesh2::IsSupported);
				}
			}
			switch(coface.polytype) 
			{
			default: mesh->_polygonsflag = true; break;
			case PreFace::line: mesh->_linesflag = true; break;
			case PreFace::point: mesh->_pointsflag = true; break;		
			case PreFace::triangle: mesh->_trianglesflag = true; break;		
			case PreFace::start: assert(coface.startindex==PreFace::start_polygons);
			}
			remove_degenerated: continue;
		}

		if(degens[0]) post("Processed ")<<degens[0]<<" degenerated faces";
		if(degens[1]) post("Processed ")<<degens[1]<<" degenerated lines";

		//AI: remove degenerated faces
		//TODO: mark scene/mesh for unused feature removal (after processing)
		if(degens[0]+degens[1]&&post.FindDegenerates.configRemoveDegenerated) 
		{
			size_t counter = 0;	il^=[&](size_t i)
			{
				if(!removing[i]) mesh->CoCopy(il[counter++],il[i]); 
			};
			if(!il.ForgoMemory(counter)) il.Clear(); if(!counter)
			{
				//TODO: add some metadata explaining the situation
				//NOTE: here Assimp thows an exception foiling the import				
				post.CriticalIssue("Mesh is empty after removal of degenerated primitives");
			}
		}
	}

public: //FindDegenerates

	FindDegenerates(Daedalus::post *p)
	:post(*p){}operator bool()
	{
		post.progLocalFileName
		(!post.FindDegenerates.configRemoveDegenerated
		?"Simplifying \"Degenerated\" Faces and Lines"
		:"Removing \"Degenerated\" Faces and Lines");  
		post.Verbose("FindDegeneratedProcess begin"); 
		post.Scene()->MeshesCoList()^[=](preMesh *ea){ Process(ea); };
		post.Verbose("FindDegeneratedProcess finished");
		return true;
	}
};		
static bool FindDegenerates(Daedalus::post *post)
{
	return class FindDegenerates(post);
}							
Daedalus::post::FindDegenerates::FindDegenerates
(post *p):Base(p,p->steps&step?::FindDegenerates:0)
{
	configRemoveDegenerated = false; 
}