
#include "x2mdl.pch.h" //PCH

#ifndef _CONSOLE //2021
void MDL::File::x2mm3d(FILE *output){ assert(0); }
#else

#include <vector> //need be?
#include <string> //need be?

#include "../lib/swordofmoonlight.h" //integer types

typedef float float32_t;

extern bool x2mm3d_lerp = false;

//same as x2mdl.cpp
static const short invert[3] = {+1,-1,-1};

// Misfit MM3D format:
//
// File Header
//
// MAGIC_NUMBER	 8 bytes  "MISFIT3D"
// MAJOR_VERSION	uint8	 0x01
// MINOR_VERSION	uint8	 0x05
// MODEL_FLAGS	  uint8	 0x00 (reserved)
// OFFSET_COUNT	 uint8	 [offc]
//
//	 [OFFSET_COUNT] instances of:
//	 Offset header list
//	 OFFSET_HEADER	6 bytes *[offc]
//
//	 Offset Header
//	 OFFSET_TYPE	  uint16 (highest bit 0 = Data type A,1 = data type B)
//	 OFFSET_VALUE	 uint32 
//
// Data header A (Variable data size)
// DATA_FLAGS		uint16 
// DATA_COUNT		uint32 
//
// Data block A
//
// [DATA_COUNT] instances of:
// DATA_SIZE		 uint32 bytes
// DATA_BLOCK_A	 [DATA_SIZE] bytes
//
// Data header B (Uniform data size)
// DATA_FLAGS		uint16 
// DATA_COUNT		uint32 
// DATA_SIZE		 uint32 
//
// Data block B
//
// [DATA_COUNT] instances of:
// DATA_BLOCK_B	[DATA_SIZE] bytes
//
//
// Offset A types:
//	0x1001			 Meta information
//	0x1002			 Unknown type information
//	0x0101			 Groups
//	0x0141			 Embedded textures
//	0x0142			 External textures
//	0x0161			 Materials
//	0x016c			 Texture Projection Triangles
//	0x0191			 Canvas background images
//
//	0x0301			 Skeletal Animations
//	0x0321			 Frame Animations
//	0x0326			 Frame Animation Points
//	0x0341			 Frame Relative Animations
//
// Offset B types:
//	0x0001			 Vertices
//	0x0021			 Triangles
//	0x0026			 Triangle Normals
//	0x0041			 Joints
//	0x0046			 Joint Vertices
//	0x0061			 Points
//	0x0106			 Smooth Angles
//	0x0168			 Texture Projections
//	0x0121			 Texture Coordinates
//
//
// 
// Meta information data
//	KEY				 ASCIIZ <=1024 (may not be empty)
//	VALUE			  ASCIIZ <=1024 (may be empty)
//
// Unknown type information
//	OFFSET_TYPE	  uint16
//	INFO_STRING	  ASCIIZ<=256 (inform user how to support this data)
//
//
// Vertex data
//	FLAGS			  uint16
//	X_COORD			float32
//	Y_COORD			float32
//	Z_COORD			float32
//
//
// Triangle data
//	FLAGS			  uint16
//	VERTEX1_INDEX	uint32
//	VERTEX2_INDEX	uint32
//	VERTEX3_INDEX	uint32
//
// Group data
//	FLAGS			  uint16
//	NAME				ASCIIZ<=inf
//	TRI_COUNT		 uint32
//	TRI_INDICES	  uint32 *[TRI_COUNT]
//	SMOOTHNESS		uint8
//	MATERIAL_INDEX  uint32
//	
// Smooth Angles (maximum angle between smoothed normals for a group)
//	GROUP_INDEX	  uint32
//	SMOOTHNESS		uint8
//	
// Weighted Influences
//	POS_TYPE		  uint8
//	POS_INDEX		 uint32
//	INF_INDEX		 uint32
//	INF_TYPE		  uint8
//	INF_WEIGHT		uint8
//	
// External texture data
//	FLAGS			  uint16
//	FILENAME		  ASCIIZ
//
// Embedded texture data
//	FLAGS			  uint16
//	FORMAT			 char8 *4  ('JPEG','PNG ','TGA ',etc...)
//	TEXTURE_SIZE	 uint32
//	DATA				uint8 *[TEXTURE_SIZE]
//
// Material
//	FLAGS			  uint16
//	TEXTURE_INDEX	uint32
//	NAME				ASCIIZ
//	AMBIENT			float32 *4
//	DIFFUSE			float32 *4
//	SPECULAR		  float32 *4
//	EMISSIVE		  float32 *4
//	SHININESS		 float32
//	
// Projection Triangles
//	PROJECTION_INDEX  uint32
//	TRI_COUNT			uint32
//	TRI_INDICES		 uint32 *[TRI_COUNT]
//	
// Canvas background image
//	FLAGS			  uint16
//	VIEW_INDEX		uint8
//	SCALE			  float32
//	CENTER_X		  float32
//	CENTER_X		  float32
//	CENTER_X		  float32
//	FILENAME		  ASCIIZ
//
// Texture coordinates
//	FLAGS			  uint16
//	TRI_INDEX		 uint32
//	COORD_S			float32 *3
//	COORD_T			float32 *3
//
// Joint data
//	FLAGS			  uint16
//	NAME				char8 *40
//	PARENT_INDEX	 int32
//	LOCAL_ROT		 float32 *3
//	LOCAL_TRANS	  float32 *3
//
// Joint Vertices
//	VERTEX_INDEX	 uint32
//	JOINT_INDEX	  int32
//
// Point data
//	FLAGS			  uint16
//	NAME				char8 *40
//	TYPE				int32
//	BONE_INDEX		int32
//	ROTATION		  float32 *3
//	TRANSLATION	  float32 *3
//
// Texture Projection 
//	FLAGS			  uint16
//	NAME				char8 *40
//	TYPE				int32
//	POSITION		  float32 *3
//	UP VECTOR		 float32 *3
//	SEAM VECTOR	  float32 *3
//	U MIN			  float32
//	V MIN			  float32
//	U MAX			  float32
//	V MAX			  float32
//
// Skeletal animation
//	FLAGS			  uint16
//	NAME				ASCIIZ
//	FPS				 float32
//	FRAME_COUNT	  uint32
//
//		[FRAME_COUNT] instances of:
//		KEYFRAME_COUNT  uint32
//
//		  [KEYFRAME_COUNT] instances of:
//		  JOINT_INDEX	  uint32
//		  KEYFRAME_TYPE	uint8  (0 = rotation,1 = translation)
//		  PARAM			  float32 *3
//
// Frame animation
//	FLAGS			  uint16
//	NAME				ASCIIZ<=64
//	FPS				 float32
//	FRAME_COUNT	  uint32
//
//		[FRAME_COUNT] instances of:
//		
//			[VERTEX_COUNT] instances of:
//			COORD_X			float32
//			COORD_Y			float32
//			COORD_Z			float32
// 
// Frame animation points
//	FLAGS				 uint16
//	FRAME_ANIM_INDEX  uint32
//	FRAME_COUNT		 uint32
//
//		[FRAME_COUNT] instances of:
//		
//			[POINT_COUNT] instances of:
//			ROT_X			float32
//			ROT_Y			float32
//			ROT_Z			float32
//			TRANS_X		 float32
//			TRANS_Y		 float32
//			TRANS_Z		 float32
// 
// Frame relative animation
//	FLAGS			  uint16
//	NAME				ASCIIZ<=64
//	FPS				 float32
//	FRAME_COUNT	  uint32
//
//		[FRAME_COUNT] instances of:
//		FVERT_COUNT	  uint32
//		
//			[FVERT_COUNT] instances of:
//			VERTEX_INDEX
//			COORD_X_OFFSET  float32
//			COORD_Y_OFFSET  float32
//			COORD_Z_OFFSET  float32
// 

#include "../lib/pack.inl" /*push*/

struct MisfitOffsetT
{
	uint16_t offsetType;
	uint32_t offsetValue;
}SWORDOFMOONLIGHT_PACK;

enum MisfitDataTypesE
{
	// A Types
	MDT_Meta,
	MDT_TypeInfo,
	MDT_Groups,
//	MDT_EmbTextures, //UNIMPLEMENTED //BINARY PLACEHOLDER?
	MDT_ExtTextures,
	MDT_Materials,
	MDT_ProjectionTriangles,
	MDT_CanvasBackgrounds,
	MDT_SkelAnims,
	MDT_FrameAnims,
	MDT_FrameAnimPoints,
//	MDT_RelativeAnims, //UNIMPLEMENTED //BINARY PLACEHOLDER?

	MDT_Animations, //2021

	// B Types
	MDT_Vertices,
	MDT_Triangles,
	MDT_TriangleNormals, //IGNORED //NO, NEEDED by x2mdl.dll
	MDT_Joints,
	MDT_JointVertices,
	MDT_Points,
	MDT_SmoothAngles,
	MDT_WeightedInfluences,
	MDT_TexProjections,
	MDT_TexCoords,

	MDT_ScaleFactors, //2021

	// End of list
	MDT_EndOfFile,
	MDT_MAX
};

enum MisfitFlagsE
{
	MF_HIDDEN	 = 1, // powers of 2
	MF_SELECTED  = 2,
	MF_VERTFREE  = 4, // vertex does not have to be conntected to a face

	// Type-specific flags
	MF_MAT_CLAMP_S = 16,
	MF_MAT_CLAMP_T = 32,

	MF_MAT_ACCUMULATE = 256, //2021 (mm3d2021)
};

enum MisfitFrameAnimFlagsE
{
	MFAF_ANIM_LOOP = 0x0001,
};

enum MisfitSkelAnimFlagsE
{
	MSAF_ANIM_LOOP = 0x0001
};

static const uint16_t MisfitOffsetTypes[MDT_MAX] = 
{

	// Offset A types
	0x1001,		  // Meta information
	0x1002,		  // Unknown type information
	0x0101,		  // Groups
//	0x0141,		  // Embedded textures
	0x0142,		  // External textures
	0x0161,		  // Materials
	0x016c,		  // Texture Projection Triangles
	0x0191,		  // Canvas background images
	0x0301,		  // Skeletal Animations
	0x0321,		  // Frame Animations
	0x0326,		  // Frame Animation Points
//	0x0341,		  // Frame Relative Animations

	0x0342, //2021 Animations

	// Offset B types:
	0x0001,		  // Vertices
	0x0021,		  // Triangles
	0x0026,		  // Triangle Normals
	0x0041,		  // Joints
	0x0046,		  // Joint Vertices
	0x0061,		  // Points
	0x0106,		  // Smooth Angles
	0x0146,		  // Weighted Influences
	0x0168,		  // Texture Projections
	0x0121,		  // Texture Coordinates

	0x0180, //2021 Scale Factors

	0x3fff,		  // End of file
};

// File header
struct MM3DFILE_HeaderT
{
	char magic[8];
	uint8_t versionMajor;
	uint8_t versionMinor;
	uint8_t modelFlags; //UNUSED
	uint8_t offsetCount;
}SWORDOFMOONLIGHT_PACK;

// Data header A (Variable data size)
struct MM3DFILE_DataHeaderAT
{
	uint16_t flags;
	uint32_t count;
}SWORDOFMOONLIGHT_PACK;

