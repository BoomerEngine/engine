/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler #]
*/

#include "build.h"
#include "graph.h"
#include "graphBlock_Output.h"
#include "graphBlock_Parameter.h"
#include "techniqueCompiler.h"
#include "codeCompiler.h"

#include "engine/material/include/runtimeLayout.h"
#include "engine/material/include/runtimeService.h"
#include "engine/material/include/runtimeTechnique.h"

#include "core/io/include/ioFileHandle.h"
#include "core/parser/include/textToken.h"

#include "gpu/shader_compiler/include/shaderCompiler.h"
#include "gpu/device/include/shaderService.h"

BEGIN_BOOMER_NAMESPACE()

//--

ConfigProperty<bool> cvPrintShaderCompilationErrors("Rendering.Material", "PrintLocalCompilationErrors", true);
ConfigProperty<bool> cvPrintShaderCompilationWarnings("Rendering.Material", "PrintLocalCompilationWarnings", true);
ConfigProperty<bool> cvDumpCompiledMaterailTechniques("Rendering.Material", "DumpCompiledMaterialTechniques", true);
ConfigProperty<StringBuf> cvDumpCompiledMaterailTechniquesPath("Rendering.Material", "DumpCompiledMaterialTechniquesPath", "Z:\\.materialdump\\");

//--

static const MaterialDataLayout* CompileDataLayout(const Array<MaterialTemplateParamInfo>& params)
{
    Array<MaterialDataLayoutEntry> layoutEntries;

    for (const auto& param : params)
    {
        auto& entry = layoutEntries.emplaceBack();
        entry.name = param.name;
        entry.type = param.parameterType;
    }

    // TODO: optimize layout/order

    std::sort(layoutEntries.begin(), layoutEntries.end(), [](const MaterialDataLayoutEntry& a, MaterialDataLayoutEntry& b) { return a.name.view() < b.name.view(); });

    return GetService<MaterialService>()->registerDataLayout(std::move(layoutEntries));
}

//--

static void GatherDefinesForCompilationSetup(const MaterialCompilationSetup& setup, HashMap<StringID, StringBuf>& outDefines)
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

MaterialCompiledTechnique* CompileTechnique(const StringBuf& materialGraphPath, const Array<MaterialTemplateParamInfo>& params, const MaterialGraphContainerPtr& graph, const MaterialCompilationSetup& setup)
{
    ScopeTimer timer;

    // determine the unique shader cache key
    CRC64 key;
    key << "MATERIAL_";
    key << materialGraphPath;
    key << setup.key();

    // check if we can use cached technique and load it
    const auto shaderService = GetService<gpu::ShaderService>();
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
    const auto* dataLayout = CompileDataLayout(params);

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
    StringBuilder finalText;
    compiler.assembleFinalShaderCode(finalText, renderStates);
    const auto generationTime = timer.timeElapsed();

	// add global defines to material compilation context based on the context (so the hard-written includes can change based on compilation settings)
	HashMap<StringID, StringBuf> defines;
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

MaterialTechniqueCompiler::MaterialTechniqueCompiler(const StringBuf& contextName, const Array<MaterialTemplateParamInfo>& params, const MaterialGraphContainerPtr& graph, const MaterialCompilationSetup& setup, MaterialTechniquePtr& outputTechnique)
    : m_setup(setup)
    , m_graph(graph)
    , m_params(params)
    , m_technique(outputTechnique)
    , m_contextName(contextName)
{}

bool MaterialTechniqueCompiler::compile()
{
    // compile technique
    auto* compiledTechnique = CompileTechnique(m_contextName, m_params, m_graph, m_setup);
    DEBUG_CHECK_RETURN_EX_V(compiledTechnique && compiledTechnique->shader, "Failed to compile material technique", false);

    // push new state to target technique, from now one this will be used for rendering
    m_technique->pushData(compiledTechnique->shader);
    delete compiledTechnique;
    return true;
}

//--

END_BOOMER_NAMESPACE()
