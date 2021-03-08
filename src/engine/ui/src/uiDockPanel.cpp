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
#include "uiMessageBox.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

RTTI_BEGIN_TYPE_CLASS(DockPanel);
    RTTI_METADATA(ElementClassNameMetadata).name("DockPanel");
RTTI_END_TYPE();

DockPanel::DockPanel(StringView title /*= ""*/, StringView id /*= ""*/)
    : m_title(title)
    , m_hasCloseButton(!id.empty())
    , m_id(id)
{
    enableAutoExpand(true, true);
    addStyleClass("expand"_id);
}

DockPanel::~DockPanel()
{
}

void DockPanel::tabLocked(bool flag)
{
    if (m_locked != flag)
    {
        m_locked = flag;

        if (auto notebook = findParent<DockNotebook>())
            notebook->updateHeaderButtons();
    }
}

void DockPanel::tabModified(bool flag)
{
    if (m_modified != flag)
    {
        m_modified = flag;

        if (auto notebook = findParent<DockNotebook>())
            notebook->updateHeaderButtons();
    }
}

void DockPanel::tabTitle(StringView titleString)
{
    if (m_title != titleString)
    {
        m_title = StringBuf(titleString);

        if (auto notebook = findParent<DockNotebook>())
            notebook->updateHeaderButtons();
    }
}

void DockPanel::tabIcon(StringView icon)
{
    if (m_icon != icon)
    {
        m_icon = StringBuf(icon);

        if (auto notebook = findParent<DockNotebook>())
            notebook->updateHeaderButtons();
    }
}


void DockPanel::tabCloseButton(bool flag)
{
    if (m_hasCloseButton != flag)
    {
        m_hasCloseButton = flag;

        if (auto notebook = findParent<DockNotebook>())
            notebook->updateHeaderButtons();
    }
}

void DockPanel::close()
{
    if (auto notebook = findParent<DockNotebook>())
        notebook->closeTab(this);
}

void DockPanel::handleCloseRequest()
{
    if (m_locked)
    {
        MessageBoxSetup setup;
        setup.title("Close locked tab").yes().no().defaultNo().question();

        StringBuilder txt;
        txt.appendf("Tab '{}' is locked, close anyway?", m_title);
        setup.message(txt.view());

        if (MessageButton::Yes != ShowMessageBox(this, setup))
            return;
    }

    close();
}

StringBuf DockPanel::compileTabTitleString() const
{
    StringBuilder txt;

    if (m_locked)
        txt << "[img:lock] ";
    else if (m_icon)
        txt.appendf("[img:{}] ", m_icon);

    txt << m_title;

    if (m_modified)
        txt << "*";

    return txt.toString();
}

//---

END_BOOMER_NAMESPACE_EX(ui)

