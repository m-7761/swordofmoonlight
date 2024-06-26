
#include "x2mdl.pch.h" //PCH

//Added for snapshot transform
#include "../lib/swordofmoonlight.h"

bool x2mdo_makedir(wchar_t *file)
{
	wchar_t *pp = PathFindFileNameW((wchar_t*)file);
	if(!*pp||pp==file) return false; //-1

	wchar_t cp = pp[-1];
	pp[-1] = '\0';
	
	//really? / gets 0x3 error code (can't find path specified)
	for(auto*p=pp;p>=file;p--) if(*p=='/') *p = '\\';
	//HRESULT ok = SHCreateDirectoryExW(0,file,0); //0x3 as well
	HRESULT ok = SHCreateDirectory(0,file);
	
	pp[-1] = cp; return !ok;
}
bool x2mdo_txr(int w, int h, int pitch, void *buf, const wchar_t *file)
{
	FILE *f = _wfopen(file,L"wb");
	
	if(!f) if(x2mdo_makedir((wchar_t*)file))
	{
		f = _wfopen(file,L"wb");		
	}
	else{ assert(0); return false; }

	uint32_t hd[4] = {w,h,3*8,1}; //TODO: INDEXED? HOW?
	fwrite(hd,sizeof(hd),1,f);

	int rowsz = w*3; 
	if(rowsz%4) rowsz+=(4-rowsz%4);

	char *row = new char[rowsz];

	char *pp = (char*)buf; for(int i=h;i-->0;pp+=pitch)
	{
		char *p = pp; for(int j=0,i=w;i-->0;j+=3,p+=4) 
		{
			row[j+0] = p[0];
			row[j+1] = p[1];
			row[j+2] = p[2];
		}
		fwrite(row,rowsz,1,f);
	}

	delete[] row;

	fclose(f); return true;
}
//extern int primitive(aiMesh*,unsigned int);
extern bool primitive2(unsigned char(&)[4],char*);
void x2mdo_cp(float *p, aiMesh *m, int i, float *xf)
{
	aiFace &fi = m->mFaces[i];
					
	aiVector3D q;
	auto nn = fi.mNumIndices;
	for(auto j=nn;j-->0;)	
	q+=m->mVertices[fi.mIndices[j]];
	q*=1/(float)nn;
					
	memcpy(p,&q,3*4); 
					
	if(xf) SWORDOFMOONLIGHT::mdl::multiply(xf,p,p,1);

//	p[1] = -p[1]; //invert? 
	p[2] = -p[2];
}
extern int8_t x2mdo_rmatindex2[33] = {};
bool MDL::File::x2mdo()
{
	auto &mdl = *this;
	extern aiNode **nodetable;	
	extern char *chanindex;
	extern int snapshot;
	extern float *bindposemats; 
	extern float *snapshotmats; 

	MDL::Diff *diff = 0;
	if(auto**p=mdl.diffs) 	
	for(;*p;p++) if(snapshot==(*p)->type)
	{
		diff = *p; break; 
	}

	auto*const o = wcsrchr(name,'.');
	//assert(o); //guaranteed?
	if(!o){ assert(0); return false; } //WIP
	wchar_t l = o[3];
	assert(l=='l'&&!o[4]); //mdl?	
	FILE *f = 0, *g = 0; //MDO?
	{
		o[3] = 'o';
		f = _wfopen(name,L"wb");
		#ifndef _CONSOLE	
		if(f) dll->makelink(name); //MDO?
		#endif
		o[3] = l;
		if(!f){ assert(0); return false; } //WIP
	}
	wmemcpy(o,L".bp",4);
	if(diff||snapshotmats) //BP?
	{		
		g = _wfopen(name,L"wb"); assert(g); //WIP
	}
	#ifndef _CONSOLE
	//need to be sure there's not a scenario where
	//an old MDL file is wrongly paired with a MDO
	if(!g) DeleteFileW(name);
	#endif
	wmemcpy(o,L".mdl",5); 

	//NOTE: I was using goto at one point, and
	//gathered these to avoid compile warnings
	uint32_t dw,deinterlace;
	uint32_t i,j,n,real_part=~0;
	
	//this has to filter out materials without
	//textures and filter duplicated materials
	//2022: I don't think it was ever setup to
	//filter out "materials without textures"?
	int8_t matindex2[33];
	memset(matindex2,0xff,sizeof(matindex2));
		
	struct uv //2022
	{
		WORD sz,proc;
		
		float tu, tv;

		uv(aiVector2D &t):sz(3),proc()
		{
			tu = t.x; tv = -t.y; //I guess?
		}
	};
	std::vector<uv> uvv; aiUVTransform uvx;

	int cp = 0;
	float cps[3*4] = {};
	struct ch : SWORDOFMOONLIGHT::mdo::channel_t
	{
		float *v,*bp; uint16_t *i,*cverts; 
		
		uint8_t part; uint16_t part_verts;

		int32_t uv_fx;

		bool operator<(const ch &cmp)const //sort
		{
			//2023: I think this is making it hard
			//to control the order of transparent
			//polygons where decals are stacked
			// 
			// but it would be good to allow for
			// explicit ordering by numbering the
			// names of meshes or materials?
			// 
			//int i = *(int32_t*) //matnumber?
			//&texnumber-*(int32_t*)&cmp.texnumber;
			int i = texnumber-(int)cmp.texnumber;
			int j = matnumber-(int)cmp.matnumber;
			return i?i<0:j?j<0:blendmode<cmp.blendmode;
		}
	};
	std::vector<ch> chv;
	std::vector<uint16_t> iv;
	std::unordered_map<uint16_t,uint16_t> vv;
	
	int len,currentnode; //goto err?

		//TEXTURES?
		
	//ref count (32-bit)	
	extern char texindex[];
	extern std::vector<std::wstring> texnames;
	dw = texnames.size();
	if(f) fwrite(&dw,4,1,f);
	if(g) fwrite(&dw,4,1,g);
	//references (0-terminated)
	len = 0;
	for(auto&ea:texnames) if(!ea.empty())
	{
		//NOTE: this is a relative path
		//it can contain slashes or even .. but
		//doesn't normally do so
		auto *p = &ea[0];
		wchar_t *fn2 = PathFindFileNameW(p);
		auto *o = wcsrchr(fn2,'.');
		
		if(o) *o = '\0';

		//YUCK: mdo must be relative
		int i = 0; while(name[i]==p[i]) i++;
		while(i&&name[i-1]!='/'&&name[i-1]!='\\') i--;

		auto pi = p+i;
		//NOTE: historically .bmp is printed here
		if(f) i = fprintf(f,"%ls.bmp%c",pi,'\0');
		if(g) i = fprintf(g,"%ls.bmp%c",pi,'\0');

		if(o) *o = '.';

		if(i>0) len+=i;
		else assert(0); //goto err; //not worth it
	}
	if(len%4) //32-bit align?
	{
		dw = 0; len = 4-len%4;

		if(f) fwrite(&dw,len,1,f);
		if(g) fwrite(&dw,len,1,g);
	}
		//MATERIALS?

	typedef unsigned char Color[4];
	extern Color controlpts[33];
	extern float materials[33][4];
	extern float materials2[33][4]; //2021
	//there's a lot of duplication
	//dw = X->mNumMaterials;
	dw = 0;
	for(i=0;i<X->mNumMaterials;i++)	
	{
		//2022: don't emit fake CP materials
		//NOTE: an old comment above says it
		//was supposed to do this... maybe I
		//forgot?! I don't see any such code
		if(-1==texindex[i]) continue;

		for(j=0;j<i;j++) if(-1!=texindex[j])
		if(!memcmp(materials+i,materials+j,4*sizeof(float))
		&&!memcmp(materials2+i,materials2+j,4*sizeof(float)))
		break;
		if(j==i)
		x2mdo_rmatindex2[dw++] = (uint8_t)j;
		for(auto k=dw;k-->0;)
		if(x2mdo_rmatindex2[k]==j)
		matindex2[i] = (uint8_t)k;
	}
	if(f) fwrite(&dw,4,1,f);
	if(g) fwrite(&dw,4,1,g);
	for(i=0;i<dw;i++)
	{
		if(f) fwrite(materials+x2mdo_rmatindex2[i],16,1,f);
		if(g) fwrite(materials+x2mdo_rmatindex2[i],16,1,g);
		if(f) fwrite(materials2+x2mdo_rmatindex2[i],16,1,f);
		if(g) fwrite(materials2+x2mdo_rmatindex2[i],16,1,g);
	}
		//MODEL?
	
	//this is because x2mdl.cpp is merging
	//soft models so they're only one part
	//(there is a facility for a multipart
	//soft model but it's not been tested)
	int cverts0 = 0;
	int cvertsN = 0; if(diffs) //YUCK
	{
		for(int k=0;k<head.parts;k++)		
		{
			Part &pt = parts[k];
			cvertsN+=pt.verts+pt.extra;
		}
	}
	//DUPLICATE
	//this is the part of x2mdl.cpp that converts the
	//vertices and normals. they can't be converted in
	//place because of -1 indices in soft animation and
	//if Assimp instances meshes
	currentnode = 0;
	chv.reserve(32);
	//dw = deinterlace = 0;
	for(int k=0;k<mdl.head.parts;k++) 
	{				
		//MDL::File::flush seems not to be in this order?
		//Part &pt = *mdl.parts[k].cpart;
		Part &pt = mdl.parts[k];

		int cverts00 = cverts0;
		if(cvertsN) cverts0+=pt.verts;

		/*2020: vertices may be in different order if 
		//converting/consolidating points to triangle
		//control points
		while(nodetable[currentnode]->mNumMeshes==0)
		currentnode++;*/
		currentnode = pt.cnode;

		Color cp2;
		bool p2 = primitive2(cp2,nodetable[currentnode]->mName.data);

		auto *cv = pt.cverts, *cv2 = cv;

		//NOTE: copying MDL::File::flush
		//if(pt.cextra) continue; //I think?		
		if(!pt.cextra) real_part++; //(CP detection)

		int currentchan = chanindex?chanindex[currentnode]:0;
		for(int l=0;l<nodetable[currentnode]->mNumMeshes;l++,cv=cv2)
		{
			aiMesh *m = X->mMeshes[nodetable[currentnode]->mMeshes[l]];

			if(cv) cv2 = cv+m->mNumVertices;
						
			//CP? GOOD ENOUGH?
			int mi = m->mMaterialIndex;			
			
			bool p3 = p2||primitive2(cp2,m->mName.data);

			if(p3||controlpts[mi][3]) //2021
			if(2!=m->mNumUVComponents[0]) //2022: UVs? (TMD)
			{					
				n = m->mNumFaces;
				for(i=0;cp<4&&i<n;i++)
				{
					//YUCK: ignore unnamed points :(
					int ni = m->mFaces[i].mNumIndices;
					if(!(ni==3||p3&&ni==1)) continue;

					float *p = cps+cp++*3;
					float *xf = bindposemats+currentchan*16;
					x2mdo_cp(p,m,i,bindposemats?xf:0);
				}
				continue; //control point?
			}
			assert(m->mPrimitiveTypes>=aiPrimitiveType_TRIANGLE);

			if(pt.cextra){ assert(0); continue; } //I think?
			
			int bm;
			if(!X->mMaterials[mi]->Get(AI_MATKEY_BLEND_FUNC,bm))
			bm = bm==aiBlendMode_Additive;
			else bm = 0;

			//EXTENSION
			int uv_fx = -1; unsigned _ = 5;
			//for some reason the template form of Get errors out if 
			//the property isn't aiPTI_Buffer type. for some reason
			//aiUVTransform has Float type.
			//if(!X->mMaterials[mi]->Get(AI_MATKEY_UVTRANSFORM(0,0),uvx))
			if(!X->mMaterials[mi]->Get(AI_MATKEY_UVTRANSFORM(0,0),(float*)&uvx,&_))
			{
				uv v(uvx.mTranslation);

				for(auto&ea:uvv) if(!memcmp(&v,&ea,sizeof(v)))
				{
					uv_fx = &ea-uvv.data(); break;
				}
				if(-1==uv_fx) 
				{
					uv_fx = (int)uvv.size(); uvv.push_back(v);
				}
			}

			//HACK: historically som_db.exe's vbuffer limit is 4096
			//NOTE: historically this number would be 128!!
			//HACK: -2 ensures whole triangles get buffered
			enum{ chunk_size=4096-2 };

			bool chunk = false;

			n = m->mNumFaces;
			for(i=0;i<n;i++)
			{
				/*I think this is impossible since CPs should
				//have a plain white base material
				if(0==primitive(m,i))
				{
					if(cp<4)
					{
						float *p = cps+cp++*3;
						float *xf = bindposemats+currentchan*16;
						x2mdo_cp(p,m,i,bindposemats?xf:0);
					}
					continue; //control point?
				}*/

				aiFace *fp = m->mFaces+i;
				auto nn = fp->mNumIndices;
				if(nn==3) for(j=3;j-->0;)
				{
					auto fi = (uint16_t)fp->mIndices[j];

					auto ins = vv.insert(std::make_pair(fi,(uint16_t)vv.size()));

					iv.push_back((uint16_t)ins.first->second);

					if(ins.second&&vv.size()>=chunk_size)
					{
						chunk = true;
					}							
				} 
				else{ assert(0); continue; } //CP?

				if(chunk) chunk:
				{
					chunk = false;

					ch tmp = {}; tmp.uv_fx = uv_fx;
					
					tmp.blendmode = bm==1;
					//extern BYTE texindex[33];
					tmp.texnumber = (int16_t)texindex[mi];
					tmp.matnumber = (int16_t)matindex2[mi];

					j = iv.size();
					tmp.ndexcount = (uint16_t)j;
					if(!j) continue;

					if(0) //REFERENCE
					{
						//this is moved out of this block to
						//after sorting according to drawing

						/*deinterlace?
						//not aligning so can be combined in 
						//drawing command
						if(j%2)
						{
							j++;
							iv.push_back(0xFFFF);
						}*/					
						tmp.ndexblock = dw;
						//j*=2;
						//tmp.vertblock = dw+j;
						//dw+=j+vv.size()*8*4;
						dw+=j*2;
						tmp.vertblock = deinterlace;
						deinterlace+=vv.size()*8*4;
					}

					tmp.i = new uint16_t[iv.size()];
					memcpy(tmp.i,iv.data(),2*iv.size());

					tmp.vertcount = (uint16_t)vv.size();

					if(cv) 
					{
						tmp.cverts = new uint16_t[vv.size()];					
						//2024: adding cstart for x2ico_mdo/x2ico::animator #1
						if(cvertsN) //YUCK
						for(auto&ea:vv)
						tmp.cverts[ea.second] = cverts00+cv[ea.first];
						else
						for(auto&ea:vv)
						tmp.cverts[ea.second] = pt.cstart+cv[ea.first];
					}

					//the BP file just has different coordinates
					for(int pass=1;pass<=2;pass++)
					{
						float* &vp = pass==1?tmp.v:tmp.bp;

						if(!(pass==1?f:g)){ vp = 0; continue; }

						float *pp = vp = new float[8*vv.size()]();

						float *xf = pass==1?snapshotmats:bindposemats;
						if(!xf) xf = bindposemats;
					
						for(auto&ea:vv)
						{
							size_t s = ea.first;

							float *p = pp+8*ea.second;

							if(auto*q=m->mVertices)
							{
								q+=s;

								memcpy(p,q,3*4);

								//WARNING: this may be required to work since I don't see Assimp's
								//node transform. REMINDER: this may be be 0 for static/MDO inputs
								if(xf) SWORDOFMOONLIGHT::mdl::multiply(xf+currentchan*16,p,p,1);

							//	p[1] = -p[1]; //invert? 
								p[2] = -p[2];
							}
							p+=3;
							if(auto*q=m->mNormals)
							{
								q+=s;

								memcpy(p,q,3*4);

								//WARNING: same note as above
								if(xf) SWORDOFMOONLIGHT::mdl::multiply(xf+currentchan*16,p,p,0); 

							//	p[1] = -p[1]; //invert? 
								p[2] = -p[2];
							}
							p+=3;
							if(auto*q=m->mTextureCoords[0])
							{
								q+=s;

								p[0] = q->x;
								p[1] = 1-q->y; //guessing?
							}
						}
						if(diff&&pass==1) 
						{
							//enum{ n=1 }; //DUPLICATE (x2mdl.cpp)
							enum{ pose=1 };

							auto *bv = verts;
							auto *d1 = diff->verts;
							auto *d2 = diff->verts+diff->width;
							auto *dt = diff->times();
							float t; if(diff->steps==1||dt[0]>=(1+pose))
							{
								d2 = d1; d1 = bv;

								t = (1+pose)/dt[0];
							}
							else t = ((1+pose)-dt[0])/(dt[1]-dt[0]);

							if(t>1) t = 1; //still image?

							for(auto&ea:vv)
							{
								float *p = pp+8*ea.second;

								int ci = 3*tmp.cverts[ea.second];

								//2024: adding cstart for x2ico_mdo/x2ico::animator #2
								//NOTE: hit this on e117.mdl. Gold Golem
								//DUPLICATE: x2mdl.cpp
//								assert(!pt.cstart);

								auto *o = bv+ci, *x = d1+ci, *y = d2+ci;

								float d[3]; for(int i=3;i-->0;)
								{
									d[i] = x[i]+t*(y[i]-x[i])-o[i];
								}
								p[0]+=d[0]*+0.0009765625f; //+1/1024
								p[1]+=d[1]*-0.0009765625f; //-1/1024
								p[2]+=d[2]*+0.0009765625f; //+1/1024
							}
						}
					}
					
					//part_verts is a checksum so som_game.exe wouldn't have
					//to scan the MDO->MDL link indices to ensure they match
					tmp.part = (uint8_t)real_part;
					tmp.part_verts = (uint16_t)(cvertsN?cvertsN:pt.verts+pt.extra);
					//HACK: just a guess to get exact match
					if(k&&!cvertsN)
					for(int kk=0;kk<head.parts;kk++) //2021
					if(k==parts[kk].cextra)
					{
						//this is based on loops inside of
						//MDL::File::flush
						tmp.part_verts+=parts[kk].verts;
						tmp.part_verts+=parts[kk].extra;
					}

					chv.push_back(tmp);

					iv.clear(); vv.clear();
				}			
			}
			if(!iv.empty())
			{
				chunk = true; goto chunk;
			}
		}		

		currentnode++;
	}
	dw = deinterlace = 0;
	std::sort(chv.begin(),chv.end());
	for(auto&ea:chv)
	{
		//this recapipulates the disabled code
		//from the main block in order to sort
		//to optimize drawing

		ea.ndexblock = dw;					
		dw+=ea.ndexcount*2;
		ea.vertblock = deinterlace;
		deinterlace+=ea.vertcount*8*4;
	}

	//enum{ ext=2 }; //size/version (64-bits)
	enum{ ext=2+1 }; //uv animation

	//cps (3*4)
	if(f) fwrite(cps,12*4,1,f);
	if(g) fwrite(cps,12*4,1,g);
	//ch count (32-bit)
	dw = chv.size();
	if(f) fwrite(&dw,4,1,f);
	if(g) fwrite(&dw,4,1,g);
	//channels (discriptors)
	dw = ftell(f?f:g)+chv.size()*20;
	//EXTENSION?
	//if(~real_part)
	{	
		dw+=ext*4*chv.size();

		size_t jmp = 20/4*chv.size();
		for(auto&ea:chv)
		{
			ea.extrasize = ext;
			ea.extradata = jmp; jmp+=ext-5;
		}		
	}	
	
	uint32_t odd;
	deinterlace = 0;
	for(auto&ea:chv)
	deinterlace+=ea.ndexcount;
	if(odd=deinterlace%2)
	deinterlace++;
	deinterlace*=2;
	deinterlace+=dw;
	for(auto&ea:chv)
	{
		ea.ndexblock+=dw;
		//ea.vertblock+=dw;
		ea.vertblock+=deinterlace;
		if(f) fwrite(&ea,20,1,f);
		if(g) fwrite(&ea,20,1,g);
	}

	dw = ftell(f?f:g); //TRICKY	
	//if(~real_part)
	{	
		if(f) fseek(f,ext*4*chv.size(),SEEK_CUR);
		if(g) fseek(g,ext*4*chv.size(),SEEK_CUR);
	}

	/*classical data
	for(auto&ea:chv)
	{
		//indices (16-bit)
		//verts (pos+lit+uv)		

		//NOTE: historically I think these are interleaved
		//but I'm not positive (maybe would be better not)

		i = ea.ndexcount;
		if(i%2) i++;
		if(f) fwrite(ea.i,2*i,1,f);
		if(g) fwrite(ea.i,2*i,1,g);
		if(f) fwrite(ea.v,8*4*ea.vertcount,1,f);
		if(g) fwrite(ea.bp,8*4*ea.vertcount,1,g);
	}*/
	//deinterlace data
	{
		//this way games can do an optimization by having
		//back-to-back parts draw with a single draw call
		//(TODO: NEED TO PRESORT PARTS BY THEIR MATERIAL)
		
		for(auto&ea:chv) 
		{
			if(f) fwrite(ea.i,2*ea.ndexcount,1,f);
			if(g) fwrite(ea.i,2*ea.ndexcount,1,g);
		}

		if(odd) //32-bit align?
		{
			odd = ~0;
			if(f) fwrite(&odd,2,1,f);
			if(g) fwrite(&odd,2,1,g);
		}
		assert(!f||ftell(f)%4==0);

		for(auto&ea:chv) 
		{
			if(f) fwrite(ea.v,8*4*ea.vertcount,1,f);
			if(g) fwrite(ea.bp,8*4*ea.vertcount,1,g);
		}
	}

	auto uvv_pos = ftell(f?f:g); //2022
	if(!uvv.empty())
	{
		if(f) fwrite(uvv.data(),sizeof(uv)*uvv.size(),1,f);
		if(g) fwrite(uvv.data(),sizeof(uv)*uvv.size(),1,g); //?
	}

	//if(~real_part) //EXTENSION
	//{
		struct
		{
			uint8_t part,_pad;
			uint16_t part_verts;
			uint32_t cverts_pos;
			//version 2
			uint32_t uv_fx_pos;
		}rec;
		rec._pad = 0; //reserved
		rec.cverts_pos = ftell(f?f:g);
	//}
	if(~real_part) //TRICKY
	{
		int compile[ext==sizeof(rec)/4]; (void)compile;

		//write MDO->MDL links data
		for(auto&ea:chv)
		{
			if(f) fwrite(ea.cverts,2*ea.vertcount,1,f);
			if(g) fwrite(ea.cverts,2*ea.vertcount,1,g); //?
		}
	}
	//if(~real_part) //EXTENSION
	{
		//write extra data descriptors
		if(f) fseek(f,dw,SEEK_SET);
		if(g) fseek(g,dw,SEEK_SET);
		for(auto&ea:chv)
		{
			rec.part = ea.part;
			rec.part_verts = ea.part_verts;
			//version 2			
			if(-1!=ea.uv_fx) 
			rec.uv_fx_pos = uvv_pos+sizeof(uv)*ea.uv_fx;
			else rec.uv_fx_pos = 0; 

			if(f) fwrite(&rec,sizeof(rec),1,f);
			if(g) fwrite(&rec,sizeof(rec),1,g); //?

			rec.cverts_pos+=2*ea.vertcount;
		}
	}

	for(auto&ea:chv) 
	{
		delete[] ea.i;
		delete[] ea.v; 
		delete[] ea.bp;
		delete[] ea.cverts;
	}

	if(f) fclose(f);
	if(g) fclose(g); return true;
}

