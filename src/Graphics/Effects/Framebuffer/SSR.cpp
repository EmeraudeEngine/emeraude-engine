/*
 * src/Graphics/Effects/Framebuffer/SSR.cpp
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

#include "SSR.hpp"

/* STL inclusions. */
#include <bit>
#include <cmath>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Graphics/ViewMatricesInterface.hpp"
#include "PrimaryServices.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Tracer.hpp"
#include "Vulkan/ShaderModule.hpp"
#include "Vulkan/Sync/ImageMemoryBarrier.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/PipelineLayout.hpp"

namespace
{
	using namespace EmEn;

	/* Hi-Z pyramid: mip 0 = scene depth copy. */
	constexpr auto SSRHiZCopyComputeShader = R"GLSL(
#version 450

layout(local_size_x = 8, local_size_y = 8) in;

layout(set = 0, binding = 0) uniform sampler2D srcDepth;
layout(set = 0, binding = 1, r32f) uniform writeonly image2D dstMip;

layout(push_constant) uniform PushConstants
{
	int destWidth;
	int destHeight;
	int sourceMaxX;
	int sourceMaxY;
};

void main()
{
	ivec2 p = ivec2(gl_GlobalInvocationID.xy);

	if (p.x >= destWidth || p.y >= destHeight)
	{
		return;
	}

	imageStore(dstMip, p, vec4(texelFetch(srcDepth, min(p, ivec2(sourceMaxX, sourceMaxY)), 0).r));
}
)GLSL";

	/* Hi-Z pyramid: mip N = MIN 2x2 of mip N-1. The MIN keeps the pyramid CONSERVATIVE:
	 * a coarse cell can never report farther than its nearest content, so the hierarchical
	 * traversal never tunnels through geometry (Uludag, GPU Pro 5). */
	constexpr auto SSRHiZReduceComputeShader = R"GLSL(
#version 450

layout(local_size_x = 8, local_size_y = 8) in;

layout(set = 0, binding = 0) uniform sampler2D srcMip;
layout(set = 0, binding = 1, r32f) uniform writeonly image2D dstMip;

layout(push_constant) uniform PushConstants
{
	int destWidth;
	int destHeight;
	int sourceMaxX;
	int sourceMaxY;
};

void main()
{
	ivec2 p = ivec2(gl_GlobalInvocationID.xy);

	if (p.x >= destWidth || p.y >= destHeight)
	{
		return;
	}

	ivec2 s = p * 2;
	ivec2 sMax = ivec2(sourceMaxX, sourceMaxY);

	float d0 = texelFetch(srcMip, min(s, sMax), 0).r;
	float d1 = texelFetch(srcMip, min(s + ivec2(1, 0), sMax), 0).r;
	float d2 = texelFetch(srcMip, min(s + ivec2(0, 1), sMax), 0).r;
	float d3 = texelFetch(srcMip, min(s + ivec2(1, 1), sMax), 0).r;

	imageStore(dstMip, p, vec4(min(min(d0, d1), min(d2, d3))));
}
)GLSL";

	/* Color pyramid downsample: 4 bilinear taps at the corners of the destination texel's
	 * source footprint — a 4x4 tent. Repeated across the chain it converges toward the
	 * gaussian pre-convolution the cone lookup interpolates (Uludag, GPU Pro 5). The same
	 * shader builds mip 0 (input color -> half res) and every subsequent mip. */
	constexpr auto SSRColorDownsampleComputeShader = R"GLSL(
#version 450

layout(local_size_x = 8, local_size_y = 8) in;

layout(set = 0, binding = 0) uniform sampler2D srcColor;
layout(set = 0, binding = 1, rgba16f) uniform writeonly image2D dstMip;

layout(push_constant) uniform PushConstants
{
	int destWidth;
	int destHeight;
	int sourceMaxX;
	int sourceMaxY;
};

void main()
{
	ivec2 p = ivec2(gl_GlobalInvocationID.xy);

	if (p.x >= destWidth || p.y >= destHeight)
	{
		return;
	}

	vec2 srcSize = vec2(float(sourceMaxX + 1), float(sourceMaxY + 1));
	vec2 invSrc = 1.0 / srcSize;
	/* Center of the destination texel's 2x2 source footprint. */
	vec2 uv = (vec2(p) * 2.0 + 1.0) * invSrc;

	vec3 color = 0.25 * (
		texture(srcColor, uv + vec2(-0.5, -0.5) * invSrc).rgb +
		texture(srcColor, uv + vec2( 0.5, -0.5) * invSrc).rgb +
		texture(srcColor, uv + vec2(-0.5,  0.5) * invSrc).rgb +
		texture(srcColor, uv + vec2( 0.5,  0.5) * invSrc).rgb);

	imageStore(dstMip, p, vec4(color, 1.0));
}
)GLSL";

	constexpr auto SSRTraceFragmentShader = R"GLSL(
#version 450

/* Hi-Z screen-space ray tracing (Uludag, "Hi-Z Screen-Space Cone-Traced Reflections",
 * GPU Pro 5 — the UE-class traversal). The ray walks a MIN-depth pyramid: coarse mips are
 * skipped in exponential steps while the ray is in front of everything, refinement descends
 * only where a hit is possible, convergence lands at mip 0 with pixel precision. No linear
 * stride, no per-step thickness heuristic — the banding of the former linear march cannot
 * exist by construction. */
layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outHit;

layout(set = 0, binding = 0) uniform sampler2D normalTex;
layout(set = 0, binding = 1) uniform sampler2D hiZTex;

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
	float fadeScreenEdge;
	uint maxSteps;
	uint hiZMaxLevel;
	uint padding;
};

/* Linearize depth from [0,1] range (Vulkan [0,1] depth convention). */
float linearizeDepth (float depth)
{
	return (nearPlane * farPlane) / (farPlane - depth * (farPlane - nearPlane));
}

/* Inverse of linearizeDepth: view-space Z to [0,1] depth-buffer value. */
float delinearizeDepth (float viewZ)
{
	return (farPlane * (viewZ - nearPlane)) / (viewZ * (farPlane - nearPlane));
}

/* Reconstruct view-space position from UV and depth. */
vec3 reconstructPosition (vec2 uv, float depth)
{
	float linearZ = linearizeDepth(depth);
	vec2 ndc = uv * 2.0 - 1.0;
	float t = abs(tanHalfFovY);
	return vec3(ndc * vec2(t * aspectRatio, t) * linearZ, linearZ);
}

/* Project view-space position back to screen UV. */
vec2 projectToUV (vec3 viewPos)
{
	float t = abs(tanHalfFovY);
	vec2 ndc = viewPos.xy / (viewPos.z * vec2(t * aspectRatio, t));
	return ndc * 0.5 + 0.5;
}

/* Screen-edge fade: 0 at edges, 1 at center. */
float screenEdgeFade (vec2 uv)
{
	vec2 fade = smoothstep(vec2(0.0), vec2(fadeScreenEdge), uv)
			  * smoothstep(vec2(0.0), vec2(fadeScreenEdge), vec2(1.0) - uv);
	return fade.x * fade.y;
}

/* Interleaved gradient noise (Jimenez) — per-pixel ray start jitter. */
float interleavedGradientNoise (vec2 pixel)
{
	return fract(52.9829189 * fract(dot(pixel, vec2(0.06711056, 0.00583715))));
}

