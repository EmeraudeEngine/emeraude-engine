/*
 * src/Graphics/Effects/Framebuffer/MarchDitherGLSL.hpp
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
 * @brief The ONE per-pixel dither every stepped march of the engine offsets its origin by.
 *
 * ⚠️ **Rule, from `docs/caution-points.md` § *Undithered Radial God Rays*: never ship uniform steps
 * without a per-pixel dither of the march origin.** Undersampling does not show up as noise — it
 * shows up as coherent BANDING on any source smaller than one step, and banding is TAA-hostile:
 * a bright source between two taps paints a discrete ghost copy of itself at every tap that does
 * catch it. Measured on the Sponza corridor bench, the undithered god rays were the single most
 * unstable feature of the whole frame (peak-to-peak flips of 220/255 against 12 for the GI
 * mottle); dithering took the trail region's >64/255 flips from 1979-2707 down to 210-350.
 *
 * ⚠️⚠️ **The dither is deliberately STATIC per pixel — do not mix a frame index in.** TAA already
 * integrates over its own jitter; a frame-varying dither fights the history and measurably
 * regresses (same rule as the RTGI noise). This is the one place in the engine where a
 * screen-space-fixed pattern is the correct answer rather than the crawl trap the PCF kernel
 * rotation had to be moved to world space to escape — because what slides through the pattern here
 * is a MARCH ORIGIN along a ray, not a surface.
 *
 * Interleaved gradient noise: J. Jimenez, *Next Generation Post Processing in Call of Duty:
 * Advanced Warfare*, SIGGRAPH 2014.
 *
 * The rule is a MACRO holding a string literal so it concatenates into an effect's own `constexpr`
 * GLSL literal at compile time — a `constexpr const char *` cannot be spliced into one. Same
 * technique, and same reason, as EMEN_RT_ALPHA_TEST_GLSL in
 * [`RTAlphaTestGLSL.hpp`](RTAlphaTestGLSL.hpp).
 *
 * Usage:
 * @code
 *   static constexpr auto MyShader = R"GLSL(
 *   #version 450
 *   )GLSL" EMEN_MARCH_DITHER_GLSL R"GLSL(
 *   void main()
 *   {
 *       float offset = emInterleavedGradientNoise(gl_FragCoord.xy); // in [0,1), one step's worth
 *   }
 *   )GLSL";
 * @endcode
 *
 * ⚠️ **Four effects still open-code this exact expression** — `MotionBlur.cpp:283`,
 * `VolumetricLight.cpp:165`, `RTR.cpp:1701`, `SSR.cpp:264`. They are bit-identical to the helper
 * below (same constants, same `fract(a * fract(dot(p, b)))` shape), so migrating them is a
 * behaviour-preserving no-op — but their GLSL is compiled at RUNTIME, so a typo in one would not
 * be caught by the C++ build. The migration is tracked as its own verified step:
 * `docs/todo/march-dither-single-source.md`.
 */

/**
 * @brief Declares emInterleavedGradientNoise(vec2), the engine's march-origin dither.
 */
#define EMEN_MARCH_DITHER_GLSL R"GLSL(
/* ---- The ONE march-origin dither (Graphics/Effects/Framebuffer/MarchDitherGLSL.hpp) ---- */

/* Interleaved gradient noise, in [0,1). STATIC per pixel on purpose — never mix a frame index in. */
float
emInterleavedGradientNoise (vec2 position)
{
	return fract(52.9829189 * fract(dot(position, vec2(0.06711056, 0.00583715))));
}
)GLSL"
