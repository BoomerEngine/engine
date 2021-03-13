/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#pragma once

#include "core/app/include/localService.h"

BEGIN_BOOMER_NAMESPACE_EX(script)

//----

class Environment;

// top level scripting service, manages all live scripting threads and objects, con
class CORE_SCRIPT_API ScriptService : public IService
{
    RTTI_DECLARE_VIRTUAL_CLASS(ScriptService, IService);

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

    // IService
    virtual bool onInitializeService( const CommandLine& cmdLine) override final;
    virtual void onShutdownService() override final;
    virtual void onSyncUpdate() override final;
};

END_BOOMER_NAMESPACE_EX(script)

//---