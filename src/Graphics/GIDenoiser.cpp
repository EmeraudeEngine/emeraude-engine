/*
 * src/Graphics/GIDenoiser.cpp
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

#include "GIDenoiser.hpp"

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Graphics/ViewMatricesInterface.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/GPUProfiler.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/PipelineLayout.hpp"

namespace
{
	/* Shared GLSL of the temporal and moments passes: the frame UBO, the flags and the
	 * REPROJECTION — one function both passes call, so they can never disagree on which history
	 * pixel is valid. Two reprojection paths:
	 *  - SURFACE: the velocity buffer (per-object motion vectors, NDC delta) with 3x3 depth-nearest
	 *    dilation, so thin foreground silhouettes drag their motion over the background edge pixels
	 *    instead of smearing.
	 *  - VIRTUAL (reflection mode, flag bit 3 — RTR, 2026-09-13): the reflected content does not
	 *    move with the reflector, it moves with its mirror image — the VIRTUAL point P + V·hitT
	 *    behind the surface (Stachowiak & Uludag, "Stochastic Screen-Space Reflections", SIGGRAPH
	 *    2015 Advances; Zhdan, "ReBLUR: A Hierarchical Recurrent Denoiser", Ray Tracing Gems II
	 *    ch. 49, 2021 — the "virtual motion"). Exact for a planar mirror reflecting a static world,
	 *    approximate on a curved reflector; the share of the virtual path fades with the roughness
	 *    (a rough lobe has no single virtual point, and its blur hides the parallax the surface path
	 *    gets wrong). Both histories are fetched and blended by their weights; an invalid one hands
	 *    its weight to the other. In reflection mode the validation distance is the VIRTUAL one,
	 *    cameraDistance + hitT, stored in the MOMENTS alpha — the colour history's alpha carries the
	 *    trace confidence — and the surface path expects it too (the same reflector point had the
	 *    same hit last frame).
	 * Descriptor bindings shared by both passes: 1 depth, 2 normals (view space, alpha = packed
	 * roughness/metalness), 4 world-normal history, 5 velocity, 7 hit data (RG16F: cone width,
	 * hitT — the reflection mode's source; the diffuse producers bind a placeholder), 8 frame UBO. */
#define GIDENOISER_REPROJECTION_GLSL R"GLSL(
layout(set = 0, binding = 1) uniform sampler2D depthTex;
layout(set = 0, binding = 2) uniform sampler2D normalTex;
layout(set = 0, binding = 4) uniform sampler2D historyNormalTex;
layout(set = 0, binding = 5) uniform sampler2D velocityTex;
layout(set = 0, binding = 7) uniform sampler2D hitDataTex;

layout(set = 0, binding = 8, std140) uniform FrameData
{
	mat4 invRelativeViewProj;
	mat4 prevViewProj;
	vec4 invViewCol0;	/* xyz = inverse view rotation column 0, w = camera position X. */
	vec4 invViewCol1;	/* xyz = inverse view rotation column 1, w = camera position Y. */
	vec4 invViewCol2;	/* xyz = inverse view rotation column 2, w = camera position Z. */
	vec4 prevCamPos;	/* xyz = previous frame camera position, w = unused. */
	vec4 traceParams;	/* x = maxDistance, y = bias, z = sampleCount, w = animated-noise frame index (R2). */
	vec4 temporalParams;	/* x = alpha, y = depthTolerance, z = normalThreshold, w = flags (see the Flag constants). */
	vec4 bounceParams;	/* x = multiBounceStrength, y = unused, z = variance-clip gamma, w = accumulation cap N. */
};

const uint FlagVarianceClip = 1u;
const uint FlagAnimatedNoise = 2u;
const uint FlagAccumulationCounter = 4u;
const uint FlagReflection = 8u;

/* Roughness range over which the virtual path hands over to the surface path (reflection mode). */
const float VirtualFadeStart = 0.15;
const float VirtualFadeEnd = 0.45;

const vec3 LumaWeights = vec3(0.2126, 0.7152, 0.0722);

struct Reprojection
{
	vec2 surfaceUV;					/* Where the SURFACE point was last frame (velocity). */
	vec2 virtualUV;					/* Where the reflected content was last frame (virtual point); = surfaceUV without a hit. */
	float virtualWeight;			/* Share of the virtual path: 0 outside reflection mode, without a hit, or on a rough surface. */
	float expectedSurfaceDistance;	/* What the history's distance channel must read at surfaceUV. */
	float expectedVirtualDistance;	/* ... at virtualUV. */
	float storeDistance;			/* The distance this pixel stores for the next frame's tests. */
};

Reprojection reproject (vec2 uv, float depth, vec3 worldPos, vec3 viewPos, float cameraDistance, float packedRM)
{
	/* Velocity DILATION: the velocity of the 3x3 neighbour closest to the camera. */
	vec2 texelD = 1.0 / vec2(textureSize(depthTex, 0));
	vec2 closestOffset = vec2(0.0);
	float closestDepth = depth;

	for (int y = -1; y <= 1; y++)
	{
		for (int x = -1; x <= 1; x++)
		{
			vec2 offset = vec2(x, y) * texelD;
			float d = texture(depthTex, uv + offset).r;

			if (d < closestDepth)
			{
				closestDepth = d;
				closestOffset = offset;
			}
		}
	}

	vec2 velocity = texture(velocityTex, uv + closestOffset).rg;

	Reprojection r;
	r.surfaceUV = uv - velocity * 0.5;
	r.virtualUV = r.surfaceUV;
	r.virtualWeight = 0.0;
	r.expectedSurfaceDistance = length(worldPos - prevCamPos.xyz);
	r.expectedVirtualDistance = r.expectedSurfaceDistance;
	r.storeDistance = cameraDistance;

	if ((uint(temporalParams.w) & FlagReflection) != 0u)
	{
		float hitT = texture(hitDataTex, uv).g;

		if (hitT > 0.0)
		{
			float roughness = packedRM >= 2.0 ? packedRM - 2.0 : packedRM;
			vec3 viewDir = (worldPos - viewPos) / max(cameraDistance, 1e-4);
			/* The mirror image of the hit point: seen through the reflector, the reflected content sits
			 * hitT BEHIND the surface along the view ray. */
			vec3 virtualPos = worldPos + viewDir * hitT;
			vec4 prevClip = prevViewProj * vec4(virtualPos, 1.0);

			if (prevClip.w > 1e-4)
			{
				r.virtualUV = (prevClip.xy / prevClip.w) * 0.5 + 0.5;
				r.virtualWeight = 1.0 - smoothstep(VirtualFadeStart, VirtualFadeEnd, roughness);
			}

			r.expectedVirtualDistance = length(virtualPos - prevCamPos.xyz);
			/* The surface path lands on the same reflector point, whose stored distance was ITS virtual one. */
			r.expectedSurfaceDistance += hitT;
			r.storeDistance = cameraDistance + hitT;
		}
	}

	return r;
}

