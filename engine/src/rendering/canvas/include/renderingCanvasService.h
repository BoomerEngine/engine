/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#pragma once

#include "renderingCanvasBatchRenderer.h"
#include "base/canvas/include/canvasService.h"

namespace rendering
{
    namespace canvas
    {

		//---

		class CanvasRenderer;
		struct CanvasRenderStates;

		//---

#pragma pack(push)
#pragma pack(4)
		struct GPUCanvasImageInfo
		{
			base::Vector2 uvMin;
			base::Vector2 uvMax;
			base::Vector2 uvScale;
			base::Vector2 uvInvScale;
		};
#pragma pack(pop)

		//---

		struct RENDERING_CANVAS_API CanvasRenderAtlasImage
		{
			uint32_t version = 0;

			ImageObjectPtr atlasImage;
			ImageSampledViewPtr atlasImageSRV;

			BufferObjectPtr atlasInfoBuffer;
			BufferStructuredViewPtr atlasInfoBufferSRV;

			CanvasRenderAtlasImage(ImageFormat format, uint32_t size, uint32_t numPages);

			void updateImage(command::CommandWriter& cmd, const base::image::ImageView& sourceImage, uint8_t pageIndex, const base::Rect& dirtyRect);
			void updateEntries(command::CommandWriter& cmd, uint32_t firstEntry, uint32_t numEntries, const base::canvas::ImageAtlasEntryInfo* entiresData);
		};

		//---

        /// shared storage of canvas related data, mainly image atlases
        class RENDERING_CANVAS_API CanvasRenderService : public base::app::ILocalService, public base::canvas::ICanvasAtlasSync
        {
			RTTI_DECLARE_VIRTUAL_CLASS(CanvasRenderService, base::app::ILocalService);

        public:
			CanvasRenderService();
            virtual ~CanvasRenderService();

			//--

			// render canvas to a given command buffer
			// NOTE: we are expected to be inside a pass
			void render(command::CommandWriter& cmd, const base::canvas::Canvas& canvas);

			//--

        private:
			bool m_allowImages = false;

			CanvasRenderAtlasImage* m_glyphCache = nullptr;
			base::InplaceArray<CanvasRenderAtlasImage*, 8> m_imageCaches;

			CanvasRenderStates* m_renderStates = nullptr;

			//--

			BufferObjectPtr m_sharedVertexBuffer;
			BufferObjectPtr m_sharedAttributesBuffer;
			BufferStructuredViewPtr m_sharedAttributesBufferSRV;

			BufferObjectPtr m_emptyAtlasEntryBuffer;
			BufferStructuredViewPtr m_emptyAtlasEntryBufferSRV;

			uint32_t m_maxBatchVetices = 0;
			uint32_t m_maxAttributes = 0;

			//--

			IDevice* m_device = nullptr;

            base::Array<ICanvasBatchRenderer*> m_batchRenderers;

			//--

			void createRenderStates();
			void createBatchRenderers();

			friend class CanvasRenderer;

			//--

			virtual base::app::ServiceInitializationResult onInitializeService(const base::app::CommandLine& cmdLine) override final;
			virtual void onShutdownService() override final;
			virtual void onSyncUpdate() override final;

			//--

			virtual void updateGlyphCache(rendering::command::CommandWriter& cmd, const base::canvas::ICanvasAtlasSync::AtlasUpdate& update) override final;
			virtual void updateAtlas(rendering::command::CommandWriter& cmd, base::canvas::ImageAtlasIndex atlasIndex, const base::canvas::ICanvasAtlasSync::AtlasUpdate& update) override final;
        };

        //--

    } // canvas
} // rendering