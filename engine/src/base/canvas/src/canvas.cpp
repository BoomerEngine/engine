/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: renderer #]
***/

#include "build.h"

#include "canvas.h"
#include "canvasGeometry.h"
#include "canvasGeometryBuilder.h"
#include "canvasGlyphCache.h"

#include "base/system/include/mutex.h"
#include "base/system/include/scopeLock.h"
#include "base/memory/include/pageAllocator.h"

namespace base
{
    namespace canvas
    {

        //--

        INLINE bool TestClip(const Vector4& clipRect, const Vector2& boundsMin, const Vector2& boundsMax)
        {
            if (boundsMin.x > clipRect.z || boundsMin.y > clipRect.w)
                return false;
            if (boundsMax.x < clipRect.x || boundsMax.y < clipRect.y)
                return false;
            return true;
        }

        INLINE bool TestClip(const Vector4& clipRect, float minX, float minY, float maxX, float maxY)
        {
            if (minX > clipRect.z || minY > clipRect.w)
                return false;
            if (maxX < clipRect.x || maxY < clipRect.y)
                return false;
            return true;
        }

        //--

        static base::mem::PageAllocator CanvasPageAllocator(POOL_CANVAS, 512 << 10, 32, 128);

        //--

        Canvas::Canvas(uint32_t width, uint32_t height, GlyphCache* glyphCache /*= nullptr */, float pixelOffsetX /*= 0.0f*/, float pixelOffsetY /*= 0.0f*/, float pixelScale /*= 1.0f*/)
            : m_vertices(CanvasPageAllocator)
            , m_indices(CanvasPageAllocator)
            , m_params(CanvasPageAllocator)
            , m_images(CanvasPageAllocator)
            , m_batches(CanvasPageAllocator)
            , m_width(width)
            , m_height(height)
            , m_pixelOffset(pixelOffsetX, pixelOffsetY)
            , m_pixelScale(pixelScale)
            , m_alpha(1.0f)
            , m_glyphCachePageMask(0)
            , m_clearColor(0,0,0,0) // 0 alpha - not used
            , m_customPayloadData(POOL_CANVAS)
        {
            memset(m_imageRefBuckets, 0, sizeof(m_imageRefBuckets));
            memset(m_glyphPageToParamsIdMapping, 0xFF, sizeof(m_glyphPageToParamsIdMapping));

            if (pixelScale != 1.0f || pixelOffsetX != 0.0f || pixelOffsetY != 0.0f)
                m_hasPixelTransform = true;
            else
                m_hasPixelTransform = false;

            m_transform.identity();
            updatePixelTransform();

            m_scissorRect = Vector4(0.0f, 0.0f, width, height);
            updateScissorRect();
        }

        Canvas::~Canvas()
        {
            releaseImageReferences();

            /*TRACE_INFO("{} styles, {} vertices, {} indices, {} commands in canvas, {} geom culled, {} batch culled", 
                m_params.size(), m_vertices.size(), m_indices.size(), m_batches.size(), m_numGeometriesCulled, m_numBatchedCulled);*/
        }

        void Canvas::updatePixelTransform()
        {
            m_transformClass = m_transform.classify();

            m_pixelTransform = m_transform;

            if (m_hasPixelTransform)
            {
                m_pixelTransform.t[4] += m_pixelOffset.x;
                m_pixelTransform.t[5] += m_pixelOffset.y;
                m_pixelTransform.t[0] *= m_pixelScale;
                m_pixelTransform.t[1] *= m_pixelScale;
                m_pixelTransform.t[2] *= m_pixelScale;
                m_pixelTransform.t[3] *= m_pixelScale;
            }

            m_pixelTransformClass = m_pixelTransform.classify();
        }

        void Canvas::updateScissorRect()
        {
            const auto prevRect = m_pixelScissorShaderValues;

            if (m_hasPixelTransform)
            {
                m_pixelScissorRect.x = (m_scissorRect.x + m_pixelOffset.x) * m_pixelScale;
                m_pixelScissorRect.y = (m_scissorRect.y + m_pixelOffset.y) * m_pixelScale;
                m_pixelScissorRect.z = (m_scissorRect.z + m_pixelOffset.x) * m_pixelScale;
                m_pixelScissorRect.w = (m_scissorRect.w + m_pixelOffset.y) * m_pixelScale;
            }
            else
            {
                m_pixelScissorRect = m_scissorRect;
            }

            if (m_pixelScissorRect.z >= (m_pixelScissorRect.x + 1.0f) && m_pixelScissorRect.w > (m_pixelScissorRect.y + 1.0f))
            {
                m_emptyScissorRect = false;

                float extentsX = (m_pixelScissorRect.z - m_pixelScissorRect.x) * 0.5f;
                float extentsY = (m_pixelScissorRect.w - m_pixelScissorRect.y) * 0.5f;

                m_pixelScissorShaderValues.x = -(m_pixelScissorRect.x + extentsX);
                m_pixelScissorShaderValues.y = -(m_pixelScissorRect.y + extentsY);
                m_pixelScissorShaderValues.z = 1.0f / extentsX;
                m_pixelScissorShaderValues.w = 1.0f / extentsY;
            }
            else
            {
                m_emptyScissorRect = true;
                m_pixelScissorShaderValues.x = 0.0f;
                m_pixelScissorShaderValues.y = 0.0f;
                m_pixelScissorShaderValues.z = 0.0f;
                m_pixelScissorShaderValues.w = 0.0f;
            }
        }

        //--

