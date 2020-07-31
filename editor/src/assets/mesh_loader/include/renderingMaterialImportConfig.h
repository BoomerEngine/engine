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

namespace rendering
{

    //---

    enum class MaterialTextureImportMode : uint8_t
    {
        DontImport, // do nothing, leaves the texture reference uninitialized
        FindOnly, // only attempt to find textures, do not import anything missing
        ImportAll, // always report textures for importing, does not use the search path
        ImportMissing, // import only the textures that were not found
    };

    //---

    /// common manifest for importable materials 
    class ASSETS_MESH_LOADER_API MaterialImportConfig : public res::ResourceConfiguration
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialImportConfig, res::ResourceConfiguration);

    public:
        MaterialImportConfig();

        //--

        // how to import textures
        MaterialTextureImportMode m_textureImportMode;

        // texture search path, relative to asset, typically ./textures
        StringBuf m_textureSearchPath;

        // if not found texture is imported here using only it's file name
        StringBuf m_textureImportPath;

        //--

        // search depth when looking file up in depot
        int m_depotSearchDepth;

        // search depth when looking file up in source asset repository
        int m_sourceAssetsSearchDepth;

        //--

        // base material to use in case of standard opaque materials
        MaterialAsyncRef m_templateDefault;

        //--

        virtual void computeConfigurationKey(CRC64& crc) const override;
    };

    //---

    /// find/load texture
    extern ASSETS_MESH_LOADER_API rendering::TextureRef ImportTextureRef(base::res::IResourceImporterInterface& importer, const MaterialImportConfig& csg, base::StringView<char> assetPathToTexture);

    //---

} // base
