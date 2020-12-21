/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

// The Canvas class is heavily based on nanovg project by Mikko Mononen
// Adaptations were made to fit the rest of the source code in here

//
// Copyright (c) 2013 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#pragma once

#include "canvasStyle.h"
#include "canvasGeometry.h"

namespace base
{
    namespace canvas
    {
        namespace prv
        {
            struct PathCache;
        } // prv

        /// style of drawing the ends of the lines 
        enum class LineCap : uint8_t
        {
            Butt,
            Square,
            Round,
        };

        /// style of drawing the line joints
        enum class LineJoin : uint8_t
        {
            Bevel,
            Round,
            Miter,
        };

        /// type of geometry to emit from the constructed path
        enum class GeometryType : uint8_t
        {
            Stroke = 1,
            Fill = 2,
        };

        /// helper class that build a renderable Geometry objects step-by-step
        /// the created Geometry objects are self contained and can be reused for rendering much later
        /// the whole point is to minimize CPU overhead when redrawing large and mostly static GUI
		/// NOTE: data storage is required for using cached images and fonts
        class BASE_CANVAS_API GeometryBuilder : public NoCopy // should be instanced on the stack
        {
            RTTI_DECLARE_POOL(POOL_CANVAS)

        public:
            GeometryBuilder(Geometry& outGeometry);
            ~GeometryBuilder();

            //---

            /// get current fill rendering style
            INLINE const RenderStyle& fillStyle() const { return m_style.fillStyle; }

            /// get current stroke rendering style
            INLINE const RenderStyle& strokeStyle() const { return m_style.strokeStyle; }

            // get the current XForm transform that is being applied to the geometry
            INLINE const XForm2D& transform() const { return m_transform; }

			// current blend operation
			INLINE BlendOp blending() const { return m_style.op; }

            //---

			// reset builder - clear renderin state, transform and internal state
			// NOTE: does not clear geometry in the output geometry
			void reset();

            // Sets the composite operation
            // The blending modes are encoded into generated geometry (this way we can create whole background, content and frame in one go)
            void blending(BlendOp op);

            //---

            // Push current render state (fill, pen, transform, etc onto a stack)
            void pushState();

            // Pop previously pushed state from the stack
            void popState();

            // Reset state
            void resetState();

            //---

            // Sets current stroke style to a solid color.
            void strokeColor(const Color& color, float width = 1.0f);

            // Sets current stroke style to a paint, which can be a one of the gradients or a pattern.
            void strokePaint(const RenderStyle& style, float width = 1.0f);

            // Sets current fill style to a solid color.
            void fillColor(const Color& color);

            // Sets current fill style to a paint, which can be a one of the gradients or a pattern.
            void fillPaint(const RenderStyle& style);

            // Sets the miter limit of the stroke style.
            // Miter limit controls when a sharp corner is beveled.
            void miterLimit(float limit);

            // Sets how the end of the line (cap) is drawn,
            // Can be one of: Butt (default), Round, Square.
            void lineCap(LineCap capStyle);

            // Sets how sharp path corners are drawn.
            // Can be one of Miter (default), Round, Bevel.
            void lineJoin(LineJoin jointStyle);

            // Sets the transparency applied to all rendered shapes.
            // Already transparent paths will get proportionally more transparent as well.
            void globalAlpha(float alpha);

			// Toggle antialiasing on/off
			void antialiasing(bool flag);

            //---

            // Resets current transform to a identity matrix.
            void resetTransform();

            // push current transform onto stack
            void pushTransform();

            // pop transform from stack
            void popTransform();
           
            // Multiply current coordinate system (from left side) by specified matrix.
            void transform(float a, float b, float c, float d, float e, float f);

            // Translates current coordinate system.
            void translate(float x, float y);
            void translatei(int x, int y);

            // Offset - translate without taking into account anything else
            void offset(float x, float y);
            void offseti(int x, int y);

            // Rotates current coordinate system. Angle is specified in radians.
            void rotate(float angle);

            // Skews the current coordinate system along X axis. Angle is specified in radians.
            void skewX(float angle);

            // Skews the current coordinate system along Y axis. Angle is specified in radians.
            void skewY(float angle);

            // Scales the current coordinate system.
            void scale(float x, float y);

            //---

			// Set pivot point for style pattern application, default is 0,0
			void stylePivot(float x, float y);

			// Set pivot point for style pattern application, default is 0,0
			void stylePivot(Vector2 offset);

