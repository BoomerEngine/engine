/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: fbx #]
***/

#pragma once

#include "core/resource/include/resource.h"
#include "core/resource_compiler/include/importInterface.h"
#include "import/mesh_loader/include/renderingMeshImportConfig.h"

BEGIN_BOOMER_NAMESPACE_EX(assets)

class FBXFile;
struct FBXSkeletonBuilder;
struct FBXMaterialMapper;

//--

/// extension of import configuration specific for FBX meshes
class IMPORT_FBX_LOADER_API FBXMeshImportConfig : public MeshImportConfig
{
    RTTI_DECLARE_VIRTUAL_CLASS(FBXMeshImportConfig, MeshImportConfig);

public:
    FBXMeshImportConfig();

    // geometry stuff
    bool m_importAtRootSpace = false;
    bool m_flipUV = true;
    bool m_forceNodeSkin = false;
    bool m_createNodeMaterials = false;
    Vector2 m_uvScale = Vector2(1, 1);

    // base material template to use when importing materials
    ResourceAsyncRef<IMaterial> m_baseMaterialTemplate;
};

//--

/// importer of FBX meshes
class IMPORT_FBX_LOADER_API MeshImporter : public IGeneralMeshImporter
{
    RTTI_DECLARE_VIRTUAL_CLASS(MeshImporter, IGeneralMeshImporter);

public:
    MeshImporter();

    virtual ResourcePtr importResource(IResourceImporterInterface& importer) const override final;

protected:
    virtual RefPtr<MaterialImportConfig> createMaterialImportConfig(const MeshImportConfig& cfg, StringView name) const override final;
};

//--

END_BOOMER_NAMESPACE_EX(assets)

