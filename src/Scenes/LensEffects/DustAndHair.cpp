/*
 * src/Scenes/LensEffects/DustAndHair.cpp
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

#include "DustAndHair.hpp"

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
	DustAndHair::generateFragmentShaderCode (Generator::Abstract & /*generator*/, FragmentShader & fragmentShader) const noexcept
	{
		fragmentShader.addComment("Dust and hair effect (projector gate debris shadows).");

		/* Step 1: Compute normalized screen coordinates [0..1]. */
		Code{fragmentShader} <<
			"vec2 dhUV = gl_FragCoord.xy / " << PostProcessingPC(PushConstant::Component::FrameSize) << ";";

		/* Step 2: Time seeds for dust (fast) and hair (slow persistence). */
		Code{fragmentShader} <<
			"float dhTime = " << PostProcessingPC(PushConstant::Component::Time) << ";" << Line::End <<
			"float dhDustSeed = floor(dhTime * 2.0);" << Line::End <<
			"float dhHairSeed = floor(dhTime * 0.3);";

		/* Step 3: Dust particles — small dark spots scattered across the gate.
		 * Each particle has a random position and radius, computed via Dave Hoskins hash.
		 * Particles persist for ~0.5s (seed changes at 2Hz). */
		Code{fragmentShader} <<
			"float dhDark = 0.0;" << Line::End <<
			"for (int dhI = 0; dhI < " << static_cast< int >(m_dustCount) << "; dhI++)" << Line::End <<
			"{" << Line::End <<
			"\tfloat dhS = float(dhI) * 73.13 + dhDustSeed * 37.7;" << Line::End <<
			"\tvec3 dhP = fract(vec3(dhS) * vec3(0.1031, 0.1030, 0.0973));" << Line::End <<
			"\tdhP += dot(dhP, dhP.yzx + 33.33);" << Line::End <<
			"\tvec2 dhPos = fract(vec2((dhP.x + dhP.y) * dhP.z, (dhP.y + dhP.z) * dhP.x));" << Line::End <<
			"\tfloat dhRadius = 0.003 + fract(dhS * 0.0517) * 0.005;" << Line::End <<
			"\tfloat dhDist = length(dhUV - dhPos) / dhRadius;" << Line::End <<
			"\tdhDark += 0.4 * smoothstep(1.0, 0.3, dhDist);" << Line::End <<
			"}";

		/* Step 4: Hair fibers — thin elongated curved lines.
		 * Each fiber is a multi-segment polyline. Distance from the fragment to
		 * each segment is accumulated. Fibers persist for ~3s (seed changes at 0.3Hz). */
		Code{fragmentShader} <<
			"for (int dhJ = 0; dhJ < " << static_cast< int >(m_hairCount) << "; dhJ++)" << Line::End <<
			"{" << Line::End <<
			"\tfloat dhHS = float(dhJ) * 131.7 + dhHairSeed * 53.3;" << Line::End <<
			"\tvec3 dhHP = fract(vec3(dhHS) * vec3(0.1031, 0.1030, 0.0973));" << Line::End <<
			"\tdhHP += dot(dhHP, dhHP.yzx + 33.33);" << Line::End <<
			"\tvec2 dhAnchor = fract(vec2((dhHP.x + dhHP.y) * dhHP.z, (dhHP.y + dhHP.z) * dhHP.x));" << Line::End <<
			"\tfloat dhAngle = fract(dhHS * 0.0371) * 3.14159;" << Line::End <<
			"\tvec2 dhDir = vec2(cos(dhAngle), sin(dhAngle)) * 0.08;" << Line::End <<
			"\tfloat dhCurve = (fract(dhHS * 0.0713) - 0.5) * 0.04;" << Line::End <<
			"\tfloat dhMinDist = 1.0;" << Line::End <<
			"\tfor (int dhK = 0; dhK < 4; dhK++)" << Line::End <<
			"\t{" << Line::End <<
			"\t\tfloat dhT0 = float(dhK) / 4.0;" << Line::End <<
			"\t\tfloat dhT1 = float(dhK + 1) / 4.0;" << Line::End <<
			"\t\tvec2 dhA = dhAnchor + dhDir * dhT0 + vec2(0.0, dhCurve * dhT0 * dhT0);" << Line::End <<
			"\t\tvec2 dhB = dhAnchor + dhDir * dhT1 + vec2(0.0, dhCurve * dhT1 * dhT1);" << Line::End <<
			"\t\tvec2 dhAB = dhB - dhA;" << Line::End <<
			"\t\tvec2 dhAP = dhUV - dhA;" << Line::End <<
			"\t\tfloat dhProj = clamp(dot(dhAP, dhAB) / dot(dhAB, dhAB), 0.0, 1.0);" << Line::End <<
			"\t\tdhMinDist = min(dhMinDist, length(dhAP - dhAB * dhProj));" << Line::End <<
			"\t}" << Line::End <<
			"\tdhDark += 0.5 * smoothstep(0.002, 0.0005, dhMinDist);" << Line::End <<
			"}";

		/* Step 5: Apply combined darkening to fragment. */
		Code{fragmentShader} <<
			PostProcessor::Fragment << ".rgb *= 1.0 - clamp(dhDark, 0.0, 1.0) * " << m_intensity << ";";

		return true;
	}

	void
	DustAndHair::setDustCount (float count) noexcept
	{
		if ( count < 0.0F )
		{
			Tracer::warning(ClassId, "Dust count must be >= 0 !");

			return;
		}

		m_dustCount = count;
	}

	void
	DustAndHair::setHairCount (float count) noexcept
	{
		if ( count < 0.0F )
		{
			Tracer::warning(ClassId, "Hair count must be >= 0 !");

			return;
		}

		m_hairCount = count;
	}
}
