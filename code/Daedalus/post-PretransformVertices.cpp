#include "Daedalus.(c).h"  
using namespace Daedalus;
class PretransformVertices //Assimp/PretransformVertices.cpp
{	
	Daedalus::post &post; 

	preScene *const scene; const size_t firstNewMeshID;

	struct SaneFormat //Sane? was testing vformat bits (eg. &0x2)
	{
		unsigned vformat; preMesh *example;	
		inline operator unsigned()const{ return vformat; }
		inline bool operator<(const SaneFormat &cmp)const{ return vformat<cmp.vformat; }
		inline bool operator==(const SaneFormat &cmp)const{ return vformat==cmp.vformat; }
		inline SaneFormat(preMesh *e=0):example(e),vformat(e?e->GetMeshVFormatUnique():0){}
	};				
	std::vector<SaneFormat> vformats;
	struct Counter //passing a fresh counter to the below two subroutines
	{ size_t position, face, index; Counter():position(),face(),index(){} };
	void RecursivelyCount(const preNode *node, preID material, unsigned vformat, Counter &count)
	{
		auto ml = PreConst(scene)->MeshesList(); node->OwnMeshesList()^[&](preID ea)
		{
			const preMesh *mesh = ml[ea]; if(material==mesh->material&&vformat==vformats[ea])
			{
				count.position+=mesh->Positions(); count.face+=mesh->Faces(); count.index+=mesh->Indices();
			}
		};
		node->NodesList()^[&](const preNode *ea){ RecursivelyCount(ea,material,vformat,count); };
	}
	//TODO: can MergeMeshesHelper not be of use here?
	void RecursivelyAppendTo(const preNode *node, preMesh *out, preID material, unsigned vformat, Counter &base)
	{	
		auto ml = PreConst(scene)->MeshesList();
		const bool identity = node->matrix.IsIdentity(); 		
		auto il = node->OwnMeshesList(); for(size_t i=il;i<il.Size();i++)
		{
			const preMesh *mesh = ml[il[i]];
			if(material==mesh->material&&vformat==vformats[il[i]])
			{	//Reminder: Morphs (animations) are deleted
				out->_polytypesflags|=mesh->_polytypesflags; 
				const size_t size3D = mesh->Positions()*sizeof(pre3D);
				const size_t size4D = mesh->Positions()*sizeof(pre4D);
				memcpy(out->positionslist+base.position,mesh->positionslist,size3D);
				if(mesh->HasNormals())
				memcpy(out->normalslist+base.position,mesh->normalslist,size3D);
				if(mesh->HasTangentsAndBitangents())
				{ memcpy(out->tangentslist+base.position,mesh->tangentslist,size3D);
				memcpy(out->bitangentslist+base.position,mesh->bitangentslist,size3D); }				
				out->ApplyTransform(node->matrix,identity);
				for(size_t i=0;mesh->HasTextureCoords(i);i++) 
				memcpy(out->texturecoordslists[i]+base.position,mesh->texturecoordslists[i],size3D);
				for(size_t i=0;mesh->HasVertexColors(i);i++)
				memcpy(out->colorslists[i]+base.position,mesh->colorslists[i],size4D);				

				if(mesh->HasIndices())
				memcpy(out->indiceslist+base.index,mesh->indiceslist,sizeof(*out->indiceslist)*mesh->indices);
				if(mesh->HasFaces())
				memcpy(out->faceslist+base.face,mesh->faceslist,sizeof(*out->faceslist)*mesh->faces);
				out->FacesList()^[&](preFace &ea){ ea.startindex+=base.position; };

				base.position+=mesh->Positions(); base.face+=mesh->Faces(); base.index+=mesh->Indices();
			}
		}		
		node->NodesList()^[&](const preNode *ea){ RecursivelyAppendTo(ea,out,material,vformat,base); };
	}	 
	//REFORMULATED TO NOT REPURPOSE PreMesh::bones/boneslist
	//THOUGH IT'S STILL REPURPOSING PreNode::matrix BY PROXY
	std::vector<std::pair<size_t,preMatrix*>> KeepHierarchy;
	void RecursivelyKeepHierarchy(preNode *node, std::vector<preMesh*> &out)
	{
		auto ml = PreConst(scene)->MeshesList();
		auto il = node->OwnMeshesList(); for(size_t i=il;i<il.Size();i++)
		{	//AI: see if this mesh can be operated upon			
			preMatrix* &matrix = KeepHierarchy[i].second;
			if(!matrix||*matrix==node->matrix) 
			{ 
				matrix = &node->matrix; KeepHierarchy[i].first = -1; 
			}
			else //Creating a transformable copy of the mesh? 
			{	//AI: first try to find among newly created meshes
				for(size_t j=0;j<out.size();j++) 				
				if(KeepHierarchy[j].first==il[i]&&*KeepHierarchy[j].second==node->matrix) 
				{  	//NEW: quotes added to support addition of the following break statement
					//AI: "ok, use this one. Update node mesh index"					
					il[i] = firstNewMeshID+j;						
					break; //NEW!!
				}
				if(il[i]<firstNewMeshID) 
				{  	//AI: Worst case. Must operate on a full copy of the mesh
					post("Copying mesh due to mismatching transforms");
					KeepHierarchy.push_back(std::make_pair(il[i],&node->matrix));
					out.push_back(new preMesh(*ml[il[i]],0));										
					il[i] = firstNewMeshID+out.size()-1;
				}
			}
		}		
		node->NodesList()^[&](preNode *ea){ RecursivelyKeepHierarchy(ea,out); };
	}	

public: //PretransformVertices
		
