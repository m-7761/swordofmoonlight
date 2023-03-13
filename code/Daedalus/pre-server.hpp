
//struct PreServer //PreServe.h
//{	
	inline preScene *Scene()
	{ return simpleSceneOut?simpleSceneOut:complexSceneOut; }

	PreServer(const PreServer &cp, PreX::Pool *p):PRESERVE_VERSION
	,pathtomoduleIn(cp.pathtomoduleIn),resourcetypeIn(cp.resourcetypeIn),resourcenameIn(cp.resourcenameIn)
	,amountofcontentIn(cp.amountofcontentIn),contentIn(cp.contentIn),interestedIn(cp.interestedIn) //compiler
	{
		PRESERVE_COPYTHISAFTER(resourcenameIn) PRESERVE_DEEPCOPY(terrainSceneOut,p)
		PRESERVE_DEEPCOPY(simpleSceneOut,p)	PRESERVE_DEEPCOPY(complexSceneOut,p)
	}
	static void _hush(const char*){} static void _noplace(int,int){}
	PreServer(const preX &ptm, const preX &urt, const preX &uri, unsigned char *c, size_t clen):PRESERVE_VERSION
	,pathtomoduleIn(ptm),resourcetypeIn(urt),resourcenameIn(uri),amountofcontentIn(clen),contentIn(c),prog(_noplace)
	{ PRESERVE_ZEROTHISAFTER(progLocalFileName) clog=clogVerbose=clogCriticalIssue=progLocalFileName =_hush; }		
	~PreServer()
	{
		delete simpleSceneOut; delete complexSceneOut; delete terrainSceneOut; 
		assert(!serverside_userdata); 
	}
	inline void operator=(const PreServer &cp)
	{
		this->~PreServer(); new(this)PreServer(cp);
	}

	//$ is a LogString (ls) token for localizing the logs
	#define CASE(x,y) case PreMaterial::x:return"$"#y"$";		
	inline const char *LogString(PreMaterial::Texture e){ switch(e){
	CASE(diffusetexture    ,Diffuse Texture)
	CASE(speculartexture   ,Specular Texture)
	CASE(ambienttexture    ,Ambient Texture)
	CASE(emissiontexture   ,Emission Texture)
	CASE(heightmap         ,Height-Map Texture)
	CASE(normalmap         ,Normal-Map Texture)
	CASE(shininesstexture  ,Shininess Texture)
	CASE(transparenttexture,Transparent Texture)
	CASE(lightmap          ,Light-Map Texture)
	CASE(reflectivetexture ,Reflective Texture) }return "$Unidentifiable Texture$";}
	inline const char *LogString(PreMaterial::Mapping e){ switch(e){
	case PreMaterial::texturecoords1D: //Texture Mapping
	case PreMaterial::texturecoords2D: case PreMaterial::texturecoords3D:
	case PreMaterial::colors1D:	case PreMaterial::colors2D:
	case PreMaterial::colors3D: case PreMaterial::colors4D: return "$Texture Mapping$";
	CASE(spheremapped      ,Sphere Mapping)
	CASE(tubemapped        ,Tube Mapping)
	CASE(cubemapped        ,Cube Mapping)
	CASE(orthomapped       ,Ortho Mapping) }return "$Unidentifiable Mapping$";}
	#undef CASE

	struct LogStream //can output int, double, or string
	{	
		void (*clog)(const char*); 
		inline LogStream(void(*log)(const char*)):clog(log){};		
		template<typename const_char> //template disambig for address of p
		inline LogStream operator<<(const_char *p){ clog(p); return *this; }
		template<typename T> inline LogStream operator<<(const T &t)//VS2010
		{ clog((t==int(t)?std::to_string((long long)t):std::to_string((long double)t)).c_str()); return *this; }				
	};
	struct LogStream2 : LogStream //auto append endl string
	{
		LogStream2(void(*log)(const char*)):LogStream(log){} ~LogStream2(){ clog("\n"); }
	};																   	
	inline LogStream2 operator*()const{ return LogStream2(clogVerbose); }
	inline LogStream2 operator!()const{ return LogStream2(clogCriticalIssue); }	
	template<class T> inline LogStream operator<<(const T &t)const{ return LogStream2(clog) << t; }
	template<class T> inline LogStream operator()(const T &t)const{ return LogStream2(clog) << t; }	
	template<class T> inline LogStream BlahBlahBlah(const T &t)const{ return LogStream2(clog) << t; }	
	template<class T> inline LogStream Verbose(const T &t)const{ return LogStream2(clogVerbose) << t; }	
	template<class T> inline LogStream CriticalIssue(const T &t)const{ return LogStream2(clogCriticalIssue) << t; }	
	
	inline void SimpleProgressUpdate(const void *place)const //Loading...
	{
		auto pos = (unsigned char*)place-contentIn; prog(pos,pos*1000/amountofcontentIn); 
	}
	inline void operator%=(const void *place)const{ SimpleProgressUpdate(place); }
	inline void ProgressUpdate(int place, int per_mille=0)const{ prog(place,per_mille); }
	
	//experimental server utility
	#define PreThrowOut(lbound,value,ubound) \
	if(lbound>value||value>=ubound) throw "\""#value"\" is outside ("#lbound","#ubound"]"
//};
