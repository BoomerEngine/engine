/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame #]
***/

#include "build.h"
#include "renderingScene.h"
#include "renderingFrameRenderer.h"
#include "renderingFrameRenderingService.h"
#include "renderingFrameCameraContext.h"
#include "renderingFrameParams.h"

#include "renderingFrameHelper_Debug.h"
#include "renderingFrameHelper_Compose.h"

#include "base/containers/include/stringBuilder.h"
#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/device/include/renderingCommandBuffer.h"
#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/mesh/include/renderingMeshService.h"
#include "rendering/material/include/renderingMaterialRuntimeService.h"
#include "rendering/device/include/renderingDescriptor.h"
#include "rendering/device/include/renderingImage.h"
#include "renderingFrameView.h"

namespace rendering
{
    namespace scene
    {
        //--

        FrameViewRecorder::FrameViewRecorder(FrameViewRecorder* parentView)
            : m_parentView(parentView)
        {}

        void FrameViewRecorder::finishRendering()
        {
            auto lock = CreateLock(m_fenceListLock);
            Fibers::GetInstance().waitForMultipleCountersAndRelease(m_fences.typedData(), m_fences.size());
        }

        void FrameViewRecorder::postFence(base::fibers::WaitCounter fence, bool localFence)
        {
            if (!m_parentView || localFence)
            {
                auto lock = CreateLock(m_fenceListLock);
                m_fences.pushBack(fence);
            }
            else
            {
                m_parentView->postFence(fence, false);
            }
        }


		//--

		FrameHelper::FrameHelper(IDevice* dev)
		{
			debug = new FrameHelperDebug(dev);
			compose = new FrameHelperCompose(dev);
		}

		FrameHelper::~FrameHelper()
		{
			delete debug;
			delete compose;
		}

        //--

        FrameRenderer::FrameRenderer(const FrameParams& frame, const FrameCompositionTarget& target, const FrameResources& resources, const FrameHelper& helpers)
            : m_frame(frame)
            , m_resources(resources)
			, m_helpers(helpers)
			, m_target(target)
            , m_allocator(POOL_RENDERING_FRAME)
        {
            // lock scenes
            {
                PC_SCOPE_LVL1(LockScenes);
                if (m_frame.scenes.backgroundScenePtr)
                    m_frame.scenes.backgroundScenePtr->renderLock();
                if (m_frame.scenes.mainScenePtr)
                    m_frame.scenes.mainScenePtr->renderLock();
            }
        }

        FrameRenderer::~FrameRenderer()
        {
            // unlock all scenes
            {
                PC_SCOPE_LVL1(UnlockScenes);
                if (m_frame.scenes.backgroundScenePtr)
                    m_frame.scenes.backgroundScenePtr->renderUnlock();
                if (m_frame.scenes.mainScenePtr)
                    m_frame.scenes.mainScenePtr->renderUnlock();
            }
        }

        bool FrameRenderer::usesMultisamping() const
        {
            return m_msaa;
        }        

        void FrameRenderer::bindFrameParameters(command::CommandWriter& cmd) const
        {
            GPUFrameParameters params;
            PackFrameParams(params, *this, m_target);

            DescriptorEntry desc[1];
            desc[0].constants(params);
            cmd.opBindDescriptor("FrameParams"_id, desc);
        }

        void FrameRenderer::prepareFrame(command::CommandWriter& cmd)
        {
            command::CommandWriterBlock block(cmd, "PrepareFrame");

            // prepare some major services
            base::GetService<rendering::MeshService>()->uploadChanges(cmd);

            // dispatch all material changes
            base::GetService<rendering::MaterialService>()->dispatchMaterialProxyChanges();

            // bind global frame params
            bindFrameParameters(cmd);

            /*// prepare background
            if (frame().scenes.backgroundScenePtr)
                frame().scenes.backgroundScenePtr->prepare(cmd, *this);*/

            // prepare main scene
            if (frame().scenes.mainScenePtr)
                frame().scenes.mainScenePtr->prepare(cmd, *this);
        }

        void FrameRenderer::finishFrame()
        {
            /*for (auto& scene : m_scenes)
                m_mergedSceneStats.merge(scene.stats);*/
        }

        //--

    } // scene
} // rendering
