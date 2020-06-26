/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
*
***/

#include "build.h"
#include "sceneRuntime.h"
#include "sceneRuntimeSystem.h"

namespace scene
{

    //---

    RuntimeSystemInitializationOrderMetadata::RuntimeSystemInitializationOrderMetadata()
    {}

    RuntimeSystemInitializationOrderMetadata::~RuntimeSystemInitializationOrderMetadata()
    {}

    RTTI_BEGIN_TYPE_CLASS(RuntimeSystemInitializationOrderMetadata);
    RTTI_END_TYPE();

    //---

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IRuntimeSystem);
    RTTI_END_TYPE();

    IRuntimeSystem::IRuntimeSystem()
    {}

    IRuntimeSystem::~IRuntimeSystem()
    {}

    bool IRuntimeSystem::checkCompatiblity(SceneType type) const
    {
        return true;
    }

    bool IRuntimeSystem::onInitialize(Scene& scene)
    {
        return true;
    }

    void IRuntimeSystem::onShutdown(Scene& scene)
    {}

    void IRuntimeSystem::onPreTick(Scene& scene, const UpdateContext& ctx)
    {}

    void IRuntimeSystem::onTick(Scene& scene, const UpdateContext& ctx)
    {}

    void IRuntimeSystem::onPreTransform(Scene& scene, const UpdateContext& ctx)
    {}

    void IRuntimeSystem::onTransform(Scene& scene, const UpdateContext& ctx)
    {}

    void IRuntimeSystem::onPostTransform(Scene& scene, const UpdateContext& ctx)
    {}

    void IRuntimeSystem::onRenderFrame(Scene& scene, rendering::scene::FrameInfo& info)
    {}

    void IRuntimeSystem::onWorldContentAttached(Scene& scene, const WorldPtr& worldPtr)
    {}

    void IRuntimeSystem::onWorldContentDetached(Scene& scene, const WorldPtr& worldPtr)
    {}

    ///---

} // scene

