/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entity #]
*
***/

#pragma once

namespace base
{
    namespace world
    {
        //---

        // behavior attached to entity
        class BASE_WORLD_API IEntityBehavior : public IObject
        {
            RTTI_DECLARE_POOL(POOL_WORLD_OBJECTS)
            RTTI_DECLARE_VIRTUAL_CLASS(IEntityBehavior, IObject);

        public:
            IEntityBehavior();
            virtual ~IEntityBehavior();
        };

        //---

    } // world
} // base
