/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once

#include "game/common/include/playerEntity.h"
#include "engine/world_entities/include/entityFreeCamera.h"

BEGIN_BOOMER_NAMESPACE()

//--

class FreeCameraEntity;

/// a non-interactive player (flying camera)
class GAME_TEMPLATES_API DefaultPlayerEntity : public IGamePlayerEntity
{
    RTTI_DECLARE_VIRTUAL_CLASS(DefaultPlayerEntity, IGamePlayerEntity);

public:
    DefaultPlayerEntity();

    void requestTransformChange(const Vector3& pos, const Angles& angles);

protected:
    virtual bool handleInput(const InputEvent& evt) override;
    virtual void handleUpdate(const EntityThreadContext& tc, WorldUpdatePhase phase, float dt) override final;
    virtual void handleUpdateMask(WorldUpdateMask& outUpdateMask) const override final;

    virtual void evaluateCamera(CameraSetup& outSetup) const override;

    FreeCamera m_freeCamera;
};

//--

END_BOOMER_NAMESPACE()
