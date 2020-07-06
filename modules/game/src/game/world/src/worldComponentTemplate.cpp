/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\content #]
*
***/

#include "build.h"
#include "worldComponentTemplate.h"

namespace game
{

    //--

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ComponentTemplate);
        RTTI_CATEGORY("Template");
        RTTI_PROPERTY(m_enabled).editable("Is this component enabled (will be created in the entity)");
        RTTI_CATEGORY("Transform");
        RTTI_PROPERTY(m_placement).editable("Placement with respect to transform parent");
    RTTI_END_TYPE();
        
    ComponentTemplate::ComponentTemplate()
        : m_enabled(true)
    {}

    //--

} // game
