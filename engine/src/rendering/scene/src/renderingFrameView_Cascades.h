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

		//--

		// info about single cascades
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

		struct RENDERING_SCENE_API CascadeData
		{
			uint8_t numCascades = 0;

			CascadeInfo cascades[MAX_CASCADES];

			ImageObjectPtr cascadesAtlas;
			ImageSampledViewPtr cascadesAtlasSRV;
			RenderTargetViewPtr cascadesAtlasRTVArray;
			RenderTargetViewPtr cascadesAtlasRTV[MAX_CASCADES];

			CascadeData();
		};

        //--

        // calculate cascade settings that best match given camera 
        //extern RENDERING_SCENE_API void CalculateCascadeSettings(const base::Vector3& lightDirection, const Camera& viewCamera, const FrameParams_ShadowCascades& setup, CascadeData& outData);

        //--

    } // scene
} // rendering

