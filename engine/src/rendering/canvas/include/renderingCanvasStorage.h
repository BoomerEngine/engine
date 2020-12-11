/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#pragma once

#include "base/canvas/include/canvasStorage.h"
#include "renderingCanvasBatchRenderer.h"

namespace rendering
{
    namespace canvas
    {

		//---

        class CanvasImageCache;
		class CanvasGlyphCache;
		class CanvasRenderer;
		struct CanvasRenderStates;

		//---

        /// shared storage of canvas related data, mainly image atlases
        class RENDERING_CANVAS_API CanvasStorage : public base::canvas::IStorage
        {
			RTTI_DECLARE_VIRTUAL_CLASS(CanvasStorage, base::canvas::IStorage);

        public:
			CanvasStorage(IDevice* dev);
            virtual ~CanvasStorage();

			//--

			// base::canvas::IStorage
			virtual void conditionalRebuild(bool* outAtlasesWereRebuilt) override final;

			virtual base::canvas::ImageAtlasIndex createAtlas(uint32_t pageSize, uint32_t numPages, base::StringView debugName) override final;
			virtual void destroyAtlas(base::canvas::ImageAtlasIndex atlas) override final;

			virtual base::canvas::ImageEntry registerImage(base::canvas::ImageAtlasIndex atlasIndex, const base::image::Image* ptr, bool supportWrapping, int additionalPixelBorder) override final;
			virtual void unregisterImage(base::canvas::ImageEntry entry) override final;

			virtual const base::canvas::ImageAtlasEntryInfo* findRenderDataForAtlasEntry(const base::canvas::ImageEntry& entry) const override final;
			virtual const base::canvas::ImageAtlasEntryInfo* findRenderDataForGlyph(const base::font::Glyph* glyph) const override final;

			//--

        private:
			bool m_allowImages = false;

			CanvasGlyphCache* m_glyphCache = nullptr;
			base::InplaceArray<CanvasImageCache*, 8> m_imageCaches;

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

			void flushDataChanges(command::CommandWriter& cmd) const;

			void createRenderStates();
			void createBatchRenderers();

			friend class CanvasRenderer;
        };

        //--

    } // canvas
} // rendering