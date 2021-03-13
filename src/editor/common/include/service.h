/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#pragma once

#include "core/app/include/application.h"
#include "core/system/include/task.h"
#include "core/io/include/io.h"
#include "mainWindow.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

DECLARE_GLOBAL_EVENT(EVENT_EDITOR_CONFIG_SAVE)

//---

/// global editor service
/// NOTE: starting this service starts the editor automatically
class EDITOR_COMMON_API EditorService : public IService
{
    RTTI_DECLARE_VIRTUAL_CLASS(EditorService, IService);

public:
    EditorService();
    virtual ~EditorService();

    ///---

    /// editor configuration
    INLINE ui::ConfigBlock& config() const { return *m_configRootBlock; }

    ///---

    /// start the editor, open initial windows
    bool start();

    ///---

    /// force save editor configuration
    void saveConfig();

    ///---

    /// add a generic background runner to the list of runners
    void scheduleTask(IEditorTask* runner, bool openUI = false);

    //--

    // attach top level editor window
    void attachWindow(IEditorWindow* window);

    // detach top level editor window
    void detachWindow(IEditorWindow* window);

    //--

    // iterate over all windows of given type
    template< typename T >
    INLINE void iterateWindows(const std::function<void(T*)>& func)
    {
        for (const auto& window : m_windows)
            if (auto t = rtti_cast<T>(window))
                func(t);
    }

    // iterate over all main panels
    template< typename T >
    INLINE void iteratePanels(const std::function<void(T*)>& func)
    {
        m_mainWindow->iteratePanels<T>(func);
    }

    template< typename T >
    INLINE RefPtr<T> findPanel(const std::function<bool(T*)>& func = nullptr)
    {
        return m_mainWindow->findPanel<T>(func);
    }

private:
    //--

    bool m_stared = false;

    RefPtr<ui::DataStash> m_dataStash;

    UniquePtr<ui::Renderer> m_renderer;

    UniquePtr<ui::NativeWindowRenderer> m_nativeRenderer;

    void updateUI();

    //--

    Array<EditorWindowPtr> m_windows;
    RefPtr<MainWindow> m_mainWindow;

    void updateWindows();
    void saveWindows() const;

    //--

    UniquePtr<ui::ConfigFileStorageDataInterface> m_configStorage;
    UniquePtr<ui::ConfigBlock> m_configRootBlock;

    NativeTimePoint m_nextConfigSave;
    NativeTimePoint m_nextAutoSave;

    StringBuf m_configPath;

    //--

    Mutex m_backgroundJobsLock;
    EditorTaskPtr m_activeBackgroundJob;
    Array<EditorTaskPtr> m_pendingBackgroundJobs;

    void updateBackgroundJobs();

    //--

    virtual bool onInitializeService(const CommandLine& cmdLine) override;
    virtual void onShutdownService() override;
    virtual void onSyncUpdate() override;

    //--

    friend class IEditorWindow;
};

//---

END_BOOMER_NAMESPACE_EX(ed)
