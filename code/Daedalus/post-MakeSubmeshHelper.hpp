#ifdef _DEBUG
#include "Daedalus.(c).h" //IDE support
#endif
enum //MakeSubmesh
{ 
MakeSubmesh_SansBones=1,
};
struct MakeSubmeshHelper
{
	std::vector<signed> vMap;
	static const signed Unset = INT_MAX;	
	//Assimp/ProcessHelper.h/cpp MakeSubmesh
	//Fact: "I" contributed this with DeboneProcess.cpp	
	void FillMorph(const preMorph *superMorph, preMorph *newMorph, size_t numSubVerts)
	{									  
		//AI: create all the arrays for this mesh if spuper mesh has them

		newMorph->PositionsCoList().Reserve(numSubVerts);
		if(superMorph->HasNormals()) 
		newMorph->NormalsCoList().Reserve(numSubVerts);
		if(superMorph->HasTangentsAndBitangents()) 
		{ newMorph->TangentsCoList().Reserve(numSubVerts);
		newMorph->BitangentsCoList().Reserve(numSubVerts); }
		for(size_t i=0;superMorph->HasTextureCoords(i);i++) 
		{ newMorph->TextureCoordsCoList(i).Reserve(numSubVerts); }	
		for(size_t i=0;superMorph->HasVertexColors(i);i++) 
		newMorph->VertexColorsCoList(i).Reserve(numSubVerts);

		//AI: copy over data, but first generate faces with linear indices 

		auto pl = superMorph->PositionsCoList();
		for(size_t i=pl;i<pl.Size();i++) 
		{
			size_t nvi = vMap[i]; if(nvi==Unset) continue;			

			newMorph->positionslist[nvi] = pl[i];
			if(newMorph->normalslist) 
			newMorph->normalslist[nvi] = superMorph->normalslist[i];
			if(newMorph->tangentslist) 
			{ newMorph->tangentslist[nvi] = superMorph->tangentslist[i];
			newMorph->bitangentslist[nvi] = superMorph->bitangentslist[i]; }			
			for(size_t c=0;c<PreMorph::colorslistsN;c++) 
			if(newMorph->colorslists[c])
			newMorph->colorslists[c][nvi] = superMorph->colorslists[c][i];
			for(size_t c=0;c<PreMorph::texturecoordslistsN;c++) 
			if(newMorph->texturecoordslists[c])
			newMorph->texturecoordslists[c][nvi] = superMorph->texturecoordslists[c][i];			
		} 
	}
	std::vector<preN> subBones;	
	preMesh *operator()(const preMesh *superMesh, const std::vector<preN> subFacesIn, unsigned subFlags)
	{	
		PreSuppose(!PreMesh2::IsSupported);		
		
		//NEW: courtesy/future proofing
		auto l = superMesh->FacesList();
		size_t numSubFaces = subFacesIn.size();
		const preN *subFaces = subFacesIn.data();
		while(numSubFaces&&!l[subFaces[0]]&&!l[subFaces[1]]){ subFaces++; numSubFaces--; }		
		if(!numSubFaces||numSubFaces==1&&!l[subFaces[0]])
		return 0; //todo: contemplate non-face meshes
		
		vMap.assign(superMesh->Positions(),Unset);
		
		size_t numSubVerts = 0, numSubIndices = 0;		
		if(superMesh->HasIndices())	for(size_t i=0;i<numSubFaces;i++) 		
		superMesh->IndicesSubList(superMesh->faceslist[subFaces[i]])
		^[&](signed ea){ if(vMap[ea]==Unset) vMap[ea] = numSubVerts++; numSubIndices++; };

		//AI: create all the arrays for this mesh if spuper mesh has them

		preMesh *newMesh = new preMesh();
		newMesh->name = superMesh->name;
		newMesh->material = superMesh->material;
		newMesh->FacesList().Reserve(numSubFaces);
		newMesh->IndicesList().Reserve(numSubIndices);		

		//AI: copy over data, but first generate faces with linear indices 
		
		for(size_t i=0,j=0;i<numSubFaces;i++)
		{
			auto &superFace = l[subFaces[i]];			
			preFace &subFace = newMesh->faceslist[i] = superFace;
			if(subFace.polytype==PreFace::start)
			continue; else subFace.startindex = j; 
			superMesh->IndicesSubList(superFace)^[&](signed ea)
			{ newMesh->indiceslist[j++] = vMap[ea]; };
			switch(subFace.polytype)
			{ default: newMesh->_polygonsflag = true; break;
			case PreFace::line: newMesh->_linesflag = true; break;
			case PreFace::point: newMesh->_pointsflag = true; break;		
			case PreFace::triangle: newMesh->_trianglesflag = true; break;		
			case PreFace::start: assert(subFace.startindex==PreFace::start_polygons);
			}
		}

		//NEW: vertex attributes moved out
		auto ml = superMesh->MorphsList();
		auto mlNew = newMesh->MorphsList();
		FillMorph(superMesh,newMesh,numSubVerts);		
		mlNew.Reserve(ml.Size()); for(size_t i=ml;i<ml.Size();i++)
		{ mlNew[i] = new preMorph; FillMorph(ml[i],mlNew[i],numSubVerts); }		

		if(superMesh->HasBones())
		if(~subFlags&MakeSubmesh_SansBones)   
		{
			size_t newBones = 0;
			auto l = superMesh->BonesList(); 
			subBones.assign(l.Size(),0);
			for(size_t i=l;i<l.Size();i++)
			l[i]->WeightsList()^[&](const PreBone::Weight &ea)
			{ if(Unset!=vMap[ea]) subBones[i]++; };
			for(size_t i=l;i<l.Size();i++) 
			if(subBones[i]>0) newBones++; if(newBones) 
			{
				auto lNew = newMesh->BonesList(); lNew.Reserve(newBones);												
				size_t i,j,k; for(i=l,j=0;i<l.Size();i++) if(subBones[i])
				{
					preBone *newBone = new preBone;
					newBone->node = l[i]->node;	newBone->matrix = l[i]->matrix;
					auto kl = newBone->WeightsList(); kl.Reserve(subBones[i]);					
					k = 0; l[i]->WeightsList()^[&](const PreBone::Weight &ea)
					{ 
						if(Unset!=vMap[ea]) 
						kl[k++] = PreBone::Weight(vMap[ea],ea.value);
					};lNew[j++] = newBone;
				}assert(j==newBones);
			}
		}

		return newMesh;
	}
};
		

