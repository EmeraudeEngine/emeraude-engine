/*
 * src/Saphir/VertexShader.hpp
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

#pragma once

/* STL inclusions. */
#include <cstdint>
#include <string>
#include <vector>
#include <set>
#include <array>
#include <utility>

/* Local inclusions for inheritances. */
#include "AbstractShader.hpp"

/* Local inclusions for usages. */
#include "Declaration/InputAttribute.hpp"
#include "Declaration/OutputBlock.hpp"
#include "Declaration/StageOutput.hpp"
#include "Graphics/Types.hpp"
#include "Types.hpp"

namespace EmEn::Saphir
{
	/** @brief Defines the scope of a synthesized variable. */
	enum class VariableScope : uint8_t
	{
		/** @brief The variable is only used in the current shader. */
		Local,
		/** @brief The variable is only an output for the next stage. */
		ToNextStage,
		/** @brief The variable is used in the shader and an output for the next stage. */
		Both
	};

	/**
	 * @brief The vertex shader class.
	 * @extends EmEn::Saphir::AbstractShader The base class of every shader type.
	 */
	class VertexShader final : public AbstractShader
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VertexShader"};

			/** @brief Extends declaration from base class. */
			using AbstractShader::declare;

			/**
			 * @brief Constructs a vertex shader.
			 * @param name The name of the shader for identification.
			 * @param GLSLVersion The GLSL version used for this code.
			 * @param GLSLProfile The GLSL profile used for this code.
			 */
			VertexShader (const std::string & name, const std::string & GLSLVersion, const std::string & GLSLProfile) noexcept;

			/** @copydoc EmEn::Saphir::AbstractShader::type() */
			[[nodiscard]]
			ShaderType
			type () const noexcept override
			{
				return ShaderType::VertexShader;
			}

			/**
			 * @brief Declares a vertex attribute to be used in a vertex shader.
			 * @param declaration A reference to a shader input attribute.
			 * @param quiet Disable message from already existing attributes. Default false.
			 * @return bool
			 */
			bool declare (const Declaration::InputAttribute & declaration, bool quiet = false) noexcept;

			/**
			 * @brief Declares a stage output variable to be used in the shader.
			 * @param declaration A reference to a ShaderStageOutput.
			 * @return bool
			 */
			bool declare (const Declaration::StageOutput & declaration) noexcept;

			/**
			 * @brief Declares an output block to be used in the shader.
			 * @param declaration A reference to an OutputBlock.
			 * @return bool
			 */
			bool declare (const Declaration::OutputBlock & declaration) noexcept;

			/**
			 * @brief Returns the list of input attribute declarations.
			 * @return const std::vector< Declaration::InputAttribute > &
			 */
			[[nodiscard]]
			const std::vector< Declaration::InputAttribute > &
			inputAttributes () const noexcept
			{
				return m_inputAttributes;
			}

			/**
			 * @brief Returns the list of stage output declarations.
			 * @return const std::vector< Declaration::StageOutput > &
			 */
			[[nodiscard]]
			const std::vector< Declaration::StageOutput > &
			stageOutputs () const noexcept
			{
				return m_stageOutputs;
			}

			/**
			 * @brief Returns the list of output block declarations.
			 * @return const std::vector< Declaration::OutputBlock > &
			 */
			[[nodiscard]]
			const std::vector< Declaration::OutputBlock > &
			outputBlocks () const noexcept
			{
				return m_outputBlocks;
			}

			/**
			 * @brief Requests to synthesize an instruction in the vertex shader.
			 * @warning This function is recursive for the variable "PositionTextureSpace".
			 * @param variableName The variable name to synthesize. This should be a key from Keys::ShaderVariables namespace.
			 * @param scope Set the variable scope in the vertex shader. Default VariableScope::ToNextStage.
			 * @return bool
			 */
			bool requestSynthesizeInstruction (const char * variableName, VariableScope scope = VariableScope::ToNextStage) noexcept;

			/**
			 * @brief Returns a list of requested vertex attributes in the shader.
			 * @return const std::set< Graphics::VertexAttributeType > &
			 */
			[[nodiscard]]
			const std::set< Graphics::VertexAttributeType > &
			vertexAttributes () const noexcept
			{
				return m_vertexAttributes;
			}

			/**
			 * @brief Sets the vertex shader uses instancing.
			 * @return void
			 */
			void
			enableInstancing () noexcept
			{
				m_flags[InstancingEnabled] = true;
			}

			/**
			 * @brief Returns whether the vertex shader is using instancing.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isInstancingEnabled () const noexcept
			{
				return m_flags[InstancingEnabled];
			}

			/**
			 * @brief Enables the need of advanced matrices (for lighting or reflection).
			 * @return void
			 */
			void
			enableAdvancedMatrices () noexcept
			{
				m_flags[AdvancedMatricesEnabled] = true;
			}

