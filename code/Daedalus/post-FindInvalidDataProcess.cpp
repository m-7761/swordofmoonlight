#include "Daedalus.(c).h"  
using namespace Daedalus;
class FindInvalidDataProcess //Assimp/FindInvalidDataProcess.cpp
{	
	Daedalus::post &post; 

	std::vector<preID> meshMapping;
	static const preID Deleted = -1;
	void RecursivelyRebuildMeshIDs(preNode *node)
	{			
		auto il = node->OwnMeshesList();
		size_t counter = 0; il^=[&](size_t i)
		{
			preID meshID = meshMapping[il[i]]; 
			if(meshID!=Deleted) il[counter++] = meshID;
		};
		if(!il.ForgoMemory(counter)) il.Clear();
		node->NodesList()^[=](preNode *ea){ RecursivelyRebuildMeshIDs(ea); };
	}

	const double aniEpsilon; //ANIMATIONS
	inline bool EpsilonCompare(double n, double s)
	{
		return std::abs(n-s)>aniEpsilon;
	}		   
	inline bool EpsilonCompare(const Pre3D::Time &n, const Pre3D::Time &s) 
	{
		return EpsilonCompare(n.value.x,s.value.x) 
		&&EpsilonCompare(n.value.y,s.value.y)&&EpsilonCompare(n.value.z,s.value.z);
	}
	inline bool EpsilonCompare(const PreQuaternion::Time &n, const PreQuaternion::Time &s)   
	{
		return EpsilonCompare(n.value.x,s.value.x)&&EpsilonCompare(n.value.y,s.value.y)
		&&EpsilonCompare(n.value.z,s.value.z)&&EpsilonCompare(n.value.w,s.value.w);
	}
	template<typename T>
	inline bool AllIdentical(T *in, size_t num)
	{
		if(num>1) if(aniEpsilon>0) for(size_t i=0;i<num-1;i++)
		{
			if(!EpsilonCompare(in[i],in[i+1])) return false;
		}
		else for(size_t i=0;i<num-1;i++) if(in[i]!=in[i+1]) return false;
		return true;
	}	
	template<class L>
	bool CollapseDummies(L l, PreAnimation::Skin *skin, double duration)
	{
		//AI: Check if all values are identical 
		if(l.RealSize()>1&&AllIdentical(l.Pointer(),l.Size()))
		{	//NEW: what about default edge cases?
			//Not necessary to consider values of 0, as values
			//are absolute relative to the matrix, and not looking for nodes!!
			auto t = l.Front(), t2 = l.Back(); 
			bool keepstart = t>0&&skin->startcase==skin->matrixofnode;			
			bool keepend = t2<duration&&skin->endcase==skin->matrixofnode;        
			size_t n = keepstart&&keepend?2:1; if(n<l.Size())
			{
				if(keepend&&!keepstart) std::swap(t,t2);
				l.Reserve(n); l.Front() = t; if(n==2) l.Back() = t2;
				return true;
			}
		}
		return false;
	}
	void Process(preAnimation *anim)
	{
		anim->SkinChannelsList()^[=](PreAnimation::Skin *ea)
		{
			if(CollapseDummies(ea->ScaleKeysList(),ea,anim->duration)
			+CollapseDummies(ea->PositionKeysList(),ea,anim->duration)
			+CollapseDummies(ea->RotationKeysList(),ea,anim->duration))
			{  	//this: VS2010 (name-conflicts bug)
				this->post("Collapsed dummy animations to one-or-two keys");
				this->log = true; 
			}
		};//hack: can probably use more thought/be done better
		anim->MorphChannelsList()^[=](PreAnimation::Morph *ea)
		{
			if(ea->MorphKeys()>1&&AllIdentical(ea->morphkeyslist,ea->morphkeys))
			ea->MorphKeysList().Assign(1,ea->morphkeyslist);
		};
	}		
	
