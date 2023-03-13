//This file maps neatly to scene.h
//https://github.com/assimp/assimp
//Refer to documentation of Assimp
#ifndef PRESERVE_H_INCLUDED
#define PRESERVE_H_INCLUDED
//#include Daedalus.h 
#define PreSuppose(x) /*apologies*/\
static_assert(x,"PreSuppose("#x");")
#include "pre-pre.hpp"
//suppression of nullary constructor
enum PreNoTouching{ preNoTouching };
typedef unsigned preN,preID; //32bit
static void *const preZeroMemory = 0; 
static double const prePi = 3.141592653589793238462643383279;
//Assimp/types.h
typedef struct PreX
{
	size_t _target;
	const char *cstring;
	preN length,_allocated;	
	#include "pre-string.hpp"
}preX;
template<class T> union PreBi
{
	struct{ T a,b; }; 
	struct{ T minima, maxima; };	
	#include "pre-bivector.hpp"
};
template<class T> struct PreTime
{
	typedef T typeofvalue; 
	typedef double typeofkey; 	
	#define typeof_this PreTime<T>
	#include "pre-key.hpp"
	PreTime<T>():value(preNoTouching){}
};
typedef union Pre2D
{
	struct{ double r,g; };	
	struct{ double s,t; };
	struct{ double x,y; };	
	#define N 2
	#include "pre-vector.hpp"
	typedef PreBi<Pre2D> Bivector, Container;
}pre2D;
typedef union Pre3D
{
	struct{ double r,g,b; };	
	struct{ double s,t,p; };
	struct{ double x,y,z; };	
	#define N 3
	#include "pre-vector.hpp"
	typedef PreTime<Pre3D> Time;
	typedef PreBi<Pre3D> Bivector, Container;
}pre3D;
typedef union Pre4D
{
	struct{ double d,c; };
	struct{ double r,g,b,a; };	
	struct{ double s,t,p,q; };
	struct{ double x,y,z,w; };	
	#define N 4
	#include "pre-vector.hpp"
	typedef PreBi<Pre4D> Bivector, Container;
}pre4D;
typedef struct PreQuaternion 
{
	union Matrix
	{	
		struct{ double a1,a2,a3,b1,b2,b3,c1,c2,c3; }; 
		#define M 3
		#include "pre-matrix.hpp"
	};
	double w,x,y,z;//Assimp order
	#include "pre-quaternion.hpp"
	typedef PreTime<PreQuaternion> Time;
}preQuaternion;
typedef union PreMatrix
{	
	struct{ double a1,a2,a3,a4,b1,b2,b3,b4,c1,c2,c3,c4,d1,d2,d3,d4; }; 	 
	#define M 4
	#include "pre-matrix.hpp"	
	//using this notation for scale/shear
	typedef PreQuaternion::Matrix Matrix;
}preMatrix;
#ifndef preSize 
typedef union PreSize //optimizing 
{
	preN n; size_t x64;
	public: //#include "pre-size_t.hpp"
	PreSize(){} PreSize(preN x):x64(x){}
	inline operator preN&(){ return n; }
	inline operator preN()const{ return n; }
	#if SIZE_MAX>0xFFFFFFFFULL&&INT_MAX==2147483647
	//Reminder: x64==n fails for big-endian.
	inline PreSize &operator=(const preN &x)
	{ n = x; assert(x64==n); return *this;
	static_assert(0,"x64 support has not been implemented"); }	
	#else //trivial: size_t IS unsigned/preN
	inline PreSize &operator=(const preN &x){ n = x; return *this; }
	#endif
}preSize; //size_t
#elif defined(NDEBUG)
#error preSize is defined
#endif //#define preSize size_t
static const size_t preMost = 2147483647;
//PreList/PreNew are lightweight STL-like APIs 
template<class T, class Co=void> class PreList
{	
	//List is a generic container. To use
	//it, do: auto l = scene->SomeStuffList();
	//It should be treated as a straight reference
	//or, do: auto v = scene->SomeStuffList().AsVector();
	typedef typename PreCopyRef<preSize,T>::type S; //class S
	S _size; T _list; union{ Co *_colistee; const Co *_cc; };
	//List keeps minimal state. IN FACT Reserve changes Size!
	#include "pre-list.hpp"		
	inline Vector AsVector(){ return Vector(*this); }	

  ////WARNING////////////////////////////////////////////////
  //Co-lists can only have one Vector outstanding at a time//
  //They are the vertex attributes that share a common Size//
  //(this could be fixed by storing _capacity in _colistee)//
  ///////////////////////////////////////////////////////////

  ////BUFFERS////////////////////////////////////////////////
  //Vector can create an un-initialized buffer like Reserve//
  //However it must use Resize(<size>,preNoTouching) in its//
  //place as, as with std::vector, Reserve is capacity only//
  ///////////////////////////////////////////////////////////

  ////POINTERS (VERY IMPORTANT)//////////////////////////////
  //Lists-of-pointers implicitly singly delete the elements//
  //whenever destruction is called for. Assign calls for it//
  //Note that if Reserve is used the pointers must be valid//
  ///////////////////////////////////////////////////////////

  ////LAMBDAS & OTHER OPERATORS//////////////////////////////
  //The ^ and ^= operators work with C++11 lambda functions//
  //(or functors or regular function pointers) and are easy//
  //to remember as ^ is shaped like the Greek letter lambda//
  //FOR HEAVY CODING, IT'S WORTHWHILE TO LOOK AT THE OTHERS//
  ///////////////////////////////////////////////////////////

  ////RANGE-BASED FOR-LOOPS & STL-STYLE ITERATORS////////////
  //NEW: there is now an iterator model; but if you want to//
  //have range-based for-loop support, you must implement a//
  //specialization of std::begin and std:end in your module//
  //ITERATORS ARE NOT BOUND CHECKED, SO IT IS BEST PRACTICE//
  //TO NOT USE THEM IN YOUR MODULE'S OWN CODE (USE ^ OR [])//
  ///////////////////////////////////////////////////////////

};enum PreDefault{ preDefault }; //nullary system constructor
//In the future--2020--PreNew can just be a "using" statement
template<class T> struct PreNew<T[]> : PreList<T*,PreDefault>
{template<class L> PreNew(L&); PreNew(){} //std::move, ::Vector, or default
template<class L> PreNew(L&&l){ _size = l._size; _list = l.MovePointer(); } 
};//optimizing? normally S and T are references, but not if of const methods
template<class T> static inline const T &PreConst(T &t){ return (const T&)t; }
template<class T> static inline const T *PreConst(T *t){ return (const T*)t; }
template<class T, int V> struct PreSet //just non-static member initializion
{ T set; PreSet(T v=(T)V):set(v){} inline operator T&(){ return set; } };
//HACK: This is a temporary workaround for MSVC2015's crashing compiler.
#define PRE_LIST_VECTOR_ONLY
template<class T, class Co> class PreList<T,Co>::
#include "pre-list.hpp"
//Assimp/texture.h
typedef struct PreTexture 
{    
	PRESERVE_FIRSTCLASS
	preSize metadata; preID *metadatalist;

  /////////////////////////////////////////
  //THIS CLASS SATISFIES Assimp's LOADERS//
  //UNTIL PreServer CAN FACILITATE IMAGES//
  /////////////////////////////////////////

	preN width,height;		
	char assimp3charcode_if0height[4];	
	int:32;
	#include "pre-texture.hpp" 
	//struct Color{ unsigned byte b,g,r,a; };	
	//Assimp union's these. colordata is compressed
	preSize colordata; unsigned char *colordatalist;	
	preSize colors; Color *colorslist;			
}preTexture;
//Assimp/mesh.h
typedef struct PreBone
{
	PRESERVE_FIRSTCLASS
	preSize metadata; 
	preID *metadatalist;
	preX node;
	preMatrix matrix; 	
	preSize weights;
	struct Weight 
	{ 	//hmm: float or double?
		typedef signed typeofkey; 
		typedef float typeofvalue;		
		#define typeof_this Weight
		#include "pre-key.hpp"
		Weight(){}		
	}*weightslist; 		
	#include "pre-bone.hpp"	
}preBone;		 
typedef struct PreMorph
{
	PRESERVE_FIRSTCLASS
	preSize metadata; 
	preID *metadatalist;
	preSize positions;
	pre3D *positionslist, *normalslist;
	pre3D *tangentslist, *bitangentslist;
	enum{ colorslistsN=8 };
	pre4D *colorslists[8];
	enum{ texturecoordslistsN=8 };
	pre3D *texturecoordslists[8];	
	#include "pre-morph.hpp" 
	const PreMesh2 *_complex;
	//-1 selects the base preMesh
	typedef PreTime<signed> Time;
}preMorph;
typedef struct PreVertex //experimental
{
	pre3D *position;
	pre3D *normal, *tangent, *bitangent;
	pre4D *colors[PreMorph::colorslistsN+1];
	pre3D *texturecoords[PreMorph::texturecoordslistsN+1];
}preVertex;
//Assimp DEVIATIONS////////////////////////////////
//In Assimp aiFace has its own new allocated indices
//Instead startindex looks into a per-mesh index list
//Or IOW f.mIndices[i]==m->indiceslist[f.startindex+i]
//A face can also be a point-cloud or a line-list, etc.
//SurfaceDetail tracks lost portions of smoothing groups
typedef struct PreFace
{	
	enum{ surfacedetail=-128 };
	const char _size1,_size2,_size;	
	union{ signed levelofdetail:8; char charLOD; };
	inline bool IsSurfaceDetail()const{ return charLOD==-128; }
	enum Polytype:int{ start=0,point,line,triangle,quadrangle };	
	union{ Polytype polytype; preN polytypeN; }; 
	enum{ start_polygons=0 };signed startindex;	
	inline operator bool()const{ return polytype!=start; }
	#include "pre-face.hpp" 
	//this operator may be transitional. PositionsRange is identical
	inline operator const Range&()const{ return (Range&)polytypeN; }
}preFace;
template<int Most> struct Pre8
{
	unsigned char _8; 
	inline operator preN()const{ return _8; }	
	template<int N> inline void SetEqualTo(){ PreSuppose(N<=Most); _8 = N; }
	template<class T> inline const T &operator=(const T &t){ assert(t<=Most); _8 = t; return t; }
};
typedef struct PreMesh : PreMorph
{	
	PRESERVE_FIRSTCLASS
	preX name;
	preSize faces;	
	preFace *faceslist;	
	preSize indices; 
	signed *indiceslist;
	//not implementing
	//enum{ texturedimensionsN=8 };	
	//Pre8<3> texturedimensions[8];
	preSize bones;
	preBone **boneslist;    
	preSize morphs;	
	preMorph **morphslist;		
	preID material;
	union{ unsigned _polytypesflags; 
	struct{ unsigned _pointsflag:1,_linesflag:1,_trianglesflag:1,_polygonsflag:1; };};
	#include "pre-mesh.hpp"
}preMesh;
//Assimp DOES NOT HAVE THESE STRUCTURES//////////////
//PreFace2 & PreMesh2 make it easy to map most formats
//to the straight 1-to-1 vertex-to-attribute of PreMesh
//by doing a straighforward top-to-bottom file traversal 
typedef struct PreFace2 : PreFace
{	
	preID material; //face's material index
	//means there is ONE normal/color index
	Pre8<1> normalflag, colorflag, _mustbezero[2]; 
	//if these are 0 the corresponding attribute
	//is not emitted with simplification. And if
	//2 or more the attributes are shared/packed
	//into normalslist for normals and bivectors
	//and into colorslists/texturecoordslists[0]
	//"back-to-back", so: [polytypeN][polytypeN]
	Pre8<3> normals; Pre8<8> colors, texturecoords, _mustbezero2[1];
	//indiceslist INDICES RELATIVE TO startindex
	signed normalsheadstart, colorsheadstart, texturecoordsheadstart;		
	#include "pre-face2.hpp"
	inline operator const Range&()const{ return (Range&)polytypeN; }

  ////SHARED INDICES////////////////////////////
  //There used to be flags for sharing indices//
  //But since the indices have been made to be//
  //outside the faces, they can simply overlap//
  //////////////////////////////////////////////

  ////SUBMESH PACKING///////////////////////////
  //Because packs are "back-to-back" and found//
  //by adding polytypeN, to avoid checking the//
  //sign of the index, pack submeshes backward//
  //////////////////////////////////////////////

}preFace2;
typedef struct PreMesh2 : PreMesh
{
	PRESERVE_FIRSTCLASS
	typedef void material;
	//these are counts; same as positions
	preSize normals, colors, texturecoords;
	preFace2 *faceslist; 
	struct Morph : PreMorph	//consistency	
	{ typedef void positions;
	Morph(PreMesh2 *mesh=0){ _complex = mesh; assert(mesh); }
	Morph(const Morph &cp, PreMesh2 *mesh=0):PreMorph(cp,mesh?mesh:_complex){};
	}**morphslist;		
	////OPTIONAL SHARED ADDRESSING MODEL////////////////////////////////////
	//Some file formats share mesh data, submesh directly maps to such files
	//Negative indices select from submesh. Its 0th index and 0th attributes
	//are inaccessible as 0 cannot be negative. Its indices are all negative
	//Submeshes are not emitted with simplification. Submeshes must be their
	//own submesh. Faces are unnecessary. Addressing is a function of i = -i
	PreMesh2 *submesh; inline bool IsSubmesh()const{ return submesh==this; }
	#include "pre-mesh2.hpp"	
}preMesh2;
//Assimp/light.h
typedef struct PreLight
{
	PRESERVE_FIRSTCLASS
	preSize metadata; preID *metadatalist;
	preX node;
	enum Shape:int{ directional=1,point,spot,ambient };
	Shape shape;int:32;
	pre3D position, direction;
	double constattenuate, linearattenuate, squareattenuate;
	pre3D diffusecolor, specularcolor, ambientcolor;
	double innercone, outercone;		
	#include "pre-light.hpp"
}preLight;
//Assimp/camera.h
typedef struct PreCamera
{
	PRESERVE_FIRSTCLASS
	preSize metadata; preID *metadatalist;
	preX node; 
	pre3D position, up, lookat;
	double halfofwidth_angleofview;
	double clipdistance, farclipdistance;
	double aspectratio;
	#include "pre-camera.hpp"
}preCamera;
//Assimp/material.h
typedef struct PreMaterial
{
	PRESERVE_FIRSTCLASS
	//NEW: please use ai:mat.name's metadata
	//preSize metadata; preID *metadatalist;
	enum Texture:int
	{
	_nontexture=0, 
	diffusetexture=1,
	speculartexture,
	ambienttexture,
	//Collada calls this <emission>
	emissiontexture,
	heightmap,bumpmap=heightmap,
	normalmap,
	//material.h says specular exponent
	//it's attractive to call this specularexponent
	//however Collada calls it <shininess> so we'll use that
	//(NOTE: other sources call this glossiness; it means flat/glossy)
	shininesstexture,
	//material.h calls this "opacity" and 
	//is not too clear that it's per component and 
	//so is really refracted light modulation by the material
	//Its Collada element is <transparent>
	transparenttexture,
	displacementmap,
	lightmap,ambientocclusion=lightmap, 
	//material.h makes this sound like an environment map
	//but it's basically the color of a mirror, so how light
	//changes color after it is reflected off the mirror surface
	//(NOTE: <reflective> is included in Collada's "COMMON" profile)
	reflectivetexture, 
	uncertaintexture, //12
	_Texture=100, //0~99 are Assimp's
	};
	enum Mapping:int
	{
	texturecoords2D=0,spheremapped,
	tubemapped,cubemapped,orthomapped, //4
	_Mapped=100, //0~99 are Assimp's
	texturecoords1D,texturecoords3D, 
	colors1D,colors2D,colors3D,colors4D,
	};
	enum Wrapping:int
	{
	wrapped=0,clamped,clipped,mirrored,	//3
	_Wrapping=100, //0~99 are Assimp's
	};
	enum Transfer:int
	{
	multiply=0,add,subtract,divide,
	smoothadd,summinusproduct=smoothadd,
	signedadd,addnegativehalf=signedadd, //5
	_Transfer=100, //0~99 are Assimp's
	};
	enum Tidbits:int 
	{//bitfield-like flags
	color_is_subtractive=1<<0,
	alpha_is_positively_included=1<<1,
	alpha_is_positively_NOT_included=1<<2,
	_Tidbits=1<<16, //0~15 are Assimp's
	};
	enum Shading:int
	{
	flatshaded=1,Gouraud,Phong,Blinn,
	cellshaded,OrenNayar,Minnaert,CookTorrance,notshaded,Fresnel, //10
	_Shading=100, //0~99 are Assimp's
	};
	enum Blending:int
	{
	alphablend=0, 
	accumulate, //source+destination
	_Blending=100, //0~99 are Assimp's
	};
	
	struct Property //metadata/direct access
	{
		PRESERVE_FIRSTCLASS	
		preN _sizeofdatalist; const int texturepair; 		
		inline preN TextureNumber()const{ return texturepair&0xFF; }
		inline Texture TextureCategory()const{ return Texture(texturepair>>16); }
		enum Type:int //0~99 are Assimp's
		{ floattype=1,stringtype=3,inttype,blobtype, _Type=100,doubletype };
		struct Key
		{
			unsigned _size:8, meta:1, _:23;
			Type storagetype; const char *permanentID;
			PreSet<size_t,0> _hopefully_this_forces_in_memory_return;
			Key(const char *id, Type s, int m=0):_size(sizeof(*this)),_(),
			meta(m!=0),storagetype(s),permanentID(id){ PreSuppose(sizeof(*this)<=255); }
		};
		struct Key255 //boundary-crossing key
		{ char key[255]; inline const Key *operator->()const{ return (Key*)key; } };
		//assuming consistently returned by value
		union{ Key(*const key)(); Key255(*_key255)(); };
		union{ char *_datalist; char _c[sizeof(void*)]; int _i,*_ip; float _f,*_fp; double *_dp; };
		#include "pre-materiality.hpp"		
		preSize metadata; preID *metadatalist;
	}**_propertieslist;
	preSize _properties, _allocated; //vector-like
	///////////////////////////////////////////////////
	//built-in keys. See "pre-material.hpp" for usage//
	///////////////////////////////////////////////////	
	//_default: See PreMaterial::PreMaterial(PreDefault)
	//white: diffuse/etc. default to grayscale pre4D(0~1) 
	//diffuselist/etc: is like sourceslist for colorslists
	#define _(ident,id,STRONGLY,TYPED,...) \
	static Property::Key(*ident(const STRONGLY,TYPED))(){ return _##ident; }\
	static Property::Key _##ident(){ return Property::Key(id,Property::__VA_ARGS__); }
	//BEFORE ADDING, UPDATE _GetTranslationUnitKeysList
	_(name,"ai:mat.name",preX&,void*,stringtype,'meta')	
	_(_default,"_pre:default",int&,void*,inttype)
	_(white,"pre:mat.white",float&,void*,floattype)
	_(twosided,"ai:mat.twosided",bool&,void*,inttype)
	_(shadingmodel,"ai:mat.shadingm",Shading&,void*,inttype)
	_(enablewireframe,"ai:mat.wireframe",bool&,void*,inttype)
	_(blendingmodel,"ai:mat.blend",Blending&,void*,inttype)
	_(transparency,"ai:mat.opacity",float&,void*,floattype)
	_(bumpscaling,"ai:mat.bumpscaling",float&,void*,floattype)
	_(shininess,"ai:mat.shininess",float&,void*,floattype)
	_(shininess_strength,"ai:mat.shinpercent",float&,void*,floattype)
	_(reflectivity,"ai:mat.reflectivity",float&,void*,floattype)
	_(index_of_refraction,"ai:mat.refracti",float&,void*,floattype)
	_(diffuse,"ai:clr.diffuse",pre4D&,void*,doubletype)	
	_(ambient,"ai:clr.ambient",pre4D&,void*,doubletype)
	_(specular,"ai:clr.specular",pre4D&,void*,doubletype)
	_(emission,"ai:clr.emissive",pre4D&,void*,doubletype)
	_(transparent,"ai:clr.transparent",pre4D&,void*,doubletype)
	_(reflective,"ai:clr.reflective",pre4D&,void*,doubletype)
	_(diffuselist,"pre:clr.diffuselist",preN&,void*,inttype)
	_(ambientlist,"pre:clr.ambientlist",preN&,void*,inttype)
	_(specularlist,"pre:clr.specularlist",preN&,void*,inttype)
	_(emissionlist,"pre:clr.emissionlist",preN&,void*,inttype)
	_(transparentlist,"pre:clr.transparentlist",preN&,void*,inttype)
	_(reflectivelist,"pre:clr.reflectivelist",preN&,void*,inttype)
	_(background,"ai:bg.global",preX&,void*,stringtype,'meta')
	_(texture,"ai:tex.file",preX&,Texture,stringtype)	
	_(texturename,"pre:tex.name",preX&,Texture,inttype,'meta')
	_(sourceslist,"ai:tex.uvwsrc",preN&,Texture,inttype)
	_(transfer,"ai:tex.op",Transfer&,Texture,inttype)
	_(mapping,"ai:tex.mapping",Mapping&,Texture,inttype)
	_(blendfactor,"ai:tex.blend",float&,Texture,floattype)
	_(wrapping,"pre:tex.mapmodeuvw",Wrapping(&)[3],Texture,inttype)
	_(mapping_axis,"ai:tex.mapaxis",pre3D&,Texture,doubletype)
	_(matrix,"pre:tex.transformuvw",preMatrix&,Texture,doubletype)
	_(tidbits,"ai:tex.flags",unsigned&,Texture,inttype)	
	_(_vformat,"_pre:vformat",int&,void*,inttype,'meta')
	_(_vformat2,"_pre:vformat2",int&,void*,inttype,'meta')
	_(_lost_in_copy,"_pre:lost_in_copy",int&,void*,inttype,'meta')
	_(_hole_in_copy,"_pre:hole_in_copy",int&,void*,inttype,'meta')
	#undef _
	#include "pre-material.hpp"	
}preMaterial;
typedef struct PreAnimation
{
	PRESERVE_FIRSTCLASS
	preSize metadata; preID *metadatalist;
	preX name;
	double duration,tickspersecond;
	preSize skinchannels;
	struct Skin
	{
		PRESERVE_FIRSTCLASS 
		preSize metadata; preID *metadatalist;
		preX node;
		preSize scalekeys; Pre3D::Time *scalekeyslist;
		preSize positionkeys; Pre3D::Time *positionkeyslist;
		preSize rotationkeys; PreQuaternion::Time *rotationkeyslist;		
		enum Edgecase:int{ matrixofnode=0,nearest,extrapolate,wrap };
		Edgecase startcase, endcase;
		preSize appendednodes; preX *appendednodeslist;
		#include "pre-skinimation.hpp"
	}**skinchannelslist;
	preSize morphchannels; 
	struct Morph
	{
		PRESERVE_FIRSTCLASS
		preSize metadata; preID *metadatalist;
		preX mesh;
		preSize morphkeys; PreMorph::Time *morphkeyslist;			
		preSize affectednodes; preX *affectednodeslist;
		#include "pre-morphimation.hpp"
	}**morphchannelslist; 	
	#include "pre-animation.hpp"
}preAnimation;
//Assimp/metadata.h
typedef struct PreMeta
{
	PRESERVE_FIRSTCLASS
	preSize metadata; preID *metadatalist;
	enum Category:int //visibility
	{
		logo=1,history, //past		
		story,text, //in-media
		model, //3D, on canvas (esp. Collada XML)
		music,vocal, sound,speech,
		none_of_the_above=-1, requires_analysis=0	
	}category;int:32;
	//UNFINISHED
	static const bool IsSupported = false;
	PRESERVE_LISTS(preID,MetaData,metadata)
	PreMeta():PRESERVE_VERSION{ PRESERVE_ZEROTHISAFTER(_size) }
	PreMeta(const PreMeta &cp, PreX::Pool *p):PRESERVE_VERSION
	{ PRESERVE_COPYTHISAFTER(_size) PRESERVE_COPYLIST(metadata) }
	~PreMeta(){ MetaDataList().Clear(); }
}preMeta;
//Assimp/scene.h
typedef struct PreNode
{
	PRESERVE_FIRSTCLASS
	preSize metadata; 
	preID *metadatalist;
	preX name; 
	preMatrix matrix;
	PreNode *node;
	preSize nodes;
	PreNode **nodeslist;	
	preSize meshes; 
	preID *mesheslist;	
	#include "pre-node.hpp"
}preNode;
typedef struct PreScene
{
	PRESERVE_FIRSTCLASS	
	preX name;
	preNode *rootnode;
	preSize meshes;
	//see complexSceneOut
	preMesh **mesheslist;
	preMesh2 **mesheslist2;
	preSize materials;
	preMaterial **materialslist;
	preSize animations;
	preAnimation **animationslist;
	preSize embeddedtextures;    
	preTexture **embeddedtextureslist;
	preSize lights; preLight **lightslist;
	preSize cameras; preCamera **cameraslist;
	preSize metadata; preMeta **metadatalist;
	//these are delete'd. See PreX::IsPooled	
	preSize pooledstrings; char **pooledstringslist;
	#include "pre-scene.hpp"
	//use "new PreScene" or override this
	void (*serverside_delete)(PreScene*);
	int:32;//WILL BE OBSOLETE
	unsigned incompleteflag:1,//???
	_validatedflag:1,_validation_warningflag:1,
	_non_verbose_formatflag:1,_terrainflag:1; 	
}preScene;
//Assimp/config.h
typedef union PreInterestedIn
{
	struct //aiComponent
	{ unsigned long long	
	photography:1, lighting:1,
	weights:1, morphs:1, meshes:1, 
	perVertexColor:1, textureCoords:1,
	lightingNormals:1, tangentBivectors:1, 
	materials:1, textures:1, embeddedImages:1,
	animations:1, skinChannels:1, morphChannels:1, 	
	nonColladaMetadata:1, Collada:1, ColladaVisuals:1,
	pointCloudLOD:1, browserFriendlyLOD:1, theFinestLOD:1; //22
	}; unsigned long long _interestingBits;
	PreInterestedIn():_interestingBits(~0ULL){}
}preInterestedIn;
/////////////////
typedef struct PreServer
{   
	PRESERVE_FIRSTCLASS	
	int Size(){return _size;};
	const preX pathtomoduleIn;
	//optional MIME Media Type
	//(see clog's notes below)
	const preX resourcetypeIn;
	const preX resourcenameIn;		
	//for files it is memory-mapped
	//(it's writable/copy-on-write)
	const size_t amountofcontentIn;
	unsigned char *const contentIn;	
	//optimizing: non-binding whitelist 
	const preInterestedIn interestedIn;
	//first log lines that are
	//like a "mime.types" file
	void (*clog)(const char*);
	void (*clogVerbose)(const char*);	
	//if PreServe fails the last issue will
	//be assumed to be the cause of failure
	//TODO? a list of numerical error codes
	void (*clogCriticalIssue)(const char*);
	//periodic progression callback wherein
	//place is the return value of PreServe 
	//0 per_mille tracks unbounded progress
	void (*prog)(int place, int per_mille);
	void (*progLocalFileName)(const char*);		
	//////////////////////////////////
	//reserved: 0 uses Assimp's math//
	//set to 1 when single-precision//
	unsigned long long mathematicsOut;
	//NOTE: modules may want to load//
	//Collada XML. This is much more//
	//work. If so the plan is to add//
	//it to the scene's metadatalist//
	//It will not be processed if so//	
	//==============================//
	//position of next logical scene//
	const unsigned char *nextSceneOut;
	//if using mesheslist2 use complexSceneOut
	preScene *simpleSceneOut, *complexSceneOut;
	preScene *terrainSceneOut; //_terrainflag:1		
	void *serverside_userdata;
	#include "pre-server.hpp"
}preServer;
//PreServe must be exported by modules
//It should write to the log, and when
//contentIN is 0, simply return with 0
//serverside_delete deletes the scenes
//(in interim a transcription is made)
//====================================
//Modules MUST BE THREADSAFE. They can
//block if they must; please do log if
//doing so! A 0 call can arrive at any
//time to display your log information
//////////////////////////////////////
//IN THE EVENT that contentIn contains
//more than one logical scene, this is
//indicated by setting nextSceneOut to
//a position past contentIn. This will
//trigger subsequent sessions starting
//at nextSceneOut instead of contentIn
//(progress updates are inter-session)
//====================================
//If resourcenameIn contains #, that's
//a logical request for PreScene::name 
//Set the name on RETURN if successful
//////////////////////////////////////
//int RETURNS the first error byte; or
//if successful (int)amountofcontentIn
//If 0, the last prog position is used
//====================================
//COMPRESSED FORMATS upon error should
//negate their offset. It is a logical
//byte offset into the virtual file in
//which the error is encountered. More
//information can be provided via clog
//////////////////////////////////////
//Last but not least, ENSURE THAT Size
//IS AT LEAST sizeof(preServer) so you
//do not read/write past the end of it
DAEDALUS_API int PreServe(preServer*);
#endif //PRESERVE_H_INCLUDED
