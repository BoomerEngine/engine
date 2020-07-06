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
#include "worldSystem.h"

namespace game
{

    //---

    RTTI_BEGIN_TYPE_CLASS(DependsOnWorldSystemMetadata);
    RTTI_END_TYPE();

    DependsOnWorldSystemMetadata::DependsOnWorldSystemMetadata()
    {}

    //---

    RTTI_BEGIN_TYPE_CLASS(TickOrderWorldSystemMetadata);
    RTTI_END_TYPE();

    TickOrderWorldSystemMetadata::TickOrderWorldSystemMetadata()
    {}

    //---
    
    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IWorldSystem);
    RTTI_END_TYPE();

    IWorldSystem::IWorldSystem()
    {}

    IWorldSystem::~IWorldSystem()
    {}

    bool IWorldSystem::handleInitialize(World& world)
    {
        return true;
    }

    void IWorldSystem::handleShutdown(World& world)
    {}

    void IWorldSystem::handlePreTick(World& world, double dt)
    {}

    void IWorldSystem::handleMainTickStart(World& scene, double dt)
    {}

    void IWorldSystem::handleMainTickFinish(World& scene, double dt)
    {}

    void IWorldSystem::handleMainTickPublish(World& scene, double dt)
    {}

    void IWorldSystem::handlePostTick(World& world, double dt)
    {}

    void IWorldSystem::handlePostTransform(World& world, double dt)
    {}

    void IWorldSystem::handlePrepareCamera(World& scene, rendering::scene::CameraSetup& outCamera)
    {}

    void IWorldSystem::handleRendering(World& world, rendering::scene::FrameParams& info)
    {}

    /*void IWorldSystem::handleWorldContentAttached(World& world, const WorldPtr& worldPtr)
    {}

    void IWorldSystem::handleWorldContentDetached(World& world, const WorldPtr& worldPtr)
    {}*/

    ///---

} // game

