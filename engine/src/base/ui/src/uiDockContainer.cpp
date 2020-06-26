/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\docking #]
***/

#include "build.h"
#include "uiDockPanel.h"
#include "uiDockContainer.h"
#include "uiDockLayout.h"

namespace ui
{

    //---

    RTTI_BEGIN_TYPE_CLASS(DockPersistentLayoutInfo);
        RTTI_PROPERTY(panels);
        RTTI_PROPERTY(visiblePanels);
        RTTI_PROPERTY(split);
        RTTI_PROPERTY(left);
        RTTI_PROPERTY(right);
        RTTI_PROPERTY(top);
        RTTI_PROPERTY(bottom);
    RTTI_END_TYPE();

    //---

    RTTI_BEGIN_TYPE_CLASS(DockContainer);
        RTTI_METADATA(ElementClassNameMetadata).name("DockContainer");
    RTTI_END_TYPE();

    DockContainer::DockContainer()
    {
        enableAutoExpand(true, true);

        if (!base::IsDefaultObjectCreation())
            m_rootLayoutNode = MemNew(DockLayoutNode, this, nullptr);
    }

    bool DockContainer::iteratePanels(const std::function<bool(DockPanel*)>& enumFunc, DockPanelIterationMode mode) const
    {
        return m_rootLayoutNode->iteratePanels(enumFunc, mode);
    }

    bool DockContainer::activatePanel(DockPanel* panel)
    {
        return m_rootLayoutNode->activatePanel(panel);
    }

    void DockContainer::applyLayout()
    {
        if (!m_layoutDirty)
        {
            m_layoutDirty = true;
            runSync<DockContainer>([](DockContainer& elem) { elem.rebuildLayout(); });
        }
    }

    void DockContainer::saveLayout(DockPersistentLayoutInfo& outLayout) const
    {

    }

    void DockContainer::loadLayout(const DockPersistentLayoutInfo& layout)
    {

    }

    void DockContainer::rebuildLayout()
    {
        m_layoutDirty = false;

        if (m_innerContent)
        {
            detachChild(m_innerContent);
            m_innerContent.reset();
        }

        m_innerContent = m_rootLayoutNode->visualize();

        if (m_innerContent)
        {
            m_innerContent->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            m_innerContent->customVerticalAligment(ui::ElementVerticalLayout::Expand);
            attachChild(m_innerContent);
        }
    }

    bool DockContainer::handleTemplateNewChild(const base::xml::IDocument& doc, const base::xml::NodeID& id, const base::xml::NodeID& childId, const ElementPtr& childElement)
    {
        if (auto panel = base::rtti_cast<DockPanel>(childElement))
        {
            auto dockModeName = doc.nodeAttributeOfDefault(childId, "dock");

            auto select = true;
            doc.nodeAttributeOfDefault(childId, "select").match(select);

            if (dockModeName == "bottom")
                layout().bottom().attachPanel(panel, select);
            else if (dockModeName == "top")
                layout().top().attachPanel(panel, select);
            else if (dockModeName == "left")
                layout().left().attachPanel(panel, select);
            else if (dockModeName == "right")
                layout().right().attachPanel(panel, select);
            else if (dockModeName == "tab")
                layout().attachPanel(panel, select);

            return true;
        }

        return false;
    }

    //---

} // ui