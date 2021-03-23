/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once

#include "engine/world/include/worldEntity.h"
#include "engine/rendering/include/stats.h"
#include "core/object/include/object.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// Identity of a player, usually connected to the play platform (Steam, GOG, etc)
class GAME_COMMON_API IGamePlayerIdentity : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(IGamePlayerIdentity, IObject);

public:
    IGamePlayerIdentity();
    virtual ~IGamePlayerIdentity();

    //--

    /// get player ID
    virtual const StringBuf& id() const = 0;

    /// get player displayable nick name
    virtual const StringBuf& name() const = 0;

    /// is this a local player ?
    virtual bool local() const = 0;

    /// is this player signed in ? (false only for local anonymous players)
    virtual bool signedIn() const = 0;

    /// get controller index assigned to this player
    virtual int controllerIndex() const = 0;

    //--

};

//--

END_BOOMER_NAMESPACE()
