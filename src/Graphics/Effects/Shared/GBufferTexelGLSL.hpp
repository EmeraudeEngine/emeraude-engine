/*
 * src/Graphics/Effects/Shared/GBufferTexelGLSL.hpp
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

/* STL inclusions. */
#include <algorithm>
#include <cstdint>
#include <string>

/**
 * @file
 * @brief The ONE way a pass rendered at a REDUCED resolution names the G-buffer texel each of its pixels stands for.
 *
 * ⚠️ Why (owner decision 2026-11-24; graphics doc 13-12 § 45): the centre of a pixel of a half-resolution pass is, in
 * the full-resolution G-buffer, exactly the CORNER four texels share (`vUV × size = 2x + 1`). Read there —
 * `texture(depthTex, vUV)` through the G-buffer's NEAREST samplers, or `texelFetch(depthTex, ivec2(vUV * size), 0)` —
 * the texel returned hangs on the last bit of the interpolated UV, and a position unprojected at `vUV` with that depth
 * lies half a texel away from the point the depth belongs to: off the surface by that offset times the depth slope, on
 * one side or the other with the pick. The pick repeats with the rounding, so the error draws a lattice locked to the
 * screen — SSGI's column grid at 1280 × 720 (a line every 10 px, half width 640 = 2⁷ · 5; none at 1024, half width
 * 512).
 *
 * The rule, for every pass whose target is smaller than the G-buffer:
 * - the pixel's texel is `emGBufferTexel(size)`: integer arithmetic on the pixel's own coordinates, the FIRST texel
 *   (lowest x and y) of the block the pixel covers — owner decision 2026-11-24, the fixed choice, one fetch;
 * - depth, normal and material properties of the centre are read from THAT texel with `texelFetch`;
 * - whatever is unprojected or marched from the centre starts at `emGBufferTexelUV(texel, size)`, the centre of the
 *   texel the depth was read from, never at `vUV`.
 *
 * ⚠️ What that choice costs, by intent: a reduced pass computes its result for a point half a full-resolution texel
 * up and left of its pixel's centre, and the combine reads it back as if it stood at that centre. Negligible for a
 * diffuse term; state it again before using the helper for a sharp one.
 *
 * The reduction is stated ONCE by the effect, in C++, and reaches both sides: `reducedSize()` sizes the pass's targets,
 * `sourceGLSL()` gives the shader the same factor.
 *
 * Usage:
 * @code
 *   constexpr uint32_t Reduction{2};
 *   const auto width = GBufferTexelGLSL::reducedSize(fullWidth, Reduction);
 *   const auto source = std::string{Header} + GBufferTexelGLSL::sourceGLSL(Reduction) + Body;
 *   // GLSL:
 *   //   const ivec2 gbufferSize = textureSize(depthTex, 0);
 *   //   const ivec2 centerTexel = emGBufferTexel(gbufferSize);
 *   //   const vec2 centerUV = emGBufferTexelUV(centerTexel, gbufferSize);
 *   //   float depth = texelFetch(depthTex, centerTexel, 0).r;
 * @endcode
 */
namespace EmEn::Graphics::GBufferTexelGLSL
{
	/**
	 * @brief Returns one side of a pass reduced by a factor: the full side divided by it, at least 1.
	 * @param fullSize The side of the G-buffer, in texels.
	 * @param reduction How many G-buffer texels a pixel of the pass covers along that side. 0 is read as 1.
	 * @return uint32_t
	 */
	[[nodiscard]]
	constexpr
	uint32_t
	reducedSize (uint32_t fullSize, uint32_t reduction) noexcept
	{
		return std::max(fullSize / std::max(reduction, 1U), 1U);
	}

	/** @brief The helpers (after the reduction's constant: sourceGLSL()). Fragment stage only (gl_FragCoord). */
	constexpr auto FunctionsGLSL = R"GLSL(
/* ---- The G-buffer texel of a reduced pass (Graphics/Effects/Shared/GBufferTexelGLSL.hpp) ---- */

/* The G-buffer texel this fragment stands for: the first texel (lowest x and y) of the block of emGBufferReduction
 * texels a side its pixel covers. Integer arithmetic on the pixel's own coordinates: no rounding decides the texel.
 * gbufferSize: textureSize() of the G-buffer attachment read (they all share one size). */
ivec2 emGBufferTexel (ivec2 gbufferSize)
{
	return clamp(ivec2(gl_FragCoord.xy) * emGBufferReduction, ivec2(0), gbufferSize - ivec2(1));
}

/* The screen UV of a G-buffer texel's centre: what a depth read from that texel is unprojected with. */
vec2 emGBufferTexelUV (ivec2 texel, ivec2 gbufferSize)
{
	return (vec2(texel) + vec2(0.5)) / vec2(gbufferSize);
}
)GLSL";

	/**
	 * @brief Returns the reduction's constant and the helpers, ready to splice into a fragment shader's source.
	 * @param reduction How many G-buffer texels a pixel of the pass covers along each side — the factor given to
	 * reducedSize() for the pass's targets. 0 is read as 1.
	 * @return std::string
	 */
	[[nodiscard]]
	inline
	std::string
	sourceGLSL (uint32_t reduction) noexcept
	{
		return "\nconst int emGBufferReduction = " + std::to_string(std::max(reduction, 1U)) + ";\n" + FunctionsGLSL;
	}
}
