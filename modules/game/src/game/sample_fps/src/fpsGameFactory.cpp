/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: fps #]
***/

#include "build.h"
#include "fpsGameFactory.h"
#include "fpsGameWorldScreen.h"
#include "fpsGame.h"
#include "fpsFreeCamera.h"

#include "game/world/include/world.h"
#include "game/world/include/worldPrefab.h"
#include "game/world/include/worldMeshComponentTemplate.h"
#include "rendering/mesh/include/renderingMesh.h"
#include "base/math/include/absoluteTransform.h"

namespace fps
{
    //--

    base::res::StaticResource<game::Prefab> resFallbackWorldPrefab("game/prefabs/debug/fallback_world.v4prefab");
    //base::res::StaticResource<rendering::Mesh> resFallbackPlaneMesh("engine/tests/meshes/plane.obj");
    base::res::StaticResource<rendering::Mesh> resFallbackPlaneMesh("engine/scene/sponza/sponza.obj");

    //--

    game::PrefabPtr CreateFallbackPrefab()
    {
        auto container = base::CreateSharedPtr<game::NodeTemplateContainer>();

        {
            game::NodeTemplateConstructionInfo data;
            data.name = "MeshNode"_id;

            {
                auto componentTemplate = base::CreateSharedPtr<game::MeshComponentTemplate>();
                componentTemplate->m_mesh = base::res::ResourceKey(base::res::ResourcePath(resFallbackPlaneMesh.path()), rendering::Mesh::GetStaticClass());
                componentTemplate->m_placement = base::AbsoluteTransform();
                data.componentData.emplaceBack("Mesh"_id, componentTemplate);
            }

            auto node = base::CreateSharedPtr<game::NodeTemplate>(data);
            container->addNode(node);
        }

        return base::CreateSharedPtr<game::Prefab>(container);
    }

    game::WorldPtr CreateFallbackWorld()
    {
        auto prefab = resFallbackWorldPrefab.loadAndGet();
        if (!prefab)
        {
            prefab = CreateFallbackPrefab(); // create it manually
            if (!prefab)
                return nullptr;
        }

        auto world = base::CreateSharedPtr<game::World>();
        world->createPrefabInstance(base::AbsoluteTransform::ROOT(), prefab);

        return world;
    }

    //--

    RTTI_BEGIN_TYPE_CLASS(GameFactory);
    RTTI_END_TYPE();

    GameFactory::GameFactory()
    {}

    GameFactory::~GameFactory()
    {}

    game::GamePtr GameFactory::createGame(const game::GameInitData& initData)
    {
        // load the initial world
        const auto world = CreateFallbackWorld();
        if (!world)
            return nullptr;

        // determine spawn position
        auto spawnPosition = base::Vector3(0, 0, 1);
        auto spawnRotation = base::Angles(20.0f, 0.0f, 0.0f);
        if (initData.spawnPositionOverrideEnabled)
            spawnPosition = initData.spawnPositionOverride;
        if (initData.spawnRotationOverrideEnabled)
            spawnRotation = initData.spawnRotationOverride;

        // create free camera entity
        auto freeCamera = base::CreateSharedPtr<FreeCameraEntity>();
        freeCamera->place(spawnPosition, spawnRotation);
        world->attachEntity(freeCamera);
        freeCamera->activate();

        // create the initial screen
        auto screen = base::CreateSharedPtr<WorldScreen>(world);

        // finally create the game
        auto game = base::CreateSharedPtr<Game>();
        game->requestScreenSwitch(game::GameScreenType::Background, screen);

        return game;
    }

    //--

} // fps