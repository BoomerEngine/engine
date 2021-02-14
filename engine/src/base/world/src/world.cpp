/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world #]
*
***/

#include "build.h"
#include "world.h"
#include "worldSystem.h"
#include "worldEntity.h"
#include "worldPrefab.h"

#include "base/object/include/rttiClassType.h"
#include "base/containers/include/queue.h"
#include "worldNodePath.h"
#include "base/imgui/include/imgui.h"

namespace base
{
    namespace world
    {

        //---

        RTTI_BEGIN_TYPE_CLASS(WorldObserver);
            RTTI_PROPERTY(position);
            RTTI_PROPERTY(velocity);
            RTTI_PROPERTY(maxStreamingRange);
        RTTI_END_TYPE();

        //---

        RTTI_BEGIN_TYPE_NATIVE_CLASS(World);
        RTTI_END_TYPE();

        ///--

        World::World()
        {
            initializeSystems();
            m_transformRequestsMap.reserve(512);
        }

        World::~World()
        {
            cancelAllTransformRequests();
            destroyEntities();
            destroySystems();
        }

        struct ScopeBool : public NoCopy
        {
        public:
            ScopeBool(bool& flag)
                : m_flag(flag)
            {
                DEBUG_CHECK_EX(flag == false, "Flag expected to be false");
                flag = true;
            }

            ~ScopeBool()
            {
                DEBUG_CHECK_EX(m_flag == true, "Flag expected to be true");
                m_flag = false;
            }

        private:
            bool& m_flag;
        };

        void CAN_YIELD World::update(double dt)
        {
            PC_SCOPE_LVL1(WorldUpdate);

            ScopeBool flag(m_protectedEntityRegion);

            WorldStats stats;
            stats.numEntites = m_entities.size();

            updatePreTick(dt, stats);

            {
                ScopeTimer timer;
                updateSystems(dt, stats);
                stats.mainTickTime = timer.timeElapsed();
            }

            {
                updatePostTick(dt, stats);
            }

            m_stats = stats;
        }

        void World::updatePreTick(double dt, WorldStats& outStats)
        {
            PC_SCOPE_LVL1(PreTick);
        
            {
                PC_SCOPE_LVL1(PreTickSystems);

                ScopeTimer timer;
                for (const auto& sys : m_systems)
                    sys->handlePreTick(dt);
                outStats.preTickSystemTime = timer.timeElapsed();
            }

            {
                PC_SCOPE_LVL1(PreTickEntitesMain);

                ScopeTimer timer;
                for (const auto& ent : m_entities.keys())
                    ent->handlePreTick(dt);

                outStats.preTickEntityTime = timer.timeElapsed();
                outStats.numPreTickEntites = m_entities.size();
            }

            serviceTransformRequests(outStats);

            {
                PC_SCOPE_LVL1(PreTickFixup);
                ScopeTimer timer;

                for (;;)
                {
                    InplaceArray<Entity*, 64> newEntities;
                    applyEntityChanges(&newEntities, outStats);
                    if (newEntities.empty())
                        break;

                    serviceTransformRequests(outStats);

                    for (auto* ent : newEntities)
                        ent->handlePreTick(dt);

                    outStats.numPreTickLoops += 1;
                }

                outStats.preTickFixupTime = timer.timeElapsed();
            }
        }

        void World::updateSystems(double dt, WorldStats& outStats)
        {
            PC_SCOPE_LVL1(SystemUpdate);

            ScopeTimer timer;

            for (const auto& sys : m_systems)
                sys->handleMainTickStart(dt);

            for (const auto& sys : m_systems)
                sys->handleMainTickFinish(dt);

            for (const auto& sys : m_systems)
                sys->handleMainTickPublish(dt);

            outStats.mainTickTime = timer.timeElapsed();
        }

        void World::updatePostTick(double dt, WorldStats& outStats)
        {
            PC_SCOPE_LVL1(PostTick);

            {
                PC_SCOPE_LVL1(PostTickSystems);

                ScopeTimer timer;
                for (const auto& ent : m_entities.keys())
                    ent->handlePostTick(dt);

                outStats.numPostTickEntites = m_entities.size();
                outStats.postTickEntityTime = timer.timeElapsed();
            }

            serviceTransformRequests(outStats);

            {
                PC_SCOPE_LVL1(PostTickEntities);

                ScopeTimer timer;
                for (const auto& sys : m_systems)
                    sys->handlePostTick(dt);

                outStats.postTickSystemTime = timer.timeElapsed();
            }

            serviceTransformRequests(outStats); // last update before render
        }

