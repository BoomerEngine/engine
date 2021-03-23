/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"
#include "defaultGamePlayerEntity.h"

#include "engine/world_entities/include/entityFreeCamera.h"
#include "engine/world/include/world.h"

BEGIN_BOOMER_NAMESPACE()

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(DefaultPlayerEntity);
RTTI_END_TYPE();

DefaultPlayerEntity::DefaultPlayerEntity()
{}

void DefaultPlayerEntity::requestTransformChange(const Vector3& pos, const Angles& angles)
{
    m_freeCamera.m_rotation = angles;
    TBaseClass::requestTransformChange(pos);
}

bool DefaultPlayerEntity::handleInput(const InputEvent& evt)
{
    return m_freeCamera.input(evt);
}

void DefaultPlayerEntity::handleUpdate(const EntityThreadContext& tc, WorldUpdatePhase phase, float dt)
{
    TBaseClass::handleUpdate(tc, phase, dt);

    if (phase == WorldUpdatePhase::PreTick)
    {
        Vector3 deltaPosition;
        m_freeCamera.update(dt, deltaPosition);

        Transform transform;
        transform.T = cachedWorldTransform().T + deltaPosition;
        transform.R = m_freeCamera.m_rotation.toQuat();
        requestTransformChangeWorldSpace(transform);
    }
}

void DefaultPlayerEntity::handleUpdateMask(WorldUpdateMask& outUpdateMask) const
{
    outUpdateMask |= WorldUpdatePhase::PreTick;
}

void DefaultPlayerEntity::evaluateCamera(CameraSetup& outSetup) const
{
    outSetup.position = cachedWorldTransform().T;
    outSetup.rotation = m_freeCamera.m_rotation.toQuat();
}

//--

END_BOOMER_NAMESPACE()
