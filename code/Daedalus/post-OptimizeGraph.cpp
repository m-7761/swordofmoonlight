#include "Daedalus.(c).h"  
using namespace Daedalus;
class OptimizeGraph //Assimp/OptimizeGraph.cpp
{	
	Daedalus::post &post; 
	
	preScene *scene;		
	std::vector<size_t> perMeshReferences;		
	std::unordered_set<const char*> offLimits;
	PreSet<size_t,0> nodesIn, nodesOut, nodesMerged;

	std::vector<preNode*> nodes, join;	
	void RecursivelyProcess(preNode *node)
	{
		nodesIn+=node->Nodes();		
		auto nl = node->NodesList();		
		//optimizing (establish subrange)
		//AI: recursively gather children
		const size_t firstChild = nodes.size();
		nl^[=](preNode* &ea){ RecursivelyProcess(ea); ea = 0; };
		const size_t lastChild = nodes.size()-1;
		size_t liveChildren = nodes.size()-firstChild;
		//AI: Check if this node is needed;
		//if not replace it with its children
		if(offLimits.find(node->name.cstring)==offLimits.end()) 
		{
			for(size_t i=firstChild;i<=lastChild;i++) 			
			if(offLimits.find(nodes[i]->name.cstring)==offLimits.end()) 
			{
				nodes[i]->matrix = node->matrix*nodes[i]->matrix;
				nodes.push_back(nodes[i]);
				nodes[i] = 0; liveChildren--; //erase
			}
			if(!node->OwnMeshes()&&0==liveChildren) //AI: bye bye, node!
			{
				delete node; node = 0; //0: important below
			}
			else nodes.push_back(node);
		}
		else //AI: Retain current position in the hierarchy
		{	
			nodes.push_back(node); 
			//AI: check for possible optimizations within the
			//child nodes subrange, joining as many as possible
			preNode *join_master = 0; preMatrix inv;
			for(size_t i=firstChild;i<=lastChild;i++)
			{ 
				preNode *child = nodes[i]; assert(child); 
				//NOTE: Assimp's implementation doesn't say so, 
				//but !child->Nodes() implies if any descendents 
				//were unqualified, this ancestor is disqualified
				if(!child->Nodes())
				if(offLimits.find(child->name.cstring)==offLimits.end())
				{
					size_t counter = 0; 					
					PreConst(child)->OwnMeshesList()^[&](preID ea)
					{  	//AI: There cannot be instanced meshes					
						if(perMeshReferences[ea]<2) counter++;
					};					
					if(counter==child->OwnMeshes()) if(!join_master) 
					{
						assert(join.empty()); 
						join.clear(); join.reserve(nodes.size()); 
						join_master = child; 
						inv = join_master->matrix.InverseMatrix();
					}
					else 
					{
						child->matrix = inv*child->matrix;
						join.push_back(child);
						nodes[i] = 0; liveChildren--; //erase
					}
				}				
			}
			if(join_master&&!join.empty()) 
			{
				join_master->name.SetString //TODO: reconsider
				(std::string("_OptimizeGraph_")+std::to_string((long long)nodesMerged));
				nodesMerged++;

				preN out_meshes = 0;
				for(auto it=join.begin();it!=join.end();it++)
				out_meshes+=(*it)->OwnMeshes();

				if(out_meshes) //AI: join mesh instances together
				{
					auto l = join_master->OwnMeshesList();
					const PreNew<const preID[]> old_meshes(std::move(l));
					l.Reserve(out_meshes+old_meshes.RealSize());
					size_t counter = 0; old_meshes^[&](preID ea)
					{ l[counter++] = ea; };
					auto ml = scene->MeshesCoList();
					std::for_each(join.begin(),join.end(),[&](preNode *ea)
					{							
						PreConst(ea)->OwnMeshesList()^[&](preID ea2)
						{	//AI: join/move its mesh into the new coordinate system	
							l[counter++] = ea2; ml[ea2]->ApplyTransform(ea->matrix);
						};delete ea; //AI: bye bye, node!
					});l&=counter;
				}
			}			
		}
		if(node) //AI: reassign children if there are changes						
		{
			size_t n,i;
			nl.Reserve_Forgo_Clear(liveChildren);			
			for(n=0,i=firstChild;i<=lastChild;i++) 
			{ 
				if(nodes[i]) (nl[n++]=nodes[i])->node = node; 
			}nl&=n; 
		}
		nodes.erase(nodes.begin()+firstChild,nodes.begin()+lastChild+1);	
		nodesOut+=liveChildren;
	}

public: //OptimizeGraph

