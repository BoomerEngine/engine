/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: service #]
***/

#pragma once

namespace base
{
    namespace res
    {
        //--

        class BackgroundSaveThread;

        /// service for saving data in the background
        class BASE_RESOURCE_COMPILER_API BackgroundSaver : public app::ILocalService
        {
            RTTI_DECLARE_VIRTUAL_CLASS(BackgroundSaver, app::ILocalService);

        public:
            BackgroundSaver();
            virtual ~BackgroundSaver();

            /// schedule new content for saving
            void scheduleSave(const StringBuf& depotPath, const ResourcePtr& data);

        private:
            virtual app::ServiceInitializationResult onInitializeService(const app::CommandLine& cmdLine) override final;
            virtual void onShutdownService() override final;
            virtual void onSyncUpdate() override final;

            BackgroundSaveThread* m_saveThread;
        };

        //--

    } // res
} // base