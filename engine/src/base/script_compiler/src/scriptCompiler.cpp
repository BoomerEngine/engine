/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#include "build.h"
#include "scriptCompiler.h"
#include "scriptLibrary.h"
#include "scriptFileParser.h"
#include "scriptFunctionCodeParser.h"
#include "scriptFunctionCode.h"

#include "base/resources/include/resourceCookingInterface.h"
#include "base/resources/include/resource.h"
#include "base/resources/include/resourceFactory.h"
#include "base/script/include/scriptCompiledProject.h"
#include "base/script/include/scriptPortableData.h"
#include "base/containers/include/inplaceArray.h"
#include "base/depot/include/depotPackageManifest.h"
#include "base/app/include/localServiceContainer.h"
#include "base/memory/include/linearAllocator.h"

namespace base
{
    namespace script
    {

		//---

		mem::PoolID POOL_SCRIPT_COMPILER("Engine.Scripts.Compiler");

        //---

        /// log based erorr handler
        class LogErrorHandler : public IErrorHandler
        {
        public:
            LogErrorHandler()
                : m_numErrors(0)
                , m_numWarnings(0)
            {}

            virtual void reportError(const StringBuf& fullPath, uint32_t line, StringView<char> message) override final
            {
#ifdef BUILD_RELEASE
                fprintf(stderr, "%s(%d): error: %s\n", fullPath.c_str(), line, StringBuf(message).c_str());
#else
                TRACE_ERROR("{}({}): error: {}", fullPath, line, message);
#endif
                ++m_numErrors;
            }

            virtual void reportWarning(const StringBuf& fullPath, uint32_t line, StringView<char> message) override final
            {
                TRACE_WARNING("{}({}): warrning: {}", fullPath, line, message);
                ++m_numWarnings;
            }

            //--

            std::atomic<uint32_t> m_numErrors;
            std::atomic<uint32_t> m_numWarnings;
        };

        //---

        /// trans cooker that cooks the simple template file into a full material template
        class ScriptProjectCooker : public base::res::IResourceCooker
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ScriptProjectCooker, base::res::IResourceCooker);

        public:
            base::res::ResourceHandle exitWithMessage(StringView<char> moduleName, LogErrorHandler& errHandler, bool valid = true) const
            {
                if (errHandler.m_numErrors.load() || !valid)
                {
                    TRACE_ERROR("Compilation of '{}' Failed, {} error(s), {} warning(s)", moduleName, errHandler.m_numErrors.load(), errHandler.m_numWarnings.load());
                }
                else if (errHandler.m_numWarnings.load())
                {
                    TRACE_WARNING("Compilation of '{}' succeeded, {} error(s), {} warning(s)", moduleName, errHandler.m_numErrors.load(), errHandler.m_numWarnings.load());
                }
                else
                {
                    TRACE_INFO("Compilation of '{}' succeeded, {} error(s), {} warning(s)", moduleName, errHandler.m_numErrors.load(), errHandler.m_numWarnings.load());
                }

                return nullptr;
            }

