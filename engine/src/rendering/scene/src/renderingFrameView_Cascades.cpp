/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\view  #]
***/

#include "build.h"
#include "renderingScene.h"
#include "renderingSceneUtils.h"
#include "renderingSceneFragment.h"
#include "renderingFrameRenderer.h"
#include "renderingFrameCameraContext.h"
#include "renderingFrameSurfaceCache.h"
#include "renderingFrameParams.h"
#include "renderingFrameView.h"
#include "renderingSceneCulling.h"
#include "renderingSceneProxy.h"
#include "renderingSceneFragmentList.h"
#include "renderingFrameView_Cascades.h"

namespace rendering
{
    namespace scene
    {
        //---

        const base::ConfigProperty<float> cvCascadesDepthBiasConstant("Rendering.Cascades", "BaseDepthConstant", 0.0f);
        const base::ConfigProperty<float> cvCascadesDepthBiasSlope("Rendering.Cascades", "BaseDepthSlope", 0.0f);
        const base::ConfigProperty<float> cvCascadesMinAngla("Rendering.Cascades", "MinAngle", 10.0f);

        //---

        FrameView_CascadeShadows::FrameView_CascadeShadows(const FrameRenderer& frame, const CascadeData& data)
            : FrameView(frame, FrameViewType::GlobalCascades, data.cascadeShadowMap.width(), data.cascadeShadowMap.height())
            , m_cascadeData(data)
        {}

        void FrameView_CascadeShadows::render(command::CommandWriter& cmd)
        {
        }

        //--

        static base::Vector3 CalculateLimitedShadowDirection(const base::Vector3& shadowDir, float degreesLimit)
        {
            auto limitedShadowDir = shadowDir;
            if (degreesLimit >= 0.0f && shadowDir.squareLength() > 0.1f)
            {
                limitedShadowDir.normalize();

                float xyLen = sqrtf(shadowDir.x * shadowDir.x + shadowDir.y * shadowDir.y);
                float angle = atan2(xyLen, shadowDir.z);
                float limitAngle = DEG2RAD * (90.0f + degreesLimit);
                if (angle < limitAngle && xyLen > 0.001f)
                {
                    auto s = sinf(limitAngle);
                    auto c = cosf(limitAngle);
                    limitedShadowDir.x = shadowDir.x / xyLen * s;
                    limitedShadowDir.y = shadowDir.y / xyLen * s;
                    limitedShadowDir.z = c;
                }
            }

            return limitedShadowDir.normalized();
        }

        static float ProjectZ(const Camera& camera, float zDist)
        {
            auto projZ = camera.viewToScreen().transformVector4(base::Vector4(0, 0, zDist, 1));
            return projZ.z / projZ.w;
        }

        static void CalculateBestCascadeFit(uint32_t resolution, const base::Vector3* corners, base::Vector3& outCenter, float& outRadius)
        {
            // find triangle's center
            auto& v1 = corners[1];
            auto& v2 = corners[3];
            auto& v3 = corners[5];

            auto denom = 1.0f / (2.0f * base::Cross(v1 - v2, v2 - v3).squareLength());
            auto bary0 = denom * (v2 - v3).squareLength() * base::Dot(v1 - v2, v1 - v3);
            auto bary1 = denom * (v1 - v3).squareLength() * base::Dot(v2 - v1, v2 - v3);
            auto bary2 = denom * (v1 - v2).squareLength() * base::Dot(v3 - v1, v3 - v2);

            auto sliceCenter = v1 * bary0 + v2 * bary1 + v3 * bary2;
            auto sliceRadius = (v1 - sliceCenter).length();

            // add margins for filtering
            auto worldTexelSize = (2.0f * sliceRadius) / std::max<float>(1.0f, resolution);
            sliceRadius += 4 * worldTexelSize;

            outCenter = sliceCenter;
            outRadius = sliceRadius;
        }

