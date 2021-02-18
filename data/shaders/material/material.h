/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Canvas Rendering Pipeline
*
***/

#pragma once

#include <math.h>
#include <frame.h>
#include <camera.h>
#include <lighting.h>
#include <selection.h>

//--

sampler DefaultMaterialTextureSampler
{
    MinFilter = Linear,
    MagFilter = Linear,
    MipFilter = Linear,
    AddressU = Wrap,
    AddressV = Wrap,
    AddressW = Wrap,
	MaxAnisotropy = 16,
}

export shader MaterialPS
{
	void main()
	{
		gl_Target0 = vec4(1,0,1,1);
	}
	
	//--
	
	void PackPBR(out PBRPixel pbr, vec3 worldPosition, vec3 worldNormal, vec3 shadingNormal, vec3 baseColor, float metallic, float specular, float roughness)
	{
		pbr.shading_position = worldPosition;
		pbr.shading_normal = shadingNormal;
		pbr.face_normal = worldNormal;
		pbr.shading_view = normalize(CameraPosition - worldPosition);
		pbr.shading_NoV = max(MIN_N_DOT_V, dot(pbr.shading_normal, pbr.shading_view));
		pbr.shading_reflected = reflect(-pbr.shading_view, pbr.shading_normal);
		pbr.diffuseColor = baseColor * (1.0 - metallic);
		pbr.f0 = ComputeF0(baseColor, metallic, ComputeDielectricF0(specular));
		pbr.perceptualRoughness = roughness;
		pbr.roughness = pbr.perceptualRoughness * pbr.perceptualRoughness;
		pbr.energyCompensation = vec3(1);
	}
	
	//--

	void EmitSelection(uint objectId, vec3 worldPosition)
	{
		uint selectableId = ObjectData[objectId].SelectionObjectID;
		uint subId = ObjectData[objectId].SelectionSubObjectID;
		float linearZ = dot((worldPosition - CameraPosition), CameraForward);
		SelectionGatherPS.EmitSelection(selectableId, subId, linearZ);
	}

	//--
	
}

//--


         