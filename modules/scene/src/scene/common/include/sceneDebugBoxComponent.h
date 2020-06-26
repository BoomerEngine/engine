/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
*
***/

#pragma once

#include "sceneComponent.h"

namespace scene
{
    //---

    // a basic runtime component
    class SCENE_COMMON_API DebugBoxComponent : public Component
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DebugBoxComponent, Component);

    public:
        DebugBoxComponent();

        float m_width;
        float m_height;
        float m_depth;
        bool m_standing;
        bool m_solid;
        
        base::Color m_lineColor;
        base::Color m_fillColor;

    protected:
        virtual void handleDebugRender(rendering::scene::FrameInfo& frame) const override;
        virtual void handleSceneAttach(Scene* scene) override;
        virtual void handleSceneDetach(Scene* scene) override;
    };

    //---

} // scene
