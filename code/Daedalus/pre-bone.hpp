
//struct PreBone //PreServe.h
//{	 
	PRESERVE_LIST(PreBone*)
	PRESERVE_LISTS(preID,MetaData,metadata)
	PRESERVE_LISTS(Weight,Weights,weights)	

	PreBone():PRESERVE_VERSION
	,matrix(preNoTouching){ PRESERVE_ZEROTHISAFTER(_size) matrix._SetDiagonal(1); }
	DAEDALUS_DEPRECATED("porting Assimp? Try adding ,0")
	PreBone(const PreBone &cp); //not implementing
	PreBone(const PreBone &cp, PreX::Pool *p):PRESERVE_VERSION
	,matrix(preNoTouching)
    {
		PRESERVE_COPYTHISAFTER(_size)
		PRESERVE_COPYLIST(metadata)
		PreX::GetPool(p)->PoolString(node);				
		PRESERVE_COPYLIST(weights)
    }
	~PreBone()
	{ 
		MetaDataList().Clear();
		WeightsList().Clear();
	}	
//};