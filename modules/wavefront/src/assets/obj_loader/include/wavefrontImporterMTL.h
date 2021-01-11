/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: import #]
***/

#pragma once

#include "base/resource_compiler/include/importInterface.h"
#include "assets/mesh_loader/include/renderingMaterialImportConfig.h"

namespace wavefront
{

    //--

    /// manifest specific for importing Wavefront materials
    class ASSETS_OBJ_LOADER_API MTLMaterialImportConfig : public rendering::MaterialImportConfig
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MTLMaterialImportConfig, rendering::MaterialImportConfig);

    public:
        MTLMaterialImportConfig();

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

    /// mesh cooker for OBJ files
    class ASSETS_OBJ_LOADER_API MTLMaterialImporter : public base::res::IResourceImporter
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
