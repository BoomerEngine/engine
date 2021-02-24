/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material #]
***/

#pragma once

#include "base/graph/include/graphContainer.h"
#include "../../material/include/renderingMaterialTemplate.h"

BEGIN_BOOMER_NAMESPACE(rendering)

///---

/// container for the material graph
class RENDERING_MATERIAL_GRAPH_API MaterialGraphContainer : public base::graph::Container
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphContainer, base::graph::Container);

public:
    MaterialGraphContainer();
    virtual ~MaterialGraphContainer();

    //--

    INLINE const base::Array<MaterialGraphParameterBlockPtr>& parameters() const { return m_parameters; }

    //--

    void refreshParameterList();

    const MaterialGraphBlockOutput* findOutputBlock() const;

    const MaterialGraphBlockParameter* findParamBlock(base::StringID name) const;

    MaterialGraphBlockParameter* findParamBlock(base::StringID name);

    //--

protected:
    virtual bool canAddBlockOfClass(base::ClassType blockClass) const override;
    virtual void supportedBlockClasses(base::Array<base::SpecificClassType<base::graph::Block>>& outBlockClasses) const override;
    virtual void notifyStructureChanged() override;
    virtual void onPostLoad() override;

    void cacheParameterBlocks();

    void buildParameterList(base::Array<MaterialGraphParameterBlockPtr>& outParamList) const;
    void buildParameterMap();

    base::Array<MaterialGraphParameterBlockPtr> m_parameters;
    base::HashMap<base::StringID, MaterialGraphBlockParameter*> m_parameterMap;
};

///---

/// a graph based material template
class RENDERING_MATERIAL_GRAPH_API MaterialGraph : public MaterialTemplate
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraph, MaterialTemplate);

public:
    MaterialGraph();
    virtual ~MaterialGraph();

    /// get the editable graph
    INLINE const MaterialGraphContainerPtr& graph() const { return m_graph; }

    //--

    // create ad-hoc preview template for this material graph
    MaterialTemplatePtr createPreviewTemplate(base::StringView label) const;

    //--

private:
    MaterialGraphContainerPtr m_graph;

    // MaterialTemplate
    virtual void listParameters(base::rtti::DataViewInfo& outInfo) const override final;
    virtual bool queryParameterInfo(base::StringID name, MaterialTemplateParamInfo& outInfo) const override final;
    virtual void queryMatadata(MaterialTemplateMetadata& outMetadata) const override final;
    virtual void queryAllParameterInfos(base::Array<MaterialTemplateParamInfo>& outParams) const override final;

    virtual const void* findParameterDataInternal(base::StringID name, base::Type& outType) const override final; // NOTE: returns pointer to the value inside the material block that defines the value

    virtual base::RefPtr<IMaterialTemplateDynamicCompiler> queryDynamicCompiler() const override final;

    // IObject - extension of object property model to allow direct writing to graph parameters via the graph object itself
    //virtual bool readDataView(const base::IDataView* rootView, base::StringView rootViewPath, base::StringView viewPath, void* targetData, base::Type targetType) const override;
    //virtual bool writeDataView(const base::IDataView* rootView, base::StringView rootViewPath, base::StringView viewPath, const void* sourceData, base::Type sourceType) override;
    //virtual bool describeDataView(base::StringView viewPath, base::rtti::DataViewInfo& outInfo) const override;
};

///---

END_BOOMER_NAMESPACE(rendering)