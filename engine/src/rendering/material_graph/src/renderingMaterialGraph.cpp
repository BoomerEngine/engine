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

#include "base/resources/src/resourceGeneralTextLoader.h"
#include "base/resources/src/resourceGeneralTextSaver.h"
#include "base/resources/include/resourceSerializationMetadata.h"
#include "base/resources/include/resourceFactory.h" 
#include "base/graph/include/graphSocket.h" 
#include "renderingMaterialGraphBlock_OutputUnlit.h"
#include "base/object/include/rttiDataView.h"

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
                graph->postEvent("OnFullStructureChange"_id);
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
        //RTTI_METADATA(base::SerializationLoaderMetadata).bind<base::res::text::TextLoader>();
        //RTTI_METADATA(base::SerializationSaverMetadata).bind<base::res::text::TextSaver>();
        RTTI_PROPERTY(m_graph);
    RTTI_END_TYPE();

    MaterialGraph::MaterialGraph()
    {
        if (!base::IsDefaultObjectCreation())
        {
            m_graph = base::CreateSharedPtr<MaterialGraphContainer>();
            m_graph->parent(this);
        }
    }

    MaterialGraph::~MaterialGraph()
    {
    }

    //---

    bool MaterialGraph::readDataView(const base::IDataView* rootView, base::StringView<char> rootViewPath, base::StringView<char> viewPath, void* targetData, base::Type targetType) const
    {
        if (TBaseClass::readDataView(rootView, rootViewPath, viewPath, targetData, targetType))
            return true;

        if (!viewPath.empty())
        {
            base::StringView<char> propertyName;
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

    bool MaterialGraph::writeDataView(const base::IDataView* rootView, base::StringView<char> rootViewPath, base::StringView<char> viewPath, const void* sourceData, base::Type sourceType)
    {
        if (TBaseClass::writeDataView(rootView, rootViewPath, viewPath, sourceData, sourceType))
            return true;

        if (!viewPath.empty())
        {
            base::StringView<char> propertyName;
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

    bool MaterialGraph::describeDataView(base::StringView<char> viewPath, base::rtti::DataViewInfo& outInfo) const
    {
        base::StringView<char> propertyName;
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
    }

    //---

    // factory class for the material graph
    class MaterialGraphFactory : public base::res::IFactory
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphFactory, base::res::IFactory);

    public:
        virtual base::res::ResourceHandle createResource() const override final
        {
            auto ret = base::CreateSharedPtr<MaterialGraph>();

            auto outputBlock = base::CreateSharedPtr<MaterialGraphBlockOutput_Unlit>();
            ret->graph()->addBlock(outputBlock);

            return ret;
        }
    };

    RTTI_BEGIN_TYPE_CLASS(MaterialGraphFactory);
        RTTI_METADATA(base::res::FactoryClassMetadata).bindResourceClass<MaterialGraph>();
    RTTI_END_TYPE();

    //--


} // rendering
