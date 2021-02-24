/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: services #]
***/

#pragma once

#include "base/containers/include/inplaceArray.h"
#include "base/system/include/timing.h"
#include "base/system/include/singleton.h"

BEGIN_BOOMER_NAMESPACE(base::app)

//---

// container for all LOCAL services in this process
class BASE_APP_API LocalServiceContainer : public ISingleton
{
    DECLARE_SINGLETON(LocalServiceContainer);

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
    INLINE T* service() const
    {
        static auto userIndex = reflection::ClassID<T>()->userIndex();
        if (userIndex != -1)
        {
            auto service = m_serviceMap[userIndex];
            //ASSERT_EX(service->cls()->is<T>(), "Incompatible service mapping");
            return static_cast<T *>(service);
        }

        return nullptr;
    }

    /// get the service by class
    INLINE void* serviceByIndex(int index) const
    {
        if (index != -1)
            return m_serviceMap[index];
        else
            return nullptr;
    }

protected:
    static const uint32_t MAX_SERVICE = 128;
    InplaceArray<ILocalService*, MAX_SERVICE> m_serviceMap; // index -> service pointer

    typedef Array< RefPtr<ILocalService> > TServices;
    TServices m_services; // all services registered in the app

    typedef Array<ILocalService*> TRawServices;
    TRawServices m_tickList; // list of services to tick (in the order of the updates)

    //---

    LocalServiceContainer();

    virtual void deinit() override final;

    //---

    /// attach service to the application
    void attachService(const RefPtr<ILocalService>& service);
};

//---

END_BOOMER_NAMESPACE(base::app)