/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material #]
***/

#include "build.h"
#include "renderingMaterialGraph.h"
#include "renderingMaterialGraphTechniqueCacheService.h"
#include "renderingMaterialGraphBlock_Output.h"
#include "renderingMaterialGraphBlock_Parameter.h"
#include "renderingMaterialGraphTechniqueCompiler.h"
#include "rendering/material/include/renderingMaterialTemplate.h"
#include "rendering/mesh/include/renderingMeshFormat.h"
#include "rendering/compiler/include/renderingShaderLibraryCompiler.h"
#include "base/resources/include/resourceCookingInterface.h"

namespace rendering
{

    ///---

    /// graph based compiler 
    class MaterialTemplateGraphTechniqueCompiler : public IMaterialTemplateDynamicCompiler
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialTemplateGraphTechniqueCompiler, IMaterialTemplateDynamicCompiler);

    public:
        MaterialTemplateGraphTechniqueCompiler()
        {}

        MaterialTemplateGraphTechniqueCompiler(const MaterialGraphContainerPtr& graph)
            : m_graph(graph)
        {
            m_graph->parent(this);
        }

        virtual ~MaterialTemplateGraphTechniqueCompiler()
        {
            TRACE_INFO("Releasing template compiler");
        }

        virtual void requestTechniqueComplation(base::StringView<char> contextName, MaterialTechnique* technique) override final
        {
            base::GetService<MaterialTechniqueCacheService>()->requestTechniqueCompilation(contextName, m_graph, technique);
        }

    private:
        MaterialGraphContainerPtr m_graph;
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

    static void GatherMaterialPermutations(base::Array<MaterialCompilationSetup>& outSetupList)
    {
        for (auto pass : PERM_PASS_LIST)
        {
            for (auto vertexFormat : PERM_VERTEX_LIST)
            {
                auto& setup = outSetupList.emplaceBack();
                setup.vertexFormat = (int)vertexFormat;
                setup.pass = pass;
                setup.vertexFetchMode = 0;
                setup.msaa = false;
            }
        }
    }

    ///---

    /// cook material graph into a material template
    class MaterialTemplateGraphCooker : public base::res::IResourceCooker
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialTemplateGraphCooker, base::res::IResourceCooker);

    public:
        virtual base::res::ResourceHandle cook(base::res::IResourceCookerInterface& cooker) const override final
        {
            auto importPath = cooker.queryResourcePath();

            // load the source graph
            auto sourceGraph = cooker.loadDependencyResource<MaterialGraph>(importPath);
            if (!sourceGraph || !sourceGraph->graph())
            {
                TRACE_ERROR("Unable to load source material graph from '{}'", importPath);
                return nullptr;
            }

            // find output node and read settings
            auto sortGroup = MaterialSortGroup::Opaque;
            const auto* outputBlock = sourceGraph->graph()->findOutputBlock();
            if (!outputBlock)
            {
                TRACE_WARNING("Material graph '{}' contains no output block", importPath);
                sortGroup = outputBlock->resolveSortGroup();
            }

            // enumerate parameters
            base::Array<MaterialTemplateParamInfo> parameters;
            parameters.reserve(sourceGraph->graph()->parameters().size());

            for (const auto& block : sourceGraph->graph()->parameters())
            {
                if (block->name() && block->dataType())
                {
                    auto& outInfo = parameters.emplaceBack();
                    outInfo.name = block->name();
                    outInfo.category = block->category() ? block->category() : "Material parameters"_id;
                    outInfo.type = block->dataType();
                    outInfo.parameterType = block->parameterType();
                    outInfo.defaultValue = base::Variant(block->dataType(), block->dataValue());
                }
            }

            // in final cook compile all shader permutations
            if (cooker.finalCooker())
            {
                base::InplaceArray<MaterialCompilationSetup, 20> setupCollection;
                GatherMaterialPermutations(setupCollection);
                TRACE_INFO("Gathered {} permutations for material {}", setupCollection.size(), importPath);

                // forward include requests and errors to cooker
                compiler::ShaderLibraryCookerIncludeHandler includeHandler(cooker);
                compiler::ShaderLibraryCookerErrorReporter errorHandler(cooker);

                bool validTechniques = true;

                base::Array< MaterialPrecompiledStaticTechnique> precompiledTechniques;
                precompiledTechniques.reserve(setupCollection.size());

                const auto contextPath = base::StringBuf(importPath);
                for (const auto& setup : setupCollection)
                {
                    auto* compiledTechnique = CompileTechnique(contextPath, sourceGraph->graph(), setup, errorHandler, includeHandler);
                    if (compiledTechnique)
                    {
                        auto& entry = precompiledTechniques.emplaceBack();
                        entry.setup = setup;
                        entry.shader = compiledTechnique->shader;
                        entry.renderStates = compiledTechnique->renderStates;

                        MemDelete(compiledTechnique);
                    }
                    else
                    {
                        validTechniques = false;
                    }
                }

                if (!validTechniques)
                    return nullptr;

                return base::CreateSharedPtr<MaterialTemplate>(std::move(parameters), sortGroup, std::move(precompiledTechniques));
            }
            else
            {
                // copy the graph
                auto graphCopy = base::rtti_cast<MaterialGraphContainer>(sourceGraph->graph()->clone());
                DEBUG_CHECK_EX(graphCopy, "Failed to make a copy of source graph");
                if (!graphCopy)
                {
                    TRACE_ERROR("Unable to copy source material graph from '{}'", importPath);
                    return nullptr;
                }

                // create a version of material template that supports runtime compilation from the source graph
                auto compiler = base::CreateSharedPtr<MaterialTemplateGraphTechniqueCompiler>(graphCopy);
                return base::CreateSharedPtr<MaterialTemplate>(std::move(parameters), sortGroup, compiler);
            }
        }
    };

    RTTI_BEGIN_TYPE_CLASS(MaterialTemplateGraphCooker);
        RTTI_METADATA(base::res::ResourceCookedClassMetadata).addClass<MaterialTemplate>();
        RTTI_METADATA(base::res::ResourceSourceFormatMetadata).addSourceClass<MaterialGraph>();
    RTTI_END_TYPE();

    //--

} // rendering
