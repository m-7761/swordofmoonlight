
//struct PreAnimation //PreServe.h
//{	
	PRESERVE_LIST(PreAnimation*)
	PRESERVE_LISTS(preID,MetaData,metadata)
	PRESERVE_LISTS(Skin*,SkinChannels,skinchannels)
	PRESERVE_LISTS(Morph*,MorphChannels,morphchannels)

	PreAnimation(const PreAnimation &cp, PreX::Pool *p):PRESERVE_VERSION
	{ 
		PRESERVE_COPYTHISAFTER(_size) 
		PRESERVE_COPYLIST(metadata) 
		PreX::GetPool(p)->PoolString(name); 
		PRESERVE_DEEPCOPYLIST(skinchannels) 
		PRESERVE_DEEPCOPYLIST(morphchannels) 
	}
	PreAnimation():PRESERVE_VERSION{ PRESERVE_ZEROTHISAFTER(_size) }
	~PreAnimation()
	{
		MetaDataList().Clear();
		SkinChannelsList().Clear();
		MorphChannelsList().Clear();
	}
//};