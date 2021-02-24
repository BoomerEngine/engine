/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\view  #]
***/

#include "build.h"
#include "renderingScene.h"
#include "renderingFrameRenderer.h"
#include "renderingFrameCameraContext.h"
#include "renderingFrameParams.h"
#include "renderingFrameView_CaptureSelection.h"
#include "renderingFrameResources.h"
#include "renderingFrameView.h"

#include "rendering/device/include/renderingDescriptor.h"
#include "rendering/device/include/renderingBuffer.h"

BEGIN_BOOMER_NAMESPACE(rendering::scene)

//----

FrameViewCaptureSelectionRecorder::FrameViewCaptureSelectionRecorder()
    : FrameViewRecorder(nullptr)
    , viewBegin(nullptr)
    , viewEnd(nullptr)
    , depthPrePass(nullptr)
    , mainFragments(nullptr)
{}

//---

FrameViewCaptureSelection::FrameViewCaptureSelection(const FrameRenderer& frame, const Setup& setup)
    : FrameViewSingleCamera(frame, setup.camera, setup.viewport)
    , m_setup(setup)
{
}

void FrameViewCaptureSelection::render(GPUCommandWriter& cmd)
{
    // initialize the scaffolding of the view
    FrameViewCaptureSelectionRecorder rec;
    initializeCommandStreams(cmd, rec);

    // start cascade rendering

    // render scene into the pre-created command buffers
    if (auto* scene = m_frame.frame().scenes.mainScenePtr)
        scene->renderCaptureSelectionView(rec, *this, m_frame);

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

void FrameViewCaptureSelection::initializeCommandStreams(GPUCommandWriter& cmd, FrameViewCaptureSelectionRecorder& rec)
{
    // trigger capture
    cmd.opTriggerCapture();

    // view start 
    rec.viewBegin.attachBuffer(cmd.opCreateChildCommandBuffer());

    // bind camera params
    bindCamera(cmd);

    // depth pre-pass
    {
        CommandWriterBlock block(cmd, "DepthPrePass");

        FrameBuffer fb;
        fb.depth.view(m_frame.resources().sceneFullDepthRTV).clearDepth(1.0f).clearStencil(0);
        fb.depth.loadOp = LoadOp::Clear;
        fb.depth.storeOp = StoreOp::Store;

        cmd.opBeingPass(fb, 1, m_viewport);

        rec.depthPrePass.attachBuffer(cmd.opCreateChildCommandBuffer());

        cmd.opEndPass();
    }

    // determine maximum number of selectable objects we are interested in capturing
    const auto maxLocalSelectables = m_setup.captureRegion.width() * m_setup.captureRegion.height() * 2;
    const auto maxSelectables = std::min<uint32_t>(m_frame.resources().maxSelectables, maxLocalSelectables);

    // capture selection data
    {
        // clear the buffer
        const auto* bufferUAV = m_frame.resources().selectablesBufferUAV.get();
        cmd.opClearWritableBuffer(bufferUAV, 0, maxLocalSelectables);

        struct
        {
            uint32_t SelectionAreaMinX;
            uint32_t SelectionAreaMinY;
            uint32_t SelectionAreaMaxX;
            uint32_t SelectionAreaMaxY;
            uint32_t MaxSelectables;
        } params;

        params.MaxSelectables = maxSelectables;
        params.SelectionAreaMinX = m_setup.captureRegion.min.x;
        params.SelectionAreaMinY = m_setup.captureRegion.min.y;
        params.SelectionAreaMaxX = m_setup.captureRegion.max.x;
        params.SelectionAreaMaxY = m_setup.captureRegion.max.y;

        DescriptorEntry desc[2];
        desc[0].constants(params);
        desc[1] = bufferUAV;
        cmd.opBindDescriptor("SelectionBufferCollectionParams"_id, desc);
    }

    // selection fragment pass
    {
        CommandWriterBlock block(cmd, "Fragments");

        FrameBuffer fb;
        fb.depth.view(m_frame.resources().sceneFullDepthRTV);
        fb.depth.loadOp = LoadOp::Keep;
        fb.depth.storeOp = StoreOp::DontCare;

        cmd.opBeingPass(fb, 1, m_viewport);

        {
            CommandWriterBlock block(cmd, "Solid");
            rec.mainFragments.attachBuffer(cmd.opCreateChildCommandBuffer());
        }

        cmd.opEndPass();
    }

    // download selection buffer
    {
        ResourceCopyRange range;
        range.buffer.offset = 0;
        range.buffer.size = maxSelectables * sizeof(EncodedSelectable);

        cmd.opTransitionLayout(m_frame.resources().selectablesBuffer, ResourceLayout::UAV, ResourceLayout::CopySource);
        cmd.opDownloadData(m_frame.resources().selectablesBuffer, range, m_setup.captureSink);
        cmd.opTransitionLayout(m_frame.resources().selectablesBuffer, ResourceLayout::CopySource, ResourceLayout::UAV);
    }

    // view end
    rec.viewEnd.attachBuffer(cmd.opCreateChildCommandBuffer());
}

//--

END_BOOMER_NAMESPACE(rendering::scene)