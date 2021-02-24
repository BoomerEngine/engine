/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\docking #]
***/

#pragma once

#include "uiElement.h"

BEGIN_BOOMER_NAMESPACE(ui)

//---

/// persistent layout info
struct BASE_UI_API DockPersistentLayoutInfo
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(DockPersistentLayoutInfo);

    base::Array<base::StringID> panels;
    base::Array<base::StringID> visiblePanels;
    float split = 0.0f;

    base::Array<DockPersistentLayoutInfo> left;
    base::Array<DockPersistentLayoutInfo> right;
    base::Array<DockPersistentLayoutInfo> top;
    base::Array<DockPersistentLayoutInfo> bottom;
};

//---

/// docking container, manages and allows docking of panels
class BASE_UI_API DockContainer : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(DockContainer, IElement);

public:
    DockContainer();

    //--

    // get layout
    INLINE DockLayoutNode& layout() { return *m_rootLayoutNode; }

    //--

    // iterate over panels
    bool iteratePanels(const std::function<bool(DockPanel*)>& enumFunc, DockPanelIterationMode mode = DockPanelIterationMode::VisibleOnly) const;

    // iterate over panels
    template< typename T >
    INLINE bool iterateSpecificPanels(const std::function<bool(T * panel)>& enumFunc, DockPanelIterationMode mode = DockPanelIterationMode::VisibleOnly) const
    {
        return iteratePanels([enumFunc](DockPanel* panel)
            {
                if (auto specificPanel = base::rtti_cast<T>(panel))
                    return enumFunc(specificPanel);
                return false;
            }, mode);
    }

    // collect specific panels
    template< typename T >
    INLINE bool collectSpecificPanels(base::Array<T*>& outTabs, DockPanelIterationMode mode = DockPanelIterationMode::VisibleOnly) const
    {
        return iteratePanels([&outTabs](DockPanel* panel)
            {
                if (auto specificPanel = base::rtti_cast<T>(panel))
                    outTabs.pushBack(specificPanel);
                return false;
            }, mode);
    }

    // make sure given panel is visible (activate tab, window, etc)
    bool activatePanel(DockPanel* panel);

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

private:
    DockLayoutNodePtr m_rootLayoutNode;
    ElementPtr m_innerContent;

    bool m_layoutDirty = false;

    // apply the layout - refills the notebooks/windows with panels, technically should not disrupt anything :P
    void rebuildLayout();


    virtual bool handleTemplateNewChild(const base::xml::IDocument& doc, const base::xml::NodeID& id, const base::xml::NodeID& childId, const ElementPtr& childElement) override final;
};

//---

END_BOOMER_NAMESPACE(ui)