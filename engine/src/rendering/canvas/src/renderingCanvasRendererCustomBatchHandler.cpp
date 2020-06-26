/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#include "build.h"
#include "renderingCanvasRenderer.h"
#include "renderingCanvasRendererCustomBatchHandler.h"

namespace rendering
{
    namespace canvas
    {
        //---

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ICanvasRendererCustomBatchHandler);
        RTTI_END_TYPE();

        ICanvasRendererCustomBatchHandler::~ICanvasRendererCustomBatchHandler()
        {}

        void ICanvasRendererCustomBatchHandler::initialize(IDriver* drv)
        {}

        void ICanvasRendererCustomBatchHandler::deinitialize(IDriver* drv)
        {}

        //---

    } // canvas
} // rendering