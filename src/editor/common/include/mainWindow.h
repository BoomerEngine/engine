/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#pragma once

#include "window.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

///---

class MainStatusBar;

/// main editor window 
class EDITOR_COMMON_API MainWindow : public IEditorWindow
{
    RTTI_DECLARE_VIRTUAL_CLASS(MainWindow, IEditorWindow);

public:
    MainWindow();
    virtual ~MainWindow();

    virtual void configLoad(const ui::ConfigBlock& block) override;
    virtual void configSave(const ui::ConfigBlock& block) const override;

    //--

    template< typename T >
    INLINE void iteratePanels(const std::function<void(T*)>& func)
    {
        for (const auto& window : m_panels)
            if (auto t = rtti_cast<T>(window))
                func(t);
    }

    template< typename T >
    INLINE RefPtr<T> findPanel(const std::function<bool(T*)>& func = nullptr)
    {
        for (const auto& window : m_panels)
            if (auto t = rtti_cast<T>(window))
                if (!func || func(t))
                    return t;

        return nullptr;
    }

protected:
    virtual void handleExternalCloseRequest() override;
    virtual void queryInitialPlacementSetup(ui::WindowInitialPlacementSetup& outSetup) const override;

    RefPtr<MainStatusBar> m_statusBar;
    Array<EditorPanelPtr> m_panels;

    void createMenu();
    void createPanels();

    virtual void update() override;
};

///---

END_BOOMER_NAMESPACE_EX(ed)

