/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#pragma once

#include "rendering/device/include/renderingManagedBuffer.h"
#include "rendering/device/include/renderingManagedBufferWithAllocator.h"
#include "base/app/include/localService.h"
#include "base/containers/include/blockPool.h"
#include "base/containers/include/staticStructurePool.h"

namespace rendering
{

    //---

    // proxy change registry
    typedef base::HashMap<base::RefWeakPtr<MaterialDataProxy>, MaterialDataProxyPtr> MaterialDataProxyChangesRegistry;

    // a listener that is interested in material data proxy changes
    class RENDERING_MATERIAL_API IMaterialDataProxyListener : public base::NoCopy
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
    class RENDERING_MATERIAL_API MaterialService : public base::app::ILocalService
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialService, base::app::ILocalService);

    public:
        MaterialService();

        //--

        /// register material data layout, returns unique ID
        /// NOTE: layouts are never unregistered (we don't have that many)
        const MaterialDataLayout* registerDataLayout( base::Array<MaterialDataLayoutEntry>&& entries);

        //--

        /// push changed for frame rendering
        void prepareForFrame(command::CommandWriter& cmd);

        /// prepare for drawing in a given command buffer - usually just bind the index buffer
        void prepareForDraw(command::CommandWriter& cmd) const;

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
		MaterialBindlessTextureID queryBindlessTextureId(const base::res::IResource& res) const;

		//--

    private:
        virtual base::app::ServiceInitializationResult onInitializeService(const base::app::CommandLine& cmdLine) override final;
        virtual void onShutdownService() override final;
        virtual void onSyncUpdate() override final;

        static const uint32_t MAX_DATA_LAYOUTS_ABSOLUTE_LIMIT = 65535;

        base::HashMap<uint64_t, MaterialDataLayoutPtr> m_dataLayoutsMap;
        base::Array<MaterialDataLayoutPtr> m_dataLayouts;
        base::SpinLock m_dataLayoutsLock;

        //--

        base::SpinLock m_changedMaterialProxiesLock;
        MaterialDataProxyChangesRegistry m_changedMaterialProxies;

        base::Mutex m_changingMaterialProxyListenersLock;
        base::Array<IMaterialDataProxyListener*> m_changingMaterialProxyListeners;

        //--
        
        bool m_closed = false;
    };

    //---

} // rendering