// Data header B (Uniform data size)
struct MM3DFILE_DataHeaderBT
{
	uint16_t flags;
	uint32_t count;
	uint32_t size;
}SWORDOFMOONLIGHT_PACK;

struct MM3DFILE_VertexT
{
	uint16_t  flags;
	float32_t coord[3];
}SWORDOFMOONLIGHT_PACK;

const size_t FILE_VERTEX_SIZE = 14;

struct MM3DFILE_TriangleT
{
	uint16_t  flags;
	uint32_t  vertex[3];
}SWORDOFMOONLIGHT_PACK;

const size_t FILE_TRIANGLE_SIZE = 14;

struct MM3DFILE_TriangleNormalsT
{
	uint16_t	flags;
	uint32_t	index;
	float32_t  normal[3][3];
}SWORDOFMOONLIGHT_PACK;

const size_t FILE_TRIANGLE_NORMAL_SIZE = 42;

struct MM3DFILE_JointT
{
	uint16_t  flags;
	char		name[40];
	int32_t	parentIndex;
	float32_t localRot[3];
	float32_t localTrans[3];
	//float32_t localScale[3]; //2020
};

const size_t FILE_JOINT_SIZE = 70; //70+12;

struct MM3DFILE_JointVertexT
{
	uint32_t  vertexIndex;
	int32_t	jointIndex;
}SWORDOFMOONLIGHT_PACK;

const size_t FILE_JOINT_VERTEX_SIZE = 8;

struct MM3DFILE_WeightedInfluenceT
{
	uint8_t	posType;
	uint32_t  posIndex;
	uint32_t  infIndex;
	uint8_t	infType;
	int8_t	 infWeight;
}SWORDOFMOONLIGHT_PACK;

const size_t FILE_WEIGHTED_INFLUENCE_SIZE = 11;

struct MM3DFILE_PointT
{
	uint16_t  flags;
	char		name[40];
	int32_t	type;
	int32_t	boneIndex;
	float32_t rot[3];
	float32_t trans[3];
	//float32_t scale[3]; //2020
}SWORDOFMOONLIGHT_PACK;

const size_t FILE_POINT_SIZE = 74; //74+12;

struct MM3DFILE_SmoothAngleT
{
	uint32_t  groupIndex;
	uint8_t	angle;
}SWORDOFMOONLIGHT_PACK;

const size_t FILE_SMOOTH_ANGLE_SIZE = 5;

struct MM3DFILE_CanvasBackgroundT
{
	uint16_t  flags;
	uint8_t	viewIndex;
	float32_t scale;
	float32_t center[3];
}SWORDOFMOONLIGHT_PACK;

const size_t FILE_CANVAS_BACKGROUND_SIZE = 19;

/*struct MM3DFILE_KeyframeT
{
	//2020: high-order 16b hold type/interp mode.
	uint32_t objectIndex;
	//0 is Rotate, 1 Translate, 2 Scale
	uint8_t	 keyframeType;
	float32_t  param[3];
}SWORDOFMOONLIGHT_PACK;

const size_t FILE_KEYFRAME_SIZE = 17;*/

struct MM3DFILE_TexCoordT
{
	uint16_t  flags;
	uint32_t  triangleIndex;
	float32_t sCoord[3];
	float32_t tCoord[3];
}SWORDOFMOONLIGHT_PACK;

const size_t FILE_TEXCOORD_SIZE = 30;

const size_t FILE_TEXTURE_PROJECTION_SIZE = 98;

struct MM3DFILE_ScaleFactorT
{
	uint16_t type;
	uint16_t index;
	float32_t scale[3];
};

const size_t FILE_SCALE_FACTOR_SIZE = 16;

struct UnknownDataT
{
	uint16_t offsetType;
	uint32_t offsetValue;
	uint32_t dataLen;
}SWORDOFMOONLIGHT_PACK;

#include "../lib/pack.inl" /*pop*/

typedef std::vector<UnknownDataT> UnknownDataList;

namespace MisfitFilter //ADAPTED
{
	//Model::ModelErrorE readFile(Model *model, const char *const filename);
	//Model::ModelErrorE writeFile(Model *model, const char *const filename, Options&);
		
	static const char MAGIC[] = "MM3D2020"; //"MISFIT3D";

	static const uint8_t WRITE_VERSION_MAJOR = 2; //0x01;
	static const uint8_t WRITE_VERSION_MINOR = 1; //0x07;

	static const uint16_t OFFSET_TYPE_MASK = 0x3fff;
	static const uint16_t OFFSET_UNI_MASK = 0x8000;

	//Just implementing minimum subset.
	//DataSource *m_src;
	//DataDest *m_dst;
	//size_t m_readLength;	
	struct Data : FILE
	{
		template<class T> void write(const T &t)
		{
			fwrite(&t,sizeof(T),1,this);
		}
		void writeBytes(const void *p, size_t len)
		{
			fwrite(p,len,1,this);
		}

		void seek(long off)
		{
			fseek(this,off,SEEK_SET);
		}

		long offset(){ return ftell(this); }
	};
	static union{ Data *m_src,*m_dst; };

	static bool doWrite[MDT_MAX] = {};

	//static void read(float32_t &val);
	//static void write(float32_t val);
	//static void writeBytes(const void *buf, size_t len);
	
	static void writeHeaderA(uint16_t flags, uint32_t count)
	{
		m_dst->write(flags); m_dst->write(count);
	}
	static void writeHeaderB(uint16_t flags,uint32_t count,uint32_t size)
	{
		m_dst->write(flags); m_dst->write(count); m_dst->write(size);
	}

	/*static void readHeaderA(uint16_t &flags,uint32_t &count)
	{
		m_src->read(flags); m_src->read(count);
	}
	static void readHeaderB(uint16_t &flags, uint32_t &count,u int32_t &size)
	{
		m_src->read(flags); m_src->read(count); m_src->read(size);
	}*/
}

struct MisfitOffsetList : std::vector<MisfitOffsetT> //ADAPTED
{
	//WRITING //WRITING //WRITING //WRITING //WRITING //WRITING //WRITING 

	void addOffset(MisfitDataTypesE type)
	{
		if(MisfitFilter::doWrite[type])
		{
			MisfitOffsetT mo;
			mo.offsetType = MisfitOffsetTypes[type];
			mo.offsetValue = 0;
		//	log_debug("adding offset type %04X\n",mo.offsetType);
			push_back(mo);
		}
	}

	void setOffset(MisfitDataTypesE type)
	{
		for(MisfitOffsetT&ea:*this)
		if((ea.offsetType&MisfitFilter::OFFSET_TYPE_MASK)==MisfitOffsetTypes[type])
		{
			ea.offsetValue = MisfitFilter::m_dst->offset();
		//	log_debug("updated offset for %04X to %08X\n",ea.offsetType,ea.offsetValue);
			break;
		}
	}

	void setUniformOffset(MisfitDataTypesE type, bool uniform)
	{
		for(MisfitOffsetT&ea:*this)
		if((ea.offsetType&MisfitFilter::OFFSET_TYPE_MASK)==MisfitOffsetTypes[type])
		{
			if(uniform)
			{
			//	log_debug("before uniform: %04X\n",ea.offsetType);
				ea.offsetType |= MisfitFilter::OFFSET_UNI_MASK;
			//	log_debug("after uniform: %04X\n",ea.offsetType);
			}
			else
			{
			//	log_debug("before variable: %04X\n",ea.offsetType);
				ea.offsetType &= MisfitFilter::OFFSET_TYPE_MASK;
			//	log_debug("after variable: %04X\n",ea.offsetType);
			}
			break;
		}
	}	
};

extern char texindex[];
extern std::vector<std::wstring> texnames;
typedef unsigned char Color[4];
extern Color controlpts[];
extern float materials[][4],materials2[][4];

extern std::vector<std::pair<int,int>> x2mm3d_mats;

void x2mm3d_sample2(aiVector3D &cmp, aiNode *p)
{
	for(int i=3;i-->0;) cmp[i] = p->mTransformation[3][i];
}
void x2mm3d_sample2(aiQuaternion &cmp, aiNode *p)
{
	cmp = aiMatrix3x3(p->mTransformation); 
}
template<class T>
void x2mm3d_sample(T &cmp, T *b, T *e, aiNode *p)
{
	float t = cmp.mTime; 
	T *lb = std::lower_bound(b,e,cmp);
	if(lb==e) //Clamp?
	{	
		//What about wrap?

		cmp.mValue = e[-1].mValue; return;
	}
	else if(lb->mTime==cmp.mTime)
	{
		cmp.mValue = lb->mValue; return;
	}
	else if(lb==b)
	{
		cmp.mTime = 0; 
		x2mm3d_sample2(cmp.mValue,p);
		b = &cmp; 
	}
	else b = lb-1; e = lb;

	t = (float)((t-b->mTime)/(e->mTime-b->mTime));

	Assimp::Interpolator<T> ipl; ipl(cmp.mValue,*b,*e,t);
}

void x2mm3d_gather_points(std::vector<aiNode*> &pts, aiNode *n)
{
	for(int i=0;i<n->mNumChildren;i++) 
	{
		aiNode *nn = n->mChildren[i];

		//TODO: need to extend this to meshes for 
		//other formats
		if(!nn->mChildren)
		{
			//CPs names must be put inside parentheses.
			if('('==nn->mName.data[0])
			{
				if(nn->mNumMeshes) 
				{
					if(1!=nn->mNumMeshes) continue;
					
					aiMesh *m = X->mMeshes[nn->mMeshes[0]];
					
					if(!m->mFaces) continue;
					
					if(1!=m->mFaces[0].mNumIndices) continue;

					if(aiVector3D*v=m->mVertices)
					{
						v+=m->mFaces[0].mIndices[0];

						for(int i=3;i-->0;) if((*v)[i]) 
						{
							assert(0); //fix me

							goto nz; //continue; //wrong
						}
					}
				}
					
				pts.push_back(nn); nz:;
			}
		}
		else x2mm3d_gather_points(pts,nn);
	}
}

extern std::vector<aiNode*> x2mm3d_nodetable;
void x2mm3d_copy_name(char *buf, int sz, aiString &name)
{
	char *n = name.data;
	int len = name.length;
	if(len&&*n=='<'&&'>'==n[len-1])
	{
		n++; len-=2; //MM3D assigns special meaning to <> strings.
	}
	len = std::min(sz-1,len); memcpy(buf,n,len); buf[len] = '\0';
}

