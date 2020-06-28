/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#include "build.h"
#include "sceneTest.h"
#include "simpleCamera.h"

#include "game/scene/include/world.h"
#include "game/scene/include/gameEntity.h"
#include "game/scene/include/worldDefinition.h"

#include "rendering/mesh/include/renderingMesh.h"

namespace game
{
    namespace test
    {

        //--

        RTTI_BEGIN_TYPE_CLASS(SceneTestOrderMetadata);
        RTTI_END_TYPE();

        //--

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ISceneTest);
        RTTI_END_TYPE();

        ISceneTest::ISceneTest()
            : m_failed(false)
        {
        }

        bool ISceneTest::processInitialization()
        {
            initialize();
            return !m_failed;
        }

        void ISceneTest::configure()
        {
            // render im gui
        }

        void ISceneTest::initialize()
        {
            // scenes usually create world here
        }

        void ISceneTest::render(rendering::scene::FrameParams& info)
        {
            if (m_world)
                m_world->render(info);
        }

        void ISceneTest::update(float dt)
        {
            if (m_world)
            {
                game::UpdateContext ctx;
                ctx.m_dt = dt;
                m_world->update(ctx);
            }
        }

        bool ISceneTest::processInput(const base::input::BaseEvent& evt)
        {
            if (m_world)
                return m_world->processInput(evt);
            return false;
        }

        void ISceneTest::reportError(base::StringView<char> msg)
        {
            TRACE_ERROR("SceneTest initialization error: {}", msg);
            m_failed = true;
        }

        rendering::MeshPtr ISceneTest::loadMesh(base::StringView<char> assetFile)
        {
            auto fullPath = base::res::ResourcePath(base::TempString("engine/tests/meshes/{}", assetFile));
            auto meshPtr = base::LoadResource<rendering::Mesh>(fullPath);
            if (!meshPtr)
            {
                reportError(base::TempString("Failed to load mesh '{}'", meshPtr));
                return nullptr;
            }

            return meshPtr.acquire();
        }
        
        //--

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ISceneTestEmptyWorld);
        
        RTTI_END_TYPE();

        ISceneTestEmptyWorld::ISceneTestEmptyWorld()
        {
            m_initialCameraPosition = base::Vector3(-2, 0, 1);
            m_initialCameraRotation = base::Angles(-20.0f, 0.0f, 0.0f);;
        }

        void ISceneTestEmptyWorld::recreateWorld()
        {
            if (m_camera)
                m_camera->deactivate();

            if (m_world)
                m_world->detachEntity(m_camera);

            m_camera.reset();
            m_world.reset();

            auto data = base::CreateSharedPtr<WorldDefinition>();
            m_world = base::CreateSharedPtr<World>(WorldType::Game, data);

            m_camera = base::CreateSharedPtr<FlyCameraEntity>(m_initialCameraPosition, m_initialCameraRotation);

            m_world->attachEntity(m_camera);
            m_camera->activate();

            createWorldContent();
        }

        void ISceneTestEmptyWorld::createWorldContent()
        {
            // TODO
        }

        //--

        void ISceneTestEmptyWorld::initialize()
        {
            TBaseClass::initialize();
            recreateWorld();
        }

        //--

    } // test
} // rendering