/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\debug #]
***/

#pragma once

#include "core/object/include/object.h"
#include "core/containers/include/rectAllocator.h"

BEGIN_BOOMER_NAMESPACE()

///--

/// icon for use with debug geometry
class ENGINE_RENDERING_API DebugGeometryImage : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(DebugGeometryImage, IObject);

public:
    DebugGeometryImage(Image* data);
    DebugGeometryImage(StringView depotPath);
    virtual ~DebugGeometryImage();

    INLINE uint32_t width() const { return m_width; }
    INLINE uint32_t height() const { return m_height; }

    INLINE AtlasImageID id() const { return m_id; }

private:
    AtlasImageID m_id = 0;

    uint32_t m_width = 0;
    uint32_t m_height = 0;

    DynamicImageAtlasEntryPtr m_entry;
};

//---

/// asset service for the debug geometry, handles built-in bitmap font and icons
class ENGINE_RENDERING_API DebugGeometryAssetService : public IService
{
    RTTI_DECLARE_VIRTUAL_CLASS(DebugGeometryAssetService, IService);

public:
    DebugGeometryAssetService();
    virtual ~DebugGeometryAssetService();

    //--

    INLINE const DynamicGlyphAtlasPtr& glyphAtlas() const { return m_glyphAtlas; }

    INLINE const DynamicImageAtlasPtr& imageAtlas() const { return m_imageAtlas; }

    //--

    // send any pending updates to the resources down the stream
    void flushUpdates(gpu::CommandWriter& cmd);

    //--
  
private:
    virtual bool onInitializeService(const CommandLine& cmdLine) override;
    virtual void onShutdownService() override;
    virtual void onSyncUpdate() override;

    //--

    DynamicGlyphAtlasPtr m_glyphAtlas;
    DynamicImageAtlasPtr m_imageAtlas;

    //--
};

//---

END_BOOMER_NAMESPACE()
