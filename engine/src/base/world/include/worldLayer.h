/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\resources #]
***/

#pragma once

#include "base/resource/include/resource.h"

namespace base
{
    namespace world
    {
        //---

        /// Uncooked layer in the world, basic building block and source of most entities
        /// Layer is composed of a list of node templates
        class BASE_WORLD_API Layer : public res::IResource
        {
            RTTI_DECLARE_VIRTUAL_CLASS(Layer, res::IResource);

        public:
            Layer();
            Layer(const Array<NodeTemplatePtr>& sourceNodes);

            INLINE const Array<NodeTemplatePtr>& nodes() const { return m_nodes; }

        private:
            Array<NodeTemplatePtr> m_nodes;
        };

        //---

    } // game
} // base