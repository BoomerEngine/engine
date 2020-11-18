/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#include "build.h"
#include "renderingCanvasImageCache.h"

#include "base/image/include/image.h"
#include "base/image/include/imageView.h"
#include "base/image/include/imageUtils.h"
#include "base/system/include/scopeLock.h"

#include "rendering/device/include/renderingCommandBuffer.h"
#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/device/include/renderingObject.h"
#include "rendering/device/include/renderingShaderLibrary.h"
#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingDeviceService.h"

namespace rendering
{
    namespace canvas
    {

        //---

        base::ConfigProperty<uint32_t> cvMaxCanvasImageWidth("Rendering.Canvas", "MaxImageWidth", 2048);
        base::ConfigProperty<uint32_t> cvMaxCanvasImageHeight("Rendering.Canvas", "MaxImageHeight", 2048);

        CanvasImageCache::CanvasImageCache(ImageFormat imageFormat, uint32_t size, uint32_t initialPageCount)
            : m_size(size)
            , m_format(imageFormat)
        {
            createImage(initialPageCount);
        }

        CanvasImageCache::~CanvasImageCache()
        {
            destroyImage();
        }

        void CanvasImageCache::purge()
        {
            if (!m_imageEntries.empty())
            {
                TRACE_WARNING("Purging canvas image cache ({} entries)", m_imageEntries.size());
                m_imageEntries.clear(); // just purge the cache, do not destroy the images
            }
        }

        void CanvasImageCache::destroyImage()
        {
            m_pageAllocators.clear();
            m_imageEntries.clear();

            m_object.reset();
            m_image = ImageView();
        }

        bool CanvasImageCache::createImage(uint32_t pageCount)
        {
            ASSERT(m_image.empty());

            ImageCreationInfo info;
            info.view = ImageViewType::View2DArray;
            info.allowDynamicUpdate = true;
            info.allowShaderReads = true;
            info.format = m_format;
            info.label = "CanvasImageCache";
            info.width = m_size;
            info.height = m_size;
            info.depth = 1;
            info.numMips = 1;
            info.numSlices = pageCount;

            auto* dev = base::GetService<DeviceService>()->device();
            m_object = dev->createImage(info);
            if (!m_object)
            {
                TRACE_ERROR("Failed to create canvas image cache texture {}x{} ({} slices)", info.width, info.height, info.numSlices);
                return false;
            }

            m_image = m_object->view();
            TRACE_INFO("Created canvas image cache texture {}x{} ({} slices)", m_image.width(), m_image.height(), m_image.numArraySlices());

            m_pageAllocators.clear();
            m_pageAllocators.reserve(pageCount);

            for (uint32_t i = 0; i < pageCount; ++i)
                m_pageAllocators.emplaceBack().reset(info.width, info.height);

            m_invAtlasWidth = 1.0f / (float)info.width;
            m_invAtlasHeight = 1.0f / (float)info.height;

            return true;
        }

        bool CanvasImageCache::findPlacement(uint32_t width, uint32_t height, bool needsWrapping, ImagePlacement& outPlacement)
        {
            uint32_t MARGIN = 2;

            if (needsWrapping)
                MARGIN += 2;

            for (uint16_t i = 0; i < m_pageAllocators.size(); ++i)
            {
                uint32_t x = 0, y = 0;
                if (m_pageAllocators[i].allocate(width + 2* MARGIN, height + 2* MARGIN, x, y))
                {
                    outPlacement.m_pageIndex = i;
                    outPlacement.m_placement.x = x + MARGIN;
                    outPlacement.m_placement.y = y + MARGIN;
                    outPlacement.m_size.x = width;
                    outPlacement.m_size.y = height;

                    outPlacement.m_uvOffset.x = outPlacement.m_placement.x * m_invAtlasWidth;
                    outPlacement.m_uvOffset.y = outPlacement.m_placement.y * m_invAtlasHeight;
                    outPlacement.m_uvScale.x = outPlacement.m_size.x * m_invAtlasWidth;
                    outPlacement.m_uvScale.y = outPlacement.m_size.y * m_invAtlasHeight;
                    return true;
                }
            }

            return false;
        }

