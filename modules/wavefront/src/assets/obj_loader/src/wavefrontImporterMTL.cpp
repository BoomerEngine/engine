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

    static base::res::StaticResource<rendering::IMaterial> resUnlitMaterialBase("/engine/materials/std_unlit.v4mg");
    static base::res::StaticResource<rendering::IMaterial> resMaskedMaterialBase("/engine/materials/std_pbr.v4mg");
    static base::res::StaticResource<rendering::IMaterial> resEmissiveMaterialBase("/engine/materials/std_pbr_emissive.v4mg");

    RTTI_BEGIN_TYPE_CLASS(MTLMaterialImportConfig);
        RTTI_CATEGORY("Material");
        RTTI_PROPERTY(m_materialName).editable("Name of the material to import").overriddable();
        RTTI_CATEGORY("Base material");
        RTTI_PROPERTY(m_templateUnlit).editable("Base material to use with illumMode < 2").overriddable();
        RTTI_PROPERTY(m_templateMasked).editable("Base material to use with mask (map_d) is defined").overriddable();
        RTTI_PROPERTY(m_templateEmissive).editable("Base material to use with emissive map (map_e) is defined").overriddable();
        RTTI_CATEGORY("Parameter binding");
        RTTI_PROPERTY(m_bindingColor).editable().overriddable();
        RTTI_PROPERTY(m_bindingMapColor).editable().overriddable();
        RTTI_PROPERTY(m_bindingMapBump).editable().overriddable();
        RTTI_PROPERTY(m_bindingMapNormal).editable().overriddable();
        RTTI_PROPERTY(m_bindingMapDissolve).editable().overriddable();
        RTTI_PROPERTY(m_bindingMapSpecular).editable().overriddable();
        RTTI_PROPERTY(m_bindingMapEmissive).editable().overriddable();
        RTTI_PROPERTY(m_bindingMapRoughness).editable().overriddable();
        RTTI_PROPERTY(m_bindingMapRoughnessSpecularity).editable().overriddable();
        RTTI_PROPERTY(m_bindingMapMetallic).editable().overriddable();
        RTTI_PROPERTY(m_bindingMapAmbientOcclusion).editable().overriddable();
    RTTI_END_TYPE();

    MTLMaterialImportConfig::MTLMaterialImportConfig()
    {
        m_templateUnlit = resUnlitMaterialBase.asyncRef();
        m_templateMasked = resMaskedMaterialBase.asyncRef();
        m_templateEmissive = resEmissiveMaterialBase.asyncRef();

        m_bindingColor = "Color";

        m_bindingMapColor = "ColorMap;BaseColorMap;AlbedoMap";
        m_bindingMapBump = "BumpMap;NormalMap";
        m_bindingMapNormal = "NormalMap";
        m_bindingMapDissolve = "MaskMap";
        m_bindingMapSpecular = "SpecularMap";
        m_bindingMapEmissive = "EmissiveMap";
        m_bindingMapRoughness = "RoughnessMap";
        m_bindingMapRoughnessSpecularity = "RoughnessSpecularMap";
        m_bindingMapMetallic = "MetallicMap";
        m_bindingMapAmbientOcclusion = "AmbientOcclusionMap;AOMap";
    }

    void MTLMaterialImportConfig::computeConfigurationKey(CRC64& crc) const
    {
        TBaseClass::computeConfigurationKey(crc);

        crc << m_templateUnlit.key().path().view();
        crc << m_templateMasked.key().path().view();
        crc << m_templateEmissive.key().path().view();
        crc << m_materialName.view();
        crc << m_bindingColor.view();
        crc << m_bindingMapColor.view();
        crc << m_bindingMapBump.view();
        crc << m_bindingMapNormal.view();
        crc << m_bindingMapDissolve.view();
        crc << m_bindingMapSpecular.view();
        crc << m_bindingMapEmissive.view();
        crc << m_bindingMapRoughness.view();
        crc << m_bindingMapRoughnessSpecularity.view();
        crc << m_bindingMapMetallic.view();
        crc << m_bindingMapAmbientOcclusion.view();
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

    static rendering::MaterialRef LoadBaseMaterial(const Material* sourceMaterial, const MTLMaterialImportConfig& cfg)
    {
        rendering::MaterialRef ret;

        if (sourceMaterial->m_illumMode < 2)
            ret = cfg.m_templateUnlit.load();
        else if (!sourceMaterial->m_mapEmissive.m_path.empty())
            ret = cfg.m_templateEmissive.load();
        else if (!sourceMaterial->m_mapDissolve.m_path.empty())
            ret = cfg.m_templateMasked.load();
        
        if (!ret)
            ret = cfg.m_templateDefault.load();

        return ret;
    }

    static bool WriteTexture(const rendering::TextureRef& textureRef, base::StringID name, base::Array<rendering::MaterialInstanceParam>& outParams)
    {
        if (!name || !textureRef)
            return false;

        for (const auto& param : outParams)
            if (param.name == name)
                return false; // do not overwrite

        auto& param = outParams.emplaceBack();
        param.name = name;
        param.value = base::CreateVariant(textureRef);
        return true;
    }

    static bool TryApplyTexture(base::res::IResourceImporterInterface& importer, const MTLMaterialImportConfig& config, base::StringView mappingParams, const rendering::MaterialTemplate* knownTemplate, const MaterialMap& mtlMap, base::Array<rendering::MaterialInstanceParam>& outParams)
    {
        if (mtlMap.m_path)
        {
            // prepare the texture
            if (auto textureRef = rendering::ImportTextureRef(importer, config, mtlMap.m_path))
            {
                // bind to first matching param, NOTE: we only take into considerations parameters from the template
                if (knownTemplate)
                {
                    base::InplaceArray<base::StringView, 10> possibleParamNames;
                    mappingParams.slice(";", false, possibleParamNames);

                    for (const auto paramNameStr : possibleParamNames)
                    {
                        const auto paramName = base::StringID::Find(paramNameStr);
                        if (const auto* materialParam = knownTemplate->findParameterInfo(paramName))
                        {
                            if (materialParam->type == TYPE_OF(textureRef))
                            {
                                return WriteTexture(textureRef, paramName, outParams);
                            }
                            else
                            {
                                TRACE_WARNING("Unable to bind texture to parameter '{}' from template '{}' as its not a texture parameter", paramName, knownTemplate->path());
                            }
                        }
                    }
                }
            }
        }

        return false;
    }

    rendering::MaterialInstancePtr ImportMaterial(base::res::IResourceImporterInterface& importer, const rendering::MaterialInstance* existingMaterial, const MTLMaterialImportConfig& config)
    {
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
        if (!config.m_materialName.empty())
        {
            sourceMaterial = sourceMaterials->findMaterial(config.m_materialName);
            if (!sourceMaterial)
            {
                TRACE_ERROR("Material '{}' not found in '{}'", config.m_materialName, sourceFilePath);
                return nullptr;
            }
        }

        return ImportMaterial(importer, existingMaterial, config, sourceMaterial);
    }

    rendering::MaterialInstancePtr ImportMaterial(base::res::IResourceImporterInterface& importer, const rendering::MaterialInstance* existingMaterial, const MTLMaterialImportConfig& config, const Material* sourceMaterial)
    {
        // load the base template to use, this can't fail - otherwise what's the point
        const auto baseMaterial = LoadBaseMaterial(sourceMaterial, config);
        if (!baseMaterial)
        {
            TRACE_ERROR("Could not load base template for material '{}'", sourceMaterial->m_name);
            return nullptr;
        }

        // we must have a valid template
        const auto baseTemplate = baseMaterial.acquire() ? baseMaterial.acquire()->resolveTemplate() : nullptr;
        if (!baseTemplate)
        {
            TRACE_ERROR("Could not resolve base template for material '{}'", sourceMaterial->m_name);
            return nullptr;
        }

        // gather parameters
        base::Array<rendering::MaterialInstanceParam> importedParameters;
        importedParameters.reserve(8);

        // apply properties
        TryApplyTexture(importer, config, config.m_bindingMapColor, baseTemplate, sourceMaterial->m_mapDiffuse, importedParameters);
        TryApplyTexture(importer, config, config.m_bindingMapNormal, baseTemplate, sourceMaterial->m_mapNormal, importedParameters);
        TryApplyTexture(importer, config, config.m_bindingMapBump, baseTemplate, sourceMaterial->m_mapBump, importedParameters);
        TryApplyTexture(importer, config, config.m_bindingMapRoughnessSpecularity, baseTemplate, sourceMaterial->m_mapRoughnessSpecularity, importedParameters);
        TryApplyTexture(importer, config, config.m_bindingMapRoughness, baseTemplate, sourceMaterial->m_mapRoughness, importedParameters);
        TryApplyTexture(importer, config, config.m_bindingMapSpecular, baseTemplate, sourceMaterial->m_mapSpecular, importedParameters);
        TryApplyTexture(importer, config, config.m_bindingMapDissolve, baseTemplate, sourceMaterial->m_mapDissolve, importedParameters);
        TryApplyTexture(importer, config, config.m_bindingMapMetallic, baseTemplate, sourceMaterial->m_mapMetallic, importedParameters);
        TryApplyTexture(importer, config, config.m_bindingMapEmissive, baseTemplate, sourceMaterial->m_mapEmissive, importedParameters);
        //TryApplyTexture(importer, config, config.m_bindingMapAmbientOcclusion, baseTemplate, sourceMaterial->m_mapAmbient, importedParameters);
        
        // build the "imported material"
        auto importedMaterial = base::RefNew<rendering::MaterialInstance>(baseMaterial, std::move(importedParameters));

        // get the "overlay" user parameters from existing material data
        base::Array<rendering::MaterialInstanceParam> userParameters;
        if (existingMaterial)
        {
            // NOTE: copy only parameters that are actually changed
            for (const auto& param : existingMaterial->parameters())
                if (existingMaterial->checkParameterOverride(param.name))
                    userParameters.emplaceBack(param);
        }

        // build final material that uses the user parameters on top of the imported base material
        return base::RefNew<rendering::MaterialInstance>(baseMaterial, std::move(userParameters), importedMaterial);
    }

    base::res::ResourcePtr MTLMaterialImporter::importResource(base::res::IResourceImporterInterface& importer) const
    {
        // get the configuration for material import
        auto config = importer.queryConfigration<MTLMaterialImportConfig>();

        // load existing material
        const auto* existingMaterial = rtti_cast<rendering::MaterialInstance>(importer.existingData());

        // import the material
        return ImportMaterial(importer, existingMaterial, *config);
    }

    //--

} // mesh


