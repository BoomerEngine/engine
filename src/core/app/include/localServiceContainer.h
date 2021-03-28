/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: services #]
***/

#pragma once

#include "core/containers/include/inplaceArray.h"
#include "core/system/include/timing.h"
#include "core/system/include/singleton.h"

BEGIN_BOOMER_NAMESPACE()

//---

// container for all LOCAL services in this process
class CORE_APP_API ServiceContainer : public ISingleton
{
    DECLARE_SINGLETON(ServiceContainer);

public:
    ///--

    /// initialize all local services, fails if any of the service initialization fails hard
    bool init(const CommandLine& commandline);

    /// update services, can be called manually (modal UI etc)
    void update();

    /// shutdown all local service
    void shutdown();

    ///--

    /// get the service by class
    template< typename T >
    INLINE static T* Service()
    {
        static auto userIndex = ClassID<T>()->userIndex();
        if (userIndex != -1)
        {
            auto service = st_serviceMap[userIndex];
            return static_cast<T *>(service);
        }

        return nullptr;
    }

    /// get the service by class
    INLINE static void* ServiceByIndex(int index)
    {
        if (index != -1)
            return st_serviceMap[index];
        else
            return nullptr;
    }

protected:
    static const uint32_t MAX_SERVICE = 128;
    static IService* st_serviceMap[MAX_SERVICE]; // index -> service pointer

    typedef Array< RefPtr<IService> > TServices;
    TServices m_services; // all services registered in the app

    typedef Array<IService*> TRawServices;
    TRawServices m_tickList; // list of services to tick (in the order of the updates)

    uint32_t m_nextServiceId = 1;

    //---

    ServiceContainer();

    virtual void deinit() override final;

    //---

    /// attach service to the application
    void attachService(const RefPtr<IService>& service);
};

//---

END_BOOMER_NAMESPACE()
