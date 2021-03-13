/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editors #]
***/

#pragma once

#include "engine/ui/include/uiWindow.h"
#include "engine/ui/include/uiDockContainer.h"
#include "engine/ui/include/uiDockPanel.h"
#include "editor/common/include/window.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

///---

/// resource container window is an editor window that contains resource editors
/// there are two major mods of operation: a tabbed window (multiple resources in one window) and singular window (one resource per window - ie. scene)
class EDITOR_ASSETS_API IResourceContainerWindow : public IEditorWindow
{
    RTTI_DECLARE_VIRTUAL_CLASS(IResourceContainerWindow, IEditorWindow);

public:
    IResourceContainerWindow(StringView tag);

    // iterate all resource editors
    virtual void iterateEditors(const std::function<void(ResourceEditor*)>& func, ClassType cls) const = 0;

    // collect resource editors matching given predicate
    virtual void collectEditors(Array<ResourceEditorPtr>& outList, const std::function<bool(ResourceEditor*)>& func, ClassType cls) const = 0;

    // find resource editor
    virtual bool findEditor(ResourceEditorPtr& outRet, const std::function<bool(ResourceEditor*)>& func, ClassType cls) const = 0;

    // select particular editor (if we have tabs)
    virtual bool selectEditor(ResourceEditor* editor) const = 0;
};

///---

/// tabbed resource container
class EDITOR_ASSETS_API TabbedResourceContainerWindow : public IResourceContainerWindow
{
    RTTI_DECLARE_VIRTUAL_CLASS(TabbedResourceContainerWindow, IResourceContainerWindow);

public:
    TabbedResourceContainerWindow(StringView tag);
    virtual ~TabbedResourceContainerWindow();

    //--

    void attachEditor(ResourceEditor* editor, bool focus = true);
    void detachEditor(ResourceEditor* editor);

    //--

    virtual void iterateEditors(const std::function<void(ResourceEditor*)>& func, ClassType cls) const;
    virtual void collectEditors(Array<ResourceEditorPtr>& outList, const std::function<bool(ResourceEditor*)>& func, ClassType cls) const;
    virtual bool findEditor(ResourceEditorPtr& outRet, const std::function<bool(ResourceEditor*)>& func, ClassType cls) const;
    virtual bool selectEditor(ResourceEditor* editor) const;

    //--

protected:
    virtual void handleExternalCloseRequest() override;
    virtual void queryInitialPlacementSetup(ui::WindowInitialPlacementSetup& outSetup) const override;

    virtual bool unnecessary() const override;
    virtual void update() override;
};

//---

END_BOOMER_NAMESPACE_EX(ed)
