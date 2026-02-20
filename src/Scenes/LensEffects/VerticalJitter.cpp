/*
 * src/Scenes/LensEffects/VerticalJitter.cpp
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

#include "VerticalJitter.hpp"

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
	VerticalJitter::generateFragmentShaderCode (Generator::Abstract & /*generator*/, FragmentShader & fragmentShader) const noexcept
	{
		fragmentShader.addComment("Vertical jitter effect (film pull-down claw mechanical instability).");

		/* Step 1: Discrete frame seed (snaps to film frame rate).
		 * Use a different seed offset (171.0) than Flicker to avoid correlation. */
		Code{fragmentShader} <<
			"float vjFrame = floor(" << PostProcessingPC(PushConstant::Component::Time) << " * " << m_speed << ") + 171.0;";

		/* Step 2: Dave Hoskins hash on the frame seed.
		 * Produces a pseudo-random value per frame for vertical displacement. */
		Code{fragmentShader} <<
			"vec3 vjH = fract(vec3(vjFrame) * vec3(0.1031, 0.1030, 0.0973));" << Line::End <<
			"vjH += dot(vjH, vjH.yzx + 33.33);" << Line::End <<
			"float vjOffset = (fract((vjH.x + vjH.y) * vjH.z) - 0.5) * " << m_intensity << ";";

		/* Step 3: Displace UV vertically and re-sample the color buffer.
		 * This overrides fragment fetching — we replace the fragment entirely. */
		Code{fragmentShader} <<
			"vec2 vjUV = " << ShaderVariable::Primary2DTextureCoordinates << " + vec2(0.0, vjOffset);" << Line::End <<
			PostProcessor::Fragment << " = texture(" << Uniform::PrimarySampler << ", vjUV);";

		return true;
	}

	void
	VerticalJitter::setSpeed (float fps) noexcept
	{
		if ( fps <= 0.0F )
		{
			Tracer::warning(ClassId, "Speed must be > 0 !");

			return;
		}

		m_speed = fps;
	}
}
