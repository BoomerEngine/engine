/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface #]
***/

#pragma once

#include "core/object/include/rttiMetadata.h"
#include "buffer.h"
#include "image.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu)

// rendering class identification (GL4, Vulkan, DX12, etc)
class GPU_DEVICE_API DeviceNameMetadata : public rtti::IMetadata
{
    RTTI_DECLARE_VIRTUAL_CLASS(DeviceNameMetadata, rtti::IMetadata);

public:
    DeviceNameMetadata();

    INLINE DeviceNameMetadata& name(const char* name)
    {
        m_name = name;
        return *this;
    }

    INLINE const char* name() const { return m_name; }

private:
    const char* m_name;
};

// performance stats for internal API workings
struct GPU_DEVICE_API PerformanceStats : public IReferencable
{
    uint32_t numLogicalCommandBuffers = 0;
    uint32_t numNativeCommandBuffers = 0;

    uint32_t numCommands = 0;
    uint32_t numPasses = 0;
    uint32_t numDrawCalls = 0;
    uint32_t numDispatchCalls = 0;
    uint32_t numTriangles = 0;

    float totalGPUTime = 0.0f;
    float totalCPUTime = 0.0f;

    float executionTime = 0.0f;
    float prepareTime = 0.0f;
    float presentTime = 0.0f;
    float uploadTime = 0.0f;

    uint32_t uploadTotalSize = 0;
    uint32_t uploadDynamicBufferCount = 0;
    uint32_t uploadDynamicBufferSize = 0;
    uint32_t uploadConstantsCount = 0;
    uint32_t uploadConstantsSize = 0;
	uint32_t uploadConstantsBuffersCount = 0;
    uint32_t uploadParametersCount = 0;
};

// adapter information
struct AdapterInfo
{
	StringBuf name;
	uint64_t deviceMemorySize = 0;
	uint64_t hostMemorySize = 0;
	uint64_t sharedMemorySize = 0;
};

// display information
struct DisplayInfo
{
    StringBuf name;
    uint32_t desktopWidth = 0;
    uint32_t desktopHeight = 0;
    bool primary = false;
    bool attached = false;
    bool active = false;
};

// display format information
struct DisplayFormatInfo
{
	ImageFormat format = ImageFormat::UNKNOWN;
	bool hdr = false;

	// TODO: more (color space)?
};

// resolution information
struct RefreshRateInfo
{
	uint32_t num = 0;
	uint32_t denom = 0;

	INLINE float rate() const
	{
		return denom ? num / (float)denom : num;
	}

	INLINE bool operator==(const RefreshRateInfo& other) const
	{
		return (num == other.num) && (denom == other.denom);
	}
};

// resolution information
struct ResolutionInfo
{
    uint32_t width = 0;
    uint32_t height = 0;

	Array<RefreshRateInfo> refreshRates;
	Array<DisplayFormatInfo> formats;
};

// vsync information
struct ResolutionSyncInfo
{
    StringBuf name;
    int value = 0;
};

// basic device timing/sync info
struct DeviceSyncInfo
{
	/// CPU (client side) frame index, advanced with every "advanceFrame"
	uint64_t cpuFrameIndex = 0;

	/// frame already started on the processing thread 
	uint64_t threadFrameIndex = 0;

	/// frame index that was last sent to GPU for actual rendering
	uint64_t gpuStartedFrameIndex = 0;

	/// frame index that was last confirmed by GPU as finished
	/// NOTE: ALL submitted command buffers from within given CPU frame must finish for this to be incremented
	uint64_t gpuFinishedFrameIndex = 0;
};

//--

// completion condition, corresponds to frame numbers in DeviceSyncInfo
enum class DeviceCompletionType : uint8_t
{
	// current CPU frame has finished (advanceFrame() was called, called form MainThread)
	CPUFrameFinished,

	// recording of command buffer to native command buffers has finished and data is about to be submitted to the GPU
	// this means that any memory blocks that were external to command buffer (i.e. not copied in) are now not needed any more
	GPUFrameRecorded,

	// GPU rendering finished, any resources that might have been used by the GPU are now free to be updated/etc
	// NOTE: on some APIs this takes a little bit longer to get signaled then on others
	GPUFrameFinished,
};

