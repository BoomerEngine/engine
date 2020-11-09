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

namespace base
{
    namespace world
    {

        //--

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ComponentTemplate);
            RTTI_PROPERTY(m_enabled);
            RTTI_PROPERTY(m_placement);
        RTTI_END_TYPE();

        ComponentTemplate::ComponentTemplate()
        {}

        void ComponentTemplate::enable(bool flag, bool callEvent /*= true*/)
        {
            if (m_enabled != flag)
            {
                m_enabled = flag;
                if (callEvent)
                    onPropertyChanged("enabled");
            }
        }

        void ComponentTemplate::placement(const EulerTransform& placement, bool callEvent/*= true*/)
        {
            if (m_placement != placement)
            {
                m_placement = placement;
                if (callEvent)
                    onPropertyChanged("placement");
            }
        }

        //--

    } // world
} // base