static struct x2msm_split_level //2022
{
	//this is for purple CPs that define
	//uneven terrain or a base elevation 

	union{ int cpmax,cpmask; }; //5 bits
	operator int&(){ return cpmask; }
	void clear(){ cpmask = 0; } //operator=

	//TODO? 8 is an arbitrary limit
	aiVector3D cpoints[8];
	aiVector2D frustum[8]; //tangent
	aiVector3D normals[8];

	float planes_d[4];
	float planes_t(int i, const aiVector3D &a, aiVector3D b)
	{
		auto &abc = normals[i]; float d = planes_d[i];

		return (d-abc*b)/(abc*(a-b)); //zero divide?
	}
	float planes_y(int i, const aiVector3D &a)
	{
		if(float ny=normals[i].y) //zero divide?
		return (planes_d[i]-normals[i]*a)/ny+a.y; return a.y;
	}

	size_t operator()(float,size_t&,uint16_t*,float*,std::vector<float>&);

	void validate()
	{
		int n; for(int i=n=0;cpmask>>n;i++)
		{
			//HACK: collapsing is easier than
			//error handling
			if(cpmask&(1<<i))
			{
				if(i!=n)
				cpoints[n] = cpoints[i]; n++;
			}
		}
		cpmax = n; //UNION

		if(n<=1) return;

		for(int i=n;i-->0;)
		{
			//matching door CPs counterclockwise order
			frustum[i].Set(cpoints[i].x,cpoints[i].z);
			frustum[i].Normalize();
			frustum[i].Set(-frustum[i].y,frustum[i].x);
		}
		for(int j=n-1,i=0;i<n;j=i++)
		{
			//just averaging to approximate 
			//a projective plane
			aiVector3D &a = cpoints[i], &b = cpoints[j];
			//just needs to be perpendicular
			aiVector3D ab = a-b;
			//HACK: could probably figure out simpler math
			aiVector3D c = ab^aiVector3D(0,1,0);

			//normal of a triangle
			//normals[i] = (a-c)^(b-c); //cross
			normals[i] = ab^c; //cross
			normals[i].Normalize(); //abc

			planes_d[i] = b*normals[i];
		}
	}

}x2msm_split2; //SINGLETON

