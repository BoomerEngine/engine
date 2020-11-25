/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "rendering_device_glue.inl"

#include "renderingObjectID.h"
#include "renderingStates.h"
#include "renderingImageFormat.h"

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
        NativeWindow, // render to native window, window can be moved and resized be user
        Fullscreen, // render to full screen device, input and rendering may be handled differently
        HMD, // render to connected HMD device (VR/AD headset)
    };

    struct OutputInitInfo;

    class IOutputObject;
    typedef base::RefPtr<IOutputObject> OutputObjectPtr;

    class INativeWindowInterface;

    class FrameBuffer;

    //----

    /// index of structure in pipeline library
    /// TOD: let's pray this won't have to be 32-bit, it would be a huge waste of memory
    typedef uint16_t PipelineIndex;

    /// index to debug string in pipeline library
    typedef uint32_t PipelineStringIndex;

    /// invalid pipeline index
    static const PipelineIndex INVALID_PIPELINE_INDEX = (PipelineIndex)-1;

    class ShaderLibrary;
    typedef base::RefPtr<ShaderLibrary> ShaderLibraryPtr;
    typedef base::res::Ref<ShaderLibrary> ShaderLibraryRef;

    class ShaderLibraryData;
    typedef base::RefPtr<ShaderLibraryData> ShaderLibraryDataPtr;

    class ShaderObject;
    typedef base::RefPtr<ShaderObject> ShaderObjectPtr;

	//--

	struct GraphicsPassLayout;

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

    /// type of object view
    enum class DeviceObjectViewType : uint8_t
    {
        Invalid,
		ConstantBuffer, // CBV (may be inlined - it's missing an object then)
        Buffer,  // SRV for buffer
        BufferWritable, // UAV for buffer
        BufferStructured, // read only UAV
        BufferStructuredWritable, // read/write UAV
        Image, // read only texture (SRV)
        ImageWritable, // UAV
        RenderTarget,
        Sampler,
    };

    struct DescriptorEntry;

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

} // rendering


#ifndef BUILD_RELEASE
	#define VALIDATE_RESOURCE_LAYOUTS
	#define VALIDATE_VERTEX_LAYOUTS
	#define VALIDATE_DESCRIPTOR_BINDINGS 
	#define VALIDATE_DESCRIPTOR_BOUND_RESOURCES	
#endif
