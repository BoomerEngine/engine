/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: services #]
***/

#include "build.h"
#include "application.h"
#include "localService.h"
#include "localServiceContainer.h"
#include "commandline.h"

#include "base/object/include/rttiClassType.h"
#include "base/object/include/rttiMetadata.h"
#include "base/containers/include/queue.h"
#include "base/memory/include/poolStats.h"

namespace base
{
    namespace app
    {

        //-----

        LocalServiceContainer::LocalServiceContainer()
        {}

        void LocalServiceContainer::attachService(const RefPtr<ILocalService>& service)
        {
            ASSERT(service.get() != nullptr);

            // keep alive
            m_services.pushBack(service);

            // map (can be at multiple slots)
            auto rootServiceClass = ILocalService::GetStaticClass();
            auto serviceClass = service->cls();
            while (serviceClass != rootServiceClass)
            {
                ASSERT(serviceClass->userIndex() == -1);

                auto serviceId = m_serviceMap.size();
                const_cast<base::rtti::IClassType*>(serviceClass.ptr())->assignUserIndex((short)serviceId); // TEMP HACK
                m_serviceMap.pushBack(service.get());

                serviceClass = serviceClass->baseClass();
            }
        }

        void LocalServiceContainer::deinit()
        {
            ASSERT_EX(m_services.empty(), "Local services not shutdown properly");
        }

        void LocalServiceContainer::shutdown()
        {
            // shutdown service in the reverse order they were initialized
            if (!m_services.empty())
            {
                // notify that we have services to close
                TRACE_INFO("Closing services");

                // close each service in reverse order they were created
                base::Array<RefWeakPtr<ILocalService>> tempServicePtrs;
                for (int i = (int)(m_services.size()) - 1; i >= 0; --i)
                {
                    auto& service = m_services[i];

                    TRACE_INFO("Shutting down service '{}'", service->cls()->name());
                    service->onShutdownService();
                    tempServicePtrs.pushBack(service.get());
                    TRACE_INFO("Service '{}' shut down", service->cls()->name());
                }

                // we may have crashed, put a log
                TRACE_INFO("All services closed");
                m_services.clear();

                // check if service pointer was released properly - it's illegal to hold onto those
                bool hasAliveServices = false;
                for (const auto& ptr : tempServicePtrs)
                {
                    if (auto servicePtr = ptr.lock())
                    {
                        TRACE_ERROR("Service '{}' is still alive. Memory leaks and crashed may occur.", servicePtr->cls()->name());
                        hasAliveServices = true;
                    }
                }
                DEBUG_CHECK_EX(!hasAliveServices, "There are some services that are still alive after platform cleanup. Please investigate as this may lead to severe leaks and/or crashes at exit.");

                // we may crash here, so add another log
                TRACE_INFO("All services released");
            }

        }

        void LocalServiceContainer::update()
        {
            PC_SCOPE_LVL1(UpdateLocalServices);

            // make sure sync jobs are done
            Fibers::GetInstance().runSyncJobs();

            // reset per frame memory stats
            base::mem::PoolStats::GetInstance().resetFrameStatistics();

            // update the sync part of all the service
            for (auto service : m_tickList)
                service->onSyncUpdate();

            // make sure sync jobs are done
            Fibers::GetInstance().runSyncJobs();
        }

        namespace helper
        {
            struct ServiceInfo
            {
                RTTI_DECLARE_POOL(POOL_TEMP)

            public:
                ClassType m_class;
                Array<ServiceInfo*> m_dependsOnInit;
                Array<ServiceInfo*> m_tickBefore;
                Array<ServiceInfo*> m_tickAfter;

                Array<ServiceInfo*> m_adj; // temp
                
                RefPtr<ILocalService> m_service;

                int m_depth;

                INLINE ServiceInfo()
                    : m_class(nullptr)
                    , m_depth(0)
                {}
            };

            typedef HashMap<ClassType, ServiceInfo*> ServicesGraph;

