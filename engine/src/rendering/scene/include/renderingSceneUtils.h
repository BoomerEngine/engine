/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
***/

#pragma once

namespace rendering
{
    namespace scene
    {

        ///--

        // camera parameters, as seen by shaders
        #pragma pack(push)
        #pragma pack(4)
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
        #pragma pack(pop)

        /// scene parameters
#pragma pack(push)
#pragma pack(4)
        struct GPUSceneInfo
        {
            base::Matrix SceneToWorld;
            base::Matrix WorldToScene;
        };
#pragma pack(pop)

        /// general global parameters
#pragma pack(push)
#pragma pack(4)
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
#pragma pack(pop)

        ///--

        // compute camera setup
        extern RENDERING_SCENE_API void CalcGPUSingleCamera(GPUCameraInfo& outInfo, const Camera& camera, const Camera* prevCamera = nullptr);

        ///---

        // bind single camera data
        //extern RENDERING_SCENE_API void BindSingleCamera(command::CommandWriter& cmd, const Camera& camera, const Camera* prevFrameCamera = nullptr);

        ///--

    } // scene
} // rendering

