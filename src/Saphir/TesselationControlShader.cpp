/*
 * src/Saphir/TesselationControlShader.cpp
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

#include "TesselationControlShader.hpp"

/* STL inclusions. */
#include <algorithm>

/* Local inclusions. */
#include "VertexShader.hpp"
#include "Tracer.hpp"

namespace EmEn::Saphir
{
	using namespace Libs;
	using namespace Saphir::Declaration;

	bool
	TesselationControlShader::declare (const StageInput & declaration) noexcept
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
	TesselationControlShader::declare (const InputBlock & declaration) noexcept
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
	TesselationControlShader::declare (const InputPrimitive & declaration) noexcept
	{
		if ( !declaration.isValid() )
		{
			Tracer::error(ClassId, "The input primitive is invalid for code generation !");

			return false;
		}

		if ( std::ranges::any_of(m_inputPrimitives, [declaration] (const auto & existing) {return existing.name() == declaration.name();}) )
		{
			TraceWarning{ClassId} << "An input primitive declaration named '" << declaration.name() << "' already exists !";

			return true;
		}

		m_inputPrimitives.emplace_back(declaration);

		return true;
	}

	bool
	TesselationControlShader::declare (const StageOutput & declaration) noexcept
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
	TesselationControlShader::declare (const OutputBlock & declaration) noexcept
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
	TesselationControlShader::declare (const OutputPrimitive & declaration) noexcept
	{
		if ( !declaration.isValid() )
		{
			Tracer::error(ClassId, "The output primitive is invalid for code generation !");

			return false;
		}

		if ( std::ranges::any_of(m_outputPrimitives, [declaration] (const auto & existing) {return existing.name() == declaration.name();}) )
		{
			TraceWarning{ClassId} << "An output primitive declaration named '" << declaration.name() << "' already exists !";

			return true;
		}

		m_outputPrimitives.emplace_back(declaration);

		return true;
	}

	bool
	TesselationControlShader::connectFromPreviousShader (const VertexShader & vertexShader) noexcept
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
	TesselationControlShader::onSourceCodeGeneration (Generator::Abstract & /*generator*/, std::stringstream & code, std::string & /*topInstructions*/, std::string & /*outputInstructions*/) noexcept
	{
		/* Specific input shader code declarations. */
		AbstractShader::generateDeclarations(code, m_stageInputs, "Stage inputs (From previous stage)");
		AbstractShader::generateDeclarations(code, m_inputBlocks, "Input blocks (From previous stage)");
		AbstractShader::generateDeclarations(code, m_inputPrimitives, "Input primitives (Geometry shader only)");

		/* Specific output shader code declarations. */
		AbstractShader::generateDeclarations(code, m_stageOutputs, "Stage outputs (To next stage)");
		AbstractShader::generateDeclarations(code, m_outputBlocks, "Output blocks (To next stage)");
		AbstractShader::generateDeclarations(code, m_outputPrimitives, "Output primitives (Geometry shader only)");

		return true;
	}

	void
	TesselationControlShader::onGetDeclarationStats (std::stringstream & output) const noexcept
	{
		output <<
			"Tesselation control shader input declarations : " "\n"
			" - Stage input : " << m_stageInputs.size() << "\n"
			" - Input block : " << m_inputBlocks.size() << "\n"
			" - Input primitive : " << m_inputPrimitives.size() << "\n"

			"Tesselation control shader output declarations : " "\n"
			" - Stage output : " << m_stageOutputs.size() << "\n"
			" - Output block : " <<  m_outputBlocks.size() << "\n"
			" - Output primitive : " << m_outputPrimitives.size() << "\n";
	}
}
