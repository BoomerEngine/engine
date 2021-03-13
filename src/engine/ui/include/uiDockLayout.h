/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\docking #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(ui)

//---

/// dock target information
struct ENGINE_UI_API DockTargetInfo
{
    enum class DockPlacement
    {
        Window, // place as a separate window
        Tab, // pace as a tab in a notebook
        Left, // place at the left side of the container
        Right, // place at the right side of the container
        Top, // place at the top side of the container
        Bottom, // place at the bottom side of the container
    };

    DockLayoutNode* target = nullptr; // target element to dock at

    DockPlacement placement = DockPlacement::Window; // how to place stuff

    int notebookTabIndex = -1; // tab index to dock at, in case when docking at a notebook 

    Position areaPosition; // where should be window/node placed
    Size areaSize; // size of the area (for visualization)

    //--

    INLINE DockTargetInfo() {};
    DockTargetInfo(DockPlacement method, DockLayoutNode* target, const Position& position, const Size& size, int tabIndex = -1);

    INLINE bool operator==(const DockTargetInfo& other) const
    {
        return (target == other.target) && (notebookTabIndex == other.notebookTabIndex) 
            && (placement == other.placement) && (areaPosition == other.areaPosition);
    }

    INLINE bool operator!=(const DockTargetInfo& other) const
    {
        return !operator==(other);
    }
};

//---

class DockLayoutNode;
typedef RefPtr<DockLayoutNode> DockLayoutNodePtr;

/// layout node
class ENGINE_UI_API DockLayoutNode : public IReferencable
{
public:
    DockLayoutNode(DockContainer* container, DockLayoutNode* parent, float splitNode = 0.0f);

    INLINE DockContainer* container() const { return m_container; }
    INLINE const DockNotebookPtr& notebook() const { return m_notebook; }

    DockLayoutNode& top(float split=0.0f);
    DockLayoutNode& left(float split=0.0f);
    DockLayoutNode& bottom(float split=0.0f);
    DockLayoutNode& right(float split=0.0f);

    bool hasAnythingVisible() const;

    void fillViewMenu(MenuButtonContainer* menu);

    ElementPtr visualize();

    //-

    // add panel to this layout node
    // NOTE: panel is not visualized until we apply the layout the the container
    void attachPanel(DockPanel* panel, bool select = true);

    // remove panel from this layout node
    void detachPanel(DockPanel* panel);

    // iterate over all panels
    void iteratePanels(const std::function<void(DockPanel*)>& enumFunc, DockPanelIterationMode mode = ui::DockPanelIterationMode::All) const;

    // iterate over all panels, allows faster exit
    bool iteratePanelsEx(const std::function<bool(DockPanel*)>& enumFunc, DockPanelIterationMode mode = ui::DockPanelIterationMode::All) const;

    // activate specific panel
    bool activatePanel(DockPanel* panel);

    // show/hide panel without deleting it
    bool showPanel(DockPanel* panel, bool visible);

    //--

    // get active panel that matches predicate
    template< typename T >
    INLINE RefPtr<T> activePanel(bool focusedOnly = false, const std::function<bool(T*)>& enumFunc = nullptr) const
    {
        RefPtr<T> ret;
        iteratePanelsEx([&enumFunc, &ret](DockPanel* panel)
            {
                if (auto specificPanel = rtti_cast<T>(panel))
                {
                    if (!enumFunc || enumFunc)
                    {
                        ret = AddRef(specificPanel);
                        return true;
                    }
                }
                return false;
            }, DockPanelIterationMode::ActiveOnly);

        return ret;
    }

    // find first panel matching predicate
    template< typename T >
    INLINE RefPtr<T> findPanel(bool visibleOnly = true, const std::function<bool(T*)>& enumFunc = nullptr) const
    {
        RefPtr<T> ret;
        iteratePanelsEx([&ret, &enumFunc](DockPanel* panel)
            {
                if (auto specificPanel = rtti_cast<T>(panel))
                {
                    if (!enumFunc || enumFunc)
                    {
                        ret = AddRef(specificPanel);
                        return true;
                    }
                }
                return false;
            }, visibleOnly ? DockPanelIterationMode::VisibleOnly : DockPanelIterationMode::All);

        return ret;
    }

