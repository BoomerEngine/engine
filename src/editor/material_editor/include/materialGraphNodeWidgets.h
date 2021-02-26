/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "engine/ui/include/uiGraphEditorNodeInnerWidget.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

// constant scalar editor
class EDITOR_MATERIAL_EDITOR_API MaterialGraphConstantScalarWidget : public ui::IGraphNodeInnerWidget
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphConstantScalarWidget, ui::IGraphNodeInnerWidget);

public:
    virtual bool bindToBlock(graph::Block* block) override;
    virtual void bindToActionHistory(ActionHistory* history) override;

    ui::DataBoxPtr m_box;
};

//--

// constant scalar editor
class EDITOR_MATERIAL_EDITOR_API MaterialGraphConstantColorWidget : public ui::IGraphNodeInnerWidget
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphConstantColorWidget, ui::IGraphNodeInnerWidget);

public:
    virtual bool bindToBlock(graph::Block* block) override;
    virtual void bindToActionHistory(ActionHistory* history) override;

    ui::DataBoxPtr m_box;
};

//--

END_BOOMER_NAMESPACE_EX(ed)
