/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material #]
***/

#include "build.h"
#include "renderingMaterialGraph.h"
#include "renderingMaterialGraphBlock.h"
#include "renderingMaterialGraphBlock_Parameter.h"
#include "renderingMaterialGraphBlock_Output.h"
#include "renderingMaterialGraphTechniqueCacheService.h"

#include "renderingMaterialGraphBlock_OutputUnlit.h"

#include "rendering/material/include/renderingMaterialTemplate.h"

#include "base/graph/include/graphSocket.h" 
#include "base/object/include/rttiDataView.h"
#include "base/resource/include/resourceTags.h"
#include "base/resource/include/resourceFactory.h" 

namespace rendering
{
    ///---

    RTTI_BEGIN_TYPE_CLASS(MaterialGraphContainer);
        RTTI_PROPERTY(m_parameters);
    RTTI_END_TYPE();

    MaterialGraphContainer::MaterialGraphContainer()
    {}

    MaterialGraphContainer::~MaterialGraphContainer()
    {}

    bool MaterialGraphContainer::canAddBlockOfClass(base::ClassType blockClass) const
    {
        return TBaseClass::canAddBlockOfClass(blockClass);
    }

    void MaterialGraphContainer::supportedBlockClasses(base::Array<base::SpecificClassType<base::graph::Block>>& outBlockClasses) const
    {
        RTTI::GetInstance().enumClasses(MaterialGraphBlock::GetStaticClass(), outBlockClasses);
    }

    void MaterialGraphContainer::notifyStructureChanged()
    {
        TBaseClass::notifyStructureChanged();
        cacheParameterBlocks();
    }

    void MaterialGraphContainer::refreshParameterList()
    {
        base::Array<MaterialGraphParameterBlockPtr> newParameters;
        buildParameterList(newParameters);

        if (m_parameters != newParameters)
        {
            m_parameters = newParameters;
            buildParameterMap();
            markModified();

            if (auto graph = findParent<MaterialGraph>())
                graph->postEvent(base::EVENT_OBJECT_STRUCTURE_CHANGED);
        }
    }

    const MaterialGraphBlockOutput* MaterialGraphContainer::findOutputBlock() const
    {
        for (const auto& block : blocks())
            if (block && block->is<MaterialGraphBlockOutput>())
                return static_cast<const MaterialGraphBlockOutput*>(block.get());

        return nullptr;
    }

    const MaterialGraphBlockParameter* MaterialGraphContainer::findParamBlock(base::StringID name) const
    {
        MaterialGraphBlockParameter* ret = nullptr;
        m_parameterMap.find(name, ret);
        return ret;
    }

    MaterialGraphBlockParameter* MaterialGraphContainer::findParamBlock(base::StringID name)
    {
        MaterialGraphBlockParameter* ret = nullptr;
        m_parameterMap.find(name, ret);
        return ret;
    }

    void MaterialGraphContainer::onPostLoad()
    {
        TBaseClass::onPostLoad();
        cacheParameterBlocks();
    }

    void MaterialGraphContainer::buildParameterMap()
    {
        m_parameterMap.reset();

        for (const auto& param : m_parameters)
        {
            DEBUG_CHECK_EX(param->name(), "Material parameter without name");
            DEBUG_CHECK_EX(!m_parameterMap.contains(param->name()), "Material parameter with duplicated name");
            m_parameterMap[param->name()] = param;
        }
    }

    void MaterialGraphContainer::buildParameterList(base::Array<MaterialGraphParameterBlockPtr>& outParamList) const
    {
        base::HashMap<base::StringID, MaterialGraphParameterBlockPtr> newParametersMap;

        for (const auto& block : blocks())
            if (auto paramBlock = base::rtti_cast<MaterialGraphBlockParameter>(block))
                if (!paramBlock->name().empty())
                    if (!newParametersMap.contains(paramBlock->name()))
                        newParametersMap[paramBlock->name()] = paramBlock;

        auto newParameters = newParametersMap.values();
        std::sort(newParameters.begin(), newParameters.end(), [](const MaterialGraphParameterBlockPtr& a, const MaterialGraphParameterBlockPtr& b)
            {
                return a->name().view() < b->name().view();
            });

        outParamList = std::move(newParameters);
    }

    void MaterialGraphContainer::cacheParameterBlocks()
    {
        m_parameters.clear();
        buildParameterList(m_parameters);
        buildParameterMap();
    }

    ///---

    RTTI_BEGIN_TYPE_CLASS(MaterialGraph);
        RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4mg");
        RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("Material Graph");
        RTTI_METADATA(base::res::ResourceTagColorMetadata).color(0xFF, 0xA6, 0x30);
        RTTI_PROPERTY(m_graph);
    RTTI_END_TYPE();

    MaterialGraph::MaterialGraph()
    {
        if (!base::IsDefaultObjectCreation())
        {
            m_graph = base::RefNew<MaterialGraphContainer>();
            m_graph->parent(this);
        }
    }

    MaterialGraph::~MaterialGraph()
    {
    }

    //--

