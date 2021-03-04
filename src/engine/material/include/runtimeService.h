/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#pragma once

#include "gpu/device/include/managedBuffer.h"
#include "gpu/device/include/managedBufferWithAllocator.h"

#include "core/app/include/localService.h"
#include "core/containers/include/blockPool.h"
#include "core/containers/include/staticStructurePool.h"

BEGIN_BOOMER_NAMESPACE()

//---

// proxy change registry
typedef HashMap<RefWeakPtr<MaterialDataProxy>, MaterialDataProxyPtr> MaterialDataProxyChangesRegistry;

// a listener that is interested in material data proxy changes
class ENGINE_MATERIAL_API IMaterialDataProxyListener : public NoCopy
{
public:
    virtual ~IMaterialDataProxyListener();

    /// notify about list of proxy changes
    virtual void handleMaterialProxyChanges(const MaterialDataProxyChangesRegistry& changedProxies) = 0;

    //--

    // patch material reference
    static bool PatchMaterialProxy(MaterialDataProxyPtr& currentPointer, const MaterialDataProxyChangesRegistry& changes);
};

//---

struct MaterialDataLayoutEntry;

// service holding and managing all material parameters and templates
class ENGINE_MATERIAL_API MaterialService : public app::ILocalService
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialService, app::ILocalService);

public:
    MaterialService();

    //--

    /// register material data layout, returns unique ID
    /// NOTE: layouts are never unregistered (we don't have that many)
    const MaterialDataLayout* registerDataLayout(Array<MaterialDataLayoutEntry>&& entries);

    //--

    /// push changed for frame rendering
    void prepareForFrame(gpu::CommandWriter& cmd);

    /// prepare for drawing in a given command buffer - usually just bind the index buffer
    void prepareForDraw(gpu::CommandWriter& cmd) const;

    //--

    /// apply all material proxy changed
    void dispatchMaterialProxyChanges();

    /// register material for data proxy recreation at next possible moment (usually during the tick)
    /// NOTE: this is mostly editor-only functionality when materials are edited, should not happen at runtime
    void notifyMaterialProxyChanged(MaterialDataProxy* currentProxy, const MaterialDataProxyPtr& newProxy);

    /// register a listener for material proxy changes
    void registerMaterialProxyChangeListener(IMaterialDataProxyListener* listener);

    /// unregister a listener for material proxy changes
    void unregisterMaterialProxyChangeListener(IMaterialDataProxyListener* listener);

    //--

	/// query texture's bindless index, NOTE: texture must be registered first
	MaterialBindlessTextureID queryBindlessTextureId(const IResource& res) const;

	//--

private:
    virtual app::ServiceInitializationResult onInitializeService(const app::CommandLine& cmdLine) override final;
    virtual void onShutdownService() override final;
    virtual void onSyncUpdate() override final;

    static const uint32_t MAX_DATA_LAYOUTS_ABSOLUTE_LIMIT = 65535;

    HashMap<uint64_t, MaterialDataLayoutPtr> m_dataLayoutsMap;
    Array<MaterialDataLayoutPtr> m_dataLayouts;
    SpinLock m_dataLayoutsLock;

    //--

    SpinLock m_changedMaterialProxiesLock;
    MaterialDataProxyChangesRegistry m_changedMaterialProxies;

    Mutex m_changingMaterialProxyListenersLock;
    Array<IMaterialDataProxyListener*> m_changingMaterialProxyListeners;

    //--
        
    bool m_closed = false;
};

//---

END_BOOMER_NAMESPACE()
