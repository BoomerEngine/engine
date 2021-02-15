/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\view #]
***/

#pragma once

namespace rendering
{
    namespace scene
    {

#pragma pack(push)
#pragma pack(4)

        ///----
        /// Packed GPU data
        /// TODO: move to common header, for now keep in sync

        /// global frame parameters, set once
        struct GPUFrameParameters
        {
            base::Point ViewportSize; // size of the frame viewport
            base::Point ViewportRTSize; // size of the frame render target (may be bigger)
            base::Vector2 InvViewportSize;
            base::Vector2 InvViewportRTSize;

            /*base::Point TargetSize; // size of the target (composition) viewport - may be totally different size
            base::Point TargetRTSize; // size of the composition render target (may be bigger)
            base::Vector2 InvTargetSize;
            base::Vector2 InvTargetRTSize;*/

            uint32_t FrameIndex; // running frame index, every frame new number
            uint32_t MSAASamples;
            uint32_t MaterialFlags;
            uint32_t __PaddingC;

            int32_t ScreenTopY;
            int32_t ScreenBottomY;
            int32_t ScreenDeltaY;
            int32_t __PaddingD;

            uint32_t PseudoRandom[4]; // pseudo random numbers - new every frame

            base::Point DebugMousePos; // position (in Frame pixel coords) of mouse cursor, can be used for debugging shaders
            base::Point DebugMouseClickPos; // position (in Frame pixel coords) of last mouse click

            float GameTime = 0.0f;
            float EngineTime = 0.0f;
            float TimeOfDay = 12.0f;
            float DayNightFraction = 1.0f;
        };

        // camera parameters, as seen by shaders
        struct GPUCameraInfo
        {
            base::Vector4 CameraPosition;
            base::Vector4 CameraForward;
            base::Vector4 CameraUp;
            base::Vector4 CameraRight;

            base::Matrix WorldToScreen;
            base::Matrix ScreenToWorld;
            base::Matrix WorldToScreenNoJitter;
            base::Matrix ScreenToWorldNoJitter;
            base::Matrix WorldToPixelCoord;
            base::Matrix PixelCoordToWorld;

            base::Vector4 LinearizeZ;
            base::Vector4 NearFarPlane;

            base::Vector4 PrevCameraPosition;
            base::Matrix PrevWorldToScreen;
            base::Matrix PrevScreenToWorld;
            base::Matrix PrevWorldToScreenNoJitter;
            base::Matrix PrevScreenToWorldNoJitter;
        };

		//--

        // single cascade slice info
        struct GPUCascadeInfo
        {
            base::Matrix ShadowTransform;
            base::Vector4 ShadowOffsetsX;
            base::Vector4 ShadowOffsetsY;
            base::Vector4 ShadowHalfSizes;
            base::Vector4 ShadowParams[4];
            base::Vector4 ShadowPoissonOffsetAndBias;
            base::Vector4 ShadowTextureSize;
            base::Vector4 ShadowFadeScales;
            base::Vector4 ShadowDepthRanges;
            uint32_t ShadowQuality;
        };

        //--

        // global lighting information
        struct GPULightingInfo
        {
            base::Vector4 globalLightDirection;
            base::Vector4 globalLightColor;
            base::Vector4 globalAmbientColorZenith;
            base::Vector4 globalAmbientColorHorizon;
        };

#pragma pack(pop)

        //--

        // setup of single global shadow cascade
        struct RENDERING_SCENE_API CascadeInfo
        {
            uint8_t cascadeIndex = 0;
            float pixelSize = 1.0f;
            float invPixelSize = 1.0f;
            float edgeFade = 0.0f;
            float filterScale = 0.0f;
            float filterTexelSize = 0.0f;
            float worldSpaceTexelSize = 0.0f;

            float depthBiasConstant = 0.0f;
            float depthBiasSlope = 0.0f;

            //ImageView m_dephtBuffer;

            Camera camera; // culling camera
            Camera jitterCamera; // rendering camera (with jitter)
        };

        //--

        // general setup for all cascades
        struct RENDERING_SCENE_API CascadeData
        {
            uint8_t numCascades = 0;
            CascadeInfo cascades[MAX_CASCADES];
        };

        //--

        // calculate cascade settings that best match given camera 
        // NOTE: texture must be bound first (we need size internally for some computations)
        extern RENDERING_SCENE_API void CalculateCascadeSettings(const base::Vector3& lightDirection, uint32_t resolution, const Camera& viewCamera, const FrameParams_ShadowCascades& setup, CascadeData& outData);

        //--

        // pack frame parameters
        extern RENDERING_SCENE_API void PackFrameParams(GPUFrameParameters& outParams, const FrameRenderer& frame, const FrameCompositionTarget& targets);

        // pack camera setup for GPU
        extern RENDERING_SCENE_API void PackSingleCameraParams(GPUCameraInfo& outInfo, const Camera& camera, const Camera* prevCamera = nullptr);

        // pack lighting setup
        extern RENDERING_SCENE_API void PackLightingParams(GPULightingInfo& outInfo, const FrameParams_GlobalLighting& lighting);

        // pack cascade shadows params
        extern RENDERING_SCENE_API void PackCascadeParams(GPUCascadeInfo& outInfo, const CascadeData& cascades);

        //--

        /// single camera view
        class RENDERING_SCENE_API FrameViewSingleCamera : public base::NoCopy
        {
        public:
            FrameViewSingleCamera(const FrameRenderer& frame, const Camera& camera, const base::Rect& viewport);
            virtual ~FrameViewSingleCamera();

            //--

            INLINE const FrameRenderer& frame() const { return m_frame; }

            INLINE const base::Rect& viewport() const { return m_viewport; }

            INLINE const Camera& visibilityCamera() const { return m_camera; }

            //--

            void bindCamera(command::CommandWriter& cmd);

            //--

        protected:
            const FrameRenderer& m_frame;

            base::Rect m_viewport;
            Camera m_camera;
        };

        //--

    } // scene
} // rendering

