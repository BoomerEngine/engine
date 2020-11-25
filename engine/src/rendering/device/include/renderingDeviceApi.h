/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface #]
***/

#pragma once

#include "base/object/include/rttiMetadata.h"
#include "renderingBuffer.h"
#include "renderingImage.h"

namespace rendering
{

    // rendering class identification (GL4, Vulkan, DX12, etc)
    class RENDERING_DEVICE_API DeviceNameMetadata : public base::rtti::IMetadata
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DeviceNameMetadata, base::rtti::IMetadata);

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
    struct RENDERING_DEVICE_API PerformanceStats : public base::IReferencable
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
        uint32_t uploadParametersCount = 0;
    };

    // display information
    struct DisplayInfo
    {
        base::StringBuf name;
        uint32_t desktopWidth = 0;
        uint32_t desktopHeight = 0;
        bool primary = false;
        bool attached = false;
        bool active = false;
    };

    // resolution information
    struct ResolutionInfo
    {
        uint32_t width = 0;
        uint32_t height = 0;

        INLINE bool operator==(const ResolutionInfo& other) const
        {
            return (width == other.width) && (height == other.height);
        }
    };

    // vsync information
    struct ResolutionSyncInfo
    {
        base::StringBuf name;
        int value = 0;
    };
	
	//--
    
    // rendering device implementation
    // this is the most "low-level" API used by the rendering, can be used directly but it's more work
    class RENDERING_DEVICE_API IDevice : public base::NoCopy
    {
        RTTI_DECLARE_POOL(POOL_RENDERING_RUNTIME);
        RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IDevice);

    public:
        IDevice();
        virtual ~IDevice();

        ///--

        /// describe the runtime device, can contain the version string
        virtual base::StringBuf name() const = 0;

        /// get the maximum size of allowed render targets (usually the size of the biggest monitor)
        virtual base::Point maxRenderTargetSize() const = 0;

        /// initialize the rendering device, can fail obviously and it should be handled gracefully
        virtual bool initialize(const base::app::CommandLine& cmdLine) = 0;

        /// close the rendering driver, must be called before deletion
        virtual void shutdown() = 0;

        /// wait for all rendering to finish, this ensures that any hard-core operations are safe
        /// NOTE: this will wait for all work schedule so far via submitWork to complete
        virtual void sync() = 0;

        /// start new frame
        virtual void advanceFrame() = 0;

        //---

        /// create rendering output 
        virtual OutputObjectPtr createOutput(const OutputInitInfo& info) = 0;

        /// prepare shader library for rendering, returns opaque object that represents the shader library on the rendering device's side
        /// NOTE: the source data will be kept alive as long as the object exists
        virtual ShaderObjectPtr createShaders(const ShaderLibraryData* shaderLibraryData, PipelineIndex shaderIndex) = 0;

        /// create a buffer, if source data is provided then it's asked to fill in the buffer with content, optionally a fiber fence may be signaled once resource initialization is completed
		/// NOTE: it's legal to use resource BEFORE it's fully initialized with data it might just contain crap
        virtual BufferObjectPtr createBuffer(const BufferCreationInfo& info, const ISourceDataProvider* sourceData = nullptr, base::fibers::WaitCounter initializationFinished = base::fibers::WaitCounter()) = 0;

        /// create an image, optionally fill it with data (one "SourceData" entry for each mip*slice) , optionally a fiber fence may be signaled once resource initialization is completed
		/// NOTE: it's legal to use resource BEFORE it's fully initialized with data it might just contain crap
        virtual ImageObjectPtr createImage(const ImageCreationInfo& info, const ISourceDataProvider* sourceData = nullptr, base::fibers::WaitCounter initializationFinished = base::fibers::WaitCounter()) = 0;

        /// create a sampler
        virtual SamplerObjectPtr createSampler(const SamplerState& info) = 0;

		/// create pass layout state, NOTE: may return shared object
		virtual GraphicsPassLayoutObjectPtr createGraphicsPassLayout(const GraphicsPassLayoutSetup& info) = 0;

		/// create pass layout state, NOTE: may return shared object
		virtual GraphicsRenderStatesObjectPtr createGraphicsRenderStates(const StaticRenderStatesSetup& states) = 0;

        //---

        /// enumerate the monitor areas of the desktop
        virtual void enumMonitorAreas(base::Array<base::Rect>& outMonitorAreas) const = 0;

        /// enumerate display devices
        virtual void enumDisplays(base::Array<DisplayInfo>& outDisplayInfos) const = 0;

        /// enumerate resolutions for given display
        virtual void enumResolutions(uint32_t displayIndex, base::Array<ResolutionInfo>& outResolutions) const = 0;

        /// enumerate vsync modes
        virtual void enumVSyncModes(uint32_t displayIndex, base::Array<ResolutionSyncInfo>& outVSyncModes) const = 0;

        /// enumerate supported refresh rates for given resolution
        virtual void enumRefreshRates(uint32_t displayIndex, const ResolutionInfo& info, base::Array<int>& outRefreshRates) const = 0;

		//---

		/// schedule asynchronous (background) copying of data to a buffer/image
		/// NOTE: me must guarantee that that part of the resource is NOT used or we may get visual glitches
		/// NOTE: resource can't be deleted while a copy is scheduled
		/// NOTE: once the copy is finished a fence is signaled (usually this or next frame)
		virtual bool asyncCopy(const IDeviceObject* object, const ResourceCopyRange& range, const ISourceDataProvider* sourceData, base::fibers::WaitCounter initializationFinished = base::fibers::WaitCounter()) = 0;

        //---

        /// Submit a command buffer (with potential child command buffers) to execution
        /// NOTE: this builds actual native command buffers required for rendering the scene
        /// NOTE: this is an async call that completes right away but the work is not done and if enough of it queries that we will wait in the advanceFrame()
        virtual void submitWork(command::CommandBuffer* commandBuffer, bool background=false) = 0;

        //--

	protected:
		bool validateBufferCreationSetup(const BufferCreationInfo& info, BufferObject::Setup& outSetup) const;
		bool validateImageCreationSetup(const ImageCreationInfo& info, ImageObject::Setup& outSetup) const;
    };

} // rendering

