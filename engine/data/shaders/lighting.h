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

descriptor LightingParams
{
	ConstantBuffer
	{
		GlobalLightingSetup GlobalLightingData;
	};	

	Texture2D GlobalShadowMaskAO; // R-shadow mask (cascades+terrain+optional raytrace) G-AO 
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
        light.attenuation = 1.0f;
		light.NoL = saturate(dot(GlobalLightingData.LightDirection, pbr.shading_normal));
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