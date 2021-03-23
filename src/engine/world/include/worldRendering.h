/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world #]
***/

#pragma once

#include "worldSystem.h"
#include "engine/rendering/include/stats.h"

BEGIN_BOOMER_NAMESPACE()

//---

struct WorldRenderingContext;

/// world rendering system (an instance of the rendering scene)
class ENGINE_WORLD_API WorldRenderingSystem : public IWorldSystem
{
    RTTI_DECLARE_POOL(POOL_WORLD_SYSTEM)
    RTTI_DECLARE_VIRTUAL_CLASS(WorldRenderingSystem, IWorldSystem);

public:
    WorldRenderingSystem();
    virtual ~WorldRenderingSystem();
            
    //---

    INLINE rendering::Scene* scene() const { return m_scene; }

    INLINE const rendering::FrameStats& lastFrameStats() const { return m_lastStats; }

    INLINE const CameraSetup& lastCamera() const { return m_lastCamera; }

    //---

    void renderViewport(World* world, gpu::CommandWriter& cmd, const gpu::AcquiredOutput& output, const WorldRenderingContext& context) const;

    //--

protected:
    virtual bool handleInitialize(World& scene) override;
    virtual void handleShutdown() override;
    virtual void handleImGuiDebugInterface() override;

    rendering::ScenePtr m_scene = nullptr;

    mutable rendering::FrameStats m_lastStats;
    mutable CameraSetup m_lastCamera;
};

//---

END_BOOMER_NAMESPACE()
