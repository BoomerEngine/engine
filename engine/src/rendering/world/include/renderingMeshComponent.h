/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: components\mesh #]
*
***/

#pragma once

#include "renderingVisualComponent.h"

namespace rendering
{

    //--

    // a scene node capable of rendering a mesh
    class RENDERING_WORLD_API MeshComponent : public IVisualComponent
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshComponent, IVisualComponent);

    public:
        MeshComponent();
        MeshComponent(const MeshPtr& mesh);
        virtual ~MeshComponent();

        INLINE const MeshRef& mesh() const { return m_mesh; }

        INLINE bool castingShadows() const { return m_castShadows; }
        INLINE bool receivingShadows() const { return m_receiveShadows; }

        INLINE char forcedDetailLevel() const { return m_forceDetailLevel; }
        INLINE uint16_t forcedDistance() const { return m_forceDistance; }
        INLINE base::Color color() const { return m_color; }
        INLINE base::Color colorEx() const { return m_colorEx; }

        ///--

        void castingShadows(bool flag);
        void receivingShadows(bool flag);

        void forceDetailLevel(char level);
        void forceDistance(float distance);

        void color(base::Color color);
        void colorEx(base::Color color);

        void mesh(const rendering::MeshPtr& mesh);

        ///--

        virtual base::Box calcBounds() const override;

    protected:
        MeshRef m_mesh;
        
        base::Color m_color = base::Color::WHITE;
        base::Color m_colorEx = base::Color::BLACK;

        bool m_castShadows = true;
        bool m_receiveShadows = true;
        char m_forceDetailLevel = 0;
        uint16_t m_forceDistance = 0;

        //--

        virtual scene::ObjectProxyPtr handleCreateProxy(scene::Scene* scene) const override;

        virtual void queryTemplateProperties(base::ITemplatePropertyBuilder& outTemplateProperties) const override;
        virtual bool initializeFromTemplateProperties(const base::ITemplatePropertyValueContainer& templateProperties) override;        
    };

    //--

} // rendering
