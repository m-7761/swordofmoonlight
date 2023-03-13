#include "Daedalus.(c).h"  
using namespace Daedalus;
class TextureTransform //Assimp/TextureTransform.cpp
{	
	Daedalus::post &post; 

	preScene *const scene;

	struct Channel //AI: property or insertion parameters
	{
		void ChangeTo(preN n) //to reuse or not to reuse?
		{	
			if(sourceslist) sourceslist->Datum(addto->sourceslist) = n;
			else addto->InsertProperty(n,addto->sourceslist,texture,textureN);						
		}		
		PreMaterial::Property *sourceslist; 
		preMaterial *addto; PreMaterial::Texture texture; preN textureN;
		Channel(preMaterial *m, PreMaterial::Property *p):
		sourceslist(),addto(m),texture(p->TextureCategory()),textureN(p->TextureNumber()){}
	};
	struct Transform //texture matrix with some accessories
	{
		//AI: materials that are referencing this transform
		std::vector<Channel> channelsList;

		preMatrix matrix; PreSet<bool,true> identity; //NEW
		inline void SetMatrixAndIdentityFlag(preMatrix &m){ matrix = m; identity = matrix.IsIdentity(); }

		preN sourceChannel, targetChannel; Transform():sourceChannel(),targetChannel(preMost){}		
	};	
	//phase one: fill in the per material lists
	typedef std::list<Transform> TransformsList;
	std::vector<TransformsList> listsList; bool FillTransformsLists() 
	{	//NOTE: Assimp's original code incorporated an inner loop that
		//took meshes into consideration, but here it's understood that
		//if meshes' materials are the same, their texcoords are as well
		auto il = scene->MaterialsList(); for(preN i=il;i<il.Size();i++)
		{	
			preMaterial *mat = il[i];
			auto &forms = listsList[i];
			auto l = mat->PropertiesSubList(0);	//per texture ranges
			for(preN j=0;l.Size();l=mat->PropertiesSubList(j+=l.Size()))
			{	//AI: Setup shortcut structure for updating later on
				Channel chan(mat,l.Front()); 
				if(!chan.texture) continue; //leading non-texture block									
				//AI: Get texture props and transform
				//todo? might look better to do a binsearch here
				Transform form; l^[&](PreMaterial::Property *ea)
				{	//deletion saved for last
					if(ea->key==mat->_matrix) 
					form.SetMatrixAndIdentityFlag(ea->Datum(mat->matrix));
					//AI: save the property to use later
					if(ea->key==mat->_sourceslist) 					 
					chan.sourceslist = ea;
				};if(chan.sourceslist) //AI: default to 0
				form.sourceChannel = chan.sourceslist->Datum(mat->sourceslist);					
				//HERE Assimp augments the Z-rotation/UV-translation according to 
				//the wrap settings (when un-rotated) and normalizes the rotation
				//This seems like it is deletorious, and not necessarily complete
				//AND BESIDE, THE MODULE/LOADER IS IN A MUCH BETTER POSITION HERE
				//PreProcessUVTransform(form); //don't forget about form.identity												
				TransformsList::iterator it; 
				for(it=forms.begin();it!=forms.end();it++)					
				if(form.sourceChannel==it->sourceChannel)
				if(!it->matrix.EpsilonCompare(form.matrix,post.TextureTransform.configMatrixEpsilon))   
				{
					it->channelsList.push_back(chan); break; 
				}
				if(it==forms.end())
				{
					forms.push_back(form);
					forms.back().channelsList.push_back(chan);
				}
			}			
			//NEW: assign channels, first come-first serve
			//ASSUMING GenUVCoords is forced on, so 1/2/3D
			enum{ n=PreMesh::texturecoordslistsN };			
			bool taken[n] = {}; //default initialized
			for(int pass=1;pass<=2;pass++)
			for(auto it=forms.begin();it!=forms.end();it++)
			{   //give 1st round to identity matrices
				if(it->identity!=(pass==1)) continue;
				if(!taken[it->sourceChannel])
				it->targetChannel = it->sourceChannel;
				else for(preN i=0;i<n;i++)
				if(!taken[i]){ it->targetChannel = i; break; }
				if(it->targetChannel>n-1)
				{
				post.CriticalIssue(n)<<" is fewer texture channels than required.";
				post.CriticalIssue("Aborting this step as arbitrarily high numbers of channels are not yet supported.");
				preX name; mat->Get(mat->name,name);
				post.CriticalIssue("Material name was ")<<name.String("not-given")<<", its number is "<<i<<".";
				return false;
				}
				else taken[it->targetChannel] = true; 
			}
		}//update the materials lastly
		for(preN i=il;i<il.Size();i++)
		{
			preMaterial *mat = il[i];			
			il[i]->RemoveEachPropertyIf([](const PreMaterial::Property *ea)
			{ return ea->key==PreMaterial::_matrix; });			
			auto &forms = listsList[i];
			for(auto it=forms.begin();it!=forms.end();it++)
			{
				auto &cl = it->channelsList;
				for(auto jt=cl.begin();jt!=cl.end();jt++) jt->ChangeTo(it->targetChannel);
			}//AND ALSO: sort so meshes can allocate new channels in order
			forms.sort([](Transform &a, Transform &b){ return a.targetChannel<b.targetChannel; });
		}		
		return true;
	}

public: //TextureTransform

