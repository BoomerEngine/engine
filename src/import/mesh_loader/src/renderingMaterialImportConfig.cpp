/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: import\config #]
***/

#include "build.h"
#include "renderingMaterialImportConfig.h"

#include "engine/texture/include/staticTexture.h"
#include "engine/material/include/material.h"
#include "engine/material/include/materialInstance.h"
#include "engine/material/include/materialTemplate.h"

#include "core/resource_compiler/include/importInterface.h"
#include "core/containers/include/path.h"

BEGIN_BOOMER_NAMESPACE_EX(assets)

//--

RTTI_BEGIN_TYPE_CLASS(MaterialImportConfig);
    RTTI_OLD_NAME("rendering::MaterialImportConfig");
    RTTI_CATEGORY("Texture import");
    RTTI_PROPERTY(m_importTextures).editable().overriddable();
    RTTI_PROPERTY(m_textureSearchPath).editable().overriddable();
    RTTI_PROPERTY(m_textureImportPath).editable().overriddable();
    RTTI_PROPERTY(m_autoDiscoverTextures).editable().overriddable();
    RTTI_CATEGORY("Asset search");
    RTTI_PROPERTY(m_depotSearchDepth).editable().overriddable();
    RTTI_PROPERTY(m_sourceAssetsSearchDepth).editable().overriddable();
    RTTI_CATEGORY("Parameter binding");
    RTTI_PROPERTY(m_bindingColor).editable().overriddable();
    RTTI_PROPERTY(m_bindingMapColor).editable().overriddable();
    RTTI_PROPERTY(m_bindingMapBump).editable().overriddable();
    RTTI_PROPERTY(m_bindingMapNormal).editable().overriddable();
    RTTI_PROPERTY(m_bindingMapMask).editable().overriddable();
    RTTI_PROPERTY(m_bindingMapSpecular).editable().overriddable();
    RTTI_PROPERTY(m_bindingMapEmissive).editable().overriddable();
    RTTI_PROPERTY(m_bindingMapRoughness).editable().overriddable();
    RTTI_PROPERTY(m_bindingMapMetallic).editable().overriddable();
    RTTI_PROPERTY(m_bindingMapAmbientOcclusion).editable().overriddable();
    RTTI_CATEGORY("Automatic texture extensions");
    RTTI_PROPERTY(m_postfixColor).editable().overriddable();
    RTTI_PROPERTY(m_postfixNormal).editable().overriddable();
    RTTI_PROPERTY(m_postfixBump).editable().overriddable();
    RTTI_PROPERTY(m_postfixMask).editable().overriddable();
    RTTI_PROPERTY(m_postfixSpecular).editable().overriddable();
    RTTI_PROPERTY(m_postfixEmissive).editable().overriddable();
    RTTI_PROPERTY(m_postfixRoughness).editable().overriddable();
    RTTI_PROPERTY(m_postfixRoughnessSpecularity).editable().overriddable();
    RTTI_PROPERTY(m_postfixNormalSpecularity).editable().overriddable();
    RTTI_PROPERTY(m_postfixMetallic).editable().overriddable();
    RTTI_PROPERTY(m_postfixAmbientOcclusion).editable().overriddable();
    RTTI_PROPERTY(m_postfixMetallicAOSpecularSmoothness).editable().overriddable();
RTTI_END_TYPE();

MaterialImportConfig::MaterialImportConfig()
{
    m_textureImportPath = StringBuf("../textures/");
    m_textureSearchPath = StringBuf("../textures/");

    m_bindingColor = "Color";

    m_bindingMapColor = "ColorMap;BaseColorMap;AlbedoMap";
    m_bindingMapBump = "BumpMap;NormalMap";
    m_bindingMapNormal = "NormalMap";
    m_bindingMapMask = "MaskMap";
    m_bindingMapSpecular = "SpecularMap";
    m_bindingMapEmissive = "EmissiveMap";
    m_bindingMapRoughness = "RoughnessMap";
    m_bindingMapMetallic = "MetallicMap";
    m_bindingMapAmbientOcclusion = "AmbientOcclusionMap;AOMap";

    m_postfixColor = "diffuse;albedo;diff;d;D";
    m_postfixColorMask = "diffuseMask;DM;dm";
    m_postfixNormal = "normal;ddna;n;N";
    m_postfixBump = "bump";
    m_postfixMask = "M";
    m_postfixSpecular = "spec;s;S";
    m_postfixRoughness = "roughness;R";
    m_postfixRoughnessSpecularity = "rs;RS";
    m_postfixNormalSpecularity = "ns;NS";
    m_postfixEmissive = "emissive;E";
    m_postfixMetallic = "metallic;MT";
    m_postfixAmbientOcclusion = "ao;AO";
    m_postfixMetallicAOSpecularSmoothness = "mask";
}

