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
    class IMPORT_MESH_LOADER_API MaterialImportConfig : public base::res::ResourceConfiguration
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialImportConfig, base::res::ResourceConfiguration);

    public:
        MaterialImportConfig();

        //--

        // how to import textures
        MaterialTextureImportMode m_textureImportMode;

        // texture search path, relative to asset, typically ./textures
        base::StringBuf m_textureSearchPath;

        // if not found texture is imported here using only it's file name
        base::StringBuf m_textureImportPath;

        //--

        // search depth when looking file up in depot
        int m_depotSearchDepth;

        // search depth when looking file up in source asset repository
        int m_sourceAssetsSearchDepth;

        //--

        virtual void computeConfigurationKey(base::CRC64& crc) const override;
    };

    //---

    /// find/load texture
    extern IMPORT_MESH_LOADER_API rendering::TextureRef ImportTextureRef(base::res::IResourceImporterInterface& importer, const MaterialImportConfig& csg, base::StringView assetPathToTexture);

    //---

} // base
