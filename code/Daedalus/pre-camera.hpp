
//struct PreCamera //PreServe.h
//{	
	PRESERVE_LIST(PreCamera*)
	PRESERVE_LISTS(preID,MetaData,metadata)

	PreCamera():PRESERVE_VERSION
	,position(preNoTouching),up(preNoTouching),lookat(preNoTouching)
	{
		PRESERVE_ZEROTHISAFTER(_size)
		up.y = 1; lookat.z = 1;
		halfofwidth_angleofview = prePi/4,
		clipdistance = 0.1; farclipdistance = 1000;
	}
	DAEDALUS_DEPRECATED("porting Assimp? Try adding ,0")
	PreCamera(const PreCamera &cp); //not implementing
	PreCamera(const PreCamera &cp, PreX::Pool*p):PRESERVE_VERSION
	,position(preNoTouching),up(preNoTouching),lookat(preNoTouching)
	{
		PRESERVE_COPYTHISAFTER(_size);
		PRESERVE_COPYLIST(metadata)
		PreX::GetPool(p)->PoolString(node);
	}
	~PreCamera()
	{
		MetaDataList().Clear();
	}
	
	//UNTESTED: says "right-handed" 
	void GetCameraMatrix(preMatrix &out)const 
	{
		pre3D zaxis = lookat;    zaxis.Normalize();
        pre3D yaxis = up;        yaxis.Normalize();
        pre3D xaxis //= up^lookat; xaxis.Normalize(); 
		(up.y*lookat.z-up.z*lookat.y,up.z*lookat.x-up.x*lookat.z,up.x*lookat.y-up.y*lookat.x);
		xaxis.Normalize();
		out.a4 = -xaxis.DotProduct(position);
		out.b4 = -yaxis.DotProduct(position);
		out.c4 = -zaxis.DotProduct(position);
		out.a1 = xaxis.x;
		out.a2 = xaxis.y;
		out.a3 = xaxis.z;
		out.b1 = yaxis.x;
		out.b2 = yaxis.y;
		out.b3 = yaxis.z;
		out.c1 = zaxis.x;
		out.c2 = zaxis.y;
		out.c3 = zaxis.z;
		out.d1 = out.d2 = out.d3 = 0.f;
		out.d4 = 1.f;
	}

	//PretransformVertices
	void ApplyTransform(const preMatrix &x);
//};
