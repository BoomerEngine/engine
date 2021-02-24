/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#include "build.h"
#include "scriptService.h"
#include "scriptEnvironment.h"
#include "scriptCompiledProject.h"

#include "base/app/include/configService.h"
#include "base/app/include/commandline.h"

BEGIN_BOOMER_NAMESPACE(base::script)

//---

RTTI_BEGIN_TYPE_CLASS(ScriptService);
    RTTI_METADATA(base::app::DependsOnServiceMetadata).dependsOn<config::ConfigService>();
RTTI_END_TYPE();

ScriptService::ScriptService()
    : m_scriptsDisabled(false)
{}


app::ServiceInitializationResult ScriptService::onInitializeService( const app::CommandLine& cmdLine)
{
    // create script env
    m_env.create();

    // scripts are disabled, don't do anything else
    if (cmdLine.hasParam("noscripts"))
    {
        TRACE_WARNING("Scripts are disabled via commandline and will not be loaded");
        m_scriptsDisabled = true;
        return app::ServiceInitializationResult::Finished;
    }

    // scripts initialized
    return app::ServiceInitializationResult::Finished;
}

void ScriptService::onShutdownService()
{
    // clear all script object data

    // cleanup and unbind all objects

    // unload all modules

    // destroy
}

void ScriptService::onSyncUpdate()
{

}

//---

bool ScriptService::loadScripts()
{
    // disabled scripts, no loading
    if (m_scriptsDisabled)
        return true;

    // load packages
    if (!m_env->load())
        return false;

    // we had loaded scripts now
    m_scriptsLoaded = true;
    return true;
}

END_BOOMER_NAMESPACE(base::script)
