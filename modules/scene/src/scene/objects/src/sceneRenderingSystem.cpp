/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
*
***/

#include "build.h"
#include "sceneRenderingSystem.h"

#include "base/app/include/localServiceContainer.h"
#include "scene/common/include/sceneRuntime.h"
#include "rendering/scene/include/renderingFrame.h"
#include "rendering/scene/include/renderingView.h"
#include "rendering/runtime/include/renderingRuntimeService.h"

namespace scene
{
    ///---

    RTTI_BEGIN_TYPE_CLASS(RenderingSystem);
        RTTI_METADATA(scene::RuntimeSystemInitializationOrderMetadata).order(0);
    RTTI_END_TYPE();

    ///---

    IRenderable::~IRenderable()
    {}

    ///---

    RenderingSystem::RenderingSystem()
    {}

    RenderingSystem::~RenderingSystem()
    {
        for (auto pool  : m_renderablePools)
            MemFree(pool);
    }

    bool RenderingSystem::checkCompatiblity(SceneType type) const
    {
        return true;//(type != SceneType::ServerGame);
    }

    bool RenderingSystem::onInitialize(Scene& scene)
    {
        prepareRenderableBatch();
        return true;
    }

    void RenderingSystem::onPreTick(Scene& scene, const UpdateContext& ctx)
    {	
    }

    void RenderingSystem::onPreTransform(Scene& scene, const UpdateContext& ctx)
    {
        // nothing
    }

    void RenderingSystem::onPostTransform(Scene& scene, const UpdateContext& ctx)
    {
        // nothing
    }

    void RenderingSystem::onRenderFrame(Scene& scene, rendering::scene::FrameInfo& info)
    {
        // when collecting fragments collect them from us as well
        info.bindFragmentSource(this);
    }

    ///---

    void RenderingSystem::prepareRenderableBatch()
    {
        static const int NUM_FRAGMENTS_PER_POOL = 1024;

        auto memory  = (RenderableInfo*) MemAlloc(POOL_TEMP, sizeof(RenderableInfo) * NUM_FRAGMENTS_PER_POOL, alignof(RenderableInfo));
        memset(memory, 0, sizeof(RenderableInfo) * NUM_FRAGMENTS_PER_POOL);

        for (int i = NUM_FRAGMENTS_PER_POOL-1; i >= 0; --i)
            m_freeRenderables.pushBack(memory + i);
    }

    RenderingSystem::RenderableInfo* RenderingSystem::allocRenderable()
    {
        if (m_freeRenderables.empty())
            prepareRenderableBatch();

        auto renderable  = m_freeRenderables.back();
        m_freeRenderables.popBack();
        m_activeRenderables.pushBack(renderable);
        return renderable;
    }

    void RenderingSystem::registerRenderable(IRenderable* renderable, const base::Box& bounds, float maxRenderingDistance)
    { 
        RenderableInfo* info = nullptr;
        if (!m_renderableMap.find(renderable, info))
        {
            info = allocRenderable();
            m_renderableMap[renderable] = info;
        }

        info->m_box.setup(bounds);
        info->m_maxDistance = maxRenderingDistance;
        info->m_renderable = renderable;
    }

    void RenderingSystem::unregisterRenderable(IRenderable* renderable)
    {
        RenderableInfo* info = nullptr;
        if (m_renderableMap.find(renderable, info))
        {
            m_activeRenderables.remove(info);
            m_freeRenderables.pushBack(info);
            m_renderableMap.remove(renderable);
        }
    }

    void RenderingSystem::collectFragmentsForView(rendering::scene::IRendererView& view) const
    {
        auto numFrustums = view.numCullingFrustums();
        auto frustums  = view.cullingFrustums();

        for (auto renderable  : m_activeRenderables)
        {
            uint8_t cameraMask = 0;

            for (uint32_t i = 0; i < numFrustums; ++i)
                if (renderable->m_box.isInFrustum(frustums[i]))
                    cameraMask |= 1U << i;

            if (cameraMask)
                renderable->m_renderable->collectRenderableStuff(cameraMask, view);
        }
    }

    ///---

} // scene