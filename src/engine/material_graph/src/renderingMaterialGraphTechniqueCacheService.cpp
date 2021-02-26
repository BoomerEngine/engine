/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shaders #]
*/

#include "build.h"
#include "renderingMaterialGraph.h"
#include "renderingMaterialGraphTechniqueCompiler.h"
#include "renderingMaterialGraphTechniqueCacheService.h"

#include "gpu/device/include/renderingDeviceService.h"
#include "engine/material/include/renderingMaterialRuntimeTechnique.h"
#include "core/resource/include/resourceLoadingService.h"
#include "core/system/include/thread.h"

BEGIN_BOOMER_NAMESPACE()

//---

RTTI_BEGIN_TYPE_CLASS(MaterialTechniqueCacheService);
RTTI_END_TYPE();

MaterialTechniqueCacheService::MaterialTechniqueCacheService()
{}

MaterialTechniqueCacheService::~MaterialTechniqueCacheService()
{}

app::ServiceInitializationResult MaterialTechniqueCacheService::onInitializeService(const app::CommandLine& cmdLine)
{
    return app::ServiceInitializationResult::Finished;
}

MaterialTechniqueCacheService::TechniqueInfo::~TechniqueInfo()
{
    TRACE_INFO("Releasing '{}'", contextName);
}

void MaterialTechniqueCacheService::onShutdownService()
{
    for (;;)
    {
        {
            auto lock = CreateLock(m_compilationJobsLock);
            if (m_compilationJobs.empty())
                break;
        }

        // TODO: add explicit cancellation

        TRACE_INFO("There are still shader that are compiling, the must stop before we can exit");
        Sleep(500);
    }

    m_allTechniques.clearPtr();
}

void MaterialTechniqueCacheService::onSyncUpdate()
{
}

//---

void MaterialTechniqueCacheService::requestTechniqueCompilation(StringView contextName, const MaterialGraphContainerPtr& graph, MaterialTechnique* technique)
{
    auto* info = new TechniqueInfo;
    info->contextName = StringBuf(contextName);
    info->graph = graph;
    info->technique = technique;

    {
        auto lock = CreateLock(m_allTechniquesMapLock);
        m_allTechniques.pushBack(info);
    }

    requestTechniqueCompilation(info);
}

void MaterialTechniqueCacheService::requestTechniqueCompilation(TechniqueInfo* info)
{
    if (auto technique = info->technique.lock())
    {
        // create compiler
        auto compiler = new MaterialTechniqueCompiler(info->contextName, info->graph, technique->setup(), technique);

        // keep track of compilers for the duration of compilation
        {
            auto lock = CreateLock(m_compilationJobsLock);
            m_compilationJobs.pushBack(compiler);
            TRACE_INFO("Requested '{}' for '{}'", technique->setup(), info->contextName);
        }

        // run compilation on fiber
        RunFiber("CompileMaterialTechnique") << [this, info, compiler](FIBER_FUNC)
        {
            processTechniqueCompilation(info, *compiler);

            {
                auto lock = CreateLock(m_compilationJobsLock);
                m_compilationJobs.remove(compiler);
            }

            delete compiler;
        };
    }
}

void MaterialTechniqueCacheService::processTechniqueCompilation(TechniqueInfo* owner, MaterialTechniqueCompiler& compiler)
{
    compiler.compile();
}

//---

uint64_t MaterialTechniqueCacheService::CalcMergedKey(StringView contextName, uint64_t graphKey, const MaterialCompilationSetup& setup)
{
    CRC64 crc;
    crc << contextName;
    crc << graphKey;
    crc << setup.key();
    return crc;
}

//--

END_BOOMER_NAMESPACE()
