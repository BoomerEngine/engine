/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame #]
***/

#include "build.h"
#include "renderingFrameAnalyzer.h"

namespace rendering
{
    namespace scene
    {

        IFrameAnalyzer::~IFrameAnalyzer()
        {}

        void IFrameAnalyzer::handleRegionBegin(const char* name, RegionType type)
        {}

        void IFrameAnalyzer::handleRegionEnd(RegionType type)
        {}

        void IFrameAnalyzer::handleCommand(const command::OpBase* op)
        {}

    } // scene
} // rendering