void main()
{
	vec2 mip0Size = vec2(textureSize(hiZTex, 0));

	/* texelFetch (no bilinear filtering): depth/normals interpolated across geometric
	 * edges produce smeared silhouettes. Mip 0 of the pyramid IS the scene depth. */
	ivec2 centerCoord = ivec2(vUV * mip0Size);
	float centerDepth = texelFetch(hiZTex, centerCoord, 0).r;

	/* Skip far-plane fragments. */
	if (centerDepth >= 1.0)
	{
		outHit = vec4(0.0);
		return;
	}

	vec3 viewPos = reconstructPosition(vUV, centerDepth);

	/* Read view-space normal and packed roughness+metalness from MRT.
	 * Alpha encoding: alpha = roughness + metalness * 2.0
	 * Decode: metalness = (alpha >= 2.0) ? 1.0 : 0.0; roughness = alpha - metalness * 2.0; */
	vec4 normalData = texelFetch(normalTex, ivec2(vUV * vec2(textureSize(normalTex, 0))), 0);
	vec3 rawN = normalData.rgb;
	float packedRM = normalData.a;
	float roughness = packedRM >= 2.0 ? packedRM - 2.0 : packedRM;

	if (dot(rawN, rawN) < 0.0001)
	{
		outHit = vec4(0.0);
		return;
	}

	/* Skip expensive traversal for very rough surfaces only: the cone-traced resolve
	 * turns mid-roughness hits into physically blurred reflections (they used to be
	 * faded out at 0.5 because the sharp fetch looked wrong on them). */
	if (roughness > 0.85)
	{
		outHit = vec4(0.0);
		return;
	}

	vec3 normal = normalize(vec3(rawN.x, rawN.y, -rawN.z));

	/* Compute reflection direction in reconstruction space. */
	vec3 viewDir = normalize(viewPos);
	vec3 reflDir = reflect(viewDir, normal);

	/* Rays toward the camera cannot be resolved against a single depth layer. */
	if (reflDir.z < 0.0)
	{
		outHit = vec4(0.0);
		return;
	}

	/* ---- Ray endpoints in Hi-Z space: xy in mip-0 TEXELS, z in [0,1] depth. A projective
	 * transform maps 3D lines to lines, so linear interpolation here is exact. ---- */
	vec3 viewEnd = viewPos + reflDir * maxDistance;

	vec3 P0 = vec3(vUV * mip0Size, centerDepth);
	vec3 P1 = vec3(projectToUV(viewEnd) * mip0Size, delinearizeDepth(viewEnd.z));
	vec3 D = P1 - P0;

	/* Guard against degenerate screen motion. */
	vec2 absD = abs(D.xy);
	if (max(absD.x, absD.y) < 0.01)
	{
		outHit = vec4(0.0);
		return;
	}

	vec2 invD = vec2(
		abs(D.x) > 1e-6 ? 1.0 / D.x : 1e18,
		abs(D.y) > 1e-6 ? 1.0 / D.y : 1e18
	);
	/* Epsilon nudging the parametric point into the NEXT cell at a boundary crossing. */
	float tEpsilon = 0.05 / max(absD.x, absD.y);

	/* Start offset: 1-2 texels along the ray, jittered per pixel (interleaved gradient
	 * noise) — avoids self-intersection and breaks sub-pixel aliasing into fine noise
	 * the bilateral blur absorbs. */
	float t = (1.0 + interleavedGradientNoise(gl_FragCoord.xy)) / max(absD.x, absD.y);

	int level = 1;
	const int maxLevel = int(hiZMaxLevel);
	bool hit = false;
	vec3 P = P0 + D * t;

	for (uint i = 0u; i < maxSteps; ++i)
	{
		if (t > 1.0 || level < 0)
		{
			break;
		}

		P = P0 + D * t;

		if (P.x < 0.0 || P.y < 0.0 || P.x >= mip0Size.x || P.y >= mip0Size.y)
		{
			break;
		}

		float cell = float(1 << level);
		vec2 cellIdx = floor(P.xy / cell);
		float minZ = texelFetch(hiZTex, ivec2(cellIdx), level).r;

		/* Parametric exit of the current cell. */
		vec2 boundary = (cellIdx + step(vec2(0.0), D.xy)) * cell;
		vec2 tB2 = (boundary - P0.xy) * invD;
		float tBoundary = min(tB2.x, tB2.y) + tEpsilon;

		/* Parametric crossing of this cell's min-depth plane. */
		float tPlane = D.z > 1e-12 ? (minZ - P0.z) / D.z : 1e18;

		if (P.z < minZ && tPlane >= tBoundary)
		{
			/* The whole span through this cell stays in FRONT of everything it contains:
			 * free flight — jump to the cell exit and coarsen. */
			t = tBoundary;
			level = min(level + 1, maxLevel);

			continue;
		}

		if (level > 0)
		{
			/* Potential intersection inside this cell: advance to the min-depth crossing
			 * (never past the cell exit) and REFINE. */
			if (P.z < minZ)
			{
				t = clamp(tPlane, t, tBoundary - tEpsilon);
			}

			level--;

			continue;
		}

		/* Level 0: exact texel. Advance to the crossing and classify with the
		 * view-space thickness (the only heuristic left — it classifies BEHIND
		 * from IN CONTACT, it no longer drives the march). */
		if (P.z < minZ)
		{
			t = clamp(tPlane, t, tBoundary - tEpsilon);
			P = P0 + D * t;
		}

		float rayLin = linearizeDepth(P.z);
		float sceneLin = linearizeDepth(minZ);
		float diff = rayLin - sceneLin;
		float adaptiveThickness = thickness * max(1.0, sceneLin * 0.05);

		if (diff >= -0.001 && diff <= adaptiveThickness)
		{
			hit = true;

			break;
		}

		/* Passed BEHIND thick geometry: step out of this texel and resume coarse. */
		t = tBoundary;
		level = 1;
	}

	/* Compute confidence from multiple fade factors. */
	float confidence = 0.0;
	vec2 hitUV = vec2(0.0);

	if (hit)
	{
		hitUV = P.xy / mip0Size;

		vec3 viewHit = reconstructPosition(hitUV, P.z);
		float rayDist = distance(viewHit, viewPos);

		/* Distance fade. */
		float distFade = 1.0 - clamp(rayDist / maxDistance, 0.0, 1.0);

		/* Screen edge fade. */
		float edgeFade = screenEdgeFade(hitUV);

		/* Facing fade: reflections nearly parallel to the view direction are weak. */
		float facingFade = 1.0 - pow(max(0.0, dot(viewDir, reflDir)), 5.0);

		/* Roughness fade: with the cone-traced resolve, mid-roughness reflections are
		 * BLURRED instead of suppressed — the fade only retires the truly diffuse tail
		 * where the specular lobe carries no readable image. */
		float roughnessFade = 1.0 - smoothstep(0.55, 0.85, roughness);

		confidence = distFade * edgeFade * facingFade * roughnessFade;
	}

	outHit = vec4(hitUV, confidence, 0.0);
}
)GLSL";

	constexpr auto SSRBlurFragmentShader = R"GLSL(
#version 450

/* Bilateral blur — depth/normal-aware separable filter, radius scaled by the surface
 * roughness (ported from RTR): a polished surface keeps a mirror-sharp reflection, a rough
 * one gets the full spread. Replaces the former fixed 5-tap gaussian that smeared everything
 * equally whatever the material. */
layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outBlur;

layout(set = 0, binding = 0) uniform sampler2D inputTex;
layout(set = 0, binding = 1) uniform sampler2D depthTex;
layout(set = 0, binding = 2) uniform sampler2D normalTex;

layout(push_constant) uniform PushConstants
{
	float texelSizeX;
	float texelSizeY;
	float directionX;
	float directionY;
	float depthSigma;
	float normalSigma;
	int blurRadius;
	float padding;
};

void main()
{
	vec2 texelSize = vec2(texelSizeX, texelSizeY);
	vec2 dir = vec2(directionX, directionY);

	vec4 centerVal = texture(inputTex, vUV);
	float centerDepth = texture(depthTex, vUV).r;
	vec3 centerNormal = texture(normalTex, vUV).rgb;

	if (centerDepth >= 1.0)
	{
		outBlur = centerVal;
		return;
	}

	vec4 result = vec4(0.0);
	float totalWeight = 0.0;

	/* Roughness-scaled radius: polished surfaces keep mirror-sharp reflections. */
	float packedRM = texture(normalTex, vUV).a;
	float centerRoughness = packedRM >= 2.0 ? packedRM - 2.0 : packedRM;
	int effectiveRadius = max(1, int(float(blurRadius) * smoothstep(0.02, 0.5, centerRoughness)));

	float spatialSigma = float(effectiveRadius) * 0.5;
	float invSpatialSigma2 = 1.0 / (2.0 * spatialSigma * spatialSigma);
	float invDepthSigma2 = 1.0 / (2.0 * depthSigma * depthSigma);

	for (int i = -effectiveRadius; i <= effectiveRadius; i++)
	{
		vec2 sampleUV = vUV + dir * texelSize * float(i);
		vec4 sampleVal = texture(inputTex, sampleUV);
		float sampleDepth = texture(depthTex, sampleUV).r;
		vec3 sampleNormal = texture(normalTex, sampleUV).rgb;

		float spatialW = exp(-float(i * i) * invSpatialSigma2);
		float depthDiff = abs(centerDepth - sampleDepth);
		float depthW = exp(-depthDiff * depthDiff * invDepthSigma2);
		float normalDot = max(dot(centerNormal, sampleNormal), 0.0);
		float normalW = pow(normalDot, 1.0 / max(normalSigma, 0.001));

		float w = spatialW * depthW * normalW;
		result += sampleVal * w;
		totalWeight += w;
	}

	outBlur = (totalWeight > 0.0) ? result / totalWeight : centerVal;
}
)GLSL";

	/* Resolve pass: reads the trace hit data and the scene color,
	 * outputs the reflected color weighted by confidence.
	 * This converts (hitUV, confidence) into (reflectedColor * confidence, confidence)
	 * so that the subsequent blur operates on colors, not UV coordinates.
	 * When confidence is zero (SSR miss), falls back to sampling the environment cubemap. */
	constexpr auto SSRResolveFragmentShader = R"GLSL(
