/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame #]
***/

#include "build.h"
#include "renderingFrameParams.h"

BEGIN_BOOMER_NAMESPACE(rendering::scene)

        //--

        base::ConfigProperty<float> cvFrameResolutionScaleFactor("Rendering.Viewport", "ResolutionScaleFactor", 1.0f);
        base::ConfigProperty<int> cvFrameDefaultMSAALevel("Rendering.Viewport", "DefaultMSAA", 0);

        FrameParams_Resolution::FrameParams_Resolution(uint32_t width_, uint32_t height_)
            : width(width_)
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
        }

        float FrameParams_Resolution::aspect() const
        {
            return height ? width / (float)height : 1.0f;
        }

        //--

        RTTI_BEGIN_TYPE_ENUM(FrameRenderMode);
            RTTI_ENUM_OPTION(Default);
            RTTI_ENUM_OPTION(WireframeSolid);
            RTTI_ENUM_OPTION(WireframePassThrough);
            RTTI_ENUM_OPTION(DebugDepth);
            RTTI_ENUM_OPTION(DebugLuminance);
            RTTI_ENUM_OPTION(DebugShadowMask);
            RTTI_ENUM_OPTION(DebugAmbientOcclusion);
            RTTI_ENUM_OPTION(DebugReconstructedViewNormals);
            RTTI_ENUM_OPTION(DebugMaterial);
        RTTI_END_TYPE();

        //--

        FrameParams_Camera::FrameParams_Camera(const Camera& camera_)
            : camera(camera_)
        {
        }

        //--

        base::ConfigProperty<base::Color> cvFrameDefaultClearColor("Rendering", "DefaultClearcolor", base::Color(50,50,50,255));

        FrameParams_Clear::FrameParams_Clear()
        {
            clearColor = cvFrameDefaultClearColor.get().toVectorLinear();
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
        base::ConfigProperty<float> cvFrameDefaultGlobalLightBrightness("Rendering.GlobalLighting", "DefaultLightBrightness", 10.0f);
        base::ConfigProperty<base::Color> cvFrameDefaultGlobalLightColor("Rendering.GlobalLighting", "DefaultLightColor", base::Color(255,255,231));
        base::ConfigProperty<base::Color> cvFrameDefaultAmbientHorizonColor("Rendering.GlobalLighting", "DefaultAmbientHorizonColor", base::Color(10, 10, 10));
        base::ConfigProperty<base::Color> cvFrameDefaultAmbientZenithColor("Rendering.GlobalLighting", "DefaultAmbientZenithColor", base::Color(12, 12, 14));

        FrameParams_GlobalLighting::FrameParams_GlobalLighting()
        {
            globalLightDirection = base::Angles(cvFrameDefaultGlobalLightPitch.get(), cvFrameDefaultGlobalLightYaw.get(), 0.0f).forward();
            if (globalLightDirection.z)
                globalLightDirection.z = -globalLightDirection.z;

            globalLightColor = cvFrameDefaultGlobalLightColor.get().toVectorLinear().xyz() * cvFrameDefaultGlobalLightBrightness.get();
            globalAmbientColorZenith = cvFrameDefaultAmbientZenithColor.get().toVectorLinear().xyz();
            globalAmbientColorHorizon = cvFrameDefaultAmbientHorizonColor.get().toVectorLinear().xyz();
        }

        //--

        RTTI_BEGIN_TYPE_ENUM(FrameToneMappingType);
            RTTI_ENUM_OPTION(None);
            RTTI_ENUM_OPTION(Linear);
            RTTI_ENUM_OPTION(SimpleReinhard);
            RTTI_ENUM_OPTION(LumabasedReinhard);
            RTTI_ENUM_OPTION(WhitePreservingLumabasedReinhard);
            RTTI_ENUM_OPTION(RomBinDaHouse);
            RTTI_ENUM_OPTION(Filmic);
            RTTI_ENUM_OPTION(Uncharted2);
        RTTI_END_TYPE();

        FrameParams_ToneMapping::FrameParams_ToneMapping()
        {}

        //--

        FrameParams_ExposureAdaptation::FrameParams_ExposureAdaptation()
        {}

        //--

        FrameParams_ColorGrading::FrameParams_ColorGrading()
        {}

        //--

        FrameParams_Scenes::FrameParams_Scenes()
        {}

        //--

        base::ConfigProperty<bool> cvAmbientOcclusionEnabled("Rendering.AmbientOcclusion", "Enabled", true);
        base::ConfigProperty<bool> cvAmbientOcclusionBlurEnabled("Rendering.AmbientOcclusion", "BlurEnabled", true);
        base::ConfigProperty<float> cvAmbientOcclusionIntensity("Rendering.AmbientOcclusion", "Intensity", 1.5f);
        base::ConfigProperty<float> cvAmbientOcclusionBias("Rendering.AmbientOcclusion", "Bias", 0.1f);
        base::ConfigProperty<float> cvAmbientOcclusionRadius("Rendering.AmbientOcclusion", "Radius", 2.0f);
        base::ConfigProperty<float> cvAmbientOcclusionBlurSharpness("Rendering.AmbientOcclusion", "BlurSharpness", 40.0f);

        FrameParams_AmbientOcclusion::FrameParams_AmbientOcclusion()
        {
            enabled = cvAmbientOcclusionEnabled.get();
            blur = cvAmbientOcclusionBlurEnabled.get();
            intensity = cvAmbientOcclusionIntensity.get();
            bias = cvAmbientOcclusionBias.get();
            radius = cvAmbientOcclusionRadius.get();
            blurSharpness = cvAmbientOcclusionBlurSharpness.get();
        }

        //--

        FrameParams_DebugGeometry::FrameParams_DebugGeometry()
        {}

        //--

        static base::ConfigProperty<base::Color> cvSelectionOutlineBackColor("Rendering.Selection", "OutlineColorBack", base::Color(200, 200, 30));
        //static base::ConfigProperty<base::Color> cvSelectionOutlineFrontColor("Rendering.Cascades", "OutlineColorFront", base::Color(30, 30, 200));
        static base::ConfigProperty<base::Color> cvSelectionOutlineFrontColor("Rendering.Selection", "OutlineColorFront", base::Color(200, 200, 30));
        static base::ConfigProperty<float> cvSelectionOutlineWidth("Rendering.Cascades", "OutlineWidth", 3.0f);
        static base::ConfigProperty<float> cvSelectionOutlineCenterOpacity("Rendering.Cascades", "OutlineCenterOpacity", 0.6f);

        FrameParams_SelectionOutline::FrameParams_SelectionOutline()
        {
            colorFront = cvSelectionOutlineFrontColor.get();
            colorBack = cvSelectionOutlineBackColor.get();
            outlineWidth = cvSelectionOutlineWidth.get();
            centerOpacity = cvSelectionOutlineCenterOpacity.get();
        }

        //--

        static base::ConfigProperty<float> cvCascadesBaseEdgeFade("Rendering.Cascades", "BaseEdgeFade", 0.05f);
        static base::ConfigProperty<float> cvCascadesBaseFilterSize("Rendering.Cascades", "BaseFilterSize", 8.0f);
        static base::ConfigProperty<float> cvCascadesBaseRange("Rendering.Cascades", "BaseRange", 4.0f);
        static base::ConfigProperty<float> cvCascadesRangeMul1("Rendering.Cascades", "RangeMul1", 3.0f); // 12
        static base::ConfigProperty<float> cvCascadesRangeMul2("Rendering.Cascades", "RangeMul2", 3.0f); // 36
        static base::ConfigProperty<float> cvCascadesRangeMul3("Rendering.Cascades", "RangeMul3", 3.0f); // 100m
        static base::ConfigProperty<float> cvCascadesDepthBiasConstant("Rendering.Cascades", "DepthConstant", 400.0f);
        static base::ConfigProperty<float> cvCascadesDepthBiasSlope("Rendering.Cascades", "DepthSlope", 2.0f);
        static base::ConfigProperty<float> cvCascadesDepthBiasTexelSizeMul("Rendering.Cascades", "DepthSlopeTexelSizeMul", 0.2f);
        static base::ConfigProperty<float> cvCascadesFilterSizeTexelSizeMul("Rendering.Cascades", "FilterSizeTexelSizeMul", -1.0f);

        FrameParams_ShadowCascades::FrameParams_ShadowCascades()
        {
            baseRange = cvCascadesBaseRange.get();
            baseEdgeFade = cvCascadesBaseEdgeFade.get();
            baseFilterSize = cvCascadesBaseFilterSize.get();
            baseDepthBiasConstant = cvCascadesDepthBiasConstant.get();
            baseDepthBiasSlope = cvCascadesDepthBiasSlope.get();

            depthBiasSlopeTexelSizeMul = cvCascadesDepthBiasTexelSizeMul.get();
            filterSizeTexelSizeMul = cvCascadesFilterSizeTexelSizeMul.get();

            rangeMul1 = cvCascadesRangeMul1.get();
            rangeMul2 = cvCascadesRangeMul2.get();
            rangeMul3 = cvCascadesRangeMul3.get();
        }

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
