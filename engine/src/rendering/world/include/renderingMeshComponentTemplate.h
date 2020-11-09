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
#include "base/world/include/worldComponentTemplate.h"
#include "rendering/scene/include/public.h"

namespace rendering
{
    //--

    // a data template for creating mesh component, usually editor only
    class RENDERING_WORLD_API MeshComponentTemplate : public base::world::ComponentTemplate
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshComponentTemplate, base::world::ComponentTemplate);

    public:
        MeshComponentTemplate();

        //--

        INLINE const rendering::MeshAsyncRef& mesh() const { return m_mesh; }

        INLINE const base::Color& color() const { return m_color; }

        INLINE const base::Color& colorEx() const { return m_colorEx; }

        //--

        void mesh(const rendering::MeshAsyncRef& meshRef, bool makeOverride = true, bool callEvent = true);

        void color(const base::Color& color, bool makeOverride = true, bool callEvent = true);

        void colorEx(const base::Color& color, bool makeOverride = true, bool callEvent = true);

        //--

    protected:
        //--

        rendering::MeshAsyncRef m_mesh;

        base::Color m_color; // internal color, passed to material shaders to be used by various effects
        base::Color m_colorEx; // internal color, passed to material shaders to be used by various effects

        char m_forceDetailLevel = 0;
        float m_visibilityDistanceForced = 0.0f;
        float m_visibilityDistanceMultiplier = 1.0f;

        //--

        bool m_castShadows = true;
        bool m_receiveShadows = true;

        //--

        // ComponentTemplate
        virtual base::world::ComponentPtr createComponent() const override;
    };

    //--

} // game
