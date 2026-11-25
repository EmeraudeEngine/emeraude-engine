/*
 * src/Graphics/Effects/Shared/TracedLightGLSL.hpp
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
 * @brief The traced light list as the traced lanes read it (RTR, RTGI, the irradiance probes): its stride, and the
 * colour projection of a light.
 *
 * One light of `lightSSBO.lights[]` is `EmTracedLightStride` vec4, written by Scenes::LightSet::updateVideoMemory()
 * from Scenes::LightSet::GPULightData (the two MUST stay in lockstep: the structure's size is asserted there):
 *   [0] colour (unit luminance), intensity
 *   [1] position, radius
 *   [2] direction, type (0 directional, 1 point, 2 spot, 3 line segment)
 *   [3] inner cosine, outer cosine, casts a shadow (0 / 1), projection kind (0 none, 1 a 2D picture through the light's
 *       matrix, 2 a cubemap by direction)
 *   [4] projection texture (a bindless index, uint bits), projection boost, through the sea (0 / 1), unused
 *   [5-8] the light's matrix, world → projection coordinates (the raster light pass's, column by column)
 *   [9] what the sea's water takes from the light per metre of its path, per channel (1/m); the sea level (m)
 *
 * `emTracedLightProjection(base, litPosition, lightToPoint)` is what the light's colour projection (a gobo: a
 * flashlight's pattern, a headlight's dipped beam) lets through toward a lit point: the raster light pass's
 * `projectionColor` (Saphir::LightGenerator), term for term — the picture sampled at the point's projection
 * coordinates, or the cubemap along the light → point direction, then the boost (1 + picture × boost when above 0).
 * Without it the traced lanes shaded their hit points with the light's full cone: a dipped beam whose pattern lets 2.4 %
 * of the cone through lit the reflections 42 times too much (measured 2026-11-22: a car's bonnet read 51 / 255 where
 * the street's reflection alone reads 10). The base mip level is read: a ray has no screen derivative.
 * Not covered: a directional light's projection and a point light's ANIMATED cubemap (their entries say "none").
 *
 * `emTracedLightThroughSea(base, litPosition)` is what the scene's sea lets of a directional light reach a point under
 * its level: the raster light pass's `seaLight` (Saphir::LightGenerator) without its caustics — the transmission
 * through the surface (Schlick, F0 = 0.02), then the absorption along the refracted path (n = 1.33), per channel.
 * A traced hit takes the caustics' mean, 1. Without it a deep seabed lit the reflections and the bounced light as if
 * it stood in the air (engine item underwater-caustics-and-depth-light).
 *
 * Needs, declared by the including shader BEFORE this chunk: `lightSSBO` (vec4 lights[]), `textures2D[]` and
 * `texturesCube[]` (the bindless tables), GL_EXT_nonuniform_qualifier.
 *
 * Usage in an effect's constexpr GLSL literal: `)GLSL" EMEN_TRACED_LIGHT_GLSL R"GLSL(` then
 * `uint base = i * EmTracedLightStride;`, `emTracedLightProjection(base, hitPos, -L)` and
 * `emTracedLightThroughSea(base, hitPos)`.
 */
#define EMEN_TRACED_LIGHT_GLSL R"GLSL(
/* One light of the traced light list (Scenes::LightSet::GPULightData), in vec4. */
const uint EmTracedLightStride = 10u;

/* What a light's colour projection lets through toward a lit point (1 for a light without one): the raster light
 * pass's projectionColor. `lightToPoint` is the direction from the light to the point (any length). */
vec3 emTracedLightProjection (uint base, vec3 litPosition, vec3 lightToPoint)
{
	float kind = lightSSBO.lights[base + 3u].w;

	if (kind < 0.5)
	{
		return vec3(1.0);
	}

	vec4 projection = lightSSBO.lights[base + 4u];
	uint textureIndex = floatBitsToUint(projection.x);
	vec3 picture;

	if (kind < 1.5)
	{
		mat4 lightMatrix = mat4(lightSSBO.lights[base + 5u], lightSSBO.lights[base + 6u], lightSSBO.lights[base + 7u], lightSSBO.lights[base + 8u]);
		vec4 coordinates = lightMatrix * vec4(litPosition, 1.0);

		picture = textureLod(textures2D[nonuniformEXT(textureIndex)], coordinates.xy / coordinates.w, 0.0).rgb;
	}
	else
	{
		picture = textureLod(texturesCube[nonuniformEXT(textureIndex)], lightToPoint, 0.0).rgb;
	}

	return projection.y > 0.0 ? vec3(1.0) + picture * projection.y : picture;
}

/* What the scene's sea lets of a light reach a lit point (1 above the sea level, and for a light that does not go
 * through a sea): the raster light pass's seaLight, without the caustics. */
vec3 emTracedLightThroughSea (uint base, vec3 litPosition)
{
	if (lightSSBO.lights[base + 4u].z < 0.5)
	{
		return vec3(1.0);
	}

	vec4 water = lightSSBO.lights[base + 9u];
	float depth = water.w - litPosition.y;

	if (depth <= 0.0)
	{
		return vec3(1.0);
	}

	float cosIncident = clamp(-normalize(lightSSBO.lights[base + 2u].xyz).y, 0.0, 1.0);
	float sinRefracted = sqrt(max(1.0 - cosIncident * cosIncident, 0.0)) / 1.33;
	float cosRefracted = sqrt(max(1.0 - sinRefracted * sinRefracted, 0.0));

	return (1.0 - (0.02 + 0.98 * pow(1.0 - cosIncident, 5.0))) * exp(-water.xyz * (depth / cosRefracted));
}
)GLSL"
