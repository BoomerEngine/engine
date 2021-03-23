/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once

#include "platform.h"
#include "playerIdentity.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// player connection tempshit
class GAME_COMMON_API IGamePlayerConnection : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(IGamePlayerConnection, IObject);

public:
    IGamePlayerConnection(IGamePlayerIdentity* identity);
    virtual ~IGamePlayerConnection();

    /// identity of the player that created this connection
    /// NOTE: this may not be known initially
    INLINE const GamePlayerIdentityPtr& identity() const { return m_identity; }

    //--

private:
    GamePlayerIdentityPtr m_identity;
};

//--

END_BOOMER_NAMESPACE()
