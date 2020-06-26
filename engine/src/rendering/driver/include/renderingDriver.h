/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface #]
***/

#pragma once

#include "renderingBufferView.h"
#include "renderingImageView.h"

#include "base/object/include/rttiMetadata.h"

namespace rendering
{

    // driver class identification (GL4, Vulkan, DX12, etc)
    class RENDERING_DRIVER_API DriverNameMetadata : public base::rtti::IMetadata
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DriverNameMetadata, base::rtti::IMetadata);

    public:
        DriverNameMetadata();

        INLINE DriverNameMetadata& name(const char* name)
        {
            m_name = name;
            return *this;
        }

        INLINE const char* name() const { return m_name; }

    private:
        const char* m_name;
    };

    // driver payload performance stats
    struct RENDERING_DRIVER_API DriverPerformanceStats : public base::IReferencable
    {
        uint32_t m_numLogicalCommandBuffers = 0;
        uint32_t m_numNativeCommandBuffers = 0;

        uint32_t m_numCommands = 0;
        uint32_t m_numPasses = 0;
        uint32_t m_numDrawCalls = 0;
        uint32_t m_numDispatchCalls = 0;
        uint32_t m_numTriangles = 0;

        float m_totalGPUTime = 0.0f;
        float m_totalCPUTime = 0.0f;

        float m_executionTime = 0.0f;
        float m_prepareTime = 0.0f;
        float m_presentTime = 0.0f;
        float m_uploadTime = 0.0f;

        uint32_t m_uploadTotalSize = 0;
        uint32_t m_uploadTransientBufferCount = 0;
        uint32_t m_uploadTransientBufferSize = 0;
        uint32_t m_uploadDynamicBufferCount = 0;
        uint32_t m_uploadDynamicBufferSize = 0;
        uint32_t m_uploadConstantsCount = 0;
        uint32_t m_uploadConstantsSize = 0;
        uint32_t m_uploadParametersCount = 0;
    };

    // display information
    struct DriverDisplayInfo
    {
        base::StringBuf name;
        uint32_t desktopWidth = 0;
        uint32_t desktopHeight = 0;
        bool primary = false;
        bool attached = false;
        bool active = false;
    };

    // resolution information
    struct DriverResolutionInfo
    {
        uint32_t width = 0;
        uint32_t height = 0;

        INLINE bool operator==(const DriverResolutionInfo& other) const
        {
            return (width == other.width) && (height == other.height);
        }
    };

    // vsync information
    struct DriverResolutionSyncInfo
    {
        base::StringBuf name;
        int value = 0;
    };
    
    // rendering driver implementation
    // this is the most "low-level" API used by the rendering, can be used directly but it's more work
    class RENDERING_DRIVER_API IDriver : public base::NoCopy
    {
        RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IDriver);

    public:
        IDriver();
        virtual ~IDriver();

        /// describe the runtime driver, can contain the version string
        virtual base::StringBuf runtimeDescription() const = 0;

        /// get the maximum size of allowed render targets (usually the size of the biggest monitor)
        virtual base::Point maxRenderTargetSize() const = 0;

        /// is this driver using OpenGL UV mode ? vertical flip of textures ?
        virtual bool isVerticalFlipRequired() const = 0;

        /// does this driver support asynchronous command buffer building ?
        virtual bool supportsAsyncCommandBufferBuilding() const = 0;

        /// initialize the rendering driver, can fail obviously and it should be handled gracefully
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
        virtual ObjectID createOutput(const DriverOutputInitInfo& info) = 0;

        /// prepare shader library for rendering, returns opaque object that represents the shader library on the rendering device's side
        virtual ObjectID createShaders(const ShaderLibraryData* shaderLibraryData) = 0;

        /// create a buffer, optionally fill it with data
        virtual BufferView createBuffer(const BufferCreationInfo& info, const SourceData* sourceData = nullptr) = 0;

        /// create an image, optionally fill it with data (one "SourceData" entry for each mip*slice) 
        virtual ImageView createImage(const ImageCreationInfo& info, const SourceData* sourceData = nullptr) = 0;

        /// create a sampler
        virtual ObjectID createSampler(const SamplerState& info) = 0;

        //---

        /// release object referenced by the ObjectID
        /// NOTE: object is actually freed only after all frames submitted during current sequence (tick) finish on the GPU (which may be a some time from now)
        /// NOTE: the ObjectID is still valid to be used during this frame even that it was released
        virtual void releaseObject(ObjectID id) = 0;

        //---

        /// enumerate the monitor areas of the desktop
        virtual void enumMonitorAreas(base::Array<base::Rect>& outMonitorAreas) const = 0;

        /// enumerate display devices
        virtual void enumDisplays(base::Array<DriverDisplayInfo>& outDisplayInfos) const = 0;

        /// enumerate resolutions for given display
        virtual void enumResolutions(uint32_t displayIndex, base::Array<DriverResolutionInfo>& outResolutions) const = 0;

        /// enumerate vsync modes
        virtual void enumVSyncModes(uint32_t displayIndex, base::Array<DriverResolutionSyncInfo>& outVSyncModes) const = 0;

        /// enumerate supported refresh rates for given resolution
        virtual void enumRefreshRates(uint32_t displayIndex, const DriverResolutionInfo& info, base::Array<int>& outRefreshRates) const = 0;

        //---

        /// get the "window like" interface for given output, allows for tighter UI integration
        /// NOTE: callable and usable from main thread only and only before call to releaseObject() that deletes given output
        virtual IDriverNativeWindowInterface* queryOutputWindow(ObjectID output) const = 0;

        /// Query output status and rendering parameters (format/resolution)
        /// NOTE: if this function returns false than output is dead (closed/lost) and has to be recreated
        virtual bool prepareOutputFrame(ObjectID output, DriverOutputFrameInfo& outFrameInfo) = 0;

        //---

        /// Submit a command buffer (with potential child command buffers) to execution
        /// NOTE: this builds actual native command buffers required for rendering the scene
        /// NOTE: this is an async call that completes right away but the work is not done and if enough of it queries that we will wait in the advanceFrame()
        virtual void submitWork(command::CommandBuffer* commandBuffer, bool background=false) = 0;

    protected:
        static void ChangeActiveDevice(IDriver* device);

        friend class DeviceService;
    };

} // rendering

