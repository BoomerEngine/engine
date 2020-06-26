/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: renderer #]
***/

#pragma once

#include "base/containers/include/hashMap.h"
#include "base/image/include/imageDynamicAtlas.h"
#include "base/font/include/fontGlyph.h"
#include "base/system/include/spinLock.h"
#include "base/system/include/mutex.h"

namespace base
{
    namespace canvas
    {

        //--

        /// information about placed glyph
        struct GlyphPlacement
        {
            Vector2 uvStart;
            Vector2 uvEnd;
            uint8_t pageIndex = 0;
        };

        //--

        /// image atlas manager for font glyphs
        class BASE_CANVAS_API GlyphCache : public ISingleton
        {
            DECLARE_SINGLETON(GlyphCache);

        public:
            //--

            //--

            // request access for updating stuff using the cache, returns true if data should be updated (call endUpdate then)
            bool beingUpdate(uint32_t& version);

            // unlock after use
            void endUpdate();

            /// locate glyph in the cache, if glyph is not in the cache it's added
            /// NOTE: adding a glyph to the cache requires the modified dynamic textures to be uploaded to the GPU
            GlyphPlacement mapGlyph(const font::Glyph& srcGlyph);

            //--

            // clear all glyphs from the cache
            void reset();

            //--

            // get image from the glyph cache
            image::ImagePtr page(uint32_t pageIndex) const;

        private:
            GlyphCache();

            //--

            Array<image::DynamicAtlas> m_pages;

            typedef HashMap<font::GlyphID, GlyphPlacement> TGlyphMap;
            TGlyphMap m_glyphMap;

            uint32_t m_glyphCacheVersion;

            Mutex m_lock;

            //--

            virtual void deinit() override final;
        };

        //--

    } // canvas
} // base