/* A history sample is usable only if every component is finite: a NaN or an infinity in the
 * ping-pong (uninitialised memory on the first frames, or a poisoned pixel) must be REJECTED, not
 * blended — `mix(NaN, current, alpha)` is NaN whatever alpha, and a NaN reaching the combine is a
 * black pixel that the TAA cannot smooth. */
bool historySampleFinite (vec4 value)
{
	return !(any(isnan(value)) || any(isinf(value)));
}

/* On-screen, distance within tolerance (rotation-invariant), reflector normal preserved.
 * ⚠️ POSITIVE logic on purpose: `stored > 0 && |delta| <= tol` is FALSE for a NaN stored distance
 * (every comparison with NaN is false), so garbage rejects itself. The inverted form
 * `stored <= 0 || |delta| > tol` ACCEPTS a NaN — it shipped for a few hours on 2026-09-13 and let
 * NaN texels of the uninitialised history through as black 4x4 blocks on the screen-space lane. */
bool historyValid (vec2 uv, float storedDistance, float expectedDistance, vec3 worldNormal)
{
	if (any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0))))
	{
		return false;
	}

	bool distanceValid = storedDistance > 0.0 && abs(storedDistance - expectedDistance) <= temporalParams.y * expectedDistance;

	if (!distanceValid)
	{
		return false;
	}

	vec3 prevNormal = texture(historyNormalTex, uv).xyz;

	return dot(prevNormal, worldNormal) >= temporalParams.z;
}
)GLSL"

	/* Temporal resolve pass: exponential moving average between the current noisy estimate and
	 * the reprojected history (see the shared reprojection above). History is rejected on
	 * disocclusion (distance mismatch, normal mismatch) and optionally rectified by variance
	 * clipping against the current 3x3 neighbourhood (anti-ghosting).
	 * Output: GI producers — RGB = resolved DEMODULATED indirect irradiance (receiver albedo applied
	 * at the combine), A = camera distance (0 = invalid/sky). Reflection mode — RGBA = the resolved
	 * premultiplied colour + confidence, the distance living in the moments.
	 *
	 * Descriptor set 0 (pass-specific bindings; the shared ones are in the prelude):
	 *   binding 0: current raw estimate (the owner's trace output)
	 *   binding 3: colour history texture (previous resolved frame)
	 *   binding 6: moments history texture (accumulation age, and the distance in reflection mode)
	 */
	constexpr auto GIDenoiserTemporalFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outResolved;

layout(set = 0, binding = 0) uniform sampler2D giTex;
layout(set = 0, binding = 3) uniform sampler2D historyTex;
layout(set = 0, binding = 6) uniform sampler2D momentsHistoryTex;
)GLSL" GIDENOISER_REPROJECTION_GLSL R"GLSL(

void main()
{
	float depth = texture(depthTex, vUV).r;

	/* Sky/far-plane: no surface, invalid history marker (a = 0). */
	if (depth >= 1.0)
	{
		outResolved = vec4(0.0);
		return;
	}

	bool reflection = (uint(temporalParams.w) & FlagReflection) != 0u;

	vec4 current = texture(giTex, vUV);

	/* Reconstruct the world position from NDC + depth. */
	vec2 ndc = vUV * 2.0 - 1.0;
	vec4 clipPos = vec4(ndc, depth, 1.0);
	/* ⚠️⚠️ CAMERA-RELATIVE: the matrix inverts projection × view ROTATION (the infinity view), so this
	 * is the offset from the eye and the world position is the camera plus it. The float inverse of
	 * the FULL view-projection put the surface 4 cm (10 m away) to 4.4 m (200 m away, 7.8 km from the
	 * world origin) off, differently at every pose (2026-09-25). */
	vec4 wp = invRelativeViewProj * clipPos;
	vec3 relativePos = wp.xyz / wp.w;

	vec3 viewPos = vec3(invViewCol0.w, invViewCol1.w, invViewCol2.w);
	vec3 worldPos = viewPos + relativePos;
	float cameraDistance = length(relativePos);

	/* Current world-space normal, for the history normal comparison. */
	vec4 normalData = texture(normalTex, vUV);
	mat3 invViewRot = mat3(invViewCol0.xyz, invViewCol1.xyz, invViewCol2.xyz);
	vec3 worldNormal = normalize(invViewRot * normalize(normalData.rgb));

	/* What this pixel writes without a usable history: the current estimate — with the surface
	 * distance in alpha for the GI producers; the reflection mode keeps its confidence there and
	 * stores its distance through the moments pass. */
	vec4 fresh = reflection ? current : vec4(current.rgb, cameraDistance);

	Reprojection rp = reproject(vUV, depth, worldPos, viewPos, cameraDistance, normalData.a);

	vec4 histS = texture(historyTex, rp.surfaceUV);
	vec4 momS = texture(momentsHistoryTex, rp.surfaceUV);
	bool validS = historySampleFinite(histS) && historySampleFinite(momS) && historyValid(rp.surfaceUV, reflection ? momS.a : histS.a, rp.expectedSurfaceDistance, worldNormal);
	float wS = validS ? (1.0 - rp.virtualWeight) : 0.0;

	vec4 histV = vec4(0.0);
	vec4 momV = vec4(0.0);
	bool validV = false;
	float wV = 0.0;

	if (rp.virtualWeight > 0.0)
	{
		histV = texture(historyTex, rp.virtualUV);
		momV = texture(momentsHistoryTex, rp.virtualUV);
		validV = historySampleFinite(histV) && historySampleFinite(momV) && historyValid(rp.virtualUV, momV.a, rp.expectedVirtualDistance, worldNormal);
		wV = validV ? rp.virtualWeight : 0.0;
	}

	float wSum = wS + wV;

	if (wSum <= 0.0)
	{
		/* Off-screen or disoccluded on every path: no history, full weight on the current estimate. */
		outResolved = fresh;
		return;
	}

	/* ⚠️ An INVALID sample is never multiplied in, not even by zero: garbage times zero is garbage
	 * (NaN * 0 = NaN). Blend only what was validated. */
	vec4 history = validS && validV ? (histS * wS + histV * wV) / wSum : (validS ? histS : histV);
	float age = validS && validV ? (momS.b * wS + momV.b * wV) / wSum : (validS ? momS.b : momV.b);

	float alpha = temporalParams.x;

	/* Per-pixel 1/N accumulation counter (SVGF): the blend weight follows the pixel's own
	 * accumulation age (from the moments history — the moments pass maintains it with the SAME
	 * validation) instead of a fixed EMA. Fast convergence after a disocclusion (1, 1/2, 1/3...),
	 * tiny steady-state variance leak (1/N at the cap). */
	if ((uint(temporalParams.w) & FlagAccumulationCounter) != 0u && alpha < 1.0)
	{
		float maxN = max(bounceParams.w, 1.0);
		alpha = max(1.0 / (age + 1.0), 1.0 / maxN);
	}

	/* History rectification: VARIANCE CLIPPING — bound the history to mean ± gamma * sigma of
	 * the current 3x3 neighbourhood (M. Salvi, "An Excursion in Temporal Supersampling", GDC 2016
	 * — the same technique as the engine TAA). It replaced the min/max clamp: with ANIMATED noise
	 * the per-frame estimates are deliberately different, and a min/max box collapses onto whatever
	 * outlier the current frame produced, killing the convergence the animation exists for. The
	 * statistical bound keeps disocclusion ghosting bounded while letting the EMA actually
	 * accumulate. In reflection mode the four channels are clipped (the confidence too).
	 * bounceParams.z = gamma (Temporal/VarianceGamma key). */
	if ((uint(temporalParams.w) & FlagVarianceClip) != 0u)
	{
		vec2 texel = 1.0 / vec2(textureSize(giTex, 0));
		vec4 m1 = vec4(0.0);
		vec4 m2 = vec4(0.0);

		for (int y = -1; y <= 1; y++)
		{
			for (int x = -1; x <= 1; x++)
			{
				vec4 nb = texture(giTex, vUV + vec2(x, y) * texel);
				m1 += nb;
				m2 += nb * nb;
			}
		}

		vec4 mu = m1 / 9.0;
		vec4 sigma = sqrt(max(m2 / 9.0 - mu * mu, vec4(0.0)));
		vec4 clipped = clamp(history, mu - bounceParams.z * sigma, mu + bounceParams.z * sigma);

		history = reflection ? clipped : vec4(clipped.rgb, history.a);
	}

	outResolved = reflection ? mix(history, current, alpha) : vec4(mix(history.rgb, current.rgb, alpha), cameraDistance);
}
)GLSL";

	/* Moments accumulation pass (SVGF, Schied et al. 2017, HPG): integrates the first and
	 * second raw moments of the RAW estimate's luminance with the SAME reprojection and
	 * disocclusion validation as the colour resolve. The temporal variance max(m2 - m1², 0)
	 * measures the per-pixel estimator noise and drives the luminance-weight normalisation of the
	 * à-trous filter (auto-dosage: noisy → smooth hard, converged → preserve). The moments
	 * deliberately read the RAW input, not the blurred one: variance of an already-smoothed
	 * signal underestimates the noise the spatial filter must remove.
	 * Output: R = m1, G = m2, B = accumulation age in frames (saturates at the cap, reset on
	 * disocclusion — the 1/N counter), A = the validation distance (camera distance; the VIRTUAL
	 * distance camera + hit in reflection mode; 0 = invalid/sky).
	 *
	 * Descriptor set 0 (pass-specific bindings; the shared ones are in the prelude):
	 *   binding 0: raw estimate (the owner's trace output)
	 *   binding 3: moments history texture (previous frame)
	 *   binding 6: unused (layout shared with the temporal pass)
	 */
	constexpr auto GIDenoiserMomentsFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outMoments;

