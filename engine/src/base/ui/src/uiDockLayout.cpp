/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\docking #]
***/

#include "build.h"
#include "uiDockPanel.h"
#include "uiDockNotebook.h"
#include "uiDockLayout.h"
#include "uiDockContainer.h"
#include "uiMenuBar.h"

namespace ui
{
    //---

    DockTargetInfo::DockTargetInfo(DockPlacement method_, DockLayoutNode* target_, const Position& position_, const Size& size_, int tabIndex_)
        : target(target_)
        , placement(method_)
        , notebookTabIndex(tabIndex_)
        , areaPosition(position_)
        , areaSize(size_)
    {}

    //--

    DockLayoutNode::DockLayoutNode(DockContainer* container, DockLayoutNode* parent, float splitNode /*= 0.0f*/)
        : m_parentSplitFactor(splitNode)
        , m_container(container)
        , m_parent(parent)
    {
        m_notebook = base::RefNew<ui::DockNotebook>(this);
    }

    DockLayoutNode& DockLayoutNode::top(float split /*= 0.0f*/)
    {
        if (!m_topNode)
            m_topNode = base::RefNew<DockLayoutNode>(m_container, this, split);
        return *m_topNode;
    }

    DockLayoutNode& DockLayoutNode::left(float split /*= 0.0f*/)
    {
        if (!m_leftNode)
            m_leftNode = base::RefNew<DockLayoutNode>(m_container, this, split);
        return *m_leftNode;
    }

    DockLayoutNode& DockLayoutNode::bottom(float split /*= 0.0f*/)
    {
        if (!m_bottomNode)
            m_bottomNode = base::RefNew<DockLayoutNode>(m_container, this, split);
        return *m_bottomNode;
    }

    DockLayoutNode& DockLayoutNode::right(float split /*= 0.0f*/)
    {
        if (!m_rightNode)
            m_rightNode = base::RefNew<DockLayoutNode>(m_container, this, split);
        return *m_rightNode;
    }

    bool DockLayoutNode::hasAnythingVisible() const
    {
        for (const auto& panel : m_panels)
            if (panel->tabVisibleInLayout())
                return true;

        if (m_topNode && m_topNode->hasAnythingVisible())
            return true;
        if (m_bottomNode && m_bottomNode->hasAnythingVisible())
            return true;
        if (m_leftNode && m_leftNode->hasAnythingVisible())
            return true;
        if (m_rightNode && m_rightNode->hasAnythingVisible())
            return true;

        return false;
    }
    
    bool DockLayoutNode::showPanel(DockPanel* panel, bool visible)
    {
        if (m_panels.contains(panel))
        {
            if (panel->m_visibleInLayout != visible)
            {
                panel->m_visibleInLayout = visible;
                m_container->applyLayout();
                return true;
            }
        }

        if (m_topNode && m_topNode->showPanel(panel, visible))
            return true;
        if (m_bottomNode && m_bottomNode->showPanel(panel, visible))
            return true;
        if (m_leftNode && m_leftNode->showPanel(panel, visible))
            return true;
        if (m_rightNode && m_rightNode->showPanel(panel, visible))
            return true;

        return false;
    }

    void DockLayoutNode::attachPanel(DockPanel* panel, bool select)
    {
        if (panel)
            m_panels.pushBackUnique(AddRef(panel));

        if (select)
            m_selectedPanel = panel;

        m_container->applyLayout();
    }

    void DockLayoutNode::detachPanel(DockPanel* panel)
    {
        if (m_panels.contains(panel))
        {
            m_panels.removeUnordered(panel);
            m_container->applyLayout();
            //if (panel->parentElement() == m_notebook)
            //{
            //    m_notebook->detachTab(panel);
            //}
        }
    }

    bool DockLayoutNode::activatePanel(DockPanel* panel)
    {
        if (m_notebook->tabs().contains(panel))
        {
            m_notebook->tab(panel);
            return true;            
        }

        if (m_topNode && m_topNode->activatePanel(panel))
            return true;
        if (m_bottomNode && m_bottomNode->activatePanel(panel))
            return true;
        if (m_leftNode && m_leftNode->activatePanel(panel))
            return true;
        if (m_rightNode && m_rightNode->activatePanel(panel))
            return true;

        return false;
    }

    bool DockLayoutNode::iteratePanels(const std::function<bool(DockPanel*)>& enumFunc, DockPanelIterationMode mode) const
    {
        if (mode == DockPanelIterationMode::All)
        {
            for (const auto& panel : m_panels)
                if (enumFunc(panel))
                    return true;
        }
        else if (mode == DockPanelIterationMode::VisibleOnly)
        {
            for (const auto& panel : m_panels)
                if (panel->tabVisibleInLayout() && enumFunc(panel))
                    return true;
        }
        else if (mode == DockPanelIterationMode::ActiveOnly)
        {
            if (auto activeTab = m_notebook->activeTab())
                if (enumFunc(activeTab))
                    return true;
        }

        if (m_leftNode && m_leftNode->iteratePanels(enumFunc, mode))
            return true;

        if (m_rightNode && m_rightNode->iteratePanels(enumFunc, mode))
            return true;

        if (m_topNode && m_topNode->iteratePanels(enumFunc, mode))
            return true;

        if (m_bottomNode && m_bottomNode->iteratePanels(enumFunc, mode))
            return true;

        return false;
    }

