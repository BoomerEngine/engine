/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: import\config #]
***/

#pragma once

#include "base/resource/include/resource.h"
#include "base/resource/include/resourceMetadata.h"
#include "base/resource_compiler/include/importInterface.h"

BEGIN_BOOMER_NAMESPACE(rendering)

//---

/// common manifest for importable materials 
class IMPORT_MESH_LOADER_API MaterialImportConfig : public base::res::ResourceConfiguration
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialImportConfig, base::res::ResourceConfiguration);

public:
    MaterialImportConfig();

    //--

    // how to import textures
    bool m_importTextures = true;

    // texture search path, relative to asset, typically ./textures
    base::StringBuf m_textureSearchPath;

    // if not found texture is imported here using only it's file name
    base::StringBuf m_textureImportPath;

    //--

    // search depth when looking file up in depot
    int m_depotSearchDepth = 6;

    // search depth when looking file up in source asset repository
    int m_sourceAssetsSearchDepth = 6;

    //--

    // auto discover additional textures (ie. infer bricks_N.png for NormalMap from having bricks_D.png for diffuse)
    bool m_autoDiscoverTextures = true;

    //--

    base::StringBuf m_bindingColor;
    base::StringBuf m_bindingMapColor;
    base::StringBuf m_bindingMapColorMask;
    base::StringBuf m_bindingMapBump;
    base::StringBuf m_bindingMapNormal;
    base::StringBuf m_bindingMapDissolve;
    base::StringBuf m_bindingMapSpecular;
    base::StringBuf m_bindingMapEmissive;
    base::StringBuf m_bindingMapRoughness;
    base::StringBuf m_bindingMapNormalSpecularity;
    base::StringBuf m_bindingMapRoughnessSpecularity;
    base::StringBuf m_bindingMapMetallic;
    base::StringBuf m_bindingMapAmbientOcclusion;
        
    base::StringBuf m_postfixColor;
    base::StringBuf m_postfixColorMask;
    base::StringBuf m_postfixNormal;
    base::StringBuf m_postfixBump;
    base::StringBuf m_postfixMask;
    base::StringBuf m_postfixSpecular;
    base::StringBuf m_postfixEmissive;
    base::StringBuf m_postfixRoughness;
    base::StringBuf m_postfixNormalSpecularity;
    base::StringBuf m_postfixRoughnessSpecularity;
    base::StringBuf m_postfixMetallic;
    base::StringBuf m_postfixAmbientOcclusion;

    //--

    virtual void computeConfigurationKey(base::CRC64& crc) const override;
};

//---

struct GeneralMaterialTextrureInfo
{
    base::StringBuf path;

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

    GeneralMaterialTextureType_MAX,
};

struct GeneralMaterialInfo
{
    bool forceUnlit = false;

    base::Color color;

    GeneralMaterialTextrureInfo textures[GeneralMaterialTextureType_MAX];
};

struct MaterialInstanceParam;
struct LoadedTexture;
struct LoadedTextures;

/// generic material importer
class IMPORT_MESH_LOADER_API IGeneralMaterialImporter : public base::res::IResourceImporter
{
    RTTI_DECLARE_VIRTUAL_CLASS(IGeneralMaterialImporter, base::res::IResourceImporter);

public:
    virtual ~IGeneralMaterialImporter();

protected:
    static base::StringView ExtractRootFileName(base::StringView assetFileName);
    static base::StringBuf ConvertPathToTextureSearchPath(base::StringView assetPathToTexture);
    static void GlueDepotPath(base::StringView path, bool isFileName, base::Array<base::StringView>& outPathParts);
    static void EmitDepotPath(const base::Array<base::StringView>& pathParts, base::IFormatStream& f);
    static base::StringBuf BuildTextureDepotPath(base::StringView referenceDepotPath, base::StringView textureImportPath, base::StringView assetFileName);

    static bool WriteTexture(const TextureRef& textureRef, base::StringID name, base::Array<MaterialInstanceParam>& outParams);

    virtual bool tryApplyTexture(base::res::IResourceImporterInterface& importer, const MaterialImportConfig& config, base::StringView mappingParams, const rendering::MaterialTemplate* knownTemplate, const LoadedTexture& texture, base::Array<MaterialInstanceParam>& outParams) const;
        
    bool loadTexture(base::res::IResourceImporterInterface& importer, const MaterialImportConfig& config, const GeneralMaterialTextrureInfo& info, LoadedTexture& outLoadedTextures) const;

    bool findAssetSourcePath(base::res::IResourceImporterInterface& importer, const base::Array<base::StringBuf>& paths, const base::Array<base::StringView>& suffixes, base::StringBuf& outPath) const;

    void loadAdditionalTextures(base::res::IResourceImporterInterface& importer, const MaterialImportConfig& config, LoadedTextures& outLoadedTextures) const;

    virtual MaterialRef loadBaseMaterial(const LoadedTextures& info, const MaterialImportConfig& config) const;

    virtual MaterialInstancePtr importMaterial(base::res::IResourceImporterInterface& importer, const MaterialImportConfig& config, const GeneralMaterialInfo& sourceMaterial) const;

    virtual TextureRef importTextureRef(base::res::IResourceImporterInterface& importer, const MaterialImportConfig& csg, base::StringView assetPathToTexture, base::StringBuf& outAssetImportPath) const;
};

//---

END_BOOMER_NAMESPACE(rendering)
