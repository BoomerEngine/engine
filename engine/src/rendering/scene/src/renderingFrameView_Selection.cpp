/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\view  #]
***/

#include "build.h"
#include "renderingScene.h"
#include "renderingSceneUtils.h"
#include "renderingSceneFragment.h"
#include "renderingFrameRenderer.h"
#include "renderingFrameCameraContext.h"
#include "renderingFrameSurfaceCache.h"
#include "renderingFrameParams.h"
#include "renderingFrameView.h"
#include "renderingSceneCulling.h"
#include "renderingSceneProxy.h"
#include "renderingSceneFragmentList.h"
#include "renderingFrameView_Selection.h"

namespace rendering
{
    namespace scene
    {

        //---

        FrameView_Selection::FrameView_Selection(const FrameRenderer& frame, const Camera& camera, ImageView depthTarget, const base::Rect& captureRect, const DownloadBufferPtr& captureData)
            : FrameView(frame, FrameViewType::SelectionRect, frame.frame().resolution.width, frame.frame().resolution.height)
            , m_camera(camera)
            , m_depthTarget(depthTarget)
            , m_captureRect(captureRect)
            , m_captureData(captureData)
        {}


        struct SelectionCaptureParams
        {
            struct Data
            {
                uint32_t SelectionAreaMinX;
                uint32_t SelectionAreaMinY;
                uint32_t SelectionAreaMaxX;
                uint32_t SelectionAreaMaxY;
                uint32_t MaxSelectables;
            };

            ConstantsView constants;
            BufferView collectedSelectables;
        };

        void FrameView_Selection::render(command::CommandWriter& parentCmd)
        {
            PC_SCOPE_LVL1(SelectionView);

            // collect fragments from the main camera
            collectSingleCamera(m_camera);

            // prepare fragments for the visible objects
            {
                command::CommandWriter cmd(parentCmd.opCreateChildCommandBuffer(), "Fragments");
                generateFragments(cmd);
            }

            // trigger capture
            parentCmd.opTriggerCapture();

            //--

            // render passes
            {
                command::CommandWriter cmd(parentCmd.opCreateChildCommandBuffer(), "SelectionView");
                base::ScopeTimer timer;

                // bind global settings
                BindSingleCamera(cmd, m_camera);

                // depth pre pass for the scene, as normal
                const auto& veloicityBuffer = renderer().surfaces().m_velocityBufferRT;
                RenderDepthPrepass(cmd, *this, m_depthTarget, veloicityBuffer);

                // capture selection data
                {
                    // determine maximum selectables we are interested in capturing
                    const auto maxLocalSelectables = m_captureRect.width() * m_captureRect.height() * 2;
                    const auto maxSelectables = std::min<uint32_t>(renderer().surfaces().m_maxSelectables, maxLocalSelectables);
                    auto& selectablesBuffer = renderer().surfaces().m_selectables;
                    auto selectablesBufferView = renderer().surfaces().m_selectables.createSubViewAtOffset(0, sizeof(EncodedSelectable) * maxSelectables);

                    // clear the buffer
                    uint32_t zero = 0;
                    cmd.opClearBuffer(selectablesBufferView, &zero, sizeof(zero));


                    // bind the selection data
                    {
                        SelectionCaptureParams::Data data;
                        data.MaxSelectables = maxSelectables;
                        data.SelectionAreaMinX = m_captureRect.min.x;
                        data.SelectionAreaMinY = m_captureRect.min.y;
                        data.SelectionAreaMaxX = m_captureRect.max.x;
                        data.SelectionAreaMaxY = m_captureRect.max.y;

                        SelectionCaptureParams params;
                        params.constants = cmd.opUploadConstants(data);
                        params.collectedSelectables = selectablesBuffer;
                        cmd.opBindParametersInline("SelectionBufferCollectionParams"_id, params);
                    }

                    // render selection capture fragments
                    RenderSelectionFragments(cmd, *this, m_depthTarget);

                    // copy data
                    cmd.opDownloadBuffer(selectablesBufferView, m_captureData);
                }

                // stats
                m_stats.recordTime += timer.timeElapsed();
            }
        }

        //--

    } // scene
} // rendering