bool MDL::File::x2msm(bool x2mhm) //2022
{
	x2msm_split2.clear();

	//FYI: ADAPTING FROM x2mdo

	auto &mdl = *this;
	extern aiNode **nodetable;	
	extern char *chanindex;
	extern int snapshot;
	extern float *bindposemats; 
	extern float *snapshotmats;
		
	MDL::Diff *diff = 0;
	if(auto**p=mdl.diffs) 	
	for(;*p;p++) if(snapshot==(*p)->type)
	{
		diff = *p; break; 
	}

	auto*const o = wcsrchr(name,'.');
	//assert(o); //guaranteed?
	if(!o){ assert(0); return false; } //WIP
	wchar_t l = o[3];
	assert(l=='l'&&!o[4]); //mdl?	
	FILE *f = 0, *g = 0; //MSM?
	{
		wmemcpy(o,L".mhm",4);
		f = _wfopen(name,L"wb");
		if(!f){ assert(0); return false; } //WIP

		if(!x2mhm) //MHM is always outputted
		{
			wmemcpy(o,L".msm",4);
			g = _wfopen(name,L"wb");	
//			o[3] = l;
//			if(!g){ assert(0); return false; } //WIP
			assert(g);
		}
		if(!f&&!g){ assert(0); return false; } //WIP
		#ifndef _CONSOLE	
		else dll->makelink(name); //MSM? //MHM?
		#endif
	}
//	wmemcpy(o,L".mdl",5); //SoftMSM?

	uint32_t dw,i,j,n;

	int cp = 0; float cps[3*4],c0[3] = {}; //UNFINISHED
	
	//TODO //TODO //TODO //TODO
	// 	  
	// my MM3D code has a pretty good algorithm for 
	// simplifying meshes which would be a big help
	// to convert MSM/MDO to MHM by removing UV map
	// data and do_aa stuff to lower polygon counts
	//
	struct nml //MHM
	{
		size_t x:14,y:14,sign:4; //8192

		operator size_t()const{ return *(size_t*)this; }
	};
	struct edg //MHM
	{
		uint16_t a,b,tri,w:1;	
	//	uint16_t x:1,y:1,z:1,w:1,diagonal:1;

		bool operator<(const edg &cmp)const //sort
		{
			return *(uint32_t*)&a<*(uint32_t*)&cmp.a;
		}

		operator uint32_t()const{ return *(uint32_t*)this; }
	};
	struct tri
	{
		uint16_t mi,vi,fi,ni;
		
		uint16_t coplanar,cverts00;

		unsigned short *cv;
	};
	std::vector<tri> tris;
	std::vector<edg> dgls;
	std::unordered_map<size_t,size_t> nmls;
	//instancing always demands duplication
	std::vector<float> vrts;	
	std::vector<aiVector3D> cvrts; //MHM
	std::vector<aiVector3D> cnmls; //TESTTING (REMOVE ME)

	int len,currentnode,currentvert; //goto err?

		//TEXTURES?
		
	//ref count (16-bit)	
	extern char texindex[]; if(g) //MSM
	{
		extern std::vector<std::wstring> texnames;
		auto w = (uint16_t)texnames.size();
		fwrite(&w,2,1,g);
		//references (0-terminated)
		len = 0;
		for(auto&ea:texnames) if(!ea.empty())
		{
			//NOTE: this is a relative path
			//it can contain slashes or even .. but
			//doesn't normally do so
			auto *p = &ea[0];
			wchar_t *fn2 = PathFindFileNameW(p);
			auto *o = wcsrchr(fn2,'.');
		
			if(o) *o = '\0';

			//YUCK: mdo must be relative
			int i = 0; while(name[i]==p[i]) i++;
			while(i&&name[i-1]!='/'&&name[i-1]!='\\') i--;

			auto pi = p+i;
			//NOTE: historically .bmp is printed here
			i = fprintf(g,"%ls.bmp%c",pi,'\0');

			if(o) *o = '.';

			if(i>0) len+=i;
			else assert(0); //goto err; //not worth it
		}
		if(len%4) //32-bit align? //EXTENSION
		{
			//COMPLICATED: len starts at 2 and the
			//vertex array count is also 2 bytes, so
			//it has to be on a 2 so the array is on 4
			int pos = 2+len;
			while(pos%4!=2) pos++;
		
			dw = 0; len = pos-2-len;
			fwrite(&dw,len,1,g);
			w+=len;
			auto os = ftell(g);
			fseek(g,0,SEEK_SET);
			fwrite(&w,2,1,g);
			fseek(g,os,SEEK_SET);
		}
	}
		//MATERIALS?

	typedef unsigned char Color[4];
	extern Color controlpts[33];
//	extern float materials[33][4];
//	extern float materials2[33][4]; //2021

			//MODEL?
	
	//this is because x2mdl.cpp is merging
	//soft models so they're only one part
	//(there is a facility for a multipart
	//soft model but it's not been tested)
	int cverts0 = 0;
	int cvertsN = 0; if(diffs) //YUCK
	{
		for(int k=0;k<head.parts;k++)
		{
			Part &pt = parts[k];
			cvertsN+=pt.verts+pt.extra;
		}
	}

	//OPTIMIZING
	int reservetris = 0;
	int reservevrts = 0;
	currentnode = currentvert = 0;
//	chv.reserve(32);
	for(int k=0;k<mdl.head.parts;k++) 
	{
		Part &pt = mdl.parts[k];

		currentnode = pt.cnode;
		int currentchan = chanindex?chanindex[currentnode]:0;
		for(int l=0;l<nodetable[currentnode]->mNumMeshes;l++)
		{
			aiMesh *m = X->mMeshes[nodetable[currentnode]->mMeshes[l]];

			currentvert+=m->mNumVertices;
			reservetris+=m->mNumFaces;
		}

		reservevrts+=pt.verts;
	}
	vrts.resize(8*currentvert);
	cvrts.resize(reservevrts); //MHM
	tris.reserve(reservetris);
	nmls.reserve(reservetris);
	dgls.reserve(3*reservetris);

	static bool degen = false;

	//DUPLICATE
	//this is the part of x2mdl.cpp that converts the
	//vertices and normals. they can't be converted in
	//place because of -1 indices in soft animation and
	//if Assimp instances meshes
	currentnode = currentvert = 0;
//	chv.reserve(32);
	for(int k=0;k<mdl.head.parts;k++) 
	{				
		//MDL::File::flush seems not to be in this order?
		//Part &pt = *mdl.parts[k].cpart;
		Part &pt = mdl.parts[k];

		int cverts00 = cverts0;
		if(cvertsN) cverts0+=pt.verts;

		/*2020: vertices may be in different order if 
		//converting/consolidating points to triangle
		//control points
		while(nodetable[currentnode]->mNumMeshes==0)
		currentnode++;*/
		currentnode = pt.cnode;
				
		Color cp2;
		bool p2 = primitive2(cp2,nodetable[currentnode]->mName.data);

		auto *cv = pt.cverts, *cv2 = cv;

		//NOTE: copying MDL::File::flush
		//if(pt.cextra) continue; //I think?		
//		if(!pt.cextra) real_part++; //(CP detection)

		int currentchan = chanindex?chanindex[currentnode]:0;
		for(int l=0;l<nodetable[currentnode]->mNumMeshes;l++,cv=cv2)
		{
			auto mii = (uint16_t)nodetable[currentnode]->mMeshes[l]; //mi

			aiMesh *m = X->mMeshes[mii];

			if(cv) cv2 = cv+m->mNumVertices;
						
			//CP? GOOD ENOUGH?
			int mi = m->mMaterialIndex;			
			
			bool p3 = p2||primitive2(cp2,m->mName.data);

			if(p3||controlpts[mi][3]) //2021
			if(2!=m->mNumUVComponents[0]) //2022: UVs? (TMD)
			{
				//TODO: might want x2mm3d_convert_points like
				//discrimination here to let untextured faces
				//pass through if not single triangles?

				bool cyan = cp2[1]==255&&cp2[2]==255;
				bool purple = cp2[0]==255&&cp2[2]==255;
				int green = cp2[1];

				n = m->mNumFaces;
				for(i=0;i<n;i++)
				{
					auto &f = m->mFaces[i];

					//YUCK: ignore unnamed points :(
					int ni = f.mNumIndices;
					if(!(ni==3||p3&&ni==1)) continue;

					if(auto*c=m->mColors[0])
					{
						auto cmp = c[f.mIndices[0]];
						if(purple=cmp.r==1.0f&&cmp.b==1.0f)
						{
							green = cmp.g*255;
							for(int j=f.mNumIndices;j-->1;)						
							if(cmp!=c[f.mIndices[j]])
							purple = false;
						}
						if(cyan=cmp.g==1.0f&&cmp.b==1.0f)
						{
							for(int j=f.mNumIndices;j-->1;)						
							if(cmp!=c[f.mIndices[j]])
							cyan = false;
						}
					}

					float *p; if(purple)
					{
						if(green>=0&&green<=_countof(x2msm_split2.cpoints))
						{
							x2msm_split2|=1<<green;

							p = &x2msm_split2.cpoints[green].x; //HACK
						}
						else continue;
					}
					else if(cyan)
					{
						p = c0;
					}
					else if(cp<4)
					{
						p = cps+cp++*3;
					}
					else continue;

					float *xf = bindposemats+currentchan*16;
					x2mdo_cp(p,m,i,bindposemats?xf:0);
				}
				continue; //control point?
			}
			assert(m->mPrimitiveTypes>=aiPrimitiveType_TRIANGLE);

			if(pt.cextra){ assert(0); continue; } //I think?

			if(!m->mNumFaces){ assert(0); continue; }

			if(!m->mNormals){ assert(!g); continue; }

			n = m->mNumVertices;

			float *xf = snapshotmats;
			if(!xf) xf = bindposemats;
			float *p = &vrts[currentvert*8];
			auto *q = m->mVertices;
			if(q) for(i=n;i-->0;p+=8,q++)
			{
				memcpy(p,q,3*4);

				//WARNING: this may be required to work since I don't see Assimp's
				//node transform. REMINDER: this may be be 0 for static/MDO inputs
				if(xf) SWORDOFMOONLIGHT::mdl::multiply(xf+currentchan*16,p,p,1);

			//	p[1] = -p[1]; //invert? 
				p[2] = -p[2];
			}
			p = &vrts[currentvert*8+3];
			q = m->mNormals;
			if(q) for(i=n;i-->0;p+=8,q++)
			{
				memcpy(p,q,3*4);

				//WARNING: same note as above
				if(xf) SWORDOFMOONLIGHT::mdl::multiply(xf+currentchan*16,p,p,0); 

			//	p[1] = -p[1]; //invert? 
				p[2] = -p[2];
			}
			p = &vrts[currentvert*8+6];
			q = m->mTextureCoords[0];
			if(q) for(i=n;i-->0;p+=8,q++)
			{
				p[0] = q->x;
				p[1] = 1-q->y; //guessing?
			}
			if(diff) 
			{
				//SHADOWING (n)
				//enum{ n=1 }; //DUPLICATE (x2mdl.cpp)
				enum{ pose=1 };

				auto *bv = mdl.verts;
				auto *d1 = diff->verts;
				auto *d2 = diff->verts+diff->width;
				auto *dt = diff->times();
				float t; if(diff->steps==1||dt[0]>=(1+pose))
				{
					d2 = d1; d1 = bv;

					t = (1+pose)/dt[0];
				}
				else t = ((1+pose)-dt[0])/(dt[1]-dt[0]);
				
				if(t>1) t = 1; //still image?

				p = &vrts[currentvert*8];
				for(i=0;i<n;i++,p+=8)
				{
				//	float *p = pp+8*ea.second;
				//	int ci = 3*tmp.cverts[ea.second];
					int ci = 3*(cverts00+cv[i]);

					auto *o = bv+ci, *x = d1+ci, *y = d2+ci;

					float d[3]; for(int i=3;i-->0;)
					{
						d[i] = x[i]+t*(y[i]-x[i])-o[i];
					}
					p[0]+=d[0]*+0.0009765625f; //+1/1024
					p[1]+=d[1]*-0.0009765625f; //-1/1024
					p[2]+=d[2]*+0.0009765625f; //+1/1024
				}
			}
			p = &vrts[currentvert*8];
			if(cv) for(i=0;i<n;i++,p+=8)
			{
				cvrts[cverts00+cv[i]] = *(aiVector3D*)p;
			}
			else assert(0);
			
			n = m->mNumFaces;

			for(i=0;i<n;i++) 
			{
				aiFace *fp = m->mFaces+i;
				auto nn = fp->mNumIndices;

				auto ti = tris.size();

				aiVector3D *np[3];

				//to avoid concave quads and for esthetic
				//reasons only the longest edge of a face
				//that is diagonal is stored
				edg e; e.tri = 0xffff; 
				int a,b = currentvert+fp->mIndices[nn-1];
				bool diagonal;
				int diagonals = 0;
				float cmp2[2] = {};				
				for(j=0;j<nn;j++,b=a)
				{
					a = currentvert+fp->mIndices[j];

					float *av = &vrts[a*8];
					float *bv = &vrts[b*8];
					float x2 = av[0]-bv[0]; x2*=x2;
					float y2 = av[1]-bv[1]; y2*=y2;
					float z2 = av[2]-bv[2]; z2*=z2;

					//TODO: maybe try to score this?
					static const float t2 = 0.00009f; //0.009^2
					if(diagonal=(x2>t2)+(y2>t2)+(z2>t2)>1)
					diagonals++;
					
					float mag2 = x2+y2+z2; 

					if(mag2>cmp2[diagonal]) //>= perhaps?
					{
						cmp2[diagonal] = mag2; //longest edge yet?

						//static const float t2 = 0.00009f; //0.009^2
						//if((x2>t2)+(y2>t2)+(z2>t2)>1) //diagonals only
						if(diagonal)
						{
							e.a = a; e.b = b; e.tri = ti;
							if(e.w=a>b) std::swap(e.a,e.b);
						}
						else //diagonal is shorter?
						{
							//this poses a slight problem for sloped
							//tile walls and some others when there's
							//a small diagonal on the bottom that lets
							//a straight edge be longer than the inner
							//diagonal
							//e.tri = 0xffff;
						}
					}

					np[std::min(2u,nn-1-j)] = (aiVector3D*)av; //HACK
				}				
				if(diagonals>1) 
				cmp2[0]*=0.9f; //ARBITRARY
				if(cmp2[diagonal]<cmp2[0])
				{
					e.tri = 0xffff;
				}
				aiVector3D fn = (*np[1]-*np[0])^(*np[2]-*np[0]);		
				fn.Normalize();
				//KF2's data has some nasty degenerate triangles which causes
				//SOM's clipping to generate false positives
				if(_isnan(fn.x)||_isnan(fn.y)||_isnan(fn.z))
				{
					if(!degen)
					{
						degen = true;

						MessageBoxW(X2MDL_MODAL,PathFindFileNameW(name),
						L"x2mdl: NaN normal in MSM/MHM output",MB_OK|MB_ICONINFORMATION);
					}
					continue; //assuming zero area triangle?
				}
				if(e.tri!=0xffff) dgls.push_back(e);

				auto fn2 = fn; //TESTING
				//this is more complicated than it needs to be (after changes)
				float &x = fn.x, &y = fn.y, &z = fn.z;
				unsigned sign = z<0?4:0;
				if(x<0){ sign|=1; x = fabsf(x); }
				if(y<0){ sign|=2; y = fabsf(y); }
				struct nml nml = {x*8192+0.5f,y*8192+0.5f,sign};
				auto ins = nmls.insert(std::make_pair((size_t)nml,nmls.size()));
				if(ins.second)
				{
					//415a4e has ==1.0 (using for clipmode)
					if(nml.y==8192) fn2.y = sign&2?-1:1;
					if(nml.y==0) fn2.y = 0;
					cnmls.push_back(fn2);
				}
				struct tri tri = {mii,currentvert,i,ins.first->second,0,cverts00,cv};
				tris.push_back(tri);
			}

			currentvert+=m->mNumVertices;			
		}		

		currentnode++;
	}
	vrts.resize(8*currentvert);
	
	if(c0[0]||c0[1]||c0[2]) //recenter?
	{
		auto &c = *(aiVector3D*)c0;
		auto *d = vrts.data();
		for(auto i=vrts.size()-8;i>=0;i-=8) 
		(*(aiVector3D*)(d+i))-=c;
		if(x2msm_split2)
		for(int i=4;i-->0;) 
		x2msm_split2.cpoints[i]-=c;
	}
	x2msm_split2.validate();

	//NOTE: to reproduce the original subdivision patterns the
	//top-level triangles need to be quadrangles, so this must
	//be done for MSM too
	size_t coplanar = 0;
	auto *ep = dgls.data();
	std::sort(dgls.begin(),dgls.end());
	if(!dgls.empty()) //-1?
	for(i=0,n=dgls.size()-1;i<n;i++)	
	if(ep[i]==ep[i+1]&&ep[i].w!=ep[i+1].w) //only naive cases?
	{
		auto &e = ep[i];
		//
		// WARNING
		// for MHM if the MSM has different UVs that polygon
		// won't be merged (could use consolidated vertices)
		// a fix must use separate edges between MSM and MHM
		// NOTE: diagonal edges will tend to share their UVs
		//
		//this is to avoid merging lateral/vertical edges that
		//don't improve clipping and could even make it worse?
	//	if(!e.diagonal) continue;

		auto &t1 = tris[e.tri], &t2 = tris[ep[i+1].tri];

		if(t1.ni!=t2.ni||t1.coplanar||t2.coplanar) continue;

		//TODO: it would be nice to continue to merge
		//triangles, especially if the result is still
		//a quadrangle, but it requires some proof that
		//results aren't concave/self-intersecting ngons

		t1.coplanar = i+1; //HACK: nonzero (i may be 0)
		t2.coplanar = 0xffff; //limiting to quadrangles??

		coplanar++;
	}

	if(f) //MHM //MHM //MHM //MHM //MHM //MHM //MHM //MHM
	{
		auto clipmode = [](float y)
		{
			return y==0?1:fabsf(y)==1?2:4;
		};

		//HACK: just write an empty file?
		if(tris.empty()) cvrts.clear();

		//MSM has many duplicated vertices!!
		//fwrite(&(dw=vrts.size()/8),4,1,f);
		fwrite(&(dw=cvrts.size()),4,1,f);
		fwrite(&(dw=nmls.size()),4,1,f);
		fwrite(&(dw=tris.size()-coplanar),4,1,f);
		uint32_t sfc[3] = {};
		for(auto&ea:tris) if(ea.coplanar!=0xffff)
		{
			sfc[clipmode(cnmls[ea.ni].y)/2]++;
		}
		fwrite(sfc,3*4,1,f);
		//for(i=0,n=vrts.size();i<n;i+=8)
		for(i=0,n=cvrts.size();i<n;i++)
		{
			fwrite(&cvrts[i],3*4,1,f); //vrts
		}
		for(auto ea:cnmls)
		{
			//NOTE: I'm not sure it's reasonable 
			//to reconstruct the Z coordinate from
			//the nmls vector. I think it's probably
			//ossible
			fwrite(&ea,3*4,1,f);
		}
		for(uint32_t pass=1;pass<=4;pass=pass<<1)
		{
			if(!sfc[pass/2]) continue;

			for(auto&ea:tris) if(ea.coplanar!=0xffff)
			{
				dw = clipmode(cnmls[ea.ni].y);

				if(pass!=dw) continue;
			
				float box[2][3]; for(int j=3;j-->0;)
				{
					box[0][j] = +FLT_MAX;
					box[1][j] = -FLT_MAX;
				}

				uint32_t c = 0; //corners			
				for(int pass=ea.coplanar!=0;pass>=0;pass--)
				{
					if(pass)
					{
						i = tris[dgls[ea.coplanar].tri].fi;
					}
					else i = ea.fi;

					auto &fi = X->mMeshes[ea.mi]->mFaces[i];
			
					i = fi.mNumIndices; c+=pass?i-2:i;

					while(i-->0)
					{
						float *v = &vrts[8*(ea.vi+fi.mIndices[i])];

						for(int j=3;j-->0;)
						{
							if(v[j]<box[0][j]) box[0][j] = v[j];
							if(v[j]>box[1][j]) box[1][j] = v[j];
						}
					}
				}

				fwrite(&dw,4,1,f);
				fwrite(box,6*4,1,f);
				fwrite(&(dw=ea.ni),4,1,f);
				fwrite(&c,4,1,f);
			}
		}
		for(uint32_t pass=1;pass<=4;pass=pass<<1)
		{
			if(!sfc[pass/2]) continue;

			for(auto&ea:tris) if(ea.coplanar!=0xffff)
			{				
				dw = clipmode(cnmls[ea.ni].y);

				if(pass!=dw) continue;			

				auto &fi = X->mMeshes[ea.mi]->mFaces[ea.fi];

				if(!ea.coplanar) //ILLUSTRATING
				{
					for(i=fi.mNumIndices;i-->0;)
					{
						//fwrite(&(dw=ea.vi+fi.mIndices[i]),4,1,f);
						fwrite(&(dw=ea.cverts00+ea.cv[fi.mIndices[i]]),4,1,f);
					}
					continue; //!!
				}
			
				//DUPLICATE
				auto e = dgls[ea.coplanar]; 
				auto &fj = X->mMeshes[ea.mi]->mFaces[tris[e.tri].fi];
				e.a-=ea.vi; e.b-=ea.vi;
				uint32_t ii,jj;
				for(int pass=1;pass<=2;pass++)
				{
					auto &fx = pass==1?fi:fj;
					n = fx.mNumIndices;
					int a,b = fx.mIndices[n-1];
					for(i=0;i<n;i++,b=a)
					{
						a = fx.mIndices[i];
						if(a==e.a||a==e.b)
						if(b==e.a||b==e.b) break;
					}				
					(pass==1?ii:jj) = i; assert(i<n);
				}
				//SOM's winding is reverse from Assimp
				//for(i=0,n=fi.mNumIndices;i<n;i++) if(i==ii)
				for(i=fi.mNumIndices;i-->0;) if(i==ii)
				{
					auto nj = fj.mNumIndices;
					//for(j=jj+1;j<nj;j++)
					for(j=jj;j-->0;) 
					fwrite(&(dw=ea.cverts00+ea.cv[fj.mIndices[j]]),4,1,f);
					//for(j=0;j<jj;j++) 
					for(j=nj;j-->jj+1;)
					fwrite(&(dw=ea.cverts00+ea.cv[fj.mIndices[j]]),4,1,f);
				}
				else fwrite(&(dw=ea.cverts00+ea.cv[fi.mIndices[i]]),4,1,f);
			}
		}
	}

	if(g) //MSM //MSM //MSM //MSM //MSM //MSM //MSM //MSM
	{
		std::vector<uint16_t> subs[3]; 
		subs[0].reserve(tris.size()*6);
		for(auto&ea:tris) if(ea.coplanar!=0xffff)
		{
			auto sd = subs[0].size();
			void x2msm_subd(int,uint16_t*,decltype(subs+0),decltype(vrts)&,int=-1);

			auto *m = X->mMeshes[ea.mi];
			auto &fi = m->mFaces[ea.fi];

			subs[0].push_back((int16_t)texindex[m->mMaterialIndex]);

			if(!ea.coplanar) //ILLUSTRATING
			{
				n = fi.mNumIndices;
				subs[0].push_back((uint16_t)n);
				for(i=n;i-->0;)
				subs[0].push_back((uint16_t)(ea.vi+fi.mIndices[i]));
			}
			else //DUPLICATE
			{
				auto e = dgls[ea.coplanar]; 
				auto &fj = X->mMeshes[ea.mi]->mFaces[tris[e.tri].fi];
				e.a-=ea.vi; e.b-=ea.vi;
				uint32_t ii,jj;
				for(int pass=1;pass<=2;pass++)
				{
					auto &fx = pass==1?fi:fj;
					n = fx.mNumIndices;
					int a,b = fx.mIndices[n-1];
					for(i=0;i<n;i++,b=a)
					{
						a = fx.mIndices[i];
						if(a==e.a||a==e.b)
						if(b==e.a||b==e.b) break;
					}				
					(pass==1?ii:jj) = i; assert(i<n);
				}
				n = n-2+fi.mNumIndices; 
				subs[0].push_back((uint16_t)n);
				//SOM's winding is reverse from Assimp
				//for(i=0,n=fi.mNumIndices;i<n;i++) if(i==ii)
				for(i=fi.mNumIndices;i-->0;) if(i==ii)
				{
					auto nj = fj.mNumIndices;
					//for(j=jj+1;j<nj;j++)
					for(j=jj;j-->0;)
					subs[0].push_back((uint16_t)(ea.vi+fj.mIndices[j]));
					//for(j=0;j<jj;j++) 
					for(j=nj;j-->jj+1;)
					subs[0].push_back((uint16_t)(ea.vi+fj.mIndices[j]));
				}
				else subs[0].push_back((uint16_t)(ea.vi+fi.mIndices[i]));
			}

			subs[0].push_back(0); //...
			
			//HACK: this is a way to disable subdivision
			if(1!=x2msm_split2||x2msm_split2.cpoints[0].y)
			{
				//NOTE: I THINK TO MAKE THIS MORE SIMPLE SoftMSM
				//CAN BE RELIED ON TO MERGE DUPLICATION VERTICES
				if(1) x2msm_subd(2,&subs[0][sd],subs+1,vrts);
			}
		}
		
		auto w = (uint16_t)(vrts.size()/8);
		fwrite(&w,2,1,g);
		fwrite(vrts.data(),8*4*w,1,g);
		w = (uint16_t)(tris.size()-coplanar);
		fwrite(&w,2,1,g);
		if(subs[0].empty())
		{
			//this currently happens when there aren't any
			//textures in the model since it can't tell them
			//apart from CPs
			fwrite(&(w=0),2,1,g);
		}
		//can't work memory this way
		//fwrite(subs.data(),2*subs.size(),1,g);
		auto *p = subs[0].data();
		auto *q = subs[1].data();
		auto *r = subs[2].data();
		auto *s = p+subs->size(); while(p<s)
		{
			size_t c = p[1], d = p[2+c];

			fwrite(p,3+c,2,g); p+=3+c;

			while(d-->0) //level 2
			{
				while(*q==0xfffe) q++; //erasure?

				size_t c2 = q[1], d2 = q[2+c2]; //SHADOWING

				fwrite(q,3+c2,2,g); q+=3+c2;

				while(d2-->0) //level 3
				{
					while(*r==0xfffe) r++; //erasure?

					size_t c3 = r[1], d3 = r[2+c3]; //SHADOWING

					fwrite(r,3+c3,2,g); r+=3+c3; assert(d3==0);
				}
			}
		}
	}
	
	if(f) fclose(f);
	if(g) fclose(g); 
	
	//TODO: better to do in memory before fwrite
	extern int SoftMSM(const wchar_t *path, int mode=0);
	if(g) SoftMSM(name);

	wmemcpy(o,L".mdl",5); return true; //SoftMSM?
}
size_t x2msm_split_level::operator()(float yy, size_t &left, uint16_t *lp, float *ln, std::vector<float> &vrts)
{
	size_t l = left;

	//NOTE: if a polygon overlaps more than 1 section
	//I think it will get criss-cross cut. I'm unsure
	//how to approach this since precutting on the XZ
	//lines could be messy and potential overkill and
	//difficult where they converge around the origin

	//YUCK: it seems too messy to spread the polygon
	//over multiple "pieces of pie"
	aiVector2D avg;
	for(size_t i=l;i-->0;)
	{
		float *va = &vrts[lp[i]*8];
		avg.x+=va[0]; avg.y+=va[2];
	}
	avg/=(float)l;

	int pie = 0; //ARBITRARY
	for(int n=cpmax,j=n-1,i=0;i<n;j=i++)	
	if(avg*frustum[j]>=0.0f&&avg*frustum[i]<=0.0f) //dot product
	{
		pie = i; break;
	}

	//NOTE: DIAGONAL CUTS CAN CUT THROUGH FLAT
	//POLYGONS. MAYBE THEY SHOULD BE REJECTED?

	//find edge intersections?
	//no, just the plane height projections...
	for(size_t a=0;a<l;a++)
	{
		auto va = *(aiVector3D*)&vrts[lp[a]*8];
		va.y-=yy;
		ln[a] = yy+planes_y(pie,va);
	}
	#ifdef _DEBUG
	float &ln0 = ln[0]; //REMOVE US
	float &ln1 = ln[1];
	float &ln2 = ln[2];
	float &ln3 = ln[3];
	#endif

	//l_1: slicing a tip off a polygon makes a polygon with +1 sides
	size_t mask = 0, l_1 = l+1;
	auto *sp = lp+l_1;
	for(size_t i=l;i-->0;)
	{
		float *v = &vrts[lp[i]*8];

		float dd = v[1]-ln[i]; //dim

		size_t cmp = dd<-0.001f?1:dd>0.001f?2:0; //EXTENSION?

		mask|=cmp; sp[i] = (uint16_t)cmp;
	}
	if(mask!=3) return 0;

	size_t ll = 0, rr = 0;
	auto *lq = lp+l_1*2, *rq = lp+l_1*3; //lp should be 5*l_1 wide
	for(size_t b=l-1,a=0;a<l;b=a++)
	{
		if(sp[b]!=sp[a]&&sp[b]&&sp[a]) //lerp?
		{
			if(sp[b]==1) //1 or 2
			{
				lq[ll++] = lp[b];
			}
			else rq[rr++] = lp[b];

			//NOTE: relying on SoftMSM to merge and renormalize
			//(I've left a note in SoftMSM.cpp about normalizing
			//where I'm unsure what is correct. my instinct is to
			//not renormalize until after subdivision is complete)

			float *vb = &vrts[lp[b]*8];
			float *va = &vrts[lp[a]*8], vt[8];
			auto wa = *(aiVector3D*)va; wa.y-=yy;
			auto wb = *(aiVector3D*)vb; wb.y-=yy;
			float t = x2msm_split2.planes_t(pie,wa,wb);			
			for(int i=8;i-->0;)
			vt[i] = vb[i]+(va[i]-vb[i])*t; //lerp(b,a,t)
			
			lq[ll++] = rq[rr++] = (uint16_t)(vrts.size()/8);

			vrts.insert(vrts.end(),vt,vt+8);
		}
		else //on split or on same side?
		{
			if(sp[b]!=2) lq[ll++] = lp[b]; //1 or 0
			if(sp[b]!=1) rq[rr++] = lp[b]; //2 or 0
		}
	}

	memcpy(lp,lq,ll*2);
	memcpy(lp+ll,rq,rr*2); left = ll; return rr;
}
size_t x2msm_split(int dim, float ln, size_t &left, uint16_t *lp, std::vector<float> &vrts)
{
	//l_1: slicing a tip off a polygon makes a polygon with +1 sides
	size_t l = left, mask = 0, l_1 = l+1;
	auto *sp = lp+l_1;
	for(size_t i=l;i-->0;)
	{
		float *v = &vrts[lp[i]*8];

		float dd = v[dim]-ln;

		size_t cmp = dd<-0.001f?1:dd>0.001f?2:0; //EXTENSION?

		mask|=cmp; sp[i] = (uint16_t)cmp;
	}
	if(mask!=3) return 0;

	size_t ll = 0, rr = 0;
	auto *lq = lp+l_1*2, *rq = lp+l_1*3; //lp should be 5*l_1 wide
	for(size_t b=l-1,a=0;a<l;b=a++)
	{
		if(sp[b]!=sp[a]&&sp[b]&&sp[a]) //lerp?
		{
			if(sp[b]==1) //1 or 2
			{
				lq[ll++] = lp[b];
			}
			else rq[rr++] = lp[b];

			//NOTE: relying on SoftMSM to merge and renormalize
			//(I've left a note in SoftMSM.cpp about normalizing
			//where I'm unsure what is correct. my instinct is to
			//not renormalize until after subdivision is complete)

			float *vb = &vrts[lp[b]*8];
			float *va = &vrts[lp[a]*8], vt[8];
			float t = fabsf((ln-vb[dim])/(vb[dim]-va[dim]));
			for(int i=8;i-->0;)
			vt[i] = vb[i]+(va[i]-vb[i])*t; //lerp(b,a,t)
			
			lq[ll++] = rq[rr++] = (uint16_t)(vrts.size()/8);

			vrts.insert(vrts.end(),vt,vt+8);
		}
		else //on split or on same side?
		{
			if(sp[b]!=2) lq[ll++] = lp[b]; //1 or 0
			if(sp[b]!=1) rq[rr++] = lp[b]; //2 or 0
		}
	}

	memcpy(lp,lq,ll*2);
	memcpy(lp+ll,rq,rr*2); left = ll; return rr;
}
void x2msm_subd(int m, uint16_t *poly, std::vector<uint16_t> *lv, std::vector<float> &vrts, int subst)
{
	size_t right,left = poly[1]; auto &st = *lv;

	//REMOVE ME
	//this has 4 sections, 2 for x2msm_split's output
	//-1/+1 is for when a corner is cut off resulting
	//in an n+1 polygon
	//2022: I'm bumping this from +1 to 2x since I can't
	//see how a well placed diamond wouldn't have all of
	//its corners cut off to form an octagon for example
	uint16_t _buf[2*8*4], *buf = _buf;
	float _buf2[2*8], *buf2 = _buf2; //x2msm_split2
	//TEMPORARY MEMORY
	//if(left>8-1)
	if(left>8)
	{			
		//REMINDER: these are deleted at the very end		
		//buf = new uint16_t[(left+1)*4]; //recursive
		//NOTE: this one doesn't have to be recursive
		if(x2msm_split2>1) 
		buf2 = new float[2*left];
		buf = new uint16_t[(2*left)*4];
	}

	//ALGORITHM
	//based on SOM_MAP's drawing code and selectively
	//isolating individual levels it can be concluded
	//there are at most 2 levels, and level #1 may be
	//like level #2 if only 1 level of subdivision is
	//required. level #1 is cut on a 2m grid unless a
	//triangle is smaller than 2m. level #2 is cut on
	//a 1m grid unless level #1 is a 1m cut because a
	//triangle is smaller than 2m (note, I don't know
	//what MapComp does, so I can't say why 2 levels)
	//MORE INFO
	//the 2m level is only on the vertical axis going
	//up from 0 to 2 to 4, etc. it should go negative
	//as well but doesn't/didn't. i.e. x2msm is buggy

	auto &subdivs = poly[2+left], before = subdivs;

	bool top = 0==subdivs;

	auto *p = poly; if(subst!=-1)
	{
		p = &st[subst]; left = p[1];
	}
	else subst = (int)st.size();

	float box[2][3]; for(int j=3;j-->0;)
	{
		box[0][j] = +FLT_MAX;
		box[1][j] = -FLT_MAX;
	}
	auto *ip = p+2; for(auto i=left;i-->0;)
	{
		buf[i] = ip[i]; //PIGGYBACKING

		float *v = &vrts[8*ip[i]];

		for(int j=3;j-->0;)
		{
			if(v[j]<box[0][j]) box[0][j] = v[j];
			if(v[j]>box[1][j]) box[1][j] = v[j];
		}
	}

	single_level: //quasi-recursion (save box)

	//COMPLICATED
	//this is extending from x2mdl so that the
	//first level splits 0 on the vertical and
	//starts at 1/-1 laterally so a 2m cube is
	//sat and split at elevation 0
	int xz = m==2, y = !xz;

	//NOTE: x2msm did no cover this much range
	//on 2m split it only split up from 2, not
	//splitting on 0 or lower. for 1m split it
	//split odd meters on vertical and only at
	//0,0 on lateral (i.e. not 1, nor 2, etc.)		
	int lb[3] = {xz,y,xz}, ub[3] = {-xz,-y,-xz};
	for(int i=3;i-->0;)
	{
		while(box[0][i]>lb[i]) lb[i]+=2;
		while(box[1][i]<ub[i]) ub[i]-=2;

		while(box[0][i]<lb[i]-2) lb[i]-=2;
		while(box[1][i]>ub[i]+2) ub[i]+=2;
	}	
	if(x2msm_split2) //sloping cuts? or elevation shift
	{
		//HACK: extend range, assuming 2m is sufficient
		lb[1]-=2; ub[1]+=2;
	}

	for(int pass=1;pass<=3;pass++)
	{
		//is order important? y first
		int dim = pass-1;
		if(dim!=2) dim = !dim;

		int l = lb[dim], u = ub[dim];
		for(int i=l;i<=u;i+=2)
		{
			if(i==0) //DEBUGGING
			{
				if(m==2) assert(dim==1);
				if(m==1) assert(dim!=1);
			}
			else //SAME
			{
				if(m==2) assert((bool)(i%2)==(dim!=1)); //-i
				if(m==1) assert((bool)(i%2)==(dim==1)); //-i
			}
						
			float ln = (float)i;

			switch(dim==1?x2msm_split2:0)
			{				
			default:

				right = x2msm_split2(ln,left,buf,buf2,vrts);
				break;

			case 1: ln+=x2msm_split2.cpoints[0].y;
			case 0: right = x2msm_split(dim,ln,left,buf,vrts);
			}
			
			if(right) //right (chipping off)
			{
				subdivs++;
			
				int subst2 = (int)st.size();

				st.push_back(poly[0]); //texture
				st.push_back((uint16_t)right);
				st.insert(st.end(),buf+left,buf+left+right);
				st.push_back(0);						
				x2msm_subd(m,poly,lv,vrts,subst2); //RECURSIVE
			}
		}
	}	
	if(before<subdivs) //left (remaining piece)
	{
		if(top) //can append to the back
		{
			subdivs++;

			st.push_back(poly[0]); //texture
			st.push_back(left);
			st.insert(st.end(),buf,buf+left);
			st.push_back(0);

			assert(p==poly);
		}
		else //seems must edit in place?
		{
			assert(p!=poly);

			p = &st[subst]; ip = p+2; //revalidate? 
			
			assert(ip[p[1]]==0); //no subdivs?

			int cmp = (int)left-(int)p[1];

			//I THINK THEORETICALLY THERE CAN ONLY BE 1 SIDE ADDED
			//OR REMOVED AT A TIME BY x2msm_split, HOWEVER I THINK
			//IT MAY BE POSSIBLE TO RUN MULTIPLE TIMES... IN WHICH
			//CASE _buf MAY BE AT ISSUE
			//
			// NOTE: I don't usually run x2mdl.dll in debug mode
			// unless I'm working on it and forget to rebuild it
			// 
		//	assert(cmp>=-1&&cmp<=2); //ii0044.msm has 2 //1 
			assert(cmp>=-2&&cmp<=2); //ii0026.msm has -2 //-1 

			if(cmp>0) //n polygon to n+1 polygon? n+2 polygon?
			{
				//NOTE: I think x2msm might have injected a polygon
				//here based on what I saw that made me to think of
				//this as a scenario
				
				int j,rem = (int)st.size()-subst;

				if(1&&cmp==1) //OPTIMIZING?
				{
					for(j=0;j<rem;j++) if(p[j]==0xFFFe) //erasure?
					{
						rem = j; //memmove upto erasure?
					}
					if(j==rem) st.push_back(0); 
				}
				else st.insert(st.end(),cmp,0);

				p = &st[subst]; ip = p+2; //revalidate? 

				j = 3+p[1]; memmove(p+j+cmp,p+j,2*(rem-j));
			}
			else while(cmp<0) //erasures?
			{
				ip[left-cmp++] = 0xFFFE; //erasure? 				
			}
			
			p[1] = (uint16_t)left;

			for(auto i=left;i-->0;) ip[i] = buf[i];

			ip[left] = 0; //no subdivs?
		}
	}

	if(top) if(--m==1) if(0==subdivs) 
	{
		goto single_level; //OPTIMIZING
	}
	else //second-level?
	{
		uint16_t *p = &st[subst];

		for(int i=subdivs;i-->0;p+=3+p[1])
		{
			while(*p==0xfffe) p++; //erasure?

			x2msm_subd(1,p,lv+1,vrts,-1);
		}		
	}

	if(buf!=_buf)
	{
		if(x2msm_split2>1) 
		delete[] buf2; 
		delete[] buf;		
	}
}