// completion callback, called from within the DEVICE once some conditions occur (usually GPU finshed particular workload)
class GPU_DEVICE_API IDeviceCompletionCallback : public IReferencable
{
	RTTI_DECLARE_POOL(POOL_GPU_RUNTIME);

public:
	virtual ~IDeviceCompletionCallback();

	// signal that particular moment has occurred
	virtual void signalCompletion() = 0;

	//--

	// create completion callback that calls a function
	static DeviceCompletionCallbackPtr CreateFunctionCallback(const std::function<void(void)>& func);

	// create completion callback that calls a fence
	static DeviceCompletionCallbackPtr CreateFenceCallback(const fibers::WaitCounter& fence, uint32_t count = 1);
};
	
//--
    
// rendering device implementation
// this is the most "low-level" API used by the rendering, can be used directly but it's more work
class GPU_DEVICE_API IDevice : public NoCopy
{
    RTTI_DECLARE_POOL(POOL_GPU_RUNTIME);
    RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IDevice);

public:
    IDevice();
    virtual ~IDevice();

    ///--

    /// describe the runtime device, can contain the version string
    virtual StringBuf name() const = 0;

    /// get the maximum size of allowed render targets (usually the size of the biggest monitor)
    virtual Point maxRenderTargetSize() const = 0;

    /// initialize the rendering device, can fail obviously and it should be handled gracefully
    virtual bool initialize(const app::CommandLine& cmdLine, DeviceCaps& outCaps) = 0;

    /// close the rendering driver, must be called before deletion
    virtual void shutdown() = 0;

	//---

	/// get current sync info, can be used to perform flip/flop resource reuse based on frame indices rather than fences
	virtual DeviceSyncInfo querySyncInfo() const = 0;

	/// sync engine, rendering thread and GPU states with optional full pipeline flush
	virtual void sync(bool flush) = 0;

	/// register callback on completion of current frame
	virtual bool registerCompletionCallback(DeviceCompletionType type, IDeviceCompletionCallback* callback) = 0;

    //---

    /// create rendering output 
    virtual OutputObjectPtr createOutput(const OutputInitInfo& info) = 0;

    /// prepare shader library for rendering, returns opaque object that represents the shader library on the rendering device's side
    /// NOTE: the source data will be kept alive as long as the object exists
    virtual ShaderObjectPtr createShaders(const ShaderData* shaderData) = 0;

    /// create a buffer, if source data is provided then it's asked to fill in the buffer with content
	/// NOTE: it's legal to use resource BEFORE it's fully initialized with data it might just contain crap
    virtual BufferObjectPtr createBuffer(const BufferCreationInfo& info, const ISourceDataProvider* sourceData = nullptr) = 0;

    /// create an image, optionally fill it with data (one "SourceData" entry for each mip*slice)
	/// NOTE: it's legal to use resource BEFORE it's fully initialized with data it might just contain crap
    virtual ImageObjectPtr createImage(const ImageCreationInfo& info, const ISourceDataProvider* sourceData = nullptr) = 0;

    /// create a sampler
    virtual SamplerObjectPtr createSampler(const SamplerState& info) = 0;

	/// create pass layout state, NOTE: may return shared object
	virtual GraphicsRenderStatesObjectPtr createGraphicsRenderStates(const GraphicsRenderStatesSetup& states) = 0;

    //---

    /// enumerate the monitor areas of the desktop
    virtual void enumMonitorAreas(Array<Rect>& outMonitorAreas) const = 0;

    /// enumerate display devices
    virtual void enumDisplays(Array<DisplayInfo>& outDisplayInfos) const = 0;

    /// enumerate resolutions for given display
    virtual void enumResolutions(uint32_t displayIndex, Array<ResolutionInfo>& outResolutions) const = 0;

    //---

    /// Submit a command buffer (with potential child command buffers) to execution
    /// NOTE: this builds actual native command buffers required for rendering the scene
    /// NOTE: this is an async call that completes right away but the work is not done and if enough of it queries that we will wait in the advanceFrame()
    virtual void submitWork(CommandBuffer* commandBuffer, bool background=false) = 0;

    //--

protected:
	bool validateBufferCreationSetup(const BufferCreationInfo& info, BufferObject::Setup& outSetup) const;
	bool validateImageCreationSetup(const ImageCreationInfo& info, ImageObject::Setup& outSetup) const;
};

END_BOOMER_NAMESPACE_EX(gpu)

