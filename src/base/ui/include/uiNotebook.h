/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\layout #]
***/

#pragma once

#include "uiElement.h"

BEGIN_BOOMER_NAMESPACE(ui)

//--

DECLARE_UI_EVENT(EVENT_TAB_CONTEXT_MENU, Position);

//--

/// flags for notebook
enum class NotebookFlagBit : uint16_t
{
    AutoSelectNewTabs = FLAG(1), // Automatically select new tabs when they appear
    TabListPopupButton = FLAG(2), // Add a popup button to open a tab list
    AllowCloseTabs = FLAG(3), // Tabs can be closed
    NoCloseWithMiddleMouseButton = FLAG(4), // Disable behavior of closing tabs with middle mouse button 
};

typedef base::DirectFlags<NotebookFlagBit> NotebookFlags;

/// simple notebook (does not support docking)
/// - Tab title is taken as "title" style from the element added as a tab
/// - When particular tab is activated the tab element gets "OnTabActivated" event
/// - When particular tab is deactivated the tab element gets "OnTabDectivated" event
/// - When particular tab is requested to close it gets the "OnTabClose" event, if "true" is returned the tab is closed immediately
/// - When tab is changed we emit the "OnTab" event
class BASE_UI_API Notebook : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(Notebook, IElement);

public:
    Notebook(NotebookFlags flags = { NotebookFlagBit::AllowCloseTabs, NotebookFlagBit::AutoSelectNewTabs });
    virtual ~Notebook();

    //--
        
    // list of all tabs
    INLINE const base::Array<IElement*>& tabs() const { return m_tabs; }

    // active tab
    INLINE ui::IElement* activeTab() const { return m_activeTab; }

    // list of all tabs with given type (NOTE: do not cache directly)
    template< typename T >
    INLINE const base::Array<T*>& typedTabsRef() const
    {
        m_tempTabs.reserve(m_tabs.size());
        m_tempTabs.reset();
        for (auto* tab : m_tabs)
            if (auto* typedTab = base::rtti_cast<T>(tab))
                m_tempTabs.pushBack(tab);
        return (const base::Array<T*>&) m_tempTabs;
    }

    //--

    // get current tab (may be NULL)
    template< typename T >
    INLINE T* tab() const
    {
        return base::rtti_cast<T>(m_activeTab);
    }

    // remove (detach) tabs from the notebook
    virtual void removeAllTabs();

    // select tab
    virtual void tab(IElement* tab);

    // check if we contain given element as a tab
    virtual bool containsTab(IElement* tab);

    // attach tab, optionally we can specify after which tab we want to be inserted (by default it's added at the end)
    virtual void attachTab(IElement* tab, IElement* afterTab = nullptr, bool activate = true);

    // detach tab, optionally we can specify which tab should be activated next (by default it's the next tab in the list, or previous if we were the last tab)
    virtual void detachTab(IElement* tab, IElement* otherTabToActive = nullptr);

    //--

    // attach element as a tab, calls attachTab with default parameters
    virtual void attachChild(IElement* childElement) override final;

    // detach tab element, calls detachTab with default parameters
    virtual void detachChild(IElement* childElement) override final;

    //--

    // attach a custom header element (usually a drop down button)
    void attachHeaderElement(IElement* element);

    // detach custom header telement
    void dettachHeaderElement(IElement* element);

    //--

private:
    virtual void computeSize(Size& outSize) const override; // biggest size of all tabs

    IElement* m_activeTab = nullptr;
    base::Array<Button*> m_buttons;

    base::Array<IElement*> m_tabs; 

    base::Array<IElement*> m_customHeaderElements;

    mutable base::Array<IElement*> m_tempTabs;

    NotebookFlags m_flags;

    ButtonPtr createHeaderButtonForTab(IElement* tab);
    void updateHeaderButtonData(Button* button, IElement* tab);

    ElementPtr m_headerTotalArea;

    ElementPtr m_header;
    ScrollAreaPtr m_headerScrollArea;

    ElementPtr m_container;
    ButtonPtr m_button;

protected:
    void updateHeaderButtons();

    IElement* tabForButton(IElement* button);

    virtual base::StringBuf tabTitle(IElement* tab);
    virtual bool tabIsVisible(IElement* tab);
    virtual bool tabHasCloseButton(IElement* tab);
    virtual void tabHandleCloseRequest(IElement* tab);
    virtual bool tabHandleContextMenu(IElement* tab, const Position& pos);
};

//--

END_BOOMER_NAMESPACE(ui)
