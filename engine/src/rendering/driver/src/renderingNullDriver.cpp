/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface #]
***/

#include "build.h"
#include "renderingNullDriver.h"
#include "renderingObject.h"
#include "renderingCommandBuffer.h"

namespace rendering
{

    RTTI_BEGIN_TYPE_CLASS(NullDriver);
        RTTI_METADATA(DriverNameMetadata).name("Null");
    RTTI_END_TYPE();

    NullDriver::NullDriver()
    {}

    NullDriver::~NullDriver()
    {}

    base::StringBuf NullDriver::runtimeDescription() const
    {
        return "Null Rendering";
    }

    bool NullDriver::isVerticalFlipRequired() const
    {
        return false;
    }

    bool NullDriver::supportsAsyncCommandBufferBuilding() const
    {
        return false;
    }

    bool NullDriver::initialize(const base::app::CommandLine& cmdLine)
    {
        TRACE_INFO("Null rendering driver initialized");
        ChangeActiveDevice(this);
        return true;
    }

    void NullDriver::shutdown()
    {
        TRACE_INFO("Null rendering driver shut down");
        ChangeActiveDevice(nullptr);
    }

    base::Point NullDriver::maxRenderTargetSize() const
    {
        return base::Point(1920, 1080);
    }

    void NullDriver::advanceFrame()
    {}

    void NullDriver::sync()
    {
        // nothing
    }

    void NullDriver::enumMonitorAreas(base::Array<base::Rect>& outMonitorAreas) const
    {}

    void NullDriver::enumDisplays(base::Array<DriverDisplayInfo>& outDisplayInfos) const
    {}

    void NullDriver::enumResolutions(uint32_t displayIndex, base::Array<DriverResolutionInfo>& outResolutions) const
    {}

    void NullDriver::enumVSyncModes(uint32_t displayIndex, base::Array<DriverResolutionSyncInfo>& outVSyncModes) const
    {}

    void NullDriver::enumRefreshRates(uint32_t displayIndex, const DriverResolutionInfo& info, base::Array<int>& outRefreshRates) const
    {}

    ObjectID NullDriver::createOutput(const DriverOutputInitInfo& info)
    {
        return ObjectID();
    }

    ObjectID NullDriver::createShaders(const ShaderLibraryData* shaderLibraryData)
    {
        return ObjectID();
    }

    BufferView NullDriver::createBuffer(const BufferCreationInfo& info, const SourceData* initializationData)
    {
        return BufferView();
    }

    ImageView NullDriver::createImage(const ImageCreationInfo& info, const SourceData* sourceData)
    {
        return ImageView();
    }

    ObjectID NullDriver::createSampler(const SamplerState& info)
    {
        return ObjectID();
    }

    void NullDriver::releaseObject(ObjectID id)
    {

    }

    IDriverNativeWindowInterface* NullDriver::queryOutputWindow(ObjectID output) const
    {
        return nullptr;
    }

    bool NullDriver::prepareOutputFrame(ObjectID output, DriverOutputFrameInfo& outFrameInfo)
    {
        return false;
    }

    void NullDriver::submitWork(command::CommandBuffer* commandBuffer, bool background /*= false*/)
    {
        if (commandBuffer)
            commandBuffer->release();
    }

} // rendering
