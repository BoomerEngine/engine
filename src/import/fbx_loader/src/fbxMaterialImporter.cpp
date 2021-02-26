/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: fbx #]
***/

#include "build.h"

#include "fbxMaterialImporter.h"
#include "fbxFileLoaderService.h"
#include "fbxFileData.h"

#include "engine/material/include/renderingMaterialInstance.h"

#include "core/app/include/localServiceContainer.h"
#include "core/resource/include/resourceTags.h"

BEGIN_BOOMER_NAMESPACE_EX(assets)

//--

RTTI_BEGIN_TYPE_CLASS(FBXMaterialImportConfig);
    RTTI_CATEGORY("Material");
    RTTI_PROPERTY(m_materialName).editable("Name of the material to import").overriddable();
RTTI_END_TYPE();

FBXMaterialImportConfig::FBXMaterialImportConfig()
{        
}

void FBXMaterialImportConfig::computeConfigurationKey(CRC64& crc) const
{
    TBaseClass::computeConfigurationKey(crc);
    crc << m_materialName.view();        
}

//--

RTTI_BEGIN_TYPE_CLASS(MaterialImporter);
    RTTI_METADATA(res::ResourceCookedClassMetadata).addClass<MaterialInstance>();
    RTTI_METADATA(res::ResourceSourceFormatMetadata).addSourceExtension("fbx").addSourceExtension("FBX");
    RTTI_METADATA(res::ResourceCookerVersionMetadata).version(0);
    RTTI_METADATA(res::ResourceImporterConfigurationClassMetadata).configurationClass<FBXMaterialImportConfig>();
RTTI_END_TYPE();

//--

// Property 'ShadingModel', type KString
// Property 'MultiLayer', type bool
// Property 'EmissiveColor', type Color
// Property 'EmissiveFactor', type Number
// Property 'AmbientColor', type Color
// Property 'AmbientFactor', type Number
// Property 'DiffuseColor', type Color
// Property 'DiffuseFactor', type Number
// Property 'Bump', type Vector
// Property 'NormalMap', type Vector
// Property 'BumpFactor', type Number
// Property 'TransparentColor', type Color
// Property 'TransparencyFactor', type Number
// Property 'DisplacementColor', type Color
// Property 'DisplacementFactor', type Number
// Property 'VectorDisplacementColor', type Color
// Property 'VectorDisplacementFactor', type Number
// Property 'SpecularColor', type Color
// Property 'SpecularFactor', type Number
// Property 'ShininessExponent', type Number
// Property 'ReflectionColor', type Color
// Property 'ReflectionFactor', type Number
// Property 'Emissive', type Vector
// Property 'Ambient', type Vector
// Property 'Diffuse', type Vector
// Property 'Specular', type Vector
// Property 'Shininess', type Number
// Property 'Opacity', type Number
// Property 'Reflectivity', type Number

//--

static bool TryReadTexturePath(const FbxSurfaceMaterial& material, const char* propertyName, GeneralMaterialTextrureInfo& outTexture)
{
    auto prop = material.FindProperty(propertyName, false);
    if (!prop.IsValid())
        return false;

    if (auto numLayeredTextures = prop.GetSrcObjectCount<FbxLayeredTexture>())
    {
        for (int j = 0; j < numLayeredTextures; j++)
        {
            FbxLayeredTexture* layered_texture = FbxCast<FbxLayeredTexture>(prop.GetSrcObject<FbxLayeredTexture>(j));
            int lcount = layered_texture->GetSrcObjectCount<FbxTexture>();
            for (int k = 0; k < lcount; k++)
            {
                if (auto* lTex = FbxCast<FbxFileTexture>(layered_texture->GetSrcObject<FbxTexture>(k)))
                {
                    if (lTex->GetFileName() && lTex->GetFileName()[0])
                    {
                        TRACE_INFO("Found texture '{}' (FbxLayeredTexture[{}][{}]) at property '{}' in material '{}'", lTex->GetFileName(), j, k, propertyName, material.GetName());

                        auto uOffset = lTex->GetTranslationU();
                        auto vOffset = lTex->GetTranslationV();
                        auto uScale = lTex->GetScaleU();
                        auto vScale = lTex->GetScaleV();
                        TRACE_INFO("UV setup for '{}': Offset: [{},{}], Scale: [{},{}]", lTex->GetFileName(), uOffset, vOffset, uScale, vScale);

                        outTexture.path = StringBuf(lTex->GetFileName());
                        return true;
                    }
                }
            }
        }
    }

    if (auto numTextures = prop.GetSrcObjectCount<FbxTexture>())
    {
        for (int j = 0; j < numTextures; ++j)
        {
            if (auto lTex = prop.GetSrcObject<FbxFileTexture>(j))
            {
                if (lTex->GetFileName() && lTex->GetFileName()[0])
                {
                    TRACE_INFO("Found texture '{}' (FbxTexture[{}]) at property '{}' in material '{}'", lTex->GetFileName(), j, propertyName, material.GetName());

                    auto uOffset = lTex->GetTranslationU();
                    auto vOffset = lTex->GetTranslationV();
                    auto uScale = lTex->GetScaleU();
                    auto vScale = lTex->GetScaleV();
                    TRACE_INFO("UV setup for '{}': Offset: [{},{}], Scale: [{},{}]", lTex->GetFileName(), uOffset, vOffset, uScale, vScale);

                    outTexture.path = StringBuf(lTex->GetFileName());
                    return true;
                }
            }
        }
    }

    return false;
}

//--

MaterialImporter::MaterialImporter()
{}

res::ResourcePtr MaterialImporter::importResource(res::IResourceImporterInterface& importer) const
{
    // load the FBX data
    auto importedScene = rtti_cast<FBXFile>(importer.loadSourceAsset(importer.queryImportPath()));
    if (!importedScene)
    {
        TRACE_ERROR("Failed to load scene from import file");
        return false;
    }

    // no materials :)
    if (importedScene->materials().empty())
    {
        TRACE_ERROR("Loaded FBX file has no materials");
        return false;
    }

    // get the configuration
    auto config = importer.queryConfigration<FBXMaterialImportConfig>();

    // find material to import
    const FbxSurfaceMaterial* material = importedScene->materials()[0];
    if (config->m_materialName)
    {
        material = importedScene->findMaterial(config->m_materialName);
        if (!material)
        {
            TRACE_ERROR("Material '{}' not found in '{}'", config->m_materialName, importer.queryImportPath());
            return nullptr;
        }
    }        

    //--

    // setup general material
    GeneralMaterialInfo info;

    // lighting mode
    {
        auto shadingModel = material->ShadingModel.Get();
        TRACE_INFO("Material '{}' uses shading model '{}'", material->GetName(), shadingModel.Buffer());
        if (shadingModel == "Unlit" || shadingModel == "unlit" || shadingModel == "flat")
            info.forceUnlit = true;
        else
            info.forceUnlit = false;
    }

    // load properties
    TryReadTexturePath(*material, "DiffuseColor", info.textures[GeneralMaterialTextureType_Diffuse]);
    TryReadTexturePath(*material, "NormalMap", info.textures[GeneralMaterialTextureType_Normal]);
    TryReadTexturePath(*material, "SpecularColor", info.textures[GeneralMaterialTextureType_Specularity]);
    TryReadTexturePath(*material, "EmissiveColor", info.textures[GeneralMaterialTextureType_Emissive]);        

    return importMaterial(importer, *config, info);
}

END_BOOMER_NAMESPACE_EX(assets)


