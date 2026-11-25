/*
 * src/Graphics/Effects/Shared/SeaCausticsGLSL.hpp
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
 * @brief The ONE statement of how a sea's waves focus the sun's light under them: the caustics' gain.
 *
 * Two shaders draw the same caustics and must agree: the sun's light pass, on what lies under the sea (generated,
 * `Saphir::LightGenerator`), and the water volume, in the water itself — the sun's shafts
 * (`Effects::Atmosphere::WaterVolume`, a hand-authored literal). Both take the two functions from here: the generated
 * one declares them from the two bodies, the literal one splices sourceGLSL(). What each fetches (the
 * curvature of each wave cascade, the shelter map) stays its own: a bindless slot on one side, a binding on the other.
 *
 * - `emSeaCurvatureLevel(blur, texels, cascadeSize)`: the mip of a cascade's curvature whose texel is `blur` metres —
 *   the low-pass that keeps the single refraction branch honest (the waves shorter than it crossed their focus above
 *   the receiver), never finer than the texture.
 * - `emSeaCausticGain(curvature, path)`: the irradiance the refracted wavefront concentrates after `path` metres in
 *   the water, 1 / |det(I − k · path · H)|, H the Hessian of the wave height (curvature = ∂²h/∂x², ∂²h/∂z², ∂²h/∂x∂z)
 *   and k = 1 − 1/1.33; bounded at 8, where the single branch stops meaning anything.
 *
 * References: J. Guardado, D. Sánchez-Crespo, "Rendering Water Caustics", GPU Gems, ch. 2, 2004; E. Wallace,
 * "WebGL Water", 2011 (the irradiance as an area ratio of the refracted wavefront). Graphics doc 45.
 */

/* STL inclusions. */
#include <string>

namespace EmEn::Graphics::SeaCausticsGLSL
{
	/** @brief The body of `float emSeaCurvatureLevel (float blur, float texels, float cascadeSize)`. */
	constexpr auto CurvatureLevelBody{"\treturn log2(max(blur * texels / cascadeSize, 1.0));\n"};

	/** @brief The body of `float emSeaCausticGain (vec3 curvature, float path)`. */
	constexpr auto CausticGainBody{
		"\tconst float focus = 0.248 * path;\n"
		"\tconst float area = (1.0 - focus * curvature.x) * (1.0 - focus * curvature.y) - (focus * curvature.z) * (focus * curvature.z);\n"
		"\treturn 1.0 / max(abs(area), 0.125);\n"
	};

	/**
	 * @brief Returns the two functions, for a hand-authored GLSL source.
	 * @return std::string
	 */
	[[nodiscard]]
	inline
	std::string
	sourceGLSL () noexcept
	{
		return
			std::string{"\n/* ---- The caustics' gain (Graphics/Effects/Shared/SeaCausticsGLSL.hpp) ---- */\n"} +
			"float\nemSeaCurvatureLevel (float blur, float texels, float cascadeSize)\n{\n" + CurvatureLevelBody + "}\n\n" +
			"float\nemSeaCausticGain (vec3 curvature, float path)\n{\n" + CausticGainBody + "}\n";
	}
}