	OptimizeGraph(Daedalus::post *p)
	:post(*p),scene(p->Scene()){}operator bool()
	{	
		post.progLocalFileName("Optimizing Graph");

		post.Verbose("OptimizeGraphProcess begin");
																
		perMeshReferences.assign(scene->Meshes(),0);
		scene->rootnode->ForEach([&](const preNode *ea)
		{ ea->OwnMeshesList()^[&](size_t ea2){ perMeshReferences[ea2]++; };	});

		offLimits.clear(); //paranoia
		//AI: build a blacklist of off limit nodes		
		post.OptimizeGraph.configOffLimits^[&](const char *ea)
		{ offLimits.insert(ea); };
		PreConst(scene)->AnimationsList()^[&](const preAnimation *ea)
		{ 
			ea->SkinChannelsList()^[&](const preAnimation::Skin *ea2)
			{ 
				offLimits.insert(ea2->node.cstring); 
				ea2->AppendedNodesList()^[&](const preX &ea3){ offLimits.insert(ea3.cstring); };
			};//this is almost surely unnecessary, but can't hurt
			ea->MorphChannelsList()^[&](const preAnimation::Morph *ea2)
			{ ea2->AffectedNodesList()^[&](const preX &ea3){ offLimits.insert(ea3.cstring); }; };
		};
		auto ml = PreConst(scene)->MeshesCoList(); ml^=[&](size_t i)
		{
			ml[i]->BonesList()^[&](const preBone *ea2)
			{	//2: AI: HACK: Meshes referencing bones cannot be transformed
				//The easiest way to do this is to increase the reference counter
				perMeshReferences[i]+=2; offLimits.insert(ea2->node.cstring);
			};if(ml[i]->HasMorphs()) perMeshReferences[i]+=2; //NEW: same deal as above
		};
		PreConst(scene)->CamerasList()^[&](const preCamera *ea){ offLimits.insert(ea->node.cstring); };
		PreConst(scene)->LightsList()^[&](const preLight *ea){ offLimits.insert(ea->node.cstring); };

		//AI: Insert a dummy master node
		preNode *dummy_root = new preNode; 
		dummy_root->name.PoolString("OptimizeGraph.cpp");
		offLimits.insert(dummy_root->name.cstring);
		const char *rootname = scene->rootnode->name.cstring;
		scene->rootnode->node = dummy_root;
		dummy_root->NodesList().Resize(1,scene->rootnode);
		//NOTE: THE LISTS IN QUESTION WERE OPTIMIZED AWAY
		//AI: For each node collect a fully new list of children and allow their 
		//children to place themselves on the same hierarchy layer as their parents.		
		RecursivelyProcess(dummy_root); assert(1==nodes.size());
		switch(dummy_root->Nodes())
		{ //AI: return the original root node
		case 1: std::swap(scene->rootnode=0,dummy_root->NodesList()[0]); break;		
		//AI: Keep the dummy node while reusing original name
		default: scene->rootnode = dummy_root; scene->rootnode->name.PoolString(rootname);	
		//note: current joint names begin with _OptimizeGraph_
		scene->rootnode->NodesList()^[&](preNode *ea){ assert(rootname!=ea->name.cstring); };
		break; //0: PROGRAMMER ERROR IN THIS CASE?
		case 0: post.CriticalIssue("After optimizing the scene graph, no data remains");						
		assert(0); //delete dummy_root; return false; 
		}
		if(scene->rootnode!=dummy_root) delete dummy_root; 
		scene->rootnode->node = 0;

		if(nodesIn!=nodesOut) 
		post("OptimizeGraphProcess finished; Input nodes: ")<<+nodesIn<<", Output nodes: "<<+nodesOut;
		else post.Verbose("OptimizeGraphProcess finished");						 		
		return true;
	}
};
static bool OptimizeGraph(Daedalus::post *post)
{
	return class OptimizeGraph(post);
}							
Daedalus::post::OptimizeGraph::OptimizeGraph
(post *p):Base(p,p->steps&step?::OptimizeGraph:0)
{}
