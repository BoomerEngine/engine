/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "rendering_api_common_glue.inl"

namespace rendering
{
	namespace api
	{

		//--

		class IBaseObject;
		class IBaseDevice;
		class IBaseThread;
		
		class IBaseCopiableObject;
		class IBaseFrameExecutor;
		class IBaseObjectCache;
		class IBaseStagingPool;
		class IBaseCopyQueue;
		class IBaseTransientBuffer;
		class IBaseTransientBufferPool;
		class IBaseSwapchain;
		class IBaseFrameFence;
		class IBaseBuffer;
		class IBaseBufferView;
		class IBaseImage;
		class IBaseImageView;
		class IBaseSampler;
		class IBaseShaders;
		class IBaseGraphicsPassLayout;
		class IBaseGraphicsRenderStates;
		class IBaseVertexBindingLayout;
		class IBaseDescriptorBindingLayout;
		class IBaseGraphicsPipeline;
		class IBaseComputePipeline;		
		class IBaseRaytracePipeline;

		class ObjectRegistry;
		class ObjectRegistryProxy;

		class WindowManager;
		class Frame;
		class RuntimeDataAllocations;

		struct StagingArea;

		class Output;

		//--

		// type of api object (set in the IBaseObject)
		enum class ObjectType : uint8_t
		{
			Unknown = 0,

			Buffer,
			Image,
			Sampler,
			Shaders,
			Output,

			ImageView,
			ImageWritableView,
			BufferTypedView,
			BufferUntypedView,
			StructuredBufferView,
			StructuredBufferWritableView,
			RenderTargetView,

			OutputRenderTargetView,

			GraphicsPassLayout,
			GraphicsRenderStates,
			GraphicsPipelineObject,
			ComputePipelineObject,
		};

		//--

		typedef std::function<void(void)> FrameCompletionCallback;

		//--

		/// type of transient buffer
		enum class TransientBufferType
		{
			Unknown,
			Staging,
			Constants,
			Geometry,
		};

		//--

		union PlatformPtr
		{
			void* dx = nullptr;
			uint64_t vk;
			uint32_t gl;

			ALWAYS_INLINE operator bool() const { return dx != nullptr; }
			ALWAYS_INLINE bool operator==(PlatformPtr other) const { return dx == other.dx; }
			ALWAYS_INLINE bool operator!=(PlatformPtr other) const { return dx != other.dx; }
		};

		//--

		struct ResourceCopyAtom
		{
			uint32_t stagingAreaOffset = 0; // where to place the copy data
			ResourceCopyElement copyElement; // what is exactly being copied
		};

		//--

	} // api
} // rendering

