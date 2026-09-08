/*
 * src/Graphics/Effects/Framebuffer/CSMSamplingGLSL.hpp
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
 * @brief The ONE cascaded-shadow-map sampling rule a post-process effect applies.
 *
 * ⚠️ The scene's own shaders get this from `LightGenerator::generateCSMShadowMapCode()`, which
 * builds it out of `Declaration::Function` objects into a GENERATED shader. A post-process effect
 * writes its GLSL as a hand-authored literal and cannot call that generator, so before this header
 * the only way to consume the cascades from the chain was to re-type the convention. **Every
 * re-typing is a place where it can drift**, and the shadow subsystem has already paid for exactly
 * that: the light-space transform is open-coded at four sites that do not agree
 * (`docs/todo/light-space-transform-single-source.md`).
 *
 * The conventions this header owns, and which must not be restated anywhere else:
 *
 * - **Cascade selection runs on VIEW-space depth**, not on a world distance. The first cascade
 *   whose split distance exceeds the depth wins; past the last split the last cascade is kept.
 * - **Only X and Y need the [-1,1] → [0,1] remap.** Z already arrives in [0,1] from the Vulkan
 *   orthographic projection — remapping it a second time is a silent half-range error.
 * - **The per-cascade depth bias is derived from the matrix**, not uploaded:
 *   `length(vec3(m[0][0], m[1][0], m[2][0]))` is the cascade's inverse radius after the
 *   bounding-sphere fit, so one authored bias scales itself across cascades whose texels differ by
 *   more than an order of magnitude. See `docs/shadow-mapping.md` § *Per-cascade depth bias*.
 * - **Outside the cascade, the sample is LIT.** Past the depth range the helper returns 1.0
 *   explicitly; outside X/Y the sampler's `CLAMP_TO_BORDER` with a white border returns 1.0 by
 *   construction. ⚠️ That border is not decoration — it was dead code until the sampler cache
 *   stopped keying on the identifier string, and everything sampled `CLAMP_TO_EDGE` instead,
 *   smearing the edge texel ring over the whole exterior.
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
 *   ...
 *   )GLSL" EMEN_CSM_SAMPLING_GLSL(3, 4) R"GLSL(
 *   void main() { float lit = emCSMVisibility(worldPos, viewDepth, cascadeCount, bias); }
 *   )GLSL";
 * @endcode
 *
 * The C++ side owes the two bindings exactly what they declare: binding `shadowBinding` gets the
 * light's shadow map through `ShadowMap::writeCombinedImageSampler()`, and binding
 * `cascadeUBOBinding` a per-frame UBO laid out as @ref EmEn::Graphics::CSMCascadeBlock — 4 mat4
 * followed by one vec4 of split distances, which is what std140 gives for `mat4[4]; vec4;`.
 *
 * ⚠️ `emCSMVisibility()` takes a view-space depth because nothing else is available: **no fragment
 * shader on the main render target can reach a view matrix** (it travels as a VERTEX-stage push
 * constant). A post-process effect that marches a world-space ray recovers the depth without one,
 * by projecting the travelled distance onto the camera forward axis — see VolumetricScattering.
 */

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <array>

namespace EmEn::Graphics
{
	/**
	 * @brief The C++ counterpart of the GLSL EmCSMCascadeBlock, for the per-frame UBO an effect
	 * uploads.
	 * @note ⚠️ It lives in the SAME file as the GLSL that reads it, on purpose: a layout and its
	 * reader that live apart drift apart. `mat4 matrices[4]; vec4 splitDistances;` is what std140
	 * gives for these members, so the two agree with no padding member.
	 * ⚠️ The split distances are a **vec4, not a float[4]**: std140 pads every element of a scalar
	 * array to 16 bytes, so `float splitDistances[4]` on the GLSL side would occupy 64 bytes with a
	 * stride of 16 and read three quarters garbage.
	 */
	struct EMEN_API CSMCascadeBlock
	{
		/** @brief Cascade view-projection matrices, column-major, 4 of them. */
		std::array< float, 64 > matrices{};
		/** @brief The four split distances, in view-space depth. */
		std::array< float, 4 > splitDistances{};
	};