            static ServiceInfo* CreateClassEntry(ClassType classType, ServicesGraph& table)
            {
                ServiceInfo* info = nullptr;
                if (table.find(classType, info))
                    return info;

                // map entry
                info = new ServiceInfo;
                info->m_class = classType;
                table[classType] = info;

                // extract dependency list
                auto classList  = classType->findMetadata<DependsOnServiceMetadata>();
                if (classList)
                {
                    for (auto refClassType : classList->classes())
                        info->m_dependsOnInit.pushBack(CreateClassEntry(refClassType, table));
                }

                // TODO: extract other types of dependencies

                return info;
            }

            void ExtractServiceClasses(ServicesGraph& outClasses)
            {
                // enumerate service classes
                Array<ClassType> serviceClasses;
                rtti::TypeSystem::GetInstance().enumClasses(ILocalService::GetStaticClass(), serviceClasses);
                TRACE_INFO("Found {} services linked with executable", serviceClasses.size());

                // create entries
                // NOTE: this may recurse a lot
                outClasses.reserve(serviceClasses.size());
                for (auto serviceClass : serviceClasses)
                    CreateClassEntry(serviceClass, outClasses);
            }

            typedef Array<ServiceInfo*> ServiceOrderTable;

            bool TopologicalSort(const ServicesGraph& graph, ServiceOrderTable& outOrder)
            {
                // reset depth
                for (auto service  : graph.values())
                    service->m_depth = 0;

                // traverse adjacency lists to fill the initial depths
                for (auto service  : graph.values())
                    for (auto dep  : service->m_adj)
                        dep->m_depth += 1;

                // create a queue and add all not referenced crap
                Queue<ServiceInfo*> q;
                for (auto service  : graph.values())
                    if (service->m_depth == 0)
                        q.push(service);

                // process vertices
                outOrder.clear();
                while (!q.empty())
                {
                    // pop and add to order
                    auto p  = q.top();
					q.pop();
                    outOrder.pushBack(p);

                    // reduce depth counts, if they become zero we are ready to dispatch
                    for (auto dep  : p->m_adj)
                        if (0 == --dep->m_depth)
                            q.push(dep);
                }

                // swap order
                for (uint32_t i=0; i<outOrder.size()/2; ++i)
                    std::swap(outOrder[i], outOrder[outOrder.size()-1 - i]);

                // we must have the same count, if not, there's a cycle
                return outOrder.size() == graph.size();
            }

            void PrintGraph(const ServicesGraph& graph)
            {
                for (auto service  : graph.values())
                {
                    TRACE_SPAM("Service '{}' ({} deps), depth {}", service->m_class->name(), service->m_adj.size(), service->m_depth);
                    for (auto dep  : service->m_adj)
                    {
                        TRACE_SPAM("   Edge to: '{}'", dep->m_class->name());
                    }
                }
            }

            void PrintTopology(const ServicesGraph& graph, const ServiceOrderTable& order)
            {
                uint32_t index = 0;
                for (auto service  : order)
                {
                    TRACE_SPAM("ServiceOrder[{}]: {}", index++, service->m_class->name());
                }

                for (auto service  : graph.values())
                {
                    if (!order.contains(service))
                    {
                        TRACE_ERROR("App:Service '{}' not included in topological order", service->m_class->name());
                    }
                }
            }

            bool GenerateInitializationOrder(const ServicesGraph& graph, ServiceOrderTable& outTable)
            {
                // create the adjacency from the "dependsOn" part
                for (auto service  : graph.values())
                    service->m_adj = service->m_dependsOnInit;

                // generate topology
                if (!TopologicalSort(graph, outTable))
                {
                    TRACE_ERROR("Unable to sort the service nodes");
                    PrintGraph(graph);
                    PrintTopology(graph, outTable);
                    return false;
                }

                // print the table
                TRACE_SPAM("Found order for service initialization:");
                PrintTopology(graph, outTable);
                return true;
            }

