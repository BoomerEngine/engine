/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\view  #]
***/

#include "build.h"
#include "scene.h"
#include "renderer.h"
#include "cameraContext.h"
#include "params.h"
#include "viewCaptureDepth.h"
#include "resources.h"
#include "view.h"

#include "gpu/device/include/descriptor.h"
#include "gpu/device/include/image.h"

BEGIN_BOOMER_NAMESPACE_EX(rendering)

//----

FrameViewCaptureDepthRecorder::FrameViewCaptureDepthRecorder()
    : FrameViewRecorder(nullptr)
    , viewBegin(nullptr)
    , viewEnd(nullptr)
    , depth(nullptr)
{}

//---

FrameViewCaptureDepth::FrameViewCaptureDepth(const FrameRenderer& frame, const Setup& setup)
    : FrameViewSingleCamera(frame, setup.camera, setup.viewport)
    , m_setup(setup)
{
}

void FrameViewCaptureDepth::render(gpu::CommandWriter& cmd)
{
    // initialize the scaffolding of the view
    FrameViewCaptureDepthRecorder rec;
    initializeCommandStreams(cmd, rec);

    // start cascade rendering

    // render scene into the pre-created command buffers
    if (auto* scene = m_frame.frame().scenes.mainScenePtr)
        scene->renderCaptureDepthView(rec, *this, m_frame);

    // TODO: render background

    // render debug fragments - they can also be selected
    if (m_frame.frame().filters & FilterBit::DebugGeometry)
    {
        /*DebugGeometryViewRecorder debugRec(&rec);
        debugRec.solid.attachBuffer(rec.mainSolid.opCreateChildCommandBuffer(false));
        m_frame.helpers().debug->render(debugRec, m_frame.frame().geometry, &m_camera);*/
    }

    // wait for all recording jobs to finish
    rec.finishRendering();
}

void FrameViewCaptureDepth::initializeCommandStreams(gpu::CommandWriter& cmd, FrameViewCaptureDepthRecorder& rec)
{
    // trigger capture
    cmd.opTriggerCapture();

    // view start 
    rec.viewBegin.attachBuffer(cmd.opCreateChildCommandBuffer());

    // bind camera params
    bindCamera(cmd);

    // depth pre-pass
    {
        gpu::CommandWriterBlock block(cmd, "DepthFill");

        gpu::FrameBuffer fb;
        fb.depth.view(m_frame.resources().sceneFullDepthRTV).clearDepth(1.0f).clearStencil(0);
        fb.depth.loadOp = gpu::LoadOp::Clear;
        fb.depth.storeOp = gpu::StoreOp::Store;

        cmd.opBeingPass(fb, 1, m_viewport);

        rec.depth.attachBuffer(cmd.opCreateChildCommandBuffer());

        cmd.opEndPass();
    }

    // download depth buffer
    {
        gpu::ResourceCopyRange range;
        range.image.mip = 0;
        range.image.slice = 0;
        range.image.offsetX = m_setup.captureRegion.min.x;
        range.image.offsetY = m_setup.captureRegion.min.y;
        range.image.offsetZ = 0;
        range.image.sizeX = m_setup.captureRegion.width();
        range.image.sizeY = m_setup.captureRegion.height();
        range.image.sizeZ = 1;

        cmd.opTransitionLayout(m_frame.resources().sceneFullDepth, gpu::ResourceLayout::DepthWrite, gpu::ResourceLayout::CopySource);
        cmd.opDownloadData(m_frame.resources().sceneFullDepth, range, m_setup.captureSink);
        cmd.opTransitionLayout(m_frame.resources().sceneFullDepth, gpu::ResourceLayout::CopySource, gpu::ResourceLayout::DepthWrite);
    }

    // view end
    rec.viewEnd.attachBuffer(cmd.opCreateChildCommandBuffer());
}

//--

END_BOOMER_NAMESPACE_EX(rendering)