        void Canvas::pixelPlacement(float pixelOffsetX, float pixelOffsetY, float pixelScale /*= 1.0f*/)
        {
            if (m_pixelOffset.x != pixelOffsetX || m_pixelOffset.y != pixelOffsetY || m_pixelScale != pixelScale)
            {
                m_pixelOffset.x = pixelOffsetX;
                m_pixelOffset.y = pixelOffsetY;
                m_pixelScale = pixelScale;

                if (pixelScale != 1.0f || pixelOffsetX != 0.0f || pixelOffsetY)
                    m_hasPixelTransform = true;
                else
                    m_hasPixelTransform = false;

                updatePixelTransform();
                updateScissorRect();
            }
        }

        void Canvas::placement(const XForm2D& absoluteXForm)
        {
            m_transform = absoluteXForm;
            updatePixelTransform();
        }

        void Canvas::placement(float tx, float ty, float scale /*= 1.0f*/)
        {
            m_transform = base::XForm2D(scale, 0.0f, 0.0f, scale, tx, ty);
            updatePixelTransform();
        }

        void Canvas::placement(float tx, float ty, float sx, float sy)
        {
            m_transform = base::XForm2D(sx, 0.0f, 0.0f, sy, tx, ty);
            updatePixelTransform();
        }

        //--

        void Canvas::alphaMultiplier(float alpha)
        {
            m_alpha = alpha;
        }

        //--

        void Canvas::clearScissorRectStack()
        {
            m_scissorRectStack.clear();
            updateScissorRect();
        }

        void Canvas::resetScissorRect()
        {
            m_scissorRect = Vector4(0.0f, 0.0f, m_width, m_height);
            updateScissorRect();
        }

        void Canvas::pushScissorRect()
        {
            auto& entry = m_scissorRectStack.emplaceBack();
            entry.rect = m_scissorRect;
        }

        void Canvas::popScissorRect()
        {
            if (!m_scissorRectStack.empty())
            {
                auto& entry = m_scissorRectStack.back();
                m_scissorRect = entry.rect;

                m_scissorRectStack.popBack();

                updateScissorRect();
            }
        }

        bool Canvas::scissorRect(const Vector2& position, const Vector2& extents)
        {
            m_scissorRect = Vector4(position.x, position.y, position.x + extents.x, position.y + extents.y);
            updateScissorRect();
            return !m_emptyScissorRect;
        }

        bool Canvas::scissorRect(float x, float y, float w, float h)
        {
            m_scissorRect = Vector4(x, y, x + w, y + h);
            updateScissorRect();
            return !m_emptyScissorRect;
        }

        bool Canvas::scissorBounds(const Vector2& bmin, const Vector2& bmax)
        {
            m_scissorRect = Vector4(bmin.x, bmin.y, bmax.x, bmax.y);
            updateScissorRect();
            return !m_emptyScissorRect;
        }

        bool Canvas::scissorBounds(float x0, float y0, float x1, float y1)
        {
            m_scissorRect = Vector4(x0, y0, x1, y1);
            updateScissorRect();
            return !m_emptyScissorRect;
        }

        bool Canvas::intersectScissorRect(const Vector2& position, const Vector2& extents)
        {
            m_scissorRect.x = std::max<float>(m_scissorRect.x, position.x);
            m_scissorRect.y = std::max<float>(m_scissorRect.y, position.y);
            m_scissorRect.z = std::min<float>(m_scissorRect.z, position.x + extents.x);
            m_scissorRect.w = std::min<float>(m_scissorRect.w, position.y + extents.y);
            updateScissorRect();
            return !m_emptyScissorRect;
        }

        bool Canvas::intersectScissorRect(float x, float y, float w, float h)
        {
            m_scissorRect.x = std::max<float>(m_scissorRect.x, x);
            m_scissorRect.y = std::max<float>(m_scissorRect.y, y);
            m_scissorRect.z = std::min<float>(m_scissorRect.z, x+w);
            m_scissorRect.w = std::min<float>(m_scissorRect.w, y+h);
            updateScissorRect();
            return !m_emptyScissorRect;
        }

        bool Canvas::intersectScissorBounds(const Vector2& bmin, const Vector2& bmax)
        {
            m_scissorRect.x = std::max<float>(m_scissorRect.x, bmin.x);
            m_scissorRect.y = std::max<float>(m_scissorRect.y, bmin.y);
            m_scissorRect.z = std::min<float>(m_scissorRect.z, bmax.x);
            m_scissorRect.w = std::min<float>(m_scissorRect.w, bmax.y);
            updateScissorRect();
            return !m_emptyScissorRect;
        }

        bool Canvas::intersectScissorBounds(float x0, float y0, float x1, float y1)
        {
            m_scissorRect.x = std::max<float>(m_scissorRect.x, x0);
            m_scissorRect.y = std::max<float>(m_scissorRect.y, y0);
            m_scissorRect.z = std::min<float>(m_scissorRect.z, x1);
            m_scissorRect.w = std::min<float>(m_scissorRect.w, y1);
            updateScissorRect();
            return !m_emptyScissorRect;
        }

        bool Canvas::testScissorRect(const Vector2& position, const Vector2& extents) const
        {
            return TestClip(m_scissorRect, position.x, position.y, position.x + extents.x, position.y + extents.y);
        }

        bool Canvas::testScissorRect(float x, float y, float w, float h) const
        {
            return TestClip(m_scissorRect, x, y, x + w, y + h);
        }

        bool Canvas::testScissorBounds(const Vector2& bmin, const Vector2& bmax) const
        {
            return TestClip(m_scissorRect, bmin.x, bmin.y, bmax.x, bmax.y);
        }

