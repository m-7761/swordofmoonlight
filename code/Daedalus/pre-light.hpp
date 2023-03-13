
//struct PreLight //PreServe.h
//{
	PRESERVE_LIST(PreLight*)
	PRESERVE_LISTS(preID,MetaData,metadata)

	PreLight():PRESERVE_VERSION
	,position(preNoTouching),direction(preNoTouching)
	,diffusecolor(preNoTouching),specularcolor(preNoTouching),ambientcolor(preNoTouching)
	{ PRESERVE_ZEROTHISAFTER(_size) linearattenuate = 1; innercone = outercone = 2*prePi; }
	DAEDALUS_DEPRECATED("porting Assimp? Try adding ,0")
	PreLight(const PreLight &cp); //not implementing
	PreLight(const PreLight &cp, PreX::Pool *p):PRESERVE_VERSION
	,position(preNoTouching),direction(preNoTouching)
	,diffusecolor(preNoTouching),specularcolor(preNoTouching),ambientcolor(preNoTouching)
	{
		PRESERVE_COPYTHISAFTER(_size);
		PRESERVE_COPYLIST(metadata)
		PreX::GetPool(p)->PoolString(node);
	}
	~PreLight()
	{
		MetaDataList().Clear();
	}	

	//PretransformVertices
	void ApplyTransform(const preMatrix &x);
//};