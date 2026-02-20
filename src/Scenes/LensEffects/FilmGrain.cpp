/*
 * src/Scenes/LensEffects/FilmGrain.cpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "FilmGrain.hpp"

/* Local inclusions. */
#include "Graphics/PostProcessor.hpp"
#include "Saphir/Generator/SceneRendering.hpp"
#include "Saphir/FragmentShader.hpp"
#include "Saphir/Code.hpp"
#include "Saphir/Keys.hpp"

namespace EmEn::Scenes::LensEffects
{
	using namespace Saphir;
	using namespace Saphir::Keys;
	using namespace Graphics;

	bool
	FilmGrain::generateFragmentShaderCode (Generator::Abstract & /*generator*/, FragmentShader & fragmentShader) const noexcept
	{
		fragmentShader.addComment("Film grain effect (two-layer silver halide simulation).");

		/* Step 1: Discrete frame seed (snaps to 24fps film frames). */
		Code{fragmentShader} <<
			"float fgFrame = floor(" << PostProcessingPC(PushConstant::Component::Time) << " * 24.0);";

		/* Step 2: Fine grain layer (pixel-level, high frequency).
		 * Dave Hoskins hash: no sin() artifacts, no diagonal patterns. */
		Code{fragmentShader} <<
			"vec3 fgP1 = fract(vec3((gl_FragCoord.xy + fgFrame * 64.0).xyx) * 0.1031);" << Line::End <<
			"fgP1 += dot(fgP1, fgP1.yzx + 33.33);" << Line::End <<
			"float fgFine = fract((fgP1.x + fgP1.y) * fgP1.z);";

		/* Step 3: Coarse grain layer (crystal clumps, low frequency).
		 * Simulates silver halide crystal clusters on the film emulsion. */
		Code{fragmentShader} <<
			"vec3 fgP2 = fract(vec3((floor(gl_FragCoord.xy / " << m_size << ") + fgFrame * 37.0).xyx) * 0.1031);" << Line::End <<
			"fgP2 += dot(fgP2, fgP2.yzx + 33.33);" << Line::End <<
			"float fgCoarse = fract((fgP2.x + fgP2.y) * fgP2.z);";

		/* Step 4: Blend layers (70% fine + 30% coarse for organic feel). */
		Code{fragmentShader} <<
			"float fgNoise = mix(fgFine, fgCoarse, 0.3);";

		/* Step 5: Film-like luminance response.
		 * Real film grain is strongest in midtones, weaker in shadows/highlights. */
		Code{fragmentShader} <<
			"float fgLuma = dot(" << PostProcessor::Fragment << ".rgb, vec3(0.2126, 0.7152, 0.0722));" << Line::End <<
			"float fgMidtoneMask = 4.0 * fgLuma * (1.0 - fgLuma);" << Line::End <<
			"float fgGrain = (fgNoise - 0.5) * " << m_intensity << " * (0.3 + 0.7 * fgMidtoneMask);";

		/* Step 6: Apply grain to fragment. */
		Code{fragmentShader} <<
			PostProcessor::Fragment << ".rgb = clamp(" << PostProcessor::Fragment << ".rgb + vec3(fgGrain), 0.0, 1.0);";

		return true;
	}

	void
	FilmGrain::setSize (float size) noexcept
	{
		if ( size <= 0.0F )
		{
			Tracer::warning(ClassId, "Grain size must be > 0 !");

			return;
		}

		m_size = size;
	}
}
