/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "dynamicAtlas.h"

#include "core/image/include/image.h"
#include "core/image/include/imageView.h"
#include "core/image/include/imageUtils.h"
#include "core/containers/include/rectAllocator.h"

#include "gpu/device/include/image.h"
#include "gpu/device/include/deviceService.h"
#include "gpu/device/include/device.h"
#include "gpu/device/include/commandWriter.h"
#include "gpu/device/include/globalObjects.h"

BEGIN_BOOMER_NAMESPACE();

ConfigProperty<uint32_t> cvDynamicImageAtlasStartSize("IDynamicAtlas", "AtlasStartSize", 512);
ConfigProperty<uint32_t> cvDynamicImageAtlasMaxSize("IDynamicAtlas", "AtlasMaxSize", 4096);
ConfigProperty<uint32_t> cvDynamicImageAtlasPadding("IDynamicAtlas", "AtlasPadding", 4);
ConfigProperty<uint32_t> cvDynamicImageAtlasIncrement("IDynamicAtlas", "AtlasIncrement", 64);
ConfigProperty<uint32_t> cvDynamicImageBufferIncrement("IDynamicAtlas", "BufferIncrement", 256);

//--

RTTI_BEGIN_TYPE_CLASS(IDynamicAtlas);
RTTI_END_TYPE();

IDynamicAtlas::IDynamicAtlas(ImageFormat format /*= ImageFormat::RGBA8_UNORM*/)
    : m_format(format)
{
    if (!IsDefaultObjectCreation())
    {
        m_entries.reserve(4096);
        m_entriesUpdateList.reserve(4096);
        m_pages.reserve(16);

        m_entries.emplaceBack(); // reserver ID 0
    }
}

IDynamicAtlas::~IDynamicAtlas()
{
}

//--

AtlasImageID IDynamicAtlas::createEntry_NoLock(const Image* data, const Rect& sourceRect, bool wrapU, bool wrapV)
{
    DEBUG_CHECK_RETURN_V(data, 0);
    DEBUG_CHECK_RETURN_V(!sourceRect.empty(), 0);

    if (!m_freeEntryIds.empty())
    {
        auto id = m_freeEntryIds.back();
        m_freeEntryIds.popBack();

        auto& entry = m_entries[id];
        entry.data = AddRef(data);
        entry.sourceRect = sourceRect;
        entry.wrapU = wrapU;
        entry.wrapV = wrapV;
        entry.packedRect = Rect();
        entry.page = INDEX_NONE;

        m_entriesUpdateList.insert(id);

        return id;
    }
    else
    {
        auto id = m_entries.size();

        auto& entry = m_entries.emplaceBack();
        entry.data = AddRef(data);
        entry.sourceRect = sourceRect;
        entry.wrapU = wrapU;
        entry.wrapV = wrapV;

        m_entriesUpdateList.insert(id);

        return id;
    }
}

void IDynamicAtlas::removeEntry_NoLock(AtlasImageID id)
{
    DEBUG_CHECK_RETURN(id >= 0 && id < m_entries.size());
    DEBUG_CHECK_RETURN(m_entries[id].data);

    m_entries[id] = PackingEntry();
    m_freeEntryIds.pushBack(id);
    m_entriesUpdateList.remove(id); // in case it gets removed before the update call
}

//--

bool IDynamicAtlas::describe(AtlasImageID id, AtlasImageInfo& outInfo) const
{
    auto lock = CreateLock(m_lock);

    if (id >= 0 && id < m_entries.size())
    {
        const auto& entry = m_entries[id];
        outInfo.width = entry.sourceRect.width();
        outInfo.height = entry.sourceRect.height();
        outInfo.wrapU = entry.wrapU;
        outInfo.wrapV = entry.wrapV;
        return true;
    }

    return false;
}

const gpu::ImageSampledViewPtr& IDynamicAtlas::imageSRV() const
{
    if (m_textureArraySRV)
        return m_textureArraySRV;
    else
        return gpu::Globals().TextureArrayWhite;
}

const gpu::BufferStructuredViewPtr& IDynamicAtlas::imageEntriesSRV() const
{
    return m_entryBufferSRV;
}

//--

bool IDynamicAtlas::tryPack_NoLock(PackingEntry& entry)
{
    DEBUG_CHECK_RETURN_EX_V(entry.data, "Entry with no data to pack", true);

    const auto padding = std::max<uint32_t>(0, cvDynamicImageAtlasPadding.get());

    const auto packWidth = entry.sourceRect.width() + 2 * padding;
    const auto packHeight = entry.sourceRect.height() + 2 * padding;

    for (auto pageIndex : m_pages.indexRange())
    {
        uint32_t x = 0, y = 0;
        if (m_pages[pageIndex]->allocate(packWidth, packHeight, x, y))
        {
            entry.page = pageIndex;
            entry.packedRect = Rect(x + padding, y + padding, x + padding + entry.data->width(), y + padding + entry.data->height());
            entry.totalRect = Rect(x, y, x + packWidth, y + packHeight);
            return true;
        }
    }

    return false;
}

