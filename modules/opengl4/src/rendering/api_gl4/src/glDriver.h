/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: driver #]
***/

#pragma once

#include "rendering/driver/include/renderingDriver.h"
#include "rendering/api_common/include/renderingWindow.h"
#include "base/containers/include/hashSet.h"
#include "base/system/include/spinLock.h"

namespace rendering
{
    namespace gl4
    {
        class Pipeline;
        class DriverThread;
        class MemoryPool;
        class BufferPool;
        class TransientAllocator;
        class ObjectCache;
        class SequenceFrame;
        class Image;

        struct ResolvedImageView;
        struct ResolvedFormatedView;
        struct ResolvedBufferView;
        struct ResolvedParameterBindingState;
        struct ResolvedVertexBindingState;

        // a OpenGL 4 implementation of the rendering driver
        class Driver : public IDriver
        {
            RTTI_DECLARE_VIRTUAL_CLASS(Driver, IDriver);

        public:
            Driver();
            virtual ~Driver();

            //---

            // get the allocator for the transient objects
            INLINE TransientAllocator& transientAllocator() const { return *m_transientAllocator; }

            // get cache for various runtime objects
            INLINE ObjectCache& objectCache() const { return *m_objectCache; }

            //--

            virtual base::StringBuf runtimeDescription() const override final;
            virtual base::Point maxRenderTargetSize() const override final;
            virtual bool isVerticalFlipRequired() const override final;
            virtual bool supportsAsyncCommandBufferBuilding() const override final;
            virtual bool initialize(const base::app::CommandLine& cmdLine) override final;
            virtual void shutdown() override final;
            virtual void sync() override final;
            virtual void advanceFrame() override final;

            virtual ObjectID createOutput(const DriverOutputInitInfo& info) override final;
            virtual BufferView createBuffer(const BufferCreationInfo& info, const SourceData* sourceData = nullptr) override final;
            virtual ObjectID createShaders(const ShaderLibraryData* shaders) override final;
            virtual ImageView createImage(const ImageCreationInfo& info, const SourceData* sourceData = nullptr) override final;
            virtual ObjectID createSampler(const SamplerState& info) override final;
            virtual void releaseObject(const ObjectID id) override final;
            
            virtual IDriverNativeWindowInterface* queryOutputWindow(ObjectID output) const override final;
            virtual bool prepareOutputFrame(ObjectID output, DriverOutputFrameInfo& outFrameInfo) override final;

            virtual void submitWork(command::CommandBuffer* masterCommandBuffer, bool background) override final;

            virtual void enumMonitorAreas(base::Array<base::Rect>& outMonitorAreas) const override final;
            virtual void enumDisplays(base::Array<DriverDisplayInfo>& outDisplayInfos) const override final;
            virtual void enumResolutions(uint32_t displayIndex, base::Array<DriverResolutionInfo>& outResolutions) const override final;
            virtual void enumVSyncModes(uint32_t displayIndex, base::Array<DriverResolutionSyncInfo>& outVSyncModes) const override final;
            virtual void enumRefreshRates(uint32_t displayIndex, const DriverResolutionInfo& info, base::Array<int>& outRefreshRates) const override final;

            //--

            // resolve a view to predefined image
            ResolvedImageView resolvePredefinedImage(const ImageView& view) const;

            // resolve a view of predefined sampler
            GLuint resolvePredefinedSampler(uint32_t id) const;

            //--

        private:
            //----

            TransientAllocator* m_transientAllocator = nullptr;
            ObjectCache* m_objectCache = nullptr;

            //-----

            DriverThread* m_thread = nullptr;
            WindowManager* m_windows = nullptr;

            //-----

            base::StringBuf m_desc;

            Image* m_predefinedImages[128];
            GLuint m_predefinedSamplers[128];

            void createPredefinedImages();
            void createPredefinedSamplers();

            void createPredefinedImageFromColor(uint32_t id, const base::Color fillColor, const ImageFormat format, const char* debugName);
            void createPredefinedSampler(uint32_t id, const SamplerState& info, const char* debugName);

            //-----
        };

    } // gl4
} // driver