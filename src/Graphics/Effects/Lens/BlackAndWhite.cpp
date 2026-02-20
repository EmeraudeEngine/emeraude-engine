/*
 * src/Graphics/Effects/Lens/BlackAndWhite.cpp
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

#include "BlackAndWhite.hpp"

/* STL inclusions. */
#include <cmath>

/* Local inclusions. */
#include "Graphics/PostProcessor.hpp"
#include "Saphir/Generator/SceneRendering.hpp"
#include "Saphir/FragmentShader.hpp"
#include "Saphir/Code.hpp"

namespace EmEn::Graphics::Effects::Lens
{
	using namespace Libs;
	using namespace Saphir;
	using namespace Saphir::Keys;
	using namespace Graphics;

	bool
	BlackAndWhite::generateFragmentShaderCode (Generator::Abstract & /*generator*/, FragmentShader & fragmentShader) const noexcept
	{
		fragmentShader.addComment("Black and white effect.");

		/* Step 1: Luminance calculation (depends on mode). */
		switch ( m_mode )
		{
			case Mode::LumaRec709 :
				Code{fragmentShader} <<
					"float bwLuma = dot(" << PostProcessor::Fragment << ".rgb, vec3(0.2126, 0.7152, 0.0722));";
				break;

			case Mode::LumaRec601 :
				Code{fragmentShader} <<
					"float bwLuma = dot(" << PostProcessor::Fragment << ".rgb, vec3(0.2989, 0.5866, 0.1145));";
				break;

			case Mode::Average :
				Code{fragmentShader} <<
					"float bwLuma = dot(" << PostProcessor::Fragment << ".rgb, vec3(1.0 / 3.0));";
				break;

			case Mode::Desaturation :
				Code{fragmentShader} <<
					"float bwLuma = (min(min(" << PostProcessor::Fragment << ".r, " << PostProcessor::Fragment << ".g), " << PostProcessor::Fragment << ".b) + max(max(" << PostProcessor::Fragment << ".r, " << PostProcessor::Fragment << ".g), " << PostProcessor::Fragment << ".b)) * 0.5;";
				break;

			case Mode::Lightness :
				Code{fragmentShader} <<
					"float bwLuma = min(min(" << PostProcessor::Fragment << ".r, " << PostProcessor::Fragment << ".g), " << PostProcessor::Fragment << ".b);";
				break;

			case Mode::Value :
				Code{fragmentShader} <<
					"float bwLuma = max(max(" << PostProcessor::Fragment << ".r, " << PostProcessor::Fragment << ".g), " << PostProcessor::Fragment << ".b);";
				break;

			case Mode::CustomWeights :
				Code{fragmentShader} <<
					"float bwLuma = dot(" << PostProcessor::Fragment << ".rgb, vec3(" << m_channelWeights[0] << ", " << m_channelWeights[1] << ", " << m_channelWeights[2] << "));";
				break;
		}

		/* Step 2: Contrast + Brightness (always emitted, noop when default). */
		Code{fragmentShader} <<
			"bwLuma = clamp((bwLuma - 0.5) * " << m_contrast << " + 0.5 + " << m_brightness << ", 0.0, 1.0);";

		/* Step 3: Gamma (only emitted if != 1.0). */
		if ( std::abs(m_gamma - 1.0F) > 0.001F )
		{
			Code{fragmentShader} <<
				"bwLuma = pow(bwLuma, 1.0 / " << m_gamma << ");";
		}

		/* Step 4: Posterize (only emitted if levels > 0). */
		if ( m_levels > 0 )
		{
			Code{fragmentShader} <<
				"bwLuma = floor(bwLuma * " << m_levels << ".0 + 0.5) / " << m_levels << ".0;";
		}

		/* Step 5: Tint + output. */
		Code{fragmentShader} <<
			PostProcessor::Fragment << " = vec4(vec3(bwLuma) * vec3(" << m_tint.red() << ", " << m_tint.green() << ", " << m_tint.blue() << "), " << PostProcessor::Fragment << ".a);";

		return true;
	}

	void
	BlackAndWhite::setContrast (float contrast) noexcept
	{
		if ( contrast < 0.0F )
		{
			Tracer::warning(ClassId, "Contrast must be >= 0 !");

			return;
		}

		m_contrast = contrast;
	}

	void
	BlackAndWhite::setGamma (float gamma) noexcept
	{
		if ( gamma <= 0.0F )
		{
			Tracer::warning(ClassId, "Gamma must be > 0 !");

			return;
		}

		m_gamma = gamma;
	}
}