layout(set = 0, binding = 0) uniform sampler2D rawTex;
layout(set = 0, binding = 3) uniform sampler2D momentsHistoryTex;
)GLSL" GIDENOISER_REPROJECTION_GLSL R"GLSL(

void main()
{
	float depth = texture(depthTex, vUV).r;

	/* Sky/far-plane: no surface, invalid marker (a = 0). */
	if (depth >= 1.0)
	{
		outMoments = vec4(0.0);
		return;
	}

	float luma = dot(texture(rawTex, vUV).rgb, LumaWeights);

	/* Reconstruct the world position from NDC + depth. */
	vec2 ndc = vUV * 2.0 - 1.0;
	vec4 clipPos = vec4(ndc, depth, 1.0);
	/* ⚠️⚠️ CAMERA-RELATIVE: the matrix inverts projection × view ROTATION (the infinity view), so this
	 * is the offset from the eye and the world position is the camera plus it. The float inverse of
	 * the FULL view-projection put the surface 4 cm (10 m away) to 4.4 m (200 m away, 7.8 km from the
	 * world origin) off, differently at every pose (2026-09-25). */
	vec4 wp = invRelativeViewProj * clipPos;
	vec3 relativePos = wp.xyz / wp.w;

	vec3 viewPos = vec3(invViewCol0.w, invViewCol1.w, invViewCol2.w);
	vec3 worldPos = viewPos + relativePos;
	float cameraDistance = length(relativePos);

	/* Current world-space normal, for the history normal comparison. */
	vec4 normalData = texture(normalTex, vUV);
	mat3 invViewRot = mat3(invViewCol0.xyz, invViewCol1.xyz, invViewCol2.xyz);
	vec3 worldNormal = normalize(invViewRot * normalize(normalData.rgb));

	float alpha = temporalParams.x;

	/* Same reprojection + validation as the colour resolve — the moments and the colour MUST
	 * agree on which pixels have a valid history. */
	Reprojection rp = reproject(vUV, depth, worldPos, viewPos, cameraDistance, normalData.a);

	vec4 momS = texture(momentsHistoryTex, rp.surfaceUV);
	bool validS = historySampleFinite(momS) && historyValid(rp.surfaceUV, momS.a, rp.expectedSurfaceDistance, worldNormal);
	float wS = validS ? (1.0 - rp.virtualWeight) : 0.0;

	vec4 momV = vec4(0.0);
	bool validV = false;
	float wV = 0.0;

	if (rp.virtualWeight > 0.0)
	{
		momV = texture(momentsHistoryTex, rp.virtualUV);
		validV = historySampleFinite(momV) && historyValid(rp.virtualUV, momV.a, rp.expectedVirtualDistance, worldNormal);
		wV = validV ? rp.virtualWeight : 0.0;
	}

	float wSum = wS + wV;

	/* The alpha >= 1 test also covers the first frame after (re)creation (the owner forces
	 * alpha to 1 while the history is invalid) — the age must restart from the uninitialised
	 * ping-pong. */
	if (alpha >= 1.0 || wSum <= 0.0)
	{
		/* Reset: single-sample moments (variance reads zero — consumers must use the
		 * age-gated spatial fallback while the accumulation is young). */
		outMoments = vec4(luma, luma * luma, 1.0, rp.storeDistance);
		return;
	}

	/* Blend only what was validated (see the colour resolve): garbage times zero is garbage. */
	vec4 history = validS && validV ? (momS * wS + momV * wV) / wSum : (validS ? momS : momV);

	float maxN = max(bounceParams.w, 1.0);

	/* Per-pixel 1/N accumulation counter — same weight as the colour resolve so both integrate
	 * identically. */
	if ((uint(temporalParams.w) & FlagAccumulationCounter) != 0u)
	{
		alpha = max(1.0 / (history.b + 1.0), 1.0 / maxN);
	}

	float m1 = mix(history.r, luma, alpha);
	float m2 = mix(history.g, luma * luma, alpha);
	float age = min(history.b + 1.0, maxN);

	outMoments = vec4(m1, m2, age, rp.storeDistance);
}
)GLSL";

	/* À-trous wavelet filter pass (SVGF, Schied et al. 2017, HPG): one iteration of the
	 * edge-avoiding à-trous transform over the temporally integrated irradiance. 5x5
	 * B3-spline kernel whose footprint doubles each iteration (texel stride 1, 2, 4, 8, 16);
	 * edge-stopping weights on depth, view-space normal and LUMINANCE — the luminance
	 * difference is normalised by the local standard deviation (temporal variance from the
	 * moments on the first iteration, then the variance filtered alongside the colour with
	 * the w² propagation rule). Auto-dosage: a noisy pixel tolerates large luminance
	 * differences (smooths hard), a converged one rejects them (preserves detail) — this is
	 * what a fixed-radius bilateral blur cannot do, and the reason the frozen-pattern
	 * marbling survives it.
	 * Young history (< 4 frames — freshly disoccluded silhouettes under the TAA jitter, the
	 * wind-animated foliage): the temporal variance reads zero there by construction, so the
	 * first iteration falls back to a 3x3 SPATIAL variance estimate of the input luminance.
	 *
	 * Descriptor set 0:
	 *   binding 0: input (first iteration: resolved history, RGB + camera distance in A;
	 *              then: previous à-trous output, RGB + filtered variance in A)
	 *   binding 1: moments texture (variance + age, first iteration only)
	 *   binding 2: depth texture
	 *   binding 3: normals texture (view space)
	 * Push constants: stride, sigmas, first-iteration flag.
	 */
	constexpr auto GIDenoiserAtrousFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outFiltered;