	const size_t animationsPrior;
	PretransformVertices(Daedalus::post *p)
	:scene(p->Scene()),firstNewMeshID(scene->Meshes())
	,animationsPrior(scene->Animations()
	+post.PretransformVertices._animationsToBeRemoved)	
	,post(*p){}operator bool()
	{
		post.progLocalFileName("Moving All Vertices to Global Coordinate System");

		//ASSUMING ANIMATIONS ARE ALREADY DELETED
		const size_t meshesPrior = scene->Meshes();
		const size_t nodesPrior = scene->CountNodes();
		post.PretransformVertices._animationsToBeRemoved = 0;

		post.Verbose("PretransformVerticesProcess begin");
														
		//UNCLEAR WHERE EXACTLY Assimp GETS THIS FROM
		if(post.PretransformVertices.configTransform)	
		scene->rootnode->matrix = post.PretransformVertices.configTransformation;

		//AI: compute absolute transformation matrices for all but root node		
		scene->rootnode->NodesList()^[](preNode *ea) //avoid writing if(node)
		{ ea->ForEach([](preNode *ea){ ea->matrix = ea->node->matrix*ea->matrix; }); };
	
		auto ml = scene->MeshesList();
		std::vector<preMesh*> outMeshes;
		//AI: Keep scene hierarchy? It's an easy job in this case ...
		//we go on and transform all meshes, if one is referenced by nodes
		//with different absolute transformations a depth copy of the mesh is required.
		if(post.PretransformVertices.configKeepHierarchy)
		{
			KeepHierarchy.reserve(scene->CountMeshes());
			KeepHierarchy.resize(firstNewMeshID);
			RecursivelyKeepHierarchy(scene->rootnode,outMeshes);			
			if(!outMeshes.empty()) //AI: append generated meshes to the scene
			{
				ml.Reserve(firstNewMeshID+outMeshes.size());
				memcpy(firstNewMeshID+ml.Pointer(),outMeshes.data(),outMeshes.size()*sizeof(ml[0]));
			}
			ml^=[&](size_t m){ ml[m]->ApplyTransform(*KeepHierarchy[m].second); };
		}
		else //or, not keeping the scene hierarchy
		{	
			outMeshes.reserve(2*scene->Materials());
			//NOTICE: reusing back of vformats after firstNewMeshID
			vformats.reserve(2*firstNewMeshID); vformats.assign(ml.Begin(),ml.End());					
			for(size_t i=0;i<scene->Materials();i++,vformats.resize(firstNewMeshID))   
			{	//AI: get per material list of vformats				
				ml^=[&](size_t m){ if(i==ml[m]->material) vformats.push_back(vformats[m]); };				
				std::sort(vformats.begin(),vformats.end()); 
				vformats.erase(std::unique(vformats.begin(),vformats.end()),vformats.end()); 
				for(auto it=vformats.cbegin();it<vformats.cend();it++)
				{					
					Counter count;
					RecursivelyCount(scene->rootnode,i,it->vformat,count);
					if(count.face&&count.position)
					{
						PreSuppose(!PreMesh2::IsSupported);
						outMeshes.push_back(new preMesh());
						preMesh *mesh = outMeshes.back();						
						mesh->PositionsCoList().Reserve(count.position);
						if(it->example->HasNormals()) 
						mesh->NormalsCoList().Reserve(count.position);
						if(it->example->HasTangentsAndBitangents())
						{ mesh->TangentsCoList().Reserve(count.position);
						mesh->BitangentsCoList().Reserve(count.position); }
						for(preN i=0;it->example->HasTextureCoords(i);i++)
						mesh->TextureCoordsCoList(i).Reserve(count.position);
						for(preN i=0;it->example->HasVertexColors(i);i++)
						mesh->VertexColorsCoList(i).Reserve(count.position);
						if(it->vformat!=mesh->GetMeshVFormatUnique())
						post.CriticalIssue("Programmer error: data structure does not reflect vertex format.");
						mesh->FacesList().Reserve(count.face);
						mesh->IndicesList().Reserve(count.index);
						//AI: fill in this mesh
						mesh->material = i; Counter lvalue; 
						RecursivelyAppendTo(scene->rootnode,mesh,i,it->vformat,lvalue);
					}
				}
			}//AI: delete meshes in the scene and build a new mesh list
			scene->MeshesList().Assign(outMeshes.size(),outMeshes.data());
		}//AI: keeping the cameras and lights
		scene->CamerasList()^[=](preCamera *ea)
		{ ea->ApplyTransform(scene->FindNode(ea->node)->matrix); };
		scene->LightsList()^[=](preLight *ea)
		{ ea->ApplyTransform(scene->FindNode(ea->node)->matrix); };
		if(!post.PretransformVertices.configKeepHierarchy) 
		{	//AI: delete all nodes in the scene and flatten it out
			delete scene->rootnode;	scene->rootnode = new preNode;
			scene->rootnode->name.SetString("_PretransformVertices_");
			auto nl = scene->rootnode->NodesList();
			nl.Reserve(scene->Meshes()+scene->Cameras()+scene->Lights());
			size_t i,n = 0; preNode *node;
			auto &new_preNode = [&](const char *name_)->preNode* //lambda
			{
				node = nl[n++] = new preNode;
				node->name.SetString(name_+std::to_string((long long)i++)); //VS2010
				node->node = scene->rootnode; return node;
			};
			i = 0; scene->MeshesList()^[&](preMesh *ea){ new_preNode("mesh_")->OwnMeshesList().Assign(1,i-1); };
			i = 0; scene->CamerasList()^[&](preCamera *ea){ ea->node = new_preNode("camera_")->name; };
			i = 0; scene->LightsList()^[&](preLight *ea){ ea->node = new_preNode("light_")->name; };
			nl&=n;
		}//AI: finally set the transformation matrices equal to the identity matrix
		else scene->rootnode->ForEach([](preNode *ea){ new(&ea->matrix)preMatrix(); }); 
		
		//Assimp provides option of fitting into +/-1
		if(post.PretransformVertices.configNormalize) 
		{	//AI: find the extents of the scene
			Pre3D::Container minmax(Pre3D::MinMax());
			scene->MeshesList()^[&](preMesh *ea){ minmax = ea->GetContainer(minmax); };
			//AI: find the dominant axis
			pre3D d = minmax.maxima-minmax.minima; 
			const double div = std::max(d.x,std::max(d.y,d.z))/2;
			d = minmax.minima+d/2; 
			//provide opportunity to scale to non +/-1 volume
			double rescale = 1/div*post.PretransformVertices.configNormalize;
			scene->MeshesList()^[&](preMesh *ea)
			{ ea->PositionsCoList()^[&](pre3D &ea2){ ea2 = (ea2-d)*rescale; }; };
		}

		post.Verbose("PretransformVerticesProcess finished:");
		post("Removed ")<<nodesPrior<<" nodes and "<<animationsPrior<<" animations ("<<scene->CountNodes()<<" output nodes)";				
		post("Kept ")<<scene->Lights()<<" lights and "<<scene->Cameras()<<" cameras";
		post("Moved ")<<meshesPrior<<" meshes to WCS (number of output meshes: "<<scene->Meshes()<<")";		
		return true;
	}
};
static bool PretransformVertices(Daedalus::post *post)
{
	return class PretransformVertices(post);
}							
Daedalus::post::PreTransformVertices::PreTransformVertices
(post *p):Base(p,p->steps&step?::PretransformVertices:0)
{
	configKeepHierarchy = configTransform = false; configNormalize = 0;
}