/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Common math functions
*
***/

#pragma once

//----

const float PI = 3.1415926535897932f;

const int NUM_SAMPLES_HQ = 27;
const int NUM_SAMPLES_LQ = 13;
const int NUM_SAMPLES_DEPTH = 6;

const vec2[] ShadowDepthSamples = array(
	vec2( 0.0f, 0.0f ),
	vec2( 0.0f, 1.0f ),
	vec2( 0.95106f, 0.30902f ),
	vec2( 0.58778f, -0.80902f ),
	vec2( -0.95106f, 0.30902f ),
	vec2( -0.58778f, -0.80902f )
);

const vec2[] PoissonDiskHQ = array(
	vec2( 0, 0 ),
	vec2( -0.2633539f, 0.03965382f ),
	vec2( -0.7172024f, 0.3683033f ),
	vec2( -0.01995306f, -0.3988392f ),
	vec2( -0.4409938f, -0.22101f ),
	vec2( 0.2200392f, 0.2943448f ),
	vec2( -0.7675657f, -0.1482753f ),
	vec2( -0.2381019f, 0.5471062f ),
	vec2( 0.2173031f, -0.1345879f ),
	vec2( -0.6387774f, 0.7561339f ),
	vec2( -0.1614144f, 0.9776521f ),
	vec2( 0.2156319f, 0.6123413f ),
	vec2( 0.2328875f, 0.9452872f ),
	vec2( -0.9440644f, 0.1236077f ),
	vec2( -0.7408237f, -0.507683f ),
	vec2( -0.4113019f, -0.8905967f ),
	vec2( -0.2486429f, -0.6213993f ),
	vec2( 0.696785f, 0.2644937f ),
	vec2( 0.5394363f, 0.8173215f ),
	vec2( 0.6151208f, -0.149864f ),
	vec2( 0.09365336f, -0.7817475f ),
	vec2( 0.2768067f, -0.4895968f ),
	vec2( 0.6639181f, -0.6007172f ),
	vec2( 0.3880369f, -0.8950894f ),
	vec2( 0.9916144f, -0.07939152f ),
	vec2( 0.7831848f, 0.6029348f ),
	vec2( 0.8998603f, -0.3983543f )
);

const vec2[] PoissonDiskLQ = array(
	vec2( 0, 0 ),
	vec2(-0.5077208f, -0.6170899f),
	vec2(-0.05154902f, -0.7508098f),
	vec2(-0.7641042f, 0.2009152f),
	vec2(0.007284774f, -0.205947f),
	vec2(-0.366204f, 0.7323797f),
	vec2(-0.9239582f, -0.2559643f),
	vec2(-0.286924f, 0.278053f),
	vec2(0.7723012f, 0.1573329f),
	vec2(0.378513f, 0.4052199f),
	vec2(0.4630217f, -0.6914619f),
	vec2(0.423523f, 0.863028f),
	vec2(0.8558643f, -0.3382554f)
);

//----

//note: from https://www.shadertoy.com/view/4djSRW
// This set suits the coords of of 0-1.0 ranges..
const vec3 MOD3 = vec3(443.8975,397.2973, 491.1871);

float hash11(float p)
{
	vec3 p3  = frac(vec3(p,p,p) * MOD3);
    p3 += dot(p3, p3.yzx + 19.19);
    return frac((p3.x + p3.y) * p3.z);
}

float hash12(vec2 p)
{
	vec3 p3  = frac(vec3(p.xyx) * MOD3);
    p3 += dot(p3, p3.yzx + 19.19);
    return frac((p3.x + p3.y) * p3.z);
}

vec3 hash32(vec2 p)
{
	vec3 p3 = frac(vec3(p.xyx) * MOD3);
    p3 += dot(p3, p3.yxz+19.19);
    return frac(vec3((p3.x + p3.y)*p3.z, (p3.x+p3.z)*p3.y, (p3.y+p3.z)*p3.x));
}

//---

float Luminance(vec3 linearColor)
{
    return dot(linearColor, vec3(0.3f, 0.59f, 0.11f));
}

int DissolvePatternHelper(uint2 crd)
{
	return (crd.x & 1)
	    ? ((crd.y & 1) ? 1 : 2)
	    : ((crd.y & 1) ? 3 : 0);
}

float CalcDissolvePattern(uint2 pixelCoord, uint numSteps)
{
	int v = 0;

	for (uint i=0; i<numSteps; i += 1)
	{
		v = v << 2;
		v += DissolvePatternHelper(pixelCoord >> i);
	}

	float r = (1 << numSteps);
	return (v + 0.5) / (r * r);
}

float CalcDissolvePattern2(uint2 pixelCoord)
{
	return CalcDissolvePattern(pixelCoord, 2);
}

//----

// TODO: move to native code

int __postInc(out int a)
{
    int temp = a;
    a += 1;
    return temp;
}

int __postDec(out int a)
{
    int temp = a;
    a -= 1;
    return temp;
}

int __preInc(out int a)
{
    a += 1;
    return a;
}

int __preDec(out int a)
{
    a -= 1;
    return a;
}

//----

void UnpackClusterCounts(uint counts, out uint lightCount, out uint decalCount, out uint probeCount)
{
    lightCount = counts & 255;
    decalCount = (counts >> 8) & 255;
    probeCount = (counts >> 16) & 255;
}

uint UnpackClusterMaxCount(uint counts)
{
    uint lightCount = counts & 255;
    uint decalCount = (counts >> 8) & 255;
    uint probeCount = (counts >> 16) & 255;
    return max(lightCount, max(decalCount, probeCount));
}

uint UnpackClusterEntryLightIndex(uint entry)
{
    return (entry & 255);
}

//---

vec3 ClosestPointOnBox(vec3 boxMin, vec3 boxMax, vec3 pos)
{
    return max(min(pos, boxMax), boxMin);
}

bool TestBoxBox(vec3 amin, vec3 amax, vec3 bmin, vec3 bmax)
{
    if (any(amin > bmax) || any(bmin > amax)) return false;
    return true;
}

bool TestBoxSphere(vec3 boxMin, vec3 boxMax, vec3 sphereCenter, float radius)
{
    vec3 e = ClosestPointOnBox(boxMin, boxMax, sphereCenter) - sphereCenter;
    return dot(e, e) < (radius*radius);
}

//--

vec3 ComputeDiffuseColor(vec3 baseColor, float metallic)
{
    return baseColor.rgb * (1.0 - metallic);
}

vec3 ComputeF0(vec3 baseColor, float metallic, float reflectance)
{
    return baseColor.rgb * metallic + (reflectance * (1.0 - metallic));
}

float ComputeDielectricF0(float reflectance)
{
    return 0.16 * reflectance * reflectance;
}

const float MIN_N_DOT_V = 0.0001;

vec3 reflect(vec3 V, vec3 N)
{
    return V - N * 2.0f * dot(V,N);
}