#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outResolve;

layout(set = 0, binding = 0) uniform sampler2D colorTex;
layout(set = 0, binding = 1) uniform sampler2D traceTex;
layout(set = 0, binding = 2) uniform sampler2D depthTex;
layout(set = 0, binding = 3) uniform sampler2D normalTex;
/* Pre-convolved color pyramid (half-res base): the cone lookup source. */
layout(set = 0, binding = 4) uniform sampler2D pyramidTex;

/* Bindless textures (set 1): the reserved cube slot 2 holds the ACTIVE SCENE's
 * GGX-prefiltered environment, re-baked at every background switch (the old dedicated
 * envCubemap binding never had a caller — the fallback was a black cubemap forever). */
layout(set = 1, binding = 3) uniform samplerCube texturesCube[];

const uint PrefilteredCubemapSlot = 2u;
const float PrefilteredMaxLod = 5.0;

layout(push_constant) uniform PushConstants
{
	vec4 invViewCol0;
	vec4 invViewCol1;
	vec4 invViewCol2;
	float texelSizeX, texelSizeY;
	float nearPlane, farPlane;
	float tanHalfFovY, aspectRatio;
	float envFallbackIntensity;
	float intensity;
	float pyramidLodOffset;
	float pyramidMaxLod;
};

float linearizeDepth (float depth)
{
	return (nearPlane * farPlane) / (farPlane - depth * (farPlane - nearPlane));
}

void main()
{
	vec4 traceData = texture(traceTex, vUV);
	float confidence = traceData.z;

	if (confidence > 0.001)
	{
		/* Cone-traced glossy resolve (Uludag, GPU Pro 5 — the second half of the Hi-Z
		 * chapter): a rough surface reflects a CONE, not a line. Its footprint at the hit
		 * is the GGX lobe tangent (alpha = roughness²) times the marched screen distance;
		 * the pre-convolved pyramid is read at the matching LOD. Mirror-sharp rays
		 * (cone < 1 trace texel) keep the full-res color fetch — zero regression. */
		float packedRM = texture(normalTex, vUV).a;
		float roughness = packedRM >= 2.0 ? packedRM - 2.0 : packedRM;
		float coneTan = roughness * roughness;
		vec2 deltaTexels = (traceData.xy - vUV) / vec2(texelSizeX, texelSizeY);
		float coneWidthTexels = 2.0 * coneTan * length(deltaTexels);

		vec3 reflColor;

		if (coneWidthTexels <= 1.0)
		{
			reflColor = texture(colorTex, traceData.xy).rgb;
		}
		else
		{
			float pyramidLOD = clamp(log2(coneWidthTexels) + pyramidLodOffset, 0.0, pyramidMaxLod);
			vec3 sharpColor = texture(colorTex, traceData.xy).rgb;
			vec3 coneColor = textureLod(pyramidTex, traceData.xy, max(pyramidLOD, 0.0)).rgb;

			/* Fade in the pyramid over cone width 1..2 texels: hides the half-res step. */
			reflColor = mix(sharpColor, coneColor, clamp(coneWidthTexels - 1.0, 0.0, 1.0));
		}

		outResolve = vec4(reflColor, confidence);
	}
	else if (envFallbackIntensity > 0.0)
	{
		/* No SSR hit: cubemap fallback. */
		float depth = texture(depthTex, vUV).r;

		if (depth >= 1.0)
		{
			outResolve = vec4(0.0);
			return;
		}

		/* Read packed roughness+metalness to modulate cubemap fallback.
		 * Decode: metalness = (alpha >= 2.0) ? 1.0 : 0.0; roughness = alpha - metalness * 2.0; */
		float packedRM = texture(normalTex, vUV).a;
		float roughness = packedRM >= 2.0 ? packedRM - 2.0 : packedRM;

		/* Reconstruct view-space position (standard: Z negative = into screen). */
		float linearZ = linearizeDepth(depth);
		vec2 ndc = vUV * 2.0 - 1.0;
		float t = abs(tanHalfFovY);
		vec3 viewPos = vec3(ndc.x * t * aspectRatio * linearZ,
							ndc.y * t * linearZ, -linearZ);

		/* Read view-space normal from MRT. */
		vec3 rawN = texture(normalTex, vUV).rgb;

		if (dot(rawN, rawN) < 0.001)
		{
			outResolve = vec4(0.0);
			return;
		}

		vec3 normal = normalize(rawN);

		/* Reflection in view space, then transform to world space for cubemap lookup. */
		vec3 reflDir = reflect(normalize(viewPos), normal);
		mat3 invViewRot = mat3(invViewCol0.xyz, invViewCol1.xyz, invViewCol2.xyz);
		vec3 worldReflDir = invViewRot * reflDir;

		/* ENGINE CUBEMAP CONVENTION: world direction D samples at vec3(D.x, -D.y, D.z)
		 * (engine UP = -Y, cubemap stored Y-up) — same as skybox/material reflections.
		 * The prefiltered chain handles the roughness (the old smoothstep attenuation
		 * compensated a mirror-only sample); mip 0 is an exact environment copy. */
		vec3 envColor = textureLod(texturesCube[nonuniformEXT(PrefilteredCubemapSlot)], vec3(worldReflDir.x, -worldReflDir.y, worldReflDir.z), clamp(roughness, 0.0, 1.0) * PrefilteredMaxLod).rgb;
		outResolve = vec4(envColor, envFallbackIntensity);
	}
	else
	{
		outResolve = vec4(0.0);
	}
}
)GLSL";

	/* Composite pass: blends the blurred reflected color with the scene,
	 * modulated by the per-pixel reflectivity from the material properties G-buffer. */
	constexpr auto SSRCompositeFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D colorTex;
layout(set = 0, binding = 1) uniform sampler2D ssrTex;
layout(set = 0, binding = 2) uniform sampler2D materialPropsTex;

layout(push_constant) uniform PushConstants
{
	float intensity;
	float padding1;
	float padding2;
	float padding3;
};

void main()
{
	vec4 color = texture(colorTex, vUV);
	vec4 ssrData = texture(ssrTex, vUV);

	/* Decode reflectivity from the material properties G-buffer (R channel, high nibble). */
	vec4 mp = texture(materialPropsTex, vUV);
	uint rPacked = uint(mp.r * 255.0);
	float reflectivity = float(rPacked >> 4u) / 15.0;

	/* ssrData.rgb = blurred reflected color, ssrData.a = blurred confidence. */
	float confidence = ssrData.a;

	if (confidence > 0.001 && reflectivity > 0.0)
	{
		color.rgb = mix(color.rgb, ssrData.rgb / max(confidence, 0.001), confidence * intensity * reflectivity);
	}

	outColor = color;
}
)GLSL";
}

namespace EmEn::Graphics::Effects::Framebuffer
{
	using namespace Base;
	using namespace Vulkan;
	using namespace Saphir;

