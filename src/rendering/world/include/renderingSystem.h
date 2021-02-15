/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world #]
***/

#pragma once

#include "base/world/include/worldSystem.h"

namespace rendering
{
    //---

    /// rendering system - contains rendering scene
    class RENDERING_WORLD_API RenderingSystem : public base::world::IWorldSystem
    {
        RTTI_DECLARE_VIRTUAL_CLASS(RenderingSystem, base::world::IWorldSystem);

    public:
        RenderingSystem();
        virtual ~RenderingSystem();

        //--

        /// get rendering scene
        INLINE rendering::scene::Scene* scene() const { return m_scene; }

        //--

    protected:
        // IWorldSystem
        virtual bool handleInitialize(base::world::World& scene) override;
        virtual void handleShutdown() override;
        virtual void handleRendering(rendering::scene::FrameParams& info) override;

        //--

        rendering::scene::ScenePtr m_scene;
    };

    //---

} // game