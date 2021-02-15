/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"
#include "game.h"
#include "gameWorld.h"
#include "base/world/include/worldEntity.h"

namespace game
{
    //--

    RTTI_BEGIN_TYPE_CLASS(GameInitData)
        RTTI_PROPERTY(spawnPositionOverride);
        RTTI_PROPERTY(spawnRotationOverride);
        RTTI_PROPERTY(spawnPositionOverrideEnabled);
        RTTI_PROPERTY(spawnRotationOverrideEnabled);
    RTTI_END_TYPE();

    //----

    RTTI_BEGIN_TYPE_CLASS(GameViewport);
        RTTI_PROPERTY(world);
        RTTI_PROPERTY(viewportRect);
        RTTI_PROPERTY(cameraPlacement);
    RTTI_END_TYPE();

    //----

    RTTI_BEGIN_TYPE_CLASS(Game);
    RTTI_END_TYPE();

    Game::Game()
    {
        m_globalWorld = RefNew<World>();
        m_worlds.pushBack(m_globalWorld);
    }

    Game::~Game()
    {
        m_worlds.clear();
        m_globalWorld.reset();
    }

    void Game::attachWorld(World* world)
    {
        DEBUG_CHECK_RETURN_EX(world, "Invalid world");
        DEBUG_CHECK_RETURN_EX(!m_worlds.contains(world), "World already attached");
        m_worlds.pushBack(AddRef(world));
    }

    void Game::detachWorld(World* world)
    {
        DEBUG_CHECK_RETURN_EX(world, "Invalid world");
        DEBUG_CHECK_RETURN_EX(world != m_globalWorld, "Cannot detach global scene");
        DEBUG_CHECK_RETURN_EX(m_worlds.contains(world), "World not attached");
        m_worlds.remove(world);
    }

    void Game::pushInputEntity(base::world::Entity* ent)
    {
        DEBUG_CHECK_RETURN_EX(ent, "Invalid entity");
        DEBUG_CHECK_RETURN_EX(m_inputStack.contains(ent), "Entity already on input stack");
        m_inputStack.pushBack(AddRef(ent));
    }

    void Game::popInputEntity(base::world::Entity* ent)
    {
        DEBUG_CHECK_RETURN_EX(ent, "Invalid entity");
        DEBUG_CHECK_RETURN_EX(!m_inputStack.contains(ent), "Entity not on input stack");
        m_inputStack.remove(ent);
    }

    void Game::pushViewEntity(base::world::Entity* ent)
    {
        DEBUG_CHECK_RETURN_EX(ent, "Invalid entity");
        DEBUG_CHECK_RETURN_EX(!m_viewStack.contains(ent), "Entity already on view stack");
        m_viewStack.pushBack(AddRef(ent));
    }

    void Game::popViewEntity(base::world::Entity* ent)
    {
        DEBUG_CHECK_RETURN_EX(ent, "Invalid entity");
        DEBUG_CHECK_RETURN_EX(m_viewStack.contains(ent), "Entity not on view stack");
        m_viewStack.remove(ent);
    }

    bool Game::processUpdate(double dt)
    {
        for (auto& scene : m_worlds)
            scene->update(dt);

        return !m_requestedClose;
    }

    void Game::processRender(const base::Rect& mainViewportRect, Array<GameViewport>& outViewports)
    {
        for (auto index : m_viewStack.indexRange().reversed())
        {
            auto camera = m_viewStack[index];

            if (auto world = rtti_cast<World>(camera->world()))
            {
                if (m_worlds.contains(world))
                {
                    base::world::EntityCameraPlacement placement;
                    if (camera->handleCamera(placement))
                    {
                        auto& viewport = outViewports.emplaceBack();
                        viewport.cameraPlacement = placement;
                        viewport.viewportRect = mainViewportRect;
                        viewport.world = AddRef(world);

                        break;
                    }
                }
            }
        }
    }

    bool Game::processInput(const base::input::BaseEvent& evt)
    {
        base::world::EntityInputEvent gameEvt;

        for (auto index : m_inputStack.indexRange().reversed())
        {
            auto ent = m_inputStack[index];
            if (ent->handleInput(gameEvt))
            {
                // TODO: remember entity that got the "press"
                return true;
            }
        }

        return false;
    }

    void Game::requestClose()
    {
        if (!m_requestedClose)
        {
            TRACE_INFO("Game requested close");
            m_requestedClose = true;
        }
    }

    //---

} // ui
