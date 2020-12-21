/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: widgets #]
***/

#pragma once

#include "cameraController.h"

#include "rendering/scene/include/renderingFrameCamera.h"
#include "rendering/scene/include/renderingSelectable.h"
#include "rendering/scene/include/renderingFrameFilters.h"
#include "rendering/ui_host/include/renderingPanel.h"

namespace ui
{
    //--

    /// selection mode for point selection
    enum class PointSelectionMode : uint8_t
    {
        // select exactly the object under the pixel
        Exact,

        // select the object with the largest area in the selection region
        LargestArea,

        // select the object with the smallest area in the selection region
        SmallestArea,

        // select the object that is the closest of all in the area
        Closest,
    };

    /// selection mode for area selection
    enum class AreaSelectionMode : uint8_t
    {
        // select everything
        Everything,

        // exclude objects at the border
        ExcludeBorder,

        // choose the object with largest area
        LargestArea,

        // choose the object with smallest area
        SmallestArea,

        // choose the closest object
        Closest,
    };

    /// selection query
    class RENDERING_UI_VIEWPORT_API RenderingPanelSelectionQuery : public base::IReferencable
    {
    public:
        RenderingPanelSelectionQuery(const base::Rect& area, base::Array<rendering::scene::EncodedSelectable> entries);

        /// get the selection area
        INLINE const base::Rect& area() const { return m_area; }

        /// get the captured selection entries
        INLINE const base::Array<rendering::scene::EncodedSelectable>& entries() const { return m_entries; }

        /// extract selection list for given point, selects best selectable at given position
        bool extractPointSelection(PointSelectionMode mode, const base::Point& point, rendering::scene::Selectable& outSelectable) const;

        /// extract selection for given area
        bool extractAreaSelection(AreaSelectionMode mode, const base::Rect& rect, base::HashSet<rendering::scene::Selectable>& outSelectables) const;

    private:
        friend class RenderingPanel;

        base::Rect m_area; // the selection area to extract
        base::Array<rendering::scene::EncodedSelectable> m_entries;
    };

    ///---

    /// depth buffer query
    class RENDERING_UI_VIEWPORT_API RenderingPanelDepthBufferQuery : public base::IReferencable
    {
    public:
        RenderingPanelDepthBufferQuery(uint32_t width, uint32_t height, const rendering::scene::Camera& camera, const base::image::ImagePtr& depthImage);

        /// get captured resolution
        INLINE uint32_t width() const { return m_width; }
        INLINE uint32_t height() const { return m_height; }

        /// get the camera used to capture the depth buffer
        INLINE const rendering::scene::Camera& camera() const { return m_camera; }

        /// calculate world position for given pixel
        bool calcWorldPosition(int x, int y, base::AbsolutePosition& outPos) const;

    private:
        uint32_t m_width;
        uint32_t m_height;
        rendering::scene::Camera m_camera;
        base::image::ImagePtr m_depthImage;
    };

    //--

    /// base panel settings
    struct RENDERING_UI_VIEWPORT_API RenderingScenePanelSettings
    {
        float cameraSpeedFactor = 1.0f;
        bool cameraForceOrbit = false;

        bool paused = false;
        float timeDeltaScale = 1.0f;

        AreaSelectionMode areaSelectionMode = AreaSelectionMode::ExcludeBorder;
        PointSelectionMode pointSelectionMode = PointSelectionMode::Closest;

        bool drawInternalGrid = true;
        bool drawInternalWorldAxis = true;
        bool drawInternalCameraAxis = true;
        bool drawInternalCameraData = true;

        //--

        void configSave(const ConfigBlock& block) const;
        void configLoad(const ConfigBlock& block);
    };

    //--

    /// ui widget capable of rendering a 3D scene with a camera and stuff
    class RENDERING_UI_VIEWPORT_API RenderingScenePanel : public RenderingPanel
    {
        RTTI_DECLARE_VIRTUAL_CLASS(RenderingScenePanel, RenderingPanel);