void MDL::File::x2mm3d(FILE *output) //MisfitFilter::writeFile
{	
	uint32_t count; //REMOVE ME
	
	////NOTES ON PRECISION////
	// 	   
	// originally this had used x2mdl.cpp's output but
	// that loses precision for non-MDL files, and the
	// MDL format can't wrap UVs, which o028.mdo needs
	// 
	//////////////////////////

	//HACK? in order to retain precision and 
	//support UV wrapping the original Assimp
	//vertices must be mapped
	extern aiNode **nodetable;
	extern char *chanindex;
	std::vector<aiVector3D*> rcverts;
	{
		//this is because x2mdl.cpp is merging
		//soft models so they're only one part
		//(there is a facility for a multipart
		//soft model but it's not been tested)
		int cverts0 = 0;
		/*seems not used
		int cvertsN = 0; //if(diffs) //YUCK
		{
			for(int k=0;k<head.parts;k++)
			{
				Part &pt = parts[k];
				cvertsN+=pt.verts+pt.extra;
			}
		}*/

		//auto count = (uint32_t)modelVertices.size();
		for(int i=count=0;i<head.parts;i++)
		count+=parts[i].verts;

		rcverts.resize(count);
		
		int currentnode = 0;
		for(int i=0;i<head.parts;i++) 
		{
			Part &pt = *parts[i].cpart;
			int cverts00 = cverts0;
			//if(cvertsN)
			cverts0+=pt.verts;
			if(pt.cextra) continue; //I think?

			/*2020: vertices may be in different order if 
			//converting/consolidating points to triangle
			//control points
			while(nodetable[currentnode]->mNumMeshes==0)
			currentnode++;*/
			currentnode = pt.cnode;

			auto *cv = pt.cverts, *cv2 = cv; 	

			if(!cv) //crashing below (x2mm3d_convert_points?)
			{
				assert(pt.verts==0); continue;
			}

			//int currentchan = chanindex?chanindex[currentnode]:0; 
			for(int j=0;j<nodetable[currentnode]->mNumMeshes;j++,cv=cv2)
			{
				aiMesh *m = X->mMeshes[nodetable[currentnode]->mMeshes[j]];

				if(cv) cv2 = cv+m->mNumVertices;

				if(m->mPrimitiveTypes>=aiPrimitiveType_TRIANGLE)
				{
					int k = m->mNumVertices;
					auto *vp = m->mVertices+k;
					while(k-->0) 
					rcverts[cverts00+cv[k]] = --vp;
				}
			}
		}
	}

	using namespace MisfitFilter;
	(void*&)MisfitFilter::m_dst = output;

	int compile[4==sizeof(float32_t)];

	std::vector<aiNode*> pts;
	x2mm3d_gather_points(pts,X->mRootNode);

	// Find out what sections we need to write
	memset(doWrite,0x00,sizeof(doWrite));
	doWrite[MDT_Vertices] = true;
	doWrite[MDT_Triangles] = true;
	doWrite[MDT_TriangleNormals] = true;
	doWrite[MDT_ExtTextures] = !texnames.empty();
	doWrite[MDT_Materials] = true;
	doWrite[MDT_Groups] = true;
	doWrite[MDT_TexCoords] = true;
	//TODO? can x2mm3d_nodetable be used when there's no
	//animation data? is that desirable?
	doWrite[MDT_Joints] = anims; //chans;
	doWrite[MDT_Points] = !pts.empty();

		//IMPLEMENT ME (hard animation scaling encoding)
		//REMINDER: there's some disabled code already
		//written for this below.
		//doWrite[MDT_ScaleFactors] = false;

	doWrite[MDT_WeightedInfluences] = doWrite[MDT_Joints];
	//doWrite[MDT_SkelAnims] = anims;
	//doWrite[MDT_FrameAnims] = diffs!=nullptr;	
	//doWrite[MDT_FrameAnimPoints] = diffs!=nullptr&&!pts.empty();
	doWrite[MDT_Animations] = anims||diffs;
	doWrite[MDT_EndOfFile] = true;

	// Write header
	MisfitOffsetList offsetList;
//	offsetList.addOffset(MDT_Meta);
	offsetList.addOffset(MDT_Vertices);
	offsetList.addOffset(MDT_Triangles);	
	offsetList.addOffset(MDT_TexCoords);
	offsetList.addOffset(MDT_TriangleNormals);
	offsetList.addOffset(MDT_ExtTextures);
	offsetList.addOffset(MDT_Materials);
	offsetList.addOffset(MDT_Groups);
	offsetList.addOffset(MDT_Joints);
	offsetList.addOffset(MDT_Points);
		//offsetList.addOffset(MDT_ScaleFactors);
	offsetList.addOffset(MDT_WeightedInfluences);	
	//offsetList.addOffset(MDT_SkelAnims);
	//offsetList.addOffset(MDT_FrameAnims);
	//offsetList.addOffset(MDT_FrameAnimPoints);
	offsetList.addOffset(MDT_Animations);	
	offsetList.addOffset(MDT_EndOfFile);

	uint8_t modelFlags = 0x00; //UNUSED
	auto offsetCount = (uint8_t)offsetList.size();

	m_dst->writeBytes(MAGIC,strlen(MAGIC));
	m_dst->write(WRITE_VERSION_MAJOR);
	m_dst->write(WRITE_VERSION_MINOR);
	m_dst->write(modelFlags); //UNUSED
	m_dst->write(offsetCount);
	for(MisfitOffsetT&mo:offsetList)
	{
		m_dst->write(mo.offsetType);
		m_dst->write(mo.offsetValue);
	}
//	log_debug("wrote %d offsets\n",offsetList.size());

	// Write data

	std::vector<int> holes; 
	holes.resize(head.parts);

	//I give up... this removes CP vertices
	//but can't be used since soft animations
	//include the vertices
	count = 0; 
	{
		//this is a debacle to remove
		//CP holes... I feel like there
		//has to be a way to do this inside
		//of consolidate() ... but I can't
		//think of a safe way to. building
		//rcverts is kind of a requirement

		int vbase = 0;
		for(int ii=0;ii<head.parts;ii++) 
		{
			Part &pt = *parts[ii].cpart;
			int i = &pt-parts;

			int n = pt.verts;
			if(!n) continue; //OOB...			
			auto *p = &rcverts[vbase];
			vbase+=n;
			for(int j=0;j<n;j++,p++)
			{
				if(*p){ count++; continue; }

				int jj = j;
				while(!*p&&j<n){ j++; p++; }

				int delta = j-jj; 

				int cmp = j-holes[i]; j--; p--;
				
					holes[i]+=delta;			

				auto *q = pt.cverts; //DESTRUCTIVE
				for(int k=pt.mm3d_cvertsz;k-->0;)
				{
					if(q[k]>=cmp)
					{
						q[k]-=delta;
					}
				}
			}
		}
	}
	const auto vcount = count;
	//if(doWrite[MDT_Vertices])
	{
		offsetList.setOffset(MDT_Vertices);
		offsetList.setUniformOffset(MDT_Vertices,true);

		writeHeaderB(0x0000,vcount,FILE_VERTEX_SIZE);

		MM3DFILE_VertexT fileVertex;
		fileVertex.flags = 0x0000;
		auto &v = *(aiVector3D*)fileVertex.coord;

		int rcv = 0;
		/*have to transform via bindposemats :(
		for(size_t i=0,n=rcverts.size();i<n;i++)
		{
			if(auto*vp=rcverts[i]) //YUCK CP residue?
			{
				v = *vp; m_dst->write(fileVertex);
			}
		}*/
		int currentnode = 0;
		for(int i=0;i<head.parts;i++) 
		{
			Part &pt = *parts[i].cpart;
			if(pt.cextra) continue; //I think?

			/*2020: vertices may be in different order if 
			//converting/consolidating points to triangle
			//control points
			while(nodetable[currentnode]->mNumMeshes==0)
			currentnode++;*/
			currentnode = pt.cnode;

			int currentchan = chanindex?chanindex[currentnode]:0; 
			
			for(int k=pt.verts;k-->0;) if(auto*vp=rcverts[rcv++])
			{
				v = *vp;

				extern float *bindposemats;
				if(float*xf=bindposemats)
				SWORDOFMOONLIGHT::mdl::multiply(xf+currentchan*16,&v.x,&v.x,1);
	
				m_dst->write(fileVertex);
			}
		}

	//	log_debug("wrote %d vertices\n",count);
	}

	// Triangles // Texture coordinates
	//if(doWrite[MDT_Triangles]||doWrite[MDT_TexCoords]||MDT_TriangleNormals)
	{
		//if(!packs) goto err;

		auto mat = x2mm3d_mats.begin();

		MM3DFILE_TriangleT fileTriangle;
		fileTriangle.flags = 0x0000;
		MM3DFILE_TexCoordT tc;
		tc.flags = 0x0000;
		tc.triangleIndex = 0;
		MM3DFILE_TriangleNormalsT fileNormals;
		fileNormals.flags = 0x0000;		
		fileNormals.index = 0;

		//auto count = (uint32_t)modelTriangles.size();
		uint32_t count = 0;
		for(MDL::Pack **p=packs;*p;p++)
		{
			//ASSUMING TRIANGLES ONLY!!
			count+=(*p)->size; 
		}

		float tu,tv;
		for(int mi=-1,pass=1;pass<=3;pass++)
		{
			MisfitDataTypesE off;
			uint32_t off2; if(pass==1)
			{
				off = MDT_Triangles; off2 = FILE_TRIANGLE_SIZE; 
			}
			else if(pass==2)
			{
				off = MDT_TexCoords; off2 = FILE_TEXCOORD_SIZE; 
			}
			else if(pass==3)
			{
				#define l_4096 0.000244140625f

				off = MDT_TriangleNormals; off2 = FILE_TRIANGLE_NORMAL_SIZE;
			}
			offsetList.setOffset(off);
			offsetList.setUniformOffset(off,true);
			writeHeaderB(0x0000,count,off2);

			//o512.mdl crashes on *packs (empty)
			if(!count) continue;

			int vbase = 0, nbase = 0;

			int ppt2 = (*packs)->part;

			MDL::Part *ppt = parts+ppt2;

			for(MDL::Pack**p=packs;*p;p++)
			{
				MDL::Pack *pack = *p;
				MDL::Part *pt = parts+pack->part;

				if(ppt!=pt)
				{
					vbase+=ppt->verts-holes[ppt2];
					nbase+=ppt->norms;
					
					ppt = pt; ppt2 = pack->part;
				}
								
//#ifdef NDEBUG //???
		//attention! consolidation optimization...
		#define O16(x,vn,i)	/*U16*/(pt->c##vn[x.vn[i]])
//#else
//		#define O16(x,vn,i)	/*U16*/(pt->c##vn?pt->c##vn[x.vn[i]]:x.vn[i])
//#endif	
		assert(pt->cverts||!pack->size); //late 2021			

				//ASSUMING TRIANGLES ONLY!!
				for(int i=0;i<pack->size;i++) 
				{
					switch(pack->type)
					{
					default: assert(0); return; //unimplemented

					case 0x00: //3x32bits
					{
						auto &x00 = pack->x00[i];

					//	fwrite(U8(x00.r),f);
					//	fwrite(U8(x00.g),f);
					//	fwrite(U8(x00.b),f);
					//	fwrite(U8(x00.c),f);

						if(pass==1)
						{
							for(int j=0;j<3;j++)
							fileTriangle.vertex[2-j] = vbase+O16(x00,verts,j);
						}
						else if(pass==3)
						{
							//it's not worth it to transform by bindposemats :(
							//for(int j=0;j<3;j++)
							//memcpy(fileNormals.normal+j,x00.mm3d_norm,3*sizeof(float));
							int ni = nbase+O16(x00,norms,0);
							short *s = norms+3*ni;
							for(int j=0;j<3;j++)
							{	
								float *d = fileNormals.normal[2-j];
								d[0] = s[0]*+l_4096;
								d[1] = s[1]*-l_4096; //invert?
								d[2] = s[2]*-l_4096; //invert?
							}
						}

						break;
					}
					case 0x03: //4x32bits
					{
						auto &x03 = pack->x03[i];

					//	fwrite(U8(x03.r),f);
					//	fwrite(U8(x03.g),f);
					//	fwrite(U8(x03.b),f);
					//	fwrite(U8(x03.c),f);

						if(pass==1)
						{
							for(int j=0;j<3;j++)
							fileTriangle.vertex[2-j] = vbase+O16(x03,verts,j);
						}
						else if(pass==3)
						{
							//it's not worth it to transform by bindposemats :(
							//for(int j=0;j<3;j++)
							//memcpy(fileNormals.normal+j,x03.mm3d_norms[j],3*sizeof(float));
							for(int j=0;j<3;j++)
							{
								int ni = nbase+O16(x03,norms,j);
								short *s = norms+3*ni;
								float *d = fileNormals.normal[2-j];
								d[0] = s[0]*+l_4096;
								d[1] = s[1]*-l_4096; //invert?
								d[2] = s[2]*-l_4096; //invert?
							}
						}

						break;
					}
					case 0x04: //6x32bits
					{			
						auto &x04 = pack->x04[i];

						if(2==pass&&x04.mm3d_uvs)
						{
							for(int j=0;j<3;j++)
							{
								tc.sCoord[j] = x04.mm3d_uvs[j]->x;
								tc.tCoord[j] = x04.mm3d_uvs[j]->y;
							}

							m_dst->write(tc);													
						}
						else if(1==pass)
						{
							for(int j=0;j<3;j++)
							fileTriangle.vertex[2-j] = vbase+O16(x04,verts,j);
						}
						else if(3==pass)
						{
							//it's not worth it to transform by bindposemats :(
							//for(int j=0;j<3;j++)
							//memcpy(fileNormals.normal+j,x04.mm3d_norms[j],3*sizeof(float));
							for(int j=0;j<3;j++)
							{
								int ni = nbase+O16(x04,norms,j);
								short *s = norms+3*ni;
								float *d = fileNormals.normal[2-j];
								d[0] = s[0]*+l_4096;
								d[1] = s[1]*-l_4096; //invert?
								d[2] = s[2]*-l_4096; //invert?
							}
						}

						break;
					}}

					if(1==pass)
					{
						m_dst->write(fileTriangle);
					}
					else if(2==pass) 
					{
						mat++; //HACK
						tc.triangleIndex++;
					}
					else if(3==pass) //ALWAYS?
					{
						m_dst->write(fileNormals);
						fileNormals.index++;
					}
				}
			}

			#undef O16
		}
		assert(count==fileNormals.index); //ALWAYS?

	//	log_debug("wrote %d texture coordinates\n",count);

		assert(mat==x2mm3d_mats.end());
	}

	// External Textures
	if(doWrite[MDT_ExtTextures])
	{
		offsetList.setOffset(MDT_ExtTextures);
		offsetList.setUniformOffset(MDT_ExtTextures,false);

		unsigned count = texnames.size();

		writeHeaderA(0x0000,count);

		unsigned baseSize = sizeof(uint16_t);

		for(unsigned i=0;i<count;i++)
		{
			auto &fn = texnames[i];
			if(fn.empty()) continue;

			//YUCK: have to convert to relative path (ideally)
			const wchar_t *p = fn.c_str(), *q = name;
			while(*p==*q&&*p){ p++; q++; }
			while(q>name&&*q!='/'&&*q!='\\'){ p--; q--; }

			int zt = q-name, rel = zt?1:0;

			char buf[MAX_PATH*4];
			zt = rel+WideCharToMultiByte
			(CP_UTF8,0,p,(int)fn.size()-zt,buf+rel,sizeof(buf)-rel,0,0);
			assert(zt>0);
			if(zt<0) zt = 0;
			buf[zt++] = 0;
			if(rel) buf[0] = '.'; //./ construction
			
			uint32_t texSize = baseSize+zt;

			uint16_t flags = 0x0000;

			m_dst->write(texSize);
			m_dst->write(flags);
			m_dst->writeBytes(buf,zt);

		//	log_debug("material file is %s\n",filename);			
		}
	//	log_debug("wrote %d external textures\n",texNum);
	}

	// Groups // Materials
	//
	// TODO? might want to output meshes as groups
	//
	if(doWrite[MDT_Groups]&&doWrite[MDT_Materials])
	{
		std::sort(x2mm3d_mats.begin(),x2mm3d_mats.end());	

		for(int pass=1;pass<=2;pass++)
		{
			//Note: switching to Materials->Groups order
			auto off = pass==2?MDT_Groups:MDT_Materials;
		
			offsetList.setOffset(off);
			offsetList.setUniformOffset(off,false);

			if(!x2mm3d_mats.empty()) //o512.mdl
			count = x2mm3d_mats.back().first+1;
			else count = 0;

			//2021: removing "Control Points" material in
			//favor of Point objects			
			auto off1 = m_dst->offset(); //2021
			unsigned real_count = 0;
			writeHeaderA(0x0000,count);

			unsigned baseSize; if(pass==2) //MDT_Groups
			{
				baseSize = sizeof(uint16_t)+sizeof(uint32_t);
				baseSize+=sizeof(uint8_t)+sizeof(uint32_t);
			}
			else //MDT_Materials
			{
				baseSize = sizeof(uint16_t)+sizeof(uint32_t);
				baseSize+=17*sizeof(float32_t);
			}

			int32_t triCounter = 0;

			aiString name;
			for(unsigned g=0;g<count;g++)
			{
				auto triCount = triCounter;
				if(g!=count-1)
				while(g==x2mm3d_mats[triCounter].first)
				triCounter++;
				else triCounter = (int)x2mm3d_mats.size();
				triCount = triCounter-triCount;

				if(!triCount) continue; //2021

					real_count++;

				X->mMaterials[g]->Get(AI_MATKEY_NAME,name);
				if(!name.length)
				{
					char n[] = "Unnamed #0";
					n[sizeof(n)-2]+=(char)g;
					name.Set(n);
				}

				if(pass==2) //MDT_Groups
				{
					//Model::Group *grp = modelGroups[g];
					unsigned groupSize = baseSize+name.length+1;
					groupSize+=triCount*sizeof(uint32_t);

					uint16_t flags = 0x0000;
				//	flags |= (modelGroups[g]->m_visible)? 0 : MF_HIDDEN;
				//	flags |= (modelGroups[g]->m_selected)? MF_SELECTED : 0;					

					m_dst->write(groupSize);
					m_dst->write(flags);
					m_dst->writeBytes(name.data,name.length+1);
					m_dst->write(triCount);

					for(int i=triCounter-triCount;i<triCounter;i++)
					{
						uint32_t tri = x2mm3d_mats[i].second;
						m_dst->write(tri);
					}

					//REMINDER: MDT_SmoothAngles set m_angle other than 89
					uint8_t smoothness = 255;
					uint32_t materialIndex = real_count-1; //g;
					m_dst->write(smoothness);
					m_dst->write(materialIndex);
				}
				else //MDT_Materials
				{
					//Model::Material *mat = modelMaterials[m];
					unsigned matSize = baseSize+name.length+1;

					uint16_t flags = 0; //MATTYPE_TEXTURE 
					//if(texnames[g].empty()) //MATTYPE_BLANK?
					if(-1==texindex[g]) flags = 0x000f;

					//int32_t temp = flags?-1:texIndex++; //!!
					int32_t temp = texindex[g];
					
				//	if(mat->m_sClamp) flags|=MF_MAT_CLAMP_S;
				//	if(mat->m_tClamp) flags|=MF_MAT_CLAMP_T;
					int bm; //2021 (November)
					if(!X->mMaterials[g]->Get(AI_MATKEY_BLEND_FUNC,bm))
					if(bm==aiBlendMode_Additive)
					flags|=MF_MAT_ACCUMULATE;

					m_dst->write(matSize);
					m_dst->write(flags);
					m_dst->write(temp);
					m_dst->writeBytes(name.data,name.length+1);

					//2021: just guessing these align
					float *m = materials[g];
					float *m2 = materials2[g];
					float32_t lightProp[4*4+1] = 
					{
						//0.5f,0.5f,0.5f,1,
						m[0],m[1],m[2],m[3],
						m[0],m[1],m[2],m[3], //2021
						0,0,0,0,
						m2[0],m2[1],m2[2],m2[3], //2021
					};
					/*float32_t lightProp = 0;
					for(unsigned i=0;i<4;i++)
					{
						lightProp = mat->m_ambient[i];
						m_dst->write(lightProp);
					}
					for(unsigned i=0;i<4;i++) //TODO: materials2?
					{
						lightProp = mat->m_diffuse[i];
						m_dst->write(lightProp);
					}
					for(unsigned i=0;i<4;i++)
					{
						lightProp = mat->m_specular[i];
						m_dst->write(lightProp);
					}
					for(unsigned i=0;i<4;i++) //TODO: materials2?
					{
						lightProp = mat->m_emissive[i];
						m_dst->write(lightProp);
					}
					lightProp = mat->m_shininess;*/
					m_dst->write(lightProp);
				}
			}

			//2021: removing "Control Points" material in
			//favor of Point objects
			auto off2 = m_dst->offset();
			m_dst->seek(off1);
			writeHeaderA(0x0000,real_count);
			m_dst->seek(off2);

		//	log_debug("wrote %d groups\n",count);
		}
	}

	//YUCK: this feels really wrong
	std::vector<std::pair<int,int>> chanmap;

	struct pose
	{
		aiVector3D t; aiQuaternion q;
	};
	std::vector<pose> posemap;

	// Joints
	if(doWrite[MDT_Joints])
	{
		offsetList.setOffset(MDT_Joints);
		offsetList.setUniformOffset(MDT_Joints,true);

		MDL::Anim *anim = anims[0];
		uint32_t count = anim->size/anim->steps;
		writeHeaderB(0x0000,count,FILE_JOINT_SIZE);

		//ALGORITHM
		//MM3D wants parents before children in its list
		//this code scores the parents lower, sorts, and
		//finally builds a reverse map 
		{
			posemap.resize(count);
			chanmap.resize(count+1);
			for(int j=count;j-->0;)
			{
				if(0xff==chans[j]) chans[j] = count; //!!

				chanmap[j].second = j;
			}
			chanmap.back() = {-1,-1}; //sentinel

			for(bool stop=false;!stop;)
			{
				stop = true;
				for(int j=count;j-->0;)
				if(chanmap[j].first<=chanmap[chans[j]].first)
				{
					stop = false; chanmap[j].first++;
				}
			}
			chanmap.pop_back(); //sentinel

			std::sort(chanmap.begin(),chanmap.end());

			for(int j=count;j-->0;) 
			{
				chanmap[chanmap[j].second].first = j;
			}		
		}

		for(int jj=0;jj<count;jj++)
		{			
			int j = chanmap[jj].second;

			MM3DFILE_JointT fileJoint;
			
			fileJoint.flags = 0x0000;			
			aiNode *nd = x2mm3d_nodetable[j];
			if(!nd) strcpy(fileJoint.name,"MM3D origin");
			else x2mm3d_copy_name(fileJoint.name,40,nd->mName);
			int pi = chans[j];
			fileJoint.parentIndex = pi==count?-1:chanmap[pi].first;
			
			//HACK: Draw these joints with lines instead of bone shapes
			//(They usually look best this way)
			for(int d=0;pi!=count&&d<2;d++) pi = chans[pi];
			if(pi==count) fileJoint.flags|=4; //MF_VERTFREE?
			
			short (&p)[3+3] = anim->info[j*anim->steps];

			bool rot = p[0]||p[1]||p[2]; assert(!rot); 

			for(int i=0;i<3;i++)
			{
				fileJoint.localRot[i] = p[i]*AI_MATH_PI/2048*invert[i];
				fileJoint.localTrans[i] = p[i+3]/1024.0f*invert[i];
				//fileJoint.localScale[i] = 1; //UNFINISHED
			}
			posemap[jj].t = *(aiVector3D*)fileJoint.localTrans;			
			if(rot)
			{
				//NOTE: This produces output completely unlike the input
				//I don't see a straightforward conversion
				//{-1.92361188, 0.392699093, -0.799203992}
				//{-2.08926797, -0.870811462, -0.00421601627}
				aiQuaternion &q = posemap[jj].q;
				Assimp::EulerAnglesToQuaternion<+1,1,2,3>(*(aiVector3D*)fileJoint.localRot,q);
				Assimp::EulerAnglesFromQuaternion<-1,1,2,3>(*(aiVector3D*)fileJoint.localRot,q);
				posemap[jj].q.Conjugate(); //invert
			}
			else posemap[jj].q.w = 0; //mark unused

			m_dst->write(fileJoint);
		}
	//	log_debug("wrote %d joints\n",count);
	}

	//Scaling is unimplemented for the "hard"
	//animation type, and control points don't
	//require scaling.
	//std::vector<std::pair<uint32_t,aiVector3D>> sf;

	// Points
	if(doWrite[MDT_Points])
	{
		offsetList.setOffset(MDT_Points);
		offsetList.setUniformOffset(MDT_Points,true);

		uint32_t count = (uint32_t)pts.size();
		writeHeaderB(0x0000,count,FILE_POINT_SIZE);
		for(uint32_t p=0;p<count;p++)
		{
			aiNode *pt = pts[p];

			MM3DFILE_PointT filePoint;

			filePoint.flags = 0x0000;
			x2mm3d_copy_name(filePoint.name,40,pt->mName);
			filePoint.type = 0;
			filePoint.boneIndex = -1;

			aiVector3D &rot = *(aiVector3D*)&filePoint.rot;
			aiVector3D &pos = *(aiVector3D*)&filePoint.trans;
			//aiVector3D &scl = *(aiVector3D*)&filePoint.scale;
			aiVector3D scl;

			aiMatrix4x4 m = pt->mTransformation;
			for(aiNode*p=pt;p=p->mParent;)
			{
				m = p->mTransformation*m; 
			}
			aiQuaternion q; m.Decompose(scl,q,pos);
			Assimp::EulerAnglesFromQuaternion<-1,1,2,3>(rot,q);

			/*Control points?
			for(int i=3;i-->0;) if(fabsf(scl[i]-1)>0.00001)
			{
				sf.push_back(std::make_pair(p,scl));
			}*/

			m_dst->write(filePoint);
		}
	//	log_debug("wrote %d points\n",count);
	}

	/*2021 Weighted influences
	if(doWrite[MDT_ScaleFactors])
	{
		offsetList.setOffset(MDT_ScaleFactors);
		offsetList.setUniformOffset(MDT_ScaleFactors,true);

		//NOTE: Maybe empty.
		writeHeaderB(0x0000,sf.size(),FILE_SCALE_FACTOR_SIZE);
		for(size_t i=0;i<sf.size();i++)
		{
			m_dst->write((uint16_t)(i<sf_joints?1:2));
			m_dst->write((uint16_t)sf[i].first);
			m_dst->write(sf[i].second);
		}
	}*/

	// Weighted influences
	if(doWrite[MDT_WeightedInfluences])
	{
		offsetList.setOffset(MDT_WeightedInfluences);
		offsetList.setUniformOffset(MDT_WeightedInfluences,true);

		auto pcount = (uint32_t)pts.size();
		writeHeaderB(0x0000,vcount+pcount,FILE_WEIGHTED_INFLUENCE_SIZE);

		int v = 0, w = 0, vend = 0;

		for(int i=0;i<head.parts;i++)
		{
			Part &pt = *parts[i].cpart;
			//int j = chanmap[i].first; 
			unsigned ci = chanindex[pt.cnode];
			int j = chanmap[ci].first;

			for(vend+=pt.verts;v<vend;v++)
			{
				if(!rcverts[v]) continue;

				MM3DFILE_WeightedInfluenceT fileWi;
				fileWi.posType = 0; //Model::PT_Vertex
				fileWi.posIndex = w++;
				fileWi.infType = 0; //Model::IT_Custom
				fileWi.infIndex = j;
				fileWi.infWeight = 100;

				m_dst->write(fileWi);
			}
		}

		for(size_t p=0;p<pts.size();p++)
		{
			int j = -1, jj = 0;
			for(auto k=chanmap.size();k-->0;)
			{
				aiNode *nd = x2mm3d_nodetable[k];
				if(pts[p]==nd) j = chanmap[k].first;
				if(pts[p]->mParent==nd&&nd) jj = chanmap[k].first;
			}
			if(j==-1) j = jj;

			MM3DFILE_WeightedInfluenceT fileWi;
			fileWi.posType = 2; //Model::PT_Point
			fileWi.posIndex = p;
			fileWi.infType = 0; //Model::IT_Custom
			fileWi.infIndex = j;
			fileWi.infWeight = 100;

			m_dst->write(fileWi);
		}

	//	log_debug("wrote %d weighted influences\n",vcount+pcount);
	}

	/*REFERENCE
	// Skel Anims
	if(doWrite[MDT_SkelAnims])
	{
		offsetList.setOffset(MDT_SkelAnims);
		offsetList.setUniformOffset(MDT_SkelAnims,false);

		uint32_t count = 0;
		for(MDL::Anim**p=anims;*p;p++)
		count++;	
		writeHeaderA(0x0000,count);

		//unsigned baseSize = 0;
		//baseSize+=sizeof(uint16_t)+sizeof(float32_t)+sizeof(uint32_t);
		//baseSize+=sizeof(float32_t); //m_frame2020

		std::vector<short> accum;
		std::vector<uint32_t> keys;
		auto **xa = X->mAnimations;
		for(MDL::Anim**p=anims;*p;p++)
		{
			MDL::Anim *anim = *p;

			int first = anims==p;
			int stride = anim->steps;
			int nchans = anim->size/stride;
			
			char a[33],*name = 0; if(xa) //mo?
			{
				while(!(*xa)->mChannels)
				xa++;
				if((*xa)->mName.length)
				{
					name = (*xa)->mName.data;
				}
				xa++;
			}
			if(!name) name = _itoa(anim->type,a,10);

			//The size depends on partial frames.			
			//auto sa = modelSkels[anim];			
			//uint32_t animSize = baseSize+sa->m_name.length()+1;
			//animSize += frameCount *sizeof(float32_t); //m_timetable2020 
			//animSize += keyframeCount *FILE_KEYFRAME_SIZE;
			uint32_t animSize = 0;

			uint16_t flags = 0;
			float32_t fps = 30;
			
			auto off1 = m_dst->offset();
			m_dst->write(animSize); //HACK

			m_dst->write(flags);
			m_dst->writeBytes(name,strlen(name)+1);
			m_dst->write(fps);
			
			accum.assign(6*nchans,0);
			short (*qq)[6] = anim->info+first;
			short (*q)[6] = qq;
			keys.assign(stride-first,0);			
			for(unsigned f=0;f<keys.size();f++,q++)
			{
				for(int j=0;j<nchans;j++)
				{
					short *a = *(q+j*stride);

					if(first&&!f)
					memcpy(&accum[j*6],a-6,sizeof(short)*6);

					if(a[0]||a[1]||a[2])
					keys[f]++;
					if(a[3]||a[4]||a[5])
					keys[f]++;
				}
			}
			//the first frame cancels out the bind pose
			//so don't emit false frames
			if(!first) for(int j=0;j<nchans;j++)
			{
				int fstride = (*anims)->steps;

				short *a = qq[j*stride];
				short *b = (*anims)->info[j*fstride];
				
				if(a[0]||a[1]||a[2])
				if(a[0]==b[0]&&a[1]==b[1]&&a[2]==b[2])
				{						
					assert(keys[0]);
					keys[0]--;
					for(int i=3;i-->0;) //destructive!					
					std::swap(a[i],accum[i+j*6]);
				}
				if(a[3]||a[4]||a[5])
				if(a[3]==b[3]&&a[4]==b[4]&&a[5]==b[5])
				{
					assert(keys[0]);
					keys[0]--;
					for(int i=6;i-->3;) //destructive!					
					std::swap(a[i],accum[i+j*6]);
				}
			}
			uint32_t frameCount = 0;
			for(auto f=0;f<keys.size();f++)
			{
				if(keys[f]) frameCount++;
			}
			m_dst->write(frameCount);			

			float32_t tempf = 0;
			for(unsigned f=0;f<keys.size();f++)
			if(keys[f]!=0)
			m_dst->write(tempf=(float)f); //2020
			m_dst->write(tempf=(float)keys.size()); //2020

			q = qq;
			for(unsigned f=0;f<keys.size();f++,q++)
			{
				if(!keys[f]) continue;
				m_dst->write(keys[f]);

				size_t test = 0;

				int i = 0; short *a = &accum[0];

				for(unsigned jj=0;jj<chanmap.size();jj++)
				{
					//if(!anim->getmask(jj)) continue;

					int j = chanmap[jj].first;
					
					short *b = *(q+i); i+=stride;

					for(int pass=1;pass<=2;pass++,a+=3,b+=3)
					{
						if(!b[0]&&!b[1]&&!b[2]) continue;
	
						MM3DFILE_KeyframeT fileKf;
						auto &oi = fileKf.objectIndex;
						oi = j;
						//oi|=1<<16|3<<24; //PT_Joint/InterpolateLerp
						oi|=1<<16|2<<24; //PT_Joint/InterpolateStep						
						switch(pass)
						{
						case 2: fileKf.keyframeType = 1; break; //translate
						case 1: fileKf.keyframeType = 0; break; //rotate
						case 3: fileKf.keyframeType = 2; break; //scale
						}

						for(int i=3;i-->0;) 
						{
							double v = a[i]+=b[i];

							if(pass==1) v = v*AI_MATH_PI/2048*invert[i];
							if(pass==2) v = v/1024.0f*invert[i]-posemap[j].t[i];
							if(pass==3) assert(pass!=3);

							fileKf.param[i] = (float)v;
						}
						if(pass==1)
						{
							//NOTE: This produces output completely unlike the input
							//I don't see a straightforward conversion
							//{-1.92361188, 0.392699093, -0.799203992}
							//{-2.08926797, -0.870811462, -0.00421601627}
							aiQuaternion tempq;
							Assimp::EulerAnglesToQuaternion<+1,1,2,3>(*(aiVector3D*)fileKf.param,tempq);
							
							if(posemap[j].q.w) 
							{
								//UNTESTED (50/50 chance the order should be reversed)
								assert(0);
								tempq = tempq*posemap[j].q;
							}

							Assimp::EulerAnglesFromQuaternion<-1,1,2,3>(*(aiVector3D*)fileKf.param,tempq);
						}
							
						if(test>=keys[f]) //OVERFLOW?
						{
							assert(test<keys[f]); break;
						}

						m_dst->write(fileKf); test++;
					}
				}

				//if this fails likely getmask is wrong or I've applied it incorrectly

				assert(test==keys[f]); //UNDERFLOW? (what to do if so?)
			}

			//TODO: Can MisfitOffsetList do this?
			auto off2 = m_dst->offset();
			animSize = off2-off1-sizeof(animSize);
			m_dst->seek(off1);
			m_dst->write(animSize);
			m_dst->seek(off2);
		}
	//	log_debug("wrote %d skel anims\n",count);
	}*/

	/*REFERENCE
	// Frame Anims
	if(doWrite[MDT_FrameAnims])
	{	
		offsetList.setOffset(MDT_FrameAnims);
		offsetList.setUniformOffset(MDT_FrameAnims,false);

		uint32_t count = 0;
		for(MDL::Diff **p=diffs;*p;p++)
		count++;		
		writeHeaderA(0x0000,count);

		//unsigned baseSize = 
		//sizeof(uint16_t)+sizeof(float32_t)+sizeof(uint32_t);

		auto **xa = X->mAnimations;
		for(MDL::Diff**p=diffs;*p;p++)
		{
			MDL::Diff *diff = *p;
			
			int16_t *verts = diff->verts;
			int16_t *times = diff->times();
			unsigned width = diff->width/3;
			uint32_t frameCount = diff->steps;
			
			char a[33],*name = 0; if(xa) //mo?
			{
				while(!(*xa)->mMeshChannels)
				xa++;
				if((*xa)->mName.length)
				{
					name = (*xa)->mName.data;
				}
				xa++;
			}
			if(!name) name = _itoa(diff->type,a,10);

			//The size depends on partial frames.			
			//unsigned animSize = baseSize+strlen(name)+1;
			//animSize+=frameCount*width*sizeof(float32_t)*3;
			uint32_t animSize = 0;

			uint16_t flags = 0;
			float32_t fps = 30;			
			
			auto off1 = m_dst->offset();
			m_dst->write(animSize); //HACK

			m_dst->write(flags);
			m_dst->writeBytes(name,strlen(name)+1);
			m_dst->write(fps);			
			m_dst->write(frameCount);

			//NOTE: MDL starts at t=1
			float32_t frame2020 = -1;
			for(unsigned f=0;f<frameCount;f++)
			{	
				frame2020+=times[f];
				m_dst->write(frame2020); //EXTENSION				
			}
			if(frame2020<0) frame2020 = 0;
			m_dst->write(frame2020); //EXTENSION

			int16_t *prev = 0;
			int16_t *curr = verts;
			int16_t *next = curr+diff->width;
			for(unsigned f=0;f<frameCount;f++)
			{
				int16_t *p = curr, *pp = p;
				int16_t *q = prev, *d = next;

				for(unsigned v=0;v<width;)
				{
					bool skip = q&&!memcmp(q,p,sizeof(*p)*3);
					if(q) while(p<d&&skip==(*p==*q&&p[1]==q[1]&&p[2]==q[2]))
					{
						p+=3; q+=3;
					}
					else //p = d; 
					{
						//FINISH ME
						//FINISH ME
						//FINISH ME
						//Do the comparison against the base mesh instead.
					
						p = d;
					}

					unsigned dv = (unsigned)(p-pp)/3;
					
					//EXTENSION
					uint32_t tmp = skip?1:3; //InterpolateCopy/Lerp
					m_dst->write(tmp);
					m_dst->write(tmp=dv); //!

					v+=dv; if(!skip) 
					{
						for(;dv-->0;pp+=3)
						{
							float32_t delta[3] =
							{
								pp[0]/+1024.0f,
								pp[1]/-1024.0f,
								pp[2]/-1024.0f
							};
							m_dst->write(delta);
						}
					}
					else pp = p;
				}

				prev = curr;
				curr+=diff->width;
				next+=diff->width;
			}

			//TODO: Can MisfitOffsetList do this?
			auto off2 = m_dst->offset();
			animSize = off2-off1-sizeof(animSize);
			m_dst->seek(off1);
			m_dst->write(animSize);
			m_dst->seek(off2);
		}
	//	log_debug("wrote %d frame anims\n",count);
	}*/

	/*REFERENCE
	// Frame Anim Points
	if(doWrite[MDT_FrameAnimPoints])
	{
		offsetList.setOffset(MDT_FrameAnimPoints);
		offsetList.setUniformOffset(MDT_FrameAnimPoints,false);

		uint32_t count = 0;
		for(MDL::Diff **p=diffs;*p;p++)
		count++;		
		writeHeaderA(0x0000,count);

		uint32_t anim = 0;
		auto **xa = X->mAnimations;
		for(MDL::Diff **p=diffs;*p;p++,anim++)
		{
			MDL::Diff *diff = *p;
			
			auto off1 = m_dst->offset();
			uint32_t animSize = 0; //HACK
			m_dst->write(animSize);

			uint32_t frameCount = diff->steps;

			uint16_t flags = 0;
			m_dst->write(flags);
			m_dst->write(anim);
			m_dst->write(frameCount);

			int pcount = 0;
			aiNodeAnim **pchans = 0; if(xa) //mo?
			{
				while(!(*xa)->mMeshChannels)
				xa++;
				pcount = (*xa)->mNumChannels;
				pchans = (*xa)->mChannels;
				xa++;
			}
			else //UNFINISHED
			{
				//Jose the NPC may have some points?
				//NOTE: x2mm3d_convert_points should
				//have already converted the diff to
				//aiNodeAnim
				//assert(0);
			}
			
			uint32_t keyframeCount = 0;
			for(int j=0;j<pcount;j++)
			{
				//TODO! ScenePreprocessor.cpp GENERATES DUMMIES!
				if(pchans[j]->mNumPositionKeys) keyframeCount++;
				if(pchans[j]->mNumRotationKeys) keyframeCount++;
			}			
			
			int16_t *times = diff->times();
			float t = 0;
			for(unsigned f=0;f<frameCount;f++)
			{
				t+=*times++;

				m_dst->write(keyframeCount);

				for(int j=0;j<pcount;j++)
				{
					auto *pc = pchans[j];
					auto *pk = pc->mPositionKeys;
					auto *rk = pc->mRotationKeys;
					auto *pe = pk+pc->mNumPositionKeys;
					auto *re = rk+pc->mNumRotationKeys;

					MM3DFILE_KeyframeT fileKf;
					aiNode *pt;
					auto &oi = fileKf.objectIndex;
					for(oi=0;oi<pts.size();oi++)					
					if((pt=pts[oi])->mName==pc->mNodeName)
					break;
					oi|=2<<16|3<<24; //PT_Point/InterpolateLerp
					aiVector3D &v = *(aiVector3D*)fileKf.param;
					for(uint8_t&k=fileKf.keyframeType=0;k<=1;k++) 
					{		
						if(k==1)
						{
							if(!pk) continue;
							aiVectorKey cmp; cmp.mTime = t;
							x2mm3d_sample(cmp,pk,pe,pt);
							v = cmp.mValue;
						}
						if(k==0)
						{
							if(!rk) continue;
							aiQuatKey cmp; cmp.mTime = t;
							x2mm3d_sample(cmp,rk,re,pt);
							Assimp::EulerAnglesFromQuaternion<-1,1,2,3>(v,cmp.mValue);
						}
						m_dst->write(fileKf);
					}
				}
			}

			//TODO: Can MisfitOffsetList do this?
			auto off2 = m_dst->offset();
			animSize = off2-off1-sizeof(animSize);
			m_dst->seek(off1);
			m_dst->write(animSize);
			m_dst->seek(off2);
		}
	//	log_debug("wrote %d frame anim points\n",count);
	}*/

	//2021 Animations
	//
	// TODO? probably would've been wiser to translate
	// directly from Assimp. I'm not sure why I didn't
	//
	if(doWrite[MDT_Animations])
	{	
		offsetList.setOffset(MDT_Animations);
		offsetList.setUniformOffset(MDT_Animations,false);

		uint32_t count = 0;
		if(anims)
		for(MDL::Anim**p=anims;*p;p++)
		count++;
		if(diffs)
		for(MDL::Diff**p=diffs;*p;p++)
		count++;		
		writeHeaderA(0x0000,count);

		//unsigned baseSize = 0;
		//baseSize+=sizeof(uint16_t)+sizeof(float32_t)+sizeof(uint32_t);
		//baseSize+=sizeof(float32_t); //m_frame2020

		std::vector<short> accum;
		std::vector<uint32_t> keys;
		auto **xa = X->mAnimations;
		if(anims)
		for(MDL::Anim**p=anims;*p;p++)
		{
			MDL::Anim *anim = *p;

			int first = anims==p;
			int stride = anim->steps;
			int nchans = anim->size/stride;
			
			char a[33],*name = 0; if(xa) //mo?
			{
				//this is how x2mdl.cpp filters, allowing empty animations
				//while(!(*xa)->mChannels)
				while((*xa)->mNumMeshChannels)
				xa++;
				if((*xa)->mName.length)
				{
					name = (*xa)->mName.data;
				}
				xa++;
			}
			if(!name) name = _itoa(anim->type,a,10);

			//The size depends on partial frames.			
			//auto sa = modelSkels[anim];			
			//uint32_t animSize = baseSize+sa->m_name.length()+1;
			//animSize += frameCount *sizeof(float32_t); //m_timetable2020 
			//animSize += keyframeCount *FILE_KEYFRAME_SIZE;
			uint32_t animSize = 0;

			uint16_t flags = 1; //skeletal
			float32_t fps = head.flags&16?60:30;
			
			auto off1 = m_dst->offset();
			m_dst->write(animSize); //HACK

			m_dst->write(flags);
			m_dst->writeBytes(name,strlen(name)+1);
			m_dst->write(fps);

			short (*qq)[6] = anim->info+first;
			short (*q)[6] = qq;
			unsigned char (*ss)[3] = anim->scale;
			unsigned char (*s)[3] = ss+=(ss?first:0);
			keys.assign(stride-first,0);			
			accum.assign(9*nchans,0);
			if(ss) for(int j=0;j<nchans;j++)
			{
				int fstride = (*anims)->steps; //same as below
				unsigned char *bs = (*anims)->scale[j*fstride];
				for(int k=3;k-->0;) accum[6+k+j*9] = bs[k];
			}
			for(unsigned f=0;f<keys.size();f++,q++,s++)
			{
				for(int j=0;j<nchans;j++)
				{
					short *a = *(q+j*stride);

					//note, scale is filled out up above
					//and more work is done in the next 
					//loop after this one to remove keys
					if(!f&&first)
					memcpy(&accum[j*9],a-6,sizeof(short)*6);

					if(a[0]||a[1]||a[2])
					keys[f]++;
					if(a[3]||a[4]||a[5])
					keys[f]++;

					if(!ss) continue; //scaling?

					auto as = s+j*stride;

					if(f==0) 
					{
						auto *cmp = &accum[6+j*9];

						for(int k=3;k-->0;) if((*as)[k]!=cmp[k])
						{
							keys[f]++; break;							
						}						
					}
					else
					{
						auto bs = as-1;

						for(int k=3;k-->0;) if((*as)[k]!=(*bs)[k])
						{
							keys[f]++; break;	
						}
					}
				}
			}
			//the first frame cancels out the bind pose
			//so don't emit false frames			
			if(!first) for(int j=0;j<nchans;j++)
			{
				int fstride = (*anims)->steps;
				
				short *a = qq[j*stride];
				short *b = (*anims)->info[j*fstride];
				
				if(a[0]||a[1]||a[2])
				if(a[0]==b[0]&&a[1]==b[1]&&a[2]==b[2])
				{						
					assert(keys[0]);
					keys[0]--;
					for(int i=3;i-->0;) //DESTRUCTIVE!!
					std::swap(a[i],accum[i+j*9]);
				}
				if(a[3]||a[4]||a[5])
				if(a[3]==b[3]&&a[4]==b[4]&&a[5]==b[5])
				{
					assert(keys[0]);
					keys[0]--;
					for(int i=6;i-->3;) //DESTRUCTIVE!!
					std::swap(a[i],accum[i+j*9]);
				}
			}
			uint32_t frameCount = 0;
			for(auto f=0;f<keys.size();f++)
			{
				if(keys[f]) frameCount++;
			}
			m_dst->write(frameCount);			

			float32_t tempf = 0;
			for(unsigned f=0;f<keys.size();f++)
			if(keys[f]!=0)
			m_dst->write(tempf=(float)f); //2020
			//m_dst->write(tempf=(float)keys.size()); //2020
			m_dst->write(tempf=(float)keys.size()-1); //2022

			m_dst->write((uint32_t)2); //2021 (KM_Joint)
			uint32_t keyframeCount = 0;
			for(unsigned f=0;f<keys.size();f++)
			keyframeCount+=keys[f];			
			m_dst->write(keyframeCount); //2021
						
			q = qq; s = ss; int f = -1;
			for(unsigned k=0;k<keys.size();k++,q++,s++)
			{
				if(keys[k]) f++; else continue;

				size_t test = 0;

				int i = 0; short *a = &accum[0];

				for(unsigned jj=0;jj<chanmap.size();jj++)
				{
					//if(!anim->getmask(jj)) continue;

					int j = chanmap[jj].first;
					
					short *b = *(q+i); //i+=stride;

					unsigned char *bs = *(s+i); i+=stride;

					for(int pass=1;pass<=3;pass++,a+=3,b+=3)
					{
						if(pass==3) //scale?
						{
							b-=3; //YUCK

							if(!ss||bs[0]==a[0]&&bs[1]==a[1]&&bs[2]==a[2])
							{
								continue;
							}
						}
						else if(!b[0]&&!b[1]&&!b[2]) continue;
	
						uint8_t fd[4]; //format descriptor

					//	fd[0] = 2; //InterpolateStep (MDL)
					//	fd[0] = 3; //InterpolateLerp
						fd[0] = 2+x2mm3d_lerp; 
						fd[1] = 0;
						switch(pass)
						{
						case 2: fd[2] = 1; break; //translate
						case 1: fd[2] = 2; break; //rotate
						case 3: fd[2] = 4; break; //scale
						}
						fd[3] = 4; //4*4B

						m_dst->write(fd);
						m_dst->write((uint16_t)j);
						m_dst->write((uint16_t)f);

						aiVector3D pv;

						if(pass==3)
						{
							for(int i=3;i-->0;)
							{
								float v = a[i] = bs[i];

								pv[i] = v*0.0078125f; //very lossy
							}
						}
						else for(int i=3;i-->0;) 
						{
							double v = a[i]+=b[i];

							if(pass==1)
							{
								v = v*AI_MATH_PI/2048*invert[i];
							}
							else if(pass==2)
							{
								v = v/1024*invert[i]-posemap[j].t[i];
							}

							pv[i] = (float)v;
						}
						if(pass==1)
						{
							//NOTE: This produces output completely unlike the input
							//I don't see a straightforward conversion
							//{-1.92361188, 0.392699093, -0.799203992}
							//{-2.08926797, -0.870811462, -0.00421601627}
							aiQuaternion tempq;
							Assimp::EulerAnglesToQuaternion<+1,1,2,3>(pv,tempq);
							
							if(posemap[j].q.w) 
							{
								//UNTESTED (50/50 chance the order should be reversed)
								static int once = 0;
								assert(once++);
								tempq = tempq*posemap[j].q;
							}

							Assimp::EulerAnglesFromQuaternion<-1,1,2,3>(pv,tempq);
						}
							
						if(test>=keys[k]) //OVERFLOW?
						{
							assert(test<keys[k]); break;
						}
												
						m_dst->write(pv); test++; 
					}
				}

				//if this fails likely getmask is wrong or I've applied it incorrectly

				assert(test==keys[k]); //UNDERFLOW? (what to do if so?)
			}

			//TODO: Can MisfitOffsetList do this?
			auto off2 = m_dst->offset();
			animSize = off2-off1-sizeof(animSize);
			m_dst->seek(off1);
			m_dst->write(animSize);
			m_dst->seek(off2);
		}

			//SOFT //SOFT //SOFT //SOFT //SOFT //SOFT //SOFT //SOFT
			//SOFT //SOFT //SOFT //SOFT //SOFT //SOFT //SOFT //SOFT
			//SOFT //SOFT //SOFT //SOFT //SOFT //SOFT //SOFT //SOFT

		//unsigned baseSize = 
		//sizeof(uint16_t)+sizeof(float32_t)+sizeof(uint32_t);

		xa = X->mAnimations;
		if(diffs)
		for(MDL::Diff**p=diffs;*p;p++)
		{
			MDL::Diff *diff = *p;
			
			int16_t *verts = diff->verts;
			int16_t *times = diff->times();
			unsigned width = diff->width/3;
			uint32_t frameCount = diff->steps;
			
			int pcount = 0; //POINTS
			aiNodeAnim **pchans = 0; //POINTS 

			char a[33],*name = 0; if(xa) //mo?
			{
				while(!(*xa)->mMeshChannels)
				xa++;
				if((*xa)->mName.length)
				{
					name = (*xa)->mName.data;
				}
				pcount = (*xa)->mNumChannels; //POINTS
				pchans = (*xa)->mChannels; //POINTS
				xa++;
			}
			if(!name) name = _itoa(diff->type,a,10);

			//The size depends on partial frames.			
			//unsigned animSize = baseSize+strlen(name)+1;
			//animSize+=frameCount*width*sizeof(float32_t)*3;
			uint32_t animSize = 0;

			uint16_t flags = 2; //"frame"
			float32_t fps = 30;			
			
			auto off1 = m_dst->offset();
			m_dst->write(animSize); //HACK

			m_dst->write(flags);
			m_dst->writeBytes(name,strlen(name)+1);
			m_dst->write(fps);			
			m_dst->write(frameCount);

			//REMINDER: might want to start at t=0
			//if the first frame uses the base mesh
			//also this code needs to agree with 
			//x2mdl.cpp around where "times" are
			//calculated
			// 
			// NOTE: t = -1 for POINTS below too
			// 
			//NOTE: MDL starts at t=1
			float32_t frame2020 = -1;
			for(unsigned f=0;f<frameCount;f++)
			{	
				frame2020+=times[f];
				m_dst->write(frame2020); //EXTENSION				
			}
			if(frame2020<0) frame2020 = 0;
			m_dst->write(frame2020); //EXTENSION
			
			uint32_t keyframeCount = 0; //POINTS
			for(int j=0;j<pcount;j++)				
			{
				//UNFINISHED :(
				///*it's not guaranteed points are globally animated
				auto *pc = pchans[j];
				for(size_t p=0;p<pts.size();p++)					
				if(pts[p]->mName==pc->mNodeName)//*/
				{
					//TODO! ScenePreprocessor.cpp GENERATES DUMMIES!
					if(pchans[j]->mNumPositionKeys) keyframeCount+=frameCount;
					if(pchans[j]->mNumRotationKeys) keyframeCount+=frameCount;

					break;
				}
			}
			uint32_t mask = 1; //KM_Vertex
			if(keyframeCount) mask|=4; //KM_Point
			m_dst->write(mask); //2021

			int16_t *prev = 0;
			int16_t *curr = verts;
			int16_t *next = curr+diff->width;
			for(unsigned f=0;f<frameCount;f++)
			{
				int16_t *p = curr, *pp = p;
				int16_t *q = prev, *d = next;

				for(unsigned v=0;v<width;)
				{
					auto *rcv = &rcverts[v];
					bool skip = q&&!memcmp(q,p,sizeof(*p)*3);
					if(q) while(!*rcv++||p<d&&skip==(*p==*q&&p[1]==q[1]&&p[2]==q[2]))
					{
						p+=3; q+=3;
					}
					else //p = d; 
					{
						//FINISH ME
						//FINISH ME
						//FINISH ME
						//Do the comparison against the base mesh instead.
					
						p = d;
					}

					unsigned dv = (unsigned)(p-pp)/3;

					rcv = &rcverts[v];
					auto dw = dv, w = v; //YUCK: CP residue?					
					for(auto i=dv;i-->0;) if(!rcv[i]) dv--;
										
					uint8_t fd[4]; //format descriptor

					fd[0] = skip?1:3; //InterpolateCopy/Lerp
					fd[1] = 0;
					fd[2] = 0;
					fd[3] = skip?0:3; //0B or 3B

					m_dst->write(fd);
					m_dst->write((uint32_t)dv); //!

					v+=dw; if(!skip) 
					{
						for(;dw-->0;pp+=3)
						{
							if(!*rcv++) continue;

							float32_t delta[3] =
							{
								pp[0]*+0.0009765625f, //+1/1024
								pp[1]*-0.0009765625f, //-1/1024
								pp[2]*-0.0009765625f, //-1/1024
							};
							m_dst->write(delta);
						}
					}
					else pp = p;
				}

				prev = curr;
				curr+=diff->width;
				next+=diff->width;
			}

			
			//POINTS //POINTS //POINTS //POINTS //POINTS //POINTS 
			//POINTS //POINTS //POINTS //POINTS //POINTS //POINTS 
			//POINTS //POINTS //POINTS //POINTS //POINTS //POINTS 


			if(keyframeCount)
			m_dst->write(keyframeCount); //2021
			
			uint32_t test = 0;

			times = diff->times();
			//-1: SEE ABOVE COMMENTS
			float t = -1;
			if(keyframeCount) //2021
			for(unsigned f=0;f<frameCount;f++)
			{
				t+=*times++;

				//m_dst->write(keyframeCount); //2020

				for(int j=0;j<pcount;j++)
				{
					auto *pc = pchans[j];

					size_t p; 
					aiNode *pt;
					for(p=0;p<pts.size();p++)					
					if((pt=pts[p])->mName==pc->mNodeName)
					break;
					if(p==pts.size()) continue;

					auto *pk = pc->mPositionKeys;
					auto *rk = pc->mRotationKeys;
					auto *pe = pk+pc->mNumPositionKeys;
					auto *re = rk+pc->mNumRotationKeys;
					
					uint8_t fd[4]; //format descriptor

					fd[0] = 3; //InterpolateLerp
					fd[1] = 0;					
					fd[3] = 4; //4*4B

					aiVector3D pv;
					for(int pass=1;pass<=2;pass++)
					{
						if(pass==2) //translate
						{
							if(!pk) continue;
							aiVectorKey cmp; cmp.mTime = t;
							x2mm3d_sample(cmp,pk,pe,pt);
							pv = cmp.mValue;
							fd[2] = 1;
						}
						if(pass==1) //rotate
						{
							if(!rk) continue;
							aiQuatKey cmp; cmp.mTime = t;
							x2mm3d_sample(cmp,rk,re,pt);
							Assimp::EulerAnglesFromQuaternion<-1,1,2,3>(pv,cmp.mValue);
							fd[2] = 2;
						}						
						m_dst->write(fd);
						m_dst->write((uint16_t)p);
						m_dst->write((uint16_t)f);
						m_dst->write(pv);

						test++;
					}
				}
			}
			assert(test==keyframeCount);

			//TODO: Can MisfitOffsetList do this?
			auto off2 = m_dst->offset();
			animSize = off2-off1-sizeof(animSize);
			m_dst->seek(off1);
			m_dst->write(animSize);
			m_dst->seek(off2);
		}
	//	log_debug("wrote %d frame anims\n",count);
	}

	// Re-write header with offsets

	offsetList.setOffset(MDT_EndOfFile);

	m_dst->seek(12);
	for(MisfitOffsetT&mo:offsetList)
	{
		m_dst->write(mo.offsetType);
		m_dst->write(mo.offsetValue);
	}
//	log_debug("wrote %d updated offsets\n",offsetList.size());

	//return Model::ERROR_NONE;

	x2mm3d_mats.clear();
}

