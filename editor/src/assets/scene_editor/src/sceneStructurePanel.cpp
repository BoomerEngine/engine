/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "sceneStructurePanel.h"
#include "base/ui/include/uiTreeView.h"

namespace ed
{
    //--

    SceneStructureTreeModel::SceneStructureTreeModel()
    {}

    SceneStructureTreeModel::~SceneStructureTreeModel()
    {}

    bool SceneStructureTreeModel::compare(const EditorNodePtr& a, const EditorNodePtr& b, int colIndex) const
    {
        return a < b;
    }

    bool SceneStructureTreeModel::filter(const EditorNodePtr& data, const ui::SearchPattern& filter, int colIndex) const
    {
        return true;
    }

    base::StringBuf SceneStructureTreeModel::displayContent(const EditorNodePtr& data, int colIndex /*= 0*/) const
    {
        return "";
    }

    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneStructurePanel);
    RTTI_END_TYPE();

    SceneStructurePanel::SceneStructurePanel()
    {}

    SceneStructurePanel::~SceneStructurePanel()
    {}

    void SceneStructurePanel::bindScene(EditorScene* scene)
    {
        if (m_scene != scene)
        {
            m_scene = AddRef(scene);
        }
    }

    //--
    
} // ed
