/*
 * src/Saphir/VertexShader.cpp
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

#include "VertexShader.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <cstring>
#include <set>
#include <sstream>
#include <vector>
#include <string>
#include <ranges>

/* Local inclusions. */
#include "Generator/Abstract.hpp"
#include "Declaration/OutputBlock.hpp"
#include "Declaration/StageOutput.hpp"
#include "Graphics/Types.hpp"
#include "Declaration/InputAttribute.hpp"
#include "AbstractShader.hpp"
#include "Keys.hpp"
#include "Tracer.hpp"
#include "Types.hpp"

namespace EmEn::Saphir
{
	using namespace Libs;
	using namespace Saphir::Declaration;
	using namespace Saphir::Keys;
	using namespace Graphics;

	Function
	VertexShader::generateComputeUpwardVectorFunction () noexcept
	{
		std::stringstream functionCode;

		functionCode <<
			// Special case when backward is equal to downward (0, 1, 0).
			"\t" "if ( backward == vec3(0.0, 1.0, 0.0) )" "\n"
			"\t" "{" "\n"
			"\t\t" "return vec3(0.0, 0.0, 1.0);" "\n"
			"\t" "}" "\n\n"

			// Special case when backward is equal to upward (0, -1, 0).
			"\t" "if ( backward == vec3(0.0, -1.0, 0.0) )" "\n"
			"\t" "{" "\n"
			"\t\t" "return vec3(0.0, 0.0, -1.0);" "\n"
			"\t" "}" "\n\n"

			"\t" "vec3 downward;" "\n\n"

			// Compute temporal downward vector based on the backward
			// vector watch out when look up/down at 90 degree for
			// example, backward vector is on the Y axis.
			"\t" "if ( abs(backward.x) < 0.0 && abs(backward.z) < 0.0 )" "\n"
			"\t" "{" "\n"
			// If backward vector is pointing on +Y axis.
			"\t\t" "downward = backward.y > 0.0 ? vec3(0.0, 0.0, -1.0) : vec3(0.0, 0.0, 1.0);" "\n"
			"\t" "}" "\n"
			"\t" "else" "\n"
			"\t" "{" "\n"
			// In general, downward vector is straight down.
			"\t\t" "downward = vec3(0.0, 1.0, 0.0);" "\n"
			"\t" "}" "\n\n"

			// Re-calculate the orthonormal downward vector with right vector.
			"\t" "const vec3 right = normalize(cross(downward, backward));" "\n\n"

			"\t" "return cross(backward, right);" "\n";

		Function function{"computeYAxis", GLSL::FloatVector3};
		function.addInParameter(GLSL::FloatVector3, "backward");
		function.addInstruction(functionCode.str());

		return function;
	}

	Function
	VertexShader::generateGetBillBoardModelMatrixFunction () noexcept
	{
		std::stringstream functionCode;

		functionCode <<
			"\t" "const vec3 backward = normalize(cameraPosition - modelPosition);" "\n"
			"\t" "const vec3 downward = computeYAxis(backward);" "\n"
			"\t" "const vec3 right = cross(downward, backward);" "\n"
			"\t" "const mat4 scalingMatrix = mat4(" "\n"
			"\t\t" "vec4(modelScaling.x, 0.0, 0.0, 0.0), " "\n"
			"\t\t" "vec4(0.0, modelScaling.y, 0.0, 0.0), " "\n"
			"\t\t" "vec4(0.0, 0.0, modelScaling.z, 0.0), " "\n"
			"\t\t" "vec4(0.0, 0.0, 0.0, 1.0)" "\n" // TODO: Try to make it work with uniform scaling 1.0/UScale
			"\t" ");" "\n\n"

			"\t" "return mat4(vec4(right, 0.0), vec4(downward.xyz, 0.0), vec4(backward, 0.0), vec4(modelPosition, 1.0)) * scalingMatrix;" "\n";

		Function function{"getBillBoardModelMatrix", GLSL::Matrix4};
		function.addInParameter(GLSL::ConstFloatVector3, "cameraPosition");
		function.addInParameter(GLSL::ConstFloatVector3, "modelPosition");
		function.addInParameter(GLSL::ConstFloatVector3, "modelScaling");
		function.addInstruction(functionCode.str());

		return function;
	}