	bool
	SSR::create (uint32_t width, uint32_t height) noexcept
	{
		auto & renderer = this->renderer();

		auto & settings = renderer.primaryServices().settings();

		/* Pixel doubling: half-res working targets to save performance. Default FALSE
		 * (owner decision): screen-space effects run full-res — they are the cheap tier of
		 * the reflection ladder, definition is their selling point. */
		const auto pixelDoubling = settings.getOrSetDefault< bool >(GraphicsScreenSpaceReflectionPixelDoublingKey, DefaultGraphicsScreenSpaceReflectionPixelDoubling);
		const auto halfW = pixelDoubling ? (width > 1 ? width / 2 : 1U) : width;
		const auto halfH = pixelDoubling ? (height > 1 ? height / 2 : 1U) : height;

		/* Bilateral blur quality knobs. */
		m_blurRadius = settings.getOrSetDefault< uint32_t >(GraphicsScreenSpaceReflectionBlurRadiusKey, DefaultGraphicsScreenSpaceReflectionBlurRadius);
		m_depthSigma = settings.getOrSetDefault< float >(GraphicsScreenSpaceReflectionDepthSigmaKey, DefaultGraphicsScreenSpaceReflectionDepthSigma);
		m_normalSigma = settings.getOrSetDefault< float >(GraphicsScreenSpaceReflectionNormalSigmaKey, DefaultGraphicsScreenSpaceReflectionNormalSigma);

		/* Trace target (half-res, RGBA16F: hitUV.xy + confidence.z). */
		if ( !m_traceTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "SSR_Trace") )
		{
			TraceError{ClassId} << "Failed to create SSR trace target !";

			return false;
		}