        void CanvasImageCache::entryFromPlacement(const ImagePlacement& placement, CanvasImageCacheEntry& entry)
        {
            entry.m_layerIndex = placement.m_pageIndex;
            entry.m_placement = placement.m_placement;
            entry.m_uvOffset = placement.m_uvOffset;
            entry.m_uvScale = placement.m_uvScale;
            entry.m_needsWrapping = placement.m_supportsWrapping;
        }

        bool CanvasImageCache::reinitializeWithImages(CanvasImageCacheEntry* entries, uint32_t numEntries, base::Array<CanvasImageCacheEntry*>& outEntriesToCopyToImage)
        {
            // reset all entries
            outEntriesToCopyToImage.reset();

            // reset allocators on all pages
            for (auto& page : m_pageAllocators)
                page.reset(m_size, m_size);

            // place all images again, if that fails we know we don't have enough space
            return placeImages(entries, numEntries, outEntriesToCopyToImage);
        }

        bool CanvasImageCache::placeImages(CanvasImageCacheEntry* entries, uint32_t numEntries, base::Array<CanvasImageCacheEntry*>& outEntriesToCopyToImage)
        {
            PC_SCOPE_LVL2(PlaceImages);

            for (uint32_t i = 0; i < numEntries; ++i)
            {
                auto& entry = entries[i];

                // never allow big images to disrupt cache situation
                if (entry.m_image->width() > cvMaxCanvasImageWidth.get() || entry.m_image->height() > cvMaxCanvasImageHeight.get())
                {
                    entry.m_layerIndex = 0;
                    entry.m_placement.x = 0;
                    entry.m_placement.y = 0;
                    entry.m_uvScale.y = 0.0f;
                    entry.m_uvScale.x = 0.0f;
                    entry.m_uvOffset.y = 0.0f;
                    entry.m_uvOffset.x = 0.0f;
                    continue;
                }

                auto placement = m_imageEntries.find(entry.m_image->runtimeUniqueId());
                if (placement && entry.m_needsWrapping && !placement->m_supportsWrapping)
                    placement = nullptr;

                if (placement)
                {
                    // use existing placement
                    entryFromPlacement(*placement, entry);

                    // up to date ?
                    if (placement->m_lastVersion == entry.m_image->runtimeVersion())
                        continue;

                    // TODO: validate that size of the image did not change
                    placement->m_lastVersion = entry.m_image->runtimeVersion();

                    // put in the list of entries to upload
                    outEntriesToCopyToImage.pushBack(&entry);
                }
                else
                {
                    // find placement for the image
                    ImagePlacement newPlacement;
                    if (findPlacement(entry.m_image->width(), entry.m_image->height(), entry.m_needsWrapping, newPlacement))
                    {
                        // we have found a valid placement for the image
                        newPlacement.m_supportsWrapping = entry.m_needsWrapping;
                        newPlacement.m_lastVersion = entry.m_image->runtimeVersion();
                        m_imageEntries[entry.m_image->runtimeUniqueId()] = newPlacement;

                        // update entry with computed placement and schedule it for update
                        entryFromPlacement(newPlacement, entry);
                        outEntriesToCopyToImage.pushBack(&entry);
                    }
                    else
                    {
                        // not enough space in the pages
                        return false;
                    }
                }
            }

            // all images were placed
            return true;
        }

