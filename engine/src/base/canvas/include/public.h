/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base_canvas_glue.inl"

namespace base
{
    namespace canvas
    {
        /// polygon winding
        enum class Winding : uint8_t
        {
            CCW = 1, // Winding for solid shapes
            CW = 2, // Winding for holes
        };

        struct RenderStyle;
        struct RenderGroup;

        class Geometry;
        typedef RefPtr<Geometry> GeometryPtr;

        class GeometryBuilder;
        class Canvas;

        class GlyphCache;

        //--

        // canvas zoom/offset helper
        class BASE_CANVAS_API CanvasVirtualZoomOffset
        {
            float minZoom = 0.1f;
            float maxZoom = 10.0f;
            float zoomStep = 1.1f;

            float ox = 0.0f;
            float oy = 0.0f;
            float sx = 1.0f;
            float sy = 1.0f;
            float s = 1.0f; // sqrt(sx*sx + sy*sy)

            // convert from canvas physical rendering coordinates to virtual coordinates using the zoom/offset
            Vector2 canvasToVirtual(Vector2 pos) const;
            //Rect canvasToVirtual(Rect pos) const;

            // convert from canvas physical rendering coordinates to virtual coordinates using the zoom/offset
            Vector2 virtualToCanvas(Vector2 pos) const;
            //Rect virtualToCanvas(Rect pos) const;

            // zoom/offset the surface
            void zoomIn(float cx, float cy);
            void zoomOut(float cx, float cy);
            void deltaZoom(float cx, float cy, float dx, float dy);
            void offset(float dx, float dy);
        };

    } // canvas
} // base