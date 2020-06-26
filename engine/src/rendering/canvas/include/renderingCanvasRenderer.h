/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#pragma once

#include "base/canvas/include/canvas.h"
#include "base/containers/include/inplaceArray.h"
#include "base/system/include/mutex.h"

#include "rendering/driver/include/renderingConstantsView.h"
#include "rendering/driver/include/renderingDriver.h"
#include "rendering/driver/include/renderingDeviceObject.h"

#include "base/containers/include/rectAllocator.h"

#include "renderingCanvasRendererCustomBatchHandler.h"

namespace rendering
{
    namespace canvas
    {

        ///---

        class CanvasImageCache;
        struct CanvasShaderParams;
        struct CanvasImageCacheEntry;

        /// canvas renderer - hosts all necessary shaders and data to render canvas
        /// NOTE: this class must be externally synchronized as it's single thread only
        class RENDERING_CANVAS_API CanvasRenderer : public IDeviceObject
        {
        public:
            CanvasRenderer();
            virtual ~CanvasRenderer();

            /// render canvas content into current render pass 
            void render(command::CommandWriter& cmd, const base::canvas::Canvas& canvas, const CanvasRenderingParams& params);

        private:
            base::UniquePtr<CanvasImageCache> m_alphaImageCache; // fonts other "iconic" white stuff
            base::UniquePtr<CanvasImageCache> m_rgbaImageCache; // images
            
            base::Array<CanvasShaderParams> m_paramDataStorage;
            base::Array<CanvasImageCacheEntry> m_colorImages;
            base::Array<CanvasImageCacheEntry> m_alphaImages;

            ICanvasRendererCustomBatchHandler** m_customHandlersFlatList;
            base::Array<base::RefPtr<ICanvasRendererCustomBatchHandler>> m_customHandlers;

            void packParameters(const base::canvas::Canvas::Params& params, CanvasShaderParams& outParams) const;

            //--

            struct Batch
            {
                uint32_t firstIndex = 0;
                uint32_t numIndices = 0;
                uint32_t numPayloads = 0;
                uint32_t firstPayload = 0;
                uint16_t customDrawer = 0;

                base::canvas::CompositeOperation blendOp = base::canvas::CompositeOperation::SourceOver;
                base::canvas::Canvas::BatchType shader = base::canvas::Canvas::BatchType::ConvexFill;
            };

            class BatchCollector
            {
            public:
                BatchCollector();

                void push(const base::canvas::Canvas::Batch& b);
                void finish();

                INLINE const base::Array<Batch>& batches() const { return m_batches; }
                INLINE const base::Array<CanvasCustomBatchPayload>& payloads() const { return m_customPayloads; }

            private:
                base::canvas::Canvas::BatchType m_currentShader = base::canvas::Canvas::BatchType::ConvexFill;
                base::canvas::CompositeOperation m_currentOp = base::canvas::CompositeOperation::SourceOver;;

                base::InplaceArray<Batch, 512> m_batches;
                base::InplaceArray<CanvasCustomBatchPayload, 512> m_customPayloads;

                uint32_t m_currentStartIndex = 0;
                uint32_t m_currentStartPayload = 0;
                uint32_t m_currentIndexCount = 0;
                int m_currentCustomHandlerType = 0;
            };

            //--

            void render(command::CommandWriter& cmd, const base::canvas::Canvas& geometry, const ImageView& colorTarget, const ImageView& depthTarget) const;

            //--

            virtual void handleDeviceReset() override final;
            virtual void handleDeviceRelease() override final;
            virtual base::StringBuf describe() const override;

            void createDeviceResources();
            void destroyDeviceResources();
            void createCustomHandlers();
        };

        //--

    } // canvas
} // rendering