		/* Resolve target (half-res, RGBA16F: reflected color RGB + confidence A). */
		if ( !m_resolveTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "SSR_Resolve") )
		{
			TraceError{ClassId} << "Failed to create SSR resolve target !";

			return false;
		}

		/* Blur targets (half-res, RGBA16F). */
		if ( !m_blurHTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "SSR_BlurH") )
		{
			TraceError{ClassId} << "Failed to create SSR blur H target !";

			return false;
		}

		if ( !m_blurVTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "SSR_BlurV") )
		{
			TraceError{ClassId} << "Failed to create SSR blur V target !";

			return false;
		}

		/* Composite target (full-res, RGBA16F). */
		if ( !m_outputTarget.create(renderer, width, height, VK_FORMAT_R16G16B16A16_SFLOAT, "SSR_Output") )
		{
			TraceError{ClassId} << "Failed to create SSR output target !";

			return false;
		}

		/* ---- Descriptor set layouts ---- */
		auto & layoutManager = renderer.layoutManager();

		/* Trace input (normals + Hi-Z pyramid): 2 combined image samplers at bindings 0,1. */
		auto traceInputLayout = this->getInputLayout(2);

		/* Blur input (color + depth + normals for the bilateral filter): 3 combined image samplers. */
		auto blurInputLayout = this->getInputLayout(3);

		/* Resolve input (color + trace + depth + normals + env cubemap): 5 bindings, custom. */
		auto resolveInputLayout = layoutManager.getDescriptorSetLayout("SSRResolveInput");

		if ( resolveInputLayout == nullptr )
		{
			resolveInputLayout = layoutManager.prepareNewDescriptorSetLayout("SSRResolveInput");
			resolveInputLayout->setIdentifier(ClassId, "SSRResolveInput", "DescriptorSetLayout");
			resolveInputLayout->declareCombinedImageSampler(0, VK_SHADER_STAGE_FRAGMENT_BIT);
			resolveInputLayout->declareCombinedImageSampler(1, VK_SHADER_STAGE_FRAGMENT_BIT);
			resolveInputLayout->declareCombinedImageSampler(2, VK_SHADER_STAGE_FRAGMENT_BIT);
			resolveInputLayout->declareCombinedImageSampler(3, VK_SHADER_STAGE_FRAGMENT_BIT);
			/* Binding 4: the pre-convolved color pyramid (cone-traced glossy lookup). */
			resolveInputLayout->declareCombinedImageSampler(4, VK_SHADER_STAGE_FRAGMENT_BIT);
			/* NOTE: The environment fallback reads the bindless prefiltered slot (set 1). */

			if ( !layoutManager.createDescriptorSetLayout(resolveInputLayout) )
			{
				return false;
			}
		}

		/* Composite input (color + blurred SSR + material properties): 3 combined image samplers at bindings 0,1,2. */
		auto compositeLayout = this->getInputLayout(3);

		if ( traceInputLayout == nullptr || blurInputLayout == nullptr || resolveInputLayout == nullptr || compositeLayout == nullptr )
		{
			return false;
		}

		/* ---- Pipeline layouts ---- */
		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(traceInputLayout);

			m_traceLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{
					.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
					.offset = 0,
					.size = sizeof(TracePushConstants)
				}
			});
		}

		{
			/* Resolve: set 0 = inputs, set 1 = bindless textures (prefiltered environment
			 * fallback, reserved cube slot 2). */
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(resolveInputLayout);
			sets.emplace_back(this->renderer().bindlessTextureManager().descriptorSetLayout());

			m_resolveLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{
					.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
					.offset = 0,
					.size = sizeof(ResolvePushConstants)
				}
			});
		}

		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(blurInputLayout);

			m_blurLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{
					.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
					.offset = 0,
					.size = sizeof(BlurPushConstants)
				}
			});
		}

		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(compositeLayout);

			m_compositeLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{
					.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
					.offset = 0,
					.size = sizeof(CompositePushConstants)
				}
			});
		}

		if ( m_traceLayout == nullptr || m_resolveLayout == nullptr || m_blurLayout == nullptr || m_compositeLayout == nullptr )
		{
			return false;
		}

		/* ---- Compile shaders ---- */
		auto & shaderManager = renderer.shaderManager();
		const auto & device = renderer.device();

		const auto vertexModule = this->getFullscreenVertexShader();
		const auto traceFragment = shaderManager.getShaderModuleFromSourceCode(device, "SSR_Trace_FS", ShaderType::FragmentShader, SSRTraceFragmentShader);
		const auto resolveFragment = shaderManager.getShaderModuleFromSourceCode(device, "SSR_Resolve_FS", ShaderType::FragmentShader, SSRResolveFragmentShader);
		const auto blurFragment = shaderManager.getShaderModuleFromSourceCode(device, "SSR_Blur_FS", ShaderType::FragmentShader, SSRBlurFragmentShader);
		const auto compositeFragment = shaderManager.getShaderModuleFromSourceCode(device, "SSR_Composite_FS", ShaderType::FragmentShader, SSRCompositeFragmentShader);

		if ( vertexModule == nullptr || traceFragment == nullptr || resolveFragment == nullptr || blurFragment == nullptr || compositeFragment == nullptr )
		{
			TraceError{ClassId} << "Failed to compile SSR shaders !";

			return false;
		}

		/* ---- Create pipelines ---- */
		m_tracePipeline = this->createFullscreenPipeline(ClassId, "SSR_Trace", vertexModule, traceFragment, m_traceLayout, m_traceTarget);
		m_resolvePipeline = this->createFullscreenPipeline(ClassId, "SSR_Resolve", vertexModule, resolveFragment, m_resolveLayout, m_resolveTarget);
		m_blurPipeline = this->createFullscreenPipeline(ClassId, "SSR_Blur", vertexModule, blurFragment, m_blurLayout, m_blurHTarget);
		m_compositePipeline = this->createFullscreenPipeline(ClassId, "SSR_Composite", vertexModule, compositeFragment, m_compositeLayout, m_outputTarget);

		if ( m_tracePipeline == nullptr || m_resolvePipeline == nullptr || m_blurPipeline == nullptr || m_compositePipeline == nullptr )
		{
			return false;
		}

		/* ---- Create descriptor sets ---- */
		/* Trace: reads depth + normals (updated per-frame). */
		m_tracePerFrame = this->createPerFrameDescriptorSets(traceInputLayout, ClassId, "Trace_DescSet");

		if ( m_tracePerFrame.empty() )
		{
			return false;
		}

		/* Resolve: reads color (binding 0, per-frame), trace (binding 1, fixed),
		 * depth (binding 2, per-frame), normals (binding 3, per-frame); the environment
		 * fallback reads the bindless prefiltered slot (set 1, always current). */
		{
			m_resolvePerFrame = this->createPerFrameDescriptorSets(resolveInputLayout, ClassId, "Resolve_DescSet");

			if ( m_resolvePerFrame.empty() )
			{
				return false;
			}

			for ( const auto & descriptorSet : m_resolvePerFrame )
			{
				/* Binding 1: trace result (same target every frame). */
				if ( !descriptorSet->writeCombinedImageSampler(1, m_traceTarget) )
				{
					return false;
				}
			}
		}

		/* Blur H: reads resolve result (fixed) + depth/normals (per-frame, bilateral filter). */
		m_blurHPerFrame = this->createPerFrameDescriptorSets(blurInputLayout, ClassId, "BlurH_DescSet");

		if ( m_blurHPerFrame.empty() )
		{
			return false;
		}

		for ( const auto & descriptorSet : m_blurHPerFrame )
		{
			if ( !descriptorSet->writeCombinedImageSampler(0, m_resolveTarget) )
			{
				return false;
			}
		}

		/* Blur V: reads blur H result (fixed) + depth/normals (per-frame). */
		m_blurVPerFrame = this->createPerFrameDescriptorSets(blurInputLayout, ClassId, "BlurV_DescSet");

		if ( m_blurVPerFrame.empty() )
		{
			return false;
		}

		for ( const auto & descriptorSet : m_blurVPerFrame )
		{
			if ( !descriptorSet->writeCombinedImageSampler(0, m_blurHTarget) )
			{
				return false;
			}
		}

		/* Composite: reads color (updated per-frame) + blurred SSR (fixed). */
		m_compositePerFrame = this->createPerFrameDescriptorSets(compositeLayout, ClassId, "Composite_DescSet");

		if ( m_compositePerFrame.empty() )
		{
			return false;
		}

		for ( const auto & descriptorSet : m_compositePerFrame )
		{
			/* Binding 1: blurred SSR (same for all frames). */
			if ( !descriptorSet->writeCombinedImageSampler(1, m_blurVTarget) )
			{
				return false;
			}
		}

		/* ---- Hi-Z depth pyramid (min-reduction mip chain) ---- */
		{
			const auto & localDevice = renderer.device();

			m_hiZMipCount = 1U;

			for ( uint32_t size = std::max(halfW, halfH); size > 1U; size >>= 1U )
			{
				m_hiZMipCount++;
			}

			m_hiZImage = std::make_shared< Image >(
				localDevice,
				VK_IMAGE_TYPE_2D,
				VK_FORMAT_R32_SFLOAT,
				VkExtent3D{
					.width = halfW,
					.height = halfH,
					.depth = 1U
				},
				VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				0,
				m_hiZMipCount
			);
			m_hiZImage->setIdentifier(ClassId, "HiZPyramid", "Image");

			if ( !m_hiZImage->createOnHardware() )
			{
				TraceError{ClassId} << "Failed to create the Hi-Z pyramid image !";

				return false;
			}

			/* Per-mip views (storage writes + reduce reads) and the full-chain view (trace). */
			m_hiZMipViews.reserve(m_hiZMipCount);

			for ( uint32_t mip = 0; mip < m_hiZMipCount; mip++ )
			{
				auto view = std::make_shared< ImageView >(
					m_hiZImage,
					VK_IMAGE_VIEW_TYPE_2D,
					VkImageSubresourceRange{
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.baseMipLevel = mip,
						.levelCount = 1U,
						.baseArrayLayer = 0U,
						.layerCount = 1U
					}
				);
				view->setIdentifier(ClassId, "HiZMip" + std::to_string(mip), "ImageView");

				if ( !view->createOnHardware() )
				{
					return false;
				}

				m_hiZMipViews.emplace_back(view);
			}

			m_hiZFullView = std::make_shared< ImageView >(
				m_hiZImage,
				VK_IMAGE_VIEW_TYPE_2D,
				VkImageSubresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0U,
					.levelCount = m_hiZMipCount,
					.baseArrayLayer = 0U,
					.layerCount = 1U
				}
			);
			m_hiZFullView->setIdentifier(ClassId, "HiZFull", "ImageView");

			if ( !m_hiZFullView->createOnHardware() )
			{
				return false;
			}

			/* NEAREST/clamp sampler: the traversal reads exact texels per mip. */
			m_hiZSampler = renderer.getSampler("SSRHiZ", [this] (Settings &, VkSamplerCreateInfo & createInfo) {
				createInfo.magFilter = VK_FILTER_NEAREST;
				createInfo.minFilter = VK_FILTER_NEAREST;
				createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
				createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				createInfo.anisotropyEnable = VK_FALSE;
				createInfo.maxLod = static_cast< float >(m_hiZMipCount);
			});

			if ( m_hiZSampler == nullptr )
			{
				return false;
			}

			/* Compute pipelines (IBLBaker pattern): binding 0 = source sampler, 1 = dest storage. */
			m_hiZDSLayout = std::make_shared< DescriptorSetLayout >(localDevice, "SSRHiZDSLayout");

			{
				VkDescriptorSetLayoutBinding binding{};
				binding.binding = 0;
				binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				binding.descriptorCount = 1;
				binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
				m_hiZDSLayout->declare(binding);
			}

			{
				VkDescriptorSetLayoutBinding binding{};
				binding.binding = 1;
				binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				binding.descriptorCount = 1;
				binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
				m_hiZDSLayout->declare(binding);
			}

			if ( !m_hiZDSLayout->createOnHardware() )
			{
				return false;
			}

			VkPushConstantRange pushConstantRange{};
			pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			pushConstantRange.offset = 0;
			pushConstantRange.size = sizeof(HiZPushConstants);

			m_hiZPipelineLayout = std::make_shared< PipelineLayout >(
				localDevice, "SSRHiZPipelineLayout",
				StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 >{m_hiZDSLayout},
				StaticVector< VkPushConstantRange, 4 >{pushConstantRange}
			);

			if ( !m_hiZPipelineLayout->createOnHardware() )
			{
				return false;
			}

			const auto copyModule = shaderManager.getShaderModuleFromSourceCode(localDevice, "SSR_HiZCopy_CS", ShaderType::ComputeShader, SSRHiZCopyComputeShader);
			const auto reduceModule = shaderManager.getShaderModuleFromSourceCode(localDevice, "SSR_HiZReduce_CS", ShaderType::ComputeShader, SSRHiZReduceComputeShader);

			if ( copyModule == nullptr || reduceModule == nullptr )
			{
				TraceError{ClassId} << "Failed to compile the Hi-Z compute shaders !";

				return false;
			}

			m_hiZCopyPipeline = std::make_unique< ComputePipeline >(m_hiZPipelineLayout);
			m_hiZCopyPipeline->setShaderModule(copyModule->handle());

			m_hiZReducePipeline = std::make_unique< ComputePipeline >(m_hiZPipelineLayout);
			m_hiZReducePipeline->setShaderModule(reduceModule->handle());

			if ( !m_hiZCopyPipeline->createOnHardware() || !m_hiZReducePipeline->createOnHardware() )
			{
				return false;
			}

			/* Descriptor sets: per-frame copy set (scene depth changes descriptor per frame),
			 * fixed reduce set per destination mip. The pool also carries the COLOR pyramid
			 * sets created below (per-frame copy + fixed per-mip downsample). */
			const auto framesInFlight = renderer.framesInFlight();

			/* Color pyramid dimensioning: half-res base, chain down to ~8 px. */
			const uint32_t pyramidBaseW = std::max(1U, width / 2U);
			const uint32_t pyramidBaseH = std::max(1U, height / 2U);

			m_colorPyramidMipCount = std::clamp(static_cast< uint32_t >(std::bit_width(std::min(pyramidBaseW, pyramidBaseH))) - 3U, 1U, 8U);

			const uint32_t computeSetCount = (framesInFlight * 2U) + m_hiZMipCount + m_colorPyramidMipCount;

			const std::vector< VkDescriptorPoolSize > poolSizes{
				{.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = computeSetCount},
				{.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = computeSetCount}
			};

			m_hiZDescriptorPool = std::make_shared< DescriptorPool >(localDevice, poolSizes, computeSetCount, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

			if ( !m_hiZDescriptorPool->createOnHardware() )
			{
				return false;
			}

			/* Raw descriptor writes (IBLBaker pattern): the storage destination has no
			 * helper, and the pyramid is SAMPLED in GENERAL layout during the reduction
			 * chain — the layouts must be exact. */
			const auto writeStorageDest = [&localDevice] (const DescriptorSet & descriptorSet, const ImageView & destView) {
				VkDescriptorImageInfo destInfo{};
				destInfo.sampler = VK_NULL_HANDLE;
				destInfo.imageView = destView.handle();
				destInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

				VkWriteDescriptorSet write{};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.dstSet = descriptorSet.handle();
				write.dstBinding = 1;
				write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				write.descriptorCount = 1;
				write.pImageInfo = &destInfo;

				vkUpdateDescriptorSets(localDevice->handle(), 1, &write, 0, nullptr);
			};

			const auto writeSampledSource = [&localDevice, this] (const DescriptorSet & descriptorSet, const ImageView & sourceView, VkImageLayout layout) {
				VkDescriptorImageInfo sourceInfo{};
				sourceInfo.sampler = m_hiZSampler->handle();
				sourceInfo.imageView = sourceView.handle();
				sourceInfo.imageLayout = layout;

				VkWriteDescriptorSet write{};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.dstSet = descriptorSet.handle();
				write.dstBinding = 0;
				write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				write.descriptorCount = 1;
				write.pImageInfo = &sourceInfo;

				vkUpdateDescriptorSets(localDevice->handle(), 1, &write, 0, nullptr);
			};

			m_hiZCopyPerFrame.reserve(framesInFlight);

			for ( uint32_t frame = 0; frame < framesInFlight; frame++ )
			{
				auto descriptorSet = std::make_unique< DescriptorSet >(m_hiZDescriptorPool, m_hiZDSLayout);
				descriptorSet->setIdentifier(ClassId, "HiZCopy_DescSet" + std::to_string(frame), "DescriptorSet");

				if ( !descriptorSet->create() )
				{
					return false;
				}

				writeStorageDest(*descriptorSet, *m_hiZMipViews[0]);

				m_hiZCopyPerFrame.emplace_back(std::move(descriptorSet));
			}

			m_hiZReduceSets.reserve(m_hiZMipCount);

			for ( uint32_t mip = 1; mip < m_hiZMipCount; mip++ )
			{
				auto descriptorSet = std::make_unique< DescriptorSet >(m_hiZDescriptorPool, m_hiZDSLayout);
				descriptorSet->setIdentifier(ClassId, "HiZReduce_DescSet" + std::to_string(mip), "DescriptorSet");

				if ( !descriptorSet->create() )
				{
					return false;
				}

				/* The reduction samples the previous mip while the image is in GENERAL. */
				writeSampledSource(*descriptorSet, *m_hiZMipViews[mip - 1], VK_IMAGE_LAYOUT_GENERAL);
				writeStorageDest(*descriptorSet, *m_hiZMipViews[mip]);

				m_hiZReduceSets.emplace_back(std::move(descriptorSet));
			}

			/* The trace reads the full pyramid chain (binding 1) after the final
			 * GENERAL -> SHADER_READ_ONLY transition, same for every frame. */
			for ( const auto & descriptorSet : m_tracePerFrame )
			{
				VkDescriptorImageInfo pyramidInfo{};
				pyramidInfo.sampler = m_hiZSampler->handle();
				pyramidInfo.imageView = m_hiZFullView->handle();
				pyramidInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				VkWriteDescriptorSet write{};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.dstSet = descriptorSet->handle();
				write.dstBinding = 1;
				write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				write.descriptorCount = 1;
				write.pImageInfo = &pyramidInfo;

				vkUpdateDescriptorSets(localDevice->handle(), 1, &write, 0, nullptr);
			}

			/* ---- Pre-convolved color pyramid (cone-traced glossy, Uludag GPU Pro 5) ---- */
			{
				m_colorPyramidImage = std::make_shared< Image >(
					localDevice,
					VK_IMAGE_TYPE_2D,
					VK_FORMAT_R16G16B16A16_SFLOAT,
					VkExtent3D{
						.width = pyramidBaseW,
						.height = pyramidBaseH,
						.depth = 1U
					},
					VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
					0,
					m_colorPyramidMipCount
				);
				m_colorPyramidImage->setIdentifier(ClassId, "ColorPyramid", "Image");

				if ( !m_colorPyramidImage->createOnHardware() )
				{
					TraceError{ClassId} << "Failed to create the color pyramid image !";

					return false;
				}

				m_colorPyramidMipViews.reserve(m_colorPyramidMipCount);

				for ( uint32_t mip = 0; mip < m_colorPyramidMipCount; mip++ )
				{
					auto view = std::make_shared< ImageView >(
						m_colorPyramidImage,
						VK_IMAGE_VIEW_TYPE_2D,
						VkImageSubresourceRange{
							.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
							.baseMipLevel = mip,
							.levelCount = 1U,
							.baseArrayLayer = 0U,
							.layerCount = 1U
						}
					);
					view->setIdentifier(ClassId, "ColorPyramidMip" + std::to_string(mip), "ImageView");

					if ( !view->createOnHardware() )
					{
						return false;
					}

					m_colorPyramidMipViews.emplace_back(view);
				}

				m_colorPyramidFullView = std::make_shared< ImageView >(
					m_colorPyramidImage,
					VK_IMAGE_VIEW_TYPE_2D,
					VkImageSubresourceRange{
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.baseMipLevel = 0U,
						.levelCount = m_colorPyramidMipCount,
						.baseArrayLayer = 0U,
						.layerCount = 1U
					}
				);
				m_colorPyramidFullView->setIdentifier(ClassId, "ColorPyramidFull", "ImageView");

				if ( !m_colorPyramidFullView->createOnHardware() )
				{
					return false;
				}

				/* TRILINEAR/clamp: the downsample taps bilinear corners, the cone lookup
				 * interpolates between mips. */
				m_colorPyramidSampler = renderer.getSampler("SSRColorPyramid", [] (Settings &, VkSamplerCreateInfo & createInfo) {
					createInfo.magFilter = VK_FILTER_LINEAR;
					createInfo.minFilter = VK_FILTER_LINEAR;
					createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
					createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
					createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
					createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
					createInfo.anisotropyEnable = VK_FALSE;
					createInfo.maxLod = VK_LOD_CLAMP_NONE;
				});

				if ( m_colorPyramidSampler == nullptr )
				{
					return false;
				}

				const auto downsampleModule = shaderManager.getShaderModuleFromSourceCode(localDevice, "SSR_ColorDownsample_CS", ShaderType::ComputeShader, SSRColorDownsampleComputeShader);

				if ( downsampleModule == nullptr )
				{
					TraceError{ClassId} << "Failed to compile the color pyramid downsample shader !";

					return false;
				}

				/* Same push constants and DS shape as the Hi-Z build: the layouts are shared. */
				m_colorDownsamplePipeline = std::make_unique< ComputePipeline >(m_hiZPipelineLayout);
				m_colorDownsamplePipeline->setShaderModule(downsampleModule->handle());

				if ( !m_colorDownsamplePipeline->createOnHardware() )
				{
					return false;
				}

				const auto writePyramidSource = [&localDevice, this] (const DescriptorSet & descriptorSet, const ImageView & sourceView, VkImageLayout layout) {
					VkDescriptorImageInfo sourceInfo{};
					sourceInfo.sampler = m_colorPyramidSampler->handle();
					sourceInfo.imageView = sourceView.handle();
					sourceInfo.imageLayout = layout;

					VkWriteDescriptorSet write{};
					write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					write.dstSet = descriptorSet.handle();
					write.dstBinding = 0;
					write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					write.descriptorCount = 1;
					write.pImageInfo = &sourceInfo;

					vkUpdateDescriptorSets(localDevice->handle(), 1, &write, 0, nullptr);
				};

				m_colorCopyPerFrame.reserve(framesInFlight);

				for ( uint32_t frame = 0; frame < framesInFlight; frame++ )
				{
					auto descriptorSet = std::make_unique< DescriptorSet >(m_hiZDescriptorPool, m_hiZDSLayout);
					descriptorSet->setIdentifier(ClassId, "ColorCopy_DescSet" + std::to_string(frame), "DescriptorSet");

					if ( !descriptorSet->create() )
					{
						return false;
					}

					writeStorageDest(*descriptorSet, *m_colorPyramidMipViews[0]);

					m_colorCopyPerFrame.emplace_back(std::move(descriptorSet));
				}

				m_colorReduceSets.reserve(m_colorPyramidMipCount);

				for ( uint32_t mip = 1; mip < m_colorPyramidMipCount; mip++ )
				{
					auto descriptorSet = std::make_unique< DescriptorSet >(m_hiZDescriptorPool, m_hiZDSLayout);
					descriptorSet->setIdentifier(ClassId, "ColorReduce_DescSet" + std::to_string(mip), "DescriptorSet");

					if ( !descriptorSet->create() )
					{
						return false;
					}

					/* The downsample samples the previous mip while the image is in GENERAL. */
					writePyramidSource(*descriptorSet, *m_colorPyramidMipViews[mip - 1], VK_IMAGE_LAYOUT_GENERAL);
					writeStorageDest(*descriptorSet, *m_colorPyramidMipViews[mip]);

					m_colorReduceSets.emplace_back(std::move(descriptorSet));
				}

				/* The resolve reads the full chain (binding 4), same view every frame. */
				for ( const auto & descriptorSet : m_resolvePerFrame )
				{
					VkDescriptorImageInfo pyramidInfo{};
					pyramidInfo.sampler = m_colorPyramidSampler->handle();
					pyramidInfo.imageView = m_colorPyramidFullView->handle();
					pyramidInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

					VkWriteDescriptorSet write{};
					write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					write.dstSet = descriptorSet->handle();
					write.dstBinding = 4;
					write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					write.descriptorCount = 1;
					write.pImageInfo = &pyramidInfo;

					vkUpdateDescriptorSets(localDevice->handle(), 1, &write, 0, nullptr);
				}
			}
		}

		return true;
	}

	void
	SSR::destroy () noexcept
	{
		m_compositePerFrame.clear();
		m_resolvePerFrame.clear();
		m_tracePerFrame.clear();
		m_blurVPerFrame.clear();
		m_blurHPerFrame.clear();
		m_hiZCopyPerFrame.clear();
		m_hiZReduceSets.clear();
		m_colorCopyPerFrame.clear();
		m_colorReduceSets.clear();
		m_colorDownsamplePipeline.reset();
		m_colorPyramidSampler.reset();
		m_colorPyramidFullView.reset();
		m_colorPyramidMipViews.clear();
		m_colorPyramidImage.reset();
		m_hiZDescriptorPool.reset();
		m_hiZCopyPipeline.reset();
		m_hiZReducePipeline.reset();
		m_hiZPipelineLayout.reset();
		m_hiZDSLayout.reset();
		m_hiZSampler.reset();
		m_hiZFullView.reset();
		m_hiZMipViews.clear();
		m_hiZImage.reset();
		
		m_compositePipeline.reset();
		m_blurPipeline.reset();
		m_resolvePipeline.reset();
		m_tracePipeline.reset();
		m_compositeLayout.reset();
		m_blurLayout.reset();
		m_resolveLayout.reset();
		m_traceLayout.reset();

		m_outputTarget.destroy();
		m_blurVTarget.destroy();
		m_blurHTarget.destroy();
		m_resolveTarget.destroy();
		m_traceTarget.destroy();
	}

	const TextureInterface &
	SSR::execute (const CommandBuffer & commandBuffer, const TextureInterface & inputColor, const FrameContext & context) noexcept
	{
		const auto * inputDepth = context.depth;
		const auto * inputNormals = context.normals;
		const auto * inputMaterialProperties = context.materialProperties;
		const auto & constants = context.constants;


		const auto frameIndex = this->renderer().currentFrameIndex();

		/* Update the normals descriptor for this frame's trace pass (binding 1, the Hi-Z
		 * pyramid, is fixed — written at creation). */
		if ( inputNormals != nullptr )
		{
			static_cast< void >(m_tracePerFrame[frameIndex]->writeCombinedImageSampler(0, *inputNormals));
		}

		/* Update color descriptor for composite pass (this frame's copy). */
		static_cast< void >(m_compositePerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));

		/* Update material properties descriptor for composite pass. */
		if ( inputMaterialProperties != nullptr )
		{
			static_cast< void >(m_compositePerFrame[frameIndex]->writeCombinedImageSampler(2, *inputMaterialProperties));
		}

		/* ---- Pass 0: Hi-Z pyramid build (compute) ---- */
		if ( inputDepth != nullptr )
		{
			/* This frame's copy set: scene depth -> mip 0. */
			static_cast< void >(m_hiZCopyPerFrame[frameIndex]->writeCombinedImageSampler(0, *inputDepth));

			/* Whole pyramid: UNDEFINED -> GENERAL (previous content discarded, fully rewritten). */
			{
				const Sync::ImageMemoryBarrier barrier{
					*m_hiZImage,
					0,
					VK_ACCESS_SHADER_WRITE_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_GENERAL
				};

				commandBuffer.pipelineBarrier(barrier, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
			}

			const auto mip0Width = static_cast< int32_t >(m_traceTarget.width());
			const auto mip0Height = static_cast< int32_t >(m_traceTarget.height());

			commandBuffer.bind(*m_hiZCopyPipeline);
			commandBuffer.bind(*m_hiZCopyPerFrame[frameIndex], *m_hiZPipelineLayout, VK_PIPELINE_BIND_POINT_COMPUTE, 0);

			{
				const HiZPushConstants pc{
					.destWidth = mip0Width,
					.destHeight = mip0Height,
					.sourceMaxX = static_cast< int32_t >(inputDepth->image()->createInfo().extent.width) - 1,
					.sourceMaxY = static_cast< int32_t >(inputDepth->image()->createInfo().extent.height) - 1
				};

				vkCmdPushConstants(commandBuffer.handle(), m_hiZPipelineLayout->handle(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(HiZPushConstants), &pc);
				commandBuffer.dispatch((mip0Width + 7) / 8, (mip0Height + 7) / 8, 1);
			}

			commandBuffer.bind(*m_hiZReducePipeline);

			for ( uint32_t mip = 1; mip < m_hiZMipCount; mip++ )
			{
				/* Previous mip written -> readable by this reduction. */
				const Sync::ImageMemoryBarrier barrier{
					*m_hiZImage,
					VK_ACCESS_SHADER_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT,
					VK_IMAGE_LAYOUT_GENERAL,
					VK_IMAGE_LAYOUT_GENERAL
				};

				commandBuffer.pipelineBarrier(barrier, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

				const auto destWidth = std::max(1, mip0Width >> mip);
				const auto destHeight = std::max(1, mip0Height >> mip);
				const auto sourceWidth = std::max(1, mip0Width >> (mip - 1));
				const auto sourceHeight = std::max(1, mip0Height >> (mip - 1));

				const HiZPushConstants pc{
					.destWidth = destWidth,
					.destHeight = destHeight,
					.sourceMaxX = sourceWidth - 1,
					.sourceMaxY = sourceHeight - 1
				};

				commandBuffer.bind(*m_hiZReduceSets[mip - 1], *m_hiZPipelineLayout, VK_PIPELINE_BIND_POINT_COMPUTE, 0);
				vkCmdPushConstants(commandBuffer.handle(), m_hiZPipelineLayout->handle(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(HiZPushConstants), &pc);
				commandBuffer.dispatch((destWidth + 7) / 8, (destHeight + 7) / 8, 1);
			}

			/* Pyramid complete: GENERAL -> SHADER_READ_ONLY for the trace fragment shader
			 * (next frame's first barrier discards from UNDEFINED, so the cycle is closed). */
			{
				const Sync::ImageMemoryBarrier barrier{
					*m_hiZImage,
					VK_ACCESS_SHADER_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT,
					VK_IMAGE_LAYOUT_GENERAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				};

				commandBuffer.pipelineBarrier(barrier, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
			}
		}

		/* ---- Pass 0b: pre-convolved color pyramid build (cone-traced glossy source) ---- */
		if ( m_colorPyramidImage != nullptr )
		{
			/* This frame's copy set: input color -> pyramid mip 0 (tent downsample). */
			static_cast< void >(m_colorCopyPerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));

			/* Whole pyramid: UNDEFINED -> GENERAL (previous content discarded, fully rewritten). */
			{
				const Sync::ImageMemoryBarrier barrier{
					*m_colorPyramidImage,
					0,
					VK_ACCESS_SHADER_WRITE_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_GENERAL
				};

				commandBuffer.pipelineBarrier(barrier, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
			}

			const auto & pyramidExtent = m_colorPyramidImage->createInfo().extent;
			const auto baseWidth = static_cast< int32_t >(pyramidExtent.width);
			const auto baseHeight = static_cast< int32_t >(pyramidExtent.height);

			commandBuffer.bind(*m_colorDownsamplePipeline);
			commandBuffer.bind(*m_colorCopyPerFrame[frameIndex], *m_hiZPipelineLayout, VK_PIPELINE_BIND_POINT_COMPUTE, 0);

			{
				const HiZPushConstants pc{
					.destWidth = baseWidth,
					.destHeight = baseHeight,
					.sourceMaxX = static_cast< int32_t >(inputColor.image()->createInfo().extent.width) - 1,
					.sourceMaxY = static_cast< int32_t >(inputColor.image()->createInfo().extent.height) - 1
				};

				vkCmdPushConstants(commandBuffer.handle(), m_hiZPipelineLayout->handle(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(HiZPushConstants), &pc);
				commandBuffer.dispatch((baseWidth + 7) / 8, (baseHeight + 7) / 8, 1);
			}

			for ( uint32_t mip = 1; mip < m_colorPyramidMipCount; mip++ )
			{
				/* Previous mip written -> readable by this downsample. */
				const Sync::ImageMemoryBarrier barrier{
					*m_colorPyramidImage,
					VK_ACCESS_SHADER_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT,
					VK_IMAGE_LAYOUT_GENERAL,
					VK_IMAGE_LAYOUT_GENERAL
				};

				commandBuffer.pipelineBarrier(barrier, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

				const auto destWidth = std::max(1, baseWidth >> mip);
				const auto destHeight = std::max(1, baseHeight >> mip);
				const auto sourceWidth = std::max(1, baseWidth >> (mip - 1));
				const auto sourceHeight = std::max(1, baseHeight >> (mip - 1));

				const HiZPushConstants pc{
					.destWidth = destWidth,
					.destHeight = destHeight,
					.sourceMaxX = sourceWidth - 1,
					.sourceMaxY = sourceHeight - 1
				};

				commandBuffer.bind(*m_colorReduceSets[mip - 1], *m_hiZPipelineLayout, VK_PIPELINE_BIND_POINT_COMPUTE, 0);
				vkCmdPushConstants(commandBuffer.handle(), m_hiZPipelineLayout->handle(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(HiZPushConstants), &pc);
				commandBuffer.dispatch((destWidth + 7) / 8, (destHeight + 7) / 8, 1);
			}

			/* Pyramid complete: GENERAL -> SHADER_READ_ONLY for the resolve fragment shader. */
			{
				const Sync::ImageMemoryBarrier barrier{
					*m_colorPyramidImage,
					VK_ACCESS_SHADER_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT,
					VK_IMAGE_LAYOUT_GENERAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				};

				commandBuffer.pipelineBarrier(barrier, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
			}
		}

		/* ---- Pass 1: Trace (Hi-Z hierarchical traversal) ---- */
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
				.fadeScreenEdge = m_parameters.fadeScreenEdge,
				.maxSteps = m_parameters.maxSteps,
				.hiZMaxLevel = m_hiZMipCount - 1U,
				.padding = 0U
			};

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_traceTarget,
				*m_tracePipeline,
				*m_traceLayout,
				*m_tracePerFrame[frameIndex],
				&pc,
				sizeof(TracePushConstants)
			);
		}

		/* ---- Pass 2: Resolve (sample reflected color at hitUV, cubemap fallback on miss) ---- */
		{
			/* Update per-frame descriptors: color (binding 0), depth (binding 2), normals (binding 3). */
			static_cast< void >(m_resolvePerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));

			if ( inputDepth != nullptr )
			{
				static_cast< void >(m_resolvePerFrame[frameIndex]->writeCombinedImageSampler(2, *inputDepth));
			}

			if ( inputNormals != nullptr )
			{
				static_cast< void >(m_resolvePerFrame[frameIndex]->writeCombinedImageSampler(3, *inputNormals));
			}

			/* Compute inverse view matrix for cubemap fallback.
			 * Use readStateIndex to match the view matrix that produced the depth buffer. */
			const auto readStateIndex = this->renderer().currentReadStateIndex();
			const auto & viewMat = this->renderer().mainRenderTarget()->viewMatrices().viewMatrix(readStateIndex, false, 0);
			const auto invView = viewMat.inverse();
			const auto * inv = invView.data();

			const ResolvePushConstants resolvePC{
				.invViewCol0 = {inv[0], inv[1], inv[2], 0.0F},
				.invViewCol1 = {inv[4], inv[5], inv[6], 0.0F},
				.invViewCol2 = {inv[8], inv[9], inv[10], 0.0F},
				.texelSizeX = 1.0F / static_cast< float >(m_resolveTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_resolveTarget.height()),
				.nearPlane = constants.nearPlane,
				.farPlane = constants.farPlane,
				.tanHalfFovY = constants.tanHalfFovY,
				.aspectRatio = constants.frameWidth / constants.frameHeight,
				.envFallbackIntensity = m_parameters.envFallbackIntensity,
				.intensity = m_parameters.intensity,
				/* Cone width is measured in TRACE texels; the pyramid base is half the
				 * effect resolution — the offset converts one into the other. */
				.pyramidLodOffset = -std::log2(static_cast< float >(m_traceTarget.width()) / static_cast< float >(std::max(1U, m_colorPyramidImage != nullptr ? m_colorPyramidImage->createInfo().extent.width : m_traceTarget.width()))),
				.pyramidMaxLod = static_cast< float >(m_colorPyramidMipCount > 0U ? m_colorPyramidMipCount - 1U : 0U)
			};

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_resolveTarget,
				*m_resolvePipeline,
				*m_resolveLayout,
				*m_resolvePerFrame[frameIndex],
				&resolvePC,
				sizeof(ResolvePushConstants),
				this->renderer().bindlessTextureManager().descriptorSet()
			);
		}

		/* ---- Pass 3: Bilateral Blur Horizontal (roughness-scaled radius) ---- */
		{
			if ( inputDepth != nullptr )
			{
				static_cast< void >(m_blurHPerFrame[frameIndex]->writeCombinedImageSampler(1, *inputDepth));
			}

			if ( inputNormals != nullptr )
			{
				static_cast< void >(m_blurHPerFrame[frameIndex]->writeCombinedImageSampler(2, *inputNormals));
			}

			const BlurPushConstants blurH{
				.texelSizeX = 1.0F / static_cast< float >(m_blurHTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_blurHTarget.height()),
				.directionX = 1.0F,
				.directionY = 0.0F,
				.depthSigma = m_depthSigma,
				.normalSigma = m_normalSigma,
				.blurRadius = static_cast< int32_t >(m_blurRadius),
				.padding = 0.0F
			};

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_blurHTarget,
				*m_blurPipeline,
				*m_blurLayout,
				*m_blurHPerFrame[frameIndex],
				&blurH,
				sizeof(BlurPushConstants)
			);
		}

		/* ---- Pass 4: Bilateral Blur Vertical ---- */
		{
			if ( inputDepth != nullptr )
			{
				static_cast< void >(m_blurVPerFrame[frameIndex]->writeCombinedImageSampler(1, *inputDepth));
			}

			if ( inputNormals != nullptr )
			{
				static_cast< void >(m_blurVPerFrame[frameIndex]->writeCombinedImageSampler(2, *inputNormals));
			}

			const BlurPushConstants blurV{
				.texelSizeX = 1.0F / static_cast< float >(m_blurVTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_blurVTarget.height()),
				.directionX = 0.0F,
				.directionY = 1.0F,
				.depthSigma = m_depthSigma,
				.normalSigma = m_normalSigma,
				.blurRadius = static_cast< int32_t >(m_blurRadius),
				.padding = 0.0F
			};

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_blurVTarget,
				*m_blurPipeline,
				*m_blurLayout,
				*m_blurVPerFrame[frameIndex],
				&blurV,
				sizeof(BlurPushConstants)
			);
		}

		/* ---- Pass 5: Composite ---- */
		{
			const CompositePushConstants comp{
				.intensity = m_parameters.intensity,
				.padding1 = 0.0F,
				.padding2 = 0.0F,
				.padding3 = 0.0F
			};

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_outputTarget,
				*m_compositePipeline,
				*m_compositeLayout,
				*m_compositePerFrame[frameIndex],
				&comp,
				sizeof(CompositePushConstants)
			);
		}

		return m_outputTarget;
	}
}
