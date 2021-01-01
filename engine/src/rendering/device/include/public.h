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

	class IDeviceCompletionCallback;
	typedef base::RefPtr<IDeviceCompletionCallback> DeviceCompletionCallbackPtr;

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

	struct ShaderSelector;

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

	enum class ShaderFeatureBit : uint32_t
	{
		ControlFlowHints = 0, // we use control flow hints
		UAVPixelShader = 1, // we use UAVs in pixel shader
		UAVOutsidePixelShader = 2, // we use UAVs in other stages
		EarlyTestsPixelShader = 3, // depth/stencil is tested before running pixel shader
		DepthModification = 4, // pixel shader is writing to depth (we cannot assume sorting)
		PixelDiscard = 5, // pixel shader is discarding pixels (we cannot assume opaque geometry)
	};

	typedef base::BitFlags<ShaderFeatureBit> ShaderFeatureMask;

	//--

	struct GraphicsRenderStatesSetup;

	class GraphicsRenderStatesObject;
	typedef base::RefPtr<GraphicsRenderStatesObject> GraphicsRenderStatesObjectPtr;

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

    class ImageSampledView;
    typedef base::RefPtr<ImageSampledView> ImageSampledViewPtr;

	class ImageReadOnlyView;
	typedef base::RefPtr<ImageReadOnlyView> ImageReadOnlyViewPtr;

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
			uint8_t mip;
			uint16_t slice;

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

	// how is stuff loaded into the RT
    enum class LoadOp : uint8_t
    {
        Keep, // keep data that exists in the memory
        Clear, // clear the attachment when binding
        DontCare, // we don't care (fastest, can produce artifacts if all pixels are not written)
    };

    // how is stuff stored to memory after pass ends
    enum class StoreOp : uint8_t
    {
        Store, // store the data into the memory
        DontCare, // we don't care (allows to use Discard)
    };

    //--

    typedef base::BitFlagsBase<ShaderStage, uint16_t> ShaderStageMask;

    //--

    enum class DeviceObjectViewType : uint8_t
    {
        Invalid = 0,

        ConstantBuffer,  // CBV (may be inlined - it's missing an object then)

        Buffer,  // read only typed buffer, all entires the same format
        BufferWritable, // read/write typed buffer, all entires the same format, goes via UAV on DirectX

        BufferStructured, // read only storage buffer with stride
        BufferStructuredWritable, // read/write storage buffer with stride, goes via UAV on DirectX

        Image, // read only image (no mips, single slice) (SRV), not samplable but can be multi-sampled
        ImageWritable, // read/write image (no mips, single slice) , not samplable but can be multi-sampled, goes via UAV on DirectX

		SampledImage, // image that can be sampled, has mipmaps, slices, etc

        Sampler, // explicit sampler entry that can be changed at runtime

        RenderTarget,
    };

    struct DescriptorEntry;

    class DescriptorID;
    class DescriptorInfo;

	//--

	// general device tier
	enum class DeviceGeometryTier : uint8_t
	{
		// absolute low-end
		//  - no compute shaders on rendering/post processing
		//  - meshes in separate buffers
		//  - textures/constants bound for each draw call
		//  - materials will be simplified
		Tier0_Legacy, 

		// non-bindless cur-gen
		//  - compute shaders but not for geometry processing
		//  - meshes in separate buffers
		//  - textures/constants bound for each draw call
		Tier1_LowEnd,

		// cur-gen with some bindless processing
		//  - compute shaders used for geometry processing
		//  - meshes in one buffer
		//  - textures/constants still bound separatelly
		Tier2_Meshlets,

		// bindless rendering (mesh lests)
		//  - meshest using compute shaders
		//  - source mesh data in one buffer
		//  - material parameters passes without binding to each drawcall
		Tier3_BindlessMeshlets,

		// next-gen tier with mesh/task shaders
		//  - meshlets using task/mesh shaders (no intermeddiate buffers)
		//  - source mesh data in one buffer
		//  - material parameters passes without binding to each drawcall
		Tier4_MeshShaders,
	};

	//--

	// transparency rendering tier
	enum class DeviceTransparencyTier : uint8_t
	{
		// no special support for anything, transparencies must be rendered back to front
		Tier0_Legacy,

		// support for enough processing power to do a MRT blending
		// minimal geometry tier: Tier1_LowEnd
		Tier1_MRTBlending,

		// support for ROV (rasterization ordered views) that allow for programable blending
		// minimal geometry tier: Tier2_Meshlets
		Tier2_ROV,
	};

	//--

	// raytracing tier
	enum class DeviceRaytracingTier : uint8_t
	{
		// no support
		Tier0_NoSupport,

		// some basic mostly emulated support (RTX 10xx, no DLSS)
		// not good enough for real game but can be used for in-editor preview
		// minimal geometry tier: Tier3_BindlessMeshlets
		Tier1_Basic,

		// full support with DLSS
		Tier2_Full,
	};

	//--

	// device capatiblities
	struct RENDERING_DEVICE_API DeviceCaps
	{
		RTTI_DECLARE_NONVIRTUAL_CLASS(DeviceCaps);

	public:
		DeviceGeometryTier geometry = DeviceGeometryTier::Tier0_Legacy;
		DeviceTransparencyTier transparency = DeviceTransparencyTier::Tier0_Legacy;
		DeviceRaytracingTier raytracing = DeviceRaytracingTier::Tier0_NoSupport;

		uint64_t vramSize = 0;		
	};

    //--

#pragma pack(push)
#pragma pack(4)
	struct GPUDispatchArguments
	{
		uint32_t groupCountX = 1;
		uint32_t groupCountY = 1;
		uint32_t groupCountZ = 1;
	};

	struct GPUDrawIndexedArguments
	{
		uint32_t indexCountPerInstance = 0;
		uint32_t instanceCount = 0;
		uint32_t startIndexLocation = 0;
		uint32_t baseVertexLocation = 0;
		uint32_t startInstanceLocation = 0;
	};

	struct GPUDrawArguments {
		uint32_t vertexCountPerInstance = 0;
		uint32_t instanceCount = 0;
		uint32_t startVertexLocation = 0;
		uint32_t startInstanceLocation = 0;
	}; 
#pragma pack(pop)


} // rendering

#include "renderingObjectID.h"
#include "renderingImageFormat.h"

#ifndef BUILD_RELEASE
	#define VALIDATE_RESOURCE_LAYOUTS
	#define VALIDATE_VERTEX_LAYOUTS
	#define VALIDATE_DESCRIPTOR_BINDINGS 
	#define VALIDATE_DESCRIPTOR_BOUND_RESOURCES	
#endif