			// Reset style pivot (and the whole stack) back to 0,0
			void resetStylePivot();

			// Push current style pivot
			void pushStylePivot();

			// Pop style pivot
			void popStylePivot();

			//---

            //
            // Paths
            //
            // Drawing a new shape starts with beginPath(), it clears all the currently defined paths.
            // Then you define one or more paths and sub-paths which describe the shape. The are functions
            // to draw common shapes like rectangles and circles, and lower level step-by-step functions,
            // which allow to define a path curve by curve.
            //
            // Rendering uses even-odd fill rule to draw the shapes. Solid shapes should have counter clockwise
            // winding and holes should have counter clockwise order. To specify winding of a path you can
            // call pathWinding(). This is useful especially for the common shapes, which are drawn CCW.
            //
            // Finally you can fill the path using current fill style by calling fill(), and stroke it
            // with current stroke style by calling stroke().
            //
            // The curve segments and sub-paths are transformed by the current transform.

            // Clears the current path and sub-paths.
            void beginPath();

            // Starts new sub-path with specified point as first point.
            void moveToi(int x, int y);
            void moveTo(float x, float y);
            void moveTo(const Vector2& pos);

            // Adds line segment from the last point in the path to the specified point.
            void lineToi(int x, int y);
            void lineTo(float x, float y);
            void lineTo(const Vector2& pos);

            // Adds cubic Bezier segment from last point in the path via two control points to the specified point.
            void bezierToi(int c1x, int c1y, int c2x, int c2y, int x, int y);
            void bezierTo(float c1x, float c1y, float c2x, float c2y, float x, float y);
            void bezierTo(const Vector2& c1, const Vector2& c2, const Vector2& pos);

            // Adds quadratic Bezier segment from last point in the path via a control point to the specified point.
            void quadTo(float cx, float cy, float x, float y);
            void quadTo(const Vector2& c, const Vector2& pos);

            // Adds an arc segment at the corner defined by the last path point, and two specified points.
            void arcTo(float x1, float y1, float x2, float y2, float radius);
            void arcTo(const Vector2& p1, const Vector2& p2, float radius);

            // Creates new circle arc shaped sub-path. The arc center is at cx,cy, the arc radius is r,
            // and the arc is drawn from angle a0 to a1, and swept in direction dir (NVG_CCW, or NVG_CW).
            // Angles are specified in radians.
            void arci(int cx, int cy, int r, float a0, float a1, Winding dir);
            void arc(float cx, float cy, float r, float a0, float a1, Winding dir);
            void arc(const Vector2& center, float r, float a0, float a1, Winding dir);

            // Creates new rectangle shaped sub-path.
            void recti(int x, int y, int w, int h);
            void rect(float x, float y, float w, float h);
            void rect(const Vector2& start, const Vector2& end);
            void rect(const Rect& r);

            // Creates new rounded rectangle shaped sub-path.
            void roundedRecti(int x, const int  y, const int  w, const int  h, int r);
            void roundedRect(float x, float y, float w, float h, float r);
            void roundedRect(const Vector2& start, const Vector2& end, float r);
            void roundedRect(const Rect& rect, float r);

            // Creates new rounded rectangle shaped sub-path with varying radii for each corner.
            void roundedRectVarying(float x, float y, float w, float h, float radTopLeft, float radTopRight, float radBottomRight, float radBottomLeft);

            // Creates new ellipse shaped sub-path.
            void ellipsei(int cx, int cy, int rx, int ry);
            void ellipse(float cx, float cy, float rx, float ry);
            void ellipse(const Vector2& center, float rx, float ry);
            void ellipse(const Vector2& tl, const Vector2& br);
            void ellipse(const Rect& rect);

            // Creates new circle shaped sub-path.
            void circlei(int cx, int cy, int r);
            void circle(float cx, float cy, float r);
            void circle(const Vector2& center, float r);

            // set point color
            void color(Color color);

            // Closes current sub-path with a line segment.
            void closePath();

            // Sets the current sub-path winding, see NVGwinding and NVGsolidity.
            void pathWinding(Winding dir);

			//--

            // Emit filled geometry
            void fill();

            // Emit stroke
            void stroke();

			//---

            // insert text from glyph buffer
            // NOTE: text is transformed by the current transform
            // NOTE: font should NOT be unloaded till the data is submitted for rendering
            void print(const font::GlyphBuffer& glyphs);

            // insert text from raw glyph table
            // NOTE: text is transformed by the current transform
            // NOTE: font should NOT be unloaded till the data is submitted for rendering
            void print(const void* glyphEntries, uint32_t numGlyphs, uint32_t dataStride);

