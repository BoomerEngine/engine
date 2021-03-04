/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame #]
***/

#include "build.h"
#include "cameraContext.h"
#include "scene.h"
#include "renderer.h"
#include "service.h"
#include "params.h"
#include "view.h"

#include "helperDebug.h"
#include "helperCompose.h"
#include "helperOutline.h"

#include "core/containers/include/stringBuilder.h"
#include "gpu/device/include/commandWriter.h"
#include "gpu/device/include/commandBuffer.h"
#include "gpu/device/include/device.h"
#include "engine/mesh/include/service.h"
#include "engine/material/include/runtimeService.h"
#include "gpu/device/include/descriptor.h"
#include "gpu/device/include/image.h"

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

FrameRenderer::FrameRenderer(const FrameParams& frame, const FrameCompositionTarget& target, const FrameResources& resources, const FrameHelper& helpers, const Scene* scene)
    : m_frame(frame)
    , m_resources(resources)
	, m_helpers(helpers)
	, m_target(target)
    , m_allocator(POOL_RENDERING_FRAME)
    , m_scene(scene)
{
}

FrameRenderer::~FrameRenderer()
{
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

void FrameRenderer::prepare(gpu::CommandWriter& cmd)
{
    gpu::CommandWriterBlock block(cmd, "PrepareFrame");

    // prepare some major services
    GetService<MeshService>()->uploadChanges(cmd);

    // dispatch all material changes
    GetService<MaterialService>()->dispatchMaterialProxyChanges();

    // bind global frame params
    bindFrameParameters(cmd);
}

void FrameRenderer::finish(gpu::CommandWriter& cmd, FrameStats& outStats)
{
}

//--

END_BOOMER_NAMESPACE_EX(rendering)