void IDynamicAtlas::repack_NoLock(gpu::CommandWriter& cmd)
{
    m_pages.clearPtr();

    struct ElementToPack
    {
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t area = 0;
        ImagePtr image;
        int index = 0;

        INLINE bool operator<(const ElementToPack& other) const
        {
            return area > other.area;
        }
    };

    Array<ElementToPack> elementsToPack;
    elementsToPack.reserve(m_entries.size());

    uint32_t maxWidth = std::max<uint32_t>(256, cvDynamicImageAtlasStartSize.get());
    uint32_t maxHeight = std::max<uint32_t>(256, cvDynamicImageAtlasStartSize.get());
    uint32_t padding = std::max<uint32_t>(0, cvDynamicImageAtlasPadding.get());
    uint32_t increment = std::max<uint32_t>(64, cvDynamicImageAtlasIncrement.get());

    for (auto i : m_entries.indexRange())
    {
        auto& icon = m_entries[i];

        icon.page = -1;
        icon.packedRect = Rect();
        icon.totalRect = Rect();

        if (icon.data)
        {
            auto& entry = elementsToPack.emplaceBack();
            entry.image = icon.data;
            entry.width = icon.data->width();
            entry.height = icon.data->height();
            entry.area = entry.width * entry.height;
            entry.index = i;

            auto packWidth = entry.width + 2 * padding;
            if (packWidth > maxWidth)
                maxWidth = Align(packWidth, increment);

            auto packHeight = entry.height + 2 * padding;
            if (packHeight > maxHeight)
                maxHeight = Align(packHeight, increment);
        }
    }

    TRACE_INFO("Maximum atlas size determined to be {}x{}", maxWidth, maxHeight);

    maxWidth = std::min<uint32_t>(maxWidth, cvDynamicImageAtlasMaxSize.get());
    maxHeight = std::min<uint32_t>(maxHeight, cvDynamicImageAtlasMaxSize.get());

    std::sort(elementsToPack.begin(), elementsToPack.end());

    for (const auto& entry : elementsToPack)
    {
        auto packWidth = entry.width + 2 * padding;
        auto packHeight = entry.height + 2 * padding;

        // to big...
        if (packWidth > maxWidth || packHeight > maxHeight)
            continue; // just won't work

        auto& icon = m_entries[entry.index];

        // try to pack in any existing page
        bool packed = false;
        for (auto pageIndex : m_pages.indexRange())
        {
            uint32_t x = 0, y = 0;
            if (m_pages[pageIndex]->allocate(packWidth, packHeight, x, y))
            {
                icon.page = pageIndex;
                icon.packedRect = Rect(x + padding, y + padding, x + padding + entry.width, y + padding + entry.height);
                icon.totalRect = Rect(x, y, x+packWidth, y+packHeight);
                packed = true;
                break;
            }
        }

        // does not fit, allocate a new page
        if (!packed)
        {
            auto* page = new RectAllocator();
            page->reset(maxWidth, maxHeight);
            m_pages.pushBack(page);

            uint32_t x = 0, y = 0;
            const auto ret = page->allocate(packWidth, packHeight, x, y);
            DEBUG_CHECK_EX(ret, "Packing in empty page failed");
            DEBUG_CHECK_EX(x == 0 && y == 0, "Unexpected packing");

            icon.page = m_pages.lastValidIndex();
            icon.packedRect = Rect(x + padding, y + padding, x + padding + entry.width, y + padding + entry.height);
            icon.totalRect = Rect(x, y, x + packWidth, y + packHeight);
        }
    }

    TRACE_INFO("Packed {} entries in {} pages", elementsToPack.size(), m_pages.size());

    // if current image atlas texture can't be used discard it
    // NOTE: it's still possible to reuse the texture because actual needed space may be smaller
    if (m_textureArray)
    {
        if (m_textureArray->width() < maxWidth || m_textureArray->height() < maxHeight || m_textureArray->slices() < m_pages.size())
        {
            TRACE_INFO("Existing atlas texture {}x{} ({} pages) can't be reused",
                m_textureArray->width(), m_textureArray->height(), m_textureArray->slices());

            m_textureArray.reset();
            m_textureArraySRV.reset();
        }
        else
        {
            // create page allocators for existing pages in the texture so we can reuse them in the future
            while (m_pages.size() < m_textureArray->slices())
            {
                auto* page = new RectAllocator();
                page->reset(maxWidth, maxHeight);
                m_pages.pushBack(page);
            }
        }
    }

    // create texture
    if (!m_textureArray)
    {
        gpu::ImageCreationInfo info;
        info.allowShaderReads = true;
        info.format = m_format;
        info.view = ImageViewType::View2DArray;
        info.label = "DynamicAtlasImage";
        info.width = maxWidth;
        info.height = maxHeight;
        info.numMips = 1;
        info.numSlices = m_pages.size();
        info.allowDynamicUpdate = true;
        info.allowCopies = true;

        m_textureArray = GetService<DeviceService>()->device()->createImage(info);
        m_textureArraySRV = m_textureArray->createSampledView();

        // TODO: clear ?
    }

    // push the updates to the pages
    // TODO: batch ?
    {
        cmd.opTransitionLayout(m_textureArray, gpu::ResourceLayout::ShaderResource, gpu::ResourceLayout::CopyDest);

        for (const auto& entry : elementsToPack)
            uploadImage_NoLock(cmd, m_entries[entry.index]);

        cmd.opTransitionLayout(m_textureArray, gpu::ResourceLayout::CopyDest, gpu::ResourceLayout::ShaderResource);
    }
}

