/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\ui #]
***/

#pragma once

namespace ed
{

    ///---

    enum class SceneGizmoMode : uint8_t
    {
        Translation,
        Rotation,
        Scale,
    };

    //---

    enum class SceneGizmoTarget : uint8_t
    {
        WholeHierarchy,
        SelectionOnly,
    };

    //---

    /// grid settings
    struct ASSETS_SCENE_EDITOR_API SceneGridSettings
    {   
        bool positionGridEnabled = true;
        bool rotationGridEnabled = true;
        float positionGridSize = 0.1f;
        float rotationGridSize = 15.0f;

        bool featureSnappingEnabled = false;
        bool rotationSnappingEnabled = false;
        float snappingDistance = 0.2f;

        SceneGridSettings();
    };

    //---

    /// selection settings
    struct ASSETS_SCENE_EDITOR_API SceneSelectionSettings
    {
        bool explorePrefabs = false; // ignore prefab instances and select actual nodes
        bool exploreComponents = false;
        bool allowTransparent = false;

        SceneSelectionSettings();
    };

    //---

    /// gizmo settings
    struct ASSETS_SCENE_EDITOR_API SceneGizmoSettings
    {
        SceneGizmoMode mode = SceneGizmoMode::Translation;
        GizmoSpace space = GizmoSpace::World;
        SceneGizmoTarget target = SceneGizmoTarget::WholeHierarchy;
        bool enableX = true;
        bool enableY = true;
        bool enableZ = true;

        SceneGizmoSettings();
    };

    //--

    class SceneNodeVisualizationHandler;

    DECLARE_UI_EVENT(EVENT_EDIT_MODE_CHANGED);
    DECLARE_UI_EVENT(EVENT_EDIT_MODE_SELECTION_CHANGED);

    //--
    
    // a container for preview panels in various layouts (single, 4 views, etc)
    class ASSETS_SCENE_EDITOR_API ScenePreviewContainer : public ui::IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ScenePreviewContainer, ui::IElement);

    public:
        ScenePreviewContainer(SceneContentStructure* content, ISceneEditMode* initialEditMode = nullptr);
        virtual ~ScenePreviewContainer();

        //--

        // scene content
        INLINE SceneContentStructure* content() { return m_content; }

        // active edit mode
        INLINE const SceneEditModePtr& mode() const { return m_editMode; }

        // all created preview panels
        INLINE const Array<ScenePreviewPanelPtr>& panels() const { return m_panels; }

        // preview world displayed in all the panels
        INLINE const base::world::WorldPtr& world() const { return m_world; }

        //--

        // shared grid settings
        INLINE const SceneGridSettings& gridSettings() const { return m_gridSettings; }

        // shared gizmo settings
        INLINE const SceneGizmoSettings& gizmoSettings() const { return m_gizmoSettings; }

        // shared selection settings
        INLINE const SceneSelectionSettings& selectionSettings() const { return m_selectionSettings; }

        //--

        // switch to edit mode 
        void actionSwitchMode(ISceneEditMode* newMode);

        //--

        // sync visuals with 
        void update();

        //--

        virtual void configSave(const ui::ConfigBlock& block) const;
        virtual void configLoad(const ui::ConfigBlock& block);

        //--

        // change grid settings
        void gridSettings(const SceneGridSettings& settings);

        // change gizmo settings
        void gizmoSettings(const SceneGizmoSettings& settings);

        // change selection settings
        void selectionSettings(const SceneSelectionSettings& settings);

        //--

        // resolve rendering selectable to scene node
        SceneContentNodePtr resolveSelectable(const rendering::scene::Selectable& selectable, bool raw=false) const;

        //--

        // request gizmos to be recreated based on current selection
        void requestRecreatePanelGizmos();

    private:
        Array<ScenePreviewPanelPtr> m_panels;

        world::WorldPtr m_world;

        NativeTimePoint m_lastWorldTick;
        float m_worldTickRatio = 1.0f;

        SceneGridSettings m_gridSettings;
        SceneGizmoSettings m_gizmoSettings;
        SceneSelectionSettings m_selectionSettings;

        SceneEditModePtr m_editMode;

        SceneContentStructure* m_content = nullptr;

        RefPtr<SceneNodeVisualizationHandler> m_visualization;

        //--

        void createWorld();
        void updateWorld();

        void createPanels();
        void destroyPanels();

        void recreatePanelBottomToolbars();
        void recreatePanelGizmos();

        void deactivateEditMode();
        void activateEditMode();

        //--

        void syncNodeState();

        //--
    };

    //--

} // ed