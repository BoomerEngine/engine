/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#pragma once

#include "atlas.h"
#include "core/containers/include/hashSet.h"

BEGIN_BOOMER_NAMESPACE()

//--

#pragma pack(push)
#pragma pack(4)
struct GPUDynamicAtlasImageInfo
{
    Vector2 uvMin;
    Vector2 uvScale;
    uint32_t page;
    uint32_t _padding;
};
#pragma pack(pop)

//--

/// abstract dynamic atlas based on some form of images
class ENGINE_ATLAS_API IDynamicAtlas : public IAtlas
{
    RTTI_DECLARE_VIRTUAL_CLASS(IDynamicAtlas, IAtlas);

public:
    IDynamicAtlas(ImageFormat format = ImageFormat::RGBA8_UNORM);
	virtual ~IDynamicAtlas();

    //--

    // push any updates to the atlas to the GPU
    void update(gpu::CommandWriter& cmd);

	//--

	// IAtlas
    virtual bool describe(AtlasImageID id, AtlasImageInfo& outInfo) const override final;

    virtual const gpu::ImageSampledViewPtr& imageSRV() const override final;
    virtual const gpu::BufferStructuredViewPtr& imageEntriesSRV() const override final;

	//--

protected:
    Mutex m_lock;

    AtlasImageID createEntry_NoLock(const Image* data, const Rect& sourceRect, bool wrapU, bool wrapV);
    void removeEntry_NoLock(AtlasImageID id);

private:
    ImageFormat m_format = ImageFormat::RGBA8_UNORM;

    gpu::ImageObjectPtr m_textureArray;
    gpu::ImageSampledViewPtr m_textureArraySRV;

    gpu::BufferObjectPtr m_entryBuffer;
    gpu::BufferStructuredViewPtr m_entryBufferSRV;

	//--

    struct PackingEntry
    {
        ImagePtr data; // may be NULL
		Rect sourceRect; // in the data
        Rect packedRect; // in the atlas
        Rect totalRect; // in the atlas
        int page = -1; // in the atlas
        bool wrapU = false;
        bool wrapV = false;
    };

	//--


    Array<int> m_freeEntryIds;

    Array<PackingEntry> m_entries;
    HashSet<uint32_t> m_entriesUpdateList;

    Array<RectAllocator*> m_pages;

	//--

    bool tryPack_NoLock(PackingEntry& entry);
    bool tryPackUpdates_NoLock(gpu::CommandWriter& cmd, bool& hasChanges);
    void repack_NoLock(gpu::CommandWriter& cmd);

    void uploadImage_NoLock(gpu::CommandWriter& cmd, const PackingEntry& entry) const;
};

//--

END_BOOMER_NAMESPACE()