void MaterialImportConfig::computeConfigurationKey(CRC64& crc) const
{
    TBaseClass::computeConfigurationKey(crc);

    crc << m_importTextures;
    crc << m_textureImportPath.view();
    crc << m_textureSearchPath.view();

    crc << m_depotSearchDepth;
    crc << m_sourceAssetsSearchDepth;

    crc << m_bindingColor.view();
    crc << m_bindingMapColor.view();
    crc << m_bindingMapBump.view();
    crc << m_bindingMapNormal.view();
    crc << m_bindingMapMask.view();
    crc << m_bindingMapSpecular.view();
    crc << m_bindingMapEmissive.view();
    crc << m_bindingMapRoughness.view();
    crc << m_bindingMapMetallic.view();
    crc << m_bindingMapAmbientOcclusion.view();

    crc << m_postfixColor.view();
    crc << m_postfixColorMask.view();
    crc << m_postfixNormal.view();
    crc << m_postfixBump.view();
    crc << m_postfixMask.view();
    crc << m_postfixSpecular.view();
    crc << m_postfixRoughness.view();
    crc << m_postfixRoughnessSpecularity.view();
    crc << m_postfixNormalSpecularity.view();
    crc << m_postfixEmissive.view();
    crc << m_postfixMetallic.view();
    crc << m_postfixAmbientOcclusion.view();
    crc << m_postfixMetallicAOSpecularSmoothness.view();
}

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IGeneralMaterialImporter);
RTTI_END_TYPE();

IGeneralMaterialImporter::~IGeneralMaterialImporter()
{
}

StringView IGeneralMaterialImporter::ExtractRootFileName(StringView assetFileName)
{
    auto name = assetFileName.afterLastOrFull("/").afterLastOrFull("\\");
    return name.beforeFirstOrFull(".");
}

StringBuf IGeneralMaterialImporter::ConvertPathToTextureSearchPath(StringView assetPathToTexture)
{
    StringBuf safeCoreName;
    const auto coreName = ExtractRootFileName(assetPathToTexture);
    if (!MakeSafeFileName(coreName, safeCoreName))
        return StringBuf();

    static const auto textureExtension = IResource::FILE_EXTENSION;
    return TempString("{}.{}", safeCoreName, textureExtension);
}

void IGeneralMaterialImporter::GlueDepotPath(StringView path, bool isFileName, Array<StringView>& outPathParts)
{
    InplaceArray<StringView, 10> pathParts;
    path.slice("/\\", false, pathParts);

    // skip the file name itself
    const auto dirPath = path.endsWith("/") || path.endsWith("\\");
    if (isFileName && (!dirPath || !pathParts.empty()))
        pathParts.popBack();

    // if we are absolute path than clear existing stuff
    const auto absolutePath = path.beginsWith("/") || path.beginsWith("\\");
    if (absolutePath)
        outPathParts.clear();

    // append path parts, respecting the ".." and "."
    for (const auto part : pathParts)
    {
        if (part == ".")
            continue;

        if (part == "..")
        {
            if (!outPathParts.empty())
                outPathParts.popBack();
            continue;
        }

        outPathParts.pushBack(part);
    }
}

void IGeneralMaterialImporter::EmitDepotPath(const Array<StringView>& pathParts, IFormatStream& f)
{
    f << "/";

    for (const auto part : pathParts)
    {
        f << part;
        f << "/";
    }
}

StringBuf IGeneralMaterialImporter::BuildTextureDepotPath(StringView referenceDepotPath, StringView textureImportPath, StringView assetFileName)
{
    InplaceArray<StringView, 20> pathParts;
    GlueDepotPath(referenceDepotPath, true, pathParts);
    GlueDepotPath(textureImportPath, false, pathParts);

    StringBuilder txt;
    EmitDepotPath(pathParts, txt);

    txt << ConvertPathToTextureSearchPath(assetFileName);

    return txt.toString();
}

