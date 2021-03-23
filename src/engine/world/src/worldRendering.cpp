/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world #]
*
***/

#include "build.h"
#include "world.h"
#include "worldRendering.h"
#include "worldViewEntity.h"

#include "engine/rendering/include/scene.h"
#include "engine/rendering/include/params.h"
#include "engine/rendering/include/service.h"

#include "gpu/device/include/commandWriter.h"
#include "gpu/device/include/image.h"
#include "gpu/device/include/deviceService.h"
#include "gpu/device/include/device.h"

BEGIN_BOOMER_NAMESPACE()

//---

RTTI_BEGIN_TYPE_CLASS(WorldRenderingSystem);
RTTI_END_TYPE();

WorldRenderingSystem::WorldRenderingSystem()
{}

WorldRenderingSystem::~WorldRenderingSystem()
{}

bool WorldRenderingSystem::handleInitialize(World& scene)
{
    switch (scene.type())
    {
    case WorldType::SimplePreview:
        m_scene = RefNew<rendering::RenderingScene>(rendering::RenderingSceneType::EditorPreview);
        break;

    case WorldType::EditorPreview:
        m_scene = RefNew<rendering::RenderingScene>(rendering::RenderingSceneType::EditorGame);
        break;

    default:
        m_scene = RefNew<rendering::RenderingScene>(rendering::RenderingSceneType::Game);
        break;
    }

    return true;
}

void WorldRenderingSystem::handleShutdown()
{
    m_scene.reset();
}

void WorldRenderingSystem::handleImGuiDebugInterface()
{
    RenderStatsGui(m_lastStats);
}

void WorldRenderingSystem::renderViewport(World* world, gpu::CommandWriter& cmd, const gpu::AcquiredOutput& output, const WorldRenderingContext& context) const
{
    DEBUG_CHECK_RETURN_EX(output.color, "Missing render target");
    DEBUG_CHECK_RETURN_EX(output.depth, "Missing render target");
    DEBUG_CHECK_RETURN_EX(output.width && output.height, "Invalid resolution");

    //--

    CameraSetup cameraSetup = context.cameraSetup;
    cameraSetup.aspect = output.width / (float)output.height;
    m_lastCamera = cameraSetup;

    Camera camera;
    camera.setup(cameraSetup);

    //--

    rendering::FrameParams frame(output.width, output.height, camera);
    frame.resolution.width = output.width;
    frame.resolution.height = output.height;
    frame.camera.cameraContext = context.cameraContext;
    
    world->renderDebugFragments(frame);

    if (context.callback)
        context.callback(frame);

    //--

    rendering::FrameStats stats;

    // TODO: implement flipping in PP stack
    if (output.color->flipped())
    {
        // HACK
        static gpu::ImageObjectPtr GFlippedColor, GFlippedDepth;
        static gpu::RenderTargetViewPtr GFlippedColorRTV, GFlippedDepthRTV;

        if (GFlippedColor && (GFlippedColor->width() != output.color->width() || GFlippedColor->height() != output.color->height()))
        {
            GFlippedColor.reset();
            GFlippedDepth.reset();
        }

        if (!GFlippedColor)
        {
            gpu::ImageCreationInfo info;
            info.allowRenderTarget = true;
            info.width = output.color->width();
            info.height = output.color->height();
            info.format = output.color->format();
            info.label = "FlippedColorOutput";
            GFlippedColor = GetService<DeviceService>()->device()->createImage(info);
            GFlippedColorRTV = GFlippedColor->createRenderTargetView();

            info.format = output.depth->format();
            GFlippedDepth = GetService<DeviceService>()->device()->createImage(info);
            GFlippedDepthRTV = GFlippedDepth->createRenderTargetView();
        }

        gpu::AcquiredOutput flippedOutput;
        flippedOutput.color = GFlippedColorRTV;
        flippedOutput.depth = GFlippedDepthRTV;
        flippedOutput.width = output.width;
        flippedOutput.height = output.height;

        if (auto sceneCmd = GetService<rendering::FrameRenderingService>()->render(frame, flippedOutput, m_scene, stats))
        {
            cmd.opAttachChildCommandBuffer(sceneCmd);
            cmd.opCopyRenderTarget(GFlippedColorRTV, output.color, 0, 0, true);
        }
    }
    else
    {
        if (auto sceneCmd = GetService<rendering::FrameRenderingService>()->render(frame, output, m_scene, stats))
            cmd.opAttachChildCommandBuffer(sceneCmd);
    }

    m_lastStats = stats;
}

//---

END_BOOMER_NAMESPACE()
