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
#include "worldRenderingSystem.h"
#include "rendering/scene/include/renderingScene.h"
#include "rendering/scene/include/renderingFrameParams.h"

namespace game
{

    //---

    RTTI_BEGIN_TYPE_CLASS(WorldRenderingSystem);
    RTTI_END_TYPE();

    WorldRenderingSystem::WorldRenderingSystem()
    {}

    WorldRenderingSystem::~WorldRenderingSystem()
    {}

    bool WorldRenderingSystem::handleInitialize(World& scene)
    {
        rendering::scene::SceneSetupInfo setup;
        setup.name = "World";
        setup.type = rendering::scene::SceneType::Game; // TODO!

        m_scene = base::CreateSharedPtr<rendering::scene::Scene>(setup);
        return true;
    }

    void WorldRenderingSystem::handleShutdown(World& scene)
    {
        m_scene.reset();
    }

    void WorldRenderingSystem::handleRendering(World& scene, rendering::scene::FrameParams& info)
    {
        auto& entry = info.scenes.scenesToDraw.emplaceBack();
        entry.scenePtr = m_scene;
    }

    ///---

} // game

