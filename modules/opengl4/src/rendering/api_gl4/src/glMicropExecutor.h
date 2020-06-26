/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: driver\execution #]
***/

#pragma once

#include "rendering/driver/include/renderingDriver.h"

namespace rendering
{
    namespace gl4
    {
        class DriverFrame;
        class DriverThread;

        /// execute the command command buffer by building a Vulkan command buffer
        /// preforms all the necessary synchronizations and data transfers, the generated command buffer is ready to use
        /// all the commands are executed on a single queue and fill a single command buffer
        extern void ExecuteCommands(Driver* drv, DriverThread* thread, DriverFrame* seq, DriverPerformanceStats* stats, command::CommandBuffer* masterCommandBuffer);

    } // gl4
} // driver