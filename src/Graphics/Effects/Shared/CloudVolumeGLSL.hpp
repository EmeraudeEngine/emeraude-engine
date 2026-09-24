/*
 * src/Graphics/Effects/Shared/CloudVolumeGLSL.hpp
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
 * @brief The ONE description of a volumetric cloud a shader reads, and its box geometry.
 *
 * Two passes read the scene's clouds — the view march (Effects::Atmosphere::VolumetricClouds) and the
 * sun's Beer shadow map (Graphics::CloudShadowMap) — and they must agree to the bit on what a cloud IS:
 * the std140 layout of @ref EmEn::Graphics::CloudBlock and its GLSL twin `EmCloud` live in this file,
 * together with the three box helpers, so neither pass can drift from the other. Same technique as
 * `EMEN_CSM_SAMPLING_GLSL`: a macro holding a string literal, spliced into each effect's `constexpr`
 * GLSL at compile time.
 */

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <array>
#include <cstdint>

namespace EmEn::Graphics
{
	/** @brief The most clouds a frame draws; the others are skipped, with one warning. */
	constexpr uint32_t MaxCloudVolumes{32};

	/**
	 * @brief One cloud as the shaders read it (std140: seven vec4).
	 * @note ⚠️ The GLSL twin is `EmCloud` in EMEN_CLOUD_VOLUME_GLSL, below: one file, one layout.
	 */
	struct EMEN_API CloudBlock
	{
		/** @brief xyz = world centre, w = extinction at full density, in 1/m. */
		std::array< float, 4 > centerAndExtinction{};
		/** @brief xyz = the cloud's local X axis in the world (unit), w = world half extent along it, in m. */
		std::array< float, 4 > axisX{};
		/** @brief Same for the local Y axis. */
		std::array< float, 4 > axisY{};
		/** @brief Same for the local Z axis. */
		std::array< float, 4 > axisZ{};
		/** @brief xyz = normalised shape half extents (X = 1), w = bindless 3D slot. */
		std::array< float, 4 > shape{};
		/** @brief x = erosion, y = detail cells across the width, z = boiling offset (periods), w = metres per normalised shape unit (smallest axis). */
		std::array< float, 4 > look{};
		/** @brief rgb = single-scattering albedo, w = unused. */
		std::array< float, 4 > albedo{};
	};

	static_assert(sizeof(CloudBlock) == 112, "CloudBlock must be seven vec4 to match the GLSL std140 struct !");
}

/**
 * @brief Declares `EmCloud`, `MaxClouds`, `MaxSkipDistance` and the box helpers.
 * @note ⚠️ `MaxSkipDistance` must equal Graphics::CloudShapeResource::MaxSkipDistance, and `MaxClouds`
 * Graphics::MaxCloudVolumes.
 */
#define EMEN_CLOUD_VOLUME_GLSL R"GLSL(
/* ---- The ONE cloud description (Graphics/Effects/Shared/CloudVolumeGLSL.hpp) ---- */

const int MaxClouds = 32;
/* Must match Graphics::CloudShapeResource::MaxSkipDistance. */
const float MaxSkipDistance = 0.5;

struct EmCloud
{
	vec4 centerAndExtinction;	/* xyz = world centre, w = extinction at full density (1/m). */
	vec4 axisX;					/* xyz = local X in the world (unit), w = world half extent (m). */
	vec4 axisY;
	vec4 axisZ;
	vec4 shape;					/* xyz = normalised shape half extents, w = bindless 3D slot. */
	vec4 look;					/* x = erosion, y = detail cells across the width, z = boiling offset, w = metres per shape unit. */
	vec4 albedo;				/* rgb = single-scattering albedo. */
};

/* A world point in the cloud's box, [-1, 1] on every axis inside it. */
vec3
toBox (EmCloud cloud, vec3 worldPosition)
{
	vec3 relative = worldPosition - cloud.centerAndExtinction.xyz;

	return vec3(dot(relative, cloud.axisX.xyz) / cloud.axisX.w, dot(relative, cloud.axisY.xyz) / cloud.axisY.w, dot(relative, cloud.axisZ.xyz) / cloud.axisZ.w);
}

/* A world direction in box space, per METRE: a box-space ray keeps the world distance as parameter. */
vec3
toBoxDirection (EmCloud cloud, vec3 worldDirection)
{
	return vec3(dot(worldDirection, cloud.axisX.xyz) / cloud.axisX.w, dot(worldDirection, cloud.axisY.xyz) / cloud.axisY.w, dot(worldDirection, cloud.axisZ.xyz) / cloud.axisZ.w);
}

/* Slab test of a box-space ray against [-1, 1]^3: (enter, exit), in the ray parameter. */
vec2
intersectBox (vec3 origin, vec3 direction)
{
	/* No division by an exact zero: an axis-aligned ray keeps a huge, correctly signed inverse. */
	vec3 safeDirection = mix(vec3(-1.0e-8), vec3(1.0e-8), greaterThanEqual(direction, vec3(0.0)));
	safeDirection = mix(safeDirection, direction, greaterThan(abs(direction), vec3(1.0e-8)));

	vec3 inverseDirection = 1.0 / safeDirection;
	vec3 t0 = (vec3(-1.0) - origin) * inverseDirection;
	vec3 t1 = (vec3(1.0) - origin) * inverseDirection;
	vec3 tMin = min(t0, t1);
	vec3 tMax = max(t0, t1);

	return vec2(max(max(tMin.x, tMin.y), tMin.z), min(min(tMax.x, tMax.y), tMax.z));
}
)GLSL"
