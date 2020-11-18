/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#pragma once

#include "base/canvas/include/canvas.h"

namespace rendering
{
    namespace canvas
    {

        ///---

        /// custom drawing payload
        struct CanvasCustomBatchPayload
        {
            uint32_t firstIndex = 0;
            uint32_t numIndices = 0;
            const void* data = nullptr;
        };

        /// rendering handler for custom canvas batches
        class RENDERING_CANVAS_API ICanvasRendererCustomBatchHandler : public base::IReferencable
        {
            RTTI_DECLARE_VIRTUAL_ROOT_CLASS(ICanvasRendererCustomBatchHandler);

        public:
            virtual ~ICanvasRendererCustomBatchHandler();

            /// initialize handler, may allocate resources, load shaders, etc
            virtual void initialize(rendering::IDevice* drv);

            /// deinitialize handler, should free resources
            virtual void deinitialize(rendering::IDevice* drv);

            /// handle rendering of batches
            virtual void render(command::CommandWriter& cmd, const base::canvas::Canvas& canvas, const rendering::canvas::CanvasRenderingParams& params, uint32_t firstIndex, uint32_t numIndices, uint32_t numPayloads, const rendering::canvas::CanvasCustomBatchPayload* payloads) = 0;
        };

        //---

       /// get the static handler ID
        template< typename T >
        static uint16_t GetHandlerIndex()
        {
            static const auto id = T::GetStaticClass()->userIndex();
            DEBUG_CHECK_EX(id != INDEX_NONE, "Handled class not properly registered");
            return (uint16_t)id;
        }

        //--

    } // canvas
} // rendering