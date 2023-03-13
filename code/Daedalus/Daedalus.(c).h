
#ifndef DAEDALUS_PCH_INCLUDED
#define DAEDALUS_PCH_INCLUDED

//THIS is the precompiled header file
//but it is renamed Daedalus.(c).h to 
//address any copyright/license terms
//The authors are of the opinion such
//terms are in truth merely talismans
//So we don't want them in every file
//We ask that you use good judgement!
//
//Assimp's modifed BSD license print:
/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------
Copyright (c) 2006-2015, assimp team
All rights reserved.
Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the following
conditions are met:
* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.
* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.
* Neither the name of the assimp team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the assimp team.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
---------------------------------------------------------------------------
*/
//Collada-DOM print, viewer-included:
/*
* Copyright 2006 Sony Computer Entertainment Inc.
*
* Licensed under the MIT Open Source License, for details please see license.txt or the website
* http://www.opensource.org/licenses/mit-license.php
*
*/
//Daedalus' code is effectively in
//the public domain. However we do
//ask that you not impersonate our
//work; like a classical copyright
//(in the court of public opinion)

#ifdef _WIN32
#define _WIN32_WINNT 0x0500 //AttachConsole
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN		
#include <windows.h> 
#include <Shellapi.h> //CommandLineToArgvW
#endif

#include "Daedalus.h" 
namespace Daedalus
{
	#ifdef _DEBUG
	#define preSize size_t
	#endif
	#include "PreServe.h"
	#include "PreServe.inl"
}

#include <array>
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace Daedalus //post.h
{
	#include "post.hpp"
	#include "post-MakeSubmeshHelper.hpp"
	#include "post-MergeMeshesHelper.hpp"
}

#define COLLADA_DOM_LITE
//#define __library_effects_type_h__http_www_collada_org_2008_03_COLLADASchema__ColladaDOM_g1__
#define __profile_bridge_type_h__http_www_collada_org_2008_03_COLLADASchema__ColladaDOM_g1__
#define __profile_gles2_type_h__http_www_collada_org_2008_03_COLLADASchema__ColladaDOM_g1__
#define __profile_glsl_type_h__http_www_collada_org_2008_03_COLLADASchema__ColladaDOM_g1__
#define __profile_cg_type_h__http_www_collada_org_2008_03_COLLADASchema__ColladaDOM_g1__
#define __profile_gles_type_h__http_www_collada_org_2008_03_COLLADASchema__ColladaDOM_g1__
//..
#define __library_force_fields_type_h__http_www_collada_org_2008_03_COLLADASchema__ColladaDOM_g1__
#define __library_physics_materials_type_h__http_www_collada_org_2008_03_COLLADASchema__ColladaDOM_g1__
#define __library_physics_models_type_h__http_www_collada_org_2008_03_COLLADASchema__ColladaDOM_g1__
#define __library_physics_scenes_type_h__http_www_collada_org_2008_03_COLLADASchema__ColladaDOM_g1__
#define __library_joints_type_h__http_www_collada_org_2008_03_COLLADASchema__ColladaDOM_g1__
#define __library_kinematics_models_type_h__http_www_collada_org_2008_03_COLLADASchema__ColladaDOM_g1__
#define __library_articulated_systems_type_h__http_www_collada_org_2008_03_COLLADASchema__ColladaDOM_g1__
#define __library_kinematics_scenes_type_h__http_www_collada_org_2008_03_COLLADASchema__ColladaDOM_g1__
#define __library_formulas_type_h__http_www_collada_org_2008_03_COLLADASchema__ColladaDOM_g1__
#define __instance_kinematics_scene_type_h__http_www_collada_org_2008_03_COLLADASchema__ColladaDOM_g1__
#include "Collada.pch.cpp"

namespace Daedalus
{	
	using namespace COLLADA;
	extern daeDOM CollaDB; //singleton?
	extern bool CollaDAPP(daeElementRef,const preScene*);
}

#endif //DAEDALUS_PCH_INCLUDED