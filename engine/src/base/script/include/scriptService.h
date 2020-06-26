/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#pragma once

#include "base/app/include/localService.h"

namespace base
{
    namespace script
    {
        //----

        //----

        class Environment;

        // top level scripting service, manages all live scripting threads and objects, con
        class BASE_SCRIPT_API ScriptService : public app::ILocalService
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ScriptService, app::ILocalService);

        public:
            ScriptService();

            //---

            // get scripting environment, in case we need it for something directly
            INLINE Environment& environment() const { return *m_env; }

            //---

            // load/reload scripts
            // NOTE: scripts must be in a pre-compiled form already
            // MUST BE CALLED FROM MAIN THREAD with no script code running on threads
            bool loadScripts();

        private:
            UniquePtr<Environment> m_env;
            bool m_scriptsDisabled;
            bool m_scriptsLoaded;

            //--

            // ILocalService
            virtual app::ServiceInitializationResult onInitializeService( const app::CommandLine& cmdLine) override final;
            virtual void onShutdownService() override final;
            virtual void onSyncUpdate() override final;
        };

    } // script
} // base