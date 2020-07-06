/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\view #]
***/

#include "build.h"
#include "renderingFrameRenderer.h"
#include "renderingFrameView.h"

namespace rendering
{
    namespace scene
    {

        //---

        // maximum supported number of cascades
        static const uint32_t MAX_CASCADES = 4;

        //---

        // calculate cascade information
        struct CascadeInfo
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


#pragma pack(push)
#pragma pack(4)
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
#pragma pack(pop)

        //--

        struct RENDERING_SCENE_API CascadeData
        {
            uint8_t numCascades = 0;
            ImageView cascadeShadowMap;
            CascadeInfo cascades[MAX_CASCADES];

            CascadeData();
        };

        //--

        struct RENDERING_SCENE_API LightingData
        {
            FrameParams_GlobalLighting globalLighting;

            ImageView globalShadowMaskAO;

            LightingData();
        };

        //--

        class RENDERING_SCENE_API FrameView_CascadeShadows : public FrameView
        {
        public:
            FrameView_CascadeShadows(const FrameRenderer& frame, const CascadeData& data);

            void render(command::CommandWriter& cmd);

        private:
            const CascadeData& m_cascadeData;
        };

        //--

        // calculate cascade settings that best match given camera 
        extern RENDERING_SCENE_API void CalculateCascadeSettings(const base::Vector3& lightDirection, const Camera& viewCamera, const FrameParams_ShadowCascades& setup, CascadeData& outData);

        /// pack cascade data for rendering
        extern RENDERING_SCENE_API void PackCascadeData(const CascadeData& src, GPUCascadeInfo& outData);

        //--

    } // scene
} // rendering

