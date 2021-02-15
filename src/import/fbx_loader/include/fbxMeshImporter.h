/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: fbx #]
***/

#pragma once

#include "base/resource/include/resource.h"
#include "base/resource_compiler/include/importInterface.h"
#include "assets/mesh_loader/include/renderingMeshImportConfig.h"

namespace fbx
{

    class LoadedFile;
    struct SkeletonBuilder;
    struct MaterialMapper;

    //--

    /// extension of import configuration specific for FBX meshes
    class IMPORT_FBX_LOADER_API FBXMeshImportConfig : public rendering::MeshImportConfig
    {
        RTTI_DECLARE_VIRTUAL_CLASS(FBXMeshImportConfig, rendering::MeshImportConfig);

    public:
        FBXMeshImportConfig();

        // geometry stuff
        bool m_alignToPivot = false;
        bool m_flipUV = true;
        bool m_forceNodeSkin = false;
        bool m_createNodeMaterials = false;

        // base material template to use when importing materials
        base::res::AsyncRef<rendering::IMaterial> m_baseMaterialTemplate;
    };

    //--

    /// importer of FBX meshes
    class IMPORT_FBX_LOADER_API MeshImporter : public base::res::IResourceImporter
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshImporter, base::res::IResourceImporter);

    public:
        MeshImporter();

        virtual base::res::ResourcePtr importResource(base::res::IResourceImporterInterface& importer) const override final;
    };

    //--

} // fbx

