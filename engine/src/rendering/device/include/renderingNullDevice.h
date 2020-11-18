/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface #]
***/

#pragma once

#include "renderingDeviceApi.h"

namespace rendering
{
    // empty implementation of the rendering device
    class RENDERING_DEVICE_API NullDevice : public IDevice
    {
        RTTI_DECLARE_VIRTUAL_CLASS(NullDevice, IDevice);

    public:
        NullDevice();
        virtual ~NullDevice();

        /// IDevice interface
        virtual base::StringBuf name() const override final;
        virtual base::Point maxRenderTargetSize() const override final;
        virtual bool initialize(const base::app::CommandLine& cmdLine) override final;
        virtual void shutdown() override final;
        virtual void sync() override final;
        virtual void advanceFrame() override final;

        virtual OutputObjectPtr createOutput(const OutputInitInfo& info) override final;
        virtual ShaderObjectPtr createShaders(const ShaderLibraryData* shaderLibraryData) override final;
        virtual BufferObjectPtr createBuffer(const BufferCreationInfo& info, const SourceData* initializationData = nullptr) override final;
        virtual ImageObjectPtr createImage(const ImageCreationInfo& info, const SourceData* sourceData = nullptr) override final;
        virtual SamplerObjectPtr createSampler(const SamplerState& info) override final;
        
        virtual void enumMonitorAreas(base::Array<base::Rect>& outMonitorAreas) const override final;
        virtual void enumDisplays(base::Array<DisplayInfo>& outDisplayInfos) const override final;
        virtual void enumResolutions(uint32_t displayIndex, base::Array<ResolutionInfo>& outResolutions) const override final;
        virtual void enumVSyncModes(uint32_t displayIndex, base::Array<ResolutionSyncInfo>& outVSyncModes) const override final;
        virtual void enumRefreshRates(uint32_t displayIndex, const ResolutionInfo& info, base::Array<int>& outRefreshRates) const override final;

        virtual void submitWork(command::CommandBuffer* commandBuffer, bool background = false) override final;
    };

} // rendering

