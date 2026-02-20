/*
 * src/Graphics/Effects/Lens/ColorGrading.cpp
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

#include "ColorGrading.hpp"

/* Local inclusions. */
#include "Graphics/PostProcessor.hpp"
#include "Saphir/Generator/SceneRendering.hpp"
#include "Saphir/FragmentShader.hpp"
#include "Saphir/Code.hpp"

/* C++ inclusions. */
#include <cmath>

namespace EmEn::Graphics::Effects::Lens
{
	using namespace Saphir;
	using namespace Saphir::Keys;
	using namespace Graphics;

	bool
	ColorGrading::generateFragmentShaderCode (Generator::Abstract & /*generator*/, FragmentShader & fragmentShader) const noexcept
	{
		fragmentShader.addComment("Color grading effect.");

		/* Step 1: Saturation (mix between Rec.709 luma grayscale and original color). */
		Code{fragmentShader} <<
			"float cgLuma = dot(" << PostProcessor::Fragment << ".rgb, vec3(0.2126, 0.7152, 0.0722));" << Line::End <<
			PostProcessor::Fragment << ".rgb = mix(vec3(cgLuma), " << PostProcessor::Fragment << ".rgb, " << m_saturation << ");";

		/* Step 2: Hue rotation in YIQ color space (only emitted if hue != 0.0). */
		if ( std::abs(m_hue) > 0.001F )
		{
			Code{fragmentShader} <<
				"const mat3 cgToYIQ = mat3(0.299, 0.587, 0.114, 0.595716, -0.274453, -0.321263, 0.211456, -0.522591, 0.311135);" << Line::End <<
				"const mat3 cgToRGB = mat3(1.0, 0.9563, 0.6210, 1.0, -0.2721, -0.6474, 1.0, -1.107, 1.7046);" << Line::End <<
				"vec3 cgYIQ = cgToYIQ * " << PostProcessor::Fragment << ".rgb;" << Line::End <<
				"float cgHueAngle = atan(cgYIQ.z, cgYIQ.y) + " << m_hue << ";" << Line::End <<
				"float cgChroma = sqrt(cgYIQ.z * cgYIQ.z + cgYIQ.y * cgYIQ.y);" << Line::End <<
				PostProcessor::Fragment << ".rgb = cgToRGB * vec3(cgYIQ.x, cgChroma * cos(cgHueAngle), cgChroma * sin(cgHueAngle));";
		}

		/* Step 3: Contrast and brightness (always emitted, noop when default values). */
		Code{fragmentShader} <<
			PostProcessor::Fragment << ".rgb = clamp((" << PostProcessor::Fragment << ".rgb - 0.5) * " << m_contrast << " + 0.5 + " << m_brightness << ", 0.0, 1.0);";

		/* Step 4: Gamma correction (only emitted if gamma != 1.0). */
		if ( std::abs(m_gamma - 1.0F) > 0.001F )
		{
			Code{fragmentShader} <<
				PostProcessor::Fragment << ".rgb = pow(" << PostProcessor::Fragment << ".rgb, vec3(1.0 / " << m_gamma << "));";
		}

		return true;
	}

	void
	ColorGrading::setSaturation (float saturation) noexcept
	{
		if ( saturation < 0.0F )
		{
			Tracer::warning(ClassId, "Saturation must be >= 0 !");

			return;
		}

		m_saturation = saturation;
	}

	void
	ColorGrading::setContrast (float contrast) noexcept
	{
		if ( contrast < 0.0F )
		{
			Tracer::warning(ClassId, "Contrast must be >= 0 !");

			return;
		}

		m_contrast = contrast;
	}

	void
	ColorGrading::setGamma (float gamma) noexcept
	{
		if ( gamma <= 0.0F )
		{
			Tracer::warning(ClassId, "Gamma must be > 0 !");

			return;
		}

		m_gamma = gamma;
	}
}