void IDynamicAtlas::uploadImage_NoLock(gpu::CommandWriter& cmd, const PackingEntry& entry) const
{
    DEBUG_CHECK_RETURN(!entry.packedRect.empty());
    DEBUG_CHECK_RETURN(entry.data);

    if (entry.wrapU || entry.wrapV)
    {
        const auto cx = entry.packedRect.min.x - entry.totalRect.min.x;
        const auto cy = entry.packedRect.min.y - entry.totalRect.min.y;
        const auto cw = entry.sourceRect.width();
        const auto ch = entry.sourceRect.height();

        const int bx = std::min<int>(cx, entry.sourceRect.width());
        const int by = std::min<int>(cy, entry.sourceRect.height());

        auto source = entry.data->view().subView(entry.sourceRect.min.x, entry.sourceRect.min.y, entry.sourceRect.width(), entry.sourceRect.height());

        // create temporary packing image
        // TODO: fix!
        Color empty(0, 0, 0, 0);
        auto temp = RefNew<Image>(entry.data->format(), entry.data->channels(), entry.totalRect.width(), entry.totalRect.height(), &empty);

        // center
        {
            auto center = temp->view().subView(cx, cy, cw, ch);
            Copy(source, center);
        }

        // left/right
        if (entry.wrapU)
        {
            {
                auto targetBorder = temp->view().subView(cx - bx, cy, bx, ch);
                auto sourceBorder = source.subView(cw - bx, 0, bx, ch);
                Copy(sourceBorder, targetBorder);
            }

            {
                auto targetBorder = temp->view().subView(cx + cw, cy, bx, ch);
                auto sourceBorder = source.subView(0, 0, bx, ch);
                Copy(sourceBorder, targetBorder);
            }
        }

        // top/bottom
        if (entry.wrapU)
        {
            {
                auto targetBorder = temp->view().subView(cx, cy - by, cw, by);
                auto sourceBorder = source.subView(0, ch - by, cw, by);
                Copy(sourceBorder, targetBorder);
            }

            {
                auto targetBorder = temp->view().subView(cx, cy + ch, cw, by);
                auto sourceBorder = source.subView(0, 0, cw, cy);
                Copy(sourceBorder, targetBorder);
            }
        }

        // corners
        if (entry.wrapU && entry.wrapV)
        {
            {
                auto targetCorner = temp->view().subView(cx - bx, cy - by, bx, by);
                auto sourceCorner = source.subView(cw - bx, ch - by, bx, by);
                Copy(sourceCorner, targetCorner);
            }

            {
                auto targetCorner = temp->view().subView(cx + cw, cy - by, bx, by);
                auto sourceCorner = source.subView(0, ch - by, bx, by);
                Copy(sourceCorner, targetCorner);
            }

            {
                auto targetCorner = temp->view().subView(cx - bx, cy + ch, bx, by);
                auto sourceCorner = source.subView(cw - bx, 0, bx, by);
                Copy(sourceCorner, targetCorner);
            }

            {
                auto targetCorner = temp->view().subView(cx + cw, cy + ch, bx, by);
                auto sourceCorner = source.subView(0, 0, bx, by);
                Copy(sourceCorner, targetCorner);
            }
        }

        cmd.opUpdateDynamicImage(m_textureArray, temp->view(), 0, entry.page, entry.totalRect.min.x, entry.totalRect.min.y);
    }
    else
    {
        const auto sourceView = entry.data->view().subView(entry.sourceRect.min.x, entry.sourceRect.min.y, entry.sourceRect.width(), entry.sourceRect.height());
        cmd.opUpdateDynamicImage(m_textureArray, sourceView, 0, entry.page, entry.packedRect.min.x, entry.packedRect.min.y);
    }
}

