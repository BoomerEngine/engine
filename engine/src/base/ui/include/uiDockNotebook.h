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

namespace ui
{
    //---

    class DockLayoutNode;

    // notebook used in the docking system
    class BASE_UI_API DockNotebook : public Notebook
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DockNotebook, Notebook);

    public:
        DockNotebook(DockLayoutNode* layoutNode);

        DockLayoutNode* layoutNode() const;

        INLINE ui::DockPanel* activeTab() const { return base::rtti_cast<ui::DockPanel>(Notebook::activeTab()); }

        void updateHeaderButtons();
        void closeTab(DockPanel* tab);

        virtual void attachTab(IElement* tab, IElement* afterTab = nullptr, bool activate = true) override;
        virtual void detachTab(IElement* tab, IElement* otherTabToActive = nullptr) override;

    protected:
        base::RefWeakPtr<DockLayoutNode> m_layoutNode;

        virtual base::StringBuf tabTitle(IElement* tab) override;
        virtual bool tabHasCloseButton(IElement* tab) override;
        virtual void tabHandleCloseRequest(IElement* tab) override;
    };
   
    //---

} // ui