        bool Canvas::testScissorBounds(float x0, float y0, float x1, float y1) const
        {
            return TestClip(m_scissorRect, x0, y0, x1, y1);
        }

        //--

        void Canvas::releaseImageReferences()
        {
            for (uint32_t i = 0; i < MAX_IMAGE_BUCKETS; ++i)
            {
                ImageRef* refList = m_imageRefBuckets[i];
                while (refList)
                {
                    //refList->imagePtr.reset();
                    refList = refList->next;
                }
            }
        }

        void Canvas::clear()
        {
            releaseImageReferences();

            m_vertices.clear();
            m_indices.clear();
            m_params.clear();
            m_images.clear();
            m_batches.clear();
            memset(m_imageRefBuckets, 0, sizeof(m_imageRefBuckets));

            m_alpha = 1.0f;

            m_transform.identity();
            updatePixelTransform();

            m_scissorRect = Vector4(0.0f, 0.0f, m_width, m_height);
            updateScissorRect();
        }

        //---

        void Canvas::transformBounds(const Vector2& localMin, const Vector2& localMax, Vector2& globalMin, Vector2& globalMax) const
        {
            if (m_transformClass == XForm2DClass::Identity)
            {
                globalMin = localMin;
                globalMax = localMax;
            }
            else if (m_transformClass == XForm2DClass::HasTransform)
            {
                globalMin = localMin + m_transform.translation();
                globalMax = localMax + m_transform.translation();
            }
            else
            {
                globalMax = globalMin = m_transform.transformPoint(localMin);

                {
                    auto v = m_transform.transformPoint(localMax);
                    globalMax = Max(globalMax, v);
                    globalMin = Min(globalMin, v);
                }

                {
                    auto v = m_transform.transformPoint(Vector2(localMax.x, localMin.y));
                    globalMax = Max(globalMax, v);
                    globalMin = Min(globalMin, v);
                }

                {
                    auto v = m_transform.transformPoint(Vector2(localMin.x, localMax.y));
                    globalMax = Max(globalMax, v);
                    globalMin = Min(globalMin, v);
                }
            }
        }

        void Canvas::place(const Geometry& geometry)
        {
            // transform bounds of the geometry to render space
            //if (geometry.numGroups() >= 2) // amortize cost of culling a little bit better
            {
                Vector2 globalMin, globalMax;
                transformBounds(geometry.boundsMin(), geometry.boundsMax(), globalMin, globalMax);

                // cull the whole geometry if it's outside current active scissor area
                if (!testScissorBounds(globalMin.x, globalMin.y, globalMax.x, globalMax.y))
                {
                    m_numGeometriesCulled += 1;
                    return;
                }
            }

            // merge reference to glyph pages we used across the whole canvas
            if (const auto glyphPagesMask = geometry.glyphPagesMask())
            {
                auto newGlyphPagesMask = glyphPagesMask & ~m_glyphCachePageMask;
                if (newGlyphPagesMask)
                {
                    mapNewGlyphPagesToParameters(newGlyphPagesMask);
                    m_glyphCachePageMask |= newGlyphPagesMask;
                }
            }

            // prepare style mapping array
            m_styleMapping.reset();
            m_styleMapping.prepareWith(geometry.numStyles(), -1);

            // push all cached groups to rendering
            auto groupPtr  = geometry.groups();
            auto endGroupPtr  = groupPtr + geometry.numGroups();
            while (groupPtr < endGroupPtr)
            {
                auto& group = *groupPtr++;

                /*// transform bounds of the geometry to render space
                Vector2 globalMin, globalMax;
                transformBounds(group.vertexBoundsMin, group.vertexBoundsMax, globalMin, globalMax);

                // cull the whole geometry if it's outside current active scissor area
                if (!testScissorBounds(globalMin.x, globalMin.y, globalMax.x, globalMax.y))
                {
                    m_numBatchedCulled += 1;
                    continue;
                }*/

                // map style
                uint32_t canvasStyleIndex = m_styleMapping[group.styleIndex];
                if (group.type != RenderGroup::Type::Glyphs && canvasStyleIndex == -1)
                {
                    const auto& renderStyle = geometry.styles()[group.styleIndex];
                    const auto paramsId = packParams(renderStyle, m_alpha);
                    m_styleMapping[group.styleIndex] = paramsId;
                    canvasStyleIndex = paramsId;
                }

                // emit geometry
                switch (group.type)
                {
                    case RenderGroup::Type::Fill:
                        renderFill(group, canvasStyleIndex);
                        break;

                    case RenderGroup::Type::Stoke:
                        renderStroke(group, canvasStyleIndex);
                        break;

                    case RenderGroup::Type::Glyphs:
                        renderGlyphs(group);
                }
            }
        }

        void Canvas::place(const GeometryBuilder& geometry)
        {
            // TODO: optimize!
            auto temp = new Geometry;
            geometry.extractNoReset(*temp);
            place(*temp);
            temp->releaseRef();
        }

        static void FindIndexRange(const uint16_t* indices, uint32_t numIndices, uint16_t& outMinVertex, uint16_t& outMaxVertex)
        {
            uint16_t minVertex = 65535;
            uint16_t maxVertex = 0;

            // assign drawing style to vertices
            const auto* endPtr = indices + numIndices;
            while (indices < endPtr)
            {
                minVertex = std::min<uint16_t>(*indices, minVertex);
                maxVertex = std::max<uint16_t>(*indices, maxVertex);
                indices += 1;
            }

            outMinVertex = minVertex;
            outMaxVertex = maxVertex;
        }

