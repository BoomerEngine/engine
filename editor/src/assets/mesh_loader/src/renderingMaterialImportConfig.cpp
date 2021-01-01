/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: import\config #]
***/

#include "build.h"
#include "renderingMaterialImportConfig.h"

#include "rendering/texture/include/renderingStaticTexture.h"
#include "rendering/material/include/renderingMaterial.h"
#include "base/resource_compiler/include/importInterface.h"

namespace rendering
{
    //--

    static base::res::StaticResource<rendering::IMaterial> resDefaultMaterialBase("/engine/materials/std_pbr.v4mg");

    //--

    RTTI_BEGIN_TYPE_ENUM(MaterialTextureImportMode);
        RTTI_ENUM_OPTION(DontImport);
        RTTI_ENUM_OPTION(FindOnly);
        RTTI_ENUM_OPTION(ImportAll);
        RTTI_ENUM_OPTION(ImportMissing);
    RTTI_END_TYPE();

    //--

    RTTI_BEGIN_TYPE_CLASS(MaterialImportConfig);
        RTTI_CATEGORY("Texture import");
        RTTI_PROPERTY(m_textureImportMode).editable().overriddable();
        RTTI_PROPERTY(m_textureSearchPath).editable().overriddable();
        RTTI_PROPERTY(m_textureImportPath).editable().overriddable();
        RTTI_CATEGORY("Asset search");
        RTTI_PROPERTY(m_depotSearchDepth).editable().overriddable();
        RTTI_PROPERTY(m_sourceAssetsSearchDepth).editable().overriddable();
        RTTI_CATEGORY("Base material");
        RTTI_PROPERTY(m_templateDefault).editable().overriddable();
    RTTI_END_TYPE();

    MaterialImportConfig::MaterialImportConfig()
    {
        m_textureImportMode = MaterialTextureImportMode::ImportAll;
        m_textureImportPath = StringBuf("../textures/");
        m_textureSearchPath = StringBuf("../textures/");

        m_depotSearchDepth = 6;
        m_sourceAssetsSearchDepth = 6;

        m_templateDefault = resDefaultMaterialBase.asyncRef();
    }

    void MaterialImportConfig::computeConfigurationKey(CRC64& crc) const
    {
        TBaseClass::computeConfigurationKey(crc);
        crc << (int)m_textureImportMode;
        crc << m_textureImportPath.view();
        crc << m_textureSearchPath.view();
        crc << m_depotSearchDepth;
        crc << m_sourceAssetsSearchDepth;
        crc << m_templateDefault.key().path().view();
    }

    //--


    static base::StringView ExtractRootFileName(base::StringView assetFileName)
    {
        auto name = assetFileName.afterLastOrFull("/").afterLastOrFull("\\");
        return name.beforeFirstOrFull(".");
    }

    static base::StringBuf ConvertPathToTextureSearchPath(base::StringView assetPathToTexture)
    {
        base::StringBuf safeCoreName;
        const auto coreName = ExtractRootFileName(assetPathToTexture);
        if (!MakeSafeFileName(coreName, safeCoreName))
            return base::StringBuf();

        static const auto textureExtension = base::res::IResource::GetResourceExtensionForClass(StaticTexture::GetStaticClass());
        return base::TempString("{}.{}", safeCoreName, textureExtension);
    }

    static void GlueDepotPath(base::StringView path, bool isFileName, base::Array<base::StringView>& outPathParts)
    {
        base::InplaceArray<base::StringView, 10> pathParts;
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

    static void EmitDepotPath(const base::Array<base::StringView>& pathParts, base::IFormatStream& f)
    {
        f << "/";

        for (const auto part : pathParts)
        {
            f << part;
            f << "/";
        }
    }

    static base::StringBuf BuildTextureDepotPath(base::StringView referenceDepotPath, base::StringView textureImportPath, base::StringView assetFileName)
    {
        base::InplaceArray<base::StringView, 20> pathParts;
        GlueDepotPath(referenceDepotPath, true, pathParts);
        GlueDepotPath(textureImportPath, false, pathParts);

        StringBuilder txt;
        EmitDepotPath(pathParts, txt);

        txt << ConvertPathToTextureSearchPath(assetFileName);

        return txt.toString();
    }

    rendering::TextureRef ImportTextureRef(base::res::IResourceImporterInterface& importer, const MaterialImportConfig& cfg, base::StringView assetPathToTexture)
    {
        // no path
        if (assetPathToTexture.empty())
            return rendering::TextureRef();

        // can we find texture
        if (cfg.m_textureImportMode == MaterialTextureImportMode::FindOnly || cfg.m_textureImportMode == MaterialTextureImportMode::ImportMissing)
        {        
            // get the findable texture path - ie. strip all nonsense from the asset path, especially the ".." or drive letters as they are not used during search
            const auto findableName = ConvertPathToTextureSearchPath(assetPathToTexture);
            TRACE_INFO("Looking for texture '{}' (imported from '{}')...", findableName, assetPathToTexture);

            // look for a depot file in existing depot
            const auto depotScanDepth = std::clamp<int>(cfg.m_depotSearchDepth, 0, 20);
            base::StringBuf depotPath;
            if (importer.findDepotFile(importer.queryResourcePath().view(), cfg.m_textureSearchPath, findableName, depotPath, depotScanDepth))
            {
                TRACE_INFO("Found '{}' at '{}'", assetPathToTexture, depotPath);

                base::res::BaseReference textureRef(base::res::ResourceKey(base::res::ResourcePath(depotPath), StaticTexture::GetStaticClass()));
                return textureRef.cast<StaticTexture>();
            }
        }

        // if nothing was found, try the import
        if (cfg.m_textureImportMode == MaterialTextureImportMode::ImportAll || cfg.m_textureImportMode == MaterialTextureImportMode::ImportMissing)
        {
            // search in the vicinity of source asset
            base::StringBuf foundTexturePath;
            const auto assetDepotScanDepth = std::clamp<int>(cfg.m_sourceAssetsSearchDepth, 0, 20);
            if (importer.findSourceFile(importer.queryImportPath(), assetPathToTexture, foundTexturePath, assetDepotScanDepth))
            {
                // build depot path for the imported texture
                const auto depotPath = BuildTextureDepotPath(importer.queryResourcePath().view(), cfg.m_textureImportPath, foundTexturePath);
                TRACE_INFO("Texture '{}' found at '{}' will be improted as '{}'", assetPathToTexture, foundTexturePath, depotPath);

                // emit the follow-up import, no extra config at the moment
                importer.followupImport(foundTexturePath, depotPath);

                // build a unloaded texture reference (so it can be saved)
                base::res::BaseReference textureRef(base::res::ResourceKey(base::res::ResourcePath(depotPath), StaticTexture::GetStaticClass()));
                return textureRef.cast<StaticTexture>();
            }
        }

        // no texture can be imported
        TRACE_WARNING("No texture will be bound despite having valid path '{}'", assetPathToTexture);
        return nullptr;
    }

    //--

} // rendering
