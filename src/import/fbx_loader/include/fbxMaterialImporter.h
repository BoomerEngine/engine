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

//--

/// manifest specific for importing Wavefront materials
class IMPORT_FBX_LOADER_API FBXMaterialImportConfig : public MaterialImportConfig
{
    RTTI_DECLARE_VIRTUAL_CLASS(FBXMaterialImportConfig, MaterialImportConfig);

public:
    FBXMaterialImportConfig();

    StringBuf m_materialName; // name of the material to import from MTL file

    StringBuf m_bindingColor;

    StringBuf m_bindingMapColor;
    StringBuf m_bindingMapBump;
    StringBuf m_bindingMapNormal;
    StringBuf m_bindingMapDissolve;
    StringBuf m_bindingMapSpecular;
    StringBuf m_bindingMapEmissive;
    StringBuf m_bindingMapRoughness;
    StringBuf m_bindingMapRoughnessSpecularity;
    StringBuf m_bindingMapMetallic;
    StringBuf m_bindingMapAmbientOcclusion;

    virtual void computeConfigurationKey(CRC64& crc) const override;
};

//--

/// importer of FBX meshes
class IMPORT_FBX_LOADER_API MaterialImporter : public IGeneralMaterialImporter
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialImporter, IGeneralMaterialImporter);

public:
    MaterialImporter();

    virtual res::ResourcePtr importResource(res::IResourceImporterInterface& importer) const override final;
};

//--

END_BOOMER_NAMESPACE_EX(assets)



