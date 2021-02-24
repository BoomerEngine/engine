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

#include "uiButton.h"
#include "uiInputAction.h"
#include "uiWindow.h"
#include "uiTextLabel.h"

#include "base/input/include/inputStructures.h"

BEGIN_BOOMER_NAMESPACE(ui)

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(DockNotebook);
    RTTI_METADATA(ElementClassNameMetadata).name("DockNotebook");
RTTI_END_TYPE();

DockNotebook::DockNotebook(DockLayoutNode* layoutNode)
    : m_layoutNode(layoutNode)
{
    enableAutoExpand(true, true);
}

DockLayoutNode* DockNotebook::layoutNode() const
{
    return m_layoutNode.unsafe();
}

ui::DockPanel* DockNotebook::activeTab() const
{
    return base::rtti_cast<ui::DockPanel>(Notebook::activeTab());
}

void DockNotebook::updateHeaderButtons()
{
    TBaseClass::updateHeaderButtons();
}

bool DockNotebook::tabHasCloseButton(IElement* tab)
{
    if (auto dockPanel = base::rtti_cast<DockPanel>(tab))
        return dockPanel->tabHasCloseButton();

    return TBaseClass::tabHasCloseButton(tab);
}

void DockNotebook::closeTab(DockPanel* tab)
{
    // manage in the layout
    if (auto node = m_layoutNode.lock())
    {
        if (auto dockPanel = base::rtti_cast<DockPanel>(tab))
        {
            if (dockPanel->id())
            {
                dockPanel->m_visibleInLayout = false;
                node->container()->applyLayout();
            }
            else
            {
                node->detachPanel(dockPanel);
            }
            return;
        }
    }

    // just close
    TBaseClass::tabHandleCloseRequest(tab);
}

void DockNotebook::tabHandleCloseRequest(IElement* tab)
{
    if (auto dockPanel = base::rtti_cast<DockPanel>(tab))
        dockPanel->handleCloseRequest();
    else
        TBaseClass::tabHandleCloseRequest(tab);
}

base::StringBuf DockNotebook::tabTitle(IElement* tab)
{
    if (auto dockPanel = base::rtti_cast<DockPanel>(tab))
        return dockPanel->compileTabTitleString();

    return TBaseClass::tabTitle(tab);
}

void DockNotebook::attachTab(IElement* tab, IElement* afterTab /*= nullptr*/, bool activate /*= true*/)
{
    TBaseClass::attachTab(tab, afterTab, activate);
}

void DockNotebook::detachTab(IElement* tab, IElement* otherTabToActive /*= nullptr*/)
{
    TBaseClass::detachTab(tab, otherTabToActive);
}

//---

END_BOOMER_NAMESPACE(ui)