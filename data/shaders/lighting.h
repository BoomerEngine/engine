/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
***/

#include "math.h"
#include "pbr.h"

//--

struct GlobalLightingSetup
{
    vec3 LightDirection; // normal vector towards the global light
    vec3 LightColor; // color of the global light, LINEAR
    vec3 AmbientColorZenith; // color of the global ambient light, LINEAR
    vec3 AmbientColorHorizon; // color of the global light, LINEAR
};

struct GlobalCascadesSetup
{
    float4x4 ShadowTransform;
    float4 ShadowOffsetsX;
    float4 ShadowOffsetsY;
    float4 ShadowHalfSizes;
    float4[4] ShadowParams;
    float4 ShadowPoissonOffsetAndBias;
    float4 ShadowTextureSize;
    float4 ShadowFadeScales;
    float4 ShadowDepthRanges;
    uint ShadowQuality;
};

//--

sampler ShadowMapSampler
{
    MinFilter = Linear,
    MagFilter = Linear,
    MipFilter = None,
    AddressU = Clamp,
    AddressV = Clamp,
    AddressW = Clamp,
    CompareEnabled = true,
    CompareOp = LessEqual,
}

sampler GlobalShadowMaskSampler
{
	MinFilter = Linear,
	MagFilter = Linear,
	MipFilter = None,
	AddressU = Clamp,
	AddressV = Clamp,
	AddressW = Clamp,
};

descriptor LightingParams
{
	ConstantBuffer
	{
		GlobalLightingSetup GlobalLightingData;
		GlobalCascadesSetup CascadeData;
	};	

	attribute(nosampler, format=rgba8) Texture2D GlobalShadowMaskAO; // R-shadow mask (cascades+terrain+optional raytrace) G-AO 
	attribute(depth, sampler = ShadowMapSampler) Texture2DArray CascadeShadowMap;
};

//--

shader Lighting
{
	vec3 ComputeGlobalAmbient(vec3 worldPosition, vec3 worldNormal, float ao)
	{
		return ao * lerp(GlobalLightingData.AmbientColorHorizon, GlobalLightingData.AmbientColorZenith, saturate(worldNormal.z));
	}
	
	vec3 ComputeGlobalLighting(PBRPixel pbr, float occlusion)
	{
		Light light;
		light.colorIntensity = GlobalLightingData.LightColor.xyz1;
		light.l = GlobalLightingData.LightDirection;
		light.NoL = saturate(dot(GlobalLightingData.LightDirection, pbr.shading_normal));
        light.attenuation = saturate((dot(GlobalLightingData.LightDirection, pbr.face_normal) + 0.05) / 1.05);
        return max(vec3(0), PBR.SurfaceShading(pbr, light, occlusion));
	}
	
	float SampleGlobalShadowMask(ivec2 screenPos)
	{
		//return 1.0;
		//return saturate(screenPos.x / 500.0f);
		return GlobalShadowMaskAO[screenPos].x;
	}

	float SampleGlobalAmbientOcclusion(ivec2 screenPos)
	{
		//return 1.0;
		//return saturate(screenPos.x / 500.0f);
		return GlobalShadowMaskAO[screenPos].y;
	}	
}

//--

