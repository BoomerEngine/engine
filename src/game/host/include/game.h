/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once

#include "base/world/include/worldEntity.h"
#include "base/object/include/object.h"

namespace game
{

    //--

    /// initialization params for the game stack
    struct GAME_HOST_API GameInitData
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(GameInitData);

        base::Vector3 spawnPositionOverride;
        base::Angles spawnRotationOverride;
        bool spawnPositionOverrideEnabled = false;
        bool spawnRotationOverrideEnabled = false;

        base::StringBuf startupSceneOverride;
    };

    //--

    /// rendering viewport
    struct GAME_HOST_API GameViewport
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(GameViewport);

        WorldPtr world;
        base::Rect viewportRect;
        base::world::EntityCameraPlacement cameraPlacement;
    };

    //--

    /// The game instance class
    class GAME_HOST_API Game : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(Game, base::IObject);

    public:
        Game();
        virtual ~Game();

        //--

        // get global scene, can be used to spawn game-global entities/controller
        // NOTE: this scene is never deleted nor are the entities
        INLINE const WorldPtr& globalWorld() const { return m_globalWorld; }

        // get all attached (ticked) scenes
        INLINE const base::Array<WorldPtr>& worlds() const { return m_worlds; }

        //--

        // update with given engine time delta
        bool processUpdate(double dt);

        // render all required content, all command should be recorded via provided command buffer writer
        void processRender(const base::Rect& mainViewportRect, Array<GameViewport>& outViewports);

        // service input message
        bool processInput(const base::input::BaseEvent& evt);

        //--

        // request closing of the game object, usually called from some kind of "Quit to Desktop" option
        void requestClose();

        //--

        // attach additional scene, will be ticked
        void attachWorld(World* world);

        /// detach already attached scene
        void detachWorld(World* world);

        //--

        /// push entity to input stack
        void pushInputEntity(base::world::Entity* ent);

        /// pop entity from input stack
        void popInputEntity(base::world::Entity* ent);

        //--

        /// push entity to camera stack
        void pushViewEntity(base::world::Entity* ent);

        /// pop entity from camera stack
        void popViewEntity(base::world::Entity* ent);

        //--

    protected:
        //--

        WorldPtr m_globalWorld;
        base::Array<WorldPtr> m_worlds;

        base::Array<base::world::EntityPtr> m_inputStack;
        base::Array<base::world::EntityPtr> m_viewStack;

        //--

        bool m_requestedClose = false;
    };

    //--

} // game
