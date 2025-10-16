/*
 * src/Scenes/Effect/ChromaticAberrationLensEffect.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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

#include "ChromaticAberrationLensEffect.hpp"

/* Local inclusions. */
#include "Graphics/PostProcessor.hpp"
#include "Saphir/Generator/SceneRendering.hpp"
#include "Saphir/FragmentShader.hpp"
#include "Saphir/Code.hpp"

namespace EmEn::Scenes
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Saphir;
	using namespace Saphir::Keys;
	using namespace Graphics;

	ChromaticAberrationLensEffect::ChromaticAberrationLensEffect (float xScale, float yScale) noexcept
	{
		Randomizer< float > randomizer;

		m_redOffset[X] = randomizer.value(-xScale, xScale);
		m_redOffset[Y] = randomizer.value(-yScale, yScale);

		m_greenOffset[X] = randomizer.value(-xScale, xScale);
		m_greenOffset[Y] = randomizer.value(-yScale, yScale);

		m_blueOffset[X] = randomizer.value(-xScale, xScale);
		m_blueOffset[Y] = randomizer.value(-yScale, yScale);
	}

	bool
	ChromaticAberrationLensEffect::generateFragmentShaderCode (Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept
	{
		m_samples = fragmentShader.samples();

		fragmentShader.addComment("Basic chromatic aberration effect.");

		Code{fragmentShader} << "float r, g, b;";

		if ( m_samples > 0 )
		{
			//gen.declare(Declaration::Uniform{Declaration::VariableType::Sampler2DMS, Uniform::PrimarySampler});
			//gen.declare(Declaration::Uniform{Declaration::VariableType::SIntVector2, RedOffset, "ivec2(0, 0)"});
			//gen.declare(Declaration::Uniform{Declaration::VariableType::SIntVector2, GreenOffset, "ivec2(0, 0)"});
			//gen.declare(Declaration::Uniform{Declaration::VariableType::SIntVector2, BlueOffset, "ivec2(0, 0)"});

			Code{fragmentShader} <<
				"ivec2 texCoords = ivec2(gl_FragCoord.xy);" << Line::Blank <<

				"for ( int subSample = 0; subSample < " << m_samples << "; subSample++ )" << Line::End <<
				"{" << Line::End <<
				"	r += texelFetch(" << Uniform::PrimarySampler << ", texCoords + " << RedOffset << ", subSample).r;" << Line::End <<
				"	g += texelFetch(" << Uniform::PrimarySampler << ", texCoords + " << GreenOffset << ", subSample).g;" << Line::End <<
				"	b += texelFetch(" << Uniform::PrimarySampler << ", texCoords + " << BlueOffset << ", subSample).b;" << Line::End <<
				"}" << Line::Blank <<

				"r = clamp(r / " << m_samples << ", 0.0, 1.0);" << Line::End <<
				"g = clamp(g / " << m_samples << ", 0.0, 1.0);" << Line::End <<
				"b = clamp(b / " << m_samples << ", 0.0, 1.0);" << Line::End;
		}
		else
		{
			//gen.declare(Declaration::Uniform{Declaration::VariableType::Sampler2D, Uniform::PrimarySampler});
			//gen.declare(Declaration::Uniform{Declaration::VariableType::FloatVector2, RedOffset, "vec2(0.0, 0.0)"});
			//gen.declare(Declaration::Uniform{Declaration::VariableType::FloatVector2, GreenOffset, "vec2(0.0, 0.0)"});
			//gen.declare(Declaration::Uniform{Declaration::VariableType::FloatVector2, BlueOffset, "vec2(0.0, 0.0)"});

			fragmentShader.declare(Declaration::StageInput{generator.getNextShaderVariableLocation(), GLSL::FloatVector2, ShaderVariable::Primary2DTextureCoordinates, GLSL::Smooth});

			Code{fragmentShader} <<
				"r = texture(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << " + " << RedOffset << ").r;" << Line::End <<
				"g = texture(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << " + " << GreenOffset << ").g;" << Line::End <<
				"b = texture(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << " + " << BlueOffset << ").b;" << Line::End;
		}

		// FIXME : Set a parametric output fragment coming from the generator.
		Code{fragmentShader} <<
			"vec4 " << PostProcessor::Fragment << " = vec4(r, g, b, 1.0);";

		return true;
	}

	bool
	ChromaticAberrationLensEffect::overrideFragmentFetching () const noexcept
	{
		return true;
	}
}
