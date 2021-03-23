/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once

#include "engine/world/include/worldViewEntity.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// A master entity for a player in a game
class GAME_COMMON_API IGamePlayerEntity : public IWorldViewEntity
{
    RTTI_DECLARE_VIRTUAL_CLASS(IGamePlayerEntity, IWorldViewEntity);

public:
    IGamePlayerEntity();
    virtual ~IGamePlayerEntity();

    //--

    /// get the owning player (MAY BE NONE - ie. player disconnected and this is the husk)
    INLINE IGamePlayer* player() const { return m_player; }

    // called when actual network player wants to take control of this entity
    void attachToPlayer(IGamePlayer* player);

    // called when actual network player no longer wants to control this entity
    void detachFromPlayer(IGamePlayer* player);

    //--

    /// handle player input, called ONLY when entity is local
    virtual bool handleInput(const InputEvent& evt) = 0;

    //--

    // push other entity to view stack
    void pushViewEntity(IWorldViewEntity* view);

    // remove entity from the view stack
    void removeViewEntity(IWorldViewEntity* view);

    //--

protected:
    virtual void evaluateCamera(CameraSetup& outSetup) const override;
    virtual void handleTransformUpdate(const EntityThreadContext& tc) override;
    virtual void handleAttach() override;
    virtual void handleDetach() override;

private:
    IGamePlayer* m_player = nullptr;

    WorldPersistentObserverPtr m_observer;

    Array<WorldViewEntityPtr> m_viewStack;
};

//--

END_BOOMER_NAMESPACE()
