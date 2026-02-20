/*
 * src/Graphics/Effects/Lens/Vignetting.cpp
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

#include "Vignetting.hpp"

/* Local inclusions. */
#include "Graphics/PostProcessor.hpp"
#include "Saphir/Generator/SceneRendering.hpp"
#include "Saphir/FragmentShader.hpp"
#include "Saphir/Code.hpp"
#include "Saphir/Keys.hpp"

namespace EmEn::Graphics::Effects::Lens
{
	using namespace Saphir;
	using namespace Saphir::Keys;
	using namespace Graphics;

	bool
	Vignetting::generateFragmentShaderCode (Generator::Abstract & /*generator*/, FragmentShader & fragmentShader) const noexcept
	{
		fragmentShader.addComment("Vignetting effect (parametric radial darkening).");

		/* Step 1: Compute normalized coordinates centered at screen middle.
		 * Uses push constant frameSize (vec2) for aspect-correct UV. */
		Code{fragmentShader} <<
			"vec2 vigUV = gl_FragCoord.xy / " << PostProcessingPC(PushConstant::Component::FrameSize) << " - 0.5;";

		/* Step 2: Aspect ratio correction so the vignette is round, not stretched. */
		Code{fragmentShader} <<
			"float vigAspect = " << PostProcessingPC(PushConstant::Component::FrameSize) << ".x / " << PostProcessingPC(PushConstant::Component::FrameSize) << ".y;" << Line::End <<
			"vigUV.x *= vigAspect;";

		/* Step 3: Compute distance from center using configurable roundness.
		 * roundness=1.0 gives a circle (Euclidean), roundness=2.0 approaches a rectangle. */
		if ( m_roundness == 1.0F )
		{
			Code{fragmentShader} <<
				"float vigDist = length(vigUV);";
		}
		else
		{
			Code{fragmentShader} <<
				"float vigDist = pow(pow(abs(vigUV.x), " << (m_roundness * 2.0F) << ") + pow(abs(vigUV.y), " << (m_roundness * 2.0F) << "), " << (1.0F / (m_roundness * 2.0F)) << ");";
		}

		/* Step 4: Smoothstep falloff from radius to radius+softness. */
		Code{fragmentShader} <<
			"float vigFactor = 1.0 - smoothstep(" << m_radius << ", " << (m_radius + m_softness) << ", vigDist) * " << m_intensity << ";";

		/* Step 5: Apply vignette to fragment. */
		Code{fragmentShader} <<
			PostProcessor::Fragment << ".rgb *= vigFactor;";

		return true;
	}

	void
	Vignetting::setSoftness (float softness) noexcept
	{
		if ( softness <= 0.0F )
		{
			Tracer::warning(ClassId, "Softness must be > 0 !");

			return;
		}

		m_softness = softness;
	}

	void
	Vignetting::setRoundness (float roundness) noexcept
	{
		if ( roundness <= 0.0F )
		{
			Tracer::warning(ClassId, "Roundness must be > 0 !");

			return;
		}

		m_roundness = roundness;
	}
}