    public:
        RenderingScenePanel();
        virtual ~RenderingScenePanel();

        //--

        // local filter flags for this viewport
        INLINE rendering::scene::FilterFlags& filterFlags() { return m_filterFlags; }
        INLINE const rendering::scene::FilterFlags& filterFlags() const { return m_filterFlags; }

        // get the overlay toolbar
        INLINE const ToolBarPtr& toolbar() const { return m_toolbar; }

        // get the bottom overlay toolbar
        INLINE const ToolBarPtr& bottomToolbar() const { return m_bottomToolbar; }

        // get the panel settings
        INLINE const RenderingScenePanelSettings& panelSettings() const { return m_panelSettings; }

        // change panel settings
        void panelSettings(const RenderingScenePanelSettings& settings);

        //--

        /// setup the camera at specific location
        void setupCamera(const base::Angles& rotation, const base::Vector3& position, const base::Vector3* oribitCenter = nullptr);

        /// setup the perspective camera so it can view given bounds
        void setupCameraAroundBounds(const base::Box& bounds, float distanceFactor = 1.0f, const base::Angles* newRotation = nullptr);

        /// get computed camera
        const rendering::scene::Camera& cachedCamera() const;

        ///--

        /// compute bounds of the content in the view
        virtual bool computeContentBounds(base::Box& outBox) const;

    protected:
        virtual bool handleKeyEvent(const base::input::KeyEvent& evt) override;
        virtual InputActionPtr handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt) override;
        virtual bool handleMouseMovement(const base::input::MouseMovementEvent& evt) override;
        virtual bool handleMouseWheel(const base::input::MouseMovementEvent& evt, float delta) override;
        virtual void handleFocusLost() override;

        virtual void handleUpdate(float dt);
        virtual void handleCamera(rendering::scene::CameraSetup& outCamera);
        virtual void handleRender(rendering::scene::FrameParams& frame);
        virtual void handlePointSelection(bool ctrl, bool shift, const base::Point& clientPosition);
        virtual void handleAreaSelection(bool ctrl, bool shift, const base::Rect& clientRect);
        virtual void handlePointSelection(bool ctrl, bool shift, const base::Point& clientPosition, const base::Array<rendering::scene::Selectable>& selectables);
        virtual void handleAreaSelection(bool ctrl, bool shift, const base::Rect& clientRect, const base::Array<rendering::scene::Selectable>& selectables);

		virtual void renderContent(const ViewportParams& viewport) override;

        void drawViewAxes(uint32_t width, uint32_t height, rendering::scene::FrameParams& frame);
        void drawCameraInfo(uint32_t width, uint32_t height, rendering::scene::FrameParams& frame);
        void drawGrid(rendering::scene::FrameParams& frame);

        void rotateGlobalLight(float deltaPitch, float deltaYaw);

        base::RefPtr<RenderingPanelSelectionQuery> querySelection(const base::Rect& area);
        base::RefPtr<RenderingPanelDepthBufferQuery> queryDepth();

        bool queryWorldPositionUnderCursor(const base::Point& localPoint, base::AbsolutePosition& outPosition);

        //-

        RenderingScenePanelSettings m_panelSettings;

    private:
        Timer m_updateTimer;
        base::NativeTimePoint m_lastUpdateTime;

        CameraController m_cameraController;

        double m_engineTimeCounter = 0.0;
        double m_gameTimeCounter = 0.0;

        uint32_t m_frameIndex = 0;

        rendering::scene::FilterFlags m_filterFlags;

        mutable rendering::scene::Camera m_cachedCamera;
        mutable uint32_t m_cachedCameraTimestamp;

        rendering::scene::CameraContextPtr m_cameraContext;

        base::RefWeakPtr<ui::IInputAction> m_renderInputAction;

		rendering::DownloadAreaObjectPtr m_downloadArea;

        ToolBarPtr m_toolbar;
        ToolBarPtr m_bottomToolbar;
    };

    ///---

} // ui