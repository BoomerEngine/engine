/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#include "build.h"
#include "renderingMaterial.h"
#include "renderingMaterialRuntimeService.h"
#include "renderingMaterialRuntimeLayout.h"

#include "rendering/device/include/renderingDeviceService.h"
#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingCommandWriter.h"
#include "renderingMaterialRuntimeProxy.h"

namespace rendering
{

    ///---

    IMaterialDataProxyListener::~IMaterialDataProxyListener()
    {}

    bool IMaterialDataProxyListener::PatchMaterialProxy(MaterialDataProxyPtr& currentPointer, const MaterialDataProxyChangesRegistry& changes)
    {
        bool changed = false;

        if (currentPointer)
        {
            while (const auto* newPointer = changes.find(currentPointer.get()))
            {
                currentPointer = *newPointer;
                changed = true;
            }
        }

        return changed;                
    }

    ///---

    RTTI_BEGIN_TYPE_CLASS(MaterialService);
        RTTI_METADATA(base::app::DependsOnServiceMetadata).dependsOn<rendering::DeviceService>();
    RTTI_END_TYPE();

    ///----

    MaterialService::MaterialService()
    {}

    base::app::ServiceInitializationResult MaterialService::onInitializeService(const base::app::CommandLine& cmdLine)
    {
        auto deviceService = base::GetService<DeviceService>();
        if (!deviceService)
            base::app::ServiceInitializationResult::FatalError;

        auto device = deviceService->device();

        m_dataLayouts.reserve(65535);
        m_dataLayoutsMap.reserve(65535);

        {
            base::Array<MaterialDataLayoutEntry> emptyLayout;
            registerDataLayout(std::move(emptyLayout));
        }
        
        return base::app::ServiceInitializationResult::Finished;
    }

    void MaterialService::onShutdownService()
    {
        m_closed = true;
    }

    void MaterialService::onSyncUpdate()
    {
        dispatchMaterialProxyChanges();
    }

	//--

	MaterialBindlessTextureID MaterialService::queryBindlessTextureId(const base::res::IResource& res) const
	{
		return 0; // for now
	}

    //--

    void MaterialService::prepareForFrame(command::CommandWriter& cmd)
    {

    }

    void MaterialService::prepareForDraw(command::CommandWriter& cmd) const
    {
    }

    ///---

    void MaterialService::registerMaterialProxyChangeListener(IMaterialDataProxyListener* listener)
    {
        if (listener)
        {
            auto lock = base::CreateLock(m_changingMaterialProxyListenersLock);
            m_changingMaterialProxyListeners.pushBackUnique(listener);
        }
    }

    void MaterialService::unregisterMaterialProxyChangeListener(IMaterialDataProxyListener* listener)
    {
        auto lock = base::CreateLock(m_changingMaterialProxyListenersLock);
        auto index = m_changingMaterialProxyListeners.find(listener);
        if (index != INDEX_NONE)
            m_changingMaterialProxyListeners[index] = nullptr;
    }
    
    void MaterialService::dispatchMaterialProxyChanges()
    {
        // get change list
        m_changedMaterialProxiesLock.acquire();
        auto changedProxies = std::move(m_changedMaterialProxies);
        m_changedMaterialProxiesLock.release();

        // dispatch changes
        if (!changedProxies.empty())
        {
            PC_SCOPE_LVL0(DispatchMaterialProxyChanges);

            base::ScopeTimer timer;
            
            TRACE_INFO("Found {} changed material data proxies", changedProxies.size());

            {
                auto lock = CreateLock(m_changingMaterialProxyListenersLock);
                for (auto* listener : m_changingMaterialProxyListeners)
                    if (listener)
                        listener->handleMaterialProxyChanges(changedProxies);

                m_changingMaterialProxyListeners.remove(nullptr);
            }

            TRACE_INFO("Changed {} material data proxies in {}", changedProxies.size(), timer);
        }
    }

    void MaterialService::notifyMaterialProxyChanged(MaterialDataProxy* currentProxy, const MaterialDataProxyPtr& newProxy)
    {
		DEBUG_CHECK_RETURN_EX(newProxy && currentProxy, "Invalid proxy change");
        DEBUG_CHECK_RETURN_EX(newProxy != currentProxy, "Proxy should not change to itself");
		DEBUG_CHECK_RETURN_EX(newProxy->templateProxy() != currentProxy->templateProxy(), "Proxy should not change if it has the same material template");

        {
            auto lock = base::CreateLock(m_changedMaterialProxiesLock);
            m_changedMaterialProxies[currentProxy] = newProxy;
        }
    }

    ///---

    const MaterialDataLayout* MaterialService::registerDataLayout(base::Array<MaterialDataLayoutEntry>&& entries)
    {
        // calculate layout hash
        base::CRC64 crc;
        for (const auto& entry : entries)
        {
            crc << (char)entry.type;
            crc << entry.name.view();
            //crc << entry.dataOffset;
            //crc << entry.resourcePlacement;
        }

        const auto key = crc.crc();

        auto lock = base::CreateLock(m_dataLayoutsLock);

        // already registered ?
        if (const auto* entry = m_dataLayoutsMap.find(key))
            return entry->get();

        // create 
        auto id = m_dataLayouts.size();
        auto layout = base::RefNew<MaterialDataLayout>(id, std::move(entries));
        m_dataLayouts.pushBack(layout);
        m_dataLayoutsMap[key] = layout;
        TRACE_SPAM("Registred material layout ID {}:\n", id, *layout);
        return layout;
    }


    ///---

} // rendering