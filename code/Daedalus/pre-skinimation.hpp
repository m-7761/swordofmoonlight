
//struct PreAnimation //PreServe.h
//{	
	//struct Skin
	//{
		PRESERVE_LIST(Skin*)
		PRESERVE_LISTS(preID,MetaData,metadata)
		PRESERVE_LISTS(Pre3D::Time,ScaleKeys,scalekeys)
		PRESERVE_LISTS(Pre3D::Time,PositionKeys,positionkeys)
		PRESERVE_LISTS(PreQuaternion::Time,RotationKeys,rotationkeys)
		PRESERVE_LISTS(preX,AppendedNodes,appendednodes)

		Skin(const Skin &cp, PreX::Pool *p):PRESERVE_VERSION
		{
			PRESERVE_COPYTHISAFTER(_size) 
			PRESERVE_COPYLIST(metadata) 					
			(p=PreX::GetPool(p))->PoolString(node);
			PRESERVE_COPYLIST(scalekeys) 
			PRESERVE_COPYLIST(positionkeys) 
			PRESERVE_COPYLIST(rotationkeys) 
			PRESERVE_COPYLIST(appendednodes) 
			for(size_t i=0;i<appendednodes;i++) p->PoolString(appendednodeslist[i]); 			
		}
		Skin():PRESERVE_VERSION{ PRESERVE_ZEROTHISAFTER(_size) }
		~Skin()
		{
			MetaDataList().Clear();
			ScaleKeysList().Clear();
			PositionKeysList().Clear();
			RotationKeysList().Clear();
			AppendedNodesList().Clear();
		}				
	//};
//};