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

BEGIN_BOOMER_NAMESPACE()

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
    DEBUG_CHECK_RETURN_V(!m_world, false);
    m_world = &world;
    return true;
}

void IWorldSystem::handleShutdown()
{}

void IWorldSystem::handlePreTick(double dt)
{}

void IWorldSystem::handleMainTickStart(double dt)
{}

void IWorldSystem::handleMainTickFinish(double dt)
{}

void IWorldSystem::handleMainTickPublish(double dt)
{}

void IWorldSystem::handlePostTick(double dt)
{}

void IWorldSystem::handlePostTransform(double dt)
{}

void IWorldSystem::handleImGuiDebugInterface()
{}

void IWorldSystem::handleDebugRender(rendering::FrameParams& info)
{}

/*void IWorldSystem::handleWorldContentAttached(const WorldPtr& worldPtr)
{}

void IWorldSystem::handleWorldContentDetached(const WorldPtr& worldPtr)
{}*/

///---

END_BOOMER_NAMESPACE()
