/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
***/

#include "build.h"
#include "renderingScene.h"
#include "renderingSceneProxyDesc.h"
#include "renderingSceneProxy.h"
#include "renderingSceneCulling.h"
#include "renderingSceneFragment.h"

#include "rendering/device/include/renderingCommandBuffer.h"
#include "rendering/device/include/renderingShaderLibrary.h"

namespace rendering
{
    namespace scene
    {
        //----

        base::ConfigProperty<uint32_t> cvRenderSceneMaxObject("Rendering.Scene", "MaxObjects", 128 * 1024);
        base::ConfigProperty<uint32_t> cvRenderSceneMaxObjectEditor("Rendering.Scene", "MaxObjectsEditor", 128 * 1024);
        base::ConfigProperty<uint32_t> cvRenderSceneMaxObjectPreview("Rendering.Scene", "MaxObjectsPreview", 64);

        //----

        Scene::Scene(const SceneSetupInfo& setup)
            : m_type(setup.type)
            , m_name(setup.name)
            , m_proxyTable(POOL_TEMP)
        {
            uint32_t maxObjects = 1024;

            // determine budgets
            if (setup.type == SceneType::EditorPreview)
                maxObjects = std::max<uint32_t>(16, cvRenderSceneMaxObjectPreview.get());
            else if (setup.type == SceneType::EditorGame)
                maxObjects = std::max<uint32_t>(1024, cvRenderSceneMaxObjectEditor.get());
            else
                maxObjects = std::max<uint32_t>(1024, cvRenderSceneMaxObject.get());
            TRACE_INFO("Rendering will use {} as asbsolute object limit for the scene '{}'", maxObjects, m_name);

            // limit object count for tiny scenes

            // create internal containers
            m_objects.create(this, maxObjects);
            m_proxyTable.resize(maxObjects);

            // create proxy and fragment handlers
            createProxyHandlers();
            createFragmentHandlers();
        }

        Scene::~Scene()
        {
            destroyAllProxies();

            for (auto* handler : m_proxyHandlers)
                delete handler;

            for (auto* handler : m_fragmentHandlers)
                delete handler;

            memset(m_proxyHandlers, 0, sizeof(m_proxyHandlers));
            memset(m_fragmentHandlers, 0, sizeof(m_fragmentHandlers));

            m_objects.reset();
        }

        bool Scene::lockForRendering()
        {
            const auto doLock = 0 == (m_lockCount++);
            if (doLock)
            {
                for (auto* handler : m_proxyHandlers)
                    if (handler)
                        handler->handleSceneLock();
            }

            return true;
        }

        void Scene::unlockAfterRendering(SceneStats&& updatedStats)
        {
            DEBUG_CHECK_EX(m_lockCount.load() > 0, "Invalid lock count");
            const auto doUnlock = 0 == (--m_lockCount);
            if (doUnlock)
            {
                for (auto* handler : m_proxyHandlers)
                    if (handler)
                        handler->handleSceneUnlock();


                m_stats.merge(updatedStats);
            }        
        }

        void Scene::prepareForRendering(command::CommandWriter& cmd)
        {
            DEBUG_CHECK_EX(m_lockCount.load() > 0, "Scene not locked for rendering");
            m_objects->prepareForFrame(cmd);

            for (auto* handler : m_proxyHandlers)
                if (handler)
                    handler->handlePrepare(cmd);
        }

        void Scene::destroyAllProxies()
        {
            uint32_t numProxiesAutoReleased = 0;
            m_proxyTable.enumerate([this, &numProxiesAutoReleased](const ProxyEntry& entry, uint32_t index)
                {
                    auto handler = m_proxyHandlers[(int)entry.type];
                    handler->handleProxyDestroy(entry.proxy);
                    numProxiesAutoReleased += 1;

                    return false;
                });

            if (numProxiesAutoReleased > 0)
            {
                TRACE_WARNING("Released {} still attached proxies when closing scene '{}'", numProxiesAutoReleased, m_name);
            }
        }

        void Scene::createProxyHandlers()
        {
            memset(m_proxyHandlers, 0, sizeof(m_proxyHandlers));

            base::InplaceArray<base::SpecificClassType<IProxyHandler>, 20> handlerClasses;
            RTTI::GetInstance().enumClasses(handlerClasses);

            for (auto cls : handlerClasses)
            {
                auto handler = cls->createPointer<IProxyHandler>();
                auto index = (int)handler->proxyType();

                DEBUG_CHECK_EX(m_proxyHandlers[index] == nullptr, "Proxy handler for that proxy type already registered!");
                if (m_proxyHandlers[index] == nullptr)
                {
                    handler->handleInit(this);
                    m_proxyHandlers[index] = handler;
                    TRACE_INFO("Registered '{}' as handle for proxy type {}", cls->name(), index);
                }
            }
        }

