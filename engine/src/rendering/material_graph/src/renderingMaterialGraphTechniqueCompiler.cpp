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
#include "rendering/compiler/include/renderingShaderCompiler.h"
#include "base/resource_compiler/include/depotStructure.h"
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

	static void GatherDefinesForCompilationSetup(const MaterialCompilationSetup& setup, base::HashMap<base::StringID, base::StringBuf>& outDefines)
	{
		switch (setup.pass)
		{
			case MaterialPass::DepthPrepass: outDefines["MAT_PASS_DEPTH_PREPASS"_id] = "1"; break;
			case MaterialPass::Wireframe: outDefines["MAT_PASS_WIREFRAME"_id] = "1"; break;
			case MaterialPass::ConstantColor: outDefines["MAT_PASS_CONST_COLOR"_id] = "1"; break;
			case MaterialPass::SelectionFragments: outDefines["MAT_PASS_SELECTION"_id] = "1"; break;
			case MaterialPass::Forward: outDefines["MAT_PASS_FORWARD"_id] = "1"; break;
			case MaterialPass::ShadowDepth: outDefines["MAT_PASS_SHADOW_DEPTH"_id] = "1"; break;
			case MaterialPass::MaterialDebug: outDefines["MAT_PASS_MATERIAL_DEBUG"_id] = "1"; break;
			default: ASSERT(!"Add define");
		}

		switch (setup.vertexFormat)
		{
			case MeshVertexFormat::PositionOnly: 
				outDefines["MAT_VERTEX_POSITION_ONLY"_id] = "1";
				break;

			case MeshVertexFormat::Static: 
				outDefines["MAT_VERTEX_STATIC"_id] = "1";
				break;

			case MeshVertexFormat::StaticEx: 
				outDefines["MAT_VERTEX_STATIC"_id] = "1";
				outDefines["MAT_VERTEX_SECOND_UV"_id] = "1";
				break;

			case MeshVertexFormat::Skinned4: 
				outDefines["MAT_VERTEX_SKINNED"_id] = "1";
				outDefines["MAT_VERTEX_NUM_BONES"_id] = "4";
				break;

			case MeshVertexFormat::Skinned4Ex: 
				outDefines["MAT_VERTEX_SKINNED"_id] = "1";
				outDefines["MAT_VERTEX_SECOND_UV"_id] = "1";
				outDefines["MAT_VERTEX_NUM_BONES"_id] = "4";
				break;
		}

		if (setup.bindlessTextures)
			outDefines["MAT_BINDLESS"_id] = "1";

		if (setup.bindlessTextures)
			outDefines["MAT_MESHLET"_id] = "1";

		if (setup.bindlessTextures)
			outDefines["MSAA"_id] = "1";
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

		// add global defines to material compilation context based on the context (so the hard-written includes can change based on compilation settings)
		base::HashMap<base::StringID, base::StringBuf> defines;
		GatherDefinesForCompilationSetup(setup, defines);

        // create the local cooker - so we can load includes
        const auto shader = compiler::CompileShader(finalText.view(), contextName, base::TempString("{}", setup), &includes, err, &defines);
        if (!shader)
        {
            TRACE_ERROR("Failed to compile shader codef from material graph '{}'", contextName);
            return nullptr;
        }

        // create a final data in a compiled technique and push it to the target technique we were requested to compile
        auto compiledTechnique = new MaterialCompiledTechnique;
        compiledTechnique->shader = shader;
        //compiledTechnique->dataLayout = dataLayout;

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
        DEBUG_CHECK_RETURN_EX_V(compiledTechnique && compiledTechnique->shader, "Failed to compile material technique", false);

        // push new state to target technique, from now one this will be used for rendering
        m_technique->pushData(compiledTechnique->shader);
        delete compiledTechnique;
        return true;
    }

    //--

    base::Buffer MaterialTechniqueCompiler::loadFileContent(base::StringView depotPath)
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
        if (!m_depot.queryFileTimestamp(depotPath, fileTimeStamp))
        {
            TRACE_WARNING("Unable to determine information about file '{}'", depotPath);
        }

        // store
        auto& entry = m_usedFiles.emplaceBack();
        entry.content = ret;
        entry.timestamp = fileTimeStamp.value();
        entry.depotPath = base::StringBuf(depotPath);

        // return loaded content
        return entry.content;
    }

    void MaterialTechniqueCompiler::reportError(const base::parser::Location& loc, base::StringView message)
    {
        auto& errorMessage = m_errors.emplaceBack();
        errorMessage.filePath = loc.contextName();
        errorMessage.linePos = loc.line();
        errorMessage.charPos = loc.charPos();
        errorMessage.text = base::StringBuf(message);

        if (cvPrintShaderCompilationErrors.get())
        {
            base::StringBuf absolutePath;
            if (m_depot.queryFileAbsolutePath(loc.contextName(), absolutePath))
            {
                base::logging::Log::Print(base::logging::OutputLevel::Error, absolutePath.c_str(), loc.line(), "", "", message.data());

                if (!m_firstErrorPrinted)
                    TRACE_ERROR("When compiling '{}' with '{}'", m_contextName, m_setup);
            }
            else
            {
                base::logging::Log::Print(base::logging::OutputLevel::Error, loc.contextName().c_str(), loc.line(), "", "", message.data());

                if (!m_firstErrorPrinted)
                    TRACE_ERROR("When compiling '{}' with '{}'", m_contextName, m_setup);
            }

            m_firstErrorPrinted = true;
        }
    }

    void MaterialTechniqueCompiler::reportWarning(const base::parser::Location& loc, base::StringView message)
    {
        if (cvPrintShaderCompilationWarnings.get())
        {
            if (!loc.contextName().empty())
            {
                base::StringBuf absolutePath;
                if (m_depot.queryFileAbsolutePath(loc.contextName(), absolutePath))
                    base::logging::Log::Print(base::logging::OutputLevel::Warning, absolutePath.c_str(), loc.line(), "", "", message.data());
                else
                    base::logging::Log::Print(base::logging::OutputLevel::Warning, loc.contextName().c_str(), loc.line(), "", "", message.data());
            }
            else
            {
                TRACE_WARNING("{}", message);
            }
        }
    }

    bool MaterialTechniqueCompiler::checkFileExists(base::StringView path) const
    {
        base::io::TimeStamp unused;
        return m_depot.queryFileTimestamp(path, unused);
    }

    bool MaterialTechniqueCompiler::queryResolvedPath(base::StringView relativePath, base::StringView contextFileSystemPath, bool global, base::StringBuf& outResourcePath) const
    {
        if (global)
        {
            base::StringBuf ret;
            if (base::ApplyRelativePath("/engine/shaders/", relativePath, ret))
            {
                outResourcePath = ret;
                return true;
            }
        }
        else
        {
            base::StringBuf ret;
            auto contextPathToUse = contextFileSystemPath.empty() ? m_contextName.view() : contextFileSystemPath;
            if (base::ApplyRelativePath(contextPathToUse, relativePath, ret))
            {
                outResourcePath = ret;
                return true;
            }
        }

        return false;
    }

    bool MaterialTechniqueCompiler::loadInclude(bool global, base::StringView path, base::StringView referencePath, base::Buffer& outContent, base::StringBuf& outPath)
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
