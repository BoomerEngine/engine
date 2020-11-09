/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world #]
*
***/

#include "build.h"
#include "renderingSystem.h"
#include "rendering/scene/include/renderingScene.h"
#include "rendering/scene/include/renderingFrameParams.h"

namespace rendering
{

    //---

    RTTI_BEGIN_TYPE_CLASS(RenderingSystem);
    RTTI_END_TYPE();

    RenderingSystem::RenderingSystem()
    {}

    RenderingSystem::~RenderingSystem()
    {}

    bool RenderingSystem::handleInitialize(base::world::World& scene)
    {
        rendering::scene::SceneSetupInfo setup;
        setup.name = "World";
        setup.type = rendering::scene::SceneType::Game; // TODO!

        m_scene = base::CreateSharedPtr<rendering::scene::Scene>(setup);
        return true;
    }

    void RenderingSystem::handleShutdown()
    {
        m_scene.reset();
    }

    void RenderingSystem::handleRendering(rendering::scene::FrameParams& info)
    {
        auto& entry = info.scenes.scenesToDraw.emplaceBack();
        entry.scenePtr = m_scene;
    }

    ///---

} // game

