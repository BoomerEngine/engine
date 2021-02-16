/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\streaming #]
*
***/

#include "build.h"
#include "worldStreamingIsland.h"
#include "worldStreamingSector.h"
#include "worldStreamingSystem.h"
#include "worldCompiledScene.h"

namespace base
{
    namespace world
    {
        ///---

        RTTI_BEGIN_TYPE_CLASS(StreamingObserver);
        RTTI_END_TYPE();

        StreamingObserver::StreamingObserver()
        {}

        ///---

        RTTI_BEGIN_TYPE_CLASS(StreamingSystem);
        RTTI_END_TYPE();

        StreamingSystem::StreamingSystem()
        {}

        StreamingSystem::~StreamingSystem()
        {}

        void StreamingSystem::unbindEntities()
        {
            for (const auto& island : m_islands)
            {
                island->loading.reset();

                if (island->attached)
                {
                    island->attached->detach(world());
                    island->attached.reset();
                }
            }

            if (m_compiledScene)
            {
                // TODO: unbind always loaded entities
            }

            m_compiledScene.reset();
        }

        void StreamingSystem::handleShutdown()
        {
            TBaseClass::handleShutdown();
            unbindEntities();
        }

        void StreamingSystem::bindScene(CompiledScene* scene)
        {
            if (m_compiledScene != scene)
            {
                unbindEntities();

                m_sectors.clear();

                m_compiledScene = AddRef(scene);

                if (m_compiledScene)
                {
                    for (const auto& cell : m_compiledScene->streamingCells())
                    {
                        auto sector = RefNew<SectorState>();
                        sector->data = cell.data;
                        sector->streamingBox = cell.streamingBox;
                    }
                }
            }
        }

        void StreamingSystem::attachObserver(StreamingObserver* observer)
        {
            DEBUG_CHECK_RETURN_EX(observer, "Invalid observer");
            DEBUG_CHECK_RETURN_EX(!m_observers.contains(observer), "Observer already registered");
            m_observers.pushBack(AddRef(observer));
        }

        void StreamingSystem::dettachObserver(StreamingObserver* observer)
        {
            DEBUG_CHECK_RETURN_EX(observer, "Invalid observer");
            DEBUG_CHECK_RETURN_EX(m_observers.contains(observer), "Observer not registered");
            m_observers.remove(observer);
        }

        ///---

        void StreamingSystem::extractSectorIslands(SectorState* sector, StreamingIsland* island, IslandState* parentIsland)
        {
            auto islandState = RefNew<IslandState>();
            islandState->streamingBox = island->streamingBounds();
            islandState->data = AddRef(island);
            sector->loadedIslands.pushBack(islandState);

            for (const auto& child : island->children())
                extractSectorIslands(sector, child, islandState);
        }

        void StreamingSystem::updateSectorStates()
        {
            for (const auto& sector : m_sectors)
            {
                bool shouldBeVisible = false;

                for (const auto& observer : m_observers)
                {
                    if (sector->streamingBox.contains(observer->position()))
                    {
                        shouldBeVisible = true;
                        break;
                    }
                }

                if (shouldBeVisible)
                {
                    if (!sector->loading)
                    {
                        auto loading = RefNew<SectorLoading>();

                        loading->data = sector->data;
                        sector->loading = loading;
                        
                        RunFiber("LoadSector") << [loading](FIBER_FUNC)
                        {
                            loading->loadedData = loading->data.load<StreamingSector>();
                            loading->finished = true;
                        };
                    }

                    if (sector->loading->finished)
                    {
                        if (sector->loadedIslands.empty() && sector->loading->loadedData)
                        {
                            for (const auto& rootIsland : sector->loading->loadedData->rootIslands())
                                extractSectorIslands(sector, rootIsland, nullptr);
                        }
                    }
                }
                else
                {
                    for (const auto& island : sector->loadedIslands)
                    {
                        island->loading.reset();

                        if (island->attached)
                        {
                            island->attached->detach(world());
                            island->attached.reset();
                        }

                        m_islands.remove(island);
                    }

                    sector->loading.reset();
                    sector->loadedIslands.clear();
                }
            }
        }

        //--

        void StreamingSystem::updateIslandStates()
        {
            for (const auto& island : m_islands)
            {
                bool shouldBeVisible = false;

                for (const auto& observer : m_observers)
                {
                    if (island->streamingBox.contains(observer->position()))
                    {
                        shouldBeVisible = true;
                        break;
                    }
                }

                if (shouldBeVisible)
                {
                    if (!island->loading)
                    {
                        auto loading = RefNew<IslandLoading>();

                        loading->data = island->data;
                        island->loading = loading;

                        RunFiber("LoadIsland") << [loading](FIBER_FUNC)
                        {
                            loading->loadedData = loading->data->load(GlobalLoader());
                            loading->finished = true;
                        };
                    }

                    if (island->loading->finished)
                    {
                        if (island->attached)
                        {
                            island->attached = island->loading->loadedData;

                            if (island->attached)
                                island->attached->attach(world());
                        }
                    }
                }
                else 
                {
                    if (island->attached)
                    {
                        island->attached->detach(world());
                        island->attached.reset();
                    }

                    island->loading.reset();
                }
            }
        }

        //--

        void StreamingSystem::handlePreTick(double dt)
        {
            PC_SCOPE_LVL0(StreamingSystem);

            updateSectorStates();
            updateIslandStates();
        }

        ///---

    } // world
} // game