	bool
	VertexShader::declare (const InputAttribute & declaration, bool quiet) noexcept
	{
		if ( !declaration.isValid() )
		{
			Tracer::error(ClassId, "The input attribute is invalid for code generation !");

			return false;
		}

		if ( !this->isInstancingEnabled() && declaration.isModelMatricesVBOAttribute() )
		{
			TraceError{ClassId} << "This is invalid to add a matrix attribute with non instancing vertex shader !";

			return false;
		}

		if ( std::ranges::any_of(m_inputAttributes, [declaration] (const auto & existing) {return existing.name() == declaration.name();}) )
		{
			if ( !quiet )
			{
				TraceWarning{ClassId} << "An input attribute declaration named '" << declaration.name() << "' already exists !";
			}

			return true;
		}

		m_inputAttributes.emplace_back(declaration);

		m_vertexAttributes.emplace(static_cast< VertexAttributeType >(declaration.location()));

		return true;
	}

	bool
	VertexShader::declare (const StageOutput & declaration) noexcept
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
	VertexShader::declare (const OutputBlock & declaration) noexcept
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
	VertexShader::preparationAlreadyDone (const char * preparation) const noexcept
	{
		/*return std::ranges::any_of(m_uniquePreparations, [preparation] (const auto & pair) {
			return std::strcmp(pair.first, preparation) == 0;
		});*/

		/* TODO: Check if the code above is right ! */
		for ( const auto & variableType : std::ranges::views::keys(m_uniquePreparations) )
		{
			if ( std::strcmp(variableType, preparation) == 0 )
			{
				return true;
			}
		}

		return false;
	}

	bool
	VertexShader::prepareSpriteModelMatrix () noexcept
	{
		if ( this->preparationAlreadyDone(ShaderVariable::SpriteModelMatrix) )
		{
			return true;
		}

		if ( !this->isBillBoardingEnabled() )
		{
			Tracer::error(ClassId, "The billBoarding is not enabled !");

			return false;
		}

		std::stringstream code;

		if ( !this->declare(InputAttribute{VertexAttributeType::ModelPosition}) )
		{
			return false;
		}

		if ( !this->declare(InputAttribute{VertexAttributeType::ModelScaling}) )
		{
			return false;
		}

		this->declare(VertexShader::generateComputeUpwardVectorFunction());
		this->declare(VertexShader::generateGetBillBoardModelMatrixFunction());

		/* TODO: Find a way to get the camera world position directly (View UBO is not constantly updated for now) */
		code <<
			"\t" "const mat4 InvView = inverse(" << MatrixPC(PushConstant::Component::ViewMatrix) << ");" "\n"
			"\t" "const mat4 " << ShaderVariable::SpriteModelMatrix << " = getBillBoardModelMatrix(InvView[3].xyz, " << Attribute::ModelPosition << ", " << Attribute::ModelScaling << ");" "\n\n";

		m_uniquePreparations.emplace_back(ShaderVariable::SpriteModelMatrix, code.str());

		return true;
	}

	bool
	VertexShader::prepareModelViewMatrix () noexcept
	{
		if ( this->preparationAlreadyDone(ShaderVariable::ModelViewMatrix) )
		{
			return true;
		}

		std::stringstream code;

		if ( this->isInstancingEnabled() )
		{
			if ( this->isBillBoardingEnabled() )
			{
				if ( !this->prepareSpriteModelMatrix() )
				{
					return false;
				}

				code << "\t" "const mat4 " << ShaderVariable::ModelViewMatrix << " = " << MatrixPC(PushConstant::Component::ViewMatrix) << " * " << ShaderVariable::SpriteModelMatrix << ";" "\n";
			}
			else
			{
				if ( !this->declare(InputAttribute{VertexAttributeType::ModelMatrixR0}) )
				{
					return false;
				}

				code << "\t" "const mat4 " << ShaderVariable::ModelViewMatrix << " = " << MatrixPC(PushConstant::Component::ViewMatrix) << " * " << Attribute::ModelMatrix << ";" "\n";
			}
		}
		else
		{
			/* NOTE: For unique sprite (this->isBillBoardingEnabled()), the model matrix is already oriented to the camera. */
			code << "\t" "const mat4 " << ShaderVariable::ModelViewMatrix << " = " << MatrixPC(PushConstant::Component::ViewMatrix) << " * " << MatrixPC(PushConstant::Component::ModelMatrix) << ";" "\n";
		}

		m_uniquePreparations.emplace_back(ShaderVariable::ModelViewMatrix, code.str());

		return true;
	}

	bool
	VertexShader::prepareNormalMatrix () noexcept
	{
		if ( this->preparationAlreadyDone(ShaderVariable::NormalMatrix) )
		{
			return true;
		}

		if ( !this->prepareModelViewMatrix() )
		{
			return false;
		}

		std::stringstream code{};

		code << "\t" "const mat3 " << ShaderVariable::NormalMatrix << " = transpose(mat3(inverse(" << ShaderVariable::ModelViewMatrix << ")));" "\n";

		m_uniquePreparations.emplace_back(ShaderVariable::NormalMatrix, code.str());

		return true;
	}