        void World::destroyEntities()
        {
            DEBUG_CHECK(m_removedEntities.empty());
            DEBUG_CHECK(m_addedEntities.empty());

            auto entities = std::move(m_entities);
            for (const auto& ent : entities)
                ent->detachFromWorld(this);
        }

        void World::applyEntityChanges(Array<Entity*>* outCreatedEntities, WorldStats& outStats)
        {
            while (!m_removedEntities.empty() || !m_addedEntities.empty())
            {
                auto removedList = std::move(m_removedEntities);
                auto addedList = std::move(m_addedEntities);

                outStats.numEntitiesAttached += addedList.size();
                outStats.numEntitiesDetached += removedList.size();

                for (const auto& removed : removedList)
                {
                    if (removed->attached())
                        removed->detachFromWorld(this);
                    m_entities.remove(removed);

                    if (outCreatedEntities)
                        outCreatedEntities->remove(removed);
                }

                for (const auto& added : addedList)
                {
                    DEBUG_CHECK(!added->attached());
                    added->attachToWorld(this);
                    m_entities.insert(added);

                    if (outCreatedEntities)
                        outCreatedEntities->pushBack(added);
                }
            }
        }

        //--

        void World::attachEntity(Entity* entity)
        {
            DEBUG_CHECK_EX(entity, "Invalid entity");
            if (entity)
            {
                ASSERT_EX(!entity->attached(), "Entity already attached");
                ASSERT_EX(entity->world() == nullptr, "Entity already owned by some world");
                ASSERT_EX(!m_entities.contains(entity), "Entity already registered");

                entity->handleTransformUpdate(entity->absoluteTransform()); // calculate initial positions

                if (m_protectedEntityRegion)
                {
                    m_removedEntities.remove(entity);
                    m_addedEntities.pushBackUnique(AddRef(entity));
                }
                else
                {
                    m_entities.insert(AddRef(entity));
                    entity->attachToWorld(this);
                }
            }
        }

        void World::detachEntity(Entity* entity)
        {
            DEBUG_CHECK_EX(entity, "Invalid entity");
            if (entity)
            {
                ASSERT_EX(entity->attached(), "Entity not attached");
                ASSERT_EX(entity->world() == this, "Entity not owned by this world");
                ASSERT_EX(m_entities.contains(entity), "Entity not registered");

                if (m_protectedEntityRegion)
                {
                    m_removedEntities.pushBackUnique(AddRef(entity));
                    m_addedEntities.remove(entity);
                }
                else
                {
                    entity->detachFromWorld(this);
                    m_entities.remove(entity);
                }
            }
        }

        //--

        EntityPtr World::createPrefabInstance(const AbsoluteTransform& placement, const Prefab* prefab, StringID appearance)
        {
            DEBUG_CHECK_RETURN_V(prefab, nullptr);

            const auto& rootNode = prefab->root();
            DEBUG_CHECK_RETURN_V(rootNode, nullptr);

            NodePathBuilder path;
            if (const auto root = CompileEntityHierarchy(path, rootNode, &placement, GlobalLoader()))
            {
                InplaceArray<EntityPtr, 10> allEntities;
                root->collectEntities(allEntities);

                if (!allEntities.empty())
                {
                    for (const auto& entity : allEntities)
                        attachEntity(entity);

                    return allEntities[0];
                }
            }

            TRACE_WARNING("Failed to create entity from prefab '{}'", prefab->path());
            return nullptr;
        }

        //--

        void World::render(rendering::scene::FrameParams& info)
        {
            PC_SCOPE_LVL1(RenderWorldFrame);

            // render all system stuff
            for (auto &sys : m_systems)
                sys->handleRendering(info);

            // allow entities to debug render as well
            for (auto& ent : m_entities)
                ent->handleDebugRender(info);
        }

        //--

        void World::destroySystems()
        {
            // shutdown systems
            for (auto i = m_systems.lastValidIndex(); i >= 0; --i)
                m_systems[i]->handleShutdown();

            // close all systems
            for (auto& system : m_systems)
                system.reset();
            m_systems.clear();
        }

        //--

        namespace WorldSystemCreator
        {
            struct Info : public NoCopy
            {
                RTTI_DECLARE_POOL(POOL_TEMP)

            public:
                SpecificClassType<IWorldSystem> m_class = nullptr;
                Array<Info*> m_dependsOnInit;
                Array<Info*> m_tickBefore;
                Array<Info*> m_tickAfter;

                Array<Info*> m_adj; // temp

                RefPtr<IWorldSystem> m_system;

                int m_depth = 0;

            };

            typedef HashMap<SpecificClassType<IWorldSystem>, Info*> Graph;

