/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#include "build.h"
#include "material.h"
#include "runtimeService.h"
#include "runtimeLayout.h"
#include "runtimeProxy.h"

#include "gpu/device/include/deviceService.h"
#include "gpu/device/include/device.h"
#include "gpu/device/include/commandWriter.h"

BEGIN_BOOMER_NAMESPACE()

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
    RTTI_METADATA(DependsOnServiceMetadata).dependsOn<gpu::DeviceService>();
RTTI_END_TYPE();

///----

MaterialService::MaterialService()
{}

bool MaterialService::onInitializeService(const CommandLine& cmdLine)
{
    auto deviceService = GetService<DeviceService>();
    if (!deviceService)
        false;

    auto device = deviceService->device();

    m_dataLayouts.reserve(65535);
    m_dataLayoutsMap.reserve(65535);

    {
        Array<MaterialDataLayoutEntry> emptyLayout;
        registerDataLayout(std::move(emptyLayout));
    }
        
    return true;
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

MaterialBindlessTextureID MaterialService::queryBindlessTextureId(const IResource& res) const
{
	return 0; // for now
}

//--

void MaterialService::prepareForFrame(gpu::CommandWriter& cmd)
{

}

void MaterialService::prepareForDraw(gpu::CommandWriter& cmd) const
{
}

///---

void MaterialService::registerMaterialProxyChangeListener(IMaterialDataProxyListener* listener)
{
    if (listener)
    {
        auto lock = CreateLock(m_changingMaterialProxyListenersLock);
        m_changingMaterialProxyListeners.pushBackUnique(listener);
    }
}

void MaterialService::unregisterMaterialProxyChangeListener(IMaterialDataProxyListener* listener)
{
    auto lock = CreateLock(m_changingMaterialProxyListenersLock);
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

        ScopeTimer timer;
            
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
        auto lock = CreateLock(m_changedMaterialProxiesLock);
        m_changedMaterialProxies[currentProxy] = newProxy;
    }
}

///---

const MaterialDataLayout* MaterialService::registerDataLayout(Array<MaterialDataLayoutEntry>&& entries)
{
    // calculate layout hash
    CRC64 crc;
    for (const auto& entry : entries)
    {
        crc << (char)entry.type;
        crc << entry.name.view();
        //crc << entry.dataOffset;
        //crc << entry.resourcePlacement;
    }

    const auto key = crc.crc();

    auto lock = CreateLock(m_dataLayoutsLock);

    // already registered ?
    if (const auto* entry = m_dataLayoutsMap.find(key))
        return entry->get();

    // create 
    auto id = m_dataLayouts.size();
    auto layout = RefNew<MaterialDataLayout>(id, std::move(entries));
    m_dataLayouts.pushBack(layout);
    m_dataLayoutsMap[key] = layout;
    TRACE_SPAM("Registred material layout ID {}:\n", id, *layout);
    return layout;
}

///---

END_BOOMER_NAMESPACE()
