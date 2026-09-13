/*
 * src/Graphics/Effects/Lighting/SSGI.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2026 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
 *
 * Emeraude-Engine is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * Emeraude-Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Emeraude-Engine; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Complete project and additional information can be found at :
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "SSGI.hpp"

/* STL inclusions. */
#include <string>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Graphics/ViewMatricesInterface.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/PipelineLayout.hpp"

namespace
{
	using namespace EmEn;

	/* SSGI sky-visibility pass: a GTAO horizon search (Jimenez, Wu, Pesce, Jarabo, "Practical
	 * Realtime Strategies for Accurate Indirect Occlusion", SIGGRAPH 2016 courses; implementation
	 * reference: Intel XeGTAO, MIT licence, https://github.com/GameTechDev/XeGTAO — the slice
	 * integral, the arc formula and the bent-normal integrals below are transcribed from it).
	 *
	 * WHAT IT IS FOR. The traced lane measures the sky with rays: RTGI casts each hemisphere ray to
	 * the FAR PLANE, and a ray that escapes returns sky radiance. The screen-space lane had no sky
	 * term at all, so the scene kept the raster's own diffuse IBL leg at full weight — the
	 * irradiance cubemap applied as if every surface saw the whole sky. Measured on `sponza`
	 * (2026-09-13, upper gallery, exposure PINNED at f/11 - 1/250 s - ISO 100, 2880x1620, 0 VUID):
	 * 11.2/255 mean on the traced lane against 52.8 on the screen-space one, the galleries lit as
	 * if the courtyard had no roof. With this pass: 29.2. The acceptance table (including the
	 * open-sky case, where the sky term equals the raster leg it replaces to the bit) is in
	 * src/Graphics/AGENTS.md, section "The screen-space sky visibility".
	 *
	 * WHAT IT COMPUTES. Per pixel, N slices (planes through the view vector) are walked on BOTH
	 * sides; each depth sample updates that side's HORIZON (the highest angle from the view vector
	 * an occluder reaches). The arc between the two horizons, integrated against the cosine lobe of
	 * the surface normal, is the visibility V; the mean direction of that arc is the BENT NORMAL.
	 * The trace pass then samples the baked irradiance cubemap along the bent normal and scales it
	 * by V — the sky irradiance the pixel actually receives.
	 *
	 * ⚠️ An occluder OFF SCREEN is invisible to this search (a roof behind the camera). That is the
	 * structural limit of the lane, not a defect of this pass: out-of-frame samples are skipped
	 * rather than clamped to the border, because a clamped sampler recycles the edge depth and
	 * paints a false occlusion band along the frame (the trap the SSAO kernel documents).
	 *
	 * Descriptor set 0 (input textures — per-frame):
	 *   binding 0: depth texture
	 *   binding 1: normals texture (view space)
	 */
	constexpr auto SSGIHorizonFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
/* xyz = WORLD-space bent normal, w = cosine-weighted visibility. */
layout(location = 0) out vec4 outBentVisibility;

layout(set = 0, binding = 0) uniform sampler2D depthTex;
layout(set = 0, binding = 1) uniform sampler2D normalTex;

layout(push_constant) uniform PushConstants
{
	vec4 invViewCol0;	/* xyz = inverse view rotation column 0, w = search radius in world units. */
	vec4 invViewCol1;	/* xyz = inverse view rotation column 1, w = falloff range (fraction of the radius). */
	vec4 invViewCol2;	/* xyz = inverse view rotation column 2, w = animated-noise frame index (< 0 = frozen). */
	float nearPlane;
	float farPlane;
	float tanHalfFovY;
	float aspectRatio;
	uint sliceCount;
	uint stepCount;
};

const float PI = 3.14159265;
const float PI_HALF = 1.57079633;

/* Linearize depth from [0,1] range (Vulkan [0,1] depth convention). */
float linearizeDepth (float depth)
{
	return (nearPlane * farPlane) / (farPlane - depth * (farPlane - nearPlane));
}

/* Reconstruct the RECONSTRUCTION-space position from UV and depth: view space with Z negated
 * (linearDepth is positive, view-space Z is negative in front of the camera), which is the space
 * the whole screen-space family works in.
 * NOTE: tanHalfFovY is SIGNED and carries the projection's Y direction. Do NOT wrap it in abs(). */
vec3 reconstructPosition (vec2 uv, float depth)
{
	float linearZ = linearizeDepth(depth);
	vec2 ndc = uv * 2.0 - 1.0;
	float t = tanHalfFovY;
	return vec3(ndc * vec2(abs(t) * aspectRatio, t) * linearZ, linearZ);
}

/* PCG integer hash -> decorrelated white noise from integer pixel coordinates (same generator as
 * the trace pass: a fract(sin(dot(...))) hash beats against float precision and freezes into a
 * grid the denoiser cannot remove). */
uint pcgHash (uint v)
{
	v = v * 747796405u + 2891336453u;
	uint s = ((v >> ((v >> 28u) + 4u)) ^ v) * 277803737u;
	return (s >> 22u) ^ s;
}

vec2 hash2 (uvec2 p)
{
	uint h = pcgHash(p.x + pcgHash(p.y));
	return vec2(float(h & 0xffffu), float((h >> 16u) & 0xffffu)) * (1.0 / 65535.0);
}

void main()
{
	float centerDepth = texture(depthTex, vUV).r;

	/* Background: nothing to occlude, and the trace pass skips those pixels anyway. */
	if (centerDepth >= 1.0)
	{
		outBentVisibility = vec4(0.0, 0.0, 0.0, 1.0);

		return;
	}

	vec3 rawN = texture(normalTex, vUV).rgb;

	if (dot(rawN, rawN) < 0.0001)
	{
		outBentVisibility = vec4(0.0, 0.0, 0.0, 1.0);

		return;
	}

	mat3 invViewRot = mat3(invViewCol0.xyz, invViewCol1.xyz, invViewCol2.xyz);
	float radius = invViewCol0.w;
	float falloffFraction = invViewCol1.w;
	float noiseFrameIndex = invViewCol2.w;

	vec3 P = reconstructPosition(vUV, centerDepth);
	/* Reconstruction space: the view-space normal with Z negated. */
	vec3 N = normalize(vec3(rawN.x, rawN.y, -rawN.z));
	/* The direction from the surface back to the eye — the axis every slice turns around. */
	vec3 V = normalize(-P);

	/* Distance falloff of an occluder, XeGTAO's affine form. ⚠️ The fraction is SMALL here on
	 * purpose (0.2 by default, not XeGTAO's 0.615): a far roof must occlude the sky at full
	 * strength, and the fade exists only to keep geometry from popping as it crosses the radius. */
	float falloffRange = max(falloffFraction * radius, 0.0001);
	float falloffMul = -1.0 / falloffRange;
	float falloffAdd = (radius * (1.0 - falloffFraction)) / falloffRange + 1.0;

	vec2 noise = hash2(uvec2(gl_FragCoord.xy));

	if (noiseFrameIndex >= 0.0)
	{
		/* R2 low-discrepancy sequence (Roberts 2018) — the same temporal decorrelation the trace
		 * uses, so the shared denoiser AVERAGES the estimator error instead of freezing it. */
		noise = fract(noise + noiseFrameIndex * vec2(0.7548776662, 0.5698402909));
	}

	float visibility = 0.0;
	vec3 bentNormal = vec3(0.0);

	for (uint slice = 0u; slice < sliceCount; ++slice)
	{
		float phi = (float(slice) + noise.x) / float(sliceCount) * PI;
		vec2 omega = vec2(cos(phi), sin(phi));

		/* The screen direction, mapped into reconstruction space at CONSTANT depth: a UV step
		 * (du, dv) moves the point by (du * 2|t| * aspect * z, dv * 2t * z, 0), so the plane
		 * direction is that vector normalized — and the inverse mapping gives the UV step of one
		 * world unit, uvPerUnit below. The Y component carries the SIGN of tanHalfFovY: get it
		 * wrong and the two horizons of a slice are attributed to the wrong sides. */
		vec2 dirScale = vec2(omega.x * abs(tanHalfFovY) * aspectRatio, omega.y * tanHalfFovY);
		float dirLength = length(dirScale);

		if (dirLength < 0.000001)
		{
			continue;
		}

		vec3 directionVec = vec3(dirScale / dirLength, 0.0);
		float uvPerUnit = 1.0 / (2.0 * dirLength * P.z);

		/* The slice frame: (V, sliceTangent) spans the plane, axisVec is its normal.
		 * ⚠️ sliceTangent is NORMALIZED, where XeGTAO rotates the raw screen direction instead —
		 * an approximation that is exact only at the centre of the frame. The arc integrals below
		 * assume an ORTHONORMAL frame. */
		vec3 sliceTangent = directionVec - dot(directionVec, V) * V;
		float sliceTangentLength = length(sliceTangent);

		if (sliceTangentLength < 0.0001)
		{
			continue;
		}

		sliceTangent /= sliceTangentLength;

		vec3 axisVec = cross(sliceTangent, V);
		/* The normal projected into the slice plane, and its angle to the view vector. */
		vec3 projectedNormal = N - axisVec * dot(N, axisVec);
		float projectedNormalLength = length(projectedNormal);

		if (projectedNormalLength < 0.0001)
		{
			continue;
		}

		float cosNorm = clamp(dot(projectedNormal, V) / projectedNormalLength, -1.0, 1.0);
		float n = (dot(sliceTangent, projectedNormal) < 0.0 ? -1.0 : 1.0) * acos(cosNorm);
		float sinN = sin(n);

		/* Horizons start on the TANGENT PLANE (nothing occludes), one per side. */
		float lowHorizonCos0 = cos(n + PI_HALF);
		float lowHorizonCos1 = cos(n - PI_HALF);
		float horizonCos0 = lowHorizonCos0;
		float horizonCos1 = lowHorizonCos1;

		for (uint step = 0u; step < stepCount; ++step)
		{
			/* Quadratic step distribution: detail in the near field, reach in the far one. */
			float stepNorm = (float(step) + noise.y) / float(stepCount);
			vec2 uvOffset = omega * (radius * stepNorm * stepNorm * uvPerUnit);

			vec2 sampleUV0 = vUV + uvOffset;
			vec2 sampleUV1 = vUV - uvOffset;

			/* ⚠️ SKIPPED, never clamped — see the header. */
			if (all(greaterThanEqual(sampleUV0, vec2(0.0))) && all(lessThanEqual(sampleUV0, vec2(1.0))))
			{
				vec3 delta = reconstructPosition(sampleUV0, texture(depthTex, sampleUV0).r) - P;
				float deltaLength = length(delta);

				if (deltaLength > 0.00001)
				{
					float weight = clamp(deltaLength * falloffMul + falloffAdd, 0.0, 1.0);
					float sampleCos = mix(lowHorizonCos0, dot(delta / deltaLength, V), weight);
					horizonCos0 = max(horizonCos0, sampleCos);
				}
			}

			if (all(greaterThanEqual(sampleUV1, vec2(0.0))) && all(lessThanEqual(sampleUV1, vec2(1.0))))
			{
				vec3 delta = reconstructPosition(sampleUV1, texture(depthTex, sampleUV1).r) - P;
				float deltaLength = length(delta);

				if (deltaLength > 0.00001)
				{
					float weight = clamp(deltaLength * falloffMul + falloffAdd, 0.0, 1.0);
					float sampleCos = mix(lowHorizonCos1, dot(delta / deltaLength, V), weight);
					horizonCos1 = max(horizonCos1, sampleCos);
				}
			}
		}

		/* The two horizon angles, signed around the view vector and clamped to the hemisphere of
		 * the projected normal. */
		float h0 = n + clamp(-acos(clamp(horizonCos1, -1.0, 1.0)) - n, -PI_HALF, PI_HALF);
		float h1 = n + clamp(acos(clamp(horizonCos0, -1.0, 1.0)) - n, -PI_HALF, PI_HALF);

		/* Visibility: the cosine-weighted arc integral of the unoccluded part of the slice. */
		float iarc0 = (cosNorm + 2.0 * h0 * sinN - cos(2.0 * h0 - n)) * 0.25;
		float iarc1 = (cosNorm + 2.0 * h1 * sinN - cos(2.0 * h1 - n)) * 0.25;
		visibility += projectedNormalLength * (iarc0 + iarc1);

		/* Bent normal: the same arc, integrated as a DIRECTION — t0 along the slice tangent, t1
		 * along the view vector (XeGTAO's closed forms). */
		float t0 = (6.0 * sin(h0 - n) - sin(3.0 * h0 - n) + 6.0 * sin(h1 - n) - sin(3.0 * h1 - n) + 16.0 * sinN - 3.0 * (sin(h0 + n) + sin(h1 + n))) / 12.0;
		float t1 = (-cos(3.0 * h0 - n) - cos(3.0 * h1 - n) + 8.0 * cos(n) - 3.0 * (cos(h0 + n) + cos(h1 + n))) / 12.0;
		bentNormal += (sliceTangent * t0 + V * t1) * projectedNormalLength;
	}

	visibility = clamp(visibility / float(sliceCount), 0.0, 1.0);

	/* Reconstruction space -> view space (Z back) -> world space, for the irradiance lookup. */
	vec3 bentNormalWorld = invViewRot * vec3(N.x, N.y, -N.z);

	if (dot(bentNormal, bentNormal) > 0.00000001)
	{
		bentNormal = normalize(bentNormal);
		bentNormalWorld = invViewRot * vec3(bentNormal.x, bentNormal.y, -bentNormal.z);
	}

	/* Safety net: a non-finite value here would reach the trace as a NaN irradiance and black out
	 * the pixel through the whole denoiser history. */
	if (any(isnan(bentNormalWorld)) || any(isinf(bentNormalWorld)) || isnan(visibility) || isinf(visibility))
	{
		outBentVisibility = vec4(0.0, 0.0, 0.0, 1.0);

		return;
	}

	outBentVisibility = vec4(bentNormalWorld, visibility);
}
)GLSL";

	/* SSGI trace pass: one-bounce diffuse indirect lighting via screen-space ray marching.
	 * For each pixel, casts cosine-weighted hemisphere rays through the depth buffer.
	 * On hit, samples the scene color at the hit UV to produce indirect radiance
	 * (color bleeding). This is the screen-space approximation of RTGI.
	 *
	 * Descriptor set 0 (input textures — per-frame):
	 *   binding 0: depth texture
	 *   binding 1: normals texture
	 *   binding 2: scene color texture (HDR, for bounce color sampling)
	 *   binding 3: sky visibility (bent normal + V), the previous pass of this same effect —
	 *              declared by the EMEN_SSGI_SKY_VISIBILITY variant only
	 *
	 * Descriptor set 1 (bindless textures, sky-visibility variant only — BindlessTextureManager):
	 *   binding 3: samplerCube[] whose reserved slot 1 holds the scene's baked irradiance cubemap
	 *
	 * ⚠️ The source below carries NO `#version`: create() prepends it, followed by the variant's
	 * define and extension. A `#extension` must precede every non-preprocessor token, so it cannot
	 * be moved inside the `#ifdef` blocks below.
	 */
	constexpr auto SSGITraceFragmentShaderBody = R"GLSL(
layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outIndirect;

layout(set = 0, binding = 0) uniform sampler2D depthTex;
layout(set = 0, binding = 1) uniform sampler2D normalTex;
layout(set = 0, binding = 2) uniform sampler2D colorTex;

#ifdef EMEN_SSGI_SKY_VISIBILITY
layout(set = 0, binding = 3) uniform sampler2D horizonTex;
layout(set = 1, binding = 3) uniform samplerCube texturesCube[];

/* Reserved bindless slot of the scene's baked irradiance cubemap (BindlessTextureManager).
 * It stores E/pi, normalized, and is parked on the engine's BLACK default when the scene derives
 * no lighting from a sky — which is why the sky luminance below doubles as the "there is a sky"
 * flag, exactly as it does for RTGI. */
const uint IrradianceCubemapSlot = 1u;
#endif

layout(push_constant) uniform PushConstants
{
	float texelSizeX;
	float texelSizeY;
	float nearPlane;
	float farPlane;
	float tanHalfFovY;
	float aspectRatio;
	float maxDistance;
	float thickness;
	uint sampleCount;
	uint stepCount;
	float noiseFrameIndex;	/* R2 sequence index; < 0 = frozen pattern. */
	float skyLuminance;		/* Scale of the irradiance cubemap, in nits (0 = no sky). */
};

