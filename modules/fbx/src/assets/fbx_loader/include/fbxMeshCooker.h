/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: fbx #]
***/

#pragma once

#include "base/resource/include/resource.h"
#include "base/geometry/include/meshManifest.h"

namespace fbx
{

    class LoadedFile;
    struct SkeletonBuilder;
    struct MaterialMapper;

    //--

    /// manifest for importing meshes from FBX file
    class ASSETS_FBX_LOADER_API MeshManifest : public base::mesh::MeshManifest
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshManifest, base::mesh::MeshManifest);

    public:
        MeshManifest();

        bool m_alignToPivot = false;
        bool m_flipUV = true;
        bool m_forceNodeSkin = false;
        bool m_createNodeMaterials = false;
    };

    //--

    /// manifest for importing meshes from FBX file
    class ASSETS_FBX_LOADER_API MeshCooker : public base::res::IResourceCooker
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshCooker, base::res::IResourceCooker);

    public:
        MeshCooker();

        virtual base::res::ResourcePtr cook(base::res::IResourceCookerInterface& cooker) const override final;
        virtual void reportManifestClasses(base::Array<base::SpecificClassType<base::res::IResourceManifest>>& outManifestClasses) const override final;
    };

    //--

} // fbx