            static Info* CreateClassEntry(SpecificClassType<IWorldSystem> classType, Graph& table)
            {
                Info* info = nullptr;
                if (table.find(classType, info))
                    return info;

                // map entry
                info = new Info;
                info->m_class = classType;
                table[classType] = info;

                // extract dependency list
                auto classList = classType->findMetadata<DependsOnWorldSystemMetadata>();
                if (classList)
                {
                    for (auto refClassType : classList->classes())
                        info->m_dependsOnInit.pushBack(CreateClassEntry(refClassType, table));
                }

                // TODO: extract other types of dependencies

                return info;
            }

            void ExtractServiceClasses(Graph& outClasses)
            {
                // enumerate service classes
                Array<SpecificClassType<IWorldSystem>> serviceClasses;
                RTTI::GetInstance().enumClasses(serviceClasses);
                TRACE_INFO("Found {} world system classes", serviceClasses.size());

                // assign indices
                static short numSceneSystems = 0;
                if (0 == numSceneSystems)
                {
                    for (auto subSystemClass : serviceClasses)
                    {
                        subSystemClass->assignUserIndex(numSceneSystems);
                        numSceneSystems += 1;
                    }
                }

                // create entries
                // NOTE: this may recurse a lot
                outClasses.reserve(serviceClasses.size());
                for (auto serviceClass : serviceClasses)
                    CreateClassEntry(serviceClass, outClasses);
            }

            typedef Array<Info*> ServiceOrderTable;

            bool TopologicalSort(const Graph& graph, ServiceOrderTable& outOrder)
            {
                // reset depth
                for (auto service : graph.values())
                    service->m_depth = 0;

                // traverse adjacency lists to fill the initial depths
                for (auto service : graph.values())
                    for (auto dep : service->m_adj)
                        dep->m_depth += 1;

                // create a queue and add all not referenced crap
                Queue<Info*> q;
                for (auto service : graph.values())
                    if (service->m_depth == 0)
                        q.push(service);

                // process vertices
                outOrder.clear();
                while (!q.empty())
                {
                    // pop and add to order
                    auto p = q.top();
                    q.pop();
                    outOrder.pushBack(p);

                    // reduce depth counts, if they become zero we are ready to dispatch
                    for (auto dep : p->m_adj)
                        if (0 == --dep->m_depth)
                            q.push(dep);
                }

                // swap order
                for (uint32_t i = 0; i < outOrder.size() / 2; ++i)
                    std::swap(outOrder[i], outOrder[outOrder.size() - 1 - i]);

                // we must have the same count, if not, there's a cycle
                return outOrder.size() == graph.size();
            }

            bool GenerateInitializationOrder(const Graph& graph, ServiceOrderTable& outTable)
            {
                // create the adjacency from the "dependsOn" part
                for (auto service : graph.values())
                    service->m_adj = service->m_dependsOnInit;

                // generate topology
                if (!TopologicalSort(graph, outTable))
                {
                    TRACE_ERROR("Unable to sort the service nodes");
                    //PrintGraph(graph);
                    //PrintTopology(graph, outTable);
                    return false;
                }

                // print the table
                //TRACE_SPAM("Found order for service initialization:");
                //PrintTopology(graph, outTable);
                return true;
            }

            bool GenerateTickingOrder(const Graph& graph, ServiceOrderTable& outTable)
            {
                // create the adjacency from the "tickBefore"
                for (auto service : graph.values())
                    service->m_adj = service->m_tickBefore;

                // create the adjacency from the "tickAfter"
                for (auto service : graph.values())
                    for (auto otherService : service->m_tickAfter)
                        otherService->m_adj.pushBackUnique(service);

                // generate topology
                if (!TopologicalSort(graph, outTable))
                {
                    TRACE_ERROR("Unable to sort the service nodes");
                    //PrintGraph(graph);
                    //PrintTopology(graph, outTable);
                    return false;
                }

                // print the table
                //TRACE_SPAM("Found order for service ticking:");
                //PrintTopology(graph, outTable);
                return true;
            }
        };

        //--

        void World::initializeSystems()
        {
            // extract all systems
            WorldSystemCreator::Graph graph;
            WorldSystemCreator::ExtractServiceClasses(graph);

            // generate ticking order
            WorldSystemCreator::ServiceOrderTable initOrder;
            if (!WorldSystemCreator::GenerateInitializationOrder(graph, initOrder))
            {
                FATAL_ERROR("World system initialization graph contains cycles. Initialization impossible.");
            }

            // initialize the services
            memset(m_systemMap, 0, sizeof(m_systemMap));
            for (auto& info : initOrder)
            {
                TRACE_SPAM("Initializing world system '{}'", info->m_class->name());

                // create service
                auto systemPtr = info->m_class.create();
                if (systemPtr->autoInitialized())
                {
                    if (systemPtr->handleInitialize(*this))
                    {
                        m_systems.pushBack(systemPtr);
                        m_systemMap[info->m_class->userIndex()] = systemPtr;
                    }
                    else
                    {
                        TRACE_WARNING("World system '{}' failed to initialize", info->m_class->name());
                    }
                }
            }
        }