	std::vector<bool> dirtyMask;
	inline const char *GetFirstError3D
	(const pre3D *arr, size_t size, bool mayBeMote, bool mayBeZero)
	{
		if(!arr) return 0; bool mote = true; //b?
		size_t counter = 0; for(size_t i=0;i<size;i++) if(!dirtyMask[i])
		{
			const pre3D &v = arr[i];
			if(!mayBeZero&&!v.x&&!v.y&&!v.z) return "Found zero-length vector";
			if(!_finite(v.x)||!_finite(v.y)||!_finite(v.z)) return "INF/NAN was found in a vector component";			
			counter++; if(i&&v!=arr[i-1]) mote = false; 
		}
		if(counter>1&&mote&&!mayBeMote) return "All vectors are identical"; return 0;
	}	
	bool Process3D(pre3D* &in, size_t num, const char *name, bool mayBeMote=false, bool mayBeZero=true)
	{
		const char *err = GetFirstError3D(in,num,mayBeMote,mayBeZero);
		if(!err) return false;
		post.CriticalIssue("FindInvalidDataProcess fails on mesh ")<<name<<": "<<err;
		delete[] in; in = 0; return true;
	}
	static const int Delete = 2; int Process(preMesh *mesh)
	{
		PreSuppose(!PreMesh2::IsSupported);
		//AI: Ignore elements that are not referenced by vertices.
		//(they are, for example, caused by the FindDegenerates step)		
		dirtyMask.assign(mesh->Positions(),mesh->HasFaces());
		if(dirtyMask.empty()){ assert(0); goto Delete; } //optimizing
		if(mesh->HasIndices()) mesh->FacesList()^[&](const preFace &ea)
		{
			mesh->IndicesSubList(ea)^[&](signed i){ dirtyMask[i] = false; };
		};
		if(Process3D(mesh->positionslist,mesh->Positions(),"positions")) Delete:
		{
			post.CriticalIssue("Deleting mesh: Unable to continue without vertex positions");
			return Delete;
		}
		bool ret = false;
		mesh->MorphsCoList()^[&](preMorph *ea) //morphs may collapse to mote
		{ 
			if(Process3D(ea->positionslist,ea->Positions(),"positions",true))
			ret = true;
		};
		mesh->ForEach([&](preMorph *ea)
		{
			for(size_t i=0;i<PreMesh::texturecoordslistsN;i++)    		
			if(Process3D(ea->texturecoordslists[i],ea->TextureCoords(),"texturecoords"))    
			{
				//AI: delete all subsequent texture coordinate sets.
				while(++i<PreMesh::texturecoordslistsN) ea->TextureCoordsCoList(i).Clear();
				ret = true; break;
			}
		});
	  
		//tangents should imply normals, but just to be thorough
		if(mesh->HasNormals()||mesh->HasTangentsAndBitangents())
		{
			//AI: Normals and tangents are undefined for point and line faces.
			if(mesh->_pointsflag||mesh->_linesflag)
			if(mesh->_trianglesflag||mesh->_polygonsflag)
			{
				//lines/points could overlap the polygons
				PreSuppose(post.MakeVerboseFormat.steps);

				if(mesh->HasIndices()) 
				//AI: pre-clear lines and points attributes. Why??
				PreConst(mesh)->FacesList()^[&](const preFace &ea)
				{
					if(ea.polytypeN<3)
					{
						if(ea.polytypeN>0)
						dirtyMask[mesh->indiceslist[ea.startindex]] = true;
						if(ea.polytypeN>1)
						dirtyMask[mesh->indiceslist[ea.startindex+1]] = true;							
					}
				};
			}
			else //return ret;
			{
				//AI: Normals, tangents & bitangents are undefined 
				//for the whole mesh (and should not even be there)
				post.CriticalIssue("Line-and-or-points (only) mesh has normals and or tangents-and-bitangents.")
				<<"This mesh is being handled identically to a polygonal mesh regarding \"Invalid Data Removal\"";
			}		 
			mesh->ForEach([&](preMorph *ea)
			{ if(Process3D(ea->normalslist,ea->Normals(),"normals",true,false))
			{ ret = true; }});
			mesh->ForEach([&](preMorph *ea)
			{ if(Process3D(ea->tangentslist,ea->Tangents(),"tangents",true,false))    
			{ ret = true; ea->BitangentsCoList().Clear(); }});
			mesh->ForEach([&](preMorph *ea)
			{ if(Process3D(ea->bitangentslist,ea->Bitangents(),"bitangents",true,false))  
			{ ret = true; ea->TangentsCoList().Clear();	}});
		}

		//AI: not validating vertex colors (it's hard to say if they're valid or not)
		if(ret) log = true;	return ret?1:0;
	}	
	bool log;

public: //FindInvalidDataProcess

	FindInvalidDataProcess(Daedalus::post *p)
	:log(),aniEpsilon(p->FindInvalidDataProcess.configAnimationKeyEpsilon)
	,post(*p){}operator bool()
	{
		post.progLocalFileName("Removing Invalid Data");

		post.Verbose("FindInvalidDataProcess begin");

		size_t counter = 0;
		preScene *scene = post.Scene();	 		
		auto ml = scene->MeshesCoList();
		preID meshID = 0; ml^[&](preMesh *ea) 
		{
			if(Delete==Process(ea)) 
			{
				scene->CoDelete(ea);
				meshMapping[meshID++] = Deleted;				
			}
			else
			{
				ml[counter] = ea;
				meshMapping[meshID++] = counter++;
			}
		};
		if(counter<ml.RealSize())    
		{
			if(!counter) //throw DeadlyImportError
			post.CriticalIssue("No meshes remain");
			if(!ml.ForgoMemory(counter)) ml.Clear();
			RecursivelyRebuildMeshIDs(scene->rootnode);						
		}
		scene->AnimationsList()^[=](preAnimation *ea){ Process(ea); };
		if(log) post("FindInvalidDataProcess finished. Found issues ...");
		else post.Verbose("FindInvalidDataProcess finished. Everything seems to be OK.");
		return true;
	}
};
static bool FindInvalidDataProcess(Daedalus::post *post)
{
	return class FindInvalidDataProcess(post);
}							
Daedalus::post::FindInvalidData::FindInvalidData
(post *p):Base(p,p->steps&step?::FindInvalidDataProcess:0)
{
	configAnimationKeyEpsilon = 0;
}
