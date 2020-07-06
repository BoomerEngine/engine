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

descriptor ReconstructNormalParams
{
	ConstantBuffer
	{ 
		uvec2 ScreenSize;
		vec2 InvScreenSize;
		vec4 ProjInfo;
	}

	Texture2D LinearDepthTexture;
    attribute(uav, format=rgba8) Texture2D ReconstructedViewNormal;
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
        return UVToView(pos * InvScreenSize, ViewDepth);
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

	attribute(local_size_x=8, local_size_y=8)
	void main()
	{
		uvec2 pos = gl_GlobalInvocationID.xy;
        vec3 P = FetchViewPos(pos);
        vec3 N = ReconstructNormal(pos, P);
        ReconstructedViewNormal[pos] = (0.5*N + 0.5).xyz0;
	}
}
