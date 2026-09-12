/*
 * src/Graphics/Effects/Shared/IrradianceProbesGLSL.hpp
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

#pragma once

/**
 * @file
 * @brief The GLSL side of the irradiance probe volume — the engine's radiance cache — as ONE macro
 * every consumer splices into its own shader literal.
 *
 * The volume is a DDGI irradiance field (Majercik, Guertin, Nowrouzezahrai, McGuire, *Dynamic
 * Diffuse Global Illumination with Ray-Traced Irradiance Fields*, JCGT 8(2), 2019; production
 * extensions in Majercik, Marrs, Spjut, McGuire, JCGT 10(2), 2021): a camera-centred grid of probes,
 * each storing an octahedral map of cosine-weighted radiance (E/π — the SAME demodulated convention
 * as the RTGI history) plus an octahedral map of mean distance and squared distance toward the
 * geometry around it, both re-traced and blended every frame. It answers ONE question at any world
 * position: "what diffuse indirect light arrives here ?" — on screen or not, which is exactly what a
 * reflection hit behind the camera needs and what no screen-space history can give.
 *
 * The macro declares the parameters UBO (binding 3) and the two atlas samplers (bindings 4 and 5) at
 * the descriptor set index the consumer binds the volume at, the octahedral mapping, the probe
 * addressing shared with the volume's own update passes (storage slot ↔ world grid index, atlas
 * tile origin, ray directions), and the query `probeIrradiance(worldPos, normal, viewDir)`:
 * trilinear over the 8 surrounding probes, weighted by a back-face ("wrap shading") term and a
 * Chebyshev visibility test on the stored distance moments (the leak stopper), faded to zero at the
 * volume's boundary. It returns E/π as ENERGY: multiply by the DIFFUSE albedo to get outgoing
 * radiance, and by the IndirectDiffuse intensity (`probeVolume.ambientColor.w`) only where a final
 * image is composed (RTR does; the probes' own recursion and the RTGI feedback do not).
 *
 * ⚠️ The mapping functions are the single source of truth for BOTH the update passes and the
 * consumers — a tile origin or an octahedral wrap computed twice would drift. Bindings 0-2 of the
 * set (ray buffer, the two storage images) are the update passes' own and are not declared here.
 * ⚠️ GLSL `%` is undefined on a negative operand: every modulo below is fed non-negative values
 * (the scroll offsets are kept in [0, count) by the C++ side).
 */

/**
 * @brief GLSL: parameters UBO, atlas samplers, probe addressing and the irradiance query.
 * @note `setIndex` is the descriptor set index the consumer binds `IrradianceProbeVolume::descriptorSet()` at.
 */
#define EMEN_IRRADIANCE_PROBES_GLSL(setIndex) R"GLSL(
/* ---- Irradiance probe volume (DDGI): the engine's radiance cache. ---- */
layout(set = )GLSL" #setIndex R"GLSL(, binding = 3, std140) uniform IrradianceProbeParams
{
	vec4 originSpacing;   /* xyz: world position of grid probe (0,0,0), the volume's min corner; w: probe spacing (m). */
	ivec4 probeCounts;    /* xyz: probes per axis; w: rays per probe. */
	ivec4 scrollReset;    /* xyz: storage scroll offsets in [0, count); w: 1 = every probe is reset this frame. */
	ivec4 resetPlanes;    /* xyz: storage plane index reset this frame per axis (-1 = none); w: frame counter. */
	vec4 biasHysteresis;  /* x: normal bias (m), y: view bias (m), z: hysteresis, w: max distance stored in the distance atlas (m). */
	vec4 skyAmbient;      /* x: sky luminance (nits), y: 1 = enabled, z: light count, w: bounce feedback weight. */
	vec4 ambientColor;    /* rgb: scene ambient colour x effective illuminance (the raster's ambient term); w: the IndirectDiffuse intensity the primary surfaces receive — for the consumers that compose an image (RTR), NOT applied by the query. */
	vec4 rotation0;       /* COLUMNS of the per-frame random rotation of the ray set. */
	vec4 rotation1;
	vec4 rotation2;
} probeVolume;

layout(set = )GLSL" #setIndex R"GLSL(, binding = 4) uniform sampler2DArray probeIrradianceAtlas;
layout(set = )GLSL" #setIndex R"GLSL(, binding = 5) uniform sampler2DArray probeDistanceAtlas;

