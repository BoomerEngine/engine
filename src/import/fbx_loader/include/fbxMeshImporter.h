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
#include "import/mesh_loader/include/renderingMeshImportConfig.h"

BEGIN_BOOMER_NAMESPACE(asset)

class FBXFile;
struct FBXSkeletonBuilder;
struct FBXMaterialMapper;

//--

/// extension of import configuration specific for FBX meshes
class IMPORT_FBX_LOADER_API FBXMeshImportConfig : public rendering::MeshImportConfig
{
    RTTI_DECLARE_VIRTUAL_CLASS(FBXMeshImportConfig, rendering::MeshImportConfig);

public:
    FBXMeshImportConfig();

    // geometry stuff
    bool m_applyNodeTransform = false;
    bool m_flipUV = true;
    bool m_forceNodeSkin = false;
    bool m_createNodeMaterials = false;

    // base material template to use when importing materials
    base::res::AsyncRef<rendering::IMaterial> m_baseMaterialTemplate;
};

//--

/// importer of FBX meshes
class IMPORT_FBX_LOADER_API MeshImporter : public rendering::IGeneralMeshImporter
{
    RTTI_DECLARE_VIRTUAL_CLASS(MeshImporter, rendering::IGeneralMeshImporter);

public:
    MeshImporter();

    virtual base::res::ResourcePtr importResource(base::res::IResourceImporterInterface& importer) const override final;

protected:
    virtual base::RefPtr<rendering::MaterialImportConfig> createMaterialImportConfig(const rendering::MeshImportConfig& cfg, base::StringView name) const override final;
};

//--

END_BOOMER_NAMESPACE(asset)

