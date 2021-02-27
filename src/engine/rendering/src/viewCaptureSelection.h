/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\view #]
***/

#include "build.h"
#include "renderer.h"
#include "view.h"

#include "gpu/device/include/commandWriter.h"

BEGIN_BOOMER_NAMESPACE_EX(rendering)

//--

/// command buffers to write to when recording selection fragments
struct FrameViewCaptureSelectionRecorder : public FrameViewRecorder
{
    gpu::CommandWriter viewBegin; // run at the start of the view rendering
    gpu::CommandWriter viewEnd; // run at the end of the view rendering

    gpu::CommandWriter depthPrePass; // depth pre pass, used to filter foreground fragments
    gpu::CommandWriter mainFragments; // selection emitting fragments

    FrameViewCaptureSelectionRecorder();
};

//--

/// helper recorder class
class ENGINE_RENDERING_API FrameViewCaptureSelection : public FrameViewSingleCamera
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

    FrameViewCaptureSelection(const FrameRenderer& frame, const Setup& setup);

    void render(gpu::CommandWriter& cmd);

    //--

private:
    const Setup& m_setup;

    void initializeCommandStreams(gpu::CommandWriter& cmd, FrameViewCaptureSelectionRecorder& rec);
};

//--

END_BOOMER_NAMESPACE_EX(rendering)
