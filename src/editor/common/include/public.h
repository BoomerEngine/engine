/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "editor_common_glue.inl"

BEGIN_BOOMER_NAMESPACE_EX(ed)

///---

class IBackgroundTask;
typedef RefPtr<IBackgroundTask> BackgroundTaskPtr;

typedef std::function<void(IProgressTracker& progress)> TLongJobFunc;

class ProgressDialog;

//--

class EditorService;

class IEditorWindow;
typedef RefPtr<IEditorWindow> EditorWindowPtr;

class IEditorPanel;
typedef RefPtr<IEditorPanel> EditorPanelPtr;

class IEditorTask;
typedef RefPtr<IEditorTask> EditorTaskPtr;

class MainWindow;

DECLARE_GLOBAL_EVENT(EVENT_EDITOR_TASK_STARTED, EditorTaskPtr);
DECLARE_GLOBAL_EVENT(EVENT_EDITOR_TASK_FINISHED, EditorTaskPtr);
DECLARE_GLOBAL_EVENT(EVENT_EDITOR_TASK_PROGRESS, EditorTaskProgress);

//--

END_BOOMER_NAMESPACE_EX(ed)


