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

#include "base/object/include/rttiClassType.h"

base::mem::PoolID POOL_SCENE_COMMON("Scene.Common");

namespace scene
{
    ///--

    RTTI_BEGIN_TYPE_ENUM(SceneType)
        RTTI_ENUM_OPTION(Editor);
        RTTI_ENUM_OPTION(Game);
    RTTI_END_TYPE();

    RTTI_BEGIN_TYPE_NATIVE_CLASS(Scene);
    RTTI_END_TYPE();

    ///--

    Scene::Scene(SceneType type)
        : m_type(type)
    {
        memset(m_systemMap, 0, sizeof(m_systemMap));

        initializeSceneSystems();
    }

    Scene::~Scene()
    {
        // detach all content
        for (auto& content : m_worldContent)
            for (auto& ptr : m_systems)
                ptr->onWorldContentDetached(*this, content);
        m_worldContent.clear();

        // shutdown systems
        for (auto i=m_systems.lastValidIndex(); i>=0; --i)
            m_systems[i]->onShutdown(*this);

        // close all systems
        for (auto& system : m_systems)
            system.reset();
        m_systems.clear();
    }

    void CAN_YIELD Scene::update(const UpdateContext& ctx)
    {
        PC_SCOPE_LVL1(SceneUpdate);

        // update the tick system
        if (m_type == SceneType::Editor)
        {
            /*PC_SCOPE_LVL1(ResourcesRefresh);
            m_rootTable.foreach([this](const GroupPtr& ptr)
                {
                    ptr->handleResourceChanges();
                });*/
        }

        // update before global tick
        {
            PC_SCOPE_LVL1(SystemPreTick);
            for (auto &sys : m_systems)
                sys->onPreTick(*this, ctx);
        }

        // update the tick system
        {
            PC_SCOPE_LVL1(GlobalTick);
            for (auto &sys : m_systems)
                sys->onTick(*this, ctx);
        }

        // update before global transform
        {
            PC_SCOPE_LVL1(SystemPreTransform);
            for (auto &sys : m_systems)
                sys->onPreTransform(*this, ctx);
        }

        // update the transforms
        {
            PC_SCOPE_LVL1(UpdateTransforms);
            for (auto &sys : m_systems)
                sys->onTransform(*this, ctx);
        }

        // update after global transform
        {
            PC_SCOPE_LVL1(SystemPostTransform);
            for (auto &sys : m_systems)
                sys->onPostTransform(*this, ctx);
        }
    }

    void CAN_YIELD Scene::render(rendering::scene::FrameInfo& info)
    {
        PC_SCOPE_LVL1(RenderSceneSystems);

        // render all system stuff
        for (auto &sys : m_systems)
            sys->onRenderFrame(*this, info);
    }

    int Scene::createObserver(const base::AbsolutePosition& initialPosition)
    {
        auto id = m_nextObserverID++;

        auto& entry = m_observers.emplaceBack();
        entry.m_id = id;
        entry.m_position = initialPosition;
        entry.m_maxStreamingRange = 0.0f;
        
        return id;
    }

    void Scene::updateObserver(int id, const base::AbsolutePosition& newPosition)
    {
        for (auto& entry : m_observers)
        {
            if (entry.m_id == id)
            {
                entry.m_position = newPosition;
                break;
            }
        }
    }

    void Scene::removeObserver(int id)
    {
        for (uint32_t i = 0; i < m_observers.size(); ++i)
        {
            if (m_observers[i].m_id == id)
            {
                m_observers.erase(i);
                break;
            }
        }
    }

    void Scene::initializeSceneSystems()
    {
        // enumerate sub-systems
        base::Array<base::SpecificClassType<IRuntimeSystem>> sceneSubSystemClasses;
        RTTI::GetInstance().enumClasses(sceneSubSystemClasses);
        TRACE_INFO("Found {} scene sub system classes", sceneSubSystemClasses.size());

        // assign indices
        static short numSceneSystems = 0;
        if (0 == numSceneSystems)
        {
            for (auto subSystemClass : sceneSubSystemClasses)
            {
                subSystemClass->assignUserIndex(numSceneSystems);
                numSceneSystems += 1;
            }
        }

        //---

        struct InitInfo
        {
            base::ClassType m_class;
            int m_initializationOrder;

            INLINE bool operator<(const InitInfo& other) const
            {
                // sort be initialization order
                if (other.m_initializationOrder != m_initializationOrder)
                    return (other.m_initializationOrder > m_initializationOrder);

                // sort by name (we need to be deterministic run to run)
                return m_class->name().view() < other.m_class->name().view();
            }
        };
        base::Array<InitInfo> initInfos;

        // create and initialize sub systems
        for (auto subSystemClass  : sceneSubSystemClasses)
        {
            // get the priority

            auto orderMetadata = subSystemClass->findMetadata<scene::RuntimeSystemInitializationOrderMetadata>();
            if (!orderMetadata)
            {
                TRACE_INFO("Runtime system '{}' has no initialization order value and will not be initialized", subSystemClass->name());
                continue;
            }

            // add to list
            auto& info = initInfos.emplaceBack();
            info.m_class = subSystemClass;
            info.m_initializationOrder = orderMetadata->order();
        }

        // sort by initialization order
        std::sort(initInfos.begin(), initInfos.end());

        // initialize systems
        for (auto& info : initInfos)
        {
            auto subSystemClass  = info.m_class;
            auto subSystem = subSystemClass->createSharedPtr<IRuntimeSystem>();
            if (!subSystem->checkCompatiblity(m_type))
                continue;

            // initialize the system
            if (!subSystem->onInitialize(*this))
            {
                TRACE_ERROR("Failed to initialize runtime system '{}'", subSystemClass->name());
                continue;
            }

            // add to sub system list
            auto systemId = subSystemClass->userIndex();
            m_systems.pushBack(subSystem);
            m_systemMap[systemId] = subSystem.get();

            // stats
            TRACE_INFO("Runtime system '{}' initialized for scene", subSystemClass->name());
        }
    }

    void Scene::attachWorldContent(const WorldPtr& worldPtr)
    {
        if (worldPtr)
        {
            if (!m_worldContent.contains(worldPtr))
            {
                m_worldContent.pushBack(worldPtr);

                for (auto& sys : m_systems)
                    sys->onWorldContentAttached(*this, worldPtr);
            }
        }
    }

    void Scene::detachWorldContent(const WorldPtr& worldPtr)
    {
        if (m_worldContent.contains(worldPtr))
        {
            for (auto& sys : m_systems)
                sys->onWorldContentDetached(*this, worldPtr);

            m_worldContent.remove(worldPtr);
        }
    }

} // scene