TextureRef IGeneralMaterialImporter::importTextureRef(IResourceImporterInterface& importer, const MaterialImportConfig& cfg, StringView assetPathToTexture, StringBuf& outAssetImportPath) const
{
    // no path
    if (assetPathToTexture.empty())
        return TextureRef();

    // not imported
    if (!cfg.m_importTextures)
        return TextureRef();

    // crap
    /*StringBuf tempAssetPathToTexture;
    if (assetPathToTexture.endsWithNoCase(".tif") || assetPathToTexture.endsWithNoCase(".tiff"))
    {
        tempAssetPathToTexture = TempString("{}{}.tga", assetPathToTexture.baseDirectory(), assetPathToTexture.fileStem());
        assetPathToTexture = tempAssetPathToTexture;
    }*/

    // try to find texture in depot if the search path was specified
    if (cfg.m_textureSearchPath)
    {        
        // get the findable texture path - ie. strip all nonsense from the asset path, especially the ".." or drive letters as they are not used during search
        const auto findableName = ConvertPathToTextureSearchPath(assetPathToTexture);
        TRACE_INFO("Looking for texture '{}' (imported from '{}')...", findableName, assetPathToTexture);

        // look for a depot file in existing depot
        const auto depotScanDepth = std::clamp<int>(cfg.m_depotSearchDepth, 0, 20);
        StringBuf depotPath;
        ResourceID depotResourceID;
        if (importer.findDepotFile(importer.queryResourcePath().view(), cfg.m_textureSearchPath, findableName, depotPath, &depotResourceID, depotScanDepth))
        {
            TRACE_INFO("Found '{}' at '{}' (as {})", assetPathToTexture, depotPath, depotResourceID);
            return TextureRef(depotResourceID);
        }
    }

    // if nothing was found, try the import
    if (cfg.m_textureImportPath)
    {
        // search in the vicinity of source asset
        StringBuf foundTexturePath;
        const auto assetDepotScanDepth = std::clamp<int>(cfg.m_sourceAssetsSearchDepth, 0, 20);
        if (importer.findSourceFile(importer.queryImportPath(), assetPathToTexture, foundTexturePath, assetDepotScanDepth))
        {
            // build depot path for the imported texture
            const auto depotPath = BuildTextureDepotPath(importer.queryResourcePath().view(), cfg.m_textureImportPath, foundTexturePath);
            TRACE_INFO("Texture '{}' found at '{}' will be improted as '{}'", assetPathToTexture, foundTexturePath, depotPath);

            // emit the follow-up import, no extra config at the moment
            if (const auto textureRef = importer.followupImport<ITexture>(foundTexturePath, depotPath))
            {
                // export the asset import path
                outAssetImportPath = foundTexturePath;

                // build a unloaded texture reference (so it can be saved)
                return textureRef;
            }
        }
    }

    // no texture can be imported
    TRACE_WARNING("No texture will be bound despite having valid path '{}'", assetPathToTexture);
    return nullptr;
}

struct LoadedTexture
{
    TextureRef textureRef;
    StringBuf importPath;

    INLINE operator bool() const
    {
        return !importPath.empty();
    }
};

struct LoadedTextures
{
    bool forceUnlit = false;
    LoadedTexture textures[GeneralMaterialTextureType_MAX];
};

bool IGeneralMaterialImporter::WriteTexture(const TextureRef& textureRef, StringID name, Array<MaterialInstanceParam>& outParams)
{
    if (!name || !textureRef)
        return false;

    for (const auto& param : outParams)
        if (param.name == name)
            return false; // do not overwrite

    auto& param = outParams.emplaceBack();
    param.name = name;
    param.value = CreateVariant(textureRef);
    return true;
}

bool IGeneralMaterialImporter::WriteBool(bool value, StringID name, Array<MaterialInstanceParam>& outParams)
{
    if (!name)
        return false;

    for (const auto& param : outParams)
        if (param.name == name)
            return false; // do not overwrite

    auto& param = outParams.emplaceBack();
    param.name = name;
    param.value = CreateVariant(value);
    return true;
}

