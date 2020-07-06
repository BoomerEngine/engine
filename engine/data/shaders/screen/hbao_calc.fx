/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
***/

/* Copyright (c) 2014-2018, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <math.h>

descriptor HBAOCalcParams
{
	ConstantBuffer
	{ 
        float RadiusToScreen;        // radius
        float R2;     // 1/radius
        float NegInvR2;     // radius * radius
        float NDotVBias;

        vec2 InvFullResolution;
        vec2 InvQuarterResolution;

        float AOMultiplier;
        float PowExponent;
        uvec2 ScreenSize;

        vec4 ProjInfo;
        vec2 ProjScale;
        int projOrtho;
        int _pad1;

        vec4[16] float2Offsets;
        vec4[16] jitters;
	}

	Texture2D LinearDepthTexture;
    attribute(uav, format=rgba8) Texture2D GlobalShadowAOMask;
}

export shader GenerateCS
{	
	vec3 UVToView(vec2 uv, float eye_z)
	{
		return vec3((uv * ProjInfo.xy + ProjInfo.zw) * eye_z, eye_z);
	}

    vec3 FetchViewPos(ivec2 pos)
    {
        pos = clamp(pos, ivec2(0), ScreenSize - ivec2(1));
        float ViewDepth = LinearDepthTexture[pos].x;
        return UVToView(pos * InvFullResolution, ViewDepth);
    }

    vec3 MinDiff(vec3 P, vec3 Pr, vec3 Pl)
    {
        vec3 V1 = Pr - P;
        vec3 V2 = P - Pl;
        return (dot(V1,V1) < dot(V2,V2)) ? V1 : V2;
    }

    vec3 ReconstructNormal(ivec2 pos, vec3 P)
    {
        vec3 Pr = FetchViewPos(pos + ivec2(1, 0));
        vec3 Pl = FetchViewPos(pos + ivec2(-1, 0));
        vec3 Pt = FetchViewPos(pos + ivec2(0, 1));
        vec3 Pb = FetchViewPos(pos + ivec2(0, -1));
        return normalize(cross(MinDiff(P, Pr, Pl), MinDiff(P, Pt, Pb)));
    }

    //--

    float Falloff(float DistanceSquare)
    {
        // 1 scalar mad instruction
        return DistanceSquare * NegInvR2 + 1.0;
    }

    // P = view-space position at the kernel center
    // N = view-space normal at the kernel center
    // S = view-space position of the current sample
    float ComputeAO(vec3 P, vec3 N, vec3 S)
    {
      vec3 V = S - P;
      float VdotV = dot(V, V);
      float NdotV = dot(N, V) * 1.0 / sqrt(VdotV);

      // Use saturate(x) instead of max(x,0.f) because that is faster on Kepler
      return clamp(NdotV - NDotVBias,0,1) * clamp(Falloff(VdotV),0,1);
    }

    vec2 RotateDirection(vec2 Dir, vec2 CosSin)
    {
        return vec2(Dir.x * CosSin.x - Dir.y * CosSin.y,
                  Dir.x * CosSin.y + Dir.y * CosSin.x);
    }

    vec4 GetJitter(ivec2 pos)
    {
        // (cos(Alpha),sin(Alpha),rand1,rand2)
        int ofs = (pos.x & 3) + (4 * (pos.y & 3));
        return jitters[ofs];
    }

    const float  NUM_STEPS = 4;
    const float  NUM_DIRECTIONS = 8; // texRandom/g_Jitter initialization depends on this

    float ComputeCoarseAO(ivec2 pos, float RadiusPixels, vec4 Rand, vec3 ViewPosition, vec3 ViewNormal)
    {
        // Divide by NUM_STEPS+1 so that the farthest samples are not fully attenuated
        float StepSizePixels = RadiusPixels / (NUM_STEPS + 1);

        const float Alpha = 2.0 * PI / NUM_DIRECTIONS;
        float AO = 0;

        for (float DirectionIndex = 0; DirectionIndex < NUM_DIRECTIONS; DirectionIndex += 1)
        {
            float Angle = Alpha * DirectionIndex;

            // Compute normalized 2D direction
            vec2 Direction = RotateDirection(vec2(cos(Angle), sin(Angle)), Rand.xy);

            // Jitter starting sample within the first step
            float RayPixels = (Rand.z * StepSizePixels + 1.0);
            for (float StepIndex = 0; StepIndex < NUM_STEPS; StepIndex += 1)
            {
                //vec2 SnappedUV = round(RayPixels * Direction) * control.InvFullResolution + FullResUV;
                ivec2 SnappedPos = ivec2(round(RayPixels * Direction)) + pos;
                vec3 S = FetchViewPos(SnappedPos);
                RayPixels += StepSizePixels;
                AO += ComputeAO(ViewPosition, ViewNormal, S);
            }
        }

        AO *= AOMultiplier / (NUM_DIRECTIONS * NUM_STEPS);
        return clamp(1.0 - AO * 2.0,0,1);
    }

    //--

    void WriteAO(ivec2 pos, float val)
    {
        vec4 data = GlobalShadowAOMask[pos];
        data.y = val;
        GlobalShadowAOMask[pos] = data;
    }

	attribute(local_size_x=8, local_size_y=8)
	void main()
	{
		uvec2 pos = gl_GlobalInvocationID.xy;
        vec3 ViewPosition = FetchViewPos(pos);

        // Reconstruct view-space normal from nearest neighbors
        vec3 ViewNormal = -ReconstructNormal(pos, ViewPosition);

        // Compute projection of disk of radius control.R into screen space
        float RadiusPixels = (RadiusToScreen / ViewPosition.z);// * 2.0;

        // Get jitter vector for the current full-res pixel
        vec4 Rand = GetJitter(pos);

        // Compute AO
        float ao = ComputeCoarseAO(pos, RadiusPixels, Rand, ViewPosition, ViewNormal);

        // write to target
        WriteAO(pos, pow(ao, PowExponent * 1));
	}
}
