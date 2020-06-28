/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\systems #]
***/

#pragma once

#include "worldSystem.h"

namespace game
{
    //---

    /// rendering system - contains rendering scene
    class GAME_SCENE_API WorldRenderingSystem : public IWorldSystem
    {
        RTTI_DECLARE_VIRTUAL_CLASS(WorldRenderingSystem, IWorldSystem);

    public:
        WorldRenderingSystem();
        virtual ~WorldRenderingSystem();

        //--

        /// get rendering scene
        INLINE rendering::scene::Scene* scene() const { return m_scene; }

        //--

    protected:
        // IWorldSystem
        virtual bool handleInitialize(World& scene) override;
        virtual void handleShutdown(World& scene) override;
        virtual void handleRendering(World& scene, rendering::scene::FrameParams& info) override;

        //--

        rendering::scene::ScenePtr m_scene;
    };

    //---

} // game