layout(set = 0, binding = 0) uniform sampler2D inputTex;
layout(set = 0, binding = 1) uniform sampler2D momentsTex;
layout(set = 0, binding = 2) uniform sampler2D depthTex;
layout(set = 0, binding = 3) uniform sampler2D normalTex;

layout(push_constant) uniform PushConstants
{
	float stepSize;		/* Texel stride of this iteration (1, 2, 4, 8, 16). */
	float depthSigma;
	float normalSigma;
	float luminanceSigma;
	float firstIteration;	/* > 0.5: variance from momentsTex (+ young-history spatial fallback). */
} pc;

const vec3 LumaWeights = vec3(0.2126, 0.7152, 0.0722);

/* B3-spline half kernel (center, 1, 2). */
const float Kernel[3] = float[3](0.375, 0.25, 0.0625);

void main()
{
	vec4 center = texture(inputTex, vUV);
	float centerDepth = texture(depthTex, vUV).r;

	bool first = pc.firstIteration > 0.5;

	/* Sky/far-plane: pass through (variance 0). */
	if (centerDepth >= 1.0)
	{
		outFiltered = vec4(center.rgb, first ? 0.0 : center.a);
		return;
	}

	vec2 texel = 1.0 / vec2(textureSize(inputTex, 0));

	/* Variance of the CENTER pixel drives the luminance tolerance. */
	float centerVar;

	if (first)
	{
		vec4 moments = texture(momentsTex, vUV);

		if (moments.b < 4.0)
		{
			/* Young history: the temporal variance is meaningless (single sample reads
			 * zero — the filter would freeze exactly where it must smooth hardest).
			 * Estimate it SPATIALLY from the 3x3 input neighbourhood instead. */
			float m1 = 0.0;
			float m2 = 0.0;

			for (int y = -1; y <= 1; y++)
			{
				for (int x = -1; x <= 1; x++)
				{
					float l = dot(texture(inputTex, vUV + vec2(x, y) * texel).rgb, LumaWeights);
					m1 += l;
					m2 += l * l;
				}
			}

			m1 /= 9.0;
			m2 /= 9.0;
			centerVar = max(m2 - m1 * m1, 0.0);
		}
		else
		{
			centerVar = max(moments.g - moments.r * moments.r, 0.0);
		}
	}
	else
	{
		centerVar = max(center.a, 0.0);
	}

	vec3 centerNormal = texture(normalTex, vUV).rgb;
	float centerLuma = dot(center.rgb, LumaWeights);

	float invDepthSigma2 = 1.0 / (2.0 * pc.depthSigma * pc.depthSigma);
	float invNormalSigma = 1.0 / max(pc.normalSigma, 0.001);
	/* Luminance tolerance scales with the local noise (the SVGF auto-dosage). */
	float lumaDenom = pc.luminanceSigma * sqrt(centerVar) + 0.0001;

	/* Center tap. */
	float centerWeight = Kernel[0] * Kernel[0];
	vec3 sumColor = center.rgb * centerWeight;
	float sumVariance = centerVar * centerWeight * centerWeight;
	float sumWeight = centerWeight;

	for (int y = -2; y <= 2; y++)
	{
		for (int x = -2; x <= 2; x++)
		{
			if (x == 0 && y == 0)
			{
				continue;
			}

			vec2 uv = vUV + vec2(x, y) * texel * pc.stepSize;
			float qDepth = texture(depthTex, uv).r;

			/* Sky samples carry no surface irradiance. */
			if (qDepth >= 1.0)
			{
				continue;
			}

			vec4 q = texture(inputTex, uv);
			vec3 qNormal = texture(normalTex, uv).rgb;

			float h = Kernel[abs(x)] * Kernel[abs(y)];

			float depthDiff = centerDepth - qDepth;
			float weightZ = exp(-depthDiff * depthDiff * invDepthSigma2);

			float weightN = pow(max(dot(centerNormal, qNormal), 0.0), invNormalSigma);

			float qLuma = dot(q.rgb, LumaWeights);
			float weightL = exp(-abs(centerLuma - qLuma) / lumaDenom);

			float weight = h * weightZ * weightN * weightL;

			/* Sample variance: temporal moments on the first iteration (a young
			 * neighbour contributes zero — its colour is still filtered), the
			 * alpha channel afterwards. */
			float qVar;

			if (first)
			{
				vec4 qMoments = texture(momentsTex, uv);
				qVar = max(qMoments.g - qMoments.r * qMoments.r, 0.0);
			}
			else
			{
				qVar = max(q.a, 0.0);
			}

			sumColor += q.rgb * weight;
			sumVariance += qVar * weight * weight;
			sumWeight += weight;
		}
	}

	outFiltered = vec4(sumColor / sumWeight, sumVariance / (sumWeight * sumWeight));
}
)GLSL";

	/* Normal history pass: converts the current view-space normals G-buffer to world space
	 * (camera-rotation invariant) and stores it at history resolution for the NEXT frame's
	 * temporal validation. The normals MRT attachment is rewritten every frame, so the
	 * previous frame's normals must be explicitly retained.
	 *
	 * Descriptor set 0:
	 *   binding 0: normals texture (view space, current frame)
	 *   binding 1: frame UBO (shared with the owner's trace pass)
	 */
	constexpr auto GIDenoiserNormalCopyFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outWorldNormal;