        static base::Quat CalculateShadowmapCameraRotation(const base::Vector3& dir)
        {
            auto right = base::Cross(base::Vector3::EZ(), dir).normalized();
            auto up = base::Cross(dir, right).normalized();

            auto lookAtMatrix = base::Matrix(dir, right, up);
            return lookAtMatrix.transposed().toQuat();

            //base::Matrix lookAtMatrix2 = dir.toRotator().toQuat().toMatrix();
            //return dir.toRotator().toQuat();
        }

        //---

        CascadeData::CascadeData()
        {
            cascadeShadowMap = ImageView::DefaultDepthArrayRT();
        }

        void CalculateCascadeSettings(const base::Vector3& globalLightDirection, const Camera& viewCamera, const FrameParams_ShadowCascades& setup, CascadeData& outData)
        {
            // get parameters for the cascades
            float paramRange[MAX_CASCADES];
            float paramEdgeFade[MAX_CASCADES];
            float paramFilterSize[MAX_CASCADES];
            float paramBiasConstant[MAX_CASCADES];
            float paramBiasSlope[MAX_CASCADES];
            paramRange[0] = std::max<float>(setup.baseRange, 1.0f);
            paramRange[1] = paramRange[0] * std::max<float>(1.01f, setup.rangeMul1);
            paramRange[2] = paramRange[1] * std::max<float>(1.01f, setup.rangeMul2);
            paramRange[3] = paramRange[2] * std::max<float>(1.01f, setup.rangeMul3);
            paramEdgeFade[0] = std::clamp<float>(setup.baseEdgeFade, 0.0f, 0.5f);
            paramEdgeFade[1] = std::clamp<float>(setup.baseEdgeFade, 0.0f, 0.5f);
            paramEdgeFade[2] = std::clamp<float>(setup.baseEdgeFade, 0.0f, 0.5f);
            paramEdgeFade[3] = std::clamp<float>(setup.baseEdgeFade, 0.0f, 0.5f);

            // filter size
            const auto depthMapResolution = outData.cascadeShadowMap.width();
            const auto singleTexelSize = 1.0f / (float)depthMapResolution;
            paramFilterSize[0] = singleTexelSize * std::clamp<float>(setup.baseFilterSize, 0.5f, depthMapResolution / 2.0f);
            paramFilterSize[1] = singleTexelSize * std::clamp<float>(setup.baseFilterSize, 0.5f, depthMapResolution / 2.0f);
            paramFilterSize[2] = singleTexelSize * std::clamp<float>(setup.baseFilterSize, 0.5f, depthMapResolution / 2.0f);
            paramFilterSize[3] = singleTexelSize * std::clamp<float>(setup.baseFilterSize, 0.5f, depthMapResolution / 2.0f);

            // depth bias 
            // TODO: more analitic calculation that uses the world-space texel size
            paramBiasConstant[0] = cvCascadesDepthBiasConstant.get();
            paramBiasConstant[1] = cvCascadesDepthBiasConstant.get();
            paramBiasConstant[2] = cvCascadesDepthBiasConstant.get();
            paramBiasConstant[3] = cvCascadesDepthBiasConstant.get();
            paramBiasSlope[0] = cvCascadesDepthBiasSlope.get();
            paramBiasSlope[1] = cvCascadesDepthBiasSlope.get();
            paramBiasSlope[2] = cvCascadesDepthBiasSlope.get();
            paramBiasSlope[3] = cvCascadesDepthBiasSlope.get();

            // generate cascades
            auto numCascades = (uint32_t)std::clamp<int>(setup.numCascades, 0, MAX_CASCADES);

            // calculate the cascades projection direction
            auto shadowDir = CalculateLimitedShadowDirection(-globalLightDirection, cvCascadesMinAngla.get());

            // build cascade parameters (spheres around the frustum slices)
            base::Vector3 cascadeSphereCenter[MAX_CASCADES];
            base::Vector3 cascadeFrustumVertices[MAX_CASCADES * 8];
            float cascadeSphereRadius[MAX_CASCADES];
            float prevDistance = 0.1f;
            for (uint32_t i = 0; i < numCascades; i++)
            {
                auto endDist = paramRange[i] / 4.0f;

                // extract slice frustum range
                auto corners = &cascadeFrustumVertices[i * 8];
                viewCamera.calcFrustumCorners(ProjectZ(viewCamera, prevDistance), corners + 0);//, true);
                viewCamera.calcFrustumCorners(ProjectZ(viewCamera, endDist), corners + 4);//, true );
                prevDistance = endDist;

                // calculate the bounding sphere
                CalculateBestCascadeFit(depthMapResolution, corners, cascadeSphereCenter[i], cascadeSphereRadius[i]);
                //cascadeSphereCenter[i] += viewCamera.position();
            }

            // TODO: stabilize

            // calculate cascade distances
            float cascadeDistances[MAX_CASCADES];
            float cascadeDistancesMin = std::numeric_limits<float>::max();
            float cascadeDistancesMax = -std::numeric_limits<float>::max();
            for (uint32_t i = 0; i < numCascades; ++i)
            {
                cascadeDistances[i] = base::Dot(shadowDir, cascadeSphereCenter[i]);
                cascadeDistancesMin = std::min(cascadeDistancesMin, cascadeDistances[i]);
                cascadeDistancesMax = std::max(cascadeDistancesMax, cascadeDistances[i]);
            }

            // calculate cameras and assign rendering slices
            for (uint32_t i = 0; i < numCascades; ++i)
            {
                auto& setup = outData.cascades.emplaceBack();

                // TODO: compute proper range for the projection
                // TODO: minimize the projection range

                // create projection camera
                CameraSetup cameraSetup;
                cameraSetup.nearPlane = -1000.0f - (cascadeDistances[i] - cascadeDistancesMin);
                cameraSetup.farPlane = 1000.0 + (cascadeDistancesMax - cascadeDistances[i]);
                cameraSetup.position = cascadeSphereCenter[i];
                cameraSetup.rotation = CalculateShadowmapCameraRotation(shadowDir);
                cameraSetup.fov = 0.0f;
                cameraSetup.aspect = 1.0f;
                cameraSetup.zoom = 4.0f * cascadeSphereRadius[i];
                setup.camera.setup(cameraSetup);

                // calculate pixel projection size
                auto viewportSize = 2.0f / viewCamera.viewToScreen().m[0][0];
                setup.pixelSize = viewportSize / std::max<float>(1, depthMapResolution);
                setup.invPixelSize = 1.0f / setup.pixelSize;
                setup.cascadeIndex = (uint8_t)i;
                setup.edgeFade = paramEdgeFade[i];
                setup.filterTexelSize = paramFilterSize[i];
                setup.filterScale = setup.filterTexelSize / (float)depthMapResolution;

                // TODO: jitter
                setup.jitterCamera = setup.camera;

                // calculate depth bias
                auto filterSizeInTexels = ceilf(1.4142f * setup.filterTexelSize);
                setup.depthBiasConstant = paramBiasConstant[i];
                setup.depthBiasSlope = paramBiasSlope[i] * filterSizeInTexels;
            }
        }