	TextureTransform(Daedalus::post *p)
	:scene(post.Scene()),listsList(scene->Materials())
	,post(*p){}operator bool()
	{	
		post.progLocalFileName("Removing Static Texture Matrices");
				
		post.Verbose("TransformUVCoordsProcess begin");
												
		if(!FillTransformsLists()) //phase one
		{
			post.Verbose("TransformUVCoordsProcess aborted");
			return true; //aborted over texture channel limit
		}
		
		preN logPrior = 0, logAfter = 0, logTransformed = 0;

		//phase two: now just more or less transforming UVW coordinates
		auto il = scene->MeshesCoList(); for(preN i=il;i<il.Size();i++)
		{	
			//determine if processing is required
			preMesh *mesh = scene->mesheslist[i];
			const preN channels = mesh->CountTextureCoordsLists(); if(!channels) continue; 
			logPrior+=channels; 
			TransformsList::iterator it, it2;			
			auto &forms = listsList[mesh->material];
			for(it=forms.begin();it!=forms.end();it++){ if(!it->identity) goto no_continue; }		
			logAfter+=channels; continue; //trivial
			no_continue: ////POINT-OF-NO-RETURN////	
			
			PreSuppose(!PreMesh2::IsSupported); //todo? packed attribute model

		//FYI: there was a heck of a lot of code in this space in the original Assimp version//
			
			auto ml = mesh->MorphsList();
			int mN = ml.RealSize(); for(int m=-1;m<mN;m++) //NEW
			{
				PreMorph *morph = m==-1?mesh:ml[m];
				int same = mesh==morph; 
				//TODO: figure out a way to use TextureCoordsCoList
				const pre3D *sources[PreMesh::texturecoordslistsN];				
				for(preN i=0;i<channels;i++) sources[i] = morph->texturecoordslists[i];
				//AI: Now continue and generate the output channels				
				for(it=forms.begin();it!=forms.end();it++)
				{
					//exclude morphs that do not override the source
					if(!same&&!sources[it->sourceChannel]) continue;
					if(same) logAfter++;

					preN i = it->targetChannel; //i is legacy
					auto dst = morph->TextureCoordsCoList(i);										
					if(dst.HasList()) //NOTICE! dst.Reserve() WILL WIPEOUT sources!!
					{
						it2 = it; it2++; 
						while(it2!=forms.end()) if(i==it2++->sourceChannel)
						{ 	//fyi: means a later transform needs to read from sources
							//so the source buffer cannot be reused as the destination
							morph->texturecoordslists[i] = new pre3D[mesh->TextureCoords()];
							break; 
						}
					}//Reminder: relying on zero-initialization in case src was empty
					else morph->texturecoordslists[i] = new pre3D[mesh->TextureCoords()];
					const pre3D *src = sources[it->sourceChannel]; if(src) //paranoia
					{
						if(!it->identity) //AI: transform all UVW coords
						{ dst^[&](pre3D &ea){ ea = it->matrix**src++; }; logTransformed+=same; }
						else dst^[&](pre3D &ea){ ea = *src++; };
					}
					else assert(0); //not sure what this could indicate
				}//NEW: THIS LOOKS like a memory-leak in Assimp's original code???
				for(preN i=0;i<channels;i++) 
				if(sources[i]!=morph->texturecoordslists[i]) delete[] sources[i];
			}
		}	  		

		if(logTransformed)    		
		post("TransformUVCoordsProcess finished: ")<<logAfter
		<<" output channels (in: "<<logPrior<<", modified: "<<logTransformed<<")";
		else post.Verbose("TransformUVCoordsProcess finished");
		return true;
	}
};
static bool TextureTransform(Daedalus::post *post)
{
	return class TextureTransform(post);
}							
Daedalus::post::TransformUVCoords::TransformUVCoords
(post *p):Base(p,p->steps&step?::TextureTransform:0)
{
	configMatrixEpsilon = 0;
}