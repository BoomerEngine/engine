/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame #]
***/

#pragma once

#include "renderingFrameDebugGeometry.h"
#include "renderingFrameCamera.h"
#include "renderingFrameFilters.h"

namespace rendering
{
    namespace scene
    {
        ///---

        // rendering resolution settings
        struct RENDERING_SCENE_API FrameParams_Resolution
        {
            uint32_t width = 0; // width of the internal rendering targets, may be different than composition size
            uint32_t height = 0; // height of the internal rendering targets, may be different than composition size

            uint32_t finalCompositionWidth = 0; // final, composition width then rendering to final render target
            uint32_t finalCompositionHeight = 0; // final, composition width then rendering to final render target

            uint8_t msaaLevel = 1; // disabled

            FrameParams_Resolution(uint32_t width_, uint32_t height_);
        };

        ///---

        // viewport camera settings
        struct RENDERING_SCENE_API FrameParams_Camera
        {
            Camera camera;
            CameraContext* cameraContext = nullptr;

            FrameParams_Camera(const Camera& camera);
        };

        ///---

        // time parameters
        struct RENDERING_SCENE_API FrameParams_Time
        {
            double gameTime = 0.0; // game time, affected by so-mo, pause, etc, resets after game reload, full screen UI and very raerly when error grows to large
            double engineRealTime = 0.0; // engine time, not effected by pause, may reset from time to time to zero
            float timeOfDay = 12.0f; // 0-24 value representing time of day
            float dayNightFrac = 1.0f; // number representing transition from night (0) to day (1) regimes, blends only during the switch, normally solid 0 or 1

            FrameParams_Time();
        };

        ///---

        /// what to capture from a frame
        enum class FrameCaptureMode : uint8_t
        {
            Disabled, // do not capture anything
            SelectionRect, // capture selection fragments in given area
            DepthRect, // capture depth data in given area
        };

        /// Capture settings for frame rendering, allows to easily extract rendered data back to a CPU side
        /// Used in editor mostly to get the selection and depth data of the scene, used also for thumbnails and such
        struct RENDERING_SCENE_API FrameParams_Capture
        {
            FrameCaptureMode mode; // capture mode
            base::Rect area; // area to capture, can be used with both the image and buffer capture
            DownloadBufferPtr dataBuffer; // output data buffer
            DownloadImagePtr imageBuffer; // output image buffer (NOTE: only direct image captures)

            FrameParams_Capture(); // assigns global (config) defaults
        };

        //---

        /// global lighting parameters
        struct RENDERING_SCENE_API FrameParams_GlobalLighting
        {
            // simplest (directional) settings
            base::Vector3 globalLightDirection; // normal vector towards the global light
            base::Vector3 globalLightColor; // color of the global light, LINEAR
            base::Vector3 globalAmbientColorZenith; // color of the global ambient light, LINEAR
            base::Vector3 globalAmbientColorHorizon; // color of the global light, LINEAR

            FrameParams_GlobalLighting(); // assigns global (config) defaults
        };

        //---

        /// collected scenes to render
        struct RENDERING_SCENE_API FrameParams_Scenes
        {
            struct SceneToDraw
            {
                Scene* scenePtr = nullptr;
            };

            base::InplaceArray<SceneToDraw, 12> scenesToDraw; // collected scenes to draw

            FrameParams_Scenes();
        };

        //---

        /// collected debug geometry to render
        struct RENDERING_SCENE_API FrameParams_DebugGeometry
        {
            DebugGeometry solid;
            DebugGeometry transparent;
            DebugGeometry overlay;
            DebugGeometry screen;

            FrameParams_DebugGeometry();
        };

        //----

        /// collected debug geometry to render
        struct RENDERING_SCENE_API FrameParams_DebugData
        {
            base::StringID materialDebugMode;

            base::Point mouseHoverPixel = base::Point(-1,-1); // viewport coordinates of pixel in the active viewport that is currently under the cursor, set to -1,-1 if no pixel is under the cursor
            base::Point mouseClickedPixel = base::Point(-1, -1); // coordinates of pixel that was clicked
            uint32_t mouseButtons = 0; // current state of mouse buttons
        };

        //---

        /// frame rendering mode
        enum class FrameRenderMode : uint8_t
        {
            Default, // default, full rendering
            WireframeSolid, // render a solid wireframe, objects are filled in with their colors and an triangles/quads have edges rendered in black
            WireframePassThrough, // render a classical "wired" wireframe

            DebugDepth, // visualize frame depth
            DebugLuminance, // visualize frame luminance
            DebugMaterial, // debug material channel - outputs specific material output instead of calculating whole material
        };

        //---

        /// structure that hold all frame rendering parameters for configuring rendering of this frame
        struct RENDERING_SCENE_API FrameParams : public base::NoCopy
        {
            FrameRenderMode mode = FrameRenderMode::Default;
            FilterFlags filters = FilterFlags::DefaultEditor();

            uint32_t index = 0; // frame index, should be increased every frame by the caller

            FrameParams_Time time;
            FrameParams_Camera camera;
            FrameParams_Resolution resolution;
            FrameParams_Capture capture;
            FrameParams_GlobalLighting globalLighting;
            FrameParams_Scenes scenes;
            FrameParams_DebugGeometry geometry;
            FrameParams_DebugData debug;

            //--

            // calculate hack "scaling factor" to keep objects constant size on screen but to render them in world space
            float screenSpaceScalingFactor(const base::Vector3& pos) const;

            // calculate position on screen of given world space position
            bool screenPosition(const base::Vector3& center, base::Vector3& outScreenPos, int margin = 0) const;

            //--

            FrameParams(uint32_t width, uint32_t height, const Camera& camera);
        };

        //---

    } // scene
} // rendering