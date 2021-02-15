/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"
#include "gameWorld.h"
#include "gameLoadingWorld.h"

#include "base/world/include/worldCompiledScene.h"
#include "base/world/include/worldStreamingSystem.h"
#include "base/world/include/world.h"

namespace game
{
    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(LoadingWorld);
    RTTI_END_TYPE();

    LoadingWorld::LoadingWorld(base::res::ResourcePath path)
        : m_path(path)
    {
        m_world = RefNew<World>();

        auto self = RefPtr<LoadingWorld>(AddRef(this));
        RunFiber("LoadScene") << [self](FIBER_FUNC)
        {
            self->processInternalLoading();
        };
    }

    LoadingWorld::~LoadingWorld()
    {
        m_world.reset();
    }

    WorldPtr LoadingWorld::acquireLoadedWorld()
    {
        DEBUG_CHECK_RETURN_EX_V(!m_loading, "Still loading", nullptr);
            
        auto world = std::move(m_world);
        return world;
    }
    
    void LoadingWorld::processInternalLoading()
    {
        TRACE_INFO("Started loading compiled scene '{}'", m_path);

        auto content = LoadResource<base::world::CompiledScene>(m_path.view()).acquire();
        if (content)
        {
            if (auto system = m_world->system<base::world::StreamingSystem>())
            {
                system->bindScene(content);
            }
        }
        else
        {
            TRACE_ERROR("Failed to load compiled scene '{}'", m_path);
        }

        m_loading = false;
    }

    //--

} // game


