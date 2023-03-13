#include "Daedalus.(c).h"  
using namespace Daedalus;
class TriangulateProcess //Assimp/TriangulateProcess.cpp
{	
	Daedalus::post &post; 

	double const epsilon;

	//Assimp/PolyTools.h
	//AI: Compute the signed area of a triangle 
	static inline double GetArea2D(const pre2D &v1, const pre2D &v2, const pre2D &v3) 
	{ return 0.5*(v1.x*(v3.y-v2.y)+v2.x*(v1.y-v3.y)+v3.x*(v2.y-v1.y)); }
	//AI: Test if a given point p2 is on the left side of the line formed by p0-p1 	
	static inline bool OnLeftSideOfLine2D(const pre2D &p0, const pre2D &p1, const pre2D &p2)
	{ return GetArea2D(p0,p2,p1)>0; }
	//AI: Test if a given point is inside a given triangle
	static inline bool PointInTriangle2D(const pre2D &p0, const pre2D &p1, const pre2D &p2, const pre2D &pp)
	{	//AI: Point in triangle test using baryzentric coordinates
		const pre2D v0 = p1-p0, v1 = p2-p0, v2 = pp-p0;
		double dot00 = v0.DotProduct(v0), dot01 = v0.DotProduct(v1);
		double dot02 = v0.DotProduct(v2), dot11 = v1.DotProduct(v1), dot12 = v1.DotProduct(v2);		
		const double invDenom = 1/(dot00*dot11-dot01*dot01);
		dot11 = (dot11*dot02-dot01*dot12)*invDenom;	dot00 = (dot00*dot12-dot01*dot02)*invDenom;
		return dot11>0&&dot00>0&&dot11+dot00<1;
	}//AI: Compute the "Newell normal" of an arbitrary polygon
	static inline pre3D NewellNormal(const preN n_plus_2, pre3D *ngon) //PLUS 2 MORE
	{	//AI: duplicate the first two vertices at the end
		ngon[n_plus_2-2] = ngon[0]; ngon[n_plus_2-1] = ngon[1]; assert(n_plus_2>=6);														 
		pre3D out; for(auto p=ngon+1,d=p+n_plus_2-2;p<d;p++)		
		out+=pre3D(p[0].y*(p[1].z-p[-1].z),p[0].z*(p[1].x-p[-1].x),p[0].x*(p[1].y-p[-1].y));
		return out;	
	}

