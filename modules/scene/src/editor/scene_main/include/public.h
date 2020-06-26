/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "editor_scene_main_glue.inl"
#include "ui/viewport/include/uiRenderingPanel.h"
#include "scene/common/include/sceneNodePlacement.h"

namespace base
{
    namespace edit
    {
        typedef uint32_t DocumentObjectID;
        typedef Array< DocumentObjectID> DocumentObjectIDSet;
    }
}

namespace ed
{
    namespace world
    {
        class ContentStructure;

        class IContentElement;
        typedef base::RefPtr<IContentElement> ContentElementPtr;

        class ContentGroup;
        typedef base::RefPtr<ContentGroup> ContentGroupPtr;

        class ContentLayer;
        typedef base::RefPtr<ContentLayer> ContentLayerPtr;

        class ContentNode;
        typedef base::RefPtr<ContentNode> ContentNodePtr;

        class ContentPrefabRoot;
        typedef base::RefPtr<ContentPrefabRoot> ContentPrefabRootPtr;

        class ContentWorldRoot;
        typedef base::RefPtr<ContentWorldRoot> ContentWorldRootPtr;

        class SceneDocument;
        typedef base::RefPtr<SceneDocument> SceneDocumentPtr;

        class IHelperObject;
        class IHelperObjectHandler;

        ///--

        /// base content type
        enum class ContentStructureSource : uint8_t
        {
            World,
            Prefab,
        };

        ///--

        class SceneEditorTab;
        class SceneRenderingContainer;
        class SceneRenderingPanel;
        class SceneRenderingPanelWrapper;

        ///--

        /// grid settings
        struct EDITOR_SCENE_MAIN_API SceneGridSettings : public ui::gizmo::GridSettings
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(SceneGridSettings);

        public:
            SceneGridSettings();

            void save(ConfigPath& config) const;
            void load(const ConfigPath& config);
        };

        //---

        /// gizmo settings
        struct EDITOR_SCENE_MAIN_API SceneGizmoSettings : public ui::gizmo::GizmoSettings
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(SceneGizmoSettings);

        public:
            SceneGizmoSettings();

            void save(ConfigPath& config) const;
            void load(const ConfigPath& config);
        };

        //---

        /// selection settings
        struct EDITOR_SCENE_MAIN_API SceneSelectionSettings
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(SceneSelectionSettings);

        public:
            ui::RenderingAreaSelectionMode m_areaMode;
            ui::RenderingPointSelectionMode m_pointMode;
            bool m_selectTransparent;
            bool m_selectWholePrefabs;

            SceneSelectionSettings();

            void save(ConfigPath& config) const;
            void load(const ConfigPath& config);
        };

        //--

        struct PlaceableNodeTemplateInfo;

        struct EDITOR_SCENE_MAIN_API NewNodeSetup
        {
            const PlaceableNodeTemplateInfo* m_template = nullptr;
            ContentNodePtr m_forceParentNode;
            scene::NodeTemplatePlacement m_placement;
            base::res::Ref<base::res::IResource> m_resource;

            scene::NodeTemplatePtr createNode() const;
        };

        //--

        class ISceneEditMode;

        class SceneTempObjectSystem;
        class ISceneTempObject;
        class SceneTempObjectNode;

        //--

    } // world
} // ed