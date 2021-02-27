/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shaders #]
***/

#pragma once

#include "core/app/include/localService.h"

BEGIN_BOOMER_NAMESPACE()

//---

class MaterialTechniqueCompiler;

// local service for compiling techniques for materials, only reason it's a service is that we need to observer the depot
class MaterialTechniqueCacheService : public app::ILocalService
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialTechniqueCacheService, app::ILocalService);

public:
    MaterialTechniqueCacheService();
    virtual ~MaterialTechniqueCacheService();

    /// request compilation of given material technique
    void requestTechniqueCompilation(StringView contextName, const MaterialGraphContainerPtr& graph, MaterialTechnique* technique);

protected:
    // ILocalService
    virtual app::ServiceInitializationResult onInitializeService(const app::CommandLine& cmdLine) override final;
    virtual void onShutdownService() override final;
    virtual void onSyncUpdate() override final;

    //---

    static uint64_t CalcMergedKey(StringView contextName, uint64_t graphKey, const MaterialCompilationSetup& setup);

    //---

    struct TechniqueInfo : public NoCopy
    {
        RTTI_DECLARE_POOL(POOL_RENDERING_TECHNIQUE)

    public:
        uint64_t key = 0;

        RefWeakPtr<MaterialTechnique> technique; // may be lost
        StringBuf contextName;
        MaterialGraphContainerPtr graph;

        ~TechniqueInfo();
    };

    Mutex m_allTechniquesMapLock;
    //HashMap<uint64_t, TechniqueInfo*> m_allTechniquesMap;
    Array<TechniqueInfo*> m_allTechniques;

    void requestTechniqueCompilation(TechniqueInfo* info);
    void processTechniqueCompilation(TechniqueInfo* owner, MaterialTechniqueCompiler& compiler);

    //--

    Array<MaterialTechniqueCompiler*> m_compilationJobs;
    Mutex m_compilationJobsLock;

    //---

    void notifyFileChanged(const StringBuf& path);
};

//---

END_BOOMER_NAMESPACE()

