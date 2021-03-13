/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "engine/ui/include/uiSimpleListView.h"
#include "engine/ui/include/uiListViewEx.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

class MaterialParametersPanel;

/// preview item for the parameter
class EDITOR_MATERIAL_EDITOR_API MaterialParametersPanel_ParameterElement : public ui::IListItem
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialParametersPanel_ParameterElement, ui::IListItem);

public:
    MaterialParametersPanel_ParameterElement(IMaterialTemplateParam* param, ActionHistory* ah, bool initiallyExpaned = false);

    INLINE const MaterialTemplateParamPtr& param() const { return m_param; }

    void applyName(StringView name);

private:
    MaterialTemplateParamPtr m_param;

    ui::EditBoxPtr m_name;
    ui::DataInspectorPtr m_data;

    void cmdRemove();
    void cmdTryRename();

    virtual bool handleItemFilter(const ui::ICollectionView* view, const ui::SearchPattern& filter) const override final;
    virtual void handleItemSort(const ui::ICollectionView* view, int colIndex, SortingData& outInfo) const override final;
};

//--

/// panel that can edit the list of material parameters
class EDITOR_MATERIAL_EDITOR_API MaterialParametersPanel : public ui::IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialParametersPanel, ui::IElement);

public:
    MaterialParametersPanel(const ActionHistoryPtr& actions);
    virtual ~MaterialParametersPanel();

    //--

    void bindGraph(const MaterialGraphPtr& graph);

    //--

    void addParameter(SpecificClassType<IMaterialTemplateParam> paramType);

    bool removeParameter(IMaterialTemplateParam* param);

    bool renameParameter(IMaterialTemplateParam* param, StringView newName);

private:
    ActionHistoryPtr m_actionHistrory;
    MaterialGraphPtr m_graph;

    //--

    ui::ListViewExPtr m_list;

    //--

    RefPtr<MaterialParametersPanel_ParameterElement> createUI(IMaterialTemplateParam* param, bool expanded);

    RefPtr<MaterialParametersPanel_ParameterElement> findUI(IMaterialTemplateParam* param) const;

    void rebuildParameterList();
};

END_BOOMER_NAMESPACE_EX(ed)
