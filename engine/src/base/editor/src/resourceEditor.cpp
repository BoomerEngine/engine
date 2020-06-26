/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editors #]
***/

#include "build.h"
#include "editorService.h"
#include "editorConfig.h"
#include "editorWindow.h"
#include "resourceEditor.h"

#include "base/canvas/include/canvas.h"
#include "base/app/include/launcherPlatform.h"

namespace ed
{

    //---

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IResourceEditorOpener);
    RTTI_END_TYPE();

    IResourceEditorOpener::~IResourceEditorOpener()
    {}

    //---

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ResourceEditor);
    RTTI_END_TYPE();

    ResourceEditor::ResourceEditor(ConfigGroup config, base::StringView<char> title)
        : DockPanel(title)
        , m_config(config)
    {
        closeButton(true);
    }

    ResourceEditor::~ResourceEditor()
    {}

    MainWindow* ResourceEditor::findMainWindow() const
    {
        if (auto window = findParent<MainWindow>())
            return window;

/*        if (auto node = findParent<ui::DockNotebook>())
            if (auto container = node->container())
                return container->findParent<MainWindow>();*/

        return nullptr;
    }

    void ResourceEditor::handleCloseRequest()
    {
        if (auto mainWindow = findMainWindow())
        {
            base::Array<ResourceEditor*> editors;
            editors.pushBack(this);
            mainWindow->requestEditorClose(editors);
        }
    }

    //---

} // editor

