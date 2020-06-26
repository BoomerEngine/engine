/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\views #]
***/

#include "build.h"
#include "renderingDriver.h"
#include "renderingDeviceObject.h"

namespace rendering
{

    //---

    namespace helper
    {
        class DeviceObjectRegistry : public base::ISingleton
        {
            DECLARE_SINGLETON(DeviceObjectRegistry);

        public:
            DeviceObjectRegistry()
            {}

            void unregisterObject(IDeviceObject* obj)
            {
                if (!m_objectsCleared)
                {
                    auto lock = CreateLock(m_lock);

                    DEBUG_CHECK_EX(m_objects.contains(obj), "Device object not previously registered");
                    m_objects.remove(obj);
                }
            }

            IDriver* safeRegisterObject(IDeviceObject* obj, std::atomic<uint32_t>& registeredFlag)
            {
                auto lock = CreateLock(m_lock);

                if (!registeredFlag.exchange(1))
                {
                    DEBUG_CHECK_EX(!m_objects.contains(obj), "Device object already registered");
                    m_objects.insert(obj);
                }

                return m_device;
            }

            void safeUnregisterObject(IDeviceObject* obj, std::atomic<uint32_t>& registeredFlag)
            {
                auto lock = CreateLock(m_lock);

                if (registeredFlag.exchange(0))
                {
                    DEBUG_CHECK_EX(m_objects.contains(obj), "Device object nor registered");
                    m_objects.remove(obj);
                }
            }

            void safeChangeDevice(IDriver* newDevice)
            {
                PC_SCOPE_LVL0(ChangeDevice);
                auto lock = CreateLock(m_lock);

                base::ScopeTimer timer;
                TRACE_INFO("Starting device change");

                if (nullptr != m_device)
                {
                    m_device->sync();

                    auto objects = m_objects.keys();
                    for (auto* obj : objects)
                        obj->switchDeviceOwnership(nullptr);
                }

                m_device = newDevice;

                if (nullptr != m_device)
                {
                    auto objects = m_objects.keys();
                    for (auto* obj : objects)
                        obj->switchDeviceOwnership(m_device);
                }

                TRACE_INFO("Finished device change in {} ({} objects)", timer, m_objects.size());
            }

        private:
            base::Mutex m_lock;

            IDriver* m_device = nullptr;
            base::HashSet<IDeviceObject*> m_objects;

            bool m_objectsCleared = false;

            virtual void deinit() override
            {
                DEBUG_CHECK_EX(m_device == nullptr, "There's still active rendering device");

                if (!m_objects.empty())
                {
                    TRACE_WARNING("There are still {} registered device objects:", m_objects.size());
                    for (uint32_t i = 0; i < m_objects.size(); ++i)
                    {
                        if (m_objects.keys()[i]->device())
                            TRACE_ERROR("  [{}]: '{}', ptr 0x{}", i, m_objects.keys()[i]->describe(), Hex((uint64_t)m_objects.keys()[i]));
                    }
                }

                //DEBUG_CHECK_EX(m_objects.empty(), "There are still registered rendering device object");

                m_objects.clear();
                m_objectsCleared = true;
            }
        };

    } // helper

    IDeviceObject::IDeviceObject()
    {
        DEBUG_CHECK_EX(!m_registered.load(), "Object already registered");
        m_device = helper::DeviceObjectRegistry::GetInstance().safeRegisterObject(this, m_registered);
    }

    IDeviceObject::~IDeviceObject()
    {
        if (1 == m_registered.exchange(0))
            helper::DeviceObjectRegistry::GetInstance().unregisterObject(this);
    }

    void IDeviceObject::switchDeviceOwnership(IDriver* device)
    {
        if (m_device != device)
        {
            if (m_device)
                handleDeviceRelease();

            m_device = nullptr;

            if (device)
            {
                m_device = device;
                handleDeviceReset();
            }
        }
    }

    //---

    void IDriver::ChangeActiveDevice(IDriver* device)
    {
        ASSERT_EX(Fibers::GetInstance().isMainThread(), "Only allowed to be called from main thread");
        helper::DeviceObjectRegistry::GetInstance().safeChangeDevice(device);
    }

    //---

} // rendering