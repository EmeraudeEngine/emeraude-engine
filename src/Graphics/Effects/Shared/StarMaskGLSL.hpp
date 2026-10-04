/*
 * src/Graphics/Effects/Shared/StarMaskGLSL.hpp
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
 * @brief The ONE mask of a celestial body declared IN the environment texture, shared by every reader of that
 * texture that also gets the body's light from the analytic directional light: the IBL bake
 * (Graphics::Compute::IBLBaker), the ray-traced indirect diffuse (RTGI) and the irradiance probe volume.
 *
 * `emMaskStar(L, starMask, rimWidening)` returns the direction to sample instead of L: L itself outside the masked
 * cone, a point of the cone's rim inside it — the sky right next to the body, never the body. `starMask` is
 * (direction toward the body, cone half-angle in radians; w <= 0 = no mask), Scenes::Scene::environmentStarMask().
 * `rimWidening` widens the cone by the footprint of the texel being read, in radians (a texel of a mip averages the
 * body into it, and a rim sample taken there would leak it back): about (PI/2) * 2^mip / faceSize.
 *
 * ⚠️⚠️ Why every reader needs it (2026-10-05): the ray-traced lanes read the RAW environment cubemap while only the
 * IBL bake masked the body. On Sponza (Kloppenheim 05, whose in-texture sun carries ~80 % of the sky's illuminance on
 * the ground) a GI or probe ray landing on the sun disc brought the sun back a second time, UNSHADOWED, as rare
 * enormous samples — whole-frame flashes of the probe volume (luminance 57 → 74 in one frame, then a decay through
 * the hysteresis). docs/caution-points.md § The in-texture sun was counted twice by the
 * ray-traced lanes.
 *
 * Usage in an effect's GLSL literal: `)GLSL" EMEN_STAR_MASK_GLSL R"GLSL(`.
 */

#define EMEN_STAR_MASK_GLSL \
"/* The in-texture celestial body mask (Graphics/Effects/Shared/StarMaskGLSL.hpp). */\n" \
"vec3 emMaskStar (vec3 L, vec4 starMask, float rimWidening)\n" \
"{\n" \
"	if ( starMask.w <= 0.0 )\n" \
"	{\n" \
"		return L;\n" \
"	}\n" \
"\n" \
"	const vec3 S = starMask.xyz;\n" \
"	const float cosAngle = dot(L, S);\n" \
"	const float rim = starMask.w + rimWidening;\n" \
"\n" \
"	if ( cosAngle <= cos(rim) )\n" \
"	{\n" \
"		return L;\n" \
"	}\n" \
"\n" \
"	vec3 perpendicular = L - S * cosAngle;\n" \
"	const float perpendicularLength = length(perpendicular);\n" \
"\n" \
"	if ( perpendicularLength < 1e-4 )\n" \
"	{\n" \
"		/* Looking straight at the body: any point of the rim circle will do. */\n" \
"		perpendicular = normalize(cross(S, abs(S.y) < 0.9 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0)));\n" \
"	}\n" \
"	else\n" \
"	{\n" \
"		perpendicular /= perpendicularLength;\n" \
"	}\n" \
"\n" \
"	return normalize(S * cos(rim) + perpendicular * sin(rim));\n" \
"}\n"
