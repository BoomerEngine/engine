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

class DockNode;

//---

/// a dockable panel, his element is never destroyed
class ENGINE_UI_API DockPanel : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(DockPanel, IElement);

public:
    DockPanel(StringView title = "", StringView id = "");
    virtual ~DockPanel();

    // get panel ID (for saving layout, panels without ID are not saved)
    INLINE const StringBuf& id() const { return m_id; }

    // get panel icon
    INLINE const StringBuf& tabIcon() const { return m_icon; }

    // get panel title text
    INLINE const StringBuf& tabTitle() const { return m_title; }

    /// should this panel be visible in the layout ?
    INLINE bool tabVisibleInLayout() const { return m_visibleInLayout; }

    /// should we display the close button
    INLINE bool tabHasCloseButton() const { return m_hasCloseButton; }

    /// is tab locked (will show message when user wants to close it)
    INLINE bool tabLocked() const { return m_locked; }

    /// general (user controled) modified flag (adds a *)
    INLINE bool tabModified() const { return m_modified; }

    //--

    // change title
    void tabTitle(const StringBuf& titleString);

    // change icon
    void tabIcon(const StringBuf& icon);

    // toggle the close button
    void tabCloseButton(bool flag);

    // toggle lock state of the tab
    void tabLocked(bool flag);

    // toggle modified state
    void tabModified(bool flag);

    // remove this panel from whatever tab it's in
    virtual void close();

    // request closing of this dock panel
    virtual void handleCloseRequest();

    //--

    // compile a title string
    virtual StringBuf compileTabTitleString() const;

    //--

private:
    StringBuf m_id;

    StringBuf m_title;
    StringBuf m_icon;
    bool m_hasCloseButton = true;
    bool m_visibleInLayout = true;
    bool m_locked = false;
    bool m_modified = false;

    friend class DockNotebook;
    friend class DockLayoutNode;
};
    
//---

END_BOOMER_NAMESPACE_EX(ui)