        base::image::ImagePtr CanvasImageCache::CreateWrapEnabledImage(const base::image::ImageView& src, uint32_t m)
        {
            auto w = src.width();
            auto h = src.height();
            auto fw = w + m * 2;
            auto fh = h + m * 2;
            
            auto ret = base::RefNew<base::image::Image>(src.format(), src.channels(), fw, fh);
            base::image::Fill(ret->view(), &base::Color::PURPLE);

            // center
            base::image::Copy(src, ret->view().subView(m, m, w, h));

            // top/bottom line
            base::image::Copy(src.subView(0, 0, w, m), ret->view().subView(m, fh-m, w, m));
            base::image::Copy(src.subView(0, h-m, w, m), ret->view().subView(m, 0, w, m));

            // left/right line
            base::image::Copy(src.subView(0, 0, m, h), ret->view().subView(fw-m, m, m, h));
            base::image::Copy(src.subView(w-m, 0, m, h), ret->view().subView(0, m, m, h));

            // corners
            base::image::Copy(src.subView(0, 0, m, m), ret->view().subView(fw-m, fh-m, m, m));
            base::image::Copy(src.subView(w-m, h-m, m, m), ret->view().subView(0, 0, m, m));
            base::image::Copy(src.subView(0, h - m, m, m), ret->view().subView(fw-m, 0, m, m));
            base::image::Copy(src.subView(w-m, 0, m, m), ret->view().subView(0, fh-m, m, m));
            
            return ret;
        }

        bool CanvasImageCache::cacheImages(command::CommandWriter& cmd, CanvasImageCacheEntry* entries, uint32_t numEntries)
        {
            PC_SCOPE_LVL2(CacheCanvasImages);

            base::InplaceArray<CanvasImageCacheEntry*, 256> entriesToUpload;

            // try to place within existing cache
            if (!placeImages(entries, numEntries, entriesToUpload))
            {
                // placing failed, reinitialize
                TRACE_WARNING("Canvas image cache cannot place any more images, {} entrie(s), {} entrie(s) to put, {}x{}, {} page(s)", m_imageEntries.size(), numEntries, m_image.width(), m_image.height(), m_pageAllocators.size());
                while (!reinitializeWithImages(entries, numEntries, entriesToUpload))
                {
                    // ok, we didn't have enough space in existing atlas, allocate more page
                    auto newPageCount = std::max<uint32_t>(m_pageAllocators.size() * 2, 1);
                    TRACE_WARNING("Canvas image cache is full, reinitializing with more pages {}->{}, {}x{}, ({} entries), ", m_pageAllocators.size(), newPageCount, m_image.width(), m_image.height(), m_imageEntries.size());

                    // drop current image
                    destroyImage();

                    // prepare new image
                    if (!createImage(newPageCount))
                    {
                        TRACE_ERROR("Canvas image failed to be recreated. No images will be displayed.");
                        return false;
                    }
                }
            }

            // copy content of images to backing texture array
            if (!entriesToUpload.empty())
            {
                cmd.opBeginBlock("UploadCanvasAtlas");
                // TODO: merge into bigger uploads if needed

                PC_SCOPE_LVL2(UploadImages);
                for (auto entry  : entriesToUpload)
                {
                    auto layerView = m_image.createSingleSliceView(entry->m_layerIndex);

                    if (entry->m_needsWrapping)
                    {
                        auto wrapEnabledImage = CreateWrapEnabledImage(entry->m_image->view(), 2);
                        if (wrapEnabledImage->channels() == 4)
                            base::image::PremultiplyAlpha(wrapEnabledImage->view());
                        cmd.opUpdateDynamicImage(layerView, wrapEnabledImage->view(), entry->m_placement.x - 2, entry->m_placement.y - 2);
                    }
                    else
                    {
                        if (entry->m_image->channels() == 4)
                        {
                            auto tempImage = base::RefNew<base::image::Image>(entry->m_image->view());
                            base::image::PremultiplyAlpha(tempImage->view());
                            cmd.opUpdateDynamicImage(layerView, tempImage->view(), entry->m_placement.x, entry->m_placement.y);
                        }
                        else
                        {
                            cmd.opUpdateDynamicImage(layerView, entry->m_image->view(), entry->m_placement.x, entry->m_placement.y);
                        }
                    }
                }

                cmd.opEndBlock();
                TRACE_INFO("Uploaded data for {} images", entriesToUpload.size());
            }

            // cache now contains all requested images
            return true;
        }

        //---

    } // canvas
} // rendering