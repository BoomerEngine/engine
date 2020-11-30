/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: renderer #]
***/

#pragma once

#include "base/image/include/image.h"
#include "base/canvas/include/canvasGeometry.h"
#include "base/containers/include/inplaceArray.h"
#include "base/containers/include/pagedBuffer.h"
#include "base/memory/include/linearAllocator.h"

namespace base
{
    namespace canvas
    {

        /// canvas renderer, integrates canvas system with external rendering
        /// NOTE: this class is VERY HEAVY and should be not instanced hastily
        class BASE_CANVAS_API Canvas : public base::NoCopy
        {
        public:
            Canvas(uint32_t width, uint32_t height, GlyphCache* glyphCache = nullptr /* uses the shared cache*/, float pixelOffsetX =0.0f, float pixelOffsetY=0.0f, float pixelScale=1.0f);
            ~Canvas();

            // get width of the currently bound target to the geometry collector
            INLINE uint32_t width() const { return m_width; }

            // get height of the currently bound target to the geometry collector
            INLINE uint32_t height() const { return m_height; }

            // get internal pixel offset, this is usually used for "canvas -> window" transformations when why go from abstract canvas space to an absolute window space
            // for example when rendering content of the UI system all of the UI's shapes are placed in absolute desktop coordinates but, obviously, when rendering them to render target we need to take into account when the actual window is located
            INLINE float pixelOffsetX() const { return m_pixelOffset.x; }
            INLINE float pixelOffsetY() const { return m_pixelOffset.y; }
            INLINE const Vector2& pixelOffset() const { return m_pixelOffset; }

            // get internal pixel scale
            // main purpose is to lousy support for DPI scaling
            INLINE float pixelScale() const { return m_pixelScale; }

            // clear color we want to have the whole output filled before drawing this canvas, mainly used to speed up ui rendering
            INLINE const Color& clearColor() const { return m_clearColor; }

            // get mask of all used glyph cache pages
            INLINE uint64_t glyphCachePageMask() const { return m_glyphCachePageMask; }

#pragma pack(push)
#pragma pack(1)
            struct Vertex
            {
                Vector2 pos;
                Vector2 uv; // always user specific
                Vector2 paintUV; // UV from the style matrix, copied from original vertices
                Vector2 clipUV; // UV from the scissor, calculated when placing vertices
                Color color;
                //uint32_t paramsId;
                float paramsId;
            };
#pragma pack(pop)

            struct ImageRef
            {
                const base::image::Image* imagePtr; // image to use while drawing
                mutable uint32_t imageIndex:29;
                mutable uint32_t imageType:2;
                mutable uint32_t imageNeedsWrapping : 1;

                ImageRef* next = nullptr; // for hash map
            };

            struct Params
            {
                Vector4 innerCol;
                Vector4 outerCol;

                Vector2 base;
                Vector2 extent;
                Vector2 uvMin;
                Vector2 uvMax;

                float featherHalf = 0.0f;
                float featherInv = 0.0f;
                float radius = 0.0f;
                float feather = 0.0f;

                uint32_t wrapType = 0;
                const ImageRef* imageRef = nullptr;
            };

            enum class BatchType : uint8_t // keep minimal to preserve batching, especially don't use anything special for TEXT!
            {
                ConcaveFill,
                ConvexFill,
                ConcaveMask,
                Custom,
            };

            struct Batch
            {
                BatchType type;
                CompositeOperation op;
                uint8_t customDrawer = 0; // 0-default drawer
                const void* customPayload = nullptr;

                uint32_t fristIndex;
                uint32_t numIndices;
            };
            
            struct RawVertex // NOTE: aimed at ImGui integration, keep in sync
            {
                Vector2 pos;
                Vector2 uv;
                Color color;
            };

            struct RawGeometry
            {
                RawVertex* vertices; // NOTE: only actually used vertices are extracted
                uint16_t* indices;
                uint32_t numIndices;
            };

            struct RenderStyleWithAlphaKey
            {
                const RenderStyle& style;
                float alpha = 1.0f;

                INLINE RenderStyleWithAlphaKey(const RenderStyle& style_, float alpha_)
                    : style(style_)
                    , alpha(alpha_)
                {}

                INLINE static uint32_t CalcHash(const RenderStyleWithAlphaKey& style)
                {
                    return (size_t)style.style.hash ^ reinterpret_cast<const uint32_t&>(style.alpha);
                }
            };

            struct RenderStyleWithAlpha
            {
                RenderStyle style;
                float alpha = 1.0f;

                INLINE RenderStyleWithAlpha(const RenderStyle& style_, float alpha_)
                    : style(style_)
                    , alpha(alpha_)
                {}

                INLINE bool operator==(const RenderStyleWithAlpha& other) const
                {
                    return (alpha == other.alpha) && (style == other.style);
                }

                INLINE bool operator==(const RenderStyleWithAlphaKey& other) const
                {
                    return (alpha == other.alpha) && (style == other.style);
                }

