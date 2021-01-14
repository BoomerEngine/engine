/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\view  #]
***/

#include "build.h"
#include "renderingFrameRenderer.h"
#include "renderingFrameCameraContext.h"
#include "renderingFrameParams.h"
#include "renderingFrameView.h"
#include "renderingFrameRenderingService.h"

#include "rendering/device/include/renderingImage.h"
#include "rendering/device/include/renderingDescriptor.h"
#include "rendering/device/include/renderingCommandWriter.h"

namespace rendering
{
    namespace scene
    {
        //---

        const base::ConfigProperty<float> cvCascadesMinAngla("Rendering.Cascades", "MinAngle", 10.0f);

        //---

#define MATERIAL_FLAG_DISABLE_COLOR 1
#define MATERIAL_FLAG_DISABLE_LIGHTING 2
#define MATERIAL_FLAG_DISABLE_TEXTURES 4
#define MATERIAL_FLAG_DISABLE_NORMAL 8
#define MATERIAL_FLAG_DISABLE_VERTEX_COLOR 16
#define MATERIAL_FLAG_DISABLE_OBJECT_COLOR 32
#define MATERIAL_FLAG_DISABLE_VERTEX_MOTION 64
#define MATERIAL_FLAG_DISABLE_MASKING 128

        static base::FastRandState GRenderingRandState;

        static base::Point CovertMouseCoords(base::Point coords, const FrameRenderer& frame)
        {
            if (coords.x < 0 || coords.y < 0)
                return base::Point(-1, -1);

            auto ret = coords;

            /*if (frame.verticalFlip())
                ret.y = (frame.frame().resolution.height - 1) - ret.y;*/

            return ret;
        }

        void PackFrameParams(GPUFrameParameters& outParams, const FrameRenderer& frame, const FrameCompositionTarget& targets)
        {
            const auto& data = frame.frame();

            memset(&outParams, 0, sizeof(outParams));

            outParams.ViewportSize.x = data.resolution.width;
            outParams.ViewportSize.y = data.resolution.height;
            outParams.ViewportRTSize.x = targets.targetColorRTV->width();
            outParams.ViewportRTSize.y = targets.targetColorRTV->height();
            outParams.InvViewportSize.x = outParams.ViewportSize.x ? (1.0f / outParams.ViewportSize.x) : 0.0f;
            outParams.InvViewportSize.y = outParams.ViewportSize.y ? (1.0f / outParams.ViewportSize.y) : 0.0f;
            outParams.InvViewportRTSize.x = outParams.ViewportRTSize.x ? (1.0f / outParams.ViewportRTSize.x) : 0.0f;
            outParams.InvViewportRTSize.y = outParams.ViewportRTSize.y ? (1.0f / outParams.ViewportRTSize.y) : 0.0f;

            outParams.FrameIndex = data.index;
            outParams.MSAASamples = 1;// view.multisampled() ? view.numSamples() : 0;

            outParams.PseudoRandom[0] = GRenderingRandState.next();
            outParams.PseudoRandom[1] = GRenderingRandState.next();
            outParams.PseudoRandom[2] = GRenderingRandState.next();
            outParams.PseudoRandom[3] = GRenderingRandState.next();

            outParams.MaterialFlags = 0;
            const auto& filters = frame.frame().filters;
            if (filters & FilterBit::Material_DisableColorMap)
                outParams.MaterialFlags |= MATERIAL_FLAG_DISABLE_COLOR;
            if (filters & FilterBit::Material_DisableLighting)
                outParams.MaterialFlags |= MATERIAL_FLAG_DISABLE_LIGHTING;
            if (filters & FilterBit::Material_DisableTextures)
                outParams.MaterialFlags |= MATERIAL_FLAG_DISABLE_TEXTURES;
            if (filters & FilterBit::Material_DisableNormals)
                outParams.MaterialFlags |= MATERIAL_FLAG_DISABLE_NORMAL;
            if (filters & FilterBit::Material_DisableObjectColor)
                outParams.MaterialFlags |= MATERIAL_FLAG_DISABLE_OBJECT_COLOR;
            if (filters & FilterBit::Material_DisableVertexColor)
                outParams.MaterialFlags |= MATERIAL_FLAG_DISABLE_VERTEX_COLOR;
            if (filters & FilterBit::Material_DisableMasking)
                outParams.MaterialFlags |= MATERIAL_FLAG_DISABLE_MASKING;
            if (filters & FilterBit::Material_DisableVertexMotion)
                outParams.MaterialFlags |= MATERIAL_FLAG_DISABLE_VERTEX_MOTION;


            //if (frame.verticalFlip())
            //{
              //  outParams.ScreenTopY = view.height()
            //}

            outParams.DebugMousePos = CovertMouseCoords(data.debug.mouseHoverPixel, frame);
            outParams.DebugMouseClickPos = CovertMouseCoords(data.debug.mouseClickedPixel, frame);

            outParams.TimeOfDay = data.time.timeOfDay;
            outParams.GameTime = data.time.gameTime;
            outParams.EngineTime = data.time.engineRealTime;
            outParams.DayNightFraction = data.time.dayNightFrac;
        }

