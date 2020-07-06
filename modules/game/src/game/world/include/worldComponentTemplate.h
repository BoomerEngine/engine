/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\content #]
***/

#pragma once

#include "base/object/include/objectTemplate.h"
#include "worldNodePlacement.h"

namespace game
{
    //--

    /// template of a component
    class GAME_WORLD_API ComponentTemplate : public base::IObjectTemplate
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ComponentTemplate, base::IObjectTemplate);

    public:
        ComponentTemplate();

        //--

        // is this template enabled ? disabled template will not be applied (as if it was deleted)
        bool m_enabled = true;

        // placement with respect to the parent entity or transform parent
        NodeTemplatePlacement m_placement;

        //--

        // create the component from template data - called only for CONTENT nodes
        virtual ComponentPtr createComponent() const = 0;

        // apply override to created component - called only for OVERRIDE nodes
        virtual void applyOverrides(Component* c) const = 0;

        //--
    };

    //--

} // game