/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "base/editor/include/resourceEditorNativeFile.h"
#include "game/world/include/worldPrefab.h"

namespace ed
{

    class EditorScenePreviewContainer;
    class SceneStructurePanel;

    /// editor for prefabs
    class ASSETS_SCENE_EDITOR_API ScenePrefabEditor : public ResourceEditorNativeFile
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ScenePrefabEditor, ResourceEditorNativeFile);

    public:
        ScenePrefabEditor(ManagedFileNativeResource* file);
        virtual ~ScenePrefabEditor();

        //--

        INLINE EditorScenePreviewContainer* previewPanel() const { return m_previewContainer; }

        INLINE game::PrefabPtr prefab() const { return base::rtti_cast<game::Prefab>(resource()); }

        //--

        virtual void bindResource(const res::ResourcePtr& resource) override;
        virtual void fillViewMenu(ui::MenuButtonContainer* menu) override;

    private:
        base::RefPtr<EditorScenePreviewContainer> m_previewContainer;
        base::RefPtr<SceneStructurePanel> m_structurePanel;

        void createInterface();
    };

} // ed