	void Process(preMesh *mesh)
	{
		//AI: Now we have _polytypesflags, 
		//so this is only here for test cases
		if(!mesh->_polytypesflags)
		{
			bool necessary = false;
			mesh->FacesList()^[&](const preFace &ea)
			{ if(ea.polytypeN>3) necessary = true; };
			if(!necessary) return;
		}
		else if(!mesh->_polygonsflag) return;
		
		////DEBUGGING APPARATUSES//////////////
		#ifdef AI_BUILD_TRIANGULATE_DEBUG_POLYS
		FILE *fout = fopen(POLY_OUTPUT_FILE,"a");
		#endif
		//AI: Apply vertex colors to represent the face winding?
		#ifdef AI_BUILD_TRIANGULATE_COLOR_FACE_WINDING
		auto cl = mesh->VertexColorsCoList(0);
		cl.Assign(mesh->Positions(),pre4D(0,0,0,1);
		#endif/////////////////////////////////
		
		preMesh jMesh; //ad-hoc RAII
		auto ol = mesh->FacesList();
		auto indices = mesh->IndicesList();
		jMesh.FacesList().Have(std::move(ol));
		auto il = PreConst(jMesh.FacesList());
		jMesh.IndicesList().Have(std::move(indices));					
		preN maxN = 0, maxFacesOut = 0, maxIndicesOut = 0;
		il^[&](const PreFace &ea)
		{
			if(ea.polytypeN>3) //polygon
			{
				preN tris = ea.polytypeN-2;
				maxFacesOut+=tris; maxIndicesOut+=tris*3;
				maxN = std::max(maxN,ea.polytypeN);
			}
			else{ maxFacesOut++; maxIndicesOut+=ea.polytypeN; }
		};
		assert(maxFacesOut!=il.Size()); //AI: _polytypesflags
		mesh->_trianglesflag = true; mesh->_polygonsflag = false;

		preN i,j,o = 0; ol.Reserve(maxFacesOut);
		preN index = 0; indices.Reserve(maxIndicesOut);				
		auto &triangulate = [&](signed a, signed b, signed c)
		{
			auto &of = ol[o++] = il[i];			
			of.startindex = index; of.polytype = PreFace::triangle; 
			indices[index++] = a; indices[index++] = b; indices[index++] = c;
		};//+2: NewellNormal, and for 2D?
		std::vector<pre3D> vbuffer(maxN+2);	//aliasing
		auto j3D = vbuffer.data(); auto j2D = (pre2D*)j3D;
		auto jEars = (bool*)&j2D[maxN+2]; //why not?
		auto pl = PreConst(mesh)->PositionsCoList(); for(i=il;i<il.Size();i++) 
		{
			auto jl = PreConst(jMesh).IndicesSubList(il[i]); 
			const preN jN = jl.Size(); 			

			////DEBUGGING APPARATUS///////////////////////
			//AI: Apply vertex colors to represent the face winding?
			#ifdef AI_BUILD_TRIANGULATE_COLOR_FACE_WINDING
			jl^[&](signed ea){ pre4D &c = cl[ea]; c.r = (e+1)/(double)jN; c.b = 1-c.r; };
			#endif////////////////////////////////////////

			if(jN<=3) //AI: if not a polygon just copy it
			{
				auto &of = ol[o++] = il[i];
				if(of) of.startindex = index;
				jl^[&](signed ea){ indices[index++] = ea; };
				continue;
			}
			else if(jN==4) //AI: optimized code for quadrilaterals
			{
				//AI: quads can have at most one concave vertex. Determine 
				//this vertex (if it exists) and start tri-fanning from it
				for(j=0;j<4;j++) 
				{
					const pre3D &v = pl[jl[j]], &v0 = 
					pl[jl[(j+3)%4]], &v1 = pl[jl[(j+2)%4]], &v2 = pl[jl[(j+1)%4]];
					pre3D left = (v0-v), diag = (v1-v), right = (v2-v);
					left.Normalize(); diag.Normalize(); right.Normalize();
					if(prePi<std::acos(left.DotProduct(diag))+std::acos(right.DotProduct(diag)))
					break; //AI: using this concave point
				}if(j==4) j = 0; //then use just any point
				triangulate(jl[j],jl[(j+1)%4],jl[(j+2)%4]);
				triangulate(jl[j],jl[(j+2)%4],jl[(j+3)%4]);			
				continue;
			}//AI: A polygon with more than 3 vertices can be either concave or convex.
			//Usually everything we're getting is convex and we could easily
			//triangulate by tri-fanning. However, LightWave is probably the only
			//modeling suite to make extensive use of highly concave, monster polygons ...
			//so we need to apply the full 'ear cutting' algorithm to get it right.
			//
			//RERQUIREMENT: polygon is expected to be simple and *nearly* planar.
			//We project it onto a plane to get a 2d triangle.

			//AI: Get Newell normal of the polygon
			for(j=0;j<jN;j++) j3D[j] = pl[jl[j]]; 				
			const pre3D nn = NewellNormal(jN+2,j3D); 
			//AI: Select largest normal coordinate to ignore for projection
			const pre3D a(nn.x>0?nn.x:-nn.x,nn.y>0?nn.y:-nn.y,nn.z>0?nn.z:-nn.z);
			double inv = nn.z;
			int ac = 0, bc = 1; //AI: no z coord. projection to xy
			if(a.x<=a.y) //AI: no y coord. projection to zy 
			{ ac = 2; bc = 0; inv = nn.y; }
			else if(a.x>a.z) //AI: no x coord. projection to yz
			{ ac = 1; bc = 2; inv = nn.x; }
			//AI: Swap projection axes to take the negated projection vector into account
			if(inv<0) std::swap(ac,bc);
			for(j=0;j<jN;jEars[j++]=false) 
			{ j2D[j].x = pl[jl[j]].n[ac]; j2D[j].y = pl[jl[j]].n[bc]; }

			////DEBUGGING APPARATUS///////////////////
			//AI: plot the plane onto which we mapped the polygon to a 2D ASCII pic
			#ifdef AI_BUILD_TRIANGULATE_DEBUG_POLYS				
			pre2D::Container minmax = Pre2D::MinMax();
			for(int i=0;i<jN;i++) minmax.AddToContainer(j2D[i]);
			char grid[POLY_GRID_Y][POLY_GRID_X+POLY_GRID_XPAD];
			std::fill_n((char*)grid,POLY_GRID_Y*(POLY_GRID_X+POLY_GRID_XPAD),' ');
			for(int i=0;i<jN;i++) 
			{	const pre2D v = (j2D[i]-minmax.minima)/(minmax.maxima-minmax.minima);
				const int x = v.x*(POLY_GRID_X-1), y = v.y*(POLY_GRID_Y-1);
				char *loc = grid[y]+x; 
				if(grid[y][x]!=' '){ while(*loc!=' ') loc++; *loc++ = '_'; }
				*(loc+sprintf(loc,"%i",i)) = ' ';
			}for(int y=0;y<POLY_GRID_Y;y++) 
			{	grid[y][POLY_GRID_X+POLY_GRID_XPAD-1] = '\0';
				fprintf(fout,"%s\n",grid[y]);
			}fprintf(fout,"\ntriangulation sequence: ");
			#endif/////////////////////////////////////

			//USED BELOW TO FINALIZE THE TRIANGULATION
			const preN output0 = o; preN &outputN = o; 

			//AI: currently this is the slow O(kn) variant with a worst case
			//complexity of O(n^2) (I think). Can be done in O(n).				
			preN allEars = jN, two_many_times = 0;
			for(preN ear=0,prev=jN-1,next=0;allEars>3;two_many_times=0)
			{
				//AI: try to find a new ear on this face
				for(ear=next;/*yuck*/;prev=ear,ear=next) 
				{
					//for the record, this loop is original Assimp code!!					
					for(next=ear+1;jEars[(next>=jN?next=0:next)];next++);
						
					//AI: break after we looped two times without a positive match						
					//CAN PASSES NOT BE COUNTED IN THE FOR STATEMENT'S MIDDLE CLAUSE?
					if(next<ear&&++two_many_times==2) break; 

					const pre2D &v1 = j2D[ear], &v0 = j2D[prev], &v2 = j2D[next];

					//AI: Must be a convex point. Assuming ccw winding, 
					//it must be on the right of the line between p-1 and p+1
					if(OnLeftSideOfLine2D(v0,v2,v1)) continue;

					//AI: and no other point may be contained in this triangle
					for(j=0;j<jN;j++)
					{
						//AI: We need to compare the actual values because it's possible that multiple indexes in
						//the polygon are referring to the same position. concave_polygon.obj is a sample
						////HISTORICAL NOTE////
						//TODO? Use EpsilonCompare instead. Due to numeric inaccuracies in
						//PointInTriangle() I'm guessing that it's actually possible to construct
						//input data that would cause us to end up with no ears. The problem is,
						//which epsilon? If we chose a too large value, we'd get wrong results
						const pre2D &v = j2D[j];
						if(PointInTriangle2D(v0,v1,v2,v))
						if(v.EpsilonCompare(v0,epsilon)&&v.EpsilonCompare(v1,epsilon)&&v.EpsilonCompare(v0,epsilon)) 
						break;							
					}if(j==jN) break; //AI: this vertex is an ear
				}
				if(2==two_many_times) //failed to find an ear
				{
					//AI: Due to the 'two ear theorem', every simple polygon with more than three points must
					//have 2 'ears'. Here's definitely someting wrong ... but we don't give up yet.
					//
					//Instead we're continuing with the standard tri-fanning algorithm which we'd
					//use if we had only convex polygons. That's life.
					post.CriticalIssue("Failed to triangulate polygon (no ear found). Probably not a simple polygon?");						
						
					////DEBUGGING APPARATUS////////////////
					#ifdef AI_BUILD_TRIANGULATE_DEBUG_POLYS
					fprintf(fout,"critical error here, no ear found! ");
					#endif/////////////////////////////////

					//ASSUMING THIS IS A BUG IN THE ORIGINAL Assimp CODE
					//allEars = 0; break;

					o-=(jN-allEars); //AI: undo all previous work
					for(j=0;j<jN-2;j++) triangulate(0,j+1,j+2);
					allEars = 0; break;
				}
				else triangulate(prev,ear,next);
				//AI: exclude the ear from most further processing
				jEars[ear] = true; allEars--;
			}
			if(allEars>0) //AI: should be the final triangle/ear
			{	
				for(j=0;jEars[j];j++); signed a = j;
				for(j++;jEars[j];j++); signed b = j;
				for(j++;jEars[j];j++); signed c = j;					
				triangulate(a,b,c);
				assert(allEars==1); 
			}
			else assert(2==two_many_times); //can this ever be?

			////DEBUGGING APPARATUS////////////////
			#ifdef AI_BUILD_TRIANGULATE_DEBUG_POLYS
			for(preN o=output0;o<outputN;o++) 
			{	auto l = PreConst(mesh)->SubIndicesList(ol[o]);
				fprintf(fout," (%i %i %i)",l[0],l[1],l[2]);
			}fprintf(fout,"\n*********************************************************************\n");	fflush(fout);
			#endif/////////////////////////////////

			//FINALIZE TRIANGLES & INDICES
			for(preN o=output0;o<outputN;) //shadowing
			{
				auto l = mesh->IndicesSubList(ol[o]);								
				if(epsilon<std::abs(GetArea2D(j2D[l[0]],j2D[l[1]],j2D[l[2]])))
				{
					o++; l^[&](signed &ea){ ea = jl[ea]; }; continue;
				}
				else outputN--; //AI: drop zero-area triangles
				post.Verbose("Omitting \"degenerated\" output triangle");					
				for(preN o2=o;o2<outputN;o2++) ol[o2].startindex-=3; index-=3; 
				memmove(l.Pointer(),l.Pointer()+3,3*(outputN-o)*sizeof(l[0]));					
			}
		}

		////DEBUGGING APPARATUS////////////////
		#ifdef AI_BUILD_TRIANGULATE_DEBUG_POLYS
		fclose(fout);
		#endif/////////////////////////////////

		ol.ForgoMemory(o); indices.ForgoMemory(index); 
		log = true;
	}
	bool log;

public: //TriangulateProcess

	TriangulateProcess(Daedalus::post *p)
	:epsilon(p->SpatialSort_Compute.configSeparationEpsilon)
	,post(*p),log(){}operator bool()
	{	
		post.progLocalFileName("Converting Polygons into Triangles");

		post.Verbose("TriangulateProcess begin");
		post.Scene()->MeshesList()^[=](preMesh *ea){ Process(ea); };
		if(log) post("TriangulateProcess finished. All polygons have been triangulated.");
		else post.Verbose("TriangulateProcess finished. There was nothing to do.");
		return true;
	}
};
static bool TriangulateProcess(Daedalus::post *post)
{
	return class TriangulateProcess(post);
}							
Daedalus::post::Triangulate::Triangulate
(post *p):Base(p,p->steps&step?::TriangulateProcess:0)
{}
