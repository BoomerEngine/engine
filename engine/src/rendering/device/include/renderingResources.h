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
        //Transient = FLAG(9), // buffer is created as "intra frame" resource that is not persistent
    };

    typedef base::DirectFlags<BufferViewFlag> BufferViewFlags;

    //---

    struct RENDERING_DEVICE_API BufferCreationInfo
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

        ResourceLayout initialLayout = ResourceLayout::INVALID;

        BufferViewFlags computeFlags() const;
		ResourceLayout computeDefaultLayout() const;

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
		SubResourceLayouts = FLAG(12), // this resource allows sub-resources to be transition independently
    };

    typedef base::DirectFlags<ImageViewFlag> ImageViewFlags;

    ///---
    
    struct RENDERING_DEVICE_API ImageCreationInfo
    {
        ImageViewType view = ImageViewType::View2D; // dimensionality of the image + is it an array
        ImageFormat format = ImageFormat::UNKNOWN; // specialized image format

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

        ResourceLayout initialLayout = ResourceLayout::INVALID; // auto determine based on flags

        base::StringBuf label; // debug label

        //--

        INLINE bool multisampled() const { return numSamples > 1; }

        //--

        ImageViewFlags computeViewFlags() const;
		ResourceLayout computeDefaultLayout() const;

        uint32_t calcMemoryUsage() const;
		uint32_t calcMipDataSize(uint8_t mipIndex) const;
		uint32_t calcMipWidth(uint8_t mipIndex) const;
		uint32_t calcMipHeight(uint8_t mipIndex) const;
		uint32_t calcMipDepth(uint8_t mipIndex) const;

		uint32_t calcRowPitch(uint8_t mipIndex) const;
		uint32_t calcRowLength(uint8_t mipIndex) const; // NOTE: for compressed textures it's never <4

        //--

        uint32_t hash() const;

        bool operator==(const ImageCreationInfo& other) const;
        bool operator!=(const ImageCreationInfo& other) const;

        //---

        void print(base::IFormatStream& f) const;
    };

    //--

    // source data for image/buffer initialization
	// NOTE: this is asynchronous in nature and can be kept alive for undetermined amount of time
    class RENDERING_DEVICE_API ISourceDataProvider : public base::IReferencable
    {
		RTTI_DECLARE_POOL(POOL_RENDERING_RUNTIME);

	public:
		virtual ~ISourceDataProvider();

		enum class ReadyStatus : uint8_t
		{
			NotReady,
			Ready,
			Failed
		};

		// get a debug label for this data, usually resource name
		virtual base::StringView debugLabel() const { return ""; }

		// called once when we want to indicate we want to begin copying from this source
		// NOTE: source should start preparing internal data, optionally a persistent load state may be returned in the "token" 
		// NOTE: this method should NOT block, if async operation is needed you can use checkSourceDataReady
		// NOTE: the copy queue assumes that ALL sub-resources may be filled at the same time, if that's not the case specify "supportsAsyncWrites=false"
		virtual bool openSourceData(const ResourceCopyRange& range, void*& outToken, bool& outSupportsAsyncWrites) const;

		// check if source is ready for copying (whatever operation has started in openSourceData) is finished
		// NOTE: this may return "late failure" - i.e. file did not open etc. If that happens the whole copy is canceled
		virtual ReadyStatus checkSourceDataReady(const void* token) const;

		// write initialization data into provided pointer
		// NOTE: the most amazing thing about it this function is that it can wait happen in background (ie. disk loading is allowed here)
		virtual CAN_YIELD void writeSourceData(void* destPointer, void* token, const ResourceCopyElement& element) const = 0;

		// called once all copying is finished, allows to free related resources
		virtual CAN_YIELD void closeSourceData(void* token) const;
    };

    //--

	// shitty implementation of source data that uses copied data to initialize the resource
	// NOTE: we do support copying from an offset in the buffer
	// NOTE: this is fallback/corner case use as this does not provide nice asynchronous resource upload
	class RENDERING_DEVICE_API SourceDataProviderBuffer : public ISourceDataProvider
	{
		RTTI_DECLARE_POOL(POOL_RENDERING_RUNTIME);

	public:
		SourceDataProviderBuffer(const base::Buffer& data, uint32_t sourceOffset=0, uint32_t sourceSize=0);

		INLINE uint32_t sourceOffset() const { return m_sourceOffset; }
		INLINE uint32_t sourceSize() const { return m_sourceSize; }

		INLINE const base::Buffer& data() const { return m_data; }

		// upload buffer to GPU memory (copies the content from buffer if it fits)
		virtual CAN_YIELD void writeSourceData(void* destPointer, void* token, const ResourceCopyElement& element) const override final;

	private:
		base::Buffer m_data;

		uint32_t m_sourceOffset = 0;
		uint32_t m_sourceSize = 0; // validated only if non-zero
	};

	//--

	// sink for GPU data download
	// NOTE: this is asynchronous in nature and can be kept alive for undetermined amount of time so it should handle cancellation internally
	// NOTE: this WILL be called from internal device thread, do not use fiber sync points inside
	class RENDERING_DEVICE_API IDownloadDataSink : public base::IReferencable
	{
		RTTI_DECLARE_POOL(POOL_RENDERING_RUNTIME);

	public:
		virtual ~IDownloadDataSink();

		// get a debug label for this data, usually resource name
		virtual base::StringView debugLabel() const { return ""; }

		// data was retrieved from GPU, do whatever you want with it
		// NOTE: the data is stored either in special staging buffer or in case of UMA you will have direct pointer to it
		// NOTE: it goes without saying that the pointer should not be cached
		virtual CAN_YIELD void processRetreivedData(const void* srcPtr, uint32_t retrievedSize, const ResourceCopyRange& info) = 0;
	};

	//--

	// simple (aka. BAD EXMAPLE) sink that keeps the retrieved data in a buffer for later inspection
	class RENDERING_DEVICE_API DownloadDataSinkBuffer : public IDownloadDataSink
	{
	public:
		DownloadDataSinkBuffer(PoolTag pool, uint32_t alignment=16);

		/// get the buffer version, initially it's zero, increments after each processRetreivedData call (so the sink can be reused)
		INLINE uint32_t version() const { return m_version.load(); }

		/// retrieve the internal buffer (NOTE: object copy, the buffer holder is ref counted inside so its fast)
		base::Buffer data(uint32_t* outVersion = nullptr) const;

	protected:
		std::atomic<uint32_t> m_version; // changed after each new upload

		base::SpinLock m_dataLock;
		base::Buffer m_data;

		PoolTag m_poolTag;
		uint32_t m_alignment;

		//--

		virtual CAN_YIELD void processRetreivedData(const void* srcPtr, uint32_t retrievedSize, const ResourceCopyRange& info) override final;
	};

	//--

} // rendering

