/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
*
***/

#include "build.h"
#include "sceneRuntime.h"
#include "sceneRuntimeSystem.h"
#include "sceneElement.h"
#include "sceneDebugRenderingSystem.h"

namespace scene
{

    ///---

    RTTI_BEGIN_TYPE_CLASS(DebugRenderingSystem);
        RTTI_METADATA(scene::RuntimeSystemInitializationOrderMetadata).order(0);
    RTTI_END_TYPE();

    DebugRenderingSystem::DebugRenderingSystem()
    {}

    DebugRenderingSystem::~DebugRenderingSystem()
    {}

    void DebugRenderingSystem::registerElementForDebugRendering(Element* elem)
    {
        m_debugElements.insert(elem);
    }

    void DebugRenderingSystem::unregisterElementForDebugRendering(Element* elem)
    {
        m_debugElements.remove(elem);
    }

    void DebugRenderingSystem::onRenderFrame(Scene& scene, rendering::scene::FrameInfo& info)
    {
        for (auto elem  : m_debugElements.keys())
            elem->handleDebugRender(info);
    }

    ///---

} // scene