			/**
			 * @brief Returns whether the advances matrices are enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isAdvancedMatricesEnabled () const noexcept
			{
				return m_flags[AdvancedMatricesEnabled];
			}

			/**
			 * @brief Enables bill boarding render.
			 * @return void
			 */
			void
			enableBillBoarding () noexcept
			{
				m_flags[BillBoardingEnabled] = true;
			}

			/**
			 * @brief Returns whether the vertex shader is rendering bill boards.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isBillBoardingEnabled () const noexcept
			{
				return m_flags[BillBoardingEnabled];
			}

		private:

			/** @copydoc EmEn::Saphir::AbstractShader::onSourceCodeGeneration() */
			[[nodiscard]]
			bool onSourceCodeGeneration (Generator::Abstract & generator, std::stringstream & code, std::string & topInstructions, std::string & outputInstructions) noexcept override;

			/** @copydoc EmEn::Saphir::AbstractShader::onGetDeclarationStats() */
			void onGetDeclarationStats (std::stringstream & output) const noexcept override;

			/**
			 * @brief Returns whether a variable preparation has already been asked.
			 * @param preparation The name of the variable.
			 * @return bool
			 */
			[[nodiscard]]
			bool preparationAlreadyDone (const char * preparation) const noexcept;

			/**
			 * @brief Creates a local variable for sprite model matrix with VBO.
			 * @return bool
			 */
			[[nodiscard]]
			bool prepareSpriteModelMatrix () noexcept;

			/**
			 * @brief Creates a local variable for modelView matrix with VBO.
			 * @return bool
			 */
			[[nodiscard]]
			bool prepareModelViewMatrix () noexcept;

			/**
			 * @brief Creates a local variable for normal matrix with VBO.
			 * @return bool
			 */
			[[nodiscard]]
			bool prepareNormalMatrix () noexcept;

			/**
			 * @brief Creates a local variable for modelViewProjection matrix with VBO.
			 * @return bool
			 */
			[[nodiscard]]
			bool prepareModelViewProjectionMatrix () noexcept;

			/**
			 * @brief Synthesizes the vertex position in world space in the vertex shader.
			 * @note gl_Position = gl_modelMatrix * gl_Vertex;
			 * @param generator A reference to the shader generator.
			 * @param topInstructions Every instruction that should be on the top of the main() function.
			 * @param outputInstructions Every instruction that should be at the end of the main() function.
			 * @param scope Set the variable scope in the vertex shader.
			 * @param asGLStandardPosition Synthesize as gl_Position for specific shaders.
			 * @return bool
			 */
			bool synthesizeVertexPositionInWorldSpace (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, VariableScope scope, bool asGLStandardPosition = false) noexcept;

			/**
			 * @brief Synthesizes the vertex position in view space in the vertex shader.
			 * @note gl_Position = gl_modelViewMatrix * gl_Vertex;
			 * @param generator A reference to the shader generator.
			 * @param topInstructions Every instruction that should be on the top of the main() function.
			 * @param outputInstructions Every instruction that should be at the end of the main() function.
			 * @param scope Set the variable scope in the vertex shader.
			 * @return bool
			 */
			bool synthesizeVertexPositionInViewSpace (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, VariableScope scope) noexcept;

			/**
			 * @brief Synthesizes the vertex position in screen space in the vertex shader.
			 * @note gl_Position = gl_modelViewProjectionMatrix * gl_Vertex;
			 * @param outputInstructions Every instruction that should be at the end of the main() function.
			 * @return bool
			 */
			bool synthesizeVertexPositionInScreenSpace (std::string & outputInstructions) noexcept;

			/**
			 * @brief Synthesizes the vertex position in texture space in the vertex shader.
			 * @note gl_Position = gl_modelViewProjectionMatrix * gl_Vertex;
			 * @param generator A reference to the shader generator.
			 * @param topInstructions Every instruction that should be on the top of the main() function.
			 * @param outputInstructions Every instruction that should be at the end of the main() function.
			 * @param scope Set the variable scope in the vertex shader.
			 * @return bool
			 */
			bool synthesizeVertexPositionInTextureSpace (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, VariableScope scope) noexcept;

			/**
			 * @brief Synthesizes the vertex color in the vertex shader.
			 * @note This will not generate a local variable.
			 * @param generator A reference to the shader generator.
			 * @param outputInstructions Every instruction that should be at the end of the main() function.
			 * @return bool
			 */
			bool synthesizeVertexColor (Generator::Abstract & generator, std::string & outputInstructions) noexcept;

