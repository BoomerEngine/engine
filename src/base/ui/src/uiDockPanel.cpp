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

BEGIN_BOOMER_NAMESPACE(ui)

//--

RTTI_BEGIN_TYPE_CLASS(DockPanel);
    RTTI_METADATA(ElementClassNameMetadata).name("DockPanel");
RTTI_END_TYPE();

DockPanel::DockPanel(base::StringView title /*= ""*/, base::StringView id /*= ""*/)
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

void DockPanel::tabTitle(const base::StringBuf& titleString)
{
    if (m_title != titleString)
    {
        m_title = titleString;

        if (auto notebook = findParent<DockNotebook>())
            notebook->updateHeaderButtons();
    }
}

void DockPanel::tabIcon(const base::StringBuf& icon)
{
    if (m_icon != icon)
    {
        m_icon = icon;

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
        ui::MessageBoxSetup setup;
        setup.title("Close locked tab").yes().no().defaultNo().question();

        base::StringBuilder txt;
        txt.appendf("Tab '{}' is locked, close anyway?", m_title);
        setup.message(txt.view());

        if (ui::MessageButton::Yes != ui::ShowMessageBox(this, setup))
            return;
    }

    close();
}

base::StringBuf DockPanel::compileTabTitleString() const
{
    base::StringBuilder txt;

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

END_BOOMER_NAMESPACE(ui)