layout(set = 0, binding = 0) uniform sampler2D normalTex;

layout(set = 0, binding = 1, std140) uniform FrameData
{
	mat4 invRelativeViewProj;
	mat4 prevViewProj;
	vec4 invViewCol0;	/* xyz = inverse view rotation column 0, w = camera position X. */
	vec4 invViewCol1;	/* xyz = inverse view rotation column 1, w = camera position Y. */
	vec4 invViewCol2;	/* xyz = inverse view rotation column 2, w = camera position Z. */
	vec4 prevCamPos;	/* xyz = previous frame camera position, w = unused. */
	vec4 traceParams;	/* x = maxDistance, y = bias, z = sampleCount, w = animated-noise frame index (R2). */
	vec4 temporalParams;	/* x = alpha, y = depthTolerance, z = normalThreshold, w = flags (bit0 variance clip, bit1 animated noise). */
	vec4 bounceParams;	/* x = multiBounceStrength, y = unused, z = variance-clip gamma, w = unused. */
};

void main()
{
	vec3 rawN = texture(normalTex, vUV).rgb;

	if (dot(rawN, rawN) < 0.0001)
	{
		outWorldNormal = vec4(0.0);
		return;
	}

	mat3 invViewRot = mat3(invViewCol0.xyz, invViewCol1.xyz, invViewCol2.xyz);

	outWorldNormal = vec4(normalize(invViewRot * normalize(rawN)), 1.0);
}
)GLSL";
}

namespace EmEn::Graphics
{
	using namespace Base;
	using namespace Vulkan;
	using namespace Saphir;

	bool
	GIDenoiser::create (uint32_t width, uint32_t height) noexcept
	{
		auto & renderer = this->renderer();

		/* History starts invalid: the first frame after (re)creation must not read the
		 * uninitialized ping-pong images (alpha forced to 1, no multi-bounce feedback). */
		m_historyValid = false;
		m_historyWriteIndex = 0;
		m_noiseFrameIndex = 0;

		const std::string baseName{m_ownerLabel};

		/* ---- Per-frame UBOs (shared by the owner's trace and the denoiser passes) ----
		 * Allocated even when the temporal chain is off: the owner's trace reads the
		 * frame data (matrices, trace/sky scalars) unconditionally. */
		m_frameUBOs = this->createPerFrameUniformBuffers(sizeof(FrameUBOData), ClassId, baseName + "_Frame_UBO");

		if ( m_frameUBOs.empty() )
		{
			return false;
		}

		if ( !m_temporalEnabled )
		{
			return true;
		}

		/* Temporal history targets (owner resolution, ping-pong). Only allocated when the
		 * temporal accumulation is enabled, so the disabled path costs no VRAM. */
		for ( size_t index = 0; index < 2; ++index )
		{
			const auto suffix = std::to_string(index);

			if ( !m_historyTargets[index].create(renderer, width, height, VK_FORMAT_R16G16B16A16_SFLOAT, baseName + "_GIHistory" + suffix) )
			{
				TraceError{ClassId} << "Failed to create the GI history target #" << index << " !";

				return false;
			}

			if ( !m_normalHistoryTargets[index].create(renderer, width, height, VK_FORMAT_R16G16B16A16_SFLOAT, baseName + "_GINormalHistory" + suffix) )
			{
				TraceError{ClassId} << "Failed to create the GI normal history target #" << index << " !";

				return false;
			}

			if ( !m_momentsTargets[index].create(renderer, width, height, VK_FORMAT_R16G16B16A16_SFLOAT, baseName + "_GIMoments" + suffix) )
			{
				TraceError{ClassId} << "Failed to create the GI moments target #" << index << " !";

				return false;
			}

			/* À-trous working pair — only when the spatial filter is enabled. */
			if ( m_parameters.atrousIterations > 0 )
			{
				if ( !m_atrousTargets[index].create(renderer, width, height, VK_FORMAT_R16G16B16A16_SFLOAT, baseName + "_GIAtrous" + suffix) )
				{
					TraceError{ClassId} << "Failed to create the GI à-trous target #" << index << " !";

					return false;
				}
			}
		}

		/* ---- Descriptor set layouts ---- */

		/* Temporal resolve input: estimate + depth + normals + history + normal history + velocity
		 * + moments history (1/N counter age) + hit data (reflection mode), plus the frame UBO at
		 * binding 8. The moments pass reuses the SAME shape (raw estimate + depth + normals + moments
		 * history + normal history + velocity + unused + hit data + UBO). */
		auto temporalInputLayout = this->getInputLayout(8, 1);

		/* Normal history input: normals, plus the frame UBO. */
		auto normalCopyInputLayout = this->getInputLayout(1, 1);

		/* À-trous input: filter input + moments + depth + normals (parameters travel
		 * through push constants). */
		auto atrousInputLayout = this->getInputLayout(4, 0);

		if ( temporalInputLayout == nullptr || normalCopyInputLayout == nullptr || atrousInputLayout == nullptr )
		{
			return false;
		}

		/* ---- Pipeline layouts ---- */
		auto & layoutManager = renderer.layoutManager();

		{
			/* Temporal resolve + moments: single set, no push constants (frame UBO). */
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(temporalInputLayout);

			m_temporalLayout = layoutManager.getPipelineLayout(sets, {});
			m_momentsLayout = m_temporalLayout;
		}

		{
			/* Normal history: single set, no push constants (frame UBO). */
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(normalCopyInputLayout);

			m_normalCopyLayout = layoutManager.getPipelineLayout(sets, {});
		}

		{
			/* À-trous: single set + push constants (stride, sigmas, first-iteration flag). */
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(atrousInputLayout);

			m_atrousLayout = layoutManager.getPipelineLayout(sets, {VkPushConstantRange{
				.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				.offset = 0,
				.size = sizeof(AtrousPushConstants)
			}});
		}

		if ( m_temporalLayout == nullptr || m_normalCopyLayout == nullptr || m_atrousLayout == nullptr )
		{
			return false;
		}

		/* ---- Compile shaders + create pipelines ---- */
		auto & shaderManager = renderer.shaderManager();
		const auto & device = renderer.device();

		const auto vertexModule = this->getFullscreenVertexShader();
		const auto temporalFragment = shaderManager.getShaderModuleFromSourceCode(device, "GIDenoiser_Temporal_FS", ShaderType::FragmentShader, GIDenoiserTemporalFragmentShader);
		const auto momentsFragment = shaderManager.getShaderModuleFromSourceCode(device, "GIDenoiser_Moments_FS", ShaderType::FragmentShader, GIDenoiserMomentsFragmentShader);
		const auto normalCopyFragment = shaderManager.getShaderModuleFromSourceCode(device, "GIDenoiser_NormalCopy_FS", ShaderType::FragmentShader, GIDenoiserNormalCopyFragmentShader);
		const auto atrousFragment = m_parameters.atrousIterations > 0 ? shaderManager.getShaderModuleFromSourceCode(device, "GIDenoiser_Atrous_FS", ShaderType::FragmentShader, GIDenoiserAtrousFragmentShader) : nullptr;

		if ( vertexModule == nullptr || temporalFragment == nullptr || momentsFragment == nullptr || normalCopyFragment == nullptr || (m_parameters.atrousIterations > 0 && atrousFragment == nullptr) )
		{
			TraceError{ClassId} << "Failed to compile the GI denoiser shaders !";

			return false;
		}

		/* NOTE: The pipelines are created against the [0] targets; recording into [1]
		 * relies on Vulkan render pass compatibility (identical format/ops), exactly
		 * like the shared denoise pipeline recording into both blur targets. */
		m_temporalPipeline = this->createFullscreenPipeline(ClassId, baseName + "_GITemporal", vertexModule, temporalFragment, m_temporalLayout, m_historyTargets[0]);
		m_momentsPipeline = this->createFullscreenPipeline(ClassId, baseName + "_GIMoments", vertexModule, momentsFragment, m_momentsLayout, m_momentsTargets[0]);
		m_normalCopyPipeline = this->createFullscreenPipeline(ClassId, baseName + "_GINormalCopy", vertexModule, normalCopyFragment, m_normalCopyLayout, m_normalHistoryTargets[0]);

		if ( m_temporalPipeline == nullptr || m_momentsPipeline == nullptr || m_normalCopyPipeline == nullptr )
		{
			return false;
		}

		if ( m_parameters.atrousIterations > 0 )
		{
			m_atrousPipeline = this->createFullscreenPipeline(ClassId, baseName + "_GIAtrous", vertexModule, atrousFragment, m_atrousLayout, m_atrousTargets[0]);

			if ( m_atrousPipeline == nullptr )
			{
				return false;
			}
		}

		/* ---- Per-frame descriptor sets (texture bindings are rewritten every frame
		 * because of the history ping-pong; the UBO binding is written once here) ---- */
		m_temporalPerFrame = this->createPerFrameDescriptorSets(temporalInputLayout, ClassId, baseName + "_GITemporal_DescSet");
		m_momentsPerFrame = this->createPerFrameDescriptorSets(temporalInputLayout, ClassId, baseName + "_GIMoments_DescSet");
		m_normalCopyPerFrame = this->createPerFrameDescriptorSets(normalCopyInputLayout, ClassId, baseName + "_GINormalCopy_DescSet");

		if ( m_temporalPerFrame.empty() || m_momentsPerFrame.empty() || m_normalCopyPerFrame.empty() )
		{
			return false;
		}

		for ( size_t f = 0; f < m_temporalPerFrame.size(); ++f )
		{
			if ( !m_temporalPerFrame[f]->writeUniformBufferObject(8, *m_frameUBOs[f]) )
			{
				return false;
			}

			if ( !m_momentsPerFrame[f]->writeUniformBufferObject(8, *m_frameUBOs[f]) )
			{
				return false;
			}

			if ( !m_normalCopyPerFrame[f]->writeUniformBufferObject(1, *m_frameUBOs[f]) )
			{
				return false;
			}
		}

		/* À-trous sets, one flavour per INPUT (the input of the first iteration — the
		 * freshly resolved history — flips parity every frame, so its binding is rewritten
		 * per frame; the ping-pong inputs of the later iterations are static). */
		if ( m_parameters.atrousIterations > 0 )
		{
			for ( size_t flavour = 0; flavour < m_atrousPerFrame.size(); ++flavour )
			{
				m_atrousPerFrame[flavour] = this->createPerFrameDescriptorSets(atrousInputLayout, ClassId, baseName + "_GIAtrous_DescSet" + std::to_string(flavour));

				if ( m_atrousPerFrame[flavour].empty() )
				{
					return false;
				}
			}

			for ( size_t f = 0; f < m_atrousPerFrame[1].size(); ++f )
			{
				if ( !m_atrousPerFrame[1][f]->writeCombinedImageSampler(0, m_atrousTargets[0]) )
				{
					return false;
				}

				if ( !m_atrousPerFrame[2][f]->writeCombinedImageSampler(0, m_atrousTargets[1]) )
				{
					return false;
				}
			}
		}

		return true;
	}

