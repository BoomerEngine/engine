/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world #]
***/

#include "build.h"
#include "sceneEntity.h"
#include "sceneWorld.h"
#include "sceneWorldStreamingSystem.h"

#include "base/resources/include/resourcePath.h"
#include "scene/common/include/sceneRuntime.h"

namespace scene
{

    //---

     //--

    struct WorldSectorLoadingState
    {
        base::res::ResourceKey m_path;
        base::RefPtr<WorldSector> m_loadedData;
        volatile bool m_finished;
        volatile bool m_canceled;

        INLINE WorldSectorLoadingState(const base::res::ResourceKey& path)
            : m_path(path)
            , m_finished(false)
            , m_canceled(false)
        {}

        void process()
        {
            if (!m_canceled)
                m_loadedData = base::LoadResource<WorldSector>(m_path.path());
            m_finished = true;
        }
    };

    //--

    WorldSectorStreamer::WorldSectorStreamer(const WorldSectorDesc& desc, const base::res::ResourcePath& worldPath)
        : m_streamingBox(desc.m_streamingBox)
        , m_name(desc.m_name)
    {
        if (desc.m_unsavedSectorData)
        {
            m_persistentData = desc.m_unsavedSectorData;
        }
        else if (!worldPath.empty())
        {
            // assemble path to the whole sector
            auto worldRootPath = worldPath.path().beforeLast("/");
            auto sectorFileExtension = base::res::IResource::GetResourceExtensionForClass(WorldSector::GetStaticClass());
            auto wholeSectorPath = base::res::ResourcePath(base::TempString("{}/{}.{}", worldRootPath, desc.m_name, sectorFileExtension).c_str());
            m_dataHandle.set(base::res::ResourceKey(wholeSectorPath, scene::WorldSector::GetStaticClass()));
        }
    }

    WorldSectorStreamer::~WorldSectorStreamer()
    {}

    void WorldSectorStreamer::requestLoad()
    {
        if (m_persistentData)
        {
            m_loadedData = m_persistentData;
        }
        else
        {
            ASSERT_EX(!m_loadingState, "Sector already loading");
            ASSERT_EX(!m_loadedData, "Sector already loaded");

            // create loading state
            m_loadingState = base::CreateSharedPtr<WorldSectorLoadingState>(m_dataHandle.key());

            // schedule loading state
            auto loadingState = m_loadingState;
            RunFiber("LoadCompiledSector") << [loadingState](FIBER_FUNC)
            {
                loadingState->process();
            };
        }
    }

    void WorldSectorStreamer::requestUnload(Scene& scene)
    {
        // unregister all proxies the sector had
        if (m_loadedData)
        {
            for (auto& ent : m_loadedData->m_entities)
                ent->detachFromScene();
            m_loadedData.reset();
        }

        // cancel any pending load
        if (m_loadingState)
        {
            m_loadingState->m_canceled = true;
            m_loadingState.reset();
        }
    }

    bool WorldSectorStreamer::updateLoadingState(Scene& scene)
    {
        if (m_persistentData)
        {
            for (auto& ent : m_loadedData->m_entities)
                ent->attachToScene(&scene);

            return true;
        }

        ASSERT_EX(!m_loadingState || !m_loadedData, "Sector was not in the loading state");

        // done loading ?
        if (m_loadingState && m_loadingState->m_finished)
        {
            // finish the loading and extract the data
            if (m_loadingState->m_loadedData)
            {
                m_loadedData = m_loadingState->m_loadedData;
                TRACE_INFO("Loaded sector '{}'", m_dataHandle.key());

                for (auto& ent : m_loadedData->m_entities)
                    ent->attachToScene(&scene);
            }
            else
            {
                TRACE_WARNING("Failed to load data from sector '{}'", m_dataHandle.key());
            }

            // release the loading token to mark that the loading phase has finished
            m_loadingState.reset();
            return true;
        }

        // still not loaded, no state change
        return false;
    }

    //---
        
    RTTI_BEGIN_TYPE_CLASS(WorldStreamingSystem);
        RTTI_METADATA(scene::RuntimeSystemInitializationOrderMetadata).order(50);
    RTTI_END_TYPE();

    WorldStreamingSystem::WorldStreamingSystem()
    {
    }

    WorldStreamingSystem::~WorldStreamingSystem()
    {}
    
    bool WorldStreamingSystem::checkCompatiblity(SceneType type) const
    {
        return (type == SceneType::Game);
    }

    bool WorldStreamingSystem::onInitialize(Scene& scene)
    {
        return true;
    }