	bool
	VertexShader::prepareModelViewProjectionMatrix () noexcept
	{
		if ( this->preparationAlreadyDone(ShaderVariable::ModelViewProjectionMatrix) )
		{
			return true;
		}

		std::stringstream code;

		if ( this->isInstancingEnabled() )
		{
			if ( this->isBillBoardingEnabled() )
			{
				if ( !this->prepareSpriteModelMatrix() )
				{
					return false;
				}

				code << "\t" "const mat4 " << ShaderVariable::ModelViewProjectionMatrix << " = " << MatrixPC(PushConstant::Component::ViewProjectionMatrix) << " * " << ShaderVariable::SpriteModelMatrix << ";" "\n";
			}
			else
			{
				if ( !this->declare(InputAttribute{VertexAttributeType::ModelMatrixR0}) )
				{
					return false;
				}

				code << "\t" "const mat4 " << ShaderVariable::ModelViewProjectionMatrix << " = " << MatrixPC(PushConstant::Component::ViewProjectionMatrix) << " * " << Attribute::ModelMatrix << ";" "\n";
			}
		}
		else
		{
			/* NOTE: For unique sprite (this->isBillBoardingEnabled()), the model matrix is already oriented to the camera. */
			code << "\t" "const mat4 " << ShaderVariable::ModelViewProjectionMatrix << " = " << ViewUB(Keys::UniformBlock::Component::ProjectionMatrix) << " * " << MatrixPC(PushConstant::Component::ViewMatrix) << " * " << MatrixPC(PushConstant::Component::ModelMatrix) << ";" "\n";
		}

		m_uniquePreparations.emplace_back(ShaderVariable::ModelViewProjectionMatrix, code.str());

		return true;
	}