        //--

        void PackCascadeData(const CascadeData& src, GPUCascadeInfo& outData)
        {
            memset(&outData, 0, sizeof(outData)); // just in case we don't set some fields

            const auto numCascades = src.cascades.size();
            if (numCascades)
            {
                // everything is calculated with respect to cascade 0
                // instead of storing 4 matrices we store one and 2D offset/scale factors for easier cascade selection
                auto basePos = src.cascades[0].camera.position();
                auto baseRight = src.cascades[0].camera.directionRight();
                auto baseUp = -src.cascades[0].camera.directionUp();
                auto baseTransform = src.cascades[0].camera.worldToScreen();

                // calculate sub pixel offset for Poisson offsets
                //auto cameraSubPixelOffsetX = 0.0f;
                //auto cameraSubPixelOffsetY = 0.0f;
                //auto subPixelOffsetXY = cameraSubPixelOffsetY * m_shadowMapResolution * 32 + cameraSubPixelOffsetX * 32;

                // Initialize the constants that will be set to mark the cascades placement
                float paramOffsetsRight[MAX_CASCADES] = { 0.f, 0.f, 0.f, 0.f };
                float paramOffsetsUp[MAX_CASCADES] = { 0.f, 0.f, 0.f, 0.f };
                float paramHalfSizes[MAX_CASCADES] = { 999, 999, 999, 999 };
                float paramDepthRanges[MAX_CASCADES] = { 1.f, 1.f, 1.f, 1.f };
                for (uint32_t i = 0; i < numCascades; ++i)
                {
                    auto& camera = src.cascades[i].camera;

                    auto cameraOffset = camera.position() - basePos;
                    paramOffsetsRight[i] = base::Dot(baseRight, cameraOffset);
                    paramOffsetsUp[i] = base::Dot(baseUp, cameraOffset);
                    paramHalfSizes[i] = 0.5f * camera.zoom();
                    paramDepthRanges[i] = camera.farPlane() - camera.nearPlane();
                }

                // normalize parameters
                auto paramsScale = 1.0f / paramHalfSizes[0];
                for (uint32_t i = 0; i < numCascades; ++i)
                {
                    paramOffsetsRight[i] *= paramsScale;
                    paramOffsetsUp[i] *= paramsScale;
                    paramHalfSizes[i] *= paramsScale;
                }

                // setup final fade
                const auto resolution = src.cascadeShadowMap.width();
                auto fadeCascadeIndex = std::max<int>(0, (int)numCascades - 1);
                auto fadeRangeInner = paramHalfSizes[fadeCascadeIndex] * (100.f / resolution);
                auto fadeRangeOuter = 0.05f;

                // pack constants
                outData.ShadowTransform = baseTransform;
                outData.ShadowTextureSize = base::Vector4(resolution, 1.0f / resolution, 0.0f, 0.0f);
                outData.ShadowOffsetsX = base::Vector4(paramOffsetsRight[0], paramOffsetsRight[1], paramOffsetsRight[2], paramOffsetsRight[3]);
                outData.ShadowOffsetsY = base::Vector4(paramOffsetsUp[0], paramOffsetsUp[1], paramOffsetsUp[2], paramOffsetsUp[3]);
                outData.ShadowHalfSizes = base::Vector4(paramHalfSizes[0], paramHalfSizes[1], paramHalfSizes[2], paramHalfSizes[3]);
                outData.ShadowFadeScales = base::Vector4(src.cascades[0].edgeFade, src.cascades[1].edgeFade, src.cascades[2].edgeFade, src.cascades[3].edgeFade);
                outData.ShadowDepthRanges = base::Vector4(paramDepthRanges[0], paramDepthRanges[1], paramDepthRanges[2], paramDepthRanges[3]);
                outData.ShadowPoissonOffsetAndBias = base::Vector4();
                outData.ShadowPoissonOffsetAndBias.z = (float)numCascades;
                outData.ShadowQuality = 0;

                for (uint32_t i = 0; i < 4; ++i)
                {
                    auto halfSegSize = (paramHalfSizes[i] > SMALL_EPSILON) ? 2.0f * paramHalfSizes[i] : 1.0f;

                    // CPU precompute. This will allow use MAD in shader
                    outData.ShadowParams[i].x = -(paramOffsetsRight[i] - paramHalfSizes[i]) / halfSegSize;    // shadow map center.x
                    outData.ShadowParams[i].y = -(paramOffsetsUp[i] - paramHalfSizes[i]) / halfSegSize;    // shadow map center.y
                    outData.ShadowParams[i].z = 1.0f / halfSegSize;

                    // prevent filter size to be smaller than one pixel
                    float minFilterTexelSize = 1.0f / static_cast<float>(resolution);
                    outData.ShadowParams[i].w = std::max<float>(minFilterTexelSize, src.cascades[i].filterScale);
                }
            }
            else
            {

            }
        }

        //--

    } // scene
} // rendering

