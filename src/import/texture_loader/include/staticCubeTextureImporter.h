/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: texture #]
***/

#pragma once

#include "staticTextureImporter.h"

BEGIN_BOOMER_NAMESPACE_EX(assets)

//--

/// face setting
struct IMPORT_TEXTURE_LOADER_API StaticTextureCubeCompressionFace
{
    RTTI_DECLARE_VIRTUAL_ROOT_CLASS(StaticTextureCubeCompressionFace);

public:
    StringBuf suffix;
    bool flipX = false;
    bool flipY = false;
    bool swapXY = false;
};

//--

// source "Space" of the cubemap
enum class StaticTextureCubeImportSpace : uint8_t
{
    SpaceYUp,
    SpaceZUp,
};

//--

/// texture compression configuration
class IMPORT_TEXTURE_LOADER_API StaticTextureCubeCompressionConfiguration : public StaticTextureCompressionConfiguration
{
    RTTI_DECLARE_VIRTUAL_CLASS(StaticTextureCubeCompressionConfiguration, StaticTextureCompressionConfiguration);

public:
    StaticTextureCubeCompressionConfiguration();

    StaticTextureCubeImportSpace m_space = StaticTextureCubeImportSpace::SpaceYUp;

    StaticTextureCubeCompressionFace m_facePX;
    StaticTextureCubeCompressionFace m_faceNX;

    StaticTextureCubeCompressionFace m_facePY;
    StaticTextureCubeCompressionFace m_faceNY;

    StaticTextureCubeCompressionFace m_facePZ;
    StaticTextureCubeCompressionFace m_faceNZ;
};

//--

// importer for static textures from images
class IMPORT_TEXTURE_LOADER_API StaticCubeTextureFromImageImporter : public IResourceImporter
{
    RTTI_DECLARE_VIRTUAL_CLASS(StaticCubeTextureFromImageImporter, IResourceImporter);

public:
    StaticCubeTextureFromImageImporter();

    virtual ResourcePtr importResource(IResourceImporterInterface& importer) const override final;
};

//--

END_BOOMER_NAMESPACE_EX(assets)
