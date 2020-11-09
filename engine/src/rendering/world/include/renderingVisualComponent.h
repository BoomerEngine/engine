/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: components\mesh #]
*
***/

#pragma once

#include "base/world/include/worldComponent.h"
#include "rendering/scene/include/renderingSelectable.h"

namespace rendering
{
    //--

    // generic visual component
    class RENDERING_WORLD_API IVisualComponent : public base::world::Component
    {
        RTTI_DECLARE_VIRTUAL_CLASS(IVisualComponent, base::world::Component);

    public:
        IVisualComponent();

        ///--

        // get selection owner
        INLINE const scene::Selectable& selectable() const { return m_selectable; }

        // bind selectable
        void bindSelectable(const scene::Selectable& data);

        ///--

    protected:
        scene::ProxyHandle m_proxy;
        scene::Selectable m_selectable;

        virtual scene::ProxyHandle handleCreateProxy(scene::Scene* scene) const = 0;

        virtual void handleAttach(base::world::World* scene) override;
        virtual void handleDetach(base::world::World* scene) override;
        virtual void handleTransformUpdate(const Component* source, const base::AbsoluteTransform& parentTransform, const base::Matrix& parentToWorld) override;
        virtual void handleSelectionHighlightChanged() override;

        void recreateRenderingProxy();

    private:
        void createRenderingProxy();
        void destroyRenderingProxy();
    };

    //--

} // rendering