bool IGeneralMaterialImporter::WriteVector4(Vector4 value, StringID name, Array<MaterialInstanceParam>& outParams)
{
    if (!name)
        return false;

    for (const auto& param : outParams)
        if (param.name == name)
            return false; // do not overwrite

    auto& param = outParams.emplaceBack();
    param.name = name;
    param.value = CreateVariant(value);
    return true;
}

bool IGeneralMaterialImporter::loadTexture(IResourceImporterInterface& importer, const MaterialImportConfig& config, const GeneralMaterialTextrureInfo& info, LoadedTexture& outLoadedTexture) const
{
    StringBuf importPath;
    if (auto textureRef = importTextureRef(importer, config, info.path, importPath))
    {
        outLoadedTexture.textureRef = textureRef;
        outLoadedTexture.importPath = importPath;
        return true;
    }

    return false;
}

bool IGeneralMaterialImporter::tryLoadAlternativeTexture(IResourceImporterInterface& importer, const MaterialImportConfig& config, const GeneralMaterialTextrureInfo& info, StringView additionalSuffixes, LoadedTexture& outLoadedTexture) const
{
    if (!info)
        return false;

    const auto coreExtensions = info.path.view().extensions();
    const auto coreDirectory = info.path.view().baseDirectory();
    const auto coreName = info.path.view().fileStem().beforeLastOrFull("_");

    InplaceArray<StringView, 20> suffixes;
    additionalSuffixes.slice(";", false, suffixes);

    for (const auto suffix : suffixes)
    {
        StringBuf importPath;
        if (auto textureRef = importTextureRef(importer, config, TempString("{}{}_{}.{}", coreDirectory, coreName, suffix, coreExtensions), importPath))
        {
            outLoadedTexture.textureRef = textureRef;
            outLoadedTexture.importPath = importPath;
            return true;
        }
    }

    return false;
}

static void ExtractPathsAndStems(StringView importPath, Array<StringBuf>& outImportPaths)
{
    if (importPath)
        outImportPaths.pushBackUnique(StringBuf(importPath));
}

bool IGeneralMaterialImporter::findAssetSourcePath(IResourceImporterInterface& importer, const Array<StringBuf>& paths, const Array<StringView>& suffixes, StringBuf& outPath) const
{
    for (const auto& path : paths)
    {
        const auto dir = path.view().baseDirectory();
        const auto ext = path.view().extensions();

        // test assuming the file name has existing suffix (ie. Bricks_D.png)
        {
            const auto stem = path.view().fileStem().beforeLastOrFull("_");
            for (const auto& suffix : suffixes)
            {
                const auto sourceTexturePath = StringBuf(TempString("{}{}_{}.{}", dir, stem, suffix, ext));
                if (importer.checkSourceFile(sourceTexturePath))
                {
                    TRACE_INFO("Auto-discovered existing texture '{}'", sourceTexturePath);
                    outPath = sourceTexturePath;
                    return true;
                }
            }
        }
            
        // test assuming the file did not have suffix
        {
            // TODO
        }
    }

    return false;
}

void IGeneralMaterialImporter::loadAdditionalTextures(IResourceImporterInterface& importer, const MaterialImportConfig& config, LoadedTextures& outLoadedTextures) const
{
    Array<StringBuf> importPaths;
    for (const auto& tex : outLoadedTextures.textures)
        ExtractPathsAndStems(tex.importPath, importPaths);

    StringView suffixes[GeneralMaterialTextureType_MAX];
    suffixes[GeneralMaterialTextureType_Diffuse] = config.m_postfixColor;
    suffixes[GeneralMaterialTextureType_DiffuseMask] = config.m_postfixColorMask;
    suffixes[GeneralMaterialTextureType_Normal] = config.m_postfixNormal;
    suffixes[GeneralMaterialTextureType_Bump] = config.m_postfixBump;
    suffixes[GeneralMaterialTextureType_Mask] = config.m_postfixMask;
    suffixes[GeneralMaterialTextureType_Specularity] = config.m_postfixSpecular;
    suffixes[GeneralMaterialTextureType_Emissive] = config.m_postfixEmissive;
    suffixes[GeneralMaterialTextureType_Roughness] = config.m_postfixRoughness;
    suffixes[GeneralMaterialTextureType_NormalSpecularity] = config.m_postfixNormalSpecularity;
    suffixes[GeneralMaterialTextureType_RoughnessSpecularity] = config.m_postfixRoughnessSpecularity;
    suffixes[GeneralMaterialTextureType_Metallic] = config.m_postfixMetallic;
    suffixes[GeneralMaterialTextureType_AO] = config.m_postfixAmbientOcclusion;
    suffixes[GeneralMaterialTextureType_MetallicAOSpecularSmoothness] = config.m_postfixMetallicAOSpecularSmoothness;

    for (uint32_t i = 0; i < GeneralMaterialTextureType_MAX; ++i)
    {
        auto& tex = outLoadedTextures.textures[i];
        if (tex.importPath || !suffixes[i])
            continue;

        InplaceArray<StringView, 10> possibleSuffixes;
        suffixes[i].slice(";", false, possibleSuffixes);

        StringBuf discoveredPath;
        if (findAssetSourcePath(importer, importPaths, possibleSuffixes, discoveredPath))
        {
            GeneralMaterialTextrureInfo info;
            info.path = discoveredPath;

            loadTexture(importer, config, info, tex);
        }
    }
}