/* Linearize depth from [0,1] range (Vulkan [0,1] depth convention). */
float linearizeDepth (float depth)
{
	return (nearPlane * farPlane) / (farPlane - depth * (farPlane - nearPlane));
}

/* Reconstruct view-space position from UV and depth. */
vec3 reconstructPosition (vec2 uv, float depth)
{
	float linearZ = linearizeDepth(depth);
	vec2 ndc = uv * 2.0 - 1.0;
	float t = tanHalfFovY;
	return vec3(ndc * vec2(abs(t) * aspectRatio, t) * linearZ, linearZ);
}

/* Project view-space position back to screen UV. */
vec2 projectToUV (vec3 viewPos)
{
	float t = tanHalfFovY;
	vec2 ndc = viewPos.xy / (viewPos.z * vec2(abs(t) * aspectRatio, t));
	return ndc * 0.5 + 0.5;
}

/* PCG integer hash → decorrelated white noise from integer pixel coordinates (same
 * upgrade as RTGI: the former fract(sin(dot(...))) hash has float-precision beating and
 * produced a fixed grid/banding pattern the spatial denoiser cannot remove). */
uint pcgHash (uint v)
{
	v = v * 747796405u + 2891336453u;
	uint s = ((v >> ((v >> 28u) + 4u)) ^ v) * 277803737u;
	return (s >> 22u) ^ s;
}

vec2 hash2 (uvec2 p)
{
	uint h = pcgHash(p.x + pcgHash(p.y));
	return vec2(float(h & 0xffffu), float((h >> 16u) & 0xffffu)) * (1.0 / 65535.0);
}