			/**
			 * @brief Synthesizes the vertex texture coordinates in the vertex shader.
			 * @note This will not generate a local variable.
			 * @param generator A reference to the shader generator.
			 * @param outputInstructions Every instruction that should be at the end of the main() function.
			 * @param TCVariableName The name of the texture coordinates variable.
			 * @return bool
			 */
			bool synthesizeVertexTextureCoordinates (Generator::Abstract & generator, std::string & outputInstructions, const char * TCVariableName) noexcept;

			/**
			 * @brief Synthesizes the vertex tangent, bi-normal or normal vector in world space in the vertex shader.
			 * @param generator A reference to the shader generator.
			 * @param topInstructions Every instruction that should be on the top of the main() function.
			 * @param outputInstructions Every instruction that should be at the end of the main() function.
			 * @param vectorType The vector type to generate.
			 * @param scope Set the variable scope in the vertex shader.
			 * @return bool
			 */
			bool synthesizeVertexVectorInWorldSpace (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, Graphics::VertexAttributeType vectorType, VariableScope scope) noexcept;

			/**
			 * @brief Synthesizes the vertex tangent, bi-normal or normal vector in view space variable in the vertex shader.
			 * @param generator A reference to the shader generator.
			 * @param topInstructions Every instruction that should be on the top of the main() function.
			 * @param outputInstructions Every instruction that should be at the end of the main() function.
			 * @param vectorType The vector type to generate.
			 * @param scope Set the variable scope in the vertex shader.
			 * @return bool
			 */
			bool synthesizeVertexVectorInViewSpace (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, Graphics::VertexAttributeType vectorType, VariableScope scope) noexcept;

			/**
			 * @brief synthesizeWorldTBNMatrix
			 * @param generator A reference to the shader generator.
			 * @param topInstructions Every instruction that should be on the top of the main() function.
			 * @param outputInstructions Every instruction that should be at the end of the main() function.
			 * @param scope Set the variable scope in the vertex shader.
			 * @return bool
			 */
			bool synthesizeWorldTBNMatrix (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, VariableScope scope) noexcept;

			/**
			 * @brief synthesizeViewTBNMatrix
			 * @param generator A reference to the shader generator.
			 * @param topInstructions Every instruction that should be on the top of the main() function.
			 * @param outputInstructions Every instruction that should be at the end of the main() function.
			 * @param scope Set the variable scope in the vertex shader.
			 * @return bool
			 */
			bool synthesizeViewTBNMatrix (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, VariableScope scope) noexcept;

			/**
			 * @brief Synthesizes the matrix to get the vertex normal in world space from a normal map.
			 * @param generator A reference to the shader generator.
			 * @param topInstructions Every instruction that should be on the top of the main() function.
			 * @param outputInstructions Every instruction that should be at the end of the main() function.
			 * @param scope Set the variable scope in the vertex shader.
			 * @return bool
			 */
			bool synthesizeWorldToTangentMatrix (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, VariableScope scope) noexcept;

			/**
			 * @brief Synthesizes instructions before generating the final source code.
			 * @param generator A reference to the shader generator.
			 * @param topInstructions Every instruction that should be on the top of the main() function.
			 * @param outputInstructions Every instruction that should be at the end of the main() function.
			 * @return bool
			 */
			bool synthesizeRequestInstructions (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions) noexcept;

			/**
			 * @brief Generates final unique instructions.
			 * @param generator A reference to the shader generator.
			 * @param topInstructions A reference to the top instructions string.
			 * @param outputInstructions  A reference to the output instructions string.
			 * @return bool
			 */
			[[nodiscard]]
			bool generateMainUniqueInstructions (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions) noexcept;

			/**
			 * @brief Checks whether a synthetic variable is allowed.
			 * @param variableName A pointer to a C-string for the variable name.
			 * @return bool
			 */
			[[nodiscard]]
			static bool isSyntheticVariableAllowed (const char * variableName) noexcept;

			[[nodiscard]]
			static Declaration::Function generateComputeUpwardVectorFunction () noexcept;

			[[nodiscard]]
			static Declaration::Function generateGetBillBoardModelMatrixFunction () noexcept;

			/* Flag names */
			static constexpr auto InstancingEnabled{0UL};
			static constexpr auto AdvancedMatricesEnabled{1UL};
			static constexpr auto BillBoardingEnabled{2UL};

			std::vector< std::pair< const char *, std::string > > m_uniquePreparations; // ie normalMatrix for VBO
			std::vector< std::pair< const char *, VariableScope > > m_requests;
			std::vector< Declaration::InputAttribute > m_inputAttributes;
			std::vector< Declaration::StageOutput > m_stageOutputs;
			std::vector< Declaration::OutputBlock > m_outputBlocks;
			std::set< Graphics::VertexAttributeType > m_vertexAttributes;
			std::array< bool, 8 > m_flags{
				false/*InstancingEnabled*/,
				false/*AdvancedMatricesEnabled*/,
				false/*BillBoardingEnabled*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};
}
