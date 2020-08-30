/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once
#include "base/ui/include/uiSimpleTreeModel.h"

namespace ed
{
    //--

    // model for a mesh structure tree
    class ASSETS_SCENE_EDITOR_API SceneStructureTreeModel : public ui::SimpleTreeModel<EditorNodePtr>
    {
    public:
        SceneStructureTreeModel();
        virtual ~SceneStructureTreeModel();

        virtual bool compare(const EditorNodePtr& a, const EditorNodePtr& b, int colIndex) const override final;
        virtual bool filter(const EditorNodePtr& data, const ui::SearchPattern& filter, int colIndex = 0) const override final;
        virtual base::StringBuf displayContent(const EditorNodePtr& data, int colIndex = 0) const override final;
    };

    //--

    // a preview panel for an image
    class ASSETS_SCENE_EDITOR_API SceneStructurePanel : public ui::IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneStructurePanel, ui::IElement);

    public:
        SceneStructurePanel();
        virtual ~SceneStructurePanel();

        void bindScene(EditorScene* scene);

    private:
        EditorScenePtr m_scene;

        ui::TreeViewPtr m_tree;
        base::RefPtr<SceneStructureTreeModel> m_treeModel;
    };

    //--

} // ed