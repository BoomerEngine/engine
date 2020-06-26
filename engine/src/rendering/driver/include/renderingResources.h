/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface #]
***/

#pragma once

#include "renderingImageFormat.h"

namespace rendering
{
    //---

    /// buffer flags, determines what kind of buffer we are dealing with and what can we do with it
    enum class BufferViewFlag : uint16_t
    {
        None = 0,
        Constants = FLAG(0), // buffer can be used as constant buffer
        Vertex = FLAG(1), // buffer can be used as vertex buffer
        Index = FLAG(2), // buffer can be used as index buffer
        ShaderReadable = FLAG(3), // buffer can be read in shaders, requires shader-side specified format to do so
        UAVCapable = FLAG(4), // buffer is capable of a "UAV like" view when it can be written to, requires shader-side specified format to do so
        Structured = FLAG(5), // buffer is structured, contains internal swizzling and thus a "structure stride"
        CopyCapable = FLAG(6), // we can copy to/from this buffer
        IndirectArgs = FLAG(7), // buffer can be used as a source of indirect arguments for calls
        Dynamic = FLAG(8), // buffer can be updated dynamically
        Transient = FLAG(9), // buffer is created as "intra frame" resource that is not persistent
    };

    typedef base::DirectFlags<BufferViewFlag> BufferViewFlags;

    //---

    struct RENDERING_DRIVER_API BufferCreationInfo
    {
        bool allowCostantReads = false; // allow this buffer to be read as a UBO (uniform buffer object) ie. constant buffer
        bool allowShaderReads = false; // allow this buffer to be READ in shaders - requires shader-side specified format
        bool allowDynamicUpdate = false; // allow this buffer to be dynamically updated
        bool allowCopies = false; // allow this buffer to be copied to/from
        bool allowUAV = false; // allow this buffer to be bound for unordered read/writes  - requires shader-side specified format/layout
        bool allowVertex = false; // allow this buffer to be bound as a vertex buffer
        bool allowIndex = false; // allow this buffer to be bound as a index buffer
        bool allowIndirect = false; // allow this buffer to be bound as a indirect argument buffer

        uint32_t size = 0; // size of the buffer data to allocate
        uint32_t stride = 0; // non zero ONLY for structured buffers
        base::StringBuf label; // debug label

        BufferViewFlags computeFlags() const;

        void print(base::IFormatStream& f) const;
    };

    //---

    /// image flags, determines what kind of image we are dealing with
    enum class ImageViewFlag : uint16_t
    {
        Preinitialized = FLAG(0), // image was created as a preinitialized texture
        Dynamic = FLAG(1), // image was created as a preinitialized texture but it's allowed to update it dynamically
        Multisampled = FLAG(2), // image was created as multisampled render target
        ShaderReadable = FLAG(3), // image can be read by shaders - not always set for render targets that are for resolve only - LAYOUT TRANSITION REQUIRED
        UAVCapable = FLAG(4), // image is writable by shaders in an "UAV style" - LAYOUT TRANSITION REQUIRED
        RenderTarget = FLAG(5), // image can be used as a render target - LAYOUT TRANSITION REQUIRED
        Depth = FLAG(6), // we are a depth buffer - LAYOUT TRANSITION REQUIRED
        SRGB = FLAG(7), // we use sRGB format
        Compressed = FLAG(8), // we are using compressed format
        CopyCapable = FLAG(9), // we can copy to/from this image
        SwapChain = FLAG(10), // render target is part of output swapchain
        FlippedY = FLAG(11), // this render target must be Y-flipped for display (OpenGL window)
    };

    typedef base::DirectFlags<ImageViewFlag> ImageViewFlags;

    // internal memory layout of image
    enum class ImageLayout : uint8_t
    {
        ShaderReadOnly, // image is readable by the shader (but it's not modified, SRV), required state for ImageViewFlag::ShaderRead
        ShaderReadWrite, // image is writable by the shader (UAV), required state for ImageViewFlag::UAVCapable
        RenderTarget, // image is used as target for rendering (read/write), required state for ImageViewFlag::RenderTarget
        DepthReadOnly, // image is used as read only depth buffer), required state for ImageViewFlag::Depth in read only mode
    };

    ///---
    
    struct RENDERING_DRIVER_API ImageCreationInfo
    {
        ImageViewType view = ImageViewType::View2D; // dimensionality of the image + is it an array
        ImageFormat format = ImageFormat::UNKNOWN; // specialized image format
        ObjectID sampler = ObjectID::DefaultBilinearSampler();

        bool allowShaderReads = false; // allow this image to be read in shaders - this is the most common state for textures, etc
        bool allowDynamicUpdate = false; // allow this image to be dynamically updated
        bool allowCopies = false; // allow this image to be copied to/from
        bool allowRenderTarget = false; // allow this image to be used as render target
        bool allowUAV = false; // allow this image to be bound for unordered read/writes 

        uint16_t width = 1; // width of the image, in pixels
        uint16_t height = 1; // height of the image, in pixels (2D and 3D only)
        uint16_t depth = 1; // depth of the image, in pixels (3D only)

        uint8_t numMips = 1; // number of mipmaps in the image you want to upload
        uint16_t numSlices = 1; // number of array slices in the image (for arrays and cubemaps, NOTE: for cubemap arrays this will be in multiples of 6)
        uint8_t numSamples = 1; // number of samples (<=1 - not mulitsampled) NOTE: lmited to 2D textures

        base::StringBuf label; // debug label

        //--

        INLINE bool multisampled() const { return numSamples > 1; }

        //--

        ImageViewFlags computeViewFlags() const;

        uint32_t calcMemoryUsage() const;

        //--

        uint32_t hash() const;

        bool operator==(const ImageCreationInfo& other) const;
        bool operator!=(const ImageCreationInfo& other) const;

        //---

        void print(base::IFormatStream& f) const;
    };

    //--

    /// holder for data we want to download from rendering device side
    template< typename T >
    class DownloadData : public base::IReferencable
    {
    public:
        typedef std::function<void(const T & data)> TCallback;

        INLINE DownloadData(const TCallback& callback = TCallback())
            : m_callback(callback)
            , m_ready(false)
        {}

        INLINE bool isReady()
        {
            auto lock = base::CreateLock(m_lock);
            return m_ready;
        }

        INLINE T data()
        {
            auto lock = base::CreateLock(m_lock);
            return m_data;
        }

        INLINE void publish(const T& data)
        {
            {
                auto lock = base::CreateLock(m_lock);

                if (m_ready)
                    return;

                m_data = data;
                m_ready = true;
            }

            if (m_callback)
                m_callback(data);
        }

        //--

    private:
        base::SpinLock m_lock;
        TCallback m_callback;
        T m_data;
        volatile bool m_ready;
    };

    using DownloadImage = DownloadData<base::image::ImagePtr>;
    using DownloadImagePtr = base::RefPtr<DownloadImage>;

    using DownloadBuffer = DownloadData<base::Buffer>;
    using DownloadBufferPtr = base::RefPtr<DownloadBuffer>;

    //--

    // source data for image/buffer initialization
    struct SourceData
    {
        base::Buffer data; // source data
        uint32_t offset = 0; // offset into the buffer we passed (we can have one buffer used to initialize multiple resources)
        uint32_t size = 0; // size of data in the buffer (for validation)
    };

    //--

} // rendering

