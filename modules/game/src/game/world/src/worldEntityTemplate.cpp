/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\content #]
*
***/

#include "build.h"
#include "worldEntityTemplate.h"

namespace game
{

    //--

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(EntityTemplate);
        RTTI_CATEGORY("Template");
        RTTI_PROPERTY(m_enabled).editable("Is this entity template enabled");
    RTTI_END_TYPE();
        
    EntityTemplate::EntityTemplate()
    {}

    //--

} // game
