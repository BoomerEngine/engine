/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: service #]
***/

#include "build.h"
#include "backgroundSaveService.h"
#include "backgroundSaveThread.h"

#include "cooker.h"
#include "cookerResourceLoader.h"

#include "base/resource_compiler/include/depotStructure.h"
#include "base/resource/include/resourceLoadingService.h"
#include "base/app/include/commandline.h"

namespace base
{
    namespace res
    {
        //--

        RTTI_BEGIN_TYPE_CLASS(BackgroundSaver);
            RTTI_METADATA(app::DependsOnServiceMetadata).dependsOn<LoadingService>();
        RTTI_END_TYPE();

        //--

        BackgroundSaver::BackgroundSaver()
            : m_saveThread(nullptr)
        {}

        BackgroundSaver::~BackgroundSaver()
        {
        }

        app::ServiceInitializationResult BackgroundSaver::onInitializeService(const app::CommandLine& cmdLine)
        {
            // only use in editor
            if (!cmdLine.hasParam("editor"))
                return app::ServiceInitializationResult::Silenced;

            // we can initialize the managed depot only if we are running from uncooked data
            auto loaderService = GetService<LoadingService>();
            if (!loaderService)
            {
                TRACE_ERROR("No resource loading service spawned, no editor can be created");
                return app::ServiceInitializationResult::Silenced;
            }

            // we need a cooker based loader to support resource edition
            auto loader = rtti_cast<ResourceLoaderCooker>(loaderService->loader());
            if (!loader)
            {
                TRACE_ERROR("Resource loader does not support cooking from depot, no editor can be created");
                return app::ServiceInitializationResult::Silenced;
            }

            // create the saving thread
            m_saveThread = MemNew(BackgroundSaveThread, loader->depot());
            return app::ServiceInitializationResult::Finished;
        }

        void BackgroundSaver::onShutdownService()
        {
            if (m_saveThread)
            {
                MemDelete(m_saveThread);
                m_saveThread = nullptr;
            }
        }

        void BackgroundSaver::onSyncUpdate()
        {
        }

        void BackgroundSaver::scheduleSave(const StringBuf& depotPath, const ResourcePtr& data)
        {
            m_saveThread->scheduleSave(depotPath, data);
        }

        //--

    } // depot
} // base
