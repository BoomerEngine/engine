/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\views #]
***/

#pragma once

#include "renderingObjectView.h"
#include "renderingImageFormat.h"
#include "renderingResources.h"

namespace rendering
{
    ///---

    /// view description "key" for map lookups
    struct RENDERING_DRIVER_API ImageViewKey
    {
        uint16_t firstSlice = 0;
        uint16_t numSlices = 0;
        ImageViewType viewType = ImageViewType::View2D;
        ImageFormat format = ImageFormat::UNKNOWN;
        uint8_t firstMip = 0;
        uint8_t numMips = 0;

        INLINE ImageViewKey() {};
        INLINE ImageViewKey(const ImageViewKey& other) = default;
        INLINE ImageViewKey& operator=(const ImageViewKey& other) = default;

        INLINE operator bool() const { return format != ImageFormat::UNKNOWN; }
        INLINE uint64_t key() const { return *(const uint64_t*)this; }

        INLINE bool cubemap() const { return (viewType == ImageViewType::ViewCube) || (viewType == ImageViewType::ViewCubeArray); }
        INLINE bool array() const { return (viewType == ImageViewType::View1DArray) || (viewType == ImageViewType::View2DArray) || (viewType == ImageViewType::ViewCubeArray); }

        INLINE ImageViewKey(ImageFormat format_, ImageViewType viewType_, uint16_t firstArraySlice_, uint16_t numArraySlices_, uint8_t firstMip_, uint8_t numMips_)
            : viewType(viewType_)
            , format(format_)
            , firstSlice(firstArraySlice_)
            , numSlices(numArraySlices_)
            , firstMip(firstMip_)
            , numMips(numMips_)
        {
            validate();        
        }

        INLINE bool operator==(const ImageViewKey& other) const { return key() == other.key(); }
        INLINE bool operator!=(const ImageViewKey & other) const { return key() != other.key(); }

        void print(base::IFormatStream& f) const;
        void validate();

        INLINE static uint32_t CalcHash(const ImageViewKey& key) { return base::CRC32() << key.key(); }
    };

    static_assert(sizeof(ImageViewKey) == 8, "There are places that take assumptions of layout of this structure");

    ///---

    /// generic image view
    TYPE_ALIGN(4, class) RENDERING_DRIVER_API ImageView : public ObjectView
    {
    public:
        ImageView();
        ImageView(ObjectID id, ImageViewKey key, uint8_t numSamples, uint16_t width, uint16_t height, uint16_t depth, ImageViewFlags flags, ObjectID sampler);
        
        /// KEY part

        INLINE ImageViewKey key() const { return m_key; }

        INLINE ImageFormat format() const { return m_key.format; }
        INLINE ImageViewType viewType() const { return m_key.viewType; }
        INLINE uint16_t firstArraySlice() const { return m_key.firstSlice; }
        INLINE uint16_t numArraySlices() const { return m_key.numSlices; }
        INLINE uint8_t firstMip() const { return m_key.firstMip; }
        INLINE uint8_t numMips() const { return m_key.numMips; }

        /// RESOURCE part

        INLINE uint8_t numSamples() const { return m_numSamples; }
        INLINE uint16_t width() const { return m_width; }
        INLINE uint16_t height() const { return m_height; }
        INLINE uint16_t depth() const { return m_depth; }
        INLINE ImageViewFlags flags() const { return m_flags; }
        INLINE ObjectID sampler() const { return m_sampler; }

        /// FLAGS

        INLINE bool preinitialized() const { return m_flags.test(ImageViewFlag::Preinitialized); }
        INLINE bool dynamic() const { return m_flags.test(ImageViewFlag::Dynamic); }
        INLINE bool shaderReadable() const { return m_flags.test(ImageViewFlag::ShaderReadable); }
        INLINE bool copyCapable() const { return m_flags.test(ImageViewFlag::CopyCapable); }
        INLINE bool uavCapable() const { return m_flags.test(ImageViewFlag::UAVCapable); }
        INLINE bool renderTarget() const { return m_flags.test(ImageViewFlag::RenderTarget); }
        INLINE bool renderTargetDepth() const { return m_flags.test(ImageViewFlag::Depth); }
        INLINE bool multisampled() const { return m_flags.test(ImageViewFlag::Multisampled); }
        INLINE bool compressed() const { return m_flags.test(ImageViewFlag::Compressed); }
        INLINE bool swapchain() const { return m_flags.test(ImageViewFlag::SwapChain); }
        INLINE bool srgb() const { return m_flags.test(ImageViewFlag::SRGB); }
        INLINE bool flippedY() const { return m_flags.test(ImageViewFlag::FlippedY); }

        ///--

        /// calculate estimated memory size
        uint32_t calcMemorySize() const;

        ///---

        /// Create a sub view of this array at given range of slices
        /// NOITE: we remain an array view, use the createSingleSliceView to collapse array to a single element
        ImageView createArrayView(uint16_t firstRelativeSlice, uint16_t numSlices) const;

        /// create a view of a single slice of the texture
        /// NOTE: legal only on arrays, changes the view type from View2DArray to View2D, etc
        ImageView createSingleSliceView(uint16_t relativeSliceIndex) const;

        /// create a view of a single mipmap
        ImageView createSingleMipView(uint8_t relativeMipIndex) const;

        /// create a view with different sampler
        ImageView createSampledView(ObjectID sampler) const;

        ///--

        /// debug print
        void print(base::IFormatStream& f) const;

        ///---

        /// default texture
        static ImageView DefaultBlack();
        static ImageView DefaultGrayLinear(); // 0.5f
        static ImageView DefaultGraySRGB(); // perceptual 0.5f
        static ImageView DefaultWhite();
        static ImageView DefaultFlatNormals();

        /// default not-used render targets
        static ImageView DefaultColorRT();
        static ImageView DefaultDepthRT();
        static ImageView DefaultDepthArrayRT();

    private:
        ImageViewKey m_key;

        ObjectID m_sampler; // sampler to use when accessing this data in shader

        ImageViewFlags m_flags = ImageViewFlags(); // view flags
        uint16_t m_width = 0; // width of the image in the view
        uint16_t m_height = 0; // height of the image in the view
        uint16_t m_depth = 0; // depth of the image in the view
        uint8_t m_numSamples = 0; // num samples in the texture, 0 - NO MSAA, 1 - MSAA with 1 sample :(
    };

    static_assert(sizeof(ImageView) == 48, "There are places that take assumptions of layout of this structure");
    
    ///---

} // rendering