bool IGeneralMaterialImporter::tryApplyTexture(IResourceImporterInterface& importer, const MaterialImportConfig& config, StringView mappingParams, const MaterialTemplate* knownTemplate, const LoadedTexture& texture, Array<MaterialInstanceParam>& outParams) const
{
    if (!knownTemplate)
        return false;

    if (!texture.textureRef)
        return false;

    // bind to first matching param, NOTE: we only take into considerations parameters from the template
    InplaceArray<StringView, 10> possibleParamNames;
    mappingParams.slice(";", false, possibleParamNames);
    for (const auto paramNameStr : possibleParamNames)
    {
        const auto paramName = StringID::Find(paramNameStr);

        if (const auto* param = knownTemplate->findParameter(paramName))
        {
            if (param->queryDataType() == TYPE_OF(texture.textureRef))
            {
                if (WriteTexture(texture.textureRef, paramName, outParams))
                    return true;
            }

            TRACE_WARNING("Unable to bind texture to parameter '{}' from template '{}' as its not a texture parameter", paramName, knownTemplate->loadPath());
        }
    }

    // not found
    return false;
}

bool IGeneralMaterialImporter::applyBoolParam(StringView mappingParams, const MaterialTemplate* knownTemplate, bool value, Array<MaterialInstanceParam>& outParams) const
{
    if (!knownTemplate)
        return false;

    // bind to first matching param, NOTE: we only take into considerations parameters from the template
    InplaceArray<StringView, 10> possibleParamNames;
    mappingParams.slice(";", false, possibleParamNames);
    for (const auto paramNameStr : possibleParamNames)
    {
        const auto paramName = StringID::Find(paramNameStr);

        if (const auto* param = knownTemplate->findParameter(paramName))
        {
            if (param->queryDataType() == TYPE_OF(value))
            {
                if (WriteBool(value, paramName, outParams))
                    return true;
            }
        }
    }

    // not found
    return false;
}

bool IGeneralMaterialImporter::applyVector4Param(StringView mappingParams, const MaterialTemplate* knownTemplate, Vector4 value, Array<MaterialInstanceParam>& outParams) const
{
    if (!knownTemplate)
        return false;

    // bind to first matching param, NOTE: we only take into considerations parameters from the template
    InplaceArray<StringView, 10> possibleParamNames;
    mappingParams.slice(";", false, possibleParamNames);
    for (const auto paramNameStr : possibleParamNames)
    {
        const auto paramName = StringID::Find(paramNameStr);

        if (const auto* param = knownTemplate->findParameter(paramName))
        {
            if (param->queryDataType() == TYPE_OF(value))
            {
                if (WriteVector4(value, paramName, outParams))
                    return true;
            }
        }
    }

    // not found
    return false;
}

static StaticResource<IMaterial> resUnlitMaterialBase("/engine/materials/std_unlit.v4mg");
static StaticResource<IMaterial> resDefaultMaterialBase("/engine/materials/std_pbr.v4mg");

