/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#pragma once

#include "editorWindow.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

///---

/// base window with resource editors
class EDITOR_ASSETS_API IBaseResourceContainerWindow : public IEditorWindow
{
    RTTI_DECLARE_VIRTUAL_CLASS(IBaseResourceContainerWindow, IEditorWindow);

public:
    IBaseResourceContainerWindow(StringView tag, StringView title);
    virtual ~IBaseResourceContainerWindow();

    virtual void configLoad(const ui::ConfigBlock& block) override;
    virtual void configSave(const ui::ConfigBlock& block) const override;

    bool closeContainedFiles();

    virtual void update() override;
    virtual bool singularEditorOnly() const;

    ResourceEditorPtr activeEditor() const;

    bool hasEditors() const;

    void iterateEditors(const std::function<void(ResourceEditor*)>& enumFunc) const;

    void attachEditor(ResourceEditor* editor, bool focus = true);
    void detachEditor(ResourceEditor* editor);
    bool selectEditor(ResourceEditor* editor);

protected:
    virtual void handleExternalCloseRequest() override;
    virtual void queryInitialPlacementSetup(ui::WindowInitialPlacementSetup& outSetup) const override;
};

///---

/// editor window with tabs containing resource editors
class EDITOR_ASSETS_API FloatingResourceContainerWindow : public IBaseResourceContainerWindow
{
    RTTI_DECLARE_VIRTUAL_CLASS(FloatingResourceContainerWindow, IBaseResourceContainerWindow);

public:
    FloatingResourceContainerWindow(StringView tag);
    virtual ~FloatingResourceContainerWindow();
};

///---

END_BOOMER_NAMESPACE_EX(ed)

