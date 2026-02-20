/*
 * src/Scenes/LensEffects/ChromaBleeding.cpp
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

#include "ChromaBleeding.hpp"

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
	ChromaBleeding::generateFragmentShaderCode (Generator::Abstract & /*generator*/, FragmentShader & fragmentShader) const noexcept
	{
		fragmentShader.addComment("Chroma bleeding effect (NTSC/PAL low-bandwidth chroma).");

		/* Step 1: Compute horizontal texel offset for chroma sampling. */
		Code{fragmentShader} <<
			"vec2 cbTexel = vec2(" << m_spread << " / " << PostProcessingPC(PushConstant::Component::FrameSize) << ".x, 0.0);";

		/* Step 2: Extract sharp luminance from current processed pixel. */
		Code{fragmentShader} <<
			"float cbY = dot(" << PostProcessor::Fragment << ".rgb, vec3(0.299, 0.587, 0.114));";

		/* Step 3: Sample horizontal neighbors for chroma blur (5-tap weighted).
		 * Weights: 1-2-4-2-1 = 10 total, simulating low-pass chroma filter. */
		Code{fragmentShader} <<
			"vec3 cbLeft  = texture(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << " - cbTexel).rgb;" << Line::End <<
			"vec3 cbRight = texture(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << " + cbTexel).rgb;" << Line::End <<
			"vec3 cbLeft2  = texture(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << " - cbTexel * 2.0).rgb;" << Line::End <<
			"vec3 cbRight2 = texture(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << " + cbTexel * 2.0).rgb;" << Line::End <<
			"vec3 cbBlur = (cbLeft2 + cbLeft * 2.0 + " << PostProcessor::Fragment << ".rgb * 4.0 + cbRight * 2.0 + cbRight2) / 10.0;";

		/* Step 4: Extract blurred U and V chrominance components. */
		Code{fragmentShader} <<
			"float cbU = dot(cbBlur, vec3(-0.14713, -0.28886, 0.436));" << Line::End <<
			"float cbV = dot(cbBlur, vec3(0.615, -0.51499, -0.10001));";

		/* Step 5: Reconstruct color from sharp Y + blurred UV. */
		Code{fragmentShader} <<
			"vec3 cbResult = vec3(" << Line::End <<
			"    cbY + 1.13983 * cbV," << Line::End <<
			"    cbY - 0.39465 * cbU - 0.58060 * cbV," << Line::End <<
			"    cbY + 2.03211 * cbU" << Line::End <<
			");";

		/* Step 6: Mix with original based on intensity. */
		Code{fragmentShader} <<
			PostProcessor::Fragment << ".rgb = mix(" << PostProcessor::Fragment << ".rgb, cbResult, " << m_intensity << ");";

		return true;
	}

	void
	ChromaBleeding::setSpread (float spread) noexcept
	{
		if ( spread <= 0.0F )
		{
			Tracer::warning(ClassId, "Spread must be > 0 !");

			return;
		}

		m_spread = spread;
	}
}
