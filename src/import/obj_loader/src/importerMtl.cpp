/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: import #]
***/

#include "build.h"
#include "importerMtl.h"
#include "fileMtl.h"

#include "engine/material/include/materialInstance.h"
#include "engine/material/include/materialTemplate.h"
#include "engine/texture/include/texture.h"

#include "core/resource/include/resourceTags.h"
#include "core/resource_compiler/include/importInterface.h"

BEGIN_BOOMER_NAMESPACE_EX(assets)

//--

RTTI_BEGIN_TYPE_CLASS(MTLMaterialImportConfig);
    RTTI_CATEGORY("Material");
    RTTI_PROPERTY(m_materialName).editable("Name of the material to import").overriddable();
    RTTI_OLD_NAME("wavefront::MTLMaterialImportConfig");
RTTI_END_TYPE();

MTLMaterialImportConfig::MTLMaterialImportConfig()
{
}

void MTLMaterialImportConfig::computeConfigurationKey(CRC64& crc) const
{
    TBaseClass::computeConfigurationKey(crc);
    crc << m_materialName.view();
}

//--

RTTI_BEGIN_TYPE_CLASS(MTLMaterialImporter);
    RTTI_OLD_NAME("wavefront::MTLMaterialImporter");
    RTTI_METADATA(res::ResourceCookedClassMetadata).addClass<MaterialInstance>();
    RTTI_METADATA(res::ResourceSourceFormatMetadata).addSourceExtension("mtl");
    RTTI_METADATA(res::ResourceCookerVersionMetadata).version(5);
    RTTI_METADATA(res::ResourceImporterConfigurationClassMetadata).configurationClass<MTLMaterialImportConfig>();
RTTI_END_TYPE();

MTLMaterialImporter::MTLMaterialImporter()
{
}

static void ExtractTextureInfo(GeneralMaterialTextrureInfo& outTexture, const MaterialMap& src)
{
    if (!src.m_path.empty())
        outTexture.path = src.m_path;
}

res::ResourcePtr MTLMaterialImporter::importResource(res::IResourceImporterInterface& importer) const
{
    // get the configuration for material import
    auto config = importer.queryConfigration<MTLMaterialImportConfig>();

    // load source data from MTL format
    auto sourceFilePath = importer.queryImportPath();
    auto sourceMaterials = rtti_cast<SourceAssetMTL>(importer.loadSourceAsset(importer.queryImportPath()));
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
    GeneralMaterialInfo materialSetup;
    materialSetup.forceUnlit = (sourceMaterial->m_illumMode < 2);
    ExtractTextureInfo(materialSetup.textures[GeneralMaterialTextureType_Diffuse], sourceMaterial->m_mapDiffuse);
    ExtractTextureInfo(materialSetup.textures[GeneralMaterialTextureType_Normal], sourceMaterial->m_mapNormal);
    ExtractTextureInfo(materialSetup.textures[GeneralMaterialTextureType_Bump], sourceMaterial->m_mapBump);
    ExtractTextureInfo(materialSetup.textures[GeneralMaterialTextureType_RoughnessSpecularity], sourceMaterial->m_mapRoughnessSpecularity);
    ExtractTextureInfo(materialSetup.textures[GeneralMaterialTextureType_Roughness], sourceMaterial->m_mapRoughness);
    ExtractTextureInfo(materialSetup.textures[GeneralMaterialTextureType_Specularity], sourceMaterial->m_mapSpecular);
    ExtractTextureInfo(materialSetup.textures[GeneralMaterialTextureType_Mask], sourceMaterial->m_mapDissolve);
    ExtractTextureInfo(materialSetup.textures[GeneralMaterialTextureType_Metallic], sourceMaterial->m_mapMetallic);
    ExtractTextureInfo(materialSetup.textures[GeneralMaterialTextureType_Emissive], sourceMaterial->m_mapEmissive);

    return importMaterial(importer, *config, materialSetup);
}

//--

END_BOOMER_NAMESPACE_EX(assets)


