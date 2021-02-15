/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Canvas Rendering Pipeline
*
***/

#include <math.h>
#include <canvas/canvas.h>

//--

descriptor ExtraParams
{
	ConstantBuffer
	{
		vec4 ColorFilter;

		int MipIndex;
		int ColorSpace;
		float ColorSpaceScale;
		int ToneMapMode;
	}

	Sampler PreviewTextureSampler;
	attribute(sampler=PreviewTextureSampler) Texture2D PreviewTexture;
}

vec3 Heatmap(float val)
{
    float level = val*3.14159265/2.0;
    
    vec3 col;
    col.r = sin(level);
    col.g = sin(level*2.);
    col.b = cos(level);
	return col;
}

float linear_to_srgb_float(float x) {
    if (x <= 0.0f)
        return 0.0f;
    else if (x >= 1.0f)
        return 1.0f;
    else if (x < 0.0031308f)
        return x * 12.92f;
    else
        return pow(x, 1.0f / 2.4f) * 1.055f - 0.055f;
}

vec3 linear_to_srgb_float_v3(vec3 c) {
	c.x = linear_to_srgb_float(c.x);
	c.y = linear_to_srgb_float(c.y);
	c.z = linear_to_srgb_float(c.z);
	return c;
}

const float GAMMA = 2.2;
const vec3 INV_GAMMA = vec3(1./GAMMA, 1./GAMMA, 1./GAMMA);

vec3 linearToneMapping(vec3 color)
{
	color = saturate(color);
	color = pow(color, INV_GAMMA);
	return color;
}

vec3 simpleReinhardToneMapping(vec3 color)
{
	float exposure = 1.5;
	color *= exposure / (1.0 + color / exposure);
	color = pow(color, INV_GAMMA);
	return color;
}

vec3 lumaBasedReinhardToneMapping(vec3 color)
{
	float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
	float toneMappedLuma = luma / (1. + luma);
	color *= toneMappedLuma / luma;
	color = pow(color, INV_GAMMA);
	return color;
}

vec3 whitePreservingLumaBasedReinhardToneMapping(vec3 color)
{
	float white = 2.;
	float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
	float toneMappedLuma = luma * (1. + luma / (white*white)) / (1. + luma);
	color *= toneMappedLuma / luma;
	color = pow(color, INV_GAMMA);
	return color;
}

vec3 RomBinDaHouseToneMapping(vec3 color)
{
    color = exp( -1.0 / ( 2.72*color + 0.15 ) );
	color = pow(color, INV_GAMMA);
	return color;
}

vec3 filmicToneMapping(vec3 color)
{
	color = max(vec3(0,0,0), color - vec3(0.004,0.004,0.004));
	color = (color * (6.2 * color + 0.5)) / (color * (6.2 * color + 1.7) + 0.06);
	return color;
}

vec3 Uncharted2ToneMapping(vec3 color)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	float W = 11.2;
	//float exposure = 2.;
	//color *= exposure;
	color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
	float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
	color /= white;
	color = pow(color, vec3(1. / GAMMA, 1. / GAMMA, 1. / GAMMA));
	return color;
}

shader PreviewPS
{
	vec3 applyExposure(vec3 c) {
		float expScale = exp2(ColorSpaceScale);
		return c * expScale;
	}

	vec4 CalcPreviewColor(vec4 color) {
		if (ColorSpace == 1) {
			color.xyz = linear_to_srgb_float_v3(color.xyz);
		} else if (ColorSpace == 2) {
			color.xyz = applyExposure(color.xyz);

			if (ToneMapMode == 1) {
				color.xyz = linearToneMapping(color.xyz);
			} else if (ToneMapMode == 2) {
				color.xyz = simpleReinhardToneMapping(color.xyz);
			} else if (ToneMapMode == 3) {
				color.xyz = lumaBasedReinhardToneMapping(color.xyz);
			} else if (ToneMapMode == 4) {
				color.xyz = whitePreservingLumaBasedReinhardToneMapping(color.xyz);
			} else if (ToneMapMode == 5) {
				color.xyz = RomBinDaHouseToneMapping(color.xyz);
			} else if (ToneMapMode == 6) {
				color.xyz = filmicToneMapping(color.xyz);
			} else if (ToneMapMode == 7) {
				color.xyz = Uncharted2ToneMapping(color.xyz);
			}
		}

		color *= ColorFilter;
		if (ColorFilter.a == 0.0) 
			color.a = 1;

		bool singleChannelMode = abs(dot(ColorFilter, vec4(1,1,1,1)) - 1.0) < 0.001;
		if (singleChannelMode)
		{
			color = dot(color, vec4(1,1,1,1)).xxx1;
			color.a = 1;
		}

		return color;
	}
}

export shader CanvasVS; // use standard shader

export shader CustomCanvasPS : CanvasPS, PreviewPS
{
	void main()
	{
		vec4 color = textureLod(PreviewTexture, CanvasUV, MipIndex);

		color = CalcPreviewColor(color);

		gl_Target0 = color * CalcScissorMask(CanvasPosition);
	}
}

//--