/* Generate a cosine-weighted hemisphere sample direction. */
vec3 hemispherePoint (uint i, vec2 noise)
{
	float fi = float(i);
	float angle = fi * 2.399963 + noise.x * 6.283185;
	float r = sqrt((fi + 0.5) / float(sampleCount));
	float z = sqrt(1.0 - r * r);
	return vec3(cos(angle) * r, sin(angle) * r, z);
}

/* Screen-edge fade: 0 at edges, 1 at center. */
float screenEdgeFade (vec2 uv)
{
	vec2 fade = smoothstep(vec2(0.0), vec2(0.05), uv)
			  * smoothstep(vec2(0.0), vec2(0.05), vec2(1.0) - uv);
	return fade.x * fade.y;
}

void main()
{
	float centerDepth = texture(depthTex, vUV).r;

	/* Skip far-plane fragments. */
	if (centerDepth >= 1.0)
	{
		outIndirect = vec4(0.0);
		return;
	}

	vec3 centerPos = reconstructPosition(vUV, centerDepth);

	/* Read view-space normal from MRT normal buffer. */
	vec3 rawN = texture(normalTex, vUV).rgb;

	if (dot(rawN, rawN) < 0.0001)
	{
		outIndirect = vec4(0.0);
		return;
	}

	/* Convert to reconstruction space (Z negated: linearDepth is positive,
	 * view-space Z is negative for objects in front of the camera). */
	vec3 normal = normalize(vec3(rawN.x, rawN.y, -rawN.z));

	/* Per-pixel random rotation to break banding. Temporal decorrelation: advance the
	 * rotation every frame along the R2 low-discrepancy sequence (Roberts 2018) so the
	 * GIDenoiser resolve AVERAGES the estimator error instead of freezing it as a static
	 * pattern (stable outliers read as "converged signal" the variance guide protects).
	 * The index is negative when the temporal chain is off: animated noise without
	 * accumulation boils. */
	vec2 noiseVec = hash2(uvec2(gl_FragCoord.xy));

	if (noiseFrameIndex >= 0.0)
	{
		noiseVec = fract(noiseVec + noiseFrameIndex * vec2(0.7548776662, 0.5698402909));
	}

	/* Build a tangent-space basis around the view-space normal.
	 * Robust construction: pick an up vector not parallel to the normal, then cross (same method
	 * as RTGI). The previous noise-based Gram-Schmidt degenerated to normalize(0) = NaN whenever the
	 * noise vector aligned with the normal (side walls seen edge-on) — the NaN then propagated
	 * through the apply pass (color += NaN) and blacked out whole surfaces. The per-sample random
	 * rotation is already provided by hemispherePoint() via noiseVec, so a deterministic basis here
	 * is fine. */
	vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(up, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);

	/* Compute stride length from max distance and step count. */
	float strideLen = maxDistance / float(stepCount);

	/* Adaptive stride: scale with depth so distant pixels cover more ground. */
	float adaptiveStride = strideLen * max(1.0, centerPos.z * 0.1);

	/* Accumulate indirect radiance. */
	vec3 indirectLight = vec3(0.0);

	for (uint i = 0u; i < sampleCount; ++i)
	{
		vec3 sampleDir = TBN * hemispherePoint(i, noiseVec);

		/* Ensure the sample direction is in the hemisphere of the normal. */
		if (dot(sampleDir, normal) < 0.0)
		{
			sampleDir = -sampleDir;
		}

		/* Ray march through the depth buffer. Instead of requiring the ray to LAND inside a thin
		 * thickness window (which a coarse march over-steps — the reason SSGI missed adjacent visible
		 * light while SSR, with 128 fine steps, did not), we detect the FRONT->BEHIND crossing
		 * (prevDiff < 0, diff > 0) and binary-refine the intersection. This catches surfaces the
		 * step size would otherwise skip, at a fraction of SSR's per-ray budget. */
		bool hit = false;
		vec2 hitUV = vec2(0.0);
		float hitDist = 0.0;

		float prevDiff = -1.0;
		vec3 prevRayPos = centerPos;

		for (uint s = 1u; s <= stepCount; ++s)
		{
			vec3 rayPos = centerPos + sampleDir * adaptiveStride * float(s);

			/* Project to screen space. */
			vec2 sampleUV = projectToUV(rayPos);

			/* Out of screen bounds. */
			if (any(lessThan(sampleUV, vec2(0.0))) || any(greaterThan(sampleUV, vec2(1.0))))
			{
				break;
			}

			/* Compare depth at the projected position. */
			float sampleDepth = linearizeDepth(texture(depthTex, sampleUV).r);
			float diff = rayPos.z - sampleDepth;

			/* Adaptive thickness based on distance (distant surfaces need larger threshold). */
			float adaptiveThick = thickness * max(1.0, sampleDepth * 0.05);

			/* Crossing: the ray went from in front of the surface to behind it. */
			if (prevDiff < 0.0 && diff > 0.0)
			{
				/* Binary-refine the intersection between the last two samples. */
				vec3 lo = prevRayPos;
				vec3 hi = rayPos;

				for (uint b = 0u; b < 8u; ++b)
				{
					vec3 mid = 0.5 * (lo + hi);
					float midDepth = linearizeDepth(texture(depthTex, projectToUV(mid)).r);

					if (mid.z - midDepth > 0.0)
					{
						hi = mid;
					}
					else
					{
						lo = mid;
					}
				}

				vec2 refUV = projectToUV(hi);
				float refDepth = linearizeDepth(texture(depthTex, refUV).r);

				/* Reject false crossings (silhouette / background gap): after refinement the ray
				 * must sit just behind the surface, not far behind it. */
				if ((hi.z - refDepth) < adaptiveThick)
				{
					hitUV = refUV;
					hitDist = length(hi - centerPos);
					hit = true;
					break;
				}
			}

			prevDiff = diff;
			prevRayPos = rayPos;
		}

		if (hit)
		{
			/* Sample scene color at the hit point (the indirect bounce). */
			vec3 hitColor = texture(colorTex, hitUV).rgb;

			/* Range fade — the RTGI curve, verbatim (Sep 2026): the transfer is already governed by
			 * the solid angle, so a fade proportional to the distance is NOT physical. This lane kept
			 * the linear `1 - hitDist / maxDistance` after RTGI retired it, halving every bounce found
			 * at mid-range; the two occupants of the IndirectDiffuse slot then differed by their
			 * attenuation as well as by their technique, and an A/B compared both at once. The fade
			 * only smooths the LAST FIFTH of the range, whose sole purpose is to keep geometry from
			 * popping as it crosses the maxDistance boundary. */
			float distFade = 1.0 - smoothstep(maxDistance * 0.8, maxDistance, hitDist);

			/* Screen edge fade at hit point to avoid artifacts at screen borders. */
			float edgeFade = screenEdgeFade(hitUV);

			/* hitColor is already the hit surface's outgoing radiance (the lit colour buffer),
			 * i.e. L_i. With cosine-weighted sampling the diffuse estimate is albedo * mean(L_i)
			 * (the receiver albedo is applied in the apply pass / omitted for white surfaces);
			 * there must be NO extra 1/PI here — dividing by PI made SSGI ~3.14x too dark. */
			indirectLight += hitColor * distFade * edgeFade;
		}
	}

	/* Normalize by sample count. Intensity is applied in the apply pass. */
	indirectLight = indirectLight / float(sampleCount);

#ifdef EMEN_SSGI_SKY_VISIBILITY
	/* THE SKY, the other half of the indirect diffuse — the half this lane used to leave to the
	 * raster, unoccluded (see the sky-visibility pass above). Same demodulated convention as the
	 * bounce: this is an irradiance E/pi in nits, the receiver's albedo is applied ONCE, at the
	 * combine. It matches the raster leg it takes over term for term — the same cubemap, the same
	 * luminance scale — with the visibility the raster could not measure.
	 * ⚠️ The bent normal is already WORLD space and the cubemap is sampled RAW (Y-up convention,
	 * no negation anywhere — same contract as the skybox and the material reflections). */
	vec4 skyVisibility = texture(horizonTex, vUV);

	indirectLight += texture(texturesCube[nonuniformEXT(IrradianceCubemapSlot)], skyVisibility.xyz).rgb * skyLuminance * skyVisibility.w;
#endif

	/* Safety net: never let a NaN/Inf or negative value reach the apply pass (color += gi would
	 * otherwise black out or blow up the pixel). */
	if (any(isnan(indirectLight)) || any(isinf(indirectLight)))
	{
		indirectLight = vec3(0.0);
	}

	indirectLight = max(indirectLight, vec3(0.0));

	outIndirect = vec4(indirectLight, 1.0);
}
)GLSL";

}

