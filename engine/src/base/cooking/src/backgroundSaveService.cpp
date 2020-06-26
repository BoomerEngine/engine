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

#include "base/depot/include/depotStructure.h"
#include "base/resources/include/resourceLoadingService.h"
#include "base/app/include/commandline.h"

namespace base
{
    namespace cooker
    {
        //--

        RTTI_BEGIN_TYPE_CLASS(BackgroundSaver);
            RTTI_METADATA(app::DependsOnServiceMetadata).dependsOn<base::res::LoadingService>();
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
            auto loaderService = GetService<res::LoadingService>();
            if (!loaderService)
            {
                TRACE_ERROR("No resource loading service spawned, no editor can be created");
                return app::ServiceInitializationResult::Silenced;
            }

            // we need a cooker based loader to support resource edition
            auto loader = rtti_cast<cooker::ResourceLoaderCooker>(loaderService->loader());
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

        void BackgroundSaver::scheduleSave(const StringBuf& depotPath, const res::ResourcePtr& data)
        {
            m_saveThread->scheduleSave(depotPath, data);
        }

        //--

    } // depot
} // base
