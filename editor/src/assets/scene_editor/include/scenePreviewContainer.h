/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\ui #]
***/

#pragma once

#include "rendering/ui_viewport/include/cameraController.h"
#include "rendering/ui_viewport/include/renderingScenePanel.h"

namespace ed
{
    //---

    /// viewport layout
    enum class SceneViewportLayout : uint8_t
    {
        Single,
        ClassVertical,
        ClassHorizontal,
        ClassFourWay,

        TwoSmallOneBigLeft,
        TwoSmallOneBigTop,
        TwoSmallOneBigRight,
        TwoSmallOneBigBottom,

        ThreeSmallOneBigLeft,
        ThreeSmallOneBigTop,
        ThreeSmallOneBigRight,
        ThreeSmallOneBigBottom,
    };

    //---

    struct ASSETS_SCENE_EDITOR_API SceneLayoutViewportSettings
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(SceneLayoutViewportSettings);

    public:
        ui::CameraControllerSettings camera;
        ui::RenderingScenePanelSettings panel;

        SceneLayoutViewportSettings();
    };

    struct ASSETS_SCENE_EDITOR_API SceneLayoutSettings
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(SceneLayoutSettings);

    public:
        static const int Splitter_MainVertical = 0;
        static const int Splitter_MainHorizontal = 1;
        static const int Splitter_AdditionalPrimaryLeft = 2;
        static const int Splitter_AdditionalPrimaryTop = 3;
        static const int Splitter_AdditionalPrimaryRight = 4;
        static const int Splitter_AdditionalPrimaryBottom = 5;
        static const int Splitter_AdditionalSecondaryVertical1 = 6;
        static const int Splitter_AdditionalSecondaryVertical2 = 7;
        static const int Splitter_AdditionalSecondaryHorizontal1 = 8;
        static const int Splitter_AdditionalSecondaryHorizontal2 = 9;
        static const int Splitter_MAX = 10;

        static const int Viewport_Main = 0;
        static const int Viewport_Extra1 = 1;
        static const int Viewport_Extra2 = 2;
        static const int Viewport_Extra3 = 3;
        static const int Viewport_MAX = 4;

        //--

        SceneViewportLayout layout = SceneViewportLayout::Single;

        float splitterState[Splitter_MAX];
        SceneLayoutViewportSettings viewportState[Viewport_MAX];

        SceneLayoutSettings();
    };

    //---

    /// grid settings
    struct ASSETS_SCENE_EDITOR_API SceneGridSettings
    {   
        RTTI_DECLARE_NONVIRTUAL_CLASS(SceneGridSettings);

    public:
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
        RTTI_DECLARE_NONVIRTUAL_CLASS(SceneSelectionSettings);

    public:
        bool explorePrefabs = false; // ignore prefab instances and select actual nodes
        bool exploreComponents = false;
        bool allowTransparent = false;

        SceneSelectionSettings();
    };

    //---

    /// gizmo settings
    struct ASSETS_SCENE_EDITOR_API SceneGizmoSettings
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(SceneGizmoSettings);

    public:
        SceneGizmoMode mode = SceneGizmoMode::Translation;
        GizmoSpace space = GizmoSpace::World;
        SceneGizmoTarget target = SceneGizmoTarget::WholeHierarchy;
        bool enableX = true;
        bool enableY = true;
        bool enableZ = true;

        SceneGizmoSettings();
    };

    //--

    /// object creation settings
    struct ASSETS_SCENE_EDITOR_API SceneCreationSettings
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(SceneCreationSettings);

    public:
        SceneContentNodeCreationMode mode = SceneContentNodeCreationMode::WrappedComponent;

        SceneCreationSettings();
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

        // preview world displayed in all the panels
        INLINE const base::world::WorldPtr& world() const { return m_world; }

        //--

        // shared grid settings
        INLINE const SceneGridSettings& gridSettings() const { return m_gridSettings; }

        // shared gizmo settings
        INLINE const SceneGizmoSettings& gizmoSettings() const { return m_gizmoSettings; }

        // shared selection settings
        INLINE const SceneSelectionSettings& selectionSettings() const { return m_selectionSettings; }

        // shared object creation settings
        INLINE const SceneCreationSettings& creationSettings() const { return m_creationSettings; }

        // viewport layout settings
        INLINE const SceneLayoutSettings& layoutSettings() const { return m_layoutSettings; }        

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

        // change creation settings
        void creationSettings(const SceneCreationSettings& settings);

        // change viewport layout mode
        void changeViewportLayout(SceneViewportLayout layout);

        //--

        // resolve rendering selectable to scene node
        SceneContentNodePtr resolveSelectable(const rendering::scene::Selectable& selectable, bool raw=false) const;

        //--

        // request gizmos to be recreated based on current selection
        void requestRecreatePanelGizmos();

        //--

        // fill the parent editor view menu with options to configure this panel
        void fillViewConfigMenu(ui::MenuButtonContainer* menu);

        //-

        // calculate visual bounds of given node(s)
        bool calcNodesVisualBounds(const Array<SceneContentNodePtr>& nodes, Box& outBounds) const;

        // calculate visual bounds of given node(s)
        bool calcNodesVisualBounds(const SceneContentNode* node, Box& outBounds) const;

        // focus on given nodes
        void focusNodes(const Array<SceneContentNodePtr>& nodes);

        // focus on given bounds 
        void focusBounds(const Box& box);

    private:
        world::WorldPtr m_world;

        NativeTimePoint m_lastWorldTick;
        float m_worldTickRatio = 1.0f;

        SceneGridSettings m_gridSettings;
        SceneGizmoSettings m_gizmoSettings;
        SceneSelectionSettings m_selectionSettings;
        SceneCreationSettings m_creationSettings;
        SceneLayoutSettings m_layoutSettings;

        SceneEditModePtr m_editMode;

        SceneContentStructure* m_content = nullptr;

        RefPtr<SceneNodeVisualizationHandler> m_visualization;

        ui::ToolBarPtr m_bottomBar;

        //--

        ScenePreviewPanelPtr m_viewports[SceneLayoutSettings::Viewport_MAX];
        ui::ElementPtr m_splitters[SceneLayoutSettings::Splitter_MAX];

        //--

        void createWorld();
        void updateWorld();

        void createPanels();
        void destroyPanels();
        void captureLayoutSettings(SceneLayoutSettings& outSettings) const;

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