namespace EmEn::Graphics::Effects::Lighting
{
	using namespace Base;
	using namespace Vulkan;
	using namespace Saphir;

	bool
	SSGI::create (uint32_t width, uint32_t height) noexcept
	{
		auto & renderer = this->renderer();

		auto & settings = renderer.primaryServices().settings();

		/* User-facing parameters, engine-wide and persisted in the settings file.
		 * These override any constructor-provided values. */
		m_parameters.maxDistance = settings.getOrSetDefault< float >(GraphicsPPIndirectDiffuseMaxDistanceKey, DefaultGraphicsPPIndirectDiffuseMaxDistance);
		m_parameters.intensity = settings.getOrSetDefault< float >(GraphicsPPIndirectDiffuseIntensityKey, DefaultGraphicsPPIndirectDiffuseIntensity);
		m_parameters.thickness = settings.getOrSetDefault< float >(GraphicsPPIndirectDiffuseSSThicknessKey, DefaultGraphicsPPIndirectDiffuseSSThickness);
		m_parameters.sampleCount = settings.getOrSetDefault< uint32_t >(GraphicsPPIndirectDiffuseSampleCountKey, DefaultGraphicsPPIndirectDiffuseSampleCount);
		m_parameters.stepCount = settings.getOrSetDefault< uint32_t >(GraphicsPPIndirectDiffuseSSStepCountKey, DefaultGraphicsPPIndirectDiffuseSSStepCount);
		m_parameters.skyVisibilityEnabled = settings.getOrSetDefault< bool >(GraphicsPPIndirectDiffuseSSSkyVisibilityEnabledKey, DefaultGraphicsPPIndirectDiffuseSSSkyVisibilityEnabled);
		m_parameters.skyVisibilityRadius = settings.getOrSetDefault< float >(GraphicsPPIndirectDiffuseSSSkyVisibilityRadiusKey, DefaultGraphicsPPIndirectDiffuseSSSkyVisibilityRadius);
		m_parameters.skyVisibilityFalloffRange = settings.getOrSetDefault< float >(GraphicsPPIndirectDiffuseSSSkyVisibilityFalloffRangeKey, DefaultGraphicsPPIndirectDiffuseSSSkyVisibilityFalloffRange);
		m_parameters.skyVisibilitySliceCount = settings.getOrSetDefault< uint32_t >(GraphicsPPIndirectDiffuseSSSkyVisibilitySliceCountKey, DefaultGraphicsPPIndirectDiffuseSSSkyVisibilitySliceCount);
		m_parameters.skyVisibilityStepCount = settings.getOrSetDefault< uint32_t >(GraphicsPPIndirectDiffuseSSSkyVisibilityStepCountKey, DefaultGraphicsPPIndirectDiffuseSSSkyVisibilityStepCount);
		m_parameters.depthSigma = settings.getOrSetDefault< float >(GraphicsPPIndirectDiffuseDepthSigmaKey, DefaultGraphicsPPIndirectDiffuseDepthSigma);
		m_parameters.normalSigma = settings.getOrSetDefault< float >(GraphicsPPIndirectDiffuseNormalSigmaKey, DefaultGraphicsPPIndirectDiffuseNormalSigma);
		m_parameters.luminanceSigma = settings.getOrSetDefault< float >(GraphicsPPIndirectDiffuseDenoiserLuminanceSigmaKey, DefaultGraphicsPPIndirectDiffuseDenoiserLuminanceSigma);
		m_parameters.atrousIterations = settings.getOrSetDefault< uint32_t >(GraphicsPPIndirectDiffuseDenoiserIterationsKey, DefaultGraphicsPPIndirectDiffuseDenoiserIterations);
		m_parameters.temporalAlpha = settings.getOrSetDefault< float >(GraphicsPPIndirectDiffuseTemporalAlphaKey, DefaultGraphicsPPIndirectDiffuseTemporalAlpha);
		m_parameters.temporalDepthTolerance = settings.getOrSetDefault< float >(GraphicsPPIndirectDiffuseTemporalDepthToleranceKey, DefaultGraphicsPPIndirectDiffuseTemporalDepthTolerance);
		m_parameters.temporalNormalThreshold = settings.getOrSetDefault< float >(GraphicsPPIndirectDiffuseTemporalNormalThresholdKey, DefaultGraphicsPPIndirectDiffuseTemporalNormalThreshold);
		m_parameters.temporalVarianceGamma = settings.getOrSetDefault< float >(GraphicsPPIndirectDiffuseTemporalVarianceGammaKey, DefaultGraphicsPPIndirectDiffuseTemporalVarianceGamma);
		m_parameters.denoiserMaxAccumulation = settings.getOrSetDefault< uint32_t >(GraphicsPPIndirectDiffuseDenoiserMaxAccumulationKey, DefaultGraphicsPPIndirectDiffuseDenoiserMaxAccumulation);
		m_parameters.denoiserDebugView = settings.getOrSetDefault< uint32_t >(GraphicsPPIndirectDiffuseDenoiserDebugViewKey, DefaultGraphicsPPIndirectDiffuseDenoiserDebugView);
		m_parameters.denoiserAccumulationCounter = settings.getOrSetDefault< bool >(GraphicsPPIndirectDiffuseDenoiserAccumulationCounterKey, DefaultGraphicsPPIndirectDiffuseDenoiserAccumulationCounter);
		m_parameters.temporalEnabled = settings.getOrSetDefault< bool >(GraphicsPPIndirectDiffuseTemporalEnabledKey, DefaultGraphicsPPIndirectDiffuseTemporalEnabled);
		m_parameters.temporalNeighborhoodClamp = settings.getOrSetDefault< bool >(GraphicsPPIndirectDiffuseTemporalNeighborhoodClampKey, DefaultGraphicsPPIndirectDiffuseTemporalNeighborhoodClamp);
		m_parameters.temporalAnimatedNoise = settings.getOrSetDefault< bool >(GraphicsPPIndirectDiffuseTemporalAnimatedNoiseKey, DefaultGraphicsPPIndirectDiffuseTemporalAnimatedNoise);

		const auto halfW = (width > 1) ? width / 2 : 1U;
		const auto halfH = (height > 1) ? height / 2 : 1U;

		/* Trace target (half-res, RGBA16F: indirect radiance RGB). */
		if ( !m_traceTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "SSGI_Trace") )
		{
			TraceError{ClassId} << "Failed to create SSGI trace target !";

			return false;
		}

