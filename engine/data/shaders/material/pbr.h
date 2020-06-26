/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* PBR helpers
*
***/

#pragma once

#include <math.h>

//----

// Light information
struct Light
{
    vec4 colorIntensity = vec4(1,1,1,1);  // rgb, pre-exposed intensity
    vec3 l = vec3(0,0,1);
    float attenuation = 1.0f;
    float NoL = 1.0f;
};

// PBR Pixel information
struct PBRPixel
{
    mat3 shading_tangentToWorld; // TBN matrix
    vec3 shading_position; // position of the fragment in world space
    vec3 shading_view; // normalized vector from the fragment to the eye
    vec3 shading_normal; // normalized normal, in world space
    vec3 shading_reflected; // reflection of view about normal
    float shading_NoV; // dot(normal, view), always strictly >= MIN_N_DOT_V

    vec3 diffuseColor = vec3(1,1,1);
    float perceptualRoughness = 0.5f;
    vec3 f0 = vec3(1,1,1);
    float roughness = 0.25f;
    vec3 dfg = vec3(0,0,0);
    vec3 energyCompensation = vec3(1,1,1);
    
    vec3 anisotropicT;
    vec3 anisotropicB;
    float anisotropy = 0.0f;
};

// PBR helper fragment
shader PBR
{
    const bool HAS_ANISOTROPY = false;
    
    //----
    
    float pow5(float x)
    {
        float x2 = x * x;
        return x2 * x2 * x;
    }

    float sq(float x)
    {
        return x*x;
    }
    
    // Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"
    vec3 F_Schlick(vec3 f0, float f90, float VoH)
    {
        return f0 + (f90 - f0) * pow5(1.0 - VoH);
    }

    vec3 F_Schlick2(vec3 f0, float VoH)
    {
        float f = pow(1.0 - VoH, 5.0);
        return f + f0 * (1.0 - f);
    }

    float F_SchlickScalar(float f0, float f90, float VoH)
    {
        return f0 + (f90 - f0) * pow5(1.0 - VoH);
    }

    vec3 fresnel(vec3 f0, float LoH)
    {
        float f90 = saturate(dot(f0, vec3(50.0 * 0.33, 50.0 * 0.33, 50.0 * 0.33)));
        return F_Schlick(f0, f90, LoH);
    }
    
    //----
    
    // Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
    float V_SmithGGXCorrelated(float roughness, float NoV, float NoL)
    {
        float a2 = roughness * roughness;
        float lambdaV = NoL * sqrt((NoV - a2 * NoV) * NoV + a2);
        float lambdaL = NoV * sqrt((NoL - a2 * NoL) * NoL + a2);
        float v = 0.5 / (lambdaV + lambdaL);
        return v;
    }

    // Hammon 2017, "PBR Diffuse Lighting for GGX+Smith Microsurfaces"
    float V_SmithGGXCorrelated_Fast(float roughness, float NoV, float NoL)
    {
        float v = 0.5 / lerp(2.0 * NoL * NoV, NoL + NoV, roughness);
        return v;
    }

    // Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
    float V_SmithGGXCorrelated_Anisotropic(float at, float ab, float ToV, float BoV, float ToL, float BoL, float NoV, float NoL)
    {
        float lambdaV = NoL * length(vec3(at * ToV, ab * BoV, NoV));
        float lambdaL = NoV * length(vec3(at * ToL, ab * BoL, NoL));
        float v = 0.5 / (lambdaV + lambdaL);
       return v;
    }

    // Kelemen 2001, "A Microfacet Based Coupled Specular-Matte BRDF Model with Importance Sampling"
    float V_Kelemen(float LoH)
    {
        return 0.25 / (LoH * LoH);
    }

    // Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for The Order: 1886"
    float V_Neubelt(float NoV, float NoL)
    {
        return 1.0 / (4.0 * (NoL + NoV - NoL * NoV));
    }
    
    float visibility(float roughness, float NoV, float NoL, float LoH)
    {
        return V_SmithGGXCorrelated(roughness, NoV, NoL);
    }
    
    //----
    
    // Walter et al. 2007, "Microfacet Models for Refractio n through Rough Surfaces"
    float D_GGX(float roughness, float NoH, vec3 h)
    {
        float oneMinusNoHSquared = 1.0 - NoH * NoH;
        float a = NoH * roughness;
        float k = roughness / (oneMinusNoHSquared + a * a);
        float d = k * k * (1.0 / PI);
        return d;
    }
    
    float distribution(float roughness, float NoH, vec3 h)
    {
        return D_GGX(roughness, NoH, h);
    }
    
    //----

    vec3 isotropicLobe(PBRPixel pixel, Light light, vec3 h, float NoV, float NoL, float NoH, float LoH)
    {
        float D = distribution(pixel.roughness, NoH, h);
        float V = visibility(pixel.roughness, NoV, NoL, LoH);
        vec3 F = fresnel(pixel.f0, LoH);

        return (D * V) * F;
    }

    vec3 specularLobe(PBRPixel pixel, Light light, vec3 h, float NoV, float NoL, float NoH, float LoH)
    {
/*        if (HAS_ANISOTROPY)
            return anisotropicLobe(pixel, light, h, NoV, NoL, NoH, LoH);
        else*/
            return isotropicLobe(pixel, light, h, NoV, NoL, NoH, LoH);
    }
    
    //----
    
    float Fd_Lambert()
    {
        return 1.0 / PI;
    }

    // Burley 2012, "Physically-Based Shading at Disney"    
    float Fd_Burley(float roughness, float NoV, float NoL, float LoH)
    {
        float f90 = 0.5 + 2.0 * roughness * LoH * LoH;
        float lightScatter = F_SchlickScalar(1.0, f90, NoL);
        float viewScatter  = F_SchlickScalar(1.0, f90, NoV);
        return lightScatter * viewScatter * (1.0 / PI);
    }

    float Fd_Wrap(float NoL, float w)
    {
        return saturate((NoL + w) / sq(1.0 + w));
    }
    
    float diffuse(float roughness, float NoV, float NoL, float LoH)
    {
//        return Fd_Lambert();
        return Fd_Burley(roughness, NoV, NoL, LoH);
    }
        
    vec3 diffuseLobe(PBRPixel pixel, float NoV, float NoL, float LoH)
    {
        return pixel.diffuseColor * diffuse(pixel.roughness, NoV, NoL, LoH);
    }

    //----

    vec3 surfaceShading(PBRPixel pixel, Light light, float occlusion)
    {
        float NoV = pixel.shading_NoV;
        vec3 h = normalize(pixel.shading_view + light.l);

        float NoL = saturate(light.NoL);
        float NoH = saturate(dot(pixel.shading_normal, h));
        float LoH = saturate(dot(light.l, h));
        
        vec3 Fr = specularLobe(pixel, light, h, NoV, NoL, NoH, LoH);
        vec3 Fd = diffuseLobe(pixel, NoV, NoL, LoH);

        // The energy compensation term is used to counteract the darkening effect at high roughness
        vec3 color = Fd + Fr;// * pixel.energyCompensation;

        return (color * light.colorIntensity.rgb) * (light.colorIntensity.w * light.attenuation * NoL * occlusion);// + vec3(light.NoL,0,0);
    }

    void configure()
    {
    }
}

//---