const int ProbeIrradianceTexels = 8;
const int ProbeDistanceTexels = 16;

vec2 probeSignNotZero (vec2 v)
{
	return vec2(v.x >= 0.0 ? 1.0 : -1.0, v.y >= 0.0 ? 1.0 : -1.0);
}

/* Octahedral mapping (Cigolle et al., "A Survey of Efficient Representations for Independent Unit
 * Vectors", JCGT 3(2), 2014): unit direction <-> [-1,1]^2. */
vec2 probeOctEncode (vec3 direction)
{
	float l1 = abs(direction.x) + abs(direction.y) + abs(direction.z);
	vec2 result = direction.xy * (1.0 / max(l1, 1e-6));

	if (direction.z < 0.0)
	{
		result = (1.0 - abs(result.yx)) * probeSignNotZero(result);
	}

	return result;
}

vec3 probeOctDecode (vec2 octant)
{
	vec3 direction = vec3(octant.x, octant.y, 1.0 - abs(octant.x) - abs(octant.y));

	if (direction.z < 0.0)
	{
		direction.xy = (1.0 - abs(direction.yx)) * probeSignNotZero(direction.xy);
	}

	return normalize(direction);
}

mat3 probeRayRotation ()
{
	return mat3(probeVolume.rotation0.xyz, probeVolume.rotation1.xyz, probeVolume.rotation2.xyz);
}

