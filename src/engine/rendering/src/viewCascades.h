/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\view #]
***/

#pragma once

#include "view.h"
#include "gpu/device/include/commandWriter.h"

BEGIN_BOOMER_NAMESPACE_EX(rendering)

//--

/// command buffers to write to when recording shadow cascades
struct ENGINE_RENDERING_API FrameViewCascadesRecorder : public FrameViewRecorder
{
    struct CascadeSlice
    {
        gpu::CommandWriter solid;
        gpu::CommandWriter masked;

        CascadeSlice();
    };

    CascadeSlice slices[MAX_SHADOW_CASCADES];

    FrameViewCascadesRecorder(FrameViewRecorder* parentView);
};

//--

/// view for rendering shadow cascades
class ENGINE_RENDERING_API FrameViewCascades : public NoCopy
{
public:
    FrameViewCascades(const FrameRenderer& frame, const CascadeData& cascades);
    ~FrameViewCascades();

    void render(gpu::CommandWriter& cmd, FrameViewRecorder* parentView);

    //--

    INLINE const CascadeInfo& cascade(int index) const { return m_cascades.cascades[index]; }

    //--

private:
    const FrameRenderer& m_frame;
    const CascadeData& m_cascades;

    //--

    void initializeCommandStreams(gpu::CommandWriter& cmd, FrameViewCascadesRecorder& rec);

    //--
};

//--

END_BOOMER_NAMESPACE_EX(rendering)
