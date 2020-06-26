/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\docking #]
***/

#pragma once

#include "uiElement.h"

namespace ui
{

    //---

    class DockNode;

    //---

    /// a dockable panel, his element is never destroyed
    class BASE_UI_API DockPanel : public IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DockPanel, IElement);

    public:
        DockPanel(base::StringView<char> title = "", base::StringView<char> id = "");
        virtual ~DockPanel();

        // get panel ID (for saving layout, panels without ID are not saved)
        INLINE const base::StringBuf& id() const { return m_id; }

        // get panel title text
        INLINE const base::StringBuf& title() const { return m_title; }

        /// should this panel be visible in the layout ?
        INLINE bool visibleInLayout() const { return m_visibleInLayout; }

        /// should we display the close button
        INLINE bool hasCloseButton() const { return m_hasCloseButton; }

        //--

        // change title
        void title(const base::StringBuf& titleString);

        // toggle the close button
        void closeButton(bool flag);

        // remove this panel from thatever tab it's in
        void close();

        // request closing of this dock panel
        virtual void handleCloseRequest();

    private:
        base::StringBuf m_title;
        base::StringBuf m_id;
        bool m_hasCloseButton = true;
        bool m_visibleInLayout = true;

        friend class DockNotebook;
        friend class DockLayoutNode;

        virtual bool handleTemplateProperty(base::StringView<char> name, base::StringView<char> value) override;
    };
    
    //---

} // ui