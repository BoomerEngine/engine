/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: import #]
***/

#pragma once

#include "core/resource_compiler/include/importInterface.h"
#include "import/mesh_loader/include/renderingMeshImportConfig.h"

BEGIN_BOOMER_NAMESPACE_EX(assets)

//--

/// attribute filter - should we output given attribute as mesh data stream or not
enum class OBJMeshAttributeMode : uint8_t
{
    Always, // attribute will be always emitted, even if it contains no data (does not make sense but zeros compress well, so..)
    IfPresentAnywhere, // attribute will be emitted to a mesh if at least one input group from .obj contains it
    IfPresentEverywhere, // attribute will be emitted to a mesh ONLY if ALL groups in a build group contain it (to avoid uninitialized data)
    Never, // attribute will never be emitted
};

//--

/// manifest specific for importing Wavefront meshes
class IMPORT_OBJ_LOADER_API OBJMeshImportConfig : public MeshImportConfig
{
    RTTI_DECLARE_VIRTUAL_CLASS(OBJMeshImportConfig, MeshImportConfig);

public:
    OBJMeshImportConfig();

    StringBuf objectFilter;
    StringBuf groupFilter;

    OBJMeshAttributeMode emitNormals = OBJMeshAttributeMode::IfPresentAnywhere;
    OBJMeshAttributeMode emitUVs = OBJMeshAttributeMode::IfPresentAnywhere;
    OBJMeshAttributeMode emitColors = OBJMeshAttributeMode::IfPresentAnywhere;

    bool forceTriangles = false;
    bool allowThreads = true;

    bool flipUV = true;

    virtual void computeConfigurationKey(CRC64& crc) const override;
};

//--

struct GroupBuildModelList;

/// mesh cooker for OBJ files
class IMPORT_OBJ_LOADER_API OBJMeshImporter : public IGeneralMeshImporter
{
    RTTI_DECLARE_VIRTUAL_CLASS(OBJMeshImporter, IGeneralMeshImporter);

public:
    OBJMeshImporter();

    virtual ResourcePtr importResource(IResourceImporterInterface& importer) const override final;

private:        
    virtual RefPtr<MaterialImportConfig> createMaterialImportConfig(const MeshImportConfig& cfg, StringView name) const override final;

    void buildMaterials(const SourceAssetOBJ& data, const Mesh* existingMesh, IResourceImporterInterface& importer, const GroupBuildModelList& exportGeometry, Array<int>& outSourceToExportMaterialIndexMapping, Array<MeshMaterial>& outExportMaterials) const;
};

//--

END_BOOMER_NAMESPACE_EX(assets)
