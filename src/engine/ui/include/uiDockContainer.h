/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\docking #]
***/

#pragma once

#include "uiElement.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//---

/// persistent layout info
struct ENGINE_UI_API DockPersistentLayoutInfo
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(DockPersistentLayoutInfo);

    Array<StringID> panels;
    Array<StringID> visiblePanels;
    float split = 0.0f;

    Array<DockPersistentLayoutInfo> left;
    Array<DockPersistentLayoutInfo> right;
    Array<DockPersistentLayoutInfo> top;
    Array<DockPersistentLayoutInfo> bottom;
};

//---

/// docking container, manages and allows docking of panels
class ENGINE_UI_API DockContainer : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(DockContainer, IElement);

public:
    DockContainer();

    //--

    // get layout
    INLINE DockLayoutNode& layout() { return *m_rootLayoutNode; }

    //--

    // reapply layout
    void applyLayout();

    //--

    // store current layout to persistent data that can be saved in config
    void saveLayout(DockPersistentLayoutInfo& outLayout) const;

    // restore layout from persistent data that was probably loaded from config
    // NOTE: non-persistent tabs are usually closed here
    void loadLayout(const DockPersistentLayoutInfo& layout);

    //--

    // make sure given panel is visible (activate tab, window, etc)
    bool activatePanel(DockPanel* panel);

private:
    DockLayoutNodePtr m_rootLayoutNode;
    ElementPtr m_innerContent;

    bool m_layoutDirty = false;

    // apply the layout - refills the notebooks/windows with panels, technically should not disrupt anything :P
    void rebuildLayout();
};

//---

END_BOOMER_NAMESPACE_EX(ui)
