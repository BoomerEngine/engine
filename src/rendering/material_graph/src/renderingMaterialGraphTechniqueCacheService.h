/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shaders #]
***/

#pragma once

#include "base/resource_compiler/include/depotStructure.h"
#include "base/app/include/localService.h"

namespace rendering
{
    //---

    class MaterialTechniqueCompiler;

    // local service for compiling techniques for materials, only reason it's a service is that we need to observer the depot
    class MaterialTechniqueCacheService : public base::app::ILocalService
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialTechniqueCacheService, base::app::ILocalService);

    public:
        MaterialTechniqueCacheService();
        virtual ~MaterialTechniqueCacheService();

        /// request compilation of given material technique
        void requestTechniqueCompilation(base::StringView contextName, const MaterialGraphContainerPtr& graph, MaterialTechnique* technique);

    protected:
        // ILocalService
        virtual base::app::ServiceInitializationResult onInitializeService(const base::app::CommandLine& cmdLine) override final;
        virtual void onShutdownService() override final;
        virtual void onSyncUpdate() override final;

        //---

        static uint64_t CalcMergedKey(base::StringView contextName, uint64_t graphKey, const MaterialCompilationSetup& setup);

        //---

        struct TechniqueInfo : public base::NoCopy
        {
            RTTI_DECLARE_POOL(POOL_RENDERING_TECHNIQUE)

        public:
            uint64_t key = 0;

            base::RefWeakPtr<MaterialTechnique> technique; // may be lost
            base::StringBuf contextName;
            MaterialGraphContainerPtr graph;

            ~TechniqueInfo();
        };

        base::Mutex m_allTechniquesMapLock;
        //base::HashMap<uint64_t, TechniqueInfo*> m_allTechniquesMap;
        base::Array<TechniqueInfo*> m_allTechniques;

        void requestTechniqueCompilation(TechniqueInfo* info);
        void processTechniqueCompilation(TechniqueInfo* owner, MaterialTechniqueCompiler& compiler);

        //--

        base::Array<MaterialTechniqueCompiler*> m_compilationJobs;
        base::Mutex m_compilationJobsLock;

        //---

        void notifyFileChanged(const base::StringBuf& path);
    };

    //---

} // rendering

