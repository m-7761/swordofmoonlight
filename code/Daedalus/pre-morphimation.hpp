
//struct PreAnimation //PreServe.h
//{	
	//struct Morph
	//{
		PRESERVE_LIST(Morph*)
		PRESERVE_LISTS(preID,MetaData,metadata)
		PRESERVE_LISTS(PreMorph::Time,MorphKeys,morphkeys)
		PRESERVE_LISTS(preX,AffectedNodes,affectednodes)

		Morph(const Morph &cp, PreX::Pool *p):PRESERVE_VERSION
		{ 
			PRESERVE_COPYTHISAFTER(_size) 
			PRESERVE_COPYLIST(metadata) 
			(p=PreX::GetPool(p))->PoolString(mesh); 				
			PRESERVE_COPYLIST(morphkeys) 
			PRESERVE_COPYLIST(affectednodes) 
			for(size_t i=0;i<affectednodes;i++) p->PoolString(affectednodeslist[i]); 
		}
		Morph():PRESERVE_VERSION{ PRESERVE_ZEROTHISAFTER(_size) }
		~Morph()
		{
			MetaDataList().Clear(); 
			MorphKeysList().Clear();
			AffectedNodesList().Clear();
		}				
	//};
//};