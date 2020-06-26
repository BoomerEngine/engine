/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#pragma once

#include "base/containers/include/rectAllocator.h"
#include "rendering/driver/include/renderingImageView.h"

namespace rendering
{
    namespace canvas
    {

        ///---

        /// entry in the canvas image cache
        struct CanvasImageCacheEntry
        {
            const base::image::Image* m_image;
            base::Vector2 m_uvOffset;
            base::Vector2 m_uvScale;
            base::Point m_placement;
            uint8_t m_layerIndex = 0;
            bool m_needsWrapping = false;
        };

        /// a simple image cache for canvas rendering
        class RENDERING_CANVAS_API CanvasImageCache : public base::NoCopy
        {
        public:
            CanvasImageCache(IDriver* drv, ImageFormat imageFormat, uint32_t size, uint32_t initialPageCount);
            ~CanvasImageCache();

            //--

            // the texture with all the images
            INLINE const ImageView& view() const { return m_image; }

            //--

            // purge cache
            void purge();

            // cache images in our atlas pages
            // NOTE: atlas may be reallocated AT WORST
            bool cacheImages(command::CommandWriter& cmd, CanvasImageCacheEntry* entries, uint32_t numEntries);

        private:
            struct ImagePlacement
            {
                uint32_t m_lastVersion;
                uint16_t m_pageIndex;
                bool m_supportsWrapping;
                base::Point m_placement;
                base::Point m_size;
                base::Vector2 m_uvOffset;
                base::Vector2 m_uvScale;
            };

            base::Array<base::RectAllocator> m_pageAllocators;

            base::HashMap<uint32_t, ImagePlacement> m_imageEntries;

            IDriver* m_device;

            uint32_t m_size;

            float m_invAtlasWidth;
            float m_invAtlasHeight;

            ImageView m_image;

            uint32_t m_numPages;
            ImageFormat m_format;

            //--

            static base::image::ImagePtr CreateWrapEnabledImage(const base::image::ImageView& src, uint32_t margins);

            //--

            bool findPlacement(uint32_t width, uint32_t height, bool needsWrapping, ImagePlacement& outPlacement);
            void entryFromPlacement(const ImagePlacement& placement, CanvasImageCacheEntry& entry);

            bool reinitializeWithImages(CanvasImageCacheEntry* entries, uint32_t numEntries, base::Array<CanvasImageCacheEntry*>& outEntriesToCopyToImage);
            bool placeImages(CanvasImageCacheEntry* entries, uint32_t numEntries, base::Array<CanvasImageCacheEntry*>& outEntriesToCopyToImage);

            void destroyImage();
            bool createImage(uint32_t pageCount);
        };

        ///---

    } // canvas
} // rendering