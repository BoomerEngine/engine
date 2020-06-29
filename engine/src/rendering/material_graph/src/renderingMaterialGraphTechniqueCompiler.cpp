/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler #]
*/

#include "build.h"
#include "renderingMaterialGraph.h"
#include "renderingMaterialGraphBlock_Output.h"
#include "renderingMaterialGraphBlock_Parameter.h"
#include "renderingMaterialGraphTechniqueCompiler.h"
#include "renderingMaterialCodeCompiler.h"

#include "rendering/material/include/renderingMaterialRuntimeLayout.h"
#include "rendering/material/include/renderingMaterialRuntimeService.h"
#include "rendering/material/include/renderingMaterialRuntimeTechnique.h"
#include "rendering/compiler/include/renderingShaderLibraryCompiler.h"
#include "base/depot/include/depotStructure.h"
#include "base/io/include/ioFileHandle.h"
#include "base/parser/include/textToken.h"

namespace rendering
{

    //--

    base::ConfigProperty<bool> cvPrintShaderCompilationErrors("Rendering.Material", "PrintLocalCompilationErrors", true);
    base::ConfigProperty<bool> cvPrintShaderCompilationWarnings("Rendering.Material", "PrintLocalCompilationWarnings", true);
    base::ConfigProperty<bool> cvDumpCompiledMaterailTechniques("Rendering.Material", "DumpCompiledMaterialTechniques", true);
    base::ConfigProperty<base::StringBuf> cvDumpCompiledMaterailTechniquesPath("Rendering.Material", "DumpCompiledMaterialTechniquesPath", "Z:\\.materialdump\\");

    //--

    static const MaterialDataLayout* CompileDataLayout(const MaterialGraphContainer& graph)
    {
        base::Array<MaterialDataLayoutEntry> layoutEntries;

        for (const auto& param : graph.parameters())
        {
            if (param->name() && param->dataType())
            {
                auto& entry = layoutEntries.emplaceBack();
                entry.name = param->name();
                entry.type = param->parameterType();
            }
        }

        // TODO: optimize layout/order

        std::sort(layoutEntries.begin(), layoutEntries.end(), [](const MaterialDataLayoutEntry& a, MaterialDataLayoutEntry& b) { return a.name.view() < b.name.view(); });

        return base::GetService<MaterialService>()->registerDataLayout(std::move(layoutEntries));
    }

    //--

    MaterialCompiledTechnique* CompileTechnique(const base::StringBuf& contextName, const MaterialGraphContainerPtr& graph, const MaterialCompilationSetup& setup, base::parser::IErrorReporter& err, base::parser::IIncludeHandler& includes)
    {
        base::ScopeTimer timer;

        // Determine the data layout of the material parameters - this is something that must match between template and and an instance
        // In here we need this layout to print the material descriptor and/or generate material attribute fetch code
        const auto* dataLayout = CompileDataLayout(*graph);

        // create the compiler regardless of the result - if anything we will generate an "error" material
        MaterialTechniqueRenderStates renderStates;
        MaterialMeshGeometryCompiler compiler(dataLayout, contextName, setup);

        // load the graph
        // find root output block
        if (const auto block = graph->findOutputBlock())
        {
            block->compilePixelFunction(compiler.m_ps, renderStates);
            block->compileVertexFunction(compiler.m_vs, renderStates);
        }
        else
        {
            TRACE_ERROR("Missing root block in material graph from '{}', no code can be generated", contextName);
            return nullptr;
        }

        // generate final code
        base::StringBuilder finalText;
        compiler.assembleFinalShaderCode(finalText, renderStates);
        const auto generationTime = timer.timeElapsed();

        // create the local cooker - so we can load includes
        const auto shader = compiler::CompileShaderLibrary(finalText.view(), contextName, &includes, err, "GLSL"_id);
        if (!shader)
        {
            TRACE_ERROR("Failed to compile shader codef from material graph '{}'", contextName);
            return nullptr;
        }

        // create a final data in a compiled technique and push it to the target technique we were requested to compile
        auto compiledTechnique = MemNew(MaterialCompiledTechnique).ptr;
        compiledTechnique->shader = shader;
        compiledTechnique->dataLayout = dataLayout;
        compiledTechnique->renderStates = renderStates;

        TRACE_INFO("Compiled '{} for '{}' in {} ({} code generation)", setup, contextName, timer, TimeInterval(generationTime));
        return compiledTechnique;
    }

    //--

    MaterialTechniqueCompiler::MaterialTechniqueCompiler(base::depot::DepotStructure& depot, const base::StringBuf& contextName, const MaterialGraphContainerPtr& graph, const MaterialCompilationSetup& setup, MaterialTechniquePtr& outputTechnique)
        : m_setup(setup)
        , m_graph(graph)
        , m_technique(outputTechnique)
        , m_contextName(contextName)
        , m_depot(depot)
    {} 

    MaterialTechniqueCompiler::~MaterialTechniqueCompiler()
    {}

