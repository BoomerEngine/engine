/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material #]
***/

#include "build.h"
#include "graph.h"
#include "graphBlock.h"
#include "graphBlock_Parameter.h"
#include "graphBlock_Output.h"
#include "techniqueCacheService.h"

#include "graphBlock_OutputUnlit.h"

#include "engine/material/include/materialTemplate.h"

#include "core/graph/include/graphSocket.h"
#include "core/object/include/rttiDataView.h"
#include "core/resource/include/resourceTags.h"
#include "core/resource/include/resourceFactory.h"

BEGIN_BOOMER_NAMESPACE()

///---

RTTI_BEGIN_TYPE_CLASS(MaterialGraphContainer);
RTTI_END_TYPE();

MaterialGraphContainer::MaterialGraphContainer()
{}

MaterialGraphContainer::~MaterialGraphContainer()
{}

bool MaterialGraphContainer::canAddBlockOfClass(ClassType blockClass) const
{
    return TBaseClass::canAddBlockOfClass(blockClass);
}

void MaterialGraphContainer::supportedBlockClasses(Array<SpecificClassType<graph::Block>>& outBlockClasses) const
{
    RTTI::GetInstance().enumClasses(MaterialGraphBlock::GetStaticClass(), outBlockClasses);
}

void MaterialGraphContainer::notifyStructureChanged()
{
    TBaseClass::notifyStructureChanged();
}

bool MaterialGraphContainer::checkParameterUsed(IMaterialTemplateParam* param) const
{
    bool hasParam = false;

    for (const auto& block : blocks())
    {

    }

    return hasParam;
}

const MaterialGraphBlockOutput* MaterialGraphContainer::findOutputBlock() const
{
    for (const auto& block : blocks())
        if (block && block->is<MaterialGraphBlockOutput>())
            return static_cast<const MaterialGraphBlockOutput*>(block.get());

    return nullptr;
}

void MaterialGraphContainer::onPostLoad()
{
    TBaseClass::onPostLoad();
}

///---

RTTI_BEGIN_TYPE_CLASS(MaterialGraph);
    RTTI_METADATA(res::ResourceExtensionMetadata).extension("v4mg");
    RTTI_METADATA(res::ResourceDescriptionMetadata).description("Material Graph");
    RTTI_METADATA(res::ResourceTagColorMetadata).color(0xFF, 0xA6, 0x30);
    RTTI_PROPERTY(m_graph);
RTTI_END_TYPE();

MaterialGraph::MaterialGraph()
{
    if (!IsDefaultObjectCreation())
    {
        m_graph = RefNew<MaterialGraphContainer>();
        m_graph->parent(this);
    }
}

MaterialGraph::~MaterialGraph()
{
}

void MaterialGraph::attachParameter(IMaterialTemplateParam* param)
{
    DEBUG_CHECK_RETURN_EX(param, "Invalid paramter");
    DEBUG_CHECK_RETURN_EX(param->parent() == nullptr, "Paramter should not be attached");
    DEBUG_CHECK_RETURN_EX(!m_parameters.contains(param), "Parameter should not be on the list");
    m_parameters.pushBack(AddRef(param));
    param->parent(this);

    onPropertyChanged("parameters");
    notifyBaseMaterialChanged();
}

bool MaterialGraph::detachParameter(IMaterialTemplateParam* param)
{
    DEBUG_CHECK_RETURN_EX_V(param, "Invalid paramter", false);
    DEBUG_CHECK_RETURN_EX_V(param->parent() == this, "Paramter should be attached", false);
    DEBUG_CHECK_RETURN_EX_V(m_parameters.contains(param), "Parameter should be on the list", false);
    DEBUG_CHECK_RETURN_EX_V(!checkParameterUsed(param), "Cannot detach used parameters", false);

    m_parameters.pushBack(AddRef(param));
    param->parent(nullptr);

    onPropertyChanged("parameters");
    notifyBaseMaterialChanged();

    return true;
}

bool MaterialGraph::renameParameter(IMaterialTemplateParam* param, StringID name)
{
    DEBUG_CHECK_RETURN_EX_V(param, "Invalid paramter", false);
    DEBUG_CHECK_RETURN_EX_V(param->parent() == this, "Paramter should be attached", false);
    DEBUG_CHECK_RETURN_EX_V(m_parameters.contains(param), "Parameter should be on the list", false);

    for (const auto& otherParam : m_parameters)
        if (otherParam != param && param->name() == name)
            return false;

    param->rename(name);

    onPropertyChanged("parameters");
    notifyBaseMaterialChanged();

    return true;
}

bool MaterialGraph::checkParameterUsed(IMaterialTemplateParam* param) const
{
    return m_graph && m_graph->checkParameterUsed(param);
}

//--

/// material compiler based on preview graph
class PreviewGraphTechniqueCompiler : public IMaterialTemplateDynamicCompiler
{
    RTTI_DECLARE_VIRTUAL_CLASS(PreviewGraphTechniqueCompiler, IMaterialTemplateDynamicCompiler);

public:
    PreviewGraphTechniqueCompiler()
    {}

    PreviewGraphTechniqueCompiler(const MaterialGraphContainerPtr& graph, const Array<MaterialTemplateParamInfo>& params)
        : m_graph(graph)
        , m_params(params)
    {
        m_output = AddRef(m_graph->findOutputBlock());
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

RTTI_BEGIN_TYPE_CLASS(PreviewGraphTechniqueCompiler);
    RTTI_PROPERTY(m_graph);
RTTI_END_TYPE();


//--

MaterialTemplatePtr MaterialGraph::createPreviewTemplate(StringView label) const
{
    auto clone = CloneObject<MaterialGraph>(this);
    if (clone)
    {
        clone->m_contextPath = StringBuf(label);
        return clone;
    }

    return nullptr;
}

//--

RefPtr<IMaterialTemplateDynamicCompiler> MaterialGraph::queryDynamicCompiler() const
{
    Array<MaterialTemplateParamInfo> paramInfos;
    paramInfos.reserve(m_parameters.size());

    for (const auto& param : m_parameters)
    {
        if (param->name() && param->queryDataType())
        {
            auto& info = paramInfos.emplaceBack();
            info.name = param->name();
            info.parameterType = param->queryType();
        }
    }

    // create a version of material template that supports runtime compilation from the source graph
    return RefNew<PreviewGraphTechniqueCompiler>(m_graph, paramInfos);
}

//---

// factory class for the material graph
class MaterialGraphFactory : public res::IFactory
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphFactory, res::IFactory);

public:
    virtual res::ResourceHandle createResource() const override final
    {
        auto ret = RefNew<MaterialGraph>();

        auto outputBlock = RefNew<MaterialGraphBlockOutput_Unlit>();
        ret->graph()->addBlock(outputBlock);

        return ret;
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphFactory);
    RTTI_METADATA(res::FactoryClassMetadata).bindResourceClass<MaterialGraph>();
RTTI_END_TYPE();

//--

END_BOOMER_NAMESPACE()
