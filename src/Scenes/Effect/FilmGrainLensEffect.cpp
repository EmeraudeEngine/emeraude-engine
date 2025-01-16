/*
 * src/Scenes/Effect/FilmGrainLensEffect.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2024 - "LondNoir" <londnoir@gmail.com>
 *
 * Emeraude-Engine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Emeraude-Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Emeraude-Engine; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 *
 * Complete project and additional information can be found at :
 * https://bitbucket.org/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "FilmGrainLensEffect.hpp"

/* Local inclusions. */
#include "Graphics/PostProcessor.hpp"
#include "Saphir/Generator/SceneRendering.hpp"
#include "Saphir/FragmentShader.hpp"
#include "Saphir/Code.hpp"

namespace Emeraude::Scenes
{
	using namespace Libraries;
	using namespace Saphir;
	using namespace Saphir::Keys;
	using namespace Graphics;

	bool
	FilmGrainLensEffect::generateFragmentShaderCode (Generator::Abstract & /*generator*/, Saphir::FragmentShader & fragmentShader) const noexcept
	{
		//fragmentShader.declare(Declaration::Uniform{Declaration::VariableType::Float, RandomA, "0.0"});
		//fragmentShader.declare(Declaration::Uniform{Declaration::VariableType::Float, RandomB, "0.5"});
		//fragmentShader.declare(Declaration::Uniform{Declaration::VariableType::Float, RandomC, "1.0"});

		fragmentShader.addComment("Film grain effect.");

		// FIXME : Set a parametric output fragment coming from the generator.
		Code{fragmentShader} <<
			"if ( "
				"mod(gl_FragCoord.y, " << RandomA << ") < 0.02 || "
				"( "
					"mod(gl_FragCoord.x, " << RandomB << ") < 0.05 && "
					"mod(gl_FragCoord.y, " << RandomA << " * 2) < 0.05"
				" )"
			" )" << Line::End <<
			'\t' << PostProcessor::Fragment << " = mix(" << PostProcessor::Fragment << ", vec4(" << RandomA << ", " << RandomB << ", " << RandomC << ", 1.0), 0.5);";

		return true;
	}

	bool
	FilmGrainLensEffect::requestScreenSize () const noexcept
	{
		return true;
	}
}
