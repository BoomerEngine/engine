/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"
#include "player.h"
#include "playerEntity.h"

#include "engine/world/include/world.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IGamePlayerEntity);
RTTI_END_TYPE();

IGamePlayerEntity::IGamePlayerEntity()
{
}

IGamePlayerEntity::~IGamePlayerEntity()
{
    DEBUG_CHECK(m_player == nullptr);
    m_observer.reset();
}

void IGamePlayerEntity::attachToPlayer(IGamePlayer* player)
{
    DEBUG_CHECK_RETURN(player != nullptr);
    DEBUG_CHECK_RETURN(m_player == nullptr);
    m_player = player;
}

void IGamePlayerEntity::detachFromPlayer(IGamePlayer* player)
{
    DEBUG_CHECK_RETURN(m_player == player);
    m_player = nullptr;
}

void IGamePlayerEntity::handleAttach()
{
    TBaseClass::handleAttach();

    m_observer = RefNew<WorldPersistentObserver>();
    world()->attachPersistentObserver(m_observer);
}

void IGamePlayerEntity::handleDetach()
{
    if (m_observer)
    {
        world()->dettachPersistentObserver(m_observer);
        m_observer.reset();
    }

    TBaseClass::handleDetach();
}

void IGamePlayerEntity::handleTransformUpdate(const EntityThreadContext& tc)
{
    TBaseClass::handleTransformUpdate(tc);

    if (m_observer)
        m_observer->move(cachedWorldTransform().T);
}

void IGamePlayerEntity::evaluateCamera(CameraSetup& outSetup) const
{
    if (!m_viewStack.empty())
    {
        m_viewStack.back()->evaluateCamera(outSetup);
        return;
    }

    outSetup.position = cachedWorldTransform().T;
    outSetup.rotation = cachedWorldTransform().R;
}

void IGamePlayerEntity::pushViewEntity(IWorldViewEntity* view)
{
    DEBUG_CHECK_RETURN(view != nullptr);
    DEBUG_CHECK_RETURN(view != this);
    m_viewStack.pushBack(AddRef(view));
}

void IGamePlayerEntity::removeViewEntity(IWorldViewEntity* view)
{
    m_viewStack.remove(view);
}

//--

END_BOOMER_NAMESPACE()