	static_assert(sizeof(CSMCascadeBlock) == 272, "CSMCascadeBlock must be exactly 272 bytes to match the GLSL std140 block !");
}

/**
 * @brief Declares the cascaded shadow map, the cascade block and the sampling helpers.
 * @param shadowBinding The set-0 binding carrying the sampler2DArrayShadow.
 * @param cascadeUBOBinding The set-0 binding carrying the cascade matrices and split distances.
 */
#define EMEN_CSM_SAMPLING_GLSL(shadowBinding, cascadeUBOBinding) R"GLSL(
/* ---- The ONE cascaded-shadow-map sampling rule (Graphics/Effects/Framebuffer/CSMSamplingGLSL.hpp) ---- */

/* The cascade map is a VK_IMAGE_VIEW_TYPE_2D_ARRAY with a COMPARE-enabled sampler, so texture()
 * returns the comparison result (PCF-filtered by the hardware), never a raw depth. */
layout(set = 0, binding = )GLSL" #shadowBinding R"GLSL() uniform sampler2DArrayShadow emCSMShadowMap;

layout(set = 0, binding = )GLSL" #cascadeUBOBinding R"GLSL() uniform EmCSMCascadeBlock
{
	mat4 matrices[4];
	vec4 splitDistances;
} emCSM;

const int EmCSMMaxCascades = 4;

/* Cascade selection on VIEW-space depth. The first cascade whose split exceeds the depth wins;
 * past the last split the last cascade is kept rather than falling off the array. */
int
emCSMSelectCascade (float viewDepth, int cascadeCount)
{
	int cascadeIndex = 0;
	int count = clamp(cascadeCount, 1, EmCSMMaxCascades);

	for ( int index = 0; index < count; ++index )
	{
		cascadeIndex = index;

		if ( viewDepth < emCSM.splitDistances[index] )
		{
			break;
		}
	}

	return cascadeIndex;
}

/* Visibility of a world-space point, in [0,1]: 1.0 fully lit, 0.0 fully shadowed. */
float
emCSMVisibilityInCascade (vec3 worldPosition, int cascadeIndex, float shadowBias)
{
	mat4 cascadeMatrix = emCSM.matrices[cascadeIndex];

	vec4 positionLightSpace = cascadeMatrix * vec4(worldPosition, 1.0);
	vec3 projCoords = positionLightSpace.xyz / positionLightSpace.w;

	/* Only X and Y need the [-1,1] to [0,1] conversion: Z already is, from the Vulkan
	 * orthographic projection. */
	projCoords.xy = projCoords.xy * 0.5 + 0.5;

	/* Per-cascade depth bias in WORLD units, derived from the matrix — see docs/shadow-mapping.md. */
	float cascadeInverseRadius = length(vec3(cascadeMatrix[0][0], cascadeMatrix[1][0], cascadeMatrix[2][0]));
	projCoords.z -= shadowBias * cascadeInverseRadius / 3.0;

	/* Outside the cascade's depth range the point is LIT — the engine-wide convention.
	 * Outside X/Y the sampler's white CLAMP_TO_BORDER returns 1.0 on its own. */
	if ( projCoords.z < 0.0 || projCoords.z > 1.0 )
	{
		return 1.0;
	}

	return texture(emCSMShadowMap, vec4(projCoords.xy, float(cascadeIndex), projCoords.z));
}

/* Selection and sampling in one call, for a consumer that has no reason to know the cascade. */
float
emCSMVisibility (vec3 worldPosition, float viewDepth, int cascadeCount, float shadowBias)
{
	return emCSMVisibilityInCascade(worldPosition, emCSMSelectCascade(viewDepth, cascadeCount), shadowBias);
}
)GLSL"
