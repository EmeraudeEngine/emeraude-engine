/*
 * src/Graphics/Effects/Shared/LineLightGLSL.hpp
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
 * @brief The ONE line-light integral (Scenes::Component::LineLight), shared by the raster light pass and the traced lanes.
 *
 * `ltcLineDiffuse(p1, p2)` is the closed-form integral of the clamped cosine distribution D = cos θ / π over the segment
 * p1 → p2, both expressed in a frame whose z is the surface normal and whose origin is the shading point (clipped to the
 * horizon). For a thin Lambertian tube of radius R and luminance L, `R × ltcLineDiffuse` is the solid-angle integral of
 * D over the tube (checked against a brute-force cylinder integral to 0.2 %, 2026-09-30): the illuminance is
 * `π × L × R × ltcLineDiffuse`, the Lambert radiance `albedo × L × R × ltcLineDiffuse`. The raster pass also runs it
 * through the LTC of the GGX lobe for the specular (LightGenerator::generateLineLightFunctions()).
 *
 * Reference: E. Heitz & S. Hill, "Real-Time Line- and Disk-Light Shading with Linearly Transformed Cosines", SIGGRAPH
 * 2017 Physically Based Shading course — `ltc_line.fs` of https://github.com/selfshadow/ltc_code (BSD-style license,
 * retained in Graphics/LTCTables.cpp), adapted: the distance to the line is floored (a point ON the line) and a
 * degenerate segment returns 0.
 *
 * Usage in an effect's constexpr GLSL literal: `)GLSL" EMEN_LINE_LIGHT_GLSL R"GLSL(` then `emLineIrradiance(...)`,
 * `emLineReach(...)`, `emLineClosest(...)`.
 */

/** @brief The body of `float ltcFpo (float d, float l)`. */
#define EMEN_LTC_FPO_BODY_GLSL \
"	return l / (d * (d * d + l * l)) + atan(l / d) / (d * d);\n"

/** @brief The body of `float ltcFwt (float d, float l)`. */
#define EMEN_LTC_FWT_BODY_GLSL \
"	return l * l / (d * (d * d + l * l));\n"

/** @brief The body of `float ltcLineDiffuse (vec3 a, vec3 b)` (the segment in the shading frame, z along the normal). */
#define EMEN_LTC_LINE_DIFFUSE_BODY_GLSL \
"	vec3 p1 = a;\n" \
"	vec3 p2 = b;\n" \
"	if ( p1.z <= 0.0 && p2.z <= 0.0 ) { return 0.0; }\n" \
"	if ( p1.z < 0.0 ) { p1 = (p1 * p2.z - p2 * p1.z) / (p2.z - p1.z); }\n" \
"	if ( p2.z < 0.0 ) { p2 = (-p1 * p2.z + p2 * p1.z) / (-p2.z + p1.z); }\n" \
"	const vec3 segment = p2 - p1;\n" \
"	const float segmentLength = length(segment);\n" \
"	if ( segmentLength < 1.0e-6 ) { return 0.0; }\n" \
"	const vec3 wt = segment / segmentLength;\n" \
"	const float l1 = dot(p1, wt);\n" \
"	const float l2 = dot(p2, wt);\n" \
"	const vec3 po = p1 - l1 * wt;\n" \
"	const float d = max(length(po), 1.0e-4);\n" \
"	return ((ltcFpo(d, l2) - ltcFpo(d, l1)) * po.z + (ltcFwt(d, l2) - ltcFwt(d, l1)) * wt.z) / 3.14159265;\n"

/** @brief The whole family, for the effects' literals (world-space helpers included). */
#define EMEN_LINE_LIGHT_GLSL \
"/* The line-light integral (Graphics/Effects/Shared/LineLightGLSL.hpp; Heitz & Hill 2017). */\n" \
"float ltcFpo (float d, float l)\n{\n" EMEN_LTC_FPO_BODY_GLSL "}\n" \
"float ltcFwt (float d, float l)\n{\n" EMEN_LTC_FWT_BODY_GLSL "}\n" \
"float ltcLineDiffuse (vec3 a, vec3 b)\n{\n" EMEN_LTC_LINE_DIFFUSE_BODY_GLSL "}\n" \
"/* The illuminance a tube of unit luminance and radius tubeRadius along a -> b gives a surface at position, normal. */\n" \
"float emLineIrradiance (vec3 a, vec3 b, vec3 position, vec3 normal, float tubeRadius)\n" \
"{\n" \
"	const vec3 reference = abs(normal.y) < 0.99 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);\n" \
"	const vec3 tangent = normalize(cross(reference, normal));\n" \
"	const mat3 frame = transpose(mat3(tangent, cross(normal, tangent), normal));\n" \
"	return 3.14159265 * tubeRadius * ltcLineDiffuse(frame * (a - position), frame * (b - position));\n" \
"}\n" \
"/* The point of the segment a -> b closest to position. */\n" \
"vec3 emLineClosest (vec3 a, vec3 b, vec3 position)\n" \
"{\n" \
"	const vec3 segment = b - a;\n" \
"	return a + segment * clamp(dot(position - a, segment) / max(dot(segment, segment), 1.0e-12), 0.0, 1.0);\n" \
"}\n" \
"/* The reach window at a distance from the line (reach <= 0: unbounded) — the point light's window, no inverse square. */\n" \
"float emLineReach (float lineDistance, float reach)\n" \
"{\n" \
"	if ( reach <= 0.0 ) { return 1.0; }\n" \
"	const float ratio = lineDistance / reach;\n" \
"	const float window = clamp(1.0 - ratio * ratio * ratio * ratio, 0.0, 1.0);\n" \
"	return window * window;\n" \
"}\n"
