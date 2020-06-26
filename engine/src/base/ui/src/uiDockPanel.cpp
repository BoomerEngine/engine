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

namespace ui
{
    //--

    RTTI_BEGIN_TYPE_CLASS(DockPanel);
        RTTI_METADATA(ElementClassNameMetadata).name("DockPanel");
    RTTI_END_TYPE();

    DockPanel::DockPanel(base::StringView<char> title /*= ""*/, base::StringView<char> id /*= ""*/)
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

    void DockPanel::title(const base::StringBuf& titleString)
    {
        if (m_title != titleString)
        {
            m_title = titleString;

            if (auto notebook = findParent<DockNotebook>())
                notebook->updateHeaderButtons();
        }
    }

    void DockPanel::closeButton(bool flag)
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
        close();
    }

    bool DockPanel::handleTemplateProperty(base::StringView<char> name, base::StringView<char> value)
    {
        if (name == "title")
        {
            m_title = base::StringBuf(value);
            m_id = m_title;
            return true;
        }

        return TBaseClass::handleTemplateProperty(name, value);
    }

    //---

} // ui