        void Scene::createFragmentHandlers()
        {
            memset(m_fragmentHandlers, 0, sizeof(m_fragmentHandlers));

            base::InplaceArray<base::SpecificClassType<IFragmentHandler>, 20> handlerClasses;
            RTTI::GetInstance().enumClasses(handlerClasses);

            for (auto cls : handlerClasses)
            {
                auto handler = cls->createPointer<IFragmentHandler>();
                auto index = (int)handler->type();

                DEBUG_CHECK_EX(m_fragmentHandlers[index] == nullptr, "Fragment handler for that proxy type already registered!");
                if (m_fragmentHandlers[index] == nullptr)
                {
                    handler->handleInit(this);
                    m_fragmentHandlers[index] = handler;
                    TRACE_INFO("Registered '{}' as handle for fragment type {}", cls->name(), index);
                }
            }
        }

        //--

        ProxyHandle Scene::proxyCreate(const ProxyBaseDesc& desc)
        {
            DEBUG_CHECK_EX(m_lockCount.load() == 0, "Creating proxy in scene locked for rendering");

            auto handlerType = (uint8_t)desc.proxyType();
            DEBUG_CHECK_EX(handlerType < (int)ProxyType::MAX, "Invalid proxy type");
            if (handlerType >= (int)ProxyType::MAX)
                return ProxyHandle();

            auto handler = m_proxyHandlers[handlerType];
            DEBUG_CHECK_EX(handler, "No handler to service proxy of this type");
            if (nullptr == handler)
                return ProxyHandle();

            DEBUG_CHECK_EX(!m_proxyTable.full(), "Scene is full, rendering artifacts will occurr");
            if (m_proxyTable.full())
                return ProxyHandle();

            auto proxy = handler->handleProxyCreate(desc);
            if (!proxy)
                return ProxyHandle();

            auto index = m_proxyTable.emplace();
            auto& proxyEntry = m_proxyTable.typedData()[index];
            proxyEntry.generationIndex = m_proxyGenerationIndex++;
            proxyEntry.proxy = proxy;
            proxyEntry.type = (ProxyType)handlerType;

            return {index, proxyEntry.generationIndex};
        }

        void Scene::proxyDestroy(ProxyHandle handle)
        {
            // empty handle
            if (!handle)
                return;

            // out of range checks
            DEBUG_CHECK_EX(handle.generation < m_proxyGenerationIndex || handle.index < m_proxyTable.capacity(), "Corrupted proxy handle");
            if (handle.generation >= m_proxyGenerationIndex || handle.index >= m_proxyTable.capacity())
                return;

            // get entry and validate handle
            const auto& entry = m_proxyTable.typedData()[handle.index];
            DEBUG_CHECK_EX(entry.generationIndex == handle.generation, "Using dead proxy");
            if (entry.generationIndex != handle.generation)
                return;

            // get the handler
            auto handler = m_proxyHandlers[(int)entry.type];
            handler->handleProxyDestroy(entry.proxy);

            // return the entry to pool
            m_proxyTable.free(handle.index);
        }

        void Scene::proxyCommand(ProxyHandle handle, const Command& command)
        {
            // empty handle
            DEBUG_CHECK_EX(handle, "Empty proxy handle");
            if (!handle)
                return;

            // out of range checks
            DEBUG_CHECK_EX(handle.generation < m_proxyGenerationIndex || handle.index < m_proxyTable.capacity(), "Corrupted proxy handle");
            if (handle.generation >= m_proxyGenerationIndex || handle.index >= m_proxyTable.capacity())
                return;

            // get entry and validate handle
            const auto& entry = m_proxyTable.typedData()[handle.index];
            DEBUG_CHECK_EX(entry.generationIndex == handle.generation, "Using dead proxy");
            if (entry.generationIndex != handle.generation)
                return;

            // run the command
            auto handler = m_proxyHandlers[(int)entry.type];
            handler->handleCommand(this, entry.proxy, command);
        }
 
        //--

    } // scene
} // rendering
