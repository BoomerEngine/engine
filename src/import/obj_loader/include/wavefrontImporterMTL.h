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

        base::StringBuf m_bindingColor;

        base::StringBuf m_bindingMapColor;
        base::StringBuf m_bindingMapBump;
        base::StringBuf m_bindingMapNormal;
        base::StringBuf m_bindingMapDissolve;
        base::StringBuf m_bindingMapSpecular;
        base::StringBuf m_bindingMapEmissive;
        base::StringBuf m_bindingMapRoughness;
        base::StringBuf m_bindingMapRoughnessSpecularity;
        base::StringBuf m_bindingMapMetallic;
        base::StringBuf m_bindingMapAmbientOcclusion;

        virtual void computeConfigurationKey(base::CRC64& crc) const override;
    };

    //--

    /// mesh cooker for OBJ files
    class IMPORT_OBJ_LOADER_API MTLMaterialImporter : public base::res::IResourceImporter
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MTLMaterialImporter, base::res::IResourceImporter);

    public:
        MTLMaterialImporter();

        virtual base::res::ResourcePtr importResource(base::res::IResourceImporterInterface& importer) const override final;
    };

    //--

    /// import material
    extern rendering::MaterialInstancePtr ImportMaterial(base::res::IResourceImporterInterface& importer, const rendering::MaterialInstance* existingMaterial, const MTLMaterialImportConfig& csg);

    /// import material
    struct Material;
    extern rendering::MaterialInstancePtr ImportMaterial(base::res::IResourceImporterInterface& importer, const rendering::MaterialInstance* existingMaterial, const MTLMaterialImportConfig& csg, const Material* sourceMaterial);

    //--

} // wavefront
