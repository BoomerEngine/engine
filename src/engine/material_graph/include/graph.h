/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material #]
***/

#pragma once

#include "core/graph/include/graphContainer.h"
#include "engine/material/include/materialTemplate.h"

BEGIN_BOOMER_NAMESPACE()

///---

/// container for the material graph
class ENGINE_MATERIAL_GRAPH_API MaterialGraphContainer : public graph::Container
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphContainer, graph::Container);

public:
    MaterialGraphContainer();
    virtual ~MaterialGraphContainer();

    //--

    const MaterialGraphBlockOutput* findOutputBlock() const;

    bool checkParameterUsed(IMaterialTemplateParam* param) const;

    //--

protected:
    virtual bool canAddBlockOfClass(ClassType blockClass) const override;
    virtual void supportedBlockClasses(Array<SpecificClassType<graph::Block>>& outBlockClasses) const override;
    virtual void notifyStructureChanged() override;
    virtual void onPostLoad() override;
};

///---

/// a graph based material template
class ENGINE_MATERIAL_GRAPH_API MaterialGraph : public MaterialTemplate
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraph, MaterialTemplate);

public:
    MaterialGraph();
    virtual ~MaterialGraph();

    /// get the editable graph
    INLINE const MaterialGraphContainerPtr& graph() const { return m_graph; }

    //--

    // create ad-hoc preview template for this material graph
    MaterialTemplatePtr createPreviewTemplate(StringView label) const;

    //--

    bool checkParameterUsed(IMaterialTemplateParam* param) const;

    bool renameParameter(IMaterialTemplateParam* param, StringID name);

    void attachParameter(IMaterialTemplateParam* param);

    bool detachParameter(IMaterialTemplateParam* param);

    //--

private:
    MaterialGraphContainerPtr m_graph;

    virtual RefPtr<IMaterialTemplateDynamicCompiler> queryDynamicCompiler() const override final;

    // IObject - extension of object property model to allow direct writing to graph parameters via the graph object itself
    //virtual bool readDataView(const IDataView* rootView, StringView rootViewPath, StringView viewPath, void* targetData, Type targetType) const override;
    //virtual bool writeDataView(const IDataView* rootView, StringView rootViewPath, StringView viewPath, const void* sourceData, Type sourceType) override;
    //virtual bool describeDataView(StringView viewPath, DataViewInfo& outInfo) const override;
};

///---

END_BOOMER_NAMESPACE()
