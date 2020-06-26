/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame #]
***/

#pragma once

#include "rendering/driver/include/renderingCommands.h"

namespace rendering
{
    namespace scene
    {

        /// type of region
        enum class RegionType : uint8_t
        {
            FrameNode,
            CommandBuffer,
            Pass,
            User,
        };

        /// analyzer of the command buffer data
        class RENDERING_SCENE_API IFrameAnalyzer : public base::NoCopy
        {
        public:
            virtual ~IFrameAnalyzer();

            /// enter region
            virtual void handleRegionBegin(const char* name, RegionType type);

            /// leaver region
            virtual void handleRegionEnd(RegionType type);

            /// general command handler
            virtual void handleCommand(const command::OpBase* op);
        };

    } // scene
} // rendering