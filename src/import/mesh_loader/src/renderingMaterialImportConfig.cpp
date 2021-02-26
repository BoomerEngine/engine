/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: import\config #]
***/

#include "build.h"
#include "renderingMaterialImportConfig.h"

#include "engine/texture/include/renderingStaticTexture.h"
#include "engine/material/include/renderingMaterial.h"
#include "engine/material/include/renderingMaterialInstance.h"

#include "core/resource_compiler/include/importInterface.h"
#include "engine/material/include/renderingMaterialTemplate.h"

BEGIN_BOOMER_NAMESPACE_EX(assets)

//--

RTTI_BEGIN_TYPE_CLASS(MaterialImportConfig);
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
    RTTI_PROPERTY(m_bindingMapColorMask).editable().overriddable();
    RTTI_PROPERTY(m_bindingMapBump).editable().overriddable();
    RTTI_PROPERTY(m_bindingMapNormal).editable().overriddable();
    RTTI_PROPERTY(m_bindingMapDissolve).editable().overriddable();
    RTTI_PROPERTY(m_bindingMapSpecular).editable().overriddable();
    RTTI_PROPERTY(m_bindingMapEmissive).editable().overriddable();
    RTTI_PROPERTY(m_bindingMapRoughness).editable().overriddable();
    RTTI_PROPERTY(m_bindingMapRoughnessSpecularity).editable().overriddable();
    RTTI_PROPERTY(m_bindingMapNormalSpecularity).editable().overriddable();
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
RTTI_END_TYPE();

