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

#include "core/containers/include/stringBuilder.h"
#include "gpu/device/include/renderingCommandWriter.h"
#include "gpu/device/include/renderingCommandBuffer.h"
#include "gpu/device/include/renderingDeviceApi.h"
#include "engine/mesh/include/renderingMeshService.h"
#include "engine/material/include/renderingMaterialRuntimeService.h"
#include "gpu/device/include/renderingDescriptor.h"
#include "gpu/device/include/renderingImage.h"
#include "renderingFrameView.h"
#include "renderingFrameHelper_Outline.h"

BEGIN_BOOMER_NAMESPACE_EX(rendering)

//--

FrameViewRecorder::FrameViewRecorder(FrameViewRecorder* parentView)
    : m_parentView(parentView)
{}

void FrameViewRecorder::finishRendering()
{
    auto lock = CreateLock(m_fenceListLock);
    Fibers::GetInstance().waitForMultipleCountersAndRelease(m_fences.typedData(), m_fences.size());
}

void FrameViewRecorder::postFence(fibers::WaitCounter fence, bool localFence)
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

FrameHelper::FrameHelper(gpu::IDevice* dev)
{
	debug = new FrameHelperDebug(dev);
	compose = new FrameHelperCompose(dev);
    outline = new FrameHelperOutline(dev);
}

FrameHelper::~FrameHelper()
{
	delete debug;
	delete compose;
    delete outline;
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

void FrameRenderer::bindFrameParameters(gpu::CommandWriter& cmd) const
{
    GPUFrameParameters params;
    PackFrameParams(params, *this, m_target);

    gpu::DescriptorEntry desc[1];
    desc[0].constants(params);
    cmd.opBindDescriptor("FrameParams"_id, desc);
}

void FrameRenderer::prepareFrame(gpu::CommandWriter& cmd)
{
    gpu::CommandWriterBlock block(cmd, "PrepareFrame");

    // prepare some major services
    GetService<MeshService>()->uploadChanges(cmd);

    // dispatch all material changes
    GetService<MaterialService>()->dispatchMaterialProxyChanges();

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

END_BOOMER_NAMESPACE_EX(rendering)
