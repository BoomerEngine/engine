/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#pragma once

#include "base/containers/include/rectAllocator.h"
#include "base/image/include/imageDynamicAtlas.h"
#include "base/canvas/include/canvasStorage.h"

namespace rendering
{
    namespace canvas
    {
		///----

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

        ///---

        /// GPU array texture based image atlas
        class RENDERING_CANVAS_API CanvasImageCache : public base::NoCopy
        {
        public:
			CanvasImageCache(IDevice* dev, ImageFormat imageFormat, uint32_t size, uint32_t pageCount, uint32_t maxEntries = INDEX_MAX);
            ~CanvasImageCache();

			//--

			// atlas image
			INLINE const ImageSampledViewPtr& atlasView() const { return m_atlasImageSRV; }

			// buffer with image informations (mainly UV wrapping)
			INLINE const BufferStructuredViewPtr& atlasInfo() const { return m_atlasInfoBufferSRV; }

            //--

			// register image
			base::canvas::ImageEntry registerImage(const base::image::Image* ptr, bool supportWrapping, int additionalPixelBorder);

			// unregister image
			void unregisterImage(base::canvas::ImageEntry entry);

			// find image data
			const base::canvas::ImageAtlasEntryInfo* fetchImageData(uint32_t entryIndex) const;

            //--

			// send updates to GPU texture
			void flushUpdate(command::CommandWriter& cmd);

			//--

        private:
			uint16_t m_maxEntries = 0;

			float m_invAtlasWidth = 0.0f;
			float m_invAtlasHeight = 0.0f;

			ImageObjectPtr m_atlasImage;
			ImageSampledViewPtr m_atlasImageSRV;

			BufferObjectPtr m_atlasInfoBuffer;
			BufferStructuredViewPtr m_atlasInfoBufferSRV;

			uint16_t m_dirtyEntryInfoMin = 0;
			uint16_t m_dirtyEntryInfoMax = 0;

			//--

			struct Entry
			{
				base::image::ImagePtr data;

				uint8_t supportWrapping = 0;
				uint8_t additionalPixelBorder = 0;
				
				base::canvas::ImageAtlasEntryInfo placement;
			};

			struct Page
			{
				base::image::DynamicAtlas image;
				base::Rect dirtyRect;

				Page(uint32_t size);
			};

			base::Array<Page> m_pages;

			base::Array<uint16_t> m_freeEntryIndices;
			base::Array<Entry> m_entries;            

			//--

			bool allocateEntryIndex(uint32_t& outIndex);
			bool placeImage(const Entry& source, base::canvas::ImageAtlasEntryInfo& outPlacement);

			//--
        };

        ///---

    } // canvas
} // rendering