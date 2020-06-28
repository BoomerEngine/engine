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
#include "worldDefinition.h"
#include "worldInputSystem.h"
#include "gameEntity.h"

#include "base/object/include/rttiClassType.h"
#include "base/containers/include/queue.h"
#include "rendering/scene/include/renderingFrameCamera.h"
#include "rendering/scene/include/renderingFrameParams.h"

base::mem::PoolID POOL_SCENE_COMMON("World.Common");

namespace game
{
    //---

    base::ConfigProperty cvGameDefaultCameraFov("Game.Camera", "DefaultFOV", 90.0f);
    base::ConfigProperty cvGameDefaultCameraNearPlane("Game.Camera", "DefaultNearPlane", 0.01f);
    base::ConfigProperty cvGameDefaultCameraFarPlane("Game.Camera", "DefaultFarPlane", 4000.0f);

    //---

    RTTI_BEGIN_TYPE_ENUM(WorldType)
        RTTI_ENUM_OPTION(Editor);
        RTTI_ENUM_OPTION(Game);
        // TODO: headless server
    RTTI_END_TYPE();

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

    World::World(WorldType type, const WorldDefinitionPtr& data)
        : m_type(type)
        , m_data(data)
    {
        initializeSystems();
    }

    World::~World()
    {
        destroyEntities();
        destroySystems();
    }

    struct ScopeBool : public base::NoCopy
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


    bool World::processInput(const base::input::BaseEvent& evt)
    {
        PC_SCOPE_LVL1(WorldInput);

        if (auto* inputSystem = system<WorldInputSystem>())
            return inputSystem->handleInput(evt);

        return false;
    }

    void CAN_YIELD World::update(const UpdateContext& ctx)
    {
        PC_SCOPE_LVL1(WorldUpdate);

        {
            ScopeBool flag(m_protectedEntityRegion);
            updatePreTick(ctx);
            updateSystems(ctx);
            updatePostTick(ctx);
            updateTransform(ctx);
        }
    }

    void World::updatePreTick(const UpdateContext& ctx)
    {
        PC_SCOPE_LVL1(PreTick);

        for (const auto& sys : m_systems)
            sys->handlePreTick(*this, ctx);

        for (const auto& ent : m_entities.keys())
            ent->handlePreTick(ctx.m_dt);

        for (;;)
        {
            base::InplaceArray<Entity*, 64> newEntities;
            applyEntityChanges(&newEntities);
            if (newEntities.empty())
                break;

            for (auto* ent : newEntities)
                ent->handlePreTick(ctx.m_dt);
        }
    }

    void World::updateSystems(const UpdateContext& ctx)
    {
        PC_SCOPE_LVL1(SystemUpdate);

        for (const auto& sys : m_systems)
            sys->handleMainTickStart(*this, ctx);

        for (const auto& sys : m_systems)
            sys->handleMainTickFinish(*this, ctx);

        for (const auto& sys : m_systems)
            sys->handleMainTickPublish(*this, ctx);
    }

    void World::updatePostTick(const UpdateContext& ctx)
    {
        for (const auto& ent : m_entities.keys())
            ent->handlePostTick(ctx.m_dt);

        for (const auto& sys : m_systems)
            sys->handlePostTick(*this, ctx);
    }

    void World::updateTransform(const UpdateContext& ctx)
    {
        // TODO: obviously we should touch only interested entities

        for (const auto& ent : m_entities.keys())
            ent->handleTransformUpdate();

        for (const auto& sys : m_systems)
            sys->handlePostTransform(*this, ctx);
    }

    void World::destroyEntities()
    {
        DEBUG_CHECK(m_removedEntities.empty());
        DEBUG_CHECK(m_addedEntities.empty());

        auto entities = std::move(m_entities);
        for (const auto& ent : entities)
            ent->detachFromWorld(this);
    }

    void World::applyEntityChanges(base::Array<Entity*>* outCreatedEntities)
    {
        while (!m_removedEntities.empty() || !m_addedEntities.empty())
        {
            auto removedList = std::move(m_removedEntities);
            auto addedList = std::move(m_addedEntities);

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

            entity->handleTransformUpdate(); // calculate initial positions

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

    void CAN_YIELD World::render(rendering::scene::FrameParams& info)
    {
        PC_SCOPE_LVL1(RenderSceneSystems);

        // setup default camera
        rendering::scene::CameraSetup defaultCamera;
        defaultCamera.fov = cvGameDefaultCameraFov.get();
        defaultCamera.aspect = info.resolution.width / (float)info.resolution.height;
        defaultCamera.nearPlane = cvGameDefaultCameraNearPlane.get();
        defaultCamera.farPlane = cvGameDefaultCameraFarPlane.get();

        // prepare camera
        for (auto& sys : m_systems)
            sys->handlePrepareCamera(*this, defaultCamera);

        // set camera
        info.camera.camera.setup(defaultCamera);

        // render all system stuff
        for (auto &sys : m_systems)
            sys->handleRendering(*this, info);

        // allow entities to debug render as well
        for (auto& ent : m_entities)
            ent->handleDebugRender(info);
    }

    //--

    void World::destroySystems()
    {
        // shutdown systems
        for (auto i = m_systems.lastValidIndex(); i >= 0; --i)
            m_systems[i]->handleShutdown(*this);

        // close all systems
        for (auto& system : m_systems)
            system.reset();
        m_systems.clear();
    }

    //--

    namespace WorldSystemCreator
    {
        struct Info
        {
            base::SpecificClassType<IWorldSystem> m_class = nullptr;
            base::Array<Info*> m_dependsOnInit;
            base::Array<Info*> m_tickBefore;
            base::Array<Info*> m_tickAfter;

            base::Array<Info*> m_adj; // temp

            base::RefPtr<IWorldSystem> m_system;

            int m_depth = 0;

        };

        typedef base::HashMap<base::SpecificClassType<IWorldSystem>, Info*> Graph;

        static Info* CreateClassEntry(base::SpecificClassType<IWorldSystem> classType, Graph& table)
        {
            Info* info = nullptr;
            if (table.find(classType, info))
                return info;

            // map entry
            info = MemNew(Info);
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
            base::Array<base::SpecificClassType<IWorldSystem>> serviceClasses;
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

        typedef base::Array<Info*> ServiceOrderTable;

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
            base::Queue<Info*> q;
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

    //---

} // game


