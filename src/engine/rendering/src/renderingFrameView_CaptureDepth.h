/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\view #]
***/

#include "build.h"
#include "renderingFrameView.h"
#include "renderingFrameRenderer.h"

#include "gpu/device/include/renderingCommandWriter.h"

BEGIN_BOOMER_NAMESPACE_EX(rendering)

//--

/// command buffers to write to when recording depth buffer for capture
struct FrameViewCaptureDepthRecorder : public FrameViewRecorder
{
    gpu::CommandWriter viewBegin; // run at the start of the view rendering
    gpu::CommandWriter viewEnd; // run at the end of the view rendering

    gpu::CommandWriter depth; // depth pre pass, used to filter foreground fragments

    FrameViewCaptureDepthRecorder();
};

//--

/// helper recorder class
class ENGINE_RENDERING_API FrameViewCaptureDepth : public FrameViewSingleCamera
{
public:
    struct Setup
    {
        Camera camera;
        Rect viewport;

        Rect captureRegion;
        gpu::DownloadDataSinkPtr captureSink;
    };

    //--

    FrameViewCaptureDepth(const FrameRenderer& frame, const Setup& setup);

    void render(gpu::CommandWriter& cmd);

    //--

private:
    const Setup& m_setup;

    void initializeCommandStreams(gpu::CommandWriter& cmd, FrameViewCaptureDepthRecorder& rec);
};

//--

END_BOOMER_NAMESPACE_EX(rendering)
