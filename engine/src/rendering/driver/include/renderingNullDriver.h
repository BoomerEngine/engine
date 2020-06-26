/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface #]
***/

#pragma once

#include "renderingDriver.h"

namespace rendering
{
    // empty implementation of the rendering driver
    class RENDERING_DRIVER_API NullDriver : public IDriver
    {
        RTTI_DECLARE_VIRTUAL_CLASS(NullDriver, IDriver);

    public:
        NullDriver();
        virtual ~NullDriver();

        /// IDriver interface
        virtual base::StringBuf runtimeDescription() const override final;
        virtual base::Point maxRenderTargetSize() const override final;
        virtual bool isVerticalFlipRequired() const override final;
        virtual bool supportsAsyncCommandBufferBuilding() const override final;
        virtual bool initialize(const base::app::CommandLine& cmdLine) override final;
        virtual void shutdown() override final;
        virtual void sync() override final;
        virtual void advanceFrame() override final;

        virtual ObjectID createOutput(const DriverOutputInitInfo& info) override final;
        virtual ObjectID createShaders(const ShaderLibraryData* shaderLibraryData) override final;
        virtual BufferView createBuffer(const BufferCreationInfo& info, const SourceData* initializationData = nullptr) override final;
        virtual ImageView createImage(const ImageCreationInfo& info, const SourceData* sourceData = nullptr) override final;
        virtual ObjectID createSampler(const SamplerState& info) override final;
        virtual void releaseObject(ObjectID id) override final;
        
        virtual void enumMonitorAreas(base::Array<base::Rect>& outMonitorAreas) const override final;
        virtual void enumDisplays(base::Array<DriverDisplayInfo>& outDisplayInfos) const override final;
        virtual void enumResolutions(uint32_t displayIndex, base::Array<DriverResolutionInfo>& outResolutions) const override final;
        virtual void enumVSyncModes(uint32_t displayIndex, base::Array<DriverResolutionSyncInfo>& outVSyncModes) const override final;
        virtual void enumRefreshRates(uint32_t displayIndex, const DriverResolutionInfo& info, base::Array<int>& outRefreshRates) const override final;

        virtual IDriverNativeWindowInterface* queryOutputWindow(ObjectID output) const override final;
        virtual bool prepareOutputFrame(ObjectID output, DriverOutputFrameInfo& outFrameInfo) override final;

        virtual void submitWork(command::CommandBuffer* commandBuffer, bool background = false) override final;
    };

} // rendering

