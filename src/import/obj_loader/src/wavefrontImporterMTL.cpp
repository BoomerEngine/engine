/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: import #]
***/

#include "build.h"
#include "wavefrontImporterMTL.h"
#include "wavefrontFormatMTL.h"
#include "rendering/material/include/renderingMaterialInstance.h"
#include "rendering/material/include/renderingMaterialTemplate.h"
#include "rendering/texture/include/renderingTexture.h"
#include "base/resource/include/resourceTags.h"
#include "base/resource_compiler/include/importInterface.h"

namespace wavefront
{

    //--

    RTTI_BEGIN_TYPE_CLASS(MTLMaterialImportConfig);
        RTTI_CATEGORY("Material");
        RTTI_PROPERTY(m_materialName).editable("Name of the material to import").overriddable();
    RTTI_END_TYPE();

    MTLMaterialImportConfig::MTLMaterialImportConfig()
    {
    }

    void MTLMaterialImportConfig::computeConfigurationKey(base::CRC64& crc) const
    {
        TBaseClass::computeConfigurationKey(crc);
        crc << m_materialName.view();
    }

    //--

    RTTI_BEGIN_TYPE_CLASS(MTLMaterialImporter);
        RTTI_METADATA(base::res::ResourceCookedClassMetadata).addClass<rendering::MaterialInstance>();
        RTTI_METADATA(base::res::ResourceSourceFormatMetadata).addSourceExtension("mtl");
        RTTI_METADATA(base::res::ResourceCookerVersionMetadata).version(5);
        RTTI_METADATA(base::res::ResourceImporterConfigurationClassMetadata).configurationClass<MTLMaterialImportConfig>();
    RTTI_END_TYPE();

    MTLMaterialImporter::MTLMaterialImporter()
    {
    }

    static void ExtractTextureInfo(rendering::GeneralMaterialTextrureInfo& outTexture, const MaterialMap& src)
    {
        if (!src.m_path.empty())
            outTexture.path = src.m_path;
    }

    base::res::ResourcePtr MTLMaterialImporter::importResource(base::res::IResourceImporterInterface& importer) const
    {
        // get the configuration for material import
        auto config = importer.queryConfigration<MTLMaterialImportConfig>();

        // load source data from MTL format
        auto sourceFilePath = importer.queryImportPath();
        auto sourceMaterials = base::rtti_cast<wavefront::FormatMTL>(importer.loadSourceAsset(importer.queryImportPath()));
        if (!sourceMaterials)
            return nullptr;

        // no materials to import ?
        if (sourceMaterials->materials().empty())
        {
            TRACE_ERROR("No materials found in '{}'. Parsing error?", sourceFilePath);
            return nullptr;
        }

        // no material specified, use the first one, otherwise search for the mentioned material
        const auto* sourceMaterial = &sourceMaterials->materials().front();
        if (config->m_materialName)
        {
            sourceMaterial = sourceMaterials->findMaterial(config->m_materialName);
            if (!sourceMaterial)
            {
                TRACE_ERROR("Material '{}' not found in '{}'", config->m_materialName, sourceFilePath);
                return nullptr;
            }
        }

        // extract crap into general material
        rendering::GeneralMaterialInfo materialSetup;
        materialSetup.forceUnlit = (sourceMaterial->m_illumMode < 2);
        ExtractTextureInfo(materialSetup.textures[rendering::GeneralMaterialTextureType_Diffuse], sourceMaterial->m_mapDiffuse);
        ExtractTextureInfo(materialSetup.textures[rendering::GeneralMaterialTextureType_Normal], sourceMaterial->m_mapNormal);
        ExtractTextureInfo(materialSetup.textures[rendering::GeneralMaterialTextureType_Bump], sourceMaterial->m_mapBump);
        ExtractTextureInfo(materialSetup.textures[rendering::GeneralMaterialTextureType_RoughnessSpecularity], sourceMaterial->m_mapRoughnessSpecularity);
        ExtractTextureInfo(materialSetup.textures[rendering::GeneralMaterialTextureType_Roughness], sourceMaterial->m_mapRoughness);
        ExtractTextureInfo(materialSetup.textures[rendering::GeneralMaterialTextureType_Specularity], sourceMaterial->m_mapSpecular);
        ExtractTextureInfo(materialSetup.textures[rendering::GeneralMaterialTextureType_Mask], sourceMaterial->m_mapDissolve);
        ExtractTextureInfo(materialSetup.textures[rendering::GeneralMaterialTextureType_Metallic], sourceMaterial->m_mapMetallic);
        ExtractTextureInfo(materialSetup.textures[rendering::GeneralMaterialTextureType_Emissive], sourceMaterial->m_mapEmissive);

        return importMaterial(importer, *config, materialSetup);
    }

    //--

} // mesh


