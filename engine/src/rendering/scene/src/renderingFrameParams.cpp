/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\debug #]
***/

#include "build.h"
#include "renderingFrameParams.h"

namespace rendering
{
    namespace scene
    {
        //--

        base::ConfigProperty<float> cvFrameResolutionScaleFactor("Rendering.Viewport", "ResolutionScaleFactor", 1.0f);
        base::ConfigProperty<int> cvFrameDefaultMSAALevel("Rendering.Viewport", "DefaultMSAA", 0);

        FrameParams_Resolution::FrameParams_Resolution(uint32_t width_, uint32_t height_)
            : finalCompositionWidth(width_)
            , finalCompositionHeight(height_)
            , width(width_)
            , height(height_)
        {
            width = (uint32_t)std::clamp<float>(std::roundf(cvFrameResolutionScaleFactor.get() * width_), 0.0f, 32738.0f);
            height = (uint32_t)std::clamp<float>(std::roundf(cvFrameResolutionScaleFactor.get() * height_), 0.0f, 32738.0f);

            if (cvFrameDefaultMSAALevel.get() > 0)
                msaaLevel = cvFrameDefaultMSAALevel.get();

            if (msaaLevel < 1)
                msaaLevel = 1;
            if (msaaLevel > 16)
                msaaLevel = 16;

            msaaLevel = 4;
        }

        //--

        FrameParams_Camera::FrameParams_Camera(const Camera& camera_)
            : camera(camera_)
        {
        }

        //--

        FrameParams_Time::FrameParams_Time()
        {}

        //--

        FrameParams_Capture::FrameParams_Capture()
            : mode(FrameCaptureMode::Disabled)
        {}

        //--

        base::ConfigProperty<float> cvFrameDefaultGlobalLightPitch("Rendering.GlobalLighting", "DefaultLightPitch", 30.0f);
        base::ConfigProperty<float> cvFrameDefaultGlobalLightYaw("Rendering.GlobalLighting", "DefaultLightYaw", 60.0f);
        base::ConfigProperty<base::Color> cvFrameDefaultGlobalLightColor("Rendering.GlobalLighting", "DefaultLightColor", base::Color(255,255,230));
        base::ConfigProperty<base::Color> cvFrameDefaultAmbientHorizonColor("Rendering.GlobalLighting", "DefaultAmbientHorizonColor", base::Color(110, 110, 130));
        base::ConfigProperty<base::Color> cvFrameDefaultAmbientZenithColor("Rendering.GlobalLighting", "DefaultAmbientZenithColor", base::Color(50, 50, 70));

        FrameParams_GlobalLighting::FrameParams_GlobalLighting()
        {
            globalLightDirection = base::Angles(cvFrameDefaultGlobalLightPitch.get(), cvFrameDefaultGlobalLightYaw.get(), 0.0f).forward();
            if (globalLightDirection.z)
                globalLightDirection.z = -globalLightDirection.z;

            globalLightColor = cvFrameDefaultGlobalLightColor.get().toVectorSRGB().xyz();
            globalAmbientColorZenith = cvFrameDefaultAmbientZenithColor.get().toVectorSRGB().xyz();
            globalAmbientColorHorizon = cvFrameDefaultAmbientHorizonColor.get().toVectorSRGB().xyz();
        }

        //--

        FrameParams_Scenes::FrameParams_Scenes()
        {}

        //--

        FrameParams_DebugGeometry::FrameParams_DebugGeometry()
            : solid(DebugGeometryLayer::SceneSolid)
            , transparent(DebugGeometryLayer::SceneTransparent)
            , overlay(DebugGeometryLayer::Overlay)
            , screen(DebugGeometryLayer::Screen)
        {}

        //--

        FrameParams::FrameParams(uint32_t width, uint32_t height, const Camera& camera_)
            : camera(camera_)
            , resolution(width, height)
        {}

        float FrameParams::screenSpaceScalingFactor(const base::Vector3& pos) const
        {
            return camera.camera.calcScreenSpaceScalingFactor(pos, resolution.width, resolution.height);
        }

        bool FrameParams::screenPosition(const base::Vector3& center, base::Vector3& outScreenPos, int margin /*= 0*/) const
        {
            base::Vector3 normalizedScreenPos;
            if (camera.camera.projectWorldToScreen(center, normalizedScreenPos))
            {
                auto sx = normalizedScreenPos.x * (float)resolution.width;
                auto sy = normalizedScreenPos.y * (float)resolution.height;
                auto marginLeft = -(float)margin;
                auto marginTop = -(float)margin;
                auto marginRight = (float)(resolution.width + margin);
                auto marginBottom = (float)(resolution.height + margin);
                if (sx >= marginLeft && sy >= marginTop && sx <= marginRight && sy <= marginBottom)
                {
                    outScreenPos.x = sx;
                    outScreenPos.y = sy;
                    outScreenPos.z = normalizedScreenPos.z;
                    return true;
                }
            }

            return false;
        }

        //--

    } // rendering
} // scene