    bool MaterialTechniqueCompiler::compile()
    {
        // compile technique
        auto* compiledTechnique = CompileTechnique(m_contextName, m_graph, m_setup, *this, *this);
        if (!compiledTechnique)
            return false;

        // push new state to target technique, from now one this will be used for rendering
        m_technique->pushData(compiledTechnique);
        return true;
    }

    //--

    base::Buffer MaterialTechniqueCompiler::loadFileContent(base::StringView<char> depotPath)
    {
        DEBUG_CHECK_EX(!depotPath.empty(), "No shader file to load");

        // check maybe we already loaded that file
        for (const auto& data : m_usedFiles)
            if (data.depotPath == depotPath)
                return data.content;

        // load file data
        auto reader = m_depot.createFileReader(depotPath);
        if (!reader)
        {
            TRACE_ERROR("Unable to open file '{}'", depotPath);
            return nullptr;
        }

        // get size of the file
        const auto size = reader->size();

        // allocate memory
        base::Buffer ret;
        ret.init(POOL_TEMP, size);
        if (size != reader->readSync(ret.data(), size))
        {
            TRACE_ERROR("Unable to load content of '{}'", depotPath);
            return nullptr;
        }

        // get some basic file info
        base::io::TimeStamp fileTimeStamp;
        uint64_t fileCRC = 0;
        if (!m_depot.queryFileInfo(depotPath, &fileCRC, nullptr, &fileTimeStamp))
        {
            TRACE_WARNING("Unable to determine information about file '{}'", depotPath);
        }

        TRACE_SPAM("Source shader file '{}': {} {}", depotPath, fileCRC, fileTimeStamp.value());

        // store
        auto& entry = m_usedFiles.emplaceBack();
        entry.content = ret;
        entry.crc = fileCRC;
        entry.timestamp = fileTimeStamp.value();
        entry.depotPath = base::StringBuf(depotPath);

        // return loaded content
        return entry.content;
    }

    void MaterialTechniqueCompiler::reportError(const base::parser::Location& loc, base::StringView<char> message)
    {
        auto& errorMessage = m_errors.emplaceBack();
        errorMessage.filePath = loc.contextName();
        errorMessage.linePos = loc.line();
        errorMessage.charPos = loc.charPos();
        errorMessage.text = base::StringBuf(message);

        if (cvPrintShaderCompilationErrors.get())
        {
            base::io::AbsolutePath absolutePath;
            if (m_depot.queryFileAbsolutePath(loc.contextName(), absolutePath))
            {
                TRACE_ERROR("{}({}): error: {}", absolutePath, loc.line(), message);

                if (!m_firstErrorPrinted)
                    TRACE_ERROR("{}({}): error: when compiling '{}' with '{}'", absolutePath, loc.line(), m_contextName, m_setup);
            }
            else
            {
                TRACE_ERROR("{}({}): error: {}", loc.contextName(), loc.line(), message);

                if (!m_firstErrorPrinted)
                    TRACE_ERROR("{}({}): error: when compiling '{}' with '{}'", loc.contextName(), loc.line(), m_contextName, m_setup);
            }

            m_firstErrorPrinted = true;
        }
    }

    void MaterialTechniqueCompiler::reportWarning(const base::parser::Location& loc, base::StringView<char> message)
    {
        if (cvPrintShaderCompilationWarnings.get())
        {
            base::io::AbsolutePath absolutePath;
            if (m_depot.queryFileAbsolutePath(loc.contextName(), absolutePath))
            {
                TRACE_WARNING("{}({}): warning: {}", absolutePath, loc.line(), message);
            }
            else
            {
                TRACE_WARNING("{}({}): warning: {}", loc.contextName(), loc.line(), message);
            }
        }
    }

    bool MaterialTechniqueCompiler::checkFileExists(base::StringView<char> path) const
    {
        uint64_t fileSize = 0;
        if (!m_depot.queryFileInfo(path, nullptr, &fileSize, nullptr))
            return false;
        return 0 != fileSize;
    }

    bool MaterialTechniqueCompiler::queryResolvedPath(base::StringView<char> relativePath, base::StringView<char> contextFileSystemPath, bool global, base::StringBuf& outResourcePath) const
    {
        if (global)
        {
            base::StringBuf ret;
            if (base::res::ApplyRelativePath("engine/shaders/", relativePath, ret))
            {
                outResourcePath = ret;
                return true;
            }
        }
        else
        {
            base::StringBuf ret;
            auto contextPathToUse = contextFileSystemPath.empty() ? m_contextName.view() : contextFileSystemPath;
            if (base::res::ApplyRelativePath(contextPathToUse, relativePath, ret))
            {
                outResourcePath = ret;
                return true;
            }
        }

        return false;
    }

    bool MaterialTechniqueCompiler::loadInclude(bool global, base::StringView<char> path, base::StringView<char> referencePath, base::Buffer& outContent, base::StringBuf& outPath)
    {
        // resolve the full depot path to the include
        base::StringBuf fullFilePath;
        if (!queryResolvedPath(path, referencePath, global, fullFilePath) || !checkFileExists(fullFilePath))
            return false;

        auto content = loadFileContent(fullFilePath);
        if (!content)
            return false;

        outContent = content;
        outPath = fullFilePath;
        return true;
    }

    //--

} // rendering
