/*
 * src/Scenes/LensEffects/ChromaticAberration.cpp
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

#include "ChromaticAberration.hpp"

/* Local inclusions. */
#include "Graphics/PostProcessor.hpp"
#include "Saphir/Generator/SceneRendering.hpp"
#include "Saphir/FragmentShader.hpp"
#include "Saphir/Code.hpp"

namespace EmEn::Scenes::LensEffects
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Saphir;
	using namespace Saphir::Keys;
	using namespace Graphics;

	ChromaticAberration::ChromaticAberration (float intensity) noexcept
		: m_intensity{intensity}
	{
		Randomizer< float > randomizer;

		m_redOffset[X] = randomizer.value(-intensity, intensity);
		m_redOffset[Y] = randomizer.value(-intensity, intensity);

		m_greenOffset[X] = randomizer.value(-intensity, intensity);
		m_greenOffset[Y] = randomizer.value(-intensity, intensity);

		m_blueOffset[X] = randomizer.value(-intensity, intensity);
		m_blueOffset[Y] = randomizer.value(-intensity, intensity);
	}

	bool
	ChromaticAberration::generateFragmentShaderCode (Generator::Abstract & /*generator*/, FragmentShader & fragmentShader) const noexcept
	{
		/* Step 1: Compute center-relative coordinates for radial/barrel modes. */
		Code{fragmentShader} <<
			"vec2 caCenter = " << ShaderVariable::Primary2DTextureCoordinates << " - vec2(0.5);";

		/* Step 2: Apply barrel distortion to UV if enabled.
		 * Barrel distortion warps UV coordinates outward based on distance from center,
		 * simulating CRT glass curvature. Applied before any RGB channel sampling. */
		if ( m_barrelEnabled )
		{
			fragmentShader.addComment("Barrel distortion (CRT glass curvature).");

			Code{fragmentShader} <<
				"float caR2 = dot(caCenter, caCenter);" << Line::End <<
				"vec2 caBarrel = caCenter * (1.0 + " << m_barrelStrength << " * caR2);";
		}
		else
		{
			Code{fragmentShader} <<
				"vec2 caBarrel = caCenter;";
		}

		Code{fragmentShader} <<
			"vec2 caBaseUV = caBarrel + vec2(0.5);";

		/* Step 3: Sample RGB channels based on aberration mode. */
		if ( m_radial )
		{
			fragmentShader.addComment("Radial chromatic aberration effect (lens dispersion).");

			/* Direction from center, scaled by distance. R pushed out, G neutral, B pulled in. */
			Code{fragmentShader} <<
				"vec2 caNorm = normalize(caCenter + vec2(0.0001));" << Line::End <<
				"float caDist = length(caCenter) * 2.0;" << Line::End <<
				"float caScale = caDist * caDist * " << m_intensity << ";" << Line::End <<
				"float caR = texture(" << Uniform::PrimarySampler << ", caBaseUV + caNorm * caScale * 1.0).r;" << Line::End <<
				"float caG = texture(" << Uniform::PrimarySampler << ", caBaseUV).g;" << Line::End <<
				"float caB = texture(" << Uniform::PrimarySampler << ", caBaseUV - caNorm * caScale * 0.8).b;" << Line::End <<
				PostProcessor::Fragment << " = vec4(caR, caG, caB, 1.0);";
		}
		else if ( !m_barrelEnabled )
		{
			fragmentShader.addComment("Linear chromatic aberration effect.");

			Code{fragmentShader} <<
				"float caR = texture(" << Uniform::PrimarySampler << ", caBaseUV + vec2(" << m_redOffset[X] << ", " << m_redOffset[Y] << ")).r;" << Line::End <<
				"float caG = texture(" << Uniform::PrimarySampler << ", caBaseUV + vec2(" << m_greenOffset[X] << ", " << m_greenOffset[Y] << ")).g;" << Line::End <<
				"float caB = texture(" << Uniform::PrimarySampler << ", caBaseUV + vec2(" << m_blueOffset[X] << ", " << m_blueOffset[Y] << ")).b;" << Line::End <<
				PostProcessor::Fragment << " = vec4(caR, caG, caB, 1.0);";
		}
		else
		{
			fragmentShader.addComment("Barrel distortion only (no chromatic aberration).");

			Code{fragmentShader} <<
				PostProcessor::Fragment << " = texture(" << Uniform::PrimarySampler << ", caBaseUV);";
		}

		return true;
	}

	void
	ChromaticAberration::setIntensity (float intensity) noexcept
	{
		if ( intensity <= 0.0F )
		{
			Tracer::warning(ClassId, "Intensity must be > 0 !");

			return;
		}

		m_intensity = intensity;

		Randomizer< float > randomizer;

		m_redOffset[X] = randomizer.value(-intensity, intensity);
		m_redOffset[Y] = randomizer.value(-intensity, intensity);

		m_greenOffset[X] = randomizer.value(-intensity, intensity);
		m_greenOffset[Y] = randomizer.value(-intensity, intensity);

		m_blueOffset[X] = randomizer.value(-intensity, intensity);
		m_blueOffset[Y] = randomizer.value(-intensity, intensity);
	}

	void
	ChromaticAberration::setBarrelStrength (float strength) noexcept
	{
		if ( strength < 0.0F )
		{
			Tracer::warning(ClassId, "Barrel strength must be >= 0 !");

			return;
		}

		m_barrelStrength = strength;
	}
}
