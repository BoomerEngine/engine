/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once

#include "engine/world/include/world.h"
#include "engine/world/include/entity.h"
#include "engine/world/include/streamingSystem.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// A scene that can be used in game
class ENGINE_GAME_API GameWorld : public World
{
    RTTI_DECLARE_VIRTUAL_CLASS(GameWorld, World);

public:
    GameWorld();
    virtual ~GameWorld();

    //--

    // update scene
    CAN_YIELD virtual void update(double dt) override;

    //--

    // render to on screen canvas
    void renderCanvas(canvas::Canvas& canvas);

    //--

    /// get entities on the input stack
    INLINE const Array<EntityPtr>& inputStack() const { return m_inputStack; }

    // service input message
    bool processInputEvent(const input::BaseEvent& evt);

    /// push entity to input stack
    void pushInputEntity(Entity* ent);

    /// pop entity from input stack
    void popInputEntity(Entity* ent);

    //--

    /// get entities on the view stack
    INLINE const Array<EntityPtr>& viewStack() const { return m_viewStack; }

    /// push entity to camera stack
    void pushViewEntity(Entity* ent);

    /// pop entity from camera stack
    void popViewEntity(Entity* ent);

    //--

    /// calculate rendering camera 
    bool calculateCamera(EntityCameraPlacement& outCamera);

    /// toggle free camera
    void toggleFreeCamera(bool enabled, const Vector3* newPosition=nullptr, const Angles* newRotation=nullptr);

    //--

    /// create content streaming task for this world so all required content is streamed
    RefPtr<StreamingTask> createStreamingTask() const;

    /// apply finished streaming update task
    void applyStreamingTask(const StreamingTask* task);

    //--

public:
    Array<EntityPtr> m_inputStack;
    Array<EntityPtr> m_viewStack;

    Vector3 m_lastViewPosition;
    Quat m_lastViewRotation;

    bool m_freeCameraEnabled = false;
    FreeCameraHelper* m_freeCamera = nullptr;
};

//--

END_BOOMER_NAMESPACE()
