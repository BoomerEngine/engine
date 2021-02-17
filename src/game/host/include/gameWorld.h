/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once

#include "base/world/include/world.h"
#include "base/world/include/worldEntity.h"
#include "base/world/include/worldStreamingSystem.h"

namespace game
{
    //--

    /// A scene that can be used in game
    class GAME_HOST_API World : public base::world::World
    {
        RTTI_DECLARE_VIRTUAL_CLASS(World, base::world::World);

    public:
        World();
        virtual ~World();

        //--

        // update scene
        CAN_YIELD virtual void update(double dt) override;

        //--

        // render to on screen canvas
        void renderCanvas(base::canvas::Canvas& canvas);

        //--

        /// get entities on the input stack
        INLINE const base::Array<base::world::EntityPtr>& inputStack() const { return m_inputStack; }

        // service input message
        bool processInputEvent(const base::input::BaseEvent& evt);

        /// push entity to input stack
        void pushInputEntity(base::world::Entity* ent);

        /// pop entity from input stack
        void popInputEntity(base::world::Entity* ent);

        //--

        /// get entities on the view stack
        INLINE const base::Array<base::world::EntityPtr>& viewStack() const { return m_viewStack; }

        /// push entity to camera stack
        void pushViewEntity(base::world::Entity* ent);

        /// pop entity from camera stack
        void popViewEntity(base::world::Entity* ent);

        //--

        /// calculate rendering camera 
        bool calculateCamera(base::world::EntityCameraPlacement& outCamera);

        /// toggle free camera
        void toggleFreeCamera(bool enabled, const base::Vector3* newPosition=nullptr, const base::Angles* newRotation=nullptr);

        //--

        /// create content streaming task for this world so all required content is streamed
        base::RefPtr<base::world::StreamingTask> createStreamingTask() const;

        /// apply finished streaming update task
        void applyStreamingTask(const base::world::StreamingTask* task);

        //--

    public:
        base::Array<base::world::EntityPtr> m_inputStack;
        base::Array<base::world::EntityPtr> m_viewStack;

        base::Vector3 m_lastViewPosition;
        base::Quat m_lastViewRotation;

        bool m_freeCameraEnabled = false;
        FreeCameraHelper* m_freeCamera = nullptr;
    };

    //--

} // game