MaterialImportConfig::MaterialImportConfig()
{
    m_textureImportPath = StringBuf("../textures/");
    m_textureSearchPath = StringBuf("../textures/");

    m_bindingColor = "Color";

    m_bindingMapColor = "ColorMap;BaseColorMap;AlbedoMap";
    m_bindingMapColorMask = "ColorMap;BaseColorMap;AlbedoMap;MaskMap";
    m_bindingMapBump = "BumpMap;NormalMap";
    m_bindingMapNormal = "NormalMap";
    m_bindingMapDissolve = "MaskMap";
    m_bindingMapSpecular = "SpecularMap";
    m_bindingMapEmissive = "EmissiveMap";
    m_bindingMapRoughness = "RoughnessMap";
    m_bindingMapRoughnessSpecularity = "RoughnessSpecularMap";
    m_bindingMapNormalSpecularity = "NormalSpecularMap";
    m_bindingMapMetallic = "MetallicMap";
    m_bindingMapAmbientOcclusion = "AmbientOcclusionMap;AOMap";

    m_postfixColor = "diffuse;albedo;diff;d;D";
    m_postfixColorMask = "diffuseMask;DM;dm";
    m_postfixNormal = "normal;ddna;n;N";
    m_postfixBump = "bump";
    m_postfixMask = "mask;M";
    m_postfixSpecular = "spec;s;S";
    m_postfixRoughness = "roughness;R";
    m_postfixRoughnessSpecularity = "rs;RS";
    m_postfixNormalSpecularity = "ns;NS";
    m_postfixEmissive = "emissive;E";
    m_postfixMetallic = "metallic;MT";
    m_postfixAmbientOcclusion = "ao;AO";
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
    crc << m_bindingMapColorMask.view();
    crc << m_bindingMapBump.view();
    crc << m_bindingMapNormal.view();
    crc << m_bindingMapDissolve.view();
    crc << m_bindingMapSpecular.view();
    crc << m_bindingMapEmissive.view();
    crc << m_bindingMapRoughness.view();
    crc << m_bindingMapRoughnessSpecularity.view();
    crc << m_bindingMapNormalSpecularity.view();
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

    static const auto textureExtension = res::IResource::GetResourceExtensionForClass(StaticTexture::GetStaticClass());
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

TextureRef IGeneralMaterialImporter::importTextureRef(res::IResourceImporterInterface& importer, const MaterialImportConfig& cfg, StringView assetPathToTexture, StringBuf& outAssetImportPath) const
{
    // no path
    if (assetPathToTexture.empty())
        return TextureRef();

    // not imported
    if (!cfg.m_importTextures)
        return TextureRef();

    // crap
    StringBuf tempAssetPathToTexture;
    if (assetPathToTexture.endsWithNoCase(".tif") || assetPathToTexture.endsWithNoCase(".tiff"))
    {
        tempAssetPathToTexture = TempString("{}{}.tga", assetPathToTexture.baseDirectory(), assetPathToTexture.fileStem());
        assetPathToTexture = tempAssetPathToTexture;
    }

    // try to find texture in depot if the search path was specified
    if (cfg.m_textureSearchPath)
    {        
        // get the findable texture path - ie. strip all nonsense from the asset path, especially the ".." or drive letters as they are not used during search
        const auto findableName = ConvertPathToTextureSearchPath(assetPathToTexture);
        TRACE_INFO("Looking for texture '{}' (imported from '{}')...", findableName, assetPathToTexture);

        // look for a depot file in existing depot
        const auto depotScanDepth = std::clamp<int>(cfg.m_depotSearchDepth, 0, 20);
        StringBuf depotPath;
        if (importer.findDepotFile(importer.queryResourcePath().view(), cfg.m_textureSearchPath, findableName, depotPath, depotScanDepth))
        {
            TRACE_INFO("Found '{}' at '{}'", assetPathToTexture, depotPath);

            return TextureRef(res::ResourcePath(depotPath));
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
            importer.followupImport(foundTexturePath, depotPath);

            // export the asset import path
            outAssetImportPath = foundTexturePath;

            // build a unloaded texture reference (so it can be saved)
            return TextureRef(res::ResourcePath(depotPath));
        }
    }

    // no texture can be imported
    TRACE_WARNING("No texture will be bound despite having valid path '{}'", assetPathToTexture);
    return nullptr;
}

static res::StaticResource<IMaterial> resUnlitMaterialBase("/engine/materials/std_unlit.v4mg");

static res::StaticResource<IMaterial> resDefaultMaterialBase("/engine/materials/std_pbr.v4mg");
static res::StaticResource<IMaterial> resMaskedMaterialBase("/engine/materials/std_pbr_masked.v4mg");
static res::StaticResource<IMaterial> resEmissiveMaterialBase("/engine/materials/std_pbr_emissive.v4mg");

static res::StaticResource<IMaterial> resDefaultMaterialNS("/engine/materials/std_pbr_ns.v4mg");
static res::StaticResource<IMaterial> resMaskedMaterialNS("/engine/materials/std_pbr_masked_ns.v4mg");
static res::StaticResource<IMaterial> resEmissiveMaterialNS("/engine/materials/std_pbr_emissive_ns.v4mg");

static res::StaticResource<IMaterial> resDefaultMaterialRS("/engine/materials/std_pbr_rs.v4mg");
static res::StaticResource<IMaterial> resMaskedMaterialRS("/engine/materials/std_pbr_masked_rs.v4mg");
static res::StaticResource<IMaterial> resEmissiveMaterialRS("/engine/materials/std_pbr_emissive_rs.v4mg");


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

MaterialRef IGeneralMaterialImporter::loadBaseMaterial(const LoadedTextures& info, const MaterialImportConfig& config) const
{
    MaterialRef ret;

    if (info.forceUnlit)
        return resUnlitMaterialBase.loadAndGetAsRef();

    if (info.textures[GeneralMaterialTextureType_Emissive])
    {
        if (info.textures[GeneralMaterialTextureType_NormalSpecularity])
            return resEmissiveMaterialNS.loadAndGetAsRef();
        else if (info.textures[GeneralMaterialTextureType_RoughnessSpecularity])
            return resEmissiveMaterialRS.loadAndGetAsRef();
        else
            return resEmissiveMaterialBase.loadAndGetAsRef();
    }

    if (info.textures[GeneralMaterialTextureType_Mask])
    {
        if (info.textures[GeneralMaterialTextureType_NormalSpecularity])
            return resMaskedMaterialNS.loadAndGetAsRef();
        else if (info.textures[GeneralMaterialTextureType_RoughnessSpecularity])
            return resMaskedMaterialRS.loadAndGetAsRef();
        else
            return resMaskedMaterialBase.loadAndGetAsRef();
    }

    if (info.textures[GeneralMaterialTextureType_NormalSpecularity])
        return resDefaultMaterialNS.loadAndGetAsRef();
    else if (info.textures[GeneralMaterialTextureType_RoughnessSpecularity])
        return resDefaultMaterialRS.loadAndGetAsRef();
    else
        return resDefaultMaterialBase.loadAndGetAsRef();
}

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

bool IGeneralMaterialImporter::loadTexture(res::IResourceImporterInterface& importer, const MaterialImportConfig& config, const GeneralMaterialTextrureInfo& info, LoadedTexture& outLoadedTexture) const
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

static void ExtractPathsAndStems(StringView importPath, Array<StringBuf>& outImportPaths)
{
    if (importPath)
        outImportPaths.pushBackUnique(StringBuf(importPath));
}

bool IGeneralMaterialImporter::findAssetSourcePath(res::IResourceImporterInterface& importer, const Array<StringBuf>& paths, const Array<StringView>& suffixes, StringBuf& outPath) const
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

void IGeneralMaterialImporter::loadAdditionalTextures(res::IResourceImporterInterface& importer, const MaterialImportConfig& config, LoadedTextures& outLoadedTextures) const
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

bool IGeneralMaterialImporter::tryApplyTexture(res::IResourceImporterInterface& importer, const MaterialImportConfig& config, StringView mappingParams, const MaterialTemplate* knownTemplate, const LoadedTexture& texture, Array<MaterialInstanceParam>& outParams) const
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

        MaterialTemplateParamInfo materialParam;
        if (knownTemplate->queryParameterInfo(paramName, materialParam))
        {
            if (materialParam.type == TYPE_OF(texture.textureRef))
            {
                if (WriteTexture(texture.textureRef, paramName, outParams))
                    return true;
            }

            TRACE_WARNING("Unable to bind texture to parameter '{}' from template '{}' as its not a texture parameter", paramName, knownTemplate->path());
        }
    }

    // not found
    return false;
}

