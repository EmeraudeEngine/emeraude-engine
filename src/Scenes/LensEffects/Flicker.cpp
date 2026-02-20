/*
 * src/Scenes/LensEffects/Flicker.cpp
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

#include "Flicker.hpp"

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
	Flicker::generateFragmentShaderCode (Generator::Abstract & /*generator*/, FragmentShader & fragmentShader) const noexcept
	{
		fragmentShader.addComment("Flicker effect (arc-lamp projector luminosity instability).");

		/* Step 1: Discrete frame seed (snaps to film frame rate).
		 * All pixels in the same frame get the same seed → uniform flicker. */
		Code{fragmentShader} <<
			"float flkFrame = floor(" << PostProcessingPC(PushConstant::Component::Time) << " * " << m_speed << ");";

		/* Step 2: Dave Hoskins hash on the frame seed.
		 * Produces a pseudo-random value per frame, no sin() artifacts. */
		Code{fragmentShader} <<
			"vec3 flkH = fract(vec3(flkFrame) * vec3(0.1031, 0.1030, 0.0973));" << Line::End <<
			"flkH += dot(flkH, flkH.yzx + 33.33);" << Line::End <<
			"float flkNoise = fract((flkH.x + flkH.y) * flkH.z);";

		/* Step 3: Apply multiplicative luminosity variation.
		 * (noise - 0.5) centers the variation around 0, so the image
		 * can get slightly brighter or darker per frame. */
		Code{fragmentShader} <<
			PostProcessor::Fragment << ".rgb *= 1.0 + (flkNoise - 0.5) * " << m_intensity << ";";

		return true;
	}

	void
	Flicker::setSpeed (float fps) noexcept
	{
		if ( fps <= 0.0F )
		{
			Tracer::warning(ClassId, "Speed must be > 0 !");

			return;
		}

		m_speed = fps;
	}
}
