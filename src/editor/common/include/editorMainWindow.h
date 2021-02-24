/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#pragma once

#include "editorResourceContainerWindow.h"

BEGIN_BOOMER_NAMESPACE(ed)

///---

/// status bar for main window
class EDITOR_COMMON_API MainWindowStatusBar : public ui::IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(MainWindowStatusBar, ui::IElement);

public:
    MainWindowStatusBar();

    void pushBackgroundJobToHistory(IBackgroundTask* job, BackgroundTaskStatus status);
    void updateBackgroundJobStatus(IBackgroundTask* job, uint64_t count, uint64_t total, StringView text, bool hasErrors);
    void resetBackgroundJobStatus();

private:
    BackgroundTaskPtr m_activeBackgroundJob;
    ui::ProgressBarPtr m_backgroundJobProgress;
    ui::TextLabelPtr m_backgroundJobStatus;

    void cmdCancelBackgroundJob();
    void cmdShowJobDetails();
};

///---

/// main editor window 
class EDITOR_COMMON_API MainWindow : public IBaseResourceContainerWindow
{
    RTTI_DECLARE_VIRTUAL_CLASS(MainWindow, IBaseResourceContainerWindow);

public:
    MainWindow();
    virtual ~MainWindow();

    INLINE MainWindowStatusBar* statusBar() const { return m_statusBar; }

    virtual void configLoad(const ui::ConfigBlock& block) override;
    virtual void configSave(const ui::ConfigBlock& block) const override;

protected:
    virtual bool singularEditorOnly() const override;
    virtual void handleExternalCloseRequest() override;
    virtual void queryInitialPlacementSetup(ui::WindowInitialPlacementSetup& outSetup) const override;

    RefPtr<MainWindowStatusBar> m_statusBar;
};

///---

END_BOOMER_NAMESPACE(ed)

