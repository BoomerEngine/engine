/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material #]
***/

#include "build.h"
#include "graph.h"
#include "techniqueCacheService.h"
#include "graphBlock_Output.h"
#include "graphBlock_Parameter.h"
#include "techniqueCompiler.h"
#include "engine/material/include/materialTemplate.h"
#include "engine/mesh/include/format.h"
#include "gpu/shader_compiler/include/shaderCompiler.h"
#include "core/resource/include/resourceTags.h"

BEGIN_BOOMER_NAMESPACE()

///---

/// graph based compiler 
class MaterialTemplateGraphTechniqueCompiler : public IMaterialTemplateDynamicCompiler
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialTemplateGraphTechniqueCompiler, IMaterialTemplateDynamicCompiler);

public:
    MaterialTemplateGraphTechniqueCompiler()
    {}

    MaterialTemplateGraphTechniqueCompiler(const MaterialGraphContainerPtr& graph, const Array<MaterialTemplateParamInfo>& params)
        : m_graph(graph)
        , m_params(params)
    {
        m_graph->parent(this);
        m_output = AddRef(m_graph->findOutputBlock());
    }

    virtual ~MaterialTemplateGraphTechniqueCompiler()
    {
        TRACE_INFO("Releasing template compiler");
    }

    virtual void evalRenderStates(const IMaterial& setup, MaterialRenderState& outRenderStates) const override final
    {
        if (m_output)
            m_output->evalRenderStates(setup, outRenderStates);
    }

    virtual void requestTechniqueComplation(StringView contextName, MaterialTechnique* technique) override final
    {
        GetService<MaterialTechniqueCacheService>()->requestTechniqueCompilation(contextName, m_params, m_graph, technique);
    }

private:
    MaterialGraphContainerPtr m_graph;
    Array<MaterialTemplateParamInfo> m_params;
    RefPtr<MaterialGraphBlockOutput> m_output;
};

RTTI_BEGIN_TYPE_CLASS(MaterialTemplateGraphTechniqueCompiler);
    RTTI_PROPERTY(m_graph);
RTTI_END_TYPE();

///---

const MaterialPass PERM_PASS_LIST[] =
{
    MaterialPass::DepthPrepass,
    MaterialPass::Forward,
    MaterialPass::ShadowDepth,
};

const MeshVertexFormat PERM_VERTEX_LIST[] =
{
    MeshVertexFormat::PositionOnly,
    MeshVertexFormat::Static,
    MeshVertexFormat::StaticEx,
};

const bool BOOL_STATE[] =
{
	false,
	true,
};

static void GatherMaterialPermutations(Array<MaterialCompilationSetup>& outSetupList)
{
    for (auto pass : PERM_PASS_LIST)
    {
        for (auto vertexFormat : PERM_VERTEX_LIST)
        {
			for (auto msaa : BOOL_STATE)
			{
				for (auto bindless : BOOL_STATE)
				{
					auto& setup = outSetupList.emplaceBack();
					setup.msaa = msaa;
					setup.vertexFormat = vertexFormat;
					setup.pass = pass;
					setup.bindlessTextures = bindless;

					if (vertexFormat == MeshVertexFormat::Static || vertexFormat == MeshVertexFormat::StaticEx)
					{
						auto& setup = outSetupList.emplaceBack();
						setup.msaa = msaa;
						setup.vertexFormat = vertexFormat;
						setup.pass = pass;
						setup.bindlessTextures = bindless;
						setup.meshletsVertices = true;
					}
				}
			}
        }
    }
}

///---

END_BOOMER_NAMESPACE()