                INLINE static uint32_t CalcHash(const RenderStyleWithAlphaKey& style)
                {
                    return (size_t)style.style.hash ^ reinterpret_cast<const uint32_t&>(style.alpha);
                }

                INLINE static uint32_t CalcHash(const RenderStyleWithAlpha& style)
                {
                    return (size_t)style.style.hash ^ reinterpret_cast<const uint32_t&>(style.alpha);
                }
            };

            ///---

            INLINE const PagedBufferTyped<Vertex>& vertices() const { return m_vertices; }
            INLINE const PagedBufferTyped<uint32_t>& indices() const { return m_indices; }
            INLINE const PagedBufferTyped<Params>& params() const { return m_params; }
            INLINE const PagedBufferTyped<ImageRef>& images() const { return m_images; }
            INLINE const PagedBufferTyped<Batch>& baches() const { return m_batches; }

            ///---

            // get current logical transform for geometry we are pushing
            // NOTE: this does not take into account the pixel transform that is applied on top
            INLINE const XForm2D& transform() const { return m_transform; }

            // get the total "vertex in pushed geometry -> render target" transform
            INLINE const XForm2D& pixelTransform() const { return m_pixelTransform; }

            // set transform for next batch of geometries
            // NOTE: this does not stack, this is the "current" transform
            void placement(const XForm2D& absoluteXForm);

            // set transform for next batch of geometries
            // NOTE: this does not stack, this is the "current" transform
            void placement(float tx, float ty, float scale=1.0f);

            // set transform for next batch of geometries
            // NOTE: this does not stack, this is the "current" transform
            void placement(float tx, float ty, float sx, float sy);

            ///---

            // set the global alpha multiplier for everything pushed, main purpose is to facilitate simple fade-in/fade-out of cached geometry
            // NOTE: this is done by affecting the RenderStyle collected, not the geometry itself (ie. no per-vertex operations, this is cheap)
            void alphaMultiplier(float alpha);

            ///---

            // clear the scissor stack
            void clearScissorRectStack();

            // reset scissor rectangle to the whole canvas area
            void resetScissorRect();

            // push current scissor rect on a stack
            // this is mostly for facilitating hierarchy of objects in UI
            void pushScissorRect();

            // pop previously pushed scissor rect
            void popScissorRect();

            // set scissor clip to a specific rectangle
            // NOTE: current transform DOES NOT AFFECT SCISSOR!
            // NOTE: scissor rectangle is provided here in canvas space but is recomputed to pixel space for rendering
            bool scissorRect(const Vector2& position, const Vector2& extents);
            bool scissorRect(float x, float y, float w, float h);
            bool scissorBounds(const Vector2& bmin, const Vector2& bmax);
            bool scissorBounds(float x0, float y0, float x1, float y1);

            // intersect current scissor rect with a new one
            // NOTE: current transform DOES NOT AFFECT SCISSOR!
            // NOTE: scissor rectangle is provided here in canvas space but is recomputed to pixel space for rendering
            // NOTE: if the final scissor rectangle is empty a FALSE is returned to facilitate culling
            bool intersectScissorRect(const Vector2& position, const Vector2& extents);
            bool intersectScissorRect(float x, float y, float w, float h);
            bool intersectScissorBounds(const Vector2& bmin, const Vector2& bmax);
            bool intersectScissorBounds(float x0, float y0, float x1, float y1);

            // is the current top-level scissor empty ?
            INLINE bool emptyScissorRect() const { return m_emptyScissorRect; }

            // get the top-level scissor range [x0,y0] - [x1,y1], canvas coordinates
            INLINE Vector4 scissorRect() const { return m_scissorRect; }

            // get the top-level scissor range [x0,y0] - [x1,y1], IN FINAL PIXEL coordinates
            INLINE Vector4 pixelScissorRect() const { return m_pixelScissorRect; }

            // test if a given area would pass the current scissor rect
            bool testScissorRect(const Vector2& position, const Vector2& extents) const;
            bool testScissorRect(float x, float y, float w, float h) const;
            bool testScissorBounds(const Vector2& bmin, const Vector2& bmax) const;
            bool testScissorBounds(float x0, float y0, float x1, float y1) const;

            // configure clear color
            INLINE void clearColor(const Color& color) { m_clearColor = color; }

            ///---

            /// create custom payload, can be any data
            void* uploadCustomPayloadData(const void* data, uint32_t size);

            /// create custom payload, can be any data
            template< typename T >
            INLINE T* uploadCustomPayloadData(const T& data) { return (T*)uploadCustomPayloadData(&data, sizeof(data)); }

            ///---

            /// clear the whole collector
            void clear();

            /// draw cached geometry into the canvas
            void place(const Geometry& geometry);

            /// draw raw geometry builder
            void place(const GeometryBuilder& geometry);

            /// place raw geometry data in canvas, paint it whole specific render style
            void place(const RenderStyle& style, const RawGeometry& geometry, uint16_t customDrawer = 0, const void* customPayload = nullptr, CompositeOperation op = CompositeOperation::SourceOver, float alpha = 1.0f);

