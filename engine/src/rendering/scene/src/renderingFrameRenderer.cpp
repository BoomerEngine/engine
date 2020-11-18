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
#include "renderingFrameCameraContext.h"
#include "renderingFrameSurfaceCache.h"
#include "renderingFrameParams.h"
#include "renderingSceneUtils.h"
#include "renderingSceneCulling.h"
#include "renderingSceneProxy.h"
#include "renderingSceneFragmentList.h"

#include "base/containers/include/stringBuilder.h"
#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/device/include/renderingCommandBuffer.h"
#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/mesh/include/renderingMeshService.h"
#include "rendering/material/include/renderingMaterialRuntimeService.h"

namespace rendering
{
    namespace scene
    {
        //--

        FrameRenderer::FrameRenderer(const FrameParams& frame, const FrameSurfaceCache& surfaces)
            : m_frame(frame)
            , m_surfaces(surfaces)
            , m_allocator(POOL_RENDERING_FRAME)
        {
            m_msaa = false;

            const auto sceneCount = m_frame.scenes.scenesToDraw.size();
            m_scenes.reserve(sceneCount);

            // lock scenes for rendering
            for (auto& scene : m_frame.scenes.scenesToDraw)
            {
                if (scene.scenePtr->lockForRendering())
                {
                    auto& entry = m_scenes.emplaceBack();
                    entry.stats.numScenes = 1;
                    entry.scene = scene.scenePtr;
                }
            }
        }

        FrameRenderer::~FrameRenderer()
        {
            // release lock on all rendered scenes
            for (auto& scene : m_scenes)
                scene.scene->unlockAfterRendering(std::move(scene.stats));
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

        static void BindFrameParameters(command::CommandWriter& cmd, const FrameRenderer& frame)
        {
            const auto& data = frame.frame();
            const auto& view = frame.surfaces().m_sceneFullColorRT;

            GPUFrameParameters params;
            memset(&params, 0, sizeof(params));

            params.ViewportSize.x = data.resolution.width;
            params.ViewportSize.y = data.resolution.height;
            params.ViewportRTSize.x = view.width();
            params.ViewportRTSize.y = view.height();
            params.InvViewportSize.x = params.ViewportSize.x ? (1.0f / params.ViewportSize.x) : 0.0f;
            params.InvViewportSize.y = params.ViewportSize.y ? (1.0f / params.ViewportSize.y) : 0.0f;
            params.InvViewportRTSize.x = params.ViewportRTSize.x ? (1.0f / params.ViewportRTSize.x) : 0.0f;
            params.InvViewportRTSize.y = params.ViewportRTSize.y ? (1.0f / params.ViewportRTSize.y) : 0.0f;

            params.FrameIndex = data.index;
            params.MSAASamples = view.multisampled() ? view.numSamples() : 0;

            params.PseudoRandom[0] = GRenderingRandState.next();
            params.PseudoRandom[1] = GRenderingRandState.next();
            params.PseudoRandom[2] = GRenderingRandState.next();
            params.PseudoRandom[3] = GRenderingRandState.next();

            params.MaterialFlags = 0;
            if (frame.frame().filters & FilterBit::Material_DisableColorMap)
                params.MaterialFlags |= MATERIAL_FLAG_DISABLE_COLOR;
            if (frame.frame().filters & FilterBit::Material_DisableLighting)
                params.MaterialFlags |= MATERIAL_FLAG_DISABLE_LIGHTING;
            if (frame.frame().filters & FilterBit::Material_DisableTextures)
                params.MaterialFlags |= MATERIAL_FLAG_DISABLE_TEXTURES;
            if (frame.frame().filters & FilterBit::Material_DisableNormals)
                params.MaterialFlags |= MATERIAL_FLAG_DISABLE_NORMAL;
            if (frame.frame().filters & FilterBit::Material_DisableObjectColor)
                params.MaterialFlags |= MATERIAL_FLAG_DISABLE_OBJECT_COLOR;
            if (frame.frame().filters & FilterBit::Material_DisableVertexColor)
                params.MaterialFlags |= MATERIAL_FLAG_DISABLE_VERTEX_COLOR;
            if (frame.frame().filters & FilterBit::Material_DisableMasking)
                params.MaterialFlags |= MATERIAL_FLAG_DISABLE_MASKING;
            if (frame.frame().filters & FilterBit::Material_DisableVertexMotion)
                params.MaterialFlags |= MATERIAL_FLAG_DISABLE_VERTEX_MOTION;


            /*if (frame.verticalFlip())
            {
                params.ScreenTopY = view.height()
            }*/
            
            params.DebugMousePos = CovertMouseCoords(data.debug.mouseHoverPixel, frame);
            params.DebugMouseClickPos = CovertMouseCoords(data.debug.mouseClickedPixel, frame);

            params.TimeOfDay = data.time.timeOfDay;
            params.GameTime = data.time.gameTime;
            params.EngineTime = data.time.engineRealTime;
            params.DayNightFraction = data.time.dayNightFrac;

            struct
            {
                ConstantsView params;
            } desc;

            desc.params = cmd.opUploadConstants(params);
            cmd.opBindParametersInline("FrameParams"_id, desc);
        }

        void FrameRenderer::prepareFrame(command::CommandWriter& cmd)
        {
            // prepare some major services
            base::GetService<rendering::MeshService>()->prepareForFrame(cmd);

            // dispatch all material changes
            base::GetService<rendering::MaterialService>()->dispatchMaterialProxyChanges();

            // bind global frame params
            BindFrameParameters(cmd, *this);
       
            // prepare frame for rendering
            for (auto& entry : m_scenes)
            {
                entry.scene->prepareForRendering(cmd);

                // TODO: a function to prepare scene info ?

                GPUSceneInfo info;
                info.SceneToWorld = base::Matrix::IDENTITY();
                info.WorldToScene = base::Matrix::IDENTITY();

                struct
                {
                    ConstantsView params;
                } desc;

                desc.params = cmd.opUploadConstants(info);
                entry.params = cmd.opUploadParameters(desc);
            }
        }

        void FrameRenderer::finishFrame()
        {
            for (auto& scene : m_scenes)
                m_mergedSceneStats.merge(scene.stats);
        }

        //--

    } // scene
} // rendering
