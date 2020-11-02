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

    class ScenePreviewContainer;
    class SceneStructurePanel;
    class SceneEditMode_Default;

    /// editor for prefabs
    class ASSETS_SCENE_EDITOR_API ScenePrefabEditor : public ResourceEditorNativeFile
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ScenePrefabEditor, ResourceEditorNativeFile);

    public:
        ScenePrefabEditor(ManagedFileNativeResource* file);
        virtual ~ScenePrefabEditor();

        //--

        INLINE ScenePreviewContainer* previewPanel() const { return m_previewContainer; }

        INLINE game::PrefabPtr prefab() const { return base::rtti_cast<game::Prefab>(resource()); }

        //--

        virtual bool initialize() override;
        virtual void fillViewMenu(ui::MenuButtonContainer* menu) override;
        virtual void bindResource(const res::ResourcePtr& resource) override;
        virtual bool save() override;

    private:
        base::RefPtr<ScenePreviewContainer> m_previewContainer;
        base::RefPtr<SceneStructurePanel> m_structurePanel;
        ui::ScrollAreaPtr m_inspectorPanel;

        base::RefPtr<SceneEditMode_Default> m_defaultEditMode;
        
        base::RefPtr<SceneContentStructure> m_content;
        game::WorldPtr m_previewWorld;

        void createInterface();
        void createContentStructure();

        void resetEditMode();
        void refreshEditMode();
    };

} // ed