    /// material compiler based on preview graph
    class PreviewGraphTechniqueCompiler : public rendering::IMaterialTemplateDynamicCompiler
    {
        RTTI_DECLARE_VIRTUAL_CLASS(PreviewGraphTechniqueCompiler, IMaterialTemplateDynamicCompiler);

    public:
        PreviewGraphTechniqueCompiler()
        {}

        PreviewGraphTechniqueCompiler(const MaterialGraphContainerPtr& graph)
            : m_graph(graph)
        {
        }

        virtual void requestTechniqueComplation(base::StringView contextName, MaterialTechnique* technique) override final
        {
            base::GetService<MaterialTechniqueCacheService>()->requestTechniqueCompilation(contextName, m_graph, technique);
        }

    private:
        MaterialGraphContainerPtr m_graph;
    };

    RTTI_BEGIN_TYPE_CLASS(PreviewGraphTechniqueCompiler);
        RTTI_PROPERTY(m_graph);
    RTTI_END_TYPE();


    //--

    MaterialTemplatePtr MaterialGraph::createPreviewTemplate(base::StringView label) const
    {
        // find output node and read settings
        MaterialTemplateMetadata metadata;
        if (const auto* outputBlock = m_graph->findOutputBlock())
            outputBlock->resolveMetadata(metadata);

        // enumerate parameters
        base::Array<MaterialTemplateParamInfo> parameters;
        parameters.reserve(m_graph->parameters().size());

        for (const auto& block : m_graph->parameters())
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

        // copy the graph
        auto graphCopy = base::rtti_cast<MaterialGraphContainer>(m_graph->clone());
        DEBUG_CHECK_EX(graphCopy, "Failed to make a copy of source graph");
        if (!graphCopy)
            return nullptr;

        // create a version of material template that supports runtime compilation from the source graph
        auto compiler = base::RefNew<PreviewGraphTechniqueCompiler>(graphCopy);
        return base::RefNew<MaterialTemplate>(std::move(parameters), metadata, compiler, base::StringBuf(label));
    }

    //---

    /*bool MaterialGraph::readDataView(const base::IDataView* rootView, base::StringView rootViewPath, base::StringView viewPath, void* targetData, base::Type targetType) const
    {
        if (TBaseClass::readDataView(rootView, rootViewPath, viewPath, targetData, targetType))
            return true;

        if (!viewPath.empty())
        {
            base::StringView propertyName;
            if (base::rtti::ParsePropertyName(viewPath, propertyName) && !propertyName.empty())
            {
                if (const auto* paramBlock = m_graph->findParamBlock(propertyName))
                {
                    return paramBlock->dataType()->readDataView(nullptr, nullptr, "", viewPath, paramBlock->dataValue(), targetData, targetType);
                }
            }
        }

        return false;
    }

    bool MaterialGraph::writeDataView(const base::IDataView* rootView, base::StringView rootViewPath, base::StringView viewPath, const void* sourceData, base::Type sourceType)
    {
        if (TBaseClass::writeDataView(rootView, rootViewPath, viewPath, sourceData, sourceType))
            return true;

        if (!viewPath.empty())
        {
            base::StringView propertyName;
            if (base::rtti::ParsePropertyName(viewPath, propertyName) && !propertyName.empty())
            {
                if (auto* paramBlock = m_graph->findParamBlock(propertyName))
                {
                    return paramBlock->dataType()->writeDataView(nullptr, nullptr, "", viewPath, (void*)paramBlock->dataValue(), sourceData, sourceType);
                }
            }
        }

        return false;
    }

    bool MaterialGraph::describeDataView(base::StringView viewPath, base::rtti::DataViewInfo& outInfo) const
    {
        base::StringView propertyName;
        if (viewPath.empty())
        {
            if (!TBaseClass::describeDataView(viewPath, outInfo))
                return false;

            if (outInfo.requestFlags.test(base::rtti::DataViewRequestFlagBit::MemberList))
            {
                for (const auto& parameterBlock : m_graph->parameters())
                {
                    auto& memberInfo = outInfo.members.emplaceBack();
                    memberInfo.name = parameterBlock->name();
                    memberInfo.category = parameterBlock->category();
                }
            }

            return true;
        }
        else
        {
            if (TBaseClass::describeDataView(viewPath, outInfo))
                return true;

            if (base::rtti::ParsePropertyName(viewPath, propertyName))
            {
                if (auto* paramBlock = m_graph->findParamBlock(propertyName))
                    return paramBlock->dataType()->describeDataView(viewPath, paramBlock->dataValue(), outInfo);
            }
        }

        return false;
    }*/

    //---

    // factory class for the material graph
    class MaterialGraphFactory : public base::res::IFactory
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphFactory, base::res::IFactory);

    public:
        virtual base::res::ResourceHandle createResource() const override final
        {
            auto ret = base::RefNew<MaterialGraph>();

            auto outputBlock = base::RefNew<MaterialGraphBlockOutput_Unlit>();
            ret->graph()->addBlock(outputBlock);

            return ret;
        }
    };

    RTTI_BEGIN_TYPE_CLASS(MaterialGraphFactory);
        RTTI_METADATA(base::res::FactoryClassMetadata).bindResourceClass<MaterialGraph>();
    RTTI_END_TYPE();

    //--


} // rendering
