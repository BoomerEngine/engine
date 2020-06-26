/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\docking #]
***/

#pragma once

namespace ui
{
    //---

    /// dock target information
    struct BASE_UI_API DockTargetInfo
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
    typedef base::RefPtr<DockLayoutNode> DockLayoutNodePtr;

    /// layout node
    class BASE_UI_API DockLayoutNode : public base::IReferencable
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

        // iterate over panels
        bool iteratePanels(const std::function<bool(DockPanel*)>& enumFunc, DockPanelIterationMode mode) const;

        // activate specific panel
        bool activatePanel(DockPanel* panel);

        // show/hide panel without deleting it
        bool showPanel(DockPanel* panel, bool visible);

    protected:
        DockContainer* m_container = nullptr;
        DockLayoutNode* m_parent = nullptr;

        DockNotebookPtr m_notebook; // may not be attached if there's nothing visible
        base::RefWeakPtr<DockPanel> m_selectedPanel; // active panel in the notebook

        base::Array<DockPanelPtr> m_panels; // note
        float m_parentSplitFactor = 0.0f;

        DockLayoutNodePtr m_topNode;
        DockLayoutNodePtr m_leftNode;
        DockLayoutNodePtr m_bottomNode;
        DockLayoutNodePtr m_rightNode;
    };

    //---

} // ui