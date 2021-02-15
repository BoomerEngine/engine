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
#include "base/io/include/ioFileHandle.h"
#include "base/parser/include/textToken.h"
#include "rendering/device/include/renderingShaderService.h"

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
			case MaterialPass::WireframeSolid: outDefines["MAT_PASS_WIREFRAME"_id] = "1"; break;
            case MaterialPass::WireframePassThrough: outDefines["MAT_PASS_WIREFRAME"_id] = "1"; break;
			case MaterialPass::ConstantColor: outDefines["MAT_PASS_CONST_COLOR"_id] = "1"; break;
			case MaterialPass::SelectionFragments: outDefines["MAT_PASS_SELECTION"_id] = "1"; break;
			case MaterialPass::Forward: outDefines["MAT_PASS_FORWARD"_id] = "1"; break;
			case MaterialPass::ShadowDepth: outDefines["MAT_PASS_SHADOW_DEPTH"_id] = "1"; break;
			case MaterialPass::MaterialDebug: outDefines["MAT_PASS_MATERIAL_DEBUG"_id] = "1"; break;

            // HACKy
            case MaterialPass::ForwardTransparent: 
                outDefines["MAT_PASS_FORWARD"_id] = "1"; 
                outDefines["MAT_PASS_FORWARD_TRANSPARENT"_id] = "1"; 
                outDefines["MAT_TRANSPARENT"_id] = "1";
                break;

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

    MaterialCompiledTechnique* CompileTechnique(const base::StringBuf& materialGraphPath, const MaterialGraphContainerPtr& graph, const MaterialCompilationSetup& setup)
    {
        base::ScopeTimer timer;

        // determine the unique shader cache key
        base::CRC64 key;
        key << "MATERIAL_";
        key << materialGraphPath;
        key << setup.key();

        // check if we can use cached technique and load it
        const auto shaderService = base::GetService<ShaderService>();
        if (const auto existingShader = shaderService->loadCustomShader(key))
        {
            auto compiledTechnique = new MaterialCompiledTechnique;
            compiledTechnique->shader = existingShader;
            compiledTechnique->shaderKey = key;
            compiledTechnique->setupKey = setup.key();
            return compiledTechnique;
        }

        // Determine the data layout of the material parameters - this is something that must match between template and and an instance
        // In here we need this layout to print the material descriptor and/or generate material attribute fetch code
        const auto* dataLayout = CompileDataLayout(*graph);

        // create the compiler regardless of the result - if anything we will generate an "error" material
        MaterialTechniqueRenderStates renderStates;
        MaterialMeshGeometryCompiler compiler(dataLayout, materialGraphPath, setup);

        // load the graph
        // find root output block
        if (const auto block = graph->findOutputBlock())
        {
            block->compilePixelFunction(compiler.m_ps, renderStates);
            block->compileVertexFunction(compiler.m_vs, renderStates);
        }
        else
        {
            TRACE_WARNING("Missing root block in material graph from '{}', no code can be generated", materialGraphPath);
            compiler.m_ps.appendf("gl_Target0 = vec4(1,0,1,1);\n");
        }

        // generate final code
        base::StringBuilder finalText;
        compiler.assembleFinalShaderCode(finalText, renderStates);
        const auto generationTime = timer.timeElapsed();

		// add global defines to material compilation context based on the context (so the hard-written includes can change based on compilation settings)
		base::HashMap<base::StringID, base::StringBuf> defines;
		GatherDefinesForCompilationSetup(setup, defines);

        // create the local cooker - so we can load includes
        const auto shader = shaderService->compileCustomShader(key, finalText.view(), &defines);
        if (!shader)
        {
            TRACE_ERROR("Failed to compile shader codef from material graph '{}'", materialGraphPath);
            return nullptr;
        }

        // create a final data in a compiled technique and push it to the target technique we were requested to compile
        auto compiledTechnique = new MaterialCompiledTechnique;
        compiledTechnique->shader = shader;
        compiledTechnique->shaderKey = key;
        compiledTechnique->setupKey = setup.key();

        TRACE_INFO("Compiled '{} for '{}' in {} ({} code generation)", setup, materialGraphPath, timer, TimeInterval(generationTime));
        return compiledTechnique;
    }

    //--

    MaterialTechniqueCompiler::MaterialTechniqueCompiler(const base::StringBuf& contextName, const MaterialGraphContainerPtr& graph, const MaterialCompilationSetup& setup, MaterialTechniquePtr& outputTechnique)
        : m_setup(setup)
        , m_graph(graph)
        , m_technique(outputTechnique)
        , m_contextName(contextName)
    {}

    bool MaterialTechniqueCompiler::compile()
    {
        // compile technique
        auto* compiledTechnique = CompileTechnique(m_contextName, m_graph, m_setup);
        DEBUG_CHECK_RETURN_EX_V(compiledTechnique && compiledTechnique->shader, "Failed to compile material technique", false);

        // push new state to target technique, from now one this will be used for rendering
        m_technique->pushData(compiledTechnique->shader);
        delete compiledTechnique;
        return true;
    }

    //--

} // rendering