        //---

        void World::cancelAllTransformRequests()
        {
            for (auto* token : m_transformRequestsMap.values())
                m_transformRequetsPool.free(token);
            m_transformRequestsMap.reset();
        }

        void World::cancelTransformRequest(Entity* entity)
        {
            if (auto* token = m_transformRequestsMap.findSafe(entity, nullptr))
            {
                m_transformRequestsMap.remove(entity);
                m_transformRequetsPool.free(token);
            }
        }

        void World::scheduleEntityForTransformUpdate(Entity* entity)
        {
            auto*& token = m_transformRequestsMap[entity];
            if (!token)
            {
                token = m_transformRequetsPool.create();
                token->entity = entity;
                token->hasNewTransform = false;
            }
        }

        void World::scheduleEntityForTransformUpdateWithTransform(Entity* entity, const AbsoluteTransform& newTransform)
        {
            auto*& token = m_transformRequestsMap[entity];
            if (!token)
            {
                token = m_transformRequetsPool.create();
                token->entity = entity;
            }

            token->hasNewTransform = true;
            token->newTransform = newTransform;
        }

        template< typename T >
        static T& SwapMaps(T& a, T& b)
        {
            auto temp = std::move(a);
            a = std::move(b);
            b = std::move(temp);
            return b;
        }

        void World::serviceTransformRequests(WorldStats& outStats)
        {
            PC_SCOPE_LVL1(TransformEntities);

            // swap maps around
            auto& requests = SwapMaps(m_transformRequestsMap, m_transformRequestsMap2);
            if (!requests.empty())
            {
                ScopeTimer timer;

                for (auto* token : requests.values())
                {
                    // apply the transform update
                    if (auto actualEntity = token->entity.lock())
                    {
                        if (token->hasNewTransform)
                            actualEntity->handleTransformUpdate(token->newTransform);
                        else
                            actualEntity->handleTransformUpdate(actualEntity->absoluteTransform());
                    }

                    // release token back to pool
                    m_transformRequetsPool.free(token);
                }

                outStats.numTransformUpdateBaches += 1;
                outStats.numTransformUpdateEntities += requests.size();
                outStats.transformUpdateTime += timer.timeElapsed();

                requests.reset();
            }
        }

        //---

        static ConfigProperty<bool> cvDebugPageWorldStats("DebugPage.World.Stats", "IsVisible", false);

        static void RenderWorldStats(const WorldStats& stats)
        {
            ImGui::Text("Total Update [us]: %d", (int)(stats.totalUpdateTime * 1000000.0));

            ImGui::Separator();

            ImGui::Text("Entities: %d", stats.numEntites);
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "  (+%u)", stats.numEntitiesAttached);
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "  (-%u)", stats.numEntitiesDetached);

            ImGui::Separator();

            ImGui::Text("Transform update: %u baches (%u entities)", stats.numTransformUpdateBaches, stats.numTransformUpdateEntities);
            ImGui::Text("Transform time: %d [us]", (int)(stats.transformUpdateTime * 1000000.0));

            ImGui::Separator();

            ImGui::Text("PreTick: %u entities", stats.numPreTickEntites);
            ImGui::Text("PreTick system time: %u [us]", (int)(stats.preTickSystemTime * 1000000.0));
            ImGui::Text("PreTick entity time: %u [us]", (int)(stats.preTickEntityTime * 1000000.0));
            ImGui::Text("PreTick fixup time: %u [us] (%u loops)", (int)(stats.preTickFixupTime * 1000000.0), stats.numPreTickLoops);

            ImGui::Separator();

            ImGui::Text("MainTick time: %u [us]", (int)(stats.preTickSystemTime * 1000000.0));

            ImGui::Separator();

            ImGui::Text("PostTick: %u entities", stats.numPostTickEntites);
            ImGui::Text("PostTick system time: %u [us]", (int)(stats.postTickSystemTime * 1000000.0));
            ImGui::Text("PostTick entity time: %u [us]", (int)(stats.postTickEntityTime * 1000000.0));
        }

        void World::renderDebugGui()
        {
            if (cvDebugPageWorldStats.get() && ImGui::Begin("World stats"))
            {
                RenderWorldStats(m_stats);
                ImGui::End();
            }
        }

        //---

    } // world
} // base


