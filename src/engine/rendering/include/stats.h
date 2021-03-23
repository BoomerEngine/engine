/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(rendering)

//--

struct ENGINE_RENDERING_API FrameViewStats
{
    uint32_t numTriangles = 0;
    uint32_t numDrawCalls = 0;
    uint32_t numChunks = 0;
    uint32_t numShaders = 0;
    uint32_t numMaterials = 0;

    double recordingTime = 0.0;
    double cullingTime = 0.0;

    void merge(const FrameViewStats& stats);
    void print(StringView prefix, IFormatStream& f) const;

    FrameViewStats() = default;
    FrameViewStats(const FrameViewStats& other) = default;
    FrameViewStats& operator=(const FrameViewStats& other) = default;
};

struct ENGINE_RENDERING_API FrameStats
{
    double totalTime = 0.0;

    FrameViewStats totals;
    FrameViewStats mainView;
    FrameViewStats depthView;
    FrameViewStats globalShadowView;
    FrameViewStats localShadowView;

    FrameStats() = default;
    FrameStats(const FrameStats& other) = default;
    FrameStats& operator=(const FrameStats& other) = default;
};

//--

// render scene stats gui (ImGui)
extern ENGINE_RENDERING_API void RenderStatsGui(const FrameStats& frameStats);

//--

END_BOOMER_NAMESPACE_EX(rendering)