        void* Canvas::uploadCustomPayloadData(const void* data, uint32_t size)
        {
            void* ret = m_customPayloadData.alloc(size, 16);
            if (data)
                memcpy(ret, data, size);
            return ret;
        }
            
        void Canvas::place(const RenderStyle& style, const RawGeometry& geometry, uint16_t customDrawer /*= 0*/, const void* customPayload /*= nullptr*/, CompositeOperation op /*= CompositeOperation::SourceOver*/, float alpha /*= 1.0f*/)
        {
            if (geometry.numIndices && geometry.indices && geometry.vertices)
            {
                // pack rendering params
                auto paramsId = packParams(style, m_alpha);

                // find range of used indices
                uint16_t minVertex, maxVertex;
                FindIndexRange(geometry.indices, geometry.numIndices, minVertex, maxVertex);
                ASSERT(minVertex <= maxVertex);

                // copy vertices and indices
                auto baseVertex = packRawVertices(geometry.vertices + minVertex, maxVertex - minVertex + 1, paramsId);
                auto baseIndex = packIndices(geometry.indices, geometry.numIndices, baseVertex - minVertex);

                // prepare batches
                auto batch = m_batches.allocSingle();
                batch->fristIndex = baseIndex;
                batch->numIndices = geometry.numIndices;
                batch->op = op;
                batch->type = BatchType::ConvexFill;

                // custom drawer ?
                if (customDrawer)
                {
                    batch->customDrawer = customDrawer;
                    batch->customPayload = customPayload;
                    batch->type = BatchType::Custom;
                }
                else
                {
                    batch->customDrawer = 0;
                    batch->customPayload = nullptr;
                }
            }
        }

        const Canvas::ImageRef* Canvas::packImageRef(const image::Image* image, bool needsWrapping)
        {
            if (image)
            {
                auto bucketHash = (uint32_t)((uint64_t)image >> 5);
                auto bucketIndex = bucketHash % MAX_IMAGE_BUCKETS;

                auto bucket  = m_imageRefBuckets[bucketIndex];
                while (bucket)
                {
                    if (bucket->imagePtr == image)
                    {
                        if (needsWrapping)
                            bucket->imageNeedsWrapping = 1;
                        return bucket;
                    }
                    bucket = bucket->next;
                }

                bucket = m_images.allocSingle();
                memset(bucket, 0, sizeof(ImageRef));
                bucket->imagePtr = image;
                bucket->imageType = 0;
                bucket->imageNeedsWrapping = needsWrapping ? 1 : 0;
                bucket->next = m_imageRefBuckets[bucketIndex];
                m_imageRefBuckets[bucketIndex] = bucket;

                return bucket;
            }
            else
            {
                return nullptr;
            }
        }

        static Vector4 MakePremultipliedColor(const Vector4& color, float globalAlpha)
        {
            auto alpha = color.w * globalAlpha;
            return Vector4(color.x * alpha, color.y * alpha, color.z * alpha, alpha);
        }

        uint32_t Canvas::packDirectTextureParams(const base::image::Image* glyphCacheImage)
        {
            DEBUG_CHECK_EX(glyphCacheImage, "Expected glyph cache image");

            auto paramId = m_params.size();
            auto params = m_params.allocSingle();

            params->innerCol = Vector4::ONE();
            params->outerCol = Vector4::ONE();
            params->extent = Vector2::ZERO();
            params->base = Vector2::ZERO();
            params->wrapType = 4; // use direct UV
            params->radius = 0.0f;;
            params->feather = 0.0f;;
            params->featherHalf = 0.0f;;
            params->featherInv = 0.0f;;
            params->uvMin = Vector2::ZERO();
            params->uvMax = Vector2::ONE();
            params->imageRef = packImageRef(glyphCacheImage, false);

            return paramId;
        }

        uint32_t Canvas::packParams(const RenderStyle& style, float alpha)
        {
            uint32_t paramId = 0;

            RenderStyleWithAlphaKey styleWithAlphaKey(style, alpha);
            if (!m_paramsMap.find(styleWithAlphaKey, paramId))
            {
                paramId = m_params.size();
                auto params = m_params.allocSingle();

                params->innerCol = MakePremultipliedColor(style.innerColor.toVectorLinear(), alpha);
                params->outerCol = MakePremultipliedColor(style.outerColor.toVectorLinear(), alpha);

                params->extent = Vector2(style.extent.x, style.extent.y);
                params->base = style.base;

                //, float width, float fringe, float strokeThr, float alpha
                //params->strokeMult = (width*0.5f + fringe*0.5f) / fringe;
                //params->strokeThr = strokeThr;
                params->wrapType = 0;
                if (style.wrapU) params->wrapType |= 1;
                if (style.wrapV) params->wrapType |= 2;
                if (style.customUV) params->wrapType |= 4;

                if (!style.image)
                {
                    params->radius = style.radius;
                    params->feather = style.feather;
                    params->featherHalf = style.feather * 0.5f;
                    params->featherInv = 1.0f / std::max<float>(0.0001f, style.feather);
                    params->uvMin = Vector2::ZERO();
                    params->uvMax = Vector2::ONE();
                }
                else
                {
                    params->uvMin = style.uvMin;
                    params->uvMax = style.uvMax;
                }

                if (style.image)
                    params->imageRef = packImageRef(style.image, style.wrapU | style.wrapV);
                else
                    params->imageRef = nullptr;

                m_paramsMap[RenderStyleWithAlpha(style, alpha)] = paramId;
            }

            return paramId;
        }

