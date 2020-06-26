/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: fbx #]
***/

#pragma once

#include "base/resources/include/resource.h"
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

        bool m_alignToPivot;
    };

    //--

    /// manifest for importing meshes from FBX file
    class ASSETS_FBX_LOADER_API MeshCooker : public base::res::IResourceCooker
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshCooker, base::res::IResourceCooker);

    public:
        MeshCooker();

        //--

        /// bake a final (cooked) resource using this manifest and provided extra stuff
        virtual base::res::ResourcePtr cook(base::res::IResourceCookerInterface& cooker) const override final;

        //--

        /// build geometry blob from parsed data
        //bool buildChunkGroups(base::res::IResourceCookerInterface& cooker, const LoadedFile& sourceGeometry, base::Array<rendering::content::GeometryChunkGroupPtr>& outGroups, SkeletonBuilder& outSkeleton, MaterialMapper& outMaterials) const;

        /// build material data
        //bool buildMaterialData(base::res::IResourceCookerInterface& cooker, const LoadedFile& sourceGeometry, const MaterialMapper& materials, base::Array<rendering::content::MeshMaterial>& outMaterials) const;

        //--
    };

    //--

} // fbx

