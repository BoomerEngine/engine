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

#include "rendering/driver/include/renderingDeviceService.h"
#include "rendering/material/include/renderingMaterialRuntimeTechnique.h"
#include "base/resource/include/resourceLoadingService.h"
#include "base/system/include/thread.h"

namespace rendering
{
    //---

    RTTI_BEGIN_TYPE_CLASS(MaterialTechniqueCacheService);
        RTTI_METADATA(base::app::DependsOnServiceMetadata).dependsOn<rendering::DeviceService>();
        RTTI_METADATA(base::app::DependsOnServiceMetadata).dependsOn<base::res::LoadingService>();
    RTTI_END_TYPE();

    MaterialTechniqueCacheService::MaterialTechniqueCacheService()
    {}

    MaterialTechniqueCacheService::~MaterialTechniqueCacheService()
    {}

    base::app::ServiceInitializationResult MaterialTechniqueCacheService::onInitializeService(const base::app::CommandLine& cmdLine)
    {
        // find the loading service and the depot - we will need it to compile shaders ad hoc
        auto loadingService = base::GetService<base::res::LoadingService>();
        if (!loadingService)
        {
            TRACE_ERROR("Loading service not initialized");
            return base::app::ServiceInitializationResult::FatalError;
        }

        // we should have the depot
        m_depot = loadingService->loader()->queryUncookedDepot();
        if (!m_depot)
        {
            TRACE_ERROR("Shader cooking via shader cache requires uncooked depot access");
            return base::app::ServiceInitializationResult::FatalError;
        }

        // observer changes in the depot
        m_depot->attachObserver(this);

        // TODO: load global and local shader cache
        // TODO: when active device changes change the shader cache
        //m_globalCache = base::CreateSharedPtr<ShaderCache>();
        //m_localCache = base::CreateSharedPtr<ShaderCache>();

        return base::app::ServiceInitializationResult::Finished;
    }

    MaterialTechniqueCacheService::TechniqueInfo::~TechniqueInfo()
    {
        TRACE_INFO("Releasing '{}'", contextName);
    }

    void MaterialTechniqueCacheService::onShutdownService()
    {
        m_depot->detttachObserver(this);

        for (;;)
        {
            {
                auto lock = base::CreateLock(m_compilationJobsLock);
                if (m_compilationJobs.empty())
                    break;
            }

            // TODO: add explicit cancellation

            TRACE_INFO("There are still shader that are compiling, the must stop before we can exit");
            base::Sleep(500);
        }

        m_sourceFileMap.clearPtr();
        m_allTechniques.clearPtr();
    }

    void MaterialTechniqueCacheService::onSyncUpdate()
    {
        // get list of files that changed
        m_changedFilesLock.acquire();
        auto changedFiles = std::move(m_changedFiles);
        m_changedFilesLock.release();

        // invalidate all proxies that used changed files to compile
        if (!changedFiles.empty())
        {
            TRACE_INFO("Found {} changed shader files", changedFiles.size());

            uint32_t numInvalidatedTechniques = 0;
            for (auto* file : changedFiles.keys())
            {
                auto lock = base::CreateLock(file->usersLock);

                numInvalidatedTechniques += file->users.size();
                for (auto* techniqueInfo : file->users)
                    requestTechniqueCompilation(techniqueInfo);
            }

            TRACE_INFO("Invalidated '{}' material techniques", numInvalidatedTechniques);
        }
    }


    //---

    void MaterialTechniqueCacheService::requestTechniqueCompilation(base::StringView<char> contextName, const MaterialGraphContainerPtr& graph, MaterialTechnique* technique)
    {
        auto* info = MemNew(TechniqueInfo).ptr;
        info->contextName = base::StringBuf(contextName);
        info->graph = graph;
        info->technique = technique;

        {
            auto lock = base::CreateLock(m_allTechniquesMapLock);
            m_allTechniques.pushBack(info);
        }

        requestTechniqueCompilation(info);
    }

    void MaterialTechniqueCacheService::requestTechniqueCompilation(TechniqueInfo* info)
    {
        if (auto technique = info->technique.lock())
        {
            // create compiler
            auto compiler = MemNew(MaterialTechniqueCompiler, *m_depot, info->contextName, info->graph, technique->setup(), technique).ptr;

            // keep track of compilers for the duration of compilation
            {
                auto lock = base::CreateLock(m_compilationJobsLock);
                m_compilationJobs.pushBack(compiler);
                TRACE_INFO("Requested '{}' for '{}'", technique->setup(), info->contextName);
            }

            // run compilation on fiber
            RunFiber("CompileMaterialTechnique") << [this, info, compiler](FIBER_FUNC)
            {
                processTechniqueCompilation(info, *compiler);

                {
                    auto lock = base::CreateLock(m_compilationJobsLock);
                    m_compilationJobs.remove(compiler);
                }

                MemDelete(compiler);
            };
        }
    }

    void MaterialTechniqueCacheService::processTechniqueCompilation(TechniqueInfo* owner, MaterialTechniqueCompiler& compiler)
    {
        if (compiler.compile())
        {
            // track dependencies so when source file changes we will recompile the technique
            for (const auto& dep : compiler.usedFiles())
            {
                if (auto* fileInfo = getFileInfo(dep.depotPath))
                {
                    auto lock = CreateLock(fileInfo->usersLock);
                    fileInfo->users.insert(owner);
                }
            }
        }
    }

    //---

    uint64_t MaterialTechniqueCacheService::CalcMergedKey(base::StringView<char> contextName, uint64_t graphKey, const MaterialCompilationSetup& setup)
    {
        base::CRC64 crc;
        crc << contextName;
        crc << graphKey;
        crc << setup.key();
        return crc;
    }

    //--

    MaterialTechniqueCacheService::FileInfo* MaterialTechniqueCacheService::getFileInfo(const base::StringView<char> depotPath)
    {
        auto lock = base::CreateLock(m_sourceFileMapLock);

        FileInfo* ret = nullptr;
        if (m_sourceFileMap.find(depotPath, ret))
            return ret;

        base::io::TimeStamp timestamp;
        if (!m_depot->queryFileTimestamp(depotPath, timestamp))
        {
            TRACE_WARNING("File '{}' not recognized in depot", depotPath);
        }

        ret = MemNew(FileInfo);
        ret->depotPath = base::StringBuf(depotPath);
        ret->timestamp = timestamp.value();
        m_sourceFileMap[ret->depotPath] = ret;

        return ret;
    }

    void MaterialTechniqueCacheService::notifyFileChanged(base::StringView<char> depotFilePath)
    {
        auto lock = base::CreateLock(m_sourceFileMapLock);

        auto safePath = base::StringBuf(depotFilePath).toLower();

        FileInfo* ret = nullptr;
        if (m_sourceFileMap.find(safePath, ret))
        {
            TRACE_INFO("Tracked source shader file '{}' reported as modified", depotFilePath);

            auto lock2 = base::CreateLock(m_changedFilesLock);
            m_changedFiles.insert(ret);
        }
    }

    void MaterialTechniqueCacheService::notifyFileAdded(base::StringView<char> depotFilePath)
    {
        notifyFileChanged(depotFilePath);
    }

    void MaterialTechniqueCacheService::notifyFileRemoved(base::StringView<char> depotFilePath)
    {
        notifyFileChanged(depotFilePath);
    }

    //--


} // rendering
