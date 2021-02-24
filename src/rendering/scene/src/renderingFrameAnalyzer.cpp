/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame #]
***/

#include "build.h"
#include "renderingFrameAnalyzer.h"

BEGIN_BOOMER_NAMESPACE(rendering::scene)

IFrameAnalyzer::~IFrameAnalyzer()
{}

void IFrameAnalyzer::handleRegionBegin(const char* name, RegionType type)
{}

void IFrameAnalyzer::handleRegionEnd(RegionType type)
{}

void IFrameAnalyzer::handleCommand(const GPUBaseOpcode* op)
{}

END_BOOMER_NAMESPACE(rendering::scene)