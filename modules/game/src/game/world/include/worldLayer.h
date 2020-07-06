/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\data #]
***/

#pragma once

#include "base/resources/include/resource.h"

namespace game
{
    //---

    /// Uncooked layer in the world, basic building block and source of most entities
    /// Layer is composed of a list of node templates
    class GAME_WORLD_API WorldLayer : public base::res::IResource
    {
        RTTI_DECLARE_VIRTUAL_CLASS(WorldLayer, base::res::IResource);

    public:
        WorldLayer();
        virtual ~WorldLayer();

        /// get node container
        INLINE const NodeTemplateContainerPtr& nodeContainer() const { return m_container; }

        /// set new content of layer
        void content(const NodeTemplateContainerPtr& container);

    private:
        NodeTemplateContainerPtr m_container;
        base::Array<NodeTemplatePtr> m_nodes;

        virtual void onPostLoad() override;
    };

    //---

} // game