	void
	GIDenoiser::destroy () noexcept
	{
		for ( auto & sets : m_atrousPerFrame )
		{
			sets.clear();
		}

		m_normalCopyPerFrame.clear();
		m_momentsPerFrame.clear();
		m_temporalPerFrame.clear();

		m_frameUBOs.clear();

		m_atrousPipeline.reset();
		m_normalCopyPipeline.reset();
		m_momentsPipeline.reset();
		m_temporalPipeline.reset();
		m_atrousLayout.reset();
		m_normalCopyLayout.reset();
		m_momentsLayout.reset();
		m_temporalLayout.reset();

		for ( auto & target : m_atrousTargets )
		{
			target.destroy();
		}

		for ( auto & target : m_momentsTargets )
		{
			target.destroy();
		}

		for ( auto & target : m_normalHistoryTargets )
		{
			target.destroy();
		}

		for ( auto & target : m_historyTargets )
		{
			target.destroy();
		}

		m_historyValid = false;
		m_historyWriteIndex = 0;
		m_noiseFrameIndex = 0;
	}

	bool
	GIDenoiser::updateFrameData (uint32_t frameIndex, const FrameContext & context, const FrameInputs & inputs) noexcept
	{
		/* Use readStateIndex for the SAME view matrix that produced the depth buffer. */
		const auto readStateIndex = this->renderer().currentReadStateIndex();
		const auto & viewMatrices = this->renderer().mainRenderTarget()->viewMatrices();
		const auto & viewMat = viewMatrices.viewMatrix(readStateIndex, false, 0);
		/* JITTERED (the default projectionMatrix contract): the depth buffer was rasterized
		 * with the TAA jitter, so unprojecting with the same matrix is geometrically exact.
		 * NOTE (measured 2026-08-05): swapping this for unjitteredProjectionMatrix() — to
		 * cancel the reprojection error against the unjittered previousProjectionMatrix() —
		 * had NO measurable effect on the temporal peak-to-peak (runs within the ×1.85
		 * run-to-run envelope). The GI temporal noise is content/RT-driven, not matrix-driven. */
		const auto & projMat = viewMatrices.projectionMatrix(readStateIndex);
		/* ⚠️⚠️ CAMERA-RELATIVE: projection × view ROTATION (the infinity view), inverted — the shader
		 * unprojects an offset from the eye and adds the camera position. The float inverse of the full
		 * view-projection put the ray origins centimetres to metres off the surface far from the world
		 * origin (docs/caution-points.md § The float inverse of the full view-projection). */
		const auto invRelativeViewProj = (projMat * viewMatrices.viewMatrix(readStateIndex, true, 0)).inverse();
		const auto * ivp = invRelativeViewProj.data();
		const auto & cameraPosition = viewMatrices.position(readStateIndex);

		/* Inverse view rotation for normal transformation (view → world). */
		const auto invView = viewMat.inverse();
		const auto * inv = invView.data();

		/* Previous rendered frame (ViewMatrices frame-history contract). Identity
		 * until the first frame is archived — irrelevant then, since the history is
		 * flagged invalid (alpha forced to 1, feedback strength forced to 0). */
		const auto & prevViewMat = viewMatrices.previousViewMatrix();
		const auto prevViewProj = viewMatrices.previousProjectionMatrix() * prevViewMat;
		const auto * pvp = prevViewProj.data();

		const auto prevInvView = prevViewMat.inverse();
		const auto * pinv = prevInvView.data();

		const bool historyUsable = this->historyUsable();

		const FrameUBOData ubo{
			.invRelativeViewProj = {
				ivp[0], ivp[1], ivp[2], ivp[3],
				ivp[4], ivp[5], ivp[6], ivp[7],
				ivp[8], ivp[9], ivp[10], ivp[11],
				ivp[12], ivp[13], ivp[14], ivp[15]
			},
			.prevViewProj = {
				pvp[0], pvp[1], pvp[2], pvp[3],
				pvp[4], pvp[5], pvp[6], pvp[7],
				pvp[8], pvp[9], pvp[10], pvp[11],
				pvp[12], pvp[13], pvp[14], pvp[15]
			},
			.invViewCol0 = {inv[0], inv[1], inv[2]},
			.viewPosX = cameraPosition.x(),
			.invViewCol1 = {inv[4], inv[5], inv[6]},
			.viewPosY = cameraPosition.y(),
			.invViewCol2 = {inv[8], inv[9], inv[10]},
			.viewPosZ = cameraPosition.z(),
			.prevCamPos = {pinv[12], pinv[13], pinv[14], 0.0F},
			.traceParams = {inputs.traceMaxDistance, inputs.traceBias, inputs.traceSampleCount, static_cast< float >(m_noiseFrameIndex)},
			.temporalParams = {
				historyUsable ? m_parameters.temporalAlpha : 1.0F,
				m_parameters.temporalDepthTolerance,
				m_parameters.temporalNormalThreshold,
				static_cast< float >(
					(m_parameters.temporalNeighborhoodClamp ? 1U : 0U) |
					(this->temporalActive() && m_parameters.temporalAnimatedNoise ? 2U : 0U) |
					(m_parameters.accumulationCounter ? 4U : 0U) |
					(m_parameters.reflectionMode ? 8U : 0U)
				)
			},
			.bounceParams = {
				/* Not gated on the history: the RTGI feedback reads the irradiance probes (Sep 2026). */
				inputs.bounceStrength,
				/* Unused: the multi-bounce clamp died with the screen-history feedback (Sep 2026). */
				0.0F,
				m_parameters.temporalVarianceGamma,
				static_cast< float >(m_parameters.maxAccumulation)
			},
			/* THE SKY IS A LIGHT SOURCE — the scalars come straight from the producer
			 * (0 = no sky term for the screen-space producers). */
			.skyParams = {inputs.skyLuminance, inputs.skyDistance, inputs.occlusionMaxDistance, 0.0F}
		};

		const auto success = IndirectPostProcessEffect::updateUniformBufferData(*m_frameUBOs[frameIndex], &ubo, sizeof(FrameUBOData));

		if ( !success )
		{
			TraceError{ClassId} << "Failed to update the GI denoiser frame UBO !";
		}

		/* Advance the animated-noise sequence once per recorded frame (wraps at 4096,
		 * exactly representable in float32 so fract(n * R2) stays precise). */
		m_noiseFrameIndex = (m_noiseFrameIndex + 1U) % 4096U;

		return success;
	}