        void PackSingleCameraParams(GPUCameraInfo& outInfo, const Camera& camera, const Camera* prevCamera /*= nullptr*/)
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

        void PackLightingParams(GPULightingInfo& outInfo, const FrameParams_GlobalLighting& lighting)
        {
            outInfo.globalAmbientColorHorizon = lighting.globalAmbientColorHorizon;
            outInfo.globalAmbientColorZenith = lighting.globalAmbientColorZenith;
            outInfo.globalLightColor = lighting.globalLightColor;
            outInfo.globalLightDirection = lighting.globalLightDirection;
        }

        void PackCascadeParams(GPUCascadeInfo& outData, const CascadeData& src)
        {
            memset(&outData, 0, sizeof(outData)); // just in case we don't set some fields

            if (src.numCascades)
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
                for (uint32_t i = 0; i < src.numCascades; ++i)
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
                for (uint32_t i = 0; i < src.numCascades; ++i)
                {
                    paramOffsetsRight[i] *= paramsScale;
                    paramOffsetsUp[i] *= paramsScale;
                    paramHalfSizes[i] *= paramsScale;
                }

                // setup final fade
                const auto resolution = 1024;// src.numCascades cascadeShadowMap.width();
                auto fadeCascadeIndex = std::max<int>(0, (int)src.numCascades - 1);
                auto fadeRangeInner = paramHalfSizes[fadeCascadeIndex] * (100.f / resolution);
                auto fadeRangeOuter = 0.05f;

                // pack constants
                outData.ShadowTransform = baseTransform.transposed();
                outData.ShadowTextureSize = base::Vector4(resolution, 1.0f / resolution, 0.0f, 0.0f);
                outData.ShadowOffsetsX = base::Vector4(paramOffsetsRight[0], paramOffsetsRight[1], paramOffsetsRight[2], paramOffsetsRight[3]);
                outData.ShadowOffsetsY = base::Vector4(paramOffsetsUp[0], paramOffsetsUp[1], paramOffsetsUp[2], paramOffsetsUp[3]);
                outData.ShadowHalfSizes = base::Vector4(paramHalfSizes[0], paramHalfSizes[1], paramHalfSizes[2], paramHalfSizes[3]);
                outData.ShadowFadeScales = base::Vector4(src.cascades[0].edgeFade, src.cascades[1].edgeFade, src.cascades[2].edgeFade, src.cascades[3].edgeFade);
                outData.ShadowDepthRanges = base::Vector4(paramDepthRanges[0], paramDepthRanges[1], paramDepthRanges[2], paramDepthRanges[3]);
                outData.ShadowPoissonOffsetAndBias = base::Vector4();
                outData.ShadowPoissonOffsetAndBias.z = (float)src.numCascades;
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

        //---

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

        void CalculateCascadeSettings(const base::Vector3& globalLightDirection, uint32_t depthMapResolution, const Camera& viewCamera, const FrameParams_ShadowCascades& setup, CascadeData& outData)
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
            const auto singleTexelSize = 1.0f / (float)depthMapResolution;
            paramFilterSize[0] = std::clamp<float>(setup.baseFilterSize, 0.5f, depthMapResolution / 2.0f);
            paramFilterSize[1] = std::clamp<float>(setup.baseFilterSize, 0.5f, depthMapResolution / 2.0f);
            paramFilterSize[2] = std::clamp<float>(setup.baseFilterSize, 0.5f, depthMapResolution / 2.0f);
            paramFilterSize[3] = std::clamp<float>(setup.baseFilterSize, 0.5f, depthMapResolution / 2.0f);

            // depth bias 
            // TODO: more analytic calculation that uses the world-space texel size
            paramBiasConstant[0] = setup.baseDepthBiasConstant;
            paramBiasConstant[1] = setup.baseDepthBiasConstant;
            paramBiasConstant[2] = setup.baseDepthBiasConstant;
            paramBiasConstant[3] = setup.baseDepthBiasConstant;
            paramBiasSlope[0] = setup.baseDepthBiasSlope;
            paramBiasSlope[1] = setup.baseDepthBiasSlope;
            paramBiasSlope[2] = setup.baseDepthBiasSlope;
            paramBiasSlope[3] = setup.baseDepthBiasSlope;

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
            outData.numCascades = numCascades;
            for (uint32_t i = 0; i < numCascades; ++i)
            {
                auto& cascadeSetup = outData.cascades[i];

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
                cascadeSetup.camera.setup(cameraSetup);

                // calculate pixel projection size
                auto viewportSize = 2.0f / viewCamera.viewToScreen().m[0][0];
                cascadeSetup.pixelSize = viewportSize / std::max<float>(1, depthMapResolution);
                cascadeSetup.invPixelSize = 1.0f / cascadeSetup.pixelSize;
                cascadeSetup.cascadeIndex = (uint8_t)i;
                cascadeSetup.edgeFade = paramEdgeFade[i];
                cascadeSetup.worldSpaceTexelSize = cameraSetup.zoom / (float)depthMapResolution;
                cascadeSetup.filterTexelSize = paramFilterSize[i];
                cascadeSetup.filterScale = cascadeSetup.filterTexelSize / (float)depthMapResolution;

                // TODO: jitter
                cascadeSetup.jitterCamera = cascadeSetup.camera;

                // calculate depth bias
                cascadeSetup.depthBiasConstant = paramBiasConstant[i];
                cascadeSetup.depthBiasSlope = paramBiasSlope[i];

                // adjust for >1 cascades
                if (i > 0)
                {
                    auto sizeAdj = std::max<float>(1.0f, (cameraSetup.zoom / outData.cascades[0].camera.zoom()));
                    cascadeSetup.depthBiasSlope *= (sizeAdj - 1.0f) * setup.depthBiasSlopeTexelSizeMul;

                    if (setup.filterSizeTexelSizeMul >= 0.0f)
                        cascadeSetup.filterScale *= (1.0f + (sizeAdj - 1.0f) * setup.filterSizeTexelSizeMul);
                    else
                        cascadeSetup.filterScale /= (1.0f + (sizeAdj - 1.0f) * -setup.filterSizeTexelSizeMul);
                }
            }
        }

        //---

        FrameViewSingleCamera::FrameViewSingleCamera(const FrameRenderer& frame, const Camera& camera, const base::Rect& viewport)
            : m_frame(frame)
            , m_camera(camera)
            , m_viewport(viewport)
        {}

        FrameViewSingleCamera::~FrameViewSingleCamera()
        {}

        void FrameViewSingleCamera::bindCamera(command::CommandWriter& cmd)
        {
            GPUCameraInfo cameraParams;
            PackSingleCameraParams(cameraParams, m_frame.frame().camera.camera);

            DescriptorEntry desc[1];
            desc[0].constants(cameraParams);
            cmd.opBindDescriptor("CameraParams"_id, desc);
        }

        //--

    } // scene
} // rendering