            // print from font
            void print(const font::Font* font, int fontSize, StringView txt, int hcenter=-1, int vcenter=-1, bool bold = false);

            //---

			/// select custom renderer
			void selectRenderer(uint8_t index, const void* data = nullptr, uint32_t dataSize = 0);

			/// select custom renderer
			template< typename T >
			INLINE void selectRenderer(uint8_t index, const T& data)
			{
				static_assert(!std::is_pointer<T>::value, "Don't use pointer here");
				selectRenderer(index, &data, sizeof(data));
			}

			/// select custom renderer
			template< typename R >
			INLINE void selectRenderer()
			{
				static const auto index = rendering::canvas::GetHandlerIndex<R>();
				selectRenderer(index);
			}

			/// select custom renderer with data
			template< typename R, typename... Args >
			INLINE void selectRenderer(Args&& ... args)
			{
				const typename R::PrivateData data(std::forward< Args >(args)...);
				static const auto index = rendering::canvas::GetHandlerIndex<R>();
				selectRenderer(index, &data, sizeof(data));
			}

			//--

			/// push current renderer on stack
			void pushRenderer();

			/// pop previously pushed custom renderer and return to the previous one (eventually return to the default one)
			void popRenderer();

			//--

        private:
            struct RenderState
            {				
                RenderStyle fillStyle;
                RenderStyle strokeStyle;

				int cachedFillStyleIndex = -1;
				int cachedStrokeStyleIndex = -1;

				const ImageAtlasEntryInfo* cachedFillImage = nullptr;

                BlendOp op = BlendOp::AlphaPremultiplied;
                LineJoin lineJoint = LineJoin::Miter;
                LineCap lineCap = LineCap::Butt;
                float strokeWidth = 1.0f;
                float miterLimit = 1.8f;
                float alpha = 1.0f;

				bool antiAlias = false;
				float antiAliasFringeWidth = 0.5f;

                RenderState();
            };

			struct CustomRenderInfo
			{
				uint8_t index = 0; // default
				uint8_t dataSize = 0;
				uint16_t dataOffset = 0;
			};

			CustomRenderInfo m_customRenderer;

			bool m_transformInvertedValid = false;
            float m_distTollerance;
            float m_distTolleranceSquared;
            float m_tessTollerance;

            XForm2D m_transform;
            XForm2D m_transformInverted;
            XForm2DClass m_transformClass;

			Vector2 m_stylePivot;

            RenderState m_style;

            prv::PathCache* m_pathCache;

            static const uint32_t CMD_MOVETO = 0;
            static const uint32_t CMD_LINETO = 1;
            static const uint32_t CMD_BEZIERTO = 2;
            static const uint32_t CMD_CLOSE = 3;
            static const uint32_t CMD_WINDING = 4;

            typedef InplaceArray<float, 256> TCommands;
            TCommands m_commands;

            Vector2 m_prevPosition;

			Array<Vertex>& m_outVertices;
			Array<Batch>& m_outBatches;
			Array<Attributes>& m_outAttributes;
			Array<uint8_t>& m_outRendererData;

			Vector2& m_outVertexBoundsMin;
			Vector2& m_outVertexBoundsMax;

			HashMap<Attributes, int> m_attributesMap;

            //--

            static const uint32_t NUM_STACK = 4;
            InplaceArray<XForm2D, NUM_STACK> m_transformStack;
            InplaceArray<RenderState, NUM_STACK> m_styleStack;
			InplaceArray<Vector2, NUM_STACK> m_stylePivotStack;
			InplaceArray<CustomRenderInfo, NUM_STACK> m_customRendererStack;

            //--

            //uint32_t cacheFillStyle();
            //uint32_t cacheStrokeStyle();

            //void beginGroup(uint32_t styleIndex, ::Type type);
            void appendCommands(const float* vals, uint32_t numVals);
            void flattenPath(prv::PathCache& cache);
            void cacheInvertexTransform();

			int mapStyle(const RenderStyle& style, float width);

            void applyPaintColor(uint32_t firstVertex, uint32_t numVertices, Color color);
			void applyPaintAttributes(uint32_t firstVertex, uint32_t numVertices, int attributesIndex);
			void applyPaintUV(uint32_t firstVertex, uint32_t numVertices, const RenderStyle& style, const ImageAtlasEntryInfo* image);
        };

		//--

    } // canvas
} // base