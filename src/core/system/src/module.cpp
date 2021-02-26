/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#include "build.h"
#include "module.h"
#include "atomic.h"

BEGIN_BOOMER_NAMESPACE_EX(modules)

static const uint32_t MAX_MODULES = 512;

static ModuleInfo GModules[MAX_MODULES];
static std::atomic<uint32_t> GModulesCount(0);

void RegisterModule(const char* name, const char* dateCompiled, const char* timeCompiled, uint64_t compilerVer, void* moduleHandle)
{
    auto index = GModulesCount++;
    if (index < MAX_MODULES)
    {
        GModules[index].m_name = name;
        GModules[index].m_dateCompiled = dateCompiled;
        GModules[index].m_timeCompiled = timeCompiled;
        GModules[index].m_compilerVersion = compilerVer;
        GModules[index].m_moduleHandle = moduleHandle;
    }
}

void GetRegisteredModules(uint32_t maxEntries, ModuleInfo* outTable, uint32_t& outNumEntries)
{
    auto count = std::min<uint32_t>(maxEntries, GModulesCount.load());
    outNumEntries = count;

    for (uint32_t i = 0; i < count; ++i)
        outTable[i] = GModules[i];
}

void LoadDynamicModule(const char* name)
{
#if !defined(PLATFORM_WINDOWS)
    TRACE_ERROR("Loading dynamic module '{}' is only supported on Windows", name);
#elif defined(BUILD_AS_LIBS)
    TRACE_ERROR("Unable to load dynamic module '{}' when built with static libs");
#else
    char moduleFileName[512];
    strcpy_s(moduleFileName, ARRAYSIZE(moduleFileName), name);
    strcat_s(moduleFileName, ARRAYSIZE(moduleFileName), ".dll");

    auto handle = LoadLibraryA(moduleFileName);
    if (handle == NULL)
    {
        auto errorCode = GetLastError();
        FATAL_ERROR(TempString("Failed to load dynamic module '{}', error code = {}", name, Hex(errorCode)));
    }
    else
    {
        TRACE_INFO("Dynamic module '{}' loaded at 0x%08p", name, handle);
    }
#endif
}

bool HasModuleLoaded(const char* name)
{
    // look for module
    auto count = GModulesCount.load();
    for (uint32_t i=0; i<count; ++i)
    {
        if (0 == strcmp(GModules[i].m_name, name))
            return true;
    }

    // module is not loaded
    return false;
}

END_BOOMER_NAMESPACE_EX(modules)