void x2mm3d_convert_points(aiMesh *m, aiNode *n)
{
	//CP files can only hold 33 points, so assume if there
	//are more this mesh is some kind of untextured geometry
	if(m->mNumVertices>33*3||m->mNumFaces>33) return;

	//2022: hard normals are disconnected
	//char unique[33*3] = {};	
	std::unordered_map<unsigned __int64,int> unique;
	auto hash = [](aiVector3D &v, aiColor4D *c=0)->long long int
	{
		//NOTE: this just has to teast apart
		//when a CP happens to be stacked on
		//top of another (probably at 0,0,0)
		int cp = 0; if(c)
		{
			//NOTE: this needs to be 16-bit
			cp|=std::min(31,(int)(c->r*255));
			cp|=std::min(31,(int)(c->g*255))<<5;
			cp|=std::min(31,(int)(c->b*255))<<10;
			cp|=c->a?0x8000:0;
		}

		//NOTE: shifting __int16 with 32ull isn't promoting to 
		//64bit... neither does casting 16b to 64b avoid sign-
		//extension (this is getting ridiculous)
		auto x = (unsigned __int64)(0xFFFF&((int)(v.x*1024)));
		auto y = (unsigned __int64)(0xFFFF&((int)(v.y*1024)));
		auto z = (unsigned __int64)(0xFFFF&((int)(v.z*1024)));
		//return (unsigned __int64)x|y<<16ull|z<<32ull;
		return x|y<<16|z<<32|(unsigned __int64)cp<<48;
	};

	//NOTICE: assuming source is vcolors (MDL)
	auto *vc = m->mColors[0];

	for(int i=m->mNumFaces;i-->0;) 
	for(int j=m->mFaces[i].mNumIndices;j-->0;)
	{
		int fi = m->mFaces[i].mIndices[j];
		//unique[fi]++;

		unique[hash(m->mVertices[fi],vc?vc+fi:0)]++;
	}

	size_t tris = 0;
	aiFace *pp[33],**p,**d = pp;
	for(int i=m->mNumFaces;i-->0;) 
	{
		aiFace &f = m->mFaces[i];
		if(f.mNumIndices!=3) continue;

		tris++;

		int j; for(j=3;j-->0;)		
		{
			int fi = f.mIndices[j];
			//if(unique[fi]>1) break;

			if(unique[hash(m->mVertices[fi],vc?vc+fi:0)]>1) break;
		}
		if(j==-1)
		{
			//HACK: Hide face?
			f.mNumIndices = 0;

			*d++ = &f;
		}
	}

	size_t pts = d-pp; if(!pts) return; //!

	//rcverts needs this to eliminate free vertices
	if(m->mPrimitiveTypes>=aiPrimitiveType_TRIANGLE)
	{
		if(tris==pts)
		m->mPrimitiveTypes&=~aiPrimitiveType_TRIANGLE;
	}

	unsigned int &nc = n->mNumChildren;
	aiNode **swap = n->mChildren;
	n->mChildren = new aiNode*[pts+nc];
	memcpy(n->mChildren,swap,nc*sizeof(void*));
	for(p=pp;p<d;p++)
	{
		aiNode *pt = new aiNode;
		pt->mParent = n;
		n->mChildren[nc++] = pt;			

		unsigned int *fi = (*p)->mIndices;

		int r,g,b; if(m->mColors[0])
		{
			aiColor4D &c = m->mColors[0][fi[0]];
			r = (int)(c.r*255+0.5f);
			g = (int)(c.g*255+0.5f);
			b = (int)(c.b*255+0.5f);
		}
		else
		{
			Color &c = controlpts[m->mMaterialIndex];
			if(*(int*)c&&c[3]==0)
			{
				r = c[0]; g = c[1]; b = c[2];
			}
			else r = g = b = 0; //???
		}
		char name[33]; 			
		if(r==255&&g<=31&&b==0) //Red CP?
		{
			sprintf(name,"(R%d)",g);
		}
		else if(r==0&&g==255&&b==255) //Cyan CP?
		{
			strcpy(name,"(C0)");
		}
		else //???
		{
			sprintf(name,"(%d,%d,%d)",r,g,b);
		}
		pt->mName.Set(name);
			
		aiVector3D &v0 = m->mVertices[fi[0]];
		aiVector3D &v1 = m->mVertices[fi[1]];
		aiVector3D &v2 = m->mVertices[fi[2]];

		aiMatrix4x4 &mat = pt->mTransformation;
		aiVector3D &biv = *(aiVector3D*)mat[0];
		aiVector3D &biw = *(aiVector3D*)mat[1];
		aiVector3D &dir = *(aiVector3D*)mat[2];			
		aiVector3D &pos = *(aiVector3D*)mat[3];
		pos = (v0+v1+v2)/3;
		extern float scale; 
		if(1==scale) pos/=1024; //HACK (tmd?)
		dir = ((v1-v0)^(v2-v0)).Normalize();
		//ARBITRARY: v0 looks best, but I've changed the
		//Assimp code to rotate the vertices to use 1 in
		//order to survive winding conventions reversals
		biv = (v1-pos).Normalize();
		biw = dir^biv;
		mat.Transpose();

		if(m->mAnimMeshes)
		for(int i=X->mNumAnimations;i-->0;)
		{
			aiMeshAnim *manim = NULL;
			aiAnimation *anim = X->mAnimations[i];
			for(int j=anim->mNumMeshChannels;j-->0;)				
			if(m->mName==anim->mMeshChannels[j]->mName)
			{
				manim = anim->mMeshChannels[j]; break;
			}
			if(!manim) continue;

			aiNodeAnim *na = new aiNodeAnim;
			unsigned int &nc2 = anim->mNumChannels;
			aiNodeAnim **swap = anim->mChannels;
			anim->mChannels = new aiNodeAnim*[1+nc2];
			memcpy(anim->mChannels,swap,nc2*sizeof(void*));					
			anim->mChannels[nc2++] = na;
			delete[] swap;

			na->mNodeName = pt->mName;
			//CUSTOM-EXTENSION
			na->mPreState = manim->mPreState;
			na->mPostState = manim->mPostState;

			int nk = manim->mNumKeys;
			na->mPositionKeys = new aiVectorKey[na->mNumPositionKeys=nk];
			na->mRotationKeys = new aiQuatKey[na->mNumRotationKeys=nk];
			while(nk-->0)
			{
				float t = manim->mKeys[nk].mTime;
				na->mPositionKeys[nk].mTime = t;
				na->mRotationKeys[nk].mTime = t;

				int amk = manim->mKeys[nk].mValue;
				aiVector3D *av = 
				amk==-1?m->mVertices:m->mAnimMeshes[amk]->mVertices;

				aiVector3D &v0 = av[fi[0]];
				aiVector3D &v1 = av[fi[1]];
				aiVector3D &v2 = av[fi[2]];
			
				aiMatrix4x4 mat;
				aiVector3D &biv = *(aiVector3D*)mat[0];
				aiVector3D &biw = *(aiVector3D*)mat[1];
				aiVector3D &dir = *(aiVector3D*)mat[2];			
				aiVector3D &pos = *(aiVector3D*)mat[3];
				pos = (v0+v1+v2)/3;
				if(1==scale) pos/=1024; //HACK (tmd?)
				dir = ((v1-v0)^(v2-v0)).Normalize();
				//see notes above for why v1 is prefered
				biv = (v1-pos).Normalize();
				biw = dir^biv;
				mat.Transpose().DecomposeNoScaling
				(na->mRotationKeys[nk].mValue,na->mPositionKeys[nk].mValue);
			}
		}
	}
}

