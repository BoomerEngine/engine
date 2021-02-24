/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\view #]
***/

#include "build.h"
#include "renderingFrameRenderer.h"
#include "renderingFrameView.h"

#include "rendering/device/include/renderingCommandWriter.h"

BEGIN_BOOMER_NAMESPACE(rendering::scene)

//--

/// command buffers to write to when recording selection fragments
struct FrameViewCaptureSelectionRecorder : public FrameViewRecorder
{
    GPUCommandWriter viewBegin; // run at the start of the view rendering
    GPUCommandWriter viewEnd; // run at the end of the view rendering

    GPUCommandWriter depthPrePass; // depth pre pass, used to filter foreground fragments
    GPUCommandWriter mainFragments; // selection emitting fragments

    FrameViewCaptureSelectionRecorder();
};

//--

/// helper recorder class
class RENDERING_SCENE_API FrameViewCaptureSelection : public FrameViewSingleCamera
{
public:
    struct Setup
    {
        Camera camera;
        base::Rect viewport;

        base::Rect captureRegion;
        DownloadDataSinkPtr captureSink;
    };

    //--

    FrameViewCaptureSelection(const FrameRenderer& frame, const Setup& setup);

    void render(GPUCommandWriter& cmd);

    //--

private:
    const Setup& m_setup;

    void initializeCommandStreams(GPUCommandWriter& cmd, FrameViewCaptureSelectionRecorder& rec);
};

//--

END_BOOMER_NAMESPACE(rendering::scene)