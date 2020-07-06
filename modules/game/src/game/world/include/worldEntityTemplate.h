/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\content #]
***/

#pragma once

#include "base/object/include/objectTemplate.h"

namespace game
{
    //--

    /// template of an entity
    class GAME_WORLD_API EntityTemplate : public base::IObjectTemplate
    {
        RTTI_DECLARE_VIRTUAL_CLASS(EntityTemplate, base::IObjectTemplate);

    public:
        EntityTemplate();

        //--

        // is this template enabled ? disabled template will not be applied (as if it was deleted)
        bool m_enabled = true;

        //--

        // create the entity from data - called only for CONTENT nodes
        virtual EntityPtr createEntity() const = 0;

        // apply override to created entity - called only for OVERRIDE nodes
        virtual void applyOverrides(Entity* c) const = 0;
    };

    //--

} // game