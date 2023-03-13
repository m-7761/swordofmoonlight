					
//Assimp/postprocess.h
struct post : preServer 
{	
	struct Base
	{size_t processordercheck;
	bool (*procedure)(post *p);
	Base(post *p, bool(*proc)(post*))
	:processordercheck(p->processes) //paranoia
	{
		procedure = proc; //performed if nonzero
		p->processeslist[p->processes++] = this;
	}Base(){} //CTOOOR
	}*processeslist[33]; size_t processes;
	//ConstrucT Out-Of-OrdeR (we do the best we can)
	#define CTOOOR(x) x(){} x(post *p);\
	inline void operator()(post *p){ new(this)x(p); }	
	struct CalcTangentSpace:Base //#1
	{enum{ step=0x1 };
	size_t configSourceUV; //0
	double configMaxAngle; //max(0,45deg)
	CTOOOR(CalcTangentSpace)
	}CalcTangentsProcess;
	struct JoinIdenticalVertices:Base //#2
	{enum{ step=0x2 };
	CTOOOR(JoinIdenticalVertices)
	}JoinVerticesProcess;
	struct MakeLeftHanded:Base //#3
	{enum{ step=0x4 };
	CTOOOR(MakeLeftHanded)
	}ConvertToLHProcess;
	struct Triangulate:Base //#4
	{enum{ step=0x8 };
	CTOOOR(Triangulate)
	}TriangulateProcess;
	//VC is Various Components?
	struct RemoveComponent:Base //#5
	{enum{ step=0x10 };		
	//not implementing UVs/Colors masks
	//configDeleteFlags; //interestedIn
	CTOOOR(RemoveComponent)
	}RemoveVCProcess; 
	struct GenNormals:Base //#6
	{enum{ step=0x20 };
	CTOOOR(GenNormals)
	//scheduled obsolete
	bool _GenFaceNormals(post*,preMesh*);
	}GenNormalsProcess_Face;
	struct GenSmoothNormals:Base //#7
	{enum{ step=0x40 };
	double configMaxAngle; //max(0,175deg)
	CTOOOR(GenSmoothNormals)
	}GenNormalsProcess_Vertex;	
	struct SplitLargeMeshes_Vertex:Base //#8
	{enum{ step=0x80 };
	size_t configVertexLimit; //1,000,000
	CTOOOR(SplitLargeMeshes_Vertex)
	}SplitLargeMeshes_Vertex;
	struct SplitLargeMeshes_Triangle:Base //#9
	{enum{ step=0x80 };
	size_t configTriangleLimit;	//1,000,000
	CTOOOR(SplitLargeMeshes_Triangle)
	}SplitLargeMeshes_Triangle;
	struct PreTransformVertices:Base //#10
	{//WARNING: removes animations//
	//TODO: config animation time//
	enum{ step=0x100 };
	bool configKeepHierarchy; //false
	//double to have non +/-1 volume
	double configNormalize; //false
	bool configTransform; //false
	preMatrix configTransformation; //identity
	size_t _animationsToBeRemoved;
	CTOOOR(PreTransformVertices)
	}PretransformVertices;
	struct LimitBoneWeights:Base //#11
	{enum{ step=0x200 };
	size_t configMaxWeights; //4
	CTOOOR(LimitBoneWeights)
	}LimitBoneWeightsProcess;
	struct ValidateDataStructure:Base //#12
	{enum{ step=0x400 };
	bool configWarnOfAssimpErrors; //true for Debug builds
	bool configWarningsAreErrors; //false
	bool configProceedWithErrors; //false
	CTOOOR(ValidateDataStructure)
	}ValidateDataStructure;
	struct ImproveCacheLocality:Base //#13
	{enum{ step=0x800 };
	size_t configCacheDepth; //32 //-12-
	bool configLogACMR; //false (verbose)
	CTOOOR(ImproveCacheLocality)		
	}ImproveCacheLocality;
	struct RemoveRedundantMaterials:Base //#14
	{enum{ step=0x1000 };
	bool configKeepUnused; //false
	PreNew<const char*[]>::Vector configOffLimits; //empty
	CTOOOR(RemoveRedundantMaterials)
	}RemoveRedundantMaterials;
	struct FixInfacingNormals:Base //#15
	{enum{ step=0x2000 };
	CTOOOR(FixInfacingNormals)
	}FixNormalsStep;
	struct SortByPType:Base //#16
	{enum{ step=0x8000 };
	bool configRemoveIfPolytope[3]; //false
	CTOOOR(SortByPType)
	}SortByPTypeProcess;
	//todo? call Degenerated
	struct FindDegenerates:Base //#17
	{enum{ step=0x10000 };
	bool configRemoveDegenerated; //false
	CTOOOR(FindDegenerates)
	}FindDegenerates;
	struct FindInvalidData:Base	//#18
	{enum{ step=0x20000 };
	double configAnimationKeyEpsilon; //0
	CTOOOR(FindInvalidData)
	}FindInvalidDataProcess;	
	struct GenUVCoords:Base //#19
	{//WARNING: disregards transforms//
	//WARNING: cubemaps are not done//
	enum{ step=0x40000 };
	CTOOOR(GenUVCoords)
	}ComputeUVMappingProcess;
	struct TransformUVCoords:Base //#20
	{enum{ step=0x80000 };
	double configMatrixEpsilon; //0
	CTOOOR(TransformUVCoords)
	}TextureTransform;
	struct FindInstances:Base //#21
	{enum{ step=0x100000 };
	bool configSpeedFlag; //false
	CTOOOR(FindInstances)
	}FindInstancesProcess;
	struct OptimizeMeshes:Base //#22
	{enum{ step=0x200000 };
	CTOOOR(OptimizeMeshes)
	}OptimizeMeshes;
	struct OptimizeGraph:Base //#23
	{enum{ step=0x400000 };
	PreNew<const char*[]>::Vector configOffLimits; //empty
	CTOOOR(OptimizeGraph)
	}OptimizeGraph;		 
	struct FlipUVs:Base //#24
	{//WARNING: ignores bitangents//
	//(see postprocess.h comment)//
	enum{ step=0x800000 };
	CTOOOR(FlipUVs)
	}ConvertToLHProcess_FlipUVs;
	struct FlipWindingOrder:Base //#25
	{enum{ step=0x1000000 };
	CTOOOR(FlipWindingOrder)
	}ConvertToLHProcess_FlipWindingOrder;
	//SplitLargeMeshesProces.cpp
	struct SplitByBoneCount:Base //#26
	{enum{ step=0x2000000 };
	size_t configMaxBoneCount; //60
	CTOOOR(SplitByBoneCount)
	}SplitLargeMeshes_Bone;
	struct Debone:Base //#27 
	{enum{ step=0x4000000 };		
	bool configAllOrNone; //false
	double configThreshold; //1
	CTOOOR(Debone)
	}DeboneProcess;		
	//compulsory	
	struct SimplifyScene:Base //#28
	{CTOOOR(SimplifyScene)}SimplifySceneProcess;	
	struct _ScenePreprocessor:Base //#29
	{CTOOOR(_ScenePreprocessor)}ScenePreprocessor;	
	struct _MakeVerboseFormat:Base //#30
	{enum{ steps=CalcTangentSpace::step|GenUVCoords::step|GenNormals::step|GenSmoothNormals::step|FindInstances::step };
	CTOOOR(_MakeVerboseFormat)
	}MakeVerboseFormat; //SCHEDULED OBSOLETE
	//optimizing
	struct _ComputeSpatialSortProcess:Base //#31
	{enum{ steps=CalcTangentSpace::step|JoinIdenticalVertices::step|GenSmoothNormals::step };
	double configSeparationEpsilon; //NEW: for joining/smoothing
	void FindCommonPositions(size_t meshes_plus_morphs_index, const pre3D &of, std::vector<signed> &fill);
	CTOOOR(_ComputeSpatialSortProcess) void *_cleanup; size_t _counter;	
	}SpatialSort_Compute; //SCHEDULED OBSOLETE
	struct _DestroySpatialSortProcess:Base //#32
	{enum{ steps=_ComputeSpatialSortProcess::steps };	
	CTOOOR(_DestroySpatialSortProcess) 
	}SpatialSort_Destroy;	
	//reserved for future use
	struct ComplicateScene:Base //#33
	{CTOOOR(ComplicateScene)}ComplicateSceneProcess;
	#undef CTOOOR //now get tri-eyed beast!!
	//processing
	//1) construct
	//2) configure
	//3) proceed()
	const unsigned steps;
	post(const preServer &ps, unsigned pp):
	PreServer(ps),steps(pp?pp|JoinIdenticalVertices::step:0),processes(){//,	
	//Assimp/PostStepRegistry.cpp	
	//THIS is the order in which these steps
	//are to be performed named according to
	//the translation unit (.cpp) file names	
	ValidateDataStructure(this),
	SimplifySceneProcess(this),
	ScenePreprocessor(this),
	RemoveVCProcess(this),
	MakeVerboseFormat(this),	
	ConvertToLHProcess(this),	
	ConvertToLHProcess_FlipUVs(this),
	ConvertToLHProcess_FlipWindingOrder(this),	
	RemoveRedundantMaterials(this),
	FindInstancesProcess(this),
	OptimizeGraph(this),		
	FindDegenerates(this),
	ComputeUVMappingProcess(this),
	TextureTransform(this),
	PretransformVertices(this),
	TriangulateProcess(this),
	SortByPTypeProcess(this),
	FindInvalidDataProcess(this),
	OptimizeMeshes(this),
	FixNormalsStep(this),
	SplitLargeMeshes_Bone(this), 
	SplitLargeMeshes_Triangle(this),
	GenNormalsProcess_Face(this),
	SpatialSort_Compute(this),	
	GenNormalsProcess_Vertex(this),	
	CalcTangentsProcess(this),
	JoinVerticesProcess(this),
	SpatialSort_Destroy(this),	
	SplitLargeMeshes_Vertex(this),
	DeboneProcess(this),
	LimitBoneWeightsProcess(this),	
	ImproveCacheLocality(this); 
	ComplicateSceneProcess(this); //{
	assert(processes==sizeof(processeslist)/sizeof(*processeslist)); }
	//proceed is concerned with simpleScene
	//only, and will output its log to clog 
	//the scene must live on the local heap
	//bool is true if all procedures finish
	inline bool proceed()
	{
		for(size_t i=0;i<processes;i++) 
		if(processeslist[i]->procedure&&!processeslist[i]->procedure(this))
		return false; return true;
	}

	//scheduled obsolete?
	//Assimp is confused about its epsilons
	//These are based on JoinVerticesProcess'
	//FindInstancesProcess USED different more
	//varied epsilons w/ ComputePositionEpsilon
	//used w/ normals/tangents and also position
	//(it USED 10e-3 w/ bones, and 10e-4 w/ color)
	//NOTICE: bone weights are now single-precision
	static inline double fixedEpsilon(){ return 10e-5; }
	static inline double fixedEpsilon2(){ return 10e-5*10e-5; }
};