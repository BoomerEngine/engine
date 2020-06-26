/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world #]
***/

#pragma once

#include "base/resources/include/resource.h"

namespace scene
{
    /// Uncooked layer in the world
    /// Layer is composed of a list of node templates
    class SCENE_COMMON_API Layer : public base::res::ITextResource
    {
        RTTI_DECLARE_VIRTUAL_CLASS(Layer, base::res::ITextResource);

    public:
        Layer();
        virtual ~Layer();

        /// get node container
        INLINE const NodeTemplateContainerPtr& nodeContainer() const { return m_container; }

        /// set new content of layer
        void content(const NodeTemplateContainerPtr& container);

    private:
        NodeTemplateContainerPtr m_container;
        base::Array<NodeTemplatePtr> m_nodes;

        virtual void onPostLoad() override;
    };

} // scene