    // iterate over all panels
    template< typename T >
    INLINE void iterateAllPanels(const std::function<void(T* panel)>& enumFunc) const
    {
        iteratePanels([&enumFunc](DockPanel* panel)
            {
                if (auto specificPanel = rtti_cast<T>(panel))
                    enumFunc(specificPanel);
            }, DockPanelIterationMode::All);
    }

    // iterate over all visible panels (each notebook has one)
    template< typename T >
    INLINE void iterateVisiblePanels(const std::function<void(T* panel)>& enumFunc) const
    {
        iteratePanels([&enumFunc](DockPanel* panel)
            {
                if (auto specificPanel = rtti_cast<T>(panel))
                    enumFunc(specificPanel);
            }, DockPanelIterationMode::VisibleOnly);
    }

    // get active panel
    template< typename T >
    INLINE void iterateActivePanels(const std::function<void(T* panel)>& enumFunc) const
    {
        iteratePanels([&enumFunc](DockPanel* panel)
            {
                if (auto specificPanel = rtti_cast<T>(panel))
                    enumFunc(specificPanel);
            }, DockPanelIterationMode::ActiveOnly);
    }

    // find active panel
    template< typename T >
    INLINE RefPtr<T> findActivePanel(const std::function<bool(T* panel)>& enumFunc = nullptr) const
    {
        RefPtr<T> ret;
        iteratePanelsEx([&enumFunc, &ret](DockPanel* panel)
            {
                if (auto specificPanel = rtti_cast<T>(panel))
                {
                    if (!enumFunc || enumFunc(specificPanel))
                    {
                        ret = AddRef(specificPanel);
                        return true;
                    }
                }
                return false;
            }, DockPanelIterationMode::ActiveOnly);

        return ret;
    }

    // check if we have visible panels
    template< typename T >
    INLINE bool hasPanels(const std::function<bool(T* panel)>& enumFunc = nullptr) const
    {
        return iteratePanelsEx([&enumFunc](DockPanel* panel) {
                if (auto specificPanel = rtti_cast<T>(panel))
                    if (!enumFunc || enumFunc(specificPanel))
                        return true;
                return false;
            }, DockPanelIterationMode::All);
    }

    // collect panels of given type
    template< typename T >
    INLINE Array<RefPtr<T>> collectPanels(const std::function<bool(T* panel)>& enumFunc = nullptr) const
    {
        Array<RefPtr<T>> ret;
        iteratePanels([&enumFunc, &ret](DockPanel* panel)
            {
                if (auto specificPanel = rtti_cast<T>(panel))
                    if (!enumFunc || enumFunc(specificPanel))
                        ret.pushBack(AddRef(specificPanel));
            }, DockPanelIterationMode::All);

        return ret;
    }

    // collect active panels of given type
    template< typename T >
    INLINE Array<RefPtr<T>> collectActivePanels(const std::function<bool(T* panel)>& enumFunc = nullptr) const
    {
        Array<RefPtr<T>> ret;
        iteratePanels([&enumFunc, &ret](DockPanel* panel)
            {
                if (auto specificPanel = rtti_cast<T>(panel))
                    if (!enumFunc || enumFunc(specificPanel))
                        ret.pushBack(AddRef(specificPanel));
            }, DockPanelIterationMode::ActiveOnly);

        return ret;
    }

    //--

protected:
    DockContainer* m_container = nullptr;
    DockLayoutNode* m_parent = nullptr;

    DockNotebookPtr m_notebook; // may not be attached if there's nothing visible
    RefWeakPtr<DockPanel> m_selectedPanel; // active panel in the notebook

    Array<DockPanelPtr> m_panels; // note
    float m_parentSplitFactor = 0.0f;

    DockLayoutNodePtr m_topNode;
    DockLayoutNodePtr m_leftNode;
    DockLayoutNodePtr m_bottomNode;
    DockLayoutNodePtr m_rightNode;
};

//---

END_BOOMER_NAMESPACE_EX(ui)
