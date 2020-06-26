/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "rendering_canvas_glue.inl"

namespace rendering
{
    namespace canvas
    {

        //----

        class CanvasRenderer;
        class CanvasRenderingService;

        struct CanvasRenderingParams
        {
            // Width and height of frame buffer we are rendering to (if rendering in pixel perfect mode)
            // If not specified we assume they are equal to the canvas size
            uint32_t frameBufferWidth = 0;
            uint32_t frameBufferHeight = 0;

            // Custom viewport in the target frame buffer to render the canvas to
            // NOTE: we do not scale, just offset + scissor the content
            const base::Rect* customViewport = nullptr;
            
            // Full matrix to transform from canvas vertices [0,W]x[0,H] into screen space
            const base::Matrix* canvasToScreen = nullptr;
        };
        
        //----

    } // runtime
} // rendering

