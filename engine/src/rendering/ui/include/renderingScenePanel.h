/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: widgets #]
***/

#pragma once

#include "renderingPanel.h"
#include "cameraController.h"

#include "rendering/scene/include/renderingFrameCamera.h"
#include "rendering/scene/include/renderingSelectable.h"
#include "rendering/scene/include/renderingFrameFilters.h"

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
    class RENDERING_UI_API RenderingPanelSelectionQuery : public base::IReferencable
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
    class RENDERING_UI_API RenderingPanelDepthBufferQuery : public base::IReferencable
    {
    public:
        RenderingPanelDepthBufferQuery(uint32_t width, uint32_t height, const rendering::scene::Camera& camera, const base::image::ImagePtr& depthImage);

        /// get captured resolution
        INLINE uint32_t width() const { return m_width; }
        INLINE uint32_t height() const { return m_height; }

        /// get the camera used to capture the depth buffer
        INLINE const rendering::scene::Camera& camera() const { return m_camera; }

        /// calcuate world position for given pixel
        bool calcWorldPosition(int x, int y, base::AbsolutePosition& outPos) const;

    private:
        uint32_t m_width;
        uint32_t m_height;
        rendering::scene::Camera m_camera;
        base::image::ImagePtr m_depthImage;
    };

    //--

    /// base panel settings
    struct RENDERING_UI_API RenderingScenePanelSettings
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
    class RENDERING_UI_API RenderingScenePanel : public RenderingPanel
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

        // get the panel settings
        INLINE const RenderingScenePanelSettings& panelSettings() const { return m_panelSettings; }

        // change panel settings
        void panelSettings(const RenderingScenePanelSettings& settings);

        //--

        /// setup the camera at specific location
        void setupCamera(const base::Angles& rotation, const base::Vector3& position, const base::Vector3* oribitCenter = nullptr);

        /// setup the perspective camera so it can view given bounds
        void setupCameraAroundBounds(const base::Box& bounds, float distanceFactor = 1.0f, const base::Angles* newRotation = nullptr);

        ///--

        /// calculate exact camera ray for given client pixel position
        bool worldSpaceRayForClientPixelExact(int x, int y, base::AbsolutePosition& outStart, base::Vector3& outDir) const;

        /// calculate camera ray for given client pixel position
        bool worldSpaceRayForClientPixel(int x, int y, base::Vector3& outStart, base::Vector3& outDir) const;

        /// calculate normalized screen position
        base::Vector3 normalizedScreenPosition(int x, int y, float z = 0.0f) const;

        /// calculate client position for given normalized coordinates
        base::Point clientPositionFromNormalizedPosition(const base::Vector3& normalizedPosition) const;

        /// query the position for given normalized screen XY (XY - [0,1], Z-linear depth)
        bool screenToWorld(const base::Vector3* normalizedScreenPos, base::AbsolutePosition* outWorldPosition, uint32_t count) const;

        /// query the screen space position (XY - [0,1], Z-linear depth) for given world position
        bool worldToScreen(const base::AbsolutePosition* worldPosition, base::Vector3* outScreenPosition, uint32_t count) const;

        /// query the client space position (XY - [0,w]x[0xh], Z-linear depth) for given world position
        /// NOTE: this is resolution dependent
        bool worldToClient(const base::AbsolutePosition* worldPosition, base::Vector3* outClientPosition, uint32_t count) const;

        /// calculate viewport scale factor for given world position, keeps stuff constant in size on the screen
        /// NOTE: by default this takes the DPI of the viewport into account for free
        float calculateViewportScaleFactor(const base::AbsolutePosition& worldPosition, bool useDPI = true) const;

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

        virtual void renderScene(const rendering::ImageView& color, const rendering::ImageView& depth, uint32_t width, uint32_t height, const rendering::scene::FrameParams_Capture* capture /*= nullptr*/) override;

        void drawViewAxes(rendering::scene::FrameParams& frame);
        void drawCameraInfo(rendering::scene::FrameParams& frame);
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

        ToolBarPtr m_toolbar;

        const rendering::scene::Camera& cachedCamera() const;
    };

    ///---

} // ui