            bool GenerateTickingOrder(const ServicesGraph& graph, ServiceOrderTable& outTable)
            {
                // create the adjacency from the "tickBefore"
                for (auto service  : graph.values())
                    service->m_adj = service->m_tickBefore;

                // create the adjacency from the "tickAfter"
                for (auto service  : graph.values())
                    for (auto otherService  : service->m_tickAfter)
                        otherService->m_adj.pushBackUnique(service);

                // generate topology
                if (!TopologicalSort(graph, outTable))
                {
                    TRACE_ERROR("Unable to sort the service nodes");
                    PrintGraph(graph);
                    PrintTopology(graph, outTable);
                    return false;
                }

                // print the table
                TRACE_SPAM("Found order for service ticking:");
                PrintTopology(graph, outTable);
                return true;
            }

        } // helper

        bool LocalServiceContainer::init(const CommandLine& commandline)
        {
            ScopeTimer initTimer;

            // extract the service classes and their information
            helper::ServicesGraph serviceGraph;
            if (commandline.hasParam("singleService"))
            {
                auto serviceClasses = commandline.allValues("singleService");
                for (auto& name : serviceClasses)
                {
                    auto serviceClass = RTTI::GetInstance().findFactoryClass(StringID(name), ILocalService::GetStaticClass());
                    if (!serviceClass)
                    {
                        TRACE_ERROR("Single service '{}' not found", name);
                        return false;
                    }

                    helper::CreateClassEntry(serviceClass, serviceGraph);
                }
            }
            else
            {
                helper::ExtractServiceClasses(serviceGraph);
            }

            // generate initialization order
            {
                helper::ServiceOrderTable initOrder;
                if (!helper::GenerateInitializationOrder(serviceGraph, initOrder))
                {
                    FATAL_ERROR("Services initialization graph contains cycles. Initialization impossible.");
                    return true;
                }

                // generate ticking order
                helper::ServiceOrderTable tickOrder;
                if (!helper::GenerateTickingOrder(serviceGraph, tickOrder))
                {
                    FATAL_ERROR("Services ticking graph contains cycles. Initialization impossible.");
                    return true;
                }

                // initialize the services
                for (auto& info : initOrder)
                {
                    TRACE_SPAM("Initializing service '{}'", info->m_class->name());

                    ScopeTimer initTime;

                    // create service
                    auto servicePtr = info->m_class.create<ILocalService>();
                    ASSERT(servicePtr);
                    attachService(servicePtr);

                    // initialize
                    auto ret = servicePtr->onInitializeService(commandline);
                    if (ret == ServiceInitializationResult::Finished)
                    {
                        TRACE_INFO("Service '{}' initialized in {}", info->m_class->name(), TimeInterval(initTime.timeElapsed()));
                        info->m_service = servicePtr;
                    }
                    else if (ret == ServiceInitializationResult::Silenced)
                    {
                        servicePtr->onShutdownService();
                        TRACE_INFO("Service '{}' failed to initialize and will be disabled", info->m_class->name());
                    }
                    else if (ret == ServiceInitializationResult::FatalError)
                    {
                        servicePtr->onShutdownService();
                        TRACE_ERROR("Service '{}' failed to initialize", info->m_class->name());
                        return false;
                    }
                }

                // assemble the final tick list
                m_tickList.reserve(tickOrder.size());
                for (auto& entry : tickOrder)
                    if (entry->m_service)
                        m_tickList.pushBack(entry->m_service.get());

                // cleanup lists
                //tickOrder.clearPtr();
                initOrder.clearPtr();
            }

            // service initialization timestamp
            TRACE_INFO("Services initialized in {}", TimeInterval(initTimer.timeElapsed()));
            return true;
        }

        //---

    } // app

    //---

    void* GetServicePtr(int serviceIndex)
    {
        return app::LocalServiceContainer::GetInstance().serviceByIndex(serviceIndex);
    }

    //---

} // base