	bool
	VertexShader::synthesizeVertexPositionInWorldSpace (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, VariableScope scope, bool asGLStandardPosition) noexcept
	{
		if ( !this->declare(InputAttribute{VertexAttributeType::Position}, true) )
		{
			return false;
		}

		std::stringstream code{};

		code << '\t';

		if ( asGLStandardPosition )
		{
			code << "gl_Position = ";
		}
		else
		{
			if ( scope == VariableScope::Local )
			{
				code << "const vec4 ";
			}
			else if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector4, ShaderVariable::PositionWorldSpace, GLSL::Smooth}) )
			{
				return false;
			}

			code << ShaderVariable::PositionWorldSpace << " = ";
		}

		if ( this->isInstancingEnabled() )
		{
			if ( !this->declare(InputAttribute{VertexAttributeType::ModelMatrixR0}) )
			{
				return false;
			}

			code << Attribute::ModelMatrix << " * vec4(" << Attribute::Position << ", 1.0);" "\n";
		}
		else
		{
			code << MatrixPC(PushConstant::Component::ModelMatrix) << " * vec4(" << Attribute::Position << ", 1.0);" "\n";
		}

		if ( scope != VariableScope::ToNextStage )
		{
			topInstructions.append(code.str());
		}
		else
		{
			outputInstructions.append(code.str());
		}

		return true;
	}

	bool
	VertexShader::synthesizeVertexPositionInViewSpace (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, VariableScope scope) noexcept
	{
		if ( !this->declare(InputAttribute{VertexAttributeType::Position}, true) )
		{
			return false;
		}

		std::stringstream code{};

		code << '\t';

		if ( scope == VariableScope::Local )
		{
			code << "const vec4 ";
		}
		else if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector4, ShaderVariable::PositionViewSpace, GLSL::Smooth}) )
		{
			return false;
		}

		if ( !this->prepareModelViewMatrix() )
		{
			return false;
		}

		code << ShaderVariable::PositionViewSpace << " = " << ShaderVariable::ModelViewMatrix << " * vec4(" << Attribute::Position << ", 1.0);" "\n";

		if ( scope != VariableScope::ToNextStage )
		{
			topInstructions.append(code.str());
		}
		else
		{
			outputInstructions.append(code.str());
		}

		return true;
	}

	bool
	VertexShader::synthesizeVertexPositionInScreenSpace (std::string & outputInstructions) noexcept
	{
		if ( !this->declare(InputAttribute{VertexAttributeType::Position}, true) )
		{
			return false;
		}

		std::string MVPMatrix;

		if ( this->isInstancingEnabled() || this->isAdvancedMatricesEnabled() )
		{
			if ( !this->prepareModelViewProjectionMatrix() )
			{
				return false;
			}

			MVPMatrix = ShaderVariable::ModelViewProjectionMatrix;
		}
		else
		{
			MVPMatrix = MatrixPC(PushConstant::Component::ModelViewProjectionMatrix);
		}

		outputInstructions.append((std::stringstream{} << "\t" "gl_Position = "  << MVPMatrix << " * vec4(" << Attribute::Position << ", 1.0);" "\n").str());

		return true;
	}

	bool
	VertexShader::synthesizeVertexPositionInTextureSpace (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, VariableScope scope) noexcept
	{
		topInstructions.append((std::stringstream{} <<
			"\t" "const float positionTextureX = dot(-" << ShaderVariable::PositionViewSpace << ".xyz, " << ShaderVariable::TangentViewSpace << ");" "\n" <<
			"\t" "const float positionTextureY = dot(-" << ShaderVariable::PositionViewSpace << ".xyz, " << ShaderVariable::BinormalViewSpace << ");" "\n" <<
			"\t" "const float positionTextureZ = dot(-" << ShaderVariable::PositionViewSpace << ".xyz, " << ShaderVariable::NormalViewSpace << ");" "\n\n"
		).str());

		/* FIXME: Rework this to avoid code duplication. */
		if ( scope == VariableScope::Local )
		{
			topInstructions.append((std::stringstream{} << "	const vec4 " << ShaderVariable::PositionTextureSpace << " = vec4(positionTextureX, positionTextureY, positionTextureZ, 1.0);" "\n").str());
		}
		else
		{
			if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector4, ShaderVariable::PositionTextureSpace, GLSL::Smooth}) )
			{
				return false;
			}

			const auto subCode = (std::stringstream{} << '\t' << ShaderVariable::PositionTextureSpace << " = vec4(positionTextureX, positionTextureY, positionTextureZ, 1.0);" "\n").str();

			if ( scope != VariableScope::ToNextStage )
			{
				topInstructions.append(subCode);
			}
			else
			{
				outputInstructions.append(subCode);
			}
		}

		return true;
	}

	bool
	VertexShader::synthesizeVertexColor (Generator::Abstract & generator, std::string & outputInstructions) noexcept
	{
		if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector4, ShaderVariable::PrimaryVertexColor, GLSL::Smooth}) )
		{
			return false;
		}

		if ( !this->declare(InputAttribute{VertexAttributeType::VertexColor}) )
		{
			return false;
		}

		outputInstructions.append((std::stringstream{} <<
			'\t' << ShaderVariable::PrimaryVertexColor << " = " << Attribute::Color << ';' << '\n'
		).str());

		return true;
	}

	bool
	VertexShader::synthesizeVertexTextureCoordinates (Generator::Abstract & generator, std::string & outputInstructions, const char * TCVariableName) noexcept
	{
		if ( std::strcmp(TCVariableName, ShaderVariable::Primary2DTextureCoordinates) == 0 )
		{
			if ( !this->declare(InputAttribute{VertexAttributeType::Primary2DTextureCoordinates}) )
			{
				return false;
			}

			if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector2, TCVariableName, GLSL::Smooth}) )
			{
				return false;
			}

			outputInstructions.append((std::stringstream{} <<
				'\t' << TCVariableName << " = " << Attribute::Primary2DTextureCoordinates << ";" "\n"
			).str());
		}
		else if ( std::strcmp(TCVariableName, ShaderVariable::Primary3DTextureCoordinates) == 0 )
		{
			if ( !this->declare(InputAttribute{VertexAttributeType::Primary3DTextureCoordinates}) )
			{
				return false;
			}

			if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector3, TCVariableName, GLSL::Smooth}) )
			{
				return false;
			}

			outputInstructions.append((std::stringstream{} <<
				'\t' << TCVariableName << " = " << Attribute::Primary3DTextureCoordinates << ";" "\n"
			).str());
		}
		else if ( std::strcmp(TCVariableName, ShaderVariable::Secondary2DTextureCoordinates) == 0 )
		{
			if ( !this->declare(InputAttribute{VertexAttributeType::Secondary2DTextureCoordinates}) )
			{
				return false;
			}

			if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector2, TCVariableName, GLSL::Smooth}) )
			{
				return false;
			}

			outputInstructions.append((std::stringstream{} <<
				'\t' << TCVariableName << " = " << Attribute::Secondary2DTextureCoordinates << ";" "\n"
			).str());
		}
		else if ( std::strcmp(TCVariableName, ShaderVariable::Secondary3DTextureCoordinates) == 0 )
		{
			if ( !this->declare(InputAttribute{VertexAttributeType::Secondary3DTextureCoordinates}) )
			{
				return false;
			}

			if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector3, TCVariableName, GLSL::Smooth}) )
			{
				return false;
			}

			outputInstructions.append((std::stringstream{} <<
				'\t' << TCVariableName << " = " << Attribute::Secondary3DTextureCoordinates << ";" "\n"
			).str());
		}
		else
		{
			TraceError{ClassId} << "Texture coordinates variable '" << TCVariableName << "' is not handled !";

			return false;
		}

		return true;
	}

	bool
	VertexShader::synthesizeVertexVectorInWorldSpace (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, VertexAttributeType vectorType, VariableScope scope) noexcept
	{
		const char * attributeName = nullptr;
		const char * vectorName = nullptr;

		switch ( vectorType )
		{
			case VertexAttributeType::Tangent :
				attributeName = Attribute::Tangent;
				vectorName = ShaderVariable::TangentWorldSpace;
				break;

			case VertexAttributeType::Binormal :
				attributeName = Attribute::Binormal;
				vectorName = ShaderVariable::BinormalWorldSpace;
				break;

			case VertexAttributeType::Normal :
				attributeName = Attribute::Normal;
				vectorName = ShaderVariable::NormalWorldSpace;
				break;

			/* NOTE: Ignorable "Unreachable code" warning. */
			default :
				Tracer::error(ClassId, "The synthetic vertex vector in world space can only be the tangent, the bi-normal or the normal !");

				return false;
		}

		if ( !this->declare(InputAttribute{vectorType}) )
		{
			return false;
		}

		std::stringstream code{};

		code << '\t';

		if ( scope == VariableScope::Local )
		{
			code << "const vec3 ";
		}
		else if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector3, vectorName, GLSL::Smooth}) )
		{
			return false;
		}

		std::string modelMatrix;

		if ( this->isInstancingEnabled() )
		{
			if ( !this->declare(InputAttribute{VertexAttributeType::ModelMatrixR0}) )
			{
				return false;
			}

			modelMatrix = Attribute::ModelMatrix;
		}
		else
		{
			modelMatrix = MatrixPC(PushConstant::Component::ModelMatrix);
		}

		/* TODO: Check why the last component should be 0.0 ? */
		code << vectorName << " = (" << modelMatrix << " * vec4(" << attributeName << ", 1.0)).xyz;" "\n";

		if ( scope != VariableScope::ToNextStage )
		{
			topInstructions.append(code.str());
		}
		else
		{
			outputInstructions.append(code.str());
		}

		return true;
	}

	bool
	VertexShader::synthesizeVertexVectorInViewSpace (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, VertexAttributeType vectorType, VariableScope scope) noexcept
	{
		const char * attributeName = nullptr;
		const char * vectorName = nullptr;

		switch ( vectorType )
		{
			case VertexAttributeType::Tangent :
				attributeName = Attribute::Tangent;
				vectorName = ShaderVariable::TangentViewSpace;
				break;

			case VertexAttributeType::Binormal :
				attributeName = Attribute::Binormal;
				vectorName = ShaderVariable::BinormalViewSpace;
				break;

			case VertexAttributeType::Normal :
				attributeName = Attribute::Normal;
				vectorName = ShaderVariable::NormalViewSpace;
				break;

			/* NOTE: Ignorable "Unreachable code" warning. */
			default :
				Tracer::error(ClassId, "The synthetic vertex vector in view space can only be the tangent, the bi-normal or the normal !");

				return false;
		}

		if ( !this->declare(InputAttribute{vectorType}) )
		{
			return false;
		}

		std::stringstream code{};

		code << '\t';

		if ( scope == VariableScope::Local )
		{
			code << "const vec3 ";
		}
		else if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector3, vectorName, GLSL::Smooth}) )
		{
			return false;
		}

		if ( !this->prepareNormalMatrix() )
		{
			return false;
		}

		code << vectorName << " = normalize(" << ShaderVariable::NormalMatrix << " * " << attributeName << ");" "\n";

		if ( scope != VariableScope::ToNextStage )
		{
			topInstructions.append(code.str());
		}
		else
		{
			outputInstructions.append(code.str());
		}

		return true;
	}

	bool
	VertexShader::synthesizeWorldTBNMatrix (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, VariableScope scope) noexcept
	{
		if ( !this->declare(InputAttribute{VertexAttributeType::Tangent}) )
		{
			return false;
		}

		if ( !this->declare(InputAttribute{VertexAttributeType::Binormal}) )
		{
			return false;
		}

		if ( !this->declare(InputAttribute{VertexAttributeType::Normal}, true) )
		{
			return false;
		}

		if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(3), GLSL::Matrix3, ShaderVariable::WorldTBNMatrix, GLSL::Smooth}) )
		{
			return false;
		}

		std::string modelMatrix;

		if ( this->isInstancingEnabled() )
		{
			if ( !this->declare(InputAttribute{VertexAttributeType::ModelMatrixR0}) )
			{
				return false;
			}

			modelMatrix = Attribute::ModelMatrix;
		}
		else
		{
			modelMatrix = MatrixPC(PushConstant::Component::ModelMatrix);
		}

		topInstructions.append((std::stringstream{} <<
			"	const vec3 worldT = normalize((" << modelMatrix << " * vec4(" << Attribute::Tangent << ", 0.0)).xyz);" "\n"
			"	const vec3 worldB = normalize((" << modelMatrix << " * vec4(" << Attribute::Binormal << ", 0.0)).xyz);" "\n"
			"	const vec3 worldN = normalize((" << modelMatrix << " * vec4(" << Attribute::Normal << ", 0.0)).xyz);" "\n"
		).str());

		const auto matrixCode = (std::stringstream{} <<
			'\t' << ShaderVariable::WorldTBNMatrix << " = mat3(worldT, worldB, worldN);" "\n"
		).str();

		if ( scope != VariableScope::ToNextStage )
		{
			topInstructions.append(matrixCode);
		}
		else
		{
			outputInstructions.append(matrixCode);
		}

		return true;
	}

	bool
	VertexShader::synthesizeViewTBNMatrix (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, VariableScope scope) noexcept
	{
		if ( !this->declare(InputAttribute{VertexAttributeType::Tangent}) )
		{
			return false;
		}

		if ( !this->declare(InputAttribute{VertexAttributeType::Binormal}) )
		{
			return false;
		}

		if ( !this->declare(InputAttribute{VertexAttributeType::Normal}, true) )
		{
			return false;
		}

		if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(3), GLSL::Matrix3, ShaderVariable::ViewTBNMatrix, GLSL::Smooth}) )
		{
			return false;
		}

		if ( !this->prepareNormalMatrix() )
		{
			return false;
		}

		topInstructions.append((std::stringstream{} <<
			"	const vec3 viewT = normalize(" << ShaderVariable::NormalMatrix << " * " << Attribute::Tangent << ");" "\n"
			"	const vec3 viewB = normalize(" << ShaderVariable::NormalMatrix << " * " << Attribute::Binormal << ");" "\n"
			"	const vec3 viewN = normalize(" << ShaderVariable::NormalMatrix << " * " << Attribute::Normal << ");" "\n"
		).str());

		const auto matrixCode = (std::stringstream{} << '\t' << ShaderVariable::ViewTBNMatrix << " = transpose(mat3(viewT, viewB, viewN));" "\n").str();

		if ( scope != VariableScope::ToNextStage )
		{
			topInstructions.append(matrixCode);
		}
		else
		{
			outputInstructions.append(matrixCode);
		}

		return true;
	}

	bool
	VertexShader::synthesizeWorldToTangentMatrix (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, VariableScope scope) noexcept
	{
		if ( !this->declare(InputAttribute{VertexAttributeType::Tangent}) )
		{
			return false;
		}

		if ( !this->declare(InputAttribute{VertexAttributeType::Binormal}) )
		{
			return false;
		}

		if ( !this->declare(InputAttribute{VertexAttributeType::Normal}, true) )
		{
			return false;
		}

		if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(3), GLSL::Matrix3, ShaderVariable::WorldToTangentMatrix, GLSL::Smooth}) )
		{
			return false;
		}

		if ( !this->prepareNormalMatrix() )
		{
			return false;
		}

		const auto matrixCode = (std::stringstream{} <<
			'\t' << ShaderVariable::WorldToTangentMatrix << " = " << ShaderVariable::NormalMatrix << " * mat3(" << Attribute::Tangent << ", " << Attribute::Binormal << ", " << Attribute::Normal << ");" "\n"
		).str();

		if ( scope != VariableScope::ToNextStage )
		{
			topInstructions.append(matrixCode);
		}
		else
		{
			outputInstructions.append(matrixCode);
		}

		return true;
	}

	bool
	VertexShader::synthesizeRequestInstructions (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions) noexcept
	{
		for ( const auto & [variableType, variableScope] : m_requests )
		{
			if ( std::strcmp(variableType, ShaderVariable::PositionScreenSpace) == 0 )
			{
				if ( !this->synthesizeVertexPositionInScreenSpace(outputInstructions) )
				{
					return false;
				}

				continue;
			}

			if ( std::strcmp(variableType, ShaderVariable::PositionWorldSpace) == 0 )
			{
				if ( !this->synthesizeVertexPositionInWorldSpace(generator, topInstructions, outputInstructions, variableScope, false) )
				{
					return false;
				}

				continue;
			}

			if ( std::strcmp(variableType, ShaderVariable::GLPositionWorldSpace) == 0 )
			{
				if ( !this->synthesizeVertexPositionInWorldSpace(generator, topInstructions, outputInstructions, variableScope, true) )
				{
					return false;
				}

				continue;
			}

			if ( std::strcmp(variableType, ShaderVariable::PositionViewSpace) == 0 )
			{
				if ( !this->synthesizeVertexPositionInViewSpace(generator, topInstructions, outputInstructions, variableScope) )
				{
					return false;
				}

				continue;
			}

			if ( std::strcmp(variableType, ShaderVariable::PositionTextureSpace) == 0 )
			{
				if ( !this->synthesizeVertexPositionInTextureSpace(generator, topInstructions, outputInstructions, variableScope) )
				{
					return false;
				}

				continue;
			}

			if ( std::strcmp(variableType, ShaderVariable::PrimaryVertexColor) == 0 )
			{
				if ( !this->synthesizeVertexColor(generator, outputInstructions) )
				{
					return false;
				}

				continue;
			}

			/* Texture coordinates flavors. */
			{
				constexpr std::array< const char *, 4 > textureCoordinates{
					ShaderVariable::Primary2DTextureCoordinates,
					ShaderVariable::Primary3DTextureCoordinates,
					ShaderVariable::Secondary2DTextureCoordinates,
					ShaderVariable::Secondary3DTextureCoordinates
				};

				bool found = false;

				for ( const auto & shaderVariable : textureCoordinates )
				{
					if ( std::strcmp(variableType, shaderVariable) != 0 )
					{
						continue;
					}

					if ( !this->synthesizeVertexTextureCoordinates(generator, outputInstructions, shaderVariable) )
					{
						return false;
					}

					found = true;
				}

				if ( found )
				{
					continue;
				}
			}

			if ( std::strcmp(variableType, ShaderVariable::TangentWorldSpace) == 0 )
			{
				if ( !this->synthesizeVertexVectorInWorldSpace(generator, topInstructions, outputInstructions, VertexAttributeType::Tangent, variableScope) )
				{
					return false;
				}

				continue;
			}

			if ( std::strcmp(variableType, ShaderVariable::TangentViewSpace) == 0 )
			{
				if ( !this->synthesizeVertexVectorInViewSpace(generator, topInstructions, outputInstructions, VertexAttributeType::Tangent, variableScope) )
				{
					return false;
				}

				continue;
			}

			if ( std::strcmp(variableType, ShaderVariable::BinormalWorldSpace) == 0 )
			{
				if ( !this->synthesizeVertexVectorInWorldSpace(generator, topInstructions, outputInstructions, VertexAttributeType::Binormal, variableScope) )
				{
					return false;
				}

				continue;
			}

			if ( std::strcmp(variableType, ShaderVariable::BinormalViewSpace) == 0 )
			{
				if ( !this->synthesizeVertexVectorInViewSpace(generator, topInstructions, outputInstructions, VertexAttributeType::Binormal, variableScope) )
				{
					return false;
				}

				continue;
			}

			if ( std::strcmp(variableType, ShaderVariable::NormalWorldSpace) == 0 )
			{
				if ( !this->synthesizeVertexVectorInWorldSpace(generator, topInstructions, outputInstructions, VertexAttributeType::Normal, variableScope) )
				{
					return false;
				}

				continue;
			}

			if ( std::strcmp(variableType, ShaderVariable::NormalViewSpace) == 0 )
			{
				if ( !this->synthesizeVertexVectorInViewSpace(generator, topInstructions, outputInstructions, VertexAttributeType::Normal, variableScope) )
				{
					return false;
				}

				continue;
			}

			if ( std::strcmp(variableType, ShaderVariable::WorldTBNMatrix) == 0 )
			{
				if ( !this->synthesizeWorldTBNMatrix(generator, topInstructions, outputInstructions, variableScope) )
				{
					return false;
				}

				continue;
			}

			if ( std::strcmp(variableType, ShaderVariable::ViewTBNMatrix) == 0 )
			{
				if ( !this->synthesizeViewTBNMatrix(generator, topInstructions, outputInstructions, variableScope) )
				{
					return false;
				}

				continue;
			}


			if ( std::strcmp(variableType, ShaderVariable::WorldToTangentMatrix) == 0 )
			{
				if ( !this->synthesizeWorldToTangentMatrix(generator, topInstructions, outputInstructions, variableScope) )
				{
					return false;
				}

				continue;
			}
		}

		return true;
	}

	bool
	VertexShader::generateMainUniqueInstructions (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions) noexcept
	{
		std::string tempTopInstructions{};

		if ( !this->synthesizeRequestInstructions(generator, tempTopInstructions, outputInstructions) )
		{
			return false;
		}

		if ( !m_uniquePreparations.empty() )
		{
			for ( const auto & instruction : std::ranges::views::values(m_uniquePreparations) )
			{
				topInstructions += instruction;
			}

			topInstructions += '\n';
		}

		topInstructions += tempTopInstructions;

		return true;
	}

	bool
	VertexShader::isSyntheticVariableAllowed (const char * variableName) noexcept
	{
		constexpr std::array< const char *, 19 > variables{
			ShaderVariable::PositionScreenSpace,
			ShaderVariable::PositionWorldSpace,
			ShaderVariable::GLPositionWorldSpace,
			ShaderVariable::PositionViewSpace,
			ShaderVariable::PositionTextureSpace,
			ShaderVariable::PrimaryVertexColor,
			ShaderVariable::Primary2DTextureCoordinates,
			ShaderVariable::Primary3DTextureCoordinates,
			ShaderVariable::Secondary2DTextureCoordinates,
			ShaderVariable::Secondary3DTextureCoordinates,
			ShaderVariable::TangentWorldSpace,
			ShaderVariable::TangentViewSpace,
			ShaderVariable::BinormalWorldSpace,
			ShaderVariable::BinormalViewSpace,
			ShaderVariable::NormalWorldSpace,
			ShaderVariable::NormalViewSpace,
			ShaderVariable::WorldTBNMatrix,
			ShaderVariable::ViewTBNMatrix,
			ShaderVariable::WorldToTangentMatrix
		};

		return std::ranges::any_of(variables, [variableName] (const auto & name) {
			return std::strcmp(name, variableName) == 0;
		});
	}

	bool
	VertexShader::requestSynthesizeInstruction (const char * variableName, VariableScope scope) noexcept
	{
		if ( !VertexShader::isSyntheticVariableAllowed(variableName) )
		{
			TraceError{ClassId} << "Unable to synthesize '" << variableName << "' variable for " << to_string(this->type()) << " '" << this->name() << "' !";

			return false;
		}

		auto requestFound = false;

		for ( auto & [requestName, requestScope] : m_requests )
		{
			if ( std::strcmp(requestName, variableName) == 0  )
			{
				/* NOTE: The variable is present, but needs to override the scope. */
				if ( requestScope != scope )
				{
					requestScope = VariableScope::Both;
				}

				requestFound = true;
			}
		}

		if ( !requestFound )
		{
			/* NOTE: Prerequisite for special shader variables.
			 * NOTE²: Ignorable Clang-Tidy warning for "Function XXX is within a recursive call chain". */
			if ( std::strcmp(variableName, ShaderVariable::PositionTextureSpace) == 0 )
			{
				if ( !this->requestSynthesizeInstruction(ShaderVariable::PositionViewSpace, VariableScope::Local) )
				{
					return false;
				}

				if ( !this->requestSynthesizeInstruction(ShaderVariable::TangentViewSpace, VariableScope::Local) )
				{
					return false;
				}

				if ( !this->requestSynthesizeInstruction(ShaderVariable::BinormalViewSpace, VariableScope::Local) )
				{
					return false;
				}

				if ( !this->requestSynthesizeInstruction(ShaderVariable::NormalViewSpace, VariableScope::Local) )
				{
					return false;
				}
			}

			m_requests.emplace_back(variableName, scope);
		}

		return true;
	}

	bool
	VertexShader::onSourceCodeGeneration (Generator::Abstract & generator, std::stringstream & code, std::string & topInstructions, std::string & outputInstructions) noexcept
	{
		/* NOTE: This will add some declarations. */
		if ( !this->generateMainUniqueInstructions(generator, topInstructions, outputInstructions) )
		{
			return false;
		}

		/* Specific input shader code declarations. */
		generateDeclarations(code, m_inputAttributes, "Input vertex attributes (Vertex shader only)");

		/* Specific output shader code declarations. */
		generateDeclarations(code, m_stageOutputs, "Stage outputs (To next stage)");
		generateDeclarations(code, m_outputBlocks, "Output blocks (To next stage)");

		return true;
	}

	void
	VertexShader::onGetDeclarationStats (std::stringstream & output) const noexcept
	{
		output <<
		 	"Vertex shader input declarations : " "\n"
			" - Input attribute : " << m_inputAttributes.size() << "\n"

			"Vertex shader output declarations : " "\n"
			" - Output block : " <<  m_outputBlocks.size() << "\n"
			" - Stage input : " << m_stageOutputs.size() << "\n";
	}
}