MaterialInstancePtr IGeneralMaterialImporter::importMaterial(IResourceImporterInterface& importer, const MaterialImportConfig& config, const GeneralMaterialInfo& sourceMaterial) const
{
    // load textures
    LoadedTextures loadedTextures;
    loadedTextures.forceUnlit = sourceMaterial.forceUnlit;
    for (uint32_t i=0; i<GeneralMaterialTextureType_MAX; ++i)
        loadTexture(importer, config, sourceMaterial.textures[i], loadedTextures.textures[i]);

    // if diffuse was not loaded try to load variants of it
    if (!loadedTextures.textures[GeneralMaterialTextureType_Diffuse] && sourceMaterial.textures[GeneralMaterialTextureType_Diffuse].path)
    {
        if (!tryLoadAlternativeTexture(importer, config, sourceMaterial.textures[GeneralMaterialTextureType_Diffuse], config.m_postfixColor, loadedTextures.textures[GeneralMaterialTextureType_Diffuse]))
            tryLoadAlternativeTexture(importer, config, sourceMaterial.textures[GeneralMaterialTextureType_Diffuse], config.m_postfixColorMask, loadedTextures.textures[GeneralMaterialTextureType_DiffuseMask]);
    }

    // auto extend textures
    if (config.m_autoDiscoverTextures)
        loadAdditionalTextures(importer, config, loadedTextures);

    // load the base template to use, this can't fail - otherwise what's the point
    const auto baseMaterial = loadedTextures.forceUnlit ? resUnlitMaterialBase.load() : resDefaultMaterialBase.load();
    if (!baseMaterial)
        return nullptr;

    // we must have a valid template
    const auto baseTemplate = baseMaterial.resource() ? baseMaterial.resource()->resolveTemplate() : nullptr;
    if (!baseTemplate)
        return nullptr;

    // gather parameters
    Array<MaterialInstanceParam> importedParameters;
    importedParameters.reserve(8);

    // apply standard textures
    bool usedDiffuseMask = false;
    if (!tryApplyTexture(importer, config, config.m_bindingMapColor, baseTemplate, loadedTextures.textures[GeneralMaterialTextureType_Diffuse], importedParameters))
        usedDiffuseMask = tryApplyTexture(importer, config, config.m_bindingMapColor, baseTemplate, loadedTextures.textures[GeneralMaterialTextureType_DiffuseMask], importedParameters);

    bool useNormalSpecularity = false;
    if (!tryApplyTexture(importer, config, config.m_bindingMapNormal, baseTemplate, loadedTextures.textures[GeneralMaterialTextureType_Normal], importedParameters))
        useNormalSpecularity = tryApplyTexture(importer, config, config.m_bindingMapNormal, baseTemplate, loadedTextures.textures[GeneralMaterialTextureType_NormalSpecularity], importedParameters);

    tryApplyTexture(importer, config, config.m_bindingMapBump, baseTemplate, loadedTextures.textures[GeneralMaterialTextureType_Bump], importedParameters);

    // masking
    if (tryApplyTexture(importer, config, config.m_bindingMapMask, baseTemplate, loadedTextures.textures[GeneralMaterialTextureType_Mask], importedParameters))
    {
        applyBoolParam("UseMaskMap", baseTemplate, true, importedParameters);
        applyBoolParam("UseMasking", baseTemplate, true, importedParameters);
    }
    else if (usedDiffuseMask)
    {
        applyBoolParam("UseMasking", baseTemplate, true, importedParameters);
    }

    // emissive
    if (tryApplyTexture(importer, config, config.m_bindingMapEmissive, baseTemplate, loadedTextures.textures[GeneralMaterialTextureType_Emissive], importedParameters))
        applyBoolParam("UseEmissiveMap", baseTemplate, true, importedParameters);

    // AO
    if (tryApplyTexture(importer, config, config.m_bindingMapAmbientOcclusion, baseTemplate, loadedTextures.textures[GeneralMaterialTextureType_AO], importedParameters))
        applyBoolParam("UseAmbientOcclusionMap", baseTemplate, true, importedParameters);

    // PBR combined texture
    if (tryApplyTexture(importer, config, config.m_bindingMapMetallic, baseTemplate, loadedTextures.textures[GeneralMaterialTextureType_MetallicAOSpecularSmoothness], importedParameters))
    {
        applyBoolParam("UseMetallicMap", baseTemplate, true, importedParameters);
        applyVector4Param("MetallicColorMixer", baseTemplate, Vector4(1, 0, 0, 0), importedParameters);

        if (tryApplyTexture(importer, config, config.m_bindingMapAmbientOcclusion, baseTemplate, loadedTextures.textures[GeneralMaterialTextureType_MetallicAOSpecularSmoothness], importedParameters))
        {
            applyBoolParam("UseAmbientOcclusionMap", baseTemplate, true, importedParameters);
            applyVector4Param("AmbientOcclusionColorMixer", baseTemplate, Vector4(0, 1, 0, 0), importedParameters);
        }

        if (tryApplyTexture(importer, config, config.m_bindingMapSpecular, baseTemplate, loadedTextures.textures[GeneralMaterialTextureType_MetallicAOSpecularSmoothness], importedParameters))
        {
            applyBoolParam("UseSpecularMap", baseTemplate, true, importedParameters);
            applyVector4Param("SpecularColorMixer", baseTemplate, Vector4(0, 0, 1, 0), importedParameters);
        }

        if (tryApplyTexture(importer, config, config.m_bindingMapRoughness, baseTemplate, loadedTextures.textures[GeneralMaterialTextureType_MetallicAOSpecularSmoothness], importedParameters))
        {
            applyBoolParam("UseRoughnessMap", baseTemplate, true, importedParameters);
            applyBoolParam("UseInvertedRoughness", baseTemplate, true, importedParameters);
            applyVector4Param("RoughnessColorMixer", baseTemplate, Vector4(0, 0, 0, 1), importedParameters);
        }
    }
    else
    {
        // PBR Roughness-Specularity textures
        if (useNormalSpecularity)
        {
            applyBoolParam("UseSpecularMap", baseTemplate, false, importedParameters); // in alpha channel of normal map

            if (tryApplyTexture(importer, config, config.m_bindingMapRoughness, baseTemplate, loadedTextures.textures[GeneralMaterialTextureType_Roughness], importedParameters))
                applyBoolParam("UseRoughnessMap", baseTemplate, true, importedParameters);
        }
        else if (tryApplyTexture(importer, config, config.m_bindingMapSpecular, baseTemplate, loadedTextures.textures[GeneralMaterialTextureType_RoughnessSpecularity], importedParameters))
        {
            applyBoolParam("UseSpecularMap", baseTemplate, true, importedParameters);
            applyBoolParam("UseRoughnessMap", baseTemplate, true, importedParameters);
            applyVector4Param("RoughnessColorMixer", baseTemplate, Vector4(0, 0, 0, 1), importedParameters); // roughness is in alpha
        }
        else
        {
            if (tryApplyTexture(importer, config, config.m_bindingMapRoughness, baseTemplate, loadedTextures.textures[GeneralMaterialTextureType_Roughness], importedParameters))
                applyBoolParam("UseRoughnessMap", baseTemplate, true, importedParameters);

            if (tryApplyTexture(importer, config, config.m_bindingMapSpecular, baseTemplate, loadedTextures.textures[GeneralMaterialTextureType_Specularity], importedParameters))
                applyBoolParam("UseSpecularMap", baseTemplate, true, importedParameters);
        }

        // PBR Metallicity
        if (tryApplyTexture(importer, config, config.m_bindingMapMetallic, baseTemplate, loadedTextures.textures[GeneralMaterialTextureType_Metallic], importedParameters))
            applyBoolParam("UseMetallicMap", baseTemplate, true, importedParameters);
    }

    // build the "imported material"
    auto importedMaterial = RefNew<MaterialInstance>(baseMaterial, std::move(importedParameters));

    // load existing material
    const auto* existingMaterial = rtti_cast<MaterialInstance>(importer.existingData());

    // get the "overlay" user parameters from existing material data
    Array<MaterialInstanceParam> userParameters;
    if (existingMaterial)
    {
        // NOTE: copy only parameters that are actually changed
        for (const auto& param : existingMaterial->parameters())
            if (existingMaterial->checkParameterOverride(param.name))
                userParameters.emplaceBack(param);
    }

    // build final material that uses the user parameters on top of the imported base material
    return RefNew<MaterialInstance>(baseMaterial, std::move(userParameters), importedMaterial);
}

//--

END_BOOMER_NAMESPACE_EX(assets)