            virtual base::res::ResourceHandle cook(base::res::IResourceCookerInterface& cooker) const override final
            {
                // get name of the compilation module
                auto moduleName = base::StringBuf("core");// cooker.queryResourcePath().param("exportAs");
                if (moduleName.empty())
                {
                    TRACE_ERROR("No 'exportAs' reference specified, unable to compile module");
                    return nullptr;
                }

                // imports only mode ?
                bool importsOnly = false;// cooker.queryResourcePath().hasParam("importsOnly");

                // get directory we are in
                InplaceArray<StringBuf, 200> scriptFilePaths;
                if (!cooker.discoverResolvedPaths("./", true, "bsc", scriptFilePaths))
                {
                    TRACE_ERROR("Failed to discover script files for module '{}' at '{}'", moduleName, cooker.queryResourcePath());
                    return nullptr;
                }

                // create the stub library
                mem::LinearAllocator mem(POOL_SCRIPT_COMPILER);
                StubLibrary stubs(mem, moduleName);
                LogErrorHandler errHandler;

                // load source code files
                bool allFilesProcessed = true;
                TRACE_INFO("Found {} script file(s) for project '{}' (loaded from '{}')", scriptFilePaths.size(), moduleName, cooker.queryResourcePath());
                for (auto &resolveFilePath : scriptFilePaths)
                {
                    // load file content
                    auto fileContent = cooker.loadToBuffer(resolveFilePath);
                    if (!fileContent)
                    {
                        TRACE_ERROR("Unable to load script file '{}'", resolveFilePath);
                        allFilesProcessed = false;
                        continue;
                    }

                    // get absolute file path
                    StringBuf fullPathName;
                    cooker.queryContextName(resolveFilePath, fullPathName);

                    // create the file stub
                    auto fileStub = stubs.createFile(stubs.primaryModule(), resolveFilePath, fullPathName);

                    // process file
                    FileParser parser(mem, errHandler, stubs);
                    allFilesProcessed &= parser.processCode(fileStub, fileContent);
                }

                // files failed to parse
                if (!exitWithMessage(moduleName, errHandler, allFilesProcessed))
                    return nullptr;

                // process stubs and compile functions
                if (!importsOnly)
                {
                    // discover the modules referenced by already parsed code, create the import stubs for the modules
                    HashMap<StringID, Array<const StubModuleImport*>> moduleImports;
                    for (auto file  : stubs.primaryModule()->files)
                        for (auto stub  : file->stubs)
                            if (auto mouduleImport  = stub->asModuleImport())
                                moduleImports[mouduleImport->name].pushBack(mouduleImport);

                    // load dependencies
                    for (auto& deps : moduleImports.values())
                    {
                        // load/create import
                        auto importedModule  = importModule(stubs, deps[0]->name);
                        if (importedModule)
                        {
                            for (auto import : deps)
                                import->m_importedModuleData = importedModule;
                        }
                        else if (deps[0]->location.file)
                        {
                            auto& dep = deps[0]->location;
                            errHandler.reportError(dep.file->absolutePath, dep.line, TempString("Unable to find package '{}'", deps[0]->name));
                        }
						else
						{
							TRACE_ERROR("Unable to find package '{}'", deps[0]->name);
						}
                    }

                    // files failed to parse
                    if (!exitWithMessage(moduleName, errHandler))
                        return false;

                    // resolve names
                    if (!stubs.buildNamedMaps(errHandler))
                        return exitWithMessage(moduleName, errHandler, false);

                    // resolve all types and fixup stub library
                    if (!stubs.validate(errHandler))
                        return exitWithMessage(moduleName, errHandler, false);

                    // compile functions in the primary module being compiled
                    Array<StubFunction *> allFunctions;
                    stubs.primaryModule()->extractGlobalFunctions(allFunctions);
                    stubs.primaryModule()->extractClassFunctions(allFunctions);
                    TRACE_INFO("Found {} functions to compile", allFunctions.size());

                    for (auto func : allFunctions)
                    {
                        // ignore imported functions
                        if (func->flags.test(StubFlag::Import))
                            continue;

                        // generate function code tree
                        FunctionCode code(mem, stubs, func);
                        FunctionParser parser(mem, errHandler, stubs);
                        if (!parser.processCode(func, code))
                            continue;

                        // compile function
                        if (!code.compile(errHandler))
                            continue;
                    }
                }
                else
                {
                    // just resolve names
                    if (!stubs.buildNamedMaps(errHandler))
                        return exitWithMessage(moduleName, errHandler, false);
                }

                // do not write if we had errors
                if (!exitWithMessage(moduleName, errHandler))
                    return false;

                // remove all unused data from the module
                stubs.pruneUnusedImports();

                // save the compiled data into the portable container from the module we just compiled
                // NOTE: this will suck in all other data referenced by the module (including other modules that we imported)
                // TODO: make sure that we only suck stuff that we actually USED
                auto data = PortableData::Create(stubs.primaryModule());
                if (!data)
                    return false;

                return base::CreateSharedPtr<CompiledProject>(data);
            }

            res::ResourcePath FindScriptImportPath(StringID name) const
            {
                int importPriority = 0;
                res::ResourcePath importPath;

                /*for (auto& manifest : base::GetService<depot::DepotService>()->manifests())
                {
                    for (auto& scriptPackage : manifest->scriptPackages())
                    {
                        if (name == scriptPackage.name && !scriptPackage.fullPath.empty())
                        {
                            if (importPath.empty() || scriptPackage.priority > importPriority)
                            {
                                importPath = scriptPackage.fullPath;
                                importPriority = scriptPackage.priority;
                            }
                        }
                    }
                }*/

                return importPath;
            }

            PortableDataPtr loadImportModule(StringID name) const
            {
                // find the module import path
                auto importPath = FindScriptImportPath(name);
                if (importPath.empty())
                {
                    TRACE_ERROR("Unable to locate script module '{}', make sure all packages and dependencies are set up correctly", name);
                    return nullptr;
                }
/*
                // create loading path
                // TODO: remove this hack :P
                auto path = res::ResourcePathBuilder().path(importPath.path()).param("exportAs", name.c_str()).param("importsOnly", "1").buildPath();
                auto compiledData = base::GetService<depot::DepotService>()->loadResource<CompiledProject>(path);
                if (!compiledData)
                {
                    TRACE_ERROR("Script module '{}' failed to load from '{}', maybe it itself can't be compiled", name, path);
                    return nullptr;
                }

                // get the data
                auto data = compiledData->data();
                if (!data || !data->exportedModule())
                {
                    TRACE_ERROR("Script module '{}' loaded from '{}' but it contains no data", name, path);
                    return nullptr;
                }*/

                return nullptr;// data;
            }

            const StubModule* importModule(StubLibrary& stubs, StringID name) const
            {
                // self import
                if (name == stubs.primaryModule()->name)
                    return stubs.primaryModule();

                // already imported ?
                for (auto module  : stubs.primaryModule()->imports)
                    if (module->name == name)
                        return module;

                // load the data
                auto data = loadImportModule(name);
                if (!data)
                    return nullptr;

                // insert the data into our library
                auto importedModule  = stubs.importModule(data->exportedModule(), name);
                if (!importedModule)
                    return nullptr;

                // load the module dependencies as well
                for (auto dependency  : data->exportedModule()->imports)
                    importModule(stubs, dependency->name);
                return importedModule;
            }
        };

        RTTI_BEGIN_TYPE_CLASS(ScriptProjectCooker);
            RTTI_METADATA(base::res::ResourceCookedClassMetadata).addClass<CompiledProject>();
            RTTI_METADATA(base::res::ResourceSourceFormatMetadata).addSourceExtension("scripts");
        RTTI_END_TYPE();

        //---

    } // script
} // base