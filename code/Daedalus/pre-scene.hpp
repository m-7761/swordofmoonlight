
//struct PreScene //PreServe.h
//{	
	PRESERVE_LIST(PreScene*)

	//delete MeshesCoList items
	template<class T> inline void CoDelete(T *co)
	{ if(co->Is2()) delete co->This2(); else delete co; }		
	//3: HasMeshes is MeshesCoList
	PRESERVE_LISTS3(preMesh*,MeshesList(),meshes,mesheslist)	
	PRESERVE_LISTS3(preMesh2*,MeshesList2(),meshes,mesheslist2)	
	PRESERVE_COLISTS(PreScene,preMesh*,Meshes,meshes,mesheslist?mesheslist:(preMesh**&)mesheslist2)	
	template<class T, class I> static inline T &_CoGet(T *p, const I &i){ return p[i]; }
	template<class List> inline void _CoClear(List &list){ _CoReserve(0,list); }
	template<class List> inline void _CoMoved(List &list){ list._size = 0; }
	template<class List> void _CoReserve(size_t size, List &list)
	{
		if(&list._list==(preMesh***)&mesheslist) list._CoNew(size,mesheslist); else
		if(&list._list==(preMesh***)&mesheslist2) list._CoNew(size,mesheslist2); else assert(0);
	}		
	PRESERVE_LISTS(preMaterial*,Materials,materials)
	PRESERVE_LISTS(preAnimation*,Animations,animations)
	PRESERVE_LISTS(preTexture*,EmbeddedTextures,embeddedtextures)
	PRESERVE_LISTS(preLight*,Lights,lights)
	PRESERVE_LISTS(preCamera*,Cameras,cameras)
	PRESERVE_LISTS(preMeta*,MetaData,metadata)
	PRESERVE_LISTS(char*,PooledStrings,pooledstrings)

	DAEDALUS_DEPRECATED("porting Assimp? Try adding ,0")
	PreScene(const PreScene &cp); //not implementing
	PreScene(const PreScene &cp, PreX::Pool *p):PRESERVE_VERSION
	{
		PRESERVE_COPYTHISAFTER(_size)
		PreX::GetPool(p)->PoolString(name);
		PRESERVE_DEEPCOPY(rootnode,p)		
		PRESERVE_DEEPCOPYLIST(meshes,p)
		preMesh2 **submeshes = mesheslist2;
		PRESERVE_DEEPCOPYLIST2(meshes,mesheslist2,p)
		preN m = 0; MeshesList2()^[&](preMesh2 *ea)
		{
			if(ea->submesh&&!ea->IsSubmesh()) //repair submesh?
			{
				while(m<meshes&&submeshes[m]!=ea->submesh) m++;
				if(m==meshes) //begin search anew from the front?
				for(m=0;m<meshes&&submeshes[m]!=ea->submesh;m++);
				if(m!=meshes) //todo? is this an error? then what?
				ea->submesh = mesheslist2[m]; else assert(m<meshes);
			}
		};
		PRESERVE_DEEPCOPYLIST(materials,p)
		PRESERVE_DEEPCOPYLIST(animations,p)
		PRESERVE_DEEPCOPYLIST(embeddedtextures,p)
		PRESERVE_DEEPCOPYLIST(lights,p)
		PRESERVE_DEEPCOPYLIST(cameras,p)
		PRESERVE_DEEPCOPYLIST(metadata,p)		
		PreX::GetPool(p)->GetDeleteBuffers(pooledstrings,pooledstringslist);
		serverside_delete = _serverside_delete;
	}
	PreScene():PRESERVE_VERSION
	{ PRESERVE_ZEROTHISAFTER(_size) serverside_delete = _serverside_delete; }
	~PreScene()	//Assimp/Version.cpp
	{
		delete rootnode;
		MeshesCoList().Clear();
		MaterialsList().Clear();
		AnimationsList().Clear();
		EmbeddedTexturesList().Clear();
		LightsList().Clear();
		CamerasList().Clear();
		MetaDataList().Clear();
		PooledStringsList().Clear();
	}
	static void _serverside_delete(PreScene *p){ delete p; }

	//Assimp/PretranformVertices.cpp
	inline preSize CountNodes()const{ return rootnode?rootnode->CountNodes(+1):0; }
	inline preSize CountMeshes()const{ return rootnode?rootnode->CountMeshes():0; }
	inline PreNode *FindNode(const preX &x){ return rootnode?rootnode->FindNode(x):0; }
	inline const PreNode *FindNode(const preX &x)const{ return rootnode?rootnode->FindNode(x):0; }
//};