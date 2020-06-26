/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
*
***/

#pragma once

#include "sceneRuntimeSystem.h"
#include "base/containers/include/hashSet.h"

namespace scene
{

    //---

    // entity collection
    class SCENE_COMMON_API DebugRenderingSystem : public IRuntimeSystem
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DebugRenderingSystem, IRuntimeSystem);

    public:
        DebugRenderingSystem();
        virtual ~DebugRenderingSystem();

        void registerElementForDebugRendering(Element* elem);
        void unregisterElementForDebugRendering(Element* elem);

    protected:
        virtual void onRenderFrame(Scene& scene, rendering::scene::FrameInfo& info) override final;

        base::HashSet<Element*> m_debugElements;
    };

    //---

} // scene
