/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "rendering_device_glue.inl"

namespace base
{
	namespace image
	{
		struct ImageRect;
		class ImageView;
	}
}

namespace rendering
{

    ///---

    namespace command
    {
        class CommandWriter;
        class CommandBuffer;
    } // command

	namespace api
	{
		class IBaseObject; // foward declaration of internal api objects
	} // api

    ///---

    class IDevice;
    struct PerformanceStats;

    ///---

    class ObjectID;

    class IDeviceObjectHandler;

    class IDeviceObject;
    typedef base::RefPtr<IDeviceObject> DeviceObjectPtr;

    class IDeviceObjectView;
    typedef base::RefPtr<IDeviceObjectView> DeviceObjectViewPtr;

    //--

    enum class OutputClass : uint8_t
    {
        Offscreen, // render off-screen, no window is allocated, content can be streamed to file/network via consumer callback
        Window, // render to native window, window can be moved and resized be user or made fullscreen
        HMD, // render to connected HMD device (VR/AD headset)
    };

    struct OutputInitInfo;

    class IOutputObject;
    typedef base::RefPtr<IOutputObject> OutputObjectPtr;

    class INativeWindowInterface;

    class FrameBuffer;

    //----

	class ShaderObject;
	typedef base::RefPtr<ShaderObject> ShaderObjectPtr;

	class ShaderData;
	typedef base::RefPtr<ShaderData> ShaderDataPtr;

	class ShaderMetadata;
	typedef base::RefPtr<ShaderMetadata> ShaderMetadataPtr;

	class ShaderFile;
	typedef base::RefPtr<ShaderFile> ShaderFilePtr;
	typedef base::res::Ref<ShaderFile> ShaderFileRef;

	struct ShaderVertexElementMetadata;
	struct ShaderVertexStreamMetadata;
	struct ShaderDescriptorEntryMetadata;
	struct ShaderDescriptorMetadata;
	struct ShaderStaticSamplerMetadata;

	//--

	struct GraphicsPassLayoutSetup;	
	struct GraphicsRenderStatesSetup;

	class GraphicsRenderStatesObject;
	typedef base::RefPtr<GraphicsRenderStatesObject> GraphicsRenderStatesObjectPtr;

	class GraphicsPassLayoutObject;
	typedef base::RefPtr<GraphicsPassLayoutObject> GraphicsPassLayoutObjectPtr;

	class GraphicsPipelineObject;
	typedef base::RefPtr<GraphicsPipelineObject> GraphicsPipelineObjectPtr;

	class ComputePipelineObject;
	typedef base::RefPtr<ComputePipelineObject> ComputePipelineObjectPtr;

    //--

    struct BufferCreationInfo;

    class BufferObject;
    typedef base::RefPtr<BufferObject> BufferObjectPtr;

    class BufferView;
    typedef base::RefPtr<BufferView> BufferViewPtr;

    class BufferWritableView;
    typedef base::RefPtr<BufferWritableView> BufferWritableViewPtr;

    class BufferStructuredView;
    typedef base::RefPtr<BufferStructuredView> BufferStructuredViewPtr;

    class BufferWritableStructuredView;
    typedef base::RefPtr<BufferWritableStructuredView> BufferWritableStructuredViewPtr;

	class BufferConstantView;
	typedef base::RefPtr<BufferConstantView> BufferConstantViewPtr;

    //--

    struct ImageCreationInfo;

    class ImageObject;
    typedef base::RefPtr<ImageObject> ImageObjectPtr;

    class ImageView;
    typedef base::RefPtr<ImageView> ImageViewPtr;

    class ImageWritableView;
    typedef base::RefPtr<ImageWritableView> ImageWritableViewPtr;

    class RenderTargetView;
    typedef base::RefPtr<RenderTargetView> RenderTargetViewPtr;

    //---

    struct SamplerState;

    class SamplerObject;
    typedef base::RefPtr<SamplerObject> SamplerObjectPtr;

    ///---

    enum class ResourceLayout : uint8_t
    {
        INVALID,

        Common,
        ConstantBuffer,
        VertexBuffer,
        IndexBuffer,
        RenderTarget,
        UAV,
        DepthWrite,
        DepthRead,
        //NonPixelShaderResource,
        //PixelShaderResource, // TODO!!!
		ShaderResource,
        IndirectArgument,
        CopyDest,
        CopySource,
        ResolveDest,
        ResolveSource,
        RayTracingAcceleration,
        ShadingRateSource,
        Present,
    };

    ///---

    class DescriptorID;
    class DescriptorInfo;
    class DescriptorInfoBuilder;

    ///---

	class ISourceDataProvider;
	typedef base::RefPtr<ISourceDataProvider> SourceDataProviderPtr;

	class IDownloadDataSink;
	typedef base::RefPtr<IDownloadDataSink> DownloadDataSinkPtr;
	
	///---

	// NOTE: always memzero!
	union ResourceCopyRange
	{
		INLINE ResourceCopyRange() {};

		struct
		{
			uint32_t offset;
			uint32_t size;  // must be set!
		} buffer;

		struct
		{
			uint8_t firstMip;
			uint8_t numMips;  // must be set
			uint16_t firstSlice;
			uint16_t numSlices;  // must be set

			uint32_t offsetX;
			uint32_t offsetY;
			uint32_t offsetZ;
			uint32_t sizeX;
			uint32_t sizeY;
			uint32_t sizeZ;
		} image;
	};

	//--

	// NOTE: always memzero!
	struct ResourceCopyElement
	{
		uint32_t dataSize = 0;

		struct
		{
			uint32_t size = 0;
			uint32_t offset = 0;
		} buffer;

		struct
		{
			uint8_t mip = 0;
			uint16_t slice = 0;
		} image;
	};

	///---

	union ResourceClearRect
	{
		struct
		{
			uint32_t offset = 0;
			uint32_t size = 0;
		}  buffer;

		struct
		{
			uint16_t offsetX = 0;
			uint16_t offsetY = 0;
			uint16_t offsetZ = 0;
			uint16_t sizeX = 0;
			uint16_t sizeY = 0;
			uint16_t sizeZ = 0;
		}  image;

		INLINE ResourceClearRect() {};
	};

	///---

    enum class ShaderStage : uint8_t
    {
        Invalid = 0,

        Vertex,
        Geometry,
        Domain,
        Hull,
        Pixel,
        Compute,
        Task,
        Mesh,

        MAX,
    };

    //--

    typedef base::BitFlagsBase<ShaderStage, uint16_t> ShaderStageMask;

    //--

    enum class DeviceObjectViewType : uint8_t
    {
        Invalid = 0,

        ConstantBuffer,  // CBV (may be inlined - it's missing an object then)

        Buffer,  // SRV for buffer, requires format
        BufferWritable, // UAV for buffer, requires format

        BufferStructured, // read only UAV
        BufferStructuredWritable, // read/write UAV

        Image, // read only texture (SRV), sampled, related to some sampler somewhere (specified in shader metadata)
        ImageWritable, // UAV

		ImageTable, // descriptor table/binding table for multiple images

        Sampler, // explicit sampler entry that can be changed at runtime

        RenderTarget,
    };

    struct DescriptorEntry;

    class DescriptorID;
    class DescriptorInfo;

    //--


} // rendering

#include "renderingObjectID.h"
#include "renderingImageFormat.h"

#ifndef BUILD_RELEASE
	#define VALIDATE_RESOURCE_LAYOUTS
	#define VALIDATE_VERTEX_LAYOUTS
	#define VALIDATE_DESCRIPTOR_BINDINGS 
	#define VALIDATE_DESCRIPTOR_BOUND_RESOURCES	
#endif
