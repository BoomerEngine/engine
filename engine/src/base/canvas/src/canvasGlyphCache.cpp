/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: renderer #]
***/

#include "build.h"
#include "canvasGlyphCache.h"
#include "base/image/include/image.h"
#include "base/image/include/imageView.h"
#include "base/system/include/scopeLock.h"
#include "base/system/include/algorithms.h"

namespace base
{
    namespace canvas
    {
        //--

        ConfigProperty<uint32_t> cvGlyphCachePageSize("Engine.GlyphCache", "PageSize", 2040); // NOTE: must be a tid bit smaller than 2048
        ConfigProperty<uint32_t> cvGlyphCacheNumInitialPages("Engine.GlyphCache", "NumInitialPages", 1);

        //--

        GlyphCache::GlyphCache()
            : m_glyphCacheVersion(1)
        {
            reset();
        }

        void GlyphCache::deinit()
        {
            m_pages.clear();
            m_glyphMap.clear();

        }

        image::ImagePtr GlyphCache::page(uint32_t pageIndex) const
        {
            auto lock = CreateLock(m_lock);
            DEBUG_CHECK(pageIndex < m_pages.size());
            return m_pages[pageIndex].image();
        }

        void GlyphCache::reset()
        {
            m_pages.clear();
            m_pages.reserve(256); // overkill
            for (uint32_t i = 0; i < cvGlyphCacheNumInitialPages.get(); ++i)
                m_pages.emplaceBack(cvGlyphCachePageSize.get(), cvGlyphCachePageSize.get(), 1, 2);

            m_glyphMap.clear();
            m_glyphMap.reserve(4096);
        }

        bool GlyphCache::beingUpdate(uint32_t& version)
        {
            if (m_glyphCacheVersion != version)
            {
                version = m_glyphCacheVersion;
                m_lock.acquire();
                return true;
            }
            else
            {
                return false;
            }
        }

        void GlyphCache::endUpdate()
        {
            m_lock.release();
        }

        GlyphPlacement GlyphCache::mapGlyph(const font::Glyph& srcGlyph)
        {
            // fetch glyph data from cache
            auto cachedGlyph  = m_glyphMap.find(srcGlyph.id());
            if (cachedGlyph != nullptr)
                return *cachedGlyph;

            // try to place the glyph in the cache
            auto numPages = m_pages.size();
            for (uint32_t i = 0; i < numPages; ++i)
            {
                image::DynamicAtlasEntry atlasEntry;
                if (m_pages[i].placeImage(srcGlyph.bitmap()->view(), atlasEntry))
                {
                    GlyphPlacement placedGlyph;
                    placedGlyph.pageIndex = range_cast<uint8_t>(i);
                    placedGlyph.uvStart = atlasEntry.uvStart;
                    placedGlyph.uvEnd = atlasEntry.uvEnd;
                    m_glyphMap.set(srcGlyph.id(), placedGlyph);
                    return placedGlyph;
                }
            }
            
            // TODO: resize ?

            TRACE_INFO("Glyph '{}' does not fit glyph cache", srcGlyph.id().glyphId);

            // does not fit
            return GlyphPlacement();
        }

        //--

    } // canvas
} // base
