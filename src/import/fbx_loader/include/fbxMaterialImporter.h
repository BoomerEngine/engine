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

namespace fbx
{

    //--

    /// manifest specific for importing Wavefront materials
    class IMPORT_FBX_LOADER_API FBXMaterialImportConfig : public rendering::MaterialImportConfig
    {
        RTTI_DECLARE_VIRTUAL_CLASS(FBXMaterialImportConfig, rendering::MaterialImportConfig);

    public:
        FBXMaterialImportConfig();

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

    /// importer of FBX meshes
    class IMPORT_FBX_LOADER_API MaterialImporter : public rendering::IGeneralMaterialImporter
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialImporter, rendering::IGeneralMaterialImporter);

    public:
        MaterialImporter();

        virtual base::res::ResourcePtr importResource(base::res::IResourceImporterInterface& importer) const override final;
    };

    //--

} // fbx