	IndirectPostProcessEffect::CombineContribution
	GIDenoiser::debugCombineContribution (const char * prefix, uint32_t mode) const noexcept
	{
		const std::string p{prefix};

		CombineContribution contribution;
		contribution.prefix = prefix;
		contribution.samplers.emplace_back(CombineSamplerInput{"Moments", &this->momentsTexture()});
		contribution.dynamics.emplace_back(Base::Math::Vector< 4, float >{static_cast< float >(mode), 0.0F, 0.0F, 0.0F});

		contribution.code =
			"\tvec4 " + p + "Mom = texture(" + p + "Moments, vUV);\n"
			"\tfloat " + p + "Var = max(" + p + "Mom.g - " + p + "Mom.r * " + p + "Mom.r, 0.0);\n"
			"\tif (emDyn." + p + "Dynamics0.x < 1.5)\n"
			"\t{\n"
			"\t\t/* Variance, amplified x1e6 and bounded (readable under any exposure). */\n"
			"\t\tem_Color.rgb = vec3(min(" + p + "Var * 1e6, 1e4));\n"
			"\t}\n"
			"\telse\n"
			"\t{\n"
			"\t\t/* Accumulation age: white = young (< 4 frames, spatial-fallback zone). */\n"
			"\t\tem_Color.rgb = " + p + "Mom.b < 4.0 ? vec3(1e4) : vec3(" + p + "Mom.b / 64.0 * 100.0);\n"
			"\t}\n";

		return contribution;
	}

