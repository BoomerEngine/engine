/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
***/

#include "build.h"
#include "renderingSceneUtils.h"
#include "renderingFrameCamera.h"

namespace rendering
{
    namespace scene
    {

        //---

        void CalcGPUSingleCamera(GPUCameraInfo& outInfo, const Camera& camera, const Camera* prevCamera /*= nullptr*/)
        {
            // get the camera for given layer
            outInfo.CameraForward = camera.directionForward();
            outInfo.CameraUp = camera.directionUp();
            outInfo.CameraRight = camera.directionRight();
            outInfo.CameraPosition = camera.position();
            outInfo.NearFarPlane.x = camera.nearPlane();
            outInfo.NearFarPlane.y = camera.farPlane();
            outInfo.NearFarPlane.z = camera.farPlane() - camera.nearPlane();
            outInfo.NearFarPlane.w = 1.0f / outInfo.NearFarPlane.z;
            outInfo.WorldToScreen = camera.worldToScreen().transposed(); // our matrices have translation ordered for easier dot product on CPU, for GPU we order it for compliance
            outInfo.ScreenToWorld = camera.screenToWorld().transposed();
            outInfo.LinearizeZ.x = camera.viewToScreen().m[2][2];
            outInfo.LinearizeZ.y = camera.viewToScreen().m[2][3];

            // create a camera without jitter
            {
                // prepare camera
                auto cameraSetup = camera.setup();
                cameraSetup.subPixelOffsetX = 0.0f;
                cameraSetup.subPixelOffsetY = 0.0f;

                // prepare camera without jitter
                Camera noJitterCamera;
                noJitterCamera.setup(cameraSetup);
                outInfo.WorldToScreenNoJitter = noJitterCamera.worldToScreen().transposed();
                outInfo.ScreenToWorldNoJitter = noJitterCamera.screenToWorld().transposed();
            }

            /*// magical matrix than converts from pixel coordinates to world space (assuming the depth is known)
            base::Matrix pixelCoordToScreen, screenToPixelCoord;
            pixelCoordToScreen.identity();
            pixelCoordToScreen.m[0][0] = 2.0f / (float)view.viewportWidth();
            pixelCoordToScreen.m[1][1] = 2.0f / (float)view.viewportHeight();
            pixelCoordToScreen.m[0][3] = -1.0f;
            pixelCoordToScreen.m[1][3] = -1.0f;
            screenToPixelCoord.identity();
            screenToPixelCoord.m[0][0] = (float)view.viewportWidth() / 2.0f;
            screenToPixelCoord.m[1][1] = (float)view.viewportHeight() / 2.0f;
            screenToPixelCoord.m[0][3] = (view.viewportWidth() + 1.0f) / 2.0f; // note half pixel offset added
            screenToPixelCoord.m[1][3] = (view.viewportHeight() + 1.0f) / 2.0f; // note half pixel offset added

            // compute the compounded viewport dependent matrices
            m_consts.WorldToPixelCoord = camera.worldToScreen() * screenToPixelCoord;
            m_consts.PixelCoordToWorld = pixelCoordToScreen * camera.screenToWorld();*/

            // setup previous camera
            if (prevCamera)
            {
                outInfo.PrevCameraPosition = prevCamera->position();
                outInfo.PrevCameraPosition.w = 1.0f; // valid
                outInfo.PrevWorldToScreen = prevCamera->worldToScreen().transposed();
                outInfo.PrevScreenToWorld = prevCamera->screenToWorld().transposed();
                outInfo.PrevWorldToScreenNoJitter = outInfo.PrevWorldToScreen;// prevCameraData.PrevWorldToScreenNoJitter;
                outInfo.PrevScreenToWorldNoJitter = outInfo.PrevWorldToScreen;// prevCameraData.PrevScreenToWorldNoJitter;
            }
            else
            {

            }
        }
        
        //---

    } // scene
} // rendering