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
#include "rendering/device/include/renderingDeviceService.h"

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
		const auto sceneType = rendering::scene::SceneType::Game; // TODO!
		m_scene = base::RefNew<rendering::scene::Scene>(sceneType);

        return true;
    }

    void RenderingSystem::handleShutdown()
    {
        m_scene.reset();
    }

    void RenderingSystem::handleRendering(rendering::scene::FrameParams& info)
    {
        info.scenes.mainScenePtr = m_scene;
    }

    ///---

} // game