	const TextureInterface *
	GIDenoiser::recordResolve (const CommandBuffer & commandBuffer, const TextureInterface & rawInput, const FrameContext & context, const TextureInterface * hitData) noexcept
	{
		if ( !this->temporalActive() )
		{
			return &rawInput;
		}

		const auto frameIndex = this->renderer().currentFrameIndex();

		const uint32_t writeIdx = m_historyWriteIndex;
		const uint32_t readIdx = 1U - writeIdx;

		/* ---- Per-frame descriptor updates ---- */

		/* SVGF order: the temporal resolve integrates the RAW estimate (the spatial filter
		 * runs AFTER, on the integrated signal — a fixed blur before the accumulation is
		 * what the à-trous replaces). */
		static_cast< void >(m_temporalPerFrame[frameIndex]->writeCombinedImageSampler(0, rawInput));
		static_cast< void >(m_momentsPerFrame[frameIndex]->writeCombinedImageSampler(0, rawInput));

		/* History ping-pong: this frame reads [readIdx] and writes [writeIdx]. */
		static_cast< void >(m_temporalPerFrame[frameIndex]->writeCombinedImageSampler(3, m_historyTargets[readIdx]));
		static_cast< void >(m_temporalPerFrame[frameIndex]->writeCombinedImageSampler(4, m_normalHistoryTargets[readIdx]));
		static_cast< void >(m_temporalPerFrame[frameIndex]->writeCombinedImageSampler(6, m_momentsTargets[readIdx]));
		static_cast< void >(m_momentsPerFrame[frameIndex]->writeCombinedImageSampler(3, m_momentsTargets[readIdx]));
		static_cast< void >(m_momentsPerFrame[frameIndex]->writeCombinedImageSampler(4, m_normalHistoryTargets[readIdx]));
		/* Binding 6 is unused by the moments shader (shared layout) — keep it valid. */
		static_cast< void >(m_momentsPerFrame[frameIndex]->writeCombinedImageSampler(6, m_momentsTargets[readIdx]));

		/* Hit data (binding 7): the reflection mode's hit distances. The diffuse producers hand
		 * nothing, and the binding must still hold a valid image — their raw input stands in (the
		 * shader never reads it outside reflection mode). */
		const TextureInterface & hitDataTexture = hitData != nullptr ? *hitData : rawInput;
		static_cast< void >(m_temporalPerFrame[frameIndex]->writeCombinedImageSampler(7, hitDataTexture));
		static_cast< void >(m_momentsPerFrame[frameIndex]->writeCombinedImageSampler(7, hitDataTexture));

		if ( context.depth != nullptr )
		{
			static_cast< void >(m_temporalPerFrame[frameIndex]->writeCombinedImageSampler(1, *context.depth));
			static_cast< void >(m_momentsPerFrame[frameIndex]->writeCombinedImageSampler(1, *context.depth));
		}

		if ( context.normals != nullptr )
		{
			static_cast< void >(m_temporalPerFrame[frameIndex]->writeCombinedImageSampler(2, *context.normals));
			static_cast< void >(m_momentsPerFrame[frameIndex]->writeCombinedImageSampler(2, *context.normals));
			static_cast< void >(m_normalCopyPerFrame[frameIndex]->writeCombinedImageSampler(0, *context.normals));
		}

		if ( context.velocity != nullptr )
		{
			static_cast< void >(m_temporalPerFrame[frameIndex]->writeCombinedImageSampler(5, *context.velocity));
			static_cast< void >(m_momentsPerFrame[frameIndex]->writeCombinedImageSampler(5, *context.velocity));
		}

		/* ---- Temporal resolve + moments accumulation + normal history ----
		 * ⚠️ One profiling scope PER PASS, and they nest under the owner's scope (the profiler
		 * tracks depth). This component is the second half of the most expensive effect in the
		 * frame, and until Sep 2026 the whole thing reported as ONE opaque number: the only way
		 * to tell the trace from the denoiser was an A/B on `Denoiser/Iterations`. A ScopedZone
		 * on a null profiler is a no-op, so these cost nothing when the profiler is off. */
		auto * const profiler = this->renderer().gpuProfiler();

		{
			const Vulkan::GPUProfiler::ScopedZone profilingZone{profiler, commandBuffer, ClassId, "temporal"};

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_historyTargets[writeIdx],
				*m_temporalPipeline,
				*m_temporalLayout,
				*m_temporalPerFrame[frameIndex],
				nullptr,
				0
			);
		}

		{
			const Vulkan::GPUProfiler::ScopedZone profilingZone{profiler, commandBuffer, ClassId, "moments"};

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_momentsTargets[writeIdx],
				*m_momentsPipeline,
				*m_momentsLayout,
				*m_momentsPerFrame[frameIndex],
				nullptr,
				0
			);
		}

		{
			const Vulkan::GPUProfiler::ScopedZone profilingZone{profiler, commandBuffer, ClassId, "normalHistory"};

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_normalHistoryTargets[writeIdx],
				*m_normalCopyPipeline,
				*m_normalCopyLayout,
				*m_normalCopyPerFrame[frameIndex],
				nullptr,
				0
			);
		}

		/* ---- À-trous iterations (variance-guided, footprint doubling) ---- */

		const TextureInterface * output = &m_historyTargets[writeIdx];

		if ( m_parameters.atrousIterations > 0 && m_atrousPipeline != nullptr )
		{
			/* The whole loop as ONE scope, not one per iteration: the iterations are identical
			 * in shape and only their step size differs, so a per-iteration split would add
			 * `atrousIterations` scopes to every frame to report the same number divided. The
			 * iteration COUNT is a setting (`Denoiser/Iterations`); divide if you need it. */
			const Vulkan::GPUProfiler::ScopedZone profilingZone{profiler, commandBuffer, ClassId, "atrous"};

			/* The first iteration reads the freshly resolved history, whose parity flips
			 * every frame — rewrite its input binding; the guides too. */
			static_cast< void >(m_atrousPerFrame[0][frameIndex]->writeCombinedImageSampler(0, m_historyTargets[writeIdx]));

			for ( size_t flavour = 0; flavour < m_atrousPerFrame.size(); ++flavour )
			{
				static_cast< void >(m_atrousPerFrame[flavour][frameIndex]->writeCombinedImageSampler(1, m_momentsTargets[writeIdx]));

				if ( context.depth != nullptr )
				{
					static_cast< void >(m_atrousPerFrame[flavour][frameIndex]->writeCombinedImageSampler(2, *context.depth));
				}

				if ( context.normals != nullptr )
				{
					static_cast< void >(m_atrousPerFrame[flavour][frameIndex]->writeCombinedImageSampler(3, *context.normals));
				}
			}

			for ( uint32_t iteration = 0; iteration < m_parameters.atrousIterations; ++iteration )
			{
				/* Iteration 0 reads the resolved history (set flavour 0); iteration i > 0
				 * reads atrous[(i-1) & 1] (set flavour 1 + ((i-1) & 1)). Output ping-pongs. */
				const size_t flavour = iteration == 0 ? 0 : 1 + ((iteration - 1U) & 1U);
				const uint32_t outputIdx = iteration & 1U;

				const AtrousPushConstants constants{
					.stepSize = static_cast< float >(1U << iteration),
					.depthSigma = m_parameters.depthSigma,
					.normalSigma = m_parameters.normalSigma,
					.luminanceSigma = m_parameters.luminanceSigma,
					.firstIteration = iteration == 0 ? 1.0F : 0.0F,
					.padding0 = 0.0F,
					.padding1 = 0.0F,
					.padding2 = 0.0F
				};

				IndirectPostProcessEffect::recordFullscreenPass(
					commandBuffer,
					m_atrousTargets[outputIdx],
					*m_atrousPipeline,
					*m_atrousLayout,
					*m_atrousPerFrame[flavour][frameIndex],
					&constants,
					sizeof(AtrousPushConstants)
				);

				output = &m_atrousTargets[outputIdx];
			}
		}

		/* Flip the history ping-pong for the next frame. */
		m_historyWriteIndex = readIdx;
		m_historyValid = true;

		/* The combine consumes the à-trous output (the freshly resolved history when the
		 * spatial filter is disabled). The colour history fed back next frame remains the
		 * TEMPORAL output — the multi-bounce algebra is unchanged by the spatial filter. */
		return output;
	}
}