    bool WorldStreamingSystem::isLoadingContent() const
    {
        for (auto world  : m_worlds)
            if (!world->m_loadingSectors.empty())
                return true;

        return false;
    }

    bool WorldStreamingSystem::isLoadingContentAround(const base::AbsolutePosition& pos) const
    {
        return isLoadingContent(); // TODO!
    }

    void WorldStreamingSystem::updateStreaming(Scene& scene, const base::AbsolutePosition* observers, uint32_t numObservers)
    {
        for (auto world  : m_worlds)
            updateStreaming(scene, world, observers, numObservers);
    }

    bool WorldStreamingSystem::CheckVisibility(const base::Box& streamingBox, const base::AbsolutePosition* observers, uint32_t numObservers)
    {
        for (uint32_t i = 0; i < numObservers; ++i)
        {
            if (streamingBox.contains(observers[i].approximate()))
                return true;
        }
        return false;
    }

    void WorldStreamingSystem::updateStreaming(Scene& scene, WorldInfo* world, const base::AbsolutePosition* observers, uint32_t numObservers)
    {
        PC_SCOPE_LVL0(UpdateWorldStreaming);

        // collect new sectors to load
        for (auto sector  : world->m_allSectors)
        {
            auto shouldStream = CheckVisibility(sector->streamingBox(), observers, numObservers);

            if (shouldStream)
            {
                if (sector->isLoaded())
                {
                    // do nothing, already loaded
                }
                else
                {
                    if (!sector->isLoading())
                    {
                        sector->requestLoad();
                        world->m_loadingSectors.insert(sector);
                    }

                    if (sector->updateLoadingState(scene))
                    {
                        world->m_loadedSectors.insert(sector);
                        world->m_loadingSectors.remove(sector);
                    }
                }
            }
            else
            {
                if (sector->isLoaded() || sector->isLoading())
                {
                    world->m_loadingSectors.remove(sector);
                    world->m_loadedSectors.remove(sector);
                    sector->requestUnload(scene);
                }
            }
        }
    }

    void WorldStreamingSystem::onPreTick(Scene& scene, const UpdateContext& ctx)
    {
        base::InplaceArray<base::AbsolutePosition, 10> observers;

        for (auto& info : scene.observers())
            observers.pushBack(info.m_position);

        if (observers.empty())
            observers.pushBack(base::AbsolutePosition()); // stream around the center of scene (0,0,0), this is usually a good default state


        updateStreaming(scene, observers.typedData(), observers.size());
    }

    void WorldStreamingSystem::onRenderFrame(Scene& scene, rendering::scene::FrameInfo& info)
    {
    }

    void WorldStreamingSystem::onShutdown(Scene& scene)
    {
        ASSERT_EX(m_worlds.empty(), "World still attached when streaming system is destroyed");
    }

    void WorldStreamingSystem::onWorldContentAttached(Scene& scene, const WorldPtr& worldPtr)
    {
        auto worldInfo  = MemNew(WorldInfo);
        worldInfo->m_world = worldPtr;
        m_worlds.pushBack(worldInfo);

        if (auto compiledWorld = base::rtti_cast<CompiledWorld>(worldPtr))
        {
            auto numSectors = compiledWorld->sectors().size();
            auto sectorTable  = (WorldSectorStreamer*)MemAlloc(POOL_STREMAING, sizeof(WorldSectorStreamer) * numSectors, alignof(WorldSectorStreamer));
            for (auto& sector : compiledWorld->sectors())
            {
                auto info  = new (sectorTable++) WorldSectorStreamer(sector, worldPtr->path());
                worldInfo->m_allSectors.pushBack(info);
            }
        }
    }

    void WorldStreamingSystem::onWorldContentDetached(Scene& scene, const WorldPtr& worldPtr)
    {
        for (auto info  : m_worlds)
        {
            if (info->m_world == worldPtr)
            {
                // detach sectors that are loaded (and thus attached)
                for (auto sector  : info->m_loadedSectors.keys())
                    sector->requestUnload(scene);
                info->m_loadedSectors.clear();

                // stop loading on rest of the sectors
                for (auto sector  : info->m_loadingSectors.keys())
                    sector->requestUnload(scene);
                info->m_loadingSectors.clear();

                // cleanup objects
                for (auto sector  : info->m_allSectors)
                    sector->~WorldSectorStreamer();
                MemFree(info->m_allSectors.front());

                m_worlds.remove(info);
                MemDelete(info);
                break;
            }
        }

    }


} // scene

