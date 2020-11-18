/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface #]
***/

#include "build.h"
#include "renderingNullDevice.h"
#include "renderingObject.h"
#include "renderingCommandBuffer.h"

namespace rendering
{

    RTTI_BEGIN_TYPE_CLASS(NullDevice);
        RTTI_METADATA(DeviceNameMetadata).name("Null");
    RTTI_END_TYPE();

    NullDevice::NullDevice()
    {}

    NullDevice::~NullDevice()
    {}

    base::StringBuf NullDevice::name() const
    {
        return "Null Rendering";
    }

    bool NullDevice::initialize(const base::app::CommandLine& cmdLine)
    {
        TRACE_INFO("Null rendering device initialized");
        return true;
    }

    void NullDevice::shutdown()
    {
        TRACE_INFO("Null rendering device shut down");
    }

    base::Point NullDevice::maxRenderTargetSize() const
    {
        return base::Point(1920, 1080);
    }

    void NullDevice::advanceFrame()
    {}

    void NullDevice::sync()
    {
        // nothing
    }

    void NullDevice::enumMonitorAreas(base::Array<base::Rect>& outMonitorAreas) const
    {}

    void NullDevice::enumDisplays(base::Array<DisplayInfo>& outDisplayInfos) const
    {}

    void NullDevice::enumResolutions(uint32_t displayIndex, base::Array<ResolutionInfo>& outResolutions) const
    {}

    void NullDevice::enumVSyncModes(uint32_t displayIndex, base::Array<ResolutionSyncInfo>& outVSyncModes) const
    {}

    void NullDevice::enumRefreshRates(uint32_t displayIndex, const ResolutionInfo& info, base::Array<int>& outRefreshRates) const
    {}

    OutputObjectPtr NullDevice::createOutput(const OutputInitInfo& info)
    {
        return nullptr;
    }

    ShaderObjectPtr NullDevice::createShaders(const ShaderLibraryData* shaderLibraryData)
    {
        return nullptr;
    }

    BufferObjectPtr NullDevice::createBuffer(const BufferCreationInfo& info, const SourceData* initializationData)
    {
        return nullptr;
    }

    ImageObjectPtr NullDevice::createImage(const ImageCreationInfo& info, const SourceData* sourceData)
    {
        return nullptr;
    }

    SamplerObjectPtr NullDevice::createSampler(const SamplerState& info)
    {
        return nullptr;
    }

    void NullDevice::submitWork(command::CommandBuffer* commandBuffer, bool background /*= false*/)
    {
        if (commandBuffer)
            commandBuffer->release();
    }

} // rendering
