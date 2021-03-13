/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#pragma once

#include "engine/ui/include/uiDockPanel.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

/// panel in the top-level editor window
class EDITOR_COMMON_API IEditorPanel : public ui::DockPanel
{
    RTTI_DECLARE_VIRTUAL_CLASS(IEditorPanel, ui::DockPanel);

public:
    IEditorPanel(StringView title, StringView id);
    virtual ~IEditorPanel();

    ///---

    virtual void configLoad(const ui::ConfigBlock& block) override;
    virtual void configSave(const ui::ConfigBlock& block) const override;

    virtual void update();

    virtual bool handleEditorClose();

    ///---
};

//---

END_BOOMER_NAMESPACE_EX(ed)
