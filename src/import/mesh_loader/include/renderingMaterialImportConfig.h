/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: import\config #]
***/

#pragma once

#include "core/resource/include/resource.h"
#include "core/resource/include/metadata.h"
#include "core/resource_compiler/include/importInterface.h"

BEGIN_BOOMER_NAMESPACE_EX(assets)

//---

/// common manifest for importable materials 
class IMPORT_MESH_LOADER_API MaterialImportConfig : public ResourceConfiguration
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialImportConfig, ResourceConfiguration);

public:
    MaterialImportConfig();

    //--

    // how to import textures
    bool m_importTextures = true;

    // texture search path, relative to asset, typically ./textures
    StringBuf m_textureSearchPath;

    // if not found texture is imported here using only it's file name
    StringBuf m_textureImportPath;

    //--

    // search depth when looking file up in depot
    int m_depotSearchDepth = 6;

    // search depth when looking file up in source asset repository
    int m_sourceAssetsSearchDepth = 6;

    //--

    // auto discover additional textures (ie. infer bricks_N.png for NormalMap from having bricks_D.png for diffuse)
    bool m_autoDiscoverTextures = true;

    //--

    StringBuf m_bindingColor;
    StringBuf m_bindingMapColor;
    StringBuf m_bindingMapBump;
    StringBuf m_bindingMapNormal;
    StringBuf m_bindingMapMask;
    StringBuf m_bindingMapSpecular;
    StringBuf m_bindingMapEmissive;
    StringBuf m_bindingMapRoughness;
    StringBuf m_bindingMapMetallic;
    StringBuf m_bindingMapAmbientOcclusion;
        
    StringBuf m_postfixColor;
    StringBuf m_postfixColorMask;
    StringBuf m_postfixNormal;
    StringBuf m_postfixBump;
    StringBuf m_postfixMask;
    StringBuf m_postfixSpecular;
    StringBuf m_postfixEmissive;
    StringBuf m_postfixRoughness;
    StringBuf m_postfixNormalSpecularity;
    StringBuf m_postfixRoughnessSpecularity;
    StringBuf m_postfixMetallic;
    StringBuf m_postfixAmbientOcclusion;
    StringBuf m_postfixMetallicAOSpecularSmoothness;
};

//---

struct GeneralMaterialTextrureInfo
{
    StringBuf path;

    INLINE operator bool() const { return !path.empty(); }
};

enum GeneralMaterialTextureType
{
    GeneralMaterialTextureType_Diffuse,
    GeneralMaterialTextureType_DiffuseMask,
    GeneralMaterialTextureType_Normal,
    GeneralMaterialTextureType_Bump,
    GeneralMaterialTextureType_Roughness,
    GeneralMaterialTextureType_Specularity,
    GeneralMaterialTextureType_NormalSpecularity,
    GeneralMaterialTextureType_RoughnessSpecularity,
    GeneralMaterialTextureType_Mask,
    GeneralMaterialTextureType_Metallic,
    GeneralMaterialTextureType_Emissive,
    GeneralMaterialTextureType_AO,
    GeneralMaterialTextureType_MetallicAOSpecularSmoothness,

    GeneralMaterialTextureType_MAX,
};

struct GeneralMaterialInfo
{
    bool forceUnlit = false;

    Color color;

    GeneralMaterialTextrureInfo textures[GeneralMaterialTextureType_MAX];
};

struct LoadedTexture;
struct LoadedTextures;

/// generic material importer
class IMPORT_MESH_LOADER_API IGeneralMaterialImporter : public IResourceImporter
{
    RTTI_DECLARE_VIRTUAL_CLASS(IGeneralMaterialImporter, IResourceImporter);

public:
    virtual ~IGeneralMaterialImporter();

protected:
    static StringView ExtractRootFileName(StringView assetFileName);
    static StringBuf ConvertPathToTextureSearchPath(StringView assetPathToTexture);
    static void GlueDepotPath(StringView path, bool isFileName, Array<StringView>& outPathParts);
    static void EmitDepotPath(const Array<StringView>& pathParts, IFormatStream& f);
    static StringBuf BuildTextureDepotPath(StringView referenceDepotPath, StringView textureImportPath, StringView assetFileName);

    static bool WriteTexture(const TextureRef& textureRef, StringID name, Array<MaterialInstanceParam>& outParams);
    static bool WriteBool(bool value, StringID name, Array<MaterialInstanceParam>& outParams);
    static bool WriteVector4(Vector4 value, StringID name, Array<MaterialInstanceParam>& outParams);

    virtual bool tryApplyTexture(IResourceImporterInterface& importer, const MaterialImportConfig& config, StringView mappingParams, const MaterialTemplate* knownTemplate, const LoadedTexture& texture, Array<MaterialInstanceParam>& outParams) const;

    bool applyBoolParam(StringView paramNames, const MaterialTemplate* knownTemplate, bool value, Array<MaterialInstanceParam>& outParams) const;
    bool applyVector4Param(StringView paramNames, const MaterialTemplate* knownTemplate, Vector4 value, Array<MaterialInstanceParam>& outParams) const;
        
    bool loadTexture(IResourceImporterInterface& importer, const MaterialImportConfig& config, const GeneralMaterialTextrureInfo& info, LoadedTexture& outLoadedTextures) const;

    bool tryLoadAlternativeTexture(IResourceImporterInterface& importer, const MaterialImportConfig& config, const GeneralMaterialTextrureInfo& info, StringView additionalSuffixes, LoadedTexture& outLoadedTextures) const;

    bool findAssetSourcePath(IResourceImporterInterface& importer, const Array<StringBuf>& paths, const Array<StringView>& suffixes, StringBuf& outPath) const;

    void loadAdditionalTextures(IResourceImporterInterface& importer, const MaterialImportConfig& config, LoadedTextures& outLoadedTextures) const;

    virtual MaterialInstancePtr importMaterial(IResourceImporterInterface& importer, const MaterialImportConfig& config, const GeneralMaterialInfo& sourceMaterial) const;

    virtual TextureRef importTextureRef(IResourceImporterInterface& importer, const MaterialImportConfig& csg, StringView assetPathToTexture, StringBuf& outAssetImportPath) const;
};

//---

END_BOOMER_NAMESPACE_EX(assets)
