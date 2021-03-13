/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#pragma once

#include "engine/ui/include/uiWindow.h"
#include "engine/ui/include/uiDockContainer.h"
#include "engine/ui/include/uiDockPanel.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

/// top-level editor window, auto registers in the editor service (for convenience)
class EDITOR_COMMON_API IEditorWindow : public ui::Window
{
    RTTI_DECLARE_VIRTUAL_CLASS(IEditorWindow, ui::Window);

public:
    IEditorWindow(StringView tag, StringView title);
    virtual ~IEditorWindow();

    //--

    INLINE const StringBuf& tag() const { return m_tag; }

    ///---

    // get master docking area
    ui::DockLayoutNode& layout();

    ///---

    virtual void configLoad(const ui::ConfigBlock& block) override;
    virtual void configSave(const ui::ConfigBlock& block) const override;

    virtual bool unnecessary() const;

    virtual void update();

protected:
    ui::DockContainerPtr m_dockArea;

    StringBuf m_tag;
};

//---

END_BOOMER_NAMESPACE_EX(ed)
