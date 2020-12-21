/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#pragma once

#include "base/image/include/image.h"
#include "base/canvas/include/canvasGeometry.h"
#include "base/containers/include/inplaceArray.h"
#include "base/containers/include/pagedBuffer.h"
#include "base/memory/include/linearAllocator.h"

namespace rendering
{
	namespace canvas
	{
		class CanvasRenderService;
	}
}

namespace base
{
    namespace canvas
    {

		//--

		struct Placement
		{
			INLINE Placement() {};
			INLINE Placement(const Placement& other) = default;
			INLINE Placement(Placement&& other) = default;
			INLINE Placement& operator=(const Placement& other) = default;
			INLINE Placement& operator=(Placement && other) = default;

			INLINE Placement(const XForm2D& full)
				: placement(full)
				, noScale(false)
				, simple(false)
			{}

			INLINE Placement(const Vector2& pos)
				: placement(pos.x, pos.y)
				, noScale(true)
				, simple(true)
			{}

			INLINE Placement(const Vector2& pos, float scale)
				: placement(scale, 0, 0, scale, pos.x, pos.y)
				, noScale(scale == 1.0f)
				, simple(true)
			{}

			INLINE Placement(float x, float y)
				: placement(x, y)
				, noScale(true)
				, simple(true)
			{}

			INLINE Placement(float x, float y, float scale)
				: placement(x, y)
				, noScale(scale == 1.0f)
				, simple(true)
			{}

			XForm2D placement;
			bool simple = true;
			bool noScale = true;
		};
		
		//--

        /// canvas renderer, collects final vertices and batches for rendering a 2D stuff
        /// NOTE: this class is VERY HEAVY and should be not instanced hastily
		class BASE_CANVAS_API Canvas : public NoCopy
		{
			RTTI_DECLARE_POOL(POOL_CANVAS);

		public:
			struct Setup
			{
				uint32_t width = 0;
				uint32_t height = 0;
				Vector2 pixelOffset = Vector2::ZERO();
				float pixelScale = 1.0f;
			};

			Canvas(const Setup& setup);
			Canvas(uint32_t width, uint32_t height);
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

			// get the total "vertex in pushed geometry -> render target" transform
			INLINE const XForm2D& pixelTransform() const { return m_pixelTransform; }

			// clear color we want to have the whole output filled before drawing this canvas, mainly used to speed up ui rendering
			INLINE const Color& clearColor() const { return m_clearColor; }

			//--

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

			/// draw geometry into the canvas at given ABSOLUTE placement, will bake it on the fly - SLOW
			/// NOTE: all vertices in the geometry will be transformed by the provided placement (+ the canvas "global" pixel offset/scale)
			void place(const Placement& pos, const Geometry& geometry, float alpha = 1.0f);
			void place(float x, float y, const Geometry& geometry, float alpha = 1.0f);

			//--

			struct QuadSetup
			{
				float x0 = 0.0f;
				float y0 = 0.0f;
				float x1 = 100.0f;
				float y1 = 100.0f;
				float u0 = 0.0f;
				float v0 = 0.0f;
				float u1 = 1.0f;
				float v1 = 1.0f;

				uint8_t renderer = 0;
				Color color = Color::WHITE;
				canvas::BlendOp op = canvas::BlendOp::AlphaPremultiplied;

				bool wrap = false;

				ImageEntry image;
			};

			/// place a simple quad, mostly useful for custom renderer stuff/debug/simple effects etc
			void quad(const Placement& placement, const QuadSetup& setup);

			///---

			/// adjust pixel offset/placement
			void pixelPlacement(float pixelOffsetX, float pixelOffsetY, float pixelScale = 1.0f);

			//---

		private:
			static const uint32_t MAX_LOCAL_VERTICES = 65536;
			static const uint32_t MAX_LOCAL_BATCHES = 4096;
			static const uint32_t MAX_LOCAL_ATTRIBUTES = 1024;
			static const uint32_t MAX_LOCAL_DATA = 8192;

			uint32_t m_width = 0;
			uint32_t m_height = 0;

			Vector2 m_pixelOffset;
			float m_pixelScale = 1.0f;
			XForm2D m_pixelTransform;

			uint8_t m_currentRenderer = 0;

			uint64_t m_usedAtlasMask = 0;
			uint64_t m_usedGlyphPagesMask = 0;

			Color m_clearColor;

			//--

			bool m_emptyScissorRect = true;

			Vector4 m_scissorRect;
			Vector4 m_pixelScissorRect;
			Vector4 m_pixelScissorShaderValues; // -x, -y, 1/width, 1/height -> transforms pixel's position into [0,0]-[1,1] range for clipping

			void updatePixelTransform();
			void updateScissorRect();

			//--

			InplaceArray<Vector4, 16> m_scissorRectStack;
			InplaceArray<uint8_t, 16> m_customRendererStack;

			//--

			void transformBounds(const Placement& transform, const Vector2& localMin, const Vector2& localMax, Vector2& globalMin, Vector2& globalMax) const;

			//--

			Vertex m_tempVertices[MAX_LOCAL_VERTICES];

			PagedBufferTyped<Vertex> m_gatheredVertices;
			base::InplaceArray<Batch, MAX_LOCAL_BATCHES> m_gatheredBatches;
			base::InplaceArray<Attributes, MAX_LOCAL_BATCHES> m_gatheredAttributes;
			base::InplaceArray<uint8_t, MAX_LOCAL_BATCHES> m_gatheredData;

			void placeInternal(const Placement& placement, const Vertex* vertices, uint32_t numVertices, uint32_t firstAttributeIndex, uint32_t firstDataOffset, const Batch& batchInfo, float alpha);
			void placeVertex(const Placement& placement, const Vertex* src, Vertex*& dest, uint32_t firstAttributeIndex);

			//--

			friend class rendering::canvas::CanvasRenderService;
		};

		//--

    } // canvas
} // base

