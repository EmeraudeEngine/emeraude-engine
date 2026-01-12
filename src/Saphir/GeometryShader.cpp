/*
 * src/Saphir/GeometryShader.cpp
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

#include "GeometryShader.hpp"

/* STL inclusions. */
#include <algorithm>

/* Local inclusions. */
#include "TesselationEvaluationShader.hpp"
#include "Tracer.hpp"
#include "VertexShader.hpp"

namespace EmEn::Saphir
{
	using namespace Libs;
	using namespace Saphir::Declaration;

	bool
	GeometryShader::declare (const StageInput & declaration) noexcept
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
	GeometryShader::declare (const InputBlock & declaration) noexcept
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
	GeometryShader::declare (const StageOutput & declaration) noexcept
	{
		if ( !declaration.isValid() )
		{
			Tracer::error(ClassId, "The stage output is invalid for code generation !");

			return false;
		}

		if ( std::ranges::any_of(m_stageOutputs, [declaration] (const auto & existing) {return existing.name() == declaration.name();}) )
		{
			TraceWarning{ClassId} << "A stage output declaration named '" << declaration.name() << "' already exists !";

			return true;
		}

		m_stageOutputs.emplace_back(declaration);

		return true;
	}

	bool
	GeometryShader::declare (const OutputBlock & declaration) noexcept
	{
		if ( !declaration.isValid() )
		{
			Tracer::error(ClassId, "The output block is invalid for code generation !");

			return false;
		}

		if ( std::ranges::any_of(m_outputBlocks, [declaration] (const auto & existing) {return existing.instanceName() == declaration.instanceName();}) )
		{
			TraceWarning{ClassId} << "An output block declaration named '" << declaration.name() << "' already exists !";

			return true;
		}

		m_outputBlocks.emplace_back(declaration);

		return true;
	}
	
	bool
	GeometryShader::connectFromPreviousShader (const VertexShader & vertexShader) noexcept
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

			return false;
		}

		int32_t inputVertexCount = 0;

		switch ( m_inputPrimitive.primitiveType() )
		{
			case InputPrimitiveType::Points :
				inputVertexCount = 1;
				break;

			case InputPrimitiveType::Lines :
				inputVertexCount = 2;
				break;

			case InputPrimitiveType::LineAdjacency :
				inputVertexCount = 4;
				break;

			case InputPrimitiveType::Triangles :
				inputVertexCount = 3;
				break;

			case InputPrimitiveType::TrianglesAdjacency :
				inputVertexCount = 6;
				break;
		}

		if ( inputVertexCount > 1 )
		{
			for ( const auto & stageOutput : vertexShader.stageOutputs() )
			{
				this->declare(StageInput{
					stageOutput.location(),
					stageOutput.type(),
					stageOutput.name(),
					stageOutput.interpolation(),
					inputVertexCount
				});
			}
		}
		else
		{
			for ( const auto & stageOutput : vertexShader.stageOutputs() )
			{
				this->declare(StageInput{stageOutput});
			}
		}

		for ( const auto & outputBlock : vertexShader.outputBlocks() )
		{
			this->declare(InputBlock{outputBlock});
		}

		return true;
	}

	bool
	GeometryShader::connectFromPreviousShader (const TesselationEvaluationShader & tesselationEvaluationShader) noexcept
	{
		if ( !tesselationEvaluationShader.isGenerated() )
		{
			TraceError{ClassId} << "The tesselation evaluation shader '" << tesselationEvaluationShader.name() << "' is not generated !";

			return false;
		}

		if ( tesselationEvaluationShader.stageOutputs().empty() && tesselationEvaluationShader.outputBlocks().empty() && tesselationEvaluationShader.outputPrimitives().empty() )
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

		/*for ( const auto & outputPrimitive : tesselationEvaluationShader.outputPrimitives() )
		{
			this->declare(InputPrimitive{outputPrimitive.name()});
		}*/

		return true;
	}

	bool
	GeometryShader::onSourceCodeGeneration (Generator::Abstract & /*generator*/, std::stringstream & code, std::string & /*topInstructions*/, std::string & /*outputInstructions*/) noexcept
	{
		code <<
			"/* Input primitive */" "\n" <<
			m_inputPrimitive.sourceCode() <<
			'\n';

		code <<
			"/* Output primitive */" "\n" <<
			m_outputPrimitive.sourceCode() <<
			'\n';

		/* Specific input shader code declarations. */
		AbstractShader::generateDeclarations(code, m_stageInputs, "Stage inputs (From previous stage)");
		AbstractShader::generateDeclarations(code, m_inputBlocks, "Input blocks (From previous stage)");

		/* Specific output shader code declarations. */
		AbstractShader::generateDeclarations(code, m_stageOutputs, "Stage outputs (To next stage)");
		AbstractShader::generateDeclarations(code, m_outputBlocks, "Output blocks (To next stage)");

		return true;
	}

	void
	GeometryShader::onGetDeclarationStats (std::stringstream & output) const noexcept
	{
		output <<
			"Geometry shader input declarations : " "\n"
			" - Stage input : " << m_stageInputs.size() << "\n"
			" - Input block : " << m_inputBlocks.size() << "\n"

			"Geometry shader output declarations : " "\n"
			" - Stage output : " << m_stageOutputs.size() << "\n"
			" - Output block : " <<  m_outputBlocks.size() << "\n";
	}
}