aiNode *x2mdo_split_XY_recursive(uint64_t xy, aiNode *cp, unsigned *mm)
{
	aiNode *o = new aiNode(*cp);
	
	unsigned i,j,k,n; 
	o->mChildren = new aiNode*[cp->mNumChildren];	
	for(i=cp->mNumChildren;i-->0;)
	{
		o->mChildren[i] = x2mdo_split_XY_recursive(xy,cp->mChildren[i],mm);
		o->mChildren[i]->mParent = o;
	}
	o->mMeshes = new unsigned[cp->mNumMeshes];
	for(i=j=k=0,n=cp->mNumMeshes;i<n;i++)
	{
		auto *m = X->mMeshes[cp->mMeshes[i]];
		if(xy&(1ull<<(uint64_t)m->mMaterialIndex))
		{
			o->mMeshes[j++] = mm[cp->mMeshes[i]];
		}
		else cp->mMeshes[k++] = mm[cp->mMeshes[i]];
	}
	o->mNumMeshes = j; cp->mNumMeshes = k; return o; 
}
void x2mdo_split_XY()
{
	//2022: this algorithm splits off MHM models
	//so x2msm can be used to add MHM to MDO/MDL

	uint64_t xy = 0;

	unsigned i,j,k,n; 
	for(i=j=0;i<X->mNumMaterials;i++)
	{
		auto *m = X->mMaterials[i];

		if(m->GetTextureCount(aiTextureType_DIFFUSE))
		continue;

		aiColor4D diffuse(1,1,1,1); //RGBA
		m->Get(AI_MATKEY_COLOR_DIFFUSE,diffuse);			
		float a = 1;
		m->Get(AI_MATKEY_OPACITY,a);
		diffuse.a*=a;

		if(diffuse.a>=1.0f) continue;

		j++; xy|=1ull<<(uint64_t)i;
	}
	if(!j||i==j||i>64) return; //ull is 64 bit

	for(i=j=0;i<X->mNumMeshes;i++)
	{
		auto *m = X->mMeshes[i];
		if(xy&(1ull<<(uint64_t)m->mMaterialIndex))
		j++;
	}
	if(!j||i==j) return;

	//NOW IT SEEMS SPLIT MAY BE NEEDED//

	aiScene *x = const_cast<aiScene*>(X);
	aiScene *y = new aiScene;

	auto *mm = new unsigned[x->mNumMeshes];	
	y->mMeshes = new aiMesh*[x->mNumMeshes];
	for(i=j=k=0,n=x->mNumMeshes;i<n;i++)
	{
		if(xy&(1ull<<(uint64_t)i))
		{
			y->mMeshes[mm[i]=j++] = x->mMeshes[i];
		}
		else x->mMeshes[mm[i]=k++] = x->mMeshes[i];
	}
	y->mNumMeshes = j; x->mNumMeshes = k;
	y->mRootNode = x2mdo_split_XY_recursive(xy,x->mRootNode,mm);
	delete[] mm;

	//NOTE: I don't see an API for duplicating aiMaterial
	//so it's left to x2mdl.cpp to set X/Y->mNumMaterials
	y->mMaterials = x->mMaterials; 
	y->mNumMaterials = x->mNumMaterials; X = y; Y = x;
}