		/* ---- The sky-visibility half of the estimator ----
		 * It needs the bindless table (the baked irradiance cubemap lives in a reserved slot of it).
		 * ⚠️ Missing table = NO sky term, but the effect is still created: the bounce half is the
		 * screen-space lane's fallback and must keep running. The consequence of the flag is the
		 * OWNERSHIP — with no sky term this effect does not claim the indirect diffuse, so the
		 * scene keeps the raster's own diffuse IBL leg and the frame still has a sky. */
		auto bindlessLayout = renderer.bindlessTextureManager().descriptorSetLayout();

		if ( m_parameters.skyVisibilityEnabled && bindlessLayout == nullptr )
		{
			TraceWarning{ClassId} << "The bindless texture table is not available: the screen-space lane runs WITHOUT its sky term, the raster keeps the unoccluded diffuse IBL leg !";
		}

		m_skyVisibilityActive = m_parameters.skyVisibilityEnabled && bindlessLayout != nullptr;

		if ( m_skyVisibilityActive && !m_horizonTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "SSGI_SkyVisibility") )
		{
			TraceError{ClassId} << "Failed to create the SSGI sky-visibility target !";

			return false;
		}

		/* The denoiser component (temporal resolve + moments + à-trous + histories) —
		 * SSGI's FIRST temporal accumulation. */
		m_denoiser.setTemporalEnabled(m_parameters.temporalEnabled);
		m_denoiser.setParameters(GIDenoiser::Parameters{
			.depthSigma = m_parameters.depthSigma,
			.normalSigma = m_parameters.normalSigma,
			.luminanceSigma = m_parameters.luminanceSigma,
			.atrousIterations = m_parameters.atrousIterations,
			.temporalAlpha = m_parameters.temporalAlpha,
			.temporalDepthTolerance = m_parameters.temporalDepthTolerance,
			.temporalNormalThreshold = m_parameters.temporalNormalThreshold,
			.temporalVarianceGamma = m_parameters.temporalVarianceGamma,
			.maxAccumulation = m_parameters.denoiserMaxAccumulation,
			.temporalNeighborhoodClamp = m_parameters.temporalNeighborhoodClamp,
			.temporalAnimatedNoise = m_parameters.temporalAnimatedNoise,
			.accumulationCounter = m_parameters.denoiserAccumulationCounter
		});

		if ( !m_denoiser.create(halfW, halfH) )
		{
			TraceError{ClassId} << "Failed to create the SSGI denoiser component !";

			return false;
		}

		/* ---- Descriptor set layouts (shared) ---- */
		/* The trace reads depth + normals + scene colour, plus the sky visibility when it has one. */
		auto traceInputLayout = this->getInputLayout(m_skyVisibilityActive ? 4 : 3);

		if ( traceInputLayout == nullptr )
		{
			return false;
		}

		/* ---- Pipeline layouts ---- */
		auto & layoutManager = renderer.layoutManager();

		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(traceInputLayout);

			/* Set 1 = the bindless table, where recordFullscreenPass() binds it. */
			if ( m_skyVisibilityActive )
			{
				sets.emplace_back(bindlessLayout);
			}

			m_traceLayout = layoutManager.getPipelineLayout(sets, {VkPushConstantRange{
				.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				.offset = 0,
				.size = sizeof(TracePushConstants)
			}});
		}

		if ( m_traceLayout == nullptr )
		{
			return false;
		}

		if ( m_skyVisibilityActive )
		{
			auto horizonInputLayout = this->getInputLayout(2);

			if ( horizonInputLayout == nullptr )
			{
				return false;
			}

			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(horizonInputLayout);

			m_horizonLayout = layoutManager.getPipelineLayout(sets, {VkPushConstantRange{
				.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				.offset = 0,
				.size = sizeof(HorizonPushConstants)
			}});

			if ( m_horizonLayout == nullptr )
			{
				return false;
			}

			m_horizonPerFrame = this->createPerFrameDescriptorSets(horizonInputLayout, ClassId, "SkyVisibility_DescSet");

			if ( m_horizonPerFrame.empty() )
			{
				return false;
			}
		}

		/* ---- Compile shaders ---- */
		auto & shaderManager = renderer.shaderManager();
		const auto & device = renderer.device();

		const auto vertexModule = this->getFullscreenVertexShader();

		/* ⚠️ The variant is decided HERE, once: the sky block declares a sampler the no-sky pipeline
		 * layout does not carry, and a `#extension` cannot live inside an `#ifdef` block. */
		const std::string traceSource = m_skyVisibilityActive
			? std::string{"#version 450\n#define EMEN_SSGI_SKY_VISIBILITY 1\n#extension GL_EXT_nonuniform_qualifier : require\n"} + SSGITraceFragmentShaderBody
			: std::string{"#version 450\n"} + SSGITraceFragmentShaderBody;

		const auto traceFragment = shaderManager.getShaderModuleFromSourceCode(device, m_skyVisibilityActive ? "SSGI_TraceSky_FS" : "SSGI_Trace_FS", ShaderType::FragmentShader, traceSource);

		if ( vertexModule == nullptr || traceFragment == nullptr )
		{
			TraceError{ClassId} << "Failed to compile SSGI shaders !";

			return false;
		}

		/* ---- Create pipelines ---- */
		m_tracePipeline = this->createFullscreenPipeline(ClassId, "SSGI_Trace", vertexModule, traceFragment, m_traceLayout, m_traceTarget);

		if ( m_tracePipeline == nullptr )
		{
			return false;
		}

		if ( m_skyVisibilityActive )
		{
			const auto horizonFragment = shaderManager.getShaderModuleFromSourceCode(device, "SSGI_SkyVisibility_FS", ShaderType::FragmentShader, SSGIHorizonFragmentShader);

			if ( horizonFragment == nullptr )
			{
				TraceError{ClassId} << "Failed to compile the SSGI sky-visibility shader !";

				return false;
			}

			m_horizonPipeline = this->createFullscreenPipeline(ClassId, "SSGI_SkyVisibility", vertexModule, horizonFragment, m_horizonLayout, m_horizonTarget);

			if ( m_horizonPipeline == nullptr )
			{
				return false;
			}
		}

		/* ---- Create descriptor sets ---- */

		/* Trace: reads depth + normals + scene color (all updated per-frame). */
		m_tracePerFrame = this->createPerFrameDescriptorSets(traceInputLayout, ClassId, "Trace_DescSet");

		if ( m_tracePerFrame.empty() )
		{
			return false;
		}

		/* Combine source default: the raw trace. recordOverlayPasses() retargets it to the
		 * denoiser output every frame when the temporal chain is active. */
		m_combineSource = &m_traceTarget;

		return true;
	}

	void
	SSGI::destroy () noexcept
	{
		m_combineSource = nullptr;

		/* ⚠️ Cleared BEFORE anything is released: it is what providesIndirectDiffuse() answers, and
		 * a destroyed effect still claiming the indirect diffuse would leave the scene with its
		 * raster IBL leg off and nothing to composite the sky. */
		m_skyVisibilityActive = false;

		m_horizonPerFrame.clear();
		m_tracePerFrame.clear();

		m_horizonPipeline.reset();
		m_horizonLayout.reset();
		m_tracePipeline.reset();
		m_traceLayout.reset();

		m_denoiser.destroy();

		m_horizonTarget.destroy();
		m_traceTarget.destroy();
	}

	void
	SSGI::recordOverlayPasses (const CommandBuffer & commandBuffer, const TextureInterface & inputColor, const FrameContext & context) noexcept
	{
		const auto * inputDepth = context.depth;
		const auto * inputNormals = context.normals;
		const auto & constants = context.constants;

		const auto frameIndex = this->renderer().currentFrameIndex();

		/* Update depth + normals + scene color descriptors for this frame's trace pass. */
		if ( inputDepth != nullptr )
		{
			static_cast< void >(m_tracePerFrame[frameIndex]->writeCombinedImageSampler(0, *inputDepth));
		}

		if ( inputNormals != nullptr )
		{
			static_cast< void >(m_tracePerFrame[frameIndex]->writeCombinedImageSampler(1, *inputNormals));
		}

		static_cast< void >(m_tracePerFrame[frameIndex]->writeCombinedImageSampler(2, inputColor));

		if ( m_skyVisibilityActive )
		{
			static_cast< void >(m_tracePerFrame[frameIndex]->writeCombinedImageSampler(3, m_horizonTarget));

			if ( inputDepth != nullptr )
			{
				static_cast< void >(m_horizonPerFrame[frameIndex]->writeCombinedImageSampler(0, *inputDepth));
			}

			if ( inputNormals != nullptr )
			{
				static_cast< void >(m_horizonPerFrame[frameIndex]->writeCombinedImageSampler(1, *inputNormals));
			}
		}

		/* ---- Frame UBO of the denoiser (matrices + temporal parameters; SSGI has no feedback
		 * loop, and its sky term is composed in the trace itself — the trace scalars travel
		 * through its own push constants, only the noise index below is shared). ---- */
		const bool animated = m_denoiser.temporalActive() && m_parameters.temporalAnimatedNoise;
		const auto noiseFrameIndex = static_cast< float >(m_denoiser.noiseFrameIndex());

		static_cast< void >(m_denoiser.updateFrameData(frameIndex, context, GIDenoiser::FrameInputs{}));

		/* ---- Pass 1: sky visibility (GTAO horizon search) ----
		 * FIRST: the trace reads its result. */
		if ( m_skyVisibilityActive )
		{
			/* Use readStateIndex for the SAME view matrix that produced the depth buffer. */
			const auto readStateIndex = this->renderer().currentReadStateIndex();
			const auto & viewMat = this->renderer().mainRenderTarget()->viewMatrices().viewMatrix(readStateIndex, false, 0);
			const auto invView = viewMat.inverse();
			const auto * inv = invView.data();

			const HorizonPushConstants pc{
				.invViewCol0 = {inv[0], inv[1], inv[2], m_parameters.skyVisibilityRadius},
				.invViewCol1 = {inv[4], inv[5], inv[6], m_parameters.skyVisibilityFalloffRange},
				.invViewCol2 = {inv[8], inv[9], inv[10], animated ? noiseFrameIndex : -1.0F},
				.nearPlane = constants.nearPlane,
				.farPlane = constants.farPlane,
				.tanHalfFovY = constants.tanHalfFovY,
				.aspectRatio = constants.frameWidth / constants.frameHeight,
				.sliceCount = m_parameters.skyVisibilitySliceCount,
				.stepCount = m_parameters.skyVisibilityStepCount
			};

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_horizonTarget,
				*m_horizonPipeline,
				*m_horizonLayout,
				*m_horizonPerFrame[frameIndex],
				&pc,
				sizeof(HorizonPushConstants)
			);
		}

		/* ---- Pass 2: Screen-Space GI Trace ---- */
		{
			const TracePushConstants pc{
				.texelSizeX = 1.0F / static_cast< float >(m_traceTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_traceTarget.height()),
				.nearPlane = constants.nearPlane,
				.farPlane = constants.farPlane,
				.tanHalfFovY = constants.tanHalfFovY,
				.aspectRatio = constants.frameWidth / constants.frameHeight,
				.maxDistance = m_parameters.maxDistance,
				.thickness = m_parameters.thickness,
				.sampleCount = m_parameters.sampleCount,
				.stepCount = m_parameters.stepCount,
				.noiseFrameIndex = animated ? noiseFrameIndex : -1.0F,
				.skyLuminance = m_skyVisibilityActive ? context.skyLuminance : 0.0F
			};

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_traceTarget,
				*m_tracePipeline,
				*m_traceLayout,
				*m_tracePerFrame[frameIndex],
				&pc,
				sizeof(TracePushConstants),
				m_skyVisibilityActive ? this->renderer().bindlessTextureManager().descriptorSet() : nullptr
			);
		}

		/* ---- Denoise chain (SVGF order): temporal resolve on the RAW trace + moments
		 * accumulation + normal history, then the variance-guided à-trous iterations. */
		m_combineSource = m_denoiser.recordResolve(commandBuffer, m_traceTarget, context);
	}

	bool
	SSGI::providesIndirectDiffuse () const noexcept
	{
		/* ⚠️ The SAME gate the sky term actually runs on — see the header. An effect claiming the
		 * indirect diffuse composites the sky ITSELF, with the visibility it measures, and the
		 * scene switches its raster leg off: claiming it without a sky term would leave the frame
		 * with no sky at all (the failure RTGI's own gate exists to prevent). */
		return m_skyVisibilityActive && this->isCreated();
	}

	IndirectPostProcessEffect::CombineContribution
	SSGI::combineContribution (const FrameContext & /*context*/) const noexcept
	{
		/* Denoiser debug views (diagnostic): draw the denoiser internals INSTEAD of the GI
		 * contribution. */
		if ( m_parameters.denoiserDebugView != 0U && m_denoiser.temporalActive() )
		{
			return m_denoiser.debugCombineContribution("ssgi", m_parameters.denoiserDebugView);
		}

		CombineContribution contribution;
		contribution.prefix = "ssgi";
		contribution.samplers.emplace_back(CombineSamplerInput{"Tex", m_combineSource});
		contribution.needsMaterialProperties = true;
		contribution.needsAlbedo = true;
		contribution.dynamics.emplace_back(Base::Math::Vector< 4, float >{m_parameters.intensity, 0.0F, 0.0F, 0.0F});

		/* Same math as the retired SSGI_Apply_FS pass: emissive surfaces reject GI
		 * (they emit their own light), the indirect diffuse is modulated by the
		 * receiver's DIFFUSE albedo = albedo G-buffer `rgb * a` (base colour times the
		 * diffuse weight (1 - metalness)(1 - transmission) — the base colour alone would
		 * light a metal, which has no diffuse lobe; without any albedo a coloured surface
		 * lit only by indirect light shows the raw incoming grey light), then the user
		 * intensity scales the additive blend.
		 * ⚠️ Since Sep 2026 the trace also carries the SKY irradiance (sky-visibility pass), and
		 * that is deliberate: the raster leg this effect takes over multiplied the very same
		 * cubemap by the very same base colour, so the demodulated convention holds for both
		 * halves of the estimator and the albedo is applied exactly ONCE, here. */
		contribution.code =
			"\tvec3 ssgiGI = texture(ssgiTex, vUV).rgb;\n"
			"\tvec4 ssgiMp = texture(emMaterialProps, vUV);\n"
			"\tfloat ssgiEmissive = float(uint(ssgiMp.b * 255.0) & 0xFu) / 15.0;\n"
			"\tssgiGI *= (1.0 - ssgiEmissive);\n"
			"\tvec4 ssgiAlbedo = texture(emAlbedo, vUV);\n"
			"\tssgiGI *= ssgiAlbedo.rgb * ssgiAlbedo.a;\n"
			"\tem_Color.rgb += ssgiGI * emDyn.ssgiDynamics0.x;\n";

		return contribution;
	}
}
