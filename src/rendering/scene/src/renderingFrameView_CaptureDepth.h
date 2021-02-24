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

#include "rendering/device/include/renderingCommandWriter.h"

BEGIN_BOOMER_NAMESPACE(rendering::scene)

//--

/// command buffers to write to when recording depth buffer for capture
struct FrameViewCaptureDepthRecorder : public FrameViewRecorder
{
    GPUCommandWriter viewBegin; // run at the start of the view rendering
    GPUCommandWriter viewEnd; // run at the end of the view rendering

    GPUCommandWriter depth; // depth pre pass, used to filter foreground fragments

    FrameViewCaptureDepthRecorder();
};

//--

/// helper recorder class
class RENDERING_SCENE_API FrameViewCaptureDepth : public FrameViewSingleCamera
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

    FrameViewCaptureDepth(const FrameRenderer& frame, const Setup& setup);

    void render(GPUCommandWriter& cmd);

    //--

private:
    const Setup& m_setup;

    void initializeCommandStreams(GPUCommandWriter& cmd, FrameViewCaptureDepthRecorder& rec);
};

//--

END_BOOMER_NAMESPACE(rendering::scene)