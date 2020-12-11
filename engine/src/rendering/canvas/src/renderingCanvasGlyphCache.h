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
#include "base/font/include/fontGlyph.h"

namespace rendering
{
    namespace canvas
    {
		///----

        /// GPU array texture holding all the glyphs
        class RENDERING_CANVAS_API CanvasGlyphCache : public base::NoCopy
        {
        public:
			CanvasGlyphCache(IDevice* dev);
            ~CanvasGlyphCache();

			//--

			// atlas image
			INLINE const ImageSampledViewPtr& atlasView() const { return m_atlasImageSRV; }

            //--

			// send updates to GPU texture
			void flushUpdate(command::CommandWriter& cmd);

			//---

			// map glyph into atlas, if glyph is not found it's added
			const base::canvas::ImageAtlasEntryInfo* findRenderDataForGlyph(const base::font::Glyph* glyph);

			//--

        private:
			float m_invAtlasSize = 0.0f;
			float m_texelOffset = 0.0f;

			ImageObjectPtr m_atlasImage;
			ImageSampledViewPtr m_atlasImageSRV;

			//--

			struct Page
			{
				base::image::DynamicAtlas image;
				base::Rect dirtyRect;

				Page(uint32_t size);
			};

			struct Entry
			{
				const base::font::Glyph* glyph = nullptr;
				base::canvas::ImageAtlasEntryInfo placement;
			};

			base::Array<Page> m_pages;
			base::HashMap<base::font::GlyphID, Entry> m_entriesMap;

			//--
        };

        ///---

    } // canvas
} // rendering