        void Canvas::calcScissorUV(Vertex* ptr, uint32_t count)
        {
            auto endPtr = ptr + count;
            while (ptr < endPtr)
            {
                ptr->clipUV.x = (ptr->pos.x + m_pixelScissorShaderValues.x) * m_pixelScissorShaderValues.z;
                ptr->clipUV.y = (ptr->pos.y + m_pixelScissorShaderValues.y) * m_pixelScissorShaderValues.w;
                ++ptr;
            }
        }

        uint32_t Canvas::packVertices(const RenderGroup& group, uint32_t paramsId)
        {
            uint32_t firstVertex = m_vertices.size();

            {
                uint32_t leftToCopy = group.numVertices;
                auto readPtr  = group.vertices;

                while (leftToCopy > 0)
                {
                    uint32_t numAllocated = 0;
                    Vertex* writePtr = m_vertices.allocateBatch(leftToCopy, numAllocated);
                    Vertex* orgWritePtr = writePtr;

                    if (m_pixelTransformClass == XForm2DClass::Identity)
                    {
                        for (uint32_t i = 0; i < numAllocated; ++i, ++readPtr, ++writePtr)
                        {
                            writePtr->pos = readPtr->pos;
                            writePtr->uv = readPtr->uv;
                            writePtr->paintUV = readPtr->paintUV;
                            writePtr->color = base::Color::WHITE;
                            writePtr->paramsId = paramsId;
                        }
                    }
                    else if (m_pixelTransformClass == XForm2DClass::HasTransform)
                    {
                        for (uint32_t i = 0; i < numAllocated; ++i, ++readPtr, ++writePtr)
                        {
                            writePtr->pos.x = readPtr->pos.x + m_pixelTransform.t[4];
                            writePtr->pos.y = readPtr->pos.x + m_pixelTransform.t[5];
                            writePtr->uv = readPtr->uv;
                            writePtr->paintUV = readPtr->paintUV;
                            writePtr->color = base::Color::WHITE;
                            writePtr->paramsId = paramsId;
                        }
                    }
                    else
                    {
                        for (uint32_t i = 0; i < numAllocated; ++i, ++readPtr, ++writePtr)
                        {
                            writePtr->pos = m_pixelTransform.transformPoint(readPtr->pos);
                            writePtr->uv = readPtr->uv;
                            writePtr->paintUV = readPtr->paintUV;
                            writePtr->color = base::Color::WHITE;
                            writePtr->paramsId = paramsId;
                        }
                    }

                    calcScissorUV(orgWritePtr, numAllocated);

                    leftToCopy -= numAllocated;
                }
            }

            return firstVertex;
        }

        uint32_t Canvas::packVertices(const Vertex* vertexData, uint32_t numVertices, uint32_t paramsId)
        {
            uint32_t firstVertex = m_vertices.size();

            {
                uint32_t leftToCopy = numVertices;
                auto readPtr  = vertexData;

                while (leftToCopy > 0)
                {
                    uint32_t numAllocated = 0;
                    Vertex* writePtr = m_vertices.allocateBatch(leftToCopy, numAllocated);
                    Vertex* orgWritePtr = writePtr;

                    if (m_pixelTransformClass == XForm2DClass::Identity)
                    {
                        for (uint32_t i = 0; i < numAllocated; ++i, ++readPtr, ++writePtr)
                        {
                            writePtr->pos = readPtr->pos;
                            writePtr->paintUV = readPtr->paintUV;
                            writePtr->uv = readPtr->uv;
                            writePtr->color = readPtr->color;
                            writePtr->paramsId = paramsId;
                        }
                    }
                    else if (m_pixelTransformClass == XForm2DClass::HasTransform)
                    {
                        for (uint32_t i = 0; i < numAllocated; ++i, ++readPtr, ++writePtr)
                        {
                            writePtr->pos.x = readPtr->pos.x + m_pixelTransform.t[4];
                            writePtr->pos.y = readPtr->pos.y + m_pixelTransform.t[5];
                            writePtr->paintUV = readPtr->paintUV;
                            writePtr->uv = readPtr->uv;
                            writePtr->color = readPtr->color;
                            writePtr->paramsId = paramsId;
                        }
                    }
                    else
                    {
                        for (uint32_t i = 0; i < numAllocated; ++i, ++readPtr, ++writePtr)
                        {
                            writePtr->pos = m_pixelTransform.transformPoint(readPtr->pos);
                            writePtr->paintUV = readPtr->paintUV;
                            writePtr->uv = readPtr->uv;
                            writePtr->color = readPtr->color;
                            writePtr->paramsId = paramsId;
                        }
                    }

                    calcScissorUV(orgWritePtr, numAllocated);

                    leftToCopy -= numAllocated;
                }
            }

            return firstVertex;
        }