            //----

            // render quad for custom style
            void customQuad(const RenderStyle& style, float x0, float y0, float x1, float y1, float u0 = 0.0f, float v0 = 0.0f, float u1 = 1.0f, float v1 = 1.0f, base::Color color = base::Color::WHITE, base::canvas::CompositeOperation op = base::canvas::CompositeOperation::Blend);

            // render quad for custom handler
            void customQuad(uint16_t id, const void* data, float x0, float y0, float x1, float y1, float u0 = 0.0f, float v0 = 0.0f, float u1 = 1.0f, float v1 = 1.0f, base::Color color = base::Color::WHITE, base::canvas::CompositeOperation op = base::canvas::CompositeOperation::Blend);

            ///---

            /// adjust pixel offset/placement
            void pixelPlacement(float pixelOffsetX, float pixelOffsetY, float pixelScale = 1.0f);

        protected:
            const ImageRef* packImageRef(const image::Image* image, bool needsWrapping = false);

            uint32_t packParams(const RenderStyle& style, float alpha);
            uint32_t packDirectTextureParams(const base::image::Image* glyphCacheImage);

            uint32_t packIndices(const uint16_t* indices, uint32_t numIndices, uint32_t baseVertexIndex);
            uint32_t packVertices(const RenderGroup& group, uint32_t paramId);
            uint32_t packVertices(const Vertex* vertexData, uint32_t numVertices, uint32_t paramId);
            uint32_t packRawVertices(const RawVertex* vertexData, uint32_t numVertices, uint32_t paramId);
            uint32_t packVerticesTransformed(const Vertex* vertexData, uint32_t numVertices);
            uint32_t packTriangleList(uint32_t baseVertexIndex, uint32_t numPathVertices);
            uint32_t packTriangleStrips(uint32_t baseVertexIndex, uint32_t numPathVertices);
            uint32_t packTriangleQuads(uint32_t baseVertexIndex, uint32_t numQuadVertices);

            void renderFill(const RenderGroup& group, uint32_t paramsId);
            void renderStroke(const RenderGroup& group, uint32_t paramsId);
            void renderGlyphs(const RenderGroup& group);

            void calcScissorUV(Vertex* first, uint32_t count);

            //--

            uint32_t m_width;
            uint32_t m_height;
            float m_alpha;
            Vector2 m_pixelOffset;
            float m_pixelScale;
            XForm2DClass m_pixelTransformClass;
            XForm2DClass m_transformClass;
            bool m_emptyScissorRect;
            bool m_hasPixelTransform;
            Color m_clearColor;

            uint32_t m_numBatchedCulled = 0;
            uint32_t m_numGeometriesCulled = 0;

            Array<int> m_styleMapping;

            uint64_t m_glyphCachePageMask;

            XForm2D m_transform;
            XForm2D m_pixelTransform;

            void updatePixelTransform();

            Vector4 m_scissorRect;
            Vector4 m_pixelScissorRect;
            Vector4 m_pixelScissorShaderValues; // -x, -y, 1/width, 1/height -> transforms pixel's position into [0,0]-[1,1] range for clipping

            void updateScissorRect();

            void transformBounds(const Vector2& localMin, const Vector2& localMax, Vector2& globalMin, Vector2& globalMax) const;

            //--

            PagedBufferTyped<Vertex> m_vertices;
			PagedBufferTyped<uint32_t> m_indices;
			PagedBufferTyped<Params> m_params;
			PagedBufferTyped<Batch> m_batches;
			PagedBufferTyped<ImageRef> m_images;

            mem::LinearAllocator m_customPayloadData;

            HashMap<RenderStyleWithAlpha, uint32_t> m_paramsMap;

            struct ScissorEntry
            {
                Vector4 rect;
            };

            InplaceArray<ScissorEntry, 16> m_scissorRectStack;

            //--

            static const uint32_t MAX_IMAGE_BUCKETS = 256; // TODO: if we have shitloads of images this will be to small
            ImageRef* m_imageRefBuckets[MAX_IMAGE_BUCKETS];
            ImageRef* m_imageRefList = nullptr;

            void releaseImageReferences();

            //--

            struct LocalGlyphCache
            {
                static const uint32_t MAX_LOCAL_GLYPHS = 1024;

                uint32_t m_numLocalGlyphs = 0;

                Canvas::Vertex m_localVertices[MAX_LOCAL_GLYPHS * 4];  // for building
                uint32_t m_localIndices[MAX_LOCAL_GLYPHS * 6]; // for building

                INLINE bool full() const
                {
                    return m_numLocalGlyphs >= MAX_LOCAL_GLYPHS;
                }
            };

            void flushGlyphCache(LocalGlyphCache& cache);

            void mapNewGlyphPagesToParameters(uint64_t newPages);

            static const uint32_t MAX_GLYPH_PAGES = 64;

            int m_glyphPageToParamsIdMapping[64];
            uint64_t m_mappedGlyphPages = 0;
        };

    } // canvas
} // base

