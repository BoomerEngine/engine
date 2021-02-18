/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: import #]
***/

#pragma once

#include "base/resource_compiler/include/importInterface.h"
#include "import/mesh_loader/include/renderingMaterialImportConfig.h"

namespace wavefront
{

    //--

    /// manifest specific for importing Wavefront materials
    class IMPORT_OBJ_LOADER_API MTLMaterialImportConfig : public rendering::MaterialImportConfig
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MTLMaterialImportConfig, rendering::MaterialImportConfig);

    public:
        MTLMaterialImportConfig();

        base::StringBuf m_materialName; // name of the material to import from MTL file

        virtual void computeConfigurationKey(base::CRC64& crc) const override;
    };

    //--

    /// mesh cooker for OBJ files
    class IMPORT_OBJ_LOADER_API MTLMaterialImporter : public rendering::IGeneralMaterialImporter
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MTLMaterialImporter, rendering::IGeneralMaterialImporter);

    public:
        MTLMaterialImporter();

        virtual base::res::ResourcePtr importResource(base::res::IResourceImporterInterface& importer) const override final;
    };

    //--

} // wavefront