        uint32_t Canvas::packRawVertices(const RawVertex* vertexData, uint32_t numVertices, uint32_t paramId)
        {
            uint32_t firstVertex = m_vertices.size();

            {
                uint32_t leftToCopy = numVertices;
                auto readPtr = vertexData;

                while (leftToCopy > 0)
                {
                    uint32_t numAllocated = 0;
                    Vertex* writePtr = m_vertices.allocateBatch(leftToCopy, numAllocated);
                    Vertex* orgWritePtr = writePtr;

                    if (m_pixelTransformClass == XForm2DClass::Identity)
                    {
                        for (uint32_t i = 0; i < numAllocated; ++i, ++readPtr, ++writePtr)
                        {
                            writePtr->pos = readPtr->pos;
                            writePtr->paintUV = readPtr->uv;
                            writePtr->uv = readPtr->uv;
                            writePtr->color = readPtr->color;
                            writePtr->paramsId = paramId;
                        }
                    }
                    else if (m_pixelTransformClass == XForm2DClass::HasTransform)
                    {
                        for (uint32_t i = 0; i < numAllocated; ++i, ++readPtr, ++writePtr)
                        {
                            writePtr->pos.x = readPtr->pos.x + m_pixelTransform.t[4];
                            writePtr->pos.y = readPtr->pos.y + m_pixelTransform.t[5];
                            writePtr->paintUV = readPtr->uv;
                            writePtr->uv = readPtr->uv;
                            writePtr->color = readPtr->color;
                            writePtr->paramsId = paramId;
                        }
                    }
                    else
                    {
                        for (uint32_t i = 0; i < numAllocated; ++i, ++readPtr, ++writePtr)
                        {
                            writePtr->pos = m_pixelTransform.transformPoint(readPtr->pos);
                            writePtr->paintUV = readPtr->uv;
                            writePtr->uv = readPtr->uv;
                            writePtr->color = readPtr->color;
                            writePtr->paramsId = paramId;
                        }
                    }

                    calcScissorUV(orgWritePtr, numAllocated);

                    leftToCopy -= numAllocated;
                }
            }

            return firstVertex;
        }

        uint32_t Canvas::packVerticesTransformed(const Vertex* vertexData, uint32_t numVertices)
        {
            uint32_t firstVertex = m_vertices.size();
            uint32_t leftToCopy = numVertices;
            auto readPtr = vertexData;

            while (leftToCopy > 0)
            {
                uint32_t numAllocated = 0;
                Vertex* writePtr = m_vertices.allocateBatch(leftToCopy, numAllocated);

                memcpy(writePtr, readPtr, numAllocated * sizeof(Vertex));
                calcScissorUV(writePtr, numAllocated);

                readPtr += numAllocated;
                leftToCopy -= numAllocated;
            }

            return firstVertex;
        }

        uint32_t Canvas::packIndices(const uint16_t* indices, uint32_t numIndices, uint32_t baseVertexIndex)
        {
            uint32_t firstIndex = m_indices.size();

            {
                uint32_t leftToCopy = numIndices;
                auto readPtr  = indices;

                while (leftToCopy > 0)
                {
                    uint32_t numAllocated = 0;
                    uint32_t* writePtr = m_indices.allocateBatch(leftToCopy, numAllocated);

                    for (uint32_t i = 0; i < numAllocated; ++i, ++readPtr, ++writePtr)
                        *writePtr = *readPtr + baseVertexIndex;

                    leftToCopy -= numAllocated;
                }
            }

            return firstIndex;
        }

        uint32_t Canvas::packTriangleStrips(uint32_t baseVertexIndex, uint32_t numPathVertices)
        {
            auto firstIndex = m_indices.size();

            for (uint32_t j = 2; j < numPathVertices; ++j)
            {
                uint32_t writeIndex[3];
                if (j & 1)
                {
                    writeIndex[2] = baseVertexIndex + j;
                    writeIndex[1] = baseVertexIndex + j - 1;
                    writeIndex[0] = baseVertexIndex + j - 2;
                }
                else
                {
                    writeIndex[0] = baseVertexIndex + j - 2;
                    writeIndex[1] = baseVertexIndex + j - 1;
                    writeIndex[2] = baseVertexIndex + j;
                }

                m_indices.writeSmall(writeIndex, 3);
            }

            return firstIndex;
        }

        uint32_t Canvas::packTriangleList(uint32_t baseVertexIndex, uint32_t numPathVertices)
        {
            auto firstIndex = m_indices.size();

            // TODO: rewrite to use allocBatch

            for (uint32_t j = 2; j < numPathVertices; ++j)
            {
                uint32_t indices[3];
                indices[0] = baseVertexIndex + 0;
                indices[1] = baseVertexIndex + j - 1;
                indices[2] = baseVertexIndex + j;
                m_indices.writeSmall(indices, 3);
            }

            return firstIndex;
        }

        uint32_t Canvas::packTriangleQuads(uint32_t baseVertexIndex, uint32_t numQuadVertices)
        {
            DEBUG_CHECK(0 == (numQuadVertices % 4));

            auto firstIndex = m_indices.size();

            auto numQuads = numQuadVertices / 4;
            auto numIndicesLeft = numQuads * 6;

            const uint8_t quadPattern[6] = { 0,1,2,0,2,3 };
            
            uint32_t quadIndex = 0;
            uint32_t quadVertex = 0;
            while (numIndicesLeft > 0)
            {
                uint32_t numToWrite = 0;
                uint32_t* writePtr = m_indices.allocateBatch(numIndicesLeft, numToWrite);
                numIndicesLeft -= numToWrite;

                uint32_t* writePtrEnd = writePtr + numToWrite;
                while (writePtr < writePtrEnd)
                {
                    *writePtr++ = baseVertexIndex + (quadIndex * 4) + quadPattern[quadVertex];

                    quadVertex += 1;
                    if (quadVertex == 6)
                    {
                        quadIndex += 1;
                        quadVertex = 0;
                    }
                }
            }

            return firstIndex;
        }