MaterialInstancePtr IGeneralMaterialImporter::importMaterial(res::IResourceImporterInterface& importer, const MaterialImportConfig& config, const GeneralMaterialInfo& sourceMaterial) const
{
    // load textures
    LoadedTextures loadedTextures;
    loadedTextures.forceUnlit = sourceMaterial.forceUnlit;
    for (uint32_t i=0; i<GeneralMaterialTextureType_MAX; ++i)
        loadTexture(importer, config, sourceMaterial.textures[i], loadedTextures.textures[i]);

    // auto extend textures
    if (config.m_autoDiscoverTextures)
        loadAdditionalTextures(importer, config, loadedTextures);

    // load the base template to use, this can't fail - otherwise what's the point
    const auto baseMaterial = loadBaseMaterial(loadedTextures, config);
    if (!baseMaterial)
        return nullptr;

    // we must have a valid template
    const auto baseTemplate = baseMaterial.load() ? baseMaterial.load()->resolveTemplate() : nullptr;
    if (!baseTemplate)
        return nullptr;

    // gather parameters
    Array<MaterialInstanceParam> importedParameters;
    importedParameters.reserve(8);

    // apply properties
    tryApplyTexture(importer, config, config.m_bindingMapColor, baseTemplate, loadedTextures.textures[GeneralMaterialTextureType_Diffuse], importedParameters);
    tryApplyTexture(importer, config, config.m_bindingMapColorMask, baseTemplate, loadedTextures.textures[GeneralMaterialTextureType_DiffuseMask], importedParameters);
    tryApplyTexture(importer, config, config.m_bindingMapNormal, baseTemplate, loadedTextures.textures[GeneralMaterialTextureType_Normal], importedParameters);
    tryApplyTexture(importer, config, config.m_bindingMapNormalSpecularity, baseTemplate, loadedTextures.textures[GeneralMaterialTextureType_NormalSpecularity], importedParameters);
    tryApplyTexture(importer, config, config.m_bindingMapBump, baseTemplate, loadedTextures.textures[GeneralMaterialTextureType_Bump], importedParameters);
    tryApplyTexture(importer, config, config.m_bindingMapRoughnessSpecularity, baseTemplate, loadedTextures.textures[GeneralMaterialTextureType_RoughnessSpecularity], importedParameters);
    tryApplyTexture(importer, config, config.m_bindingMapRoughness, baseTemplate, loadedTextures.textures[GeneralMaterialTextureType_Roughness], importedParameters);
    tryApplyTexture(importer, config, config.m_bindingMapSpecular, baseTemplate, loadedTextures.textures[GeneralMaterialTextureType_Specularity], importedParameters);
    tryApplyTexture(importer, config, config.m_bindingMapDissolve, baseTemplate, loadedTextures.textures[GeneralMaterialTextureType_Mask], importedParameters);
    tryApplyTexture(importer, config, config.m_bindingMapMetallic, baseTemplate, loadedTextures.textures[GeneralMaterialTextureType_Metallic], importedParameters);
    tryApplyTexture(importer, config, config.m_bindingMapEmissive, baseTemplate, loadedTextures.textures[GeneralMaterialTextureType_Emissive], importedParameters);
    tryApplyTexture(importer, config, config.m_bindingMapAmbientOcclusion, baseTemplate, loadedTextures.textures[GeneralMaterialTextureType_AO], importedParameters);

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
