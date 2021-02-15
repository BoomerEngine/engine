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
#include "renderingFrameView_CaptureDepth.h"
#include "renderingFrameResources.h"
#include "renderingFrameView.h"

#include "rendering/device/include/renderingDescriptor.h"
#include "rendering/device/include/renderingImage.h"

namespace rendering
{
    namespace scene
    {

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

        void FrameViewCaptureDepth::render(command::CommandWriter& cmd)
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

        void FrameViewCaptureDepth::initializeCommandStreams(command::CommandWriter& cmd, FrameViewCaptureDepthRecorder& rec)
        {
            // trigger capture
            cmd.opTriggerCapture();

            // view start 
            rec.viewBegin.attachBuffer(cmd.opCreateChildCommandBuffer());

            // bind camera params
            bindCamera(cmd);

            // depth pre-pass
            {
                command::CommandWriterBlock block(cmd, "DepthFill");

                FrameBuffer fb;
                fb.depth.view(m_frame.resources().sceneFullDepthRTV).clearDepth(1.0f).clearStencil(0);
                fb.depth.loadOp = LoadOp::Clear;
                fb.depth.storeOp = StoreOp::Store;

                cmd.opBeingPass(fb, 1, m_viewport);

                rec.depth.attachBuffer(cmd.opCreateChildCommandBuffer());

                cmd.opEndPass();
            }

            // download depth buffer
            {
                ResourceCopyRange range;
                range.image.mip = 0;
                range.image.slice = 0;
                range.image.offsetX = m_setup.captureRegion.min.x;
                range.image.offsetY = m_setup.captureRegion.min.y;
                range.image.offsetZ = 0;
                range.image.sizeX = m_setup.captureRegion.width();
                range.image.sizeY = m_setup.captureRegion.height();
                range.image.sizeZ = 1;

                cmd.opTransitionLayout(m_frame.resources().sceneFullDepth, ResourceLayout::DepthWrite, ResourceLayout::CopySource);
                cmd.opDownloadData(m_frame.resources().sceneFullDepth, range, m_setup.captureSink);
                cmd.opTransitionLayout(m_frame.resources().sceneFullDepth, ResourceLayout::CopySource, ResourceLayout::DepthWrite);
            }

            // view end
            rec.viewEnd.attachBuffer(cmd.opCreateChildCommandBuffer());
        }

        //--

    } // scene
} // rendering