shader Cascades
{
    float ComputeShadowsAtPoint(vec3 worldPos, ivec2 pixelCoord)
    {
        // calculate the 
        vec3 pos_sh = (CascadeData.ShadowTransform * worldPos.xyz1).xyz;

        // if shadow pixel is on the far plane (or beyond) of the shadowmap
        // this is the case of pixels that are very far away and fall outside cascade depth bounds
        // do not calculate shadow factor for such pixels, assume they are visible
        //[branch]
        if (abs(pos_sh.z) >= 0.999f)
            return 1.0f;

        // sample cascades
        float4 _dx = (pos_sh.xxxx - CascadeData.ShadowOffsetsX);
        float4 _dy = (pos_sh.yyyy - CascadeData.ShadowOffsetsY);
        float4 _max_d = max(abs(_dx), abs(_dy));

        // one for every cascade we are outside
        float4 _it = (_max_d < CascadeData.ShadowHalfSizes);

        // calculate the primary cascade we are inside
        // in cascade 0: 1 1 1 1  // 4
        // in cascade 1: 0 1 1 1  // 3
        // in cascade 2: 0 0 1 1  // 2
        // in cascade 3: 0 0 0 1  // 1
        // outside:      0 0 0 0  // 0
        float maxCascades = CascadeData.ShadowPoissonOffsetAndBias.z;
        float4 mask = (maxCascades.xxxx > float4(0.5f, 1.5f, 2.5f, 3.5f));
        float _i = dot(_it, mask);

        // outside cascade system
        //[branch]
        if (_i < 0.5f)
            return 1.0f;

        // calculate cascade fade factors (last 20% of each cascade)
        float4 fadeFactors = saturate(((_max_d / CascadeData.ShadowHalfSizes) - (1.0f - CascadeData.ShadowFadeScales)) / CascadeData.ShadowFadeScales);
        float[4] fadeFactorsArray;
        fadeFactorsArray[0] = fadeFactors.x;
        fadeFactorsArray[1] = fadeFactors.y;
        fadeFactorsArray[2] = fadeFactors.z;
        fadeFactorsArray[3] = fadeFactors.w;

        // calculate cascade index
        int cascadeIndex = (int)(maxCascades - _i);
        int lastCascadeID = (int)(maxCascades - 1);

        // dither the cascade transition
        cascadeIndex += (CalcDissolvePattern(pixelCoord, 4) < fadeFactorsArray[cascadeIndex]) ? 1 : 0;
        cascadeIndex = min(cascadeIndex, lastCascadeID);
        //vec3 ret = vec3((float)(cascadeIndex == 0), (float)(cascadeIndex == 1), (float)(cascadeIndex == 2));

        // calculate shadow factor for selected cascade
        float ret = ComputeShadowsFromCascade(pixelCoord, pos_sh, cascadeIndex);
        ret = saturate((float)(cascadeIndex == lastCascadeID) * fadeFactorsArray[lastCascadeID] + ret);
        return ret;
    }

    float ComputeShadowsFromCascade(ivec2 screen_pos, vec3 pos_sh, int cascadeIndex)
    {
        // normalize coordinates to fit inside shadow cascade
        float4 shadowParam = CascadeData.ShadowParams[cascadeIndex];
        float2 sh_coord = pos_sh.xy * float2(shadowParam.z, shadowParam.z) + shadowParam.xy;

        // filter shadows
        float gradientSize = 0.0f;
        float kernelSize = shadowParam.w;
        return SampleShadowmap(screen_pos, sh_coord, cascadeIndex, pos_sh.z, kernelSize, gradientSize);
    }

    float hash(vec2 p)
    {
        return frac(1000.0 * sin(17.0 * p.x + p.y * 0.1) * (0.1 + abs(sin(p.y * 13.0 + p.x))));
    }

    float SampleShadowmap(ivec2 screen_pos, float2 uv, int cascadeIndex, float zReceiver, float kernelSize, float gradientSize)
    {
        float sum = 0.0f;

        float rot = hash(screen_pos);
        vec2 rotX = vec2(sin(rot), cos(rot));
        vec2 rotY = vec2(rotX.y, -rotX.x);

        for (int i = 0; i < NUM_SAMPLES_LQ; ++i)
        {
            vec2 offset = PoissonDiskLQ[i] * kernelSize;

            offset = vec2(dot(offset, rotX), dot(offset, rotY));

            float zReceiverTest = zReceiver - gradientSize * (i / (float)NUM_SAMPLES_LQ);
            sum += textureDepthCompare(CascadeShadowMap, vec3(uv.xy + offset.xy, cascadeIndex), zReceiverTest);
        }

        float result = sum / NUM_SAMPLES_LQ;
        result *= result; // gamma
        return result;
    }
};