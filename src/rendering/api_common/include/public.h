/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "rendering_api_common_glue.inl"

BEGIN_BOOMER_NAMESPACE(rendering::api)

//--

class IBaseObject;
class IBaseDevice;
class IBaseThread;
		
class IBaseCopiableObject;
class IBaseFrameExecutor;
class IBaseObjectCache;
class IBaseSwapchain;
class IBaseBuffer;
class IBaseBufferView;
class IBaseImage;
class IBaseImageView;
class IBaseSampler;
class IBaseShaders;
class IBaseGraphicsRenderStates;
class IBaseVertexBindingLayout;
class IBaseDescriptorBindingLayout;
class IBaseGraphicsPipeline;
class IBaseComputePipeline;		
class IBaseRaytracePipeline;
class IBaseBackgroundJob;
class IBaseBackgroundQueue;
class IBaseDownloadArea;

class ObjectRegistry;
class ObjectRegistryProxy;

class WindowManager;
class FrameCompleteionQueue;
class FrameExecutionData;

class Output;

typedef base::RefPtr<IBaseBackgroundJob> BackgroundJobPtr;

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

	ImageReadOnlyView,
	ImageWritableView,
	SampledImageView,

	RenderTargetView,
	OutputRenderTargetView,

	BufferTypedView,
	BufferUntypedView,
	StructuredBufferView,
	StructuredBufferWritableView,

	GraphicsPassLayout,
	GraphicsRenderStates,
	GraphicsPipelineObject,
	ComputePipelineObject,

	DownloadArea,
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

END_BOOMER_NAMESPACE(rendering::api)