bool IDynamicAtlas::tryPackUpdates_NoLock(gpu::CommandWriter& cmd, bool& hasChanges)
{
    bool allPacked = true;
    bool transitioned = false;

    if (!m_entriesUpdateList.empty())
    {
        auto newEntriesIndices = m_entriesUpdateList.keys();
        m_entriesUpdateList.reset();

        // pack entries
//        Array<int> openedPages;
        for (auto index : newEntriesIndices)
        {
            auto& entry = m_entries[index];
            DEBUG_CHECK_EX(entry.data, "No data");

            if (!tryPack_NoLock(entry))
            {
                TRACE_INFO("Failed to pack {}x{} into debug atlas", entry.sourceRect.width(), entry.sourceRect.height());

                // not worth trying more
                allPacked = false;
                break;
            }

            // make sure image is transitioned
            /*if (!openedPages.contains(entry.page))
            {
                openedPages.pushBack(entry.page);
                cmd.opTransitionImageArrayRangeLayout(m_textureArray, entry.page, 1, gpu::ResourceLayout::ShaderResource, gpu::ResourceLayout::CopyDest);
            }*/
            if (!transitioned)
            {
                cmd.opTransitionLayout(m_textureArray, gpu::ResourceLayout::ShaderResource, gpu::ResourceLayout::CopyDest);
                transitioned = true;
            }

            // copy image content
            uploadImage_NoLock(cmd, entry);

            // make sure the data buffer gets updated as well
            hasChanges = true;
        }

        // close pages back
        if (transitioned)
        {
            cmd.opTransitionLayout(m_textureArray, gpu::ResourceLayout::CopyDest, gpu::ResourceLayout::ShaderResource);
            transitioned = false;
        }
        /*r (const auto& index : openedPages)
            cmd.opTransitionImageArrayRangeLayout(m_textureArray, index, 1, gpu::ResourceLayout::CopyDest, gpu::ResourceLayout::ShaderResource);*/
    }

    return allPacked;
}

//--

void IDynamicAtlas::update(gpu::CommandWriter& cmd)
{
    auto lock = CreateLock(m_lock);

    // try to pack updates incrementally
    bool hasChanges = false;
    if (!tryPackUpdates_NoLock(cmd, hasChanges))
    {
        // we failed to pack updates incrementally, rebuild the whole atlas
        hasChanges = true;
        repack_NoLock(cmd);
    }

    // make sure we have enough space in the buffer
    {
        const auto bufferIncrement = std::max<uint32_t>(1, cvDynamicImageBufferIncrement.get());
        const auto entrySizeNeeded = Align(m_entries.size(), bufferIncrement) * sizeof(GPUDynamicAtlasImageInfo);

        if (m_entryBuffer && m_entryBuffer->size() < entrySizeNeeded)
        {
            TRACE_INFO("Existing atlas entry buffer can't be reused");
            m_entryBufferSRV.reset();
            m_entryBuffer.reset();
        }

        if (!m_entryBuffer)
        {
            gpu::BufferCreationInfo info;
            info.label = "DynamicImageAtlasEntries";
            info.allowCopies = true;
            info.allowShaderReads = true;
            info.allowDynamicUpdate = true;
            info.stride = sizeof(GPUDynamicAtlasImageInfo);
            info.size = entrySizeNeeded;
            
            m_entryBuffer = GetService<DeviceService>()->device()->createBuffer(info);
            m_entryBufferSRV = m_entryBuffer->createStructuredView();
        }
    }

    // if anything changed update the entry buffer
    // TODO: update only changed entries
    if (hasChanges)
    {
        cmd.opTransitionLayout(m_entryBuffer, gpu::ResourceLayout::ShaderResource, gpu::ResourceLayout::CopyDest);

        const auto invWidth = 1.0f / (float)m_textureArray->width();
        const auto invHeight = 1.0f / (float)m_textureArray->height();

        auto* writePtr = cmd.opUpdateDynamicBufferPtrN<GPUDynamicAtlasImageInfo>(m_entryBuffer, 0, m_entries.size());
        for (const auto& entry : m_entries)
        {
            if (entry.data)
            {
                writePtr->uvMin.x = (entry.packedRect.min.x + 0.5f) * invWidth;
                writePtr->uvMin.y = (entry.packedRect.min.y + 0.5f) * invHeight;
                writePtr->uvScale.x = entry.packedRect.width() * invWidth;
                writePtr->uvScale.y = entry.packedRect.height() * invHeight;
                writePtr->page = entry.page;
                writePtr->_padding = 0;
            }

            writePtr++;
        }

        cmd.opTransitionLayout(m_entryBuffer, gpu::ResourceLayout::CopyDest, gpu::ResourceLayout::ShaderResource);
    }
}

//--

END_BOOMER_NAMESPACE();
