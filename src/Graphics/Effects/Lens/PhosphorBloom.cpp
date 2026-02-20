/*
 * src/Graphics/Effects/Lens/PhosphorBloom.cpp
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

#include "PhosphorBloom.hpp"

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
	PhosphorBloom::generateFragmentShaderCode (Generator::Abstract & /*generator*/, FragmentShader & fragmentShader) const noexcept
	{
		fragmentShader.addComment("CRT phosphor bloom effect.");

		/* Step 1: Compute texel size from frame dimensions. */
		Code{fragmentShader} <<
			"vec2 pbTexel = " << m_spread << " / " << PostProcessingPC(PushConstant::Component::FrameSize) << ";";

		/* Step 2: 9-tap weighted cross filter (4 + 2*4 + 1*4 = 20 weight total).
		 * Center weighted x4, cardinal neighbors x2, diagonal neighbors x1. */
		Code{fragmentShader} <<
			"vec3 pbBlur = texture(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << ").rgb * 4.0;" << Line::End <<
			"pbBlur += texture(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << " + vec2(pbTexel.x, 0.0)).rgb * 2.0;" << Line::End <<
			"pbBlur += texture(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << " - vec2(pbTexel.x, 0.0)).rgb * 2.0;" << Line::End <<
			"pbBlur += texture(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << " + vec2(0.0, pbTexel.y)).rgb * 2.0;" << Line::End <<
			"pbBlur += texture(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << " - vec2(0.0, pbTexel.y)).rgb * 2.0;" << Line::End <<
			"pbBlur += texture(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << " + pbTexel).rgb;" << Line::End <<
			"pbBlur += texture(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << " + vec2(-pbTexel.x, pbTexel.y)).rgb;" << Line::End <<
			"pbBlur += texture(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << " + vec2(pbTexel.x, -pbTexel.y)).rgb;" << Line::End <<
			"pbBlur += texture(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << " - pbTexel).rgb;" << Line::End <<
			"pbBlur /= 20.0;";

		/* Step 3: Extract bright areas above threshold and add as bloom. */
		Code{fragmentShader} <<
			"vec3 pbBright = max(pbBlur - vec3(" << m_threshold << "), vec3(0.0));" << Line::End <<
			PostProcessor::Fragment << ".rgb += pbBright * " << m_intensity << ";" << Line::End <<
			PostProcessor::Fragment << ".rgb = clamp(" << PostProcessor::Fragment << ".rgb, 0.0, 1.0);";

		return true;
	}

	void
	PhosphorBloom::setSpread (float spread) noexcept
	{
		if ( spread <= 0.0F )
		{
			Tracer::warning(ClassId, "Spread must be > 0 !");

			return;
		}

		m_spread = spread;
	}
}
