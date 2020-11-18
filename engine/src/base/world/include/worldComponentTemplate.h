/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\content #]
***/

#pragma once

#include "base/object/include/objectTemplate.h"

namespace base
{
    namespace world
    {

        //--

        /// template of a component
        class BASE_WORLD_API ComponentTemplate : public IObjectTemplate
        {
            RTTI_DECLARE_POOL(POOL_WORLD_OBJECTS)
            RTTI_DECLARE_VIRTUAL_CLASS(ComponentTemplate, IObjectTemplate);

        public:
            ComponentTemplate();

            //--

            // is this template enabled ? disabled template will not be applied (as if it was deleted)
            // NOTE: this property is not overridden but directly controlled
            INLINE bool enabled() const { return m_enabled; }

            // placement
            // NOTE: this property is not overridden but stacks (ie. transforms are stacking together)
            INLINE const Transform& placement() const { return m_placement; }

            //--

            // toggle the enable flag
            void enable(bool flag, bool callEvent = true);

            // set placement
            void placement(const Transform& placement, bool callEvent = true);

            //--

            // create the component from template data - called only for CONTENT nodes
            virtual ComponentPtr createComponent() const = 0;

        protected:
            bool m_enabled = true;
            Transform m_placement;
        };

        //--

    } // world
} // base