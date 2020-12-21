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
#include "renderingSceneUtils.h"

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

        static base::Point CovertMouseCoords(base::Point coords, const FrameRenderer& frame)
        {
            if (coords.x < 0 || coords.y < 0)
                return base::Point(-1, -1);

            auto ret = coords;

            /*if (frame.verticalFlip())
                ret.y = (frame.frame().resolution.height - 1) - ret.y;*/

            return ret;
        }

#define MATERIAL_FLAG_DISABLE_COLOR 1
#define MATERIAL_FLAG_DISABLE_LIGHTING 2
#define MATERIAL_FLAG_DISABLE_TEXTURES 4
#define MATERIAL_FLAG_DISABLE_NORMAL 8
#define MATERIAL_FLAG_DISABLE_VERTEX_COLOR 16
#define MATERIAL_FLAG_DISABLE_OBJECT_COLOR 32
#define MATERIAL_FLAG_DISABLE_VERTEX_MOTION 64
#define MATERIAL_FLAG_DISABLE_MASKING 128

        static base::FastRandState GRenderingRandState;

        void FrameRenderer::bindFrameParameters(command::CommandWriter& cmd) const
        {
            const auto& data = m_frame;

            GPUFrameParameters params;
            memset(&params, 0, sizeof(params));

            params.ViewportSize.x = data.resolution.width;
            params.ViewportSize.y = data.resolution.height;
            params.ViewportRTSize.x = m_target.targetColorRTV->width();
            params.ViewportRTSize.y = m_target.targetColorRTV->height();
            params.InvViewportSize.x = params.ViewportSize.x ? (1.0f / params.ViewportSize.x) : 0.0f;
            params.InvViewportSize.y = params.ViewportSize.y ? (1.0f / params.ViewportSize.y) : 0.0f;
            params.InvViewportRTSize.x = params.ViewportRTSize.x ? (1.0f / params.ViewportRTSize.x) : 0.0f;
            params.InvViewportRTSize.y = params.ViewportRTSize.y ? (1.0f / params.ViewportRTSize.y) : 0.0f;

            params.FrameIndex = data.index;
            params.MSAASamples = 1;// view.multisampled() ? view.numSamples() : 0;

            params.PseudoRandom[0] = GRenderingRandState.next();
            params.PseudoRandom[1] = GRenderingRandState.next();
            params.PseudoRandom[2] = GRenderingRandState.next();
            params.PseudoRandom[3] = GRenderingRandState.next();

            params.MaterialFlags = 0;
            if (m_frame.filters & FilterBit::Material_DisableColorMap)
                params.MaterialFlags |= MATERIAL_FLAG_DISABLE_COLOR;
            if (m_frame.filters & FilterBit::Material_DisableLighting)
                params.MaterialFlags |= MATERIAL_FLAG_DISABLE_LIGHTING;
            if (m_frame.filters & FilterBit::Material_DisableTextures)
                params.MaterialFlags |= MATERIAL_FLAG_DISABLE_TEXTURES;
            if (m_frame.filters & FilterBit::Material_DisableNormals)
                params.MaterialFlags |= MATERIAL_FLAG_DISABLE_NORMAL;
            if (m_frame.filters & FilterBit::Material_DisableObjectColor)
                params.MaterialFlags |= MATERIAL_FLAG_DISABLE_OBJECT_COLOR;
            if (m_frame.filters & FilterBit::Material_DisableVertexColor)
                params.MaterialFlags |= MATERIAL_FLAG_DISABLE_VERTEX_COLOR;
            if (m_frame.filters & FilterBit::Material_DisableMasking)
                params.MaterialFlags |= MATERIAL_FLAG_DISABLE_MASKING;
            if (m_frame.filters & FilterBit::Material_DisableVertexMotion)
                params.MaterialFlags |= MATERIAL_FLAG_DISABLE_VERTEX_MOTION;


            //if (frame.verticalFlip())
            //{
              //  params.ScreenTopY = view.height()
            //}
            
            params.DebugMousePos = CovertMouseCoords(data.debug.mouseHoverPixel, *this);
            params.DebugMouseClickPos = CovertMouseCoords(data.debug.mouseClickedPixel, *this);

            params.TimeOfDay = data.time.timeOfDay;
            params.GameTime = data.time.gameTime;
            params.EngineTime = data.time.engineRealTime;
            params.DayNightFraction = data.time.dayNightFrac;

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
