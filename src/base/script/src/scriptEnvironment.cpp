/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#include "build.h"
#include "scriptEnvironment.h"
#include "scriptTypeRegistry.h"
#include "scriptCompiledProject.h"
#include "scriptPortableData.h"
#include "scriptFunctionRuntimeCode.h"
#include "scriptPortableStubs.h"
#include "scriptFunctionStackFrame.h"
#include "scriptClass.h"
#include "scriptObject.h"
#include "scriptLoader.h"
#include "scriptJIT.h"

#include "base/object/include/rttiClassRefType.h"
#include "base/object/include/rttiArrayType.h"
#include "base/object/include/rttiHandleType.h"
#include "base/config/include/configSystem.h"
#include "base/config/include/configGroup.h"
#include "base/config/include/configEntry.h"
#include "base/app/include/localServiceContainer.h"

BEGIN_BOOMER_NAMESPACE(base::script)

//---

extern void InitOpcodeTable();

Environment::Environment()
{
    InitOpcodeTable();
    m_types.create();
}

Environment::~Environment()
{
    m_types.reset();
}

/*static void EnumerateScriptPackages(Array<depot::PackageManifestScriptsReference>& outScriptPackageInfo)
{
    if (auto depotServce  = base::GetService<depot::DepotService>())
    {
        for (auto& manifest : depotServce->manifests())
        {
            for (auto& scriptPackageInfo : manifest->scriptPackages())
            {
                bool add = true;
                for (auto& existingInfo : outScriptPackageInfo)
                {
                    if (existingInfo.name == scriptPackageInfo.name && existingInfo.priority < scriptPackageInfo.priority)
                    {
                        existingInfo.priority = scriptPackageInfo.priority;
                        existingInfo.path = scriptPackageInfo.path;
                        existingInfo.fullPath = scriptPackageInfo.fullPath;
                        add = false;
                    }
                }

                if (add)
                    outScriptPackageInfo.pushBack(scriptPackageInfo);
            }
        }
    }
}*/

bool Environment::load()
{
    ScopeTimer timer;
#if 0
    // enumerate all scripts packages from all used projects
    //InplaceArray<depot::PackageManifestScriptsReference, 20> scriptPackageInfos;
    //EnumerateScriptPackages(scriptPackageInfos);
    //TRACE_INFO("Found {} script package(s) references in the depot manifests", scriptPackageInfos.size());

    // create package entries
    bool valid = true;
    Array<Package> packages;
    for (auto& entry : scriptPackageInfos)
    {
        auto& package = packages.emplaceBack();
        package.m_name = StringID(entry.name);
        package.m_path = entry.fullPath;

        // load the script definitions for package (striped down, imports only data)
        res::ResourcePathBuilder pathBuilder;
        /*pathBuilder
                .path(package.path.path())
                .param("exportAs", package.name.c_str())
                .param("importsOnly", "1")
                .buildPath();

        //package.imports = m_services.service<depot::DepotService>()->loadResource<CompiledProject>(importPath);

        // if this step fails it usually mean that there are some simple typing errors in the packages
        if (!package.imports)
        {
            TRACE_ERROR("Unable to load import definitions for package '{}'", package.name);
            valid = false;
        }*/
    }

    // failed to load imports, do not attempt script compilation
    if (!valid)
    {
        TRACE_ERROR("Failed to generate imports for some script packages, compilation stopped.");
        return false;
    }

    // compile all script packages
    for (auto& package : packages)
    {
        // load the script definitions for package (striped down, imports only data)
        /*auto compilePath  = res::ResourcePathBuilder()
                .path(package.path.path())
                .param("exportAs", package.name.c_str())
                .buildPath();*/

        //package.m_compiled = m_services.service<depot::DepotService>()->loadResource<CompiledProject>(compilePath);

        // if this step fails it usually mean that there are some simple typing errors in the packages
        if (!package.m_compiled)
        {
            TRACE_ERROR("Unable to load compiled package '{}'", package.m_name);
            valid = false;
        }
    }

    // failed to load compiled packages, do not attempt loading into RTTI
    if (!valid)
    {
        TRACE_ERROR("Failed to compile some of script packages, loading stopped.");
        return false;
    }

    // create the loader and insert data from all packages
    Loader loader(*m_types);
    for (auto& package : packages)
    {
        if (auto data  = package.m_compiled->data())
            loader.linkModule(*data);
    }

    // validate that we can safely loaded the script
    if (!loader.validate())
    {
        TRACE_ERROR("Not possible to load scripts");
        return false;
    }

    // TODO: capture object's state

    // prepare for script loading
    m_types->prepareForReload();

    // apply new data
    loader.createExports();

    // TODO: reapply object's state

    // swap current package list
    m_packages = std::move(packages);

    // we are done
    TRACE_INFO("Scripts loaded in {}", TimeInterval(timer.timeElapsed()));
#endif
    return true;
}

END_BOOMER_NAMESPACE(base::script)
