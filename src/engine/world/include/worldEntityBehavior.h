/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entity #]
*
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//---

// behavior attached to entity
class ENGINE_WORLD_API IEntityBehavior : public IObject
{
    RTTI_DECLARE_POOL(POOL_WORLD_OBJECTS)
    RTTI_DECLARE_VIRTUAL_CLASS(IEntityBehavior, IObject);

public:
    IEntityBehavior();
    virtual ~IEntityBehavior();
};

//---

END_BOOMER_NAMESPACE()
