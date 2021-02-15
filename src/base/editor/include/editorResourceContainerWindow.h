/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#pragma once

#include "base/ui/include/uiWindow.h"
#include "base/ui/include/uiDockContainer.h"
#include "base/ui/include/uiDockPanel.h"

namespace ed
{
    ///---

    /// base window with resource editors
    class BASE_EDITOR_API IBaseResourceContainerWindow : public ui::Window
    {
        RTTI_DECLARE_VIRTUAL_CLASS(IBaseResourceContainerWindow, ui::Window);

    public:
        IBaseResourceContainerWindow(StringView tag, StringView title);
        virtual ~IBaseResourceContainerWindow();

        INLINE const StringBuf& tag() const { return m_tag; }

        ui::DockLayoutNode& layout();

        virtual void configLoad(const ui::ConfigBlock& block) override;
        virtual void configSave(const ui::ConfigBlock& block) const override;

        bool closeContainedFiles();

        virtual bool singularEditorOnly() const;

        ResourceEditorPtr activeEditor() const;

        bool hasEditors() const;

        void iterateAllEditors(const std::function<void(ResourceEditor*)>& enumFunc) const;
        bool iterateEditors(const std::function<bool(ResourceEditor*)>& enumFunc, ui::DockPanelIterationMode mode = ui::DockPanelIterationMode::All) const;

        void attachEditor(ResourceEditor* editor, bool focus = true);
        void detachEditor(ResourceEditor* editor);
        bool selectEditor(ResourceEditor* editor);

    protected:
        virtual void handleExternalCloseRequest() override;
        virtual void queryInitialPlacementSetup(ui::WindowInitialPlacementSetup& outSetup) const override;

        ui::DockContainerPtr m_dockArea;

        StringBuf m_tag;
    };

    ///---

    /// editor window with tabs containing resource editors
    class BASE_EDITOR_API FloatingResourceContainerWindow : public IBaseResourceContainerWindow
    {
        RTTI_DECLARE_VIRTUAL_CLASS(FloatingResourceContainerWindow, IBaseResourceContainerWindow);

    public:
        FloatingResourceContainerWindow(StringView tag);
        virtual ~FloatingResourceContainerWindow();
    };

    ///---

} // editor

