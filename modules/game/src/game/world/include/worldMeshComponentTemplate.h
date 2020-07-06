/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entity\components\mesh #]
*
***/

#pragma once

#include "worldComponent.h"
#include "worldComponentTemplate.h"
#include "rendering/scene/include/public.h"

namespace game
{
    //--

    // a data template for creating mesh component, usually editor only
    class GAME_WORLD_API MeshComponentTemplate : public ComponentTemplate
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshComponentTemplate, ComponentTemplate);

    public:
        MeshComponentTemplate();

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
        virtual ComponentPtr createComponent() const override;
        virtual void applyOverrides(Component* c) const override;
    };

    //--

} // game