        void Canvas::renderFill(const RenderGroup& group, uint32_t paramsID)
        {
            // copy vertex data of all sub-path in one go
            auto baseVertexIndex = packVertices(group, paramsID);

            // create the batches for all rendering parts of the render group
            for (uint32_t i = 0; i < group.numPaths; ++i)
            {
                auto& srcPath = group.paths[i];
                if (srcPath.fillVertexCount > 0)
                {
                    auto batch  = m_batches.allocSingle();
                    batch->fristIndex = packTriangleList(baseVertexIndex, srcPath.fillVertexCount);
                    batch->numIndices = m_indices.size() - batch->fristIndex;
                    //batch->paramsId = paramsID;
                    batch->op = group.op;
                    batch->customDrawer = 0;
                    batch->customPayload = nullptr;
                    batch->type = group.convex ? BatchType::ConvexFill : BatchType::ConcaveMask;

                    baseVertexIndex += srcPath.fillVertexCount;
                }
            }

            // create the "fill" batch for filling in the concave shit that is masked in previous calls
            if (!group.convex)
            {
                auto batch  = m_batches.allocSingle();
                batch->fristIndex = packTriangleList(baseVertexIndex, 4);
                batch->numIndices = m_indices.size() - batch->fristIndex;
                batch->op = group.op;
                batch->type = BatchType::ConcaveFill;
                batch->customDrawer = 0;
                batch->customPayload = nullptr;
            }
        }

        void Canvas::renderStroke(const RenderGroup& group, uint32_t paramsId)
        {
            // copy vertex data of all sub-path in one go
            auto baseVertexIndex = packVertices(group, paramsId);

            // create the batches for all rendering parts of the render group
            for (uint32_t i = 0; i < group.numPaths; ++i)
            {
                auto& srcPath = group.paths[i];
                if (srcPath.strokeVertexCount > 0)
                {
                    auto batch  = m_batches.allocSingle();
                    batch->fristIndex = packTriangleStrips(baseVertexIndex, srcPath.strokeVertexCount);
                    batch->numIndices = m_indices.size() - batch->fristIndex;
                    batch->op = group.op;
                    batch->type = BatchType::ConvexFill;
                    batch->customDrawer = 0;
                    batch->customPayload = nullptr;

                    baseVertexIndex += srcPath.strokeVertexCount;
                }
            }
        }

        void Canvas::flushGlyphCache(LocalGlyphCache& cache)
        {
            // push all vertices allocated so far
            uint32_t baseVertexIndex = m_vertices.size();
            calcScissorUV(cache.m_localVertices, cache.m_numLocalGlyphs * 4);
            m_vertices.writeLarge(cache.m_localVertices, cache.m_numLocalGlyphs * 4);

            // generate index buffer
            uint32_t baseIndex = packTriangleQuads(baseVertexIndex, cache.m_numLocalGlyphs * 4);

            // emit batch
            auto batch = m_batches.allocSingle();
            batch->fristIndex = baseIndex;
            batch->numIndices = cache.m_numLocalGlyphs * 6;
            batch->op = CompositeOperation::SourceOver;
            batch->type = BatchType::ConvexFill;
            batch->customDrawer = 0;
            batch->customPayload = nullptr;

            // reset state
            cache.m_numLocalGlyphs = 0;
        }

        void Canvas::mapNewGlyphPagesToParameters(uint64_t newPagesMask)
        {
            DEBUG_CHECK_EX(newPagesMask != 0, "Why call this if nothing to do ?");

            while (newPagesMask != 0)
            {
                uint32_t pageIndex = __builtin_ctzll(newPagesMask);

                DEBUG_CHECK(m_glyphPageToParamsIdMapping[pageIndex] == -1);
                DEBUG_CHECK_EX(0 == (m_mappedGlyphPages & (1ULL << pageIndex)), "Page was not marked as used");

                {
                    auto glyphCacheImage = GlyphCache::GetInstance().page(pageIndex);
                    DEBUG_CHECK_EX(glyphCacheImage, "No glyph cache image");

                    auto paramIndex = packDirectTextureParams(glyphCacheImage);
                    m_glyphPageToParamsIdMapping[pageIndex] = paramIndex;
                    m_mappedGlyphPages |= (1ULL << pageIndex); // mark page as used
                }
                    
                newPagesMask ^= newPagesMask & -newPagesMask;
            }
        }

