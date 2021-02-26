/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\docking #]
***/

#pragma once

#include "uiElement.h"
#include "uiNotebook.h"
#include "uiSplitter.h"
#include "uiWindow.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//---

class DockLayoutNode;
class DockPanel;

// notebook used in the docking system
class ENGINE_UI_API DockNotebook : public Notebook
{
    RTTI_DECLARE_VIRTUAL_CLASS(DockNotebook, Notebook);

public:
    DockNotebook(DockLayoutNode* layoutNode);

    DockLayoutNode* layoutNode() const;

    DockPanel* activeTab() const;

    void updateHeaderButtons();
    void closeTab(DockPanel* tab);

    virtual void attachTab(IElement* tab, IElement* afterTab = nullptr, bool activate = true) override;
    virtual void detachTab(IElement* tab, IElement* otherTabToActive = nullptr) override;

protected:
    RefWeakPtr<DockLayoutNode> m_layoutNode;

    virtual StringBuf tabTitle(IElement* tab) override;
    virtual bool tabHasCloseButton(IElement* tab) override;
    virtual void tabHandleCloseRequest(IElement* tab) override;
};
   
//---

END_BOOMER_NAMESPACE_EX(ui)
