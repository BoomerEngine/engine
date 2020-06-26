/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
*
***/

#include "build.h"
#include "sceneDebugBoxComponent.h"
#include "sceneDebugRenderingSystem.h"
#include "sceneRuntime.h"

#include "rendering/scene/include/renderingFrameGeometryCanvas.h"

namespace scene
{
    //--

    RTTI_BEGIN_TYPE_CLASS(DebugBoxComponent);
    RTTI_END_TYPE();

    DebugBoxComponent::DebugBoxComponent()
        : m_width(0.2f)
        , m_height(0.2f)
        , m_depth(0.2f)
        , m_standing(false)
        , m_solid(true)
        , m_lineColor(0,0,0,255)
        , m_fillColor(60,60,60,255)
    {}

    void DebugBoxComponent::handleDebugRender(rendering::scene::FrameInfo& frame) const
    {
        TBaseClass::handleDebugRender(frame);

        rendering::scene::GeometryCanvas dd(frame, localToWorld());

        base::Vector3 boxMin, boxMax;
        boxMin.x = -m_width * 0.5f;
        boxMin.y = -m_depth * 0.5f;
        boxMax.x = m_width * 0.5f;
        boxMax.y = m_depth * 0.5f;

        if (m_standing)
        {
            boxMin.z = 0.0f;
            boxMax.z = m_height;
        }
        else
        {
            boxMin.z = -m_height * 0.5f;
            boxMax.z = m_height * 0.5f;
        }

        if (m_solid && m_fillColor.a > 0)
        {
            if (m_fillColor.a < 255)
                dd.mode(rendering::scene::FrameGeometryRenderMode::AlphaBlend);
            else
                dd.mode(rendering::scene::FrameGeometryRenderMode::Solid);

            dd.fillColor(m_fillColor);
            dd.box(boxMin, boxMax, true);
        }

        if (m_lineColor.a > 0)
        {
            dd.lineColor(m_lineColor);
            dd.mode(rendering::scene::FrameGeometryRenderMode::Solid);
            dd.box(boxMin, boxMax, false);
        }
    }

    void DebugBoxComponent::handleSceneAttach(Scene* scene)
    {
        TBaseClass::handleSceneAttach(scene);

        if (auto s = scene->system<DebugRenderingSystem>())
            s->registerElementForDebugRendering(this);
    }

    void DebugBoxComponent::handleSceneDetach(Scene* scene)
    {
        TBaseClass::handleSceneDetach(scene);

        if (auto s = scene->system<DebugRenderingSystem>())
            s->unregisterElementForDebugRendering(this);
    }

    //--

} // scene