/* Spherical Fibonacci point set: `count` directions spread evenly over the sphere. */
vec3 probeSphericalFibonacci (uint index, uint count)
{
	const float GoldenRatioConjugate = 0.6180339887498949;
	float phi = 6.28318530718 * fract(float(index) * GoldenRatioConjugate);
	float cosTheta = 1.0 - (2.0 * float(index) + 1.0) / float(count);
	float sinTheta = sqrt(clamp(1.0 - cosTheta * cosTheta, 0.0, 1.0));

	return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

/* The direction of ray `rayIndex` of EVERY probe this frame (one rotation shared by all probes). */
vec3 probeRayDirection (uint rayIndex)
{
	return probeRayRotation() * probeSphericalFibonacci(rayIndex, uint(probeVolume.probeCounts.w));
}

/* The volume SCROLLS with the camera: a probe keeps its storage slot while its world grid index
 * changes. slot = (grid + scroll) mod count, both operands in [0, count). */
ivec3 probeStorageSlot (ivec3 gridIndex)
{
	return (gridIndex + probeVolume.scrollReset.xyz) % probeVolume.probeCounts.xyz;
}

ivec3 probeGridIndex (ivec3 slot)
{
	ivec3 counts = probeVolume.probeCounts.xyz;

	return (slot - probeVolume.scrollReset.xyz + counts) % counts;
}

/* Linear probe index (one thread group per probe in the update passes): x fastest, then z, then y. */
ivec3 probeSlotFromLinear (uint linear)
{
	ivec3 counts = probeVolume.probeCounts.xyz;
	int index = int(linear);

	return ivec3(index % counts.x, index / (counts.x * counts.z), (index / counts.x) % counts.z);
}

vec3 probeWorldPosition (ivec3 gridIndex)
{
	return probeVolume.originSpacing.xyz + vec3(gridIndex) * probeVolume.originSpacing.w;
}

/* Atlas texel of a probe's interior (0,0): one 2D array layer per y plane, tiles of `texels` + a
 * 1-texel border laid out x along the atlas width, z along its height. */
ivec2 probeTileOrigin (ivec3 slot, int texels)
{
	return ivec2(slot.x, slot.z) * (texels + 2) + 1;
}

vec3 probeAtlasUV (ivec3 slot, vec3 direction, int texels)
{
	vec2 octant = probeOctEncode(direction) * 0.5 + 0.5;
	vec2 texel = vec2(probeTileOrigin(slot, texels)) + octant * float(texels);
	vec2 atlasSize = vec2(probeVolume.probeCounts.x, probeVolume.probeCounts.z) * float(texels + 2);

	return vec3(texel / atlasSize, float(slot.y));
}

bool probeIsReset (ivec3 slot)
{
	return probeVolume.scrollReset.w != 0
		|| slot.x == probeVolume.resetPlanes.x
		|| slot.y == probeVolume.resetPlanes.y
		|| slot.z == probeVolume.resetPlanes.z;
}

/* Influence of the volume at a point: 1 inside, fading to 0 over the last probe spacing before
 * the boundary. A point outside the volume receives no cached irradiance (there is none to give). */
float probeVolumeWeight (vec3 worldPos)
{
	vec3 extent = vec3(probeVolume.probeCounts.xyz - 1) * probeVolume.originSpacing.w;
	vec3 local = worldPos - probeVolume.originSpacing.xyz;
	vec3 distanceToBoundary = min(local, extent - local);
	vec3 fade = clamp(distanceToBoundary / probeVolume.originSpacing.w, 0.0, 1.0);

	return fade.x * fade.y * fade.z;
}

/* THE query. worldPos/normal describe the receiving surface, viewDir points FROM the surface TOWARD
 * whoever is looking at it (the camera, or the reflection ray's origin). Returns E/pi: multiply by
 * the diffuse albedo to get the outgoing radiance, exactly like the RTGI history. */
vec3 probeIrradiance (vec3 worldPos, vec3 normal, vec3 viewDir)
{
	if (probeVolume.skyAmbient.y < 0.5)
	{
		return vec3(0.0);
	}

	float volumeWeight = probeVolumeWeight(worldPos);

	if (volumeWeight <= 0.0)
	{
		return vec3(0.0);
	}

	ivec3 counts = probeVolume.probeCounts.xyz;
	float spacing = probeVolume.originSpacing.w;

	/* Self-shadow bias: sample a little off the surface, toward the normal and the viewer, so the
	 * Chebyshev test does not see the surface itself as an occluder. */
	vec3 biasedPos = worldPos + normal * probeVolume.biasHysteresis.x + viewDir * probeVolume.biasHysteresis.y;
	vec3 gridPos = (biasedPos - probeVolume.originSpacing.xyz) / spacing;
	ivec3 baseIndex = clamp(ivec3(floor(gridPos)), ivec3(0), counts - 2);
	vec3 alpha = clamp(gridPos - vec3(baseIndex), 0.0, 1.0);

	vec3 sum = vec3(0.0);
	float weightSum = 0.0;

	for (int corner = 0; corner < 8; ++corner)
	{
		ivec3 offset = ivec3(corner, corner >> 1, corner >> 2) & 1;
		ivec3 gridIndex = baseIndex + offset;
		ivec3 slot = probeStorageSlot(gridIndex);
		vec3 probePos = probeWorldPosition(gridIndex);

		vec3 trilinear = mix(1.0 - alpha, alpha, vec3(offset));
		float weight = trilinear.x * trilinear.y * trilinear.z;

		/* Back-face weight: a probe behind the surface it lights contributes little. */
		vec3 toProbe = normalize(probePos - worldPos);
		float wrap = (dot(toProbe, normal) + 1.0) * 0.5;
		weight *= wrap * wrap + 0.2;

		/* Chebyshev visibility from the probe's distance moments toward the biased point. */
		vec3 probeToPoint = biasedPos - probePos;
		float pointDistance = length(probeToPoint);
		/* Explicit LOD 0: the atlases have no mip chain and the query also runs in compute passes. */
		vec2 moments = textureLod(probeDistanceAtlas, probeAtlasUV(slot, probeToPoint / max(pointDistance, 1e-4), ProbeDistanceTexels), 0.0).rg;
		float variance = abs(moments.x * moments.x - moments.y);
		float visibility = 1.0;

		if (pointDistance > moments.x)
		{
			float excess = pointDistance - moments.x;
			visibility = variance / (variance + excess * excess);
			visibility = max(visibility * visibility * visibility, 0.0);
		}

		weight *= max(0.05, visibility);
		weight = max(weight, 1e-6);

		/* Crush tiny weights so a probe does not pop in and out at the visibility threshold. */
		const float CrushThreshold = 0.2;

		if (weight < CrushThreshold)
		{
			weight *= weight * weight * (1.0 / (CrushThreshold * CrushThreshold));
		}

		vec3 irradiance = textureLod(probeIrradianceAtlas, probeAtlasUV(slot, normal, ProbeIrradianceTexels), 0.0).rgb;

		sum += irradiance * weight;
		weightSum += weight;
	}

	/* ENERGY, unscaled: the update passes and the RTGI feedback consume it as such. A consumer
	 * composing a final image applies the IndirectDiffuse intensity itself
	 * (probeVolume.ambientColor.w), the way RTGI applies its own to its result. */
	return weightSum > 0.0 ? (sum / weightSum) * volumeWeight : vec3(0.0);
}
)GLSL"
