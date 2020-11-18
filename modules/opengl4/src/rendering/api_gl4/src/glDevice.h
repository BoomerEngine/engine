/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/api_common/include/renderingWindow.h"
#include "base/containers/include/hashSet.h"
#include "base/system/include/spinLock.h"

namespace rendering
{
    namespace gl4
    {
        // a OpenGL 4 implementation of the rendering driver
        class Device : public IDevice
        {
            RTTI_DECLARE_VIRTUAL_CLASS(Device, IDevice);

        public:
            Device();
            virtual ~Device();

            //---

            // pool for temporary constant buffers
            INLINE TempBufferPool& costantsBufferPool() const { return *m_bufferPoolConstants; }

            // pool for temporary staging buffers
            INLINE TempBufferPool& stagingBufferPool() const { return *m_bufferPoolStaging; }

            // get cache for various runtime objects
            INLINE ObjectCache& objectCache() const { return *m_objectCache; }

            // object registry (all API objects are here)
            INLINE ObjectRegistry& objectRegistry() const { return *m_objectRegistry; }

            //--

            virtual base::StringBuf name() const override final;
            virtual base::Point maxRenderTargetSize() const override final;
            virtual bool initialize(const base::app::CommandLine& cmdLine) override final;
            virtual void shutdown() override final;
            virtual void sync() override final;
            virtual void advanceFrame() override final;

            virtual OutputObjectPtr createOutput(const OutputInitInfo& info) override final;
            virtual BufferObjectPtr createBuffer(const BufferCreationInfo& info, const SourceData* sourceData = nullptr) override final;
            virtual ShaderObjectPtr createShaders(const ShaderLibraryData* shaders) override final;
            virtual ImageObjectPtr createImage(const ImageCreationInfo& info, const SourceData* sourceData = nullptr) override final;
            virtual SamplerObjectPtr createSampler(const SamplerState& info) override final;

            virtual void submitWork(command::CommandBuffer* masterCommandBuffer, bool background) override final;

            virtual void enumMonitorAreas(base::Array<base::Rect>& outMonitorAreas) const override final;
            virtual void enumDisplays(base::Array<DisplayInfo>& outDisplayInfos) const override final;
            virtual void enumResolutions(uint32_t displayIndex, base::Array<ResolutionInfo>& outResolutions) const override final;
            virtual void enumVSyncModes(uint32_t displayIndex, base::Array<ResolutionSyncInfo>& outVSyncModes) const override final;
            virtual void enumRefreshRates(uint32_t displayIndex, const ResolutionInfo& info, base::Array<int>& outRefreshRates) const override final;

            //--

            // resolve a view to predefined image
            ResolvedImageView resolvePredefinedImage(const ImageView& view) const;

            // resolve a view of predefined sampler
            GLuint resolvePredefinedSampler(uint32_t id) const;

            //--

        private:
            //----

            ObjectCache* m_objectCache = nullptr;
            ObjectRegistry* m_objectRegistry = nullptr;
            TempBufferPool* m_bufferPoolStaging = nullptr;
            TempBufferPool* m_bufferPoolConstants = nullptr;

            DeviceThread* m_thread = nullptr;
            WindowManager* m_windows = nullptr;

            //-----

            base::StringBuf m_desc;

            Image* m_predefinedImages[128];
            GLuint m_predefinedSamplers[128];

            void createPredefinedImages();
            void createPredefinedSamplers();

            void createPredefinedImageFromColor(uint32_t id, const base::Color fillColor, const ImageFormat format, const char* debugName);
            void createPredefinedSampler(uint32_t id, const SamplerState& info, const char* debugName);
            void createPredefinedRenderTarget(uint32_t id, const ImageFormat format, uint32_t numSlices, const char* debugName);

            //-----
        };

    } // gl4
} // rendering