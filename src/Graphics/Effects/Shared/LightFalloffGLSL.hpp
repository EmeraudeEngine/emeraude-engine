/*
 * src/Graphics/Effects/Shared/LightFalloffGLSL.hpp
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
 * @brief The ONE photometric falloff of a point or spot light, shared by the raster light pass and the traced lanes.
 *
 * `emLightFalloff(d, r)` = `saturate(1 - (d/r)^4)^2 / max(d^2, 0.01^2)` (d, r in metres; r <= 0 = unbounded, the
 * inverse square alone). The inverse square is what makes a light intensity in CANDELA mean something: the
 * illuminance it produces at d is `I / d^2` lux. The window forces the contribution to exactly zero at the radius, so
 * lights keep being culled by radius, and squaring it removes the visible edge. The 1 cm clamp removes the singularity
 * at the source without biasing anything a camera frames.
 * References: B. Lagarde & C. de Rousiers, "Moving Frostbite to Physically Based Rendering", SIGGRAPH 2014
 * (smoothDistanceAtt / max(d², 0.01²)); B. Karis, "Real Shading in Unreal Engine 4", SIGGRAPH 2013 (the same window).
 *
 * ⚠️⚠️ History: the windowed inverse square of 2026-07-26 used `1 / (d^2 + 1)` — Unreal's `+1` is in CENTIMETRES², in
 * metres it halved the illuminance at 1 m — and `1c1d94ba` (2026-08-12) deleted it with the Blinn-Phong machinery,
 * leaving `max(1 - (d/r)^2, 0)` (no inverse square) in every lane for six weeks. Restored here, ONCE, so the raster
 * (Saphir, through EMEN_LIGHT_FALLOFF_BODY_GLSL) and RTGI / RTR / the probes (EMEN_LIGHT_FALLOFF_GLSL) cannot drift
 * apart again.
 *
 * Usage in an effect's constexpr GLSL literal: `)GLSL" EMEN_LIGHT_FALLOFF_GLSL R"GLSL(` then `emLightFalloff(d, r)`.
 */

/** @brief The body of `float emLightFalloff (float lightDistance, float lightRadius)`. */
#define EMEN_LIGHT_FALLOFF_BODY_GLSL \
"	const float distanceSquared = lightDistance * lightDistance;\n" \
"	float window = 1.0;\n" \
"	if ( lightRadius > 0.0 )\n" \
"	{\n" \
"		const float ratioSquared = distanceSquared / (lightRadius * lightRadius);\n" \
"		window = clamp(1.0 - ratioSquared * ratioSquared, 0.0, 1.0);\n" \
"		window *= window;\n" \
"	}\n" \
"	return window / max(distanceSquared, 1.0e-4);\n"

/** @brief The whole function, for the effects' literals. */
#define EMEN_LIGHT_FALLOFF_GLSL \
"/* The photometric point/spot falloff: windowed inverse square (Graphics/Effects/Shared/LightFalloffGLSL.hpp). */\n" \
"float emLightFalloff (float lightDistance, float lightRadius)\n" \
"{\n" \
EMEN_LIGHT_FALLOFF_BODY_GLSL \
"}\n"
