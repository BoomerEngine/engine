/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#pragma once

#include "task.h"
#include "core/object/include/globalEventTable.h"
#include "engine/ui/include/uiElement.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

///---

/// status bar for main window
class EDITOR_COMMON_API MainStatusBar : public ui::IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(MainStatusBar, ui::IElement);

public:
    MainStatusBar();

    void update();

private:
    ui::ProgressBarPtr m_backgroundJobProgress;
    ui::TextLabelPtr m_backgroundJobStatus;

    //--

    GlobalEventTable m_taskEvents;

    EditorTaskPtr m_activeBackgroundJob;

    EditorTaskPtr m_pendingBackgroundJobUIRequest;
    EditorTaskPtr m_pendingBackgroundJobUIOpenedDialogRequest;

    void handleJobStarted(EditorTaskPtr task);
    void handleJobFinished(EditorTaskPtr task);
    void handleJobProgress(EditorTaskProgress progress);

    //--

    void cmdCancelBackgroundJob();
    void cmdShowJobDetails();

};

///---

END_BOOMER_NAMESPACE_EX(ed)

