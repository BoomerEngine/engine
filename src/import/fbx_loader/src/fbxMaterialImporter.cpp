/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: fbx #]
***/

#include "build.h"

#include "fbxMaterialImporter.h"
#include "fbxFileData.h"

#include "engine/material/include/materialInstance.h"

#include "core/app/include/localServiceContainer.h"
#include "core/resource/include/tags.h"

BEGIN_BOOMER_NAMESPACE_EX(assets)

//--

RTTI_BEGIN_TYPE_CLASS(FBXMaterialImportConfig);
    RTTI_OLD_NAME("fbx::FBXMaterialImportConfig");
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
    RTTI_METADATA(ResourceCookedClassMetadata).addClass<MaterialInstance>();
    RTTI_METADATA(ResourceSourceFormatMetadata).addSourceExtension("fbx").addSourceExtension("FBX");
    RTTI_METADATA(ResourceCookerVersionMetadata).version(0);
    RTTI_METADATA(ResourceImporterConfigurationClassMetadata).configurationClass<FBXMaterialImportConfig>();
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

static bool TryReadString(const ofbx::DataView path, StringBuf& outString)
{
    const auto txt = StringView((const char*)path.begin, (const char*)path.end);
    if (!txt.empty())
    {
        outString = StringBuf(txt);
        return true;
    }

    return false;    
}

static bool TryReadTexturePath(const ofbx::Material* material, ofbx::Texture::TextureType textureType, GeneralMaterialTextrureInfo& outTexture)
{
    if (const auto* texture = material->getTexture(textureType))
    {
        TRACE_INFO("Found texture '{}' in material '{}'", texture->name, material->name);

        StringBuf path;
        if (TryReadString(texture->getFileName(), path))
        {
            TRACE_INFO("Texture path for '{}': '{}'", texture->name, path);
            outTexture.path = path;
            return true;
        }

        if (TryReadString(texture->getRelativeFileName(), path))
        {
            TRACE_INFO("Texture path for '{}': '{}'", texture->name, path);
            outTexture.path = path;
            return true;
        }
    }

    return false;
}

//--

MaterialImporter::MaterialImporter()
{}

ResourcePtr MaterialImporter::importResource(IResourceImporterInterface& importer) const
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
    const auto* material = importedScene->materials()[0];
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
    if (StringView(material->name).endsWithNoCase("unlit"))
        info.forceUnlit = true;
    else
        info.forceUnlit = false;

    // load properties
    TryReadTexturePath(material, ofbx::Texture::DIFFUSE, info.textures[GeneralMaterialTextureType_Diffuse]);
    TryReadTexturePath(material, ofbx::Texture::NORMAL, info.textures[GeneralMaterialTextureType_Normal]);
    TryReadTexturePath(material, ofbx::Texture::SPECULAR, info.textures[GeneralMaterialTextureType_Specularity]);
    TryReadTexturePath(material, ofbx::Texture::EMISSIVE, info.textures[GeneralMaterialTextureType_Emissive]);
    TryReadTexturePath(material, ofbx::Texture::SHININESS, info.textures[GeneralMaterialTextureType_Roughness]);
    TryReadTexturePath(material, ofbx::Texture::REFLECTION, info.textures[GeneralMaterialTextureType_Roughness]);

    return importMaterial(importer, *config, info);
}

END_BOOMER_NAMESPACE_EX(assets)