//EXPERIMENTAL
/*I just realized this is a waste of time as
//long as x2mm3d is outputting the MDL::Anim
//data, since it's non-sparse.
extern void x2mm3d_lerp_prep() 
{
	//2022: The idea here is to eliminate redundant
	//keyframes with regard to linear interpolation.

	for(int i=X->mNumAnimations;i-->0;)
	{
		auto *anim = X->mAnimations[i];		
		int jN = anim->mNumChannels;
		for(int j=0;j<jN;j++)
		{
			auto *chan = anim->mChannels[j];

			int kN = (int)chan->mNumPositionKeys-1;
			if(kN>1)
			{
				//auto *cmp = &chan->mPositionKeys[0]				
				for(int k=1;k<kN;k++)
				{
					chan->mPositionKeys[k]; //UNFINISHED
				}
			}
			kN = (int)chan->mNumRotationKeys-1;
			if(kN>1)
			{
				for(int k=1;k<kN;k++)
				{
					chan->mRotationKeys[k]; //UNFINISHED
				}
			}
			kN = (int)chan->mNumScalingKeys-1;
			if(kN>1)
			{
				for(int k=1;k<kN;k++)
				{
					chan->mScalingKeys[k]; //UNFINISHED
				}
			}
		}
	}
}*/

#endif //_CONSOLE