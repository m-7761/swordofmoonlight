#ifdef _DEBUG
#include "Daedalus.(c).h" //IDE support
#endif
struct MergeMeshesHelper //Assimp/SceneCombiner.h/cpp
{	
	typedef std::vector<const preMesh*>::iterator mesh_it;
	typedef	std::vector<std::pair<const preBone*,signed>> SuperBone;

	std::vector<SuperBone> boneBuffer;
	size_t FillBoneBuffer(size_t subBones, mesh_it it, mesh_it itt)
	{
		if(boneBuffer.size()<subBones) boneBuffer.resize(subBones);
		size_t out = 0;	for(signed pos=0;it!=itt;pos+=(*it)->Positions(),it++)  
		{
			auto il = (*it)->BonesList(); for(size_t i=il;i<il.Size();i++)
			{
				size_t j; for(j=0;j<out;j++)
				if(il[i]->node==boneBuffer[j][0].first->node)
				if(il[i]->matrix==boneBuffer[j][0].first->matrix)   
				{
					boneBuffer[j].push_back(std::make_pair(il[i],pos)); 
					break;
				}
				if(j==out) //AI: begin a new bone entry							
				boneBuffer[out++].assign(1,std::make_pair(il[i],pos));
			}
		}
		return out;
	}	
	void MergeBones(preMesh *out, size_t subBones, mesh_it begin, mesh_it end)
	{
		auto il = out->BonesList();
		il.Reserve(FillBoneBuffer(subBones,begin,end));		
		auto it = boneBuffer.begin();
		for(size_t i=il;i<il.Size();i++,it++)
		{		   			
			preBone *sup = il[i] = new preBone();
			sup->matrix = it->front().first->matrix; 
			sup->node.SetString(it->front().first->node); 
			//AI: Loop through all bones to be joined for this bone
			for(size_t i=0;i<it->size();i++) 
			sup->weights+=it->at(i).first->weights;	
			sup->WeightsList().Reserve(sup->weights);						
			//AI: copy the final weights - adjust the vertex IDs by 
			//the position count based offset of the corresponding mesh.
			auto avw = sup->WeightsList().Pointer();
			for(size_t i=0;i<it->size();i++)
			{
				signed pos = it->at(i).second;
				it->at(i).first->WeightsList()^[&](const PreBone::Weight &ea)
				{					
					avw->key = pos+ea; avw->value = ea.value; avw++;
				};
			}
		}
	}	 
	template<class T, class Co> //crazy: attrib is overloaded
	void MergeVAttribs(PreList<T*&,Co> (PreMorph::*attrib)(size_t),
	size_t n, preMorph *sup, signed morphKey, mesh_it begin, mesh_it end, T def)
	{
		size_t i = 0; auto il = (sup->*attrib)(n); for(auto it=begin;it!=end;it++) 
		{	
			const preMesh *base = *it; const preMorph *morph =
			morphKey>=0&&(size_t)morphKey<base->Morphs()?base->morphslist[morphKey]:base;
			size_t sz = base->Positions(); assert(sz);
			auto jl = (const_cast<preMorph*>(morph)->*attrib)(n); if(jl.HasList())
			{
				if(!il.Pointer()) //first time this attribute has appeared?
				{
					il.Reserve(sup->positions); if(i>0) //must do catch up?
					{
						size_t ii = i,i = 0; for(auto it=begin;it!=end;it++) 
						{
							auto l = (const_cast<preMesh*>(*it)->*attrib)(n);
							if(l.HasList()) l^[&](const T &ea){ il[i++] = ea; };
							else (*it)->PositionsCoList()^=[&](size_t){ il[i++] = def; };
						}
						assert(ii==i); while(i<ii) il[i++] = def;
					}
				}for(size_t j=0;sz--;il[i++]=jl[j++]); 
			}else if(!il.Pointer()) i+=sz; else while(sz--) il[i++] = def; 
		}assert(i==il.Size()||!il.HasList());		
		if(il.HasList()) while(i<il.Size()) il[i++] = def;
	}
	void MergeVertices(preMesh *out, size_t subMorphs, mesh_it begin, mesh_it end)
	{	
		assert(out->positions);
		out->MorphsList().Reserve(subMorphs);		
		preMorph *p = out; for(size_t i=0;i<=subMorphs;i++)
		{	
			signed mKey = (signed)i-1;
			if(i) p = out->morphslist[mKey] = new preMorph;
			const pre3D zero; const pre4D white(1);
			const pre3D qnan(std::numeric_limits<double>::quiet_NaN());
			if((*begin)->HasPositions()) 
			MergeVAttribs(&PreMorph::PositionsCoList,0,p,mKey,begin,end,zero);
			if((*begin)->HasNormals())
			MergeVAttribs(&PreMorph::NormalsCoList,0,p,mKey,begin,end,qnan);
			if((*begin)->HasTangentsAndBitangents())
			{ MergeVAttribs(&PreMorph::TangentsCoList,0,p,mKey,begin,end,qnan);
			MergeVAttribs(&PreMorph::BitangentsCoList,0,p,mKey,begin,end,qnan);
			}for(size_t i=0;(*begin)->HasTextureCoords(i);i++) 
			MergeVAttribs(&PreMorph::TextureCoordsCoList,i,p,mKey,begin,end,zero); 
			for(size_t i=0;(*begin)->HasVertexColors(i);i++) 
			MergeVAttribs(&PreMorph::VertexColorsCoList,i,p,mKey,begin,end,white);
			assert(p->positions);
		}
		assert(out->Positions());
	}
	preMesh *operator()(mesh_it begin, mesh_it end)
	{
		PreSuppose(!PreMesh2::IsSupported);

		assert(end-begin>1);
		preMesh *out = new preMesh();
		out->name.SetString((*begin)->name);
		out->material = (*begin)->material;		
		//AI: precompute storage requirements
		for(auto it=begin;it!=end;it++)
		{
			out->metadata+=(*it)->MetaData();
			out->positions+=(*it)->Positions(); 
			out->faces+=(*it)->Faces(); 
			out->indices+=(*it)->Indices();
			out->bones+=(*it)->Bones();
			out->morphs = std::max(out->morphs,(*it)->Morphs());
			out->_polytypesflags|=(*it)->_polytypesflags;
		}						
		if(out->metadata) //todo? mark somehow
		{
			auto il = out->MetaDataList(); il.Reserve(out->metadata);
			size_t i = 0; for(auto it=begin;it!=end;it++)
			{ (*it)->MetaDataList()^[&](preID ea){ il[i++] = ea; }; 
			}il&=i;
		}				
		if(out->faces)
		{
			auto il = out->FacesList(); il.Reserve(out->faces);
			size_t i = 0, pos = 0; for(auto it=begin;it!=end;it++)
			{ (*it)->FacesList()^[&](const preFace &ea)
			{ il[i] = ea; if(il[i].polytype) il[i].startindex+=pos; i++; }; pos+=(*it)->Positions();
			}il&=i;
		}		
		if(out->indices) 
		{
			auto il = out->IndicesList(); il.Reserve(out->indices);
			size_t i = 0, pos = 0; for(auto it=begin;it!=end;it++)
			{ (*it)->IndicesList()^[&](signed ea){ il[i++] = pos+ea; }; pos+=(*it)->Positions(); 
			}il&=i;
		}
		assert(!out->bones); //UNTESTED:
		//according to the Assimp code base, this subroutine is
		//not being entered, or some ai_assert clauses would've been raised if so
		MergeBones(out,out->bones,begin,end);		
		assert(0); //UNTESTED (it's a mess)	
		MergeVertices(out,out->morphs,begin,end);						
		
		//adding 0/removing const_iterator/adding const*
		for(auto it=begin;it!=end;it++){ delete *it; *it = 0; }
		return out;
	}	 
};