
//struct PreNode //PreServe.h
//{	
	PRESERVE_LIST(PreNode*)
	PRESERVE_LISTS(PreNode*,Nodes,nodes)
	PRESERVE_LISTS(preID,OwnMeshes,meshes)
	PRESERVE_LISTS(preID,OwnMetaData,metadata)		

	DAEDALUS_DEPRECATED("porting Assimp? Try adding ,0")
	PreNode(const PreNode &cp); //not implementing
	PreNode(const PreNode &cp, PreX::Pool *p):PRESERVE_VERSION
	{	
		PRESERVE_COPYTHISAFTER(_size)	
		PRESERVE_COPYLIST(metadata)
		PreX::GetPool(p)->PoolString(name);			
		node = 0;
		PRESERVE_DEEPCOPYLIST(nodes,p)
		for(preN i=0;i<nodes;i++) nodeslist[i]->node = this;
		PRESERVE_COPYLIST(meshes)		
	}
	PreNode():PRESERVE_VERSION
	,matrix(preNoTouching){ PRESERVE_ZEROTHISAFTER(_size) matrix._SetDiagonal(1); }	
	~PreNode()
	{
		OwnMetaDataList().Clear(); OwnMeshesList().Clear();	NodesList().Clear();
	}
		
	template<class T> //see PreX::IsPooled
	DAEDALUS_DEPRECATED("please use FindNode(preX&) instead")
	PreNode *FindNode(T x)const; //not implementing
	PreNode *FindNode(const preX &x)
	{
		//==: order is important here in case x is string pooled
		if(x==name) return this; if(nodes) for(preN i=0;i<nodes;i++)
		{ PreNode *p = nodeslist[i]->FindNode(name); if(p) return p; }
		return 0;
	}	
	inline const PreNode *FindNode(const preX &x)const
	{ return ((PreNode*)this)->FindNode(x); }
	//VS2010: without const a nested lambda expression has
	//"int" type. const is removed to allow mutable lambdas
	template<class Lambda> inline void ForEach(const Lambda &f)
	{ ((Lambda&)f)(this); NodesList()^[&](preNode *ea){ ea->ForEach(f); }; }
	template<class Lambda> inline void ForEach(const Lambda &f)const
	{ ((Lambda&)f)(this); NodesList()^[&](const preNode *ea){ ea->ForEach(f); }; }
	template<typename F> inline preSize _Count(F f)const
	{ preSize n = 0; ForEach([&](const preNode *ea){ n+=(ea->*f)(); }); return n; }
	//Assimp/PretranformVertices.cpp
	inline preSize CountMeshes()const{ return _Count(&PreNode::OwnMeshes); }
	//write CountNodes(+1) to be safe
	inline preSize CountNodes(bool self)const{ return _Count(&PreNode::Nodes)+self; }
//};