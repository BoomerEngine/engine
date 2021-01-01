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

            uint8_t msaaLevel = 1; // disabled

            FrameParams_Resolution(uint32_t width_, uint32_t height_);

            float aspect() const; // width/height
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

        // clear color
        struct RENDERING_SCENE_API FrameParams_Clear
        {
            bool clear = true;
            base::Vector4 clearColor;

            FrameParams_Clear();
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
            base::Rect region; // area to capture, can be used with both the image and buffer capture

            DownloadDataSinkPtr sink; // output sink for data download
            
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

        /// global shadow cascades
        struct RENDERING_SCENE_API FrameParams_ShadowCascades
        {
            int numCascades = 0; // number of cascades, 0 to disable
            float baseRange = 2.0f; // base cascade range - this is the range of the 1st cascade
            float baseEdgeFade = 0.05f; // size of fade between cascades
            float baseFilterSize = 16.0f; // in texels of the image

            float baseDepthBiasConstant = 0.0f;
            float baseDepthBiasSlope = 0.0f;

            float filterSizeTexelSizeMul = 0.0f;
            float depthBiasSlopeTexelSizeMul = 0.0f;

            float rangeMul1 = 5.0f; // scale factor between cascade 0 and 1
            float rangeMul2 = 5.0f; // scale factor between cascade 1 and 2
            float rangeMul3 = 5.0f; // scale factor between cascade 2 and 3

            FrameParams_ShadowCascades();
        };

        //---

        /// ambient occlusion params
        struct RENDERING_SCENE_API FrameParams_AmbientOcclusion
        {
            bool enabled = true;
            bool blur = true;
            float intensity = 1.5f;
            float bias = 0.1f;
            float radius = 2.0f;
            float blurSharpness = 40.0f;

            FrameParams_AmbientOcclusion();
        };

        //---

        /// tone mapping type
        enum class FrameToneMappingType : uint8_t
        {
            None, // no tone mapping, data is output directly
            Linear, // simple 1/2.2 gamma
            SimpleReinhard,
            LumabasedReinhard,
            WhitePreservingLumabasedReinhard,
            RomBinDaHouse,
            Filmic,
            Uncharted2,
        };

        /// tone mapping parameters
        struct RENDERING_SCENE_API FrameParams_ToneMapping
        {
            FrameToneMappingType type = FrameToneMappingType::Uncharted2; // best game ever

            FrameParams_ToneMapping();
        };

        //---

        // auto exposure adaptation mode
        struct RENDERING_SCENE_API FrameParams_ExposureAdaptation
        {
            float keyValue = 0.18f;
            float exposureCompensationEV = 0.0f;
            float minLuminanceEV = -2.0f;
            float maxLuminanceEV = 5.0f;
            float adaptationSpeed = 0.1f; // 0-instant adaptation

            FrameParams_ExposureAdaptation();
        };

        //---

        /// color grading parameters
        struct RENDERING_SCENE_API FrameParams_ColorGrading
        {
            float temperatureShift = 0.0f; // no shift (-1 to 1)
            float temeratureTint = 0.0f; // no tint (-1 to 1)

            float contrast = 1.0f;
            float vibrance = 1.0f;
            float saturation = 1.0f;

            base::Vector3 mixerRed = base::Vector3(1, 0, 0);
            base::Vector3 mixerGreen = base::Vector3(0, 1, 0);
            base::Vector3 mixerBlue = base::Vector3(0, 0, 1);

            base::Vector3 toneShadows = base::Vector3(1, 1, 1);
            base::Vector3 toneMidtones = base::Vector3(1, 1, 1);
            base::Vector3 toneHighlights = base::Vector3(1, 1, 1);
            base::Vector2 toneShadowsRange = base::Vector2(0.0f, 0.333f);
            base::Vector2 toneHighlightRange = base::Vector2(0.55f, 1.0f);

            base::Vector3 colorSlope = base::Vector3(1, 1, 1);
            base::Vector3 colorOffset = base::Vector3(0, 0, 0);
            base::Vector3 colorPower = base::Vector3(1, 1, 1);

            base::Vector3 curveShadowGamma = base::Vector3(1, 1, 1);
            base::Vector3 curveMidPoint = base::Vector3(1, 1, 1);
            base::Vector3 curveHighlightScale = base::Vector3(1, 1, 1);

            FrameParams_ColorGrading();
        };

        //---

        /// collected scenes to render
        struct RENDERING_SCENE_API FrameParams_Scenes
        {
			// NOTE: all scenes use the same camera setup

			Scene* backgroundScenePtr = nullptr; // rendered after main scene's opaque objects but before transparencies
            Scene* mainScenePtr = nullptr; // main scene

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

        /// selection outline parameters
        struct RENDERING_SCENE_API FrameParams_SelectionOutline
        {
            base::Color colorFront;
            base::Color colorBack;
            float outlineWidth = 4.0f; // DPI invariant (get's thicker with higher DPI)
            float centerOpacity = 0.5f; // inner highlight opacity

            FrameParams_SelectionOutline();
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
            DebugShadowMask, // visualize shadow mask buffer
            DebugReconstructedViewNormals, // visualize the reconstructed view-space normals
            DebugAmbientOcclusion, // visualize AO buffer
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
            FrameParams_Clear clear;
            FrameParams_Resolution resolution;
            FrameParams_Capture capture;
            FrameParams_GlobalLighting globalLighting;
            FrameParams_Scenes scenes;
            FrameParams_DebugGeometry geometry;
            FrameParams_DebugData debug;
            FrameParams_ToneMapping toneMapping;
            FrameParams_ExposureAdaptation exposureAdaptation;
            FrameParams_ColorGrading colorGrading;
            FrameParams_ShadowCascades cascades;
            FrameParams_AmbientOcclusion ao;
            FrameParams_SelectionOutline selectionOutline;

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