/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: texture #]
***/

#pragma once

#include "renderingTexture.h"
#include "core/resource/include/resource.h"
#include "core/resource/include/bufferAsync.h"

BEGIN_BOOMER_NAMESPACE()

// compiled static texture mipmap
// NOTE: compressed data is opaque, no internal layout information (like row pitch) is given
struct ENGINE_TEXTURE_API StaticTextureMip
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(StaticTextureMip);

public:
    uint32_t dataOffset = 0; // offset to data for this mip in the texture data buffer
    uint32_t dataSize = 0; // size of mipmap data

    uint32_t width = 0; // width of this mipmap image (in pixels)
    uint32_t height = 0; // height of this mipmap image (in pixels)
    uint32_t depth = 0; // depth of this mipmap image (in pixels)

    uint32_t rowPitch = 0; // how many bytes to the next row of data
    uint32_t slicePitch = 0; // how many bytes to the data for the next slice (3D textures only)
    bool compressed = false; // data is internally compressed (and must be decompresed before used)
    bool streamed = false; // is this a streamed mip 
};

/// static texture, does not change much (usually baked) but can be streamed
class ENGINE_TEXTURE_API StaticTexture : public ITexture
{
    RTTI_DECLARE_VIRTUAL_CLASS(StaticTexture, ITexture);

public:
    StaticTexture();
    StaticTexture(Buffer&& data, res::AsyncBuffer&& asyncData, Array<StaticTextureMip>&& mips, const TextureInfo& info);
    StaticTexture(const image::ImageView& image); // create a simple texture directly from image, no compression
    virtual ~StaticTexture();

    //--

    // get persistent block data
    INLINE const Buffer& persistentData() const { return m_persistentPayload; }

    // get mipmaps
    INLINE const Array<StaticTextureMip>& mips() const { return m_mips; }

    //--

    // ITexture interface
    virtual gpu::ImageSampledViewPtr view() const override final;

    //--

protected:
    Buffer m_persistentPayload; // payload loaded with the resource, contains part of the texture we don't want to stream
    res::AsyncBuffer m_streamingPayload; // additional data payloads for streaming data - high res data
    Array<StaticTextureMip> m_mips; // mip map data

    //--

    gpu::ImageObjectPtr m_object;
    gpu::ImageSampledViewPtr m_mainView;

    //--

    // IResource
    virtual void onPostLoad() override;

    void createDeviceResources();
    void destroyDeviceResources();
};

//--

END_BOOMER_NAMESPACE()
