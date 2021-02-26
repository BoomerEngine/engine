/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: import #]
***/

#pragma once

#include "core/resource_compiler/include/importInterface.h"
#include "import/mesh_loader/include/renderingMaterialImportConfig.h"

BEGIN_BOOMER_NAMESPACE_EX(assets)

//--

/// manifest specific for importing Wavefront materials
class IMPORT_OBJ_LOADER_API MTLMaterialImportConfig : public MaterialImportConfig
{
    RTTI_DECLARE_VIRTUAL_CLASS(MTLMaterialImportConfig, MaterialImportConfig);

public:
    MTLMaterialImportConfig();

    StringBuf m_materialName; // name of the material to import from MTL file

    virtual void computeConfigurationKey(CRC64& crc) const override;
};

//--

/// mesh cooker for OBJ files
class IMPORT_OBJ_LOADER_API MTLMaterialImporter : public IGeneralMaterialImporter
{
    RTTI_DECLARE_VIRTUAL_CLASS(MTLMaterialImporter, IGeneralMaterialImporter);

public:
    MTLMaterialImporter();

    virtual res::ResourcePtr importResource(res::IResourceImporterInterface& importer) const override final;
};

//--

END_BOOMER_NAMESPACE_EX(assets)
