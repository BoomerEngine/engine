/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#pragma once

#include "renderingFramePayload.h"

#include "base/canvas/include/canvas.h"
#include "base/canvas/include/canvasStyle.h"
#include "base/canvas/include/canvasGeometryBuilder.h"

namespace rendering
{
    namespace canvas
    {

        /// rendering payload for rendering canvas data
        class RENDERING_RUNTIME_API CanvasFramePayload : public runtime::IFramePayload
        {
            RTTI_DECLARE_VIRTUAL_CLASS(CanvasFramePayload, runtime::IFramePayload);

        public:
            CanvasFramePayload(uint32_t width, uint32_t height, float pixelScale=1.0f);
            virtual ~CanvasFramePayload();

            /// get the canvas area
            INLINE uint32_t width() const { return m_width; }

            /// get the canvas area
            INLINE uint32_t height() const { return m_height; }

            /// get the pixel scale (DPI)
            INLINE float pixelScale() const { return m_pixelScale; }

            /// get raw canvas collector, we can draw more stuff to it
            INLINE base::canvas::Canvas& canvasCollector() { return *m_collector; }
            INLINE const base::canvas::Canvas& canvasCollector() const { return *m_collector; }

        private:
            uint32_t m_width;
            uint32_t m_height;
            float m_pixelScale;
            base::canvas::Canvas* m_collector;
        };

        ///---

    } // runtime
} // rendering