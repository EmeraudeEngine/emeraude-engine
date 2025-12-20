/*
 * src/Saphir/FragmentShader.cpp
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

#include "FragmentShader.hpp"

/* STL inclusions. */
#include <algorithm>

/* Local inclusions. */
#include "VertexShader.hpp"
#include "TesselationEvaluationShader.hpp"
#include "GeometryShader.hpp"
#include "Tracer.hpp"

namespace EmEn::Saphir
{
	using namespace Libs;
	using namespace Saphir::Declaration;
	using namespace Saphir::Keys;
	using namespace Graphics;

	bool
	FragmentShader::declare (const StageInput & declaration) noexcept
	{
		if ( !declaration.isValid() )
		{
			Tracer::error(ClassId, "The stage input is invalid for code generation !");

			return false;
		}

		if ( std::ranges::any_of(m_stageInputs, [declaration] (const auto & existing) {return existing.name() == declaration.name();}) )
		{
			TraceWarning{ClassId} << "A stage input declaration named '" << declaration.name() << "' already exists !";

			return true;
		}

		m_stageInputs.emplace_back(declaration);

		return true;
	}

	bool
	FragmentShader::declare (const InputBlock & declaration) noexcept
	{
		if ( !declaration.isValid() )
		{
			Tracer::error(ClassId, "The input block is invalid for code generation !");

			return false;
		}

		if ( std::ranges::any_of(m_inputBlocks, [declaration] (const auto & existing) {return existing.instanceName() == declaration.instanceName();}) )
		{
			TraceWarning{ClassId} << "An input block declaration named '" << declaration.name() << "' already exists !";

			return true;
		}

		m_inputBlocks.emplace_back(declaration);

		return true;
	}

	bool
	FragmentShader::declare(const OutputFragment & declaration) noexcept
	{
		if ( !declaration.isValid() )
		{
			Tracer::error(ClassId, "The output fragment is invalid for code generation !");

			return false;
		}

		if ( std::ranges::any_of(m_outputFragments, [declaration] (const auto & existing) {return existing.name() == declaration.name();}) )
		{
			TraceWarning{ClassId} << "An output fragment declaration named '" << declaration.name() << "' already exists !";

			return true;
		}

		m_outputFragments.emplace_back(declaration);

		return true;
	}

	bool
	FragmentShader::connectFromPreviousShader (const VertexShader & vertexShader) noexcept
	{
		if ( !vertexShader.isGenerated() )
		{
			TraceError{ClassId} << "The vertex shader '" << vertexShader.name() << "' is not generated !";

			return false;
		}

		if ( vertexShader.stageOutputs().empty() && vertexShader.outputBlocks().empty() )
		{
			/* NOTE : This can only have gl_Position. */
			//TraceWarning{ClassId} << "The vertex shader '" << vertexShader.name() << "' has no output declaration !";

			return true;
		}

		for ( const auto & stageOutput : vertexShader.stageOutputs() )
		{
			this->declare(StageInput{stageOutput});
		}

		for ( const auto & outputBlock : vertexShader.outputBlocks() )
		{
			this->declare(InputBlock{outputBlock});
		}

		return true;
	}

	bool
	FragmentShader::connectFromPreviousShader (const TesselationEvaluationShader & tesselationEvaluationShader) noexcept
	{
		if ( !tesselationEvaluationShader.isGenerated() )
		{
			TraceError{ClassId} << "The tesselation evaluation shader '" << tesselationEvaluationShader.name() << "' is not generated !";

			return false;
		}

		if ( tesselationEvaluationShader.stageOutputs().empty() && tesselationEvaluationShader.outputBlocks().empty() )
		{
			TraceWarning{ClassId} << "The tesselation evaluation shader '" << tesselationEvaluationShader.name() << "' has no output declaration !";

			return false;
		}

		for ( const auto & stageOutput : tesselationEvaluationShader.stageOutputs() )
		{
			this->declare(StageInput{stageOutput});
		}

		for ( const auto & outputBlock : tesselationEvaluationShader.outputBlocks() )
		{
			this->declare(InputBlock{outputBlock});
		}

		return true;
	}