    static float CalcSplitFactorMin(float factor, float defaultValue)
    {
        if (factor <= 0.0f)
            factor = defaultValue;

        return std::clamp<float>(factor, 0.05f, 0.95f);
    }

    static float CalcSplitFactorMax(float factor, float defaultValue)
    {
        if (factor <= 0.0f)
            factor = defaultValue;

        return std::clamp<float>(1.0f - factor, 0.05f, 0.95f);
    }

    ElementPtr DockLayoutNode::visualize()
    {
        // get pad to select
        DockPanelPtr selectedPanel = m_selectedPanel.lock();
        if (!selectedPanel)
            selectedPanel = AddRef(m_notebook->activeTab());
        m_selectedPanel = nullptr;

        // cleanup center notebook
        m_notebook->removeAllTabs();

        // rebuild center notebook with tabs that are valid
        for (const auto& panel : m_panels)
            if (panel->tabVisibleInLayout())
                m_notebook->attachTab(panel, nullptr, panel == selectedPanel);

        ElementPtr ret;

        // detach notebook (rips apart old hierarchy)
        if (m_notebook->parentElement())
            m_notebook->parentElement()->detachChild(ret);

        // if center notebook has content use it as the root
        // NOTE: also keep the center notebook in case we are the root node
        if (!m_notebook->tabs().empty() || m_parent == nullptr)
        {
            ret = m_notebook;
        }

        // add right split
        if (m_rightNode)
        {
            if (auto content = m_rightNode->visualize())
            {
                if (ret)
                {
                    auto splitFactor = CalcSplitFactorMax(m_rightNode->m_parentSplitFactor, 0.33f);
                    auto splitter = base::RefNew<Splitter>(Direction::Vertical, splitFactor);
                    splitter->attachChild(ret);
                    splitter->attachChild(content);
                    ret = splitter;
                }
                else
                {
                    ret = content;
                }
            }
        }

        // add left split
        if (m_leftNode)
        {
            if (auto content = m_leftNode->visualize())
            {
                if (ret)
                {
                    auto splitFactor = CalcSplitFactorMin(m_leftNode->m_parentSplitFactor, 0.33f);
                    auto splitter = base::RefNew<Splitter>(Direction::Vertical, splitFactor);
                    splitter->attachChild(content);
                    splitter->attachChild(ret);
                    ret = splitter;
                }
                else
                {
                    ret = content;
                }
            }
        }

        // add bottom split
        if (m_bottomNode)
        {
            if (auto content = m_bottomNode->visualize())
            {
                if (ret)
                {
                    auto splitFactor = CalcSplitFactorMax(m_bottomNode->m_parentSplitFactor, 0.33f);
                    auto splitter = base::RefNew<Splitter>(Direction::Horizontal, splitFactor);
                    splitter->attachChild(ret);
                    splitter->attachChild(content);
                    ret = splitter;
                }
                else
                {
                    ret = content;
                }
            }
        }

        // add top split
        if (m_topNode)
        {
            if (auto content = m_topNode->visualize())
            {
                if (ret)
                {
                    auto splitFactor = CalcSplitFactorMin(m_topNode->m_parentSplitFactor, 0.33f);
                    auto splitter = base::RefNew<Splitter>(Direction::Horizontal, splitFactor);
                    splitter->attachChild(content);
                    splitter->attachChild(ret);
                    ret = splitter;
                }
                else
                {
                    ret = content;
                }
            }
        }

        // return content
        DEBUG_CHECK_EX(!ret || !ret->parentElement(), "Create content should not be attached to anything");
        return ret;
    }

    //---

    void DockLayoutNode::fillViewMenu(MenuButtonContainer* menu)
    {
        // collect the persistent panels - ones with IDs
        base::InplaceArray<DockPanel*, 10> persistentPanels;
        iteratePanels([&persistentPanels](DockPanel* panel)
            {
                if (!panel->id().empty())
                    persistentPanels.pushBack(panel);
                return false;
            }, DockPanelIterationMode::All);

        // create the items to toggle visibility of stuff
        if (!persistentPanels.empty())
        {
            menu->createSeparator();

            auto selfRef = base::RefWeakPtr<DockLayoutNode>(this);
            for (auto* panel : persistentPanels)
            {
                const auto visible = panel->tabVisibleInLayout();
                const auto icon = visible ? "[img:tick]" : "";
                const auto panelRef = base::RefWeakPtr<DockPanel>(panel);
                menu->createCallback(panel->compileTabTitleString(), icon) = [selfRef, visible, panelRef]()
                {
                    if (auto node = selfRef.lock())
                        if (auto panel = panelRef.lock())
                            node->showPanel(panel, !visible);
                };
            }
        }
    }

    //---

} // ui