        void Canvas::renderGlyphs(const RenderGroup& group)
        {
            static LocalGlyphCache localCache;

            // pack the glyphs into glyphs cache, group the glyphs by the glyphs page needed
            base::Color color = base::Color::WHITE;
            base::Color finalColor = base::Color::WHITE;
            for (uint32_t i = 0; i < group.numGlyphs; ++i)
            {
                auto& glyph = group.glyphs[i];

                base::Vector2 pos[4];
                if ((int)m_pixelTransformClass >= (int)XForm2DClass::HasTransform)
                {
                    pos[0] = m_pixelTransform.transformPoint(glyph.coords[0]);
                    pos[1] = m_pixelTransform.transformPoint(glyph.coords[1]);
                    pos[2] = m_pixelTransform.transformPoint(glyph.coords[2]);
                    pos[3] = m_pixelTransform.transformPoint(glyph.coords[3]);
                }
                else
                {
                    pos[0].x = glyph.coords[0].x + m_pixelTransform.t[4];
                    pos[0].y = glyph.coords[0].y + m_pixelTransform.t[5];
                    pos[1].x = glyph.coords[1].x + m_pixelTransform.t[4];
                    pos[1].y = glyph.coords[1].y + m_pixelTransform.t[5];
                    pos[2].x = glyph.coords[2].x + m_pixelTransform.t[4];
                    pos[2].y = glyph.coords[2].y + m_pixelTransform.t[5];
                    pos[3].x = glyph.coords[3].x + m_pixelTransform.t[4];
                    pos[3].y = glyph.coords[3].y + m_pixelTransform.t[5];
                }

                if (TestClip(m_pixelScissorRect, pos[0].x, pos[0].y, pos[2].x, pos[2].y))
                {
                    if (localCache.full())
                        flushGlyphCache(localCache);

                    // get mapped page
                    auto paramsIndex = m_glyphPageToParamsIdMapping[glyph.page];
                    DEBUG_CHECK_EX(paramsIndex != -1, "Glyph page should already be mapped, did you call prepareGlyphsForRendering() on your Geometry ?");

                    // update color
                    if (color != glyph.color)
                    {
                        color = glyph.color;
                        finalColor = color;
                        finalColor.a = FloatTo255(FloatFrom255(finalColor.a) * m_alpha);
                    }

                    // write glyph vertices
                    auto writeVertex  = localCache.m_localVertices + (4 * localCache.m_numLocalGlyphs);
                    writeVertex[0].pos = pos[0];
                    writeVertex[0].uv.x = glyph.uvMin.x;
                    writeVertex[0].uv.y = glyph.uvMin.y;
                    writeVertex[1].pos = pos[1];
                    writeVertex[1].uv.x = glyph.uvMax.x;
                    writeVertex[1].uv.y = glyph.uvMin.y;
                    writeVertex[2].pos = pos[2];
                    writeVertex[2].uv.x = glyph.uvMax.x;
                    writeVertex[2].uv.y = glyph.uvMax.y;
                    writeVertex[3].pos = pos[3];
                    writeVertex[3].uv.x = glyph.uvMin.x;
                    writeVertex[3].uv.y = glyph.uvMax.y;
                    writeVertex[0].color = finalColor;
                    writeVertex[1].color = finalColor;
                    writeVertex[2].color = finalColor;
                    writeVertex[3].color = finalColor;
                    writeVertex[0].paramsId = paramsIndex;
                    writeVertex[1].paramsId = paramsIndex;
                    writeVertex[2].paramsId = paramsIndex;
                    writeVertex[3].paramsId = paramsIndex;

                    // add glyph to linked list of given page
                    localCache.m_numLocalGlyphs += 1;
                }
            }

            // flush anything not yet flushed
            flushGlyphCache(localCache);
        }

        //---

        void Canvas::customQuad(const RenderStyle& style, float x0, float y0, float x1, float y1, float u0 /*= 0.0f*/, float v0 /*= 0.0f*/, float u1 /*= 1.0f*/, float v1 /*= 1.0f*/, base::Color color /*= base::Color::WHITE*/, base::canvas::CompositeOperation op /*= base::canvas::CompositeOperation::Blend*/)
        {
            base::canvas::Canvas::RawVertex v[4];
            v[0].uv.x = u0;
            v[0].uv.y = v0;
            v[1].uv.x = u1;
            v[1].uv.y = v0;
            v[2].uv.x = u1;
            v[2].uv.y = v1;
            v[3].uv.x = u0;
            v[3].uv.y = v1;
            v[0].color = color;
            v[1].color = color;
            v[2].color = color;
            v[3].color = color;
            v[0].pos.x = x0;
            v[0].pos.y = y0;
            v[1].pos.x = x1;
            v[1].pos.y = y0;
            v[2].pos.x = x1;
            v[2].pos.y = y1;
            v[3].pos.x = x0;
            v[3].pos.y = y1;

            uint16_t i[6];
            i[0] = 0;
            i[1] = 1;
            i[2] = 2;
            i[3] = 0;
            i[4] = 2;
            i[5] = 3;

            base::canvas::Canvas::RawGeometry geom;
            geom.indices = i;
            geom.vertices = v;
            geom.numIndices = 6;

            place(style, geom, 0, nullptr, op);
        }

        void Canvas::customQuad(uint16_t customDrawerId, const void* data, float x0, float y0, float x1, float y1, float u0 /*= 0.0f*/, float v0 /*= 0.0f*/, float u1 /*= 1.0f*/, float v1 /*= 1.0f*/, base::Color color /*= base::Color::WHITE*/, base::canvas::CompositeOperation op /*= base::canvas::CompositeOperation::Blend*/)
        {
            base::canvas::Canvas::RawVertex v[4];
            v[0].uv.x = x0;
            v[0].uv.y = y0;
            v[1].uv.x = x1;
            v[1].uv.y = y0;
            v[2].uv.x = x1;
            v[2].uv.y = y1;
            v[3].uv.x = x0;
            v[3].uv.y = y1;
            v[0].color = color;
            v[1].color = color;
            v[2].color = color;
            v[3].color = color;
            v[0].pos.x = u0;
            v[0].pos.y = v0;
            v[1].pos.x = u1;
            v[1].pos.y = v0;
            v[2].pos.x = u1;
            v[2].pos.y = v1;
            v[3].pos.x = u0;
            v[3].pos.y = v1;

            uint16_t i[6];
            i[0] = 0;
            i[1] = 1;
            i[2] = 2;
            i[3] = 0;
            i[4] = 2;
            i[5] = 3;

            base::canvas::Canvas::RawGeometry geom;
            geom.indices = i;
            geom.vertices = v;
            geom.numIndices = 6;

            const auto style = base::canvas::SolidColor(base::Color::WHITE);
            place(style, geom, customDrawerId, data, op);
        }

        //---
        
    } // canvas
} // base