	bool
	FragmentShader::connectFromPreviousShader (const GeometryShader & geometryShader) noexcept
	{
		if ( !geometryShader.isGenerated() )
		{
			TraceError{ClassId} << "The geometry shader '" << geometryShader.name() << "' is not generated !";

			return false;
		}

		if ( geometryShader.stageOutputs().empty() && geometryShader.outputBlocks().empty() )
		{
			TraceWarning{ClassId} << "The geometry shader '" << geometryShader.name() << "' has no output declaration !";

			return false;
		}

		for ( const auto & stageOutput : geometryShader.stageOutputs() )
		{
			this->declare(StageInput{stageOutput});
		}

		for ( const auto & outputBlock : geometryShader.outputBlocks() )
		{
			this->declare(InputBlock{outputBlock});
		}

		return true;
	}

	bool
	FragmentShader::onSourceCodeGeneration (Generator::Abstract & /*generator*/, std::stringstream & code, std::string & /*topInstructions*/, std::string & /*outputInstructions*/) noexcept
	{
		/* Specific input shader code declarations. */
		AbstractShader::generateDeclarations(code, m_stageInputs, "Stage inputs (From previous stage)");
		AbstractShader::generateDeclarations(code, m_inputBlocks, "Input blocks (From previous stage)");

		/* Specific output shader code declarations. */
		AbstractShader::generateDeclarations(code, m_outputFragments, "Output fragments (Fragment shader only)");

		return true;
	}

	void
	FragmentShader::onGetDeclarationStats (std::stringstream & output) const noexcept
	{
		output <<
		 	"Fragment shader input declarations : " "\n"
			" - Stage input : " << m_stageInputs.size() << "\n"
			" - Input block : " << m_inputBlocks.size() << "\n"

			"Vertex shader output declarations : " "\n"
			" - Output fragment (FS) : " << m_outputFragments.size() << "\n";
	}

	Function
	FragmentShader::generateToSRGBColorFunction () noexcept
	{
		std::stringstream functionCode;

		/* NOTE: Only convert RGB components, alpha channel is always linear. */
		functionCode <<
			"\t" "bvec3 cutoff = lessThan(linearRGB.rgb, vec3(0.0031308));" "\n"
			"\t" "vec3 higher = vec3(1.055) * pow(linearRGB.rgb, vec3(1.0 / 2.4)) - vec3(0.055);" "\n"
			"\t" "vec3 lower = linearRGB.rgb * vec3(12.92);" "\n\n"

			"\t" "return vec4(mix(higher, lower, cutoff), linearRGB.a);" "\n";

		Function function{"toSRGBColor", GLSL::FloatVector4};
		function.addInParameter(GLSL::FloatVector4, "linearRGB", true);
		function.addInstruction(functionCode.str());

		return function;
	}

	Function
	FragmentShader::generateToLinearColorFunction () noexcept
	{
		std::stringstream functionCode;

		/* NOTE: Only convert RGB components, alpha channel is always linear. */
		functionCode <<
			"\t" "bvec3 cutoff = lessThan(sRGB.rgb, vec3(0.04045));" "\n"
			"\t" "vec3 higher = pow((sRGB.rgb + vec3(0.055)) / vec3(1.055), vec3(2.4));" "\n"
			"\t" "vec3 lower = sRGB.rgb / vec3(12.92);" "\n\n"

			"\t" "return vec4(mix(higher, lower, cutoff), sRGB.a);" "\n";

		Function function{"toLinearColor", GLSL::FloatVector4};
		function.addInParameter(GLSL::FloatVector4, "sRGB", true);
		function.addInstruction(functionCode.str());

		return function;
	}
}
