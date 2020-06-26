/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
*
***/

#pragma once

#include "scene/common/include/sceneRuntimeSystem.h"
#include "rendering/scene/include/renderingFragmentHandler.h"
#include "rendering/scene/include/renderingFrameCamera.h"

namespace scene
{
    /// base interface for renderable component
    class SCENE_OBJECTS_API IRenderable : public base::NoCopy
    {
    public:
        virtual ~IRenderable();

        /// collect stuff into view
        virtual void collectRenderableStuff(uint8_t cameraMask, rendering::scene::IRendererView& view) = 0;
    };

    /// the rendering integration for components
    class SCENE_OBJECTS_API RenderingSystem : public IRuntimeSystem, public rendering::scene::IFragmentSource
    {
        RTTI_DECLARE_VIRTUAL_CLASS(RenderingSystem, IRuntimeSystem);

    public:
        RenderingSystem();
        virtual ~RenderingSystem();

        /// register/update renderable within the scene
        void registerRenderable(IRenderable* renderable, const base::Box& bounds, float maxRenderingDistance);

        /// remove collectible from scene
        void unregisterRenderable(IRenderable* collectible);

    private:
        virtual bool checkCompatiblity(SceneType type) const override final;
        virtual bool onInitialize(Scene& scene) override final;
        virtual void onPreTick(Scene& scene, const UpdateContext& ctx) override final;
        virtual void onPreTransform(Scene& scene, const UpdateContext& ctx) override final;
        virtual void onPostTransform(Scene& scene, const UpdateContext& ctx) override final;
        virtual void onRenderFrame(Scene& scene, rendering::scene::FrameInfo& info) override final;

        //--

        virtual void collectFragmentsForView(rendering::scene::IRendererView& view) const override final;

        //--

        struct RenderableInfo
        {
            rendering::scene::VisibilityBox m_box;
            float m_maxDistance;
            IRenderable* m_renderable;
        };

        base::Array<void*> m_renderablePools;
        base::Array<RenderableInfo*> m_freeRenderables;
        base::Array<RenderableInfo*> m_activeRenderables;
        base::HashMap<IRenderable*, RenderableInfo*> m_renderableMap;

        void prepareRenderableBatch();
        RenderableInfo* allocRenderable();
    };

} // scene

